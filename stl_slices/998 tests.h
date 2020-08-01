/* *****************************************************************************














                                Testing














***************************************************************************** */

#if !defined(FIO_FIO_TEST_CSTL_ONLY_ONCE) && (defined(FIO_TEST_CSTL))
#undef FIO_TEST_CSTL
#define FIO_FIO_TEST_CSTL_ONLY_ONCE 1

#ifdef FIO_EXTERN_TEST
void fio_test_dynamic_types(void);
#else
FIO_SFUNC void fio_test_dynamic_types(void);
#endif
#if !defined(FIO_EXTERN_TEST) || defined(FIO_EXTERN_COMPLETE)

/* Common testing values / Macros */
#define TEST_FUNC static __attribute__((unused))
#define TEST_REPEAT 4096
#define FIO_T_ASSERT(cond, ...)                                                \
  if (!(cond)) {                                                               \
    FIO_LOG2STDERR2(__VA_ARGS__);                                              \
    abort();                                                                   \
  }

/* Make sure logging and FIOBJ memory marking are set. */
#if !defined(FIO_LOG) || defined(FIO_LOG2STDERR2)
#define FIO_LOG
#endif
#ifndef FIOBJ_MARK_MEMORY
#define FIOBJ_MARK_MEMORY 1
#endif
#ifndef FIO_FIOBJ
#define FIO_FIOBJ
#endif

/* Add non-type options to minimize `#include` instructions */
#define FIO_ATOL
#define FIO_BITWISE
#define FIO_BITMAP
#define FIO_RAND
#define FIO_ATOMIC
#define FIO_RISKY_HASH
#include __FILE__

TEST_FUNC uintptr_t fio___dynamic_types_test_tag(uintptr_t i) { return i | 1; }
TEST_FUNC uintptr_t fio___dynamic_types_test_untag(uintptr_t i) {
  return i & (~((uintptr_t)1UL));
}

/* *****************************************************************************
String <=> Number - test
***************************************************************************** */

TEST_FUNC void fio___dynamic_types_test___atol(void) {
  fprintf(stderr, "* Testing fio_atol and fio_ltoa.\n");
  char buffer[1024];
  for (int i = 0 - TEST_REPEAT; i < TEST_REPEAT; ++i) {
    size_t tmp = fio_ltoa(buffer, i, 0);
    FIO_T_ASSERT(tmp > 0, "fio_ltoa returned length error");
    buffer[tmp++] = 0;
    char *tmp2 = buffer;
    int i2 = fio_atol(&tmp2);
    FIO_T_ASSERT(tmp2 > buffer, "fio_atol pointer motion error");
    FIO_T_ASSERT(i == i2, "fio_ltoa-fio_atol roundtrip error %lld != %lld", i,
                 i2);
  }
  for (size_t bit = 0; bit < sizeof(int64_t) * 8; ++bit) {
    uint64_t i = (uint64_t)1 << bit;
    size_t tmp = fio_ltoa(buffer, (int64_t)i, 0);
    FIO_T_ASSERT(tmp > 0, "fio_ltoa return length error");
    buffer[tmp] = 0;
    char *tmp2 = buffer;
    int64_t i2 = fio_atol(&tmp2);
    FIO_T_ASSERT(tmp2 > buffer, "fio_atol pointer motion error");
    FIO_T_ASSERT((int64_t)i == i2,
                 "fio_ltoa-fio_atol roundtrip error %lld != %lld", i, i2);
  }
  fprintf(stderr, "* Testing fio_atol samples.\n");
#define TEST_ATOL(s, n)                                                        \
  do {                                                                         \
    char *p = (char *)(s);                                                     \
    int64_t r = fio_atol(&p);                                                  \
    FIO_ASSERT(r == (n), "fio_atol test error! %s => %zd (not %zd)",           \
               ((char *)(s)), (size_t)r, (size_t)n);                           \
    FIO_ASSERT((s) + strlen((s)) == p,                                         \
               "fio_atol test error! %s reading position not at end (%zu)",    \
               (s), (size_t)(p - (s)));                                        \
    char buf[72];                                                              \
    buf[fio_ltoa(buf, n, 2)] = 0;                                              \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 2 test error! "                                  \
               "%s != %s (%zd)",                                               \
               buf, ((char *)(s)), (size_t)((p = buf), fio_atol(&p)));         \
    buf[fio_ltoa(buf, n, 8)] = 0;                                              \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 8 test error! "                                  \
               "%s != %s (%zd)",                                               \
               buf, ((char *)(s)), (size_t)((p = buf), fio_atol(&p)));         \
    buf[fio_ltoa(buf, n, 10)] = 0;                                             \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 10 test error! "                                 \
               "%s != %s (%zd)",                                               \
               buf, ((char *)(s)), (size_t)((p = buf), fio_atol(&p)));         \
    buf[fio_ltoa(buf, n, 16)] = 0;                                             \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 16 test error! "                                 \
               "%s != %s (%zd)",                                               \
               buf, ((char *)(s)), (size_t)((p = buf), fio_atol(&p)));         \
  } while (0)

  TEST_ATOL("0x1", 1);
  TEST_ATOL("-0x1", -1);
  TEST_ATOL("-0xa", -10);                                /* sign before hex */
  TEST_ATOL("0xe5d4c3b2a1908770", -1885667171979196560); /* sign within hex */
  TEST_ATOL("0b00000000000011", 3);
  TEST_ATOL("-0b00000000000011", -3);
  TEST_ATOL("0b0000000000000000000000000000000000000000000000000", 0);
  TEST_ATOL("0", 0);
  TEST_ATOL("1", 1);
  TEST_ATOL("2", 2);
  TEST_ATOL("-2", -2);
  TEST_ATOL("0000000000000000000000000000000000000000000000042", 34); /* oct */
  TEST_ATOL("9223372036854775807", 9223372036854775807LL); /* INT64_MAX */
  TEST_ATOL("9223372036854775808",
            9223372036854775807LL); /* INT64_MAX overflow protection */
  TEST_ATOL("9223372036854775999",
            9223372036854775807LL); /* INT64_MAX overflow protection */
#undef TEST_ATOL

#ifdef FIO_ATOF_ALT
#define TEST_DOUBLE(s, d, stop)                                                \
  do {                                                                         \
    union {                                                                    \
      double d_;                                                               \
      uint64_t as_i;                                                           \
    } pn, pn2;                                                                 \
    pn2.d_ = d;                                                                \
    char *p = (char *)(s);                                                     \
    char *p2 = (char *)(s);                                                    \
    double r = fio_atof(&p);                                                   \
    double std = strtod(p2, &p2);                                              \
    (void)std;                                                                 \
    pn.d_ = r;                                                                 \
    FIO_ASSERT(*p == stop || p == p2,                                          \
               "float parsing didn't stop at correct possition! %x != %x", *p, \
               stop);                                                          \
    if ((double)d == r || r == std) {                                          \
      /** fprintf(stderr, "Okay for %s\n", s); */                              \
    } else if ((pn2.as_i + 1) == (pn.as_i) || (pn.as_i + 1) == pn2.as_i) {     \
      fprintf(stderr, "* WARNING: Single bit rounding error detected: %s\n",   \
              s);                                                              \
    } else if (r == 0.0 && d != 0.0) {                                         \
      fprintf(stderr, "* WARNING: float range limit marked before: %s\n", s);  \
    } else {                                                                   \
      char f_buf[164];                                                         \
      pn.d_ = std;                                                             \
      pn2.d_ = r;                                                              \
      size_t tmp_pos = fio_ltoa(f_buf, pn.as_i, 2);                            \
      f_buf[tmp_pos] = '\n';                                                   \
      fio_ltoa(f_buf + tmp_pos + 1, pn2.as_i, 2);                              \
      FIO_ASSERT(0,                                                            \
                 "Float error bigger than a single bit rounding error. exp. "  \
                 "vs. act.:\n%.19g\n%.19g\nBinary:\n%s",                       \
                 std, r, f_buf);                                               \
    }                                                                          \
  } while (0)

  fprintf(stderr, "* Testing fio_atof samples.\n");

  /* A few hex-float examples  */
  TEST_DOUBLE("0x10.1p0", 0x10.1p0, 0);
  TEST_DOUBLE("0x1.8p1", 0x1.8p1, 0);
  TEST_DOUBLE("0x1.8p5", 0x1.8p5, 0);
  TEST_DOUBLE("0x4.0p5", 0x4.0p5, 0);
  TEST_DOUBLE("0x1.0p50a", 0x1.0p50, 'a');
  TEST_DOUBLE("0x1.0p500", 0x1.0p500, 0);
  TEST_DOUBLE("0x1.0P-1074", 0x1.0P-1074, 0);
  TEST_DOUBLE("0x3a.0P-1074", 0x3a.0P-1074, 0);

  /* These numbers were copied from https://gist.github.com/mattn/1890186 */
  TEST_DOUBLE(".1", 0.1, 0);
  TEST_DOUBLE("  .", 0, 0);
  TEST_DOUBLE("  1.2e3", 1.2e3, 0);
  TEST_DOUBLE(" +1.2e3", 1.2e3, 0);
  TEST_DOUBLE("1.2e3", 1.2e3, 0);
  TEST_DOUBLE("+1.2e3", 1.2e3, 0);
  TEST_DOUBLE("+1.e3", 1000, 0);
  TEST_DOUBLE("-1.2e3", -1200, 0);
  TEST_DOUBLE("-1.2e3.5", -1200, '.');
  TEST_DOUBLE("-1.2e", -1.2, 0);
  TEST_DOUBLE("--1.2e3.5", 0, '-');
  TEST_DOUBLE("--1-.2e3.5", 0, '-');
  TEST_DOUBLE("-a", 0, 'a');
  TEST_DOUBLE("a", 0, 'a');
  TEST_DOUBLE(".1e", 0.1, 0);
  TEST_DOUBLE(".1e3", 100, 0);
  TEST_DOUBLE(".1e-3", 0.1e-3, 0);
  TEST_DOUBLE(".1e-", 0.1, 0);
  TEST_DOUBLE(" .e-", 0, 0);
  TEST_DOUBLE(" .e", 0, 0);
  TEST_DOUBLE(" e", 0, 0);
  TEST_DOUBLE(" e0", 0, 0);
  TEST_DOUBLE(" ee", 0, 'e');
  TEST_DOUBLE(" -e", 0, 0);
  TEST_DOUBLE(" .9", 0.9, 0);
  TEST_DOUBLE(" ..9", 0, '.');
  TEST_DOUBLE("009", 9, 0);
  TEST_DOUBLE("0.09e02", 9, 0);
  /* http://thread.gmane.org/gmane.editors.vim.devel/19268/ */
  TEST_DOUBLE("0.9999999999999999999999999999999999", 1, 0);
  TEST_DOUBLE("2.2250738585072010e-308", 2.225073858507200889e-308, 0);
  TEST_DOUBLE("2.2250738585072013e-308", 2.225073858507201383e-308, 0);
  TEST_DOUBLE("9214843084008499", 9214843084008499, 0);
  TEST_DOUBLE("30078505129381147446200", 3.007850512938114954e+22, 0);

  /* These numbers were copied from https://github.com/miloyip/rapidjson */
  TEST_DOUBLE("0.0", 0.0, 0);
  TEST_DOUBLE("-0.0", -0.0, 0);
  TEST_DOUBLE("1.0", 1.0, 0);
  TEST_DOUBLE("-1.0", -1.0, 0);
  TEST_DOUBLE("1.5", 1.5, 0);
  TEST_DOUBLE("-1.5", -1.5, 0);
  TEST_DOUBLE("3.1416", 3.1416, 0);
  TEST_DOUBLE("1E10", 1E10, 0);
  TEST_DOUBLE("1e10", 1e10, 0);
  TEST_DOUBLE("100000000000000000000000000000000000000000000000000000000000"
              "000000000000000000000",
              1E80, 0);
  TEST_DOUBLE("1E+10", 1E+10, 0);
  TEST_DOUBLE("1E-10", 1E-10, 0);
  TEST_DOUBLE("-1E10", -1E10, 0);
  TEST_DOUBLE("-1e10", -1e10, 0);
  TEST_DOUBLE("-1E+10", -1E+10, 0);
  TEST_DOUBLE("-1E-10", -1E-10, 0);
  TEST_DOUBLE("1.234E+10", 1.234E+10, 0);
  TEST_DOUBLE("1.234E-10", 1.234E-10, 0);
  TEST_DOUBLE("1.79769e+308", 1.79769e+308, 0);
  TEST_DOUBLE("2.22507e-308", 2.22507e-308, 0);
  TEST_DOUBLE("-1.79769e+308", -1.79769e+308, 0);
  TEST_DOUBLE("-2.22507e-308", -2.22507e-308, 0);
  TEST_DOUBLE("4.9406564584124654e-324", 4.9406564584124654e-324, 0);
  TEST_DOUBLE("2.2250738585072009e-308", 2.2250738585072009e-308, 0);
  TEST_DOUBLE("2.2250738585072014e-308", 2.2250738585072014e-308, 0);
  TEST_DOUBLE("1.7976931348623157e+308", 1.7976931348623157e+308, 0);
  TEST_DOUBLE("1e-10000", 0.0, 0);
  TEST_DOUBLE("18446744073709551616", 18446744073709551616.0, 0);

  TEST_DOUBLE("-9223372036854775809", -9223372036854775809.0, 0);

  TEST_DOUBLE("0.9868011474609375", 0.9868011474609375, 0);
  TEST_DOUBLE("123e34", 123e34, 0);
  TEST_DOUBLE("45913141877270640000.0", 45913141877270640000.0, 0);
  TEST_DOUBLE("2.2250738585072011e-308", 2.2250738585072011e-308, 0);
  TEST_DOUBLE("1e-214748363", 0.0, 0);
  TEST_DOUBLE("1e-214748364", 0.0, 0);
  TEST_DOUBLE("0.017976931348623157e+310, 1", 1.7976931348623157e+308, ',');

  TEST_DOUBLE("2.2250738585072012e-308", 2.2250738585072014e-308, 0);
  TEST_DOUBLE("2.22507385850720113605740979670913197593481954635164565e-308",
              2.2250738585072014e-308, 0);

  TEST_DOUBLE("0.999999999999999944488848768742172978818416595458984375", 1.0,
              0);
  TEST_DOUBLE("0.999999999999999944488848768742172978818416595458984376", 1.0,
              0);
  TEST_DOUBLE("1.00000000000000011102230246251565404236316680908203125", 1.0,
              0);
  TEST_DOUBLE("1.00000000000000011102230246251565404236316680908203124", 1.0,
              0);

  TEST_DOUBLE("72057594037927928.0", 72057594037927928.0, 0);
  TEST_DOUBLE("72057594037927936.0", 72057594037927936.0, 0);
  TEST_DOUBLE("72057594037927932.0", 72057594037927936.0, 0);
  TEST_DOUBLE("7205759403792793200001e-5", 72057594037927936.0, 0);

  TEST_DOUBLE("9223372036854774784.0", 9223372036854774784.0, 0);
  TEST_DOUBLE("9223372036854775808.0", 9223372036854775808.0, 0);
  TEST_DOUBLE("9223372036854775296.0", 9223372036854775808.0, 0);
  TEST_DOUBLE("922337203685477529600001e-5", 9223372036854775808.0, 0);

  TEST_DOUBLE("10141204801825834086073718800384",
              10141204801825834086073718800384.0, 0);
  TEST_DOUBLE("10141204801825835211973625643008",
              10141204801825835211973625643008.0, 0);
  TEST_DOUBLE("10141204801825834649023672221696",
              10141204801825835211973625643008.0, 0);
  TEST_DOUBLE("1014120480182583464902367222169600001e-5",
              10141204801825835211973625643008.0, 0);

  TEST_DOUBLE("5708990770823838890407843763683279797179383808",
              5708990770823838890407843763683279797179383808.0, 0);
  TEST_DOUBLE("5708990770823839524233143877797980545530986496",
              5708990770823839524233143877797980545530986496.0, 0);
  TEST_DOUBLE("5708990770823839207320493820740630171355185152",
              5708990770823839524233143877797980545530986496.0, 0);
  TEST_DOUBLE("5708990770823839207320493820740630171355185152001e-3",
              5708990770823839524233143877797980545530986496.0, 0);
#undef TEST_DOUBLE
#if !DEBUG
  {
    clock_t start, stop;
    memcpy(buffer, "1234567890.123", 14);
    buffer[14] = 0;
    size_t r = 0;
    start = clock();
    for (int i = 0; i < (TEST_REPEAT << 3); ++i) {
      char *pos = buffer;
      r += fio_atol(&pos);
      __asm__ volatile("" ::: "memory");
      // FIO_T_ASSERT(r == exp, "fio_atol failed during speed test");
    }
    stop = clock();
    fprintf(stderr, "* fio_atol speed test completed in %zu cycles\n",
            stop - start);
    r = 0;
    start = clock();
    for (int i = 0; i < (TEST_REPEAT << 3); ++i) {
      char *pos = buffer;
      r += strtol(pos, NULL, 10);
      __asm__ volatile("" ::: "memory");
      // FIO_T_ASSERT(r == exp, "system strtol failed during speed test");
    }
    stop = clock();
    fprintf(stderr, "* system atol speed test completed in %zu cycles\n",
            stop - start);
  }
