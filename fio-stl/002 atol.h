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
#include <float.h>
#include <inttypes.h>
#include <math.h>

#ifndef FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER
#define FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER 1
#endif

/* *****************************************************************************
Buffer Requirements (Guard-Byte Contract)
*****************************************************************************

Number parsing (`fio_atol*`, `fio_atof`, `fio_aton`) is optimized for
NUL-terminated strings and may read up to - and including - the first
non-numeric byte following a number (parsing stops at, but still reads,
that byte).

Callers MUST keep the buffer readable through that guard byte: pass a
NUL-terminated string or ensure a non-numeric guard byte follows the data.
`fio_bstr` strings always satisfy this contract. Exact-size buffers with
no readable byte past the number (e.g., a 1-byte allocation holding "1")
violate it, causing a 1-byte out-of-bounds read (AddressSanitizer
detectable, use `FIO_MEMORY_DISABLE` when sanitizing).

***************************************************************************** */

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
 * Binary with an `f` suffix (`0b...f`) is a float's raw IEEE 754 bits.
 *
 * Auto detects octal when number starts with zero.
 *
 * Auto detects the Strings "inf", "infinity" and "nan" as float values.
 *
 * The number's format and type are returned in the return type.
 *
 * If a numerical overflow or format error occurred, the `.err` flag is set.
 * Input without digits (i.e., "-", "." or "e5") sets `.err` and isn't consumed.
 * An exponent without digits (i.e., the `e` in "1e") isn't consumed.
 *
 * Decimal floats are correctly rounded (round-half-even, as `strtod`), using
 * Clinger's fast path and the Eisel-Lemire algorithm. Leading-zero numbers with
 * a fraction or exponent are decimal (e.g., `012.5` is 12.5, as in C).
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

/**
 * Converts String data to a double (locale independent, correctly rounded).
 *
 * Decimal (leading zeros are decimal, not octal), hex (`0x`), `inf` and `nan`
 * are parsed using `fio_aton`. Binary (`0b...`, optional `f` suffix) is read
 * as the raw IEEE 754 bit pattern. On error, returns 0 without moving `*pstr`.
 */
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
 * default to base 10. Base 2 writes the raw IEEE 754 bits as `0b...f`, which
 * `fio_aton` and `fio_atof` read back exactly. Base 16 writes a C99 `%a` hex
 * float (i.e., `-0x1.8p+0`), which `strtod`, `fio_aton` and `fio_atof` read
 * back exactly.
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

/** Reads a signed base 8 formatted number - may overflow buffer. */
SFUNC uint64_t fio_atol8u(char **pstr);
/** Reads a signed base 10 formatted number - may overflow buffer. */
SFUNC uint64_t fio_atol10u(char **pstr);
/** Reads an unsigned hex formatted number (possibly prefixed with "0x"). */
SFUNC uint64_t fio_atol16u(char **pstr);
/** Reads an unsigned binary formatted number (possibly prefixed with "0b"). */
SFUNC uint64_t fio_atol_bin(char **pstr);
/** Read an unsigned number in any base up to base 36. */
SFUNC uint64_t fio_atol_xbase(char **pstr, size_t base);

/* *****************************************************************************
Strict, Bounded Number Parsing (wire-safe)
***************************************************************************** */

/** Reads an unsigned base 10 number within `[pos, end)` - strict digits only
 * (no whitespace, sign, underscores or prefixes). Advances `pos` past the
 * digits consumed. On overflow sets `errno == E2BIG` and stops at the last
 * valid digit. Callers detect empty / trailing junk by testing `pos`. */
SFUNC uint64_t fio_stol10u(char **pos, const char *end);

/** Reads a signed base 10 number within `[pos, end)` - an optional leading
 * `-` / `+` followed by strict digits only. Sets `errno == E2BIG` on
 * overflow (either the magnitude or the signed range limit). */
SFUNC int64_t fio_stol10(char **pos, const char *end);

/** Reads an unsigned hex number within `[pos, end)` - strict hex digits
 * only (NO `0x` prefix, no underscores). Advances `pos` past the digits
 * consumed. On overflow sets `errno == E2BIG` and stops at the last valid
 * digit. */
SFUNC uint64_t fio_stol16u(char **pos, const char *end);

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
    return fio_digits10u((uint64_t)i);
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
    *dest-- = (char)('0' + (i & 7));
    i >>= 3;
  }
  *dest = (char)('0' + i);
}

FIO_IFUNC void fio_ltoa10u(char *dest, uint64_t i, size_t digits) {
  dest += digits;
  *dest-- = 0;
  while (i > 9) {
    uint64_t nxt = i / 10;
    *dest-- = (char)('0' + (i - (nxt * 10ULL)));
    i = nxt;
  }
  *dest = (char)('0' + (unsigned char)i);
}

FIO_IFUNC void fio_ltoa16u(char *dest, uint64_t i, size_t digits) {
  digits += (digits & 1U); /* force even number of digits */
  dest += digits;
  *dest-- = 0;
  while (digits) {
    digits -= 2;
    *dest-- = (char)fio_i2c((uint8_t)(i & 15));
    i >>= 4;
    *dest-- = (char)fio_i2c((uint8_t)(i & 15));
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
    *dest-- = (char)fio_i2c((uint8_t)(i - (nxt * base)));
    i = nxt;
  }
  *dest = (char)fio_i2c((uint8_t)i);
}

