/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_SHA1               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                    SHA 1



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_SHA1) && !defined(H___FIO_SHA1___H)
#define H___FIO_SHA1___H
/* *****************************************************************************
SHA 1
***************************************************************************** */

/** The data type containing the SHA1 digest (result). */
typedef union {
#ifdef __SIZEOF_INT128__
  __uint128_t align__;
#else
  uint64_t align__;
#endif
  uint32_t v[5];
  uint8_t digest[20];
} fio_sha1_s;

/**
 * A simple, non streaming, implementation of the SHA1 hashing algorithm.
 *
 * Do NOT use - SHA1 is broken... but for some reason some protocols still
 * require it's use (i.e., WebSockets), so it's here for your convenience.
 */
SFUNC fio_sha1_s fio_sha1(const void *data, uint64_t len);

/** Returns the digest length of SHA1 in bytes (20 bytes) */
FIO_IFUNC size_t fio_sha1_len(void);

/** Returns the 20 Byte long digest of a SHA1 object. */
FIO_IFUNC uint8_t *fio_sha1_digest(fio_sha1_s *s);

/* *****************************************************************************
SHA 1 Implementation - inlined static functions
***************************************************************************** */

/** returns the digest length of SHA1 in bytes */
FIO_IFUNC size_t fio_sha1_len(void) { return 20; }

