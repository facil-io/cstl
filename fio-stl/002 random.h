/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_RAND               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                      Psedo-Random Generator Functions
                    and friends - risky hash / stable hash



Copyright and License: see header file (000 copyright.h) / top of file
***************************************************************************** */
#if defined(FIO_RAND) && !defined(H___FIO_RAND_H)
#define H___FIO_RAND_H

/* *****************************************************************************
Random - API
***************************************************************************** */

/** Returns 64 psedo-random bits. Probably not cryptographically safe. */
SFUNC uint64_t fio_rand64(void);

/** Returns 64 psedo-random bits. Probably not cryptographically safe. */
SFUNC fio_u128 fio_rand128(void);

/** Writes `len` bytes of psedo-random bits to the target buffer. */
SFUNC void fio_rand_bytes(void *target, size_t len);

/**
 * Writes `len` bytes of cryptographically secure random data to `target`.
 *
 * Uses the system CSPRNG: arc4random_buf() on BSD/macOS, BCryptGenRandom() on
 * Windows (MSVC links bcrypt.lib automatically; MinGW must link `-lbcrypt`),
 * getrandom() on Linux, or /dev/urandom as fallback.
 *
 * Returns 0 on success, -1 on failure (never partial success; on failure the
 * `target` content MUST NOT be used). A NULL `target` with `len > 0` fails.
 *
 * IMPORTANT: Use this for security-sensitive operations like key generation.
 */
SFUNC int fio_rand_bytes_secure(void *target, size_t len);

/** Reseeds the random engine using system state (rusage / jitter). */
SFUNC void fio_rand_reseed(void);

/** Reseeds the random engine (call in forked children). */
SFUNC void fio_rand_on_fork(void *is_null);

/** Resets the random engine state to its default seed (testing only). */
SFUNC void fio_rand_reset(void);

/* *****************************************************************************
Risky / Stable Hash - API
***************************************************************************** */

/** Computes a facil.io Risky Hash (Risky v.3). */
SFUNC uint64_t fio_risky_hash(const void *buf, size_t len, uint64_t seed);

/** Adds a bit of entropy to pointer values. Designed to be unsafe. */
FIO_IFUNC uint64_t fio_risky_ptr(void *ptr);

/** Adds a bit of entropy to numeral values. Designed to be unsafe. */
FIO_IFUNC uint64_t fio_risky_num(uint64_t number, uint64_t seed);

/** Computes a facil.io Stable Hash (will not be updated, even if broken). */
SFUNC uint64_t fio_stable_hash(const void *data, size_t len, uint64_t seed);

/** Computes a facil.io Stable Hash (will not be updated, even if broken). */
SFUNC void fio_stable_hash128(void *restrict dest,
                              const void *restrict data,
                              size_t len,
                              uint64_t seed);

/**
 * Computes a 256-bit non-cryptographic hash (RiskyHash 256).
 *
 * 2-lane multiply-fold with cross-lane mixing and per-block feed-forward,
 * 128 bytes/iteration, single pass, constant time where 64x64 multiply is.
 *
 * Unseeded: anyone can compute it, so it offers no protection against chosen
 * inputs (e.g., hash flooding). Use fio_risky256_keyed with a secret key for
 * attacker-influenced data.
 *
 * Returns a `fio_u256` (32 bytes). Outputs may change between versions.
 */
SFUNC fio_u256 fio_risky256(const void *data, uint64_t len);

/**
 * Computes a 512-bit non-cryptographic hash (RiskyHash 512).
 *
 * The first 256 bits are identical to fio_risky256 (truncation-safe). The
 * second 256 bits come from an additional squeeze round.
 *
 * Returns a `fio_u512` (64 bytes).
 */
SFUNC fio_u512 fio_risky512(const void *data, uint64_t len);

/**
 * Computes a keyed 256-bit RiskyHash (fast keyed hash / KDF).
 *
 * All 16 state words (both lanes) are seeded from the key, its length and the
 * data length. Keys of up to 64 bytes are used as-is (zero padded, length
 * separated); longer keys are first hashed with fio_risky512. Single pass,
 * costs about one fio_risky256 call plus one (parallel) round.
 *
 * The key should hold at least 128 bits of secret entropy. Not a vetted
 * cryptographic primitive.
 */
SFUNC fio_u256 fio_risky256_keyed(const void *key,
                                  uint64_t key_len,
                                  const void *data,
                                  uint64_t len);

