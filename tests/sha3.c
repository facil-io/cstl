/* *****************************************************************************
SHA-3 Tests
***************************************************************************** */
#define FIO_LOG
#define FIO_SHA3
#include "test-helpers.h"

/* Test vectors from NIST FIPS 202 */

/* SHA3-256 test vectors */
static void fio___test_sha3_256_empty(void) {
  /* SHA3-256("") */
  static const uint8_t expected[32] = {
      0xa7, 0xff, 0xc6, 0xf8, 0xbf, 0x1e, 0xd7, 0x66, 0x51, 0xc1, 0x47,
      0x56, 0xa0, 0x61, 0xd6, 0x62, 0xf5, 0x80, 0xff, 0x4d, 0xe4, 0x3b,
      0x49, 0xfa, 0x82, 0xd8, 0x0a, 0x4b, 0x80, 0xf8, 0x43, 0x4a};
  uint8_t out[32];
  fio_sha3_256(out, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 32),
             "SHA3-256 empty string test failed");
  FIO_LOG_DDEBUG("SHA3-256 empty string: PASSED");
}

static void fio___test_sha3_256_abc(void) {
  /* SHA3-256("abc") */
  static const uint8_t expected[32] = {
      0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2, 0x04, 0x5c, 0x17,
      0x2d, 0x6b, 0xd3, 0x90, 0xbd, 0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d,
      0x52, 0x5b, 0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32};
  uint8_t out[32];
  fio_sha3_256(out, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 32), "SHA3-256 'abc' test failed");
  FIO_LOG_DDEBUG("SHA3-256 'abc': PASSED");
}

/* SHA3-512 test vectors */
static void fio___test_sha3_512_empty(void) {
  /* SHA3-512("") */
  static const uint8_t expected[64] = {
      0xa6, 0x9f, 0x73, 0xcc, 0xa2, 0x3a, 0x9a, 0xc5, 0xc8, 0xb5, 0x67,
      0xdc, 0x18, 0x5a, 0x75, 0x6e, 0x97, 0xc9, 0x82, 0x16, 0x4f, 0xe2,
      0x58, 0x59, 0xe0, 0xd1, 0xdc, 0xc1, 0x47, 0x5c, 0x80, 0xa6, 0x15,
      0xb2, 0x12, 0x3a, 0xf1, 0xf5, 0xf9, 0x4c, 0x11, 0xe3, 0xe9, 0x40,
      0x2c, 0x3a, 0xc5, 0x58, 0xf5, 0x00, 0x19, 0x9d, 0x95, 0xb6, 0xd3,
      0xe3, 0x01, 0x75, 0x85, 0x86, 0x28, 0x1d, 0xcd, 0x26};
  uint8_t out[64];
  fio_sha3_512(out, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64),
             "SHA3-512 empty string test failed");
  FIO_LOG_DDEBUG("SHA3-512 empty string: PASSED");
}

static void fio___test_sha3_512_abc(void) {
  /* SHA3-512("abc") */
  static const uint8_t expected[64] = {
      0xb7, 0x51, 0x85, 0x0b, 0x1a, 0x57, 0x16, 0x8a, 0x56, 0x93, 0xcd,
      0x92, 0x4b, 0x6b, 0x09, 0x6e, 0x08, 0xf6, 0x21, 0x82, 0x74, 0x44,
      0xf7, 0x0d, 0x88, 0x4f, 0x5d, 0x02, 0x40, 0xd2, 0x71, 0x2e, 0x10,
      0xe1, 0x16, 0xe9, 0x19, 0x2a, 0xf3, 0xc9, 0x1a, 0x7e, 0xc5, 0x76,
      0x47, 0xe3, 0x93, 0x40, 0x57, 0x34, 0x0b, 0x4c, 0xf4, 0x08, 0xd5,
      0xa5, 0x65, 0x92, 0xf8, 0x27, 0x4e, 0xec, 0x53, 0xf0};
  uint8_t out[64];
  fio_sha3_512(out, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64), "SHA3-512 'abc' test failed");
  FIO_LOG_DDEBUG("SHA3-512 'abc': PASSED");
}

/* SHA3-224 test */
static void fio___test_sha3_224_abc(void) {
  /* SHA3-224("abc") */
  static const uint8_t expected[28] = {
      0xe6, 0x42, 0x82, 0x4c, 0x3f, 0x8c, 0xf2, 0x4a, 0xd0, 0x92,
      0x34, 0xee, 0x7d, 0x3c, 0x76, 0x6f, 0xc9, 0xa3, 0xa5, 0x16,
      0x8d, 0x0c, 0x94, 0xad, 0x73, 0xb4, 0x6f, 0xdf};
  uint8_t out[28];
  fio_sha3_224(out, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 28), "SHA3-224 'abc' test failed");
  FIO_LOG_DDEBUG("SHA3-224 'abc': PASSED");
}

