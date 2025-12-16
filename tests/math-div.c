/* *****************************************************************************
 * Multi-precision Division Tests for fio_math_div
 *
 * Tests the fio_math_div function for correctness and performance.
 * Verifies: quotient * divisor + remainder = dividend
 * ***************************************************************************/
#define FIO_CORE
#define FIO_MATH
#include "../fio-stl/include.h"

#include <time.h>

/* Test result tracking */
static size_t tests_passed = 0;
static size_t tests_failed = 0;

/* Helper: print a multi-precision number in hex (big-endian display) */
static void print_mp(const char *name, const uint64_t *n, size_t len) {
  fprintf(stderr, "%s = 0x", name);
  for (size_t i = len; i > 0; --i) {
    fprintf(stderr, "%016llx", (unsigned long long)n[i - 1]);
  }
  fprintf(stderr, "\n");
}

/* Helper: check if a multi-precision number is zero */
static int mp_is_zero(const uint64_t *n, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (n[i] != 0)
      return 0;
  }
  return 1;
}

/* Helper: compare two multi-precision numbers, returns -1, 0, or 1 */
static int mp_cmp(const uint64_t *a, const uint64_t *b, size_t len) {
  for (size_t i = len; i > 0; --i) {
    if (a[i - 1] < b[i - 1])
      return -1;
    if (a[i - 1] > b[i - 1])
      return 1;
  }
  return 0;
}

/* Helper: verify quotient * divisor + remainder = dividend */
static int verify_division(const uint64_t *dividend,
                           const uint64_t *divisor,
                           const uint64_t *quotient,
                           const uint64_t *remainder,
                           size_t len) {
  /* Allocate space for q * d (needs 2*len words) */
  uint64_t product[64] = {0}; /* max 2048-bit for our tests */
  uint64_t result[32] = {0};

  if (len > 32) {
    fprintf(stderr, "ERROR: verify_division len too large\n");
    return 0;
  }

  /* product = quotient * divisor */
  fio_math_mul(product, quotient, divisor, len);

  /* result = product[0..len-1] + remainder (ignore high words of product) */
  for (size_t i = 0; i < len; ++i) {
    result[i] = product[i];
  }
  (void)fio_math_add(result, result, remainder, len);

  /* Check if result == dividend */
  if (mp_cmp(result, dividend, len) != 0) {
    fprintf(stderr, "VERIFICATION FAILED:\n");
    print_mp("  dividend ", dividend, len);
    print_mp("  divisor  ", divisor, len);
    print_mp("  quotient ", quotient, len);
    print_mp("  remainder", remainder, len);
    print_mp("  q*d      ", product, len);
    print_mp("  q*d+r    ", result, len);
    return 0;
  }

  /* Also verify remainder < divisor */
  if (mp_cmp(remainder, divisor, len) >= 0) {
    fprintf(stderr, "VERIFICATION FAILED: remainder >= divisor\n");
    print_mp("  remainder", remainder, len);
    print_mp("  divisor  ", divisor, len);
    return 0;
  }

  return 1;
}

/* Test macro */
#define TEST(name, cond)                                                       \
  do {                                                                         \
    if (cond) {                                                                \
      tests_passed++;                                                          \
      fprintf(stderr, "    PASS: %s\n", name);                                 \
    } else {                                                                   \
      tests_failed++;                                                          \
      fprintf(stderr, "*** FAIL: %s\n", name);                                 \
    }                                                                          \
  } while (0)

/* *****************************************************************************
 * Test Cases
 * ***************************************************************************/

