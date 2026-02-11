/* *****************************************************************************
Performance Tests: Core Module

These tests measure performance of core operations and are skipped in DEBUG
mode. Run with: make tests/performance-core
***************************************************************************** */

#define FIO_LOG
#define FIO_TIME
#define FIO_RAND
#include "test-helpers.h"

/* Skip all performance tests in DEBUG mode */
#ifdef DEBUG
int main(void) {
  FIO_LOG_INFO("Performance tests skipped in DEBUG mode");
  return 0;
}
#else

/* *****************************************************************************
Helper Macros
***************************************************************************** */

/** Benchmark macro - runs code and reports throughput in M ops/sec */
#define FIO_PERF_BENCHMARK(name, iterations, setup, code, teardown)            \
  do {                                                                         \
    setup;                                                                     \
    for (size_t i_ = 0; i_ < (iterations); ++i_) {                             \
      code;                                                                    \
      FIO_COMPILER_GUARD; /* warmup */                                         \
    }                                                                          \
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
Performance Tests: Vector XOR/AND/OR Operations
***************************************************************************** */

FIO_SFUNC void fio___perf_vector_xor(void) {
  fprintf(stderr, "\t* Benchmarking Vector XOR operations...\n");

  const size_t iterations = 1000000;

  /* fio_u128 XOR */
  {
    fio_u128 a = fio_u128_init64(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL);
    fio_u128 b = fio_u128_init64(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL);
    fio_u128 r;
    FIO_PERF_BENCHMARK("fio_u128_xorv",
                       iterations,
                       (void)0,
                       (r = fio_u128_xorv(a, b), a = r),
                       (void)r);
  }

  /* fio_u256 XOR */
  {
    fio_u256 a = fio_u256_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL);
    fio_u256 b = fio_u256_init64(0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0x3333333333333333ULL,
                                 0xCCCCCCCCCCCCCCCCULL);
    fio_u256 r;
    FIO_PERF_BENCHMARK("fio_u256_xorv",
                       iterations,
                       (void)0,
                       (r = fio_u256_xorv(a, b), a = r),
                       (void)r);
  }

  /* fio_u512 XOR */
  {
    fio_u512 a = fio_u512_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL,
                                 0x1111111111111111ULL,
                                 0x2222222222222222ULL,
                                 0x3333333333333333ULL,
                                 0x4444444444444444ULL);
    fio_u512 b = fio_u512_init64(0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0x3333333333333333ULL,
                                 0xCCCCCCCCCCCCCCCCULL,
                                 0xDDDDDDDDDDDDDDDDULL,
                                 0xEEEEEEEEEEEEEEEEULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0x0000000000000000ULL);
    fio_u512 r;
    FIO_PERF_BENCHMARK("fio_u512_xorv",
                       iterations,
                       (void)0,
                       (r = fio_u512_xorv(a, b), a = r),
                       (void)r);
  }
}

FIO_SFUNC void fio___perf_vector_and(void) {
  fprintf(stderr, "\t* Benchmarking Vector AND operations...\n");

  const size_t iterations = 1000000;

  /* fio_u128 AND */
  {
    fio_u128 a = fio_u128_init64(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL);
    fio_u128 b = fio_u128_init64(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    fio_u128 r;
    FIO_PERF_BENCHMARK("fio_u128_andv",
                       iterations,
                       (void)0,
                       (r = fio_u128_andv(a, b), a.u64[0] ^= r.u64[0]),
                       (void)r);
  }

  /* fio_u256 AND */
  {
    fio_u256 a = fio_u256_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL);
    fio_u256 b = fio_u256_init64(0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL);
    fio_u256 r;
    FIO_PERF_BENCHMARK("fio_u256_andv",
                       iterations,
                       (void)0,
                       (r = fio_u256_andv(a, b), a.u64[0] ^= r.u64[0]),
                       (void)r);
  }

  /* fio_u512 AND */
  {
    fio_u512 a = fio_u512_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL,
                                 0x1111111111111111ULL,
                                 0x2222222222222222ULL,
                                 0x3333333333333333ULL,
                                 0x4444444444444444ULL);
    fio_u512 b = fio_u512_init64(0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL);
    fio_u512 r;
    FIO_PERF_BENCHMARK("fio_u512_andv",
                       iterations,
                       (void)0,
                       (r = fio_u512_andv(a, b), a.u64[0] ^= r.u64[0]),
                       (void)r);
  }
}

