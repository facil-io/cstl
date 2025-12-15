/* *****************************************************************************
BLAKE2 Tests
***************************************************************************** */
#define FIO_LOG
#define FIO_BLAKE2
#include "fio-stl.h"

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
  FIO_LOG_DEBUG("BLAKE2b-512 empty string: PASSED");
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
  FIO_LOG_DEBUG("BLAKE2b-512 'abc': PASSED");
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
  FIO_LOG_DEBUG("BLAKE2b-512 keyed (64-byte key, 255-byte msg): PASSED");
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
  FIO_LOG_DEBUG("BLAKE2b-512 streaming: PASSED");
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
  FIO_LOG_DEBUG("BLAKE2s-256 empty string: PASSED");
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
  FIO_LOG_DEBUG("BLAKE2s-256 'abc': PASSED");
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
  FIO_LOG_DEBUG("BLAKE2s-256 streaming: PASSED");
}

int main(void) {
  FIO_LOG_INFO("Testing BLAKE2 implementation...\n");

  /* BLAKE2b tests */
  FIO_LOG_INFO("=== BLAKE2b Tests ===");
  fio___test_blake2b_empty();
  fio___test_blake2b_abc();
  fio___test_blake2b_keyed();
  fio___test_blake2b_streaming();

  /* BLAKE2s tests */
  FIO_LOG_INFO("=== BLAKE2s Tests ===");
  fio___test_blake2s_empty();
  fio___test_blake2s_abc();
  fio___test_blake2s_streaming();

  FIO_LOG_INFO("\nAll BLAKE2 tests passed!");
  return 0;
}
