/* *****************************************************************************
Test for 152 sha3.h

Coverage: SHA3-224/256/384/512 and SHAKE128/256 one-shot APIs, NIST FIPS 202
known-answer vectors, streaming (init/consume/finalize) consistency, SHAKE
variable-output and multi-squeeze behavior, and edge cases. Performance loops
are intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SHA3
#include FIO_INCLUDE_FILE

#include <string.h>

/* *****************************************************************************
SHA3-224 Known-Answer Vectors
***************************************************************************** */

static void fio___test_sha3_224_kat(void) {
  static const uint8_t empty[28] = {
      0x6b, 0x4e, 0x03, 0x42, 0x36, 0x67, 0xdb, 0xb7, 0x3b, 0x6e, 0x15,
      0x45, 0x4f, 0x0e, 0xb1, 0xab, 0xd4, 0x59, 0x7f, 0x9a, 0x1b, 0x07,
      0x8e, 0x3f, 0x5b, 0x5a, 0x6b, 0xc7};
  static const uint8_t abc[28] = {
      0xe6, 0x42, 0x82, 0x4c, 0x3f, 0x8c, 0xf2, 0x4a, 0xd0, 0x92, 0x34,
      0xee, 0x7d, 0x3c, 0x76, 0x6f, 0xc9, 0xa3, 0xa5, 0x16, 0x8d, 0x0c,
      0x94, 0xad, 0x73, 0xb4, 0x6f, 0xdf};

  uint8_t out[28];
  fio_sha3_224(out, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, empty, 28), "SHA3-224 empty vector mismatch");

  fio_sha3_224(out, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, abc, 28), "SHA3-224 \"abc\" vector mismatch");
}

/* *****************************************************************************
SHA3-256 Known-Answer Vectors
***************************************************************************** */

static void fio___test_sha3_256_kat(void) {
  static const uint8_t empty[32] = {
      0xa7, 0xff, 0xc6, 0xf8, 0xbf, 0x1e, 0xd7, 0x66, 0x51, 0xc1, 0x47,
      0x56, 0xa0, 0x61, 0xd6, 0x62, 0xf5, 0x80, 0xff, 0x4d, 0xe4, 0x3b,
      0x49, 0xfa, 0x82, 0xd8, 0x0a, 0x4b, 0x80, 0xf8, 0x43, 0x4a};
  static const uint8_t abc[32] = {
      0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2, 0x04, 0x5c, 0x17,
      0x2d, 0x6b, 0xd3, 0x90, 0xbd, 0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d,
      0x52, 0x5b, 0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32};

  uint8_t out[32];
  fio_sha3_256(out, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, empty, 32), "SHA3-256 empty vector mismatch");

  fio_sha3_256(out, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, abc, 32), "SHA3-256 \"abc\" vector mismatch");
}

/* *****************************************************************************
SHA3-384 Known-Answer Vectors
***************************************************************************** */

static void fio___test_sha3_384_kat(void) {
  static const uint8_t abc[48] = {
      0xec, 0x01, 0x49, 0x82, 0x88, 0x51, 0x6f, 0xc9, 0x26, 0x45, 0x9f,
      0x58, 0xe2, 0xc6, 0xad, 0x8d, 0xf9, 0xb4, 0x73, 0xcb, 0x0f, 0xc0,
      0x8c, 0x25, 0x96, 0xda, 0x7c, 0xf0, 0xe4, 0x9b, 0xe4, 0xb2, 0x98,
      0xd8, 0x8c, 0xea, 0x92, 0x7a, 0xc7, 0xf5, 0x39, 0xf1, 0xed, 0xf2,
      0x28, 0x37, 0x6d, 0x25};

  uint8_t out[48];
  fio_sha3_384(out, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, abc, 48), "SHA3-384 \"abc\" vector mismatch");
}

/* *****************************************************************************
SHA3-512 Known-Answer Vectors
***************************************************************************** */

