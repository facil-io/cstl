/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "005 math.h"               /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                        Risky Hash - a fast and simple hash




***************************************************************************** */
#if defined(FIO_RISKY_HASH) && !defined(H___FIO_RISKY_HASH_H)
#define H___FIO_RISKY_HASH_H
/* *****************************************************************************
Risky Hash - API
***************************************************************************** */

/** Computes a facil.io Stable Hash (will not be updated, even if broken). */
SFUNC uint64_t fio_stable_hash(const void *data, size_t len, uint64_t seed);

/** Computes a facil.io Stable Hash (will not be updated, even if broken). */
SFUNC void fio_stable_hash128(void *restrict dest,
                              const void *restrict data,
                              size_t len,
                              uint64_t seed);

/** Computes a facil.io Risky Hash (Risky v.3). */
SFUNC uint64_t fio_risky_hash(const void *buf, size_t len, uint64_t seed);

/** Adds bit of entropy to pointer values. Designed to be unsafe. */
FIO_IFUNC uint64_t fio_risky_ptr(void *ptr);

/**
 * Masks data using a Risky Hash and a counter mode nonce.
 *
 * Used for mitigating memory access attacks when storing "secret" information
 * in memory.
 *
 * Keep the nonce information in a different memory address then the secret. For
 * example, if the secret is on the stack, store the nonce on the heap or using
 * a static variable.
 *
 * Don't use the same nonce-secret combination for other data.
 *
 * This is NOT a cryptographically secure encryption. Even if the algorithm was
 * secure, it would provide no more then a 32bit level encryption, which isn't
 * strong enough for any cryptographic use-case.
 *
 * However, this could be used to mitigate memory probing attacks. Secrets
 * stored in the memory might remain accessible after the program exists or
 * through core dump information. By storing "secret" information masked in this
 * way, it mitigates the risk of secret information being recognized or
 * deciphered.
 */
IFUNC void fio_risky_mask(char *buf, size_t len, uint64_t key, uint64_t nonce);

/* *****************************************************************************
Risky Hash - Implementation

Note: I don't remember what information I used when designing this, but Risky
Hash is probably NOT cryptographically safe (though I wanted it to be).

Here's a few resources about hashes that might explain more:
- https://komodoplatform.com/cryptographic-hash-function/
- https://en.wikipedia.org/wiki/Avalanche_effect
- http://ticki.github.io/blog/designing-a-good-non-cryptographic-hash-function/

***************************************************************************** */

/* Risky Hash primes */
#define FIO_RISKY3_PRIME0 0x39664DEECA23D825ULL
#define FIO_RISKY3_PRIME1 0x48644F7B3959621FULL
#define FIO_RISKY3_PRIME2 0x613A19F5CB0D98D5ULL
#define FIO_RISKY3_PRIME3 0x84B56B93C869EA0FULL
#define FIO_RISKY3_PRIME4 0x8EE38D13E0D95A8DULL

/** Adds bit entropy to a pointer values. Designed to be unsafe. */
FIO_IFUNC uint64_t fio_risky_ptr(void *ptr) {
  uint64_t n = (uint64_t)(uintptr_t)ptr;
  n ^= (n + FIO_RISKY3_PRIME0) * FIO_RISKY3_PRIME1;
  n ^= fio_rrot64(n, 7);
  n ^= fio_rrot64(n, 13);
  n ^= fio_rrot64(n, 17);
  n ^= fio_rrot64(n, 31);
  return n;
}

#ifdef FIO_EXTERN_COMPLETE

/* Risky Hash initialization constants */
#define FIO_RISKY3_IV0 0x0000001000000001ULL
#define FIO_RISKY3_IV1 0x0000010000000010ULL
#define FIO_RISKY3_IV2 0x0000100000000100ULL
#define FIO_RISKY3_IV3 0x0001000000001000ULL
/* read u64 in little endian */
#define FIO_RISKY_BUF2U64 fio_buf2u64_little

