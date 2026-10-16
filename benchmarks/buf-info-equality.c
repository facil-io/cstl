/* Buffer-info equality benchmarks: make benchmarks/buf-info-equality
 * Case-insensitive CRT comparisons use NUL-free ASCII only (C locale).
 */
#define FIO_TIME
#define FIO_CORE
#include "fio-stl/include.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <strings.h>
#endif

#define CAP 2050
#define MAX_LEN 2048
#define REPEATS 7

/* Use the compiler builtin when available, independent of FIO_MEMCMP. */
#if __has_builtin(__builtin_memcmp)
#define BENCH_MEMCMP __builtin_memcmp
#define BENCH_MEMCMP_NAME "__builtin_memcmp"
#else
#define BENCH_MEMCMP memcmp
#define BENCH_MEMCMP_NAME "memcmp"
#endif
#define ROUNDS 7
static char a_storage[CAP + 16], b_storage[CAP + 16];
static volatile size_t sink;
static size_t failures, checks;

typedef bool (*cmp_fn)(fio_buf_info_s, fio_buf_info_s);
static bool ascii_reference(fio_buf_info_s a, fio_buf_info_s b) {
  if (a.len != b.len) return false;
  for (size_t i = 0; i < a.len; ++i) {
    unsigned char x = (unsigned char)a.buf[i], y = (unsigned char)b.buf[i];
    if (x >= 'A' && x <= 'Z') x = (unsigned char)(x + 32);
    if (y >= 'A' && y <= 'Z') y = (unsigned char)(y + 32);
    if (x != y) return false;
  }
  return true;
}
#if defined(_WIN32)
#define BENCH_CASECMP_NAME "_strnicmp"
#else
#define BENCH_CASECMP_NAME "strncasecmp"
#endif
static bool via_casecmp(fio_buf_info_s a, fio_buf_info_s b) {
  if (a.len != b.len)
    return false;
#if defined(_WIN32)
  return !_strnicmp(a.buf, b.buf, a.len);
#else
  return !strncasecmp(a.buf, b.buf, a.len);
#endif
}
static void check(fio_buf_info_s a, fio_buf_info_s b, const char *scenario,
                  size_t position) {
  bool want = ascii_reference(a, b);
  bool got = fio_buf_info_is_ieq(a, b);
  ++checks;
  if (got != want) {
    if (failures < 24)
      fprintf(stderr, "FAIL %s len=%zu pos=%zu a=%02x b=%02x got=%d want=%d\n",
              scenario, a.len, position,
              a.len && position < a.len ? (unsigned char)a.buf[position] : 0,
              b.len && position < b.len ? (unsigned char)b.buf[position] : 0,
              got, want);
    ++failures;
  }
}
static void correctness(void) {
  check((fio_buf_info_s){0, NULL}, (fio_buf_info_s){0, NULL}, "empty", 0);
  /* Exhaustive single-byte comparisons, including high bytes and NUL. */
  for (unsigned x = 0; x < 256; ++x)
    for (unsigned y = 0; y < 256; ++y) {
      a_storage[0] = (char)x; b_storage[0] = (char)y;
      check((fio_buf_info_s){1, a_storage}, (fio_buf_info_s){1, b_storage},
            "all byte pairs", 0);
    }
  const size_t lengths[] = {1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33,
                            63, 64, 65, 127, 128, 129, 255, 256,
                            1023, 1024, 1025, 2047, 2048};
  for (size_t k = 0; k < sizeof lengths / sizeof *lengths; ++k) {
    size_t n = lengths[k];
    for (size_t offset = 0; offset < 8; ++offset) {
      char *a = a_storage + offset, *b = b_storage + ((offset + 3) & 7);
      for (size_t i = 0; i < n; ++i) {
        a[i] = "Az@`[_{|9!"[i % 10];
        b[i] = a[i];
        if (a[i] >= 'A' && a[i] <= 'Z') b[i] += 32;
      }
      fio_buf_info_s x = {n, a}, y = {n, b};
      check(x, y, "mixed case", n - 1);
      check(x, (fio_buf_info_s){n - 1, b}, "length", n - 1);
      for (size_t pos = 0; pos < n; ++pos) {
        char old = b[pos];
        b[pos] = '#'; check(x, y, "mismatch", pos);
        b[pos] = old;
        a[pos] = (char)0xC0; b[pos] = (char)0xE0;
        check(x, y, "high byte", pos);
        a[pos] = old >= 'a' && old <= 'z' ? old - 32 : old;
        b[pos] = old;
      }
    }
  }
  printf("correctness: %zu checks, %zu failures\n", checks, failures);
}
static int cmp_u64(const void *a, const void *b) {
  uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
  return (x > y) - (x < y);
}
static unsigned char exact_left[MAX_LEN + 16], exact_right[MAX_LEN + 16];
static volatile size_t exact_observed;

