require_relative "test_helper"

# Every bit position in this API -- the single-bit offset of bit_get and
# friends, the offset and length of an offset/length pair, and each endpoint of
# a bit Range -- shares one representable range, the one accepted into Ruby
# core (docs/v1.md, and Discussion.md "Error behavior for out-of-range bit
# indices"):
#
#   - any non-negative position below 2**64 is well-formed. One past the end of
#     the receiver is not an error in itself; it takes the ordinary
#     out-of-range path of whichever method received it;
#   - only a position that does not fit in 64 bits raises ArgumentError,
#     because no byte buffer on any platform can be addressed by it.
#
# The boundary is therefore the same on a 64-bit and a 32-bit build, which is
# what these tests pin. Internally the extension still holds bit indices in a
# pointer-width signed integer, so the tests around the platform's Fixnum
# boundary also guard against that internal type leaking back into the public
# contract.
class TestIndexRange < Minitest::Test
  WORD = 1.size * 8        # 64 or 32 on this platform
  FIX  = 1 << (WORD - 2)   # smallest Bignum on this platform
  MAX  = 2**64 - 1         # largest representable bit position
  OVER = 2**64             # first position outside the representable range

  # --- positions past the end of the string ---------------------------------

  def test_read_methods_answer_out_of_range
    s = "\xFF".b
    assert_nil s.bit_get(FIX)
    assert_nil s.bit_set?(MAX)
    assert_equal 0, s.bit_count(FIX, 8)
    assert_equal 8, s.bit_count(0, MAX)
    assert_nil s.bit_run_count(true, FIX)
  end

  def test_iterator_methods_yield_nothing
    s = "\xFF".b
    assert_empty s.each_bit(FIX).to_a
    assert_empty s.each_bit_offset(1, FIX).to_a
    assert_empty s.each_bit_run(FIX).to_a
    assert_empty s.bits(MAX)
  end

  def test_slice_method_answers_nil_or_clamps
    s = "\xFF".b
    assert_nil s.bit_slice(FIX, 8)
    assert_equal "\xFF".b, s.bit_slice(0, MAX)
  end

  def test_range_endpoint_past_the_end
    s = "\xFF".b
    assert_equal 8, s.bit_count(0..FIX)
    assert_equal 0, s.bit_count(FIX..MAX)
    assert_nil s.bit_slice(FIX..MAX)
    assert_equal "\xFF".b, s.bit_slice(0..FIX)
  end

  def test_mutation_methods_raise_index_error
    assert_raises(IndexError) { (+"\xFF").bit_set(FIX) }
    assert_raises(IndexError) { (+"\xFF").bit_set(0, FIX) }
    assert_raises(IndexError) { (+"\xFF").bit_clear(0..FIX) }
    assert_raises(IndexError) { (+"\xFF").bit_splice(FIX, 8, "\x00") }
  end

  # --- positions outside the representable range ----------------------------

  def test_read_methods_raise_argument_error
    s = "\xFF".b
    assert_raises(ArgumentError) { s.bit_get(OVER) }
    assert_raises(ArgumentError) { s.bit_set?(OVER) }
    assert_raises(ArgumentError) { s.bit_count(OVER, 8) }
    assert_raises(ArgumentError) { s.bit_count(0, OVER) }
    assert_raises(ArgumentError) { s.bit_run_count(true, OVER) }
  end

  def test_iterator_methods_raise_argument_error
    s = "\xFF".b
    assert_raises(ArgumentError) { s.each_bit(OVER) {} }
    assert_raises(ArgumentError) { s.each_bit_offset(1, OVER) {} }
    assert_raises(ArgumentError) { s.each_bit_run(OVER) {} }
  end

  def test_slice_method_raises_argument_error
    s = "\xFF".b
    assert_raises(ArgumentError) { s.bit_slice(OVER, 8) }
    assert_raises(ArgumentError) { s.bit_slice(0, OVER) }
  end

  def test_range_endpoint_out_of_range_raises_argument_error
    s = "\xFF".b
    assert_raises(ArgumentError) { s.bit_count(0..OVER) }
    assert_raises(ArgumentError) { s.bit_slice(0..OVER) }
    assert_raises(ArgumentError) { (+"\xFF").bit_set(0..OVER) }
  end

  def test_mutation_methods_raise_argument_error
    assert_raises(ArgumentError) { (+"\xFF").bit_set(OVER) }
    assert_raises(ArgumentError) { (+"\xFF").bit_set(0, OVER) }
    assert_raises(ArgumentError) { (+"\xFF").bit_splice(OVER, 8, "\x00") }
  end

  # --- negative positions ---------------------------------------------------

  def test_negative_positions_raise_index_error
    s = "\xFF".b
    assert_raises(IndexError) { s.bit_get(-1) }
    assert_raises(IndexError) { s.bit_count(-1, 8) }
    assert_raises(IndexError) { s.bit_count(-1..7) }
    assert_raises(IndexError) { s.each_bit(-1) {} }
    assert_raises(IndexError) { (+"\xFF").bit_set(-1) }
    assert_raises(IndexError) { (+"\xFF").bit_splice(-1, 8, "\x00") }
  end
end
