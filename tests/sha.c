/* *****************************************************************************
Test for 152 sha1.h and 152 sha2.h

Coverage: SHA-1 and SHA-2 (SHA-256 / SHA-512) one-shot and streaming APIs,
known-answer vectors from NIST and RFC 4634, RFC 4231 HMAC-SHA-256/512 test
vectors, incremental hashing consistency, and OpenSSL cross-validation when
available. Performance loops are intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SHA1
#define FIO_SHA2
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL
#include <openssl/hmac.h>
#include <openssl/sha.h>
#endif

/* *****************************************************************************
SHA-1 Known-Answer Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, sha1_kat)(void) {
  struct {
    const char *str;
    const uint8_t digest[20];
  } data[] = {
      {
          .str = "",
          .digest = {0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32,
                     0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8,
                     0x07, 0x09},
      },
      {
          .str = "The quick brown fox jumps over the lazy dog",
          .digest = {0x2f, 0xd4, 0xe1, 0xc6, 0x7a, 0x2d, 0x28, 0xfc, 0xed,
                     0x84, 0x9e, 0xe1, 0xbb, 0x76, 0xe7, 0x39, 0x1b, 0x93,
                     0xeb, 0x12},
      },
      {
          .str = "The quick brown fox jumps over the lazy cog",
          .digest = {0xde, 0x9f, 0x2c, 0x7f, 0xd2, 0x5e, 0x1b, 0x3a, 0xfa,
                     0xd3, 0xe8, 0x5a, 0x0b, 0xd1, 0x7d, 0x9b, 0x10, 0x0d,
                     0xb4, 0xb3},
      },
  };

  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    fio_sha1_s sha1 = fio_sha1(data[i].str, FIO_STRLEN(data[i].str));
    FIO_ASSERT(!FIO_MEMCMP(sha1.digest, data[i].digest, 20),
               "SHA-1 mismatch for \"%s\"",
               data[i].str);
  }
}

/* *****************************************************************************
SHA-1 padding boundary + HMAC-SHA1 Known-Answer Tests (FIPS 180, RFC 2202)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, sha1_hex2bin)(uint8_t *dest, const char *hex) {
  while (hex[0] && hex[1]) {
    uint8_t hi = (uint8_t)(hex[0] <= '9' ? hex[0] - '0' : (hex[0] | 32) - 87);
    uint8_t lo = (uint8_t)(hex[1] <= '9' ? hex[1] - '0' : (hex[1] | 32) - 87);
    *dest++ = (uint8_t)((hi << 4) | lo);
    hex += 2;
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, sha1_hmac_kat)(void) {
  uint8_t expected[20];
  uint8_t key[80];
  uint8_t data[50];
  /* 56 byte message: length field forced into a second block */
  {
    const char *str =
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    fio_sha1_s r = fio_sha1(str, 56);
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected, "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    FIO_ASSERT(!FIO_MEMCMP(r.digest, expected, 20), "SHA-1 56 byte KAT");
    r = fio_sha1("abc", 3);
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected, "a9993e364706816aba3e25717850c26c9cd0d89d");
    FIO_ASSERT(!FIO_MEMCMP(r.digest, expected, 20), "SHA-1 \"abc\" KAT");
  }
  /* RFC 2202 test case 1 */
  FIO_MEMSET(key, 0x0b, 20);
  {
    fio_sha1_s r = fio_sha1_hmac(key, 20, "Hi There", 8);
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected, "b617318655057264e28bc0b6fb378c8ef146be00");
    FIO_ASSERT(!FIO_MEMCMP(r.digest, expected, 20), "HMAC-SHA1 RFC 2202 #1");
  }
  /* RFC 2202 test case 2 */
  {
    fio_sha1_s r = fio_sha1_hmac("Jefe", 4, "what do ya want for nothing?", 28);
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected, "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");
    FIO_ASSERT(!FIO_MEMCMP(r.digest, expected, 20), "HMAC-SHA1 RFC 2202 #2");
  }
  /* RFC 2202 test case 3 */
  FIO_MEMSET(key, 0xaa, 80);
  FIO_MEMSET(data, 0xdd, 50);
  {
    fio_sha1_s r = fio_sha1_hmac(key, 20, data, 50);
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected, "125d7342b9ac11cd91a39af48aa17b4f63f175d3");
    FIO_ASSERT(!FIO_MEMCMP(r.digest, expected, 20), "HMAC-SHA1 RFC 2202 #3");
  }
  /* RFC 2202 test case 6 (key longer than block size) */
  {
    const char *msg = "Test Using Larger Than Block-Size Key - Hash Key First";
    fio_sha1_s r = fio_sha1_hmac(key, 80, msg, FIO_STRLEN(msg));
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected, "aa4ae5e15272d00e95705637ce8a3b55ed402112");
    FIO_ASSERT(!FIO_MEMCMP(r.digest, expected, 20), "HMAC-SHA1 RFC 2202 #6");
  }
  /* RFC 2202 test case 7 (long key, data longer than one block) */
  {
    const char *msg = "Test Using Larger Than Block-Size Key and Larger Than "
                      "One Block-Size Data";
    fio_sha1_s r = fio_sha1_hmac(key, 80, msg, FIO_STRLEN(msg));
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected, "e8e99d0f45237d786d6bbaa7965c7808bbff1a91");
    FIO_ASSERT(!FIO_MEMCMP(r.digest, expected, 20), "HMAC-SHA1 RFC 2202 #7");
  }
}

