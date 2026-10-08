/* *****************************************************************************
Randomness Quality + Performance: Core and Random Modules

Generates the same amount of data from every randomness source, measures the
generation speed (median of 5 runs after 2 warmups, plus CV) and reports
statistical quality metrics for the final run's output.

Sources:
- C `rand` (naive: two calls per 64 bits, ignores the missing RAND_MAX bits).
- C `rand` (fixed: enough calls / shifts to fill all 64 bits).
- Random module: `fio_rand64`, `fio_rand128`, `fio_rand_bytes` (reseeding).
- Random module: `fio_rand_bytes_secure` (system CSPRNG).
- Core module: a non-reseeding `FIO_DEFINE_RANDOM128_FN` (64/128/bytes).

Metrics (z-scores: |z| < 3 ok, < 5 weak, otherwise BAD):
- bit frequency     - ratio of 1 bits (monobit test).
- bit positions     - chi-square over the bias of all 64 bit indexes (stuck or
                      biased bits; the most biased index is named).
- hamming distance  - mean bit difference between consecutive 64 bit words.
- bit transitions   - runs test: 0/1 changes between consecutive bits.
- chi-square (8b)   - byte value distribution (255 degrees of freedom).
- chi-square (16b)  - 16 bit value distribution (65535 degrees of freedom).
- serial corr.      - lag-1 byte correlation coefficient (ENT style).
- Monte Carlo pi    - 32 bit (x,y) pairs inside the unit quarter circle.
- entropy           - Shannon bits per byte (informational, no z-score).

The process fails (exit 1) only if a facil.io source scores BAD; the C `rand`
rows are reference points and never fail the run.

Run with: make benchmarks/random   (sample size: -DFIO_RANDOM_BENCH_LOG=21)
***************************************************************************** */
#define FIO_LOG
#define FIO_TIME
#define FIO_RAND
#include "tests/test-helpers.h"

#ifndef FIO_RANDOM_BENCH_LOG
/* log2 of the number of 64 bit words per sample (21 => 16 MiB) */
#define FIO_RANDOM_BENCH_LOG 21
#endif
#define FIO___RB_WORDS  ((size_t)1 << FIO_RANDOM_BENCH_LOG)
#define FIO___RB_RUNS   7 /* total timed runs */
#define FIO___RB_WARMUP 2 /* discarded runs */
#define FIO___RB_PI     3.14159265358979323846

/* Core module: a deterministic, never reseeding PRNG (reseed_log == 0). */
FIO_DEFINE_RANDOM128_FN(FIO_SFUNC, fio___rb_prng, 0, 0)

/* *****************************************************************************
Sources (each fills `words` 64 bit words)
***************************************************************************** */

