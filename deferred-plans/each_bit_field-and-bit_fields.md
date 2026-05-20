# Future Proposal Plan

This file documents `each_bit_field` and `bit_fields` as well as `Array#mask`, which are implemented and tested but intentionally excluded from the main proposal in README.md.

## Packed Bit-Field Iteration

### Reason for deferral

These methods yield plain `Integer` values, which is a different yield type from all other iteration methods in the proposal (which yield `true`/`false` or flat `Integer` positions).
Introducing a method that yields typed field values decoded from a packed binary layout is expected to prolong discussion on the core-ruby-dev mailing list. The rest of the proposal is kept clean to reduce that risk.

## `each_bit_field(*bitlens, lsb_first: true) { |*fields| } -> self`
## `each_bit_field(*bitlens, lsb_first: true) -> Enumerator`

Iterates over the string as a sequence of packed bit-field records. Each positional argument specifies the width (in bits) of one field in the record. On each iteration, one value per field is yielded as an `Integer` (LSB-first). At least one bitlen must be given; each must be between 1 and 64. Passing no bitlens or a zero/negative/out-of-range value raises `ArgumentError`. Without a block, returns an `Enumerator`.

**Integer width and portability.** The 64-bit limit reflects the `uint64_t` extraction used in this CRuby implementation. On mruby, `mrb_int` is either 32-bit (`MRB_INT32`, common on microcontrollers) or 64-bit (`MRB_INT64`). mruby does have a BigInt, but it is an optional gem rather than a core type. For core compatibility --- i.e., without assuming `MRB_USE_BIGINT=1` is defined --- a portable implementation should enforce `bitlen <= MRB_INT_BIT - 1`, yielding at most 31 or 63 bits depending on the build.
Whether CRuby should adopt the same `SIZEOF_LONG * 8 - 1` cap (63 on 64-bit systems) for cross-implementation consistency is an open question: it would sacrifice the ability to yield a full 64-bit unsigned field, which CRuby can always represent via Bignum.

**Record length and data length.** One record consumes exactly `sum(bitlens)` bits. The method iterates over as many complete records as the string contains, then stops. Trailing bits that do not fill a complete record are silently dropped, matching the behavior of `Enumerable#each_slice`.

Two boundary cases follow directly from this rule:

- **Data shorter than one record** (`bytesize * 8 < sum(bitlens)`): no complete records exist; the method yields nothing, exactly as `[].each_slice(n)` produces an empty result.
- **Data length not a multiple of `sum(bitlens)`**: all complete records are yielded; the leftover bits at the end are discarded without error.

```ruby
data = "\xAA\xCC"   # 16 bits

data.each_bit_field(8).to_a
#=> [0xAA, 0xCC]           # two single-field records

data.each_bit_field(8, 8).to_a
#=> [[0xAA, 0xCC]]         # one record with two fields

# data shorter than one record: yields nothing
"\x00".each_bit_field(5, 6, 5).to_a     # 8 bits < 16-bit record
#=> []

# data not a multiple of sum(bitlens): last partial record dropped
"\x00\x00\x00".each_bit_field(5, 6, 5).to_a  # 24 bits, record = 16 bits
#=> [[0, 0, 0]]             # one complete record; trailing 8 bits discarded
```

This is consistent with the iterator pattern across the entire API: `each_bit`, `each_bit_run`, and `each_set_bit_offset` also yield nothing when there is no data to yield, without raising. Callers that require exact alignment can guard beforehand:

```ruby
step = bitlens.sum
raise ArgumentError, "data length is not a multiple of record width" unless data.bytesize * 8 % step == 0
data.each_bit_field(*bitlens) { ... }
```

**RGB565 pixel manipulation**

Bit offsets and iteration indices are derived outside the block. `with_index` wraps the enumerator form; the bit offset of each record is `iter * step` where `step` is the sum of bitlens (16 for 5+6+5).

```ruby
# eg1: Swap R and B channels in an RGB565 buffer
# RGB565 LSB-first layout: bits 0-4 = blue (5), bits 5-10 = green (6), bits 11-15 = red (5)
rgb565data.each_bit_field(5, 6, 5).with_index do |(b, _g, r), iter|
  offset = iter * 16
  rgb565data.bit_splice(offset,      5, r)  # write red into the blue field
  rgb565data.bit_splice(offset + 11, 5, b)  # write blue into the red field
end

# eg2: Convert RGB565 to 4-bit grayscale
gray4data = "\x00" * (rgb565data.bytesize / 4)
rgb565data.each_bit_field(5, 6, 5).with_index do |(b, g, r), index|
  gray8 = ((r * 255 / 31) + (g * 255 / 63) + (b * 255 / 31)) / 3
  gray4data.bit_splice(index * 4, 4, gray8 >> 4)
end
```

