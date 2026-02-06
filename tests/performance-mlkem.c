/* *****************************************************************************
Performance Tests: ML-KEM Key Encapsulation Mechanism

These tests measure performance of ML-KEM-768 and X25519MLKEM768 operations
and are skipped in DEBUG mode. Run with: make tests/performance-mlkem

Includes OpenSSL head-to-head comparison when HAVE_OPENSSL is defined.
OpenSSL 3.5+ required for native ML-KEM support.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_TIME
#define FIO_RAND
#define FIO_MLKEM
#define FIO_SHA3
#define FIO_SHA2
#define FIO_ED25519
#include FIO_INCLUDE_FILE

/* OpenSSL Integration (Conditional Compilation) */
#ifdef HAVE_OPENSSL
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>

/* Check for OpenSSL 3.5+ which has native ML-KEM support */
#if OPENSSL_VERSION_NUMBER >= 0x30500000L
#define FIO___PERF_HAS_OPENSSL_MLKEM 1
#endif
#endif /* HAVE_OPENSSL */

/* Skip all performance tests in DEBUG mode */
#ifdef DEBUG
int main(void) {
  FIO_LOG_INFO("Performance tests skipped in DEBUG mode");
  return 0;
}
#else

/* *****************************************************************************
Benchmarking Macro (from performance-crypto-real.c pattern)
***************************************************************************** */

#define FIO_BENCH_MLKEM(name_str, target_time_ms, code_block)                  \
  do {                                                                         \
    clock_t bench_start = clock();                                             \
    uint64_t iters = 0;                                                        \
    for (;                                                                     \
         (clock() - bench_start) < ((target_time_ms)*CLOCKS_PER_SEC / 1000) || \
         iters < 100;                                                          \
         ++iters) {                                                            \
      code_block;                                                              \
      FIO_COMPILER_GUARD;                                                      \
    }                                                                          \
    clock_t bench_end = clock();                                               \
    double elapsed = (double)(bench_end - bench_start) / CLOCKS_PER_SEC;       \
    double ops_sec = iters / (elapsed > 0.0 ? elapsed : 0.0001);               \
    double us_op = (elapsed * 1000000.0) / iters;                              \
    fprintf(stderr,                                                            \
            "\t    %-45s %10.2f ops/sec  (%8.2f us/op)  [%llu iters]\n",       \
            name_str,                                                          \
            ops_sec,                                                           \
            us_op,                                                             \
            (unsigned long long)iters);                                        \
  } while (0)

/* *****************************************************************************
Performance Tests: ML-KEM-768 (facil.io implementation)
***************************************************************************** */

FIO_SFUNC void fio___perf_mlkem768_keypair(void) {
  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];

  FIO_BENCH_MLKEM("ML-KEM-768 keypair generation", 2000, {
    (void)fio_mlkem768_keypair(pk, sk);
    pk[0] ^= sk[0]; /* Prevent optimization */
  });
}

FIO_SFUNC void fio___perf_mlkem768_encaps(void) {
  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss[FIO_MLKEM768_SSBYTES];

  /* Generate a keypair for encapsulation */
  (void)fio_mlkem768_keypair(pk, sk);

  FIO_BENCH_MLKEM("ML-KEM-768 encapsulation", 2000, {
    (void)fio_mlkem768_encaps(ct, ss, pk);
    pk[0] ^= ss[0]; /* Prevent optimization */
  });
}

FIO_SFUNC void fio___perf_mlkem768_decaps(void) {
  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss_enc[FIO_MLKEM768_SSBYTES];
  uint8_t ss_dec[FIO_MLKEM768_SSBYTES];

  /* Generate keypair and encapsulate */
  (void)fio_mlkem768_keypair(pk, sk);
  (void)fio_mlkem768_encaps(ct, ss_enc, pk);

  FIO_BENCH_MLKEM("ML-KEM-768 decapsulation", 2000, {
    (void)fio_mlkem768_decaps(ss_dec, ct, sk);
    sk[0] ^= ss_dec[0]; /* Prevent optimization */
  });
}

