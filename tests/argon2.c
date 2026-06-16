/* *****************************************************************************
Test for 159 argon2.h

Coverage: Argon2d, Argon2i, and Argon2id RFC 9106 Section 5 known-answer
vectors, basic non-zero output, determinism, and type differentiation.
Performance loops are intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_ARGON2
#include FIO_INCLUDE_FILE

/* Helper: parse a hex string to bytes (no sscanf, validates every nibble). */
static void fio___test_hex2bin(uint8_t *out, const char *hex, size_t out_len) {
  for (size_t i = 0; i < out_len; ++i) {
    uint8_t hi = fio_c2i((unsigned char)hex[2 * i]);
    uint8_t lo = fio_c2i((unsigned char)hex[2 * i + 1]);
    FIO_ASSERT(hi < 16 && lo < 16, "invalid hex character in test vector");
    out[i] = (uint8_t)((hi << 4) | lo);
  }
}

static void fio___test_print_hex(const char *label,
                                 const uint8_t *data,
                                 size_t len) {
  fprintf(stderr, "    %s: ", label);
  for (size_t i = 0; i < len; ++i)
    fprintf(stderr, "%02x", data[i]);
  fprintf(stderr, "\n");
}

/* RFC 9106 Section 5 common inputs. */
static const uint8_t rfc_password[32] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
static const uint8_t rfc_salt[16] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                                     0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                                     0x02, 0x02, 0x02, 0x02};
static const uint8_t rfc_secret[8] =
    {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};
static const uint8_t rfc_ad[12] =
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};

FIO_SFUNC void FIO_NAME_TEST(stl, argon2_basic)(void) {
  fio_u512 r = fio_argon2(.password = FIO_BUF_INFO2((char *)"password", 8),
                          .salt = FIO_BUF_INFO2((char *)"salt", 4),
                          .t_cost = 1,
                          .m_cost = 64,
                          .parallelism = 1,
                          .outlen = 32,
                          .type = FIO_ARGON2ID);
  int zero = 1;
  for (size_t i = 0; i < 32; ++i)
    if (r.u8[i] != 0)
      zero = 0;
  FIO_ASSERT(!zero, "Argon2 output should not be all zeros");
}

FIO_SFUNC void FIO_NAME_TEST(stl, argon2_determinism)(void) {
  fio_u512 r1 = fio_argon2(.password = FIO_BUF_INFO2((char *)"test", 4),
                           .salt = FIO_BUF_INFO2((char *)"saltsalt", 8),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2ID);
  fio_u512 r2 = fio_argon2(.password = FIO_BUF_INFO2((char *)"test", 4),
                           .salt = FIO_BUF_INFO2((char *)"saltsalt", 8),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2ID);
  FIO_ASSERT(!FIO_MEMCMP(r1.u8, r2.u8, 32), "Argon2 should be deterministic");
}

FIO_SFUNC void FIO_NAME_TEST(stl, argon2_types_differ)(void) {
  fio_u512 rd = fio_argon2(.password = FIO_BUF_INFO2((char *)"password", 8),
                           .salt = FIO_BUF_INFO2((char *)"saltsalt", 8),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2D);
  fio_u512 ri = fio_argon2(.password = FIO_BUF_INFO2((char *)"password", 8),
                           .salt = FIO_BUF_INFO2((char *)"saltsalt", 8),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2I);
  fio_u512 rid = fio_argon2(.password = FIO_BUF_INFO2((char *)"password", 8),
                            .salt = FIO_BUF_INFO2((char *)"saltsalt", 8),
                            .t_cost = 1,
                            .m_cost = 64,
                            .parallelism = 1,
                            .outlen = 32,
                            .type = FIO_ARGON2ID);
  FIO_ASSERT(FIO_MEMCMP(rd.u8, ri.u8, 32) != 0,
             "Argon2d and Argon2i should differ");
  FIO_ASSERT(FIO_MEMCMP(rd.u8, rid.u8, 32) != 0,
             "Argon2d and Argon2id should differ");
  FIO_ASSERT(FIO_MEMCMP(ri.u8, rid.u8, 32) != 0,
             "Argon2i and Argon2id should differ");
}

