require_relative "test_helper"

class TestBitCount < Minitest::Test
  def test_empty_string
    assert_equal 0, "".bit_count
  end

  def test_all_zeros
    assert_equal 0, "\x00\x00\x00".bit_count
  end

  def test_all_ones
    assert_equal 8,  "\xFF".bit_count
    assert_equal 24, "\xFF\xFF\xFF".bit_count
  end

  def test_single_byte
    assert_equal 4, "\xAA".bit_count  # 0b10101010
    assert_equal 4, "\x0F".bit_count  # 0b00001111
    assert_equal 1, "\x01".bit_count
    assert_equal 1, "\x80".bit_count
  end

  def test_multi_byte
    # 0xAA=4 bits, 0xCC=4 bits
    assert_equal 8, [0xAA, 0xCC].pack('C*').bit_count
  end

  def test_matches_each_bit_count
    data = [0b10110111, 0b00101101, 0b11000001].pack('C*')
    expected = data.each_bit.count(1)
    assert_equal expected, data.bit_count
  end

  def test_bit_count_is_byte_size_times_8_for_all_ones
    data = "\xFF" * 10
    assert_equal 80, data.bit_count
  end

  def test_lsb_first_only_is_ignored
    # lsb_first: alone with no positional args counts the whole string
    assert_equal "\xAA".bit_count, "\xAA".bit_count(lsb_first: true)
    assert_equal "\xAA".bit_count, "\xAA".bit_count(lsb_first: false)
  end

  # --- start, length form ---

  def test_start_length_full_byte
    assert_equal 8, "\xFF".bit_count(0, 8)
    assert_equal 0, "\x00".bit_count(0, 8)
    assert_equal 4, "\xAA".bit_count(0, 8)
  end

  def test_start_length_second_byte
    assert_equal 8, "\x00\xFF".bit_count(8, 8)
    assert_equal 0, "\xFF\x00".bit_count(8, 8)
  end

  def test_start_length_partial_byte_lsb
    # "\xF0" = 11110000 lsb: bits 0-3 are 0, bits 4-7 are 1
    assert_equal 0, "\xF0".bit_count(0, 4)
    assert_equal 4, "\xF0".bit_count(4, 4)
  end

  def test_start_length_cross_byte
    # bits 4..11 span byte boundary
    assert_equal 8, "\xFF\xFF".bit_count(4, 8)
    assert_equal 0, "\x00\x00".bit_count(4, 8)
  end

  def test_start_length_zero_length
    assert_equal 0, "\xFF".bit_count(0, 0)
  end

  def test_start_length_clamps_to_end
    # start+length exceeds string, result counts only available bits
    assert_equal 4, "\xFF".bit_count(4, 100)
  end

  def test_start_at_total_bits_returns_zero
    assert_equal 0, "\xFF".bit_count(8, 8)
  end

  def test_start_beyond_total_bits_returns_zero
    assert_equal 0, "\xFF".bit_count(100, 8)
  end

  def test_start_length_msb
    # "\xF0" msb: bits 0-3 are 1 (high nibble), bits 4-7 are 0
    assert_equal 4, "\xF0".bit_count(0, 4, lsb_first: false)
    assert_equal 0, "\xF0".bit_count(4, 4, lsb_first: false)
  end

  def test_start_length_negative_start_raises_index_error
    assert_raises(IndexError) { "\xFF".bit_count(-1, 8) }
  end

  def test_start_length_start_beyond_string_counts_zero
    assert_equal 0, "\xFF".bit_count(2**62, 8)
    assert_equal 0, "\xFF".bit_count(2**64 - 1, 8)
  end

  def test_start_length_unrepresentable_start_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bit_count(2**64, 8) }
  end

  def test_start_length_negative_length_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bit_count(0, -1) }
  end

  def test_start_length_length_beyond_string_is_clamped
    assert_equal 8, "\xFF".bit_count(0, 2**62)
    assert_equal 8, "\xFF".bit_count(0, 2**64 - 1)
  end

  def test_start_length_unrepresentable_length_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bit_count(0, 2**64) }
  end

  def test_start_length_type_error_on_start
    assert_raises(TypeError) { "\xFF".bit_count("0", 8) }
  end

  def test_start_length_type_error_on_length
    assert_raises(TypeError) { "\xFF".bit_count(0, "8") }
  end

  def test_start_length_consistent_with_no_arg
    data = "\xAA\xCC\xFF\x00"
    assert_equal data.bit_count, data.bit_count(0, data.bytesize * 8)
  end

  # --- range form ---

  def test_range_full_byte
    assert_equal 8, "\xFF".bit_count(0..7)
    assert_equal 0, "\x00".bit_count(0..7)
  end

  def test_range_exclusive
    assert_equal 8, "\xFF".bit_count(0...8)
    assert_equal 4, "\xFF".bit_count(0...4)
  end

  def test_range_partial_byte
    # "\xF0" lsb: bits 4-7 are set
    assert_equal 0, "\xF0".bit_count(0..3)
    assert_equal 4, "\xF0".bit_count(4..7)
  end

  def test_range_cross_byte
    assert_equal 8, "\xFF\xFF".bit_count(4..11)
    assert_equal 0, "\x00\x00".bit_count(4..11)
  end

  def test_range_out_of_bounds_returns_zero
    assert_equal 0, "\xFF".bit_count(8..15)
    assert_equal 0, "\xFF".bit_count(100..107)
  end

  def test_range_clamps_to_end
    assert_equal 4, "\xFF".bit_count(4..100)
  end

  def test_range_msb
    assert_equal 4, "\xF0".bit_count(0..3, lsb_first: false)
    assert_equal 0, "\xF0".bit_count(4..7, lsb_first: false)
  end

  def test_range_negative_endpoint_raises_index_error
    assert_raises(IndexError) { "\xFF".bit_count(-1..7) }
    assert_raises(IndexError) { "\xFF".bit_count(0..-1) }
  end

  def test_range_endpoint_beyond_string_is_clamped
    assert_equal 8, "\xFF".bit_count(0..2**62)
    assert_equal 0, "\xFF".bit_count(2**62..100)
  end

  def test_range_unrepresentable_endpoint_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bit_count(0..2**64) }
    assert_raises(ArgumentError) { "\xFF".bit_count((2**64)..(2**64 + 4)) }
  end

  def test_range_consistent_with_no_arg
    data = "\xAA\xCC\xFF\x00"
    assert_equal data.bit_count, data.bit_count(0..(data.bytesize * 8 - 1))
  end

  def test_range_consistent_with_start_length
    data = "\xAA\xCC\xFF\x00\xF0"
    assert_equal data.bit_count(4, 24), data.bit_count(4..27)
    assert_equal data.bit_count(4, 24), data.bit_count(4...28)
  end
end
