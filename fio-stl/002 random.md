# Pseudo-Random and Hashing

```c
#define FIO_RAND
#include "fio-stl.h"
```

A non-cryptographic pseudo-random generator plus RiskyHash and Stable Hash. The PRNG is seeded from system jitter (`getrusage`, clocks) and hashed with RiskyHash. It is faster and more random than typical `rand()` implementations, but **do not use it for cryptography** — use `fio_rand_bytes_secure` for that.

RiskyHash is an ephemeral, non-cryptographic hash family that may change between releases. Stable Hash is frozen and safe for persistent data. RiskyHash 256/512 come with keyed and HMAC variants.

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

Fills `target` with `len` cryptographically secure random bytes from the system CSPRNG: `arc4random_buf` on BSD/macOS, `BCryptGenRandom` on Windows, `getrandom` on Linux, and `/dev/urandom` as the fallback (also used when `getrandom` is unavailable or blocked). Returns `0` on success, `-1` on failure (no partial success; never falls back to the non-cryptographic PRNG). A `len` of `0` succeeds; a `NULL` `target` with `len > 0` fails.

**Windows linking**: MSVC links `bcrypt.lib` automatically (`#pragma comment`); MinGW / GNU-driver builds must link `-lbcrypt`.

Use this for keys, nonces, and anything security-sensitive.

#### `fio_rand_reseed`

```c
SFUNC void fio_rand_reseed(void);
```

Rotates the PRNG state with fresh entropy. The PRNG also reseeds itself periodically.

#### `fio_rand_on_fork`

```c
SFUNC void fio_rand_on_fork(void *is_null);
```

Reseeds the PRNG in a forked child, so it doesn't repeat the parent's sequence. The `FIO_CALL_IN_CHILD` state callback calls it automatically; call it yourself only when forking without facil.io's fork helpers.

#### `fio_rand_reset`

```c
SFUNC void fio_rand_reset(void);
```

Resets the PRNG state to its default seed. Intended for tests; never use it for anything security-sensitive.

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

256-bit RiskyHash. Two 512-bit lanes, 128 bytes per iteration, multiply-fold rounds with round constants and cross-lane mixing. Each 64-byte block is absorbed with feed-forward (`v = round(v ^ block) + v`), so a chosen block can't erase earlier input. Returns a `fio_u256`.

The hash has no seed: anyone can compute it, so it doesn't protect against chosen inputs (for example, hash flooding). Use `fio_risky256_keyed` with a secret key for attacker-influenced data. Outputs may change between versions; use Stable Hash for stored values.

#### `fio_risky512`

```c
SFUNC fio_u512 fio_risky512(const void *data, uint64_t len);
```

512-bit RiskyHash. The first 256 bits are identical to `fio_risky256`; the rest come from an extra squeeze round.

#### `fio_risky256_keyed`

```c
SFUNC fio_u256 fio_risky256_keyed(const void *key,
                                  uint64_t key_len,
                                  const void *data,
                                  uint64_t len);
```

Keyed 256-bit RiskyHash. The key seeds all 16 state words (both lanes), together with the key length and the data length. Keys of up to 64 bytes are used as-is; longer keys are first hashed with `fio_risky512`. Single pass: it costs about one `fio_risky256` call plus one round.

Use a key with at least 128 bits of secret entropy. Fit for fast keyed hashing and key derivation where the output stays private; it is not a vetted cryptographic primitive.

#### `fio_risky512_keyed`

```c
SFUNC fio_u512 fio_risky512_keyed(const void *key,
                                  uint64_t key_len,
                                  const void *data,
                                  uint64_t len);
```

Keyed 512-bit RiskyHash. The first 256 bits are identical to `fio_risky256_keyed` for the same key and data.

#### `fio_risky256_hmac`

```c
SFUNC fio_u256 fio_risky256_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len);
```

Keyed MAC with a 32-byte tag, built as a nested keyed hash:

```
tag = Keyed(key, outer, Keyed(key, inner, msg))
```

`inner` and `outer` are domain separation values, so the tag differs from `fio_risky256_keyed`. This is not RFC 2104 HMAC: both passes key both lanes, and the inner digest is never exposed. Key material, state and the inner digest are wiped before returning.

**Note:** the underlying hash is non-cryptographic and unvetted. Use Blake2 or Poly1305 when a cryptographic MAC is required.

#### `fio_risky512_hmac`

```c
SFUNC fio_u512 fio_risky512_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len);
```

Same construction as `fio_risky256_hmac`, with 512-bit inner and outer digests (64-byte tag).

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
