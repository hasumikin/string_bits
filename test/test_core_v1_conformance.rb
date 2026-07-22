# encoding: ASCII-8BIT
#
# Conformance tests for the subset accepted into Ruby core ([Feature #22118],
# docs/v1.md). These are the assertions from test/ruby/test_string.rb in the
# ruby/ruby branch, transcribed so that any drift between this gem and the
# merged behaviour shows up here rather than in review.
#
# Anything the gem adds on top of that subset (range-oriented forms, iterators,
# bit_slice, bit_splice, ...) is covered by the other test files, not here.

require_relative "test_helper"

class TestCoreV1Conformance < Minitest::Test
  def test_bit_get
    s = "\xAA\x80"
    assert_equal 0, s.bit_get(0)
    assert_equal 1, s.bit_get(1)
    assert_equal 1, s.bit_get(7)
    assert_equal 1, s.bit_get(0, lsb_first: false)
    assert_equal 0, s.bit_get(1, lsb_first: false)
    assert_equal 1, s.bit_get(8, lsb_first: false)
    assert_nil s.bit_get(16)
    assert_raises(IndexError) { s.bit_get(-1) }
    assert_raises(ArgumentError) { s.bit_get(2**100) }
    assert_raises(ArgumentError) { s.bit_get(0, lsb_first: nil) }
  end

  def test_bit_set_p
    s = "\xAA\x80"
    assert_equal false, s.bit_set?(0)
    assert_equal true, s.bit_set?(1)
    assert_equal true, s.bit_set?(7)
    assert_equal true, s.bit_set?(0, lsb_first: false)
    assert_equal false, s.bit_set?(1, lsb_first: false)
    assert_equal true, s.bit_set?(8, lsb_first: false)
    assert_nil s.bit_set?(16)
    assert_raises(IndexError) { s.bit_set?(-1) }
    assert_raises(ArgumentError) { s.bit_set?(2**100) }
    assert_raises(ArgumentError) { s.bit_set?(0, lsb_first: nil) }
  end

  def test_bit_set_clear_flip
    s = +"\x00"
    assert_same s, s.bit_set(1)
    assert_equal "\x02", s
    assert_same s, s.bit_clear(1)
    assert_equal "\x00", s
    assert_same s, s.bit_flip(1)
    assert_equal "\x02", s
    assert_same s, s.bit_flip(1)
    assert_equal "\x00", s

    s.bit_set(1, lsb_first: false)
    assert_equal "\x40", s
    s.bit_clear(1, lsb_first: false)
    assert_equal "\x00", s

    s = +"\x00\x00"
    s.bit_set(8, lsb_first: false)
    assert_equal "\x00\x80", s
    s.bit_clear(8, lsb_first: false)
    assert_equal "\x00\x00", s
    s.bit_flip(8, lsb_first: false)
    assert_equal "\x00\x80", s

    assert_raises(IndexError) { (+"\x00").bit_set(8) }
    assert_raises(IndexError) { (+"\x00").bit_set(-1) }
    assert_raises(IndexError) { (+"\x00").bit_clear(8) }
    assert_raises(IndexError) { (+"\x00").bit_clear(-1) }
    assert_raises(IndexError) { (+"\x00").bit_flip(8) }
    assert_raises(IndexError) { (+"\x00").bit_flip(-1) }
    assert_raises(ArgumentError) { (+"\x00").bit_set(0, lsb_first: nil) }
    assert_raises(FrozenError) { "\x00".freeze.bit_set(0) }

    shared = "fooXbar".split("X").last
    shared.bit_set(0)
    assert_equal "car", shared
  end

  def test_bit_count
    assert_equal 0, "".bit_count
    assert_equal 0, "\x00".bit_count
    assert_equal 8, "\xFF".bit_count
    assert_equal 8, "\xAA\xF0".bit_count
    assert_raises(ArgumentError) { "\x00".bit_count(0) }
  end

  # The one deliberate divergence from the core test suite: core asserts
  # ArgumentError for `bit_count(lsb_first: false)`, because there bit_count
  # takes no arguments at all. Here the keyword is meaningful for the
  # range-oriented forms the gem prototypes, so the no-argument form accepts
  # and ignores it rather than rejecting it (a whole-string popcount is
  # order-independent). See docs/v1.md, "Where this gem goes beyond the
  # accepted subset".
  def test_bit_count_ignores_lsb_first_without_a_range
    assert_equal 8, "\xFF".bit_count(lsb_first: false)
  end

  def test_bitwise
    s = "\x00\xAA"
    result = s.bitwise_not
    assert_equal "\xFF\x55", result
    refute_same s, result
    assert_equal "\x00\xAA", s
    assert_equal Encoding::BINARY, result.encoding

    s = +"\x00\xAA"
    assert_same s, s.bitwise_not!
    assert_equal "\xFF\x55", s

    assert_equal "\xC0", "\xF0".bitwise_and("\xCC")
    assert_equal "\xFC", "\xF0".bitwise_or("\x0C")
    assert_equal "\x3C", "\xF0".bitwise_xor("\xCC")

    utf8 = "\xF0".dup.force_encoding("UTF-8")
    assert_equal Encoding::BINARY, utf8.bitwise_and("\xCC").encoding
    assert_equal Encoding::BINARY, utf8.bitwise_or("\x0C").encoding
    assert_equal Encoding::BINARY, utf8.bitwise_xor("\xCC").encoding

    s = +"\xF0"
    assert_same s, s.bitwise_and!("\xCC")
    assert_equal "\xC0", s
    assert_same s, s.bitwise_or!("\x0C")
    assert_equal "\xCC", s
    assert_same s, s.bitwise_xor!("\xFF")
    assert_equal "\x33", s

    other = Object.new
    def other.to_str
      "\xCC"
    end
    assert_equal "\xC0", "\xF0".bitwise_and(other)

    assert_raises(ArgumentError) { "\x00".bitwise_and("\x00\x00") }
    assert_raises(TypeError) { "\x00".bitwise_or(Object.new) }
    assert_raises(FrozenError) { "\x00".freeze.bitwise_not! }
    assert_raises(FrozenError) { "\x00".freeze.bitwise_xor!("\x00") }
  end
end
