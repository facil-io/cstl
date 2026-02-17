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
  fio_blake2b_hash(out, 64, NULL, 0, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64),
             "BLAKE2b-512 empty string test failed");
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
  fio_blake2b_hash(out, 64, "abc", 3, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64), "BLAKE2b-512 'abc' test failed");
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
  fio_blake2b_hash(out, 64, msg, 255, key, 64);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64), "BLAKE2b-512 keyed test failed");
}

static void fio___test_blake2b_streaming(void) {
  /* Test streaming API produces same result as one-shot */
  const char *msg = "The quick brown fox jumps over the lazy dog";
  size_t len = strlen(msg);

  uint8_t out1[64], out2[64];

  /* One-shot */
  fio_blake2b_hash(out1, 64, msg, len, NULL, 0);

  /* Streaming - feed one byte at a time */
  fio_blake2b_s h = fio_blake2b_init(64, NULL, 0);
  for (size_t i = 0; i < len; ++i)
    fio_blake2b_consume(&h, msg + i, 1);
  {
    fio_u512 result = fio_blake2b_finalize(&h);
    FIO_MEMCPY(out2, result.u8, 64);
  }

  FIO_ASSERT(!FIO_MEMCMP(out1, out2, 64), "BLAKE2b streaming test failed");
}

/* BLAKE2s test vectors */
static void fio___test_blake2s_empty(void) {
  /* BLAKE2s-256("") */
  static const uint8_t expected[32] = {
      0x69, 0x21, 0x7a, 0x30, 0x79, 0x90, 0x80, 0x94, 0xe1, 0x11, 0x21,
      0xd0, 0x42, 0x35, 0x4a, 0x7c, 0x1f, 0x55, 0xb6, 0x48, 0x2c, 0xa1,
      0xa5, 0x1e, 0x1b, 0x25, 0x0d, 0xfd, 0x1e, 0xd0, 0xee, 0xf9};
  uint8_t out[32];
  fio_blake2s_hash(out, 32, NULL, 0, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 32),
             "BLAKE2s-256 empty string test failed");
}

static void fio___test_blake2s_abc(void) {
  /* BLAKE2s-256("abc") */
  static const uint8_t expected[32] = {
      0x50, 0x8c, 0x5e, 0x8c, 0x32, 0x7c, 0x14, 0xe2, 0xe1, 0xa7, 0x2b,
      0xa3, 0x4e, 0xeb, 0x45, 0x2f, 0x37, 0x45, 0x8b, 0x20, 0x9e, 0xd6,
      0x3a, 0x29, 0x4d, 0x99, 0x9b, 0x4c, 0x86, 0x67, 0x59, 0x82};
  uint8_t out[32];
  fio_blake2s_hash(out, 32, "abc", 3, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 32), "BLAKE2s-256 'abc' test failed");
}

static void fio___test_blake2s_streaming(void) {
  /* Test streaming API produces same result as one-shot */
  const char *msg = "The quick brown fox jumps over the lazy dog";
  size_t len = strlen(msg);

  uint8_t out1[32], out2[32];

  /* One-shot */
  fio_blake2s_hash(out1, 32, msg, len, NULL, 0);

  /* Streaming - feed one byte at a time */
  fio_blake2s_s h = fio_blake2s_init(32, NULL, 0);
  for (size_t i = 0; i < len; ++i)
    fio_blake2s_consume(&h, msg + i, 1);
  {
    fio_u256 result = fio_blake2s_finalize(&h);
    FIO_MEMCPY(out2, result.u8, 32);
  }

  FIO_ASSERT(!FIO_MEMCMP(out1, out2, 32), "BLAKE2s streaming test failed");
}

