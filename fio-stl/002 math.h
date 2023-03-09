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

#ifndef FIO_MIFN
#define FIO_MIFN FIO_IFUNC __attribute__((warn_unused_result))
#endif

/* *****************************************************************************
Useful math unions.
***************************************************************************** */
/** An unsigned 128bit union type. */
typedef union {
  uint64_t u64[2];
  uint32_t u32[4];
  uint16_t u16[8];
  uint8_t u8[16];
#if defined(__SIZEOF_INT128__)
  __uint128_t alignment_for_u128_[1];
#endif
} fio_u128 FIO_ALIGN(16);

/** An unsigned 256bit union type. */
typedef union {
  uint64_t u64[4];
  uint32_t u32[8];
  uint16_t u16[16];
  uint8_t u8[32];
  fio_u128 u128[2];
#if defined(__SIZEOF_INT128__)
  __uint128_t alignment_for_u128_[2];
#endif
#if defined(__SIZEOF_INT256__)
  __uint256_t alignment_for_u256_[1];
#endif
} fio_u256 FIO_ALIGN(16);

/** An unsigned 512bit union type. */
typedef union {
  uint64_t u64[8];
  uint32_t u32[16];
  uint16_t u16[32];
  uint8_t u8[64];
  fio_u128 u128[4];
  fio_u256 u256[2];
} fio_u512 FIO_ALIGN(16);

/** An unsigned 1024bit union type. */
typedef union {
  uint64_t u64[16];
  uint32_t u32[32];
  uint16_t u16[64];
  uint8_t u8[128];
  fio_u128 u128[8];
  fio_u256 u256[4];
  fio_u512 u512[2];
} fio_u1024 FIO_ALIGN(16);

/** An unsigned 2048bit union type. */
typedef union {
  uint64_t u64[32];
  uint32_t u32[64];
  uint16_t u16[128];
  uint8_t u8[256];
  fio_u128 u128[16];
  fio_u256 u256[8];
  fio_u512 u512[4];
  fio_u1024 u1024[2];
} fio_u2048 FIO_ALIGN(16);

/** An unsigned 4096bit union type. */
typedef union {
  uint64_t u64[64];
  uint32_t u32[128];
  uint16_t u16[256];
  uint8_t u8[512];
  fio_u128 u128[32];
  fio_u256 u256[16];
  fio_u512 u512[8];
  fio_u1024 u1024[4];
  fio_u2048 u2048[2];
} fio_u4096 FIO_ALIGN(16);

FIO_ASSERT_STATIC(sizeof(fio_u4096) == 512, "Math type size error!");

#define fio_u128_init8(...)  ((fio_u128){.u8 = {__VA_ARGS__}})
#define fio_u128_init16(...) ((fio_u128){.u16 = {__VA_ARGS__}})
#define fio_u128_init32(...) ((fio_u128){.u32 = {__VA_ARGS__}})
#define fio_u128_init64(...) ((fio_u128){.u164 = {__VA_ARGS__}})
#define fio_u256_init8(...)  ((fio_u256){.u8 = {__VA_ARGS__}})
#define fio_u256_init16(...) ((fio_u256){.u16 = {__VA_ARGS__}})
#define fio_u256_init32(...) ((fio_u256){.u32 = {__VA_ARGS__}})
#define fio_u256_init64(...) ((fio_u256){.u164 = {__VA_ARGS__}})
#define fio_u512_init8(...)  ((fio_u512){.u8 = {__VA_ARGS__}})
#define fio_u512_init16(...) ((fio_u512){.u16 = {__VA_ARGS__}})
#define fio_u512_init32(...) ((fio_u512){.u32 = {__VA_ARGS__}})
#define fio_u512_init64(...) ((fio_u512){.u164 = {__VA_ARGS__}})

#define fio_u1024_init8(...)  ((fio_u1024){.u8 = {__VA_ARGS__}})
#define fio_u1024_init16(...) ((fio_u1024){.u16 = {__VA_ARGS__}})
#define fio_u1024_init32(...) ((fio_u1024){.u32 = {__VA_ARGS__}})
#define fio_u1024_init64(...) ((fio_u1024){.u164 = {__VA_ARGS__}})
#define fio_u2048_init8(...)  ((fio_u2048){.u8 = {__VA_ARGS__}})
#define fio_u2048_init16(...) ((fio_u2048){.u16 = {__VA_ARGS__}})
#define fio_u2048_init32(...) ((fio_u2048){.u32 = {__VA_ARGS__}})
#define fio_u2048_init64(...) ((fio_u2048){.u164 = {__VA_ARGS__}})
#define fio_u4096_init8(...)  ((fio_u4096){.u8 = {__VA_ARGS__}})
#define fio_u4096_init16(...) ((fio_u4096){.u16 = {__VA_ARGS__}})
#define fio_u4096_init32(...) ((fio_u4096){.u32 = {__VA_ARGS__}})
#define fio_u4096_init64(...) ((fio_u4096){.u164 = {__VA_ARGS__}})

