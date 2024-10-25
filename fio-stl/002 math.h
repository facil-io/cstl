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
 * This is NOT constant time.
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

/** Multi-precision shift right for `len` word number `n`. */
FIO_IFUNC void fio_math_shr(uint64_t *dest,
                            uint64_t *n,
                            const size_t right_shift_bits,
                            size_t number_array_length);

/** Multi-precision shift left for `len*64` bit number `n`. */
FIO_IFUNC void fio_math_shl(uint64_t *dest,
                            uint64_t *n,
                            const size_t left_shift_bits,
                            const size_t number_array_length);

/** Multi-precision Inverse for `len*64` bit number `n` (turn `1` into `-1`). */
FIO_IFUNC void fio_math_inv(uint64_t *dest, uint64_t *n, size_t len);

/** Multi-precision - returns the index for the most significant bit or -1. */
FIO_MIFN size_t fio_math_msb_index(uint64_t *n, const size_t len);

/** Multi-precision - returns the index for the least significant bit or -1. */
FIO_MIFN size_t fio_math_lsb_index(uint64_t *n, const size_t len);

/* *****************************************************************************
Multi-precision, little endian helpers. Works with full uint64_t arrays.
***************************************************************************** */

/** Multi-precision Inverse for `bits` number `n`. */
FIO_IFUNC void fio_math_inv(uint64_t *dest, uint64_t *n, const size_t len) {
  uint64_t c = 1;
  for (size_t i = 0; i < len; ++i) {
    uint64_t tmp = ~n[i] + c;
    c = (tmp ^ n[i]) >> 63;
    dest[i] = tmp;
  }
}

/** Multi-precision shift right for `bits` number `n`. */
FIO_IFUNC void fio_math_shr(uint64_t *dest,
                            uint64_t *n,
                            size_t bits,
                            size_t len) {
  const size_t offset = len - (bits >> 6);
  bits &= 63;
  // FIO_LOG_DEBUG("Shift Light of %zu bytes and %zu
  // bits", len - offset, bits);
  uint64_t c = 0, trash;
  uint64_t *p_select[] = {dest + offset, &trash};
  if (bits) {
    while (len--) {
      --p_select[0];
      uint64_t ntmp = n[len];
      uint64_t ctmp = (ntmp << (64 - bits));
      dest[len] &= (uint64_t)0ULL - (len < offset);
      p_select[p_select[0] < dest][0] = ((ntmp >> bits) | c);
      c = ctmp;
    }
    return;
  }
  while (len--) {
    --p_select[0];
    uint64_t ntmp = n[len];
    dest[len] &= (uint64_t)0ULL - (len < offset);
    p_select[p_select[0] < dest][0] = ntmp;
  }
}

/** Multi-precision shift left for `bits` number `n`. */
FIO_IFUNC void fio_math_shl(uint64_t *dest,
                            uint64_t *n,
                            size_t bits,
                            const size_t len) {
  if (!len || !bits || !n || !dest)
    return;
  const size_t offset = bits >> 6;
  bits &= 63;
  uint64_t c = 0, trash;
  uint64_t *p_select[] = {dest + offset, &trash};
  if (bits) {
    for (size_t i = 0; i < len; (++i), ++p_select[0]) {
      uint64_t ntmp = n[i];
      uint64_t ctmp = (ntmp >> (64 - bits)) & ((uint64_t)0ULL - (!!bits));
      ;
      dest[i] &= (uint64_t)0ULL - (i >= offset);
      p_select[p_select[0] >= (dest + len)][0] = ((ntmp << bits) | c);
      c = ctmp;
    }
    return;
  }
  for (size_t i = 0; i < len; (++i), ++p_select[0]) {
    uint64_t ntmp = n[i];
    dest[i] &= (uint64_t)0ULL - (i >= offset);
    p_select[p_select[0] >= (dest + len)][0] = ntmp;
  }
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

/** Multi-precision DIV for `len*64` bit long a, b. NOT
 * constant time. */
FIO_IFUNC void fio_math_div(uint64_t *dest,
                            uint64_t *reminder,
                            const uint64_t *a,
                            const uint64_t *b,
                            const size_t len) {
  if (!len)
    return;
#if !defined(_MSC_VER) && (!defined(__cplusplus) || __cplusplus > 201402L)
  uint64_t t[len];
  uint64_t r[len];
  uint64_t q[len];
#else
  uint64_t t[256];
  uint64_t r[256];
  uint64_t q[256];
  FIO_ASSERT(
      len <= 256,
      "Multi Precision DIV (fio_math_div) overflows at 16384 bit numbers");
#endif
  FIO_MEMCPY(r, a, sizeof(uint64_t) * len);
  FIO_MEMSET(q, 0, sizeof(uint64_t) * len);
  size_t rlen;
  uint64_t c, mask, imask;
  const size_t blen = fio_math_msb_index((uint64_t *)b, len) + 1;
  if (!blen)
    goto divide_by_zero; /* divide by zero! */
  while ((rlen = fio_math_msb_index((uint64_t *)r, len)) >= blen) {
    const size_t delta = rlen - blen;
    fio_math_shl(t, (uint64_t *)b, delta, len);
    (void)fio_math_sub(r, (uint64_t *)r, t, len);
    q[delta >> 6] |= (1ULL << (delta & 63)); /* set the bit used */
  }
  mask = (uint64_t)0ULL - fio_math_sub(t, (uint64_t *)r, (uint64_t *)b, len);
  imask = ~mask; /* r was >= b */
  q[0] = fio_math_addc64(q[0], (imask & 1), 0, &c);
  for (size_t i = 1; i < len; ++i) {
    q[i] = fio_math_addc64(q[i], 0, c, &c);
  }
  if (dest) {
    FIO_MEMCPY(dest, q, len * sizeof(uint64_t));
  }
  if (reminder) {
    for (size_t i = 0; i < len; ++i) {
      reminder[i] = (t[i] & imask) | (r[i] & mask);
    }
  }
  return;
divide_by_zero:
  FIO_LOG_ERROR("divide by zero!");
  if (dest)
    FIO_MEMSET(dest, 0xFFFFFFFF, sizeof(*dest) * len);
  if (reminder)
    FIO_MEMSET(reminder, 0xFFFFFFFF, sizeof(*dest) * len);
  return;
}

/* *****************************************************************************
Math - cleanup
***************************************************************************** */
#endif /* FIO_MATH */
#undef FIO_MATH
#undef FIO_MIFN