#endif /* !DEBUG */
#endif /* FIO_ATOF_ALT */
}

/* *****************************************************************************
Bit-Byte operations - test
***************************************************************************** */

TEST_FUNC void fio___dynamic_types_test___bitwise(void) {
  fprintf(stderr, "* Testing fio_bswapX macros.\n");
  FIO_T_ASSERT(fio_bswap16(0x0102) == (uint16_t)0x0201, "fio_bswap16 failed");
  FIO_T_ASSERT(fio_bswap32(0x01020304) == (uint32_t)0x04030201,
               "fio_bswap32 failed");
  FIO_T_ASSERT(fio_bswap64(0x0102030405060708ULL) == 0x0807060504030201ULL,
               "fio_bswap64 failed");

  fprintf(stderr, "* Testing fio_lrotX and fio_rrotX macros.\n");
  {
    uint64_t tmp = 1;
    tmp = FIO_RROT(tmp, 1);
    __asm__ volatile("" ::: "memory");
    FIO_T_ASSERT(tmp == ((uint64_t)1 << ((sizeof(uint64_t) << 3) - 1)),
                 "fio_rrot failed");
    tmp = FIO_LROT(tmp, 3);
    __asm__ volatile("" ::: "memory");
    FIO_T_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot failed");
    tmp = 1;
    tmp = fio_rrot32(tmp, 1);
    __asm__ volatile("" ::: "memory");
    FIO_T_ASSERT(tmp == ((uint64_t)1 << 31), "fio_rrot32 failed");
    tmp = fio_lrot32(tmp, 3);
    __asm__ volatile("" ::: "memory");
    FIO_T_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot32 failed");
    tmp = 1;
    tmp = fio_rrot64(tmp, 1);
    __asm__ volatile("" ::: "memory");
    FIO_T_ASSERT(tmp == ((uint64_t)1 << 63), "fio_rrot64 failed");
    tmp = fio_lrot64(tmp, 3);
    __asm__ volatile("" ::: "memory");
    FIO_T_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot64 failed");
  }

  fprintf(stderr, "* Testing fio_buf2uX and fio_u2bufX helpers.\n");
  char buffer[32];
  for (int64_t i = -TEST_REPEAT; i < TEST_REPEAT; ++i) {
    FIO_NAME2(fio_u, buf64)(buffer, i);
    __asm__ volatile("" ::: "memory");
    FIO_T_ASSERT((int64_t)FIO_NAME2(fio_buf, u64)(buffer) == i,
                 "fio_u2buf64 / fio_buf2u64  mismatch %zd != %zd",
                 (ssize_t)FIO_NAME2(fio_buf, u64)(buffer), (ssize_t)i);
  }
  for (int32_t i = -TEST_REPEAT; i < TEST_REPEAT; ++i) {
    FIO_NAME2(fio_u, buf32)(buffer, i);
    __asm__ volatile("" ::: "memory");
    FIO_T_ASSERT((int32_t)FIO_NAME2(fio_buf, u32)(buffer) == i,
                 "fio_u2buf32 / fio_buf2u32  mismatch %zd != %zd",
                 (ssize_t)(FIO_NAME2(fio_buf, u32)(buffer)), (ssize_t)i);
  }
  for (int16_t i = -TEST_REPEAT; i < TEST_REPEAT; ++i) {
    FIO_NAME2(fio_u, buf16)(buffer, i);
    __asm__ volatile("" ::: "memory");
    FIO_T_ASSERT((int16_t)FIO_NAME2(fio_buf, u16)(buffer) == i,
                 "fio_u2buf16 / fio_buf2u16  mismatch %zd != %zd",
                 (ssize_t)(FIO_NAME2(fio_buf, u16)(buffer)), (ssize_t)i);
  }

  fprintf(stderr, "* Testing constant-time helpers.\n");
  FIO_T_ASSERT(fio_ct_true(0) == 0, "fio_ct_true(0) should be zero!");
  for (uintptr_t i = 1; i; i <<= 1) {
    FIO_T_ASSERT(fio_ct_true(i) == 1, "fio_ct_true(%p) should be true!",
                 (void *)i);
  }
  for (uintptr_t i = 1; i + 1 != 0; i = (i << 1) | 1) {
    FIO_T_ASSERT(fio_ct_true(i) == 1, "fio_ct_true(%p) should be true!",
                 (void *)i);
  }
  FIO_T_ASSERT(fio_ct_true((~0ULL)) == 1, "fio_ct_true(%p) should be true!",
               (void *)(~0ULL));

  FIO_T_ASSERT(fio_ct_false(0) == 1, "fio_ct_false(0) should be true!");
  for (uintptr_t i = 1; i; i <<= 1) {
    FIO_T_ASSERT(fio_ct_false(i) == 0, "fio_ct_false(%p) should be zero!",
                 (void *)i);
  }
  for (uintptr_t i = 1; i + 1 != 0; i = (i << 1) | 1) {
    FIO_T_ASSERT(fio_ct_false(i) == 0, "fio_ct_false(%p) should be zero!",
                 (void *)i);
  }
  FIO_T_ASSERT(fio_ct_false((~0ULL)) == 0, "fio_ct_false(%p) should be zero!",
               (void *)(~0ULL));
  FIO_T_ASSERT(fio_ct_true(8), "fio_ct_true should be true.");
  FIO_T_ASSERT(!fio_ct_true(0), "fio_ct_true should be false.");
  FIO_T_ASSERT(!fio_ct_false(8), "fio_ct_false should be false.");
  FIO_T_ASSERT(fio_ct_false(0), "fio_ct_false should be true.");
  FIO_T_ASSERT(fio_ct_if_bool(0, 1, 2) == 2,
               "fio_ct_if_bool selection error (false).");
  FIO_T_ASSERT(fio_ct_if_bool(1, 1, 2) == 1,
               "fio_ct_if_bool selection error (true).");
  FIO_T_ASSERT(fio_ct_if(0, 1, 2) == 2, "fio_ct_if selection error (false).");
  FIO_T_ASSERT(fio_ct_if(8, 1, 2) == 1, "fio_ct_if selection error (true).");
  {
    uint8_t bitmap[1024];
    memset(bitmap, 0, 1024);
    fprintf(stderr, "* Testing bitmap helpers.\n");
    FIO_T_ASSERT(!fio_bitmap_get(bitmap, 97), "fio_bitmap_get should be 0.");
    fio_bitmap_set(bitmap, 97);
    FIO_T_ASSERT(fio_bitmap_get(bitmap, 97) == 1,
                 "fio_bitmap_get should be 1 after being set");
    FIO_T_ASSERT(!fio_bitmap_get(bitmap, 96),
                 "other bits shouldn't be effected by set.");
    FIO_T_ASSERT(!fio_bitmap_get(bitmap, 98),
                 "other bits shouldn't be effected by set.");
    fio_bitmap_flip(bitmap, 96);
    fio_bitmap_flip(bitmap, 97);
    FIO_T_ASSERT(!fio_bitmap_get(bitmap, 97),
                 "fio_bitmap_get should be 0 after flip.");
    FIO_T_ASSERT(fio_bitmap_get(bitmap, 96) == 1,
                 "other bits shouldn't be effected by flip");
    fio_bitmap_unset(bitmap, 96);
    fio_bitmap_flip(bitmap, 97);
    FIO_T_ASSERT(!fio_bitmap_get(bitmap, 96),
                 "fio_bitmap_get should be 0 after unset.");
    FIO_T_ASSERT(fio_bitmap_get(bitmap, 97) == 1,
                 "other bits shouldn't be effected by unset");
    fio_bitmap_unset(bitmap, 96);
  }
  {
    fprintf(stderr, "* Testing popcount and hemming distance calculation.\n");
    for (int i = 0; i < 64; ++i) {
      FIO_T_ASSERT(fio_popcount((uint64_t)1 << i) == 1,
                   "fio_popcount error for 1 bit");
    }
    for (int i = 0; i < 63; ++i) {
      FIO_T_ASSERT(fio_popcount((uint64_t)3 << i) == 2,
                   "fio_popcount error for 2 bits");
    }
    for (int i = 0; i < 62; ++i) {
      FIO_T_ASSERT(fio_popcount((uint64_t)7 << i) == 3,
                   "fio_popcount error for 3 bits");
    }
    for (int i = 0; i < 59; ++i) {
      FIO_T_ASSERT(fio_popcount((uint64_t)21 << i) == 3,
                   "fio_popcount error for 3 alternating bits");
    }
    for (int i = 0; i < 64; ++i) {
      FIO_T_ASSERT(fio_hemming_dist(((uint64_t)1 << i) - 1, 0) == i,
                   "fio_hemming_dist error at %d", i);
    }
  }
  {
    struct test_s {
      int a;
      char force_padding;
      int b;
    } stst = {.a = 1};
    struct test_s *stst_p = FIO_PTR_FROM_FIELD(struct test_s, b, &stst.b);
    FIO_T_ASSERT(stst_p == &stst,
                 "FIO_PTR_FROM_FIELD failed to retrace pointer");
  }
}

/* *****************************************************************************
Psedo Random Generator - test
***************************************************************************** */

TEST_FUNC void fio___dynamic_types_test___random_buffer(uint64_t *stream,
                                                        size_t len,
                                                        const char *name,
                                                        size_t clk) {
  size_t totals[2] = {0};
  size_t freq[256] = {0};
  const size_t total_bits = (len * sizeof(*stream) * 8);
  uint64_t hemming = 0;
  /* collect data */
  for (size_t i = 1; i < len; i += 2) {
    hemming += fio_hemming_dist(stream[i], stream[i - 1]);
    for (size_t byte = 0; byte < (sizeof(*stream) << 1); ++byte) {
      uint8_t val = ((uint8_t *)(stream + (i - 1)))[byte];
      ++freq[val];
      for (int bit = 0; bit < 8; ++bit) {
        ++totals[(val >> bit) & 1];
      }
    }
  }
  hemming /= len;
  fprintf(stderr, "\n");
#if DEBUG
  fprintf(stderr, "\t- \x1B[1m%s\x1B[0m (%zu CPU cycles NOT OPTIMIZED):\n",
          name, clk);
#else
  fprintf(stderr, "\t- \x1B[1m%s\x1B[0m (%zu CPU cycles):\n", name, clk);
#endif
  fprintf(stderr, "\t  zeros / ones (bit frequency)\t%.05f\n",
          ((float)1.0 * totals[0]) / totals[1]);
  FIO_T_ASSERT(totals[0] < totals[1] + (total_bits / 20) &&
                   totals[1] < totals[0] + (total_bits / 20),
               "randomness isn't random?");
  fprintf(stderr, "\t  avarage hemming distance\t%zu\n", (size_t)hemming);
  /* expect avarage hemming distance of 25% == 16 bits */
  FIO_T_ASSERT(hemming >= 14 && hemming <= 18,
               "randomness isn't random (hemming distance failed)?");
  /* test chi-square ... I think */
  if (len * sizeof(*stream) > 2560) {
    double n_r = (double)1.0 * ((len * sizeof(*stream)) / 256);
    double chi_square = 0;
    for (unsigned int i = 0; i < 256; ++i) {
      double f = freq[i] - n_r;
      chi_square += (f * f);
    }
    chi_square /= n_r;
    double chi_square_r_abs =
        (chi_square - 256 >= 0) ? chi_square - 256 : (256 - chi_square);
    fprintf(
        stderr, "\t  chi-sq. variation\t\t%.02lf - %s (expect <= %0.2lf)\n",
        chi_square_r_abs,
        ((chi_square_r_abs <= 2 * (sqrt(n_r)))
             ? "good"
             : ((chi_square_r_abs <= 3 * (sqrt(n_r))) ? "not amazing"
                                                      : "\x1B[1mBAD\x1B[0m")),
        2 * (sqrt(n_r)));
  }
}

