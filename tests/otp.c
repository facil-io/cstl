/* *****************************************************************************
Test for 160 otp.h

Coverage: OTP key generation, key printing/Base32 encoding, basic TOTP
functionality, time offsets, intervals, Base32 and hex secret decoding,
consistency, edge cases, and RFC 6238 Appendix B SHA-1 test vectors.
Performance loops are intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_OTP
#include FIO_INCLUDE_FILE

static struct {
  size_t passed;
  size_t failed;
} OTP_TEST_RESULTS;

#define OTP_TEST_ASSERT(cond, ...)                                             \
  do {                                                                         \
    if (!(cond)) {                                                             \
      ++OTP_TEST_RESULTS.failed;                                               \
      FIO_LOG_ERROR(__VA_ARGS__);                                              \
    } else {                                                                   \
      ++OTP_TEST_RESULTS.passed;                                               \
    }                                                                          \
  } while (0)

/* RFC 6238 Appendix B test vectors (SHA-1, 8-digit, 30-second interval). */
static const char *rfc6238_secret = "12345678901234567890";
static const size_t rfc6238_secret_len = 20;

typedef struct {
  uint64_t unix_time;
  uint32_t expected_totp;
} rfc6238_test_vector_t;

static const rfc6238_test_vector_t rfc6238_vectors[] = {
    {.unix_time = 59, .expected_totp = 94287082},
    {.unix_time = 1111111109, .expected_totp = 7081804},
    {.unix_time = 1111111111, .expected_totp = 14050471},
    {.unix_time = 1234567890, .expected_totp = 89005924},
    {.unix_time = 2000000000, .expected_totp = 69279037},
    {.unix_time = 20000000000ULL, .expected_totp = 65353130},
};

/* RFC 4226 Appendix D HOTP test vectors (SHA-1, 6-digit). The same secret is
 * used with counters 0..9. We drive the counter through fio_otp_at() by
 * choosing time = counter * 30 with a 30-second interval. */
static const uint32_t rfc4226_expected[10] = {
    755224, 287082, 359152, 969429, 338314,
    254676, 287922, 162583, 399871, 520489};

