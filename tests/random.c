/*
 * Original Source code copied from: http://xoshiro.di.unimi.it/hwd.php
 *
 * Copyright (C) 2004-2016 David Blackman.
 * Copyright (C) 2017-2018 David Blackman and Sebastiano Vigna.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

// I edited the original code only slightly, the original copyright still holds.

#define FIO_RAND
#define FIO_CLI
#include "fio-stl.h"

#define HWD_BITS 64
static uint64_t (*next)(void) = fio_rand64;

static uint64_t sys_next(void) {
  uint64_t r = ((uint64_t)rand() >> 8) | ((uint64_t)rand() << 40);
  r ^= (uint64_t)rand() << 20;
  return r;
}

#include <assert.h>
#include <fcntl.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef HWD_MMAP
#include <sys/mman.h>
#endif

/*
   HWD 1.1 (2018-05-24)

   This code implements the Hamming-weight dependency test based on z9
   from gjrand 4.2.1.0 and described in detail in

   David Blackman and Sebastiano Vigna, "Scrambled linear pseudorandom number
   generators", 2018.

   Please refer to the paper for details about the test.

   To compile, you must define:

   - HWD_BITS is set to 64 in this implementation.
   - HWD_BITS, which is the number of bits output by the PRNG, and it is by
     default HWD_BITS. Presently legal combinations are 32/32, 32/64,
     64/64 and 128/64.
   - Optionally HWD_DIM, which defines the length of the signatures examined
     (parameter k in the paper). Valid values are between 1 and 19;
     the default value is 8.
   - Optionally, HWD_NOPOPCOUNT, if your compiler does not support gcc's
   builtins.
   - Optionally, HWD_NUMCATS, if you want to override the default number
     of categories. Valid values are between 1 and HWD_DIM; the default value
     is HWD_DIM/2 + 1.
   - Optionally, HWD_MMAP if you want to allocate memory in huge pages using
   mmap().

   You must insert the code for your PRNG, providing a suitable next()
   method (returning a uint32_t or a uint64_t, depending on HWD_BITS)
   at the HERE comment below. You may additionally initialize his state in
   the main() if necessary.
*/

#ifndef HWD_DIM
// This must be at most 19
#define DIM (8)
#else
#define DIM (HWD_DIM)
#endif

#ifndef HWD_NUMCATS
// This must be at most DIM
#define NUMCATS (DIM / 2 + 1)
#else
#define NUMCATS (HWD_NUMCATS)
#endif

// Number of bits used for the sum in cs[] (small counters/sums).
#define SUM_BITS (19)

// Compile-time computation of 3^DIM
#define SIZE                                                                   \
  ((DIM >= 1 ? UINT64_C(3) : UINT64_C(1)) * (DIM >= 2 ? 3 : 1) *               \
   (DIM >= 3 ? 3 : 1) * (DIM >= 4 ? 3 : 1) * (DIM >= 5 ? 3 : 1) *              \
   (DIM >= 6 ? 3 : 1) * (DIM >= 7 ? 3 : 1) * (DIM >= 8 ? 3 : 1) *              \
   (DIM >= 9 ? 3 : 1) * (DIM >= 10 ? 3 : 1) * (DIM >= 11 ? 3 : 1) *            \
   (DIM >= 12 ? 3 : 1) * (DIM >= 13 ? 3 : 1) * (DIM >= 14 ? 3 : 1) *           \
   (DIM >= 15 ? 3 : 1) * (DIM >= 16 ? 3 : 1) * (DIM >= 17 ? 3 : 1) *           \
   (DIM >= 18 ? 3 : 1) * (DIM >= 19 ? 3 : 1))

// Fast division by 3; works up to DIM = 19.
#define DIV3(x) ((x)*UINT64_C(1431655766) >> 32)

// batch_size values MUST be even. P is the probability of a 1 trit.

#define P (0.46769122397215788544)
const int64_t batch_size[] = {-1,
                              UINT64_C(14744),
                              UINT64_C(28320),
                              UINT64_C(56616),
                              UINT64_C(116264),
                              UINT64_C(242784),
                              UINT64_C(512040),
                              UINT64_C(1086096),
                              UINT64_C(2311072),
                              UINT64_C(4926224),
                              UINT64_C(10510376),
                              UINT64_C(22435504),
                              UINT64_C(47903280),
                              UINT64_C(102294608),
                              UINT64_C(218459240),
                              UINT64_C(466556056),
                              UINT64_C(996427288),
                              UINT64_C(2128099936),
                              UINT64_C(4545075936),
                              UINT64_C(9707156552)};

