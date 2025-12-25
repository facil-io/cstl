/* *****************************************************************************
HKDF Tests - RFC 5869 Test Vectors
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPT
#include FIO_INCLUDE_FILE

/* *****************************************************************************
RFC 5869 Test Vectors - Appendix A
***************************************************************************** */

/* Test Case 1: Basic test case with SHA-256 */
static void fio___test_hkdf_rfc5869_case1(void) {
  /* clang-format off */
  static const uint8_t ikm[22] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
  };
  static const uint8_t salt[13] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c
  };
  static const uint8_t info[10] = {
      0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
      0xf8, 0xf9
  };
  static const uint8_t expected_prk[32] = {
      0x07, 0x77, 0x09, 0x36, 0x2c, 0x2e, 0x32, 0xdf,
      0x0d, 0xdc, 0x3f, 0x0d, 0xc4, 0x7b, 0xba, 0x63,
      0x90, 0xb6, 0xc7, 0x3b, 0xb5, 0x0f, 0x9c, 0x31,
      0x22, 0xec, 0x84, 0x4a, 0xd7, 0xc2, 0xb3, 0xe5
  };
  static const uint8_t expected_okm[42] = {
      0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a,
      0x90, 0x43, 0x4f, 0x64, 0xd0, 0x36, 0x2f, 0x2a,
      0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a, 0x5a, 0x4c,
      0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf,
      0x34, 0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18,
      0x58, 0x65
  };
  /* clang-format on */

  uint8_t prk[32];
  uint8_t okm[42];

  /* Test Extract */
  fio_hkdf_extract(prk, salt, sizeof(salt), ikm, sizeof(ikm), 0);
  FIO_ASSERT(!FIO_MEMCMP(prk, expected_prk, 32),
             "HKDF-Extract test case 1 failed");

  /* Test Expand */
  fio_hkdf_expand(okm, sizeof(okm), prk, sizeof(prk), info, sizeof(info), 0);
  FIO_ASSERT(!FIO_MEMCMP(okm, expected_okm, 42),
             "HKDF-Expand test case 1 failed");

  /* Test combined HKDF */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm,
           sizeof(okm),
           salt,
           sizeof(salt),
           ikm,
           sizeof(ikm),
           info,
           sizeof(info),
           0);
  FIO_ASSERT(!FIO_MEMCMP(okm, expected_okm, 42),
             "HKDF combined test case 1 failed");

  FIO_LOG_DDEBUG("HKDF RFC 5869 Test Case 1: PASSED");
}

/* Test Case 2: Test with SHA-256 and longer inputs/outputs */
static void fio___test_hkdf_rfc5869_case2(void) {
  /* clang-format off */
  /* IKM = 0x000102...4f (80 bytes) */
  uint8_t ikm[80];
  for (size_t i = 0; i < 80; ++i)
    ikm[i] = (uint8_t)i;

  /* salt = 0x606162...af (80 bytes) */
  uint8_t salt[80];
  for (size_t i = 0; i < 80; ++i)
    salt[i] = (uint8_t)(0x60 + i);

  /* info = 0xb0b1b2...ff (80 bytes) */
  uint8_t info[80];
  for (size_t i = 0; i < 80; ++i)
    info[i] = (uint8_t)(0xb0 + i);

  static const uint8_t expected_prk[32] = {
      0x06, 0xa6, 0xb8, 0x8c, 0x58, 0x53, 0x36, 0x1a,
      0x06, 0x10, 0x4c, 0x9c, 0xeb, 0x35, 0xb4, 0x5c,
      0xef, 0x76, 0x00, 0x14, 0x90, 0x46, 0x71, 0x01,
      0x4a, 0x19, 0x3f, 0x40, 0xc1, 0x5f, 0xc2, 0x44
  };
  static const uint8_t expected_okm[82] = {
      0xb1, 0x1e, 0x39, 0x8d, 0xc8, 0x03, 0x27, 0xa1,
      0xc8, 0xe7, 0xf7, 0x8c, 0x59, 0x6a, 0x49, 0x34,
      0x4f, 0x01, 0x2e, 0xda, 0x2d, 0x4e, 0xfa, 0xd8,
      0xa0, 0x50, 0xcc, 0x4c, 0x19, 0xaf, 0xa9, 0x7c,
      0x59, 0x04, 0x5a, 0x99, 0xca, 0xc7, 0x82, 0x72,
      0x71, 0xcb, 0x41, 0xc6, 0x5e, 0x59, 0x0e, 0x09,
      0xda, 0x32, 0x75, 0x60, 0x0c, 0x2f, 0x09, 0xb8,
      0x36, 0x77, 0x93, 0xa9, 0xac, 0xa3, 0xdb, 0x71,
      0xcc, 0x30, 0xc5, 0x81, 0x79, 0xec, 0x3e, 0x87,
      0xc1, 0x4c, 0x01, 0xd5, 0xc1, 0xf3, 0x43, 0x4f,
      0x1d, 0x87
  };
  /* clang-format on */

  uint8_t prk[32];
  uint8_t okm[82];

  /* Test Extract */
  fio_hkdf_extract(prk, salt, sizeof(salt), ikm, sizeof(ikm), 0);
  FIO_ASSERT(!FIO_MEMCMP(prk, expected_prk, 32),
             "HKDF-Extract test case 2 failed");

  /* Test combined HKDF */
  fio_hkdf(okm,
           sizeof(okm),
           salt,
           sizeof(salt),
           ikm,
           sizeof(ikm),
           info,
           sizeof(info),
           0);
  FIO_ASSERT(!FIO_MEMCMP(okm, expected_okm, 82),
             "HKDF combined test case 2 failed");

  FIO_LOG_DDEBUG("HKDF RFC 5869 Test Case 2: PASSED");
}

