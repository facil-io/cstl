/* Buffer-info equality benchmarks: make benchmarks/buf-info-equality
 * strncasecmp/_strnicmp timings use NUL-free ASCII in the C locale.
 */
#define FIO_CORE
#include "fio-stl/include.h"
#include "memory-compare.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <strings.h>
#endif

#if __has_builtin(__builtin_memcmp)
#define BENCH_MEMCMP __builtin_memcmp
#define BENCH_MEMCMP_NAME "__builtin_memcmp"
#else
#define BENCH_MEMCMP memcmp
#define BENCH_MEMCMP_NAME "memcmp"
#endif
#if defined(_WIN32)
#define BENCH_CASECMP_NAME "_strnicmp"
#else
#define BENCH_CASECMP_NAME "strncasecmp"
#endif

static intptr_t buf_eq(const unsigned char *a, const unsigned char *b, size_t n) {
  return fio_buf_info_is_eq((fio_buf_info_s){n, (char *)a},
                            (fio_buf_info_s){n, (char *)b});
}
static intptr_t mem_eq(const unsigned char *a, const unsigned char *b, size_t n) {
  return BENCH_MEMCMP(a, b, n) == 0;
}
static intptr_t buf_ieq(const unsigned char *a, const unsigned char *b, size_t n) {
  return fio_buf_info_is_ieq((fio_buf_info_s){n, (char *)a},
                             (fio_buf_info_s){n, (char *)b});
}
static intptr_t case_eq(const unsigned char *a, const unsigned char *b, size_t n) {
#if defined(_WIN32)
  return _strnicmp((const char *)a, (const char *)b, n) == 0;
#else
  return strncasecmp((const char *)a, (const char *)b, n) == 0;
#endif
}

/* Unlike the CRT comparator, this oracle also covers NUL and high bytes. */
static int ascii_eq(const unsigned char *a, const unsigned char *b, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    unsigned x = a[i], y = b[i];
    if (x >= 'A' && x <= 'Z') x += 'a' - 'A';
    if (y >= 'A' && y <= 'Z') y += 'a' - 'A';
    if (x != y) return 0;
  }
  return 1;
}

static void check(const unsigned char *a, const unsigned char *b, size_t an,
                  size_t bn, const char *scenario, size_t pos) {
  int exact = an == bn && (an == 0 || BENCH_MEMCMP(a, b, an) == 0);
  int folded = an == bn && (an == 0 || ascii_eq(a, b, an));
  int got_exact = fio_buf_info_is_eq((fio_buf_info_s){an, (char *)a},
                                     (fio_buf_info_s){bn, (char *)b});
  int got_folded = fio_buf_info_is_ieq((fio_buf_info_s){an, (char *)a},
                                       (fio_buf_info_s){bn, (char *)b});
  if (got_exact != exact || got_folded != folded) {
    fprintf(stderr, "FAIL %s len=%zu/%zu pos=%zu eq=%d/%d ieq=%d/%d\n",
            scenario, an, bn, pos, got_exact, exact, got_folded, folded);
    exit(1);
  }
}

static void correctness(void) {
  unsigned char a_storage[2064], b_storage[2064];
  check(NULL, NULL, 0, 0, "empty", 0);
  /* Every single-byte pairing, including NUL, ASCII case, and high bytes. */
  for (unsigned x = 0; x < 256; ++x)
    for (unsigned y = 0; y < 256; ++y) {
      a_storage[0] = (unsigned char)x;
      b_storage[0] = (unsigned char)y;
      check(a_storage, b_storage, 1, 1, "all byte pairs", 0);
    }
  const size_t lengths[] = {1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33,
                            63, 64, 65, 127, 128, 129, 255, 256,
                            1023, 1024, 1025, 2047, 2048};
  for (size_t k = 0; k < sizeof lengths / sizeof *lengths; ++k) {
    size_t n = lengths[k];
    for (size_t offset = 0; offset < 8; ++offset) {
      unsigned char *a = a_storage + offset;
      unsigned char *b = b_storage + ((offset + 3) & 7);
      for (size_t i = 0; i < n; ++i) {
        a[i] = (unsigned char)"Az@`[_{|9!"[i % 10];
        b[i] = a[i];
        if (b[i] >= 'A' && b[i] <= 'Z') b[i] += 'a' - 'A';
      }
      check(a, b, n, n, "mixed case", n - 1);
      check(a, b, n, n - 1, "unequal lengths", n - 1);
      check(a, a, n, n, "same pointer", n - 1);
      for (size_t pos = 0; pos < n; ++pos) {
        unsigned char old_a = a[pos], old_b = b[pos];
        b[pos] = '#';
        check(a, b, n, n, "mismatch", pos);
        a[pos] = 0xC0;
        b[pos] = 0xE0;
        check(a, b, n, n, "high byte", pos);
        a[pos] = old_a;
        b[pos] = old_b;
      }
    }
  }
  puts("correctness: byte pairs, lengths, offsets, mismatches and high bytes passed");
}

int main(void) {
  if (!setlocale(LC_CTYPE, "C")) {
    fputs("Could not set C locale\n", stderr);
    return 1;
  }
  correctness();
  const bench_op_s exact = {"fio_buf_info_is_eq", BENCH_MEMCMP_NAME,
                            buf_eq, mem_eq, 0, 0};
  const bench_op_s folded = {"fio_buf_info_is_ieq", BENCH_CASECMP_NAME,
                             buf_ieq, case_eq, 0, 1};
  bench_memory_run(&exact);
  bench_memory_run(&folded);
  return 0;
}