TEST_FUNC void fio___dynamic_types_test___random(void) {
  fprintf(stderr, "* Testing randomness "
                  "- bit frequency / hemming distance / chi-square.\n");
  const size_t test_len = (TEST_REPEAT << 7);
  uint64_t *rs = (uint64_t *)FIO_MEM_CALLOC(sizeof(*rs), test_len);
  clock_t start, end;
  FIO_ASSERT_ALLOC(rs);

  rand(); /* warmup */
  if (sizeof(int) < sizeof(uint64_t)) {
    start = clock();
    for (size_t i = 0; i < test_len; ++i) {
      rs[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
    end = clock();
  } else {
    start = clock();
    for (size_t i = 0; i < test_len; ++i) {
      rs[i] = (uint64_t)rand();
    }
    end = clock();
  }
  fio___dynamic_types_test___random_buffer(
      rs, test_len, "rand (system - naive, ignoring missing bits)",
      end - start);

  memset(rs, 0, sizeof(*rs) * test_len);
  {
    if (RAND_MAX == ~(uint64_t)0ULL) {
      /* RAND_MAX fills all bits */
      start = clock();
      for (size_t i = 0; i < test_len; ++i) {
        rs[i] = (uint64_t)rand();
      }
      end = clock();
    } else if (RAND_MAX >= (~(uint32_t)0UL)) {
      /* RAND_MAX fill at least 32 bits per call */
      uint32_t *rs_adjusted = (uint32_t *)rs;
      start = clock();
      for (size_t i = 0; i < (test_len << 1); ++i) {
        rs_adjusted[i] = (uint32_t)rand();
      }
      end = clock();
    } else if (RAND_MAX >= (~(uint16_t)0U)) {
      /* RAND_MAX fill at least 16 bits per call */
      uint16_t *rs_adjusted = (uint16_t *)rs;
      start = clock();
      for (size_t i = 0; i < (test_len << 2); ++i) {
        rs_adjusted[i] = (uint16_t)rand();
      }
      end = clock();
    } else {
      /* assume RAND_MAX fill at least 8 bits per call */
      uint8_t *rs_adjusted = (uint8_t *)rs;
      start = clock();
      for (size_t i = 0; i < (test_len << 2); ++i) {
        rs_adjusted[i] = (uint8_t)rand();
      }
      end = clock();
    }
    /* test RAND_MAX value */
    uint8_t rand_bits = 63;
    while (rand_bits) {
      if (RAND_MAX <= (~(0ULL)) >> rand_bits)
        break;
      --rand_bits;
    }
    rand_bits = 64 - rand_bits;
    char buffer[128] = {0};
    snprintf(buffer, 128 - 14, "rand (system - fixed, testing %d random bits)",
             (int)rand_bits);
    fio___dynamic_types_test___random_buffer(rs, test_len, buffer, end - start);
  }

  memset(rs, 0, sizeof(*rs) * test_len);
  fio_rand64(); /* warmup */
  start = clock();
  for (size_t i = 0; i < test_len; ++i) {
    rs[i] = fio_rand64();
  }
  end = clock();
  fio___dynamic_types_test___random_buffer(rs, test_len, "fio_rand64",
                                           end - start);
  memset(rs, 0, sizeof(*rs) * test_len);
  start = clock();
  fio_rand_bytes(rs, test_len * sizeof(*rs));
  end = clock();
  fio___dynamic_types_test___random_buffer(rs, test_len, "fio_rand_bytes",
                                           end - start);

  fio_rand_feed2seed(rs, sizeof(*rs) * test_len);
  FIO_MEM_FREE(rs, sizeof(*rs) * test_len);
  fprintf(stderr, "\n");
#if DEBUG
  fprintf(stderr,
          "\t- to compare CPU cycles, test randomness with optimization.\n\n");
#endif
}

/* *****************************************************************************
Atomic operations - test
***************************************************************************** */

TEST_FUNC void fio___dynamic_types_test___atomic(void) {
  fprintf(stderr, "* Testing atomic operation macros.\n");
  struct fio___atomic_test_s {
    size_t w;
    unsigned long l;
    unsigned short s;
    unsigned char c;
  } s = {0}, r1 = {0}, r2 = {0};
  fio_lock_i lock = FIO_LOCK_INIT;

  r1.c = fio_atomic_add(&s.c, 1);
  r1.s = fio_atomic_add(&s.s, 1);
  r1.l = fio_atomic_add(&s.l, 1);
  r1.w = fio_atomic_add(&s.w, 1);
  FIO_T_ASSERT(r1.c == 0 && s.c == 1, "fio_atomic_add failed for c");
  FIO_T_ASSERT(r1.s == 0 && s.s == 1, "fio_atomic_add failed for s");
  FIO_T_ASSERT(r1.l == 0 && s.l == 1, "fio_atomic_add failed for l");
  FIO_T_ASSERT(r1.w == 0 && s.w == 1, "fio_atomic_add failed for w");
  r2.c = fio_atomic_add_fetch(&s.c, 1);
  r2.s = fio_atomic_add_fetch(&s.s, 1);
  r2.l = fio_atomic_add_fetch(&s.l, 1);
  r2.w = fio_atomic_add_fetch(&s.w, 1);
  FIO_T_ASSERT(r2.c == 2 && s.c == 2, "fio_atomic_add_fetch failed for c");
  FIO_T_ASSERT(r2.s == 2 && s.s == 2, "fio_atomic_add_fetch failed for s");
  FIO_T_ASSERT(r2.l == 2 && s.l == 2, "fio_atomic_add_fetch failed for l");
  FIO_T_ASSERT(r2.w == 2 && s.w == 2, "fio_atomic_add_fetch failed for w");
  r1.c = fio_atomic_sub(&s.c, 1);
  r1.s = fio_atomic_sub(&s.s, 1);
  r1.l = fio_atomic_sub(&s.l, 1);
  r1.w = fio_atomic_sub(&s.w, 1);
  FIO_T_ASSERT(r1.c == 2 && s.c == 1, "fio_atomic_sub failed for c");
  FIO_T_ASSERT(r1.s == 2 && s.s == 1, "fio_atomic_sub failed for s");
  FIO_T_ASSERT(r1.l == 2 && s.l == 1, "fio_atomic_sub failed for l");
  FIO_T_ASSERT(r1.w == 2 && s.w == 1, "fio_atomic_sub failed for w");
  r2.c = fio_atomic_sub_fetch(&s.c, 1);
  r2.s = fio_atomic_sub_fetch(&s.s, 1);
  r2.l = fio_atomic_sub_fetch(&s.l, 1);
  r2.w = fio_atomic_sub_fetch(&s.w, 1);
  FIO_T_ASSERT(r2.c == 0 && s.c == 0, "fio_atomic_sub_fetch failed for c");
  FIO_T_ASSERT(r2.s == 0 && s.s == 0, "fio_atomic_sub_fetch failed for s");
  FIO_T_ASSERT(r2.l == 0 && s.l == 0, "fio_atomic_sub_fetch failed for l");
  FIO_T_ASSERT(r2.w == 0 && s.w == 0, "fio_atomic_sub_fetch failed for w");
  fio_atomic_add(&s.c, 1);
  fio_atomic_add(&s.s, 1);
  fio_atomic_add(&s.l, 1);
  fio_atomic_add(&s.w, 1);
  r1.c = fio_atomic_exchange(&s.c, 99);
  r1.s = fio_atomic_exchange(&s.s, 99);
  r1.l = fio_atomic_exchange(&s.l, 99);
  r1.w = fio_atomic_exchange(&s.w, 99);
  FIO_T_ASSERT(r1.c == 1 && s.c == 99, "fio_atomic_exchange failed for c");
  FIO_T_ASSERT(r1.s == 1 && s.s == 99, "fio_atomic_exchange failed for s");
  FIO_T_ASSERT(r1.l == 1 && s.l == 99, "fio_atomic_exchange failed for l");
  FIO_T_ASSERT(r1.w == 1 && s.w == 99, "fio_atomic_exchange failed for w");
  // clang-format off
  FIO_T_ASSERT(!fio_atomic_compare_exchange_p(&s.c, &r1.c, &r1.c), "fio_atomic_compare_exchange_p didn't fail for c");
  FIO_T_ASSERT(!fio_atomic_compare_exchange_p(&s.s, &r1.s, &r1.s), "fio_atomic_compare_exchange_p didn't fail for s");
  FIO_T_ASSERT(!fio_atomic_compare_exchange_p(&s.l, &r1.l, &r1.l), "fio_atomic_compare_exchange_p didn't fail for l");
  FIO_T_ASSERT(!fio_atomic_compare_exchange_p(&s.w, &r1.w, &r1.w), "fio_atomic_compare_exchange_p didn't fail for w");
  r1.c = 1;s.c = 99; r1.s = 1;s.s = 99; r1.l = 1;s.l = 99; r1.w = 1;s.w = 99; /* ignore system spefcific behavior. */
  r1.c = fio_atomic_compare_exchange_p(&s.c,&s.c, &r1.c);
  r1.s = fio_atomic_compare_exchange_p(&s.s,&s.s, &r1.s);
  r1.l = fio_atomic_compare_exchange_p(&s.l,&s.l, &r1.l);
  r1.w = fio_atomic_compare_exchange_p(&s.w,&s.w, &r1.w);
  FIO_T_ASSERT(r1.c == 1 && s.c == 1, "fio_atomic_compare_exchange_p failed for c");
  FIO_T_ASSERT(r1.s == 1 && s.s == 1, "fio_atomic_compare_exchange_p failed for s");
  FIO_T_ASSERT(r1.l == 1 && s.l == 1, "fio_atomic_compare_exchange_p failed for l");
  FIO_T_ASSERT(r1.w == 1 && s.w == 1, "fio_atomic_compare_exchange_p failed for w");
  // clang-format on

  uint64_t val = 1;
  FIO_T_ASSERT(fio_atomic_and(&val, 2) == 1,
               "fio_atomic_and should return old value");
  FIO_T_ASSERT(val == 0, "fio_atomic_and should update value");
  FIO_T_ASSERT(fio_atomic_xor(&val, 1) == 0,
               "fio_atomic_xor should return old value");
  FIO_T_ASSERT(val == 1, "fio_atomic_xor_fetch should update value");
  FIO_T_ASSERT(fio_atomic_xor_fetch(&val, 1) == 0,
               "fio_atomic_xor_fetch should return new value");
  FIO_T_ASSERT(val == 0, "fio_atomic_xor should update value");
  FIO_T_ASSERT(fio_atomic_or(&val, 2) == 0,
               "fio_atomic_or should return old value");
  FIO_T_ASSERT(val == 2, "fio_atomic_or should update value");
  FIO_T_ASSERT(fio_atomic_or_fetch(&val, 1) == 3,
               "fio_atomic_or_fetch should return new value");
  FIO_T_ASSERT(val == 3, "fio_atomic_or_fetch should update value");
  FIO_T_ASSERT(fio_atomic_nand_fetch(&val, 4) == ~0ULL,
               "fio_atomic_nand_fetch should return new value");
  FIO_T_ASSERT(val == ~0ULL, "fio_atomic_nand_fetch should update value");
  val = 3ULL;
  FIO_T_ASSERT(fio_atomic_nand(&val, 4) == 3ULL,
               "fio_atomic_nand should return old value");
  FIO_T_ASSERT(val == ~0ULL, "fio_atomic_nand_fetch should update value");

  FIO_T_ASSERT(!fio_is_locked(&lock),
               "lock should be initialized in unlocked state");
  FIO_T_ASSERT(!fio_trylock(&lock), "fio_trylock should succeed");
  FIO_T_ASSERT(fio_trylock(&lock), "fio_trylock should fail");
  FIO_T_ASSERT(fio_is_locked(&lock), "lock should be engaged");
  fio_unlock(&lock);
  FIO_T_ASSERT(!fio_is_locked(&lock), "lock should be released");
  fio_lock(&lock);
  FIO_T_ASSERT(fio_is_locked(&lock), "lock should be engaged (fio_lock)");
  for (uint8_t i = 1; i < 8; ++i) {
    FIO_T_ASSERT(!fio_is_sublocked(&lock, i),
                 "sublock flagged, but wasn't engaged (%u - %p)",
                 (unsigned int)i, (void *)(uintptr_t)lock);
  }
  fio_unlock(&lock);
  FIO_T_ASSERT(!fio_is_locked(&lock), "lock should be released");
  lock = FIO_LOCK_INIT;
  for (size_t i = 0; i < 8; ++i) {
    FIO_T_ASSERT(!fio_is_sublocked(&lock, i),
                 "sublock should be initialized in unlocked state");
    FIO_T_ASSERT(!fio_trylock_sublock(&lock, i),
                 "fio_trylock_sublock should succeed");
    FIO_T_ASSERT(fio_trylock_sublock(&lock, i), "fio_trylock should fail");
    FIO_T_ASSERT(fio_trylock_full(&lock), "fio_trylock_full should fail");
    FIO_T_ASSERT(fio_is_sublocked(&lock, i), "lock should be engaged");
    {
      uint8_t g =
          fio_trylock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(3));
      FIO_T_ASSERT((i != 1 && i != 3 && !g) || ((i == 1 || i == 3) && g),
                   "fio_trylock_group should succeed / fail");
      if (!g)
        fio_unlock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(3));
    }
    for (uint8_t j = 1; j < 8; ++j) {
      FIO_T_ASSERT(i == j || !fio_is_sublocked(&lock, j),
                   "another sublock was flagged, though it wasn't engaged");
    }
    FIO_T_ASSERT(fio_is_sublocked(&lock, i), "lock should remain engaged");
    fio_unlock_sublock(&lock, i);
    FIO_T_ASSERT(!fio_is_sublocked(&lock, i), "sublock should be released");
    FIO_T_ASSERT(!fio_trylock_full(&lock), "fio_trylock_full should succeed");
    fio_unlock_full(&lock);
    FIO_T_ASSERT(!lock, "fio_unlock_full should unlock all");
  }
}

/* *****************************************************************************
Locking - Speed Test
***************************************************************************** */
#define FIO___LOCK2_TEST_TASK (1LU << 25)
#define FIO___LOCK2_TEST_THREADS 32U
#define FIO___LOCK2_TEST_REPEAT 1

#ifndef H___FIO_LOCK2___H
#include <pthread.h>
#endif

FIO_IFUNC void fio___lock_speedtest_task_inner(void *s) {
  size_t *r = (size_t *)s;
  static size_t i;
  for (i = 0; i < FIO___LOCK2_TEST_TASK; ++i) {
    __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
    ++r[0];
  }
}

static void *fio___lock_mytask_lock(void *s) {
  static fio_lock_i lock = FIO_LOCK_INIT;
  fio_lock(&lock);
  if (s)
    fio___lock_speedtest_task_inner(s);
  fio_unlock(&lock);
  return NULL;
}

#ifdef H___FIO_LOCK2___H
static void *fio___lock_mytask_lock2(void *s) {
  static fio_lock2_s lock = {FIO_LOCK_INIT};
  fio_lock2(&lock, 1);
  if (s)
    fio___lock_speedtest_task_inner(s);
  fio_unlock2(&lock, 1);
  return NULL;
}
#endif

static void *fio___lock_mytask_mutex(void *s) {
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&mutex);
  if (s)
    fio___lock_speedtest_task_inner(s);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

TEST_FUNC void fio___dynamic_types_test___lock2_speed(void) {
  uint64_t start, end;
  pthread_t threads[FIO___LOCK2_TEST_THREADS];

  struct {
    size_t type_size;
    const char *type_name;
    const char *name;
    void *(*task)(void *);
  } test_funcs[] = {
      {
          .type_size = sizeof(fio_lock_i),
          .type_name = "fio_lock_i",
          .name = "fio_lock      (spinlock)",
          .task = fio___lock_mytask_lock,
      },
#ifdef H___FIO_LOCK2___H
      {
          .type_size = sizeof(fio_lock2_s),
          .type_name = "fio_lock2_s",
          .name = "fio_lock2 (pause/resume)",
          .task = fio___lock_mytask_lock2,
      },
#endif
      {
          .type_size = sizeof(pthread_mutex_t),
          .type_name = "pthread_mutex_t",
          .name = "pthreads (pthread_mutex)",
          .task = fio___lock_mytask_mutex,
      },
      {
          .name = NULL,
          .task = NULL,
      },
  };
  fprintf(stderr, "* Speed testing The following types:\n");
  for (size_t fn = 0; test_funcs[fn].name; ++fn) {
    fprintf(stderr, "\t%s\t(%zu bytes)\n", test_funcs[fn].type_name,
            test_funcs[fn].type_size);
  }
#ifndef H___FIO_LOCK2___H
  FIO_LOG_WARNING("Won't test `fio_lock2` functions (needs `FIO_LOCK2`).");
#endif

  start = fio_time_micro();
  for (size_t i = 0; i < FIO___LOCK2_TEST_TASK; ++i) {
    __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
  }
  end = fio_time_micro();
  fprintf(stderr,
          "\n* Speed testing locking schemes - no contention, short work (%zu "
          "mms):\n"
          "\t\t(%zu itterations)\n",
          (size_t)(end - start), (size_t)FIO___LOCK2_TEST_TASK);

  for (int test_repeat = 0; test_repeat < FIO___LOCK2_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK2_TEST_REPEAT > 1)
      fprintf(stderr, "%s (%d)\n", (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      test_funcs[fn].task(NULL); /* warmup */
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK2_TEST_TASK; ++i) {
        __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
        test_funcs[fn].task(NULL);
      }
      end = fio_time_micro();
      fprintf(stderr, "\t%s: %zu mms\n", test_funcs[fn].name,
              (size_t)(end - start));
    }
  }

  fprintf(stderr,
          "\n* Speed testing locking schemes - no contention, long work ");
  start = fio_time_micro();
  for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
    size_t result = 0;
    __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
    fio___lock_speedtest_task_inner(&result);
  }
  end = fio_time_micro();
  fprintf(stderr, " %zu mms\n", (size_t)(end - start));
  clock_t long_work = end - start;
  fprintf(stderr, "(%zu mms):\n", long_work);
  for (int test_repeat = 0; test_repeat < FIO___LOCK2_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK2_TEST_REPEAT > 1)
      fprintf(stderr, "%s (%d)\n", (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
        __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
        test_funcs[fn].task(&result);
      }
      end = fio_time_micro();
      fprintf(stderr, "\t%s: %zu mms (%zu mms)\n", test_funcs[fn].name,
              (size_t)(end - start), (size_t)(end - (start + long_work)));
      FIO_T_ASSERT(result == (FIO___LOCK2_TEST_TASK * FIO___LOCK2_TEST_THREADS),
                   "%s final result error.", test_funcs[fn].name);
    }
  }

  fprintf(stderr,
          "\n* Speed testing locking schemes - %zu threads, long work (%zu "
          "mms):\n",
          (size_t)FIO___LOCK2_TEST_THREADS, long_work);
  for (int test_repeat = 0; test_repeat < FIO___LOCK2_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK2_TEST_REPEAT > 1)
      fprintf(stderr, "%s (%d)\n", (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
        pthread_create(threads + i, NULL, test_funcs[fn].task, &result);
      }
      for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
        pthread_join(threads[i], NULL);
      }
      end = fio_time_micro();
      fprintf(stderr, "\t%s: %zu mms (%zu mms)\n", test_funcs[fn].name,
              (size_t)(end - start), (size_t)(end - (start + long_work)));
      FIO_T_ASSERT(result == (FIO___LOCK2_TEST_TASK * FIO___LOCK2_TEST_THREADS),
                   "%s final result error.", test_funcs[fn].name);
    }
  }
}
/* *****************************************************************************
URL parsing - Test
***************************************************************************** */

#define FIO_URL
#include __FILE__

/* Test for URI variations:
 *
 * * `/complete_path?query#target`
 *
 *   i.e.: /index.html?page=1#list
 *
 * * `host:port/complete_path?query#target`
 *
 *   i.e.:
 *      example.com
 *      example.com:8080
 *      example.com/index.html
 *      example.com:8080/index.html
 *      example.com:8080/index.html?key=val#target
 *
 * * `user:password@host:port/path?query#target`
 *
 *   i.e.: user:1234@example.com:8080/index.html
 *
 * * `username[:password]@host[:port][...]`
 *
 *   i.e.: john:1234@example.com
 *
 * * `schema://user:password@host:port/path?query#target`
 *
 *   i.e.: http://example.com/index.html?page=1#list
 */
