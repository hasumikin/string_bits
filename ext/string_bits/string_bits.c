#include "ruby.h"
#include "ruby/encoding.h"

#include <limits.h>     /* CHAR_BIT */
#include <stdint.h>     /* uint64_t, UINT64_MAX */
#include <string.h>     /* memcpy */
#include <sys/types.h>  /* ssize_t (Ruby typedefs it on Windows) */

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
static VALUE sym_lsb_first, sym_lsb, sym_msb, sym_invert;

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

/* Convert a Ruby Integer to a ssize_t bit index.
 *
 * Raises ArgumentError for Bignums on all platforms: a Bignum cannot be a
 * valid bit index for any real string, and raising explicitly is clearer than
 * silently mapping to a sentinel value that later triggers a different error.
 * NUM2SSIZET is width-aware (uses FIX2LL on LLP64, FIX2LONG on LP64) so the
 * FIXNUM extraction does not truncate large FIXNUMs on Windows.
 *
 * RBIGNUM_NEGATIVE_P is available via ruby.h -> ruby/internal/core/rbignum.h. */
static ssize_t
integer_to_bit_idx(VALUE n)
{
    if (FIXNUM_P(n)) return NUM2SSIZET(n);
    RUBY_ASSERT(RB_TYPE_P(n, T_BIGNUM));
    rb_raise(rb_eArgError, "bit index out of representable range");
    UNREACHABLE_RETURN(0);
}

static ssize_t
check_bit_index(VALUE self, VALUE n, int lsb_first)
{
    if (!rb_integer_type_p(n)) {
        rb_raise(rb_eTypeError, "bit index must be an integer");
    }
    ssize_t idx = integer_to_bit_idx(n);
    ssize_t size = RSTRING_LEN(self) * 8;
    if (idx < 0 || idx >= size) {
        rb_raise(rb_eIndexError, "bit index out of range");
    }
    if (!lsb_first) idx = (idx & ~7L) | (7 - (idx & 7L));
    return idx;
}

static inline ssize_t
physical_to_count_from(ssize_t physical, int lsb_first)
{
    return lsb_first ? physical : ((physical & ~7L) | (7 - (physical & 7L)));
}

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

/* ssize_t-interface wrapper around rb_range_beg_len.
 *
 * rb_range_beg_len() takes (long *begp, long *lenp, long len), but this
 * extension uses ssize_t throughout for LP64/LLP64 uniformity.  On Windows
 * (LLP64) long is 32-bit while ssize_t is 64-bit, so passing &ssize_t to the
 * stock API is a type error and the build fails.  This wrapper bridges the
 * two and clamps the input length to LONG_MAX on platforms where ssize_t
 * is wider than long, which has no practical effect: a 2 GiB string is
 * already past what any realistic caller will hand us.
 *
 * It also catches the RangeError that rb_range_beg_len raises when a Range
 * endpoint is a Bignum too large for `long`, and re-raises it as IndexError
 * so that out-of-range bit positions report uniformly across LP64 and LLP64.
 */
struct sb_range_args {
    VALUE range;
    long *lbegp;
    long *llenp;
    long len;
    int err;
};

static VALUE
sb_range_beg_len_call(VALUE arg)
{
    struct sb_range_args *a = (struct sb_range_args *)arg;
    return rb_range_beg_len(a->range, a->lbegp, a->llenp, a->len, a->err);
}

