/* *****************************************************************************
Test ATOL
***************************************************************************** */
#include "test-helpers.h"

#include <locale.h>

FIO_SFUNC double fio___aton_float_wrapper(char **pstr) {
  fio_aton_s r = fio_aton(pstr);
  if (r.is_float)
    return r.f;
  return (double)r.i;
}

FIO_SFUNC double fio___strtod_wrapper(char **pstr) {
  return strtod(*pstr, pstr);
}

FIO_SFUNC void FIO_NAME_TEST(stl, aton_vectors)(void) {
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
      "0.3",
      "3e-5",
      "123e-5",
      "012.5",
      "9007199254740993",
      "1e23",
      "2.4703282292062327e-324",
      "2.4703282292062328e-324",
      "1.7976931348623158e308",
      "1.7976931348623159e308",
      "0.000000000000000000000000000000000000001e39",
      "1000000000000000000000000000000000000000e-39",
  };
  fprintf(stderr, "\t* Testing fio_aton against strtod vectors:\n");
  /* Sanity Test */
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
      if (tmp2 == tmp && isnan(u1.f) && isnan(u2.f))
        continue;
      FIO_ASSERT(
          tmp2 == tmp && u1.u64 == u2.u64,
          "Sanity test failed for %s\n\t %.17g ?!= %.17g\n\t %s ?!= %s %s",
          (char *)floats[n_i],
          u1.f,
          u2.f,
          tmp,
          tmp2,
          (tmp == tmp2 ? "√" : "X"));
    }
  }
  /* Round-trip: shortest-exact (%.17g) decimals must parse bit-exact */
  for (size_t i = 0; i < (1ULL << 16); ++i) {
    union {
      double f;
      uint64_t u64;
    } u = {.u64 = fio_rand64()}, r;
    char buf[64];
    if (isnan(u.f) || isinf(u.f))
      continue;
    snprintf(buf, sizeof(buf), "%.17g", u.f);
    char *tmp = buf;
    r.f = fio___aton_float_wrapper(&tmp);
    FIO_ASSERT(r.u64 == u.u64 && !*tmp,
               "fio_aton round-trip error for %s (%.17g)",
               buf,
               r.f);
  }
}

