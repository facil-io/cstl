/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_SHA2               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                    SHA 2
                        SHA-256 / SHA-512 and variations



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_SHA2) && !defined(H___FIO_SHA2___H)
#define H___FIO_SHA2___H
/* *****************************************************************************
SHA 2 API
***************************************************************************** */

/** Streaming SHA-256 type. */
typedef struct {
  fio_u256 hash;
  fio_u512 cache;
  uint64_t total_len;
} fio_sha256_s;

/** A simple, non streaming, implementation of the SHA-256 hashing algorithm. */
FIO_IFUNC fio_u256 fio_sha256(const void *data, uint64_t len);

/** initializes a fio_u256 so the hash can consume streaming data. */
FIO_IFUNC fio_sha256_s fio_sha256_init(void);
/** Feed data into the hash */
SFUNC void fio_sha256_consume(fio_sha256_s *h, const void *data, uint64_t len);
/** finalizes a fio_u256 with the SHA 256 hash. */
SFUNC fio_u256 fio_sha256_finalize(fio_sha256_s *h);

/** Streaming SHA-512 type. */
typedef struct {
  fio_u512 hash;
  fio_u1024 cache;
  uint64_t total_len;
} fio_sha512_s;

/** A simple, non streaming, implementation of the SHA-512 hashing algorithm. */
FIO_IFUNC fio_u512 fio_sha512(const void *data, uint64_t len);

/** initializes a fio_u512 so the hash can consume streaming data. */
FIO_IFUNC fio_sha512_s fio_sha512_init(void);
/** Feed data into the hash */
SFUNC void fio_sha512_consume(fio_sha512_s *h, const void *data, uint64_t len);
/** finalizes a fio_u512 with the SHA 512 hash. */
SFUNC fio_u512 fio_sha512_finalize(fio_sha512_s *h);

/* *****************************************************************************
Implementation - static / inline functions.
***************************************************************************** */

/** initializes a fio_u256 so the hash can be consumed. */
FIO_IFUNC fio_sha256_s fio_sha256_init(void) {
  fio_sha256_s h = {.hash.u32 = {0x6A09E667ULL,
                                 0xBB67AE85ULL,
                                 0x3C6EF372ULL,
                                 0xA54FF53AULL,
                                 0x510E527FULL,
                                 0x9B05688CULL,
                                 0x1F83D9ABULL,
                                 0x5BE0CD19ULL}};
  return h;
}

/** A simple, non streaming, implementation of the SHA-256 hashing algorithm. */
FIO_IFUNC fio_u256 fio_sha256(const void *data, uint64_t len) {
  fio_sha256_s h = fio_sha256_init();
  fio_sha256_consume(&h, data, len);
  return fio_sha256_finalize(&h);
}

/** initializes a fio_u256 so the hash can be consumed. */
FIO_IFUNC fio_sha512_s fio_sha512_init(void) {
  fio_sha512_s h = {.hash.u64 = {0ULL}}; /* TODO! */
  return h;
}

/** A simple, non streaming, implementation of the SHA-256 hashing algorithm. */
FIO_IFUNC fio_u512 fio_sha512(const void *data, uint64_t len) {
  fio_sha512_s h = fio_sha512_init();
  fio_sha512_consume(&h, data, len);
  return fio_sha512_finalize(&h);
}

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Implementation - SHA-256
***************************************************************************** */

