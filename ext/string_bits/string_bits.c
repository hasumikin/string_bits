#include "ruby.h"
#include "ruby/encoding.h"

#include <limits.h>     /* CHAR_BIT */
#include <stdint.h>     /* uint64_t, UINT64_MAX, int64_t, intptr_t */
#include <inttypes.h>   /* PRIdPTR (ssize_t via intptr_t), PRId64 */
#include <string.h>     /* memcpy */
#include <sys/types.h>  /* ssize_t (Ruby typedefs it on Windows) */

/* Whole-string bit length, computed in 64 bits.
 *
 * RSTRING_LEN returns a pointer-width signed length, so `RSTRING_LEN(s) * 8`
 * overflows a signed 32-bit ssize_t once a string reaches 2**28 bytes (256 MiB)
 * on an ILP32 build, corrupting every bounds check that compares a bit offset
 * against it. Valid bit indices are confined to the Fixnum range and always fit
 * ssize_t, so only this whole-string bit length needs the wider type: computing
 * it in int64_t keeps the bounds checks correct on 32-bit without changing the
 * public pointer-width bit-index contract (see Discussion.md, "Error behavior
 * for out-of-range bit indices").
 *
 * Porting to Ruby Core:
 *   1. Core String lengths are `long` (RSTRING_LEN), which is pointer-width,
 *      so `RSTRING_LEN(str) * 8` overflows on ILP32 for strings >= 256 MiB
 *      exactly as it does for ssize_t here. Keep the whole-string bit length
 *      in a 64-bit intermediate at every bounds check; do not hold it in a
 *      `long`. Reuse this macro (or an equivalent inline) rather than open-
 *      coding `len * 8`.
 *   2. Keep the public bit-index type pointer-width and keep rejecting
 *      out-of-range positions with ArgumentError (see the cross-reference
 *      above). Only this internal length is widened, so the contract that
 *      core inherits is unchanged.
 *   3. The error-message format specifiers below (<inttypes.h>: (intptr_t)
 *      with PRIdPTR for bit offsets, PRId64 for this widened length) exist
 *      only because this length is wider than the offsets. In core, follow
 *      the local convention for formatting `long` offsets and pick a 64-bit
 *      specifier for the widened length accordingly. */
#define SB_BIT_LEN(byte_len) ((int64_t)(byte_len) * 8)

/* popcount ----------------------------------------------------------------- */
/*
 * Porting to Ruby Core:
 *   1. Remove sb_popcount64 and the #include block below.
 *      Use rb_popcount64 from internal/bits.h instead.
 *   2. Add #include "internal/bits.h" at the top of string.c (or wherever
 *      String#popcount is implemented).
 *   3. Replace all sb_popcount64 calls with rb_popcount64.
 *   4. Move rb_str_popcount into string.c and register it in Init_String().
 */

#if defined(HAVE_X86INTRIN_H)
#  include <x86intrin.h>
#elif defined(_MSC_VER)
#  include <intrin.h>
#endif

/* __has_builtin polyfill: GCC < 10 and other compilers do not define it.
 * Treating it as always-false there causes the fallback paths to be used. */
#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

/* Endianness detection.
 * Prefer Ruby's autoconf-derived WORDS_BIGENDIAN (always available in Ruby
 * extension builds). Falls back to __BYTE_ORDER__ (GCC/Clang) and known LE
 * targets for MSVC. SB_LITTLE_ENDIAN evaluates to 1 only when we can prove
 * the platform is LE; otherwise the portable byte-by-byte path is used. */
#if defined(WORDS_BIGENDIAN)
#  define SB_LITTLE_ENDIAN 0
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define SB_LITTLE_ENDIAN 1
#elif defined(_MSC_VER)
#  define SB_LITTLE_ENDIAN 1   /* MSVC targets (x86/x64/ARM64) are all LE */
#else
#  define SB_LITTLE_ENDIAN 0
#endif

static inline unsigned int
sb_popcount64(uint64_t x)
{
#if defined(_MSC_VER) && defined(__AVX__)
    return (unsigned int)__popcnt64(x);
#elif __has_builtin(__builtin_popcount)
    if (sizeof(long) * CHAR_BIT == 64) {
        return (unsigned int)__builtin_popcountl((unsigned long)x);
    }
    else {
        return (unsigned int)__builtin_popcountll((unsigned long long)x);
    }
#else
    x = (x & 0x5555555555555555) + (x >> 1 & 0x5555555555555555);
    x = (x & 0x3333333333333333) + (x >> 2 & 0x3333333333333333);
    x = (x & 0x0707070707070707) + (x >> 4 & 0x0707070707070707);
    x = (x & 0x000f000f000f000f) + (x >> 8 & 0x000f000f000f000f);
    x = (x & 0x0000001f0000001f) + (x >>16 & 0x0000001f0000001f);
    x = (x & 0x000000000000003f) + (x >>32 & 0x000000000000003f);
    return (unsigned int)x;
#endif
}

/* ctz / clz helpers for set-bit iteration ---------------------------------- */

static ID id_bracket;
static VALUE sym_lsb_first, sym_invert;

enum sb_kw_flag {
    SB_KW_INVERT = 1 << 0,
    SB_KW_LSB_FIRST = 1 << 1
};

static inline int
sb_ctz8(unsigned int x)
{
    /* position of lowest set bit; x must be non-zero */
#if __has_builtin(__builtin_ctz)
    return __builtin_ctz(x);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanForward(&index, x);
    return (int)index;
#else
    int n = 0;
    if (!(x & 0x0F)) { n += 4; x >>= 4; }
    if (!(x & 0x03)) { n += 2; x >>= 2; }
    if (!(x & 0x01))   n += 1;
    return n;
#endif
}

static inline int
sb_highest_bit8(unsigned int x)
{
    /* position of highest set bit; x must be non-zero */
#if __has_builtin(__builtin_clz)
    return 31 - __builtin_clz(x);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse(&index, x);
    return (int)index;
#else
    int n = 0;
    if (x >= 16) { n += 4; x >>= 4; }
    if (x >= 4)  { n += 2; x >>= 2; }
    if (x >= 2)    n += 1;
    return n;
#endif
}

static inline int
sb_ctzll(uint64_t x)
{
    /* position of lowest set bit in a non-zero 64-bit word */
#if __has_builtin(__builtin_ctzll)
    return __builtin_ctzll(x);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, x);
    return (int)index;
#else
    int n = 0;
    if (!(x & 0x00000000FFFFFFFFull)) { n += 32; x >>= 32; }
    if (!(x & 0x0000FFFFull))         { n += 16; x >>= 16; }
    if (!(x & 0x00FFull))             { n +=  8; x >>=  8; }
    if (!(x & 0x0Full))               { n +=  4; x >>=  4; }
    if (!(x & 0x03ull))               { n +=  2; x >>=  2; }
    if (!(x & 0x01ull))                 n +=  1;
    return n;
#endif
}

/* common functions --------------------------------------------------------- */

/*
 * rb_str_get_bit: read one bit from a raw byte sequence.
 *
 * Porting to Ruby Core:
 *   1. Move this declaration to include/ruby/internal/string.h (or
 *      internal/string.h for a non-public internal API).
 *   2. Remove the `static inline` storage class; declare as:
 *        static inline int rb_str_get_bit(const char *ptr, ssize_t bit_index, int lsb_first);
 *      in the header so both string.c and array.c can include it.
 *   3. Replace all call sites in string.c and array.c accordingly.
 *
 * Parameters:
 *   ptr        - pointer to the first byte of the bitmap
 *   bit_index  - flat zero-based position; byte = bit_index/8 from ptr
 *   lsb_first  - non-zero: bit 0 of each byte is the LSB (Arrow/hardware convention)
 *                zero:     bit 0 of each byte is the MSB (byte order preserved)
 *
 * Returns 0 or 1.
 */
static inline int
rb_str_get_bit(const char *ptr, ssize_t bit_index, int lsb_first)
{
    ssize_t byte_index = bit_index / 8;
    int bit_offset = lsb_first ? (bit_index % 8) : (7 - bit_index % 8);
    return (ptr[byte_index] >> bit_offset) & 1;
}

static inline int
test_bit(const char *ptr, ssize_t bit_index)
{
    return rb_str_get_bit(ptr, bit_index, 1);
}

/* Bit position arguments -------------------------------------------------- */
/*
 * Every bit offset in this API -- the single-bit offset of bit_get and friends,
 * the offset of an offset/length pair, and each endpoint of a bit Range --
 * follows one contract, the one accepted into Ruby core:
 *
 *   - the argument goes through rb_to_int, so any object responding to #to_int
 *     is accepted, exactly like the index of String#setbyte;
 *   - a negative offset raises IndexError. Negative positions are not read as
 *     count-from-end, because combining that normalization with the
 *     lsb_first: true/false numbering would make the addressing rule much
 *     harder to state (see Discussion.md);
 *   - any non-negative offset below 2**64 is a well-formed position. One past
 *     the end of the receiver is not an error in itself: it takes the ordinary
 *     out-of-range path of whichever method received it (nil, zero, an empty
 *     iteration, or IndexError for a mutation);
 *   - only an offset that does not fit in uint64_t raises ArgumentError, since
 *     no byte buffer on any platform can be addressed by it.
 *
 * The return type is therefore uint64_t. Callers bounds-check it against
 * SB_BIT_LEN(), and any offset that survives that check is below the bit
 * length of a real string, hence always representable as ssize_t.
 *
 * NUM2SSIZET is width-aware (uses FIX2LL on LLP64, FIX2LONG on LP64) so the
 * FIXNUM extraction does not truncate large FIXNUMs on Windows.
 * RBIGNUM_NEGATIVE_P and rb_absint_size are available via ruby.h.
 *
 * Porting to Ruby Core:
 *   Core keeps a `long` fast path alongside the uint64_t value so that the
 *   common Fixnum case never leaves the pointer-width arithmetic. That split
 *   is a pure optimization; the observable contract is the one above.
 */

/* True when a positive Integer does not fit uint64_t. rb_absint_size returns
 * the byte width of the absolute value, so the bound is exactly 8 bytes. */
static inline int
sb_exceeds_uint64(VALUE integer)
{
    return rb_absint_size(integer, NULL) > sizeof(uint64_t);
}

static uint64_t
sb_bit_offset(VALUE n)
{
    VALUE integer = rb_to_int(n);

    if (FIXNUM_P(integer)) {
        ssize_t value = NUM2SSIZET(integer);
        if (value < 0) {
            rb_raise(rb_eIndexError, "bit index out of range");
        }
        return (uint64_t)value;
    }

    RUBY_ASSERT(RB_TYPE_P(integer, T_BIGNUM));
    if (RBIGNUM_NEGATIVE_P(integer)) {
        rb_raise(rb_eIndexError, "bit index out of range");
    }
    if (sb_exceeds_uint64(integer)) {
        rb_raise(rb_eArgError, "bit index out of representable range");
    }
    return (uint64_t)NUM2ULL(integer);
}

