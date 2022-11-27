/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___   /* Development inclusion - ignore line */
#define FIO_MATH       /* Development inclusion - ignore line */
#include "./include.h" /* Development inclusion - ignore line */
#endif                 /* Development inclusion - ignore line */
/* *****************************************************************************




                    Basic Math Operations and Multi-Precision
                        Constant Time (when possible)



Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#if defined(FIO_MATH) && !defined(H___FIO_MATH___H)
#define H___FIO_MATH___H 1

/* *****************************************************************************
Useful math unions.
***************************************************************************** */

/** An unsigned 128bit union type. */
typedef union {
  uint8_t u8[16];
  uint16_t u16[8];
  uint32_t u32[4];
  uint64_t u64[2];
#if defined(__SIZEOF_INT128__)
  __uint128_t u128[1];
#endif
} fio_128u;

/** An unsigned 256bit union type. */
typedef union {
  uint8_t u8[32];
  uint16_t u16[16];
  uint32_t u32[8];
  uint64_t u64[4];
#if defined(__SIZEOF_INT128__)
  __uint128_t u128[2];
#endif
#if defined(__SIZEOF_INT256__)
  __uint256_t u256[1];
#endif

} fio_256u;

/** An unsigned 512bit union type. */
typedef union {
  uint8_t u8[64];
  uint16_t u16[32];
  uint32_t u32[16];
  uint64_t u64[8];
#if defined(__SIZEOF_INT128__)
  __uint128_t u128[4];
#endif
#if defined(__SIZEOF_INT256__)
  __uint256_t u256[2];
#endif
} fio_512u;

/* *****************************************************************************
64bit addition (ADD) / subtraction (SUB) / multiplication (MUL) with carry.
***************************************************************************** */

/** Add with carry. */
FIO_IFUNC uint64_t fio_math_addc64(uint64_t a,
                                   uint64_t b,
                                   uint64_t carry_in,
                                   uint64_t *carry_out);
/** Subtract with carry. */
FIO_IFUNC uint64_t fio_math_subc64(uint64_t a,
                                   uint64_t b,
                                   uint64_t carry_in,
                                   uint64_t *carry_out);
/** Multiply with carry out. */
FIO_IFUNC uint64_t fio_math_mulc64(uint64_t a, uint64_t b, uint64_t *carry_out);

/* *****************************************************************************
Multi-precision, little endian helpers.

Works with little endian uint64_t arrays or 64 bit numbers.
***************************************************************************** */

/** Multi-precision ADD for `len*64` bit long a + b. Returns the carry. */
FIO_IFUNC uint64_t fio_math_add(uint64_t *dest,
                                const uint64_t *a,
                                const uint64_t *b,
                                const size_t number_array_length);

/** Multi-precision SUB for `len*64` bit long a + b. Returns the carry. */
FIO_IFUNC uint64_t fio_math_sub(uint64_t *dest,
                                const uint64_t *a,
                                const uint64_t *b,
                                const size_t number_array_length);

/** Multi-precision MUL for `len*64` bit long a, b. `dest` must be `len*2` .*/
FIO_IFUNC void fio_math_mul(uint64_t *restrict dest,
                            const uint64_t *a,
                            const uint64_t *b,
                            const size_t number_array_length);

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
FIO_IFUNC size_t fio_math_msb_index(uint64_t *n, const size_t len);

/** Multi-precision - returns the index for the least significant bit or -1. */
FIO_IFUNC size_t fio_math_lsb_index(uint64_t *n, const size_t len);

/* *****************************************************************************
64bit addition (ADD) / subtraction (SUB) / multiplication (MUL) with carry.
***************************************************************************** */

/** Add with carry. */
FIO_IFUNC uint64_t fio_math_addc64(uint64_t a,
                                   uint64_t b,
                                   uint64_t carry_in,
                                   uint64_t *carry_out) {
  FIO_ASSERT_DEBUG(carry_out, "fio_math_addc64 requires a carry pointer");
#if __has_builtin(__builtin_addcll)
  return __builtin_addcll(a, b, carry_in, carry_out);
#elif defined(__SIZEOF_INT128__) && 0
  /* This is actually slower as it occupies more CPU registers */
  __uint128_t u = (__uint128_t)a + b + carry_in;
  *carry_out = (uint64_t)(u >> 64U);
  return (uint64_t)u;
#else
  uint64_t u = a + (b += carry_in);
  *carry_out = (b < carry_in) + (u < a);
  return u;
#endif
}