static inline VALUE
sb_range_beg_len(VALUE range, ssize_t *begp, ssize_t *lenp, ssize_t len, int err)
{
    long lbeg = 0, llen = 0;
    long clipped = (len > (ssize_t)LONG_MAX) ? LONG_MAX : (long)len;
    struct sb_range_args args = { range, &lbeg, &llen, clipped, err };
    int state = 0;
    VALUE result = rb_protect(sb_range_beg_len_call, (VALUE)&args, &state);
    if (state) {
        VALUE exc = rb_errinfo();
        rb_set_errinfo(Qnil);
        if (rb_obj_is_kind_of(exc, rb_eRangeError)) {
            rb_raise(rb_eIndexError, "bit range out of range");
        }
        rb_exc_raise(exc);
    }
    if (begp) *begp = (ssize_t)lbeg;
    if (lenp) *lenp = (ssize_t)llen;
    return result;
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

static int
parse_bool_opt(VALUE opts, VALUE key, const char *name, int default_value)
{
    if (NIL_P(opts)) return default_value;
    VALUE value = rb_hash_aref(opts, key);
    if (NIL_P(value)) return default_value;
    if (value == Qtrue) return 1;
    if (value == Qfalse) return 0;
    rb_raise(rb_eArgError, "%s must be true or false", name);
    return default_value;
}

static int
parse_lsb_first_opt(VALUE opts)
{
    return parse_bool_opt(opts, sym_lsb_first, "lsb_first", 1);
}

static int
parse_lsb_first(int argc, VALUE *argv)
{
    VALUE opts = Qnil;
    rb_scan_args(argc, argv, "0:", &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    return parse_lsb_first_opt(opts);
}

/* read -------------------------------------------------------------------- */

/* String#bit_at(n, lsb_first: true) -> true or false
 *
 * bit_at uses flat/Arrow convention: byte_index = n/8 from start, bit = n%8 from LSB
 * e.g. "\xAA\xCC": bit 0..7 live in byte[0]=0xAA, bit 8..15 live in byte[1]=0xCC
 *
 *   str = "\xFF\xAA" # 11111111 10101010
 *   str.bit_at(0) # => true (1st bit is set)
 *   str.bit_at(7) # => true (8th bit is set)
 *   str.bit_at(8) # => false (9th bit is clear)
 *   str.bit_at(9) # => true (10th bit is set)
 *   str.bit_at(16) # => nil
 */
static VALUE
rb_str_bit_at(int argc, VALUE *argv, VALUE self)
{
    VALUE n, opts;
    rb_scan_args(argc, argv, "1:", &n, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);

    if (!rb_integer_type_p(n)) {
        rb_raise(rb_eTypeError, "bit index must be an integer");
    }
    ssize_t idx = integer_to_bit_idx(n);
    if (idx < 0) {
        rb_raise(rb_eIndexError, "bit index out of range");
    }
    ssize_t size = RSTRING_LEN(self) * 8;
    if (size <= idx) {
        return Qnil;
    }

    int lsb_first = parse_lsb_first_opt(opts);

    if (!lsb_first) {
        idx = (idx & ~7L) | (7 - (idx & 7L));
    }

    if (test_bit(RSTRING_PTR(self), idx)) {
        return Qtrue;
    } else {
        return Qfalse;
    }
}

static VALUE
rb_str_bit_count(VALUE self)
{
    ssize_t count = 0;
    ssize_t len = RSTRING_LEN(self);
    const char *str = RSTRING_PTR(self);
    ssize_t off = 0;
    ssize_t unrolled_end = len & ~31L;
    ssize_t aligned_end  = len & ~7L;

    /* Use memcpy to avoid unaligned loads (SIGBUS on SPARC, MIPS, etc.)
     * and strict-aliasing violations. Modern compilers fold 8-byte memcpy
     * into a single load on platforms that allow unaligned access. */
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
        const unsigned char *tail = (const unsigned char *)(str + aligned_end);
        for (ssize_t i = 0; i < remainder; i++) {
            last |= (uint64_t)tail[i] << (i * 8);
        }
        count += sb_popcount64(last);
    }

    return SSIZET2NUM(count);
}

/* iterate bits ------------------------------------------------------------ */

static VALUE
rb_str_each_bit(int argc, VALUE *argv, VALUE self)
{
    RETURN_ENUMERATOR(self, argc, argv);

    int lsb_first = parse_lsb_first(argc, argv);
    ssize_t len = RSTRING_LEN(self);
    const unsigned char *str = (const unsigned char *)RSTRING_PTR(self);

    for (ssize_t i = 0; i < len; i++) {
        unsigned char b = str[i];
        if (lsb_first) {
            for (int j = 0; j < 8; j++) {
                rb_yield((b >> j) & 1 ? Qtrue : Qfalse);
            }
        } else {
            for (int j = 7; j >= 0; j--) {
                rb_yield((b >> j) & 1 ? Qtrue : Qfalse);
            }
        }
    }

    return self;
}

static VALUE
rb_str_bits(int argc, VALUE *argv, VALUE self)
{
    int lsb_first = parse_lsb_first(argc, argv);
    ssize_t len = RSTRING_LEN(self);
    const unsigned char *str = (const unsigned char *)RSTRING_PTR(self);
    ssize_t total_bits = len * 8;
    int have_block = rb_block_given_p();

    VALUE ary = have_block ? Qnil : rb_ary_new_capa(total_bits);

    for (ssize_t i = 0; i < len; i++) {
        unsigned char b = str[i];
        if (lsb_first) {
            for (int j = 0; j < 8; j++) {
                VALUE bit = (b >> j) & 1 ? Qtrue : Qfalse;
                have_block ? rb_yield(bit) : rb_ary_push(ary, bit);
            }
        } else {
            for (int j = 7; j >= 0; j--) {
                VALUE bit = (b >> j) & 1 ? Qtrue : Qfalse;
                have_block ? rb_yield(bit) : rb_ary_push(ary, bit);
            }
        }
    }

    return have_block ? self : ary;
}

/* iterate bit positions matching `bit` ------------------------------------ */

/* parse the required `bit` argument (true/false/1/0) to 0 or 1 */
static int
parse_bit_target(VALUE bit_val)
{
    if (bit_val == Qtrue || bit_val == INT2FIX(1)) return 1;
    if (bit_val == Qfalse || bit_val == INT2FIX(0)) return 0;
    rb_raise(rb_eArgError, "bit must be 0, 1, false, or true");
}

static VALUE
rb_str_each_bit_offset(int argc, VALUE *argv, VALUE self)
{
    RETURN_ENUMERATOR(self, argc, argv);

    VALUE bit_val, opts;
    rb_scan_args(argc, argv, "10:", &bit_val, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);
    int target    = parse_bit_target(bit_val);

    ssize_t len = RSTRING_LEN(self);
    const unsigned char *str = (const unsigned char *)RSTRING_PTR(self);
    if (lsb_first) {
        /* LSB-first: ascending positions 0, 1, 2, ...
         * On little-endian, loading 8 bytes as a uint64_t preserves the flat
         * LSB-first bit numbering: word bit 0 == position 0, bit 63 == 63.
         * For target=0, invert the loaded word/byte so the same ctz scan
         * yields unset-bit positions; all 8/64 bits of the inverted unit are
         * valid positions, since each byte contributes exactly 8 positions. */
#if SB_LITTLE_ENDIAN
        ssize_t n_words = len >> 3;
        for (ssize_t wi = 0; wi < n_words; wi++) {
            uint64_t w;
            memcpy(&w, str + wi * 8, 8);
            if (target == 0) w = ~w;
            while (w != 0) {
                int bit = sb_ctzll(w);
                rb_yield(SSIZET2NUM(wi * 64 + bit));
                w &= w - 1;
            }
        }
        for (ssize_t bi = n_words << 3; bi < len; bi++) {
            unsigned int b = str[bi];
            if (target == 0) b = (~b) & 0xFF;
            while (b != 0) {
                int bit = sb_ctz8(b);
                rb_yield(SSIZET2NUM(bi * 8 + bit));
                b &= b - 1;
            }
        }
#else
        for (ssize_t bi = 0; bi < len; bi++) {
            unsigned int b = str[bi];
            if (target == 0) b = (~b) & 0xFF;
            while (b != 0) {
                int bit = sb_ctz8(b);
                rb_yield(SSIZET2NUM(bi * 8 + bit));
                b &= b - 1;
            }
        }
#endif
    }
    else {
        /* lsb_first: false => byte order preserved, bits 7..0 map to logical 0..7 */
        for (ssize_t bi = 0; bi < len; bi++) {
            unsigned int b = str[bi];
            if (target == 0) b = (~b) & 0xFF;
            while (b != 0) {
                int bit = sb_highest_bit8(b);
                ssize_t physical = bi * 8 + bit;
                rb_yield(SSIZET2NUM(physical_to_count_from(physical, 0)));
                b ^= (1u << bit);  /* clear highest set bit */
            }
        }
    }

    return self;
}

static VALUE
rb_str_bit_offsets(int argc, VALUE *argv, VALUE self)
{
    VALUE bit_val, opts;
    rb_scan_args(argc, argv, "10:", &bit_val, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);
    int target    = parse_bit_target(bit_val);

    ssize_t len = RSTRING_LEN(self);
    const unsigned char *str = (const unsigned char *)RSTRING_PTR(self);
    int have_block = rb_block_given_p();

    VALUE ary;
    if (have_block) {
        ary = Qnil;
    }
    else {
        /* Pre-size the Array with popcount to avoid repeated reallocation.
         * For target=0 the count is (len * 8 - popcount).
         * memcpy avoids unaligned-load issues on strict-alignment platforms. */
        ssize_t set_count = 0;
        ssize_t nw = len >> 3;
        for (ssize_t wi = 0; wi < nw; wi++) {
            uint64_t w;
            memcpy(&w, str + wi * 8, 8);
            set_count += sb_popcount64(w);
        }
        for (ssize_t bi = nw << 3; bi < len; bi++)
            set_count += sb_popcount64((uint64_t)(unsigned char)str[bi]);
        ssize_t count = (target == 1) ? set_count : (len * 8 - set_count);
        ary = rb_ary_new_capa(count);
    }

    if (lsb_first) {
#if SB_LITTLE_ENDIAN
        ssize_t n_words = len >> 3;
        for (ssize_t wi = 0; wi < n_words; wi++) {
            uint64_t w;
            memcpy(&w, str + wi * 8, 8);
            if (target == 0) w = ~w;
            while (w != 0) {
                int bit = sb_ctzll(w);
                VALUE pos = SSIZET2NUM(wi * 64 + bit);
                have_block ? rb_yield(pos) : rb_ary_push(ary, pos);
                w &= w - 1;
            }
        }
        for (ssize_t bi = n_words << 3; bi < len; bi++) {
            unsigned int b = str[bi];
            if (target == 0) b = (~b) & 0xFF;
            while (b != 0) {
                int bit = sb_ctz8(b);
                VALUE pos = SSIZET2NUM(bi * 8 + bit);
                have_block ? rb_yield(pos) : rb_ary_push(ary, pos);
                b &= b - 1;
            }
        }
#else
        for (ssize_t bi = 0; bi < len; bi++) {
            unsigned int b = str[bi];
            if (target == 0) b = (~b) & 0xFF;
            while (b != 0) {
                int bit = sb_ctz8(b);
                VALUE pos = SSIZET2NUM(bi * 8 + bit);
                have_block ? rb_yield(pos) : rb_ary_push(ary, pos);
                b &= b - 1;
            }
        }
#endif
    }
    else {
        for (ssize_t bi = 0; bi < len; bi++) {
            unsigned int b = str[bi];
            if (target == 0) b = (~b) & 0xFF;
            while (b != 0) {
                int bit = sb_highest_bit8(b);
                ssize_t physical = bi * 8 + bit;
                VALUE pos = SSIZET2NUM(physical_to_count_from(physical, 0));
                have_block ? rb_yield(pos) : rb_ary_push(ary, pos);
                b ^= (1u << bit);
            }
        }
    }

    return have_block ? self : ary;
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

/* String#bit_slice(bit_offset, bit_length) -> String
 * String#bit_slice(range) -> String
 *
 *   str = "\xFF\x00" # 11111111 00000000
 *   str.bit_slice(4, 8) # => "\xF0" (11110000)
 */
static VALUE
rb_str_bit_slice(int argc, VALUE *argv, VALUE self)
{
    ssize_t src_len = RSTRING_LEN(self);
    ssize_t total_bits = src_len * 8;
    ssize_t offset, length;
    VALUE v0, v1, opts;
    int n_pos = rb_scan_args(argc, argv, "11:", &v0, &v1, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);

    if (n_pos == 1 && rb_obj_is_kind_of(v0, rb_cRange)) {
        ssize_t beg, len;
        if (!RTEST(sb_range_beg_len(v0, &beg, &len, total_bits, 0))) {
            return Qnil;
        }
        offset = beg;
        length = len;
    }
    else if (n_pos == 2) {
        if (!rb_integer_type_p(v0) || !rb_integer_type_p(v1)) {
            return Qnil;
        }

        offset = integer_to_bit_idx(v0);
        length = integer_to_bit_idx(v1);

        if (offset < 0 || length < 0) return Qnil;
    }
    else if (n_pos == 1) {
        return Qnil;
    }
    else {
        rb_raise(rb_eArgError,
                 "wrong number of arguments (given %d, expected 1 or 2)", n_pos);
    }

    if (offset > total_bits) return Qnil;
    ssize_t available = total_bits - offset;
    if (length > available) length = available;

    if (length == 0) return rb_str_new("", 0);

    ssize_t out_bytes = (length + 7) / 8;
    VALUE result = rb_str_buf_new(out_bytes);
    rb_str_resize(result, out_bytes);
    rb_enc_associate(result, rb_enc_get(self));
    unsigned char *dst = (unsigned char *)RSTRING_PTR(result);
    const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);

    memset(dst, 0, out_bytes);

    if (lsb_first) {
        bit_copy_core(dst, 0, src, src_len, offset, length);
    } else {
        ssize_t dst_bit = 0;
        ssize_t start_byte = offset >> 3;
        ssize_t end_byte = (offset + length - 1) >> 3;

        for (ssize_t b = start_byte; b <= end_byte; b++) {
            ssize_t b_start_l = b << 3;
            ssize_t b_end_l = b_start_l + 7;
            ssize_t l_min = (offset > b_start_l) ? offset : b_start_l;
            ssize_t l_max = ((offset + length - 1) < b_end_l) ? (offset + length - 1) : b_end_l;

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
    VALUE target, opts;
    rb_scan_args(argc, argv, "1:", &target, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);

    rb_str_modify(self);
    unsigned char *ptr = (unsigned char *)RSTRING_PTR(self);

    if (rb_integer_type_p(target)) {
        ssize_t idx = check_bit_index(self, target, lsb_first);
        unsigned char mask = (unsigned char)(1u << (idx % 8));
        switch (op) {
          case SB_MUT_SET:   ptr[idx / 8] |= mask; break;
          case SB_MUT_CLEAR: ptr[idx / 8] &= (unsigned char)~mask; break;
          case SB_MUT_FLIP:  ptr[idx / 8] ^= mask; break;
        }
        return self;
    }

    if (rb_obj_is_kind_of(target, rb_cRange)) {
        ssize_t total_bits = RSTRING_LEN(self) * 8;
        ssize_t beg, len;

        /* err=0 returns Qnil for out-of-range begin (after negative normalization);
         * convert that to IndexError to stay consistent with single-bit access. */
        if (!RTEST(sb_range_beg_len(target, &beg, &len, total_bits, 0))) {
            rb_raise(rb_eIndexError, "bit range out of range");
        }

        /* err=0 silently clamps end > total. Detect that and raise instead,
         * to stay consistent with bit_splice and single-bit mutation. */
        VALUE rng_end = rb_funcall(target, rb_intern("end"), 0);
        if (!NIL_P(rng_end)) {
            ssize_t end_val = integer_to_bit_idx(rng_end);
            if (end_val < 0) end_val += total_bits;
            int exclusive = RTEST(rb_funcall(target, rb_intern("exclude_end?"), 0));
            ssize_t end_excl = exclusive ? end_val : end_val + 1;
            if (end_excl > total_bits) {
                rb_raise(rb_eIndexError, "bit range out of range");
            }
        }

        for (ssize_t logical = beg; logical < beg + len; logical++) {
            ssize_t idx = lsb_first ? logical : ((logical & ~7L) | (7 - (logical & 7L)));
            unsigned char mask = (unsigned char)(1u << (idx % 8));
            switch (op) {
              case SB_MUT_SET:   ptr[idx / 8] |= mask; break;
              case SB_MUT_CLEAR: ptr[idx / 8] &= (unsigned char)~mask; break;
              case SB_MUT_FLIP:  ptr[idx / 8] ^= mask; break;
            }
        }
        return self;
    }

    rb_raise(rb_eTypeError, "bit index must be an integer or Range");
    UNREACHABLE_RETURN(Qnil);
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

static VALUE
alloc_result(VALUE self)
{
    ssize_t len = RSTRING_LEN(self);
    VALUE result = rb_str_buf_new(len);
    rb_str_resize(result, len);
    rb_enc_associate(result, rb_enc_get(self));
    return result;
}

static VALUE
rb_str_bitwise_not(VALUE self)
{
    ssize_t len = RSTRING_LEN(self);
    VALUE result = alloc_result(self);
    const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
    unsigned char *dst = (unsigned char *)RSTRING_PTR(result);
    for (ssize_t i = 0; i < len; i++) dst[i] = ~src[i];
    return result;
}

static VALUE
rb_str_bitwise_not_bang(VALUE self)
{
    rb_str_modify(self);
    ssize_t len = RSTRING_LEN(self);
    unsigned char *ptr = (unsigned char *)RSTRING_PTR(self);
    for (ssize_t i = 0; i < len; i++) ptr[i] = ~ptr[i];
    return self;
}

static VALUE
rb_str_bitwise_and(VALUE self, VALUE other)
{
    check_binary_op_lengths(self, other);
    ssize_t len = RSTRING_LEN(self);
    VALUE result = alloc_result(self);
    const unsigned char *a = (const unsigned char *)RSTRING_PTR(self);
    const unsigned char *b = (const unsigned char *)RSTRING_PTR(other);
    unsigned char *dst = (unsigned char *)RSTRING_PTR(result);
    for (ssize_t i = 0; i < len; i++) dst[i] = a[i] & b[i];
    return result;
}

static VALUE
rb_str_bitwise_and_bang(VALUE self, VALUE other)
{
    check_binary_op_lengths(self, other);
    rb_str_modify(self);
    ssize_t len = RSTRING_LEN(self);
    unsigned char *a = (unsigned char *)RSTRING_PTR(self);
    const unsigned char *b = (const unsigned char *)RSTRING_PTR(other);
    for (ssize_t i = 0; i < len; i++) a[i] &= b[i];
    return self;
}

static VALUE
rb_str_bitwise_or(VALUE self, VALUE other)
{
    check_binary_op_lengths(self, other);
    ssize_t len = RSTRING_LEN(self);
    VALUE result = alloc_result(self);
    const unsigned char *a = (const unsigned char *)RSTRING_PTR(self);
    const unsigned char *b = (const unsigned char *)RSTRING_PTR(other);
    unsigned char *dst = (unsigned char *)RSTRING_PTR(result);
    for (ssize_t i = 0; i < len; i++) dst[i] = a[i] | b[i];
    return result;
}

static VALUE
rb_str_bitwise_or_bang(VALUE self, VALUE other)
{
    check_binary_op_lengths(self, other);
    rb_str_modify(self);
    ssize_t len = RSTRING_LEN(self);
    unsigned char *a = (unsigned char *)RSTRING_PTR(self);
    const unsigned char *b = (const unsigned char *)RSTRING_PTR(other);
    for (ssize_t i = 0; i < len; i++) a[i] |= b[i];
    return self;
}

static VALUE
rb_str_bitwise_xor(VALUE self, VALUE other)
{
    check_binary_op_lengths(self, other);
    ssize_t len = RSTRING_LEN(self);
    VALUE result = alloc_result(self);
    const unsigned char *a = (const unsigned char *)RSTRING_PTR(self);
    const unsigned char *b = (const unsigned char *)RSTRING_PTR(other);
    unsigned char *dst = (unsigned char *)RSTRING_PTR(result);
    for (ssize_t i = 0; i < len; i++) dst[i] = a[i] ^ b[i];
    return result;
}

static VALUE
rb_str_bitwise_xor_bang(VALUE self, VALUE other)
{
    check_binary_op_lengths(self, other);
    rb_str_modify(self);
    ssize_t len = RSTRING_LEN(self);
    unsigned char *a = (unsigned char *)RSTRING_PTR(self);
    const unsigned char *b = (const unsigned char *)RSTRING_PTR(other);
    for (ssize_t i = 0; i < len; i++) a[i] ^= b[i];
    return self;
}

/* packed bit-field iteration ---------------------------------------------- */
/*
 * NOTE: each_bit_field and bit_fields are implemented here and fully tested,
 * but are NOT part of the current core proposal (see FUTURE_PROPOSAL_PLAN.md).
 * They are deferred because yielding Integer field values is a qualitatively
 * different contract from the rest of the API, and that difference is expected
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

/* String#each_bit_field(*bitlens, lsb_first: true) -> self
 * String#each_bit_field(*bitlens, lsb_first: true) -> Enumerator
 *
 * Iterates over the string as a sequence of packed bit-field records. Each
 * positional argument specifies the width (in bits) of one field in the record.
 * On each iteration, one Integer per field is yielded (LSB-first bit layout).
 * Each bitlen must be in the range 1..64.
 *
 * lsb_first:   true (default)  -- intra-byte field extraction uses bit 0..7.
 * lsb_first:   false           -- intra-byte field extraction uses bit 7..0.
 *
 * Incomplete trailing bits (when bytesize*8 is not a multiple of sum(bitlens))
 * are silently dropped, matching the behavior of Enumerable#each_slice.
 *
 * Porting to Ruby Core:
 *   1. Move extract_uint64 and this function into string.c.
 *   2. Register with rb_define_method in Init_String().
 *   3. Replace ALLOCA_N with stack arrays for small field counts and heap otherwise.
 */
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
            rb_raise(rb_eArgError, "bitlen must be <= 64 (got %ld)", bl);
        }
        bitlens[f] = bl;
        step += bl;
    }

    int lsb_first = parse_lsb_first_opt(opts);

    ssize_t src_len = RSTRING_LEN(self);
    ssize_t total_bits = src_len * 8;
    ssize_t iterations = total_bits / step;

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

/* String#bit_fields(*bitlens, lsb_first: true) -> Array
 * String#bit_fields(*bitlens, lsb_first: true) { |*fields| } -> self
 *
 * Non-iterator complement of each_bit_field.  Without a block, returns an
 * Array of all extracted records.  With a single bitlen the array is flat
 * (matching each_bit_field(n).to_a); with multiple bitlens each record is
 * itself an Array (matching each_bit_field(a, b, ...).to_a).
 *
 * With a block, behaves identically to each_bit_field without with: ---
 * yielding one Integer per field and returning self.
 *
 * Porting to Ruby Core:
 *   1. Move alongside each_bit_field in string.c.
 *   2. Share extract_uint64 and the bitlen validation logic.
 *   3. Register with rb_define_method in Init_String().
 */
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
            rb_raise(rb_eArgError, "bitlen must be <= 64 (got %ld)", bl);
        }
        bitlens[f] = bl;
        step += bl;
    }

    int lsb_first = parse_lsb_first_opt(opts);

    ssize_t src_len = RSTRING_LEN(self);
    ssize_t total_bits = src_len * 8;
    ssize_t iterations = total_bits / step;

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
 * position `pos` (LSB-first).  Uses ctz / ctzll to skip bits in bulk:
 *   - partial first byte: ctz on the inverted masked nibble
 *   - full 64-bit words (LE): ctzll on the inverted word (64 bits per step)
 *   - remaining bytes: ctz on the inverted byte
 *
 * Porting to Ruby Core:
 *   1. Move to string.c alongside bit_at and each_bit.
 *   2. Share sb_ctz8 / sb_ctzll with the existing set-bit helpers.
 */
