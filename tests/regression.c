/* *****************************************************************************
Regression Tests - Tests for previously fixed bugs to prevent regression
***************************************************************************** */
#undef FIO_MEMALT
#define FIO_MEMALT /* Enable fio_memcpy, fio_memset, fio_memchr, etc. */
#include "test-helpers.h"

#define FIO_JSON
#define FIO_STR
#define FIO_CRYPTO
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Part 1: fio_memchr with small lengths (len < 128)

Bug: The scalar fallback had an underflow bug when `len - 127` was calculated
for small lengths. This caused buffer overread/underread issues.

Test: Verify fio_memchr works correctly for lengths 0, 1, 7, 8, 64, 127, 128,
129
***************************************************************************** */

FIO_SFUNC void fio___test_regression_memchr_small_lengths(void) {
  /* Test buffer - fill with known pattern, place token at specific positions */
  char buf[256];
  FIO_MEMSET(buf, 'X', sizeof(buf));

  /* Test lengths that previously caused issues */
  size_t test_lengths[] = {0, 1, 7, 8, 64, 127, 128, 129};
  size_t num_lengths = sizeof(test_lengths) / sizeof(test_lengths[0]);

  for (size_t i = 0; i < num_lengths; ++i) {
    size_t len = test_lengths[i];

    /* Reset buffer */
    FIO_MEMSET(buf, 'X', sizeof(buf));

    /* Test 1: Token not present - should return NULL */
    {
      void *result = fio_memchr(buf, 'Z', len);
      FIO_ASSERT(result == NULL,
                 "fio_memchr(len=%zu) should return NULL when token not found",
                 len);
    }

    /* Test 2: Token at position 0 (if len > 0) */
    if (len > 0) {
      buf[0] = 'A';
      void *result = fio_memchr(buf, 'A', len);
      FIO_ASSERT(result == buf,
                 "fio_memchr(len=%zu) failed to find token at position 0",
                 len);
      buf[0] = 'X';
    }

    /* Test 3: Token at last position (if len > 0) */
    if (len > 0) {
      buf[len - 1] = 'B';
      void *result = fio_memchr(buf, 'B', len);
      FIO_ASSERT(result == buf + len - 1,
                 "fio_memchr(len=%zu) failed to find token at last position",
                 len);
      buf[len - 1] = 'X';
    }

    /* Test 4: Token at middle position (if len > 2) */
    if (len > 2) {
      size_t mid = len / 2;
      buf[mid] = 'C';
      void *result = fio_memchr(buf, 'C', len);
      FIO_ASSERT(result == buf + mid,
                 "fio_memchr(len=%zu) failed to find token at middle position",
                 len);
      buf[mid] = 'X';
    }

    /* Test 5: Token just past the search length - should NOT be found */
    if (len < sizeof(buf) - 1) {
      buf[len] = 'D';
      void *result = fio_memchr(buf, 'D', len);
      FIO_ASSERT(result == NULL,
                 "fio_memchr(len=%zu) incorrectly found token past search "
                 "length (buffer overread)",
                 len);
      buf[len] = 'X';
    }
  }

  /* Test 6: Specific edge case - len=127 (boundary for scalar fallback) */
  {
    FIO_MEMSET(buf, 'Y', sizeof(buf));
    buf[126] = 'E';
    void *result = fio_memchr(buf, 'E', 127);
    FIO_ASSERT(result == buf + 126,
               "fio_memchr(len=127) failed at boundary position 126");
  }

  /* Test 7: NUL byte search (common use case) */
  {
    FIO_MEMSET(buf, 'Z', sizeof(buf));
    for (size_t i = 0; i < num_lengths; ++i) {
      size_t len = test_lengths[i];
      if (len > 0) {
        buf[len / 2] = '\0';
        void *result = fio_memchr(buf, '\0', len);
        FIO_ASSERT(result == buf + len / 2,
                   "fio_memchr(len=%zu) failed to find NUL byte",
                   len);
        buf[len / 2] = 'Z';
      }
    }
  }
}

/* *****************************************************************************
Part 2: JSON comment parsing edge cases

Bug: There was a buffer overflow when parsing block comments (slash-star style),
particularly with minimal or edge-case comment content.

Test: Verify JSON parser handles various comment edge cases correctly.
***************************************************************************** */

