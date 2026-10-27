### Risky256 / Risky512 SMHasher Results

SMHasher passes a 32-bit seed to every hash, so Risky256 and Risky512 are tested through their keyed variants (`fio_risky256_keyed` / `fio_risky512_keyed`) with the seed as the key. SMHasher tests at most 256 bits, so Risky512 is tested on its high 256 bits (its low 256 bits equal Risky256). Runs marked `--extra` include tests (such as BIC) that SMHasher otherwise skips for wide hashes. Compare with the SHA-2 rows of the summary table.

**Risky256 (keyed)**:

```txt
# SMHasher fio-risky256
# date: 2026-11-01T07:33:22.127Z
# library: 615c171, fio-stl.h sha256 fd8359f5f75947b0
# tools: SMHasher e154a82a5a9a (+facil.io patch), PractRand 0.95, xoshiro hwd.c 97ec22f5b2e7
# machine: Apple M4 Max, Apple clang version 21.0.0 (clang-2100.3.34.2)
# note: on Apple Silicon SMHasher counts timer ticks x64, not CPU cycles: 'MiB/sec' is correct,
#       'cycles/hash' / (MHz/1000) = ns/hash, and HashMapTest PASS/FAIL (a 1500-cycle threshold) is meaningless.
# 
-------------------------------------------------------------------------------
--- Testing fio-risky256 "facil.io Risky256 keyed (key = seed)" GOOD

[[[ Sanity Tests ]]]

Verification value 0xE6AD4AC9 ....... PASS
Running sanity check 1       .......... PASS
Running AppendedZeroesTest .......... PASS

[[[ Speed Tests ]]]

WARNING: timer resolution is 9223372036854775807 (0x7fffffffffffffff) ticks (0xd79727d67b5d00 - 0xd79727d67b5d00). Broken VDSO?
Bulk speed test - 262144-byte keys
Alignment  7 -  0.265 bytes/cycle - 16154.95 MiB/sec @ 64000 MHz
Alignment  6 -  0.265 bytes/cycle - 16202.63 MiB/sec @ 64000 MHz
Alignment  5 -  0.266 bytes/cycle - 16231.72 MiB/sec @ 64000 MHz
Alignment  4 -  0.266 bytes/cycle - 16256.39 MiB/sec @ 64000 MHz
Alignment  3 -  0.266 bytes/cycle - 16210.05 MiB/sec @ 64000 MHz
Alignment  2 -  0.266 bytes/cycle - 16243.78 MiB/sec @ 64000 MHz
Alignment  1 -  0.263 bytes/cycle - 16051.00 MiB/sec @ 64000 MHz
Alignment  0 -  0.249 bytes/cycle - 15209.82 MiB/sec @ 64000 MHz
Average      -  0.263 bytes/cycle - 16070.04 MiB/sec @ 64000 MHz

Small key speed test -    1-byte keys -  2014.19 cycles/hash
Small key speed test -    2-byte keys -  1996.54 cycles/hash
Small key speed test -    3-byte keys -  2013.00 cycles/hash
Small key speed test -    4-byte keys -  2085.54 cycles/hash
Small key speed test -    5-byte keys -  1985.03 cycles/hash
Small key speed test -    6-byte keys -  1987.82 cycles/hash
Small key speed test -    7-byte keys -  2002.06 cycles/hash
Small key speed test -    8-byte keys -  1990.23 cycles/hash
Small key speed test -    9-byte keys -  1984.84 cycles/hash
Small key speed test -   10-byte keys -  1986.89 cycles/hash
Small key speed test -   11-byte keys -  1982.37 cycles/hash
Small key speed test -   12-byte keys -  1985.85 cycles/hash
Small key speed test -   13-byte keys -  1987.13 cycles/hash
Small key speed test -   14-byte keys -  1987.24 cycles/hash
Small key speed test -   15-byte keys -  1987.85 cycles/hash
Small key speed test -   16-byte keys -  1989.85 cycles/hash
Small key speed test -   17-byte keys -  2026.98 cycles/hash
Small key speed test -   18-byte keys -  2026.77 cycles/hash
Small key speed test -   19-byte keys -  2026.58 cycles/hash
Small key speed test -   20-byte keys -  2033.29 cycles/hash
Small key speed test -   21-byte keys -  2031.78 cycles/hash
Small key speed test -   22-byte keys -  2149.59 cycles/hash
Small key speed test -   23-byte keys -  2112.92 cycles/hash
Small key speed test -   24-byte keys -  2001.05 cycles/hash
Small key speed test -   25-byte keys -  2001.25 cycles/hash
Small key speed test -   26-byte keys -  2001.36 cycles/hash
Small key speed test -   27-byte keys -  2005.98 cycles/hash
Small key speed test -   28-byte keys -  2001.52 cycles/hash
Small key speed test -   29-byte keys -  2002.03 cycles/hash
Small key speed test -   30-byte keys -  2002.19 cycles/hash
Small key speed test -   31-byte keys -  2001.54 cycles/hash
Small key speed test -   32-byte keys -  1989.20 cycles/hash
Average                                  2011.890 cycles/hash
Average, weighted by key length freq.    2008.072 cycles/hash (using 93.0% of top-7m Tranco DNS names dataset)
Average, weighted by key length freq.    1988.921 cycles/hash (using 27.1% of startup-1M UMASH trace dataset)

[[[ 'Hashmap' Speed Tests ]]]

std::unordered_map
Init std HashMapTest:     4082.370 cycles/op (235976 inserts, 1% deletions)
Running std HashMapTest:  3631.235 cycles/op (38.6 stdv)

greg7mdp/parallel-hashmap
Init fast HashMapTest:    5154.619 cycles/op (235976 inserts, 1% deletions)
Running fast HashMapTest: 3586.662 cycles/op (687.1 stdv)  ....... FAIL

[[[ Avalanche Tests ]]]

Testing   24-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.730000%
Testing   32-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.713333%
Testing   40-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.714667%
Testing   48-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.763333%
Testing   56-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.757333%
Testing   64-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.693333%
Testing   72-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.726667%
Testing   80-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.854000%
Testing   96-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.751333%
Testing  112-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.839333%
Testing  128-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.734667%
Testing  160-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.795333%

[[[ Keyset 'Sparse' Tests ]]]

Keyset 'Sparse' - 16-bit keys with up to 9 bits set - 50643 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (high 19-25 bits) - Worst is 23 bits: 183/152 (1.20x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (low  19-25 bits) - Worst is 21 bits: 630/606 (1.04x)
Testing distribution - Worst bias is the 13-bit window at bit 250 - 0.665%

Keyset 'Sparse' - 24-bit keys with up to 8 bits set - 1271626 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        188.2, actual    190 (1.01x) (2)
Testing collisions (high 24-35 bits) - Worst is 33 bits: 96/94 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        188.2, actual    162 (0.86x)
Testing collisions (low  24-35 bits) - Worst is 26 bits: 11998/11972 (1.00x)
Testing distribution - Worst bias is the 16-bit window at bit 168 - 0.092%

Keyset 'Sparse' - 32-bit keys with up to 7 bits set - 4514873 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2372.2, actual   2380 (1.00x) (8)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 77/74 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2372.2, actual   2352 (0.99x) (-20)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 39/37 (1.05x)
Testing distribution - Worst bias is the 19-bit window at bit 100 - 0.058%

Keyset 'Sparse' - 40-bit keys with up to 6 bits set - 4598479 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2460.8, actual   2416 (0.98x) (-44)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 82/76 (1.07x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2460.8, actual   2469 (1.00x) (9)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 43/38 (1.12x)
Testing distribution - Worst bias is the 19-bit window at bit 205 - 0.055%

Keyset 'Sparse' - 48-bit keys with up to 6 bits set - 14196869 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      23437.8, actual  23394 (1.00x) (-43)
Testing collisions (high 27-42 bits) - Worst is 35 bits: 2999/2932 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      23437.8, actual  23450 (1.00x) (13)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 26/22 (1.13x)
Testing distribution - Worst bias is the 20-bit window at bit  8 - 0.028%

Keyset 'Sparse' - 56-bit keys with up to 5 bits set - 4216423 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2069.0, actual   2099 (1.01x) (31)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 134/129 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2069.0, actual   2098 (1.01x) (30)
Testing collisions (low  25-38 bits) - Worst is 36 bits: 136/129 (1.05x)
Testing distribution - Worst bias is the 19-bit window at bit 51 - 0.075%

Keyset 'Sparse' - 64-bit keys with up to 5 bits set - 8303633 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8021.7, actual   8168 (1.02x) (147)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 140/125 (1.12x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       8021.7, actual   7947 (0.99x) (-74)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 42/31 (1.34x)
Testing distribution - Worst bias is the 20-bit window at bit 167 - 0.048%

Keyset 'Sparse' - 72-bit keys with up to 5 bits set - 15082603 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      26451.8, actual  26509 (1.00x) (58)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 27/25 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      26451.8, actual  26553 (1.00x) (102)
Testing collisions (low  27-42 bits) - Worst is 34 bits: 6759/6618 (1.02x)
Testing distribution - Worst bias is the 20-bit window at bit 61 - 0.027%

Keyset 'Sparse' - 96-bit keys with up to 4 bits set - 3469497 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1401.0, actual   1408 (1.01x) (8)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 28/21 (1.28x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1401.0, actual   1328 (0.95x)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 24/21 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit 131 - 0.079%

Keyset 'Sparse' - 160-bit keys with up to 4 bits set - 26977161 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      84546.1, actual  84732 (1.00x) (186)
Testing collisions (high 28-44 bits) - Worst is 39 bits: 677/661 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      84546.1, actual  84221 (1.00x) (-325)
Testing collisions (low  28-44 bits) - Worst is 44 bits: 29/20 (1.40x)
Testing distribution - Worst bias is the 20-bit window at bit 184 - 0.014%

Keyset 'Sparse' - 256-bit keys with up to 3 bits set - 2796417 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        910.2, actual    882 (0.97x)
Testing collisions (high 25-37 bits) - Worst is 37 bits: 34/28 (1.20x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        910.2, actual    946 (1.04x) (36)
Testing collisions (low  25-37 bits) - Worst is 35 bits: 130/113 (1.14x)
Testing distribution - Worst bias is the 19-bit window at bit 120 - 0.105%


[[[ Keyset 'Permutation' Tests ]]]

Combination Lowbits Tests:
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    623 (0.93x)
Testing collisions (high 24-37 bits) - Worst is 37 bits: 27/20 (1.29x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        668.6, actual    660 (0.99x) (-8)
Testing collisions (low  24-37 bits) - Worst is 35 bits: 91/83 (1.09x)
Testing distribution - Worst bias is the 17-bit window at bit 140 - 0.080%


Combination Highbits Tests
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    670 (1.00x) (2)
Testing collisions (high 24-37 bits) - Worst is 33 bits: 345/334 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        668.6, actual    684 (1.02x) (16)
Testing collisions (low  24-37 bits) - Worst is 37 bits: 28/20 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 53 - 0.077%


Combination Hi-Lo Tests:
Keyset 'Combination' - up to 6 blocks from a set of 15 - 12204240 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      17322.9, actual  17407 (1.00x) (85)
Testing collisions (high 27-41 bits) - Worst is 40 bits: 76/67 (1.12x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      17322.9, actual  17041 (0.98x) (-281)
Testing collisions (low  27-41 bits) - Worst is 40 bits: 72/67 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit 146 - 0.027%


Combination 0x80000000 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual     10 (1.25x) (3)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 39/31 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (low  21-30 bits) - Worst is 26 bits: 517/511 (1.01x)
Testing distribution - Worst bias is the 15-bit window at bit 51 - 0.309%


Combination 0x00000001 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 28 bits: 130/127 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      5 (0.63x)
Testing collisions (low  21-30 bits) - Worst is 28 bits: 145/127 (1.13x)
Testing distribution - Worst bias is the 15-bit window at bit 244 - 0.264%


Combination 0x8000000000000000 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      6 (0.75x)
Testing collisions (high 21-30 bits) - Worst is 22 bits: 7983/8023 (0.99x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual     11 (1.38x) (4)
Testing collisions (low  21-30 bits) - Worst is 30 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 15-bit window at bit 37 - 0.241%


Combination 0x0000000000000001 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 25 bits: 1030/1021 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      5 (0.63x)
Testing collisions (low  21-30 bits) - Worst is 25 bits: 1036/1021 (1.01x)
Testing distribution - Worst bias is the 15-bit window at bit 74 - 0.227%


Combination 16-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual     11 (1.38x) (4)
Testing collisions (high 21-30 bits) - Worst is 23 bits: 4110/4053 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (low  21-30 bits) - Worst is 23 bits: 4099/4053 (1.01x)
Testing distribution - Worst bias is the 15-bit window at bit 35 - 0.270%


Combination 16-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual     11 (1.38x) (4)
Testing collisions (high 21-30 bits) - Worst is 28 bits: 146/127 (1.14x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual     12 (1.50x) (5)
Testing collisions (low  21-30 bits) - Worst is 30 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 15-bit window at bit 23 - 0.230%


Combination 32-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (high 21-30 bits) - Worst is 29 bits: 71/63 (1.11x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      9 (1.13x) (2)
Testing collisions (low  21-30 bits) - Worst is 29 bits: 78/63 (1.22x)
Testing distribution - Worst bias is the 15-bit window at bit 167 - 0.206%


Combination 32-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      9 (1.13x) (2)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 40/31 (1.25x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      2 (0.25x)
Testing collisions (low  21-30 bits) - Worst is 28 bits: 128/127 (1.00x)
Testing distribution - Worst bias is the 15-bit window at bit 76 - 0.245%


Combination 64-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      3 (0.38x)
Testing collisions (high 21-30 bits) - Worst is 26 bits: 519/511 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      3 (0.38x)
Testing collisions (low  21-30 bits) - Worst is 23 bits: 4181/4053 (1.03x)
Testing distribution - Worst bias is the 15-bit window at bit 179 - 0.213%


Combination 64-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (high 21-30 bits) - Worst is 28 bits: 138/127 (1.08x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      5 (0.63x)
Testing collisions (low  21-30 bits) - Worst is 25 bits: 1019/1021 (1.00x)
Testing distribution - Worst bias is the 15-bit window at bit 247 - 0.326%


Combination 128-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      6 (0.75x)
Testing collisions (high 21-30 bits) - Worst is 27 bits: 272/255 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (low  21-30 bits) - Worst is 28 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 15-bit window at bit 115 - 0.290%


Combination 128-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 36/31 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      6 (0.75x)
Testing collisions (low  21-30 bits) - Worst is 30 bits: 42/31 (1.31x)
Testing distribution - Worst bias is the 15-bit window at bit 192 - 0.269%


[[[ Keyset 'Window' Tests ]]]

Keyset 'Window' -  32-bit key,  25-bit window - 32 tests, 33554432 keys per test
Window at   0 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   1 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   2 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   3 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   4 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   5 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   6 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   7 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   8 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   9 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  10 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  11 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  12 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  13 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  14 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  15 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  16 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  17 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  18 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  19 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  20 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  21 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  22 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  23 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  24 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  25 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  26 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  27 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  28 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  29 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  30 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  31 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  32 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)

[[[ Keyset 'Cyclic' Tests ]]]

Keyset 'Cyclic' - 8 cycles of 32 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    117 (1.01x) (1)
Testing collisions (high 23-34 bits) - Worst is 31 bits: 236/232 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    127 (1.09x) (11)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 35/29 (1.20x)
Testing distribution - Worst bias is the 17-bit window at bit 113 - 0.195%

Keyset 'Cyclic' - 8 cycles of 33 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    121 (1.04x) (5)
Testing collisions (high 23-34 bits) - Worst is 32 bits: 121/116 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    114 (0.98x)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 35/29 (1.20x)
Testing distribution - Worst bias is the 17-bit window at bit 125 - 0.123%

Keyset 'Cyclic' - 8 cycles of 34 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    121 (1.04x) (5)
Testing collisions (high 23-34 bits) - Worst is 33 bits: 66/58 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    116 (1.00x)
Testing collisions (low  23-34 bits) - Worst is 33 bits: 61/58 (1.05x)
Testing distribution - Worst bias is the 17-bit window at bit 174 - 0.161%

Keyset 'Cyclic' - 8 cycles of 35 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    114 (0.98x)
Testing collisions (high 23-34 bits) - Worst is 28 bits: 1911/1860 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    110 (0.94x)
Testing collisions (low  23-34 bits) - Worst is 28 bits: 1879/1860 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 214 - 0.151%

Keyset 'Cyclic' - 8 cycles of 36 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    107 (0.92x)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 37/29 (1.27x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual     99 (0.85x)
Testing collisions (low  23-34 bits) - Worst is 26 bits: 7457/7413 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 166 - 0.151%

Keyset 'Cyclic' - 8 cycles of 40 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    126 (1.08x) (10)
Testing collisions (high 23-34 bits) - Worst is 33 bits: 67/58 (1.15x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual     99 (0.85x)
Testing collisions (low  23-34 bits) - Worst is 26 bits: 7508/7413 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 65 - 0.169%


[[[ Keyset 'TwoBytes' Tests ]]]

Keyset 'TwoBytes' - up-to-4-byte keys, 652545 total keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         49.6, actual     43 (0.87x)
Testing collisions (high 23-33 bits) - Worst is 26 bits: 3185/3162 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected         49.6, actual     49 (0.99x)
Testing collisions (low  23-33 bits) - Worst is 30 bits: 220/198 (1.11x)
Testing distribution - Worst bias is the 16-bit window at bit 143 - 0.180%

Keyset 'TwoBytes' - up-to-8-byte keys, 5471025 total keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       3483.1, actual   3467 (1.00x) (-16)
Testing collisions (high 26-39 bits) - Worst is 34 bits: 906/871 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       3483.1, actual   3558 (1.02x) (75)
Testing collisions (low  26-39 bits) - Worst is 37 bits: 114/108 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 217 - 0.063%

Keyset 'TwoBytes' - up-to-12-byte keys, 18616785 total keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      40289.5, actual  40043 (0.99x) (-246)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 46/39 (1.17x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      40289.5, actual  40082 (0.99x) (-207)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 83/78 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 148 - 0.019%


[[[ Keyset 'Text' Tests ]]]

Keyset 'Text' - keys of form "FooXXXXBar" - 14776336 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25562 (1.01x) (173)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 58/49 (1.17x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25223 (0.99x) (-166)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 54/49 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 203 - 0.028%

Keyset 'Text' - keys of form "FooBarXXXX" - 14776336 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25328 (1.00x) (-61)
Testing collisions (high 27-42 bits) - Worst is 40 bits: 116/99 (1.17x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25687 (1.01x) (298)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 56/49 (1.13x)
Testing distribution - Worst bias is the 20-bit window at bit 216 - 0.024%

Keyset 'Text' - keys of form "XXXXFooBar" - 14776336 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25360 (1.00x) (-29)
Testing collisions (high 27-42 bits) - Worst is 38 bits: 415/397 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25213 (0.99x) (-176)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 67/49 (1.35x)
Testing distribution - Worst bias is the 20-bit window at bit 53 - 0.025%

Keyset 'Words' - 4000000 random keys of len 6-16 from alnum charset
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1869 (1.00x) (7)
Testing collisions (high 25-38 bits) - Worst is 34 bits: 490/465 (1.05x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1867 (1.00x) (5)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 66/58 (1.13x)
Testing distribution - Worst bias is the 19-bit window at bit 243 - 0.075%

Keyset 'Words' - 4000000 random keys of len 6-16 from password charset
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1895 (1.02x) (33)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 78/58 (1.34x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1837 (0.99x) (-25)
Testing collisions (low  25-38 bits) - Worst is 34 bits: 482/465 (1.04x)
Testing distribution - Worst bias is the 19-bit window at bit 127 - 0.061%

Keyset 'Words' - 235976 dict words
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          6.5, actual      6 (0.93x)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 27/25 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          6.5, actual      6 (0.93x)
Testing collisions (low  21-30 bits) - Worst is 30 bits: 29/25 (1.12x)
Testing distribution - Worst bias is the 15-bit window at bit 133 - 0.322%


[[[ Keyset 'Zeroes' Tests ]]]

Keyset 'Zeroes' - 204800 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          4.9, actual      5 (1.02x) (1)
Testing collisions (high 21-29 bits) - Worst is 25 bits: 621/623 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          4.9, actual      2 (0.41x)
Testing collisions (low  21-29 bits) - Worst is 29 bits: 50/39 (1.28x)
Testing distribution - Worst bias is the 15-bit window at bit 73 - 0.368%


[[[ Keyset 'Seed' Tests ]]]

Keyset 'Seed' - 5000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2909.3, actual   2919 (1.00x) (10)
Testing collisions (high 26-39 bits) - Worst is 33 bits: 1499/1454 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2909.3, actual   2951 (1.01x) (42)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 33/22 (1.45x)
Testing distribution - Worst bias is the 19-bit window at bit 64 - 0.054%


[[[ Keyset 'PerlinNoise' Tests ]]]

Testing 16777216 coordinates (L2) : 
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      32725.4, actual  32860 (1.00x) (135)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 36/31 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      32725.4, actual  32645 (1.00x) (-80)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 66/63 (1.03x)

Testing AV variant, 128 count with 4 spacing, 4-12:
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1116.2, actual   1141 (1.02x) (25)
Testing collisions (high 25-37 bits) - Worst is 31 bits: 2296/2231 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1116.2, actual   1151 (1.03x) (35)
Testing collisions (low  25-37 bits) - Worst is 34 bits: 292/279 (1.05x)


[[[ Diff 'Differential' Tests ]]]

Testing 8303632 up-to-5-bit differentials in 64-bit keys -> 256 bit hashes.
100 reps, 830363200 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 11017632 up-to-4-bit differentials in 128-bit keys -> 256 bit hashes.
100 reps, 1101763200 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 2796416 up-to-3-bit differentials in 256-bit keys -> 256 bit hashes.
100 reps, 279641600 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored


[[[ DiffDist 'Differential Distribution' Tests ]]]

Testing bit 0
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    474 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16436/16298 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    476 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 65/63 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 104 - 0.095%

Testing bit 1
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    523 (1.02x) (12)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing distribution - Worst bias is the 18-bit window at bit 212 - 0.088%

Testing bit 2
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    476 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32532/32429 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.083%

Testing bit 3
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 533/511 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 206 - 0.101%

Testing bit 4
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    487 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16391/16298 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125897/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 133 - 0.082%

Testing bit 5
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 82/63 (1.28x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 67 - 0.103%

Testing bit 6
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 222 - 0.075%

Testing bit 7
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 205 - 0.083%

Testing bit 8
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    484 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 112 - 0.112%

Testing bit 9
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    487 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16464/16298 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 160 - 0.090%

Testing bit 10
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    475 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 103 - 0.104%

Testing bit 11
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125950/125777 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4146/4090 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 103 - 0.090%

Testing bit 12
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 45/31 (1.41x)
Testing distribution - Worst bias is the 18-bit window at bit 211 - 0.088%

Testing bit 13
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    551 (1.08x) (40)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.087%

Testing bit 14
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 148 - 0.084%

Testing bit 15
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    568 (1.11x) (57)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    536 (1.05x) (25)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 286/255 (1.12x)
Testing distribution - Worst bias is the 18-bit window at bit 186 - 0.093%

Testing bit 16
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 134/127 (1.05x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 240 - 0.103%

Testing bit 17
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8271/8170 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    531 (1.04x) (20)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 272/255 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 161 - 0.088%

Testing bit 18
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 276/255 (1.08x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    540 (1.05x) (29)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 54 - 0.092%

Testing bit 19
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    566 (1.11x) (55)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 79/63 (1.23x)
Testing distribution - Worst bias is the 18-bit window at bit 135 - 0.082%

Testing bit 20
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    490 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 46 - 0.083%

Testing bit 21
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 272/255 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 160 - 0.085%

Testing bit 22
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    476 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    523 (1.02x) (12)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.087%

Testing bit 23
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 272/255 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 64 - 0.101%

Testing bit 24
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1066/1023 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 229 - 0.117%

Testing bit 25
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 141/127 (1.10x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1039/1023 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 75 - 0.089%

Testing bit 26
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 518/511 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 116 - 0.095%

Testing bit 27
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    476 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 235 - 0.094%

Testing bit 28
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    454 (0.89x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125877/125777 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing distribution - Worst bias is the 18-bit window at bit 135 - 0.137%

Testing bit 29
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4156/4090 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 524/511 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 170 - 0.112%

Testing bit 30
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    482 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125491/125777 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 81/63 (1.27x)
Testing distribution - Worst bias is the 18-bit window at bit 254 - 0.103%

Testing bit 31
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    555 (1.08x) (44)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 82/63 (1.28x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    447 (0.87x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125858/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 201 - 0.090%

Testing bit 32
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 280/255 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 201 - 0.087%

Testing bit 33
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 68/63 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 204 - 0.093%

Testing bit 34
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8233/8170 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 150 - 0.089%

Testing bit 35
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32809/32429 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    479 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32543/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 219 - 0.086%

Testing bit 36
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    490 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8146/8170 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 99 - 0.121%

Testing bit 37
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    547 (1.07x) (36)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    468 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32586/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 111 - 0.089%

Testing bit 38
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 272/255 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2081/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 226 - 0.079%

Testing bit 39
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32536/32429 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 27 bits: 16280/16298 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 56 - 0.085%

Testing bit 40
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2134/2046 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 170 - 0.104%

Testing bit 41
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125755/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.106%

Testing bit 42
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125994/125777 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    473 (0.92x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 237 - 0.079%

Testing bit 43
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64635/64191 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 239 - 0.101%

Testing bit 44
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1054/1023 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 40 - 0.120%

Testing bit 45
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    558 (1.09x) (47)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 212 - 0.086%

Testing bit 46
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 167 - 0.112%

Testing bit 47
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    490 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8225/8170 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 159/127 (1.24x)
Testing distribution - Worst bias is the 18-bit window at bit 156 - 0.096%

Testing bit 48
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 114 - 0.106%

Testing bit 49
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    486 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 220 - 0.095%

Testing bit 50
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    541 (1.06x) (30)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 541/511 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    480 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.091%

Testing bit 51
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    444 (0.87x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125842/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 255 - 0.083%

Testing bit 52
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    477 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8357/8170 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 28 - 0.098%

Testing bit 53
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 111 - 0.095%

Testing bit 54
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing distribution - Worst bias is the 18-bit window at bit 43 - 0.090%

Testing bit 55
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 75/63 (1.17x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 190 - 0.096%

Testing bit 56
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8316/8170 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 133/127 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 18 - 0.113%

Testing bit 57
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 265/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.123%

Testing bit 58
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1085/1023 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 17 - 0.090%

Testing bit 59
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    547 (1.07x) (36)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 202 - 0.101%

Testing bit 60
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4172/4090 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2136/2046 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 228 - 0.074%

Testing bit 61
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 538/511 (1.05x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1059/1023 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 192 - 0.122%

Testing bit 62
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 538/511 (1.05x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing distribution - Worst bias is the 18-bit window at bit 208 - 0.086%

Testing bit 63
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 267/255 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 265/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 102 - 0.093%


[[[ MomentChi2 Tests ]]]

Analyze hashes produced from a serie of linearly increasing numbers of 32-bit, using a step of 6 ... 
Target values to approximate : 38918200.000000 - 820900.000000 
Popcount 1 stats : 38918908.713247 - 820879.935146
Popcount 0 stats : 38918260.662289 - 820917.952467
MomentChi2 for bits 1 :  0.305933 
MomentChi2 for bits 0 :  0.00224137 

Derivative stats (transition from 2 consecutive values) : 
Popcount 1 stats : 38920947.277544 - 821030.055877
Popcount 0 stats : 38917100.038414 - 820886.933836
MomentChi2 for deriv b1 :   4.59675 
MomentChi2 for deriv b0 :   0.73695 

  Great 


[[[ Prng Tests ]]]

Generating 33554432 random numbers : 
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     130731.3, actual 130724 (1.00x) (-7)
Testing collisions (high 28-44 bits) - Worst is 43 bits: 72/63 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected     130731.3, actual 130410 (1.00x) (-321)
Testing collisions (low  28-44 bits) - Worst is 44 bits: 34/31 (1.06x)

[[[ BadSeeds Tests ]]]

0x0 PASS


Input vcode 0x00000001, Output vcode 0x00000001, Result vcode 0x00000001
Verification value is 0x00000001 - Testing took 686.364475 seconds
-------------------------------------------------------------------------------
```

