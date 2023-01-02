/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___   /* Development inclusion - ignore line */
#define FIO_BITWISE    /* Development inclusion - ignore line */
#define FIO_BITMAP     /* Development inclusion - ignore line */
#include "./include.h" /* Development inclusion - ignore line */
#endif                 /* Development inclusion - ignore line */
/* *****************************************************************************
More joyful ideas at:      https://graphics.stanford.edu/~seander/bithacks.html




                            Bit-Byte Operations



Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */

#if defined(FIO_BITWISE) && !defined(H___BITWISE___H)
#define H___BITWISE___H

#ifndef FIO_BITWISE_USE_MEMCPY
#define FIO_BITWISE_USE_MEMCPY 1
#endif

/* *****************************************************************************
Bit rotation
***************************************************************************** */

/** Left rotation for an unknown size element, inlined. */
#define FIO_LROT(i, bits)                                                      \
  (((i) << ((bits) & ((sizeof((i)) << 3) - 1))) |                              \
   ((i) >> ((-(bits)) & ((sizeof((i)) << 3) - 1))))

/** Right rotation for an unknown size element, inlined. */
#define FIO_RROT(i, bits)                                                      \
  (((i) >> ((bits) & ((sizeof((i)) << 3) - 1))) |                              \
   ((i) << ((-(bits)) & ((sizeof((i)) << 3) - 1))))

#if __has_builtin(__builtin_rotateleft8)
/** 8Bit left rotation, inlined. */
#define fio_lrot8(i, bits) __builtin_rotateleft8(i, bits)
#else
/** 8Bit left rotation, inlined. */
FIO_IFUNC uint8_t fio_lrot8(uint8_t i, uint8_t bits) {
  return ((i << (bits & 7UL)) | (i >> ((-(bits)) & 7UL)));
}
#endif

#if __has_builtin(__builtin_rotateleft16)
/** 16Bit left rotation, inlined. */
#define fio_lrot16(i, bits) __builtin_rotateleft16(i, bits)
#else
/** 16Bit left rotation, inlined. */
FIO_IFUNC uint16_t fio_lrot16(uint16_t i, uint8_t bits) {
  return ((i << (bits & 15UL)) | (i >> ((-(bits)) & 15UL)));
}
#endif

#if __has_builtin(__builtin_rotateleft32)
/** 32Bit left rotation, inlined. */
#define fio_lrot32(i, bits) __builtin_rotateleft32(i, bits)
#else
/** 32Bit left rotation, inlined. */
FIO_IFUNC uint32_t fio_lrot32(uint32_t i, uint8_t bits) {
  return ((i << (bits & 31UL)) | (i >> ((-(bits)) & 31UL)));
}
#endif

#if __has_builtin(__builtin_rotateleft64)
/** 64Bit left rotation, inlined. */
#define fio_lrot64(i, bits) __builtin_rotateleft64(i, bits)
#else
/** 64Bit left rotation, inlined. */
FIO_IFUNC uint64_t fio_lrot64(uint64_t i, uint8_t bits) {
  return ((i << ((bits)&63UL)) | (i >> ((-(bits)) & 63UL)));
}
#endif

#if __has_builtin(__builtin_rotatrightt8)
/** 8Bit right rotation, inlined. */
#define fio_rrot8(i, bits) __builtin_rotateright8(i, bits)
#else
/** 8Bit right rotation, inlined. */
FIO_IFUNC uint8_t fio_rrot8(uint8_t i, uint8_t bits) {
  return ((i >> (bits & 7UL)) | (i << ((-(bits)) & 7UL)));
}
#endif

#if __has_builtin(__builtin_rotateright16)
/** 16Bit right rotation, inlined. */
#define fio_rrot16(i, bits) __builtin_rotateright16(i, bits)
#else
/** 16Bit right rotation, inlined. */
FIO_IFUNC uint16_t fio_rrot16(uint16_t i, uint8_t bits) {
  return ((i >> (bits & 15UL)) | (i << ((-(bits)) & 15UL)));
}
#endif

#if __has_builtin(__builtin_rotateright32)
/** 32Bit right rotation, inlined. */
#define fio_rrot32(i, bits) __builtin_rotateright32(i, bits)
#else
/** 32Bit right rotation, inlined. */
FIO_IFUNC uint32_t fio_rrot32(uint32_t i, uint8_t bits) {
  return ((i >> (bits & 31UL)) | (i << ((-(bits)) & 31UL)));
}
#endif

#if __has_builtin(__builtin_rotateright64)
/** 64Bit right rotation, inlined. */
#define fio_rrot64(i, bits) __builtin_rotateright64(i, bits)
#else
/** 64Bit right rotation, inlined. */
FIO_IFUNC uint64_t fio_rrot64(uint64_t i, uint8_t bits) {
  return ((i >> ((bits)&63UL)) | (i << ((-(bits)) & 63UL)));
}
#endif

#ifdef __SIZEOF_INT128__
#if __has_builtin(__builtin_rotateright128) &&                                 \
    __has_builtin(__builtin_rotateleft128)