FIO_SFUNC void FIO_NAME_TEST(stl, otp_key_generation)(void) {
  for (int i = 0; i < 10; ++i) {
    fio_u128 key = fio_otp_generate_key();
    OTP_TEST_ASSERT(key.u64[0] != 0 || key.u64[1] != 0,
                    "Generated OTP key should not be all zeros");
  }

  fio_u128 keys[32];
  for (int i = 0; i < 32; ++i)
    keys[i] = fio_otp_generate_key();
  for (int i = 0; i < 32; ++i) {
    for (int j = i + 1; j < 32; ++j) {
      OTP_TEST_ASSERT(keys[i].u64[0] != keys[j].u64[0] ||
                          keys[i].u64[1] != keys[j].u64[1],
                      "Generated OTP keys should be unique");
    }
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_key_printing)(void) {
  uint8_t test_key[10] = {0x48, 0x65, 0x6C, 0x6C, 0x6F,
                          0x21, 0xDE, 0xAD, 0xBE, 0xEF};
  char output[64] = {0};
  size_t len = fio_otp_print_key(output, test_key, 10);
  OTP_TEST_ASSERT(len > 0, "fio_otp_print_key should return non-zero length");
  OTP_TEST_ASSERT(output[0] != 0, "OTP key output should not be empty");
  /* Output should be a valid Base32 (RFC 4648) string. */
  for (size_t i = 0; i < len; ++i) {
    char c = output[i];
    OTP_TEST_ASSERT((c >= 'A' && c <= 'Z') || (c >= '2' && c <= '7'),
                    "OTP key output should be Base32 alphabet");
  }

  char output1[64] = {0};
  char output2[64] = {0};
  size_t len1 = fio_otp_print_key(output1, NULL, 16);
  size_t len2 = fio_otp_print_key(output2, NULL, 16);
  OTP_TEST_ASSERT(len1 > 0 && len2 > 0,
                  "Should generate keys when NULL passed");
  OTP_TEST_ASSERT(strcmp(output1, output2) != 0,
                  "Generated OTP keys should be different");
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_basic)(void) {
  fio_buf_info_s secret_buf =
      FIO_BUF_INFO2((char *)rfc6238_secret, rfc6238_secret_len);

  /* Use a fixed time so the comparison is deterministic. */
  uint32_t otp1 = fio_otp_at(secret_buf, 1234567890ULL,
                             .is_raw = 1, .interval = 30, .digits = 6);
  uint32_t otp2 = fio_otp_at(secret_buf, 1234567890ULL,
                             .is_raw = 1, .interval = 30, .digits = 6);
  OTP_TEST_ASSERT(otp1 == otp2,
                  "Same secret and time should produce same OTP");
  OTP_TEST_ASSERT(otp1 < 1000000, "6-digit OTP should be less than 1000000");

  for (size_t digits = 1; digits <= 9; ++digits) {
    uint32_t otp = fio_otp_at(FIO_BUF_INFO2((char *)"TESTSECRET123456", 16),
                              0ULL, .is_raw = 1, .interval = 30,
                              .digits = digits);
    uint32_t max_val = 1;
    for (size_t d = 0; d < digits; ++d)
      max_val *= 10;
    OTP_TEST_ASSERT(otp < max_val,
                    "%zu-digit OTP should be less than %u", digits, max_val);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_time_offset)(void) {
  fio_buf_info_s secret_buf =
      FIO_BUF_INFO2((char *)rfc6238_secret, rfc6238_secret_len);
  uint64_t base = 1234567890ULL;
  uint32_t otp_current = fio_otp_at(secret_buf, base,
                                    .is_raw = 1, .offset = 0);
  uint32_t otp_prev = fio_otp_at(secret_buf, base,
                                 .is_raw = 1, .offset = 1);
  uint32_t otp_next = fio_otp_at(secret_buf, base,
                                 .is_raw = 1, .offset = -1);
  OTP_TEST_ASSERT(otp_current < 1000000, "Current OTP out of range");
  OTP_TEST_ASSERT(otp_prev < 1000000, "Previous OTP out of range");
  OTP_TEST_ASSERT(otp_next < 1000000, "Next OTP out of range");
  OTP_TEST_ASSERT(otp_prev != otp_next,
                  "Previous and next OTPs should differ");
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_interval)(void) {
  fio_buf_info_s secret_buf =
      FIO_BUF_INFO2((char *)rfc6238_secret, rfc6238_secret_len);
  uint64_t t = 59ULL;
  uint32_t otp_30s = fio_otp_at(secret_buf, t,
                                .is_raw = 1, .interval = 30);
  uint32_t otp_60s = fio_otp_at(secret_buf, t,
                                .is_raw = 1, .interval = 60);
  uint32_t otp_default = fio_otp_at(secret_buf, t,
                                    .is_raw = 1, .interval = 0);
  OTP_TEST_ASSERT(otp_30s == otp_default,
                  "Zero interval should default to 30 seconds");
  OTP_TEST_ASSERT(otp_30s != otp_60s,
                  "Different intervals should produce different counters");
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_base32)(void) {
  uint8_t raw_secret[10] = {0x48, 0x65, 0x6C, 0x6C, 0x6F,
                            0x21, 0xDE, 0xAD, 0xBE, 0xEF};
  char base32_secret[64] = {0};
  fio_str_info_s b32 = FIO_STR_INFO3(base32_secret, 0, sizeof(base32_secret));
  fio_string_write_base32enc(&b32, NULL, raw_secret, 10);

  fio_buf_info_s raw_buf = FIO_BUF_INFO2((char *)raw_secret, 10);
  uint32_t otp_raw = fio_otp_at(raw_buf, 0ULL, .is_raw = 1);

  fio_buf_info_s b32_buf = FIO_BUF_INFO2(base32_secret, b32.len);
  uint32_t otp_b32 = fio_otp_at(b32_buf, 0ULL, .is_raw = 0, .is_hex = 0);

  OTP_TEST_ASSERT(otp_raw == otp_b32,
                  "Raw and Base32 encoded secrets should produce same OTP");
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_hex)(void) {
  uint8_t raw_secret[10] = {0x48, 0x65, 0x6C, 0x6C, 0x6F,
                            0x21, 0xDE, 0xAD, 0xBE, 0xEF};
  const char *hex_secret = "48656C6C6F21DEADBEEF";

  fio_buf_info_s raw_buf = FIO_BUF_INFO2((char *)raw_secret, 10);
  uint32_t otp_raw = fio_otp_at(raw_buf, 0ULL, .is_raw = 1);

  fio_buf_info_s hex_buf = FIO_BUF_INFO1((char *)hex_secret);
  uint32_t otp_hex = fio_otp_at(hex_buf, 0ULL, .is_hex = 1);
  OTP_TEST_ASSERT(otp_raw == otp_hex,
                  "Raw and Hex encoded secrets should produce same OTP");

  uint8_t raw_secret2[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  const char *hex_with_sep = "DE-AD-BE-EF";
  fio_buf_info_s raw_buf2 = FIO_BUF_INFO2((char *)raw_secret2, 4);
  uint32_t otp_raw2 = fio_otp_at(raw_buf2, 0ULL, .is_raw = 1);
  fio_buf_info_s hex_buf2 = FIO_BUF_INFO1((char *)hex_with_sep);
  uint32_t otp_hex2 = fio_otp_at(hex_buf2, 0ULL, .is_hex = 1);
  OTP_TEST_ASSERT(otp_raw2 == otp_hex2,
                  "Hex with separators should work");
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_consistency)(void) {
  fio_buf_info_s secret_buf = FIO_BUF_INFO1((char *)"CONSISTENCYTEST1234567890");
  uint32_t results[16];
  for (int i = 0; i < 16; ++i)
    results[i] = fio_otp_at(secret_buf, 0ULL,
                            .is_raw = 1, .digits = 6, .interval = 30);
  for (int i = 1; i < 16; ++i)
    OTP_TEST_ASSERT(results[0] == results[i], "OTP should be consistent");
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_edge_cases)(void) {
  fio_buf_info_s short_buf = FIO_BUF_INFO1((char *)"A");
  uint32_t otp = fio_otp(short_buf, .is_raw = 1);
  OTP_TEST_ASSERT(otp < 1000000, "Short secret should still produce valid OTP");

  fio_buf_info_s default_buf = FIO_BUF_INFO1((char *)"DEFAULTTEST");
  otp = fio_otp(default_buf, .is_raw = 1);
  OTP_TEST_ASSERT(otp < 1000000,
                  "Default settings should produce 6-digit OTP");

  otp = fio_otp(default_buf, .is_raw = 1, .digits = 8);
  OTP_TEST_ASSERT(otp < 100000000, "8-digit OTP should be less than 100000000");
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_rfc6238)(void) {
  fio_buf_info_s secret =
      FIO_BUF_INFO2((char *)rfc6238_secret, rfc6238_secret_len);
  for (size_t i = 0; i < sizeof(rfc6238_vectors) / sizeof(rfc6238_vectors[0]); ++i) {
    uint64_t test_time = rfc6238_vectors[i].unix_time;
    uint32_t expected = rfc6238_vectors[i].expected_totp;
    uint32_t actual = fio_otp_at(secret, test_time, .is_raw = 1,
                                 .interval = 30, .digits = 8);
    OTP_TEST_ASSERT(actual == expected,
                    "RFC 6238 vector failed at time %llu: expected %08u, got %08u",
                    (unsigned long long)test_time, expected, actual);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_rfc4226)(void) {
  fio_buf_info_s secret =
      FIO_BUF_INFO2((char *)rfc6238_secret, rfc6238_secret_len);
  for (size_t counter = 0; counter < 10; ++counter) {
    uint32_t expected = rfc4226_expected[counter];
    /* time = counter * 30 makes the TOTP counter equal to counter. */
    uint32_t actual = fio_otp_at(secret, counter * 30ULL,
                                 .is_raw = 1, .interval = 30, .digits = 6);
    OTP_TEST_ASSERT(actual == expected,
                    "RFC 4226 vector failed at counter %zu: expected %06u, got %06u",
                    counter, expected, actual);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, otp_google_auth_compat)(void) {
  const char *ga_secret = "JBSWY3DPEHPK3PXP";
  fio_buf_info_s secret_buf = FIO_BUF_INFO1((char *)ga_secret);
  uint32_t otp = fio_otp(secret_buf,
                         .is_raw = 0,
                         .is_hex = 0,
                         .interval = 30,
                         .digits = 6,
                         .offset = 0);
  OTP_TEST_ASSERT(otp < 1000000,
                  "Google Auth compatible OTP should be 6 digits");
}

int main(void) {
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
  FIO_NAME_TEST(stl, otp_rfc4226)();
  FIO_NAME_TEST(stl, otp_google_auth_compat)();
  return (int)OTP_TEST_RESULTS.failed;
}