FIO_SFUNC void FIO_NAME_TEST(stl, argon2_rfc9106)(void) {
  struct {
    fio_argon2_type_e type;
    const char *name;
    const char *expected_hex;
  } vectors[] = {
      {FIO_ARGON2D,
       "Argon2d",
       "512b391b6f1162975371d30919734294f868e3be3984f3c1a13a4db9fabe4acb"},
      {FIO_ARGON2I,
       "Argon2i",
       "c814d9d1dc7f37aa13f0d77f2494bda1c8de6b016dd388d29952a4c4672b6ce8"},
      {FIO_ARGON2ID,
       "Argon2id",
       "0d640df58d78766c08c037a34a8b53c9d01ef0452d75b65eb52520e96b01e659"},
  };

  for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
    uint8_t expected[32];
    uint8_t tag[32];
    fio___test_hex2bin(expected, vectors[i].expected_hex, 32);

    int rc = fio_argon2_hash(tag,
                             .password = FIO_BUF_INFO2((char *)rfc_password, 32),
                             .salt = FIO_BUF_INFO2((char *)rfc_salt, 16),
                             .secret = FIO_BUF_INFO2((char *)rfc_secret, 8),
                             .ad = FIO_BUF_INFO2((char *)rfc_ad, 12),
                             .t_cost = 3,
                             .m_cost = 32,
                             .parallelism = 4,
                             .outlen = 32,
                             .type = vectors[i].type);
    FIO_ASSERT(rc == 0, "fio_argon2_hash (%s) should return 0",
               vectors[i].name);

    if (FIO_MEMCMP(tag, expected, 32) != 0) {
      fio___test_print_hex("expected", expected, 32);
      fio___test_print_hex("got     ", tag, 32);
    }
    FIO_ASSERT(!FIO_MEMCMP(tag, expected, 32),
               "Argon2 %s RFC 9106 test vector mismatch", vectors[i].name);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, argon2_long_output)(void) {
  uint8_t out[128];
  int rc = fio_argon2_hash(out,
                           .password = FIO_BUF_INFO2((char *)"password", 8),
                           .salt = FIO_BUF_INFO2((char *)"salt", 4),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 128,
                           .type = FIO_ARGON2ID);
  FIO_ASSERT(rc == 0, "fio_argon2_hash with outlen=128 should return 0");
  int zero = 1;
  for (size_t i = 0; i < 128; ++i)
    if (out[i] != 0)
      zero = 0;
  FIO_ASSERT(!zero, "Argon2 long output should not be all zeros");
}

FIO_SFUNC void FIO_NAME_TEST(stl, argon2_long_output_determinism)(void) {
  uint8_t out1[128], out2[128];
  int rc1 = fio_argon2_hash(out1,
                            .password = FIO_BUF_INFO2((char *)"password", 8),
                            .salt = FIO_BUF_INFO2((char *)"salt", 4),
                            .t_cost = 1,
                            .m_cost = 64,
                            .parallelism = 1,
                            .outlen = 128,
                            .type = FIO_ARGON2ID);
  int rc2 = fio_argon2_hash(out2,
                            .password = FIO_BUF_INFO2((char *)"password", 8),
                            .salt = FIO_BUF_INFO2((char *)"salt", 4),
                            .t_cost = 1,
                            .m_cost = 64,
                            .parallelism = 1,
                            .outlen = 128,
                            .type = FIO_ARGON2ID);
  FIO_ASSERT(rc1 == 0 && rc2 == 0,
             "fio_argon2_hash long output should return 0");
  FIO_ASSERT(!FIO_MEMCMP(out1, out2, 128),
             "Argon2 long output should be deterministic");
}

FIO_SFUNC void FIO_NAME_TEST(stl, argon2_empty_optional)(void) {
  fio_u512 r1 = fio_argon2(.password = FIO_BUF_INFO2((char *)"password", 8),
                           .salt = FIO_BUF_INFO2((char *)"salt", 4),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2ID);
  fio_u512 r2 = fio_argon2(.password = FIO_BUF_INFO2((char *)"password", 8),
                           .salt = FIO_BUF_INFO2((char *)"salt", 4),
                           .secret = FIO_BUF_INFO2(NULL, 0),
                           .ad = FIO_BUF_INFO2(NULL, 0),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2ID);
  FIO_ASSERT(!FIO_MEMCMP(r1.u8, r2.u8, 32),
             "Argon2 with empty optional params should match omitted params");
}

FIO_SFUNC void FIO_NAME_TEST(stl, argon2_null_out)(void) {
  int rc = fio_argon2_hash(NULL,
                           .password = FIO_BUF_INFO2((char *)"password", 8),
                           .salt = FIO_BUF_INFO2((char *)"salt", 4),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2ID);
  FIO_ASSERT(rc == -1, "fio_argon2_hash should reject NULL output buffer");
}

int main(void) {
  FIO_NAME_TEST(stl, argon2_basic)();
  FIO_NAME_TEST(stl, argon2_determinism)();
  FIO_NAME_TEST(stl, argon2_types_differ)();
  FIO_NAME_TEST(stl, argon2_rfc9106)();
  FIO_NAME_TEST(stl, argon2_long_output)();
  FIO_NAME_TEST(stl, argon2_long_output_determinism)();
  FIO_NAME_TEST(stl, argon2_empty_optional)();
  FIO_NAME_TEST(stl, argon2_null_out)();
  return 0;
}
