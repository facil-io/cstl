/* *****************************************************************************
Performance Tests: Memory Operations (memalt)

These tests measure performance of memory operations and are skipped in DEBUG
mode. Run with: make tests/performance-memalt
***************************************************************************** */

#include "test-helpers.h"

#undef FIO_MEMALT
#define FIO_MEMALT
#include FIO_INCLUDE_FILE

/* Skip all performance tests in DEBUG mode */
#ifdef DEBUG
int main(void) {
  FIO_LOG_INFO("Performance tests skipped in DEBUG mode");
  return 0;
}
#else

#include <string.h> /* for system memcpy, memset, memchr, memcmp, strlen */

/* *****************************************************************************
Helper Macros
***************************************************************************** */

/** Benchmark macro - runs code and reports throughput in MB/sec */
#define FIO_PERF_BENCHMARK_BYTES(name,                                         \
                                 iterations,                                   \
                                 bytes_per_op,                                 \
                                 setup,                                        \
                                 code,                                         \
                                 teardown)                                     \
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
    double mb_per_sec_ =                                                       \
        ((double)(iterations) * (bytes_per_op)) / (elapsed_ * 1.0);            \
    fprintf(stderr,                                                            \
            "\t\t%-40s: %8.2f MB/sec (%llu us)\n",                             \
            name,                                                              \
            mb_per_sec_,                                                       \
            (unsigned long long)elapsed_);                                     \
    teardown;                                                                  \
  } while (0)

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
            name,                                                              \
            ops_per_sec_,                                                      \
            (unsigned long long)elapsed_);                                     \
    teardown;                                                                  \
  } while (0)

/* *****************************************************************************
Performance Tests: fio_memcpy primitives (fixed sizes)
***************************************************************************** */

FIO_SFUNC void fio___perf_memcpy_primitives(void) {
  fprintf(stderr, "\t* Benchmarking fio_memcpy primitives (fixed sizes)...\n");

  const size_t base_iterations = 8192 << 4;
  char buf[4096 * 2];
  memset(buf, 0x80, sizeof(buf));

  /* Test fixed-size memcpy primitives */
  struct {
    void *(*fn)(void *, const void *);
    size_t bytes;
    const char *name;
  } tests[] = {
      {fio_memcpy8, 8, "fio_memcpy8"},
      {fio_memcpy16, 16, "fio_memcpy16"},
      {fio_memcpy32, 32, "fio_memcpy32"},
      {fio_memcpy64, 64, "fio_memcpy64"},
      {fio_memcpy128, 128, "fio_memcpy128"},
      {fio_memcpy256, 256, "fio_memcpy256"},
      {fio_memcpy512, 512, "fio_memcpy512"},
      {fio_memcpy1024, 1024, "fio_memcpy1024"},
      {fio_memcpy2048, 2048, "fio_memcpy2048"},
      {fio_memcpy4096, 4096, "fio_memcpy4096"},
      {NULL, 0, NULL},
  };

  for (size_t i = 0; tests[i].fn; ++i) {
    char name_fio[64];
    char name_sys[64];
    snprintf(name_fio, sizeof(name_fio), "%s", tests[i].name);
    snprintf(name_sys, sizeof(name_sys), "memcpy (%zu bytes)", tests[i].bytes);

    FIO_PERF_BENCHMARK_BYTES(name_fio,
                             base_iterations,
                             tests[i].bytes,
                             (void)0,
                             tests[i].fn(buf, buf + 4096),
                             (void)0);

    FIO_PERF_BENCHMARK_BYTES(name_sys,
                             base_iterations,
                             tests[i].bytes,
                             (void)0,
                             memcpy(buf, buf + 4096, tests[i].bytes),
                             (void)0);
    fprintf(stderr, "\n");
  }
}

/* *****************************************************************************
Performance Tests: fio_memcpy variable sizes
***************************************************************************** */

FIO_SFUNC void fio___perf_memcpy_variable(void) {
  fprintf(stderr, "\t* Benchmarking fio_memcpy (variable sizes)...\n");

  const size_t base_iterations = 8192;

  size_t test_sizes[] = {31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t s = 0; s < num_sizes; ++s) {
    size_t mem_len = test_sizes[s];
    size_t iterations = base_iterations
                        << (mem_len < 1024 ? (10 - (mem_len >> 7)) : 0);
    void *mem = malloc(mem_len << 1);
    if (!mem)
      continue;

    uint64_t sig = (uintptr_t)mem;
    sig ^= sig >> 13;
    sig ^= sig << 17;
    fio_memset(mem, sig, mem_len);

    char name_fio[64];
    char name_sys[64];
    snprintf(name_fio, sizeof(name_fio), "fio_memcpy (%zu bytes)", mem_len);
    snprintf(name_sys, sizeof(name_sys), "memcpy (%zu bytes)", mem_len);

    FIO_PERF_BENCHMARK_BYTES(name_fio,
                             iterations,
                             mem_len,
                             (void)0,
                             fio_memcpy((char *)mem + mem_len, mem, mem_len),
                             (void)0);

    FIO_PERF_BENCHMARK_BYTES(name_sys,
                             iterations,
                             mem_len,
                             (void)0,
                             memcpy((char *)mem + mem_len, mem, mem_len),
                             (void)0);

    fprintf(stderr, "\n");
    free(mem);
  }
}

