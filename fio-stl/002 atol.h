/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_ATOL               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                            String <=> Number helpers



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_ATOL) && !defined(H___FIO_ATOL___H)
#define H___FIO_ATOL___H
#include <inttypes.h>
#include <math.h>

#ifndef FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER
#define FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER 1
#endif

/* *****************************************************************************
Strings to Signed Numbers - The fio_aton function
***************************************************************************** */

/** Result type for fio_aton */
typedef struct {
  union {
    int64_t i;
    double f;
    uint64_t u;
  };
  int is_float;
  int err;
} fio_aton_s;

/**
 * Converts a String to a number - either an integer or a float (double).
 *
 * Skips white space at the beginning of the string.
 *
 * Auto detects binary and hex formats when prefix is provided (0x / 0b).
 *
 * Auto detects octal when number starts with zero.
 *
 * Auto detects the Strings "inf", "infinity" and "nan" as float values.
 *
 * The number's format and type are returned in the return type.
 *
 * If a numerical overflow or format error occurred, the `.err` flag is set.
 *
 * Note: rounding errors may occur, as this is not an `strtod` exact match.
 */
FIO_SFUNC fio_aton_s fio_aton(char **pstr);

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
IEEE 754 Floating Points, Building Blocks and Helpers
***************************************************************************** */

/** Converts a 64 bit integer to an IEEE 754 formatted double. */
FIO_IFUNC double fio_i2d(int64_t mant, int64_t exponent_in_base_2);

/** Converts a 64 bit unsigned integer to an IEEE 754 formatted double. */
FIO_IFUNC double fio_u2d(uint64_t mant, int64_t exponent_in_base_2);

/* *****************************************************************************
Big Numbers
***************************************************************************** */

/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u128 fio_u128_hex_read(char **pstr);
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u256 fio_u256_hex_read(char **pstr);
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u512 fio_u512_hex_read(char **pstr);
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u1024 fio_u1024_hex_read(char **pstr);
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u2048 fio_u2048_hex_read(char **pstr);
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u4096 fio_u4096_hex_read(char **pstr);

/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u128_hex_write(char *dest, const fio_u128 *);
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u256_hex_write(char *dest, const fio_u256 *);
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u512_hex_write(char *dest, const fio_u512 *);
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u1024_hex_write(char *dest, const fio_u1024 *);
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u2048_hex_write(char *dest, const fio_u2048 *);
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u4096_hex_write(char *dest, const fio_u4096 *);

/* *****************************************************************************


Implementation - inlined


***************************************************************************** */

/** Returns the number of digits in base 10. */
FIO_IFUNC size_t fio_digits10(int64_t i) {
  if (i >= 0)
    return fio_digits10u(i);
  return fio_digits10u((0ULL - (uint64_t)i)) + 1;
}

/** Returns the number of digits in base 2 for an unsigned number. */
FIO_SFUNC size_t fio_digits_bin(uint64_t i) {
  size_t r = 1;
  if (!i)
    return r;
  r = fio_msb_index_unsafe(i) + 1;
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
    i /= base4;
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
  digits += (digits & 1U); /* force even number of digits */
  dest += digits;
  *dest-- = 0;
  while (digits) {
    digits -= 2;
    *dest-- = fio_i2c(i & 15);
    i >>= 4;
    *dest-- = fio_i2c(i & 15);
    i >>= 4;
  }
}

