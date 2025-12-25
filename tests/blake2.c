/* *****************************************************************************
BLAKE2 Tests
***************************************************************************** */
#include "test-helpers.h"
#define FIO_LOG
#define FIO_BLAKE2
#include FIO_INCLUDE_FILE

/* Test vectors from RFC 7693 and BLAKE2 reference implementation */

/* BLAKE2b test vectors */
static void fio___test_blake2b_empty(void) {
  /* BLAKE2b-512("") */
  static const uint8_t expected[64] = {
      0x78, 0x6a, 0x02, 0xf7, 0x42, 0x01, 0x59, 0x03, 0xc6, 0xc6, 0xfd,
      0x85, 0x25, 0x52, 0xd2, 0x72, 0x91, 0x2f, 0x47, 0x40, 0xe1, 0x58,
      0x47, 0x61, 0x8a, 0x86, 0xe2, 0x17, 0xf7, 0x1f, 0x54, 0x19, 0xd2,
      0x5e, 0x10, 0x31, 0xaf, 0xee, 0x58, 0x53, 0x13, 0x89, 0x64, 0x44,
      0x93, 0x4e, 0xb0, 0x4b, 0x90, 0x3a, 0x68, 0x5b, 0x14, 0x48, 0xb7,
      0x55, 0xd5, 0x6f, 0x70, 0x1a, 0xfe, 0x9b, 0xe2, 0xce};
  uint8_t out[64];
  fio_blake2b(out, 64, NULL, 0, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64),
             "BLAKE2b-512 empty string test failed");
  FIO_LOG_DDEBUG("BLAKE2b-512 empty string: PASSED");
}

static void fio___test_blake2b_abc(void) {
  /* BLAKE2b-512("abc") */
  static const uint8_t expected[64] = {
      0xba, 0x80, 0xa5, 0x3f, 0x98, 0x1c, 0x4d, 0x0d, 0x6a, 0x27, 0x97,
      0xb6, 0x9f, 0x12, 0xf6, 0xe9, 0x4c, 0x21, 0x2f, 0x14, 0x68, 0x5a,
      0xc4, 0xb7, 0x4b, 0x12, 0xbb, 0x6f, 0xdb, 0xff, 0xa2, 0xd1, 0x7d,
      0x87, 0xc5, 0x39, 0x2a, 0xab, 0x79, 0x2d, 0xc2, 0x52, 0xd5, 0xde,
      0x45, 0x33, 0xcc, 0x95, 0x18, 0xd3, 0x8a, 0xa8, 0xdb, 0xf1, 0x92,
      0x5a, 0xb9, 0x23, 0x86, 0xed, 0xd4, 0x00, 0x99, 0x23};
  uint8_t out[64];
  fio_blake2b(out, 64, "abc", 3, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64), "BLAKE2b-512 'abc' test failed");
  FIO_LOG_DDEBUG("BLAKE2b-512 'abc': PASSED");
}

static void fio___test_blake2b_keyed(void) {
  /* BLAKE2b-512 keyed hash test from reference implementation */
  /* Key: 000102...3f (64 bytes), Message: 000102...fe (255 bytes) */
  uint8_t key[64];
  uint8_t msg[255];
  for (size_t i = 0; i < 64; ++i)
    key[i] = (uint8_t)i;
  for (size_t i = 0; i < 255; ++i)
    msg[i] = (uint8_t)i;

  /* Expected result for BLAKE2b(key=64 bytes, msg=255 bytes) */
  static const uint8_t expected[64] = {
      0x14, 0x27, 0x09, 0xd6, 0x2e, 0x28, 0xfc, 0xcc, 0xd0, 0xaf, 0x97,
      0xfa, 0xd0, 0xf8, 0x46, 0x5b, 0x97, 0x1e, 0x82, 0x20, 0x1d, 0xc5,
      0x10, 0x70, 0xfa, 0xa0, 0x37, 0x2a, 0xa4, 0x3e, 0x92, 0x48, 0x4b,
      0xe1, 0xc1, 0xe7, 0x3b, 0xa1, 0x09, 0x06, 0xd5, 0xd1, 0x85, 0x3d,
      0xb6, 0xa4, 0x10, 0x6e, 0x0a, 0x7b, 0xf9, 0x80, 0x0d, 0x37, 0x3d,
      0x6d, 0xee, 0x2d, 0x46, 0xd6, 0x2e, 0xf2, 0xa4, 0x61};
  uint8_t out[64];
  fio_blake2b(out, 64, msg, 255, key, 64);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64), "BLAKE2b-512 keyed test failed");
  FIO_LOG_DDEBUG("BLAKE2b-512 keyed (64-byte key, 255-byte msg): PASSED");
}

