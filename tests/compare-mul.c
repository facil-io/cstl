/*
Compare different multiplication implementations
*/
#include "test-helpers.h"
#include <time.h>

#define BENCH_ITERATIONS 100000

static void benchmark_compare_256bit(void) {
  uint64_t a[4], b[4], result[8];

  // Initialize with random data
  for (int i = 0; i < 4; i++) {
    a[i] = 0x123456789ABCDEF0ULL + i;
    b[i] = 0xFEDCBA9876543210ULL + i;
  }

  FIO_LOG_DDEBUG("256-bit Multiplication Comparison:");

  // Benchmark fast path directly
  clock_t start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio___math_mul_256(result, a, b);
    a[0] = result[0];
  }
  clock_t end = clock();
  double us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  double ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug output */
  FIO_LOG_DDEBUG("  Fast path (256): %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);

  // Benchmark long mul
  start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio___math_mul_long(result, a, b, 4);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug output */
  FIO_LOG_DDEBUG("  Long mul path:    %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);

  // Benchmark via dispatcher
  start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio_math_mul(result, a, b, 4);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug output */
  FIO_LOG_DDEBUG("  Via dispatcher:  %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);
}

static void benchmark_compare_512bit(void) {
  uint64_t a[8], b[8], result[16];

  for (int i = 0; i < 8; i++) {
    a[i] = 0x123456789ABCDEF0ULL + i;
    b[i] = 0xFEDCBA9876543210ULL + i;
  }

  FIO_LOG_DDEBUG("512-bit Multiplication Comparison:");

  // Fast path
  clock_t start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio___math_mul_512(result, a, b);
    a[0] = result[0];
  }
  clock_t end = clock();
  double us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  double ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug output */
  FIO_LOG_DDEBUG("  Fast path (512): %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);

  // Long mul
  start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio___math_mul_long(result, a, b, 8);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug output */
  FIO_LOG_DDEBUG("  Long mul path:   %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);

  // Via dispatcher
  start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio_math_mul(result, a, b, 8);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug output */
  FIO_LOG_DDEBUG("  Via dispatcher:  %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);
}

static void benchmark_compare_1024bit(void) {
  uint64_t a[16], b[16], result[32];

  for (int i = 0; i < 16; i++) {
    a[i] = 0x123456789ABCDEF0ULL + i;
    b[i] = 0xFEDCBA9876543210ULL + i;
  }

  FIO_LOG_DDEBUG("1024-bit Multiplication Comparison:");

  // Fast path
  clock_t start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio___math_mul_1024(result, a, b);
    a[0] = result[0];
  }
  clock_t end = clock();
  double us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  double ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug output */
  FIO_LOG_DDEBUG("  Fast path (1024): %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);

  // Long mul
  start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio___math_mul_long(result, a, b, 16);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug output */
  FIO_LOG_DDEBUG("  Long mul path:   %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);

  // Via dispatcher
  start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio_math_mul(result, a, b, 16);
    a[0] = result[0];
  }
  end = clock();
  us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug output */
  FIO_LOG_DDEBUG("  Via dispatcher:   %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);
}

int main(void) {
  FIO_LOG_DDEBUG("==================================");
  FIO_LOG_DDEBUG("Multiplication Implementation Comparison");
  FIO_LOG_DDEBUG("==================================");

  benchmark_compare_256bit();
  FIO_LOG_DDEBUG("");
  benchmark_compare_512bit();
  FIO_LOG_DDEBUG("");
  benchmark_compare_1024bit();

  FIO_LOG_DDEBUG("==================================");
  FIO_LOG_DDEBUG("All benchmarks complete!");
  return 0;
}
