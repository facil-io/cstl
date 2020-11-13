/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_ATOL                    /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#define FIO_TEST_CSTL               /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                            String <=> Number helpers










***************************************************************************** */
#if defined(FIO_ATOL) && !defined(H___FIO_ATOL_H)
#define H___FIO_ATOL_H
#include <inttypes.h>
#include <math.h>
/* *****************************************************************************
Strings to Numbers - API
***************************************************************************** */
/**
 * A helper function that converts between String data to a signed int64_t.
 *
 * Numbers are assumed to be in base 10. Octal (`0###`), Hex (`0x##`/`x##`) and
 * binary (`0b##`/ `b##`) are recognized as well. For binary Most Significant
 * Bit must come first.
 *
 * The most significant difference between this function and `strtol` (aside of
 * API design), is the added support for binary representations.
 */
SFUNC int64_t fio_atol(char **pstr);

/** A helper function that converts between String data to a signed double. */
SFUNC double fio_atof(char **pstr);

/**
 * Maps characters to alphanumerical value, where numbers have their natural
 * values (0-9) and `A-Z` (or `a-z`) are the values 10-35.
 *
 * Out of bound values return 255.
 *
 * This allows calculations for up to base 36.
 */
IFUNC uint8_t fio_c2i(unsigned char c);

/* *****************************************************************************
Numbers to Strings - API
***************************************************************************** */

/**
 * A helper function that writes a signed int64_t to a string.
 *
 * No overflow guard is provided, make sure there's at least 68 bytes
 * available (for base 2).
 *
 * Offers special support for base 2 (binary), base 8 (octal), base 10 and base
 * 16 (hex). An unsupported base will silently default to base 10. Prefixes
 * are automatically added (i.e., "0x" for hex and "0b" for base 2).
 *
 * Returns the number of bytes actually written (excluding the NUL
 * terminator).
 */
SFUNC size_t fio_ltoa(char *dest, int64_t num, uint8_t base);

/**
 * A helper function that converts between a double to a string.
 *
 * No overflow guard is provided, make sure there's at least 130 bytes
 * available (for base 2).
 *
 * Supports base 2, base 10 and base 16. An unsupported base will silently
 * default to base 10. Prefixes aren't added (i.e., no "0x" or "0b" at the
 * beginning of the string).
 *
 * Returns the number of bytes actually written (excluding the NUL
 * terminator).
 */
SFUNC size_t fio_ftoa(char *dest, double num, uint8_t base);
/* *****************************************************************************
Strings to Numbers - Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

typedef struct {
  uint64_t val;
  int64_t expo;
  uint8_t sign;
} fio___number_s;

/**
 * Maps characters to alphanumerical value, where numbers have their natural
 * values (0-9) and `A-Z` (or `a-z`) are the values 10-35.
 *
 * Out of bound values return 255.
 *
 * This allows calculations for up to base 36.
 */
IFUNC uint8_t fio_c2i(unsigned char c) {
  static uint8_t fio___alphanumerical_map[256] = {
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255,
      255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
      20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
      35,  255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,
      18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
      33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255};
  return fio___alphanumerical_map[c];
}

/** Reads number information in base 2. Returned expo in base 2. */
FIO_IFUNC fio___number_s fio___aton_read_b2_b2(char **pstr) {
  fio___number_s r = (fio___number_s){0};
  const uint64_t mask = ((1ULL) << ((sizeof(mask) << 3) - 1));
  while (**pstr >= '0' && **pstr <= '1' && !(r.val & mask)) {
    r.val = (r.val << 1) | (**pstr - '0');
    ++(*pstr);
  }
  while (**pstr >= '0' && **pstr <= '1') {
    ++r.expo;
    ++(*pstr);
  }
  return r;
}

FIO_IFUNC fio___number_s fio___aton_read_b2_bX(char **pstr, uint8_t base) {
  fio___number_s r = (fio___number_s){0};
  const uint64_t limit = ((~0ULL) / base) - (base - 1);
  register uint8_t tmp;
  while ((tmp = fio_c2i(**pstr)) < base && r.val <= (limit)) {
    r.val = (r.val * base) + tmp;
    ++(*pstr);
  }
  while (fio_c2i(**pstr) < base) {
    ++r.expo;
    ++(*pstr);
  }
  return r;
}

