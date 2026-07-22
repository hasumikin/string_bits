require_relative "test_helper"

class TestSetClearFlipBit < Minitest::Test
  def test_set_bit_on_zero_byte
    s = +"\x00"
    s.bit_set(0)
    assert_equal "\x01", s
    s.bit_set(7)
    assert_equal "\x81", s
  end

  def test_set_bit_is_idempotent
    s = +"\xFF"
    s.bit_set(3)
    assert_equal "\xFF", s
  end

  def test_set_bit_only_affects_target_bit
    s = +"\x00\x00"
    s.bit_set(8)  # LSB of byte[1]
    assert_equal "\x00\x01", s
  end

  def test_set_bit_returns_self
    s = +"\x00"
    assert_same s, s.bit_set(0)
  end

  def test_set_bit_range_inclusive
    s = +"\x00\x00"
    s.bit_set(0..7)
    assert_equal "\xFF\x00", s
  end

  def test_set_bit_range_exclusive
    s = +"\x00\x00"
    s.bit_set(0...7)
    assert_equal "\x7F\x00", s
  end

  def test_set_bit_range_endless
    s = +"\x00\x00"
    s.bit_set(8..)
    assert_equal "\x00\xFF", s
  end

  def test_set_bit_range_beginless
    s = +"\x00\x00"
    s.bit_set(..7)
    assert_equal "\xFF\x00", s
  end

  def test_set_bit_range_nil_nil
    s = +"\x00\x00"
    s.bit_set(nil..nil)
    assert_equal "\xFF\xFF", s
  end

  def test_set_bit_out_of_range_raises
    s = +"\x00"
    assert_raises(IndexError) { s.dup.bit_set(8) }
    assert_raises(IndexError) { s.dup.bit_set(-1) }
  end

  def test_clear_bit
    s = +"\xFF"
    s.bit_clear(0)
    assert_equal "\xFE", s
    s.bit_clear(7)
    assert_equal "\x7E", s
  end

  def test_clear_bit_is_idempotent
    s = +"\x00"
    s.bit_clear(3)
    assert_equal "\x00", s
  end

  def test_clear_bit_only_affects_target_bit
    s = +"\xFF\xFF"
    s.bit_clear(8)  # LSB of byte[1]
    assert_equal "\xFF\xFE", s
  end

  def test_clear_bit_returns_self
    s = +"\xFF"
    assert_same s, s.bit_clear(0)
  end

  def test_clear_bit_range
    s = +"\xFF\xFF"
    s.bit_clear(4..11)
    assert_equal "\x0F\xF0", s
  end

  def test_clear_bit_out_of_range_raises
    s = +"\xFF"
    assert_raises(IndexError) { s.dup.bit_clear(8) }
  end

  def test_flip_bit_zero_to_one
    s = +"\x00"
    s.bit_flip(3)
    assert_equal "\x08", s
  end

  def test_flip_bit_one_to_zero
    s = +"\xFF"
    s.bit_flip(3)
    assert_equal "\xF7", s
  end

  def test_flip_bit_twice_is_identity
    s = +"\xAA"
    original = s.dup
    s.bit_flip(0).bit_flip(0)
    assert_equal original, s
  end

  def test_flip_bit_returns_self
    s = +"\x00"
    assert_same s, s.bit_flip(0)
  end

  def test_flip_bit_range
    s = +"\x00\x00"
    s.bit_flip(4..11)
    assert_equal "\xF0\x0F", s
  end

  def test_set_clear_roundtrip_consistent_with_bit_set_p
    s = "\x00" * 2
    [0, 1, 7, 8, 15].each do |n|
      s.bit_set(n)
      assert_equal true, s.bit_set?(n), "bit_set?(#{n}) should be true after bit_set"
      s.bit_clear(n)
      assert_equal false, s.bit_set?(n), "bit_set?(#{n}) should be false after bit_clear"
    end
  end

  def test_build_arrow_bitmap_incrementally
    # Simulate building a validity bitmap: elements 0,2,4 are valid
    bitmap = +"\x00"
    [0, 2, 4].each { |i| bitmap.bit_set(i) }
    assert_equal true,  bitmap.bit_set?(0)
    assert_equal false, bitmap.bit_set?(1)
    assert_equal true,  bitmap.bit_set?(2)
    assert_equal false, bitmap.bit_set?(3)
    assert_equal true,  bitmap.bit_set?(4)
    assert_equal 3, bitmap.bit_count
  end

  def test_set_bit_non_integer_raises_type_error
    s = +"\xFF"
    assert_raises(TypeError) { s.bit_set("0") }
    assert_raises(TypeError) { s.bit_set(nil) }
    assert_raises(TypeError) { s.bit_set(:foo) }
  end

  def test_clear_bit_non_integer_raises_type_error
    s = +"\xFF"
    assert_raises(TypeError) { s.bit_clear("0") }
    assert_raises(TypeError) { s.bit_clear(nil) }
    assert_raises(TypeError) { s.bit_clear(:foo) }
  end

  def test_flip_bit_non_integer_raises_type_error
    s = +"\xFF"
    assert_raises(TypeError) { s.bit_flip("0") }
    assert_raises(TypeError) { s.bit_flip(nil) }
    assert_raises(TypeError) { s.bit_flip(:foo) }
  end

  def test_bit_offset_is_converted_with_to_int
    # The offset goes through to_int, like the index of setbyte, so a Float is
    # truncated rather than rejected.
    s = +"\x00"
    s.bit_set(1.9)
    assert_equal "\x02".b, s.b

    offset = Object.new
    def offset.to_int
      0
    end
    s.bit_set(offset)
    assert_equal "\x03".b, s.b
  end

  def test_set_bit_out_of_range_raises_index_error
    # A Bignum offset is a well-formed position that lies past the end of the
    # string, so it takes the ordinary out-of-range path for a mutation.
    assert_raises(IndexError) { (+"\xFF").bit_set(2**62) }
    assert_raises(IndexError) { (+"\xFF").bit_set(2**63) }
    assert_raises(IndexError) { (+"\xFF").bit_clear(2**62) }
    assert_raises(IndexError) { (+"\xFF").bit_flip(2**62) }
  end

  def test_unrepresentable_bit_offset_raises_argument_error
    # Offsets that do not fit in 64 bits cannot address any byte buffer.
    assert_raises(ArgumentError) { (+"\xFF").bit_set(2**100) }
    assert_raises(ArgumentError) { (+"\xFF").bit_clear(2**100) }
    assert_raises(ArgumentError) { (+"\xFF").bit_flip(2**100) }
  end

  def test_negative_bignum_raises_index_error
    assert_raises(IndexError) { (+"\xFF").bit_set(-(2**100)) }
  end

  def test_lsb_first_must_be_true_or_false
    assert_raises(ArgumentError) { (+"\xFF").bit_set(0, lsb_first: nil) }
    assert_raises(ArgumentError) { (+"\xFF").bit_clear(0, lsb_first: 1) }
  end

  def test_lsb_first
    # "\x00\x00"
    s = +"\x00\x00"

    # bit_set with lsb_first: false (byte order preserved, numbering reversed within each byte)
    s.bit_set(0, lsb_first: false) # Physical 7 (bit 7 of s[0])
    assert_equal "\x80\x00", s

    s.bit_set(8, lsb_first: false) # Physical 15 (bit 7 of s[1])
    assert_equal "\x80\x80", s

    # bit_clear with lsb_first: false
    s.bit_clear(0, lsb_first: false)
    assert_equal "\x00\x80", s

    # bit_flip with lsb_first: false
    s.bit_flip(15, lsb_first: false) # Physical 8 (bit 0 of s[1])
    assert_equal "\x00\x81", s
  end

  def test_lsb_first_false_range_uses_logical_positions
    s = +"\x00\x00"
    s.bit_set(6..9, lsb_first: false)
    assert_equal "\x03\xC0", s
  end

  def test_empty_range_is_noop
    s = +"\x00"
    assert_same s, s.bit_set(8..)
    assert_equal "\x00", s
  end

  def test_out_of_range_range_raises_index_error
    assert_raises(IndexError) { (+"\x00").bit_set(9..) }
    assert_raises(IndexError) { (+"\x00").bit_clear(9..10) }
  end

  def test_range_with_end_past_total_raises_index_error
    # Explicit end past total must raise (no silent clipping).
    assert_raises(IndexError) { (+"\x00").bit_set(0..100) }
    assert_raises(IndexError) { (+"\x00").bit_set(8..15) }
    assert_raises(IndexError) { (+"\x00").bit_set(8..8) }
    assert_raises(IndexError) { (+"\x00").bit_clear(0..100) }
    assert_raises(IndexError) { (+"\x00").bit_flip(0..100) }
  end

  def test_range_on_empty_string
    # Non-empty range on an empty string is out-of-range.
    assert_raises(IndexError) { (+"").bit_set(0..7) }
    # Empty range on an empty string is a no-op.
    s = +""
    assert_same s, s.bit_set(0...0)
    assert_equal "", s
  end

  def test_range_beyond_string_raises_index_error
    # A write is never allowed to silently shrink, so a range reaching past
    # the end is an IndexError rather than a clamped write.
    assert_raises(IndexError) { (+"\x00").bit_set((2**62)..(2**62 + 4)) }
    assert_raises(IndexError) { (+"\x00").bit_set(0..(2**62)) }
    assert_raises(IndexError) { (+"\x00").bit_clear((2**62)..(2**62 + 4)) }
  end

  def test_range_unrepresentable_endpoint_raises_argument_error
    assert_raises(ArgumentError) { (+"\x00").bit_flip(0..(2**100)) }
    assert_raises(ArgumentError) { (+"\x00").bit_set((2**64)..(2**64 + 4)) }
  end

  def test_range_exclusive_endless
    s = +"\x00"
    s.bit_set(0...)
    assert_equal "\xFF", s
  end

  def test_range_negative_endpoints_raise_index_error
    assert_raises(IndexError) { (+"\x00").bit_set(..-1) }
    assert_raises(IndexError) { (+"\x00").bit_set(-1..-1) }
    assert_raises(IndexError) { (+"\x00").bit_clear(-8..-1) }
    assert_raises(IndexError) { (+"\x00").bit_flip(-1..) }
  end

  def test_range_lsb_first_false_end_past_total_raises
    assert_raises(IndexError) { (+"\x00").bit_set(0..100, lsb_first: false) }
  end

  def test_lsb_first_negative_raises_index_error
    assert_raises(IndexError) { (+"\x00").bit_set(-1, lsb_first: false) }
    assert_raises(IndexError) { (+"\xFF").bit_clear(-1, lsb_first: false) }
    assert_raises(IndexError) { (+"\x00").bit_flip(-1, lsb_first: false) }
  end

  # --- 2-arg (bit_offset, bit_length) form ---

  def test_set_two_arg_equals_range
    s1 = +"\x00\x00"; s1.bit_set(4, 8)
    s2 = +"\x00\x00"; s2.bit_set(4..11)
    assert_equal s2, s1
  end

  def test_clear_two_arg_equals_range
    s1 = +"\xFF\xFF"; s1.bit_clear(4, 8)
    s2 = +"\xFF\xFF"; s2.bit_clear(4..11)
    assert_equal s2, s1
  end

  def test_flip_two_arg_equals_range
    s1 = +"\x00\x00"; s1.bit_flip(4, 8)
    s2 = +"\x00\x00"; s2.bit_flip(4..11)
    assert_equal s2, s1
  end

  def test_two_arg_length_one_equals_single_bit
    s1 = +"\x00"; s1.bit_set(3, 1)
    s2 = +"\x00"; s2.bit_set(3)
    assert_equal s2, s1
  end

  def test_two_arg_length_zero_is_noop
    s = +"\xAA"
    s.bit_set(0, 0)
    assert_equal "\xAA", s
    s.bit_clear(0, 0)
    assert_equal "\xAA", s
    s.bit_flip(0, 0)
    assert_equal "\xAA", s
  end

  def test_two_arg_lsb_first_false
    s1 = +"\x00\x00"; s1.bit_set(0, 4, lsb_first: false)
    s2 = +"\x00\x00"; s2.bit_set(0..3, lsb_first: false)
    assert_equal s2, s1
  end

  def test_two_arg_returns_self
    s = +"\x00\x00"
    assert_same s, s.bit_set(0, 8)
    assert_same s, s.bit_clear(0, 8)
    assert_same s, s.bit_flip(0, 8)
  end

  def test_two_arg_cross_byte
    s = +"\x00\x00"
    s.bit_set(4, 8)
    assert_equal "\xF0\x0F", s
  end

  def test_two_arg_out_of_range_raises_index_error
    assert_raises(IndexError) { (+"\x00").bit_set(8, 1) }
    assert_raises(IndexError) { (+"\x00").bit_set(7, 2) }
    assert_raises(IndexError) { (+"\x00").bit_clear(0, 9) }
    assert_raises(IndexError) { (+"\x00").bit_flip(1, 8) }
  end

  def test_two_arg_negative_offset_raises_index_error
    assert_raises(IndexError) { (+"\x00").bit_set(-1, 4) }
  end

  def test_two_arg_negative_length_raises_argument_error
    assert_raises(ArgumentError) { (+"\x00").bit_set(0, -1) }
    assert_raises(ArgumentError) { (+"\xFF").bit_clear(0, -1) }
    assert_raises(ArgumentError) { (+"\x00").bit_flip(0, -1) }
  end

  def test_two_arg_length_beyond_string_raises_index_error
    assert_raises(IndexError) { (+"\x00").bit_set(0, 2**62) }
    assert_raises(IndexError) { (+"\x00").bit_set(2**62, 1) }
  end

  def test_two_arg_unrepresentable_length_raises_argument_error
    assert_raises(ArgumentError) { (+"\x00").bit_set(0, 2**64) }
    assert_raises(ArgumentError) { (+"\x00").bit_set(2**64, 1) }
  end

  def test_two_arg_type_error_on_length
    assert_raises(TypeError) { (+"\x00").bit_set(0, "4") }
  end
end
