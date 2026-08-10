require_relative "test_helper"

class TestBitOffsets < Minitest::Test
  def setup
    @data = [0b10101010, 0b11001100].pack('C*')
  end

  def test_returns_array_without_block_set
    assert_instance_of Array, @data.bit_offsets(1)
    assert_instance_of Array, @data.bit_offsets(1, lsb_first: true)
  end

  def test_returns_array_without_block_unset
    assert_instance_of Array, @data.bit_offsets(0)
    assert_instance_of Array, @data.bit_offsets(0, lsb_first: true)
  end

  def test_array_content_lsb_set
    assert_equal [1, 3, 5, 7, 10, 11, 14, 15], @data.bit_offsets(1)
  end

  def test_array_content_msb_set
    assert_equal [0, 2, 4, 6, 8, 9, 12, 13], @data.bit_offsets(1, lsb_first: false)
  end

  def test_array_content_lsb_unset
    assert_equal [0, 2, 4, 6, 8, 9, 12, 13], @data.bit_offsets(0)
  end

  def test_array_content_msb_unset
    assert_equal [1, 3, 5, 7, 10, 11, 14, 15], @data.bit_offsets(0, lsb_first: false)
  end

  def test_accepts_boolean_bit_aliases
    assert_equal @data.bit_offsets(1), @data.bit_offsets(true)
    assert_equal @data.bit_offsets(0), @data.bit_offsets(false)
  end

  def test_with_block_yields_same_as_each_bit_offset
    collected_block = []
    collected_each  = []
    @data.bit_offsets(1)      { |n| collected_block << n }
    @data.each_bit_offset(1)  { |n| collected_each  << n }
    assert_equal collected_each, collected_block

    collected_block = []
    collected_each  = []
    @data.bit_offsets(0)     { |n| collected_block << n }
    @data.each_bit_offset(0) { |n| collected_each  << n }
    assert_equal collected_each, collected_block
  end

  def test_with_block_returns_self
    assert_same @data, @data.bit_offsets(1) {}
    assert_same @data, @data.bit_offsets(0) {}
  end

  def test_bit_offsets_equals_each_bit_offset_to_a
    assert_equal @data.each_bit_offset(1).to_a,  @data.bit_offsets(1)
    assert_equal @data.each_bit_offset(0).to_a, @data.bit_offsets(0)
    assert_equal @data.each_bit_offset(1,  lsb_first: false).to_a, @data.bit_offsets(1,  lsb_first: false)
    assert_equal @data.each_bit_offset(0, lsb_first: false).to_a, @data.bit_offsets(0, lsb_first: false)
  end

  def test_empty_string
    assert_equal [], "".bit_offsets(1)
    assert_equal [], "".bit_offsets(0)
  end

  # --- bit_offset ---

  def test_bit_offset_skips_leading_bits
    assert_equal [10, 11, 14, 15], @data.bit_offsets(1, 8)
  end

  def test_bit_offset_non_byte_aligned
    assert_equal [4, 5, 6, 7], "\xF0".bit_offsets(1, 4)
  end

  def test_bit_offset_zero_same_as_default
    assert_equal @data.bit_offsets(1), @data.bit_offsets(1, 0)
  end

  def test_bit_offset_at_total_bits_returns_empty
    assert_empty "\xFF".bit_offsets(1, 8)
  end

  def test_bit_offset_msb
    assert_empty "\xF0".bit_offsets(1, 4, lsb_first: false)
    assert_equal [4, 5, 6, 7], "\xF0".bit_offsets(0, 4, lsb_first: false)
  end

  def test_bit_offset_negative_raises_index_error
    assert_raises(IndexError) { "\xFF".bit_offsets(1, -1) }
  end

  def test_bit_offset_beyond_string_is_empty
    assert_empty "\xFF".bit_offsets(1, 2**62)
  end

  def test_unrepresentable_bit_offset_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bit_offsets(1, 2**64) }
  end

  def test_bit_offset_matches_each_bit_offset
    assert_equal @data.each_bit_offset(1, 8).to_a,  @data.bit_offsets(1, 8)
    assert_equal @data.each_bit_offset(0, 8).to_a, @data.bit_offsets(0, 8)
  end
end
