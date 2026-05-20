# Future Proposal Plan: Bit-Field Pattern Matching

This file proposes a new pattern syntax for extracting packed bit fields directly in `case/in` expressions when the target is a `String`. It is deferred from the main proposal to avoid expanding scope.

---

## Motivation

`each_bit_field` and `bit_fields` return plain integers. Pattern matching can already work on the resulting array, but requires an explicit intermediate step:

```ruby
# current: .first needed to unwrap the outer array
case pixel.bit_fields(5, 6, 5).first
in [blue, green, red]
  process(blue, green, red)
end
```

The goal is to allow the `case` target to be a plain `String` and express the bit-field layout directly in the `in` clause.

---

## Proposed Syntax

```ruby
case "\x45"
in {4: version, 4: ihl}
  # version => 5, ihl => 4  (lsb_first: true, default)
end

case "\x45"
in {4: version, 4: ihl, lsb_first: false}
  # version => 4, ihl => 5  (MSB-first; matches IPv4 byte layout)
end
```

The integer key specifies the bit width of the field. Each entry extracts that many bits sequentially from the string and matches or binds the value on the right-hand side.

### Grammar

```
bit_field_pattern ::= '{' field (',' field)* (',' 'lsb_first:' bool)? '}'
field             ::= INTEGER ':' pattern
```

`pattern` follows the same rules as the right-hand side of any other pattern: a bare identifier binds the extracted value, a literal or range matches against it.

```ruby
in {4: 4, 4: ihl}        # match only when first field == 4, bind second to ihl
in {4: (1..), 4: ihl}    # match only when first field >= 1
in {4: version, 4: ihl}  # bind both
```

### Constraints

- Only integer keys and the `lsb_first:` keyword are allowed. Any other symbol key is a `SyntaxError`.
- `lsb_first:`, if present, must appear last.
- Default for `lsb_first:` is `true`, consistent with the rest of the API.

These constraints are enforced at parse time, so there is no runtime ambiguity.

### Why this is currently a SyntaxError

```
irb> case "\x45"; in {4: version, 4: ihl}; end
SyntaxError: expected a label as the key in the hash pattern
```

Integer literals are not valid label keys in existing hash patterns, and duplicate keys are not allowed. The new grammar occupies this currently-invalid syntactic space, so there is no conflict with existing code.

---

## Implementation Strategy

The new pattern desugars to existing method calls. No VM-level changes are needed for a working prototype; only parser and compiler changes are required.

```ruby
# written
case recv
in {4: version, 4: ihl, lsb_first: false}
  process(version, ihl)
end

# compiler generates (conceptually)
_tmp = recv.bit_fields(4, 4, lsb_first: false).first
unless _tmp.nil?
  version, ihl = _tmp
  process(version, ihl)
end
```

For value-match entries, the generated code adds an equality check before binding:

```ruby
# written
in {4: 4, 4: ihl, lsb_first: false}

# generated
_tmp = recv.bit_fields(4, 4, lsb_first: false).first
unless _tmp.nil? || _tmp[0] != 4
  ihl = _tmp[1]
end
```

### Future optimization

Once the prototype is validated, the two-call chain `bit_fields(...).first` can be replaced by a private internal method `bit_fields_first` on `String` that avoids the intermediate `Array` allocation. This optimization is local to the desugar target and does not affect the public API.

---

## `String#deconstruct`

A no-op `String#deconstruct` (returning `self` or `[]`) is independent of the bit-field pattern. The compiler generates `bit_fields` calls directly for integer-keyed hash patterns and does not go through `deconstruct` at all.

The no-op is needed for a separate reason: without it, using a `String` as a `case` target in any array pattern (`in [a, b]`) raises `NoMethodError`. Defining `String#deconstruct` preemptively makes `String` a safe pattern-matching target in general.

---

## Use Cases

**IPv4 header field extraction**

```ruby
case ip_header.byteslice(0, 1)
in {4: 4, 4: ihl, lsb_first: false}
  # matched IPv4; ihl is bound
in {4: 6, 4: _, lsb_first: false}
  # matched IPv6 (different layout handled elsewhere)
end
```

**mruby OP_ENTER operand**

```ruby
case operand_bytes
in {1: noblock, 5: req, 5: opt, 1: rest, 5: post, 5: key_count, 1: kdict, 1: block, lsb_first: false}
  handle_enter(req, opt, rest)
end
```

**RGB565 single-pixel dispatch**

```ruby
case pixel
in {5: 0..3, 6: _, 5: 28.., lsb_first: true}
  "low blue, high red"
end
```
