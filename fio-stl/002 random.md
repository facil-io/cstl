# Pseudo-Random and Hashing

```c
#define FIO_RAND
#include "fio-stl.h"
```

A non-cryptographic pseudo-random generator plus RiskyHash and Stable Hash. The PRNG is seeded from system jitter (`getrusage`, clocks) and hashed with RiskyHash. It is faster and more random than typical `rand()` implementations, but **do not use it for cryptography** — use `fio_rand_bytes_secure` for that.

RiskyHash is an ephemeral, non-cryptographic hash family that may change between releases. Stable Hash is frozen and safe for persistent data. RiskyHash 256/512 and HMAC variants are also provided.

### Pseudo-Random Functions

#### `fio_rand64`

```c
SFUNC uint64_t fio_rand64(void);
```

Returns 64 pseudo-random bits.

#### `fio_rand128`

```c
SFUNC fio_u128 fio_rand128(void);
```

Returns 128 pseudo-random bits as a `fio_u128`.

#### `fio_rand_bytes`

```c
SFUNC void fio_rand_bytes(void *target, size_t len);
```

Fills `target` with `len` pseudo-random bytes.

#### `fio_rand_bytes_secure`

```c
SFUNC int fio_rand_bytes_secure(void *target, size_t len);
```

Fills `target` with `len` cryptographically secure random bytes from the system CSPRNG (`arc4random_buf` on BSD/macOS, `/dev/urandom` fallback elsewhere). Returns `0` on success, `-1` on failure.

Use this for keys, nonces, and anything security-sensitive.

#### `fio_rand_reseed`

```c
SFUNC void fio_rand_reseed(void);
```

Rotates the PRNG state with fresh entropy. Call after `fork` to avoid duplicated sequences.

### Risky / Stable Hash

#### `FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH`

```c
#define FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH 0
```

When set to `1`, `fio_risky_hash` redirects to `fio_stable_hash`.

#### `fio_risky_hash`

```c
SFUNC uint64_t fio_risky_hash(const void *buf, size_t len, uint64_t seed);
```

64-bit RiskyHash v.3. Ephemeral — hash values may change in future releases. Good for hash-map keys, not for storage.

#### `fio_risky_ptr`

```c
FIO_IFUNC uint64_t fio_risky_ptr(void *ptr);
```

Fast entropy mix for pointer values.

#### `fio_risky_num`

```c
FIO_IFUNC uint64_t fio_risky_num(uint64_t number, uint64_t seed);
```

Fast entropy mix for integer values.

#### `fio_stable_hash`

```c
SFUNC uint64_t fio_stable_hash(const void *data, size_t len, uint64_t seed);
```

64-bit Stable Hash. Frozen after 1.0; safe for on-disk or cross-system use.

#### `fio_stable_hash128`

```c
SFUNC void fio_stable_hash128(void *restrict dest,
                              const void *restrict data,
                              size_t len,
                              uint64_t seed);
```

Writes a 128-bit Stable Hash into `dest` (16 bytes).

#### `fio_risky256`

```c
SFUNC fio_u256 fio_risky256(const void *data, uint64_t len);
```

256-bit RiskyHash. Based on the A3 (Zero-Copy ILP) design: two 512-bit states, 128 bytes per iteration, multiply-fold with cross-lane mixing. Returns a `fio_u256`.

#### `fio_risky512`

```c
SFUNC fio_u512 fio_risky512(const void *data, uint64_t len);
```

512-bit RiskyHash. The first 256 bits are identical to `fio_risky256`; the rest come from an extra squeeze round.

#### `fio_risky256_hmac`

```c
SFUNC fio_u256 fio_risky256_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len);
```

RFC 2104 HMAC using `fio_risky256` as the hash, 64-byte block size. Keys longer than 64 bytes are hashed first. Securely zeros intermediates.

**Note:** the underlying hash is non-cryptographic; standard HMAC security proofs do not apply. Use Blake2 for cryptographic HMAC.

#### `fio_risky512_hmac`

```c
SFUNC fio_u512 fio_risky512_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len);
```

Same construction as `fio_risky256_hmac`, but with `fio_risky512` and a 64-byte digest.

### Example

```c
#define FIO_RAND
#include "fio-stl.h"

int main(void) {
  printf("rand64: %016llX\n", (unsigned long long)fio_rand64());

  uint8_t key[32];
  fio_rand_bytes_secure(key, sizeof(key));

  uint64_t h = fio_stable_hash("hello", 5, 0);
  printf("stable: %016llX\n", (unsigned long long)h);

  fio_u256 r = fio_risky256("hello", 5);
  printf("risky256: %016llX...\n",
         (unsigned long long)r.u64[0]);
  return 0;
}
```

------------------------------------------------------------
