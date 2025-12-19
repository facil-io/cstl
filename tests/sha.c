/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPT
#include FIO_INCLUDE_FILE

/* *****************************************************************************
SHA1
***************************************************************************** */

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha1_wrapper)(char *data, size_t len) {
  fio_sha1_s h = fio_sha1((const void *)data, (uint64_t)len);
  return *(uintptr_t *)h.digest;
}

#if HAVE_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/ssl.h>

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha1_open_ssl_wrapper)(char *data,
                                                                size_t len) {
  fio_u256 result;
  SHA1((const unsigned char *)data, len, result.u8);
  return result.u64[0];
}

#endif

FIO_SFUNC void FIO_NAME_TEST(stl, sha1)(void) {
  FIO_LOG_DDEBUG("Testing SHA-1");
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
    fio_sha1_s sha1 = fio_sha1(data[i].str, FIO_STRLEN(data[i].str));

    FIO_ASSERT(!memcmp(sha1.digest, data[i].sha1, fio_sha1_len()),
               "SHA1 mismatch for \"%s\"",
               data[i].str);
  }
  /* Performance tests moved to tests/performance-crypto.c */
}

/* *****************************************************************************
SHA2
***************************************************************************** */

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha256_wrapper)(char *data,
                                                         size_t len) {
  fio_u256 h = fio_sha256((const void *)data, (uint64_t)len);
  return (uintptr_t)(h.u64[0]);
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha512_wrapper)(char *data,
                                                         size_t len) {
  fio_u512 h = fio_sha512((const void *)data, (uint64_t)len);
  return (uintptr_t)(h.u64[0]);
}

#if HAVE_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha256_open_ssl_wrapper)(char *data,
                                                                  size_t len) {
  fio_u256 result;
  SHA256((const unsigned char *)data, len, result.u8);
  return result.u64[0];
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha512_open_ssl_wrapper)(char *data,
                                                                  size_t len) {
  fio_u512 result;
  SHA512((const unsigned char *)data, len, result.u8);
  return result.u64[0];
}
#endif /* HAVE_OPENSSL */

FIO_SFUNC void FIO_NAME_TEST(stl, sha2)(void) {
  FIO_LOG_DDEBUG("Testing SHA-2");

  /* Test SHA-256 for various input sizes around block boundaries */
#if HAVE_OPENSSL
  {
    for (size_t test_len = 1; test_len <= 200; test_len++) {
      uint8_t *test_data = malloc(test_len);
      memset(test_data, 0x41, test_len);
      fio_u256 fio_result = fio_sha256(test_data, test_len);
      uint8_t openssl_result[32];
      SHA256(test_data, test_len, openssl_result);
      FIO_ASSERT(!memcmp(fio_result.u8, openssl_result, 32),
                 "SHA-256 mismatch at len=%zu",
                 test_len);
      free(test_data);
    }
  }
#endif

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
          .sha512 =
              (char *)"\xDD\xAF\x35\xA1\x93\x61\x7A\xBA\xCC\x41\x73\x49\xAE"
                      "\x20\x41\x31\x12\xE6\xFA\x4E\x89\xA9\x7E\xA2\x0A\x9E"
                      "\xEE\xE6\x4B\x55\xD3\x9A\x21\x92\x99\x2A\x27\x4F\xC1"
                      "\xA8\x36\xBA\x3C\x23\xA3\xFE\xEB\xBD\x45\x4D\x44\x23"
                      "\x64\x3C\xE8\x0E\x2A\x9A\xC9\x4F\xA5\x4C\xA4\x9F",
      },
      {
          .str = (char *)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnl"
                         "mnomnopnopq",
          .sha256 = (char *)"\x24\x8D\x6A\x61\xD2\x06\x38\xB8\xE5\xC0\x26"
                            "\x93\x0C\x3E\x60\x39\xA3\x3C\xE4\x59\x64\xFF"
                            "\x21\x67\xF6\xEC\xED\xD4\x19\xDB\x06\xC1",
          .sha512 =
              (char *)"\x20\x4A\x8F\xC6\xDD\xA8\x2F\x0A\x0C\xED\x7B\xEB\x8E\x08"
                      "\xA4\x16\x57\xC1\x6E\xF4\x68\xB2\x28\xA8\x27\x9B\xE3\x31"
                      "\xA7\x03\xC3\x35\x96\xFD\x15\xC1\x3B\x1B\x07\xF9\xAA\x1D"
                      "\x3B\xEA\x57\x78\x9C\xA0\x31\xAD\x85\xC7\xA7\x1D\xD7\x03"
                      "\x54\xEC\x63\x12\x38\xCA\x34\x45",
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
      {
          .str = (char *)"There was an Old Man on some rocks, Who shut his "
                         "wife up in a box; When she said, \"Let me out,\" He "
                         "exclaimed, \"Without doubt, You will pass all your "
                         "life in that box.\"",
      },
      {
          .str = (char *)"Once upon a time, there was a little secret garden "
                         "locked away behind a little mouse hole in Agatha's "
                         "kitchen. The mouse hole was tucked away behind the "
                         "fridge.",
      },
      {
          .str = (char *)"It was a loud fridge, that would klunk happily "
                         "through the night, humming softly to mask the sound "
                         "of chippering birds from t...",
      },
  };
