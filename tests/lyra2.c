/* *****************************************************************************
Test for 159 lyra2.h

Coverage: Lyra2 basic non-zero output, determinism, password/salt sensitivity,
and self-generated reference test vectors verified against the official C
reference implementation. Performance loops and OpenSSL comparisons are
intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_LYRA2
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

FIO_SFUNC void FIO_NAME_TEST(stl, lyra2_basic)(void) {
  fio_u512 r = fio_lyra2(.password = FIO_BUF_INFO2((char *)"password", 8),
                         .salt = FIO_BUF_INFO2((char *)"salt", 4),
                         .t_cost = 1,
                         .m_cost = 3,
                         .n_cols = 4,
                         .outlen = 32);
  int zero = 1;
  for (size_t i = 0; i < 32; ++i)
    if (r.u8[i] != 0)
      zero = 0;
  FIO_ASSERT(!zero, "Lyra2 output should not be all zeros");
}

FIO_SFUNC void FIO_NAME_TEST(stl, lyra2_determinism)(void) {
  fio_u512 r1 = fio_lyra2(.password = FIO_BUF_INFO2((char *)"test_password", 13),
                          .salt = FIO_BUF_INFO2((char *)"test_salt", 9),
                          .t_cost = 1,
                          .m_cost = 4,
                          .n_cols = 4,
                          .outlen = 32);
  fio_u512 r2 = fio_lyra2(.password = FIO_BUF_INFO2((char *)"test_password", 13),
                          .salt = FIO_BUF_INFO2((char *)"test_salt", 9),
                          .t_cost = 1,
                          .m_cost = 4,
                          .n_cols = 4,
                          .outlen = 32);
  FIO_ASSERT(!FIO_MEMCMP(r1.u8, r2.u8, 32), "Lyra2 should be deterministic");
}

FIO_SFUNC void FIO_NAME_TEST(stl, lyra2_different_passwords)(void) {
  fio_u512 r1 = fio_lyra2(.password = FIO_BUF_INFO2((char *)"password1", 9),
                          .salt = FIO_BUF_INFO2((char *)"salt", 4),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  fio_u512 r2 = fio_lyra2(.password = FIO_BUF_INFO2((char *)"password2", 9),
                          .salt = FIO_BUF_INFO2((char *)"salt", 4),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  FIO_ASSERT(FIO_MEMCMP(r1.u8, r2.u8, 32) != 0,
             "Different passwords should produce different hashes");
}

FIO_SFUNC void FIO_NAME_TEST(stl, lyra2_different_salts)(void) {
  fio_u512 r1 = fio_lyra2(.password = FIO_BUF_INFO2((char *)"password", 8),
                          .salt = FIO_BUF_INFO2((char *)"salt1", 5),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  fio_u512 r2 = fio_lyra2(.password = FIO_BUF_INFO2((char *)"password", 8),
                          .salt = FIO_BUF_INFO2((char *)"salt2", 5),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  FIO_ASSERT(FIO_MEMCMP(r1.u8, r2.u8, 32) != 0,
             "Different salts should produce different hashes");
}

/* Reference vectors verified against the official C implementation at
 * github.com/leocalm/Lyra (nPARALLEL=1, SPONGE=0/Blake2b, RHO=1). */
