/* *****************************************************************************
Lyra2 Tests - Reference test vectors from verified Rust port of official C impl
***************************************************************************** */
#include "test-helpers.h"
#define FIO_LOG
#define FIO_LYRA2
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

/* Test: basic invocation does not crash and produces non-zero output */
static void fio___test_lyra2_basic(void) {
  fio_u512 r = fio_lyra2(.password = FIO_BUF_INFO2("password", 8),
                         .salt = FIO_BUF_INFO2("salt", 4),
                         .t_cost = 1,
                         .m_cost = 3,
                         .n_cols = 4,
                         .outlen = 32);
  int zero = 1;
  for (size_t i = 0; i < 32; ++i)
    if (r.u8[i] != 0)
      zero = 0;
  FIO_ASSERT(!zero, "Lyra2 output should not be all zeros");
  fprintf(stderr, "    * Lyra2 basic test: PASSED\n");
}

/* Test: determinism - same inputs produce same output */
static void fio___test_lyra2_determinism(void) {
  fio_u512 r1 = fio_lyra2(.password = FIO_BUF_INFO2("test_password", 13),
                          .salt = FIO_BUF_INFO2("test_salt", 9),
                          .t_cost = 1,
                          .m_cost = 4,
                          .n_cols = 4,
                          .outlen = 32);
  fio_u512 r2 = fio_lyra2(.password = FIO_BUF_INFO2("test_password", 13),
                          .salt = FIO_BUF_INFO2("test_salt", 9),
                          .t_cost = 1,
                          .m_cost = 4,
                          .n_cols = 4,
                          .outlen = 32);
  FIO_ASSERT(!FIO_MEMCMP(r1.u8, r2.u8, 32), "Lyra2 should be deterministic");
  fprintf(stderr, "    * Lyra2 determinism test: PASSED\n");
}

