/* *****************************************************************************
Test - Math Module
Merged correctness coverage from math-div, math-helpers, karatsuba, and
vector-math while omitting their old benchmark sections.
***************************************************************************** */
#define FIO_MATH
#include "test-helpers.h"
#include <time.h>

/* --- ./tests-old/math-div.c --- */

#include <time.h>

/* Test result tracking */
static size_t tests_passed = 0;
static size_t tests_failed = 0;

/* Helper: print a multi-precision number in hex (big-endian display) - for
 * errors */
static void print_mp(const char *name, const uint64_t *n, size_t len) {
  char buf[1024];
  char *p = buf;
  p += snprintf(p, sizeof(buf), "%s = 0x", name);
  for (size_t i = len; i > 0 && (size_t)(p - buf) < sizeof(buf) - 20; --i) {
    p += snprintf(p,
                  sizeof(buf) - (size_t)(p - buf),
                  "%016llx",
                  (unsigned long long)n[i - 1]);
  }
  FIO_LOG_ERROR("%s", buf);
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
    FIO_LOG_ERROR("ERROR: verify_division len too large");
    return 0;
  }

  /* product = quotient * divisor */
  fio_math_mul(product, quotient, divisor, len);

  /* result = product[0..len-1] + remainder (ignore high words of product) */
  for (size_t i = 0; i < len; ++i) {
    result[i] = product[i];
  }
  /* Intentionally ignore return value - we only care about the sum */
  if (fio_math_add(result, result, remainder, len)) { /* overflow is ok here */
  }

  /* Check if result == dividend */
  if (mp_cmp(result, dividend, len) != 0) {
    FIO_LOG_ERROR("VERIFICATION FAILED:");
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
    FIO_LOG_ERROR("VERIFICATION FAILED: remainder >= divisor");
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
      FIO_LOG_DDEBUG("    PASS: %s", name);                                    \
    } else {                                                                   \
      tests_failed++;                                                          \
      FIO_LOG_ERROR("*** FAIL: %s", name);                                     \
    }                                                                          \
  } while (0)

/* *****************************************************************************
 * Test Cases
 * ***************************************************************************/

/* Test 1: Simple division 100 / 7 = 14 remainder 2 */
static void test_simple_division(void) {
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
  uint64_t a[4] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0, 0};
  uint64_t b[4] = {0x8000000000000000ULL, 0, 0, 0}; /* 2^63 */
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("high bit divisor verification", verify_division(a, b, q, r, 4));
}

/* Test 8: Single word operands */
static void test_single_word(void) {
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
  uint64_t a[2] = {0xFFFFFFFFFFFFFFFFULL, 0x00000000FFFFFFFFULL};
  uint64_t b[2] = {0x100000000ULL, 0}; /* 2^32 */
  uint64_t q[2] = {0};
  uint64_t r[2] = {0};

  fio_math_div(q, r, a, b, 2);

  TEST("two word verification", verify_division(a, b, q, r, 2));
}

/* Test 10: Random test vectors */
static void test_random_vectors(void) {
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
      FIO_LOG_ERROR("*** FAIL: random test %d", test);
      all_passed = 0;
      tests_failed++;
    }
  }
  if (all_passed) {
    tests_passed++;
  }
#undef NEXT_RAND
}

/* Test 11: 256-bit numbers (crypto relevant) */
static void test_256bit(void) {
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
  /* Intentionally ignore return value - we only care about the sum */
  if (fio_math_add(result, result, r, 8)) { /* overflow is ok here */
  }

  TEST("512-bit verification", mp_cmp(result, a, 8) == 0);
  TEST("512-bit remainder < divisor", mp_cmp(r, b, 8) < 0);
}

/* Test 13: Edge case - dividend equals divisor minus 1 */
static void test_dividend_one_less(void) {
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
  uint64_t a[4] = {100, 0, 0, 0};
  uint64_t b[4] = {7, 0, 0, 0};
  uint64_t r[4] = {0};

  fio_math_div(NULL, r, a, b, 4);

  TEST("NULL dest: 100 % 7 = 2", r[0] == 2 && mp_is_zero(r + 1, 3));
}