FIO_SFUNC void FIO_NAME_TEST(stl, lyra2_vectors)(void) {
  struct {
    const char *pwd;
    size_t pwd_len;
    const char *salt;
    size_t salt_len;
    uint64_t t_cost;
    uint64_t m_cost;
    size_t n_cols;
    size_t outlen;
    const char *expected_hex;
  } vectors[] = {
      {"abc", 3, "abc", 3, 1, 4, 4, 32,
       "6e5995eee68c2dcb7322d500460082ccdf159bb4c0a7b94e8c3325b456fcaeda"},
      {NULL, 0, NULL, 0, 1, 3, 4, 48,
       "d34c7266d7243544c85fabe32d9b962062ba8951c96c5d46570c9ff241ffb64e"
       "adf55dc49659a4053979dd3f509f58fd"},
      {NULL, 0, NULL, 0, 1, 4, 2, 16,
       "dd3ed5336edb96d38f43c037616973fb"},
  };

  /* UTF-8 "脇山珠美ちゃんかわいい！" (36 bytes). */
  static const char pwd_utf8[] =
      "\xe8\x84\x87\xe5\xb1\xb1\xe7\x8f\xa0\xe7\xbe\x8e\xe3\x81\xa1"
      "\xe3\x82\x83\xe3\x82\x93\xe3\x81\x8b\xe3\x82\x8f\xe3\x81\x84"
      "\xe3\x81\x84\xef\xbc\x81";
  /* UTF-8 "😀😁😂" (12 bytes). */
  static const char pwd_emoji[] =
      "\xf0\x9f\x98\x80\xf0\x9f\x98\x81\xf0\x9f\x98\x82";

  vectors[1].pwd = pwd_utf8;
  vectors[1].pwd_len = sizeof(pwd_utf8) - 1;
  vectors[1].salt = pwd_utf8;
  vectors[1].salt_len = sizeof(pwd_utf8) - 1;
  vectors[2].pwd = pwd_emoji;
  vectors[2].pwd_len = sizeof(pwd_emoji) - 1;
  vectors[2].salt = pwd_emoji;
  vectors[2].salt_len = sizeof(pwd_emoji) - 1;

  for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
    uint8_t expected[48];
    uint8_t out[48];
    fio___test_hex2bin(expected, vectors[i].expected_hex, vectors[i].outlen);

    int rc = fio_lyra2_hash(out,
                            .password = FIO_BUF_INFO2(
                                (char *)vectors[i].pwd, vectors[i].pwd_len),
                            .salt = FIO_BUF_INFO2(
                                (char *)vectors[i].salt, vectors[i].salt_len),
                            .t_cost = vectors[i].t_cost,
                            .m_cost = vectors[i].m_cost,
                            .n_cols = vectors[i].n_cols,
                            .outlen = vectors[i].outlen);
    FIO_ASSERT(rc == 0, "fio_lyra2_hash vector %zu failed", i + 1);

    if (FIO_MEMCMP(out, expected, vectors[i].outlen) != 0) {
      fio___test_print_hex("expected", expected, vectors[i].outlen);
      fio___test_print_hex("got     ", out, vectors[i].outlen);
    }
    FIO_ASSERT(!FIO_MEMCMP(out, expected, vectors[i].outlen),
               "Lyra2 vector %zu mismatch", i + 1);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, lyra2_long_output)(void) {
  uint8_t out[128];
  int rc = fio_lyra2_hash(out,
                          .password = FIO_BUF_INFO2((char *)"password", 8),
                          .salt = FIO_BUF_INFO2((char *)"salt", 4),
                          .t_cost = 1,
                          .m_cost = 4,
                          .n_cols = 4,
                          .outlen = 128);
  FIO_ASSERT(rc == 0, "fio_lyra2_hash with outlen=128 should return 0");
  int zero = 1;
  for (size_t i = 0; i < 128; ++i)
    if (out[i] != 0)
      zero = 0;
  FIO_ASSERT(!zero, "Lyra2 long output should not be all zeros");
}

FIO_SFUNC void FIO_NAME_TEST(stl, lyra2_long_output_determinism)(void) {
  uint8_t out1[128], out2[128];
  int rc1 = fio_lyra2_hash(out1,
                           .password = FIO_BUF_INFO2((char *)"password", 8),
                           .salt = FIO_BUF_INFO2((char *)"salt", 4),
                           .t_cost = 1,
                           .m_cost = 4,
                           .n_cols = 4,
                           .outlen = 128);
  int rc2 = fio_lyra2_hash(out2,
                           .password = FIO_BUF_INFO2((char *)"password", 8),
                           .salt = FIO_BUF_INFO2((char *)"salt", 4),
                           .t_cost = 1,
                           .m_cost = 4,
                           .n_cols = 4,
                           .outlen = 128);
  FIO_ASSERT(rc1 == 0 && rc2 == 0,
             "fio_lyra2_hash long output should return 0");
  FIO_ASSERT(!FIO_MEMCMP(out1, out2, 128),
             "Lyra2 long output should be deterministic");
}

FIO_SFUNC void FIO_NAME_TEST(stl, lyra2_empty_pwd_salt)(void) {
  fio_u512 r1 = fio_lyra2(.password = FIO_BUF_INFO2(NULL, 0),
                          .salt = FIO_BUF_INFO2(NULL, 0),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  fio_u512 r2 = fio_lyra2(.password = FIO_BUF_INFO2(NULL, 0),
                          .salt = FIO_BUF_INFO2(NULL, 0),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  int zero = 1;
  for (size_t i = 0; i < 32; ++i)
    if (r1.u8[i] != 0)
      zero = 0;
  FIO_ASSERT(!zero, "Lyra2 empty password/salt output should not be zeros");
  FIO_ASSERT(!FIO_MEMCMP(r1.u8, r2.u8, 32),
             "Lyra2 empty password/salt should be deterministic");
}

FIO_SFUNC void FIO_NAME_TEST(stl, lyra2_null_out)(void) {
  int rc = fio_lyra2_hash(NULL,
                          .password = FIO_BUF_INFO2((char *)"password", 8),
                          .salt = FIO_BUF_INFO2((char *)"salt", 4),
                          .t_cost = 1,
                          .m_cost = 4,
                          .n_cols = 4,
                          .outlen = 32);
  FIO_ASSERT(rc == -1, "fio_lyra2_hash should reject NULL output buffer");
}

int main(void) {
  FIO_NAME_TEST(stl, lyra2_basic)();
  FIO_NAME_TEST(stl, lyra2_determinism)();
  FIO_NAME_TEST(stl, lyra2_different_passwords)();
  FIO_NAME_TEST(stl, lyra2_different_salts)();
  FIO_NAME_TEST(stl, lyra2_vectors)();
  FIO_NAME_TEST(stl, lyra2_long_output)();
  FIO_NAME_TEST(stl, lyra2_long_output_determinism)();
  FIO_NAME_TEST(stl, lyra2_empty_pwd_salt)();
  FIO_NAME_TEST(stl, lyra2_null_out)();
  return 0;
}