/**
 * Computes a keyed 512-bit RiskyHash.
 *
 * The first 256 bits are identical to fio_risky256_keyed (same key / data).
 */
SFUNC fio_u512 fio_risky512_keyed(const void *key,
                                  uint64_t key_len,
                                  const void *data,
                                  uint64_t len);

/**
 * Computes HMAC-RiskyHash-256 (nested keyed hash, 32-byte digest).
 *
 *     tag = Keyed(key, outer, Keyed(key, inner, msg))
 *
 * `inner` / `outer` are domain separation tweaks, so the tag differs from
 * fio_risky256_keyed. Not RFC 2104 (both lanes are keyed instead of padding a
 * key block); the inner digest is never exposed.
 *
 * NOTE: The underlying hash is non-cryptographic and unvetted. For
 * cryptographic MACs use blake2 or Poly1305.
 */
SFUNC fio_u256 fio_risky256_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len);

/**
 * Computes HMAC-RiskyHash-512 (nested keyed hash, 64-byte digest).
 *
 * Same construction as fio_risky256_hmac with 512-bit inner and outer digests.
 */
SFUNC fio_u512 fio_risky512_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len);

#define FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH 0
/* *****************************************************************************
Risky Hash - Implementation

Note: I don't remember what information I used when designing this, but Risky
Hash is probably NOT cryptographically safe (though I wish it was).

Here's a few resources about hashes that might explain more:
- https://komodoplatform.com/cryptographic-hash-function/
- https://en.wikipedia.org/wiki/Avalanche_effect
- http://ticki.github.io/blog/designing-a-good-non-cryptographic-hash-function/

***************************************************************************** */

/** Adds bit entropy to a pointer values. Designed to be unsafe. */
FIO_IFUNC uint64_t fio_risky_num(uint64_t n, uint64_t seed) {
  uint64_t h, tmp;
  h = fio_math_mulc64(n + seed, FIO_U64_HASH_PRIME0, &tmp);
  h += tmp;
  return h;
}

/** Adds bit entropy to a pointer values. Designed to be unsafe. */
FIO_IFUNC uint64_t fio_risky_ptr(void *ptr) {
  return fio_risky_num((uint64_t)(uintptr_t)ptr, 0);
}

/* *****************************************************************************
Risky Hash 256/512 - Internal Helpers (always-inline)

Design: 2-lane multiply-fold with cross-lane mixing. Each lane (v0, v1) is
8 x uint64_t (512 bits); 128-byte pairs feed both lanes for instruction-level
parallelism, the lanes merge only at finalization.

Per 64-byte block (Davies-Meyer style feed-forward):

    v = round(v ^ block) + v

The feed-forward keeps the previous state in the result, so a chosen block
can't erase earlier input (no "set the state, forget the history" collisions).

Round = 4 multiply-fold pairs + rotate-XOR cross mix. Per-word round constants
are added to words 0..3 alongside the fold (`a = a + c + lo(a * b)`, computed
in parallel with the multiply, so no extra latency): `round(0) != 0`, so there
is no all-zero fixed point. Every round uses the same constants (block
position is covered by the length seed and the feed-forward).

Absorption order (one pass, forward order):
1. Main loop (128-byte pairs) → v0, v1
2. Odd block (64 bytes if len & 64) → v1
3. Partial block (1-63 bytes at END, zero padded, length byte) → v0

Keyed variants seed all 16 state words from the key (see
fio___risky256_init_keyed); the unkeyed hash is seeded from the length only.
***************************************************************************** */

/**
 * Cross-lane mixing: snapshot all 8 lanes, then XOR rotated pairs into each
 * lane. Provides symmetric diffusion across multiply-fold pairs.
 */
FIO_IFUNC void fio___risky256_cross(uint64_t v[8]) {
  uint64_t s0 = v[0], s1 = v[1], s2 = v[2], s3 = v[3];
  uint64_t s4 = v[4], s5 = v[5], s6 = v[6], s7 = v[7];
  v[0] ^= fio_lrot64(s1, 17) ^ fio_lrot64(s6, 47);
  v[1] ^= fio_lrot64(s2, 29) ^ fio_lrot64(s7, 37);
  v[2] ^= fio_lrot64(s3, 41) ^ fio_lrot64(s4, 19);
  v[3] ^= fio_lrot64(s0, 53) ^ fio_lrot64(s5, 7);
  v[4] ^= fio_lrot64(s5, 13) ^ fio_lrot64(s2, 51);
  v[5] ^= fio_lrot64(s6, 23) ^ fio_lrot64(s3, 43);
  v[6] ^= fio_lrot64(s7, 11) ^ fio_lrot64(s0, 59);
  v[7] ^= fio_lrot64(s4, 3) ^ fio_lrot64(s1, 31);
}