/* *****************************************************************************
SHA-2 Known-Answer Tests (NIST / RFC 4634 vectors)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, sha2_kat)(void) {
  struct {
    const char *str;
    const uint8_t sha256[32];
    const uint8_t sha512[64];
  } data[] = {
      {
          .str = "abc",
          .sha256 = {0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41,
                     0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03,
                     0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff,
                     0x61, 0xf2, 0x00, 0x15, 0xad},
          .sha512 = {0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc,
                     0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31, 0x12, 0xe6,
                     0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2, 0x0a, 0x9e, 0xee,
                     0xe6, 0x4b, 0x55, 0xd3, 0x9a, 0x21, 0x92, 0x99, 0x2a,
                     0x27, 0x4f, 0xc1, 0xa8, 0x36, 0xba, 0x3c, 0x23, 0xa3,
                     0xfe, 0xeb, 0xbd, 0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c,
                     0xe8, 0x0e, 0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4,
                     0x9f},
      },
      {
          .str = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
          .sha256 = {0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8, 0xe5,
                     0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39, 0xa3, 0x3c,
                     0xe4, 0x59, 0x64, 0xff, 0x21, 0x67, 0xf6, 0xec, 0xed,
                     0xd4, 0x19, 0xdb, 0x06, 0xc1},
          .sha512 = {0x20, 0x4a, 0x8f, 0xc6, 0xdd, 0xa8, 0x2f, 0x0a, 0x0c,
                     0xed, 0x7b, 0xeb, 0x8e, 0x08, 0xa4, 0x16, 0x57, 0xc1,
                     0x6e, 0xf4, 0x68, 0xb2, 0x28, 0xa8, 0x27, 0x9b, 0xe3,
                     0x31, 0xa7, 0x03, 0xc3, 0x35, 0x96, 0xfd, 0x15, 0xc1,
                     0x3b, 0x1b, 0x07, 0xf9, 0xaa, 0x1d, 0x3b, 0xea, 0x57,
                     0x78, 0x9c, 0xa0, 0x31, 0xad, 0x85, 0xc7, 0xa7, 0x1d,
                     0xd7, 0x03, 0x54, 0xec, 0x63, 0x12, 0x38, 0xca, 0x34,
                     0x45},
      },
      {
          .str = "The quick brown fox jumps over the lazy dog",
          .sha256 = {0xd7, 0xa8, 0xfb, 0xb3, 0x07, 0xd7, 0x80, 0x94, 0x69,
                     0xca, 0x9a, 0xbc, 0xb0, 0x08, 0x2e, 0x4f, 0x8d, 0x56,
                     0x51, 0xe4, 0x6d, 0x3c, 0xdb, 0x76, 0x2d, 0x02, 0xd0,
                     0xbf, 0x37, 0xc9, 0xe5, 0x92},
          .sha512 = {0x07, 0xe5, 0x47, 0xd9, 0x58, 0x6f, 0x6a, 0x73, 0xf7,
                     0x3f, 0xba, 0xc0, 0x43, 0x5e, 0xd7, 0x69, 0x51, 0x21,
                     0x8f, 0xb7, 0xd0, 0xc8, 0xd7, 0x88, 0xa3, 0x09, 0xd7,
                     0x85, 0x43, 0x6b, 0xbb, 0x64, 0x2e, 0x93, 0xa2, 0x52,
                     0xa9, 0x54, 0xf2, 0x39, 0x12, 0x54, 0x7d, 0x1e, 0x8a,
                     0x3b, 0x5e, 0xd6, 0xe1, 0xbf, 0xd7, 0x09, 0x78, 0x21,
                     0x23, 0x3f, 0xa0, 0x53, 0x8f, 0x3d, 0xb8, 0x54, 0xfe,
                     0xe6},
      },
      {
          .str = "The quick brown fox jumps over the lazy cog",
          .sha256 = {0xe4, 0xc4, 0xd8, 0xf3, 0xbf, 0x76, 0xb6, 0x92, 0xde,
                     0x79, 0x1a, 0x17, 0x3e, 0x05, 0x32, 0x11, 0x50, 0xf7,
                     0xa3, 0x45, 0xb4, 0x64, 0x84, 0xfe, 0x42, 0x7f, 0x6a,
                     0xcc, 0x7e, 0xcc, 0x81, 0xbe},
          .sha512 = {0x3e, 0xee, 0xe1, 0xd0, 0xe1, 0x17, 0x33, 0xef, 0x15,
                     0x2a, 0x6c, 0x29, 0x50, 0x3b, 0x3a, 0xe2, 0x0c, 0x4f,
                     0x1f, 0x3c, 0xda, 0x4c, 0xb2, 0x6f, 0x1b, 0xc1, 0xa4,
                     0x1f, 0x91, 0xc7, 0xfe, 0x4a, 0xb3, 0xbd, 0x86, 0x49,
                     0x40, 0x49, 0xe2, 0x01, 0xc4, 0xbd, 0x51, 0x55, 0xf3,
                     0x1e, 0xcb, 0x7a, 0x3c, 0x86, 0x06, 0x84, 0x3c, 0x4c,
                     0xc8, 0xdf, 0xca, 0xb7, 0xda, 0x11, 0xc8, 0xae, 0x50,
                     0x45},
      },
      {
          .str = "",
          .sha256 = {0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a,
                     0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae,
                     0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99,
                     0x1b, 0x78, 0x52, 0xb8, 0x55},
          .sha512 = {0xcf, 0x83, 0xe1, 0x35, 0x7e, 0xef, 0xb8, 0xbd, 0xf1,
                     0x54, 0x28, 0x50, 0xd6, 0x6d, 0x80, 0x07, 0xd6, 0x20,
                     0xe4, 0x05, 0x0b, 0x57, 0x15, 0xdc, 0x83, 0xf4, 0xa9,
                     0x21, 0xd3, 0x6c, 0xe9, 0xce, 0x47, 0xd0, 0xd1, 0x3c,
                     0x5d, 0x85, 0xf2, 0xb0, 0xff, 0x83, 0x18, 0xd2, 0x87,
                     0x7e, 0xec, 0x2f, 0x63, 0xb9, 0x31, 0xbd, 0x47, 0x41,
                     0x7a, 0x81, 0xa5, 0x38, 0x32, 0x7a, 0xf9, 0x27, 0xda,
                     0x3e},
      },
  };

  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    fio_u256 sha256 = fio_sha256(data[i].str, FIO_STRLEN(data[i].str));
    FIO_ASSERT(!FIO_MEMCMP(sha256.u8, data[i].sha256, 32),
               "SHA-256 mismatch for \"%s\"",
               data[i].str);

    fio_u512 sha512 = fio_sha512(data[i].str, FIO_STRLEN(data[i].str));
    FIO_ASSERT(!FIO_MEMCMP(sha512.u8, data[i].sha512, 64),
               "SHA-512 mismatch for \"%s\"",
               data[i].str);
  }
}

/* *****************************************************************************
SHA-384 Known-Answer Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, sha384_kat)(void) {
  /* NIST test vector: SHA-384("abc") */
  static const uint8_t abc_sha384[48] = {
      0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b, 0xb5, 0xa0, 0x3d, 0x69,
      0x9a, 0xc6, 0x50, 0x07, 0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
      0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed, 0x80, 0x86, 0x07, 0x2b,
      0xa1, 0xe7, 0xcc, 0x23, 0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7};
  fio_u512 h = fio_sha384("abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(h.u8, abc_sha384, 48),
             "SHA-384 one-shot mismatch (must differ from truncated SHA-512)");

  /* Streaming must match one-shot */
  fio_sha512_s ctx = fio_sha384_init();
  fio_sha384_consume(&ctx, "a", 1);
  fio_sha384_consume(&ctx, "bc", 2);
  fio_u512 hs = fio_sha384_finalize(&ctx);
  FIO_ASSERT(!FIO_MEMCMP(hs.u8, abc_sha384, 48),
             "SHA-384 streaming mismatch");

  /* SHA-384 must NOT equal truncated SHA-512 (different initial values) */
  fio_u512 h512 = fio_sha512("abc", 3);
  FIO_ASSERT(FIO_MEMCMP(h.u8, h512.u8, 48) != 0,
             "SHA-384 must not equal truncated SHA-512");
}

