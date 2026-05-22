require_relative "test_helper"

class TestEachBitOffset < Minitest::Test
  def setup
    @data = [0b10101010, 0b11001100].pack('C*')
  end

  # --- bit=true / 1 (set bits) ---

  def test_lsb_set_positions
    positions = []
    @data.each_bit_offset(true) { |i| positions << i }
    assert_equal [1, 3, 5, 7, 10, 11, 14, 15], positions
  end

  def test_msb_set_positions
    positions = []
    @data.each_bit_offset(true, lsb_first: false) { |i| positions << i }
    assert_equal [0, 2, 4, 6, 8, 9, 12, 13], positions
  end

  def test_set_with_integer_1
    positions = []
    @data.each_bit_offset(1) { |i| positions << i }
    assert_equal [1, 3, 5, 7, 10, 11, 14, 15], positions
  end

  # --- bit=false / 0 (unset bits) ---

  def test_lsb_unset_positions
    positions = []
    @data.each_bit_offset(false) { |i| positions << i }
    assert_equal [0, 2, 4, 6, 8, 9, 12, 13], positions
  end

  def test_msb_unset_positions
    positions = []
    @data.each_bit_offset(false, lsb_first: false) { |i| positions << i }
    assert_equal [1, 3, 5, 7, 10, 11, 14, 15], positions
  end

  def test_unset_with_integer_0
    positions = []
    @data.each_bit_offset(0) { |i| positions << i }
    assert_equal [0, 2, 4, 6, 8, 9, 12, 13], positions
  end

  # --- set and unset partition the full bit range ---

  def test_set_and_unset_partition_all_positions
    set_positions   = @data.each_bit_offset(true).to_a
    unset_positions = @data.each_bit_offset(false).to_a
    assert_equal (0...(@data.bytesize * 8)).to_a, (set_positions + unset_positions).sort
  end

  # --- edge cases ---

  def test_empty_string
    assert_empty "".each_bit_offset(true).to_a
    assert_empty "".each_bit_offset(false).to_a
  end

  def test_all_zeros_set
    assert_empty "\x00\x00".each_bit_offset(true).to_a
  end

  def test_all_zeros_unset
    assert_equal (0..15).to_a, "\x00\x00".each_bit_offset(false).to_a
  end

  def test_all_ones_set
    assert_equal (0..7).to_a, "\xFF".each_bit_offset(true).to_a
  end

  def test_all_ones_unset
    assert_empty "\xFF".each_bit_offset(false).to_a
  end

  def test_single_bit_msb_high
    positions = []
    "\x80".each_bit_offset(true) { |i| positions << i }
    assert_equal [7], positions
  end

  def test_single_bit_msb_low
    positions = []
    "\x01".each_bit_offset(true) { |i| positions << i }
    assert_equal [0], positions
  end

  # --- Enumerator / self return values ---

  def test_returns_enumerator_without_block
    assert_instance_of Enumerator, @data.each_bit_offset(true)
    assert_instance_of Enumerator, @data.each_bit_offset(false)
  end

  def test_returns_self_with_block
    assert_same @data, @data.each_bit_offset(true) {}
    assert_same @data, @data.each_bit_offset(false) {}
  end

  def test_enumerator_set
    assert_equal [1, 3, 5, 7, 10, 11, 14, 15], @data.each_bit_offset(true).to_a
  end

  def test_enumerator_unset
    assert_equal [0, 2, 4, 6, 8, 9, 12, 13], @data.each_bit_offset(false).to_a
  end

  # --- consistency with bit_at ---

  def test_set_positions_match_bit_at
    @data.each_bit_offset(true) do |n|
      assert_equal true, @data.bit_at(n), "bit_at(#{n}) should be set"
    end
  end

  def test_unset_positions_match_bit_at
    @data.each_bit_offset(false) do |n|
      assert_equal false, @data.bit_at(n), "bit_at(#{n}) should be unset"
    end
  end

  def test_msb_set_positions_roundtrip_with_bit_at
    @data.each_bit_offset(true, lsb_first: false) do |n|
      assert_equal true, @data.bit_at(n, lsb_first: false)
    end
  end

  def test_msb_unset_positions_roundtrip_with_bit_at
    @data.each_bit_offset(false, lsb_first: false) do |n|
      assert_equal false, @data.bit_at(n, lsb_first: false)
    end
  end

  # --- relationship between LSB-first and MSB-first ---

  def test_msb_reverses_bit_numbering_within_each_byte_set
    msb = @data.each_bit_offset(true, lsb_first: false).to_a
    lsb = @data.each_bit_offset(true, lsb_first: true).to_a
    assert_equal msb, lsb.map { |n| (n & ~7) | (7 - (n & 7)) }.sort
  end

  def test_msb_and_lsb_cover_same_count
    assert_equal @data.each_bit_offset(true).to_a.size,
                 @data.each_bit_offset(true, lsb_first: false).to_a.size
  end

  def test_set_and_unset_counts_match_bit_count
    set_count   = @data.each_bit_offset(true).to_a.size
    unset_count = @data.each_bit_offset(false).to_a.size
    assert_equal @data.bit_count, set_count
    assert_equal @data.bytesize * 8 - @data.bit_count, unset_count
  end
end