/** 128Bit left rotation, inlined. */
#define fio_lrot128(i, bits) __builtin_rotateleft128(i, bits)
/** 128Bit right rotation, inlined. */
#define fio_rrot128(i, bits) __builtin_rotateright128(i, bits)
#else
/** 128Bit left rotation, inlined. */
FIO_IFUNC __uint128_t fio_lrot128(__uint128_t i, uint8_t bits) {
  return ((i << ((bits)&127UL)) | (i >> ((-(bits)) & 127UL)));
}
/** 128Bit right rotation, inlined. */
FIO_IFUNC __uint128_t fio_rrot128(__uint128_t i, uint8_t bits) {
  return ((i >> ((bits)&127UL)) | (i << ((-(bits)) & 127UL)));
}
#endif
#endif /* __SIZEOF_INT128__ */

/* *****************************************************************************
Unaligned memory read / write operations
***************************************************************************** */

/** Converts an unaligned byte stream to a 16 bit number (local byte order). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16_local)(const void *c) {
  uint16_t tmp; /* fio_buf2u16 */
  fio_memcpy2(&tmp, c);
  return tmp;
}
/** Converts an unaligned byte stream to a 32 bit number (local byte order). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32_local)(const void *c) {
  uint32_t tmp; /* fio_buf2u32 */
  fio_memcpy4(&tmp, c);
  return tmp;
}
/** Converts an unaligned byte stream to a 64 bit number (local byte order). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64_local)(const void *c) {
  uint64_t tmp; /* fio_buf2u64 */
  fio_memcpy8(&tmp, c);
  return tmp;
}

/** Writes a local 16 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16_local)(void *buf, uint16_t i) {
  fio_memcpy2(buf, &i); /* fio_u2buf16 */
}
/** Writes a local 32 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32_local)(void *buf, uint32_t i) {
  fio_memcpy4(buf, &i); /* fio_u2buf32 */
}
/** Writes a local 64 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64_local)(void *buf, uint64_t i) {
  fio_memcpy8(buf, &i); /* fio_u2buf64 */
}

#ifdef __SIZEOF_INT128__
/** Converts an unaligned byte stream to a 128 bit number (local byte order). */
FIO_IFUNC __uint128_t FIO_NAME2(fio_buf, u128_local)(const void *c) {
  __uint128_t tmp; /* fio_buf2u1128 */
  fio_memcpy16(&tmp, c);
  return tmp;
}

/** Writes a local 128 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf128_local)(void *buf, __uint128_t i) {
  fio_memcpy16(buf, &i); /* fio_u2buf128 */
}
#endif /* __SIZEOF_INT128__ */

/** Converts an unaligned byte stream to a 16 bit number (reversed order). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16_bswap)(const void *c) {
  return fio_bswap16(FIO_NAME2(fio_buf, u16_local)(c)); /* fio_buf2u16 */
}
/** Converts an unaligned byte stream to a 32 bit number (reversed order). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32_bswap)(const void *c) {
  return fio_bswap32(FIO_NAME2(fio_buf, u32_local)(c)); /* fio_buf2u32 */
}
/** Converts an unaligned byte stream to a 64 bit number (reversed order). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64_bswap)(const void *c) {
  return fio_bswap64(FIO_NAME2(fio_buf, u64_local)(c)); /* fio_buf2u64 */
}

/** Writes a local 16 bit number to an unaligned buffer in reversed order. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16_bswap)(void *buf, uint16_t i) {
  FIO_NAME2(fio_u, buf16_local)(buf, fio_bswap16(i));
}
/** Writes a local 32 bit number to an unaligned buffer in reversed order. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32_bswap)(void *buf, uint32_t i) {
  FIO_NAME2(fio_u, buf32_local)(buf, fio_bswap32(i));
}
/** Writes a local 64 bit number to an unaligned buffer in reversed order. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64_bswap)(void *buf, uint64_t i) {
  FIO_NAME2(fio_u, buf64_local)(buf, fio_bswap64(i));
}

#ifdef __SIZEOF_INT128__
/** Writes a local 64 bit number to an unaligned buffer in reversed order. */
FIO_IFUNC void FIO_NAME2(fio_u, buf128_bswap)(void *buf, __uint128_t i) {
  FIO_NAME2(fio_u, buf128_local)(buf, fio_bswap128(i));
}
#endif /* __SIZEOF_INT128__ */

/** Converts an unaligned byte stream to a 16 bit number (Big Endian). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16)(const void *c) { /* fio_buf2u16 */
  uint16_t i = FIO_NAME2(fio_buf, u16_local)(c);
  return fio_lton16(i);
}
/** Converts an unaligned byte stream to a 32 bit number (Big Endian). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32)(const void *c) { /* fio_buf2u32 */
  uint32_t i = FIO_NAME2(fio_buf, u32_local)(c);
  return fio_lton32(i);
}
/** Converts an unaligned byte stream to a 64 bit number (Big Endian). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64)(const void *c) { /* fio_buf2u64 */
  uint64_t i = FIO_NAME2(fio_buf, u64_local)(c);
  return fio_lton64(i);
}

/** Writes a local 16 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16)(void *buf, uint16_t i) {
  FIO_NAME2(fio_u, buf16_local)(buf, fio_ntol16(i));
}
/** Writes a local 32 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32)(void *buf, uint32_t i) {
  FIO_NAME2(fio_u, buf32_local)(buf, fio_ntol32(i));
}
/** Writes a local 64 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64)(void *buf, uint64_t i) {
  FIO_NAME2(fio_u, buf64_local)(buf, fio_ntol64(i));
}

#ifdef __SIZEOF_INT128__
/** Converts an unaligned byte stream to a 128 bit number (Big Endian). */
FIO_IFUNC __uint128_t FIO_NAME2(fio_buf,
                                u128)(const void *c) { /* fio_buf2u64 */
  __uint128_t i = FIO_NAME2(fio_buf, u128_local)(c);
  return fio_lton128(i);
}
/** Writes a local 128 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf128)(void *buf, __uint128_t i) {
  FIO_NAME2(fio_u, buf128_local)(buf, fio_ntol128(i));
}
#endif /* __SIZEOF_INT128__ */