/** Subtract with carry. */
FIO_IFUNC uint64_t fio_math_subc64(uint64_t a,
                                   uint64_t b,
                                   uint64_t carry_in,
                                   uint64_t *carry_out) {
  FIO_ASSERT_DEBUG(carry_out, "fio_math_subc64 requires a carry pointer");
#if __has_builtin(__builtin_subcll)
  uint64_t u = __builtin_subcll(a, b, carry_in, carry_out);
#elif defined(__SIZEOF_INT128__)
  __uint128_t u = (__uint128_t)a - b - carry_in;
  if (carry_out)
    *carry_out = (uint64_t)(u >> 127U);
#else
  uint64_t u = a - b;
  a = u > a;
  b = u < carry_in;
  u -= carry_in;
  if (carry_out)
    *carry_out = a + b;
#endif
  return (uint64_t)u;
}

/** Multiply with carry out. */
FIO_IFUNC uint64_t fio_math_mulc64(uint64_t a,
                                   uint64_t b,
                                   uint64_t *carry_out) {
#if defined(__SIZEOF_INT128__)
  __uint128_t r = (__uint128_t)a * b;
  *carry_out = (uint64_t)(r >> 64U);
#elif 1 /* At this point long multiplication makes sense... */
  uint64_t r, midc = 0, lowc = 0;
  const uint64_t al = a & 0xFFFFFFFF;
  const uint64_t ah = a >> 32;
  const uint64_t bl = b & 0xFFFFFFFF;
  const uint64_t bh = b >> 32;
  const uint64_t lo = al * bl;
  const uint64_t hi = ah * bh;
  const uint64_t mid = fio_math_addc64(al * bh, ah * bl, 0, &midc);
  r = fio_math_addc64(lo, (mid << 32), 0, &lowc);
  *carry_out = hi + (mid >> 32) + (midc << 32) + lowc;
#elif 1 /* Using Karatsuba Multiplication will degrade performance */
  uint64_t r, c;
  const uint64_t al = a & 0xFFFFFFFF;
  const uint64_t ah = a >> 32;
  const uint64_t bl = b & 0xFFFFFFFF;
  const uint64_t bh = b >> 32;
  const uint64_t asum = al + ah;
  const uint64_t bsum = bl + bh;
  const uint64_t lo = al * bl;
  const uint64_t hi = ah * bh;
  /* asum * bsum might overflow, but we know each value is <= 0x100000000 */
  uint64_t midlo = (asum & 0xFFFFFFFF) * (bsum & 0xFFFFFFFF);
  uint64_t midhi = (asum & bsum) >> 32;
  uint64_t midmid = (bsum & (((uint64_t)0ULL - (asum >> 32)) >> 32)) +
                    (asum & (((uint64_t)0ULL - (bsum >> 32)) >> 32));
  midlo = fio_math_addc64(midlo, (midmid << 32), 0, &c);
  midhi += c + (midmid >> 32);
  midlo = fio_math_subc64(midlo, lo, 0, &c);
  midhi -= c;
  midlo = fio_math_subc64(midlo, hi, 0, &c);
  midhi -= c;
  r = fio_math_addc64(lo, midlo << 32, 0, &c);
  *carry_out = c + hi + (midlo >> 32) + (midhi << 32);
#else   /* never use binary for MUL... so slow... */
  uint64_t r, c = 0;
  r = a & ((uint64_t)0ULL - (b & 1));
  for (uint_fast8_t i = 1; i < 64; ++i) {
    uint64_t mask = ((uint64_t)0ULL - ((b >> i) & 1));
    uint64_t tmp = a & mask;
    uint64_t al = (tmp << i);
    uint64_t ah = (tmp >> (64 - i));
    r = fio_math_addc64(r, al, 0, &tmp);
    c += ah + tmp;
  }
  *carry_out = c;
#endif
  return (uint64_t)r;
}

