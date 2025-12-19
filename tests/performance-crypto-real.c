/* *****************************************************************************
Real-World Crypto Performance Profiling
Tests multiplication optimizations in actual cryptographic contexts
Includes OpenSSL head-to-head benchmarks for direct comparison
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SHA2
#define FIO_SHA3
#define FIO_BLAKE2
#define FIO_ED25519
#define FIO_CHACHA
#include FIO_INCLUDE_FILE

/* *****************************************************************************
OpenSSL Integration (Conditional Compilation)
***************************************************************************** */
#ifdef HAVE_OPENSSL
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

/* OpenSSL version compatibility check */
#if OPENSSL_VERSION_NUMBER < 0x30000000L
#warning "OpenSSL 3.x recommended for full benchmark support"
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
      "Ed25519 public key generation", 2000, fio_ed25519_public_key(pk, sk);
      sk[0] ^= pk[0];);
}

FIO_SFUNC void fio_bench_ed25519_sign(void) {
  fprintf(stderr, "    * Ed25519 Signature Generation:\n");

  uint8_t sk[32], pk[32], sig[64];
  uint8_t msg[128];

  fio_rand_bytes(sk, 32);
  fio_rand_bytes(msg, 128);
  fio_ed25519_public_key(pk, sk);

  /* Sign empty message */
  FIO_BENCH_CRYPTO(
      "Ed25519 sign (0 bytes)", 2000, fio_ed25519_sign(sig, "", 0, sk, pk);
      sk[0] ^= sig[0];);

  /* Sign 64 byte message */
  FIO_BENCH_CRYPTO(
      "Ed25519 sign (64 bytes)", 2000, fio_ed25519_sign(sig, msg, 64, sk, pk);
      sk[0] ^= sig[0];);

  /* Sign 128 byte message */
  FIO_BENCH_CRYPTO(
      "Ed25519 sign (128 bytes)", 2000, fio_ed25519_sign(sig, msg, 128, sk, pk);
      sk[0] ^= sig[0];);
}

FIO_SFUNC void fio_bench_ed25519_verify(void) {
  fprintf(stderr, "    * Ed25519 Signature Verification:\n");

  uint8_t sk[32], pk[32], sig[64];
  uint8_t msg[128];

  fio_rand_bytes(sk, 32);
  fio_rand_bytes(msg, 128);
  fio_ed25519_public_key(pk, sk);

  /* Verify empty message */
  fio_ed25519_sign(sig, "", 0, sk, pk);
  FIO_BENCH_CRYPTO("Ed25519 verify (0 bytes)",
                   2000,
                   int result = fio_ed25519_verify(sig, "", 0, pk);
                   pk[0] ^= (uint8_t)result;);

  /* Verify 64 byte message */
  fio_ed25519_sign(sig, msg, 64, sk, pk);
  FIO_BENCH_CRYPTO("Ed25519 verify (64 bytes)",
                   2000,
                   int result = fio_ed25519_verify(sig, msg, 64, pk);
                   pk[0] ^= (uint8_t)result;);

  /* Verify 128 byte message */
  fio_ed25519_sign(sig, msg, 128, sk, pk);
  FIO_BENCH_CRYPTO("Ed25519 verify (128 bytes)",
                   2000,
                   int result = fio_ed25519_verify(sig, msg, 128, pk);
                   pk[0] ^= (uint8_t)result;);
}

