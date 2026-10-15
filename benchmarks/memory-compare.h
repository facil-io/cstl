/* Shared seek / comparison benchmark. Include after fio-stl/include.h. */
#ifndef H___BENCH_MEMORY_COMPARE___H
#define H___BENCH_MEMORY_COMPARE___H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BENCH_MAX_LEN ((size_t)65536)
#define BENCH_ROUNDS  5
#define BENCH_WARMUPS 2
#define BENCH_RESULT_CAP 256

typedef struct bench_results_s {
  double exact[BENCH_RESULT_CAP];
  double ranges[BENCH_RESULT_CAP];
  size_t exact_count;
  size_t range_count;
  size_t exact_wins;
  size_t range_wins;
} bench_results_s;

typedef struct bench_op_s {
  const char *name;
  const char *reference_name;
  intptr_t (*fio)(const unsigned char *, const unsigned char *, size_t);
  intptr_t (*reference)(const unsigned char *, const unsigned char *, size_t);
  int seek;  /* 0: equality, 1: seek index, 2: signed comparison */
  int ascii; /* case-folded equal buffers use upper/lowercase ASCII */
} bench_op_s;

static volatile uintptr_t bench_observed;
static unsigned char bench_a[BENCH_MAX_LEN + 16];
static unsigned char bench_b[BENCH_MAX_LEN + 16];

FIO_SFUNC int bench_double_cmp(const void *a, const void *b) {
  double x = *(const double *)a, y = *(const double *)b;
  return (x > y) - (x < y);
}
FIO_SFUNC int bench_u64_cmp(const void *a, const void *b) {
  uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
  return (x > y) - (x < y);
}

/* Odd index just below halfway; avoid unsigned underflow for len < 2. */
FIO_SFUNC size_t bench_middle_index(size_t len) {
  return len > 1 ? (((len >> 1) - 1) | (size_t)1) : 0;
}
FIO_SFUNC int bench_has_middle(size_t len) {
  size_t middle = bench_middle_index(len);
  return middle && middle < len - 1;
}

/* Positions: equal/absent, first, middle (when distinct), last. */
FIO_SFUNC size_t bench_positions(size_t len, size_t *positions) {
  size_t count = 0;
  positions[count++] = len; /* equal / absent sentinel */
  positions[count++] = 0;
  if (bench_has_middle(len))
    positions[count++] = bench_middle_index(len);
  if (len > 1)
    positions[count++] = len - 1;
  return count;
}

FIO_SFUNC void bench_prepare(const bench_op_s *op, size_t len, size_t pos) {
  /* Buffers are preallocated and initialized once per invocation. */
  if (op->seek == 1) {
    if (pos < len)
      bench_a[pos] = bench_b[0];
  } else if (pos < len) {
    bench_b[pos] = '#';
  }
}
FIO_SFUNC void bench_restore(const bench_op_s *op, size_t len, size_t pos) {
  if (pos >= len)
    return;
  if (op->seek == 1)
    bench_a[pos] = 'A';
  else
    bench_b[pos] = op->ascii ? 'a' : 'A';
}
FIO_SFUNC void bench_check(const bench_op_s *op, size_t len, size_t pos) {
  intptr_t got = op->fio(bench_a, bench_b, len);
  intptr_t ref = op->reference(bench_a, bench_b, len);
  int okay = op->seek == 1
                 ? (got == ref && (pos == len ? got == -1 : got == (intptr_t)pos))
                 : op->seek == 2
                       ? ((got > 0) - (got < 0) == (ref > 0) - (ref < 0) &&
                          (pos == len ? got == 0 : got != 0))
                       : (got == ref && !!got == (pos == len));
  if (!okay) {
    fprintf(stderr,
            "FAIL %s %zu[%zu] fio=%lld reference=%lld\n",
            op->name,
            len,
            pos,
            (long long)got,
            (long long)ref);
    exit(1);
  }
}