/*  Computes a facil.io Risky Hash. */
SFUNC uint64_t fio_risky_hash(const void *data_, size_t len, uint64_t seed) {
  uint64_t FIO_ALIGN(32)
      v[4] = {FIO_RISKY3_IV0, FIO_RISKY3_IV1, FIO_RISKY3_IV2, FIO_RISKY3_IV3};
  uint64_t FIO_ALIGN(32) w[4];
  const uint8_t *data = (const uint8_t *)data_;

#define FIO_RISKY3_ROUND64(vi, w_)                                             \
  w[vi] = w_;                                                                  \
  v[vi] += w[vi];                                                              \
  v[vi] = fio_lrot64(v[vi], 29);                                               \
  v[vi] += w[vi];                                                              \
  v[vi] *= FIO_RISKY3_PRIME##vi;

#define FIO_RISKY3_ROUND256(w0, w1, w2, w3)                                    \
  FIO_RISKY3_ROUND64(0, w0);                                                   \
  FIO_RISKY3_ROUND64(1, w1);                                                   \
  FIO_RISKY3_ROUND64(2, w2);                                                   \
  FIO_RISKY3_ROUND64(3, w3);

  if (seed) {
    /* process the seed as if it was a prepended 8 Byte string. */
    v[0] *= seed;
    v[1] *= seed;
    v[2] *= seed;
    v[3] *= seed;
    v[1] ^= seed;
    v[2] ^= seed;
    v[3] ^= seed;
  }

  for (size_t i = 31; i < len; i += 32) {
    /* 32 bytes / 256 bit access */
    FIO_RISKY3_ROUND256(FIO_RISKY_BUF2U64(data),
                        FIO_RISKY_BUF2U64(data + 8),
                        FIO_RISKY_BUF2U64(data + 16),
                        FIO_RISKY_BUF2U64(data + 24));
    data += 32;
  }
  switch (len & 24) { // clang-format off
  case 24: FIO_RISKY3_ROUND64(2, FIO_RISKY_BUF2U64(data + 16)); /* fall through */
  case 16: FIO_RISKY3_ROUND64(1, FIO_RISKY_BUF2U64(data + 8)); /* fall through */
  case 8:  FIO_RISKY3_ROUND64(0, FIO_RISKY_BUF2U64(data + 0));
    data += len & 24;
  } // clang-format on

  /* add offset information to padding */
  uint64_t tmp = ((uint64_t)len & 0xFF) << 56;
  /* leftover bytes */
  switch ((len & 7)) { // clang-format off
  case 7: tmp |= ((uint64_t)data[6]) << 48; /* fall through */
  case 6: tmp |= ((uint64_t)data[5]) << 40; /* fall through */
  case 5: tmp |= ((uint64_t)data[4]) << 32; /* fall through */
  case 4: tmp |= ((uint64_t)data[3]) << 24; /* fall through */
  case 3: tmp |= ((uint64_t)data[2]) << 16; /* fall through */
  case 2: tmp |= ((uint64_t)data[1]) << 8;  /* fall through */
  case 1: tmp |= ((uint64_t)data[0]);
    /* the last (now padded) byte's position */
    switch ((len & 24)) {
    case 24: FIO_RISKY3_ROUND64(3, tmp); break; /* offset 24 in 32 byte segment */
    case 16: FIO_RISKY3_ROUND64(2, tmp); break; /* offset 16 in 32 byte segment */
    case 8:  FIO_RISKY3_ROUND64(1, tmp); break; /* offset 8 in 32 byte segment */
    case 0:  FIO_RISKY3_ROUND64(0, tmp); break; /* offset 0 in 32 byte segment */
    }
  } // clang-format on

  /* irreversible avalanche... I think */
  uint64_t r = (len) ^ ((uint64_t)len << 36);
  r += fio_lrot64(v[0], 17) + fio_lrot64(v[1], 13) + fio_lrot64(v[2], 47) +
       fio_lrot64(v[3], 57);
  r += v[0] ^ v[1];
  r ^= fio_lrot64(r, 13);
  r += v[1] ^ v[2];
  r ^= fio_lrot64(r, 29);
  r += v[2] ^ v[3];
  r += fio_lrot64(r, 33);
  r += v[3] ^ v[0];
  r ^= fio_lrot64(r, 51);
  r ^= (r >> 29) * FIO_RISKY3_PRIME4;
  return r;
}

/**
 * Masks data using a Risky Hash and a counter mode nonce.
 */
IFUNC void fio_risky_mask(char *buf, size_t len, uint64_t key, uint64_t nonce) {
  { /* avoid zero nonce, make sure nonce is effective and odd */
    nonce |= 1;
    nonce *= 0xDB1DD478B9E93B1ULL;
    nonce ^= ((nonce << 24) | (nonce >> 40));
    nonce |= 1;
  }
  uint64_t hash = fio_risky_hash(&key, sizeof(key), nonce);
  fio_xmask2(buf, len, hash, nonce);
}