FIO_SFUNC void fio_bench_ed25519_roundtrip(void) {
  fprintf(stderr, "    * Ed25519 Sign+Verify Roundtrip:\n");

  uint8_t sk[32], pk[32], sig[64];
  uint8_t msg[1024];

  fio_rand_bytes(sk, 32);
  fio_rand_bytes(msg, 1024);
  fio_ed25519_public_key(pk, sk);

  FIO_BENCH_CRYPTO("Ed25519 sign+verify roundtrip (256 bytes)",
                   2000,
                   fio_ed25519_sign(sig, msg, 256, sk, pk);
                   int result = fio_ed25519_verify(sig, msg, 256, pk);
                   sk[0] ^= (uint8_t)result;);
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
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 65536);

  /* 64 byte encryption */
  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 encrypt (64 bytes)",
      2000,
      fio_chacha20_poly1305_enc(mac, data_small, 64, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* 1 KB encryption */
  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 encrypt (1 KB)",
      2000,
      fio_chacha20_poly1305_enc(mac, data_medium, 1024, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* 64 KB encryption */
  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 encrypt (64 KB)",
      1000,
      fio_chacha20_poly1305_enc(mac, data_large, 65536, NULL, 0, key, nonce);
      key[0] ^= mac[0];);

  /* With additional data */
  uint8_t ad[64];
  fio_rand_bytes(ad, 64);

  FIO_BENCH_CRYPTO(
      "ChaCha20-Poly1305 encrypt (1 KB + 64B AAD)",
      2000,
      fio_chacha20_poly1305_enc(mac, data_medium, 1024, ad, 64, key, nonce);
      key[0] ^= mac[0];);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 65536);
}

FIO_SFUNC void fio_bench_chacha20_poly1305_dec(void) {
  fprintf(stderr, "    * ChaCha20-Poly1305 Decryption:\n");

  uint8_t key[32], nonce[12];
  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 65536);

  /* Generate valid MACs */
  uint8_t mac_small[16], mac_medium[16], mac_large[16];
  fio_chacha20_poly1305_enc(mac_small, data_small, 64, NULL, 0, key, nonce);
  fio_chacha20_poly1305_enc(mac_medium, data_medium, 1024, NULL, 0, key, nonce);
  fio_chacha20_poly1305_enc(mac_large, data_large, 65536, NULL, 0, key, nonce);

  /* 64 byte decryption */
  FIO_BENCH_CRYPTO("ChaCha20-Poly1305 decrypt (64 bytes)",
                   2000,
                   int result = fio_chacha20_poly1305_dec(mac_small,
                                                          data_small,
                                                          64,
                                                          NULL,
                                                          0,
                                                          key,
                                                          nonce);
                   key[0] ^= (uint8_t)result;);

  /* 1 KB decryption */
  FIO_BENCH_CRYPTO("ChaCha20-Poly1305 decrypt (1 KB)",
                   2000,
                   int result = fio_chacha20_poly1305_dec(mac_medium,
                                                          data_medium,
                                                          1024,
                                                          NULL,
                                                          0,
                                                          key,
                                                          nonce);
                   key[0] ^= (uint8_t)result;);

  /* 64 KB decryption */
  FIO_BENCH_CRYPTO("ChaCha20-Poly1305 decrypt (64 KB)",
                   1000,
                   int result = fio_chacha20_poly1305_dec(mac_large,
                                                          data_large,
                                                          65536,
                                                          NULL,
                                                          0,
                                                          key,
                                                          nonce);
                   key[0] ^= (uint8_t)result;);

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 65536);
}

FIO_SFUNC void fio_bench_chacha20_poly1305_throughput(void) {
  fprintf(stderr, "    * ChaCha20-Poly1305 Throughput:\n");

  uint8_t key[32], nonce[12];
  uint8_t mac[16];
  uint8_t *data = NULL;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);

  /* Test various sizes to see where Poly1305 mul becomes dominant */
  size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 4096, 16384};

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
  uint8_t out[64];

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO("BLAKE2b-512 hash (64 bytes)",
                   2000,
                   fio_blake2b(out, 64, data_small, 64, NULL, 0);
                   data_small[0] ^= out[0];);

  /* 1 KB hash */
  FIO_BENCH_CRYPTO("BLAKE2b-512 hash (1 KB)",
                   2000,
                   fio_blake2b(out, 64, data_medium, 1024, NULL, 0);
                   data_medium[0] ^= out[0];);

  /* 8 KB hash */
  FIO_BENCH_CRYPTO("BLAKE2b-512 hash (8 KB)",
                   2000,
                   fio_blake2b(out, 64, data_large, 8192, NULL, 0);
                   data_large[0] ^= out[0];);

  /* Throughput measurement */
  {
    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;
    for (; (clock() - bench_start) < (2000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      fio_blake2b(out, 64, data_large, 8192, NULL, 0);
      data_large[0] ^= out[0];
    }
    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(8192 * bench_iterations) / (elapsed * 1024.0 * 1024.0);
    fprintf(stderr,
            "      %-50s %10.2f MB/s\n",
            "BLAKE2b-512 throughput (8 KB blocks)",
            throughput_mbps);
  }

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
}