#if __LITTLE_ENDIAN__

/** Converts an unaligned byte stream to a 16 bit number (Little Endian). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16_little)(const void *c) {
  return FIO_NAME2(fio_buf, u16_local)(c); /* fio_buf2u16 */
}
/** Converts an unaligned byte stream to a 32 bit number (Little Endian). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32_little)(const void *c) {
  return FIO_NAME2(fio_buf, u32_local)(c); /* fio_buf2u32 */
}
/** Converts an unaligned byte stream to a 64 bit number (Little Endian). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64_little)(const void *c) {
  return FIO_NAME2(fio_buf, u64_local)(c); /* fio_buf2u64 */
}

/** Writes a local 16 bit number to an unaligned buffer in Little Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16_little)(void *buf, uint16_t i) {
  FIO_NAME2(fio_u, buf16_local)(buf, i);
}
/** Writes a local 32 bit number to an unaligned buffer in Little Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32_little)(void *buf, uint32_t i) {
  FIO_NAME2(fio_u, buf32_local)(buf, i);
}
/** Writes a local 64 bit number to an unaligned buffer in Little Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64_little)(void *buf, uint64_t i) {
  FIO_NAME2(fio_u, buf64_local)(buf, i);
}

#ifdef __SIZEOF_INT128__
/** Converts an unaligned byte stream to a 128 bit number (Little Endian). */
FIO_IFUNC __uint128_t FIO_NAME2(fio_buf, u128_little)(const void *c) {
  return FIO_NAME2(fio_buf, u128_local)(c); /* fio_buf2u64 */
}
/** Writes a local 128 bit number to an unaligned buffer in Little Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf128_little)(void *buf, __uint128_t i) {
  FIO_NAME2(fio_u, buf128_local)(buf, i);
}
#endif /* __SIZEOF_INT128__ */

#else /* !__LITTLE_ENDIAN__ */

/** Converts an unaligned byte stream to a 16 bit number (Little Endian). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16_little)(const void *c) {
  return FIO_NAME2(fio_buf, u16_bswap)(c); /* fio_buf2u16 */
}
/** Converts an unaligned byte stream to a 32 bit number (Little Endian). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32_little)(const void *c) {
  return FIO_NAME2(fio_buf, u32_bswap)(c); /* fio_buf2u32 */
}
/** Converts an unaligned byte stream to a 64 bit number (Little Endian). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64_little)(const void *c) {
  return FIO_NAME2(fio_buf, u64_bswap)(c); /* fio_buf2u64 */
}

/** Writes a local 16 bit number to an unaligned buffer in Little Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16_little)(void *buf, uint16_t i) {
  FIO_NAME2(fio_u, buf16_bswap)(buf, i);
}
/** Writes a local 32 bit number to an unaligned buffer in Little Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32_little)(void *buf, uint32_t i) {
  FIO_NAME2(fio_u, buf32_bswap)(buf, i);
}
/** Writes a local 64 bit number to an unaligned buffer in Little Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64_little)(void *buf, uint64_t i) {
  FIO_NAME2(fio_u, buf64_bswap)(buf, i);
}

#ifdef __SIZEOF_INT128__
/** Converts an unaligned byte stream to a 128 bit number (Little Endian). */
FIO_IFUNC __uint128_t FIO_NAME2(fio_buf, u128_little)(const void *c) {
  return FIO_NAME2(fio_buf, u128_bswap)(c); /* fio_buf2u64 */
}
/** Writes a local 128 bit number to an unaligned buffer in Little Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf128_little)(void *buf, __uint128_t i) {
  FIO_NAME2(fio_u, buf128_bswap)(buf, i);
}
#endif /* __SIZEOF_INT128__ */

#endif /* __LITTLE_ENDIAN__ */

/** Convenience function for reading 1 byte (8 bit) from a buffer. */
FIO_IFUNC uint8_t FIO_NAME2(fio_buf, u8_local)(const void *c) {
  const uint8_t *tmp = (const uint8_t *)c; /* fio_buf2u16 */
  return *tmp;
}

/** Convinience function for writing 1 byte (8 bit) to a buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf8_local)(void *buf, uint8_t i) {
  *((uint8_t *)buf) = i; /* fio_u2buf16 */
}

/** Convinience function for reading 1 byte (8 bit) from a buffer. */
FIO_IFUNC uint8_t FIO_NAME2(fio_buf, u8_bswap)(const void *c) {
  const uint8_t *tmp = (const uint8_t *)c; /* fio_buf2u16 */
  return *tmp;
}

/** Convinience function for writing 1 byte (8 bit) to a buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf8_bswap)(void *buf, uint8_t i) {
  *((uint8_t *)buf) = i; /* fio_u2buf16 */
}

/** Convinience function for reading 1 byte (8 bit) from a buffer. */
FIO_IFUNC uint8_t FIO_NAME2(fio_buf, u8_little)(const void *c) {
  const uint8_t *tmp = (const uint8_t *)c; /* fio_buf2u16 */
  return *tmp;
}

