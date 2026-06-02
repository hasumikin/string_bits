require_relative "test_helper"

# The bit-index API uses a fixed-width signed (Fixnum-range) bit index and
# rejects any position outside that range with ArgumentError, on every platform
# (Discussion.md, "Error behavior for out-of-range bit indices"). Internally
# integer_to_bit_idx accepts any Fixnum and raises ArgumentError for any Bignum,
# so the boundary is the platform's Fixnum/Bignum split.
#
# That split is platform-dependent: the largest representable index is the
# largest Fixnum (2**(WORD-2) - 1), so the smallest out-of-range position is
# 2**(WORD-2) -- 2**62 on a 64-bit build, 2**30 on a 32-bit build.
#
# These tests pin that contract on the current platform. They pass on both the
# buggy and a corrected build; their purpose is to guard the spec against a
# mistaken "widen the bit index to 64-bit" fix, which would make a 32-bit build
# silently accept such positions (returning nil / mutating) instead of raising.
class TestIndexRange < Minitest::Test
  WORD = 1.size * 8        # 64 or 32 on this platform
  OVER = 1 << (WORD - 2)   # smallest Bignum: first out-of-representable-range index
  MAX  = OVER - 1          # largest Fixnum: still representable (just out of any 1-byte string)

  def test_boundary_is_at_the_fixnum_bignum_split
    s = "\xFF".b
    # One below the boundary is representable: in-range check applies, so a
    # position past a 1-byte string is a normal out-of-range read (nil),
    # not an ArgumentError.
    assert_nil s.bit_at(MAX)
    # At the boundary the position is outside the representable range.
    assert_raises(ArgumentError) { s.bit_at(OVER) }
  end

  def test_read_methods_raise_argument_error
    s = "\xFF".b
    assert_raises(ArgumentError) { s.bit_at(OVER) }
    assert_raises(ArgumentError) { s.bit_count(OVER, 8) }
    assert_raises(ArgumentError) { s.bit_count(0, OVER) }
    assert_raises(ArgumentError) { s.bit_run_count(true, OVER) }
  end

  def test_iterator_methods_raise_argument_error
    s = "\xFF".b
    assert_raises(ArgumentError) { s.each_bit(OVER) {} }
    assert_raises(ArgumentError) { s.each_bit_offset(true, OVER) {} }
    assert_raises(ArgumentError) { s.each_bit_run(OVER) {} }
  end

  def test_slice_method_raises_argument_error
    s = "\xFF".b
    assert_raises(ArgumentError) { s.bit_slice(OVER, 8) }
    assert_raises(ArgumentError) { s.bit_slice(0, OVER) }
  end

  def test_range_endpoint_out_of_range_raises_argument_error
    s = "\xFF".b
    assert_raises(ArgumentError) { s.bit_count(0..OVER) }
    assert_raises(ArgumentError) { s.bit_slice(0..OVER) }
  end
end
