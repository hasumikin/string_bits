## Discussion: Use Cases and Design Considerations

### Bit position numbering

All methods share one flat physical numbering scheme: position `N` lives in `byte[N / 8]` at bit `N % 8` (LSB-first within each byte).

A single keyword, `lsb_first:`, runs through every position-aware method in the proposal. It selects intra-byte numbering for methods that exchange positions with the caller (`bit_at`, `set_bit`, `each_set_bit_offset`, `bit_slice`, `bit_splice`, `bit_run_count`), and intra-byte scan direction for the walker methods (`each_bit`, `bits`, `each_bit_run`, `bit_runs`). Across-byte order is always `byte[0]` to `byte[n-1]` and is not affected by the keyword. `bit_slice` results are always packed LSB-first regardless of the keyword used for the input position, which keeps every returned `String` in one canonical layout. The full semantics are described in [BitNumbering.md](./BitNumbering.md); this section focuses only on the design consequences.

### Error behavior for out-of-range bit indices

Two distinct categories of "out of range" are handled differently.

**Index outside the string's bit length** --- read methods return `nil`; mutation methods raise `IndexError`. The asymmetry is intentional: a missed read is a logic question ("is this bit set?"), while a missed write risks silent data corruption. This mirrors Ruby's own `String#[]` (returns `nil` for out-of-bounds reads) and `String#setbyte` (raises `IndexError` for out-of-bounds writes).

**Index outside the implementation's supported integer range** --- all methods raise `ArgumentError`. The goal is deterministic behavior for clearly invalid input, rather than leaking platform-dependent conversion details into the public API. Bit indices are held internally in a pointer-width signed integer (`ssize_t`), so the supported range is the same across LP64 and LLP64 (64-bit Windows) systems.

```ruby
s = "\xFF"
s.bit_at(100)            #=> nil
s.bit_at(2**100)         #=> ArgumentError
s.bit_run_count(100, 0)  #=> nil
s.set_bit(100)           #=> IndexError
s.set_bit(2**100)        #=> ArgumentError
```

### Naming convention: symmetry with `bytes` / `each_byte`

The method pairs `each_bit`/`bits`, `each_set_bit_offset`/`set_bit_offsets`, and `each_bit_run`/`bit_runs` follow the same basic Ruby idiom as `each_byte` / `bytes`: iterator form plus collected form.

If `each_set_bit_offset`/`set_bit_offsets` feel too long, `each_setbit_offset`/`setbit_offsets` are reasonable shorter alternatives --- dropping one underscore makes the name read as three chunks (`each`/`setbit`/`offset`) instead of four.

### Why extend `String` rather than introduce a new class?

The obvious alternative is a dedicated `BitSet` class (analogous to Java's `java.util.BitSet` or C++'s `std::bitset`). Two arguments favour extending `String` instead.

**Adding a new top-level constant is a high bar for Ruby core.** Introducing `BitSet` would permanently reserve a widely plausible name and force conversion at boundaries where Ruby code already uses `String`.

**`String` is already Ruby's binary buffer type.** Socket reads, file reads, `pack`/`unpack`, and similar APIs already hand raw bytes to Ruby as `String`. Since `String` already exposes byte-level operations such as `bytesize`, `getbyte`, `setbyte`, `byteslice`, and `bytesplice`, bit-level methods are a depth extension of an existing role rather than a new category.

This proposal is intentionally limited to bit-level operations on an already materialized `String`; concerns such as zero-copy slicing or mapped I/O belong to abstractions like `IO::Buffer`, not to this proposal.

### Bit ordering across domains

| domain                                     | native bit ordering                                | native via         |
|--------------------------------------------|----------------------------------------------------|--------------------|
| Apache Arrow validity bitmap               | LSB-first (element i = byte[i/8] bit i%8)          | `lsb_first: true`  |
| ext4 block bitmap                          | LSB-first                                          | `lsb_first: true`  |
| RFC-style network headers (IPv4, TCP, DNS) | bit 0 = MSB of first byte (RFC diagram convention) | `lsb_first: false` |
| PNG 1/2/4-bit scanlines                    | MSB = leftmost pixel                               | `lsb_first: false` |

The table is limited to cases where bit numbering is directly relevant to a byte buffer held in memory. Both LSB-first and intra-byte MSB-first layouts are directly addressable through `lsb_first: true` and `lsb_first: false`.

### Why `lsb_first: true` is the default

Both numbering conventions are first-class, but `lsb_first: true` is the default for the following reasons.

**Consistency with Ruby's existing bit access.** `Integer#[]` already uses LSB-first numbering:

```ruby
4[0]   #=> 0   (4 = 0b100, bit 0 is the LSB)
4[1]   #=> 0
4[2]   #=> 1
```

A user who reaches for `String#bit_at` after using `Integer#[]` should not need to relearn which end is bit 0.

**In-memory bitmaps overwhelmingly use LSB-first.** Apache Arrow validity bitmaps, ext4 block bitmaps, Roaring bitmap containers, and most hardware peripheral register documentation (ARM Cortex-M, STM32, x86 control registers) all number bits from the LSB. This is the convention any caller building or reading an in-memory bitmap is likely to expect.

**The mapping formula is simpler.** LSB-first maps directly to byte arithmetic: `n = byte_index * 8 + bit_in_byte`. MSB-first needs an extra `7 - bit_in_byte` flip. Making the simpler form the default reduces the cognitive cost of the common case.

**MSB-first is mostly a display convention.** The MSB-first formats --- RFC network headers, PNG scanlines, BitTorrent bitfield messages --- use MSB-first as a *diagram or transmission* convention, where bits are read left-to-right. The in-memory layout itself is byte-addressable and bit-order-agnostic; the choice between LSB and MSB only matters when bits are presented to a human or transmitted with a documented numbering.

### Apache Arrow Compatibility

Apache Arrow validity bitmaps use the same flat LSB-first layout: element `i` is stored in `byte[i / 8]` at bit `i % 8`.

`bit_at(i)` maps directly to Arrow element index `i`. `each_set_bit_offset(lsb_first: true)` yields valid element indices in ascending order.

#### Arrow IPC serialization

Arrow supports zero-copy slicing in memory, so a sliced validity bitmap may start at a non-zero bit offset. The IPC format, however, requires the serialized bitmap to start at bit 0 of the first byte. `bit_slice` fills that gap:

```ruby
# column slice: validity bitmap starts at in-memory bit offset 5, covering 100 elements
# IPC requires the bitmap to start at bit 0
ipc_validity = validity_bitmap.bit_slice(5, 100)
```
