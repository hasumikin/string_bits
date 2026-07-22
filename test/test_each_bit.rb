require_relative "test_helper"

class TestEachBit < Minitest::Test
  def test_lsb_single_byte
    bits = []
    "\xAA".each_bit { |b| bits << b }
    assert_equal [false, true, false, true, false, true, false, true], bits
  end

  def test_msb_single_byte
    bits = []
    "\xAA".each_bit(lsb_first: false) { |b| bits << b }
    assert_equal [true, false, true, false, true, false, true, false], bits
  end

  def test_lsb_multi_byte
    bits = []
    [0b10101010, 0b11001100].pack('C*').each_bit { |b| bits << (b ? 1 : 0) }
    assert_equal [0,1,0,1,0,1,0,1, 0,0,1,1,0,0,1,1], bits
  end

  def test_msb_multi_byte
    bits = []
    [0b10101010, 0b11001100].pack('C*').each_bit(lsb_first: false) { |b| bits << (b ? 1 : 0) }
    assert_equal [1,0,1,0,1,0,1,0, 1,1,0,0,1,1,0,0], bits
  end

  def test_empty_string
    bits = []
    "".each_bit { |b| bits << b }
    assert_empty bits
  end

  def test_all_zeros
    bits = []
    "\x00\x00".each_bit { |b| bits << b }
    assert_equal [false] * 16, bits
  end

  def test_all_ones
    bits = []
    "\xFF\xFF".each_bit { |b| bits << b }
    assert_equal [true] * 16, bits
  end

  def test_returns_enumerator_without_block
    assert_instance_of Enumerator, "\xAA".each_bit
    assert_instance_of Enumerator, "\xAA".each_bit(lsb_first: false)
  end

  def test_returns_self_with_block
    s = "\xAA"
    assert_same s, s.each_bit {}
    assert_same s, s.each_bit(lsb_first: false) {}
  end

  def test_enumerator_lsb
    e = "\xAA".each_bit
    assert_equal [false, true, false, true, false, true, false, true], e.to_a
  end

  def test_enumerator_msb
    e = "\xAA".each_bit(lsb_first: false)
    assert_equal [true, false, true, false, true, false, true, false], e.to_a
  end

  def test_enumerator_map
    result = "\xFF".each_bit.map { |b| b ? 1 : 0 }
    assert_equal [1] * 8, result
  end

  def test_msb_changes_intra_byte_order_only
    data = [0b10101010, 0b11001100].pack('C*')
    assert_equal(
      [true, false, true, false, true, false, true, false,
       true, true, false, false, true, true, false, false],
      data.each_bit(lsb_first: false).to_a
    )
  end

  def test_count_set_bit_positions
    count = "\xAA".each_bit.count { |b| b }
    assert_equal 4, count
  end

  def test_unknown_keyword_raises_argument_error
    err = assert_raises(ArgumentError) { "\xAA".each_bit(reverse: true).to_a }
    assert_match(/unknown keyword/, err.message)
  end

  # --- bit_offset ---

  def test_bit_offset_byte_aligned
    assert_equal "\xAA".each_bit.to_a, "\xFF\xAA".each_bit(8).to_a
  end

  def test_bit_offset_non_byte_aligned
    # "\xF0" = 11110000 lsb: bits 0-3 are 0, bits 4-7 are 1; starting at 4 yields all-true
    assert_equal [true, true, true, true], "\xF0".each_bit(4).to_a
  end

  def test_bit_offset_zero_same_as_default
    assert_equal "\xFF\xAA".each_bit.to_a, "\xFF\xAA".each_bit(0).to_a
  end

  def test_bit_offset_at_total_bits_yields_nothing
    assert_empty "\xFF".each_bit(8).to_a
  end

  def test_bit_offset_beyond_total_bits_yields_nothing
    assert_empty "\xFF".each_bit(100).to_a
  end

  def test_bit_offset_enumerator
    assert_instance_of Enumerator, "\xFF".each_bit(4)
  end

  def test_bit_offset_msb
    # "\xF0" MSB-first: positions 0-3 are 1 (high nibble), positions 4-7 are 0; starting at 4 yields all-false
    assert_equal [false, false, false, false], "\xF0".each_bit(4, lsb_first: false).to_a
  end

  def test_bit_offset_negative_raises_index_error
    assert_raises(IndexError) { "\xFF".each_bit(-1).to_a }
  end

  def test_bit_offset_beyond_string_yields_nothing
    # A 64-bit offset is a well-formed position; one past the end simply has
    # no bits to walk.
    assert_empty "\xFF".each_bit(2**62).to_a
    assert_empty "\xFF".each_bit(2**64 - 1).to_a
  end

  def test_unrepresentable_bit_offset_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".each_bit(2**64).to_a }
  end
end