static ssize_t
count_run_lsb(const unsigned char *src, ssize_t src_len, ssize_t pos, int target)
{
    ssize_t max_run  = src_len * 8 - pos;
    ssize_t byte_idx = pos >> 3;
    int  bit_off  = pos & 7;
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
            return count < max_run ? count : max_run;
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
            return count < max_run ? count : max_run;
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
            return count < max_run ? count : max_run;
        }
    }

    return count < max_run ? count : max_run;
}

/* String#bit_run_count(pos, bit) -> Integer | nil
 *
 * Returns the length of the consecutive run of `bit` starting at flat
 * position `pos`.  Returns nil when `pos` is out of range or the bit at `pos`
 * does not equal `bit`.
 *
 * `bit` accepts 0, 1, false, or true (false/true are aliases for 0/1,
 * matching the values yielded by each_bit_run).
 *
 * Counts forward from `pos` toward higher bit indices.
 *
 * Inspired by Gauche Scheme's (bitvector-count-run bit bvec i).
 *
 * Uses the same flat LSB-first addressing as bit_at: byte[pos/8] bit pos%8.
 *
 * Porting to Ruby Core:
 *   1. Move to string.c; register in Init_String().
 *   2. Reuse integer_to_bit_idx for consistent Bignum handling.
 */
