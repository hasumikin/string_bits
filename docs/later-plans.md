# After v1: Roadmap for the Remaining String Bit Operations

This document is the working plan for proposing the rest of the string_bits menu to
Ruby core, now that the v1 subset has been merged. For each phase it records the
use-case argument and the technical argument that the ticket should lead with.

## Current status

v1 ("Introduce Basic Bit Operations into String") is in ruby/ruby:

- Ticket: [Feature #22118] (child of the parent ticket #22082)
- PR: https://github.com/ruby/ruby/pull/17353
- Merged: commit `2461caebba` (2026-08-06), NEWS entry in `43c9190369`
- Methods: `bit_get`, `bit_set?`, scalar `bit_set` / `bit_clear` / `bit_flip`,
  no-arg `bit_count`, `bitwise_not(!)`, `bitwise_and(!)`, `bitwise_or(!)`,
  `bitwise_xor(!)`

### Contracts settled by v1 that every follow-up inherits

These were the contested points, and they are now precedent. Follow-up tickets
should cite them as settled rather than reopening them:

1. `lsb_first:` keyword name, `true` default, and strict `true`/`false`
   validation (explicit `nil` raises `ArgumentError`).
2. Read out-of-range returns `nil` / write out-of-range raises `IndexError`.
3. Negative bit offsets are rejected with `IndexError`; no from-the-end
   interpretation.
4. Offsets are held in a 64-bit unsigned integer; `2**64` raises
   `ArgumentError` before the range rule applies.
5. Non-destructive results are `Encoding::BINARY`; destructive methods leave
   the receiver's encoding untouched (the `setbyte` precedent).
6. Binary bitwise operations require equal `bytesize` (`ArgumentError`
   otherwise); callers pad explicitly with `ljust`.
7. Destructive-only methods take no `!` suffix (the `setbyte` precedent);
   `!` marks the dangerous member of a destructive/non-destructive pair.

Every follow-up below is designed so that it only *applies* these rules to new
arities and methods, never extends or contradicts them.

## Inventory: what is not yet proposed

### Tier A --- in the main menu (ProposedMethods.md), prototyped and tested in the gem

| # | Feature | Methods |
|---|---------|---------|
| A1 | Range/length forms of accepted methods | `bit_count(offset, length)`, `bit_count(range)` (+ `lsb_first:`), `bit_set` / `bit_clear` / `bit_flip` with `(offset, length)` / `(range)` |
| A2 | Bit iteration | `each_bit`, `bits`, `each_bit_offset`, `bit_offsets` |
| A3 | Run-length operations | `bit_run_count`, `each_bit_run`, `bit_runs` |
| A4 | Bit slicing | `bit_slice(offset, length)`, `bit_slice(range)` |
| A5 | Bit splicing | `bit_splice` (all forms) |

### Tier B --- documented in deferred-plans/, intentionally outside the main menu

| # | Feature | Methods | Prototype status |
|---|---------|---------|------------------|
| B1 | Array masking | `Array#mask`, `Array#mask!` | implemented and tested |
| B2 | Scalar bit scan | `bit_index`, `bit_rindex` | documented only |
| B3 | Integer-view slice/splice | `bit_field_slice`, `bit_field_splice` | documented only |
| B4 | Packed bit-field iteration | `each_bit_field`, `bit_fields` | implemented and tested |
| B5 | Whole-bitmap shift | `bit_lshift(!)`, `bit_rshift(!)` | documented only |
| B6 | Combining splice (bit-blit) | `bit_splice(..., op: :and/:or/:xor)` | documented only |

### Tier C --- not core method proposals

| # | Feature | Route |
|---|---------|-------|
| C1 | Bit-field pattern matching (`in {4: version, 4: ihl}`) | Needs a parser change; separate long-term proposal, only after B4 settles the field-decoding semantics |
| C2 | `String#bit` binary inspection (PrettyPrint.md) | Third-party gem by design; explicitly NOT for core |

## Proposal phases

The first wave (Phases 1-5) covers Tier A and follows the dependency order of the
internal helpers: each phase builds on semantics the previous one has already made
precedent, and the two spec-densest features (`bit_slice` / `bit_splice`) come
last, when the position arithmetic, range handling, and iteration conventions are
no longer open questions. Phase 6 (Array#mask) opens the second wave.

One ticket per phase, each a child (or sibling, where noted) of parent #22082,
each citing #22118 for the settled contracts.

---

### Phase 1: Add Range Support to String Bit Operations

**Methods:** `bit_count(bit_offset, bit_length)`, `bit_count(bit_range)`,
`bit_count(... , lsb_first:)`; `bit_set` / `bit_clear` / `bit_flip` with
`(bit_offset, bit_length)` and `(bit_range)` forms.

**Why first (technical):**

- Pure arity extension of methods that are already in core; zero new names, so
  the naming discussion is already closed. The diff is small and the review
  burden is low.
- v1 documented one temporary exception: no-arg `bit_count` takes no
  `lsb_first:`. This phase resolves that exception exactly the way v1's
  documentation promised ("if arguments for specifying a range are introduced in
  the future, the bit-order keyword should be introduced at that time"), which
  makes the ticket read as the planned continuation rather than a new idea.
- This phase fixes, once, the helpers every later phase needs: bit-range
  parsing, the read-clamps / write-raises distinction applied to regions
  (a count past the end clamps to what exists; a mutation past the end raises
  `IndexError`), and negative range endpoints (`IndexError`). Settling these on
  familiar methods means Phases 2-5 inherit them instead of debating them.

**Why first (use case):**

- The bitmap-allocator pattern needs the region forms to be atomic and fast:
  verify a candidate region is free with `bit_count(offset...(offset + n)) == 0`,
  then commit with `bit_set(offset...(offset + n))`. With only scalar v1 methods
  this is a Ruby-level loop per bit.
- Apache Arrow: count nulls in a row-group sub-range (`bit_count(lo, len)`),
  bulk-initialize a validity region (`bit_set(range)`).
- The range forms are also what makes benchmarks against `String#count` and
  hand-rolled loops most persuasive; the gem benchmark files
  (`bit_count_range.yaml`, `bit_set_range.yaml`, ...) are ready to quote.

**Discussion points to anticipate:** whether `bit_count`'s region form should
clamp or raise for a region past the end (the plan: clamp, because it is a read;
cite contract 2); endless / beginless ranges (follow `byteslice` conventions).

---

### Phase 2: Introduce Bit Iteration Methods for String

**Methods:** `each_bit(start_offset=0)`, `bits`, `each_bit_offset(bit, start_offset=0)`,
`bit_offsets` (all with `lsb_first:`).

**Why second (technical):**

- Read-only; no mutation-side questions at all. `each_bit` is semantically an
  iterated `bit_get`, so correctness is easy to specify against already-merged
  behavior.
- Two conventions get settled here that the run family (Phase 3) then reuses:
  (a) bits are yielded as Integer `1` / `0` --- the representation `bit_get`
  already returns, and the same choice `Integer#[]` and `each_byte` embody
  (predicate-style consumption stays with `bit_set?`, and position-oriented
  filtering with `each_bit_offset`) --- and (b) where a method takes a bit as
  an *argument* (`each_bit_offset`), `true` / `false` are accepted as aliases
  of `1` / `0`. Settling (b) here avoids re-arguing it for `bit_run_count`.
- The `Enumerator`-without-block, `self`-with-block shape follows `each_byte` /
  `each_char` precedent directly.

**Why second (use case):**

- `each_bit_offset` carries the strongest stories: enumerate valid (or null)
  element indices from an Arrow validity bitmap, owned/unowned piece indices
  from a BitTorrent bitfield (`lsb_first: false`, positions directly usable as
  piece indices), free blocks in an ext4-style block bitmap.
- `each_bit` covers MSB-first packed streams (Huffman codes, PNG 1-bit
  scanlines) where the consumer genuinely wants every bit in order.
- Without these, v1 users iterate with `(0...s.bytesize*8).each { |i| s.bit_get(i) }`,
  which is both slow and noisy --- an easy before/after to show.

**Discussion points to anticipate:** whether `bits` (Array-returning) is worth
adding alongside the Enumerator form (argument: parallel to `bytes` / `chars`);
`start_offset` semantics (absolute positions are always yielded).

---

### Phase 3: Introduce Bit Run Methods for String

**Methods:** `bit_run_count(bit, bit_offset)`, `each_bit_run(start_offset=0)`,
`bit_runs` (all with `lsb_first:`).

**Why a separate ticket (technical):**

- Run-length is a distinct concept, not an iteration variant: it needs its own
  definitions (a run is one or more identical bits; runs merge across byte
  boundaries; under `lsb_first: false` the merge happens in MSB-first scan
  order). Bundling this into Phase 2 would make that ticket carry two debates.
- The `nil`-never-`0` rule for `bit_run_count` (no run exists vs. a
  zero-length run is a contradiction in terms) deserves its own focused
  explanation; it is the kind of detail that derails a broader ticket.
- Depends on Phase 2 only for the `0`/`1`-as-bit-argument convention.

**Why this order (use case):**

- RLE encoding is the primary motivation and is a one-line before/after against
  the 15-line `each_bit` version.
- Bitmap allocator next-fit: `each_bit_run(last_alloc)` finds the first free
  run of sufficient length without restarting at bit 0;
  `bit_run_count(0, pos)` checks capacity at a known position.
- Embedded/protocol: PPP/HDLC flag detection and bit-stuffing over UART needs
  MSB-first runs that straddle byte boundaries --- a use case that motivates
  `lsb_first: false` beyond addressing.

---

### Phase 4: Introduce Bit Slicing for String

**Methods:** `bit_slice(bit_offset, bit_length)`, `bit_slice(bit_range)`
(with `lsb_first:`).

**Why after the iterators (technical):**

- The one genuinely new design decision is the result-packing rule: the
  coordinates follow `lsb_first:`, but the result String is always packed
  LSB-first with the unused high bits of the last byte cleared, so every other
  bit method works on the result under its default convention. That rule is
  much easier to defend when `bit_set?` / `each_bit_offset` behavior on the
  result can be demonstrated against already-merged methods.
- Everything else is inherited: `byteslice` naming and shape, Phase 1 range
  parsing, contract 2 for `nil` returns, contract 5 for `Encoding::BINARY`.

**Use case:**

- Apache Arrow IPC: in-memory Arrow arrays may carry a non-zero bit offset, but
  the IPC format requires offset 0; normalizing a validity bitmap is exactly
  `bit_slice(slice_offset, slice_length)`. This is the flagship example.
- Extracting an MSB-first wire-format field (PNG low-bit-depth scanline region,
  RFC header sub-field) at the coordinate the spec diagram writes, with numeric
  significance preserved (`"\xAC".bit_slice(0, 4, lsb_first: false)` => `"\x0A"`).

---

### Phase 5: Introduce Bit Splicing for String

**Methods:** `bit_splice(bit_offset, bit_length, str, str_bit_offset=0)`,
`bit_splice(bit_range, str, str_bit_offset=0)` (with `lsb_first:`).

**Why last in the first wave (technical):**

- Highest spec density in the menu: destination addressing under `lsb_first:`,
  source offset semantics, length always taken from the destination, and the
  no-resize rule (unlike `bytesplice`, because partial bytes cannot be shifted
  to make room). Proposing it as the inverse of an already-merged `bit_slice`
  cuts the surface in half: the round-trip property
  (`buf.bit_splice(o, l, src.bit_slice(o, l))` restores the bits, in both
  orderings) becomes the spec.
- Source-out-of-range raises `IndexError` even though it is a "read" of `str`
  --- because the operation as a whole is a write; this nuance is easier to
  state once contract 2 has been applied in four previous tickets.

**Use case:**

- Overwrite a sub-range of an Arrow validity bitmap in place
  (`bitmap.bit_splice(40, 40, new_mask)`).
- Compose packed wire-format buffers: write a field into a non-byte-aligned
  position without read-modify-write byte arithmetic.
- Completes the read/write symmetry of the API (slice/splice), which is itself
  an argument: the API is finished, not open-ended.

---

### Phase 6: Introduce Array Masking with String Bitmaps

**Methods:** `Array#mask(bitmap, lsb_first:, invert:)`, `Array#mask!`.
Prototyped and tested in the gem (`deferred-plans/Array-mask.md`).

**Positioning (technical):**

- A sibling ticket of parent #22082, not a child: it extends `Array`, so it
  should not ride on the String tickets' momentum, and String reviewers should
  not be forced into Array API design.
- Requires only that `lsb_first:` and the bitmap conventions are stable ---
  which is true after Phase 1-2; it does not need slice/splice. It is placed
  after the first wave so that the String discussion is closed, not because of
  a hard dependency. If the first wave stalls at Phase 4-5, this can be
  proposed in parallel.

**Use case:**

- The pandas `arr[bool_mask]` / NumPy `np.where(mask, arr, None)` pattern in
  Ruby: apply a packed validity bitmap to a parallel value array without a
  per-element block (`values.mask(bitmap)`, nulls as `nil`;
  `.mask(bitmap).compact` for filtering).
- Closes the loop with `bitwise_*`: building, combining (`bitwise_and` for
  filter-AND-validity), and applying masks all stay in one idiom. Red Arrow /
  Red Data Tools integration is the concrete ecosystem story.

**Risk:** Array methods attract naming bikeshed (`mask` vs `filter_by_bitmap`
etc.) and `nil`-vs-drop semantics debate. The doc's answer (keep length, `nil`
holes, `compact` to shorten) should lead the ticket.

---

## Second wave (Tier B remainder) --- propose on demand, not on schedule

These are deliberately not scheduled into the phase sequence. Each has a
documented trigger; proposing them without the trigger weakens the whole
program by making the API look open-ended.

### B2: `bit_index` / `bit_rindex` --- trigger: performance evidence

Derivable from `each_bit_offset(...).first`, so they add no capability, only a
zero-allocation scalar form that can lower to hardware bit-scan instructions
(`ffs` / `ctz` / `clz`). Propose after Phase 2 is merged, with allocator-style
benchmarks (Enumerator allocation per call vs. none) as the centerpiece, and
the mruby/PicoRuby portability argument (kernel-style `find_first_bit` /
`find_next_zero_bit` on microcontrollers). The `String#index` / `rindex`
naming parallel makes the ticket short. Note one deliberate divergence to
resolve: the deferred doc allows a negative `start_offset` counting from the
end (the `String#index` precedent), which conflicts with contract 3; the
proposal should follow contract 3 (reject negatives) unless the dev meeting
prefers the `index` parallel.

### B4 then B3: the Integer-view family --- trigger: field-decoding demand

`each_bit_field` / `bit_fields` (B4) yield decoded `Integer` field values ---
a different yield type from everything else, and the reason they were deferred.
The RGB565 / sensor-record use cases are strong for the embedded story, but
this family reopens two questions the first wave never touches: the numeric
significance convention (LSB-first = little-endian reading) and the maximum
field width (64-bit vs. `MRB_INT_BIT - 1` portability cap). Propose B4 first
and alone; `bit_field_slice` / `bit_field_splice` (B3) then follow as the
scalar/write counterparts reusing B4's significance convention. B3's write
side (`bit_field_splice`) is the practically valuable half (today it takes a
`pack` + `bit_splice` round-trip), so if B4 stalls on the iteration design,
consider proposing B3 alone with the significance convention defined locally.

### B5: `bit_lshift` / `bit_rshift` --- trigger: a concrete alignment need

Niche. The honest motivations are aligning validity bitmaps sliced at
different bit offsets before a `bitwise_*` combine, and shift-register
algorithms (LFSR, CRC, serial framing). Hold until a real request appears
(e.g. from Red Arrow work); if Phase 4-5 discussions surface the alignment
problem organically, that is the moment to file it. Depends on the
significance convention --- so B4 (or at least its convention) should land
first, or the ticket must define significance standalone.

### B6: `bit_splice(op:)` bit-blit --- trigger: after Phase 5, embedded pull

Purely additive keyword (`op: :and / :or / :xor`, default `:copy` identical to
Phase 5 behavior), so it can come any time after `bit_splice` merges. The
motivating domain is 1-bpp raster compositing on embedded displays (SSD1306,
e-paper) --- strongest when paired with a PicoRuby/mruby demonstration. Low
urgency, high demo value.

## Out of core, on purpose (Tier C)

- **C1 Pattern matching** (`case str; in {4: version, 4: ihl}`): a parser
  change, so the audience and process differ from method tickets entirely.
  Prerequisites: B4 merged (establishes field decoding semantics) and visible
  usage of `bit_fields` in the wild. Treat as a Ruby 4.x-horizon idea; do not
  attach it to any first-wave ticket, and do not mention it in them --- it
  makes the method proposals look like a syntax campaign.
- **C2 `String#bit` inspection**: ship and maintain as a third-party gem
  (MSB-first display default, documented as such). Its adoption is future
  *evidence*, not a proposal. Keeping it out of core is itself a talking point:
  the core API stays data-manipulation only.

## Sequencing and cadence notes

- v1 merged 2026-08-06, i.e. inside the Ruby 3.6 development cycle
  (3.6.0 expected 2026-12-25). Phases 1-3 are small enough that, filed
  promptly and taken to consecutive monthly dev meetings, they can plausibly
  land in the same release as v1 --- which is worth doing: shipping v1 alone
  for a full release invites "is this API abandoned half-done?" impressions.
  Target: Phase 1 immediately, Phase 2 after Phase 1's dev-meeting outcome,
  Phase 3 in the same window if Phase 2 is uncontroversial.
- Phases 4-5 should not be rushed against the 3.6 freeze; slice/splice carry
  the spec density, and a hurried packing-rule discussion is the biggest risk
  in the program. Landing them early in the 3.7 cycle is the realistic default,
  with 3.6 as upside if Phases 1-3 go smoothly.
- Keep one ticket per phase and resist scope-merging suggestions ("why not add
  runs to the iterator ticket?"): the per-ticket focus is what made v1
  reviewable. If a dev meeting asks to merge Phase 2 and 3, accept only if
  both are otherwise approved.
- Before each filing: sync the gem so its behavior matches the proposal text
  exactly (it is cited as the reference implementation), refresh the relevant
  `benchmark/*.yaml` numbers against ruby master (which now contains the v1
  C implementation, changing baselines for range/iterator comparisons), and
  add conformance tests mirroring `test_core_v1_conformance.rb` for the new
  surface.
- The gem remains the staging ground: methods merged into core should be
  feature-gated out of the gem's C extension at build time on new-enough
  rubies (as done for v1) so `require "string_bits"` stays a no-op-safe
  polyfill.
