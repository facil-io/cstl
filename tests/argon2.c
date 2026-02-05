/* *****************************************************************************
Argon2 Tests - RFC 9106 test vectors (Section 5)
***************************************************************************** */
#include "test-helpers.h"
#define FIO_LOG
#define FIO_ARGON2
#include FIO_INCLUDE_FILE

/* Helper: parse hex string to bytes */
static void fio___test_hex2bin(uint8_t *out, const char *hex, size_t out_len) {
  for (size_t i = 0; i < out_len; ++i) {
    unsigned int byte;
    sscanf(hex + 2 * i, "%02x", &byte);
    out[i] = (uint8_t)byte;
  }
}

/* Helper: print hex for debugging */
static void fio___test_print_hex(const char *label,
                                 const uint8_t *data,
                                 size_t len) {
  fprintf(stderr, "    %s: ", label);
  for (size_t i = 0; i < len; ++i)
    fprintf(stderr, "%02x", data[i]);
  fprintf(stderr, "\n");
}

/* *****************************************************************************
RFC 9106 Section 5 Test Vectors
All three variants use the same inputs:
  Password[32]:  01 01 01 01 ... (32 bytes of 0x01)
  Salt[16]:      02 02 02 02 ... (16 bytes of 0x02)
  Secret[8]:     03 03 03 03 03 03 03 03
  AD[12]:        04 04 04 04 04 04 04 04 04 04 04 04
  Parallelism:   4 lanes
  Tag length:    32 bytes
  Memory:        32 KiB
  Passes:        3
***************************************************************************** */

static const uint8_t rfc_password[32] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
static const uint8_t rfc_salt[16] = {0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02,
                                     0x02};
static const uint8_t rfc_secret[8] =
    {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};
static const uint8_t rfc_ad[12] =
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};

/* Section 5.1: Argon2d tag */
static void fio___test_argon2d_rfc(void) {
  const char *expected_hex =
      "512b391b6f1162975371d30919734294f868e3be3984f3c1a13a4db9fabe4acb";
  uint8_t expected[32];
  fio___test_hex2bin(expected, expected_hex, 32);

  uint8_t tag[32];
  int rc = fio_argon2_hash(tag,
                           .password = FIO_BUF_INFO2((char *)rfc_password, 32),
                           .salt = FIO_BUF_INFO2((char *)rfc_salt, 16),
                           .secret = FIO_BUF_INFO2((char *)rfc_secret, 8),
                           .ad = FIO_BUF_INFO2((char *)rfc_ad, 12),
                           .t_cost = 3,
                           .m_cost = 32,
                           .parallelism = 4,
                           .outlen = 32,
                           .type = FIO_ARGON2D);
  FIO_ASSERT(rc == 0, "fio_argon2_hash (Argon2d) should return 0");

  if (FIO_MEMCMP(tag, expected, 32) != 0) {
    fio___test_print_hex("expected", expected, 32);
    fio___test_print_hex("got     ", tag, 32);
  }
  FIO_ASSERT(!FIO_MEMCMP(tag, expected, 32),
             "Argon2d RFC 9106 test vector mismatch");
  fprintf(stderr, "    * Argon2d RFC 9106 test vector: PASSED\n");
}

/* Section 5.2: Argon2i tag */
static void fio___test_argon2i_rfc(void) {
  const char *expected_hex =
      "c814d9d1dc7f37aa13f0d77f2494bda1c8de6b016dd388d29952a4c4672b6ce8";
  uint8_t expected[32];
  fio___test_hex2bin(expected, expected_hex, 32);

  uint8_t tag[32];
  int rc = fio_argon2_hash(tag,
                           .password = FIO_BUF_INFO2((char *)rfc_password, 32),
                           .salt = FIO_BUF_INFO2((char *)rfc_salt, 16),
                           .secret = FIO_BUF_INFO2((char *)rfc_secret, 8),
                           .ad = FIO_BUF_INFO2((char *)rfc_ad, 12),
                           .t_cost = 3,
                           .m_cost = 32,
                           .parallelism = 4,
                           .outlen = 32,
                           .type = FIO_ARGON2I);
  FIO_ASSERT(rc == 0, "fio_argon2_hash (Argon2i) should return 0");

  if (FIO_MEMCMP(tag, expected, 32) != 0) {
    fio___test_print_hex("expected", expected, 32);
    fio___test_print_hex("got     ", tag, 32);
  }
  FIO_ASSERT(!FIO_MEMCMP(tag, expected, 32),
             "Argon2i RFC 9106 test vector mismatch");
  fprintf(stderr, "    * Argon2i RFC 9106 test vector: PASSED\n");
}

/* Section 5.3: Argon2id tag */
static void fio___test_argon2id_rfc(void) {
  const char *expected_hex =
      "0d640df58d78766c08c037a34a8b53c9d01ef0452d75b65eb52520e96b01e659";
  uint8_t expected[32];
  fio___test_hex2bin(expected, expected_hex, 32);

  uint8_t tag[32];
  int rc = fio_argon2_hash(tag,
                           .password = FIO_BUF_INFO2((char *)rfc_password, 32),
                           .salt = FIO_BUF_INFO2((char *)rfc_salt, 16),
                           .secret = FIO_BUF_INFO2((char *)rfc_secret, 8),
                           .ad = FIO_BUF_INFO2((char *)rfc_ad, 12),
                           .t_cost = 3,
                           .m_cost = 32,
                           .parallelism = 4,
                           .outlen = 32,
                           .type = FIO_ARGON2ID);
  FIO_ASSERT(rc == 0, "fio_argon2_hash (Argon2id) should return 0");

  if (FIO_MEMCMP(tag, expected, 32) != 0) {
    fio___test_print_hex("expected", expected, 32);
    fio___test_print_hex("got     ", tag, 32);
  }
  FIO_ASSERT(!FIO_MEMCMP(tag, expected, 32),
             "Argon2id RFC 9106 test vector mismatch");
  fprintf(stderr, "    * Argon2id RFC 9106 test vector: PASSED\n");
}