/* Bit lengths share the offset's representable range, but a negative length is
 * an ArgumentError rather than an IndexError: it is a malformed size, not a
 * position outside the string. A length that runs past the end of the receiver
 * is clamped by the caller, matching byteslice. */
static uint64_t
sb_bit_length(VALUE n)
{
    VALUE integer = rb_to_int(n);

    if (FIXNUM_P(integer)) {
        ssize_t value = NUM2SSIZET(integer);
        if (value < 0) {
            rb_raise(rb_eArgError, "bit_length must be non-negative");
        }
        return (uint64_t)value;
    }

    RUBY_ASSERT(RB_TYPE_P(integer, T_BIGNUM));
    if (RBIGNUM_NEGATIVE_P(integer)) {
        rb_raise(rb_eArgError, "bit_length must be non-negative");
    }
    if (sb_exceeds_uint64(integer)) {
        rb_raise(rb_eArgError, "bit_length out of representable range");
    }
    return (uint64_t)NUM2ULL(integer);
}

/* Soft variants for bit_slice, which answers nil for an argument it cannot
 * use instead of raising -- the same leniency String#byteslice has. Only the
 * unrepresentable case still raises, so the boundary between "a position past
 * the end" and "not a position at all" stays where the rest of the API puts
 * it. Returns 0 on success, or -1 for a non-Integer or a negative value. */
static int
sb_bit_position_soft(VALUE n, uint64_t *out)
{
    if (!rb_integer_type_p(n)) return -1;

    if (FIXNUM_P(n)) {
        ssize_t value = NUM2SSIZET(n);
        if (value < 0) return -1;
        *out = (uint64_t)value;
        return 0;
    }

    RUBY_ASSERT(RB_TYPE_P(n, T_BIGNUM));
    if (RBIGNUM_NEGATIVE_P(n)) return -1;
    if (sb_exceeds_uint64(n)) {
        rb_raise(rb_eArgError, "bit index out of representable range");
    }
    *out = (uint64_t)NUM2ULL(n);
    return 0;
}

/* Narrowing a parsed position to the internal index ----------------------- */
/*
 * Positions are parsed in 64 bits but held in a pointer-width signed integer
 * once they address memory, and on an ILP32 build those two widths differ:
 * SB_BIT_LEN can reach 2**34 while ssize_t stops at 2**31-1, so a string of
 * 256 MiB or more has bits that exist but cannot be indexed internally. Such a
 * position gets the same answer as one that does not fit 64 bits at all --
 * ArgumentError -- rather than being silently truncated into a valid-looking
 * index. On LP64 the gap is empty and none of this is reachable: no String is
 * large enough for its bit length to exceed SSIZE_MAX.
 */
#ifndef SSIZE_MAX
#  define SSIZE_MAX ((ssize_t)(((size_t)-1) >> 1))
#endif
#define SB_MAX_BIT_POS ((uint64_t)SSIZE_MAX)

/* The receiver's bit length as an internal index, saturated at ssize_t. */
static inline ssize_t
sb_addressable_bits(int64_t total)
{
    return (ssize_t)(((uint64_t)total > SB_MAX_BIT_POS) ? SB_MAX_BIT_POS : (uint64_t)total);
}

/* Narrow a parsed position for the methods whose answer at or past the end is
 * "there is nothing here" (an empty iteration, a count of zero, an empty
 * slice): those saturate at the end of the string rather than failing. */
static ssize_t
sb_narrow_bit_pos(uint64_t pos, int64_t total)
{
    if (pos >= (uint64_t)total) return sb_addressable_bits(total);
    if (pos > SB_MAX_BIT_POS) {
        rb_raise(rb_eArgError, "bit index out of representable range");
    }
    return (ssize_t)pos;
}

/* Clamp a parsed length to the bits still addressable after `beg`. */
static ssize_t
sb_clamp_bit_length(uint64_t len, ssize_t beg, int64_t total)
{
    uint64_t available = (uint64_t)sb_addressable_bits(total) - (uint64_t)beg;
    return (ssize_t)(len > available ? available : len);
}

/* Resolve a single-bit offset against the receiver's bit length.
 * Returns -1 when the offset is past the end of the string. */
static ssize_t
sb_resolve_bit_offset(VALUE self, VALUE n)
{
    int64_t total = SB_BIT_LEN(RSTRING_LEN(self));
    uint64_t offset = sb_bit_offset(n);
    if ((uint64_t)total <= offset) return -1;
    if (offset > SB_MAX_BIT_POS) {
        rb_raise(rb_eArgError, "bit index out of representable range");
    }
    return (ssize_t)offset;
}

/* Bit numbering between byte-with-LSB-as-bit-0 and byte-with-MSB-as-bit-0
 * is an involution: swapping in either direction uses the same formula
 * `(x & ~7) | (7 - (x & 7))`. logical_to_physical is therefore symmetric and
 * is reused on the return path (physical -> logical) as well. */
static inline ssize_t
logical_to_physical(ssize_t logical, int lsb_first)
{
    return lsb_first ? logical : ((logical & ~7L) | (7 - (logical & 7L)));
}

static inline int
logical_get_bit(const unsigned char *ptr, ssize_t logical_index, int lsb_first)
{
    return test_bit((const char *)ptr, logical_to_physical(logical_index, lsb_first));
}

static inline void
physical_write_bit(unsigned char *ptr, ssize_t bit_index, int bit)
{
    unsigned char mask = (unsigned char)(1u << (bit_index & 7));
    if (bit) ptr[bit_index >> 3] |= mask;
    else     ptr[bit_index >> 3] &= (unsigned char)~mask;
}

static inline void
logical_write_bit(unsigned char *ptr, ssize_t logical_index, int lsb_first, int bit)
{
    physical_write_bit(ptr, logical_to_physical(logical_index, lsb_first), bit);
}

static ssize_t
check_bit_index(VALUE self, VALUE n, int lsb_first)
{
    ssize_t idx = sb_resolve_bit_offset(self, n);
    if (idx < 0) {
        rb_raise(rb_eIndexError, "bit index out of range");
    }
    return logical_to_physical(idx, lsb_first);
}

/* Resolve a bit Range against the receiver's bit length.
 *
 * This replaces rb_range_beg_len() rather than wrapping it, for two reasons.
 * rb_range_beg_len() takes `long *` endpoints, so on Windows (LLP64), where
 * long is 32-bit while ssize_t is 64-bit, its interface does not match the
 * pointer-width bit index used here. More importantly, it raises RangeError
 * for any endpoint that does not fit `long`, whereas a bit Range endpoint is
 * allowed to be any position up to UINT64_MAX (see sb_bit_offset); such an
 * endpoint simply lies past the end of the string.
 *
 * The resolution rules match rb_range_beg_len() with err=0, minus the
 * count-from-end normalization that this API does not have:
 *   - a nil begin is 0, a nil end runs to the end of the string;
 *   - an end past the end of the string is clamped to it;
 *   - a begin past the end of the string reports SB_RANGE_OUT, and each caller
 *     decides what that means (0 bits counted, a nil slice, an IndexError).
 * Like rb_range_beg_len(), the returned length may exceed the bits actually
 * available when the end was clamped, so callers that write must re-check
 * beg + len against the total (see rb_str_mutate_bits and rb_str_bit_splice).
 *
 * Porting to Ruby Core:
 *   Replace rb_range_values() with direct struct access:
 *     #include "internal/range.h"
 *     beg  = RANGE_BEG(range);
 *     end  = RANGE_END(range);
 *     excl = RANGE_EXCL(range);
 */
enum sb_range_result {
    SB_RANGE_OUT = 0,   /* the range begins past the end of the string */
    SB_RANGE_OK  = 1
};

static enum sb_range_result
sb_bit_range_beg_len(VALUE range, ssize_t *begp, ssize_t *lenp, int64_t total)
{
    VALUE beg_v, end_v;
    int excl;
    rb_range_values(range, &beg_v, &end_v, &excl);

    uint64_t total_u = (uint64_t)total;
    uint64_t beg = NIL_P(beg_v) ? 0 : sb_bit_offset(beg_v);
    if (beg > total_u) return SB_RANGE_OUT;

    uint64_t end_exclusive;
    if (NIL_P(end_v)) {
        end_exclusive = total_u;
    }
    else {
        uint64_t end = sb_bit_offset(end_v);
        if (end > total_u) end = total_u;
        end_exclusive = excl ? end : end + 1;
    }

    /* The length is deliberately left unclamped: when the end was clamped it
     * can exceed the bits available, which is exactly what lets a writing
     * caller detect the overrun. Reading callers clamp it themselves. */
    uint64_t len = (end_exclusive > beg) ? end_exclusive - beg : 0;
    *begp = sb_narrow_bit_pos(beg, total);
    *lenp = (ssize_t)(len > SB_MAX_BIT_POS ? SB_MAX_BIT_POS : len);
    return SB_RANGE_OK;
}

static void
validate_option_hash(VALUE opts, unsigned allowed)
{
    if (NIL_P(opts)) return;
    Check_Type(opts, T_HASH);

    VALUE keys = rb_funcall(opts, rb_intern("keys"), 0);
    ssize_t len = RARRAY_LEN(keys);

    for (ssize_t i = 0; i < len; i++) {
        VALUE key = RARRAY_AREF(keys, i);
        if (((allowed & SB_KW_LSB_FIRST) && key == sym_lsb_first) ||
            ((allowed & SB_KW_INVERT) && key == sym_invert)) {
            continue;
        }

        rb_raise(rb_eArgError, "unknown keyword: %"PRIsVALUE, rb_inspect(key));
    }
}

/* An absent keyword falls back to the default, but an explicitly passed value
 * must be true or false: `lsb_first: nil` is a caller mistake, not a request
 * for the default, and silently treating it as LSB-first would flip the bit
 * numbering of a getter and its matching setter apart. */
static int
parse_bool_opt(VALUE opts, VALUE key, const char *name, int default_value)
{
    if (NIL_P(opts)) return default_value;
    VALUE value = rb_hash_lookup2(opts, key, Qundef);
    if (value == Qundef) return default_value;
    if (value == Qtrue) return 1;
    if (value == Qfalse) return 0;
    rb_raise(rb_eArgError, "%s must be true or false", name);
    UNREACHABLE_RETURN(default_value);
}

static int
parse_lsb_first_opt(VALUE opts)
{
    return parse_bool_opt(opts, sym_lsb_first, "lsb_first", 1);
}

/* Parse the optional start_offset positional argument of the iterators
 * (Qnil => 0). A start past the end of the receiver is clamped to its bit
 * length, where every emitter already stops immediately. */
static ssize_t
parse_start_offset(VALUE self, VALUE v)
{
    if (NIL_P(v)) return 0;
    return sb_narrow_bit_pos(sb_bit_offset(v), SB_BIT_LEN(RSTRING_LEN(self)));
}

/* read -------------------------------------------------------------------- */

/* Shared body of bit_get and bit_set?.
 * Returns the bit value (0 or 1), or -1 when the offset is out of range. */
