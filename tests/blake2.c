/* *****************************************************************************
Test for 152 blake2.h

Coverage: BLAKE2b and BLAKE2s one-shot hashing (fixed and variable output),
keyed hashing, streaming (init/consume/finalize) consistency, HMAC variants,
RFC 7693 known-answer vectors, the official BLAKE2 KAT suite, and OpenSSL
cross-validation when available. Performance loops are intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_BLAKE2
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

/* *****************************************************************************
BLAKE2b Known-Answer Vectors (RFC 7693 / reference implementation)
***************************************************************************** */

static void fio___test_blake2b_kat(void) {
  static const uint8_t empty[64] = {
      0x78, 0x6a, 0x02, 0xf7, 0x42, 0x01, 0x59, 0x03, 0xc6, 0xc6, 0xfd,
      0x85, 0x25, 0x52, 0xd2, 0x72, 0x91, 0x2f, 0x47, 0x40, 0xe1, 0x58,
      0x47, 0x61, 0x8a, 0x86, 0xe2, 0x17, 0xf7, 0x1f, 0x54, 0x19, 0xd2,
      0x5e, 0x10, 0x31, 0xaf, 0xee, 0x58, 0x53, 0x13, 0x89, 0x64, 0x44,
      0x93, 0x4e, 0xb0, 0x4b, 0x90, 0x3a, 0x68, 0x5b, 0x14, 0x48, 0xb7,
      0x55, 0xd5, 0x6f, 0x70, 0x1a, 0xfe, 0x9b, 0xe2, 0xce};
  static const uint8_t abc[64] = {
      0xba, 0x80, 0xa5, 0x3f, 0x98, 0x1c, 0x4d, 0x0d, 0x6a, 0x27, 0x97,
      0xb6, 0x9f, 0x12, 0xf6, 0xe9, 0x4c, 0x21, 0x2f, 0x14, 0x68, 0x5a,
      0xc4, 0xb7, 0x4b, 0x12, 0xbb, 0x6f, 0xdb, 0xff, 0xa2, 0xd1, 0x7d,
      0x87, 0xc5, 0x39, 0x2a, 0xab, 0x79, 0x2d, 0xc2, 0x52, 0xd5, 0xde,
      0x45, 0x33, 0xcc, 0x95, 0x18, 0xd3, 0x8a, 0xa8, 0xdb, 0xf1, 0x92,
      0x5a, 0xb9, 0x23, 0x86, 0xed, 0xd4, 0x00, 0x99, 0x23};

  uint8_t out[64];
  fio_blake2b_hash(out, 64, NULL, 0, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, empty, 64),
             "BLAKE2b empty unkeyed vector mismatch");

  fio_blake2b_hash(out, 64, "abc", 3, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, abc, 64), "BLAKE2b \"abc\" vector mismatch");
}

static void fio___test_blake2b_keyed(void) {
  /* Reference BLAKE2b keyed vector: key={0..63}, msg={0..254}. */
  uint8_t key[64];
  uint8_t msg[255];
  for (size_t i = 0; i < 64; ++i)
    key[i] = (uint8_t)i;
  for (size_t i = 0; i < 255; ++i)
    msg[i] = (uint8_t)i;

  static const uint8_t expected[64] = {
      0x14, 0x27, 0x09, 0xd6, 0x2e, 0x28, 0xfc, 0xcc, 0xd0, 0xaf, 0x97,
      0xfa, 0xd0, 0xf8, 0x46, 0x5b, 0x97, 0x1e, 0x82, 0x20, 0x1d, 0xc5,
      0x10, 0x70, 0xfa, 0xa0, 0x37, 0x2a, 0xa4, 0x3e, 0x92, 0x48, 0x4b,
      0xe1, 0xc1, 0xe7, 0x3b, 0xa1, 0x09, 0x06, 0xd5, 0xd1, 0x85, 0x3d,
      0xb6, 0xa4, 0x10, 0x6e, 0x0a, 0x7b, 0xf9, 0x80, 0x0d, 0x37, 0x3d,
      0x6d, 0xee, 0x2d, 0x46, 0xd6, 0x2e, 0xf2, 0xa4, 0x61};

  uint8_t out[64];
  fio_blake2b_hash(out, 64, msg, 255, key, 64);
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 64),
             "BLAKE2b keyed 255-byte vector mismatch");
}

