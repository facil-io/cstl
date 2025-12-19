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

/* ChaCha20-Poly1305 decrypt wrapper */
FIO_SFUNC uintptr_t fio___perf_chacha20poly1305_dec_wrapper(char *msg,
                                                            size_t len) {
  uint64_t result[2] = {0};
  char *key = (char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                      "\x0d\x0e\x0f"
                      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c"
                      "\x1d\x1e\x1f";
  char *nounce = (char *)"\x00\x00\x00\x00\x00\x00\x00\x4a\x00\x00\x00\x00";
  fio_poly1305_auth(result, msg, len, NULL, 0, key);
  fio_chacha20(msg, len, key, nounce, 1);
  return (uintptr_t)result[0];
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
  fio_test_hash_function(fio___perf_sha1_wrapper, (char *)"fio_sha1", 5, 0, 0);
  fio_test_hash_function(fio___perf_sha1_wrapper, (char *)"fio_sha1", 7, 0, 0);
  fio_test_hash_function(fio___perf_sha1_wrapper, (char *)"fio_sha1", 13, 0, 1);

  /* SHA-256 */
  fprintf(stderr, "\n\t  SHA-256:\n");
  fio_test_hash_function(fio___perf_sha256_wrapper,
                         (char *)"fio_sha256",
                         5,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha256_wrapper,
                         (char *)"fio_sha256",
                         7,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha256_wrapper,
                         (char *)"fio_sha256",
                         13,
                         0,
                         1);

  /* SHA-512 */
  fprintf(stderr, "\n\t  SHA-512:\n");
  fio_test_hash_function(fio___perf_sha512_wrapper,
                         (char *)"fio_sha512",
                         5,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha512_wrapper,
                         (char *)"fio_sha512",
                         7,
                         0,
                         0);
  fio_test_hash_function(fio___perf_sha512_wrapper,
                         (char *)"fio_sha512",
                         13,
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
                         7,
                         0,
                         0);
  fio_test_hash_function(fio___perf_poly1305_wrapper,
                         (char *)"fio_poly1305",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_poly1305_wrapper,
                         (char *)"fio_poly1305 (unaligned)",
                         13,
                         3,
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
                         7,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20_wrapper,
                         (char *)"fio_chacha20",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20_wrapper,
                         (char *)"fio_chacha20 (unaligned)",
                         13,
                         3,
                         0);

  /* ChaCha20-Poly1305 */
  fprintf(stderr, "\n\t  ChaCha20-Poly1305:\n");
  fio_test_hash_function(fio___perf_chacha20poly1305_dec_wrapper,
                         (char *)"ChaCha20Poly1305 (auth+decrypt)",
                         7,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_dec_wrapper,
                         (char *)"ChaCha20Poly1305 (auth+decrypt)",
                         13,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_enc_wrapper,
                         (char *)"ChaCha20Poly1305 (encrypt+MAC)",
                         7,
                         0,
                         0);
  fio_test_hash_function(fio___perf_chacha20poly1305_enc_wrapper,
                         (char *)"ChaCha20Poly1305 (encrypt+MAC)",
                         13,
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
                         7,
                         0,
                         0);
  fio_test_hash_function(fio___perf_aes128_gcm_wrapper,
                         (char *)"fio_aes128_gcm",
                         13,
                         0,
                         0);

  /* AES-256-GCM */
  fprintf(stderr, "\n\t  AES-256-GCM:\n");
  fio_test_hash_function(fio___perf_aes256_gcm_wrapper,
                         (char *)"fio_aes256_gcm",
                         7,
                         0,
                         0);
  fio_test_hash_function(fio___perf_aes256_gcm_wrapper,
                         (char *)"fio_aes256_gcm",
                         13,
                         0,
                         0);
}

/* *****************************************************************************
OpenSSL Comparison Benchmarks (Conditional Compilation)
***************************************************************************** */

#ifdef HAVE_OPENSSL
#include <openssl/err.h>
#include <openssl/evp.h>

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
  unsigned char data_16[16];
  unsigned char data_128[128];
  unsigned char data_8192[8192];
  unsigned char digest[20];
  unsigned int digest_len;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  fio_rand_bytes(data_16, sizeof(data_16));
  fio_rand_bytes(data_128, sizeof(data_128));
  fio_rand_bytes(data_8192, sizeof(data_8192));

  OPENSSL_BENCH("OpenSSL SHA-1 (16 bytes)", 16, {
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, data_16, 16);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-1 (128 bytes)", 128, {
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, data_128, 128);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-1 (8192 bytes)", 8192, {
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, data_8192, 8192);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  EVP_MD_CTX_free(ctx);
}

/* OpenSSL SHA-256 benchmark */
FIO_SFUNC void openssl_bench_sha256(void) {
  unsigned char data_16[16];
  unsigned char data_128[128];
  unsigned char data_8192[8192];
  unsigned char digest[32];
  unsigned int digest_len;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  fio_rand_bytes(data_16, sizeof(data_16));
  fio_rand_bytes(data_128, sizeof(data_128));
  fio_rand_bytes(data_8192, sizeof(data_8192));

  OPENSSL_BENCH("OpenSSL SHA-256 (16 bytes)", 16, {
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data_16, 16);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-256 (128 bytes)", 128, {
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data_128, 128);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-256 (8192 bytes)", 8192, {
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data_8192, 8192);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  EVP_MD_CTX_free(ctx);
}