/* SHA3-384 test */
static void fio___test_sha3_384_abc(void) {
  /* SHA3-384("abc") */
  static const uint8_t expected[48] = {
      0xec, 0x01, 0x49, 0x82, 0x88, 0x51, 0x6f, 0xc9, 0x26, 0x45, 0x9f, 0x58,
      0xe2, 0xc6, 0xad, 0x8d, 0xf9, 0xb4, 0x73, 0xcb, 0x0f, 0xc0, 0x8c, 0x25,
      0x96, 0xda, 0x7c, 0xf0, 0xe4, 0x9b, 0xe4, 0xb2, 0x98, 0xd8, 0x8c, 0xea,
      0x92, 0x7a, 0xc7, 0xf5, 0x39, 0xf1, 0xed, 0xf2, 0x28, 0x37, 0x6d, 0x25};
  uint8_t out[48];
  fio_sha3_384(out, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 48), "SHA3-384 'abc' test failed");
  FIO_LOG_DDEBUG("SHA3-384 'abc': PASSED");
}

/* SHAKE128 test */
static void fio___test_shake128(void) {
  /* SHAKE128("abc", 32 bytes output) */
  static const uint8_t expected[32] = {
      0x58, 0x81, 0x09, 0x2d, 0xd8, 0x18, 0xbf, 0x5c, 0xf8, 0xa3, 0xdd,
      0xb7, 0x93, 0xfb, 0xcb, 0xa7, 0x40, 0x97, 0xd5, 0xc5, 0x26, 0xa6,
      0xd3, 0x5f, 0x97, 0xb8, 0x33, 0x51, 0x94, 0x0f, 0x2c, 0xc8};
  uint8_t out[32];
  fio_shake128(out, 32, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 32), "SHAKE128 'abc' test failed");
  FIO_LOG_DDEBUG("SHAKE128 'abc' (32 bytes): PASSED");
}

/* SHAKE256 test */
static void fio___test_shake256(void) {
  /* SHAKE256("abc", 64 bytes output) */
  static const uint8_t expected[64] = {
      0x48, 0x33, 0x66, 0x60, 0x13, 0x60, 0xa8, 0x77, 0x1c, 0x68, 0x63,
      0x08, 0x0c, 0xc4, 0x11, 0x4d, 0x8d, 0xb4, 0x45, 0x30, 0xf8, 0xf1,
      0xe1, 0xee, 0x4f, 0x94, 0xea, 0x37, 0xe7, 0x8b, 0x57, 0x39, 0xd5,
      0xa1, 0x5b, 0xef, 0x18, 0x6a, 0x53, 0x86, 0xc7, 0x57, 0x44, 0xc0,
      0x52, 0x7e, 0x1f, 0xaa, 0x9f, 0x87, 0x26, 0xe4, 0x62, 0xa1, 0x2a,
      0x4f, 0xeb, 0x06, 0xbd, 0x88, 0x01, 0xe7, 0x51, 0xe4};
  uint8_t out[64];
  fio_shake256(out, 64, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64), "SHAKE256 'abc' test failed");
  FIO_LOG_DDEBUG("SHAKE256 'abc' (64 bytes): PASSED");
}

/* Streaming test */
static void fio___test_sha3_streaming(void) {
  const char *msg = "The quick brown fox jumps over the lazy dog";
  size_t len = strlen(msg);

  uint8_t out1[32], out2[32];

  /* One-shot */
  fio_sha3_256(out1, msg, len);

  /* Streaming - feed one byte at a time */
  fio_sha3_s h = fio_sha3_256_init();
  for (size_t i = 0; i < len; ++i)
    fio_sha3_consume(&h, msg + i, 1);
  fio_sha3_finalize(&h, out2);

  FIO_ASSERT(!FIO_MEMCMP(out1, out2, 32), "SHA3-256 streaming test failed");
  FIO_LOG_DDEBUG("SHA3-256 streaming: PASSED");
}