/* Number formats: digit-less input, prefixes, binary floats, NaN, fio_atof */
FIO_SFUNC void FIO_NAME_TEST(stl, aton_formats)(void) {
  fprintf(stderr, "\t* Testing fio_aton / fio_atof formats:\n");
  union {
    double f;
    uint64_t u;
  } u;
  char buf[160];
  char *p;
  fio_aton_s r;
  /* input without digits is an error and isn't consumed */
  const char *bad[] = {"-", "+", ".", "e5", "-.e", "", "abc", " ._"};
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
    p = (char *)bad[i];
    r = fio_aton(&p);
    FIO_ASSERT(r.err && p == bad[i], "fio_aton should reject \"%s\"", bad[i]);
    p = (char *)bad[i];
    FIO_ASSERT(fio_atof(&p) == 0 && p == bad[i],
               "fio_atof should reject \"%s\"",
               bad[i]);
  }
  struct {
    const char *s;
    int64_t i;
    size_t len;
  } ints[] = {
      {"0", 0, 1},
      {"0x1F", 31, 4},
      {"x1F", 31, 3},
      {"b101", 5, 4},
      {"-0b101", -5, 6},
      {"0x", 0, 1},
      {"0b2", 0, 1},
      {"0123", 83, 4},
      {"1e", 1, 1},
      {"2E+x", 2, 1},
  };
  for (size_t i = 0; i < sizeof(ints) / sizeof(ints[0]); ++i) {
    p = (char *)ints[i].s;
    r = fio_aton(&p);
    FIO_ASSERT(!r.err && !r.is_float && r.i == ints[i].i &&
                   (size_t)(p - ints[i].s) == ints[i].len,
               "fio_aton integer error for \"%s\": %lld (%zu bytes)",
               ints[i].s,
               (long long)r.i,
               (size_t)(p - ints[i].s));
  }
  /* `0b...f` is a float's raw IEEE 754 bits; a leading `-` flips the sign */
  u.f = 1.5;
  buf[0] = '-';
  FIO_ASSERT(fio_ftoa(buf + 1, u.f, 2) == 2 + 62 + 1 && buf[1] == '0' &&
                 buf[2] == 'b' && buf[65] == 'f' && !buf[66],
             "fio_ftoa base 2 should write 0b...f: %s",
             buf + 1);
  for (size_t neg = 0; neg < 2; ++neg) {
    p = buf + !neg;
    r = fio_aton(&p);
    FIO_ASSERT(r.is_float && r.f == (neg ? -1.5 : 1.5) && !*p,
               "fio_aton binary float error for %s",
               buf + !neg);
    p = buf + !neg;
    FIO_ASSERT(fio_atof(&p) == (neg ? -1.5 : 1.5) && !*p,
               "fio_atof binary float error for %s",
               buf + !neg);
    buf[65] = 0; /* without the `f`: fio_atof still reads raw bits */
    p = buf + !neg;
    FIO_ASSERT(fio_atof(&p) == (neg ? -1.5 : 1.5) && !*p,
               "fio_atof binary (no f) error for %s",
               buf + !neg);
    buf[65] = 'f';
  }
  /* fio_ftoa base 2 round-trips any bit pattern (incl. NaN payloads, -0) */
  for (size_t i = 0; i < (1ULL << 12); ++i) {
    union {
      double f;
      uint64_t u;
    } src = {.u = fio_rand64() >> (i & 63)}, dst;
    if (i < 2)
      src.u = (uint64_t)i << 63; /* 0.0, -0.0 */
    fio_ftoa(buf, src.f, 2);
    p = buf;
    r = fio_aton(&p);
    FIO_ASSERT(r.is_float && r.u == src.u && !*p,
               "fio_aton 0b...f round-trip error for %s",
               buf);
    p = buf;
    dst.f = fio_atof(&p);
    FIO_ASSERT(dst.u == src.u && !*p, "fio_atof round-trip error: %s", buf);
  }
  /* fio_ftoa base 16 writes C99 `%a` hex floats, read back exactly */
  struct {
    double d;
    const char *s;
  } hexf[] = {
      {1.5, "0x1.8p+0"},
      {1.0, "0x1p+0"},
      {0.0, "0x0p+0"},
      {-0.0, "-0x0p+0"},
      {-0.1, "-0x1.999999999999ap-4"},
      {4.9406564584124654e-324, "0x0.0000000000001p-1022"},
      {1.7976931348623157e+308, "0x1.fffffffffffffp+1023"},
      {INFINITY, "Infinity"},
  };
  for (size_t i = 0; i < sizeof(hexf) / sizeof(hexf[0]); ++i) {
    size_t len = fio_ftoa(buf, hexf[i].d, 16);
    FIO_ASSERT(len == strlen(hexf[i].s) && !strcmp(buf, hexf[i].s),
               "fio_ftoa base 16 error: %s != %s",
               buf,
               hexf[i].s);
  }
  for (size_t i = 0; i < (1ULL << 12); ++i) {
    union {
      double f;
      uint64_t u;
    } src = {.u = fio_rand64() >> (i & 63)}, dst;
    if (isnan(src.f) || isinf(src.f))
      continue;
    fio_ftoa(buf, src.f, 16);
    p = buf;
    r = fio_aton(&p);
    FIO_ASSERT(r.is_float && r.u == src.u && !*p,
               "fio_aton hex float round-trip error for %s",
               buf);
    p = buf;
    dst.f = fio_atof(&p);
    FIO_ASSERT(dst.u == src.u && !*p, "fio_atof hex round-trip error: %s", buf);
    p = buf;
    dst.f = strtod(p, &p);
    FIO_ASSERT(dst.u == src.u && !*p, "strtod hex round-trip error: %s", buf);
  }
  /* fio_ftoa base 10: fast %g-compatible path, byte-identical to the
     snprintf("%g") + ".0" fixup reference (fuzz verified with 30M cases) */
  struct {
    double d;
    const char *s;
  } g10[] = {
      {0.0, "0.0"},
      {-0.0, "-0.0"},
      {1.0, "1.0"},
      {-1.0, "-1.0"},
      {0.5, "0.5"},
      {3.14, "3.14"},
      {1e-4, "0.0001"},
      {1e-5, "1e-05"},
      {1e5, "100000.0"},
      {1e6, "1e+06"},
      {999999.0, "999999.0"},
      {999999.5, "1e+06"},
      {1000000.0, "1e+06"},
      {123456.7, "123457.0"},
      {0.1, "0.1"},
      {2.0 / 3.0, "0.666667"},
      {123456789.0, "1.23457e+08"},
      {0.15625, "0.15625"},
      {25000.0, "25000.0"},
      {INFINITY, "Infinity"},
      {-INFINITY, "-Infinity"},
      {NAN, "NaN"},
  };
  for (size_t i = 0; i < sizeof(g10) / sizeof(g10[0]); ++i) {
    size_t len = fio_ftoa(buf, g10[i].d, 10);
    FIO_ASSERT(len == strlen(g10[i].s) && !strcmp(buf, g10[i].s),
               "fio_ftoa base 10 error: %s != %s",
               buf,
               g10[i].s);
  }
  /* randomized parity vs the snprintf("%g") reference (fixed seed) */
  {
    uint64_t ftoa_rnd = 0xF704A11;
    for (size_t i = 0; i < 65536; ++i) {
      union {
        double f;
        uint64_t u;
      } src;
      ftoa_rnd ^= ftoa_rnd << 13;
      ftoa_rnd ^= ftoa_rnd >> 7;
      ftoa_rnd ^= ftoa_rnd << 17;
      src.u = ftoa_rnd;
      if (isnan(src.f) || isinf(src.f))
        continue;
      char ref[64];
      snprintf(ref, 30, "%g", src.f);
      /* reference ".0" fixup (no 'e'/'.'/',' in output) */
      if (!strchr(ref, 'e') && !strchr(ref, '.') && !strchr(ref, ','))
        strcat(ref, ".0");
      fio_ftoa(buf, src.f, 10);
      FIO_ASSERT(!strcmp(buf, ref),
                 "fio_ftoa base 10 parity error: %s != %s (%.17g)",
                 buf,
                 ref,
                 src.f);
    }
  }
  /* fio_atol: a prefix counts only when a valid digit follows */
  struct {
    const char *s;
    int64_t i;
    size_t len;
  } atol_prefix[] = {
      {"x1F", 31, 3},
      {"-x10", -16, 4},
      {"b101", 5, 4},
      {"0x", 0, 1},
      {"0b", 0, 1},
      {"0b2", 0, 1},
      {"0xg", 0, 1},
  };
  for (size_t i = 0; i < sizeof(atol_prefix) / sizeof(atol_prefix[0]); ++i) {
    p = (char *)atol_prefix[i].s;
    int64_t n = fio_atol(&p);
    FIO_ASSERT(n == atol_prefix[i].i &&
                   (size_t)(p - atol_prefix[i].s) == atol_prefix[i].len,
               "fio_atol prefix error for \"%s\": %lld (%zu bytes)",
               atol_prefix[i].s,
               (long long)n,
               (size_t)(p - atol_prefix[i].s));
  }
  /* NaN is the standard quiet NaN; C's `nan(...)` payload isn't consumed */
  p = (char *)"nan(1)";
  r = fio_aton(&p);
  FIO_ASSERT(r.is_float && r.u == 0x7FF8000000000000ULL && *p == '(',
             "fio_aton nan error");
  p = (char *)"-NaN";
  r = fio_aton(&p);
  FIO_ASSERT(r.is_float && r.u == 0xFFF8000000000000ULL && !*p,
             "fio_aton -nan error");
  /* fio_atof: decimal leading zeros, signs, hex, large integers */
  struct {
    const char *s;
    double d;
    size_t len;
  } fl[] = {
      {"0123", 123.0, 4},
      {"-0123.5", -123.5, 7},
      {" +1.5", 1.5, 5},
      {"0x10", 16.0, 4},
      {"0xFFFFFFFFFFFFFFFF", 18446744073709551615.0, 18},
      {"-0.0", -0.0, 4},
      {"0.3", 0.3, 3},
      {"--1", 0.0, 0},
      {"- 1", 0.0, 0},
  };
  for (size_t i = 0; i < sizeof(fl) / sizeof(fl[0]); ++i) {
    p = (char *)fl[i].s;
    u.f = fio_atof(&p);
    FIO_ASSERT(u.f == fl[i].d && signbit(u.f) == signbit(fl[i].d) &&
                   (size_t)(p - fl[i].s) == fl[i].len,
               "fio_atof error for \"%s\": %.17g (%zu bytes)",
               fl[i].s,
               u.f,
               (size_t)(p - fl[i].s));
  }
  /* locale independence (skipped if the locale isn't installed) */
  if (setlocale(LC_NUMERIC, "de_DE.UTF-8")) {
    p = (char *)"1.5";
    FIO_ASSERT(fio_atof(&p) == 1.5 && !*p, "fio_atof depends on the locale");
    setlocale(LC_NUMERIC, "C");
  }
}

