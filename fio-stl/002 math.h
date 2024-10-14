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
FIO_MIFN size_t fio_math_msb_index(uint64_t *n, const size_t len);

/** Multi-precision - returns the index for the least significant bit or -1. */
FIO_MIFN size_t fio_math_lsb_index(uint64_t *n, const size_t len);

/* *****************************************************************************
Vector Helpers - Simple Math functions
***************************************************************************** */

#define FIO_VECTOR_OPERATION_CONST(total_bits, bits, opt_name, opt)            \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_c##opt_name##bits(            \
      fio_u##total_bits a,                                                     \
      const uint##bits##_t b) {                                                \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / bits); ++i) {                         \
      r.u##bits[i] = a.u##bits[i] opt b;                                       \
    }                                                                          \
    return r;                                                                  \
  }

#define FIO_VECTOR_OPERATION(total_bits, bits, opt_name, opt)                  \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_##opt_name##bits(             \
      fio_u##total_bits a,                                                     \
      const fio_u##total_bits b) {                                             \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / bits); ++i) {                         \
      r.u##bits[i] = a.u##bits[i] opt b.u##bits[i];                            \
    }                                                                          \
    return r;                                                                  \
  }                                                                            \
  FIO_VECTOR_OPERATION_CONST(total_bits, bits, opt_name, opt)

#define FIO_VECTOR_OPERATION_BIG(total_bits, opt_name, opt)                    \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_##opt_name(                   \
      fio_u##total_bits a,                                                     \
      const fio_u##total_bits b) {                                             \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / 64); ++i) {                           \
      r.u64[i] = a.u64[i] opt b.u64[i];                                        \
    }                                                                          \
    return r;                                                                  \
  }

#define FIO_VECTOR_OPERATION_SINGLE(total_bits, opt_name, opt)                 \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_##opt_name(                   \
      fio_u##total_bits a) {                                                   \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / 64); ++i) {                           \
      r.u64[i] = opt a.u64[i];                                                 \
    }                                                                          \
    return r;                                                                  \
  }

#define FIO_VECTOR_OPERATION_ROT_SHFT(total_bits, bits, dir, opt, opt_inv)     \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_c##dir##rot##bits(            \
      fio_u##total_bits a,                                                     \
      size_t bits_) {                                                          \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / bits); ++i) {                         \
      r.u##bits[i] = (a.u##bits[i] opt bits_) |                                \
                     (a.u##bits[i] opt_inv((bits - bits_) & (bits - 1)));      \
    }                                                                          \
    return r;                                                                  \
  }                                                                            \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_c##dir##shift##bits(          \
      fio_u##total_bits a,                                                     \
      size_t bits_) {                                                          \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / bits); ++i) {                         \
      r.u##bits[i] = (a.u##bits[i] opt bits_);                                 \
    }                                                                          \
    return r;                                                                  \
  }                                                                            \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_##dir##rot##bits(             \
      fio_u##total_bits a,                                                     \
      fio_u##total_bits b) {                                                   \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / bits); ++i) {                         \
      r.u##bits[i] =                                                           \
          (a.u##bits[i] opt b.u##bits[i]) |                                    \
          (a.u##bits[i] opt_inv((bits - b.u##bits[i]) & (bits - 1)));          \
    }                                                                          \
    return r;                                                                  \
  }                                                                            \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_##dir##shift##bits(           \
      fio_u##total_bits a,                                                     \
      fio_u##total_bits b) {                                                   \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / bits); ++i) {                         \
      r.u##bits[i] = (a.u##bits[i] opt b.u##bits[i]);                          \
    }                                                                          \
    return r;                                                                  \
  }

/* *****************************************************************************
Vector Helpers - Shuffle
***************************************************************************** */
#define FIO_VECTOR_SHUFFLE_FN(total_bits, bits)                                \
  /** Performs a "shuffle" operation, returning a new, reordered vector. */    \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_shuffle##bits(                \
      fio_u##total_bits v,                                                     \
      const char vi[total_bits / bits]) {                                      \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / bits); ++i) {                         \
      r.u##bits[i] = v.u##bits[vi[i] & ((total_bits / bits) - 1)];             \
    }                                                                          \
    return r;                                                                  \
  }

/* *****************************************************************************
Vector Helpers - Reduce
***************************************************************************** */

#define FIO_VECTOR_OPERATION_REDUCE(total_bits, bits, opt_name, opt)           \
  FIO_MIFN uint##bits##_t fio_u##total_bits##_reduce_##opt_name##bits(         \
      fio_u##total_bits a) {                                                   \
    uint##bits##_t r = a.u##bits[0];                                           \
    for (size_t i = 1; i < (total_bits / bits); ++i) {                         \
      r = r opt a.u##bits[i];                                                  \
    }                                                                          \
    return r;                                                                  \
  }