/* *****************************************************************************
BLAKE2s Known-Answer Vectors (RFC 7693 / reference implementation)
***************************************************************************** */

static void fio___test_blake2s_kat(void) {
  static const uint8_t empty[32] = {
      0x69, 0x21, 0x7a, 0x30, 0x79, 0x90, 0x80, 0x94, 0xe1, 0x11, 0x21,
      0xd0, 0x42, 0x35, 0x4a, 0x7c, 0x1f, 0x55, 0xb6, 0x48, 0x2c, 0xa1,
      0xa5, 0x1e, 0x1b, 0x25, 0x0d, 0xfd, 0x1e, 0xd0, 0xee, 0xf9};
  static const uint8_t abc[32] = {
      0x50, 0x8c, 0x5e, 0x8c, 0x32, 0x7c, 0x14, 0xe2, 0xe1, 0xa7, 0x2b,
      0xa3, 0x4e, 0xeb, 0x45, 0x2f, 0x37, 0x45, 0x8b, 0x20, 0x9e, 0xd6,
      0x3a, 0x29, 0x4d, 0x99, 0x9b, 0x4c, 0x86, 0x67, 0x59, 0x82};

  uint8_t out[32];
  fio_blake2s_hash(out, 32, NULL, 0, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, empty, 32),
             "BLAKE2s empty unkeyed vector mismatch");

  fio_blake2s_hash(out, 32, "abc", 3, NULL, 0);
  FIO_ASSERT(!FIO_MEMCMP(out, abc, 32), "BLAKE2s \"abc\" vector mismatch");
}

/* *****************************************************************************
BLAKE2 Streaming / Incremental Tests
***************************************************************************** */

static void fio___test_blake2_streaming(void) {
  const char *msg = "The quick brown fox jumps over the lazy dog";
  size_t len = strlen(msg);

  /* BLAKE2b streaming. */
  {
    uint8_t out1[64], out2[64];
    fio_blake2b_hash(out1, 64, msg, len, NULL, 0);

    fio_blake2b_s h = fio_blake2b_init(64, NULL, 0);
    for (size_t i = 0; i < len; ++i)
      fio_blake2b_consume(&h, msg + i, 1);
    fio_u512 result = fio_blake2b_finalize(&h);
    FIO_MEMCPY(out2, result.u8, 64);
    FIO_ASSERT(!FIO_MEMCMP(out1, out2, 64),
               "BLAKE2b byte-by-byte streaming mismatch");
  }

  /* BLAKE2s streaming. */
  {
    uint8_t out1[32], out2[32];
    fio_blake2s_hash(out1, 32, msg, len, NULL, 0);

    fio_blake2s_s h = fio_blake2s_init(32, NULL, 0);
    for (size_t i = 0; i < len; ++i)
      fio_blake2s_consume(&h, msg + i, 1);
    fio_u256 result = fio_blake2s_finalize(&h);
    FIO_MEMCPY(out2, result.u8, 32);
    FIO_ASSERT(!FIO_MEMCMP(out1, out2, 32),
               "BLAKE2s byte-by-byte streaming mismatch");
  }
}

/* *****************************************************************************
BLAKE2 HMAC Tests
***************************************************************************** */

static void fio___test_blake2_hmac(void) {
  static const char key[] = "authentication-key";
  static const char msg[] = "message to authenticate";

  fio_u512 b2b = fio_blake2b_hmac(key, strlen(key), msg, strlen(msg));
  fio_u256 b2s = fio_blake2s_hmac(key, strlen(key), msg, strlen(msg));

  uint8_t zero64[64] = {0};
  uint8_t zero32[32] = {0};
  FIO_ASSERT(FIO_MEMCMP(b2b.u8, zero64, 64) != 0,
             "HMAC-BLAKE2b produced all zeros");
  FIO_ASSERT(FIO_MEMCMP(b2s.u8, zero32, 32) != 0,
             "HMAC-BLAKE2s produced all zeros");

  /* Different messages produce different MACs. */
  fio_u512 b2b2 = fio_blake2b_hmac(key, strlen(key), msg, strlen(msg) - 1);
  FIO_ASSERT(FIO_MEMCMP(b2b.u8, b2b2.u8, 64) != 0,
             "HMAC-BLAKE2b insensitive to message length");

  /* Long keys are hashed internally. */
  uint8_t long_key[256];
  fio_rand_bytes(long_key, sizeof(long_key));
  fio_u512 b2b_long = fio_blake2b_hmac(long_key, sizeof(long_key), msg, strlen(msg));
  FIO_ASSERT(FIO_MEMCMP(b2b_long.u8, zero64, 64) != 0,
             "HMAC-BLAKE2b with long key produced all zeros");
}