#undef FIO_STABLE_HASH_ROUND64
#undef FIO_STABLE_HASH_ROUND128

/* *****************************************************************************
Stable Hash (unlike Risky Hash, this can be used for non-ephemeral hashing)
***************************************************************************** */

/* Stable Hash primes */
#define FIO_STABLE_HASH_PRIME0 0x39664DEECA23D825ULL /* prime 32 set bits */
#define FIO_STABLE_HASH_PRIME1 0x48644F7B3959621FULL /* prime 32 set bits */
#define FIO_STABLE_HASH_PRIME2 0x613A19F5CB0D98D5ULL /* prime 32 set bits */
#define FIO_STABLE_HASH_PRIME3 0x84B56B93C869EA0FULL /* prime 32 set bits */
#define FIO_STABLE_HASH_PRIME4 0x8EE38D13E0D95A8DULL /* prime 32 set bits */

#define FIO_STABLE_HASH_ROUND_FULL()                                           \
  v[0] ^= w[0];                                                                \
  v[1] ^= w[1];                                                                \
  v[2] ^= w[2];                                                                \
  v[3] ^= w[3];                                                                \
  v[0] *= prime[0]; /* FIO_STABLE_HASH_PRIME0 */                               \
  v[1] *= prime[1]; /* FIO_STABLE_HASH_PRIME1 */                               \
  v[2] *= prime[2]; /* FIO_STABLE_HASH_PRIME2 */                               \
  v[3] *= prime[3]; /* FIO_STABLE_HASH_PRIME3 */                               \
  w[0] = fio_lrot64(w[0], 31) ^ seed;                                          \
  w[1] = fio_lrot64(w[1], 31) ^ seed;                                          \
  w[2] = fio_lrot64(w[2], 31) ^ seed;                                          \
  w[3] = fio_lrot64(w[3], 31) ^ seed;                                          \
  v[0] += w[0];                                                                \
  v[1] += w[1];                                                                \
  v[2] += w[2];                                                                \
  v[3] += w[3];

FIO_IFUNC void fio_stable_hash___inner(uint64_t *FIO_ALIGN(16) dest,
                                       const void *data_,
                                       size_t len,
                                       uint64_t seed) {
  const uint8_t *data = (const uint8_t *)data_;
  /* seed selection is constant time to avoid leaking seed data */
  seed += len;
  seed ^= fio_lrot64(seed, 47);
  seed ^= FIO_STABLE_HASH_PRIME0;
  seed |= (seed == 0);

  uint64_t FIO_ALIGN(16) w[4], FIO_ALIGN(16) v[4] = {seed, seed, seed, seed};
  const uint64_t FIO_ALIGN(16) prime[4] = {FIO_STABLE_HASH_PRIME0,
                                           FIO_STABLE_HASH_PRIME1,
                                           FIO_STABLE_HASH_PRIME2,
                                           FIO_STABLE_HASH_PRIME3};
  for (size_t i = 31; i < len; i += 32) {
    /* consumes 32 bytes (256 bits) each loop */
    w[0] = fio_buf2u64_little(data);
    w[1] = fio_buf2u64_little(data + 8);
    w[2] = fio_buf2u64_little(data + 16);
    w[3] = fio_buf2u64_little(data + 24);
    data += 32;
    seed ^= w[0] + w[1] + w[2] + w[3];
    FIO_STABLE_HASH_ROUND_FULL();
  }
  /* copy bytes to the word block in little endian */
  if ((len & 31)) {
    register const size_t word_tail_len = (len & 24);
    register uint64_t tmp = 0;
    w[0] = w[1] = w[2] = w[3] = 0;
    switch (word_tail_len) { // clang-format off
      case 24: w[2] = fio_buf2u64_little(data + 16); /* fall through */
      case 16: w[1] = fio_buf2u64_little(data + 8 ); /* fall through */
      case 8:  w[0] = fio_buf2u64_little(data);
      data += word_tail_len;
    }
    switch ((len & 7)) {
      case 7: tmp |= (((uint64_t)data[6] & 0xFF) << 48);    /* fall through */
      case 6: tmp |= (((uint64_t)data[5] & 0xFF) << 40);    /* fall through */
      case 5: tmp |= (((uint64_t)data[4] & 0xFF) << 32);    /* fall through */
      case 4: tmp |= (((uint64_t)data[3] & 0xFF) << 24);    /* fall through */
      case 3: tmp |= (((uint64_t)data[2] & 0xFF) << 16);    /* fall through */
      case 2: tmp |= (((uint64_t)data[1] & 0xFF) << 8);     /* fall through */
      case 1: tmp |= (((uint64_t)data[0] & 0xFF));
      switch(word_tail_len) {
        case 24: w[3] = tmp; break;
        case 16: w[2] = tmp; break;
        case 8:  w[1] = tmp; break;
        case 0:  w[0] = tmp; break;
      }
    } // clang-format on
    FIO_STABLE_HASH_ROUND_FULL();
  }
  /* inner vector avalanche */
  w[0] = v[0] * prime[0];
  w[1] = v[1] * prime[1];
  w[2] = v[2] * prime[2];
  w[3] = v[3] * prime[3];

  v[0] ^= fio_lrot64(w[0], 7);
  v[1] ^= fio_lrot64(w[1], 11);
  v[2] ^= fio_lrot64(w[2], 13);
  v[3] ^= fio_lrot64(w[3], 17);

  dest[0] = v[0];
  dest[1] = v[1];
  dest[2] = v[2];
  dest[3] = v[3];
}