int main(void) {
  char buffer[1024];
  for (int i = 0 - FIO_ATOL_TEST_MAX; i < FIO_ATOL_TEST_MAX; ++i) {
    size_t tmp = fio_ltoa(buffer, i, 0);
    FIO_ASSERT(tmp > 0, "fio_ltoa returned length error");
    char *tmp2 = buffer;
    int i2 = (int)fio_atol(&tmp2);
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
  for (size_t i = 1; i < (~0ULL); i = ((i << 1U) | 1U)) {
    union {
      double d;
      void *p;
    } tst[2];
    tst[0].d = fio_u2d(i, 0);
    tst[1].d = (double)i;
    char buf[128];
    buf[0] = 'x';
    fio_ltoa16u(buf + 1, i, 16);
    buf[17] = 0;
    FIO_ASSERT(tst[0].d == tst[0].d,
               "fio_u2d failed (%s) %g != %g\n\t%p != %p",
               buf,
               tst[0].d,
               tst[1].d,
               tst[0].p,
               tst[1].p);
  }
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

  /* Strict, bounded variants (fio_stol10u / fio_stol10 / fio_stol16u) */
  {
    fprintf(stderr, "  * strict bounded number parsing\n");
    char *p, *base;
    /* fio_stol10u: digits only, bounded, E2BIG on overflow */
    p = (char *)"123";
    errno = 0;
    FIO_ASSERT(fio_stol10u(&p, p + 3) == 123 && p[0] == 0 && !errno,
               "fio_stol10u basic failed");
    /* NOTE: string literal pooling is unspecified (C11 6.4.5p7) - compare
     * against a named base pointer, never a second literal instance. */
    base = (char *)"12345";
    p = base;
    errno = 0;
    FIO_ASSERT(fio_stol10u(&p, base + 3) == 123 && p == base + 3,
               "fio_stol10u bound (end pointer) failed");
    p = (char *)"18446744073709551615"; /* UINT64_MAX */
    errno = 0;
    FIO_ASSERT(fio_stol10u(&p, p + 20) == ~(uint64_t)0ULL && !errno,
               "fio_stol10u UINT64_MAX failed");
    p = (char *)"18446744073709551616"; /* UINT64_MAX + 1 */
    errno = 0;
    FIO_ASSERT(fio_stol10u(&p, p + 20) == 1844674407370955161ULL &&
                   errno == E2BIG && p[0] == '6',
               "fio_stol10u overflow must set E2BIG and stop at last valid "
               "digit (got errno=%d)",
               errno);
    p = (char *)"1_0";
    errno = 0;
    FIO_ASSERT(fio_stol10u(&p, p + 3) == 1 && p[0] == '_' && !errno,
               "fio_stol10u must reject '_' separators (stop before '_')");
    p = (char *)" 12";
    errno = 0;
    FIO_ASSERT(fio_stol10u(&p, p + 3) == 0 && p[0] == ' ',
               "fio_stol10u must not skip whitespace");
    p = (char *)"-12";
    errno = 0;
    FIO_ASSERT(fio_stol10u(&p, p + 3) == 0 && p[0] == '-',
               "fio_stol10u must not accept a sign");
    p = (char *)"x";
    errno = 0;
    FIO_ASSERT(fio_stol10u(&p, p + 1) == 0 && p[0] == 'x',
               "fio_stol10u empty number must not advance pos");
    /* fio_stol10: optional sign + strict digits, signed-range E2BIG */
    p = (char *)"-123";
    errno = 0;
    FIO_ASSERT(fio_stol10(&p, p + 4) == -123 && !errno,
               "fio_stol10 negative failed");
    p = (char *)"+123";
    errno = 0;
    FIO_ASSERT(fio_stol10(&p, p + 4) == 123 && !errno,
               "fio_stol10 plus-sign failed");
    p = (char *)"9223372036854775807"; /* INT64_MAX */
    errno = 0;
    FIO_ASSERT(fio_stol10(&p, p + 19) == INT64_MAX && !errno,
               "fio_stol10 INT64_MAX failed");
    p = (char *)"9223372036854775808"; /* INT64_MAX + 1 */
    errno = 0;
    FIO_ASSERT(fio_stol10(&p, p + 19) == INT64_MAX && errno == E2BIG,
               "fio_stol10 must set E2BIG past INT64_MAX");
    p = (char *)"-9223372036854775808"; /* INT64_MIN */
    errno = 0;
    FIO_ASSERT(fio_stol10(&p, p + 20) == INT64_MIN && !errno,
               "fio_stol10 INT64_MIN failed");
    p = (char *)"-9223372036854775809"; /* INT64_MIN - 1 */
    errno = 0;
    FIO_ASSERT(fio_stol10(&p, p + 20) == INT64_MIN && errno == E2BIG,
               "fio_stol10 must set E2BIG past INT64_MIN");
    /* fio_stol16u: hex digits only, no 0x, bounded, E2BIG on overflow */
    p = (char *)"fF";
    errno = 0;
    FIO_ASSERT(fio_stol16u(&p, p + 2) == 255 && !errno,
               "fio_stol16u basic failed");
    p = (char *)"0x10";
    errno = 0;
    FIO_ASSERT(fio_stol16u(&p, p + 4) == 0 && p[0] == 'x',
               "fio_stol16u must NOT consume a 0x prefix");
    p = (char *)"ffffffffffffffff"; /* 16 x f = UINT64_MAX */
    errno = 0;
    FIO_ASSERT(fio_stol16u(&p, p + 16) == ~(uint64_t)0ULL && !errno,
               "fio_stol16u UINT64_MAX failed");
    p = (char *)"10000000000000000"; /* 17 digits */
    errno = 0;
    FIO_ASSERT(fio_stol16u(&p, p + 17) == 0x1000000000000000ULL &&
                   errno == E2BIG && p[0] == '0',
               "fio_stol16u overflow must set E2BIG and stop");
    p = (char *)"g1";
    errno = 0;
    FIO_ASSERT(fio_stol16u(&p, p + 2) == 0 && p[0] == 'g',
               "fio_stol16u non-hex must not advance pos");
    base = (char *)"abcdef";
    p = base;
    errno = 0;
    FIO_ASSERT(fio_stol16u(&p, base + 3) == 0xabc && p == base + 3,
               "fio_stol16u bound (end pointer) failed");
  }
  FIO_NAME_TEST(stl, aton_vectors)();
  FIO_NAME_TEST(stl, aton_formats)();

#define TEST_DOUBLE(s, d, stop)                                                \
  do {                                                                         \
    union {                                                                    \
      double d_;                                                               \
      uint64_t as_i;                                                           \
    } pn, pn1, pn2;                                                            \
    pn2.d_ = (double)d;                                                        \
    char *start = (char *)(s);                                                 \
    char *p = start;                                                           \
    char *p1 = start;                                                          \
    char *p2 = start;                                                          \
    double r = fio_atof(&p);                                                   \
    fio_aton_s num_result = fio_aton(&p1);                                     \
    double r2 = num_result.is_float ? num_result.f : (double)num_result.i;     \
    double std = strtod(p2, &p2);                                              \
    (void)std;                                                                 \
    pn.d_ = r;                                                                 \
    pn1.d_ = r2;                                                               \
    FIO_ASSERT(r2 == (double)d || (isnan(r2) && isnan((double)d)),             \
               "fio_aton rounding error for %s\n\t%.17g != %.17g",            \
               s,                                                              \
               r2,                                                             \
               (double)d);                                                     \
    FIO_ASSERT(                                                                \
        *p == stop || p == p2 || ((FIO_OS_WIN - 1 + 1) && p == start),         \
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
    } else if (r == 0.0 && (double)d != 0.0 && !isnan((double)d)) {            \
      if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_DEBUG)                                \
        FIO_LOG_WARNING("float range limit marked before: %s\n", s);           \
    } else if (r2 == 0.0 && (double)d != 0.0 && !isnan((double)d)) {           \
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
}
#undef FIO_ATOL_TEST_MAX
