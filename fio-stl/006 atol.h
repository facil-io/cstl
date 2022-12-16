/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___   /* Development inclusion - ignore line */
#define FIO_ATOL       /* Development inclusion - ignore line */
#include "./include.h" /* Development inclusion - ignore line */
#endif                 /* Development inclusion - ignore line */
/* *****************************************************************************




                            String <=> Number helpers



Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#if defined(FIO_ATOL) && !defined(H___FIO_ATOL_H)
#define H___FIO_ATOL_H
#include <inttypes.h>
#include <math.h>

/*TODO: cleanup + remove ltoa (doubles in String Core) */
/* *****************************************************************************
Strings to Signed Numbers - API
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
Signed Numbers to Strings - API
***************************************************************************** */

/**
 * A helper function that writes a signed int64_t to a string.
 *
 * No overflow guard is provided, make sure there's at least 68 bytes available
 * (for base 2).
 *
 * Offers special support for base 2 (binary), base 8 (octal), base 10 and base
 * 16 (hex) where prefixes are automatically added if required (i.e.,`"0x"` for
 * hex and `"0b"` for base 2, and `"0"` for octal).
 *
 * Supports any base up to base 36 (using 0-9,A-Z).
 *
 * An unsupported base will log an error and print zero.
 *
 * Returns the number of bytes actually written (excluding the NUL terminator).
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
Unsigned Numbers, Building Blocks and Helpers
***************************************************************************** */
/**
 * Maps characters to alphanumerical value, where numbers have their natural
 * values (0-9) and `A-Z` (or `a-z`) are the values 10-35.
 *
 * Out of bound values return 255.
 *
 * This allows parsing of numeral strings for up to base 36.
 */
IFUNC uint8_t fio_c2i(unsigned char c);

/**
 * Maps numeral values to alphanumerical characters, where numbers have their
 * natural values (0-9) and `A-Z` are the values 10-35.
 *
 * Accepts values up to 63. Returns zero for values over 35. Out of bound values
 * produce undefined behavior.
 *
 * This allows printing of numerals for up to base 36.
 */
IFUNC uint8_t fio_i2c(unsigned char i);

/** Returns the number of digits in base 10. */
FIO_IFUNC size_t fio_digits10(int64_t i);
/** Returns the number of digits in base 10 for an unsigned number. */
FIO_SFUNC size_t fio_digits10u(uint64_t i);

/** Returns the number of digits in base 8 for an unsigned number. */
FIO_SFUNC size_t fio_digits8u(uint64_t i);
/** Returns the number of digits in base 16 for an unsigned number. */
FIO_SFUNC size_t fio_digits16u(uint64_t i);
/** Returns the number of digits in base 2 for an unsigned number. */
FIO_SFUNC size_t fio_digits_bin(uint64_t i);
/** Returns the number of digits in any base X<65 for an unsigned number. */
FIO_SFUNC size_t fio_digits_xbase(uint64_t i, size_t base);

/** Writes a signed number to `dest` using `digits` bytes (+ `NUL`) */
FIO_IFUNC void fio_ltoa10(char *dest, int64_t i, size_t digits);
/** Reads a signed base 10 formatted number. */
SFUNC int64_t fio_atol10(char **pstr);

/** Writes unsigned number to `dest` using `digits` bytes (+ `NUL`) */
FIO_IFUNC void fio_ltoa10u(char *dest, uint64_t i, size_t digits);
/** Writes unsigned number to `dest` using `digits` bytes (+ `NUL`) */
FIO_IFUNC void fio_ltoa16u(char *dest, uint64_t i, size_t digits);
/** Writes unsigned number to `dest` using `digits` bytes (+ `NUL`) */
FIO_IFUNC void fio_ltoa_bin(char *dest, uint64_t i, size_t digits);
/** Writes unsigned number to `dest` using `digits` bytes (+ `NUL`) */
FIO_IFUNC void fio_ltoa_xbase(char *dest,
                              uint64_t i,
                              size_t digits,
                              size_t base);

