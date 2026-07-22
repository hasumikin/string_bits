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
RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_get.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_AT ===
Warming up --------------------------------------
            baseline     11.326 i/s -      12.000 times in 1.059528s (88.29ms/i)
         string_bits     21.893 i/s -      24.000 times in 1.096259s (45.68ms/i)
Calculating -------------------------------------
            baseline     12.021 i/s -      33.000 times in 2.745156s (83.19ms/i)
         string_bits     22.279 i/s -      65.000 times in 2.917498s (44.88ms/i)

Comparison:
            baseline:        12.0 i/s 
         string_bits:        22.3 i/s - 1.85x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_get.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     37.121 i/s -      40.000 times in 1.077545s (26.94ms/i)
         string_bits     37.140 i/s -      40.000 times in 1.077011s (26.93ms/i)
Calculating -------------------------------------
            baseline     39.403 i/s -     111.000 times in 2.817061s (25.38ms/i)
         string_bits     38.385 i/s -     111.000 times in 2.891717s (26.05ms/i)

Comparison:
            baseline:        39.4 i/s 
         string_bits:        38.4 i/s - 1.03x  slower

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_clear.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_CLEAR ===
Warming up --------------------------------------
            baseline     10.035 i/s -      12.000 times in 1.195839s (99.65ms/i)
         string_bits     21.805 i/s -      24.000 times in 1.100674s (45.86ms/i)
Calculating -------------------------------------
            baseline      9.522 i/s -      30.000 times in 3.150645s (105.02ms/i)
         string_bits     20.673 i/s -      65.000 times in 3.144139s (48.37ms/i)

Comparison:
            baseline:         9.5 i/s 
         string_bits:        20.7 i/s - 2.17x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_clear.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     27.096 i/s -      30.000 times in 1.107190s (36.91ms/i)
         string_bits     32.745 i/s -      36.000 times in 1.099398s (30.54ms/i)
Calculating -------------------------------------
            baseline     22.971 i/s -      81.000 times in 3.526237s (43.53ms/i)
         string_bits     37.616 i/s -      98.000 times in 2.605302s (26.58ms/i)

Comparison:
            baseline:        23.0 i/s 
         string_bits:        37.6 i/s - 1.64x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_clear_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_CLEAR_RANGE ===
Warming up --------------------------------------
            baseline      1.591 i/s -       2.000 times in 1.257188s (628.59ms/i)
         string_bits     78.983 i/s -      80.000 times in 1.012878s (12.66ms/i)
Calculating -------------------------------------
            baseline      1.412 i/s -       4.000 times in 2.833802s (708.45ms/i)
         string_bits     75.038 i/s -     236.000 times in 3.145058s (13.33ms/i)

Comparison:
            baseline:         1.4 i/s 
         string_bits:        75.0 i/s - 53.16x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_clear_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline      3.870 i/s -       4.000 times in 1.033550s (258.39ms/i)
         string_bits     91.926 i/s -     100.000 times in 1.087830s (10.88ms/i)
Calculating -------------------------------------
            baseline      3.099 i/s -      11.000 times in 3.549880s (322.72ms/i)
         string_bits     87.913 i/s -     275.000 times in 3.128098s (11.37ms/i)

Comparison:
            baseline:         3.1 i/s 
         string_bits:        87.9 i/s - 28.37x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_count.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_COUNT ===
Warming up --------------------------------------
            baseline     22.038 i/s -      24.000 times in 1.089031s (45.38ms/i)
         string_bits     6.486k i/s -      7.117k times in 1.097309s (154.18μs/i)
Calculating -------------------------------------
            baseline     18.887 i/s -      66.000 times in 3.494397s (52.95ms/i)
         string_bits     6.061k i/s -     19.457k times in 3.210154s (164.99μs/i)

