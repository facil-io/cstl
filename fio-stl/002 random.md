## Pseudo Random Generation

```c
#define FIO_RAND
#include "fio-stl.h"
```

If the `FIO_RAND` macro is defined, the following non-cryptographic pseudo-random generator and hash functions will be defined.

The "random" data is initialized / seeded automatically using a small number of functional cycles that collect data and hash it, hopefully resulting in enough jitter entropy.

The data is collected using `getrusage` (or the system clock if `getrusage` is unavailable) and hashed using RiskyHash. The data is then combined with the previous state / cycle.

The CPU "jitter" within the calculation **should** affect `getrusage` in a way that makes it impossible for an attacker to determine the resulting random state (assuming jitter exists).

However, this is unlikely to prove cryptographically safe and isn't likely to produce a large number of entropy bits (even though a small number of bits have a large impact on the final state).

The facil.io random generator functions appear both faster and more random than the standard `rand` on my computer (you can test it for yours).

I designed it in the hopes of achieving a cryptographically safe PRNG, but it wasn't cryptographically analyzed, lacks a good source of entropy and should be considered as a good enough non-cryptographic PRNG for general use.

**Note**: bitwise operations (`FIO_BITWISE`), Risky Hash and Stable Hash are automatically defined along with `FIO_RAND`, since they are required by the algorithm.

### Pseudo-Random Generator Functions

#### `fio_rand64`

```c
uint64_t fio_rand64(void);
```

Returns 64 pseudo-random bits. Probably **not** cryptographically safe.

#### `fio_rand128`

```c
fio_u128 fio_rand128(void);
```

Returns 128 pseudo-random bits. Probably **not** cryptographically safe.

**Note**: returns a `fio_u128` type which is a 128-bit unsigned integer structure.

#### `fio_rand_bytes`

```c
void fio_rand_bytes(void *target, size_t len);
```

Writes `len` bytes of pseudo-random data to the buffer pointed to by `target`. Probably **not** cryptographically safe.

#### `fio_rand_bytes_secure`

```c
int fio_rand_bytes_secure(void *target, size_t len);
```

Writes `len` bytes of cryptographically secure random data to `target`.

Uses the system CSPRNG: `arc4random_buf()` on BSD/macOS, or `/dev/urandom` as fallback on other POSIX systems.

**Returns:** `0` on success, `-1` on failure.

**Note**: Use this function for security-sensitive operations like key generation, nonces, or any cryptographic purpose. For non-security-critical random data, prefer `fio_rand_bytes` which is faster.

#### `fio_rand_reseed`

```c
void fio_rand_reseed(void);
```

Forces the random generator state to rotate.

**Note**: SHOULD be called after `fork` to prevent the two processes from outputting the same random numbers (until a reseed is called automatically).

### Risky Hash / Stable Hash (data hashing)

Stable Hash is a stable block hashing algorithm that can be used to hash non-ephemeral data. The hashing speeds are competitively fast, the algorithm is fairly simple with good avalanche dispersion and minimal bias.

Risky Hash is a non-stable hashing algorithm that is aimed at ephemeral data hashing (i.e., hash map keys) and might be updated periodically to produce different hashing results. It too aims to balance security concerns with performance.

Both algorithms are **non-cryptographic** and produce 64-bit hashes by default (though internally both use a 256-bit block that could be used to produce 128-bit hashes). Both pass the SMHasher test suite for hashing functions.

#### `FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH`

```c
#define FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH 0
```

When set to `1`, calls to `fio_risky_hash` will be redirected to `fio_stable_hash` instead. This can be useful when you need stable hashing behavior but are using code that calls `fio_risky_hash`.

Default is `0` (disabled).

#### `fio_stable_hash`

```c
uint64_t fio_stable_hash(const void *data, size_t len, uint64_t seed);
```

Computes a 64-bit facil.io Stable Hash.

Once version 1.0 is released, this algorithm will not be updated, even if broken. This makes it suitable for persistent storage or cross-system communication where hash stability is required.

#### `fio_stable_hash128`

```c
void fio_stable_hash128(void *restrict dest,
                        const void *restrict data,
                        size_t len,
                        uint64_t seed);
```

Computes a 128-bit facil.io Stable Hash.