#define FIO_VECTOR_OPERATION_REDUCE_FN(total_bits, bits)                       \
  FIO_VECTOR_OPERATION_REDUCE(total_bits, bits, mul, *)                        \
  FIO_VECTOR_OPERATION_REDUCE(total_bits, bits, add, +)                        \
  FIO_VECTOR_OPERATION_REDUCE(total_bits, bits, and, &)                        \
  FIO_VECTOR_OPERATION_REDUCE(total_bits, bits, or, |)                         \
  FIO_VECTOR_OPERATION_REDUCE(total_bits, bits, xor, ^)                        \
  FIO_MIFN uint##bits##_t fio_u##total_bits##_reduce_max##bits(                \
      fio_u##total_bits a) {                                                   \
    uint##bits##_t r = a.u##bits[0];                                           \
    for (size_t i = 1; i < (total_bits / bits); ++i) {                         \
      r = r < a.u##bits[i] ? a.u##bits[i] : r;                                 \
    }                                                                          \
    return r;                                                                  \
  }                                                                            \
  FIO_MIFN uint##bits##_t fio_u##total_bits##_reduce_min##bits(                \
      fio_u##total_bits a) {                                                   \
    uint##bits##_t r = a.u##bits[0];                                           \
    for (size_t i = 1; i < (total_bits / bits); ++i) {                         \
      r = r > a.u##bits[i] ? a.u##bits[i] : r;                                 \
    }                                                                          \
    return r;                                                                  \
  }

/* *****************************************************************************
Vector Helpers - Actual Functions (Macros used to write function code)
***************************************************************************** */

#define FIO_VECTOR_GROUP_FUNCTIONS_BITS(total_bits, bits)                      \
  FIO_VECTOR_OPERATION(total_bits, bits, mul, *)                               \
  FIO_VECTOR_OPERATION(total_bits, bits, add, +)                               \
  FIO_VECTOR_OPERATION(total_bits, bits, sub, -)                               \
  FIO_VECTOR_OPERATION(total_bits, bits, div, /)                               \
  FIO_VECTOR_OPERATION_CONST(total_bits, bits, and, &)                         \
  FIO_VECTOR_OPERATION_CONST(total_bits, bits, or, |)                          \
  FIO_VECTOR_OPERATION_CONST(total_bits, bits, xor, ^)                         \
  FIO_VECTOR_OPERATION_CONST(total_bits, bits, reminder, %)                    \
  FIO_VECTOR_OPERATION_ROT_SHFT(total_bits, bits, l, <<, >>)                   \
  FIO_VECTOR_OPERATION_ROT_SHFT(total_bits, bits, r, >>, <<)                   \
  FIO_VECTOR_OPERATION_REDUCE_FN(total_bits, bits)                             \
  FIO_VECTOR_SHUFFLE_FN(total_bits, bits)

#define FIO_VECTOR_GROUP_FUNCTIONS(total_bits)                                 \
  FIO_VECTOR_OPERATION_SINGLE(total_bits, flip, ~)                             \
  FIO_VECTOR_OPERATION_BIG(total_bits, and, &)                                 \
  FIO_VECTOR_OPERATION_BIG(total_bits, or, |)                                  \
  FIO_VECTOR_OPERATION_BIG(total_bits, xor, ^)                                 \
  FIO_VECTOR_GROUP_FUNCTIONS_BITS(total_bits, 8)                               \
  FIO_VECTOR_GROUP_FUNCTIONS_BITS(total_bits, 16)                              \
  FIO_VECTOR_GROUP_FUNCTIONS_BITS(total_bits, 32)                              \
  FIO_VECTOR_GROUP_FUNCTIONS_BITS(total_bits, 64)

FIO_VECTOR_GROUP_FUNCTIONS(128)
FIO_VECTOR_GROUP_FUNCTIONS(256)
FIO_VECTOR_GROUP_FUNCTIONS(512)
FIO_VECTOR_GROUP_FUNCTIONS(1024)
FIO_VECTOR_GROUP_FUNCTIONS(2048)
FIO_VECTOR_GROUP_FUNCTIONS(4096)

FIO_VECTOR_SHUFFLE_FN(256, 128)
FIO_VECTOR_SHUFFLE_FN(512, 128)
FIO_VECTOR_SHUFFLE_FN(512, 256)
FIO_VECTOR_SHUFFLE_FN(1024, 128)
FIO_VECTOR_SHUFFLE_FN(1024, 256)
FIO_VECTOR_SHUFFLE_FN(1024, 512)
FIO_VECTOR_SHUFFLE_FN(2048, 128)
FIO_VECTOR_SHUFFLE_FN(2048, 256)
FIO_VECTOR_SHUFFLE_FN(2048, 512)
FIO_VECTOR_SHUFFLE_FN(2048, 1024)
FIO_VECTOR_SHUFFLE_FN(4096, 128)
FIO_VECTOR_SHUFFLE_FN(4096, 256)
FIO_VECTOR_SHUFFLE_FN(4096, 512)
FIO_VECTOR_SHUFFLE_FN(4096, 1024)
FIO_VECTOR_SHUFFLE_FN(4096, 2048)

