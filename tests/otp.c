/* *****************************************************************************
OTP (One-Time Password) Unit Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_OTP
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Test Tracking
***************************************************************************** */

static struct {
  size_t passed;
  size_t failed;
} OTP_TEST_RESULTS;

#define OTP_TEST_PASS()                                                        \
  do {                                                                         \
    ++OTP_TEST_RESULTS.passed;                                                 \
  } while (0)

#define OTP_TEST_FAIL(...)                                                     \
  do {                                                                         \
    ++OTP_TEST_RESULTS.failed;                                                 \
    FIO_LOG_ERROR(__VA_ARGS__);                                                \
  } while (0)

#define OTP_TEST_ASSERT(cond, ...)                                             \
  do {                                                                         \
    if (!(cond)) {                                                             \
      OTP_TEST_FAIL(__VA_ARGS__);                                              \
    } else {                                                                   \
      OTP_TEST_PASS();                                                         \
    }                                                                          \
  } while (0)

/* *****************************************************************************
RFC 6238 Test Vectors for TOTP
https://www.rfc-editor.org/rfc/rfc6238#appendix-B

Note: RFC 6238 uses SHA-1 based HOTP (RFC 4226) with time-based counter.
The test vectors use the ASCII string "12345678901234567890" as the shared
secret (raw bytes, not Base32 encoded).
***************************************************************************** */

/* RFC 6238 Appendix B test secret (ASCII - 20 bytes for SHA-1) */
#define RFC6238_SECRET_RAW "12345678901234567890"
#define RFC6238_SECRET_LEN 20

/* RFC 6238 test vectors - time values and expected 8-digit TOTP codes */
typedef struct {
  uint64_t unix_time;
  uint32_t expected_totp;
} rfc6238_test_vector_t;

/*
 * These are the SHA-1 based test vectors from RFC 6238 Appendix B.
 * The RFC specifies 8-digit codes with 30-second time step.
 */
static const rfc6238_test_vector_t rfc6238_vectors[] = {
    {.unix_time = 59, .expected_totp = 94287082},
    {.unix_time = 1111111109, .expected_totp = 7081804},
    {.unix_time = 1111111111, .expected_totp = 14050471},
    {.unix_time = 1234567890, .expected_totp = 89005924},
    {.unix_time = 2000000000, .expected_totp = 69279037},
    {.unix_time = 20000000000ULL, .expected_totp = 65353130},
};
#define RFC6238_VECTOR_COUNT                                                   \
  (sizeof(rfc6238_vectors) / sizeof(rfc6238_vectors[0]))

/* *****************************************************************************
Key Generation Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_key_generation)(void) {
  FIO_LOG_DDEBUG("Testing OTP key generation");

  /* Test that generated keys are non-zero */
  for (int i = 0; i < 10; ++i) {
    fio_u128 key = fio_otp_generate_key();
    OTP_TEST_ASSERT(key.u64[0] != 0 || key.u64[1] != 0,
                    "Generated OTP key should not be all zeros");
  }

  /* Test that generated keys are unique (probabilistic) */
  {
    fio_u128 keys[100];
    for (int i = 0; i < 100; ++i) {
      keys[i] = fio_otp_generate_key();
    }
    /* Check for duplicates - with 128-bit keys, collision is astronomically
     * unlikely */
    for (int i = 0; i < 100; ++i) {
      for (int j = i + 1; j < 100; ++j) {
        OTP_TEST_ASSERT(keys[i].u64[0] != keys[j].u64[0] ||
                            keys[i].u64[1] != keys[j].u64[1],
                        "Generated OTP keys should be unique (collision at %d, "
                        "%d)",
                        i,
                        j);
      }
    }
  }

  FIO_LOG_DDEBUG("  Key generation: PASSED");
}