/* Test: basic invocation does not crash and produces non-zero output */
static void fio___test_argon2_basic(void) {
  fio_u512 r = fio_argon2(.password = FIO_BUF_INFO2("password", 8),
                          .salt = FIO_BUF_INFO2("salt", 4),
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
  fprintf(stderr, "    * Argon2 basic test: PASSED\n");
}

/* Test: determinism - same inputs produce same output */
static void fio___test_argon2_determinism(void) {
  fio_u512 r1 = fio_argon2(.password = FIO_BUF_INFO2("test", 4),
                           .salt = FIO_BUF_INFO2("saltsalt", 8),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2ID);
  fio_u512 r2 = fio_argon2(.password = FIO_BUF_INFO2("test", 4),
                           .salt = FIO_BUF_INFO2("saltsalt", 8),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2ID);
  FIO_ASSERT(!FIO_MEMCMP(r1.u8, r2.u8, 32), "Argon2 should be deterministic");
  fprintf(stderr, "    * Argon2 determinism test: PASSED\n");
}

/* Test: different types produce different outputs */
static void fio___test_argon2_types_differ(void) {
  fio_u512 rd = fio_argon2(.password = FIO_BUF_INFO2("password", 8),
                           .salt = FIO_BUF_INFO2("saltsalt", 8),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2D);
  fio_u512 ri = fio_argon2(.password = FIO_BUF_INFO2("password", 8),
                           .salt = FIO_BUF_INFO2("saltsalt", 8),
                           .t_cost = 1,
                           .m_cost = 64,
                           .parallelism = 1,
                           .outlen = 32,
                           .type = FIO_ARGON2I);
  fio_u512 rid = fio_argon2(.password = FIO_BUF_INFO2("password", 8),
                            .salt = FIO_BUF_INFO2("saltsalt", 8),
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
  fprintf(stderr, "    * Argon2 type differentiation test: PASSED\n");
}

/* *****************************************************************************
Performance Benchmarks
***************************************************************************** */

typedef struct {
  const char *name;
  uint32_t t_cost;
  uint32_t m_cost;
  uint32_t parallelism;
  uint32_t outlen;
  fio_argon2_type_e type;
} fio___argon2_bench_config_s;

FIO_SFUNC void fio___perf_argon2(void) {
#if defined(DEBUG) && (DEBUG)
  FIO_LOG_INFO("Argon2 performance tests skipped in DEBUG mode");
  return;
#else
  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tPerformance Tests: Argon2 Password Hashing\n");
  fprintf(stderr, "\t===========================================\n\n");

  static const fio___argon2_bench_config_s configs[] = {
      {"Argon2id (t=1, m=64K,  p=4)", 1, 65536, 4, 32, FIO_ARGON2ID},
      {"Argon2id (t=3, m=64K,  p=4)", 3, 65536, 4, 32, FIO_ARGON2ID},
      {"Argon2id (t=1, m=256K, p=4)", 1, 262144, 4, 32, FIO_ARGON2ID},
      {"Argon2d  (t=1, m=64K,  p=4)", 1, 65536, 4, 32, FIO_ARGON2D},
      {"Argon2i  (t=3, m=64K,  p=4)", 3, 65536, 4, 32, FIO_ARGON2I},
  };
  static const size_t n_configs = sizeof(configs) / sizeof(configs[0]);

  const char *pwd = "password";
  const size_t pwd_len = 8;
  const char *salt = "saltsaltsaltsalt";
  const size_t salt_len = 16;

  fprintf(stderr,
          "\t  %-45s %10s  %10s\n",
          "Configuration",
          "ops/sec",
          "memory KB");
  fprintf(stderr,
          "\t  -----------------------------------------------------------"
          "--------\n");

  for (size_t i = 0; i < n_configs; ++i) {
    const fio___argon2_bench_config_s *cfg = &configs[i];
    double mem_kb = (double)cfg->m_cost;

    uint64_t iterations = 0;
    clock_t start = clock();
    clock_t min_duration = CLOCKS_PER_SEC;
    for (;;) {
      fio_u512 r = fio_argon2(.password = FIO_BUF_INFO2((char *)pwd, pwd_len),
                              .salt = FIO_BUF_INFO2((char *)salt, salt_len),
                              .t_cost = cfg->t_cost,
                              .m_cost = cfg->m_cost,
                              .parallelism = cfg->parallelism,
                              .outlen = cfg->outlen,
                              .type = cfg->type);
      (void)r;
      FIO_COMPILER_GUARD;
      ++iterations;
      if (iterations >= 4 && (clock() - start) >= min_duration)
        break;
    }
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    double ops_sec = (double)iterations / (elapsed > 0.0 ? elapsed : 0.0001);

    fprintf(stderr, "\t    %-45s %10.2f  %10.1f\n", cfg->name, ops_sec, mem_kb);
  }

  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tArgon2 benchmarks complete.\n");
  fprintf(stderr, "\t===========================================\n");
#endif /* DEBUG */
}

int main(void) {
  fprintf(stderr, "Testing Argon2 implementation (RFC 9106)...\n");

  fio___test_argon2_basic();
  fio___test_argon2_determinism();
  fio___test_argon2_types_differ();
  fio___test_argon2d_rfc();
  fio___test_argon2i_rfc();
  fio___test_argon2id_rfc();

  fprintf(stderr, "All Argon2 tests passed!\n");

  fio___perf_argon2();

  return 0;
}