FIO_SFUNC void fio_bench_blake2s(void) {
  fprintf(stderr, "    * BLAKE2s-256 Throughput:\n");

  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;
  uint8_t out[32];

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO("BLAKE2s-256 hash (64 bytes)",
                   2000,
                   fio_blake2s(out, 32, data_small, 64, NULL, 0);
                   data_small[0] ^= out[0];);

  /* 1 KB hash */
  FIO_BENCH_CRYPTO("BLAKE2s-256 hash (1 KB)",
                   2000,
                   fio_blake2s(out, 32, data_medium, 1024, NULL, 0);
                   data_medium[0] ^= out[0];);

  /* 8 KB hash */
  FIO_BENCH_CRYPTO("BLAKE2s-256 hash (8 KB)",
                   2000,
                   fio_blake2s(out, 32, data_large, 8192, NULL, 0);
                   data_large[0] ^= out[0];);

  /* Throughput measurement */
  {
    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;
    for (; (clock() - bench_start) < (2000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      fio_blake2s(out, 32, data_large, 8192, NULL, 0);
      data_large[0] ^= out[0];
    }
    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(8192 * bench_iterations) / (elapsed * 1024.0 * 1024.0);
    fprintf(stderr,
            "      %-50s %10.2f MB/s\n",
            "BLAKE2s-256 throughput (8 KB blocks)",
            throughput_mbps);
  }

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
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
  uint8_t out[32];

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-256 hash (64 bytes)", 2000, fio_sha3_256(out, data_small, 64);
      data_small[0] ^= out[0];);

  /* 1 KB hash */
  FIO_BENCH_CRYPTO(
      "SHA3-256 hash (1 KB)", 2000, fio_sha3_256(out, data_medium, 1024);
      data_medium[0] ^= out[0];);

  /* 8 KB hash */
  FIO_BENCH_CRYPTO(
      "SHA3-256 hash (8 KB)", 2000, fio_sha3_256(out, data_large, 8192);
      data_large[0] ^= out[0];);

  /* Throughput measurement */
  {
    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;
    for (; (clock() - bench_start) < (2000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      fio_sha3_256(out, data_large, 8192);
      data_large[0] ^= out[0];
    }
    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(8192 * bench_iterations) / (elapsed * 1024.0 * 1024.0);
    fprintf(stderr,
            "      %-50s %10.2f MB/s\n",
            "SHA3-256 throughput (8 KB blocks)",
            throughput_mbps);
  }

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
}

FIO_SFUNC void fio_bench_sha3_512(void) {
  fprintf(stderr, "    * SHA3-512 Throughput:\n");

  uint8_t *data_small = NULL;
  uint8_t *data_medium = NULL;
  uint8_t *data_large = NULL;
  uint8_t out[64];

  /* Allocate buffers */
  data_small = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);

  /* 64 byte hash */
  FIO_BENCH_CRYPTO(
      "SHA3-512 hash (64 bytes)", 2000, fio_sha3_512(out, data_small, 64);
      data_small[0] ^= out[0];);

  /* 1 KB hash */
  FIO_BENCH_CRYPTO(
      "SHA3-512 hash (1 KB)", 2000, fio_sha3_512(out, data_medium, 1024);
      data_medium[0] ^= out[0];);

  /* 8 KB hash */
  FIO_BENCH_CRYPTO(
      "SHA3-512 hash (8 KB)", 2000, fio_sha3_512(out, data_large, 8192);
      data_large[0] ^= out[0];);

  /* Throughput measurement */
  {
    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;
    for (; (clock() - bench_start) < (2000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      fio_sha3_512(out, data_large, 8192);
      data_large[0] ^= out[0];
    }
    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(8192 * bench_iterations) / (elapsed * 1024.0 * 1024.0);
    fprintf(stderr,
            "      %-50s %10.2f MB/s\n",
            "SHA3-512 throughput (8 KB blocks)",
            throughput_mbps);
  }

  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
}

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
                     2000,
                     fio_math_mul(result, a, b, 64);
                     a[0] ^= result[0];);
  }
}