static void fio___test_sha3_512_kat(void) {
  static const uint8_t empty[64] = {
      0xa6, 0x9f, 0x73, 0xcc, 0xa2, 0x3a, 0x9a, 0xc5, 0xc8, 0xb5, 0x67,
      0xdc, 0x18, 0x5a, 0x75, 0x6e, 0x97, 0xc9, 0x82, 0x16, 0x4f, 0xe2,
      0x58, 0x59, 0xe0, 0xd1, 0xdc, 0xc1, 0x47, 0x5c, 0x80, 0xa6, 0x15,
      0xb2, 0x12, 0x3a, 0xf1, 0xf5, 0xf9, 0x4c, 0x11, 0xe3, 0xe9, 0x40,
      0x2c, 0x3a, 0xc5, 0x58, 0xf5, 0x00, 0x19, 0x9d, 0x95, 0xb6, 0xd3,
      0xe3, 0x01, 0x75, 0x85, 0x86, 0x28, 0x1d, 0xcd, 0x26};
  static const uint8_t abc[64] = {
      0xb7, 0x51, 0x85, 0x0b, 0x1a, 0x57, 0x16, 0x8a, 0x56, 0x93, 0xcd,
      0x92, 0x4b, 0x6b, 0x09, 0x6e, 0x08, 0xf6, 0x21, 0x82, 0x74, 0x44,
      0xf7, 0x0d, 0x88, 0x4f, 0x5d, 0x02, 0x40, 0xd2, 0x71, 0x2e, 0x10,
      0xe1, 0x16, 0xe9, 0x19, 0x2a, 0xf3, 0xc9, 0x1a, 0x7e, 0xc5, 0x76,
      0x47, 0xe3, 0x93, 0x40, 0x57, 0x34, 0x0b, 0x4c, 0xf4, 0x08, 0xd5,
      0xa5, 0x65, 0x92, 0xf8, 0x27, 0x4e, 0xec, 0x53, 0xf0};

  uint8_t out[64];
  fio_sha3_512(out, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, empty, 64), "SHA3-512 empty vector mismatch");

  fio_sha3_512(out, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, abc, 64), "SHA3-512 \"abc\" vector mismatch");
}

/* *****************************************************************************
SHAKE Known-Answer Vectors
***************************************************************************** */

static void fio___test_shake_kat(void) {
  static const uint8_t shake128_abc_32[32] = {
      0x58, 0x81, 0x09, 0x2d, 0xd8, 0x18, 0xbf, 0x5c, 0xf8, 0xa3, 0xdd,
      0xb7, 0x93, 0xfb, 0xcb, 0xa7, 0x40, 0x97, 0xd5, 0xc5, 0x26, 0xa6,
      0xd3, 0x5f, 0x97, 0xb8, 0x33, 0x51, 0x94, 0x0f, 0x2c, 0xc8};
  static const uint8_t shake256_abc_64[64] = {
      0x48, 0x33, 0x66, 0x60, 0x13, 0x60, 0xa8, 0x77, 0x1c, 0x68, 0x63,
      0x08, 0x0c, 0xc4, 0x11, 0x4d, 0x8d, 0xb4, 0x45, 0x30, 0xf8, 0xf1,
      0xe1, 0xee, 0x4f, 0x94, 0xea, 0x37, 0xe7, 0x8b, 0x57, 0x39, 0xd5,
      0xa1, 0x5b, 0xef, 0x18, 0x6a, 0x53, 0x86, 0xc7, 0x57, 0x44, 0xc0,
      0x52, 0x7e, 0x1f, 0xaa, 0x9f, 0x87, 0x26, 0xe4, 0x62, 0xa1, 0x2a,
      0x4f, 0xeb, 0x06, 0xbd, 0x88, 0x01, 0xe7, 0x51, 0xe4};

  uint8_t out[64];
  fio_shake128(out, 32, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, shake128_abc_32, 32),
             "SHAKE128 \"abc\" 32-byte vector mismatch");

  fio_shake256(out, 64, "abc", 3);
  FIO_ASSERT(!FIO_MEMCMP(out, shake256_abc_64, 64),
             "SHAKE256 \"abc\" 64-byte vector mismatch");
}

/* *****************************************************************************
Streaming / Incremental Tests
***************************************************************************** */

static void fio___test_sha3_streaming(void) {
  const char *msg = "The quick brown fox jumps over the lazy dog";
  size_t len = strlen(msg);

  uint8_t out1[32], out2[32], out3[32];

  /* SHA3-256 one-shot. */
  fio_sha3_256(out1, msg, len);

  /* Byte-by-byte streaming. */
  fio_sha3_s h = fio_sha3_256_init();
  for (size_t i = 0; i < len; ++i)
    fio_sha3_consume(&h, msg + i, 1);
  fio_sha3_finalize(&h, out2);
  FIO_ASSERT(!FIO_MEMCMP(out1, out2, 32),
             "SHA3-256 byte-by-byte streaming mismatch");

  /* Chunked streaming. */
  h = fio_sha3_256_init();
  fio_sha3_consume(&h, msg, len / 3);
  fio_sha3_consume(&h, msg + len / 3, len / 3);
  fio_sha3_consume(&h, msg + 2 * (len / 3), len - 2 * (len / 3));
  fio_sha3_finalize(&h, out3);
  FIO_ASSERT(!FIO_MEMCMP(out1, out3, 32), "SHA3-256 chunked streaming mismatch");

  /* All fixed-output variants produce different digests for the same input. */
  uint8_t out224[28], out256[32], out384[48], out512[64];
  fio_sha3_224(out224, msg, len);
  fio_sha3_256(out256, msg, len);
  fio_sha3_384(out384, msg, len);
  fio_sha3_512(out512, msg, len);
  FIO_ASSERT(FIO_MEMCMP(out224, out256, 28) != 0,
             "SHA3-224 and SHA3-256 produced identical outputs");
  FIO_ASSERT(FIO_MEMCMP(out256, out384, 32) != 0,
             "SHA3-256 and SHA3-384 produced identical outputs");
  FIO_ASSERT(FIO_MEMCMP(out384, out512, 48) != 0,
             "SHA3-384 and SHA3-512 produced identical outputs");
}

