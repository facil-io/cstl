### Risky Hash SMHasher Results

The following results were achieved on my personal computer when testing the facil.io Risky Hash (`fio_risky_hash`).

```txt
-------------------------------------------------------------------------------
--- Testing Risky "facil.io Risky Hash" GOOD

[[[ Sanity Tests ]]]

Verification value 0x407D2C05 ....... PASS
Running sanity check 1     .......... PASS
Running AppendedZeroesTest .......... PASS

[[[ Speed Tests ]]]

Bulk speed test - 262144-byte keys
Alignment  7 - 17.892 bytes/cycle - 51190.53 MiB/sec @ 3 ghz
Alignment  6 - 17.906 bytes/cycle - 51229.69 MiB/sec @ 3 ghz
Alignment  5 - 17.900 bytes/cycle - 51211.34 MiB/sec @ 3 ghz
Alignment  4 - 17.889 bytes/cycle - 51179.97 MiB/sec @ 3 ghz
Alignment  3 - 17.901 bytes/cycle - 51215.52 MiB/sec @ 3 ghz
Alignment  2 - 17.877 bytes/cycle - 51145.72 MiB/sec @ 3 ghz
Alignment  1 - 17.857 bytes/cycle - 51089.71 MiB/sec @ 3 ghz
Alignment  0 - 19.055 bytes/cycle - 54516.81 MiB/sec @ 3 ghz
Average      - 18.035 bytes/cycle - 51597.41 MiB/sec @ 3 ghz

Small key speed test -    1-byte keys -    13.90 cycles/hash
Small key speed test -    2-byte keys -    14.62 cycles/hash
Small key speed test -    3-byte keys -    14.24 cycles/hash
Small key speed test -    4-byte keys -    14.00 cycles/hash
Small key speed test -    5-byte keys -    14.28 cycles/hash
Small key speed test -    6-byte keys -    14.95 cycles/hash
Small key speed test -    7-byte keys -    15.00 cycles/hash
Small key speed test -    8-byte keys -    14.00 cycles/hash
Small key speed test -    9-byte keys -    14.00 cycles/hash
Small key speed test -   10-byte keys -    14.00 cycles/hash
Small key speed test -   11-byte keys -    14.00 cycles/hash
Small key speed test -   12-byte keys -    14.00 cycles/hash
Small key speed test -   13-byte keys -    14.00 cycles/hash
Small key speed test -   14-byte keys -    14.00 cycles/hash
Small key speed test -   15-byte keys -    14.00 cycles/hash
Small key speed test -   16-byte keys -    14.61 cycles/hash
Small key speed test -   17-byte keys -    15.61 cycles/hash
Small key speed test -   18-byte keys -    15.68 cycles/hash
Small key speed test -   19-byte keys -    15.67 cycles/hash
Small key speed test -   20-byte keys -    15.61 cycles/hash
Small key speed test -   21-byte keys -    15.66 cycles/hash
Small key speed test -   22-byte keys -    15.64 cycles/hash
Small key speed test -   23-byte keys -    15.66 cycles/hash
Small key speed test -   24-byte keys -    15.71 cycles/hash
Small key speed test -   25-byte keys -    15.63 cycles/hash
Small key speed test -   26-byte keys -    15.64 cycles/hash
Small key speed test -   27-byte keys -    15.64 cycles/hash
Small key speed test -   28-byte keys -    15.65 cycles/hash
Small key speed test -   29-byte keys -    15.66 cycles/hash
Small key speed test -   30-byte keys -    15.64 cycles/hash
Small key speed test -   31-byte keys -    15.65 cycles/hash
Average                                    14.914 cycles/hash

[[[ 'Hashmap' Speed Tests ]]]

std::unordered_map
Init std HashMapTest:     147.319 cycles/op (466569 inserts, 1% deletions)
Running std HashMapTest:  88.414 cycles/op (1.1 stdv, found 461903)

greg7mdp/parallel-hashmap
Init fast HashMapTest:    138.590 cycles/op (466569 inserts, 1% deletions)
Running fast HashMapTest: 88.165 cycles/op (0.6 stdv, found 461903)

facil.io HashMap
Init fast fio_map_s Test:    75.517 cycles/op (466569 inserts, 1% deletions)
Running fast fio_map_s Test: 42.490 cycles/op (0.2 stdv, found 461903)
 ....... PASS

[[[ Avalanche Tests ]]]

Testing   24-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.585333%
Testing   32-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.695333%
Testing   40-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.619333%
Testing   48-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.662667%
Testing   56-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.706000%
Testing   64-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.693333%
Testing   72-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.730667%
Testing   80-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.706667%
Testing   96-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.672000%
Testing  112-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.786667%
Testing  128-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.621333%
Testing  160-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.710000%
Testing  512-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.835333%
Testing 1024-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.800000%

[[[ Keyset 'Sparse' Tests ]]]

Keyset 'Sparse' - 16-bit keys with up to 9 bits set - 50643 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (high 19-25 bits) - Worst is 19 bits: 2358/2368 (1.00x)
Testing collisions (low  32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (low  19-25 bits) - Worst is 24 bits: 90/76 (1.18x)
Testing distribution - Worst bias is the 13-bit window at bit 41 - 0.600%

Keyset 'Sparse' - 24-bit keys with up to 8 bits set - 1271626 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        188.2, actual    179 (0.95x)
Testing collisions (high 24-35 bits) - Worst is 25 bits: 23889/23794 (1.00x)
Testing collisions (low  32-bit) - Expected        188.2, actual    215 (1.14x) (27)
Testing collisions (low  24-35 bits) - Worst is 33 bits: 108/94 (1.15x)
Testing distribution - Worst bias is the 17-bit window at bit 16 - 0.075%

Keyset 'Sparse' - 32-bit keys with up to 7 bits set - 4514873 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2372.2, actual   2390 (1.01x) (18)
Testing collisions (high 25-38 bits) - Worst is 34 bits: 602/593 (1.01x)
Testing collisions (low  32-bit) - Expected       2372.2, actual   2313 (0.98x)
Testing collisions (low  25-38 bits) - Worst is 35 bits: 301/296 (1.01x)
Testing distribution - Worst bias is the 19-bit window at bit 15 - 0.051%

Keyset 'Sparse' - 40-bit keys with up to 6 bits set - 4598479 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2460.8, actual   2509 (1.02x) (49)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 42/38 (1.09x)
Testing collisions (low  32-bit) - Expected       2460.8, actual   2449 (1.00x) (-11)
Testing collisions (low  25-38 bits) - Worst is 35 bits: 316/307 (1.03x)
Testing distribution - Worst bias is the 19-bit window at bit 12 - 0.058%

Keyset 'Sparse' - 48-bit keys with up to 6 bits set - 14196869 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      23437.8, actual  23532 (1.00x) (95)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 28/22 (1.22x)
Testing collisions (low  32-bit) - Expected      23437.8, actual  23333 (1.00x) (-104)
Testing collisions (low  27-42 bits) - Worst is 40 bits: 96/91 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 16 - 0.020%

Keyset 'Sparse' - 56-bit keys with up to 5 bits set - 4216423 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2069.0, actual   2026 (0.98x)
Testing collisions (high 25-38 bits) - Worst is 34 bits: 518/517 (1.00x)
Testing collisions (low  32-bit) - Expected       2069.0, actual   2139 (1.03x) (71)
Testing collisions (low  25-38 bits) - Worst is 36 bits: 142/129 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit  8 - 0.043%

Keyset 'Sparse' - 64-bit keys with up to 5 bits set - 8303633 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8021.7, actual   8072 (1.01x) (51)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 37/31 (1.18x)
Testing collisions (low  32-bit) - Expected       8021.7, actual   8035 (1.00x) (14)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 32/31 (1.02x)
Testing distribution - Worst bias is the 20-bit window at bit 37 - 0.039%

Keyset 'Sparse' - 72-bit keys with up to 5 bits set - 15082603 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      26451.8, actual  26268 (0.99x) (-183)
Testing collisions (high 27-42 bits) - Worst is 39 bits: 225/206 (1.09x)
Testing collisions (low  32-bit) - Expected      26451.8, actual  26552 (1.00x) (101)
Testing collisions (low  27-42 bits) - Worst is 35 bits: 3381/3309 (1.02x)
Testing distribution - Worst bias is the 20-bit window at bit 36 - 0.027%

Keyset 'Sparse' - 96-bit keys with up to 4 bits set - 3469497 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1401.0, actual   1392 (0.99x) (-8)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 93/87 (1.06x)
Testing collisions (low  32-bit) - Expected       1401.0, actual   1402 (1.00x) (2)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 48/43 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit 11 - 0.051%

Keyset 'Sparse' - 160-bit keys with up to 4 bits set - 26977161 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      84546.1, actual  84403 (1.00x) (-143)
Testing collisions (high 28-44 bits) - Worst is 34 bits: 21303/21169 (1.01x)
Testing collisions (low  32-bit) - Expected      84546.1, actual  84679 (1.00x) (133)
Testing collisions (low  28-44 bits) - Worst is 43 bits: 45/41 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 49 - 0.014%

Keyset 'Sparse' - 256-bit keys with up to 3 bits set - 2796417 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        910.2, actual    930 (1.02x) (20)
Testing collisions (high 25-37 bits) - Worst is 36 bits: 71/56 (1.25x)
Testing collisions (low  32-bit) - Expected        910.2, actual    922 (1.01x) (12)
Testing collisions (low  25-37 bits) - Worst is 35 bits: 121/113 (1.06x)
Testing distribution - Worst bias is the 19-bit window at bit 52 - 0.094%

Keyset 'Sparse' - 512-bit keys with up to 3 bits set - 22370049 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      58155.4, actual  58001 (1.00x) (-154)
Testing collisions (high 28-43 bits) - Worst is 42 bits: 69/56 (1.21x)
Testing collisions (low  32-bit) - Expected      58155.4, actual  58151 (1.00x) (-4)
Testing collisions (low  28-43 bits) - Worst is 41 bits: 121/113 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit  3 - 0.015%

Keyset 'Sparse' - 1024-bit keys with up to 2 bits set - 524801 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         32.1, actual     37 (1.15x) (5)
Testing collisions (high 22-32 bits) - Worst is 31 bits: 75/64 (1.17x)
Testing collisions (low  32-bit) - Expected         32.1, actual     40 (1.25x) (8)
Testing collisions (low  22-32 bits) - Worst is 32 bits: 40/32 (1.25x)
Testing distribution - Worst bias is the 16-bit window at bit 17 - 0.136%

Keyset 'Sparse' - 2048-bit keys with up to 2 bits set - 2098177 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        512.4, actual    551 (1.08x) (39)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1105/1024 (1.08x)
Testing collisions (low  32-bit) - Expected        512.4, actual    526 (1.03x) (14)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/64 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 52 - 0.103%


[[[ Keyset 'Permutation' Tests ]]]

Combination Lowbits Tests:
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    670 (1.00x) (2)
Testing collisions (high 24-37 bits) - Worst is 37 bits: 28/20 (1.34x)
Testing collisions (low  32-bit) - Expected        668.6, actual    675 (1.01x) (7)
Testing collisions (low  24-37 bits) - Worst is 33 bits: 350/334 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit  1 - 0.086%


Combination Highbits Tests
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    620 (0.93x)
Testing collisions (high 24-37 bits) - Worst is 36 bits: 45/41 (1.08x)
Testing collisions (low  32-bit) - Expected        668.6, actual    634 (0.95x)
Testing collisions (low  24-37 bits) - Worst is 28 bits: 10778/10667 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.053%


Combination Hi-Lo Tests:
Keyset 'Combination' - up to 6 blocks from a set of 15 - 12204240 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      17322.9, actual  17547 (1.01x) (225)
Testing collisions (high 27-41 bits) - Worst is 38 bits: 283/270 (1.04x)
Testing collisions (low  32-bit) - Expected      17322.9, actual  17096 (0.99x) (-226)
Testing collisions (low  27-41 bits) - Worst is 41 bits: 37/33 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 46 - 0.035%


Combination 0x8000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8170 (1.00x) (-16)
Testing collisions (high 26-40 bits) - Worst is 33 bits: 4129/4094 (1.01x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8208 (1.00x) (22)
Testing collisions (low  26-40 bits) - Worst is 33 bits: 4128/4094 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit 21 - 0.041%


Combination 0x0000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8179 (1.00x) (-7)
Testing collisions (high 26-40 bits) - Worst is 31 bits: 16584/16362 (1.01x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8243 (1.01x) (57)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 270/255 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit  5 - 0.042%


Combination 0x800000000000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8296 (1.01x) (110)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 74/63 (1.16x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8168 (1.00x) (-18)
Testing collisions (low  26-40 bits) - Worst is 38 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 20-bit window at bit 55 - 0.035%


Combination 0x000000000000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8186 (1.00x)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 72/63 (1.13x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8238 (1.01x) (52)
Testing collisions (low  26-40 bits) - Worst is 35 bits: 1063/1023 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 63 - 0.031%


Combination 16-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8212 (1.00x) (26)
Testing collisions (high 26-40 bits) - Worst is 34 bits: 2056/2047 (1.00x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8083 (0.99x) (-103)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 259/255 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit 46 - 0.037%


Combination 16-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8052 (0.98x) (-134)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 66/63 (1.03x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8267 (1.01x) (81)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 20-bit window at bit  2 - 0.045%


Combination 32-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8136 (0.99x) (-50)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 130/127 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8138 (0.99x) (-48)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit 58 - 0.044%


Combination 32-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   7961 (0.97x)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8088 (0.99x) (-98)
Testing collisions (low  26-40 bits) - Worst is 35 bits: 1038/1023 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit 24 - 0.047%


Combination 64-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8054 (0.98x) (-132)
Testing collisions (high 26-40 bits) - Worst is 26 bits: 502359/503108 (1.00x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8309 (1.01x) (123)
Testing collisions (low  26-40 bits) - Worst is 34 bits: 2101/2047 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 14 - 0.052%


Combination 64-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8247 (1.01x) (61)
Testing collisions (high 26-40 bits) - Worst is 33 bits: 4159/4094 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8217 (1.00x) (31)
Testing collisions (low  26-40 bits) - Worst is 39 bits: 71/63 (1.11x)
Testing distribution - Worst bias is the 20-bit window at bit 31 - 0.052%


Combination 128-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8076 (0.99x) (-110)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 150/127 (1.17x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8278 (1.01x) (92)
Testing collisions (low  26-40 bits) - Worst is 35 bits: 1069/1023 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 36 - 0.041%


Combination 128-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8108 (0.99x) (-78)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8100 (0.99x) (-86)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 295/255 (1.15x)
Testing distribution - Worst bias is the 20-bit window at bit 55 - 0.049%


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
Testing collisions (high 32-bit) - Expected        116.4, actual    116 (1.00x)
Testing collisions (high 23-34 bits) - Worst is 30 bits: 499/465 (1.07x)
Testing collisions (low  32-bit) - Expected        116.4, actual    102 (0.88x)
Testing collisions (low  23-34 bits) - Worst is 25 bits: 14836/14754 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 24 - 0.118%

Keyset 'Cyclic' - 8 cycles of 9 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual     96 (0.82x)
Testing collisions (high 23-34 bits) - Worst is 26 bits: 7398/7413 (1.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    116 (1.00x)
Testing collisions (low  23-34 bits) - Worst is 25 bits: 14917/14754 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 46 - 0.104%

Keyset 'Cyclic' - 8 cycles of 10 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    102 (0.88x)
Testing collisions (high 23-34 bits) - Worst is 29 bits: 934/930 (1.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 35/29 (1.20x)
Testing distribution - Worst bias is the 17-bit window at bit 39 - 0.070%

Keyset 'Cyclic' - 8 cycles of 11 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    112 (0.96x)
Testing collisions (high 23-34 bits) - Worst is 27 bits: 3790/3716 (1.02x)
Testing collisions (low  32-bit) - Expected        116.4, actual    140 (1.20x) (24)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 17-bit window at bit 39 - 0.112%

Keyset 'Cyclic' - 8 cycles of 12 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing collisions (low  32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (low  23-34 bits) - Worst is 30 bits: 483/465 (1.04x)
Testing distribution - Worst bias is the 17-bit window at bit 52 - 0.132%

Keyset 'Cyclic' - 8 cycles of 16 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    132 (1.13x) (16)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing collisions (low  32-bit) - Expected        116.4, actual    100 (0.86x)
Testing collisions (low  23-34 bits) - Worst is 29 bits: 947/930 (1.02x)
Testing distribution - Worst bias is the 17-bit window at bit 15 - 0.109%


[[[ Keyset 'TwoBytes' Tests ]]]

Keyset 'TwoBytes' - up-to-4-byte keys, 652545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         49.6, actual     38 (0.77x)
Testing collisions (high 23-33 bits) - Worst is 23 bits: 24681/24735 (1.00x)
Testing collisions (low  32-bit) - Expected         49.6, actual     44 (0.89x)
Testing collisions (low  23-33 bits) - Worst is 31 bits: 108/99 (1.09x)
Testing distribution - Worst bias is the 16-bit window at bit 22 - 0.120%

Keyset 'TwoBytes' - up-to-8-byte keys, 5471025 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       3483.1, actual   3447 (0.99x) (-36)
Testing collisions (high 26-39 bits) - Worst is 26 bits: 217954/217072 (1.00x)
Testing collisions (low  32-bit) - Expected       3483.1, actual   3344 (0.96x)
Testing collisions (low  26-39 bits) - Worst is 38 bits: 57/54 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 10 - 0.087%

Keyset 'TwoBytes' - up-to-12-byte keys, 18616785 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      40289.5, actual  40269 (1.00x) (-20)
Testing collisions (high 27-42 bits) - Worst is 38 bits: 644/630 (1.02x)
Testing collisions (low  32-bit) - Expected      40289.5, actual  40110 (1.00x) (-179)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 41/39 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 49 - 0.024%

Keyset 'TwoBytes' - up-to-16-byte keys, 44251425 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     227182.3, actual 227638 (1.00x) (456)
Testing collisions (high 29-45 bits) - Worst is 39 bits: 1794/1780 (1.01x)
Testing collisions (low  32-bit) - Expected     227182.3, actual 226966 (1.00x) (-216)
Testing collisions (low  29-45 bits) - Worst is 42 bits: 253/222 (1.14x)
Testing distribution - Worst bias is the 20-bit window at bit 46 - 0.009%

Keyset 'TwoBytes' - up-to-20-byte keys, 86536545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     865959.1, actual 866176 (1.00x) (217)
Testing collisions (high 30-47 bits) - Worst is 43 bits: 437/425 (1.03x)
Testing collisions (low  32-bit) - Expected     865959.1, actual 865959 (1.00x)
Testing collisions (low  30-47 bits) - Worst is 45 bits: 121/106 (1.14x)
Testing distribution - Worst bias is the 20-bit window at bit 45 - 0.004%


[[[ Keyset 'Text' Tests ]]]

Keyset 'Text' - keys of form "FooXXXXBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25652 (1.01x) (263)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 31/24 (1.25x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25380 (1.00x) (-9)
Testing collisions (low  27-42 bits) - Worst is 38 bits: 412/397 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 12 - 0.022%

Keyset 'Text' - keys of form "FooBarXXXX" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25507 (1.00x) (118)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 29/24 (1.17x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25496 (1.00x) (107)
Testing collisions (low  27-42 bits) - Worst is 39 bits: 204/198 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 61 - 0.021%

Keyset 'Text' - keys of form "XXXXFooBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25559 (1.01x) (170)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 60/49 (1.21x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25471 (1.00x) (82)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 35/24 (1.41x)
Testing distribution - Worst bias is the 20-bit window at bit 63 - 0.033%

Keyset 'Words' - 4000000 random keys of len 6-16 from alnum charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1822 (0.98x)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 30/29 (1.03x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1837 (0.99x) (-25)
Testing collisions (low  25-38 bits) - Worst is 25 bits: 229258/229220 (1.00x)
Testing distribution - Worst bias is the 19-bit window at bit 42 - 0.071%

Keyset 'Words' - 4000000 random keys of len 6-16 from password charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1827 (0.98x) (-35)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 131/116 (1.13x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1832 (0.98x) (-30)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 32/29 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit 24 - 0.069%

Keyset 'Words' - 466569 dict words
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         25.3, actual     27 (1.07x) (2)
Testing collisions (high 22-32 bits) - Worst is 32 bits: 27/25 (1.07x)
Testing collisions (low  32-bit) - Expected         25.3, actual     26 (1.03x) (1)
Testing collisions (low  22-32 bits) - Worst is 30 bits: 120/101 (1.18x)
Testing distribution - Worst bias is the 16-bit window at bit 48 - 0.155%


[[[ Keyset 'Zeroes' Tests ]]]

Keyset 'Zeroes' - 204800 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          4.9, actual      4 (0.82x)
Testing collisions (high 21-29 bits) - Worst is 28 bits: 83/78 (1.06x)
Testing collisions (low  32-bit) - Expected          4.9, actual      2 (0.41x)
Testing collisions (low  21-29 bits) - Worst is 27 bits: 169/156 (1.08x)
Testing distribution - Worst bias is the 15-bit window at bit 31 - 0.261%


[[[ Keyset 'Seed' Tests ]]]

Keyset 'Seed' - 5000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2909.3, actual   2808 (0.97x)
Testing collisions (high 26-39 bits) - Worst is 37 bits: 93/90 (1.02x)
Testing collisions (low  32-bit) - Expected       2909.3, actual   3032 (1.04x) (123)
Testing collisions (low  26-39 bits) - Worst is 38 bits: 64/45 (1.41x)
Testing distribution - Worst bias is the 19-bit window at bit 63 - 0.044%


[[[ Keyset 'PerlinNoise' Tests ]]]

Testing 16777216 coordinates (L2) : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      32725.4, actual  32777 (1.00x) (52)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 37/31 (1.16x)
Testing collisions (low  32-bit) - Expected      32725.4, actual  32511 (0.99x) (-214)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 36/31 (1.13x)

Testing AV variant, 128 count with 4 spacing, 4-12:
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1116.2, actual   1087 (0.97x)
Testing collisions (high 25-37 bits) - Worst is 27 bits: 35400/35452 (1.00x)
Testing collisions (low  32-bit) - Expected       1116.2, actual   1067 (0.96x)
Testing collisions (low  25-37 bits) - Worst is 34 bits: 280/279 (1.00x)


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
Testing collisions (high 32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit  3 - 0.118%

Testing bit 1
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64220/64191 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 60 - 0.077%

Testing bit 2
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.078%

Testing bit 3
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1083/1023 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    470 (0.92x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2093/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.069%

Testing bit 4
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8315/8170 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    549 (1.07x) (38)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 52 - 0.081%

Testing bit 5
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing distribution - Worst bias is the 18-bit window at bit 40 - 0.116%

Testing bit 6
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing distribution - Worst bias is the 18-bit window at bit 14 - 0.106%

Testing bit 7
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    553 (1.08x) (42)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.095%

Testing bit 8
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 513/511 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.075%

Testing bit 9
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.113%

Testing bit 10
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32540/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 40 - 0.099%

Testing bit 11
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32534/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.059%

Testing bit 12
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 136/127 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 46/31 (1.44x)
Testing distribution - Worst bias is the 18-bit window at bit 10 - 0.058%

Testing bit 13
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8316/8170 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    553 (1.08x) (42)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.054%

Testing bit 14
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 43 - 0.088%

Testing bit 15
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 281/255 (1.10x)
Testing collisions (low  32-bit) - Expected        511.9, actual    555 (1.08x) (44)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.079%

Testing bit 16
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2084/2046 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 520/511 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 58 - 0.076%

Testing bit 17
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 260/255 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 138/127 (1.08x)
Testing distribution - Worst bias is the 17-bit window at bit 54 - 0.068%

Testing bit 18
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2114/2046 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.083%

Testing bit 19
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 529/511 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 27 bits: 16419/16298 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 29 - 0.080%

Testing bit 20
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1040/1023 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.092%

Testing bit 21
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1062/1023 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 34 - 0.079%

Testing bit 22
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 260/255 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    466 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 47 - 0.077%

Testing bit 23
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32370/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.064%

Testing bit 24
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 272/255 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 27 bits: 16412/16298 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 46 - 0.057%

Testing bit 25
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 269/255 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    549 (1.07x) (38)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 549/511 (1.07x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.089%

Testing bit 26
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1036/1023 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    481 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit  1 - 0.065%

Testing bit 27
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    465 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 13 - 0.101%

Testing bit 28
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    484 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 25 bits: 64180/64191 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    536 (1.05x) (25)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit  8 - 0.130%

Testing bit 29
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 126329/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1036/1023 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit  3 - 0.059%

Testing bit 30
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2107/2046 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    478 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64338/64191 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit  4 - 0.078%

Testing bit 31
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    493 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125635/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.072%

Testing bit 32
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125444/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4149/4090 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 12 - 0.094%

Testing bit 33
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 546/511 (1.07x)
Testing collisions (low  32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 139/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 45 - 0.069%

Testing bit 34
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing distribution - Worst bias is the 18-bit window at bit 52 - 0.133%

Testing bit 35
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 17-bit window at bit 31 - 0.062%

Testing bit 36
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 519/511 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2121/2046 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 37 - 0.119%

Testing bit 37
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing collisions (low  32-bit) - Expected        511.9, actual    465 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4123/4090 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.082%

Testing bit 38
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    542 (1.06x) (31)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1065/1023 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.058%

Testing bit 39
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    523 (1.02x) (12)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1056/1023 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    479 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125933/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 11 - 0.066%

Testing bit 40
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 47/31 (1.47x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 134/127 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.109%

Testing bit 41
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    474 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16336/16298 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1038/1023 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.067%

Testing bit 42
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 137/127 (1.07x)
Testing collisions (low  32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit  5 - 0.079%

Testing bit 43
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 38 - 0.089%

Testing bit 44
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing collisions (low  32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing distribution - Worst bias is the 18-bit window at bit 51 - 0.073%

Testing bit 45
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 529/511 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 276/255 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 47 - 0.092%

Testing bit 46
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 143/127 (1.12x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4172/4090 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit  9 - 0.073%

Testing bit 47
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 263/255 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    560 (1.09x) (49)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 24 - 0.073%

Testing bit 48
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4230/4090 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.069%

Testing bit 49
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 265/255 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 49 - 0.077%

Testing bit 50
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 53 - 0.088%

Testing bit 51
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    461 (0.90x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2065/2046 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2045/2046 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit  2 - 0.073%

Testing bit 52
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 527/511 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.083%

Testing bit 53
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 126008/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 18 - 0.083%

Testing bit 54
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 146/127 (1.14x)
Testing collisions (low  32-bit) - Expected        511.9, actual    544 (1.06x) (33)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 544/511 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 49 - 0.052%

Testing bit 55
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    454 (0.89x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125990/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8220/8170 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 21 - 0.108%

Testing bit 56
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    561 (1.10x) (50)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 148/127 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 526/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.067%

Testing bit 57
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    468 (0.91x)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32528/32429 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 46 - 0.111%

Testing bit 58
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 48/31 (1.50x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 264/255 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.080%

Testing bit 59
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2078/2046 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2113/2046 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.088%

Testing bit 60
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    465 (0.91x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8183/8170 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 38 - 0.067%

Testing bit 61
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 68/63 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.075%

Testing bit 62
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    572 (1.12x) (61)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 304/255 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.068%

Testing bit 63
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 529/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.089%


[[[ MomentChi2 Tests ]]]

Analyze hashes produced from a serie of linearly increasing numbers of 32-bit, using a step of 2 ... 
Target values to approximate : 38918200.000000 - 273633.333333 
Popcount 1 stats : 38919570.005124 - 273658.703091
Popcount 0 stats : 38918319.622388 - 273636.624134
MomentChi2 for bits 1 :   3.42946 
MomentChi2 for bits 0 :  0.0261471 

Derivative stats (transition from 2 consecutive values) : 
Popcount 1 stats : 38919286.374871 - 273651.807467
Popcount 0 stats : 38918700.222160 - 273641.544934
MomentChi2 for deriv b1 :   2.15648 
MomentChi2 for deriv b0 :  0.457215 

  Great 


[[[ Prng Tests ]]]

Generating 33554432 random numbers : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     130731.3, actual 130706 (1.00x) (-25)
Testing collisions (high 28-44 bits) - Worst is 44 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected     130731.3, actual 130394 (1.00x) (-337)
Testing collisions (low  28-44 bits) - Worst is 41 bits: 261/255 (1.02x)

[[[ BadSeeds Tests ]]]

0x0 PASS


Input vcode 0x00000001, Output vcode 0x00000001, Result vcode 0x00000001
Verification value is 0x00000001 - Testing took 685.179970 seconds
-------------------------------------------------------------------------------
```

-------------------------------------------------------------------------------
