/* *****************************************************************************
Test - Karatsuba Multiplication
***************************************************************************** */
#include "test-helpers.h"

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

int main(void) {
  FIO_LOG_DDEBUG("Testing Karatsuba multiplication...");

  /* Test 1: Simple 8-word (512-bit) multiplication */
  {
    uint64_t a[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    uint64_t b[8] = {2, 0, 0, 0, 0, 0, 0, 0};
    test_mul_against_long(a, b, 8, "1 * 2 (512-bit)");
    FIO_LOG_DDEBUG("  [PASS] 1 * 2 (512-bit)");
  }

  /* Test 2: Max value in low word */
  {
    uint64_t a[8] = {~0ULL, 0, 0, 0, 0, 0, 0, 0};
    uint64_t b[8] = {~0ULL, 0, 0, 0, 0, 0, 0, 0};
    test_mul_against_long(a, b, 8, "max_u64 * max_u64 (512-bit)");
    FIO_LOG_DDEBUG("  [PASS] max_u64 * max_u64 (512-bit)");
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
    FIO_LOG_DDEBUG("  [PASS] full 512-bit numbers");
  }

  /* Test 4: 16-word (1024-bit) multiplication */
  {
    uint64_t a[16], b[16];
    for (size_t i = 0; i < 16; ++i) {
      a[i] = 0x123456789ABCDEF0ULL ^ (i * 0x1111111111111111ULL);
      b[i] = 0xFEDCBA9876543210ULL ^ (i * 0x2222222222222222ULL);
    }
    test_mul_against_long(a, b, 16, "1024-bit multiplication");
    FIO_LOG_DDEBUG("  [PASS] 1024-bit multiplication");
  }

  /* Test 5: Random values */
  {
    uint64_t a[8], b[8];
    for (int test = 0; test < 100; ++test) {
      fio_rand_bytes(a, sizeof(a));
      fio_rand_bytes(b, sizeof(b));
      test_mul_against_long(a, b, 8, "random 512-bit");
    }
    FIO_LOG_DDEBUG("  [PASS] 100 random 512-bit multiplications");
  }

  /* Test 6: Random 1024-bit values */
  {
    uint64_t a[16], b[16];
    for (int test = 0; test < 50; ++test) {
      fio_rand_bytes(a, sizeof(a));
      fio_rand_bytes(b, sizeof(b));
      test_mul_against_long(a, b, 16, "random 1024-bit");
    }
    FIO_LOG_DDEBUG("  [PASS] 50 random 1024-bit multiplications");
  }

  /* Test 7: Edge case - all ones */
  {
    uint64_t a[8], b[8];
    for (size_t i = 0; i < 8; ++i) {
      a[i] = ~0ULL;
      b[i] = ~0ULL;
    }
    test_mul_against_long(a, b, 8, "all-ones 512-bit");
    FIO_LOG_DDEBUG("  [PASS] all-ones 512-bit");
  }

  /* Test 8: Edge case - alternating bits */
  {
    uint64_t a[8], b[8];
    for (size_t i = 0; i < 8; ++i) {
      a[i] = 0xAAAAAAAAAAAAAAAAULL;
      b[i] = 0x5555555555555555ULL;
    }
    test_mul_against_long(a, b, 8, "alternating bits 512-bit");
    FIO_LOG_DDEBUG("  [PASS] alternating bits 512-bit");
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
    FIO_LOG_DDEBUG("  [PASS] 256-bit multiplication (long mul path)");
  }

  /* Test 10: 2048-bit multiplication */
  {
    uint64_t a[32], b[32];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    test_mul_against_long(a, b, 32, "2048-bit multiplication");
    FIO_LOG_DDEBUG("  [PASS] 2048-bit multiplication");
  }

  FIO_LOG_DDEBUG("All Karatsuba tests passed!");
  return 0;
}
