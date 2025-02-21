### `FIO_DEFINE_RANDOM128_FN`

The following are the tests for the built-in `FIO_DEFINE_RANDOM128_FN` macro using the deterministic PRNG (where the auto-reseeding `reseed_log` is set to `0`).

**Note**: setting `reseed_log` to any practical value (such as `31`) would improve randomness make the PRNG non-deterministic, improving the security of the result.

**The `PractRand` results**:

```txt
# ./tmp/rnd -p M128 | RNG_test stdin
RNG_test using PractRand version 0.95
RNG = RNG_stdin, seed = unknown
test set = core, folding = standard(unknown format)

rng=RNG_stdin, seed=unknown
length= 256 megabytes (2^28 bytes), time= 2.4 seconds
  no anomalies in 217 test result(s)

rng=RNG_stdin, seed=unknown
length= 512 megabytes (2^29 bytes), time= 5.0 seconds
  no anomalies in 232 test result(s)

rng=RNG_stdin, seed=unknown
length= 1 gigabyte (2^30 bytes), time= 10.1 seconds
  no anomalies in 251 test result(s)

rng=RNG_stdin, seed=unknown
length= 2 gigabytes (2^31 bytes), time= 20.6 seconds
  no anomalies in 269 test result(s)

rng=RNG_stdin, seed=unknown
length= 4 gigabytes (2^32 bytes), time= 41.2 seconds
  no anomalies in 283 test result(s)

rng=RNG_stdin, seed=unknown
length= 8 gigabytes (2^33 bytes), time= 83.9 seconds
  no anomalies in 300 test result(s)

rng=RNG_stdin, seed=unknown
length= 16 gigabytes (2^34 bytes), time= 167 seconds
  no anomalies in 315 test result(s)

rng=RNG_stdin, seed=unknown
length= 32 gigabytes (2^35 bytes), time= 330 seconds
  no anomalies in 328 test result(s)

rng=RNG_stdin, seed=unknown
length= 64 gigabytes (2^36 bytes), time= 665 seconds
  no anomalies in 344 test result(s)

rng=RNG_stdin, seed=unknown
length= 128 gigabytes (2^37 bytes), time= 1329 seconds
  no anomalies in 359 test result(s)

rng=RNG_stdin, seed=unknown
length= 256 gigabytes (2^38 bytes), time= 2630 seconds
  no anomalies in 372 test result(s)

rng=RNG_stdin, seed=unknown
length= 512 gigabytes (2^39 bytes), time= 6846 seconds
  no anomalies in 387 test result(s)

rng=RNG_stdin, seed=unknown
length= 1 terabyte (2^40 bytes), time= 18633 seconds
  no anomalies in 401 test result(s)

rng=RNG_stdin, seed=unknown
length= 2 terabytes (2^41 bytes), time= 39061 seconds
  no anomalies in 413 test result(s)
```

**The tests adopted from the `xoshiro` code base**:


