/* *****************************************************************************
Real-World Crypto Performance Profiling
Tests multiplication optimizations in actual cryptographic contexts
Includes OpenSSL head-to-head benchmarks for direct comparison
***************************************************************************** */
#include "test-helpers.h"

#define FIO_TIME
#define FIO_RAND
#define FIO_SHA2
#define FIO_SHA3
#define FIO_BLAKE2
#define FIO_ED25519
#define FIO_CHACHA
#define FIO_AES
#define FIO_MLKEM
#define FIO_LYRA2
#define FIO_ARGON2
#include FIO_INCLUDE_FILE

/* *****************************************************************************
OpenSSL Integration (Conditional Compilation)
***************************************************************************** */
#ifdef HAVE_OPENSSL
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>

/* OpenSSL version compatibility check */
#if OPENSSL_VERSION_NUMBER < 0x30000000L
#warning "OpenSSL 3.x recommended for full benchmark support"
#endif

/* Check for OpenSSL 3.5+ which has native ML-KEM support */
#if OPENSSL_VERSION_NUMBER >= 0x30500000L
#define FIO___PERF_HAS_OPENSSL_MLKEM 1
#endif
#endif /* HAVE_OPENSSL */

/* *****************************************************************************
Benchmarking Macro
***************************************************************************** */

#define FIO_BENCH_CRYPTO(name_str, target_time_ms, code_block)                 \
  do {                                                                         \
    clock_t bench_start = clock();                                             \
    uint64_t bench_iterations = 0;                                             \
    for (;                                                                     \
         (clock() - bench_start) < ((target_time_ms)*CLOCKS_PER_SEC / 1000) || \
         bench_iterations < 100;                                               \
         ++bench_iterations) {                                                 \
      code_block;                                                              \
      FIO_COMPILER_GUARD;                                                      \
    }                                                                          \
    clock_t bench_end = clock();                                               \
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;       \
    double ops_sec = bench_iterations / (elapsed > 0.0 ? elapsed : 0.0001);    \
    double ms_op = (elapsed * 1000.0) / bench_iterations;                      \
    fprintf(stderr,                                                            \
            "      %-50s %10.2f ops/sec  (%8.4f ms/op)  [%llu iters]\n",       \
            name_str,                                                          \
            ops_sec,                                                           \
            ms_op,                                                             \
            (unsigned long long)bench_iterations);                             \
  } while (0)

/* *****************************************************************************
Ed25519 Profiling - Elliptic Curve Digital Signatures
***************************************************************************** */

/* Ed25519 uses field multiplication heavily in:
 * - Point addition (10-15 field muls per operation)
 * - Point doubling (8-10 field muls per operation)
 * - Scalar multiplication (thousands of field muls)
 * - Signature generation (includes scalar mul)
 * - Signature verification (includes 2 scalar muls)
 *
 * Field elements are 256-bit (5 x 51-bit limbs internally)
 * Each field mul uses ~25 fio_math_mulc64 calls
 */

FIO_SFUNC void fio_bench_ed25519_keygen(void) {
  fprintf(stderr, "    * Ed25519 Key Generation:\n");

  uint8_t sk[32], pk[32];
  fio_rand_bytes(sk, 32);

  FIO_BENCH_CRYPTO(
      "Ed25519 public key generation", 500, fio_ed25519_public_key(pk, sk);
      sk[0] ^= pk[0];);
}

FIO_SFUNC void fio_bench_ed25519_sign(void) {
  fprintf(stderr, "    * Ed25519 Signature Generation:\n");

  uint8_t sk[32], pk[32], sig[64];
  uint8_t *msg_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  uint8_t *msg_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  uint8_t *msg_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  uint8_t *msg_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(sk, 32);
  fio_rand_bytes(msg_small, 64);
  fio_rand_bytes(msg_medium, 1024);
  fio_rand_bytes(msg_large, 8192);
  fio_rand_bytes(msg_xlarge, 65536);
  fio_ed25519_public_key(pk, sk);

  /* Sign 64 byte message */
  FIO_BENCH_CRYPTO("Ed25519 sign (64 bytes)",
                   500,
                   fio_ed25519_sign(sig, msg_small, 64, sk, pk);
                   sk[0] ^= sig[0];);

  /* Sign 1024 byte message */
  FIO_BENCH_CRYPTO("Ed25519 sign (1024 bytes)",
                   500,
                   fio_ed25519_sign(sig, msg_medium, 1024, sk, pk);
                   sk[0] ^= sig[0];);

  /* Sign 8192 byte message */
  FIO_BENCH_CRYPTO("Ed25519 sign (8192 bytes)",
                   500,
                   fio_ed25519_sign(sig, msg_large, 8192, sk, pk);
                   sk[0] ^= sig[0];);

  /* Sign 65536 byte message */
  FIO_BENCH_CRYPTO("Ed25519 sign (65536 bytes)",
                   500,
                   fio_ed25519_sign(sig, msg_xlarge, 65536, sk, pk);
                   sk[0] ^= sig[0];);

  FIO_MEM_FREE(msg_small, 64);
  FIO_MEM_FREE(msg_medium, 1024);
  FIO_MEM_FREE(msg_large, 8192);
  FIO_MEM_FREE(msg_xlarge, 65536);
}

FIO_SFUNC void fio_bench_ed25519_verify(void) {
  fprintf(stderr, "    * Ed25519 Signature Verification:\n");

  uint8_t sk[32], pk[32], sig[64];
  uint8_t *msg_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  uint8_t *msg_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  uint8_t *msg_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  uint8_t *msg_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(sk, 32);
  fio_rand_bytes(msg_small, 64);
  fio_rand_bytes(msg_medium, 1024);
  fio_rand_bytes(msg_large, 8192);
  fio_rand_bytes(msg_xlarge, 65536);
  fio_ed25519_public_key(pk, sk);

  /* Verify 64 byte message */
  fio_ed25519_sign(sig, msg_small, 64, sk, pk);
  FIO_BENCH_CRYPTO("Ed25519 verify (64 bytes)",
                   500,
                   int result = fio_ed25519_verify(sig, msg_small, 64, pk);
                   pk[0] ^= (uint8_t)result;);

  /* Verify 1024 byte message */
  fio_ed25519_sign(sig, msg_medium, 1024, sk, pk);
  FIO_BENCH_CRYPTO("Ed25519 verify (1024 bytes)",
                   500,
                   int result = fio_ed25519_verify(sig, msg_medium, 1024, pk);
                   pk[0] ^= (uint8_t)result;);

  /* Verify 8192 byte message */
  fio_ed25519_sign(sig, msg_large, 8192, sk, pk);
  FIO_BENCH_CRYPTO("Ed25519 verify (8192 bytes)",
                   500,
                   int result = fio_ed25519_verify(sig, msg_large, 8192, pk);
                   pk[0] ^= (uint8_t)result;);

  /* Verify 65536 byte message */
  fio_ed25519_sign(sig, msg_xlarge, 65536, sk, pk);
  FIO_BENCH_CRYPTO("Ed25519 verify (65536 bytes)",
                   500,
                   int result = fio_ed25519_verify(sig, msg_xlarge, 65536, pk);
                   pk[0] ^= (uint8_t)result;);

  FIO_MEM_FREE(msg_small, 64);
  FIO_MEM_FREE(msg_medium, 1024);
  FIO_MEM_FREE(msg_large, 8192);
  FIO_MEM_FREE(msg_xlarge, 65536);
}

FIO_SFUNC void fio_bench_ed25519_roundtrip(void) {
  fprintf(stderr, "    * Ed25519 Sign+Verify Roundtrip:\n");

  uint8_t sk[32], pk[32], sig[64];
  uint8_t *msg = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);

  fio_rand_bytes(sk, 32);
  fio_rand_bytes(msg, 1024);
  fio_ed25519_public_key(pk, sk);

  FIO_BENCH_CRYPTO("Ed25519 sign+verify roundtrip (1024 bytes)",
                   500,
                   fio_ed25519_sign(sig, msg, 1024, sk, pk);
                   int result = fio_ed25519_verify(sig, msg, 1024, pk);
                   sk[0] ^= (uint8_t)result;);

  FIO_MEM_FREE(msg, 1024);
}

/* *****************************************************************************
ChaCha20-Poly1305 Profiling - Authenticated Encryption
***************************************************************************** */

/* Poly1305 uses field multiplication in GF(2^130-5):
 * - Each 16-byte block requires ~9 fio_math_mulc64 calls
 * - Processing 1 MB requires ~65,536 blocks = ~589,824 multiplications
 * - Field elements are 130-bit (stored as 3 x 44-bit limbs)
 * - Multiplication is the dominant cost in Poly1305 MAC
 */

FIO_SFUNC void fio_bench_chacha20_poly1305_enc(void) {
  fprintf(stderr, "    * ChaCha20-Poly1305 Encryption:\n");

  uint8_t key[32], nonce[12], mac[16];
  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  uint8_t *data_8k = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_8k, 8192);
  fio_rand_bytes(data_large, 65536);

  /* 64 byte encryption */
  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 encrypt (64 bytes)",
      500,
      fio_chacha20_poly1305_enc(mac, data_small, 64, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* 1024 byte encryption */
  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 encrypt (1024 bytes)",
      500,
      fio_chacha20_poly1305_enc(mac, data_medium, 1024, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* 8192 byte encryption */
  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 encrypt (8192 bytes)",
      500,
      fio_chacha20_poly1305_enc(mac, data_8k, 8192, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* 65536 byte encryption */
  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 encrypt (65536 bytes)",
      1000,
      fio_chacha20_poly1305_enc(mac, data_large, 65536, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* With additional data */
  uint8_t ad[64];
  fio_rand_bytes(ad, 64);

  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 encrypt (1024B + 64B AAD)",
      500,
      fio_chacha20_poly1305_enc(mac, data_medium, 1024, ad, 64, key, nonce);
      key[0] ^= mac[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_8k, 8192);
  FIO_MEM_FREE(data_large, 65536);
}

FIO_SFUNC void fio_bench_chacha20_poly1305_dec(void) {
  fprintf(stderr, "    * ChaCha20-Poly1305 Decryption:\n");

  uint8_t key[32], nonce[12];
  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_8k = NULL;
  uint8_t *data_large = NULL;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_8k = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_8k, 8192);
  fio_rand_bytes(data_large, 65536);

  /* Generate valid MACs */
  uint8_t mac_small[16], mac_medium[16], mac_8k[16], mac_large[16];
  fio_chacha20_poly1305_enc(mac_small, data_small, 64, NULL, 0, key, nonce);
  fio_chacha20_poly1305_enc(mac_medium, data_medium, 1024, NULL, 0, key, nonce);
  fio_chacha20_poly1305_enc(mac_8k, data_8k, 8192, NULL, 0, key, nonce);
  fio_chacha20_poly1305_enc(mac_large, data_large, 65536, NULL, 0, key, nonce);

  /* 64 byte decryption
   * Re-encrypt after each decrypt to restore valid ciphertext+MAC.
   * Without this, fio_chacha20_poly1305_dec returns -1 early on iterations 2+
   * (MAC mismatch on plaintext) and skips the ChaCha20 XOR pass entirely. */
  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 decrypt (64 bytes)",
      500,
      fio_chacha20_poly1305_dec(mac_small, data_small, 64, NULL, 0, key, nonce);
      fio_chacha20_poly1305_enc(mac_small, data_small, 64, NULL, 0, key, nonce);
      key[0] ^= mac_small[0];);

  /* 1024 byte decryption */
  FIO_BENCH_CRYPTO("ChaCha20-Poly1305 decrypt (1024 bytes)",
                   500,
                   fio_chacha20_poly1305_dec(mac_medium,
                                             data_medium,
                                             1024,
                                             NULL,
                                             0,
                                             key,
                                             nonce);
                   fio_chacha20_poly1305_enc(mac_medium,
                                             data_medium,
                                             1024,
                                             NULL,
                                             0,
                                             key,
                                             nonce);
                   key[0] ^= mac_medium[0];);

  /* 8192 byte decryption */
  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 decrypt (8192 bytes)",
      500,
      fio_chacha20_poly1305_dec(mac_8k, data_8k, 8192, NULL, 0, key, nonce);
      fio_chacha20_poly1305_enc(mac_8k, data_8k, 8192, NULL, 0, key, nonce);
      key[0] ^= mac_8k[0];);

  /* 65536 byte decryption */
  FIO_BENCH_CRYPTO("ChaCha20-Poly1305 decrypt (65536 bytes)",
                   1000,
                   fio_chacha20_poly1305_dec(mac_large,
                                             data_large,
                                             65536,
                                             NULL,
                                             0,
                                             key,
                                             nonce);
                   fio_chacha20_poly1305_enc(mac_large,
                                             data_large,
                                             65536,
                                             NULL,
                                             0,
                                             key,
                                             nonce);
                   key[0] ^= mac_large[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_8k, 8192);
  FIO_MEM_FREE(data_large, 65536);
}

FIO_SFUNC void fio_bench_chacha20_poly1305_throughput(void) {
  fprintf(stderr, "    * ChaCha20-Poly1305 Throughput:\n");

  uint8_t key[32], nonce[12];
  uint8_t mac[16];
  uint8_t *data = NULL;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);

  /* Standard benchmark sizes: registers, small cache, large cache, out of cache
   */
  size_t sizes[] = {64, 1024, 8192, 65536};

  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t size = sizes[i];
    data = (uint8_t *)FIO_MEM_REALLOC(data, i ? sizes[i - 1] : 0, size, 0);
    fio_rand_bytes(data, size);

    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;

    for (; (clock() - bench_start) < (1000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      fio_chacha20_poly1305_enc(mac, data, size, NULL, 0, key, nonce);
      key[0] ^= mac[0];
    }

    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(size * bench_iterations) / (elapsed * 1024.0 * 1024.0);

    fprintf(stderr,
            "      ChaCha20-Poly1305 %-6zu bytes: %10.2f MB/s\n",
            size,
            throughput_mbps);
  }

  FIO_MEM_FREE(data, sizes[(sizeof(sizes) / sizeof(sizes[0])) - 1]);
}