/* *****************************************************************************
SHA-2 Padding Boundary Known-Answer Tests

The final block must hold the 0x80 padding byte plus the message bit length
(8 bytes for SHA-256, 16 bytes for SHA-384/512). Lengths where the remainder
is exactly at the limit (55/56 mod 64, 111/112 mod 128) need an extra block.
Regression: SHA-384/512 produced wrong digests for len % 128 == 112, which
broke ~1/128 TLS 1.3 TLS_AES_256_GCM_SHA384 handshakes (transcript hash).
Digests of 'a' * len, cross-checked with `shasum` and `openssl dgst`. Runs
without OpenSSL (e.g., Windows CI).
***************************************************************************** */

/** Compares `len` bytes of `digest` with a lowercase hex string. */
FIO_SFUNC int fio___test_sha_hex_eq(const uint8_t *digest,
                                    const char *hex,
                                    size_t len) {
  static const char map[] = "0123456789abcdef";
  if (FIO_STRLEN(hex) != (len << 1))
    return 0;
  for (size_t i = 0; i < len; ++i) {
    if (hex[i << 1] != map[digest[i] >> 4] ||
        hex[(i << 1) + 1] != map[digest[i] & 15])
      return 0;
  }
  return 1;
}

FIO_SFUNC void FIO_NAME_TEST(stl, sha2_padding_boundary)(void) {
  static const struct {
    size_t len;
    const char *sha256;
    const char *sha384;
    const char *sha512;
  } kat[] = {
      {55,
       "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318",
       NULL,
       NULL},
      {56,
       "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a",
       NULL,
       NULL},
      {111,
       NULL,
       "3c37955051cb5c3026f94d551d5b5e2ac38d572ae4e07172085fed81f8466b8f90dc23a"
       "8ffcdea0b8d8e58e8fdacc80a",
       "fa9121c7b32b9e01733d034cfc78cbf67f926c7ed83e82200ef86818196921760b4bef"
       "f48404df811b953828274461673c68d04e297b0eb7b2b4d60fc6b566a2"},
      {112,
       NULL,
       "187d4e07cb306103c69967bf544d0dfbe9042577599c73c330abc0cb64c61236d5ed565"
       "ee19119d8c31779a38f791fcd",
       "c01d080efd492776a1c43bd23dd99d0a2e626d481e16782e75d54c2503b5dc32bd05f0"
       "f1ba33e568b88fd2d970929b719ecbb152f58f130a407c8830604b70ca"},
      {113,
       NULL,
       "1d6bed01626682961b50da078a6b1da707c1da0c8a0a3226f159235bd45ed724a0622fa"
       "6f39fd70007a6c72a5cda43ae",
       "55ddd8ac210a6e18ba1ee055af84c966e0dbff091c43580ae1be703bdb85da31acf694"
       "8cf5bd90c55a20e5450f22fb89bd8d0085e39f85a86cc46abbca75e24d"},
      {119,
       "31eba51c313a5c08226adf18d4a359cfdfd8d2e816b13f4af952f7ea6584dcfb",
       NULL,
       NULL},
      {120,
       "2f3d335432c70b580af0e8e1b3674a7c020d683aa5f73aaaedfdc55af904c21c",
       NULL,
       NULL},
      {240,
       NULL,
       "4d86957beab348a29180f02d02564ac1d32f5b4c217ece2b038f7c184f0cafc8c8e438e"
       "b82aa03796170e0a7ce8c0675",
       "4c296d90c61052a62ffb1dd196f1b7b09373b1f93e71836baebf89690546b7595684db"
       "e9467a8e484fa0d1094272b4344a7c24f5fee8daedeb0bf549c985ab5f"},
  };
  uint8_t data[240];
  FIO_MEMSET(data, 'a', sizeof(data));
  for (size_t i = 0; i < sizeof(kat) / sizeof(kat[0]); ++i) {
    const size_t len = kat[i].len;
    const size_t split = len / 3; /* streaming: 3 uneven parts */
    if (kat[i].sha256) {
      fio_u256 h = fio_sha256(data, len);
      FIO_ASSERT(fio___test_sha_hex_eq(h.u8, kat[i].sha256, 32),
                 "SHA-256 padding boundary KAT failed at len=%zu",
                 len);
      fio_sha256_s ctx = fio_sha256_init();
      fio_sha256_consume(&ctx, data, split);
      fio_sha256_consume(&ctx, data + split, 1);
      fio_sha256_consume(&ctx, data + split + 1, len - split - 1);
      h = fio_sha256_finalize(&ctx);
      FIO_ASSERT(fio___test_sha_hex_eq(h.u8, kat[i].sha256, 32),
                 "SHA-256 streaming padding boundary KAT failed at len=%zu",
                 len);
    }
    if (kat[i].sha384) {
      fio_u512 h = fio_sha384(data, len);
      FIO_ASSERT(fio___test_sha_hex_eq(h.u8, kat[i].sha384, 48),
                 "SHA-384 padding boundary KAT failed at len=%zu",
                 len);
      fio_sha512_s ctx = fio_sha384_init();
      fio_sha384_consume(&ctx, data, split);
      fio_sha384_consume(&ctx, data + split, 1);
      fio_sha384_consume(&ctx, data + split + 1, len - split - 1);
      h = fio_sha384_finalize(&ctx);
      FIO_ASSERT(fio___test_sha_hex_eq(h.u8, kat[i].sha384, 48),
                 "SHA-384 streaming padding boundary KAT failed at len=%zu",
                 len);
    }
    if (kat[i].sha512) {
      fio_u512 h = fio_sha512(data, len);
      FIO_ASSERT(fio___test_sha_hex_eq(h.u8, kat[i].sha512, 64),
                 "SHA-512 padding boundary KAT failed at len=%zu",
                 len);
      fio_sha512_s ctx = fio_sha512_init();
      fio_sha512_consume(&ctx, data, split);
      fio_sha512_consume(&ctx, data + split, 1);
      fio_sha512_consume(&ctx, data + split + 1, len - split - 1);
      h = fio_sha512_finalize(&ctx);
      FIO_ASSERT(fio___test_sha_hex_eq(h.u8, kat[i].sha512, 64),
                 "SHA-512 streaming padding boundary KAT failed at len=%zu",
                 len);
    }
  }
}

