## Pseudo-Random Function Testing

Testing the Pseudo-Random Number Generator (PRNG) Functions isn't somewhat of a chore, as a complete test could take a week or so to complete and my laptop wasn't designed for such long running intensive tasks.

Tests were conducted by utilizing [PractRand](https://pracrand.sourceforge.net) as well as some tests adopted from the [xoshiro](http://xoshiro.di.unimi.it/hwd.php) testing code, running both up to 2TB of random data (which took about 1 night for PractRand).

**Results status (re-testing pending)**: the results below were recorded in February 2025. The `FIO_DEFINE_RANDOM128_FN` algorithm has changed several times since then, which also affects `fio_rand64` / `fio_rand_bytes` because they are built on the same macro. The results describe the earlier version and have not yet been re-confirmed for the current one.

Results are as follows:

------------------------------------------------------------