```txt
# ./tmp/random m128
mix3 extreme = 1.90403 (sig = 00020000) weight 1 (16), p-value = 0.608
mix3 extreme = 2.78422 (sig = 01000001) weight 2 (112), p-value = 0.453
mix3 extreme = 2.98802 (sig = 00001220) weight 3 (448), p-value = 0.716
mix3 extreme = 3.51263 (sig = 22110000) weight 4 (1120), p-value = 0.392
mix3 extreme = 3.86128 (sig = 22110200) weight >=5 (4864), p-value = 0.422
bits per word = 64 (analyzing bits); min category p-value = 0.392

processed 1.11e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.917
------

mix3 extreme = 2.33512 (sig = 00020000) weight 1 (16), p-value = 0.271
mix3 extreme = 2.60631 (sig = 00000011) weight 2 (112), p-value = 0.643
mix3 extreme = 3.20941 (sig = 00001220) weight 3 (448), p-value = 0.449
mix3 extreme = 3.23491 (sig = 10000221) weight 4 (1120), p-value = 0.744
mix3 extreme = 3.98181 (sig = 12101200) weight >=5 (4864), p-value = 0.283
bits per word = 64 (analyzing bits); min category p-value = 0.271

processed 1.29e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.794
------

mix3 extreme = 2.09795 (sig = 00020000) weight 1 (16), p-value = 0.443
mix3 extreme = 2.59859 (sig = 00000011) weight 2 (112), p-value = 0.651
mix3 extreme = 3.16919 (sig = 00020012) weight 3 (448), p-value = 0.496
mix3 extreme = 3.32522 (sig = 01002210) weight 4 (1120), p-value = 0.628
mix3 extreme = 3.87995 (sig = 20211102) weight >=5 (4864), p-value = 0.398
bits per word = 64 (analyzing bits); min category p-value = 0.398

processed 1.66e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.921
------

mix3 extreme = 1.97959 (sig = 00020000) weight 1 (16), p-value = 0.543
mix3 extreme = 2.47936 (sig = 00000011) weight 2 (112), p-value = 0.773
mix3 extreme = 3.25629 (sig = 02000022) weight 3 (448), p-value = 0.397
mix3 extreme = 3.20819 (sig = 20001012) weight 4 (1120), p-value = 0.776
mix3 extreme = 3.85604 (sig = 21111101) weight >=5 (4864), p-value = 0.429
bits per word = 64 (analyzing bits); min category p-value = 0.397

processed 1.85e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.92
------

mix3 extreme = 2.14920 (sig = 00020000) weight 1 (16), p-value = 0.402
mix3 extreme = 2.58549 (sig = 00000011) weight 2 (112), p-value = 0.665
mix3 extreme = 2.97421 (sig = 02000022) weight 3 (448), p-value = 0.732
mix3 extreme = 2.92783 (sig = 00112001) weight 4 (1120), p-value = 0.978
mix3 extreme = 3.95789 (sig = 22110200) weight >=5 (4864), p-value = 0.308
bits per word = 64 (analyzing bits); min category p-value = 0.308

processed 2.03e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.841
------

mix3 extreme = 1.47529 (sig = 00001000) weight 1 (16), p-value = 0.911
mix3 extreme = 2.73432 (sig = 10000100) weight 2 (112), p-value = 0.505
mix3 extreme = 2.85412 (sig = 00220100) weight 3 (448), p-value = 0.856
mix3 extreme = 3.71866 (sig = 20200011) weight 4 (1120), p-value = 0.201
mix3 extreme = 3.82703 (sig = 12212021) weight >=5 (4864), p-value = 0.468
bits per word = 64 (analyzing bits); min category p-value = 0.201

processed 2.59e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.674
------

mix3 extreme = 1.48360 (sig = 20000000) weight 1 (16), p-value = 0.907
mix3 extreme = 2.46121 (sig = 00002002) weight 2 (112), p-value = 0.79
mix3 extreme = 2.87026 (sig = 20000201) weight 3 (448), p-value = 0.841
mix3 extreme = 3.86069 (sig = 20200011) weight 4 (1120), p-value = 0.119
mix3 extreme = 3.76155 (sig = 21201200) weight >=5 (4864), p-value = 0.56
bits per word = 64 (analyzing bits); min category p-value = 0.119

processed 3.14e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.469
------

mix3 extreme = 1.84311 (sig = 20000000) weight 1 (16), p-value = 0.661
mix3 extreme = 2.57296 (sig = 10001000) weight 2 (112), p-value = 0.679
mix3 extreme = 3.19014 (sig = 20000201) weight 3 (448), p-value = 0.471
mix3 extreme = 3.98985 (sig = 20200011) weight 4 (1120), p-value = 0.0714
mix3 extreme = 3.85549 (sig = 22121010) weight >=5 (4864), p-value = 0.43
bits per word = 64 (analyzing bits); min category p-value = 0.0714

processed 4.07e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.309
------

mix3 extreme = 1.41079 (sig = 20000000) weight 1 (16), p-value = 0.937
mix3 extreme = 3.04738 (sig = 10001000) weight 2 (112), p-value = 0.228
mix3 extreme = 3.74043 (sig = 00220100) weight 3 (448), p-value = 0.079
mix3 extreme = 3.82980 (sig = 00101210) weight 4 (1120), p-value = 0.134
mix3 extreme = 3.95613 (sig = 12012120) weight >=5 (4864), p-value = 0.31
bits per word = 64 (analyzing bits); min category p-value = 0.079

processed 5.18e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.337
------

mix3 extreme = 1.46023 (sig = 00020000) weight 1 (16), p-value = 0.917
mix3 extreme = 2.75344 (sig = 00200100) weight 2 (112), p-value = 0.484
mix3 extreme = 3.35270 (sig = 20000201) weight 3 (448), p-value = 0.301
mix3 extreme = 3.74339 (sig = 02102100) weight 4 (1120), p-value = 0.184
mix3 extreme = 3.75002 (sig = 12012120) weight >=5 (4864), p-value = 0.577
bits per word = 64 (analyzing bits); min category p-value = 0.184

processed 6.1e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.638
------

mix3 extreme = 1.50237 (sig = 00000020) weight 1 (16), p-value = 0.898
mix3 extreme = 2.42220 (sig = 00200100) weight 2 (112), p-value = 0.825
mix3 extreme = 2.96916 (sig = 20000201) weight 3 (448), p-value = 0.738
mix3 extreme = 3.78954 (sig = 02102100) weight 4 (1120), p-value = 0.156
mix3 extreme = 3.66917 (sig = 10010121) weight >=5 (4864), p-value = 0.694
bits per word = 64 (analyzing bits); min category p-value = 0.156

processed 7.03e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.571
------

mix3 extreme = 1.78774 (sig = 00000020) weight 1 (16), p-value = 0.707
mix3 extreme = 2.70149 (sig = 00010100) weight 2 (112), p-value = 0.54
mix3 extreme = 3.27303 (sig = 01120000) weight 3 (448), p-value = 0.379
mix3 extreme = 4.21573 (sig = 02102100) weight 4 (1120), p-value = 0.0275
mix3 extreme = 3.70578 (sig = 12100210) weight >=5 (4864), p-value = 0.641
bits per word = 64 (analyzing bits); min category p-value = 0.0275

processed 8.5e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.13
------

mix3 extreme = 2.31948 (sig = 10000000) weight 1 (16), p-value = 0.281
mix3 extreme = 2.44934 (sig = 00010100) weight 2 (112), p-value = 0.801
mix3 extreme = 3.53130 (sig = 01120000) weight 3 (448), p-value = 0.169
mix3 extreme = 4.28717 (sig = 02102100) weight 4 (1120), p-value = 0.0201
mix3 extreme = 3.66277 (sig = 12100210) weight >=5 (4864), p-value = 0.703
bits per word = 64 (analyzing bits); min category p-value = 0.0201

processed 1.02e+09 bytes in 1 seconds (1.017 GB/s, 3.661 TB/h). Fri Feb 21 23:22:46 2025

p = 0.0964
------

mix3 extreme = 2.60876 (sig = 00000020) weight 1 (16), p-value = 0.136
mix3 extreme = 2.67150 (sig = 22000000) weight 2 (112), p-value = 0.572
mix3 extreme = 3.39462 (sig = 01120000) weight 3 (448), p-value = 0.265
mix3 extreme = 3.62549 (sig = 02200120) weight 4 (1120), p-value = 0.276
mix3 extreme = 3.63707 (sig = 12100021) weight >=5 (4864), p-value = 0.739
bits per word = 64 (analyzing bits); min category p-value = 0.136

processed 1.26e+09 bytes in 1 seconds (1.257 GB/s, 4.526 TB/h). Fri Feb 21 23:22:46 2025

p = 0.518
------

mix3 extreme = 2.15590 (sig = 00000020) weight 1 (16), p-value = 0.397
mix3 extreme = 2.80570 (sig = 00001010) weight 2 (112), p-value = 0.431
mix3 extreme = 3.40481 (sig = 02020001) weight 3 (448), p-value = 0.257
mix3 extreme = 3.36693 (sig = 01100012) weight 4 (1120), p-value = 0.573
mix3 extreme = 3.51384 (sig = 12012120) weight >=5 (4864), p-value = 0.883
bits per word = 64 (analyzing bits); min category p-value = 0.257

processed 1.52e+09 bytes in 1 seconds (1.516 GB/s, 5.458 TB/h). Fri Feb 21 23:22:46 2025

p = 0.773
------

mix3 extreme = 1.90047 (sig = 00000020) weight 1 (16), p-value = 0.611
mix3 extreme = 3.23402 (sig = 00001010) weight 2 (112), p-value = 0.128
mix3 extreme = 2.96881 (sig = 20001001) weight 3 (448), p-value = 0.739
mix3 extreme = 3.52470 (sig = 01012002) weight 4 (1120), p-value = 0.378
mix3 extreme = 3.72569 (sig = 22111101) weight >=5 (4864), p-value = 0.612
bits per word = 64 (analyzing bits); min category p-value = 0.128

processed 1.76e+09 bytes in 1 seconds (1.756 GB/s, 6.323 TB/h). Fri Feb 21 23:22:46 2025

p = 0.495
------

mix3 extreme = 1.57279 (sig = 00000010) weight 1 (16), p-value = 0.86
mix3 extreme = 2.81387 (sig = 00001010) weight 2 (112), p-value = 0.423
mix3 extreme = 3.18715 (sig = 02002010) weight 3 (448), p-value = 0.475
mix3 extreme = 3.33070 (sig = 00101120) weight 4 (1120), p-value = 0.621
mix3 extreme = 4.16820 (sig = 20122202) weight >=5 (4864), p-value = 0.139
bits per word = 64 (analyzing bits); min category p-value = 0.139

processed 2.02e+09 bytes in 1 seconds (2.015 GB/s, 7.255 TB/h). Fri Feb 21 23:22:46 2025

p = 0.526
------

mix3 extreme = 1.53157 (sig = 00000020) weight 1 (16), p-value = 0.883
mix3 extreme = 2.71599 (sig = 00001010) weight 2 (112), p-value = 0.524
mix3 extreme = 3.16021 (sig = 02002010) weight 3 (448), p-value = 0.507
mix3 extreme = 3.26824 (sig = 00101120) weight 4 (1120), p-value = 0.703
mix3 extreme = 4.01251 (sig = 22021002) weight >=5 (4864), p-value = 0.253
bits per word = 64 (analyzing bits); min category p-value = 0.253

processed 2.51e+09 bytes in 1 seconds (2.514 GB/s, 9.052 TB/h). Fri Feb 21 23:22:46 2025

p = 0.768
------

mix3 extreme = 1.74122 (sig = 00000020) weight 1 (16), p-value = 0.744
mix3 extreme = 2.86200 (sig = 00010100) weight 2 (112), p-value = 0.377
mix3 extreme = 3.41797 (sig = 02002010) weight 3 (448), p-value = 0.246
mix3 extreme = 3.48775 (sig = 00101120) weight 4 (1120), p-value = 0.421
mix3 extreme = 4.32378 (sig = 22021002) weight >=5 (4864), p-value = 0.0719
bits per word = 64 (analyzing bits); min category p-value = 0.0719

processed 3.01e+09 bytes in 1 seconds (3.014 GB/s, 10.85 TB/h). Fri Feb 21 23:22:46 2025

p = 0.311
------

mix3 extreme = 1.61472 (sig = 00000020) weight 1 (16), p-value = 0.835
mix3 extreme = 3.22143 (sig = 00010100) weight 2 (112), p-value = 0.133
mix3 extreme = 3.34281 (sig = 02002010) weight 3 (448), p-value = 0.31
mix3 extreme = 3.41424 (sig = 10011100) weight 4 (1120), p-value = 0.512
mix3 extreme = 3.68273 (sig = 00122011) weight >=5 (4864), p-value = 0.675
bits per word = 64 (analyzing bits); min category p-value = 0.133

processed 4.01e+09 bytes in 1 seconds (4.012 GB/s, 14.44 TB/h). Fri Feb 21 23:22:46 2025

p = 0.511
------

mix3 extreme = 2.08971 (sig = 00000020) weight 1 (16), p-value = 0.45
mix3 extreme = 3.03226 (sig = 00010100) weight 2 (112), p-value = 0.238
mix3 extreme = 3.22814 (sig = 00010102) weight 3 (448), p-value = 0.428
mix3 extreme = 3.27577 (sig = 10011100) weight 4 (1120), p-value = 0.693
mix3 extreme = 3.67641 (sig = 12012120) weight >=5 (4864), p-value = 0.684
bits per word = 64 (analyzing bits); min category p-value = 0.238

processed 5.01e+09 bytes in 2 seconds (2.505 GB/s, 9.019 TB/h). Fri Feb 21 23:22:47 2025

p = 0.744
------

mix3 extreme = 1.91894 (sig = 10000000) weight 1 (16), p-value = 0.595
mix3 extreme = 2.77625 (sig = 00010100) weight 2 (112), p-value = 0.461
mix3 extreme = 3.06382 (sig = 00011001) weight 3 (448), p-value = 0.625
mix3 extreme = 3.14390 (sig = 21020200) weight 4 (1120), p-value = 0.846
mix3 extreme = 3.59615 (sig = 00122011) weight >=5 (4864), p-value = 0.792
bits per word = 64 (analyzing bits); min category p-value = 0.461

processed 6.01e+09 bytes in 2 seconds (3.004 GB/s, 10.82 TB/h). Fri Feb 21 23:22:47 2025

p = 0.954
------

mix3 extreme = 1.86597 (sig = 00000020) weight 1 (16), p-value = 0.641
mix3 extreme = 2.61078 (sig = 00010100) weight 2 (112), p-value = 0.638
mix3 extreme = 3.42404 (sig = 00011001) weight 3 (448), p-value = 0.242
mix3 extreme = 3.40741 (sig = 00020122) weight 4 (1120), p-value = 0.52
mix3 extreme = 3.89941 (sig = 00122011) weight >=5 (4864), p-value = 0.374
bits per word = 64 (analyzing bits); min category p-value = 0.242

processed 7.01e+09 bytes in 2 seconds (3.504 GB/s, 12.61 TB/h). Fri Feb 21 23:22:47 2025

p = 0.749
------

mix3 extreme = 1.73399 (sig = 00000020) weight 1 (16), p-value = 0.75
mix3 extreme = 2.96016 (sig = 20000200) weight 2 (112), p-value = 0.292
mix3 extreme = 3.09543 (sig = 02000022) weight 3 (448), p-value = 0.586
mix3 extreme = 3.42588 (sig = 10101001) weight 4 (1120), p-value = 0.497
mix3 extreme = 3.66808 (sig = 10110201) weight >=5 (4864), p-value = 0.695
bits per word = 64 (analyzing bits); min category p-value = 0.292

processed 8.5e+09 bytes in 3 seconds (2.835 GB/s, 10.21 TB/h). Fri Feb 21 23:22:48 2025

p = 0.822
------

mix3 extreme = 1.52914 (sig = 00200000) weight 1 (16), p-value = 0.885
mix3 extreme = 3.81757 (sig = 20000200) weight 2 (112), p-value = 0.015
mix3 extreme = 3.07953 (sig = 02201000) weight 3 (448), p-value = 0.605
mix3 extreme = 3.26619 (sig = 01102001) weight 4 (1120), p-value = 0.705
mix3 extreme = 3.73963 (sig = 02122020) weight >=5 (4864), p-value = 0.592
bits per word = 64 (analyzing bits); min category p-value = 0.015

processed 1e+10 bytes in 3 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:22:48 2025

p = 0.0727
------

mix3 extreme = 2.00898 (sig = 00020000) weight 1 (16), p-value = 0.518
mix3 extreme = 2.73872 (sig = 20000200) weight 2 (112), p-value = 0.5
mix3 extreme = 2.89865 (sig = 22000200) weight 3 (448), p-value = 0.814
mix3 extreme = 3.51139 (sig = 10002220) weight 4 (1120), p-value = 0.393
mix3 extreme = 3.46747 (sig = 21020120) weight >=5 (4864), p-value = 0.922
bits per word = 64 (analyzing bits); min category p-value = 0.393

processed 1.25e+10 bytes in 4 seconds (3.129 GB/s, 11.27 TB/h). Fri Feb 21 23:22:49 2025

p = 0.918
------

mix3 extreme = 2.54743 (sig = 00020000) weight 1 (16), p-value = 0.16
mix3 extreme = 2.66976 (sig = 00020200) weight 2 (112), p-value = 0.574
mix3 extreme = 3.21302 (sig = 02020010) weight 3 (448), p-value = 0.445
mix3 extreme = 3.17616 (sig = 10002220) weight 4 (1120), p-value = 0.812
mix3 extreme = 4.08852 (sig = 00122011) weight >=5 (4864), p-value = 0.19
bits per word = 64 (analyzing bits); min category p-value = 0.16

processed 1.5e+10 bytes in 5 seconds (3.003 GB/s, 10.81 TB/h). Fri Feb 21 23:22:50 2025

p = 0.582
------

mix3 extreme = 2.32339 (sig = 00020000) weight 1 (16), p-value = 0.278
mix3 extreme = 2.41362 (sig = 00001010) weight 2 (112), p-value = 0.832
mix3 extreme = 3.50336 (sig = 02020010) weight 3 (448), p-value = 0.186
mix3 extreme = 3.13576 (sig = 20221000) weight 4 (1120), p-value = 0.854
mix3 extreme = 4.28795 (sig = 00122011) weight >=5 (4864), p-value = 0.084
bits per word = 64 (analyzing bits); min category p-value = 0.084

processed 1.75e+10 bytes in 5 seconds (3.502 GB/s, 12.61 TB/h). Fri Feb 21 23:22:50 2025

p = 0.355
------

mix3 extreme = 2.55481 (sig = 00020000) weight 1 (16), p-value = 0.157
mix3 extreme = 2.72892 (sig = 00001010) weight 2 (112), p-value = 0.51
mix3 extreme = 3.28466 (sig = 11000001) weight 3 (448), p-value = 0.367
mix3 extreme = 3.42088 (sig = 01020102) weight 4 (1120), p-value = 0.503
mix3 extreme = 4.05525 (sig = 00122011) weight >=5 (4864), p-value = 0.216
bits per word = 64 (analyzing bits); min category p-value = 0.157

processed 2e+10 bytes in 6 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:22:51 2025

p = 0.575
------

mix3 extreme = 2.46488 (sig = 00020000) weight 1 (16), p-value = 0.198
mix3 extreme = 2.44365 (sig = 20000200) weight 2 (112), p-value = 0.806
mix3 extreme = 3.31277 (sig = 02020010) weight 3 (448), p-value = 0.339
mix3 extreme = 3.81631 (sig = 01020102) weight 4 (1120), p-value = 0.141
mix3 extreme = 4.12544 (sig = 00122011) weight >=5 (4864), p-value = 0.165
bits per word = 64 (analyzing bits); min category p-value = 0.141

processed 2.5e+10 bytes in 8 seconds (3.127 GB/s, 11.26 TB/h). Fri Feb 21 23:22:53 2025

p = 0.532
------

mix3 extreme = 2.18715 (sig = 00020000) weight 1 (16), p-value = 0.373
mix3 extreme = 2.10006 (sig = 20000020) weight 2 (112), p-value = 0.983
mix3 extreme = 3.08063 (sig = 00210002) weight 3 (448), p-value = 0.604
mix3 extreme = 3.66924 (sig = 02110010) weight 4 (1120), p-value = 0.239
mix3 extreme = 4.35154 (sig = 00122011) weight >=5 (4864), p-value = 0.0636
bits per word = 64 (analyzing bits); min category p-value = 0.0636

processed 3e+10 bytes in 9 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:22:54 2025

p = 0.28
------

mix3 extreme = 1.43572 (sig = 00020000) weight 1 (16), p-value = 0.927
mix3 extreme = 2.41760 (sig = 02000020) weight 2 (112), p-value = 0.829
mix3 extreme = 3.16026 (sig = 02100020) weight 3 (448), p-value = 0.507
mix3 extreme = 3.44650 (sig = 02220002) weight 4 (1120), p-value = 0.471
mix3 extreme = 4.35459 (sig = 00221220) weight >=5 (4864), p-value = 0.0628
bits per word = 64 (analyzing bits); min category p-value = 0.0628

processed 4e+10 bytes in 12 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:22:57 2025

p = 0.277
------

mix3 extreme = 1.73103 (sig = 02000000) weight 1 (16), p-value = 0.752
mix3 extreme = 2.59948 (sig = 02000020) weight 2 (112), p-value = 0.65
mix3 extreme = 3.24793 (sig = 02020010) weight 3 (448), p-value = 0.406
mix3 extreme = 3.18550 (sig = 00022021) weight 4 (1120), p-value = 0.802
mix3 extreme = 4.36307 (sig = 00122011) weight >=5 (4864), p-value = 0.0605
bits per word = 64 (analyzing bits); min category p-value = 0.0605

processed 5e+10 bytes in 15 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:23:00 2025

p = 0.268
------

mix3 extreme = 1.69245 (sig = 02000000) weight 1 (16), p-value = 0.781
mix3 extreme = 2.90012 (sig = 02000020) weight 2 (112), p-value = 0.342
mix3 extreme = 3.04010 (sig = 02100020) weight 3 (448), p-value = 0.654
mix3 extreme = 3.18726 (sig = 11012000) weight 4 (1120), p-value = 0.8
mix3 extreme = 4.13605 (sig = 00122011) weight >=5 (4864), p-value = 0.158
bits per word = 64 (analyzing bits); min category p-value = 0.158

processed 6e+10 bytes in 18 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:23:03 2025

p = 0.577
------

mix3 extreme = 2.02538 (sig = 02000000) weight 1 (16), p-value = 0.504
mix3 extreme = 2.69836 (sig = 20000020) weight 2 (112), p-value = 0.543
mix3 extreme = 2.74898 (sig = 00200110) weight 3 (448), p-value = 0.932
mix3 extreme = 3.09844 (sig = 11012000) weight 4 (1120), p-value = 0.887
mix3 extreme = 4.04079 (sig = 20022201) weight >=5 (4864), p-value = 0.228
bits per word = 64 (analyzing bits); min category p-value = 0.228

processed 7e+10 bytes in 21 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:23:06 2025

p = 0.726
------

mix3 extreme = 2.00762 (sig = 00020000) weight 1 (16), p-value = 0.519
mix3 extreme = 2.67485 (sig = 20000020) weight 2 (112), p-value = 0.569
mix3 extreme = 2.98340 (sig = 00110100) weight 3 (448), p-value = 0.722
mix3 extreme = 3.30873 (sig = 12210000) weight 4 (1120), p-value = 0.65
mix3 extreme = 4.04171 (sig = 20022201) weight >=5 (4864), p-value = 0.227
bits per word = 64 (analyzing bits); min category p-value = 0.227

processed 8.5e+10 bytes in 25 seconds (3.4 GB/s, 12.24 TB/h). Fri Feb 21 23:23:10 2025

p = 0.725
------

mix3 extreme = 1.98503 (sig = 00100000) weight 1 (16), p-value = 0.538
mix3 extreme = 2.83929 (sig = 20000020) weight 2 (112), p-value = 0.398
mix3 extreme = 3.24294 (sig = 00110100) weight 3 (448), p-value = 0.412
mix3 extreme = 3.26307 (sig = 00022012) weight 4 (1120), p-value = 0.709
mix3 extreme = 3.82851 (sig = 20212120) weight >=5 (4864), p-value = 0.466
bits per word = 64 (analyzing bits); min category p-value = 0.398

processed 1e+11 bytes in 30 seconds (3.333 GB/s, 12 TB/h). Fri Feb 21 23:23:15 2025

p = 0.921
------

mix3 extreme = 1.80847 (sig = 00000001) weight 1 (16), p-value = 0.69
mix3 extreme = 2.36679 (sig = 00001020) weight 2 (112), p-value = 0.868
mix3 extreme = 3.12962 (sig = 00110100) weight 3 (448), p-value = 0.544
mix3 extreme = 3.41207 (sig = 00022012) weight 4 (1120), p-value = 0.514
mix3 extreme = 3.50238 (sig = 01120021) weight >=5 (4864), p-value = 0.894
bits per word = 64 (analyzing bits); min category p-value = 0.514

processed 1.25e+11 bytes in 37 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:23:22 2025

p = 0.973
------

mix3 extreme = 1.61617 (sig = 01000000) weight 1 (16), p-value = 0.834
mix3 extreme = 2.75953 (sig = 10002000) weight 2 (112), p-value = 0.478
mix3 extreme = 3.01768 (sig = 22000200) weight 3 (448), p-value = 0.681
mix3 extreme = 3.12351 (sig = 20120020) weight 4 (1120), p-value = 0.865
mix3 extreme = 3.88895 (sig = 01201202) weight >=5 (4864), p-value = 0.387
bits per word = 64 (analyzing bits); min category p-value = 0.387

processed 1.5e+11 bytes in 45 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:23:30 2025

p = 0.914
------

mix3 extreme = 1.77082 (sig = 10000000) weight 1 (16), p-value = 0.721
mix3 extreme = 3.16452 (sig = 20000020) weight 2 (112), p-value = 0.16
mix3 extreme = 3.33010 (sig = 22000200) weight 3 (448), p-value = 0.322
mix3 extreme = 3.30032 (sig = 10202001) weight 4 (1120), p-value = 0.661
mix3 extreme = 3.71439 (sig = 01201202) weight >=5 (4864), p-value = 0.629
bits per word = 64 (analyzing bits); min category p-value = 0.16

processed 1.75e+11 bytes in 52 seconds (3.366 GB/s, 12.12 TB/h). Fri Feb 21 23:23:37 2025

p = 0.581
------

mix3 extreme = 1.63286 (sig = 10000000) weight 1 (16), p-value = 0.823
mix3 extreme = 3.04031 (sig = 20000020) weight 2 (112), p-value = 0.233
mix3 extreme = 3.07386 (sig = 00020120) weight 3 (448), p-value = 0.612
mix3 extreme = 3.25670 (sig = 20200101) weight 4 (1120), p-value = 0.717
mix3 extreme = 3.86559 (sig = 02121212) weight >=5 (4864), p-value = 0.417
bits per word = 64 (analyzing bits); min category p-value = 0.233

processed 2e+11 bytes in 59 seconds (3.39 GB/s, 12.2 TB/h). Fri Feb 21 23:23:44 2025

p = 0.734
------

mix3 extreme = 1.16364 (sig = 00000010) weight 1 (16), p-value = 0.989
mix3 extreme = 3.24185 (sig = 20000020) weight 2 (112), p-value = 0.125
mix3 extreme = 3.33043 (sig = 00020120) weight 3 (448), p-value = 0.322
mix3 extreme = 3.08521 (sig = 01100011) weight 4 (1120), p-value = 0.898
mix3 extreme = 3.38515 (sig = 20222110) weight >=5 (4864), p-value = 0.969
bits per word = 64 (analyzing bits); min category p-value = 0.125

processed 2.5e+11 bytes in 74 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:23:59 2025

p = 0.486
------

mix3 extreme = 1.51793 (sig = 02000000) weight 1 (16), p-value = 0.89
mix3 extreme = 3.13632 (sig = 02000020) weight 2 (112), p-value = 0.175
mix3 extreme = 4.30962 (sig = 00020120) weight 3 (448), p-value = 0.0073
mix3 extreme = 3.03490 (sig = 01000212) weight 4 (1120), p-value = 0.933
mix3 extreme = 3.69122 (sig = 12111112) weight >=5 (4864), p-value = 0.662
bits per word = 64 (analyzing bits); min category p-value = 0.0073

processed 3e+11 bytes in 89 seconds (3.371 GB/s, 12.14 TB/h). Fri Feb 21 23:24:14 2025

p = 0.036
------

mix3 extreme = 1.90308 (sig = 00002000) weight 1 (16), p-value = 0.609
mix3 extreme = 2.65558 (sig = 00022000) weight 2 (112), p-value = 0.589
mix3 extreme = 3.65368 (sig = 00020120) weight 3 (448), p-value = 0.109
mix3 extreme = 3.49934 (sig = 01000212) weight 4 (1120), p-value = 0.407
mix3 extreme = 3.96134 (sig = 12111112) weight >=5 (4864), p-value = 0.304
bits per word = 64 (analyzing bits); min category p-value = 0.109

processed 4e+11 bytes in 119 seconds (3.361 GB/s, 12.1 TB/h). Fri Feb 21 23:24:44 2025

p = 0.44
------

mix3 extreme = 2.01103 (sig = 00000010) weight 1 (16), p-value = 0.516
mix3 extreme = 2.75092 (sig = 20000020) weight 2 (112), p-value = 0.487
mix3 extreme = 3.58533 (sig = 00020120) weight 3 (448), p-value = 0.14
mix3 extreme = 3.00752 (sig = 00101101) weight 4 (1120), p-value = 0.948
mix3 extreme = 3.78108 (sig = 20222110) weight >=5 (4864), p-value = 0.532
bits per word = 64 (analyzing bits); min category p-value = 0.14

processed 5e+11 bytes in 148 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:25:13 2025

p = 0.53
------

mix3 extreme = 2.46075 (sig = 00000010) weight 1 (16), p-value = 0.2
mix3 extreme = 3.05595 (sig = 10001000) weight 2 (112), p-value = 0.222
mix3 extreme = 3.57444 (sig = 00020120) weight 3 (448), p-value = 0.146
mix3 extreme = 3.08460 (sig = 02100120) weight 4 (1120), p-value = 0.898
mix3 extreme = 3.57242 (sig = 21020202) weight >=5 (4864), p-value = 0.821
bits per word = 64 (analyzing bits); min category p-value = 0.146

processed 6e+11 bytes in 178 seconds (3.371 GB/s, 12.14 TB/h). Fri Feb 21 23:25:43 2025

p = 0.544
------

mix3 extreme = 2.39889 (sig = 00010000) weight 1 (16), p-value = 0.233
mix3 extreme = 3.05936 (sig = 20000020) weight 2 (112), p-value = 0.22
mix3 extreme = 3.35611 (sig = 00020120) weight 3 (448), p-value = 0.298
mix3 extreme = 3.06385 (sig = 12002002) weight 4 (1120), p-value = 0.914
mix3 extreme = 3.77731 (sig = 21020202) weight >=5 (4864), p-value = 0.538
bits per word = 64 (analyzing bits); min category p-value = 0.22

processed 7e+11 bytes in 207 seconds (3.382 GB/s, 12.17 TB/h). Fri Feb 21 23:26:12 2025

p = 0.712
------

mix3 extreme = 2.44670 (sig = 00010000) weight 1 (16), p-value = 0.207
mix3 extreme = 3.17448 (sig = 00101000) weight 2 (112), p-value = 0.155
mix3 extreme = 3.94606 (sig = 00020120) weight 3 (448), p-value = 0.035
mix3 extreme = 3.39578 (sig = 12002002) weight 4 (1120), p-value = 0.535
mix3 extreme = 3.46995 (sig = 22000212) weight >=5 (4864), p-value = 0.921
bits per word = 64 (analyzing bits); min category p-value = 0.035

processed 8.5e+11 bytes in 252 seconds (3.373 GB/s, 12.14 TB/h). Fri Feb 21 23:26:57 2025

p = 0.163
------

mix3 extreme = 2.60423 (sig = 00000200) weight 1 (16), p-value = 0.138
mix3 extreme = 4.14356 (sig = 02000020) weight 2 (112), p-value = 0.00382
mix3 extreme = 3.30244 (sig = 00210010) weight 3 (448), p-value = 0.349
mix3 extreme = 2.92058 (sig = 00111100) weight 4 (1120), p-value = 0.98
mix3 extreme = 3.64298 (sig = 21221022) weight >=5 (4864), p-value = 0.73
bits per word = 64 (analyzing bits); min category p-value = 0.00382

processed 1e+12 bytes in 296 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:27:41 2025

p = 0.019
------

mix3 extreme = 2.39411 (sig = 00000200) weight 1 (16), p-value = 0.236
mix3 extreme = 3.86207 (sig = 02000020) weight 2 (112), p-value = 0.0125
mix3 extreme = 3.43732 (sig = 00210010) weight 3 (448), p-value = 0.231
mix3 extreme = 3.30612 (sig = 10010102) weight 4 (1120), p-value = 0.654
mix3 extreme = 3.69570 (sig = 02122102) weight >=5 (4864), p-value = 0.656
bits per word = 64 (analyzing bits); min category p-value = 0.0125

processed 1.25e+12 bytes in 370 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:28:55 2025

p = 0.061
------

mix3 extreme = 1.91917 (sig = 00000200) weight 1 (16), p-value = 0.595
mix3 extreme = 2.88512 (sig = 10001000) weight 2 (112), p-value = 0.355
mix3 extreme = 3.79690 (sig = 00210010) weight 3 (448), p-value = 0.0635
mix3 extreme = 3.81192 (sig = 10010102) weight 4 (1120), p-value = 0.143
mix3 extreme = 3.94746 (sig = 20221222) weight >=5 (4864), p-value = 0.319
bits per word = 64 (analyzing bits); min category p-value = 0.0635

processed 1.5e+12 bytes in 444 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:30:09 2025

p = 0.28
------

mix3 extreme = 1.91653 (sig = 00000200) weight 1 (16), p-value = 0.598
mix3 extreme = 3.17062 (sig = 02000020) weight 2 (112), p-value = 0.157
mix3 extreme = 3.50267 (sig = 00210010) weight 3 (448), p-value = 0.186
mix3 extreme = 4.12447 (sig = 10010102) weight 4 (1120), p-value = 0.0408
mix3 extreme = 4.27098 (sig = 20221222) weight >=5 (4864), p-value = 0.0903
bits per word = 64 (analyzing bits); min category p-value = 0.0408

processed 1.75e+12 bytes in 518 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:31:23 2025

p = 0.188
------

mix3 extreme = 1.81906 (sig = 00000200) weight 1 (16), p-value = 0.681
mix3 extreme = 3.06798 (sig = 10100000) weight 2 (112), p-value = 0.215
mix3 extreme = 3.71332 (sig = 02020200) weight 3 (448), p-value = 0.0876
mix3 extreme = 4.23944 (sig = 10010102) weight 4 (1120), p-value = 0.0248
mix3 extreme = 4.44624 (sig = 20221222) weight >=5 (4864), p-value = 0.0416
bits per word = 64 (analyzing bits); min category p-value = 0.0248

processed 2e+12 bytes in 592 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:32:37 2025

p = 0.118
------

mix3 extreme = 1.53717 (sig = 01000000) weight 1 (16), p-value = 0.88
mix3 extreme = 3.32377 (sig = 10100000) weight 2 (112), p-value = 0.0947
mix3 extreme = 3.12416 (sig = 02200001) weight 3 (448), p-value = 0.55
mix3 extreme = 3.46863 (sig = 02110100) weight 4 (1120), p-value = 0.443
mix3 extreme = 4.84658 (sig = 20221222) weight >=5 (4864), p-value = 0.00609
bits per word = 64 (analyzing bits); min category p-value = 0.00609

processed 2.5e+12 bytes in 740 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:35:05 2025

p = 0.0301
------

mix3 extreme = 1.61173 (sig = 00000200) weight 1 (16), p-value = 0.837
mix3 extreme = 3.37008 (sig = 10100000) weight 2 (112), p-value = 0.0807
mix3 extreme = 3.93348 (sig = 00202100) weight 3 (448), p-value = 0.0368
mix3 extreme = 3.29910 (sig = 02200021) weight 4 (1120), p-value = 0.663
mix3 extreme = 4.29826 (sig = 20221222) weight >=5 (4864), p-value = 0.0803
bits per word = 64 (analyzing bits); min category p-value = 0.0368

processed 3e+12 bytes in 887 seconds (3.382 GB/s, 12.18 TB/h). Fri Feb 21 23:37:32 2025

p = 0.171
------

mix3 extreme = 2.09163 (sig = 00000200) weight 1 (16), p-value = 0.448
mix3 extreme = 2.61382 (sig = 01100000) weight 2 (112), p-value = 0.635
mix3 extreme = 3.98155 (sig = 00202100) weight 3 (448), p-value = 0.0302
mix3 extreme = 3.38573 (sig = 00002211) weight 4 (1120), p-value = 0.549
mix3 extreme = 4.15291 (sig = 20211110) weight >=5 (4864), p-value = 0.148
bits per word = 64 (analyzing bits); min category p-value = 0.0302

processed 4e+12 bytes in 1.18e+03 seconds (3.381 GB/s, 12.17 TB/h). Fri Feb 21 23:42:28 2025

p = 0.142
------
```
-------------------------------------------------------------------------------