/* Edge case tests */
static void fio___test_sha3_edge_cases(void) {
  FIO_LOG_DDEBUG("Testing SHA-3 edge cases...");

  /* Test: Single byte input */
  {
    FIO_LOG_DDEBUG("  Testing single byte input...");
    uint8_t data[1] = {0x00};
    uint8_t out256[32], out512[64];

    fio_sha3_256(out256, data, 1);
    fio_sha3_512(out512, data, 1);

    /* Verify non-zero output */
    int zero256 = 1, zero512 = 1;
    for (int i = 0; i < 32; ++i)
      if (out256[i] != 0)
        zero256 = 0;
    for (int i = 0; i < 64; ++i)
      if (out512[i] != 0)
        zero512 = 0;

    FIO_ASSERT(!zero256, "SHA3-256 single byte should not be all zeros");
    FIO_ASSERT(!zero512, "SHA3-512 single byte should not be all zeros");
  }

  /* Test: Block boundary inputs (SHA3-256 rate = 136 bytes) */
  {
    FIO_LOG_DDEBUG("  Testing block boundary inputs...");
    size_t test_sizes[] = {135, 136, 137, 271, 272, 273};

    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); ++i) {
      size_t len = test_sizes[i];
      uint8_t *data = (uint8_t *)malloc(len);
      uint8_t out[32];
      FIO_ASSERT(data, "Memory allocation failed");
      FIO_MEMSET(data, 'A', len);

      fio_sha3_256(out, data, len);

      int zero = 1;
      for (int j = 0; j < 32; ++j)
        if (out[j] != 0)
          zero = 0;
      FIO_ASSERT(!zero, "SHA3-256 at %zu bytes should not be all zeros", len);

      free(data);
    }
  }

  /* Test: Incremental vs one-shot with various chunk sizes */
  {
    FIO_LOG_DDEBUG("  Testing incremental vs one-shot...");
    uint8_t data[1000];
    for (size_t i = 0; i < 1000; ++i)
      data[i] = (uint8_t)(i & 0xFF);

    uint8_t out1[32], out2[32];

    /* One-shot */
    fio_sha3_256(out1, data, 1000);

    /* Incremental - various chunk sizes */
    fio_sha3_s h = fio_sha3_256_init();
    fio_sha3_consume(&h, data, 100);
    fio_sha3_consume(&h, data + 100, 400);
    fio_sha3_consume(&h, data + 500, 500);
    fio_sha3_finalize(&h, out2);

    FIO_ASSERT(!FIO_MEMCMP(out1, out2, 32), "SHA3-256 incremental mismatch");
  }

  /* Test: Large input (1MB) */
  {
    FIO_LOG_DDEBUG("  Testing large input (1MB)...");
    size_t len = 1024 * 1024;
    uint8_t *data = (uint8_t *)malloc(len);
    uint8_t out[32];
    FIO_ASSERT(data, "Memory allocation failed");

    for (size_t i = 0; i < len; ++i)
      data[i] = (uint8_t)(i & 0xFF);

    fio_sha3_256(out, data, len);

    int zero = 1;
    for (int i = 0; i < 32; ++i)
      if (out[i] != 0)
        zero = 0;
    FIO_ASSERT(!zero, "SHA3-256 1MB should not be all zeros");

    /* Verify determinism */
    uint8_t out2[32];
    fio_sha3_256(out2, data, len);
    FIO_ASSERT(!FIO_MEMCMP(out, out2, 32), "SHA3-256 should be deterministic");

    free(data);
  }

  /* Test: SHAKE variable output lengths */
  {
    FIO_LOG_DDEBUG("  Testing SHAKE variable output lengths...");
    uint8_t data[] = "test input";
    size_t data_len = sizeof(data) - 1;

    /* Test various output lengths for SHAKE128 */
    size_t out_lens[] = {1, 16, 32, 64, 128, 256};
    for (size_t i = 0; i < sizeof(out_lens) / sizeof(out_lens[0]); ++i) {
      size_t out_len = out_lens[i];
      uint8_t *out = (uint8_t *)malloc(out_len);
      FIO_ASSERT(out, "Memory allocation failed");

      fio_shake128(out, out_len, data, data_len);

      int zero = 1;
      for (size_t j = 0; j < out_len; ++j)
        if (out[j] != 0)
          zero = 0;
      FIO_ASSERT(!zero,
                 "SHAKE128 %zu-byte output should not be all zeros",
                 out_len);

      free(out);
    }

    /* Test various output lengths for SHAKE256 */
    for (size_t i = 0; i < sizeof(out_lens) / sizeof(out_lens[0]); ++i) {
      size_t out_len = out_lens[i];
      uint8_t *out = (uint8_t *)malloc(out_len);
      FIO_ASSERT(out, "Memory allocation failed");

      fio_shake256(out, out_len, data, data_len);

      int zero = 1;
      for (size_t j = 0; j < out_len; ++j)
        if (out[j] != 0)
          zero = 0;
      FIO_ASSERT(!zero,
                 "SHAKE256 %zu-byte output should not be all zeros",
                 out_len);

      free(out);
    }
  }

  /* Test: SHAKE with 1 byte output */
  {
    FIO_LOG_DDEBUG("  Testing SHAKE with 1 byte output...");
    uint8_t data[] = "test";
    uint8_t out128[1], out256[1];

    fio_shake128(out128, 1, data, 4);
    fio_shake256(out256, 1, data, 4);

    /* Just verify it doesn't crash and produces output */
    FIO_LOG_DDEBUG("    SHAKE128(1 byte) = 0x%02x", out128[0]);
    FIO_LOG_DDEBUG("    SHAKE256(1 byte) = 0x%02x", out256[0]);
  }

  /* Test: All zeros input */
  {
    FIO_LOG_DDEBUG("  Testing all-zeros input...");
    uint8_t data[64] = {0};
    uint8_t out[32];

    fio_sha3_256(out, data, 64);

    int zero = 1;
    for (int i = 0; i < 32; ++i)
      if (out[i] != 0)
        zero = 0;
    FIO_ASSERT(!zero, "SHA3-256 of zeros should not be all zeros");
  }

  /* Test: All ones input */
  {
    FIO_LOG_DDEBUG("  Testing all-ones input...");
    uint8_t data[64];
    uint8_t out[32];
    FIO_MEMSET(data, 0xFF, 64);

    fio_sha3_256(out, data, 64);

    int zero = 1;
    for (int i = 0; i < 32; ++i)
      if (out[i] != 0)
        zero = 0;
    FIO_ASSERT(!zero, "SHA3-256 of ones should not be all zeros");
  }

  /* Test: Different inputs produce different hashes */
  {
    FIO_LOG_DDEBUG("  Testing different inputs produce different hashes...");
    uint8_t data1[32] = {0};
    uint8_t data2[32] = {0};
    data2[0] = 1;

    uint8_t hash1[32], hash2[32];
    fio_sha3_256(hash1, data1, 32);
    fio_sha3_256(hash2, data2, 32);

    FIO_ASSERT(FIO_MEMCMP(hash1, hash2, 32) != 0,
               "Different inputs should produce different hashes");
  }

  /* Test: All SHA3 variants produce different outputs for same input */
  {
    FIO_LOG_DDEBUG("  Testing all SHA3 variants produce different outputs...");
    uint8_t data[] = "test input for all variants";
    size_t data_len = sizeof(data) - 1;

    uint8_t out224[28], out256[32], out384[48], out512[64];
    fio_sha3_224(out224, data, data_len);
    fio_sha3_256(out256, data, data_len);
    fio_sha3_384(out384, data, data_len);
    fio_sha3_512(out512, data, data_len);

    /* Compare first 28 bytes (minimum common length) */
    FIO_ASSERT(FIO_MEMCMP(out224, out256, 28) != 0,
               "SHA3-224 and SHA3-256 should differ");
    FIO_ASSERT(FIO_MEMCMP(out256, out384, 28) != 0,
               "SHA3-256 and SHA3-384 should differ");
    FIO_ASSERT(FIO_MEMCMP(out384, out512, 28) != 0,
               "SHA3-384 and SHA3-512 should differ");
  }

  FIO_LOG_DDEBUG("SHA-3 edge case tests passed!");
}

int main(void) {
  FIO_LOG_DDEBUG("Testing SHA-3 implementation...");

  /* SHA3 tests */
  FIO_LOG_DDEBUG("=== SHA3-224 Tests ===");
  fio___test_sha3_224_abc();

  FIO_LOG_DDEBUG("=== SHA3-256 Tests ===");
  fio___test_sha3_256_empty();
  fio___test_sha3_256_abc();

  FIO_LOG_DDEBUG("=== SHA3-384 Tests ===");
  fio___test_sha3_384_abc();

  FIO_LOG_DDEBUG("=== SHA3-512 Tests ===");
  fio___test_sha3_512_empty();
  fio___test_sha3_512_abc();

  /* SHAKE tests */
  FIO_LOG_DDEBUG("=== SHAKE Tests ===");
  fio___test_shake128();
  fio___test_shake256();

  /* Streaming test */
  FIO_LOG_DDEBUG("=== Streaming Tests ===");
  fio___test_sha3_streaming();

  /* Edge case tests */
  FIO_LOG_DDEBUG("=== Edge Case Tests ===");
  fio___test_sha3_edge_cases();

  FIO_LOG_DDEBUG("All SHA-3 tests passed!");
  return 0;
}
