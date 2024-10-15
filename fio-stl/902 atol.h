/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_ATOL Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_ATOL_TEST___H)
#define H___FIO_ATOL_TEST___H
#ifndef H___FIO_ATOL___H
#define FIO_ATOL
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

#define FIO_ATOL_TEST_MAX 1048576

FIO_IFUNC int64_t FIO_NAME_TEST(stl, atol_time)(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return ((int64_t)t.tv_sec * 1000000) + (int64_t)t.tv_nsec / 1000;
}

FIO_SFUNC double fio___aton_float_wrapper(char **pstr) {
  fio_aton_s r = fio_aton(pstr);
  if (r.is_float)
    return r.f;
  return (double)r.i;
}

FIO_SFUNC double fio___strtod_wrapper(char **pstr) {
  return strtod(*pstr, pstr);
}

FIO_SFUNC void FIO_NAME_TEST(stl, aton_speed)(void) {
  struct {
    const char *n;
    double (*fn)(char **);
  } to_test[] = {
      {.n = "fio_aton", .fn = fio___aton_float_wrapper},
      {.n = "strtod  ", .fn = fio___strtod_wrapper},
  };
  const char *floats[] = {
      "inf",
      "nan",
      "-inf",
      "-nan",
      "infinity",
      "1E+1000",
      "1E-1000",
      "1E+10",
      "1E-10",
      "-1E10",
      "-1e10",
      "-1E+10",
      "-1E-10",
      "1.234E+10",
      "1.234E-10",
      "1.79769e+308",
      "2.22507e-308",
      "1.79769e+308",
      "2.22507e-308",
      "4.9406564584124654e-324",
      "2.2250738585072009e-308",
      "2.2250738585072014e-308",
      "1.7976931348623157e+308",
      "2.171e-308",
      "2.2250738585072012e-308", /* possible infinit loop bug for strtod */
      "1.0020284025808569e-134",
      "1.00000000000000011102230246251565404236316680908203124",
      "72057594037927928.0",
      "7205759403792793200001e-5",
      "5708990770823839207320493820740630171355185152001e-3",
      "0x10.1p0",
      "0x1.8p1",
      "0x1.8p5",
      "0x4.0p5",
      "0x1.0p50a",
      "0x1.0p500",
      "0x1.0P-1074",
      "0x3a.0P-1074",
      "0x0.f9c7573d7fe52p-1022",
  };
  printf("* Testing fio_aton/strtod performance:\n");
  /* Sanity Test */
  bool rounding_errors_detected = 0;
  for (size_t n_i = 0; n_i < sizeof(floats) / sizeof(floats[0]); ++n_i) {
    union {
      double f;
      uint64_t u64;
    } u1, u2;
    char *tmp = (char *)floats[n_i];
    u1.f = to_test[0].fn(&tmp);
    for (size_t fn_i = 1; fn_i < sizeof(to_test) / sizeof(to_test[0]); ++fn_i) {
      char *tmp2 = (char *)floats[n_i];
      u2.f = to_test[fn_i].fn(&tmp2);
      if (tmp2 == tmp) {
        if ((isnan(u1.f) && isnan(u2.f)) || u1.u64 == u2.u64)
          continue;
        rounding_errors_detected = 1;
#ifdef DEBUG
        FIO_LOG_WARNING("Rounding error for %s:\n\t%.17g ?= %.17g",
                        floats[n_i],
                        u1.f,
                        u2.f);
#endif
        if (u1.u64 + 1 == u2.u64)
          continue;
        if (u2.u64 + 1 == u1.u64)
          continue;
      }
      FIO_ASSERT(tmp2 == tmp && u1.u64 == u2.u64,
                 "Sanity test failed for %s\n\t %.17g ?!= %.17g\n\t %s ?!= %s",
                 (char *)floats[n_i],
                 u1.f,
                 u2.f,
                 tmp,
                 tmp2);
    }
  }
  /* Speed Test */
  for (size_t fn_i = 0; fn_i < sizeof(to_test) / sizeof(to_test[0]); ++fn_i) {
    double unused;
    printf("\t%s\t", to_test[fn_i].n);
    int64_t start = FIO_NAME_TEST(stl, atol_time)();
    for (size_t i = 0; i < (FIO_ATOL_TEST_MAX / 10); ++i) {
      for (size_t n_i = 0; n_i < sizeof(floats) / sizeof(floats[0]); ++n_i) {
        char *tmp = (char *)floats[n_i];
        unused = to_test[fn_i].fn(&tmp);
        FIO_COMPILER_GUARD;
      }
    }
    (void)unused;
    int64_t end = FIO_NAME_TEST(stl, atol_time)();
    printf("%lld us\n", (long long int)(end - start));
  }
  if (rounding_errors_detected)
    FIO_LOG_WARNING("Single bit rounding errors detected when comparing "
                    "`fio_aton` to `strtod`.\n");
}

