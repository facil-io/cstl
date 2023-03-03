### Risky Hash SMHasher Results

The following results were achieved on my personal computer when testing the facil.io Risky Hash (`fio_risky_hash`).

```txt
-------------------------------------------------------------------------------
--- Testing Risky "facil.io Risky Hash" GOOD

[[[ Sanity Tests ]]]

Verification value 0x8EA1315C ....... PASS
Running sanity check 1     .......... PASS
Running AppendedZeroesTest .......... PASS

[[[ Speed Tests ]]]

Bulk speed test - 262144-byte keys
Alignment  7 - 10.030 bytes/cycle - 28695.79 MiB/sec @ 3 ghz
Alignment  6 - 10.029 bytes/cycle - 28694.02 MiB/sec @ 3 ghz
Alignment  5 - 10.030 bytes/cycle - 28696.76 MiB/sec @ 3 ghz
Alignment  4 - 10.037 bytes/cycle - 28715.74 MiB/sec @ 3 ghz
Alignment  3 - 10.030 bytes/cycle - 28696.32 MiB/sec @ 3 ghz
Alignment  2 -  9.995 bytes/cycle - 28595.75 MiB/sec @ 3 ghz
Alignment  1 - 10.030 bytes/cycle - 28695.60 MiB/sec @ 3 ghz
Alignment  0 - 10.032 bytes/cycle - 28702.83 MiB/sec @ 3 ghz
Average      - 10.027 bytes/cycle - 28686.60 MiB/sec @ 3 ghz

Small key speed test -    1-byte keys -    17.00 cycles/hash
Small key speed test -    2-byte keys -    17.96 cycles/hash
Small key speed test -    3-byte keys -    19.00 cycles/hash
Small key speed test -    4-byte keys -    19.96 cycles/hash
Small key speed test -    5-byte keys -    21.00 cycles/hash
Small key speed test -    6-byte keys -    22.00 cycles/hash
Small key speed test -    7-byte keys -    22.99 cycles/hash
Small key speed test -    8-byte keys -    16.99 cycles/hash
Small key speed test -    9-byte keys -    18.64 cycles/hash
Small key speed test -   10-byte keys -    19.66 cycles/hash
Small key speed test -   11-byte keys -    20.40 cycles/hash
Small key speed test -   12-byte keys -    21.49 cycles/hash
Small key speed test -   13-byte keys -    22.47 cycles/hash
Small key speed test -   14-byte keys -    23.22 cycles/hash
Small key speed test -   15-byte keys -    23.97 cycles/hash
Small key speed test -   16-byte keys -    17.00 cycles/hash
Small key speed test -   17-byte keys -    19.00 cycles/hash
Small key speed test -   18-byte keys -    20.00 cycles/hash
Small key speed test -   19-byte keys -    20.99 cycles/hash
Small key speed test -   20-byte keys -    22.00 cycles/hash
Small key speed test -   21-byte keys -    23.00 cycles/hash
Small key speed test -   22-byte keys -    23.99 cycles/hash
Small key speed test -   23-byte keys -    25.07 cycles/hash
Small key speed test -   24-byte keys -    17.93 cycles/hash
Small key speed test -   25-byte keys -    18.64 cycles/hash
Small key speed test -   26-byte keys -    19.06 cycles/hash
Small key speed test -   27-byte keys -    19.86 cycles/hash
Small key speed test -   28-byte keys -    21.08 cycles/hash
Small key speed test -   29-byte keys -    21.99 cycles/hash
Small key speed test -   30-byte keys -    22.94 cycles/hash
Small key speed test -   31-byte keys -    23.99 cycles/hash
Average                                    20.750 cycles/hash

[[[ 'Hashmap' Speed Tests ]]]

std::unordered_map
Init std HashMapTest:     167.908 cycles/op (466569 inserts, 1% deletions)
Running std HashMapTest:  110.055 cycles/op (5.3 stdv)

greg7mdp/parallel-hashmap
Init fast HashMapTest:    232.017 cycles/op (466569 inserts, 1% deletions)
Running fast HashMapTest: 86.434 cycles/op (1.6 stdv)  ....... PASS

[[[ Avalanche Tests ]]]

Testing   24-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.759333%
Testing   32-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.586000%
Testing   40-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.793333%
Testing   48-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.736667%
Testing   56-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.703333%
Testing   64-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.732667%
Testing   72-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.721333%
Testing   80-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.647333%
Testing   96-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.788000%
Testing  112-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.779333%
Testing  128-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.760000%
Testing  160-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.823333%
Testing  512-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.786000%
Testing 1024-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.791333%

[[[ Keyset 'Sparse' Tests ]]]

Keyset 'Sparse' - 16-bit keys with up to 9 bits set - 50643 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          0.3, actual      1 (3.35x) (1) !
Testing collisions (high 19-25 bits) - Worst is 19 bits: 2349/2368 (0.99x)
Testing collisions (low  32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (low  19-25 bits) - Worst is 19 bits: 2373/2368 (1.00x)
Testing distribution - Worst bias is the 13-bit window at bit 48 - 0.370%

Keyset 'Sparse' - 24-bit keys with up to 8 bits set - 1271626 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        188.2, actual    206 (1.09x) (18)
Testing collisions (high 24-35 bits) - Worst is 33 bits: 111/94 (1.18x)
Testing collisions (low  32-bit) - Expected        188.2, actual    198 (1.05x) (10)
Testing collisions (low  24-35 bits) - Worst is 35 bits: 31/23 (1.32x)
Testing distribution - Worst bias is the 17-bit window at bit 57 - 0.129%

Keyset 'Sparse' - 32-bit keys with up to 7 bits set - 4514873 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2372.2, actual   2258 (0.95x)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 80/74 (1.08x)
Testing collisions (low  32-bit) - Expected       2372.2, actual   2350 (0.99x) (-22)
Testing collisions (low  25-38 bits) - Worst is 30 bits: 9473/9478 (1.00x)
Testing distribution - Worst bias is the 19-bit window at bit 41 - 0.050%

Keyset 'Sparse' - 40-bit keys with up to 6 bits set - 4598479 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2460.8, actual   2435 (0.99x) (-25)
Testing collisions (high 25-38 bits) - Worst is 25 bits: 301722/301185 (1.00x)
Testing collisions (low  32-bit) - Expected       2460.8, actual   2388 (0.97x)
Testing collisions (low  25-38 bits) - Worst is 36 bits: 163/153 (1.06x)
Testing distribution - Worst bias is the 19-bit window at bit 23 - 0.048%

Keyset 'Sparse' - 48-bit keys with up to 6 bits set - 14196869 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      23437.8, actual  23461 (1.00x) (24)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 27/22 (1.18x)
Testing collisions (low  32-bit) - Expected      23437.8, actual  23607 (1.01x) (170)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 27/22 (1.18x)
Testing distribution - Worst bias is the 20-bit window at bit 13 - 0.022%

Keyset 'Sparse' - 56-bit keys with up to 5 bits set - 4216423 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2069.0, actual   1987 (0.96x)
Testing collisions (high 25-38 bits) - Worst is 25 bits: 254222/254159 (1.00x)
Testing collisions (low  32-bit) - Expected       2069.0, actual   2097 (1.01x) (29)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 36/32 (1.11x)
Testing distribution - Worst bias is the 19-bit window at bit 27 - 0.050%

Keyset 'Sparse' - 64-bit keys with up to 5 bits set - 8303633 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8021.7, actual   8000 (1.00x) (-21)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 34/31 (1.08x)
Testing collisions (low  32-bit) - Expected       8021.7, actual   7940 (0.99x) (-81)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 40/31 (1.28x)
Testing distribution - Worst bias is the 20-bit window at bit 42 - 0.040%

Keyset 'Sparse' - 72-bit keys with up to 5 bits set - 15082603 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      26451.8, actual  26556 (1.00x) (105)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 57/51 (1.10x)
Testing collisions (low  32-bit) - Expected      26451.8, actual  26449 (1.00x) (-2)
Testing collisions (low  27-42 bits) - Worst is 40 bits: 119/103 (1.15x)
Testing distribution - Worst bias is the 20-bit window at bit 45 - 0.022%

Keyset 'Sparse' - 96-bit keys with up to 4 bits set - 3469497 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1401.0, actual   1417 (1.01x) (17)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 26/21 (1.19x)
Testing collisions (low  32-bit) - Expected       1401.0, actual   1433 (1.02x) (33)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 31/21 (1.42x)
Testing distribution - Worst bias is the 19-bit window at bit 58 - 0.094%

Keyset 'Sparse' - 160-bit keys with up to 4 bits set - 26977161 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      84546.1, actual  84347 (1.00x) (-199)
Testing collisions (high 28-44 bits) - Worst is 44 bits: 27/20 (1.31x)
Testing collisions (low  32-bit) - Expected      84546.1, actual  84804 (1.00x) (258)
Testing collisions (low  28-44 bits) - Worst is 43 bits: 46/41 (1.11x)
Testing distribution - Worst bias is the 20-bit window at bit 52 - 0.014%

Keyset 'Sparse' - 256-bit keys with up to 3 bits set - 2796417 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        910.2, actual    915 (1.01x) (5)
Testing collisions (high 25-37 bits) - Worst is 36 bits: 71/56 (1.25x)
Testing collisions (low  32-bit) - Expected        910.2, actual    910 (1.00x)
Testing collisions (low  25-37 bits) - Worst is 31 bits: 1855/1819 (1.02x)
Testing distribution - Worst bias is the 19-bit window at bit  0 - 0.102%

Keyset 'Sparse' - 512-bit keys with up to 3 bits set - 22370049 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      58155.4, actual  58172 (1.00x) (17)
Testing collisions (high 28-43 bits) - Worst is 40 bits: 253/227 (1.11x)
Testing collisions (low  32-bit) - Expected      58155.4, actual  58037 (1.00x) (-118)
Testing collisions (low  28-43 bits) - Worst is 41 bits: 136/113 (1.20x)
Testing distribution - Worst bias is the 20-bit window at bit 59 - 0.015%

Keyset 'Sparse' - 1024-bit keys with up to 2 bits set - 524801 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         32.1, actual     27 (0.84x)
Testing collisions (high 22-32 bits) - Worst is 30 bits: 142/128 (1.11x)
Testing collisions (low  32-bit) - Expected         32.1, actual     37 (1.15x) (5)
Testing collisions (low  22-32 bits) - Worst is 29 bits: 299/256 (1.17x)
Testing distribution - Worst bias is the 16-bit window at bit 55 - 0.110%

Keyset 'Sparse' - 2048-bit keys with up to 2 bits set - 2098177 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        512.4, actual    460 (0.90x)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32425/32460 (1.00x)
Testing collisions (low  32-bit) - Expected        512.4, actual    481 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8226/8178 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.073%


[[[ Keyset 'Permutation' Tests ]]]

Combination Lowbits Tests:
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    672 (1.01x) (4)
Testing collisions (high 24-37 bits) - Worst is 37 bits: 27/20 (1.29x)
Testing collisions (low  32-bit) - Expected        668.6, actual    650 (0.97x)
Testing collisions (low  24-37 bits) - Worst is 34 bits: 170/167 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 45 - 0.077%


Combination Highbits Tests
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    683 (1.02x) (15)
Testing collisions (high 24-37 bits) - Worst is 37 bits: 26/20 (1.24x)
Testing collisions (low  32-bit) - Expected        668.6, actual    656 (0.98x) (-12)
Testing collisions (low  24-37 bits) - Worst is 34 bits: 178/167 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.054%


Combination Hi-Lo Tests:
Keyset 'Combination' - up to 6 blocks from a set of 15 - 12204240 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      17322.9, actual  17089 (0.99x) (-233)
Testing collisions (high 27-41 bits) - Worst is 27 bits: 538204/538415 (1.00x)
Testing collisions (low  32-bit) - Expected      17322.9, actual  17308 (1.00x) (-14)
Testing collisions (low  27-41 bits) - Worst is 37 bits: 549/541 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit 27 - 0.021%


Combination 0x8000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8130 (0.99x) (-56)
Testing collisions (high 26-40 bits) - Worst is 33 bits: 4118/4094 (1.01x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8299 (1.01x) (113)
Testing collisions (low  26-40 bits) - Worst is 33 bits: 4192/4094 (1.02x)
Testing distribution - Worst bias is the 20-bit window at bit 13 - 0.038%


Combination 0x0000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8195 (1.00x) (9)
Testing collisions (high 26-40 bits) - Worst is 37 bits: 258/255 (1.01x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8327 (1.02x) (141)
Testing collisions (low  26-40 bits) - Worst is 35 bits: 1055/1023 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 34 - 0.043%


Combination 0x800000000000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8337 (1.02x) (151)
Testing collisions (high 26-40 bits) - Worst is 32 bits: 8337/8186 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8237 (1.01x) (51)
Testing collisions (low  26-40 bits) - Worst is 33 bits: 4152/4094 (1.01x)
Testing distribution - Worst bias is the 19-bit window at bit 41 - 0.031%


Combination 0x000000000000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8204 (1.00x) (18)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8172 (1.00x) (-14)
Testing collisions (low  26-40 bits) - Worst is 39 bits: 70/63 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 50 - 0.049%


Combination 16-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8236 (1.01x) (50)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8076 (0.99x) (-110)
Testing collisions (low  26-40 bits) - Worst is 39 bits: 71/63 (1.11x)
Testing distribution - Worst bias is the 19-bit window at bit 16 - 0.027%


Combination 16-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8068 (0.99x) (-118)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8264 (1.01x) (78)
Testing collisions (low  26-40 bits) - Worst is 39 bits: 67/63 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit  8 - 0.040%


Combination 32-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8156 (1.00x) (-30)
Testing collisions (high 26-40 bits) - Worst is 35 bits: 1040/1023 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8361 (1.02x) (175)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 20-bit window at bit  4 - 0.044%


Combination 32-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8211 (1.00x) (25)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 45/31 (1.41x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8201 (1.00x) (15)
Testing collisions (low  26-40 bits) - Worst is 38 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 25 - 0.033%


Combination 64-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8274 (1.01x) (88)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 134/127 (1.05x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8281 (1.01x) (95)
Testing collisions (low  26-40 bits) - Worst is 36 bits: 526/511 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 57 - 0.047%


Combination 64-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8305 (1.01x) (119)
Testing collisions (high 26-40 bits) - Worst is 33 bits: 4185/4094 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8116 (0.99x) (-70)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit 61 - 0.029%


Combination 128-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8363 (1.02x) (177)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 75/63 (1.17x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8090 (0.99x) (-96)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 290/255 (1.13x)
Testing distribution - Worst bias is the 20-bit window at bit 38 - 0.042%


Combination 128-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8167 (1.00x) (-19)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 65/63 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8223 (1.00x) (37)
Testing collisions (low  26-40 bits) - Worst is 33 bits: 4116/4094 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit 31 - 0.031%


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
Testing collisions (high 32-bit) - Expected        116.4, actual    124 (1.07x) (8)
Testing collisions (high 23-34 bits) - Worst is 33 bits: 63/58 (1.08x)
Testing collisions (low  32-bit) - Expected        116.4, actual    102 (0.88x)
Testing collisions (low  23-34 bits) - Worst is 33 bits: 61/58 (1.05x)
Testing distribution - Worst bias is the 16-bit window at bit 12 - 0.099%

Keyset 'Cyclic' - 8 cycles of 9 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    100 (0.86x)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 30/29 (1.03x)
Testing collisions (low  32-bit) - Expected        116.4, actual     93 (0.80x)
Testing collisions (low  23-34 bits) - Worst is 26 bits: 7479/7413 (1.01x)
Testing distribution - Worst bias is the 16-bit window at bit 36 - 0.061%

Keyset 'Cyclic' - 8 cycles of 10 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    120 (1.03x) (4)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing collisions (low  32-bit) - Expected        116.4, actual    114 (0.98x)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing distribution - Worst bias is the 17-bit window at bit  7 - 0.074%

Keyset 'Cyclic' - 8 cycles of 11 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual     97 (0.83x)
Testing collisions (high 23-34 bits) - Worst is 29 bits: 945/930 (1.02x)
Testing collisions (low  32-bit) - Expected        116.4, actual    104 (0.89x)
Testing collisions (low  23-34 bits) - Worst is 29 bits: 960/930 (1.03x)
Testing distribution - Worst bias is the 17-bit window at bit 32 - 0.096%

Keyset 'Cyclic' - 8 cycles of 12 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    102 (0.88x)
Testing collisions (high 23-34 bits) - Worst is 30 bits: 498/465 (1.07x)
Testing collisions (low  32-bit) - Expected        116.4, actual    112 (0.96x)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 17-bit window at bit 16 - 0.096%

Keyset 'Cyclic' - 8 cycles of 16 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    121 (1.04x) (5)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 35/29 (1.20x)
Testing collisions (low  32-bit) - Expected        116.4, actual    113 (0.97x)
Testing collisions (low  23-34 bits) - Worst is 30 bits: 466/465 (1.00x)
Testing distribution - Worst bias is the 16-bit window at bit 24 - 0.103%


[[[ Keyset 'TwoBytes' Tests ]]]

Keyset 'TwoBytes' - up-to-4-byte keys, 652545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         49.6, actual     63 (1.27x) (14)
Testing collisions (high 23-33 bits) - Worst is 32 bits: 63/49 (1.27x)
Testing collisions (low  32-bit) - Expected         49.6, actual     53 (1.07x) (4)
Testing collisions (low  23-33 bits) - Worst is 33 bits: 27/24 (1.09x)
Testing distribution - Worst bias is the 16-bit window at bit 63 - 0.122%

Keyset 'TwoBytes' - up-to-8-byte keys, 5471025 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       3483.1, actual   3549 (1.02x) (66)
Testing collisions (high 26-39 bits) - Worst is 39 bits: 34/27 (1.25x)
Testing collisions (low  32-bit) - Expected       3483.1, actual   3521 (1.01x) (38)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 31/27 (1.14x)
Testing distribution - Worst bias is the 20-bit window at bit 31 - 0.058%

Keyset 'TwoBytes' - up-to-12-byte keys, 18616785 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      40289.5, actual  40554 (1.01x) (265)
Testing collisions (high 27-42 bits) - Worst is 40 bits: 196/157 (1.24x)
Testing collisions (low  32-bit) - Expected      40289.5, actual  40552 (1.01x) (263)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 89/78 (1.13x)
Testing distribution - Worst bias is the 19-bit window at bit 47 - 0.016%

Keyset 'TwoBytes' - up-to-16-byte keys, 44251425 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     227182.3, actual 227709 (1.00x) (527)
Testing collisions (high 29-45 bits) - Worst is 44 bits: 72/55 (1.29x)
Testing collisions (low  32-bit) - Expected     227182.3, actual 227650 (1.00x) (468)
Testing collisions (low  29-45 bits) - Worst is 40 bits: 948/890 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit  8 - 0.007%

Keyset 'TwoBytes' - up-to-20-byte keys, 86536545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     865959.1, actual 866620 (1.00x) (661)
Testing collisions (high 30-47 bits) - Worst is 47 bits: 31/26 (1.17x)
Testing collisions (low  32-bit) - Expected     865959.1, actual 866109 (1.00x) (150)
Testing collisions (low  30-47 bits) - Worst is 45 bits: 113/106 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit  6 - 0.003%


[[[ Keyset 'Text' Tests ]]]

Keyset 'Text' - keys of form "FooXXXXBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25306 (1.00x) (-83)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 57/49 (1.15x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25035 (0.99x) (-354)
Testing collisions (low  27-42 bits) - Worst is 27 bits: 783377/784335 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit  7 - 0.031%

Keyset 'Text' - keys of form "FooBarXXXX" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25694 (1.01x) (305)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 57/49 (1.15x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25467 (1.00x) (78)
Testing collisions (low  27-42 bits) - Worst is 38 bits: 409/397 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit  4 - 0.026%

Keyset 'Text' - keys of form "XXXXFooBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25382 (1.00x) (-7)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 58/49 (1.17x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25369 (1.00x) (-20)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 26/24 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 58 - 0.015%

Keyset 'Words' - 4000000 random keys of len 6-16 from alnum charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1898 (1.02x) (36)
Testing collisions (high 25-38 bits) - Worst is 32 bits: 1898/1862 (1.02x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1933 (1.04x) (71)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 66/58 (1.13x)
Testing distribution - Worst bias is the 19-bit window at bit 61 - 0.035%

Keyset 'Words' - 4000000 random keys of len 6-16 from password charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1888 (1.01x) (26)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 36/29 (1.24x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1909 (1.03x) (47)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 64/58 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit 59 - 0.037%

Keyset 'Words' - 466569 dict words
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         25.3, actual     20 (0.79x)
Testing collisions (high 22-32 bits) - Worst is 23 bits: 12760/12737 (1.00x)
Testing collisions (low  32-bit) - Expected         25.3, actual     25 (0.99x)
Testing collisions (low  22-32 bits) - Worst is 29 bits: 220/202 (1.09x)
Testing distribution - Worst bias is the 16-bit window at bit 28 - 0.246%


[[[ Keyset 'Zeroes' Tests ]]]

Keyset 'Zeroes' - 204800 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          4.9, actual     12 (2.46x) (8) !
Testing collisions (high 21-29 bits) - Worst is 29 bits: 46/39 (1.18x)
Testing collisions (low  32-bit) - Expected          4.9, actual      2 (0.41x)
Testing collisions (low  21-29 bits) - Worst is 25 bits: 637/623 (1.02x)
Testing distribution - Worst bias is the 15-bit window at bit 32 - 0.232%


[[[ Keyset 'Seed' Tests ]]]

Keyset 'Seed' - 5000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2909.3, actual   2888 (0.99x) (-21)
Testing collisions (high 26-39 bits) - Worst is 33 bits: 1474/1454 (1.01x)
Testing collisions (low  32-bit) - Expected       2909.3, actual   2802 (0.96x)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 25/22 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit 61 - 0.038%


[[[ Keyset 'PerlinNoise' Tests ]]]

Testing 16777216 coordinates (L2) : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      32725.4, actual  32825 (1.00x) (100)
Testing collisions (high 27-42 bits) - Worst is 35 bits: 4139/4095 (1.01x)
Testing collisions (low  32-bit) - Expected      32725.4, actual  32577 (1.00x) (-148)
Testing collisions (low  27-42 bits) - Worst is 40 bits: 131/127 (1.02x)

Testing AV variant, 128 count with 4 spacing, 4-12:
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1116.2, actual   1047 (0.94x)
Testing collisions (high 25-37 bits) - Worst is 36 bits: 72/69 (1.03x)
Testing collisions (low  32-bit) - Expected       1116.2, actual   1113 (1.00x) (-3)
Testing collisions (low  25-37 bits) - Worst is 35 bits: 151/139 (1.08x)


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
Testing collisions (high 24-36 bits) - Worst is 32 bits: 539/511 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 34 - 0.054%

Testing bit 1
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.060%

Testing bit 2
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    486 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2096/2046 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.075%

Testing bit 3
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit  6 - 0.074%

Testing bit 4
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit  0 - 0.060%

Testing bit 5
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    540 (1.05x) (29)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 284/255 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 514/511 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.063%

Testing bit 6
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.070%

Testing bit 7
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 63943/64191 (1.00x)
Testing distribution - Worst bias is the 17-bit window at bit 36 - 0.042%

Testing bit 8
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 260/255 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    543 (1.06x) (32)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 543/511 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.113%

Testing bit 9
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    557 (1.09x) (46)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 557/511 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.098%

Testing bit 10
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 268/255 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 27 bits: 16400/16298 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 17 - 0.063%

Testing bit 11
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.075%

Testing bit 12
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 68/63 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 18 - 0.078%

Testing bit 13
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    542 (1.06x) (31)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 43 - 0.093%

Testing bit 14
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    564 (1.10x) (53)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 80/63 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    479 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125857/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.094%

Testing bit 15
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8252/8170 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.096%

Testing bit 16
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125705/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    556 (1.09x) (45)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 36 - 0.079%

Testing bit 17
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32468/32429 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    576 (1.13x) (65)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 576/511 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 22 - 0.042%

Testing bit 18
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 268/255 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 28 - 0.077%

Testing bit 19
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1027/1023 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 11 - 0.073%

Testing bit 20
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    463 (0.90x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4125/4090 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 272/255 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.074%

Testing bit 21
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 528/511 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    472 (0.92x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 24 - 0.072%

Testing bit 22
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 263/255 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit  4 - 0.044%

Testing bit 23
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 533/511 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.061%

Testing bit 24
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1069/1023 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit  8 - 0.057%

Testing bit 25
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    477 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4113/4090 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 37 - 0.096%

Testing bit 26
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8358/8170 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit  7 - 0.077%

Testing bit 27
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    461 (0.90x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    531 (1.04x) (20)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 11 - 0.061%

Testing bit 28
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125462/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 527/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 36 - 0.080%

Testing bit 29
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 45/31 (1.41x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 33 - 0.076%

Testing bit 30
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    557 (1.09x) (46)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.073%

Testing bit 31
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    544 (1.06x) (33)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 30 - 0.083%

Testing bit 32
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    543 (1.06x) (32)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 281/255 (1.10x)
Testing collisions (low  32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4149/4090 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 28 - 0.080%

Testing bit 33
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 267/255 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125343/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 21 - 0.059%

Testing bit 34
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 52 - 0.069%

Testing bit 35
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 143/127 (1.12x)
Testing collisions (low  32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 261/255 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit  2 - 0.093%

Testing bit 36
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 136/127 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    555 (1.08x) (44)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 37 - 0.086%

Testing bit 37
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    551 (1.08x) (40)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 18 - 0.062%

Testing bit 38
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4106/4090 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing distribution - Worst bias is the 17-bit window at bit 55 - 0.060%

Testing bit 39
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.081%

Testing bit 40
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8268/8170 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 17-bit window at bit 17 - 0.050%

Testing bit 41
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    493 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 129/127 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    544 (1.06x) (33)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 49 - 0.090%

Testing bit 42
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing collisions (low  32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2070/2046 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.110%

Testing bit 43
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2163/2046 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.061%

Testing bit 44
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 529/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 38 - 0.092%

Testing bit 45
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 75/63 (1.17x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 54 - 0.088%

Testing bit 46
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1056/1023 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    493 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1052/1023 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.101%

Testing bit 47
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    473 (0.92x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125038/125777 (0.99x)
Testing collisions (low  32-bit) - Expected        511.9, actual    489 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 259/255 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 44 - 0.060%

Testing bit 48
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 152/127 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1067/1023 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 24 - 0.066%

Testing bit 49
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 148/127 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 66/63 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.078%

Testing bit 50
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 138/127 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    479 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 43 - 0.098%

Testing bit 51
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 14 - 0.060%

Testing bit 52
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing distribution - Worst bias is the 18-bit window at bit 33 - 0.097%

Testing bit 53
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 31 - 0.070%

Testing bit 54
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    486 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 131/127 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 270/255 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 32 - 0.075%

Testing bit 55
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 137/127 (1.07x)
Testing collisions (low  32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit  8 - 0.088%

Testing bit 56
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2128/2046 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.091%

Testing bit 57
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125099/125777 (0.99x)
Testing collisions (low  32-bit) - Expected        511.9, actual    543 (1.06x) (32)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1107/1023 (1.08x)
Testing distribution - Worst bias is the 17-bit window at bit  0 - 0.063%

Testing bit 58
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    485 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing collisions (low  32-bit) - Expected        511.9, actual    486 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 59 - 0.078%

Testing bit 59
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    490 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 56 - 0.080%

Testing bit 60
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 149/127 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.073%

Testing bit 61
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1045/1023 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing distribution - Worst bias is the 18-bit window at bit 10 - 0.109%

Testing bit 62
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 45 - 0.063%

Testing bit 63
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    547 (1.07x) (36)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 285/255 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 22 - 0.081%


[[[ MomentChi2 Tests ]]]

Analyze hashes produced from a serie of linearly increasing numbers of 32-bit, using a step of 2 ... 
Target values to approximate : 38918200.000000 - 273633.333333 
Popcount 1 stats : 38918909.622742 - 273636.496203
Popcount 0 stats : 38919146.902142 - 273657.912377
MomentChi2 for bits 1 :  0.920139 
MomentChi2 for bits 0 :   1.63829 

Derivative stats (transition from 2 consecutive values) : 
Popcount 1 stats : 38919847.819394 - 273675.028795
Popcount 0 stats : 38918490.169516 - 273642.402601
MomentChi2 for deriv b1 :    4.9612 
MomentChi2 for deriv b0 :   0.15385 

  Great 


[[[ Prng Tests ]]]

Generating 33554432 random numbers : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     130731.3, actual 131061 (1.00x) (330)
Testing collisions (high 28-44 bits) - Worst is 42 bits: 133/127 (1.04x)
Testing collisions (low  32-bit) - Expected     130731.3, actual 130667 (1.00x) (-64)
Testing collisions (low  28-44 bits) - Worst is 42 bits: 140/127 (1.09x)

[[[ BadSeeds Tests ]]]

0x0 PASS


Input vcode 0x00000001, Output vcode 0x00000001, Result vcode 0x00000001
Verification value is 0x00000001 - Testing took 665.664205 seconds
-------------------------------------------------------------------------------
```

-------------------------------------------------------------------------------
