/* *****************************************************************************
Performance Tests: Multi-Precision Multiplication

Benchmarks fio_math_mul() dispatcher and compares fast path vs long mul.
Run with: make tests/performance-mul
***************************************************************************** */
#define FIO_LOG
#define FIO_TIME
#define FIO_RAND
#include "test-helpers.h"

#include <time.h>

/* Skip all performance tests in DEBUG mode */
#ifdef DEBUG
int main(void) {
  FIO_LOG_INFO("Performance tests skipped in DEBUG mode");
  return 0;
}
#else

/* *****************************************************************************
Helper Macros
***************************************************************************** */

/** Benchmark macro - runs code and reports throughput in M ops/sec */
#define FIO_PERF_BENCHMARK(name, iterations, setup, code, teardown)            \
  do {                                                                         \
    setup;                                                                     \
    uint64_t start_ = fio_time_micro();                                        \
    for (size_t i_ = 0; i_ < (iterations); ++i_) {                             \
      code;                                                                    \
      FIO_COMPILER_GUARD;                                                      \
    }                                                                          \
    uint64_t end_ = fio_time_micro();                                          \
    uint64_t elapsed_ = end_ - start_;                                         \
    if (!elapsed_)                                                             \
      elapsed_ = 1;                                                            \
    double ops_per_sec_ = (double)(iterations) / elapsed_;                     \
    fprintf(stderr,                                                            \
            "\t\t%-40s: %8.2f M ops/sec (%llu us)\n",                          \
            (name),                                                            \
            ops_per_sec_,                                                      \
            (unsigned long long)elapsed_);                                     \
    teardown;                                                                  \
  } while (0)

#define FIO_PERF_ITERATIONS 10000
#define FIO_CMP_ITERATIONS  100000

/* *****************************************************************************
Section 1: Dispatcher Throughput (fio_math_mul)
***************************************************************************** */

static void benchmark_dispatcher(void) {
  fprintf(stderr, "\n\t--- fio_math_mul() Dispatcher Throughput ---\n");

  /* 256-bit (4 words) */
  {
    uint64_t a[4], b[4], result[8];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    FIO_PERF_BENCHMARK("256-bit fio_math_mul",
                       FIO_PERF_ITERATIONS,
                       (void)0,
                       (fio_math_mul(result, a, b, 4), a[0] = result[0]),
                       (void)0);
  }

  /* 512-bit (8 words) */
  {
    uint64_t a[8], b[8], result[16];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    FIO_PERF_BENCHMARK("512-bit fio_math_mul",
                       FIO_PERF_ITERATIONS,
                       (void)0,
                       (fio_math_mul(result, a, b, 8), a[0] = result[0]),
                       (void)0);
  }

  /* 1024-bit (16 words) */
  {
    uint64_t a[16], b[16], result[32];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    FIO_PERF_BENCHMARK("1024-bit fio_math_mul",
                       FIO_PERF_ITERATIONS,
                       (void)0,
                       (fio_math_mul(result, a, b, 16), a[0] = result[0]),
                       (void)0);
  }

  /* 2048-bit (32 words) - should use Karatsuba */
  {
    uint64_t a[32], b[32], result[64];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    FIO_PERF_BENCHMARK("2048-bit fio_math_mul",
                       FIO_PERF_ITERATIONS / 2,
                       (void)0,
                       (fio_math_mul(result, a, b, 32), a[0] = result[0]),
                       (void)0);
  }
}

/* *****************************************************************************
Section 2: Implementation Comparison (fast path vs long mul vs dispatcher)
***************************************************************************** */