/* *****************************************************************************
64bit addition (ADD) / subtraction (SUB) / multiplication (MUL) with carry.
***************************************************************************** */

/** Add with carry. */
FIO_MIFN uint64_t fio_math_addc64(uint64_t a,
                                  uint64_t b,
                                  uint64_t carry_in,
                                  uint64_t *carry_out);

/** Subtract with carry. */
FIO_MIFN uint64_t fio_math_subc64(uint64_t a,
                                  uint64_t b,
                                  uint64_t carry_in,
                                  uint64_t *carry_out);

/** Multiply with carry out. */
FIO_MIFN uint64_t fio_math_mulc64(uint64_t a, uint64_t b, uint64_t *carry_out);

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
Byte Shuffle & Reduction (on native types, up to 2048 bits == 256 bytes)
***************************************************************************** */
#define FIO____SHFL_FN(T, prefx, len)                                          \
  FIO_IFUNC void fio_##prefx##x##len##_reshuffle(T *v, uint8_t indx[len]) {    \
    T tmp[len];                                                                \
    for (size_t i = 0; i < len; ++i) {                                         \
      tmp[i] = v[indx[i] & (len - 1)];                                         \
    }                                                                          \
    for (size_t i = 0; i < len; ++i) {                                         \
      v[i] = tmp[i];                                                           \
    }                                                                          \
  }

#define FIO____REDUCE_FN(T, prefx, len, opnm, op)                              \
  FIO_IFUNC T fio_##prefx##x##len##_reduce_##opnm(T *v) {                      \
    T r = v[0];                                                                \
    for (size_t i = 1; i < len; ++i) {                                         \
      r = r op v[i];                                                           \
    }                                                                          \
    return r;                                                                  \
  }
#define FIO____REDUCE_MINMAX(T, prefx, len)                                    \
  FIO_IFUNC T fio_##prefx##x##len##_reduce_max(T *v) {                         \
    T r = v[0];                                                                \
    for (size_t i = 1; i < len; ++i) {                                         \
      r = r < v[i] ? v[i] : r;                                                 \
    }                                                                          \
    return r;                                                                  \
  }                                                                            \
  FIO_IFUNC T fio_##prefx##x##len##_reduce_min(T *v) {                         \
    T r = v[0];                                                                \
    for (size_t i = 1; i < len; ++i) {                                         \
      r = r > v[i] ? v[i] : r;                                                 \
    }                                                                          \
    return r;                                                                  \
  }

#define FIO____SHFL_REDUCE(T, prefx, len)                                      \
  FIO____SHFL_FN(T, prefx, len)                                                \
  FIO____REDUCE_FN(T, prefx, len, add, +)                                      \
  FIO____REDUCE_FN(T, prefx, len, mul, *)                                      \
  FIO____REDUCE_FN(T, prefx, len, and, &)                                      \
  FIO____REDUCE_FN(T, prefx, len, or, |)                                       \
  FIO____REDUCE_FN(T, prefx, len, xor, ^)                                      \
  FIO____REDUCE_MINMAX(T, prefx, len)

FIO____SHFL_REDUCE(uint8_t, u8, 4)
FIO____SHFL_REDUCE(uint8_t, u8, 8)
FIO____SHFL_REDUCE(uint8_t, u8, 16)
FIO____SHFL_REDUCE(uint8_t, u8, 32)
FIO____SHFL_REDUCE(uint8_t, u8, 64)
FIO____SHFL_REDUCE(uint8_t, u8, 128)
FIO____SHFL_REDUCE(uint8_t, u8, 256)
FIO____SHFL_REDUCE(uint16_t, u16, 2)
FIO____SHFL_REDUCE(uint16_t, u16, 4)
FIO____SHFL_REDUCE(uint16_t, u16, 8)
FIO____SHFL_REDUCE(uint16_t, u16, 16)
FIO____SHFL_REDUCE(uint16_t, u16, 32)
FIO____SHFL_REDUCE(uint16_t, u16, 64)
FIO____SHFL_REDUCE(uint16_t, u16, 128)
FIO____SHFL_REDUCE(uint32_t, u32, 2)
FIO____SHFL_REDUCE(uint32_t, u32, 4)
FIO____SHFL_REDUCE(uint32_t, u32, 8)
FIO____SHFL_REDUCE(uint32_t, u32, 16)
FIO____SHFL_REDUCE(uint32_t, u32, 32)
FIO____SHFL_REDUCE(uint32_t, u32, 64)
FIO____SHFL_REDUCE(uint64_t, u64, 2)
FIO____SHFL_REDUCE(uint64_t, u64, 4)
FIO____SHFL_REDUCE(uint64_t, u64, 8)
FIO____SHFL_REDUCE(uint64_t, u64, 16)
FIO____SHFL_REDUCE(uint64_t, u64, 32)

