# Discussion

## Why extend `String` rather than introduce a new class?

The obvious alternative is a dedicated `BitSet` class (analogous to Java's `java.util.BitSet` or C++'s `std::bitset`). Two arguments favour extending `String` instead.

**Adding a new top-level constant is a high bar for Ruby core** --- Introducing `BitSet` would permanently reserve a widely plausible name and force conversion at boundaries where Ruby code already uses `String`.

**`String` is already Ruby's binary buffer type** --- Socket reads, file reads, `pack`/`unpack`, and similar APIs already hand raw bytes to Ruby as `String`. Since `String` already exposes byte-level operations such as `bytesize`, `getbyte`, `setbyte`, `byteslice`, and `bytesplice`, bit-level methods are one depth extension of an existing role rather than a new category.

This proposal is intentionally limited to bit-level operations on an already materialized `String`; concerns such as zero-copy slicing or mapped I/O belong to abstractions like `IO::Buffer`, not to this proposal.

## Naming convention: symmetry with `bytes` / `each_byte`

The method pairs `each_bit`/`bits`, `each_bit_offset`/`bit_offsets`, and `each_bit_run`/`bit_runs` follow the same basic Ruby idiom as `each_byte` / `bytes`: iterator form plus collected form.

## Bit Position Numbering of the String bit API

This section explains how the String bit API addresses individual bits. A single keyword, `lsb_first:`, decides intra-byte bit numbering wherever bit positions are exchanged with the caller, and intra-byte scan direction wherever the API walks the sequence.

### 1. Physical layout of bits

A `String` is a sequence of bytes; each byte holds 8 bits. For example, `"\xFF\xAA"` is laid out as:

```text
  byte[0] = 0xFF (0b11111111)                   byte[1] = 0xAA (0b10101010)
  +----+----+----+----+----+----+----+----+     +----+----+----+----+----+----+----+----+
  |  1 |  1 |  1 |  1 |  1 |  1 |  1 |  1 |     |  1 |  0 |  1 |  0 |  1 |  0 |  1 |  0 |
  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |     | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
  +----+----+----+----+----+----+----+----+     +----+----+----+----+----+----+----+----+
```

### 2. Intra-byte numbering --- `lsb_first:`

Important common rule: Across-byte order is always `byte[0]` to `byte[n-1]`; `lsb_first:` does not change that.

#### `lsb_first: true` (default)

Within each byte, `n = 0` is the LSB. Numbering proceeds upward through byte[0] and then continues at the LSB of byte[1]:

```text
n :   7  6  5  4  3  2  1  0     15 14 13 12 11 10 9  8
bit:  b7 b6 b5 b4 b3 b2 b1 b0    b7 b6 b5 b4 b3 b2 b1 b0
      byte[0]              ^     byte[1]              ^
                           LSB                        LSB
```

Formula: `n = byte_index * 8 + bit_in_byte`.

#### `lsb_first: false`

Byte order is preserved (`byte[0]` is still first), but within each byte numbering starts at the MSB:

```text
n :   0  1  2  3  4  5  6  7     8  9  10 11 12 13 14 15
bit:  b7 b6 b5 b4 b3 b2 b1 b0    b7 b6 b5 b4 b3 b2 b1 b0
      ^                          ^
      MSB                        MSB
```

Formula: `n = byte_index * 8 + (7 - bit_in_byte)`.

This convention is used by RFC network header diagrams (IPv4, TCP, DNS), PNG 1/2/4-bit scanlines, and the BitTorrent bitfield message.
In all of those, "bit 0" is the leftmost (most significant) bit of the first byte.

#### Same `n`, different bit

The two conventions disagree on what each integer refers to:

```ruby
data = "\xAA"   # byte[0] = 0b10101010

data.bit_at(0)                    #=> false (byte[0] bit 0 is unset)
data.bit_at(0, lsb_first: false)  #=> true  (byte[0] bit 7 is set)
```

### 3. How `lsb_first:` applies across the API

`lsb_first:` is a single switch, but different methods consume it slightly differently:

| group                                                              | role of `lsb_first:`                                  |
|--------------------------------------------------------------------|-------------------------------------------------------|
| `bit_at`, `bit_set`, `bit_clear`, `bit_flip`                       | interpretation of the integer position (or range)     |
| `each_bit_offset`, `bit_offsets`                                   | numbering used for yielded positions                  |
| `each_bit`, `bits`, `each_bit_run`, `bit_runs`                     | intra-byte scan direction during traversal            |
| `bit_slice`, `bit_splice`, `bit_run_count`                         | interpretation of the input position (see Section 4)  |
| `bit_count`, `bitwise_not(!)`, `bitwise_and(!)`, `bitwise_or(!)`, `bitwise_xor(!)` | none (order-independent operations)                   |

Across-byte order is always `byte[0]` to `byte[n-1]` regardless of `lsb_first:`. The two conventions only differ in how each individual byte is walked or numbered internally.

For methods that yield integer positions (`each_bit_offset`, `bit_offsets`), the yielded values can be fed back into any position-taking method under the same `lsb_first:`:

```ruby
data.each_bit_offset(true, lsb_first: bool).all? do |n|
  data.bit_at(n, lsb_first: bool)
end
#=> true, for any bool
```

### 4. Result Strings preserve the physical bit sequence

`bit_slice`, `bit_splice`, and `bit_run_count` exchange bit positions with the caller based on the chosen `lsb_first:` coordinate system. However, once the physical range is identified, the resulting `String` always preserves the physical bit-sequence of that range.

Specifically, for a slice starting at physical bit `offset`, the result is packed such that:
`result.bit_at(i, lsb_first: true)` == `source.bit_at(offset + i, lsb_first: true)`

This means that slicing the same physical memory range always yields the same result String, regardless of which `lsb_first:` convention was used to specify the range.

```ruby
data = "\xAC".b  # 0b10101100

# Both of these refer to the same physical bits (4, 5, 6, 7)
s1 = data.bit_slice(0, 4, lsb_first: false) # MSB-first: first 4 bits
s2 = data.bit_slice(4, 4, lsb_first: true)  # LSB-first: high 4 bits

s1 == s2   #=> true
s1         #=> "\x0A" (0b00001010)
```

#### Consistent Numeric Significance

Because this rule preserves the relative weights of bits (which bit is "more significant" than another in memory), extracted fields can be read as integers (via `String#ord`, `String#unpack`, etc.) without bit-reversal.

```
The first 8 bit of IPv4 header:
  01000101...
  ^^^^     Version
      ^^^^ IHL (Header length)
```

A 4-bit version field at the start of an IPv4 header (`0x45` = `01000101`) remains `4` when sliced, and an IHL field remains `5`:

```ruby
ipv4_header = "\x45".b
version = ipv4_header.bit_slice(0, 4, lsb_first: false)
version.ord #=> 4
ihl = ipv4_header.bit_slice(4, 4, lsb_first: false)
ihl.ord #=> 5
```

### 5. Summary

`lsb_first:` is the single switch this API exposes for bit ordering:

```text
lsb_first: true   intra-byte numbering / scan starts at the LSB (default)
lsb_first: false  intra-byte numbering / scan starts at the MSB
```

Across-byte order, and the packing of result Strings, are fixed. The keyword only changes what an integer position means and which direction a byte is walked internally.

## Why `lsb_first: true` is the default?

Both numbering conventions are first-class, but `lsb_first: true` is the default for the following reasons.

**Composability with `String#getbyte` and `Integer#[]`.** `Integer#[]` already uses LSB-first numbering (`4[0] == 0`, `4[2] == 1`). `lsb_first: true` extends this convention to `String` so that bit-level access composes directly with byte-level access:

```ruby
data.bit_at(n)   # equivalent to: data.getbyte(n / 8)[n % 8] == 1
```

The `Integer#[]` analogy is applied per byte --- `getbyte(n / 8)` yields a single byte, and `[n % 8]` indexes within that byte --- so the correspondence is naturally intra-byte. A caller mixing byte-level access (`getbyte`/`setbyte`, `pack`/`unpack`) with bit-level access never has to relearn which end is bit 0 or insert a `7 - k` flip between the two styles.

**Most in-memory bitmap formats use LSB-first** --- Apache Arrow validity bitmaps, ext4 block bitmaps, Roaring bitmap containers, the Linux kernel bitmap API, BSD `bitstring(3)`, and most hardware peripheral register documentation (ARM Cortex-M, STM32, x86 control registers) all number bits from the LSB. This is the convention any caller building or reading an in-memory bitmap is likely to expect.