FIO_SFUNC void fio___perf_vector_or(void) {
  fprintf(stderr, "\t* Benchmarking Vector OR operations...\n");

  const size_t iterations = 1000000;

  /* fio_u128 OR */
  {
    fio_u128 a = fio_u128_init64(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL);
    fio_u128 b = fio_u128_init64(0x0000000000000000ULL, 0x0000000000000000ULL);
    fio_u128 r;
    FIO_PERF_BENCHMARK("fio_u128_orv",
                       iterations,
                       (void)0,
                       (r = fio_u128_orv(a, b), a.u64[0] ^= r.u64[0]),
                       (void)r);
  }

  /* fio_u256 OR */
  {
    fio_u256 a = fio_u256_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL);
    fio_u256 b = fio_u256_init64(0, 0, 0, 0);
    fio_u256 r;
    FIO_PERF_BENCHMARK("fio_u256_orv",
                       iterations,
                       (void)0,
                       (r = fio_u256_orv(a, b), a.u64[0] ^= r.u64[0]),
                       (void)r);
  }

  /* fio_u512 OR */
  {
    fio_u512 a = fio_u512_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL,
                                 0x1111111111111111ULL,
                                 0x2222222222222222ULL,
                                 0x3333333333333333ULL,
                                 0x4444444444444444ULL);
    fio_u512 b = fio_u512_init64(0, 0, 0, 0, 0, 0, 0, 0);
    fio_u512 r;
    FIO_PERF_BENCHMARK("fio_u512_orv",
                       iterations,
                       (void)0,
                       (r = fio_u512_orv(a, b), a.u64[0] ^= r.u64[0]),
                       (void)r);
  }
}

/* *****************************************************************************
Performance Tests: Vector Addition (lane-wise)
***************************************************************************** */

FIO_SFUNC void fio___perf_vector_add(void) {
  fprintf(stderr, "\t* Benchmarking Vector ADD operations (lane-wise)...\n");

  const size_t iterations = 1000000;

  /* fio_u128 addv64 */
  {
    fio_u128 a = fio_u128_init64(1, 2);
    fio_u128 b = fio_u128_init64(3, 4);
    fio_u128 r;
    FIO_PERF_BENCHMARK("fio_u128_addv64",
                       iterations,
                       (void)0,
                       (r = fio_u128_addv64(a, b), a = r),
                       (void)r);
  }

  /* fio_u256 addv64 */
  {
    fio_u256 a = fio_u256_init64(1, 2, 3, 4);
    fio_u256 b = fio_u256_init64(5, 6, 7, 8);
    fio_u256 r;
    FIO_PERF_BENCHMARK("fio_u256_addv64",
                       iterations,
                       (void)0,
                       (r = fio_u256_addv64(a, b), a = r),
                       (void)r);
  }

  /* fio_u512 addv64 */
  {
    fio_u512 a = fio_u512_init64(1, 2, 3, 4, 5, 6, 7, 8);
    fio_u512 b = fio_u512_init64(9, 10, 11, 12, 13, 14, 15, 16);
    fio_u512 r;
    FIO_PERF_BENCHMARK("fio_u512_addv64",
                       iterations,
                       (void)0,
                       (r = fio_u512_addv64(a, b), a = r),
                       (void)r);
  }
}

/* *****************************************************************************
Performance Tests: Multi-Precision ADD/SUB (with carry)
***************************************************************************** */