/** Round: round constants, 4 multiply-fold pairs, cross-lane mixing. */
FIO_IFUNC void fio___risky256_round(uint64_t v[8]) {
  static const uint64_t c[4] = {
      FIO_U64_HASH_PRIME1,
      FIO_U64_HASH_PRIME2,
      FIO_U64_HASH_PRIME3,
      FIO_U64_HASH_PRIME4,
  };
  uint64_t t[4];
  for (size_t i = 0; i < 4; ++i) {
    uint64_t lo = fio_math_mulc64(v[i], v[i + 4], t + i);
    v[i] += c[i]; /* parallel to the multiply (off the critical path) */
    v[i] += lo;
    v[i + 4] += t[i];
  }
  fio___risky256_cross(v);
}

/** Absorbs one 64-byte block: `v = round(v ^ block) + v`. */
FIO_IFUNC void fio___risky256_block(uint64_t v[8], const uint8_t *data) {
  uint64_t s[8];
  for (size_t i = 0; i < 8; ++i) {
    s[i] = v[i];
    v[i] ^= fio_buf2u64_le(data + (i << 3));
  }
  fio___risky256_round(v);
  for (size_t i = 0; i < 8; ++i)
    v[i] += s[i];
}

/** Initializes both lanes from the message length (unkeyed seed). */
FIO_IFUNC void fio___risky256_init(uint64_t v0[8],
                                   uint64_t v1[8],
                                   uint64_t len) {
  static const uint64_t p[8] = {
      FIO_U64_HASH_PRIME1,
      FIO_U64_HASH_PRIME2,
      FIO_U64_HASH_PRIME3,
      FIO_U64_HASH_PRIME4,
      FIO_U64_HASH_PRIME5,
      FIO_U64_HASH_PRIME6,
      FIO_U64_HASH_PRIME7,
      FIO_U64_HASH_PRIME0,
  };
  static const uint64_t q[8] = {
      FIO_U64_HASH_PRIME8,
      FIO_U64_HASH_PRIME9,
      FIO_U64_HASH_PRIME10,
      FIO_U64_HASH_PRIME11,
      FIO_U64_HASH_PRIME12,
      FIO_U64_HASH_PRIME13,
      FIO_U64_HASH_PRIME14,
      FIO_U64_HASH_PRIME15,
  };
  /* Seed expansion from length */
  uint64_t seed = len;
  seed ^= fio_lrot64(seed, 47);
  for (size_t i = 0; i < 8; ++i) {
    v0[i] = seed + p[i];
    v1[i] = seed + q[i];
  }
}

/* *****************************************************************************
Possibly `extern` Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Risky Hash
***************************************************************************** */