FIO_IFUNC void fio___sha256_round(fio_u256 *h, const uint8_t *block) {
  const uint32_t sha256_consts[64] = {
      0x428A2F98ULL, 0x71374491ULL, 0xB5C0FBCFULL, 0xE9B5DBA5ULL, 0x3956C25BULL,
      0x59F111F1ULL, 0x923F82A4ULL, 0xAB1C5ED5ULL, 0xD807AA98ULL, 0x12835B01ULL,
      0x243185BEULL, 0x550C7DC3ULL, 0x72BE5D74ULL, 0x80DEB1FEULL, 0x9BDC06A7ULL,
      0xC19BF174ULL, 0xE49B69C1ULL, 0xEFBE4786ULL, 0x0FC19DC6ULL, 0x240CA1CCULL,
      0x2DE92C6FULL, 0x4A7484AAULL, 0x5CB0A9DCULL, 0x76F988DAULL, 0x983E5152ULL,
      0xA831C66DULL, 0xB00327C8ULL, 0xBF597FC7ULL, 0xC6E00BF3ULL, 0xD5A79147ULL,
      0x06CA6351ULL, 0x14292967ULL, 0x27B70A85ULL, 0x2E1B2138ULL, 0x4D2C6DFCULL,
      0x53380D13ULL, 0x650A7354ULL, 0x766A0ABBULL, 0x81C2C92EULL, 0x92722C85ULL,
      0xA2BFE8A1ULL, 0xA81A664BULL, 0xC24B8B70ULL, 0xC76C51A3ULL, 0xD192E819ULL,
      0xD6990624ULL, 0xF40E3585ULL, 0x106AA070ULL, 0x19A4C116ULL, 0x1E376C08ULL,
      0x2748774CULL, 0x34B0BCB5ULL, 0x391C0CB3ULL, 0x4ED8AA4AULL, 0x5B9CCA4FULL,
      0x682E6FF3ULL, 0x748F82EEULL, 0x78A5636FULL, 0x84C87814ULL, 0x8CC70208ULL,
      0x90BEFFFAULL, 0xA4506CEBULL, 0xBEF9A3F7ULL, 0xC67178F2ULL};

  uint32_t v[8];
  for (size_t i = 0; i < 8; ++i) {
    v[i] = h->u32[i];
  }
  /* read data as an array of 16 big endian 32 bit integers. */
  uint32_t w[16] FIO_ALIGN(16);
  fio_memcpy64(w, block);
  for (size_t i = 0; i < 16; ++i) {
    w[i] = fio_lton32(w[i]); /* no-op on big endien systems */
  }

#define FIO___SHA256_ROUND_INNER_COMMON()                                      \
  uint32_t t2 =                                                                \
      ((v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2])) +                        \
      (fio_rrot32(v[0], 2) ^ fio_rrot32(v[0], 13) ^ fio_rrot32(v[0], 22));     \
  fio_u32x8_reshuffle(v, 7, 0, 1, 2, 3, 4, 5, 6);                              \
  v[4] += t1;                                                                  \
  v[0] = t1 + t2;

  for (size_t i = 0; i < 16; ++i) {
    const uint32_t t1 =
        v[7] + sha256_consts[i] + w[i] + ((v[4] & v[5]) ^ ((~v[4]) & v[6])) +
        (fio_rrot32(v[4], 6) ^ fio_rrot32(v[4], 11) ^ fio_rrot32(v[4], 25));
    FIO___SHA256_ROUND_INNER_COMMON();
  }
  for (size_t i = 0; i < 48; ++i) { /* expand block */
    w[(i & 15)] =
        (fio_rrot32(w[((i + 14) & 15)], 17) ^
         fio_rrot32(w[((i + 14) & 15)], 19) ^ (w[((i + 14) & 15)] >> 10)) +
        w[((i + 9) & 15)] + w[(i & 15)] +
        (fio_rrot32(w[((i + 1) & 15)], 7) ^ fio_rrot32(w[((i + 1) & 15)], 18) ^
         (w[((i + 1) & 15)] >> 3));
    const uint32_t t1 =
        v[7] + sha256_consts[i + 16] + w[(i & 15)] +
        ((v[4] & v[5]) ^ ((~v[4]) & v[6])) +
        (fio_rrot32(v[4], 6) ^ fio_rrot32(v[4], 11) ^ fio_rrot32(v[4], 25));
    FIO___SHA256_ROUND_INNER_COMMON();
  }
  for (size_t i = 0; i < 8; ++i)
    h->u32[i] += v[i]; /* compress block with previous state */

#undef FIO___SHA256_ROUND_INNER_COMMON
}