/* *****************************************************************************
Performance Tests: fio_memset
***************************************************************************** */

FIO_SFUNC void fio___perf_memset(void) {
  fprintf(stderr, "\t* Benchmarking fio_memset...\n");

  const size_t base_iterations = 8192;

  size_t test_sizes[] = {32, 64, 128, 256, 512, 1024, 4096, 16384, 65536};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t s = 0; s < num_sizes; ++s) {
    size_t mem_len = test_sizes[s];
    size_t iterations = base_iterations
                        << (mem_len < 4096 ? (12 - (mem_len >> 9)) : 0);
    void *mem = malloc(mem_len + 32);
    if (!mem)
      continue;

    uint64_t sig = 0x0101010101010101ULL * 0x42;

    char name_fio[64];
    char name_sys[64];
    snprintf(name_fio, sizeof(name_fio), "fio_memset (%zu bytes)", mem_len);
    snprintf(name_sys, sizeof(name_sys), "memset (%zu bytes)", mem_len);

    FIO_PERF_BENCHMARK_BYTES(name_fio,
                             iterations,
                             mem_len,
                             (void)0,
                             fio_memset(mem, sig, mem_len),
                             (void)0);

    FIO_PERF_BENCHMARK_BYTES(name_sys,
                             iterations,
                             mem_len,
                             (void)0,
                             memset(mem, 0x42, mem_len),
                             (void)0);

    fprintf(stderr, "\n");
    free(mem);
  }
}

/* *****************************************************************************
Performance Tests: fio_memchr
***************************************************************************** */

FIO_SFUNC void fio___perf_memchr(void) {
  fprintf(stderr, "\t* Benchmarking fio_memchr...\n");

  const size_t base_iterations = 8192;

  size_t test_sizes[] = {4, 16, 64, 256, 1024, 4096, 16384};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t s = 0; s < num_sizes; ++s) {
    size_t mem_len = test_sizes[s] - 1;
    size_t iterations = base_iterations
                        << (mem_len < 4096 ? (12 - (mem_len >> 9)) : 0);
    void *mem = malloc(mem_len + 2);
    if (!mem)
      continue;

    /* Fill with non-zero bytes, put zero at end */
    fio_memset(mem, 0x0101010101010101ULL * 0x80, mem_len + 1);
    ((uint8_t *)mem)[mem_len] = 0;

    char name_fio[64];
    char name_sys[64];
    snprintf(name_fio, sizeof(name_fio), "fio_memchr (%zu bytes)", mem_len);
    snprintf(name_sys, sizeof(name_sys), "memchr (%zu bytes)", mem_len);

    char *result = NULL;
    FIO_PERF_BENCHMARK(name_fio,
                       iterations,
                       (void)0,
                       result = (char *)fio_memchr(mem, 0, mem_len + 1),
                       (void)result);

    FIO_PERF_BENCHMARK(name_sys,
                       iterations,
                       (void)0,
                       result = (char *)memchr(mem, 0, mem_len + 1),
                       (void)result);

    fprintf(stderr, "\n");
    free(mem);
  }
}

/* *****************************************************************************
Performance Tests: fio_memcmp
***************************************************************************** */

FIO_SFUNC void fio___perf_memcmp(void) {
  fprintf(stderr, "\t* Benchmarking fio_memcmp...\n");

  const size_t base_iterations = 8192;

  size_t test_sizes[] = {4, 16, 64, 256, 1024, 4096, 16384, 65536};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t s = 0; s < num_sizes; ++s) {
    size_t mem_len = test_sizes[s];
    size_t iterations = base_iterations
                        << (mem_len < 4096 ? (12 - (mem_len >> 9)) : 2);
    char *mem = (char *)malloc((mem_len << 1) + 128);
    if (!mem)
      continue;

    uint64_t sig = (uintptr_t)mem;
    sig ^= sig >> 13;
    sig ^= sig << 17;
    char *a = mem;
    char *b = mem + mem_len + 32;
    fio_memset(a, sig, mem_len);
    fio_memset(b, sig, mem_len);

    char name_fio[64];
    char name_sys[64];
    char name_ct[64];
    snprintf(name_fio, sizeof(name_fio), "fio_memcmp (%zu bytes)", mem_len);
    snprintf(name_sys, sizeof(name_sys), "memcmp (%zu bytes)", mem_len);
    snprintf(name_ct, sizeof(name_ct), "fio_ct_is_eq (%zu bytes)", mem_len);

    int result = 0;
    FIO_PERF_BENCHMARK(name_fio,
                       iterations,
                       (void)0,
                       result = fio_memcmp(a, b, mem_len),
                       (void)result);

    FIO_PERF_BENCHMARK(name_sys,
                       iterations,
                       (void)0,
                       result = memcmp(a, b, mem_len),
                       (void)result);

    FIO_PERF_BENCHMARK(name_ct,
                       iterations,
                       (void)0,
                       result = fio_ct_is_eq(a, b, mem_len),
                       (void)result);

    fprintf(stderr, "\n");
    free(mem);
  }
}