/** returns the digest of a SHA1 object. */
FIO_IFUNC uint8_t *fio_sha1_digest(fio_sha1_s *s) { return s->digest; }

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO_IFUNC void fio___sha1_round512(uint32_t *old, /* state */
                                   uint32_t *w /* 16 words */) {
#if FIO___HAS_X86_SHA_INTRIN
  /* Code adjusted from:
   * https://github.com/noloader/SHA-Intrinsics/blob/master/sha1-x86.c
   * Credit to Jeffrey Walton.
   */
  __m128i abcd, e0, e1;
  __m128i abcd_save, e_save;
  __m128i msg0, msg1, msg2, msg3;

  /* Load initial values */
  abcd = _mm_loadu_si128((const __m128i *)old);
  e0 = _mm_set_epi32(old[4], 0, 0, 0);
  abcd = _mm_shuffle_epi32(abcd, 0x1B); /* big endian */
  /* Save current state */
  abcd_save = abcd;
  e_save = e0;

  /* Load and convert message to big endian */
  msg0 = _mm_loadu_si128((const __m128i *)(w + 0));
  msg1 = _mm_loadu_si128((const __m128i *)(w + 4));
  msg2 = _mm_loadu_si128((const __m128i *)(w + 8));
  msg3 = _mm_loadu_si128((const __m128i *)(w + 12));
  msg0 = _mm_shuffle_epi8(
      msg0,
      _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
  msg1 = _mm_shuffle_epi8(
      msg1,
      _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
  msg2 = _mm_shuffle_epi8(
      msg2,
      _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
  msg3 = _mm_shuffle_epi8(
      msg3,
      _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));

  /* Rounds 0-3 */
  e0 = _mm_add_epi32(e0, msg0);
  e1 = abcd;
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);

  /* Rounds 4-7 */
  e1 = _mm_sha1nexte_epu32(e1, msg1);
  e0 = abcd;
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 0);
  msg0 = _mm_sha1msg1_epu32(msg0, msg1);

  /* Rounds 8-11 */
  e0 = _mm_sha1nexte_epu32(e0, msg2);
  e1 = abcd;
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);
  msg1 = _mm_sha1msg1_epu32(msg1, msg2);
  msg0 = _mm_xor_si128(msg0, msg2);

  /* Rounds 12-15 */
  e1 = _mm_sha1nexte_epu32(e1, msg3);
  e0 = abcd;
  msg0 = _mm_sha1msg2_epu32(msg0, msg3);
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 0);
  msg2 = _mm_sha1msg1_epu32(msg2, msg3);
  msg1 = _mm_xor_si128(msg1, msg3);

  /* Rounds 16-19 */
  e0 = _mm_sha1nexte_epu32(e0, msg0);
  e1 = abcd;
  msg1 = _mm_sha1msg2_epu32(msg1, msg0);
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 0);
  msg3 = _mm_sha1msg1_epu32(msg3, msg0);
  msg2 = _mm_xor_si128(msg2, msg0);

  /* Rounds 20-23 */
  e1 = _mm_sha1nexte_epu32(e1, msg1);
  e0 = abcd;
  msg2 = _mm_sha1msg2_epu32(msg2, msg1);
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 1);
  msg0 = _mm_sha1msg1_epu32(msg0, msg1);
  msg3 = _mm_xor_si128(msg3, msg1);

  /* Rounds 24-27 */
  e0 = _mm_sha1nexte_epu32(e0, msg2);
  e1 = abcd;
  msg3 = _mm_sha1msg2_epu32(msg3, msg2);
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 1);
  msg1 = _mm_sha1msg1_epu32(msg1, msg2);
  msg0 = _mm_xor_si128(msg0, msg2);

  /* Rounds 28-31 */
  e1 = _mm_sha1nexte_epu32(e1, msg3);
  e0 = abcd;
  msg0 = _mm_sha1msg2_epu32(msg0, msg3);
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 1);
  msg2 = _mm_sha1msg1_epu32(msg2, msg3);
  msg1 = _mm_xor_si128(msg1, msg3);

  /* Rounds 32-35 */
  e0 = _mm_sha1nexte_epu32(e0, msg0);
  e1 = abcd;
  msg1 = _mm_sha1msg2_epu32(msg1, msg0);
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 1);
  msg3 = _mm_sha1msg1_epu32(msg3, msg0);
  msg2 = _mm_xor_si128(msg2, msg0);

  /* Rounds 36-39 */
  e1 = _mm_sha1nexte_epu32(e1, msg1);
  e0 = abcd;
  msg2 = _mm_sha1msg2_epu32(msg2, msg1);
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 1);
  msg0 = _mm_sha1msg1_epu32(msg0, msg1);
  msg3 = _mm_xor_si128(msg3, msg1);

  /* Rounds 40-43 */
  e0 = _mm_sha1nexte_epu32(e0, msg2);
  e1 = abcd;
  msg3 = _mm_sha1msg2_epu32(msg3, msg2);
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 2);
  msg1 = _mm_sha1msg1_epu32(msg1, msg2);
  msg0 = _mm_xor_si128(msg0, msg2);

  /* Rounds 44-47 */
  e1 = _mm_sha1nexte_epu32(e1, msg3);
  e0 = abcd;
  msg0 = _mm_sha1msg2_epu32(msg0, msg3);
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 2);
  msg2 = _mm_sha1msg1_epu32(msg2, msg3);
  msg1 = _mm_xor_si128(msg1, msg3);

  /* Rounds 48-51 */
  e0 = _mm_sha1nexte_epu32(e0, msg0);
  e1 = abcd;
  msg1 = _mm_sha1msg2_epu32(msg1, msg0);
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 2);
  msg3 = _mm_sha1msg1_epu32(msg3, msg0);
  msg2 = _mm_xor_si128(msg2, msg0);

  /* Rounds 52-55 */
  e1 = _mm_sha1nexte_epu32(e1, msg1);
  e0 = abcd;
  msg2 = _mm_sha1msg2_epu32(msg2, msg1);
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 2);
  msg0 = _mm_sha1msg1_epu32(msg0, msg1);
  msg3 = _mm_xor_si128(msg3, msg1);

  /* Rounds 56-59 */
  e0 = _mm_sha1nexte_epu32(e0, msg2);
  e1 = abcd;
  msg3 = _mm_sha1msg2_epu32(msg3, msg2);
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 2);
  msg1 = _mm_sha1msg1_epu32(msg1, msg2);
  msg0 = _mm_xor_si128(msg0, msg2);

  /* Rounds 60-63 */
  e1 = _mm_sha1nexte_epu32(e1, msg3);
  e0 = abcd;
  msg0 = _mm_sha1msg2_epu32(msg0, msg3);
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 3);
  msg2 = _mm_sha1msg1_epu32(msg2, msg3);
  msg1 = _mm_xor_si128(msg1, msg3);

  /* Rounds 64-67 */
  e0 = _mm_sha1nexte_epu32(e0, msg0);
  e1 = abcd;
  msg1 = _mm_sha1msg2_epu32(msg1, msg0);
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 3);
  msg3 = _mm_sha1msg1_epu32(msg3, msg0);
  msg2 = _mm_xor_si128(msg2, msg0);

  /* Rounds 68-71 */
  e1 = _mm_sha1nexte_epu32(e1, msg1);
  e0 = abcd;
  msg2 = _mm_sha1msg2_epu32(msg2, msg1);
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 3);
  msg3 = _mm_xor_si128(msg3, msg1);

  /* Rounds 72-75 */
  e0 = _mm_sha1nexte_epu32(e0, msg2);
  e1 = abcd;
  msg3 = _mm_sha1msg2_epu32(msg3, msg2);
  abcd = _mm_sha1rnds4_epu32(abcd, e0, 3);

  /* Rounds 76-79 */
  e1 = _mm_sha1nexte_epu32(e1, msg3);
  e0 = abcd;
  abcd = _mm_sha1rnds4_epu32(abcd, e1, 3);

  /* Combine state */
  e0 = _mm_sha1nexte_epu32(e0, e_save);
  abcd = _mm_add_epi32(abcd, abcd_save);

  /* Save state (convert back from big endian) */
  abcd = _mm_shuffle_epi32(abcd, 0x1B);
  _mm_storeu_si128((__m128i *)old, abcd);
  old[4] = _mm_extract_epi32(e0, 3);

