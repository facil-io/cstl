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
#if FIO___HAS_ARM_INTRIN
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

#else /* !FIO___HAS_ARM_INTRIN portable implementation */

  uint32_t v[8] = {0}; /* copy old state to new + reserve registers (8 not 6) */
  for (size_t i = 0; i < 5; ++i)
    v[i] = old[i];

  for (size_t i = 0; i < 16; ++i) /* convert read buffer to Big Endian */
    w[i] = fio_ntol32(w[i]);

#define FIO___SHA1_ROUND4(K, F, i)                                             \
  FIO___SHA1_ROUND((K), (F), i);                                               \
  FIO___SHA1_ROUND((K), (F), i + 1);                                           \
  FIO___SHA1_ROUND((K), (F), i + 2);                                           \
  FIO___SHA1_ROUND((K), (F), i + 3);
#define FIO___SHA1_ROUND16(K, F, i)                                            \
  FIO___SHA1_ROUND4((K), (F), i);                                              \
  FIO___SHA1_ROUND4((K), (F), i + 4);                                          \
  FIO___SHA1_ROUND4((K), (F), i + 8);                                          \
  FIO___SHA1_ROUND4((K), (F), i + 12);
#define FIO___SHA1_ROUND20(K, F, i)                                            \
  FIO___SHA1_ROUND16(K, F, i);                                                 \
  FIO___SHA1_ROUND4((K), (F), i + 16);

#define FIO___SHA1_ROTATE_OLD(K, F, i)                                         \
  v[5] = fio_lrot32(v[0], 5) + v[4] + F + (uint32_t)K + w[(i)&15];             \
  v[4] = v[3];                                                                 \
  v[3] = v[2];                                                                 \
  v[2] = fio_lrot32(v[1], 30);                                                 \
  v[1] = v[0];                                                                 \
  v[0] = v[5];

#define FIO___SHA1_ROTATE(K, F, i)                                             \
  v[5] = fio_lrot32(v[0], 5) + v[4] + F + (uint32_t)K + w[(i)&15];             \
  v[1] = fio_lrot32(v[1], 30);                                                 \
  fio_u32x8_reshuffle(v, 5, 0, 1, 2, 3, 5, 6, 7);

#define FIO___SHA1_CALC_WORD(i)                                                \
  fio_lrot32(                                                                  \
      (w[(i + 13) & 15] ^ w[(i + 8) & 15] ^ w[(i + 2) & 15] ^ w[(i)&15]),      \
      1);

#define FIO___SHA1_ROUND(K, F, i) FIO___SHA1_ROTATE(K, F, i);
  /* perform first 16 rounds with simple words as copied from data */
  FIO___SHA1_ROUND16(0x5A827999, ((v[1] & v[2]) | ((~v[1]) & (v[3]))), 0);

/* change round definition so now we compute the word's value per round */
#undef FIO___SHA1_ROUND
#define FIO___SHA1_ROUND(K, F, i)                                              \
  w[(i)&15] = FIO___SHA1_CALC_WORD(i);                                         \
  FIO___SHA1_ROTATE(K, F, i);

  /* complete last 4 round from the first 20 round group */
  FIO___SHA1_ROUND4(0x5A827999, ((v[1] & v[2]) | ((~v[1]) & (v[3]))), 16);

  /* remaining 20 round groups */
  FIO___SHA1_ROUND20(0x6ED9EBA1, (v[1] ^ v[2] ^ v[3]), 20);
  FIO___SHA1_ROUND20(0x8F1BBCDC, ((v[1] & (v[2] | v[3])) | (v[2] & v[3])), 40);
  FIO___SHA1_ROUND20(0xCA62C1D6, (v[1] ^ v[2] ^ v[3]), 60);
  /* sum and store */
  for (size_t i = 0; i < 5; ++i)
    old[i] += v[i];

#undef FIO___SHA1_ROTATE
#undef FIO___SHA1_ROTATE_OLD
#undef FIO___SHA1_CALC_WORD
#undef FIO___SHA1_ROUND
#undef FIO___SHA1_ROUND4
#undef FIO___SHA1_ROUND16
#undef FIO___SHA1_ROUND20
#endif /* FIO___HAS_ARM_INTRIN */
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
  fio___sha1_round512(inner.v, k.u32);
  /* consume data */
  for (size_t i = 63; i < msg_len; i += 64) {
    fio_memcpy64(v.u8, buf);
    fio___sha1_round512(inner.v, v.u32);
    buf += 64;
  }
  /* finalize temporary hash */
  if ((msg_len & 63)) {
    v = fio_u512_init64(0);
    fio_memcpy63x(v.u8, buf, msg_len);
  }
  v.u8[(msg_len & 63)] = 0x80;
  if ((msg_len & 63) > 55) {
    fio___sha1_round512(inner.v, v.u32);
    v = fio_u512_init64(0);
  }
  msg_len += 64; /* add the 64 byte inner key to the length count */
  msg_len <<= 3;
  msg_len = fio_lton64(msg_len);
  v.u32[14] = (uint32_t)(msg_len & 0xFFFFFFFFUL);
  v.u32[15] = (uint32_t)(msg_len >> 32);
  fio___sha1_round512(inner.v, v.u32);
  for (size_t i = 0; i < 5; ++i)
    inner.v[i] = fio_ntol32(inner.v[i]);

  /* switch key to outer key */
  for (size_t i = 0; i < 8; ++i)
    k.u64[i] ^=
        ((uint64_t)0x3636363636363636ULL ^ (uint64_t)0x5C5C5C5C5C5C5C5CULL);

  /* hash outer key block */
  fio___sha1_round512(outer.v, k.u32);
  /* hash inner (temporary) hash result and finalize */
  v = fio_u512_init64(0);
  for (size_t i = 0; i < 5; ++i)
    v.u32[i] = inner.v[i];
  v.u8[20] = 0x80;
  msg_len = ((64U + 20U) << 3);
  msg_len = fio_lton64(msg_len);
  v.u32[14] = (uint32_t)(msg_len & 0xFFFFFFFF);
  v.u32[15] = (uint32_t)(msg_len >> 32);
  fio___sha1_round512(outer.v, v.u32);
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