/* Simple JSON callbacks for testing */
static void *test_json_reg_on_null(void) { return (void *)1; }
static void *test_json_reg_on_true(void) { return (void *)2; }
static void *test_json_reg_on_false(void) { return (void *)3; }
static void *test_json_reg_on_number(int64_t i) {
  return (void *)(uintptr_t)(i + 1000);
}
static void *test_json_reg_on_float(double f) {
  (void)f;
  return (void *)4;
}
static void *test_json_reg_on_string(const void *start, size_t len) {
  (void)start;
  (void)len;
  return (void *)5;
}
static void *test_json_reg_on_map(void *ctx, void *at) {
  (void)ctx;
  (void)at;
  return (void *)6;
}
static void *test_json_reg_on_array(void *ctx, void *at) {
  (void)ctx;
  (void)at;
  return (void *)7;
}
static int test_json_reg_map_push(void *ctx, void *key, void *value) {
  (void)ctx;
  (void)key;
  (void)value;
  return 0;
}
static int test_json_reg_array_push(void *ctx, void *value) {
  (void)ctx;
  (void)value;
  return 0;
}
static void test_json_reg_free_unused(void *obj) { (void)obj; }
static void *test_json_reg_on_error(void *ctx) { return ctx; }

static fio_json_parser_callbacks_s test_json_reg_callbacks = {
    .on_null = test_json_reg_on_null,
    .on_true = test_json_reg_on_true,
    .on_false = test_json_reg_on_false,
    .on_number = test_json_reg_on_number,
    .on_float = test_json_reg_on_float,
    .on_string = test_json_reg_on_string,
    .on_string_simple = test_json_reg_on_string,
    .on_map = test_json_reg_on_map,
    .on_array = test_json_reg_on_array,
    .map_push = test_json_reg_map_push,
    .array_push = test_json_reg_array_push,
    .free_unused_object = test_json_reg_free_unused,
    .on_error = test_json_reg_on_error,
};

FIO_SFUNC void fio___test_regression_json_comments(void) {
  /* Test 1: Minimal block comment followed by value */
  {
    const char *json = "/**/42";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(!r.err, "JSON minimal block comment /**/ should parse");
    FIO_ASSERT(r.ctx == (void *)(uintptr_t)(42 + 1000),
               "JSON value after /**/ should be 42");
  }

  /* Test 2: Block comment with single space */
  {
    const char *json = "/* */42";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(!r.err, "JSON block comment /* */ should parse");
    FIO_ASSERT(r.ctx == (void *)(uintptr_t)(42 + 1000),
               "JSON value after /* */ should be 42");
  }

  /* Test 3: Block comment with asterisks inside */
  {
    const char *json = "/****/42";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(!r.err, "JSON block comment /****/ should parse");
  }

  /* Test 4: Block comment with content */
  {
    const char *json = "/* this is a comment */42";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(!r.err, "JSON block comment with content should parse");
  }

  /* Test 5: Unclosed block comment - should fail */
  {
    const char *json = "/* unclosed";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(r.err, "JSON unclosed block comment should fail");
  }

  /* Test 6: Block comment at exact buffer boundary */
  {
    const char *json = "/**/";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    /* This should either parse successfully with no value, or fail gracefully
     */
    /* The key is it shouldn't crash or read past buffer */
    (void)r;
  }

  /* Test 7: Line comment followed by value */
  {
    const char *json = "// comment\n42";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(!r.err, "JSON line comment should parse");
    FIO_ASSERT(r.ctx == (void *)(uintptr_t)(42 + 1000),
               "JSON value after // comment should be 42");
  }

  /* Test 8: Hash comment followed by value */
  {
    const char *json = "# comment\n42";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(!r.err, "JSON hash comment should parse");
    FIO_ASSERT(r.ctx == (void *)(uintptr_t)(42 + 1000),
               "JSON value after # comment should be 42");
  }

  /* Test 9: Multiple consecutive comments */
  {
    const char *json = "/* a *//* b */42";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(!r.err, "JSON multiple block comments should parse");
  }

  /* Test 10: Comment with slash inside */
  {
    const char *json = "/* a/b */42";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(!r.err, "JSON block comment with slash inside should parse");
  }

  /* Test 11: Very short buffer - just slash-star */
  {
    const char *json = "/*";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(r.err, "JSON incomplete block comment /* should fail");
  }

  /* Test 12: Just "/" - not a comment */
  {
    const char *json = "/";
    fio_json_result_s r =
        fio_json_parse(&test_json_reg_callbacks, json, FIO_STRLEN(json));
    FIO_ASSERT(r.err, "JSON single slash should fail (not a valid comment)");
  }
}

