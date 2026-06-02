# Proposed Methods

| category | methods                                                       | keyword param | allocates result object      |
|----------|---------------------------------------------------------------|---------------|------------------------------|
| Read     | `bit_at`                                                      | `lsb_first:`  | no                           |
| Read     | `bit_count`                                                   | `lsb_first:`  | no                           |
| Read     | `bit_run_count`                                               | `lsb_first:`  | no                           |
| Iterator | `each_bit`, `bits`                                            | `lsb_first:`  | `bits` only (`Array`)        |
| Iterator | `each_bit_run`, `bit_runs`                                    | `lsb_first:`  | `bit_runs` only (`Array`)    |
| Iterator | `each_bit_offset`, `bit_offsets`                              | `lsb_first:`  | `bit_offsets` only (`Array`) |
| Mutation | `bit_set`, `bit_clear`, `bit_flip`                            | `lsb_first:`  | no                           |
| Mutation | `bit_splice`                                                  | `lsb_first:`  | no                           |
| Slice    | `bit_slice`                                                   | `lsb_first:`  | yes (`String`)               |
| Bitwise  | `bitwise_not!`, `bitwise_and!`, `bitwise_or!`, `bitwise_xor!` | no            | no                           |
| Bitwise  | `bitwise_not`, `bitwise_and`, `bitwise_or`, `bitwise_xor`     | no            | yes (`String`)               |

Methods are grouped by what they do to the bitmap:

- **Read** -- inspect bits without modifying the receiver
- **Iterator** -- walk every bit and yield each one (or its position / run)
- **Mutation** -- modify the receiver in place
- **Slice** -- extract a sub-sequence of bits into a new `String`
- **Bitwise** -- whole-bitmap logical operations (`!` form is in-place, the non-`!` form returns a new `String`)

A single keyword, `lsb_first:`, controls bit ordering wherever it appears. `Bitwise` methods and `bit_count` are order-independent and take no keyword.

