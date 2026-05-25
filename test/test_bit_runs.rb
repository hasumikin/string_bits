require_relative "test_helper"

class TestBitRuns < Minitest::Test
  # --- return value ---

  def test_returns_array_without_block
    assert_instance_of Array, "\xFF".bit_runs
  end

  def test_returns_self_with_block
    s = "\xFF"
    assert_same s, s.bit_runs { |_, _, _| }
  end

  def test_empty_string_returns_empty_array
    assert_empty "".bit_runs
  end

  def test_empty_string_with_block_returns_self
    s = ""
    assert_same s, s.bit_runs { |_, _| }
  end

  # --- matches each_bit_run.to_a ---

  def test_matches_each_bit_run_to_a
    data = "\xAA\xCC\xFF\x00\xF0"
    assert_equal data.each_bit_run.to_a, data.bit_runs
  end

  def test_matches_each_bit_run_to_a_msb
    data = "\xAA\xCC\xFF\x00\xF0"
    assert_equal data.each_bit_run(lsb_first: false).to_a, data.bit_runs(lsb_first: false)
  end

  # --- content ---

  def test_all_ones
    assert_equal [[true, 0, 8]], "\xFF".bit_runs
  end

  def test_all_zeros
    assert_equal [[false, 0, 8]], "\x00".bit_runs
  end

  def test_two_runs
    assert_equal [[false, 0, 4], [true, 4, 4]], "\xF0".bit_runs
  end

  def test_cross_byte_run
    assert_equal [[true, 0, 8], [false, 8, 8]], "\xFF\x00".bit_runs
  end

  # --- msb order ---

  def test_msb_can_merge_runs_across_byte_boundaries
    data = "\x0F\xF0"
    assert_equal [[false, 0, 4], [true, 4, 8], [false, 12, 4]], data.bit_runs(lsb_first: false)
  end

  def test_msb_two_runs
    assert_equal [[true, 0, 4], [false, 4, 4]], "\xF0".bit_runs(lsb_first: false)
  end

  # --- block form ---

  def test_block_yields_bit_offset_and_length
    triples = []
    "\xFF\x00".bit_runs { |bit, offset, len| triples << [bit, offset, len] }
    assert_equal [[true, 0, 8], [false, 8, 8]], triples
  end

  def test_block_msb_order
    triples = []
    "\xF0".bit_runs(lsb_first: false) { |bit, offset, len| triples << [bit, offset, len] }
    assert_equal [[true, 0, 4], [false, 4, 4]], triples
  end

  # --- run lengths sum to total bits ---

  def test_run_lengths_cover_all_bits
    data = "\xAA\xCC\xFF\x00\xF0\x0F"
    total = data.bit_runs.sum { |_, _, len| len }
    assert_equal data.bytesize * 8, total
  end

  # --- error handling ---

  def test_invalid_order
    assert_raises(ArgumentError) { "\xFF".bit_runs(lsb_first: :foo) }
  end

  # --- bit_offset ---

  def test_bit_offset_byte_aligned
    assert_equal [[false, 8, 8]], "\xFF\x00".bit_runs(8)
  end

  def test_bit_offset_non_byte_aligned
    assert_equal [[true, 4, 4]], "\xF0".bit_runs(4)
  end

  def test_bit_offset_zero_same_as_default
    data = "\xAA\xCC"
    assert_equal data.bit_runs, data.bit_runs(0)
  end

  def test_bit_offset_at_total_bits_returns_empty
    assert_empty "\xFF".bit_runs(8)
  end

  def test_bit_offset_msb
    assert_equal [[false, 4, 4]], "\xF0".bit_runs(4, lsb_first: false)
  end

  def test_bit_offset_negative_raises_index_error
    assert_raises(IndexError) { "\xFF".bit_runs(-1) }
  end

  def test_bit_offset_bignum_raises_argument_error
    assert_raises(ArgumentError) { "\xFF".bit_runs(2**62) }
  end

  def test_bit_offset_matches_each_bit_run
    data = "\x00\xFF\xAA"
    assert_equal data.each_bit_run(8).to_a, data.bit_runs(8)
    assert_equal data.each_bit_run(8, lsb_first: false).to_a, data.bit_runs(8, lsb_first: false)
  end
end