FIO_IFUNC void fio_ltoa_bin(char *dest, uint64_t i, size_t digits) {
  dest += digits;
  *dest = 0;
  switch (digits & 7) {
  case 7: *--dest = '0' + (i & 1); i >>= 1; /* fall through */
  case 6: *--dest = '0' + (i & 1); i >>= 1; /* fall through */
  case 5: *--dest = '0' + (i & 1); i >>= 1; /* fall through */
  case 4: *--dest = '0' + (i & 1); i >>= 1; /* fall through */
  case 3: *--dest = '0' + (i & 1); i >>= 1; /* fall through */
  case 2: *--dest = '0' + (i & 1); i >>= 1; /* fall through */
  case 1: *--dest = '0' + (i & 1); i >>= 1; /* fall through */
  case 0:;
  }
  digits &= ~(uint64_t)7ULL;
  while (digits > 7) {
    uint64_t tmp = (i & 0xFFULL);
    dest -= 8;
    digits -= 8;
    i >>= 8;
    tmp = ((tmp & 0x7F) * 0x02040810204081ULL) | ((tmp & 0x80) << 49);
    tmp &= 0x0101010101010101ULL;
    tmp += (0x0101010101010101ULL * '0');
    fio_u2buf64_be(dest, tmp);
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
    *dest-- = fio_i2c(i - (nxt * base));
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
IEEE 754 Floating Points, Building Blocks and Helpers
***************************************************************************** */

#ifndef FIO_MATH_DBL_MANT_MASK
#define FIO_MATH_DBL_MANT_MASK (((uint64_t)1ULL << 52) - 1)
#define FIO_MATH_DBL_EXPO_MASK ((uint64_t)2047ULL << 52)
#define FIO_MATH_DBL_SIGN_MASK ((uint64_t)1ULL << 63)
#endif

FIO_IFUNC int fio_d2expo(double d) {
  int r;
  union {
    uint64_t u64;
    double d;
  } u = {.d = d};
  u.u64 &= FIO_MATH_DBL_EXPO_MASK;
  r = (int)(u.u64 >> 52);
  r -= 1023;
  r *= -1;
  return r;
}

/** Converts a 64 bit integer to an IEEE 754 formatted double. */
FIO_IFUNC double fio_u2d(uint64_t mant, int64_t exponent) {
#ifndef FIO___ATON_TIE2EVEN
  /* If set, performs a rounding attempt with tie to even */
#define FIO___ATON_TIE2EVEN 0
#endif
  union {
    uint64_t u64;
    double d;
  } u = {0};
  size_t msbi;
  if (!mant)
    return u.d;
  msbi = fio_msb_index_unsafe(mant);
  if (FIO___ATON_TIE2EVEN && FIO_UNLIKELY(msbi > 52)) { /* losing precision */
    bool not53 = (msbi != 53);
    bool far_set = ((mant >> (53 + not53)) != 0);
    mant = mant >> (msbi - (53 + not53));
    mant |= far_set;
    mant |= (mant >> (1U + not53)) & 1; /* set the non-even bit as rounder */
    mant += 1; /* 1 will propagate if rounding is necessary. */
    bool add_to_expo = (mant >> (53U + not53)) & 1;
    mant >>= (1U + not53 + add_to_expo);
    exponent += add_to_expo;
  }
  /* normalize exponent */
  exponent += msbi + 1023;
  if (FIO_UNLIKELY(exponent > 2047))
    goto is_inifinity_or_nan;
  if (FIO_UNLIKELY(exponent <= 0))
    goto is_subnormal;
  exponent = (uint64_t)exponent << 52;
  u.u64 |= exponent;
  /* reposition mant bits so we "hide" the fist set bit in bit[52] */
  if (msbi < 52)
    mant = mant << (52 - msbi);
  else if (!FIO___ATON_TIE2EVEN &&
           FIO_UNLIKELY(msbi > 52)) /* losing precision */
    mant = mant >> (msbi - 52);
  u.u64 |= mant & FIO_MATH_DBL_MANT_MASK; /* remove the 1 set bit */
  return u.d;

is_inifinity_or_nan:
  u.u64 = FIO_MATH_DBL_EXPO_MASK;
  return u.d;

is_subnormal:
  exponent += 51 - msbi;
  if (exponent < 0)
    return u.d;
  u.u64 = mant >> exponent;
  return u.d;
}

/** Converts a 64 bit integer to an IEEE 754 formatted double. */
FIO_IFUNC double fio_i2d(int64_t mant, int64_t exponent) {
  union {
    uint64_t u64;
    int64_t i64;
    double d;
  } u = {.i64 = mant};
  bool sign = (u.i64 < 0);
  if (sign)
    u.i64 = -u.i64;
  u.d = fio_u2d(u.u64, exponent);
  u.u64 |= ((uint64_t)sign) << 63;
  return u.d;
}

/* *****************************************************************************
Implementation - possibly externed
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

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
#if FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER
    *pstr += (**pstr == '_'); /* allow '_' as a divider. */
#endif
  }
  if ((fio_c2i(**pstr)) < 8)
    errno = E2BIG;
  return r;
}

