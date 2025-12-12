### `fio_rand`

The following are the tests for the core  `fio_rand64` and `fio_rand_bytes` functions provided when using `FIO_RAND`.

**The `PractRand` results**:

```txt
# ./tmp/rnd -p fio | RNG_test stdin
RNG_test using PractRand version 0.95
RNG = RNG_stdin, seed = unknown
test set = core, folding = standard(unknown format)

rng=RNG_stdin, seed=unknown
length= 256 megabytes (2^28 bytes), time= 3.0 seconds
  no anomalies in 217 test result(s)

rng=RNG_stdin, seed=unknown
length= 512 megabytes (2^29 bytes), time= 6.5 seconds
  no anomalies in 232 test result(s)

rng=RNG_stdin, seed=unknown
length= 1 gigabyte (2^30 bytes), time= 13.0 seconds
  no anomalies in 251 test result(s)

rng=RNG_stdin, seed=unknown
length= 2 gigabytes (2^31 bytes), time= 25.2 seconds
  no anomalies in 269 test result(s)

rng=RNG_stdin, seed=unknown
length= 4 gigabytes (2^32 bytes), time= 48.2 seconds
  no anomalies in 283 test result(s)

rng=RNG_stdin, seed=unknown
length= 8 gigabytes (2^33 bytes), time= 96.5 seconds
  Test Name                         Raw       Processed     Evaluation
  BCFN(2+7,13-2,T)                  R=  -9.1  p =1-3.9e-5   unusual          
  ...and 299 test result(s) without anomalies

rng=RNG_stdin, seed=unknown
length= 16 gigabytes (2^34 bytes), time= 190 seconds
  no anomalies in 315 test result(s)

rng=RNG_stdin, seed=unknown
length= 32 gigabytes (2^35 bytes), time= 376 seconds
  no anomalies in 328 test result(s)

rng=RNG_stdin, seed=unknown
length= 64 gigabytes (2^36 bytes), time= 752 seconds
  no anomalies in 344 test result(s)

rng=RNG_stdin, seed=unknown
length= 128 gigabytes (2^37 bytes), time= 1498 seconds
  no anomalies in 359 test result(s)

rng=RNG_stdin, seed=unknown
length= 256 gigabytes (2^38 bytes), time= 2978 seconds
  no anomalies in 372 test result(s)

rng=RNG_stdin, seed=unknown
length= 512 gigabytes (2^39 bytes), time= 9897 seconds
  no anomalies in 387 test result(s)

rng=RNG_stdin, seed=unknown
length= 1 terabyte (2^40 bytes), time= 21004 seconds
  no anomalies in 401 test result(s)

rng=RNG_stdin, seed=unknown
length= 2 terabytes (2^41 bytes), time= 43108 seconds
  no anomalies in 413 test result(s)
```

**The tests adopted from the `xoshiro` code base**:


```txt
# ./tmp/random fio
mix3 extreme = 1.80533 (sig = 00100000) weight 1 (16), p-value = 0.692
mix3 extreme = 2.35650 (sig = 00000102) weight 2 (112), p-value = 0.876
mix3 extreme = 3.33991 (sig = 20002100) weight 3 (448), p-value = 0.313
mix3 extreme = 4.30088 (sig = 02210200) weight 4 (1120), p-value = 0.0189
mix3 extreme = 4.03228 (sig = 01002212) weight >=5 (4864), p-value = 0.236
bits per word = 64 (analyzing bits); min category p-value = 0.0189

processed 1.11e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.0909
------

mix3 extreme = 1.84047 (sig = 20000000) weight 1 (16), p-value = 0.663
mix3 extreme = 3.12258 (sig = 00000102) weight 2 (112), p-value = 0.182
mix3 extreme = 3.47031 (sig = 20002100) weight 3 (448), p-value = 0.208
mix3 extreme = 3.61471 (sig = 12000101) weight 4 (1120), p-value = 0.286
mix3 extreme = 3.76213 (sig = 10111202) weight >=5 (4864), p-value = 0.559
bits per word = 64 (analyzing bits); min category p-value = 0.182

processed 1.29e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.634
------

mix3 extreme = 1.65805 (sig = 00000002) weight 1 (16), p-value = 0.806
mix3 extreme = 3.09464 (sig = 00000102) weight 2 (112), p-value = 0.198
mix3 extreme = 3.53210 (sig = 00102010) weight 3 (448), p-value = 0.169
mix3 extreme = 3.35900 (sig = 02210200) weight 4 (1120), p-value = 0.584
mix3 extreme = 3.58826 (sig = 02011101) weight >=5 (4864), p-value = 0.802
bits per word = 64 (analyzing bits); min category p-value = 0.169

processed 1.66e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.603
------

mix3 extreme = 1.75332 (sig = 00000001) weight 1 (16), p-value = 0.735
mix3 extreme = 2.97345 (sig = 00000102) weight 2 (112), p-value = 0.281
mix3 extreme = 3.21121 (sig = 20002100) weight 3 (448), p-value = 0.447
mix3 extreme = 2.99207 (sig = 10102020) weight 4 (1120), p-value = 0.955
mix3 extreme = 3.53616 (sig = 21120101) weight >=5 (4864), p-value = 0.861
bits per word = 64 (analyzing bits); min category p-value = 0.281

processed 1.85e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.808
------

mix3 extreme = 1.83225 (sig = 00000100) weight 1 (16), p-value = 0.67
mix3 extreme = 3.10258 (sig = 00000102) weight 2 (112), p-value = 0.194
mix3 extreme = 3.03291 (sig = 20002100) weight 3 (448), p-value = 0.663
mix3 extreme = 3.09549 (sig = 10200022) weight 4 (1120), p-value = 0.89
mix3 extreme = 3.76428 (sig = 02011101) weight >=5 (4864), p-value = 0.556
bits per word = 64 (analyzing bits); min category p-value = 0.194

processed 2.03e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.659
------

mix3 extreme = 1.84289 (sig = 00000100) weight 1 (16), p-value = 0.661
mix3 extreme = 3.57529 (sig = 00000102) weight 2 (112), p-value = 0.0384
mix3 extreme = 3.46502 (sig = 20002010) weight 3 (448), p-value = 0.211
mix3 extreme = 3.25764 (sig = 00220201) weight 4 (1120), p-value = 0.716
mix3 extreme = 4.19914 (sig = 11222212) weight >=5 (4864), p-value = 0.122
bits per word = 64 (analyzing bits); min category p-value = 0.0384

processed 2.59e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.178
------

mix3 extreme = 2.19203 (sig = 00000002) weight 1 (16), p-value = 0.369
mix3 extreme = 3.04970 (sig = 00000102) weight 2 (112), p-value = 0.227
mix3 extreme = 3.44778 (sig = 20002010) weight 3 (448), p-value = 0.224
mix3 extreme = 3.66866 (sig = 22110000) weight 4 (1120), p-value = 0.239
mix3 extreme = 3.58667 (sig = 02021021) weight >=5 (4864), p-value = 0.804
bits per word = 64 (analyzing bits); min category p-value = 0.224

processed 3.14e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.718
------

mix3 extreme = 2.51064 (sig = 00000002) weight 1 (16), p-value = 0.176
mix3 extreme = 2.57871 (sig = 20010000) weight 2 (112), p-value = 0.672
mix3 extreme = 3.85273 (sig = 20002010) weight 3 (448), p-value = 0.051
mix3 extreme = 3.18862 (sig = 01101010) weight 4 (1120), p-value = 0.799
mix3 extreme = 3.60146 (sig = 22121010) weight >=5 (4864), p-value = 0.785
bits per word = 64 (analyzing bits); min category p-value = 0.051

processed 4.07e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.23
------

mix3 extreme = 1.89834 (sig = 00000002) weight 1 (16), p-value = 0.613
mix3 extreme = 2.27677 (sig = 00100200) weight 2 (112), p-value = 0.924
mix3 extreme = 3.52766 (sig = 20000022) weight 3 (448), p-value = 0.171
mix3 extreme = 3.50133 (sig = 11000102) weight 4 (1120), p-value = 0.405
mix3 extreme = 3.47778 (sig = 11121011) weight >=5 (4864), p-value = 0.915
bits per word = 64 (analyzing bits); min category p-value = 0.171

processed 5.18e+08 bytes in 1 seconds (0.5177 GB/s, 1.864 TB/h). Fri Feb 21 23:22:22 2025

p = 0.609
------

mix3 extreme = 1.99814 (sig = 00000001) weight 1 (16), p-value = 0.527
mix3 extreme = 2.43362 (sig = 00100200) weight 2 (112), p-value = 0.815
mix3 extreme = 3.46309 (sig = 20002010) weight 3 (448), p-value = 0.213
mix3 extreme = 3.67295 (sig = 20110001) weight 4 (1120), p-value = 0.236
mix3 extreme = 3.50848 (sig = 00222210) weight >=5 (4864), p-value = 0.888
bits per word = 64 (analyzing bits); min category p-value = 0.213

processed 6.1e+08 bytes in 1 seconds (0.6101 GB/s, 2.196 TB/h). Fri Feb 21 23:22:22 2025

p = 0.698
------

mix3 extreme = 1.78022 (sig = 00000002) weight 1 (16), p-value = 0.713
mix3 extreme = 2.64570 (sig = 10000001) weight 2 (112), p-value = 0.6
mix3 extreme = 3.55344 (sig = 20002010) weight 3 (448), p-value = 0.157
mix3 extreme = 3.15383 (sig = 11000102) weight 4 (1120), p-value = 0.836
mix3 extreme = 4.03858 (sig = 11121011) weight >=5 (4864), p-value = 0.23
bits per word = 64 (analyzing bits); min category p-value = 0.157

processed 7.03e+08 bytes in 1 seconds (0.7026 GB/s, 2.529 TB/h). Fri Feb 21 23:22:22 2025

p = 0.573
------

mix3 extreme = 1.83987 (sig = 00000002) weight 1 (16), p-value = 0.663
mix3 extreme = 2.43194 (sig = 00000110) weight 2 (112), p-value = 0.816
mix3 extreme = 3.40962 (sig = 20000022) weight 3 (448), p-value = 0.253
mix3 extreme = 3.02964 (sig = 02012200) weight 4 (1120), p-value = 0.936
mix3 extreme = 3.75207 (sig = 20222201) weight >=5 (4864), p-value = 0.574
bits per word = 64 (analyzing bits); min category p-value = 0.253

processed 8.5e+08 bytes in 1 seconds (0.8505 GB/s, 3.062 TB/h). Fri Feb 21 23:22:22 2025

p = 0.767
------

mix3 extreme = 2.03491 (sig = 00000002) weight 1 (16), p-value = 0.496
mix3 extreme = 2.48334 (sig = 00001002) weight 2 (112), p-value = 0.769
mix3 extreme = 3.15042 (sig = 10200002) weight 3 (448), p-value = 0.519
mix3 extreme = 3.19215 (sig = 21200001) weight 4 (1120), p-value = 0.795
mix3 extreme = 3.68390 (sig = 20222201) weight >=5 (4864), p-value = 0.673
bits per word = 64 (analyzing bits); min category p-value = 0.496

processed 1.02e+09 bytes in 1 seconds (1.017 GB/s, 3.661 TB/h). Fri Feb 21 23:22:22 2025

p = 0.967
------

mix3 extreme = 1.98184 (sig = 00000001) weight 1 (16), p-value = 0.541
mix3 extreme = 2.49397 (sig = 01001000) weight 2 (112), p-value = 0.759
mix3 extreme = 2.98386 (sig = 02000011) weight 3 (448), p-value = 0.721
mix3 extreme = 3.29022 (sig = 20200220) weight 4 (1120), p-value = 0.674
mix3 extreme = 4.30382 (sig = 11122011) weight >=5 (4864), p-value = 0.0784
bits per word = 64 (analyzing bits); min category p-value = 0.0784

processed 1.26e+09 bytes in 1 seconds (1.257 GB/s, 4.526 TB/h). Fri Feb 21 23:22:22 2025

p = 0.335
------

mix3 extreme = 2.33599 (sig = 00000001) weight 1 (16), p-value = 0.27
mix3 extreme = 2.45643 (sig = 00000110) weight 2 (112), p-value = 0.795
mix3 extreme = 2.84156 (sig = 00022100) weight 3 (448), p-value = 0.867
mix3 extreme = 3.61210 (sig = 01100202) weight 4 (1120), p-value = 0.288
mix3 extreme = 3.51573 (sig = 21101210) weight >=5 (4864), p-value = 0.882
bits per word = 64 (analyzing bits); min category p-value = 0.27

processed 1.52e+09 bytes in 1 seconds (1.516 GB/s, 5.458 TB/h). Fri Feb 21 23:22:22 2025

p = 0.793
------

mix3 extreme = 2.57590 (sig = 00000001) weight 1 (16), p-value = 0.149
mix3 extreme = 2.18056 (sig = 10000002) weight 2 (112), p-value = 0.964
mix3 extreme = 2.94943 (sig = 20102000) weight 3 (448), p-value = 0.76
mix3 extreme = 3.39387 (sig = 11010002) weight 4 (1120), p-value = 0.538
mix3 extreme = 3.85744 (sig = 22210212) weight >=5 (4864), p-value = 0.427
bits per word = 64 (analyzing bits); min category p-value = 0.149

processed 1.76e+09 bytes in 1 seconds (1.756 GB/s, 6.323 TB/h). Fri Feb 21 23:22:22 2025

p = 0.552
------

mix3 extreme = 2.64879 (sig = 00000001) weight 1 (16), p-value = 0.122
mix3 extreme = 2.69778 (sig = 10000002) weight 2 (112), p-value = 0.544
mix3 extreme = 3.15759 (sig = 20102000) weight 3 (448), p-value = 0.51
mix3 extreme = 3.41194 (sig = 11010002) weight 4 (1120), p-value = 0.515
mix3 extreme = 4.19052 (sig = 11122011) weight >=5 (4864), p-value = 0.127
bits per word = 64 (analyzing bits); min category p-value = 0.122

processed 2.02e+09 bytes in 1 seconds (2.015 GB/s, 7.255 TB/h). Fri Feb 21 23:22:22 2025

p = 0.477
------

mix3 extreme = 2.10150 (sig = 00000001) weight 1 (16), p-value = 0.44
mix3 extreme = 2.31937 (sig = 00100100) weight 2 (112), p-value = 0.9
mix3 extreme = 2.98427 (sig = 20102000) weight 3 (448), p-value = 0.721
mix3 extreme = 3.62790 (sig = 21200001) weight 4 (1120), p-value = 0.274
mix3 extreme = 3.47379 (sig = 22011100) weight >=5 (4864), p-value = 0.918
bits per word = 64 (analyzing bits); min category p-value = 0.274

processed 2.51e+09 bytes in 1 seconds (2.514 GB/s, 9.052 TB/h). Fri Feb 21 23:22:22 2025

p = 0.798
------

mix3 extreme = 2.20184 (sig = 00100000) weight 1 (16), p-value = 0.362
mix3 extreme = 2.44576 (sig = 00100100) weight 2 (112), p-value = 0.804
mix3 extreme = 2.66267 (sig = 10200002) weight 3 (448), p-value = 0.969
mix3 extreme = 3.76139 (sig = 21200001) weight 4 (1120), p-value = 0.172
mix3 extreme = 3.82048 (sig = 20222201) weight >=5 (4864), p-value = 0.477
bits per word = 64 (analyzing bits); min category p-value = 0.172

processed 3.01e+09 bytes in 2 seconds (1.507 GB/s, 5.425 TB/h). Fri Feb 21 23:22:23 2025

p = 0.612
------

mix3 extreme = 2.18426 (sig = 00100000) weight 1 (16), p-value = 0.375
mix3 extreme = 2.65062 (sig = 20002000) weight 2 (112), p-value = 0.595
mix3 extreme = 2.92917 (sig = 01100200) weight 3 (448), p-value = 0.782
mix3 extreme = 3.89526 (sig = 21200001) weight 4 (1120), p-value = 0.104
mix3 extreme = 3.78679 (sig = 20220022) weight >=5 (4864), p-value = 0.524
bits per word = 64 (analyzing bits); min category p-value = 0.104

processed 4.01e+09 bytes in 2 seconds (2.006 GB/s, 7.222 TB/h). Fri Feb 21 23:22:23 2025

p = 0.423
------

mix3 extreme = 2.46216 (sig = 20000000) weight 1 (16), p-value = 0.199
mix3 extreme = 2.30564 (sig = 20002000) weight 2 (112), p-value = 0.909
mix3 extreme = 2.97563 (sig = 00100220) weight 3 (448), p-value = 0.731
mix3 extreme = 3.70435 (sig = 20110001) weight 4 (1120), p-value = 0.211
mix3 extreme = 3.47865 (sig = 22100011) weight >=5 (4864), p-value = 0.914
bits per word = 64 (analyzing bits); min category p-value = 0.199

processed 5.01e+09 bytes in 2 seconds (2.505 GB/s, 9.019 TB/h). Fri Feb 21 23:22:23 2025

p = 0.671
------

mix3 extreme = 2.48402 (sig = 00100000) weight 1 (16), p-value = 0.189
mix3 extreme = 2.32620 (sig = 00000012) weight 2 (112), p-value = 0.896
mix3 extreme = 3.13986 (sig = 00120200) weight 3 (448), p-value = 0.531
mix3 extreme = 3.56935 (sig = 20110001) weight 4 (1120), p-value = 0.33
mix3 extreme = 3.62670 (sig = 01210201) weight >=5 (4864), p-value = 0.753
bits per word = 64 (analyzing bits); min category p-value = 0.189

processed 6.01e+09 bytes in 3 seconds (2.003 GB/s, 7.211 TB/h). Fri Feb 21 23:22:24 2025

p = 0.649
------

mix3 extreme = 2.07995 (sig = 00100000) weight 1 (16), p-value = 0.458
mix3 extreme = 2.14438 (sig = 00002010) weight 2 (112), p-value = 0.974
mix3 extreme = 3.12213 (sig = 00120200) weight 3 (448), p-value = 0.553
mix3 extreme = 3.59330 (sig = 20110001) weight 4 (1120), p-value = 0.306
mix3 extreme = 3.93490 (sig = 01210201) weight >=5 (4864), p-value = 0.333
bits per word = 64 (analyzing bits); min category p-value = 0.306

processed 7.01e+09 bytes in 3 seconds (2.336 GB/s, 8.409 TB/h). Fri Feb 21 23:22:24 2025

p = 0.839
------

mix3 extreme = 1.82939 (sig = 20000000) weight 1 (16), p-value = 0.672
mix3 extreme = 2.04212 (sig = 00002010) weight 2 (112), p-value = 0.991
mix3 extreme = 3.40400 (sig = 00120200) weight 3 (448), p-value = 0.257
mix3 extreme = 3.20336 (sig = 12020002) weight 4 (1120), p-value = 0.782
mix3 extreme = 4.26991 (sig = 01210201) weight >=5 (4864), p-value = 0.0907
bits per word = 64 (analyzing bits); min category p-value = 0.0907

processed 8.5e+09 bytes in 4 seconds (2.126 GB/s, 7.654 TB/h). Fri Feb 21 23:22:25 2025

p = 0.378
------

mix3 extreme = 1.82510 (sig = 00100000) weight 1 (16), p-value = 0.676
mix3 extreme = 2.69933 (sig = 00002020) weight 2 (112), p-value = 0.542
mix3 extreme = 2.78787 (sig = 02020200) weight 3 (448), p-value = 0.908
mix3 extreme = 3.06049 (sig = 00001122) weight 4 (1120), p-value = 0.916
mix3 extreme = 4.40242 (sig = 01210201) weight >=5 (4864), p-value = 0.0507
bits per word = 64 (analyzing bits); min category p-value = 0.0507

processed 1e+10 bytes in 4 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:25 2025

p = 0.229
------

mix3 extreme = 2.20665 (sig = 00100000) weight 1 (16), p-value = 0.358
mix3 extreme = 2.66682 (sig = 00002020) weight 2 (112), p-value = 0.577
mix3 extreme = 3.18173 (sig = 02200010) weight 3 (448), p-value = 0.481
mix3 extreme = 3.39020 (sig = 01202100) weight 4 (1120), p-value = 0.543
mix3 extreme = 3.99569 (sig = 01210201) weight >=5 (4864), p-value = 0.269
bits per word = 64 (analyzing bits); min category p-value = 0.269

processed 1.25e+10 bytes in 5 seconds (2.503 GB/s, 9.012 TB/h). Fri Feb 21 23:22:26 2025

p = 0.792
------

mix3 extreme = 1.60923 (sig = 00000002) weight 1 (16), p-value = 0.838
mix3 extreme = 2.73755 (sig = 02000001) weight 2 (112), p-value = 0.501
mix3 extreme = 3.23640 (sig = 00101200) weight 3 (448), p-value = 0.419
mix3 extreme = 3.31439 (sig = 21220000) weight 4 (1120), p-value = 0.643
mix3 extreme = 3.57996 (sig = 21001101) weight >=5 (4864), p-value = 0.812
bits per word = 64 (analyzing bits); min category p-value = 0.419

processed 1.5e+10 bytes in 6 seconds (2.502 GB/s, 9.008 TB/h). Fri Feb 21 23:22:27 2025

p = 0.934
------

mix3 extreme = 1.91785 (sig = 00000002) weight 1 (16), p-value = 0.596
mix3 extreme = 2.60874 (sig = 02000001) weight 2 (112), p-value = 0.64
mix3 extreme = 3.38156 (sig = 00101200) weight 3 (448), p-value = 0.276
mix3 extreme = 3.24044 (sig = 21220000) weight 4 (1120), p-value = 0.737
mix3 extreme = 3.69590 (sig = 12211201) weight >=5 (4864), p-value = 0.656
bits per word = 64 (analyzing bits); min category p-value = 0.276

processed 1.75e+10 bytes in 7 seconds (2.501 GB/s, 9.004 TB/h). Fri Feb 21 23:22:28 2025

p = 0.801
------

mix3 extreme = 1.80532 (sig = 00000001) weight 1 (16), p-value = 0.692
mix3 extreme = 2.54416 (sig = 00020010) weight 2 (112), p-value = 0.709
mix3 extreme = 3.15400 (sig = 00101200) weight 3 (448), p-value = 0.514
mix3 extreme = 3.33906 (sig = 21220000) weight 4 (1120), p-value = 0.61
mix3 extreme = 3.56547 (sig = 00012112) weight >=5 (4864), p-value = 0.829
bits per word = 64 (analyzing bits); min category p-value = 0.514

processed 2e+10 bytes in 8 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:29 2025

p = 0.973
------

mix3 extreme = 1.94039 (sig = 00000002) weight 1 (16), p-value = 0.577
mix3 extreme = 2.27077 (sig = 01010000) weight 2 (112), p-value = 0.928
mix3 extreme = 3.19342 (sig = 00120200) weight 3 (448), p-value = 0.468
mix3 extreme = 3.51655 (sig = 21220000) weight 4 (1120), p-value = 0.387
mix3 extreme = 3.56607 (sig = 20122012) weight >=5 (4864), p-value = 0.828
bits per word = 64 (analyzing bits); min category p-value = 0.387

processed 2.5e+10 bytes in 10 seconds (2.502 GB/s, 9.005 TB/h). Fri Feb 21 23:22:31 2025

p = 0.914
------

mix3 extreme = 1.89200 (sig = 01000000) weight 1 (16), p-value = 0.619
mix3 extreme = 2.24801 (sig = 01010000) weight 2 (112), p-value = 0.938
mix3 extreme = 2.88784 (sig = 02000210) weight 3 (448), p-value = 0.825
mix3 extreme = 3.34758 (sig = 21220000) weight 4 (1120), p-value = 0.599
mix3 extreme = 3.45248 (sig = 11200111) weight >=5 (4864), p-value = 0.933
bits per word = 64 (analyzing bits); min category p-value = 0.599

processed 3e+10 bytes in 12 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:33 2025

p = 0.99
------

mix3 extreme = 1.56396 (sig = 00000200) weight 1 (16), p-value = 0.865
mix3 extreme = 1.97760 (sig = 00000022) weight 2 (112), p-value = 0.996
mix3 extreme = 3.23027 (sig = 20021000) weight 3 (448), p-value = 0.426
mix3 extreme = 3.36856 (sig = 21021000) weight 4 (1120), p-value = 0.571
mix3 extreme = 4.27622 (sig = 20122012) weight >=5 (4864), p-value = 0.0883
bits per word = 64 (analyzing bits); min category p-value = 0.0883

processed 4e+10 bytes in 16 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:37 2025

p = 0.37
------

mix3 extreme = 1.89666 (sig = 00000100) weight 1 (16), p-value = 0.615
mix3 extreme = 2.70819 (sig = 20000010) weight 2 (112), p-value = 0.532
mix3 extreme = 3.11747 (sig = 20021000) weight 3 (448), p-value = 0.559
mix3 extreme = 2.83492 (sig = 21021000) weight 4 (1120), p-value = 0.994
mix3 extreme = 3.73280 (sig = 20122012) weight >=5 (4864), p-value = 0.602
bits per word = 64 (analyzing bits); min category p-value = 0.532

processed 5e+10 bytes in 20 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:41 2025

p = 0.978
------

mix3 extreme = 1.60871 (sig = 00000001) weight 1 (16), p-value = 0.838
mix3 extreme = 2.92820 (sig = 20000010) weight 2 (112), p-value = 0.318
mix3 extreme = 3.30722 (sig = 20021000) weight 3 (448), p-value = 0.344
mix3 extreme = 3.12534 (sig = 12100002) weight 4 (1120), p-value = 0.863
mix3 extreme = 3.90592 (sig = 21101011) weight >=5 (4864), p-value = 0.367
bits per word = 64 (analyzing bits); min category p-value = 0.318

processed 6e+10 bytes in 24 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:45 2025

p = 0.852
------

mix3 extreme = 1.48010 (sig = 02000000) weight 1 (16), p-value = 0.909
mix3 extreme = 2.48996 (sig = 20000010) weight 2 (112), p-value = 0.763
mix3 extreme = 3.17803 (sig = 20021000) weight 3 (448), p-value = 0.486
mix3 extreme = 3.17420 (sig = 20101010) weight 4 (1120), p-value = 0.814
mix3 extreme = 3.79968 (sig = 21101011) weight >=5 (4864), p-value = 0.506
bits per word = 64 (analyzing bits); min category p-value = 0.486

processed 7e+10 bytes in 28 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:49 2025

p = 0.964
------

mix3 extreme = 2.31418 (sig = 02000000) weight 1 (16), p-value = 0.284
mix3 extreme = 2.75073 (sig = 02000001) weight 2 (112), p-value = 0.487
mix3 extreme = 3.22184 (sig = 20021000) weight 3 (448), p-value = 0.435
mix3 extreme = 2.99913 (sig = 12100002) weight 4 (1120), p-value = 0.952
mix3 extreme = 4.17272 (sig = 21101011) weight >=5 (4864), p-value = 0.136
bits per word = 64 (analyzing bits); min category p-value = 0.136

processed 8.5e+10 bytes in 35 seconds (2.429 GB/s, 8.744 TB/h). Fri Feb 21 23:22:56 2025

p = 0.519
------

mix3 extreme = 2.96167 (sig = 02000000) weight 1 (16), p-value = 0.0478
mix3 extreme = 2.15955 (sig = 02000001) weight 2 (112), p-value = 0.97
mix3 extreme = 3.37730 (sig = 20021000) weight 3 (448), p-value = 0.28
mix3 extreme = 3.52700 (sig = 12100002) weight 4 (1120), p-value = 0.376
mix3 extreme = 4.04580 (sig = 21101011) weight >=5 (4864), p-value = 0.224
bits per word = 64 (analyzing bits); min category p-value = 0.0478

processed 1e+11 bytes in 41 seconds (2.439 GB/s, 8.781 TB/h). Fri Feb 21 23:23:02 2025

p = 0.217
------

mix3 extreme = 2.47196 (sig = 02000000) weight 1 (16), p-value = 0.195
mix3 extreme = 2.51373 (sig = 02000001) weight 2 (112), p-value = 0.74
mix3 extreme = 3.39366 (sig = 20021000) weight 3 (448), p-value = 0.266
mix3 extreme = 3.21456 (sig = 12100002) weight 4 (1120), p-value = 0.769
mix3 extreme = 3.77086 (sig = 12221021) weight >=5 (4864), p-value = 0.547
bits per word = 64 (analyzing bits); min category p-value = 0.195

processed 1.25e+11 bytes in 51 seconds (2.451 GB/s, 8.824 TB/h). Fri Feb 21 23:23:12 2025

p = 0.661
------

mix3 extreme = 2.48207 (sig = 02000000) weight 1 (16), p-value = 0.19
mix3 extreme = 2.29109 (sig = 20020000) weight 2 (112), p-value = 0.917
mix3 extreme = 3.01591 (sig = 20021000) weight 3 (448), p-value = 0.683
mix3 extreme = 3.36633 (sig = 20200101) weight 4 (1120), p-value = 0.574
mix3 extreme = 3.40749 (sig = 02222012) weight >=5 (4864), p-value = 0.959
bits per word = 64 (analyzing bits); min category p-value = 0.19

processed 1.5e+11 bytes in 61 seconds (2.459 GB/s, 8.853 TB/h). Fri Feb 21 23:23:22 2025

p = 0.651
------

mix3 extreme = 2.14788 (sig = 02000000) weight 1 (16), p-value = 0.403
mix3 extreme = 2.23188 (sig = 01000002) weight 2 (112), p-value = 0.945
mix3 extreme = 3.01619 (sig = 20100002) weight 3 (448), p-value = 0.683
mix3 extreme = 3.35531 (sig = 22020010) weight 4 (1120), p-value = 0.589
mix3 extreme = 3.90091 (sig = 20120120) weight >=5 (4864), p-value = 0.373
bits per word = 64 (analyzing bits); min category p-value = 0.373

processed 1.75e+11 bytes in 71 seconds (2.465 GB/s, 8.874 TB/h). Fri Feb 21 23:23:32 2025

p = 0.903
------

mix3 extreme = 2.56990 (sig = 00010000) weight 1 (16), p-value = 0.151
mix3 extreme = 2.15879 (sig = 20020000) weight 2 (112), p-value = 0.97
mix3 extreme = 3.11637 (sig = 20100002) weight 3 (448), p-value = 0.56
mix3 extreme = 3.75031 (sig = 20200101) weight 4 (1120), p-value = 0.179
mix3 extreme = 3.82089 (sig = 11022021) weight >=5 (4864), p-value = 0.476
bits per word = 64 (analyzing bits); min category p-value = 0.151

processed 2e+11 bytes in 81 seconds (2.469 GB/s, 8.889 TB/h). Fri Feb 21 23:23:42 2025

p = 0.559
------

mix3 extreme = 2.68286 (sig = 00010000) weight 1 (16), p-value = 0.111
mix3 extreme = 2.03612 (sig = 01000002) weight 2 (112), p-value = 0.992
mix3 extreme = 2.72508 (sig = 01110000) weight 3 (448), p-value = 0.944
mix3 extreme = 3.29533 (sig = 20200101) weight 4 (1120), p-value = 0.668
mix3 extreme = 3.94062 (sig = 11101011) weight >=5 (4864), p-value = 0.327
bits per word = 64 (analyzing bits); min category p-value = 0.111

processed 2.5e+11 bytes in 102 seconds (2.451 GB/s, 8.824 TB/h). Fri Feb 21 23:24:03 2025

p = 0.444
------

mix3 extreme = 3.02806 (sig = 00010000) weight 1 (16), p-value = 0.0387
mix3 extreme = 2.64241 (sig = 12000000) weight 2 (112), p-value = 0.604
mix3 extreme = 3.14107 (sig = 12001000) weight 3 (448), p-value = 0.53
mix3 extreme = 3.10292 (sig = 10202002) weight 4 (1120), p-value = 0.883
mix3 extreme = 3.78800 (sig = 11122221) weight >=5 (4864), p-value = 0.522
bits per word = 64 (analyzing bits); min category p-value = 0.0387

processed 3e+11 bytes in 122 seconds (2.459 GB/s, 8.853 TB/h). Fri Feb 21 23:24:23 2025

p = 0.179
------

mix3 extreme = 2.11373 (sig = 00010000) weight 1 (16), p-value = 0.43
mix3 extreme = 2.53673 (sig = 00010020) weight 2 (112), p-value = 0.716
mix3 extreme = 3.82566 (sig = 01000220) weight 3 (448), p-value = 0.0568
mix3 extreme = 3.94266 (sig = 22020010) weight 4 (1120), p-value = 0.0863
mix3 extreme = 3.38864 (sig = 02212120) weight >=5 (4864), p-value = 0.967
bits per word = 64 (analyzing bits); min category p-value = 0.0568

processed 4e+11 bytes in 163 seconds (2.454 GB/s, 8.834 TB/h). Fri Feb 21 23:25:04 2025

p = 0.253
------

mix3 extreme = 2.02087 (sig = 02000000) weight 1 (16), p-value = 0.507
mix3 extreme = 3.38282 (sig = 00010020) weight 2 (112), p-value = 0.0772
mix3 extreme = 3.77275 (sig = 01000220) weight 3 (448), p-value = 0.0698
mix3 extreme = 3.96045 (sig = 22020010) weight 4 (1120), p-value = 0.0804
mix3 extreme = 3.96698 (sig = 10212100) weight >=5 (4864), p-value = 0.298
bits per word = 64 (analyzing bits); min category p-value = 0.0698

processed 5e+11 bytes in 204 seconds (2.451 GB/s, 8.824 TB/h). Fri Feb 21 23:25:45 2025

p = 0.304
------

mix3 extreme = 2.11460 (sig = 02000000) weight 1 (16), p-value = 0.429
mix3 extreme = 3.45081 (sig = 00010020) weight 2 (112), p-value = 0.0607
mix3 extreme = 4.11176 (sig = 01000220) weight 3 (448), p-value = 0.0174
mix3 extreme = 3.90641 (sig = 22020010) weight 4 (1120), p-value = 0.0996
mix3 extreme = 3.86051 (sig = 10110212) weight >=5 (4864), p-value = 0.423
bits per word = 64 (analyzing bits); min category p-value = 0.0174

processed 6e+11 bytes in 244 seconds (2.459 GB/s, 8.853 TB/h). Fri Feb 21 23:26:25 2025

p = 0.0842
------

mix3 extreme = 2.19471 (sig = 02000000) weight 1 (16), p-value = 0.367
mix3 extreme = 3.42206 (sig = 00010020) weight 2 (112), p-value = 0.0673
mix3 extreme = 3.76615 (sig = 01000220) weight 3 (448), p-value = 0.0716
mix3 extreme = 3.59051 (sig = 22020010) weight 4 (1120), p-value = 0.309
mix3 extreme = 3.55018 (sig = 20012221) weight >=5 (4864), p-value = 0.846
bits per word = 64 (analyzing bits); min category p-value = 0.0673

processed 7e+11 bytes in 285 seconds (2.456 GB/s, 8.842 TB/h). Fri Feb 21 23:27:06 2025

p = 0.294
------

mix3 extreme = 2.08568 (sig = 02000000) weight 1 (16), p-value = 0.453
mix3 extreme = 2.70372 (sig = 00010020) weight 2 (112), p-value = 0.537
mix3 extreme = 3.67441 (sig = 20021000) weight 3 (448), p-value = 0.101
mix3 extreme = 3.40600 (sig = 22020010) weight 4 (1120), p-value = 0.522
mix3 extreme = 3.74169 (sig = 10110212) weight >=5 (4864), p-value = 0.589
bits per word = 64 (analyzing bits); min category p-value = 0.101

processed 8.5e+11 bytes in 346 seconds (2.457 GB/s, 8.844 TB/h). Fri Feb 21 23:28:07 2025

p = 0.414
------

mix3 extreme = 1.70943 (sig = 00000100) weight 1 (16), p-value = 0.768
mix3 extreme = 3.16917 (sig = 00010020) weight 2 (112), p-value = 0.157
mix3 extreme = 3.64671 (sig = 02000201) weight 3 (448), p-value = 0.112
mix3 extreme = 3.82262 (sig = 11010200) weight 4 (1120), p-value = 0.137
mix3 extreme = 3.64758 (sig = 00222021) weight >=5 (4864), p-value = 0.724
bits per word = 64 (analyzing bits); min category p-value = 0.112

processed 1e+12 bytes in 407 seconds (2.457 GB/s, 8.845 TB/h). Fri Feb 21 23:29:08 2025

p = 0.448
------

mix3 extreme = 1.76773 (sig = 20000000) weight 1 (16), p-value = 0.723
mix3 extreme = 3.14942 (sig = 00002020) weight 2 (112), p-value = 0.168
mix3 extreme = 3.45782 (sig = 02000201) weight 3 (448), p-value = 0.217
mix3 extreme = 3.68367 (sig = 11010200) weight 4 (1120), p-value = 0.227
mix3 extreme = 4.05225 (sig = 00222021) weight >=5 (4864), p-value = 0.219
bits per word = 64 (analyzing bits); min category p-value = 0.168

processed 1.25e+12 bytes in 509 seconds (2.456 GB/s, 8.841 TB/h). Fri Feb 21 23:30:50 2025

p = 0.6
------

mix3 extreme = 1.78446 (sig = 00000001) weight 1 (16), p-value = 0.709
mix3 extreme = 3.00030 (sig = 00002020) weight 2 (112), p-value = 0.261
mix3 extreme = 3.68239 (sig = 02000201) weight 3 (448), p-value = 0.0983
mix3 extreme = 3.38984 (sig = 11010020) weight 4 (1120), p-value = 0.543
mix3 extreme = 4.00734 (sig = 02221002) weight >=5 (4864), p-value = 0.258
bits per word = 64 (analyzing bits); min category p-value = 0.0983

processed 1.5e+12 bytes in 611 seconds (2.455 GB/s, 8.838 TB/h). Fri Feb 21 23:32:32 2025

p = 0.404
------

mix3 extreme = 1.49970 (sig = 02000000) weight 1 (16), p-value = 0.899
mix3 extreme = 2.64430 (sig = 20000020) weight 2 (112), p-value = 0.602
mix3 extreme = 3.85954 (sig = 02000201) weight 3 (448), p-value = 0.0496
mix3 extreme = 3.76326 (sig = 11010020) weight 4 (1120), p-value = 0.171
mix3 extreme = 3.91001 (sig = 00222021) weight >=5 (4864), p-value = 0.362
bits per word = 64 (analyzing bits); min category p-value = 0.0496

processed 1.75e+12 bytes in 713 seconds (2.454 GB/s, 8.836 TB/h). Fri Feb 21 23:34:14 2025

p = 0.225
------

mix3 extreme = 1.64313 (sig = 02000000) weight 1 (16), p-value = 0.816
mix3 extreme = 3.02382 (sig = 02100000) weight 2 (112), p-value = 0.244
mix3 extreme = 3.21441 (sig = 02000201) weight 3 (448), p-value = 0.443
mix3 extreme = 4.07811 (sig = 11010020) weight 4 (1120), p-value = 0.0496
mix3 extreme = 4.50737 (sig = 00222021) weight >=5 (4864), p-value = 0.0314
bits per word = 64 (analyzing bits); min category p-value = 0.0314

processed 2e+12 bytes in 815 seconds (2.454 GB/s, 8.834 TB/h). Fri Feb 21 23:35:56 2025

p = 0.148
------

mix3 extreme = 1.57884 (sig = 20000000) weight 1 (16), p-value = 0.857
mix3 extreme = 2.68908 (sig = 00001002) weight 2 (112), p-value = 0.553
mix3 extreme = 3.22275 (sig = 01000220) weight 3 (448), p-value = 0.434
mix3 extreme = 3.84110 (sig = 11010020) weight 4 (1120), p-value = 0.128
mix3 extreme = 3.89955 (sig = 10020112) weight >=5 (4864), p-value = 0.374
bits per word = 64 (analyzing bits); min category p-value = 0.128

processed 2.5e+12 bytes in 1.02e+03 seconds (2.456 GB/s, 8.841 TB/h). Fri Feb 21 23:39:19 2025

p = 0.496
------

mix3 extreme = 1.67736 (sig = 00001000) weight 1 (16), p-value = 0.792
mix3 extreme = 2.62488 (sig = 00001002) weight 2 (112), p-value = 0.623
mix3 extreme = 3.38419 (sig = 02002002) weight 3 (448), p-value = 0.274
mix3 extreme = 3.80757 (sig = 11010020) weight 4 (1120), p-value = 0.145
mix3 extreme = 3.95015 (sig = 00211221) weight >=5 (4864), p-value = 0.316
bits per word = 64 (analyzing bits); min category p-value = 0.145

processed 3e+12 bytes in 1.22e+03 seconds (2.451 GB/s, 8.824 TB/h). Fri Feb 21 23:42:45 2025

p = 0.544
------

mix3 extreme = 1.79534 (sig = 00002000) weight 1 (16), p-value = 0.701
mix3 extreme = 2.76965 (sig = 00001002) weight 2 (112), p-value = 0.468
mix3 extreme = 3.58785 (sig = 01000220) weight 3 (448), p-value = 0.139
mix3 extreme = 3.25341 (sig = 20200012) weight 4 (1120), p-value = 0.721
mix3 extreme = 3.65576 (sig = 12111122) weight >=5 (4864), p-value = 0.713
bits per word = 64 (analyzing bits); min category p-value = 0.139

processed 4e+12 bytes in 1.63e+03 seconds (2.448 GB/s, 8.813 TB/h). Fri Feb 21 23:49:35 2025

p = 0.526
------
```

------------------------------------------------------------
