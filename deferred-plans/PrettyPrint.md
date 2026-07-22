# Third-Party Gem Plan: `String#bit` Binary Inspection

> **This is NOT a proposal for Ruby core.** It is a plan for a separate third-party gem. The bit-manipulation methods in docs/Main.md target Ruby core; the binary-inspection feature described here deliberately does not. A debugging / visualization convenience is a better fit for a gem, which can iterate freely, requires no core approval, and does not add a permanent method to everyone's `String`. If the gem proves widely useful, that adoption itself becomes evidence for a possible future core proposal.

## Abstract

Provide `String#bit`, which returns a lightweight wrapper whose `inspect` and `pretty_print` render the byte sequence as a binary (bit) string. This makes bit-level data readable at the irb prompt and under `p` / `pp`, without changing the underlying data.

## Design

### `String#bit(lsb_first: false) -> Bits`

Returns a `Bits` instance wrapping the receiver's bytes.

### `Bits < ::String`

`Bits` subclasses `String` so the wrapped value still behaves as the original byte sequence (it can be passed anywhere a `String` is expected, and the bit-manipulation methods still work on it). Only `inspect` and `pretty_print` are overridden to render binary.

```ruby
class String
  def bit(lsb_first: false)
    Bits.new(self, lsb_first: lsb_first)
  end

  class Bits < ::String
    def initialize(entity, lsb_first: false)
      super(entity)
      @lsb_first = lsb_first
    end

    def inspect
      directive = @lsb_first ? "b*" : "B*"
      "\"#{unpack1(directive).scan(/.{1,8}/).join(' ')}\""
    end

    def pretty_print(q)
      q.text(inspect)
    end
  end
end

"\xAA".bit         #=> "10101010"
("\xAA\xFF").bit   #=> "10101010 11111111"
```

### Bit ordering default: `lsb_first: false`

The default here is `lsb_first: false` (MSB-first), which is the opposite of the core proposal's `lsb_first: true` default. This is intentional for a display feature: MSB-first renders `0xAA` as `"10101010"`, the same order a human writes `0b10101010` and the same result as `unpack1("B*")`, which is what one expects when visually checking "does this byte look right?".

The divergence from the core proposal's default is a conscious trade-off and must be stated plainly in the gem's README, because a user switching between `bit_set?` (LSB-first by default) and `#bit` (MSB-first by default) would otherwise be surprised by the reversed order. The rule of thumb: data manipulation defaults to LSB-first, visual display defaults to MSB-first.
