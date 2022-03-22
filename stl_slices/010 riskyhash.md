## Risky Hash / Stable Hash (data hashing):

Stable Hash is a stable block hashing algorithm that can be used to hash non-ephemeral data. The hashing speeds are competitively fast, the algorithm is fairly simple with good avalanche dispersion and minimal bias.

Risky Hash is a non-stable hashing algorithm that is aimed at ephemeral data hashing (i.e., hash maps keys) and might be updated periodically to produce different hashing results. It too aims to balance security concerns with all the features 

Both algorithms are **non-cryptographic** and produce 64 bit hashes by default (though internally both use a 256 block that could be used to produce 128bit hashes). Both pass the SMHasher test suite for hashing functions.

If the `FIO_RISKY_HASH` macro is defined than the following static function will be defined:


#### `fio_stable_hash`

```c
uint64_t fio_stable_hash(const void *data, size_t len, uint64_t seed);
```

Computes a 64 bit facil.io Stable Hash (once version 0.8 is released, this algorithm will not be updated, even if broken).

#### `fio_stable_hash128`

```c
void fio_stable_hash128(void *restrict dest,
                        const void *restrict data,
                        size_t len,
                        uint64_t seed);
```

Computes a 128 bit facil.io Stable Hash (once version 0.8 is released, this algorithm will not be updated, even if broken).

#### `fio_risky_hash`

```c
uint64_t fio_risky_hash(const void *data, size_t len, uint64_t seed)
```

This is a non-streaming implementation of the RiskyHash v.3 algorithm.

This function will produce a 64 bit hash for X bytes of data.

#### `fio_risky_ptr`

```c
uint64_t fio_risky_ptr(void *ptr);
```

Adds a bit of entropy to pointer values. Designed to be unsafe enough for use with hash maps.

#### `fio_risky_mask`

```c
void fio_risky_mask(char *buf, size_t len, uint64_t key, uint64_t nonce);
```

Masks data using a Risky Hash and a counter mode nonce, using `fio_xmask2`.

Used for mitigating memory access attacks when storing "secret" information in memory.

Keep the nonce information in a different memory address then the secret. For example, if the secret is on the stack, store the nonce on the heap or using a static variable.

Don't use the same nonce-secret combination for other data.

This is **not** a cryptographically secure encryption. Even **if** the algorithm was secure, it would provide no more then a 32 bit level encryption, which isn't strong enough for any cryptographic use-case.

However, this could be used to mitigate memory probing attacks. Secrets stored in the memory might remain accessible after the program exists or through core dump information. By storing "secret" information masked in this way, it mitigates the risk of secret information being easily recognized.


-------------------------------------------------------------------------------

## Pseudo Random Generation

If the `FIO_RAND` macro is defined, the following, non-cryptographic psedo-random generator functions will be defined.

The "random" data is initialized / seeded automatically using a small number of functional cycles that collect data and hash it, hopefully resulting in enough jitter entropy.

The data is collected using `getrusage` (or the system clock if `getrusage` is unavailable) and hashed using RiskyHash. The data is then combined with the previous state / cycle.

The CPU "jitter" within the calculation **should** effect `getrusage` in a way that makes it impossible for an attacker to determine the resulting random state (assuming jitter exists).

However, this is unlikely to prove cryptographically safe and isn't likely to produce a large number of entropy bits (even though a small number of bits have a large impact on the final state).

The facil.io random generator functions appear both faster and more random then the standard `rand` on my computer (you can test it for yours).

I designed it in the hopes of achieving a cryptographically safe PRNG, but it wasn't cryptographically analyzed, lacks a good source of entropy and should be considered as a non-cryptographic PRNG.

**Note**: bitwise operations (`FIO_BITWISE`) and Risky Hash (`FIO_RISKY_HASH`) are automatically defined along with `FIO_RAND`, since they are required by the algorithm.

#### `fio_rand64`

```c
uint64_t fio_rand64(void)
```

Returns 64 random bits. Probably **not** cryptographically safe.

#### `fio_rand_bytes`

```c
void fio_rand_bytes(void *data_, size_t len)
```

Writes `len` random Bytes to the buffer pointed to by `data`. Probably **not**
cryptographically safe.

#### `fio_rand_feed2seed`

```c
static void fio_rand_feed2seed(void *buf_, size_t len);
```

An internal function (accessible from the translation unit) that allows a program to feed random data to the PRNG (`fio_rand64`).

The random data will effect the random seed on the next reseeding.

Limited to 1023 bytes of data per function call.

#### `fio_rand_reseed`

```c
void fio_rand_reseed(void);
```

Forces the random generator state to rotate.

SHOULD be called after `fork` to prevent the two processes from outputting the same random numbers (until a reseed is called automatically).

-------------------------------------------------------------------------------