Comparison:
            baseline:        18.9 i/s 
         string_bits:      6061.1 i/s - 320.91x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_count.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     24.843 i/s -      27.000 times in 1.086833s (40.25ms/i)
         string_bits     6.481k i/s -      7.128k times in 1.099840s (154.30μs/i)
Calculating -------------------------------------
            baseline     24.793 i/s -      74.000 times in 2.984759s (40.33ms/i)
         string_bits     6.483k i/s -     19.442k times in 2.998797s (154.24μs/i)

Comparison:
            baseline:        24.8 i/s 
         string_bits:      6483.3 i/s - 261.50x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_count_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_COUNT_RANGE ===
RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_count_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster
Warming up --------------------------------------
            baseline    716.656 i/s -     792.000 times in 1.105133s (1.40ms/i)
         string_bits   253.333k i/s -    263.748k times in 1.041111s (3.95μs/i)
Calculating -------------------------------------
            baseline    807.394 i/s -      2.149k times in 2.661650s (1.24ms/i)
         string_bits   259.074k i/s -    759.999k times in 2.933517s (3.86μs/i)

Comparison:
            baseline:       807.4 i/s
         string_bits:    259074.4 i/s - 320.88x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_count_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     1.568k i/s -      1.694k times in 1.080599s (637.90μs/i)
         string_bits   228.900k i/s -    243.573k times in 1.064102s (4.37μs/i)
Calculating -------------------------------------
            baseline     1.520k i/s -      4.702k times in 3.093935s (658.00μs/i)
         string_bits   258.070k i/s -    686.699k times in 2.660901s (3.87μs/i)

Comparison:
            baseline:      1519.7 i/s
         string_bits:    258070.1 i/s - 169.81x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_flip.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_FLIP ===
Warming up --------------------------------------
            baseline      9.772 i/s -      10.000 times in 1.023294s (102.33ms/i)
         string_bits     21.690 i/s -      24.000 times in 1.106478s (46.10ms/i)
Calculating -------------------------------------
            baseline      9.165 i/s -      29.000 times in 3.164163s (109.11ms/i)
         string_bits     20.494 i/s -      65.000 times in 3.171679s (48.80ms/i)

Comparison:
            baseline:         9.2 i/s 
         string_bits:        20.5 i/s - 2.24x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_flip.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     25.457 i/s -      27.000 times in 1.060629s (39.28ms/i)
         string_bits     32.511 i/s -      36.000 times in 1.107308s (30.76ms/i)
Calculating -------------------------------------
            baseline     27.866 i/s -      76.000 times in 2.727375s (35.89ms/i)
         string_bits     37.338 i/s -      97.000 times in 2.597922s (26.78ms/i)

Comparison:
            baseline:        27.9 i/s 
         string_bits:        37.3 i/s - 1.34x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_flip_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_FLIP_RANGE ===
Warming up --------------------------------------
            baseline      1.520 i/s -       2.000 times in 1.315706s (657.85ms/i)
         string_bits     78.236 i/s -      80.000 times in 1.022552s (12.78ms/i)
Calculating -------------------------------------
            baseline      1.513 i/s -       4.000 times in 2.644090s (661.02ms/i)
         string_bits     73.926 i/s -     234.000 times in 3.165325s (13.53ms/i)

Comparison:
            baseline:         1.5 i/s 
         string_bits:        73.9 i/s - 48.87x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_flip_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline      3.605 i/s -       4.000 times in 1.109663s (277.42ms/i)
         string_bits     81.317 i/s -      90.000 times in 1.106774s (12.30ms/i)
Calculating -------------------------------------
            baseline      3.619 i/s -      10.000 times in 2.762899s (276.29ms/i)
         string_bits     92.074 i/s -     243.000 times in 2.639189s (10.86ms/i)

Comparison:
            baseline:         3.6 i/s 
         string_bits:        92.1 i/s - 25.44x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_offsets.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_OFFSETS ===
Warming up --------------------------------------
            baseline     12.500 i/s -      14.000 times in 1.120020s (80.00ms/i)
         string_bits    215.645 i/s -     220.000 times in 1.020196s (4.64ms/i)