FIO_SFUNC uint64_t bench_exact_run(const bench_op_s *op,
                                   size_t len,
                                   size_t calls,
                                   int reference) {
  intptr_t (*fn)(const unsigned char *, const unsigned char *, size_t) =
      reference ? op->reference : op->fio;
  uintptr_t observed = 0;
  uint64_t start = (uint64_t)fio_time_nano();
  for (size_t i = 0; i < calls; ++i) {
    observed += (uintptr_t)fn(bench_a, bench_b, len);
    FIO_COMPILER_GUARD;
  }
  uint64_t elapsed = (uint64_t)fio_time_nano() - start;
  bench_observed += observed;
  return elapsed;
}

FIO_SFUNC void bench_print(const char *label,
                           double fio_ns,
                           double ref_ns,
                           size_t count,
                           size_t rounds) {
  if (fio_ns <= 0 || ref_ns <= 0) {
    printf("%-16s %9zu %4zu %20.2f %20.2f %8s\n",
           label,
           count,
           rounds,
           fio_ns,
           ref_ns,
           "--");
    return;
  }
  double ratio = ref_ns / fio_ns;
  /* Match FIO_LOG_ERROR (bold), FIO_LOG_WARNING (dim), and
   * FIO_LOG_FATAL (bold inverse) without hard-coded terminal colors. */
  const char *fb = fio_ns < ref_ns   ? "\033[1m"
                   : fio_ns > ref_ns ? "\033[2m"
                                     : "";
  const char *rb = ref_ns < fio_ns   ? "\033[1m"
                   : ref_ns > fio_ns ? "\033[2m"
                                     : "";
  const char *xb = ratio < 0.99 ? "\033[1;7m" : "";
  printf("%-16s %9zu %4zu %s%20.2f%s %s%20.2f%s %s%7.3fx%s\n",
         label,
         count,
         rounds,
         fb,
         fio_ns,
         *fb ? "\033[0m" : "",
         rb,
         ref_ns,
         *rb ? "\033[0m" : "",
         xb,
         ratio,
         *xb ? "\033[0m" : "");
}

/* Store per-row reference/C STL multipliers; medians avoid letting a single
 * extreme length dominate the summary. Each exact or prefix row counts once. */
FIO_SFUNC void bench_record(bench_results_s *results, double ratio, int range) {
  double *values = range ? results->ranges : results->exact;
  size_t *count = range ? &results->range_count : &results->exact_count;
  size_t *wins = range ? &results->range_wins : &results->exact_wins;
  if (*count >= BENCH_RESULT_CAP) {
    fputs("Too many benchmark results\n", stderr);
    exit(1);
  }
  values[(*count)++] = ratio;
  *wins += ratio > 1.0;
}

FIO_SFUNC void bench_summary_group(const char *name, double *values,
                                   size_t count, size_t wins) {
  if (!count) return;
  qsort(values, count, sizeof *values, bench_double_cmp);
  double median = count & 1 ? values[count / 2]
                            : (values[count / 2 - 1] + values[count / 2]) / 2;
  printf("  %-8s %3zu rows, %3zu wins, median %7.3fx (min %7.3fx, max %7.3fx)\n",
         name, count, wins, median, values[0], values[count - 1]);
}

/* Returns the unrounded comparison multiplier for adaptive aggregate rounds. */
FIO_SFUNC double bench_exact(const bench_op_s *op, size_t len, size_t pos) {
  size_t calls = pos == 0 ? 16384 : 262144 / len;
  if (calls < 32)
    calls = 32;
  if (calls > 16384)
    calls = 16384;
  bench_prepare(op, len, pos);
  bench_check(op, len, pos);
  uint64_t times[2][BENCH_ROUNDS];
  for (size_t round = 0; round < BENCH_ROUNDS + BENCH_WARMUPS; ++round)
    for (size_t slot = 0; slot < 2; ++slot) {
      size_t index = (round + slot) & 1;
      uint64_t t = bench_exact_run(op, len, calls, (int)index);
      if (round >= BENCH_WARMUPS)
        times[index][round - BENCH_WARMUPS] = t;
    }
  for (size_t i = 0; i < 2; ++i)
    qsort(times[i], BENCH_ROUNDS, sizeof times[i][0], bench_u64_cmp);
  char label[48];
  if (pos == len)
    snprintf(label, sizeof label, "%zu[eq]", len);
  else
    snprintf(label, sizeof label, "%zu[%zu]", len, pos);
  bench_print(label,
              (double)times[0][BENCH_ROUNDS / 2] / calls,
              (double)times[1][BENCH_ROUNDS / 2] / calls,
              calls,
              BENCH_ROUNDS);
  bench_restore(op, len, pos);
  return times[0][BENCH_ROUNDS / 2]
             ? (double)times[1][BENCH_ROUNDS / 2] / times[0][BENCH_ROUNDS / 2]
             : 1.0;
}