#ifdef HWD_NO_POPCOUNT
static inline int popcount64(uint64_t x) {
  x = x - ((x >> 1) & 0x5555555555555555);
  x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
  x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
  x = x + (x >> 8);
  x = x + (x >> 16);
  x = x + (x >> 32);
  return x & 0x7f;
}
#else
#define popcount64(x) __builtin_popcountll(x)
#endif

/* Probability that the smallest of n numbers in [0..1) is <= x . */
static double pco_scale(double x, double n) {
  if (x >= 1.0 || x <= 0.0)
    return x;

  /* This is the result we want: return 1.0 - pow(1.0 - x, n); except the
     important cases are with x very small so this method gives better
     accuracy. */

  return -expm1(log1p(-x) * n);
}

/* The idea of the test is based around Hamming weights. We calculate the
   average number of bits per BITS-bit word and how it depends on the
   weights of the previous DIM words. There are SIZE different categories
   for the previous words. For each one accumulate number of samples
   (get_count(cs[j]) and count_sum[j].c) and number of bits per sample
   (get_sum(cs[j]) and count_sum[j].s) .

   To increase cache hits, we pack a 13-bit unsigned counter (upper bits)
   and a and a 19-bit unsigned sum of Hamming weights (lower bits) into a
   uint32_t. It would make sense to use bitfields, but in this way
   update_cs() can update both fields with a single sum. */

static inline int get_count(uint32_t cs) { return cs >> SUM_BITS; }

static inline int get_sum(uint32_t cs) { return cs & ((1 << SUM_BITS) - 1); }

/* We add bc to the sum field of *p then add 1 to the count field. */
static inline void update_cs(int bc, uint32_t *p) {
  *p += bc + (1 << SUM_BITS);
}

#ifdef HWD_MMAP
// "Small" counters/sums
static uint32_t *cs;

// "Large" counters/sums
static struct {
  uint64_t c;
  int64_t s;
} * count_sum;
#else
// "Small" counters/sums
static uint32_t cs[SIZE];

// "Large" counters/sums
static struct {
  uint64_t c;
  int64_t s;
} count_sum[SIZE];
#endif

/* Copy accumulated numbers out of cs[] into count_sum, then zero the ones
   in cs[]. Note it is impossible for totals to overflow unless counts do. */

static void desat(const int64_t next_batch_size) {
  int64_t c = 0;

  for (uint64_t i = 0; i < SIZE; i++) {
    const int32_t st = cs[i];
    const int count = get_count(st);

    c += count;

    count_sum[i].c += count;
    /* In cs[] the total Hamming weight is stored as actual weight. In
       count_sum, it is stored as difference from expected average
       Hamming weight, hence (BITS/2) * ct */
    count_sum[i].s += get_sum(st) - (HWD_BITS / 2) * count;
    cs[i] = 0;
  }

  if (c != next_batch_size) {
    fprintf(stderr, "Counters overflowed. Seriously non-random.\n");
    printf("p = %.3g\n", 1e-100);
    exit(0);
  }
}

/* sig is the last signature from the previous call. At each step it
   contains an index into cs[], derived from the Hamming weights of the
   previous DIM numbers. Considered as a base 3 number, the most
   significant digit is the most recent trit. n is the batch size. */
static inline uint32_t scan_batch(uint32_t sig, int64_t n, uint64_t *ts) {
  uint64_t t = ts ? *ts : 0;
  int bc;

  for (int64_t i = 0; i < n; i++) {
    const uint64_t w = next();

    if (ts) {
      bc = popcount64(w ^ w << 1 ^ t);
      t = w >> 63;
    } else
      bc = popcount64(w);

    update_cs(bc, cs + sig);
    sig = DIV3(sig) + ((bc >= 30) + (bc >= 35)) * (SIZE / 3);
  }

  if (ts)
    *ts = t;
  /* return the current signature so it can be passed back in on the next batch
   */
  return sig;
}

/* Now we're out of the the accumulate phase, which is the inside loop.
   Next is analysis. */

/* Mostly a debugging printf, though it can tell you a bit about the
   structure of a prng when it fails. Print sig out in base 3, least
   significant digits first. This means the most recent trit is the
   rightmost. */

static void print_sig(uint32_t sig) {
  for (uint64_t i = DIM; i > 0; i--) {
    putchar(sig % 3 + '0');
    sig /= 3;
  }
}

#ifndef M_SQRT1_2
/* 1.0/sqrt(2.0) */
#define M_SQRT1_2 0.70710678118654752438
#endif
/* 1.0/sqrt(3.0) */
#define CORRECT3 0.57735026918962576451
/* 1.0/sqrt(6.0) */
#define CORRECT6 0.40824829046386301636

/* This is a transform similar in spirit to the Walsh-Hadamard transform
  (see the paper). It's ortho-normal. So with independent normal
  distribution mean 0 standard deviation 1 in, we get independent normal
  distribution mean 0 standard deviation 1 out, except maybe for element 0.
  And of course, for certain kinds of bad prngs when the null hypthosis is
  false, some of these numbers will get extreme. */

