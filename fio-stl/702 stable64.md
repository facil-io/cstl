### Stable Hash 64bit SMHasher Results

The following results were achieved on my personal computer when testing the 64bit variant of the facil.io Stable Hash (`fio_stable_hash`).

```txt
-------------------------------------------------------------------------------
--- Testing Stable "facil.io Stable Hash 64bit" GOOD

[[[ Sanity Tests ]]]

Verification value 0x6993BB92 ....... PASS
Running sanity check 1     .......... PASS
Running AppendedZeroesTest .......... PASS

[[[ Speed Tests ]]]

Bulk speed test - 262144-byte keys
Alignment  7 - 12.548 bytes/cycle - 35899.15 MiB/sec @ 3 ghz
Alignment  6 - 12.560 bytes/cycle - 35934.05 MiB/sec @ 3 ghz
Alignment  5 - 12.554 bytes/cycle - 35917.07 MiB/sec @ 3 ghz
Alignment  4 - 12.560 bytes/cycle - 35935.44 MiB/sec @ 3 ghz
Alignment  3 - 12.559 bytes/cycle - 35931.79 MiB/sec @ 3 ghz
Alignment  2 - 12.557 bytes/cycle - 35926.78 MiB/sec @ 3 ghz
Alignment  1 - 12.561 bytes/cycle - 35936.39 MiB/sec @ 3 ghz
Alignment  0 - 12.678 bytes/cycle - 36273.41 MiB/sec @ 3 ghz
Average      - 12.572 bytes/cycle - 35969.26 MiB/sec @ 3 ghz

Small key speed test -    1-byte keys -    20.57 cycles/hash
Small key speed test -    2-byte keys -    21.00 cycles/hash
Small key speed test -    3-byte keys -    21.00 cycles/hash
Small key speed test -    4-byte keys -    20.99 cycles/hash
Small key speed test -    5-byte keys -    21.00 cycles/hash
Small key speed test -    6-byte keys -    21.00 cycles/hash
Small key speed test -    7-byte keys -    22.00 cycles/hash
Small key speed test -    8-byte keys -    21.00 cycles/hash
Small key speed test -    9-byte keys -    21.00 cycles/hash
Small key speed test -   10-byte keys -    21.00 cycles/hash
Small key speed test -   11-byte keys -    21.00 cycles/hash
Small key speed test -   12-byte keys -    21.00 cycles/hash
Small key speed test -   13-byte keys -    21.00 cycles/hash
Small key speed test -   14-byte keys -    21.00 cycles/hash
Small key speed test -   15-byte keys -    21.00 cycles/hash
Small key speed test -   16-byte keys -    21.00 cycles/hash
Small key speed test -   17-byte keys -    22.88 cycles/hash
Small key speed test -   18-byte keys -    22.92 cycles/hash
Small key speed test -   19-byte keys -    22.94 cycles/hash
Small key speed test -   20-byte keys -    22.93 cycles/hash
Small key speed test -   21-byte keys -    22.94 cycles/hash
Small key speed test -   22-byte keys -    22.92 cycles/hash
Small key speed test -   23-byte keys -    22.89 cycles/hash
Small key speed test -   24-byte keys -    22.89 cycles/hash
Small key speed test -   25-byte keys -    22.93 cycles/hash
Small key speed test -   26-byte keys -    22.90 cycles/hash
Small key speed test -   27-byte keys -    23.03 cycles/hash
Small key speed test -   28-byte keys -    22.81 cycles/hash
Small key speed test -   29-byte keys -    22.89 cycles/hash
Small key speed test -   30-byte keys -    23.13 cycles/hash
Small key speed test -   31-byte keys -    22.99 cycles/hash
Average                                    21.953 cycles/hash

[[[ 'Hashmap' Speed Tests ]]]

std::unordered_map
Init std HashMapTest:     155.970 cycles/op (466569 inserts, 1% deletions)
Running std HashMapTest:  99.425 cycles/op (1.2 stdv)

greg7mdp/parallel-hashmap
Init fast HashMapTest:    181.680 cycles/op (466569 inserts, 1% deletions)
Running fast HashMapTest: 97.842 cycles/op (1.6 stdv)  ....... PASS

[[[ Avalanche Tests ]]]

Testing   24-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.632667%
Testing   32-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.712000%
Testing   40-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.704000%
Testing   48-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.696667%
Testing   56-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.686000%
Testing   64-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.712000%
Testing   72-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.636000%
Testing   80-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.630667%
Testing   96-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.690667%
Testing  112-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.670667%
Testing  128-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.760667%
Testing  160-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.773333%
Testing  512-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.825333%
Testing 1024-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.753333%

[[[ Keyset 'Sparse' Tests ]]]

Keyset 'Sparse' - 16-bit keys with up to 9 bits set - 50643 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (high 19-25 bits) - Worst is 23 bits: 171/152 (1.12x)
Testing collisions (low  32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (low  19-25 bits) - Worst is 21 bits: 602/606 (0.99x)
Testing distribution - Worst bias is the 13-bit window at bit 31 - 0.586%

Keyset 'Sparse' - 24-bit keys with up to 8 bits set - 1271626 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        188.2, actual    197 (1.05x) (9)
Testing collisions (high 24-35 bits) - Worst is 33 bits: 100/94 (1.06x)
Testing collisions (low  32-bit) - Expected        188.2, actual    170 (0.90x)
Testing collisions (low  24-35 bits) - Worst is 26 bits: 12071/11972 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 48 - 0.087%

Keyset 'Sparse' - 32-bit keys with up to 7 bits set - 4514873 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2372.2, actual   2345 (0.99x) (-27)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 47/37 (1.27x)
Testing collisions (low  32-bit) - Expected       2372.2, actual   2374 (1.00x) (2)
Testing collisions (low  25-38 bits) - Worst is 30 bits: 9492/9478 (1.00x)
Testing distribution - Worst bias is the 19-bit window at bit 35 - 0.057%

Keyset 'Sparse' - 40-bit keys with up to 6 bits set - 4598479 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2460.8, actual   2380 (0.97x)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 163/153 (1.06x)
Testing collisions (low  32-bit) - Expected       2460.8, actual   2490 (1.01x) (30)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 45/38 (1.17x)
Testing distribution - Worst bias is the 19-bit window at bit 50 - 0.061%

Keyset 'Sparse' - 48-bit keys with up to 6 bits set - 14196869 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      23437.8, actual  23277 (0.99x) (-160)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 28/22 (1.22x)
Testing collisions (low  32-bit) - Expected      23437.8, actual  23272 (0.99x) (-165)
Testing collisions (low  27-42 bits) - Worst is 30 bits: 93648/93442 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 29 - 0.022%

Keyset 'Sparse' - 56-bit keys with up to 5 bits set - 4216423 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2069.0, actual   2065 (1.00x) (-3)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 37/32 (1.14x)
Testing collisions (low  32-bit) - Expected       2069.0, actual   2042 (0.99x) (-26)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 40/32 (1.24x)
Testing distribution - Worst bias is the 19-bit window at bit 37 - 0.050%

Keyset 'Sparse' - 64-bit keys with up to 5 bits set - 8303633 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8021.7, actual   8020 (1.00x) (-1)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 36/31 (1.15x)
Testing collisions (low  32-bit) - Expected       8021.7, actual   8038 (1.00x) (17)
Testing collisions (low  26-40 bits) - Worst is 31 bits: 16101/16033 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 43 - 0.043%

Keyset 'Sparse' - 72-bit keys with up to 5 bits set - 15082603 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      26451.8, actual  26679 (1.01x) (228)
Testing collisions (high 27-42 bits) - Worst is 39 bits: 218/206 (1.05x)
Testing collisions (low  32-bit) - Expected      26451.8, actual  26434 (1.00x) (-17)
Testing collisions (low  27-42 bits) - Worst is 35 bits: 3324/3309 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 26 - 0.028%

Keyset 'Sparse' - 96-bit keys with up to 4 bits set - 3469497 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1401.0, actual   1462 (1.04x) (62)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 53/43 (1.21x)
Testing collisions (low  32-bit) - Expected       1401.0, actual   1406 (1.00x) (6)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 60/43 (1.37x)
Testing distribution - Worst bias is the 19-bit window at bit 12 - 0.088%

Keyset 'Sparse' - 160-bit keys with up to 4 bits set - 26977161 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      84546.1, actual  84527 (1.00x) (-19)
Testing collisions (high 28-44 bits) - Worst is 44 bits: 29/20 (1.40x)
Testing collisions (low  32-bit) - Expected      84546.1, actual  84586 (1.00x) (40)
Testing collisions (low  28-44 bits) - Worst is 41 bits: 172/165 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 39 - 0.017%

Keyset 'Sparse' - 256-bit keys with up to 3 bits set - 2796417 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        910.2, actual    947 (1.04x) (37)
Testing collisions (high 25-37 bits) - Worst is 36 bits: 63/56 (1.11x)
Testing collisions (low  32-bit) - Expected        910.2, actual    925 (1.02x) (15)
Testing collisions (low  25-37 bits) - Worst is 33 bits: 473/455 (1.04x)
Testing distribution - Worst bias is the 19-bit window at bit 50 - 0.082%

Keyset 'Sparse' - 512-bit keys with up to 3 bits set - 22370049 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      58155.4, actual  57664 (0.99x) (-491)
Testing collisions (high 28-43 bits) - Worst is 38 bits: 924/910 (1.02x)
Testing collisions (low  32-bit) - Expected      58155.4, actual  58193 (1.00x) (38)
Testing collisions (low  28-43 bits) - Worst is 39 bits: 498/455 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 23 - 0.017%

Keyset 'Sparse' - 1024-bit keys with up to 2 bits set - 524801 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         32.1, actual     30 (0.94x)
Testing collisions (high 22-32 bits) - Worst is 28 bits: 518/512 (1.01x)
Testing collisions (low  32-bit) - Expected         32.1, actual     23 (0.72x)
Testing collisions (low  22-32 bits) - Worst is 29 bits: 294/256 (1.15x)
Testing distribution - Worst bias is the 16-bit window at bit 48 - 0.155%

Keyset 'Sparse' - 2048-bit keys with up to 2 bits set - 2098177 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        512.4, actual    499 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4139/4094 (1.01x)
Testing collisions (low  32-bit) - Expected        512.4, actual    529 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 143/128 (1.12x)
Testing distribution - Worst bias is the 18-bit window at bit 30 - 0.057%


[[[ Keyset 'Permutation' Tests ]]]

Combination Lowbits Tests:
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    670 (1.00x) (2)
Testing collisions (high 24-37 bits) - Worst is 36 bits: 44/41 (1.05x)
Testing collisions (low  32-bit) - Expected        668.6, actual    655 (0.98x)
Testing collisions (low  24-37 bits) - Worst is 34 bits: 173/167 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit  9 - 0.080%


Combination Highbits Tests
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    657 (0.98x) (-11)
Testing collisions (high 24-37 bits) - Worst is 37 bits: 24/20 (1.15x)
Testing collisions (low  32-bit) - Expected        668.6, actual    691 (1.03x) (23)
Testing collisions (low  24-37 bits) - Worst is 33 bits: 362/334 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit  9 - 0.047%


Combination Hi-Lo Tests:
Keyset 'Combination' - up to 6 blocks from a set of 15 - 12204240 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      17322.9, actual  17106 (0.99x) (-216)
Testing collisions (high 27-41 bits) - Worst is 41 bits: 37/33 (1.09x)
Testing collisions (low  32-bit) - Expected      17322.9, actual  17214 (0.99x) (-108)
Testing collisions (low  27-41 bits) - Worst is 28 bits: 273805/273271 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit  3 - 0.030%


Combination 0x8000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8288 (1.01x) (102)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8135 (0.99x) (-51)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 20-bit window at bit 52 - 0.038%


Combination 0x0000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8207 (1.00x) (21)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8100 (0.99x) (-86)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit 43 - 0.036%


Combination 0x800000000000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   7983 (0.98x)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8152 (1.00x) (-34)
Testing collisions (low  26-40 bits) - Worst is 36 bits: 525/511 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit  4 - 0.054%


Combination 0x000000000000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8201 (1.00x) (15)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 66/63 (1.03x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8064 (0.99x) (-122)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 20-bit window at bit 49 - 0.040%


Combination 16-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8156 (1.00x) (-30)
Testing collisions (high 26-40 bits) - Worst is 28 bits: 130239/129717 (1.00x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8355 (1.02x) (169)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 20-bit window at bit 44 - 0.040%


Combination 16-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8046 (0.98x) (-140)
Testing collisions (high 26-40 bits) - Worst is 36 bits: 550/511 (1.07x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8034 (0.98x) (-152)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 265/255 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 24 - 0.042%


Combination 32-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8155 (1.00x) (-31)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 84/63 (1.31x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8295 (1.01x) (109)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 41/31 (1.28x)
Testing distribution - Worst bias is the 20-bit window at bit  4 - 0.036%


Combination 32-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8248 (1.01x) (62)
Testing collisions (high 26-40 bits) - Worst is 37 bits: 279/255 (1.09x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8246 (1.01x) (60)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 20-bit window at bit 51 - 0.040%


Combination 64-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8174 (1.00x) (-12)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 136/127 (1.06x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8053 (0.98x) (-133)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 269/255 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 46 - 0.043%


Combination 64-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8175 (1.00x) (-11)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 68/63 (1.06x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8116 (0.99x) (-70)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 20-bit window at bit 10 - 0.042%


Combination 128-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8223 (1.00x) (37)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 130/127 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8294 (1.01x) (108)
Testing collisions (low  26-40 bits) - Worst is 38 bits: 149/127 (1.16x)
Testing distribution - Worst bias is the 20-bit window at bit 55 - 0.045%


Combination 128-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8163 (1.00x) (-23)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8198 (1.00x) (12)
Testing collisions (low  26-40 bits) - Worst is 35 bits: 1055/1023 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 45 - 0.041%


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
Testing collisions (high 32-bit) - Expected        116.4, actual    104 (0.89x)
Testing collisions (high 23-34 bits) - Worst is 24 bits: 29355/29218 (1.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    107 (0.92x)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing distribution - Worst bias is the 17-bit window at bit 20 - 0.156%

Keyset 'Cyclic' - 8 cycles of 9 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    117 (1.01x) (1)
Testing collisions (high 23-34 bits) - Worst is 32 bits: 117/116 (1.01x)
Testing collisions (low  32-bit) - Expected        116.4, actual    111 (0.95x)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 17-bit window at bit 40 - 0.129%

Keyset 'Cyclic' - 8 cycles of 10 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual     95 (0.82x)
Testing collisions (high 23-34 bits) - Worst is 24 bits: 29375/29218 (1.01x)
Testing collisions (low  32-bit) - Expected        116.4, actual    136 (1.17x) (20)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 17-bit window at bit 12 - 0.166%

Keyset 'Cyclic' - 8 cycles of 11 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    110 (0.94x)
Testing collisions (high 23-34 bits) - Worst is 23 bits: 57228/57305 (1.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    128 (1.10x) (12)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing distribution - Worst bias is the 17-bit window at bit 40 - 0.096%

Keyset 'Cyclic' - 8 cycles of 12 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    102 (0.88x)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 30/29 (1.03x)
Testing collisions (low  32-bit) - Expected        116.4, actual    111 (0.95x)
Testing collisions (low  23-34 bits) - Worst is 24 bits: 29234/29218 (1.00x)
Testing distribution - Worst bias is the 17-bit window at bit 27 - 0.099%

Keyset 'Cyclic' - 8 cycles of 16 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    128 (1.10x) (12)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing collisions (low  32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (low  23-34 bits) - Worst is 29 bits: 930/930 (1.00x)
Testing distribution - Worst bias is the 17-bit window at bit 30 - 0.148%


[[[ Keyset 'TwoBytes' Tests ]]]

Keyset 'TwoBytes' - up-to-4-byte keys, 652545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         49.6, actual     47 (0.95x)
Testing collisions (high 23-33 bits) - Worst is 33 bits: 29/24 (1.17x)
Testing collisions (low  32-bit) - Expected         49.6, actual     47 (0.95x)
Testing collisions (low  23-33 bits) - Worst is 33 bits: 27/24 (1.09x)
Testing distribution - Worst bias is the 16-bit window at bit 16 - 0.189%

Keyset 'TwoBytes' - up-to-8-byte keys, 5471025 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       3483.1, actual   3504 (1.01x) (21)
Testing collisions (high 26-39 bits) - Worst is 39 bits: 31/27 (1.14x)
Testing collisions (low  32-bit) - Expected       3483.1, actual   3493 (1.00x) (10)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 38/27 (1.40x)
Testing distribution - Worst bias is the 20-bit window at bit 52 - 0.096%

Keyset 'TwoBytes' - up-to-12-byte keys, 18616785 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      40289.5, actual  40041 (0.99x) (-248)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 45/39 (1.14x)
Testing collisions (low  32-bit) - Expected      40289.5, actual  40866 (1.01x) (577)
Testing collisions (low  27-42 bits) - Worst is 40 bits: 177/157 (1.12x)
Testing distribution - Worst bias is the 20-bit window at bit 18 - 0.021%

Keyset 'TwoBytes' - up-to-16-byte keys, 44251425 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     227182.3, actual 227276 (1.00x) (94)
Testing collisions (high 29-45 bits) - Worst is 40 bits: 938/890 (1.05x)
Testing collisions (low  32-bit) - Expected     227182.3, actual 227236 (1.00x) (54)
Testing collisions (low  29-45 bits) - Worst is 45 bits: 37/27 (1.33x)
Testing distribution - Worst bias is the 20-bit window at bit 18 - 0.006%

Keyset 'TwoBytes' - up-to-20-byte keys, 86536545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     865959.1, actual 866307 (1.00x) (348)
Testing collisions (high 30-47 bits) - Worst is 37 bits: 27622/27237 (1.01x)
Testing collisions (low  32-bit) - Expected     865959.1, actual 865094 (1.00x) (-865)
Testing collisions (low  30-47 bits) - Worst is 46 bits: 68/53 (1.28x)
Testing distribution - Worst bias is the 20-bit window at bit 37 - 0.004%


[[[ Keyset 'Text' Tests ]]]

Keyset 'Text' - keys of form "FooXXXXBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25376 (1.00x) (-13)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 26/24 (1.05x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25263 (1.00x) (-126)
Testing collisions (low  27-42 bits) - Worst is 38 bits: 425/397 (1.07x)
Testing distribution - Worst bias is the 19-bit window at bit 27 - 0.018%

Keyset 'Text' - keys of form "FooBarXXXX" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25364 (1.00x) (-25)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 27/24 (1.09x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25570 (1.01x) (181)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 30/24 (1.21x)
Testing distribution - Worst bias is the 20-bit window at bit 13 - 0.031%

Keyset 'Text' - keys of form "XXXXFooBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25556 (1.01x) (167)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 63/49 (1.27x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25622 (1.01x) (233)
Testing collisions (low  27-42 bits) - Worst is 39 bits: 212/198 (1.07x)
Testing distribution - Worst bias is the 20-bit window at bit 54 - 0.031%

Keyset 'Words' - 4000000 random keys of len 6-16 from alnum charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1859 (1.00x) (-3)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 70/58 (1.20x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1926 (1.03x) (64)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 19-bit window at bit 30 - 0.067%

Keyset 'Words' - 4000000 random keys of len 6-16 from password charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1860 (1.00x) (-2)
Testing collisions (high 25-38 bits) - Worst is 27 bits: 59197/59016 (1.00x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1829 (0.98x) (-33)
Testing collisions (low  25-38 bits) - Worst is 25 bits: 228540/229220 (1.00x)
Testing distribution - Worst bias is the 19-bit window at bit  7 - 0.042%

Keyset 'Words' - 466569 dict words
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         25.3, actual     31 (1.22x) (6)
Testing collisions (high 22-32 bits) - Worst is 32 bits: 31/25 (1.22x)
Testing collisions (low  32-bit) - Expected         25.3, actual     20 (0.79x)
Testing collisions (low  22-32 bits) - Worst is 26 bits: 1650/1618 (1.02x)
Testing distribution - Worst bias is the 16-bit window at bit  1 - 0.220%


[[[ Keyset 'Zeroes' Tests ]]]

Keyset 'Zeroes' - 204800 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          4.9, actual      5 (1.02x) (1)
Testing collisions (high 21-29 bits) - Worst is 29 bits: 44/39 (1.13x)
Testing collisions (low  32-bit) - Expected          4.9, actual      4 (0.82x)
Testing collisions (low  21-29 bits) - Worst is 29 bits: 49/39 (1.25x)
Testing distribution - Worst bias is the 15-bit window at bit 29 - 0.313%


[[[ Keyset 'Seed' Tests ]]]

Keyset 'Seed' - 5000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2909.3, actual   2994 (1.03x) (85)
Testing collisions (high 26-39 bits) - Worst is 37 bits: 103/90 (1.13x)
Testing collisions (low  32-bit) - Expected       2909.3, actual   2973 (1.02x) (64)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 36/22 (1.58x)
Testing distribution - Worst bias is the 19-bit window at bit 10 - 0.042%


[[[ Keyset 'PerlinNoise' Tests ]]]

Testing 16777216 coordinates (L2) : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      32725.4, actual  32957 (1.01x) (232)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected      32725.4, actual  32857 (1.00x) (132)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 69/63 (1.08x)

Testing AV variant, 128 count with 4 spacing, 4-12:
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1116.2, actual   1110 (0.99x) (-6)
Testing collisions (high 25-37 bits) - Worst is 31 bits: 2271/2231 (1.02x)
Testing collisions (low  32-bit) - Expected       1116.2, actual   1068 (0.96x)
Testing collisions (low  25-37 bits) - Worst is 37 bits: 42/34 (1.20x)


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
Testing collisions (high 32-bit) - Expected        511.9, actual    473 (0.92x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 145/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.095%

Testing bit 1
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 17-bit window at bit 23 - 0.077%

Testing bit 2
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    479 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8185/8170 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 136/127 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.062%

Testing bit 3
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    490 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2047/2046 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8326/8170 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 10 - 0.061%

Testing bit 4
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    463 (0.90x)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64159/64191 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.076%

Testing bit 5
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 135/127 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 58 - 0.071%

Testing bit 6
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 17-bit window at bit 28 - 0.054%

Testing bit 7
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    553 (1.08x) (42)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 278/255 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing distribution - Worst bias is the 17-bit window at bit 60 - 0.058%

Testing bit 8
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing collisions (low  32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 519/511 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.088%

Testing bit 9
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 269/255 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 147/127 (1.15x)
Testing distribution - Worst bias is the 18-bit window at bit 29 - 0.063%

Testing bit 10
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 131/127 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.081%

Testing bit 11
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 67/63 (1.05x)
Testing distribution - Worst bias is the 17-bit window at bit 45 - 0.064%

Testing bit 12
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    565 (1.10x) (54)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 79/63 (1.23x)
Testing distribution - Worst bias is the 18-bit window at bit 16 - 0.086%

Testing bit 13
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 517/511 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    471 (0.92x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125649/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 29 - 0.071%

Testing bit 14
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 66/63 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 54 - 0.104%

Testing bit 15
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 513/511 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2182/2046 (1.07x)
Testing distribution - Worst bias is the 18-bit window at bit  7 - 0.062%

Testing bit 16
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2094/2046 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.103%

Testing bit 17
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    480 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 25 bits: 64414/64191 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    480 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 30 - 0.090%

Testing bit 18
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.072%

Testing bit 19
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    556 (1.09x) (45)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 45 - 0.055%

Testing bit 20
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 283/255 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    548 (1.07x) (37)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.061%

Testing bit 21
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    489 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4170/4090 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.063%

Testing bit 22
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 135/127 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.111%

Testing bit 23
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    486 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8215/8170 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.096%

Testing bit 24
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 139/127 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.085%

Testing bit 25
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    484 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit  2 - 0.072%

Testing bit 26
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1046/1023 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 44 - 0.060%

Testing bit 27
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 12 - 0.067%

Testing bit 28
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1027/1023 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 44 - 0.072%

Testing bit 29
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    560 (1.09x) (49)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 153/127 (1.20x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 17-bit window at bit 59 - 0.067%

Testing bit 30
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    484 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4102/4090 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 516/511 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 16 - 0.084%

Testing bit 31
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    549 (1.07x) (38)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 549/511 (1.07x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit  6 - 0.070%

Testing bit 32
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 138/127 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 31 - 0.086%

Testing bit 33
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    536 (1.05x) (25)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 271/255 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    568 (1.11x) (57)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 79/63 (1.23x)
Testing distribution - Worst bias is the 18-bit window at bit  9 - 0.075%

Testing bit 34
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 257/255 (1.00x)
Testing distribution - Worst bias is the 17-bit window at bit 24 - 0.047%

Testing bit 35
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 17-bit window at bit 28 - 0.061%

Testing bit 36
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2062/2046 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 16 - 0.092%

Testing bit 37
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.085%

Testing bit 38
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    489 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    556 (1.09x) (45)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 556/511 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.045%

Testing bit 39
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing collisions (low  32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 282/255 (1.10x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.081%

Testing bit 40
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    459 (0.90x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.095%

Testing bit 41
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 135/127 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 270/255 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 31 - 0.100%

Testing bit 42
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1041/1023 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    477 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2094/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.080%

Testing bit 43
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 276/255 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.091%

Testing bit 44
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2095/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.088%

Testing bit 45
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    559 (1.09x) (48)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    558 (1.09x) (47)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit  4 - 0.073%

Testing bit 46
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 271/255 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.073%

Testing bit 47
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    472 (0.92x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125700/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 11 - 0.084%

Testing bit 48
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32601/32429 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 139/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 46 - 0.053%

Testing bit 49
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1046/1023 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    544 (1.06x) (33)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit  5 - 0.074%

Testing bit 50
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    558 (1.09x) (47)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 66/63 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 56 - 0.061%

Testing bit 51
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.079%

Testing bit 52
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8213/8170 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    477 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 28 - 0.110%

Testing bit 53
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit  7 - 0.073%

Testing bit 54
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 283/255 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit  5 - 0.079%

Testing bit 55
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 270/255 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing distribution - Worst bias is the 18-bit window at bit 54 - 0.066%

Testing bit 56
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4143/4090 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing distribution - Worst bias is the 18-bit window at bit 17 - 0.112%

Testing bit 57
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.142%

Testing bit 58
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 131/127 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 269/255 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.091%

Testing bit 59
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    541 (1.06x) (30)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 146/127 (1.14x)
Testing distribution - Worst bias is the 18-bit window at bit  4 - 0.056%

Testing bit 60
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 82/63 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.083%

Testing bit 61
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    545 (1.06x) (34)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 545/511 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 526/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 12 - 0.083%

Testing bit 62
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 75/63 (1.17x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 37 - 0.095%

Testing bit 63
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 264/255 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 68/63 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit  8 - 0.077%


[[[ MomentChi2 Tests ]]]

Analyze hashes produced from a serie of linearly increasing numbers of 32-bit, using a step of 2 ... 
Target values to approximate : 38918200.000000 - 273633.333333 
Popcount 1 stats : 38918998.920034 - 273638.558723
Popcount 0 stats : 38918770.531038 - 273645.996303
MomentChi2 for bits 1 :   1.16628 
MomentChi2 for bits 0 :  0.594771 

Derivative stats (transition from 2 consecutive values) : 
Popcount 1 stats : 38919257.619283 - 273628.012809
Popcount 0 stats : 38918472.697479 - 273650.071385
MomentChi2 for deriv b1 :   2.04392 
MomentChi2 for deriv b0 :  0.135878 

  Great 


[[[ Prng Tests ]]]

Generating 33554432 random numbers : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     130731.3, actual 130884 (1.00x) (153)
Testing collisions (high 28-44 bits) - Worst is 43 bits: 75/63 (1.17x)
Testing collisions (low  32-bit) - Expected     130731.3, actual 131211 (1.00x) (480)
Testing collisions (low  28-44 bits) - Worst is 43 bits: 68/63 (1.06x)

[[[ BadSeeds Tests ]]]

0x0 PASS


Input vcode 0x00000001, Output vcode 0x00000001, Result vcode 0x00000001
Verification value is 0x00000001 - Testing took 755.424002 seconds
-------------------------------------------------------------------------------
```

-------------------------------------------------------------------------------