#undef FIO____SHFL_REDUCE
#define FIO____SHFL_REDUCE(T, prefx, len)                                      \
  FIO____SHFL_FN(T, prefx, len)                                                \
  FIO____REDUCE_FN(T, prefx, len, add, +)                                      \
  FIO____REDUCE_FN(T, prefx, len, mul, *)                                      \
  FIO____REDUCE_MINMAX(T, prefx, len)

FIO____SHFL_REDUCE(float, float, 2)
FIO____SHFL_REDUCE(float, float, 4)
FIO____SHFL_REDUCE(float, float, 8)
FIO____SHFL_REDUCE(float, float, 16)
FIO____SHFL_REDUCE(float, float, 32)
FIO____SHFL_REDUCE(float, float, 64)
FIO____SHFL_REDUCE(double, dbl, 2)
FIO____SHFL_REDUCE(double, dbl, 4)
FIO____SHFL_REDUCE(double, dbl, 8)
FIO____SHFL_REDUCE(double, dbl, 16)
FIO____SHFL_REDUCE(double, dbl, 32)
#undef FIO____SHFL_REDUCE
#undef FIO____REDUCE_FN
#undef FIO____SHFL_FN

/* clang-format off */
#define fio_u8x4_reshuffle(v, ...)     fio_u8x4_reshuffle(v,     (uint8_t[4]){__VA_ARGS__})
#define fio_u8x8_reshuffle(v, ...)     fio_u8x8_reshuffle(v,     (uint8_t[8]){__VA_ARGS__})
#define fio_u8x16_reshuffle(v, ...)    fio_u8x16_reshuffle(v,    (uint8_t[16]){__VA_ARGS__})
#define fio_u8x32_reshuffle(v, ...)    fio_u8x32_reshuffle(v,    (uint8_t[32]){__VA_ARGS__})
#define fio_u8x64_reshuffle(v, ...)    fio_u8x64_reshuffle(v,    (uint8_t[64]){__VA_ARGS__})
#define fio_u8x128_reshuffle(v, ...)   fio_u8x128_reshuffle(v,   (uint8_t[128]){__VA_ARGS__})
#define fio_u8x256_reshuffle(v, ...)   fio_u8x256_reshuffle(v,   (uint8_t[256]){__VA_ARGS__})
#define fio_u16x2_reshuffle(v, ...)    fio_u16x2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
#define fio_u16x4_reshuffle(v, ...)    fio_u16x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
#define fio_u16x8_reshuffle(v, ...)    fio_u16x8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
#define fio_u16x16_reshuffle(v, ...)   fio_u16x16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
#define fio_u16x32_reshuffle(v, ...)   fio_u16x32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
#define fio_u16x64_reshuffle(v, ...)   fio_u16x64_reshuffle(v,   (uint8_t[64]){__VA_ARGS__})
#define fio_u16x128_reshuffle(v,...)   fio_u16x128_reshuffle(v,  (uint8_t[128]){__VA_ARGS__})
#define fio_u32x2_reshuffle(v, ...)    fio_u32x2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
#define fio_u32x4_reshuffle(v, ...)    fio_u32x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
#define fio_u32x8_reshuffle(v, ...)    fio_u32x8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
#define fio_u32x16_reshuffle(v, ...)   fio_u32x16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
#define fio_u32x32_reshuffle(v, ...)   fio_u32x32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
#define fio_u32x64_reshuffle(v, ...)   fio_u32x64_reshuffle(v,   (uint8_t[64]){__VA_ARGS__})
#define fio_u64x2_reshuffle(v, ...)    fio_u64x2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
#define fio_u64x4_reshuffle(v, ...)    fio_u64x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
#define fio_u64x8_reshuffle(v, ...)    fio_u64x8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
#define fio_u64x16_reshuffle(v, ...)   fio_u64x16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
#define fio_u64x32_reshuffle(v, ...)   fio_u64x32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
#define fio_floatx2_reshuffle(v, ...)  fio_floatx2_reshuffle(v,  (uint8_t[2]){__VA_ARGS__})
#define fio_floatx4_reshuffle(v, ...)  fio_floatx4_reshuffle(v,  (uint8_t[4]){__VA_ARGS__})
#define fio_floatx8_reshuffle(v, ...)  fio_floatx8_reshuffle(v,  (uint8_t[8]){__VA_ARGS__})
#define fio_floatx16_reshuffle(v, ...) fio_floatx16_reshuffle(v, (uint8_t[16]){__VA_ARGS__})
#define fio_floatx32_reshuffle(v, ...) fio_floatx32_reshuffle(v, (uint8_t[32]){__VA_ARGS__})
#define fio_floatx64_reshuffle(v, ...) fio_floatx64_reshuffle(v, (uint8_t[64]){__VA_ARGS__})
#define fio_dblx2_reshuffle(v, ...)    fio_dblx2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
#define fio_dblx4_reshuffle(v, ...)    fio_dblx4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
#define fio_dblx8_reshuffle(v, ...)    fio_dblx8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
#define fio_dblx16_reshuffle(v, ...)   fio_dblx16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
#define fio_dblx32_reshuffle(v, ...)   fio_dblx32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
/* clang-format on */