/* *****************************************************************************
Multi-precision, little endian helpers. Works with full uint64_t arrays.
***************************************************************************** */

/** Multi-precision ADD for `bits` long a + b. Returns the carry. */
FIO_IFUNC uint64_t fio_math_add(uint64_t *dest,
                                const uint64_t *a,
                                const uint64_t *b,
                                const size_t len) {
  uint64_t c = 0;
  for (size_t i = 0; i < len; ++i) {
    dest[i] = fio_math_addc64(a[i], b[i], c, &c);
  }
  return c;
}

/** Multi-precision SUB for `bits` long a + b. Returns the carry. */
FIO_IFUNC uint64_t fio_math_sub(uint64_t *dest,
                                const uint64_t *a,
                                const uint64_t *b,
                                const size_t len) {
  uint64_t c = 0;
  for (size_t i = 0; i < len; ++i) {
    dest[i] = fio_math_subc64(a[i], b[i], c, &c);
  }
  return c;
}

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
  // FIO_LOG_DEBUG("Shift Light of %zu bytes and %zu bits", len - offset, bits);
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

/** Multi-precision - returns the index for the most significant bit. */
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

/** Multi-precision - returns the index for the least significant bit. */
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

/** Multi-precision MUL for `bits` long a + b. `dest` must be `len * 2`. */
FIO_IFUNC void fio_math_mul(uint64_t *restrict dest,
                            const uint64_t *a,
                            const uint64_t *b,
                            const size_t len) {
  if (len == 1) { /* route to the correct function */
    dest[0] = fio_math_mulc64(a[0], b[0], dest + 1);
    return;
  } else if (len == 2) { /* long MUL is faster */
    uint64_t tmp[2], c;
    dest[0] = fio_math_mulc64(a[0], b[0], dest + 1);
    tmp[0] = fio_math_mulc64(a[0], b[1], dest + 2);
    dest[1] = fio_math_addc64(dest[1], tmp[0], 0, &c);
    dest[2] += c;

    tmp[0] = fio_math_mulc64(a[1], b[0], tmp + 1);
    dest[1] = fio_math_addc64(dest[1], tmp[0], 0, &c);
    dest[2] = fio_math_addc64(dest[2], tmp[1], c, &c);
    dest[3] = c;
    tmp[0] = fio_math_mulc64(a[1], b[1], tmp + 1);
    dest[2] = fio_math_addc64(dest[2], tmp[0], 0, &c);
    dest[3] += tmp[1] + c;
    return;
  } else if (len == 3) { /* long MUL is still faster */
    uint64_t tmp[2], c;
    dest[0] = fio_math_mulc64(a[0], b[0], dest + 1);
    tmp[0] = fio_math_mulc64(a[0], b[1], dest + 2);
    dest[1] = fio_math_addc64(dest[1], tmp[0], 0, &c);
    dest[2] += c;
    tmp[0] = fio_math_mulc64(a[0], b[2], dest + 3);
    dest[2] = fio_math_addc64(dest[2], tmp[0], 0, &c);
    dest[3] += c;

    tmp[0] = fio_math_mulc64(a[1], b[0], tmp + 1);
    dest[1] = fio_math_addc64(dest[1], tmp[0], 0, &c);
    dest[2] = fio_math_addc64(dest[2], tmp[1], c, &c);
    dest[3] += c;
    tmp[0] = fio_math_mulc64(a[1], b[1], tmp + 1);
    dest[2] = fio_math_addc64(dest[2], tmp[0], 0, &c);
    dest[3] = fio_math_addc64(dest[3], tmp[1], c, &c);
    dest[4] = c;
    tmp[0] = fio_math_mulc64(a[1], b[2], tmp + 1);
    dest[3] = fio_math_addc64(dest[3], tmp[0], 0, &c);
    dest[4] = fio_math_addc64(dest[4], tmp[1], c, &c);
    dest[5] = c;

    tmp[0] = fio_math_mulc64(a[2], b[0], tmp + 1);
    dest[2] = fio_math_addc64(dest[2], tmp[0], 0, &c);
    dest[3] = fio_math_addc64(dest[3], tmp[1], c, &c);
    dest[4] = fio_math_addc64(dest[4], c, 0, &c);
    dest[5] += c;
    tmp[0] = fio_math_mulc64(a[2], b[1], tmp + 1);
    dest[3] = fio_math_addc64(dest[3], tmp[0], 0, &c);
    dest[4] = fio_math_addc64(dest[4], tmp[1], c, &c);
    dest[5] += c;
    tmp[0] = fio_math_mulc64(a[2], b[2], tmp + 1);
    dest[4] = fio_math_addc64(dest[4], tmp[0], 0, &c);
    dest[5] += tmp[1] + c;
  } else { /* long MUL is just too long to write */
    uint64_t c = 0;
#if !defined(__cplusplus) || __cplusplus > 201402L
    uint64_t abwmul[len * 2];
#else
    uint64_t abwmul[512];
    FIO_ASSERT(
        len <= 256,
        "Multi Precision MUL (fio_math_mul) overflows at 16384 bit numbers");
#endif
    for (size_t i = 0; i < len; ++i) { // clang-format off
     dest[(i << 1)]     = abwmul[(i << 1)]     = fio_math_mulc64(a[i], b[i], &c);
     dest[(i << 1) + 1] = abwmul[(i << 1) + 1] = c;
    } // clang-format on
    c = 0;
    for (size_t i = 0; i < len - 1; ++i) {
      dest[(i + 1) << 1] += c;
      for (size_t j = i + 1; j < len; ++j) {
        /* calculate the "middle" word sum */
        uint64_t mid0, mid1, mid2, ac, bc;
        uint64_t asum = fio_math_addc64(a[i], a[j], 0, &ac);
        uint64_t bsum = fio_math_addc64(b[i], b[j], 0, &bc);
        mid0 = fio_math_mulc64(asum, bsum, &mid1);
        mid2 = ac & bc;
        mid1 = fio_math_addc64(mid1, (asum & ((uint64_t)0ULL - bc)), 0, &c);
        mid2 += c;
        mid1 = fio_math_addc64(mid1, (bsum & ((uint64_t)0ULL - ac)), 0, &c);
        mid2 += c;
        mid0 = fio_math_subc64(mid0, abwmul[(i << 1)], 0, &c);
        mid1 = fio_math_subc64(mid1, abwmul[(i << 1) + 1], c, &c);
        mid2 -= c;
        mid0 = fio_math_subc64(mid0, abwmul[(j << 1)], 0, &c);
        mid1 = fio_math_subc64(mid1, abwmul[(j << 1) + 1], c, &c);
        mid2 -= c;
        dest[i + j] = fio_math_addc64(dest[i + j], mid0, 0, &c);
        dest[i + j + 1] = fio_math_addc64(dest[i + j + 1], mid1, c, &c);
        c += mid2;
      }
    }
  }
}