/* For sizes >4 KiB, a slow equal/absent result reduces measured full-prefix
 * sweeps for BOTH functions. factor = 2^floor(log2(1/ratio)); 0.20 => 4.
 * The trigger is strictly ratio <0.5; the 64 KiB aggregate is capped at two
 * measured sweeps regardless of ratio. Always retain at least one sweep. */
FIO_SFUNC size_t bench_aggregate_rounds(size_t n, double ratio) {
  size_t factor = 1;
  if (n > 4096 && ratio > 0.0 && ratio < 0.5) {
    do {
      factor <<= 1;
      ratio *= 2.0;
    } while (ratio <= 0.5 && factor < BENCH_ROUNDS);
  }
  size_t result = BENCH_ROUNDS / factor;
  if (n == BENCH_MAX_LEN && result > 2)
    result = 2;
  return result ? result : 1;
}

/* Change only this function to adjust the aggregate window. The default
 * tests the current power-of-two group; alternatives:
 *   return (n >> 2) + 1;
 *   size_t begin = (n >> 2) + 1;
 *   return begin > 9 ? 9 : begin;
 */
FIO_SFUNC size_t bench_aggregate_begin(size_t n) {
  return (n >> 1) + 1;
}

/* Each measured round traverses lengths begin..N and all distinct cases.
 * Short ranges repeat until a round has at least 32768 calls.
 * Two complete-range warmups are discarded; up to five complete-range
 * measurements are sorted and the middle (upper middle for two) is divided
 * by the calls per round. Preparation/restoration is included equally;
 * correctness checks are untimed. */
FIO_SFUNC uint64_t bench_block_run(const bench_op_s *op,
                                   size_t begin,
                                   size_t end,
                                   size_t repeats,
                                   int reference) {
  intptr_t (*fn)(const unsigned char *, const unsigned char *, size_t) =
      reference ? op->reference : op->fio;
  uintptr_t observed = 0;
  uint64_t start = (uint64_t)fio_time_nano();
  for (size_t rep = 0; rep < repeats; ++rep)
    for (size_t len = begin; len <= end; ++len) {
      size_t positions[4];
      size_t count = bench_positions(len, positions);
      for (size_t k = 0; k < count; ++k) {
        size_t pos = positions[k];
        bench_prepare(op, len, pos);
        observed += (uintptr_t)fn(bench_a, bench_b, len);
        FIO_COMPILER_GUARD;
        bench_restore(op, len, pos);
      }
    }
  uint64_t elapsed = (uint64_t)fio_time_nano() - start;
  bench_observed += observed;
  return elapsed;
}

FIO_SFUNC size_t bench_block_count(size_t begin, size_t end) {
  size_t total = 0;
  for (size_t len = begin; len <= end; ++len) {
    size_t positions[4];
    total += bench_positions(len, positions);
  }
  return total;
}