/* *****************************************************************************
Performance Tests: fio_strlen
***************************************************************************** */

FIO_SFUNC void fio___perf_strlen(void) {
  fprintf(stderr, "\t* Benchmarking fio_strlen...\n");

  const size_t base_iterations = 8192;

  size_t test_sizes[] = {4, 16, 64, 256, 1024, 4096, 16384};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t s = 0; s < num_sizes; ++s) {
    size_t mem_len = test_sizes[s] - 1;
    size_t iterations = base_iterations
                        << (mem_len < 4096 ? (12 - (mem_len >> 9)) : 0);
    void *mem = malloc(mem_len + 1);
    if (!mem)
      continue;

    /* Fill with non-zero bytes, put zero at end */
    fio_memset(mem, 0x0101010101010101ULL * 0x80, mem_len);
    ((uint8_t *)mem)[mem_len] = 0;

    char name_fio[64];
    char name_sys[64];
    snprintf(name_fio, sizeof(name_fio), "fio_strlen (%zu chars)", mem_len);
    snprintf(name_sys, sizeof(name_sys), "strlen (%zu chars)", mem_len);

    size_t result = 0;
    FIO_PERF_BENCHMARK(name_fio,
                       iterations,
                       (void)0,
                       result = fio_strlen((char *)mem),
                       (void)result);

    FIO_PERF_BENCHMARK(name_sys,
                       iterations,
                       (void)0,
                       result = strlen((char *)mem),
                       (void)result);

    fprintf(stderr, "\n");
    free(mem);
  }
}

/* *****************************************************************************
Performance Tests: fio_memcpyX primitives (variable length up to X)
***************************************************************************** */

FIO_SFUNC void fio___perf_memcpy_x_primitives(void) {
  fprintf(stderr,
          "\t* Benchmarking fio_memcpyXx primitives (variable up to X)...\n");

  const size_t base_iterations = 8192 << 4;
  char buf[4096 * 2];
  memset(buf, 0x80, sizeof(buf));

  /* Test variable-size memcpy primitives */
  struct {
    void *(*fn)(void *, const void *, size_t);
    size_t max_bytes;
    const char *name;
  } tests[] = {
      {fio_memcpy7x, 7, "fio_memcpy7x"},
      {fio_memcpy15x, 15, "fio_memcpy15x"},
      {fio_memcpy31x, 31, "fio_memcpy31x"},
      {fio_memcpy63x, 63, "fio_memcpy63x"},
      {fio_memcpy127x, 127, "fio_memcpy127x"},
      {fio_memcpy255x, 255, "fio_memcpy255x"},
      {fio_memcpy511x, 511, "fio_memcpy511x"},
      {fio_memcpy1023x, 1023, "fio_memcpy1023x"},
      {fio_memcpy2047x, 2047, "fio_memcpy2047x"},
      {fio_memcpy4095x, 4095, "fio_memcpy4095x"},
      {NULL, 0, NULL},
  };

  for (size_t i = 0; tests[i].fn; ++i) {
    char name_fio[64];
    char name_sys[64];
    snprintf(name_fio, sizeof(name_fio), "%s", tests[i].name);
    snprintf(name_sys,
             sizeof(name_sys),
             "memcpy (up to %zu bytes)",
             tests[i].max_bytes);

    /* Use varying lengths within the range */
    size_t len = tests[i].max_bytes;
    FIO_PERF_BENCHMARK_BYTES(
        name_fio,
        base_iterations,
        len,
        (void)0,
        tests[i].fn(buf, buf + 4096, ((len + i_) & tests[i].max_bytes)),
        (void)0);

    FIO_PERF_BENCHMARK_BYTES(
        name_sys,
        base_iterations,
        len,
        (void)0,
        memcpy(buf, buf + 4096, ((len + i_) & tests[i].max_bytes)),
        (void)0);
    fprintf(stderr, "\n");
  }
}

/* *****************************************************************************
Main Entry Point
***************************************************************************** */

int main(void) {
  fprintf(stderr, "===========================================\n");
  fprintf(stderr, "Performance Tests: Memory Operations\n");
  fprintf(stderr, "===========================================\n\n");

  fio___perf_memcpy_primitives();
  fio___perf_memcpy_x_primitives();
  fio___perf_memcpy_variable();
  fio___perf_memset();
  fio___perf_memchr();
  fio___perf_memcmp();
  fio___perf_strlen();

  fprintf(stderr, "\n===========================================\n");
  fprintf(stderr, "Performance tests complete.\n");
  fprintf(stderr, "===========================================\n");
  return 0;
}

#endif /* DEBUG */