/* *****************************************************************************
Official BLAKE2 Known-Answer Test (KAT) Suite
Key = {0,1,...,63} for BLAKE2b / {0,...,31} for BLAKE2s.
Message[i] = {0,1,...,i-1} for entry i.
***************************************************************************** */

typedef struct {
  size_t msg_len;
  uint8_t hash[64];
} fio___blake2b_kat_s;

static const fio___blake2b_kat_s fio___blake2b_kat[] = {
    /* entry 0: empty message */
    {0,
     {0x10, 0xeb, 0xb6, 0x77, 0x00, 0xb1, 0x86, 0x8e, 0xfb, 0x44, 0x17,
      0x98, 0x7a, 0xcf, 0x46, 0x90, 0xae, 0x9d, 0x97, 0x2f, 0xb7, 0xa5,
      0x90, 0xc2, 0xf0, 0x28, 0x71, 0x79, 0x9a, 0xaa, 0x47, 0x86, 0xb5,
      0xe9, 0x96, 0xe8, 0xf0, 0xf4, 0xeb, 0x98, 0x1f, 0xc2, 0x14, 0xb0,
      0x05, 0xf4, 0x2d, 0x2f, 0xf4, 0x23, 0x34, 0x99, 0x39, 0x16, 0x53,
      0xdf, 0x7a, 0xef, 0xcb, 0xc1, 0x3f, 0xc5, 0x15, 0x68}},
    /* entry 1: {0x00} */
    {1,
     {0x96, 0x1f, 0x6d, 0xd1, 0xe4, 0xdd, 0x30, 0xf6, 0x39, 0x01, 0x69,
      0x0c, 0x51, 0x2e, 0x78, 0xe4, 0xb4, 0x5e, 0x47, 0x42, 0xed, 0x19,
      0x7c, 0x3c, 0x5e, 0x45, 0xc5, 0x49, 0xfd, 0x25, 0xf2, 0xe4, 0x18,
      0x7b, 0x0b, 0xc9, 0xfe, 0x30, 0x49, 0x2b, 0x16, 0xb0, 0xd0, 0xbc,
      0x4e, 0xf9, 0xb0, 0xf3, 0x4c, 0x70, 0x03, 0xfa, 0xc0, 0x9a, 0x5e,
      0xf1, 0x53, 0x2e, 0x69, 0x43, 0x02, 0x34, 0xce, 0xbd}},
    /* entry 2: {0x00,0x01} */
    {2,
     {0xda, 0x2c, 0xfb, 0xe2, 0xd8, 0x40, 0x9a, 0x0f, 0x38, 0x02, 0x61,
      0x13, 0x88, 0x4f, 0x84, 0xb5, 0x01, 0x56, 0x37, 0x1a, 0xe3, 0x04,
      0xc4, 0x43, 0x01, 0x73, 0xd0, 0x8a, 0x99, 0xd9, 0xfb, 0x1b, 0x98,
      0x31, 0x64, 0xa3, 0x77, 0x07, 0x06, 0xd5, 0x37, 0xf4, 0x9e, 0x0c,
      0x91, 0x6d, 0x9f, 0x32, 0xb9, 0x5c, 0xc3, 0x7a, 0x95, 0xb9, 0x9d,
      0x85, 0x74, 0x36, 0xf0, 0x23, 0x2c, 0x88, 0xa9, 0x65}},
    /* entry 7: {0x00..0x06} */
    {7,
     {0x7a, 0x8c, 0xfe, 0x9b, 0x90, 0xf7, 0x5f, 0x7e, 0xcb, 0x3a, 0xcc,
      0x05, 0x3a, 0xae, 0xd6, 0x19, 0x31, 0x12, 0xb6, 0xf6, 0xa4, 0xae,
      0xeb, 0x3f, 0x65, 0xd3, 0xde, 0x54, 0x19, 0x42, 0xde, 0xb9, 0xe2,
      0x22, 0x81, 0x52, 0xa3, 0xc4, 0xbb, 0xbe, 0x72, 0xfc, 0x3b, 0x12,
      0x62, 0x95, 0x28, 0xcf, 0xbb, 0x09, 0xfe, 0x63, 0x0f, 0x04, 0x74,
      0x33, 0x9f, 0x54, 0xab, 0xf4, 0x53, 0xe2, 0xed, 0x52}},
    /* entry 255: {0x00..0xfe} */
    {255,
     {0x14, 0x27, 0x09, 0xd6, 0x2e, 0x28, 0xfc, 0xcc, 0xd0, 0xaf, 0x97,
      0xfa, 0xd0, 0xf8, 0x46, 0x5b, 0x97, 0x1e, 0x82, 0x20, 0x1d, 0xc5,
      0x10, 0x70, 0xfa, 0xa0, 0x37, 0x2a, 0xa4, 0x3e, 0x92, 0x48, 0x4b,
      0xe1, 0xc1, 0xe7, 0x3b, 0xa1, 0x09, 0x06, 0xd5, 0xd1, 0x85, 0x3d,
      0xb6, 0xa4, 0x10, 0x6e, 0x0a, 0x7b, 0xf9, 0x80, 0x0d, 0x37, 0x3d,
      0x6d, 0xee, 0x2d, 0x46, 0xd6, 0x2e, 0xf2, 0xa4, 0x61}},
};