static void benchmark_compare_256bit(void) {
  uint64_t a[4], b[4], result[8];

  for (int i = 0; i < 4; i++) {
    a[i] = 0x123456789ABCDEF0ULL + (uint64_t)i;
    b[i] = 0xFEDCBA9876543210ULL + (uint64_t)i;
  }

  fprintf(stderr, "\n\t--- 256-bit Implementation Comparison ---\n");

  clock_t start = clock();
  for (size_t iter = 0; iter < FIO_CMP_ITERATIONS; iter++) {
    fio___math_mul_256(result, a, b);
    a[0] = result[0];
  }
  clock_t end = clock();
  double us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  double ops_per_sec = (FIO_CMP_ITERATIONS * 1000000.0) / us;
  fprintf(stderr,
          "\t\t%-40s: %8.2f M ops/sec\n",
          "Fast path (fio___math_mul_256)",
          ops_per_sec / 1000000.0);

  start = clock();
  for (size_t iter = 0; iter < FIO_CMP_ITERATIONS; iter++) {
    fio___math_mul_long(result, a, b, 4);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (FIO_CMP_ITERATIONS * 1000000.0) / us;
  fprintf(stderr,
          "\t\t%-40s: %8.2f M ops/sec\n",
          "Long mul (fio___math_mul_long)",
          ops_per_sec / 1000000.0);

  start = clock();
  for (size_t iter = 0; iter < FIO_CMP_ITERATIONS; iter++) {
    fio_math_mul(result, a, b, 4);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (FIO_CMP_ITERATIONS * 1000000.0) / us;
  fprintf(stderr,
          "\t\t%-40s: %8.2f M ops/sec\n",
          "Dispatcher (fio_math_mul)",
          ops_per_sec / 1000000.0);
}

static void benchmark_compare_512bit(void) {
  uint64_t a[8], b[8], result[16];

  for (int i = 0; i < 8; i++) {
    a[i] = 0x123456789ABCDEF0ULL + (uint64_t)i;
    b[i] = 0xFEDCBA9876543210ULL + (uint64_t)i;
  }

  fprintf(stderr, "\n\t--- 512-bit Implementation Comparison ---\n");

  clock_t start = clock();
  for (size_t iter = 0; iter < FIO_CMP_ITERATIONS; iter++) {
    fio___math_mul_512(result, a, b);
    a[0] = result[0];
  }
  clock_t end = clock();
  double us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  double ops_per_sec = (FIO_CMP_ITERATIONS * 1000000.0) / us;
  fprintf(stderr,
          "\t\t%-40s: %8.2f M ops/sec\n",
          "Fast path (fio___math_mul_512)",
          ops_per_sec / 1000000.0);

  start = clock();
  for (size_t iter = 0; iter < FIO_CMP_ITERATIONS; iter++) {
    fio___math_mul_long(result, a, b, 8);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (FIO_CMP_ITERATIONS * 1000000.0) / us;
  fprintf(stderr,
          "\t\t%-40s: %8.2f M ops/sec\n",
          "Long mul (fio___math_mul_long)",
          ops_per_sec / 1000000.0);

  start = clock();
  for (size_t iter = 0; iter < FIO_CMP_ITERATIONS; iter++) {
    fio_math_mul(result, a, b, 8);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (FIO_CMP_ITERATIONS * 1000000.0) / us;
  fprintf(stderr,
          "\t\t%-40s: %8.2f M ops/sec\n",
          "Dispatcher (fio_math_mul)",
          ops_per_sec / 1000000.0);
}

static void benchmark_compare_1024bit(void) {
  uint64_t a[16], b[16], result[32];

  for (int i = 0; i < 16; i++) {
    a[i] = 0x123456789ABCDEF0ULL + (uint64_t)i;
    b[i] = 0xFEDCBA9876543210ULL + (uint64_t)i;
  }

  fprintf(stderr, "\n\t--- 1024-bit Implementation Comparison ---\n");

  clock_t start = clock();
  for (size_t iter = 0; iter < FIO_CMP_ITERATIONS; iter++) {
    fio___math_mul_1024(result, a, b);
    a[0] = result[0];
  }
  clock_t end = clock();
  double us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  double ops_per_sec = (FIO_CMP_ITERATIONS * 1000000.0) / us;
  fprintf(stderr,
          "\t\t%-40s: %8.2f M ops/sec\n",
          "Fast path (fio___math_mul_1024)",
          ops_per_sec / 1000000.0);

  start = clock();
  for (size_t iter = 0; iter < FIO_CMP_ITERATIONS; iter++) {
    fio___math_mul_long(result, a, b, 16);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (FIO_CMP_ITERATIONS * 1000000.0) / us;
  fprintf(stderr,
          "\t\t%-40s: %8.2f M ops/sec\n",
          "Long mul (fio___math_mul_long)",
          ops_per_sec / 1000000.0);

  start = clock();
  for (size_t iter = 0; iter < FIO_CMP_ITERATIONS; iter++) {
    fio_math_mul(result, a, b, 16);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (FIO_CMP_ITERATIONS * 1000000.0) / us;
  fprintf(stderr,
          "\t\t%-40s: %8.2f M ops/sec\n",
          "Dispatcher (fio_math_mul)",
          ops_per_sec / 1000000.0);
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr, "\n\t=== Multi-Precision Multiplication Benchmarks ===\n");

  benchmark_dispatcher();
  benchmark_compare_256bit();
  benchmark_compare_512bit();
  benchmark_compare_1024bit();

  fprintf(stderr, "\n\t=== Benchmark complete! ===\n");
  return 0;
}

#endif /* !DEBUG */