typedef bool (*exact_compare_fn)(fio_buf_info_s, fio_buf_info_s);

static bool exact_compare_memcmp(fio_buf_info_s a, fio_buf_info_s b) {
  return a.len == b.len && !BENCH_MEMCMP(a.buf, b.buf, a.len);
}

static bool exact_compare_buf(fio_buf_info_s a, fio_buf_info_s b) {
  return fio_buf_info_is_eq(a, b);
}

static void exact_check(fio_buf_info_s a, fio_buf_info_s b, bool expected,
                  size_t length, size_t position) {
  bool actual = exact_compare_buf(a, b);
  bool reference = exact_compare_memcmp(a, b);
  if (actual != expected || reference != expected) {
    fprintf(stderr, "FAIL len=%zu pos=%zu a.len=%zu b.len=%zu buf=%d memcmp=%d expected=%d\n",
            length, position, a.len, b.len, actual, reference, expected);
    exit(1);
  }
}

static void exact_correctness(void) {
  for (size_t i = 0; i < sizeof exact_left; ++i) {
    exact_left[i] = (unsigned char)(i * 137U + 29U);
    exact_right[i] = exact_left[i];
  }
  exact_check((fio_buf_info_s){0, NULL}, (fio_buf_info_s){0, NULL}, true, 0, 0);
  for (size_t offset = 0; offset < 8; ++offset) {
    char *a = (char *)exact_left + offset;
    char *b = (char *)exact_right + ((offset + 3) & 7);
    /* Recreate identical bytes even at different alignments. */
    for (size_t n = 0; n < MAX_LEN; ++n) b[n] = a[n];
    for (size_t len = 0; len <= MAX_LEN; ++len) {
      fio_buf_info_s x = {len, a}, y = {len, b};
      exact_check(x, y, true, len, len);
      exact_check(x, x, true, len, len);
      exact_check(x, (fio_buf_info_s){len + 1, b}, false, len, len);
      if (!len) continue;
      for (size_t pos = 0; pos < len; ++pos) {
        b[pos] ^= 0x80;
        exact_check(x, y, false, len, pos);
        b[pos] ^= 0x80;
      }
    }
  }
  puts("correctness: all lengths 0..2048, byte positions, alignments, and unequal lengths passed");
}

static uint64_t exact_run(exact_compare_fn fn, fio_buf_info_s a, fio_buf_info_s b,
                    size_t iterations) {
  size_t sum = 0;
  uint64_t start = fio_time_micro();
  for (size_t i = 0; i < iterations; ++i) {
    sum += fn(a, b);
    FIO_COMPILER_GUARD;
  }
  uint64_t elapsed = fio_time_micro() - start;
  exact_observed += sum;
  return elapsed;
}

static void exact_bench(size_t len, size_t mismatch, size_t iterations) {
  char *a = (char *)exact_left, *b = (char *)exact_right;
  for (size_t i = 0; i < len; ++i) b[i] = a[i];
  if (mismatch < len) b[mismatch] ^= 0x80;
  fio_buf_info_s x = {len, a}, y = {len, b};
  uint64_t times[2][REPEATS];
  exact_compare_fn fns[2] = {exact_compare_buf, exact_compare_memcmp};
  for (size_t round = 0; round < REPEATS + 2; ++round) {
    for (size_t slot = 0; slot < 2; ++slot) {
      size_t index = (round + slot) & 1;
      uint64_t t = exact_run(fns[index], x, y, iterations);
      if (round >= 2) times[index][round - 2] = t;
    }
  }
  qsort(times[0], REPEATS, sizeof times[0][0], cmp_u64);
  qsort(times[1], REPEATS, sizeof times[1][0], cmp_u64);
  printf("%5zu %-6s %9zu %21llu %11llu %6.2fx\n", len,
         mismatch == len ? "equal" : mismatch == 0 ? "first" :
         mismatch == len - 1 ? "last" : "middle", iterations,
         (unsigned long long)times[0][REPEATS / 2],
         (unsigned long long)times[1][REPEATS / 2],
         (double)times[1][REPEATS / 2] / (double)times[0][REPEATS / 2]);
  if (mismatch < len) b[mismatch] ^= 0x80;
}