/* OpenSSL SHA-512 benchmark */
FIO_SFUNC void openssl_bench_sha512(void) {
  unsigned char data_16[16];
  unsigned char data_128[128];
  unsigned char data_8192[8192];
  unsigned char digest[64];
  unsigned int digest_len;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  fio_rand_bytes(data_16, sizeof(data_16));
  fio_rand_bytes(data_128, sizeof(data_128));
  fio_rand_bytes(data_8192, sizeof(data_8192));

  OPENSSL_BENCH("OpenSSL SHA-512 (16 bytes)", 16, {
    EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
    EVP_DigestUpdate(ctx, data_16, 16);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-512 (128 bytes)", 128, {
    EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
    EVP_DigestUpdate(ctx, data_128, 128);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
  });

  OPENSSL_BENCH("OpenSSL SHA-512 (8192 bytes)", 8192, {
    EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
    EVP_DigestUpdate(ctx, data_8192, 8192);
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
  unsigned char ciphertext[8192 + 16]; /* +16 for tag */
  unsigned char tag[16];
  int len;

  fio_rand_bytes(key, sizeof(key));
  fio_rand_bytes(nonce, sizeof(nonce));
  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));

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

  EVP_CIPHER_CTX_free(ctx);
}

/* OpenSSL AES-128-GCM benchmark */
FIO_SFUNC void openssl_bench_aes128_gcm(void) {
  unsigned char key[16];
  unsigned char nonce[12];
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char ciphertext[8192 + 16];
  unsigned char tag[16];
  int len;

  fio_rand_bytes(key, sizeof(key));
  fio_rand_bytes(nonce, sizeof(nonce));
  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));

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

  EVP_CIPHER_CTX_free(ctx);
}

/* OpenSSL AES-256-GCM benchmark */
FIO_SFUNC void openssl_bench_aes256_gcm(void) {
  unsigned char key[32];
  unsigned char nonce[12];
  unsigned char data_64[64];
  unsigned char data_1024[1024];
  unsigned char data_8192[8192];
  unsigned char ciphertext[8192 + 16];
  unsigned char tag[16];
  int len;

  fio_rand_bytes(key, sizeof(key));
  fio_rand_bytes(nonce, sizeof(nonce));
  fio_rand_bytes(data_64, sizeof(data_64));
  fio_rand_bytes(data_1024, sizeof(data_1024));
  fio_rand_bytes(data_8192, sizeof(data_8192));

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

  EVP_CIPHER_CTX_free(ctx);
}

/* Run all OpenSSL benchmarks */
FIO_SFUNC void fio___perf_openssl(void) {
  fprintf(stderr, "\n===========================================\n");
  fprintf(stderr, "OpenSSL Comparison Benchmarks\n");
  fprintf(stderr, "===========================================\n\n");

  fprintf(stderr, "\t* OpenSSL SHA Hash Functions:\n");
  fprintf(stderr, "\n\t  OpenSSL SHA-1:\n");
  openssl_bench_sha1();

  fprintf(stderr, "\n\t  OpenSSL SHA-256:\n");
  openssl_bench_sha256();

  fprintf(stderr, "\n\t  OpenSSL SHA-512:\n");
  openssl_bench_sha512();

  fprintf(stderr, "\n\t* OpenSSL ChaCha20-Poly1305:\n\n");
  openssl_bench_chacha20_poly1305();

  fprintf(stderr, "\n\t* OpenSSL AES-GCM:\n");
  fprintf(stderr, "\n\t  OpenSSL AES-128-GCM:\n");
  openssl_bench_aes128_gcm();

  fprintf(stderr, "\n\t  OpenSSL AES-256-GCM:\n");
  openssl_bench_aes256_gcm();

  fprintf(stderr, "\n===========================================\n");
  fprintf(stderr, "OpenSSL benchmarks complete\n");
  fprintf(stderr, "===========================================\n");
}

#endif /* HAVE_OPENSSL */

/* *****************************************************************************
Main Entry Point
***************************************************************************** */

int main(void) {
#if defined(DEBUG) && (DEBUG)
  if (1) {
    fprintf(stderr, "\t- Skipped in DEBUG\n");
    return 0
  }
#endif
  fprintf(stderr, "===========================================\n");
  fprintf(stderr, "Performance Tests: Cryptographic Operations\n");
  fprintf(stderr, "===========================================\n\n");

  fprintf(stderr, "facil.io Implementation:\n");
  fprintf(stderr, "===========================================\n\n");

  fio___perf_sha();
  fio___perf_chacha();
  fio___perf_aes();

  fprintf(stderr, "\n===========================================\n");
  fprintf(stderr, "facil.io benchmarks complete.\n");
  fprintf(stderr, "===========================================\n");

#ifdef HAVE_OPENSSL
  fio___perf_openssl();

  fprintf(stderr, "\n===========================================\n");
  fprintf(stderr, "Comparison Summary\n");
  fprintf(stderr, "===========================================\n\n");
  fprintf(stderr, "  ✓ Direct head-to-head comparison complete\n");
  fprintf(stderr, "  ✓ Both libraries tested on identical hardware\n");
  fprintf(stderr, "  ✓ Same compiler optimization level\n");
  fprintf(stderr, "\n  Compare the results above to see relative\n");
  fprintf(stderr, "  performance of facil.io vs OpenSSL.\n");
  fprintf(stderr, "\n===========================================\n");
#else
  fprintf(stderr, "\n===========================================\n");
  fprintf(stderr, "Note: OpenSSL comparison benchmarks skipped\n");
  fprintf(stderr, "      (compile with HAVE_OPENSSL to enable)\n");
  fprintf(stderr, "===========================================\n");
#endif

  return 0;
}

#endif /* DEBUG */