Once version 1.0 is released, this algorithm will not be updated, even if broken.

**Parameters:**
- `dest` - pointer to a 16-byte buffer where the 128-bit hash will be written
- `data` - pointer to the data to hash
- `len` - length of the data in bytes
- `seed` - seed value for the hash

#### `fio_risky_hash`

```c
uint64_t fio_risky_hash(const void *buf, size_t len, uint64_t seed);
```

This is a non-streaming implementation of the RiskyHash v.3 algorithm.

This function will produce a 64-bit hash for the given data.

**Note**: the hashing algorithm may change at any time and the hash value should be considered ephemeral. Meant to be safe enough for use with hash maps.

**Note**: if `FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH` is defined and true, `fio_stable_hash` will be called instead.

#### `fio_risky_ptr`

```c
uint64_t fio_risky_ptr(void *ptr);
```

Adds a bit of entropy to pointer values. Designed to be unsafe (fast, not cryptographically secure).

**Note**: the hashing algorithm may change at any time and the hash value should be considered ephemeral. Meant to be safe enough for use with hash maps.

#### `fio_risky_num`

```c
uint64_t fio_risky_num(uint64_t number, uint64_t seed);
```

Adds a bit of entropy to numeral values. Designed to be unsafe (fast, not cryptographically secure).

**Note**: the hashing algorithm may change at any time and the hash value should be considered ephemeral. Meant to be safe enough for use with hash maps, but that's about it.

#### `fio_risky256`

```c
fio_u256 fio_risky256(const void *data, uint64_t len);
```

Computes a 256-bit non-cryptographic hash using the RiskyHash 256 algorithm.

Based on the A3 (Zero-Copy ILP) design: two independent 512-bit states process 128 bytes per iteration using multiply-fold with cross-lane mixing. Returns a `fio_u256` (32 bytes).

**Quality**: passes strict avalanche (50.0%), collision resistance (0 collisions in 1M hashes), differential (min Hamming distance 85/256), and length independence tests.

**Performance**: ~25-27 GB/s on modern hardware for large inputs, ~3.7 GB/s for 64-byte inputs.

**Note**: this is a non-cryptographic hash. The algorithm may change at any time and hash values should be considered ephemeral.

#### `fio_risky512`

```c
fio_u512 fio_risky512(const void *data, uint64_t len);
```

Computes a 512-bit non-cryptographic hash using the RiskyHash 512 algorithm.

This is a SHAKE-style extension of `fio_risky256`: the first 256 bits of the 512-bit output are **identical** to `fio_risky256` (truncation-safe). The second 256 bits come from an additional squeeze round. Returns a `fio_u512` (64 bytes).

**Note**: this is a non-cryptographic hash. The algorithm may change at any time and hash values should be considered ephemeral.

#### `fio_risky256_hmac`

```c
fio_u256 fio_risky256_hmac(const void *key, uint64_t key_len,
                           const void *msg, uint64_t msg_len);
```

Computes HMAC-RiskyHash-256 using the standard RFC 2104 HMAC construction with `fio_risky256` as the underlying hash function.

Uses a 64-byte block size. If `key_len > 64`, the key is first hashed with `fio_risky256`. Returns a `fio_u256` (32 bytes).

All sensitive intermediates (padded key, inner hash) are securely zeroed after use.

**Note**: this uses a non-cryptographic hash internally. For cryptographic HMAC, use `fio_blake2b_hmac` or `fio_blake2s_hmac`.

#### `fio_risky512_hmac`

```c
fio_u512 fio_risky512_hmac(const void *key, uint64_t key_len,
                           const void *msg, uint64_t msg_len);
```

Computes HMAC-RiskyHash-512 using the standard RFC 2104 HMAC construction with `fio_risky512` as the underlying hash function.

Uses a 64-byte block size. If `key_len > 64`, the key is first hashed with `fio_risky512`. Returns a `fio_u512` (64 bytes).

All sensitive intermediates (padded key, inner hash) are securely zeroed after use.

**Note**: this uses a non-cryptographic hash internally. For cryptographic HMAC, use `fio_blake2b_hmac` or `fio_blake2s_hmac`.

-------------------------------------------------------------------------------
