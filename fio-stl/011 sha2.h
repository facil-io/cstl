/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___   /* Development inclusion - ignore line */
#define FIO_SHA2       /* Development inclusion - ignore line */
#include "./include.h" /* Development inclusion - ignore line */
#endif                 /* Development inclusion - ignore line */
/* *****************************************************************************




                                    SHA 2
                        SHA-256 / SHA-512 and variations



Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#if defined(FIO_SHA2) && !defined(H___FIO_SHA2___H)
#define H___FIO_SHA2___H
/* *****************************************************************************
SHA 2 API
***************************************************************************** */

/** Streaming SHA-256 type. */
typedef struct {
  fio_256u hash;
  fio_512u cache;
  uint64_t total_len;
} fio_sha256_s;

/** A simple, non streaming, implementation of the SHA-256 hashing algorithm. */
FIO_IFUNC fio_256u fio_sha256(const void *data, uint64_t len);

/** initializes a fio_256u so the hash can consume streaming data. */
FIO_IFUNC fio_sha256_s fio_sha256_init();
/** Feed data into the hash */
SFUNC void fio_sha256_consume(fio_sha256_s *h, const void *data, uint64_t len);
/** finalizes a fio_256u with the SHA 256 hash. */
SFUNC fio_256u fio_sha256_finalize(fio_sha256_s *h);

/** Streaming SHA-512 type. */
typedef struct {
  fio_512u hash;
  fio_1024u cache;
  uint64_t total_len;
} fio_sha512_s;

/** A simple, non streaming, implementation of the SHA-512 hashing algorithm. */
FIO_IFUNC fio_512u fio_sha512(const void *data, uint64_t len);

/** initializes a fio_512u so the hash can consume streaming data. */
FIO_IFUNC fio_sha512_s fio_sha512_init();
/** Feed data into the hash */
SFUNC void fio_sha512_consume(fio_sha512_s *h, const void *data, uint64_t len);
/** finalizes a fio_512u with the SHA 512 hash. */
SFUNC fio_512u fio_sha512_finalize(fio_sha512_s *h);

/* *****************************************************************************
Implementation - static / inline functions.
***************************************************************************** */

