ref: Integer#popcount https://bugs.ruby-lang.org/issues/20163

----

# Introduce Bit Operations into String

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
((byte >> bit_offset) & 1) == 1

# The "concise" way using Integer#[]
(data.getbyte(10 / 8)[10 % 8]) == 1
```

While the latter is a single line, the developer is still forced to maintain a **C-like procedural mental model**: thinking in terms of "which byte" and "which bit inside it." This "byte + bit-offset" translation adds constant cognitive load to every bit-level operation.

With a native bit-level API, the string is treated as a first-class **bit sequence**:

```ruby
data = "\xAA\xAA\xAA\xAA"
data.bit_at(10)
```

By providing a flat bit-addressing model that handles the underlying bit-to-byte mapping, we allow developers to focus on the logical layout of their data (e.g., an Apache Arrow bitmap or a pixel buffer), making the code more readable and less error-prone.

This proposal presents a family of methods for bit-oriented use of `String`, organized into groups below.
The immediate goal is agreement on the overall direction and feedback on which subset should be pursued first.

Presenting the full menu matters because some design questions only become clear at that level:

- bit numbering keyword (`lsb_first:`) and its consistent application across methods
- naming symmetry such as `bits` / `each_bit`
- behavior for out-of-range bit indices

## Implementation

- You can try this out with `gem install string_bits`.

## Proposed Methods

Full prototype and documentation:
https://github.com/hasumikin/string_bits/blob/master/docs/ProposedMethods.md

**Read**

- `bit_at(n, lsb_first: true) -> true | false | nil` -- read a single bit
- `bit_count -> Integer` -- count of set-bits (popcount)
- `bit_run_count(pos, bit, lsb_first: true) -> Integer | nil` -- length of the run of `bit` starting at pos

**Iterator**

- `each_bit(lsb_first: true) { |bool| ... } -> self` -- yield each bit as true/false  
  `each_bit(lsb_first: true) -> Enumerator`
- `bits(lsb_first: true) -> Array` -- Array form of `each_bit`  
  `bits(lsb_first: true) { |bool| ... } -> self`
- `each_bit_run(lsb_first: true) { |bit, len| } -> self` -- yield `(bit, run_length)` pairs  
  `each_bit_run(lsb_first: true) -> Enumerator`
- `bit_runs(lsb_first: true) -> Array` -- Array form of `each_bit_run`  
  `bit_runs(lsb_first: true) { |bit, len| } -> self`
- `each_set_bit_offset(lsb_first: true) { |n| ... } -> self` -- yield position of each set-bit    
  `each_set_bit_offset(lsb_first: true) -> Enumerator`
- `set_bit_offsets(lsb_first: true) -> Array` -- Array form of `each_set_bit_offset`  
  `set_bit_offsets(lsb_first: true) { |n| ... } -> self`

**Mutation**

- `set_bit(n_or_range, lsb_first: true) -> self` -- set one bit or a logical bit range to 1
- `clear_bit(n_or_range, lsb_first: true) -> self` -- set one bit or a logical bit range to 0
- `flip_bit(n_or_range, lsb_first: true) -> self` -- toggle one bit or a logical bit range
- `bit_splice(bit_index, bit_length, str, lsb_first: true) -> self`  
  `bit_splice(bit_index, bit_length, str, str_bit_index, str_bit_length, lsb_first: true) -> self`  
  `bit_splice(range, str, lsb_first: true) -> self`  
  `bit_splice(range, str, str_range, lsb_first: true) -> self` -- write a sub-sequence of bits in place (bit-granularity `bytesplice`)

**Slice**

- `bit_slice(bit_offset, bit_length, lsb_first: true) -> String | nil` -- extract a sub-sequence of bits (bit-granularity `byteslice`)  
  `bit_slice(range, lsb_first: true) -> String | nil`

**Bitwise**

- `bit_not -> String` / `bit_not! -> self` -- invert every bit
- `bit_and(other) -> String` / `bit_and!(other) -> self` -- bitwise AND
- `bit_or(other) -> String` / `bit_or!(other) -> self` -- bitwise OR
- `bit_xor(other) -> String` / `bit_xor!(other) -> self` -- bitwise XOR

## Performance

This is not only about convenience.
In a prototype implementation, bulk operations such as `bit_and`, `bit_or`, and `bit_count` are also substantially faster than Ruby-level loops over bytes.
I do not think performance alone is the reason to add the feature, but it is a practical benefit.

## Notes

Detailed benchmarks, discussion, and prior art:

- Proposed methods: https://github.com/hasumikin/string_bits/blob/master/docs/ProposedMethods.md
- Benchmark: https://github.com/hasumikin/string_bits/blob/master/docs/Benchmark.md
- Discussion: https://github.com/hasumikin/string_bits/blob/master/docs/Discussion.md
- Bit numbering: https://github.com/hasumikin/string_bits/blob/master/docs/BitNumbering.md
- Prior art: https://github.com/hasumikin/string_bits/blob/master/docs/PriorArt.md