/** Reads a signed base 8 formatted number. */
SFUNC uint64_t fio_atol8u(char **pstr);
/** Reads a signed base 10 formatted number. */
SFUNC uint64_t fio_atol10u(char **pstr);
/** Reads an unsigned hex formatted number (possibly prefixed with "0x"). */
SFUNC uint64_t fio_atol16u(char **pstr);
/** Reads an unsigned binary formatted number (possibly prefixed with "0b"). */
SFUNC uint64_t fio_atol_bin(char **pstr);
/** Read an unsigned number in any base up to base 36. */
SFUNC uint64_t fio_atol_xbase(char **pstr, size_t base);

/** Converts an unsigned `val` to a signed `val`, with overflow protection. */
FIO_IFUNC int64_t fio_u2i_limit(uint64_t val, size_t invert);

/* *****************************************************************************


Implementation - inlined


***************************************************************************** */

/** Returns the number of digits in base 10. */
FIO_IFUNC size_t fio_digits10(int64_t i) {
  if (i >= 0)
    return fio_digits10u(i);
  return fio_digits10u((0 - i)) + 1;
}

/** Returns the number of digits in base 2 for an unsigned number. */
FIO_SFUNC size_t fio_digits_bin(uint64_t i) {
  size_t r = 1;
  if (!i)
    return r;
  r = fio___msb_index_unsafe(i) + 1;
  r += (r & 1); /* binary is written 2 zeros at a time */
  return r;
}

/** Returns the number of digits in base 8 for an unsigned number. */
FIO_SFUNC size_t fio_digits8u(uint64_t i) {
  size_t r = 1;
  for (;;) {
    if (i < 8)
      return r;
    if (i < 64)
      return r + 1;
    if (i < 512)
      return r + 2;
    if (i < 4096)
      return r + 3;
    if (i < 32768)
      return r + 4;
    if (i < 262144)
      return r + 5;
    if (i < 2097152)
      return r + 6;
    if (i < 16777216)
      return r + 7;
    r += 8;
    i >>= 24;
  }
}

/** Returns the number of digits in base 10 for an unsigned number. */
FIO_SFUNC size_t fio_digits10u(uint64_t i) {
  size_t r = 1;
  for (;;) {
    if (i < 10ULL)
      return r;
    if (i < 100ULL)
      return r + 1;
    if (i < 1000ULL)
      return r + 2;
    if (i < 10000ULL)
      return r + 3;
    r += 4;
    i /= 10000ULL;
  }
}

/** Returns the number of digits in base 16 for an unsigned number. */
FIO_SFUNC size_t fio_digits16u(uint64_t i) {
  if (i < 0x100ULL)
    return 2;
  if (i < 0x10000ULL)
    return 4;
  if (i < 0x1000000ULL)
    return 6;
  if (i < 0x100000000ULL)
    return 8;
  if (i < 0x10000000000ULL)
    return 10;
  if (i < 0x1000000000000ULL)
    return 12;
  if (i < 0x100000000000000ULL)
    return 14;
  return 16;
}

/** Returns the number of digits in base X<65 for an unsigned number. */
FIO_SFUNC size_t fio_digits_xbase(uint64_t i, size_t base) {
  size_t base2 = base * base;
  size_t base3 = base2 * base;
  size_t base4 = base3 * base;
  size_t base5 = base4 * base;
  size_t r = 1;
  for (;;) {
    if (i < base)
      return r;
    if (i < base2)
      return r + 1;
    if (i < base3)
      return r + 2;
    if (i < base4)
      return r + 3;
    r += 4;
    i /= base5;
  }
}

FIO_IFUNC void fio_ltoa10(char *dest, int64_t i, size_t digits) {
  size_t inv = i < 0;
  dest[0] = '-';
  dest += inv;
  if (inv)
    i = (int64_t)((uint64_t)0 - (uint64_t)i);
  fio_ltoa10u(dest, (uint64_t)i, digits - inv);
}

FIO_IFUNC void fio_ltoa8u(char *dest, uint64_t i, size_t digits) {
  dest += digits;
  *dest-- = 0;
  while (i > 7) {
    *dest-- = '0' + (i & 7);
    i >>= 3;
  }
  *dest = '0' + i;
}

FIO_IFUNC void fio_ltoa10u(char *dest, uint64_t i, size_t digits) {
  dest += digits;
  *dest-- = 0;
  while (i > 9) {
    uint64_t nxt = i / 10;
    *dest-- = '0' + (i - (nxt * 10ULL));
    i = nxt;
  }
  *dest = '0' + (unsigned char)i;
}