/* Test 16: NULL remainder pointer (only compute quotient) */
static void test_null_remainder(void) {
  uint64_t a[4] = {100, 0, 0, 0};
  uint64_t b[4] = {7, 0, 0, 0};
  uint64_t q[4] = {0};

  fio_math_div(q, NULL, a, b, 4);

  TEST("NULL remainder: 100 / 7 = 14", q[0] == 14 && mp_is_zero(q + 1, 3));
}

/* Test 17: Multi-word divisor with leading zeros in high words */
static void test_leading_zeros(void) {
  uint64_t a[4] = {0, 0, 0x100, 0};
  uint64_t b[4] = {0, 0x10, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("leading zeros verification", verify_division(a, b, q, r, 4));
}

/* Test 18: Maximum values */
static void test_max_values(void) {
  uint64_t a[4] = {~0ULL, ~0ULL, ~0ULL, ~0ULL};
  uint64_t b[4] = {~0ULL, ~0ULL, 0, 0};
  uint64_t q[4] = {0};
  uint64_t r[4] = {0};

  fio_math_div(q, r, a, b, 4);

  TEST("max values verification", verify_division(a, b, q, r, 4));
}

/* Test 19: Edge case - same MSB position but different values */
static void test_same_msb_different_values(void) {
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

/* *****************************************************************************
 * Main
 * ***************************************************************************/

FIO_SFUNC int fio___test_math_div(void) {
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
  /* Benchmarks are intentionally omitted from correctness tests. */

  return tests_failed > 0 ? 1 : 0;
}

/* --- ./tests-old/math-helpers.c --- */

FIO_SFUNC int fio___test_math_helpers(void) {
  { /* Test add/sub carry */
    uint64_t a, c;
    a = fio_math_addc64(1ULL, 1ULL, 1ULL, &c);
    FIO_ASSERT(a == 3 && c == 0,
               "fio_math_addc64(1ULL, 1ULL, 1ULL, &c) failed");
    a = fio_math_addc64(~(uint64_t)0ULL, 1ULL, 0ULL, &c);
    FIO_ASSERT(!a && c == 1,
               "fio_math_addc64(~(uint64_t)0ULL, 1ULL, "
               "0ULL, &c) failed");
    c = 0;
    a = fio_math_addc64(~(uint64_t)0ULL, 1ULL, 1ULL, &c);
    FIO_ASSERT(a == 1 && c == 1,
               "fio_math_addc64(~(uint64_t)0ULL, 1ULL, "
               "1ULL, &c) failed");
    c = 0;
    a = fio_math_addc64(~(uint64_t)0ULL, 0ULL, 1ULL, &c);
    FIO_ASSERT(!a && c == 1,
               "fio_math_addc64(~(uint64_t)0ULL, 0ULL, "
               "1ULL, &c) failed");
    a = fio_math_subc64(3ULL, 1ULL, 1ULL, &c);
    FIO_ASSERT(a == 1 && c == 0, "fio_math_subc64 failed");
    a = fio_math_subc64(~(uint64_t)0ULL, 1ULL, 0ULL, &c);
    FIO_ASSERT(c == 0,
               "fio_math_subc64(~(uint64_t)0ULL, 1ULL, "
               "0ULL, &c) failed");
    a = fio_math_subc64(0ULL, ~(uint64_t)0ULL, 1ULL, &c);
    FIO_ASSERT(!a && c == 1,
               "fio_math_subc64(0ULL, ~(uint64_t)0ULL, "
               "1ULL, &c) failed "
               "(%llu, %llu)",
               a,
               c);
    a = fio_math_subc64(0ULL, 1ULL, 0ULL, &c);
    FIO_ASSERT(a == ~(uint64_t)0ULL && c == 1,
               "fio_math_subc64(0ULL, 1ULL, 0ULL, &c) failed");
  }

  { /* Test division */
    uint64_t n = 0, d = 1;
    for (size_t i = 0; i < 64; ++i) {
      n = (n << 7) ^ 0xAA;
      for (size_t j = 0; j < 64; ++j) {
        d = (d << 3) ^ 0xAA;
        uint64_t q, r;
        FIO_COMPILER_GUARD;
        fio_math_div(&q, &r, &n, &d, 1);
        FIO_COMPILER_GUARD;
        FIO_ASSERT(q == (n / d),
                   "fio_math_div failed quotient for "
                   "0x%llX / 0x%llX (Q=0x%llX "
                   "R=0x%llX)",
                   (long long)n,
                   (long long)d,
                   (long long)q,
                   (long long)r);
        FIO_ASSERT((q * d) + r == n,
                   "fio_math_div failed remainder for "
                   "0x%llX / 0x%llX (Q=0x%llX "
                   "R=0x%llX)",
                   (long long)n,
                   (long long)d,
                   (long long)q,
                   (long long)r);
      }
    }
  }
  { /* Test bit shifting */
    uint64_t a[] = {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0};
    uint64_t b[] = {0xFFFFFFFFFFFFFFFE, 0xFFFFFFFFFFFFFFFF, 1};
    uint64_t c[3];
    fio_math_shl(c, a, 1, 3);
    FIO_ASSERT(!memcmp(b, c, sizeof(c)),
               "left shift failed, %llX:%llX:%llX",
               c[0],
               c[1],
               c[2]);
    fio_math_shr(c, c, 1, 3);
    FIO_ASSERT(!memcmp(a, c, sizeof(c)),
               "right shift failed, %llX:%llX:%llX",
               c[0],
               c[1],
               c[2]);
    fio_math_shl(c, a, 128, 3);
    FIO_ASSERT(!c[0] && !c[1] && !(~c[2]),
               "left shift failed, %llX:%llX:%llX",
               c[0],
               c[1],
               c[2]);
    FIO_ASSERT(fio_math_msb_index(a, 3) == 127,
               "fio_math_msb_index(a) failed %zu",
               fio_math_msb_index(a, 3));
    FIO_ASSERT(fio_math_lsb_index(a, 3) == 0,
               "fio_math_lsb_index(a) failed %zu",
               fio_math_lsb_index(a, 3));
    FIO_ASSERT(fio_math_msb_index(b, 3) == 128,
               "fio_math_msb_index(b) failed %zu",
               fio_math_msb_index(b, 3));
    FIO_ASSERT(fio_math_lsb_index(b, 3) == 1,
               "fio_math_lsb_index(b) failed %zu",
               fio_math_lsb_index(b, 3));
  }
  { /* Test vectors (partial) */
    fio_u128 v128 = {.u64 = {0}};
    fio_u256 v256 = {.u64 = {0}};
    fio_u512 v512 = {.u64 = {0}};
#define FIO_VTEST_ACT_CONST(opt, val)                                          \
  fio_u128_c##opt##64(&v128, &v128, val);                                      \
  fio_u256_c##opt##64(&v256, &v256, val);                                      \
  fio_u512_c##opt##64(&v512, &v512, val);
#define FIO_VTEST_ACT(opt, val)                                                \
  fio_u128_##opt##64(&v128, &v128, &((fio_u128){.u64 = {val, val}}));          \
  fio_u256_##opt##64(&v256,                                                    \
                     &v256,                                                    \
                     &((fio_u256){.u64 = {val, val, val, val}}));              \
  fio_u512_##opt##64(                                                          \
      &v512,                                                                   \
      &v512,                                                                   \
      &((fio_u512){.u64 = {val, val, val, val, val, val, val, val}}));
#define FIO_VTEST_ACT_BIG(opt, val)                                            \
  fio_u128_c##opt##64(&v128, &v128, val);                                      \
  fio_u256_##opt(&v256, &v256, &((fio_u256){.u64 = {val, val, val, val}}));    \
  fio_u512_c##opt##64(&v512, &v512, val);

#define FIO_VTEST_IS_EQ(val)                                                   \
  (v128.u64[0] == val && v128.u64[1] == val && v256.u64[0] == val &&           \
   v256.u64[1] == val && v256.u64[2] == val && v256.u64[3] == val &&           \
   v512.u64[0] == val && v512.u64[1] == val && v512.u64[2] == val &&           \
   v512.u64[3] == val && v512.u64[4] == val && v512.u64[5] == val &&           \
   v512.u64[6] == val && v512.u64[7] == val)

    FIO_VTEST_ACT_CONST(add, 1);
    FIO_VTEST_ACT_CONST(mul, 31);
    FIO_VTEST_ACT_BIG(and, 15);
    FIO_ASSERT(FIO_VTEST_IS_EQ(15),
               "fio_u128 / fio_u256 / fio_u512 failed "
               "with constant vec. operations");
    FIO_VTEST_ACT(sub, 15);
    FIO_VTEST_ACT(add, 1);
    FIO_VTEST_ACT(mul, 31);
    FIO_VTEST_ACT_BIG(and, 15);
    FIO_ASSERT(FIO_VTEST_IS_EQ(15),
               "fio_u128 / fio_u256 / fio_u512 failed "
               "with vector operations");
    FIO_ASSERT(fio_u128_reduce_add64(&v128) == 30 &&
                   fio_u256_reduce_add64(&v256) == 60 &&
                   fio_u512_reduce_add64(&v512) == 120,
               "fio_u128 / fio_u256 / fio_u512 reduce "
               "(add) failed");
    FIO_ASSERT(FIO_VTEST_IS_EQ(15), " reduce had side-effects!");

    fio_u256_add64(&v256, &v256, &(fio_u256){.u64 = {1, 2, 3, 0}});
    FIO_ASSERT(v256.u64[0] == 16 && v256.u64[1] == 17 && v256.u64[2] == 18 &&
                   v256.u64[3] == 15,
               "fio_u256_add64 failed");
    // v256 = fio_u256_shuffle64(v256, 3, 0, 1, 2);
    // FIO_ASSERT(v256.u64[0] == 15 && v256.u64[1] == 16 && v256.u64[2] == 17 &&
    //                v256.u64[3] == 18,
    //            "fio_u256_shuffle64 failed");
  }
  return 0;
}

