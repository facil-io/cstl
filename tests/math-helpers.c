/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

int main(void) {
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
}