FIO_SFUNC void fio___perf_mlkem768_roundtrip(void) {
  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss_enc[FIO_MLKEM768_SSBYTES];
  uint8_t ss_dec[FIO_MLKEM768_SSBYTES];

  FIO_BENCH_MLKEM("ML-KEM-768 full roundtrip (keygen+encaps+decaps)", 2000, {
    (void)fio_mlkem768_keypair(pk, sk);
    (void)fio_mlkem768_encaps(ct, ss_enc, pk);
    (void)fio_mlkem768_decaps(ss_dec, ct, sk);
    pk[0] ^= ss_dec[0]; /* Prevent optimization */
  });
}

/* *****************************************************************************
Performance Tests: X25519MLKEM768 Hybrid (facil.io implementation)
***************************************************************************** */

FIO_SFUNC void fio___perf_x25519mlkem768_keypair(void) {
  uint8_t pk[FIO_X25519MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_X25519MLKEM768_SECRETKEYBYTES];

  FIO_BENCH_MLKEM("X25519MLKEM768 keypair generation", 2000, {
    (void)fio_x25519mlkem768_keypair(pk, sk);
    pk[0] ^= sk[0]; /* Prevent optimization */
  });
}

FIO_SFUNC void fio___perf_x25519mlkem768_encaps(void) {
  uint8_t pk[FIO_X25519MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_X25519MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_X25519MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss[FIO_X25519MLKEM768_SSBYTES];

  /* Generate a keypair for encapsulation */
  (void)fio_x25519mlkem768_keypair(pk, sk);

  FIO_BENCH_MLKEM("X25519MLKEM768 encapsulation", 2000, {
    (void)fio_x25519mlkem768_encaps(ct, ss, pk);
    pk[0] ^= ss[0]; /* Prevent optimization */
  });
}

FIO_SFUNC void fio___perf_x25519mlkem768_decaps(void) {
  uint8_t pk[FIO_X25519MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_X25519MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_X25519MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss_enc[FIO_X25519MLKEM768_SSBYTES];
  uint8_t ss_dec[FIO_X25519MLKEM768_SSBYTES];

  /* Generate keypair and encapsulate */
  (void)fio_x25519mlkem768_keypair(pk, sk);
  (void)fio_x25519mlkem768_encaps(ct, ss_enc, pk);

  FIO_BENCH_MLKEM("X25519MLKEM768 decapsulation", 2000, {
    (void)fio_x25519mlkem768_decaps(ss_dec, ct, sk);
    sk[0] ^= ss_dec[0]; /* Prevent optimization */
  });
}

FIO_SFUNC void fio___perf_x25519mlkem768_roundtrip(void) {
  uint8_t pk[FIO_X25519MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_X25519MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_X25519MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss_enc[FIO_X25519MLKEM768_SSBYTES];
  uint8_t ss_dec[FIO_X25519MLKEM768_SSBYTES];

  FIO_BENCH_MLKEM("X25519MLKEM768 full roundtrip (keygen+encaps+decaps)",
                  2000,
                  {
                    (void)fio_x25519mlkem768_keypair(pk, sk);
                    (void)fio_x25519mlkem768_encaps(ct, ss_enc, pk);
                    (void)fio_x25519mlkem768_decaps(ss_dec, ct, sk);
                    pk[0] ^= ss_dec[0]; /* Prevent optimization */
                  });
}

/* *****************************************************************************
Performance Tests: X25519 Baseline (for comparison)
***************************************************************************** */

FIO_SFUNC void fio___perf_x25519_baseline(void) {
  uint8_t sk1[32], sk2[32], pk1[32], pk2[32], ss[32];

  fio_rand_bytes(sk1, 32);
  fio_rand_bytes(sk2, 32);

  FIO_BENCH_MLKEM("X25519 keypair generation (baseline)", 2000, {
    fio_x25519_public_key(pk1, sk1);
    sk1[0] ^= pk1[0];
  });

  fio_x25519_public_key(pk1, sk1);
  fio_x25519_public_key(pk2, sk2);

  FIO_BENCH_MLKEM("X25519 shared secret (baseline)", 2000, {
    fio_x25519_shared_secret(ss, sk1, pk2);
    sk1[0] ^= ss[0];
  });

  FIO_BENCH_MLKEM("X25519 full exchange (keygen+shared)", 2000, {
    fio_x25519_public_key(pk1, sk1);
    fio_x25519_shared_secret(ss, sk1, pk2);
    sk1[0] ^= ss[0];
  });
}

/* *****************************************************************************
OpenSSL Comparison Benchmarks (Conditional Compilation)
***************************************************************************** */

#ifdef FIO___PERF_HAS_OPENSSL_MLKEM

/*
 * OpenSSL 3.5+ ML-KEM-768 benchmarks using EVP_PKEY API
 *
 * API pattern:
 *   - Key generation: EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768")
 *   - Encapsulation: EVP_PKEY_encapsulate_init() + EVP_PKEY_encapsulate()
 *   - Decapsulation: EVP_PKEY_decapsulate_init() + EVP_PKEY_decapsulate()
 */

FIO_SFUNC void openssl_bench_mlkem768_keypair(void) {
  EVP_PKEY *pkey = NULL;

  FIO_BENCH_MLKEM("OpenSSL ML-KEM-768 keypair generation", 2000, {
    pkey = EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768");
    if (pkey) {
      EVP_PKEY_free(pkey);
      pkey = NULL;
    }
  });
}

FIO_SFUNC void openssl_bench_mlkem768_encaps(void) {
  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *ctx = NULL;
  unsigned char ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  unsigned char ss[FIO_MLKEM768_SSBYTES];
  size_t ct_len, ss_len;

  /* Generate a keypair for encapsulation */
  pkey = EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768");
  if (!pkey) {
    fprintf(stderr,
            "\t    OpenSSL ML-KEM-768 encapsulation: N/A (keygen failed)\n");
    return;
  }

  /* Create context once before benchmark loop */
  ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pkey, NULL);
  if (!ctx || EVP_PKEY_encapsulate_init(ctx, NULL) <= 0) {
    fprintf(stderr,
            "\t    OpenSSL ML-KEM-768 encapsulation: N/A (ctx init failed)\n");
    EVP_PKEY_free(pkey);
    if (ctx)
      EVP_PKEY_CTX_free(ctx);
    return;
  }

  FIO_BENCH_MLKEM("OpenSSL ML-KEM-768 encapsulation", 2000, {
    ct_len = sizeof(ct);
    ss_len = sizeof(ss);
    (void)EVP_PKEY_encapsulate(ctx, ct, &ct_len, ss, &ss_len);
  });

  EVP_PKEY_CTX_free(ctx);
  EVP_PKEY_free(pkey);
}

FIO_SFUNC void openssl_bench_mlkem768_decaps(void) {
  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *enc_ctx = NULL;
  EVP_PKEY_CTX *dec_ctx = NULL;
  unsigned char ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  unsigned char ss_enc[FIO_MLKEM768_SSBYTES];
  unsigned char ss_dec[FIO_MLKEM768_SSBYTES];
  size_t ct_len, ss_len;

  /* Generate keypair and encapsulate */
  pkey = EVP_PKEY_Q_keygen(NULL, NULL, "ML-KEM-768");
  if (!pkey) {
    fprintf(stderr,
            "\t    OpenSSL ML-KEM-768 decapsulation: N/A (keygen failed)\n");
    return;
  }

  enc_ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pkey, NULL);
  if (!enc_ctx || EVP_PKEY_encapsulate_init(enc_ctx, NULL) <= 0) {
    fprintf(
        stderr,
        "\t    OpenSSL ML-KEM-768 decapsulation: N/A (encaps init failed)\n");
    EVP_PKEY_free(pkey);
    if (enc_ctx)
      EVP_PKEY_CTX_free(enc_ctx);
    return;
  }

  ct_len = sizeof(ct);
  ss_len = sizeof(ss_enc);
  if (EVP_PKEY_encapsulate(enc_ctx, ct, &ct_len, ss_enc, &ss_len) <= 0) {
    fprintf(stderr,
            "\t    OpenSSL ML-KEM-768 decapsulation: N/A (encaps failed)\n");
    EVP_PKEY_CTX_free(enc_ctx);
    EVP_PKEY_free(pkey);
    return;
  }
  EVP_PKEY_CTX_free(enc_ctx);

  /* Create decapsulation context once before benchmark loop */
  dec_ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pkey, NULL);
  if (!dec_ctx || EVP_PKEY_decapsulate_init(dec_ctx, NULL) <= 0) {
    fprintf(
        stderr,
        "\t    OpenSSL ML-KEM-768 decapsulation: N/A (decaps init failed)\n");
    EVP_PKEY_free(pkey);
    if (dec_ctx)
      EVP_PKEY_CTX_free(dec_ctx);
    return;
  }

  FIO_BENCH_MLKEM("OpenSSL ML-KEM-768 decapsulation", 2000, {
    ss_len = sizeof(ss_dec);
    (void)EVP_PKEY_decapsulate(dec_ctx, ss_dec, &ss_len, ct, ct_len);
  });

  EVP_PKEY_CTX_free(dec_ctx);
  EVP_PKEY_free(pkey);
}

