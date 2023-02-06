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

/** returns the digest length of SHA1 in bytes */
FIO_IFUNC size_t fio_sha1_len(void);

/** returns the digest of a SHA1 object. */
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

#if !defined(DEBUG) && defined(__ARM_FEATURE_CRYPTO) &&                        \
    __ARM_FEATURE_CRYPTO && defined(__ARM_NEON)
#include <arm_acle.h>
#include <arm_neon.h>
#define FIO___SHA1_ARM_INTRIN 1
#endif

FIO_IFUNC void fio___sha1_round512(fio_sha1_s *old, /* state */
                                   uint32_t *w /* 16 words */) {
#if FIO___SHA1_ARM_INTRIN
  /* Code adjusted from:
   * https://github.com/noloader/SHA-Intrinsics/blob/master/sha1-arm.c
   * Credit to Jeffrey Walton.
   */
  uint32x4_t w0, w1, w2, w3;
  uint32x4_t t0, t1, v0, v_old;
  uint32_t e0, e1, e_old;
  e0 = e_old = old->v[4];
  v_old = vld1q_u32(old->v);
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

  /* round: 12-15 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1cq_u32(v0, e1, t1);
  t1 = vaddq_u32(w1, vdupq_n_u32(0x6ED9EBA1));
  w2 = vsha1su1q_u32(w2, w1);
  w3 = vsha1su0q_u32(w3, w0, w1);

  /* round: 16-19 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1cq_u32(v0, e0, t0);
  t0 = vaddq_u32(w2, vdupq_n_u32(0x6ED9EBA1));
  w3 = vsha1su1q_u32(w3, w2);
  w0 = vsha1su0q_u32(w0, w1, w2);

  /* round: 20-23 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e1, t1);
  t1 = vaddq_u32(w3, vdupq_n_u32(0x6ED9EBA1));
  w0 = vsha1su1q_u32(w0, w3);
  w1 = vsha1su0q_u32(w1, w2, w3);

  /* round: 24-27 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e0, t0);
  t0 = vaddq_u32(w0, vdupq_n_u32(0x6ED9EBA1));
  w1 = vsha1su1q_u32(w1, w0);
  w2 = vsha1su0q_u32(w2, w3, w0);

  /* round: 28-31 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e1, t1);
  t1 = vaddq_u32(w1, vdupq_n_u32(0x6ED9EBA1));
  w2 = vsha1su1q_u32(w2, w1);
  w3 = vsha1su0q_u32(w3, w0, w1);

  /* round: 32-35 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e0, t0);
  t0 = vaddq_u32(w2, vdupq_n_u32(0x8F1BBCDC));
  w3 = vsha1su1q_u32(w3, w2);
  w0 = vsha1su0q_u32(w0, w1, w2);

  /* round: 36-39 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e1, t1);
  t1 = vaddq_u32(w3, vdupq_n_u32(0x8F1BBCDC));
  w0 = vsha1su1q_u32(w0, w3);
  w1 = vsha1su0q_u32(w1, w2, w3);

  /* round: 40-43 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1mq_u32(v0, e0, t0);
  t0 = vaddq_u32(w0, vdupq_n_u32(0x8F1BBCDC));
  w1 = vsha1su1q_u32(w1, w0);
  w2 = vsha1su0q_u32(w2, w3, w0);

  /* round: 44-47 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1mq_u32(v0, e1, t1);
  t1 = vaddq_u32(w1, vdupq_n_u32(0x8F1BBCDC));
  w2 = vsha1su1q_u32(w2, w1);
  w3 = vsha1su0q_u32(w3, w0, w1);

  /* round: 48-51 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1mq_u32(v0, e0, t0);
  t0 = vaddq_u32(w2, vdupq_n_u32(0x8F1BBCDC));
  w3 = vsha1su1q_u32(w3, w2);
  w0 = vsha1su0q_u32(w0, w1, w2);

  /* round: 52-55 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1mq_u32(v0, e1, t1);
  t1 = vaddq_u32(w3, vdupq_n_u32(0xCA62C1D6));
  w0 = vsha1su1q_u32(w0, w3);
  w1 = vsha1su0q_u32(w1, w2, w3);

  /* round: 56-59 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1mq_u32(v0, e0, t0);
  t0 = vaddq_u32(w0, vdupq_n_u32(0xCA62C1D6));
  w1 = vsha1su1q_u32(w1, w0);
  w2 = vsha1su0q_u32(w2, w3, w0);

  /* round: 60-63 */
  e0 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e1, t1);
  t1 = vaddq_u32(w1, vdupq_n_u32(0xCA62C1D6));
  w2 = vsha1su1q_u32(w2, w1);
  w3 = vsha1su0q_u32(w3, w0, w1);

  /* round: 64-67 */
  e1 = vsha1h_u32(vgetq_lane_u32(v0, 0));
  v0 = vsha1pq_u32(v0, e0, t0);
  t0 = vaddq_u32(w2, vdupq_n_u32(0xCA62C1D6));
  w3 = vsha1su1q_u32(w3, w2);
  w0 = vsha1su0q_u32(w0, w1, w2);

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
  vst1q_u32(old->v, v0);
  old->v[4] = e0;

#else
  register uint32_t v0 = old->v[0];
  register uint32_t v1 = old->v[1];
  register uint32_t v2 = old->v[2];
  register uint32_t v3 = old->v[3];
  register uint32_t v4 = old->v[4];
  register uint32_t v5;

  // vsha1h_u32(uint32_t __p0)

#define FIO___SHA1_ROTATE(K, F, i)                                             \
  v5 = fio_lrot32(v0, 5) + v4 + F + (uint32_t)K + w[(i)&15];                   \
  v4 = v3;                                                                     \
  v3 = v2;                                                                     \
  v2 = fio_lrot32(v1, 30);                                                     \
  v1 = v0;                                                                     \
  v0 = v5;