/*  Computes a facil.io Stable Hash. */
SFUNC uint64_t fio_stable_hash(const void *data_, size_t len, uint64_t seed) {
  uint64_t r;
  uint64_t FIO_ALIGN(16) v[4];
  fio_stable_hash___inner(v, data_, len, seed);
  /* summing avalanche */
  r = v[0] + v[1] + v[2] + v[3];
  r ^= r >> 31;
  r *= FIO_STABLE_HASH_PRIME4;
  r ^= r >> 31;
  return r;
}

SFUNC void fio_stable_hash128(void *restrict dest,
                              const void *restrict data_,
                              size_t len,
                              uint64_t seed) {
  uint64_t FIO_ALIGN(16) v[4];
  fio_stable_hash___inner(v, data_, len, seed);
  uint64_t r[2];
  r[0] = v[0] + v[1] + v[2] + v[3];
  r[1] = v[0] ^ v[1] ^ v[2] ^ v[3];
  r[0] ^= r[0] >> 31;
  r[1] ^= r[1] >> 31;
  r[0] *= FIO_STABLE_HASH_PRIME4;
  r[1] *= FIO_STABLE_HASH_PRIME0;
  r[0] ^= r[0] >> 31;
  r[1] ^= r[1] >> 31;
  FIO_MEMCPY(dest, r, sizeof(r[0]) * 2);
}

#undef FIO_STABLE_HASH_AVA
#undef FIO_STABLE_HASH_ROUND_FULL

/* *****************************************************************************
Risky Hash - Cleanup
***************************************************************************** */
#undef FIO_RISKY_BUF2U64

#endif /* FIO_EXTERN_COMPLETE */
#endif
#undef FIO_RISKY_HASH

/* *****************************************************************************




                      Psedo-Random Generator Functions




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

/** Reseeds the random engin using system state (rusage / jitter). */
IFUNC void fio_rand_reseed(void);

/* *****************************************************************************
Random - Implementation
***************************************************************************** */

#ifdef FIO_EXTERN_COMPLETE

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
    tmp = FIO_NAME2(fio_buf, u64_local)(buf);
    fio___rand_buffer[(offset++ & 3)] ^= tmp;
    buf += 8;
  }
  switch (len & 7) { // clang-format off
  case 7: tmp <<= 8; tmp |= buf[6]; /* fall through */
  case 6: tmp <<= 8; tmp |= buf[5]; /* fall through */
  case 5: tmp <<= 8; tmp |= buf[4]; /* fall through */
  case 4: tmp <<= 8; tmp |= buf[3]; /* fall through */
  case 3: tmp <<= 8; tmp |= buf[2]; /* fall through */
  case 2: tmp <<= 8; tmp |= buf[1]; /* fall through */
  case 1: tmp <<= 8; tmp |= buf[1];
          fio___rand_buffer[(offset & 3)] ^= tmp;
  } // clang-format on
}

/* used here, defined later */
FIO_IFUNC int64_t fio_time_nano();