FIO_SFUNC void fio___perf_multiprecision_add(void) {
  fprintf(stderr, "\t* Benchmarking Multi-precision ADD (with carry)...\n");

  const size_t iterations = 1000000;

  /* fio_u128_add */
  {
    fio_u128 a = fio_u128_init64(0xFFFFFFFFFFFFFFFFULL, 0x0000000000000001ULL);
    fio_u128 b = fio_u128_init64(0x0000000000000001ULL, 0x0000000000000001ULL);
    fio_u128 r;
    bool carry;
    FIO_PERF_BENCHMARK("fio_u128_add",
                       iterations,
                       (void)0,
                       (carry = fio_u128_add(&r, &a, &b), a = r),
                       (void)carry);
  }

  /* fio_u256_add */
  {
    fio_u256 a = fio_u256_init64(0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL);
    fio_u256 b = fio_u256_init64(0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL);
    fio_u256 r;
    bool carry;
    FIO_PERF_BENCHMARK("fio_u256_add",
                       iterations,
                       (void)0,
                       (carry = fio_u256_add(&r, &a, &b), a = r),
                       (void)carry);
  }

  /* fio_u512_add */
  {
    fio_u512 a = fio_u512_init64(0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL);
    fio_u512 b = fio_u512_init64(0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL);
    fio_u512 r;
    bool carry;
    FIO_PERF_BENCHMARK("fio_u512_add",
                       iterations,
                       (void)0,
                       (carry = fio_u512_add(&r, &a, &b), a = r),
                       (void)carry);
  }
}