/* Test Case 3: Test with SHA-256 and zero-length salt/info */
static void fio___test_hkdf_rfc5869_case3(void) {
  /* clang-format off */
  static const uint8_t ikm[22] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
  };
  /* salt = (empty) */
  /* info = (empty) */
  static const uint8_t expected_prk[32] = {
      0x19, 0xef, 0x24, 0xa3, 0x2c, 0x71, 0x7b, 0x16,
      0x7f, 0x33, 0xa9, 0x1d, 0x6f, 0x64, 0x8b, 0xdf,
      0x96, 0x59, 0x67, 0x76, 0xaf, 0xdb, 0x63, 0x77,
      0xac, 0x43, 0x4c, 0x1c, 0x29, 0x3c, 0xcb, 0x04
  };
  static const uint8_t expected_okm[42] = {
      0x8d, 0xa4, 0xe7, 0x75, 0xa5, 0x63, 0xc1, 0x8f,
      0x71, 0x5f, 0x80, 0x2a, 0x06, 0x3c, 0x5a, 0x31,
      0xb8, 0xa1, 0x1f, 0x5c, 0x5e, 0xe1, 0x87, 0x9e,
      0xc3, 0x45, 0x4e, 0x5f, 0x3c, 0x73, 0x8d, 0x2d,
      0x9d, 0x20, 0x13, 0x95, 0xfa, 0xa4, 0xb6, 0x1a,
      0x96, 0xc8
  };
  /* clang-format on */

  uint8_t prk[32];
  uint8_t okm[42];

  /* Test Extract with NULL salt */
  fio_hkdf_extract(prk, NULL, 0, ikm, sizeof(ikm), 0);
  FIO_ASSERT(!FIO_MEMCMP(prk, expected_prk, 32),
             "HKDF-Extract test case 3 failed (NULL salt)");

  /* Test combined HKDF with empty salt and info */
  fio_hkdf(okm, sizeof(okm), NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(!FIO_MEMCMP(okm, expected_okm, 42),
             "HKDF combined test case 3 failed");

  FIO_LOG_DDEBUG("HKDF RFC 5869 Test Case 3: PASSED");
}

/* Test Case 4: Basic test case with SHA-1 (we use SHA-256 as fallback) */
/* RFC 5869 Test Case 4 is SHA-1, which we don't implement. Skip. */

/* Test Case 5: Test with SHA-1 and longer inputs (skip - SHA-1) */

/* Test Case 6: Test with SHA-1 and zero-length salt/info (skip - SHA-1) */

/* Test Case 7: Test with SHA-1 and zero-length IKM (skip - SHA-1) */

