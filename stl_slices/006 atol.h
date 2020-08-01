/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                            String <=> Number helpers










***************************************************************************** */
#if defined(FIO_ATOL) && !defined(H___FIO_ATOL_H)
#define H___FIO_ATOL_H
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

/** Reads number information, up to base 10 numbers. Returned expo in `base`. */
FIO_IFUNC fio___number_s fio___aton_read_b2_b10(char **pstr, uint8_t base) {
  fio___number_s r = (fio___number_s){0};
  const uint64_t limit = ((~0ULL) / base) - (base - 1);
  while (**pstr >= '0' && **pstr < ('0' + base) && r.val <= (limit)) {
    r.val = (r.val * base) + (**pstr - '0');
    ++(*pstr);
  }
  while (**pstr >= '0' && **pstr < ('0' + base)) {
    ++r.expo;
    ++(*pstr);
  }
  return r;
}

/** Reads number information for base 16 (hex). Returned expo in base 4. */
FIO_IFUNC fio___number_s fio___aton_read_b2_b16(char **pstr) {
  fio___number_s r = (fio___number_s){0};
  const uint64_t mask = (~0ULL) << ((sizeof(mask) << 3) - 4);
  for (; !(r.val & mask);) {
    uint8_t tmp;
    if (**pstr >= '0' && **pstr <= '9')
      tmp = **pstr - '0';
    else if ((**pstr >= 'A' && **pstr <= 'F') ||
             (**pstr >= 'a' && **pstr <= 'f'))
      tmp = (**pstr | 32) - ('a' - 10);
    else
      return r;
    r.val = (r.val << 4) | tmp;
    ++(*pstr);
  }
  for (;;) {
    if ((**pstr >= '0' && **pstr <= '9') || (**pstr >= 'A' && **pstr <= 'F') ||
        (**pstr >= 'a' && **pstr <= 'f'))
      ++r.expo;
    else
      return r;
  }
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
  n = fio___aton_read_b2_b10(pstr, 10);

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
  n = fio___aton_read_b2_b10(pstr, 8);
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
        n = n << 1;
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

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_ATOL */
#undef FIO_ATOL
