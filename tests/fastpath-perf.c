/*
Test fast path performance
*/
#include "test-helpers.h"
#include <time.h>

#define BENCH_ITERATIONS 100000

static void benchmark_256bit(void) {
  uint64_t a[4], b[4], result[8];

  // Initialize with random data
  for (int i = 0; i < 4; i++) {
    a[i] = 0x123456789ABCDEF0ULL + i;
    b[i] = 0xFEDCBA9876543210ULL + i;
  }

  // Benchmark the fast path using dispatcher
  clock_t start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio_math_mul(result, a, b, 4);
    a[0] = result[0]; // prevent optimization
  }
  clock_t end = clock();

  double us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  double ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug logging */

  FIO_LOG_DDEBUG("256-bit (via dispatcher): %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);
}

static void benchmark_512bit(void) {
  uint64_t a[8], b[8], result[16];

  for (int i = 0; i < 8; i++) {
    a[i] = 0x123456789ABCDEF0ULL + i;
    b[i] = 0xFEDCBA9876543210ULL + i;
  }

  clock_t start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio_math_mul(result, a, b, 8);
    a[0] = result[0];
  }
  clock_t end = clock();

  double us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  double ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug logging */

  FIO_LOG_DDEBUG("512-bit (via dispatcher): %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);
}

static void benchmark_1024bit(void) {
  uint64_t a[16], b[16], result[32];

  for (int i = 0; i < 16; i++) {
    a[i] = 0x123456789ABCDEF0ULL + i;
    b[i] = 0xFEDCBA9876543210ULL + i;
  }

  clock_t start = clock();
  for (int iter = 0; iter < BENCH_ITERATIONS; iter++) {
    fio_math_mul(result, a, b, 16);
    a[0] = result[0];
  }
  clock_t end = clock();

  double us = (double)(end - start) * 1000000 / CLOCKS_PER_SEC;
  double ops_per_sec = (BENCH_ITERATIONS * 1000000.0) / us;
  (void)ops_per_sec; /* Used only in debug logging */

  FIO_LOG_DDEBUG("1024-bit (via dispatcher): %.2f M ops/sec (%d us)",
                 ops_per_sec / 1000000.0,
                 (int)us);
}

int main(void) {
  FIO_LOG_DDEBUG("==================================");
  FIO_LOG_DDEBUG("Fast Path Performance Test");
  FIO_LOG_DDEBUG("==================================");

  benchmark_256bit();
  benchmark_512bit();
  benchmark_1024bit();

  FIO_LOG_DDEBUG("==================================");
  FIO_LOG_DDEBUG("All benchmarks complete!");
  return 0;
}
