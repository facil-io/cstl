/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_BLAKE2             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                  BLAKE2
                        BLAKE2b (64-bit) and BLAKE2s (32-bit)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_BLAKE2) && !defined(H___FIO_BLAKE2___H)
#define H___FIO_BLAKE2___H
/* *****************************************************************************
BLAKE2 API
***************************************************************************** */

/** Streaming BLAKE2b type (64-bit, up to 64-byte digest). */
typedef struct {
  uint64_t h[8];    /* state */
  uint64_t t[2];    /* total bytes processed (128-bit counter) */
  uint64_t f[2];    /* finalization flags */
  uint8_t buf[128]; /* input buffer */
  size_t buflen;    /* bytes in buffer */
  size_t outlen;    /* digest length */
} fio_blake2b_s;

/** Streaming BLAKE2s type (32-bit, up to 32-byte digest). */
typedef struct {
  uint32_t h[8];   /* state */
  uint32_t t[2];   /* total bytes processed (64-bit counter) */
  uint32_t f[2];   /* finalization flags */
  uint8_t buf[64]; /* input buffer */
  size_t buflen;   /* bytes in buffer */
  size_t outlen;   /* digest length */
} fio_blake2s_s;

/* *****************************************************************************
BLAKE2b API (64-bit optimized, up to 64-byte digest)
***************************************************************************** */

/**
 * A simple, non-streaming implementation of BLAKE2b.
 *
 * `out` must point to a buffer of at least `outlen` bytes.
 * `outlen` must be between 1 and 64 (default 64 if 0).
 * `key` and `keylen` are optional (set to NULL/0 for unkeyed hashing).
 */
SFUNC void fio_blake2b(void *restrict out,
                       size_t outlen,
                       const void *restrict data,
                       size_t len,
                       const void *restrict key,
                       size_t keylen);

/** Initialize a BLAKE2b streaming context. outlen: 1-64 (default 64). */
SFUNC fio_blake2b_s fio_blake2b_init(size_t outlen,
                                     const void *key,
                                     size_t keylen);

/** Feed data into BLAKE2b hash. */
SFUNC void fio_blake2b_consume(fio_blake2b_s *restrict h,
                               const void *restrict data,
                               size_t len);

/** Finalize BLAKE2b hash. Writes `h->outlen` bytes to `out`. */
SFUNC void fio_blake2b_finalize(fio_blake2b_s *restrict h, void *restrict out);

/* *****************************************************************************
BLAKE2s API (32-bit optimized, up to 32-byte digest)
***************************************************************************** */

/**
 * A simple, non-streaming implementation of BLAKE2s.
 *
 * `out` must point to a buffer of at least `outlen` bytes.
 * `outlen` must be between 1 and 32 (default 32 if 0).
 * `key` and `keylen` are optional (set to NULL/0 for unkeyed hashing).
 */
SFUNC void fio_blake2s(void *restrict out,
                       size_t outlen,
                       const void *restrict data,
                       size_t len,
                       const void *restrict key,
                       size_t keylen);

/** Initialize a BLAKE2s streaming context. outlen: 1-32 (default 32). */
SFUNC fio_blake2s_s fio_blake2s_init(size_t outlen,
                                     const void *key,
                                     size_t keylen);

/** Feed data into BLAKE2s hash. */
SFUNC void fio_blake2s_consume(fio_blake2s_s *restrict h,
                               const void *restrict data,
                               size_t len);

/** Finalize BLAKE2s hash. Writes `h->outlen` bytes to `out`. */
SFUNC void fio_blake2s_finalize(fio_blake2s_s *restrict h, void *restrict out);

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
BLAKE2b Implementation (64-bit)
***************************************************************************** */

/* BLAKE2b initialization vector */
static const uint64_t fio___blake2b_iv[8] = {0x6A09E667F3BCC908ULL,
                                             0xBB67AE8584CAA73BULL,
                                             0x3C6EF372FE94F82BULL,
                                             0xA54FF53A5F1D36F1ULL,
                                             0x510E527FADE682D1ULL,
                                             0x9B05688C2B3E6C1FULL,
                                             0x1F83D9ABFB41BD6BULL,
                                             0x5BE0CD19137E2179ULL};