/** Multi-precision DIV for `len*64` bit long a, b. NOT constant time. */
FIO_IFUNC void fio_math_div(uint64_t *dest,
                            uint64_t *reminder,
                            const uint64_t *a,
                            const uint64_t *b,
                            const size_t len) {
#if !defined(__cplusplus) || __cplusplus > 201402L
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
  uint64_t c;
  const size_t blen = fio_math_msb_index((uint64_t *)b, len) + 1;
  if (!blen) { /* divide by zero! */
    FIO_LOG_ERROR("divide by zero!");
    if (dest)
      FIO_MEMSET(dest, 0xFFFFFFFF, sizeof(*dest) * len);
    if (reminder)
      FIO_MEMSET(reminder, 0xFFFFFFFF, sizeof(*dest) * len);
    return;
  }
  while ((rlen = fio_math_msb_index((uint64_t *)r, len)) >= blen) {
    const size_t delta = rlen - blen;
    fio_math_shl(t, (uint64_t *)b, delta, len);
    fio_math_sub(r, (uint64_t *)r, t, len);
    q[delta >> 6] =
        fio_math_addc64(q[delta >> 6], (1ULL << (delta & 63)), 0, &c);
    for (size_t i = ((delta >> 6) + 1); i < len; ++i) {
      q[i] = fio_math_addc64(q[i], 0, c, &c);
    }
  }
  fio_math_sub(t, (uint64_t *)r, (uint64_t *)b, len);
  const uint64_t mask =
      (uint64_t)0ULL -
      ((t[len - 1] ^ (b[len - 1] ^ a[len - 1])) >> 63); /* SUB overflowed */
  const uint64_t imask = ~mask;                         /* r was >= b */
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
}

