# string_bits gem

Ruby's `String` is already a byte sequence. This gem extends it one level lower: **a bit sequence** --- making packed binary buffers first-class values without any new class.

The methods are designed for real workloads: Apache Arrow validity bitmaps, bitmap font glyph data, and any protocol buffer where bits carry meaning. Many methods read or modify the existing string bytes directly; methods such as `bit_slice`, `bit_not`, `bit_and`, `bit_or`, `bit_xor`, `bits`, `set_bit_offsets`, `bit_runs`, and `Array#mask` allocate a derived result object. Because the API relies solely on `String`, the same methods would benefit memory-constrained implementations such as mruby and PicoRuby once adopted into CRuby.

A single keyword, `lsb_first:`, controls bit ordering across the API: it selects intra-byte numbering wherever positions are exchanged with the caller, and intra-byte scan direction wherever the API walks the sequence. Across-byte order is always `byte[0]` first, and result Strings from `bit_slice` are always packed LSB-first regardless, so `data.each_set_bit_offset(lsb_first: false).all? { |n| data.bit_at(n, lsb_first: false) }` stays true.

## Usage

```
gem install string_bits
```

```ruby
require "string_bits"
puts "\xAA\xAA\xAA\xAA".bit_at(10)
#=> false

puts "\xAA".each_bit(lsb_first: false).to_a.inspect
#=> [true, false, true, false, true, false, true, false]
```