FIO_IFUNC void fio_ltoa16u(char *dest, uint64_t i, size_t digits) {
  dest += digits;
  *dest-- = 0;
  while (i > 255) {
    *dest-- = fio_i2c(i & 15);
    i >>= 4;
    *dest-- = fio_i2c(i & 15);
    i >>= 4;
  }
  *dest-- = fio_i2c(i & 15);
  i >>= 4;
  *dest = fio_i2c(i);
}

FIO_IFUNC void fio_ltoa_bin(char *dest, uint64_t i, size_t digits) {
  dest += digits;
  *dest-- = 0;
  switch (digits & 7) { /* last use of `digits` */
    while (i) {
      *dest-- = '0' + (i & 1);
      i >>= 1;                                /* fall through */
    case 7: *dest-- = '0' + (i & 1); i >>= 1; /* fall through */
    case 6: *dest-- = '0' + (i & 1); i >>= 1; /* fall through */
    case 5: *dest-- = '0' + (i & 1); i >>= 1; /* fall through */
    case 4: *dest-- = '0' + (i & 1); i >>= 1; /* fall through */
    case 3: *dest-- = '0' + (i & 1); i >>= 1; /* fall through */
    case 2: *dest-- = '0' + (i & 1); i >>= 1; /* fall through */
    case 1: *dest-- = '0' + (i & 1); i >>= 1; /* fall through */
    case 0:;
    }
  }
}

FIO_IFUNC void fio_ltoa_xbase(char *dest,
                              uint64_t i,
                              size_t digits,
                              size_t base) {
  dest += digits;
  *dest-- = 0;
  while (i >= base) {
    uint64_t nxt = i / base;
    *dest-- = fio_i2c(i - (nxt * 10ULL));
    i = nxt;
  }
  *dest = fio_i2c(i);
}

/** Converts an unsigned `val` to a signed `val`, with overflow protection. */
FIO_IFUNC int64_t fio_u2i_limit(uint64_t val, size_t to_negative) {
  if (!to_negative) {
    /* overflow? */
    if (!(val & 0x8000000000000000ULL))
      return val;
    errno = E2BIG;
    val = 0x7FFFFFFFFFFFFFFFULL;
    return val;
  }
  if (!(val & 0x8000000000000000ULL)) {
    val = (int64_t)0LL - (int64_t)val;
    return val;
  }
  /* read overflow */
  errno = E2BIG;
  return (int64_t)(val = 0x8000000000000000ULL);
}

/* *****************************************************************************
Implementation - possibly externed
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

typedef struct {
  uint64_t val;
  int64_t expo;
  uint8_t sign;
} fio___number_s;

/* *****************************************************************************
Unsigned core and helpers
***************************************************************************** */

/**
 * Maps characters to alphanumerical value, where numbers have their natural
 * values (0-9) and `A-Z` (or `a-z`) are the values 10-35.
 *
 * Out of bound values return 255.
 *
 * This allows parsing of numeral strings for up to base 36.
 */
