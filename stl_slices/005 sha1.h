/* *****************************************************************************
Copyright: Boaz Segev, 2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_BITWISE                 /* Development inclusion - ignore line */
#define FIO_SHA1                    /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                                    SHA 1










***************************************************************************** */
#ifdef FIO_SHA1
/* *****************************************************************************
SHA 1
***************************************************************************** */

/** The data tyope containing the SHA1 digest (result). */
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
#ifdef FIO_EXTERN_COMPLETE

FIO_IFUNC void fio___sha1_round512(fio_sha1_s *old, /* state */
                                   uint32_t *w /* 16 words */) {

  register uint32_t v0 = old->v[0];
  register uint32_t v1 = old->v[1];
  register uint32_t v2 = old->v[2];
  register uint32_t v3 = old->v[3];
  register uint32_t v4 = old->v[4];
  register uint32_t v5;

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
}

/**
 * A simple, non streaming, implementation of the SHA1 hashing algorithm.
 *
 * Do NOT use - SHA1 is broken... but for some reason some protocols still
 * require it's use (i.e., WebSockets), so it's here for your convinience.
 */
SFUNC fio_sha1_s fio_sha1(const void *data, uint64_t len) {
  /* TODO: hash */

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
    FIO_MEMCPY(vec, buf, 64);
    fio___sha1_round512(&s, vec);
    buf += 64;
  }
  memset(vec, 0, sizeof(vec));
  if ((len & 63)) {
    FIO_MEMCPY(vec, buf, (len & 63));
  }
  ((uint8_t *)vec)[(len & 63)] = 0x80;

  if ((len & 63) > 55) {
    fio___sha1_round512(&s, vec);
    memset(vec, 0, sizeof(vec));
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
  /* test based on code from BearSSL with credit to Thomas Pornin */
  uintptr_t result[6];
  SHA_CTX o_sh1;
  SHA1_Init(&o_sh1);
  SHA1_Update(&o_sh1, data, len);
  SHA1_Final((unsigned char *)result, &o_sh1);
  return result[0];
}

#endif

FIO_SFUNC void FIO_NAME_TEST(stl, sha1)(void) {
  fprintf(stderr, "* Testing SHA1\n");
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
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_open_ssl_wrapper),
                         (char *)"OpenSSL SHA1",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_open_ssl_wrapper),
                         (char *)"OpenSSL SHA1",
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