FIO_SFUNC void bench_memory_run(const bench_op_s *op) {
  bench_results_s results = {0};
  bench_observed = 0;
  /* Guard the adaptive-round thresholds, including the example 0.20 -> 1/4. */
  if (bench_aggregate_rounds(4096, 0.20) != 5 ||
      bench_aggregate_rounds(8192, 0.50) != 5 ||
      bench_aggregate_rounds(8192, 0.49) != 2 ||
      bench_aggregate_rounds(8192, 0.20) != 1 ||
      bench_aggregate_rounds(8192, 0.10) != 1 ||
      bench_aggregate_rounds(BENCH_MAX_LEN, 1.0) != 2 ||
      bench_aggregate_rounds(BENCH_MAX_LEN, 0.20) != 1) {
    fputs("Invalid aggregate round selection\n", stderr);
    exit(1);
  }
  for (size_t i = 0; i < BENCH_MAX_LEN; ++i) {
    bench_a[i] = 'A';
    bench_b[i] = op->seek == 1 ? '#' : op->ascii ? 'a' : 'A';
  }
  /* Search target is '#', absent from the remaining data. */
  bench_check(op, 0, 0);
  printf("\n%s vs %s\n", op->name, op->reference_name);
  puts("ns/call, median after 2 warmups; ranges average the shown lengths "
       "and all cases; multiplier = reference/C STL (>1 favors C STL)");
  printf("%-16s %9s %4s %20s %20s %8s\n",
         "input",
         "samples",
         "runs",
         op->name,
         op->reference_name,
         "mult.");
  size_t previous = 0;
  for (size_t n = 1; n <= BENCH_MAX_LEN; n <<= 1) {
    double equal_ratio = 1.0;
    /* Show the byte before each power-of-two boundary as an exact-size case.
     * 1 is already an endpoint, so the first distinct predecessor is 3. */
    size_t lengths[2] = {n > 2 ? n - 1 : n, n};
    for (size_t row = (n > 2 ? 0 : 1); row < 2; ++row) {
      size_t len = lengths[row];
      size_t positions[4];
      size_t count = bench_positions(len, positions);
      for (size_t k = 0; k < count; ++k) {
        double ratio = bench_exact(op, len, positions[k]);
        bench_record(&results, ratio, 0);
        if (row == 1 && !k)
          equal_ratio = ratio;
      }
    }
    /* Validate the whole block before taking performance measurements. */
    for (size_t len = previous + 1; len <= n; ++len) {
      size_t variants[4];
      size_t variants_count = bench_positions(len, variants);
      for (size_t k = 0; k < variants_count; ++k) {
        bench_prepare(op, len, variants[k]);
        bench_check(op, len, variants[k]);
        bench_restore(op, len, variants[k]);
      }
    }
    /* n=1 has an exact row, but no aggregate. */
    if (n < 2) {
      previous = n;
      continue;
    }
    size_t begin = bench_aggregate_begin(n);
    size_t count_all = bench_block_count(begin, n);
    size_t repeats =
        count_all < 32768 ? (32768 + count_all - 1) / count_all : 1;
    size_t rounds = bench_aggregate_rounds(n, equal_ratio);
    uint64_t times[2][BENCH_ROUNDS];
    for (size_t round = 0; round < rounds + BENCH_WARMUPS; ++round)
      for (size_t slot = 0; slot < 2; ++slot) {
        size_t index = (round + slot) & 1;
        uint64_t t = bench_block_run(op, begin, n, repeats, (int)index);
        if (round >= BENCH_WARMUPS)
          times[index][round - BENCH_WARMUPS] = t;
      }
    for (size_t i = 0; i < 2; ++i)
      qsort(times[i], rounds, sizeof times[i][0], bench_u64_cmp);
    char label[48];
    snprintf(label, sizeof label, "%zu-%zu", begin, n);
    double fio_ns = (double)times[0][rounds / 2] / (count_all * repeats);
    double ref_ns = (double)times[1][rounds / 2] / (count_all * repeats);
    bench_print(label, fio_ns, ref_ns, count_all * repeats, rounds);
    bench_record(&results, fio_ns > 0 ? ref_ns / fio_ns : 1.0, 1);
    previous = n;
    fflush(stdout);
  }
  printf("Summary for %s vs %s (per-row reference/C STL; >1 favors C STL):\n",
         op->name, op->reference_name);
  bench_summary_group("exact", results.exact, results.exact_count,
                      results.exact_wins);
  bench_summary_group("ranges", results.ranges, results.range_count,
                      results.range_wins);
  fprintf(stderr, "%s checksum: %zu\n", op->name, (size_t)bench_observed);
}
#endif /* H___BENCH_MEMORY_COMPARE___H */