IFUNC uint8_t fio_c2i(unsigned char c) {
  static const uint8_t fio___alphanumeric_map[256] = {
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
  return fio___alphanumeric_map[c];
}

/**
 * Maps numeral values to alphanumerical characters, where numbers have their
 * natural values (0-9) and `A-Z` are the values 10-35.
 *
 * Accepts values up to 63. Returns zero for values over 35. Out of bound values
 * produce undefined behavior.
 *
 * This allows printing of numerals for up to base 36.
 */
IFUNC uint8_t fio_i2c(unsigned char i) {
  static const uint8_t fio___alphanumeric_map[64] = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
      'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
      'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
  return fio___alphanumeric_map[i & 63];
}

/** Reads a signed base 10 formatted number. */
SFUNC int64_t fio_atol10(char **pstr) {
  const uint64_t add_limit = (~(uint64_t)0ULL) - 9;
  char *pos = *pstr;
  const size_t inv = (pos[0] == '-');
  pos += inv;
  uint64_t val = 0;
  uint64_t r0;
  while (((r0 = pos[0] - '0') < 10ULL) & (val < add_limit)) {
    val *= 10;
    val += r0;
    ++pos;
  }
  if (((size_t)(pos[0] - '0') < 10ULL)) {
    errno = E2BIG;
  }
  *pstr = pos;
  return fio_u2i_limit(val, inv);
}

/** Reads a signed base 8 formatted number. */
SFUNC uint64_t fio_atol8u(char **pstr) {
  uint64_t r = 0;
  size_t d;
  while ((d = (size_t)fio_c2i((unsigned char)(**pstr))) < 8) {
    r <<= 3;
    r |= d;
    ++*pstr;
    if ((r & UINT64_C(0xE000000000000000)))
      break;
  }
  if ((fio_c2i(**pstr)) < 8)
    errno = E2BIG;
  return r;
}

/** Reads an unsigned base 10 formatted number. */
SFUNC uint64_t fio_atol10u(char **pstr) {
  uint64_t r = 0;
  const uint64_t add_limit = (~(uint64_t)0ULL) - 9;
  char *pos = *pstr;
  uint64_t r0;
  while (((r0 = (uint64_t)(pos[0] - '0')) < 10ULL) & (r < add_limit)) {
    r *= 10;
    r += r0;
    ++pos;
  }
  while (((size_t)(pos[0] - '0') < 10ULL)) {
    errno = E2BIG;
    ++pos;
  }
  *pstr = pos;
  return r;
}

/** Reads an unsigned hex formatted number (possibly prefixed with "0x"). */
SFUNC uint64_t fio_atol16u(char **pstr) {
  uint64_t r = 0;
  size_t d;
  *pstr += (**pstr == '0');
  *pstr += (**pstr | 32) == 'x' && fio_c2i((unsigned char)(pstr[0][1])) < 16;
  while ((d = (size_t)fio_c2i((unsigned char)(**pstr))) < 16) {
    r <<= 4;
    r |= d;
    ++*pstr;
    if ((r & UINT64_C(0xF000000000000000)))
      break;
  }
  if ((fio_c2i(**pstr)) < 16)
    errno = E2BIG;
  return r;
}

/** Reads an unsigned binary formatted number (possibly prefixed with "0b"). */
SFUNC uint64_t fio_atol_bin(char **pstr) {
  uint64_t r = 0;
  size_t d;
  *pstr += (**pstr == '0');
  *pstr += (**pstr | 32) == 'b' && ((size_t)(pstr[0][1]) - (size_t)'0') < 2;
  while ((d = (size_t)((unsigned char)(**pstr)) - (size_t)'0') < 2) {
    r <<= 1;
    r |= d;
    ++*pstr;
    if ((r & UINT64_C(0x8000000000000000)))
      break;
  }
  if ((d = (size_t)(**pstr) - (size_t)'0') < 2)
    errno = E2BIG;
  return r;
}

/** Attempts to read an unsigned number in any base up to base 36. */
SFUNC uint64_t fio_atol_xbase(char **pstr, size_t base) {
  uint64_t r = 0;
  if (base > 36)
    return r;
  if (base == 10)
    return (r = fio_atol10u(pstr));
  if (base == 16)
    return (r = fio_atol16u(pstr));
  if (base == 2)
    return (r = fio_atol_bin(pstr));
  const uint64_t limit = (~UINT64_C(0)) / base;
  size_t d;
  while ((d = (size_t)fio_c2i((unsigned char)(**pstr))) < base) {
    r *= base;
    r += d;
    ++*pstr;
    if (r > limit)
      break;
  }
  if ((fio_c2i(**pstr)) < base)
    errno = E2BIG;
  return r;
}

/* *****************************************************************************
fio_atol
***************************************************************************** */

SFUNC int64_t fio_atol(char **pstr) {
  if (!pstr || !(*pstr))
    return 0;
  uint64_t v = 0;
  uint64_t (*fn)(char **) = fio_atol10u;
  char *p = *pstr;
  unsigned inv = (p[0] == '-');
  p += inv;
  char *const s = p;
  switch (*p) {
  case 'x': /* fall through */
  case 'X': fn = fio_atol16u; goto compute;
  case 'b': /* fall through */
  case 'B': fn = fio_atol_bin; goto compute;
  case '0':
    switch (p[1]) {
    case 'x': /* fall through */
    case 'X': fn = fio_atol16u; goto compute;
    case 'b': /* fall through */
    case 'B': fn = fio_atol_bin; goto compute;
    case '0': /* fall through */
    case '1': /* fall through */
    case '2': /* fall through */
    case '3': /* fall through */
    case '4': /* fall through */
    case '5': /* fall through */
    case '6': /* fall through */
    case '7': fn = fio_atol8u;
    }
  }
compute:
  v = fn(&p);
  if (p != s)
    *pstr = p;
  if (fn == fio_atol10u)
    return fio_u2i_limit(v, inv);
  if (!inv) /* sign embedded in the representation */
    return (int64_t)v;
  return fio_u2i_limit(v, inv);
}

/* *****************************************************************************
fio_ltoa
***************************************************************************** */

SFUNC size_t fio_ltoa(char *dest, int64_t num, uint8_t base) {
  size_t len = 0;
  uint64_t n = (uint64_t)num;
  size_t digits;
  char dump[96];
  if (!dest)
    dest = dump;

  switch (base) {
  case 1: /* fall through */
  case 2: /* Base 2 */
    len += (digits = fio_digits_bin(n));
    fio_ltoa_bin(dest, n, digits); /* embedded sign bit */
    return len;
  case 8: /* Base 8 */
    if (num < 0) {
      *(dest++) = '-';
      n = 0 - n;
      ++len;
    }
    len += (digits = fio_digits8u(n));
    fio_ltoa8u(dest, n, digits);
    return len;
  case 16: /* Base 16 */
    len += (digits = fio_digits16u(n));
    fio_ltoa16u(dest, n, digits); /* embedded sign bit */
    return len;
  case 0:  /* fall through */
  case 10: /* Base 10 */
    if (num < 0) {
      *(dest++) = '-';
      n = 0 - n;
      ++len;
    }
    len += (digits = fio_digits10u(n));
    fio_ltoa10u(dest, n, digits);
    return len;
  default: /* any base up to base 36 */
    if (base > 36)
      goto base_error;
    if (num < 0) {
      *(dest++) = '-';
      n = 0 - n;
      ++len;
    }
    len += (digits = fio_digits_xbase(n, base));
    fio_ltoa_xbase(dest, n, digits, base);
    return len;
  }

base_error:
  FIO_LOG_ERROR("fio_ltoa base out of range");
  return len;
}

/* *****************************************************************************
fio_atof
***************************************************************************** */

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
fio_ftoa
***************************************************************************** */

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

  if (isinf(num))
    goto is_inifinity;
  if (isnan(num))
    goto is_nan;

  written = snprintf(dest, 30, "%g", num);
  /* test if we need to add a ".0" to the end of the string */
  for (char *start = dest;;) {
    switch (*start) {
    case ',':
      *start = '.'; // locale issues?
    /* fall through */
    case 'e': /* fall through */
    case '.': /* fall through */ goto finish;
    case 0: goto add_dot_zero;
    }
    ++start;
  }
add_dot_zero:
  dest[written++] = '.';
  dest[written++] = '0';

finish:
  dest[written] = 0;
  return written;

is_inifinity:
  if (num < 0)
    dest[written++] = '-';
  fio_memcpy8(dest + written, "Infinity");
  written += 8;
  dest[written] = 0;
  return written;
is_nan:
  fio_memcpy4(dest, "NaN");
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

SFUNC size_t sprintf_wrapper(char *dest, int64_t num, uint8_t base) {
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

SFUNC int64_t strtoll_wrapper(char **pstr) { return strtoll(*pstr, pstr, 0); }

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
  for (unsigned char i = 0; i < 36; ++i) {
    FIO_ASSERT(i == fio_c2i(fio_i2c(i)), "fio_c2i / fio_i2c roundtrip error.");
  }
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
    FIO_ASSERT((s) + strlen((s)) == p,                                         \
               "fio_atol test error! %s reading position not at end "          \
               "(!%zu == %zu)\n\t0x%p - 0x%p",                                 \
               (s),                                                            \
               (size_t)strlen((s)),                                            \
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
  ("system strtoll/sprintf", strtoll_wrapper, sprintf_wrapper);

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
    fio_memcpy15x(buffer, "1234567890.123", 14);
    buffer[14] = 0;
    size_t r = 0;
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