See [Discussion.md#bit-ordering-across-domains](./Discussion.md#bit-position-numbering-of-the-string-bit-api) about `lsb_first:` keyword.

## Read

#### `bit_at(bit_offset, lsb_first: true) -> true | false | nil`

Returns whether bit at flat position `bit_offset` is set. Returns `nil` if `bit_offset` is out of range.

`lsb_first: true` (default) uses LSB-first numbering within each byte. `lsb_first: false` preserves byte order but uses MSB-first numbering within each byte. See [Discussion.md#bit-ordering-across-domains](./Discussion.md#bit-position-numbering-of-the-string-bit-api) for how `bit_offset` maps to a specific bit under each convention.

```ruby
bitmap = "\xFF\xAA"                 # byte[0]=0xFF, byte[1]=0xAA (0b10101010)
bitmap.bit_at(0)                    #=> true  (bit 0 of byte[0])
bitmap.bit_at(0, lsb_first: false)  #=> true  (bit 7 of byte[0])
bitmap.bit_at(8, lsb_first: false)  #=> true  (bit 7 of byte[1])
bitmap.bit_at(100)                  #=> nil
```

Apache Arrow idiom --- check if element `i` is valid:

```ruby
valid = bitmap.bit_at(i)
```

**Use case for `lsb_first: false`:** RFC / wire-format idiom --- read by the RFC diagram "bit 0" convention (leftmost bit of the first byte):

```ruby
header = "\xC0\x00\x00\x00".b           # IPv4 header byte 0 = 0b11000000
header.bit_at(0, lsb_first: false)      #=> true   (version field, leading bit)
header.bit_at(1, lsb_first: false)      #=> true   (version field, second bit)
header.bit_at(2, lsb_first: false)      #=> false
```

---

### `bit_count -> Integer`
### `bit_count(bit_offset, bit_length, lsb_first: true) -> Integer`
### `bit_count(bit_range, lsb_first: true) -> Integer`

Returns the number of set-bits. Without arguments, counts the entire string. With `bit_offset` and `bit_length`, counts only the `bit_length` bits beginning at flat bit position `bit_offset`. With a `bit_range`, counts the bits covered by that range. If the region extends beyond the string, the excess is silently ignored. Returns `0` if `bit_offset` is at or beyond the end of the string.

`lsb_first:` is irrelevant for the no-argument form (a full-string popcount is order-independent) and is silently ignored even if passed. For the `bit_offset`/`bit_length` and `bit_range` forms, `lsb_first:` controls which physical bits a non-byte-aligned `bit_offset` refers to, using the same convention as `bit_at` and `bit_slice`.

Raises `IndexError` for a negative `bit_offset` or a negative Range endpoint. Raises `ArgumentError` for a Bignum argument or a negative `bit_length`.

```ruby
"\x00".bit_count       #=> 0
"\xFF".bit_count       #=> 8
"\xAA".bit_count       #=> 4   # 0b10101010

data = "\xFF\x00\xF0"
data.bit_count(0, 8)   #=> 8   (byte 0: all ones)
data.bit_count(8, 8)   #=> 0   (byte 1: all zeros)
data.bit_count(16, 4)  #=> 0   (byte 2 low nibble in LSB-first: bits 16-19 = 0000)
data.bit_count(20, 4)  #=> 4   (byte 2 high nibble in LSB-first: bits 20-23 = 1111)

data.bit_count(0..7)   #=> 8
data.bit_count(8..15)  #=> 0
data.bit_count(0...8)  #=> 8   # exclusive range

# lsb_first: false changes which physical bits a non-byte-aligned bit_offset refers to
"\xF0".bit_count(0, 4)                   #=> 0  (LSB-first bits 0-3: low nibble = 0000)
"\xF0".bit_count(0, 4, lsb_first: false) #=> 4  (MSB-first bits 0-3: high nibble = 1111)
```

Apache Arrow idiom --- count valid and null elements (note that the bitmap may have unused trailing bits in the last byte, so the column's row count must come from schema metadata, not from `bytesize * 8`):

```ruby
valid_count = bitmap.bit_count
null_count  = row_count - valid_count
```

**Use case for `bit_count(bit_offset, bit_length)`/`bit_count(bit_range)` --- bitmap allocator:** after a first-fit scan finds a candidate free region `offset...(offset + n)`, verify it is entirely free before committing the allocation:

```ruby
if bitmap.bit_count(offset...(offset + n)) == 0
  bitmap.bit_set(offset...(offset + n))
end
```

---

### `bit_run_count(bit, bit_offset, lsb_first: true) -> Integer | nil`

Returns the length of the consecutive run of `bit` starting at flat position `bit_offset`, counting forward toward higher bit indices.

If a run of `bit` starts at `bit_offset`, returns its length as an `Integer`.
Otherwise, returns `nil`. This includes both cases where `bit_offset` is out of range and where the bit at `bit_offset` does not equal `bit`.

`bit` accepts `false`, `true` `0`, or `1` (`0`/`1` are aliases for `false`/`true`, matching the values yielded by `each_bit_run`).

```ruby
data = "\xF0".b           # 11110000 (LSB-first: bits 0-3 are 0, bits 4-7 are 1)

data.bit_run_count(false, 0)  #=> 4  (4 zeros forward from bit 0)
data.bit_run_count(true, 4)   #=> 4  (4 ones forward from bit 4)
data.bit_run_count(true, 0)   #=> nil  (bit 0 is not 1)

data = "\xFF\xFF\x00".b
data.bit_run_count(true, 0)  #=> 16 (16 ones forward from bit 0)
data.bit_run_count(false, 16) #=> 8  (8 zeros forward from bit 16)
data.bit_run_count(false, 24) #=> nil  (out of range)
```

**Use case --- bitmap allocator capacity check:** at a known-free position, measure the free run before committing an allocation. A single `bit_run_count(false, pos)` reports how many contiguous free (`0`) blocks start at `pos`, so the allocator can confirm a request of `n` blocks fits without scanning the rest of the bitmap:

```ruby
run = bitmap.bit_run_count(false, pos)
if run && n <= run
  bitmap.bit_set(pos, n)   # the free run is long enough; take n blocks
end
```

**Use case for `lsb_first: false`:** scalar form of the same UART/PPP bit-stuffing logic --- given a starting position in the MSB-first byte stream just received over the UART, return how many consecutive matching bits follow, so the parser can decide whether to insert or remove a stuffed bit. Runs in MSB-first mode merge across byte boundaries just like `each_bit_run(lsb_first: false)`.

**Why `bit_run_count` never returns 0:** A run is defined as one or more consecutive identical bits, so a run of length zero is a contradiction in terms. When the bit at `bit_offset` does not equal `bit`, there is no run of `bit` starting there --- the correct result is `nil` (no such run exists), not `0` (a run of zero length). This is consistent with `each_bit_run`, which never yields a run length of zero. Note that `nil` unifies two cases that are logically equivalent from the caller's perspective: `bit_offset` is beyond the string boundary, and `bit_offset` is within the string but outside the caller's region of interest for `bit`.

---

## Iterator

### `each_bit(start_offset=0, lsb_first: true) { |bool| ... } -> self`
### `each_bit(start_offset=0, lsb_first: true) -> Enumerator`

Yields each bit as `true` or `false`, starting at flat bit position `start_offset` (default: `0`). Without a block, returns an `Enumerator`. With a block, returns `self`.
`lsb_first: true` walks each byte from LSB to MSB; `lsb_first: false` walks each byte from MSB to LSB. Byte order is always `byte[0]` first.

```ruby
"\xAA".each_bit.to_a
#=> [false, true, false, true, false, true, false, true]
#       (byte 0xAA walked b0 -> b7)

"\xAA".each_bit(lsb_first: false).to_a
#=> [true, false, true, false, true, false, true, false]
#       (byte 0xAA walked b7 -> b0)

"\xFF\xAA".each_bit(8).to_a
#=> [false, true, false, true, false, true, false, true]
#       (starts at bit 8, i.e. byte[1]=0xAA)
```

**Use case for `lsb_first: false`:** walking a packed bit stream that was specified MSB-first --- variable-length codes in JPEG/Deflate Huffman, BitTorrent piece bitfields read in protocol order, or PNG 1-bit scanlines presented left-to-right.

---

### `bits(start_offset=0, lsb_first: true) -> Array`
### `bits(start_offset=0, lsb_first: true) { |bool| ... } -> self`

Without a block, equivalent to `each_bit(start_offset, lsb_first: lsb_first).to_a`. With a block, equivalent to `each_bit(start_offset, lsb_first: lsb_first) { |b| ... }`.

---

### `each_bit_run(start_offset=0, lsb_first: true) { |bool, offset, len| } -> self`
### `each_bit_run(start_offset=0, lsb_first: true) -> Enumerator`

Yields `(bool, offset, run_length)` triples for each consecutive run of identical bits, starting the scan at flat bit position `start_offset` (default: `0`). `offset` is the absolute starting position of the run within the receiver, regardless of `start_offset`.

RLE encoding --- the primary motivation:

```ruby
# with each_bit
runs = [];
current = nil;
count = start = pos = 0
"\xF0".each_bit do |bit|
  if bit == current
    count += 1
  else
    runs << [current, start, count] unless current.nil?
    current = bit
    start = pos
    count = 1
  end
  pos += 1
end
runs << [current, start, count] unless current.nil?

# with each_bit_run
"\xF0".each_bit_run.to_a
#=> [[false, 0, 4], [true, 4, 4]]
```

**Use case for `start_offset` --- bitmap allocator:** scanning for a free run of `n` contiguous blocks starting from a known position `last_alloc` (next-fit strategy):

```ruby
bitmap.each_bit_run(last_alloc) do |bit, offset, len|
  if !bit && len >= n
    bitmap.bit_set(offset...(offset + n))
    last_alloc = offset + n
    break offset
  end
end
```

Without `start_offset`, the scan would always restart at bit 0, and without the yielded `offset` the caller would have to maintain a position counter manually.

**Use case for `lsb_first: false`:** detecting flag-byte boundaries in an MSB-first bit stream received over a UART. For example, a microcontroller talking to a cellular modem over PPP must find five consecutive `1` bits in the incoming UART byte stream --- the PPP/HDLC framing trigger that delimits frames and signals bit-stuffing. Because the protocol is MSB-first while runs straddle byte boundaries, the scan must walk each byte from MSB downward:

```ruby
"\x0F\xF0".each_bit_run(lsb_first: false).to_a
#=> [[false, 0, 4], [true, 4, 8], [false, 12, 4]]
# Runs merge across the byte boundary because the scan is MSB-first.
```

---

### `bit_runs(start_offset=0, lsb_first: true) -> Array`
### `bit_runs(start_offset=0, lsb_first: true) { |bool, offset, len| } -> self`

Without a block, equivalent to `each_bit_run(start_offset, lsb_first: lsb_first).to_a`. With a block, equivalent to `each_bit_run(start_offset, lsb_first: lsb_first) { |bool, offset, len| ... }`.

---

### `each_bit_offset(bit, start_offset=0, lsb_first: true) { |offset| ... } -> self`
### `each_bit_offset(bit, start_offset=0, lsb_first: true) -> Enumerator`

Yields the position of each bit equal to `bit` under the chosen numbering convention, starting the scan at flat bit position `start_offset` (default: `0`). Yielded positions are always absolute within the receiver. Without a block, returns an `Enumerator`. With a block, returns `self`.

`bit` accepts `false`, `true`, `0`, or `1`,  (`0`/`1` are aliases for `false`/`true`, matching the values yielded by `each_bit` and `each_bit_run`).

```
data = "\xAA\xCC"  (byte[0]=0b10101010, byte[1]=0b11001100)

Flat positions of all set-bits (bit=1):

  byte[0]: b1 b3 b5 b7  =>  positions  1,  3,  5,  7
  byte[1]: b2 b3 b6 b7  =>  positions 10, 11, 14, 15

  each_bit_offset(true)                    #=>  1,  3,  5,  7, 10, 11, 14, 15
  each_bit_offset(true, lsb_first: false)  #=>  0,  2,  4,  6,  8,  9, 12, 13

Flat positions of all unset bits (bit=0) are the complement:

  each_bit_offset(false)                   #=>  0,  2,  4,  6,  8,  9, 12, 13
  each_bit_offset(false, lsb_first: false) #=>  1,  3,  5,  7, 10, 11, 14, 15
```

The returned positions use the same numbering convention as `bit_at`:

```ruby
data.each_bit_offset(true, lsb_first: false).all? do |offset|
  data.bit_at(offset, lsb_first: false)
end
#=> true

data.each_bit_offset(false, lsb_first: true).none? do |offset|
  data.bit_at(offset, lsb_first: true)
end
#=> true
```

**Use case for `bit=true`:** enumerating valid element indices from an Apache Arrow validity bitmap, or owned piece indices from a BitTorrent bitfield (`lsb_first: false`; the yielded integers are directly usable as piece indices).

**Use case for `bit=false`:** enumerating null element indices in an Arrow validity bitmap, free blocks in an ext4 block bitmap, or unowned pieces to request next from a BitTorrent peer (`lsb_first: false`).

---

### `bit_offsets(bit, start_offset=0, lsb_first: true) -> Array`
### `bit_offsets(bit, start_offset=0, lsb_first: true) { |offset| ... } -> self`

Without a block, equivalent to `each_bit_offset(bit, start_offset, lsb_first: lsb_first).to_a`. With a block, equivalent to `each_bit_offset(bit, start_offset, lsb_first: lsb_first) { |offset| ... }`.

---

## Mutation

### `bit_set(bit_offset, bit_length=1, lsb_first: true) -> self`
### `bit_set(bit_range, lsb_first: true) -> self`

Sets `bit_length` consecutive logical bits starting at flat bit position `bit_offset` to 1. `bit_length` defaults to 1, so `bit_set(n)` sets a single bit (same as the existing scalar form). The Range form is equivalent to `bit_set(range.first, range.size)`.

```ruby
bitmap = +"\x00\x00".b
bitmap.bit_set(0)                      #=> "\x01\x00"  (1 bit at position 0)
bitmap.bit_set(9)                      #=> "\x01\x02"  (1 bit at position 9)
bitmap.bit_set(4, 8)                   #=> "\xF0\x0F"  (8 bits starting at 4)
bitmap.bit_set(4..11)                  #=> "\xF0\x0F"  (same via Range)
bitmap.bit_set(6..9, lsb_first: false) #=> "\x03\xC0"
bitmap.bit_set(100)                    #=> IndexError
```

Apache Arrow idiom --- build a validity bitmap incrementally:

```ruby
bitmap = +"\x00" * ((row_count + 7) / 8)
rows.each_with_index { |row, i| bitmap.bit_set(i) unless row[:value].nil? }
```

**Use case for `lsb_first: false`:** marking a piece as owned in a BitTorrent bitfield (piece `i` lives at MSB-first position `i`):

```ruby
bitfield.bit_set(piece_index, lsb_first: false)
```

---

### `bit_clear(bit_offset, bit_length=1, lsb_first: true) -> self`
### `bit_clear(bit_range, lsb_first: true) -> self`

Sets `bit_length` consecutive logical bits starting at `bit_offset` to 0. `bit_length` defaults to 1.

```ruby
bitmap = +"\xFF\xFF".b
bitmap.bit_clear(0)                      #=> "\xFE\xFF"
bitmap.bit_clear(8)                      #=> "\xFE\xFE"
bitmap.bit_clear(4, 8)                   #=> "\x0F\xF0"  (8 bits starting at 4)
bitmap.bit_clear(4..11)                  #=> "\x0F\xF0"  (same via Range)
bitmap.bit_clear(6..9, lsb_first: false) #=> "\xFC\x3F"
bitmap.bit_clear(100)                    #=> IndexError
```

**Use case for `lsb_first: false`:** clearing a flag bit in a wire-format header without first reinterpreting positions to byte-local indices.

---

### `bit_flip(bit_offset, bit_length=1, lsb_first: true) -> self`
### `bit_flip(bit_range, lsb_first: true) -> self`

Toggles `bit_length` consecutive logical bits starting at `bit_offset`. `bit_length` defaults to 1.

```ruby
bitmap = +"\x00".b
bitmap.bit_flip(3)                      #=> "\x08"
bitmap.bit_flip(3)                      #=> "\x00"  (back to original)
bitmap.bit_flip(4, 8)                   #=> "\xF0\x0F"  (8 bits starting at 4)
bitmap.bit_flip(4..11)                  #=> "\xF0\x0F"  (same via Range)
bitmap = +"\x00\x00".b
bitmap.bit_flip(6..9, lsb_first: false) #=> "\x03\xC0"
bitmap.bit_flip(100)                    #=> IndexError
```

**Use case for `lsb_first: false`:** toggling a single flag bit in an MSB-first packet header (e.g. an ECN bit in an IP header) using the same diagram coordinate the spec writes.

---

### `bit_splice(bit_offset, bit_length, str, str_bit_offset=0, lsb_first: true) -> self`
### `bit_splice(bit_range, str, str_bit_offset=0, lsb_first: true) -> self`

The bit-granularity analog of `String#bytesplice`. Writes `bit_length` bits from `str` into `self` starting at flat bit position `bit_offset`.

The inverse of `bit_slice`: where `bit_slice` reads a sub-sequence of bits into a new String, `bit_splice` writes one back. Returns `self`.

Unlike `bytesplice`, `bit_splice` does not resize `self`. The destination range always has length `bit_length` (or the length implied by the destination range form). This is the only sensible choice at sub-byte granularity: partial bytes cannot be shifted to make room.

In all forms, the number of bits copied equals the destination length. The optional `str_bit_offset` argument (default 0) sets where reading starts in `str`; the length is always taken from the destination. Raises `IndexError` if the source range falls outside `str`.

A negative `bit_offset`, `bit_length`, `str_bit_offset`, or Range endpoint raises `IndexError`; negative positions are never interpreted as counting backward from the end (see [Discussion.md#error-behavior-for-out-of-range-bit-indices](./Discussion.md#error-behavior-for-out-of-range-bit-indices)).

```ruby
# 3-arg form: write bits 0-7 of "\xFF" into bits 0-7 of buf
buf = +"\x00\x00".b
buf.bit_splice(0, 8, "\xFF")     #=> buf is "\xFF\x00"

# write 4 bits starting at a non-byte-aligned position
buf = +"\x00\x00".b
buf.bit_splice(4, 4, "\x0A")     # 0x0A = 0b00001010; bits 0-3 = 1010
# bits 4-7 of buf[0] become 1010 => 0b10101111 = 0xAF
# buf is "\xAF\x00"

# 4-arg form: copy bits 4-7 of src into bits 0-3 of buf
src = +"\xAA".b   # 0b10101010
buf.bit_splice(0, 4, src, 4)
# src bits 4-7 = 1010; written into buf bits 0-3

# range form (2-arg): same as bit_splice(0, 8, "\x00")
buf.bit_splice(0..7, "\x00")

# range form (3-arg): copy 8 bits of src starting at bit 8 into bits 0..7 of buf
buf.bit_splice(0..7, src, 8)

# source range too short
buf.bit_splice(1, 7, "")         #=> IndexError

# destination range out of bounds
"\xAA\xCC".bit_splice(1, 17, "abcalkjsdcfkljaf") #=> IndexError
```

`lsb_first:` only changes how the **destination position** is interpreted. The physical bit-sequence from the source `str` is preserved, so the result of `bit_slice(..., lsb_first: false)` can be passed straight back to `bit_splice(..., lsb_first: false)` and the round-trip is exact.

Roundtrip symmetry with `bit_slice`:

```ruby
src = Random.bytes(8)
# Extract 12 bits from a non-byte-aligned position
slice = src.bit_slice(4, 12)

# Write them back into a zero buffer at the same position
buf = ("\x00" * src.bytesize).b
buf.bit_splice(4, 12, slice)

buf.bit_slice(4, 12) == slice   #=> true

# The same roundtrip holds under MSB-first coordinates:
buf2 = ("\x00" * src.bytesize).b
slice2 = src.bit_slice(4, 12, lsb_first: false)
buf2.bit_splice(4, 12, slice2, lsb_first: false)
buf2.bit_slice(4, 12, lsb_first: false) == slice2  #=> true
```

Apache Arrow idiom --- overwrite a sub-range of a validity bitmap in place:

```ruby
# Replace elements 40..79 of the bitmap with a new validity mask
bitmap.bit_splice(40, 40, new_mask)
```

**Use case for `lsb_first: false`:** writing into an MSB-first packed wire-format buffer at a position the spec writes in MSB-first coordinates (PNG scanline pixel region, RFC header sub-field).

---

## Slice

### `bit_slice(bit_offset, bit_length, lsb_first: true) -> String | nil`
### `bit_slice(bit_range, lsb_first: true) -> String | nil`

The bit-granularity analog of `String#byteslice`. Extracts `bit_length` bits starting at flat bit position `bit_offset`.

The range form is equivalent to the integer form, with the offset and length derived from the given bit range. A negative `bit_offset`, `bit_length`, or Range endpoint raises `IndexError`; negative positions are never interpreted as counting backward from the end (see [Discussion.md#error-behavior-for-out-of-range-bit-indices](./Discussion.md#error-behavior-for-out-of-range-bit-indices)).

The result length is `ceil(bit_length / 8)` bytes. If `bit_length` is not a multiple of 8, the unused high bits of the last byte are cleared to zero.

Returns `nil` if `bit_offset` or `bit_length` is not an `Integer`, or if `bit_offset` is beyond the end of the string. A negative `bit_offset` or `bit_length` raises `IndexError` rather than returning `nil`.

```ruby
data = "\xFF\xAA".b    # byte[0]=0xFF, byte[1]=0xAA (0b10101010)

data.bit_slice(0, 8)   #=> "\xFF"
data.bit_slice(8, 8)   #=> "\xAA"
data.bit_slice(4, 8)   #=> "\xAF"   # bits 4-11 packed LSB-first

data.bit_slice(0..7)   #=> "\xFF"
data.bit_slice(0...8)  #=> "\xFF"
data.bit_slice(-8..-1) #=> IndexError   # negative endpoints are not allowed
```

Regardless of `lsb_first:`, the result String is always packed LSB-first, so `bit_at` and all other methods work on it under their default convention:

```ruby
result = data.bit_slice(4, 8)
result.bit_at(0)                #=> same as data.bit_at(4)
result.each_bit_offset(true)    # yields set-bit positions within the extracted range
```

Apache Arrow idiom --- normalize a non-byte-aligned validity bitmap for IPC serialization:

```ruby
# Arrow in-memory slice has bit offset 5; IPC requires offset 0
ipc_validity = validity_bitmap.bit_slice(slice_offset, slice_length)
```

**Use case for `lsb_first: false`:** extracting a sub-range of an MSB-first packed buffer (PNG 1/2/4-bit scanline, RFC header field) using the same coordinate the spec writes. The result preserves the numeric significance of the field:

```ruby
"\xAC".bit_slice(0, 4, lsb_first: false)  #=> "\x0A"
# the leading 4 bits of "\xAC" (1010) are preserved as 0x0A (1010)
```

---

## Bitwise

Each operation comes in a non-destructive form (returns a new `String`) and a destructive in-place form (`!`, returns `self`). Both forms raise `ArgumentError` if the two strings differ in `bytesize`.

```
non-destructive:  result = a.bitwise_and(b)   -- allocates a new String
destructive:      a.bitwise_and!(b)           -- modifies a in place, no allocation
```

---

### `bitwise_not -> String` / `bitwise_not! -> self`

Inverts every bit.

```ruby
"\xAA".bitwise_not   #=> "\x55"   # 0b10101010 -> 0b01010101
"\x00".bitwise_not   #=> "\xFF"
```

---

### `bitwise_and(other) -> String` / `bitwise_and!(other) -> self`

Bitwise AND. A bit in the result is 1 only if both operands have 1 at that position.

```
  0b11110000   (left)
& 0b11001100   (right)
-----------
  0b11000000   (result: only bits set in both)
```

```ruby
"\xF0".bitwise_and("\xCC")  #=> "\xC0"
```

Apache Arrow idiom --- null propagation (result valid only where both inputs are valid):

```ruby
result_validity = left_validity.bitwise_and(right_validity)
```

Apache Arrow idiom --- apply a boolean filter to a column:

```ruby
result_validity = source_validity.bitwise_and(filter_bitmap)
```

---

### `bitwise_or(other) -> String` / `bitwise_or!(other) -> self`

Bitwise OR. A bit in the result is 1 if either operand has 1 at that position.

```
  0b11110000   (left)
| 0b00001111   (right)
-----------
  0b11111111   (result: union)
```

```ruby
"\xF0".bitwise_or("\x0F")  #=> "\xFF"
```

Apache Arrow idiom --- union of two validity bitmaps:

```ruby
either_valid = left_validity.bitwise_or(right_validity)
```

---

### `bitwise_xor(other) -> String` / `bitwise_xor!(other) -> self`

Bitwise XOR. A bit in the result is 1 if the operands differ at that position.

```
  0b11111111   (left)
^ 0b10101010   (right)
-----------
  0b01010101   (result: difference)
```

```ruby
"\xFF".bitwise_xor("\xAA")  #=> "\x55"
"\xAA".bitwise_xor("\xAA")  #=> "\x00"   # XOR with self is always zero
"\xAA".bitwise_xor("\xFF")  #=> "\x55"   # XOR with all-ones is bitwise_not
```

---

## Why no `bit_size`?

This proposal does not add `bit_size`. The physical bit count is always `bytesize * 8` --- short enough that a dedicated method adds little. The semantically meaningful count, when a `String` is used as a bitmap, is format-dependent (Arrow tracks element count via schema metadata; MSB-first packed buffers may have padding in the last byte) and belongs alongside the format's other metadata, not on `String`. A method called `bit_size` would risk being read as either, so the proposal deliberately leaves the name unused.