typedef struct {
  size_t msg_len;
  uint8_t hash[32];
} fio___blake2s_kat_s;

static const fio___blake2s_kat_s fio___blake2s_kat[] = {
    /* entry 0: empty message */
    {0,
     {0x48, 0xa8, 0x99, 0x7d, 0xa4, 0x07, 0x87, 0x6b, 0x3d, 0x79, 0xc0,
      0xd9, 0x23, 0x25, 0xad, 0x3b, 0x89, 0xcb, 0xb7, 0x54, 0xd8, 0x6a,
      0xb7, 0x1a, 0xee, 0x04, 0x7a, 0xd3, 0x45, 0xfd, 0x2c, 0x49}},
    /* entry 1: {0x00} */
    {1,
     {0x40, 0xd1, 0x5f, 0xee, 0x7c, 0x32, 0x88, 0x30, 0x16, 0x6a, 0xc3,
      0xf9, 0x18, 0x65, 0x0f, 0x80, 0x7e, 0x7e, 0x01, 0xe1, 0x77, 0x25,
      0x8c, 0xdc, 0x0a, 0x39, 0xb1, 0x1f, 0x59, 0x80, 0x66, 0xf1}},
    /* entry 2: {0x00,0x01} */
    {2,
     {0x6b, 0xb7, 0x13, 0x00, 0x64, 0x4c, 0xd3, 0x99, 0x1b, 0x26, 0xcc,
      0xd4, 0xd2, 0x74, 0xac, 0xd1, 0xad, 0xea, 0xb8, 0xb1, 0xd7, 0x91,
      0x45, 0x46, 0xc1, 0x19, 0x8b, 0xbe, 0x9f, 0xc9, 0xd8, 0x03}},
    /* entry 7: {0x00..0x06} */
    {7,
     {0xe6, 0xc8, 0x12, 0x56, 0x37, 0x43, 0x8d, 0x09, 0x05, 0xb7, 0x49,
      0xf4, 0x65, 0x60, 0xac, 0x89, 0xfd, 0x47, 0x1c, 0xf8, 0x69, 0x2e,
      0x28, 0xfa, 0xb9, 0x82, 0xf7, 0x3f, 0x01, 0x9b, 0x83, 0xa9}},
    /* entry 255: {0x00..0xfe} */
    {255,
     {0x3f, 0xb7, 0x35, 0x06, 0x1a, 0xbc, 0x51, 0x9d, 0xfe, 0x97, 0x9e,
      0x54, 0xc1, 0xee, 0x5b, 0xfa, 0xd0, 0xa9, 0xd8, 0x58, 0xb3, 0x31,
      0x5b, 0xad, 0x34, 0xbd, 0xe9, 0x99, 0xef, 0xd7, 0x24, 0xdd}},
};