/** initializes a fio_256u so the hash can be consumed. */
FIO_IFUNC fio_sha256_s fio_sha256_init() {
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
FIO_IFUNC fio_256u fio_sha256(const void *data, uint64_t len) {
  fio_sha256_s h = fio_sha256_init();
  fio_sha256_consume(&h, data, len);
  return fio_sha256_finalize(&h);
}

/** initializes a fio_256u so the hash can be consumed. */
FIO_IFUNC fio_sha512_s fio_sha512_init() {
  fio_sha512_s h = {.hash.u64 = {0ULL}}; /* TODO! */
  return h;
}

/** A simple, non streaming, implementation of the SHA-256 hashing algorithm. */
FIO_IFUNC fio_512u fio_sha512(const void *data, uint64_t len) {
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

FIO_IFUNC void fio___sha256_round(fio_256u *h, const uint8_t *block) {
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
  const fio_256u old = *h;
  /* read data as an array of 16 big endian 32 bit integers. */
  uint32_t w[16];
  FIO_MEMCPY64(w, block);
  for (size_t i = 0; i < 16; ++i) {
    w[i] = fio_lton32(w[i]); /* no-op on big endien systems */
  }

#define FIO___SHA256_ROUND_INNER_COMMON()                                      \
  const uint32_t t2 = ((h->u32[0] & h->u32[1]) ^ (h->u32[0] & h->u32[2]) ^     \
                       (h->u32[1] & h->u32[2])) +                              \
                      (fio_rrot32(h->u32[0], 2) ^ fio_rrot32(h->u32[0], 13) ^  \
                       fio_rrot32(h->u32[0], 22));                             \
  h->u32[7] = h->u32[6];                                                       \
  h->u32[6] = h->u32[5];                                                       \
  h->u32[5] = h->u32[4];                                                       \
  h->u32[4] = h->u32[3];                                                       \
  h->u32[3] = h->u32[2];                                                       \
  h->u32[2] = h->u32[1];                                                       \
  h->u32[1] = h->u32[0];                                                       \
  h->u32[4] += t1;                                                             \
  h->u32[0] = t1 + t2
  for (size_t i = 0; i < 16; ++i) {
    const uint32_t t1 = h->u32[7] + sha256_consts[i] + w[i] +
                        ((h->u32[4] & h->u32[5]) ^ ((~h->u32[4]) & h->u32[6])) +
                        (fio_rrot32(h->u32[4], 6) ^ fio_rrot32(h->u32[4], 11) ^
                         fio_rrot32(h->u32[4], 25));
    FIO___SHA256_ROUND_INNER_COMMON();
  }
  for (size_t i = 0; i < 48; ++i) { /* expand block */
    w[(i & 15)] =
        (fio_rrot32(w[((i + 14) & 15)], 17) ^
         fio_rrot32(w[((i + 14) & 15)], 19) ^ (w[((i + 14) & 15)] >> 10)) +
        w[((i + 9) & 15)] + w[(i & 15)] +
        (fio_rrot32(w[((i + 1) & 15)], 7) ^ fio_rrot32(w[((i + 1) & 15)], 18) ^
         (w[((i + 1) & 15)] >> 3));
    const uint32_t t1 = h->u32[7] + sha256_consts[i + 16] + w[(i & 15)] +
                        ((h->u32[4] & h->u32[5]) ^ ((~h->u32[4]) & h->u32[6])) +
                        (fio_rrot32(h->u32[4], 6) ^ fio_rrot32(h->u32[4], 11) ^
                         fio_rrot32(h->u32[4], 25));
    FIO___SHA256_ROUND_INNER_COMMON();
  }
  for (size_t i = 0; i < 8; ++i)
    h->u32[i] += old.u32[i]; /* compress block with previous state */
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
      FIO_MEMCPY63x((h->cache.u8 + offset), r, len);
      return;
    }
    /* consume cache */
    const size_t byte2copy = 64UL - offset;
    FIO_MEMCPY63x(h->cache.u8 + offset, r, byte2copy);
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
  FIO_MEMCPY63x(h->cache.u64, r, len);
}

SFUNC fio_256u fio_sha256_finalize(fio_sha256_s *h) {
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

FIO_IFUNC void fio___sha512_round(fio_512u *h, const uint8_t *block) {
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
      FIO_MEMCPY127x((h->cache.u8 + offset), r, len);
      return;
    }
    /* consume cache */
    const size_t byte2copy = 128UL - offset;
    FIO_MEMCPY127x(h->cache.u8 + offset, r, byte2copy);
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
  FIO_MEMCPY63x(h->cache.u64, r, len);
}

/** finalizes a fio_512u with the SHA 512 hash. */
SFUNC fio_512u fio_sha512_finalize(fio_sha512_s *h) {
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
SHA2 Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha256_wrapper)(char *data,
                                                         size_t len) {
  fio_256u h = fio_sha256((const void *)data, (uint64_t)len);
  return (uintptr_t)(h.u64[0]);
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha512_wrapper)(char *data,
                                                         size_t len) {
  fio_512u h = fio_sha512((const void *)data, (uint64_t)len);
  return (uintptr_t)(h.u64[0]);
}

#if HAVE_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha256_open_ssl_wrapper)(char *data,
                                                                  size_t len) {
  fio_256u result;
  SHA256((const unsigned char *)data, len, result.u8);
  return result.u64[0];
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha512_open_ssl_wrapper)(char *data,
                                                                  size_t len) {
  fio_512u result;
  SHA512((const unsigned char *)data, len, result.u8);
  return result.u64[0];
}
#endif /* HAVE_OPENSSL */

