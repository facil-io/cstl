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

/** One-shot BLAKE2b hash with max-length (64 byte) digest. */
FIO_IFUNC fio_u512 fio_blake2b(const void *data, uint64_t len);

/**
 * Flexible-output BLAKE2b hash.
 *
 * `out` must point to a buffer of at least `outlen` bytes.
 * `outlen` must be between 1 and 64 (default 64 if 0).
 * `key` and `keylen` are optional (set to NULL/0 for unkeyed hashing).
 */
SFUNC void fio_blake2b_hash(void *restrict out,
                            size_t outlen,
                            const void *restrict data,
                            size_t len,
                            const void *restrict key,
                            size_t keylen);

/** HMAC-BLAKE2b (64 byte key, 64 byte digest), compatible with SHA HMAC. */
SFUNC fio_u512 fio_blake2b_hmac(const void *key,
                                uint64_t key_len,
                                const void *msg,
                                uint64_t msg_len);

/** Initialize a BLAKE2b streaming context. outlen: 1-64 (default 64). */
SFUNC fio_blake2b_s fio_blake2b_init(size_t outlen,
                                     const void *key,
                                     size_t keylen);

/** Feed data into BLAKE2b hash. */
SFUNC void fio_blake2b_consume(fio_blake2b_s *restrict h,
                               const void *restrict data,
                               size_t len);

/** Finalize BLAKE2b hash. Returns result in fio_u512 (valid bytes = outlen). */
SFUNC fio_u512 fio_blake2b_finalize(fio_blake2b_s *h);

/* *****************************************************************************
BLAKE2s API (32-bit optimized, up to 32-byte digest)
***************************************************************************** */

/** One-shot BLAKE2s hash with max-length (32 byte) digest. */
FIO_IFUNC fio_u256 fio_blake2s(const void *data, uint64_t len);

/**
 * Flexible-output BLAKE2s hash.
 *
 * `out` must point to a buffer of at least `outlen` bytes.
 * `outlen` must be between 1 and 32 (default 32 if 0).
 * `key` and `keylen` are optional (set to NULL/0 for unkeyed hashing).
 */
SFUNC void fio_blake2s_hash(void *restrict out,
                            size_t outlen,
                            const void *restrict data,
                            size_t len,
                            const void *restrict key,
                            size_t keylen);

/** HMAC-BLAKE2s (32 byte key, 32 byte digest), compatible with SHA HMAC. */
SFUNC fio_u256 fio_blake2s_hmac(const void *key,
                                uint64_t key_len,
                                const void *msg,
                                uint64_t msg_len);

/** Initialize a BLAKE2s streaming context. outlen: 1-32 (default 32). */
SFUNC fio_blake2s_s fio_blake2s_init(size_t outlen,
                                     const void *key,
                                     size_t keylen);

/** Feed data into BLAKE2s hash. */
SFUNC void fio_blake2s_consume(fio_blake2s_s *restrict h,
                               const void *restrict data,
                               size_t len);

/** Finalize BLAKE2s hash. Returns result in fio_u256 (valid bytes = outlen). */
SFUNC fio_u256 fio_blake2s_finalize(fio_blake2s_s *h);

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

/* BLAKE2b G mixing function with hardcoded message word indices */
#define FIO___BLAKE2B_G(a, b, c, d, mx, my)                                    \
  do {                                                                         \
    (a) += (b) + (mx);                                                         \
    (d) = fio_rrot64((d) ^ (a), 32);                                           \
    (c) += (d);                                                                \
    (b) = fio_rrot64((b) ^ (c), 24);                                           \
    (a) += (b) + (my);                                                         \
    (d) = fio_rrot64((d) ^ (a), 16);                                           \
    (c) += (d);                                                                \
    (b) = fio_rrot64((b) ^ (c), 63);                                           \
  } while (0)

