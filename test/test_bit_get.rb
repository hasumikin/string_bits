require_relative "test_helper"

# bit_get and bit_set? are the same read, differing only in the value they
# return: bit_get answers 1/0 and bit_set? answers true/false. Both answer nil
# for an offset past the end of the string.
#
# Both use the flat/Arrow convention: byte_index = n/8 from the start of the
# string, bit = n%8 counted from the LSB of that byte.
# e.g. "\xAA\xCC": bits 0..7 live in byte[0]=0xAA, bits 8..15 in byte[1]=0xCC
class TestBitGet < Minitest::Test
  def test_single_byte_set_bit_positions
    # 0xAA = 0b10101010: bits 1,3,5,7 are set
    [1, 3, 5, 7].each do |n|
      assert_equal 1,    "\xAA".bit_get(n),  "bit #{n} should be set"
      assert_equal true,  "\xAA".bit_set?(n), "bit #{n} should be set"
    end
    [0, 2, 4, 6].each do |n|
      assert_equal 0,     "\xAA".bit_get(n),  "bit #{n} should be clear"
      assert_equal false, "\xAA".bit_set?(n), "bit #{n} should be clear"
    end
  end

  def test_lsb_is_bit_0
    assert_equal 1, "\x01".bit_get(0)
    assert_equal 0, "\x80".bit_get(0)
    assert_equal true,  "\x01".bit_set?(0)
    assert_equal false, "\x80".bit_set?(0)
  end

  def test_msb_is_bit_7
    assert_equal 1, "\x80".bit_get(7)
    assert_equal 0, "\x01".bit_get(7)
    assert_equal true,  "\x80".bit_set?(7)
    assert_equal false, "\x01".bit_set?(7)
  end

  def test_cross_byte_boundary
    # "\x00\xFF": byte[0]=0x00, byte[1]=0xFF
    data = "\x00\xFF"
    assert_equal 0, data.bit_get(7)   # MSB of byte[0]
    assert_equal 1, data.bit_get(8)   # LSB of byte[1]
    assert_equal 1, data.bit_get(15)  # MSB of byte[1]
    assert_equal false, data.bit_set?(7)
    assert_equal true,  data.bit_set?(8)
    assert_equal true,  data.bit_set?(15)
  end

  def test_arrow_bitmap_convention
    # Arrow element index maps directly to bit_set?: element i -> byte[i/8], bit i%8
    # validity bitmap: elements 0..7 all valid (0xFF), elements 8..15 half null (0xAA)
    bitmap = "\xFF\xAA"
    (0..7).each  { |i| assert_equal true,  bitmap.bit_set?(i), "element #{i} should be valid" }
    [8, 10, 12, 14].each { |i| assert_equal false, bitmap.bit_set?(i), "element #{i} should be null" }
    [9, 11, 13, 15].each { |i| assert_equal true,  bitmap.bit_set?(i), "element #{i} should be valid" }
  end

  def test_out_of_bounds_returns_nil
    assert_nil "\xAA".bit_get(8)
    assert_nil "\xAA".bit_get(100)
    assert_nil "\xAA".bit_set?(8)
    assert_nil "\xAA".bit_set?(100)
  end

  def test_empty_string_returns_nil
    assert_nil "".bit_get(0)
    assert_nil "".bit_set?(0)
  end

  def test_all_zeros
    "\x00".tap do |s|
      8.times do |n|
        assert_equal 0,     s.bit_get(n)
        assert_equal false, s.bit_set?(n)
      end
    end
  end

  def test_all_ones
    "\xFF".tap do |s|
      8.times do |n|
        assert_equal 1,    s.bit_get(n)
        assert_equal true, s.bit_set?(n)
      end
    end
  end

  def test_non_integer_raises_type_error
    s = "\xFF"
    assert_raises(TypeError) { s.bit_get("0") }
    assert_raises(TypeError) { s.bit_get(nil) }
    assert_raises(TypeError) { s.bit_get(:foo) }
    assert_raises(TypeError) { s.bit_set?(:foo) }
  end

  def test_to_int_object_is_accepted
    # The offset is converted with to_int, like the index of setbyte, so a
    # Float is truncated rather than rejected.
    offset = Object.new
    def offset.to_int
      1
    end
    assert_equal 1,    "\xAA".bit_get(offset)
    assert_equal true, "\xAA".bit_set?(offset)
    assert_equal 1,    "\xAA".bit_get(1.9)
  end

  def test_negative_raises_index_error
    assert_raises(IndexError) { "\xFF".bit_get(-1) }
    assert_raises(IndexError) { "\xFF".bit_get(-8) }
    assert_raises(IndexError) { "\xFF".bit_set?(-1) }
  end

  def test_offset_beyond_string_but_representable_returns_nil
    # A Bignum offset is a well-formed position that simply lies past the end
    # of the string, so it follows the ordinary out-of-range rule.
    assert_nil "\xFF".bit_get(2**62)
    assert_nil "\xFF".bit_get(2**64 - 1)
    assert_nil "\xFF".bit_set?(2**62)
  end

  def test_unrepresentable_offset_raises_argument_error
    # Offsets that do not fit in 64 bits cannot address any byte buffer.
    assert_raises(ArgumentError) { "\xFF".bit_get(2**64) }
    assert_raises(ArgumentError) { "\xFF".bit_get(2**100) }
    assert_raises(ArgumentError) { "\xFF".bit_set?(2**100) }
  end

  def test_negative_bignum_raises_index_error
    assert_raises(IndexError) { "\xFF".bit_get(-(2**100)) }
    assert_raises(IndexError) { "\xFF".bit_set?(-(2**100)) }
  end

  def test_lsb_first
    # "\xFF\xAA": byte[0]=0xFF, byte[1]=0xAA (0b10101010)
    data = "\xFF\xAA"

    # LSB order (default)
    assert_equal 1, data.bit_get(0, lsb_first: true) # byte[0] bit 0
    assert_equal 0, data.bit_get(8, lsb_first: true) # byte[1] bit 0

    # lsb_first: false preserves byte order and reverses numbering within each byte
    assert_equal 1, data.bit_get(0, lsb_first: false)  # byte[0] bit 7
    assert_equal 1, data.bit_get(7, lsb_first: false)  # byte[0] bit 0
    assert_equal 1, data.bit_get(8, lsb_first: false)  # byte[1] bit 7
    assert_equal 0, data.bit_get(15, lsb_first: false) # byte[1] bit 0

    assert_equal true,  data.bit_set?(8, lsb_first: false)
    assert_equal false, data.bit_set?(15, lsb_first: false)

    # Out of range
    assert_nil data.bit_get(16, lsb_first: false)
    assert_nil data.bit_set?(16, lsb_first: false)
  end

  def test_lsb_first_negative_raises_index_error
    assert_raises(IndexError) { "\xFF".bit_get(-1, lsb_first: false) }
    assert_raises(IndexError) { "\xFF".bit_set?(-1, lsb_first: false) }
  end

  def test_lsb_first_must_be_true_or_false
    # An explicit nil is a caller mistake, not a request for the default.
    err = assert_raises(ArgumentError) { "\xFF".bit_get(0, lsb_first: nil) }
    assert_match(/lsb_first must be true or false/, err.message)
    assert_raises(ArgumentError) { "\xFF".bit_set?(0, lsb_first: nil) }
    assert_raises(ArgumentError) { "\xFF".bit_get(0, lsb_first: 1) }
  end

  def test_unknown_keyword_raises_argument_error
    err = assert_raises(ArgumentError) { "\xFF".bit_get(0, reverse: false) }
    assert_match(/unknown keyword/, err.message)
    assert_raises(ArgumentError) { "\xFF".bit_set?(0, reverse: false) }
  end
end
