#!/usr/bin/env ruby

# frozen_string_literal: true
#
# Benchmark: Ruby object allocations per operation
#
# Uses GC.stat[:total_allocated_objects] to count heap objects allocated
# during each operation. YJIT has no effect on allocation counts, so this
# runs in a single process without subprocess isolation.
#
# Note: Ruby Fixnum/Integer values up to a certain size are immediate (tagged
# pointers) and do NOT count as allocations. Only String, Array, etc. do.
#
# Usage: ruby docs/benchmark/memory.rb

$LOAD_PATH.unshift File.expand_path("../lib", __dir__)
require "string_bits"

DATA_1MB  = Random.bytes(1_000_000)
A_1MB     = Random.bytes(1_000_000)
B_1MB     = Random.bytes(1_000_000)
DATA_100K = Random.bytes(100_000)

# Validity bitmap: long runs of valid rows (0xFF) punctuated by short null
# bursts (0x00). Models Apache Arrow null bitmaps or sensor active/inactive
# windows. Pattern: 100 bytes of 0xFF (800 ones) + 2 bytes of 0x00 (16 zeros).
# ~1,960 runs total vs ~400,000 for random data.
RLE_DATA = (("\xFF" * 100) + ("\x00" * 2)) * (100_000 / 102)

COL_LABEL = 70
COL_COUNT =  9

def measure(label)
  GC.start(full_mark: true, immediate_sweep: true)
  before = GC.stat[:total_allocated_objects]
  yield
  after = GC.stat[:total_allocated_objects]
  printf "  %-#{COL_LABEL}s  %#{COL_COUNT}d\n", label, after - before
end

def section(title)
  puts
  puts "  #{title}"
  puts "  " + "-" * (COL_LABEL + COL_COUNT + 4)
end

puts "Allocation benchmark"
puts "Ruby #{RUBY_VERSION} | string_bits #{StringBits::VERSION rescue "dev"}"
printf "  %-#{COL_LABEL}s  %#{COL_COUNT}s\n", "", "allocs"
puts  "  " + "=" * (COL_LABEL + COL_COUNT + 4)

# --- bit_count ---
section "bit_count (1MB data)"
measure("baseline:    each_byte { b.to_s(2).count(\"1\") }") {
  DATA_1MB.each_byte.sum { |b| b.to_s(2).count("1") }
}
measure("baseline:    bytes.sum { ... }  (+ Array alloc)") {
  DATA_1MB.bytes.sum { |b| b.to_s(2).count("1") }
}
measure("string_bits: bit_count") {
  DATA_1MB.bit_count
}

# --- bulk bitwise ---
section "bulk bitwise AND (1MB + 1MB)"
measure("baseline:    bytes/zip/map/pack  (natural)") {
  A_1MB.bytes.zip(B_1MB.bytes).map { |a, b| a & b }.pack("C*")
}
measure("baseline:    dup + setbyte loop  (optimised)") {
  result = A_1MB.dup
  A_1MB.bytesize.times { |i| result.setbyte(i, A_1MB.getbyte(i) & B_1MB.getbyte(i)) }
  result
}
measure("string_bits: bitwise_and  (returns new String)") {
  A_1MB.bitwise_and(B_1MB)
}
measure("string_bits: bitwise_and! (in-place, needs dup)") {
  A_1MB.dup.bitwise_and!(B_1MB)
}

# --- bit_slice ---
section "bit_slice x1000 (64-bit unaligned windows, 100KB data)"
measure("baseline:    byte-shift loop x1000") {
  1_000.times do |i|
    offset = i * 37
    shift  = offset & 7
    bstart = offset >> 3
    anti   = 8 - shift
    result = (0.chr * 8).b
    8.times do |j|
      lo = DATA_100K.getbyte(bstart + j) || 0
      hi = DATA_100K.getbyte(bstart + j + 1) || 0
      result.setbyte(j, ((lo >> shift) | (hi << anti)) & 0xFF)
    end
  end
}
measure("string_bits: bit_slice x1000") {
  1_000.times { |i| DATA_100K.bit_slice(i * 37, 64) }
}

