/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MATH               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                    Basic Math Operations and Multi-Precision
                        Constant Time (when possible)



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_MATH) && !defined(H___FIO_MATH___H)
#define H___FIO_MATH___H 1

/* *****************************************************************************
Multi-precision, little endian helpers.

Works with little endian uint64_t arrays or 64 bit numbers.
***************************************************************************** */

/**
 * Multi-precision DIV for `len*64` bit long a, b.
 *
 * This is NOT constant time - use only with public values.
 *
 * Supports up to 1024 words (65,536 bits); longer lengths, like division by
 * zero, log an error and fill `dest` and `reminder` with `0xFF` bytes.
 *
 * The algorithm might be slow, as my math isn't that good and I couldn't
 * understand faster division algorithms (such as Newton–Raphson division)... so
 * this is sort of a factorized variation on long division.
 */
FIO_IFUNC void fio_math_div(uint64_t *dest,
                            uint64_t *reminder,
                            const uint64_t *a,
                            const uint64_t *b,
                            const size_t number_array_length);

/**
 * Multi-precision shift right for `len` word number `n`.
 *
 * `dest` and `n` must be the same buffer or not overlap. Shifts of `len * 64`
 * bits or more zero `dest`.
 */
FIO_IFUNC void fio_math_shr(uint64_t *dest,
                            uint64_t *n,
                            const size_t right_shift_bits,
                            size_t number_array_length);

/**
 * Multi-precision shift left for `len*64` bit number `n`.
 *
 * `dest` and `n` must be the same buffer or not overlap. Shifts of `len * 64`
 * bits or more zero `dest`.
 */
FIO_IFUNC void fio_math_shl(uint64_t *dest,
                            uint64_t *n,
                            const size_t left_shift_bits,
                            const size_t number_array_length);

/**
 * Multi-precision Inverse for `len*64` bit number `n` (turn `1` into `-1`).
 *
 * `dest` and `n` must be the same buffer or not overlap.
 */
FIO_IFUNC void fio_math_inv(uint64_t *dest, uint64_t *n, size_t len);

/** Multi-precision - returns the index for the most significant bit or -1. */
FIO_MIFN size_t fio_math_msb_index(uint64_t *n, const size_t len);

/** Multi-precision - returns the index for the least significant bit or -1. */
FIO_MIFN size_t fio_math_lsb_index(uint64_t *n, const size_t len);

/* *****************************************************************************
Multi-precision, little endian helpers. Works with full uint64_t arrays.
***************************************************************************** */

/** Maximum word count supported by `fio_math_div` (65,536 bit numbers). */
#define FIO___MATH_DIV_MAX_WORDS 1024

/** Multi-precision Inverse for `bits` number `n`. */
FIO_IFUNC void fio_math_inv(uint64_t *dest, uint64_t *n, const size_t len) {
  uint64_t c = 1;
  for (size_t i = 0; i < len; ++i) {
    const uint64_t tmp = ~n[i] + c;
    c &= (uint64_t)fio_ct_false(tmp); /* carry only past a zero result word */
    dest[i] = tmp;
  }
}

/** Multi-precision shift right for `bits` number `n`. */
FIO_IFUNC void fio_math_shr(uint64_t *dest,
                            uint64_t *n,
                            size_t bits,
                            const size_t len) {
  if (!len || !n || !dest)
    return;
  const size_t offset = bits >> 6;
  size_t i = 0;
  if (offset < len) {
    const size_t last = len - 1 - offset; /* last destination word with data */
    bits &= 63;
    /* ascending order: reads never trail writes, so `dest == n` is safe */
    for (; i < last; ++i)
      dest[i] =
          (n[i + offset] >> bits) | ((n[i + offset + 1] << (63 - bits)) << 1);
    dest[i++] = n[len - 1] >> bits;
  }
  for (; i < len; ++i)
    dest[i] = 0;
}

/** Multi-precision shift left for `bits` number `n`. */
FIO_IFUNC void fio_math_shl(uint64_t *dest,
                            uint64_t *n,
                            size_t bits,
                            const size_t len) {
  if (!len || !n || !dest)
    return;
  const size_t offset = bits >> 6;
  size_t i = len;
  if (offset < len) {
    bits &= 63;
    /* descending order: reads never trail writes, so `dest == n` is safe */
    for (i = len - 1; i > offset; --i)
      dest[i] =
          (n[i - offset] << bits) | ((n[i - offset - 1] >> (63 - bits)) >> 1);
    dest[i] = n[0] << bits;
  }
  while (i--)
    dest[i] = 0;
}

/** Multi-precision - returns the index for the most
 * significant bit. */