static uint64_t timed(cmp_fn fn, fio_buf_info_s a, fio_buf_info_s b, size_t iters) {
  size_t total = 0;
  uint64_t start = fio_time_micro();
  for (size_t i = 0; i < iters; ++i) {
    total += fn(a, b);
    FIO_COMPILER_GUARD;
  }
  sink += total;
  return fio_time_micro() - start;
}
static void bench(size_t n, size_t mismatch) {
  for (size_t i = 0; i < n; ++i) {
    a_storage[i] = (char)('A' + i % 26);
    b_storage[i] = (char)('a' + i % 26);
  }
  a_storage[n] = b_storage[n] = 0;
  if (mismatch < n) b_storage[mismatch] = '#';
  fio_buf_info_s a = {n, a_storage}, b = {n, b_storage};
  size_t iters = 2000000 / n;
  if (iters < 20000) iters = 20000;
  /* Only compare timings for inputs both implementations handle correctly. */
  bool want = ascii_reference(a, b);
  if (fio_buf_info_is_ieq(a, b) != want || via_casecmp(a, b) != want) {
    fprintf(stderr, "FAIL benchmark input len=%zu mismatch=%zu\n", n, mismatch);
    ++failures;
    return;
  }
  cmp_fn fns[2] = {fio_buf_info_is_ieq, via_casecmp};
  uint64_t times[2][ROUNDS];
  for (size_t round = 0; round < ROUNDS + 2; ++round)
    for (size_t slot = 0; slot < 2; ++slot) {
      size_t index = (round + slot) % 2;
      uint64_t us = timed(fns[index], a, b, iters);
      if (round >= 2) times[index][round - 2] = us;
    }
  for (size_t i = 0; i < 2; ++i)
    qsort(times[i], ROUNDS, sizeof times[i][0], cmp_u64);
  printf("%5zu %-6s %10zu %12llu %14llu %14.2fx\n", n,
         mismatch == n ? "equal" : mismatch == 0 ? "first" :
         mismatch == n - 1 ? "last" : "middle", iters,
         (unsigned long long)times[0][ROUNDS / 2],
         (unsigned long long)times[1][ROUNDS / 2],
         (double)times[1][ROUNDS / 2] / (double)times[0][ROUNDS / 2]);
}
int main(void) {
  correctness();
  if (failures) {
    fprintf(stderr, "Skipping timings: correctness failures.\n");
    return 1;
  }
  printf("ASCII case-insensitive equality: fio_buf_info_is_ieq vs %s\n", BENCH_CASECMP_NAME);
  puts("NUL-free ASCII; C locale; times are microseconds, median of 7 runs after 2 warmups.");
  printf("Ratio = %s time / fio_buf_info_is_ieq time (>1 favors fio_buf_info_is_ieq).\n", BENCH_CASECMP_NAME);
  printf("bytes case        calls/run  fio_buf_info_is_ieq  %s  %s/ieq\n", BENCH_CASECMP_NAME, BENCH_CASECMP_NAME);
  const size_t lengths[] = {1, 3, 4, 5, 7, 8, 9, 15, 16, 17,
                            31, 32, 33, 63, 64, 65, 127, 128, 129,
                            1023, 1024, 1025};
  for (size_t i = 0; i < sizeof lengths / sizeof *lengths; ++i) {
    size_t n = lengths[i];
    bench(n, n);
    bench(n, 0);
    if (n > 2) bench(n, n / 2);
    if (n > 1) bench(n, n - 1);
  }
  puts("\nExact buffer-info equality: fio_buf_info_is_eq vs " BENCH_MEMCMP_NAME);
  exact_correctness();
  puts("fio_buf_info_is_eq benchmarks: times in microseconds, median of 7 runs after 2 warmups.");
  puts("Ratio = comparator time / fio_buf_info_is_eq time (>1 favors fio_buf_info_is_eq).");
  printf("bytes case   calls/run  fio_buf_info_is_eq_us  %s_us  %s/eq\n",
         BENCH_MEMCMP_NAME, BENCH_MEMCMP_NAME);
  fflush(stdout);
  const size_t exact_lengths[] = {1, 7, 8, 9, 31, 32, 33, 127, 128, 129,
                                  1023, 1024, 1025};
  for (size_t k = 0; k < sizeof exact_lengths / sizeof exact_lengths[0]; ++k) {
    size_t len = exact_lengths[k];
    size_t iterations = 20000000 / len;
    if (iterations < 100000) iterations = 100000;
    exact_bench(len, len, iterations);
    exact_bench(len, 0, iterations);
    if (len > 1) exact_bench(len, len / 2, iterations);
    if (len > 1) exact_bench(len, len - 1, iterations);
  }
  fflush(stdout);
  fprintf(stderr, "checksums: ASCII=%zu exact=%zu\n", (size_t)sink, (size_t)exact_observed);
  return failures ? 1 : 0;
}