/* *****************************************************************************
SHAKE Multi-Squeeze Tests
***************************************************************************** */

static void fio___test_shake_multi_squeeze(void) {
  const char *msg = "seed data for SHAKE multi-squeeze";
  size_t len = strlen(msg);

  uint8_t one_shot[128];
  fio_shake128(one_shot, sizeof(one_shot), msg, len);

  uint8_t squeezed[128];
  fio_sha3_s ctx = fio_shake128_init();
  fio_shake_consume(&ctx, msg, len);
  uint8_t first[64];
  fio_shake_squeeze(&ctx, first, 64);
  fio_shake_squeeze(&ctx, squeezed, 64);

  FIO_ASSERT(!FIO_MEMCMP(one_shot, first, 64),
             "SHAKE128 first 64-byte squeeze mismatch");
  FIO_ASSERT(!FIO_MEMCMP(one_shot + 64, squeezed, 64),
             "SHAKE128 second 64-byte squeeze mismatch");
}

/* *****************************************************************************
SHA-3 / SHAKE Edge Cases
***************************************************************************** */

static void fio___test_sha3_edge_cases(void) {
  /* Empty SHAKE outputs are non-zero-length and non-trivial. */
  {
    uint8_t out128[16] = {0};
    uint8_t out256[16] = {0};
    fio_shake128(out128, 16, "", 0);
    fio_shake256(out256, 16, "", 0);

    uint8_t zero[16] = {0};
    FIO_ASSERT(FIO_MEMCMP(out128, zero, 16) != 0,
               "SHAKE128 empty input produced all zeros");
    FIO_ASSERT(FIO_MEMCMP(out256, zero, 16) != 0,
               "SHAKE256 empty input produced all zeros");
    FIO_ASSERT(FIO_MEMCMP(out128, out256, 16) != 0,
               "SHAKE128 and SHAKE256 produced identical empty outputs");
  }

  /* Rate-boundary inputs for SHA3-256 (rate = 136 bytes). */
  {
    size_t test_sizes[] = {135, 136, 137, 271, 272, 273};
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); ++i) {
      size_t len = test_sizes[i];
      uint8_t *data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, len, 0);
      FIO_ASSERT(data, "SHA3 edge-case allocation failed");
      FIO_MEMSET(data, 'A', len);

      uint8_t out[32];
      fio_sha3_256(out, data, len);

      uint8_t zero[32] = {0};
      FIO_ASSERT(FIO_MEMCMP(out, zero, 32) != 0,
                 "SHA3-256 at %zu bytes produced all zeros",
                 len);
      FIO_MEM_FREE(data, len);
    }
  }

  /* Determinism. */
  {
    uint8_t data[64];
    fio_rand_bytes(data, sizeof(data));

    uint8_t out1[32], out2[32];
    fio_sha3_256(out1, data, sizeof(data));
    fio_sha3_256(out2, data, sizeof(data));
    FIO_ASSERT(!FIO_MEMCMP(out1, out2, 32), "SHA3-256 not deterministic");
  }

  /* Input sensitivity. */
  {
    uint8_t data1[32] = {0};
    uint8_t data2[32] = {0};
    data2[0] = 1;

    uint8_t hash1[32], hash2[32];
    fio_sha3_256(hash1, data1, 32);
    fio_sha3_256(hash2, data2, 32);
    FIO_ASSERT(FIO_MEMCMP(hash1, hash2, 32) != 0,
               "SHA3-256 failed avalanche sensitivity test");
  }

  /* SHAKE variable output lengths are non-trivial. */
  {
    static const uint8_t data[] = "test input";
    size_t data_len = sizeof(data) - 1;
    size_t out_lens[] = {1, 16, 32, 64, 128, 256};

    for (size_t i = 0; i < sizeof(out_lens) / sizeof(out_lens[0]); ++i) {
      size_t out_len = out_lens[i];
      uint8_t *out = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, out_len, 0);
      FIO_ASSERT(out, "SHAKE allocation failed");

      fio_shake128(out, out_len, data, data_len);
      int zero = 1;
      for (size_t j = 0; j < out_len; ++j)
        if (out[j] != 0)
          zero = 0;
      FIO_ASSERT(!zero,
                 "SHAKE128 %zu-byte output is all zeros",
                 out_len);
      FIO_MEM_FREE(out, out_len);
    }
  }
}

int main(void) {
  fio___test_sha3_224_kat();
  fio___test_sha3_256_kat();
  fio___test_sha3_384_kat();
  fio___test_sha3_512_kat();
  fio___test_shake_kat();
  fio___test_sha3_streaming();
  fio___test_shake_multi_squeeze();
  fio___test_sha3_edge_cases();
  return 0;
}