The half-block rendering pattern used in bitmap fonts and braille displays also benefits: two scan-lines are delivered simultaneously so the vertical combination can be computed without maintaining external state between calls:

```ruby
bitlen = 12

data.each_bit_field(bitlen, bitlen) do |plane0, plane1|
  line = ""
  i = 0
  while i < bitlen
    case [(plane0 >> i) & 1, (plane1 >> i) & 1]
    in [1, 1] then line << "\xE2\x96\x88"  # Full Block U+2588
    in [1, 0] then line << "\xE2\x96\x80"  # Upper Half Block U+2580
    in [0, 1] then line << "\xE2\x96\x84"  # Lower Half Block U+2584
    else           line << " "
    end
    i += 1
  end
  puts line
end
```

Each extracted field is a plain `Integer`, so arithmetic on channel values and direct use with `bit_splice` require no intermediate conversion or packing step.

**Single-record decode.** When the string contains exactly one record (`bytesize * 8 == sum(bitlens)`), the block runs exactly once and each parameter binds directly to a field value. This is the natural form for decoding a non-repeating packed layout:

```ruby
# Block runs once; parameters bind directly to the eight fields.
operand_bytes.each_bit_field(1, 5, 5, 1, 5, 5, 1, 1, lsb_first: false) do |noblock, req, opt, rest, post, key_count, kdict, block|
  # ...
end
```

For this pattern `each_bit_field` is more appropriate than `bit_fields`, which returns `Array[Array[Integer]]` and would require `.first` to unwrap the single record.

---

### `bit_fields(*bitlens, lsb_first: true) -> Array`
### `bit_fields(*bitlens, lsb_first: true) { |*fields| } -> self`

Convenience complement of `each_bit_field`. Without a block, equivalent to `each_bit_field(*bitlens, lsb_first: lsb_first).to_a`: collects all complete records and returns them as an `Array`. Useful when all records must be available at once --- for cross-record processing, passing to another method, or random access by record index. With a block, yields the same values as `each_bit_field` and returns `self`. The same constraints on bitlens apply: at least one must be given, each must be between 1 and 64, and invalid values raise `ArgumentError`.

For decoding a single non-repeating record, prefer `each_bit_field` with a block: its parameters bind directly to field values without the nested-array wrapping that `bit_fields` produces.

The returned array mirrors `each_bit_field(*bitlens).to_a`: when exactly one bitlen is given the array is flat (`Array[Integer]`); when multiple bitlens are given each element is itself an `Array` (`Array[Array[Integer]]`).

```ruby
data = "\xAA\xCC"

data.bit_fields(8)
#=> [0xAA, 0xCC]          # single field: flat array

data.bit_fields(8, 8)
#=> [[0xAA, 0xCC]]        # multiple fields: array of arrays

pixel = [(21) | (42 << 5) | (10 << 11)].pack("S<")
pixel.bit_fields(5, 6, 5)
#=> [[21, 42, 10]]
```

**Data length vs. record width.** `bit_fields` applies the same rule as `each_bit_field`: complete records are collected; trailing bits that do not fill a record are discarded without error. When the data is shorter than one record, the returned array is empty.

```ruby
# data shorter than one record
"\x00".bit_fields(5, 6, 5)       # 8 bits < 16-bit record
#=> []

# data not a multiple of sum(bitlens)
"\x00\x00\x00".bit_fields(5, 6, 5)  # 24 bits, record = 16 bits
#=> [[0, 0, 0]]   # one complete record; trailing 8 bits discarded

# two complete 16-bit records
"\x00\x00\x00\x00".bit_fields(5, 6, 5)
#=> [[0, 0, 0], [0, 0, 0]]
```

Unlike `each_bit_field`, `bit_fields` returns all records at once, so the caller can compute offsets or indices from the returned array directly.

With `lsb_first: false`, each field collects bits in intra-byte MSB-first order. To preserve the numeric significance of MSB-first fields, the first bit scanned is mapped to the MSB of the result Integer, matching the convention of RFC headers and MSB-first packed formats:

```ruby
"\x96\x3C".bit_fields(8, lsb_first: false)
#=> [0x96, 0x3C]
# 0x96 = 0b10010110; first bit (1) becomes result bit 7 (MSB).
```

---

### Use Case: IoT Sensor Telemetry (Non-Byte-Aligned Packed Frames)

Compact binary telemetry protocols pack multiple sensor readings into sub-byte-aligned fields to minimize transmission overhead. A typical environmental sensor might encode three measurements into a 36-bit frame:

```
bits  0-11 : temperature  (12 bits, 0.1 deg resolution, 0-409.5)
bits 12-21 : humidity     (10 bits, 0.1% resolution, 0-102.3)
bits 22-35 : CO2          (14 bits, ppm, 0-16383)
```

36 bits = 4.5 bytes, so frames are **not byte-aligned**: even frames start at a byte boundary, odd frames start 4 bits into the preceding byte. Extracting fields with pure Ruby requires maintaining two separate code paths:

```ruby
# Pure Ruby: two hard-coded extraction paths for the two alignment cases.
N_FRAMES.times do |i|
  b = i * 36 >> 3
  if i.even?
    temp =  DATA.getbyte(b)         | ((DATA.getbyte(b+1) & 0x0F) << 8)
    hum  = (DATA.getbyte(b+1) >> 4) | ((DATA.getbyte(b+2) & 0x3F) << 4)
    co2  = (DATA.getbyte(b+2) >> 6) |  (DATA.getbyte(b+3) << 2) | ((DATA.getbyte(b+4) & 0x0F) << 10)
  else
    temp = (DATA.getbyte(b) >> 4)   |  (DATA.getbyte(b+1) << 4)
    hum  =  DATA.getbyte(b+2)       | ((DATA.getbyte(b+3) & 0x03) << 8)
    co2  = (DATA.getbyte(b+3) >> 2) |  (DATA.getbyte(b+4) << 6)
  end
  process(temp, hum, co2)
end
```

`each_bit_field` handles both alignment cases uniformly --- the bit offset arithmetic is done once in C, and the block always receives three typed integers:

```ruby
# each_bit_field: alignment is handled transparently.
DATA.each_bit_field(12, 10, 14) do |temp, hum, co2|
  process(temp, hum, co2)
end
```

The two versions produce identical results. The `each_bit_field` version eliminates the alignment branch entirely, and the yielded integers can be used directly in arithmetic without any further unpacking.

---

### Use Case: Parsing mruby `OP_ENTER` Argument Specs

mruby encodes a method/lambda argument specification into the 24-bit operand of `OP_ENTER`.
In `mrbgems/mruby-compiler/core/codegen.c`, `lambda_body()` builds the operand as:

```c
/* (24bits = 1:5:5:1:5:5:1:1) */
a = (noblock ? MRB_ARGS_NOBLOCK() : 0)
  | MRB_ARGS_REQ(ma)
  | MRB_ARGS_OPT(oa)
  | (ra ? MRB_ARGS_REST() : 0)
  | MRB_ARGS_POST(pa)
  | MRB_ARGS_KEY(ka, kd)
  | (ba ? MRB_ARGS_BLOCK() : 0);
genop_W(s, OP_ENTER, a);
```

The packed layout is therefore:

```text
noblock : req : opt : rest : post : key_count : kdict : block
   1       5     5      1      5        5         1       1
```

`genop_W()` emits the 24-bit value in big-endian byte order: the leading `noblock` bit lives at the MSB of `byte[0]`, and `block` lives at the LSB of `byte[2]`. The natural way to walk this record is intra-byte MSB-first, and each numeric field is the MSB-first integer formed by its 5 (or 1) bits in that order.

`lsb_first: false` matches both halves of that convention in a single call. Because this is a single non-repeating record, `each_bit_field` with a block is the right tool: the block runs exactly once and each parameter receives one field directly:

```ruby
# operand_bytes is the 3-byte operand that follows OP_ENTER (exactly 24 bits = 1+5+5+1+5+5+1+1)
operand_bytes.each_bit_field(1, 5, 5, 1, 5, 5, 1, 1, lsb_first: false) do |noblock, req, opt, rest, post, key_count, kdict, block|
  # ...
end
```

This use case is particularly valuable because it is not about image pixels or network packets. It is a real VM/compiler metadata format whose record runs MSB-first across a big-endian operand. It also illustrates the single-record decode pattern: the data width equals the record width exactly, so the block fires once and field values flow directly into named parameters without any array unpacking.
