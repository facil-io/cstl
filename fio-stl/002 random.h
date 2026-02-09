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
 * Uses system CSPRNG: getrandom() on Linux, arc4random_buf() on BSD/macOS,
 * or /dev/urandom as fallback. Returns 0 on success, -1 on failure.
 *
 * IMPORTANT: Use this for security-sensitive operations like key generation.
 */
SFUNC int fio_rand_bytes_secure(void *target, size_t len);

/** Reseeds the random engine using system state (rusage / jitter). */
SFUNC void fio_rand_reseed(void);

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
 * Based on the A3 (Zero-Copy ILP) design: 2-state multiply-fold with
 * cross-lane mixing, 128 bytes/iteration, zero-copy XOR from input.
 *
 * Returns a `fio_u256` (32 bytes). Passes strict avalanche (50.0%),
 * collision, differential, and length independence tests.
 */
SFUNC fio_u256 fio_risky256(const void *data, uint64_t len);

/**
 * Computes a 512-bit non-cryptographic hash (RiskyHash 512).
 *
 * SHAKE-style extension of fio_risky256: the first 256 bits of the 512-bit
 * output are identical to fio_risky256 (truncation-safe). The second 256 bits
 * come from an additional squeeze round.
 *
 * Returns a `fio_u512` (64 bytes).
 */
SFUNC fio_u512 fio_risky512(const void *data, uint64_t len);

/**
 * Computes HMAC-RiskyHash-256 (RFC 2104 construction, 32-byte digest).
 *
 * Uses fio_risky256 as the underlying hash with a 64-byte block size.
 * If `key_len > 64`, the key is hashed first with fio_risky256.
 *
 * NOTE: The underlying hash is non-cryptographic, so standard HMAC
 * security proofs do not apply. For cryptographic HMAC, use blake2.
 */
SFUNC fio_u256 fio_risky256_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len);

/**
 * Computes HMAC-RiskyHash-512 (RFC 2104 construction, 64-byte digest).
 *
 * Uses fio_risky512 as the underlying hash with a 64-byte block size.
 * If `key_len > 64`, the key is hashed first with fio_risky512.
 *
 * NOTE: The underlying hash is non-cryptographic, so standard HMAC
 * security proofs do not apply. For cryptographic HMAC, use blake2.
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
  seed ^= fio_lrot64(seed, 47);
  seed += FIO_U64_HASH_PRIME0;
  seed = seed | 1;
  uint64_t h = n + seed;
  h += fio_lrot64(seed, 5);
  h += fio_bswap64(seed);
  h += fio_lrot64(h, 27);
  h += fio_lrot64(h, 49);
  return h;
}

/** Adds bit entropy to a pointer values. Designed to be unsafe. */
FIO_IFUNC uint64_t fio_risky_ptr(void *ptr) {
  return fio_risky_num((uint64_t)(uintptr_t)ptr, FIO_U64_HASH_PRIME9);
}

/* *****************************************************************************
Risky Hash 256/512 - Internal Helpers (always-inline)

Design: A3 (Zero-Copy ILP) — 2-state multiply-fold with cross-lane mixing.
Each state is 8 x uint64_t (512 bits). Two states (v0, v1) process 128 bytes
per iteration for instruction-level parallelism. Cross-lane mixing after each
round ensures full diffusion across all 8 multiply-fold pairs.

Streaming-Compatible Absorption Order:
1. Main loop (128-byte pairs) → v0, v1 alternating — processed FIRST
2. Odd block (64 bytes if total_len & 64) → v1 — processed SECOND
3. Partial block (0-63 bytes at END) → v0 — processed LAST

This order enables streaming: data is processed in forward order, with the
partial block coming from the logical END of the message.

Streaming API for HMAC:
- fio___risky256_init(v0, v1, total_len) — init with TOTAL message length
- fio___risky256_consume(state, data, len) — absorb streaming chunks
- fio___risky256_finish(state) — finalize and return fio_u256
- fio___risky512_finish(state) — finalize and return fio_u512
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

/** Multiply-fold round: 4 multiply-fold pairs + cross-lane mixing. */
FIO_IFUNC void fio___risky256_round(uint64_t v[8]) {
  uint64_t t[4];
  for (size_t i = 0; i < 4; ++i) {
    v[i] += fio_math_mulc64(v[i], v[i + 4], t + i);
    v[i + 4] += t[i];
  }
  fio___risky256_cross(v);
}

