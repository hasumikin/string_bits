require_relative "test_helper"

class TestBitAt < Minitest::Test
  # bit_at uses flat/Arrow convention: byte_index = n/8 from start, bit = n%8 from LSB
  # e.g. "\xAA\xCC": bit 0..7 live in byte[0]=0xAA, bit 8..15 live in byte[1]=0xCC

  def test_single_byte_set_bit_positions
    # 0xAA = 0b10101010: bits 1,3,5,7 are set
    [1, 3, 5, 7].each { |n| assert_equal true,  "\xAA".bit_at(n), "bit #{n} should be set" }
    [0, 2, 4, 6].each { |n| assert_equal false, "\xAA".bit_at(n), "bit #{n} should be clear" }
  end

  def test_lsb_is_bit_0
    assert_equal true,  "\x01".bit_at(0)
    assert_equal false, "\x80".bit_at(0)
  end

  def test_msb_is_bit_7
    assert_equal true,  "\x80".bit_at(7)
    assert_equal false, "\x01".bit_at(7)
  end

  def test_cross_byte_boundary
    # "\x00\xFF": byte[0]=0x00, byte[1]=0xFF
    data = "\x00\xFF"
    assert_equal false, data.bit_at(7)   # MSB of byte[0]
    assert_equal true,  data.bit_at(8)   # LSB of byte[1]
    assert_equal true,  data.bit_at(15)  # MSB of byte[1]
  end

  def test_arrow_bitmap_convention
    # Arrow element index maps directly to bit_at: element i -> byte[i/8], bit i%8
    # validity bitmap: elements 0..7 all valid (0xFF), elements 8..15 half null (0xAA)
    bitmap = "\xFF\xAA"
    (0..7).each  { |i| assert_equal true,  bitmap.bit_at(i), "element #{i} should be valid" }
    [8, 10, 12, 14].each { |i| assert_equal false, bitmap.bit_at(i), "element #{i} should be null" }
    [9, 11, 13, 15].each { |i| assert_equal true,  bitmap.bit_at(i), "element #{i} should be valid" }
  end

  def test_out_of_bounds_returns_nil
    assert_nil "\xAA".bit_at(8)
    assert_nil "\xAA".bit_at(100)
  end

  def test_empty_string_returns_nil
    assert_nil "".bit_at(0)
  end

  def test_all_zeros
    "\x00".tap { |s| 8.times { |n| assert_equal false, s.bit_at(n) } }
  end

  def test_all_ones
    "\xFF".tap { |s| 8.times { |n| assert_equal true, s.bit_at(n) } }
  end

  def test_non_integer_raises_type_error
    s = "\xFF"
    assert_raises(TypeError) { s.bit_at("0") }
    assert_raises(TypeError) { s.bit_at(0.5) }
    assert_raises(TypeError) { s.bit_at(nil) }
    assert_raises(TypeError) { s.bit_at(:foo) }
  end

  def test_negative_raises_index_error
    assert_raises(IndexError) { "\xFF".bit_at(-1) }
    assert_raises(IndexError) { "\xFF".bit_at(-8) }
  end

  def test_bignum_raises_argument_error
    # Integers outside the supported index range raise ArgumentError.
    assert_raises(ArgumentError) { "\xFF".bit_at(2**62) }
    assert_raises(ArgumentError) { "\xFF".bit_at(2**63) }
    assert_raises(ArgumentError) { "\xFF".bit_at(2**100) }
  end

  def test_negative_bignum_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bit_at(-(2**100)) }
  end

  def test_lsb_first
    # "\xFF\xAA": byte[0]=0xFF, byte[1]=0xAA (0b10101010)
    data = "\xFF\xAA"

    # LSB order (default)
    assert_equal true,  data.bit_at(0, lsb_first: true) # byte[0] bit 0
    assert_equal false, data.bit_at(8, lsb_first: true) # byte[1] bit 0

    # lsb_first: false preserves byte order and reverses numbering within each byte
    assert_equal true,  data.bit_at(0, lsb_first: false)  # byte[0] bit 7
    assert_equal true,  data.bit_at(7, lsb_first: false)  # byte[0] bit 0
    assert_equal true,  data.bit_at(8, lsb_first: false)  # byte[1] bit 7
    assert_equal false, data.bit_at(15, lsb_first: false) # byte[1] bit 0

    # Out of range
    assert_nil data.bit_at(16, lsb_first: false)
  end

  def test_lsb_first_negative_raises_index_error
    assert_raises(IndexError) { "\xFF".bit_at(-1, lsb_first: false) }
  end

  def test_unknown_keyword_raises_argument_error
    err = assert_raises(ArgumentError) { "\xFF".bit_at(0, reverse: false) }
    assert_match(/unknown keyword/, err.message)
  end
end
