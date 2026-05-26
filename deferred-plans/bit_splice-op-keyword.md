# Future Proposal Plan: `bit_splice` `op:` keyword (bit-blit)

This file documents an `op:` keyword for `bit_splice` that combines the source bits into the destination with a bitwise operation (AND / OR / XOR) instead of plain copy. It is intentionally excluded from the main proposal in docs/Main.md, where `bit_splice` is a plain copy.

## Combining Splice (bit-blit)

`bit_splice` currently overwrites the destination bits with the source bits. The `op:` keyword generalizes that write step into a read-modify-write that combines each source bit with the existing destination bit. This is the bit-granular form of a raster bit-block transfer (BitBlt).

### Reason for deferral

The motivating domain is specialized: monochrome (1 bit per pixel) raster graphics --- compositing sprites onto a framebuffer on embedded displays (SSD1306 OLED, e-paper), where OR draws, AND masks, and XOR toggles, and the sprite usually lands at a pixel offset that is not a multiple of 8. This fits the embedded / device-driver direction but is narrower than the rest of the proposal. A keyword that switches the core combining operation also deserves its own discussion on core-ruby-dev. The change is purely additive --- the default `op: :copy` is identical to today's behavior --- so it can be adopted later without breaking anything.

### Why a keyword on `bit_splice`, and not elsewhere

- **Not new `bit_blit_*` methods.** A single method with keyword control is easier to write and read than a family of near-identical methods. The default keeps the common case (`op: :copy`) unchanged.
- **Not offsets on `bitwise_*`.** The `bitwise_not/and/or/xor` methods are whole-string, equal-length operations. A blit needs *two* offsets plus a length (a source position and a destination position), which is exactly the shape `bit_splice` already has. Adding offsets to `bitwise_*` cannot express that.
- **Uniform region ops are already covered.** Setting a contiguous region to 1 is `bit_set(bit_offset, bit_length)` (OR with all-ones); clearing is `bit_clear` (AND with all-zeros); toggling is `bit_flip` (XOR with all-ones). The only region operation not yet expressible is a read-modify-write against an *arbitrary, non-uniform* mask --- which is precisely what `op:` adds.

## `bit_splice(bit_offset, bit_length, str, str_bit_offset=0, lsb_first: true, op: :copy) -> self`
## `bit_splice(bit_range, str, str_bit_offset=0, lsb_first: true, op: :copy) -> self`

All existing arguments and forms are unchanged; `op:` is a new trailing keyword.

### `op:` values

For each `k` in `0...bit_length`, let `s` be the source bit (`str` at `str_bit_offset + k`) and `d` be the destination bit (`self` at `bit_offset + k`, interpreted under `lsb_first:`):

| `op:`             | result       | use                          |
|-------------------|--------------|------------------------------|
| `:copy` (default) | `d = s`      | plain overwrite              |
| `:and`            | `d = d & s`  | mask / erase                 |
| `:or`             | `d = d \| s` | draw (set pixels)            |
| `:xor`            | `d = d ^ s`  | toggle (cursor draw / erase) |

The symbol names mirror `bitwise_and` / `bitwise_or` / `bitwise_xor`. An unknown symbol raises `ArgumentError`.

### `lsb_first:` and invariants

`op:` does not change how positions are interpreted: `lsb_first:` still controls only the destination position, and the source's physical bit sequence is preserved, exactly as for `:copy`. The combining is bit-for-bit in the same order `:copy` would write. Two consequences worth stating:

- `op: :xor` is an involution: applying it twice with the same `str` and arguments restores `self`.
- `op: :or` and `op: :and` are idempotent: applying twice equals applying once.

### Examples

```ruby
buf = +"\x0F".b
buf.bit_splice(0, 8, "\xF0", op: :or)    # 0x0F | 0xF0 -> buf is "\xFF"  (draw)

buf = +"\xFF".b
buf.bit_splice(0, 8, "\x0F", op: :and)   # 0xFF & 0x0F -> buf is "\x0F"  (mask)

buf = +"\x00".b
buf.bit_splice(0, 8, "\xFF", op: :xor)   # buf is "\xFF"  (toggle on)
buf.bit_splice(0, 8, "\xFF", op: :xor)   # buf is "\x00"  (toggle off; XOR is its own inverse)

# non-byte-aligned blit: OR a sprite into bits 4..11
buf = +"\x00\x00".b
buf.bit_splice(4, 8, "\xFF", op: :or)    # buf is "\xF0\x0F"

# default is unchanged
buf = +"\x00\x00".b
buf.bit_splice(0, 8, "\xAA")             # op: :copy; buf is "\xAA\x00"
```

Monochrome framebuffer idiom --- XOR a cursor sprite onto a 1bpp display buffer at an arbitrary pixel column, then erase it by repeating the same call:

```ruby
framebuffer.bit_splice(x, sprite_bits, cursor, 0, op: :xor)   # draw
framebuffer.bit_splice(x, sprite_bits, cursor, 0, op: :xor)   # erase (restores pixels)
```

### Errors and edge cases

- Bounds checking is identical to plain `bit_splice`: `IndexError` if the destination or source range falls outside its string.
- `bit_length == 0` is a no-op and returns `self`.
- The self-aliasing guard (duplicating `str` when `str == self`) applies unchanged.
- An `op:` value other than `:copy`, `:and`, `:or`, or `:xor` raises `ArgumentError`.

### Relationship to existing methods

- Contiguous uniform fills stay with `bit_set` / `bit_clear` / `bit_flip` --- simpler and clearer than passing an all-ones or all-zeros source to `op:`.
- Whole-buffer, equal-length logic stays with `bitwise_and` / `bitwise_or` / `bitwise_xor` --- no offsets needed.
- `bit_splice` with `op:` is the tool for combining an arbitrary pattern into a bit-granular sub-region.
- Combining an Integer value into a field (the hardware-register read-modify-write case) belongs to `bit_field_splice`, which carries its own `op:` keyword --- see `bit_field_slice-and-bit_field_splice.md`.

### Implementation note

The existing C helper `bit_copy_core` already performs the shift/mask merge that writes source bits into a non-byte-aligned destination. Generalizing only its final write step --- from "replace the masked destination bits with the source bits" to `&=` / `|=` / `^=` against them --- yields the three combining ops while reusing the same alignment machinery. `:copy` keeps the current path.

### Open questions

- **Naming of values.** `:copy` / `:and` / `:or` / `:xor` mirror the `bitwise_*` family. An alternative spelling (e.g. `:set` for copy) was considered and rejected as less clear.