/* *****************************************************************************
AES-GCM Profiling - Authenticated Encryption
***************************************************************************** */

/* AES-GCM is the most common cipher suite for TLS 1.3:
 * - AES-128-GCM: 128-bit key, 96-bit nonce, 128-bit tag
 * - AES-256-GCM: 256-bit key, 96-bit nonce, 128-bit tag
 * - Uses hardware AES-NI on x86 and ARM Crypto Extensions on ARM
 */

FIO_SFUNC void fio_bench_aes128_gcm_enc(void) {
  fprintf(stderr, "    * AES-128-GCM Encryption:\n");

  uint8_t key[16], nonce[12], mac[16];
  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_8k = NULL;
  uint8_t *data_large = NULL;

  fio_rand_bytes(key, 16);
  fio_rand_bytes(nonce, 12);

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_8k = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_8k, 8192);
  fio_rand_bytes(data_large, 65536);

  /* 64 byte encryption */
  FIO_BENCH_CRYPTO("AES-128-GCM encrypt (64 bytes)",
                   500,
                   fio_aes128_gcm_enc(mac, data_small, 64, NULL, 0, key, nonce);
                   key[0] ^= mac[0];);

  /* 1024 byte encryption */
  FIO_BENCH_CRYPTO(
      "AES-128-GCM encrypt (1024 bytes)",
      500,
      fio_aes128_gcm_enc(mac, data_medium, 1024, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* 8192 byte encryption */
  FIO_BENCH_CRYPTO("AES-128-GCM encrypt (8192 bytes)",
                   500,
                   fio_aes128_gcm_enc(mac, data_8k, 8192, NULL, 0, key, nonce);
                   key[0] ^= mac[0];);

  /* 65536 byte encryption */
  FIO_BENCH_CRYPTO(
      "AES-128-GCM encrypt (65536 bytes)",
      500,
      fio_aes128_gcm_enc(mac, data_large, 65536, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* With additional data */
  uint8_t ad[64];
  fio_rand_bytes(ad, 64);

  FIO_BENCH_CRYPTO(
      "AES-128-GCM encrypt (1024B + 64B AAD)",
      500,
      fio_aes128_gcm_enc(mac, data_medium, 1024, ad, 64, key, nonce);
      key[0] ^= mac[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_8k, 8192);
  FIO_MEM_FREE(data_large, 65536);
}

FIO_SFUNC void fio_bench_aes256_gcm_enc(void) {
  fprintf(stderr, "    * AES-256-GCM Encryption:\n");

  uint8_t key[32], nonce[12], mac[16];
  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_8k = NULL;
  uint8_t *data_large = NULL;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_8k = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_8k, 8192);
  fio_rand_bytes(data_large, 65536);

  /* 64 byte encryption */
  FIO_BENCH_CRYPTO("AES-256-GCM encrypt (64 bytes)",
                   500,
                   fio_aes256_gcm_enc(mac, data_small, 64, NULL, 0, key, nonce);
                   key[0] ^= mac[0];);

  /* 1024 byte encryption */
  FIO_BENCH_CRYPTO(
      "AES-256-GCM encrypt (1024 bytes)",
      500,
      fio_aes256_gcm_enc(mac, data_medium, 1024, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* 8192 byte encryption */
  FIO_BENCH_CRYPTO("AES-256-GCM encrypt (8192 bytes)",
                   500,
                   fio_aes256_gcm_enc(mac, data_8k, 8192, NULL, 0, key, nonce);
                   key[0] ^= mac[0];);

  /* 65536 byte encryption */
  FIO_BENCH_CRYPTO(
      "AES-256-GCM encrypt (65536 bytes)",
      500,
      fio_aes256_gcm_enc(mac, data_large, 65536, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* With additional data */
  uint8_t ad[64];
  fio_rand_bytes(ad, 64);

  FIO_BENCH_CRYPTO(
      "AES-256-GCM encrypt (1024B + 64B AAD)",
      500,
      fio_aes256_gcm_enc(mac, data_medium, 1024, ad, 64, key, nonce);
      key[0] ^= mac[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_8k, 8192);
  FIO_MEM_FREE(data_large, 65536);
}

FIO_SFUNC void fio_bench_aes_gcm_throughput(void) {
  fprintf(stderr, "    * AES-GCM Throughput:\n");

  uint8_t key128[16], key256[32], nonce[12];
  uint8_t mac[16];
  uint8_t *data = NULL;

  fio_rand_bytes(key128, 16);
  fio_rand_bytes(key256, 32);
  fio_rand_bytes(nonce, 12);

  /* Standard benchmark sizes */
  size_t sizes[] = {64, 1024, 8192, 65536};

  /* AES-128-GCM throughput */
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t size = sizes[i];
    data = (uint8_t *)FIO_MEM_REALLOC(data, i ? sizes[i - 1] : 0, size, 0);
    fio_rand_bytes(data, size);

    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;

    for (; (clock() - bench_start) < (500 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      fio_aes128_gcm_enc(mac, data, size, NULL, 0, key128, nonce);
      key128[0] ^= mac[0];
    }

    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(size * bench_iterations) / (elapsed * 1024.0 * 1024.0);

    fprintf(stderr,
            "      AES-128-GCM %-6zu bytes: %10.2f MB/s\n",
            size,
            throughput_mbps);
  }

  /* AES-256-GCM throughput */
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t size = sizes[i];
    data = (uint8_t *)FIO_MEM_REALLOC(data, i ? sizes[i - 1] : 0, size, 0);
    fio_rand_bytes(data, size);

    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;

    for (; (clock() - bench_start) < (500 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      fio_aes256_gcm_enc(mac, data, size, NULL, 0, key256, nonce);
      key256[0] ^= mac[0];
    }

    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(size * bench_iterations) / (elapsed * 1024.0 * 1024.0);

    fprintf(stderr,
            "      AES-256-GCM %-6zu bytes: %10.2f MB/s\n",
            size,
            throughput_mbps);
  }

  FIO_MEM_FREE(data, sizes[(sizeof(sizes) / sizeof(sizes[0])) - 1]);
}

/* OpenSSL AES-GCM AEAD Encryption */
#ifdef HAVE_OPENSSL
FIO_SFUNC void openssl_bench_aes128_gcm_enc(void) {
  fprintf(stderr, "    * OpenSSL AES-128-GCM Encryption:\n");

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char key[16], nonce[12], tag[16];
  unsigned char ad[64];
  unsigned char *data_small = NULL;
  unsigned char *data_medium = NULL;
  unsigned char *data_8k = NULL;
  unsigned char *data_large = NULL;
  int outlen;

  fio_rand_bytes(key, 16);
  fio_rand_bytes(nonce, 12);
  fio_rand_bytes(ad, 64);

  /* Allocate buffers */
  data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_8k = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_8k, 8192);
  fio_rand_bytes(data_large, 65536);

  /* 64 byte encryption */
  FIO_BENCH_CRYPTO("OpenSSL AES-128-GCM encrypt (64 bytes)",
                   500,
                   EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, nonce);
                   EVP_EncryptUpdate(ctx, data_small, &outlen, data_small, 64);
                   EVP_EncryptFinal_ex(ctx, data_small + outlen, &outlen);
                   EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
                   EVP_CIPHER_CTX_reset(ctx););

  /* 1024 byte encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL AES-128-GCM encrypt (1024 bytes)",
      500,
      EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_medium, &outlen, data_medium, 1024);
      EVP_EncryptFinal_ex(ctx, data_medium + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 8192 byte encryption */
  FIO_BENCH_CRYPTO("OpenSSL AES-128-GCM encrypt (8192 bytes)",
                   500,
                   EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, nonce);
                   EVP_EncryptUpdate(ctx, data_8k, &outlen, data_8k, 8192);
                   EVP_EncryptFinal_ex(ctx, data_8k + outlen, &outlen);
                   EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
                   EVP_CIPHER_CTX_reset(ctx););

  /* 65536 byte encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL AES-128-GCM encrypt (65536 bytes)",
      1000,
      EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_large, &outlen, data_large, 65536);
      EVP_EncryptFinal_ex(ctx, data_large + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 1024 byte encryption with 64B AAD */
  FIO_BENCH_CRYPTO(
      "OpenSSL AES-128-GCM encrypt (1024B + 64B AAD)",
      500,
      EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, NULL, &outlen, ad, 64);
      EVP_EncryptUpdate(ctx, data_medium, &outlen, data_medium, 1024);
      EVP_EncryptFinal_ex(ctx, data_medium + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  EVP_CIPHER_CTX_free(ctx);
  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_8k, 8192);
  FIO_MEM_FREE(data_large, 65536);
}

FIO_SFUNC void openssl_bench_aes256_gcm_enc(void) {
  fprintf(stderr, "    * OpenSSL AES-256-GCM Encryption:\n");

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char key[32], nonce[12], tag[16];
  unsigned char ad[64];
  unsigned char *data_small = NULL;
  unsigned char *data_medium = NULL;
  unsigned char *data_8k = NULL;
  unsigned char *data_large = NULL;
  int outlen;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);
  fio_rand_bytes(ad, 64);

  /* Allocate buffers */
  data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_8k = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_8k, 8192);
  fio_rand_bytes(data_large, 65536);

  /* 64 byte encryption */
  FIO_BENCH_CRYPTO("OpenSSL AES-256-GCM encrypt (64 bytes)",
                   500,
                   EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
                   EVP_EncryptUpdate(ctx, data_small, &outlen, data_small, 64);
                   EVP_EncryptFinal_ex(ctx, data_small + outlen, &outlen);
                   EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
                   EVP_CIPHER_CTX_reset(ctx););

  /* 1024 byte encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL AES-256-GCM encrypt (1024 bytes)",
      500,
      EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_medium, &outlen, data_medium, 1024);
      EVP_EncryptFinal_ex(ctx, data_medium + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 8192 byte encryption */
  FIO_BENCH_CRYPTO("OpenSSL AES-256-GCM encrypt (8192 bytes)",
                   500,
                   EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
                   EVP_EncryptUpdate(ctx, data_8k, &outlen, data_8k, 8192);
                   EVP_EncryptFinal_ex(ctx, data_8k + outlen, &outlen);
                   EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
                   EVP_CIPHER_CTX_reset(ctx););

  /* 65536 byte encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL AES-256-GCM encrypt (65536 bytes)",
      1000,
      EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_large, &outlen, data_large, 65536);
      EVP_EncryptFinal_ex(ctx, data_large + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 1024 byte encryption with 64B AAD */
  FIO_BENCH_CRYPTO(
      "OpenSSL AES-256-GCM encrypt (1024B + 64B AAD)",
      500,
      EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, NULL, &outlen, ad, 64);
      EVP_EncryptUpdate(ctx, data_medium, &outlen, data_medium, 1024);
      EVP_EncryptFinal_ex(ctx, data_medium + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  EVP_CIPHER_CTX_free(ctx);
  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_8k, 8192);
  FIO_MEM_FREE(data_large, 65536);
}

FIO_SFUNC void openssl_bench_aes_gcm_throughput(void) {
  fprintf(stderr, "    * OpenSSL AES-GCM Throughput:\n");

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char key128[16], key256[32], nonce[12], tag[16];
  unsigned char *data = NULL;
  int outlen;

  fio_rand_bytes(key128, 16);
  fio_rand_bytes(key256, 32);
  fio_rand_bytes(nonce, 12);

  /* Standard benchmark sizes */
  size_t sizes[] = {64, 1024, 8192, 65536};

  /* AES-128-GCM throughput */
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t size = sizes[i];
    data =
        (unsigned char *)FIO_MEM_REALLOC(data, i ? sizes[i - 1] : 0, size, 0);
    fio_rand_bytes(data, size);

    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;

    for (; (clock() - bench_start) < (1000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, key128, nonce);
      EVP_EncryptUpdate(ctx, data, &outlen, data, (int)size);
      EVP_EncryptFinal_ex(ctx, data + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx);
      key128[0] ^= tag[0];
    }

    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(size * bench_iterations) / (elapsed * 1024.0 * 1024.0);

    fprintf(stderr,
            "      OpenSSL AES-128-GCM %-6zu bytes: %10.2f MB/s\n",
            size,
            throughput_mbps);
  }

  FIO_MEM_FREE(data, sizes[(sizeof(sizes) / sizeof(sizes[0])) - 1]);
  data = NULL;

  /* AES-256-GCM throughput */
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t size = sizes[i];
    data =
        (unsigned char *)FIO_MEM_REALLOC(data, i ? sizes[i - 1] : 0, size, 0);
    fio_rand_bytes(data, size);

    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;

    for (; (clock() - bench_start) < (1000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key256, nonce);
      EVP_EncryptUpdate(ctx, data, &outlen, data, (int)size);
      EVP_EncryptFinal_ex(ctx, data + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx);
      key256[0] ^= tag[0];
    }

    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(size * bench_iterations) / (elapsed * 1024.0 * 1024.0);

    fprintf(stderr,
            "      OpenSSL AES-256-GCM %-6zu bytes: %10.2f MB/s\n",
            size,
            throughput_mbps);
  }

  EVP_CIPHER_CTX_free(ctx);
  FIO_MEM_FREE(data, sizes[(sizeof(sizes) / sizeof(sizes[0])) - 1]);
}
#else  /* !HAVE_OPENSSL */
FIO_SFUNC void openssl_bench_aes128_gcm_enc(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_aes256_gcm_enc(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_aes_gcm_throughput(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
#endif /* HAVE_OPENSSL */

/* *****************************************************************************
RiskyHash Profiling - Non-Cryptographic Hash (Reference)
***************************************************************************** */

/* RiskyHash is a fast non-cryptographic hash function:
 * - 64-bit output, optimized for hash tables and checksums
 * - NOT suitable for cryptographic use (no collision resistance guarantee)
 * - Included as a speed reference to contextualize crypto hash overhead
 */

FIO_SFUNC void fio_bench_risky_hash(void) {
  fprintf(stderr,
          "    * RiskyHash (non-crypto, reference only):\n"
          "      NOTE: NOT a cryptographic hash — included for speed "
          "reference\n");

  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;
  uint8_t *data_xlarge = NULL;
  uint64_t hash = 0;

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO(
      "RiskyHash (64 bytes)", 500, hash = fio_risky_hash(data_small, 64, hash);
      data_small[0] ^= (uint8_t)hash;);

  /* 1024 byte hash */
  FIO_BENCH_CRYPTO("RiskyHash (1024 bytes)",
                   500,
                   hash = fio_risky_hash(data_medium, 1024, hash);
                   data_medium[0] ^= (uint8_t)hash;);

  /* 8192 byte hash */
  FIO_BENCH_CRYPTO("RiskyHash (8192 bytes)",
                   500,
                   hash = fio_risky_hash(data_large, 8192, hash);
                   data_large[0] ^= (uint8_t)hash;);

  /* 65536 byte hash */
  FIO_BENCH_CRYPTO("RiskyHash (65536 bytes)",
                   500,
                   hash = fio_risky_hash(data_xlarge, 65536, hash);
                   data_xlarge[0] ^= (uint8_t)hash;);

  /* --- RiskyHash 256-bit --- */
  {
    fio_u256 h256;
    fprintf(stderr, "    * RiskyHash-256 Throughput:\n");
    FIO_BENCH_CRYPTO(
        "RiskyHash-256 (64 bytes)", 500, h256 = fio_risky256(data_small, 64);
        data_small[0] ^= h256.u8[0];);
    FIO_BENCH_CRYPTO("RiskyHash-256 (1024 bytes)",
                     500,
                     h256 = fio_risky256(data_medium, 1024);
                     data_medium[0] ^= h256.u8[0];);
    FIO_BENCH_CRYPTO("RiskyHash-256 (8192 bytes)",
                     500,
                     h256 = fio_risky256(data_large, 8192);
                     data_large[0] ^= h256.u8[0];);
    FIO_BENCH_CRYPTO("RiskyHash-256 (65536 bytes)",
                     500,
                     h256 = fio_risky256(data_xlarge, 65536);
                     data_xlarge[0] ^= h256.u8[0];);
  }

  /* --- RiskyHash 512-bit --- */
  {
    fio_u512 h512;
    fprintf(stderr, "    * RiskyHash-512 Throughput:\n");
    FIO_BENCH_CRYPTO(
        "RiskyHash-512 (64 bytes)", 500, h512 = fio_risky512(data_small, 64);
        data_small[0] ^= h512.u8[0];);
    FIO_BENCH_CRYPTO("RiskyHash-512 (1024 bytes)",
                     500,
                     h512 = fio_risky512(data_medium, 1024);
                     data_medium[0] ^= h512.u8[0];);
    FIO_BENCH_CRYPTO("RiskyHash-512 (8192 bytes)",
                     500,
                     h512 = fio_risky512(data_large, 8192);
                     data_large[0] ^= h512.u8[0];);
    FIO_BENCH_CRYPTO("RiskyHash-512 (65536 bytes)",
                     500,
                     h512 = fio_risky512(data_xlarge, 65536);
                     data_xlarge[0] ^= h512.u8[0];);
  }

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

/* *****************************************************************************
BLAKE2 Profiling - Fast Cryptographic Hash
***************************************************************************** */

/* BLAKE2 is a high-speed cryptographic hash function:
 * - BLAKE2b: Optimized for 64-bit platforms, produces up to 64-byte digests
 * - BLAKE2s: Optimized for 32-bit platforms, produces up to 32-byte digests
 * - Faster than MD5, SHA-1, SHA-2, and SHA-3 on modern CPUs
 */

FIO_SFUNC void fio_bench_blake2b(void) {
  fprintf(stderr, "    * BLAKE2b-512 Throughput:\n");

  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;
  uint8_t *data_xlarge = NULL;
  fio_u512 out;

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO(
      "BLAKE2b-512 hash (64 bytes)", 500, out = fio_blake2b(data_small, 64);
      data_small[0] ^= out.u8[0];);

  /* 1024 byte hash */
  FIO_BENCH_CRYPTO("BLAKE2b-512 hash (1024 bytes)",
                   500,
                   out = fio_blake2b(data_medium, 1024);
                   data_medium[0] ^= out.u8[0];);

  /* 8192 byte hash */
  FIO_BENCH_CRYPTO(
      "BLAKE2b-512 hash (8192 bytes)", 500, out = fio_blake2b(data_large, 8192);
      data_large[0] ^= out.u8[0];);

  /* 65536 byte hash */
  FIO_BENCH_CRYPTO("BLAKE2b-512 hash (65536 bytes)",
                   500,
                   out = fio_blake2b(data_xlarge, 65536);
                   data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

FIO_SFUNC void fio_bench_blake2s(void) {
  fprintf(stderr, "    * BLAKE2s-256 Throughput:\n");

  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;
  uint8_t *data_xlarge = NULL;
  fio_u256 out;

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO(
      "BLAKE2s-256 hash (64 bytes)", 500, out = fio_blake2s(data_small, 64);
      data_small[0] ^= out.u8[0];);

  /* 1024 byte hash */
  FIO_BENCH_CRYPTO("BLAKE2s-256 hash (1024 bytes)",
                   500,
                   out = fio_blake2s(data_medium, 1024);
                   data_medium[0] ^= out.u8[0];);

  /* 8192 byte hash */
  FIO_BENCH_CRYPTO(
      "BLAKE2s-256 hash (8192 bytes)", 500, out = fio_blake2s(data_large, 8192);
      data_large[0] ^= out.u8[0];);

  /* 65536 byte hash */
  FIO_BENCH_CRYPTO("BLAKE2s-256 hash (65536 bytes)",
                   500,
                   out = fio_blake2s(data_xlarge, 65536);
                   data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

/* *****************************************************************************
SHA-2 Profiling - SHA-256 and SHA-512
***************************************************************************** */

/* SHA-2 is the widely-used cryptographic hash family:
 * - SHA-256: 256-bit output, 64-byte block size
 * - SHA-512: 512-bit output, 128-byte block size
 * - Used in TLS, certificates, HMAC, and many protocols
 */

FIO_SFUNC void fio_bench_sha256(void) {
  fprintf(stderr, "    * SHA-256 Throughput:\n");

  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;
  uint8_t *data_xlarge = NULL;
  fio_u256 out;

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA-256 hash (64 bytes)", 500, out = fio_sha256(data_small, 64);
      data_small[0] ^= out.u8[0];);

  /* 1024 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA-256 hash (1024 bytes)", 500, out = fio_sha256(data_medium, 1024);
      data_medium[0] ^= out.u8[0];);

  /* 8192 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA-256 hash (8192 bytes)", 500, out = fio_sha256(data_large, 8192);
      data_large[0] ^= out.u8[0];);

  /* 65536 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA-256 hash (65536 bytes)", 500, out = fio_sha256(data_xlarge, 65536);
      data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

FIO_SFUNC void fio_bench_sha512(void) {
  fprintf(stderr, "    * SHA-512 Throughput:\n");

  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;
  uint8_t *data_xlarge = NULL;
  fio_u512 out;

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA-512 hash (64 bytes)", 500, out = fio_sha512(data_small, 64);
      data_small[0] ^= out.u8[0];);

  /* 1024 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA-512 hash (1024 bytes)", 500, out = fio_sha512(data_medium, 1024);
      data_medium[0] ^= out.u8[0];);

  /* 8192 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA-512 hash (8192 bytes)", 500, out = fio_sha512(data_large, 8192);
      data_large[0] ^= out.u8[0];);

  /* 65536 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA-512 hash (65536 bytes)", 500, out = fio_sha512(data_xlarge, 65536);
      data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

/* *****************************************************************************
SHA-3 Profiling - NIST Standard Hash
***************************************************************************** */

/* SHA-3 is the NIST standard based on Keccak:
 * - SHA3-256: 256-bit output, 136-byte rate (1088 bits)
 * - SHA3-512: 512-bit output, 72-byte rate (576 bits)
 * - Uses sponge construction with Keccak-f[1600] permutation
 */

FIO_SFUNC void fio_bench_sha3_256(void) {
  fprintf(stderr, "    * SHA3-256 Throughput:\n");

  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;
  uint8_t *data_xlarge = NULL;
  uint8_t out[32];

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-256 hash (64 bytes)", 500, fio_sha3_256(out, data_small, 64);
      data_small[0] ^= out[0];);

  /* 1024 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-256 hash (1024 bytes)", 500, fio_sha3_256(out, data_medium, 1024);
      data_medium[0] ^= out[0];);

  /* 8192 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-256 hash (8192 bytes)", 500, fio_sha3_256(out, data_large, 8192);
      data_large[0] ^= out[0];);

  /* 65536 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-256 hash (65536 bytes)", 500, fio_sha3_256(out, data_xlarge, 65536);
      data_xlarge[0] ^= out[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

FIO_SFUNC void fio_bench_sha3_512(void) {
  fprintf(stderr, "    * SHA3-512 Throughput:\n");

  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;
  uint8_t *data_xlarge = NULL;
  uint8_t out[64];

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-512 hash (64 bytes)", 500, fio_sha3_512(out, data_small, 64);
      data_small[0] ^= out[0];);

  /* 1024 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-512 hash (1024 bytes)", 500, fio_sha3_512(out, data_medium, 1024);
      data_medium[0] ^= out[0];);

  /* 8192 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-512 hash (8192 bytes)", 500, fio_sha3_512(out, data_large, 8192);
      data_large[0] ^= out[0];);

  /* 65536 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-512 hash (65536 bytes)", 500, fio_sha3_512(out, data_xlarge, 65536);
      data_xlarge[0] ^= out[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

/* *****************************************************************************
HMAC Functions Profiling
***************************************************************************** */

/* HMAC (Hash-based Message Authentication Code) provides message authentication
 * using a cryptographic hash function combined with a secret key.
 * Standard sizes: 64B (small), 1024B (medium), 8192B (large), 65536B (xlarge)
 */

FIO_SFUNC void fio_bench_hmac_blake2b(void) {
  fprintf(stderr, "    * HMAC-BLAKE2b-512 Throughput:\n");

  static const uint8_t hmac_key[64] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  };

  uint8_t *data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  uint8_t *data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  uint8_t *data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  uint8_t *data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);
  fio_u512 out;

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  FIO_BENCH_CRYPTO("HMAC-BLAKE2b-512 (64B msg)",
                   500,
                   out = fio_blake2b_hmac(hmac_key, 64, data_small, 64);
                   data_small[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-BLAKE2b-512 (1024B msg)",
                   500,
                   out = fio_blake2b_hmac(hmac_key, 64, data_medium, 1024);
                   data_medium[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-BLAKE2b-512 (8192B msg)",
                   500,
                   out = fio_blake2b_hmac(hmac_key, 64, data_large, 8192);
                   data_large[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-BLAKE2b-512 (65536B msg)",
                   500,
                   out = fio_blake2b_hmac(hmac_key, 64, data_xlarge, 65536);
                   data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

FIO_SFUNC void fio_bench_hmac_blake2s(void) {
  fprintf(stderr, "    * HMAC-BLAKE2s-256 Throughput:\n");

  static const uint8_t hmac_key[32] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  };

  uint8_t *data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  uint8_t *data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  uint8_t *data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  uint8_t *data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);
  fio_u256 out;

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  FIO_BENCH_CRYPTO("HMAC-BLAKE2s-256 (64B msg)",
                   500,
                   out = fio_blake2s_hmac(hmac_key, 32, data_small, 64);
                   data_small[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-BLAKE2s-256 (1024B msg)",
                   500,
                   out = fio_blake2s_hmac(hmac_key, 32, data_medium, 1024);
                   data_medium[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-BLAKE2s-256 (8192B msg)",
                   500,
                   out = fio_blake2s_hmac(hmac_key, 32, data_large, 8192);
                   data_large[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-BLAKE2s-256 (65536B msg)",
                   500,
                   out = fio_blake2s_hmac(hmac_key, 32, data_xlarge, 65536);
                   data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

FIO_SFUNC void fio_bench_hmac_sha256(void) {
  fprintf(stderr, "    * HMAC-SHA256 Throughput:\n");

  static const uint8_t hmac_key[32] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  };

  uint8_t *data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  uint8_t *data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  uint8_t *data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  uint8_t *data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);
  fio_u256 out;

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  FIO_BENCH_CRYPTO("HMAC-SHA256 (64B msg)",
                   500,
                   out = fio_sha256_hmac(hmac_key, 32, data_small, 64);
                   data_small[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-SHA256 (1024B msg)",
                   500,
                   out = fio_sha256_hmac(hmac_key, 32, data_medium, 1024);
                   data_medium[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-SHA256 (8192B msg)",
                   500,
                   out = fio_sha256_hmac(hmac_key, 32, data_large, 8192);
                   data_large[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-SHA256 (65536B msg)",
                   500,
                   out = fio_sha256_hmac(hmac_key, 32, data_xlarge, 65536);
                   data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

FIO_SFUNC void fio_bench_hmac_sha512(void) {
  fprintf(stderr, "    * HMAC-SHA512 Throughput:\n");

  static const uint8_t hmac_key[64] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  };

  uint8_t *data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  uint8_t *data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  uint8_t *data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  uint8_t *data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);
  fio_u512 out;

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  FIO_BENCH_CRYPTO("HMAC-SHA512 (64B msg)",
                   500,
                   out = fio_sha512_hmac(hmac_key, 64, data_small, 64);
                   data_small[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-SHA512 (1024B msg)",
                   500,
                   out = fio_sha512_hmac(hmac_key, 64, data_medium, 1024);
                   data_medium[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-SHA512 (8192B msg)",
                   500,
                   out = fio_sha512_hmac(hmac_key, 64, data_large, 8192);
                   data_large[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-SHA512 (65536B msg)",
                   500,
                   out = fio_sha512_hmac(hmac_key, 64, data_xlarge, 65536);
                   data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

/* RiskyHash HMAC Functions (non-cryptographic, for comparison only) */

FIO_SFUNC void fio_bench_hmac_risky256(void) {
  fprintf(stderr, "    * HMAC-RiskyHash-256 (non-cryptographic) Throughput:\n");

  static const uint8_t hmac_key[32] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  };

  uint8_t *data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  uint8_t *data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  uint8_t *data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  uint8_t *data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);
  fio_u256 out;

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  FIO_BENCH_CRYPTO("HMAC-RiskyHash-256 (64B msg)",
                   500,
                   out = fio_risky256_hmac(hmac_key, 32, data_small, 64);
                   data_small[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-RiskyHash-256 (1024B msg)",
                   500,
                   out = fio_risky256_hmac(hmac_key, 32, data_medium, 1024);
                   data_medium[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-RiskyHash-256 (8192B msg)",
                   500,
                   out = fio_risky256_hmac(hmac_key, 32, data_large, 8192);
                   data_large[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-RiskyHash-256 (65536B msg)",
                   500,
                   out = fio_risky256_hmac(hmac_key, 32, data_xlarge, 65536);
                   data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

FIO_SFUNC void fio_bench_hmac_risky512(void) {
  fprintf(stderr, "    * HMAC-RiskyHash-512 (non-cryptographic) Throughput:\n");

  static const uint8_t hmac_key[32] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  };

  uint8_t *data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  uint8_t *data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  uint8_t *data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  uint8_t *data_xlarge = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);
  fio_u512 out;

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  FIO_BENCH_CRYPTO("HMAC-RiskyHash-512 (64B msg)",
                   500,
                   out = fio_risky512_hmac(hmac_key, 32, data_small, 64);
                   data_small[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-RiskyHash-512 (1024B msg)",
                   500,
                   out = fio_risky512_hmac(hmac_key, 32, data_medium, 1024);
                   data_medium[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-RiskyHash-512 (8192B msg)",
                   500,
                   out = fio_risky512_hmac(hmac_key, 32, data_large, 8192);
                   data_large[0] ^= out.u8[0];);

  FIO_BENCH_CRYPTO("HMAC-RiskyHash-512 (65536B msg)",
                   500,
                   out = fio_risky512_hmac(hmac_key, 32, data_xlarge, 65536);
                   data_xlarge[0] ^= out.u8[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

/* OpenSSL HMAC Functions */
#ifdef HAVE_OPENSSL
#include <openssl/hmac.h>

FIO_SFUNC void openssl_bench_hmac_sha256(void) {
  fprintf(stderr, "    * OpenSSL HMAC-SHA256 Throughput:\n");

  static const unsigned char hmac_key[32] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  };

  unsigned char *data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  unsigned char *data_medium =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  unsigned char *data_large =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  unsigned char *data_xlarge =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);
  unsigned char out[32];
  unsigned int out_len;

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  FIO_BENCH_CRYPTO(
      "OpenSSL HMAC-SHA256 (64B msg)",
      500,
      HMAC(EVP_sha256(), hmac_key, 32, data_small, 64, out, &out_len);
      data_small[0] ^= out[0];);

  FIO_BENCH_CRYPTO(
      "OpenSSL HMAC-SHA256 (1024B msg)",
      500,
      HMAC(EVP_sha256(), hmac_key, 32, data_medium, 1024, out, &out_len);
      data_medium[0] ^= out[0];);

  FIO_BENCH_CRYPTO(
      "OpenSSL HMAC-SHA256 (8192B msg)",
      500,
      HMAC(EVP_sha256(), hmac_key, 32, data_large, 8192, out, &out_len);
      data_large[0] ^= out[0];);

  FIO_BENCH_CRYPTO(
      "OpenSSL HMAC-SHA256 (65536B msg)",
      500,
      HMAC(EVP_sha256(), hmac_key, 32, data_xlarge, 65536, out, &out_len);
      data_xlarge[0] ^= out[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

FIO_SFUNC void openssl_bench_hmac_sha512(void) {
  fprintf(stderr, "    * OpenSSL HMAC-SHA512 Throughput:\n");

  static const unsigned char hmac_key[64] = {
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  };

  unsigned char *data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  unsigned char *data_medium =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  unsigned char *data_large =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  unsigned char *data_xlarge =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);
  unsigned char out[64];
  unsigned int out_len;

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  FIO_BENCH_CRYPTO(
      "OpenSSL HMAC-SHA512 (64B msg)",
      500,
      HMAC(EVP_sha512(), hmac_key, 64, data_small, 64, out, &out_len);
      data_small[0] ^= out[0];);

  FIO_BENCH_CRYPTO(
      "OpenSSL HMAC-SHA512 (1024B msg)",
      500,
      HMAC(EVP_sha512(), hmac_key, 64, data_medium, 1024, out, &out_len);
      data_medium[0] ^= out[0];);

  FIO_BENCH_CRYPTO(
      "OpenSSL HMAC-SHA512 (8192B msg)",
      500,
      HMAC(EVP_sha512(), hmac_key, 64, data_large, 8192, out, &out_len);
      data_large[0] ^= out[0];);

  FIO_BENCH_CRYPTO(
      "OpenSSL HMAC-SHA512 (65536B msg)",
      500,
      HMAC(EVP_sha512(), hmac_key, 64, data_xlarge, 65536, out, &out_len);
      data_xlarge[0] ^= out[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

/* Note: OpenSSL does not support HMAC-BLAKE2 in the standard HMAC API.
 * BLAKE2 has its own keyed mode which is different from HMAC construction.
 * OpenSSL's EVP_blake2b512/EVP_blake2s256 don't work with HMAC(). */
FIO_SFUNC void openssl_bench_hmac_blake2(void) {
  fprintf(stderr,
          "      [OpenSSL]  HMAC-BLAKE2 (unavailable - BLAKE2 uses native "
          "keying, not HMAC)\n");
}
#else  /* !HAVE_OPENSSL */
FIO_SFUNC void openssl_bench_hmac_sha256(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_hmac_sha512(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_hmac_blake2(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
#endif /* HAVE_OPENSSL */

/* *****************************************************************************
Synthetic Multiplication Profiling in Crypto Context
***************************************************************************** */

/* These benchmarks isolate the multiplication operations to show
 * the raw impact of the optimizations on crypto-relevant sizes */

FIO_SFUNC void fio_bench_mul_crypto_sizes(void) {
  fprintf(stderr, "    * Multiplication at Crypto-Relevant Sizes:\n");

  /* 256-bit (Ed25519, secp256k1, P-256) */
  {
    uint64_t a[4], b[4], result[8];
    for (int i = 0; i < 4; ++i) {
      a[i] = fio_rand64();
      b[i] = fio_rand64();
    }

    FIO_BENCH_CRYPTO("256-bit multiplication (Ed25519 field ops)",
                     5000,
                     fio_math_mul(result, a, b, 4);
                     a[0] ^= result[0];);
  }

  /* 384-bit (P-384, Curve383187) */
  {
    uint64_t a[6], b[6], result[12];
    for (int i = 0; i < 6; ++i) {
      a[i] = fio_rand64();
      b[i] = fio_rand64();
    }

    FIO_BENCH_CRYPTO("384-bit multiplication (P-384 field ops)",
                     5000,
                     fio_math_mul(result, a, b, 6);
                     a[0] ^= result[0];);
  }

  /* 512-bit (Curve25519 intermediate, RSA-512 deprecated) */
  {
    uint64_t a[8], b[8], result[16];
    for (int i = 0; i < 8; ++i) {
      a[i] = fio_rand64();
      b[i] = fio_rand64();
    }

    FIO_BENCH_CRYPTO("512-bit multiplication (intermediate values)",
                     5000,
                     fio_math_mul(result, a, b, 8);
                     a[0] ^= result[0];);
  }

  /* 1024-bit (RSA-1024 deprecated, DH-1024) */
  {
    uint64_t a[16], b[16], result[32];
    for (int i = 0; i < 16; ++i) {
      a[i] = fio_rand64();
      b[i] = fio_rand64();
    }

    FIO_BENCH_CRYPTO("1024-bit multiplication (RSA-1024, DH-1024)",
                     5000,
                     fio_math_mul(result, a, b, 16);
                     a[0] ^= result[0];);
  }

  /* 2048-bit (RSA-2048, DH-2048) */
  {
    uint64_t a[32], b[32], result[64];
    for (int i = 0; i < 32; ++i) {
      a[i] = fio_rand64();
      b[i] = fio_rand64();
    }

    FIO_BENCH_CRYPTO("2048-bit multiplication (RSA-2048, DH-2048)",
                     5000,
                     fio_math_mul(result, a, b, 32);
                     a[0] ^= result[0];);
  }

  /* 4096-bit (RSA-4096, DH-4096) */
  {
    uint64_t a[64], b[64], result[128];
    for (int i = 0; i < 64; ++i) {
      a[i] = fio_rand64();
      b[i] = fio_rand64();
    }

    FIO_BENCH_CRYPTO("4096-bit multiplication (RSA-4096, DH-4096)",
                     500,
                     fio_math_mul(result, a, b, 64);
                     a[0] ^= result[0];);
  }
}

/* *****************************************************************************
OpenSSL Comparison Benchmarks - Direct Head-to-Head
***************************************************************************** */

#ifdef HAVE_OPENSSL
/* OpenSSL Ed25519 Key Generation */
FIO_SFUNC void openssl_bench_ed25519_keygen(void) {
  fprintf(stderr, "    * OpenSSL Ed25519 Key Generation:\n");

  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *pkey_ctx = NULL;

  FIO_BENCH_CRYPTO("OpenSSL Ed25519 public key generation",
                   500,
                   pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
                   EVP_PKEY_keygen_init(pkey_ctx);
                   EVP_PKEY_keygen(pkey_ctx, &pkey);
                   EVP_PKEY_CTX_free(pkey_ctx);
                   EVP_PKEY_free(pkey);
                   pkey = NULL;);
}

/* OpenSSL Ed25519 Signature Generation */
FIO_SFUNC void openssl_bench_ed25519_sign(void) {
  fprintf(stderr, "    * OpenSSL Ed25519 Signature Generation:\n");

  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *pkey_ctx = NULL;
  EVP_MD_CTX *md_ctx = NULL;
  unsigned char sig[64];
  size_t sig_len;

  /* Generate Ed25519 key pair */
  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
  if (!pkey_ctx) {
    fprintf(stderr, "ERROR: Failed to create OpenSSL Ed25519 context\n");
    return;
  }

  if (EVP_PKEY_keygen_init(pkey_ctx) <= 0 ||
      EVP_PKEY_keygen(pkey_ctx, &pkey) <= 0) {
    fprintf(stderr, "ERROR: Failed to generate OpenSSL Ed25519 key\n");
    EVP_PKEY_CTX_free(pkey_ctx);
    return;
  }
  EVP_PKEY_CTX_free(pkey_ctx);

  md_ctx = EVP_MD_CTX_new();
  if (!md_ctx) {
    fprintf(stderr, "ERROR: Failed to create OpenSSL MD context\n");
    EVP_PKEY_free(pkey);
    return;
  }

  unsigned char *msg_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  unsigned char *msg_medium =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  unsigned char *msg_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  unsigned char *msg_xlarge =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(msg_small, 64);
  fio_rand_bytes(msg_medium, 1024);
  fio_rand_bytes(msg_large, 8192);
  fio_rand_bytes(msg_xlarge, 65536);

  /* Benchmark 64 byte message signing */
  FIO_BENCH_CRYPTO(
      "OpenSSL Ed25519 sign (64 bytes)", 500, sig_len = sizeof(sig);
      EVP_DigestSignInit(md_ctx, NULL, NULL, NULL, pkey);
      EVP_DigestSign(md_ctx, sig, &sig_len, msg_small, 64);
      EVP_MD_CTX_reset(md_ctx););

  /* Benchmark 1024 byte message signing */
  FIO_BENCH_CRYPTO(
      "OpenSSL Ed25519 sign (1024 bytes)", 500, sig_len = sizeof(sig);
      EVP_DigestSignInit(md_ctx, NULL, NULL, NULL, pkey);
      EVP_DigestSign(md_ctx, sig, &sig_len, msg_medium, 1024);
      EVP_MD_CTX_reset(md_ctx););

  /* Benchmark 8192 byte message signing */
  FIO_BENCH_CRYPTO(
      "OpenSSL Ed25519 sign (8192 bytes)", 500, sig_len = sizeof(sig);
      EVP_DigestSignInit(md_ctx, NULL, NULL, NULL, pkey);
      EVP_DigestSign(md_ctx, sig, &sig_len, msg_large, 8192);
      EVP_MD_CTX_reset(md_ctx););

  /* Benchmark 65536 byte message signing */
  FIO_BENCH_CRYPTO(
      "OpenSSL Ed25519 sign (65536 bytes)", 500, sig_len = sizeof(sig);
      EVP_DigestSignInit(md_ctx, NULL, NULL, NULL, pkey);
      EVP_DigestSign(md_ctx, sig, &sig_len, msg_xlarge, 65536);
      EVP_MD_CTX_reset(md_ctx););

  EVP_MD_CTX_free(md_ctx);
  EVP_PKEY_free(pkey);
  FIO_MEM_FREE(msg_small, 64);
  FIO_MEM_FREE(msg_medium, 1024);
  FIO_MEM_FREE(msg_large, 8192);
  FIO_MEM_FREE(msg_xlarge, 65536);
}

/* OpenSSL Ed25519 Signature Verification */
FIO_SFUNC void openssl_bench_ed25519_verify(void) {
  fprintf(stderr, "    * OpenSSL Ed25519 Signature Verification:\n");

  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *pkey_ctx = NULL;
  EVP_MD_CTX *md_ctx = NULL;
  EVP_MD_CTX *sign_ctx = NULL;
  unsigned char sig[64];
  size_t sig_len = sizeof(sig);

  /* Generate key and prepare contexts */
  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
  if (!pkey_ctx) {
    fprintf(stderr, "ERROR: Failed to create OpenSSL Ed25519 context\n");
    return;
  }

  EVP_PKEY_keygen_init(pkey_ctx);
  EVP_PKEY_keygen(pkey_ctx, &pkey);
  EVP_PKEY_CTX_free(pkey_ctx);

  md_ctx = EVP_MD_CTX_new();
  sign_ctx = EVP_MD_CTX_new();

  unsigned char *msg_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  unsigned char *msg_medium =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  unsigned char *msg_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  unsigned char *msg_xlarge =
      (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(msg_small, 64);
  fio_rand_bytes(msg_medium, 1024);
  fio_rand_bytes(msg_large, 8192);
  fio_rand_bytes(msg_xlarge, 65536);

  /* Sign 64 byte message for verification benchmark */
  sig_len = sizeof(sig);
  EVP_DigestSignInit(sign_ctx, NULL, NULL, NULL, pkey);
  EVP_DigestSign(sign_ctx, sig, &sig_len, msg_small, 64);
  EVP_MD_CTX_reset(sign_ctx);

  FIO_BENCH_CRYPTO("OpenSSL Ed25519 verify (64 bytes)",
                   500,
                   EVP_DigestVerifyInit(md_ctx, NULL, NULL, NULL, pkey);
                   EVP_DigestVerify(md_ctx, sig, sig_len, msg_small, 64);
                   EVP_MD_CTX_reset(md_ctx););

  /* Sign 1024 byte message for verification benchmark */
  sig_len = sizeof(sig);
  EVP_DigestSignInit(sign_ctx, NULL, NULL, NULL, pkey);
  EVP_DigestSign(sign_ctx, sig, &sig_len, msg_medium, 1024);
  EVP_MD_CTX_reset(sign_ctx);

  FIO_BENCH_CRYPTO("OpenSSL Ed25519 verify (1024 bytes)",
                   500,
                   EVP_DigestVerifyInit(md_ctx, NULL, NULL, NULL, pkey);
                   EVP_DigestVerify(md_ctx, sig, sig_len, msg_medium, 1024);
                   EVP_MD_CTX_reset(md_ctx););

  /* Sign 8192 byte message for verification benchmark */
  sig_len = sizeof(sig);
  EVP_DigestSignInit(sign_ctx, NULL, NULL, NULL, pkey);
  EVP_DigestSign(sign_ctx, sig, &sig_len, msg_large, 8192);
  EVP_MD_CTX_reset(sign_ctx);

  FIO_BENCH_CRYPTO("OpenSSL Ed25519 verify (8192 bytes)",
                   500,
                   EVP_DigestVerifyInit(md_ctx, NULL, NULL, NULL, pkey);
                   EVP_DigestVerify(md_ctx, sig, sig_len, msg_large, 8192);
                   EVP_MD_CTX_reset(md_ctx););

  /* Sign 65536 byte message for verification benchmark */
  sig_len = sizeof(sig);
  EVP_DigestSignInit(sign_ctx, NULL, NULL, NULL, pkey);
  EVP_DigestSign(sign_ctx, sig, &sig_len, msg_xlarge, 65536);
  EVP_MD_CTX_reset(sign_ctx);

  FIO_BENCH_CRYPTO("OpenSSL Ed25519 verify (65536 bytes)",
                   500,
                   EVP_DigestVerifyInit(md_ctx, NULL, NULL, NULL, pkey);
                   EVP_DigestVerify(md_ctx, sig, sig_len, msg_xlarge, 65536);
                   EVP_MD_CTX_reset(md_ctx););

  EVP_MD_CTX_free(sign_ctx);
  EVP_MD_CTX_free(md_ctx);
  EVP_PKEY_free(pkey);
  FIO_MEM_FREE(msg_small, 64);
  FIO_MEM_FREE(msg_medium, 1024);
  FIO_MEM_FREE(msg_large, 8192);
  FIO_MEM_FREE(msg_xlarge, 65536);
}

/* OpenSSL Ed25519 Sign+Verify Roundtrip */
FIO_SFUNC void openssl_bench_ed25519_roundtrip(void) {
  fprintf(stderr, "    * OpenSSL Ed25519 Sign+Verify Roundtrip:\n");

  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *pkey_ctx = NULL;
  EVP_MD_CTX *sign_ctx = NULL;
  EVP_MD_CTX *verify_ctx = NULL;
  unsigned char sig[64];
  size_t sig_len;

  /* Generate key */
  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
  EVP_PKEY_keygen_init(pkey_ctx);
  EVP_PKEY_keygen(pkey_ctx, &pkey);
  EVP_PKEY_CTX_free(pkey_ctx);

  sign_ctx = EVP_MD_CTX_new();
  verify_ctx = EVP_MD_CTX_new();

  unsigned char *msg = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  fio_rand_bytes(msg, 1024);

  FIO_BENCH_CRYPTO("OpenSSL Ed25519 sign+verify roundtrip (1024 bytes)",
                   500,
                   sig_len = sizeof(sig);
                   EVP_DigestSignInit(sign_ctx, NULL, NULL, NULL, pkey);
                   EVP_DigestSign(sign_ctx, sig, &sig_len, msg, 1024);
                   EVP_MD_CTX_reset(sign_ctx);
                   EVP_DigestVerifyInit(verify_ctx, NULL, NULL, NULL, pkey);
                   EVP_DigestVerify(verify_ctx, sig, sig_len, msg, 1024);
                   EVP_MD_CTX_reset(verify_ctx););

  EVP_MD_CTX_free(sign_ctx);
  EVP_MD_CTX_free(verify_ctx);
  EVP_PKEY_free(pkey);
  FIO_MEM_FREE(msg, 1024);
}
#else  /* !HAVE_OPENSSL */
FIO_SFUNC void openssl_bench_ed25519_keygen(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_ed25519_sign(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_ed25519_verify(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_ed25519_roundtrip(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
#endif /* HAVE_OPENSSL */

/* OpenSSL X25519 Key Exchange */
#ifdef HAVE_OPENSSL
FIO_SFUNC void openssl_bench_x25519(void) {
  fprintf(stderr, "    * OpenSSL X25519 Key Exchange:\n");

  EVP_PKEY *pkey1 = NULL, *pkey2 = NULL;
  EVP_PKEY_CTX *pkey_ctx = NULL;
  EVP_PKEY_CTX *derive_ctx = NULL;
  EVP_PKEY_CTX *derive_ctx2 = NULL;
  unsigned char shared_secret1[32], shared_secret2[32];
  size_t secret_len;

  /* Benchmark public key generation */
  FIO_BENCH_CRYPTO("OpenSSL X25519 public key generation",
                   500,
                   pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
                   EVP_PKEY_keygen_init(pkey_ctx);
                   EVP_PKEY_keygen(pkey_ctx, &pkey1);
                   EVP_PKEY_CTX_free(pkey_ctx);
                   EVP_PKEY_free(pkey1);
                   pkey1 = NULL;);

  /* Generate key pairs for shared secret benchmark */
  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
  EVP_PKEY_keygen_init(pkey_ctx);
  EVP_PKEY_keygen(pkey_ctx, &pkey1);
  EVP_PKEY_CTX_free(pkey_ctx);

  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
  EVP_PKEY_keygen_init(pkey_ctx);
  EVP_PKEY_keygen(pkey_ctx, &pkey2);
  EVP_PKEY_CTX_free(pkey_ctx);

  /* Benchmark shared secret derivation */
  derive_ctx = EVP_PKEY_CTX_new(pkey1, NULL);

  FIO_BENCH_CRYPTO("OpenSSL X25519 shared secret derivation",
                   500,
                   secret_len = sizeof(shared_secret1);
                   EVP_PKEY_derive_init(derive_ctx);
                   EVP_PKEY_derive_set_peer(derive_ctx, pkey2);
                   EVP_PKEY_derive(derive_ctx, shared_secret1, &secret_len);
                   EVP_PKEY_CTX_free(derive_ctx);
                   derive_ctx = EVP_PKEY_CTX_new(pkey1, NULL););

  EVP_PKEY_CTX_free(derive_ctx);
  EVP_PKEY_free(pkey1);
  EVP_PKEY_free(pkey2);
  pkey1 = NULL;
  pkey2 = NULL;

  /* Benchmark full key exchange (both sides) - generates fresh keys each iter
   */
  FIO_BENCH_CRYPTO("OpenSSL X25519 full key exchange (both sides)",
                   500,
                   /* Generate both key pairs */
                   pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
                   EVP_PKEY_keygen_init(pkey_ctx);
                   EVP_PKEY_keygen(pkey_ctx, &pkey1);
                   EVP_PKEY_CTX_free(pkey_ctx);

                   pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
                   EVP_PKEY_keygen_init(pkey_ctx);
                   EVP_PKEY_keygen(pkey_ctx, &pkey2);
                   EVP_PKEY_CTX_free(pkey_ctx);

                   /* Derive shared secrets from both sides */
                   derive_ctx = EVP_PKEY_CTX_new(pkey1, NULL);
                   secret_len = sizeof(shared_secret1);
                   EVP_PKEY_derive_init(derive_ctx);
                   EVP_PKEY_derive_set_peer(derive_ctx, pkey2);
                   EVP_PKEY_derive(derive_ctx, shared_secret1, &secret_len);
                   EVP_PKEY_CTX_free(derive_ctx);

                   derive_ctx2 = EVP_PKEY_CTX_new(pkey2, NULL);
                   secret_len = sizeof(shared_secret2);
                   EVP_PKEY_derive_init(derive_ctx2);
                   EVP_PKEY_derive_set_peer(derive_ctx2, pkey1);
                   EVP_PKEY_derive(derive_ctx2, shared_secret2, &secret_len);
                   EVP_PKEY_CTX_free(derive_ctx2);

                   EVP_PKEY_free(pkey1);
                   EVP_PKEY_free(pkey2);
                   pkey1 = NULL;
                   pkey2 = NULL;);
}
#else  /* !HAVE_OPENSSL */
FIO_SFUNC void openssl_bench_x25519(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
#endif /* HAVE_OPENSSL */

/* OpenSSL ChaCha20-Poly1305 AEAD Encryption */
#ifdef HAVE_OPENSSL
FIO_SFUNC void openssl_bench_chacha20_poly1305_enc(void) {
  fprintf(stderr, "    * OpenSSL ChaCha20-Poly1305 Encryption:\n");

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char key[32], nonce[12], tag[16];
  unsigned char ad[64];
  unsigned char *data_small = NULL;
  unsigned char *data_medium = NULL;
  unsigned char *data_8k = NULL;
  unsigned char *data_large = NULL;
  int outlen;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);
  fio_rand_bytes(ad, 64);

  /* Allocate buffers */
  data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_8k = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_8k, 8192);
  fio_rand_bytes(data_large, 65536);

  /* 64 byte encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 encrypt (64 bytes)",
      500,
      EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_small, &outlen, data_small, 64);
      EVP_EncryptFinal_ex(ctx, data_small + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 1024 byte encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 encrypt (1024 bytes)",
      500,
      EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_medium, &outlen, data_medium, 1024);
      EVP_EncryptFinal_ex(ctx, data_medium + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 8192 byte encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 encrypt (8192 bytes)",
      500,
      EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_8k, &outlen, data_8k, 8192);
      EVP_EncryptFinal_ex(ctx, data_8k + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 65536 byte encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 encrypt (65536 bytes)",
      1000,
      EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_large, &outlen, data_large, 65536);
      EVP_EncryptFinal_ex(ctx, data_large + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 1024 byte encryption with 64B AAD */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 encrypt (1024B + 64B AAD)",
      500,
      EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, NULL, &outlen, ad, 64);
      EVP_EncryptUpdate(ctx, data_medium, &outlen, data_medium, 1024);
      EVP_EncryptFinal_ex(ctx, data_medium + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  EVP_CIPHER_CTX_free(ctx);
  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_8k, 8192);
  FIO_MEM_FREE(data_large, 65536);
}

/* OpenSSL ChaCha20-Poly1305 AEAD Decryption */
FIO_SFUNC void openssl_bench_chacha20_poly1305_dec(void) {
  fprintf(stderr, "    * OpenSSL ChaCha20-Poly1305 Decryption:\n");

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char key[32], nonce[12];
  unsigned char tag_small[16], tag_medium[16], tag_8k[16], tag_large[16];
  unsigned char *data_small = NULL;
  unsigned char *data_medium = NULL;
  unsigned char *data_8k = NULL;
  unsigned char *data_large = NULL;
  int outlen;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);

  /* Allocate buffers */
  data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_8k = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_8k, 8192);
  fio_rand_bytes(data_large, 65536);

  /* Generate valid tags by encrypting first */
  EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
  EVP_EncryptUpdate(ctx, data_small, &outlen, data_small, 64);
  EVP_EncryptFinal_ex(ctx, data_small + outlen, &outlen);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag_small);
  EVP_CIPHER_CTX_reset(ctx);

  EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
  EVP_EncryptUpdate(ctx, data_medium, &outlen, data_medium, 1024);
  EVP_EncryptFinal_ex(ctx, data_medium + outlen, &outlen);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag_medium);
  EVP_CIPHER_CTX_reset(ctx);

  EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
  EVP_EncryptUpdate(ctx, data_8k, &outlen, data_8k, 8192);
  EVP_EncryptFinal_ex(ctx, data_8k + outlen, &outlen);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag_8k);
  EVP_CIPHER_CTX_reset(ctx);

  EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
  EVP_EncryptUpdate(ctx, data_large, &outlen, data_large, 65536);
  EVP_EncryptFinal_ex(ctx, data_large + outlen, &outlen);
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag_large);
  EVP_CIPHER_CTX_reset(ctx);

  /* 64 byte decryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 decrypt (64 bytes)",
      500,
      EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_DecryptUpdate(ctx, data_small, &outlen, data_small, 64);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag_small);
      EVP_DecryptFinal_ex(ctx, data_small + outlen, &outlen);
      EVP_CIPHER_CTX_reset(ctx););

  /* 1024 byte decryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 decrypt (1024 bytes)",
      500,
      EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_DecryptUpdate(ctx, data_medium, &outlen, data_medium, 1024);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag_medium);
      EVP_DecryptFinal_ex(ctx, data_medium + outlen, &outlen);
      EVP_CIPHER_CTX_reset(ctx););

  /* 8192 byte decryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 decrypt (8192 bytes)",
      500,
      EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_DecryptUpdate(ctx, data_8k, &outlen, data_8k, 8192);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag_8k);
      EVP_DecryptFinal_ex(ctx, data_8k + outlen, &outlen);
      EVP_CIPHER_CTX_reset(ctx););

  /* 65536 byte decryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 decrypt (65536 bytes)",
      1000,
      EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_DecryptUpdate(ctx, data_large, &outlen, data_large, 65536);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag_large);
      EVP_DecryptFinal_ex(ctx, data_large + outlen, &outlen);
      EVP_CIPHER_CTX_reset(ctx););

  EVP_CIPHER_CTX_free(ctx);
  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_8k, 8192);
  FIO_MEM_FREE(data_large, 65536);
}

/* OpenSSL ChaCha20-Poly1305 Throughput Sweep */
FIO_SFUNC void openssl_bench_chacha20_poly1305_throughput(void) {
  fprintf(stderr, "    * OpenSSL ChaCha20-Poly1305 Throughput:\n");

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char key[32], nonce[12], tag[16];
  unsigned char *data = NULL;
  int outlen;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);

  /* Standard benchmark sizes: registers, small cache, large cache, out of cache
   */
  size_t sizes[] = {64, 1024, 8192, 65536};

  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t size = sizes[i];
    data =
        (unsigned char *)FIO_MEM_REALLOC(data, i ? sizes[i - 1] : 0, size, 0);
    fio_rand_bytes(data, size);

    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;

    for (; (clock() - bench_start) < (1000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data, &outlen, data, (int)size);
      EVP_EncryptFinal_ex(ctx, data + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx);
      key[0] ^= tag[0];
    }

    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(size * bench_iterations) / (elapsed * 1024.0 * 1024.0);

    fprintf(stderr,
            "      OpenSSL ChaCha20-Poly1305 %-6zu bytes: %10.2f MB/s\n",
            size,
            throughput_mbps);
  }

  EVP_CIPHER_CTX_free(ctx);
  FIO_MEM_FREE(data, sizes[(sizeof(sizes) / sizeof(sizes[0])) - 1]);
}
#else  /* !HAVE_OPENSSL */
FIO_SFUNC void openssl_bench_chacha20_poly1305_enc(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_chacha20_poly1305_dec(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_chacha20_poly1305_throughput(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
#endif /* HAVE_OPENSSL */

/* OpenSSL BLAKE2 Hash Functions */
#ifdef HAVE_OPENSSL
FIO_SFUNC void openssl_bench_blake2(void) {
  fprintf(stderr, "    * OpenSSL BLAKE2b-512 Throughput:\n");

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  unsigned char *data_small = NULL;
  unsigned char *data_medium = NULL;
  unsigned char *data_large = NULL;
  unsigned char *data_xlarge = NULL;
  unsigned char out[64];
  unsigned int outlen;

  /* Allocate buffers */
  data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_xlarge = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  /* BLAKE2b-512: 64 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2b-512 hash (64 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
                   EVP_DigestUpdate(ctx, data_small, 64);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2b-512: 1024 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2b-512 hash (1024 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
                   EVP_DigestUpdate(ctx, data_medium, 1024);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2b-512: 8192 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2b-512 hash (8192 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
                   EVP_DigestUpdate(ctx, data_large, 8192);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2b-512: 65536 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2b-512 hash (65536 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
                   EVP_DigestUpdate(ctx, data_xlarge, 65536);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  fprintf(stderr, "    * OpenSSL BLAKE2s-256 Throughput:\n");

  /* BLAKE2s-256: 64 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2s-256 hash (64 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
                   EVP_DigestUpdate(ctx, data_small, 64);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2s-256: 1024 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2s-256 hash (1024 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
                   EVP_DigestUpdate(ctx, data_medium, 1024);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2s-256: 8192 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2s-256 hash (8192 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
                   EVP_DigestUpdate(ctx, data_large, 8192);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2s-256: 65536 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2s-256 hash (65536 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
                   EVP_DigestUpdate(ctx, data_xlarge, 65536);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  EVP_MD_CTX_free(ctx);
  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}

/* OpenSSL SHA-3 Hash Functions */
FIO_SFUNC void openssl_bench_sha3(void) {
  fprintf(stderr, "    * OpenSSL SHA3-256 Throughput:\n");

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  unsigned char *data_small = NULL;
  unsigned char *data_medium = NULL;
  unsigned char *data_large = NULL;
  unsigned char *data_xlarge = NULL;
  unsigned char out[64];
  unsigned int outlen;

  /* Allocate buffers */
  data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);
  data_xlarge = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);
  fio_rand_bytes(data_xlarge, 65536);

  /* SHA3-256: 64 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-256 hash (64 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
                   EVP_DigestUpdate(ctx, data_small, 64);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-256: 1024 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-256 hash (1024 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
                   EVP_DigestUpdate(ctx, data_medium, 1024);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-256: 8192 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-256 hash (8192 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
                   EVP_DigestUpdate(ctx, data_large, 8192);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-256: 65536 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-256 hash (65536 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
                   EVP_DigestUpdate(ctx, data_xlarge, 65536);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  fprintf(stderr, "    * OpenSSL SHA3-512 Throughput:\n");

  /* SHA3-512: 64 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-512 hash (64 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL);
                   EVP_DigestUpdate(ctx, data_small, 64);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-512: 1024 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-512 hash (1024 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL);
                   EVP_DigestUpdate(ctx, data_medium, 1024);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-512: 8192 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-512 hash (8192 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL);
                   EVP_DigestUpdate(ctx, data_large, 8192);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-512: 65536 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-512 hash (65536 bytes)",
                   500,
                   EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL);
                   EVP_DigestUpdate(ctx, data_xlarge, 65536);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  EVP_MD_CTX_free(ctx);
  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
  FIO_MEM_FREE(data_xlarge, 65536);
}
#else  /* !HAVE_OPENSSL */
FIO_SFUNC void openssl_bench_blake2(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_sha3(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
#endif /* HAVE_OPENSSL */

/* OpenSSL BIGNUM Multiplication */
#ifdef HAVE_OPENSSL
FIO_SFUNC void openssl_bench_bn_multiplication(void) {
  fprintf(stderr, "    * OpenSSL BIGNUM Multiplication:\n");

  BN_CTX *bn_ctx = BN_CTX_new();
  BIGNUM *a = BN_new();
  BIGNUM *b = BN_new();
  BIGNUM *result = BN_new();

  /* 256-bit multiplication */
  BN_rand(a, 256, -1, 0);
  BN_rand(b, 256, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (256-bit)", 500, BN_mul(result, a, b, bn_ctx););

  /* 384-bit multiplication */
  BN_rand(a, 384, -1, 0);
  BN_rand(b, 384, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (384-bit)", 500, BN_mul(result, a, b, bn_ctx););

  /* 512-bit multiplication */
  BN_rand(a, 512, -1, 0);
  BN_rand(b, 512, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (512-bit)", 500, BN_mul(result, a, b, bn_ctx););

  /* 1024-bit multiplication */
  BN_rand(a, 1024, -1, 0);
  BN_rand(b, 1024, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (1024-bit)", 500, BN_mul(result, a, b, bn_ctx););

  /* 2048-bit multiplication */
  BN_rand(a, 2048, -1, 0);
  BN_rand(b, 2048, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (2048-bit)", 500, BN_mul(result, a, b, bn_ctx););

  /* 4096-bit multiplication */
  BN_rand(a, 4096, -1, 0);
  BN_rand(b, 4096, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (4096-bit)", 500, BN_mul(result, a, b, bn_ctx););

  BN_free(a);
  BN_free(b);
  BN_free(result);
  BN_CTX_free(bn_ctx);
}
#else  /* !HAVE_OPENSSL */
FIO_SFUNC void openssl_bench_bn_multiplication(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
#endif /* HAVE_OPENSSL */

/* *****************************************************************************
OpenSSL ML-KEM-768 Benchmarks (requires OpenSSL 3.5+)
***************************************************************************** */

/* OpenSSL X25519 Baseline (for KEM comparison, one-side only) */
#ifdef FIO___PERF_HAS_OPENSSL_MLKEM
FIO_SFUNC void openssl_bench_x25519_baseline(void) {
  fprintf(stderr, "    * OpenSSL X25519 Baseline (for KEM comparison):\n");

  EVP_PKEY *pkey1 = NULL, *pkey2 = NULL;
  EVP_PKEY_CTX *pkey_ctx = NULL;
  EVP_PKEY_CTX *derive_ctx = NULL;
  unsigned char shared_secret[32];
  size_t secret_len;

  /* Generate first key pair */
  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
  if (!pkey_ctx) {
    fprintf(stderr, "      OpenSSL X25519: N/A (context creation failed)\n");
    return;
  }
  EVP_PKEY_keygen_init(pkey_ctx);
  EVP_PKEY_keygen(pkey_ctx, &pkey1);
  EVP_PKEY_CTX_free(pkey_ctx);

  /* Generate second key pair */
  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
  EVP_PKEY_keygen_init(pkey_ctx);
  EVP_PKEY_keygen(pkey_ctx, &pkey2);
  EVP_PKEY_CTX_free(pkey_ctx);

  FIO_BENCH_CRYPTO(
      "OpenSSL X25519 keypair generation (baseline)", 500, EVP_PKEY *tmp = NULL;
      pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
      if (pkey_ctx) {
        EVP_PKEY_keygen_init(pkey_ctx);
        EVP_PKEY_keygen(pkey_ctx, &tmp);
        EVP_PKEY_CTX_free(pkey_ctx);
        if (tmp)
          EVP_PKEY_free(tmp);
      });

  FIO_BENCH_CRYPTO(
      "OpenSSL X25519 shared secret (baseline)",
      500,
      derive_ctx = EVP_PKEY_CTX_new(pkey1, NULL);
      if (derive_ctx) {
        secret_len = sizeof(shared_secret);
        EVP_PKEY_derive_init(derive_ctx);
        EVP_PKEY_derive_set_peer(derive_ctx, pkey2);
        EVP_PKEY_derive(derive_ctx, shared_secret, &secret_len);
        EVP_PKEY_CTX_free(derive_ctx);
      });

  EVP_PKEY_free(pkey1);
  EVP_PKEY_free(pkey2);
}
#elif defined(HAVE_OPENSSL)
FIO_SFUNC void openssl_bench_x25519_baseline(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable - requires OpenSSL 3.5+)\n");
}
#else
FIO_SFUNC void openssl_bench_x25519_baseline(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
#endif

/* OpenSSL ML-KEM-768 Key Generation */
#ifdef FIO___PERF_HAS_OPENSSL_MLKEM
FIO_SFUNC void openssl_bench_mlkem768_keypair(void) {
  fprintf(stderr, "    * OpenSSL ML-KEM-768 Key Generation:\n");

  EVP_PKEY *pkey = NULL;

  FIO_BENCH_CRYPTO(
      "OpenSSL ML-KEM-768 keypair generation",
      500,
      pkey = EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768");
      if (pkey) {
        EVP_PKEY_free(pkey);
        pkey = NULL;
      });
}

/* OpenSSL ML-KEM-768 Encapsulation */
FIO_SFUNC void openssl_bench_mlkem768_encaps(void) {
  fprintf(stderr, "    * OpenSSL ML-KEM-768 Encapsulation:\n");

  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *ctx = NULL;
  unsigned char ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  unsigned char ss[FIO_MLKEM768_SSBYTES];
  size_t ct_len, ss_len;

  pkey = EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768");
  if (!pkey) {
    fprintf(stderr,
            "      OpenSSL ML-KEM-768 encapsulation: N/A (keygen failed)\n");
    return;
  }

  ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pkey, NULL);
  if (!ctx || EVP_PKEY_encapsulate_init(ctx, NULL) <= 0) {
    fprintf(stderr,
            "      OpenSSL ML-KEM-768 encapsulation: N/A (ctx init failed)\n");
    EVP_PKEY_free(pkey);
    if (ctx)
      EVP_PKEY_CTX_free(ctx);
    return;
  }

  FIO_BENCH_CRYPTO("OpenSSL ML-KEM-768 encapsulation", 500, ct_len = sizeof(ct);
                   ss_len = sizeof(ss);
                   (void)EVP_PKEY_encapsulate(ctx, ct, &ct_len, ss, &ss_len););

  EVP_PKEY_CTX_free(ctx);
  EVP_PKEY_free(pkey);
}

/* OpenSSL ML-KEM-768 Decapsulation */
FIO_SFUNC void openssl_bench_mlkem768_decaps(void) {
  fprintf(stderr, "    * OpenSSL ML-KEM-768 Decapsulation:\n");

  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *enc_ctx = NULL;
  EVP_PKEY_CTX *dec_ctx = NULL;
  unsigned char ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  unsigned char ss_enc[FIO_MLKEM768_SSBYTES];
  unsigned char ss_dec[FIO_MLKEM768_SSBYTES];
  size_t ct_len, ss_len;

  pkey = EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768");
  if (!pkey) {
    fprintf(stderr,
            "      OpenSSL ML-KEM-768 decapsulation: N/A (keygen failed)\n");
    return;
  }

  enc_ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pkey, NULL);
  if (!enc_ctx || EVP_PKEY_encapsulate_init(enc_ctx, NULL) <= 0) {
    fprintf(
        stderr,
        "      OpenSSL ML-KEM-768 decapsulation: N/A (encaps init failed)\n");
    EVP_PKEY_free(pkey);
    if (enc_ctx)
      EVP_PKEY_CTX_free(enc_ctx);
    return;
  }

  ct_len = sizeof(ct);
  ss_len = sizeof(ss_enc);
  if (EVP_PKEY_encapsulate(enc_ctx, ct, &ct_len, ss_enc, &ss_len) <= 0) {
    fprintf(stderr,
            "      OpenSSL ML-KEM-768 decapsulation: N/A (encaps failed)\n");
    EVP_PKEY_CTX_free(enc_ctx);
    EVP_PKEY_free(pkey);
    return;
  }
  EVP_PKEY_CTX_free(enc_ctx);

  dec_ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pkey, NULL);
  if (!dec_ctx || EVP_PKEY_decapsulate_init(dec_ctx, NULL) <= 0) {
    fprintf(
        stderr,
        "      OpenSSL ML-KEM-768 decapsulation: N/A (decaps init failed)\n");
    EVP_PKEY_free(pkey);
    if (dec_ctx)
      EVP_PKEY_CTX_free(dec_ctx);
    return;
  }

  FIO_BENCH_CRYPTO(
      "OpenSSL ML-KEM-768 decapsulation", 500, ss_len = sizeof(ss_dec);
      (void)EVP_PKEY_decapsulate(dec_ctx, ss_dec, &ss_len, ct, ct_len););

  EVP_PKEY_CTX_free(dec_ctx);
  EVP_PKEY_free(pkey);
}

/* OpenSSL ML-KEM-768 Full Roundtrip */
FIO_SFUNC void openssl_bench_mlkem768_roundtrip(void) {
  fprintf(stderr, "    * OpenSSL ML-KEM-768 Full Roundtrip:\n");

  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *enc_ctx = NULL;
  EVP_PKEY_CTX *dec_ctx = NULL;
  unsigned char ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  unsigned char ss_enc[FIO_MLKEM768_SSBYTES];
  unsigned char ss_dec[FIO_MLKEM768_SSBYTES];
  size_t ct_len, ss_len;

  FIO_BENCH_CRYPTO(
      "OpenSSL ML-KEM-768 full roundtrip (keygen+encaps+decaps)",
      500,
      /* Keygen */
      pkey = EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768");
      if (pkey) {
        /* Encapsulate */
        enc_ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pkey, NULL);
        if (enc_ctx && EVP_PKEY_encapsulate_init(enc_ctx, NULL) > 0) {
          ct_len = sizeof(ct);
          ss_len = sizeof(ss_enc);
          (void)EVP_PKEY_encapsulate(enc_ctx, ct, &ct_len, ss_enc, &ss_len);
          /* Decapsulate */
          dec_ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pkey, NULL);
          if (dec_ctx && EVP_PKEY_decapsulate_init(dec_ctx, NULL) > 0) {
            ss_len = sizeof(ss_dec);
            (void)EVP_PKEY_decapsulate(dec_ctx, ss_dec, &ss_len, ct, ct_len);
          }
          if (dec_ctx)
            EVP_PKEY_CTX_free(dec_ctx);
        }
        if (enc_ctx)
          EVP_PKEY_CTX_free(enc_ctx);
        EVP_PKEY_free(pkey);
      } pkey = NULL;
      enc_ctx = NULL;
      dec_ctx = NULL;);
}
#elif defined(HAVE_OPENSSL)
FIO_SFUNC void openssl_bench_mlkem768_keypair(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable - requires OpenSSL 3.5+)\n");
}
FIO_SFUNC void openssl_bench_mlkem768_encaps(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable - requires OpenSSL 3.5+)\n");
}
FIO_SFUNC void openssl_bench_mlkem768_decaps(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable - requires OpenSSL 3.5+)\n");
}
FIO_SFUNC void openssl_bench_mlkem768_roundtrip(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable - requires OpenSSL 3.5+)\n");
}
#else
FIO_SFUNC void openssl_bench_mlkem768_keypair(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_mlkem768_encaps(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_mlkem768_decaps(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
FIO_SFUNC void openssl_bench_mlkem768_roundtrip(void) {
  fprintf(stderr, "      [OpenSSL]  (unavailable)\n");
}
#endif

/* *****************************************************************************
X25519 Profiling - Elliptic Curve Diffie-Hellman
***************************************************************************** */

FIO_SFUNC void fio_bench_x25519(void) {
  fprintf(stderr, "    * X25519 Key Exchange:\n");

  uint8_t sk1[32], sk2[32], pk1[32], pk2[32], shared1[32], shared2[32];

  fio_rand_bytes(sk1, 32);
  fio_rand_bytes(sk2, 32);

  /* Key generation (scalar multiplication by base point) */
  FIO_BENCH_CRYPTO(
      "X25519 public key generation", 500, fio_x25519_public_key(pk1, sk1);
      sk1[0] ^= pk1[0];);

  /* Shared secret derivation (scalar multiplication) */
  fio_x25519_public_key(pk1, sk1);
  fio_x25519_public_key(pk2, sk2);

  FIO_BENCH_CRYPTO("X25519 shared secret derivation",
                   500,
                   int result = fio_x25519_shared_secret(shared1, sk1, pk2);
                   sk1[0] ^= (uint8_t)result;);

  /* Full key exchange roundtrip */
  FIO_BENCH_CRYPTO("X25519 full key exchange (both sides)",
                   500,
                   fio_x25519_public_key(pk1, sk1);
                   fio_x25519_public_key(pk2, sk2);
                   fio_x25519_shared_secret(shared1, sk1, pk2);
                   fio_x25519_shared_secret(shared2, sk2, pk1);
                   sk1[0] ^= shared1[0] ^ shared2[0];);
}

/* *****************************************************************************
X25519 Baseline for ML-KEM Comparison (one-side only)
***************************************************************************** */

/* This provides a one-side-only X25519 baseline (keygen + shared secret)
 * that matches the semantics of ML-KEM encaps (one side generates, one side
 * derives). The full two-sided exchange is in fio_bench_x25519() above. */

FIO_SFUNC void fio_bench_x25519_baseline(void) {
  fprintf(stderr, "    * X25519 Baseline (for KEM comparison):\n");

  uint8_t sk1[32], sk2[32], pk1[32], pk2[32], ss[32];

  fio_rand_bytes(sk1, 32);
  fio_rand_bytes(sk2, 32);

  FIO_BENCH_CRYPTO("X25519 keypair generation (baseline)",
                   500,
                   fio_x25519_public_key(pk1, sk1);
                   sk1[0] ^= pk1[0];);

  fio_x25519_public_key(pk1, sk1);
  fio_x25519_public_key(pk2, sk2);

  FIO_BENCH_CRYPTO("X25519 shared secret (baseline)",
                   500,
                   fio_x25519_shared_secret(ss, sk1, pk2);
                   sk1[0] ^= ss[0];);

  FIO_BENCH_CRYPTO("X25519 full exchange (keygen+shared)",
                   500,
                   fio_x25519_public_key(pk1, sk1);
                   fio_x25519_shared_secret(ss, sk1, pk2);
                   sk1[0] ^= ss[0];);
}

/* *****************************************************************************
ML-KEM-768 Profiling - Post-Quantum Key Encapsulation
***************************************************************************** */

/* ML-KEM-768 is a NIST-standardized post-quantum KEM:
 * - Based on Module-LWE (Learning With Errors) over polynomial rings
 * - NIST Level 3 security (192-bit equivalent)
 * - Key sizes: pk=1184, sk=2400, ct=1088, ss=32
 * - Uses SHA3 (SHAKE) internally for hashing and PRF
 */

FIO_SFUNC void fio_bench_mlkem768_keypair(void) {
  fprintf(stderr, "    * ML-KEM-768 Key Generation:\n");

  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];

  FIO_BENCH_CRYPTO(
      "ML-KEM-768 keypair generation", 500, (void)fio_mlkem768_keypair(pk, sk);
      pk[0] ^= sk[0];);
}

FIO_SFUNC void fio_bench_mlkem768_encaps(void) {
  fprintf(stderr, "    * ML-KEM-768 Encapsulation:\n");

  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss[FIO_MLKEM768_SSBYTES];

  (void)fio_mlkem768_keypair(pk, sk);

  FIO_BENCH_CRYPTO(
      "ML-KEM-768 encapsulation", 500, (void)fio_mlkem768_encaps(ct, ss, pk);
      pk[0] ^= ss[0];);
}

FIO_SFUNC void fio_bench_mlkem768_decaps(void) {
  fprintf(stderr, "    * ML-KEM-768 Decapsulation:\n");

  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss_enc[FIO_MLKEM768_SSBYTES];
  uint8_t ss_dec[FIO_MLKEM768_SSBYTES];

  (void)fio_mlkem768_keypair(pk, sk);
  (void)fio_mlkem768_encaps(ct, ss_enc, pk);

  FIO_BENCH_CRYPTO("ML-KEM-768 decapsulation",
                   500,
                   (void)fio_mlkem768_decaps(ss_dec, ct, sk);
                   sk[0] ^= ss_dec[0];);
}

FIO_SFUNC void fio_bench_mlkem768_roundtrip(void) {
  fprintf(stderr, "    * ML-KEM-768 Full Roundtrip:\n");

  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss_enc[FIO_MLKEM768_SSBYTES];
  uint8_t ss_dec[FIO_MLKEM768_SSBYTES];

  FIO_BENCH_CRYPTO("ML-KEM-768 full roundtrip (keygen+encaps+decaps)",
                   500,
                   (void)fio_mlkem768_keypair(pk, sk);
                   (void)fio_mlkem768_encaps(ct, ss_enc, pk);
                   (void)fio_mlkem768_decaps(ss_dec, ct, sk);
                   pk[0] ^= ss_dec[0];);
}

/* *****************************************************************************
X25519MLKEM768 Profiling - Hybrid Post-Quantum Key Exchange
***************************************************************************** */

/* X25519MLKEM768 is the TLS 1.3 hybrid key exchange (NamedGroup 0x11ec):
 * - Combines X25519 (classical) with ML-KEM-768 (post-quantum)
 * - Key sizes: pk=1216, sk=2432, ct=1120, ss=64
 * - Byte order: ML-KEM first, X25519 second (IETF
 * draft-ietf-tls-ecdhe-mlkem-03)
 */

FIO_SFUNC void fio_bench_x25519mlkem768_keypair(void) {
  fprintf(stderr, "    * X25519MLKEM768 Key Generation:\n");

  uint8_t pk[FIO_X25519MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_X25519MLKEM768_SECRETKEYBYTES];

  FIO_BENCH_CRYPTO("X25519MLKEM768 keypair generation",
                   500,
                   (void)fio_x25519mlkem768_keypair(pk, sk);
                   pk[0] ^= sk[0];);
}

FIO_SFUNC void fio_bench_x25519mlkem768_encaps(void) {
  fprintf(stderr, "    * X25519MLKEM768 Encapsulation:\n");

  uint8_t pk[FIO_X25519MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_X25519MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_X25519MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss[FIO_X25519MLKEM768_SSBYTES];

  (void)fio_x25519mlkem768_keypair(pk, sk);

  FIO_BENCH_CRYPTO("X25519MLKEM768 encapsulation",
                   500,
                   (void)fio_x25519mlkem768_encaps(ct, ss, pk);
                   pk[0] ^= ss[0];);
}

FIO_SFUNC void fio_bench_x25519mlkem768_decaps(void) {
  fprintf(stderr, "    * X25519MLKEM768 Decapsulation:\n");

  uint8_t pk[FIO_X25519MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_X25519MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_X25519MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss_enc[FIO_X25519MLKEM768_SSBYTES];
  uint8_t ss_dec[FIO_X25519MLKEM768_SSBYTES];

  (void)fio_x25519mlkem768_keypair(pk, sk);
  (void)fio_x25519mlkem768_encaps(ct, ss_enc, pk);

  FIO_BENCH_CRYPTO("X25519MLKEM768 decapsulation",
                   500,
                   (void)fio_x25519mlkem768_decaps(ss_dec, ct, sk);
                   sk[0] ^= ss_dec[0];);
}

FIO_SFUNC void fio_bench_x25519mlkem768_roundtrip(void) {
  fprintf(stderr, "    * X25519MLKEM768 Full Roundtrip:\n");

  uint8_t pk[FIO_X25519MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_X25519MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_X25519MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss_enc[FIO_X25519MLKEM768_SSBYTES];
  uint8_t ss_dec[FIO_X25519MLKEM768_SSBYTES];

  FIO_BENCH_CRYPTO("X25519MLKEM768 full roundtrip (keygen+encaps+decaps)",
                   500,
                   (void)fio_x25519mlkem768_keypair(pk, sk);
                   (void)fio_x25519mlkem768_encaps(ct, ss_enc, pk);
                   (void)fio_x25519mlkem768_decaps(ss_dec, ct, sk);
                   pk[0] ^= ss_dec[0];);
}

/* *****************************************************************************
Password Hashing / KDF Profiling - Lyra2 & Argon2
***************************************************************************** */

/* Lyra2 is a memory-hard password hashing scheme:
 * - Uses a matrix of (m_cost * n_cols) blocks of 96 bytes each
 * - t_cost controls time cost (number of passes)
 * - Memory-hard: resists GPU/ASIC attacks by requiring large memory
 *
 * Argon2 (winner of the Password Hashing Competition):
 * - Argon2id: hybrid of Argon2i (data-independent) and Argon2d (data-dependent)
 * - m_cost in KB, t_cost = time iterations, parallelism = lanes
 * - Recommended for password storage and key derivation
 */

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

FIO_SFUNC void fio_bench_kdf(void) {
  fprintf(stderr, "    * Password Hashing (KDF) Benchmarks:\n");

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
  fprintf(stderr, "\n      Lyra2:\n");
  fprintf(stderr,
          "      %-50s %10s  %10s\n",
          "Configuration",
          "ops/sec",
          "memory KB");
  fprintf(stderr,
          "      ---------------------------------------------------------"
          "------------\n");

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
            "      %-50s %10.2f  %10.1f\n",
            cfg->name,
            ops_sec,
            matrix_kb);
  }

  /* ---- Argon2 (matching memory tiers, p=1 for single-threaded comparison) */
  fprintf(stderr, "\n      Argon2:\n");
  fprintf(stderr,
          "      %-50s %10s  %10s\n",
          "Configuration",
          "ops/sec",
          "memory KB");
  fprintf(stderr,
          "      ---------------------------------------------------------"
          "------------\n");

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

    fprintf(stderr, "      %-50s %10.2f  %10.1f\n", cfg->name, ops_sec, mem_kb);
  }
}

/* OpenSSL Argon2id benchmark (requires OpenSSL 3.2+) */
#ifdef HAVE_OPENSSL
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
  fprintf(stderr, "    * OpenSSL Argon2id (KDF comparison):\n");

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
          "      %-50s %10s  %10s\n",
          "Configuration",
          "ops/sec",
          "mem KB");
  fprintf(stderr,
          "      ---------------------------------------------------------"
          "------------\n");

  for (size_t i = 0; i < n_configs; ++i) {
    const fio___perf_openssl_argon2_config_s *cfg = &configs[i];

    EVP_KDF *kdf = EVP_KDF_fetch(NULL, "ARGON2ID", NULL);
    if (!kdf) {
      fprintf(stderr, "      %-50s %10s  %10s\n", cfg->name, "N/A", "N/A");
      fprintf(stderr, "      (OpenSSL Argon2id not available in this build)\n");
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
            "      %-50s %10.2f  %10.1f\n",
            cfg->name,
            ops_sec,
            (double)cfg->m_cost_kb);
  }
}
#endif /* OPENSSL_VERSION_NUMBER >= 0x30200000L */
#endif /* HAVE_OPENSSL */

/* Stub for OpenSSL Argon2id when unavailable */
#ifndef FIO___PERF_HAS_OPENSSL_ARGON2
FIO_SFUNC void openssl_bench_argon2id(void) {
  fprintf(stderr, "      [OpenSSL Argon2id]  (unavailable)\n");
}
#endif

/* *****************************************************************************
Key Size Summary
***************************************************************************** */

FIO_SFUNC void fio_bench_kem_key_sizes(void) {
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Key Size Summary (bytes)\n"
          "---------------------------------------------------\n");
  fprintf(stderr,
          "      Algorithm              PK       SK       CT       SS\n");
  fprintf(stderr,
          "      ---------------------------------------------------\n");
  fprintf(stderr,
          "      X25519                 32       32       32       32\n");
  fprintf(stderr,
          "      ML-KEM-768           %4d     %4d     %4d       %2d\n",
          FIO_MLKEM768_PUBLICKEYBYTES,
          FIO_MLKEM768_SECRETKEYBYTES,
          FIO_MLKEM768_CIPHERTEXTBYTES,
          FIO_MLKEM768_SSBYTES);
  fprintf(stderr,
          "      X25519MLKEM768       %4d     %4d     %4d       %2d\n",
          FIO_X25519MLKEM768_PUBLICKEYBYTES,
          FIO_X25519MLKEM768_SECRETKEYBYTES,
          FIO_X25519MLKEM768_CIPHERTEXTBYTES,
          FIO_X25519MLKEM768_SSBYTES);
  fprintf(stderr, "\n");
  fprintf(stderr, "      PK = Public Key, SK = Secret Key\n");
  fprintf(stderr, "      CT = Ciphertext, SS = Shared Secret\n");
  fprintf(stderr, "      X25519MLKEM768 is the TLS 1.3 hybrid (0x11ec)\n");
  fprintf(stderr,
          "      ML-KEM-768 provides NIST Level 3 (192-bit) security\n\n");
}

/* *****************************************************************************
Platform / Build Info Helper
***************************************************************************** */

FIO_SFUNC void fio_bench_print_platform_info(void) {
  fprintf(stderr,
          "\n"
          "=======================================================\n"
          "  Platform / Compiler / OpenSSL Info\n"
          "=======================================================\n\n");
  fprintf(stderr, "  Platform: ");
#if defined(__APPLE__)
  fprintf(stderr, "macOS");
#if defined(__aarch64__) || defined(__arm64__)
  fprintf(stderr, " (Apple Silicon)");
#endif
#elif defined(__linux__)
  fprintf(stderr, "Linux");
#elif defined(_WIN32)
  fprintf(stderr, "Windows");
#else
  fprintf(stderr, "Unknown");
#endif

  fprintf(stderr, "\n  Compiler: ");
#if defined(__clang__)
  fprintf(stderr,
          "clang %d.%d.%d",
          __clang_major__,
          __clang_minor__,
          __clang_patchlevel__);
#elif defined(__GNUC__)
  fprintf(stderr,
          "gcc %d.%d.%d",
          __GNUC__,
          __GNUC_MINOR__,
          __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
  fprintf(stderr, "MSVC %d", _MSC_VER);
#else
  fprintf(stderr, "unknown");
#endif

#ifdef HAVE_OPENSSL
  fprintf(stderr, "\n  OpenSSL: %s\n\n", OPENSSL_VERSION_TEXT);
#else
  fprintf(stderr, "\n  OpenSSL: (unavailable)\n\n");
#endif
}

/* *****************************************************************************
Main Test Runner
***************************************************************************** */

int main(void) {
#if defined(DEBUG) && (DEBUG)
  if (1) {
    fprintf(stderr, "\t- Skipped in DEBUG\n");
    return 0;
  }
#endif
  fprintf(stderr,
          "\n"
          "=======================================================\n"
          "  Crypto Performance: facil.io vs OpenSSL\n"
          "  Real-World Performance Comparison\n"
          "=======================================================\n\n");

  fprintf(stderr, "Multiplication is heavily used in:\n");
  fprintf(stderr,
          "  - Ed25519: ~25 mulc64 per field mul, ~250 field muls per "
          "sign/verify\n");
  fprintf(stderr, "  - Poly1305: ~9 mulc64 per 16-byte block processed\n");
  fprintf(stderr, "  - X25519: Similar to Ed25519 (scalar multiplication)\n\n");

  /* ===================================================================
     Ed25519 Digital Signatures
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Ed25519 Digital Signatures (256-bit field ops)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_ed25519_keygen();
  fio_bench_ed25519_sign();
  fio_bench_ed25519_verify();
  fio_bench_ed25519_roundtrip();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_ed25519_keygen();
  openssl_bench_ed25519_sign();
  openssl_bench_ed25519_verify();
  openssl_bench_ed25519_roundtrip();

  /* ===================================================================
     X25519 Key Exchange
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  X25519 Key Exchange (256-bit scalar mul)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_x25519();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_x25519();

  /* ===================================================================
     ChaCha20-Poly1305 AEAD
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  ChaCha20-Poly1305 AEAD (130-bit field ops)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_chacha20_poly1305_enc();
  fio_bench_chacha20_poly1305_dec();
  fio_bench_chacha20_poly1305_throughput();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_chacha20_poly1305_enc();
  openssl_bench_chacha20_poly1305_dec();
  openssl_bench_chacha20_poly1305_throughput();

  /* ===================================================================
     AES-GCM AEAD
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  AES-GCM AEAD (TLS 1.3 cipher suite)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_aes128_gcm_enc();
  fio_bench_aes256_gcm_enc();
  fio_bench_aes_gcm_throughput();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_aes128_gcm_enc();
  openssl_bench_aes256_gcm_enc();
  openssl_bench_aes_gcm_throughput();

  /* ===================================================================
     Hash Functions
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Hash Functions\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_risky_hash();
  fio_bench_blake2b();
  fio_bench_blake2s();
  fio_bench_sha256();
  fio_bench_sha512();
  fio_bench_sha3_256();
  fio_bench_sha3_512();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_blake2();
  openssl_bench_sha3();

  /* ===================================================================
     HMAC Functions
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  HMAC Functions (Message Authentication)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_hmac_blake2b();
  fio_bench_hmac_blake2s();
  fio_bench_hmac_sha256();
  fio_bench_hmac_sha512();
  fio_bench_hmac_risky256();
  fio_bench_hmac_risky512();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_hmac_blake2();
  openssl_bench_hmac_sha256();
  openssl_bench_hmac_sha512();

  /* ===================================================================
     Big Number Multiplication
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Multiplication (Crypto-Relevant Sizes)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_mul_crypto_sizes();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_bn_multiplication();

  /* ===================================================================
     X25519 Baseline (for KEM comparison)
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  X25519 Baseline (one-side, for KEM comparison)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_x25519_baseline();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_x25519_baseline();

  /* ===================================================================
     ML-KEM-768 Post-Quantum KEM
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  ML-KEM-768 Key Encapsulation (Post-Quantum)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_mlkem768_keypair();
  fio_bench_mlkem768_encaps();
  fio_bench_mlkem768_decaps();
  fio_bench_mlkem768_roundtrip();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_mlkem768_keypair();
  openssl_bench_mlkem768_encaps();
  openssl_bench_mlkem768_decaps();
  openssl_bench_mlkem768_roundtrip();

  /* ===================================================================
     X25519MLKEM768 Hybrid Key Exchange
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  X25519MLKEM768 Hybrid Key Exchange (TLS 1.3)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_x25519mlkem768_keypair();
  fio_bench_x25519mlkem768_encaps();
  fio_bench_x25519mlkem768_decaps();
  fio_bench_x25519mlkem768_roundtrip();
  fprintf(stderr, "  (no OpenSSL equivalent)\n");

  /* ===================================================================
     Password Hashing / KDF (Lyra2 & Argon2)
     =================================================================== */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Password Hashing / KDF (Lyra2 & Argon2)\n"
          "---------------------------------------------------\n");
  fprintf(stderr, "  [fio-stl]\n");
  fio_bench_kdf();
  fprintf(stderr, "  [OpenSSL]\n");
  openssl_bench_argon2id();

  /* ===================================================================
     Summary
     =================================================================== */
  fio_bench_kem_key_sizes();
  fio_bench_print_platform_info();

  return 0;
}