/* Test: different passwords produce different outputs */
static void fio___test_lyra2_different_passwords(void) {
  fio_u512 r1 = fio_lyra2(.password = FIO_BUF_INFO2("password1", 9),
                          .salt = FIO_BUF_INFO2("salt", 4),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  fio_u512 r2 = fio_lyra2(.password = FIO_BUF_INFO2("password2", 9),
                          .salt = FIO_BUF_INFO2("salt", 4),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  FIO_ASSERT(FIO_MEMCMP(r1.u8, r2.u8, 32) != 0,
             "Different passwords should produce different hashes");
  fprintf(stderr, "    * Lyra2 different passwords test: PASSED\n");
}

/* Test: different salts produce different outputs */
static void fio___test_lyra2_different_salts(void) {
  fio_u512 r1 = fio_lyra2(.password = FIO_BUF_INFO2("password", 8),
                          .salt = FIO_BUF_INFO2("salt1", 5),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  fio_u512 r2 = fio_lyra2(.password = FIO_BUF_INFO2("password", 8),
                          .salt = FIO_BUF_INFO2("salt2", 5),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 32);
  FIO_ASSERT(FIO_MEMCMP(r1.u8, r2.u8, 32) != 0,
             "Different salts should produce different hashes");
  fprintf(stderr, "    * Lyra2 different salts test: PASSED\n");
}

/* ============================================================================
   Regression test vectors (self-generated).
   Implementation verified line-by-line against the official C reference at
   github.com/leocalm/Lyra (nPARALLEL=1, SPONGE=0/Blake2b, RHO=1).
   NOTE: The Rust port (wakiyamap/lyra2) implements an older/different Lyra2
   variant with different param encoding (u64 vs u32), different wandering
   phase, and rotate-by-1 instead of rotate-by-2 — it is NOT comparable.
   ============================================================================
 */

/* Vector 1: lyra2(kLen=32, pwd="abc", salt="abc", t_cost=1, n_rows=4, n_cols=4)
 */
static void fio___test_lyra2_vector1(void) {
  const char *expected_hex =
      "6e5995eee68c2dcb7322d500460082ccdf159bb4c0a7b94e8c3325b456fcaeda";
  uint8_t expected[32];
  fio___test_hex2bin(expected, expected_hex, 32);

  fio_u512 r = fio_lyra2(.password = FIO_BUF_INFO2("abc", 3),
                         .salt = FIO_BUF_INFO2("abc", 3),
                         .t_cost = 1,
                         .m_cost = 4,
                         .n_cols = 4,
                         .outlen = 32);

  if (FIO_MEMCMP(r.u8, expected, 32) != 0) {
    fio___test_print_hex("expected", expected, 32);
    fio___test_print_hex("got     ", r.u8, 32);
  }
  FIO_ASSERT(!FIO_MEMCMP(r.u8, expected, 32),
             "Lyra2 vector 1 mismatch (abc/abc t=1 r=4 c=4 k=32)");
  fprintf(stderr, "    * Lyra2 vector 1: PASSED\n");
}

/* Vector 2: lyra2(kLen=48, pwd=utf8, salt=same, t_cost=1, n_rows=3, n_cols=4)
 */
static void fio___test_lyra2_vector2(void) {
  const char *expected_hex =
      "d34c7266d7243544c85fabe32d9b962062ba8951c96c5d46570c9ff241ffb64e"
      "adf55dc49659a4053979dd3f509f58fd";
  uint8_t expected[48];
  fio___test_hex2bin(expected, expected_hex, 48);

  /* UTF-8 encoding of "脇山珠美ちゃんかわいい！" */
  char pwd[] = "\xe8\x84\x87\xe5\xb1\xb1\xe7\x8f\xa0\xe7\xbe\x8e\xe3\x81\xa1"
               "\xe3\x82\x83\xe3\x82\x93\xe3\x81\x8b\xe3\x82\x8f\xe3\x81\x84"
               "\xe3\x81\x84\xef\xbc\x81";
  size_t pwd_len = sizeof(pwd) - 1; /* 36 bytes */

  uint8_t out[48];
  int rc = fio_lyra2_hash(out,
                          .password = FIO_BUF_INFO2(pwd, pwd_len),
                          .salt = FIO_BUF_INFO2(pwd, pwd_len),
                          .t_cost = 1,
                          .m_cost = 3,
                          .n_cols = 4,
                          .outlen = 48);
  FIO_ASSERT(rc == 0, "fio_lyra2_hash should return 0");

  if (FIO_MEMCMP(out, expected, 48) != 0) {
    fio___test_print_hex("expected", expected, 48);
    fio___test_print_hex("got     ", out, 48);
  }
  FIO_ASSERT(!FIO_MEMCMP(out, expected, 48),
             "Lyra2 vector 2 mismatch (UTF-8 t=1 r=3 c=4 k=48)");
  fprintf(stderr, "    * Lyra2 vector 2: PASSED\n");
}

/* Vector 3: lyra2(kLen=16, pwd=emoji, salt=same, t_cost=1, n_rows=4, n_cols=2)
 */
static void fio___test_lyra2_vector3(void) {
  const char *expected_hex = "dd3ed5336edb96d38f43c037616973fb";
  uint8_t expected[16];
  fio___test_hex2bin(expected, expected_hex, 16);

  /* UTF-8 encoding of "😀😁😂" = 3 x 4-byte emoji = 12 bytes */
  char pwd[] = "\xf0\x9f\x98\x80\xf0\x9f\x98\x81\xf0\x9f\x98\x82";
  size_t pwd_len = 12;

  fio_u512 r = fio_lyra2(.password = FIO_BUF_INFO2(pwd, pwd_len),
                         .salt = FIO_BUF_INFO2(pwd, pwd_len),
                         .t_cost = 1,
                         .m_cost = 4,
                         .n_cols = 2,
                         .outlen = 16);

  if (FIO_MEMCMP(r.u8, expected, 16) != 0) {
    fio___test_print_hex("expected", expected, 16);
    fio___test_print_hex("got     ", r.u8, 16);
  }
  FIO_ASSERT(!FIO_MEMCMP(r.u8, expected, 16),
             "Lyra2 vector 3 mismatch (emoji t=1 r=4 c=2 k=16)");
  fprintf(stderr, "    * Lyra2 vector 3: PASSED\n");
}

/* *****************************************************************************
Performance Benchmarks: Lyra2
***************************************************************************** */

/* Lyra2 benchmark configuration */
typedef struct {
  const char *name;
  uint32_t t_cost;
  uint32_t m_cost;
  uint32_t n_cols;
  uint32_t outlen;
} fio___lyra2_bench_config_s;

FIO_SFUNC void fio___perf_lyra2(void) {
#if defined(DEBUG) && (DEBUG)
  FIO_LOG_INFO("Lyra2 performance tests skipped in DEBUG mode");
  return;
#else
  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tPerformance Tests: Lyra2 Password Hashing\n");
  fprintf(stderr, "\t===========================================\n\n");

  static const fio___lyra2_bench_config_s configs[] = {
      {"Small  (t=1, m=3,   c=256)", 1, 3, 256, 32},
      {"Medium (t=1, m=100, c=256)", 1, 100, 256, 32},
      {"Std    (t=3, m=100, c=256)", 3, 100, 256, 32},
      {"High   (t=3, m=256, c=256)", 3, 256, 256, 32},
  };
  static const size_t n_configs = sizeof(configs) / sizeof(configs[0]);

  const char *pwd = "password";
  const size_t pwd_len = sizeof("password") - 1;
  const char *salt = "saltsaltsaltsalt";
  const size_t salt_len = sizeof("saltsaltsaltsalt") - 1;

  /* BLOCK_LEN_BYTES = 12 uint64_t * 8 = 96 bytes per column (display only) */
  static const size_t block_bytes = 12 * sizeof(uint64_t);

  fprintf(stderr,
          "\t  %-45s %10s  %10s\n",
          "Configuration",
          "ops/sec",
          "matrix KB");
  fprintf(stderr,
          "\t  -----------------------------------------------------------"
          "--------\n");

  for (size_t i = 0; i < n_configs; ++i) {
    const fio___lyra2_bench_config_s *cfg = &configs[i];
    size_t matrix_bytes = (size_t)cfg->m_cost * cfg->n_cols * block_bytes;
    double matrix_kb = (double)matrix_bytes / 1024.0;

    /* Run for at least 1 second or minimum 8 iterations */
    uint64_t iterations = 0;
    clock_t start = clock();
    clock_t min_duration = CLOCKS_PER_SEC; /* 1 second */
    for (;;) {
      fio_u512 r = fio_lyra2(.password = FIO_BUF_INFO2((char *)pwd, pwd_len),
                             .salt = FIO_BUF_INFO2((char *)salt, salt_len),
                             .t_cost = cfg->t_cost,
                             .m_cost = cfg->m_cost,
                             .n_cols = cfg->n_cols,
                             .outlen = cfg->outlen);
      (void)r;
      FIO_COMPILER_GUARD;
      ++iterations;
      if (iterations >= 8 && (clock() - start) >= min_duration)
        break;
    }
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    double ops_sec = (double)iterations / (elapsed > 0.0 ? elapsed : 0.0001);

    fprintf(stderr,
            "\t    %-45s %10.2f  %10.1f\n",
            cfg->name,
            ops_sec,
            matrix_kb);
  }

  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tLyra2 benchmarks complete.\n");
  fprintf(stderr, "\t===========================================\n");
#endif /* DEBUG */
}

/* *****************************************************************************
Performance Benchmarks: OpenSSL Argon2id (for comparison)
Requires OpenSSL 3.2+ for Argon2 KDF support.
***************************************************************************** */
#ifdef HAVE_OPENSSL
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x30200000L
#include <openssl/core_names.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#define FIO___TEST_HAS_OPENSSL_ARGON2 1

typedef struct {
  const char *name;
  uint32_t t_cost;
  uint32_t m_cost_kb;
  uint32_t threads;
  uint32_t outlen;
} fio___argon2_bench_config_s;

FIO_SFUNC void fio___perf_argon2id(void) {
#if defined(DEBUG) && (DEBUG)
  FIO_LOG_INFO("Argon2id performance tests skipped in DEBUG mode");
  return;
#else
  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tPerformance Tests: OpenSSL Argon2id\n");
  fprintf(stderr, "\t===========================================\n\n");

  /*
   * Map Lyra2 matrix sizes to Argon2 memcost (KB):
   *   Lyra2 matrix = m_cost * n_cols * 96 bytes
   *   Small:  3   * 256 * 96 =   72 KB  ->   72 KB
   *   Medium: 100 * 256 * 96 = 2400 KB  -> 2400 KB
   *   Std:    100 * 256 * 96 = 2400 KB  -> 2400 KB (t=3)
   *   High:   256 * 256 * 96 = 6144 KB  -> 6144 KB
   */
  static const fio___argon2_bench_config_s configs[] = {
      {"Small  (t=1, mem=72KB,   p=1)", 1, 72, 1, 32},
      {"Medium (t=1, mem=2400KB, p=1)", 1, 2400, 1, 32},
      {"Std    (t=3, mem=2400KB, p=1)", 3, 2400, 1, 32},
      {"High   (t=3, mem=6144KB, p=1)", 3, 6144, 1, 32},
  };
  static const size_t n_configs = sizeof(configs) / sizeof(configs[0]);

  const char *pwd = "password";
  const size_t pwd_len = sizeof("password") - 1;
  const char *salt = "saltsaltsaltsalt";
  const size_t salt_len = sizeof("saltsaltsaltsalt") - 1;

  fprintf(stderr,
          "\t  %-45s %10s  %10s\n",
          "Configuration",
          "ops/sec",
          "mem KB");
  fprintf(stderr,
          "\t  -----------------------------------------------------------"
          "--------\n");

  for (size_t i = 0; i < n_configs; ++i) {
    const fio___argon2_bench_config_s *cfg = &configs[i];

    /* Verify Argon2id is available with a single test call */
    EVP_KDF *kdf = EVP_KDF_fetch(NULL, "ARGON2ID", NULL);
    if (!kdf) {
      fprintf(stderr, "\t    %-45s %10s  %10s\n", cfg->name, "N/A", "N/A");
      fprintf(stderr, "\t    (OpenSSL Argon2id not available in this build)\n");
      return;
    }

    uint64_t iterations = 0;
    clock_t start = clock();
    clock_t min_duration = CLOCKS_PER_SEC; /* 1 second */
    for (;;) {
      EVP_KDF_CTX *ctx = EVP_KDF_CTX_new(kdf);
      uint8_t out[64];
      uint32_t threads = cfg->threads;
      uint32_t m_cost = cfg->m_cost_kb;
      uint32_t t_cost = cfg->t_cost;
      OSSL_PARAM params[] = {
          OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD,
                                            (void *)pwd,
                                            pwd_len),
          OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
                                            (void *)salt,
                                            salt_len),
          OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_THREADS, &threads),
          OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_LANES, &threads),
          OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_MEMCOST, &m_cost),
          OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ITER, &t_cost),
          OSSL_PARAM_END,
      };
      EVP_KDF_derive(ctx, out, cfg->outlen, params);
      EVP_KDF_CTX_free(ctx);
      FIO_COMPILER_GUARD;
      ++iterations;
      if (iterations >= 8 && (clock() - start) >= min_duration)
        break;
    }
    EVP_KDF_free(kdf);
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    double ops_sec = (double)iterations / (elapsed > 0.0 ? elapsed : 0.0001);

    fprintf(stderr,
            "\t    %-45s %10.2f  %10.1f\n",
            cfg->name,
            ops_sec,
            (double)cfg->m_cost_kb);
  }

  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tArgon2id benchmarks complete.\n");
  fprintf(stderr, "\t===========================================\n");
#endif /* DEBUG */
}
#endif /* OPENSSL_VERSION_NUMBER >= 3.2 */
#endif /* HAVE_OPENSSL */

int main(void) {
  fprintf(stderr, "Testing Lyra2 implementation...\n");

  fio___test_lyra2_basic();
  fio___test_lyra2_determinism();
  fio___test_lyra2_different_passwords();
  fio___test_lyra2_different_salts();
  fio___test_lyra2_vector1();
  fio___test_lyra2_vector2();
  fio___test_lyra2_vector3();

  fprintf(stderr, "All Lyra2 tests passed!\n");

  fio___perf_lyra2();
#ifdef FIO___TEST_HAS_OPENSSL_ARGON2
  fio___perf_argon2id();
#endif

  return 0;
}