/* *****************************************************************************
Key Printing (Base32 encoding) Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_key_printing)(void) {
  FIO_LOG_DDEBUG("Testing OTP key printing (Base32 encoding)");

  /* Test printing a known key */
  {
    uint8_t test_key[10] =
        {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x21, 0xDE, 0xAD, 0xBE, 0xEF};
    char output[64] = {0};
    size_t len = fio_otp_print_key(output, test_key, 10);
    OTP_TEST_ASSERT(len > 0, "fio_otp_print_key should return non-zero length");
    OTP_TEST_ASSERT(output[0] != 0, "Output should not be empty");
    FIO_LOG_DDEBUG("  Test key Base32: %s (len=%zu)", output, len);
  }

  /* Test with NULL key (should generate a new key) */
  {
    char output1[64] = {0};
    char output2[64] = {0};
    size_t len1 = fio_otp_print_key(output1, NULL, 16);
    size_t len2 = fio_otp_print_key(output2, NULL, 16);
    OTP_TEST_ASSERT(len1 > 0 && len2 > 0,
                    "Should generate keys when NULL passed");
    /* Keys should be different (probabilistic) */
    OTP_TEST_ASSERT(strcmp(output1, output2) != 0,
                    "Generated keys should be different");
  }

  FIO_LOG_DDEBUG("  Key printing: PASSED");
}

/* *****************************************************************************
TOTP Basic Functionality Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_basic)(void) {
  FIO_LOG_DDEBUG("Testing OTP basic functionality");

  /* Test with raw secret */
  {
    const char *secret = "12345678901234567890";
    fio_buf_info_s secret_buf = FIO_BUF_INFO1((char *)secret);

    uint32_t otp1 = fio_otp(secret_buf, .is_raw = 1, .digits = 6);
    uint32_t otp2 = fio_otp(secret_buf, .is_raw = 1, .digits = 6);

    /* Same time window should produce same OTP */
    OTP_TEST_ASSERT(otp1 == otp2,
                    "Same secret and time should produce same OTP: %u vs %u",
                    otp1,
                    otp2);

    /* OTP should be within valid range for 6 digits */
    OTP_TEST_ASSERT(otp1 < 1000000,
                    "6-digit OTP should be less than 1000000: got %u",
                    otp1);

    FIO_LOG_DDEBUG("  Current 6-digit TOTP: %06u", otp1);
  }

  /* Test different digit counts (1-9, as 10 digits overflows uint32_t) */
  {
    const char *secret = "TESTSECRET123456";
    fio_buf_info_s secret_buf = FIO_BUF_INFO1((char *)secret);

    for (size_t digits = 1; digits <= 9; ++digits) {
      uint32_t otp = fio_otp(secret_buf, .is_raw = 1, .digits = digits);
      uint32_t max_val = 1;
      for (size_t d = 0; d < digits; ++d)
        max_val *= 10;
      OTP_TEST_ASSERT(otp < max_val,
                      "%zu-digit OTP should be less than %u: got %u",
                      digits,
                      max_val,
                      otp);
    }
  }

  FIO_LOG_DDEBUG("  Basic functionality: PASSED");
}

/* *****************************************************************************
TOTP Time Offset Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_time_offset)(void) {
  FIO_LOG_DDEBUG("Testing OTP time offset");

  const char *secret = "12345678901234567890";
  fio_buf_info_s secret_buf = FIO_BUF_INFO1((char *)secret);

  /* Get current OTP */
  uint32_t otp_current = fio_otp(secret_buf, .is_raw = 1, .offset = 0);

  /* Get previous time window OTP */
  uint32_t otp_prev = fio_otp(secret_buf, .is_raw = 1, .offset = 1);

  /* Get next time window OTP */
  uint32_t otp_next = fio_otp(secret_buf, .is_raw = 1, .offset = -1);

  FIO_LOG_DDEBUG("  Previous: %06u, Current: %06u, Next: %06u",
                 otp_prev,
                 otp_current,
                 otp_next);

  /* They should all be valid 6-digit codes */
  OTP_TEST_ASSERT(otp_current < 1000000, "Current OTP out of range");
  OTP_TEST_ASSERT(otp_prev < 1000000, "Previous OTP out of range");
  OTP_TEST_ASSERT(otp_next < 1000000, "Next OTP out of range");

  /* In most cases, they should be different (unless we hit a boundary) */
  /* Note: This is probabilistic - in rare cases they could be the same */

  FIO_LOG_DDEBUG("  Time offset: PASSED");
}