static VALUE
rb_str_bit_run_count(int argc, VALUE *argv, VALUE self)
{
    VALUE pos_val, bit_val, opts;
    rb_scan_args(argc, argv, "20:", &pos_val, &bit_val, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);

    if (!rb_integer_type_p(pos_val)) {
        rb_raise(rb_eTypeError, "position must be an integer");
    }
    int target;
    if (bit_val == Qtrue || bit_val == INT2FIX(1)) {
        target = 1;
    } else if (bit_val == Qfalse || bit_val == INT2FIX(0)) {
        target = 0;
    } else {
        rb_raise(rb_eArgError, "bit must be 0, 1, false, or true");
    }
    ssize_t pos     = integer_to_bit_idx(pos_val);
    ssize_t src_len = RSTRING_LEN(self);
    if (pos < 0 || pos >= src_len * 8) return Qnil;

    const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
    if (lsb_first) {
        if (((src[pos >> 3] >> (pos & 7)) & 1) != target) return Qnil;
        return SSIZET2NUM(count_run_lsb(src, src_len, pos, target));
    }

    if (logical_get_bit(src, pos, 0) != target) return Qnil;

    ssize_t run = 1;
    ssize_t total_bits = src_len * 8;
    while (pos + run < total_bits && logical_get_bit(src, pos + run, 0) == target) {
        run++;
    }
    return SSIZET2NUM(run);
}