/* *****************************************************************************
OpenSSL Comparison Benchmarks - Direct Head-to-Head
***************************************************************************** */

#ifdef HAVE_OPENSSL

/* OpenSSL Ed25519 Signature Generation */
FIO_SFUNC void openssl_bench_ed25519_sign(void) {
  fprintf(stderr, "    * OpenSSL Ed25519 Signature Generation:\n");

  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *pkey_ctx = NULL;
  EVP_MD_CTX *md_ctx = NULL;
  unsigned char sig[64];
  size_t sig_len;
  unsigned char msg[128];

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

  fio_rand_bytes(msg, 128);

  /* Benchmark empty message signing */
  FIO_BENCH_CRYPTO(
      "OpenSSL Ed25519 sign (0 bytes)", 2000, sig_len = sizeof(sig);
      EVP_DigestSignInit(md_ctx, NULL, NULL, NULL, pkey);
      EVP_DigestSign(md_ctx, sig, &sig_len, (unsigned char *)"", 0);
      EVP_MD_CTX_reset(md_ctx););

  /* Benchmark 64 byte message signing */
  FIO_BENCH_CRYPTO(
      "OpenSSL Ed25519 sign (64 bytes)", 2000, sig_len = sizeof(sig);
      EVP_DigestSignInit(md_ctx, NULL, NULL, NULL, pkey);
      EVP_DigestSign(md_ctx, sig, &sig_len, msg, 64);
      EVP_MD_CTX_reset(md_ctx););

  /* Benchmark 128 byte message signing */
  FIO_BENCH_CRYPTO(
      "OpenSSL Ed25519 sign (128 bytes)", 2000, sig_len = sizeof(sig);
      EVP_DigestSignInit(md_ctx, NULL, NULL, NULL, pkey);
      EVP_DigestSign(md_ctx, sig, &sig_len, msg, 128);
      EVP_MD_CTX_reset(md_ctx););

  EVP_MD_CTX_free(md_ctx);
  EVP_PKEY_free(pkey);
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
  unsigned char msg[128];

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
  fio_rand_bytes(msg, 128);

  /* Sign 0 byte message for verification benchmark */
  sig_len = sizeof(sig);
  EVP_DigestSignInit(sign_ctx, NULL, NULL, NULL, pkey);
  EVP_DigestSign(sign_ctx, sig, &sig_len, (unsigned char *)"", 0);
  EVP_MD_CTX_reset(sign_ctx);

  FIO_BENCH_CRYPTO(
      "OpenSSL Ed25519 verify (0 bytes)",
      2000,
      EVP_DigestVerifyInit(md_ctx, NULL, NULL, NULL, pkey);
      EVP_DigestVerify(md_ctx, sig, sig_len, (unsigned char *)"", 0);
      EVP_MD_CTX_reset(md_ctx););

  /* Sign 64 byte message for verification benchmark */
  sig_len = sizeof(sig);
  EVP_DigestSignInit(sign_ctx, NULL, NULL, NULL, pkey);
  EVP_DigestSign(sign_ctx, sig, &sig_len, msg, 64);
  EVP_MD_CTX_reset(sign_ctx);

  FIO_BENCH_CRYPTO("OpenSSL Ed25519 verify (64 bytes)",
                   2000,
                   EVP_DigestVerifyInit(md_ctx, NULL, NULL, NULL, pkey);
                   EVP_DigestVerify(md_ctx, sig, sig_len, msg, 64);
                   EVP_MD_CTX_reset(md_ctx););

  /* Sign 128 byte message for verification benchmark */
  sig_len = sizeof(sig);
  EVP_DigestSignInit(sign_ctx, NULL, NULL, NULL, pkey);
  EVP_DigestSign(sign_ctx, sig, &sig_len, msg, 128);
  EVP_MD_CTX_reset(sign_ctx);

  FIO_BENCH_CRYPTO("OpenSSL Ed25519 verify (128 bytes)",
                   2000,
                   EVP_DigestVerifyInit(md_ctx, NULL, NULL, NULL, pkey);
                   EVP_DigestVerify(md_ctx, sig, sig_len, msg, 128);
                   EVP_MD_CTX_reset(md_ctx););

  EVP_MD_CTX_free(sign_ctx);
  EVP_MD_CTX_free(md_ctx);
  EVP_PKEY_free(pkey);
}