/* --- ./tests-old/karatsuba.c --- */

/* Helper to print a multi-word number in hex (for debugging on failure) */
FIO_SFUNC void print_bignum(const char *label, const uint64_t *n, size_t len) {
  char buf[1024];
  char *p = buf;
  p += snprintf(p, sizeof(buf), "%s: 0x", label);
  for (size_t i = len; i-- > 0 && (size_t)(p - buf) < sizeof(buf) - 20;) {
    p += snprintf(p,
                  sizeof(buf) - (size_t)(p - buf),
                  "%016llx",
                  (unsigned long long)n[i]);
  }
  FIO_LOG_ERROR("%s", buf);
}

/* Compare two multi-word numbers, returns 0 if equal */
FIO_SFUNC int bignum_cmp(const uint64_t *a, const uint64_t *b, size_t len) {
  for (size_t i = len; i-- > 0;) {
    if (a[i] != b[i])
      return (a[i] > b[i]) ? 1 : -1;
  }
  return 0;
}

/* Test multiplication using long multiplication as reference */
FIO_SFUNC void test_mul_against_long(const uint64_t *a,
                                     const uint64_t *b,
                                     size_t len,
                                     const char *test_name) {
  uint64_t result_karatsuba[64];
  uint64_t result_long[64];

  FIO_ASSERT(len * 2 <= 64, "Test buffer too small");

  /* Initialize result buffers to zero */
  FIO_MEMSET(result_karatsuba, 0, sizeof(result_karatsuba));
  FIO_MEMSET(result_long, 0, sizeof(result_long));

  /* Compute using the main fio_math_mul (which may use Karatsuba) */
  fio_math_mul(result_karatsuba, a, b, len);

  /* Compute using long multiplication directly */
  fio___math_mul_long(result_long, a, b, len);

  /* Compare results */
  if (bignum_cmp(result_karatsuba, result_long, len * 2) != 0) {
    FIO_LOG_ERROR("FAIL: %s", test_name);
    print_bignum("  a", a, len);
    print_bignum("  b", b, len);
    print_bignum("  karatsuba", result_karatsuba, len * 2);
    print_bignum("  long_mul ", result_long, len * 2);
    FIO_ASSERT(0, "Karatsuba multiplication mismatch!");
  }
}

