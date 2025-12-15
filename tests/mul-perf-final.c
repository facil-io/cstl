/*
Accurate performance benchmark matching original methodology
*/
#define FIO_LOG
#define FIO_TIME
#define FIO_RAND
#include "test-helpers.h"

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

int main(void) {
  FIO_LOG_INFO("==================================");
  FIO_LOG_INFO("Schoolbook Multiplication Performance");
  FIO_LOG_INFO("Testing via fio_math_mul() dispatcher");
  FIO_LOG_INFO("==================================");
  FIO_LOG_INFO("");

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

  FIO_LOG_INFO("");
  FIO_LOG_INFO("==================================");
  FIO_LOG_INFO("Benchmark complete!");
  return 0;
}