/*  Computes a facil.io Risky Hash. */
SFUNC uint64_t fio_risky_hash(const void *data_, size_t len, uint64_t seed) {
#if FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH
  return fio_stable_hash(data_, len, seed);
#endif
#define FIO___RISKY_HASH_ROUND64()                                             \
  do {                                                                         \
    for (size_t i = 0; i < 8; ++i) /* use little endian? */                    \
      w[i] = fio_ltole64(w[i]);                                                \
    for (size_t i = 0; i < 8; ++i) { /* xor vector with input (words) */       \
      v[i] ^= w[i];                                                            \
    }                                                                          \
    for (size_t i = 0; i < 4; ++i) { /* MUL folding, adding high bits */       \
      v[i] += fio_math_mulc64(v[i], v[i + 4], w + i);                          \
      v[i + 4] += w[i];                                                        \
    }                                                                          \
  } while (0)
  /* Approach inspired by komihash, copyrighted: Aleksey Vaneev, MIT license */
  const uint8_t *data = (const uint8_t *)data_;
  uint64_t v[8] FIO_ALIGN(16), w[8] FIO_ALIGN(16) = {0};
  uint64_t const prime[8] FIO_ALIGN(16) = {
      FIO_U64_HASH_PRIME1,
      FIO_U64_HASH_PRIME2,
      FIO_U64_HASH_PRIME3,
      FIO_U64_HASH_PRIME4,
      FIO_U64_HASH_PRIME5,
      FIO_U64_HASH_PRIME6,
      FIO_U64_HASH_PRIME7,
      FIO_U64_HASH_PRIME0,
  };
  /* seed mixing is constant time to avoid leaking seed data */
  seed += len;
  seed ^= fio_lrot64(seed, 47);
  /* initialize vector with mixed secret */
  for (size_t i = 0; i < 8; ++i)
    v[i] = seed + prime[i];
  /* pad uneven head with zeros and consume (if any) */
  if ((len & 63)) {
    for (size_t i = 0; i < 8; ++i)
      w[i] = 0;
    fio_memcpy63x(w, data, len);
    data += (len & 63);
    ((uint8_t *)w)[63] = (uint8_t)(len & 63);
    FIO___RISKY_HASH_ROUND64();
  }
  /* consumes remaining 64 bytes (512 bits) blocks */
  for (size_t j = 63; j < len; j += 64) {
    for (size_t i = 0; i < 4; ++i)
      v[i] += prime[i]; /* mark each round, may double mark if(!(len & 63)) */
    fio_memcpy64(w, data);
    data += 64;
    FIO___RISKY_HASH_ROUND64();
  }

  w[4] = (v[0] ^ v[1]) + (v[1] ^ v[2]) + (v[2] ^ v[3]);
  w[5] = (v[4] + v[5]) ^ (v[5] + v[6]) ^ (v[6] + v[7]);
  w[6] = (w[0] + w[1]) ^ (w[1] + w[2]) ^ (w[2] + w[3]);
  v[0] = w[5] + fio_math_mulc64(w[4], w[6], v + 1);
  v[0] += v[1];
  return v[0];
#undef FIO___RISKY_HASH_ROUND64
}

/* *****************************************************************************
Risky Hash 256 / 512 — Constants
***************************************************************************** */

/** Finalization constants for lane v0. */
static const uint64_t fio___risky256_fin_primes[8] = {
    FIO_U64_HASH_PRIME8,
    FIO_U64_HASH_PRIME9,
    FIO_U64_HASH_PRIME10,
    FIO_U64_HASH_PRIME11,
    FIO_U64_HASH_PRIME12,
    FIO_U64_HASH_PRIME13,
    FIO_U64_HASH_PRIME14,
    FIO_U64_HASH_PRIME15,
};
/** Finalization constants for lane v1 (lanes never finalize alike). */
static const uint64_t fio___risky256_extra0[8] = {
    FIO_U64_HASH_PRIME16,
    FIO_U64_HASH_PRIME17,
    FIO_U64_HASH_PRIME18,
    FIO_U64_HASH_PRIME19,
    FIO_U64_HASH_PRIME20,
    FIO_U64_HASH_PRIME21,
    FIO_U64_HASH_PRIME22,
    FIO_U64_HASH_PRIME23,
};
/** Second squeeze constants (512-bit output). */
static const uint64_t fio___risky256_extra1[8] = {
    FIO_U64_HASH_PRIME24,
    FIO_U64_HASH_PRIME25,
    FIO_U64_HASH_PRIME26,
    FIO_U64_HASH_PRIME27,
    FIO_U64_HASH_PRIME28,
    FIO_U64_HASH_PRIME29,
    FIO_U64_HASH_PRIME30,
    FIO_U64_HASH_PRIME31,
};

/* Domain separation tweaks for the keyed seed. */
#define FIO___RISKY256_TWEAK_KEYED      0ULL
#define FIO___RISKY256_TWEAK_HMAC_INNER 0x3636363636363636ULL
#define FIO___RISKY256_TWEAK_HMAC_OUTER 0x5C5C5C5C5C5C5C5CULL

/* *****************************************************************************
Risky Hash 256 / 512 — Absorb / Finalize (internal)
***************************************************************************** */