/* Edge case tests */
static void fio___test_blake2_edge_cases(void) {
  /* Test: Single byte input */
  {
    uint8_t data[1] = {0x00};
    uint8_t out_b[64], out_s[32];

    fio_blake2b_hash(out_b, 64, data, 1, NULL, 0);
    fio_blake2s_hash(out_s, 32, data, 1, NULL, 0);

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
    size_t test_sizes[] = {63, 64, 65, 127, 128, 129};

    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); ++i) {
      size_t len = test_sizes[i];
      uint8_t *data = (uint8_t *)malloc(len);
      uint8_t out_b[64], out_s[32];
      FIO_ASSERT(data, "Memory allocation failed");
      FIO_MEMSET(data, 'A', len);

      fio_blake2b_hash(out_b, 64, data, len, NULL, 0);
      fio_blake2s_hash(out_s, 32, data, len, NULL, 0);

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
    uint8_t data[] = "test input";
    size_t data_len = sizeof(data) - 1;

    /* BLAKE2b: 1 to 64 bytes output */
    size_t b_lens[] = {1, 16, 32, 48, 64};
    for (size_t i = 0; i < sizeof(b_lens) / sizeof(b_lens[0]); ++i) {
      size_t out_len = b_lens[i];
      uint8_t out[64];
      FIO_MEMSET(out, 0, 64);

      fio_blake2b_hash(out, out_len, data, data_len, NULL, 0);

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

      fio_blake2s_hash(out, out_len, data, data_len, NULL, 0);

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
    uint8_t data[] = "test message";
    size_t data_len = sizeof(data) - 1;

    /* BLAKE2b: key up to 64 bytes */
    size_t b_key_lens[] = {1, 16, 32, 64};
    for (size_t i = 0; i < sizeof(b_key_lens) / sizeof(b_key_lens[0]); ++i) {
      size_t key_len = b_key_lens[i];
      uint8_t key[64];
      uint8_t out[64];
      FIO_MEMSET(key, (int)i, key_len);

      fio_blake2b_hash(out, 64, data, data_len, key, key_len);

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

      fio_blake2s_hash(out, 32, data, data_len, key, key_len);

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
    uint8_t data[] = "test";
    uint8_t out1[64], out2[64];

    fio_blake2b_hash(out1, 64, data, 4, NULL, 0);
    fio_blake2b_hash(out2, 64, data, 4, (uint8_t *)"", 0);

    FIO_ASSERT(!FIO_MEMCMP(out1, out2, 64), "Empty key should equal no key");
  }

  /* Test: Large input (1MB) */
  {
    size_t len = 1024 * 1024;
    uint8_t *data = (uint8_t *)malloc(len);
    uint8_t out[64];
    FIO_ASSERT(data, "Memory allocation failed");

    for (size_t i = 0; i < len; ++i)
      data[i] = (uint8_t)(i & 0xFF);

    fio_blake2b_hash(out, 64, data, len, NULL, 0);

    int zero = 1;
    for (int i = 0; i < 64; ++i)
      if (out[i] != 0)
        zero = 0;
    FIO_ASSERT(!zero, "BLAKE2b 1MB should not be all zeros");

    /* Verify determinism */
    uint8_t out2[64];
    fio_blake2b_hash(out2, 64, data, len, NULL, 0);
    FIO_ASSERT(!FIO_MEMCMP(out, out2, 64), "BLAKE2b should be deterministic");

    free(data);
  }

  /* Test: Incremental with various chunk sizes */
  {
    uint8_t data[1000];
    for (size_t i = 0; i < 1000; ++i)
      data[i] = (uint8_t)(i & 0xFF);

    uint8_t out1[64], out2[64];

    /* One-shot */
    fio_blake2b_hash(out1, 64, data, 1000, NULL, 0);

    /* Incremental - single byte at a time */
    fio_blake2b_s h = fio_blake2b_init(64, NULL, 0);
    for (size_t i = 0; i < 1000; ++i) {
      fio_blake2b_consume(&h, data + i, 1);
    }
    {
      fio_u512 result = fio_blake2b_finalize(&h);
      FIO_MEMCPY(out2, result.u8, 64);
    }

    FIO_ASSERT(!FIO_MEMCMP(out1, out2, 64),
               "BLAKE2b incremental (1 byte) mismatch");
  }

  /* Test: All zeros input */
  {
    uint8_t data[64] = {0};
    uint8_t out[64];

    fio_blake2b_hash(out, 64, data, 64, NULL, 0);

    int zero = 1;
    for (int i = 0; i < 64; ++i)
      if (out[i] != 0)
        zero = 0;
    FIO_ASSERT(!zero, "BLAKE2b of zeros should not be all zeros");
  }

  /* Test: All ones input */
  {
    uint8_t data[64];
    uint8_t out[64];
    FIO_MEMSET(data, 0xFF, 64);

    fio_blake2b_hash(out, 64, data, 64, NULL, 0);

    int zero = 1;
    for (int i = 0; i < 64; ++i)
      if (out[i] != 0)
        zero = 0;
    FIO_ASSERT(!zero, "BLAKE2b of ones should not be all zeros");
  }

  /* Test: Different keys produce different outputs */
  {
    uint8_t data[] = "test";
    uint8_t key1[32] = {0};
    uint8_t key2[32] = {0};
    key2[0] = 1;

    uint8_t out1[64], out2[64];
    fio_blake2b_hash(out1, 64, data, 4, key1, 32);
    fio_blake2b_hash(out2, 64, data, 4, key2, 32);

    FIO_ASSERT(FIO_MEMCMP(out1, out2, 64) != 0,
               "Different keys should produce different outputs");
  }

  /* Test: Different output lengths produce different prefixes */
  {
    uint8_t data[] = "test";
    uint8_t out32[32], out64[64];

    fio_blake2b_hash(out32, 32, data, 4, NULL, 0);
    fio_blake2b_hash(out64, 64, data, 4, NULL, 0);

    /* The 32-byte output should NOT be a prefix of the 64-byte output */
    /* (BLAKE2 encodes output length in the hash) */
    FIO_ASSERT(FIO_MEMCMP(out32, out64, 32) != 0,
               "Different output lengths should produce different hashes");
  }
}

/* *****************************************************************************
Official BLAKE2 Known-Answer Test (KAT) Suite
Source: https://github.com/BLAKE2/BLAKE2/blob/master/testvectors/
Pattern: key = {0,1,...,63} for b / {0,1,...,31} for s
         message[i] = {0,1,...,i-1} (i bytes for entry i, 0-indexed)
         output = full 64 bytes (b) / 32 bytes (s)
***************************************************************************** */

/* BLAKE2b KAT: first 10 entries (msg len 0..9) + last 5 (msg len 251..255) */
/* Each entry: outlen=64, key={0..63}, msg={0..i-1} */
typedef struct {
  size_t msg_len;
  uint8_t hash[64];
} fio___blake2b_kat_s;

static const fio___blake2b_kat_s fio___blake2b_kat[] = {
    /* entry 0: msg=empty */
    {0, {0x10, 0xeb, 0xb6, 0x77, 0x00, 0xb1, 0x86, 0x8e, 0xfb, 0x44, 0x17,
         0x98, 0x7a, 0xcf, 0x46, 0x90, 0xae, 0x9d, 0x97, 0x2f, 0xb7, 0xa5,
         0x90, 0xc2, 0xf0, 0x28, 0x71, 0x79, 0x9a, 0xaa, 0x47, 0x86, 0xb5,
         0xe9, 0x96, 0xe8, 0xf0, 0xf4, 0xeb, 0x98, 0x1f, 0xc2, 0x14, 0xb0,
         0x05, 0xf4, 0x2d, 0x2f, 0xf4, 0x23, 0x34, 0x99, 0x39, 0x16, 0x53,
         0xdf, 0x7a, 0xef, 0xcb, 0xc1, 0x3f, 0xc5, 0x15, 0x68}},
    /* entry 1: msg={0x00} */
    {1, {0x96, 0x1f, 0x6d, 0xd1, 0xe4, 0xdd, 0x30, 0xf6, 0x39, 0x01, 0x69,
         0x0c, 0x51, 0x2e, 0x78, 0xe4, 0xb4, 0x5e, 0x47, 0x42, 0xed, 0x19,
         0x7c, 0x3c, 0x5e, 0x45, 0xc5, 0x49, 0xfd, 0x25, 0xf2, 0xe4, 0x18,
         0x7b, 0x0b, 0xc9, 0xfe, 0x30, 0x49, 0x2b, 0x16, 0xb0, 0xd0, 0xbc,
         0x4e, 0xf9, 0xb0, 0xf3, 0x4c, 0x70, 0x03, 0xfa, 0xc0, 0x9a, 0x5e,
         0xf1, 0x53, 0x2e, 0x69, 0x43, 0x02, 0x34, 0xce, 0xbd}},
    /* entry 2: msg={0x00,0x01} */
    {2, {0xda, 0x2c, 0xfb, 0xe2, 0xd8, 0x40, 0x9a, 0x0f, 0x38, 0x02, 0x61,
         0x13, 0x88, 0x4f, 0x84, 0xb5, 0x01, 0x56, 0x37, 0x1a, 0xe3, 0x04,
         0xc4, 0x43, 0x01, 0x73, 0xd0, 0x8a, 0x99, 0xd9, 0xfb, 0x1b, 0x98,
         0x31, 0x64, 0xa3, 0x77, 0x07, 0x06, 0xd5, 0x37, 0xf4, 0x9e, 0x0c,
         0x91, 0x6d, 0x9f, 0x32, 0xb9, 0x5c, 0xc3, 0x7a, 0x95, 0xb9, 0x9d,
         0x85, 0x74, 0x36, 0xf0, 0x23, 0x2c, 0x88, 0xa9, 0x65}},
    /* entry 3: msg={0x00,0x01,0x02} */
    {3, {0x33, 0xd0, 0x82, 0x5d, 0xdd, 0xf7, 0xad, 0xa9, 0x9b, 0x0e, 0x7e,
         0x30, 0x71, 0x04, 0xad, 0x07, 0xca, 0x9c, 0xfd, 0x96, 0x92, 0x21,
         0x4f, 0x15, 0x61, 0x35, 0x63, 0x15, 0xe7, 0x84, 0xf3, 0xe5, 0xa1,
         0x7e, 0x36, 0x4a, 0xe9, 0xdb, 0xb1, 0x4c, 0xb2, 0x03, 0x6d, 0xf9,
         0x32, 0xb7, 0x7f, 0x4b, 0x29, 0x27, 0x61, 0x36, 0x5f, 0xb3, 0x28,
         0xde, 0x7a, 0xfd, 0xc6, 0xd8, 0x99, 0x8f, 0x5f, 0xc1}},
    /* entry 4: msg={0x00..0x03} */
    {4, {0xbe, 0xaa, 0x5a, 0x3d, 0x08, 0xf3, 0x80, 0x71, 0x43, 0xcf, 0x62,
         0x1d, 0x95, 0xcd, 0x69, 0x05, 0x14, 0xd0, 0xb4, 0x9e, 0xff, 0xf9,
         0xc9, 0x1d, 0x24, 0xb5, 0x92, 0x41, 0xec, 0x0e, 0xef, 0xa5, 0xf6,
         0x01, 0x96, 0xd4, 0x07, 0x04, 0x8b, 0xba, 0x8d, 0x21, 0x46, 0x82,
         0x8e, 0xbc, 0xb0, 0x48, 0x8d, 0x88, 0x42, 0xfd, 0x56, 0xbb, 0x4f,
         0x6d, 0xf8, 0xe1, 0x9c, 0x4b, 0x4d, 0xaa, 0xb8, 0xac}},
    /* entry 5: msg={0x00..0x04} */
    {5, {0x09, 0x80, 0x84, 0xb5, 0x1f, 0xd1, 0x3d, 0xea, 0xe5, 0xf4, 0x32,
         0x0d, 0xe9, 0x4a, 0x68, 0x8e, 0xe0, 0x7b, 0xae, 0xa2, 0x80, 0x04,
         0x86, 0x68, 0x9a, 0x86, 0x36, 0x11, 0x7b, 0x46, 0xc1, 0xf4, 0xc1,
         0xf6, 0xaf, 0x7f, 0x74, 0xae, 0x7c, 0x85, 0x76, 0x00, 0x45, 0x6a,
         0x58, 0xa3, 0xaf, 0x25, 0x1d, 0xc4, 0x72, 0x3a, 0x64, 0xcc, 0x7c,
         0x0a, 0x5a, 0xb6, 0xd9, 0xca, 0xc9, 0x1c, 0x20, 0xbb}},
    /* entry 6: msg={0x00..0x05} */
    {6, {0x60, 0x44, 0x54, 0x0d, 0x56, 0x08, 0x53, 0xeb, 0x1c, 0x57, 0xdf,
         0x00, 0x77, 0xdd, 0x38, 0x10, 0x94, 0x78, 0x1c, 0xdb, 0x90, 0x73,
         0xe5, 0xb1, 0xb3, 0xd3, 0xf6, 0xc7, 0x82, 0x9e, 0x12, 0x06, 0x6b,
         0xba, 0xca, 0x96, 0xd9, 0x89, 0xa6, 0x90, 0xde, 0x72, 0xca, 0x31,
         0x33, 0xa8, 0x36, 0x52, 0xba, 0x28, 0x4a, 0x6d, 0x62, 0x94, 0x2b,
         0x27, 0x1f, 0xfa, 0x26, 0x20, 0xc9, 0xe7, 0x5b, 0x1f}},
    /* entry 7: msg={0x00..0x06} */
    /* hash: 7a8cfe9b90f75f7ecb3acc053aaed6193112b6f6a4aeeb3f65d3de541942deb9
             e2228152a3c4bbbe72fc3b12629528cfbb09fe630f0474339f54abf453e2ed52 */
    {7, {0x7a, 0x8c, 0xfe, 0x9b, 0x90, 0xf7, 0x5f, 0x7e, 0xcb, 0x3a, 0xcc,
         0x05, 0x3a, 0xae, 0xd6, 0x19, 0x31, 0x12, 0xb6, 0xf6, 0xa4, 0xae,
         0xeb, 0x3f, 0x65, 0xd3, 0xde, 0x54, 0x19, 0x42, 0xde, 0xb9, 0xe2,
         0x22, 0x81, 0x52, 0xa3, 0xc4, 0xbb, 0xbe, 0x72, 0xfc, 0x3b, 0x12,
         0x62, 0x95, 0x28, 0xcf, 0xbb, 0x09, 0xfe, 0x63, 0x0f, 0x04, 0x74,
         0x33, 0x9f, 0x54, 0xab, 0xf4, 0x53, 0xe2, 0xed, 0x52}},
    /* entry 8: msg={0x00..0x07} */
    {8, {0x38, 0x0b, 0xea, 0xf6, 0xea, 0x7c, 0xc9, 0x36, 0x5e, 0x27, 0x0e,
         0xf0, 0xe6, 0xf3, 0xa6, 0x4f, 0xb9, 0x02, 0xac, 0xae, 0x51, 0xdd,
         0x55, 0x12, 0xf8, 0x42, 0x59, 0xad, 0x2c, 0x91, 0xf4, 0xbc, 0x41,
         0x08, 0xdb, 0x73, 0x19, 0x2a, 0x5b, 0xbf, 0xb0, 0xcb, 0xcf, 0x71,
         0xe4, 0x6c, 0x3e, 0x21, 0xae, 0xe1, 0xc5, 0xe8, 0x60, 0xdc, 0x96,
         0xe8, 0xeb, 0x0b, 0x7b, 0x84, 0x26, 0xe6, 0xab, 0xe9}},
    /* entry 9: msg={0x00..0x08} */
    {9, {0x60, 0xfe, 0x3c, 0x45, 0x35, 0xe1, 0xb5, 0x9d, 0x9a, 0x61, 0xea,
         0x85, 0x00, 0xbf, 0xac, 0x41, 0xa6, 0x9d, 0xff, 0xb1, 0xce, 0xad,
         0xd9, 0xac, 0xa3, 0x23, 0xe9, 0xa6, 0x25, 0xb6, 0x4d, 0xa5, 0x76,
         0x3b, 0xad, 0x72, 0x26, 0xda, 0x02, 0xb9, 0xc8, 0xc4, 0xf1, 0xa5,
         0xde, 0x14, 0x0a, 0xc5, 0xa6, 0xc1, 0x12, 0x4e, 0x4f, 0x71, 0x8c,
         0xe0, 0xb2, 0x8e, 0xa4, 0x73, 0x93, 0xaa, 0x66, 0x37}},
    /* entry 255: msg={0x00..0xfe} (255 bytes) - verified against existing test
     */
    {255, {0x14, 0x27, 0x09, 0xd6, 0x2e, 0x28, 0xfc, 0xcc, 0xd0, 0xaf, 0x97,
           0xfa, 0xd0, 0xf8, 0x46, 0x5b, 0x97, 0x1e, 0x82, 0x20, 0x1d, 0xc5,
           0x10, 0x70, 0xfa, 0xa0, 0x37, 0x2a, 0xa4, 0x3e, 0x92, 0x48, 0x4b,
           0xe1, 0xc1, 0xe7, 0x3b, 0xa1, 0x09, 0x06, 0xd5, 0xd1, 0x85, 0x3d,
           0xb6, 0xa4, 0x10, 0x6e, 0x0a, 0x7b, 0xf9, 0x80, 0x0d, 0x37, 0x3d,
           0x6d, 0xee, 0x2d, 0x46, 0xd6, 0x2e, 0xf2, 0xa4, 0x61}},
};

/* BLAKE2s KAT: first 10 entries (msg len 0..9) + last 5 (msg len 251..255) */
/* Each entry: outlen=32, key={0..31}, msg={0..i-1} */
typedef struct {
  size_t msg_len;
  uint8_t hash[32];
} fio___blake2s_kat_s;

static const fio___blake2s_kat_s fio___blake2s_kat[] = {
    /* entry 0: msg=empty */
    {0, {0x48, 0xa8, 0x99, 0x7d, 0xa4, 0x07, 0x87, 0x6b, 0x3d, 0x79, 0xc0,
         0xd9, 0x23, 0x25, 0xad, 0x3b, 0x89, 0xcb, 0xb7, 0x54, 0xd8, 0x6a,
         0xb7, 0x1a, 0xee, 0x04, 0x7a, 0xd3, 0x45, 0xfd, 0x2c, 0x49}},
    /* entry 1: msg={0x00} */
    {1, {0x40, 0xd1, 0x5f, 0xee, 0x7c, 0x32, 0x88, 0x30, 0x16, 0x6a, 0xc3,
         0xf9, 0x18, 0x65, 0x0f, 0x80, 0x7e, 0x7e, 0x01, 0xe1, 0x77, 0x25,
         0x8c, 0xdc, 0x0a, 0x39, 0xb1, 0x1f, 0x59, 0x80, 0x66, 0xf1}},
    /* entry 2: msg={0x00,0x01} */
    {2, {0x6b, 0xb7, 0x13, 0x00, 0x64, 0x4c, 0xd3, 0x99, 0x1b, 0x26, 0xcc,
         0xd4, 0xd2, 0x74, 0xac, 0xd1, 0xad, 0xea, 0xb8, 0xb1, 0xd7, 0x91,
         0x45, 0x46, 0xc1, 0x19, 0x8b, 0xbe, 0x9f, 0xc9, 0xd8, 0x03}},
    /* entry 3: msg={0x00,0x01,0x02} */
    {3, {0x1d, 0x22, 0x0d, 0xbe, 0x2e, 0xe1, 0x34, 0x66, 0x1f, 0xdf, 0x6d,
         0x9e, 0x74, 0xb4, 0x17, 0x04, 0x71, 0x05, 0x56, 0xf2, 0xf6, 0xe5,
         0xa0, 0x91, 0xb2, 0x27, 0x69, 0x74, 0x45, 0xdb, 0xea, 0x6b}},
    /* entry 4: msg={0x00..0x03} */
    {4, {0xf6, 0xc3, 0xfb, 0xad, 0xb4, 0xcc, 0x68, 0x7a, 0x00, 0x64, 0xa5,
         0xbe, 0x6e, 0x79, 0x1b, 0xec, 0x63, 0xb8, 0x68, 0xad, 0x62, 0xfb,
         0xa6, 0x1b, 0x37, 0x57, 0xef, 0x9c, 0xa5, 0x2e, 0x05, 0xb2}},
    /* entry 5: msg={0x00..0x04} */
    {5, {0x49, 0xc1, 0xf2, 0x11, 0x88, 0xdf, 0xd7, 0x69, 0xae, 0xa0, 0xe9,
         0x11, 0xdd, 0x6b, 0x41, 0xf1, 0x4d, 0xab, 0x10, 0x9d, 0x2b, 0x85,
         0x97, 0x7a, 0xa3, 0x08, 0x8b, 0x5c, 0x70, 0x7e, 0x85, 0x98}},
    /* entry 6: msg={0x00..0x05} */
    {6, {0xfd, 0xd8, 0x99, 0x3d, 0xcd, 0x43, 0xf6, 0x96, 0xd4, 0x4f, 0x3c,
         0xea, 0x0f, 0xf3, 0x53, 0x45, 0x23, 0x4e, 0xc8, 0xee, 0x08, 0x3e,
         0xb3, 0xca, 0xda, 0x01, 0x7c, 0x7f, 0x78, 0xc1, 0x71, 0x43}},
    /* entry 7: msg={0x00..0x06} */
    {7, {0xe6, 0xc8, 0x12, 0x56, 0x37, 0x43, 0x8d, 0x09, 0x05, 0xb7, 0x49,
         0xf4, 0x65, 0x60, 0xac, 0x89, 0xfd, 0x47, 0x1c, 0xf8, 0x69, 0x2e,
         0x28, 0xfa, 0xb9, 0x82, 0xf7, 0x3f, 0x01, 0x9b, 0x83, 0xa9}},
    /* entry 8: msg={0x00..0x07} */
    /* hash: 19fc8ca6979d60e6edd3b4541e2f967ced740df6ec1eaebbfe813832e96b2974 */
    {8, {0x19, 0xfc, 0x8c, 0xa6, 0x97, 0x9d, 0x60, 0xe6, 0xed, 0xd3, 0xb4,
         0x54, 0x1e, 0x2f, 0x96, 0x7c, 0xed, 0x74, 0x0d, 0xf6, 0xec, 0x1e,
         0xae, 0xbb, 0xfe, 0x81, 0x38, 0x32, 0xe9, 0x6b, 0x29, 0x74}},
    /* entry 9: msg={0x00..0x08} */
    {9, {0xa6, 0xad, 0x77, 0x7c, 0xe8, 0x81, 0xb5, 0x2b, 0xb5, 0xa4, 0x42,
         0x1a, 0xb6, 0xcd, 0xd2, 0xdf, 0xba, 0x13, 0xe9, 0x63, 0x65, 0x2d,
         0x4d, 0x6d, 0x12, 0x2a, 0xee, 0x46, 0x54, 0x8c, 0x14, 0xa7}},
    /* entry 251: msg={0x00..0xfa} (251 bytes) */
    /* hash: d12bf3732ef4af5c22fa90356af8fc50fcb40f8f2ea5c8594737a3b3d5abdbd7 */
    {251, {0xd1, 0x2b, 0xf3, 0x73, 0x2e, 0xf4, 0xaf, 0x5c, 0x22, 0xfa, 0x90,
           0x35, 0x6a, 0xf8, 0xfc, 0x50, 0xfc, 0xb4, 0x0f, 0x8f, 0x2e, 0xa5,
           0xc8, 0x59, 0x47, 0x37, 0xa3, 0xb3, 0xd5, 0xab, 0xdb, 0xd7}},
    /* entry 252: msg={0x00..0xfb} (252 bytes) */
    /* hash: 11030b9289bba5af65260672ab6fee88b87420acef4a1789a2073b7ec2f2a09e */
    {252, {0x11, 0x03, 0x0b, 0x92, 0x89, 0xbb, 0xa5, 0xaf, 0x65, 0x26, 0x06,
           0x72, 0xab, 0x6f, 0xee, 0x88, 0xb8, 0x74, 0x20, 0xac, 0xef, 0x4a,
           0x17, 0x89, 0xa2, 0x07, 0x3b, 0x7e, 0xc2, 0xf2, 0xa0, 0x9e}},
    /* entry 253: msg={0x00..0xfc} (253 bytes) */
    /* hash: 69cb192b8444005c8c0ceb12c846860768188cda0aec27a9c8a55cdee2123632 */
    {253, {0x69, 0xcb, 0x19, 0x2b, 0x84, 0x44, 0x00, 0x5c, 0x8c, 0x0c, 0xeb,
           0x12, 0xc8, 0x46, 0x86, 0x07, 0x68, 0x18, 0x8c, 0xda, 0x0a, 0xec,
           0x27, 0xa9, 0xc8, 0xa5, 0x5c, 0xde, 0xe2, 0x12, 0x36, 0x32}},
    /* entry 254: msg={0x00..0xfd} (254 bytes) */
    /* hash: db444c15597b5f1a03d1f9edd16e4a9f43a667cc275175dfa2b704e3bb1a9b83 */
    {254, {0xdb, 0x44, 0x4c, 0x15, 0x59, 0x7b, 0x5f, 0x1a, 0x03, 0xd1, 0xf9,
           0xed, 0xd1, 0x6e, 0x4a, 0x9f, 0x43, 0xa6, 0x67, 0xcc, 0x27, 0x51,
           0x75, 0xdf, 0xa2, 0xb7, 0x04, 0xe3, 0xbb, 0x1a, 0x9b, 0x83}},
    /* entry 255: msg={0x00..0xfe} (255 bytes) */
    /* hash: 3fb735061abc519dfe979e54c1ee5bfad0a9d858b3315bad34bde999efd724dd */
    {255, {0x3f, 0xb7, 0x35, 0x06, 0x1a, 0xbc, 0x51, 0x9d, 0xfe, 0x97, 0x9e,
           0x54, 0xc1, 0xee, 0x5b, 0xfa, 0xd0, 0xa9, 0xd8, 0x58, 0xb3, 0x31,
           0x5b, 0xad, 0x34, 0xbd, 0xe9, 0x99, 0xef, 0xd7, 0x24, 0xdd}},
};

static void fio___test_blake2_official_kat(void) {
  /* BLAKE2b KAT: key={0..63}, outlen=64, msg={0..i-1} */
  uint8_t key_b[64];
  for (size_t i = 0; i < 64; ++i)
    key_b[i] = (uint8_t)i;

  size_t n_b = sizeof(fio___blake2b_kat) / sizeof(fio___blake2b_kat[0]);
  for (size_t i = 0; i < n_b; ++i) {
    size_t msg_len = fio___blake2b_kat[i].msg_len;
    uint8_t *msg = NULL;
    if (msg_len > 0) {
      msg = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, msg_len, 0);
      FIO_ASSERT(msg, "BLAKE2b KAT: allocation failed for entry %zu", i);
      for (size_t j = 0; j < msg_len; ++j)
        msg[j] = (uint8_t)j;
    }

    uint8_t out[64];
    fio_blake2b_hash(out, 64, msg, msg_len, key_b, 64);

    if (FIO_MEMCMP(out, fio___blake2b_kat[i].hash, 64) != 0) {
      FIO_LOG_ERROR("BLAKE2b KAT entry %zu (msg_len=%zu) FAILED", i, msg_len);
      FIO_LOG_ERROR("  expected: ");
      for (size_t k = 0; k < 64; ++k)
        FIO_LOG_ERROR("    [%02zu] expected=%02x got=%02x",
                      k,
                      fio___blake2b_kat[i].hash[k],
                      out[k]);
      FIO_ASSERT(0, "BLAKE2b KAT entry %zu failed", i);
    }
    FIO_LOG_INFO("BLAKE2b KAT entry %zu (msg_len=%zu): PASS", i, msg_len);

    if (msg)
      FIO_MEM_FREE(msg, msg_len);
  }

  /* BLAKE2s KAT: key={0..31}, outlen=32, msg={0..i-1} */
  uint8_t key_s[32];
  for (size_t i = 0; i < 32; ++i)
    key_s[i] = (uint8_t)i;

  size_t n_s = sizeof(fio___blake2s_kat) / sizeof(fio___blake2s_kat[0]);
  for (size_t i = 0; i < n_s; ++i) {
    size_t msg_len = fio___blake2s_kat[i].msg_len;
    uint8_t *msg = NULL;
    if (msg_len > 0) {
      msg = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, msg_len, 0);
      FIO_ASSERT(msg, "BLAKE2s KAT: allocation failed for entry %zu", i);
      for (size_t j = 0; j < msg_len; ++j)
        msg[j] = (uint8_t)j;
    }

    uint8_t out[32];
    fio_blake2s_hash(out, 32, msg, msg_len, key_s, 32);

    if (FIO_MEMCMP(out, fio___blake2s_kat[i].hash, 32) != 0) {
      FIO_LOG_ERROR("BLAKE2s KAT entry %zu (msg_len=%zu) FAILED", i, msg_len);
      for (size_t k = 0; k < 32; ++k)
        FIO_LOG_ERROR("    [%02zu] expected=%02x got=%02x",
                      k,
                      fio___blake2s_kat[i].hash[k],
                      out[k]);
      FIO_ASSERT(0, "BLAKE2s KAT entry %zu failed", i);
    }
    FIO_LOG_INFO("BLAKE2s KAT entry %zu (msg_len=%zu): PASS", i, msg_len);

    if (msg)
      FIO_MEM_FREE(msg, msg_len);
  }
}