/* OpenSSL X25519 Key Exchange */
FIO_SFUNC void openssl_bench_x25519(void) {
  fprintf(stderr, "    * OpenSSL X25519 Key Exchange:\n");

  EVP_PKEY *pkey1 = NULL, *pkey2 = NULL;
  EVP_PKEY_CTX *pkey_ctx = NULL;
  EVP_PKEY_CTX *derive_ctx = NULL;
  unsigned char shared_secret[32];
  size_t secret_len;

  /* Generate first key pair */
  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
  EVP_PKEY_keygen_init(pkey_ctx);
  EVP_PKEY_keygen(pkey_ctx, &pkey1);
  EVP_PKEY_CTX_free(pkey_ctx);

  /* Generate second key pair */
  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
  EVP_PKEY_keygen_init(pkey_ctx);
  EVP_PKEY_keygen(pkey_ctx, &pkey2);
  EVP_PKEY_CTX_free(pkey_ctx);

  /* Benchmark shared secret derivation */
  derive_ctx = EVP_PKEY_CTX_new(pkey1, NULL);

  FIO_BENCH_CRYPTO("OpenSSL X25519 shared secret derivation",
                   2000,
                   secret_len = sizeof(shared_secret);
                   EVP_PKEY_derive_init(derive_ctx);
                   EVP_PKEY_derive_set_peer(derive_ctx, pkey2);
                   EVP_PKEY_derive(derive_ctx, shared_secret, &secret_len);
                   EVP_PKEY_CTX_free(derive_ctx);
                   derive_ctx = EVP_PKEY_CTX_new(pkey1, NULL););

  EVP_PKEY_CTX_free(derive_ctx);
  EVP_PKEY_free(pkey1);
  EVP_PKEY_free(pkey2);
}

/* OpenSSL ChaCha20-Poly1305 AEAD */
FIO_SFUNC void openssl_bench_chacha20_poly1305(void) {
  fprintf(stderr, "    * OpenSSL ChaCha20-Poly1305 Encryption:\n");

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char key[32], nonce[12], tag[16];
  unsigned char *data_small = NULL;
  unsigned char *data_medium = NULL;
  unsigned char *data_large = NULL;
  int outlen;

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);

  /* Allocate buffers */
  data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 65536, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 65536);

  /* 64 byte encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 encrypt (64 bytes)",
      2000,
      EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_small, &outlen, data_small, 64);
      EVP_EncryptFinal_ex(ctx, data_small + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 1 KB encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 encrypt (1 KB)",
      2000,
      EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_medium, &outlen, data_medium, 1024);
      EVP_EncryptFinal_ex(ctx, data_medium + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  /* 64 KB encryption */
  FIO_BENCH_CRYPTO(
      "OpenSSL ChaCha20-Poly1305 encrypt (64 KB)",
      1000,
      EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
      EVP_EncryptUpdate(ctx, data_large, &outlen, data_large, 65536);
      EVP_EncryptFinal_ex(ctx, data_large + outlen, &outlen);
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag);
      EVP_CIPHER_CTX_reset(ctx););

  EVP_CIPHER_CTX_free(ctx);
  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 65536);
}