Calculating -------------------------------------
            baseline     13.046 i/s -      37.000 times in 2.836152s (76.65ms/i)
         string_bits    225.445 i/s -     646.000 times in 2.865443s (4.44ms/i)

Comparison:
            baseline:        13.0 i/s 
         string_bits:       225.4 i/s - 17.28x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_offsets.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     52.635 i/s -      54.000 times in 1.025924s (19.00ms/i)
         string_bits    215.587 i/s -     220.000 times in 1.020471s (4.64ms/i)
Calculating -------------------------------------
            baseline     49.219 i/s -     157.000 times in 3.189836s (20.32ms/i)
         string_bits    225.205 i/s -     646.000 times in 2.868502s (4.44ms/i)

Comparison:
            baseline:        49.2 i/s 
         string_bits:       225.2 i/s - 4.58x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_run_count.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_RUN_COUNT ===
Warming up --------------------------------------
            baseline    288.882 i/s -     290.000 times in 1.003871s (3.46ms/i)
         string_bits    400.287 i/s -     410.000 times in 1.024266s (2.50ms/i)
Calculating -------------------------------------
            baseline    269.428 i/s -     866.000 times in 3.214217s (3.71ms/i)
         string_bits    451.775 i/s -      1.200k times in 2.656189s (2.21ms/i)

Comparison:
            baseline:       269.4 i/s 
         string_bits:       451.8 i/s - 1.68x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_run_count.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    895.456 i/s -     957.000 times in 1.068729s (1.12ms/i)
         string_bits    555.509 i/s -     580.000 times in 1.044088s (1.80ms/i)
Calculating -------------------------------------
            baseline    829.097 i/s -      2.686k times in 3.239668s (1.21ms/i)
         string_bits    545.197 i/s -      1.666k times in 3.055775s (1.83ms/i)

Comparison:
            baseline:       829.1 i/s 
         string_bits:       545.2 i/s - 1.52x  slower

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_runs_offset.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_RUNS_OFFSET ===
Warming up --------------------------------------
            baseline     17.191 i/s -      18.000 times in 1.047071s (58.17ms/i)
         string_bits     83.448 i/s -      90.000 times in 1.078521s (11.98ms/i)
Calculating -------------------------------------
            baseline     18.685 i/s -      51.000 times in 2.729443s (53.52ms/i)
         string_bits     99.283 i/s -     250.000 times in 2.518063s (10.07ms/i)

Comparison:
            baseline:        18.7 i/s 
         string_bits:        99.3 i/s - 5.31x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_runs_offset.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     42.634 i/s -      45.000 times in 1.055489s (23.46ms/i)
         string_bits     92.697 i/s -      99.000 times in 1.068001s (10.79ms/i)
Calculating -------------------------------------
            baseline     41.122 i/s -     127.000 times in 3.088382s (24.32ms/i)
         string_bits     92.757 i/s -     278.000 times in 2.997068s (10.78ms/i)

Comparison:
            baseline:        41.1 i/s 
         string_bits:        92.8 i/s - 2.26x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_set.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SET ===
Warming up --------------------------------------
            baseline     10.773 i/s -      12.000 times in 1.113917s (92.83ms/i)
         string_bits     19.403 i/s -      20.000 times in 1.030771s (51.54ms/i)
Calculating -------------------------------------
            baseline     10.689 i/s -      32.000 times in 2.993770s (93.56ms/i)
         string_bits     21.787 i/s -      58.000 times in 2.662105s (45.90ms/i)

Comparison:
            baseline:        10.7 i/s 
         string_bits:        21.8 i/s - 2.04x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_set.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     27.537 i/s -      30.000 times in 1.089435s (36.31ms/i)
         string_bits     33.394 i/s -      36.000 times in 1.078035s (29.95ms/i)