/** Reads number information for base 16 (hex). Returned expo in base 4. */
FIO_IFUNC fio___number_s fio___aton_read_b2_b16(char **pstr) {
  fio___number_s r = (fio___number_s){0};
  const uint64_t mask = ~((~(uint64_t)0ULL) >> 4);
  for (; !(r.val & mask);) {
    uint8_t tmp = fio_c2i(**pstr);
    if (tmp > 15)
      return r;
    r.val = (r.val << 4) | tmp;
    ++(*pstr);
  }
  while ((fio_c2i(**pstr)) < 16)
    ++r.expo;
  return r;
}

SFUNC int64_t fio_atol(char **pstr) {
  if (!pstr || !(*pstr))
    return 0;
  char *p = *pstr;
  unsigned char invert = 0;
  fio___number_s n = (fio___number_s){0};

  while ((int)(unsigned char)isspace(*p))
    ++p;
  if (*p == '-') {
    invert = 1;
    ++p;
  } else if (*p == '+') {
    ++p;
  }
  switch (*p) {
  case 'x': /* fallthrough */
  case 'X':
    goto is_hex;
  case 'b': /* fallthrough */
  case 'B':
    goto is_binary;
  case '0':
    ++p;
    switch (*p) {
    case 'x': /* fallthrough */
    case 'X':
      goto is_hex;
    case 'b': /* fallthrough */
    case 'B':
      goto is_binary;
    }
    goto is_base8;
  }

  /* is_base10: */
  *pstr = p;
  n = fio___aton_read_b2_bX(pstr, 10);

  /* sign can't be embeded */
#define CALC_N_VAL()                                                           \
  if (invert) {                                                                \
    if (n.expo || ((n.val << 1) && (n.val >> ((sizeof(n.val) << 3) - 1)))) {   \
      errno = E2BIG;                                                           \
      return (int64_t)(1ULL << ((sizeof(n.val) << 3) - 1));                    \
    }                                                                          \
    n.val = 0 - n.val;                                                         \
  } else {                                                                     \
    if (n.expo || (n.val >> ((sizeof(n.val) << 3) - 1))) {                     \
      errno = E2BIG;                                                           \
      return (int64_t)((~0ULL) >> 1);                                          \
    }                                                                          \
  }

  CALC_N_VAL();
  return n.val;

is_hex:
  ++p;
  while (*p == '0') {
    ++p;
  }
  *pstr = p;
  n = fio___aton_read_b2_b16(pstr);

  /* sign can be embeded */
#define CALC_N_VAL_EMBEDABLE()                                                 \
  if (invert) {                                                                \
    if (n.expo) {                                                              \
      errno = E2BIG;                                                           \
      return (int64_t)(1ULL << ((sizeof(n.val) << 3) - 1));                    \
    }                                                                          \
    n.val = 0 - n.val;                                                         \
  } else {                                                                     \
    if (n.expo) {                                                              \
      errno = E2BIG;                                                           \
      return (int64_t)((~0ULL) >> 1);                                          \
    }                                                                          \
  }

  CALC_N_VAL_EMBEDABLE();
  return n.val;

is_binary:
  ++p;
  while (*p == '0') {
    ++p;
  }
  *pstr = p;
  n = fio___aton_read_b2_b2(pstr);
  CALC_N_VAL_EMBEDABLE()
  return n.val;

is_base8:
  while (*p == '0') {
    ++p;
  }
  *pstr = p;
  n = fio___aton_read_b2_bX(pstr, 8);
  CALC_N_VAL();
  return n.val;
}

SFUNC double fio_atof(char **pstr) {
  if (!pstr || !(*pstr))
    return 0;
  if ((*pstr)[1] == 'b' || ((*pstr)[1] == '0' && (*pstr)[1] == 'b'))
    goto binary_raw;
  return strtod(*pstr, pstr);
binary_raw:
  /* binary representation is assumed to spell an exact double */
  (void)0;
  union {
    uint64_t i;
    double d;
  } punned = {.i = (uint64_t)fio_atol(pstr)};
  return punned.d;
}

/* *****************************************************************************
Numbers to Strings - Implementation
***************************************************************************** */

