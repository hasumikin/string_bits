require_relative "test_helper"

class TestBits < Minitest::Test
  def test_returns_array_without_block
    assert_instance_of Array, "\xAA".bits
    assert_instance_of Array, "\xAA".bits(lsb_first: false)
  end

  def test_array_content_lsb
    assert_equal [false, true, false, true, false, true, false, true], "\xAA".bits
  end

  def test_array_content_msb
    assert_equal [true, false, true, false, true, false, true, false], "\xAA".bits(lsb_first: false)
  end

  def test_with_block_yields_same_as_each_bit
    collected_bits = []
    collected_each = []
    "\xAA\xCC".bits     { |b| collected_bits << b }
    "\xAA\xCC".each_bit { |b| collected_each << b }
    assert_equal collected_each, collected_bits
  end

  def test_with_block_returns_self
    s = "\xAA"
    assert_same s, s.bits {}
    assert_same s, s.bits(lsb_first: false) {}
  end

  def test_bits_equals_each_bit_to_a
    data = [0b10101010, 0b11001100].pack('C*')
    assert_equal data.each_bit.to_a,              data.bits
    assert_equal data.each_bit(lsb_first: false).to_a, data.bits(lsb_first: false)
  end

  def test_empty_string
    assert_equal [], "".bits
  end

  # --- bit_offset ---

  def test_bit_offset_byte_aligned
    assert_equal "\xAA".bits, "\xFF\xAA".bits(8)
  end

  def test_bit_offset_non_byte_aligned
    # "\xF0" lsb: bits 0-3 are 0, bits 4-7 are 1; starting at 4 yields all-true
    assert_equal [true, true, true, true], "\xF0".bits(4)
  end

  def test_bit_offset_zero_same_as_default
    assert_equal "\xFF\xAA".bits, "\xFF\xAA".bits(0)
  end

  def test_bit_offset_at_total_bits_returns_empty
    assert_empty "\xFF".bits(8)
  end

  def test_bit_offset_beyond_total_bits_returns_empty
    assert_empty "\xFF".bits(100)
  end

  def test_bit_offset_msb
    assert_equal [false, false, false, false], "\xF0".bits(4, lsb_first: false)
  end

  def test_bit_offset_negative_raises_index_error
    assert_raises(IndexError) { "\xFF".bits(-1) }
  end

  def test_bit_offset_beyond_string_is_empty
    assert_empty "\xFF".bits(2**62)
  end

  def test_unrepresentable_bit_offset_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bits(2**64) }
  end

  def test_bit_offset_matches_each_bit_offset
    assert_equal "\xFF\xAA".each_bit(8).to_a, "\xFF\xAA".bits(8)
    assert_equal "\xF0".each_bit(4, lsb_first: false).to_a, "\xF0".bits(4, lsb_first: false)
  end
end