/* String#each_bit_run(lsb_first: true) { |bit, len| } -> self
 * String#each_bit_run(lsb_first: true) -> Enumerator
 *
 * Yields (bit, run_length) pairs for each consecutive run of identical bits.
 * Run-length boundary detection and counting happen entirely in C, replacing
 * the Ruby-level current/count state machine required when using each_bit.
 *
 * For random data (~50% density) each_bit_run yields ~half as many times as
 * each_bit.  For structured data (sparse validity bitmaps, sensor bursts) the
 * ratio is proportional to the average run length.
 *
 * lsb_first: true (default) iterates bit 0..7 within each byte.
 * lsb_first: false iterates bit 7..0 within each byte.
 *
 * Porting to Ruby Core:
 *   1. Move to string.c; register in Init_String().
 *   2. count_run_lsb / count_run_msb move with it.
 */
static VALUE
rb_str_each_bit_run(int argc, VALUE *argv, VALUE self)
{
    RETURN_ENUMERATOR(self, argc, argv);

    int lsb_first = parse_lsb_first(argc, argv);
    ssize_t src_len  = RSTRING_LEN(self);
    if (src_len == 0) return self;

    ssize_t total_bits = src_len * 8;

    if (lsb_first) {
        ssize_t pos = 0;
        while (pos < total_bits) {
            const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
            int bit  = (src[pos >> 3] >> (pos & 7)) & 1;
            ssize_t run = count_run_lsb(src, src_len, pos, bit);
            rb_yield_values(2, bit ? Qtrue : Qfalse, SSIZET2NUM(run));
            pos += run;
        }
    }
    else {
        ssize_t pos = 0;
        while (pos < total_bits) {
            const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
            int bit = logical_get_bit(src, pos, 0);
            ssize_t run = 1;
            while (pos + run < total_bits && logical_get_bit(src, pos + run, 0) == bit) {
                run++;
            }
            rb_yield_values(2, bit ? Qtrue : Qfalse, SSIZET2NUM(run));
            pos += run;
        }
    }

    return self;
}