Calculating -------------------------------------
            baseline     25.776 i/s -      82.000 times in 3.181294s (38.80ms/i)
         string_bits     36.518 i/s -     100.000 times in 2.738397s (27.38ms/i)

Comparison:
            baseline:        25.8 i/s 
         string_bits:        36.5 i/s - 1.42x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_set_range.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SET_RANGE ===
Warming up --------------------------------------
            baseline      1.541 i/s -       2.000 times in 1.297589s (648.79ms/i)
         string_bits     69.137 i/s -      70.000 times in 1.012479s (14.46ms/i)
Calculating -------------------------------------
            baseline      1.670 i/s -       4.000 times in 2.394687s (598.67ms/i)
         string_bits     72.453 i/s -     207.000 times in 2.857027s (13.80ms/i)

Comparison:
            baseline:         1.7 i/s 
         string_bits:        72.5 i/s - 43.38x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_set_range.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline      4.188 i/s -       5.000 times in 1.193801s (238.76ms/i)
         string_bits     90.926 i/s -      99.000 times in 1.088802s (11.00ms/i)
Calculating -------------------------------------
            baseline      3.175 i/s -      12.000 times in 3.779065s (314.92ms/i)
         string_bits     86.137 i/s -     272.000 times in 3.157745s (11.61ms/i)

Comparison:
            baseline:         3.2 i/s 
         string_bits:        86.1 i/s - 27.13x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_slice.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SLICE ===
Warming up --------------------------------------
            baseline     71.173 i/s -      72.000 times in 1.011627s (14.05ms/i)
         string_bits    381.165 i/s -     418.000 times in 1.096639s (2.62ms/i)
Calculating -------------------------------------
            baseline     75.036 i/s -     213.000 times in 2.838631s (13.33ms/i)
         string_bits    417.100 i/s -      1.143k times in 2.740351s (2.40ms/i)

Comparison:
            baseline:        75.0 i/s 
         string_bits:       417.1 i/s - 5.56x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_slice.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    327.606 i/s -     330.000 times in 1.007309s (3.05ms/i)
         string_bits    486.442 i/s -     528.000 times in 1.085432s (2.06ms/i)
Calculating -------------------------------------
            baseline    304.162 i/s -     982.000 times in 3.228547s (3.29ms/i)
         string_bits    447.330 i/s -      1.459k times in 3.261571s (2.24ms/i)

Comparison:
            baseline:       304.2 i/s 
         string_bits:       447.3 i/s - 1.47x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_splice.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BIT_SPLICE ===
RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_splice.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster
Warming up --------------------------------------
            baseline    104.431 i/s -     110.000 times in 1.053326s (9.58ms/i)
         string_bits    12.266k i/s -     12.420k times in 1.012571s (81.53μs/i)
Calculating -------------------------------------
            baseline    113.770 i/s -     313.000 times in 2.751164s (8.79ms/i)
         string_bits    11.252k i/s -     36.797k times in 3.270252s (88.87μs/i)

Comparison:
            baseline:       113.8 i/s
         string_bits:     11252.0 i/s - 98.90x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bit_splice.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    424.090 i/s -     460.000 times in 1.084677s (2.36ms/i)
         string_bits    10.884k i/s -     11.016k times in 1.012167s (91.88μs/i)
Calculating -------------------------------------
            baseline    477.876 i/s -      1.272k times in 2.661779s (2.09ms/i)
         string_bits    11.989k i/s -     32.650k times in 2.723435s (83.41μs/i)

Comparison:
            baseline:       477.9 i/s
         string_bits:     11988.5 i/s - 25.09x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_and.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BITWISE_AND ===
Warming up --------------------------------------
            baseline     13.422 i/s -      14.000 times in 1.043032s (74.50ms/i)
         string_bits    17.673k i/s -     18.832k times in 1.065610s (56.59μs/i)
