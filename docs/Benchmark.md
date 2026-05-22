# Benchmark

Benchmark scripts live in https://github.com/hasumikin/string_bits/tree/master/benchmark

## Environment

```bash
$> uname -a
Linux hasumi-Ubuntu-Desktop 6.17.0-20-generic #20~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Thu Mar 19 01:28:37 UTC 2 x86_64 x86_64 x86_64 GNU/Linux
$> ruby -v
ruby 4.0.4 (2026-05-12 revision b89eb1bcbf) +PRISM [x86_64-linux]
```

## Result

Each benchmark is run in the order of "without YJIT" followed by "with YJIT":

```bash
$> rake benchmark
```

=>

```
(cd tmp/x86_64-linux/string_bits/4.0.4 && /usr/bin/gmake install sitearchdir=../../../../lib/string_bits sitelibdir=../../../../lib/string_bits target_prefix=)
/usr/bin/install -c -m 0755 string_bits.so ../../../../lib/string_bits
cp tmp/x86_64-linux/string_bits/4.0.4/string_bits.so tmp/x86_64-linux/stage/lib/string_bits/string_bits.so
RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_at.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_AT ===
Warming up --------------------------------------
            baseline     11.558 i/s -      12.000 times in 1.038246s (86.52ms/i)
         string_bits     19.866 i/s -      20.000 times in 1.006757s (50.34ms/i)
Calculating -------------------------------------
            baseline     12.042 i/s -      34.000 times in 2.823381s (83.04ms/i)
         string_bits     20.947 i/s -      59.000 times in 2.816672s (47.74ms/i)

Comparison:
            baseline:        12.0 i/s 
         string_bits:        20.9 i/s - 1.74x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_at.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     37.437 i/s -      40.000 times in 1.068449s (26.71ms/i)
         string_bits     37.401 i/s -      40.000 times in 1.069496s (26.74ms/i)
Calculating -------------------------------------
            baseline     39.700 i/s -     112.000 times in 2.821140s (25.19ms/i)
         string_bits     39.749 i/s -     112.000 times in 2.817649s (25.16ms/i)

Comparison:
            baseline:        39.7 i/s 
         string_bits:        39.7 i/s - 1.00x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_clear.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_CLEAR ===
Warming up --------------------------------------
            baseline      8.792 i/s -       9.000 times in 1.023692s (113.74ms/i)
         string_bits     22.177 i/s -      24.000 times in 1.082225s (45.09ms/i)
Calculating -------------------------------------
            baseline     10.216 i/s -      26.000 times in 2.545119s (97.89ms/i)
         string_bits     20.718 i/s -      66.000 times in 3.185646s (48.27ms/i)

Comparison:
            baseline:        10.2 i/s 
         string_bits:        20.7 i/s - 2.03x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_clear.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     30.460 i/s -      33.000 times in 1.083387s (32.83ms/i)
         string_bits     33.746 i/s -      36.000 times in 1.066784s (29.63ms/i)
Calculating -------------------------------------
            baseline     24.472 i/s -      91.000 times in 3.718478s (40.86ms/i)
         string_bits     38.007 i/s -     101.000 times in 2.657381s (26.31ms/i)

Comparison:
            baseline:        24.5 i/s 
         string_bits:        38.0 i/s - 1.55x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_clear_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_CLEAR_RANGE ===
Warming up --------------------------------------
            baseline      1.599 i/s -       2.000 times in 1.251042s (625.52ms/i)
         string_bits     67.351 i/s -      70.000 times in 1.039336s (14.85ms/i)
Calculating -------------------------------------
            baseline      1.473 i/s -       4.000 times in 2.714679s (678.67ms/i)
         string_bits     63.015 i/s -     202.000 times in 3.205605s (15.87ms/i)

Comparison:
            baseline:         1.5 i/s 
         string_bits:        63.0 i/s - 42.77x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_clear_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline      3.486 i/s -       4.000 times in 1.147534s (286.88ms/i)
         string_bits     76.200 i/s -      80.000 times in 1.049870s (13.12ms/i)
Calculating -------------------------------------
            baseline      3.442 i/s -      10.000 times in 2.905305s (290.53ms/i)
         string_bits     67.999 i/s -     228.000 times in 3.353013s (14.71ms/i)

Comparison:
            baseline:         3.4 i/s 
         string_bits:        68.0 i/s - 19.76x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_count.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_COUNT ===
Warming up --------------------------------------
            baseline     22.394 i/s -      24.000 times in 1.071736s (44.66ms/i)
         string_bits     6.486k i/s -      7.128k times in 1.098982s (154.18μs/i)
Calculating -------------------------------------
            baseline     21.559 i/s -      67.000 times in 3.107705s (46.38ms/i)
         string_bits     6.043k i/s -     19.458k times in 3.219815s (165.48μs/i)

Comparison:
            baseline:        21.6 i/s 
         string_bits:      6043.2 i/s - 280.31x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_count.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     22.298 i/s -      24.000 times in 1.076352s (44.85ms/i)
         string_bits     6.508k i/s -      7.029k times in 1.080032s (153.65μs/i)
Calculating -------------------------------------
            baseline     25.171 i/s -      66.000 times in 2.622036s (39.73ms/i)
         string_bits     6.509k i/s -     19.524k times in 2.999735s (153.64μs/i)

Comparison:
            baseline:        25.2 i/s 
         string_bits:      6508.6 i/s - 258.57x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_flip.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_FLIP ===
Warming up --------------------------------------
            baseline      8.673 i/s -       9.000 times in 1.037680s (115.30ms/i)
         string_bits     19.944 i/s -      20.000 times in 1.002805s (50.14ms/i)
Calculating -------------------------------------
            baseline      9.587 i/s -      26.000 times in 2.711951s (104.31ms/i)
         string_bits     21.129 i/s -      59.000 times in 2.792436s (47.33ms/i)

Comparison:
            baseline:         9.6 i/s 
         string_bits:        21.1 i/s - 2.20x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_flip.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     25.802 i/s -      27.000 times in 1.046433s (38.76ms/i)
         string_bits     28.995 i/s -      32.000 times in 1.103651s (34.49ms/i)
Calculating -------------------------------------
            baseline     27.033 i/s -      77.000 times in 2.848359s (36.99ms/i)
         string_bits     36.733 i/s -      86.000 times in 2.341215s (27.22ms/i)

Comparison:
            baseline:        27.0 i/s 
         string_bits:        36.7 i/s - 1.36x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_flip_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_FLIP_RANGE ===
Warming up --------------------------------------
            baseline      1.484 i/s -       2.000 times in 1.347468s (673.73ms/i)
         string_bits     57.357 i/s -      60.000 times in 1.046086s (17.43ms/i)
Calculating -------------------------------------
            baseline      1.418 i/s -       4.000 times in 2.821370s (705.34ms/i)
         string_bits     65.094 i/s -     172.000 times in 2.642352s (15.36ms/i)

Comparison:
            baseline:         1.4 i/s 
         string_bits:        65.1 i/s - 45.91x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_flip_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline      4.061 i/s -       5.000 times in 1.231079s (246.22ms/i)
         string_bits     65.220 i/s -      70.000 times in 1.073283s (15.33ms/i)
Calculating -------------------------------------
            baseline      3.450 i/s -      12.000 times in 3.478024s (289.84ms/i)
         string_bits     68.234 i/s -     195.000 times in 2.857830s (14.66ms/i)

Comparison:
            baseline:         3.5 i/s 
         string_bits:        68.2 i/s - 19.78x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_offsets.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_OFFSETS ===
Warming up --------------------------------------
            baseline     14.330 i/s -      16.000 times in 1.116532s (69.78ms/i)
         string_bits    247.570 i/s -     253.000 times in 1.021933s (4.04ms/i)
Calculating -------------------------------------
            baseline     13.988 i/s -      42.000 times in 3.002510s (71.49ms/i)
         string_bits    245.007 i/s -     742.000 times in 3.028482s (4.08ms/i)

Comparison:
            baseline:        14.0 i/s 
         string_bits:       245.0 i/s - 17.52x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_offsets.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     44.954 i/s -      50.000 times in 1.112238s (22.24ms/i)
         string_bits    246.908 i/s -     264.000 times in 1.069222s (4.05ms/i)
Calculating -------------------------------------
            baseline     52.596 i/s -     134.000 times in 2.547745s (19.01ms/i)
         string_bits    228.054 i/s -     740.000 times in 3.244851s (4.38ms/i)

Comparison:
            baseline:        52.6 i/s 
         string_bits:       228.1 i/s - 4.34x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_run_count.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_RUN_COUNT ===
Warming up --------------------------------------
            baseline    279.356 i/s -     280.000 times in 1.002304s (3.58ms/i)
         string_bits    472.871 i/s -     480.000 times in 1.015077s (2.11ms/i)
Calculating -------------------------------------
            baseline    264.633 i/s -     838.000 times in 3.166646s (3.78ms/i)
         string_bits    470.721 i/s -      1.418k times in 3.012398s (2.12ms/i)

Comparison:
            baseline:       264.6 i/s 
         string_bits:       470.7 i/s - 1.78x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_run_count.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    804.110 i/s -     869.000 times in 1.080697s (1.24ms/i)
         string_bits    500.319 i/s -     520.000 times in 1.039337s (2.00ms/i)
Calculating -------------------------------------
            baseline    869.189 i/s -      2.412k times in 2.775001s (1.15ms/i)
         string_bits    588.335 i/s -      1.500k times in 2.549566s (1.70ms/i)

Comparison:
            baseline:       869.2 i/s 
         string_bits:       588.3 i/s - 1.48x  slower

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_set.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SET ===
Warming up --------------------------------------
            baseline     10.654 i/s -      12.000 times in 1.126330s (93.86ms/i)
         string_bits     22.280 i/s -      24.000 times in 1.077220s (44.88ms/i)
Calculating -------------------------------------
            baseline     10.750 i/s -      31.000 times in 2.883682s (93.02ms/i)
         string_bits     22.142 i/s -      66.000 times in 2.980698s (45.16ms/i)

Comparison:
            baseline:        10.8 i/s 
         string_bits:        22.1 i/s - 2.06x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_set.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     27.391 i/s -      30.000 times in 1.095243s (36.51ms/i)
         string_bits     32.377 i/s -      36.000 times in 1.111903s (30.89ms/i)
Calculating -------------------------------------
            baseline     27.436 i/s -      82.000 times in 2.988771s (36.45ms/i)
         string_bits     35.032 i/s -      97.000 times in 2.768923s (28.55ms/i)

Comparison:
            baseline:        27.4 i/s 
         string_bits:        35.0 i/s - 1.28x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_set_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SET_RANGE ===
Warming up --------------------------------------
            baseline      1.681 i/s -       2.000 times in 1.189828s (594.91ms/i)
         string_bits     58.173 i/s -      60.000 times in 1.031410s (17.19ms/i)
Calculating -------------------------------------
            baseline      1.596 i/s -       5.000 times in 3.132340s (626.47ms/i)
         string_bits     59.628 i/s -     174.000 times in 2.918107s (16.77ms/i)

Comparison:
            baseline:         1.6 i/s 
         string_bits:        59.6 i/s - 37.35x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_set_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline      4.221 i/s -       5.000 times in 1.184668s (236.93ms/i)
         string_bits     67.057 i/s -      70.000 times in 1.043890s (14.91ms/i)
Calculating -------------------------------------
            baseline      3.374 i/s -      12.000 times in 3.556181s (296.35ms/i)
         string_bits     75.817 i/s -     201.000 times in 2.651109s (13.19ms/i)

Comparison:
            baseline:         3.4 i/s 
         string_bits:        75.8 i/s - 22.47x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_slice.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SLICE ===
Warming up --------------------------------------
            baseline     82.055 i/s -      90.000 times in 1.096819s (12.19ms/i)
         string_bits    386.063 i/s -     420.000 times in 1.087906s (2.59ms/i)
Calculating -------------------------------------
            baseline     79.768 i/s -     246.000 times in 3.083948s (12.54ms/i)
         string_bits    394.262 i/s -      1.158k times in 2.937133s (2.54ms/i)

Comparison:
            baseline:        79.8 i/s 
         string_bits:       394.3 i/s - 4.94x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_slice.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    303.673 i/s -     330.000 times in 1.086697s (3.29ms/i)
         string_bits    440.369 i/s -     473.000 times in 1.074099s (2.27ms/i)
Calculating -------------------------------------
            baseline    309.302 i/s -     911.000 times in 2.945340s (3.23ms/i)
         string_bits    444.917 i/s -      1.321k times in 2.969096s (2.25ms/i)

Comparison:
            baseline:       309.3 i/s 
         string_bits:       444.9 i/s - 1.44x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_splice.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SPLICE ===
Warming up --------------------------------------
            baseline     71.255 i/s -      72.000 times in 1.010460s (14.03ms/i)
         string_bits     39.900 i/s -      40.000 times in 1.002495s (25.06ms/i)
Calculating -------------------------------------
            baseline     80.112 i/s -     213.000 times in 2.658772s (12.48ms/i)
         string_bits     39.893 i/s -     119.000 times in 2.982980s (25.07ms/i)

Comparison:
            baseline:        80.1 i/s 
         string_bits:        39.9 i/s - 2.01x  slower

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_splice.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    213.474 i/s -     220.000 times in 1.030569s (4.68ms/i)
         string_bits     46.353 i/s -      50.000 times in 1.078677s (21.57ms/i)
Calculating -------------------------------------
            baseline    197.938 i/s -     640.000 times in 3.233333s (5.05ms/i)
         string_bits     42.916 i/s -     139.000 times in 3.238875s (23.30ms/i)

Comparison:
            baseline:       197.9 i/s 
         string_bits:        42.9 i/s - 4.61x  slower

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_and.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BITWISE_AND ===
Warming up --------------------------------------
            baseline     12.666 i/s -      14.000 times in 1.105348s (78.95ms/i)
         string_bits    17.198k i/s -     18.876k times in 1.097558s (58.15μs/i)
Calculating -------------------------------------
            baseline     14.205 i/s -      37.000 times in 2.604797s (70.40ms/i)
         string_bits    17.247k i/s -     51.594k times in 2.991530s (57.98μs/i)

Comparison:
            baseline:        14.2 i/s 
         string_bits:     17246.7 i/s - 1214.17x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_and.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     30.805 i/s -      32.000 times in 1.038795s (32.46ms/i)
         string_bits    18.391k i/s -     18.590k times in 1.010813s (54.37μs/i)
Calculating -------------------------------------
            baseline     34.666 i/s -      92.000 times in 2.653894s (28.85ms/i)
         string_bits    17.056k i/s -     55.173k times in 3.234852s (58.63μs/i)

Comparison:
            baseline:        34.7 i/s 
         string_bits:     17055.8 i/s - 492.00x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_not.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BITWISE_NOT ===
Warming up --------------------------------------
            baseline     15.771 i/s -      16.000 times in 1.014497s (63.41ms/i)
         string_bits    19.168k i/s -     20.340k times in 1.061131s (52.17μs/i)
Calculating -------------------------------------
            baseline     15.922 i/s -      47.000 times in 2.951803s (62.80ms/i)
         string_bits    19.206k i/s -     57.504k times in 2.994029s (52.07μs/i)

Comparison:
            baseline:        15.9 i/s 
         string_bits:     19206.2 i/s - 1206.23x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_not.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     29.600 i/s -      30.000 times in 1.013526s (33.78ms/i)
         string_bits    19.355k i/s -     20.757k times in 1.072425s (51.67μs/i)
Calculating -------------------------------------
            baseline     31.597 i/s -      88.000 times in 2.785034s (31.65ms/i)
         string_bits    18.669k i/s -     58.065k times in 3.110197s (53.56μs/i)

Comparison:
            baseline:        31.6 i/s 
         string_bits:     18669.2 i/s - 590.85x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_or.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BITWISE_OR ===
Warming up --------------------------------------
            baseline     14.208 i/s -      16.000 times in 1.126109s (70.38ms/i)
         string_bits    17.305k i/s -     18.110k times in 1.046499s (57.79μs/i)
Calculating -------------------------------------
            baseline     14.235 i/s -      42.000 times in 2.950412s (70.25ms/i)
         string_bits    17.619k i/s -     51.915k times in 2.946527s (56.76μs/i)

Comparison:
            baseline:        14.2 i/s 
         string_bits:     17619.0 i/s - 1237.70x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_or.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     30.854 i/s -      32.000 times in 1.037149s (32.41ms/i)
         string_bits    16.930k i/s -     17.446k times in 1.030503s (59.07μs/i)
Calculating -------------------------------------
            baseline     34.212 i/s -      92.000 times in 2.689143s (29.23ms/i)
         string_bits    17.322k i/s -     50.788k times in 2.931930s (57.73μs/i)

Comparison:
            baseline:        34.2 i/s 
         string_bits:     17322.4 i/s - 506.33x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_xor.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BITWISE_XOR ===
Warming up --------------------------------------
            baseline     11.280 i/s -      12.000 times in 1.063822s (88.65ms/i)
         string_bits    17.140k i/s -     18.689k times in 1.090403s (58.34μs/i)
Calculating -------------------------------------
            baseline     11.730 i/s -      33.000 times in 2.813204s (85.25ms/i)
         string_bits    18.019k i/s -     51.418k times in 2.853517s (55.50μs/i)

Comparison:
            baseline:        11.7 i/s 
         string_bits:     18019.2 i/s - 1536.11x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_xor.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     27.573 i/s -      30.000 times in 1.088024s (36.27ms/i)
         string_bits    18.572k i/s -     20.339k times in 1.095137s (53.84μs/i)
Calculating -------------------------------------
            baseline     33.160 i/s -      82.000 times in 2.472823s (30.16ms/i)
         string_bits    17.592k i/s -     55.716k times in 3.167154s (56.84μs/i)

Comparison:
            baseline:        33.2 i/s 
         string_bits:     17591.8 i/s - 530.51x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bloom_filter.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BLOOM_FILTER ===
Warming up --------------------------------------
            baseline      7.163 i/s -       8.000 times in 1.116912s (139.61ms/i)
         string_bits      8.820 i/s -       9.000 times in 1.020414s (113.38ms/i)
Calculating -------------------------------------
            baseline      7.135 i/s -      21.000 times in 2.943095s (140.15ms/i)
         string_bits      8.925 i/s -      26.000 times in 2.913097s (112.04ms/i)

Comparison:
            baseline:         7.1 i/s 
         string_bits:         8.9 i/s - 1.25x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bloom_filter.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     14.375 i/s -      16.000 times in 1.113016s (69.56ms/i)
         string_bits     15.547 i/s -      16.000 times in 1.029151s (64.32ms/i)
Calculating -------------------------------------
            baseline     14.798 i/s -      43.000 times in 2.905734s (67.58ms/i)
         string_bits     15.697 i/s -      46.000 times in 2.930562s (63.71ms/i)

Comparison:
            baseline:        14.8 i/s 
         string_bits:        15.7 i/s - 1.06x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== EACH_BIT ===
Warming up --------------------------------------
            baseline     17.795 i/s -      18.000 times in 1.011494s (56.19ms/i)
         string_bits     94.087 i/s -     100.000 times in 1.062842s (10.63ms/i)
Calculating -------------------------------------
            baseline     17.842 i/s -      53.000 times in 2.970502s (56.05ms/i)
         string_bits     98.661 i/s -     282.000 times in 2.858279s (10.14ms/i)

Comparison:
            baseline:        17.8 i/s 
         string_bits:        98.7 i/s - 5.53x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     48.035 i/s -      50.000 times in 1.040907s (20.82ms/i)
         string_bits     94.130 i/s -     100.000 times in 1.062362s (10.62ms/i)
Calculating -------------------------------------
            baseline     59.657 i/s -     144.000 times in 2.413797s (16.76ms/i)
         string_bits     99.100 i/s -     282.000 times in 2.845613s (10.09ms/i)

Comparison:
            baseline:        59.7 i/s 
         string_bits:        99.1 i/s - 1.66x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit_run.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== EACH_BIT_RUN ===
Warming up --------------------------------------
            baseline      9.860 i/s -      10.000 times in 1.014161s (101.42ms/i)
         string_bits     49.932 i/s -      50.000 times in 1.001370s (20.03ms/i)
Calculating -------------------------------------
            baseline     11.149 i/s -      29.000 times in 2.601240s (89.70ms/i)
         string_bits     50.293 i/s -     149.000 times in 2.962664s (19.88ms/i)

Comparison:
            baseline:        11.1 i/s 
         string_bits:        50.3 i/s - 4.51x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit_run.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     31.786 i/s -      33.000 times in 1.038200s (31.46ms/i)
         string_bits     44.643 i/s -      45.000 times in 1.008000s (22.40ms/i)
Calculating -------------------------------------
            baseline     34.611 i/s -      95.000 times in 2.744809s (28.89ms/i)
         string_bits     49.935 i/s -     133.000 times in 2.663447s (20.03ms/i)

Comparison:
            baseline:        34.6 i/s 
         string_bits:        49.9 i/s - 1.44x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/hamming.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== HAMMING ===
Warming up --------------------------------------
            baseline     37.407 i/s -      40.000 times in 1.069320s (26.73ms/i)
         string_bits    847.598 i/s -     880.000 times in 1.038229s (1.18ms/i)
Calculating -------------------------------------
            baseline     42.130 i/s -     112.000 times in 2.658448s (23.74ms/i)
         string_bits    880.735 i/s -      2.542k times in 2.886227s (1.14ms/i)

Comparison:
            baseline:        42.1 i/s 
         string_bits:       880.7 i/s - 20.91x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/hamming.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    331.289 i/s -     340.000 times in 1.026294s (3.02ms/i)
         string_bits     1.071k i/s -      1.155k times in 1.078645s (933.89μs/i)
Calculating -------------------------------------
            baseline    334.873 i/s -     993.000 times in 2.965300s (2.99ms/i)
         string_bits     1.199k i/s -      3.212k times in 2.677852s (833.70μs/i)

Comparison:
            baseline:       334.9 i/s 
         string_bits:      1199.5 i/s - 3.58x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/jaccard.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== JACCARD ===
Warming up --------------------------------------
            baseline     84.044 i/s -      90.000 times in 1.070866s (11.90ms/i)
         string_bits    19.449k i/s -     20.552k times in 1.056688s (51.42μs/i)
Calculating -------------------------------------
            baseline     86.231 i/s -     252.000 times in 2.922395s (11.60ms/i)
         string_bits    19.790k i/s -     58.348k times in 2.948385s (50.53μs/i)

Comparison:
            baseline:        86.2 i/s 
         string_bits:     19789.8 i/s - 229.50x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/jaccard.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    175.017 i/s -     180.000 times in 1.028470s (5.71ms/i)
         string_bits    20.443k i/s -     20.544k times in 1.004951s (48.92μs/i)
Calculating -------------------------------------
            baseline    193.550 i/s -     525.000 times in 2.712480s (5.17ms/i)
         string_bits    19.662k i/s -     61.328k times in 3.119092s (50.86μs/i)

Comparison:
            baseline:       193.5 i/s 
         string_bits:     19662.1 i/s - 101.59x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/rle.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== RLE ===
Warming up --------------------------------------
            baseline      9.750 i/s -      10.000 times in 1.025600s (102.56ms/i)
            each_bit     15.958 i/s -      16.000 times in 1.002627s (62.66ms/i)
        each_bit_run     48.497 i/s -      50.000 times in 1.030998s (20.62ms/i)
Calculating -------------------------------------
            baseline     10.002 i/s -      29.000 times in 2.899342s (99.98ms/i)
            each_bit     15.594 i/s -      47.000 times in 3.013906s (64.13ms/i)
        each_bit_run     44.608 i/s -     145.000 times in 3.250505s (22.42ms/i)

Comparison:
            baseline:        10.0 i/s 
            each_bit:        15.6 i/s - 1.56x  faster
        each_bit_run:        44.6 i/s - 4.46x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/rle.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     31.927 i/s -      33.000 times in 1.033608s (31.32ms/i)
            each_bit     22.215 i/s -      24.000 times in 1.080346s (45.01ms/i)
        each_bit_run     49.161 i/s -      50.000 times in 1.017064s (20.34ms/i)
Calculating -------------------------------------
            baseline     34.895 i/s -      95.000 times in 2.722454s (28.66ms/i)
            each_bit     22.831 i/s -      66.000 times in 2.890868s (43.80ms/i)
        each_bit_run     44.433 i/s -     147.000 times in 3.308376s (22.51ms/i)

Comparison:
            baseline:        34.9 i/s 
            each_bit:        22.8 i/s - 1.53x  slower
        each_bit_run:        44.4 i/s - 1.27x  faster

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
  string_bits: bitwise_and  (returns new String)                                  5
  string_bits: bitwise_and! (in-place, needs dup)                                 5

  bit_slice x1000 (64-bit unaligned windows, 100KB data)
  -----------------------------------------------------------------------------------
  baseline:    byte-shift loop x1000                                           3007
  string_bits: bit_slice x1000                                                 1003

  set-bit iteration (1M bits, ~50% set)
  -----------------------------------------------------------------------------------
  baseline:    manual byte loop + conditional push                                6
  string_bits: each_bit_offset(true) { block }  (yields Fixnum)                   6
  string_bits: bit_offsets(true)  (-> Array)                                      6

  run-length encoding -- validity bitmap (~100KB, ~1,960 runs)
  -----------------------------------------------------------------------------------
  baseline:    each_byte + bit loop                                            1968
  string_bits: each_bit { block }                                              1964
  string_bits: each_bit_run { block }                                          1964

  range mutation x1000 (64-bit ranges in 100KB bitmap)
  -----------------------------------------------------------------------------------
  baseline:    range.each { bit_set logic } x1000                                 6
  string_bits: bit_set(range) x1000                                               7

  Array#mask (100K elements, ~50% masked)
  -----------------------------------------------------------------------------------
  baseline:    each_byte + byte[bit] loop                                         7
  string_bits: mask  (returns new Array)                                          6
  string_bits: mask! (in-place, needs dup)                                        7
```
