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

  /* unpadded output fits `len * 2 + 1` bytes and round-trips for any len */
  for (size_t l = 1; l <= 21; ++l) {
    uint8_t raw[21];
    char out[64];
    for (size_t i = 0; i < l; ++i)
      raw[i] = (uint8_t)(i * 37 + l);
    memset(out, 0x7F, sizeof(out));
    size_t ol = fio_otp_print_key(out, raw, l);
    OTP_TEST_ASSERT(ol == (l * 8 + 4) / 5 && !out[ol] && out[l * 2 + 1] == 0x7F,
                    "print_key length / bounds for %zu bytes", l);
    for (size_t i = 0; i < ol; ++i)
      OTP_TEST_ASSERT(out[i] != '=', "print_key should not pad");
    OTP_TEST_ASSERT(fio_otp_at(FIO_BUF_INFO2(out, ol), 59, .is_raw = 0) ==
                        fio_otp_at(FIO_BUF_INFO2((char *)raw, l), 59, .is_raw = 1),
                    "print_key output should round-trip (%zu bytes)", l);
  }
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

FIO_SFUNC void FIO_NAME_TEST(stl, otp_errors)(void) {
  fio_buf_info_s rfc =
      FIO_BUF_INFO2((char *)rfc6238_secret, rfc6238_secret_len);
  /* 10 digits: full 31 bit truncated value (RFC 4226 counter 1) */
  OTP_TEST_ASSERT(fio_otp_at(rfc, 59, .is_raw = 1, .digits = 10) ==
                      1094287082UL,
                  "10-digit OTP should be the full truncated value");
  for (size_t d = 11; d < 70; d += 29)
    OTP_TEST_ASSERT(fio_otp_at(rfc, 59, .is_raw = 1, .digits = d) ==
                        FIO_OTP_ERROR,
                    "%zu digits should be rejected", d);
  OTP_TEST_ASSERT(fio_otp_at(rfc, 59, .is_raw = 1, .digits = SIZE_MAX) ==
                      FIO_OTP_ERROR,
                  "SIZE_MAX digits should be rejected");
  /* extreme offsets wrap (defined), never crash */
  (void)fio_otp_at(rfc, 59, .is_raw = 1, .offset = INT64_MIN);
  (void)fio_otp_at(rfc, 59, .is_raw = 1, .offset = INT64_MAX,
                   .interval = SIZE_MAX);

  /* RFC 2104: keys longer than 64 bytes are replaced by SHA1(key) */
  {
    char long_key[100];
    for (size_t i = 0; i < sizeof(long_key); ++i)
      long_key[i] = (char)i;
    fio_sha1_s h = fio_sha1(long_key, sizeof(long_key));
    OTP_TEST_ASSERT(
        fio_otp_at(FIO_BUF_INFO2(long_key, sizeof(long_key)), 59, .is_raw = 1) ==
            fio_otp_at(FIO_BUF_INFO2((char *)fio_sha1_digest(&h), 20),
                       59,
                       .is_raw = 1),
        "long raw keys should follow RFC 2104 HMAC key hashing");
    char hex[2 * sizeof(long_key) + 1];
    for (size_t i = 0; i < sizeof(long_key); ++i) {
      hex[i * 2] = "0123456789ABCDEF"[(uint8_t)long_key[i] >> 4];
      hex[i * 2 + 1] = "0123456789ABCDEF"[(uint8_t)long_key[i] & 15];
    }
    OTP_TEST_ASSERT(
        fio_otp_at(FIO_BUF_INFO2(hex, sizeof(long_key) * 2), 59, .is_hex = 1) ==
            fio_otp_at(FIO_BUF_INFO2(long_key, sizeof(long_key)),
                       59,
                       .is_raw = 1),
        "long hex keys should match raw keys");
  }

  /* malformed hex is rejected, not aliased ("0G" != "10") */
  OTP_TEST_ASSERT(fio_otp_at(FIO_BUF_INFO1((char *)"0G"), 59, .is_hex = 1) ==
                      FIO_OTP_ERROR,
                  "invalid hex digits should be rejected");
  {
    char hex[258];
    memset(hex, 'A', sizeof(hex));
    OTP_TEST_ASSERT(fio_otp_at(FIO_BUF_INFO2(hex, 256), 59, .is_hex = 1) !=
                        FIO_OTP_ERROR,
                    "256 hex nibbles (128 bytes) should be accepted");
    OTP_TEST_ASSERT(fio_otp_at(FIO_BUF_INFO2(hex, 257), 59, .is_hex = 1) ==
                        FIO_OTP_ERROR,
                    "hex keys over 128 bytes should be rejected");
  }

  /* Base32: junk suffix rejected, padding / whitespace / lower case fine */
  {
    uint32_t ok = fio_otp_at(FIO_BUF_INFO1((char *)"JBSWY3DPEHPK3PXP"), 59, .is_hex = 0);
    OTP_TEST_ASSERT(ok != FIO_OTP_ERROR, "valid Base32 should be accepted");
    OTP_TEST_ASSERT(
        fio_otp_at(FIO_BUF_INFO1((char *)"jbsw y3dp ehpk 3pxp\n"), 59, .is_hex = 0) == ok,
        "lower case / whitespace Base32 should be accepted");
    OTP_TEST_ASSERT(
        fio_otp_at(FIO_BUF_INFO1((char *)"JBSWY3DPEHPK3PXP!"), 59, .is_hex = 0) ==
            FIO_OTP_ERROR,
        "Base32 with a junk suffix should be rejected");
    OTP_TEST_ASSERT(
        fio_otp_at(FIO_BUF_INFO1((char *)"JBSWY3DP=EHPK3PXP"), 59, .is_hex = 0) ==
            FIO_OTP_ERROR,
        "Base32 data after padding should be rejected");
    OTP_TEST_ASSERT(fio_otp_at(FIO_BUF_INFO1((char *)"JBSWY3DPEE======"), 59, .is_hex = 0) !=
                        FIO_OTP_ERROR,
                    "padded Base32 should be accepted");
    /* lenient: separators, extra / missing padding, trailing bits */
    OTP_TEST_ASSERT(
        fio_otp_at(FIO_BUF_INFO1((char *)"JBSW-Y3DP\vEHPK:3PXP\f=="), 59, .is_hex = 0) ==
            ok,
        "Base32 separators and extra padding should be ignored");
    uint32_t ee = fio_otp_at(FIO_BUF_INFO1((char *)"JBSWY3DPEE"), 59, .is_hex = 0);
    OTP_TEST_ASSERT(
        ee != FIO_OTP_ERROR &&
            fio_otp_at(FIO_BUF_INFO1((char *)"JBSWY3DPEE======"), 59, .is_hex = 0) == ee &&
            fio_otp_at(FIO_BUF_INFO1((char *)"JBSWY3DPEF="), 59, .is_hex = 0) == ee,
        "Base32 padding is optional and trailing bits are ignored");
    OTP_TEST_ASSERT(
        fio_otp_at(FIO_BUF_INFO1((char *)"========"), 59, .is_hex = 0) ==
            fio_otp_at(FIO_BUF_INFO2(NULL, 0), 59, .is_raw = 1),
        "padding-only Base32 should be an empty key");
    OTP_TEST_ASSERT(
        fio_otp_at(FIO_BUF_INFO1((char *)"JBSWY3DP_EHPK3PXP"), 59, .is_hex = 0) ==
            FIO_OTP_ERROR,
        "non-Base32 characters should be rejected");
    char b32[207];
    memset(b32, 'A', sizeof(b32));
    OTP_TEST_ASSERT(fio_otp_at(FIO_BUF_INFO2(b32, 205), 59, .is_hex = 0) !=
                        FIO_OTP_ERROR,
                    "205 Base32 chars (128 bytes) should be accepted");
    OTP_TEST_ASSERT(fio_otp_at(FIO_BUF_INFO2(b32, 207), 59, .is_hex = 0) ==
                        FIO_OTP_ERROR,
                    "Base32 keys over 128 bytes should be rejected");
  }
  OTP_TEST_ASSERT(
      fio_otp_at(FIO_BUF_INFO1((char *)"de:ad\tbe-ef"), 59, .is_hex = 1) ==
          fio_otp_at(FIO_BUF_INFO1((char *)"DEADBEEF"), 59, .is_hex = 1),
      "Hex separators / lower case should be accepted");
  OTP_TEST_ASSERT(fio_otp_at(FIO_BUF_INFO2(NULL, 4), 59, .is_raw = 1) ==
                      FIO_OTP_ERROR,
                  "NULL secret with a length should be rejected");

  /* generated keys need 33 bytes regardless of `len` */
  {
    char dest[40];
    memset(dest, 0x7F, sizeof(dest));
    size_t len = fio_otp_print_key(dest, NULL, 0);
    OTP_TEST_ASSERT(len > 0 && len < 33 && !dest[len] && dest[33] == 0x7F,
                    "generated key should fit 33 bytes");
    OTP_TEST_ASSERT(fio_otp_print_key(NULL, NULL, 0) == 0,
                    "NULL destination should fail");
  }
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
  FIO_NAME_TEST(stl, otp_errors)();
  return (int)OTP_TEST_RESULTS.failed;
}