/* *****************************************************************************
Part 3: Ed25519 negative carry (UB fix)

Bug: Left-shifting negative values was undefined behavior. The fix used
multiplication instead of left-shift for negative values.

Test: Verify Ed25519 operations work correctly, especially with edge case keys.
***************************************************************************** */

FIO_SFUNC void fio___test_regression_ed25519_negative_carry(void) {
  /* Test with RFC 8032 test vectors - these exercise the field arithmetic */

  /* Test Vector 1: Empty message */
  {
    static const uint8_t sk[32] = {
        0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60, 0xba, 0x84, 0x4a,
        0xf4, 0x92, 0xec, 0x2c, 0xc4, 0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32,
        0x69, 0x19, 0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60};
    static const uint8_t expected_pk[32] = {
        0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7, 0xd5, 0x4b, 0xfe,
        0xd3, 0xc9, 0x64, 0x07, 0x3a, 0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6,
        0x23, 0x25, 0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a};

    uint8_t pk[32];
    fio_ed25519_public_key(pk, sk);

    FIO_ASSERT(!memcmp(pk, expected_pk, 32),
               "Ed25519 public key derivation failed (regression test)");
  }

  /* Test with random keys - multiple iterations to catch intermittent issues */
  for (int i = 0; i < 10; ++i) {
    uint8_t sk[32], pk[32], sig[64];
    fio_ed25519_keypair(sk, pk);

    const char *message = "Test message for Ed25519 regression testing";
    size_t msg_len = strlen(message);

    fio_ed25519_sign(sig, message, msg_len, sk, pk);

    int verify_result = fio_ed25519_verify(sig, message, msg_len, pk);
    FIO_ASSERT(verify_result == 0,
               "Ed25519 sign/verify roundtrip failed (iteration %d)",
               i);
  }

  /* Test with all-zeros key (edge case for field arithmetic) */
  {
    uint8_t sk[32] = {0};
    uint8_t pk[32];
    /* This should not crash even with degenerate input */
    fio_ed25519_public_key(pk, sk);
    /* We don't check the result, just that it doesn't crash */
  }

  /* Test with all-ones key (edge case) */
  {
    uint8_t sk[32];
    memset(sk, 0xFF, 32);
    uint8_t pk[32];
    fio_ed25519_public_key(pk, sk);
    /* We don't check the result, just that it doesn't crash */
  }
}

/* *****************************************************************************
Part 4: AES-GCM unaligned access

Bug: Misaligned memory access was undefined behavior on some platforms.

Test: Verify AES-GCM works correctly with unaligned buffers.
***************************************************************************** */