# --- each_bit_offset / bit_offsets ---
section "set-bit iteration (1M bits, ~50% set)"
validity = Random.bytes(125_000)
measure("baseline:    manual byte loop + conditional push") {
  positions = []
  validity.each_byte.with_index do |byte, bi|
    base = bi * 8
    8.times { |bit| positions << base + bit if (byte >> bit) & 1 == 1 }
  end
  positions
}
measure("string_bits: each_bit_offset(true) { block }  (yields Fixnum)") {
  validity.each_bit_offset(true, lsb_first: true) { |_i| }
}
measure("string_bits: bit_offsets(true)  (-> Array)") {
  validity.bit_offsets(true, lsb_first: true)
}

# --- RLE ---
# RLE_DATA: validity-bitmap pattern (~1,960 runs over ~100KB).
# Allocations are dominated by [bit, len] pairs pushed to the result array,
# so each approach allocates proportionally to run count, not bit count.
# each_bit_run's advantage over each_bit is CPU time (fewer block calls), not allocs.
section "run-length encoding -- validity bitmap (~100KB, ~1,960 runs)"
measure("baseline:    each_byte + bit loop") {
  runs = []; current = nil; count = 0
  RLE_DATA.each_byte do |byte|
    8.times do |bit|
      b = (byte >> bit) & 1 == 1
      if b == current then count += 1
      else runs << [current, count] unless current.nil?; current = b; count = 1
      end
    end
  end
  runs << [current, count] unless current.nil?
  runs
}
measure("string_bits: each_bit { block }") {
  runs = []; current = nil; count = 0
  RLE_DATA.each_bit do |b|
    if b == current then count += 1
    else runs << [current, count] unless current.nil?; current = b; count = 1
    end
  end
  runs << [current, count] unless current.nil?
  runs
}
measure("string_bits: each_bit_run { block }") {
  runs = []
  RLE_DATA.each_bit_run { |bit, len| runs << [bit, len] }
  runs
}

# --- range mutation ---
section "range mutation x1000 (64-bit ranges in 100KB bitmap)"
RANGE_BITMAP = ("\x00" * 12_500).b
RANGES_1K = Array.new(1_000) do
  start = rand(RANGE_BITMAP.bytesize * 8 - 64 + 1)
  start..(start + 63)
end
measure("baseline:    range.each { bit_set logic } x1000") {
  bitmap = RANGE_BITMAP.dup
  RANGES_1K.each do |range|
    range.each do |pos|
      byte_idx = pos >> 3
      bit_mask = 1 << (pos & 7)
      bitmap.setbyte(byte_idx, bitmap.getbyte(byte_idx) | bit_mask)
    end
  end
  bitmap
}
measure("string_bits: bit_set(range) x1000") {
  bitmap = RANGE_BITMAP.dup
  RANGES_1K.each { |range| bitmap.bit_set(range) }
  bitmap
}

# --- Array#mask ---
section "Array#mask (100K elements, ~50% masked)"
bm_values = Array.new(100_000) { rand(1000) }
bm_bitmap = Random.bytes((100_000 + 7) / 8)
measure("baseline:    each_byte + byte[bit] loop") {
  result = Array.new(100_000)
  bm_bitmap.each_byte.with_index do |byte, bi|
    base = bi * 8
    8.times { |bit| result[base + bit] = bm_values[base + bit] if byte[bit] == 1 }
  end
  result
}
measure("string_bits: mask  (returns new Array)") {
  bm_values.mask(bm_bitmap, lsb_first: true)
}
measure("string_bits: mask! (in-place, needs dup)") {
  bm_values.dup.mask!(bm_bitmap, lsb_first: true)
}
