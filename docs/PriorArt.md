# Prior Art: Bit Operations in Other Languages

Bit-level operations are common across languages, but they are typically exposed either as low-level primitives on integers or through dedicated container types. This proposal instead attaches bit-oriented operations to Ruby's existing byte container, `String`.

## Python

Python provides limited standard bit-level functionality:

* `int.bit_count()` --- population count (popcount)
* `int.to_bytes()` / `int.from_bytes()` --- conversion between integers and byte sequences

The standard library keeps bit operations on numeric values rather than on byte-sequence containers.

## Java

Java provides `BitSet`, a dedicated mutable bit container:

* `get(int index)` --- read bit
* `set(int index)` / `clear(int index)` / `flip(int index)` --- mutate
* `nextSetBit(int fromIndex)` --- iterate set bits
* `cardinality()` --- population count

Iteration over set bits is positional:

```java
for (int i = bs.nextSetBit(0); i >= 0; i = bs.nextSetBit(i + 1)) {
    // ...
}
```

## Rust

Rust exposes bit operations at both the primitive and library level:

* Integer methods: `count_ones()` (popcount), `trailing_zeros()` (ctz)
* External crates such as `bitvec`: `get(index) -> Option<bool>`, `set(index, bool)`, `iter()`

## C++

C++ exposes bit-oriented containers through types such as `std::bitset` and the packed specialization `std::vector<bool>`.

`std::bitset` in particular uses technical bit-manipulation APIs:

* `test(pos)` --- read bit
* `set(pos)` / `reset(pos)` / `flip(pos)` --- mutate
* `count()` --- popcount

## Go

Go provides low-level bit operations via `math/bits`:

* `bits.OnesCount(x)` --- popcount

## Python (pandas / NumPy)

The Python data analysis stack treats Boolean-valued arrays as boolean masks, often backed by packed bitmaps. Element-wise comparison produces such an array, the standard bitwise operators compose them, and they are used to filter parallel value arrays:

```python
import pandas as pd

a = pd.array([1, 2, 3, 4])

a > 2              # BooleanArray [False, False, True, True]
(a > 2) & (a < 4)  # BooleanArray [False, False, True, False]
a[a > 2]           # IntegerArray [3, 4]
```

The `bit_and` / `bit_or` / `bit_xor` / `bit_not` methods proposed here are the same compositional primitives applied to a `String` viewed as a packed bitmap.

Apache Arrow's validity bitmap convention --- supported natively via `lsb_first: true` --- follows the same general model: a packed boolean mask used to express which positions of a parallel array are valid or selected.

## Erlang

Erlang treats bitstrings and binaries as first-class bit-level sequences
via its bit syntax, supporting pattern matching and construction at
arbitrary bit granularity:

```erlang
%% Match the first 3 bits as A and the remaining 5 as B
<<A:3, B:5>> = <<255>>.          %% A = 7, B = 31

%% Extract one bit, leaving the rest as a bitstring
<<X:1, Rest/bitstring>> = SomeBits.
```

Among major languages, Erlang's bit syntax is the closest prior art for this proposal. Bits are exposed directly on the same data type that holds byte buffers, with no intermediate container class. The mechanism differs (pattern matching vs. method calls), but the architectural choice --- bit-level access on the existing byte container --- is the same.

## Summary

Across languages, bit operations typically fall into three categories:

| approach             | languages                | characteristics                               |
|----------------------|--------------------------|-----------------------------------------------|
| integer-centric      | Python, Go               | bit operations tied to numeric types          |
| dedicated container  | Java, C++, Rust (bitvec) | separate types, explicit APIs                 |
| packed boolean masks | Python (pandas / NumPy)  | bitmap as a first-class predicate / filter    |
| in-buffer bit access | Erlang (bit syntax)      | bit operations on the existing byte container |

This proposal is closest in spirit to Erlang: **integrate bit-level operations directly into `String`, the existing byte container**, but adapted to Ruby's method-call style.

Compared with the above:

* No new container type in the public API
* Operations on existing byte buffers held in `String`
* Iteration methods aligned with Ruby's `each_*` style
* Naming intended to parallel `String#bytes` / `each_byte`