/**
 * Streaming state for RiskyHash 256/512.
 * Allows incremental absorption of data chunks.
 */
typedef struct {
  uint64_t v0[8];     /* First state vector */
  uint64_t v1[8];     /* Second state vector */
  uint64_t total_len; /* Total message length (must be known upfront) */
  uint64_t absorbed;  /* Bytes absorbed so far */
  uint8_t buf[128];   /* Buffer for partial data (up to 127 bytes) */
  uint8_t buf_len;    /* Bytes in buffer */
} fio___risky256_stream_s;

/** Initialize state with total message length (does seed expansion). */
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

/** Initialize streaming state with total message length. */
FIO_IFUNC void fio___risky256_stream_init(fio___risky256_stream_s *s,
                                          uint64_t total_len) {
  fio___risky256_init(s->v0, s->v1, total_len);
  s->total_len = total_len;
  s->absorbed = 0;
  s->buf_len = 0;
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

/** Prime constants shared by absorption and finalization. */
static const uint64_t fio___risky256_primes[8] = {
    FIO_U64_HASH_PRIME1,
    FIO_U64_HASH_PRIME2,
    FIO_U64_HASH_PRIME3,
    FIO_U64_HASH_PRIME4,
    FIO_U64_HASH_PRIME5,
    FIO_U64_HASH_PRIME6,
    FIO_U64_HASH_PRIME7,
    FIO_U64_HASH_PRIME0,
};
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

/* *****************************************************************************
Risky Hash 256 / 512 — Streaming Absorb (internal)

Absorption order (streaming-compatible):
1. Main loop (128-byte pairs) → v0, v1 alternating — FIRST
2. Odd block (64 bytes if total_len & 64) → v1 — SECOND
3. Partial block (0-63 bytes at END) → v0 — LAST

This order processes data in forward order, enabling true streaming.
The partial block contains the TAIL bytes of the message.
***************************************************************************** */

/** Absorb a 128-byte pair into v0 and v1. */
FIO_IFUNC void fio___risky256_absorb_pair(uint64_t v0[8],
                                          uint64_t v1[8],
                                          const uint8_t *data) {
  for (size_t i = 0; i < 4; ++i)
    v0[i] += fio___risky256_primes[i];
  for (size_t i = 0; i < 4; ++i)
    v1[i] += fio___risky256_primes[i];
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio_buf2u64_le(data + (i << 3));
  for (size_t i = 0; i < 8; ++i)
    v1[i] ^= fio_buf2u64_le(data + 64 + (i << 3));
  fio___risky256_round(v0);
  fio___risky256_round(v1);
}

/** Absorb a 64-byte odd block into v1. */
FIO_IFUNC void fio___risky256_absorb_odd(uint64_t v1[8], const uint8_t *data) {
  for (size_t i = 0; i < 4; ++i)
    v1[i] += fio___risky256_primes[i];
  for (size_t i = 0; i < 8; ++i)
    v1[i] ^= fio_buf2u64_le(data + (i << 3));
  fio___risky256_round(v1);
}

/** Absorb a partial block (0-63 bytes) into v0. */
FIO_IFUNC void fio___risky256_absorb_partial(uint64_t v0[8],
                                             const uint8_t *data,
                                             uint64_t partial_len) {
  uint64_t w[8] FIO_ALIGN(16);
  FIO_MEMSET(w, 0, 64);
  fio_memcpy63x(w, data, partial_len);
  ((uint8_t *)w)[63] = (uint8_t)partial_len;
  for (size_t i = 0; i < 8; ++i)
    w[i] = fio_ltole64(w[i]);
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= w[i];
  fio___risky256_round(v0);
}

/**
 * Streaming consume: absorb data chunks in forward order.
 * Call multiple times with consecutive chunks, then call finish.
 */
FIO_SFUNC void fio___risky256_consume(fio___risky256_stream_s *s,
                                      const void *data_,
                                      uint64_t len) {
  const uint8_t *data = (const uint8_t *)data_;
  uint64_t total = s->total_len;
  uint64_t main_end = (total >> 7) << 7; /* End of 128-byte pairs region */
  uint64_t odd_end = main_end + ((total & 64) ? 64 : 0); /* End of odd block */

  /* If we have buffered data, try to complete a block */
  if (s->buf_len > 0) {
    uint64_t need = 0;
    if (s->absorbed < main_end) {
      /* In main loop region: need 128 bytes total */
      need = 128 - s->buf_len;
      if (s->absorbed + 128 > main_end)
        need = main_end - s->absorbed - s->buf_len;
    } else if (s->absorbed < odd_end) {
      /* In odd block region: need 64 bytes total */
      need = 64 - s->buf_len;
    }
    /* Partial region: buffer until finish */

    if (need > 0 && len >= need) {
      FIO_MEMCPY(s->buf + s->buf_len, data, (size_t)need);
      data += need;
      len -= need;

      if (s->absorbed < main_end && s->buf_len + need == 128) {
        fio___risky256_absorb_pair(s->v0, s->v1, s->buf);
        s->absorbed += 128;
        s->buf_len = 0;
      } else if (s->absorbed >= main_end && s->absorbed < odd_end &&
                 s->buf_len + need == 64) {
        fio___risky256_absorb_odd(s->v1, s->buf);
        s->absorbed += 64;
        s->buf_len = 0;
      } else {
        s->buf_len += (uint8_t)need;
      }
    } else if (need > 0) {
      /* Not enough data to complete block, buffer it */
      FIO_MEMCPY(s->buf + s->buf_len, data, (size_t)len);
      s->buf_len += (uint8_t)len;
      return;
    }
  }

  /* Process full 128-byte pairs directly from input */
  while (s->absorbed + 128 <= main_end && len >= 128) {
    fio___risky256_absorb_pair(s->v0, s->v1, data);
    data += 128;
    len -= 128;
    s->absorbed += 128;
  }

  /* Process odd block if we're at that boundary */
  if (s->absorbed == main_end && (total & 64) && len >= 64) {
    fio___risky256_absorb_odd(s->v1, data);
    data += 64;
    len -= 64;
    s->absorbed += 64;
  }

  /* Buffer remaining data for partial block or incomplete block */
  if (len > 0) {
    FIO_MEMCPY(s->buf + s->buf_len, data, (size_t)len);
    s->buf_len += (uint8_t)len;
  }
}

/**
 * Streaming finish: absorb any remaining buffered data and finalize.
 * Returns 256-bit hash.
 */
FIO_SFUNC fio_u256 fio___risky256_finish(fio___risky256_stream_s *s);

/**
 * Streaming finish: absorb any remaining buffered data and finalize.
 * Returns 512-bit hash.
 */
FIO_SFUNC fio_u512 fio___risky512_finish(fio___risky256_stream_s *s);

/**
 * One-shot absorb: absorbs entire message into state.
 * Uses the new streaming-compatible order (main → odd → partial).
 */
FIO_SFUNC void fio___risky256_absorb(uint64_t v0[8],
                                     uint64_t v1[8],
                                     const void *data_,
                                     uint64_t len) {
  const uint8_t *data = (const uint8_t *)data_;
  uint64_t main_pairs = len >> 7; /* Number of 128-byte pairs */

  /* 1. Main loop: 128-byte pairs */
  for (uint64_t i = 0; i < main_pairs; ++i) {
    fio___risky256_absorb_pair(v0, v1, data);
    data += 128;
  }

  /* 2. Odd block: 64 bytes if (len & 64) */
  if (len & 64) {
    fio___risky256_absorb_odd(v1, data);
    data += 64;
  }

  /* 3. Partial block: 0-63 bytes at END */
  if (len & 63) {
    fio___risky256_absorb_partial(v0, data, len & 63);
  }
}

/**
 * Finalize state and return 256-bit hash.
 * Merges v1 into v0, applies finalization rounds, and squeezes output.
 */
FIO_SFUNC fio_u256 fio___risky256_finalize(uint64_t v0[8], uint64_t v1[8]) {
  /* Finalization: merge v1 into v0 */
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio___risky256_fin_primes[i];
  fio___risky256_round(v0);
  for (size_t i = 0; i < 8; ++i)
    v1[i] ^= fio___risky256_fin_primes[i];
  fio___risky256_round(v1);
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= v1[i];
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio___risky256_extra0[i];
  fio___risky256_round(v0);
  /* Squeeze: XOR-fold 8 lanes into 4 */
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio___risky256_primes[i];
  fio___risky256_round(v0);
  fio_u256 r;
  for (size_t i = 0; i < 4; ++i)
    r.u64[i] = v0[i] ^ v0[i + 4];
  return r;
}

/**
 * Finalize state and return 512-bit hash.
 * Same as fio___risky256_finalize but does 2 squeezes for 512 bits.
 */
FIO_SFUNC fio_u512 fio___risky512_finalize(uint64_t v0[8], uint64_t v1[8]) {
  /* Finalization: merge v1 into v0 */
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio___risky256_fin_primes[i];
  fio___risky256_round(v0);
  for (size_t i = 0; i < 8; ++i)
    v1[i] ^= fio___risky256_fin_primes[i];
  fio___risky256_round(v1);
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= v1[i];
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio___risky256_extra0[i];
  fio___risky256_round(v0);
  /* First squeeze: 256 bits */
  fio_u512 r;
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio___risky256_primes[i];
  fio___risky256_round(v0);
  for (size_t i = 0; i < 4; ++i)
    r.u64[i] = v0[i] ^ v0[i + 4];
  /* Second squeeze: additional 256 bits */
  for (size_t i = 0; i < 8; ++i)
    v0[i] ^= fio___risky256_extra1[i];
  fio___risky256_round(v0);
  for (size_t i = 0; i < 4; ++i)
    r.u64[i + 4] = v0[i] ^ v0[i + 4];
  return r;
}

/* *****************************************************************************
Risky Hash 256 / 512 — Streaming Finish (implementations)
***************************************************************************** */

/** Streaming finish: process buffered data and finalize to 256 bits. */
FIO_SFUNC fio_u256 fio___risky256_finish(fio___risky256_stream_s *s) {
  uint64_t total = s->total_len;
  uint64_t main_end = (total >> 7) << 7;
  uint64_t odd_end = main_end + ((total & 64) ? 64 : 0);

  /* Process any remaining buffered data */
  if (s->buf_len > 0) {
    if (s->absorbed < main_end) {
      /* Incomplete 128-byte pair — should not happen if total_len is correct */
      /* Pad with zeros and absorb as pair */
      FIO_MEMSET(s->buf + s->buf_len, 0, 128 - s->buf_len);
      fio___risky256_absorb_pair(s->v0, s->v1, s->buf);
      s->absorbed = main_end;
      s->buf_len = 0;
    }
    if (s->absorbed < odd_end && s->absorbed >= main_end) {
      /* Incomplete odd block — pad and absorb */
      if (s->buf_len < 64) {
        FIO_MEMSET(s->buf + s->buf_len, 0, 64 - s->buf_len);
      }
      fio___risky256_absorb_odd(s->v1, s->buf);
      s->absorbed = odd_end;
      s->buf_len = 0;
    }
    if (s->absorbed >= odd_end && s->buf_len > 0) {
      /* Partial block at end */
      fio___risky256_absorb_partial(s->v0, s->buf, s->buf_len);
      s->buf_len = 0;
    }
  }

  return fio___risky256_finalize(s->v0, s->v1);
}

/** Streaming finish: process buffered data and finalize to 512 bits. */
FIO_SFUNC fio_u512 fio___risky512_finish(fio___risky256_stream_s *s) {
  uint64_t total = s->total_len;
  uint64_t main_end = (total >> 7) << 7;
  uint64_t odd_end = main_end + ((total & 64) ? 64 : 0);

  /* Process any remaining buffered data */
  if (s->buf_len > 0) {
    if (s->absorbed < main_end) {
      /* Incomplete 128-byte pair — pad and absorb */
      FIO_MEMSET(s->buf + s->buf_len, 0, 128 - s->buf_len);
      fio___risky256_absorb_pair(s->v0, s->v1, s->buf);
      s->absorbed = main_end;
      s->buf_len = 0;
    }
    if (s->absorbed < odd_end && s->absorbed >= main_end) {
      /* Incomplete odd block — pad and absorb */
      if (s->buf_len < 64) {
        FIO_MEMSET(s->buf + s->buf_len, 0, 64 - s->buf_len);
      }
      fio___risky256_absorb_odd(s->v1, s->buf);
      s->absorbed = odd_end;
      s->buf_len = 0;
    }
    if (s->absorbed >= odd_end && s->buf_len > 0) {
      /* Partial block at end */
      fio___risky256_absorb_partial(s->v0, s->buf, s->buf_len);
      s->buf_len = 0;
    }
  }

  return fio___risky512_finalize(s->v0, s->v1);
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

/* *****************************************************************************
Risky Hash 256 / 512 — HMAC (RFC 2104 compliant, allocation-free)

Uses streaming API for the inner hash:
  Inner: H(K ^ ipad || msg) — streaming: feed k_ipad, then msg
  Outer: H(K ^ opad || inner_hash) — fixed 96/128 bytes on stack

Block size is 64 bytes. Keys > 64 bytes are hashed first.
***************************************************************************** */

/** HMAC-RiskyHash-256 (RFC 2104 compliant, 32-byte digest, allocation-free).
 *
 * Memory layout uses fio_u1024 (128 bytes) for zero-copy outer hash:
 *   buf.u64[0..7]  = k_padded (64 bytes) - key XOR'd with ipad/opad
 *   buf.u64[8..11] = inner hash result (32 bytes)
 * This gives contiguous k_opad||inner_hash for outer hash input.
 */
SFUNC fio_u256 fio_risky256_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len) {
  fio_u1024 buf = {0};
  uint64_t *k_padded = buf.u64;       /* bytes 0-63 */
  fio_u256 *inner_ptr = buf.u256 + 2; /* bytes 64-95 (buf.u64[8..11]) */

  /* Key preparation: truncate or hash if > 64 bytes */
  if (key_len > 64) {
    fio_u256 hk = fio_risky256(key, key_len);
    FIO_MEMCPY(k_padded, hk.u64, 32);
    fio_secure_zero(hk.u8, sizeof(hk));
  } else if (key_len) {
    FIO_MEMCPY(k_padded, key, (size_t)key_len);
  }

  /* Inner hash: H(K ^ ipad || msg) using streaming API */
  for (size_t i = 0; i < 8; ++i)
    k_padded[i] ^= 0x3636363636363636ULL;

  fio___risky256_stream_s state;
  fio___risky256_stream_init(&state, 64 + msg_len);
  fio___risky256_consume(&state, k_padded, 64);
  if (msg_len)
    fio___risky256_consume(&state, msg, msg_len);
  *inner_ptr =
      fio___risky256_finish(&state); /* write directly to buf[64..95] */

  /* Toggle ipad→opad: XOR with 0x6A (0x36 ^ 0x5C) */
  for (size_t i = 0; i < 8; ++i)
    k_padded[i] ^= 0x6A6A6A6A6A6A6A6AULL;

  /* Outer hash: H(K ^ opad || inner_hash) — contiguous 96 bytes in buf */
  fio_u256 result = fio_risky256(buf.u8, 96);

  /* Secure cleanup */
  fio_secure_zero(&state, sizeof(state));
  fio_secure_zero(buf.u8, 96);
  return result;
}

/** HMAC-RiskyHash-512 (RFC 2104 compliant, 64-byte digest, allocation-free).
 *
 * Memory layout uses fio_u1024 (128 bytes) for zero-copy outer hash:
 *   buf.u64[0..7]  = k_padded (64 bytes) - key XOR'd with ipad/opad
 *   buf.u64[8..15] = inner hash result (64 bytes)
 * This gives contiguous k_opad||inner_hash for outer hash input.
 */
SFUNC fio_u512 fio_risky512_hmac(const void *key,
                                 uint64_t key_len,
                                 const void *msg,
                                 uint64_t msg_len) {
  fio_u1024 buf = {0};
  uint64_t *k_padded = buf.u64;       /* bytes 0-63 */
  fio_u512 *inner_ptr = buf.u512 + 1; /* bytes 64-127 (buf.u64[8..15]) */

  /* Key preparation: truncate or hash if > 64 bytes */
  if (key_len > 64) {
    fio_u512 hk = fio_risky512(key, key_len);
    FIO_MEMCPY(k_padded, hk.u64, 64);
    fio_secure_zero(hk.u8, sizeof(hk));
  } else if (key_len) {
    FIO_MEMCPY(k_padded, key, (size_t)key_len);
  }

  /* Inner hash: H(K ^ ipad || msg) using streaming API */
  for (size_t i = 0; i < 8; ++i)
    k_padded[i] ^= 0x3636363636363636ULL;

  fio___risky256_stream_s state;
  fio___risky256_stream_init(&state, 64 + msg_len);
  fio___risky256_consume(&state, k_padded, 64);
  if (msg_len)
    fio___risky256_consume(&state, msg, msg_len);
  *inner_ptr =
      fio___risky512_finish(&state); /* write directly to buf[64..127] */

  /* Toggle ipad→opad: XOR with 0x6A (0x36 ^ 0x5C) */
  for (size_t i = 0; i < 8; ++i)
    k_padded[i] ^= 0x6A6A6A6A6A6A6A6AULL;

  /* Outer hash: H(K ^ opad || inner_hash) — contiguous 128 bytes in buf */
  fio_u512 result = fio_risky512(buf.u8, 128);

  /* Secure cleanup */
  fio_secure_zero(&state, sizeof(state));
  fio_secure_zero(buf.u8, 128);
  return result;
}

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

/* The fio_rand64 implementation. */
FIO_DEFINE_RANDOM128_FN(SFUNC, fio_rand, 11, 0)

/**
 * Cryptographically secure random bytes using system CSPRNG.
 * Returns 0 on success, -1 on failure.
 */
SFUNC int fio_rand_bytes_secure(void *target, size_t len) {
  if (!target || !len)
    return 0;

#if (defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) ||     \
     defined(__NetBSD__) || defined(__DragonFly__))
  /* BSD/macOS: use arc4random_buf (always succeeds, CSPRNG) */
  arc4random_buf(target, len);
  return 0;
#else
  /* Generic POSIX fallback: read from /dev/urandom */
  uint8_t *buf = (uint8_t *)target;
  int fd = open("/dev/urandom", O_RDONLY);
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
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_RAND */
#undef FIO_RAND