IFUNC void fio_rand_reseed(void) {
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
    uint64_t clk = (uint64_t)fio_time_nano();
    fio___rand_state[0] ^=
        fio_risky_hash(&clk, sizeof(clk), fio___rand_state[0] + i);
    clk = fio_time_nano();
    fio___rand_state[1] ^=
        fio_risky_hash(&clk,
                       sizeof(clk),
                       fio___rand_state[1] + fio___rand_counter);
  }
  fio___rand_state[2] ^=
      fio_risky_hash((void *)fio___rand_buffer,
                     sizeof(fio___rand_buffer),
                     fio___rand_counter + fio___rand_state[0]);
  fio___rand_state[3] ^= fio_risky_hash((void *)fio___rand_state,
                                        sizeof(fio___rand_state),
                                        fio___rand_state[1] + jitter_samples);
  fio___rand_buffer[0] = fio_lrot64(fio___rand_buffer[0], 31);
  fio___rand_buffer[1] = fio_lrot64(fio___rand_buffer[1], 29);
  fio___rand_buffer[2] ^= fio___rand_buffer[0];
  fio___rand_buffer[3] ^= fio___rand_buffer[1];
  fio___rand_counter += jitter_samples;
}

/* tested for randomness using code from: http://xoshiro.di.unimi.it/hwd.php */
SFUNC uint64_t fio_rand64(void) {
  /* modeled after xoroshiro128+, by David Blackman and Sebastiano Vigna */
  const uint64_t P[] = {0x37701261ED6C16C7ULL, 0x764DBBB75F3B3E0DULL};
  if (((fio___rand_counter++) & (((size_t)1 << 19) - 1)) == 0) {
    /* re-seed state every 524,288 requests / 2^19-1 attempts  */
    fio_rand_reseed();
  }
  fio___rand_state[0] +=
      (fio_lrot64(fio___rand_state[0], 33) + fio___rand_counter) * P[0];
  fio___rand_state[1] += fio_lrot64(fio___rand_state[1], 33) * P[1];
  fio___rand_state[2] +=
      (fio_lrot64(fio___rand_state[2], 33) + fio___rand_counter) * (~P[0]);
  fio___rand_state[3] += fio_lrot64(fio___rand_state[3], 33) * (~P[1]);
  return fio_lrot64(fio___rand_state[0], 31) +
         fio_lrot64(fio___rand_state[1], 29) +
         fio_lrot64(fio___rand_state[2], 27) +
         fio_lrot64(fio___rand_state[3], 30);
}

/* copies 64 bits of randomness (8 bytes) repeatedly. */
SFUNC void fio_rand_bytes(void *data_, size_t len) {
  if (!data_ || !len)
    return;
  uint8_t *data = (uint8_t *)data_;

  if (len < 8)
    goto small_random;

  if ((uintptr_t)data & 7) {
    /* align pointer to 64 bit word */
    size_t offset = 8 - ((uintptr_t)data & 7);
    fio_rand_bytes(data_, offset); /* perform small_random */
    data += offset;
    len -= offset;
  }

  /* 128 random bits at a time */
  for (size_t i = (len >> 4); i; --i) {
    uint64_t t0 = fio_rand64();
    uint64_t t1 = fio_rand64();
    FIO_NAME2(fio_u, buf64_local)(data, t0);
    FIO_NAME2(fio_u, buf64_local)(data + 8, t1);
    data += 16;
  }
  /* 64 random bits at tail */
  if ((len & 8)) {
    uint64_t t0 = fio_rand64();
    FIO_NAME2(fio_u, buf64_local)(data, t0);
  }

small_random:
  if ((len & 7)) {
    /* leftover bits */
    uint64_t tmp = fio_rand64();
    /* leftover bytes */
    switch ((len & 7)) {
    case 7:
      data[6] = (tmp >> 8) & 0xFF;
      /* fall through */
    case 6:
      data[5] = (tmp >> 16) & 0xFF;
      /* fall through */
    case 5:
      data[4] = (tmp >> 24) & 0xFF;
      /* fall through */
    case 4:
      data[3] = (tmp >> 32) & 0xFF;
      /* fall through */
    case 3:
      data[2] = (tmp >> 40) & 0xFF;
      /* fall through */
    case 2:
      data[1] = (tmp >> 48) & 0xFF;
      /* fall through */
    case 1:
      data[0] = (tmp >> 56) & 0xFF;
    }
  }
}

/* *****************************************************************************
Hashing speed test
***************************************************************************** */
#ifdef FIO_TEST_CSTL
#include <math.h>