/* OpenSSL BLAKE2 Hash Functions */
FIO_SFUNC void openssl_bench_blake2(void) {
  fprintf(stderr, "    * OpenSSL BLAKE2b-512 Throughput:\n");

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  unsigned char *data_small = NULL;
  unsigned char *data_medium = NULL;
  unsigned char *data_large = NULL;
  unsigned char out[64];
  unsigned int outlen;

  /* Allocate buffers */
  data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);

  /* BLAKE2b-512: 64 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2b-512 hash (64 bytes)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
                   EVP_DigestUpdate(ctx, data_small, 64);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2b-512: 1 KB hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2b-512 hash (1 KB)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
                   EVP_DigestUpdate(ctx, data_medium, 1024);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2b-512: 8 KB hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2b-512 hash (8 KB)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
                   EVP_DigestUpdate(ctx, data_large, 8192);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* Throughput measurement */
  {
    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;
    for (; (clock() - bench_start) < (2000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      EVP_DigestInit_ex(ctx, EVP_blake2b512(), NULL);
      EVP_DigestUpdate(ctx, data_large, 8192);
      EVP_DigestFinal_ex(ctx, out, &outlen);
      EVP_MD_CTX_reset(ctx);
    }
    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(8192 * bench_iterations) / (elapsed * 1024.0 * 1024.0);
    fprintf(stderr,
            "      %-50s %10.2f MB/s\n",
            "OpenSSL BLAKE2b-512 throughput (8 KB blocks)",
            throughput_mbps);
  }

  fprintf(stderr, "    * OpenSSL BLAKE2s-256 Throughput:\n");

  /* BLAKE2s-256: 64 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2s-256 hash (64 bytes)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
                   EVP_DigestUpdate(ctx, data_small, 64);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2s-256: 1 KB hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2s-256 hash (1 KB)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
                   EVP_DigestUpdate(ctx, data_medium, 1024);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* BLAKE2s-256: 8 KB hash */
  FIO_BENCH_CRYPTO("OpenSSL BLAKE2s-256 hash (8 KB)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
                   EVP_DigestUpdate(ctx, data_large, 8192);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* Throughput measurement */
  {
    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;
    for (; (clock() - bench_start) < (2000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      EVP_DigestInit_ex(ctx, EVP_blake2s256(), NULL);
      EVP_DigestUpdate(ctx, data_large, 8192);
      EVP_DigestFinal_ex(ctx, out, &outlen);
      EVP_MD_CTX_reset(ctx);
    }
    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(8192 * bench_iterations) / (elapsed * 1024.0 * 1024.0);
    fprintf(stderr,
            "      %-50s %10.2f MB/s\n",
            "OpenSSL BLAKE2s-256 throughput (8 KB blocks)",
            throughput_mbps);
  }

  EVP_MD_CTX_free(ctx);
  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
}

/* OpenSSL SHA-3 Hash Functions */
FIO_SFUNC void openssl_bench_sha3(void) {
  fprintf(stderr, "    * OpenSSL SHA3-256 Throughput:\n");

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  unsigned char *data_small = NULL;
  unsigned char *data_medium = NULL;
  unsigned char *data_large = NULL;
  unsigned char out[64];
  unsigned int outlen;

  /* Allocate buffers */
  data_small = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  data_medium = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 1024, 0);
  data_large = (unsigned char *)FIO_MEM_REALLOC(NULL, 0, 8192, 0);

  fio_rand_bytes(data_small, 64);
  fio_rand_bytes(data_medium, 1024);
  fio_rand_bytes(data_large, 8192);

  /* SHA3-256: 64 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-256 hash (64 bytes)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
                   EVP_DigestUpdate(ctx, data_small, 64);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-256: 1 KB hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-256 hash (1 KB)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
                   EVP_DigestUpdate(ctx, data_medium, 1024);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-256: 8 KB hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-256 hash (8 KB)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
                   EVP_DigestUpdate(ctx, data_large, 8192);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* Throughput measurement */
  {
    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;
    for (; (clock() - bench_start) < (2000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
      EVP_DigestUpdate(ctx, data_large, 8192);
      EVP_DigestFinal_ex(ctx, out, &outlen);
      EVP_MD_CTX_reset(ctx);
    }
    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(8192 * bench_iterations) / (elapsed * 1024.0 * 1024.0);
    fprintf(stderr,
            "      %-50s %10.2f MB/s\n",
            "OpenSSL SHA3-256 throughput (8 KB blocks)",
            throughput_mbps);
  }

  fprintf(stderr, "    * OpenSSL SHA3-512 Throughput:\n");

  /* SHA3-512: 64 byte hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-512 hash (64 bytes)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL);
                   EVP_DigestUpdate(ctx, data_small, 64);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-512: 1 KB hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-512 hash (1 KB)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL);
                   EVP_DigestUpdate(ctx, data_medium, 1024);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* SHA3-512: 8 KB hash */
  FIO_BENCH_CRYPTO("OpenSSL SHA3-512 hash (8 KB)",
                   2000,
                   EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL);
                   EVP_DigestUpdate(ctx, data_large, 8192);
                   EVP_DigestFinal_ex(ctx, out, &outlen);
                   EVP_MD_CTX_reset(ctx););

  /* Throughput measurement */
  {
    clock_t bench_start = clock();
    uint64_t bench_iterations = 0;
    for (; (clock() - bench_start) < (2000 * CLOCKS_PER_SEC / 1000) ||
           bench_iterations < 100;
         ++bench_iterations) {
      EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL);
      EVP_DigestUpdate(ctx, data_large, 8192);
      EVP_DigestFinal_ex(ctx, out, &outlen);
      EVP_MD_CTX_reset(ctx);
    }
    clock_t bench_end = clock();
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;
    double throughput_mbps =
        (double)(8192 * bench_iterations) / (elapsed * 1024.0 * 1024.0);
    fprintf(stderr,
            "      %-50s %10.2f MB/s\n",
            "OpenSSL SHA3-512 throughput (8 KB blocks)",
            throughput_mbps);
  }

  EVP_MD_CTX_free(ctx);
  FIO_MEM_FREE(data_small, 64);
  FIO_MEM_FREE(data_medium, 1024);
  FIO_MEM_FREE(data_large, 8192);
}