/** Convinience function for writing 1 byte (8 bit) to a buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf8_little)(void *buf, uint8_t i) {
  *((uint8_t *)buf) = i; /* fio_u2buf16 */
}

/** Convinience function for reading 1 byte (8 bit) from a buffer. */
FIO_IFUNC uint8_t FIO_NAME2(fio_buf, u8)(const void *c) {
  const uint8_t *tmp = (const uint8_t *)c; /* fio_buf2u16 */
  return *tmp;
}

/** Convinience function for writing 1 byte (8 bit) to a buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf8)(void *buf, uint8_t i) {
  *((uint8_t *)buf) = i; /* fio_u2buf16 */
}

/* *****************************************************************************
Constant-Time Selectors
***************************************************************************** */

/** Returns 1 if the expression is true (input isn't zero). */
FIO_IFUNC uintptr_t fio_ct_true(uintptr_t cond) {
  // promise that the highest bit is set if any bits are set, than shift.
  return ((cond | (0 - cond)) >> ((sizeof(cond) << 3) - 1));
}

/** Returns 1 if the expression is false (input is zero). */
FIO_IFUNC uintptr_t fio_ct_false(uintptr_t cond) {
  // fio_ct_true returns only one bit, XOR will inverse that bit.
  return fio_ct_true(cond) ^ 1;
}

/** Returns `a` if `cond` is boolean and true, returns b otherwise. */
FIO_IFUNC uintptr_t fio_ct_if_bool(uint8_t cond, uintptr_t a, uintptr_t b) {
  // b^(a^b) cancels b out. 0-1 => sets all bits.
  return (b ^ ((0 - (cond & 1)) & (a ^ b)));
}

/** Returns `a` if `cond` isn't zero (uses fio_ct_true), returns b otherwise. */
FIO_IFUNC uintptr_t fio_ct_if(uintptr_t cond, uintptr_t a, uintptr_t b) {
  // b^(a^b) cancels b out. 0-1 => sets all bits.
  return fio_ct_if_bool(fio_ct_true(cond), a, b);
}

/** Returns `a` if a >= `b`. */
FIO_IFUNC intptr_t fio_ct_max(intptr_t a_, intptr_t b_) {
  // if b - a is negative, a > b, unless both / one are negative.
  const uintptr_t a = a_, b = b_;
  return (
      intptr_t)fio_ct_if_bool(((a - b) >> ((sizeof(a) << 3) - 1)) & 1, b, a);
}

/* *****************************************************************************
SIMD emulation helpers
***************************************************************************** */

/**
 * Detects a byte where all the bits are set (255) within a 4 byte vector.
 *
 * The full byte will be be set to 0x80, all other bytes will be 0x0.
 */
FIO_IFUNC uint32_t fio_has_full_byte32(uint32_t row) {
  return ((row & UINT32_C(0x7F7F7F7F)) + UINT32_C(0x01010101)) &
         (row & UINT32_C(0x80808080));
}

/**
 * Detects a byte where no bits are set (0) within a 4 byte vector.
 *
 * The zero byte will be be set to 0x80, all other bytes will be 0x0.
 */
FIO_IFUNC uint32_t fio_has_zero_byte32(uint32_t row) {
  return (row - UINT32_C(0x01010101)) & (~row & UINT32_C(0x80808080));
}

/**
 * Detects if `byte` exists within a 4 byte vector.
 *
 * The requested byte will be be set to 0x80, all other bytes will be 0x0.
 */
FIO_IFUNC uint32_t fio_has_byte32(uint32_t row, uint8_t byte) {
  return fio_has_zero_byte32((row ^ (UINT32_C(0x01010101) * byte)));
}

/**
 * Detects a byte where all the bits are set (255) within an 8 byte vector.
 *
 * The full byte will be be set to 0x80, all other bytes will be 0x0.
 */
FIO_IFUNC uint64_t fio_has_full_byte64(uint64_t row) {
  return ((row & UINT64_C(0x7F7F7F7F7F7F7F7F)) + UINT64_C(0x0101010101010101)) &
         (row & UINT64_C(0x8080808080808080));
}

/**
 * Detects a byte where no bits are set (0) within an 8 byte vector.
 *
 * The zero byte will be be set to 0x80, all other bytes will be 0x0.
 */
FIO_IFUNC uint64_t fio_has_zero_byte64(uint64_t row) {
  return (row - UINT64_C(0x0101010101010101)) &
         ((~row) & UINT64_C(0x8080808080808080));
}

/**
 * Detects if `byte` exists within an 8 byte vector.
 *
 * The requested byte will be be set to 0x80, all other bytes will be 0x0.
 */
FIO_IFUNC uint64_t fio_has_byte64(uint64_t row, uint8_t byte) {
  return fio_has_zero_byte64((row ^ (UINT64_C(0x0101010101010101) * byte)));
}

#ifdef __SIZEOF_INT128__
/**
 * Detects a byte where all the bits are set (255) within an 16 byte vector.
 *
 * The full byte will be be set to 0x80, all other bytes will be 0x0.
 */
FIO_IFUNC __uint128_t fio_has_full_byte128(__uint128_t row) {
  const __uint128_t allF7 = ((__uint128_t)(0x7F7F7F7F7F7F7F7FULL) << 64) |
                            (__uint128_t)(0x7F7F7F7F7F7F7F7FULL);
  const __uint128_t all80 = ((__uint128_t)(0x8080808080808080) << 64) |
                            (__uint128_t)(0x8080808080808080);
  const __uint128_t all01 = ((__uint128_t)(0x0101010101010101) << 64) |
                            (__uint128_t)(0x0101010101010101);
  return ((row & allF7) + all01) & (row & all80);
}