typedef uintptr_t (*fio__hashing_func_fn)(char *, size_t);

FIO_SFUNC void fio_test_hash_function(fio__hashing_func_fn h,
                                      char *name,
                                      uint8_t size_log,
                                      uint8_t mem_alignment_offset,
                                      uint8_t fast) {
  /* test based on code from BearSSL with credit to Thomas Pornin */
  if (size_log >= 21 || ((sizeof(uint64_t) - 1) >> size_log)) {
    FIO_LOG_ERROR("fio_test_hash_function called with a log size too big.");
    return;
  }
  mem_alignment_offset &= 7;
  size_t const buffer_len = (1ULL << size_log) - mem_alignment_offset;
  uint64_t cycles_start_at = (1ULL << (16 + (fast * 2)));
  if (size_log < 13)
    cycles_start_at <<= (13 - size_log);
  else if (size_log > 13)
    cycles_start_at >>= (size_log - 13);

#ifdef DEBUG
  fprintf(stderr,
          "* Testing %s speed with %zu byte blocks"
          "(DEBUG mode detected - speed may be affected).\n",
          name,
          buffer_len);
#else
  fprintf(stderr,
          "* Testing %s speed with %zu byte blocks.\n",
          name,
          buffer_len);
#endif

  uint8_t *buffer_mem = (uint8_t *)
      FIO_MEM_REALLOC(NULL, 0, (buffer_len + mem_alignment_offset) + 64, 0);
  uint8_t *buffer = buffer_mem + mem_alignment_offset;

  memset(buffer, 'T', buffer_len);
  /* warmup */
  uint64_t hash = 0;
  for (size_t i = 0; i < 4; i++) {
    hash += h((char *)buffer, buffer_len);
    FIO_MEMCPY(buffer, &hash, sizeof(hash));
  }
  /* loop until test runs for more than 2 seconds */
  for (uint64_t cycles = cycles_start_at;;) {
    clock_t start, end;
    start = clock();
    for (size_t i = cycles; i > 0; i--) {
      hash += h((char *)buffer, buffer_len);
      FIO_COMPILER_GUARD;
    }
    end = clock();
    FIO_MEMCPY(buffer, &hash, sizeof(hash));
    if ((end - start) >= (2 * CLOCKS_PER_SEC) ||
        cycles >= ((uint64_t)1 << 62)) {
      fprintf(stderr,
              "\t%-40s %8.2f MB/s\n",
              name,
              (double)(buffer_len * cycles) /
                  (((end - start) * (1000000.0 / CLOCKS_PER_SEC))));
      break;
    }
    cycles <<= 1;
  }
  FIO_MEM_FREE(buffer_mem, (buffer_len + mem_alignment_offset) + 64);
}

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, risky_wrapper)(char *buf, size_t len) {
  return fio_risky_hash(buf, len, 1);
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, stable_wrapper)(char *buf, size_t len) {
  return fio_stable_hash(buf, len, 1);
}

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, risky_mask_wrapper)(char *buf,
                                                           size_t len) {
  fio_risky_mask(buf, len, 0, 0);
  return len;
}

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, xmask_wrapper)(char *buf, size_t len) {
  fio_xmask(buf, len, fio_rand64());
  return len;
}