## Bit ordering across domains

| domain                                          | native bit ordering                                |
|-------------------------------------------------|----------------------------------------------------|
| Apache Arrow validity / boolean bitmap          | LSB-first (element i = byte[i/8] bit i%8)          |
| ext4 block bitmap                               | LSB-first                                          |
| Roaring bitmap containers                       | LSB-first                                          |
| Linux kernel bitmap API (`bitmap.h`, `bit_set`) | LSB-first                                          |
| BSD `bitstring(3)`                              | LSB-first                                          |
| Hardware peripheral registers (ARM, STM32, x86) | LSB-first                                          |
| RFC-style network headers (IPv4, TCP, DNS)      | bit 0 = MSB of first byte (RFC diagram convention) |
| PNG 1/2/4-bit scanlines                         | MSB = leftmost pixel                               |
| BitTorrent bitfield message                     | MSB-first (piece i at MSB-first position i)        |
| JPEG / DEFLATE Huffman bit stream               | MSB-first                                          |

The table is drawn from in-memory bit-addressing conventions where a byte buffer is indexed at bit granularity; pure display or wire-only formats that are never held as an indexable bitmap are excluded. Both `lsb_first: true` and `lsb_first: false` address conventions that exist in real systems, though LSB-first is the more common choice in in-memory bitmap formats.

## Apache Arrow Compatibility

Apache Arrow validity bitmaps use the same flat LSB-first layout: element `i` is stored in `byte[i / 8]` at bit `i % 8`.

`bit_at(i)` maps directly to Arrow element index `i`. `each_bit_offset(true, lsb_first: true)` yields valid element indices in ascending order; `each_bit_offset(false, lsb_first: true)` yields null element indices.

### Arrow IPC serialization

Arrow supports zero-copy slicing in memory, so a sliced validity bitmap may start at a non-zero bit offset. The IPC format, however, requires the serialized bitmap to start at bit 0 of the first byte. `bit_slice` fills that gap:

```ruby
# column slice: validity bitmap starts at in-memory bit offset 5, covering 100 elements
# IPC requires the bitmap to start at bit 0
ipc_validity = validity_bitmap.bit_slice(5, 100)
```

## Error behavior for out-of-range bit indices

Three distinct categories of bad input are handled separately.

**Negative index** --- all methods raise `IndexError`. The API uses only non-negative bit positions; negative integers are not interpreted as "count from end" the way `String#[]` or `String#getbyte` do. Rejecting them explicitly is clearer than silently treating them as out-of-range positives. This applies equally to scalar indices and to Range endpoints: a negative Range endpoint raises `IndexError` just as `bit_at(-1)` does, because both represent the same invalid input --- a negative bit position. In particular, allowing negative Range endpoints would combine count-from-end index normalization with the `lsb_first:` coordinate transformation, creating a confusing interaction where the same negative index resolves to a different physical bit depending on the `lsb_first:` flag --- a likely source of subtle bugs.

**Non-negative index beyond the string's bit length** --- read methods return `nil`; mutation methods raise `IndexError`. The asymmetry is intentional: a missed read is a logic question ("is this bit set?"), while a missed write risks silent data corruption. This mirrors `String#setbyte` (raises `IndexError` for out-of-bounds writes) on the mutation side.

**Index outside the implementation's supported integer range** --- all methods raise `ArgumentError`. The goal is deterministic behavior for clearly invalid input, rather than leaking platform-dependent conversion details into the public API. Implementations are expected to hold bit indices in a fixed-width signed integer wide enough to address any in-memory bitmap (a pointer-width signed integer is the natural choice); positions that do not fit are rejected at the API boundary rather than silently truncated.

```ruby
s = "\xFF"
s.bit_at(-1)             #=> IndexError
s.bit_at(100)            #=> nil
s.bit_at(2**100)         #=> ArgumentError
s.bit_run_count(100, 0)  #=> nil
s.bit_set(-1)            #=> IndexError
s.bit_set(100)           #=> IndexError
s.bit_set(2**100)        #=> ArgumentError
s.bit_slice(-8..-1)      #=> IndexError (negative Range endpoint)
s.bit_set(..-1)          #=> IndexError (negative Range endpoint)
```