#undef FIO_VECTOR_GROUP_FUNCTIONS
#undef FIO_VECTOR_GROUP_FUNCTIONS_BITS
#undef FIO_VECTOR_OPERATION
#undef FIO_VECTOR_OPERATION_BIG
#undef FIO_VECTOR_OPERATION_CONST
#undef FIO_VECTOR_OPERATION_ROT_SHFT
#undef FIO_VECTOR_OPERATION_SINGLE
#undef FIO_VECTOR_SHUFFLE_FN

/* *****************************************************************************
Vector Helpers - Shuffle Macros
***************************************************************************** */
// clang-format off
#define fio_u128_shuffle8(v, ...)  fio_u128_shuffle8(v, (char[16]){__VA_ARGS__})
#define fio_u128_shuffle16(v, ...) fio_u128_shuffle16(v, (char[8]){__VA_ARGS__})
#define fio_u128_shuffle32(v, ...) fio_u128_shuffle32(v, (char[4]){__VA_ARGS__})
#define fio_u128_shuffle64(v, ...) fio_u128_shuffle64(v, (char[2]){__VA_ARGS__})

#define fio_u256_shuffle8(v, ...)  fio_u256_shuffle8(v, (char[32]){__VA_ARGS__})
#define fio_u256_shuffle16(v, ...) fio_u256_shuffle16(v, (char[16]){__VA_ARGS__})
#define fio_u256_shuffle32(v, ...) fio_u256_shuffle32(v, (char[8]){__VA_ARGS__})
#define fio_u256_shuffle64(v, ...) fio_u256_shuffle64(v, (char[4]){__VA_ARGS__})
#define fio_u256_shuffle128(v, ...) fio_u256_shuffle128(v, (char[2]){__VA_ARGS__})

#define fio_u512_shuffle8(v, ...)  fio_u512_shuffle8(v, (char[64]){__VA_ARGS__})
#define fio_u512_shuffle16(v, ...) fio_u512_shuffle16(v, (char[32]){__VA_ARGS__})
#define fio_u512_shuffle32(v, ...) fio_u512_shuffle32(v, (char[16]){__VA_ARGS__})
#define fio_u512_shuffle64(v, ...) fio_u512_shuffle64(v, (char[8]){__VA_ARGS__})
#define fio_u512_shuffle128(v, ...) fio_u512_shuffle128(v, (char[4]){__VA_ARGS__})
#define fio_u512_shuffle256(v, ...) fio_u512_shuffle256(v, (char[2]){__VA_ARGS__})

#define fio_u1024_shuffle8(v, ...)  fio_u1024_shuffle8(v, (char[128]){__VA_ARGS__})
#define fio_u1024_shuffle16(v, ...) fio_u1024_shuffle16(v, (char[64]){__VA_ARGS__})
#define fio_u1024_shuffle32(v, ...) fio_u1024_shuffle32(v, (char[32]){__VA_ARGS__})
#define fio_u1024_shuffle64(v, ...) fio_u1024_shuffle64(v, (char[16]){__VA_ARGS__})
#define fio_u1024_shuffle128(v, ...) fio_u1024_shuffle128(v, (char[8]){__VA_ARGS__})
#define fio_u1024_shuffle256(v, ...) fio_u1024_shuffle256(v, (char[4]){__VA_ARGS__})
#define fio_u1024_shuffle512(v, ...) fio_u1024_shuffle512(v, (char[2]){__VA_ARGS__})
// clang-format on

/* *****************************************************************************
Multi-precision, little endian helpers. Works with full
uint64_t arrays.
***************************************************************************** */

/** Multi-precision ADD for `bits` long a + b. Returns the
 * carry. */
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

/** Multi-precision SUB for `bits` long a + b. Returns the
 * carry. */
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

/** Multi-precision MUL for `bits` long a + b. `dest` must
 * be `len * 2`. */
FIO_IFUNC void fio_math_mul(uint64_t *restrict dest,
                            const uint64_t *a,
                            const uint64_t *b,
                            const size_t len) {
  if (!len)
    return;
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
#if !defined(_MSC_VER) && (!defined(__cplusplus) || __cplusplus > 201402L)
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
    fio_math_sub(r, (uint64_t *)r, t, len);
    q[delta >> 6] |= (1ULL << (delta & 63)); /* set the bit used */
  }
  fio_math_sub(t, (uint64_t *)r, (uint64_t *)b, len);
  mask = (uint64_t)0ULL -
         ((t[len - 1] ^ (b[len - 1] ^ a[len - 1])) >> 63); /* SUB overflowed */
  imask = ~mask;                                           /* r was >= b */
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