/** consume data and feed it to hash. */
SFUNC void fio_sha256_consume(fio_sha256_s *h, const void *data, uint64_t len) {
  const uint8_t *r = (const uint8_t *)data;
  const size_t old_total = h->total_len;
  const size_t new_total = len + h->total_len;
  h->total_len = new_total;
  /* manage cache */
  if (old_total & 63) {
    const size_t offset = (old_total & 63);
    if (len + offset < 64) { /* not enough - copy to cache */
      fio_memcpy63x((h->cache.u8 + offset), r, len);
      return;
    }
    /* consume cache */
    const size_t byte2copy = 64UL - offset;
    fio_memcpy63x(h->cache.u8 + offset, r, byte2copy);
    fio___sha256_round(&h->hash, h->cache.u8);
    FIO_MEMSET(h->cache.u8, 0, 64);
    r += byte2copy;
    len -= byte2copy;
  }
  const uint8_t *end = r + (len & (~(uint64_t)63ULL));
  while ((uintptr_t)r < (uintptr_t)end) {
    fio___sha256_round(&h->hash, r);
    r += 64;
  }
  fio_memcpy63x(h->cache.u64, r, len);
}

SFUNC fio_u256 fio_sha256_finalize(fio_sha256_s *h) {
  if (h->total_len == ((uint64_t)0ULL - 1ULL))
    return h->hash;
  const size_t total = h->total_len;
  const size_t remainder = total & 63;
  h->cache.u8[remainder] = 0x80U; /* set the 1 bit at the left most position */
  if ((remainder) > 47) { /* make sure there's room to attach `total_len` */
    fio___sha256_round(&h->hash, h->cache.u8);
    FIO_MEMSET(h->cache.u8, 0, 64);
  }
  h->cache.u64[7] = fio_lton64((total << 3));
  fio___sha256_round(&h->hash, h->cache.u8);
  for (size_t i = 0; i < 8; ++i)
    h->hash.u32[i] = fio_ntol32(h->hash.u32[i]); /* back to big endien */
  h->total_len = ((uint64_t)0ULL - 1ULL);
  return h->hash;
}

/* *****************************************************************************
Implementation - SHA-512
***************************************************************************** */

FIO_IFUNC void fio___sha512_round(fio_u512 *h, const uint8_t *block) {
  /* TODO! */
  (void)h;
  (void)block;
}

/** Feed data into the hash */
SFUNC void fio_sha512_consume(fio_sha512_s *h, const void *data, uint64_t len) {
  (void)h;
  (void)data;
  (void)len;
  /* TODO! */

  const uint8_t *r = (const uint8_t *)data;
  const size_t old_total = h->total_len;
  const size_t new_total = len + h->total_len;
  h->total_len = new_total;
  /* manage cache */
  if (old_total & 127) {
    const size_t offset = (old_total & 127);
    if (len + offset < 128) { /* not enough - copy to cache */
      fio_memcpy127x((h->cache.u8 + offset), r, len);
      return;
    }
    /* consume cache */
    const size_t byte2copy = 128UL - offset;
    fio_memcpy127x(h->cache.u8 + offset, r, byte2copy);
    fio___sha512_round(&h->hash, h->cache.u8);
    FIO_MEMSET(h->cache.u8, 0, 128);
    r += byte2copy;
    len -= byte2copy;
  }
  const uint8_t *end = r + (len & (~(uint64_t)127ULL));
  while ((uintptr_t)r < (uintptr_t)end) {
    fio___sha512_round(&h->hash, r);
    r += 128;
  }
  fio_memcpy63x(h->cache.u64, r, len);
}

/** finalizes a fio_u512 with the SHA 512 hash. */
SFUNC fio_u512 fio_sha512_finalize(fio_sha512_s *h) {
  /* TODO! */
  if (h->total_len == ((uint64_t)0ULL - 1ULL))
    return h->hash;
  const size_t total = h->total_len;
  const size_t remainder = total & 127;
  h->cache.u8[remainder] = 0x80U; /* set the 1 bit at the left most position */
  if ((remainder) > 119) { /* make sure there's room to attach `total_len` */
    fio___sha512_round(&h->hash, h->cache.u8);
    FIO_MEMSET(h->cache.u8, 0, 128);
  }
  h->cache.u64[15] = fio_lton64((total << 3));
  fio___sha512_round(&h->hash, h->cache.u8);
  for (size_t i = 0; i < 8; ++i)
    h->hash.u64[i] = fio_ntol64(h->hash.u64[i]); /* back to big endien */
  h->total_len = ((uint64_t)0ULL - 1ULL);
  return h->hash;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_SHA2 */
#undef FIO_SHA2