/**
 * Detects a byte where no bits are set (0) within an 8 byte vector.
 *
 * The zero byte will be be set to 0x80, all other bytes will be 0x0.
 */
FIO_IFUNC __uint128_t fio_has_zero_byte128(__uint128_t row) {
  const __uint128_t all80 = ((__uint128_t)(0x8080808080808080) << 64) |
                            (__uint128_t)(0x8080808080808080);
  const __uint128_t all01 = ((__uint128_t)(0x0101010101010101) << 64) |
                            (__uint128_t)(0x0101010101010101);
  return ((row - all01) & ((~row) & all80));
}

/**
 * Detects if `byte` exists within an 8 byte vector.
 *
 * The requested byte will be be set to 0x80, all other bytes will be 0x0.
 */
FIO_IFUNC __uint128_t fio_has_byte128(__uint128_t row, uint8_t byte) {
  const __uint128_t all01 = ((__uint128_t)(0x0101010101010101) << 64) |
                            (__uint128_t)(0x0101010101010101);
  return fio_has_zero_byte128((row ^ (all01 * byte)));
}
#endif /* __SIZEOF_INT128__ */

/** Converts a `fio_has_byteX` result to a bitmap. */
FIO_IFUNC uint64_t fio_has_byte2bitmap(uint64_t result) {
  result >>= 7;             /* move result to first bit of each byte */
  result |= (result >> 7);  /* combine 2 bytes of result */
  result |= (result >> 14); /* combine 4 bytes of result */
  result |= (result >> 28); /* combine 8 bytes of result */
  return result & 0xFFULL;
}

/** Isolates the least significant (lowest) bit. */
FIO_IFUNC uint64_t fio_bits_lsb(uint64_t i) { return (i & ((~i) + 1)); }

/** Isolates the most significant (highest) bit. */
FIO_IFUNC uint64_t fio_bits_msb(uint64_t i) {
  i |= i >> 1;
  i |= i >> 2;
  i |= i >> 4;
  i |= i >> 8;
  i |= i >> 16;
  i |= i >> 32;
  i = ((i + 1) >> 1) | (i & ((uint64_t)1ULL << 63));
  return i;
}

/** Returns the index of the most significant (highest) bit. */
FIO_IFUNC size_t fio_bits_msb_index(uint64_t i) {
  if (!i)
    goto zero;
  return fio___msb_index_unsafe(i);
zero:
  return (size_t)-1;
}

/** Returns the index of the least significant (lowest) bit. */
FIO_IFUNC size_t fio_bits_lsb_index(uint64_t i) {
  if (!i)
    goto zero;
  return fio___lsb_index_unsafe(i);
zero:
  return (size_t)-1;
}

/* *****************************************************************************
Byte masking (XOR) with nonce (counter mode)
***************************************************************************** */

/**
 * Masks data using a 64 bit mask and a counter mode nonce. When the buffer's
 * memory is aligned, the function may perform significantly better.
 *
 * Returns the end state of the mask.
 */
FIO_IFUNC uint64_t fio_xmask2(char *buf_,
                              size_t len,
                              uint64_t mask,
                              uint64_t nonce) {
  register char *buf = (char *)buf_;
  uint64_t tmp;
  for (size_t i = 7; i < len; i += 8) {
    fio_memcpy8(&tmp, buf);
    tmp ^= mask;
    fio_memcpy8(buf, &tmp);
    buf += 8;
    mask += nonce;
  }
  fio_memcpy7x(&tmp, buf, len);
  tmp ^= mask;
  fio_memcpy7x(buf, &tmp, len);
  mask += nonce;
  return mask;
}

/* *****************************************************************************
Byte masking (XOR) - no nonce
***************************************************************************** */

/**
 * Masks data using a persistent 64 bit mask.
 *
 * When the buffer's memory is aligned, the function may perform significantly
 * better.
 */
FIO_IFUNC void fio_xmask(char *buf_, size_t len, uint64_t mask) {
  register char *buf = (char *)buf_;
  uint64_t m[4] FIO_ALIGN(16) = {mask, mask, mask, mask};
  uint64_t tmp[4] FIO_ALIGN(16);
  for (size_t i = 31; i < len; i += 32) {
    fio_memcpy32(tmp, buf);
    tmp[0] ^= m[0];
    tmp[1] ^= m[1];
    tmp[2] ^= m[2];
    tmp[3] ^= m[3];
    fio_memcpy32(buf, tmp);
    buf += 32;
  }
  fio_memcpy31x(tmp, buf, len);
  tmp[0] ^= m[0];
  tmp[1] ^= m[1];
  tmp[2] ^= m[2];
  tmp[3] ^= m[3];
  fio_memcpy31x(buf, tmp, len);
}

/* *****************************************************************************
Hemming Distance and bit counting
***************************************************************************** */