static void fio___test_blake2b_streaming(void) {
  /* Test streaming API produces same result as one-shot */
  const char *msg = "The quick brown fox jumps over the lazy dog";
  size_t len = strlen(msg);

  uint8_t out1[64], out2[64];

  /* One-shot */
  fio_blake2b(out1, 64, msg, len, NULL, 0);

  /* Streaming - feed one byte at a time */
  fio_blake2b_s h = fio_blake2b_init(64, NULL, 0);
  for (size_t i = 0; i < len; ++i)
    fio_blake2b_consume(&h, msg + i, 1);
  fio_blake2b_finalize(&h, out2);

  FIO_ASSERT(!FIO_MEMCMP(out1, out2, 64), "BLAKE2b streaming test failed");
  FIO_LOG_DDEBUG("BLAKE2b-512 streaming: PASSED");
}

/* BLAKE2s test vectors */
static void fio___test_blake2s_empty(void) {
  /* BLAKE2s-256("") */
  static const uint8_t expected[32] = {
      0x69, 0x21, 0x7a, 0x30, 0x79, 0x90, 0x80, 0x94, 0xe1, 0x11, 0x21,
      0xd0, 0x42, 0x35, 0x4a, 0x7c, 0x1f, 0x55, 0xb6, 0x48, 0x2c, 0xa1,
      0xa5, 0x1e, 0x1b, 0x25, 0x0d, 0xfd, 0x1e, 0xd0, 0xee, 0xf9};
  uint8_t out[32];
  fio_blake2s(out, 32, NULL, 0, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 32),
             "BLAKE2s-256 empty string test failed");
  FIO_LOG_DDEBUG("BLAKE2s-256 empty string: PASSED");
}

static void fio___test_blake2s_abc(void) {
  /* BLAKE2s-256("abc") */
  static const uint8_t expected[32] = {
      0x50, 0x8c, 0x5e, 0x8c, 0x32, 0x7c, 0x14, 0xe2, 0xe1, 0xa7, 0x2b,
      0xa3, 0x4e, 0xeb, 0x45, 0x2f, 0x37, 0x45, 0x8b, 0x20, 0x9e, 0xd6,
      0x3a, 0x29, 0x4d, 0x99, 0x9b, 0x4c, 0x86, 0x67, 0x59, 0x82};
  uint8_t out[32];
  fio_blake2s(out, 32, "abc", 3, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 32), "BLAKE2s-256 'abc' test failed");
  FIO_LOG_DDEBUG("BLAKE2s-256 'abc': PASSED");
}

static void fio___test_blake2s_streaming(void) {
  /* Test streaming API produces same result as one-shot */
  const char *msg = "The quick brown fox jumps over the lazy dog";
  size_t len = strlen(msg);

  uint8_t out1[32], out2[32];

  /* One-shot */
  fio_blake2s(out1, 32, msg, len, NULL, 0);

  /* Streaming - feed one byte at a time */
  fio_blake2s_s h = fio_blake2s_init(32, NULL, 0);
  for (size_t i = 0; i < len; ++i)
    fio_blake2s_consume(&h, msg + i, 1);
  fio_blake2s_finalize(&h, out2);

  FIO_ASSERT(!FIO_MEMCMP(out1, out2, 32), "BLAKE2s streaming test failed");
  FIO_LOG_DDEBUG("BLAKE2s-256 streaming: PASSED");
}