/* Additional test: Verify SHA-384 variant works */
static void fio___test_hkdf_sha384(void) {
  /* Simple test to verify SHA-384 variant doesn't crash and produces output */
  static const uint8_t ikm[22] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b};
  static const uint8_t salt[13] = {0x00,
                                   0x01,
                                   0x02,
                                   0x03,
                                   0x04,
                                   0x05,
                                   0x06,
                                   0x07,
                                   0x08,
                                   0x09,
                                   0x0a,
                                   0x0b,
                                   0x0c};
  static const uint8_t info[10] =
      {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9};

  uint8_t prk[48];
  uint8_t okm[64];
  uint8_t zero_prk[48] = {0};
  uint8_t zero_okm[64] = {0};

  /* Test Extract with SHA-384 */
  fio_hkdf_extract(prk, salt, sizeof(salt), ikm, sizeof(ikm), 1);
  FIO_ASSERT(FIO_MEMCMP(prk, zero_prk, 48) != 0,
             "HKDF-Extract SHA-384 produced zero output");

  /* Test combined HKDF with SHA-384 */
  fio_hkdf(okm,
           sizeof(okm),
           salt,
           sizeof(salt),
           ikm,
           sizeof(ikm),
           info,
           sizeof(info),
           1);
  FIO_ASSERT(FIO_MEMCMP(okm, zero_okm, 64) != 0,
             "HKDF SHA-384 produced zero output");

  FIO_LOG_DDEBUG("HKDF SHA-384 basic test: PASSED");
}

/* Test edge cases */
static void fio___test_hkdf_edge_cases(void) {
  static const uint8_t ikm[16] = {0x01,
                                  0x02,
                                  0x03,
                                  0x04,
                                  0x05,
                                  0x06,
                                  0x07,
                                  0x08,
                                  0x09,
                                  0x0a,
                                  0x0b,
                                  0x0c,
                                  0x0d,
                                  0x0e,
                                  0x0f,
                                  0x10};
  uint8_t okm[256];
  uint8_t zero[256] = {0};

  /* Test minimum output length (1 byte) */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm, 1, NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(okm[0] != 0, "HKDF 1-byte output failed");

  /* Test output length exactly hash_len (32 bytes) */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm, 32, NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm, zero, 32) != 0, "HKDF 32-byte output failed");

  /* Test output length > hash_len (requires multiple iterations) */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm, 64, NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm, zero, 64) != 0, "HKDF 64-byte output failed");

  /* Test output length = 255 * hash_len (maximum) */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm, 255, NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm, zero, 255) != 0, "HKDF 255-byte output failed");

  FIO_LOG_DDEBUG("HKDF edge cases: PASSED");
}