/* BLAKE2b sigma permutation table */
static const uint8_t fio___blake2b_sigma[12][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
    {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
    {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
    {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
    {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
    {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
    {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
    {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
    {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}};

/* BLAKE2b G mixing function */
#define FIO___BLAKE2B_G(r, i, a, b, c, d, m)                                   \
  do {                                                                         \
    (a) += (b) + m[fio___blake2b_sigma[r][2 * (i)]];                           \
    (d) = fio_rrot64((d) ^ (a), 32);                                           \
    (c) += (d);                                                                \
    (b) = fio_rrot64((b) ^ (c), 24);                                           \
    (a) += (b) + m[fio___blake2b_sigma[r][2 * (i) + 1]];                       \
    (d) = fio_rrot64((d) ^ (a), 16);                                           \
    (c) += (d);                                                                \
    (b) = fio_rrot64((b) ^ (c), 63);                                           \
  } while (0)

/* BLAKE2b round function */
#define FIO___BLAKE2B_ROUND(r, v, m)                                           \
  do {                                                                         \
    FIO___BLAKE2B_G(r, 0, v[0], v[4], v[8], v[12], m);                         \
    FIO___BLAKE2B_G(r, 1, v[1], v[5], v[9], v[13], m);                         \
    FIO___BLAKE2B_G(r, 2, v[2], v[6], v[10], v[14], m);                        \
    FIO___BLAKE2B_G(r, 3, v[3], v[7], v[11], v[15], m);                        \
    FIO___BLAKE2B_G(r, 4, v[0], v[5], v[10], v[15], m);                        \
    FIO___BLAKE2B_G(r, 5, v[1], v[6], v[11], v[12], m);                        \
    FIO___BLAKE2B_G(r, 6, v[2], v[7], v[8], v[13], m);                         \
    FIO___BLAKE2B_G(r, 7, v[3], v[4], v[9], v[14], m);                         \
  } while (0)

/* BLAKE2b compression function */
FIO_IFUNC void fio___blake2b_compress(fio_blake2b_s *restrict h,
                                      const uint8_t *restrict block,
                                      int is_last) {
  uint64_t v[16] FIO_ALIGN(64);
  uint64_t m[16] FIO_ALIGN(64);

  /* Initialize working vector */
  for (size_t i = 0; i < 8; ++i) {
    v[i] = h->h[i];
    v[i + 8] = fio___blake2b_iv[i];
  }
  v[12] ^= h->t[0];
  v[13] ^= h->t[1];
  if (is_last)
    v[14] = ~v[14]; /* Invert finalization flag */

  /* Load message block (little-endian) */
  for (size_t i = 0; i < 16; ++i)
    m[i] = fio_buf2u64_le(block + i * 8);

  /* 12 rounds of mixing */
  FIO___BLAKE2B_ROUND(0, v, m);
  FIO___BLAKE2B_ROUND(1, v, m);
  FIO___BLAKE2B_ROUND(2, v, m);
  FIO___BLAKE2B_ROUND(3, v, m);
  FIO___BLAKE2B_ROUND(4, v, m);
  FIO___BLAKE2B_ROUND(5, v, m);
  FIO___BLAKE2B_ROUND(6, v, m);
  FIO___BLAKE2B_ROUND(7, v, m);
  FIO___BLAKE2B_ROUND(8, v, m);
  FIO___BLAKE2B_ROUND(9, v, m);
  FIO___BLAKE2B_ROUND(10, v, m);
  FIO___BLAKE2B_ROUND(11, v, m);

  /* Finalize state */
  for (size_t i = 0; i < 8; ++i)
    h->h[i] ^= v[i] ^ v[i + 8];
}

#undef FIO___BLAKE2B_G
#undef FIO___BLAKE2B_ROUND

/** Initialize a BLAKE2b streaming context. */
SFUNC fio_blake2b_s fio_blake2b_init(size_t outlen,
                                     const void *key,
                                     size_t keylen) {
  fio_blake2b_s h = {0};

  /* Validate parameters */
  if (outlen == 0)
    outlen = 64;
  if (outlen > 64)
    outlen = 64;
  if (keylen > 64)
    keylen = 64;

  h.outlen = outlen;

  /* Initialize state with IV */
  for (size_t i = 0; i < 8; ++i)
    h.h[i] = fio___blake2b_iv[i];

  /* XOR parameter block into state[0] */
  /* Parameter block: fanout=1, depth=1, leaf_len=0, node_offset=0,
   * node_depth=0, inner_len=0, reserved=0, salt=0, personal=0 */
  h.h[0] ^= 0x01010000ULL ^ ((uint64_t)keylen << 8) ^ (uint64_t)outlen;

  /* If keyed, pad key to 128 bytes and process as first block */
  if (keylen > 0 && key) {
    FIO_MEMSET(h.buf, 0, 128);
    FIO_MEMCPY(h.buf, key, keylen);
    h.buflen = 128;
  }

  return h;
}

/** Feed data into BLAKE2b hash. */
SFUNC void fio_blake2b_consume(fio_blake2b_s *restrict h,
                               const void *restrict data,
                               size_t len) {
  const uint8_t *p = (const uint8_t *)data;

  /* If we have buffered data, try to complete a block */
  if (h->buflen > 0) {
    size_t fill = 128 - h->buflen;
    if (len < fill) {
      FIO_MEMCPY(h->buf + h->buflen, p, len);
      h->buflen += len;
      return;
    }
    FIO_MEMCPY(h->buf + h->buflen, p, fill);
    h->t[0] += 128;
    if (h->t[0] < 128)
      h->t[1]++; /* Overflow */
    fio___blake2b_compress(h, h->buf, 0);
    h->buflen = 0;
    p += fill;
    len -= fill;
  }

  /* Process full blocks */
  while (len > 128) {
    h->t[0] += 128;
    if (h->t[0] < 128)
      h->t[1]++;
    fio___blake2b_compress(h, p, 0);
    p += 128;
    len -= 128;
  }

  /* Buffer remaining data */
  if (len > 0) {
    FIO_MEMCPY(h->buf, p, len);
    h->buflen = len;
  }
}

/** Finalize BLAKE2b hash. */
SFUNC void fio_blake2b_finalize(fio_blake2b_s *restrict h, void *restrict out) {
  /* Update counter with remaining bytes */
  h->t[0] += h->buflen;
  if (h->t[0] < h->buflen)
    h->t[1]++;

  /* Pad remaining buffer with zeros */
  if (h->buflen < 128)
    FIO_MEMSET(h->buf + h->buflen, 0, 128 - h->buflen);

  /* Final compression */
  fio___blake2b_compress(h, h->buf, 1);

  /* Output hash (little-endian) */
  uint8_t *o = (uint8_t *)out;
  for (size_t i = 0; i < h->outlen; ++i)
    o[i] = (uint8_t)(h->h[i / 8] >> (8 * (i % 8)));
}

/** Simple non-streaming BLAKE2b. */
SFUNC void fio_blake2b(void *restrict out,
                       size_t outlen,
                       const void *restrict data,
                       size_t len,
                       const void *restrict key,
                       size_t keylen) {
  fio_blake2b_s h = fio_blake2b_init(outlen, key, keylen);
  fio_blake2b_consume(&h, data, len);
  fio_blake2b_finalize(&h, out);
}

/* *****************************************************************************
BLAKE2s Implementation (32-bit)
***************************************************************************** */

/* BLAKE2s initialization vector */
static const uint32_t fio___blake2s_iv[8] = {0x6A09E667UL,
                                             0xBB67AE85UL,
                                             0x3C6EF372UL,
                                             0xA54FF53AUL,
                                             0x510E527FUL,
                                             0x9B05688CUL,
                                             0x1F83D9ABUL,
                                             0x5BE0CD19UL};

/* BLAKE2s sigma permutation table (same as BLAKE2b) */
static const uint8_t fio___blake2s_sigma[10][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
    {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
    {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
    {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
    {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
    {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
    {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
    {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
    {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0}};

/* BLAKE2s G mixing function */
#define FIO___BLAKE2S_G(r, i, a, b, c, d, m)                                   \
  do {                                                                         \
    (a) += (b) + m[fio___blake2s_sigma[r][2 * (i)]];                           \
    (d) = fio_rrot32((d) ^ (a), 16);                                           \
    (c) += (d);                                                                \
    (b) = fio_rrot32((b) ^ (c), 12);                                           \
    (a) += (b) + m[fio___blake2s_sigma[r][2 * (i) + 1]];                       \
    (d) = fio_rrot32((d) ^ (a), 8);                                            \
    (c) += (d);                                                                \
    (b) = fio_rrot32((b) ^ (c), 7);                                            \
  } while (0)

/* BLAKE2s round function */
#define FIO___BLAKE2S_ROUND(r, v, m)                                           \
  do {                                                                         \
    FIO___BLAKE2S_G(r, 0, v[0], v[4], v[8], v[12], m);                         \
    FIO___BLAKE2S_G(r, 1, v[1], v[5], v[9], v[13], m);                         \
    FIO___BLAKE2S_G(r, 2, v[2], v[6], v[10], v[14], m);                        \
    FIO___BLAKE2S_G(r, 3, v[3], v[7], v[11], v[15], m);                        \
    FIO___BLAKE2S_G(r, 4, v[0], v[5], v[10], v[15], m);                        \
    FIO___BLAKE2S_G(r, 5, v[1], v[6], v[11], v[12], m);                        \
    FIO___BLAKE2S_G(r, 6, v[2], v[7], v[8], v[13], m);                         \
    FIO___BLAKE2S_G(r, 7, v[3], v[4], v[9], v[14], m);                         \
  } while (0)

/* BLAKE2s compression function */
FIO_IFUNC void fio___blake2s_compress(fio_blake2s_s *restrict h,
                                      const uint8_t *restrict block,
                                      int is_last) {
  uint32_t v[16] FIO_ALIGN(64);
  uint32_t m[16] FIO_ALIGN(64);

  /* Initialize working vector */
  for (size_t i = 0; i < 8; ++i) {
    v[i] = h->h[i];
    v[i + 8] = fio___blake2s_iv[i];
  }
  v[12] ^= h->t[0];
  v[13] ^= h->t[1];
  if (is_last)
    v[14] = ~v[14];

  /* Load message block (little-endian) */
  for (size_t i = 0; i < 16; ++i)
    m[i] = fio_buf2u32_le(block + i * 4);

  /* 10 rounds of mixing */
  FIO___BLAKE2S_ROUND(0, v, m);
  FIO___BLAKE2S_ROUND(1, v, m);
  FIO___BLAKE2S_ROUND(2, v, m);
  FIO___BLAKE2S_ROUND(3, v, m);
  FIO___BLAKE2S_ROUND(4, v, m);
  FIO___BLAKE2S_ROUND(5, v, m);
  FIO___BLAKE2S_ROUND(6, v, m);
  FIO___BLAKE2S_ROUND(7, v, m);
  FIO___BLAKE2S_ROUND(8, v, m);
  FIO___BLAKE2S_ROUND(9, v, m);

  /* Finalize state */
  for (size_t i = 0; i < 8; ++i)
    h->h[i] ^= v[i] ^ v[i + 8];
}

#undef FIO___BLAKE2S_G
#undef FIO___BLAKE2S_ROUND

/** Initialize a BLAKE2s streaming context. */
SFUNC fio_blake2s_s fio_blake2s_init(size_t outlen,
                                     const void *key,
                                     size_t keylen) {
  fio_blake2s_s h = {0};

  /* Validate parameters */
  if (outlen == 0)
    outlen = 32;
  if (outlen > 32)
    outlen = 32;
  if (keylen > 32)
    keylen = 32;

  h.outlen = outlen;

  /* Initialize state with IV */
  for (size_t i = 0; i < 8; ++i)
    h.h[i] = fio___blake2s_iv[i];

  /* XOR parameter block into state[0] */
  h.h[0] ^= 0x01010000UL ^ ((uint32_t)keylen << 8) ^ (uint32_t)outlen;

  /* If keyed, pad key to 64 bytes and process as first block */
  if (keylen > 0 && key) {
    FIO_MEMSET(h.buf, 0, 64);
    FIO_MEMCPY(h.buf, key, keylen);
    h.buflen = 64;
  }

  return h;
}

/** Feed data into BLAKE2s hash. */
SFUNC void fio_blake2s_consume(fio_blake2s_s *restrict h,
                               const void *restrict data,
                               size_t len) {
  const uint8_t *p = (const uint8_t *)data;

  /* If we have buffered data, try to complete a block */
  if (h->buflen > 0) {
    size_t fill = 64 - h->buflen;
    if (len < fill) {
      FIO_MEMCPY(h->buf + h->buflen, p, len);
      h->buflen += len;
      return;
    }
    FIO_MEMCPY(h->buf + h->buflen, p, fill);
    h->t[0] += 64;
    if (h->t[0] < 64)
      h->t[1]++;
    fio___blake2s_compress(h, h->buf, 0);
    h->buflen = 0;
    p += fill;
    len -= fill;
  }

  /* Process full blocks */
  while (len > 64) {
    h->t[0] += 64;
    if (h->t[0] < 64)
      h->t[1]++;
    fio___blake2s_compress(h, p, 0);
    p += 64;
    len -= 64;
  }

  /* Buffer remaining data */
  if (len > 0) {
    FIO_MEMCPY(h->buf, p, len);
    h->buflen = len;
  }
}

/** Finalize BLAKE2s hash. */
SFUNC void fio_blake2s_finalize(fio_blake2s_s *restrict h, void *restrict out) {
  /* Update counter with remaining bytes */
  h->t[0] += (uint32_t)h->buflen;
  if (h->t[0] < h->buflen)
    h->t[1]++;

  /* Pad remaining buffer with zeros */
  if (h->buflen < 64)
    FIO_MEMSET(h->buf + h->buflen, 0, 64 - h->buflen);

  /* Final compression */
  fio___blake2s_compress(h, h->buf, 1);

  /* Output hash (little-endian) */
  uint8_t *o = (uint8_t *)out;
  for (size_t i = 0; i < h->outlen; ++i)
    o[i] = (uint8_t)(h->h[i / 4] >> (8 * (i % 4)));
}

/** Simple non-streaming BLAKE2s. */
SFUNC void fio_blake2s(void *restrict out,
                       size_t outlen,
                       const void *restrict data,
                       size_t len,
                       const void *restrict key,
                       size_t keylen) {
  fio_blake2s_s h = fio_blake2s_init(outlen, key, keylen);
  fio_blake2s_consume(&h, data, len);
  fio_blake2s_finalize(&h, out);
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_BLAKE2 */
#undef FIO_BLAKE2