#elif FIO___HAS_ARM_INTRIN
  /* Code adjusted from:
   * https://github.com/noloader/SHA-Intrinsics/blob/master/sha1-arm.c
   * Credit to Jeffrey Walton.
   */
  uint32x4_t w0, w1, w2, w3;
  uint32x4_t t0, t1, v0, v_old;
  uint32_t e0, e1, e_old;
  e0 = e_old = old[4];
  v_old = vld1q_u32(old);
  v0 = v_old;

  /* load to vectors */
  w0 = vld1q_u32(w);
  w1 = vld1q_u32(w + 4);
  w2 = vld1q_u32(w + 8);
  w3 = vld1q_u32(w + 12);
  /* make little endian */
  w0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w0)));
  w1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w1)));
  w2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w2)));
  w3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w3)));

  t0 = vaddq_u32(w0, vdupq_n_u32(0x5A827999));
  t1 = vaddq_u32(w1, vdupq_n_u32(0x5A827999));

  /* round: 0-3 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1cq_u32(v0, e0, t0);
  t0 = vaddq_u32(w2, vdupq_n_u32(0x5A827999));
  w0 = vsha1su0q_u32(w0, w1, w2);

  /* round: 4-7 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1cq_u32(v0, e1, t1);
  t1 = vaddq_u32(w3, vdupq_n_u32(0x5A827999));
  w0 = vsha1su1q_u32(w0, w3);
  w1 = vsha1su0q_u32(w1, w2, w3);

  /* round: 8-11 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1cq_u32(v0, e0, t0);
  t0 = vaddq_u32(w0, vdupq_n_u32(0x5A827999));
  w1 = vsha1su1q_u32(w1, w0);
  w2 = vsha1su0q_u32(w2, w3, w0);

#define FIO_SHA1_ROUND_(K, rn_fn, n, ni, n0, n1, n2, n3)                       \
  e##n = vsha1h_u32(vgetq_lane_u32(v0, 0));                                    \
  v0 = rn_fn(v0, e##ni, t##ni);                                                \
  t##ni = vaddq_u32(w##n1, vdupq_n_u32(K));                                    \
  w##n2 = vsha1su1q_u32(w##n2, w##n1);                                         \
  w##n3 = vsha1su0q_u32(w##n3, w##n0, w##n1);
  FIO_SHA1_ROUND_(0x6ED9EBA1, vsha1cq_u32, 0, 1, 0, 1, 2, 3)
  FIO_SHA1_ROUND_(0x6ED9EBA1, vsha1cq_u32, 1, 0, 1, 2, 3, 0)
  FIO_SHA1_ROUND_(0x6ED9EBA1, vsha1pq_u32, 0, 1, 2, 3, 0, 1)
  FIO_SHA1_ROUND_(0x6ED9EBA1, vsha1pq_u32, 1, 0, 3, 0, 1, 2)
  FIO_SHA1_ROUND_(0x6ED9EBA1, vsha1pq_u32, 0, 1, 0, 1, 2, 3)

  FIO_SHA1_ROUND_(0x8F1BBCDC, vsha1pq_u32, 1, 0, 1, 2, 3, 0)
  FIO_SHA1_ROUND_(0x8F1BBCDC, vsha1pq_u32, 0, 1, 2, 3, 0, 1)
  FIO_SHA1_ROUND_(0x8F1BBCDC, vsha1mq_u32, 1, 0, 3, 0, 1, 2)
  FIO_SHA1_ROUND_(0x8F1BBCDC, vsha1mq_u32, 0, 1, 0, 1, 2, 3)
  FIO_SHA1_ROUND_(0x8F1BBCDC, vsha1mq_u32, 1, 0, 1, 2, 3, 0)

  FIO_SHA1_ROUND_(0xCA62C1D6, vsha1mq_u32, 0, 1, 2, 3, 0, 1)
  FIO_SHA1_ROUND_(0xCA62C1D6, vsha1mq_u32, 1, 0, 3, 0, 1, 2)
  FIO_SHA1_ROUND_(0xCA62C1D6, vsha1pq_u32, 0, 1, 0, 1, 2, 3)
  FIO_SHA1_ROUND_(0xCA62C1D6, vsha1pq_u32, 1, 0, 1, 2, 3, 0)
#undef FIO_SHA1_ROUND_
  /* round: 68-71 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e1, t1);
  t1 = vaddq_u32(w3, vdupq_n_u32(0xCA62C1D6));
  w0 = vsha1su1q_u32(w0, w3);

  /* round: 72-75 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e0, t0);

  /* round: 76-79 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e1, t1);

  /* combine and store */
  e0 += e_old;
  v0 = vaddq_u32(v_old, v0);
  vst1q_u32(old, v0);
  old[4] = e0;