/** Absorbs the whole message (main pairs → odd block → partial block). */
FIO_SFUNC void fio___risky256_absorb(uint64_t v0[8],
                                     uint64_t v1[8],
                                     const void *data_,
                                     uint64_t len) {
  const uint8_t *data = (const uint8_t *)data_;
  for (uint64_t i = len >> 7; i; --i) { /* 1. 128-byte pairs */
    fio___risky256_block(v0, data);
    fio___risky256_block(v1, data + 64);
    data += 128;
  }
  if (len & 64) { /* 2. odd 64-byte block */
    fio___risky256_block(v1, data);
    data += 64;
  }
  if (len & 63) { /* 3. partial block: zero padded, length in last byte */
    uint64_t w[8] FIO_ALIGN(16) = {0};
    fio_memcpy63x(w, data, len & 63);
    ((uint8_t *)w)[63] = (uint8_t)(len & 63);
    fio___risky256_block(v0, (const uint8_t *)w);
    fio_secure_zero(w, sizeof(w));
  }
}

/** Merges both lanes into v0 (4 rounds, lanes use distinct constants). */
FIO_IFUNC void fio___risky256_merge(uint64_t v0[8], uint64_t v1[8]) {
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio___risky256_fin_primes[i];
  for (size_t i = 0; i < 8; ++i)
    v1[i] ^= fio___risky256_extra0[i];
  fio___risky256_round(v0);
  fio___risky256_round(v1);
  for (size_t i = 0; i < 8; ++i)
    v0[i] += v1[i];
  fio___risky256_round(v0);
  fio___risky256_round(v0);
}

/** Finalizes to 256 bits (XOR-fold 8 words into 4) and wipes the state. */
FIO_IFUNC fio_u256 fio___risky256_finalize(uint64_t v0[8], uint64_t v1[8]) {
  fio_u256 r;
  fio___risky256_merge(v0, v1);
  for (size_t i = 0; i < 4; ++i)
    r.u64[i] = v0[i] ^ v0[i + 4];
  fio_secure_zero(v0, 64);
  fio_secure_zero(v1, 64);
  return r;
}

/** Finalizes to 512 bits (first 256 bits match the 256-bit finalization). */
FIO_IFUNC fio_u512 fio___risky512_finalize(uint64_t v0[8], uint64_t v1[8]) {
  fio_u512 r;
  fio___risky256_merge(v0, v1);
  for (size_t i = 0; i < 4; ++i)
    r.u64[i] = v0[i] ^ v0[i + 4];
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio___risky256_extra1[i];
  fio___risky256_round(v0);
  for (size_t i = 0; i < 4; ++i)
    r.u64[i + 4] = v0[i] ^ v0[i + 4];
  fio_secure_zero(v0, 64);
  fio_secure_zero(v1, 64);
  return r;
}

/**
 * Key schedule, step 1: key words `k` (keys up to 64 bytes are zero padded,
 * longer keys are replaced by their fio_risky512 digest).
 */
FIO_SFUNC void fio___risky256_key(uint64_t k[8],
                                  const void *key,
                                  uint64_t key_len) {
  for (size_t i = 0; i < 8; ++i)
    k[i] = 0;
  if (key_len > 64) {
    fio_u512 h = fio_risky512(key, key_len);
    for (size_t i = 0; i < 8; ++i)
      k[i] = h.u64[i];
    fio_secure_zero(&h, sizeof(h));
    return;
  }
  if (key_len)
    FIO_MEMCPY(k, key, (size_t)key_len);
  for (size_t i = 0; i < 8; ++i)
    k[i] = fio_ltole64(k[i]);
}

/**
 * Key schedule, step 2: seeds all 16 state words.
 *
 *     v0[i] = init0(len)[i] ^ k[i]
 *     v1[i] = (init1(len)[i] + {key_len, tweak, 0...}[i]) ^ k[i ^ 4]
 *     v1 = round(v1)
 *
 * Before the v1 round, (k, key_len, tweak) map injectively to the state (v0
 * fixes k, then v1 fixes key_len and tweak), so keys are length separated.
 * The v1 round makes the lanes nonlinearly related; it runs in parallel with
 * v0's first block, so keying adds little latency.
 */
FIO_IFUNC void fio___risky256_init_keyed(uint64_t v0[8],
                                         uint64_t v1[8],
                                         const uint64_t k[8],
                                         uint64_t key_len,
                                         uint64_t len,
                                         uint64_t tweak) {
  fio___risky256_init(v0, v1, len);
  v1[0] += key_len;
  v1[1] += tweak;
  for (size_t i = 0; i < 8; ++i) {
    v0[i] ^= k[i];
    v1[i] ^= k[i ^ 4];
  }
  fio___risky256_round(v1);
}