/* String#bit_runs(lsb_first: true) -> Array
 * String#bit_runs(lsb_first: true) { |bit, len| } -> self
 *
 * Non-iterator complement of each_bit_run. Without a block, collects all
 * (bit, run_length) pairs into an Array and returns it. With a block,
 * yields each pair and returns self.
 *
 * Follows the same pattern as String#bytes vs String#each_byte.
 *
 * Porting to Ruby Core:
 *   1. Move to string.c alongside each_bit_run; register in Init_String().
 */
static VALUE
rb_str_bit_runs(int argc, VALUE *argv, VALUE self)
{
    int lsb_first = parse_lsb_first(argc, argv);
    ssize_t src_len   = RSTRING_LEN(self);
    int have_block = rb_block_given_p();

    if (src_len == 0) return have_block ? self : rb_ary_new();

    ssize_t total_bits = src_len * 8;
    VALUE result    = have_block ? Qnil : rb_ary_new();

    if (lsb_first) {
        ssize_t pos = 0;
        while (pos < total_bits) {
            const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
            int bit  = (src[pos >> 3] >> (pos & 7)) & 1;
            ssize_t run = count_run_lsb(src, src_len, pos, bit);
            VALUE bval = bit ? Qtrue : Qfalse;
            VALUE lval = SSIZET2NUM(run);
            have_block ? rb_yield_values(2, bval, lval)
                       : rb_ary_push(result, rb_assoc_new(bval, lval));
            pos += run;
        }
    } else {
        ssize_t pos = 0;
        while (pos < total_bits) {
            const unsigned char *src = (const unsigned char *)RSTRING_PTR(self);
            int bit = logical_get_bit(src, pos, 0);
            ssize_t run = 1;
            while (pos + run < total_bits && logical_get_bit(src, pos + run, 0) == bit) {
                run++;
            }
            VALUE bval = bit ? Qtrue : Qfalse;
            VALUE lval = SSIZET2NUM(run);
            have_block ? rb_yield_values(2, bval, lval)
                       : rb_ary_push(result, rb_assoc_new(bval, lval));
            pos += run;
        }
    }

    return have_block ? self : result;
}

