/* *****************************************************************************
Test for 152 sha2z hkdf.h

Coverage: HKDF-Extract, HKDF-Expand, and combined HKDF against RFC 5869 test
vectors for SHA-256; SHA-384 variant smoke tests; incremental output length
boundaries; and salt/info sensitivity. Performance loops are intentionally
omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SHA2
#define FIO_HKDF
#include FIO_INCLUDE_FILE

/* *****************************************************************************
RFC 5869 Test Case 1 - Basic SHA-256
***************************************************************************** */

static void fio___test_hkdf_rfc5869_case1(void) {
  static const uint8_t ikm[22] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b};
  static const uint8_t salt[13] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c};
  static const uint8_t info[10] = {
      0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9};
  static const uint8_t expected_prk[32] = {
      0x07, 0x77, 0x09, 0x36, 0x2c, 0x2e, 0x32, 0xdf, 0x0d, 0xdc, 0x3f,
      0x0d, 0xc4, 0x7b, 0xba, 0x63, 0x90, 0xb6, 0xc7, 0x3b, 0xb5, 0x0f,
      0x9c, 0x31, 0x22, 0xec, 0x84, 0x4a, 0xd7, 0xc2, 0xb3, 0xe5};
  static const uint8_t expected_okm[42] = {
      0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a, 0x90, 0x43, 0x4f,
      0x64, 0xd0, 0x36, 0x2f, 0x2a, 0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a,
      0x5a, 0x4c, 0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf, 0x34,
      0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18, 0x58, 0x65};

  uint8_t prk[32];
  uint8_t okm[42];

  fio_hkdf_extract(prk, salt, sizeof(salt), ikm, sizeof(ikm), 0);
  FIO_ASSERT(!FIO_MEMCMP(prk, expected_prk, 32),
             "HKDF-Extract RFC 5869 case 1 PRK mismatch");

  fio_hkdf_expand(okm, sizeof(okm), prk, sizeof(prk), info, sizeof(info), 0);
  FIO_ASSERT(!FIO_MEMCMP(okm, expected_okm, 42),
             "HKDF-Expand RFC 5869 case 1 OKM mismatch");

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
             "Combined HKDF RFC 5869 case 1 OKM mismatch");
}

/* *****************************************************************************
RFC 5869 Test Case 2 - Longer SHA-256 inputs/outputs
***************************************************************************** */

static void fio___test_hkdf_rfc5869_case2(void) {
  uint8_t ikm[80];
  uint8_t salt[80];
  uint8_t info[80];
  for (size_t i = 0; i < 80; ++i) {
    ikm[i] = (uint8_t)i;
    salt[i] = (uint8_t)(0x60 + i);
    info[i] = (uint8_t)(0xb0 + i);
  }

  static const uint8_t expected_prk[32] = {
      0x06, 0xa6, 0xb8, 0x8c, 0x58, 0x53, 0x36, 0x1a, 0x06, 0x10, 0x4c,
      0x9c, 0xeb, 0x35, 0xb4, 0x5c, 0xef, 0x76, 0x00, 0x14, 0x90, 0x46,
      0x71, 0x01, 0x4a, 0x19, 0x3f, 0x40, 0xc1, 0x5f, 0xc2, 0x44};
  static const uint8_t expected_okm[82] = {
      0xb1, 0x1e, 0x39, 0x8d, 0xc8, 0x03, 0x27, 0xa1, 0xc8, 0xe7, 0xf7,
      0x8c, 0x59, 0x6a, 0x49, 0x34, 0x4f, 0x01, 0x2e, 0xda, 0x2d, 0x4e,
      0xfa, 0xd8, 0xa0, 0x50, 0xcc, 0x4c, 0x19, 0xaf, 0xa9, 0x7c, 0x59,
      0x04, 0x5a, 0x99, 0xca, 0xc7, 0x82, 0x72, 0x71, 0xcb, 0x41, 0xc6,
      0x5e, 0x59, 0x0e, 0x09, 0xda, 0x32, 0x75, 0x60, 0x0c, 0x2f, 0x09,
      0xb8, 0x36, 0x77, 0x93, 0xa9, 0xac, 0xa3, 0xdb, 0x71, 0xcc, 0x30,
      0xc5, 0x81, 0x79, 0xec, 0x3e, 0x87, 0xc1, 0x4c, 0x01, 0xd5, 0xc1,
      0xf3, 0x43, 0x4f, 0x1d, 0x87};

  uint8_t prk[32];
  uint8_t okm[82];

  fio_hkdf_extract(prk, salt, sizeof(salt), ikm, sizeof(ikm), 0);
  FIO_ASSERT(!FIO_MEMCMP(prk, expected_prk, 32),
             "HKDF-Extract RFC 5869 case 2 PRK mismatch");

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
             "Combined HKDF RFC 5869 case 2 OKM mismatch");
}