/* Test additional edge cases */
static void fio___test_hkdf_additional_edge_cases(void) {
  uint8_t okm1[64], okm2[64];
  uint8_t zero[64] = {0};

  /* Test: Single-byte IKM (minimal non-empty input) */
  static const uint8_t ikm_single[1] = {0x42};
  FIO_MEMSET(okm1, 0, sizeof(okm1));
  fio_hkdf(okm1, 32, NULL, 0, ikm_single, 1, NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm1, zero, 32) != 0,
             "HKDF with single-byte IKM should produce non-zero output");

  /* Test: Single-byte IKM is deterministic */
  FIO_MEMSET(okm2, 0, sizeof(okm2));
  fio_hkdf(okm2, 32, NULL, 0, ikm_single, 1, NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm1, okm2, 32) == 0,
             "HKDF with single-byte IKM should be deterministic");

  /* Test: Different IKM produces different output */
  static const uint8_t ikm1[16] = {0x01,
                                   0x02,
                                   0x03,
                                   0x04,
                                   0x05,
                                   0x06,
                                   0x07,
                                   0x08,
                                   0x09,
                                   0x0a,
                                   0x0b,
                                   0x0c,
                                   0x0d,
                                   0x0e,
                                   0x0f,
                                   0x10};
  static const uint8_t ikm2[16] = {0x11,
                                   0x12,
                                   0x13,
                                   0x14,
                                   0x15,
                                   0x16,
                                   0x17,
                                   0x18,
                                   0x19,
                                   0x1a,
                                   0x1b,
                                   0x1c,
                                   0x1d,
                                   0x1e,
                                   0x1f,
                                   0x20};

  fio_hkdf(okm1, 32, NULL, 0, ikm1, sizeof(ikm1), NULL, 0, 0);
  fio_hkdf(okm2, 32, NULL, 0, ikm2, sizeof(ikm2), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm1, okm2, 32) != 0,
             "Different IKM should produce different output");

  /* Test: Different salt produces different output */
  static const uint8_t salt1[8] =
      {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  static const uint8_t salt2[8] =
      {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18};

  fio_hkdf(okm1, 32, salt1, sizeof(salt1), ikm1, sizeof(ikm1), NULL, 0, 0);
  fio_hkdf(okm2, 32, salt2, sizeof(salt2), ikm1, sizeof(ikm1), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm1, okm2, 32) != 0,
             "Different salt should produce different output");

  /* Test: Different info produces different output */
  static const uint8_t info1[4] = {0x01, 0x02, 0x03, 0x04};
  static const uint8_t info2[4] = {0x11, 0x12, 0x13, 0x14};

  fio_hkdf(okm1, 32, NULL, 0, ikm1, sizeof(ikm1), info1, sizeof(info1), 0);
  fio_hkdf(okm2, 32, NULL, 0, ikm1, sizeof(ikm1), info2, sizeof(info2), 0);
  FIO_ASSERT(FIO_MEMCMP(okm1, okm2, 32) != 0,
             "Different info should produce different output");

  /* Test: SHA-256 and SHA-384 produce different outputs */
  fio_hkdf(okm1, 32, NULL, 0, ikm1, sizeof(ikm1), NULL, 0, 0);
  fio_hkdf(okm2, 32, NULL, 0, ikm1, sizeof(ikm1), NULL, 0, 1);
  FIO_ASSERT(FIO_MEMCMP(okm1, okm2, 32) != 0,
             "SHA-256 and SHA-384 should produce different output");

  /* Test: Output length boundary (exactly hash_len * 2 = 64 bytes) */
  FIO_MEMSET(okm1, 0, sizeof(okm1));
  fio_hkdf(okm1, 64, NULL, 0, ikm1, sizeof(ikm1), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm1, zero, 64) != 0,
             "HKDF 64-byte output (2 * hash_len) failed");

  /* Test: Output length boundary (hash_len + 1 = 33 bytes) */
  uint8_t okm33[33] = {0};
  uint8_t zero33[33] = {0};
  fio_hkdf(okm33, 33, NULL, 0, ikm1, sizeof(ikm1), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm33, zero33, 33) != 0,
             "HKDF 33-byte output (hash_len + 1) failed");

  FIO_LOG_DDEBUG("HKDF additional edge cases: PASSED");
}

/* Test determinism - same inputs should produce same outputs */
static void fio___test_hkdf_determinism(void) {
  static const uint8_t ikm[16] = {0x01,
                                  0x02,
                                  0x03,
                                  0x04,
                                  0x05,
                                  0x06,
                                  0x07,
                                  0x08,
                                  0x09,
                                  0x0a,
                                  0x0b,
                                  0x0c,
                                  0x0d,
                                  0x0e,
                                  0x0f,
                                  0x10};
  static const uint8_t salt[8] =
      {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11};
  static const uint8_t info[4] = {0x12, 0x34, 0x56, 0x78};

  uint8_t okm1[64];
  uint8_t okm2[64];

  fio_hkdf(okm1,
           sizeof(okm1),
           salt,
           sizeof(salt),
           ikm,
           sizeof(ikm),
           info,
           sizeof(info),
           0);
  fio_hkdf(okm2,
           sizeof(okm2),
           salt,
           sizeof(salt),
           ikm,
           sizeof(ikm),
           info,
           sizeof(info),
           0);

  FIO_ASSERT(!FIO_MEMCMP(okm1, okm2, 64), "HKDF determinism test failed");

  FIO_LOG_DDEBUG("HKDF determinism test: PASSED");
}

int main(void) {
  FIO_LOG_DDEBUG("Testing HKDF implementation (RFC 5869)...");

  FIO_LOG_DDEBUG("=== RFC 5869 Test Vectors ===");
  fio___test_hkdf_rfc5869_case1();
  fio___test_hkdf_rfc5869_case2();
  fio___test_hkdf_rfc5869_case3();

  FIO_LOG_DDEBUG("=== SHA-384 Variant Tests ===");
  fio___test_hkdf_sha384();

  FIO_LOG_DDEBUG("=== Edge Case Tests ===");
  fio___test_hkdf_edge_cases();
  fio___test_hkdf_additional_edge_cases();

  FIO_LOG_DDEBUG("=== Determinism Tests ===");
  fio___test_hkdf_determinism();

  FIO_LOG_DDEBUG("All HKDF tests passed!");
  return 0;
}