static void fio___test_blake2_official_kat(void) {
  uint8_t key_b[64];
  for (size_t i = 0; i < 64; ++i)
    key_b[i] = (uint8_t)i;

  for (size_t i = 0; i < sizeof(fio___blake2b_kat) / sizeof(fio___blake2b_kat[0]); ++i) {
    size_t msg_len = fio___blake2b_kat[i].msg_len;
    uint8_t *msg = NULL;
    if (msg_len) {
      msg = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, msg_len, 0);
      FIO_ASSERT(msg, "BLAKE2b KAT allocation failed");
      for (size_t j = 0; j < msg_len; ++j)
        msg[j] = (uint8_t)j;
    }

    uint8_t out[64];
    fio_blake2b_hash(out, 64, msg, msg_len, key_b, 64);
    FIO_ASSERT(!FIO_MEMCMP(out, fio___blake2b_kat[i].hash, 64),
               "BLAKE2b KAT entry %zu (msg_len=%zu) failed",
               i,
               msg_len);
    if (msg)
      FIO_MEM_FREE(msg, msg_len);
  }

  uint8_t key_s[32];
  for (size_t i = 0; i < 32; ++i)
    key_s[i] = (uint8_t)i;

  for (size_t i = 0; i < sizeof(fio___blake2s_kat) / sizeof(fio___blake2s_kat[0]); ++i) {
    size_t msg_len = fio___blake2s_kat[i].msg_len;
    uint8_t *msg = NULL;
    if (msg_len) {
      msg = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, msg_len, 0);
      FIO_ASSERT(msg, "BLAKE2s KAT allocation failed");
      for (size_t j = 0; j < msg_len; ++j)
        msg[j] = (uint8_t)j;
    }

    uint8_t out[32];
    fio_blake2s_hash(out, 32, msg, msg_len, key_s, 32);
    FIO_ASSERT(!FIO_MEMCMP(out, fio___blake2s_kat[i].hash, 32),
               "BLAKE2s KAT entry %zu (msg_len=%zu) failed",
               i,
               msg_len);
    if (msg)
      FIO_MEM_FREE(msg, msg_len);
  }
}

/* *****************************************************************************
OpenSSL Cross-Validation (library only, no external processes)
***************************************************************************** */

#if HAVE_OPENSSL
static void fio___test_blake2_openssl_compare(void) {
  static const size_t test_sizes[] = {0, 1, 63, 64, 65, 127, 128, 129,
                                      255, 256, 1000, 65536};
  size_t n = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t t = 0; t < n; ++t) {
    size_t len = test_sizes[t];
    uint8_t *data = NULL;
    if (len) {
      data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, len, 0);
      FIO_ASSERT(data, "BLAKE2 OpenSSL compare allocation failed");
      RAND_bytes(data, (int)len);
    }

    /* BLAKE2b-512 vs OpenSSL EVP_blake2b512(). */
    {
      uint8_t fio_out[64];
      fio_blake2b_hash(fio_out, 64, data, len, NULL, 0);

      uint8_t ossl_out[64];
      unsigned int ossl_len = 64;
      EVP_MD_CTX *ctx = EVP_MD_CTX_new();
      FIO_ASSERT(ctx, "EVP_MD_CTX_new failed");
      FIO_ASSERT(EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL),
                 "EVP_DigestInit_ex(blake2b512) failed");
      if (len)
        FIO_ASSERT(EVP_DigestUpdate(ctx, data, len),
                   "EVP_DigestUpdate failed");
      FIO_ASSERT(EVP_DigestFinal_ex(ctx, ossl_out, &ossl_len),
                 "EVP_DigestFinal_ex failed");
      EVP_MD_CTX_free(ctx);

      FIO_ASSERT(!FIO_MEMCMP(fio_out, ossl_out, 64),
                 "BLAKE2b-512 OpenSSL mismatch at input size %zu",
                 len);
    }

    /* BLAKE2s-256 vs OpenSSL EVP_blake2s256(). */
    {
      uint8_t fio_out[32];
      fio_blake2s_hash(fio_out, 32, data, len, NULL, 0);

      uint8_t ossl_out[32];
      unsigned int ossl_len = 32;
      EVP_MD_CTX *ctx = EVP_MD_CTX_new();
      FIO_ASSERT(ctx, "EVP_MD_CTX_new failed");
      FIO_ASSERT(EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL),
                 "EVP_DigestInit_ex(blake2s256) failed");
      if (len)
        FIO_ASSERT(EVP_DigestUpdate(ctx, data, len),
                   "EVP_DigestUpdate failed");
      FIO_ASSERT(EVP_DigestFinal_ex(ctx, ossl_out, &ossl_len),
                 "EVP_DigestFinal_ex failed");
      EVP_MD_CTX_free(ctx);

      FIO_ASSERT(!FIO_MEMCMP(fio_out, ossl_out, 32),
                 "BLAKE2s-256 OpenSSL mismatch at input size %zu",
                 len);
    }

    if (data)
      FIO_MEM_FREE(data, len);
  }
}
#endif /* HAVE_OPENSSL */