static int
str_bit_read(int argc, VALUE *argv, VALUE self)
{
    VALUE bit_offset_v, opts;
    rb_scan_args(argc, argv, "1:", &bit_offset_v, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);

    ssize_t bit_offset = sb_resolve_bit_offset(self, bit_offset_v);
    if (bit_offset < 0) return -1;

    return test_bit(RSTRING_PTR(self), logical_to_physical(bit_offset, lsb_first));
}

/* Return 1/0/nil for the bit at flat position n. */
static VALUE
rb_str_bit_get(int argc, VALUE *argv, VALUE self)
{
    int bit = str_bit_read(argc, argv, self);
    return bit < 0 ? Qnil : INT2FIX(bit);
}

/* Return true/false/nil for the bit at flat position n. */
static VALUE
rb_str_bit_set_p(int argc, VALUE *argv, VALUE self)
{
    int bit = str_bit_read(argc, argv, self);
    if (bit < 0) return Qnil;
    return bit ? Qtrue : Qfalse;
}

/* count_set_bits: popcount over a raw byte buffer.
 *
 * Uses a 32-byte (4 x uint64_t) unrolled inner loop, falls back to 8-byte
 * steps, and finally collects the partial trailing bytes into a single
 * uint64_t for one more popcount. memcpy avoids unaligned-load issues on
 * strict-alignment platforms (SPARC, MIPS); modern compilers fold the 8-byte
 * memcpy into a single load on platforms that allow unaligned access. */
static ssize_t
count_set_bits(const unsigned char *str, ssize_t len)
{
    ssize_t count = 0;
    ssize_t off = 0;
    ssize_t unrolled_end = len & ~31L;
    ssize_t aligned_end  = len & ~7L;

    for (; off < unrolled_end; off += 32) {
        uint64_t w0, w1, w2, w3;
        memcpy(&w0, str + off,      8);
        memcpy(&w1, str + off + 8,  8);
        memcpy(&w2, str + off + 16, 8);
        memcpy(&w3, str + off + 24, 8);
        count += sb_popcount64(w0);
        count += sb_popcount64(w1);
        count += sb_popcount64(w2);
        count += sb_popcount64(w3);
    }

    for (; off < aligned_end; off += 8) {
        uint64_t w;
        memcpy(&w, str + off, 8);
        count += sb_popcount64(w);
    }

    ssize_t remainder = len - aligned_end;
    if (remainder > 0) {
        uint64_t last = 0;
        const unsigned char *tail = str + aligned_end;
        for (ssize_t i = 0; i < remainder; i++) {
            last |= (uint64_t)tail[i] << (i * 8);
        }
        count += sb_popcount64(last);
    }

    return count;
}

/* count_set_bits_range: popcount over [start, start+length) in LSB-first numbering.
 * Handles non-byte-aligned start and length by masking partial first/last bytes. */
static ssize_t
count_set_bits_range(const unsigned char *str, ssize_t total_bytes,
                     ssize_t start, ssize_t length)
{
    if (length <= 0) return 0;
    int64_t total_bits = SB_BIT_LEN(total_bytes);
    if (start >= total_bits) return 0;
    if (start + length > total_bits) length = (ssize_t)(total_bits - start);

    ssize_t byte_start = start >> 3;
    int bit_lo = (int)(start & 7);
    ssize_t end_bit = start + length;
    ssize_t last_byte = (end_bit - 1) >> 3;
    int e_bit = (int)(end_bit & 7);  /* bits to use in last byte; 0 means full byte */

    if (byte_start == last_byte) {
        unsigned int b = (unsigned int)str[byte_start] >> bit_lo;
        b &= (1u << (unsigned)length) - 1u;
        return (ssize_t)sb_popcount64(b);
    }

    ssize_t count = 0;
    if (bit_lo != 0) {
        count += sb_popcount64((unsigned int)str[byte_start] >> bit_lo);
        byte_start++;
    }
    ssize_t full_last = (e_bit == 0) ? last_byte + 1 : last_byte;
    count += count_set_bits(str + byte_start, full_last - byte_start);
    if (e_bit != 0) {
        unsigned int b = (unsigned int)str[last_byte] & ((1u << (unsigned)e_bit) - 1u);
        count += sb_popcount64(b);
    }
    return count;
}

/* count_set_bits_range_msb: same as count_set_bits_range but for MSB-first numbering.
 * In MSB-first, position 0 within a byte is physical bit 7 (the MSB). */
static ssize_t
count_set_bits_range_msb(const unsigned char *str, ssize_t total_bytes,
                          ssize_t start, ssize_t length)
{
    if (length <= 0) return 0;
    int64_t total_bits = SB_BIT_LEN(total_bytes);
    if (start >= total_bits) return 0;
    if (start + length > total_bits) length = (ssize_t)(total_bits - start);

    ssize_t byte_start = start >> 3;
    int s_bit = (int)(start & 7);      /* MSB-first within-byte start index */
    ssize_t end_bit = start + length;
    ssize_t last_byte = (end_bit - 1) >> 3;
    int e_bit = (int)(end_bit & 7);    /* bits to use in last byte; 0 means full byte */

    if (byte_start == last_byte) {
        /* physical bits (7-s_bit) down to (7-s_bit-length+1) */
        unsigned int b = (unsigned int)str[byte_start] >> (unsigned)(8 - s_bit - (int)length);
        b &= (1u << (unsigned)length) - 1u;
        return (ssize_t)sb_popcount64(b);
    }

    ssize_t count = 0;
    /* partial first byte: MSB-first positions s_bit..7 = physical bits 0..(7-s_bit) */
    if (s_bit != 0) {
        unsigned int b = (unsigned int)str[byte_start] & ((1u << (unsigned)(8 - s_bit)) - 1u);
        count += sb_popcount64(b);
        byte_start++;
    }
    ssize_t full_last = (e_bit == 0) ? last_byte + 1 : last_byte;
    count += count_set_bits(str + byte_start, full_last - byte_start);
    /* partial last byte: MSB-first positions 0..(e_bit-1) = physical bits (8-e_bit)..7 */
    if (e_bit != 0) {
        unsigned int b = (unsigned int)str[last_byte] >> (unsigned)(8 - e_bit);
        count += sb_popcount64(b);
    }
    return count;
}

static VALUE
rb_str_bit_count(int argc, VALUE *argv, VALUE self)
{
    const unsigned char *str = (const unsigned char *)RSTRING_PTR(self);
    ssize_t src_len = RSTRING_LEN(self);

    VALUE v0 = Qnil, v1 = Qnil, opts = Qnil;
    rb_scan_args(argc, argv, "02:", &v0, &v1, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);

    /* No positional args: count the whole string; lsb_first: is ignored (order-independent) */
    if (NIL_P(v0))
        return SSIZET2NUM(count_set_bits(str, src_len));

    int lsb_first = parse_lsb_first_opt(opts);
    int64_t total_bits = SB_BIT_LEN(src_len);
    ssize_t bit_offset, bit_length;

    if (rb_obj_is_kind_of(v0, rb_cRange)) {
        if (!NIL_P(v1))
            rb_raise(rb_eArgError, "wrong number of arguments");
        ssize_t beg, len;
        if (sb_bit_range_beg_len(v0, &beg, &len, total_bits) == SB_RANGE_OUT)
            return INT2FIX(0);
        bit_offset = beg;
        bit_length = len;
    }
    else if (!NIL_P(v1)) {
        uint64_t off = sb_bit_offset(v0);
        uint64_t len = sb_bit_length(v1);
        /* A region starting at or past the end holds no bits, and one that
         * runs off the end contributes only the part that exists. */
        if (off >= (uint64_t)total_bits) return INT2FIX(0);
        bit_offset = sb_narrow_bit_pos(off, total_bits);
        bit_length = sb_clamp_bit_length(len, bit_offset, total_bits);
    }
    else {
        rb_raise(rb_eArgError,
                 "wrong number of arguments (given 1, expected 0, 1 Range, or 2)");
    }

    if (lsb_first)
        return SSIZET2NUM(count_set_bits_range(str, src_len, bit_offset, bit_length));
    else
        return SSIZET2NUM(count_set_bits_range_msb(str, src_len, bit_offset, bit_length));
}

/* iterate bits ------------------------------------------------------------ */

/* Unified emitter for each_bit / bits.
 *
 * Yields (when ary == Qnil) or pushes to a pre-allocated Array. lsb_first is
 * hoisted outside the byte loop so the inner walk direction is straight-line
 * code, removing a per-byte branch.
 */
static void
emit_bits(const unsigned char *str, ssize_t len, int lsb_first, ssize_t start_offset, VALUE ary)
{
    if (start_offset >= SB_BIT_LEN(len)) return;

#define SB_EMIT(v) \
    do { VALUE _b = (v); \
         if (ary == Qnil) rb_yield(_b); else rb_ary_push(ary, _b); } while (0)

    ssize_t byte_start = start_offset >> 3;
    int bit_start = (int)(start_offset & 7);

    if (lsb_first) {
        for (ssize_t i = byte_start; i < len; i++) {
            unsigned char b = str[i];
            int j_start = (i == byte_start) ? bit_start : 0;
            for (int j = j_start; j < 8; j++) {
                SB_EMIT(INT2FIX((b >> j) & 1));
            }
        }
    } else {
        for (ssize_t i = byte_start; i < len; i++) {
            unsigned char b = str[i];
            int j_end = (i == byte_start) ? (7 - bit_start) : 7;
            for (int j = j_end; j >= 0; j--) {
                SB_EMIT(INT2FIX((b >> j) & 1));
            }
        }
    }

#undef SB_EMIT
}

static VALUE
rb_str_each_bit(int argc, VALUE *argv, VALUE self)
{
    RETURN_ENUMERATOR(self, argc, argv);

    VALUE start_offset_v = Qnil, opts = Qnil;
    rb_scan_args(argc, argv, "01:", &start_offset_v, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);
    ssize_t start_offset = parse_start_offset(self, start_offset_v);

    emit_bits((const unsigned char *)RSTRING_PTR(self), RSTRING_LEN(self),
              lsb_first, start_offset, Qnil);
    return self;
}

static VALUE
rb_str_bits(int argc, VALUE *argv, VALUE self)
{
    VALUE start_offset_v = Qnil, opts = Qnil;
    rb_scan_args(argc, argv, "01:", &start_offset_v, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);
    ssize_t start_offset = parse_start_offset(self, start_offset_v);
    ssize_t len = RSTRING_LEN(self);
    const unsigned char *str = (const unsigned char *)RSTRING_PTR(self);

    if (rb_block_given_p()) {
        emit_bits(str, len, lsb_first, start_offset, Qnil);
        return self;
    }

    int64_t total_bits = SB_BIT_LEN(len);
    ssize_t nbits = (start_offset >= total_bits) ? 0 : (ssize_t)(total_bits - start_offset);
    VALUE ary = rb_ary_new_capa(nbits);
    emit_bits(str, len, lsb_first, start_offset, ary);
    return ary;
}

/* iterate bit positions matching `bit` ------------------------------------ */