/* BLAKE2b compression function - sigma indices fully hardcoded per round */
FIO_IFUNC void fio___blake2b_compress(fio_blake2b_s *restrict h,
                                      const uint8_t *restrict block,
                                      int is_last) {
  uint64_t v[16] FIO_ALIGN(64);
  uint64_t m[16] FIO_ALIGN(64);

  /* Initialize working vector - unrolled */
  v[0] = h->h[0];
  v[1] = h->h[1];
  v[2] = h->h[2];
  v[3] = h->h[3];
  v[4] = h->h[4];
  v[5] = h->h[5];
  v[6] = h->h[6];
  v[7] = h->h[7];
  v[8] = fio___blake2b_iv[0];
  v[9] = fio___blake2b_iv[1];
  v[10] = fio___blake2b_iv[2];
  v[11] = fio___blake2b_iv[3];
  v[12] = fio___blake2b_iv[4] ^ h->t[0];
  v[13] = fio___blake2b_iv[5] ^ h->t[1];
  v[14] = is_last ? ~fio___blake2b_iv[6] : fio___blake2b_iv[6];
  v[15] = fio___blake2b_iv[7];

  /* Load message block (little-endian) - unrolled */
  m[0] = fio_buf2u64_le(block);
  m[1] = fio_buf2u64_le(block + 8);
  m[2] = fio_buf2u64_le(block + 16);
  m[3] = fio_buf2u64_le(block + 24);
  m[4] = fio_buf2u64_le(block + 32);
  m[5] = fio_buf2u64_le(block + 40);
  m[6] = fio_buf2u64_le(block + 48);
  m[7] = fio_buf2u64_le(block + 56);
  m[8] = fio_buf2u64_le(block + 64);
  m[9] = fio_buf2u64_le(block + 72);
  m[10] = fio_buf2u64_le(block + 80);
  m[11] = fio_buf2u64_le(block + 88);
  m[12] = fio_buf2u64_le(block + 96);
  m[13] = fio_buf2u64_le(block + 104);
  m[14] = fio_buf2u64_le(block + 112);
  m[15] = fio_buf2u64_le(block + 120);

  /* 12 rounds with hardcoded sigma permutation indices */
  /* Round 0: sigma = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[0], m[1]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[2], m[3]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[4], m[5]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[6], m[7]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[8], m[9]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[10], m[11]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[12], m[13]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[14], m[15]);

  /* Round 1: sigma = {14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[14], m[10]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[4], m[8]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[9], m[15]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[13], m[6]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[1], m[12]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[0], m[2]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[11], m[7]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[5], m[3]);

  /* Round 2: sigma = {11,8,12,0,5,2,15,13,10,14,3,6,7,1,9,4} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[11], m[8]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[12], m[0]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[5], m[2]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[15], m[13]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[10], m[14]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[3], m[6]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[7], m[1]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[9], m[4]);

  /* Round 3: sigma = {7,9,3,1,13,12,11,14,2,6,5,10,4,0,15,8} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[7], m[9]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[3], m[1]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[13], m[12]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[11], m[14]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[2], m[6]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[5], m[10]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[4], m[0]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[15], m[8]);

  /* Round 4: sigma = {9,0,5,7,2,4,10,15,14,1,11,12,6,8,3,13} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[9], m[0]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[5], m[7]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[2], m[4]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[10], m[15]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[14], m[1]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[11], m[12]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[6], m[8]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[3], m[13]);

  /* Round 5: sigma = {2,12,6,10,0,11,8,3,4,13,7,5,15,14,1,9} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[2], m[12]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[6], m[10]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[0], m[11]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[8], m[3]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[4], m[13]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[7], m[5]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[15], m[14]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[1], m[9]);

  /* Round 6: sigma = {12,5,1,15,14,13,4,10,0,7,6,3,9,2,8,11} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[12], m[5]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[1], m[15]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[14], m[13]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[4], m[10]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[0], m[7]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[6], m[3]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[9], m[2]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[8], m[11]);

  /* Round 7: sigma = {13,11,7,14,12,1,3,9,5,0,15,4,8,6,2,10} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[13], m[11]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[7], m[14]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[12], m[1]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[3], m[9]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[5], m[0]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[15], m[4]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[8], m[6]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[2], m[10]);

  /* Round 8: sigma = {6,15,14,9,11,3,0,8,12,2,13,7,1,4,10,5} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[6], m[15]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[14], m[9]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[11], m[3]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[0], m[8]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[12], m[2]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[13], m[7]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[1], m[4]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[10], m[5]);

  /* Round 9: sigma = {10,2,8,4,7,6,1,5,15,11,9,14,3,12,13,0} */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[10], m[2]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[8], m[4]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[7], m[6]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[1], m[5]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[15], m[11]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[9], m[14]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[3], m[12]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[13], m[0]);

  /* Round 10: sigma = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} (same as 0) */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[0], m[1]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[2], m[3]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[4], m[5]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[6], m[7]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[8], m[9]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[10], m[11]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[12], m[13]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[14], m[15]);

  /* Round 11: sigma = {14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3} (same as 1) */
  FIO___BLAKE2B_G(v[0], v[4], v[8], v[12], m[14], m[10]);
  FIO___BLAKE2B_G(v[1], v[5], v[9], v[13], m[4], m[8]);
  FIO___BLAKE2B_G(v[2], v[6], v[10], v[14], m[9], m[15]);
  FIO___BLAKE2B_G(v[3], v[7], v[11], v[15], m[13], m[6]);
  FIO___BLAKE2B_G(v[0], v[5], v[10], v[15], m[1], m[12]);
  FIO___BLAKE2B_G(v[1], v[6], v[11], v[12], m[0], m[2]);
  FIO___BLAKE2B_G(v[2], v[7], v[8], v[13], m[11], m[7]);
  FIO___BLAKE2B_G(v[3], v[4], v[9], v[14], m[5], m[3]);

  /* Finalize state - unrolled */
  h->h[0] ^= v[0] ^ v[8];
  h->h[1] ^= v[1] ^ v[9];
  h->h[2] ^= v[2] ^ v[10];
  h->h[3] ^= v[3] ^ v[11];
  h->h[4] ^= v[4] ^ v[12];
  h->h[5] ^= v[5] ^ v[13];
  h->h[6] ^= v[6] ^ v[14];
  h->h[7] ^= v[7] ^ v[15];
}

