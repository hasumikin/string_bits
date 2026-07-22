# encoding: ASCII-8BIT
#
# Every literal here is a byte pattern, and the non-destructive bitwise methods
# return BINARY strings, so the whole file is read as BINARY to keep the
# expected and actual values directly comparable. The encoding contract itself
# is pinned by TestBitwiseEncoding below.

require_relative "test_helper"

class TestBitNot < Minitest::Test
  def test_bit_not_all_zeros
    assert_equal "\xFF", "\x00".bitwise_not
  end

  def test_bit_not_all_ones
    assert_equal "\x00", "\xFF".bitwise_not
  end

  def test_bit_not_pattern
    assert_equal "\x55", "\xAA".bitwise_not  # 0b10101010 -> 0b01010101
  end

  def test_bit_not_multi_byte
    assert_equal "\xFF\x00", "\x00\xFF".bitwise_not
  end

  def test_bit_not_does_not_modify_original
    original = "\xAA"
    result = original.bitwise_not
    assert_equal "\xAA", original
    assert_equal "\x55", result
  end

  def test_bit_not_twice_is_identity
    s = "\xAA\xCC"
    assert_equal s, s.bitwise_not.bitwise_not
  end

  def test_bit_not_bang_modifies_in_place
    s = +"\xAA"
    result = s.bitwise_not!
    assert_equal "\x55", s
    assert_same s, result
  end

  def test_bit_not_bit_count
    s = "\xAA"  # 4 bits set
    assert_equal 8 - s.bit_count, s.bitwise_not.bit_count
  end
end

class TestBitBinaryOps < Minitest::Test
  def test_bit_and
    assert_equal "\xAA", "\xFF".bitwise_and("\xAA")
  end

  def test_bit_and_bang_modifies_in_place
    a = +"\xFF"
    result = a.bitwise_and!("\xAA")
    assert_equal "\xAA", a
    assert_same a, result
  end

  def test_bit_and_does_not_modify_original
    a = "\xFF"
    a.bitwise_and("\xAA")
    assert_equal "\xFF", a
  end

  def test_bit_or
    assert_equal "\xAA", "\x00".bitwise_or("\xAA")
  end

  def test_bit_or_bang_modifies_in_place
    a = +"\x00"
    result = a.bitwise_or!("\xAA")
    assert_equal "\xAA", a
    assert_same a, result
  end

  def test_bit_xor
    assert_equal "\x55", "\xFF".bitwise_xor("\xAA")  # 0xFF ^ 0xAA = 0x55
    assert_equal "\x00", "\xAA".bitwise_xor("\xAA")
  end

  def test_bit_xor_bang_modifies_in_place
    a = +"\xFF"
    result = a.bitwise_xor!("\xAA")
    assert_equal "\x55", a
    assert_same a, result
  end

  def test_size_mismatch_raises
    assert_raises(ArgumentError) { "\xFF".bitwise_and!("\xFF\xFF") }
    assert_raises(ArgumentError) { "\xFF".bitwise_or!("\xFF\xFF") }
    assert_raises(ArgumentError) { "\xFF".bitwise_xor!("\xFF\xFF") }
    assert_raises(ArgumentError) { "\xFF".bitwise_and("\xFF\xFF") }
  end

  def test_bit_and_identity_is_all_ones
    s = "\xAA\xCC"
    assert_equal s, s.bitwise_and("\xFF\xFF")
  end

  def test_bit_or_identity_is_all_zeros
    s = "\xAA\xCC"
    assert_equal s, s.bitwise_or("\x00\x00")
  end

  def test_bit_xor_with_self_is_all_zeros
    s = "\xAA\xCC"
    assert_equal "\x00\x00", s.bitwise_xor(s)
  end

  def test_bit_xor_with_all_ones_is_bit_not
    s = "\xAA\xCC"
    assert_equal s.bitwise_not, s.bitwise_xor("\xFF\xFF")
  end

  def test_de_morgan_and_or
    a = "\xAA\xF0"
    b = "\xCC\x0F"
    # ~(a & b) == ~a | ~b
    lhs = a.bitwise_and(b).bitwise_not
    rhs = a.bitwise_not.bitwise_or(b.bitwise_not)
    assert_equal lhs, rhs
  end

  def test_arrow_combined_validity
    # null propagation: result is valid only where both inputs are valid
    left_validity  = "\xF0"  # elements 4-7 valid
    right_validity = "\xCC"  # elements 2,3,6,7 valid
    result = left_validity.bitwise_and(right_validity)
    assert_equal "\xC0", result  # elements 6,7 valid (intersection)
  end

  def test_arrow_union_validity
    # OR: valid if either source is valid
    a = "\xF0"
    b = "\x0F"
    assert_equal "\xFF", a.bitwise_or(b)
  end

  def test_operand_is_converted_with_to_str
    other = Object.new
    def other.to_str
      "\xCC"
    end
    assert_equal "\xC0", "\xF0".bitwise_and(other)
    assert_equal "\xC0", (+"\xF0").bitwise_and!(other)
  end

  def test_non_string_operand_raises_type_error
    assert_raises(TypeError) { "\x00".bitwise_and(Object.new) }
    assert_raises(TypeError) { "\x00".bitwise_or(0) }
    assert_raises(TypeError) { (+"\x00").bitwise_xor!(nil) }
  end
end

# Bit operations read the receiver as a byte sequence, so the bytes they
# produce need not be valid in the receiver's encoding. Every String returned
# by a non-destructive method is therefore BINARY, while the destructive
# variants leave the receiver's encoding untouched (as String#setbyte does).
class TestBitwiseEncoding < Minitest::Test
  def utf8(bytes)
    bytes.dup.force_encoding(Encoding::UTF_8)
  end

  def test_non_destructive_results_are_binary
    s = utf8("\xF0")
    assert_equal Encoding::BINARY, s.bitwise_not.encoding
    assert_equal Encoding::BINARY, s.bitwise_and(utf8("\xCC")).encoding
    assert_equal Encoding::BINARY, s.bitwise_or(utf8("\x0C")).encoding
    assert_equal Encoding::BINARY, s.bitwise_xor(utf8("\xCC")).encoding
  end

  def test_destructive_variants_keep_the_receiver_encoding
    s = utf8(+"\xF0")
    s.bitwise_not!
    assert_equal Encoding::UTF_8, s.encoding
    s.bitwise_and!(utf8("\xCC"))
    assert_equal Encoding::UTF_8, s.encoding
  end

  def test_bit_slice_result_is_binary
    assert_equal Encoding::BINARY, utf8("\xFF\xAA").bit_slice(4, 8).encoding
  end
end