#if HAVE_OPENSSL
  FIO_LOG_DEBUG2("Testing against OpenSSL SHA512 and SHA256");
#endif
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    if (!data[i].str)
      continue;
#if HAVE_OPENSSL
    {
      fio_u512 openssl_result;
      fio_u512 sha512 = fio_sha512(data[i].str, FIO_STRLEN(data[i].str));
      fio_u256 sha256 = fio_sha256(data[i].str, FIO_STRLEN(data[i].str));
      SHA256((const unsigned char *)data[i].str,
             FIO_STRLEN(data[i].str),
             openssl_result.u8);
      FIO_ASSERT(fio_u256_is_eq(openssl_result.u256, &sha256),
                 "SHA256 OpenSSL mismatch for \"%s\":\n\t %X%X%X%X...%X%X%X%X",
                 data[i].str,
                 sha256.u8[0],
                 sha256.u8[1],
                 sha256.u8[2],
                 sha256.u8[3],
                 sha256.u8[28],
                 sha256.u8[29],
                 sha256.u8[30],
                 sha256.u8[31]);

      SHA512((const unsigned char *)data[i].str,
             FIO_STRLEN(data[i].str),
             openssl_result.u8);
      FIO_ASSERT(fio_u512_is_eq(&openssl_result, &sha512),
                 "SHA512 OpenSSL mismatch for \"%s\":\n\t "
                 "%X%X%X%X%X%X%X%X...%X%X%X%X%X%X%X%X",
                 data[i].str,
                 sha512.u8[0],
                 sha512.u8[1],
                 sha512.u8[2],
                 sha512.u8[3],
                 sha512.u8[4],
                 sha512.u8[5],
                 sha512.u8[6],
                 sha512.u8[7],
                 sha512.u8[24],
                 sha512.u8[25],
                 sha512.u8[26],
                 sha512.u8[27],
                 sha512.u8[28],
                 sha512.u8[29],
                 sha512.u8[30],
                 sha512.u8[31]);

      if (FIO_STRLEN(data[i].str) > 128) {
        fio_sha512_s tmp = fio_sha512_init();
        fio_sha512_consume(&tmp, data[i].str, 128);
        fio_sha512_consume(&tmp,
                           data[i].str + 128,
                           FIO_STRLEN(data[i].str) - 128);
        sha512 = fio_sha512_finalize(&tmp);
        FIO_ASSERT(fio_u512_is_eq(&openssl_result, &sha512),
                   "SHA512 OpenSSL mismatch for streamed \"%s\":\n\t "
                   "%X%X%X%X%X%X%X%X...%X%X%X%X%X%X%X%X",
                   data[i].str,
                   sha512.u8[0],
                   sha512.u8[1],
                   sha512.u8[2],
                   sha512.u8[3],
                   sha512.u8[4],
                   sha512.u8[5],
                   sha512.u8[6],
                   sha512.u8[7],
                   sha512.u8[24],
                   sha512.u8[25],
                   sha512.u8[26],
                   sha512.u8[27],
                   sha512.u8[28],
                   sha512.u8[29],
                   sha512.u8[30],
                   sha512.u8[31]);
      }
    }
