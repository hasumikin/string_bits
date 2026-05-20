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

## 4. Result Strings preserve the physical bit sequence

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

### Consistent Numeric Significance

Because this rule preserves the relative weights of bits (which bit is "more significant" than another in memory), extracted fields naturally map to Ruby's `Integer#[n]` without bit-reversal.

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

## 5. Summary

`lsb_first:` is the single switch this API exposes for bit ordering:

```text
lsb_first: true   intra-byte numbering / scan starts at the LSB (default)
lsb_first: false  intra-byte numbering / scan starts at the MSB
```

Across-byte order, and the packing of result Strings, are fixed. The keyword only changes what an integer position means and which direction a byte is walked internally.