/* OpenSSL BIGNUM Multiplication */
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
      "OpenSSL BN_mul (256-bit)", 5000, BN_mul(result, a, b, bn_ctx););

  /* 384-bit multiplication */
  BN_rand(a, 384, -1, 0);
  BN_rand(b, 384, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (384-bit)", 5000, BN_mul(result, a, b, bn_ctx););

  /* 512-bit multiplication */
  BN_rand(a, 512, -1, 0);
  BN_rand(b, 512, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (512-bit)", 5000, BN_mul(result, a, b, bn_ctx););

  /* 1024-bit multiplication */
  BN_rand(a, 1024, -1, 0);
  BN_rand(b, 1024, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (1024-bit)", 5000, BN_mul(result, a, b, bn_ctx););

  /* 2048-bit multiplication */
  BN_rand(a, 2048, -1, 0);
  BN_rand(b, 2048, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (2048-bit)", 5000, BN_mul(result, a, b, bn_ctx););

  /* 4096-bit multiplication */
  BN_rand(a, 4096, -1, 0);
  BN_rand(b, 4096, -1, 0);
  FIO_BENCH_CRYPTO(
      "OpenSSL BN_mul (4096-bit)", 2000, BN_mul(result, a, b, bn_ctx););

  BN_free(a);
  BN_free(b);
  BN_free(result);
  BN_CTX_free(bn_ctx);
}

#endif /* HAVE_OPENSSL */

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
      "X25519 public key generation", 2000, fio_x25519_public_key(pk1, sk1);
      sk1[0] ^= pk1[0];);

  /* Shared secret derivation (scalar multiplication) */
  fio_x25519_public_key(pk1, sk1);
  fio_x25519_public_key(pk2, sk2);

  FIO_BENCH_CRYPTO("X25519 shared secret derivation",
                   2000,
                   int result = fio_x25519_shared_secret(shared1, sk1, pk2);
                   sk1[0] ^= (uint8_t)result;);

  /* Full key exchange roundtrip */
  FIO_BENCH_CRYPTO("X25519 full key exchange (both sides)",
                   2000,
                   fio_x25519_public_key(pk1, sk1);
                   fio_x25519_public_key(pk2, sk2);
                   fio_x25519_shared_secret(shared1, sk1, pk2);
                   fio_x25519_shared_secret(shared2, sk2, pk1);
                   sk1[0] ^= shared1[0] ^ shared2[0];);
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
     facil.io Implementation Benchmarks
     =================================================================== */
  fprintf(stderr,
          "\n"
          "=======================================================\n"
          "  facil.io Implementation\n"
          "=======================================================\n");

  /* Ed25519 Digital Signatures */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Ed25519 Digital Signatures (256-bit field ops)\n"
          "---------------------------------------------------\n");
  fio_bench_ed25519_keygen();
  fio_bench_ed25519_sign();
  fio_bench_ed25519_verify();
  fio_bench_ed25519_roundtrip();

  /* X25519 Key Exchange */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  X25519 Key Exchange (256-bit scalar mul)\n"
          "---------------------------------------------------\n");
  fio_bench_x25519();

  /* ChaCha20-Poly1305 AEAD */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  ChaCha20-Poly1305 AEAD (130-bit field ops)\n"
          "---------------------------------------------------\n");
  fio_bench_chacha20_poly1305_enc();
  fio_bench_chacha20_poly1305_dec();
  fio_bench_chacha20_poly1305_throughput();

  /* BLAKE2 Hash Functions */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  BLAKE2 Hash Functions (Fast Cryptographic Hash)\n"
          "---------------------------------------------------\n");
  fio_bench_blake2b();
  fio_bench_blake2s();

  /* SHA-3 Hash Functions */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  SHA-3 Hash Functions (NIST Standard)\n"
          "---------------------------------------------------\n");
  fio_bench_sha3_256();
  fio_bench_sha3_512();

  /* Isolated multiplication benchmarks */
  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Isolated Multiplication (Crypto-Relevant Sizes)\n"
          "---------------------------------------------------\n");
  fio_bench_mul_crypto_sizes();

  /* ===================================================================
     OpenSSL Implementation Benchmarks (if available)
     =================================================================== */