FIO_SFUNC void fio___rb_c_rand_naive(uint64_t *w, size_t words) {
  for (size_t i = 0; i < words; ++i)
    w[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
}

/* bits produced per `rand()` call (assumes RAND_MAX == 2^n - 1) */
FIO_SFUNC size_t fio___rb_c_rand_bits(void) {
  size_t bits = 0;
  for (uint64_t m = (uint64_t)RAND_MAX; m & 1; m >>= 1)
    ++bits;
  return bits ? bits : 1;
}

FIO_SFUNC void fio___rb_c_rand_full(uint64_t *w, size_t words) {
  const size_t bits = fio___rb_c_rand_bits();
  for (size_t i = 0; i < words; ++i) {
    uint64_t r = 0;
    for (size_t b = 0; b < 64; b += bits)
      r = (r << (bits & 63)) ^ (uint64_t)rand();
    w[i] = r;
  }
}

FIO_SFUNC void fio___rb_fio_rand64(uint64_t *w, size_t words) {
  for (size_t i = 0; i < words; ++i)
    w[i] = fio_rand64();
}

FIO_SFUNC void fio___rb_fio_rand128(uint64_t *w, size_t words) {
  for (size_t i = 0; i + 1 < words; i += 2) {
    fio_u128 r = fio_rand128();
    fio_memcpy16(w + i, r.u8);
  }
}

FIO_SFUNC void fio___rb_fio_rand_bytes(uint64_t *w, size_t words) {
  fio_rand_bytes(w, words * sizeof(*w));
}

FIO_SFUNC void fio___rb_fio_rand_secure(uint64_t *w, size_t words) {
  FIO_ASSERT(!fio_rand_bytes_secure(w, words * sizeof(*w)),
             "fio_rand_bytes_secure failed");
}

FIO_SFUNC void fio___rb_prng64_fill(uint64_t *w, size_t words) {
  for (size_t i = 0; i < words; ++i)
    w[i] = fio___rb_prng64();
}

FIO_SFUNC void fio___rb_prng128_fill(uint64_t *w, size_t words) {
  for (size_t i = 0; i + 1 < words; i += 2) {
    fio_u128 r = fio___rb_prng128();
    fio_memcpy16(w + i, r.u8);
  }
}

FIO_SFUNC void fio___rb_prng_bytes_fill(uint64_t *w, size_t words) {
  fio___rb_prng_bytes(w, words * sizeof(*w));
}

typedef struct {
  const char *name;
  const char *details;
  void (*fill)(uint64_t *, size_t);
  int is_fio; /* facil.io sources fail the run when BAD */
} fio___rb_source_s;

static const fio___rb_source_s fio___rb_sources[] = {
    {"rand (naive)", "C rand, 2 calls / 64 bits", fio___rb_c_rand_naive, 0},
    {"rand (fixed)", "C rand, fills all 64 bits", fio___rb_c_rand_full, 0},
    {"fio_rand64", "random module, reseeds", fio___rb_fio_rand64, 1},
    {"fio_rand128", "random module, reseeds", fio___rb_fio_rand128, 1},
    {"fio_rand_bytes", "random module, reseeds", fio___rb_fio_rand_bytes, 1},
    {"fio_rand_bytes_secure",
     "random module, system CSPRNG",
     fio___rb_fio_rand_secure,
     1},
    {"RANDOM128_FN 64", "core macro, no reseed", fio___rb_prng64_fill, 1},
    {"RANDOM128_FN 128", "core macro, no reseed", fio___rb_prng128_fill, 1},
    {"RANDOM128_FN bytes", "core macro, no reseed", fio___rb_prng_bytes_fill, 1},
};
#define FIO___RB_SOURCE_COUNT                                                  \
  (sizeof(fio___rb_sources) / sizeof(fio___rb_sources[0]))

/* *****************************************************************************
Performance
***************************************************************************** */

typedef struct {
  double median_ns; /* per full sample */
  double cv;        /* coefficient of variation (percent) */
} fio___rb_speed_s;

FIO_SFUNC int fio___rb_cmp_double(const void *a, const void *b) {
  double x = *(const double *)a, y = *(const double *)b;
  return (x > y) - (x < y);
}

FIO_SFUNC fio___rb_speed_s fio___rb_time(const fio___rb_source_s *s,
                                         uint64_t *w,
                                         size_t words) {
  double t[FIO___RB_RUNS - FIO___RB_WARMUP];
  for (size_t run = 0; run < FIO___RB_RUNS; ++run) {
    int64_t start = fio_time_nano();
    s->fill(w, words);
    FIO_COMPILER_GUARD;
    int64_t end = fio_time_nano();
    if (run >= FIO___RB_WARMUP)
      t[run - FIO___RB_WARMUP] = (double)(end - start);
  }
  const size_t n = FIO___RB_RUNS - FIO___RB_WARMUP;
  qsort(t, n, sizeof(t[0]), fio___rb_cmp_double);
  double mean = 0, var = 0;
  for (size_t i = 0; i < n; ++i)
    mean += t[i];
  mean /= n;
  for (size_t i = 0; i < n; ++i)
    var += (t[i] - mean) * (t[i] - mean);
  var /= (n - 1);
  fio___rb_speed_s r = {.median_ns = t[n / 2],
                        .cv = mean > 0 ? 100.0 * sqrt(var) / mean : 0};
  if (r.median_ns < 1)
    r.median_ns = 1;
  return r;
}

/* *****************************************************************************
Statistics
***************************************************************************** */

typedef enum {
  FIO___RB_M_MONOBIT,
  FIO___RB_M_POSITION,
  FIO___RB_M_HAMMING,
  FIO___RB_M_RUNS,
  FIO___RB_M_CHI8,
  FIO___RB_M_CHI16,
  FIO___RB_M_SERIAL,
  FIO___RB_M_PI,
  FIO___RB_M_COUNT
} fio___rb_metric_e;

typedef struct {
  double value[FIO___RB_M_COUNT];
  double z[FIO___RB_M_COUNT];
  double entropy;
  double worst_z;
  size_t worst_bit;
} fio___rb_stats_s;

static const char *fio___rb_metric_names[FIO___RB_M_COUNT] = {
    "bit frequency (% ones)",
    "bit positions (chi-sq.)",
    "hamming distance (bits)",
    "bit transitions (%)",
    "chi-square (8 bit)",
    "chi-square (16 bit)",
    "serial correlation",
    "Monte Carlo pi",
};
static const char *fio___rb_metric_expect[FIO___RB_M_COUNT] = {
    "50",
    "64",
    "32",
    "50",
    "255",
    "65535",
    "0",
    "3.14159",
};

static size_t fio___rb_freq16[65536];

FIO_SFUNC fio___rb_stats_s fio___rb_analyze(const uint64_t *w, size_t words) {
  fio___rb_stats_s st = {{0}};
  const double n_bytes = (double)words * 8;
  const double n_bits = n_bytes * 8;
  size_t freq8[256] = {0};
  size_t position[64] = {0};
  uint64_t ones = 0, hamming = 0, transitions = 0, pi_hits = 0;
  double sx = 0, sxx = 0, sxy = 0;
  uint8_t prev_byte = (uint8_t)(w[words - 1] >> 56); /* cyclic, as in ENT */
  FIO_MEMSET(fio___rb_freq16, 0, sizeof(fio___rb_freq16));

  for (size_t i = 0; i < words; ++i) {
    const uint64_t v = w[i];
    ones += (uint64_t)fio_popcount(v);
    for (size_t b = 0; b < 64; ++b)
      position[b] += (size_t)((v >> b) & 1);
    if (i) {
      hamming += (uint64_t)fio_hemming_dist(v, w[i - 1]);
      transitions += (w[i - 1] >> 63) ^ (v & 1); /* LSB first bit stream */
    }
    transitions += (uint64_t)fio_popcount((v ^ (v >> 1)) & (~0ULL >> 1));
    for (size_t b = 0; b < 4; ++b)
      ++fio___rb_freq16[(v >> (b << 4)) & 0xFFFF];
    for (size_t b = 0; b < 8; ++b) {
      const uint8_t c = (uint8_t)(v >> (b << 3));
      ++freq8[c];
      sx += c;
      sxx += (double)c * c;
      sxy += (double)prev_byte * c;
      prev_byte = c;
    }
    const double x = (double)(uint32_t)v, y = (double)(v >> 32);
    pi_hits += (x * x + y * y) < 18446744073709551616.0; /* 2^64 */
  }

  /* monobit */
  st.value[FIO___RB_M_MONOBIT] = 100.0 * (double)ones / n_bits;
  st.z[FIO___RB_M_MONOBIT] = ((double)ones - n_bits / 2) / sqrt(n_bits / 4);
  /* bit positions: sum of squared per-index z-scores ~ chi-square(64) */
  {
    double chi = 0, worst = -1;
    for (size_t b = 0; b < 64; ++b) {
      const double z = ((double)position[b] - (double)words / 2) /
                       sqrt((double)words / 4);
      chi += z * z;
      if (fabs(z) > worst) {
        worst = fabs(z);
        st.worst_bit = b;
      }
    }
    st.value[FIO___RB_M_POSITION] = chi;
    st.z[FIO___RB_M_POSITION] = (chi - 64) / sqrt(2.0 * 64);
  }
  /* hamming distance between consecutive words (independent pairs) */
  {
    const double pairs = (double)(words - 1);
    st.value[FIO___RB_M_HAMMING] = (double)hamming / pairs;
    st.z[FIO___RB_M_HAMMING] = ((double)hamming - 32 * pairs) / sqrt(16 * pairs);
  }
  /* runs test (bit transitions) */
  {
    const double pairs = n_bits - 1;
    st.value[FIO___RB_M_RUNS] = 100.0 * (double)transitions / pairs;
    st.z[FIO___RB_M_RUNS] =
        ((double)transitions - pairs / 2) / sqrt(pairs / 4);
  }
  /* chi-square + entropy (bytes) */
  {
    const double e = n_bytes / 256;
    double chi = 0;
    for (size_t i = 0; i < 256; ++i) {
      const double d = (double)freq8[i] - e;
      chi += d * d / e;
      if (freq8[i]) {
        const double p = (double)freq8[i] / n_bytes;
        st.entropy -= p * log2(p);
      }
    }
    st.value[FIO___RB_M_CHI8] = chi;
    st.z[FIO___RB_M_CHI8] = (chi - 255) / sqrt(2.0 * 255);
  }
  /* chi-square (16 bit values) */
  {
    const double e = (double)words * 4 / 65536;
    double chi = 0;
    for (size_t i = 0; i < 65536; ++i) {
      const double d = (double)fio___rb_freq16[i] - e;
      chi += d * d / e;
    }
    st.value[FIO___RB_M_CHI16] = chi;
    st.z[FIO___RB_M_CHI16] = (chi - 65535) / sqrt(2.0 * 65535);
  }
  /* serial correlation (lag-1 bytes, ENT formula) */
  {
    const double d = n_bytes * sxx - sx * sx;
    const double r = d > 0 ? (n_bytes * sxy - sx * sx) / d : 1;
    st.value[FIO___RB_M_SERIAL] = r;
    st.z[FIO___RB_M_SERIAL] = r * sqrt(n_bytes);
  }
  /* Monte Carlo pi */
  {
    const double p = FIO___RB_PI / 4, n = (double)words;
    st.value[FIO___RB_M_PI] = 4.0 * (double)pi_hits / n;
    st.z[FIO___RB_M_PI] = ((double)pi_hits - n * p) / sqrt(n * p * (1 - p));
  }
  for (size_t i = 0; i < FIO___RB_M_COUNT; ++i)
    if (fabs(st.z[i]) > st.worst_z)
      st.worst_z = fabs(st.z[i]);
  return st;
}

FIO_SFUNC const char *fio___rb_verdict(double z) {
  z = fabs(z);
  return z < 3 ? "ok" : z < 5 ? "weak" : "\x1B[1mBAD\x1B[0m";
}

/* *****************************************************************************
Report
***************************************************************************** */

FIO_SFUNC void fio___rb_print_source(const fio___rb_source_s *s,
                                     fio___rb_speed_s sp,
                                     const fio___rb_stats_s *st) {
  const double bytes = (double)FIO___RB_WORDS * 8;
  fprintf(stderr,
          "\n* \x1B[1m%s\x1B[0m (%s)\n"
          "\tspeed: %.3f ns / 64 bits, %.1f MB/s (median of %d, CV %.1f%%)\n"
          "\t%-26s %14s %10s %9s  %s\n",
          s->name,
          s->details,
          sp.median_ns / FIO___RB_WORDS,
          bytes * 1000.0 / sp.median_ns,
          FIO___RB_RUNS - FIO___RB_WARMUP,
          sp.cv,
          "metric",
          "value",
          "expected",
          "z-score",
          "verdict");
  for (size_t i = 0; i < FIO___RB_M_COUNT; ++i) {
    char name[64];
    if (i == FIO___RB_M_POSITION)
      snprintf(name,
               sizeof(name),
               "bit positions (worst #%zu)",
               st->worst_bit);
    else
      snprintf(name, sizeof(name), "%s", fio___rb_metric_names[i]);
    fprintf(stderr,
            "\t%-26s %14.6f %10s %9.2f  %s\n",
            name,
            st->value[i],
            fio___rb_metric_expect[i],
            st->z[i],
            fio___rb_verdict(st->z[i]));
  }
  fprintf(stderr,
          "\t%-26s %14.6f %10s %9s  %s\n",
          "entropy (bits / byte)",
          st->entropy,
          "8",
          "-",
          "info");
}

int main(void) {
  const size_t words = FIO___RB_WORDS;
  uint64_t *w = (uint64_t *)FIO_MEM_REALLOC(NULL, 0, words * sizeof(*w), 0);
  FIO_ASSERT_ALLOC(w);
  fio___rb_speed_s speed[FIO___RB_SOURCE_COUNT];
  fio___rb_stats_s stats[FIO___RB_SOURCE_COUNT];
  size_t failed = 0;

  fprintf(stderr,
          "===========================================\n"
          "Randomness Quality + Performance (%zu MiB per source)\n"
          "RAND_MAX: %llu (%zu bits per rand() call)\n"
          "===========================================\n",
          (words * sizeof(*w)) >> 20,
          (unsigned long long)RAND_MAX,
          fio___rb_c_rand_bits());
#ifdef DEBUG
  fprintf(stderr,
          "NOTE: DEBUG build - speed numbers are NOT optimized; "
          "quality metrics remain valid.\n");
#endif

  for (size_t i = 0; i < FIO___RB_SOURCE_COUNT; ++i) {
    FIO_MEMSET(w, 0, words * sizeof(*w));
    speed[i] = fio___rb_time(fio___rb_sources + i, w, words);
    stats[i] = fio___rb_analyze(w, words);
    fio___rb_print_source(fio___rb_sources + i, speed[i], stats + i);
  }

  fprintf(stderr,
          "\n===========================================\n"
          "Summary (worst |z|: < 3 ok, < 5 weak, otherwise BAD)\n"
          "===========================================\n"
          "\t%-24s %12s %12s %8s %9s  %s\n",
          "source",
          "ns / 64 bit",
          "MB/s",
          "CV %",
          "worst |z|",
          "verdict");
  for (size_t i = 0; i < FIO___RB_SOURCE_COUNT; ++i) {
    fprintf(stderr,
            "\t%-24s %12.3f %12.1f %8.1f %9.2f  %s\n",
            fio___rb_sources[i].name,
            speed[i].median_ns / words,
            (double)words * 8 * 1000.0 / speed[i].median_ns,
            speed[i].cv,
            stats[i].worst_z,
            fio___rb_verdict(stats[i].worst_z));
    if (fio___rb_sources[i].is_fio && stats[i].worst_z >= 5) {
      FIO_LOG_ERROR("%s failed randomness metrics (worst |z| %.2f)",
                    fio___rb_sources[i].name,
                    stats[i].worst_z);
      ++failed;
    }
  }
  FIO_MEM_FREE(w, words * sizeof(*w));
  return failed ? 1 : 0;
}