static void mix3(double *ct, int sig) {
  double *p1 = ct + sig, *p2 = p1 + sig;
  double a, b, c;

  for (int i = 0; i < sig; i++) {
    a = ct[i];
    b = p1[i];
    c = p2[i];
    ct[i] = (a + b + c) * CORRECT3;
    p1[i] = (a - c) * M_SQRT1_2;
    p2[i] = (2 * b - a - c) * CORRECT6;
  }

  sig = DIV3(sig);
  if (sig) {
    mix3(ct, sig);
    mix3(p1, sig);
    mix3(p2, sig);
  }
}

/* categorise sig based on nonzero ternary digits. */
static int cat(uint32_t sig) {
  int r = 0;

  while (sig) {
    r += (sig % 3) != 0;
    sig /= 3;
  }

  return (r >= NUMCATS ? NUMCATS : r) - 1;
}

/* Apply the transform; then, compute, log and return the resulting p-value. */

#ifdef HWD_MMAP
static double *norm;
#else
static double norm[SIZE]; // This might be large
#endif

static double compute_pvalue(const bool trans) {
  const double db = HWD_BITS * 0.25;

  for (uint64_t i = 0; i < SIZE; i++) {
    /* copy the bit count totals from count_sum[i].s to norm[i] with
       normalisation. We expect mean 0 standard deviation 1 db is the
       expected variance for Hamming weight of BITS-bit words.
       count_sum[i].c is number of samples */
    if (count_sum[i].c == 0)
      norm[i] = 0.0;
    else
      norm[i] = count_sum[i].s / sqrt(count_sum[i].c * db);
  }

  /* The transform. The wonderful transform. After this we expect still
     normalised to mean 0 stdev 1 under the null hypothesis. (But not for
     element 0 which we will ignore.) */
  mix3(norm, SIZE / 3);

  double overall_pvalue = DBL_MAX;

  /* To make the test more sensitive (see the paper) we split the
     elements of norm into NUMCAT categories. These are based only on the
     index into norm, not the content. We go though norm[], decide which
     category each one is in, and record the signature (sig[]) and the
     absolute value (sigma[]) For the most extreme value in each
     category. Also a count (cat_count[]) of how many were in each
     category. */

  double sigma[NUMCATS];
  uint32_t sig[NUMCATS], cat_count[NUMCATS] = {0};
  for (int i = 0; i < NUMCATS; i++)
    sigma[i] = DBL_MIN;

  for (uint64_t i = 1; i < SIZE; i++) {
    const int c = cat(i);
    cat_count[c]++;
    const double x = fabs(norm[i]);
    if (x > sigma[c]) {
      sig[c] = i;
      sigma[c] = x;
    }
  }

  /* For each category, calculate a p-value, put the lowest into
     overall_pvalue, and print something out. */
  for (int i = 0; i < NUMCATS; i++) {
    printf("mix3 extreme = %.5f (sig = ", sigma[i]);
    print_sig(sig[i]);
    /* convert absolute value of approximate normal into p-value. */
    double pvalue = erfc(M_SQRT1_2 * sigma[i]);
    /* Ok, that's the lowest p-value cherry picked out of a choice of
       cat_count[i] of them. Must correct for that. */
    pvalue = pco_scale(pvalue, cat_count[i]);
    printf(") weight %s%d (%" PRIu32 "), p-value = %.3g\n",
           i == NUMCATS - 1 ? ">=" : "",
           i + 1,
           cat_count[i],
           pvalue);
    if (pvalue < overall_pvalue)
      overall_pvalue = pvalue;
  }

  printf("bits per word = %d (analyzing %s); min category p-value = %.3g\n\n",
         HWD_BITS,
         trans ? "transitions" : "bits",
         overall_pvalue);
  /* again, we're cherry picking worst of NUMCATS, so correct it again. */
  return pco_scale(overall_pvalue, NUMCATS);
}

static time_t tstart;
static double low_pvalue = DBL_MIN;

/* This is the call made when we want to print some analysis. This will be
   done multiple times if --progress is used. */
static void analyze(int64_t pos, bool trans, bool final) {

  if (pos < 2 * pow(2.0 / (1.0 - P), DIM))
    printf("WARNING: p-values are unreliable, you have to wait (insufficient "
           "data for meaningful answer)\n");

  const double pvalue = compute_pvalue(trans);
  const time_t tm = time(0);

  printf("processed %.3g bytes in %.3g seconds (%.4g GB/s, %.4g TB/h). %s\n",
         (double)pos,
         (double)(tm - tstart),
         pos * 1E-9 / (double)(tm - tstart),
         pos * (3600 * 1E-12) / (double)(tm - tstart),
         ctime(&tm));

  if (final)
    printf("final\n");
  printf("p = %.3g\n", pvalue);

  if (pvalue < low_pvalue)
    exit(0);

  if (!final)
    printf("------\n\n");
}