/* parse the required `bit` argument (0/1/true/false) to 0 or 1 */
static int
parse_bit_target(VALUE bit_val)
{
    if (bit_val == Qtrue || bit_val == INT2FIX(1)) return 1;
    if (bit_val == Qfalse || bit_val == INT2FIX(0)) return 0;
    rb_raise(rb_eArgError, "bit must be 0, 1, false, or true");
    UNREACHABLE_RETURN(0);
}

/* Unified scanner for each_bit_offset / bit_offsets.
 *
 * Emit each bit position equal to `target` either by yielding to the block
 * (when ary == Qnil) or by pushing to the pre-allocated Array. Both call
 * paths share the same hot loops; the only per-emit cost is one branch on
 * (ary == Qnil), which the compiler can lift out of the inner while loop.
 *
 * LSB-first path: on little-endian, an 8-byte memcpy preserves the flat
 * LSB-first bit numbering (word bit 0 == position 0), so we can scan 64 bits
 * per ctzll. For target=0, invert the loaded word/byte; all 8/64 bits of the
 * inverted unit are valid positions since each byte contributes exactly 8.
 *
 * MSB-first path: walk byte-by-byte with sb_highest_bit8, mapping each
 * physical (LSB-first) bit position into the MSB-first count via
 * logical_to_physical (the operation is its own inverse).
 */
static void
emit_bit_offsets(const unsigned char *str, ssize_t len, int target, int lsb_first,
                 ssize_t start_offset, VALUE ary)
{
    if (start_offset >= SB_BIT_LEN(len)) return;

#define SB_EMIT(pos_val) \
    do { VALUE _p = (pos_val); \
         if (ary == Qnil) rb_yield(_p); else rb_ary_push(ary, _p); } while (0)

    ssize_t byte_start = start_offset >> 3;
    int bit_lo = (int)(start_offset & 7);

    if (lsb_first) {
        /* Handle the partial first byte before aligning to byte boundary */
        if (bit_lo != 0) {
            unsigned int b = str[byte_start];
            if (target == 0) b = (~b) & 0xFF;
            b >>= bit_lo;
            while (b != 0) {
                int bit = sb_ctz8(b);
                SB_EMIT(SSIZET2NUM(byte_start * 8 + bit_lo + bit));
                b &= b - 1;
            }
            byte_start++;
        }
#if SB_LITTLE_ENDIAN
        ssize_t n_words = (len - byte_start) >> 3;
        for (ssize_t wi = 0; wi < n_words; wi++) {
            uint64_t w;
            memcpy(&w, str + byte_start + wi * 8, 8);
            if (target == 0) w = ~w;
            while (w != 0) {
                int bit = sb_ctzll(w);
                SB_EMIT(SSIZET2NUM((byte_start + wi * 8) * 8 + bit));
                w &= w - 1;
            }
        }
        for (ssize_t bi = byte_start + (n_words << 3); bi < len; bi++) {
            unsigned int b = str[bi];
            if (target == 0) b = (~b) & 0xFF;
            while (b != 0) {
                int bit = sb_ctz8(b);
                SB_EMIT(SSIZET2NUM(bi * 8 + bit));
                b &= b - 1;
            }
        }
#else
        for (ssize_t bi = byte_start; bi < len; bi++) {
            unsigned int b = str[bi];
            if (target == 0) b = (~b) & 0xFF;
            while (b != 0) {
                int bit = sb_ctz8(b);
                SB_EMIT(SSIZET2NUM(bi * 8 + bit));
                b &= b - 1;
            }
        }
#endif
    }
    else {
        /* lsb_first: false => byte order preserved, bits 7..0 map to logical 0..7.
         * In the first (possibly partial) byte, skip the top bit_lo bits. */
        for (ssize_t bi = byte_start; bi < len; bi++) {
            unsigned int b = str[bi];
            if (target == 0) b = (~b) & 0xFF;
            if (bi == byte_start && bit_lo != 0)
                b &= (1u << (8 - bit_lo)) - 1;  /* clear top bit_lo bits */
            while (b != 0) {
                int bit = sb_highest_bit8(b);
                ssize_t physical = bi * 8 + bit;
                SB_EMIT(SSIZET2NUM(logical_to_physical(physical, 0)));
                b ^= (1u << bit);
            }
        }
    }

#undef SB_EMIT
}

static VALUE
rb_str_each_bit_offset(int argc, VALUE *argv, VALUE self)
{
    RETURN_ENUMERATOR(self, argc, argv);

    VALUE bit_val, start_offset_v = Qnil, opts = Qnil;
    rb_scan_args(argc, argv, "11:", &bit_val, &start_offset_v, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);
    int target    = parse_bit_target(bit_val);
    ssize_t start_offset = parse_start_offset(self, start_offset_v);

    emit_bit_offsets((const unsigned char *)RSTRING_PTR(self), RSTRING_LEN(self),
                     target, lsb_first, start_offset, Qnil);
    return self;
}

static VALUE
rb_str_bit_offsets(int argc, VALUE *argv, VALUE self)
{
    VALUE bit_val, start_offset_v = Qnil, opts = Qnil;
    rb_scan_args(argc, argv, "11:", &bit_val, &start_offset_v, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);
    int target    = parse_bit_target(bit_val);
    ssize_t start_offset = parse_start_offset(self, start_offset_v);

    ssize_t len = RSTRING_LEN(self);
    const unsigned char *str = (const unsigned char *)RSTRING_PTR(self);

    if (rb_block_given_p()) {
        emit_bit_offsets(str, len, target, lsb_first, start_offset, Qnil);
        return self;
    }

    /* Pre-size the Array using popcount to avoid repeated reallocation.
     * For target=0 the expected count is (len * 8 - popcount). */
    ssize_t set_count = count_set_bits(str, len);
    ssize_t count     = (target == 1) ? set_count : (ssize_t)(SB_BIT_LEN(len) - set_count);
    VALUE ary = rb_ary_new_capa(count);
    emit_bit_offsets(str, len, target, lsb_first, start_offset, ary);
    return ary;
}

/* multi-bit mutation ------------------------------------------------------ */

/*
 * bit_copy_core: copy `length` bits from src[src_bit_off] to dst[dst_bit_off].
 *
 * Both offsets are in the LSB-first flat bit numbering used throughout
 * string_bits.  The routine does not resize dst; the caller must ensure
 * dst has enough bytes.
 *
 * Algorithm:
 *   1. Extract the src bits into a small aligned tmp buffer (identical to the
 *      bit_slice read path).
 *   2. Write tmp into dst with shift/mask merge (handles the unaligned case).
 *
 * Porting to Ruby Core:
 *   1. Move alongside bit_slice in string.c.
 *   2. Share the extract loop with rb_str_bit_slice via ebs_extract.
 *   3. Remove `static`.
 */
static void
bit_copy_core(unsigned char *dst, ssize_t dst_bit_off,
              const unsigned char *src, ssize_t src_len_bytes,
              ssize_t src_bit_off, ssize_t length)
{
    if (length == 0) return;
    ssize_t out_bytes = (length + 7) >> 3;

    unsigned char  stack_tmp[256];
    unsigned char *tmp = (out_bytes <= (ssize_t)sizeof(stack_tmp))
                         ? stack_tmp
                         : (unsigned char *)ruby_xmalloc(out_bytes);

    /* Step 1: extract src bits into tmp (aligned, zero-padded tail) */
    {
        ssize_t src_byte_off = src_bit_off >> 3;
        int  src_shift    = (int)(src_bit_off & 7);
        if (src_shift == 0) {
            memcpy(tmp, src + src_byte_off, out_bytes);
        }
        else {
            int anti = 8 - src_shift;
            for (ssize_t i = 0; i < out_bytes; i++) {
                unsigned char lo = src[src_byte_off + i];
                unsigned char hi = (src_byte_off + i + 1 < src_len_bytes)
                                   ? src[src_byte_off + i + 1] : 0;
                tmp[i] = (unsigned char)((lo >> src_shift) | (hi << anti));
            }
        }
        int tail = (int)(length & 7);
        if (tail) tmp[out_bytes - 1] &= (unsigned char)((1u << tail) - 1);
    }

    /* Step 2: write aligned tmp into dst at dst_bit_off */
    {
        ssize_t dst_byte_off = dst_bit_off >> 3;
        int  dst_shift    = (int)(dst_bit_off & 7);

        if (dst_shift == 0) {
            ssize_t full = length >> 3;
            int  tail = (int)(length & 7);
            memcpy(dst + dst_byte_off, tmp, full);
            if (tail) {
                unsigned char mask = (unsigned char)((1u << tail) - 1);
                dst[dst_byte_off + full] =
                    (dst[dst_byte_off + full] & (unsigned char)~mask)
                    | (tmp[full] & mask);
            }
        }
        else {
            int  anti  = 8 - dst_shift;
            ssize_t n_dst = ((dst_bit_off + length - 1) >> 3) - dst_byte_off + 1;

            for (ssize_t i = 0; i < n_dst; i++) {
                ssize_t byte_base = (dst_byte_off + i) * 8;
                ssize_t wstart = dst_bit_off > byte_base ? dst_bit_off - byte_base : 0;
                ssize_t wend   = (dst_bit_off + length - 1 < byte_base + 7)
                              ? dst_bit_off + length - 1 - byte_base : 7;
                unsigned char wmask =
                    (unsigned char)(((1u << (wend + 1)) - 1) ^ ((1u << wstart) - 1));

                /* lo_t: high bits of the previous tmp byte spill into this dst byte */
                unsigned char lo_t = (i > 0 && i - 1 < out_bytes) ? tmp[i - 1] : 0;
                /* hi_t: low bits of the current tmp byte fill the upper part */
                unsigned char hi_t = (i < out_bytes) ? tmp[i] : 0;
                unsigned char nv = (unsigned char)((lo_t >> anti) | (hi_t << dst_shift));
                dst[dst_byte_off + i] =
                    (dst[dst_byte_off + i] & (unsigned char)~wmask) | (nv & wmask);
            }
        }
    }

    if (tmp != stack_tmp) ruby_xfree(tmp);
}