/** Reads an unsigned base 10 formatted number. */
SFUNC uint64_t fio_atol10u(char **pstr) {
  uint64_t r = 0, u0 = 0, u1 = 0;
  char *pos = *pstr;
  /* can't use SIMD, as we don't want to overflow. */
  for (size_t i = 0; i < 8; ++i)
    u0 += ((pos[u0] >= '0') & (pos[u0] <= '9'));
  switch ((u0 & 12)) { /* now we are safe to copy all bytes validated */
  case 8:
    r = fio_buf2u64_le(pos);
    *pstr = (pos += 8); /* credit Johnny Lee, not mine... */
    r = ((r & 0x0F0F0F0F0F0F0F0FULL) * 2561ULL) >> 8;
    r = ((r & 0x00FF00FF00FF00FFULL) * 6553601ULL) >> 16;
    r = ((r & 0x0000FFFF0000FFFFULL) * 42949672960001ULL) >> 32;
    u1 = r; /* https://johnnylee-sde.github.io/Fast-numeric-string-to-int/ */
    break;
  case 4:
    r = ((unsigned)(pos[0] - '0') * 1000) + ((unsigned)(pos[1] - '0') * 100) +
        ((unsigned)(pos[2] - '0') * 10) + (unsigned)(pos[3] - '0');
    *pstr = (pos += 4);
    u1 = r;
    break;
  }

  u0 = (uint64_t)(pos[0] - '0');
  if (u0 > 9ULL)
    return r;
  r *= 10;
  for (;;) {
    r += u0;
    if (r < u1)
      goto value_overflow;
    ++pos;
#if FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER
    pos += (*pos == '_'); /* allow '_' as a divider. */
#endif
    u0 = (uint64_t)(pos[0] - '0');
    if (u0 > 9ULL)
      break;
    if (r > ((~(uint64_t)0ULL) / 10))
      goto value_overflow_stepback;
    u1 = r;
    r *= 10;
  }
  *pstr = pos;
  return r;

value_overflow_stepback:
  --pos;
value_overflow:
  r = u1;
  errno = E2BIG;
  *pstr = pos;
  return r;
}

/** Reads a signed base 10 formatted number. */
SFUNC int64_t fio_atol10(char **pstr) {
  // const uint64_t add_limit = (~(uint64_t)0ULL) - 9;
  char *pos = *pstr;
  const size_t inv = (pos[0] == '-');
  pos += inv;
  // uint64_t val = 0;
  // uint64_t r0;
  // while (((r0 = pos[0] - '0') < 10ULL) & (val < add_limit)) {
  //   val *= 10;
  //   val += r0;
  //   ++pos;
  // }
  // if (((size_t)(pos[0] - '0') < 10ULL)) {
  //   errno = E2BIG;
  // }
  *pstr = pos;
  uint64_t val = fio_atol10u(pstr);
  if (((size_t)(**pstr - '0') < 10ULL))
    errno = E2BIG;
  return fio_u2i_limit(val, inv);
}

/** Reads an unsigned hex formatted number (possibly prefixed with "0x"). */
FIO_IFUNC uint64_t fio___atol16u_with_prefix(uint64_t r, char **pstr) {
  size_t d;
  unsigned char *p = (unsigned char *)*pstr;
  p += ((p[0] == '0') & ((p[1] | 32) == 'x')) << 1;
  if ((d = fio_c2i(*p)) > 15)
    goto possible_misread;
  for (;;) {
    r |= d;
    ++p;
    d = (size_t)fio_c2i(*p);
    if (d > 15)
      break;
    if ((r & UINT64_C(0xF000000000000000))) {
      errno = E2BIG;
      break;
    }
    r <<= 4;
#if FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER
    p += (*p == '_'); /* allow '_' as a divider. */
#endif
  }
  *pstr = (char *)p;
  return r;
possible_misread:
  /* if 0x was read, move to X. */
  *pstr += ((pstr[0][0] == '0') & ((pstr[0][1] | 32) == 'x'));
  return r;
}