/* *****************************************************************************
TOTP Interval Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_interval)(void) {
  FIO_LOG_DDEBUG("Testing OTP interval settings");

  const char *secret = "12345678901234567890";
  fio_buf_info_s secret_buf = FIO_BUF_INFO1((char *)secret);

  /* Default interval (30 seconds - Google Authenticator standard) */
  uint32_t otp_30s = fio_otp(secret_buf, .is_raw = 1, .interval = 30);

  /* 60 second interval */
  uint32_t otp_60s = fio_otp(secret_buf, .is_raw = 1, .interval = 60);
  (void)otp_60s; /* Used only in debug output */

  /* With 0 interval (should default to 30) */
  uint32_t otp_default = fio_otp(secret_buf, .is_raw = 1, .interval = 0);

  OTP_TEST_ASSERT(otp_30s == otp_default,
                  "Zero interval should default to 30 seconds");

  FIO_LOG_DDEBUG("  30s interval: %06u, 60s interval: %06u", otp_30s, otp_60s);

  FIO_LOG_DDEBUG("  Interval settings: PASSED");
}

/* *****************************************************************************
Base32 Encoded Secret Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_base32)(void) {
  FIO_LOG_DDEBUG("Testing OTP with Base32 encoded secrets");

  /* "Hello!" in Base32 is "JBSWY3DPEE" (padded) or similar */
  /* Let's use a known Base32 encoded secret */

  /* First, encode a raw secret to Base32, then decode and verify */
  {
    uint8_t raw_secret[10] =
        {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x21, 0xDE, 0xAD, 0xBE, 0xEF};
    char base32_secret[64] = {0};

    /* Encode to Base32 */
    fio_str_info_s b32 = FIO_STR_INFO3(base32_secret, 0, sizeof(base32_secret));
    fio_string_write_base32enc(&b32, NULL, raw_secret, 10);

    FIO_LOG_DDEBUG("  Base32 encoded secret: %s", base32_secret);

    /* Get OTP with raw secret */
    fio_buf_info_s raw_buf = FIO_BUF_INFO2((char *)raw_secret, 10);
    uint32_t otp_raw = fio_otp(raw_buf, .is_raw = 1);

    /* Get OTP with Base32 encoded secret */
    fio_buf_info_s b32_buf = FIO_BUF_INFO2(base32_secret, b32.len);
    uint32_t otp_b32 = fio_otp(b32_buf, .is_raw = 0, .is_hex = 0);

    OTP_TEST_ASSERT(otp_raw == otp_b32,
                    "Raw and Base32 encoded secrets should produce same OTP: "
                    "%06u vs %06u",
                    otp_raw,
                    otp_b32);

    FIO_LOG_DDEBUG("  OTP from raw: %06u, from Base32: %06u", otp_raw, otp_b32);
  }

  FIO_LOG_DDEBUG("  Base32 encoding: PASSED");
}

/* *****************************************************************************
Hex Encoded Secret Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_hex)(void) {
  FIO_LOG_DDEBUG("Testing OTP with Hex encoded secrets");

  {
    /* Raw secret bytes */
    uint8_t raw_secret[10] =
        {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x21, 0xDE, 0xAD, 0xBE, 0xEF};

    /* Hex representation */
    const char *hex_secret = "48656C6C6F21DEADBEEF";

    /* Get OTP with raw secret */
    fio_buf_info_s raw_buf = FIO_BUF_INFO2((char *)raw_secret, 10);
    uint32_t otp_raw = fio_otp(raw_buf, .is_raw = 1);

    /* Get OTP with hex encoded secret */
    fio_buf_info_s hex_buf = FIO_BUF_INFO1((char *)hex_secret);
    uint32_t otp_hex = fio_otp(hex_buf, .is_hex = 1);

    OTP_TEST_ASSERT(otp_raw == otp_hex,
                    "Raw and Hex encoded secrets should produce same OTP: "
                    "%06u vs %06u",
                    otp_raw,
                    otp_hex);

    FIO_LOG_DDEBUG("  OTP from raw: %06u, from hex: %06u", otp_raw, otp_hex);
  }

  /* Test hex with separators */
  {
    uint8_t raw_secret[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    const char *hex_with_sep = "DE-AD-BE-EF";

    fio_buf_info_s raw_buf = FIO_BUF_INFO2((char *)raw_secret, 4);
    uint32_t otp_raw = fio_otp(raw_buf, .is_raw = 1);

    fio_buf_info_s hex_buf = FIO_BUF_INFO1((char *)hex_with_sep);
    uint32_t otp_hex = fio_otp(hex_buf, .is_hex = 1);

    OTP_TEST_ASSERT(otp_raw == otp_hex,
                    "Hex with separators should work: %06u vs %06u",
                    otp_raw,
                    otp_hex);

    FIO_LOG_DDEBUG("  Hex with separators: PASSED");
  }

  FIO_LOG_DDEBUG("  Hex encoding: PASSED");
}