/* Extract a sub-sequence of bits into a new String. */
static VALUE
rb_str_bit_slice(int argc, VALUE *argv, VALUE self)
{
    ssize_t src_len = RSTRING_LEN(self);
    int64_t total_bits = SB_BIT_LEN(src_len);
    ssize_t bit_offset, bit_length;
    VALUE v0, v1, opts;
    int n_pos = rb_scan_args(argc, argv, "11:", &v0, &v1, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);

    if (n_pos == 1 && rb_obj_is_kind_of(v0, rb_cRange)) {
        ssize_t beg, len;
        if (sb_bit_range_beg_len(v0, &beg, &len, total_bits) == SB_RANGE_OUT) {
            return Qnil;
        }
        bit_offset = beg;
        bit_length = len;
    }
    else if (n_pos == 2) {
        uint64_t off, len;
        if (sb_bit_position_soft(v0, &off) < 0) return Qnil;
        if (sb_bit_position_soft(v1, &len) < 0) return Qnil;
        if (off > (uint64_t)total_bits) return Qnil;

        bit_offset = sb_narrow_bit_pos(off, total_bits);
        bit_length = sb_clamp_bit_length(len, bit_offset, total_bits);
    }
    else if (n_pos == 1) {
        return Qnil;
    }
    else {
        rb_raise(rb_eArgError,
                 "wrong number of arguments (given %d, expected 1 or 2)", n_pos);
    }

    if (bit_offset > total_bits) return Qnil;
    int64_t available = total_bits - bit_offset;
    if (bit_length > available) bit_length = (ssize_t)available;

    if (bit_length == 0) return rb_str_new("", 0);

    ssize_t out_bytes = (bit_length + 7) / 8;
    VALUE result = rb_str_buf_new(out_bytes);
    rb_str_resize(result, out_bytes);
    /* A bit-aligned slice is a repacked byte sequence, not a substring, so the
     * result carries BINARY like every other String returned by this API. */
    rb_enc_associate(result, rb_ascii8bit_encoding());
    ENC_CODERANGE_CLEAR(result);
    unsigned char *dst = (unsigned char *)RSTRING_PTR(result);
    const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);

    memset(dst, 0, out_bytes);

    if (lsb_first) {
        bit_copy_core(dst, 0, src, src_len, bit_offset, bit_length);
    } else {
        ssize_t dst_bit = 0;
        ssize_t start_byte = bit_offset >> 3;
        ssize_t end_byte = (bit_offset + bit_length - 1) >> 3;

        for (ssize_t b = start_byte; b <= end_byte; b++) {
            ssize_t b_start_l = b << 3;
            ssize_t b_end_l = b_start_l + 7;
            ssize_t l_min = (bit_offset > b_start_l) ? bit_offset : b_start_l;
            ssize_t l_max = ((bit_offset + bit_length - 1) < b_end_l) ? (bit_offset + bit_length - 1) : b_end_l;

            ssize_t p_min = b_start_l + (7 - (l_max & 7L));
            ssize_t p_max = b_start_l + (7 - (l_min & 7L));
            ssize_t chunk_len = p_max - p_min + 1;

            bit_copy_core(dst, dst_bit, src, src_len, p_min, chunk_len);
            dst_bit += chunk_len;
        }
    }
    return result;
}

/* single-bit mutation ----------------------------------------------------- */

enum sb_mutation_op {
    SB_MUT_SET = 1,
    SB_MUT_CLEAR = 2,
    SB_MUT_FLIP = 3
};

static VALUE
rb_str_mutate_bits(int argc, VALUE *argv, VALUE self, enum sb_mutation_op op)
{
    VALUE target, bit_length_v = Qnil, opts = Qnil;
    rb_scan_args(argc, argv, "11:", &target, &bit_length_v, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);

    unsigned char *ptr;

    /* Single-bit form: bit_set(n). The offset is validated before
     * rb_str_modify, so an out-of-range offset raises IndexError even on a
     * frozen receiver, matching the order of the core implementation. */
    if (NIL_P(bit_length_v) && !rb_obj_is_kind_of(target, rb_cRange)) {
        ssize_t idx = check_bit_index(self, target, lsb_first);
        rb_str_modify(self);
        ptr = (unsigned char *)RSTRING_PTR(self);
        unsigned char mask = (unsigned char)(1u << (idx % 8));
        switch (op) {
          case SB_MUT_SET:   ptr[idx / 8] |= mask; break;
          case SB_MUT_CLEAR: ptr[idx / 8] &= (unsigned char)~mask; break;
          case SB_MUT_FLIP:  ptr[idx / 8] ^= mask; break;
        }
        return self;
    }

    int64_t total_bits = SB_BIT_LEN(RSTRING_LEN(self));
    ssize_t bit_offset, bit_length;

    if (rb_obj_is_kind_of(target, rb_cRange)) {
        if (!NIL_P(bit_length_v))
            rb_raise(rb_eArgError, "wrong number of arguments");

        /* A range that begins past the end has no bits to write, which is an
         * error for a mutation, not a silent no-op. */
        if (sb_bit_range_beg_len(target, &bit_offset, &bit_length, total_bits) == SB_RANGE_OUT)
            rb_raise(rb_eIndexError, "bit range out of range");

        /* sb_bit_range_beg_len clamps the end, so a range that overruns the
         * string comes back longer than the bits available. Writing is not
         * allowed to silently shrink, so report the overrun instead. */
        if ((int64_t)bit_offset + (int64_t)bit_length > total_bits)
            rb_raise(rb_eIndexError, "bit range out of range");
    }
    else {
        /* 2-arg form: bit_set(bit_offset, bit_length) */
        uint64_t off = sb_bit_offset(target);
        uint64_t len = sb_bit_length(bit_length_v);
        if (len == 0) return self;
        if (off >= (uint64_t)total_bits || len > (uint64_t)total_bits - off)
            rb_raise(rb_eIndexError, "bit range out of range");
        bit_offset = sb_narrow_bit_pos(off, total_bits);
        if (len > (uint64_t)(sb_addressable_bits(total_bits) - bit_offset))
            rb_raise(rb_eArgError, "bit index out of representable range");
        bit_length = (ssize_t)len;
    }

    rb_str_modify(self);
    ptr = (unsigned char *)RSTRING_PTR(self);

    for (ssize_t logical = bit_offset; logical < bit_offset + bit_length; logical++) {
        ssize_t idx = logical_to_physical(logical, lsb_first);
        unsigned char mask = (unsigned char)(1u << (idx % 8));
        switch (op) {
          case SB_MUT_SET:   ptr[idx / 8] |= mask; break;
          case SB_MUT_CLEAR: ptr[idx / 8] &= (unsigned char)~mask; break;
          case SB_MUT_FLIP:  ptr[idx / 8] ^= mask; break;
        }
    }
    return self;
}

static VALUE
rb_str_bit_set(int argc, VALUE *argv, VALUE self)
{
    return rb_str_mutate_bits(argc, argv, self, SB_MUT_SET);
}

static VALUE
rb_str_bit_clear(int argc, VALUE *argv, VALUE self)
{
    return rb_str_mutate_bits(argc, argv, self, SB_MUT_CLEAR);
}

static VALUE
rb_str_bit_flip(int argc, VALUE *argv, VALUE self)
{
    return rb_str_mutate_bits(argc, argv, self, SB_MUT_FLIP);
}

/* bulk bitwise ------------------------------------------------------------ */

static void
check_binary_op_lengths(VALUE self, VALUE other)
{
    if (RSTRING_LEN(self) != RSTRING_LEN(other)) {
        rb_raise(rb_eArgError, "operands must have the same length (%ld vs %ld)",
                 RSTRING_LEN(self), RSTRING_LEN(other));
    }
}

/* Allocate the destination buffer of a non-destructive bitwise operation.
 *
 * Bit operations read the receiver as a raw byte sequence, so the bytes they
 * produce need not be valid in the receiver's encoding; the result is always
 * BINARY. The destructive (`!`) variants leave the receiver's encoding alone
 * instead, which is what String#setbyte does for the same reason. */
static VALUE
alloc_result(VALUE self)
{
    ssize_t len = RSTRING_LEN(self);
    VALUE result = rb_str_buf_new(len);
    rb_str_resize(result, len);
    rb_enc_associate(result, rb_ascii8bit_encoding());
    ENC_CODERANGE_CLEAR(result);
    return result;
}

/*
 * Bitwise op kernels: process 32 bytes (4 x uint64_t) per loop iteration via
 * memcpy + word-wise op + memcpy, then any 8-byte tail, then byte-by-byte for
 * the final < 8 bytes. memcpy avoids unaligned-load/store issues on strict-
 * alignment platforms; modern compilers fold each 8-byte memcpy into a single
 * load/store. Macro-generated to avoid 8 near-identical functions.
 *
 * NOT operands take only `src`; binary AND/OR/XOR take `a` and `b`.
 */
#define SB_DEFINE_UNARY_KERNEL(name, expr_word, expr_byte)            \
    static void                                                       \
    name(unsigned char *dst, const unsigned char *src, ssize_t len)   \
    {                                                                 \
        ssize_t off = 0;                                              \
        ssize_t unrolled_end = len & ~31L;                            \
        ssize_t aligned_end  = len & ~7L;                             \
        for (; off < unrolled_end; off += 32) {                       \
            uint64_t s0, s1, s2, s3;                                  \
            memcpy(&s0, src + off,      8);                           \
            memcpy(&s1, src + off + 8,  8);                           \
            memcpy(&s2, src + off + 16, 8);                           \
            memcpy(&s3, src + off + 24, 8);                           \
            uint64_t d0 = (expr_word(s0));                            \
            uint64_t d1 = (expr_word(s1));                            \
            uint64_t d2 = (expr_word(s2));                            \
            uint64_t d3 = (expr_word(s3));                            \
            memcpy(dst + off,      &d0, 8);                           \
            memcpy(dst + off + 8,  &d1, 8);                           \
            memcpy(dst + off + 16, &d2, 8);                           \
            memcpy(dst + off + 24, &d3, 8);                           \
        }                                                             \
        for (; off < aligned_end; off += 8) {                         \
            uint64_t s;                                               \
            memcpy(&s, src + off, 8);                                 \
            uint64_t d = (expr_word(s));                              \
            memcpy(dst + off, &d, 8);                                 \
        }                                                             \
        for (; off < len; off++) dst[off] = (expr_byte(src[off]));    \
    }

#define SB_DEFINE_BINARY_KERNEL(name, expr_word, expr_byte)                       \
    static void                                                                   \
    name(unsigned char *dst, const unsigned char *a, const unsigned char *b,      \
         ssize_t len)                                                             \
    {                                                                             \
        ssize_t off = 0;                                                          \
        ssize_t unrolled_end = len & ~31L;                                        \
        ssize_t aligned_end  = len & ~7L;                                         \
        for (; off < unrolled_end; off += 32) {                                   \
            uint64_t a0, a1, a2, a3, b0, b1, b2, b3;                              \
            memcpy(&a0, a + off,      8); memcpy(&b0, b + off,      8);           \
            memcpy(&a1, a + off + 8,  8); memcpy(&b1, b + off + 8,  8);           \
            memcpy(&a2, a + off + 16, 8); memcpy(&b2, b + off + 16, 8);           \
            memcpy(&a3, a + off + 24, 8); memcpy(&b3, b + off + 24, 8);           \
            uint64_t d0 = expr_word(a0, b0);                                      \
            uint64_t d1 = expr_word(a1, b1);                                      \
            uint64_t d2 = expr_word(a2, b2);                                      \
            uint64_t d3 = expr_word(a3, b3);                                      \
            memcpy(dst + off,      &d0, 8);                                       \
            memcpy(dst + off + 8,  &d1, 8);                                       \
            memcpy(dst + off + 16, &d2, 8);                                       \
            memcpy(dst + off + 24, &d3, 8);                                       \
        }                                                                         \
        for (; off < aligned_end; off += 8) {                                     \
            uint64_t av, bv;                                                      \
            memcpy(&av, a + off, 8); memcpy(&bv, b + off, 8);                     \
            uint64_t d = expr_word(av, bv);                                       \
            memcpy(dst + off, &d, 8);                                             \
        }                                                                         \
        for (; off < len; off++) dst[off] = expr_byte(a[off], b[off]);            \
    }

