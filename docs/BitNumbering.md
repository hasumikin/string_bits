# Bit Numbering

This note explains how the String bit API addresses individual bits. A single keyword, `lsb_first:`, decides intra-byte bit numbering wherever bit positions are exchanged with the caller, and intra-byte scan direction wherever the API walks the sequence.

## 1. Physical layout of bits

A `String` is a sequence of bytes; each byte holds 8 bits. For example, `"\xFF\xAA"` is laid out as:

```text
  byte[0] = 0xFF (0b11111111)                   byte[1] = 0xAA (0b10101010)
  +----+----+----+----+----+----+----+----+     +----+----+----+----+----+----+----+----+
  |  1 |  1 |  1 |  1 |  1 |  1 |  1 |  1 |     |  1 |  0 |  1 |  0 |  1 |  0 |  1 |  0 |
  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |     | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
  +----+----+----+----+----+----+----+----+     +----+----+----+----+----+----+----+----+
```

## 2. Intra-byte numbering --- `lsb_first:`

Important common rule: Across-byte order is always `byte[0]` to `byte[n-1]`; `lsb_first:` does not change that.

### `lsb_first: true` (default)

Within each byte, `n = 0` is the LSB. Numbering proceeds upward through byte[0] and then continues at the LSB of byte[1]:

```text
n :   7  6  5  4  3  2  1  0     15 14 13 12 11 10 9  8
bit:  b7 b6 b5 b4 b3 b2 b1 b0    b7 b6 b5 b4 b3 b2 b1 b0
      byte[0]              ^     byte[1]              ^
                           LSB                        LSB
```

Formula: `n = byte_index * 8 + bit_in_byte`.

This convention is used by Apache Arrow validity bitmaps, ext4 block bitmaps, and most hardware register documentation.
It is the default because it matches plain byte arithmetic.

### `lsb_first: false`

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

### Same `n`, different bit

The two conventions disagree on what each integer refers to:

```ruby
data = "\xAA"   # byte[0] = 0b10101010

data.bit_at(0)                    #=> false (byte[0] bit 0 is unset)
data.bit_at(0, lsb_first: false)  #=> true  (byte[0] bit 7 is set)
```

## 3. How `lsb_first:` applies across the API

`lsb_first:` is a single switch, but different methods consume it slightly differently:

| group                                                              | role of `lsb_first:`                                  |
|--------------------------------------------------------------------|-------------------------------------------------------|
| `bit_at`, `set_bit`, `clear_bit`, `flip_bit`                       | interpretation of the integer position (or range)     |
| `each_set_bit_offset`, `set_bit_offsets`                           | numbering used for yielded positions                  |
| `each_bit`, `bits`, `each_bit_run`, `bit_runs`                     | intra-byte scan direction during traversal            |
| `bit_slice`, `bit_splice`, `bit_run_count`                         | interpretation of the input position (see Section 4)  |
| `bit_count`, `bit_not(!)`, `bit_and(!)`, `bit_or(!)`, `bit_xor(!)` | none (order-independent operations)                   |

Across-byte order is always `byte[0]` to `byte[n-1]` regardless of `lsb_first:`. The two conventions only differ in how each individual byte is walked or numbered internally.

For methods that yield integer positions (`each_set_bit_offset`, `set_bit_offsets`), the yielded values can be fed back into any position-taking method under the same `lsb_first:`:

```ruby
data.each_set_bit_offset(lsb_first: bool).all? do |n|
  data.bit_at(n, lsb_first: bool)
end
#=> true, for any bool
```

## 4. Result Strings use canonical integer bit packing

`bit_slice`, `bit_splice`, and `bit_run_count` exchange flat bit positions with the caller. `lsb_first: false` affects only how input positions are interpreted. The resulting packed bit sequence always follows Ruby's native integer semantics, where bit *n* corresponds to `Integer#[n]`.

```ruby
"\xAC".bit_slice(0, 4, lsb_first: false)
#=> "\x05"
# Input position 0..3 selects the four MSB-side bits of "\xAC" (= 1010).
# The result is the canonical integer bit packing, so the returned String
# can be read by default-convention methods.
```

This rule keeps every result String in one canonical layout: `bit_at`, `bit_count`, `bit_and`, and so on all behave the same way on slices regardless of which convention produced them.

### Whole-byte slice under `lsb_first: false` reverses the byte

A subtle but important consequence: when the slice covers an entire byte, the result is the original byte **bit-reversed**.

```ruby
"\xAC".bit_slice(0, 8, lsb_first: false)
#=> "\x35"
# 0xAC = 0b10101100
#   collected MSB-first        : [1, 0, 1, 0, 1, 1, 0, 0]
#   canonical integer packing  : bit 0 = first collected, bit 7 = last collected
#                              = 0b00110101 = 0x35
```

This is the natural composition of two rules:

- *"Position 0 under `lsb_first: false` is the MSB"* --- so the scan starts at the original byte's MSB
- *"Result Strings use canonical integer bit packing"* --- so the first collected bit becomes the result's bit 0

Compose them and a byte read with MSB at position 0 lands with its MSB at the result's LSB --- a full bit reverse. The four-bit example above is the same operation restricted to half a byte (`0xA` -> `0x5`).

Note that this is NOT asymmetry; it is the price of having a single canonical packing for every result `String`. The reverse happens transparently again on the way back through `bit_splice(..., lsb_first: false)` (which reads its source as canonical integer packing), which is why the MSB-first round-trip in the example below still recovers the original buffer.

A direct corollary: when the slice length is a multiple of 8, applying `bit_slice(..., lsb_first: false)` twice returns the original buffer --- bit reversal is its own inverse.

```ruby
"\xAA\xAA".bit_slice(0, 16, lsb_first: false)
#=> "\x55\x55"

"\xAA\xAA".bit_slice(0, 16, lsb_first: false).bit_slice(0, 16, lsb_first: false)
#=> "\xAA\xAA"
```

`bit_splice` mirrors this asymmetry. It interprets the destination position under the chosen `lsb_first:`, but always reads its source String using canonical integer packing. A `bit_slice(..., lsb_first: false)` result can therefore be written back through `bit_splice(..., lsb_first: false)` and round-trips exactly:

```ruby
data  = "\x96\x3C\xA5"
slice = data.bit_slice(3, 11, lsb_first: false)   # canonical integer packing
buf   = +"\x00" * 3
buf.bit_splice(3, 11, slice, lsb_first: false)    # written back to MSB-first position 3
buf.bit_slice(3, 11, lsb_first: false) == slice   #=> true
```


## 5. Summary

`lsb_first:` is the single switch this API exposes for bit ordering:

```text
lsb_first: true   intra-byte numbering / scan starts at the LSB (default)
lsb_first: false  intra-byte numbering / scan starts at the MSB
```

Across-byte order, and the packing of result Strings, are fixed. The keyword only changes what an integer position means and which direction a byte is walked internally.