TEST_FUNC void fio___dynamic_types_test___url(void) {
  fprintf(stderr, "* Testing URL (URI) parser.\n");
  struct {
    char *url;
    size_t len;
    fio_url_s expected;
  } tests[] = {
      {
          .url = (char *)"file://go/home/",
          .len = 15,
          .expected =
              {
                  .scheme = {.buf = (char *)"file", .len = 4},
                  .path = {.buf = (char *)"go/home/", .len = 8},
              },
      },
      {
          .url = (char *)"unix:///go/home/",
          .len = 16,
          .expected =
              {
                  .scheme = {.buf = (char *)"unix", .len = 4},
                  .path = {.buf = (char *)"/go/home/", .len = 9},
              },
      },
      {
          .url = (char *)"schema://user:password@host:port/path?query#target",
          .len = 50,
          .expected =
              {
                  .scheme = {.buf = (char *)"schema", .len = 6},
                  .user = {.buf = (char *)"user", .len = 4},
                  .password = {.buf = (char *)"password", .len = 8},
                  .host = {.buf = (char *)"host", .len = 4},
                  .port = {.buf = (char *)"port", .len = 4},
                  .path = {.buf = (char *)"/path", .len = 5},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"schema://user@host:port/path?query#target",
          .len = 41,
          .expected =
              {
                  .scheme = {.buf = (char *)"schema", .len = 6},
                  .user = {.buf = (char *)"user", .len = 4},
                  .host = {.buf = (char *)"host", .len = 4},
                  .port = {.buf = (char *)"port", .len = 4},
                  .path = {.buf = (char *)"/path", .len = 5},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"http://localhost.com:3000/home?is=1",
          .len = 35,
          .expected =
              {
                  .scheme = {.buf = (char *)"http", .len = 4},
                  .host = {.buf = (char *)"localhost.com", .len = 13},
                  .port = {.buf = (char *)"3000", .len = 4},
                  .path = {.buf = (char *)"/home", .len = 5},
                  .query = {.buf = (char *)"is=1", .len = 4},
              },
      },
      {
          .url = (char *)"/complete_path?query#target",
          .len = 27,
          .expected =
              {
                  .path = {.buf = (char *)"/complete_path", .len = 14},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"/index.html?page=1#list",
          .len = 23,
          .expected =
              {
                  .path = {.buf = (char *)"/index.html", .len = 11},
                  .query = {.buf = (char *)"page=1", .len = 6},
                  .target = {.buf = (char *)"list", .len = 4},
              },
      },
      {
          .url = (char *)"example.com",
          .len = 11,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
              },
      },

      {
          .url = (char *)"example.com:8080",
          .len = 16,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
              },
      },
      {
          .url = (char *)"example.com/index.html",
          .len = 22,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {
          .url = (char *)"example.com:8080/index.html",
          .len = 27,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {
          .url = (char *)"example.com:8080/index.html?key=val#target",
          .len = 42,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
                  .query = {.buf = (char *)"key=val", .len = 7},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"user:1234@example.com:8080/index.html",
          .len = 37,
          .expected =
              {
                  .user = {.buf = (char *)"user", .len = 4},
                  .password = {.buf = (char *)"1234", .len = 4},
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {
          .url = (char *)"user@example.com:8080/index.html",
          .len = 32,
          .expected =
              {
                  .user = {.buf = (char *)"user", .len = 4},
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {.url = NULL},
  };
  for (size_t i = 0; tests[i].url; ++i) {
    fio_url_s result = fio_url_parse(tests[i].url, tests[i].len);
    if (0) {
      fprintf(stderr,
              "Result for: %s"
              "\n\tscheme (%zu):\t\t%.*s"
              "\n\tuser (%zu):\t\t%.*s"
              "\n\tpassword (%zu):\t\t%.*s"
              "\n\thost (%zu):\t\t%.*s"
              "\n\tport (%zu):\t\t%.*s"
              "\n\tpath (%zu):\t\t%.*s"
              "\n\tquery (%zu):\t\t%.*s"
              "\n\ttarget (%zu):\t\t%.*s\n",
              tests[i].url, result.scheme.len, (int)result.scheme.len,
              result.scheme.buf, result.user.len, (int)result.user.len,
              result.user.buf, result.password.len, (int)result.password.len,
              result.password.buf, result.host.len, (int)result.host.len,
              result.host.buf, result.port.len, (int)result.port.len,
              result.port.buf, result.path.len, (int)result.path.len,
              result.path.buf, result.query.len, (int)result.query.len,
              result.query.buf, result.target.len, (int)result.target.len,
              result.target.buf);
    }
    FIO_T_ASSERT(result.scheme.len == tests[i].expected.scheme.len &&
                     (!result.scheme.len ||
                      !memcmp(result.scheme.buf, tests[i].expected.scheme.buf,
                              tests[i].expected.scheme.len)),
                 "scheme result failed for:\n\ttest[%zu]: %s\n\texpected: "
                 "%s\n\tgot: %.*s",
                 i, tests[i].url, tests[i].expected.scheme.buf,
                 (int)result.scheme.len, result.scheme.buf);
    FIO_T_ASSERT(
        result.user.len == tests[i].expected.user.len &&
            (!result.user.len ||
             !memcmp(result.user.buf, tests[i].expected.user.buf,
                     tests[i].expected.user.len)),
        "user result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i, tests[i].url, tests[i].expected.user.buf, (int)result.user.len,
        result.user.buf);
    FIO_T_ASSERT(
        result.password.len == tests[i].expected.password.len &&
            (!result.password.len ||
             !memcmp(result.password.buf, tests[i].expected.password.buf,
                     tests[i].expected.password.len)),
        "password result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: "
        "%.*s",
        i, tests[i].url, tests[i].expected.password.buf,
        (int)result.password.len, result.password.buf);
    FIO_T_ASSERT(
        result.host.len == tests[i].expected.host.len &&
            (!result.host.len ||
             !memcmp(result.host.buf, tests[i].expected.host.buf,
                     tests[i].expected.host.len)),
        "host result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i, tests[i].url, tests[i].expected.host.buf, (int)result.host.len,
        result.host.buf);
    FIO_T_ASSERT(
        result.port.len == tests[i].expected.port.len &&
            (!result.port.len ||
             !memcmp(result.port.buf, tests[i].expected.port.buf,
                     tests[i].expected.port.len)),
        "port result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i, tests[i].url, tests[i].expected.port.buf, (int)result.port.len,
        result.port.buf);
    FIO_T_ASSERT(
        result.path.len == tests[i].expected.path.len &&
            (!result.path.len ||
             !memcmp(result.path.buf, tests[i].expected.path.buf,
                     tests[i].expected.path.len)),
        "path result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i, tests[i].url, tests[i].expected.path.buf, (int)result.path.len,
        result.path.buf);
    FIO_T_ASSERT(result.query.len == tests[i].expected.query.len &&
                     (!result.query.len ||
                      !memcmp(result.query.buf, tests[i].expected.query.buf,
                              tests[i].expected.query.len)),
                 "query result failed for:\n\ttest[%zu]: %s\n\texpected: "
                 "%s\n\tgot: %.*s",
                 i, tests[i].url, tests[i].expected.query.buf,
                 (int)result.query.len, result.query.buf);
    FIO_T_ASSERT(result.target.len == tests[i].expected.target.len &&
                     (!result.target.len ||
                      !memcmp(result.target.buf, tests[i].expected.target.buf,
                              tests[i].expected.target.len)),
                 "target result failed for:\n\ttest[%zu]: %s\n\texpected: "
                 "%s\n\tgot: %.*s",
                 i, tests[i].url, tests[i].expected.target.buf,
                 (int)result.target.len, result.target.buf);
  }
}

/* *****************************************************************************
Linked List - Test
***************************************************************************** */

typedef struct {
  int data;
  FIO_LIST_NODE node;
} ls____test_s;

#define FIO_LIST_NAME ls____test
#define FIO_PTR_TAG(p) fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p) fio___dynamic_types_test_untag(((uintptr_t)p))

#include __FILE__

TEST_FUNC void fio___dynamic_types_test___linked_list_test(void) {
  fprintf(stderr, "* Testing linked lists.\n");
  FIO_LIST_HEAD ls = FIO_LIST_INIT(ls);
  for (int i = 0; i < TEST_REPEAT; ++i) {
    ls____test_s *node =
        ls____test_push(&ls, (ls____test_s *)FIO_MEM_CALLOC(sizeof(*node), 1));
    node->data = i;
  }
  int tester = 0;
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    FIO_T_ASSERT(pos->data == tester++,
                 "Linked list ordering error for push or each");
    FIO_T_ASSERT(ls____test_root(&pos->node) == pos,
                 "Linked List root offset error");
  }
  FIO_T_ASSERT(tester == TEST_REPEAT,
               "linked list EACH didn't loop through all the list");
  while (ls____test_any(&ls)) {
    ls____test_s *node = ls____test_pop(&ls);
    node = (ls____test_s *)fio___dynamic_types_test_untag((uintptr_t)(node));
    FIO_T_ASSERT(node, "Linked list pop or any failed");
    FIO_T_ASSERT(node->data == --tester, "Linked list ordering error for pop");
    FIO_MEM_FREE(node, sizeof(*node));
  }
  tester = TEST_REPEAT;
  for (int i = 0; i < TEST_REPEAT; ++i) {
    ls____test_s *node = ls____test_unshift(
        &ls, (ls____test_s *)FIO_MEM_CALLOC(sizeof(*node), 1));
    node->data = i;
  }
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    FIO_T_ASSERT(pos->data == --tester,
                 "Linked list ordering error for unshift or each");
  }
  FIO_T_ASSERT(
      tester == 0,
      "linked list EACH didn't loop through all the list after unshift");
  tester = TEST_REPEAT;
  while (ls____test_any(&ls)) {
    ls____test_s *node = ls____test_shift(&ls);
    node = (ls____test_s *)fio___dynamic_types_test_untag((uintptr_t)(node));
    FIO_T_ASSERT(node, "Linked list pop or any failed");
    FIO_T_ASSERT(node->data == --tester,
                 "Linked list ordering error for shift");
    FIO_MEM_FREE(node, sizeof(*node));
  }
  FIO_T_ASSERT(FIO_NAME_BL(ls____test, empty)(&ls),
               "Linked list empty should have been true");
  for (int i = 0; i < TEST_REPEAT; ++i) {
    ls____test_s *node =
        ls____test_push(&ls, (ls____test_s *)FIO_MEM_CALLOC(sizeof(*node), 1));
    node->data = i;
  }
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    ls____test_remove(pos);
    pos = (ls____test_s *)fio___dynamic_types_test_untag((uintptr_t)(pos));
    FIO_MEM_FREE(pos, sizeof(*pos));
  }
  FIO_T_ASSERT(FIO_NAME_BL(ls____test, empty)(&ls),
               "Linked list empty should have been true");
}

/* *****************************************************************************
Dynamic Array - Test
***************************************************************************** */

static int ary____test_was_destroyed = 0;
#define FIO_ARRAY_NAME ary____test
#define FIO_ARRAY_TYPE int
#define FIO_REF_NAME ary____test
#define FIO_REF_INIT(obj) obj = (ary____test_s)FIO_ARRAY_INIT
#define FIO_REF_DESTROY(obj)                                                   \
  do {                                                                         \
    ary____test_destroy(&obj);                                                 \
    ary____test_was_destroyed = 1;                                             \
  } while (0)
#define FIO_ATOMIC
#define FIO_PTR_TAG(p) fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p) fio___dynamic_types_test_untag(((uintptr_t)p))
#include __FILE__

#define FIO_ARRAY_NAME ary2____test
#define FIO_ARRAY_TYPE uint8_t
#define FIO_ARRAY_TYPE_INVALID 0xFF
#define FIO_ARRAY_TYPE_COPY(dest, src) (dest) = (src)
#define FIO_ARRAY_TYPE_DESTROY(obj) (obj = FIO_ARRAY_TYPE_INVALID)
#define FIO_ARRAY_TYPE_CMP(a, b) (a) == (b)
#define FIO_PTR_TAG(p) fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p) fio___dynamic_types_test_untag(((uintptr_t)p))
#include __FILE__

static int fio_____dynamic_test_array_task(int o, void *c_) {
  ((size_t *)(c_))[0] += o;
  if (((size_t *)(c_))[0] >= 256)
    return -1;
  return 0;
}

TEST_FUNC void fio___dynamic_types_test___array_test(void) {
  int tmp = 0;
  ary____test_s a = FIO_ARRAY_INIT;
  fprintf(stderr, "* Testing dynamic arrays.\n");

  fprintf(stderr, "* Testing on stack, push/pop.\n");
  /* test stack allocated array (initialization) */
  FIO_T_ASSERT(ary____test_capa(&a) == 0,
               "Freshly initialized array should have zero capacity");
  FIO_T_ASSERT(ary____test_count(&a) == 0,
               "Freshly initialized array should have zero elements");
  memset(&a, 1, sizeof(a));
  a = (ary____test_s)FIO_ARRAY_INIT;
  FIO_T_ASSERT(ary____test_capa(&a) == 0,
               "Reinitialized array should have zero capacity");
  FIO_T_ASSERT(ary____test_count(&a) == 0,
               "Reinitialized array should have zero elements");
  ary____test_push(&a, 1);
  ary____test_push(&a, 2);
  /* test get/set array functions */
  FIO_T_ASSERT(ary____test_get(&a, 1) == 2,
               "`get` by index failed to return correct element.");
  FIO_T_ASSERT(ary____test_get(&a, -1) == 2,
               "last element `get` failed to return correct element.");
  FIO_T_ASSERT(ary____test_get(&a, 0) == 1,
               "`get` by index 0 failed to return correct element.");
  FIO_T_ASSERT(ary____test_get(&a, -2) == 1,
               "last element `get(-2)` failed to return correct element.");
  ary____test_pop(&a, &tmp);
  FIO_T_ASSERT(tmp == 2, "pop failed to set correct element.");
  ary____test_pop(&a, &tmp);
  /* array is now empty */
  ary____test_push(&a, 1);
  ary____test_push(&a, 2);
  ary____test_push(&a, 3);
  ary____test_set(&a, 99, 1, NULL);
  FIO_T_ASSERT(ary____test_count(&a) == 100,
               "set with 100 elements should force create elements.");
  FIO_T_ASSERT(ary____test_get(&a, 0) == 1,
               "Intialized element should be kept (index 0)");
  FIO_T_ASSERT(ary____test_get(&a, 1) == 2,
               "Intialized element should be kept (index 1)");
  FIO_T_ASSERT(ary____test_get(&a, 2) == 3,
               "Intialized element should be kept (index 2)");
  for (int i = 3; i < 99; ++i) {
    FIO_T_ASSERT(ary____test_get(&a, i) == 0,
                 "Unintialized element should be 0");
  }
  ary____test_remove2(&a, 0);
  FIO_T_ASSERT(ary____test_count(&a) == 4,
               "remove2 should have removed all zero elements.");
  FIO_T_ASSERT(ary____test_get(&a, 0) == 1,
               "remove2 should have compacted the array (index 0)");
  FIO_T_ASSERT(ary____test_get(&a, 1) == 2,
               "remove2 should have compacted the array (index 1)");
  FIO_T_ASSERT(ary____test_get(&a, 2) == 3,
               "remove2 should have compacted the array (index 2)");
  FIO_T_ASSERT(ary____test_get(&a, 3) == 1,
               "remove2 should have compacted the array (index 4)");
  tmp = 9;
  ary____test_remove(&a, 0, &tmp);
  FIO_T_ASSERT(tmp == 1, "remove should have copied the value to the pointer.");
  FIO_T_ASSERT(ary____test_count(&a) == 3,
               "remove should have removed an element.");
  FIO_T_ASSERT(ary____test_get(&a, 0) == 2,
               "remove should have compacted the array.");
  /* test stack allocated array (destroy) */
  ary____test_destroy(&a);
  FIO_T_ASSERT(ary____test_capa(&a) == 0,
               "Destroyed array should have zero capacity");
  FIO_T_ASSERT(ary____test_count(&a) == 0,
               "Destroyed array should have zero elements");
  FIO_T_ASSERT(a.ary == NULL,
               "Destroyed array shouldn't have memory allocated");
  ary____test_push(&a, 1);
  ary____test_push(&a, 2);
  ary____test_push(&a, 3);
  ary____test_reserve(&a, 100);
  FIO_T_ASSERT(ary____test_count(&a) == 3,
               "reserve shouldn't effect itme count.");
  FIO_T_ASSERT(ary____test_capa(&a) >= 100, "reserve should reserve.");
  FIO_T_ASSERT(ary____test_get(&a, 0) == 1,
               "Element should be kept after reserve (index 0)");
  FIO_T_ASSERT(ary____test_get(&a, 1) == 2,
               "Element should be kept after reserve (index 1)");
  FIO_T_ASSERT(ary____test_get(&a, 2) == 3,
               "Element should be kept after reserve (index 2)");
  ary____test_compact(&a);
  FIO_T_ASSERT(ary____test_capa(&a) == 3,
               "reserve shouldn't effect itme count.");
  ary____test_destroy(&a);

  /* Round 2 - heap, shift/unshift, negative ary_set index */

  fprintf(stderr, "* Testing on heap, shift/unshift.\n");
  /* test heap allocated array (initialization) */
  ary____test_s *pa = ary____test_new();
  FIO_T_ASSERT(ary____test_capa(pa) == 0,
               "Freshly initialized array should have zero capacity");
  FIO_T_ASSERT(ary____test_count(pa) == 0,
               "Freshly initialized array should have zero elements");
  ary____test_unshift(pa, 2);
  ary____test_unshift(pa, 1);
  /* test get/set/shift/unshift array functions */
  FIO_T_ASSERT(ary____test_get(pa, 1) == 2,
               "`get` by index failed to return correct element.");
  FIO_T_ASSERT(ary____test_get(pa, -1) == 2,
               "last element `get` failed to return correct element.");
  FIO_T_ASSERT(ary____test_get(pa, 0) == 1,
               "`get` by index 0 failed to return correct element.");
  FIO_T_ASSERT(ary____test_get(pa, -2) == 1,
               "last element `get(-2)` failed to return correct element.");
  ary____test_shift(pa, &tmp);
  FIO_T_ASSERT(tmp == 1, "shift failed to set correct element.");
  ary____test_shift(pa, &tmp);
  FIO_T_ASSERT(tmp == 2, "shift failed to set correct element.");
  /* array now empty */
  ary____test_unshift(pa, 1);
  ary____test_unshift(pa, 2);
  ary____test_unshift(pa, 3);
  ary____test_set(pa, -100, 1, NULL);
  FIO_T_ASSERT(ary____test_count(pa) == 100,
               "set with 100 elements should force create elements.");
  // FIO_ARRAY_EACH(pa, pos) {
  //   fprintf(stderr, "[%zu]  %d\n", (size_t)(pos -
  //   FIO_NAME2(ary____test,ptr)(pa)), *pos);
  // }
  FIO_T_ASSERT(ary____test_get(pa, 99) == 1,
               "Intialized element should be kept (index 99)");
  FIO_T_ASSERT(ary____test_get(pa, 98) == 2,
               "Intialized element should be kept (index 98)");
  FIO_T_ASSERT(ary____test_get(pa, 97) == 3,
               "Intialized element should be kept (index 97)");
  for (int i = 1; i < 97; ++i) {
    FIO_T_ASSERT(ary____test_get(pa, i) == 0,
                 "Unintialized element should be 0");
  }
  ary____test_remove2(pa, 0);
  FIO_T_ASSERT(ary____test_count(pa) == 4,
               "remove2 should have removed all zero elements.");
  FIO_T_ASSERT(ary____test_get(pa, 0) == 1, "remove2 should have kept index 0");
  FIO_T_ASSERT(ary____test_get(pa, 1) == 3, "remove2 should have kept index 1");
  FIO_T_ASSERT(ary____test_get(pa, 2) == 2, "remove2 should have kept index 2");
  FIO_T_ASSERT(ary____test_get(pa, 3) == 1, "remove2 should have kept index 3");
  tmp = 9;
  ary____test_remove(pa, 0, &tmp);
  FIO_T_ASSERT(tmp == 1, "remove should have copied the value to the pointer.");
  FIO_T_ASSERT(ary____test_count(pa) == 3,
               "remove should have removed an element.");
  FIO_T_ASSERT(ary____test_get(pa, 0) == 3,
               "remove should have compacted the array.");
  /* test heap allocated array (destroy) */
  ary____test_destroy(pa);
  FIO_T_ASSERT(ary____test_capa(pa) == 0,
               "Destroyed array should have zero capacity");
  FIO_T_ASSERT(ary____test_count(pa) == 0,
               "Destroyed array should have zero elements");
  FIO_T_ASSERT(FIO_NAME2(ary____test, ptr)(pa) == NULL,
               "Destroyed array shouldn't have memory allocated");
  ary____test_unshift(pa, 1);
  ary____test_unshift(pa, 2);
  ary____test_unshift(pa, 3);
  ary____test_reserve(pa, -100);
  FIO_T_ASSERT(ary____test_count(pa) == 3,
               "reserve shouldn't change item count.");
  FIO_T_ASSERT(ary____test_capa(pa) >= 100, "reserve should reserve.");
  FIO_T_ASSERT(ary____test_get(pa, 0) == 3, "reserve should have kept index 0");
  FIO_T_ASSERT(ary____test_get(pa, 1) == 2, "reserve should have kept index 1");
  FIO_T_ASSERT(ary____test_get(pa, 2) == 1, "reserve should have kept index 2");
  ary____test_destroy(pa);
  ary____test_free(pa);

  fprintf(stderr, "* Testing non-zero value for uninitialized elements.\n");
  ary2____test_s a2 = FIO_ARRAY_INIT;
  ary2____test_set(&a2, 99, 1, NULL);
  FIO_ARRAY_EACH(&a2, pos) {
    FIO_T_ASSERT(
        (*pos == 0xFF || (pos - FIO_NAME2(ary2____test, ptr)(&a2)) == 99),
        "uninitialized elements should be initialized as "
        "FIO_ARRAY_TYPE_INVALID");
  }
  ary2____test_set(&a2, -200, 1, NULL);
  FIO_T_ASSERT(ary2____test_count(&a2) == 200, "array should have 100 items.");
  FIO_ARRAY_EACH(&a2, pos) {
    FIO_T_ASSERT((*pos == 0xFF ||
                  (pos - FIO_NAME2(ary2____test, ptr)(&a2)) == 0 ||
                  (pos - FIO_NAME2(ary2____test, ptr)(&a2)) == 199),
                 "uninitialized elements should be initialized as "
                 "FIO_ARRAY_TYPE_INVALID (index %zd)",
                 (pos - FIO_NAME2(ary2____test, ptr)(&a2)));
  }
  ary2____test_destroy(&a2);

  /* Round 3 - heap, with reference counting */
  fprintf(stderr, "* Testing reference counting.\n");
  /* test heap allocated array (initialization) */
  pa = ary____test_new2();
  ary____test_up_ref(pa);
  ary____test_unshift(pa, 2);
  ary____test_unshift(pa, 1);
  ary____test_free2(pa);
  FIO_T_ASSERT(!ary____test_was_destroyed,
               "reference counted array destroyed too early.");
  FIO_T_ASSERT(ary____test_get(pa, 1) == 2,
               "`get` by index failed to return correct element.");
  FIO_T_ASSERT(ary____test_get(pa, -1) == 2,
               "last element `get` failed to return correct element.");
  FIO_T_ASSERT(ary____test_get(pa, 0) == 1,
               "`get` by index 0 failed to return correct element.");
  FIO_T_ASSERT(ary____test_get(pa, -2) == 1,
               "last element `get(-2)` failed to return correct element.");
  ary____test_free2(pa);
  FIO_T_ASSERT(ary____test_was_destroyed,
               "reference counted array not destroyed.");

  fprintf(stderr, "* Testing dynamic arrays helpers.\n");
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    ary____test_push(&a, i);
  }
  FIO_T_ASSERT(ary____test_count(&a) == TEST_REPEAT, "push object count error");
  {
    size_t c = 0;
    size_t i = ary____test_each(&a, 3, fio_____dynamic_test_array_task, &c);
    FIO_T_ASSERT(i < 64, "too many objects counted in each loop.");
    FIO_T_ASSERT(c >= 256 && c < 512, "each loop too long.");
  }
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    FIO_T_ASSERT((size_t)ary____test_get(&a, i) == i,
                 "push order / insert issue");
  }
  ary____test_destroy(&a);
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    ary____test_unshift(&a, i);
  }
  FIO_T_ASSERT(ary____test_count(&a) == TEST_REPEAT,
               "unshift object count error");
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    int old = 0;
    ary____test_pop(&a, &old);
    FIO_T_ASSERT((size_t)old == i, "shift order / insert issue");
  }
  ary____test_destroy(&a);
}