#undef FIO___BLAKE2B_G

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

  /* Initialize state with IV - unrolled */
  h.h[0] = fio___blake2b_iv[0];
  h.h[1] = fio___blake2b_iv[1];
  h.h[2] = fio___blake2b_iv[2];
  h.h[3] = fio___blake2b_iv[3];
  h.h[4] = fio___blake2b_iv[4];
  h.h[5] = fio___blake2b_iv[5];
  h.h[6] = fio___blake2b_iv[6];
  h.h[7] = fio___blake2b_iv[7];

  /* XOR parameter block into state[0] */
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
  if (!len)
    return;
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

/** Finalize BLAKE2b hash. Returns result in fio_u512 (valid bytes = outlen). */
SFUNC fio_u512 fio_blake2b_finalize(fio_blake2b_s *h) {
  fio_u512 r = {0};
  /* Update counter with remaining bytes */
  h->t[0] += h->buflen;
  if (h->t[0] < h->buflen)
    h->t[1]++;

  /* Pad remaining buffer with zeros */
  if (h->buflen < 128)
    FIO_MEMSET(h->buf + h->buflen, 0, 128 - h->buflen);

  /* Final compression */
  fio___blake2b_compress(h, h->buf, 1);

  /* Output hash (little-endian) - word-sized writes */
  size_t full_words = h->outlen >> 3; /* outlen / 8 */
  size_t i;
  for (i = 0; i < full_words; ++i)
    fio_u2buf64_le(r.u8 + (i << 3), h->h[i]);
  /* Handle remaining bytes (outlen not multiple of 8) */
  size_t remaining = h->outlen & 7;
  if (remaining) {
    uint64_t last = h->h[i];
    uint8_t *dst = r.u8 + (i << 3);
    for (size_t j = 0; j < remaining; ++j)
      dst[j] = (uint8_t)(last >> (8 * j));
  }
  return r;
}

/** Flexible-output non-streaming BLAKE2b. */
SFUNC void fio_blake2b_hash(void *restrict out,
                            size_t outlen,
                            const void *restrict data,
                            size_t len,
                            const void *restrict key,
                            size_t keylen) {
  fio_blake2b_s h = fio_blake2b_init(outlen, key, keylen);
  fio_blake2b_consume(&h, data, len);
  fio_u512 r = fio_blake2b_finalize(&h);
  FIO_MEMCPY(out, r.u8, h.outlen);
}

/** One-shot BLAKE2b hash with max-length (64 byte) digest. */
FIO_IFUNC fio_u512 fio_blake2b(const void *data, uint64_t len) {
  fio_blake2b_s h = fio_blake2b_init(64, NULL, 0);
  fio_blake2b_consume(&h, data, (size_t)len);
  return fio_blake2b_finalize(&h);
}

