# Future Proposal Plan

This file documents `bit_field_slice` and `bit_field_splice`, the Integer-view counterpart of `bit_slice` / `bit_splice`, intentionally excluded from the main proposal in docs/Main.md.

## Integer-View Slice and Splice

`bit_slice` / `bit_splice` are the raw-bit-sequence (`String`) view of a bit region: read bits out into a `String`, write a `String` back. `bit_field_slice` / `bit_field_splice` are the numeric-field (`Integer`) view of the same region, with identical `(offset, length)` addressing. The `_field` infix marks the change of lens --- "interpret these bits as a numeric field" --- while `slice` / `splice` keep the read / write roles parallel to the `String` pair.

```text
bit_slice(offset, length)               -> String    # raw bit-sequence view
bit_field_slice(offset, length)         -> Integer    # numeric-field view
bit_splice(offset, length, str)         -> self
bit_field_splice(offset, length, value) -> self
```

### Reason for deferral

The read side, `bit_field_slice`, is largely a scalar, zero-allocation variant of `each_bit_field` / `bit_fields`, which are themselves deferred (see `each_bit_field-and-bit_fields.md`). It returns a typed `Integer` decoded from a packed binary layout --- the same yield-type concern that defers `each_bit_field`, and which is expected to prolong discussion on the core-ruby-dev mailing list. The write side, `bit_field_splice`, is a genuine new capability (today, writing an integer field requires a `[value].pack(...)` plus `bit_splice` roundtrip), but it pairs with the deferred read side and reuses the same significance convention, so the two are deferred together to keep the core proposal minimal.

## `bit_field_slice(offset, length, lsb_first: true) -> Integer | nil`
## `bit_field_slice(bit_range, lsb_first: true) -> Integer | nil`

Reads `length` bits beginning at flat bit position `offset` and returns their value as a non-negative `Integer`. The range form derives `offset` and `length` from the range, exactly as `bit_slice` does. Negative indices count backward from the end, as in `bit_slice` and `[]`.

The numeric significance follows the convention of `each_bit_field` / `bit_fields`: under `lsb_first: true` the first (lowest) position contributes the least-significant bit, so the buffer reads little-endian; under `lsb_first: false` the first scanned bit becomes the most-significant bit of the result, matching RFC and MSB-first packed formats.

Returns `nil` if `offset` or `length` is not an `Integer`, if either is negative, or if the region lies outside the string --- the same boundary behavior as `bit_slice`.

```ruby
data = "\xF0\xAA".b   # byte[0]=0xF0, byte[1]=0xAA (0b10101010)

data.bit_field_slice(0, 8)   #=> 0xF0   (240; bits 0-7 = byte[0])
data.bit_field_slice(8, 8)   #=> 0xAA   (170; bits 8-15 = byte[1])
data.bit_field_slice(4, 4)   #=> 0x0F   (high nibble of 0xF0 is all ones)
data.bit_field_slice(0, 4)   #=> 0x00   (low nibble of 0xF0 in LSB-first is zero)
data.bit_field_slice(4, 8)   #=> 0xAF   (bits 4-11; mirrors bit_slice(4, 8) => "\xAF")

# range form
data.bit_field_slice(0..7)   #=> 0xF0

# lsb_first: false preserves the numeric significance of an MSB-first field
"\xAC".bit_field_slice(0, 4, lsb_first: false)  #=> 0x0A   (leading 4 bits 1010 = 10)
```

## `bit_field_splice(offset, length, value, lsb_first: true) -> self`
## `bit_field_splice(bit_range, value, lsb_first: true) -> self`

Writes the non-negative `Integer` `value` into the `length` bits beginning at flat bit position `offset`, then returns `self`. The inverse of `bit_field_slice`. Like `bit_splice`, it does not resize `self`: the destination is always exactly `length` bits.

`value` is interpreted under the same significance convention as `bit_field_slice`, so a value read by `bit_field_slice` can be written straight back by `bit_field_splice` with the same arguments. Raises `ArgumentError` if `value` does not fit in `length` bits, and `IndexError` if the destination region lies outside the string (the read-returns-`nil` / write-raises discipline shared with `bit_slice` / `bit_splice`).

```ruby
buf = +"\x00\x00".b
buf.bit_field_splice(0, 8, 0xF0)   # buf => "\xF0\x00"
buf.bit_field_splice(8, 8, 0xAA)   # buf => "\xF0\xAA"

# range form
buf.bit_field_splice(0..7, 0x00)   # buf => "\x00\xAA"

# value too wide for the field
buf.bit_field_splice(0, 4, 0x1F)   #=> ArgumentError  (0x1F needs 5 bits)
```

Roundtrip symmetry with `bit_field_slice`, under either convention:

```ruby
v = data.bit_field_slice(offset, length, lsb_first: order)
buf.bit_field_splice(offset, length, v, lsb_first: order)
buf.bit_field_slice(offset, length, lsb_first: order) == v   #=> true
```

### Use case --- hardware register / packet field read-modify-write

Reading and writing a sub-byte numeric field by the coordinate the spec writes, without a `pack` / `unpack` detour:

```ruby
# IPv4 header: the version field is the leading 4 bits (MSB-first)
version = header.bit_field_slice(0, 4, lsb_first: false)   #=> 4 for IPv4

# Write a 3-bit mode field into a device register in place
reg.bit_field_splice(5, 3, mode)
```

### Use case --- zero-allocation single-field decode

`bit_field_slice` is the scalar primitive that `bit_fields` can be built on: it focuses on one field and returns a plain `Integer`, allocating no `Array` or `Enumerator`. This is the form to reach for in an AOT-compiled inner loop, where `bit_fields(...).first` would allocate, exactly as `bit_index` relates to `each_bit_offset(...).first`.

### Open questions

- **Combining op (register read-modify-write).** Like the `op:` keyword proposed for `bit_splice` (see `bit_splice-op-keyword.md`), `bit_field_splice` could accept `op: :copy|:and|:or|:xor` to combine the Integer `value` with the field's existing bits instead of overwriting them. This is the idiomatic hardware-register update --- `op: :or` sets the masked bits, `op: :and` clears them, `op: :xor` toggles them --- and pairs with `bit_field_slice` for a read-modify-write cycle. It would be specified together with the String-source `op:`.
- **Signed fields.** Values are unsigned here. A `signed: true` keyword could decode / encode a two's-complement field of the given width. Deferred until a concrete use case appears.
- **Width cap.** `length` follows the same range as `each_bit_field` (1..64, with the mruby `MRB_INT_BIT - 1` portability note). Whether CRuby should cap at 63 for cross-implementation consistency, or allow a full-width Bignum result, is the same open question raised for `each_bit_field`.
- **Relationship to `each_bit_field` / `bit_fields`.** `bit_field_slice` overlaps with the single-field case of `bit_fields`. If both are eventually adopted, `bit_field_slice` is the natural primitive and `bit_fields` the bulk, record-tiling convenience built over it.