/* *****************************************************************************
RFC 5869 Test Case 3 - Zero-length salt/info (SHA-256)
***************************************************************************** */

static void fio___test_hkdf_rfc5869_case3(void) {
  static const uint8_t ikm[22] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b};
  static const uint8_t expected_prk[32] = {
      0x19, 0xef, 0x24, 0xa3, 0x2c, 0x71, 0x7b, 0x16, 0x7f, 0x33, 0xa9,
      0x1d, 0x6f, 0x64, 0x8b, 0xdf, 0x96, 0x59, 0x67, 0x76, 0xaf, 0xdb,
      0x63, 0x77, 0xac, 0x43, 0x4c, 0x1c, 0x29, 0x3c, 0xcb, 0x04};
  static const uint8_t expected_okm[42] = {
      0x8d, 0xa4, 0xe7, 0x75, 0xa5, 0x63, 0xc1, 0x8f, 0x71, 0x5f, 0x80,
      0x2a, 0x06, 0x3c, 0x5a, 0x31, 0xb8, 0xa1, 0x1f, 0x5c, 0x5e, 0xe1,
      0x87, 0x9e, 0xc3, 0x45, 0x4e, 0x5f, 0x3c, 0x73, 0x8d, 0x2d, 0x9d,
      0x20, 0x13, 0x95, 0xfa, 0xa4, 0xb6, 0x1a, 0x96, 0xc8};

  uint8_t prk[32];
  uint8_t okm[42];

  fio_hkdf_extract(prk, NULL, 0, ikm, sizeof(ikm), 0);
  FIO_ASSERT(!FIO_MEMCMP(prk, expected_prk, 32),
             "HKDF-Extract RFC 5869 case 3 PRK mismatch");

  fio_hkdf(okm, sizeof(okm), NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(!FIO_MEMCMP(okm, expected_okm, 42),
             "Combined HKDF RFC 5869 case 3 OKM mismatch");
}

/* *****************************************************************************
SHA-384 Variant Smoke Tests
***************************************************************************** */

static void fio___test_hkdf_sha384(void) {
  static const uint8_t ikm[22] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b};
  static const uint8_t salt[13] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c};
  static const uint8_t info[10] = {
      0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9};

  uint8_t prk[48];
  uint8_t okm[64];
  uint8_t zero_prk[48] = {0};
  uint8_t zero_okm[64] = {0};

  fio_hkdf_extract(prk, salt, sizeof(salt), ikm, sizeof(ikm), 1);
  FIO_ASSERT(FIO_MEMCMP(prk, zero_prk, 48) != 0,
             "HKDF-Extract SHA-384 produced zero PRK");

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
             "HKDF SHA-384 produced zero OKM");
}

/* *****************************************************************************
HKDF Output-Length and Sensitivity Edge Cases
***************************************************************************** */

