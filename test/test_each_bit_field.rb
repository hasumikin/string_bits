# NOTE: each_bit_field is NOT part of the current core proposal.
# It is deferred pending discussion on yielding Integer field values.
# See FUTURE_PROPOSAL_PLAN.md for the rationale.
require_relative "test_helper"

class TestEachBitField < Minitest::Test
  # --- Enumerator / return value ---

  def test_returns_enumerator_without_block
    assert_instance_of Enumerator, "\xAA\xCC".each_bit_field(8)
  end

  def test_returns_enumerator_with_options
    assert_instance_of Enumerator, "\xAA\xCC".each_bit_field(8, 8)
    assert_instance_of Enumerator, "\xAA\xCC".each_bit_field(8, lsb_first: false)
  end

  def test_returns_self_with_block
    s = "\xAA\xCC"
    assert_same s, s.each_bit_field(8) { |_| }
  end

  # --- Yields Integer values ---

  def test_yields_integers
    "\xAA\xCC".each_bit_field(8) { |v| assert_instance_of Integer, v }
  end

  def test_yields_integers_multi_field
    "\xAA\xCC".each_bit_field(8, 8) { |a, b| assert_instance_of Integer, a; assert_instance_of Integer, b }
  end

  # --- Single field, byte-aligned ---

  def test_byte_aligned_single_field
    assert_equal [0xAA, 0xCC], "\xAA\xCC".each_bit_field(8).to_a
  end

  def test_four_byte_string
    assert_equal [0x01, 0x02, 0x03, 0x04], "\x01\x02\x03\x04".each_bit_field(8).to_a
  end

  # --- Non-byte-aligned single field ---

  def test_4bit_single_field
    # 0xAB = bits 0-3: 1011 = 0x0B; bits 4-7: 1010 = 0x0A
    assert_equal [0x0B, 0x0A], "\xAB".each_bit_field(4).to_a
  end

  def test_extracted_bits_match_bit_set_p
    data = "\xAA\xCC\xFF\x00"
    bitlen = 6
    data.each_bit_field(bitlen).with_index do |val, iter|
      bitlen.times do |j|
        expected = data.bit_set?(iter * bitlen + j) ? 1 : 0
        assert_equal expected, (val >> j) & 1, "iteration #{iter}, bit #{j}"
      end
    end
  end

  def test_12bit_single_field_bits_match
    data = "\xAA\xCC\xFF"
    data.each_bit_field(12).with_index do |val, iter|
      12.times do |j|
        expected = data.bit_set?(iter * 12 + j) ? 1 : 0
        assert_equal expected, (val >> j) & 1, "iteration #{iter}, bit #{j}"
      end
    end
  end

  # --- Trailing bits dropped ---

  def test_trailing_bits_dropped
    assert_equal 2, "\xAA\xCC\xFF".each_bit_field(10).to_a.length
  end

  def test_string_shorter_than_bitlen_yields_nothing
    assert_empty "\xAA".each_bit_field(16).to_a
  end

  def test_empty_string_yields_nothing
    assert_empty "".each_bit_field(8).to_a
  end

  # --- Multiple fields (equal widths) ---

  def test_two_equal_fields_byte_aligned
    pairs = []
    "\xAA\xCC".each_bit_field(8, 8) { |a, b| pairs << [a, b] }
    assert_equal [[0xAA, 0xCC]], pairs
  end

  def test_three_equal_fields
    groups = []
    "\xAA\xBB\xCC".each_bit_field(8, 8, 8) { |a, b, c| groups << [a, b, c] }
    assert_equal [[0xAA, 0xBB, 0xCC]], groups
  end

  def test_two_equal_fields_multiple_iterations
    groups = []
    "\xAA\xBB\xCC\xDD".each_bit_field(8, 8) { |a, b| groups << [a, b] }
    assert_equal [[0xAA, 0xBB], [0xCC, 0xDD]], groups
  end

  def test_two_12bit_fields_bits_match
    data = "\xAA\xCC\xFF"
    data.each_bit_field(12, 12) do |ch0, ch1|
      12.times do |j|
        assert_equal (data.bit_set?(j)      ? 1 : 0), (ch0 >> j) & 1, "ch0 bit #{j}"
        assert_equal (data.bit_set?(12 + j) ? 1 : 0), (ch1 >> j) & 1, "ch1 bit #{j}"
      end
    end
  end

  # --- Multiple fields (mixed widths) ---

  def test_mixed_width_fields_565
    # RGB565: blue=21 (5-bit), green=42 (6-bit), red=10 (5-bit)
    pixel = [(21) | (42 << 5) | (10 << 11)].pack("S<")
    fields = []
    pixel.each_bit_field(5, 6, 5) { |b, g, r| fields << [b, g, r] }
    assert_equal [[21, 42, 10]], fields
  end

  def test_mixed_width_fields_bits_match
    data = "\xAA\xCC\xFF"
    data.each_bit_field(5, 6, 5) do |f0, f1, f2|
      5.times { |j| assert_equal (data.bit_set?(j)      ? 1 : 0), (f0 >> j) & 1, "f0 bit #{j}" }
      6.times { |j| assert_equal (data.bit_set?(5 + j)  ? 1 : 0), (f1 >> j) & 1, "f1 bit #{j}" }
      5.times { |j| assert_equal (data.bit_set?(11 + j) ? 1 : 0), (f2 >> j) & 1, "f2 bit #{j}" }
    end
  end

  # --- 64-bit field limit ---

  def test_64bit_field
    data = "\xFF" * 8
    assert_equal [0xFFFFFFFFFFFFFFFF], data.each_bit_field(64).to_a
  end

  def test_argument_error_for_bitlen_over_64
    assert_raises(ArgumentError) { ("\xFF" * 9).each_bit_field(65) {} }
    assert_raises(ArgumentError) { "\xFF".each_bit_field(4, 65) {} }
  end

  # --- lsb_first: false ---

  def test_msb_passes_byte_through_for_byte_aligned_fields
    # MSB-first scan of a whole byte, packed MSB-first as Integer,
    # reproduces the byte value itself.
    data = "\x96\x3C"
    assert_equal [0x96, 0x3C], data.each_bit_field(8, lsb_first: false).to_a
  end

  def test_msb_sub_byte_fields_split_at_msb_side
    # "\xF0" = 0b11110000.  MSB-first split into 4+4:
    #   field 0 = top nibble (0b1111 = 0xF)
    #   field 1 = bottom nibble (0b0000 = 0)
    assert_equal [[0xF, 0]], "\xF0".each_bit_field(4, 4, lsb_first: false).to_a
  end

  def test_msb_extracted_bits_match_bit_set_p
    data = "\x96\x3C\xA5\x5A"
    bitlen = 6
    data.each_bit_field(bitlen, lsb_first: false).with_index do |val, iter|
      bitlen.times do |j|
        # MSB-first packing: the j-th bit collected from the buffer
        # is at integer bit position (bitlen-1-j).
        expected = data.bit_set?(iter * bitlen + j, lsb_first: false) ? 1 : 0
        assert_equal expected, (val >> (bitlen - 1 - j)) & 1, "iteration #{iter}, bit #{j}"
      end
    end
  end

  def test_msb_op_enter_like_record
    # mruby OP_ENTER operand layout (24 bits = 1:5:5:1:5:5:1:1, big-endian).
    # noblock=0, req=3, opt=2, rest=0, post=1, key=0, kdict=1, block=1:
    # a = (3 << 18) | (2 << 13) | (1 << 7) | (1 << 1) | 1
    a = (3 << 18) | (2 << 13) | (1 << 7) | (1 << 1) | 1
    operand = [(a >> 16) & 0xFF, (a >> 8) & 0xFF, a & 0xFF].pack("C*")
    fields = operand.each_bit_field(1, 5, 5, 1, 5, 5, 1, 1, lsb_first: false).to_a
    assert_equal [[0, 3, 2, 0, 1, 0, 1, 1]], fields
  end

  def test_single_record_block_decode
    # data width == record width: block runs exactly once, parameters bind directly to fields.
    a = (3 << 18) | (2 << 13) | (1 << 7) | (1 << 1) | 1
    operand = [(a >> 16) & 0xFF, (a >> 8) & 0xFF, a & 0xFF].pack("C*")
    result = []
    operand.each_bit_field(1, 5, 5, 1, 5, 5, 1, 1, lsb_first: false) do |noblock, req, opt, rest, post, key_count, kdict, block|
      result = [noblock, req, opt, rest, post, key_count, kdict, block]
    end
    assert_equal [0, 3, 2, 0, 1, 0, 1, 1], result
  end

  # --- Error handling ---

  def test_argument_error_for_no_fields
    assert_raises(ArgumentError) { "\xAA".each_bit_field {} }
  end

  def test_type_error_for_non_integer_bitlen
    assert_raises(TypeError) { "\xAA".each_bit_field("8") {} }
    assert_raises(TypeError) { "\xAA".each_bit_field(nil) {} }
    assert_raises(TypeError) { "\xAA".each_bit_field(4, "2") {} }
  end

  def test_argument_error_for_zero_bitlen
    assert_raises(ArgumentError) { "\xAA".each_bit_field(0) {} }
    assert_raises(ArgumentError) { "\xAA".each_bit_field(4, 0) {} }
  end

  def test_argument_error_for_negative_bitlen
    assert_raises(ArgumentError) { "\xAA".each_bit_field(-1) {} }
    assert_raises(ArgumentError) { "\xAA".each_bit_field(4, -1) {} }
  end

  def test_argument_error_for_invalid_lsb_first
    assert_raises(ArgumentError) { "\xAA".each_bit_field(4, lsb_first: :foo) {} }
  end

  def test_argument_error_for_unknown_keyword
    err = assert_raises(ArgumentError) { "\xAA".each_bit_field(4, count_from: :lsb).to_a }
    assert_match(/unknown keyword/, err.message)
  end

  def test_field_order_is_unknown_keyword
    err = assert_raises(ArgumentError) { "\xAA".each_bit_field(4, field_order: :msb).to_a }
    assert_match(/unknown keyword/, err.message)
  end

  # --- Enumerator behavior ---

  def test_enumerator_to_a_single_field
    assert_equal [0xAA, 0xCC], "\xAA\xCC".each_bit_field(8).to_a
  end

  def test_enumerator_to_a_two_fields
    assert_equal [[0xAA, 0xCC]], "\xAA\xCC".each_bit_field(8, 8).to_a
  end

  def test_enumerator_count
    assert_equal 4, "\xAA\xCC\xFF\x00".each_bit_field(8).count
  end
end
