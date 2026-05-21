## Benchmark

Benchmark scripts live in https://github.com/hasumikin/string_bits/tree/master/benchmark

Environment:

```bash
$> uname -a
Linux hasumi-Ubuntu-Desktop 6.17.0-20-generic #20~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Thu Mar 19 01:28:37 UTC 2 x86_64 x86_64 x86_64 GNU/Linux
$> ruby -v
ruby 4.0.4 (2026-05-12 revision b89eb1bcbf) +PRISM [x86_64-linux]
```

Result:

```bash
$> rake benchmark
```

Each benchmark is run in the order of "without YJIT" followed by "with YJIT":

```
(cd tmp/x86_64-linux/string_bits/4.0.4 && /usr/bin/gmake install sitearchdir=../../../../lib/string_bits sitelibdir=../../../../lib/string_bits target_prefix=)
/usr/bin/install -c -m 0755 string_bits.so ../../../../lib/string_bits
cp tmp/x86_64-linux/string_bits/4.0.4/string_bits.so tmp/x86_64-linux/stage/lib/string_bits/string_bits.so
RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_and.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_AND ===
Warming up --------------------------------------
            baseline     12.646 i/s -      14.000 times in 1.107076s (79.08ms/i)
         string_bits    15.755k i/s -     16.731k times in 1.061964s (63.47μs/i)
Calculating -------------------------------------
            baseline     13.137 i/s -      37.000 times in 2.816443s (76.12ms/i)
         string_bits    15.987k i/s -     47.264k times in 2.956432s (62.55μs/i)

Comparison:
            baseline:        13.1 i/s 
         string_bits:     15986.8 i/s - 1216.92x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_and.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     27.442 i/s -      30.000 times in 1.093212s (36.44ms/i)
         string_bits    16.353k i/s -     16.390k times in 1.002237s (61.15μs/i)
Calculating -------------------------------------
            baseline     34.132 i/s -      82.000 times in 2.402404s (29.30ms/i)
         string_bits    15.789k i/s -     49.060k times in 3.107209s (63.33μs/i)

Comparison:
            baseline:        34.1 i/s 
         string_bits:     15789.1 i/s - 462.58x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_at.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_AT ===
Warming up --------------------------------------
            baseline     11.459 i/s -      12.000 times in 1.047217s (87.27ms/i)
         string_bits     20.951 i/s -      24.000 times in 1.145524s (47.73ms/i)
Calculating -------------------------------------
            baseline     12.241 i/s -      34.000 times in 2.777530s (81.69ms/i)
         string_bits     22.318 i/s -      62.000 times in 2.777965s (44.81ms/i)

Comparison:
            baseline:        12.2 i/s 
         string_bits:        22.3 i/s - 1.82x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_at.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     41.655 i/s -      45.000 times in 1.080304s (24.01ms/i)
         string_bits     37.313 i/s -      40.000 times in 1.072020s (26.80ms/i)
Calculating -------------------------------------
            baseline     36.915 i/s -     124.000 times in 3.359069s (27.09ms/i)
         string_bits     41.911 i/s -     111.000 times in 2.648473s (23.86ms/i)

Comparison:
            baseline:        36.9 i/s 
         string_bits:        41.9 i/s - 1.14x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_count.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_COUNT ===
Warming up --------------------------------------
            baseline     19.832 i/s -      20.000 times in 1.008488s (50.42ms/i)
         string_bits     5.640k i/s -      6.138k times in 1.088277s (177.30μs/i)
Calculating -------------------------------------
            baseline     20.289 i/s -      59.000 times in 2.908019s (49.29ms/i)
         string_bits     5.284k i/s -     16.920k times in 3.202190s (189.25μs/i)

Comparison:
            baseline:        20.3 i/s 
         string_bits:      5283.9 i/s - 260.43x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_count.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     25.043 i/s -      27.000 times in 1.078145s (39.93ms/i)
         string_bits     4.998k i/s -      5.478k times in 1.095951s (200.06μs/i)
Calculating -------------------------------------
            baseline     23.105 i/s -      75.000 times in 3.246110s (43.28ms/i)
         string_bits     5.310k i/s -     14.995k times in 2.823919s (188.32μs/i)

Comparison:
            baseline:        23.1 i/s 
         string_bits:      5310.0 i/s - 229.82x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_not.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_NOT ===
Warming up --------------------------------------
            baseline     15.248 i/s -      16.000 times in 1.049301s (65.58ms/i)
         string_bits    17.497k i/s -     17.650k times in 1.008763s (57.15μs/i)
Calculating -------------------------------------
            baseline     15.650 i/s -      45.000 times in 2.875482s (63.90ms/i)
         string_bits    18.372k i/s -     52.490k times in 2.857034s (54.43μs/i)

Comparison:
            baseline:        15.6 i/s 
         string_bits:     18372.2 i/s - 1173.98x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_not.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     29.540 i/s -      30.000 times in 1.015585s (33.85ms/i)
         string_bits    18.881k i/s -     19.580k times in 1.037012s (52.96μs/i)
Calculating -------------------------------------
            baseline     33.766 i/s -      88.000 times in 2.606144s (29.62ms/i)
         string_bits    18.634k i/s -     56.643k times in 3.039842s (53.67μs/i)

Comparison:
            baseline:        33.8 i/s 
         string_bits:     18633.5 i/s - 551.84x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_or.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_OR ===
Warming up --------------------------------------
            baseline     14.246 i/s -      16.000 times in 1.123109s (70.19ms/i)
         string_bits    16.761k i/s -     18.370k times in 1.095996s (59.66μs/i)
Calculating -------------------------------------
            baseline     13.878 i/s -      42.000 times in 3.026279s (72.05ms/i)
         string_bits    16.516k i/s -     50.283k times in 3.044552s (60.55μs/i)

Comparison:
            baseline:        13.9 i/s 
         string_bits:     16515.7 i/s - 1190.03x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_or.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     30.731 i/s -      32.000 times in 1.041308s (32.54ms/i)
         string_bits    16.827k i/s -     16.890k times in 1.003765s (59.43μs/i)
Calculating -------------------------------------
            baseline     31.853 i/s -      92.000 times in 2.888244s (31.39ms/i)
         string_bits    15.981k i/s -     50.479k times in 3.158777s (62.58μs/i)

Comparison:
            baseline:        31.9 i/s 
         string_bits:     15980.6 i/s - 501.69x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_run_count.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_RUN_COUNT ===
Warming up --------------------------------------
            baseline    276.527 i/s -     300.000 times in 1.084884s (3.62ms/i)
         string_bits    412.993 i/s -     420.000 times in 1.016966s (2.42ms/i)
Calculating -------------------------------------
            baseline    266.929 i/s -     829.000 times in 3.105690s (3.75ms/i)
         string_bits    463.873 i/s -      1.238k times in 2.668833s (2.16ms/i)

Comparison:
            baseline:       266.9 i/s 
         string_bits:       463.9 i/s - 1.74x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_run_count.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    888.420 i/s -     957.000 times in 1.077193s (1.13ms/i)
         string_bits    563.978 i/s -     600.000 times in 1.063871s (1.77ms/i)
Calculating -------------------------------------
            baseline    883.940 i/s -      2.665k times in 3.014909s (1.13ms/i)
         string_bits    594.806 i/s -      1.691k times in 2.842941s (1.68ms/i)

Comparison:
            baseline:       883.9 i/s 
         string_bits:       594.8 i/s - 1.49x  slower

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_slice.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SLICE ===
Warming up --------------------------------------
            baseline     73.767 i/s -      80.000 times in 1.084499s (13.56ms/i)
         string_bits    377.354 i/s -     380.000 times in 1.007012s (2.65ms/i)
Calculating -------------------------------------
            baseline     81.994 i/s -     221.000 times in 2.695334s (12.20ms/i)
         string_bits    421.453 i/s -      1.132k times in 2.685943s (2.37ms/i)

Comparison:
            baseline:        82.0 i/s 
         string_bits:       421.5 i/s - 5.14x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_slice.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    301.307 i/s -     319.000 times in 1.058721s (3.32ms/i)
         string_bits    435.459 i/s -     440.000 times in 1.010428s (2.30ms/i)
Calculating -------------------------------------
            baseline    312.139 i/s -     903.000 times in 2.892941s (3.20ms/i)
         string_bits    474.669 i/s -      1.306k times in 2.751389s (2.11ms/i)

Comparison:
            baseline:       312.1 i/s 
         string_bits:       474.7 i/s - 1.52x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_splice.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SPLICE ===
Warming up --------------------------------------
            baseline     77.176 i/s -      80.000 times in 1.036590s (12.96ms/i)
         string_bits     34.731 i/s -      36.000 times in 1.036534s (28.79ms/i)
Calculating -------------------------------------
            baseline     71.927 i/s -     231.000 times in 3.211596s (13.90ms/i)
         string_bits     37.998 i/s -     104.000 times in 2.737005s (26.32ms/i)

Comparison:
            baseline:        71.9 i/s 
         string_bits:        38.0 i/s - 1.89x  slower

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_splice.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    212.820 i/s -     220.000 times in 1.033737s (4.70ms/i)
         string_bits     40.729 i/s -      44.000 times in 1.080304s (24.55ms/i)
Calculating -------------------------------------
            baseline    185.135 i/s -     638.000 times in 3.446140s (5.40ms/i)
         string_bits     45.743 i/s -     122.000 times in 2.667098s (21.86ms/i)

Comparison:
            baseline:       185.1 i/s 
         string_bits:        45.7 i/s - 4.05x  slower

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_xor.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_XOR ===
Warming up --------------------------------------
            baseline     11.035 i/s -      12.000 times in 1.087478s (90.62ms/i)
         string_bits    16.921k i/s -     18.260k times in 1.079101s (59.10μs/i)
Calculating -------------------------------------
            baseline     11.618 i/s -      33.000 times in 2.840333s (86.07ms/i)
         string_bits    16.609k i/s -     50.764k times in 3.056434s (60.21μs/i)

Comparison:
            baseline:        11.6 i/s 
         string_bits:     16608.9 i/s - 1429.54x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_xor.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     27.557 i/s -      30.000 times in 1.088664s (36.29ms/i)
         string_bits    15.751k i/s -     16.863k times in 1.070616s (63.49μs/i)
Calculating -------------------------------------
            baseline     31.090 i/s -      82.000 times in 2.637497s (32.16ms/i)
         string_bits    15.783k i/s -     47.252k times in 2.993921s (63.36μs/i)

Comparison:
            baseline:        31.1 i/s 
         string_bits:     15782.6 i/s - 507.64x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bloom_filter.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BLOOM_FILTER ===
Warming up --------------------------------------
            baseline      7.194 i/s -       8.000 times in 1.112098s (139.01ms/i)
         string_bits      9.396 i/s -      10.000 times in 1.064266s (106.43ms/i)
Calculating -------------------------------------
            baseline      8.000 i/s -      21.000 times in 2.624942s (125.00ms/i)
         string_bits      8.805 i/s -      28.000 times in 3.179999s (113.57ms/i)

Comparison:
            baseline:         8.0 i/s 
         string_bits:         8.8 i/s - 1.10x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bloom_filter.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     14.290 i/s -      16.000 times in 1.119625s (69.98ms/i)
         string_bits     14.602 i/s -      16.000 times in 1.095747s (68.48ms/i)
Calculating -------------------------------------
            baseline     14.636 i/s -      42.000 times in 2.869730s (68.33ms/i)
         string_bits     14.089 i/s -      43.000 times in 3.051955s (70.98ms/i)

Comparison:
            baseline:        14.6 i/s 
         string_bits:        14.1 i/s - 1.04x  slower

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/clear_bit.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== CLEAR_BIT ===
Warming up --------------------------------------
            baseline      9.281 i/s -      10.000 times in 1.077490s (107.75ms/i)
         string_bits     22.259 i/s -      24.000 times in 1.078215s (44.93ms/i)
Calculating -------------------------------------
            baseline     10.096 i/s -      27.000 times in 2.674317s (99.05ms/i)
         string_bits     20.895 i/s -      66.000 times in 3.158702s (47.86ms/i)

Comparison:
            baseline:        10.1 i/s 
         string_bits:        20.9 i/s - 2.07x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/clear_bit.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     29.584 i/s -      30.000 times in 1.014055s (33.80ms/i)
         string_bits     33.653 i/s -      36.000 times in 1.069751s (29.72ms/i)
Calculating -------------------------------------
            baseline     22.817 i/s -      88.000 times in 3.856837s (43.83ms/i)
         string_bits     37.800 i/s -     100.000 times in 2.645490s (26.45ms/i)

Comparison:
            baseline:        22.8 i/s 
         string_bits:        37.8 i/s - 1.66x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/clear_bit_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== CLEAR_BIT_RANGE ===
Warming up --------------------------------------
            baseline      1.428 i/s -       2.000 times in 1.400408s (700.20ms/i)
         string_bits     60.583 i/s -      66.000 times in 1.089423s (16.51ms/i)
Calculating -------------------------------------
            baseline      1.597 i/s -       4.000 times in 2.503922s (625.98ms/i)
         string_bits     61.681 i/s -     181.000 times in 2.934446s (16.21ms/i)

Comparison:
            baseline:         1.6 i/s 
         string_bits:        61.7 i/s - 38.61x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/clear_bit_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline      3.757 i/s -       4.000 times in 1.064733s (266.18ms/i)
         string_bits     67.198 i/s -      70.000 times in 1.041702s (14.88ms/i)
Calculating -------------------------------------
            baseline      3.166 i/s -      11.000 times in 3.474207s (315.84ms/i)
         string_bits     75.668 i/s -     201.000 times in 2.656355s (13.22ms/i)

Comparison:
            baseline:         3.2 i/s 
         string_bits:        75.7 i/s - 23.90x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== EACH_BIT ===
Warming up --------------------------------------
            baseline     17.756 i/s -      18.000 times in 1.013727s (56.32ms/i)
         string_bits     93.863 i/s -      99.000 times in 1.054732s (10.65ms/i)
Calculating -------------------------------------
            baseline     16.850 i/s -      53.000 times in 3.145360s (59.35ms/i)
         string_bits    101.838 i/s -     281.000 times in 2.759282s (9.82ms/i)

Comparison:
            baseline:        16.9 i/s 
         string_bits:       101.8 i/s - 6.04x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     48.485 i/s -      50.000 times in 1.031247s (20.62ms/i)
         string_bits     96.821 i/s -     100.000 times in 1.032834s (10.33ms/i)
Calculating -------------------------------------
            baseline     53.254 i/s -     145.000 times in 2.722818s (18.78ms/i)
         string_bits    100.366 i/s -     290.000 times in 2.889421s (9.96ms/i)

Comparison:
            baseline:        53.3 i/s 
         string_bits:       100.4 i/s - 1.88x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit_run.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== EACH_BIT_RUN ===
Warming up --------------------------------------
            baseline      9.817 i/s -      10.000 times in 1.018623s (101.86ms/i)
         string_bits     44.554 i/s -      45.000 times in 1.010004s (22.44ms/i)
Calculating -------------------------------------
            baseline     10.373 i/s -      29.000 times in 2.795854s (96.41ms/i)
         string_bits     52.092 i/s -     133.000 times in 2.553176s (19.20ms/i)

Comparison:
            baseline:        10.4 i/s 
         string_bits:        52.1 i/s - 5.02x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit_run.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     35.534 i/s -      36.000 times in 1.013118s (28.14ms/i)
         string_bits     48.640 i/s -      54.000 times in 1.110206s (20.56ms/i)
Calculating -------------------------------------
            baseline     34.788 i/s -     106.000 times in 3.047026s (28.75ms/i)
         string_bits     54.549 i/s -     145.000 times in 2.658180s (18.33ms/i)

Comparison:
            baseline:        34.8 i/s 
         string_bits:        54.5 i/s - 1.57x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/flip_bit.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== FLIP_BIT ===
Warming up --------------------------------------
            baseline      8.763 i/s -       9.000 times in 1.027063s (114.12ms/i)
         string_bits     20.897 i/s -      21.000 times in 1.004931s (47.85ms/i)
Calculating -------------------------------------
            baseline      9.038 i/s -      26.000 times in 2.876607s (110.64ms/i)
         string_bits     22.211 i/s -      62.000 times in 2.791422s (45.02ms/i)

Comparison:
            baseline:         9.0 i/s 
         string_bits:        22.2 i/s - 2.46x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/flip_bit.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     22.798 i/s -      24.000 times in 1.052716s (43.86ms/i)
         string_bits     30.786 i/s -      32.000 times in 1.039428s (32.48ms/i)
Calculating -------------------------------------
            baseline     26.386 i/s -      68.000 times in 2.577131s (37.90ms/i)
         string_bits     37.312 i/s -      92.000 times in 2.465719s (26.80ms/i)

Comparison:
            baseline:        26.4 i/s 
         string_bits:        37.3 i/s - 1.41x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/flip_bit_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== FLIP_BIT_RANGE ===
Warming up --------------------------------------
            baseline      1.387 i/s -       2.000 times in 1.442438s (721.22ms/i)
         string_bits     57.540 i/s -      60.000 times in 1.042745s (17.38ms/i)
Calculating -------------------------------------
            baseline      1.371 i/s -       4.000 times in 2.916836s (729.21ms/i)
         string_bits     62.444 i/s -     172.000 times in 2.754487s (16.01ms/i)

Comparison:
            baseline:         1.4 i/s 
         string_bits:        62.4 i/s - 45.53x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/flip_bit_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline      3.674 i/s -       4.000 times in 1.088785s (272.20ms/i)
         string_bits     71.036 i/s -      77.000 times in 1.083952s (14.08ms/i)
Calculating -------------------------------------
            baseline      3.290 i/s -      11.000 times in 3.343047s (303.91ms/i)
         string_bits     64.818 i/s -     213.000 times in 3.286149s (15.43ms/i)

Comparison:
            baseline:         3.3 i/s 
         string_bits:        64.8 i/s - 19.70x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/hamming.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== HAMMING ===
Warming up --------------------------------------
            baseline     43.008 i/s -      45.000 times in 1.046327s (23.25ms/i)
         string_bits    889.670 i/s -     890.000 times in 1.000371s (1.12ms/i)
Calculating -------------------------------------
            baseline     40.039 i/s -     129.000 times in 3.221853s (24.98ms/i)
         string_bits    897.699 i/s -      2.669k times in 2.973156s (1.11ms/i)

Comparison:
            baseline:        40.0 i/s 
         string_bits:       897.7 i/s - 22.42x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/hamming.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    386.000 i/s -     418.000 times in 1.082900s (2.59ms/i)
         string_bits     1.103k i/s -      1.199k times in 1.087300s (906.84μs/i)
Calculating -------------------------------------
            baseline    354.160 i/s -      1.158k times in 3.269711s (2.82ms/i)
         string_bits     1.232k i/s -      3.308k times in 2.684063s (811.39μs/i)

Comparison:
            baseline:       354.2 i/s 
         string_bits:      1232.5 i/s - 3.48x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/jaccard.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== JACCARD ===
Warming up --------------------------------------
            baseline     94.322 i/s -     100.000 times in 1.060193s (10.60ms/i)
         string_bits    17.681k i/s -     19.071k times in 1.078644s (56.56μs/i)
Calculating -------------------------------------
            baseline     92.990 i/s -     282.000 times in 3.032573s (10.75ms/i)
         string_bits    16.184k i/s -     53.041k times in 3.277346s (61.79μs/i)

Comparison:
            baseline:        93.0 i/s 
         string_bits:     16184.1 i/s - 174.04x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/jaccard.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    183.834 i/s -     198.000 times in 1.077059s (5.44ms/i)
         string_bits    15.776k i/s -     16.692k times in 1.058089s (63.39μs/i)
Calculating -------------------------------------
            baseline    180.358 i/s -     551.000 times in 3.055037s (5.54ms/i)
         string_bits    15.983k i/s -     47.326k times in 2.961092s (62.57μs/i)

Comparison:
            baseline:       180.4 i/s 
         string_bits:     15982.6 i/s - 88.62x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/rle.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== RLE ===
Warming up --------------------------------------
            baseline     10.760 i/s -      11.000 times in 1.022289s (92.94ms/i)
            each_bit     13.948 i/s -      14.000 times in 1.003749s (71.70ms/i)
        each_bit_run     42.625 i/s -      44.000 times in 1.032267s (23.46ms/i)
Calculating -------------------------------------
            baseline     10.851 i/s -      32.000 times in 2.948971s (92.16ms/i)
            each_bit     14.354 i/s -      41.000 times in 2.856308s (69.67ms/i)
        each_bit_run     44.129 i/s -     127.000 times in 2.877915s (22.66ms/i)

Comparison:
            baseline:        10.9 i/s 
            each_bit:        14.4 i/s - 1.32x  faster
        each_bit_run:        44.1 i/s - 4.07x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/rle.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     35.373 i/s -      36.000 times in 1.017730s (28.27ms/i)
            each_bit     24.509 i/s -      27.000 times in 1.101658s (40.80ms/i)
        each_bit_run     40.250 i/s -      44.000 times in 1.093174s (24.84ms/i)
Calculating -------------------------------------
            baseline     32.442 i/s -     106.000 times in 3.267358s (30.82ms/i)
            each_bit     22.165 i/s -      73.000 times in 3.293531s (45.12ms/i)
        each_bit_run     40.585 i/s -     120.000 times in 2.956785s (24.64ms/i)

Comparison:
            baseline:        32.4 i/s 
            each_bit:        22.2 i/s - 1.46x  slower
        each_bit_run:        40.6 i/s - 1.25x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/set_bit.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== SET_BIT ===
Warming up --------------------------------------
            baseline     10.581 i/s -      11.000 times in 1.039637s (94.51ms/i)
         string_bits     21.421 i/s -      24.000 times in 1.120386s (46.68ms/i)
Calculating -------------------------------------
            baseline     10.420 i/s -      31.000 times in 2.974912s (95.96ms/i)
         string_bits     20.623 i/s -      64.000 times in 3.103283s (48.49ms/i)

Comparison:
            baseline:        10.4 i/s 
         string_bits:        20.6 i/s - 1.98x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/set_bit.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     27.641 i/s -      30.000 times in 1.085330s (36.18ms/i)
         string_bits     33.764 i/s -      36.000 times in 1.066226s (29.62ms/i)
Calculating -------------------------------------
            baseline     26.849 i/s -      82.000 times in 3.054133s (37.25ms/i)
         string_bits     37.376 i/s -     101.000 times in 2.702234s (26.75ms/i)

Comparison:
            baseline:        26.8 i/s 
         string_bits:        37.4 i/s - 1.39x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/set_bit_offsets.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== SET_BIT_OFFSETS ===
Warming up --------------------------------------
            baseline     14.178 i/s -      16.000 times in 1.128533s (70.53ms/i)
         string_bits    243.810 i/s -     264.000 times in 1.082811s (4.10ms/i)
Calculating -------------------------------------
            baseline     13.919 i/s -      42.000 times in 3.017351s (71.84ms/i)
         string_bits    241.491 i/s -     731.000 times in 3.027033s (4.14ms/i)

Comparison:
            baseline:        13.9 i/s 
         string_bits:       241.5 i/s - 17.35x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/set_bit_offsets.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     51.858 i/s -      54.000 times in 1.041313s (19.28ms/i)
         string_bits    223.457 i/s -     242.000 times in 1.082983s (4.48ms/i)
Calculating -------------------------------------
            baseline     47.838 i/s -     155.000 times in 3.240114s (20.90ms/i)
         string_bits    225.987 i/s -     670.000 times in 2.964778s (4.43ms/i)

Comparison:
            baseline:        47.8 i/s 
         string_bits:       226.0 i/s - 4.72x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/set_bit_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== SET_BIT_RANGE ===
Warming up --------------------------------------
            baseline      1.515 i/s -       2.000 times in 1.320294s (660.15ms/i)
         string_bits     59.741 i/s -      60.000 times in 1.004336s (16.74ms/i)
Calculating -------------------------------------
            baseline      1.568 i/s -       4.000 times in 2.551406s (637.85ms/i)
         string_bits     64.987 i/s -     179.000 times in 2.754391s (15.39ms/i)

Comparison:
            baseline:         1.6 i/s 
         string_bits:        65.0 i/s - 41.45x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/set_bit_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline      3.959 i/s -       4.000 times in 1.010364s (252.59ms/i)
         string_bits     73.311 i/s -      80.000 times in 1.091238s (13.64ms/i)
Calculating -------------------------------------
            baseline      3.532 i/s -      11.000 times in 3.114767s (283.16ms/i)
         string_bits     68.918 i/s -     219.000 times in 3.177687s (14.51ms/i)

Comparison:
            baseline:         3.5 i/s 
         string_bits:        68.9 i/s - 19.51x  faster

/home/hasumi/.rbenv/versions/4.0.4/bin/ruby benchmark/allocation.rb

Allocation benchmark
Ruby 4.0.4 | string_bits dev
                                                                             allocs
  ===================================================================================

  bit_count (1MB data)
  -----------------------------------------------------------------------------------
  baseline:    each_byte { b.to_s(2).count("1") }                           1000014
  baseline:    bytes.sum { ... }  (+ Array alloc)                           1000005
  string_bits: bit_count                                                          3

  bulk bitwise AND (1MB + 1MB)
  -----------------------------------------------------------------------------------
  baseline:    bytes/zip/map/pack  (natural)                                1000012
  baseline:    dup + setbyte loop  (optimised)                                   11
  string_bits: bit_and  (returns new String)                                      5
  string_bits: bit_and! (in-place, needs dup)                                     5

  bit_slice x1000 (64-bit unaligned windows, 100KB data)
  -----------------------------------------------------------------------------------
  baseline:    byte-shift loop x1000                                           3007
  string_bits: bit_slice x1000                                                 1003

  set-bit iteration (1M bits, ~50% set)
  -----------------------------------------------------------------------------------
  baseline:    manual byte loop + conditional push                                6
  string_bits: each_set_bit_offset { block }  (yields Fixnum)                     6
  string_bits: set_bit_offsets  (-> Array)                                        6

  run-length encoding -- validity bitmap (~100KB, ~1,960 runs)
  -----------------------------------------------------------------------------------
  baseline:    each_byte + bit loop                                            1968
  string_bits: each_bit { block }                                              1964
  string_bits: each_bit_run { block }                                          1964

  range mutation x1000 (64-bit ranges in 100KB bitmap)
  -----------------------------------------------------------------------------------
  baseline:    range.each { set_bit logic } x1000                                 6
  string_bits: set_bit(range) x1000                                               7

  Array#mask (100K elements, ~50% masked)
  -----------------------------------------------------------------------------------
  baseline:    each_byte + byte[bit] loop                                         7
  string_bits: mask  (returns new Array)                                          6
  string_bits: mask! (in-place, needs dup)                                        7
```