/** Converts an unsigned `val` to a signed `val`, with overflow protection. */
FIO_IFUNC int64_t fio_u2i_limit(uint64_t val, size_t to_negative) {
  if (!to_negative) {
    /* overflow? */
    if (!(val & 0x8000000000000000ULL))
      return (int64_t)val;
    errno = E2BIG;
    val = 0x7FFFFFFFFFFFFFFFULL;
    return (int64_t)val;
  }
  /* a magnitude of exactly 2**63 is INT64_MIN - valid, not an overflow */
  if (val == 0x8000000000000000ULL)
    return (int64_t)val;
  if (!(val & 0x8000000000000000ULL)) {
    val = (uint64_t)((int64_t)0LL - (int64_t)val);
    return (int64_t)val;
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
  exponent = (int64_t)((uint64_t)exponent << 52);
  u.u64 |= (uint64_t)exponent;
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
  /*
  a = []; 256.times { a.push(255) };
  128.times { |b| a[b] = (((b) >='0'.ord && (b) <='9'.ord) ?
                             (b) - '0'.ord :
                          ((b|32) >= 'a'.ord && (b|32) <= 'z'.ord) ?
                             (b|32) + 10 - 'a'.ord : 255) }
  puts "static const uint8_t fio___alphanumeric_map[256] = { #{
        a.join(", ")} };"
  */
  static const uint8_t fio___alphanumeric_map[256] = {
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
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
  if ((fio_c2i((uint8_t)(**pstr))) < 8)
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

/* *****************************************************************************
Strict, Bounded Number Parsing (wire-safe)
***************************************************************************** */

/** Strict, bounded unsigned base 10 read - see API notes. */
SFUNC uint64_t fio_stol10u(char **pos, const char *end) {
  const char *p = *pos;
  uint64_t r = 0;
  while (p < end) {
    const uint64_t d = (uint64_t)(p[0] - '0');
    if (d > 9ULL)
      break;
    if (r > ((~(uint64_t)0ULL) - d) / 10ULL) {
      errno = E2BIG;
      break;
    }
    r = (r * 10ULL) + d;
    ++p;
  }
  *pos = (char *)p;
  return r;
}

/** Strict, bounded signed base 10 read - see API notes. */
SFUNC int64_t fio_stol10(char **pos, const char *end) {
  const char *p = *pos;
  size_t inv = 0;
  if (p < end && (p[0] == '-' || p[0] == '+')) {
    inv = (size_t)(p[0] == '-');
    ++p;
  }
  *pos = (char *)p;
  return fio_u2i_limit(fio_stol10u(pos, end), inv);
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
  *pstr += ((pstr[0][0] == '0') && ((pstr[0][1] | 32) == 'x'));
  return r;
}

/** Reads an unsigned hex formatted number (possibly prefixed with "0x"). */
SFUNC uint64_t fio_atol16u(char **pstr) {
  uint64_t r = 0;
  size_t d;
  unsigned char *p = (unsigned char *)*pstr;
  p += ((p[0] == '0') && ((p[1] | 32) == 'x')) << 1;
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
  *pstr += ((pstr[0][0] == '0') && ((pstr[0][1] | 32) == 'x'));
  return r;
}

/** Strict, bounded unsigned hex read - see API notes. */
SFUNC uint64_t fio_stol16u(char **pos, const char *end) {
  const char *p = *pos;
  uint64_t r = 0;
  while (p < end) {
    const size_t d = (size_t)fio_c2i((unsigned char)p[0]);
    if (d > 15)
      break;
    if (r & UINT64_C(0xF000000000000000)) {
      errno = E2BIG;
      break;
    }
    r = (r << 4) | (uint64_t)d;
    ++p;
  }
  *pos = (char *)p;
  return r;
}

/** Reads an unsigned binary formatted number (possibly prefixed with "0b"). */
SFUNC FIO___ASAN_AVOID uint64_t fio_atol_bin(char **pstr) {
  uint64_t r = 0;
  *pstr += (**pstr == '0');
  *pstr += (**pstr | 32) == 'b' && (((size_t)(pstr[0][1]) - (size_t)'0') < 2);
#if (defined(FIO___ASAN_DETECTED) && FIO___ASAN_DETECTED) || 1
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
  if ((fio_c2i((uint8_t)(**pstr))) < base)
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

  base = (p[0] == '0'); /* starts with zero? - oct */
  p += base;            /* consume the possible '0' */
  if ((p[0] | 32) == 'b' && (uint8_t)(p[1] - '0') < 2)
    base = 2; /* binary: `0b` / `b` prefix followed by a binary digit */
  else if ((p[0] | 32) == 'x' && fio_c2i((uint8_t)p[1]) < 16)
    base = 3;      /* hex: `0x` / `x` prefix followed by a hex digit */
  p += (base > 1); /* consume 'b' or 'x' */
  char *const s = p;                             /* mark starting point */
  u.u64 = fn[base](&p);    /* convert string to unsigned long long */
  if (p != s || base == 1) /* false oct base, a single '0'? */
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
  union {
    uint64_t u;
    double d;
  } r = {0};
  if (!pstr || !(*pstr))
    return r.d;
  char *p = *pstr;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    ++p;
  const uint64_t neg = (p[0] == '-');
  p += (neg | (p[0] == '+'));
  if (p[0] == '-' || p[0] == '+' || p[0] == ' ' || p[0] == '\t' ||
      p[0] == '\n' || p[0] == '\r')
    return r.d; /* no conversion (a single sign, as `strtod`) */
  p += (p[0] == '0' && (p[1] | 32) == 'b' && (uint8_t)(p[2] - '0') < 2);
  if ((p[0] | 32) == 'b' && (uint8_t)(p[1] - '0') < 2) {
    r.u = fio_atol_bin(&p); /* raw IEEE 754 bits, the `f` suffix is optional */
    p += ((p[0] | 32) == 'f');
  } else {
    while (p[0] == '0' && (uint8_t)(p[1] - '0') < 10)
      ++p; /* leading zeros are decimal, not octal */
    fio_aton_s n = fio_aton(&p);
    if (n.err)
      return r.d;
    r.d = n.is_float ? n.f : (double)n.u;
  }
  r.u ^= neg << 63;
  *pstr = p;
  return r.d;
}

/* *****************************************************************************
fio_ftoa
***************************************************************************** */

/* fast base-10 (%g compatible) converter, implemented after fio___aton_dec2d */
FIO_SFUNC size_t fio___ftoa_g(char *dest, double num);
/** Reference implementation: snprintf("%g") + ".0" fixup (fallback path). */
FIO_SFUNC size_t fio___ftoa_g_snprintf(char *dest, double num) {
  size_t written = (size_t)snprintf(dest, 30, "%g", num);
  if ((written + 1) == 0)
    written = 0;
  if (written > 30)
    written = 30;
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
}

SFUNC size_t fio_ftoa(char *dest, double num, uint8_t base) {
  if (base == 2) { /* `0b...f`: raw IEEE 754 bits, read by fio_aton/fio_atof */
    union {
      uint64_t u;
      double d;
    } p = {.d = num};
    const size_t digits = fio_digits_bin(p.u);
    if (dest) {
      dest[0] = '0';
      dest[1] = 'b';
      fio_ltoa_bin(dest + 2, p.u, digits);
      dest[digits + 2] = 'f';
      dest[digits + 3] = 0;
    }
    return digits + 3;
  }
  size_t written = 0;

  if (isinf(num))
    goto is_inifinity;
  if (isnan(num))
    goto is_nan;

  if (base == 16) { /* C99 `%a` hex float (i.e., `-0x1.8p+0`), exact */
    union {
      uint64_t u;
      double d;
    } p = {.d = num};
    char tmp[32], *pos = (dest ? dest : tmp);
    uint64_t m = p.u & FIO_MATH_DBL_MANT_MASK;
    int64_t e = (int64_t)((p.u >> 52) & 2047);
    size_t digits = 13;
    if (p.u >> 63)
      *pos++ = '-';
    *pos++ = '0';
    *pos++ = 'x';
    *pos++ = (char)('0' + (e != 0)); /* subnormals and zero: `0x0.` */
    e = e ? (e - 1023) : (m ? -1022 : 0);
    if (m) {
      for (; !(m & 15); m >>= 4)
        --digits; /* trim trailing zeros */
      *pos++ = '.';
      for (size_t i = digits; i--; m >>= 4)
        pos[i] = (char)fio_i2c((uint8_t)(m & 15)) | 32; /* lower case */
      pos += digits;
    }
    *pos++ = 'p';
    *pos++ = (e < 0) ? '-' : '+';
    m = (uint64_t)(e < 0 ? -e : e);
    fio_ltoa10u(pos, m, (digits = fio_digits10u(m)));
    pos += digits;
    *pos = 0;
    return (size_t)(pos - (dest ? dest : tmp));
  }

  return fio___ftoa_g(dest, num);

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
/* Exact decimal fast path (Clinger) requires plain double evaluation. */
#ifndef FIO___ATON_CLINGER
#if defined(FLT_EVAL_METHOD) && FLT_EVAL_METHOD != 0 && FLT_EVAL_METHOD != 1
#define FIO___ATON_CLINGER 0
#else
#define FIO___ATON_CLINGER 1
#endif
#endif

/** Eisel-Lemire table: 128 bit truncated 5^q, q = -342..308 ({hi, lo}). */
static const uint64_t fio___aton_pow5[] = {
    // clang-format off
      0xEEF453D6923BD65AULL, 0x113FAA2906A13B3FULL, 0x9558B4661B6565F8ULL, 0x4AC7CA59A424C507ULL,
      0xBAAEE17FA23EBF76ULL, 0x5D79BCF00D2DF649ULL, 0xE95A99DF8ACE6F53ULL, 0xF4D82C2C107973DCULL,
      0x91D8A02BB6C10594ULL, 0x79071B9B8A4BE869ULL, 0xB64EC836A47146F9ULL, 0x9748E2826CDEE284ULL,
      0xE3E27A444D8D98B7ULL, 0xFD1B1B2308169B25ULL, 0x8E6D8C6AB0787F72ULL, 0xFE30F0F5E50E20F7ULL,
      0xB208EF855C969F4FULL, 0xBDBD2D335E51A935ULL, 0xDE8B2B66B3BC4723ULL, 0xAD2C788035E61382ULL,
      0x8B16FB203055AC76ULL, 0x4C3BCB5021AFCC31ULL, 0xADDCB9E83C6B1793ULL, 0xDF4ABE242A1BBF3DULL,
      0xD953E8624B85DD78ULL, 0xD71D6DAD34A2AF0DULL, 0x87D4713D6F33AA6BULL, 0x8672648C40E5AD68ULL,
      0xA9C98D8CCB009506ULL, 0x680EFDAF511F18C2ULL, 0xD43BF0EFFDC0BA48ULL, 0x0212BD1B2566DEF2ULL,
      0x84A57695FE98746DULL, 0x014BB630F7604B57ULL, 0xA5CED43B7E3E9188ULL, 0x419EA3BD35385E2DULL,
      0xCF42894A5DCE35EAULL, 0x52064CAC828675B9ULL, 0x818995CE7AA0E1B2ULL, 0x7343EFEBD1940993ULL,
      0xA1EBFB4219491A1FULL, 0x1014EBE6C5F90BF8ULL, 0xCA66FA129F9B60A6ULL, 0xD41A26E077774EF6ULL,
      0xFD00B897478238D0ULL, 0x8920B098955522B4ULL, 0x9E20735E8CB16382ULL, 0x55B46E5F5D5535B0ULL,
      0xC5A890362FDDBC62ULL, 0xEB2189F734AA831DULL, 0xF712B443BBD52B7BULL, 0xA5E9EC7501D523E4ULL,
      0x9A6BB0AA55653B2DULL, 0x47B233C92125366EULL, 0xC1069CD4EABE89F8ULL, 0x999EC0BB696E840AULL,
      0xF148440A256E2C76ULL, 0xC00670EA43CA250DULL, 0x96CD2A865764DBCAULL, 0x380406926A5E5728ULL,
      0xBC807527ED3E12BCULL, 0xC605083704F5ECF2ULL, 0xEBA09271E88D976BULL, 0xF7864A44C633682EULL,
      0x93445B8731587EA3ULL, 0x7AB3EE6AFBE0211DULL, 0xB8157268FDAE9E4CULL, 0x5960EA05BAD82964ULL,
      0xE61ACF033D1A45DFULL, 0x6FB92487298E33BDULL, 0x8FD0C16206306BABULL, 0xA5D3B6D479F8E056ULL,
      0xB3C4F1BA87BC8696ULL, 0x8F48A4899877186CULL, 0xE0B62E2929ABA83CULL, 0x331ACDABFE94DE87ULL,
      0x8C71DCD9BA0B4925ULL, 0x9FF0C08B7F1D0B14ULL, 0xAF8E5410288E1B6FULL, 0x07ECF0AE5EE44DD9ULL,
      0xDB71E91432B1A24AULL, 0xC9E82CD9F69D6150ULL, 0x892731AC9FAF056EULL, 0xBE311C083A225CD2ULL,
      0xAB70FE17C79AC6CAULL, 0x6DBD630A48AAF406ULL, 0xD64D3D9DB981787DULL, 0x092CBBCCDAD5B108ULL,
      0x85F0468293F0EB4EULL, 0x25BBF56008C58EA5ULL, 0xA76C582338ED2621ULL, 0xAF2AF2B80AF6F24EULL,
      0xD1476E2C07286FAAULL, 0x1AF5AF660DB4AEE1ULL, 0x82CCA4DB847945CAULL, 0x50D98D9FC890ED4DULL,
      0xA37FCE126597973CULL, 0xE50FF107BAB528A0ULL, 0xCC5FC196FEFD7D0CULL, 0x1E53ED49A96272C8ULL,
      0xFF77B1FCBEBCDC4FULL, 0x25E8E89C13BB0F7AULL, 0x9FAACF3DF73609B1ULL, 0x77B191618C54E9ACULL,
      0xC795830D75038C1DULL, 0xD59DF5B9EF6A2417ULL, 0xF97AE3D0D2446F25ULL, 0x4B0573286B44AD1DULL,
      0x9BECCE62836AC577ULL, 0x4EE367F9430AEC32ULL, 0xC2E801FB244576D5ULL, 0x229C41F793CDA73FULL,
      0xF3A20279ED56D48AULL, 0x6B43527578C1110FULL, 0x9845418C345644D6ULL, 0x830A13896B78AAA9ULL,
      0xBE5691EF416BD60CULL, 0x23CC986BC656D553ULL, 0xEDEC366B11C6CB8FULL, 0x2CBFBE86B7EC8AA8ULL,
      0x94B3A202EB1C3F39ULL, 0x7BF7D71432F3D6A9ULL, 0xB9E08A83A5E34F07ULL, 0xDAF5CCD93FB0CC53ULL,
      0xE858AD248F5C22C9ULL, 0xD1B3400F8F9CFF68ULL, 0x91376C36D99995BEULL, 0x23100809B9C21FA1ULL,
      0xB58547448FFFFB2DULL, 0xABD40A0C2832A78AULL, 0xE2E69915B3FFF9F9ULL, 0x16C90C8F323F516CULL,
      0x8DD01FAD907FFC3BULL, 0xAE3DA7D97F6792E3ULL, 0xB1442798F49FFB4AULL, 0x99CD11CFDF41779CULL,
      0xDD95317F31C7FA1DULL, 0x40405643D711D583ULL, 0x8A7D3EEF7F1CFC52ULL, 0x482835EA666B2572ULL,
      0xAD1C8EAB5EE43B66ULL, 0xDA3243650005EECFULL, 0xD863B256369D4A40ULL, 0x90BED43E40076A82ULL,
      0x873E4F75E2224E68ULL, 0x5A7744A6E804A291ULL, 0xA90DE3535AAAE202ULL, 0x711515D0A205CB36ULL,
      0xD3515C2831559A83ULL, 0x0D5A5B44CA873E03ULL, 0x8412D9991ED58091ULL, 0xE858790AFE9486C2ULL,
      0xA5178FFF668AE0B6ULL, 0x626E974DBE39A872ULL, 0xCE5D73FF402D98E3ULL, 0xFB0A3D212DC8128FULL,
      0x80FA687F881C7F8EULL, 0x7CE66634BC9D0B99ULL, 0xA139029F6A239F72ULL, 0x1C1FFFC1EBC44E80ULL,
      0xC987434744AC874EULL, 0xA327FFB266B56220ULL, 0xFBE9141915D7A922ULL, 0x4BF1FF9F0062BAA8ULL,
      0x9D71AC8FADA6C9B5ULL, 0x6F773FC3603DB4A9ULL, 0xC4CE17B399107C22ULL, 0xCB550FB4384D21D3ULL,
      0xF6019DA07F549B2BULL, 0x7E2A53A146606A48ULL, 0x99C102844F94E0FBULL, 0x2EDA7444CBFC426DULL,
      0xC0314325637A1939ULL, 0xFA911155FEFB5308ULL, 0xF03D93EEBC589F88ULL, 0x793555AB7EBA27CAULL,
      0x96267C7535B763B5ULL, 0x4BC1558B2F3458DEULL, 0xBBB01B9283253CA2ULL, 0x9EB1AAEDFB016F16ULL,
      0xEA9C227723EE8BCBULL, 0x465E15A979C1CADCULL, 0x92A1958A7675175FULL, 0x0BFACD89EC191EC9ULL,
      0xB749FAED14125D36ULL, 0xCEF980EC671F667BULL, 0xE51C79A85916F484ULL, 0x82B7E12780E7401AULL,
      0x8F31CC0937AE58D2ULL, 0xD1B2ECB8B0908810ULL, 0xB2FE3F0B8599EF07ULL, 0x861FA7E6DCB4AA15ULL,
      0xDFBDCECE67006AC9ULL, 0x67A791E093E1D49AULL, 0x8BD6A141006042BDULL, 0xE0C8BB2C5C6D24E0ULL,
      0xAECC49914078536DULL, 0x58FAE9F773886E18ULL, 0xDA7F5BF590966848ULL, 0xAF39A475506A899EULL,
      0x888F99797A5E012DULL, 0x6D8406C952429603ULL, 0xAAB37FD7D8F58178ULL, 0xC8E5087BA6D33B83ULL,
      0xD5605FCDCF32E1D6ULL, 0xFB1E4A9A90880A64ULL, 0x855C3BE0A17FCD26ULL, 0x5CF2EEA09A55067FULL,
      0xA6B34AD8C9DFC06FULL, 0xF42FAA48C0EA481EULL, 0xD0601D8EFC57B08BULL, 0xF13B94DAF124DA26ULL,
      0x823C12795DB6CE57ULL, 0x76C53D08D6B70858ULL, 0xA2CB1717B52481EDULL, 0x54768C4B0C64CA6EULL,
      0xCB7DDCDDA26DA268ULL, 0xA9942F5DCF7DFD09ULL, 0xFE5D54150B090B02ULL, 0xD3F93B35435D7C4CULL,
      0x9EFA548D26E5A6E1ULL, 0xC47BC5014A1A6DAFULL, 0xC6B8E9B0709F109AULL, 0x359AB6419CA1091BULL,
      0xF867241C8CC6D4C0ULL, 0xC30163D203C94B62ULL, 0x9B407691D7FC44F8ULL, 0x79E0DE63425DCF1DULL,
      0xC21094364DFB5636ULL, 0x985915FC12F542E4ULL, 0xF294B943E17A2BC4ULL, 0x3E6F5B7B17B2939DULL,
      0x979CF3CA6CEC5B5AULL, 0xA705992CEECF9C42ULL, 0xBD8430BD08277231ULL, 0x50C6FF782A838353ULL,
      0xECE53CEC4A314EBDULL, 0xA4F8BF5635246428ULL, 0x940F4613AE5ED136ULL, 0x871B7795E136BE99ULL,
      0xB913179899F68584ULL, 0x28E2557B59846E3FULL, 0xE757DD7EC07426E5ULL, 0x331AEADA2FE589CFULL,
      0x9096EA6F3848984FULL, 0x3FF0D2C85DEF7621ULL, 0xB4BCA50B065ABE63ULL, 0x0FED077A756B53A9ULL,
      0xE1EBCE4DC7F16DFBULL, 0xD3E8495912C62894ULL, 0x8D3360F09CF6E4BDULL, 0x64712DD7ABBBD95CULL,
      0xB080392CC4349DECULL, 0xBD8D794D96AACFB3ULL, 0xDCA04777F541C567ULL, 0xECF0D7A0FC5583A0ULL,
      0x89E42CAAF9491B60ULL, 0xF41686C49DB57244ULL, 0xAC5D37D5B79B6239ULL, 0x311C2875C522CED5ULL,
      0xD77485CB25823AC7ULL, 0x7D633293366B828BULL, 0x86A8D39EF77164BCULL, 0xAE5DFF9C02033197ULL,
      0xA8530886B54DBDEBULL, 0xD9F57F830283FDFCULL, 0xD267CAA862A12D66ULL, 0xD072DF63C324FD7BULL,
      0x8380DEA93DA4BC60ULL, 0x4247CB9E59F71E6DULL, 0xA46116538D0DEB78ULL, 0x52D9BE85F074E608ULL,
      0xCD795BE870516656ULL, 0x67902E276C921F8BULL, 0x806BD9714632DFF6ULL, 0x00BA1CD8A3DB53B6ULL,
      0xA086CFCD97BF97F3ULL, 0x80E8A40ECCD228A4ULL, 0xC8A883C0FDAF7DF0ULL, 0x6122CD128006B2CDULL,
      0xFAD2A4B13D1B5D6CULL, 0x796B805720085F81ULL, 0x9CC3A6EEC6311A63ULL, 0xCBE3303674053BB0ULL,
      0xC3F490AA77BD60FCULL, 0xBEDBFC4411068A9CULL, 0xF4F1B4D515ACB93BULL, 0xEE92FB5515482D44ULL,
      0x991711052D8BF3C5ULL, 0x751BDD152D4D1C4AULL, 0xBF5CD54678EEF0B6ULL, 0xD262D45A78A0635DULL,
      0xEF340A98172AACE4ULL, 0x86FB897116C87C34ULL, 0x9580869F0E7AAC0EULL, 0xD45D35E6AE3D4DA0ULL,
      0xBAE0A846D2195712ULL, 0x8974836059CCA109ULL, 0xE998D258869FACD7ULL, 0x2BD1A438703FC94BULL,
      0x91FF83775423CC06ULL, 0x7B6306A34627DDCFULL, 0xB67F6455292CBF08ULL, 0x1A3BC84C17B1D542ULL,
      0xE41F3D6A7377EECAULL, 0x20CABA5F1D9E4A93ULL, 0x8E938662882AF53EULL, 0x547EB47B7282EE9CULL,
      0xB23867FB2A35B28DULL, 0xE99E619A4F23AA43ULL, 0xDEC681F9F4C31F31ULL, 0x6405FA00E2EC94D4ULL,
      0x8B3C113C38F9F37EULL, 0xDE83BC408DD3DD04ULL, 0xAE0B158B4738705EULL, 0x9624AB50B148D445ULL,
      0xD98DDAEE19068C76ULL, 0x3BADD624DD9B0957ULL, 0x87F8A8D4CFA417C9ULL, 0xE54CA5D70A80E5D6ULL,
      0xA9F6D30A038D1DBCULL, 0x5E9FCF4CCD211F4CULL, 0xD47487CC8470652BULL, 0x7647C3200069671FULL,
      0x84C8D4DFD2C63F3BULL, 0x29ECD9F40041E073ULL, 0xA5FB0A17C777CF09ULL, 0xF468107100525890ULL,
      0xCF79CC9DB955C2CCULL, 0x7182148D4066EEB4ULL, 0x81AC1FE293D599BFULL, 0xC6F14CD848405530ULL,
      0xA21727DB38CB002FULL, 0xB8ADA00E5A506A7CULL, 0xCA9CF1D206FDC03BULL, 0xA6D90811F0E4851CULL,
      0xFD442E4688BD304AULL, 0x908F4A166D1DA663ULL, 0x9E4A9CEC15763E2EULL, 0x9A598E4E043287FEULL,
      0xC5DD44271AD3CDBAULL, 0x40EFF1E1853F29FDULL, 0xF7549530E188C128ULL, 0xD12BEE59E68EF47CULL,
      0x9A94DD3E8CF578B9ULL, 0x82BB74F8301958CEULL, 0xC13A148E3032D6E7ULL, 0xE36A52363C1FAF01ULL,
      0xF18899B1BC3F8CA1ULL, 0xDC44E6C3CB279AC1ULL, 0x96F5600F15A7B7E5ULL, 0x29AB103A5EF8C0B9ULL,
      0xBCB2B812DB11A5DEULL, 0x7415D448F6B6F0E7ULL, 0xEBDF661791D60F56ULL, 0x111B495B3464AD21ULL,
      0x936B9FCEBB25C995ULL, 0xCAB10DD900BEEC34ULL, 0xB84687C269EF3BFBULL, 0x3D5D514F40EEA742ULL,
      0xE65829B3046B0AFAULL, 0x0CB4A5A3112A5112ULL, 0x8FF71A0FE2C2E6DCULL, 0x47F0E785EABA72ABULL,
      0xB3F4E093DB73A093ULL, 0x59ED216765690F56ULL, 0xE0F218B8D25088B8ULL, 0x306869C13EC3532CULL,
      0x8C974F7383725573ULL, 0x1E414218C73A13FBULL, 0xAFBD2350644EEACFULL, 0xE5D1929EF90898FAULL,
      0xDBAC6C247D62A583ULL, 0xDF45F746B74ABF39ULL, 0x894BC396CE5DA772ULL, 0x6B8BBA8C328EB783ULL,
      0xAB9EB47C81F5114FULL, 0x066EA92F3F326564ULL, 0xD686619BA27255A2ULL, 0xC80A537B0EFEFEBDULL,
      0x8613FD0145877585ULL, 0xBD06742CE95F5F36ULL, 0xA798FC4196E952E7ULL, 0x2C48113823B73704ULL,
      0xD17F3B51FCA3A7A0ULL, 0xF75A15862CA504C5ULL, 0x82EF85133DE648C4ULL, 0x9A984D73DBE722FBULL,
      0xA3AB66580D5FDAF5ULL, 0xC13E60D0D2E0EBBAULL, 0xCC963FEE10B7D1B3ULL, 0x318DF905079926A8ULL,
      0xFFBBCFE994E5C61FULL, 0xFDF17746497F7052ULL, 0x9FD561F1FD0F9BD3ULL, 0xFEB6EA8BEDEFA633ULL,
      0xC7CABA6E7C5382C8ULL, 0xFE64A52EE96B8FC0ULL, 0xF9BD690A1B68637BULL, 0x3DFDCE7AA3C673B0ULL,
      0x9C1661A651213E2DULL, 0x06BEA10CA65C084EULL, 0xC31BFA0FE5698DB8ULL, 0x486E494FCFF30A62ULL,
      0xF3E2F893DEC3F126ULL, 0x5A89DBA3C3EFCCFAULL, 0x986DDB5C6B3A76B7ULL, 0xF89629465A75E01CULL,
      0xBE89523386091465ULL, 0xF6BBB397F1135823ULL, 0xEE2BA6C0678B597FULL, 0x746AA07DED582E2CULL,
      0x94DB483840B717EFULL, 0xA8C2A44EB4571CDCULL, 0xBA121A4650E4DDEBULL, 0x92F34D62616CE413ULL,
      0xE896A0D7E51E1566ULL, 0x77B020BAF9C81D17ULL, 0x915E2486EF32CD60ULL, 0x0ACE1474DC1D122EULL,
      0xB5B5ADA8AAFF80B8ULL, 0x0D819992132456BAULL, 0xE3231912D5BF60E6ULL, 0x10E1FFF697ED6C69ULL,
      0x8DF5EFABC5979C8FULL, 0xCA8D3FFA1EF463C1ULL, 0xB1736B96B6FD83B3ULL, 0xBD308FF8A6B17CB2ULL,
      0xDDD0467C64BCE4A0ULL, 0xAC7CB3F6D05DDBDEULL, 0x8AA22C0DBEF60EE4ULL, 0x6BCDF07A423AA96BULL,
      0xAD4AB7112EB3929DULL, 0x86C16C98D2C953C6ULL, 0xD89D64D57A607744ULL, 0xE871C7BF077BA8B7ULL,
      0x87625F056C7C4A8BULL, 0x11471CD764AD4972ULL, 0xA93AF6C6C79B5D2DULL, 0xD598E40D3DD89BCFULL,
      0xD389B47879823479ULL, 0x4AFF1D108D4EC2C3ULL, 0x843610CB4BF160CBULL, 0xCEDF722A585139BAULL,
      0xA54394FE1EEDB8FEULL, 0xC2974EB4EE658828ULL, 0xCE947A3DA6A9273EULL, 0x733D226229FEEA32ULL,
      0x811CCC668829B887ULL, 0x0806357D5A3F525FULL, 0xA163FF802A3426A8ULL, 0xCA07C2DCB0CF26F7ULL,
      0xC9BCFF6034C13052ULL, 0xFC89B393DD02F0B5ULL, 0xFC2C3F3841F17C67ULL, 0xBBAC2078D443ACE2ULL,
      0x9D9BA7832936EDC0ULL, 0xD54B944B84AA4C0DULL, 0xC5029163F384A931ULL, 0x0A9E795E65D4DF11ULL,
      0xF64335BCF065D37DULL, 0x4D4617B5FF4A16D5ULL, 0x99EA0196163FA42EULL, 0x504BCED1BF8E4E45ULL,
      0xC06481FB9BCF8D39ULL, 0xE45EC2862F71E1D6ULL, 0xF07DA27A82C37088ULL, 0x5D767327BB4E5A4CULL,
      0x964E858C91BA2655ULL, 0x3A6A07F8D510F86FULL, 0xBBE226EFB628AFEAULL, 0x890489F70A55368BULL,
      0xEADAB0ABA3B2DBE5ULL, 0x2B45AC74CCEA842EULL, 0x92C8AE6B464FC96FULL, 0x3B0B8BC90012929DULL,
      0xB77ADA0617E3BBCBULL, 0x09CE6EBB40173744ULL, 0xE55990879DDCAABDULL, 0xCC420A6A101D0515ULL,
      0x8F57FA54C2A9EAB6ULL, 0x9FA946824A12232DULL, 0xB32DF8E9F3546564ULL, 0x47939822DC96ABF9ULL,
      0xDFF9772470297EBDULL, 0x59787E2B93BC56F7ULL, 0x8BFBEA76C619EF36ULL, 0x57EB4EDB3C55B65AULL,
      0xAEFAE51477A06B03ULL, 0xEDE622920B6B23F1ULL, 0xDAB99E59958885C4ULL, 0xE95FAB368E45ECEDULL,
      0x88B402F7FD75539BULL, 0x11DBCB0218EBB414ULL, 0xAAE103B5FCD2A881ULL, 0xD652BDC29F26A119ULL,
      0xD59944A37C0752A2ULL, 0x4BE76D3346F0495FULL, 0x857FCAE62D8493A5ULL, 0x6F70A4400C562DDBULL,
      0xA6DFBD9FB8E5B88EULL, 0xCB4CCD500F6BB952ULL, 0xD097AD07A71F26B2ULL, 0x7E2000A41346A7A7ULL,
      0x825ECC24C873782FULL, 0x8ED400668C0C28C8ULL, 0xA2F67F2DFA90563BULL, 0x728900802F0F32FAULL,
      0xCBB41EF979346BCAULL, 0x4F2B40A03AD2FFB9ULL, 0xFEA126B7D78186BCULL, 0xE2F610C84987BFA8ULL,
      0x9F24B832E6B0F436ULL, 0x0DD9CA7D2DF4D7C9ULL, 0xC6EDE63FA05D3143ULL, 0x91503D1C79720DBBULL,
      0xF8A95FCF88747D94ULL, 0x75A44C6397CE912AULL, 0x9B69DBE1B548CE7CULL, 0xC986AFBE3EE11ABAULL,
      0xC24452DA229B021BULL, 0xFBE85BADCE996168ULL, 0xF2D56790AB41C2A2ULL, 0xFAE27299423FB9C3ULL,
      0x97C560BA6B0919A5ULL, 0xDCCD879FC967D41AULL, 0xBDB6B8E905CB600FULL, 0x5400E987BBC1C920ULL,
      0xED246723473E3813ULL, 0x290123E9AAB23B68ULL, 0x9436C0760C86E30BULL, 0xF9A0B6720AAF6521ULL,
      0xB94470938FA89BCEULL, 0xF808E40E8D5B3E69ULL, 0xE7958CB87392C2C2ULL, 0xB60B1D1230B20E04ULL,
      0x90BD77F3483BB9B9ULL, 0xB1C6F22B5E6F48C2ULL, 0xB4ECD5F01A4AA828ULL, 0x1E38AEB6360B1AF3ULL,
      0xE2280B6C20DD5232ULL, 0x25C6DA63C38DE1B0ULL, 0x8D590723948A535FULL, 0x579C487E5A38AD0EULL,
      0xB0AF48EC79ACE837ULL, 0x2D835A9DF0C6D851ULL, 0xDCDB1B2798182244ULL, 0xF8E431456CF88E65ULL,
      0x8A08F0F8BF0F156BULL, 0x1B8E9ECB641B58FFULL, 0xAC8B2D36EED2DAC5ULL, 0xE272467E3D222F3FULL,
      0xD7ADF884AA879177ULL, 0x5B0ED81DCC6ABB0FULL, 0x86CCBB52EA94BAEAULL, 0x98E947129FC2B4E9ULL,
      0xA87FEA27A539E9A5ULL, 0x3F2398D747B36224ULL, 0xD29FE4B18E88640EULL, 0x8EEC7F0D19A03AADULL,
      0x83A3EEEEF9153E89ULL, 0x1953CF68300424ACULL, 0xA48CEAAAB75A8E2BULL, 0x5FA8C3423C052DD7ULL,
      0xCDB02555653131B6ULL, 0x3792F412CB06794DULL, 0x808E17555F3EBF11ULL, 0xE2BBD88BBEE40BD0ULL,
      0xA0B19D2AB70E6ED6ULL, 0x5B6ACEAEAE9D0EC4ULL, 0xC8DE047564D20A8BULL, 0xF245825A5A445275ULL,
      0xFB158592BE068D2EULL, 0xEED6E2F0F0D56712ULL, 0x9CED737BB6C4183DULL, 0x55464DD69685606BULL,
      0xC428D05AA4751E4CULL, 0xAA97E14C3C26B886ULL, 0xF53304714D9265DFULL, 0xD53DD99F4B3066A8ULL,
      0x993FE2C6D07B7FABULL, 0xE546A8038EFE4029ULL, 0xBF8FDB78849A5F96ULL, 0xDE98520472BDD033ULL,
      0xEF73D256A5C0F77CULL, 0x963E66858F6D4440ULL, 0x95A8637627989AADULL, 0xDDE7001379A44AA8ULL,
      0xBB127C53B17EC159ULL, 0x5560C018580D5D52ULL, 0xE9D71B689DDE71AFULL, 0xAAB8F01E6E10B4A6ULL,
      0x9226712162AB070DULL, 0xCAB3961304CA70E8ULL, 0xB6B00D69BB55C8D1ULL, 0x3D607B97C5FD0D22ULL,
      0xE45C10C42A2B3B05ULL, 0x8CB89A7DB77C506AULL, 0x8EB98A7A9A5B04E3ULL, 0x77F3608E92ADB242ULL,
      0xB267ED1940F1C61CULL, 0x55F038B237591ED3ULL, 0xDF01E85F912E37A3ULL, 0x6B6C46DEC52F6688ULL,
      0x8B61313BBABCE2C6ULL, 0x2323AC4B3B3DA015ULL, 0xAE397D8AA96C1B77ULL, 0xABEC975E0A0D081AULL,
      0xD9C7DCED53C72255ULL, 0x96E7BD358C904A21ULL, 0x881CEA14545C7575ULL, 0x7E50D64177DA2E54ULL,
      0xAA242499697392D2ULL, 0xDDE50BD1D5D0B9E9ULL, 0xD4AD2DBFC3D07787ULL, 0x955E4EC64B44E864ULL,
      0x84EC3C97DA624AB4ULL, 0xBD5AF13BEF0B113EULL, 0xA6274BBDD0FADD61ULL, 0xECB1AD8AEACDD58EULL,
      0xCFB11EAD453994BAULL, 0x67DE18EDA5814AF2ULL, 0x81CEB32C4B43FCF4ULL, 0x80EACF948770CED7ULL,
      0xA2425FF75E14FC31ULL, 0xA1258379A94D028DULL, 0xCAD2F7F5359A3B3EULL, 0x096EE45813A04330ULL,
      0xFD87B5F28300CA0DULL, 0x8BCA9D6E188853FCULL, 0x9E74D1B791E07E48ULL, 0x775EA264CF55347EULL,
      0xC612062576589DDAULL, 0x95364AFE032A819EULL, 0xF79687AED3EEC551ULL, 0x3A83DDBD83F52205ULL,
      0x9ABE14CD44753B52ULL, 0xC4926A9672793543ULL, 0xC16D9A0095928A27ULL, 0x75B7053C0F178294ULL,
      0xF1C90080BAF72CB1ULL, 0x5324C68B12DD6339ULL, 0x971DA05074DA7BEEULL, 0xD3F6FC16EBCA5E04ULL,
      0xBCE5086492111AEAULL, 0x88F4BB1CA6BCF585ULL, 0xEC1E4A7DB69561A5ULL, 0x2B31E9E3D06C32E6ULL,
      0x9392EE8E921D5D07ULL, 0x3AFF322E62439FD0ULL, 0xB877AA3236A4B449ULL, 0x09BEFEB9FAD487C3ULL,
      0xE69594BEC44DE15BULL, 0x4C2EBE687989A9B4ULL, 0x901D7CF73AB0ACD9ULL, 0x0F9D37014BF60A11ULL,
      0xB424DC35095CD80FULL, 0x538484C19EF38C95ULL, 0xE12E13424BB40E13ULL, 0x2865A5F206B06FBAULL,
      0x8CBCCC096F5088CBULL, 0xF93F87B7442E45D4ULL, 0xAFEBFF0BCB24AAFEULL, 0xF78F69A51539D749ULL,
      0xDBE6FECEBDEDD5BEULL, 0xB573440E5A884D1CULL, 0x89705F4136B4A597ULL, 0x31680A88F8953031ULL,
      0xABCC77118461CEFCULL, 0xFDC20D2B36BA7C3EULL, 0xD6BF94D5E57A42BCULL, 0x3D32907604691B4DULL,
      0x8637BD05AF6C69B5ULL, 0xA63F9A49C2C1B110ULL, 0xA7C5AC471B478423ULL, 0x0FCF80DC33721D54ULL,
      0xD1B71758E219652BULL, 0xD3C36113404EA4A9ULL, 0x83126E978D4FDF3BULL, 0x645A1CAC083126EAULL,
      0xA3D70A3D70A3D70AULL, 0x3D70A3D70A3D70A4ULL, 0xCCCCCCCCCCCCCCCCULL, 0xCCCCCCCCCCCCCCCDULL,
      0x8000000000000000ULL, 0x0000000000000000ULL, 0xA000000000000000ULL, 0x0000000000000000ULL,
      0xC800000000000000ULL, 0x0000000000000000ULL, 0xFA00000000000000ULL, 0x0000000000000000ULL,
      0x9C40000000000000ULL, 0x0000000000000000ULL, 0xC350000000000000ULL, 0x0000000000000000ULL,
      0xF424000000000000ULL, 0x0000000000000000ULL, 0x9896800000000000ULL, 0x0000000000000000ULL,
      0xBEBC200000000000ULL, 0x0000000000000000ULL, 0xEE6B280000000000ULL, 0x0000000000000000ULL,
      0x9502F90000000000ULL, 0x0000000000000000ULL, 0xBA43B74000000000ULL, 0x0000000000000000ULL,
      0xE8D4A51000000000ULL, 0x0000000000000000ULL, 0x9184E72A00000000ULL, 0x0000000000000000ULL,
      0xB5E620F480000000ULL, 0x0000000000000000ULL, 0xE35FA931A0000000ULL, 0x0000000000000000ULL,
      0x8E1BC9BF04000000ULL, 0x0000000000000000ULL, 0xB1A2BC2EC5000000ULL, 0x0000000000000000ULL,
      0xDE0B6B3A76400000ULL, 0x0000000000000000ULL, 0x8AC7230489E80000ULL, 0x0000000000000000ULL,
      0xAD78EBC5AC620000ULL, 0x0000000000000000ULL, 0xD8D726B7177A8000ULL, 0x0000000000000000ULL,
      0x878678326EAC9000ULL, 0x0000000000000000ULL, 0xA968163F0A57B400ULL, 0x0000000000000000ULL,
      0xD3C21BCECCEDA100ULL, 0x0000000000000000ULL, 0x84595161401484A0ULL, 0x0000000000000000ULL,
      0xA56FA5B99019A5C8ULL, 0x0000000000000000ULL, 0xCECB8F27F4200F3AULL, 0x0000000000000000ULL,
      0x813F3978F8940984ULL, 0x4000000000000000ULL, 0xA18F07D736B90BE5ULL, 0x5000000000000000ULL,
      0xC9F2C9CD04674EDEULL, 0xA400000000000000ULL, 0xFC6F7C4045812296ULL, 0x4D00000000000000ULL,
      0x9DC5ADA82B70B59DULL, 0xF020000000000000ULL, 0xC5371912364CE305ULL, 0x6C28000000000000ULL,
      0xF684DF56C3E01BC6ULL, 0xC732000000000000ULL, 0x9A130B963A6C115CULL, 0x3C7F400000000000ULL,
      0xC097CE7BC90715B3ULL, 0x4B9F100000000000ULL, 0xF0BDC21ABB48DB20ULL, 0x1E86D40000000000ULL,
      0x96769950B50D88F4ULL, 0x1314448000000000ULL, 0xBC143FA4E250EB31ULL, 0x17D955A000000000ULL,
      0xEB194F8E1AE525FDULL, 0x5DCFAB0800000000ULL, 0x92EFD1B8D0CF37BEULL, 0x5AA1CAE500000000ULL,
      0xB7ABC627050305ADULL, 0xF14A3D9E40000000ULL, 0xE596B7B0C643C719ULL, 0x6D9CCD05D0000000ULL,
      0x8F7E32CE7BEA5C6FULL, 0xE4820023A2000000ULL, 0xB35DBF821AE4F38BULL, 0xDDA2802C8A800000ULL,
      0xE0352F62A19E306EULL, 0xD50B2037AD200000ULL, 0x8C213D9DA502DE45ULL, 0x4526F422CC340000ULL,
      0xAF298D050E4395D6ULL, 0x9670B12B7F410000ULL, 0xDAF3F04651D47B4CULL, 0x3C0CDD765F114000ULL,
      0x88D8762BF324CD0FULL, 0xA5880A69FB6AC800ULL, 0xAB0E93B6EFEE0053ULL, 0x8EEA0D047A457A00ULL,
      0xD5D238A4ABE98068ULL, 0x72A4904598D6D880ULL, 0x85A36366EB71F041ULL, 0x47A6DA2B7F864750ULL,
      0xA70C3C40A64E6C51ULL, 0x999090B65F67D924ULL, 0xD0CF4B50CFE20765ULL, 0xFFF4B4E3F741CF6DULL,
      0x82818F1281ED449FULL, 0xBFF8F10E7A8921A4ULL, 0xA321F2D7226895C7ULL, 0xAFF72D52192B6A0DULL,
      0xCBEA6F8CEB02BB39ULL, 0x9BF4F8A69F764490ULL, 0xFEE50B7025C36A08ULL, 0x02F236D04753D5B4ULL,
      0x9F4F2726179A2245ULL, 0x01D762422C946590ULL, 0xC722F0EF9D80AAD6ULL, 0x424D3AD2B7B97EF5ULL,
      0xF8EBAD2B84E0D58BULL, 0xD2E0898765A7DEB2ULL, 0x9B934C3B330C8577ULL, 0x63CC55F49F88EB2FULL,
      0xC2781F49FFCFA6D5ULL, 0x3CBF6B71C76B25FBULL, 0xF316271C7FC3908AULL, 0x8BEF464E3945EF7AULL,
      0x97EDD871CFDA3A56ULL, 0x97758BF0E3CBB5ACULL, 0xBDE94E8E43D0C8ECULL, 0x3D52EEED1CBEA317ULL,
      0xED63A231D4C4FB27ULL, 0x4CA7AAA863EE4BDDULL, 0x945E455F24FB1CF8ULL, 0x8FE8CAA93E74EF6AULL,
      0xB975D6B6EE39E436ULL, 0xB3E2FD538E122B44ULL, 0xE7D34C64A9C85D44ULL, 0x60DBBCA87196B616ULL,
      0x90E40FBEEA1D3A4AULL, 0xBC8955E946FE31CDULL, 0xB51D13AEA4A488DDULL, 0x6BABAB6398BDBE41ULL,
      0xE264589A4DCDAB14ULL, 0xC696963C7EED2DD1ULL, 0x8D7EB76070A08AECULL, 0xFC1E1DE5CF543CA2ULL,
      0xB0DE65388CC8ADA8ULL, 0x3B25A55F43294BCBULL, 0xDD15FE86AFFAD912ULL, 0x49EF0EB713F39EBEULL,
      0x8A2DBF142DFCC7ABULL, 0x6E3569326C784337ULL, 0xACB92ED9397BF996ULL, 0x49C2C37F07965404ULL,
      0xD7E77A8F87DAF7FBULL, 0xDC33745EC97BE906ULL, 0x86F0AC99B4E8DAFDULL, 0x69A028BB3DED71A3ULL,
      0xA8ACD7C0222311BCULL, 0xC40832EA0D68CE0CULL, 0xD2D80DB02AABD62BULL, 0xF50A3FA490C30190ULL,
      0x83C7088E1AAB65DBULL, 0x792667C6DA79E0FAULL, 0xA4B8CAB1A1563F52ULL, 0x577001B891185938ULL,
      0xCDE6FD5E09ABCF26ULL, 0xED4C0226B55E6F86ULL, 0x80B05E5AC60B6178ULL, 0x544F8158315B05B4ULL,
      0xA0DC75F1778E39D6ULL, 0x696361AE3DB1C721ULL, 0xC913936DD571C84CULL, 0x03BC3A19CD1E38E9ULL,
      0xFB5878494ACE3A5FULL, 0x04AB48A04065C723ULL, 0x9D174B2DCEC0E47BULL, 0x62EB0D64283F9C76ULL,
      0xC45D1DF942711D9AULL, 0x3BA5D0BD324F8394ULL, 0xF5746577930D6500ULL, 0xCA8F44EC7EE36479ULL,
      0x9968BF6ABBE85F20ULL, 0x7E998B13CF4E1ECBULL, 0xBFC2EF456AE276E8ULL, 0x9E3FEDD8C321A67EULL,
      0xEFB3AB16C59B14A2ULL, 0xC5CFE94EF3EA101EULL, 0x95D04AEE3B80ECE5ULL, 0xBBA1F1D158724A12ULL,
      0xBB445DA9CA61281FULL, 0x2A8A6E45AE8EDC97ULL, 0xEA1575143CF97226ULL, 0xF52D09D71A3293BDULL,
      0x924D692CA61BE758ULL, 0x593C2626705F9C56ULL, 0xB6E0C377CFA2E12EULL, 0x6F8B2FB00C77836CULL,
      0xE498F455C38B997AULL, 0x0B6DFB9C0F956447ULL, 0x8EDF98B59A373FECULL, 0x4724BD4189BD5EACULL,
      0xB2977EE300C50FE7ULL, 0x58EDEC91EC2CB657ULL, 0xDF3D5E9BC0F653E1ULL, 0x2F2967B66737E3EDULL,
      0x8B865B215899F46CULL, 0xBD79E0D20082EE74ULL, 0xAE67F1E9AEC07187ULL, 0xECD8590680A3AA11ULL,
      0xDA01EE641A708DE9ULL, 0xE80E6F4820CC9495ULL, 0x884134FE908658B2ULL, 0x3109058D147FDCDDULL,
      0xAA51823E34A7EEDEULL, 0xBD4B46F0599FD415ULL, 0xD4E5E2CDC1D1EA96ULL, 0x6C9E18AC7007C91AULL,
      0x850FADC09923329EULL, 0x03E2CF6BC604DDB0ULL, 0xA6539930BF6BFF45ULL, 0x84DB8346B786151CULL,
      0xCFE87F7CEF46FF16ULL, 0xE612641865679A63ULL, 0x81F14FAE158C5F6EULL, 0x4FCB7E8F3F60C07EULL,
      0xA26DA3999AEF7749ULL, 0xE3BE5E330F38F09DULL, 0xCB090C8001AB551CULL, 0x5CADF5BFD3072CC5ULL,
      0xFDCB4FA002162A63ULL, 0x73D9732FC7C8F7F6ULL, 0x9E9F11C4014DDA7EULL, 0x2867E7FDDCDD9AFAULL,
      0xC646D63501A1511DULL, 0xB281E1FD541501B8ULL, 0xF7D88BC24209A565ULL, 0x1F225A7CA91A4226ULL,
      0x9AE757596946075FULL, 0x3375788DE9B06958ULL, 0xC1A12D2FC3978937ULL, 0x0052D6B1641C83AEULL,
      0xF209787BB47D6B84ULL, 0xC0678C5DBD23A49AULL, 0x9745EB4D50CE6332ULL, 0xF840B7BA963646E0ULL,
      0xBD176620A501FBFFULL, 0xB650E5A93BC3D898ULL, 0xEC5D3FA8CE427AFFULL, 0xA3E51F138AB4CEBEULL,
      0x93BA47C980E98CDFULL, 0xC66F336C36B10137ULL, 0xB8A8D9BBE123F017ULL, 0xB80B0047445D4184ULL,
      0xE6D3102AD96CEC1DULL, 0xA60DC059157491E5ULL, 0x9043EA1AC7E41392ULL, 0x87C89837AD68DB2FULL,
      0xB454E4A179DD1877ULL, 0x29BABE4598C311FBULL, 0xE16A1DC9D8545E94ULL, 0xF4296DD6FEF3D67AULL,
      0x8CE2529E2734BB1DULL, 0x1899E4A65F58660CULL, 0xB01AE745B101E9E4ULL, 0x5EC05DCFF72E7F8FULL,
      0xDC21A1171D42645DULL, 0x76707543F4FA1F73ULL, 0x899504AE72497EBAULL, 0x6A06494A791C53A8ULL,
      0xABFA45DA0EDBDE69ULL, 0x0487DB9D17636892ULL, 0xD6F8D7509292D603ULL, 0x45A9D2845D3C42B6ULL,
      0x865B86925B9BC5C2ULL, 0x0B8A2392BA45A9B2ULL, 0xA7F26836F282B732ULL, 0x8E6CAC7768D7141EULL,
      0xD1EF0244AF2364FFULL, 0x3207D795430CD926ULL, 0x8335616AED761F1FULL, 0x7F44E6BD49E807B8ULL,
      0xA402B9C5A8D3A6E7ULL, 0x5F16206C9C6209A6ULL, 0xCD036837130890A1ULL, 0x36DBA887C37A8C0FULL,
      0x802221226BE55A64ULL, 0xC2494954DA2C9789ULL, 0xA02AA96B06DEB0FDULL, 0xF2DB9BAA10B7BD6CULL,
      0xC83553C5C8965D3DULL, 0x6F92829494E5ACC7ULL, 0xFA42A8B73ABBF48CULL, 0xCB772339BA1F17F9ULL,
      0x9C69A97284B578D7ULL, 0xFF2A760414536EFBULL, 0xC38413CF25E2D70DULL, 0xFEF5138519684ABAULL,
      0xF46518C2EF5B8CD1ULL, 0x7EB258665FC25D69ULL, 0x98BF2F79D5993802ULL, 0xEF2F773FFBD97A61ULL,
      0xBEEEFB584AFF8603ULL, 0xAAFB550FFACFD8FAULL, 0xEEAABA2E5DBF6784ULL, 0x95BA2A53F983CF38ULL,
      0x952AB45CFA97A0B2ULL, 0xDD945A747BF26183ULL, 0xBA756174393D88DFULL, 0x94F971119AEEF9E4ULL,
      0xE912B9D1478CEB17ULL, 0x7A37CD5601AAB85DULL, 0x91ABB422CCB812EEULL, 0xAC62E055C10AB33AULL,
      0xB616A12B7FE617AAULL, 0x577B986B314D6009ULL, 0xE39C49765FDF9D94ULL, 0xED5A7E85FDA0B80BULL,
      0x8E41ADE9FBEBC27DULL, 0x14588F13BE847307ULL, 0xB1D219647AE6B31CULL, 0x596EB2D8AE258FC8ULL,
      0xDE469FBD99A05FE3ULL, 0x6FCA5F8ED9AEF3BBULL, 0x8AEC23D680043BEEULL, 0x25DE7BB9480D5854ULL,
      0xADA72CCC20054AE9ULL, 0xAF561AA79A10AE6AULL, 0xD910F7FF28069DA4ULL, 0x1B2BA1518094DA04ULL,
      0x87AA9AFF79042286ULL, 0x90FB44D2F05D0842ULL, 0xA99541BF57452B28ULL, 0x353A1607AC744A53ULL,
      0xD3FA922F2D1675F2ULL, 0x42889B8997915CE8ULL, 0x847C9B5D7C2E09B7ULL, 0x69956135FEBADA11ULL,
      0xA59BC234DB398C25ULL, 0x43FAB9837E699095ULL, 0xCF02B2C21207EF2EULL, 0x94F967E45E03F4BBULL,
      0x8161AFB94B44F57DULL, 0x1D1BE0EEBAC278F5ULL, 0xA1BA1BA79E1632DCULL, 0x6462D92A69731732ULL,
      0xCA28A291859BBF93ULL, 0x7D7B8F7503CFDCFEULL, 0xFCB2CB35E702AF78ULL, 0x5CDA735244C3D43EULL,
      0x9DEFBF01B061ADABULL, 0x3A0888136AFA64A7ULL, 0xC56BAEC21C7A1916ULL, 0x088AAA1845B8FDD0ULL,
      0xF6C69A72A3989F5BULL, 0x8AAD549E57273D45ULL, 0x9A3C2087A63F6399ULL, 0x36AC54E2F678864BULL,
      0xC0CB28A98FCF3C7FULL, 0x84576A1BB416A7DDULL, 0xF0FDF2D3F3C30B9FULL, 0x656D44A2A11C51D5ULL,
      0x969EB7C47859E743ULL, 0x9F644AE5A4B1B325ULL, 0xBC4665B596706114ULL, 0x873D5D9F0DDE1FEEULL,
      0xEB57FF22FC0C7959ULL, 0xA90CB506D155A7EAULL, 0x9316FF75DD87CBD8ULL, 0x09A7F12442D588F2ULL,
      0xB7DCBF5354E9BECEULL, 0x0C11ED6D538AEB2FULL, 0xE5D3EF282A242E81ULL, 0x8F1668C8A86DA5FAULL,
      0x8FA475791A569D10ULL, 0xF96E017D694487BCULL, 0xB38D92D760EC4455ULL, 0x37C981DCC395A9ACULL,
      0xE070F78D3927556AULL, 0x85BBE253F47B1417ULL, 0x8C469AB843B89562ULL, 0x93956D7478CCEC8EULL,
      0xAF58416654A6BABBULL, 0x387AC8D1970027B2ULL, 0xDB2E51BFE9D0696AULL, 0x06997B05FCC0319EULL,
      0x88FCF317F22241E2ULL, 0x441FECE3BDF81F03ULL, 0xAB3C2FDDEEAAD25AULL, 0xD527E81CAD7626C3ULL,
      0xD60B3BD56A5586F1ULL, 0x8A71E223D8D3B074ULL, 0x85C7056562757456ULL, 0xF6872D5667844E49ULL,
      0xA738C6BEBB12D16CULL, 0xB428F8AC016561DBULL, 0xD106F86E69D785C7ULL, 0xE13336D701BEBA52ULL,
      0x82A45B450226B39CULL, 0xECC0024661173473ULL, 0xA34D721642B06084ULL, 0x27F002D7F95D0190ULL,
      0xCC20CE9BD35C78A5ULL, 0x31EC038DF7B441F4ULL, 0xFF290242C83396CEULL, 0x7E67047175A15271ULL,
      0x9F79A169BD203E41ULL, 0x0F0062C6E984D386ULL, 0xC75809C42C684DD1ULL, 0x52C07B78A3E60868ULL,
      0xF92E0C3537826145ULL, 0xA7709A56CCDF8A82ULL, 0x9BBCC7A142B17CCBULL, 0x88A66076400BB691ULL,
      0xC2ABF989935DDBFEULL, 0x6ACFF893D00EA435ULL, 0xF356F7EBF83552FEULL, 0x0583F6B8C4124D43ULL,
      0x98165AF37B2153DEULL, 0xC3727A337A8B704AULL, 0xBE1BF1B059E9A8D6ULL, 0x744F18C0592E4C5CULL,
      0xEDA2EE1C7064130CULL, 0x1162DEF06F79DF73ULL, 0x9485D4D1C63E8BE7ULL, 0x8ADDCB5645AC2BA8ULL,
      0xB9A74A0637CE2EE1ULL, 0x6D953E2BD7173692ULL, 0xE8111C87C5C1BA99ULL, 0xC8FA8DB6CCDD0437ULL,
      0x910AB1D4DB9914A0ULL, 0x1D9C9892400A22A2ULL, 0xB54D5E4A127F59C8ULL, 0x2503BEB6D00CAB4BULL,
      0xE2A0B5DC971F303AULL, 0x2E44AE64840FD61DULL, 0x8DA471A9DE737E24ULL, 0x5CEAECFED289E5D2ULL,
      0xB10D8E1456105DADULL, 0x7425A83E872C5F47ULL, 0xDD50F1996B947518ULL, 0xD12F124E28F77719ULL,
      0x8A5296FFE33CC92FULL, 0x82BD6B70D99AAA6FULL, 0xACE73CBFDC0BFB7BULL, 0x636CC64D1001550BULL,
      0xD8210BEFD30EFA5AULL, 0x3C47F7E05401AA4EULL, 0x8714A775E3E95C78ULL, 0x65ACFAEC34810A71ULL,
      0xA8D9D1535CE3B396ULL, 0x7F1839A741A14D0DULL, 0xD31045A8341CA07CULL, 0x1EDE48111209A050ULL,
      0x83EA2B892091E44DULL, 0x934AED0AAB460432ULL, 0xA4E4B66B68B65D60ULL, 0xF81DA84D5617853FULL,
      0xCE1DE40642E3F4B9ULL, 0x36251260AB9D668EULL, 0x80D2AE83E9CE78F3ULL, 0xC1D72B7C6B426019ULL,
      0xA1075A24E4421730ULL, 0xB24CF65B8612F81FULL, 0xC94930AE1D529CFCULL, 0xDEE033F26797B627ULL,
      0xFB9B7CD9A4A7443CULL, 0x169840EF017DA3B1ULL, 0x9D412E0806E88AA5ULL, 0x8E1F289560EE864EULL,
      0xC491798A08A2AD4EULL, 0xF1A6F2BAB92A27E2ULL, 0xF5B5D7EC8ACB58A2ULL, 0xAE10AF696774B1DBULL,
      0x9991A6F3D6BF1765ULL, 0xACCA6DA1E0A8EF29ULL, 0xBFF610B0CC6EDD3FULL, 0x17FD090A58D32AF3ULL,
      0xEFF394DCFF8A948EULL, 0xDDFC4B4CEF07F5B0ULL, 0x95F83D0A1FB69CD9ULL, 0x4ABDAF101564F98EULL,
      0xBB764C4CA7A4440FULL, 0x9D6D1AD41ABE37F1ULL, 0xEA53DF5FD18D5513ULL, 0x84C86189216DC5EDULL,
      0x92746B9BE2F8552CULL, 0x32FD3CF5B4E49BB4ULL, 0xB7118682DBB66A77ULL, 0x3FBC8C33221DC2A1ULL,
      0xE4D5E82392A40515ULL, 0x0FABAF3FEAA5334AULL, 0x8F05B1163BA6832DULL, 0x29CB4D87F2A7400EULL,
      0xB2C71D5BCA9023F8ULL, 0x743E20E9EF511012ULL, 0xDF78E4B2BD342CF6ULL, 0x914DA9246B255416ULL,
      0x8BAB8EEFB6409C1AULL, 0x1AD089B6C2F7548EULL, 0xAE9672ABA3D0C320ULL, 0xA184AC2473B529B1ULL,
      0xDA3C0F568CC4F3E8ULL, 0xC9E5D72D90A2741EULL, 0x8865899617FB1871ULL, 0x7E2FA67C7A658892ULL,
      0xAA7EEBFB9DF9DE8DULL, 0xDDBB901B98FEEAB7ULL, 0xD51EA6FA85785631ULL, 0x552A74227F3EA565ULL,
      0x8533285C936B35DEULL, 0xD53A88958F87275FULL, 0xA67FF273B8460356ULL, 0x8A892ABAF368F137ULL,
      0xD01FEF10A657842CULL, 0x2D2B7569B0432D85ULL, 0x8213F56A67F6B29BULL, 0x9C3B29620E29FC73ULL,
      0xA298F2C501F45F42ULL, 0x8349F3BA91B47B8FULL, 0xCB3F2F7642717713ULL, 0x241C70A936219A73ULL,
      0xFE0EFB53D30DD4D7ULL, 0xED238CD383AA0110ULL, 0x9EC95D1463E8A506ULL, 0xF4363804324A40AAULL,
      0xC67BB4597CE2CE48ULL, 0xB143C6053EDCD0D5ULL, 0xF81AA16FDC1B81DAULL, 0xDD94B7868E94050AULL,
      0x9B10A4E5E9913128ULL, 0xCA7CF2B4191C8326ULL, 0xC1D4CE1F63F57D72ULL, 0xFD1C2F611F63A3F0ULL,
      0xF24A01A73CF2DCCFULL, 0xBC633B39673C8CECULL, 0x976E41088617CA01ULL, 0xD5BE0503E085D813ULL,
      0xBD49D14AA79DBC82ULL, 0x4B2D8644D8A74E18ULL, 0xEC9C459D51852BA2ULL, 0xDDF8E7D60ED1219EULL,
      0x93E1AB8252F33B45ULL, 0xCABB90E5C942B503ULL, 0xB8DA1662E7B00A17ULL, 0x3D6A751F3B936243ULL,
      0xE7109BFBA19C0C9DULL, 0x0CC512670A783AD4ULL, 0x906A617D450187E2ULL, 0x27FB2B80668B24C5ULL,
      0xB484F9DC9641E9DAULL, 0xB1F9F660802DEDF6ULL, 0xE1A63853BBD26451ULL, 0x5E7873F8A0396973ULL,
      0x8D07E33455637EB2ULL, 0xDB0B487B6423E1E8ULL, 0xB049DC016ABC5E5FULL, 0x91CE1A9A3D2CDA62ULL,
      0xDC5C5301C56B75F7ULL, 0x7641A140CC7810FBULL, 0x89B9B3E11B6329BAULL, 0xA9E904C87FCB0A9DULL,
      0xAC2820D9623BF429ULL, 0x546345FA9FBDCD44ULL, 0xD732290FBACAF133ULL, 0xA97C177947AD4095ULL,
      0x867F59A9D4BED6C0ULL, 0x49ED8EABCCCC485DULL, 0xA81F301449EE8C70ULL, 0x5C68F256BFFF5A74ULL,
      0xD226FC195C6A2F8CULL, 0x73832EEC6FFF3111ULL, 0x83585D8FD9C25DB7ULL, 0xC831FD53C5FF7EABULL,
      0xA42E74F3D032F525ULL, 0xBA3E7CA8B77F5E55ULL, 0xCD3A1230C43FB26FULL, 0x28CE1BD2E55F35EBULL,
      0x80444B5E7AA7CF85ULL, 0x7980D163CF5B81B3ULL, 0xA0555E361951C366ULL, 0xD7E105BCC332621FULL,
      0xC86AB5C39FA63440ULL, 0x8DD9472BF3FEFAA7ULL, 0xFA856334878FC150ULL, 0xB14F98F6F0FEB951ULL,
      0x9C935E00D4B9D8D2ULL, 0x6ED1BF9A569F33D3ULL, 0xC3B8358109E84F07ULL, 0x0A862F80EC4700C8ULL,
      0xF4A642E14C6262C8ULL, 0xCD27BB612758C0FAULL, 0x98E7E9CCCFBD7DBDULL, 0x8038D51CB897789CULL,
      0xBF21E44003ACDD2CULL, 0xE0470A63E6BD56C3ULL, 0xEEEA5D5004981478ULL, 0x1858CCFCE06CAC74ULL,
      0x95527A5202DF0CCBULL, 0x0F37801E0C43EBC8ULL, 0xBAA718E68396CFFDULL, 0xD30560258F54E6BAULL,
      0xE950DF20247C83FDULL, 0x47C6B82EF32A2069ULL, 0x91D28B7416CDD27EULL, 0x4CDC331D57FA5441ULL,
      0xB6472E511C81471DULL, 0xE0133FE4ADF8E952ULL, 0xE3D8F9E563A198E5ULL, 0x58180FDDD97723A6ULL,
      0x8E679C2F5E44FF8FULL, 0x570F09EAA7EA7648ULL,
    // clang-format on
};

/** Returns `w * 10^q` correctly rounded (Clinger fast path + Eisel-Lemire). */
FIO_SFUNC double fio___aton_dec2d(uint64_t w, int64_t q) {
  static const double p10[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,
                               1e8,  1e9,  1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
                               1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};
  union {
    uint64_t u;
    double d;
  } r = {0};
  if (FIO___ATON_CLINGER && w <= ((uint64_t)1 << 53) && q >= -22 && q <= 22) {
    const double a = (double)w, m = p10[q < 0 ? -q : q]; /* both exact */
    const double dv = a / m, ml = a * m; /* branchless: sign is unpredictable */
    return q < 0 ? dv : ml;
  }
  if (!w || q < -342)
    return r.d;
  if (q > 308)
    goto is_infinity;
  {
    const uint64_t *p5 = fio___aton_pow5 + ((q + 342) << 1);
    const int64_t lz = 63 - (int64_t)fio_msb_index_unsafe(w);
    uint64_t hi, hi2, lo, m, up;
    int64_t e2;
    w <<= lz;
    lo = fio_math_mulc64(w, p5[0], &hi);
    if ((hi & 0x1FF) == 0x1FF) { /* proven sufficient (Mushtak & Lemire) */
      m = fio_math_mulc64(w, p5[1], &hi2); /* only the high word is used */
      lo += hi2;
      hi += (lo < hi2);
    }
    up = hi >> 63;
    m = hi >> (up + 9); /* 54 bits: mantissa + rounding bit */
    e2 = ((217706 * q) >> 16) + 63 + (int64_t)up - lz + 1023;
    if (e2 <= 0) { /* subnormal (rounding may produce the smallest normal) */
      if (e2 < -62)
        return r.d;
      m >>= (1 - e2);
      r.u = (m + (m & 1)) >> 1;
      return r.d;
    }
    if (lo <= 1 && q >= -4 && q <= 23 && (m & 3) == 1 && (m << (up + 9)) == hi)
      m &= ~(uint64_t)1; /* exact tie - round to even */
    m = (m + (m & 1)) >> 1;
    e2 += (int64_t)(m >> 53);
    if (e2 >= 0x7FF)
      goto is_infinity;
    r.u = ((uint64_t)e2 << 52) | (m & FIO_MATH_DBL_MANT_MASK);
    return r.d;
  }
is_infinity:
  r.u = FIO_MATH_DBL_EXPO_MASK;
  return r.d;
}

/**
 * Writes `num` to `dest` (up to 39 bytes + NUL), matching the output of the
 * snprintf("%g") reference path (with the ".0" fixup), without the snprintf
 * overhead (~4-8x faster). `num` MUST be finite (fio_ftoa handles inf/nan).
 *
 * Approach (contributed by the Iodine project, MIT licensed):
 * - frexp() for a decimal exponent estimate, corrected one step if needed.
 * - fio___aton_dec2d(1, q) for correctly-rounded powers of ten (reuses the
 *   Eisel-Lemire machinery - no new tables).
 * - Rounds the scaled 6-digit mantissa; falls back to the snprintf reference
 *   path for extreme exponents (pow10 out of table range) and values within
 *   ~1e-7 (relative) of a rounding boundary, where exact half-to-even
 *   rounding of the exact decimal expansion is required (frequency ~2e-7 per
 *   random value - the snprintf cost is amortized away).
 */
FIO_SFUNC size_t fio___ftoa_g(char *dest, double num) {
  char *p = dest;
  double a = num;
  uint64_t bits;
  FIO_MEMCPY(&bits, &num, 8);
  if (bits >> 63) {
    *p++ = '-';
    a = -a;
  }
  if (a == 0) { /* %g prints "0" (or "-0"), fio_ftoa adds ".0" */
    FIO_MEMCPY(p, "0.0", 4);
    return (size_t)(p + 3 - dest);
  }
  /* decimal exponent estimate: a = m * 2^e2, m in [0.5, 1) */
  int e2;
  frexp(a, &e2);
  int e10 = (int)(((int64_t)e2 * 78913) >> 18); /* ~floor(log10(a)) +/- 1 */
  if (FIO_UNLIKELY(e10 < -302 || e10 > 302))
    return fio___ftoa_g_snprintf(dest, num); /* pow10 out of table range */
  /* scale to a 6 digit mantissa (q ~= a * 10^(5-e10) in [1e5, 1e6)) */
  double q = a * fio___aton_dec2d(1, 5 - e10);
  if (q >= 1e6 || q < 1e5) { /* correct the estimate (off by one at most) */
    e10 += (q >= 1e6) ? 1 : -1;
    q = a * fio___aton_dec2d(1, 5 - e10);
  }
  double fl = floor(q);
  if (FIO_UNLIKELY(q - fl > 0.4999999 && q - fl < 0.5000001))
    return fio___ftoa_g_snprintf(dest, num); /* rounding boundary - tie break */
  uint64_t r = (uint64_t)fl + (q - fl >= 0.5);
  if (FIO_UNLIKELY(r == 1000000)) { /* rounded up to the next power of 10 */
    r = 100000;
    ++e10;
  }
  /* the 6 digits of r (d.ddddd * 10^e10) */
  char dig[6];
  dig[5] = (char)('0' + (r % 10));
  r /= 10;
  dig[4] = (char)('0' + (r % 10));
  r /= 10;
  dig[3] = (char)('0' + (r % 10));
  r /= 10;
  dig[2] = (char)('0' + (r % 10));
  r /= 10;
  dig[1] = (char)('0' + (r % 10));
  r /= 10;
  dig[0] = (char)('0' + r);
  if (e10 >= -4 && e10 < 6) {
    /* %f style (trimming trailing zeros after the decimal point) */
    if (e10 >= 0) {
      const int idig = e10 + 1; /* integer digits (1..6) */
      FIO_MEMCPY(p, dig, (size_t)idig);
      p += idig;
      int f = 5;
      while (f >= idig && dig[f] == '0')
        --f;
      if (f >= idig) {
        *p++ = '.';
        FIO_MEMCPY(p, dig + idig, (size_t)(f - idig + 1));
        p += f - idig + 1;
      } else {
        FIO_MEMCPY(p, ".0", 3); /* fio_ftoa's ".0" fixup */
        p += 2;
      }
    } else {
      FIO_MEMCPY(p, "0.", 2);
      p += 2;
      for (int z = -e10 - 1; z--;)
        *p++ = '0';
      int f = 5;
      while (f > 0 && dig[f] == '0')
        --f;
      FIO_MEMCPY(p, dig, (size_t)(f + 1));
      p += f + 1;
    }
  } else {
    /* %e style: d[.ddddd]e+XX (trimming trailing zeros in the mantissa) */
    *p++ = dig[0];
    int f = 5;
    while (f > 0 && dig[f] == '0')
      --f;
    if (f) {
      *p++ = '.';
      FIO_MEMCPY(p, dig + 1, (size_t)f);
      p += f;
    }
    *p++ = 'e';
    *p++ = (e10 < 0) ? '-' : '+';
    int x = (e10 < 0) ? -e10 : e10;
    if (x >= 100)
      *p++ = (char)('0' + (x / 100));
    *p++ = (char)('0' + ((x / 10) % 10));
    *p++ = (char)('0' + (x % 10));
  }
  *p = 0;
  return (size_t)(p - dest);
}

/**
 * Appends decimal digits to `w` (up to 19 significant digits), adding `q_add`
 * (appended) or `q_drop` (dropped) to `q` per digit, flags dropped non-zeros.
 */
FIO_IFUNC char *fio___aton_digits(char *p,
                                  uint64_t *w,
                                  int64_t *q,
                                  uint64_t *truncated,
                                  int64_t q_add,
                                  int64_t q_drop) {
#define FIO___ATON_ISD(i) ((uint8_t)(p[i] - '0') < 10)
  /* 8 digits at a time (SWAR), each byte validated before the 8 byte read */
  while (*w < 100000000000ULL && FIO___ATON_ISD(0) && FIO___ATON_ISD(1) &&
         FIO___ATON_ISD(2) && FIO___ATON_ISD(3) && FIO___ATON_ISD(4) &&
         FIO___ATON_ISD(5) && FIO___ATON_ISD(6) && FIO___ATON_ISD(7)) {
    uint64_t x = fio_buf2u64_le(p);
    x = ((x & 0x0F0F0F0F0F0F0F0FULL) * 2561ULL) >> 8;
    x = ((x & 0x00FF00FF00FF00FFULL) * 6553601ULL) >> 16;
    x = ((x & 0x0000FFFF0000FFFFULL) * 42949672960001ULL) >> 32;
    *w = (*w * 100000000ULL) + x;
    *q += q_add * 8;
    p += 8;
  }
#undef FIO___ATON_ISD
  for (uint64_t d; (d = fio_c2i((uint8_t)p[0])) < 10; ++p) {
    if (*w < 1000000000000000000ULL) {
      *w = (*w * 10) + d;
      *q += q_add;
    } else {
      *truncated |= d;
      *q += q_drop;
    }
#if FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER
    if (FIO_UNLIKELY(p[1] == '_')) /* a branch keeps `p` off the load chain */
      ++p;
#endif
  }
  return p;
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
  fio_aton_s r = {0};
  long double dbl = 0, dbl_dot = 0;
  if (!pstr || !(*pstr))
    return r;
  char *start, *head, *p = *pstr;
  uint64_t before_dot = 0, after_dot = 0, expo = 0;
  size_t head_expo = 0, dot_expo = 0;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    ++p;

  uint16_t neg = 0, expo_neg = 0, base = 0, force_float = 0, zero;
  neg = (p[0] == '-');
  p += (neg | (p[0] == '+'));

  if ((p[0] | 32) == 'i')
    goto is_infinity;
  if ((p[0] | 32) == 'n')
    goto is_nan;

  zero = (uint16_t)(p[0] == '0'); /* oct (or a decimal's leading zero) */
  p += zero;
  base = zero;
  if ((p[0] | 32) == 'b' && (uint8_t)(p[1] - '0') < 2)
    base = 2; /* binary: `0b` / `b` prefix followed by a binary digit */
  else if ((p[0] | 32) == 'x' && (fio_c2i((uint8_t)p[1]) < 16 || p[1] == '.'))
    base = 3; /* hex: `0x` / `x` prefix followed by a hex digit (or dot) */
  p += (base > 1); /* consume 'b' or 'x' */
  start = p;       /* mark starting point */
  if (base == 1) { /* leading zero: octal, unless it's a decimal float */
    for (head = p; (uint8_t)(head[0] - '0') < 10 || head[0] == '_'; ++head)
      ;
    base = (uint16_t)(head[0] != '.' && (head[0] | 32) != 'e');
  }

  if (!base)
    goto is_decimal;
  before_dot = fn[base]((char **)&p);
  if (base == 2)
    goto is_binary;
  head = p;
  while (fio_c2i((uint8_t)(p[0])) < base_limit[base])
    ++p;
  head_expo = (size_t)(p - head);
  force_float |= !!(head_expo);
  if (p[0] == '.') { /* hex (octal floats are parsed as decimal) */
    ++p;
    force_float = 1;
    head = p;
    after_dot = fn[base]((char **)&p);
    dot_expo = (size_t)(p - head);
    while (fio_c2i((uint8_t)(p[0])) < base_limit[base])
      ++p;
  }
  if ((p[0] | 32) == 'p' && base == 3) {
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
  if (!force_float && (!(before_dot >> 63) || !neg)) { /* is integer */
    r.u = before_dot;
    if (neg)
      r.i = (int64_t)(0 - r.u);
    return r;
  }
  dbl = (long double)before_dot;
  dbl_dot = (long double)after_dot;
  if (base == 3) {
    dbl *= fio_u2d(1U, (int64_t)(head_expo << 2));
    dbl_dot *= fio_u2d(1U, ((int64_t)0 - (dot_expo << 2)));
  } else { /* if (base == 1) */
    dbl *= fio_u2d(1U, (int64_t)(head_expo * 3));
    dbl_dot *= fio_u2d(1U, ((int64_t)0 - (dot_expo * 3)));
  }
  dbl += dbl_dot;
  if (expo)
    dbl *=
        fio_u2d(1U,
                (expo_neg ? (int64_t)((uint64_t)0ULL - expo) : (int64_t)expo));
  r.is_float = 1;
  r.f = (double)dbl;
  r.u |= (uint64_t)neg << 63;
  return r;

is_decimal : {
  /* collect up to 19 significant digits in `w`, value is `w * 10^q` */
  uint64_t w = 0, truncated = 0, d;
  int64_t q = 0;
  p = fio___aton_digits(p, &w, &q, &truncated, 0, 1);
  force_float = (q != 0); /* over 19 digits */
  zero |= (uint16_t)(p != start);
  if (p[0] == '.') {
    force_float = 1;
    head = p + 1;
    p = fio___aton_digits(head, &w, &q, &truncated, -1, 0);
    zero |= (uint16_t)(p != head);
  }
  if (!zero) { /* no digits (i.e., "-", ".", "e5"): don't consume */
    r.err = 1;
    return r;
  }
  if ((p[0] | 32) == 'e') {
    head = p + 1;
    expo_neg = (head[0] == '-');
    head += (expo_neg | (head[0] == '+'));
    if ((uint8_t)(head[0] - '0') < 10) { /* exponent needs digits (strtod) */
      force_float = 1;
      for (p = head; (d = (uint64_t)(uint8_t)(p[0] - '0')) < 10; ++p)
        expo = (expo < 0xFFFFFFFULL) ? ((expo * 10) + d) : expo; /* saturate */
      q += expo_neg ? (0 - (int64_t)expo) : (int64_t)expo;
    }
  }
  if (p != start)
    *pstr = p;
  if (!force_float && !(w >> 63)) { /* is integer */
    r.u = w;
    if (neg)
      r.i = (int64_t)(0 - r.u);
    return r;
  }
  r.is_float = 1;
  r.f = fio___aton_dec2d(w, q);
  if (FIO_UNLIKELY(truncated) && r.f != fio___aton_dec2d(w + 1, q)) {
    double tmp = strtod(start, &head); /* rare: digits beyond 19 matter */
    if (head == p) /* ignore if `strtod` disagrees on format (i.e., locale) */
      r.f = tmp;
  }
  r.u |= (uint64_t)neg << 63;
  return r;
}

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
    r.u = ((uint64_t)neg << 63) | 0x7FF8000000000000ULL; /* quiet NaN */
    p += 3;
    *pstr = (char *)p;
  } else
    r.err = 1;
  return r;

is_binary:
  if (p == start)
    return r;
  r.u = before_dot;
  r.is_float = ((p[0] | 32) == 'f'); /* `0b...f`: raw IEEE 754 bits */
  p += r.is_float;
  *pstr = (char *)p;
  if (neg) /* negate: flip the sign bit of a float, or negate an integer */
    r.u = r.is_float ? (r.u ^ ((uint64_t)1 << 63)) : (0 - r.u);
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