/* *****************************************************************************
Consistency Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_consistency)(void) {
  FIO_LOG_DDEBUG("Testing OTP consistency");

  const char *secret = "CONSISTENCYTEST1234567890";
  fio_buf_info_s secret_buf = FIO_BUF_INFO1((char *)secret);

  /* Multiple calls with same parameters should return same result */
  uint32_t results[100];
  for (int i = 0; i < 100; ++i) {
    results[i] = fio_otp(secret_buf, .is_raw = 1, .digits = 6);
  }

  for (int i = 1; i < 100; ++i) {
    OTP_TEST_ASSERT(results[0] == results[i],
                    "OTP should be consistent: %u vs %u at iteration %d",
                    results[0],
                    results[i],
                    i);
  }

  FIO_LOG_DDEBUG("  100 consecutive calls returned: %06u", results[0]);
  FIO_LOG_DDEBUG("  Consistency: PASSED");
}

/* *****************************************************************************
Edge Cases
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_edge_cases)(void) {
  FIO_LOG_DDEBUG("Testing OTP edge cases");

  /* Minimum secret length */
  {
    const char *short_secret = "A";
    fio_buf_info_s buf = FIO_BUF_INFO1((char *)short_secret);
    uint32_t otp = fio_otp(buf, .is_raw = 1);
    OTP_TEST_ASSERT(otp < 1000000,
                    "Short secret should still produce valid OTP");
    FIO_LOG_DDEBUG("  Single byte secret: %06u", otp);
  }

  /* Default settings (0 values should use defaults) */
  {
    const char *secret = "DEFAULTTEST";
    fio_buf_info_s buf = FIO_BUF_INFO1((char *)secret);
    uint32_t otp = fio_otp(buf, .is_raw = 1);
    /* With defaults (interval=30, digits=6), should produce valid 6-digit OTP
     */
    OTP_TEST_ASSERT(otp < 1000000,
                    "Default settings should produce 6-digit OTP: got %u",
                    otp);
  }

  /* 8-digit OTP (common alternative to 6-digit) */
  {
    const char *secret = "EIGHTDIGITTEST";
    fio_buf_info_s buf = FIO_BUF_INFO1((char *)secret);
    uint32_t otp = fio_otp(buf, .is_raw = 1, .digits = 8);
    OTP_TEST_ASSERT(otp < 100000000,
                    "8-digit OTP should be less than 100000000: got %u",
                    otp);
    FIO_LOG_DDEBUG("  8-digit OTP: %08u", otp);
  }

  FIO_LOG_DDEBUG("  Edge cases: PASSED");
}

/* *****************************************************************************
RFC 6238 Test Vector Verification
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_rfc6238)(void) {
  FIO_LOG_DDEBUG("Testing RFC 6238 test vectors");

  /*
   * RFC 6238 Appendix B specifies:
   * - Secret: ASCII "12345678901234567890" (20 bytes for SHA-1)
   * - Time step: 30 seconds
   * - Digits: 8
   */
  fio_buf_info_s secret =
      FIO_BUF_INFO2((char *)RFC6238_SECRET_RAW, RFC6238_SECRET_LEN);

  for (size_t i = 0; i < RFC6238_VECTOR_COUNT; ++i) {
    uint64_t test_time = rfc6238_vectors[i].unix_time;
    uint32_t expected = rfc6238_vectors[i].expected_totp;

    uint32_t actual =
        fio_otp_at(secret, test_time, .is_raw = 1, .interval = 30, .digits = 8);

    OTP_TEST_ASSERT(actual == expected,
                    "RFC 6238 test vector failed at time %llu: expected %08u, "
                    "got %08u",
                    (unsigned long long)test_time,
                    expected,
                    actual);

    if (actual == expected) {
      FIO_LOG_DDEBUG("  Time %llu: %08u PASSED",
                     (unsigned long long)test_time,
                     actual);
    } else {
      FIO_LOG_DDEBUG("  Time %llu: expected %08u, got %08u FAILED",
                     (unsigned long long)test_time,
                     expected,
                     actual);
    }
  }
}

