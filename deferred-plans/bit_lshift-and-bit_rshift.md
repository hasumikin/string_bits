# Future Proposal Plan

This file documents `bit_lshift`, `bit_rshift`, and their in-place forms `bit_lshift!` / `bit_rshift!`, whole-bitmap bit shifts intentionally excluded from the main proposal in docs/Main.md.

## Whole-Bitmap Bit Shift

### Reason for deferral

A whole-bitmap shift is niche relative to the core read / mutate / bitwise set. Its two real motivations are aligning two bitmaps whose logical bit 0 sits at different physical offsets before a bitwise combine (Apache Arrow slice concatenation), and shift-register algorithms (LFSR, CRC, serial framing). The core proposal leaves it out to keep its scope small.

### Direction and naming

The left/right split follows the `ljust`/`rjust` and `lstrip`/`rstrip` precedent in `String`. Direction is defined by integer magnitude, which is the universal meaning of a shift in computing:

- `bit_lshift(count)` multiplies the value by `2**count` (shifts toward the most-significant end).
- `bit_rshift(count)` divides the value by `2**count` (shifts toward the least-significant end).

"Which end is most significant" reuses the significance convention already fixed by `each_bit_field` / `bit_fields`: under `lsb_first: true` the buffer reads little-endian (flat position 0 is the LSB), and under `lsb_first: false` it reads big-endian (flat position 0 is the MSB). The arithmetic meaning of left and right is therefore constant across modes; only the physical direction in which bits travel differs. Because the direction is encoded in the method name, `count` must be non-negative; a negative `count` raises `ArgumentError`.

In all forms the `bytesize` is preserved: vacated positions are filled with `0`, and bits shifted past either end are discarded. A `count` greater than or equal to `bytesize * 8` yields an all-zero result. The non-`!` forms return a new `String`; the `!` forms mutate `self` and return it, mirroring the `bitwise_*` / `bitwise_*!` pair.

## `bit_lshift(count, lsb_first: true) -> String`
## `bit_lshift!(count, lsb_first: true) -> self`

Shifts toward the most-significant end (multiply by `2**count`).

```ruby
"\x01\x00".b.bit_lshift(1)   #=> "\x02\x00"   # little-endian value 1 -> 2
"\x01\x00".b.bit_lshift(8)   #=> "\x00\x01"   # value 1 -> 256, byte-granular
"\xFF\xFF".b.bit_lshift(8)   #=> "\x00\xFF"   # top byte shifted out, low byte zero-filled
"\xFF\xFF".b.bit_lshift(16)  #=> "\x00\x00"   # everything shifted out

# lsb_first: false reads the buffer big-endian
"\x00\x01".b.bit_lshift(1, lsb_first: false)  #=> "\x00\x02"   # big-endian value 1 -> 2
"\x00\x01".b.bit_lshift(8, lsb_first: false)  #=> "\x01\x00"   # value 1 -> 256
```

## `bit_rshift(count, lsb_first: true) -> String`
## `bit_rshift!(count, lsb_first: true) -> self`

Shifts toward the least-significant end (divide by `2**count`).

```ruby
"\x02\x00".b.bit_rshift(1)   #=> "\x01\x00"   # little-endian value 2 -> 1
"\x00\x01".b.bit_rshift(1)   #=> "\x80\x00"   # value 256 -> 128, crosses byte boundary
"\x01\x00".b.bit_rshift(1)   #=> "\x00\x00"   # value 1 -> 0, the LSB drops off

# lsb_first: false reads the buffer big-endian
"\x00\x02".b.bit_rshift(1, lsb_first: false)  #=> "\x00\x01"   # big-endian value 2 -> 1
```

### Use case --- aligning bitmaps before a bitwise combine

A bitwise operation requires both operands to address the same logical bit at the same physical position. When two validity bitmaps come from slices taken at different bit offsets, one must be shifted so that its logical bit 0 lines up before the combine. In an LSB-first Arrow bitmap, `bit_lshift(k)` moves every logical bit from position `i` to position `i + k`:

```ruby
aligned = other.bit_lshift(offset_delta)
result  = base.bitwise_and(aligned)
```

Without a shift, this requires materializing a re-aligned copy bit by bit, which is exactly the work `bit_lshift` does in one pass.

### Use case for `lsb_first: false` --- shift register over an MSB-first stream

LFSR, CRC, and serial-framing logic shift a register one bit per step. When the data is an MSB-first wire stream, the register's "left" (toward the MSB-first leading edge) is `bit_lshift(1, lsb_first: false)`, and the new bit pushed out the top is the leading wire bit:

```ruby
frame.bit_lshift!(1, lsb_first: false)
```

### Open questions

- **Rotate variant.** A circular shift that wraps discarded bits back in (rather than zero-filling) would serve LFSR and some cryptographic routines. If demand appears it belongs in a separate `bit_lrotate` / `bit_rrotate` pair rather than a flag on the shift methods.
- **Fill bit.** Vacated positions are always `0` here. A `fill:` keyword (`0`/`1`) could be added if a concrete use case needs one-fill, but no motivating example is known yet.