SFUNC size_t fio_ltoa(char *dest, int64_t num, uint8_t base) {
  // clang-format off
  const char notation[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  // clang-format on
  size_t len = 0;
  char buf[48]; /* we only need up to 20 for base 10, but base 3 needs 41... */

  if (!num)
    goto zero;

  switch (base) {
  case 1: /* fallthrough */
  case 2:
    /* Base 2 */
    {
      uint64_t n = num; /* avoid bit shifting inconsistencies with signed bit */
      uint8_t i = 0;    /* counting bits */
      dest[len++] = '0';
      dest[len++] = 'b';
#if __has_builtin(__builtin_clzll)
      i = __builtin_clzll(n);
      /* make sure the Binary representation doesn't appear signed */
      if (i) {
        --i;
        /*  keep it even */
        if ((i & 1))
          --i;
        n <<= i;
      }
#else
      while ((i < 64) && (n & 0x8000000000000000) == 0) {
        n <<= 1;
        i++;
      }
      /* make sure the Binary representation doesn't appear signed */
      if (i) {
        --i;
        n = n >> 1;
        /*  keep it even */
        if ((i & 1)) {
          --i;
          n = n >> 1;
        }
      }
#endif
      /* write to dest. */
      while (i < 64) {
        dest[len++] = ((n & 0x8000000000000000) ? '1' : '0');
        n = n << 1;
        i++;
      }
      dest[len] = 0;
      return len;
    }
  case 8:
    /* Base 8 */
    {
      uint64_t l = 0;
      if (num < 0) {
        dest[len++] = '-';
        num = 0 - num;
      }
      dest[len++] = '0';

      while (num) {
        buf[l++] = '0' + (num & 7);
        num = num >> 3;
      }
      while (l) {
        --l;
        dest[len++] = buf[l];
      }
      dest[len] = 0;
      return len;
    }

  case 16:
    /* Base 16 */
    {
      uint64_t n = num; /* avoid bit shifting inconsistencies with signed bit */
      uint8_t i = 0;    /* counting bits */
      dest[len++] = '0';
      dest[len++] = 'x';
      while ((n & 0xFF00000000000000) == 0) { // since n != 0, then i < 8
        n = n << 8;
        i++;
      }
      /* make sure the Hex representation doesn't appear misleadingly signed. */
      if (i && (n & 0x8000000000000000) && (n & 0x00FFFFFFFFFFFFFF)) {
        dest[len++] = '0';
        dest[len++] = '0';
      }
      /* write the damn thing, high to low */
      while (i < 8) {
        uint8_t tmp = (n & 0xF000000000000000) >> 60;
        uint8_t tmp2 = (n & 0x0F00000000000000) >> 56;
        dest[len++] = notation[tmp];
        dest[len++] = notation[tmp2];
        i++;
        n = n << 8;
      }
      dest[len] = 0;
      return len;
    }
  case 3: /* fallthrough */
  case 4: /* fallthrough */
  case 5: /* fallthrough */
  case 6: /* fallthrough */
  case 7: /* fallthrough */
  case 9: /* fallthrough */
    /* rare bases */
    {
      int64_t t = num / base;
      uint64_t l = 0;
      if (num < 0) {
        num = 0 - num; /* might fail due to overflow, but fixed with tail (t) */
        t = (int64_t)0 - t;
        dest[len++] = '-';
      }
      while (num) {
        buf[l++] = '0' + (num - (t * base));
        num = t;
        t = num / base;
      }
      while (l) {
        --l;
        dest[len++] = buf[l];
      }
      dest[len] = 0;
      return len;
    }

  default:
    break;
  }
  /* Base 10, the default base */
  {
    int64_t t = num / 10;
    uint64_t l = 0;
    if (num < 0) {
      num = 0 - num; /* might fail due to overflow, but fixed with tail (t) */
      t = (int64_t)0 - t;
      dest[len++] = '-';
    }
    while (num) {
      buf[l++] = '0' + (num - (t * 10));
      num = t;
      t = num / 10;
    }
    while (l) {
      --l;
      dest[len++] = buf[l];
    }
    dest[len] = 0;
    return len;
  }

zero:
  switch (base) {
  case 1:
  case 2:
    dest[len++] = '0';
    dest[len++] = 'b';
    break;
  case 8:
    dest[len++] = '0';
    break;
  case 16:
    dest[len++] = '0';
    dest[len++] = 'x';
    dest[len++] = '0';
    break;
  }
  dest[len++] = '0';
  dest[len] = 0;
  return len;
}

SFUNC size_t fio_ftoa(char *dest, double num, uint8_t base) {
  if (base == 2 || base == 16) {
    /* handle binary / Hex representation the same as an int64_t */
    /* FIXME: Hex representation should use floating-point hex instead */
    union {
      int64_t i;
      double d;
    } p;
    p.d = num;
    return fio_ltoa(dest, p.i, base);
  }
  size_t written = 0;
  uint8_t need_zero = 1;
  char *start = dest;

  if (isinf(num))
    goto is_inifinity;
  if (isnan(num))
    goto is_nan;

  written = sprintf(dest, "%g", num);
  while (*start) {
    if (*start == 'e')
      goto finish;
    if (*start == ',') // locale issues?
      *start = '.';
    if (*start == '.') {
      need_zero = 0;
    }
    start++;
  }
  if (need_zero) {
    dest[written++] = '.';
    dest[written++] = '0';
  }

finish:
  dest[written] = 0;
  return written;

is_inifinity:
  if (num < 0)
    dest[written++] = '-';
  memcpy(dest + written, "Infinity", 9);
  return written + 8;
is_nan:
  memcpy(dest, "NaN", 4);
  return 3;
}

/* *****************************************************************************
Numbers <=> Strings - Testing
***************************************************************************** */

#ifdef FIO_TEST_CSTL

#define FIO_ATOL_TEST_MAX 1048576

FIO_IFUNC int64_t FIO_NAME_TEST(stl, atol_time)(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return ((int64_t)t.tv_sec * 1000000) + (int64_t)t.tv_nsec / 1000;
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
              // {.str = "Binary ", .prefix = "0b", .prefix_len = 2, .base = 2},
              // {.str = "Oct    ", .prefix = "0", .prefix_len = 1, .base = 8},
              /* end marker */
              {.str = NULL},
          };
  fprintf(stderr, "    * %s test performance:\n", name);
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
          FIO___MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      } else {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test] == pb->prefix[pre_test])
            continue;
          FIO___MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      }
      __asm__ volatile("" ::: "memory"); /* don't optimize this loop */
      int64_t n = a2l(&bf);
      bf = buf;
      FIO_ASSERT(n == i,
                 "roundtrip error for %s: %s != %lld (got %lld)",
                 name,
                 buf,
                 i,
                 a2l(&bf));
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
          FIO___MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      } else {
        for (int pre_test = 0; pre_test < pb->prefix_len; ++pre_test) {
          if (bf[pre_test] == pb->prefix[pre_test])
            continue;
          FIO___MEMCPY(buf, pb->prefix, pb->prefix_len);
          bf = buf;
          break;
        }
      }
      __asm__ volatile("" ::: "memory"); /* don't optimize this loop */
    }
    tw = FIO_NAME_TEST(stl, atol_time)() - start;
    // clang-format off
    fprintf(stderr, "        - %s roundtrip   %zd us\n", pb->str, (size_t)trt);
    fprintf(stderr, "        - %s write       %zd us\n", pb->str, (size_t)tw);
    fprintf(stderr, "        - %s read (calc) %zd us\n", pb->str, (size_t)(trt - tw));
    // clang-format on
  }
}

