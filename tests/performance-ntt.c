/* *****************************************************************************
Performance Tests: NTT (Number Theoretic Transform) for ML-KEM

Compares scalar vs SIMD implementations of NTT/InvNTT.
Run with: make tests/performance-ntt
***************************************************************************** */
#include "test-helpers.h"

#define FIO_TIME
#define FIO_RAND
#define FIO_MLKEM
#define FIO_SHA3
#define FIO_SHA2
#define FIO_ED25519
#include FIO_INCLUDE_FILE

/* Skip all performance tests in DEBUG mode */
#ifdef DEBUG
int main(void) {
  FIO_LOG_INFO("Performance tests skipped in DEBUG mode");
  return 0;
}
#else

/* *****************************************************************************
Benchmarking Macro
***************************************************************************** */

#define FIO_BENCH_NTT(name_str, target_time_ms, code_block)                    \
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
            "\t    %-45s %10.0f ops/sec  (%8.3f us/op)\n",                     \
            name_str,                                                          \
            ops_sec,                                                           \
            us_op);                                                            \
  } while (0)

/* *****************************************************************************
Direct NTT Benchmarks (accessing internal functions)
***************************************************************************** */

/* We need to access the internal NTT functions directly.
 * Since they're static, we'll benchmark through the full ML-KEM operations
 * and also create isolated NTT tests by including the implementation. */

/* For isolated NTT testing, we'll use the polyvec_ntt which calls NTT 3 times
 */

FIO_SFUNC void bench_ntt_via_mlkem(void) {
  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  uint8_t ss_enc[FIO_MLKEM768_SSBYTES];
  uint8_t ss_dec[FIO_MLKEM768_SSBYTES];

  /* Pre-generate a keypair */
  fio_mlkem768_keypair(pk, sk);

  fprintf(stderr, "\n\t* ML-KEM-768 Operations (NTT-heavy):\n");

  /* Keypair generation: 6 NTTs (2 polyvec_ntt calls) */
  FIO_BENCH_NTT("Keypair generation (6 NTTs)", 2000, {
    fio_mlkem768_keypair(pk, sk);
    pk[0] ^= sk[0];
  });

  /* Encapsulation: 6 NTTs + 3 InvNTTs */
  FIO_BENCH_NTT("Encapsulation (6 NTTs + 3 InvNTTs)", 2000, {
    fio_mlkem768_encaps(ct, ss_enc, pk);
    pk[0] ^= ss_enc[0];
  });

  /* Decapsulation: 3 NTTs + 1 InvNTT + re-encryption (6 NTTs + 3 InvNTTs) */
  fio_mlkem768_encaps(ct, ss_enc, pk);
  FIO_BENCH_NTT("Decapsulation (9 NTTs + 4 InvNTTs)", 2000, {
    fio_mlkem768_decaps(ss_dec, ct, sk);
    sk[0] ^= ss_dec[0];
  });
}

/* *****************************************************************************
Comparison Summary
***************************************************************************** */

FIO_SFUNC void print_simd_info(void) {
  fprintf(stderr, "\n\t* SIMD Implementation Status:\n");

#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  fprintf(stderr, "\t    NTT:    AVX2 (16-wide, layers 0-3 vectorized)\n");
  fprintf(stderr, "\t    InvNTT: AVX2 (16-wide, layers 3-6 vectorized)\n");
#elif FIO___HAS_ARM_INTRIN
  fprintf(stderr, "\t    NTT:    NEON (8-wide, layers 0-4 vectorized)\n");
  fprintf(stderr, "\t    InvNTT: NEON (8-wide, layers 2-6 vectorized)\n");
#else
  fprintf(stderr, "\t    NTT:    Scalar (no SIMD available)\n");
  fprintf(stderr, "\t    InvNTT: Scalar (no SIMD available)\n");
#endif

  fprintf(stderr, "\n\t* NTT Structure (256 coefficients, q=3329):\n");
  fprintf(stderr, "\t    Layer 0: len=128, 1 group,  128 butterflies\n");
  fprintf(stderr, "\t    Layer 1: len=64,  2 groups, 64 butterflies each\n");
  fprintf(stderr, "\t    Layer 2: len=32,  4 groups, 32 butterflies each\n");
  fprintf(stderr, "\t    Layer 3: len=16,  8 groups, 16 butterflies each\n");
  fprintf(stderr, "\t    Layer 4: len=8,  16 groups, 8 butterflies each\n");
  fprintf(stderr, "\t    Layer 5: len=4,  32 groups, 4 butterflies each\n");
  fprintf(stderr, "\t    Layer 6: len=2,  64 groups, 2 butterflies each\n");
  fprintf(stderr, "\n");
}

/* *****************************************************************************
Main Entry Point
***************************************************************************** */

int main(void) {
  fprintf(stderr, "\t===========================================\n");
  fprintf(stderr, "\tPerformance Tests: NTT for ML-KEM\n");
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
  fprintf(stderr, "\n");

  print_simd_info();

  /* Run benchmarks */
  bench_ntt_via_mlkem();

  fprintf(stderr, "\n\t===========================================\n");
  fprintf(stderr, "\tNTT Performance Summary\n");
  fprintf(stderr, "\t===========================================\n\n");

  fprintf(stderr, "\t  Key Optimizations Applied:\n");
  fprintf(stderr, "\t  - Vectorized Montgomery reduction (mod q=3329)\n");
  fprintf(stderr, "\t  - Vectorized Barrett reduction\n");
  fprintf(stderr, "\t  - Vectorized butterfly operations\n");
  fprintf(stderr, "\t  - SIMD for large-stride layers, scalar for small\n");
  fprintf(stderr, "\n");

  fprintf(stderr, "\t  Reusable SIMD Primitives:\n");
  fprintf(stderr, "\t  - fio___mlkem_neon_montgomery_reduce()\n");
  fprintf(stderr, "\t  - fio___mlkem_neon_fqmul()\n");
  fprintf(stderr, "\t  - fio___mlkem_neon_barrett_reduce()\n");
  fprintf(stderr, "\t  - fio___mlkem_avx2_montgomery_reduce()\n");
  fprintf(stderr, "\t  - fio___mlkem_avx2_fqmul()\n");
  fprintf(stderr, "\t  - fio___mlkem_avx2_barrett_reduce()\n");
  fprintf(stderr, "\n");

  return 0;
}

#endif /* DEBUG */