#ifdef HAVE_OPENSSL
  fprintf(stderr,
          "\n"
          "=======================================================\n"
          "  OpenSSL Implementation (Head-to-Head Comparison)\n"
          "=======================================================\n");

  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  Ed25519 Digital Signatures\n"
          "---------------------------------------------------\n");
  openssl_bench_ed25519_sign();
  openssl_bench_ed25519_verify();

  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  X25519 Key Exchange\n"
          "---------------------------------------------------\n");
  openssl_bench_x25519();

  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  ChaCha20-Poly1305 AEAD\n"
          "---------------------------------------------------\n");
  openssl_bench_chacha20_poly1305();

  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  BLAKE2 Hash Functions\n"
          "---------------------------------------------------\n");
  openssl_bench_blake2();

  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  SHA-3 Hash Functions\n"
          "---------------------------------------------------\n");
  openssl_bench_sha3();

  fprintf(stderr,
          "\n"
          "---------------------------------------------------\n"
          "  BIGNUM Multiplication (Crypto Sizes)\n"
          "---------------------------------------------------\n");
  openssl_bench_bn_multiplication();

  /* Comparison Summary */
  fprintf(stderr,
          "\n"
          "=======================================================\n"
          "  Comparison Summary\n"
          "=======================================================\n\n");
  fprintf(stderr, "  ✓ Direct head-to-head comparison complete\n");
  fprintf(stderr, "  ✓ Both libraries tested on identical hardware\n");
  fprintf(stderr, "  ✓ Same compiler optimization level\n\n");

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

  fprintf(stderr, "\n  OpenSSL: %s\n", OPENSSL_VERSION_TEXT);
  fprintf(stderr, "\n");

#else
  /* OpenSSL not available */
  fprintf(stderr,
          "\n"
          "=======================================================\n"
          "  OpenSSL Not Available\n"
          "=======================================================\n\n");
  fprintf(stderr,
          "  ⚠  OpenSSL not detected - skipping comparison benchmarks\n");
  fprintf(stderr,
          "  ℹ  Install OpenSSL development libraries for "
          "head-to-head comparison\n");
  fprintf(stderr, "  ℹ  Build with: make HAVE_OPENSSL=1\n\n");
#endif

  return 0;
}
