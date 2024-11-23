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

/** Writes `len` bytes of psedo-random bits to the target buffer. */
SFUNC void fio_rand_bytes(void *target, size_t len);

/** Feeds up to 1023 bytes of entropy to the random state. */
IFUNC void fio_rand_feed2seed(void *buf_, size_t len);

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
Possibly `extern` Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

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
  uint64_t const prime[4] = {FIO_U32_HASH_PRIME0,
                             FIO_U32_HASH_PRIME1,
                             FIO_U32_HASH_PRIME2,
                             FIO_U32_HASH_PRIME3};

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

#if FIO_OS_POSIX ||                                                            \
    (__has_include("sys/resource.h") && __has_include("sys/time.h"))
#include <sys/resource.h>
#include <sys/time.h>
#endif

static volatile uint64_t fio___rand_state[4]; /* random state */
static volatile size_t fio___rand_counter;    /* seed counter */
/* feeds random data to the algorithm through this 256 bit feed. */
static volatile uint64_t fio___rand_buffer[4] = {0x9c65875be1fce7b9ULL,
                                                 0x7cc568e838f6a40d,
                                                 0x4bb8d885a0fe47d5,
                                                 0x95561f0927ad7ecd};

IFUNC void fio_rand_feed2seed(void *buf_, size_t len) {
  len &= 1023;
  uint8_t *buf = (uint8_t *)buf_;
  uint8_t offset = (fio___rand_counter & 3);
  uint64_t tmp = 0;
  for (size_t i = 0; i < (len >> 3); ++i) {
    tmp = fio_buf2u64u(buf);
    fio___rand_buffer[(offset++ & 3)] ^= tmp;
    buf += 8;
  }
  if ((len & 7)) {
    tmp = 0;
    fio_memcpy7x(&tmp, buf, len);
    fio___rand_buffer[(offset++ & 3)] ^= tmp;
  }
}

SFUNC void fio_rand_reseed(void) {
  const size_t jitter_samples = 16 | (fio___rand_state[0] & 15);
#if defined(RUSAGE_SELF)
  {
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    fio___rand_state[0] ^=
        fio_risky_hash(&rusage, sizeof(rusage), fio___rand_state[0]);
  }
#endif
  for (size_t i = 0; i < jitter_samples; ++i) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t clk =
        (uint64_t)((t.tv_sec << 30) + (int64_t)t.tv_nsec) + fio___rand_counter;
    fio___rand_state[0] ^= fio_risky_num(clk, fio___rand_state[0] + i);
    fio___rand_state[1] ^= fio_risky_num(clk, fio___rand_state[1] + i);
  }
  {
    uint64_t tmp[2];
    tmp[0] = fio_risky_num(fio___rand_buffer[0], fio___rand_state[0]) +
             fio_risky_num(fio___rand_buffer[1], fio___rand_state[1]);
    tmp[1] = fio_risky_num(fio___rand_buffer[2], fio___rand_state[0]) +
             fio_risky_num(fio___rand_buffer[3], fio___rand_state[1]);
    fio___rand_state[2] ^= tmp[0];
    fio___rand_state[3] ^= tmp[1];
  }
  fio___rand_buffer[0] = fio_lrot64(fio___rand_buffer[0], 31);
  fio___rand_buffer[1] = fio_lrot64(fio___rand_buffer[1], 29);
  fio___rand_buffer[2] ^= fio___rand_buffer[0];
  fio___rand_buffer[3] ^= fio___rand_buffer[1];
  fio___rand_counter += jitter_samples;
}

/* tested for randomness using code from: http://xoshiro.di.unimi.it/hwd.php */
SFUNC uint64_t fio_rand64(void) {
  /* modeled after xoroshiro128+, by David Blackman and Sebastiano Vigna */
  uint64_t r = 0;
  if (!((fio___rand_counter++) & (((size_t)1 << 12) - 1))) {
    /* re-seed state every 524,288 requests / 2^19-1 attempts  */
    fio_rand_reseed();
  }
  const uint64_t s0[] = {fio___rand_state[0],
                         fio___rand_state[1],
                         fio___rand_state[2],
                         fio___rand_state[3]}; /* load to registers */
  uint64_t s1[4];
  {
    const uint64_t mulp[] = {0x37701261ED6C16C7ULL,
                             0x764DBBB75F3B3E0DULL,
                             ~(0x37701261ED6C16C7ULL),
                             ~(0x764DBBB75F3B3E0DULL)}; /* load to registers */
    const uint64_t addc[] = {fio___rand_counter, 0, fio___rand_counter, 0};
    for (size_t i = 0; i < 4; ++i) {
      s1[i] = fio_lrot64(s0[i], 33);
      s1[i] += addc[i];
      s1[i] *= mulp[i];
      s1[i] += s0[i];
    }
  }
  for (size_t i = 0; i < 4; ++i) { /* store to memory */
    fio___rand_state[i] = s1[i];
  }
  {
    uint8_t rotc[] = {31, 29, 27, 30};
    for (size_t i = 0; i < 4; ++i) {
      s1[i] = fio_lrot64(s1[i], rotc[i]);
      r += s1[i];
    }
  }
  return r;
}

/* copies 64 bits of randomness (8 bytes) repeatedly. */
SFUNC void fio_rand_bytes(void *data_, size_t len) {
  if (!data_ || !len)
    return;
  uint8_t *data = (uint8_t *)data_;
  for (unsigned i = 31; i < len; i += 32) {
    uint64_t rv[4] = {fio_rand64(), fio_rand64(), fio_rand64(), fio_rand64()};
    fio_memcpy32(data, rv);
    data += 32;
  }
  if (len & 31) {
    uint64_t rv[4] = {fio_rand64(), fio_rand64(), fio_rand64(), fio_rand64()};
    fio_memcpy31x(data, rv, len);
  }
}
/* *****************************************************************************
Random - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_RAND */
#undef FIO_RAND