/* Edge case tests */
static void fio___test_blake2_edge_cases(void) {
  FIO_LOG_DDEBUG("Testing BLAKE2 edge cases...");

  /* Test: Single byte input */
  {
    FIO_LOG_DDEBUG("  Testing single byte input...");
    uint8_t data[1] = {0x00};
    uint8_t out_b[64], out_s[32];

    fio_blake2b(out_b, 64, data, 1, NULL, 0);
    fio_blake2s(out_s, 32, data, 1, NULL, 0);

    int zero_b = 1, zero_s = 1;
    for (int i = 0; i < 64; ++i)
      if (out_b[i] != 0)
        zero_b = 0;
    for (int i = 0; i < 32; ++i)
      if (out_s[i] != 0)
        zero_s = 0;

    FIO_ASSERT(!zero_b, "BLAKE2b single byte should not be all zeros");
    FIO_ASSERT(!zero_s, "BLAKE2s single byte should not be all zeros");
  }

  /* Test: Block boundary inputs (BLAKE2b block = 128 bytes, BLAKE2s = 64 bytes)
   */
  {
    FIO_LOG_DDEBUG("  Testing block boundary inputs...");
    size_t test_sizes[] = {63, 64, 65, 127, 128, 129};

    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); ++i) {
      size_t len = test_sizes[i];
      uint8_t *data = (uint8_t *)malloc(len);
      uint8_t out_b[64], out_s[32];
      FIO_ASSERT(data, "Memory allocation failed");
      FIO_MEMSET(data, 'A', len);

      fio_blake2b(out_b, 64, data, len, NULL, 0);
      fio_blake2s(out_s, 32, data, len, NULL, 0);

      int zero_b = 1, zero_s = 1;
      for (int j = 0; j < 64; ++j)
        if (out_b[j] != 0)
          zero_b = 0;
      for (int j = 0; j < 32; ++j)
        if (out_s[j] != 0)
          zero_s = 0;

      FIO_ASSERT(!zero_b, "BLAKE2b at %zu bytes should not be all zeros", len);
      FIO_ASSERT(!zero_s, "BLAKE2s at %zu bytes should not be all zeros", len);

      free(data);
    }
  }

  /* Test: Variable output lengths */
  {
    FIO_LOG_DDEBUG("  Testing variable output lengths...");
    uint8_t data[] = "test input";
    size_t data_len = sizeof(data) - 1;

    /* BLAKE2b: 1 to 64 bytes output */
    size_t b_lens[] = {1, 16, 32, 48, 64};
    for (size_t i = 0; i < sizeof(b_lens) / sizeof(b_lens[0]); ++i) {
      size_t out_len = b_lens[i];
      uint8_t out[64];
      FIO_MEMSET(out, 0, 64);

      fio_blake2b(out, out_len, data, data_len, NULL, 0);

      int zero = 1;
      for (size_t j = 0; j < out_len; ++j)
        if (out[j] != 0)
          zero = 0;
      FIO_ASSERT(!zero,
                 "BLAKE2b %zu-byte output should not be all zeros",
                 out_len);
    }

    /* BLAKE2s: 1 to 32 bytes output */
    size_t s_lens[] = {1, 8, 16, 24, 32};
    for (size_t i = 0; i < sizeof(s_lens) / sizeof(s_lens[0]); ++i) {
      size_t out_len = s_lens[i];
      uint8_t out[32];
      FIO_MEMSET(out, 0, 32);

      fio_blake2s(out, out_len, data, data_len, NULL, 0);

      int zero = 1;
      for (size_t j = 0; j < out_len; ++j)
        if (out[j] != 0)
          zero = 0;
      FIO_ASSERT(!zero,
                 "BLAKE2s %zu-byte output should not be all zeros",
                 out_len);
    }
  }

  /* Test: Keyed hashing with various key lengths */
  {
    FIO_LOG_DDEBUG("  Testing keyed hashing with various key lengths...");
    uint8_t data[] = "test message";
    size_t data_len = sizeof(data) - 1;

    /* BLAKE2b: key up to 64 bytes */
    size_t b_key_lens[] = {1, 16, 32, 64};
    for (size_t i = 0; i < sizeof(b_key_lens) / sizeof(b_key_lens[0]); ++i) {
      size_t key_len = b_key_lens[i];
      uint8_t key[64];
      uint8_t out[64];
      FIO_MEMSET(key, (int)i, key_len);

      fio_blake2b(out, 64, data, data_len, key, key_len);

      int zero = 1;
      for (int j = 0; j < 64; ++j)
        if (out[j] != 0)
          zero = 0;
      FIO_ASSERT(!zero,
                 "BLAKE2b with %zu-byte key should not be all zeros",
                 key_len);
    }

    /* BLAKE2s: key up to 32 bytes */
    size_t s_key_lens[] = {1, 8, 16, 32};
    for (size_t i = 0; i < sizeof(s_key_lens) / sizeof(s_key_lens[0]); ++i) {
      size_t key_len = s_key_lens[i];
      uint8_t key[32];
      uint8_t out[32];
      FIO_MEMSET(key, (int)i, key_len);

      fio_blake2s(out, 32, data, data_len, key, key_len);

      int zero = 1;
      for (int j = 0; j < 32; ++j)
        if (out[j] != 0)
          zero = 0;
      FIO_ASSERT(!zero,
                 "BLAKE2s with %zu-byte key should not be all zeros",
                 key_len);
    }
  }

  /* Test: Empty key (should work like unkeyed) */
  {
    FIO_LOG_DDEBUG("  Testing empty key...");
    uint8_t data[] = "test";
    uint8_t out1[64], out2[64];

    fio_blake2b(out1, 64, data, 4, NULL, 0);
    fio_blake2b(out2, 64, data, 4, (uint8_t *)"", 0);

    FIO_ASSERT(!FIO_MEMCMP(out1, out2, 64), "Empty key should equal no key");
  }

  /* Test: Large input (1MB) */
  {
    FIO_LOG_DDEBUG("  Testing large input (1MB)...");
    size_t len = 1024 * 1024;
    uint8_t *data = (uint8_t *)malloc(len);
    uint8_t out[64];
    FIO_ASSERT(data, "Memory allocation failed");

    for (size_t i = 0; i < len; ++i)
      data[i] = (uint8_t)(i & 0xFF);

    fio_blake2b(out, 64, data, len, NULL, 0);

    int zero = 1;
    for (int i = 0; i < 64; ++i)
      if (out[i] != 0)
        zero = 0;
    FIO_ASSERT(!zero, "BLAKE2b 1MB should not be all zeros");

    /* Verify determinism */
    uint8_t out2[64];
    fio_blake2b(out2, 64, data, len, NULL, 0);
    FIO_ASSERT(!FIO_MEMCMP(out, out2, 64), "BLAKE2b should be deterministic");

    free(data);
  }

  /* Test: Incremental with various chunk sizes */
  {
    FIO_LOG_DDEBUG("  Testing incremental with various chunk sizes...");
    uint8_t data[1000];
    for (size_t i = 0; i < 1000; ++i)
      data[i] = (uint8_t)(i & 0xFF);

    uint8_t out1[64], out2[64];

    /* One-shot */
    fio_blake2b(out1, 64, data, 1000, NULL, 0);

    /* Incremental - single byte at a time */
    fio_blake2b_s h = fio_blake2b_init(64, NULL, 0);
    for (size_t i = 0; i < 1000; ++i) {
      fio_blake2b_consume(&h, data + i, 1);
    }
    fio_blake2b_finalize(&h, out2);

    FIO_ASSERT(!FIO_MEMCMP(out1, out2, 64),
               "BLAKE2b incremental (1 byte) mismatch");
  }

  /* Test: All zeros input */
  {
    FIO_LOG_DDEBUG("  Testing all-zeros input...");
    uint8_t data[64] = {0};
    uint8_t out[64];

    fio_blake2b(out, 64, data, 64, NULL, 0);

    int zero = 1;
    for (int i = 0; i < 64; ++i)
      if (out[i] != 0)
        zero = 0;
    FIO_ASSERT(!zero, "BLAKE2b of zeros should not be all zeros");
  }

  /* Test: All ones input */
  {
    FIO_LOG_DDEBUG("  Testing all-ones input...");
    uint8_t data[64];
    uint8_t out[64];
    FIO_MEMSET(data, 0xFF, 64);

    fio_blake2b(out, 64, data, 64, NULL, 0);

    int zero = 1;
    for (int i = 0; i < 64; ++i)
      if (out[i] != 0)
        zero = 0;
    FIO_ASSERT(!zero, "BLAKE2b of ones should not be all zeros");
  }

  /* Test: Different keys produce different outputs */
  {
    FIO_LOG_DDEBUG("  Testing different keys produce different outputs...");
    uint8_t data[] = "test";
    uint8_t key1[32] = {0};
    uint8_t key2[32] = {0};
    key2[0] = 1;

    uint8_t out1[64], out2[64];
    fio_blake2b(out1, 64, data, 4, key1, 32);
    fio_blake2b(out2, 64, data, 4, key2, 32);

    FIO_ASSERT(FIO_MEMCMP(out1, out2, 64) != 0,
               "Different keys should produce different outputs");
  }

  /* Test: Different output lengths produce different prefixes */
  {
    FIO_LOG_DDEBUG("  Testing different output lengths...");
    uint8_t data[] = "test";
    uint8_t out32[32], out64[64];

    fio_blake2b(out32, 32, data, 4, NULL, 0);
    fio_blake2b(out64, 64, data, 4, NULL, 0);

    /* The 32-byte output should NOT be a prefix of the 64-byte output */
    /* (BLAKE2 encodes output length in the hash) */
    FIO_ASSERT(FIO_MEMCMP(out32, out64, 32) != 0,
               "Different output lengths should produce different hashes");
  }

  FIO_LOG_DDEBUG("BLAKE2 edge case tests passed!");
}

int main(void) {
  FIO_LOG_DDEBUG("Testing BLAKE2 implementation...");

  /* BLAKE2b tests */
  FIO_LOG_DDEBUG("=== BLAKE2b Tests ===");
  fio___test_blake2b_empty();
  fio___test_blake2b_abc();
  fio___test_blake2b_keyed();
  fio___test_blake2b_streaming();

  /* BLAKE2s tests */
  FIO_LOG_DDEBUG("=== BLAKE2s Tests ===");
  fio___test_blake2s_empty();
  fio___test_blake2s_abc();
  fio___test_blake2s_streaming();

  /* Edge case tests */
  FIO_LOG_DDEBUG("=== Edge Case Tests ===");
  fio___test_blake2_edge_cases();

  FIO_LOG_DDEBUG("All BLAKE2 tests passed!");
  return 0;
}