/* *****************************************************************************
OpenSSL Cross-Validation (when HAVE_OPENSSL is defined)
***************************************************************************** */
#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#include <openssl/rand.h>

static void fio___test_blake2_openssl_compare(void) {
  static const size_t test_sizes[] =
      {0, 1, 63, 64, 65, 127, 128, 129, 255, 256, 1000, 65536, 1048576};
  size_t n = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t t = 0; t < n; ++t) {
    size_t len = test_sizes[t];

    /* Allocate and fill with random data */
    uint8_t *data = NULL;
    if (len > 0) {
      data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, len, 0);
      FIO_ASSERT(data, "OpenSSL compare: allocation failed for size %zu", len);
      RAND_bytes(data, (int)len);
    }

    /* --- BLAKE2b-512 --- */
    {
      uint8_t fio_out[64];
      fio_blake2b_hash(fio_out, 64, data, len, NULL, 0);

      uint8_t ossl_out[64];
      unsigned int ossl_len = 64;
      EVP_MD_CTX *ctx = EVP_MD_CTX_new();
      FIO_ASSERT(ctx, "EVP_MD_CTX_new failed");
      FIO_ASSERT(EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL),
                 "EVP_DigestInit_ex(blake2b512) failed");
      if (len > 0)
        FIO_ASSERT(EVP_DigestUpdate(ctx, data, len), "EVP_DigestUpdate failed");
      FIO_ASSERT(EVP_DigestFinal_ex(ctx, ossl_out, &ossl_len),
                 "EVP_DigestFinal_ex failed");
      EVP_MD_CTX_free(ctx);

      if (FIO_MEMCMP(fio_out, ossl_out, 64) != 0) {
        FIO_LOG_ERROR("BLAKE2b-512 OpenSSL mismatch at input size %zu", len);
        for (size_t k = 0; k < 64; ++k)
          FIO_LOG_ERROR("  [%02zu] fio=%02x ossl=%02x",
                        k,
                        fio_out[k],
                        ossl_out[k]);
        FIO_ASSERT(0,
                   "BLAKE2b-512 OpenSSL cross-validation FAILED at size %zu",
                   len);
      }
      FIO_LOG_INFO("BLAKE2b-512 OpenSSL cross-validation PASS (size=%zu)", len);
    }

    /* --- BLAKE2s-256 --- */
    {
      uint8_t fio_out[32];
      fio_blake2s_hash(fio_out, 32, data, len, NULL, 0);

      uint8_t ossl_out[32];
      unsigned int ossl_len = 32;
      EVP_MD_CTX *ctx = EVP_MD_CTX_new();
      FIO_ASSERT(ctx, "EVP_MD_CTX_new failed");
      FIO_ASSERT(EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL),
                 "EVP_DigestInit_ex(blake2s256) failed");
      if (len > 0)
        FIO_ASSERT(EVP_DigestUpdate(ctx, data, len), "EVP_DigestUpdate failed");
      FIO_ASSERT(EVP_DigestFinal_ex(ctx, ossl_out, &ossl_len),
                 "EVP_DigestFinal_ex failed");
      EVP_MD_CTX_free(ctx);

      if (FIO_MEMCMP(fio_out, ossl_out, 32) != 0) {
        FIO_LOG_ERROR("BLAKE2s-256 OpenSSL mismatch at input size %zu", len);
        for (size_t k = 0; k < 32; ++k)
          FIO_LOG_ERROR("  [%02zu] fio=%02x ossl=%02x",
                        k,
                        fio_out[k],
                        ossl_out[k]);
        FIO_ASSERT(0,
                   "BLAKE2s-256 OpenSSL cross-validation FAILED at size %zu",
                   len);
      }
      FIO_LOG_INFO("BLAKE2s-256 OpenSSL cross-validation PASS (size=%zu)", len);
    }

    if (data)
      FIO_MEM_FREE(data, len);
  }

  /* --- 1MB deterministic cross-validation --- */
  {
    const size_t MB = 1048576;
    uint8_t *data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, MB, 0);
    FIO_ASSERT(data, "OpenSSL compare: 1MB allocation failed");

    for (size_t i = 0; i < MB; ++i)
      data[i] = (uint8_t)(i * 2654435761UL ^ (i >> 7));

    /* BLAKE2b-512 vs OpenSSL EVP_blake2b512() */
    {
      uint8_t fio_out[64];
      fio_blake2b_hash(fio_out, 64, data, MB, NULL, 0);

      uint8_t ossl_out[64];
      unsigned int ossl_len = 64;
      EVP_MD_CTX *ctx = EVP_MD_CTX_new();
      FIO_ASSERT(ctx, "EVP_MD_CTX_new failed (1MB b2b)");
      FIO_ASSERT(EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL),
                 "EVP_DigestInit_ex(blake2b512) failed (1MB)");
      FIO_ASSERT(EVP_DigestUpdate(ctx, data, MB),
                 "EVP_DigestUpdate failed (1MB b2b)");
      FIO_ASSERT(EVP_DigestFinal_ex(ctx, ossl_out, &ossl_len),
                 "EVP_DigestFinal_ex failed (1MB b2b)");
      EVP_MD_CTX_free(ctx);

      if (FIO_MEMCMP(fio_out, ossl_out, 64) != 0) {
        FIO_LOG_ERROR("BLAKE2b-512 OpenSSL mismatch at 1MB input");
        for (size_t k = 0; k < 64; ++k)
          FIO_LOG_ERROR("  [%02zu] fio=%02x ossl=%02x",
                        k,
                        fio_out[k],
                        ossl_out[k]);
        FIO_ASSERT(0, "BLAKE2b-512 OpenSSL cross-validation FAILED at 1MB");
      }
      FIO_LOG_INFO("BLAKE2b-512 OpenSSL cross-validation PASS (size=1MB)");
    }

    /* BLAKE2s-256 vs OpenSSL EVP_blake2s256() */
    {
      uint8_t fio_out[32];
      fio_blake2s_hash(fio_out, 32, data, MB, NULL, 0);

      uint8_t ossl_out[32];
      unsigned int ossl_len = 32;
      EVP_MD_CTX *ctx = EVP_MD_CTX_new();
      FIO_ASSERT(ctx, "EVP_MD_CTX_new failed (1MB b2s)");
      FIO_ASSERT(EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL),
                 "EVP_DigestInit_ex(blake2s256) failed (1MB)");
      FIO_ASSERT(EVP_DigestUpdate(ctx, data, MB),
                 "EVP_DigestUpdate failed (1MB b2s)");
      FIO_ASSERT(EVP_DigestFinal_ex(ctx, ossl_out, &ossl_len),
                 "EVP_DigestFinal_ex failed (1MB b2s)");
      EVP_MD_CTX_free(ctx);

      if (FIO_MEMCMP(fio_out, ossl_out, 32) != 0) {
        FIO_LOG_ERROR("BLAKE2s-256 OpenSSL mismatch at 1MB input");
        for (size_t k = 0; k < 32; ++k)
          FIO_LOG_ERROR("  [%02zu] fio=%02x ossl=%02x",
                        k,
                        fio_out[k],
                        ossl_out[k]);
        FIO_ASSERT(0, "BLAKE2s-256 OpenSSL cross-validation FAILED at 1MB");
      }
      FIO_LOG_INFO("BLAKE2s-256 OpenSSL cross-validation PASS (size=1MB)");
    }

    FIO_MEM_FREE(data, MB);
  }
}
#endif /* HAVE_OPENSSL */

int main(void) {
  /* BLAKE2b tests */
  fio___test_blake2b_empty();
  fio___test_blake2b_abc();
  fio___test_blake2b_keyed();
  fio___test_blake2b_streaming();

  /* BLAKE2s tests */
  fio___test_blake2s_empty();
  fio___test_blake2s_abc();
  fio___test_blake2s_streaming();

  /* Edge case tests */
  fio___test_blake2_edge_cases();

  /* Official KAT suite */
  fio___test_blake2_official_kat();

#ifdef HAVE_OPENSSL
  /* OpenSSL cross-validation */
  fio___test_blake2_openssl_compare();
#endif

  return 0;
}