FIO_SFUNC void openssl_bench_mlkem768_roundtrip(void) {
  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *enc_ctx = NULL;
  EVP_PKEY_CTX *dec_ctx = NULL;
  unsigned char ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  unsigned char ss_enc[FIO_MLKEM768_SSBYTES];
  unsigned char ss_dec[FIO_MLKEM768_SSBYTES];
  size_t ct_len, ss_len;

  FIO_BENCH_MLKEM("OpenSSL ML-KEM-768 full roundtrip", 2000, {
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
    }
    pkey = NULL;
    enc_ctx = NULL;
    dec_ctx = NULL;
  });
}

/* OpenSSL X25519 baseline for comparison */
FIO_SFUNC void openssl_bench_x25519_baseline(void) {
  EVP_PKEY *pkey1 = NULL, *pkey2 = NULL;
  EVP_PKEY_CTX *pkey_ctx = NULL;
  EVP_PKEY_CTX *derive_ctx = NULL;
  unsigned char shared_secret[32];
  size_t secret_len;

  /* Generate first key pair */
  pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
  if (!pkey_ctx) {
    fprintf(stderr, "\t    OpenSSL X25519: N/A (context creation failed)\n");
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

  FIO_BENCH_MLKEM("OpenSSL X25519 keypair generation (baseline)", 2000, {
    EVP_PKEY *tmp = NULL;
    pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (pkey_ctx) {
      EVP_PKEY_keygen_init(pkey_ctx);
      EVP_PKEY_keygen(pkey_ctx, &tmp);
      EVP_PKEY_CTX_free(pkey_ctx);
      if (tmp)
        EVP_PKEY_free(tmp);
    }
  });

  FIO_BENCH_MLKEM("OpenSSL X25519 shared secret (baseline)", 2000, {
    derive_ctx = EVP_PKEY_CTX_new(pkey1, NULL);
    if (derive_ctx) {
      secret_len = sizeof(shared_secret);
      EVP_PKEY_derive_init(derive_ctx);
      EVP_PKEY_derive_set_peer(derive_ctx, pkey2);
      EVP_PKEY_derive(derive_ctx, shared_secret, &secret_len);
      EVP_PKEY_CTX_free(derive_ctx);
    }
  });

  EVP_PKEY_free(pkey1);
  EVP_PKEY_free(pkey2);
}

/* Run all OpenSSL ML-KEM benchmarks */
FIO_SFUNC void fio___perf_openssl_mlkem(void) {
  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tOpenSSL ML-KEM Comparison (OpenSSL 3.5+)\n");
  fprintf(stderr, "\t===========================================\n\n");

  fprintf(stderr, "\t* OpenSSL ML-KEM-768:\n");
  openssl_bench_mlkem768_keypair();
  openssl_bench_mlkem768_encaps();
  openssl_bench_mlkem768_decaps();
  openssl_bench_mlkem768_roundtrip();

  fprintf(stderr, "\n\t* OpenSSL X25519 (baseline comparison):\n");
  openssl_bench_x25519_baseline();

  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tOpenSSL ML-KEM benchmarks complete\n");
  fprintf(stderr, "\t===========================================\n");
}

#endif /* FIO___PERF_HAS_OPENSSL_MLKEM */

/* *****************************************************************************
Summary Statistics
***************************************************************************** */

FIO_SFUNC void fio___perf_mlkem_summary(void) {
  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tKey Size Summary\n");
  fprintf(stderr, "\t===========================================\n\n");

  fprintf(stderr, "\t  Algorithm              PK       SK       CT       SS\n");
  fprintf(stderr, "\t  ---------------------------------------------------\n");
  fprintf(stderr, "\t  X25519                 32       32       32       32\n");
  fprintf(stderr,
          "\t  ML-KEM-768           %4d     %4d     %4d       %2d\n",
          FIO_MLKEM768_PUBLICKEYBYTES,
          FIO_MLKEM768_SECRETKEYBYTES,
          FIO_MLKEM768_CIPHERTEXTBYTES,
          FIO_MLKEM768_SSBYTES);
  fprintf(stderr,
          "\t  X25519MLKEM768       %4d     %4d     %4d       %2d\n",
          FIO_X25519MLKEM768_PUBLICKEYBYTES,
          FIO_X25519MLKEM768_SECRETKEYBYTES,
          FIO_X25519MLKEM768_CIPHERTEXTBYTES,
          FIO_X25519MLKEM768_SSBYTES);
  fprintf(stderr, "\n");

  fprintf(stderr, "\t  Notes:\n");
  fprintf(stderr, "\t  - PK = Public Key, SK = Secret Key\n");
  fprintf(stderr, "\t  - CT = Ciphertext, SS = Shared Secret\n");
  fprintf(stderr, "\t  - X25519MLKEM768 is the TLS 1.3 hybrid (0x11ec)\n");
  fprintf(stderr,
          "\t  - ML-KEM-768 provides NIST Level 3 (192-bit) security\n");
  fprintf(stderr, "\n");
}

/* *****************************************************************************
Main Entry Point
***************************************************************************** */

int main(void) {
  fprintf(stderr, "\t===========================================\n");
  fprintf(stderr, "\tPerformance Tests: ML-KEM Key Encapsulation\n");
  fprintf(stderr, "\t===========================================\n\n");

  /* Print platform info */
  fprintf(stderr, "\t  Platform: ");
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
  fprintf(stderr, "\n");

  fprintf(stderr, "\t  Compiler: ");
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
  fprintf(stderr, "\n\n");

  /* facil.io Implementation */
  fprintf(stderr, "\tfacil.io Implementation:\n");
  fprintf(stderr, "\t===========================================\n\n");

  /* X25519 baseline */
  fprintf(stderr, "\t* X25519 (classical baseline):\n");
  fio___perf_x25519_baseline();

  /* ML-KEM-768 */
  fprintf(stderr, "\n\t* ML-KEM-768 (post-quantum):\n");
  fio___perf_mlkem768_keypair();
  fio___perf_mlkem768_encaps();
  fio___perf_mlkem768_decaps();
  fio___perf_mlkem768_roundtrip();

  /* X25519MLKEM768 Hybrid */
  fprintf(stderr,
          "\n\t* X25519MLKEM768 (hybrid, TLS 1.3 NamedGroup 0x11ec):\n");
  fio___perf_x25519mlkem768_keypair();
  fio___perf_x25519mlkem768_encaps();
  fio___perf_x25519mlkem768_decaps();
  fio___perf_x25519mlkem768_roundtrip();

  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tfacil.io ML-KEM benchmarks complete.\n");
  fprintf(stderr, "\t===========================================\n");

  /* OpenSSL Comparison (if available) */
#ifdef HAVE_OPENSSL
#ifdef FIO___PERF_HAS_OPENSSL_MLKEM
  fio___perf_openssl_mlkem();
#else
  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tOpenSSL ML-KEM Not Available\n");
  fprintf(stderr, "\t===========================================\n\n");
  fprintf(stderr, "\t  OpenSSL version: %s\n", OPENSSL_VERSION_TEXT);
  fprintf(stderr, "\t  ML-KEM requires OpenSSL 3.5+\n");
  fprintf(stderr, "\t  Skipping OpenSSL ML-KEM comparison.\n\n");
#endif
#else
  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tOpenSSL Not Available\n");
  fprintf(stderr, "\t===========================================\n\n");
  fprintf(stderr,
          "\t  OpenSSL not detected - skipping comparison benchmarks\n");
  fprintf(stderr,
          "\t  Build with: make tests/performance-mlkem TEST4CRYPTO=1\n\n");
#endif

  /* Summary */
  fio___perf_mlkem_summary();

  return 0;
}

#endif /* DEBUG */