/* Test 1: Simple division 100 / 7 = 14 remainder 2 */
static void test_simple_division(void) {
  fprintf(stderr, "  Testing simple division (100 / 7)...\n");
  uint64_t a[4] = {100, 0, 0, 0};
  uint64_t b[4] = {7, 0, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("100 / 7 = 14", q[0] == 14 && q[1] == 0 && q[2] == 0 && q[3] == 0);
  TEST("100 % 7 = 2", r[0] == 2 && r[1] == 0 && r[2] == 0 && r[3] == 0);
  TEST("100 / 7 verification", verify_division(a, b, q, r, 4));
}

/* Test 2: Division by 1 */
static void test_division_by_one(void) {
  fprintf(stderr, "  Testing division by 1...\n");
  uint64_t a[4] = {0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL, 0, 0};
  uint64_t b[4] = {1, 0, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("n / 1 = n (quotient)",
       q[0] == a[0] && q[1] == a[1] && q[2] == 0 && q[3] == 0);
  TEST("n % 1 = 0 (remainder)", mp_is_zero(r, 4));
  TEST("n / 1 verification", verify_division(a, b, q, r, 4));
}

/* Test 3: Division by self */
static void test_division_by_self(void) {
  fprintf(stderr, "  Testing division by self...\n");
  uint64_t a[4] = {0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL, 0, 0};
  uint64_t b[4] = {0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("n / n = 1", q[0] == 1 && q[1] == 0 && q[2] == 0 && q[3] == 0);
  TEST("n % n = 0", mp_is_zero(r, 4));
  TEST("n / n verification", verify_division(a, b, q, r, 4));
}

/* Test 4: Dividend < Divisor */
static void test_dividend_less_than_divisor(void) {
  fprintf(stderr, "  Testing dividend < divisor...\n");
  uint64_t a[4] = {42, 0, 0, 0};
  uint64_t b[4] = {100, 0, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("small / large = 0", mp_is_zero(q, 4));
  TEST("small % large = small",
       r[0] == 42 && r[1] == 0 && r[2] == 0 && r[3] == 0);
  TEST("small / large verification", verify_division(a, b, q, r, 4));
}

/* Test 5: Large numbers - 2^192 / 2^64 */
static void test_large_power_of_two(void) {
  fprintf(stderr, "  Testing large power of two division...\n");
  uint64_t a[4] = {0, 0, 0, 1}; /* 2^192 */
  uint64_t b[4] = {0, 1, 0, 0}; /* 2^64 */
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  /* 2^192 / 2^64 = 2^128 */
  TEST("2^192 / 2^64 = 2^128",
       q[0] == 0 && q[1] == 0 && q[2] == 1 && q[3] == 0);
  TEST("2^192 % 2^64 = 0", mp_is_zero(r, 4));
  TEST("2^192 / 2^64 verification", verify_division(a, b, q, r, 4));
}

/* Test 6: Division by power of 2 */
static void test_division_by_power_of_two(void) {
  fprintf(stderr, "  Testing division by power of 2...\n");
  uint64_t a[4] = {0x8000000000000000ULL, 0x1234567890ABCDEFULL, 0, 0};
  uint64_t b[4] = {0x100, 0, 0, 0}; /* 256 = 2^8 */
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  /* Verify using the invariant */
  TEST("division by 256 verification", verify_division(a, b, q, r, 4));
}

/* Test 7: Edge case - divisor with high bit set */
static void test_high_bit_divisor(void) {
  fprintf(stderr, "  Testing high bit divisor...\n");
  uint64_t a[4] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0, 0};
  uint64_t b[4] = {0x8000000000000000ULL, 0, 0, 0}; /* 2^63 */
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("high bit divisor verification", verify_division(a, b, q, r, 4));
}

/* Test 8: Single word operands */
static void test_single_word(void) {
  fprintf(stderr, "  Testing single word operands...\n");
  uint64_t a[1] = {1000000007ULL};
  uint64_t b[1] = {12345ULL};
  uint64_t q[1] = {0};
  uint64_t r[1] = {0};

  fio_math_div(q, r, a, b, 1);

  uint64_t expected_q = 1000000007ULL / 12345ULL;
  uint64_t expected_r = 1000000007ULL % 12345ULL;

  TEST("single word quotient", q[0] == expected_q);
  TEST("single word remainder", r[0] == expected_r);
  TEST("single word verification", verify_division(a, b, q, r, 1));
}

/* Test 9: Two word operands */
static void test_two_word(void) {
  fprintf(stderr, "  Testing two word operands...\n");
  uint64_t a[2] = {0xFFFFFFFFFFFFFFFFULL, 0x00000000FFFFFFFFULL};
  uint64_t b[2] = {0x100000000ULL, 0}; /* 2^32 */
  uint64_t q[2] = {0};
  uint64_t r[2] = {0};

  fio_math_div(q, r, a, b, 2);

  TEST("two word verification", verify_division(a, b, q, r, 2));
}

/* Test 10: Random test vectors */
static void test_random_vectors(void) {
  fprintf(stderr, "  Testing random vectors (100 iterations)...\n");
  /* Use a simple PRNG for reproducibility */
  uint64_t seed = 0x123456789ABCDEF0ULL;
#define NEXT_RAND()                                                            \
  (seed = seed * 6364136223846793005ULL + 1442695040888963407ULL)

  int all_passed = 1;
  for (int test = 0; test < 100; ++test) {
    uint64_t a[4], b[4], q[4], r[4];

    /* Generate random dividend */
    for (int i = 0; i < 4; ++i)
      a[i] = NEXT_RAND();

    /* Generate random non-zero divisor */
    do {
      for (int i = 0; i < 4; ++i)
        b[i] = NEXT_RAND();
    } while (mp_is_zero(b, 4));

    fio_math_div(q, r, a, b, 4);

    if (!verify_division(a, b, q, r, 4)) {
      fprintf(stderr, "*** FAIL: random test %d\n", test);
      all_passed = 0;
      tests_failed++;
    }
  }
  if (all_passed) {
    fprintf(stderr, "    PASS: all 100 random tests\n");
    tests_passed++;
  }
#undef NEXT_RAND
}

/* Test 11: 256-bit numbers (crypto relevant) */
static void test_256bit(void) {
  fprintf(stderr, "  Testing 256-bit numbers...\n");
  /* P-256 prime: 2^256 - 2^224 + 2^192 + 2^96 - 1 */
  uint64_t p256[4] = {0xFFFFFFFFFFFFFFFFULL,
                      0x00000000FFFFFFFFULL,
                      0x0000000000000000ULL,
                      0xFFFFFFFF00000001ULL};

  /* Some value larger than p256 */
  uint64_t a[4] = {0xFFFFFFFFFFFFFFFFULL,
                   0xFFFFFFFFFFFFFFFFULL,
                   0xFFFFFFFFFFFFFFFFULL,
                   0xFFFFFFFFFFFFFFFFULL};

  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, p256, 4);

  TEST("256-bit p256 verification", verify_division(a, p256, q, r, 4));
}

/* Test 12: 512-bit numbers */
static void test_512bit(void) {
  fprintf(stderr, "  Testing 512-bit numbers...\n");
  uint64_t a[8] = {0xFFFFFFFFFFFFFFFFULL,
                   0xFFFFFFFFFFFFFFFFULL,
                   0xFFFFFFFFFFFFFFFFULL,
                   0xFFFFFFFFFFFFFFFFULL,
                   0x1234567890ABCDEFULL,
                   0xFEDCBA0987654321ULL,
                   0xDEADBEEFCAFEBABEULL,
                   0x0123456789ABCDEFULL};

  uint64_t b[8] =
      {0x1000000000000000ULL, 0x0000000000000001ULL, 0, 0, 0, 0, 0, 0};

  uint64_t q[8] = {0};
  uint64_t r[8] = {0};

  fio_math_div(q, r, a, b, 8);

  /* Verify using extended arrays for multiplication */
  uint64_t product[16] = {0};
  uint64_t result[8] = {0};

  fio_math_mul(product, q, b, 8);
  for (size_t i = 0; i < 8; ++i)
    result[i] = product[i];
  (void)fio_math_add(result, result, r, 8);

  TEST("512-bit verification", mp_cmp(result, a, 8) == 0);
  TEST("512-bit remainder < divisor", mp_cmp(r, b, 8) < 0);
}

/* Test 13: Edge case - dividend equals divisor minus 1 */
static void test_dividend_one_less(void) {
  fprintf(stderr, "  Testing dividend = divisor - 1...\n");
  uint64_t a[4] = {99, 0, 0, 0};
  uint64_t b[4] = {100, 0, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("99 / 100 = 0", mp_is_zero(q, 4));
  TEST("99 % 100 = 99", r[0] == 99 && mp_is_zero(r + 1, 3));
  TEST("99 / 100 verification", verify_division(a, b, q, r, 4));
}

/* Test 14: Edge case - dividend equals divisor plus 1 */
static void test_dividend_one_more(void) {
  fprintf(stderr, "  Testing dividend = divisor + 1...\n");
  uint64_t a[4] = {101, 0, 0, 0};
  uint64_t b[4] = {100, 0, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("101 / 100 = 1", q[0] == 1 && mp_is_zero(q + 1, 3));
  TEST("101 % 100 = 1", r[0] == 1 && mp_is_zero(r + 1, 3));
  TEST("101 / 100 verification", verify_division(a, b, q, r, 4));
}

/* Test 15: NULL dest pointer (only compute remainder) */
static void test_null_dest(void) {
  fprintf(stderr, "  Testing NULL dest pointer...\n");
  uint64_t a[4] = {100, 0, 0, 0};
  uint64_t b[4] = {7, 0, 0, 0};
  uint64_t r[4] = {0};

  fio_math_div(NULL, r, a, b, 4);

  TEST("NULL dest: 100 % 7 = 2", r[0] == 2 && mp_is_zero(r + 1, 3));
}

/* Test 16: NULL remainder pointer (only compute quotient) */
static void test_null_remainder(void) {
  fprintf(stderr, "  Testing NULL remainder pointer...\n");
  uint64_t a[4] = {100, 0, 0, 0};
  uint64_t b[4] = {7, 0, 0, 0};
  uint64_t q[4] = {0};

  fio_math_div(q, NULL, a, b, 4);

  TEST("NULL remainder: 100 / 7 = 14", q[0] == 14 && mp_is_zero(q + 1, 3));
}

/* Test 17: Multi-word divisor with leading zeros in high words */
static void test_leading_zeros(void) {
  fprintf(stderr, "  Testing leading zeros...\n");
  uint64_t a[4] = {0, 0, 0x100, 0};
  uint64_t b[4] = {0, 0x10, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("leading zeros verification", verify_division(a, b, q, r, 4));
}

/* Test 18: Maximum values */
static void test_max_values(void) {
  fprintf(stderr, "  Testing maximum values...\n");
  uint64_t a[4] = {~0ULL, ~0ULL, ~0ULL, ~0ULL};
  uint64_t b[4] = {~0ULL, ~0ULL, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("max values verification", verify_division(a, b, q, r, 4));
}

/* Test 19: Edge case - same MSB position but different values */
static void test_same_msb_different_values(void) {
  fprintf(stderr, "  Testing same MSB position but different values...\n");
  /* Both have MSB at position 127 (bit 63 of word 1) */
  uint64_t a[4] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0, 0};
  uint64_t b[4] = {0x0000000000000001ULL, 0x8000000000000000ULL, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("same MSB verification", verify_division(a, b, q, r, 4));
}

/* Test 20: Regression test - potential off-by-one in quotient estimation */
static void test_quotient_estimation(void) {
  fprintf(stderr, "  Testing quotient estimation edge cases...\n");
  /* This tests the case where initial quotient estimate might be off */
  uint64_t a[4] = {0, 0x8000000000000000ULL, 0, 0};
  uint64_t b[4] = {1, 0x4000000000000000ULL, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("quotient estimation verification", verify_division(a, b, q, r, 4));
}

/* *****************************************************************************
 * Performance Benchmarks
 * ***************************************************************************/

static uint64_t get_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void benchmark_division(const char *name,
                               size_t len,
                               size_t iterations) {
  uint64_t a[32] = {0};
  uint64_t b[32] = {0};
  uint64_t q[32] = {0};
  uint64_t r[32] = {0};

  /* Initialize with some values */
  for (size_t i = 0; i < len; ++i) {
    a[i] = 0xFFFFFFFFFFFFFFFFULL - i;
    b[i] = (i == 0) ? 0x123456789ABCDEF0ULL : (i < len / 2 ? 0 : i);
  }
  /* Ensure divisor is non-zero */
  if (mp_is_zero(b, len))
    b[0] = 1;

  uint64_t start = get_time_ns();
  for (size_t i = 0; i < iterations; ++i) {
    fio_math_div(q, r, a, b, len);
    /* Prevent optimization */
    a[0] ^= q[0];
  }
  uint64_t end = get_time_ns();

  double ns_per_op = (double)(end - start) / (double)iterations;
  double ops_per_sec = 1e9 / ns_per_op;

  fprintf(stderr,
          "    %s: %.2f ns/op (%.2f M ops/sec)\n",
          name,
          ns_per_op,
          ops_per_sec / 1e6);
}

static void run_benchmarks(void) {
  fprintf(stderr, "\n=== Division Benchmarks ===\n");

  benchmark_division("256-bit / 128-bit (4 words)", 4, 100000);
  benchmark_division("512-bit / 256-bit (8 words)", 8, 50000);
  benchmark_division("1024-bit / 512-bit (16 words)", 16, 10000);
  benchmark_division("2048-bit / 1024-bit (32 words)", 32, 5000);
}

/* *****************************************************************************
 * Main
 * ***************************************************************************/

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  fprintf(stderr, "=== fio_math_div Test Suite ===\n\n");

  /* Run all tests */
  test_simple_division();
  test_division_by_one();
  test_division_by_self();
  test_dividend_less_than_divisor();
  test_large_power_of_two();
  test_division_by_power_of_two();
  test_high_bit_divisor();
  test_single_word();
  test_two_word();
  test_random_vectors();
  test_256bit();
  test_512bit();
  test_dividend_one_less();
  test_dividend_one_more();
  test_null_dest();
  test_null_remainder();
  test_leading_zeros();
  test_max_values();
  test_same_msb_different_values();
  test_quotient_estimation();

  fprintf(stderr, "\n=== Test Results ===\n");
  fprintf(stderr, "Passed: %zu\n", tests_passed);
  fprintf(stderr, "Failed: %zu\n", tests_failed);

  /* Run benchmarks if all tests pass */
  if (tests_failed == 0) {
    run_benchmarks();
  }

  return tests_failed > 0 ? 1 : 0;
}