FIO_SFUNC void FIO_NAME_TEST(stl, risky)(void) {
  for (int i = 0; i < 8; ++i) {
    char buf[128];
    uint64_t nonce = fio_rand64();
    const char *str = "this is a short text, to test risky masking";
    char *tmp = buf + i;
    FIO_MEMCPY(tmp, str, strlen(str));
    fio_risky_mask(tmp, strlen(str), (uint64_t)(uintptr_t)tmp, nonce);
    FIO_ASSERT(memcmp(tmp, str, strlen(str)), "Risky Hash masking failed");
    size_t err = 0;
    for (size_t b = 0; b < strlen(str); ++b) {
      FIO_ASSERT(tmp[b] != str[b] || (err < 2),
                 "Risky Hash masking didn't mask buf[%zu] on offset "
                 "%d (statistical deviation?)",
                 b,
                 i);
      err += (tmp[b] == str[b]);
    }
    fio_risky_mask(tmp, strlen(str), (uint64_t)(uintptr_t)tmp, nonce);
    FIO_ASSERT(!memcmp(tmp, str, strlen(str)), "Risky Hash masking RT failed");
  }
  const uint8_t alignment_test_offset = 0;
  if (alignment_test_offset)
    fprintf(stderr,
            "The following speed tests use a memory alignment offset of %d "
            "bytes.\n",
            (int)(alignment_test_offset & 7));
#if !DEBUG
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_wrapper),
                         (char *)"fio_risky_hash",
                         7,
                         0,
                         3);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_wrapper),
                         (char *)"fio_risky_hash",
                         13,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_wrapper),
                         (char *)"fio_risky_hash (unaligned)",
                         6,
                         3,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_wrapper),
                         (char *)"fio_risky_hash (unaligned)",
                         5,
                         3,
                         3);
  fio_test_hash_function(FIO_NAME_TEST(stl, stable_wrapper),
                         (char *)"fio_stable_hash (64 bit)",
                         7,
                         0,
                         3);
  fio_test_hash_function(FIO_NAME_TEST(stl, stable_wrapper),
                         (char *)"fio_stable_hash (64 bit)",
                         13,
                         0,
                         3);
  fio_test_hash_function(FIO_NAME_TEST(stl, stable_wrapper),
                         (char *)"fio_stable_hash (64 bit unaligned)",
                         6,
                         3,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, stable_wrapper),
                         (char *)"fio_stable_hash (64 bit unaligned)",
                         5,
                         3,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_mask_wrapper),
                         (char *)"fio_risky_mask (Risky XOR + counter)",
                         13,
                         0,
                         4);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_mask_wrapper),
                         (char *)"fio_risky_mask (unaligned)",
                         13,
                         1,
                         4);
  if (0) {
    fio_test_hash_function(FIO_NAME_TEST(stl, xmask_wrapper),
                           (char *)"fio_xmask (XOR, NO counter)",
                           13,
                           0,
                           4);
    fio_test_hash_function(FIO_NAME_TEST(stl, xmask_wrapper),
                           (char *)"fio_xmask (unaligned)",
                           13,
                           1,
                           4);
  }
#endif
}