/* String#bit_splice(bit_index, bit_length, str) -> self
 * String#bit_splice(bit_index, bit_length, str, str_bit_index, str_bit_length) -> self
 * String#bit_splice(range, str) -> self
 * String#bit_splice(range, str, str_range) -> self
 *
 * Writes bits from str into self at bit-level granularity.  The inverse of
 * bit_slice: where bit_slice reads a sub-sequence of bits, bit_splice writes one.
 *
 * The destination and source bit lengths must be equal; bit_splice does not
 * resize self (sub-byte resize is undefined).  This mirrors the constraint that
 * bytesplice imposes when the replacement has the same byte length.
 *
 * Negative indices count backward from the end, exactly as in bytesplice.
 * Returns self.
 *
 * Porting to Ruby Core:
 *   1. Move to string.c; register in Init_String().
 *   2. Use rb_str_modify_expand if resize support is ever added.
 *   3. bit_copy_core moves with it; share ebs_extract with bit_slice.
 */
static VALUE
rb_str_bit_splice(int argc, VALUE *argv, VALUE self)
{
    ssize_t dst_bit_off, dst_bit_len;
    ssize_t src_bit_off, src_bit_len;
    VALUE str;
    ssize_t dst_total = RSTRING_LEN(self) * 8;
    VALUE v0, v1, v2, v3, v4, opts;

    int n_pos = rb_scan_args(argc, argv, "23:", &v0, &v1, &v2, &v3, &v4, &opts);
    validate_option_hash(opts, SB_KW_LSB_FIRST);
    int lsb_first = parse_lsb_first_opt(opts);

    if (n_pos == 2 && rb_obj_is_kind_of(v0, rb_cRange)) {
        /* bit_splice(range, str) */
        ssize_t beg, len;
        sb_range_beg_len(v0, &beg, &len, dst_total, 1);
        dst_bit_off = beg;
        dst_bit_len = len;
        str = v1;
        Check_Type(str, T_STRING);
        src_bit_off = 0;
        src_bit_len = dst_bit_len;
    }
    else if (n_pos == 3 && rb_obj_is_kind_of(v0, rb_cRange)) {
        /* bit_splice(range, str, str_range) */
        ssize_t beg, len;
        sb_range_beg_len(v0, &beg, &len, dst_total, 1);
        dst_bit_off = beg;
        dst_bit_len = len;
        str = v1;
        Check_Type(str, T_STRING);
        if (!rb_obj_is_kind_of(v2, rb_cRange)) {
            rb_raise(rb_eTypeError, "third argument must be a Range");
        }
        ssize_t src_total = RSTRING_LEN(str) * 8;
        sb_range_beg_len(v2, &beg, &len, src_total, 1);
        src_bit_off = beg;
        src_bit_len = len;
    }
    else if (n_pos == 3) {
        /* bit_splice(bit_index, bit_length, str) */
        if (!rb_integer_type_p(v0) || !rb_integer_type_p(v1)) {
            rb_raise(rb_eTypeError, "bit index and length must be integers");
        }
        dst_bit_off = integer_to_bit_idx(v0);
        dst_bit_len = integer_to_bit_idx(v1);
        if (dst_bit_off < 0) dst_bit_off += dst_total;

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
        src_bit_off = 0;
        src_bit_len = dst_bit_len;
    }
    else if (n_pos == 5) {
        /* bit_splice(bit_index, bit_length, str, str_bit_index, str_bit_length) */
        if (!rb_integer_type_p(v0) || !rb_integer_type_p(v1) ||
            !rb_integer_type_p(v3) || !rb_integer_type_p(v4)) {
            rb_raise(rb_eTypeError, "bit indices and lengths must be integers");
        }
        dst_bit_off = integer_to_bit_idx(v0);
        dst_bit_len = integer_to_bit_idx(v1);
        if (dst_bit_off < 0) dst_bit_off += dst_total;
        str = v2;
        Check_Type(str, T_STRING);
        ssize_t src_total = RSTRING_LEN(str) * 8;
        src_bit_off = integer_to_bit_idx(v3);
        src_bit_len = integer_to_bit_idx(v4);
        if (src_bit_off < 0) src_bit_off += src_total;
    }
    else {
        rb_raise(rb_eArgError,
                 "wrong number of arguments (given %d, expected 2, 3, or 5)", n_pos);
    }

    if (dst_bit_off < 0 || dst_bit_len < 0 || dst_bit_off + dst_bit_len > dst_total) {
        rb_raise(rb_eIndexError,
                 "bit_splice: destination range [%ld, %ld] out of bounds (total %ld bits)",
                 dst_bit_off, dst_bit_len, dst_total);
    }

    ssize_t src_total_bits = RSTRING_LEN(str) * 8;
    if (src_bit_off < 0 || src_bit_len < 0 || src_bit_off + src_bit_len > src_total_bits) {
        rb_raise(rb_eIndexError,
                 "bit_splice: source range [%ld, %ld] out of bounds (total %ld bits)",
                 src_bit_off, src_bit_len, src_total_bits);
    }

    if (dst_bit_len != src_bit_len) {
        rb_raise(rb_eArgError,
                 "bit_splice: destination length (%ld) must equal source length (%ld)",
                 dst_bit_len, src_bit_len);
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
                     "bitmap too short: need %ld bytes for %ld elements, got %ld",
                     needed, ary_len, bmp_len);

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
                     "bitmap too short: need %ld bytes for %ld elements, got %ld",
                     needed, ary_len, bmp_len);

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
    id_bracket = rb_intern("[]");
    sym_lsb_first   = ID2SYM(rb_intern("lsb_first"));
    sym_lsb         = ID2SYM(rb_intern("lsb"));
    sym_msb         = ID2SYM(rb_intern("msb"));
    sym_invert      = ID2SYM(rb_intern("invert"));

    rb_define_method(rb_cString, "bit_at",            rb_str_bit_at,           -1);
    rb_define_method(rb_cString, "bit_count",         rb_str_bit_count,         0);
    rb_define_method(rb_cString, "each_bit",          rb_str_each_bit,         -1);
    rb_define_method(rb_cString, "bits",              rb_str_bits,             -1);
    rb_define_method(rb_cString, "each_bit_offset",   rb_str_each_bit_offset,  -1);
    rb_define_method(rb_cString, "bit_offsets",       rb_str_bit_offsets,      -1);
    rb_define_method(rb_cString, "bit_slice",         rb_str_bit_slice,        -1);
    rb_define_method(rb_cString, "bit_splice",        rb_str_bit_splice,       -1);
    rb_define_method(rb_cString, "bit_run_count",     rb_str_bit_run_count,    -1);
    rb_define_method(rb_cString, "each_bit_run",      rb_str_each_bit_run,     -1);
    rb_define_method(rb_cString, "bit_runs",          rb_str_bit_runs,         -1);
    rb_define_method(rb_cString, "bit_set",           rb_str_bit_set,          -1);
    rb_define_method(rb_cString, "bit_clear",         rb_str_bit_clear,        -1);
    rb_define_method(rb_cString, "bit_flip",          rb_str_bit_flip,         -1);
    rb_define_method(rb_cString, "bitwise_not",           rb_str_bitwise_not,           0);
    rb_define_method(rb_cString, "bitwise_not!",          rb_str_bitwise_not_bang,      0);
    rb_define_method(rb_cString, "bitwise_and",           rb_str_bitwise_and,           1);
    rb_define_method(rb_cString, "bitwise_and!",          rb_str_bitwise_and_bang,      1);
    rb_define_method(rb_cString, "bitwise_or",            rb_str_bitwise_or,            1);
    rb_define_method(rb_cString, "bitwise_or!",           rb_str_bitwise_or_bang,       1);
    rb_define_method(rb_cString, "bitwise_xor",           rb_str_bitwise_xor,           1);
    rb_define_method(rb_cString, "bitwise_xor!",          rb_str_bitwise_xor_bang,      1);

    // These methods are defined here to avoid cluttering this file, but they are not part of the current core proposal (see FUTURE_PROPOSAL_PLAN.md).
    rb_define_method(rb_cString, "each_bit_field",    rb_str_each_bit_field,   -1);
    rb_define_method(rb_cString, "bit_fields",        rb_str_bit_fields,       -1);
    rb_define_method(rb_cArray,  "mask",              rb_ary_mask,             -1);
    rb_define_method(rb_cArray,  "mask!",             rb_ary_mask_bang,        -1);
}