#if __has_builtin(__builtin_popcountll)
/** performs a `popcount` operation to count the set bits. */
#define fio_popcount(n) __builtin_popcountll(n)
#else
FIO_IFUNC int fio_popcount(uint64_t n) {
  /* for logic, see Wikipedia: https://en.wikipedia.org/wiki/Hamming_weight */
  n = n - ((n >> 1) & 0x5555555555555555);
  n = (n & 0x3333333333333333) + ((n >> 2) & 0x3333333333333333);
  n = (n + (n >> 4)) & 0x0f0f0f0f0f0f0f0f;
  n = n + (n >> 8);
  n = n + (n >> 16);
  n = n + (n >> 32);
  return n & 0x7f;
}
#endif

#define fio_hemming_dist(n1, n2) fio_popcount(((uint64_t)(n1) ^ (uint64_t)(n2)))

/* *****************************************************************************
Bitewise helpers cleanup
***************************************************************************** */
#endif /* FIO_BITWISE */
#undef FIO_BITWISE

/* *****************************************************************************




                                Bitmap Helpers




***************************************************************************** */
#if defined(FIO_BITMAP) && !defined(H___FIO_BITMAP_H)
#define H___FIO_BITMAP_H
/* *****************************************************************************
Bitmap access / manipulation
***************************************************************************** */

/** Gets the state of a bit in a bitmap. */
FIO_IFUNC uint8_t fio_bitmap_get(void *map, size_t bit) {
  return ((((uint8_t *)(map))[(bit) >> 3] >> ((bit)&7)) & 1);
}

/** Sets the a bit in a bitmap (sets to 1). */
FIO_IFUNC void fio_bitmap_set(void *map, size_t bit) {
  fio_atomic_or((uint8_t *)(map) + ((bit) >> 3), (1UL << ((bit)&7)));
}

/** Unsets the a bit in a bitmap (sets to 0). */
FIO_IFUNC void fio_bitmap_unset(void *map, size_t bit) {
  fio_atomic_and((uint8_t *)(map) + ((bit) >> 3),
                 (uint8_t)(~(1UL << ((bit)&7))));
}

/** Flips the a bit in a bitmap (sets to 0 if 1, sets to 1 if 0). */
FIO_IFUNC void fio_bitmap_flip(void *map, size_t bit) {
  fio_atomic_xor((uint8_t *)(map) + ((bit) >> 3), (1UL << ((bit)&7)));
}

/* *****************************************************************************
Bit-Byte operations - testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL

/* used in the test, defined later */
SFUNC uint64_t fio_rand64(void);
SFUNC void fio_rand_bytes(void *target, size_t len);

FIO_SFUNC void FIO_NAME_TEST(stl, bitwise)(void) {
  fprintf(stderr, "* Testing fio_memcpy primitives.\n");
  {
    struct {
      void *(*fn)(void *, const void *, size_t);
      const char *name;
      size_t len;
    } tests[] = {
        {fio_memcpy7x, "fio_memcpy7x", 7},
        {fio_memcpy15x, "fio_memcpy15x", 15},
        {fio_memcpy31x, "fio_memcpy31x", 31},
        {fio_memcpy63x, "fio_memcpy63x", 63},
        {fio_memcpy127x, "fio_memcpy127x", 127},
        {fio_memcpy255x, "fio_memcpy255x", 255},
        {fio_memcpy511x, "fio_memcpy511x", 511},
        {fio_memcpy1023x, "fio_memcpy1023x", 1023},
        {fio_memcpy2047x, "fio_memcpy2047x", 2047},
        {fio_memcpy4095x, "fio_memcpy4095x", 4095},
        {NULL},
    };
    char buf[(4096 << 1) + 64];
    fio_rand_bytes(buf + (4096 + 32), (4096 + 32));
    for (size_t ifn = 0; tests[ifn].fn; ++ifn) {
      /* test all x primitives */
      size_t len = tests[ifn].len;
      for (size_t i = 0; i < 31; ++i) {
        memset(buf, 0, 4096 + 32);
        buf[i + len] = '\xFF';
        tests[ifn].fn(buf + i, buf + (4096 + 32), len);
        FIO_ASSERT(!memcmp(buf + i, buf + (4096 + 32), len),
                   "%s failed @ %zu\n",
                   tests[ifn].name,
                   i);
        FIO_ASSERT(buf[i + len] == '\xFF', "%s overflow?", tests[ifn].name);
      }
    }
  }
  fprintf(stderr, "* Testing fio_bswapX macros.\n");
  FIO_ASSERT(fio_bswap16(0x0102) == (uint16_t)0x0201, "fio_bswap16 failed");
  FIO_ASSERT(fio_bswap32(0x01020304) == (uint32_t)0x04030201,
             "fio_bswap32 failed");
  FIO_ASSERT(fio_bswap64(0x0102030405060708ULL) == 0x0807060504030201ULL,
             "fio_bswap64 failed");

  fprintf(stderr, "* Testing fio_lrotX and fio_rrotX macros.\n");
  {
    uint64_t tmp = 1;
    tmp = FIO_RROT(tmp, 1);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << ((sizeof(uint64_t) << 3) - 1)),
               "fio_rrot failed");
    tmp = FIO_LROT(tmp, 3);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot failed");
    tmp = 1;
    tmp = fio_rrot32(tmp, 1);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 31), "fio_rrot32 failed");
    tmp = fio_lrot32(tmp, 3);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot32 failed");
    tmp = 1;
    tmp = fio_rrot64(tmp, 1);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 63), "fio_rrot64 failed");
    tmp = fio_lrot64(tmp, 3);
    FIO_COMPILER_GUARD;
    FIO_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot64 failed");
  }
  for (size_t i = 0; i < 63; ++i) {
#if !defined(__has_builtin) || !__has_builtin(__builtin_ctzll) ||              \
    !__has_builtin(__builtin_clzll)
    FIO_ASSERT(fio___single_bit_index_unsafe((1ULL << i)) == i,
               "bit index map[%zu] error != %zu",
               (size_t)(1ULL << i),
               i);
#endif
    FIO_ASSERT(fio_bits_msb_index(((1ULL << i) | 1)) == i,
               "fio_bits_msb_index(%zu) != %zu",
               ((1ULL << i)),
               (size_t)fio_bits_msb_index(((1ULL << i) | 1)));
    FIO_ASSERT(fio_bits_lsb_index(((~0ULL) << i)) == i,
               "fio_bits_lsb_index(%zu) != %zu",
               1,
               (size_t)fio_bits_lsb_index(((~0ULL) << i)));
  }

  fprintf(stderr, "* Testing fio_buf2uX and fio_u2bufX helpers.\n");