/* *****************************************************************************
SHA-2 Streaming / Incremental Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, sha2_streaming)(void) {
  uint8_t data[1000];
  fio_rand_bytes(data, sizeof(data));

  /* SHA-256 incremental vs one-shot. */
  {
    fio_u256 one_shot = fio_sha256(data, sizeof(data));

    fio_sha256_s ctx = fio_sha256_init();
    fio_sha256_consume(&ctx, data, 100);
    fio_sha256_consume(&ctx, data + 100, 400);
    fio_sha256_consume(&ctx, data + 500, 500);
    fio_u256 chunked = fio_sha256_finalize(&ctx);
    FIO_ASSERT(!FIO_MEMCMP(one_shot.u8, chunked.u8, 32),
               "SHA-256 chunked incremental mismatch");

    ctx = fio_sha256_init();
    for (size_t i = 0; i < sizeof(data); ++i)
      fio_sha256_consume(&ctx, data + i, 1);
    fio_u256 byte_by_byte = fio_sha256_finalize(&ctx);
    FIO_ASSERT(!FIO_MEMCMP(one_shot.u8, byte_by_byte.u8, 32),
               "SHA-256 byte-by-byte incremental mismatch");
  }

  /* SHA-512 incremental vs one-shot. */
  {
    fio_u512 one_shot = fio_sha512(data, sizeof(data));

    fio_sha512_s ctx = fio_sha512_init();
    fio_sha512_consume(&ctx, data, 100);
    fio_sha512_consume(&ctx, data + 100, 400);
    fio_sha512_consume(&ctx, data + 500, 500);
    fio_u512 chunked = fio_sha512_finalize(&ctx);
    FIO_ASSERT(!FIO_MEMCMP(one_shot.u8, chunked.u8, 64),
               "SHA-512 chunked incremental mismatch");

    ctx = fio_sha512_init();
    for (size_t i = 0; i < sizeof(data); ++i)
      fio_sha512_consume(&ctx, data + i, 1);
    fio_u512 byte_by_byte = fio_sha512_finalize(&ctx);
    FIO_ASSERT(!FIO_MEMCMP(one_shot.u8, byte_by_byte.u8, 64),
               "SHA-512 byte-by-byte incremental mismatch");
  }
}

