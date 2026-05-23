# Introduce Bit Operations into String

## Relevant tickets

- Introduce #bit_count method on Integer --- https://bugs.ruby-lang.org/issues/20163

## Abstract

Ruby's `String` is already a byte sequence, but it lacks high-level bit operations.
As a result, packed binary data must be handled with manual byte arithmetic.

For example, checking set-bit at offset `10` currently requires calculating the byte position and the bit's position within that byte:

```ruby
data = "\xAA\xAA\xAA\xAA"

# The "classic" way
byte_offset = 10 / 8
byte = data.getbyte(byte_offset)
bit_offset = 10 % 8
((byte >> bit_offset) & 1) == 1 #=> false

# The "concise" way using Integer#[]
data.getbyte(10 / 8)[10 % 8] == 1 #=> false
```

The concise form is a single line, but it still leaks implementation details into every call site: the caller writes `10 / 8` and `10 % 8` by hand (a recurring source of off-by-one errors at byte boundaries), and the `Integer` result must be compared against `1` to be used as a boolean. With a bit-addressed API, `data.bit_at(10)` takes a bit position and returns `true`/`false` directly. The cost compounds for iteration, run-length scanning, or splicing, where each operation otherwise needs its own byte/bit arithmetic.

With a native bit-level API, the string is treated as a first-class **bit sequence**:

```ruby
data = "\xAA\xAA\xAA\xAA"
data.bit_at(10) #=> false
```

By providing a flat bit-addressing model that handles the underlying bit-to-byte mapping, we allow developers to focus on the logical layout of their data (e.g., an Apache Arrow bitmap or a pixel buffer), making the code more readable and less error-prone.

This proposal presents a family of methods for bit-oriented use of `String`, organized into groups below.
The immediate goal is agreement on the overall direction and feedback on which subset should be pursued first.

Presenting the full menu matters because some design questions only become clear at that level:

- bit numbering keyword (`lsb_first:`) and its consistent application across methods
- naming symmetry such as `bits` / `each_bit`
- behavior for out-of-range bit indices

## Implementation (Prototype)

You can try this out with `gem install string_bits`.
The source code can be seen in https://github.com/hasumikin/string_bits

## Proposed Methods

Full prototype and documentation:
https://github.com/hasumikin/string_bits/blob/0.3.0/docs/ProposedMethods.md

**Read**

- `bit_at(n, lsb_first: true) -> true | false | nil` -- read a single bit
- `bit_count -> Integer` -- count of set-bits (popcount)
- `bit_run_count(pos, bit, lsb_first: true) -> Integer | nil` -- length of the run of `bit` starting at pos

**Iterator**

- `each_bit(lsb_first: true) { |bool| ... } -> self` -- yield each bit as true/false  
  `each_bit(lsb_first: true) -> Enumerator`
- `bits(lsb_first: true) -> Array` -- Array form of `each_bit`  
  `bits(lsb_first: true) { |bool| ... } -> self`
- `each_bit_run(lsb_first: true) { |bool, len| } -> self` -- yield `(bool, run_length)` pairs  
  `each_bit_run(lsb_first: true) -> Enumerator`
- `bit_runs(lsb_first: true) -> Array` -- Array form of `each_bit_run`  
  `bit_runs(lsb_first: true) { |bool, len| } -> self`
- `each_bit_offset(bit, lsb_first: true) { |n| ... } -> self` -- yield position of each bit equal to `bit`  
  `each_bit_offset(bit, lsb_first: true) -> Enumerator`
- `bit_offsets(bit, lsb_first: true) -> Array` -- Array form of `each_bit_offset`  
  `bit_offsets(bit, lsb_first: true) { |n| ... } -> self`

**Mutation**

- `bit_set(n_or_range, lsb_first: true) -> self` -- set one bit or a logical bit range to 1
- `bit_clear(n_or_range, lsb_first: true) -> self` -- set one bit or a logical bit range to 0
- `bit_flip(n_or_range, lsb_first: true) -> self` -- toggle one bit or a logical bit range
- `bit_splice(bit_index, bit_length, str, lsb_first: true) -> self`  
  `bit_splice(bit_index, bit_length, str, str_bit_index, str_bit_length, lsb_first: true) -> self`  
  `bit_splice(range, str, lsb_first: true) -> self`  
  `bit_splice(range, str, str_range, lsb_first: true) -> self` -- write a sub-sequence of bits in place (bit-granularity `bytesplice`)

**Slice**

- `bit_slice(bit_offset, bit_length, lsb_first: true) -> String | nil` -- extract a sub-sequence of bits (bit-granularity `byteslice`)  
  `bit_slice(range, lsb_first: true) -> String | nil`

**Bitwise**

- `bitwise_not -> String` / `bitwise_not! -> self` -- invert every bit
- `bitwise_and(other) -> String` / `bitwise_and!(other) -> self` -- bitwise AND
- `bitwise_or(other) -> String` / `bitwise_or!(other) -> self` -- bitwise OR
- `bitwise_xor(other) -> String` / `bitwise_xor!(other) -> self` -- bitwise XOR

## Performance

This is not only about convenience.
In a prototype implementation (string_bits gem), bulk operations such as `bitwise_and`, `bitwise_or`, and `bit_count` are also substantially faster than Ruby-level loops over bytes (see the Benchmark link below).
I do not think performance alone is the reason to add the feature, but it is a practical benefit.

## Notes

Benchmarks, discussion, and prior art:

- Proposed methods (with use cases): https://github.com/hasumikin/string_bits/blob/0.3.0/docs/ProposedMethods.md
- Benchmark: https://github.com/hasumikin/string_bits/blob/0.3.0/docs/Benchmark.md
- Discussion: https://github.com/hasumikin/string_bits/blob/0.3.0/docs/Discussion.md
    - Why extend `String` rather than introduce a new class?
    - Naming convention: symmetry with `bytes` / `each_byte`
    - Error behavior for out-of-range bit indices
    - Bit Position Numbering of the String bit API
    - Why `lsb_first: true` is the default?
    - Bit ordering across domains
    - Apache Arrow Compatibility
- Prior art: https://github.com/hasumikin/string_bits/blob/0.3.0/docs/PriorArt.md