#define FIO___BITMAP_TEST_BITS(itype, utype, bits)                             \
  for (size_t i = 0; i < (bits); ++i) {                                        \
    char tmp_buf[32];                                                          \
    itype n = ((utype)1 << i);                                                 \
    FIO_NAME2(fio_u, buf##bits##_local)(tmp_buf, n);                           \
    itype r = FIO_NAME2(fio_buf, u##bits##_local)(tmp_buf);                    \
    FIO_ASSERT(r == n,                                                         \
               "roundtrip failed for U" #bits " at bit %zu\n\t%zu != %zu",     \
               i,                                                              \
               (size_t)n,                                                      \
               (size_t)r);                                                     \
    FIO_ASSERT(!memcmp(tmp_buf, &n, (bits) >> 3),                              \
               "memory ordering implementation error for U" #bits "!");        \
  }
  FIO___BITMAP_TEST_BITS(int8_t, uint8_t, 8);
  FIO___BITMAP_TEST_BITS(int16_t, uint16_t, 16);
  FIO___BITMAP_TEST_BITS(int32_t, uint32_t, 32);
  FIO___BITMAP_TEST_BITS(int64_t, uint64_t, 64);
#ifdef __SIZEOF_INT128__
  FIO___BITMAP_TEST_BITS(__int128_t, __uint128_t, 128);
#endif
#undef FIO___BITMAP_TEST_BITS

  fprintf(stderr, "* Testing constant-time helpers.\n");
  FIO_ASSERT(fio_ct_true(0) == 0, "fio_ct_true(0) should be zero!");
  for (uintptr_t i = 1; i; i <<= 1) {
    FIO_ASSERT(fio_ct_true(i) == 1,
               "fio_ct_true(%p) should be true!",
               (void *)i);
  }
  for (uintptr_t i = 1; i + 1 != 0; i = (i << 1) | 1) {
    FIO_ASSERT(fio_ct_true(i) == 1,
               "fio_ct_true(%p) should be true!",
               (void *)i);
  }
  FIO_ASSERT(fio_ct_true(((uintptr_t)~0ULL)) == 1,
             "fio_ct_true(%p) should be true!",
             (void *)(uintptr_t)(~0ULL));

  FIO_ASSERT(fio_ct_false(0) == 1, "fio_ct_false(0) should be true!");
  for (uintptr_t i = 1; i; i <<= 1) {
    FIO_ASSERT(fio_ct_false(i) == 0,
               "fio_ct_false(%p) should be zero!",
               (void *)i);
  }
  for (uintptr_t i = 1; i + 1 != 0; i = (i << 1) | 1) {
    FIO_ASSERT(fio_ct_false(i) == 0,
               "fio_ct_false(%p) should be zero!",
               (void *)i);
  }
  FIO_ASSERT(fio_ct_false(((uintptr_t)~0ULL)) == 0,
             "fio_ct_false(%p) should be zero!",
             (void *)(uintptr_t)(~0ULL));
  FIO_ASSERT(fio_ct_true(8), "fio_ct_true should be true.");
  FIO_ASSERT(!fio_ct_true(0), "fio_ct_true should be false.");
  FIO_ASSERT(!fio_ct_false(8), "fio_ct_false should be false.");
  FIO_ASSERT(fio_ct_false(0), "fio_ct_false should be true.");
  FIO_ASSERT(fio_ct_if_bool(0, 1, 2) == 2,
             "fio_ct_if_bool selection error (false).");
  FIO_ASSERT(fio_ct_if_bool(1, 1, 2) == 1,
             "fio_ct_if_bool selection error (true).");
  FIO_ASSERT(fio_ct_if(0, 1, 2) == 2, "fio_ct_if selection error (false).");
  FIO_ASSERT(fio_ct_if(8, 1, 2) == 1, "fio_ct_if selection error (true).");
  FIO_ASSERT(fio_ct_max(1, 2) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(2, 1) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-1, 2) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(2, -1) == 2, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(1, -2) == 1, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-2, 1) == 1, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-1, -2) == -1, "fio_ct_max error.");
  FIO_ASSERT(fio_ct_max(-2, -1) == -1, "fio_ct_max error.");
  {
    uint8_t bitmap[1024];
    FIO_MEMSET(bitmap, 0, 1024);
    fprintf(stderr, "* Testing bitmap helpers.\n");
    FIO_ASSERT(!fio_bitmap_get(bitmap, 97), "fio_bitmap_get should be 0.");
    fio_bitmap_set(bitmap, 97);
    FIO_ASSERT(fio_bitmap_get(bitmap, 97) == 1,
               "fio_bitmap_get should be 1 after being set");
    FIO_ASSERT(!fio_bitmap_get(bitmap, 96),
               "other bits shouldn't be effected by set.");
    FIO_ASSERT(!fio_bitmap_get(bitmap, 98),
               "other bits shouldn't be effected by set.");
    fio_bitmap_flip(bitmap, 96);
    fio_bitmap_flip(bitmap, 97);
    FIO_ASSERT(!fio_bitmap_get(bitmap, 97),
               "fio_bitmap_get should be 0 after flip.");
    FIO_ASSERT(fio_bitmap_get(bitmap, 96) == 1,
               "other bits shouldn't be effected by flip");
    fio_bitmap_unset(bitmap, 96);
    fio_bitmap_flip(bitmap, 97);
    FIO_ASSERT(!fio_bitmap_get(bitmap, 96),
               "fio_bitmap_get should be 0 after unset.");
    FIO_ASSERT(fio_bitmap_get(bitmap, 97) == 1,
               "other bits shouldn't be effected by unset");
    fio_bitmap_unset(bitmap, 96);
  }
  {
    fprintf(stderr, "* Testing popcount and hemming distance calculation.\n");
    for (int i = 0; i < 64; ++i) {
      FIO_ASSERT(fio_popcount((uint64_t)1 << i) == 1,
                 "fio_popcount error for 1 bit");
    }
    for (int i = 0; i < 63; ++i) {
      FIO_ASSERT(fio_popcount((uint64_t)3 << i) == 2,
                 "fio_popcount error for 2 bits");
    }
    for (int i = 0; i < 62; ++i) {
      FIO_ASSERT(fio_popcount((uint64_t)7 << i) == 3,
                 "fio_popcount error for 3 bits");
    }
    for (int i = 0; i < 59; ++i) {
      FIO_ASSERT(fio_popcount((uint64_t)21 << i) == 3,
                 "fio_popcount error for 3 alternating bits");
    }
    for (int i = 0; i < 64; ++i) {
      FIO_ASSERT(fio_hemming_dist(((uint64_t)1 << i) - 1, 0) == i,
                 "fio_hemming_dist error at %d",
                 i);
    }
  }
  {
    struct test_s {
      int a;
      char force_padding;
      int b;
    } stst = {.a = 1};

    struct test_s *stst_p = FIO_PTR_FROM_FIELD(struct test_s, b, &stst.b);
    FIO_ASSERT(stst_p == &stst, "FIO_PTR_FROM_FIELD failed to retrace pointer");
  }
  {
    fprintf(stderr, "* Testing fio_xmask and fio_xmask2.\n");
    char data[128], buf[256];
    uint64_t mask;
    uint64_t counter;
    do {
      mask = fio_rand64();
      counter = fio_rand64();
    } while (!mask || !counter);
    fio_rand_bytes(data, 128);
    const size_t len = 127;
    for (uint8_t i = 0; i < 16; ++i) {
      FIO_MEMCPY(buf + i, data, len);
      buf[len + i] = '\xFF';
      fio_xmask(buf + i, len, mask);
      FIO_ASSERT(buf[len + i] == '\xFF', "fio_xmask overflow?");
      FIO_ASSERT(memcmp(buf + i, data, len), "fio_xmask masking error");
      FIO_ASSERT(memcmp(buf + i, data, 8), "fio_xmask didn't mask data head?");
      FIO_ASSERT(
          !(len & 7) ||
              memcmp(buf + i + (len & (~7U)), data + (len & (~7U)), (len & 7)),
          "fio_xmask mask didn't mask data's tail?");
      fio_xmask(buf + i, len, mask);
      FIO_ASSERT(!memcmp(buf + i, data, len), "fio_xmask rountrip error");
      fio_xmask(buf + i, len, mask);
      memmove(buf + i + 1, buf + i, len);
      fio_xmask(buf + i + 1, len, mask);
      FIO_ASSERT(!memcmp(buf + i + 1, data, len),
                 "fio_xmask rountrip (with move) error");
    }
    for (uint8_t i = 0; i < 16; ++i) {
      FIO_MEMCPY(buf + i, data, len);
      buf[len + i] = '\xFF';
      fio_xmask2(buf + i, len, mask, counter);
      FIO_ASSERT(buf[len + i] == '\xFF', "fio_xmask2 overflow?");
      FIO_ASSERT(memcmp(buf + i, data, len), "fio_xmask2 (CM) masking error");
      FIO_ASSERT(memcmp(buf + i, data, 8), "fio_xmask2 didn't mask data head?");
      FIO_ASSERT(memcmp(buf + i + (len - 8), data + (len - 8), 8),
                 "fio_xmask2 mask didn't mask string tail?");
      fio_xmask2(buf + i, len, mask, counter);
      FIO_ASSERT(!memcmp(buf + i, data, len), "fio_xmask2 rountrip error");
      fio_xmask2(buf + i, len, mask, counter);
      memmove(buf + i + 1, buf + i, len);
      fio_xmask2(buf + i + 1, len, mask, counter);
      FIO_ASSERT(!memcmp(buf + i + 1, data, len), "fio_xmask2 with move error");
    }
  }
}
#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Bit-Byte operations - cleanup
***************************************************************************** */
#endif /* FIO_BITMAP */
#undef FIO_BITMAP