FIO_SFUNC size_t sprintf_wrapper(char *dest, int64_t num, uint8_t base) {
  switch (base) {
  case 2: /* overflow - unsupported */
  case 8: /* overflow - unsupported */
  case 10: return snprintf(dest, 256, "%" PRId64, num);
  case 16:
    if (num >= 0)
      return snprintf(dest, 256, "0x%.16" PRIx64, num);
    return snprintf(dest, 256, "-0x%.8" PRIx64, (0 - num));
  }
  return snprintf(dest, 256, "%" PRId64, num);
}

FIO_SFUNC int64_t strtoll_wrapper(char **pstr) {
  return strtoll(*pstr, pstr, 0);
}
FIO_SFUNC int64_t fio_aton_wrapper(char **pstr) {
  fio_aton_s r = fio_aton(pstr);
  if (r.is_float)
    return (int64_t)r.f;
  return r.i;
}

FIO_SFUNC void FIO_NAME_TEST(stl, atol_speed)(const char *name,
                                              int64_t (*a2l)(char **),
                                              size_t (*l2a)(char *,
                                                            int64_t,
                                                            uint8_t)) {
  int64_t start;
  int64_t tw = 0;
  int64_t trt = 0;
  char buf[1024];
  struct {
    const char *str;
    const char *prefix;
    uint8_t prefix_len;
    uint8_t base;
  } * pb, b[] = {
              {.str = "Base 10", .base = 10},
              {.str = "Hex    ", .prefix = "0x", .prefix_len = 2, .base = 16},
              {.str = "Binary ", .prefix = "0b", .prefix_len = 2, .base = 2},
              // {.str = "Oct    ", .prefix = "0", .prefix_len = 1, .base = 8},
              /* end marker */
              {.str = NULL},
          };
  fprintf(stderr, "    * %s test performance:\n", name);
  if (l2a == sprintf_wrapper)
    b[2].str = NULL;
  for (pb = b; pb->str; ++pb) {
    start = FIO_NAME_TEST(stl, atol_time)();
    for (int64_t i = -FIO_ATOL_TEST_MAX; i < FIO_ATOL_TEST_MAX; ++i) {
      char *bf = buf + pb->prefix_len;
      size_t len = l2a(bf, i, pb->base);
      bf[len] = 0;
      if (bf[0] == '-') {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test + 1] == pb->prefix[pre_test])
            continue;
          FIO_MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      } else {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test] == pb->prefix[pre_test])
            continue;
          FIO_MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      }
      FIO_COMPILER_GUARD; /* don't optimize this loop */
      int64_t n = a2l(&bf);
      bf = buf;
      FIO_ASSERT(n == i,
                 "roundtrip error for %s: %s != %lld (got %lld stopped: %s)",
                 name,
                 buf,
                 i,
                 a2l(&bf),
                 bf);
    }
    trt = FIO_NAME_TEST(stl, atol_time)() - start;
    start = FIO_NAME_TEST(stl, atol_time)();
    for (int64_t i = -FIO_ATOL_TEST_MAX; i < FIO_ATOL_TEST_MAX; ++i) {
      char *bf = buf + pb->prefix_len;
      size_t len = l2a(bf, i, pb->base);
      bf[len] = 0;
      if (bf[0] == '-') {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test + 1] == pb->prefix[pre_test])
            continue;
          FIO_MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      } else {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test] == pb->prefix[pre_test])
            continue;
          FIO_MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      }
      FIO_COMPILER_GUARD; /* don't optimize this loop */
    }
    tw = FIO_NAME_TEST(stl, atol_time)() - start;
    // clang-format off
    fprintf(stderr, "        - %s roundtrip   %zd us\n", pb->str, (size_t)trt);
    fprintf(stderr, "        - %s write       %zd us\n", pb->str, (size_t)tw);
    fprintf(stderr, "        - %s read (calc) %zd us\n", pb->str, (size_t)(trt - tw));
    // clang-format on
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, atol)(void) {
  fprintf(stderr, "* Testing fio_atol and fio_ltoa.\n");
  char buffer[1024];
  for (int i = 0 - FIO_ATOL_TEST_MAX; i < FIO_ATOL_TEST_MAX; ++i) {
    size_t tmp = fio_ltoa(buffer, i, 0);
    FIO_ASSERT(tmp > 0, "fio_ltoa returned length error");
    char *tmp2 = buffer;
    int i2 = fio_atol(&tmp2);
    FIO_ASSERT(tmp2 > buffer, "fio_atol pointer motion error (1:%i)", i);
    FIO_ASSERT(i == i2,
               "fio_ltoa-fio_atol roundtrip error %lld != %lld",
               i,
               i2);
  }
  for (size_t bit = 0; bit < sizeof(int64_t) * 8; ++bit) {
    uint64_t i = (uint64_t)1 << bit;
    size_t tmp = fio_ltoa(buffer, (int64_t)i, 0);
    FIO_ASSERT(tmp > 0, "fio_ltoa return length error");
    buffer[tmp] = 0;
    char *tmp2 = buffer;
    int64_t i2 = fio_atol(&tmp2);
    FIO_ASSERT(tmp2 > buffer, "fio_atol pointer motion error (2:%zu)", bit);
    FIO_ASSERT((int64_t)i == i2,
               "fio_ltoa-fio_atol roundtrip error %lld != %lld",
               i,
               i2);
  }
  for (unsigned char i = 0; i < 36; ++i) {
    FIO_ASSERT(i == fio_c2i(fio_i2c(i)), "fio_c2i / fio_i2c roundtrip error.");
  }
  for (size_t i = 1; i < (1ULL << 10); ++i) {
    union {
      double d;
      void *p;
    } e[2], r[2];
    e[0].d = (1.0 + i);
    e[1].d = (1.0 - i);
    r[0].d = fio_i2d(1LL + i, 0);
    r[1].d = fio_i2d(1LL - i, 0);
    FIO_ASSERT(e[0].d == r[0].d,
               "fio_i2d failed at (1+%zu) %g != %g\n\t%p != %p",
               i,
               e[0].d,
               r[0].d,
               e[0].p,
               r[0].p);
    FIO_ASSERT(e[1].d == r[1].d,
               "fio_i2d failed at (1-%zu) %g != %g\n\t%p != %p",
               i,
               e[1].d,
               r[1].d,
               e[1].p,
               r[1].p);
  }
#if 1 || !(DEBUG - 1 + 1)
  {
    uint64_t start, end, rep = (1ULL << 22);
    int64_t u64[128] = {0};
    double dbl[128] = {0.0};
    double rtest;
    fprintf(stderr, "* Testing fio_i2d conversion overhead.\n");
    start = fio_time_micro();
    for (size_t i = 0; i < rep; ++i) {
      u64[i & 127] -= i;
      FIO_COMPILER_GUARD;
      dbl[i & 127] += 2.0 * u64[i & 127];
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr, "\t- C cast:  %zuus\n", (size_t)(end - start));
    rtest = dbl[127];
    FIO_MEMSET(u64, 0, sizeof(u64));
    FIO_MEMSET(dbl, 0, sizeof(dbl));
    start = fio_time_micro();
    for (size_t i = 0; i < rep; ++i) {
      u64[i & 127] -= i;
      FIO_COMPILER_GUARD;
      dbl[i & 127] += fio_i2d((int64_t)u64[i & 127], 1);
      FIO_COMPILER_GUARD;
    }
    end = fio_time_micro();
    fprintf(stderr, "\t- fio_i2d: %zuus\n", (size_t)(end - start));
    FIO_ASSERT(rtest == dbl[127], "fio_i2d results not the same as C cast?");
  }
#endif
  fprintf(stderr, "* Testing fio_atol samples.\n");
#define TEST_ATOL(s_, n)                                                       \
  do {                                                                         \
    char *s = (char *)s_;                                                      \
    char *p = (char *)(s);                                                     \
    int64_t r = fio_atol(&p);                                                  \
    FIO_ASSERT(r == (n),                                                       \
               "fio_atol test error! %s => %zd (not %zd)",                     \
               ((char *)(s)),                                                  \
               (size_t)r,                                                      \
               (size_t)n);                                                     \
    FIO_ASSERT((s) + FIO_STRLEN((s)) == p,                                     \
               "fio_atol test error! %s reading position not at end "          \
               "(!%zu == %zu)\n\t0x%p - 0x%p",                                 \
               (s),                                                            \
               (size_t)FIO_STRLEN((s)),                                        \
               (size_t)(p - (s)),                                              \
               (void *)p,                                                      \
               (void *)s);                                                     \
    char buf[96];                                                              \
    buf[0] = '0';                                                              \
    buf[1] = 'b';                                                              \
    buf[fio_ltoa(buf + 2, n, 2) + 2] = 0;                                      \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 2 test error! "                                  \
               "%s != %s (%zd)",                                               \
               buf,                                                            \
               ((char *)(s)),                                                  \
               (size_t)((p = buf), fio_atol(&p)));                             \
    fio_ltoa(buf, n, 8);                                                       \
    p = buf;                                                                   \
    p += buf[0] == '-';                                                        \
    FIO_ASSERT((r = (int64_t)fio_atol8u(&p)) ==                                \
                   ((buf[0] == '-') ? (0 - (n)) : (n)),                        \
               "fio_ltoa base 8 test error! "                                  \
               "%s != %s (%zd)",                                               \
               buf,                                                            \
               ((char *)(s)),                                                  \
               (size_t)r);                                                     \
    buf[fio_ltoa(buf, n, 10)] = 0;                                             \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 10 test error! "                                 \
               "%s != %s (%zd)",                                               \
               buf,                                                            \
               ((char *)(s)),                                                  \
               (size_t)((p = buf), fio_atol(&p)));                             \
    buf[0] = '0';                                                              \
    buf[1] = 'x';                                                              \
    buf[fio_ltoa(buf + 2, n, 16) + 2] = 0;                                     \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 16 test error! "                                 \
               "%s != %s (%zd)",                                               \
               buf,                                                            \
               ((char *)(s)),                                                  \
               (size_t)((p = buf), fio_atol(&p)));                             \
  } while (0)

  TEST_ATOL("0x1", 1);
  TEST_ATOL("-0x1", -1);
  TEST_ATOL("-0xa", -10);                                  /* sign before hex */
  TEST_ATOL("0xe5d4c3b2a1908770", -1885667171979196560LL); /* sign within hex */
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
  TEST_ATOL("9223372036854775806",
            9223372036854775806LL); /* almost INT64_MAX */
#undef TEST_ATOL

#define TEST_LTOA_DIGITS10(num, digits)                                        \
  FIO_ASSERT(fio_digits10(num) == digits,                                      \
             "fio_digits10 failed for " #num " != (%zu)",                      \
             (size_t)fio_digits10(num));                                       \
  {                                                                            \
    char *number_str__ = (char *)#num;                                         \
    char *pstr__ = number_str__;                                               \
    FIO_ASSERT(fio_atol10(&pstr__) == num, "fio_atol10 failed for " #num);     \
  }
  TEST_LTOA_DIGITS10(1LL, 1);
  TEST_LTOA_DIGITS10(22LL, 2);
  TEST_LTOA_DIGITS10(333LL, 3);
  TEST_LTOA_DIGITS10(4444LL, 4);
  TEST_LTOA_DIGITS10(55555LL, 5);
  TEST_LTOA_DIGITS10(666666LL, 6);
  TEST_LTOA_DIGITS10(7777777LL, 7);
  TEST_LTOA_DIGITS10(88888888LL, 8);
  TEST_LTOA_DIGITS10(999999999LL, 9);
  TEST_LTOA_DIGITS10(-1LL, (1 + 1));
  TEST_LTOA_DIGITS10(-22LL, (2 + 1));
  TEST_LTOA_DIGITS10(-333LL, (3 + 1));
  TEST_LTOA_DIGITS10(-4444LL, (4 + 1));
  TEST_LTOA_DIGITS10(-55555LL, (5 + 1));
  TEST_LTOA_DIGITS10(-666666LL, (6 + 1));
  TEST_LTOA_DIGITS10(-7777777LL, (7 + 1));
  TEST_LTOA_DIGITS10(-88888888LL, (8 + 1));
  TEST_LTOA_DIGITS10(-999999999LL, (9 + 1));
  TEST_LTOA_DIGITS10(-9223372036854775807LL, (19 + 1));
#undef TEST_LTOA_DIGITS10

#define TEST_LTOA_DIGITS16(num, digits)                                        \
  FIO_ASSERT(fio_digits16u(num) == digits,                                     \
             "fio_digits16u failed for " #num " != (%zu)",                     \
             (size_t)fio_digits16u(num));                                      \
  {                                                                            \
    char *number_str__ = (char *)#num;                                         \
    char *pstr__ = number_str__;                                               \
    FIO_ASSERT(fio_atol16u(&pstr__) == (uint64_t)(num),                        \
               "fio_atol16u failed for " #num " != %zu",                       \
               ((pstr__ = number_str__), (size_t)fio_atol16u(&pstr__)));       \
  }
  TEST_LTOA_DIGITS16(0x00ULL, 2);
  TEST_LTOA_DIGITS16(0x10ULL, 2);
  TEST_LTOA_DIGITS16(0x100ULL, 4);
  TEST_LTOA_DIGITS16(0x10000ULL, 6);
  TEST_LTOA_DIGITS16(0xFFFFFFULL, 6);
  TEST_LTOA_DIGITS16(0x1000000ULL, 8);
  TEST_LTOA_DIGITS16(0x10000000ULL, 8);
  TEST_LTOA_DIGITS16(0x100000000ULL, 10);
  TEST_LTOA_DIGITS16(0x10000000000ULL, 12);
  TEST_LTOA_DIGITS16(0x1000000000000ULL, 14);
  TEST_LTOA_DIGITS16(0x100000000000000ULL, 16);
  TEST_LTOA_DIGITS16(0xFF00000000000000ULL, 16);
#undef TEST_LTOA_DIGITS16

#define TEST_LTOA_DIGITS_BIN(num, digits)                                      \
  FIO_ASSERT(fio_digits_bin(num) == digits,                                    \
             "fio_digits_bin failed for " #num " != (%zu)",                    \
             (size_t)fio_digits_bin(num));

  TEST_LTOA_DIGITS_BIN(0x00ULL, 1);
  TEST_LTOA_DIGITS_BIN(-0x01ULL, 64);
  TEST_LTOA_DIGITS_BIN(0x10ULL, 6);
  TEST_LTOA_DIGITS_BIN(0x100ULL, 10);
  TEST_LTOA_DIGITS_BIN(0x10000ULL, 18);
  TEST_LTOA_DIGITS_BIN(0x20000ULL, 18);
  TEST_LTOA_DIGITS_BIN(0xFFFFFFULL, 24);
  TEST_LTOA_DIGITS_BIN(0x1000000ULL, 26);
  TEST_LTOA_DIGITS_BIN(0x10000000ULL, 30);
  TEST_LTOA_DIGITS_BIN(0x100000000ULL, 34);
  TEST_LTOA_DIGITS_BIN(0x10000000000ULL, 42);
  TEST_LTOA_DIGITS_BIN(0x1000000000000ULL, 50);
  TEST_LTOA_DIGITS_BIN(0x100000000000000ULL, 58);
  TEST_LTOA_DIGITS_BIN(0xFF00000000000000ULL, 64);
#undef TEST_LTOA_DIGITS_BIN

  FIO_NAME_TEST(stl, atol_speed)("fio_atol/fio_ltoa", fio_atol, fio_ltoa);
  FIO_NAME_TEST(stl, atol_speed)
  ("fio_aton/fio_ltoa", fio_aton_wrapper, fio_ltoa);

  FIO_NAME_TEST(stl, atol_speed)
  ("system strtoll/sprintf", strtoll_wrapper, sprintf_wrapper);
  FIO_NAME_TEST(stl, aton_speed)();

#define TEST_DOUBLE(s, d, stop)                                                \
  do {                                                                         \
    union {                                                                    \
      double d_;                                                               \
      uint64_t as_i;                                                           \
    } pn, pn1, pn2;                                                            \
    pn2.d_ = d;                                                                \
    char *p = (char *)(s);                                                     \
    char *p1 = (char *)(s);                                                    \
    char *p2 = (char *)(s);                                                    \
    double r = fio_atof(&p);                                                   \
    fio_aton_s num_result = fio_aton(&p1);                                     \
    double r2 = num_result.is_float ? num_result.f : (double)num_result.i;     \
    double std = strtod(p2, &p2);                                              \
    (void)std;                                                                 \
    pn.d_ = r;                                                                 \
    pn1.d_ = r2;                                                               \
    FIO_ASSERT(                                                                \
        *p == stop || p == p2,                                                 \
        "atof float parsing didn't stop at correct position! %x != %x\n%s",    \
        *p,                                                                    \
        stop,                                                                  \
        (s));                                                                  \
    FIO_ASSERT(*p1 == stop || p1 == p2,                                        \
               "aton float parsing didn't stop at correct position!\n\t%s"     \
               "\n\t%x != %x",                                                 \
               s,                                                              \
               *p1,                                                            \
               stop);                                                          \
    if (((double)d == r && (double)d == r2) || (r == std && r2 == std)) {      \
      /** fprintf(stderr, "Okay for %s\n", s); */                              \
    } else if ((pn2.as_i + 1) == (pn.as_i) || (pn.as_i + 1) == pn2.as_i) {     \
      if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_DEBUG)                                \
        FIO_LOG_WARNING("Single bit rounding error detected (%s1): %s\n",      \
                        ((pn2.as_i + 1) == (pn.as_i) ? "-" : "+"),             \
                        s);                                                    \
    } else if ((pn1.as_i + 1) == (pn.as_i) || (pn.as_i + 1) == pn1.as_i) {     \
      if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_DEBUG)                                \
        FIO_LOG_WARNING("aton Single bit rounding error detected (%s1): %s\n"  \
                        "\t%g != %g",                                          \
                        ((pn1.as_i + 1) == (pn.as_i) ? "-" : "+"),             \
                        s,                                                     \
                        r2,                                                    \
                        std);                                                  \
    } else if (r == 0.0 && d != 0.0 && !isnan((double)d)) {                    \
      if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_DEBUG)                                \
        FIO_LOG_WARNING("float range limit marked before: %s\n", s);           \
    } else if (r2 == 0.0 && d != 0.0 && !isnan((double)d)) {                   \
      if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_DEBUG)                                \
        FIO_LOG_WARNING("aton float range limit marked before: %s\n", s);      \
    } else {                                                                   \
      char f_buf[256];                                                         \
      pn.d_ = std;                                                             \
      pn2.d_ = r;                                                              \
      size_t tmp_pos = fio_ltoa(f_buf, pn2.as_i, 2);                           \
      f_buf[tmp_pos++] = '\n';                                                 \
      tmp_pos += fio_ltoa(f_buf + tmp_pos, pn.as_i, 2);                        \
      f_buf[tmp_pos++] = '\n';                                                 \
      fio_ltoa(f_buf + tmp_pos, pn1.as_i, 2);                                  \
      FIO_ASSERT(0,                                                            \
                 "Float error bigger than a single bit rounding error."        \
                 "\n\tString: %s"                                              \
                 "\n\texp. "                                                   \
                 "vs. act.:\nstd %.19g\natof %.19g\naton %.19g\nBinary:\n%s",  \
                 s,                                                            \
                 std,                                                          \
                 r,                                                            \
                 r2,                                                           \
                 f_buf);                                                       \
    }                                                                          \
  } while (0)

  fprintf(stderr, "* Testing fio_atof & fio_aton samples.\n");

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
  TEST_DOUBLE("007", 7, 0);
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
              1E80,
              0);
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
              2.2250738585072014e-308,
              0);

  TEST_DOUBLE("0.999999999999999944488848768742172978818416595458984375",
              1.0,
              0);
  TEST_DOUBLE("0.999999999999999944488848768742172978818416595458984376",
              1.0,
              0);
  TEST_DOUBLE("1.00000000000000011102230246251565404236316680908203125",
              1.0,
              0);
  TEST_DOUBLE("1.00000000000000011102230246251565404236316680908203124",
              1.0,
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
              10141204801825834086073718800384.0,
              0);
  TEST_DOUBLE("10141204801825835211973625643008",
              10141204801825835211973625643008.0,
              0);
  TEST_DOUBLE("10141204801825834649023672221696",
              10141204801825835211973625643008.0,
              0);
  TEST_DOUBLE("1014120480182583464902367222169600001e-5",
              10141204801825835211973625643008.0,
              0);

  TEST_DOUBLE("5708990770823838890407843763683279797179383808",
              5708990770823838890407843763683279797179383808.0,
              0);
  TEST_DOUBLE("5708990770823839524233143877797980545530986496",
              5708990770823839524233143877797980545530986496.0,
              0);
  TEST_DOUBLE("5708990770823839207320493820740630171355185152",
              5708990770823839524233143877797980545530986496.0,
              0);
  TEST_DOUBLE("5708990770823839207320493820740630171355185152001e-3",
              5708990770823839524233143877797980545530986496.0,
              0);
#undef TEST_DOUBLE
#if !DEBUG
  {
    clock_t start, stop;
    fio_memcpy15x(buffer, "1234567890.123", 14);
    buffer[14] = 0;
    volatile size_t r = 0;
    start = clock();
    for (int i = 0; i < (FIO_ATOL_TEST_MAX << 3); ++i) {
      char *pos = buffer;
      r += fio_atol(&pos);
      FIO_COMPILER_GUARD;
      // FIO_ASSERT(r == exp, "fio_atol failed during speed test");
    }
    stop = clock();
    fprintf(stderr,
            "* fio_atol speed test completed in %zu cycles\n",
            stop - start);
    r = 0;

    start = clock();
    for (int i = 0; i < (FIO_ATOL_TEST_MAX << 3); ++i) {
      char *pos = buffer;
      r += strtol(pos, NULL, 10);
      FIO_COMPILER_GUARD;
      // FIO_ASSERT(r == exp, "system strtol failed during speed test");
    }
    stop = clock();
    fprintf(stderr,
            "* system atol speed test completed in %zu cycles\n",
            stop - start);
  }
#endif /* !DEBUG */
}
#undef FIO_ATOL_TEST_MAX

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
