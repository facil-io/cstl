/* *****************************************************************************
Performance Tests: Cryptographic Operations

These tests measure performance of cryptographic operations and are skipped in
DEBUG mode. Run with: make tests/performance-crypto

Includes OpenSSL head-to-head comparison when HAVE_OPENSSL is defined.
***************************************************************************** */

#define FIO_LOG
#define FIO_TIME
#define FIO_RAND
#define FIO_CRYPT
#define FIO_AES
#define FIO_LYRA2
#define FIO_ARGON2
#include "test-helpers.h"

/* OpenSSL Integration (Conditional Compilation) */
#ifdef HAVE_OPENSSL
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

/* Skip all performance tests in DEBUG mode */
#ifdef DEBUG
int main(void) {
  FIO_LOG_INFO("Performance tests skipped in DEBUG mode");
  return 0;
}
#else

/* *****************************************************************************
Speed Test Wrappers
***************************************************************************** */

/* SHA-1 wrapper */
FIO_SFUNC uintptr_t fio___perf_sha1_wrapper(char *data, size_t len) {
  fio_sha1_s h = fio_sha1((const void *)data, (uint64_t)len);
  return *(uintptr_t *)h.digest;
}

/* SHA-256 wrapper */
FIO_SFUNC uintptr_t fio___perf_sha256_wrapper(char *data, size_t len) {
  fio_u256 h = fio_sha256((const void *)data, (uint64_t)len);
  return (uintptr_t)(h.u64[0]);
}

/* SHA-512 wrapper */
FIO_SFUNC uintptr_t fio___perf_sha512_wrapper(char *data, size_t len) {
  fio_u512 h = fio_sha512((const void *)data, (uint64_t)len);
  return (uintptr_t)(h.u64[0]);
}

/* Blake2b wrapper */
FIO_SFUNC uintptr_t fio___perf_blake2b_wrapper(char *data, size_t len) {
  uint64_t out[8];
  fio_blake2b(out, 64, (const void *)data, len, NULL, 0);
  return (uintptr_t)(out[0]);
}

/* Blake2s wrapper */
FIO_SFUNC uintptr_t fio___perf_blake2s_wrapper(char *data, size_t len) {
  uint32_t out[8];
  fio_blake2s(out, 32, (const void *)data, len, NULL, 0);
  return (uintptr_t)(out[0]);
}

/* Poly1305 wrapper */
FIO_SFUNC uintptr_t fio___perf_poly1305_wrapper(char *msg, size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x85\xd6\xbe\x78\x57\x55\x6d\x33\x7f\x44\x52\xfe\x42"
                      "\xd5\x06\xa8"
                      "\x01\x03\x80\x8a\xfb\x0d\xb2\xfd\x4a\xbf\xf6\xaf\x41"
                      "\x49\xf5\x1b";
  fio_poly1305_auth(result, msg, len, NULL, 0, key);
  return (uintptr_t)result[0];
}

/* ChaCha20 wrapper */
FIO_SFUNC uintptr_t fio___perf_chacha20_wrapper(char *msg, size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_chacha20(msg, len, key, nounce, 1);
  result[0] = fio_buf2u64u(msg);
  return (uintptr_t)result[0];
}

/* ChaCha20-Poly1305 encrypt wrapper */
FIO_SFUNC uintptr_t fio___perf_chacha20poly1305_enc_wrapper(char *msg,
                                                            size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_chacha20_poly1305_enc(result, msg, len, NULL, 0, key, nounce);
  return (uintptr_t)result[0];
}

/* ChaCha20-Poly1305 decrypt wrapper
 * Calls auth + chacha20 directly to replicate the exact work of a successful
 * fio_chacha20_poly1305_dec without needing a pre-computed valid MAC tag.
 * This avoids calling enc inside the wrapper (which would measure enc+dec). */