FIO_SFUNC void FIO_NAME_TEST(stl, random_buffer)(uint64_t *stream,
                                                 size_t len,
                                                 const char *name,
                                                 size_t clk) {
  size_t totals[2] = {0};
  size_t freq[256] = {0};
  const size_t total_bits = (len * sizeof(*stream) * 8);
  uint64_t hemming = 0;
  /* collect data */
  for (size_t i = 1; i < len; i += 2) {
    hemming += fio_hemming_dist(stream[i], stream[i - 1]);
    for (size_t byte = 0; byte < (sizeof(*stream) << 1); ++byte) {
      uint8_t val = ((uint8_t *)(stream + (i - 1)))[byte];
      ++freq[val];
      for (int bit = 0; bit < 8; ++bit) {
        ++totals[(val >> bit) & 1];
      }
    }
  }
  hemming /= len;
  fprintf(stderr, "\n");
#if DEBUG
  fprintf(stderr,
          "\t- \x1B[1m%s\x1B[0m (%zu CPU cycles NOT OPTIMIZED):\n",
          name,
          clk);
#else
  fprintf(stderr, "\t- \x1B[1m%s\x1B[0m (%zu CPU cycles):\n", name, clk);
#endif
  fprintf(stderr,
          "\t  zeros / ones (bit frequency)\t%.05f\n",
          ((float)1.0 * totals[0]) / totals[1]);
  if (!(totals[0] < totals[1] + (total_bits / 20) &&
        totals[1] < totals[0] + (total_bits / 20)))
    FIO_LOG_ERROR("randomness isn't random?");
  fprintf(stderr,
          "\t  avarage hemming distance\t%zu (should be: 14-18)\n",
          (size_t)hemming);
  /* expect avarage hemming distance of 25% == 16 bits */
  if (!(hemming >= 14 && hemming <= 18))
    FIO_LOG_ERROR("randomness isn't random (hemming distance failed)?");
  /* test chi-square ... I think */
  if (len * sizeof(*stream) > 2560) {
    double n_r = (double)1.0 * ((len * sizeof(*stream)) / 256);
    double chi_square = 0;
    for (unsigned int i = 0; i < 256; ++i) {
      double f = freq[i] - n_r;
      chi_square += (f * f);
    }
    chi_square /= n_r;
    double chi_square_r_abs =
        (chi_square - 256 >= 0) ? chi_square - 256 : (256 - chi_square);
    fprintf(
        stderr,
        "\t  chi-sq. variation\t\t%.02lf - %s (expect <= %0.2lf)\n",
        chi_square_r_abs,
        ((chi_square_r_abs <= 2 * (sqrt(n_r)))
             ? "good"
             : ((chi_square_r_abs <= 3 * (sqrt(n_r))) ? "not amazing"
                                                      : "\x1B[1mBAD\x1B[0m")),
        2 * (sqrt(n_r)));
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, random)(void) {
  fprintf(stderr,
          "* Testing randomness "
          "- bit frequency / hemming distance / chi-square.\n");
  const size_t test_len = (FIO_TEST_REPEAT << 7);
  uint64_t *rs =
      (uint64_t *)FIO_MEM_REALLOC(NULL, 0, sizeof(*rs) * test_len, 0);
  clock_t start, end;
  FIO_ASSERT_ALLOC(rs);

  rand(); /* warmup */
  if (sizeof(int) < sizeof(uint64_t)) {
    start = clock();
    for (size_t i = 0; i < test_len; ++i) {
      rs[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
    end = clock();
  } else {
    start = clock();
    for (size_t i = 0; i < test_len; ++i) {
      rs[i] = (uint64_t)rand();
    }
    end = clock();
  }
  FIO_NAME_TEST(stl, random_buffer)
  (rs, test_len, "rand (system - naive, ignoring missing bits)", end - start);

  memset(rs, 0, sizeof(*rs) * test_len);
  {
    if (RAND_MAX == ~(uint64_t)0ULL) {
      /* RAND_MAX fills all bits */
      start = clock();
      for (size_t i = 0; i < test_len; ++i) {
        rs[i] = (uint64_t)rand();
      }
      end = clock();
    } else if (RAND_MAX >= (~(uint32_t)0UL)) {
      /* RAND_MAX fill at least 32 bits per call */
      uint32_t *rs_adjusted = (uint32_t *)rs;

      start = clock();
      for (size_t i = 0; i < (test_len << 1); ++i) {
        rs_adjusted[i] = (uint32_t)rand();
      }
      end = clock();
    } else if (RAND_MAX >= (~(uint16_t)0U)) {
      /* RAND_MAX fill at least 16 bits per call */
      uint16_t *rs_adjusted = (uint16_t *)rs;

      start = clock();
      for (size_t i = 0; i < (test_len << 2); ++i) {
        rs_adjusted[i] = (uint16_t)rand();
      }
      end = clock();
    } else {
      /* assume RAND_MAX fill at least 8 bits per call */
      uint8_t *rs_adjusted = (uint8_t *)rs;

      start = clock();
      for (size_t i = 0; i < (test_len << 2); ++i) {
        rs_adjusted[i] = (uint8_t)rand();
      }
      end = clock();
    }
    /* test RAND_MAX value */
    uint8_t rand_bits = 63;
    while (rand_bits) {
      if (RAND_MAX <= (~(0ULL)) >> rand_bits)
        break;
      --rand_bits;
    }
    rand_bits = 64 - rand_bits;

    char buffer[128] = {0};
    snprintf(buffer,
             128 - 14,
             "rand (system - fixed, testing %d random bits)",
             (int)rand_bits);
    FIO_NAME_TEST(stl, random_buffer)(rs, test_len, buffer, end - start);
  }

  memset(rs, 0, sizeof(*rs) * test_len);
  fio_rand64(); /* warmup */
  start = clock();
  for (size_t i = 0; i < test_len; ++i) {
    rs[i] = fio_rand64();
  }
  end = clock();
  FIO_NAME_TEST(stl, random_buffer)(rs, test_len, "fio_rand64", end - start);
  memset(rs, 0, sizeof(*rs) * test_len);
  start = clock();
  fio_rand_bytes(rs, test_len * sizeof(*rs));
  end = clock();
  FIO_NAME_TEST(stl, random_buffer)
  (rs, test_len, "fio_rand_bytes", end - start);

  fio_rand_feed2seed(rs, sizeof(*rs) * test_len);
  FIO_MEM_FREE(rs, sizeof(*rs) * test_len);
  fprintf(stderr, "\n");
#if DEBUG
  fprintf(stderr,
          "\t- to compare CPU cycles, test randomness with optimization.\n\n");
#endif /* DEBUG */
}
#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Random - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_RAND */
#undef FIO_RAND
