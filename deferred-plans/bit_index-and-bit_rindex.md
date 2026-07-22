# Future Proposal Plan

This file documents `bit_index` and `bit_rindex`, the scalar bit-scan primitives, intentionally excluded from the main proposal in docs/Main.md.

## Scalar Bit Scan

### Reason for deferral

`bit_index` and `bit_rindex` are derivable from `each_bit_offset`: the result of `bit_index(bit, start_offset)` is the same as `each_bit_offset(bit, start_offset).first`, and `bit_rindex` corresponds to the last yielded position scanning backward. They therefore add no new capability to the proposal, only a zero-allocation scalar form, so the core proposal omits them to keep its surface minimal.

They become compelling specifically under an AOT-compiled, systems-programming workload. `each_bit_offset(bit, start_offset).first` allocates an `Enumerator` on every call, which is unacceptable inside the inner loop of a bitmap allocator. A dedicated scalar method allocates nothing and lowers directly to a single hardware bit-scan instruction (`ffs`/`clz`/`ctz`, x86 `BSF`/`BSR`, ARM `RBIT`+`CLZ`). This is the canonical "find first/next set or clear bit" primitive of every kernel-level bitmap (`find_first_bit`, `find_next_zero_bit`).

## `bit_index(bit, start_offset=0, lsb_first: true) -> Integer | nil`

Returns the flat bit position of the first bit equal to `bit`, scanning forward from `start_offset` toward higher bit positions. Returns `nil` if no matching bit exists at or after `start_offset`.

`bit` accepts `false`, `true`, `0`, or `1` (`0`/`1` are aliases for `false`/`true`), matching the values used by `each_bit_offset` and `bit_run_count`.

A negative `start_offset` counts backward from the end, exactly as in `String#index`. `lsb_first:` selects which physical bit each flat position refers to, using the same convention as `bit_set?`.

```ruby
data = "\xF0".b           # LSB-first: bits 0-3 are 0, bits 4-7 are 1

data.bit_index(true)      #=> 4    (first set bit)
data.bit_index(false)     #=> 0    (first clear bit)
data.bit_index(true, 5)   #=> 5    (first set bit at or after position 5)
"\x00".bit_index(true)    #=> nil  (no set bit)

# lsb_first: false changes which physical bit each position refers to
"\xF0".bit_index(true, lsb_first: false)   #=> 0   (MSB-first: bits 0-3 are 1)
"\xF0".bit_index(false, lsb_first: false)  #=> 4
```

## `bit_rindex(bit, start_offset=bytesize*8-1, lsb_first: true) -> Integer | nil`

Returns the flat bit position of the last bit equal to `bit`, scanning backward from `start_offset` toward lower bit positions (default: the final bit). Returns `nil` if no matching bit exists at or before `start_offset`.

The relationship to `bit_index` mirrors `String#rindex` to `String#index`: same arguments, opposite scan direction. A negative `start_offset` counts backward from the end.

```ruby
data = "\xF0".b           # LSB-first: bits 0-3 are 0, bits 4-7 are 1

data.bit_rindex(true)     #=> 7    (last set bit)
data.bit_rindex(false)    #=> 3    (last clear bit)
data.bit_rindex(true, 5)  #=> 5    (last set bit at or before position 5)
```

### Use case --- bitmap allocator (find first free, find next free)

The core scan of a first-fit or next-fit allocator. `bit_index(false)` is the "find first zero bit" primitive; passing a cursor turns it into "find next free bit" without restarting at position 0:

```ruby
free = bitmap.bit_index(false)              # first free block, or nil if full
free = bitmap.bit_index(false, last_alloc)  # next-fit: scan forward from cursor
```

`bit_rindex(true)` recovers the high-water mark --- the position of the last allocated block --- for trimming or compaction:

```ruby
high_water = bitmap.bit_rindex(true)        # nil if the bitmap is entirely free
```

Compared with `each_bit_offset(false).first`, the scalar form allocates no `Enumerator`, which is what makes it viable in an AOT-compiled hot loop.

### Use case for `lsb_first: false`

Locating a bit in an MSB-first wire bitfield by its spec coordinate: the first owned piece in a BitTorrent bitfield, or the first set flag in an RFC bitmask. The returned position is directly usable as the spec-defined index.

```ruby
first_owned = bitfield.bit_index(true, lsb_first: false)
```