/* *****************************************************************************
Common Math operations - test
***************************************************************************** */
#if defined(FIO_TEST_CSTL)

FIO_SFUNC void FIO_NAME_TEST(stl, math)(void) {
  fprintf(stderr, "* Testing multi-precision math operations (partial).\n");

  { /* Test add/sub carry */
    uint64_t a, c;
    a = fio_math_addc64(1ULL, 1ULL, 1ULL, &c);
    FIO_ASSERT(a == 3 && c == 0,
               "fio_math_addc64(1ULL, 1ULL, 1ULL, &c) failed");
    a = fio_math_addc64(~(uint64_t)0ULL, 1ULL, 0ULL, &c);
    FIO_ASSERT(!a && c == 1,
               "fio_math_addc64(~(uint64_t)0ULL, 1ULL, 0ULL, &c) failed");
    c = 0;
    a = fio_math_addc64(~(uint64_t)0ULL, 1ULL, 1ULL, &c);
    FIO_ASSERT(a == 1 && c == 1,
               "fio_math_addc64(~(uint64_t)0ULL, 1ULL, 1ULL, &c) failed");
    c = 0;
    a = fio_math_addc64(~(uint64_t)0ULL, 0ULL, 1ULL, &c);
    FIO_ASSERT(!a && c == 1,
               "fio_math_addc64(~(uint64_t)0ULL, 0ULL, 1ULL, &c) failed");
    a = fio_math_subc64(3ULL, 1ULL, 1ULL, &c);
    FIO_ASSERT(a == 1 && c == 0, "fio_math_subc64 failed");
    a = fio_math_subc64(~(uint64_t)0ULL, 1ULL, 0ULL, &c);
    FIO_ASSERT(c == 0,
               "fio_math_subc64(~(uint64_t)0ULL, 1ULL, 0ULL, &c) failed");
    a = fio_math_subc64(0ULL, ~(uint64_t)0ULL, 1ULL, &c);
    FIO_ASSERT(!a && c == 1,
               "fio_math_subc64(0ULL, ~(uint64_t)0ULL, 1ULL, &c) failed "
               "(%llu, %llu)",
               a,
               c);
    a = fio_math_subc64(0ULL, 1ULL, 0ULL, &c);
    FIO_ASSERT(a == ~(uint64_t)0ULL && c == 1,
               "fio_math_subc64(0ULL, 1ULL, 0ULL, &c) failed");
  }

  for (size_t k = 0; k < 16; ++k) { /* Test multiplication */
    for (size_t j = 0; j < 16; ++j) {
      uint64_t a = (j << (k << 1)), b = (j << k);
      {
        for (int i = 0; i < 16; ++i) {
          uint64_t r0, r1, c0, c1;
          // FIO_LOG_DEBUG("Test MUL a = %p; b = %p", (void *)a, (void *)b);
          r0 = fio_math_mulc64(a, b, &c0); /* implementation for the system. */
          // FIO_LOG_DEBUG("Sys  Mul      MUL = %p, carry = %p",
          //               (void *)r0,
          //               (void *)c0);

          { /* long multiplication (school algorithm). */
            uint64_t midc = 0, lowc = 0;
            const uint64_t al = a & 0xFFFFFFFF;
            const uint64_t ah = a >> 32;
            const uint64_t bl = b & 0xFFFFFFFF;
            const uint64_t bh = b >> 32;
            const uint64_t lo = al * bl;
            const uint64_t hi = ah * bh;
            const uint64_t mid = fio_math_addc64(al * bh, ah * bl, 0, &midc);
            const uint64_t r = fio_math_addc64(lo, (mid << 32), 0, &lowc);
            const uint64_t c = hi + (mid >> 32) + (midc << 32) + lowc;
            // FIO_LOG_DEBUG("Long Mul      MUL = %p, carry = %p",
            //               (void *)r,
            //               (void *)c);
            r1 = r;
            c1 = c;
          }
          FIO_ASSERT((r0 == r1) && (c0 == c1), "fail");
          {
            uint64_t r2[2];
            fio_math_mul(r2, &a, &b, 1);
            // FIO_LOG_DEBUG("multi Mul     MUL = %p, carry = %p",
            //               (void *)r2[0],
            //               (void *)r2[1]);
            FIO_ASSERT((r0 == r2[0]) && (c0 == r2[1]),
                       "fail Xlen MUL with len == 1");
          }
          {
            uint64_t a2[4] = {a, 0, 0, a};
            uint64_t b2[4] = {b, 0, 0, 0};
            uint64_t r2[8];
            fio_math_mul(r2, a2, b2, 4);
            // FIO_LOG_DEBUG("multi4 Mul    MUL = %p, carry = %p",
            //               (void *)r2[3],
            //               (void *)r2[4]);
            FIO_ASSERT((r0 == r2[0]) && (c0 == r2[1]),
                       "fail Xlen MUL (1) with len == 4");
            FIO_ASSERT((r0 == r2[3]) && (c0 == r2[4]),
                       "fail Xlen MUL (2) with len == 4");
          }

          a <<= 8;
          b <<= 8;
          a += 0xFAFA;
          b += 0xAFAF;
        }
      }
    }
  }
  { /* Test division */
    uint64_t n = 0, d = 1;
    for (size_t i = 0; i < 64; ++i) {
      n = (n << 7) ^ 0xAA;
      for (size_t j = 0; j < 64; ++j) {
        d = (d << 3) ^ 0xAA;
        uint64_t q, r;
        FIO_COMPILER_GUARD;
        fio_math_div(&q, &r, &n, &d, 1);
        FIO_COMPILER_GUARD;
        FIO_ASSERT(q == (n / d),
                   "fio_math_div failed quotient for 0x%llX / 0x%llX (Q=0x%llX "
                   "R=0x%llX)",
                   (long long)n,
                   (long long)d,
                   (long long)q,
                   (long long)r);
        FIO_ASSERT(
            (q * d) + r == n,
            "fio_math_div failed remainder for 0x%llX / 0x%llX (Q=0x%llX "
            "R=0x%llX)",
            (long long)n,
            (long long)d,
            (long long)q,
            (long long)r);
      }
    }
  }
  { /* Test bit shifting */
    uint64_t a[] = {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0};
    uint64_t b[] = {0xFFFFFFFFFFFFFFFE, 0xFFFFFFFFFFFFFFFF, 1};
    uint64_t c[3];
    fio_math_shl(c, a, 1, 3);
    FIO_ASSERT(!memcmp(b, c, sizeof(c)),
               "left shift failed, %llX:%llX:%llX",
               c[0],
               c[1],
               c[2]);
    fio_math_shr(c, c, 1, 3);
    FIO_ASSERT(!memcmp(a, c, sizeof(c)),
               "right shift failed, %llX:%llX:%llX",
               c[0],
               c[1],
               c[2]);
    fio_math_shl(c, a, 128, 3);
    FIO_ASSERT(!c[0] && !c[1] && !(~c[2]),
               "left shift failed, %llX:%llX:%llX",
               c[0],
               c[1],
               c[2]);
    FIO_ASSERT(fio_math_msb_index(a, 3) == 127,
               "fio_math_msb_index(a) failed %zu",
               fio_math_msb_index(a, 3));
    FIO_ASSERT(fio_math_lsb_index(a, 3) == 0,
               "fio_math_lsb_index(a) failed %zu",
               fio_math_lsb_index(a, 3));
    FIO_ASSERT(fio_math_msb_index(b, 3) == 128,
               "fio_math_msb_index(b) failed %zu",
               fio_math_msb_index(b, 3));
    FIO_ASSERT(fio_math_lsb_index(b, 3) == 1,
               "fio_math_lsb_index(b) failed %zu",
               fio_math_lsb_index(b, 3));
  }
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Math - cleanup
***************************************************************************** */
#endif /* FIO_MATH */
#undef FIO_MATH