Calculating -------------------------------------
            baseline     13.037 i/s -      40.000 times in 3.068234s (76.71ms/i)
         string_bits    16.723k i/s -     53.017k times in 3.170335s (59.80μs/i)

Comparison:
            baseline:        13.0 i/s 
         string_bits:     16722.8 i/s - 1282.74x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_and.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     27.556 i/s -      30.000 times in 1.088692s (36.29ms/i)
         string_bits    17.109k i/s -     17.360k times in 1.014669s (58.45μs/i)
Calculating -------------------------------------
            baseline     31.772 i/s -      82.000 times in 2.580876s (31.47ms/i)
         string_bits    17.615k i/s -     51.327k times in 2.913773s (56.77μs/i)

Comparison:
            baseline:        31.8 i/s 
         string_bits:     17615.3 i/s - 554.43x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_not.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BITWISE_NOT ===
Warming up --------------------------------------
            baseline     13.017 i/s -      14.000 times in 1.075529s (76.82ms/i)
         string_bits    19.857k i/s -     20.020k times in 1.008211s (50.36μs/i)
Calculating -------------------------------------
            baseline     15.201 i/s -      39.000 times in 2.565612s (65.78ms/i)
         string_bits    18.551k i/s -     59.570k times in 3.211101s (53.90μs/i)

Comparison:
            baseline:        15.2 i/s 
         string_bits:     18551.3 i/s - 1220.39x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_not.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     26.482 i/s -      27.000 times in 1.019577s (37.76ms/i)
         string_bits    19.387k i/s -     19.590k times in 1.010486s (51.58μs/i)
Calculating -------------------------------------
            baseline     30.916 i/s -      79.000 times in 2.555345s (32.35ms/i)
         string_bits    18.609k i/s -     58.160k times in 3.125374s (53.74μs/i)

Comparison:
            baseline:        30.9 i/s 
         string_bits:     18609.0 i/s - 601.93x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_or.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BITWISE_OR ===
Warming up --------------------------------------
            baseline     12.453 i/s -      14.000 times in 1.124213s (80.30ms/i)
         string_bits    16.911k i/s -     18.161k times in 1.073926s (59.13μs/i)
Calculating -------------------------------------
            baseline     13.973 i/s -      37.000 times in 2.648034s (71.57ms/i)
         string_bits    17.655k i/s -     50.732k times in 2.873541s (56.64μs/i)

Comparison:
            baseline:        14.0 i/s 
         string_bits:     17654.9 i/s - 1263.53x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_or.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     30.702 i/s -      32.000 times in 1.042286s (32.57ms/i)
         string_bits    18.058k i/s -     18.700k times in 1.035532s (55.38μs/i)
Calculating -------------------------------------
            baseline     32.094 i/s -      92.000 times in 2.866547s (31.16ms/i)
         string_bits    17.180k i/s -     54.175k times in 3.153324s (58.21μs/i)

Comparison:
            baseline:        32.1 i/s 
         string_bits:     17180.3 i/s - 535.31x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_xor.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BITWISE_XOR ===
Warming up --------------------------------------
            baseline     12.291 i/s -      14.000 times in 1.139040s (81.36ms/i)
         string_bits    16.850k i/s -     18.458k times in 1.095398s (59.35μs/i)
Calculating -------------------------------------
            baseline     11.685 i/s -      36.000 times in 3.080869s (85.58ms/i)
         string_bits    17.874k i/s -     50.551k times in 2.828140s (55.95μs/i)

Comparison:
            baseline:        11.7 i/s 
         string_bits:     17874.3 i/s - 1529.68x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bitwise_xor.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     31.049 i/s -      32.000 times in 1.030624s (32.21ms/i)
         string_bits    17.902k i/s -     18.450k times in 1.030605s (55.86μs/i)
Calculating -------------------------------------
            baseline     34.261 i/s -      93.000 times in 2.714440s (29.19ms/i)
         string_bits    16.892k i/s -     53.706k times in 3.179348s (59.20μs/i)