#else /* Portable implementation */

  uint32_t v[5]; /* working variables a, b, c, d, e */
  for (size_t i = 0; i < 5; ++i)
    v[i] = old[i];

  /* convert read buffer to Big Endian */
  for (size_t i = 0; i < 16; ++i)
    w[i] = fio_ntol32(w[i]);

    /* SHA-1 round function */
#define FIO___SHA1_ROUND(i, k, f)                                              \
  do {                                                                         \
    const uint32_t t = fio_lrot32(v[0], 5) + (f) + v[4] + (k) + w[(i)&15];     \
    v[4] = v[3];                                                               \
    v[3] = v[2];                                                               \
    v[2] = fio_lrot32(v[1], 30);                                               \
    v[1] = v[0];                                                               \
    v[0] = t;                                                                  \
  } while (0)

    /* Message schedule expansion */
#define FIO___SHA1_EXPAND(i)                                                   \
  (w[(i)&15] = fio_lrot32(w[((i) + 13) & 15] ^ w[((i) + 8) & 15] ^             \
                              w[((i) + 2) & 15] ^ w[(i)&15],                   \
                          1))

  /* Rounds 0-15: use message words directly */
  for (size_t i = 0; i < 16; ++i)
    FIO___SHA1_ROUND(i, 0x5A827999UL, fio_ct_mux32(v[1], v[2], v[3]));

  /* Rounds 16-19: Ch function */
  for (size_t i = 16; i < 20; ++i) {
    FIO___SHA1_EXPAND(i);
    FIO___SHA1_ROUND(i, 0x5A827999UL, fio_ct_mux32(v[1], v[2], v[3]));
  }

  /* Rounds 20-39: Parity function */
  for (size_t i = 20; i < 40; ++i) {
    FIO___SHA1_EXPAND(i);
    FIO___SHA1_ROUND(i, 0x6ED9EBA1UL, fio_ct_xor3_32(v[1], v[2], v[3]));
  }

  /* Rounds 40-59: Maj function */
  for (size_t i = 40; i < 60; ++i) {
    FIO___SHA1_EXPAND(i);
    FIO___SHA1_ROUND(i, 0x8F1BBCDCUL, fio_ct_maj32(v[1], v[2], v[3]));
  }

  /* Rounds 60-79: Parity function */
  for (size_t i = 60; i < 80; ++i) {
    FIO___SHA1_EXPAND(i);
    FIO___SHA1_ROUND(i, 0xCA62C1D6UL, fio_ct_xor3_32(v[1], v[2], v[3]));
  }

#undef FIO___SHA1_ROUND
#undef FIO___SHA1_EXPAND

  /* Add compressed chunk to current hash value */
  for (size_t i = 0; i < 5; ++i)
    old[i] += v[i];

#endif /* FIO___HAS_X86_SHA_INTRIN / FIO___HAS_ARM_INTRIN */
}
/**
 * A simple, non streaming, implementation of the SHA1 hashing algorithm.
 *
 * Do NOT use - SHA1 is broken... but for some reason some protocols still
 * require it's use (i.e., WebSockets), so it's here for your convinience.
 */
SFUNC fio_sha1_s fio_sha1(const void *data, uint64_t len) {
  fio_sha1_s s FIO_ALIGN(16) = {.v = {
                                    0x67452301,
                                    0xEFCDAB89,
                                    0x98BADCFE,
                                    0x10325476,
                                    0xC3D2E1F0,
                                }};
  uint32_t vec[16] FIO_ALIGN(16);

  const uint8_t *buf = (const uint8_t *)data;

  for (size_t i = 63; i < len; i += 64) {
    fio_memcpy64(vec, buf);
    fio___sha1_round512(s.v, vec);
    buf += 64;
  }
  for (size_t i = 0; i < 16; ++i) {
    vec[i] = 0;
  }
  if ((len & 63)) {
    uint32_t tbuf[16] = {0};
    fio_memcpy63x(tbuf, buf, len);
    fio_memcpy64(vec, tbuf);
  }
  ((uint8_t *)vec)[(len & 63)] = 0x80;

  if ((len & 63) > 55) {
    fio___sha1_round512(s.v, vec);
    for (size_t i = 0; i < 16; ++i) {
      vec[i] = 0;
    }
  }
  len <<= 3;
  len = fio_lton64(len);
  vec[14] = (uint32_t)(len & 0xFFFFFFFF);
  vec[15] = (uint32_t)(len >> 32);
  fio___sha1_round512(s.v, vec);
  for (size_t i = 0; i < 5; ++i) {
    s.v[i] = fio_ntol32(s.v[i]);
  }
  return s;
}