SFUNC size_t sprintf_wrapper(char *dest, int64_t num, uint8_t base) {
  switch (base) {
  case 2: /* overflow - unsupported */
  case 8: /* overflow - unsupported */
  case 10:
    return sprintf(dest, "%" PRId64, num);
  case 16:
    if (num >= 0)
      return sprintf(dest, "0x%.16" PRIx64, num);
    return sprintf(dest, "-0x%.8" PRIx64, (0 - num));
  }
  return sprintf(dest, "%" PRId64, num);
}

SFUNC int64_t strtoll_wrapper(char **pstr) { return strtoll(*pstr, pstr, 0); }

FIO_SFUNC void FIO_NAME_TEST(stl, atol)(void) {
  fprintf(stderr, "* Testing fio_atol and fio_ltoa.\n");
  FIO_NAME_TEST(stl, atol_speed)("fio_atol/fio_ltoa", fio_atol, fio_ltoa);
  FIO_NAME_TEST(stl, atol_speed)
  ("system strtoll/sprintf", strtoll_wrapper, sprintf_wrapper);
  char buffer[1024];
  for (int i = 0 - FIO_ATOL_TEST_MAX; i < FIO_ATOL_TEST_MAX; ++i) {
    size_t tmp = fio_ltoa(buffer, i, 0);
    FIO_ASSERT(tmp > 0, "fio_ltoa returned length error");
    buffer[tmp++] = 0;
    char *tmp2 = buffer;
    int i2 = fio_atol(&tmp2);
    FIO_ASSERT(tmp2 > buffer, "fio_atol pointer motion error");
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
    FIO_ASSERT(tmp2 > buffer, "fio_atol pointer motion error");
    FIO_ASSERT((int64_t)i == i2,
               "fio_ltoa-fio_atol roundtrip error %lld != %lld",
               i,
               i2);
  }
  fprintf(stderr, "* Testing fio_atol samples.\n");
#define TEST_ATOL(s, n)                                                        \
  do {                                                                         \
    char *p = (char *)(s);                                                     \
    int64_t r = fio_atol(&p);                                                  \
    FIO_ASSERT(r == (n),                                                       \
               "fio_atol test error! %s => %zd (not %zd)",                     \
               ((char *)(s)),                                                  \
               (size_t)r,                                                      \
               (size_t)n);                                                     \
    FIO_ASSERT((s) + strlen((s)) == p,                                         \
               "fio_atol test error! %s reading position not at end (%zu)",    \
               (s),                                                            \
               (size_t)(p - (s)));                                             \
    char buf[72];                                                              \
    buf[fio_ltoa(buf, n, 2)] = 0;                                              \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 2 test error! "                                  \
               "%s != %s (%zd)",                                               \
               buf,                                                            \
               ((char *)(s)),                                                  \
               (size_t)((p = buf), fio_atol(&p)));                             \
    buf[fio_ltoa(buf, n, 8)] = 0;                                              \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 8 test error! "                                  \
               "%s != %s (%zd)",                                               \
               buf,                                                            \
               ((char *)(s)),                                                  \
               (size_t)((p = buf), fio_atol(&p)));                             \
    buf[fio_ltoa(buf, n, 10)] = 0;                                             \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 10 test error! "                                 \
               "%s != %s (%zd)",                                               \
               buf,                                                            \
               ((char *)(s)),                                                  \
               (size_t)((p = buf), fio_atol(&p)));                             \
    buf[fio_ltoa(buf, n, 16)] = 0;                                             \
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
  TEST_ATOL("9223372036854775806",
            9223372036854775806LL); /* almost INT64_MAX */
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
               "float parsing didn't stop at correct possition! %x != %x",     \
               *p,                                                             \
               stop);                                                          \
    if ((double)d == r || r == std) {                                          \
      /** fprintf(stderr, "Okay for %s\n", s); */                              \
    } else if ((pn2.as_i + 1) == (pn.as_i) || (pn.as_i + 1) == pn2.as_i) {     \
      fprintf(stderr,                                                          \
              "* WARNING: Single bit rounding error detected: %s\n",           \
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
                 std,                                                          \
                 r,                                                            \
                 f_buf);                                                       \
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
    memcpy(buffer, "1234567890.123", 14);
    buffer[14] = 0;
    size_t r = 0;
    start = clock();
    for (int i = 0; i < (FIO_ATOL_TEST_MAX << 3); ++i) {
      char *pos = buffer;
      r += fio_atol(&pos);
      __asm__ volatile("" ::: "memory");
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
      __asm__ volatile("" ::: "memory");
      // FIO_ASSERT(r == exp, "system strtol failed during speed test");
    }
    stop = clock();
    fprintf(stderr,
            "* system atol speed test completed in %zu cycles\n",
            stop - start);
  }
#endif /* !DEBUG */
#endif /* FIO_ATOF_ALT */
}
#undef FIO_ATOL_TEST_MAX
#endif /* FIO_TEST_CSTL */

/* *****************************************************************************
Numbers <=> Strings - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_ATOL */
#undef FIO_ATOL