/* *****************************************************************************
Hash Map / Set - test
***************************************************************************** */

/* a simple set of numbers */
#define FIO_MAP_NAME set_____test
#define FIO_MAP_TYPE size_t
#define FIO_MAP_TYPE_CMP(a, b) ((a) == (b))
#define FIO_PTR_TAG(p) fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p) fio___dynamic_types_test_untag(((uintptr_t)p))
#include __FILE__

/* a simple set of numbers */
#define FIO_MAP_NAME set2_____test
#define FIO_MAP_TYPE size_t
#define FIO_MAP_TYPE_CMP(a, b) 1
#define FIO_PTR_TAG(p) fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p) fio___dynamic_types_test_untag(((uintptr_t)p))
#include __FILE__

TEST_FUNC size_t map_____test_key_copy_counter = 0;
TEST_FUNC void map_____test_key_copy(char **dest, char *src) {
  *dest = (char *)FIO_MEM_CALLOC(strlen(src) + 1, sizeof(*dest));
  FIO_T_ASSERT(*dest, "not memory to allocate key in map_test")
  strcpy(*dest, src);
  ++map_____test_key_copy_counter;
}
TEST_FUNC void map_____test_key_destroy(char **dest) {
  FIO_MEM_FREE(*dest, strlen(*dest) + 1);
  *dest = NULL;
  --map_____test_key_copy_counter;
}

/* keys are strings, values are numbers */
#define FIO_MAP_KEY char *
#define FIO_MAP_KEY_CMP(a, b) (strcmp((a), (b)) == 0)
#define FIO_MAP_KEY_COPY(a, b) map_____test_key_copy(&(a), (b))
#define FIO_MAP_KEY_DESTROY(a) map_____test_key_destroy(&(a))
#define FIO_MAP_TYPE size_t
#define FIO_MAP_NAME map_____test
#include __FILE__

#define HASHOFi(i) i /* fio_risky_hash(&(i), sizeof((i)), 0) */
#define HASHOFs(s) fio_risky_hash(s, strlen((s)), 0)

TEST_FUNC int set_____test_each_task(size_t o, void *a_) {
  uintptr_t *i_p = (uintptr_t *)a_;
  FIO_T_ASSERT(o == ++(*i_p), "set_each started at a bad offset!");
  FIO_T_ASSERT(HASHOFi((o - 1)) == set_____test_each_get_key(),
               "set_each key error!");
  return 0;
}

TEST_FUNC void fio___dynamic_types_test___map_test(void) {
  {
    set_____test_s m = FIO_MAP_INIT;
    fprintf(stderr, "* Testing dynamic hash / set maps.\n");

    fprintf(stderr, "* Testing set (hash map where value == key).\n");
    FIO_T_ASSERT(set_____test_count(&m) == 0,
                 "freshly initialized map should have no objects");
    FIO_T_ASSERT(set_____test_capa(&m) == 0,
                 "freshly initialized map should have no capacity");
    FIO_T_ASSERT(set_____test_reserve(&m, (TEST_REPEAT >> 1)) >=
                     (TEST_REPEAT >> 1),
                 "reserve should increase capacity.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set_____test_set_if_missing(&m, HASHOFi(i), i + 1);
    }
    {
      uintptr_t pos_test = (TEST_REPEAT >> 1);
      size_t count =
          set_____test_each(&m, pos_test, set_____test_each_task, &pos_test);
      FIO_T_ASSERT(count == set_____test_count(&m),
                   "set_each tast returned the wrong counter.");
      FIO_T_ASSERT(count == pos_test, "set_each position testing error");
    }

    FIO_T_ASSERT(set_____test_count(&m) == TEST_REPEAT,
                 "After inserting %zu items to set, got %zu items",
                 (size_t)TEST_REPEAT, (size_t)set_____test_count(&m));
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set_____test_get(&m, HASHOFi(i), i + 1) == i + 1,
                   "item retrival error in set.");
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set_____test_get(&m, HASHOFi(i), i + 2) == 0,
                   "item retrival error in set - object comparisson error?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set_____test_set_if_missing(&m, HASHOFi(i), i + 1);
    }
    {
      size_t i = 0;
      FIO_MAP_EACH2(set_____test, &m, pos) {
        FIO_T_ASSERT(pos->obj == pos->hash + 1 || !(~pos->hash),
                     "FIO_MAP_EACH loop out of order?")
        ++i;
      }
      FIO_T_ASSERT(i == set_____test_count(&m), "FIO_MAP_EACH loop incomplete?")
    }
    FIO_T_ASSERT(set_____test_count(&m) == TEST_REPEAT,
                 "Inserting existing object should keep existing object.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set_____test_get(&m, HASHOFi(i), i + 1) == i + 1,
                   "item retrival error in set - insert failed to update?");
      FIO_T_ASSERT(set_____test_get_ptr(&m, HASHOFi(i), i + 1) &&
                       set_____test_get_ptr(&m, HASHOFi(i), i + 1)[0] == i + 1,
                   "pointer retrival error in set.");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set_____test_set(&m, HASHOFi(i), i + 2, &old);
      FIO_T_ASSERT(old == 0,
                   "old pointer not initialized with old (or missing) data");
    }

    FIO_T_ASSERT(set_____test_count(&m) == (TEST_REPEAT * 2),
                 "full hash collision shoudn't break map until attack limit.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set_____test_get(&m, HASHOFi(i), i + 2) == i + 2,
                   "item retrival error in set - overwrite failed to update?");
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set_____test_get(&m, HASHOFi(i), i + 1) == i + 1,
                   "item retrival error in set - collision resolution error?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set_____test_remove(&m, HASHOFi(i), i + 1, &old);
      FIO_T_ASSERT(old == i + 1,
                   "removed item not initialized with old (or missing) data");
    }
    FIO_T_ASSERT(set_____test_count(&m) == TEST_REPEAT,
                 "removal should update object count.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set_____test_get(&m, HASHOFi(i), i + 1) == 0,
                   "removed items should be unavailable");
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set_____test_get(&m, HASHOFi(i), i + 2) == i + 2,
                   "previous items should be accessible after removal");
    }
    set_____test_destroy(&m);
  }
  {
    set2_____test_s m = FIO_MAP_INIT;
    fprintf(stderr, "* Testing set map without value comparison.\n");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set2_____test_set_if_missing(&m, HASHOFi(i), i + 1);
    }

    FIO_T_ASSERT(set2_____test_count(&m) == TEST_REPEAT,
                 "After inserting %zu items to set, got %zu items",
                 (size_t)TEST_REPEAT, (size_t)set2_____test_count(&m));
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == i + 1,
                   "item retrival error in set.");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set2_____test_set_if_missing(&m, HASHOFi(i), i + 2);
    }
    FIO_T_ASSERT(set2_____test_count(&m) == TEST_REPEAT,
                 "Inserting existing object should keep existing object.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == i + 1,
                   "item retrival error in set - insert failed to update?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set2_____test_set(&m, HASHOFi(i), i + 2, &old);
      FIO_T_ASSERT(old == i + 1,
                   "old pointer not initialized with old (or missing) data");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == i + 2,
                   "item retrival error in set - overwrite failed to update?");
    }
    {
      /* test partial removal */
      for (size_t i = 1; i < TEST_REPEAT; i += 2) {
        size_t old = 5;
        set2_____test_remove(&m, HASHOFi(i), 0, &old);
        FIO_T_ASSERT(old == i + 2,
                     "removed item not initialized with old (or missing) data "
                     "(%zu != %zu)",
                     old, i + 2);
      }
      for (size_t i = 1; i < TEST_REPEAT; i += 2) {
        FIO_T_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == 0,
                     "previous items should NOT be accessible after removal");
        set2_____test_set_if_missing(&m, HASHOFi(i), i + 2);
      }
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set2_____test_remove(&m, HASHOFi(i), 0, &old);
      FIO_T_ASSERT(old == i + 2,
                   "removed item not initialized with old (or missing) data "
                   "(%zu != %zu)",
                   old, i + 2);
    }
    FIO_T_ASSERT(set2_____test_count(&m) == 0,
                 "removal should update object count.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_T_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == 0,
                   "previous items should NOT be accessible after removal");
    }
    set2_____test_destroy(&m);
  }

  {
    map_____test_s *m = map_____test_new();
    fprintf(stderr, "* Testing hash map.\n");
    FIO_T_ASSERT(map_____test_count(m) == 0,
                 "freshly initialized map should have no objects");
    FIO_T_ASSERT(map_____test_capa(m) == 0,
                 "freshly initialized map should have no capacity");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      char buffer[64];
      int l = snprintf(buffer, 63, "%zu", i);
      buffer[l] = 0;
      map_____test_set(m, HASHOFs(buffer), buffer, i + 1, NULL);
    }
    FIO_T_ASSERT(map_____test_key_copy_counter == TEST_REPEAT,
                 "key copying error - was the key copied?");
    FIO_T_ASSERT(map_____test_count(m) == TEST_REPEAT,
                 "After inserting %zu items to map, got %zu items",
                 (size_t)TEST_REPEAT, (size_t)map_____test_count(m));
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      char buffer[64];
      int l = snprintf(buffer + 1, 61, "%zu", i);
      buffer[l + 1] = 0;
      FIO_T_ASSERT(map_____test_get(m, HASHOFs(buffer + 1), buffer + 1) ==
                       i + 1,
                   "item retrival error in map.");
      FIO_T_ASSERT(map_____test_get_ptr(m, HASHOFs(buffer + 1), buffer + 1) &&
                       map_____test_get_ptr(m, HASHOFs(buffer + 1),
                                            buffer + 1)[0] == i + 1,
                   "pointer retrival error in map.");
    }
    map_____test_free(m);
    FIO_T_ASSERT(map_____test_key_copy_counter == 0,
                 "key destruction error - was the key freed?");
  }
  {
    set_____test_s s = FIO_MAP_INIT;
    map_____test_s m = FIO_MAP_INIT;
    fprintf(stderr, "* Testing attack resistance (SHOULD print warnings).\n");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      char buf[64];
      fio_ltoa(buf, i, 16);
      set_____test_set(&s, 1, i + 1, NULL);
      map_____test_set(&m, 1, buf, i + 1, NULL);
    }
    FIO_T_ASSERT(set_____test_count(&s) != TEST_REPEAT,
                 "full collision protection failed (set)?");
    FIO_T_ASSERT(map_____test_count(&m) != TEST_REPEAT,
                 "full collision protection failed (map)?");
    FIO_T_ASSERT(set_____test_count(&s) != 1,
                 "full collision test failed to push elements (set)?");
    FIO_T_ASSERT(map_____test_count(&m) != 1,
                 "full collision test failed to push elements (map)?");
    set_____test_destroy(&s);
    map_____test_destroy(&m);
  }
}

#undef HASHOFi
#undef HASHOFs

/* *****************************************************************************
Dynamic Strings - test
***************************************************************************** */

#define FIO_STR_NAME fio_big_str
#define FIO_STR_WRITE_TEST_FUNC
#include __FILE__

#define FIO_STR_SMALL fio_small_str
#define FIO_STR_WRITE_TEST_FUNC
#include __FILE__