FIO_SFUNC int fio___test_karatsuba(void) {
  /* Test 1: Simple 8-word (512-bit) multiplication */
  {
    uint64_t a[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    uint64_t b[8] = {2, 0, 0, 0, 0, 0, 0, 0};
    test_mul_against_long(a, b, 8, "1 * 2 (512-bit)");
  }

  /* Test 2: Max value in low word */
  {
    uint64_t a[8] = {~0ULL, 0, 0, 0, 0, 0, 0, 0};
    uint64_t b[8] = {~0ULL, 0, 0, 0, 0, 0, 0, 0};
    test_mul_against_long(a, b, 8, "max_u64 * max_u64 (512-bit)");
  }

  /* Test 3: Full 512-bit numbers */
  {
    uint64_t a[8] = {0x123456789ABCDEF0ULL,
                     0xFEDCBA9876543210ULL,
                     0x1111111111111111ULL,
                     0x2222222222222222ULL,
                     0x3333333333333333ULL,
                     0x4444444444444444ULL,
                     0x5555555555555555ULL,
                     0x6666666666666666ULL};
    uint64_t b[8] = {0xAAAAAAAAAAAAAAAAULL,
                     0xBBBBBBBBBBBBBBBBULL,
                     0xCCCCCCCCCCCCCCCCULL,
                     0xDDDDDDDDDDDDDDDDULL,
                     0xEEEEEEEEEEEEEEEEULL,
                     0xFFFFFFFFFFFFFFFFULL,
                     0x0000000000000001ULL,
                     0x0000000000000002ULL};
    test_mul_against_long(a, b, 8, "full 512-bit numbers");
  }

  /* Test 4: 16-word (1024-bit) multiplication */
  {
    uint64_t a[16], b[16];
    for (size_t i = 0; i < 16; ++i) {
      a[i] = 0x123456789ABCDEF0ULL ^ (i * 0x1111111111111111ULL);
      b[i] = 0xFEDCBA9876543210ULL ^ (i * 0x2222222222222222ULL);
    }
    test_mul_against_long(a, b, 16, "1024-bit multiplication");
  }

  /* Test 5: Random values */
  {
    uint64_t a[8], b[8];
    for (int test = 0; test < 100; ++test) {
      fio_rand_bytes(a, sizeof(a));
      fio_rand_bytes(b, sizeof(b));
      test_mul_against_long(a, b, 8, "random 512-bit");
    }
  }

  /* Test 6: Random 1024-bit values */
  {
    uint64_t a[16], b[16];
    for (int test = 0; test < 50; ++test) {
      fio_rand_bytes(a, sizeof(a));
      fio_rand_bytes(b, sizeof(b));
      test_mul_against_long(a, b, 16, "random 1024-bit");
    }
  }

  /* Test 7: Edge case - all ones */
  {
    uint64_t a[8], b[8];
    for (size_t i = 0; i < 8; ++i) {
      a[i] = ~0ULL;
      b[i] = ~0ULL;
    }
    test_mul_against_long(a, b, 8, "all-ones 512-bit");
  }

  /* Test 8: Edge case - alternating bits */
  {
    uint64_t a[8], b[8];
    for (size_t i = 0; i < 8; ++i) {
      a[i] = 0xAAAAAAAAAAAAAAAAULL;
      b[i] = 0x5555555555555555ULL;
    }
    test_mul_against_long(a, b, 8, "alternating bits 512-bit");
  }

  /* Test 9: Verify that smaller sizes still work (use long mul) */
  {
    uint64_t a[4] = {0x123456789ABCDEF0ULL,
                     0xFEDCBA9876543210ULL,
                     0x1111111111111111ULL,
                     0x2222222222222222ULL};
    uint64_t b[4] = {0xAAAAAAAAAAAAAAAAULL,
                     0xBBBBBBBBBBBBBBBBULL,
                     0xCCCCCCCCCCCCCCCCULL,
                     0xDDDDDDDDDDDDDDDDULL};
    test_mul_against_long(a, b, 4, "256-bit (should use long mul)");
  }

  /* Test 10: 2048-bit multiplication */
  {
    uint64_t a[32], b[32];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    test_mul_against_long(a, b, 32, "2048-bit multiplication");
  }
  return 0;
}

/* --- ./tests-old/vector-math.c --- */

#define VECTOR_TYPE   float
#define VECTOR_LENGTH 65536

#define FAIL_IF_NON_EQ(v1, v2)                                                 \
  do {                                                                         \
    for (size_t i = 0; i < VECTOR_LENGTH; ++i) {                               \
      FIO_ASSERT((v1)[i] == (v2)[i],                                           \
                 "(%zu) unexpected value, %zu != %zu",                         \
                 (size_t)__LINE__,                                             \
                 (size_t)((v1)[i]),                                            \
                 (size_t)((v2)[i]));                                           \
    }                                                                          \
  } while (0)

#define FAIL_IF_EQ(v1, v2)                                                     \
  do {                                                                         \
    for (size_t i = 0; i < VECTOR_LENGTH; ++i) {                               \
      FIO_ASSERT((v1)[i] != (v2)[i],                                           \
                 "(%zu) unexpected value, %zu == %zu",                         \
                 (size_t)__LINE__,                                             \
                 (size_t)((v1)[i]),                                            \
                 (size_t)((v2)[i]));                                           \
    }                                                                          \
  } while (0)

FIO_SFUNC int fio___test_vector_math(void) {
  static VECTOR_TYPE buffer[VECTOR_LENGTH * 4];
  VECTOR_TYPE *v[4] = {buffer + (VECTOR_LENGTH * 0),
                       buffer + (VECTOR_LENGTH * 1),
                       buffer + (VECTOR_LENGTH * 2),
                       buffer + (VECTOR_LENGTH * 3)};
  VECTOR_TYPE result = 0;

  for (size_t i = 0; i < VECTOR_LENGTH; ++i) {
    v[0][i] = (VECTOR_TYPE)0;
    v[1][i] = (VECTOR_TYPE)1;
    v[2][i] = (VECTOR_TYPE)2;
    v[3][i] = (VECTOR_TYPE)4;
  }
  FAIL_IF_EQ(v[1], v[2]);
  FAIL_IF_NON_EQ(v[1], v[1]);

  FIO_VEC_REDUCE_ADD(result, v[1], VECTOR_LENGTH);
  FIO_ASSERT(result == VECTOR_LENGTH,
             "FIO_VEC_REDUCE_ADD failed? got %zu",
             (size_t)result);
  result = 0;
  FIO_VEC_REDUCE_ADD(result, v[2], VECTOR_LENGTH);
  FIO_ASSERT(result == VECTOR_LENGTH * 2,
             "FIO_VEC_REDUCE_ADD failed? got %zu",
             (size_t)result);
  result = 0;
  FIO_VEC_DOT(result, v[1], v[2], VECTOR_LENGTH);
  FIO_ASSERT(result == VECTOR_LENGTH * 2,
             "FIO_VEC_DOT failed? got %zu",
             (size_t)result);

  FIO_VEC_MUL(v[0], v[2], v[1], VECTOR_LENGTH);
  FAIL_IF_NON_EQ(v[0], v[2]);
  FAIL_IF_EQ(v[0], v[1]);
  FIO_VEC_SUB(v[0], v[0], v[1], VECTOR_LENGTH);
  FAIL_IF_NON_EQ(v[0], v[1]);
  FAIL_IF_EQ(v[0], v[2]);
  FIO_VEC_ADD(v[0], v[1], v[1], VECTOR_LENGTH);
  FAIL_IF_NON_EQ(v[0], v[2]);
  FIO_VEC_MUL(v[0], v[0], v[2], VECTOR_LENGTH);
  FAIL_IF_NON_EQ(v[0], v[3]);

  return 0;
}

int main(void) {
  FIO_ASSERT(fio___test_math_div() == 0, "math division tests failed");
  FIO_ASSERT(fio___test_math_helpers() == 0, "math helper tests failed");
  FIO_ASSERT(fio___test_karatsuba() == 0, "karatsuba tests failed");
  FIO_ASSERT(fio___test_vector_math() == 0, "vector math tests failed");
  return 0;
}