/** HMAC-BLAKE2b (standard HMAC construction, 64-byte digest). */
SFUNC fio_u512 fio_blake2b_hmac(const void *key,
                                uint64_t key_len,
                                const void *msg,
                                uint64_t msg_len) {
  uint64_t k_padded[16] = {0};
  /* If key > block size, hash it first */
  if (key_len > 128) {
    fio_blake2b_s hk = fio_blake2b_init(64, NULL, 0);
    fio_blake2b_consume(&hk, key, (size_t)key_len);
    fio_u512 hashed = fio_blake2b_finalize(&hk);
    fio_memcpy64(&k_padded, hashed.u8);
  } else if (key_len) {
    FIO_MEMCPY(&k_padded, key, (size_t)key_len);
  }

  /* Inner hash: H((padded_key ^ 0x36) || msg) */
  for (size_t i = 0; i < 16; ++i)
    k_padded[i] ^= (uint64_t)0x3636363636363636ULL;
  fio_blake2b_s inner = fio_blake2b_init(64, NULL, 0);
  fio_blake2b_consume(&inner, k_padded, 128);
  fio_blake2b_consume(&inner, msg, (size_t)msg_len);
  fio_u512 inner_hash = fio_blake2b_finalize(&inner);

  /* Outer hash: H((padded_key ^ 0x5C) || inner_hash) */
  for (size_t i = 0; i < 16; ++i)
    k_padded[i] ^=
        (uint64_t)0x3636363636363636ULL ^ (uint64_t)0x5C5C5C5C5C5C5C5CULL;
  fio_blake2b_s outer = fio_blake2b_init(64, NULL, 0);
  fio_blake2b_consume(&outer, k_padded, 128);
  fio_blake2b_consume(&outer, inner_hash.u8, 64);
  fio_u512 result = fio_blake2b_finalize(&outer);
  fio_secure_zero(&k_padded, sizeof(k_padded));
  fio_secure_zero(&inner_hash, sizeof(inner_hash));
  return result;
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

/* BLAKE2s G mixing function with hardcoded message word indices */
#define FIO___BLAKE2S_G(a, b, c, d, mx, my)                                    \
  do {                                                                         \
    (a) += (b) + (mx);                                                         \
    (d) = fio_rrot32((d) ^ (a), 16);                                           \
    (c) += (d);                                                                \
    (b) = fio_rrot32((b) ^ (c), 12);                                           \
    (a) += (b) + (my);                                                         \
    (d) = fio_rrot32((d) ^ (a), 8);                                            \
    (c) += (d);                                                                \
    (b) = fio_rrot32((b) ^ (c), 7);                                            \
  } while (0)

/* BLAKE2s compression function - sigma indices fully hardcoded per round */
FIO_IFUNC void fio___blake2s_compress(fio_blake2s_s *restrict h,
                                      const uint8_t *restrict block,
                                      int is_last) {
  uint32_t v[16] FIO_ALIGN(64);
  uint32_t m[16] FIO_ALIGN(64);

  /* Initialize working vector - unrolled */
  v[0] = h->h[0];
  v[1] = h->h[1];
  v[2] = h->h[2];
  v[3] = h->h[3];
  v[4] = h->h[4];
  v[5] = h->h[5];
  v[6] = h->h[6];
  v[7] = h->h[7];
  v[8] = fio___blake2s_iv[0];
  v[9] = fio___blake2s_iv[1];
  v[10] = fio___blake2s_iv[2];
  v[11] = fio___blake2s_iv[3];
  v[12] = fio___blake2s_iv[4] ^ h->t[0];
  v[13] = fio___blake2s_iv[5] ^ h->t[1];
  v[14] = is_last ? ~fio___blake2s_iv[6] : fio___blake2s_iv[6];
  v[15] = fio___blake2s_iv[7];

  /* Load message block (little-endian) - unrolled */
  m[0] = fio_buf2u32_le(block);
  m[1] = fio_buf2u32_le(block + 4);
  m[2] = fio_buf2u32_le(block + 8);
  m[3] = fio_buf2u32_le(block + 12);
  m[4] = fio_buf2u32_le(block + 16);
  m[5] = fio_buf2u32_le(block + 20);
  m[6] = fio_buf2u32_le(block + 24);
  m[7] = fio_buf2u32_le(block + 28);
  m[8] = fio_buf2u32_le(block + 32);
  m[9] = fio_buf2u32_le(block + 36);
  m[10] = fio_buf2u32_le(block + 40);
  m[11] = fio_buf2u32_le(block + 44);
  m[12] = fio_buf2u32_le(block + 48);
  m[13] = fio_buf2u32_le(block + 52);
  m[14] = fio_buf2u32_le(block + 56);
  m[15] = fio_buf2u32_le(block + 60);

  /* 10 rounds with hardcoded sigma permutation indices */
  /* Round 0: sigma = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[0], m[1]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[2], m[3]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[4], m[5]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[6], m[7]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[8], m[9]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[10], m[11]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[12], m[13]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[14], m[15]);

  /* Round 1: sigma = {14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[14], m[10]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[4], m[8]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[9], m[15]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[13], m[6]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[1], m[12]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[0], m[2]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[11], m[7]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[5], m[3]);

  /* Round 2: sigma = {11,8,12,0,5,2,15,13,10,14,3,6,7,1,9,4} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[11], m[8]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[12], m[0]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[5], m[2]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[15], m[13]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[10], m[14]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[3], m[6]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[7], m[1]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[9], m[4]);

  /* Round 3: sigma = {7,9,3,1,13,12,11,14,2,6,5,10,4,0,15,8} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[7], m[9]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[3], m[1]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[13], m[12]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[11], m[14]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[2], m[6]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[5], m[10]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[4], m[0]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[15], m[8]);

  /* Round 4: sigma = {9,0,5,7,2,4,10,15,14,1,11,12,6,8,3,13} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[9], m[0]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[5], m[7]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[2], m[4]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[10], m[15]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[14], m[1]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[11], m[12]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[6], m[8]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[3], m[13]);

  /* Round 5: sigma = {2,12,6,10,0,11,8,3,4,13,7,5,15,14,1,9} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[2], m[12]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[6], m[10]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[0], m[11]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[8], m[3]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[4], m[13]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[7], m[5]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[15], m[14]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[1], m[9]);

  /* Round 6: sigma = {12,5,1,15,14,13,4,10,0,7,6,3,9,2,8,11} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[12], m[5]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[1], m[15]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[14], m[13]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[4], m[10]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[0], m[7]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[6], m[3]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[9], m[2]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[8], m[11]);

  /* Round 7: sigma = {13,11,7,14,12,1,3,9,5,0,15,4,8,6,2,10} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[13], m[11]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[7], m[14]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[12], m[1]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[3], m[9]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[5], m[0]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[15], m[4]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[8], m[6]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[2], m[10]);

  /* Round 8: sigma = {6,15,14,9,11,3,0,8,12,2,13,7,1,4,10,5} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[6], m[15]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[14], m[9]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[11], m[3]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[0], m[8]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[12], m[2]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[13], m[7]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[1], m[4]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[10], m[5]);

  /* Round 9: sigma = {10,2,8,4,7,6,1,5,15,11,9,14,3,12,13,0} */
  FIO___BLAKE2S_G(v[0], v[4], v[8], v[12], m[10], m[2]);
  FIO___BLAKE2S_G(v[1], v[5], v[9], v[13], m[8], m[4]);
  FIO___BLAKE2S_G(v[2], v[6], v[10], v[14], m[7], m[6]);
  FIO___BLAKE2S_G(v[3], v[7], v[11], v[15], m[1], m[5]);
  FIO___BLAKE2S_G(v[0], v[5], v[10], v[15], m[15], m[11]);
  FIO___BLAKE2S_G(v[1], v[6], v[11], v[12], m[9], m[14]);
  FIO___BLAKE2S_G(v[2], v[7], v[8], v[13], m[3], m[12]);
  FIO___BLAKE2S_G(v[3], v[4], v[9], v[14], m[13], m[0]);

  /* Finalize state - unrolled */
  h->h[0] ^= v[0] ^ v[8];
  h->h[1] ^= v[1] ^ v[9];
  h->h[2] ^= v[2] ^ v[10];
  h->h[3] ^= v[3] ^ v[11];
  h->h[4] ^= v[4] ^ v[12];
  h->h[5] ^= v[5] ^ v[13];
  h->h[6] ^= v[6] ^ v[14];
  h->h[7] ^= v[7] ^ v[15];
}

#undef FIO___BLAKE2S_G

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

  /* Initialize state with IV - unrolled */
  h.h[0] = fio___blake2s_iv[0];
  h.h[1] = fio___blake2s_iv[1];
  h.h[2] = fio___blake2s_iv[2];
  h.h[3] = fio___blake2s_iv[3];
  h.h[4] = fio___blake2s_iv[4];
  h.h[5] = fio___blake2s_iv[5];
  h.h[6] = fio___blake2s_iv[6];
  h.h[7] = fio___blake2s_iv[7];

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
  if (!len)
    return;
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

/** Finalize BLAKE2s hash. Returns result in fio_u256 (valid bytes = outlen). */
SFUNC fio_u256 fio_blake2s_finalize(fio_blake2s_s *h) {
  fio_u256 r = {0};
  /* Update counter with remaining bytes */
  h->t[0] += (uint32_t)h->buflen;
  if (h->t[0] < h->buflen)
    h->t[1]++;

  /* Pad remaining buffer with zeros */
  if (h->buflen < 64)
    FIO_MEMSET(h->buf + h->buflen, 0, 64 - h->buflen);

  /* Final compression */
  fio___blake2s_compress(h, h->buf, 1);

  /* Output hash (little-endian) - word-sized writes */
  size_t full_words = h->outlen >> 2; /* outlen / 4 */
  size_t i;
  for (i = 0; i < full_words; ++i)
    fio_u2buf32_le(r.u8 + (i << 2), h->h[i]);
  /* Handle remaining bytes (outlen not multiple of 4) */
  size_t remaining = h->outlen & 3;
  if (remaining) {
    uint32_t last = h->h[i];
    uint8_t *dst = r.u8 + (i << 2);
    for (size_t j = 0; j < remaining; ++j)
      dst[j] = (uint8_t)(last >> (8 * j));
  }
  return r;
}

/** Flexible-output non-streaming BLAKE2s. */
SFUNC void fio_blake2s_hash(void *restrict out,
                            size_t outlen,
                            const void *restrict data,
                            size_t len,
                            const void *restrict key,
                            size_t keylen) {
  fio_blake2s_s h = fio_blake2s_init(outlen, key, keylen);
  fio_blake2s_consume(&h, data, len);
  fio_u256 r = fio_blake2s_finalize(&h);
  FIO_MEMCPY(out, r.u8, h.outlen);
}

/** One-shot BLAKE2s hash with max-length (32 byte) digest. */
FIO_IFUNC fio_u256 fio_blake2s(const void *data, uint64_t len) {
  fio_blake2s_s h = fio_blake2s_init(32, NULL, 0);
  fio_blake2s_consume(&h, data, (size_t)len);
  return fio_blake2s_finalize(&h);
}

/** HMAC-BLAKE2s (standard HMAC construction, 32-byte digest). */
SFUNC fio_u256 fio_blake2s_hmac(const void *key,
                                uint64_t key_len,
                                const void *msg,
                                uint64_t msg_len) {
  uint64_t k_padded[8] = {0};
  /* If key > block size, hash it first */
  if (key_len > 64) {
    fio_blake2s_s hk = fio_blake2s_init(32, NULL, 0);
    fio_blake2s_consume(&hk, key, (size_t)key_len);
    fio_u256 hashed = fio_blake2s_finalize(&hk);
    FIO_MEMCPY(k_padded, hashed.u8, 32);
  } else if (key_len) {
    FIO_MEMCPY(k_padded, key, (size_t)key_len);
  }

  for (size_t i = 0; i < 8; ++i)
    k_padded[i] ^= (uint64_t)0x3636363636363636ULL;

  /* Inner hash: H(i_pad || msg) */
  fio_blake2s_s inner = fio_blake2s_init(32, NULL, 0);
  fio_blake2s_consume(&inner, k_padded, 64);
  fio_blake2s_consume(&inner, msg, (size_t)msg_len);
  fio_u256 inner_hash = fio_blake2s_finalize(&inner);

  for (size_t i = 0; i < 8; ++i)
    k_padded[i] ^=
        (uint64_t)0x3636363636363636ULL ^ (uint64_t)0x5C5C5C5C5C5C5C5CULL;

  /* Outer hash: H(o_pad || inner_hash) */
  fio_blake2s_s outer = fio_blake2s_init(32, NULL, 0);
  fio_blake2s_consume(&outer, k_padded, 64);
  fio_blake2s_consume(&outer, inner_hash.u8, 32);
  fio_u256 result = fio_blake2s_finalize(&outer);
  fio_secure_zero(k_padded, sizeof(k_padded));
  fio_secure_zero(&inner_hash, sizeof(inner_hash));
  return result;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_BLAKE2 */
#undef FIO_BLAKE2