/**
 * Tests the fio_str functionality.
 */
TEST_FUNC void fio___dynamic_types_test___str(void) {
  fio_big_str___dynamic_test();
  fio_small_str___dynamic_test();
}

/* *****************************************************************************
Time - test
***************************************************************************** */
#define FIO_TIME
#include __FILE__

#define FIO___GMTIME_TEST_INTERVAL ((60L * 60 * 24) - 7) /* 1day - 7seconds */
#define FIO___GMTIME_TEST_RANGE (4093L * 365) /* test ~4 millenium  */

TEST_FUNC void fio___dynamic_types_test___gmtime(void) {
  fprintf(stderr, "* Testing facil.io fio_time2gm vs gmtime_r\n");
  struct tm tm1, tm2;
  const time_t now = fio_time_real().tv_sec;
  const time_t end =
      now + (FIO___GMTIME_TEST_RANGE * FIO___GMTIME_TEST_INTERVAL);
  time_t t = now - (FIO___GMTIME_TEST_RANGE * FIO___GMTIME_TEST_INTERVAL);
  while (t < end) {
    time_t tmp = t;
    t += FIO___GMTIME_TEST_INTERVAL;
    tm2 = fio_time2gm(tmp);
    FIO_T_ASSERT(fio_gm2time(tm2) == tmp,
                 "fio_gm2time roundtrip error (%ld != %ld)",
                 (long)fio_gm2time(tm2), (long)tmp);
    gmtime_r(&tmp, &tm1);
    if (tm1.tm_year != tm2.tm_year || tm1.tm_mon != tm2.tm_mon ||
        tm1.tm_mday != tm2.tm_mday || tm1.tm_yday != tm2.tm_yday ||
        tm1.tm_hour != tm2.tm_hour || tm1.tm_min != tm2.tm_min ||
        tm1.tm_sec != tm2.tm_sec || tm1.tm_wday != tm2.tm_wday) {
      char buf[256];
      fio_time2rfc7231(buf, tmp);
      FIO_T_ASSERT(0,
                   "system gmtime_r != fio_time2gm for %ld!\n"
                   "-- System:\n"
                   "\ttm_year: %d\n"
                   "\ttm_mon: %d\n"
                   "\ttm_mday: %d\n"
                   "\ttm_yday: %d\n"
                   "\ttm_hour: %d\n"
                   "\ttm_min: %d\n"
                   "\ttm_sec: %d\n"
                   "\ttm_wday: %d\n"
                   "-- facil.io:\n"
                   "\ttm_year: %d\n"
                   "\ttm_mon: %d\n"
                   "\ttm_mday: %d\n"
                   "\ttm_yday: %d\n"
                   "\ttm_hour: %d\n"
                   "\ttm_min: %d\n"
                   "\ttm_sec: %d\n"
                   "\ttm_wday: %d\n"
                   "-- As String:\n"
                   "\t%s",
                   (long)t, tm1.tm_year, tm1.tm_mon, tm1.tm_mday, tm1.tm_yday,
                   tm1.tm_hour, tm1.tm_min, tm1.tm_sec, tm1.tm_wday,
                   tm2.tm_year, tm2.tm_mon, tm2.tm_mday, tm2.tm_yday,
                   tm2.tm_hour, tm2.tm_min, tm2.tm_sec, tm2.tm_wday, buf);
    }
  }
  {
    uint64_t start, stop;
#if DEBUG
    fprintf(stderr, "PERFOMEANCE TESTS IN DEBUG MODE ARE BIASED\n");
#endif
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      volatile struct tm tm = fio_time2gm(now);
      __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
      (void)tm;
    }
    stop = fio_time_micro();
    fprintf(stderr, "\t- fio_time2gm speed test took:\t%zuus\n",
            (size_t)(stop - start));
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      volatile struct tm tm;
      time_t tmp = now;
      gmtime_r(&tmp, (struct tm *)&tm);
      __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
    }
    stop = fio_time_micro();
    fprintf(stderr, "\t- gmtime_r speed test took:  \t%zuus\n",
            (size_t)(stop - start));
    fprintf(stderr, "\n");
    struct tm tm_now = fio_time2gm(now);
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      tm_now = fio_time2gm(now + i);
      time_t t_tmp = fio_gm2time(tm_now);
      __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
      (void)t_tmp;
    }
    stop = fio_time_micro();
    fprintf(stderr, "\t- fio_gm2time speed test took:\t%zuus\n",
            (size_t)(stop - start));
    start = fio_time_micro();
    for (size_t i = 0; i < (1 << 17); ++i) {
      tm_now = fio_time2gm(now + i);
      volatile time_t t_tmp = mktime((struct tm *)&tm_now);
      __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
      (void)t_tmp;
    }
    stop = fio_time_micro();
    fprintf(stderr, "\t- mktime speed test took:    \t%zuus\n",
            (size_t)(stop - start));
    fprintf(stderr, "\n");
  }
}

/* *****************************************************************************
Queue - test
***************************************************************************** */
#define FIO_QUEUE
#include __FILE__

#ifndef FIO___QUEUE_TEST_PRINT
#define FIO___QUEUE_TEST_PRINT 0
#endif

#include "pthread.h"

#define FIO___QUEUE_TOTAL_COUNT (512 * 1024)

typedef struct {
  fio_queue_s *q;
  uintptr_t count;
} fio___queue_test_s;

TEST_FUNC void fio___queue_test_sample_task(void *i_count, void *unused2) {
  (void)(unused2);
  fio_atomic_add((uintptr_t *)i_count, 1);
}

TEST_FUNC void fio___queue_test_sched_sample_task(void *t_, void *i_count) {
  fio___queue_test_s *t = (fio___queue_test_s *)t_;
  for (size_t i = 0; i < t->count; i++) {
    fio_queue_push(t->q, .fn = fio___queue_test_sample_task, .udata1 = i_count);
  }
}

TEST_FUNC int fio___queue_test_timer_task(void *i_count, void *unused2) {
  fio_atomic_add((uintptr_t *)i_count, 1);
  return (unused2 ? -1 : 0);
}

TEST_FUNC void fio___dynamic_types_test___queue(void) {
  fprintf(stderr, "* Testing facil.io task scheduling (fio_queue)\n");
  fio_queue_s *q = fio_queue_new();

  fprintf(stderr, "\t- size of queue object (fio_queue_s): %zu\n", sizeof(*q));
  fprintf(stderr, "\t- size of queue ring buffer (per allocation): %zu\n",
          sizeof(q->mem));
  fprintf(stderr, "\t- event slots per queue allocation: %zu\n",
          (size_t)FIO_QUEUE_TASKS_PER_ALLOC);

  const size_t max_threads = 12; // assumption / pure conjuncture...
  uintptr_t i_count;
  clock_t start, end;
  i_count = 0;
  start = clock();
  for (size_t i = 0; i < FIO___QUEUE_TOTAL_COUNT; i++) {
    fio___queue_test_sample_task(&i_count, NULL);
  }
  end = clock();
  if (FIO___QUEUE_TEST_PRINT) {
    fprintf(
        stderr,
        "\t- Queueless (direct call) counter: %lu cycles with i_count = %lu\n",
        (unsigned long)(end - start), (unsigned long)i_count);
  }
  size_t i_count_should_be = i_count;
  i_count = 0;
  start = clock();
  for (size_t i = 0; i < FIO___QUEUE_TOTAL_COUNT; i++) {
    fio_queue_push(q, .fn = fio___queue_test_sample_task,
                   .udata1 = (void *)&i_count);
    fio_queue_perform(q);
  }
  end = clock();
  if (FIO___QUEUE_TEST_PRINT) {
    fprintf(stderr, "\t- single task counter: %lu cycles with i_count = %lu\n",
            (unsigned long)(end - start), (unsigned long)i_count);
  }
  FIO_ASSERT(i_count == i_count_should_be, "ERROR: queue count invalid\n");

  if (FIO___QUEUE_TEST_PRINT) {
    fprintf(stderr, "\n");
  }

  for (size_t i = 1; i < 32 && FIO___QUEUE_TOTAL_COUNT >> i; ++i) {
    i_count = 0;
    fio___queue_test_s info = {
        .q = q, .count = (uintptr_t)(FIO___QUEUE_TOTAL_COUNT >> i)};
    const size_t tasks = 1 << i;
    start = clock();
    for (size_t j = 0; j < tasks; ++j) {
      fio_queue_push(q, fio___queue_test_sched_sample_task, (void *)&info,
                     &i_count);
    }
    FIO_ASSERT(fio_queue_count(q), "tasks not counted?!") {
      const size_t t_count = (i % max_threads) + 1;
      pthread_t *threads =
          (pthread_t *)FIO_MEM_CALLOC(sizeof(*threads), t_count);
      for (size_t j = 0; j < t_count; ++j) {
        if (pthread_create(threads + j, NULL,
                           (void *(*)(void *))fio_queue_perform_all, q)) {
          abort();
        }
      }
      for (size_t j = 0; j < t_count; ++j) {
        pthread_join(threads[j], NULL);
      }
      FIO_MEM_FREE(threads, sizeof(*threads) * t_count);
    }

    end = clock();
    if (FIO___QUEUE_TEST_PRINT) {
      fprintf(stderr,
              "- queue performed using %zu threads, %zu scheduling loops (%zu "
              "each):\n"
              "    %lu cycles with i_count = %lu\n",
              ((i % max_threads) + 1), tasks, info.count,
              (unsigned long)(end - start), (unsigned long)i_count);
    } else {
      fprintf(stderr, ".");
    }
    FIO_ASSERT(i_count == i_count_should_be, "ERROR: queue count invalid\n");
  }
  if (!(FIO___QUEUE_TEST_PRINT))
    fprintf(stderr, "\n");
  FIO_ASSERT(q->w == &q->mem,
             "queue library didn't release dynamic queue (should be static)");
  fio_queue_free(q);
  {
    fprintf(stderr, "* testing urgent insertion\n");
    fio_queue_s q2 = FIO_QUEUE_INIT(q2);
    for (size_t i = 0; i < (FIO_QUEUE_TASKS_PER_ALLOC * 3); ++i) {
      FIO_T_ASSERT(
          !fio_queue_push_urgent(&q2, .fn = (void (*)(void *, void *))(i + 1),
                                 .udata1 = (void *)(i + 1)),
          "fio_queue_push_urgent failed");
    }
    FIO_T_ASSERT(q2.r->next && q2.r->next->next && !q2.r->next->next->next,
                 "should have filled only three task blocks");
    for (size_t i = 0; i < (FIO_QUEUE_TASKS_PER_ALLOC * 3); ++i) {
      fio_queue_task_s t = fio_queue_pop(&q2);
      FIO_T_ASSERT(
          t.fn && (size_t)t.udata1 == (FIO_QUEUE_TASKS_PER_ALLOC * 3) - i,
          "fio_queue_push_urgent pop ordering error [%zu] %zu != %zu (%p)", i,
          (size_t)t.udata1, (FIO_QUEUE_TASKS_PER_ALLOC * 3) - i,
          (void *)(uintptr_t)t.fn);
    }
    FIO_T_ASSERT(fio_queue_pop(&q2).fn == NULL,
                 "pop overflow after urgent tasks");
    fio_queue_destroy(&q2);
  }
  {
    fprintf(stderr,
            "* Testing facil.io timer scheduling (fio_timer_queue_s)\n");
    fprintf(stderr, "  Note: Errors SHOULD print out to the log.\n");
    fio_queue_s q2 = FIO_QUEUE_INIT(q2);
    uintptr_t tester = 0;
    fio_timer_queue_s tq = FIO_TIMER_QUEUE_INIT;

    /* test failuers */
    fio_timer_schedule(&tq, .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task, .every = 100,
                       .repetitions = -1);
    FIO_T_ASSERT(tester == 1,
                 "fio_timer_schedule should have called `on_finish`");
    tester = 0;
    fio_timer_schedule(NULL, .fn = fio___queue_test_timer_task,
                       .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task, .every = 100,
                       .repetitions = -1);
    FIO_T_ASSERT(tester == 1,
                 "fio_timer_schedule should have called `on_finish`");
    tester = 0;
    fio_timer_schedule(&tq, .fn = fio___queue_test_timer_task,
                       .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task, .every = 0,
                       .repetitions = -1);
    FIO_T_ASSERT(tester == 1,
                 "fio_timer_schedule should have called `on_finish`");

    /* test endless task */
    tester = 0;
    fio_timer_schedule(&tq, .fn = fio___queue_test_timer_task,
                       .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task, .every = 1,
                       .repetitions = -1, .start_at = fio_time_milli() - 10);
    FIO_T_ASSERT(tester == 0,
                 "fio_timer_schedule should have scheduled the task.");
    for (size_t i = 0; i < 10; ++i) {
      fio_timer_push2queue(&q2, &tq, fio_time_milli());
      FIO_T_ASSERT(fio_queue_count(&q2) == 1,
                   "task should have been scheduled");
      fio_queue_perform(&q2);
      FIO_T_ASSERT(!fio_queue_count(&q2), "queue should be empty");
      FIO_T_ASSERT(tester == i + 1, "task should have been performed (%zu).",
                   (size_t)tester);
    }
    tester = 0;
    fio_timer_clear(&tq);
    FIO_T_ASSERT(tester == 1, "fio_timer_clear should have called `on_finish`");

    /* test single-use task */
    tester = 0;
    uint64_t milli_now = fio_time_milli();
    fio_timer_schedule(&tq, .fn = fio___queue_test_timer_task,
                       .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task, .every = 100,
                       .repetitions = 1, .start_at = milli_now - 10);
    FIO_T_ASSERT(tester == 0,
                 "fio_timer_schedule should have scheduled the task.");
    fio_timer_schedule(&tq, .fn = fio___queue_test_timer_task,
                       .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task, .every = 1,
                       // .repetitions = 1, // auto-value is 1
                       .start_at = milli_now - 10);
    FIO_T_ASSERT(tester == 0,
                 "fio_timer_schedule should have scheduled the task.");
    FIO_T_ASSERT(fio_timer_next_at(&tq) == milli_now - 9,
                 "fio_timer_next_at value error.");
    fio_timer_push2queue(&q2, &tq, milli_now);
    FIO_T_ASSERT(fio_queue_count(&q2) == 1, "task should have been scheduled");
    FIO_T_ASSERT(fio_timer_next_at(&tq) == milli_now + 90,
                 "fio_timer_next_at value error for unscheduled task.");
    fio_queue_perform(&q2);
    FIO_T_ASSERT(!fio_queue_count(&q2), "queue should be empty");
    FIO_T_ASSERT(tester == 2,
                 "task should have been performed and on_finish called (%zu).",
                 (size_t)tester);
    fio_timer_clear(&tq);
    FIO_T_ASSERT(
        tester == 3,
        "fio_timer_clear should have called on_finish of future task (%zu).",
        (size_t)tester);
    FIO_T_ASSERT(!tq.next, "timer queue should be empty.");
    fio_queue_destroy(&q2);
  }
  fprintf(stderr, "* passed.\n");
}

/* *****************************************************************************
CLI - test
***************************************************************************** */

#define FIO_CLI
#include __FILE__

TEST_FUNC void fio___dynamic_types_test___cli(void) {
  const char *argv[] = {
      "appname", "-i11", "-i2=2", "-i3", "3", "-t", "-s", "test", "unnamed",
  };
  const int argc = sizeof(argv) / sizeof(argv[0]);
  fprintf(stderr, "* Testing CLI helpers.\n");
  fio_cli_start(argc, argv, 0, -1, NULL,
                FIO_CLI_INT("-integer1 -i1 first integer"),
                FIO_CLI_INT("-integer2 -i2 second integer"),
                FIO_CLI_INT("-integer3 -i3 third integer"),
                FIO_CLI_BOOL("-boolean -t boolean"),
                FIO_CLI_BOOL("-boolean_false -f boolean"),
                FIO_CLI_STRING("-str -s a string"));
  FIO_T_ASSERT(fio_cli_get_i("-i2") == 2, "CLI second integer error.");
  FIO_T_ASSERT(fio_cli_get_i("-i3") == 3, "CLI third integer error.");
  FIO_T_ASSERT(fio_cli_get_i("-i1") == 1, "CLI first integer error.");
  FIO_T_ASSERT(fio_cli_get_i("-i2") == fio_cli_get_i("-integer2"),
               "CLI second integer error.");
  FIO_T_ASSERT(fio_cli_get_i("-i3") == fio_cli_get_i("-integer3"),
               "CLI third integer error.");
  FIO_T_ASSERT(fio_cli_get_i("-i1") == fio_cli_get_i("-integer1"),
               "CLI first integer error.");
  FIO_T_ASSERT(fio_cli_get_i("-t") == 1, "CLI boolean true error.");
  FIO_T_ASSERT(fio_cli_get_i("-f") == 0, "CLI boolean false error.");
  FIO_T_ASSERT(!strcmp(fio_cli_get("-s"), "test"), "CLI string error.");
  FIO_T_ASSERT(fio_cli_unnamed_count() == 1, "CLI unnamed count error.");
  FIO_T_ASSERT(!strcmp(fio_cli_unnamed(0), "unnamed"), "CLI unnamed error.");
  fio_cli_set("-manual", "okay");
  FIO_T_ASSERT(!strcmp(fio_cli_get("-manual"), "okay"), "CLI set/get error.");
  fio_cli_end();
  FIO_T_ASSERT(fio_cli_get_i("-i1") == 0, "CLI cleanup error.");
}