/* *****************************************************************************
HMAC-SHA-256 / HMAC-SHA-512 Tests (RFC 4231 Test Vectors)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, hmac_rfc4231)(void) {
  /* RFC 4231 Test Case 1 */
  {
    static const uint8_t key[20] = {
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b};
    static const char data[] = "Hi There";
    static const uint8_t expected_256[32] = {
        0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53, 0x5c, 0xa8, 0xaf,
        0xce, 0xaf, 0x0b, 0xf1, 0x2b, 0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83,
        0x3d, 0xa7, 0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7};
    static const uint8_t expected_512[64] = {
        0x87, 0xaa, 0x7c, 0xde, 0xa5, 0xef, 0x61, 0x9d, 0x4f, 0xf0, 0xb4,
        0x24, 0x1a, 0x1d, 0x6c, 0xb0, 0x23, 0x79, 0xf4, 0xe2, 0xce, 0x4e,
        0xc2, 0x78, 0x7a, 0xd0, 0xb3, 0x05, 0x45, 0xe1, 0x7c, 0xde, 0xda,
        0xa8, 0x33, 0xb7, 0xd6, 0xb8, 0xa7, 0x02, 0x03, 0x8b, 0x27, 0x4e,
        0xae, 0xa3, 0xf4, 0xe4, 0xbe, 0x9d, 0x91, 0x4e, 0xeb, 0x61, 0xf1,
        0x70, 0x2e, 0x69, 0x6c, 0x20, 0x3a, 0x12, 0x68, 0x54};
    static const uint8_t expected_384[48] = {
        0xaf, 0xd0, 0x39, 0x44, 0xd8, 0x48, 0x95, 0x62, 0x6b, 0x08, 0x25,
        0xf4, 0xab, 0x46, 0x90, 0x7f, 0x15, 0xf9, 0xda, 0xdb, 0xe4, 0x10,
        0x1e, 0xc6, 0x82, 0xaa, 0x03, 0x4c, 0x7c, 0xeb, 0xc5, 0x9c, 0xfa,
        0xea, 0x9e, 0xa9, 0x07, 0x6e, 0xde, 0x7f, 0x4a, 0xf1, 0x52, 0xe8,
        0xb2, 0xfa, 0x9c, 0xb6};

    fio_u256 result_256 = fio_sha256_hmac(key, 20, data, 8);
    FIO_ASSERT(!FIO_MEMCMP(result_256.u8, expected_256, 32),
               "HMAC-SHA-256 RFC 4231 test case 1 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 20, data, 8);
    FIO_ASSERT(!FIO_MEMCMP(result_512.u8, expected_512, 64),
               "HMAC-SHA-512 RFC 4231 test case 1 failed");

    fio_u512 result_384 = fio_sha384_hmac(key, 20, data, 8);
    FIO_ASSERT(!FIO_MEMCMP(result_384.u8, expected_384, 48),
               "HMAC-SHA-384 RFC 4231 test case 1 failed");
  }

  /* RFC 4231 Test Case 2 - "Jefe" key. */
  {
    static const char key[] = "Jefe";
    static const char data[] = "what do ya want for nothing?";
    static const uint8_t expected_256[32] = {
        0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e, 0x6a, 0x04, 0x24,
        0x26, 0x08, 0x95, 0x75, 0xc7, 0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27,
        0x39, 0x83, 0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43};
    static const uint8_t expected_512[64] = {
        0x16, 0x4b, 0x7a, 0x7b, 0xfc, 0xf8, 0x19, 0xe2, 0xe3, 0x95, 0xfb,
        0xe7, 0x3b, 0x56, 0xe0, 0xa3, 0x87, 0xbd, 0x64, 0x22, 0x2e, 0x83,
        0x1f, 0xd6, 0x10, 0x27, 0x0c, 0xd7, 0xea, 0x25, 0x05, 0x54, 0x97,
        0x58, 0xbf, 0x75, 0xc0, 0x5a, 0x99, 0x4a, 0x6d, 0x03, 0x4f, 0x65,
        0xf8, 0xf0, 0xe6, 0xfd, 0xca, 0xea, 0xb1, 0xa3, 0x4d, 0x4a, 0x6b,
        0x4b, 0x63, 0x6e, 0x07, 0x0a, 0x38, 0xbc, 0xe7, 0x37};
    static const uint8_t expected_384[48] = {
        0xaf, 0x45, 0xd2, 0xe3, 0x76, 0x48, 0x40, 0x31, 0x61, 0x7f, 0x78,
        0xd2, 0xb5, 0x8a, 0x6b, 0x1b, 0x9c, 0x7e, 0xf4, 0x64, 0xf5, 0xa0,
        0x1b, 0x47, 0xe4, 0x2e, 0xc3, 0x73, 0x63, 0x22, 0x44, 0x5e, 0x8e,
        0x22, 0x40, 0xca, 0x5e, 0x69, 0xe2, 0xc7, 0x8b, 0x32, 0x39, 0xec,
        0xfa, 0xb2, 0x16, 0x49};

    fio_u256 result_256 = fio_sha256_hmac(key, 4, data, 28);
    FIO_ASSERT(!FIO_MEMCMP(result_256.u8, expected_256, 32),
               "HMAC-SHA-256 RFC 4231 test case 2 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 4, data, 28);
    FIO_ASSERT(!FIO_MEMCMP(result_512.u8, expected_512, 64),
               "HMAC-SHA-512 RFC 4231 test case 2 failed");

    fio_u512 result_384 = fio_sha384_hmac(key, 4, data, 28);
    FIO_ASSERT(!FIO_MEMCMP(result_384.u8, expected_384, 48),
               "HMAC-SHA-384 RFC 4231 test case 2 failed");
  }

  /* RFC 4231 Test Case 3 - 20 bytes of 0xaa key, 50 bytes of 0xdd data. */
  {
    uint8_t key[20];
    uint8_t data[50];
    FIO_MEMSET(key, 0xaa, sizeof(key));
    FIO_MEMSET(data, 0xdd, sizeof(data));
    static const uint8_t expected_256[32] = {
        0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46, 0x85, 0x4d, 0xb8,
        0xeb, 0xd0, 0x91, 0x81, 0xa7, 0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8,
        0xc1, 0x22, 0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe};
    static const uint8_t expected_512[64] = {
        0xfa, 0x73, 0xb0, 0x08, 0x9d, 0x56, 0xa2, 0x84, 0xef, 0xb0, 0xf0,
        0x75, 0x6c, 0x89, 0x0b, 0xe9, 0xb1, 0xb5, 0xdb, 0xdd, 0x8e, 0xe8,
        0x1a, 0x36, 0x55, 0xf8, 0x3e, 0x33, 0xb2, 0x27, 0x9d, 0x39, 0xbf,
        0x3e, 0x84, 0x82, 0x79, 0xa7, 0x22, 0xc8, 0x06, 0xb4, 0x85, 0xa4,
        0x7e, 0x67, 0xc8, 0x07, 0xb9, 0x46, 0xa3, 0x37, 0xbe, 0xe8, 0x94,
        0x26, 0x74, 0x27, 0x88, 0x59, 0xe1, 0x32, 0x92, 0xfb};

    fio_u256 result_256 = fio_sha256_hmac(key, 20, data, 50);
    FIO_ASSERT(!FIO_MEMCMP(result_256.u8, expected_256, 32),
               "HMAC-SHA-256 RFC 4231 test case 3 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 20, data, 50);
    FIO_ASSERT(!FIO_MEMCMP(result_512.u8, expected_512, 64),
               "HMAC-SHA-512 RFC 4231 test case 3 failed");
  }

  /* RFC 4231 Test Case 4 - incrementing key and data. */
  {
    static const uint8_t key[25] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
        0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
        0x17, 0x18, 0x19};
    uint8_t data[50];
    FIO_MEMSET(data, 0xcd, sizeof(data));
    static const uint8_t expected_256[32] = {
        0x82, 0x55, 0x8a, 0x38, 0x9a, 0x44, 0x3c, 0x0e, 0xa4, 0xcc, 0x81,
        0x98, 0x99, 0xf2, 0x08, 0x3a, 0x85, 0xf0, 0xfa, 0xa3, 0xe5, 0x78,
        0xf8, 0x07, 0x7a, 0x2e, 0x3f, 0xf4, 0x67, 0x29, 0x66, 0x5b};
    static const uint8_t expected_512[64] = {
        0xb0, 0xba, 0x46, 0x56, 0x37, 0x45, 0x8c, 0x69, 0x90, 0xe5, 0xa8,
        0xc5, 0xf6, 0x1d, 0x4a, 0xf7, 0xe5, 0x76, 0xd9, 0x7f, 0xf9, 0x4b,
        0x87, 0x2d, 0xe7, 0x6f, 0x80, 0x50, 0x36, 0x1e, 0xe3, 0xdb, 0xa9,
        0x1c, 0xa5, 0xc1, 0x1a, 0xa2, 0x5e, 0xb4, 0xd6, 0x79, 0x27, 0x5c,
        0xc5, 0x78, 0x80, 0x63, 0xa5, 0xf1, 0x97, 0x41, 0x12, 0x0c, 0x4f,
        0x2d, 0xe2, 0xad, 0xeb, 0xeb, 0x10, 0xa2, 0x98, 0xdd};

    fio_u256 result_256 = fio_sha256_hmac(key, 25, data, 50);
    FIO_ASSERT(!FIO_MEMCMP(result_256.u8, expected_256, 32),
               "HMAC-SHA-256 RFC 4231 test case 4 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 25, data, 50);
    FIO_ASSERT(!FIO_MEMCMP(result_512.u8, expected_512, 64),
               "HMAC-SHA-512 RFC 4231 test case 4 failed");
  }

  /* RFC 4231 Test Case 6 - 131 byte key (larger than block size). */
  {
    uint8_t key[131];
    FIO_MEMSET(key, 0xaa, sizeof(key));
    static const char data[] = "Test Using Larger Than Block-Size Key - Hash Key First";
    static const uint8_t expected_256[32] = {
        0x60, 0xe4, 0x31, 0x59, 0x1e, 0xe0, 0xb6, 0x7f, 0x0d, 0x8a, 0x26,
        0xaa, 0xcb, 0xf5, 0xb7, 0x7f, 0x8e, 0x0b, 0xc6, 0x21, 0x37, 0x28,
        0xc5, 0x14, 0x05, 0x46, 0x04, 0x0f, 0x0e, 0xe3, 0x7f, 0x54};
    static const uint8_t expected_512[64] = {
        0x80, 0xb2, 0x42, 0x63, 0xc7, 0xc1, 0xa3, 0xeb, 0xb7, 0x14, 0x93,
        0xc1, 0xdd, 0x7b, 0xe8, 0xb4, 0x9b, 0x46, 0xd1, 0xf4, 0x1b, 0x4a,
        0xee, 0xc1, 0x12, 0x1b, 0x01, 0x37, 0x83, 0xf8, 0xf3, 0x52, 0x6b,
        0x56, 0xd0, 0x37, 0xe0, 0x5f, 0x25, 0x98, 0xbd, 0x0f, 0xd2, 0x21,
        0x5d, 0x6a, 0x1e, 0x52, 0x95, 0xe6, 0x4f, 0x73, 0xf6, 0x3f, 0x0a,
        0xec, 0x8b, 0x91, 0x5a, 0x98, 0x5d, 0x78, 0x65, 0x98};
    static const uint8_t expected_384[48] = {
        0x4e, 0xce, 0x08, 0x44, 0x85, 0x81, 0x3e, 0x90, 0x88, 0xd2, 0xc6,
        0x3a, 0x04, 0x1b, 0xc5, 0xb4, 0x4f, 0x9e, 0xf1, 0x01, 0x2a, 0x2b,
        0x58, 0x8f, 0x3c, 0xd1, 0x1f, 0x05, 0x03, 0x3a, 0xc4, 0xc6, 0x0c,
        0x2e, 0xf6, 0xab, 0x40, 0x30, 0xfe, 0x82, 0x96, 0x24, 0x8d, 0xf1,
        0x63, 0xf4, 0x49, 0x52};

    fio_u256 result_256 = fio_sha256_hmac(key, 131, data, 54);
    FIO_ASSERT(!FIO_MEMCMP(result_256.u8, expected_256, 32),
               "HMAC-SHA-256 RFC 4231 test case 6 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 131, data, 54);
    FIO_ASSERT(!FIO_MEMCMP(result_512.u8, expected_512, 64),
               "HMAC-SHA-512 RFC 4231 test case 6 failed");

    fio_u512 result_384 = fio_sha384_hmac(key, 131, data, 54);
    FIO_ASSERT(!FIO_MEMCMP(result_384.u8, expected_384, 48),
               "HMAC-SHA-384 RFC 4231 test case 6 failed");
  }

  /* RFC 4231 Test Case 7 - 131 byte key, longer data. */
  {
    uint8_t key[131];
    FIO_MEMSET(key, 0xaa, sizeof(key));
    static const char data[] =
        "This is a test using a larger than block-size key "
        "and a larger than block-size data. The key needs to "
        "be hashed before being used by the HMAC algorithm.";
    static const uint8_t expected_256[32] = {
        0x9b, 0x09, 0xff, 0xa7, 0x1b, 0x94, 0x2f, 0xcb, 0x27, 0x63, 0x5f,
        0xbc, 0xd5, 0xb0, 0xe9, 0x44, 0xbf, 0xdc, 0x63, 0x64, 0x4f, 0x07,
        0x13, 0x93, 0x8a, 0x7f, 0x51, 0x53, 0x5c, 0x3a, 0x35, 0xe2};
    static const uint8_t expected_512[64] = {
        0xe3, 0x7b, 0x6a, 0x77, 0x5d, 0xc8, 0x7d, 0xba, 0xa4, 0xdf, 0xa9,
        0xf9, 0x6e, 0x5e, 0x3f, 0xfd, 0xde, 0xbd, 0x71, 0xf8, 0x86, 0x72,
        0x89, 0x86, 0x5d, 0xf5, 0xa3, 0x2d, 0x20, 0xcd, 0xc9, 0x44, 0xb6,
        0x02, 0x2c, 0xac, 0x3c, 0x49, 0x82, 0xb1, 0x0d, 0x5e, 0xeb, 0x55,
        0xc3, 0xe4, 0xde, 0x15, 0x13, 0x46, 0x76, 0xfb, 0x6d, 0xe0, 0x44,
        0x60, 0x65, 0xc9, 0x74, 0x40, 0xfa, 0x8c, 0x6a, 0x58};
    static const uint8_t expected_384[48] = {
        0x66, 0x17, 0x17, 0x8e, 0x94, 0x1f, 0x02, 0x0d, 0x35, 0x1e, 0x2f,
        0x25, 0x4e, 0x8f, 0xd3, 0x2c, 0x60, 0x24, 0x20, 0xfe, 0xb0, 0xb8,
        0xfb, 0x9a, 0xdc, 0xce, 0xbb, 0x82, 0x46, 0x1e, 0x99, 0xc5, 0xa6,
        0x78, 0xcc, 0x31, 0xe7, 0x99, 0x17, 0x6d, 0x38, 0x60, 0xe6, 0x11,
        0x0c, 0x46, 0x52, 0x3e};

    fio_u256 result_256 = fio_sha256_hmac(key, 131, data, 152);
    FIO_ASSERT(!FIO_MEMCMP(result_256.u8, expected_256, 32),
               "HMAC-SHA-256 RFC 4231 test case 7 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 131, data, 152);
    FIO_ASSERT(!FIO_MEMCMP(result_512.u8, expected_512, 64),
               "HMAC-SHA-512 RFC 4231 test case 7 failed");

    fio_u512 result_384 = fio_sha384_hmac(key, 131, data, 152);
    FIO_ASSERT(!FIO_MEMCMP(result_384.u8, expected_384, 48),
               "HMAC-SHA-384 RFC 4231 test case 7 failed");
  }
}

/* *****************************************************************************
OpenSSL Cross-Validation (library only, no external processes)
***************************************************************************** */

#if HAVE_OPENSSL
FIO_SFUNC void FIO_NAME_TEST(stl, sha_openssl_compare)(void) {
  /* SHA-384 / SHA-512 / HMAC length sweep: every remainder mod 128 (twice),
   * one-shot and streaming, plus HMAC (inner hash = 128 byte key block +
   * message, so every inner remainder is hit as well). */
  {
    uint8_t buf[300];
    uint8_t key[48];
    fio_rand_bytes(buf, sizeof(buf));
    fio_rand_bytes(key, sizeof(key));
    for (size_t len = 0; len <= sizeof(buf); ++len) {
      uint8_t ossl[64];
      unsigned int ossl_len = 0;
      const size_t split = len >> 1;

      SHA512(buf, len, ossl);
      fio_u512 h = fio_sha512(buf, len);
      FIO_ASSERT(!FIO_MEMCMP(h.u8, ossl, 64),
                 "SHA-512 mismatch with OpenSSL at len=%zu",
                 len);
      fio_sha512_s ctx = fio_sha512_init();
      fio_sha512_consume(&ctx, buf, split);
      fio_sha512_consume(&ctx, buf + split, len - split);
      h = fio_sha512_finalize(&ctx);
      FIO_ASSERT(!FIO_MEMCMP(h.u8, ossl, 64),
                 "SHA-512 streaming mismatch with OpenSSL at len=%zu",
                 len);

      SHA384(buf, len, ossl);
      h = fio_sha384(buf, len);
      FIO_ASSERT(!FIO_MEMCMP(h.u8, ossl, 48),
                 "SHA-384 mismatch with OpenSSL at len=%zu",
                 len);
      ctx = fio_sha384_init();
      fio_sha384_consume(&ctx, buf, split);
      fio_sha384_consume(&ctx, buf + split, len - split);
      h = fio_sha384_finalize(&ctx);
      FIO_ASSERT(!FIO_MEMCMP(h.u8, ossl, 48),
                 "SHA-384 streaming mismatch with OpenSSL at len=%zu",
                 len);

      HMAC(EVP_sha384(), key, sizeof(key), buf, len, ossl, &ossl_len);
      h = fio_sha384_hmac(key, sizeof(key), buf, len);
      FIO_ASSERT(ossl_len == 48 && !FIO_MEMCMP(h.u8, ossl, 48),
                 "HMAC-SHA-384 mismatch with OpenSSL at len=%zu",
                 len);
      HMAC(EVP_sha512(), key, sizeof(key), buf, len, ossl, &ossl_len);
      h = fio_sha512_hmac(key, sizeof(key), buf, len);
      FIO_ASSERT(ossl_len == 64 && !FIO_MEMCMP(h.u8, ossl, 64),
                 "HMAC-SHA-512 mismatch with OpenSSL at len=%zu",
                 len);
    }
  }

  /* SHA-256 block-boundary sweep. */
  for (size_t test_len = 1; test_len <= 200; test_len++) {
    uint8_t *test_data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, test_len, 0);
    FIO_ASSERT(test_data, "SHA-256 OpenSSL compare allocation failed");
    FIO_MEMSET(test_data, 0x41, test_len);

    fio_u256 fio_result = fio_sha256(test_data, test_len);
    uint8_t openssl_result[32];
    SHA256(test_data, test_len, openssl_result);

    FIO_ASSERT(!FIO_MEMCMP(fio_result.u8, openssl_result, 32),
               "SHA-256 mismatch with OpenSSL at len=%zu",
               test_len);
    FIO_MEM_FREE(test_data, test_len);
  }

  /* SHA-512 / SHA-256 known string comparison. */
  struct {
    const char *str;
  } data[] = {
      {.str = "abc"},
      {.str = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"},
      {.str = "The quick brown fox jumps over the lazy dog"},
      {.str = "The quick brown fox jumps over the lazy cog"},
      {.str = ""},
  };
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    size_t len = FIO_STRLEN(data[i].str);
    fio_u256 result256 = fio_sha256(data[i].str, len);
    fio_u512 result512 = fio_sha512(data[i].str, len);

    uint8_t openssl_256[32];
    uint8_t openssl_512[64];
    SHA256((const unsigned char *)data[i].str, len, openssl_256);
    SHA512((const unsigned char *)data[i].str, len, openssl_512);

    FIO_ASSERT(!FIO_MEMCMP(result256.u8, openssl_256, 32),
               "SHA-256 OpenSSL mismatch for \"%s\"",
               data[i].str);
    FIO_ASSERT(!FIO_MEMCMP(result512.u8, openssl_512, 64),
               "SHA-512 OpenSSL mismatch for \"%s\"",
               data[i].str);

    /* Streaming SHA-512 matches one-shot OpenSSL for longer inputs. */
    if (len > 8) {
      fio_sha512_s ctx = fio_sha512_init();
      fio_sha512_consume(&ctx, data[i].str, 4);
      fio_sha512_consume(&ctx, data[i].str + 4, 4);
      fio_sha512_consume(&ctx, data[i].str + 8, len - 8);
      fio_u512 streamed = fio_sha512_finalize(&ctx);
      FIO_ASSERT(!FIO_MEMCMP(streamed.u8, openssl_512, 64),
                 "SHA-512 streamed OpenSSL mismatch for \"%s\"",
                 data[i].str);
    }
  }

  /* HMAC-SHA-256 / HMAC-SHA-512 random cross-check. */
  {
    uint8_t key[64];
    uint8_t msg[256];
    fio_rand_bytes(key, sizeof(key));
    fio_rand_bytes(msg, sizeof(msg));

    fio_u256 fio_256 = fio_sha256_hmac(key, sizeof(key), msg, sizeof(msg));
    fio_u512 fio_512 = fio_sha512_hmac(key, sizeof(key), msg, sizeof(msg));

    uint8_t openssl_256[32];
    uint8_t openssl_512[64];
    unsigned int len;
    HMAC(EVP_sha256(),
         key,
         sizeof(key),
         msg,
         sizeof(msg),
         openssl_256,
         &len);
    HMAC(EVP_sha512(),
         key,
         sizeof(key),
         msg,
         sizeof(msg),
         openssl_512,
         &len);

    FIO_ASSERT(!FIO_MEMCMP(fio_256.u8, openssl_256, 32),
               "HMAC-SHA-256 mismatch with OpenSSL");
    FIO_ASSERT(!FIO_MEMCMP(fio_512.u8, openssl_512, 64),
               "HMAC-SHA-512 mismatch with OpenSSL");
  }
}
#endif /* HAVE_OPENSSL */

/* *****************************************************************************
SHA Edge Cases
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, sha_edge_cases)(void) {
  /* Empty input. */
  {
    fio_sha1_s sha1 = fio_sha1(NULL, 0);
    fio_u256 sha256 = fio_sha256(NULL, 0);
    fio_u512 sha512 = fio_sha512(NULL, 0);

    uint8_t zero20[20] = {0};
    uint8_t zero32[32] = {0};
    uint8_t zero64[64] = {0};
    FIO_ASSERT(FIO_MEMCMP(sha1.digest, zero20, 20) != 0,
               "SHA-1 empty input produced all zeros");
    FIO_ASSERT(FIO_MEMCMP(sha256.u8, zero32, 32) != 0,
               "SHA-256 empty input produced all zeros");
    FIO_ASSERT(FIO_MEMCMP(sha512.u8, zero64, 64) != 0,
               "SHA-512 empty input produced all zeros");
  }

  /* Single byte and block-boundary sizes. */
  {
    size_t test_sizes[] = {1, 63, 64, 65, 127, 128, 129};
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); ++i) {
      size_t len = test_sizes[i];
      uint8_t *data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, len, 0);
      FIO_ASSERT(data, "SHA edge-case allocation failed");
      FIO_MEMSET(data, 'A', len);

      fio_u256 sha256 = fio_sha256(data, len);
      fio_u512 sha512 = fio_sha512(data, len);

      uint8_t zero32[32] = {0};
      uint8_t zero64[64] = {0};
      FIO_ASSERT(FIO_MEMCMP(sha256.u8, zero32, 32) != 0,
                 "SHA-256 at %zu bytes produced all zeros",
                 len);
      FIO_ASSERT(FIO_MEMCMP(sha512.u8, zero64, 64) != 0,
                 "SHA-512 at %zu bytes produced all zeros",
                 len);

      FIO_MEM_FREE(data, len);
    }
  }

  /* Determinism and avalanche. */
  {
    uint8_t data1[32] = {0};
    uint8_t data2[32] = {0};
    data2[0] = 1;

    fio_u256 h1 = fio_sha256(data1, 32);
    fio_u256 h2 = fio_sha256(data2, 32);
    FIO_ASSERT(FIO_MEMCMP(h1.u8, h2.u8, 32) != 0,
               "SHA-256 failed avalanche sensitivity test");

    fio_u256 h1_2 = fio_sha256(data1, 32);
    FIO_ASSERT(!FIO_MEMCMP(h1.u8, h1_2.u8, 32),
               "SHA-256 deterministic mismatch");
  }

  /* Synthetic huge lengths (state forged, zero data bytes): the length field
   * must use the full 64 bit counter (SHA-256, 32 bit `size_t` truncation)
   * and the high 64 bits of SHA-512's 128 bit length (>= 2^61 bytes).
   * Expected values from an independent Python compression model. */
  {
    uint8_t expected[64];
    fio_sha256_s s256 = fio_sha256_init();
    s256.total_len = ((uint64_t)1ULL << 32) + 5;
    fio_u256 r256 = fio_sha256_finalize(&s256);
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected,
     "438c1ec60ee4f7f050dc693f3301107f3a16b1fa3c59a9f9315111e343ade545");
    FIO_ASSERT(!FIO_MEMCMP(r256.u8, expected, 32),
               "SHA-256 synthetic 2^32+5 byte length mismatch");

    const char *hex512[2] = {
        "4e15588b073f68f07592e9a51190a7e00e2efaaa365fb318f27e82e5c6df94a8"
        "62d54b825cefcc3bd28977fcefc7fd8cac281f334977797ea395dbff21311a72",
        "1e389e2b9e9ebb5694436173cf1bc43eff2a046d8f36e2db573458a857633708"
        "1c5ca192ec821ad21e3ec4a6a8b8112fc6774ec05c5ad2fad7b20d4c3684f53a"};
    for (size_t i = 0; i < 2; ++i) {
      fio_sha512_s s512 = fio_sha512_init();
      s512.total_len = ((uint64_t)1ULL << 61) + (i * 5);
      fio_u512 r512 = fio_sha512_finalize(&s512);
      FIO_NAME_TEST(stl, sha1_hex2bin)(expected, hex512[i]);
      FIO_ASSERT(!FIO_MEMCMP(r512.u8, expected, 64),
                 "SHA-512 synthetic 2^61+%zu byte length mismatch",
                 i * 5);
    }
  }

  /* Empty HMAC key: NULL and non-NULL zero length keys are equivalent. */
  {
    uint8_t expected[64];
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected,
     "fd7adb152c05ef80dccf50a1fa4c05d5a3ec6da95575fc312ae7c5d091836351");
    fio_u256 r256 = fio_sha256_hmac(NULL, 0, "abc", 3);
    FIO_ASSERT(!FIO_MEMCMP(r256.u8, expected, 32),
               "HMAC-SHA-256 empty (NULL) key mismatch");
    r256 = fio_sha256_hmac("", 0, "abc", 3);
    FIO_ASSERT(!FIO_MEMCMP(r256.u8, expected, 32),
               "HMAC-SHA-256 empty key mismatch");
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected,
     "29689f6b79a8dd686068c2eeae97fd8769ad3ba65cb5381f838358a8045a358e"
     "e3ba1739c689c7805e31734fb6072f87261d1256995370d55725cba00d10bdd0");
    fio_u512 r512 = fio_sha512_hmac(NULL, 0, "abc", 3);
    FIO_ASSERT(!FIO_MEMCMP(r512.u8, expected, 64),
               "HMAC-SHA-512 empty (NULL) key mismatch");
    FIO_NAME_TEST(stl, sha1_hex2bin)
    (expected,
     "948f7c5caa500c31d7d4a0f52f3e3da7e33c8a9fe6ef528b"
     "8a9ac3e4adc4e24d908e6f40b737510e82354759dc5e9f06");
    r512 = fio_sha384_hmac(NULL, 0, "abc", 3);
    FIO_ASSERT(!FIO_MEMCMP(r512.u8, expected, 48),
               "HMAC-SHA-384 empty (NULL) key mismatch");
  }
}

int main(void) {
  FIO_NAME_TEST(stl, sha1_kat)();
  FIO_NAME_TEST(stl, sha1_hmac_kat)();
  FIO_NAME_TEST(stl, sha2_kat)();
  FIO_NAME_TEST(stl, sha384_kat)();
  FIO_NAME_TEST(stl, sha2_padding_boundary)();
  FIO_NAME_TEST(stl, sha2_streaming)();
  FIO_NAME_TEST(stl, hmac_rfc4231)();
#if HAVE_OPENSSL
  FIO_NAME_TEST(stl, sha_openssl_compare)();
#endif
  FIO_NAME_TEST(stl, sha_edge_cases)();
  return 0;
}