/* *****************************************************************************
Risky Hash 256 / 512 — Public API
***************************************************************************** */

/** Computes a 256-bit non-cryptographic hash (RiskyHash 256). */
SFUNC fio_u256 fio_risky256(const void *data_, uint64_t len) {
  uint64_t v0[8] FIO_ALIGN(16), v1[8] FIO_ALIGN(16);
  fio___risky256_init(v0, v1, len);
  fio___risky256_absorb(v0, v1, data_, len);
  return fio___risky256_finalize(v0, v1);
}

/** Computes a 512-bit non-cryptographic hash (RiskyHash 512). */
SFUNC fio_u512 fio_risky512(const void *data_, uint64_t len) {
  uint64_t v0[8] FIO_ALIGN(16), v1[8] FIO_ALIGN(16);
  fio___risky256_init(v0, v1, len);
  fio___risky256_absorb(v0, v1, data_, len);
  return fio___risky512_finalize(v0, v1);
}

/** Computes a keyed 256-bit RiskyHash (all 16 state words key-seeded). */
SFUNC fio_u256 fio_risky256_keyed(const void *key,
                                  uint64_t key_len,
                                  const void *data,
                                  uint64_t len) {
  uint64_t k[8] FIO_ALIGN(16), v0[8] FIO_ALIGN(16), v1[8] FIO_ALIGN(16);
  fio___risky256_key(k, key, key_len);
  fio___risky256_init_keyed(v0,
                            v1,
                            k,
                            key_len,
                            len,
                            FIO___RISKY256_TWEAK_KEYED);
  fio_secure_zero(k, sizeof(k));
  fio___risky256_absorb(v0, v1, data, len);
  return fio___risky256_finalize(v0, v1);
}

/** Computes a keyed 512-bit RiskyHash (all 16 state words key-seeded). */
SFUNC fio_u512 fio_risky512_keyed(const void *key,
                                  uint64_t key_len,
                                  const void *data,
                                  uint64_t len) {
  uint64_t k[8] FIO_ALIGN(16), v0[8] FIO_ALIGN(16), v1[8] FIO_ALIGN(16);
  fio___risky256_key(k, key, key_len);
  fio___risky256_init_keyed(v0,
                            v1,
                            k,
                            key_len,
                            len,
                            FIO___RISKY256_TWEAK_KEYED);
  fio_secure_zero(k, sizeof(k));
  fio___risky256_absorb(v0, v1, data, len);
  return fio___risky512_finalize(v0, v1);
}

/* *****************************************************************************
Risky Hash 256 / 512 — HMAC (nested keyed hash, allocation-free)

    inner = Keyed(K, tweak = 0x36.., msg)
    tag   = Keyed(K, tweak = 0x5C.., inner)

Both lanes are keyed in both passes; the inner digest is never exposed.
***************************************************************************** */

/** HMAC-RiskyHash-256 (nested keyed RiskyHash, 32-byte digest). */
SFUNC fio_u256 fio_risky256_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len) {
  uint64_t k[8] FIO_ALIGN(16), v0[8] FIO_ALIGN(16), v1[8] FIO_ALIGN(16);
  fio_u256 inner;
  fio___risky256_key(k, key, key_len);
  fio___risky256_init_keyed(v0,
                            v1,
                            k,
                            key_len,
                            msg_len,
                            FIO___RISKY256_TWEAK_HMAC_INNER);
  fio___risky256_absorb(v0, v1, msg, msg_len);
  inner = fio___risky256_finalize(v0, v1);
  fio___risky256_init_keyed(v0,
                            v1,
                            k,
                            key_len,
                            sizeof(inner),
                            FIO___RISKY256_TWEAK_HMAC_OUTER);
  fio_secure_zero(k, sizeof(k));
  fio___risky256_absorb(v0, v1, inner.u8, sizeof(inner));
  fio_secure_zero(&inner, sizeof(inner));
  return fio___risky256_finalize(v0, v1);
}