/* *****************************************************************************
Memory Allocation - test
***************************************************************************** */

#ifdef FIO_MALLOC_FORCE_SYSTEM

TEST_FUNC void fio___dynamic_types_test___mem(void) {
  fprintf(stderr, "* Custom memory allocator bypassed.\n");
}

#else

#define FIO_MALLOC
#include __FILE__

TEST_FUNC void fio___dynamic_types_test___mem(void) {
  fprintf(stderr, "* Testing core memory allocator (fio_malloc).\n");
  const size_t three_blocks = ((size_t)3ULL * FIO_MEMORY_BLOCKS_PER_ALLOCATION)
                              << FIO_MEMORY_BLOCK_SIZE_LOG;
  for (int cycles = 4; cycles < FIO_MEMORY_BLOCK_SIZE_LOG; ++cycles) {
    fprintf(stderr, "* Testing %zu byte allocation blocks.\n",
            (size_t)(1UL << cycles));
    const size_t limit = (three_blocks >> cycles);
    char **ary = (char **)fio_calloc(sizeof(*ary), limit);
    FIO_T_ASSERT(ary, "allocation failed for test container");
    for (size_t i = 0; i < limit; ++i) {
      ary[i] = (char *)fio_malloc(1UL << cycles);
      FIO_T_ASSERT(ary[i], "allocation failed!")
      FIO_T_ASSERT(!ary[i][0], "allocated memory not zero");
      memset(ary[i], 0xff, (1UL << cycles));
    }
    for (size_t i = 0; i < limit; ++i) {
      char *tmp =
          (char *)fio_realloc2(ary[i], (2UL << cycles), (1UL << cycles));
      FIO_T_ASSERT(tmp, "re-allocation failed!")
      ary[i] = tmp;
      FIO_T_ASSERT(!ary[i][(2UL << cycles) - 1], "fio_realloc2 copy overflow!");
      tmp = (char *)fio_realloc2(ary[i], (1UL << cycles), (2UL << cycles));
      FIO_T_ASSERT(tmp, "re-allocation (shrinking) failed!")
      ary[i] = tmp;
      FIO_T_ASSERT(ary[i][(1UL << cycles) - 1] == (char)0xFF,
                   "fio_realloc2 copy underflow!");
    }
    for (size_t i = 0; i < limit; ++i) {
      fio_free(ary[i]);
    }
    fio_free(ary);
  }
#if DEBUG && FIO_EXTERN_COMPLETE
  fio___mem_destroy();
  FIO_T_ASSERT(fio___mem_block_count <= 1, "memory leaks?");
#endif
}
#endif

/* *****************************************************************************
Socket helper testing
***************************************************************************** */

#define FIO_SOCK
#include __FILE__

TEST_FUNC void fio___sock_test_before_events(void *udata) {
  *(size_t *)udata = 0;
}
TEST_FUNC void fio___sock_test_on_event(int fd, size_t index, void *udata) {
  *(size_t *)udata += 1;
  if (errno) {
    FIO_LOG_WARNING("(possibly expected) %s", strerror(errno));
    errno = 0;
  }
  (void)fd;
  (void)index;
}
TEST_FUNC void fio___sock_test_after_events(void *udata) {
  if (*(size_t *)udata)
    *(size_t *)udata += 1;
}

TEST_FUNC void fio___dynamic_types_test___sock(void) {
  fprintf(stderr,
          "* Testing socket helpers (FIO_SOCK) - partial tests only!\n");
  struct {
    const char *address;
    const char *port;
    const char *msg;
    uint16_t flag;
  } server_tests[] = {
      {"127.0.0.1", "9437", "TCP", FIO_SOCK_TCP},
#ifdef P_tmpdir
      {P_tmpdir "/tmp_unix_testing_socket_facil_io.sock", NULL, "Unix",
       FIO_SOCK_UNIX},
#else
      {"./tmp_unix_testing_socket_facil_io.sock", NULL, "Unix", FIO_SOCK_UNIX},
#endif
      /* accept doesn't work with UDP, not like this... UDP test is seperate */
      // {"127.0.0.1", "9437", "UDP", FIO_SOCK_UDP},
      {.address = NULL},
  };
  for (size_t i = 0; server_tests[i].address; ++i) {
    size_t flag = (size_t)-1;
    errno = 0;
    fprintf(stderr, "* Testing %s socket API\n", server_tests[i].msg);
    int srv = fio_sock_open(server_tests[i].address, server_tests[i].port,
                            server_tests[i].flag | FIO_SOCK_SERVER);
    FIO_T_ASSERT(srv != -1, "server socket failed to open: %s",
                 strerror(errno));
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL, .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events, .udata = &flag);
    FIO_T_ASSERT(!flag, "before_events not called for missing list! (%zu)",
                 flag);
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL, .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST({.fd = -1}));
    FIO_T_ASSERT(!flag, "before_events not called for empty list! (%zu)", flag);
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL, .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(srv)));
    FIO_T_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL, .on_data = fio___sock_test_on_event,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(srv)));
    FIO_T_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    flag = (size_t)-1;
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = fio___sock_test_on_event, .on_data = NULL,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(srv)));
    FIO_T_ASSERT(!flag, "No event should have occured here! (%zu)", flag);

    int cl = fio_sock_open(server_tests[i].address, server_tests[i].port,
                           server_tests[i].flag | FIO_SOCK_CLIENT);
    FIO_T_ASSERT(cl != -1, "client socket failed to open");
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL, .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_T_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL, .on_data = fio___sock_test_on_event,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_T_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    // // is it possible to write to a still-connecting socket?
    // fio_sock_poll(.before_events = fio___sock_test_before_events,
    //               .after_events = fio___sock_test_after_events,
    //               .on_ready = fio___sock_test_on_event, .on_data = NULL,
    //               .on_error = NULL, .udata = &flag,
    //               .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    // FIO_T_ASSERT(!flag, "No event should have occured here! (%zu)", flag);
    FIO_LOG_INFO("error may print when polling server for `write`.");
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL, .on_data = fio___sock_test_on_event,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .timeout = 100,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(srv)));
    FIO_T_ASSERT(flag == 2, "Event should have occured here! (%zu)", flag);
    FIO_LOG_INFO("error may have been emitted.");

    int accepted = accept(srv, NULL, NULL);
    FIO_T_ASSERT(accepted != -1, "client socket failed to open");
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = fio___sock_test_on_event, .on_data = NULL,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .timeout = 100,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_T_ASSERT(flag, "Event should have occured here! (%zu)", flag);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = fio___sock_test_on_event, .on_data = NULL,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .timeout = 100,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(accepted)));
    FIO_T_ASSERT(flag, "Event should have occured here! (%zu)", flag);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL, .on_data = fio___sock_test_on_event,
                  .on_error = NULL,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_T_ASSERT(!flag, "No event should have occured here! (%zu)", flag);

    if (write(accepted, "hello", 5) > 0) {
      // wait for read
      fio_sock_poll(.before_events = fio___sock_test_before_events,
                    .on_ready = NULL, .on_data = fio___sock_test_on_event,
                    .on_error = NULL,
                    .after_events = fio___sock_test_after_events,
                    .udata = &flag, .timeout = 100,
                    .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_R(cl)));
      // test read/write
      fio_sock_poll(.before_events = fio___sock_test_before_events,
                    .on_ready = fio___sock_test_on_event,
                    .on_data = fio___sock_test_on_event, .on_error = NULL,
                    .after_events = fio___sock_test_after_events,
                    .udata = &flag, .timeout = 100,
                    .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
      {
        char buf[64];
        errno = 0;
        FIO_T_ASSERT(read(cl, buf, 64) > 0,
                     "Read should have read some data...\n\t"
                     "error: %s",
                     strerror(errno));
      }
      FIO_T_ASSERT(flag == 3, "Event should have occured here! (%zu)", flag);
    } else
      FIO_T_ASSERT(0, "write failed! error: %s", strerror(errno));
    close(accepted);
    close(cl);
    close(srv);
    fio_sock_poll(.before_events = fio___sock_test_before_events,
                  .on_ready = NULL, .on_data = NULL,
                  .on_error = fio___sock_test_on_event,
                  .after_events = fio___sock_test_after_events, .udata = &flag,
                  .fds = FIO_SOCK_POLL_LIST(FIO_SOCK_POLL_RW(cl)));
    FIO_T_ASSERT(flag, "Event should have occured here! (%zu)", flag);
    if (FIO_SOCK_UNIX == server_tests[i].flag)
      unlink(server_tests[i].address);
  }
  {
    /* UDP semi test */
    fprintf(stderr, "* Testing UDP socket (abbreviated test)\n");
    int srv = fio_sock_open(NULL, "9437", FIO_SOCK_UDP | FIO_SOCK_SERVER);
    int n = 0; /* try for 32Mb */
    socklen_t sn = sizeof(n);
    if (-1 != getsockopt(srv, SOL_SOCKET, SO_RCVBUF, &n, &sn) &&
        sizeof(n) == sn)
      fprintf(stderr, "\t- UDP default receive buffer is %d bytes\n", n);
    n = 32 * 1024 * 1024; /* try for 32Mb */
    sn = sizeof(n);
    while (setsockopt(srv, SOL_SOCKET, SO_RCVBUF, &n, sn) == -1) {
      /* failed - repeat attempt at 0.5Mb interval */
      if (n >= (1024 * 1024)) // OS may have returned max value
        n -= 512 * 1024;
      else
        break;
    }
    if (-1 != getsockopt(srv, SOL_SOCKET, SO_RCVBUF, &n, &sn) &&
        sizeof(n) == sn)
      fprintf(stderr, "\t- UDP receive buffer could be set to %d bytes\n", n);
    FIO_T_ASSERT(srv != -1, "Couldn't open UDP server socket: %s",
                 strerror(errno));
    int cl = fio_sock_open(NULL, "9437", FIO_SOCK_UDP | FIO_SOCK_CLIENT);
    FIO_T_ASSERT(cl != -1, "Couldn't open UDP client socket: %s",
                 strerror(errno));
    FIO_T_ASSERT(send(cl, "hello", 5, 0) != -1,
                 "couldn't send datagram from client");
    char buf[64];
    FIO_T_ASSERT(recvfrom(srv, buf, 64, 0, NULL, NULL) != -1,
                 "couldn't read datagram");
    FIO_T_ASSERT(!memcmp(buf, "hello", 5), "transmission error");
    close(srv);
    close(cl);
  }
}

/* *****************************************************************************
Hashing speed test
***************************************************************************** */

typedef uintptr_t (*fio__hashing_func_fn)(char *, size_t);

TEST_FUNC void fio_test_hash_function(fio__hashing_func_fn h, char *name,
                                      uint8_t mem_alignment_ofset) {
#ifdef DEBUG
  fprintf(stderr,
          "* Testing %s speed "
          "(DEBUG mode detected - speed may be affected).\n",
          name);
  uint64_t cycles_start_at = (8192 << 4);
#else
  fprintf(stderr, "* Testing %s speed.\n", name);
  uint64_t cycles_start_at = (8192 << 8);
#endif
  /* test based on code from BearSSL with credit to Thomas Pornin */
  size_t const buffer_len = 8192;
  uint8_t buffer_[8200];
  uint8_t *buffer = buffer_ + (mem_alignment_ofset & 7);
  // uint64_t buffer[1024];
  memset(buffer, 'T', buffer_len);
  /* warmup */
  uint64_t hash = 0;
  for (size_t i = 0; i < 4; i++) {
    hash += h((char *)buffer, buffer_len);
    memcpy(buffer, &hash, sizeof(hash));
  }
  /* loop until test runs for more than 2 seconds */
  for (uint64_t cycles = cycles_start_at;;) {
    clock_t start, end;
    start = clock();
    for (size_t i = cycles; i > 0; i--) {
      hash += h((char *)buffer, buffer_len);
      __asm__ volatile("" ::: "memory");
    }
    end = clock();
    memcpy(buffer, &hash, sizeof(hash));
    if ((end - start) >= (2 * CLOCKS_PER_SEC) ||
        cycles >= ((uint64_t)1 << 62)) {
      fprintf(stderr, "\t%-40s %8.2f MB/s\n", name,
              (double)(buffer_len * cycles) /
                  (((end - start) * (1000000.0 / CLOCKS_PER_SEC))));
      break;
    }
    cycles <<= 1;
  }
}

TEST_FUNC uintptr_t fio___dynamic_types_test___risky_wrapper(char *buf,
                                                             size_t len) {
  return fio_risky_hash(buf, len, 1);
}

// TEST_FUNC uintptr_t
// fio___dynamic_types_test___risky_stream_wrapper(char *buf, size_t len) {
//   fio_risky_hash_s r = fio_risky_hash_init(0);
//   __asm__ volatile("" ::: "memory");
//   fio_risky_hash_stream(&r, buf, len);
//   __asm__ volatile("" ::: "memory");
//   return fio_risky_hash_value(&r);
// }

TEST_FUNC uintptr_t fio___dynamic_types_test___risky_mask_wrapper(char *buf,
                                                                  size_t len) {
  fio_risky_mask(buf, len, 0, 0);
  return len;
}

TEST_FUNC void fio___dynamic_types_test___risky(void) {
#if 0
  {
    uint64_t h1, h2;
    const char *buf =
        "This is a small string consisting of 127 bytes (uneven data), meant "
        "for testing that risky streaming == risky non-streaming.123";
    const size_t len = 127;
    fio_risky_hash_s r = fio_risky_hash_init(0);
    // fio_risky_hash_stream(&r, buf, len);
    fio_risky_hash_stream(&r, buf, 37);
    // fio_risky_hash_stream(&r, buf + 37, len - 37);
    h1 = fio_risky_hash_value(&r);
    h2 = fio_risky_hash(buf, len, 0);
    FIO_T_ASSERT(h1 == h2, "Risky Hash Streaming != Non-Streaming %p != %p",
                 (void *)h1, (void *)h2);
  }
#endif
  for (int i = 0; i < 8; ++i) {
    char buf[128];
    uint64_t nonce = fio_rand64();
    const char *str = "this is a short text, to test risky masking";
    char *tmp = buf + i;
    memcpy(tmp, str, strlen(str));
    fio_risky_mask(tmp, strlen(str), (uint64_t)tmp, nonce);
    FIO_T_ASSERT(memcmp(tmp, str, strlen(str)), "Risky Hash masking failed");
    size_t err = 0;
    for (size_t b = 0; b < strlen(str); ++b) {
      FIO_T_ASSERT(tmp[b] != str[b] || (err < 2),
                   "Risky Hash masking didn't mask buf[%zu] on offset "
                   "%d (statistical deviation?)",
                   b, i);
      err += (tmp[b] == str[b]);
    }
    fio_risky_mask(tmp, strlen(str), (uint64_t)tmp, nonce);
    FIO_T_ASSERT(!memcmp(tmp, str, strlen(str)),
                 "Risky Hash masking RT failed");
  }
  const uint8_t alignment_test_offset = 0;
  if (alignment_test_offset)
    fprintf(stderr,
            "The following speed tests use a memory alignment offset of %d "
            "bytes.\n",
            (int)(alignment_test_offset & 7));
  fio_test_hash_function(fio___dynamic_types_test___risky_wrapper,
                         (char *)"fio_risky_hash", alignment_test_offset);
  // fio_test_hash_function(fio___dynamic_types_test___risky_stream_wrapper,
  //                        "fio_risky_hash (streaming)",
  //                        alignment_test_offset);
  fio_test_hash_function(fio___dynamic_types_test___risky_mask_wrapper,
                         (char *)"fio_risky_mask (Risky XOR + counter)",
                         alignment_test_offset);
  fio_test_hash_function(fio___dynamic_types_test___risky_mask_wrapper,
                         (char *)"fio_risky_mask (unaligned)", 1);
}

/* *****************************************************************************
FIOBJ and JSON testing
***************************************************************************** */

TEST_FUNC int fio___dynamic_types_test___fiobj_task(FIOBJ o, void *e_) {
  static size_t index = 0;
  int *expect = (int *)e_;
  if (expect[index] == -1) {
    FIO_T_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_ARRAY,
                 "each2 ordering issue [%zu] (array).", index);
  } else {
    FIO_T_ASSERT(FIO_NAME2(fiobj, i)(o) == expect[index],
                 "each2 ordering issue [%zu] (number) %ld != %d", index,
                 FIO_NAME2(fiobj, i)(o), expect[index]);
  }
  ++index;
  return 0;
}