FIO_SFUNC void fio___perf_multiprecision_sub(void) {
  fprintf(stderr, "\t* Benchmarking Multi-precision SUB (with borrow)...\n");

  const size_t iterations = 1000000;

  /* fio_u128_sub */
  {
    fio_u128 a = fio_u128_init64(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    fio_u128 b = fio_u128_init64(0x0000000000000001ULL, 0x0000000000000001ULL);
    fio_u128 r;
    bool borrow;
    FIO_PERF_BENCHMARK("fio_u128_sub",
                       iterations,
                       (void)0,
                       (borrow = fio_u128_sub(&r, &a, &b), a = r),
                       (void)borrow);
  }

  /* fio_u256_sub */
  {
    fio_u256 a = fio_u256_init64(0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL);
    fio_u256 b = fio_u256_init64(0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL);
    fio_u256 r;
    bool borrow;
    FIO_PERF_BENCHMARK("fio_u256_sub",
                       iterations,
                       (void)0,
                       (borrow = fio_u256_sub(&r, &a, &b), a = r),
                       (void)borrow);
  }

  /* fio_u512_sub */
  {
    fio_u512 a = fio_u512_init64(0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL,
                                 0xFFFFFFFFFFFFFFFFULL);
    fio_u512 b = fio_u512_init64(0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL,
                                 0x0000000000000001ULL);
    fio_u512 r;
    bool borrow;
    FIO_PERF_BENCHMARK("fio_u512_sub",
                       iterations,
                       (void)0,
                       (borrow = fio_u512_sub(&r, &a, &b), a = r),
                       (void)borrow);
  }
}

/* *****************************************************************************
Performance Tests: Multi-Precision MUL
***************************************************************************** */

FIO_SFUNC void fio___perf_multiprecision_mul(void) {
  fprintf(stderr, "\t* Benchmarking Multi-precision MUL...\n");

  const size_t iterations = 100000;

  /* fio_u128_mul */
  {
    fio_u128 a = fio_u128_init64(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL);
    fio_u128 b = fio_u128_init64(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL);
    fio_u256 r;
    FIO_PERF_BENCHMARK(
        "fio_u128_mul",
        iterations,
        (void)0,
        (fio_u128_mul(&r, &a, &b), a.u64[0] = r.u64[0], a.u64[1] = r.u64[1]),
        (void)r);
  }

  /* fio_u256_mul */
  {
    fio_u256 a = fio_u256_init64(0x123456789ABCDEF0ULL,
                                 0xFEDCBA9876543210ULL,
                                 0x0F0F0F0F0F0F0F0FULL,
                                 0xF0F0F0F0F0F0F0F0ULL);
    fio_u256 b = fio_u256_init64(0xAAAAAAAAAAAAAAAAULL,
                                 0x5555555555555555ULL,
                                 0x3333333333333333ULL,
                                 0xCCCCCCCCCCCCCCCCULL);
    fio_u512 r;
    FIO_PERF_BENCHMARK("fio_u256_mul",
                       iterations,
                       (void)0,
                       (fio_u256_mul(&r, &a, &b),
                        a.u64[0] = r.u64[0],
                        a.u64[1] = r.u64[1],
                        a.u64[2] = r.u64[2],
                        a.u64[3] = r.u64[3]),
                       (void)r);
  }
}

/* *****************************************************************************
Performance Tests: Karatsuba vs Long Multiplication
***************************************************************************** */

FIO_SFUNC void fio___perf_karatsuba_vs_long(void) {
  fprintf(stderr, "\t* Benchmarking Karatsuba vs Long multiplication...\n");

  const size_t iterations = 10000;

  /* 256-bit (4 words) - should use long multiplication */
  {
    uint64_t a[4], b[4], result[8];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    FIO_PERF_BENCHMARK("256-bit fio_math_mul (long)",
                       iterations,
                       (void)0,
                       (fio___math_mul_long(result, a, b, 4), a[0] = result[0]),
                       (void)0);
    FIO_PERF_BENCHMARK(
        "256-bit fio_math_mul (Karatsuba)",
        iterations,
        (void)0,
        (fio___math_mul_karatsuba(result, a, b, 4), a[0] = result[0]),
        (void)0);
  }

  /* 512-bit (8 words) - should use Karatsuba */
  {
    uint64_t a[8], b[8], result[16];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    FIO_PERF_BENCHMARK("512-bit fio_math_mul (long)",
                       iterations,
                       (void)0,
                       (fio___math_mul_long(result, a, b, 8), a[0] = result[0]),
                       (void)0);
    FIO_PERF_BENCHMARK(
        "512-bit fio_math_mul (Karatsuba)",
        iterations,
        (void)0,
        (fio___math_mul_karatsuba(result, a, b, 8), a[0] = result[0]),
        (void)0);
  }

  /* 1024-bit (16 words) - should use Karatsuba */
  {
    uint64_t a[16], b[16], result[32];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    FIO_PERF_BENCHMARK(
        "1024-bit fio_math_mul (long)",
        iterations,
        (void)0,
        (fio___math_mul_long(result, a, b, 16), a[0] = result[0]),
        (void)0);
    FIO_PERF_BENCHMARK("1024-bit fio_math_mul (Karatsuba)",
                       iterations,
                       (void)0,
                       (fio_math_mul(result, a, b, 16), a[0] = result[0]),
                       (void)0);
  }

  /* 2048-bit (32 words) - should use Karatsuba */
  {
    uint64_t a[32], b[32], result[64];
    fio_rand_bytes(a, sizeof(a));
    fio_rand_bytes(b, sizeof(b));
    FIO_PERF_BENCHMARK(
        "2048-bit fio_math_mul (long)",
        iterations / 2,
        (void)0,
        (fio___math_mul_long(result, a, b, 32), a[0] = result[0]),
        (void)0);
    FIO_PERF_BENCHMARK("2048-bit fio_math_mul (Karatsuba)",
                       iterations / 2,
                       (void)0,
                       (fio_math_mul(result, a, b, 32), a[0] = result[0]),
                       (void)0);
  }
}

/* *****************************************************************************
Performance Tests: Byte Swap Operations
***************************************************************************** */

FIO_SFUNC void fio___perf_byte_swap(void) {
  fprintf(stderr, "\t* Benchmarking Byte swap operations...\n");

  const size_t iterations = 10000000;

  /* fio_bswap16 */
  {
    uint16_t v = 0x0102;
    FIO_PERF_BENCHMARK("fio_bswap16",
                       iterations,
                       (void)0,
                       v = fio_bswap16(v),
                       (void)v);
  }

  /* fio_bswap32 */
  {
    uint32_t v = 0x01020304;
    FIO_PERF_BENCHMARK("fio_bswap32",
                       iterations,
                       (void)0,
                       v = fio_bswap32(v),
                       (void)v);
  }

  /* fio_bswap64 */
  {
    uint64_t v = 0x0102030405060708ULL;
    FIO_PERF_BENCHMARK("fio_bswap64",
                       iterations,
                       (void)0,
                       v = fio_bswap64(v),
                       (void)v);
  }

#ifdef __SIZEOF_INT128__
  /* fio_bswap128 */
  {
    __uint128_t v =
        ((__uint128_t)0x0102030405060708ULL << 64) | 0x090A0B0C0D0E0F10ULL;
    FIO_PERF_BENCHMARK("fio_bswap128",
                       iterations,
                       (void)0,
                       v = fio_bswap128(v),
                       (void)v);
  }
#endif
}

/* *****************************************************************************
Performance Tests: Bit Rotation Operations
***************************************************************************** */

FIO_SFUNC void fio___perf_bit_rotation(void) {
  fprintf(stderr, "\t* Benchmarking Bit rotation operations...\n");

  const size_t iterations = 10000000;

  /* fio_lrot8 */
  {
    uint8_t v = 0x12;
    FIO_PERF_BENCHMARK("fio_lrot8",
                       iterations,
                       (void)0,
                       v = fio_lrot8(v, 3),
                       (void)v);
  }

  /* fio_rrot8 */
  {
    uint8_t v = 0x12;
    FIO_PERF_BENCHMARK("fio_rrot8",
                       iterations,
                       (void)0,
                       v = fio_rrot8(v, 3),
                       (void)v);
  }

  /* fio_lrot32 */
  {
    uint32_t v = 0x01020304;
    FIO_PERF_BENCHMARK("fio_lrot32",
                       iterations,
                       (void)0,
                       v = fio_lrot32(v, 17),
                       (void)v);
  }

  /* fio_rrot32 */
  {
    uint32_t v = 0x01020304;
    FIO_PERF_BENCHMARK("fio_rrot32",
                       iterations,
                       (void)0,
                       v = fio_rrot32(v, 17),
                       (void)v);
  }

  /* fio_lrot64 */
  {
    uint64_t v = 0x0102030405060708ULL;
    FIO_PERF_BENCHMARK("fio_lrot64",
                       iterations,
                       (void)0,
                       v = fio_lrot64(v, 17),
                       (void)v);
  }

  /* fio_rrot64 */
  {
    uint64_t v = 0x0102030405060708ULL;
    FIO_PERF_BENCHMARK("fio_rrot64",
                       iterations,
                       (void)0,
                       v = fio_rrot64(v, 17),
                       (void)v);
  }
}

/* *****************************************************************************
Performance Tests: Popcount
***************************************************************************** */

FIO_SFUNC void fio___perf_popcount(void) {
  fprintf(stderr, "\t* Benchmarking Popcount operations...\n");

  const size_t iterations = 10000000;

  {
    uint64_t v = 0xAAAAAAAAAAAAAAAAULL;
    int sum = 0;
    FIO_PERF_BENCHMARK("fio_popcount",
                       iterations,
                       (void)0,
                       (sum += fio_popcount(v), v = fio_lrot64(v, 1)),
                       (void)sum);
  }
}

/* *****************************************************************************
Performance Tests: Constant-Time Operations
***************************************************************************** */

FIO_SFUNC void fio___perf_ct_operations(void) {
  fprintf(stderr, "\t* Benchmarking Constant-time operations...\n");

  const size_t iterations = 10000000;

  /* fio_ct_true */
  {
    uintptr_t v = 1;
    uintptr_t result = 0;
    FIO_PERF_BENCHMARK("fio_ct_true",
                       iterations,
                       (void)0,
                       (result = fio_ct_true(v), v = result + 1),
                       (void)result);
  }

  /* fio_ct_false */
  {
    uintptr_t v = 0;
    uintptr_t result = 0;
    FIO_PERF_BENCHMARK("fio_ct_false",
                       iterations,
                       (void)0,
                       (result = fio_ct_false(v), v = result),
                       (void)result);
  }

  /* fio_ct_if */
  {
    uintmax_t cond = 1;
    uintmax_t result = 0;
    FIO_PERF_BENCHMARK("fio_ct_if",
                       iterations,
                       (void)0,
                       (result = fio_ct_if(cond, 42, 0), cond = result & 1),
                       (void)result);
  }

  /* fio_ct_if_bool */
  {
    uintmax_t cond = 1;
    uintmax_t result = 0;
    FIO_PERF_BENCHMARK(
        "fio_ct_if_bool",
        iterations,
        (void)0,
        (result = fio_ct_if_bool(cond, 42, 0), cond = result & 1),
        (void)result);
  }

  /* fio_ct_max */
  {
    intmax_t a = 100;
    intmax_t b = 200;
    intmax_t result = 0;
    FIO_PERF_BENCHMARK(
        "fio_ct_max",
        iterations,
        (void)0,
        (result = fio_ct_max(a, b), a = result - 1, b = result + 1),
        (void)result);
  }

  /* fio_ct_min */
  {
    intmax_t a = 100;
    intmax_t b = 200;
    intmax_t result = 0;
    FIO_PERF_BENCHMARK(
        "fio_ct_min",
        iterations,
        (void)0,
        (result = fio_ct_min(a, b), a = result - 1, b = result + 1),
        (void)result);
  }

  /* fio_ct_abs */
  {
    intmax_t v = -42;
    uintmax_t result = 0;
    FIO_PERF_BENCHMARK("fio_ct_abs",
                       iterations,
                       (void)0,
                       (result = fio_ct_abs(v), v = -(intmax_t)result),
                       (void)result);
  }
}

/* *****************************************************************************
Performance Tests: Constant-Time Bitwise Selection
***************************************************************************** */

FIO_SFUNC void fio___perf_ct_bitwise(void) {
  fprintf(stderr, "\t* Benchmarking Constant-time bitwise selection...\n");

  const size_t iterations = 10000000;

  /* fio_ct_mux32 */
  {
    uint32_t x = 0xF0F0F0F0;
    uint32_t y = 0xAAAAAAAA;
    uint32_t z = 0x55555555;
    uint32_t result = 0;
    FIO_PERF_BENCHMARK("fio_ct_mux32",
                       iterations,
                       (void)0,
                       (result = fio_ct_mux32(x, y, z), x ^= result),
                       (void)result);
  }

  /* fio_ct_mux64 */
  {
    uint64_t x = 0xF0F0F0F0F0F0F0F0ULL;
    uint64_t y = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t z = 0x5555555555555555ULL;
    uint64_t result = 0;
    FIO_PERF_BENCHMARK("fio_ct_mux64",
                       iterations,
                       (void)0,
                       (result = fio_ct_mux64(x, y, z), x ^= result),
                       (void)result);
  }

  /* fio_ct_maj32 */
  {
    uint32_t x = 0xAAAAAAAA;
    uint32_t y = 0xCCCCCCCC;
    uint32_t z = 0xF0F0F0F0;
    uint32_t result = 0;
    FIO_PERF_BENCHMARK("fio_ct_maj32",
                       iterations,
                       (void)0,
                       (result = fio_ct_maj32(x, y, z), x ^= result),
                       (void)result);
  }

  /* fio_ct_maj64 */
  {
    uint64_t x = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t y = 0xCCCCCCCCCCCCCCCCULL;
    uint64_t z = 0xF0F0F0F0F0F0F0F0ULL;
    uint64_t result = 0;
    FIO_PERF_BENCHMARK("fio_ct_maj64",
                       iterations,
                       (void)0,
                       (result = fio_ct_maj64(x, y, z), x ^= result),
                       (void)result);
  }
}

/* *****************************************************************************
Main Entry Point
***************************************************************************** */

int main(void) {
#if defined(DEBUG) && (DEBUG)
  if (1) {
    fprintf(stderr, "\t- Skipped in DEBUG\n");
    return 0;
  }
#endif

  fprintf(stderr, "===========================================\n");
  fprintf(stderr, "Performance Tests: Core Module\n");
  fprintf(stderr, "===========================================\n\n");

  fio___perf_vector_xor();
  fio___perf_vector_and();
  fio___perf_vector_or();
  fio___perf_vector_add();
  fio___perf_multiprecision_add();
  fio___perf_multiprecision_sub();
  fio___perf_multiprecision_mul();
  fio___perf_karatsuba_vs_long();
  fio___perf_byte_swap();
  fio___perf_bit_rotation();
  fio___perf_popcount();
  fio___perf_ct_operations();
  fio___perf_ct_bitwise();

  fprintf(stderr, "\n===========================================\n");
  fprintf(stderr, "Performance tests complete.\n");
  fprintf(stderr, "===========================================\n");
  return 0;
}

#endif /* DEBUG */
