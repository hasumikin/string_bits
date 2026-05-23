# Bitmask Operation into Array

This method is the Ruby counterpart of pandas's `arr[bool_mask]` / NumPy's `np.where(mask, arr, None)` pattern: a packed boolean mask is applied to a parallel value array, keeping the elements where the corresponding bit is 1 and replacing the others with `nil`. (For pandas-style filtering with shortening, use `arr.mask(mask).compact`.)
The compositional primitives `bitwise_and` / `bitwise_or` / `bitwise_xor` / `bitwise_not` on the bitmap correspond directly to `&` / `|` / `^` / `~` on a `BooleanArray`, so building, combining, and applying masks all live within the same Ruby idiom.

## Methods for Array

- `mask(bitmap, lsb_first: true, invert: false) -> Array` -- returns a new array (allocates)
- `mask!(bitmap, lsb_first: true, invert: false) -> self` -- modifies the receiver in place

Both apply the bitmap directly without a block. The `lsb_first:` keyword follows the same convention as the `String` bit API: see [../docs/Discussion.md#bit-ordering-across-domains](../docs/Discussion.md#bit-position-numbering-of-the-string-bit-api).

#### `mask(bitmap, lsb_first: true, invert: false) -> Array`

Returns a new `Array` of the same length. Elements whose corresponding bit is 1 (for `invert: false`) or 0 (for `invert: true`) are kept; all others become `nil`.

```ruby
data   = [1, 2, 3, 4]

# LSB-first (default, Apache Arrow convention): 0b00001101 -> bits 0,2,3 set = elements 0,2,3 valid
bitmap = "\x0D".b   # 0b00001101
data.mask(bitmap)                        #=> [1, nil, 3, 4]

# MSB-first: 0b11010000 -> bits 7,6,4 set = elements 0,1,3 valid
bitmap = "\xD0".b   # 0b11010000
data.mask(bitmap, lsb_first: false)      #=> [1, 2, nil, 4]

# invert: true --- keep where bit is 0, nil where bit is 1
bitmap = "\x0D".b
data.mask(bitmap, invert: true)          #=> [nil, 2, nil, nil]
```

Apache Arrow idiom --- materialize an Arrow column with nulls applied:

```ruby
# One-time setup: build validity bitmap from source data.
bitmap = ("\x00" * ((n + 7) / 8)).b
source_data.each_with_index { |v, i| bitmap.bit_set(i) if v }

# Apply bitmap to a pre-fetched values array (no per-element rb_yield):
result = values.mask(bitmap)
```

#### `mask!(bitmap, lsb_first: true, invert: false) -> self`

Same as `mask` but modifies the array in place and returns `self`. Elements that would become `nil` are overwritten; valid elements are untouched.

```ruby
ary = [1, 2, 3, 4]
ary.mask!("\x0D".b)   #=> [1, nil, 3, 4]  (LSB-first default)
ary                      #=> [1, nil, 3, 4]  (modified in place)
```