/** HMAC-RiskyHash-512 (nested keyed RiskyHash, 64-byte digest). */
SFUNC fio_u512 fio_risky512_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len) {
  uint64_t k[8] FIO_ALIGN(16), v0[8] FIO_ALIGN(16), v1[8] FIO_ALIGN(16);
  fio_u512 inner;
  fio___risky256_key(k, key, key_len);
  fio___risky256_init_keyed(v0,
                            v1,
                            k,
                            key_len,
                            msg_len,
                            FIO___RISKY256_TWEAK_HMAC_INNER);
  fio___risky256_absorb(v0, v1, msg, msg_len);
  inner = fio___risky512_finalize(v0, v1);
  fio___risky256_init_keyed(v0,
                            v1,
                            k,
                            key_len,
                            sizeof(inner),
                            FIO___RISKY256_TWEAK_HMAC_OUTER);
  fio_secure_zero(k, sizeof(k));
  fio___risky256_absorb(v0, v1, inner.u8, sizeof(inner));
  fio_secure_zero(&inner, sizeof(inner));
  return fio___risky512_finalize(v0, v1);
}

#undef FIO___RISKY256_TWEAK_KEYED
#undef FIO___RISKY256_TWEAK_HMAC_INNER
#undef FIO___RISKY256_TWEAK_HMAC_OUTER

/* *****************************************************************************
Stable Hash (unlike Risky Hash, this can be used for non-ephemeral hashing)
***************************************************************************** */
#define FIO_STABLE_HASH_ROUND_WORD(i)                                          \
  v[i] += w[i];                                                                \
  v[i] += prime[i];                                                            \
  v[i] *= prime[i];                                                            \
  w[i] = fio_lrot64(w[i], 19);                                                 \
  v[i] += w[i] + seed;

FIO_IFUNC void fio_stable_hash___inner(uint64_t dest[4],
                                       const void *restrict data_,
                                       const size_t len,
                                       uint64_t seed) {
  const uint8_t *data = (const uint8_t *)data_;
  /* seed selection is constant time to avoid leaking seed data */
  seed += len;
  seed ^= fio_lrot64(seed, 47);
  seed = (seed << 1) + 1;
  uint64_t v[4] = {seed, seed, seed, seed};
  uint64_t const prime[4] = {0xC19F5985UL,
                             0x8D567931UL,
                             0x9C178B17UL,
                             0xA4B842DFUL};

  for (size_t j = 31; j < len; j += 32) {
    /* consumes 32 bytes (256 bits) each loop */
    uint64_t w[4];
    for (size_t i = 0; i < 4; ++i) {
      w[i] = fio_ltole64(fio_buf2u64u(data));
      data += 8;
      FIO_STABLE_HASH_ROUND_WORD(i);
    }
  }
  { /* pad with zeros (even if %32 == 0) and add len to last word */
    uint64_t w[4] = {0};
    fio_memcpy31x(w, data, len); /* copies `len & 31` bytes */
    for (size_t i = 0; i < 4; ++i)
      w[i] = fio_ltole64(w[i]);
    w[3] += len;
    for (size_t i = 0; i < 4; ++i) {
      FIO_STABLE_HASH_ROUND_WORD(i);
    }
  }
  for (size_t i = 0; i < 4; ++i) {
    dest[i] = v[i];
  }
}

/* Computes a facil.io Stable Hash. */
SFUNC uint64_t fio_stable_hash(const void *data_, size_t len, uint64_t seed) {
  uint64_t r;
  uint64_t v[4];
  fio_stable_hash___inner(v, data_, len, seed);
  /* summing & avalanche */
  r = v[0] + v[1] + v[2] + v[3];
  for (size_t i = 0; i < 4; ++i)
    v[i] = fio_bswap64(v[i]);
  r ^= fio_lrot64(r, 5);
  r += v[0] ^ v[1];
  r ^= fio_lrot64(r, 27);
  r += v[1] ^ v[2];
  r ^= fio_lrot64(r, 49);
  r += v[2] ^ v[3];
  r ^= (r >> 29) * FIO_U64_HASH_PRIME0;
  r ^= fio_lrot64(r, 29);
  return r;
}

SFUNC void fio_stable_hash128(void *restrict dest,
                              const void *restrict data_,
                              size_t len,
                              uint64_t seed) {

  uint64_t v[4];
  fio_stable_hash___inner(v, data_, len, seed);
  uint64_t r[2];
  uint64_t prime[2] = {FIO_U64_HASH_PRIME0, FIO_U64_HASH_PRIME1};
  r[0] = v[0] + v[1] + v[2] + v[3];
  r[1] = v[0] ^ v[1] ^ v[2] ^ v[3];
  for (size_t i = 0; i < 4; ++i)
    v[i] = fio_bswap64(v[i]);
  for (size_t i = 0; i < 2; ++i) {
    r[i] ^= fio_lrot64(r[i], 5);
    r[i] += v[0] ^ v[1];
    r[i] ^= fio_lrot64(r[i], 27);
    r[i] += v[1] ^ v[2];
    r[i] ^= fio_lrot64(r[i], 49);
    r[i] += v[2] ^ v[3];
    r[i] ^= (r[i] >> 29) * prime[i];
    r[i] ^= fio_lrot64(r[i], 29);
  }
  fio_memcpy16(dest, r);
}