FIO_SFUNC void fio___test_regression_aes_gcm_unaligned(void) {
  /* Create a buffer with extra space for alignment testing */
  uint8_t buffer_storage[512 + 64] FIO_ALIGN(64);
  uint8_t key[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                     0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                     0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  uint8_t nonce[12] =
      {0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88};
  uint8_t plaintext[64];
  uint8_t expected[64];
  uint8_t mac[16];

  /* Initialize plaintext with known pattern */
  for (size_t i = 0; i < 64; ++i)
    plaintext[i] = (uint8_t)(i + 1);
  FIO_MEMCPY(expected, plaintext, 64);

  /* Test with various alignment offsets (0-15 bytes) */
  for (size_t offset = 0; offset < 16; ++offset) {
    uint8_t *buffer = buffer_storage + offset;

    /* Test AES-128-GCM with unaligned buffer */
    {
      FIO_MEMCPY(buffer, plaintext, 64);
      fio_aes128_gcm_enc(mac, buffer, 64, NULL, 0, key, nonce);

      /* Verify ciphertext is different from plaintext */
      FIO_ASSERT(memcmp(buffer, plaintext, 64) != 0,
                 "AES-128-GCM encryption should modify data (offset=%zu)",
                 offset);

      /* Decrypt and verify roundtrip */
      int ret = fio_aes128_gcm_dec(mac, buffer, 64, NULL, 0, key, nonce);
      FIO_ASSERT(ret == 0,
                 "AES-128-GCM decryption auth failed (offset=%zu)",
                 offset);
      FIO_ASSERT(!memcmp(buffer, plaintext, 64),
                 "AES-128-GCM roundtrip failed (offset=%zu)",
                 offset);
    }

    /* Test AES-256-GCM with unaligned buffer */
    {
      FIO_MEMCPY(buffer, plaintext, 64);
      fio_aes256_gcm_enc(mac, buffer, 64, NULL, 0, key, nonce);

      /* Verify ciphertext is different from plaintext */
      FIO_ASSERT(memcmp(buffer, plaintext, 64) != 0,
                 "AES-256-GCM encryption should modify data (offset=%zu)",
                 offset);

      /* Decrypt and verify roundtrip */
      int ret = fio_aes256_gcm_dec(mac, buffer, 64, NULL, 0, key, nonce);
      FIO_ASSERT(ret == 0,
                 "AES-256-GCM decryption auth failed (offset=%zu)",
                 offset);
      FIO_ASSERT(!memcmp(buffer, plaintext, 64),
                 "AES-256-GCM roundtrip failed (offset=%zu)",
                 offset);
    }
  }

  /* Test with unaligned AAD as well */
  {
    uint8_t aad_storage[64 + 16] FIO_ALIGN(16);
    for (size_t aad_offset = 1; aad_offset < 8; ++aad_offset) {
      uint8_t *aad = aad_storage + aad_offset;
      for (size_t i = 0; i < 32; ++i)
        aad[i] = (uint8_t)(i * 3);

      for (size_t buf_offset = 1; buf_offset < 8; ++buf_offset) {
        uint8_t *buffer = buffer_storage + buf_offset;
        FIO_MEMCPY(buffer, plaintext, 64);

        fio_aes256_gcm_enc(mac, buffer, 64, aad, 32, key, nonce);
        int ret = fio_aes256_gcm_dec(mac, buffer, 64, aad, 32, key, nonce);

        FIO_ASSERT(ret == 0,
                   "AES-256-GCM with unaligned AAD failed "
                   "(buf_offset=%zu, aad_offset=%zu)",
                   buf_offset,
                   aad_offset);
        FIO_ASSERT(!memcmp(buffer, plaintext, 64),
                   "AES-256-GCM roundtrip with unaligned AAD failed "
                   "(buf_offset=%zu, aad_offset=%zu)",
                   buf_offset,
                   aad_offset);
      }
    }
  }

  /* Test with various message sizes at unaligned offsets */
  {
    size_t test_sizes[] = {1, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64};
    size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

    for (size_t s = 0; s < num_sizes; ++s) {
      size_t len = test_sizes[s];
      for (size_t offset = 1; offset < 8; ++offset) {
        uint8_t *buffer = buffer_storage + offset;
        FIO_MEMCPY(buffer, plaintext, len);

        fio_aes256_gcm_enc(mac, buffer, len, NULL, 0, key, nonce);
        int ret = fio_aes256_gcm_dec(mac, buffer, len, NULL, 0, key, nonce);

        FIO_ASSERT(ret == 0,
                   "AES-256-GCM unaligned failed (len=%zu, offset=%zu)",
                   len,
                   offset);
        FIO_ASSERT(!memcmp(buffer, plaintext, len),
                   "AES-256-GCM roundtrip unaligned failed "
                   "(len=%zu, offset=%zu)",
                   len,
                   offset);
      }
    }
  }
}

/* *****************************************************************************
Main Test Entry Point
***************************************************************************** */

int main(void) {
  /* Part 1: fio_memchr small lengths */
  fio___test_regression_memchr_small_lengths();

  /* Part 2: JSON comment parsing */
  fio___test_regression_json_comments();

  /* Part 3: Ed25519 negative carry */
  fio___test_regression_ed25519_negative_carry();

  /* Part 4: AES-GCM unaligned access */
  fio___test_regression_aes_gcm_unaligned();
  return 0;
}