#define SB_NOT_WORD(x)    (~(x))
#define SB_NOT_BYTE(x)    ((unsigned char)~(x))
#define SB_AND_WORD(x, y) ((x) & (y))
#define SB_AND_BYTE(x, y) ((unsigned char)((x) & (y)))
#define SB_OR_WORD(x, y)  ((x) | (y))
#define SB_OR_BYTE(x, y)  ((unsigned char)((x) | (y)))
#define SB_XOR_WORD(x, y) ((x) ^ (y))
#define SB_XOR_BYTE(x, y) ((unsigned char)((x) ^ (y)))

SB_DEFINE_UNARY_KERNEL (kern_not, SB_NOT_WORD, SB_NOT_BYTE)
SB_DEFINE_BINARY_KERNEL(kern_and, SB_AND_WORD, SB_AND_BYTE)
SB_DEFINE_BINARY_KERNEL(kern_or,  SB_OR_WORD,  SB_OR_BYTE)
SB_DEFINE_BINARY_KERNEL(kern_xor, SB_XOR_WORD, SB_XOR_BYTE)

/* Method wrappers: allocate-and-return form, and the in-place (!) form. */
#define SB_DEFINE_UNARY_METHODS(op_name, kernel)                          \
    static VALUE                                                          \
    rb_str_bitwise_##op_name(VALUE self)                                  \
    {                                                                     \
        ssize_t len = RSTRING_LEN(self);                                  \
        VALUE result = alloc_result(self);                                \
        kernel((unsigned char *)RSTRING_PTR(result),                      \
               (const unsigned char *)RSTRING_PTR(self), len);            \
        return result;                                                    \
    }                                                                     \
    static VALUE                                                          \
    rb_str_bitwise_##op_name##_bang(VALUE self)                           \
    {                                                                     \
        rb_str_modify(self);                                              \
        ssize_t len = RSTRING_LEN(self);                                  \
        unsigned char *ptr = (unsigned char *)RSTRING_PTR(self);          \
        kernel(ptr, ptr, len);                                            \
        return self;                                                      \
    }

#define SB_DEFINE_BINARY_METHODS(op_name, kernel)                         \
    static VALUE                                                          \
    rb_str_bitwise_##op_name(VALUE self, VALUE other)                     \
    {                                                                     \
        StringValue(other);                                               \
        check_binary_op_lengths(self, other);                             \
        ssize_t len = RSTRING_LEN(self);                                  \
        VALUE result = alloc_result(self);                                \
        kernel((unsigned char *)RSTRING_PTR(result),                      \
               (const unsigned char *)RSTRING_PTR(self),                  \
               (const unsigned char *)RSTRING_PTR(other), len);           \
        return result;                                                    \
    }                                                                     \
    static VALUE                                                          \
    rb_str_bitwise_##op_name##_bang(VALUE self, VALUE other)              \
    {                                                                     \
        StringValue(other);                                               \
        check_binary_op_lengths(self, other);                             \
        rb_str_modify(self);                                              \
        ssize_t len = RSTRING_LEN(self);                                  \
        unsigned char *a = (unsigned char *)RSTRING_PTR(self);            \
        const unsigned char *b = (const unsigned char *)RSTRING_PTR(other); \
        kernel(a, a, b, len);                                             \
        return self;                                                      \
    }

SB_DEFINE_UNARY_METHODS (not, kern_not)
SB_DEFINE_BINARY_METHODS(and, kern_and)
SB_DEFINE_BINARY_METHODS(or,  kern_or)
SB_DEFINE_BINARY_METHODS(xor, kern_xor)

/* packed bit-field iteration ---------------------------------------------- */
/*
 * NOTE: each_bit_field and bit_fields are implemented here and fully tested,
 * but are NOT part of the current core proposal (see FUTURE_PROPOSAL_PLAN.md).
 * They are deferred because yielding multi-bit field values decoded from a
 * packed layout is a qualitatively different contract from the single-bit
 * 1/0 values the rest of the API exchanges, and that difference is expected
 * to extend core-ruby-dev discussion. The code is kept so the proposal can be
 * extended later without re-implementation.
 */

/*
 * extract_uint64: extract up to 64 bits starting at bit_offset from src as an
 * unsigned 64-bit integer (LSB-first bit layout, matching bit_slice).
 *
 * Porting to Ruby Core:
 *   1. Move into string.c alongside the bit_slice implementation.
 *   2. Share with rb_str_bit_slice to avoid duplication.
 */
static uint64_t
extract_uint64_lsb(const unsigned char *src, ssize_t src_len, ssize_t bit_offset, ssize_t bitlen)
{
    uint64_t val = 0;
    ssize_t byte_off = bit_offset >> 3;
    int  shift    = (int)(bit_offset & 7);
    int  n        = (shift + (int)bitlen + 7) / 8;
    for (int i = 0; i < n; i++) {
        if (byte_off + i < src_len)
            val |= (uint64_t)src[byte_off + i] << (i * 8);
    }
    val >>= shift;
    if (bitlen < 64) val &= (UINT64_C(1) << bitlen) - 1;
    return val;
}

static uint64_t
extract_uint64(const unsigned char *src, ssize_t src_len,
               ssize_t bit_offset, ssize_t bitlen, int lsb_first)
{
    if (lsb_first) return extract_uint64_lsb(src, src_len, bit_offset, bitlen);

    /* MSB-first integer packing: the first collected bit becomes the MSB of
     * the result. This matches the natural integer encoding of MSB-first
     * packed records (e.g. mruby OP_ENTER fields, RFC header bit-fields,
     * BitTorrent piece-index fields). */
    uint64_t val = 0;
    for (ssize_t j = 0; j < bitlen; j++) {
        if (logical_get_bit(src, bit_offset + j, 0)) {
            val |= UINT64_C(1) << (bitlen - 1 - j);
        }
    }
    return val;
}

/* Yield each packed bit-field record as one Integer per field. */
static VALUE
rb_str_each_bit_field(int argc, VALUE *argv, VALUE self)
{
    RETURN_ENUMERATOR(self, argc, argv);

    VALUE rest, opts;
    rb_scan_args(argc, argv, "*:", &rest, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);

    ssize_t num_fields = RARRAY_LEN(rest);
    if (num_fields == 0) {
        rb_raise(rb_eArgError, "wrong number of arguments (given 0, expected 1+)");
    }

    ssize_t *bitlens = ALLOCA_N(ssize_t, num_fields);
    ssize_t step = 0;
    for (ssize_t f = 0; f < num_fields; f++) {
        VALUE v = RARRAY_AREF(rest, f);
        if (!rb_integer_type_p(v)) {
            rb_raise(rb_eTypeError, "bitlen must be an integer");
        }
        ssize_t bl = NUM2SSIZET(v);
        if (bl <= 0) {
            rb_raise(rb_eArgError, "bitlen must be positive");
        }
        if (bl > 64) {
            rb_raise(rb_eArgError, "bitlen must be <= 64 (got %" PRIdPTR ")", (intptr_t)bl);
        }
        bitlens[f] = bl;
        step += bl;
    }

    int lsb_first = parse_lsb_first_opt(opts);

    ssize_t src_len = RSTRING_LEN(self);
    int64_t total_bits = SB_BIT_LEN(src_len);
    ssize_t iterations = (ssize_t)(total_bits / step);

    VALUE *field_vals = ALLOCA_N(VALUE, num_fields);

    for (ssize_t iter = 0; iter < iterations; iter++) {
        ssize_t base_bit = iter * step;
        const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
        ssize_t field_bit = base_bit;
        for (ssize_t f = 0; f < num_fields; f++) {
            uint64_t val = extract_uint64(src, src_len, field_bit, bitlens[f], lsb_first);
            field_vals[f] = ULL2NUM(val);
            field_bit += bitlens[f];
        }
        rb_yield_values2((int)num_fields, field_vals);
    }

    return self;
}

/* Non-iterator form of each_bit_field; collect bit-field records into an Array. */
static VALUE
rb_str_bit_fields(int argc, VALUE *argv, VALUE self)
{
    VALUE rest, opts;
    rb_scan_args(argc, argv, "*:", &rest, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);

    ssize_t num_fields = RARRAY_LEN(rest);
    if (num_fields == 0) {
        rb_raise(rb_eArgError, "wrong number of arguments (given 0, expected 1+)");
    }

    ssize_t *bitlens = ALLOCA_N(ssize_t, num_fields);
    ssize_t step = 0;
    for (ssize_t f = 0; f < num_fields; f++) {
        VALUE v = RARRAY_AREF(rest, f);
        if (!rb_integer_type_p(v)) {
            rb_raise(rb_eTypeError, "bitlen must be an integer");
        }
        ssize_t bl = NUM2SSIZET(v);
        if (bl <= 0) {
            rb_raise(rb_eArgError, "bitlen must be positive");
        }
        if (bl > 64) {
            rb_raise(rb_eArgError, "bitlen must be <= 64 (got %" PRIdPTR ")", (intptr_t)bl);
        }
        bitlens[f] = bl;
        step += bl;
    }

    int lsb_first = parse_lsb_first_opt(opts);

    ssize_t src_len = RSTRING_LEN(self);
    int64_t total_bits = SB_BIT_LEN(src_len);
    ssize_t iterations = (ssize_t)(total_bits / step);

    int have_block = rb_block_given_p();
    VALUE result = have_block ? Qnil : rb_ary_new_capa(iterations);

    VALUE *field_vals = ALLOCA_N(VALUE, num_fields);

    for (ssize_t iter = 0; iter < iterations; iter++) {
        ssize_t base_bit = iter * step;
        const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
        ssize_t field_bit = base_bit;
        for (ssize_t f = 0; f < num_fields; f++) {
            uint64_t val = extract_uint64(src, src_len, field_bit, bitlens[f], lsb_first);
            field_vals[f] = ULL2NUM(val);
            field_bit += bitlens[f];
        }
        if (have_block) {
            rb_yield_values2((int)num_fields, field_vals);
        } else if (num_fields == 1) {
            rb_ary_push(result, field_vals[0]);
        } else {
            rb_ary_push(result, rb_ary_new_from_values(num_fields, field_vals));
        }
    }

    return have_block ? self : result;
}

/* run-length iteration ---------------------------------------------------- */

/*
 * count_run_lsb: count consecutive bits equal to `target` starting at flat
 * position `bit_offset` (LSB-first).  Uses ctz / ctzll to skip bits in bulk:
 *   - partial first byte: ctz on the inverted masked nibble
 *   - full 64-bit words (LE): ctzll on the inverted word (64 bits per step)
 *   - remaining bytes: ctz on the inverted byte
 *
 * Porting to Ruby Core:
 *   1. Move to string.c alongside bit_get and each_bit.
 *   2. Share sb_ctz8 / sb_ctzll with the existing set-bit helpers.
 */
