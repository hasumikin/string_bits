require_relative "test_helper"

class TestBitSlice < Minitest::Test
  def test_byte_aligned_full
    assert_equal "\xFF\xAA", "\xFF\xAA".bit_slice(0, 16)
  end

  def test_byte_aligned_first_byte
    assert_equal "\xFF", "\xFF\xAA".bit_slice(0, 8)
  end

  def test_byte_aligned_second_byte
    assert_equal "\xAA", "\xFF\xAA".bit_slice(8, 8)
  end

  def test_crossing_byte_boundary
    # bits 4-11: bits 4-7 from 0xFF, bits 8-11 from 0xAA=0b10101010
    # result bits 0-3 = 1,1,1,1; bits 4-7 = 0,1,0,1 => 0b10101111 = 0xAF
    assert_equal "\xAF", "\xFF\xAA".bit_slice(4, 8)
  end

  def test_range_inclusive_first_byte
    assert_equal "\xFF", "\xFF\xAA".bit_slice(0..7)
  end

  def test_range_exclusive_first_byte
    assert_equal "\xFF", "\xFF\xAA".bit_slice(0...8)
  end

  def test_range_crossing_byte_boundary
    assert_equal "\xAF", "\xFF\xAA".bit_slice(4..11)
  end

  def test_negative_range_counts_from_end
    assert_equal "\xAA", "\xFF\xAA".bit_slice(-8..-1)
  end

  def test_endless_range_runs_to_end
    assert_equal "\xAA", "\xFF\xAA".bit_slice(8..)
  end

  def test_beginless_range_starts_from_zero
    assert_equal "\xFF\x0A", "\xFF\xAA".bit_slice(..11)
  end

  def test_nil_nil_range_means_whole_bitmap
    assert_equal "\xFF\xAA", "\xFF\xAA".bit_slice(nil..nil)
  end

  def test_length_zero_returns_empty_string
    assert_equal "", "\xFF".bit_slice(0, 0)
  end

  def test_offset_at_total_bits_returns_empty_string
    assert_equal "", "\xFF".bit_slice(8, 0)
  end

  def test_offset_beyond_range_returns_nil
    assert_nil "\xFF".bit_slice(9, 1)
  end

  def test_range_beyond_range_returns_nil
    assert_nil "\xFF".bit_slice(9..10)
  end

  def test_negative_offset_returns_nil
    assert_nil "\xFF".bit_slice(-1, 4)
  end

  def test_negative_length_returns_nil
    assert_nil "\xFF".bit_slice(0, -1)
  end

  def test_length_clamped_to_available_bits
    # bits 12-15 of "\xFF\xAA": only 4 bits; 0xAA bits 4-7 = 0,1,0,1 => 0b00001010 = 0x0A
    assert_equal "\x0A", "\xFF\xAA".bit_slice(12, 8)
  end

  def test_sub_byte_extraction
    # first 4 bits of "\xAA" (0b10101010): bits 0,1,2,3 = 0,1,0,1 => 0b00001010 = 0x0A
    assert_equal "\x0A", "\xAA".bit_slice(0, 4)
  end

  def test_unused_high_bits_of_last_byte_are_zeroed
    result = "\xFF\xFF".bit_slice(0, 9)
    assert_equal 2, result.bytesize
    assert_equal "\xFF\x01", result
  end

  def test_returns_string_instance
    assert_instance_of String, "\xFF".bit_slice(0, 4)
  end

  def test_roundtrip_bits_match_bit_at
    data = "\xAA\xCC\xFF"
    result = data.bit_slice(4, 12)
    12.times do |i|
      assert_equal data.bit_at(4 + i), result.bit_at(i), "bit #{i} mismatch"
    end
  end

  def test_does_not_modify_original
    data = +"\xFF\xAA"
    data.bit_slice(0, 8)
    assert_equal "\xFF\xAA", data
  end

  def test_empty_string_offset_zero_returns_empty
    assert_equal "", "".bit_slice(0, 0)
  end

  def test_non_integer_offset_returns_nil
    assert_nil "\xFF".bit_slice("0", 4)
    assert_nil "\xFF".bit_slice(0.5, 4)
    assert_nil "\xFF".bit_slice(nil, 4)
    assert_nil "\xFF".bit_slice(:foo, 4)
  end

  def test_non_integer_length_returns_nil
    assert_nil "\xFF".bit_slice(0, "4")
    assert_nil "\xFF".bit_slice(0, 0.5)
    assert_nil "\xFF".bit_slice(0, nil)
    assert_nil "\xFF".bit_slice(0, :foo)
  end

  def test_non_range_single_argument_returns_nil
    assert_nil "\xFF".bit_slice("0")
    assert_nil "\xFF".bit_slice(nil)
    assert_nil "\xFF".bit_slice(:foo)
  end

  def test_bignum_offset_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bit_slice(2**62, 4) }
    assert_raises(ArgumentError) { "\xFF".bit_slice(2**63, 4) }
  end

  def test_bignum_length_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bit_slice(0, 2**63) }
  end

  def test_msb_positions_return_physically_preserved_result
    # 0xAC = 1010 1100.
    # lsb_first: false index 0-3 is physical b7-b4 (1010).
    # Result b0-b3 becomes 1010 => 0x0A.
    assert_equal "\x0A", "\xAC".bit_slice(0, 4, lsb_first: false)
    # lsb_first: false index 4-7 is physical b3-b0 (1100).
    # Result b0-b3 becomes 1100 => 0x0C.
    assert_equal "\x0C", "\xAC".bit_slice(4, 4, lsb_first: false)
  end

  def test_msb_range_roundtrip_with_bit_at
    data = "\x96\x3C"
    # lsb_first: false index 3 is physical b4 of byte 0.
    # range 3, length 7 spans physical b4..b0 (byte 0) and b7..b5 (byte 1).
    result = data.bit_slice(3, 7, lsb_first: false)
    7.times do |i|
      # i=0 of result should map to the first physical bit touched by the range.
      # Which physical bit?
      # logical 3..7 are physical b4, b3, b2, b1, b0. (Smallest physical is 0).
      # logical 8..9 are physical b15, b14. (Wait, length 7 means 3..9).
      # logical 8, 9 are physical b15, b14.
      # Total physical bits touched: 0, 1, 2, 3, 4 (byte 0) and 14, 15 (byte 1).
      # In physical ascending order: 0, 1, 2, 3, 4, 14, 15.
      # These go to result physical bits 0, 1, 2, 3, 4, 5, 6.
      src_physical = [0, 1, 2, 3, 4, 14, 15][i]
      assert_equal data.bit_at(src_physical), result.bit_at(i), "bit #{i} mismatch"
    end
  end

  def test_unknown_keyword_raises_argument_error
    err = assert_raises(ArgumentError) { "\xFF".bit_slice(0, 4, reverse: true) }
    assert_match(/unknown keyword/, err.message)
  end

end