#define FIO___SHA1_CALC_WORD(i)                                                \
  fio_lrot32(                                                                  \
      (w[(i + 13) & 15] ^ w[(i + 8) & 15] ^ w[(i + 2) & 15] ^ w[(i)&15]),      \
      1);

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

#define FIO___SHA1_ROUND(K, F, i)                                              \
  w[i] = fio_ntol32(w[i]);                                                     \
  FIO___SHA1_ROTATE(K, F, i);

  FIO___SHA1_ROUND16(0x5A827999, ((v1 & v2) | ((~v1) & (v3))), 0);

#undef FIO___SHA1_ROUND
#define FIO___SHA1_ROUND(K, F, i)                                              \
  w[(i)&15] = FIO___SHA1_CALC_WORD(i);                                         \
  FIO___SHA1_ROTATE(K, F, i);

  FIO___SHA1_ROUND4(0x5A827999, ((v1 & v2) | ((~v1) & (v3))), 16);

  FIO___SHA1_ROUND20(0x6ED9EBA1, (v1 ^ v2 ^ v3), 20);
  FIO___SHA1_ROUND20(0x8F1BBCDC, ((v1 & (v2 | v3)) | (v2 & v3)), 40);
  FIO___SHA1_ROUND20(0xCA62C1D6, (v1 ^ v2 ^ v3), 60);

  old->v[0] += v0;
  old->v[1] += v1;
  old->v[2] += v2;
  old->v[3] += v3;
  old->v[4] += v4;

#undef FIO___SHA1_ROTATE
#undef FIO___SHA1_CALC_WORD
#undef FIO___SHA1_ROUND
#undef FIO___SHA1_ROUND4
#undef FIO___SHA1_ROUND16
#undef FIO___SHA1_ROUND20
#endif /* FIO___SHA1_ARM_INTRIN */
#undef FIO___SHA1_ARM_INTRIN
}
/**
 * A simple, non streaming, implementation of the SHA1 hashing algorithm.
 *
 * Do NOT use - SHA1 is broken... but for some reason some protocols still
 * require it's use (i.e., WebSockets), so it's here for your convinience.
 */
SFUNC fio_sha1_s fio_sha1(const void *data, uint64_t len) {
  fio_sha1_s s = (fio_sha1_s){
      .v =
          {
              0x67452301,
              0xEFCDAB89,
              0x98BADCFE,
              0x10325476,
              0xC3D2E1F0,
          },
  };

  const uint8_t *buf = (const uint8_t *)data;

  uint32_t vec[16];

  for (size_t i = 63; i < len; i += 64) {
    fio_memcpy64(vec, buf);
    fio___sha1_round512(&s, vec);
    buf += 64;
  }
  FIO_MEMSET(vec, 0, sizeof(vec));
  if ((len & 63)) {
    fio_memcpy63x(vec, buf, len);
  }
  ((uint8_t *)vec)[(len & 63)] = 0x80;

  if ((len & 63) > 55) {
    fio___sha1_round512(&s, vec);
    FIO_MEMSET(vec, 0, sizeof(vec));
  }

  fio_u2buf64((void *)(vec + 14), (len << 3));
  fio___sha1_round512(&s, vec);

  s.v[0] = fio_ntol32(s.v[0]);
  s.v[1] = fio_ntol32(s.v[1]);
  s.v[2] = fio_ntol32(s.v[2]);
  s.v[3] = fio_ntol32(s.v[3]);
  s.v[4] = fio_ntol32(s.v[4]);
  return s;
}

/* *****************************************************************************
SHA1 Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha1_wrapper)(char *data, size_t len) {
  fio_sha1_s h = fio_sha1((const void *)data, (uint64_t)len);
  return *(uintptr_t *)h.digest;
}

#if HAVE_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha1_open_ssl_wrapper)(char *data,
                                                                size_t len) {
  fio_256u result;
  SHA1((const unsigned char *)data, len, result.u8);
  return result.u64[0];
}

#endif

FIO_SFUNC void FIO_NAME_TEST(stl, sha1)(void) {
  fprintf(stderr, "* Testing SHA-1\n");
  struct {
    const char *str;
    const char *sha1;
  } data[] = {
      {
          .str = "",
          .sha1 = "\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18"
                  "\x90\xaf\xd8\x07\x09",
      },
      {
          .str = "The quick brown fox jumps over the lazy dog",
          .sha1 = "\x2f\xd4\xe1\xc6\x7a\x2d\x28\xfc\xed\x84\x9e\xe1\xbb\x76\xe7"
                  "\x39\x1b\x93\xeb\x12",
      },
      {
          .str = "The quick brown fox jumps over the lazy cog",
          .sha1 = "\xde\x9f\x2c\x7f\xd2\x5e\x1b\x3a\xfa\xd3\xe8\x5a\x0b\xd1\x7d"
                  "\x9b\x10\x0d\xb4\xb3",
      },
  };
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    fio_sha1_s sha1 = fio_sha1(data[i].str, strlen(data[i].str));

    FIO_ASSERT(!memcmp(sha1.digest, data[i].sha1, fio_sha1_len()),
               "SHA1 mismatch for \"%s\"",
               data[i].str);
  }
#if !DEBUG
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_wrapper),
                         (char *)"fio_sha1",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_wrapper),
                         (char *)"fio_sha1",
                         13,
                         0,
                         1);
#if HAVE_OPENSSL
  fprintf(stderr, "* Comparing to " OPENSSL_VERSION_TEXT "\n");
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-1",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-1",
                         13,
                         0,
                         1);
#endif /* HAVE_OPENSSL */
#endif /* !DEBUG */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_SHA1 */
#undef FIO_SHA1