#endif

    if (data[i].sha256) {
      fio_u256 sha256 = fio_sha256(data[i].str, FIO_STRLEN(data[i].str));
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
      if (FIO_STRLEN(data[i].str) > 8) {
        fio_sha256_s sha = fio_sha256_init();
        fio_sha256_consume(&sha, data[i].str, 4);
        FIO_COMPILER_GUARD;
        fio_sha256_consume(&sha, data[i].str + 4, 4);
        FIO_COMPILER_GUARD;
        fio_sha256_consume(&sha, data[i].str + 8, FIO_STRLEN(data[i].str) - 8);
        sha256 = fio_sha256_finalize(&sha);
        FIO_ASSERT(
            !memcmp(sha256.u8, data[i].sha256, 32),
            "SHA256 mismatch for \"%s\" (in parts):\n\t %X%X%X%X...%X%X%X%X",
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
    }
    if (data[i].sha512) {
      fio_u512 sha512 = fio_sha512(data[i].str, FIO_STRLEN(data[i].str));
      FIO_ASSERT(
          !memcmp(sha512.u8, data[i].sha512, 64),
          "SHA512 mismatch for \"%s\":\n\t %X%X%X%X%X%X%X%X...%X%X%X%X%X%X%X%X",
          data[i].str,
          sha512.u8[0],
          sha512.u8[1],
          sha512.u8[2],
          sha512.u8[3],
          sha512.u8[4],
          sha512.u8[5],
          sha512.u8[6],
          sha512.u8[7],
          sha512.u8[24],
          sha512.u8[25],
          sha512.u8[26],
          sha512.u8[27],
          sha512.u8[28],
          sha512.u8[29],
          sha512.u8[30],
          sha512.u8[31]);
      if (FIO_STRLEN(data[i].str) > 8) {
        fio_sha512_s sha = fio_sha512_init();
        fio_sha512_consume(&sha, data[i].str, 4);
        FIO_COMPILER_GUARD;
        fio_sha512_consume(&sha, data[i].str + 4, 4);
        FIO_COMPILER_GUARD;
        fio_sha512_consume(&sha, data[i].str + 8, FIO_STRLEN(data[i].str) - 8);
        sha512 = fio_sha512_finalize(&sha);
        FIO_ASSERT(!memcmp(sha512.u8, data[i].sha512, 64),
                   "SHA512 mismatch for \"%s\" (in parts):\n\t "
                   "%X%X%X%X%X%X%X%X...%X%X%X%X%X%X%X%X",
                   data[i].str,
                   sha512.u8[0],
                   sha512.u8[1],
                   sha512.u8[2],
                   sha512.u8[3],
                   sha512.u8[4],
                   sha512.u8[5],
                   sha512.u8[6],
                   sha512.u8[7],
                   sha512.u8[24],
                   sha512.u8[25],
                   sha512.u8[26],
                   sha512.u8[27],
                   sha512.u8[28],
                   sha512.u8[29],
                   sha512.u8[30],
                   sha512.u8[31]);
      }
    }
  }

  {
    FIO_LOG_DDEBUG("Testing SHA-2 based secret.");
    fio_u512 s0 = {0};
    fio_u512 s1 = fio_secret();
    FIO_ASSERT(!fio_u512_is_eq(&s0, &s1), "Secret is zero?!");
    s0 = fio_secret();
    FIO_ASSERT(fio_u512_is_eq(&s0, &s1), "Secret should be consistent");
    FIO_ASSERT(!fio_u512_is_eq(&s0, &fio___secret), "Secret should be masked");
  }
  /* Performance tests moved to tests/performance-crypto.c */
}