/* *****************************************************************************
Vector Helpers - memory load operations (implementation starts here)
***************************************************************************** */

#define FIO_MATH_TYPE_LOADER(bits, bytes)                                      \
  /** Loads from memory using local-endian. */                                 \
  FIO_MIFN fio_u##bits fio_u##bits##_load(const void *buf) {                   \
    fio_u##bits r;                                                             \
    fio_memcpy##bytes(&r, buf);                                                \
    return r;                                                                  \
  }                                                                            \
  /** Stores to memory using local-endian. */                                  \
  FIO_IFUNC void fio_u##bits##_store(void *buf, const fio_u##bits a) {         \
    fio_memcpy##bytes(buf, &a);                                                \
  }

#define FIO_VECTOR_LOADER_ENDIAN_FUNC(total_bits, bits)                        \
  /** Loads vector from memory, reading from little-endian.  */                \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_load_le##bits(                \
      const void *buf) {                                                       \
    fio_u##total_bits r = fio_u##total_bits##_load(buf);                       \
    for (size_t i = 0; i < (total_bits / bits); ++i) {                         \
      r.u##bits[i] = fio_ltole##bits(r.u##bits[i]);                            \
    }                                                                          \
    return r;                                                                  \
  }                                                                            \
  /** Loads vector from memory, reading from big-endian.  */                   \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_load_be##bits(                \
      const void *buf) {                                                       \
    fio_u##total_bits r = fio_u##total_bits##_load(buf);                       \
    for (size_t i = 0; i < (total_bits / bits); ++i) {                         \
      r.u##bits[i] = fio_lton##bits(r.u##bits[i]);                             \
    }                                                                          \
    return r;                                                                  \
  }                                                                            \
  FIO_MIFN fio_u##total_bits fio_u##total_bits##_bswap##bits(                  \
      fio_u##total_bits a) {                                                   \
    fio_u##total_bits r;                                                       \
    for (size_t i = 0; i < (total_bits / bits); ++i)                           \
      r.u##bits[i] = fio_bswap##bits(a.u##bits[i]);                            \
    return r;                                                                  \
  }

FIO_MATH_TYPE_LOADER(128, 16)
FIO_MATH_TYPE_LOADER(256, 32)
FIO_MATH_TYPE_LOADER(512, 64)
FIO_MATH_TYPE_LOADER(1024, 128)
FIO_MATH_TYPE_LOADER(2048, 256)
FIO_MATH_TYPE_LOADER(4096, 512)

#define FIO_VECTOR_LOADER_ENDIAN(total_bits)                                   \
  FIO_VECTOR_LOADER_ENDIAN_FUNC(total_bits, 16)                                \
  FIO_VECTOR_LOADER_ENDIAN_FUNC(total_bits, 32)                                \
  FIO_VECTOR_LOADER_ENDIAN_FUNC(total_bits, 64)

FIO_VECTOR_LOADER_ENDIAN(128)
FIO_VECTOR_LOADER_ENDIAN(256)
FIO_VECTOR_LOADER_ENDIAN(512)
FIO_VECTOR_LOADER_ENDIAN(1024)
FIO_VECTOR_LOADER_ENDIAN(2048)
FIO_VECTOR_LOADER_ENDIAN(4096)

#undef FIO_MATH_TYPE_LOADER
#undef FIO_VECTOR_LOADER_ENDIAN_FUNC
#undef FIO_VECTOR_LOADER_ENDIAN
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
64bit addition (ADD) / subtraction (SUB) / multiplication (MUL) with carry.
***************************************************************************** */

/** Add with carry. */
FIO_IFUNC uint64_t fio_math_addc64(uint64_t a,
                                   uint64_t b,
                                   uint64_t carry_in,
                                   uint64_t *carry_out) {
  FIO_ASSERT_DEBUG(carry_out, "fio_math_addc64 requires a carry pointer");
#if __has_builtin(__builtin_addcll) && UINT64_MAX == LLONG_MAX
  return __builtin_addcll(a, b, carry_in, (unsigned long long *)carry_out);
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
#if __has_builtin(__builtin_subcll) && UINT64_MAX == LLONG_MAX
  uint64_t u =
      __builtin_subcll(a, b, carry_in, (unsigned long long *)carry_out);
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