Comparison:
            baseline:        34.3 i/s 
         string_bits:     16892.1 i/s - 493.04x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bloom_filter.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== BLOOM_FILTER ===
Warming up --------------------------------------
            baseline      8.038 i/s -       9.000 times in 1.119620s (124.40ms/i)
         string_bits      8.442 i/s -       9.000 times in 1.066122s (118.46ms/i)
Calculating -------------------------------------
            baseline      7.916 i/s -      24.000 times in 3.031988s (126.33ms/i)
         string_bits      8.787 i/s -      25.000 times in 2.844993s (113.80ms/i)

Comparison:
            baseline:         7.9 i/s 
         string_bits:         8.8 i/s - 1.11x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/bloom_filter.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     14.367 i/s -      16.000 times in 1.113695s (69.61ms/i)
         string_bits     13.673 i/s -      14.000 times in 1.023900s (73.14ms/i)
Calculating -------------------------------------
            baseline     13.869 i/s -      43.000 times in 3.100360s (72.10ms/i)
         string_bits     15.567 i/s -      41.000 times in 2.633711s (64.24ms/i)

Comparison:
            baseline:        13.9 i/s 
         string_bits:        15.6 i/s - 1.12x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== EACH_BIT ===
Warming up --------------------------------------
            baseline     17.893 i/s -      18.000 times in 1.005975s (55.89ms/i)
         string_bits     93.046 i/s -      99.000 times in 1.063987s (10.75ms/i)
Calculating -------------------------------------
            baseline     17.532 i/s -      53.000 times in 3.023077s (57.04ms/i)
         string_bits    105.668 i/s -     279.000 times in 2.640344s (9.46ms/i)

Comparison:
            baseline:        17.5 i/s 
         string_bits:       105.7 i/s - 6.03x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     48.701 i/s -      50.000 times in 1.026682s (20.53ms/i)
         string_bits    105.614 i/s -     110.000 times in 1.041528s (9.47ms/i)
Calculating -------------------------------------
            baseline     54.821 i/s -     146.000 times in 2.663205s (18.24ms/i)
         string_bits     99.695 i/s -     316.000 times in 3.169665s (10.03ms/i)

Comparison:
            baseline:        54.8 i/s 
         string_bits:        99.7 i/s - 1.82x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit_run.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== EACH_BIT_RUN ===
Warming up --------------------------------------
            baseline      9.979 i/s -      10.000 times in 1.002102s (100.21ms/i)
         string_bits     44.272 i/s -      45.000 times in 1.016433s (22.59ms/i)
Calculating -------------------------------------
            baseline      9.882 i/s -      29.000 times in 2.934626s (101.19ms/i)
         string_bits     48.381 i/s -     132.000 times in 2.728322s (20.67ms/i)

Comparison:
            baseline:         9.9 i/s 
         string_bits:        48.4 i/s - 4.90x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/each_bit_run.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     34.140 i/s -      36.000 times in 1.054475s (29.29ms/i)
         string_bits     44.150 i/s -      45.000 times in 1.019245s (22.65ms/i)
Calculating -------------------------------------
            baseline     33.921 i/s -     102.000 times in 3.006945s (29.48ms/i)
         string_bits     47.865 i/s -     132.000 times in 2.757734s (20.89ms/i)

Comparison:
            baseline:        33.9 i/s 
         string_bits:        47.9 i/s - 1.41x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/hamming.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== HAMMING ===
Warming up --------------------------------------
            baseline     38.921 i/s -      40.000 times in 1.027735s (25.69ms/i)
         string_bits    887.621 i/s -     902.000 times in 1.016199s (1.13ms/i)
Calculating -------------------------------------
            baseline     40.521 i/s -     116.000 times in 2.862704s (24.68ms/i)
         string_bits    853.453 i/s -      2.662k times in 3.119093s (1.17ms/i)