/* *****************************************************************************
HMAC-SHA256 / HMAC-SHA512 Tests (RFC 4231 Test Vectors)
https://www.rfc-editor.org/rfc/rfc4231
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, hmac)(void) {
  FIO_LOG_DDEBUG("Testing HMAC-SHA256 / HMAC-SHA512 (RFC 4231)");

  /* RFC 4231 Test Case 1 */
  {
    /* clang-format off */
    static const uint8_t key[20] = {
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b
    };
    static const char *data = "Hi There";
    static const uint8_t expected_256[32] = {
        0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53,
        0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
        0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7,
        0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7
    };
    static const uint8_t expected_512[64] = {
        0x87, 0xaa, 0x7c, 0xde, 0xa5, 0xef, 0x61, 0x9d,
        0x4f, 0xf0, 0xb4, 0x24, 0x1a, 0x1d, 0x6c, 0xb0,
        0x23, 0x79, 0xf4, 0xe2, 0xce, 0x4e, 0xc2, 0x78,
        0x7a, 0xd0, 0xb3, 0x05, 0x45, 0xe1, 0x7c, 0xde,
        0xda, 0xa8, 0x33, 0xb7, 0xd6, 0xb8, 0xa7, 0x02,
        0x03, 0x8b, 0x27, 0x4e, 0xae, 0xa3, 0xf4, 0xe4,
        0xbe, 0x9d, 0x91, 0x4e, 0xeb, 0x61, 0xf1, 0x70,
        0x2e, 0x69, 0x6c, 0x20, 0x3a, 0x12, 0x68, 0x54
    };
    /* clang-format on */

    fio_u256 result_256 = fio_sha256_hmac(key, 20, data, 8);
    FIO_ASSERT(!memcmp(result_256.u8, expected_256, 32),
               "HMAC-SHA256 test case 1 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 20, data, 8);
    FIO_ASSERT(!memcmp(result_512.u8, expected_512, 64),
               "HMAC-SHA512 test case 1 failed");
  }

  /* RFC 4231 Test Case 2 - "Jefe" key */
  {
    /* clang-format off */
    static const char *key = "Jefe";
    static const char *data = "what do ya want for nothing?";
    static const uint8_t expected_256[32] = {
        0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
        0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
        0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
        0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43
    };
    static const uint8_t expected_512[64] = {
        0x16, 0x4b, 0x7a, 0x7b, 0xfc, 0xf8, 0x19, 0xe2,
        0xe3, 0x95, 0xfb, 0xe7, 0x3b, 0x56, 0xe0, 0xa3,
        0x87, 0xbd, 0x64, 0x22, 0x2e, 0x83, 0x1f, 0xd6,
        0x10, 0x27, 0x0c, 0xd7, 0xea, 0x25, 0x05, 0x54,
        0x97, 0x58, 0xbf, 0x75, 0xc0, 0x5a, 0x99, 0x4a,
        0x6d, 0x03, 0x4f, 0x65, 0xf8, 0xf0, 0xe6, 0xfd,
        0xca, 0xea, 0xb1, 0xa3, 0x4d, 0x4a, 0x6b, 0x4b,
        0x63, 0x6e, 0x07, 0x0a, 0x38, 0xbc, 0xe7, 0x37
    };
    /* clang-format on */

    fio_u256 result_256 = fio_sha256_hmac(key, 4, data, 28);
    FIO_ASSERT(!memcmp(result_256.u8, expected_256, 32),
               "HMAC-SHA256 test case 2 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 4, data, 28);
    FIO_ASSERT(!memcmp(result_512.u8, expected_512, 64),
               "HMAC-SHA512 test case 2 failed");
  }

  /* RFC 4231 Test Case 3 - 20 bytes of 0xaa key, 50 bytes of 0xdd data */
  {
    /* clang-format off */
    uint8_t key[20];
    uint8_t data[50];
    memset(key, 0xaa, 20);
    memset(data, 0xdd, 50);
    static const uint8_t expected_256[32] = {
        0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46,
        0x85, 0x4d, 0xb8, 0xeb, 0xd0, 0x91, 0x81, 0xa7,
        0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8, 0xc1, 0x22,
        0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe
    };
    
    static const uint8_t expected_512[64] = {
        0xfa, 0x73, 0xb0, 0x08, 0x9d, 0x56, 0xa2, 0x84,
        0xef, 0xb0, 0xf0, 0x75, 0x6c, 0x89, 0x0b, 0xe9,
        0xb1, 0xb5, 0xdb, 0xdd, 0x8e, 0xe8, 0x1a, 0x36,
        0x55, 0xf8, 0x3e, 0x33, 0xb2, 0x27, 0x9d, 0x39,
        0xbf, 0x3e, 0x84, 0x82, 0x79, 0xa7, 0x22, 0xc8,
        0x06, 0xb4, 0x85, 0xa4, 0x7e, 0x67, 0xc8, 0x07,
        0xb9, 0x46, 0xa3, 0x37, 0xbe, 0xe8, 0x94, 0x26,
        0x74, 0x27, 0x88, 0x59, 0xe1, 0x32, 0x92, 0xfb
    };
    /* clang-format on */

    fio_u256 result_256 = fio_sha256_hmac(key, 20, data, 50);
    FIO_ASSERT(!memcmp(result_256.u8, expected_256, 32),
               "HMAC-SHA256 test case 3 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 20, data, 50);
    FIO_ASSERT(!memcmp(result_512.u8, expected_512, 64),
               "HMAC-SHA512 test case 3 failed");
  }

  /* RFC 4231 Test Case 4 - incrementing key and data */
  {
    /* clang-format off */
    static const uint8_t key[25] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19
    };
    uint8_t data[50];
    memset(data, 0xcd, 50);
    static const uint8_t expected_256[32] = {
        0x82, 0x55, 0x8a, 0x38, 0x9a, 0x44, 0x3c, 0x0e,
        0xa4, 0xcc, 0x81, 0x98, 0x99, 0xf2, 0x08, 0x3a,
        0x85, 0xf0, 0xfa, 0xa3, 0xe5, 0x78, 0xf8, 0x07,
        0x7a, 0x2e, 0x3f, 0xf4, 0x67, 0x29, 0x66, 0x5b
    };
    static const uint8_t expected_512[64] = {
        0xb0, 0xba, 0x46, 0x56, 0x37, 0x45, 0x8c, 0x69,
        0x90, 0xe5, 0xa8, 0xc5, 0xf6, 0x1d, 0x4a, 0xf7,
        0xe5, 0x76, 0xd9, 0x7f, 0xf9, 0x4b, 0x87, 0x2d,
        0xe7, 0x6f, 0x80, 0x50, 0x36, 0x1e, 0xe3, 0xdb,
        0xa9, 0x1c, 0xa5, 0xc1, 0x1a, 0xa2, 0x5e, 0xb4,
        0xd6, 0x79, 0x27, 0x5c, 0xc5, 0x78, 0x80, 0x63,
        0xa5, 0xf1, 0x97, 0x41, 0x12, 0x0c, 0x4f, 0x2d,
        0xe2, 0xad, 0xeb, 0xeb, 0x10, 0xa2, 0x98, 0xdd
    };
    /* clang-format on */

    fio_u256 result_256 = fio_sha256_hmac(key, 25, data, 50);
    FIO_ASSERT(!memcmp(result_256.u8, expected_256, 32),
               "HMAC-SHA256 test case 4 failed");

    fio_u512 result_512 = fio_sha512_hmac(key, 25, data, 50);
    FIO_ASSERT(!memcmp(result_512.u8, expected_512, 64),
               "HMAC-SHA512 test case 4 failed");
  }

  /* RFC 4231 Test Case 6 - 131 byte key (larger than block size) */
  {
    /* clang-format off */
    uint8_t key[131];
    memset(key, 0xaa, 131);
    static const char *data = "Test Using Larger Than Block-Size Key - Hash Key First";
    static const uint8_t expected_256[32] = {
        0x60, 0xe4, 0x31, 0x59, 0x1e, 0xe0, 0xb6, 0x7f,
        0x0d, 0x8a, 0x26, 0xaa, 0xcb, 0xf5, 0xb7, 0x7f,
        0x8e, 0x0b, 0xc6, 0x21, 0x37, 0x28, 0xc5, 0x14,
        0x05, 0x46, 0x04, 0x0f, 0x0e, 0xe3, 0x7f, 0x54
    };
    static const uint8_t expected_512[64] = {
        0x80, 0xb2, 0x42, 0x63, 0xc7, 0xc1, 0xa3, 0xeb,
        0xb7, 0x14, 0x93, 0xc1, 0xdd, 0x7b, 0xe8, 0xb4,
        0x9b, 0x46, 0xd1, 0xf4, 0x1b, 0x4a, 0xee, 0xc1,
        0x12, 0x1b, 0x01, 0x37, 0x83, 0xf8, 0xf3, 0x52,
        0x6b, 0x56, 0xd0, 0x37, 0xe0, 0x5f, 0x25, 0x98,
        0xbd, 0x0f, 0xd2, 0x21, 0x5d, 0x6a, 0x1e, 0x52,
        0x95, 0xe6, 0x4f, 0x73, 0xf6, 0x3f, 0x0a, 0xec,
        0x8b, 0x91, 0x5a, 0x98, 0x5d, 0x78, 0x65, 0x98
    };
    /* clang-format on */

    fio_u256 result_256 = fio_sha256_hmac(key, 131, data, 54);
    FIO_ASSERT(!memcmp(result_256.u8, expected_256, 32),
               "HMAC-SHA256 test case 6 failed (large key)");

    fio_u512 result_512 = fio_sha512_hmac(key, 131, data, 54);
    FIO_ASSERT(!memcmp(result_512.u8, expected_512, 64),
               "HMAC-SHA512 test case 6 failed (large key)");
  }

  /* RFC 4231 Test Case 7 - 131 byte key, longer data */
  {
    /* clang-format off */
    uint8_t key[131];
    memset(key, 0xaa, 131);
    static const char *data = "This is a test using a larger than block-size key "
                              "and a larger than block-size data. The key needs to "
                              "be hashed before being used by the HMAC algorithm.";
    static const uint8_t expected_256[32] = {
        0x9b, 0x09, 0xff, 0xa7, 0x1b, 0x94, 0x2f, 0xcb,
        0x27, 0x63, 0x5f, 0xbc, 0xd5, 0xb0, 0xe9, 0x44,
        0xbf, 0xdc, 0x63, 0x64, 0x4f, 0x07, 0x13, 0x93,
        0x8a, 0x7f, 0x51, 0x53, 0x5c, 0x3a, 0x35, 0xe2
    };
    static const uint8_t expected_512[64] = {
        0xe3, 0x7b, 0x6a, 0x77, 0x5d, 0xc8, 0x7d, 0xba,
        0xa4, 0xdf, 0xa9, 0xf9, 0x6e, 0x5e, 0x3f, 0xfd,
        0xde, 0xbd, 0x71, 0xf8, 0x86, 0x72, 0x89, 0x86,
        0x5d, 0xf5, 0xa3, 0x2d, 0x20, 0xcd, 0xc9, 0x44,
        0xb6, 0x02, 0x2c, 0xac, 0x3c, 0x49, 0x82, 0xb1,
        0x0d, 0x5e, 0xeb, 0x55, 0xc3, 0xe4, 0xde, 0x15,
        0x13, 0x46, 0x76, 0xfb, 0x6d, 0xe0, 0x44, 0x60,
        0x65, 0xc9, 0x74, 0x40, 0xfa, 0x8c, 0x6a, 0x58
    };
    /* clang-format on */

    fio_u256 result_256 = fio_sha256_hmac(key, 131, data, 152);
    FIO_ASSERT(!memcmp(result_256.u8, expected_256, 32),
               "HMAC-SHA256 test case 7 failed (large key + data)");

    fio_u512 result_512 = fio_sha512_hmac(key, 131, data, 152);
    FIO_ASSERT(!memcmp(result_512.u8, expected_512, 64),
               "HMAC-SHA512 test case 7 failed (large key + data)");
  }

  /* Test empty message */
  {
    static const char *key = "key";
    fio_u256 result_256 = fio_sha256_hmac(key, 3, "", 0);
    fio_u512 result_512 = fio_sha512_hmac(key, 3, "", 0);
    /* Just verify it doesn't crash and produces non-zero output */
    uint8_t zero_256[32] = {0};
    uint8_t zero_512[64] = {0};
    FIO_ASSERT(memcmp(result_256.u8, zero_256, 32) != 0,
               "HMAC-SHA256 empty message produced zero output");
    FIO_ASSERT(memcmp(result_512.u8, zero_512, 64) != 0,
               "HMAC-SHA512 empty message produced zero output");
  }

#if HAVE_OPENSSL
  /* Compare with OpenSSL for random inputs */
  {
    FIO_LOG_DDEBUG("Comparing HMAC with OpenSSL");
    uint8_t key[64];
    uint8_t data[256];
    fio_rand_bytes(key, sizeof(key));
    fio_rand_bytes(data, sizeof(data));

    fio_u256 fio_result_256 =
        fio_sha256_hmac(key, sizeof(key), data, sizeof(data));
    fio_u512 fio_result_512 =
        fio_sha512_hmac(key, sizeof(key), data, sizeof(data));

    uint8_t openssl_256[32];
    uint8_t openssl_512[64];
    unsigned int len;

    HMAC(EVP_sha256(), key, sizeof(key), data, sizeof(data), openssl_256, &len);
    FIO_ASSERT(!memcmp(fio_result_256.u8, openssl_256, 32),
               "HMAC-SHA256 mismatch with OpenSSL");

    HMAC(EVP_sha512(), key, sizeof(key), data, sizeof(data), openssl_512, &len);
    FIO_ASSERT(!memcmp(fio_result_512.u8, openssl_512, 64),
               "HMAC-SHA512 mismatch with OpenSSL");
  }
#endif

  FIO_LOG_DDEBUG("HMAC tests passed.");
}

int main(void) {
  FIO_NAME_TEST(stl, sha1)();
  FIO_NAME_TEST(stl, sha2)();
  FIO_NAME_TEST(stl, hmac)();
}