TEST_FUNC void fio___dynamic_types_test___fiobj(void) {
  FIOBJ o = FIOBJ_INVALID;
  if (!FIOBJ_MARK_MEMORY_ENABLED) {
    FIO_LOG_WARNING("FIOBJ defined without allocation counter. "
                    "Tests might not be complete.");
  }
  /* primitives - (in)sanity */
  {
    fprintf(stderr, "* Testing FIOBJ primitives.\n");
    FIO_T_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_NULL,
                 "invalid FIOBJ type should be FIOBJ_T_NULL.");
    FIO_T_ASSERT(
        !FIO_NAME_BL(fiobj, eq)(o, FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
        "invalid FIOBJ is NOT a fiobj_null().");
    FIO_T_ASSERT(!FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(),
                                         FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
                 "fiobj_true() is NOT fiobj_null().");
    FIO_T_ASSERT(!FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_FALSE)(),
                                         FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
                 "fiobj_false() is NOT fiobj_null().");
    FIO_T_ASSERT(!FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_FALSE)(),
                                         FIO_NAME(fiobj, FIOBJ___NAME_TRUE)()),
                 "fiobj_false() is NOT fiobj_true().");
    FIO_T_ASSERT(FIOBJ_TYPE(FIO_NAME(fiobj, FIOBJ___NAME_NULL)()) ==
                     FIOBJ_T_NULL,
                 "fiobj_null() type should be FIOBJ_T_NULL.");
    FIO_T_ASSERT(FIOBJ_TYPE(FIO_NAME(fiobj, FIOBJ___NAME_TRUE)()) ==
                     FIOBJ_T_TRUE,
                 "fiobj_true() type should be FIOBJ_T_TRUE.");
    FIO_T_ASSERT(FIOBJ_TYPE(FIO_NAME(fiobj, FIOBJ___NAME_FALSE)()) ==
                     FIOBJ_T_FALSE,
                 "fiobj_false() type should be FIOBJ_T_FALSE.");
    FIO_T_ASSERT(FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_NULL)(),
                                        FIO_NAME(fiobj, FIOBJ___NAME_NULL)()),
                 "fiobj_null() should be equal to self.");
    FIO_T_ASSERT(FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(),
                                        FIO_NAME(fiobj, FIOBJ___NAME_TRUE)()),
                 "fiobj_true() should be equal to self.");
    FIO_T_ASSERT(FIO_NAME_BL(fiobj, eq)(FIO_NAME(fiobj, FIOBJ___NAME_FALSE)(),
                                        FIO_NAME(fiobj, FIOBJ___NAME_FALSE)()),
                 "fiobj_false() should be equal to self.");
  }
  {
    fprintf(stderr, "* Testing FIOBJ integers.\n");
    uint8_t allocation_flags = 0;
    for (uint8_t bit = 0; bit < (sizeof(intptr_t) * 8); ++bit) {
      uintptr_t i = (uintptr_t)1 << bit;
      o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)((intptr_t)i);
      FIO_T_ASSERT(FIO_NAME2(fiobj, i)(o) == (intptr_t)i,
                   "Number not reversible at bit %d (%zd != %zd)!", (int)bit,
                   (ssize_t)FIO_NAME2(fiobj, i)(o), (ssize_t)i);
      allocation_flags |= (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_NUMBER) ? 1 : 2;
      fiobj_free(o);
    }
    FIO_T_ASSERT(allocation_flags == 3,
                 "no bits are allocated / no allocations optimized away (%d)",
                 (int)allocation_flags);
  }
  {
    fprintf(stderr, "* Testing FIOBJ floats.\n");
    uint8_t allocation_flags = 0;
    for (uint8_t bit = 0; bit < (sizeof(double) * 8); ++bit) {
      union {
        double d;
        uint64_t i;
      } punned;
      punned.i = (uint64_t)1 << bit;
      o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(punned.d);
      FIO_T_ASSERT(FIO_NAME2(fiobj, f)(o) == punned.d,
                   "Float not reversible at bit %d (%lf != %lf)!", (int)bit,
                   FIO_NAME2(fiobj, f)(o), punned.d);
      allocation_flags |= (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_FLOAT) ? 1 : 2;
      fiobj_free(o);
    }
    FIO_T_ASSERT(allocation_flags == 3,
                 "no bits are allocated / no allocations optimized away (%d)",
                 (int)allocation_flags);
  }
  {
    fprintf(stderr, "* Testing FIOBJ each2.\n");
    FIOBJ a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(o, a);
    for (int i = 1; i < 10; ++i) // 1, 2, 3 ... 10
    {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(i));
      if (i % 3 == 0) {
        a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(o, a);
      }
    }
    int expectation[] = {
        -1 /* array */, -1, 1, 2, 3, -1, 4, 5, 6, -1, 7, 8, 9, -1};
    size_t c = fiobj_each2(o, fio___dynamic_types_test___fiobj_task,
                           (void *)expectation);
    FIO_T_ASSERT(c == FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o) +
                          9 + 1,
                 "each2 repetition count error");
    fiobj_free(o);
  }
  {
    fprintf(stderr, "* Testing FIOBJ JSON handling.\n");
    char json[] =
        "                    "
        "\n# comment 1"
        "\n// comment 2"
        "\n/* comment 3 */"
        "{\"true\":true,\"false\":false,\"null\":null,\"array\":[1,2,3,4.2,"
        "\"five\"],"
        "\"string\":\"hello\\tjson\\bworld!\\r\\n\",\"hash\":{\"true\":true,"
        "\"false\":false},\"array2\":[1,2,3,4.2,\"five\",{\"hash\":true}]}";
    o = fiobj_json_parse2(json, strlen(json), NULL);
    FIO_T_ASSERT(o, "JSON parsing failed - no data returned.");
    FIOBJ j = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, o, 0);
#if DEBUG
    fprintf(stderr, "JSON: %s\n", FIO_NAME2(fiobj, cstr)(j).buf);
#endif
    FIO_T_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(j) ==
                     strlen(json + 61),
                 "JSON roundtrip failed (length error).");
    FIO_T_ASSERT(
        !memcmp(json + 61,
                FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(j),
                strlen(json + 61)),
        "JSON roundtrip failed (data error).");
    fiobj_free(o);
    fiobj_free(j);
    o = FIOBJ_INVALID;
  }
  {
    fprintf(stderr, "* Testing FIOBJ array equality test (fiobj_is_eq).\n");
    FIOBJ a1 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIOBJ a2 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIOBJ n1 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIOBJ n2 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a1, fiobj_null());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a2, fiobj_null());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n1, fiobj_true());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n2, fiobj_true());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a1, n1);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a2, n2);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
    (a1, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new_cstr)("test", 4));
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
    (a2, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new_cstr)("test", 4));
    FIO_T_ASSERT(FIO_NAME_BL(fiobj, eq)(a1, a2), "equal arrays aren't equal?");
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n1, fiobj_null());
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(n2, fiobj_false());
    FIO_T_ASSERT(!FIO_NAME_BL(fiobj, eq)(a1, a2), "unequal arrays are equal?");
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(n1, -1, NULL);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(n2, -1, NULL);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(a1, 0, NULL);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(a2, -1, NULL);
    FIO_T_ASSERT(!FIO_NAME_BL(fiobj, eq)(a1, a2), "unequal arrays are equal?");
    fiobj_free(a1);
    fiobj_free(a2);
  }
  {
    fprintf(stderr, "* Testing FIOBJ array ownership.\n");
    FIOBJ a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    for (int i = 1; i <= TEST_REPEAT; ++i) {
      FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                           new_cstr)("number: ", 8);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a, tmp);
    }
    FIOBJ shifted = FIOBJ_INVALID;
    FIOBJ popped = FIOBJ_INVALID;
    FIOBJ removed = FIOBJ_INVALID;
    FIOBJ set = FIOBJ_INVALID;
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), shift)(a, &shifted);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), pop)(a, &popped);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), set)
    (a, 1, FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(), &set);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), remove)(a, 2, &removed);
    fiobj_free(a);
    if (1) {
      FIO_T_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(popped) ==
                  strlen("number: " FIO_MACRO2STR(TEST_REPEAT)) &&
              !memcmp(
                  "number: " FIO_MACRO2STR(TEST_REPEAT),
                  FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(popped),
                  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(popped)),
          "Object popped from Array lost it's value %s",
          FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(popped));
      FIO_T_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(shifted) == 9 &&
              !memcmp(
                  "number: 1",
                  FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(shifted),
                  9),
          "Object shifted from Array lost it's value %s",
          FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(shifted));
      FIO_T_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(set) == 9 &&
              !memcmp("number: 3",
                      FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set),
                      9),
          "Object retrieved from Array using fiobj_array_set() lost it's "
          "value %s",
          FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set));
      FIO_T_ASSERT(
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(removed) == 9 &&
              !memcmp(
                  "number: 4",
                  FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed),
                  9),
          "Object retrieved from Array using fiobj_array_set() lost it's "
          "value %s",
          FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed));
    }
    fiobj_free(shifted);
    fiobj_free(popped);
    fiobj_free(set);
    fiobj_free(removed);
  }
  {
    fprintf(stderr, "* Testing FIOBJ hash ownership after concat.\n");
    FIOBJ a1, a2;
    a1 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    a2 = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    for (int i = 0; i < TEST_REPEAT; ++i) {
      FIOBJ str = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(str, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a1, str);
    }
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), concat)(a2, a1);
    fiobj_free(a1);
    for (int i = 0; i < TEST_REPEAT; ++i) {
      FIOBJ_STR_TEMP_VAR(tmp);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_T_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(FIO_NAME(
                       FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(a2, i)) ==
                       FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(tmp),
                   "string length zeroed out - string freed?");
      FIO_T_ASSERT(
          !memcmp(FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(tmp),
                  FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(FIO_NAME(
                      FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(a2, i)),
                  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(tmp)),
          "string data error - string freed?");
      FIOBJ_STR_TEMP_DESTROY(tmp);
    }
    fiobj_free(a2);
  }
  {
    fprintf(stderr, "* Testing FIOBJ hash ownership.\n");
    o = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
    for (int i = 1; i <= TEST_REPEAT; ++i) {
      FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                           new_cstr)("number: ", 8);
      FIOBJ k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_i)(tmp, i);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set2)(o, k, tmp);
      fiobj_free(k);
    }

    FIOBJ set = FIOBJ_INVALID;
    FIOBJ removed = FIOBJ_INVALID;
    FIOBJ k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(1);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), remove2)(o, k, &removed);
    fiobj_free(k);
    k = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(2);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)
    (o, fiobj2hash(o, k), k, FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(), &set);
    fiobj_free(k);
    FIO_T_ASSERT(set,
                 "fiobj_hash_set2 didn't copy information to old pointer?");
    FIO_T_ASSERT(removed,
                 "fiobj_hash_remove2 didn't copy information to old pointer?");
    // fiobj_hash_set(o, uintptr_t hash, FIOBJ key, FIOBJ value, FIOBJ *old)
    FIO_T_ASSERT(
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(removed) ==
                strlen("number: 1") &&
            !memcmp(
                "number: 1",
                FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed),
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(removed)),
        "Object removed from Hash lost it's value %s",
        FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(removed));
    FIO_T_ASSERT(
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(set) ==
                strlen("number: 2") &&
            !memcmp("number: 2",
                    FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set),
                    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(set)),
        "Object removed from Hash lost it's value %s",
        FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(set));

    fiobj_free(removed);
    fiobj_free(set);
    fiobj_free(o);
  }

#if FIOBJ_MARK_MEMORY
  {
    fprintf(stderr, "* Testing FIOBJ for memory leaks.\n");
    FIOBJ a = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), reserve)(a, 64);
    for (uint8_t bit = 0; bit < (sizeof(intptr_t) * 8); ++bit) {
      uintptr_t i = (uintptr_t)1 << bit;
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)((intptr_t)i));
    }
    FIOBJ h = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
    FIOBJ key = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(key, "array", 5);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set2)(h, key, a);
    FIO_T_ASSERT(FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(h, key) ==
                     a,
                 "FIOBJ Hash retrival failed");
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(a, key);
    if (0) {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(fiobj, FIOBJ___NAME_NULL)());
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(fiobj, FIOBJ___NAME_TRUE)());
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(fiobj, FIOBJ___NAME_FALSE)());
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)
      (a, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(0.42));

      FIOBJ json = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, h, 0);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(json, "\n", 1);
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), reserve)
      (json, FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(json)
                 << 1); /* prevent memory realloc */
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_escape)
      (json, FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING), ptr)(json),
       FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(json) - 1);
      fprintf(stderr, "%s\n", FIO_NAME2(fiobj, cstr)(json).buf);
      fiobj_free(json);
    }
    fiobj_free(h);

    FIO_T_ASSERT(
        FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER,
        "FIOBJ leak detected (freed %zu/%zu)", FIOBJ_MARK_MEMORY_FREE_COUNTER,
        FIOBJ_MARK_MEMORY_ALLOC_COUNTER);
  }
#endif
  fprintf(stderr, "* Passed.\n");
}

/* *****************************************************************************
Environment printout
***************************************************************************** */

#define FIO_PRINT_SIZE_OF(T) fprintf(stderr, "\t" #T "\t%zu Bytes\n", sizeof(T))

TEST_FUNC void fio___dynamic_types_test___print_sizes(void) {
  switch (sizeof(void *)) {
  case 2:
    fprintf(stderr, "* 16bit words size (unexpected, unknown effects).\n");
    break;
  case 4:
    fprintf(stderr, "* 32bit words size (some features might be slower).\n");
    break;
  case 8:
    fprintf(stderr, "* 64bit words size okay.\n");
    break;
  case 16:
    fprintf(stderr, "* 128bit words size... wow!\n");
    break;
  default:
    fprintf(stderr, "* Unknown words size %zubit!\n", sizeof(void *) << 3);
    break;
  }
  fprintf(stderr, "* Using the following type sizes:\n");
  FIO_PRINT_SIZE_OF(char);
  FIO_PRINT_SIZE_OF(short);
  FIO_PRINT_SIZE_OF(int);
  FIO_PRINT_SIZE_OF(float);
  FIO_PRINT_SIZE_OF(long);
  FIO_PRINT_SIZE_OF(double);
  FIO_PRINT_SIZE_OF(size_t);
  FIO_PRINT_SIZE_OF(void *);
}
#undef FIO_PRINT_SIZE_OF

/* *****************************************************************************
Testing functiun
***************************************************************************** */

TEST_FUNC void fio____test_dynamic_types__stack_poisoner(void) {
  const size_t len = 1UL << 16;
  uint8_t buf[len];
  __asm__ __volatile__("" ::: "memory");
  memset(buf, (int)(~0U), len);
  __asm__ __volatile__("" ::: "memory");
  fio_trylock(buf);
}

TEST_FUNC void fio_test_dynamic_types(void) {
  char *filename = (char *)FIO__FILE__;
  while (filename[0] == '.' && filename[1] == '/')
    filename += 2;
  fio____test_dynamic_types__stack_poisoner();
  fprintf(stderr, "===============\n");
  fprintf(stderr, "Testing Dynamic Types (%s)\n", filename);
  fprintf(
      stderr,
      "facil.io core: version \x1B[1m" FIO_VERSION_STRING "\x1B[0m\n"
      "The facil.io library was originally coded by \x1B[1mBoaz Segev\x1B[0m.\n"
      "Please give credit where credit is due.\n"
      "\x1B[1mYour support is only fair\x1B[0m - give value for value.\n"
      "(code contributions / donations)\n\n");
  fprintf(stderr, "===============\n");
  FIO_LOG_DEBUG("example FIO_LOG_DEBUG message.");
  FIO_LOG_DEBUG2("example FIO_LOG_DEBUG2 message.");
  FIO_LOG_INFO("example FIO_LOG_INFO message.");
  FIO_LOG_WARNING("example FIO_LOG_WARNING message.");
  FIO_LOG_SECURITY("example FIO_LOG_SECURITY message.");
  FIO_LOG_ERROR("example FIO_LOG_ERROR message.");
  FIO_LOG_FATAL("example FIO_LOG_FATAL message.");
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___print_sizes();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___random();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___atomic();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___bitwise();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___atol();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___url();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___linked_list_test();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___array_test();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___map_test();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___str();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___gmtime();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___queue();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___cli();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___mem();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___sock();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___fiobj();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___risky();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___lock2_speed();
  fprintf(stderr, "===============\n");
  {
    char timebuf[64];
    fio_time2rfc7231(timebuf, fio_time_real().tv_sec);
    fprintf(stderr,
            "On %s\n"
            "Testing \x1B[1mPASSED\x1B[0m "
            "for facil.io core version: "
            "\x1B[1m" FIO_VERSION_STRING "\x1B[0m"
            "\n",
            timebuf);
  }
  fprintf(stderr,
          "\nThe facil.io library was originally coded by \x1B[1mBoaz "
          "Segev\x1B[0m.\n"
          "Please give credit where credit is due.\n"
          "\x1B[1mYour support is only fair\x1B[0m - give value for value.\n"
          "(code contributions / donations)\n\n");
#if !defined(FIO_NO_COOKIE)
  fio___();
#endif
}

/* *****************************************************************************
Testing cleanup
***************************************************************************** */

#undef TEST_REPEAT
#undef TEST_FUNC
#undef FIO_T_ASSERT

#endif /* FIO_EXTERN_COMPLETE */
#endif