/* *****************************************************************************
BLAKE2 Edge Cases
***************************************************************************** */

static void fio___test_blake2_edge_cases(void) {
  /* Variable output lengths encode the digest length. */
  {
    static const uint8_t data[] = "test";
    uint8_t out32[32], out64[64];
    fio_blake2b_hash(out32, 32, data, 4, NULL, 0);
    fio_blake2b_hash(out64, 64, data, 4, NULL, 0);
    FIO_ASSERT(FIO_MEMCMP(out32, out64, 32) != 0,
               "BLAKE2b different output lengths produced matching prefix");
  }

  /* Different keys produce different outputs. */
  {
    static const uint8_t data[] = "test";
    uint8_t key1[32] = {0};
    uint8_t key2[32] = {0};
    key2[0] = 1;
    uint8_t out1[64], out2[64];
    fio_blake2b_hash(out1, 64, data, 4, key1, 32);
    fio_blake2b_hash(out2, 64, data, 4, key2, 32);
    FIO_ASSERT(FIO_MEMCMP(out1, out2, 64) != 0,
               "BLAKE2b keyed output insensitive to key");
  }

  /* Empty key is equivalent to no key. */
  {
    static const uint8_t data[] = "test";
    uint8_t out1[64], out2[64];
    fio_blake2b_hash(out1, 64, data, 4, NULL, 0);
    fio_blake2b_hash(out2, 64, data, 4, (const uint8_t *)"", 0);
    FIO_ASSERT(!FIO_MEMCMP(out1, out2, 64),
               "BLAKE2b empty key differs from NULL key");
  }

  /* Block-boundary inputs. */
  {
    size_t test_sizes[] = {63, 64, 65, 127, 128, 129};
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); ++i) {
      size_t len = test_sizes[i];
      uint8_t *data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, len, 0);
      FIO_ASSERT(data, "BLAKE2 edge-case allocation failed");
      FIO_MEMSET(data, 'A', len);

      uint8_t out_b[64], out_s[32];
      fio_blake2b_hash(out_b, 64, data, len, NULL, 0);
      fio_blake2s_hash(out_s, 32, data, len, NULL, 0);

      uint8_t zero64[64] = {0};
      uint8_t zero32[32] = {0};
      FIO_ASSERT(FIO_MEMCMP(out_b, zero64, 64) != 0,
                 "BLAKE2b at %zu bytes produced all zeros",
                 len);
      FIO_ASSERT(FIO_MEMCMP(out_s, zero32, 32) != 0,
                 "BLAKE2s at %zu bytes produced all zeros",
                 len);
      FIO_MEM_FREE(data, len);
    }
  }
}

int main(void) {
  fio___test_blake2b_kat();
  fio___test_blake2b_keyed();
  fio___test_blake2s_kat();
  fio___test_blake2_streaming();
  fio___test_blake2_hmac();
  fio___test_blake2_official_kat();
#if HAVE_OPENSSL
  fio___test_blake2_openssl_compare();
#endif
  fio___test_blake2_edge_cases();
  return 0;
}