**Risky256, low 64 bits** (SMHasher runs more collision tests on 64-bit outputs):

```txt
# SMHasher fio-risky256_64
# date: 2026-11-01T07:44:48.683Z
# library: 615c171, fio-stl.h sha256 fd8359f5f75947b0
# tools: SMHasher e154a82a5a9a (+facil.io patch), PractRand 0.95, xoshiro hwd.c 97ec22f5b2e7
# machine: Apple M4 Max, Apple clang version 21.0.0 (clang-2100.3.34.2)
# note: on Apple Silicon SMHasher counts timer ticks x64, not CPU cycles: 'MiB/sec' is correct,
#       'cycles/hash' / (MHz/1000) = ns/hash, and HashMapTest PASS/FAIL (a 1500-cycle threshold) is meaningless.
# 
-------------------------------------------------------------------------------
--- Testing fio-risky256_64 "facil.io Risky256 keyed, low 64 bits" GOOD

[[[ Sanity Tests ]]]

Verification value 0x71DF5C7A ....... PASS
Running sanity check 1       .......... PASS
Running AppendedZeroesTest .......... PASS

[[[ Speed Tests ]]]

WARNING: timer resolution is 9223372036854775807 (0x7fffffffffffffff) ticks (0xd7bf1bf6bb19c0 - 0xd7bf1bf6bb19c0). Broken VDSO?
Bulk speed test - 262144-byte keys
Alignment  7 -  0.262 bytes/cycle - 15994.60 MiB/sec @ 64000 MHz
Alignment  6 -  0.263 bytes/cycle - 16051.47 MiB/sec @ 64000 MHz
Alignment  5 -  0.261 bytes/cycle - 15909.00 MiB/sec @ 64000 MHz
Alignment  4 -  0.262 bytes/cycle - 15973.30 MiB/sec @ 64000 MHz
Alignment  3 -  0.261 bytes/cycle - 15905.47 MiB/sec @ 64000 MHz
Alignment  2 -  0.265 bytes/cycle - 16162.03 MiB/sec @ 64000 MHz
Alignment  1 -  0.267 bytes/cycle - 16325.96 MiB/sec @ 64000 MHz
Alignment  0 -  0.271 bytes/cycle - 16524.87 MiB/sec @ 64000 MHz
Average      -  0.264 bytes/cycle - 16105.84 MiB/sec @ 64000 MHz

Small key speed test -    1-byte keys -  1997.04 cycles/hash
Small key speed test -    2-byte keys -  1979.83 cycles/hash
Small key speed test -    3-byte keys -  2010.86 cycles/hash
Small key speed test -    4-byte keys -  1990.86 cycles/hash
Small key speed test -    5-byte keys -  1998.69 cycles/hash
Small key speed test -    6-byte keys -  2008.66 cycles/hash
Small key speed test -    7-byte keys -  2038.03 cycles/hash
Small key speed test -    8-byte keys -  2038.75 cycles/hash
Small key speed test -    9-byte keys -  2022.60 cycles/hash
Small key speed test -   10-byte keys -  2031.19 cycles/hash
Small key speed test -   11-byte keys -  2010.59 cycles/hash
Small key speed test -   12-byte keys -  1988.32 cycles/hash
Small key speed test -   13-byte keys -  1987.73 cycles/hash
Small key speed test -   14-byte keys -  1997.57 cycles/hash
Small key speed test -   15-byte keys -  1984.94 cycles/hash
Small key speed test -   16-byte keys -  2003.13 cycles/hash
Small key speed test -   17-byte keys -  2018.29 cycles/hash
Small key speed test -   18-byte keys -  2031.25 cycles/hash
Small key speed test -   19-byte keys -  2039.50 cycles/hash
Small key speed test -   20-byte keys -  2056.64 cycles/hash
Small key speed test -   21-byte keys -  2030.98 cycles/hash
Small key speed test -   22-byte keys -  2032.15 cycles/hash
Small key speed test -   23-byte keys -  2005.62 cycles/hash
Small key speed test -   24-byte keys -  1994.40 cycles/hash
Small key speed test -   25-byte keys -  1993.69 cycles/hash
Small key speed test -   26-byte keys -  1996.53 cycles/hash
Small key speed test -   27-byte keys -  1995.95 cycles/hash
Small key speed test -   28-byte keys -  2001.31 cycles/hash
Small key speed test -   29-byte keys -  2029.84 cycles/hash
Small key speed test -   30-byte keys -  2032.59 cycles/hash
Small key speed test -   31-byte keys -  2031.20 cycles/hash
Small key speed test -   32-byte keys -  2033.89 cycles/hash
Average                                  2012.894 cycles/hash
Average, weighted by key length freq.    2012.623 cycles/hash (using 93.0% of top-7m Tranco DNS names dataset)
Average, weighted by key length freq.    2011.068 cycles/hash (using 27.1% of startup-1M UMASH trace dataset)

[[[ 'Hashmap' Speed Tests ]]]

std::unordered_map
Init std HashMapTest:     4220.047 cycles/op (235976 inserts, 1% deletions)
Running std HashMapTest:  3808.049 cycles/op (161.7 stdv)

greg7mdp/parallel-hashmap
Init fast HashMapTest:    5154.710 cycles/op (235976 inserts, 1% deletions)
Running fast HashMapTest: 3949.977 cycles/op (526.6 stdv)  ....... FAIL

[[[ Avalanche Tests ]]]

Testing   24-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.615333%
Testing   32-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.624667%
Testing   40-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.627333%
Testing   48-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.676000%
Testing   56-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.712667%
Testing   64-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.668000%
Testing   72-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.648000%
Testing   80-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.668667%
Testing   96-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.690667%
Testing  112-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.678667%
Testing  128-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.726000%
Testing  160-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.701333%
Testing  512-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.763333%
Testing 1024-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.843333%

[[[ Keyset 'Sparse' Tests ]]]

Keyset 'Sparse' - 16-bit keys with up to 9 bits set - 50643 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          0.3, actual      1 (3.35x) (1) !
Testing collisions (high 19-25 bits) - Worst is 20 bits: 1234/1203 (1.03x)
Testing collisions (low  32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (low  19-25 bits) - Worst is 21 bits: 630/606 (1.04x)
Testing distribution - Worst bias is the 13-bit window at bit 57 - 0.670%

Keyset 'Sparse' - 24-bit keys with up to 8 bits set - 1271626 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        188.2, actual    189 (1.00x) (1)
Testing collisions (high 24-35 bits) - Worst is 31 bits: 380/376 (1.01x)
Testing collisions (low  32-bit) - Expected        188.2, actual    162 (0.86x)
Testing collisions (low  24-35 bits) - Worst is 26 bits: 11998/11972 (1.00x)
Testing distribution - Worst bias is the 17-bit window at bit 44 - 0.075%

Keyset 'Sparse' - 32-bit keys with up to 7 bits set - 4514873 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2372.2, actual   2372 (1.00x)
Testing collisions (high 25-38 bits) - Worst is 33 bits: 1193/1186 (1.01x)
Testing collisions (low  32-bit) - Expected       2372.2, actual   2352 (0.99x) (-20)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 39/37 (1.05x)
Testing distribution - Worst bias is the 19-bit window at bit 26 - 0.050%

Keyset 'Sparse' - 40-bit keys with up to 6 bits set - 4598479 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2460.8, actual   2476 (1.01x) (16)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 82/76 (1.07x)
Testing collisions (low  32-bit) - Expected       2460.8, actual   2469 (1.00x) (9)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 43/38 (1.12x)
Testing distribution - Worst bias is the 19-bit window at bit 21 - 0.045%

Keyset 'Sparse' - 48-bit keys with up to 6 bits set - 14196869 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      23437.8, actual  23691 (1.01x) (254)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 25/22 (1.09x)
Testing collisions (low  32-bit) - Expected      23437.8, actual  23450 (1.00x) (13)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 26/22 (1.13x)
Testing distribution - Worst bias is the 20-bit window at bit  8 - 0.028%

Keyset 'Sparse' - 56-bit keys with up to 5 bits set - 4216423 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2069.0, actual   2075 (1.00x) (7)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 135/129 (1.04x)
Testing collisions (low  32-bit) - Expected       2069.0, actual   2098 (1.01x) (30)
Testing collisions (low  25-38 bits) - Worst is 36 bits: 136/129 (1.05x)
Testing distribution - Worst bias is the 19-bit window at bit  3 - 0.049%

Keyset 'Sparse' - 64-bit keys with up to 5 bits set - 8303633 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8021.7, actual   8068 (1.01x) (47)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 75/62 (1.20x)
Testing collisions (low  32-bit) - Expected       8021.7, actual   7947 (0.99x) (-74)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 42/31 (1.34x)
Testing distribution - Worst bias is the 20-bit window at bit 54 - 0.049%

Keyset 'Sparse' - 72-bit keys with up to 5 bits set - 15082603 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      26451.8, actual  26364 (1.00x) (-87)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 37/25 (1.43x)
Testing collisions (low  32-bit) - Expected      26451.8, actual  26553 (1.00x) (102)
Testing collisions (low  27-42 bits) - Worst is 34 bits: 6759/6618 (1.02x)
Testing distribution - Worst bias is the 20-bit window at bit 61 - 0.029%

Keyset 'Sparse' - 96-bit keys with up to 4 bits set - 3469497 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1401.0, actual   1457 (1.04x) (57)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 95/87 (1.08x)
Testing collisions (low  32-bit) - Expected       1401.0, actual   1328 (0.95x)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 24/21 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit 24 - 0.065%

Keyset 'Sparse' - 160-bit keys with up to 4 bits set - 26977161 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      84546.1, actual  84424 (1.00x) (-122)
Testing collisions (high 28-44 bits) - Worst is 38 bits: 1382/1323 (1.04x)
Testing collisions (low  32-bit) - Expected      84546.1, actual  84221 (1.00x) (-325)
Testing collisions (low  28-44 bits) - Worst is 44 bits: 29/20 (1.40x)
Testing distribution - Worst bias is the 20-bit window at bit 58 - 0.014%

Keyset 'Sparse' - 256-bit keys with up to 3 bits set - 2796417 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        910.2, actual    921 (1.01x) (11)
Testing collisions (high 25-37 bits) - Worst is 37 bits: 31/28 (1.09x)
Testing collisions (low  32-bit) - Expected        910.2, actual    946 (1.04x) (36)
Testing collisions (low  25-37 bits) - Worst is 35 bits: 130/113 (1.14x)
Testing distribution - Worst bias is the 19-bit window at bit 18 - 0.081%

Keyset 'Sparse' - 512-bit keys with up to 3 bits set - 22370049 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      58155.4, actual  57950 (1.00x) (-205)
Testing collisions (high 28-43 bits) - Worst is 36 bits: 3693/3640 (1.01x)
Testing collisions (low  32-bit) - Expected      58155.4, actual  58280 (1.00x) (125)
Testing collisions (low  28-43 bits) - Worst is 40 bits: 246/227 (1.08x)
Testing distribution - Worst bias is the 20-bit window at bit  3 - 0.013%

Keyset 'Sparse' - 1024-bit keys with up to 2 bits set - 524801 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         32.1, actual     37 (1.15x) (5)
Testing collisions (high 22-32 bits) - Worst is 32 bits: 37/32 (1.15x)
Testing collisions (low  32-bit) - Expected         32.1, actual     33 (1.03x) (1)
Testing collisions (low  22-32 bits) - Worst is 31 bits: 73/64 (1.14x)
Testing distribution - Worst bias is the 16-bit window at bit  4 - 0.113%

Keyset 'Sparse' - 2048-bit keys with up to 2 bits set - 2098177 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        512.4, actual    508 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 144/128 (1.12x)
Testing collisions (low  32-bit) - Expected        512.4, actual    507 (0.99x) (-5)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 41/32 (1.28x)
Testing distribution - Worst bias is the 18-bit window at bit 44 - 0.103%


[[[ Keyset 'Permutation' Tests ]]]

Combination Lowbits Tests:
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    668 (1.00x)
Testing collisions (high 24-37 bits) - Worst is 36 bits: 53/41 (1.27x)
Testing collisions (low  32-bit) - Expected        668.6, actual    660 (0.99x) (-8)
Testing collisions (low  24-37 bits) - Worst is 35 bits: 91/83 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 54 - 0.090%


Combination Highbits Tests
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    672 (1.01x) (4)
Testing collisions (high 24-37 bits) - Worst is 35 bits: 87/83 (1.04x)
Testing collisions (low  32-bit) - Expected        668.6, actual    684 (1.02x) (16)
Testing collisions (low  24-37 bits) - Worst is 37 bits: 28/20 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 58 - 0.044%


Combination Hi-Lo Tests:
Keyset 'Combination' - up to 6 blocks from a set of 15 - 12204240 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      17322.9, actual  17502 (1.01x) (180)
Testing collisions (high 27-41 bits) - Worst is 41 bits: 39/33 (1.15x)
Testing collisions (low  32-bit) - Expected      17322.9, actual  17041 (0.98x) (-281)
Testing collisions (low  27-41 bits) - Worst is 40 bits: 72/67 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit 45 - 0.024%


Combination 0x80000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8220 (1.00x) (34)
Testing collisions (high 26-40 bits) - Worst is 37 bits: 283/255 (1.11x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8231 (1.01x) (45)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 20-bit window at bit 19 - 0.037%


Combination 0x00000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8156 (1.00x) (-30)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 73/63 (1.14x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8198 (1.00x) (12)
Testing collisions (low  26-40 bits) - Worst is 39 bits: 79/63 (1.23x)
Testing distribution - Worst bias is the 20-bit window at bit 10 - 0.034%


Combination 0x8000000000000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8138 (0.99x) (-48)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8382 (1.02x) (196)
Testing collisions (low  26-40 bits) - Worst is 33 bits: 4219/4094 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 18 - 0.039%


Combination 0x0000000000000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8292 (1.01x) (106)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 149/127 (1.16x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8190 (1.00x) (4)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 20-bit window at bit 13 - 0.040%


Combination 16-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8164 (1.00x) (-22)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 66/63 (1.03x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8132 (0.99x) (-54)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit 26 - 0.032%


Combination 16-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8273 (1.01x) (87)
Testing collisions (high 26-40 bits) - Worst is 32 bits: 8273/8186 (1.01x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8185 (1.00x) (-1)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 20-bit window at bit  4 - 0.037%


Combination 32-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8120 (0.99x) (-66)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8113 (0.99x) (-73)
Testing collisions (low  26-40 bits) - Worst is 39 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 20-bit window at bit  1 - 0.048%


Combination 32-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8213 (1.00x) (27)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 67/63 (1.05x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8331 (1.02x) (145)
Testing collisions (low  26-40 bits) - Worst is 39 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 20-bit window at bit 23 - 0.032%


Combination 64-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8287 (1.01x) (101)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8226 (1.00x) (40)
Testing collisions (low  26-40 bits) - Worst is 31 bits: 16475/16362 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit  1 - 0.032%


Combination 64-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8094 (0.99x) (-92)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 142/127 (1.11x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8266 (1.01x) (80)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 20-bit window at bit  5 - 0.045%


Combination 128-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8210 (1.00x) (24)
Testing collisions (high 26-40 bits) - Worst is 32 bits: 8210/8186 (1.00x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8341 (1.02x) (155)
Testing collisions (low  26-40 bits) - Worst is 33 bits: 4227/4094 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 46 - 0.033%


Combination 128-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8118 (0.99x) (-68)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8062 (0.98x) (-124)
Testing collisions (low  26-40 bits) - Worst is 26 bits: 502879/503108 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 51 - 0.040%


[[[ Keyset 'Window' Tests ]]]

Keyset 'Window' -  32-bit key,  25-bit window - 32 tests, 33554432 keys per test
Window at   0 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   1 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   2 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   3 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   4 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   5 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   6 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   7 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   8 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   9 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  10 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  11 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  12 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  13 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  14 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  15 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  16 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  17 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  18 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  19 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  20 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  21 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  22 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  23 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  24 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  25 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  26 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  27 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  28 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  29 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  30 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  31 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  32 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)

[[[ Keyset 'Cyclic' Tests ]]]

Keyset 'Cyclic' - 8 cycles of 8 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    122 (1.05x) (6)
Testing collisions (high 23-34 bits) - Worst is 30 bits: 510/465 (1.10x)
Testing collisions (low  32-bit) - Expected        116.4, actual    122 (1.05x) (6)
Testing collisions (low  23-34 bits) - Worst is 33 bits: 67/58 (1.15x)
Testing distribution - Worst bias is the 17-bit window at bit 49 - 0.131%

Keyset 'Cyclic' - 8 cycles of 9 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    131 (1.13x) (15)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing collisions (low  32-bit) - Expected        116.4, actual    124 (1.07x) (8)
Testing collisions (low  23-34 bits) - Worst is 33 bits: 69/58 (1.19x)
Testing distribution - Worst bias is the 17-bit window at bit 26 - 0.102%

Keyset 'Cyclic' - 8 cycles of 10 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    127 (1.09x) (11)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing collisions (low  32-bit) - Expected        116.4, actual    126 (1.08x) (10)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 32/29 (1.10x)
Testing distribution - Worst bias is the 17-bit window at bit 61 - 0.123%

Keyset 'Cyclic' - 8 cycles of 11 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    124 (1.07x) (8)
Testing collisions (high 23-34 bits) - Worst is 31 bits: 254/232 (1.09x)
Testing collisions (low  32-bit) - Expected        116.4, actual    104 (0.89x)
Testing collisions (low  23-34 bits) - Worst is 30 bits: 471/465 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit  2 - 0.113%

Keyset 'Cyclic' - 8 cycles of 12 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    138 (1.19x) (22)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 39/29 (1.34x)
Testing collisions (low  32-bit) - Expected        116.4, actual    111 (0.95x)
Testing collisions (low  23-34 bits) - Worst is 33 bits: 61/58 (1.05x)
Testing distribution - Worst bias is the 17-bit window at bit 63 - 0.084%

Keyset 'Cyclic' - 8 cycles of 16 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (high 23-34 bits) - Worst is 27 bits: 3850/3716 (1.04x)
Testing collisions (low  32-bit) - Expected        116.4, actual    132 (1.13x) (16)
Testing collisions (low  23-34 bits) - Worst is 33 bits: 74/58 (1.27x)
Testing distribution - Worst bias is the 17-bit window at bit 10 - 0.137%


[[[ Keyset 'TwoBytes' Tests ]]]

Keyset 'TwoBytes' - up-to-4-byte keys, 652545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         49.6, actual     53 (1.07x) (4)
Testing collisions (high 23-33 bits) - Worst is 33 bits: 35/24 (1.41x)
Testing collisions (low  32-bit) - Expected         49.6, actual     49 (0.99x)
Testing collisions (low  23-33 bits) - Worst is 30 bits: 220/198 (1.11x)
Testing distribution - Worst bias is the 16-bit window at bit 17 - 0.148%

Keyset 'TwoBytes' - up-to-8-byte keys, 5471025 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       3483.1, actual   3484 (1.00x) (1)
Testing collisions (high 26-39 bits) - Worst is 39 bits: 32/27 (1.18x)
Testing collisions (low  32-bit) - Expected       3483.1, actual   3558 (1.02x) (75)
Testing collisions (low  26-39 bits) - Worst is 37 bits: 114/108 (1.05x)
Testing distribution - Worst bias is the 19-bit window at bit 62 - 0.043%

Keyset 'TwoBytes' - up-to-12-byte keys, 18616785 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      40289.5, actual  40460 (1.00x) (171)
Testing collisions (high 27-42 bits) - Worst is 37 bits: 1274/1260 (1.01x)
Testing collisions (low  32-bit) - Expected      40289.5, actual  40082 (0.99x) (-207)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 83/78 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 34 - 0.019%

Keyset 'TwoBytes' - up-to-16-byte keys, 44251425 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     227182.3, actual 227828 (1.00x) (646)
Testing collisions (high 29-45 bits) - Worst is 44 bits: 64/55 (1.15x)
Testing collisions (low  32-bit) - Expected     227182.3, actual 227206 (1.00x) (24)
Testing collisions (low  29-45 bits) - Worst is 44 bits: 61/55 (1.10x)
Testing distribution - Worst bias is the 20-bit window at bit 23 - 0.007%

Keyset 'TwoBytes' - up-to-20-byte keys, 86536545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     865959.1, actual 865690 (1.00x) (-269)
Testing collisions (high 30-47 bits) - Worst is 42 bits: 881/851 (1.03x)
Testing collisions (low  32-bit) - Expected     865959.1, actual 865274 (1.00x) (-685)
Testing collisions (low  30-47 bits) - Worst is 47 bits: 29/26 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 63 - 0.003%


[[[ Keyset 'Text' Tests ]]]

Keyset 'Text' - keys of form "FooXXXXBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25132 (0.99x) (-257)
Testing collisions (high 27-42 bits) - Worst is 37 bits: 819/794 (1.03x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25223 (0.99x) (-166)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 54/49 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 52 - 0.026%

Keyset 'Text' - keys of form "FooBarXXXX" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25359 (1.00x) (-30)
Testing collisions (high 27-42 bits) - Worst is 35 bits: 3275/3176 (1.03x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25687 (1.01x) (298)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 56/49 (1.13x)
Testing distribution - Worst bias is the 20-bit window at bit 21 - 0.018%

Keyset 'Text' - keys of form "XXXXFooBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25589 (1.01x) (200)
Testing collisions (high 27-42 bits) - Worst is 35 bits: 3215/3176 (1.01x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25213 (0.99x) (-176)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 67/49 (1.35x)
Testing distribution - Worst bias is the 20-bit window at bit 42 - 0.021%

Keyset 'Words' - 4000000 random keys of len 6-16 from alnum charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1930 (1.04x) (68)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 62/58 (1.07x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1867 (1.00x) (5)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 66/58 (1.13x)
Testing distribution - Worst bias is the 19-bit window at bit 56 - 0.053%

Keyset 'Words' - 4000000 random keys of len 6-16 from password charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1837 (0.99x) (-25)
Testing collisions (high 25-38 bits) - Worst is 33 bits: 960/931 (1.03x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1837 (0.99x) (-25)
Testing collisions (low  25-38 bits) - Worst is 34 bits: 482/465 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.039%

Keyset 'Words' - 235976 dict words
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          6.5, actual      4 (0.62x)
Testing collisions (high 21-30 bits) - Worst is 26 bits: 425/414 (1.03x)
Testing collisions (low  32-bit) - Expected          6.5, actual      6 (0.93x)
Testing collisions (low  21-30 bits) - Worst is 30 bits: 29/25 (1.12x)
Testing distribution - Worst bias is the 15-bit window at bit 29 - 0.267%


[[[ Keyset 'Zeroes' Tests ]]]

Keyset 'Zeroes' - 204800 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          4.9, actual      4 (0.82x)
Testing collisions (high 21-29 bits) - Worst is 28 bits: 86/78 (1.10x)
Testing collisions (low  32-bit) - Expected          4.9, actual      2 (0.41x)
Testing collisions (low  21-29 bits) - Worst is 29 bits: 50/39 (1.28x)
Testing distribution - Worst bias is the 15-bit window at bit 15 - 0.327%


[[[ Keyset 'Seed' Tests ]]]

Keyset 'Seed' - 5000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2909.3, actual   2885 (0.99x) (-24)
Testing collisions (high 26-39 bits) - Worst is 39 bits: 30/22 (1.32x)
Testing collisions (low  32-bit) - Expected       2909.3, actual   2951 (1.01x) (42)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 33/22 (1.45x)
Testing distribution - Worst bias is the 19-bit window at bit 52 - 0.062%


[[[ Keyset 'PerlinNoise' Tests ]]]

Testing 16777216 coordinates (L2) : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      32725.4, actual  32506 (0.99x) (-219)
Testing collisions (high 27-42 bits) - Worst is 35 bits: 4101/4095 (1.00x)
Testing collisions (low  32-bit) - Expected      32725.4, actual  32645 (1.00x) (-80)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 66/63 (1.03x)

Testing AV variant, 128 count with 4 spacing, 4-12:
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1116.2, actual   1110 (0.99x) (-6)
Testing collisions (high 25-37 bits) - Worst is 35 bits: 150/139 (1.07x)
Testing collisions (low  32-bit) - Expected       1116.2, actual   1151 (1.03x) (35)
Testing collisions (low  25-37 bits) - Worst is 34 bits: 292/279 (1.05x)


[[[ Diff 'Differential' Tests ]]]

Testing 8303632 up-to-5-bit differentials in 64-bit keys -> 64 bit hashes.
1000 reps, 8303632000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 11017632 up-to-4-bit differentials in 128-bit keys -> 64 bit hashes.
1000 reps, 11017632000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 2796416 up-to-3-bit differentials in 256-bit keys -> 64 bit hashes.
1000 reps, 2796416000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored


[[[ DiffDist 'Differential Distribution' Tests ]]]

Testing bit 0
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 46/31 (1.44x)
Testing collisions (low  32-bit) - Expected        511.9, actual    476 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 65/63 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 14 - 0.087%

Testing bit 1
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 137/127 (1.07x)
Testing collisions (low  32-bit) - Expected        511.9, actual    523 (1.02x) (12)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.120%

Testing bit 2
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    541 (1.06x) (30)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 146/127 (1.14x)
Testing collisions (low  32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 11 - 0.074%

Testing bit 3
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1055/1023 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 12 - 0.078%

Testing bit 4
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 66/63 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125897/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 44 - 0.064%

Testing bit 5
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 270/255 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 17-bit window at bit 23 - 0.063%

Testing bit 6
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    568 (1.11x) (57)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.054%

Testing bit 7
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.060%

Testing bit 8
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 139/127 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 37 - 0.065%

Testing bit 9
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    474 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.085%

Testing bit 10
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    475 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.062%

Testing bit 11
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4146/4090 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 36 - 0.089%

Testing bit 12
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8265/8170 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 45/31 (1.41x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.074%

Testing bit 13
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2116/2046 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    551 (1.08x) (40)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 51 - 0.072%

Testing bit 14
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125825/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 54 - 0.038%

Testing bit 15
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    542 (1.06x) (31)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    536 (1.05x) (25)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 286/255 (1.12x)
Testing distribution - Worst bias is the 18-bit window at bit 45 - 0.065%

Testing bit 16
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 133/127 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.102%

Testing bit 17
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    531 (1.04x) (20)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 272/255 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.111%

Testing bit 18
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 87/63 (1.36x)
Testing collisions (low  32-bit) - Expected        511.9, actual    540 (1.05x) (29)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 43 - 0.066%

Testing bit 19
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    566 (1.11x) (55)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 79/63 (1.23x)
Testing distribution - Worst bias is the 18-bit window at bit 55 - 0.065%

Testing bit 20
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32681/32429 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 46 - 0.083%

Testing bit 21
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2104/2046 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit  6 - 0.078%

Testing bit 22
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    523 (1.02x) (12)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.087%

Testing bit 23
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8285/8170 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 272/255 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 16 - 0.067%

Testing bit 24
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    476 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4184/4090 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1066/1023 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.075%

Testing bit 25
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1039/1023 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.062%

Testing bit 26
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 12 - 0.081%

Testing bit 27
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.081%

Testing bit 28
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    489 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8203/8170 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing distribution - Worst bias is the 18-bit window at bit 18 - 0.059%

Testing bit 29
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    482 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 524/511 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 25 - 0.043%

Testing bit 30
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 81/63 (1.27x)
Testing distribution - Worst bias is the 18-bit window at bit  4 - 0.048%

Testing bit 31
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    447 (0.87x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125858/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 36 - 0.073%

Testing bit 32
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    531 (1.04x) (20)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 133/127 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 40 - 0.083%

Testing bit 33
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2063/2046 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 17 - 0.075%

Testing bit 34
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8233/8170 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 22 - 0.076%

Testing bit 35
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 539/511 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    479 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32543/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 53 - 0.067%

Testing bit 36
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    490 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8146/8170 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.104%

Testing bit 37
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    531 (1.04x) (20)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 48/31 (1.50x)
Testing collisions (low  32-bit) - Expected        511.9, actual    468 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32586/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 10 - 0.065%

Testing bit 38
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2064/2046 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2081/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 56 - 0.055%

Testing bit 39
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 27 bits: 16280/16298 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 46 - 0.064%

Testing bit 40
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    474 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125443/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2134/2046 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit  6 - 0.056%

Testing bit 41
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1035/1023 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125755/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.106%

Testing bit 42
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    473 (0.92x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.074%

Testing bit 43
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4124/4090 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64635/64191 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 14 - 0.067%

Testing bit 44
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 65/63 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1054/1023 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 40 - 0.120%

Testing bit 45
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    558 (1.09x) (47)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.082%

Testing bit 46
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    487 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 138/127 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 32 - 0.109%

Testing bit 47
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    487 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 159/127 (1.24x)
Testing distribution - Worst bias is the 18-bit window at bit  6 - 0.058%

Testing bit 48
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1075/1023 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.087%

Testing bit 49
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 139/127 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    486 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.056%

Testing bit 50
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 539/511 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    480 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.091%

Testing bit 51
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 539/511 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    444 (0.87x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125842/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit  0 - 0.072%

Testing bit 52
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8357/8170 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 28 - 0.098%

Testing bit 53
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 67/63 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit  5 - 0.093%

Testing bit 54
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing distribution - Worst bias is the 18-bit window at bit 43 - 0.090%

Testing bit 55
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1035/1023 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.084%

Testing bit 56
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    551 (1.08x) (40)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 551/511 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 133/127 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 18 - 0.113%

Testing bit 57
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    523 (1.02x) (12)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 523/511 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 265/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit  0 - 0.090%

Testing bit 58
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 150/127 (1.17x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 17 - 0.090%

Testing bit 59
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32686/32429 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    547 (1.07x) (36)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 34 - 0.080%

Testing bit 60
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2080/2046 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2136/2046 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 55 - 0.070%

Testing bit 61
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 278/255 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1059/1023 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 21 - 0.061%

Testing bit 62
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing distribution - Worst bias is the 18-bit window at bit 30 - 0.068%

Testing bit 63
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 265/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 16 - 0.085%


[[[ MomentChi2 Tests ]]]

Analyze hashes produced from a serie of linearly increasing numbers of 32-bit, using a step of 2 ... 
Target values to approximate : 38918200.000000 - 273633.333333 
Popcount 1 stats : 38918652.494215 - 273634.910473
Popcount 0 stats : 38918979.415324 - 273650.197666
MomentChi2 for bits 1 :  0.374133 
MomentChi2 for bits 0 :   1.11001 

Derivative stats (transition from 2 consecutive values) : 
Popcount 1 stats : 38919231.255505 - 273630.414097
Popcount 0 stats : 38918467.360033 - 273641.436163
MomentChi2 for deriv b1 :   1.94328 
MomentChi2 for deriv b0 :  0.130613 

  Great 


[[[ Prng Tests ]]]

Generating 33554432 random numbers : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     130731.3, actual 130975 (1.00x) (244)
Testing collisions (high 28-44 bits) - Worst is 44 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected     130731.3, actual 131129 (1.00x) (398)
Testing collisions (low  28-44 bits) - Worst is 44 bits: 35/31 (1.09x)

[[[ BadSeeds Tests ]]]

0x0 PASS


Input vcode 0x00000001, Output vcode 0x00000001, Result vcode 0x00000001
Verification value is 0x00000001 - Testing took 879.932608 seconds
-------------------------------------------------------------------------------
```

