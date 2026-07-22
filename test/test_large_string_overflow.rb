require_relative "test_helper"

# Regression tests for the 32-bit (ILP32) internal integer-overflow bug.
#
# The public API deliberately uses a pointer-width signed bit index, and raises
# ArgumentError for positions outside that range (see Discussion.md, "Error
# behavior for out-of-range bit indices"). That part is by design: on a 32-bit
# build the top of a string larger than ~256 MiB is simply not bit-addressable.
#
# The actual bug is internal: the extension computes a string's bit length as
# `RSTRING_LEN(self) * 8` in many places (ext/string_bits/string_bits.c). When
# the string is at least 2**28 bytes that product overflows a signed 32-bit
# integer (2**28 * 8 == 2**31), so the corrupted size makes even small, fully
# representable, in-range offsets look out of range. bit_set?(8) then returns nil
# instead of the real bit, bit_count(8, 8) returns 0, bit_set(8) raises, etc.
#
# These tests therefore use a 2**28-byte string (the smallest size that
# overflows) and only small offsets that fit a 32-bit index, so a correct fix
# (non-overflowing bounds checks, not a wider public index) makes them pass.
#
# Allocates ~256 MiB (up to ~512 MiB peak with the mutation test).
class TestLargeStringOverflow < Minitest::Test
  N = 268_435_456   # 2**28 bytes; 8 * N == 2**31 overflows signed 32-bit
  # "a" == 0x61 == 0b0110_0001: LSB-first set bits at 0, 5, 6 (popcount 3).
  # byte[1] therefore has set bits at flat offsets 8, 13, 14.

  # One shared read-only buffer; building it costs ~256 MiB so reuse it.
  def self.big
    @big ||= ("a" * N).freeze
  end

  def big
    self.class.big
  end

  # --- bit_set? (ext sites ~230 / ~392) ------------------------------------
  def test_bit_set_p_small_offset
    assert_equal true,  big.bit_set?(8)    # byte[1] bit 0; nil on the buggy build
    assert_equal false, big.bit_set?(15)   # byte[1] bit 7
  end

  # --- bit_count (ext sites ~460 / ~546) ---------------------------------
  def test_bit_count_full_string_is_unaffected
    # No-arg popcount loops over bytes and does not use bytesize * 8, so it is
    # correct even on the buggy build. Kept as a sanity anchor / contrast.
    assert_equal 3 * N, big.bit_count
  end

  def test_bit_count_offset_length
    assert_equal 3, big.bit_count(8, 8)    # byte[1] popcount; 0 on the buggy build
    assert_equal 6, big.bit_count(8, 16)   # byte[1..2] popcount
  end

  # --- bit_slice (ext sites ~911 / ~945) ---------------------------------
  def test_bit_slice_small_offset
    assert_equal "a",  big.bit_slice(8, 8)    # nil on the buggy build
    assert_equal "aa", big.bit_slice(8, 16)
  end

  # --- each_bit (ext site ~593) ------------------------------------------
  def test_each_bit_small_start_offset
    # .first stops the enumerator early, so only a handful of bits are walked.
    assert_equal [true, false, false, false, false, true, true, false],
                 big.each_bit(8).first(8)
  end

  # --- each_bit_offset (ext site ~693) -----------------------------------
  def test_each_bit_offset_small_start_offset
    assert_equal [8, 13, 14], big.each_bit_offset(true, 8).first(3)
  end

  # --- each_bit_run (ext site ~1548) -------------------------------------
  def test_each_bit_run_small_start_offset
    assert_equal [[true, 8, 1], [false, 9, 4]], big.each_bit_run(8).first(2)
  end

  # --- bit_run_count (ext sites ~1515 / ~1526) ---------------------------
  def test_bit_run_count_small_offset
    assert_equal 1, big.bit_run_count(true, 8)    # single 1 at bit 8
    assert_equal 4, big.bit_run_count(false, 9)   # four 0s at bits 9..12
  end

  # --- bit_set / mutation (ext site ~1025) -------------------------------
  def test_bit_set_small_offset
    buf = "\x00".b * N   # fresh ~256 MiB writable buffer
    buf.bit_set(8)       # raises IndexError on the buggy build
    assert_equal 1, buf.getbyte(1)
  end
end
