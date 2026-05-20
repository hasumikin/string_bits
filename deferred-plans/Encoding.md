# Proposal: `String#bit` and `BITS` Encoding

## Abstract

Introduce `String#bit` method to switch the encoding of a string to a specialized `BITS` encoding. This encoding primarily affects the inspection and pretty-printing of strings, making bit-level data more human-readable.

## Motivation

When working with binary data, `Kernel#p` and `pp` typically display strings as hex-escaped characters or raw bytes. Visualizing the actual bit patterns is often tedious, requiring manual conversion or helper methods. By introducing a `BITS` encoding, we can provide a native way to inspect bit-level representations directly.

### Precedent: `String#b` and `ASCII-8BIT`

Ruby already uses the encoding system to switch between "character-oriented" and "data-oriented" views. `String#b` sets the encoding to `ASCII-8BIT` (alias `BINARY`), effectively signaling that the string should be treated as a raw byte sequence rather than a sequence of characters.

The proposed `BITS` encoding follows this established pattern by taking it one level deeper: signaling that the string should be viewed and inspected as a sequence of bits. It remains a `String` (byte sequence), preserving all standard string behaviors (length is bytes, `[]` returns bytes), while providing a specialized "bit-level" inspection view.

## Specification

### `String#bit`

Returns a new string with its encoding set to `BITS`. This is equivalent to `String#encode("BITS")`.

```ruby
"\xAA".bit # => returns string with Encoding: BITS
```

### `Kernel#p` behavior

Strings with `BITS` encoding are inspected as a sequence of bits, prefixed by `\b` for each byte (or a similar marker).

```ruby
p "\xAA\xFF\xAA\xFF"      #=> "\xAA\xFF\xAA\xFF" (Standard behavior)
p "\xAA\xFF\xAA\xFF".bit  #=> "\b10101010\b11111111\b10101010\b11111111"
```

### `pp` (Pretty Print) behavior

`pp` will format `BITS` encoded strings to be more readable:
1. Inserts a space every 8 bits.
2. Automatically wraps the output based on the terminal's width.

```ruby
pp ("\xAA\xFF" * 11).bit
#=>
"\b10101010 11111111 10101010 11111111 10101010 11111111 10101010 11111111
 \b10101010 11111111 10101010 11111111 10101010 11111111 10101010 11111111
 \b10101010 11111111 10101010 11111111 10101010 11111111"
```

## Benefits

- **Improved Debugging**: Easier to verify bit-level manipulations (like bitwise operations, masks, or custom protocols).
- **Consistency**: Leverages Ruby's existing encoding system to change representation without changing the underlying data.
- **Readability**: Automated formatting in `pp` prevents long, unreadable strings of bits from cluttering the terminal.