static ssize_t
count_run_lsb(const unsigned char *src, ssize_t src_len, ssize_t bit_offset, int target)
{
    int64_t max_run  = SB_BIT_LEN(src_len) - bit_offset;
    ssize_t byte_idx = bit_offset >> 3;
    int  bit_off  = bit_offset & 7;
    ssize_t count    = 0;

    /* partial first byte: shift pos to bit 0, mask remaining bits */
    {
        int          remaining = 8 - bit_off;
        unsigned int b         = (unsigned int)src[byte_idx] >> bit_off;
        if (!target) b         = ~b;
        unsigned int mask      = (1u << remaining) - 1;
        b                     &= mask;
        unsigned int inv       = (~b) & mask;
        int run                = (inv == 0) ? remaining : sb_ctz8(inv);
        count                 += run;
        byte_idx++;
        if (run < remaining)
            return (ssize_t)(count < max_run ? count : max_run);
    }

#if SB_LITTLE_ENDIAN
    /* full 8-byte words: skip 64 identical bits per iteration */
    while (byte_idx + 8 <= src_len) {
        uint64_t word;
        memcpy(&word, src + byte_idx, 8);
        if (!target) word = ~word;
        if (word == UINT64_MAX) {
            count    += 64;
            byte_idx += 8;
        } else {
            count += sb_ctzll(~word);
            return (ssize_t)(count < max_run ? count : max_run);
        }
    }
#endif

    /* remaining bytes (< 8) */
    while (byte_idx < src_len) {
        unsigned int b = (unsigned int)src[byte_idx];
        if (!target) b = ~b;
        b &= 0xFF;
        if (b == 0xFF) {
            count += 8;
            byte_idx++;
        } else {
            count += sb_ctz8(~b);
            return (ssize_t)(count < max_run ? count : max_run);
        }
    }

    return (ssize_t)(count < max_run ? count : max_run);
}

/* Return the length of the consecutive run of `bit` starting at pos, or nil. */
static VALUE
rb_str_bit_run_count(int argc, VALUE *argv, VALUE self)
{
    VALUE bit_offset_v, bit_val, opts;
    rb_scan_args(argc, argv, "20:", &bit_val, &bit_offset_v, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);

    if (!rb_integer_type_p(bit_offset_v)) {
        rb_raise(rb_eTypeError, "position must be an integer");
    }
    int target = parse_bit_target(bit_val);
    ssize_t src_len = RSTRING_LEN(self);
    /* A position with no run starting at it and a position outside the string
     * are the same answer here -- nil -- so a negative or past-the-end offset
     * is not an error (see ProposedMethods.md, "Why bit_run_count never
     * returns 0"). */
    uint64_t off;
    if (sb_bit_position_soft(bit_offset_v, &off) < 0) return Qnil;
    if (off >= (uint64_t)SB_BIT_LEN(src_len)) return Qnil;
    ssize_t bit_offset = (ssize_t)off;

    const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
    if (lsb_first) {
        if (((src[bit_offset >> 3] >> (bit_offset & 7)) & 1) != target) return Qnil;
        return SSIZET2NUM(count_run_lsb(src, src_len, bit_offset, target));
    }

    if (logical_get_bit(src, bit_offset, 0) != target) return Qnil;

    ssize_t run = 1;
    int64_t total_bits = SB_BIT_LEN(src_len);
    while (bit_offset + run < total_bits && logical_get_bit(src, bit_offset + run, 0) == target) {
        run++;
    }
    return SSIZET2NUM(run);
}

/* Yield (bit, offset, run_length) triples for each consecutive run of identical bits. */
/* Unified emitter for each_bit_run / bit_runs.
 *
 * Walks the bitmap in (bit, run_length) chunks. Yields each pair (when
 * ary == Qnil) or pushes (bit, run_length) Arrays to the pre-allocated
 * result. The LSB-first path uses the fast count_run_lsb (word-at-a-time
 * via ctzll); the MSB-first path scans bit by bit through logical_get_bit.
 *
 * self is re-read inside the loop because rb_yield can invoke Ruby code
 * that mutates the receiver, potentially invalidating RSTRING_PTR.
 */
static void
emit_bit_runs(VALUE self, int lsb_first, ssize_t start_offset, VALUE ary)
{
    ssize_t src_len    = RSTRING_LEN(self);
    int64_t total_bits = SB_BIT_LEN(src_len);
    if (src_len == 0 || start_offset >= total_bits) return;
    ssize_t offset     = start_offset;

#define SB_EMIT_TRIPLE(bval, oval, lval) \
    do { if (ary == Qnil) rb_yield_values(3, (bval), (oval), (lval)); \
         else rb_ary_push(ary, rb_ary_new3(3, (bval), (oval), (lval))); } while (0)

    if (lsb_first) {
        while (offset < total_bits) {
            const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
            int bit = (src[offset >> 3] >> (offset & 7)) & 1;
            ssize_t run = count_run_lsb(src, src_len, offset, bit);
            SB_EMIT_TRIPLE(INT2FIX(bit), SSIZET2NUM(offset), SSIZET2NUM(run));
            offset += run;
        }
    }
    else {
        while (offset < total_bits) {
            const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
            int bit = logical_get_bit(src, offset, 0);
            ssize_t run = 1;
            while (offset + run < total_bits && logical_get_bit(src, offset + run, 0) == bit) {
                run++;
            }
            SB_EMIT_TRIPLE(INT2FIX(bit), SSIZET2NUM(offset), SSIZET2NUM(run));
            offset += run;
        }
    }

#undef SB_EMIT_TRIPLE
}

static VALUE
rb_str_each_bit_run(int argc, VALUE *argv, VALUE self)
{
    RETURN_ENUMERATOR(self, argc, argv);

    VALUE start_offset_v = Qnil, opts = Qnil;
    rb_scan_args(argc, argv, "01:", &start_offset_v, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);
    ssize_t start_offset = parse_start_offset(self, start_offset_v);

    emit_bit_runs(self, lsb_first, start_offset, Qnil);
    return self;
}

/* Non-iterator form of each_bit_run; collect run triples into an Array. */
static VALUE
rb_str_bit_runs(int argc, VALUE *argv, VALUE self)
{
    VALUE start_offset_v = Qnil, opts = Qnil;
    rb_scan_args(argc, argv, "01:", &start_offset_v, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);
    ssize_t start_offset = parse_start_offset(self, start_offset_v);

    if (rb_block_given_p()) {
        emit_bit_runs(self, lsb_first, start_offset, Qnil);
        return self;
    }

    VALUE ary = rb_ary_new();
    emit_bit_runs(self, lsb_first, start_offset, ary);
    return ary;
}

/* Write bits from str into self at bit-level granularity (inverse of bit_slice). */
static VALUE
rb_str_bit_splice(int argc, VALUE *argv, VALUE self)
{
    ssize_t dst_bit_off, dst_bit_len;
    ssize_t src_bit_off;
    VALUE str;
    int64_t dst_total = SB_BIT_LEN(RSTRING_LEN(self));
    VALUE v0, v1, v2, v3, opts;

    int n_pos = rb_scan_args(argc, argv, "22:", &v0, &v1, &v2, &v3, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);

    uint64_t dst_off_u, dst_len_u, src_off_u, src_len_u;

    if (n_pos <= 3 && rb_obj_is_kind_of(v0, rb_cRange)) {
        /* bit_splice(range, str) and bit_splice(range, str, str_bit_index) */
        ssize_t beg, len;
        if (sb_bit_range_beg_len(v0, &beg, &len, dst_total) == SB_RANGE_OUT) {
            rb_raise(rb_eIndexError,
                     "bit_splice: destination range starts past the end "
                     "(total %" PRId64 " bits)", (int64_t)dst_total);
        }
        dst_off_u = (uint64_t)beg;
        dst_len_u = (uint64_t)len;
        str = v1;
        Check_Type(str, T_STRING);
        if (n_pos == 3) {
            if (!rb_integer_type_p(v2)) {
                rb_raise(rb_eTypeError, "third argument must be an Integer");
            }
            src_off_u = sb_bit_offset(v2);
        }
        else {
            src_off_u = 0;
        }
        src_len_u = dst_len_u;
    }
    else if (n_pos == 3) {
        /* bit_splice(bit_index, bit_length, str) */
        if (!rb_integer_type_p(v0) || !rb_integer_type_p(v1)) {
            rb_raise(rb_eTypeError, "bit index and length must be integers");
        }
        dst_off_u = sb_bit_offset(v0);
        dst_len_u = sb_bit_length(v1);

        /*
         * Integer source support was prototyped here, but it is intentionally
         * disabled in the current proposal to keep the public API limited to
         * String-to-String splicing.
         */
        if (rb_integer_type_p(v2)) {
            rb_raise(rb_eArgError,
                     "bit_splice source must be a String in the current proposal");
        }

        str = v2;
        Check_Type(str, T_STRING);
        src_off_u = 0;
        src_len_u = dst_len_u;
    }
    else if (n_pos == 4) {
        /* bit_splice(bit_index, bit_length, str, str_bit_index) */
        if (!rb_integer_type_p(v0) || !rb_integer_type_p(v1) || !rb_integer_type_p(v3)) {
            rb_raise(rb_eTypeError, "bit indices and lengths must be integers");
        }
        dst_off_u = sb_bit_offset(v0);
        dst_len_u = sb_bit_length(v1);
        str = v2;
        Check_Type(str, T_STRING);
        src_off_u = sb_bit_offset(v3);
        src_len_u = dst_len_u;
    }
    else {
        rb_raise(rb_eArgError,
                 "wrong number of arguments (given %d, expected 2, 3, or 4)", n_pos);
    }

    /* Both ranges are checked in 64 bits: a position past the end of the
     * string is well-formed input here, so it must reach this bounds check
     * rather than being rejected while it is parsed. */
    if (dst_off_u > (uint64_t)dst_total || dst_len_u > (uint64_t)dst_total - dst_off_u) {
        rb_raise(rb_eIndexError,
                 "bit_splice: destination range [%" PRIu64 ", %" PRIu64
                 "] out of bounds (total %" PRId64 " bits)",
                 dst_off_u, dst_len_u, (int64_t)dst_total);
    }

    int64_t src_total_bits = SB_BIT_LEN(RSTRING_LEN(str));
    if (src_off_u > (uint64_t)src_total_bits ||
        src_len_u > (uint64_t)src_total_bits - src_off_u) {
        rb_raise(rb_eIndexError,
                 "bit_splice: source range [%" PRIu64 ", %" PRIu64
                 "] out of bounds (total %" PRId64 " bits)",
                 src_off_u, src_len_u, (int64_t)src_total_bits);
    }

    /* Both ranges are inside their strings now, so narrowing only fails on an
     * ILP32 build whose bit length exceeds the internal index (see
     * sb_narrow_bit_pos). */
    dst_bit_off = sb_narrow_bit_pos(dst_off_u, dst_total);
    dst_bit_len = (ssize_t)dst_len_u;
    if (dst_len_u > (uint64_t)(sb_addressable_bits(dst_total) - dst_bit_off)) {
        rb_raise(rb_eArgError, "bit index out of representable range");
    }
    src_bit_off = sb_narrow_bit_pos(src_off_u, src_total_bits);
    if (src_len_u > (uint64_t)(sb_addressable_bits(src_total_bits) - src_bit_off)) {
        rb_raise(rb_eArgError, "bit index out of representable range");
    }

    if (dst_bit_len == 0) return self;

    /* Guard against self-aliasing: duplicate src before modifying self */
    VALUE src_str = (str == self) ? rb_str_dup(str) : str;

    rb_str_modify(self);

    unsigned char       *dst          = (unsigned char *)RSTRING_PTR(self);
    const unsigned char *src          = (const unsigned char *)RSTRING_PTR(src_str);
    ssize_t                 src_len_bytes = RSTRING_LEN(src_str);

    if (lsb_first) {
        bit_copy_core(dst, dst_bit_off, src, src_len_bytes, src_bit_off, dst_bit_len);
    } else {
        ssize_t current_src_bit = src_bit_off;
        ssize_t start_byte = dst_bit_off >> 3;
        ssize_t end_byte = (dst_bit_off + dst_bit_len - 1) >> 3;

        for (ssize_t b = start_byte; b <= end_byte; b++) {
            ssize_t b_start_l = b << 3;
            ssize_t b_end_l = b_start_l + 7;
            ssize_t l_min = (dst_bit_off > b_start_l) ? dst_bit_off : b_start_l;
            ssize_t l_max = ((dst_bit_off + dst_bit_len - 1) < b_end_l) ? (dst_bit_off + dst_bit_len - 1) : b_end_l;

            ssize_t p_min = b_start_l + (7 - (l_max & 7L));
            ssize_t p_max = b_start_l + (7 - (l_min & 7L));
            ssize_t chunk_len = p_max - p_min + 1;

            bit_copy_core(dst, p_min, src, src_len_bytes, current_src_bit, chunk_len);
            current_src_bit += chunk_len;
        }
    }

    RB_GC_GUARD(src_str);
    return self;
}