static int64_t progsize[] = {100000000,
                             125000000,
                             150000000,
                             175000000,
                             200000000,
                             250000000,
                             300000000,
                             400000000,
                             500000000,
                             600000000,
                             700000000,
                             850000000,
                             0};

/* We use the all-one signature (the most probable) as initial signature. */
static int64_t pos;
static uint32_t last_sig = (SIZE - 1) / 2;
static uint64_t ts;
static int64_t next_progr = 100000000; // progsize[0]
static int progr_index;

static void run_test(const int64_t n, const bool trans, const bool progress) {

  uint64_t *const p = trans ? &ts : NULL;

  while (n < 0 || pos < n) {
    int64_t next_batch_size = batch_size[DIM];
    if (n >= 0 && (n - pos) / (HWD_BITS / 8) < next_batch_size)
      next_batch_size = (n - pos) / (HWD_BITS / 8) & ~UINT64_C(7);

    if (next_batch_size == 0)
      break;
    last_sig = scan_batch(last_sig, next_batch_size, p);
    desat(next_batch_size);
    pos += next_batch_size * (HWD_BITS / 8);

    if (progress && pos >= next_progr) {
      analyze(pos, trans, false);
      progsize[progr_index++] *= 10;
      next_progr = progsize[progr_index];
      if (next_progr == 0) {
        progr_index = 0;
        next_progr = progsize[0];
      }
    }
  }

  analyze(pos, trans, true);
}

int main(int argc, const char **argv) {
  int64_t n = -1;
  bool trans = false, progress = false;

  fio_cli_start(
      argc,
      argv,
      0,
      1,
      "implements the Hamming-weight dependency test based on z9 from gjrand "
      "4.2.1.0 and described by David Blackman and Sebastiano Vigna.\n"
      "Runs the test, by default, on the facil.io random function.",
      FIO_CLI_BOOL("--system -s tests a patched  `rand` instead of facil.io's "
                   "`fio_rand64`."),
      FIO_CLI_BOOL("--progress -p uses progressive test sizes (true unless N "
                   "is provided)."),
      FIO_CLI_BOOL("--trans -t tests transitions (vs. bits)."),
      FIO_CLI_STRING("--low-pv a float indicating the test's low P value."),
      FIO_CLI_INT("--N -n the tests iteration size limit."));

  if (fio_cli_get_bool("-p"))
    progress = true;
  if (fio_cli_get_bool("-t"))
    trans = true;
  if (fio_cli_get_i("-n"))
    n = fio_cli_get_i("-n");
  if (fio_cli_get("--low-pv")) {
    if (fio_cli_get("--low-pv") &&
        sscanf(fio_cli_get("--low-pv"), "%lf", &low_pvalue) != 1)
      fprintf(stderr, "Optional --low-pv must be a float.\n");
    exit(1);
  }
  if (fio_cli_get_bool("-s")) {
    fprintf(
        stderr,
        "Testing a fixed variation of system's `rand` instead of facil.io.\n");
    sys_next();
    next = sys_next;
  }
  fio_cli_end();

#ifdef HWD_MMAP
  fprintf(stderr, "Allocating memory via mmap()... ");
  // (SIZE + 1) is necessary for a correct memory alignment.
  cs = mmap((void *)(0x0UL),
            (SIZE + 1) * sizeof *cs + SIZE * sizeof *norm +
                SIZE * sizeof *count_sum,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (30 << MAP_HUGE_SHIFT),
            0,
            0);
  if (cs == MAP_FAILED) {
    fprintf(stderr, "Failed.\n");
    exit(1);
  }
  fprintf(stderr, "OK.\n");
  norm = (void *)(cs + SIZE + 1);
  count_sum = (void *)(norm + SIZE);
#endif

  tstart = time(0);

  // for (int i = 1; i < argc; i++) {
  //   double dn;
  //   if (strcmp(argv[i], "--progress") == 0)
  //     progress = true;
  //   else if (strcmp(argv[i], "-t") == 0)
  //     trans = true;
  //   else if (sscanf(argv[i], "%lf", &dn) == 1)
  //     n = (int64_t)dn;
  //   else if (sscanf(argv[i], "--low-pv=%lf", &low_pvalue) == 1) {
  //   } else {
  //     fprintf(stderr,
  //             "Optional arg must be --progress or -t or "
  //             "--low-pv=number or numeric\n");
  //     exit(1);
  //   }
  // }

  if (n <= 0)
    progress = true;

  run_test(n, trans, progress);

  exit(0);
}