FIO_SFUNC uintptr_t fio___perf_chacha20poly1305_dec_wrapper(char *msg,
                                                            size_t len) {
  uint64_t mac[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_chacha20_poly1305_auth(mac, msg, len, NULL, 0, key, nounce);
  fio_chacha20(msg, len, key, nounce, 1);
  return (uintptr_t)mac[0];
}

/* AES-128-GCM wrapper */
FIO_SFUNC uintptr_t fio___perf_aes128_gcm_wrapper(char *msg, size_t len) {
  uint8_t mac[16];
  uint8_t key[16] = {0x00,
                     0x01,
                     0x02,
                     0x03,
                     0x04,
                     0x05,
                     0x06,
                     0x07,
                     0x08,
                     0x09,
                     0x0a,
                     0x0b,
                     0x0c,
                     0x0d,
                     0x0e,
                     0x0f};
  uint8_t nonce[12] =
      {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
  fio_aes128_gcm_enc(mac, msg, len, NULL, 0, key, nonce);
  return (uintptr_t)fio_buf2u64u(mac);
}

/* AES-256-GCM wrapper */
FIO_SFUNC uintptr_t fio___perf_aes256_gcm_wrapper(char *msg, size_t len) {
  uint8_t mac[16];
  uint8_t key[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                     0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                     0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  uint8_t nonce[12] =
      {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
  fio_aes256_gcm_enc(mac, msg, len, NULL, 0, key, nonce);
  return (uintptr_t)fio_buf2u64u(mac);
}

/* *****************************************************************************
Performance Tests: SHA Family
***************************************************************************** */

FIO_SFUNC void fio___perf_sha(void) {
  fprintf(stderr, "\t* Benchmarking SHA hash functions...\n");

  /* SHA-1 */
  fprintf(stderr, "\n\t  SHA-1:\n");
  fio_test_hash_function(fio___perf_sha1_wrapper, (char *)"fio_sha1", 6, 0, 0);
  fio_test_hash_function(fio___perf_sha1_wrapper, (char *)"fio_sha1", 10, 0, 0);
  fio_test_hash_function(fio___perf_sha1_wrapper, (char *)"fio_sha1", 13, 0, 0);
  fio_test_hash_function(fio___perf_sha1_wrapper, (char *)"fio_sha1", 16, 0, 1);

  /* SHA-256 */
  fprintf(stderr, "\n\t  SHA-256:\n");
  fio_test_hash_function(fio___perf_sha256_wrapper,
                         (char *)"fio_sha256",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha256_wrapper,
                         (char *)"fio_sha256",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha256_wrapper,
                         (char *)"fio_sha256",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha256_wrapper,
                         (char *)"fio_sha256",
                         16,
                         0,
                         1);

  /* SHA-512 */
  fprintf(stderr, "\n\t  SHA-512:\n");
  fio_test_hash_function(fio___perf_sha512_wrapper,
                         (char *)"fio_sha512",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha512_wrapper,
                         (char *)"fio_sha512",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha512_wrapper,
                         (char *)"fio_sha512",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha512_wrapper,
                         (char *)"fio_sha512",
                         16,
                         0,
                         1);
}

/* *****************************************************************************
Performance Tests: Blake2
***************************************************************************** */

FIO_SFUNC void fio___perf_blake2(void) {
  fprintf(stderr, "\t* Benchmarking Blake2 hash functions...\n");

  /* Blake2b */
  fprintf(stderr, "\n\t  Blake2b:\n");
  fio_test_hash_function(fio___perf_blake2b_wrapper,
                         (char *)"fio_blake2b",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_blake2b_wrapper,
                         (char *)"fio_blake2b",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_blake2b_wrapper,
                         (char *)"fio_blake2b",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_blake2b_wrapper,
                         (char *)"fio_blake2b",
                         16,
                         0,
                         1);

  /* Blake2s */
  fprintf(stderr, "\n\t  Blake2s:\n");
  fio_test_hash_function(fio___perf_blake2s_wrapper,
                         (char *)"fio_blake2s",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_blake2s_wrapper,
                         (char *)"fio_blake2s",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_blake2s_wrapper,
                         (char *)"fio_blake2s",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_blake2s_wrapper,
                         (char *)"fio_blake2s",
                         16,
                         0,
                         1);
}

/* *****************************************************************************
Performance Tests: ChaCha20/Poly1305
***************************************************************************** */

FIO_SFUNC void fio___perf_chacha(void) {
  fprintf(stderr, "\t* Benchmarking ChaCha20/Poly1305...\n");

  /* Poly1305 */
  fprintf(stderr, "\n\t  Poly1305:\n");
  fio_test_hash_function(fio___perf_poly1305_wrapper,
                         (char *)"fio_poly1305",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_poly1305_wrapper,
                         (char *)"fio_poly1305",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_poly1305_wrapper,
                         (char *)"fio_poly1305",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_poly1305_wrapper,
                         (char *)"fio_poly1305",
                         16,
                         0,
                         0);

  /* ChaCha20 */
  fprintf(stderr, "\n\t  ChaCha20:\n");
  fio_test_hash_function(fio___perf_chacha20_wrapper,
                         (char *)"fio_chacha20",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20_wrapper,
                         (char *)"fio_chacha20",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20_wrapper,
                         (char *)"fio_chacha20",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20_wrapper,
                         (char *)"fio_chacha20",
                         16,
                         0,
                         0);

  /* ChaCha20-Poly1305 */
  fprintf(stderr, "\n\t  ChaCha20-Poly1305:\n");
  fio_test_hash_function(fio___perf_chacha20poly1305_enc_wrapper,
                         (char *)"ChaCha20Poly1305 (encrypt+MAC)",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_enc_wrapper,
                         (char *)"ChaCha20Poly1305 (encrypt+MAC)",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_enc_wrapper,
                         (char *)"ChaCha20Poly1305 (encrypt+MAC)",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_enc_wrapper,
                         (char *)"ChaCha20Poly1305 (encrypt+MAC)",
                         16,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_dec_wrapper,
                         (char *)"ChaCha20Poly1305 (auth+decrypt)",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_dec_wrapper,
                         (char *)"ChaCha20Poly1305 (auth+decrypt)",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_dec_wrapper,
                         (char *)"ChaCha20Poly1305 (auth+decrypt)",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_dec_wrapper,
                         (char *)"ChaCha20Poly1305 (auth+decrypt)",
                         16,
                         0,
                         0);
}

/* *****************************************************************************
Performance Tests: AES-GCM
***************************************************************************** */

FIO_SFUNC void fio___perf_aes(void) {
  fprintf(stderr, "\t* Benchmarking AES-GCM...\n");

#if FIO___HAS_X86_AES_INTRIN
  fprintf(stderr, "\t  (using x86 AES-NI)\n");
#elif FIO___HAS_ARM_AES_INTRIN
  fprintf(stderr, "\t  (using ARM Crypto Extensions)\n");
#else
  fprintf(stderr, "\t  (using software fallback)\n");
#endif

  /* AES-128-GCM */
  fprintf(stderr, "\n\t  AES-128-GCM:\n");
  fio_test_hash_function(fio___perf_aes128_gcm_wrapper,
                         (char *)"fio_aes128_gcm",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_aes128_gcm_wrapper,
                         (char *)"fio_aes128_gcm",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_aes128_gcm_wrapper,
                         (char *)"fio_aes128_gcm",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_aes128_gcm_wrapper,
                         (char *)"fio_aes128_gcm",
                         16,
                         0,
                         0);

  /* AES-256-GCM */
  fprintf(stderr, "\n\t  AES-256-GCM:\n");
  fio_test_hash_function(fio___perf_aes256_gcm_wrapper,
                         (char *)"fio_aes256_gcm",
                         6,
                         0,
                         0);
  fio_test_hash_function(fio___perf_aes256_gcm_wrapper,
                         (char *)"fio_aes256_gcm",
                         10,
                         0,
                         0);
  fio_test_hash_function(fio___perf_aes256_gcm_wrapper,
                         (char *)"fio_aes256_gcm",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_aes256_gcm_wrapper,
                         (char *)"fio_aes256_gcm",
                         16,
                         0,
                         0);
}

/* *****************************************************************************
Performance Tests: Password Hashing (KDF)
***************************************************************************** */

/* Lyra2 benchmark configuration */
typedef struct {
  const char *name;
  uint32_t t_cost;
  uint32_t m_cost;
  uint32_t n_cols;
  uint32_t outlen;
} fio___perf_lyra2_config_s;

/* Argon2 benchmark configuration */
typedef struct {
  const char *name;
  uint32_t t_cost;
  uint32_t m_cost;
  uint32_t parallelism;
  uint32_t outlen;
  fio_argon2_type_e type;
} fio___perf_argon2_config_s;

FIO_SFUNC void fio___perf_kdf(void) {
  fprintf(stderr, "\t* Benchmarking Password Hashing (KDF)...\n");

  const char *pwd = "password";
  const size_t pwd_len = 8;
  const char *salt = "saltsaltsaltsalt";
  const size_t salt_len = 16;

  /* BLOCK_LEN_BYTES = 12 uint64_t * 8 = 96 bytes per column */
  static const size_t lyra2_block_bytes = 12 * sizeof(uint64_t);

  /*
   * Memory tiers for fair comparison (Lyra2 matrix = m_cost * n_cols * 96):
   *   ~2.4 MB:  Lyra2 m=100  c=256 -> 2400 KB   |  Argon2 m_cost=2400
   *   ~6   MB:  Lyra2 m=256  c=256 -> 6144 KB   |  Argon2 m_cost=6144
   *   ~24  MB:  Lyra2 m=1024 c=256 -> 24000 KB  |  Argon2 m_cost=24000
   *   ~64  MB:  Lyra2 m=2731 c=256 -> 65546 KB  |  Argon2 m_cost=65536
   */

  /* ---- Lyra2 ---- */
  fprintf(stderr, "\n\t  Lyra2:\n");
  fprintf(stderr,
          "\t  %-45s %10s  %10s\n",
          "Configuration",
          "ops/sec",
          "memory KB");
  fprintf(stderr,
          "\t  -----------------------------------------------------------"
          "--------\n");

  static const fio___perf_lyra2_config_s lyra2_configs[] = {
      {"Lyra2  (t=1, ~2.4MB)", 1, 100, 256, 32},
      {"Lyra2  (t=1, ~6MB)", 1, 256, 256, 32},
      {"Lyra2  (t=1, ~24MB)", 1, 1024, 256, 32},
      {"Lyra2  (t=1, ~64MB)", 1, 2731, 256, 32},
      {"Lyra2  (t=3, ~2.4MB)", 3, 100, 256, 32},
      {"Lyra2  (t=3, ~6MB)", 3, 256, 256, 32},
  };

  for (size_t i = 0; i < sizeof(lyra2_configs) / sizeof(lyra2_configs[0]);
       ++i) {
    const fio___perf_lyra2_config_s *cfg = &lyra2_configs[i];
    size_t matrix_bytes = (size_t)cfg->m_cost * cfg->n_cols * lyra2_block_bytes;
    double matrix_kb = (double)matrix_bytes / 1024.0;

    uint64_t iterations = 0;
    clock_t start = clock();
    clock_t min_duration = CLOCKS_PER_SEC;
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
      if (iterations >= 4 && (clock() - start) >= min_duration)
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

  /* ---- Argon2 (matching memory tiers, p=1 for single-threaded comparison) */
  fprintf(stderr, "\n\t  Argon2:\n");
  fprintf(stderr,
          "\t  %-45s %10s  %10s\n",
          "Configuration",
          "ops/sec",
          "memory KB");
  fprintf(stderr,
          "\t  -----------------------------------------------------------"
          "--------\n");

  static const fio___perf_argon2_config_s argon2_configs[] = {
      {"Argon2id (t=1, ~2.4MB, p=1)", 1, 2400, 1, 32, FIO_ARGON2ID},
      {"Argon2id (t=1, ~6MB,   p=1)", 1, 6144, 1, 32, FIO_ARGON2ID},
      {"Argon2id (t=1, ~24MB,  p=1)", 1, 24000, 1, 32, FIO_ARGON2ID},
      {"Argon2id (t=1, ~64MB,  p=1)", 1, 65536, 1, 32, FIO_ARGON2ID},
      {"Argon2id (t=3, ~2.4MB, p=1)", 3, 2400, 1, 32, FIO_ARGON2ID},
      {"Argon2id (t=3, ~6MB,   p=1)", 3, 6144, 1, 32, FIO_ARGON2ID},
  };

  for (size_t i = 0; i < sizeof(argon2_configs) / sizeof(argon2_configs[0]);
       ++i) {
    const fio___perf_argon2_config_s *cfg = &argon2_configs[i];
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
}

/* *****************************************************************************
OpenSSL Comparison Benchmarks (Conditional Compilation)
***************************************************************************** */

#ifdef HAVE_OPENSSL
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/params.h>

/* Benchmarking macro for OpenSSL */
#define OPENSSL_BENCH(name_str, size_bytes, code_block)                        \
  do {                                                                         \
    clock_t bench_start = clock();                                             \
    uint64_t bench_iterations = 0;                                             \
    double total_bytes = 0.0;                                                  \
    for (; (clock() - bench_start) < (2000 * CLOCKS_PER_SEC / 1000) ||         \
           bench_iterations < 100;                                             \
         ++bench_iterations) {                                                 \
      code_block;                                                              \
      total_bytes += (size_bytes);                                             \
    }                                                                          \
    clock_t bench_end = clock();                                               \
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;       \
    double ops_sec = bench_iterations / (elapsed > 0.0 ? elapsed : 0.0001);    \
    double mb_sec = (total_bytes / (1024.0 * 1024.0)) / elapsed;               \
    fprintf(stderr,                                                            \
            "\t    %-45s %10.2f ops/sec  %8.2f MB/s\n",                        \
            name_str,                                                          \
            ops_sec,                                                           \
            mb_sec);                                                           \
  } while (0)

/* OpenSSL SHA-1 benchmark */
FIO_SFUNC void openssl_bench_sha1(void) {
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char digest[20];
  unsigned int digest_len;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  OPENSSL_BENCH("OpenSSL SHA-1 (64 bytes)", 64, {
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, data_64, 64);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-1 (1024 bytes)", 1024, {
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, data_1024, 1024);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-1 (8192 bytes)", 8192, {
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, data_8192, 8192);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-1 (65536 bytes)", 65536, {
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, data_65536, 65536);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  EVP_MD_CTX_free(ctx);
}

/* OpenSSL SHA-256 benchmark */
FIO_SFUNC void openssl_bench_sha256(void) {
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char digest[32];
  unsigned int digest_len;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  OPENSSL_BENCH("OpenSSL SHA-256 (64 bytes)", 64, {
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data_64, 64);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-256 (1024 bytes)", 1024, {
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data_1024, 1024);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-256 (8192 bytes)", 8192, {
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data_8192, 8192);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-256 (65536 bytes)", 65536, {
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data_65536, 65536);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  EVP_MD_CTX_free(ctx);
}

/* OpenSSL SHA-512 benchmark */
FIO_SFUNC void openssl_bench_sha512(void) {
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char digest[64];
  unsigned int digest_len;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  OPENSSL_BENCH("OpenSSL SHA-512 (64 bytes)", 64, {
    EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
    EVP_DigestUpdate(ctx, data_64, 64);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-512 (1024 bytes)", 1024, {
    EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
    EVP_DigestUpdate(ctx, data_1024, 1024);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-512 (8192 bytes)", 8192, {
    EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
    EVP_DigestUpdate(ctx, data_8192, 8192);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-512 (65536 bytes)", 65536, {
    EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
    EVP_DigestUpdate(ctx, data_65536, 65536);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  EVP_MD_CTX_free(ctx);
}

/* OpenSSL ChaCha20-Poly1305 benchmark */
FIO_SFUNC void openssl_bench_chacha20_poly1305(void) {
  unsigned char key[32];
  unsigned char nonce[12];
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char ciphertext[65536 + 16]; /* +16 for tag */
  unsigned char tag[16];
  int len;

  fio_rand_bytes(key, sizeof(key));
  fio_rand_bytes(nonce, sizeof(nonce));
  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  OPENSSL_BENCH("OpenSSL ChaCha20-Poly1305 encrypt (64 bytes)", 64, {
    EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_64, 64);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  OPENSSL_BENCH("OpenSSL ChaCha20-Poly1305 encrypt (1024 bytes)", 1024, {
    EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_1024, 1024);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  OPENSSL_BENCH("OpenSSL ChaCha20-Poly1305 encrypt (8192 bytes)", 8192, {
    EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_8192, 8192);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  OPENSSL_BENCH("OpenSSL ChaCha20-Poly1305 encrypt (65536 bytes)", 65536, {
    EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_65536, 65536);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  EVP_CIPHER_CTX_free(ctx);
}

/* OpenSSL Poly1305 standalone MAC benchmark */
FIO_SFUNC void openssl_bench_poly1305(void) {
  unsigned char key[32];
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char mac[16];
  size_t mac_len = sizeof(mac);

  fio_rand_bytes(key, sizeof(key));
  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  EVP_MAC *evp_mac = EVP_MAC_fetch(NULL, "POLY1305", NULL);
  if (!evp_mac) {
    fprintf(stderr, "\t    OpenSSL Poly1305 not available\n");
    return;
  }

  EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(evp_mac);
  OSSL_PARAM params[] = {OSSL_PARAM_END};

  OPENSSL_BENCH("OpenSSL Poly1305 (64 bytes)", 64, {
    EVP_MAC_init(ctx, key, sizeof(key), params);
    EVP_MAC_update(ctx, data_64, sizeof(data_64));
    EVP_MAC_final(ctx, mac, &mac_len, sizeof(mac));
  });

  OPENSSL_BENCH("OpenSSL Poly1305 (1024 bytes)", 1024, {
    EVP_MAC_init(ctx, key, sizeof(key), params);
    EVP_MAC_update(ctx, data_1024, sizeof(data_1024));
    EVP_MAC_final(ctx, mac, &mac_len, sizeof(mac));
  });

  OPENSSL_BENCH("OpenSSL Poly1305 (8192 bytes)", 8192, {
    EVP_MAC_init(ctx, key, sizeof(key), params);
    EVP_MAC_update(ctx, data_8192, sizeof(data_8192));
    EVP_MAC_final(ctx, mac, &mac_len, sizeof(mac));
  });

  OPENSSL_BENCH("OpenSSL Poly1305 (65536 bytes)", 65536, {
    EVP_MAC_init(ctx, key, sizeof(key), params);
    EVP_MAC_update(ctx, data_65536, sizeof(data_65536));
    EVP_MAC_final(ctx, mac, &mac_len, sizeof(mac));
  });

  EVP_MAC_CTX_free(ctx);
  EVP_MAC_free(evp_mac);
}

/* OpenSSL ChaCha20 standalone cipher benchmark */
FIO_SFUNC void openssl_bench_chacha20(void) {
  unsigned char key[32];
  unsigned char
      iv[16]; /* ChaCha20 uses 16-byte IV: 4-byte counter + 12-byte nonce */
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char ciphertext[65536];
  int len;

  fio_rand_bytes(key, sizeof(key));
  fio_rand_bytes(iv, sizeof(iv));
  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  OPENSSL_BENCH("OpenSSL ChaCha20 (64 bytes)", 64, {
    EVP_EncryptInit_ex(ctx, EVP_chacha20(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_64, 64);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
  });

  OPENSSL_BENCH("OpenSSL ChaCha20 (1024 bytes)", 1024, {
    EVP_EncryptInit_ex(ctx, EVP_chacha20(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_1024, 1024);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
  });

  OPENSSL_BENCH("OpenSSL ChaCha20 (8192 bytes)", 8192, {
    EVP_EncryptInit_ex(ctx, EVP_chacha20(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_8192, 8192);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
  });

  OPENSSL_BENCH("OpenSSL ChaCha20 (65536 bytes)", 65536, {
    EVP_EncryptInit_ex(ctx, EVP_chacha20(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_65536, 65536);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
  });

  EVP_CIPHER_CTX_free(ctx);
}

/* OpenSSL ChaCha20-Poly1305 decrypt benchmark */
FIO_SFUNC void openssl_bench_chacha20_poly1305_decrypt(void) {
  unsigned char key[32];
  unsigned char nonce[12];
  unsigned char plaintext_64[64];
  unsigned char plaintext_1024[1024];
  unsigned char plaintext_8192[8192];
  unsigned char plaintext_65536[65536];
  unsigned char ciphertext_64[64];
  unsigned char ciphertext_1024[1024];
  unsigned char ciphertext_8192[8192];
  unsigned char ciphertext_65536[65536];
  unsigned char decrypted[65536];
  unsigned char tag_64[16];
  unsigned char tag_1024[16];
  unsigned char tag_8192[16];
  unsigned char tag_65536[16];
  int len, tmplen;

  fio_rand_bytes(key, sizeof(key));
  fio_rand_bytes(nonce, sizeof(nonce));
  fio_rand_bytes(plaintext_64, sizeof(plaintext_64));
  fio_rand_bytes(plaintext_1024, sizeof(plaintext_1024));
  fio_rand_bytes(plaintext_8192, sizeof(plaintext_8192));
  fio_rand_bytes(plaintext_65536, sizeof(plaintext_65536));

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  /* Pre-encrypt data to get valid ciphertext and tags for decryption */
  EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
  EVP_EncryptUpdate(ctx, ciphertext_64, &len, plaintext_64, 64);
  EVP_EncryptFinal_ex(ctx, ciphertext_64 + len, &tmplen);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag_64);

  EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
  EVP_EncryptUpdate(ctx, ciphertext_1024, &len, plaintext_1024, 1024);
  EVP_EncryptFinal_ex(ctx, ciphertext_1024 + len, &tmplen);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag_1024);

  EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
  EVP_EncryptUpdate(ctx, ciphertext_8192, &len, plaintext_8192, 8192);
  EVP_EncryptFinal_ex(ctx, ciphertext_8192 + len, &tmplen);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag_8192);

  EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
  EVP_EncryptUpdate(ctx, ciphertext_65536, &len, plaintext_65536, 65536);
  EVP_EncryptFinal_ex(ctx, ciphertext_65536 + len, &tmplen);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag_65536);

  /* Benchmark decryption */
  OPENSSL_BENCH("OpenSSL ChaCha20-Poly1305 decrypt (64 bytes)", 64, {
    EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag_64);
    EVP_DecryptUpdate(ctx, decrypted, &len, ciphertext_64, 64);
    EVP_DecryptFinal_ex(ctx, decrypted + len, &tmplen);
  });

  OPENSSL_BENCH("OpenSSL ChaCha20-Poly1305 decrypt (1024 bytes)", 1024, {
    EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag_1024);
    EVP_DecryptUpdate(ctx, decrypted, &len, ciphertext_1024, 1024);
    EVP_DecryptFinal_ex(ctx, decrypted + len, &tmplen);
  });

  OPENSSL_BENCH("OpenSSL ChaCha20-Poly1305 decrypt (8192 bytes)", 8192, {
    EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag_8192);
    EVP_DecryptUpdate(ctx, decrypted, &len, ciphertext_8192, 8192);
    EVP_DecryptFinal_ex(ctx, decrypted + len, &tmplen);
  });

  OPENSSL_BENCH("OpenSSL ChaCha20-Poly1305 decrypt (65536 bytes)", 65536, {
    EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag_65536);
    EVP_DecryptUpdate(ctx, decrypted, &len, ciphertext_65536, 65536);
    EVP_DecryptFinal_ex(ctx, decrypted + len, &tmplen);
  });

  EVP_CIPHER_CTX_free(ctx);
}

/* OpenSSL AES-128-GCM benchmark */
FIO_SFUNC void openssl_bench_aes128_gcm(void) {
  unsigned char key[16];
  unsigned char nonce[12];
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char ciphertext[65536 + 16];
  unsigned char tag[16];
  int len;

  fio_rand_bytes(key, sizeof(key));
  fio_rand_bytes(nonce, sizeof(nonce));
  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  OPENSSL_BENCH("OpenSSL AES-128-GCM encrypt (64 bytes)", 64, {
    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_64, 64);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  OPENSSL_BENCH("OpenSSL AES-128-GCM encrypt (1024 bytes)", 1024, {
    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_1024, 1024);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  OPENSSL_BENCH("OpenSSL AES-128-GCM encrypt (8192 bytes)", 8192, {
    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_8192, 8192);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  OPENSSL_BENCH("OpenSSL AES-128-GCM encrypt (65536 bytes)", 65536, {
    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_65536, 65536);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  EVP_CIPHER_CTX_free(ctx);
}

/* OpenSSL AES-256-GCM benchmark */
FIO_SFUNC void openssl_bench_aes256_gcm(void) {
  unsigned char key[32];
  unsigned char nonce[12];
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char ciphertext[65536 + 16];
  unsigned char tag[16];
  int len;

  fio_rand_bytes(key, sizeof(key));
  fio_rand_bytes(nonce, sizeof(nonce));
  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  OPENSSL_BENCH("OpenSSL AES-256-GCM encrypt (64 bytes)", 64, {
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_64, 64);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  OPENSSL_BENCH("OpenSSL AES-256-GCM encrypt (1024 bytes)", 1024, {
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_1024, 1024);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  OPENSSL_BENCH("OpenSSL AES-256-GCM encrypt (8192 bytes)", 8192, {
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_8192, 8192);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  OPENSSL_BENCH("OpenSSL AES-256-GCM encrypt (65536 bytes)", 65536, {
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
    EVP_EncryptUpdate(ctx, ciphertext, &len, data_65536, 65536);
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
  });

  EVP_CIPHER_CTX_free(ctx);
}

/* OpenSSL Argon2id benchmark (requires OpenSSL 3.2+) */
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x30200000L
#include <openssl/core_names.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#define FIO___PERF_HAS_OPENSSL_ARGON2 1

typedef struct {
  const char *name;
  uint32_t t_cost;
  uint32_t m_cost_kb;
  uint32_t threads;
  uint32_t outlen;
} fio___perf_openssl_argon2_config_s;

FIO_SFUNC void openssl_bench_argon2id(void) {
  /* Matching memory tiers for fair comparison with facil.io Lyra2/Argon2 */
  static const fio___perf_openssl_argon2_config_s configs[] = {
      {"OpenSSL Argon2id (t=1, ~2.4MB, p=1)", 1, 2400, 1, 32},
      {"OpenSSL Argon2id (t=1, ~6MB,   p=1)", 1, 6144, 1, 32},
      {"OpenSSL Argon2id (t=1, ~24MB,  p=1)", 1, 24000, 1, 32},
      {"OpenSSL Argon2id (t=1, ~64MB,  p=1)", 1, 65536, 1, 32},
      {"OpenSSL Argon2id (t=3, ~2.4MB, p=1)", 3, 2400, 1, 32},
      {"OpenSSL Argon2id (t=3, ~6MB,   p=1)", 3, 6144, 1, 32},
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
          "mem KB");
  fprintf(stderr,
          "\t  -----------------------------------------------------------"
          "--------\n");

  for (size_t i = 0; i < n_configs; ++i) {
    const fio___perf_openssl_argon2_config_s *cfg = &configs[i];

    EVP_KDF *kdf = EVP_KDF_fetch(NULL, "ARGON2ID", NULL);
    if (!kdf) {
      fprintf(stderr, "\t    %-45s %10s  %10s\n", cfg->name, "N/A", "N/A");
      fprintf(stderr, "\t    (OpenSSL Argon2id not available in this build)\n");
      return;
    }

    uint64_t iterations = 0;
    clock_t start = clock();
    clock_t min_duration = CLOCKS_PER_SEC;
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
      if (iterations >= 4 && (clock() - start) >= min_duration)
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
}
#endif /* OPENSSL_VERSION_NUMBER >= 0x30200000L */

/* OpenSSL Blake2b benchmark */
FIO_SFUNC void openssl_bench_blake2b(void) {
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char digest[64];
  unsigned int digest_len;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  OPENSSL_BENCH("OpenSSL Blake2b-512 (64 bytes)", 64, {
    EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
    EVP_DigestUpdate(ctx, data_64, 64);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL Blake2b-512 (1024 bytes)", 1024, {
    EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
    EVP_DigestUpdate(ctx, data_1024, 1024);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL Blake2b-512 (8192 bytes)", 8192, {
    EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
    EVP_DigestUpdate(ctx, data_8192, 8192);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL Blake2b-512 (65536 bytes)", 65536, {
    EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
    EVP_DigestUpdate(ctx, data_65536, 65536);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  EVP_MD_CTX_free(ctx);
}

/* OpenSSL Blake2s benchmark */
FIO_SFUNC void openssl_bench_blake2s(void) {
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char data_65536[65536];
  unsigned char digest[32];
  unsigned int digest_len;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));
  fio_rand_bytes(data_65536, sizeof(data_65536));

  OPENSSL_BENCH("OpenSSL Blake2s-256 (64 bytes)", 64, {
    EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
    EVP_DigestUpdate(ctx, data_64, 64);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL Blake2s-256 (1024 bytes)", 1024, {
    EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
    EVP_DigestUpdate(ctx, data_1024, 1024);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL Blake2s-256 (8192 bytes)", 8192, {
    EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
    EVP_DigestUpdate(ctx, data_8192, 8192);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL Blake2s-256 (65536 bytes)", 65536, {
    EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
    EVP_DigestUpdate(ctx, data_65536, 65536);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  EVP_MD_CTX_free(ctx);
}

/* Run all OpenSSL benchmarks */
FIO_SFUNC void fio___perf_openssl(void) {
  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tOpenSSL Comparison Benchmarks\n");
  fprintf(stderr, "\t===========================================\n\n");

  fprintf(stderr, "\t* OpenSSL SHA Hash Functions:\n");
  fprintf(stderr, "\n\t  OpenSSL SHA-1:\n");
  openssl_bench_sha1();

  fprintf(stderr, "\n\t  OpenSSL SHA-256:\n");
  openssl_bench_sha256();

  fprintf(stderr, "\n\t  OpenSSL SHA-512:\n");
  openssl_bench_sha512();

  fprintf(stderr, "\n\t* OpenSSL Blake2 Hash Functions:\n");
  fprintf(stderr, "\n\t  OpenSSL Blake2b-512:\n");
  openssl_bench_blake2b();

  fprintf(stderr, "\n\t  OpenSSL Blake2s-256:\n");
  openssl_bench_blake2s();

  fprintf(stderr, "\n\t* OpenSSL Poly1305 (standalone MAC):\n\n");
  openssl_bench_poly1305();

  fprintf(stderr, "\n\t* OpenSSL ChaCha20 (standalone cipher):\n\n");
  openssl_bench_chacha20();

  fprintf(stderr, "\n\t* OpenSSL ChaCha20-Poly1305:\n\n");
  openssl_bench_chacha20_poly1305();

  fprintf(stderr, "\n\t* OpenSSL ChaCha20-Poly1305 Decrypt:\n\n");
  openssl_bench_chacha20_poly1305_decrypt();

  fprintf(stderr, "\n\t* OpenSSL AES-GCM:\n");
  fprintf(stderr, "\n\t  OpenSSL AES-128-GCM:\n");
  openssl_bench_aes128_gcm();

  fprintf(stderr, "\n\t  OpenSSL AES-256-GCM:\n");
  openssl_bench_aes256_gcm();

#ifdef FIO___PERF_HAS_OPENSSL_ARGON2
  fprintf(stderr, "\n\t* OpenSSL Argon2id (KDF comparison):\n\n");
  openssl_bench_argon2id();
#endif

  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tOpenSSL benchmarks complete\n");
  fprintf(stderr, "\t===========================================\n");
}

#endif /* HAVE_OPENSSL */

/* *****************************************************************************
Main Entry Point
***************************************************************************** */

int main(void) {
#if defined(DEBUG) && (DEBUG)
  if (1) {
    fprintf(stderr, "\t- Skipped in DEBUG\n");
    return 0;
  }
#endif
  fprintf(stderr, "\t===========================================\n");
  fprintf(stderr, "\tPerformance Tests: Cryptographic Operations\n");
  fprintf(stderr, "\t===========================================\n\n");

  fprintf(stderr, "\tfacil.io Implementation:\n");
  fprintf(stderr, "\t===========================================\n\n");

  fio___perf_sha();
  fio___perf_blake2();
  fio___perf_chacha();
  fio___perf_aes();
  fio___perf_kdf();

  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tfacil.io benchmarks complete.\n");
  fprintf(stderr, "\t===========================================\n");

#ifdef HAVE_OPENSSL
  fio___perf_openssl();
#endif

  return 0;
}

#endif /* DEBUG */