#undef FIO_STABLE_HASH_ROUND_WORD
/* *****************************************************************************
Random - Implementation
***************************************************************************** */

#if FIO_OS_POSIX || (__has_include("sys/resource.h") &&                        \
                     __has_include("sys/time.h") && __has_include("fcntl.h"))
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif

#if FIO_OS_WIN && !defined(__CYGWIN__)
/* BCryptGenRandom backs fio_rand_bytes_secure. MSVC links bcrypt.lib through
 * the pragma; MinGW / clang (GNU driver) users must link `-lbcrypt`. */
#include <bcrypt.h>
#if _MSC_VER
#pragma comment(lib, "bcrypt.lib")
#endif
#elif defined(__linux__) && __has_include(<sys/random.h>)
#include <sys/random.h>
#define FIO___RAND_HAS_GETRANDOM 1
#endif

/* The fio_rand64 implementation. */
FIO_DEFINE_RANDOM128_FN(SFUNC, fio_rand, 11, 0)

/**
 * Cryptographically secure random bytes using the system CSPRNG.
 *
 * Backends: arc4random_buf (BSD/macOS), BCryptGenRandom (Windows; MinGW must
 * link `-lbcrypt`), getrandom (Linux) and /dev/urandom (fallback).
 *
 * Returns 0 on success, -1 on failure (`target` content is then undefined and
 * MUST NOT be used). Never falls back to a non-cryptographic generator.
 */
SFUNC int fio_rand_bytes_secure(void *target, size_t len) {
  if (!len)
    return 0;
  if (!target)
    return -1;

#if (defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) ||     \
     defined(__NetBSD__) || defined(__DragonFly__))
  /* BSD/macOS: use arc4random_buf (always succeeds, CSPRNG) */
  arc4random_buf(target, len);
  return 0;
#elif FIO_OS_WIN && !defined(__CYGWIN__)
  /* Windows: system-preferred CSPRNG; ULONG is 32 bit, so chunk large requests
   */
  uint8_t *buf = (uint8_t *)target;
  while (len) {
    ULONG chunk =
        (len > (size_t)0x40000000UL) ? (ULONG)0x40000000UL : (ULONG)len;
    NTSTATUS st = BCryptGenRandom(NULL,
                                  (PUCHAR)buf,
                                  chunk,
                                  BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (st < 0) /* !BCRYPT_SUCCESS(st) */
      return -1;
    buf += chunk;
    len -= (size_t)chunk;
  }
  return 0;
#else
  uint8_t *buf = (uint8_t *)target;
#if FIO___RAND_HAS_GETRANDOM
  /* Linux: getrandom blocks only until the pool is first initialized and
   * needs no file descriptor. Fall back to /dev/urandom if the syscall is
   * unavailable (ENOSYS) or filtered (e.g., EPERM under seccomp). */
  while (len) {
    ssize_t got = getrandom(buf, len, 0);
    if (got < 0) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (got == 0)
      break;
    buf += got;
    len -= (size_t)got;
  }
  if (!len)
    return 0;
#endif /* FIO___RAND_HAS_GETRANDOM */
    /* Generic POSIX fallback: read from /dev/urandom */
#ifdef O_CLOEXEC
  int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
#else
  int fd = open("/dev/urandom", O_RDONLY);
#endif
  if (fd < 0)
    return -1;
  while (len > 0) {
    ssize_t got = read(fd, buf, len);
    if (got < 0) {
      if (errno == EINTR)
        continue;
      close(fd);
      return -1;
    }
    if (got == 0) {
      close(fd);
      return -1; /* unexpected EOF */
    }
    buf += got;
    len -= (size_t)got;
  }
  close(fd);
  return 0;
#endif
}
/* *****************************************************************************
Random - Cleanup
***************************************************************************** */
#undef FIO___RAND_HAS_GETRANDOM
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_RAND */
#undef FIO_RAND