**Risky512 (keyed), high 256 bits**:

```txt
# SMHasher fio-risky512_hi
# date: 2026-11-01T07:59:28.767Z
# library: 615c171, fio-stl.h sha256 fd8359f5f75947b0
# tools: SMHasher e154a82a5a9a (+facil.io patch), PractRand 0.95, xoshiro hwd.c 97ec22f5b2e7
# machine: Apple M4 Max, Apple clang version 21.0.0 (clang-2100.3.34.2)
# note: on Apple Silicon SMHasher counts timer ticks x64, not CPU cycles: 'MiB/sec' is correct,
#       'cycles/hash' / (MHz/1000) = ns/hash, and HashMapTest PASS/FAIL (a 1500-cycle threshold) is meaningless.
# 
-------------------------------------------------------------------------------
--- Testing fio-risky512_hi "facil.io Risky512 keyed, high 256 bits" GOOD

[[[ Sanity Tests ]]]

Verification value 0x52F03B1D ....... PASS
Running sanity check 1       .......... PASS
Running AppendedZeroesTest .......... PASS

[[[ Speed Tests ]]]

WARNING: timer resolution is 9223372036854775807 (0x7fffffffffffffff) ticks (0xd7f258715a48c0 - 0xd7f258715a48c0). Broken VDSO?
Bulk speed test - 262144-byte keys
Alignment  7 -  0.267 bytes/cycle - 16298.83 MiB/sec @ 64000 MHz
Alignment  6 -  0.265 bytes/cycle - 16164.55 MiB/sec @ 64000 MHz
Alignment  5 -  0.266 bytes/cycle - 16237.94 MiB/sec @ 64000 MHz
Alignment  4 -  0.267 bytes/cycle - 16271.95 MiB/sec @ 64000 MHz
Alignment  3 -  0.268 bytes/cycle - 16358.15 MiB/sec @ 64000 MHz
Alignment  2 -  0.265 bytes/cycle - 16186.01 MiB/sec @ 64000 MHz
Alignment  1 -  0.265 bytes/cycle - 16196.52 MiB/sec @ 64000 MHz
Alignment  0 -  0.271 bytes/cycle - 16568.53 MiB/sec @ 64000 MHz
Average      -  0.267 bytes/cycle - 16285.31 MiB/sec @ 64000 MHz

Small key speed test -    1-byte keys -  1918.28 cycles/hash
Small key speed test -    2-byte keys -  1912.55 cycles/hash
Small key speed test -    3-byte keys -  1922.45 cycles/hash
Small key speed test -    4-byte keys -  1911.38 cycles/hash
Small key speed test -    5-byte keys -  1917.30 cycles/hash
Small key speed test -    6-byte keys -  1914.80 cycles/hash
Small key speed test -    7-byte keys -  1934.17 cycles/hash
Small key speed test -    8-byte keys -  1920.86 cycles/hash
Small key speed test -    9-byte keys -  1921.13 cycles/hash
Small key speed test -   10-byte keys -  1944.41 cycles/hash
Small key speed test -   11-byte keys -  1968.82 cycles/hash
Small key speed test -   12-byte keys -  1957.74 cycles/hash
Small key speed test -   13-byte keys -  1962.79 cycles/hash
Small key speed test -   14-byte keys -  1954.76 cycles/hash
Small key speed test -   15-byte keys -  1955.43 cycles/hash
Small key speed test -   16-byte keys -  1961.14 cycles/hash
Small key speed test -   17-byte keys -  2025.20 cycles/hash
Small key speed test -   18-byte keys -  2027.82 cycles/hash
Small key speed test -   19-byte keys -  2017.89 cycles/hash
Small key speed test -   20-byte keys -  1992.76 cycles/hash
Small key speed test -   21-byte keys -  1989.51 cycles/hash
Small key speed test -   22-byte keys -  1989.71 cycles/hash
Small key speed test -   23-byte keys -  1989.59 cycles/hash
Small key speed test -   24-byte keys -  1992.12 cycles/hash
Small key speed test -   25-byte keys -  1990.05 cycles/hash
Small key speed test -   26-byte keys -  1989.62 cycles/hash
Small key speed test -   27-byte keys -  1992.35 cycles/hash
Small key speed test -   28-byte keys -  1993.16 cycles/hash
Small key speed test -   29-byte keys -  2010.40 cycles/hash
Small key speed test -   30-byte keys -  2024.32 cycles/hash
Small key speed test -   31-byte keys -  1989.98 cycles/hash
Small key speed test -   32-byte keys -  2001.88 cycles/hash
Average                                  1968.574 cycles/hash
Average, weighted by key length freq.    1975.711 cycles/hash (using 93.0% of top-7m Tranco DNS names dataset)
Average, weighted by key length freq.    1946.570 cycles/hash (using 27.1% of startup-1M UMASH trace dataset)

[[[ 'Hashmap' Speed Tests ]]]

std::unordered_map
Init std HashMapTest:     4224.764 cycles/op (235976 inserts, 1% deletions)
Running std HashMapTest:  3792.564 cycles/op (103.0 stdv)

greg7mdp/parallel-hashmap
Init fast HashMapTest:    5498.775 cycles/op (235976 inserts, 1% deletions)
Running fast HashMapTest: 3411.535 cycles/op (120.9 stdv)  ....... FAIL

[[[ Avalanche Tests ]]]

Testing   24-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.778667%
Testing   32-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.693333%
Testing   40-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.800000%
Testing   48-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.812000%
Testing   56-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.687333%
Testing   64-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.797333%
Testing   72-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.759333%
Testing   80-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.807333%
Testing   96-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.795333%
Testing  112-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.814667%
Testing  128-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.828000%
Testing  160-bit keys -> 256-bit hashes, 300000 reps.......... worst bias is 0.767333%

[[[ Keyset 'Sparse' Tests ]]]

Keyset 'Sparse' - 16-bit keys with up to 9 bits set - 50643 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (high 19-25 bits) - Worst is 20 bits: 1222/1203 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (low  19-25 bits) - Worst is 25 bits: 45/38 (1.18x)
Testing distribution - Worst bias is the 13-bit window at bit 31 - 0.885%

Keyset 'Sparse' - 24-bit keys with up to 8 bits set - 1271626 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        188.2, actual    174 (0.92x)
Testing collisions (high 24-35 bits) - Worst is 28 bits: 3080/3007 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        188.2, actual    188 (1.00x)
Testing collisions (low  24-35 bits) - Worst is 33 bits: 100/94 (1.06x)
Testing distribution - Worst bias is the 17-bit window at bit 90 - 0.113%

Keyset 'Sparse' - 32-bit keys with up to 7 bits set - 4514873 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2372.2, actual   2365 (1.00x) (-7)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 166/148 (1.12x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2372.2, actual   2393 (1.01x) (21)
Testing collisions (low  25-38 bits) - Worst is 36 bits: 172/148 (1.16x)
Testing distribution - Worst bias is the 19-bit window at bit 80 - 0.060%

Keyset 'Sparse' - 40-bit keys with up to 6 bits set - 4598479 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2460.8, actual   2524 (1.03x) (64)
Testing collisions (high 25-38 bits) - Worst is 32 bits: 2524/2460 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2460.8, actual   2467 (1.00x) (7)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 50/38 (1.30x)
Testing distribution - Worst bias is the 19-bit window at bit 42 - 0.073%

Keyset 'Sparse' - 48-bit keys with up to 6 bits set - 14196869 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      23437.8, actual  23819 (1.02x) (382)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 31/22 (1.35x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      23437.8, actual  23374 (1.00x) (-63)
Testing collisions (low  27-42 bits) - Worst is 40 bits: 100/91 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 152 - 0.031%

Keyset 'Sparse' - 56-bit keys with up to 5 bits set - 4216423 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2069.0, actual   2075 (1.00x) (7)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 36/32 (1.11x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2069.0, actual   2044 (0.99x) (-24)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 75/64 (1.16x)
Testing distribution - Worst bias is the 19-bit window at bit 17 - 0.069%

Keyset 'Sparse' - 64-bit keys with up to 5 bits set - 8303633 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8021.7, actual   7981 (0.99x) (-40)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 32/31 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       8021.7, actual   7971 (0.99x) (-50)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 32/31 (1.02x)
Testing distribution - Worst bias is the 20-bit window at bit 126 - 0.052%

Keyset 'Sparse' - 72-bit keys with up to 5 bits set - 15082603 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      26451.8, actual  26440 (1.00x) (-11)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 55/51 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      26451.8, actual  26549 (1.00x) (98)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 52/51 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit 48 - 0.024%

Keyset 'Sparse' - 96-bit keys with up to 4 bits set - 3469497 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1401.0, actual   1459 (1.04x) (59)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 24/21 (1.10x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1401.0, actual   1368 (0.98x)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 53/43 (1.21x)
Testing distribution - Worst bias is the 19-bit window at bit 149 - 0.082%

Keyset 'Sparse' - 160-bit keys with up to 4 bits set - 26977161 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      84546.1, actual  84624 (1.00x) (78)
Testing collisions (high 28-44 bits) - Worst is 44 bits: 25/20 (1.21x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      84546.1, actual  84373 (1.00x) (-173)
Testing collisions (low  28-44 bits) - Worst is 44 bits: 29/20 (1.40x)
Testing distribution - Worst bias is the 20-bit window at bit 171 - 0.015%

Keyset 'Sparse' - 256-bit keys with up to 3 bits set - 2796417 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        910.2, actual    941 (1.03x) (31)
Testing collisions (high 25-37 bits) - Worst is 37 bits: 35/28 (1.23x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        910.2, actual    933 (1.03x) (23)
Testing collisions (low  25-37 bits) - Worst is 37 bits: 33/28 (1.16x)
Testing distribution - Worst bias is the 19-bit window at bit 187 - 0.128%


[[[ Keyset 'Permutation' Tests ]]]

Combination Lowbits Tests:
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    687 (1.03x) (19)
Testing collisions (high 24-37 bits) - Worst is 32 bits: 687/668 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        668.6, actual    658 (0.98x) (-10)
Testing collisions (low  24-37 bits) - Worst is 31 bits: 1361/1336 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 221 - 0.080%


Combination Highbits Tests
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    634 (0.95x)
Testing collisions (high 24-37 bits) - Worst is 37 bits: 22/20 (1.05x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        668.6, actual    638 (0.95x)
Testing collisions (low  24-37 bits) - Worst is 37 bits: 29/20 (1.39x)
Testing distribution - Worst bias is the 18-bit window at bit 74 - 0.084%


Combination Hi-Lo Tests:
Keyset 'Combination' - up to 6 blocks from a set of 15 - 12204240 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      17322.9, actual  17409 (1.00x) (87)
Testing collisions (high 27-41 bits) - Worst is 37 bits: 574/541 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      17322.9, actual  17330 (1.00x) (8)
Testing collisions (low  27-41 bits) - Worst is 41 bits: 37/33 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 148 - 0.025%


Combination 0x80000000 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual     10 (1.25x) (3)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 43/31 (1.34x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      2 (0.25x)
Testing collisions (low  21-30 bits) - Worst is 28 bits: 137/127 (1.07x)
Testing distribution - Worst bias is the 15-bit window at bit 234 - 0.375%


Combination 0x00000001 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 35/31 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      3 (0.38x)
Testing collisions (low  21-30 bits) - Worst is 22 bits: 8045/8023 (1.00x)
Testing distribution - Worst bias is the 15-bit window at bit 138 - 0.345%


Combination 0x8000000000000000 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 27 bits: 275/255 (1.07x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      6 (0.75x)
Testing collisions (low  21-30 bits) - Worst is 27 bits: 280/255 (1.09x)
Testing distribution - Worst bias is the 15-bit window at bit 178 - 0.276%


Combination 0x0000000000000001 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual     10 (1.25x) (3)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 34/31 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      9 (1.13x) (2)
Testing collisions (low  21-30 bits) - Worst is 29 bits: 82/63 (1.28x)
Testing distribution - Worst bias is the 15-bit window at bit 234 - 0.285%


Combination 16-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 33/31 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      6 (0.75x)
Testing collisions (low  21-30 bits) - Worst is 21 bits: 15770/15721 (1.00x)
Testing distribution - Worst bias is the 15-bit window at bit 66 - 0.232%


Combination 16-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      9 (1.13x) (2)
Testing collisions (high 21-30 bits) - Worst is 29 bits: 75/63 (1.17x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      5 (0.63x)
Testing collisions (low  21-30 bits) - Worst is 27 bits: 272/255 (1.06x)
Testing distribution - Worst bias is the 15-bit window at bit 97 - 0.212%


Combination 32-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 25 bits: 1028/1021 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      6 (0.75x)
Testing collisions (low  21-30 bits) - Worst is 24 bits: 2080/2037 (1.02x)
Testing distribution - Worst bias is the 15-bit window at bit 42 - 0.230%


Combination 32-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual     14 (1.75x) (7)
Testing collisions (high 21-30 bits) - Worst is 29 bits: 66/63 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (low  21-30 bits) - Worst is 26 bits: 515/511 (1.01x)
Testing distribution - Worst bias is the 15-bit window at bit 113 - 0.342%


Combination 64-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (low  21-30 bits) - Worst is 26 bits: 542/511 (1.06x)
Testing distribution - Worst bias is the 15-bit window at bit 90 - 0.297%


Combination 64-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      9 (1.13x) (2)
Testing collisions (low  21-30 bits) - Worst is 29 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 15-bit window at bit 49 - 0.265%


Combination 128-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual     12 (1.50x) (5)
Testing collisions (high 21-30 bits) - Worst is 26 bits: 521/511 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual     10 (1.25x) (3)
Testing collisions (low  21-30 bits) - Worst is 29 bits: 91/63 (1.42x)
Testing distribution - Worst bias is the 15-bit window at bit 52 - 0.303%


Combination 128-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 27 bits: 274/255 (1.07x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      3 (0.38x)
Testing collisions (low  21-30 bits) - Worst is 27 bits: 276/255 (1.08x)
Testing distribution - Worst bias is the 15-bit window at bit 131 - 0.267%


[[[ Keyset 'Window' Tests ]]]

Keyset 'Window' -  32-bit key,  25-bit window - 32 tests, 33554432 keys per test
Window at   0 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   1 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   2 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   3 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   4 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   5 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   6 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   7 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   8 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at   9 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  10 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  11 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  12 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  13 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  14 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  15 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  16 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  17 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  18 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  19 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  20 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  21 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  22 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  23 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  24 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  25 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  26 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  27 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  28 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  29 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  30 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  31 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Window at  32 - Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)

[[[ Keyset 'Cyclic' Tests ]]]

Keyset 'Cyclic' - 8 cycles of 32 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    106 (0.91x)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 35/29 (1.20x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    119 (1.02x) (3)
Testing collisions (low  23-34 bits) - Worst is 33 bits: 62/58 (1.07x)
Testing distribution - Worst bias is the 17-bit window at bit 163 - 0.120%

Keyset 'Cyclic' - 8 cycles of 33 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    106 (0.91x)
Testing collisions (high 23-34 bits) - Worst is 25 bits: 14682/14754 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    132 (1.13x) (16)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 39/29 (1.34x)
Testing distribution - Worst bias is the 17-bit window at bit 29 - 0.161%

Keyset 'Cyclic' - 8 cycles of 34 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    113 (0.97x)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 30/29 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    118 (1.01x) (2)
Testing collisions (low  23-34 bits) - Worst is 33 bits: 65/58 (1.12x)
Testing distribution - Worst bias is the 17-bit window at bit 31 - 0.140%

Keyset 'Cyclic' - 8 cycles of 35 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    117 (1.01x) (1)
Testing collisions (high 23-34 bits) - Worst is 26 bits: 7468/7413 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    108 (0.93x)
Testing collisions (low  23-34 bits) - Worst is 31 bits: 238/232 (1.02x)
Testing distribution - Worst bias is the 17-bit window at bit 147 - 0.148%

Keyset 'Cyclic' - 8 cycles of 36 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    142 (1.22x) (26)
Testing collisions (high 23-34 bits) - Worst is 32 bits: 142/116 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    117 (1.01x) (1)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 30/29 (1.03x)
Testing distribution - Worst bias is the 17-bit window at bit 49 - 0.141%

Keyset 'Cyclic' - 8 cycles of 40 bytes - 1000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    119 (1.02x) (3)
Testing collisions (high 23-34 bits) - Worst is 30 bits: 481/465 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    109 (0.94x)
Testing collisions (low  23-34 bits) - Worst is 31 bits: 254/232 (1.09x)
Testing distribution - Worst bias is the 17-bit window at bit 202 - 0.149%


[[[ Keyset 'TwoBytes' Tests ]]]

Keyset 'TwoBytes' - up-to-4-byte keys, 652545 total keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         49.6, actual     39 (0.79x)
Testing collisions (high 23-33 bits) - Worst is 28 bits: 811/792 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected         49.6, actual     52 (1.05x) (3)
Testing collisions (low  23-33 bits) - Worst is 32 bits: 52/49 (1.05x)
Testing distribution - Worst bias is the 16-bit window at bit 195 - 0.170%

Keyset 'TwoBytes' - up-to-8-byte keys, 5471025 total keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       3483.1, actual   3509 (1.01x) (26)
Testing collisions (high 26-39 bits) - Worst is 36 bits: 232/217 (1.07x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       3483.1, actual   3382 (0.97x)
Testing collisions (low  26-39 bits) - Worst is 38 bits: 56/54 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 221 - 0.078%

Keyset 'TwoBytes' - up-to-12-byte keys, 18616785 total keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      40289.5, actual  40583 (1.01x) (294)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 96/78 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      40289.5, actual  40403 (1.00x) (114)
Testing collisions (low  27-42 bits) - Worst is 31 bits: 80777/80462 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 140 - 0.023%


[[[ Keyset 'Text' Tests ]]]

Keyset 'Text' - keys of form "FooXXXXBar" - 14776336 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25343 (1.00x) (-46)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 58/49 (1.17x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25298 (1.00x) (-91)
Testing collisions (low  27-42 bits) - Worst is 27 bits: 784404/784335 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 230 - 0.036%

Keyset 'Text' - keys of form "FooBarXXXX" - 14776336 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25679 (1.01x) (290)
Testing collisions (high 27-42 bits) - Worst is 32 bits: 25679/25389 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25580 (1.01x) (191)
Testing collisions (low  27-42 bits) - Worst is 40 bits: 113/99 (1.14x)
Testing distribution - Worst bias is the 20-bit window at bit  3 - 0.026%

Keyset 'Text' - keys of form "XXXXFooBar" - 14776336 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25561 (1.01x) (172)
Testing collisions (high 27-42 bits) - Worst is 34 bits: 6556/6352 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25483 (1.00x) (94)
Testing collisions (low  27-42 bits) - Worst is 37 bits: 848/794 (1.07x)
Testing distribution - Worst bias is the 20-bit window at bit 255 - 0.030%

Keyset 'Words' - 4000000 random keys of len 6-16 from alnum charset
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1813 (0.97x)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 36/29 (1.24x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1875 (1.01x) (13)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 62/58 (1.07x)
Testing distribution - Worst bias is the 19-bit window at bit 66 - 0.068%

Keyset 'Words' - 4000000 random keys of len 6-16 from password charset
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1905 (1.02x) (43)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 128/116 (1.10x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1832 (0.98x) (-30)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 32/29 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit 33 - 0.067%

Keyset 'Words' - 235976 dict words
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          6.5, actual      8 (1.23x) (2)
Testing collisions (high 21-30 bits) - Worst is 27 bits: 232/207 (1.12x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          6.5, actual      2 (0.31x)
Testing collisions (low  21-30 bits) - Worst is 23 bits: 3332/3288 (1.01x)
Testing distribution - Worst bias is the 15-bit window at bit 130 - 0.432%


[[[ Keyset 'Zeroes' Tests ]]]

Keyset 'Zeroes' - 204800 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          4.9, actual      7 (1.43x) (3)
Testing collisions (high 21-29 bits) - Worst is 21 bits: 9506/9682 (0.98x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          4.9, actual      5 (1.02x) (1)
Testing collisions (low  21-29 bits) - Worst is 28 bits: 83/78 (1.06x)
Testing distribution - Worst bias is the 15-bit window at bit 238 - 0.262%


[[[ Keyset 'Seed' Tests ]]]

Keyset 'Seed' - 5000000 keys
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2909.3, actual   2964 (1.02x) (55)
Testing collisions (high 26-39 bits) - Worst is 37 bits: 95/90 (1.04x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2909.3, actual   2899 (1.00x) (-10)
Testing collisions (low  26-39 bits) - Worst is 35 bits: 368/363 (1.01x)
Testing distribution - Worst bias is the 19-bit window at bit  9 - 0.057%


[[[ Keyset 'PerlinNoise' Tests ]]]

Testing 16777216 coordinates (L2) : 
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      32725.4, actual  32829 (1.00x) (104)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 44/31 (1.38x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      32725.4, actual  32809 (1.00x) (84)
Testing collisions (low  27-42 bits) - Worst is 37 bits: 1040/1023 (1.02x)

Testing AV variant, 128 count with 4 spacing, 4-12:
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1116.2, actual   1096 (0.98x) (-20)
Testing collisions (high 25-37 bits) - Worst is 31 bits: 2277/2231 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1116.2, actual   1150 (1.03x) (34)
Testing collisions (low  25-37 bits) - Worst is 31 bits: 2315/2231 (1.04x)


[[[ Diff 'Differential' Tests ]]]

Testing 8303632 up-to-5-bit differentials in 64-bit keys -> 256 bit hashes.
100 reps, 830363200 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 11017632 up-to-4-bit differentials in 128-bit keys -> 256 bit hashes.
100 reps, 1101763200 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 2796416 up-to-3-bit differentials in 256-bit keys -> 256 bit hashes.
100 reps, 279641600 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored


[[[ DiffDist 'Differential Distribution' Tests ]]]

Testing bit 0
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125639/125777 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    472 (0.92x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 210 - 0.111%

Testing bit 1
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 128 - 0.114%

Testing bit 2
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing distribution - Worst bias is the 18-bit window at bit 250 - 0.104%

Testing bit 3
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 521/511 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 32/31 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 112 - 0.126%

Testing bit 4
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1039/1023 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 88 - 0.094%

Testing bit 5
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 139 - 0.115%

Testing bit 6
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    542 (1.06x) (31)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 273/255 (1.07x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.083%

Testing bit 7
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1043/1023 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    484 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 144 - 0.127%

Testing bit 8
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    480 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 135/127 (1.05x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 234 - 0.099%

Testing bit 9
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 263/255 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 249 - 0.097%

Testing bit 10
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    474 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 65/63 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    560 (1.09x) (49)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 560/511 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 226 - 0.106%

Testing bit 11
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    484 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32580/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 235 - 0.147%

Testing bit 12
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.109%

Testing bit 13
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    485 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2061/2046 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 154 - 0.077%

Testing bit 14
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    469 (0.92x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 263/255 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.090%

Testing bit 15
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    536 (1.05x) (25)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 48/31 (1.50x)
Testing distribution - Worst bias is the 18-bit window at bit 154 - 0.080%

Testing bit 16
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 269/255 (1.05x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    536 (1.05x) (25)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.063%

Testing bit 17
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 68/63 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8188/8170 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 231 - 0.098%

Testing bit 18
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    531 (1.04x) (20)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    466 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8188/8170 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 80 - 0.097%

Testing bit 19
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    523 (1.02x) (12)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8200/8170 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 44 - 0.096%

Testing bit 20
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 537/511 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 109 - 0.090%

Testing bit 21
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    536 (1.05x) (25)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 536/511 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 230 - 0.097%

Testing bit 22
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    493 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 25 bits: 64282/64191 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing distribution - Worst bias is the 18-bit window at bit 207 - 0.106%

Testing bit 23
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 48/31 (1.50x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 135 - 0.111%

Testing bit 24
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    474 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64061/64191 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 164 - 0.104%

Testing bit 25
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    467 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32605/32429 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 218 - 0.102%

Testing bit 26
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    490 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4107/4090 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 154 - 0.086%

Testing bit 27
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1059/1023 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2110/2046 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 181 - 0.108%

Testing bit 28
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    485 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 68 - 0.124%

Testing bit 29
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 273/255 (1.07x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 520/511 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 234 - 0.090%

Testing bit 30
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    565 (1.10x) (54)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 291/255 (1.14x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 180 - 0.092%

Testing bit 31
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 66/63 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    493 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32735/32429 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 17 - 0.097%

Testing bit 32
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 249 - 0.100%

Testing bit 33
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 213 - 0.102%

Testing bit 34
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    561 (1.10x) (50)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 148/127 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.103%

Testing bit 35
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    462 (0.90x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8152/8170 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2061/2046 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 88 - 0.098%

Testing bit 36
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    575 (1.12x) (64)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 159 - 0.084%

Testing bit 37
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1096/1023 (1.07x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8447/8170 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 107 - 0.070%

Testing bit 38
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    486 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 113 - 0.090%

Testing bit 39
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 81/63 (1.27x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 67/63 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 103 - 0.095%

Testing bit 40
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 135/127 (1.05x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    490 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 171 - 0.103%

Testing bit 41
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 134/127 (1.05x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 145 - 0.094%

Testing bit 42
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    482 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2102/2046 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    489 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 129/127 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 70 - 0.098%

Testing bit 43
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125533/125777 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125555/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 176 - 0.122%

Testing bit 44
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8222/8170 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 60 - 0.105%

Testing bit 45
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    531 (1.04x) (20)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 277/255 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 123 - 0.119%

Testing bit 46
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 267/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 157 - 0.087%

Testing bit 47
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 136/127 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 202 - 0.099%

Testing bit 48
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 526/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 67 - 0.093%

Testing bit 49
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 261/255 (1.02x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4159/4090 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 96 - 0.084%

Testing bit 50
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 534/511 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 135 - 0.094%

Testing bit 51
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 529/511 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 98 - 0.087%

Testing bit 52
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    549 (1.07x) (38)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.084%

Testing bit 53
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 277/255 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 174 - 0.111%

Testing bit 54
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 289/255 (1.13x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 171 - 0.100%

Testing bit 55
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1050/1023 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 151 - 0.089%

Testing bit 56
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.100%

Testing bit 57
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32662/32429 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2042/2046 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 17 - 0.084%

Testing bit 58
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 525/511 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 278/255 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 228 - 0.095%

Testing bit 59
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    471 (0.92x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4087/4090 (1.00x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2062/2046 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 147 - 0.088%

Testing bit 60
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2107/2046 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    593 (1.16x) (82)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 166/127 (1.30x)
Testing distribution - Worst bias is the 18-bit window at bit 234 - 0.080%

Testing bit 61
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    480 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16414/16298 (1.01x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4227/4090 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 234 - 0.081%

Testing bit 62
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 28 - 0.080%

Testing bit 63
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1048/1023 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 130 - 0.108%


[[[ MomentChi2 Tests ]]]

Analyze hashes produced from a serie of linearly increasing numbers of 32-bit, using a step of 6 ... 
Target values to approximate : 38918200.000000 - 820900.000000 
Popcount 1 stats : 38918943.168321 - 820927.115351
Popcount 0 stats : 38919304.358184 - 820942.652186
MomentChi2 for bits 1 :  0.336393 
MomentChi2 for bits 0 :  0.742828 

Derivative stats (transition from 2 consecutive values) : 
Popcount 1 stats : 38919628.581192 - 821094.486591
Popcount 0 stats : 38918888.942172 - 820925.468753
MomentChi2 for deriv b1 :   1.24291 
MomentChi2 for deriv b0 :  0.289094 

  Great 


[[[ Prng Tests ]]]

Generating 33554432 random numbers : 
Testing collisions (256-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     130731.3, actual 130556 (1.00x) (-175)
Testing collisions (high 28-44 bits) - Worst is 44 bits: 34/31 (1.06x)
Testing collisions (low  224-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  160-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  128-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected     130731.3, actual 131246 (1.00x) (515)
Testing collisions (low  28-44 bits) - Worst is 43 bits: 78/63 (1.22x)

[[[ BadSeeds Tests ]]]

0x0 PASS


Input vcode 0x00000001, Output vcode 0x00000001, Result vcode 0x00000001
Verification value is 0x00000001 - Testing took 672.953801 seconds
-------------------------------------------------------------------------------
```