/* Array#mask and Array#mask! --------------------------------------- */
/*
 * NOTE: Array#mask and Array#mask! are implemented here and fully tested,
 * but are NOT part of the current core proposal (see FUTURE_PROPOSAL_PLAN.md).
 */
/*
 * parse_mask_kwargs: extract bitmap, lsb_first:, and invert: from method arguments.
 *
 * Porting to Ruby Core:
 *   1. Keep this as a `static` helper in array.c — it is only called by
 *      ary_mask and ary_mask_bang, so no header declaration is needed.
 *   2. Rename to ary_mask_kwargs or similar to follow array.c conventions
 *      (static helpers in array.c use the `ary_` prefix, not `rb_ary_`.
 */
static void
parse_mask_kwargs(int argc, VALUE *argv, VALUE *bitmap_out,
                     int *lsb_first_out, int *invert_out, int *is_integer_out)
{
    VALUE bitmap, opts;
    rb_scan_args(argc, argv, "1:", &bitmap, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST | SB_KW_INVERT);

    int is_integer = rb_integer_type_p(bitmap);

    if (!is_integer) Check_Type(bitmap, T_STRING);

    int lsb_first  = parse_lsb_first_opt(opts);
    int invert     = 0; /* default false */

    if (!lsb_first && is_integer) {
        rb_raise(rb_eArgError,
                 "lsb_first: false is not supported for Integer bitmap; "
                 "Integer bits are always LSB-first");
    }

    if (!NIL_P(opts)) {
        VALUE inv = rb_hash_aref(opts, sym_invert);
        if (!NIL_P(inv)) {
            invert = RTEST(inv) ? 1 : 0;
        }
    }

    *bitmap_out     = bitmap;
    *lsb_first_out  = lsb_first;
    *invert_out     = invert;
    *is_integer_out = is_integer;
}

/* Read bit i from an Integer bitmap (always LSB-first).
 * Bits beyond the integer's width are 0 (valid for non-negative integers). */
static inline int
integer_get_bit(VALUE n, ssize_t i)
{
    if (FIXNUM_P(n)) {
        ssize_t v = NUM2SSIZET(n);
        if (v < 0)
            rb_raise(rb_eArgError, "Integer bitmap must be non-negative");
        if (i >= (ssize_t)(sizeof(ssize_t) * CHAR_BIT) - 1) return 0;
        return (int)((v >> i) & 1);
    }
    if (RBIGNUM_NEGATIVE_P(n))
        rb_raise(rb_eArgError, "Integer bitmap must be non-negative");
    VALUE bit = rb_funcall(n, id_bracket, 1, SSIZET2NUM(i));
    return RB_TEST(bit) ? 1 : 0;
}

static VALUE
rb_ary_mask(int argc, VALUE *argv, VALUE self)
{
    VALUE bitmap;
    int lsb_first, invert, is_integer;
    parse_mask_kwargs(argc, argv, &bitmap, &lsb_first, &invert, &is_integer);

    ssize_t ary_len = RARRAY_LEN(self);
    const VALUE *src = RARRAY_CONST_PTR(self);
    VALUE result = rb_ary_new_capa(ary_len);

    if (is_integer) {
        for (ssize_t i = 0; i < ary_len; i++) {
            int bit = integer_get_bit(bitmap, i);
            if (invert) bit = !bit;
            rb_ary_push(result, bit ? src[i] : Qnil);
        }
    }
    else {
        const unsigned char *bmp = (const unsigned char *)RSTRING_PTR(bitmap);
        ssize_t bmp_len    = RSTRING_LEN(bitmap);
        ssize_t needed     = (ary_len + 7) >> 3;
        if (needed > bmp_len)
            rb_raise(rb_eArgError,
                     "bitmap too short: need %" PRIdPTR " bytes for %" PRIdPTR
                     " elements, got %" PRIdPTR,
                     (intptr_t)needed, (intptr_t)ary_len, (intptr_t)bmp_len);

        if (!lsb_first) {
            for (ssize_t i = 0; i < ary_len; i++) {
                int bit = (bmp[i >> 3] >> (7 - (i & 7))) & 1;
                if (invert) bit = !bit;
                rb_ary_push(result, bit ? src[i] : Qnil);
            }
        }
        else {
            for (ssize_t i = 0; i < ary_len; i++) {
                int bit = (bmp[i >> 3] >> (i & 7)) & 1;
                if (invert) bit = !bit;
                rb_ary_push(result, bit ? src[i] : Qnil);
            }
        }
    }

    return result;
}

static VALUE
rb_ary_mask_bang(int argc, VALUE *argv, VALUE self)
{
    VALUE bitmap;
    int lsb_first, invert, is_integer;
    parse_mask_kwargs(argc, argv, &bitmap, &lsb_first, &invert, &is_integer);

    ssize_t ary_len = RARRAY_LEN(self);
    rb_ary_modify(self);

    if (is_integer) {
        for (ssize_t i = 0; i < ary_len; i++) {
            int bit = integer_get_bit(bitmap, i);
            if (invert) bit = !bit;
            if (!bit) rb_ary_store(self, i, Qnil);
        }
    }
    else {
        const unsigned char *bmp = (const unsigned char *)RSTRING_PTR(bitmap);
        ssize_t bmp_len    = RSTRING_LEN(bitmap);
        ssize_t needed     = (ary_len + 7) >> 3;
        if (needed > bmp_len)
            rb_raise(rb_eArgError,
                     "bitmap too short: need %" PRIdPTR " bytes for %" PRIdPTR
                     " elements, got %" PRIdPTR,
                     (intptr_t)needed, (intptr_t)ary_len, (intptr_t)bmp_len);

        if (!lsb_first) {
            for (ssize_t i = 0; i < ary_len; i++) {
                int bit = (bmp[i >> 3] >> (7 - (i & 7))) & 1;
                if (invert) bit = !bit;
                if (!bit) rb_ary_store(self, i, Qnil);
            }
        }
        else {
            for (ssize_t i = 0; i < ary_len; i++) {
                int bit = (bmp[i >> 3] >> (i & 7)) & 1;
                if (invert) bit = !bit;
                if (!bit) rb_ary_store(self, i, Qnil);
            }
        }
    }

    return self;
}

/* Init -------------------------------------------------------------------- */

void
Init_string_bits(void)
{
    id_bracket    = rb_intern("[]");
    sym_lsb_first = ID2SYM(rb_intern("lsb_first"));
    sym_invert    = ID2SYM(rb_intern("invert"));

    rb_define_method(rb_cString, "bit_get",         rb_str_bit_get,          -1);
    rb_define_method(rb_cString, "bit_set?",        rb_str_bit_set_p,        -1);
    rb_define_method(rb_cString, "bit_count",       rb_str_bit_count,        -1);
    rb_define_method(rb_cString, "each_bit",        rb_str_each_bit,         -1);
    rb_define_method(rb_cString, "bits",            rb_str_bits,             -1);
    rb_define_method(rb_cString, "each_bit_offset", rb_str_each_bit_offset,  -1);
    rb_define_method(rb_cString, "bit_offsets",     rb_str_bit_offsets,      -1);
    rb_define_method(rb_cString, "bit_slice",       rb_str_bit_slice,        -1);
    rb_define_method(rb_cString, "bit_splice",      rb_str_bit_splice,       -1);
    rb_define_method(rb_cString, "bit_run_count",   rb_str_bit_run_count,    -1);
    rb_define_method(rb_cString, "each_bit_run",    rb_str_each_bit_run,     -1);
    rb_define_method(rb_cString, "bit_runs",        rb_str_bit_runs,         -1);
    rb_define_method(rb_cString, "bit_set",         rb_str_bit_set,          -1);
    rb_define_method(rb_cString, "bit_clear",       rb_str_bit_clear,        -1);
    rb_define_method(rb_cString, "bit_flip",        rb_str_bit_flip,         -1);
    rb_define_method(rb_cString, "bitwise_not",     rb_str_bitwise_not,       0);
    rb_define_method(rb_cString, "bitwise_not!",    rb_str_bitwise_not_bang,  0);
    rb_define_method(rb_cString, "bitwise_and",     rb_str_bitwise_and,       1);
    rb_define_method(rb_cString, "bitwise_and!",    rb_str_bitwise_and_bang,  1);
    rb_define_method(rb_cString, "bitwise_or",      rb_str_bitwise_or,        1);
    rb_define_method(rb_cString, "bitwise_or!",     rb_str_bitwise_or_bang,   1);
    rb_define_method(rb_cString, "bitwise_xor",     rb_str_bitwise_xor,       1);
    rb_define_method(rb_cString, "bitwise_xor!",    rb_str_bitwise_xor_bang,  1);

    // These methods are defined here to avoid cluttering this file, but they are not part of the current core proposal (see FUTURE_PROPOSAL_PLAN.md).
    rb_define_method(rb_cString, "each_bit_field",  rb_str_each_bit_field,   -1);
    rb_define_method(rb_cString, "bit_fields",      rb_str_bit_fields,       -1);
    rb_define_method(rb_cArray,  "mask",            rb_ary_mask,             -1);
    rb_define_method(rb_cArray,  "mask!",           rb_ary_mask_bang,        -1);
}