static void fio___test_hkdf_edge_cases(void) {
  static const uint8_t ikm[16] = {
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
      0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
  uint8_t okm[256];
  uint8_t zero[256] = {0};

  /* Minimum non-trivial output length. */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm, 1, NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(okm[0] != 0, "HKDF 1-byte output is zero");

  /* Exactly one hash block. */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm, 32, NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm, zero, 32) != 0, "HKDF 32-byte output is all zeros");

  /* Just past one hash block. */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm, 33, NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm, zero, 33) != 0, "HKDF 33-byte output is all zeros");

  /* Two hash blocks. */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm, 64, NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm, zero, 64) != 0, "HKDF 64-byte output is all zeros");

  /* Near maximum (255 hash blocks for SHA-256 = 8160 bytes). */
  FIO_MEMSET(okm, 0, sizeof(okm));
  fio_hkdf(okm, 255, NULL, 0, ikm, sizeof(ikm), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm, zero, 255) != 0,
             "HKDF 255-byte output is all zeros");
}

/* *****************************************************************************
HKDF Sensitivity and Determinism
***************************************************************************** */

static void fio___test_hkdf_sensitivity(void) {
  static const uint8_t ikm1[16] = {
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
      0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
  static const uint8_t ikm2[16] = {
      0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
      0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};
  static const uint8_t salt1[8] = {
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  static const uint8_t salt2[8] = {
      0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18};
  static const uint8_t info1[4] = {0x01, 0x02, 0x03, 0x04};
  static const uint8_t info2[4] = {0x11, 0x12, 0x13, 0x14};

  uint8_t okm1[32], okm2[32];

  /* Different IKM produces different OKM. */
  fio_hkdf(okm1, 32, NULL, 0, ikm1, sizeof(ikm1), NULL, 0, 0);
  fio_hkdf(okm2, 32, NULL, 0, ikm2, sizeof(ikm2), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm1, okm2, 32) != 0,
             "HKDF OKM insensitive to IKM");

  /* Different salt produces different OKM. */
  fio_hkdf(okm1, 32, salt1, sizeof(salt1), ikm1, sizeof(ikm1), NULL, 0, 0);
  fio_hkdf(okm2, 32, salt2, sizeof(salt2), ikm1, sizeof(ikm1), NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(okm1, okm2, 32) != 0,
             "HKDF OKM insensitive to salt");

  /* Different info produces different OKM. */
  fio_hkdf(okm1, 32, NULL, 0, ikm1, sizeof(ikm1), info1, sizeof(info1), 0);
  fio_hkdf(okm2, 32, NULL, 0, ikm1, sizeof(ikm1), info2, sizeof(info2), 0);
  FIO_ASSERT(FIO_MEMCMP(okm1, okm2, 32) != 0,
             "HKDF OKM insensitive to info");

  /* SHA-256 and SHA-384 produce different outputs. */
  fio_hkdf(okm1, 32, NULL, 0, ikm1, sizeof(ikm1), NULL, 0, 0);
  fio_hkdf(okm2, 32, NULL, 0, ikm1, sizeof(ikm1), NULL, 0, 1);
  FIO_ASSERT(FIO_MEMCMP(okm1, okm2, 32) != 0,
             "HKDF SHA-256 and SHA-384 produced identical OKM");

  /* Same inputs produce same outputs. */
  fio_hkdf(okm1, 32, NULL, 0, ikm1, sizeof(ikm1), info1, sizeof(info1), 0);
  fio_hkdf(okm2, 32, NULL, 0, ikm1, sizeof(ikm1), info1, sizeof(info1), 0);
  FIO_ASSERT(!FIO_MEMCMP(okm1, okm2, 32), "HKDF not deterministic");
}

int main(void) {
  fio___test_hkdf_rfc5869_case1();
  fio___test_hkdf_rfc5869_case2();
  fio___test_hkdf_rfc5869_case3();
  fio___test_hkdf_sha384();
  fio___test_hkdf_edge_cases();
  fio___test_hkdf_sensitivity();
  return 0;
}