/** Reads an unsigned hex formatted number (possibly prefixed with "0x"). */
SFUNC uint64_t fio_atol16u(char **pstr) {
  uint64_t r = 0;
  size_t d;
  unsigned char *p = (unsigned char *)*pstr;
  p += ((p[0] == '0') & ((p[1] | 32) == 'x')) << 1;
  if ((d = fio_c2i(*p)) > 15)
    goto possible_misread;
  for (;;) {
    r |= d;
    ++p;
    d = (size_t)fio_c2i(*p);
    if (d > 15)
      break;
    if ((r & UINT64_C(0xF000000000000000))) {
      errno = E2BIG;
      break;
    }
    r <<= 4;
#if FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER
    p += (*p == '_'); /* allow '_' as a divider. */
#endif
  }
  *pstr = (char *)p;
  return r;
possible_misread:
  /* if 0x was read, move to X. */
  *pstr += ((pstr[0][0] == '0') & ((pstr[0][1] | 32) == 'x'));
  return r;
}

/** Reads an unsigned binary formatted number (possibly prefixed with "0b"). */
SFUNC FIO___ASAN_AVOID uint64_t fio_atol_bin(char **pstr) {
  uint64_t r = 0;
  *pstr += (**pstr == '0');
  *pstr += (**pstr | 32) == 'b' && (((size_t)(pstr[0][1]) - (size_t)'0') < 2);
#if FIO___ASAN_DETECTED || 1
  for (;;) { /* Prevent safe overflow of allocated memory region */
    if ((r & UINT64_C(0x8000000000000000)))
      break;
    size_t len = 0;
    union {
      uint64_t u64;
      uint32_t u32;
    } u;
    for (size_t i = 0; i < 8; ++i)
      len += (((size_t)pstr[0][len] - (size_t)'0') < 2);
    if (!len)
      goto done;
    switch (len & 12) {
    case 8:
      if ((r & UINT64_C(0xFF00000000000000)))
        break; /* from switch */
      u.u64 = fio_buf2u64_be(*pstr);
      u.u64 -= 0x0101010101010101ULL * '0';
      u.u64 |= u.u64 >> 7;
      u.u64 |= u.u64 >> 14;
      u.u64 |= u.u64 >> 28;
      u.u64 &= 0xFF;
      r <<= 8;
      r |= u.u64;
      *pstr += 8;
      continue;
    case 4:
      if ((r & UINT64_C(0xF000000000000000)))
        break; /* from switch */
      u.u32 = fio_buf2u32_be(*pstr);
      u.u32 -= (0x01010101UL * '0');
      u.u32 |= u.u32 >> 7;
      u.u32 |= u.u32 >> 14;
      u.u32 &= 0x0F;
      r <<= 4;
      r |= u.u32;
      *pstr += 4;
      continue;
    }
    while ((len = (size_t)((unsigned char)(**pstr)) - (size_t)'0') < 2) {
      r <<= 1;
      r |= len;
      ++*pstr;
      if ((r & UINT64_C(0x8000000000000000)))
        break;
    }
#if FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER
    if ((**pstr == '_') | (**pstr == '.')) { /* allow as a dividers */
      ++*pstr;
      continue;
    }
#endif
    break;
  }
done:
#else
  size_t d;
  for (; (((uintptr_t)*pstr & 4095) < 4089);) { /* respect page boundary */
    uint64_t tmp = fio_buf2u64_be(*pstr);       /* may overflow */
    tmp -= 0x0101010101010101ULL * '0';         /* was it all `0`s and `1`s? */
    if (tmp & (~0x0101010101010101ULL))         /* if note, break. */
      break;
    tmp |= tmp >> 7;
    tmp |= tmp >> 14;
    tmp |= tmp >> 28;
    tmp &= 0xFF;
    r <<= 8;
    r |= tmp;
    *pstr += 8;
    if ((r & UINT64_C(0xFF00000000000000)))
      break;
  }
  while ((d = (size_t)((unsigned char)(**pstr)) - (size_t)'0') < 2) {
    r <<= 1;
    r |= d;
    ++*pstr;
    if ((r & UINT64_C(0x8000000000000000)))
      break;
#if FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER
    *pstr += (**pstr == '_') | (**pstr == '.'); /* allow as a dividers */
#endif
  }
#endif
  if (((size_t)(**pstr) - (size_t)'0') < 2)
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

SFUNC int64_t FIO___ASAN_AVOID fio_atol(char **pstr) {
  /* note: sanitizer avoided due to possible 8 byte overflow within mem-page */
  static uint64_t (*const fn[])(char **) = {
      fio_atol10u,
      fio_atol8u,
      fio_atol_bin,
      fio_atol16u,
  };
  if (!pstr || !(*pstr))
    return 0;
  union {
    uint64_t u64;
    int64_t i64;
  } u = {0};
  char *p = *pstr;

  uint32_t neg = 0, base = 0;
  neg = (p[0] == '-');
  p += (neg | (p[0] == '+'));

  base += (p[0] == '0');             /* starts with zero? - oct */
  p += base;                         /* consume the possible '0' */
  base += ((p[0] | 32) == 'b');      /* binary */
  base += ((p[0] | 32) == 'x') << 1; /* hex */
  p += (base > 1);                   /* consume 'b' or 'x' */
  char *const s = p;                 /* mark starting point */
  u.u64 = fn[base](&p);              /* convert string to unsigned long long */
  if (p != s || base == 1)           /* false oct base, a single '0'? */
    *pstr = p;
  if ((neg | !base)) /* if base 10 or negative, treat signed bit as overflow */
    return fio_u2i_limit(u.u64, neg);
  return u.i64;
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
  if ((*pstr)[0] == 'b' || ((*pstr)[1] == '0' && (*pstr)[1] == 'b'))
    goto binary_raw;
  return strtod(*pstr, pstr);
binary_raw:
  /* binary representation is assumed to spell an exact double */
  (void)0;
  union {
    uint64_t i;
    double d;
  } punned = {.i = (uint64_t)fio_atol_bin(pstr)};
  *pstr += ((**pstr | 32) == 'f'); /* support 0b1111111F */
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
fio_aton
***************************************************************************** */
/** Returns a power of 10. Supports values up to 1.0e308. */
FIO_IFUNC long double fio___aton_pow10(uint64_t e10) {
  // clang-format off
#define fio___aton_pow10_map_row(i) 1.0e##i##0L, 1.0e##i##1L, 1.0e##i##2L, 1.0e##i##3L, 1.0e##i##4L, 1.0e##i##5L, 1.0e##i##6L, 1.0e##i##7L, 1.0e##i##8L, 1.0e##i##9L
  static const long double pow_map[] = {
      fio___aton_pow10_map_row(0),  fio___aton_pow10_map_row(1),  fio___aton_pow10_map_row(2),  fio___aton_pow10_map_row(3),  fio___aton_pow10_map_row(4),
      fio___aton_pow10_map_row(5),  fio___aton_pow10_map_row(6),  fio___aton_pow10_map_row(7),  fio___aton_pow10_map_row(8),  fio___aton_pow10_map_row(9),
      fio___aton_pow10_map_row(10), fio___aton_pow10_map_row(11), fio___aton_pow10_map_row(12), fio___aton_pow10_map_row(13), fio___aton_pow10_map_row(14),
      fio___aton_pow10_map_row(15), fio___aton_pow10_map_row(16), fio___aton_pow10_map_row(17), fio___aton_pow10_map_row(18), fio___aton_pow10_map_row(19),
      fio___aton_pow10_map_row(20), fio___aton_pow10_map_row(21), fio___aton_pow10_map_row(22), fio___aton_pow10_map_row(23), fio___aton_pow10_map_row(24),
      fio___aton_pow10_map_row(25), fio___aton_pow10_map_row(26), fio___aton_pow10_map_row(27), fio___aton_pow10_map_row(28), fio___aton_pow10_map_row(29),
      1.0e300L, 1.0e301L, 1.0e302L, 1.0e303L, 1.0e304L, 1.0e305L, 1.0e306L, 1.0e307L, 1.0e308L, // clang-format on
  };
#undef fio___aton_pow10_map_row
  if (e10 < sizeof(pow_map) / sizeof(pow_map[0]))
    return pow_map[e10];
  return powl(10, e10); /* return infinity? */
}

/** Returns a power of 10. Supports values up to 1.0e-308. */
FIO_IFUNC long double fio___aton_pow10n(uint64_t e10) {
  // clang-format off
#define fio___aton_pow10_map_row(i) 1.0e-##i##0L, 1.0e-##i##1L, 1.0e-##i##2L, 1.0e-##i##3L, 1.0e-##i##4L, 1.0e-##i##5L, 1.0e-##i##6L, 1.0e-##i##7L, 1.0e-##i##8L, 1.0e-##i##9L
  static const long double pow_map[] = {
      fio___aton_pow10_map_row(0),  fio___aton_pow10_map_row(1),  fio___aton_pow10_map_row(2),  fio___aton_pow10_map_row(3),  fio___aton_pow10_map_row(4),
      fio___aton_pow10_map_row(5),  fio___aton_pow10_map_row(6),  fio___aton_pow10_map_row(7),  fio___aton_pow10_map_row(8),  fio___aton_pow10_map_row(9),
      fio___aton_pow10_map_row(10), fio___aton_pow10_map_row(11), fio___aton_pow10_map_row(12), fio___aton_pow10_map_row(13), fio___aton_pow10_map_row(14),
      fio___aton_pow10_map_row(15), fio___aton_pow10_map_row(16), fio___aton_pow10_map_row(17), fio___aton_pow10_map_row(18), fio___aton_pow10_map_row(19),
      fio___aton_pow10_map_row(20), fio___aton_pow10_map_row(21), fio___aton_pow10_map_row(22), fio___aton_pow10_map_row(23), fio___aton_pow10_map_row(24),
      fio___aton_pow10_map_row(25), fio___aton_pow10_map_row(26), fio___aton_pow10_map_row(27), fio___aton_pow10_map_row(28), fio___aton_pow10_map_row(29),
      1.0e-300L, 1.0e-301L, 1.0e-302L, 1.0e-303L, 1.0e-304L, 1.0e-305L, 1.0e-306L, 1.0e-307L, 1.0e-308L, // clang-format on
  };
#undef fio___aton_pow10_map_row
  if (e10 < sizeof(pow_map) / sizeof(pow_map[0]))
    return pow_map[e10];
  return powl(10, (int64_t)(0 - e10)); /* return zero? */
}

FIO_SFUNC FIO___ASAN_AVOID fio_aton_s fio_aton(char **pstr) {
  /* note: sanitizer avoided due to possible 8 byte overflow within mem-page */
  static uint64_t (*const fn[])(char **) = {
      fio_atol10u,
      fio_atol8u,
      fio_atol_bin,
      fio_atol16u,
  };
  static uint32_t base_limit[] = {10, 8, 1, 16};
  static char exponent_char[] = "eepp";
  fio_aton_s r = {0};
  long double dbl = 0, dbl_dot = 0;
  if (!pstr || !(*pstr))
    return r;
  char *start, *head, *p = *pstr;
  uint64_t before_dot = 0, after_dot = 0, expo = 0;
  size_t head_expo = 0, dot_expo = 0;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    ++p;

  uint16_t neg = 0, expo_neg = 0, base = 0, force_float = 0;
  neg = (p[0] == '-');
  p += (neg | (p[0] == '+'));

  if ((p[0] | 32) == 'i')
    goto is_infinity;
  if ((p[0] | 32) == 'n')
    goto is_nan;

  base += (p[0] == '0');             /* oct */
  p += base;                         /* consume '0' */
  base += ((p[0] | 32) == 'b');      /* binary */
  base += ((p[0] | 32) == 'x') << 1; /* hex */
  base -= (base & (p[0] == '.'));    /* 0. isn't oct...  */
  p += (base > 1);                   /* consume 'b' or 'x' */
  start = p;                         /* mark starting point */

  // FIO_LOG_INFO("Start Unsigned: %s", p);
  before_dot = fn[base]((char **)&p);
  if (base == 2)
    goto is_binary;
  head = p;
  while (fio_c2i(p[0]) < base_limit[base])
    ++p;
  head_expo = p - head;
  force_float |= !!(head_expo);
  if (p[0] == '.') {
    ++p;
    force_float = 1;
    head = p;
    after_dot = fn[base]((char **)&p);
    dot_expo = p - head;
    while (fio_c2i(p[0]) < base_limit[base])
      ++p;
  }
  if ((p[0] | 32) == exponent_char[base]) {
    force_float = 1;
    ++p;
    expo_neg = (p[0] == '-');
    p += (expo_neg | (p[0] == '+'));
    expo = fio_atol10u((char **)&p);
    while ((uint8_t)(p[0] - '0') < 10)
      ++p;
  }
  if (p != start || base == 1) /* false oct base, a single '0'? */
    *pstr = p;
  // FIO_LOG_INFO("Start Tail: %s", p);
  if (!force_float && (!(before_dot & ((uint64_t)1ULL << 63)) ||
                       (!neg && base))) { /* is integer */
    r.u = before_dot;
    if (neg)
      r.i = 0 - r.u;
    return r;
  }
  dbl = (long double)before_dot;
  dbl_dot = (long double)after_dot;
  if (!base) {
    dbl *= fio___aton_pow10(head_expo);
    if (after_dot)
      dbl_dot *= fio___aton_pow10n(dot_expo);
  } else if (base == 3) {
    dbl *= fio_u2d(1, (head_expo << 2));
    dbl_dot *= fio_u2d(1, 0 - (dot_expo << 2));
  } else { /* if (base == 1) */
    dbl *= fio_u2d(1, (head_expo * 3));
    dbl_dot *= fio_u2d(1, 0 - (dot_expo * 3));
  }
  dbl += dbl_dot;
  if (expo) {
    if (base < 2) { /* base 10 / Oct */
      dbl *= (expo_neg ? fio___aton_pow10n : fio___aton_pow10)(expo);
    } else {
      dbl *= fio_u2d(1, (int64_t)(expo_neg ? 0 - expo : expo));
    }
  }
  r.is_float = 1;
  r.f = (double)dbl;
  r.u |= (uint64_t)neg << 63;
  return r;

is_infinity:
  if ((p[1] | 32) == 'n' && (p[2] | 32) == 'f') { /* inf */
    r.is_float = 1;
    r.u = ((uint64_t)neg << 63) | ((uint64_t)2047ULL << 52);
    p += 3 + (((p[3] | 32) == 'i' &&
               fio_buf2u64u("infinity") ==
                   (fio_buf2u64u(p) | 0x2020202020202020ULL)) *
              5);
    *pstr = (char *)p;
  } else
    r.err = 1;
  return r;

is_nan:
  if ((p[1] | 32) == 'a' && (p[2] | 32) == 'n') { /* nan */
    r.is_float = 1;
    r.i = ((~(uint64_t)0) >> (!neg));
    p += 3;
    *pstr = (char *)p;
  } else
    r.err = 1;
  return r;

is_binary:
  if (p == start)
    return r;
  r.u = before_dot;
  r.is_float = ((p[0] | 32) == 'f');
  p += r.is_float;
  *pstr = (char *)p;
  return r;
}

/* *****************************************************************************
Big Numbers
***************************************************************************** */

FIO_IFUNC void fio___uXXX_hex_read(uint64_t *t, char **p, size_t l) {
  char *start = *p;
  start += (((unsigned)(start[0] == '0') & (start[1] == 'x')) << 1);
  char *pos = start;
  while (fio_i2c((uint8_t)*pos) < 16)
    ++pos;
  ++pos;
  *p = pos;
  for (size_t i = 0; i < l; ++i) { /* per uint64_t in t */
    uint64_t wrd = 0;
    for (size_t j = 0; j < 16 && pos > start; ++j) {
      --pos;
      wrd |= ((uint64_t)fio_i2c((uint8_t)*pos) << (j << 2));
    }
    *t = wrd;
    ++t;
  }
}

FIO_IFUNC size_t fio___uXXX_hex_write(char *dest, const uint64_t *t, size_t l) {
  while (--l && !t[l])
    ;
  if (!l && !t[0]) {
    dest[0] = '0';
    return 1;
  }
  char *pos = dest;
  size_t digits = fio_digits16u(t[l]);
  ++l;
  while (l--) {
    fio_ltoa16u(pos, t[l], digits);
    pos += digits;
    digits = 16;
  }
  return (size_t)(pos - dest);
}

/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u128 fio_u128_hex_read(char **pstr) {
  fio_u128 r;
  fio___uXXX_hex_read(r.u64, pstr, sizeof(r) / sizeof(r.u64[0]));
  return r;
}
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u256 fio_u256_hex_read(char **pstr) {
  fio_u256 r;
  fio___uXXX_hex_read(r.u64, pstr, sizeof(r) / sizeof(r.u64[0]));
  return r;
}
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u512 fio_u512_hex_read(char **pstr) {
  fio_u512 r;
  fio___uXXX_hex_read(r.u64, pstr, sizeof(r) / sizeof(r.u64[0]));
  return r;
}
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u1024 fio_u1024_hex_read(char **pstr) {
  fio_u1024 r;
  fio___uXXX_hex_read(r.u64, pstr, sizeof(r) / sizeof(r.u64[0]));
  return r;
}
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u2048 fio_u2048_hex_read(char **pstr) {
  fio_u2048 r;
  fio___uXXX_hex_read(r.u64, pstr, sizeof(r) / sizeof(r.u64[0]));
  return r;
}
/** Reads a hex numeral string and initializes the numeral. */
SFUNC fio_u4096 fio_u4096_hex_read(char **pstr) {
  fio_u4096 r;
  fio___uXXX_hex_read(r.u64, pstr, sizeof(r) / sizeof(r.u64[0]));
  return r;
}

/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u128_hex_write(char *dest, const fio_u128 *u) {
  return fio___uXXX_hex_write(dest, u->u64, sizeof(u->u64) / sizeof(u->u64[0]));
}
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u256_hex_write(char *dest, const fio_u256 *u) {
  return fio___uXXX_hex_write(dest, u->u64, sizeof(u->u64) / sizeof(u->u64[0]));
}
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u512_hex_write(char *dest, const fio_u512 *u) {
  return fio___uXXX_hex_write(dest, u->u64, sizeof(u->u64) / sizeof(u->u64[0]));
}
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u1024_hex_write(char *dest, const fio_u1024 *u) {
  return fio___uXXX_hex_write(dest, u->u64, sizeof(u->u64) / sizeof(u->u64[0]));
}
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u2048_hex_write(char *dest, const fio_u2048 *u) {
  return fio___uXXX_hex_write(dest, u->u64, sizeof(u->u64) / sizeof(u->u64[0]));
}
/** Prints out the underlying 64 bit array (for debugging). */
SFUNC size_t fio_u4096_hex_write(char *dest, const fio_u4096 *u) {
  return fio___uXXX_hex_write(dest, u->u64, sizeof(u->u64) / sizeof(u->u64[0]));
}

/* *****************************************************************************
Numbers <=> Strings - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_ATOL */
#undef FIO_ATOL