Comparison:
            baseline:        40.5 i/s 
         string_bits:       853.5 i/s - 21.06x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/hamming.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    346.055 i/s -     374.000 times in 1.080752s (2.89ms/i)
         string_bits     1.116k i/s -      1.199k times in 1.074556s (896.21μs/i)
Calculating -------------------------------------
            baseline    358.747 i/s -      1.038k times in 2.893402s (2.79ms/i)
         string_bits     1.099k i/s -      3.347k times in 3.045818s (910.01μs/i)

Comparison:
            baseline:       358.7 i/s 
         string_bits:      1098.9 i/s - 3.06x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/jaccard.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== JACCARD ===
Warming up --------------------------------------
            baseline     94.096 i/s -     100.000 times in 1.062746s (10.63ms/i)
         string_bits    20.371k i/s -     21.996k times in 1.079766s (49.09μs/i)
Calculating -------------------------------------
            baseline     90.326 i/s -     282.000 times in 3.122009s (11.07ms/i)
         string_bits    19.855k i/s -     61.113k times in 3.077953s (50.36μs/i)

Comparison:
            baseline:        90.3 i/s 
         string_bits:     19855.1 i/s - 219.81x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/jaccard.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline    184.104 i/s -     200.000 times in 1.086343s (5.43ms/i)
         string_bits    19.383k i/s -     20.776k times in 1.071865s (51.59μs/i)
Calculating -------------------------------------
            baseline    193.598 i/s -     552.000 times in 2.851263s (5.17ms/i)
         string_bits    19.744k i/s -     58.149k times in 2.945143s (50.65μs/i)

Comparison:
            baseline:       193.6 i/s 
         string_bits:     19744.0 i/s - 101.98x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/rle.yaml --executables 'ruby::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby' --output faster

=== RLE ===
Warming up --------------------------------------
            baseline     10.031 i/s -      11.000 times in 1.096591s (99.69ms/i)
            each_bit     14.275 i/s -      16.000 times in 1.120874s (70.05ms/i)
        each_bit_run     42.578 i/s -      44.000 times in 1.033403s (23.49ms/i)
Calculating -------------------------------------
            baseline      9.371 i/s -      30.000 times in 3.201455s (106.72ms/i)
            each_bit     13.093 i/s -      42.000 times in 3.207741s (76.37ms/i)
        each_bit_run     43.668 i/s -     127.000 times in 2.908310s (22.90ms/i)

Comparison:
            baseline:         9.4 i/s 
            each_bit:        13.1 i/s - 1.40x  faster
        each_bit_run:        43.7 i/s - 4.66x  faster

RUBYLIB=/home/hasumi/work/string_bits/lib bundle exec benchmark-driver benchmark/rle.yaml --executables 'yjit::/home/hasumi/.rbenv/versions/4.0.4/bin/ruby --yjit' --output faster
Warming up --------------------------------------
            baseline     34.156 i/s -      36.000 times in 1.053978s (29.28ms/i)
            each_bit     24.222 i/s -      27.000 times in 1.114685s (41.28ms/i)
        each_bit_run     42.845 i/s -      44.000 times in 1.026966s (23.34ms/i)
Calculating -------------------------------------
            baseline     31.885 i/s -     102.000 times in 3.199013s (31.36ms/i)
            each_bit     24.309 i/s -      72.000 times in 2.961883s (41.14ms/i)
        each_bit_run     43.771 i/s -     128.000 times in 2.924299s (22.85ms/i)

Comparison:
            baseline:        31.9 i/s 
            each_bit:        24.3 i/s - 1.31x  slower
        each_bit_run:        43.8 i/s - 1.37x  faster

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
  string_bits: bit_set(range) x1000                                               5

  Array#mask (100K elements, ~50% masked)
  -----------------------------------------------------------------------------------
  baseline:    each_byte + byte[bit] loop                                         7
  string_bits: mask  (returns new Array)                                          6
  string_bits: mask! (in-place, needs dup)                                        7
```