FIO_SFUNC void FIO_NAME_TEST(stl, sha2)(void) {
  fprintf(stderr, "* Testing SHA-2\n");
  struct {
    const char *str;
    const char *sha256;
    const char *sha512;
  } data[] = {
      {
          .str = (char *)"abc",
          .sha256 = (char *)"\xBA\x78\x16\xBF\x8F\x01\xCF\xEA\x41\x41\x40\xDE"
                            "\x5D\xAE\x22\x23\xB0\x03\x61\xA3\x96\x17\x7A\x9C"
                            "\xB4\x10\xFF\x61\xF2\x00\x15\xAD",
      },
      {
          .str = (char *)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnl"
                         "mnomnopnopq",
          .sha256 = (char *)"\x24\x8D\x6A\x61\xD2\x06\x38\xB8\xE5\xC0\x26"
                            "\x93\x0C\x3E\x60\x39\xA3\x3C\xE4\x59\x64\xFF"
                            "\x21\x67\xF6\xEC\xED\xD4\x19\xDB\x06\xC1",
      },
      {
          .str = (char *)"The quick brown fox jumps over the lazy dog",
          .sha256 = (char *)"\xD7\xA8\xFB\xB3\x07\xD7\x80\x94\x69\xCA\x9A\xBC"
                            "\xB0\x08\x2E\x4F\x8D\x56\x51\xE4\x6D\x3C\xDB\x76"
                            "\x2D\x02\xD0\xBF\x37\xC9\xE5\x92",
          .sha512 =
              (char *)"\x07\xE5\x47\xD9\x58\x6F\x6A\x73\xF7\x3F\xBA\xC0\x43\x5E"
                      "\xD7\x69\x51\x21\x8F\xB7\xD0\xC8\xD7\x88\xA3\x09\xD7\x85"
                      "\x43\x6B\xBB\x64\x2E\x93\xA2\x52\xA9\x54\xF2\x39\x12\x54"
                      "\x7D\x1E\x8A\x3B\x5E\xD6\xE1\xBF\xD7\x09\x78\x21\x23\x3F"
                      "\xA0\x53\x8F\x3D\xB8\x54\xFE\xE6",
      },
      {
          .str = (char *)"The quick brown fox jumps over the lazy cog",
          .sha256 = (char *)"\xE4\xC4\xD8\xF3\xBF\x76\xB6\x92\xDE\x79\x1A\x17"
                            "\x3E\x05\x32\x11\x50\xF7\xA3\x45\xB4\x64\x84\xFE"
                            "\x42\x7F\x6A\xCC\x7E\xCC\x81\xBE",
          .sha512 =
              (char *)"\x3E\xEE\xE1\xD0\xE1\x17\x33\xEF\x15\x2A\x6C\x29\x50\x3B"
                      "\x3A\xE2\x0C\x4F\x1F\x3C\xDA\x4C\xB2\x6F\x1B\xC1\xA4\x1F"
                      "\x91\xC7\xFE\x4A\xB3\xBD\x86\x49\x40\x49\xE2\x01\xC4\xBD"
                      "\x51\x55\xF3\x1E\xCB\x7A\x3C\x86\x06\x84\x3C\x4C\xC8\xDF"
                      "\xCA\xB7\xDA\x11\xC8\xAE\x50\x45",
      },
      {
          .str = (char *)"",
          .sha256 = (char *)"\xE3\xB0\xC4\x42\x98\xFC\x1C\x14\x9A\xFB\xF4\xC8"
                            "\x99\x6F\xB9\x24\x27\xAE\x41\xE4\x64\x9B\x93\x4C"
                            "\xA4\x95\x99\x1B\x78\x52\xB8\x55",
          .sha512 =
              (char *)"\xCF\x83\xE1\x35\x7E\xEF\xB8\xBD\xF1\x54\x28\x50\xD6\x6D"
                      "\x80\x07\xD6\x20\xE4\x05\x0B\x57\x15\xDC\x83\xF4\xA9\x21"
                      "\xD3\x6C\xE9\xCE\x47\xD0\xD1\x3C\x5D\x85\xF2\xB0\xFF\x83"
                      "\x18\xD2\x87\x7E\xEC\x2F\x63\xB9\x31\xBD\x47\x41\x7A\x81"
                      "\xA5\x38\x32\x7A\xF9\x27\xDA\x3E",
      },
  };
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    if (!data[i].str)
      continue;
    if (data[i].sha256) {
      fio_256u sha256 = fio_sha256(data[i].str, strlen(data[i].str));
      FIO_ASSERT(!memcmp(sha256.u8, data[i].sha256, 32),
                 "SHA256 mismatch for \"%s\":\n\t %X%X%X%X...%X%X%X%X",
                 data[i].str,
                 sha256.u8[0],
                 sha256.u8[1],
                 sha256.u8[2],
                 sha256.u8[3],
                 sha256.u8[28],
                 sha256.u8[29],
                 sha256.u8[30],
                 sha256.u8[31]);
    }
    // if (data[i].sha512) {
    //   fio_512u sha512 = fio_sha512(data[i].str, strlen(data[i].str));
    //   FIO_ASSERT(!memcmp(sha512.u8, data[i].sha512, 64),
    //              "SHA512 mismatch for \"%s\"",
    //              data[i].str);
    // }
  }
#if !DEBUG
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha256_wrapper),
                         (char *)"fio_sha256",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha256_wrapper),
                         (char *)"fio_sha256",
                         13,
                         0,
                         1);
  // fio_test_hash_function(FIO_NAME_TEST(stl, __sha512_wrapper),
  //                        (char *)"fio_sha512",
  //                        5,
  //                        0,
  //                        0);
  // fio_test_hash_function(FIO_NAME_TEST(stl, __sha512_wrapper),
  //                        (char *)"fio_sha512",
  //                        13,
  //                        0,
  //                        1);
#if HAVE_OPENSSL
  fprintf(stderr, "* Comparing to " OPENSSL_VERSION_TEXT "\n");
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha256_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-256",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha256_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-256",
                         13,
                         0,
                         1);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha512_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-512",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha512_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-512",
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
#endif /* FIO_SHA2 */
#undef FIO_SHA2