FIO_IFUNC size_t fio_math_msb_index(uint64_t *n, size_t len) {
  size_t r[2] = {0, (size_t)-1};
  uint64_t a = 0;
  while (len--) {
    const uint64_t mask = ((uint64_t)0ULL - (!a));
    a |= (mask & n[len]);
    r[0] += (64 & (~mask));
  }
  r[0] += fio_bits_msb_index(a);
  return r[!a];
}

/** Multi-precision - returns the index for the least
 * significant bit. */
FIO_IFUNC size_t fio_math_lsb_index(uint64_t *n, const size_t len) {
  size_t r[2] = {0, (size_t)-1};
  uint64_t a = 0;
  uint64_t mask = (~(uint64_t)0ULL);
  for (size_t i = 0; i < len; ++i) {
    a |= mask & n[i];
    mask = ((uint64_t)0ULL - (!a));
    r[0] += (64 & mask);
  }
  r[0] += fio_bits_lsb_index(a);
  return r[!a];
}

/**
 * Multi-precision DIV for `len*64` bit long a, b.
 *
 * This is NOT constant time.
 *
 * Uses binary long division: for each bit position from high to low,
 * if shifted divisor <= remainder, subtract it and set the quotient bit.
 */
FIO_IFUNC void fio_math_div(uint64_t *dest,
                            uint64_t *remainder,
                            const uint64_t *a,
                            const uint64_t *b,
                            const size_t len) {
  if (!len)
    return;

  /* Get MSB index of divisor (returns (size_t)-1 if zero) */
  const size_t b_msb = (len > FIO___MATH_DIV_MAX_WORDS)
                           ? (size_t)-1
                           : fio_math_msb_index((uint64_t *)b, len);
  if (b_msb == (size_t)-1) {
    /* Division by zero or unsupported length (bounds stack scratch) */
    if (len > FIO___MATH_DIV_MAX_WORDS)
      FIO_LOG_ERROR("fio_math_div length %zu exceeds %zu words (65536 bits)",
                    (size_t)len,
                    (size_t)FIO___MATH_DIV_MAX_WORDS);
    else
      FIO_LOG_ERROR("divide by zero!");
    if (dest)
      FIO_MEMSET(dest, 0xFF, sizeof(uint64_t) * len);
    if (remainder)
      FIO_MEMSET(remainder, 0xFF, sizeof(uint64_t) * len);
    return;
  }

#if !defined(_MSC_VER) && (!defined(__cplusplus) || __cplusplus > 201402L)
  uint64_t t[len];
  uint64_t r[len];
  uint64_t q[len];
#else
  uint64_t t[FIO___MATH_DIV_MAX_WORDS];
  uint64_t r[FIO___MATH_DIV_MAX_WORDS];
  uint64_t q[FIO___MATH_DIV_MAX_WORDS];
#endif

  /* Initialize: r = a (remainder starts as dividend), q = 0 */
  FIO_MEMCPY(r, a, sizeof(uint64_t) * len);
  FIO_MEMSET(q, 0, sizeof(uint64_t) * len);

  /* Get initial MSB of remainder */
  size_t r_msb = fio_math_msb_index((uint64_t *)r, len);

  /* If dividend is zero or smaller than divisor, quotient is 0, remainder is a
   */
  if (!(r_msb == (size_t)-1 || r_msb < b_msb)) {
    /* Binary long division: iterate from highest possible shift down to 0 */
    size_t shift = r_msb - b_msb;
    for (;;) {
      /* Shift divisor left to current position */
      fio_math_shl(t, (uint64_t *)b, shift, len);

      /* Try to subtract: compute r - t */
      uint64_t borrow = fio_math_sub(t, (uint64_t *)r, t, len);

      if (!borrow) {
        /* t <= r: subtraction succeeded, update remainder and set quotient bit
         */
        FIO_MEMCPY(r, t, sizeof(uint64_t) * len);
        q[shift >> 6] |= (1ULL << (shift & 63));

        /* Recalculate r_msb since remainder changed */
        r_msb = fio_math_msb_index((uint64_t *)r, len);

        /* If remainder is now zero or less than divisor, we're done */
        if (r_msb == (size_t)-1 || r_msb < b_msb)
          break;

        /* Update shift based on new remainder's MSB */
        shift = r_msb - b_msb;
      } else {
        /* t > r: can't subtract at this shift, try smaller shift */
        if (shift == 0)
          break;
        --shift;
      }
    }
  }

  /* Copy results to output */
  if (dest)
    FIO_MEMCPY(dest, q, len * sizeof(uint64_t));
  if (remainder)
    FIO_MEMCPY(remainder, r, len * sizeof(uint64_t));
}

/* *****************************************************************************
Math - cleanup
***************************************************************************** */
#undef FIO___MATH_DIV_MAX_WORDS
#endif /* FIO_MATH */
#undef FIO_MATH