/* *****************************************************************************
Google Authenticator Compatibility Test
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, otp_google_auth_compat)(void) {
  FIO_LOG_DDEBUG("Testing Google Authenticator compatibility");

  /*
   * Google Authenticator uses:
   * - TOTP (RFC 6238)
   * - SHA-1 HMAC
   * - 30-second intervals
   * - 6-digit codes
   * - Base32 encoded secrets (no padding)
   *
   * To verify: use a known Base32 secret in Google Authenticator
   * and compare the generated codes.
   */

  /* Example Base32 secret that could be used in Google Authenticator */
  const char *ga_secret = "JBSWY3DPEHPK3PXP"; /* Example secret */
  fio_buf_info_s secret_buf = FIO_BUF_INFO1((char *)ga_secret);

  uint32_t otp = fio_otp(secret_buf,
                         .is_raw = 0,    /* Base32 encoded */
                         .is_hex = 0,    /* Not hex */
                         .interval = 30, /* 30 second window */
                         .digits = 6,    /* 6 digits */
                         .offset = 0);   /* Current time */

  OTP_TEST_ASSERT(otp < 1000000,
                  "Google Auth compatible OTP should be 6 digits");
  FIO_LOG_DDEBUG("  Current GA-compatible OTP for '%s': %06u", ga_secret, otp);

  /* Also test previous and next windows (common for verification) */
  uint32_t otp_prev =
      fio_otp(secret_buf, .interval = 30, .digits = 6, .offset = 1);
  uint32_t otp_next =
      fio_otp(secret_buf, .interval = 30, .digits = 6, .offset = -1);
  (void)otp_prev; /* Used only in debug output */
  (void)otp_next; /* Used only in debug output */

  FIO_LOG_DDEBUG("  Window -1: %06u, Current: %06u, Window +1: %06u",
                 otp_prev,
                 otp,
                 otp_next);

  FIO_LOG_DDEBUG("  Google Authenticator compatibility: PASSED");
}

/* *****************************************************************************
Main Test Runner
***************************************************************************** */

int main(void) {
  FIO_LOG_DDEBUG("Testing OTP (One-Time Password) Module");

  FIO_NAME_TEST(stl, otp_key_generation)();
  FIO_NAME_TEST(stl, otp_key_printing)();
  FIO_NAME_TEST(stl, otp_basic)();
  FIO_NAME_TEST(stl, otp_time_offset)();
  FIO_NAME_TEST(stl, otp_interval)();
  FIO_NAME_TEST(stl, otp_base32)();
  FIO_NAME_TEST(stl, otp_hex)();
  FIO_NAME_TEST(stl, otp_consistency)();
  FIO_NAME_TEST(stl, otp_edge_cases)();
  FIO_NAME_TEST(stl, otp_rfc6238)();
  FIO_NAME_TEST(stl, otp_google_auth_compat)();

  size_t total = OTP_TEST_RESULTS.passed + OTP_TEST_RESULTS.failed;
  (void)total; /* Used only in debug output */
  FIO_LOG_DDEBUG("OTP tests: %zu passed, %zu failed (out of %zu)",
                 OTP_TEST_RESULTS.passed,
                 OTP_TEST_RESULTS.failed,
                 total);

  if (OTP_TEST_RESULTS.failed) {
    FIO_LOG_DDEBUG("OTP tests FAILED!");
  } else {
    FIO_LOG_DDEBUG("OTP tests completed successfully!");
  }

  return (int)OTP_TEST_RESULTS.failed;
}