/** HMAC-SHA1, resulting in a 20 byte authentication code. */
SFUNC fio_sha1_s fio_sha1_hmac(const void *key,
                               uint64_t key_len,
                               const void *msg,
                               uint64_t msg_len) {
  fio_sha1_s inner FIO_ALIGN(16) = {.v =
                                        {
                                            0x67452301,
                                            0xEFCDAB89,
                                            0x98BADCFE,
                                            0x10325476,
                                            0xC3D2E1F0,
                                        }},
                   outer FIO_ALIGN(16) = {.v = {
                                              0x67452301,
                                              0xEFCDAB89,
                                              0x98BADCFE,
                                              0x10325476,
                                              0xC3D2E1F0,
                                          }};
  fio_u512 v = fio_u512_init64(0), k = fio_u512_init64(0);
  uint32_t tmp[16] FIO_ALIGN(16); /* temp buffer for sha1 rounds */
  const uint8_t *buf = (const uint8_t *)msg;

  /* copy key */
  if (key_len > 64)
    goto key_too_long;
  if (key_len == 64)
    fio_memcpy64(k.u8, key);
  else
    fio_memcpy63x(k.u8, key, key_len);
  /* prepare inner key */
  for (size_t i = 0; i < 8; ++i)
    k.u64[i] ^= (uint64_t)0x3636363636363636ULL;

  /* hash inner key block */
  fio_memcpy64(tmp, k.u32);
  fio___sha1_round512(inner.v, tmp);
  /* consume data */
  for (size_t i = 63; i < msg_len; i += 64) {
    fio_memcpy64(tmp, buf);
    fio___sha1_round512(inner.v, tmp);
    buf += 64;
  }
  /* finalize temporary hash */
  if ((msg_len & 63)) {
    v = fio_u512_init64(0);
    fio_memcpy63x(v.u8, buf, msg_len);
  }
  v.u8[(msg_len & 63)] = 0x80;
  if ((msg_len & 63) > 55) {
    fio_memcpy64(tmp, v.u32);
    fio___sha1_round512(inner.v, tmp);
    v = fio_u512_init64(0);
  }
  msg_len += 64; /* add the 64 byte inner key to the length count */
  msg_len <<= 3;
  msg_len = fio_lton64(msg_len);
  v.u32[14] = (uint32_t)(msg_len & 0xFFFFFFFFUL);
  v.u32[15] = (uint32_t)(msg_len >> 32);
  fio_memcpy64(tmp, v.u32);
  fio___sha1_round512(inner.v, tmp);
  for (size_t i = 0; i < 5; ++i)
    inner.v[i] = fio_ntol32(inner.v[i]);

  /* switch key to outer key */
  for (size_t i = 0; i < 8; ++i)
    k.u64[i] ^=
        ((uint64_t)0x3636363636363636ULL ^ (uint64_t)0x5C5C5C5C5C5C5C5CULL);

  /* hash outer key block */
  fio_memcpy64(tmp, k.u32);
  fio___sha1_round512(outer.v, tmp);
  /* hash inner (temporary) hash result and finalize */
  v = fio_u512_init64(0);
  for (size_t i = 0; i < 5; ++i)
    v.u32[i] = inner.v[i];
  v.u8[20] = 0x80;
  msg_len = ((64U + 20U) << 3);
  msg_len = fio_lton64(msg_len);
  v.u32[14] = (uint32_t)(msg_len & 0xFFFFFFFF);
  v.u32[15] = (uint32_t)(msg_len >> 32);
  fio_memcpy64(tmp, v.u32);
  fio___sha1_round512(outer.v, tmp);
  for (size_t i = 0; i < 5; ++i)
    outer.v[i] = fio_ntol32(outer.v[i]);

  return outer;

key_too_long:
  inner = fio_sha1(key, key_len);
  return fio_sha1_hmac(inner.digest, 20, msg, msg_len);
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_SHA1 */
#undef FIO_SHA1
