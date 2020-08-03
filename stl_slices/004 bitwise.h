/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










                            Bit-Byte Operations










***************************************************************************** */

#if defined(FIO_BITWISE) && !defined(H___BITWISE___H)
#define H___BITWISE___H
/* *****************************************************************************
Swapping byte's order (`bswap` variations)
***************************************************************************** */

/** Byte swap a 16 bit integer, inlined. */
#if __has_builtin(__builtin_bswap16)
#define fio_bswap16(i) __builtin_bswap16((uint16_t)(i))
#else
FIO_IFUNC uint16_t fio_bswap16(uint16_t i) {
  return ((((i)&0xFFU) << 8) | (((i)&0xFF00U) >> 8));
}
#endif

/** Byte swap a 32 bit integer, inlined. */
#if __has_builtin(__builtin_bswap32)
#define fio_bswap32(i) __builtin_bswap32((uint32_t)(i))
#else
FIO_IFUNC uint32_t fio_bswap32(uint32_t i) {
  return ((((i)&0xFFUL) << 24) | (((i)&0xFF00UL) << 8) |
          (((i)&0xFF0000UL) >> 8) | (((i)&0xFF000000UL) >> 24));
}
#endif

/** Byte swap a 64 bit integer, inlined. */
#if __has_builtin(__builtin_bswap64)
#define fio_bswap64(i) __builtin_bswap64((uint64_t)(i))
#else
FIO_IFUNC uint64_t fio_bswap64(uint64_t i) {
  return ((((i)&0xFFULL) << 56) | (((i)&0xFF00ULL) << 40) |
          (((i)&0xFF0000ULL) << 24) | (((i)&0xFF000000ULL) << 8) |
          (((i)&0xFF00000000ULL) >> 8) | (((i)&0xFF0000000000ULL) >> 24) |
          (((i)&0xFF000000000000ULL) >> 40) |
          (((i)&0xFF00000000000000ULL) >> 56));
}
#endif

/* *****************************************************************************
Big Endian / Small Endian
***************************************************************************** */
#if (defined(__LITTLE_ENDIAN__) && __LITTLE_ENDIAN__) ||                       \
    (defined(__BIG_ENDIAN__) && !__BIG_ENDIAN__) ||                            \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__ 0
#endif
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__ 1
#endif
#elif (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) ||                           \
    (defined(__LITTLE_ENDIAN__) && !__LITTLE_ENDIAN__) ||                      \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__ 1
#endif
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__ 0
#endif
#elif !defined(__BIG_ENDIAN__) && !defined(__BYTE_ORDER__) &&                  \
    !defined(__LITTLE_ENDIAN__)
#error Could not detect byte order on this system.
#endif

#if __BIG_ENDIAN__

/** Local byte order to Network byte order, 16 bit integer */
#define fio_lton16(i) (i)
/** Local byte order to Network byte order, 32 bit integer */
#define fio_lton32(i) (i)
/** Local byte order to Network byte order, 62 bit integer */
#define fio_lton64(i) (i)

/** Network byte order to Local byte order, 16 bit integer */
#define fio_ntol16(i) (i)
/** Network byte order to Local byte order, 32 bit integer */
#define fio_ntol32(i) (i)
/** Network byte order to Local byte order, 62 bit integer */
#define fio_ntol64(i) (i)

#else /* Little Endian */

/** Local byte order to Network byte order, 16 bit integer */
#define fio_lton16(i) fio_bswap16((i))
/** Local byte order to Network byte order, 32 bit integer */
#define fio_lton32(i) fio_bswap32((i))
/** Local byte order to Network byte order, 62 bit integer */
#define fio_lton64(i) fio_bswap64((i))

/** Network byte order to Local byte order, 16 bit integer */
#define fio_ntol16(i) fio_bswap16((i))
/** Network byte order to Local byte order, 32 bit integer */
#define fio_ntol32(i) fio_bswap32((i))
/** Network byte order to Local byte order, 62 bit integer */
#define fio_ntol64(i) fio_bswap64((i))

#endif /* __BIG_ENDIAN__ */

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
/** 8Bit left rotation, inlined. */
#define fio_rrot8(i, bits) __builtin_rotateright8(i, bits)
#else
/** 8Bit left rotation, inlined. */
FIO_IFUNC uint8_t fio_rrot8(uint8_t i, uint8_t bits) {
  return ((i >> (bits & 7UL)) | (i << ((-(bits)) & 7UL)));
}
#endif

#if __has_builtin(__builtin_rotateright16)
/** 16Bit left rotation, inlined. */
#define fio_rrot16(i, bits) __builtin_rotateright16(i, bits)
#else
/** 16Bit left rotation, inlined. */
FIO_IFUNC uint16_t fio_rrot16(uint16_t i, uint8_t bits) {
  return ((i >> (bits & 15UL)) | (i << ((-(bits)) & 15UL)));
}
#endif

#if __has_builtin(__builtin_rotateright32)
/** 32Bit left rotation, inlined. */
#define fio_rrot32(i, bits) __builtin_rotateright32(i, bits)
#else
/** 32Bit left rotation, inlined. */
FIO_IFUNC uint32_t fio_rrot32(uint32_t i, uint8_t bits) {
  return ((i >> (bits & 31UL)) | (i << ((-(bits)) & 31UL)));
}
#endif

#if __has_builtin(__builtin_rotateright64)
/** 64Bit left rotation, inlined. */
#define fio_rrot64(i, bits) __builtin_rotateright64(i, bits)
#else
/** 64Bit left rotation, inlined. */
FIO_IFUNC uint64_t fio_rrot64(uint64_t i, uint8_t bits) {
  return ((i >> ((bits)&63UL)) | (i << ((-(bits)) & 63UL)));
}
#endif

/* *****************************************************************************
Unaligned memory read / write operations
***************************************************************************** */

#if __has_builtin(__builtin_memcpy)
/** Converts an unaligned byte stream to a 16 bit number (local byte order). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16_local)(const void *c) {
  uint16_t tmp; /* fio_buf2u16 */
  __builtin_memcpy(&tmp, c, sizeof(tmp));
  return tmp;
}
/** Converts an unaligned byte stream to a 32 bit number (local byte order). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32_local)(const void *c) {
  uint32_t tmp; /* fio_buf2u32 */
  __builtin_memcpy(&tmp, c, sizeof(tmp));
  return tmp;
}
/** Converts an unaligned byte stream to a 64 bit number (local byte order). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64_local)(const void *c) {
  /* fio_buf2u64 */
  uint64_t tmp;
  __builtin_memcpy(&tmp, c, sizeof(tmp));
  return tmp;
}

/** Writes a local 16 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16_local)(void *buf,
                                             uint16_t i) { /* fio_u2buf16 */
  __builtin_memcpy(buf, &i, sizeof(i));
}
/** Writes a local 32 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32_local)(void *buf,
                                             uint32_t i) { /* fio_u2buf32 */
  __builtin_memcpy(buf, &i, sizeof(i));
}
/** Writes a local 64 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64_local)(void *buf,
                                             uint64_t i) { /* fio_u2buf64 */
  __builtin_memcpy(buf, &i, sizeof(i));
}

#elif FIO_UNALIGNED_MEMORY_ACCESS_ENABLED
/** Converts an unaligned byte stream to a 16 bit number (local byte order). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16_local)(const void *c) {
  const uint16_t *tmp = (const uint16_t *)c; /* fio_buf2u16 */
  return *tmp;
}
/** Converts an unaligned byte stream to a 32 bit number (local byte order). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32_local)(const void *c) {
  const uint32_t *tmp = (const uint32_t *)c; /* fio_buf2u32 */
  return *tmp;
}
/** Converts an unaligned byte stream to a 64 bit number (local byte order). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64_local)(const void *c) {
  const uint64_t *tmp = (const uint64_t *)c; /* fio_buf2u64 */
  return *tmp;
}

/** Writes a local 16 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16_local)(void *buf, uint16_t i) {
  *((uint16_t *)buf) = i; /* fio_u2buf16 */
}
/** Writes a local 32 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32_local)(void *buf, uint32_t i) {
  *((uint32_t *)buf) = i; /* fio_u2buf32 */
}
/** Writes a local 64 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64_local)(void *buf, uint64_t i) {
  *((uint64_t *)buf) = i; /* fio_u2buf64 */
}

#else  /* no unaligned access, no builtin memcpy, use hope.. */
/** Converts an unaligned byte stream to a 16 bit number (local byte order). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16_local)(const void *c) {
  uint16_t tmp; /* fio_buf2u16 */
  memcpy(&tmp, c, sizeof(tmp));
  return tmp;
}
/** Converts an unaligned byte stream to a 32 bit number (local byte order). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32_local)(const void *c) {
  uint32_t tmp; /* fio_buf2u32 */
  memcpy(&tmp, c, sizeof(tmp));
  return tmp;
}
/** Converts an unaligned byte stream to a 64 bit number (local byte order). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64_local)(const void *c) {
  uint64_t tmp; /* fio_buf2u64 */
  memcpy(&tmp, c, sizeof(tmp));
  return tmp;
}

/** Writes a local 16 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16_local)(void *buf, uint16_t i) {
  memcpy(buf, &i, sizeof(i)); /* fio_u2buf16 */
}
/** Writes a local 32 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32_local)(void *buf, uint32_t i) {
  memcpy(buf, &i, sizeof(i)); /* fio_u2buf32 */
}
/** Writes a local 64 bit number to an unaligned buffer. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64_local)(void *buf, uint64_t i) {
  memcpy(buf, &i, sizeof(i)); /* fio_u2buf64 */
}
#endif /* __has_builtin(__builtin_memcpy) /                                    \
          FIO_UNALIGNED_MEMORY_ACCESS_ENABLED */

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

#if __LITTLE_ENDIAN__
/** Converts an unaligned byte stream to a 16 bit number (Big Endian). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16)(const void *c) { /* fio_buf2u16 */
  return FIO_NAME2(fio_buf, u16_bswap)(c);
}
/** Converts an unaligned byte stream to a 32 bit number (Big Endian). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32)(const void *c) { /* fio_buf2u32 */
  return FIO_NAME2(fio_buf, u32_bswap)(c);
}
/** Converts an unaligned byte stream to a 64 bit number (Big Endian). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64)(const void *c) { /* fio_buf2u64 */
  return FIO_NAME2(fio_buf, u64_bswap)(c);
}
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

/** Writes a local 16 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16)(void *buf, uint16_t i) {
  FIO_NAME2(fio_u, buf16_bswap)(buf, i);
}
/** Writes a local 32 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32)(void *buf, uint32_t i) {
  FIO_NAME2(fio_u, buf32_bswap)(buf, i);
}
/** Writes a local 64 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64)(void *buf, uint64_t i) {
  FIO_NAME2(fio_u, buf64_bswap)(buf, i);
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
#else
/** Converts an unaligned byte stream to a 16 bit number (Big Endian). */
FIO_IFUNC uint16_t FIO_NAME2(fio_buf, u16)(const void *c) { /* fio_buf2u16 */
  return FIO_NAME2(fio_buf, u16_local)(c);
}
/** Converts an unaligned byte stream to a 32 bit number (Big Endian). */
FIO_IFUNC uint32_t FIO_NAME2(fio_buf, u32)(const void *c) { /* fio_buf2u32 */
  return FIO_NAME2(fio_buf, u32_local)(c);
}
/** Converts an unaligned byte stream to a 64 bit number (Big Endian). */
FIO_IFUNC uint64_t FIO_NAME2(fio_buf, u64)(const void *c) { /* fio_buf2u64 */
  return FIO_NAME2(fio_buf, u64_local)(c);
}
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

/** Writes a local 16 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf16)(void *buf, uint16_t i) {
  FIO_NAME2(fio_u, buf16_local)(buf, i);
}
/** Writes a local 32 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf32)(void *buf, uint32_t i) {
  FIO_NAME2(fio_u, buf32_local)(buf, i);
}
/** Writes a local 64 bit number to an unaligned buffer in Big Endian. */
FIO_IFUNC void FIO_NAME2(fio_u, buf64)(void *buf, uint64_t i) {
  FIO_NAME2(fio_u, buf64_local)(buf, i);
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

#endif

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

/* *****************************************************************************
Byte masking (XOR)
***************************************************************************** */

/**
 * Masks 64 bit memory aligned data using a 64 bit mask and a counter mode
 * nonce.
 *
 * Returns the end state of the mask.
 */
FIO_IFUNC uint64_t fio___xmask_aligned64(uint64_t buf[],
                                         size_t byte_len,
                                         uint64_t mask,
                                         uint64_t nonce) {

  register uint64_t m = mask;
  for (size_t i = byte_len >> 3; i; --i) {
    *buf ^= m;
    m += nonce;
    ++buf;
  }
  mask = m;
  union { /* type punning */
    char *p8;
    uint64_t *p64;
  } pn, mpn;
  pn.p64 = buf;
  mpn.p64 = &mask;

  switch ((byte_len & 7)) {
  case 0:
    return mask;
  case 7:
    pn.p8[6] ^= mpn.p8[6];
  /* fallthrough */
  case 6:
    pn.p8[5] ^= mpn.p8[5];
  /* fallthrough */
  case 5:
    pn.p8[4] ^= mpn.p8[4];
  /* fallthrough */
  case 4:
    pn.p8[3] ^= mpn.p8[3];
  /* fallthrough */
  case 3:
    pn.p8[2] ^= mpn.p8[2];
  /* fallthrough */
  case 2:
    pn.p8[1] ^= mpn.p8[1];
  /* fallthrough */
  case 1:
    pn.p8[0] ^= mpn.p8[0];
    /* fallthrough */
  }
  return mask;
}

/**
 * Masks unaligned memory data using a 64 bit mask and a counter mode nonce.
 *
 * Returns the end state of the mask.
 */
FIO_IFUNC uint64_t fio___xmask_unaligned_words(void *buf_,
                                               size_t len,
                                               uint64_t mask,
                                               const uint64_t nonce) {
  register uint8_t *buf = (uint8_t *)buf_;
  register uint64_t m = mask;
  for (size_t i = len >> 3; i; --i) {
    uint64_t tmp;
    tmp = FIO_NAME2(fio_buf, u64_local)(buf);
    tmp ^= m;
    FIO_NAME2(fio_u, buf64_local)(buf, tmp);
    m += nonce;
    buf += 8;
  }
  mask = m;
  switch ((len & 7)) {
  case 0:
    return mask;
  case 7:
    buf[6] ^= ((uint8_t *)(&mask))[6];
  /* fallthrough */
  case 6:
    buf[5] ^= ((uint8_t *)(&mask))[5];
  /* fallthrough */
  case 5:
    buf[4] ^= ((uint8_t *)(&mask))[4];
  /* fallthrough */
  case 4:
    buf[3] ^= ((uint8_t *)(&mask))[3];
  /* fallthrough */
  case 3:
    buf[2] ^= ((uint8_t *)(&mask))[2];
  /* fallthrough */
  case 2:
    buf[1] ^= ((uint8_t *)(&mask))[1];
  /* fallthrough */
  case 1:
    buf[0] ^= ((uint8_t *)(&mask))[0];
    /* fallthrough */
  }
  return mask;
}

/**
 * Masks data using a 64 bit mask and a counter mode nonce. When the buffer's
 * memory is aligned, the function may perform significantly better.
 *
 * Returns the end state of the mask.
 */
FIO_IFUNC uint64_t fio_xmask(char *buf,
                             size_t len,
                             uint64_t mask,
                             uint64_t nonce) {
  if (!((uintptr_t)buf & 7)) {
    union {
      char *p8;
      uint64_t *p64;
    } pn;
    pn.p8 = buf;
    return fio___xmask_aligned64(pn.p64, len, mask, nonce);
  }
  return fio___xmask_unaligned_words(buf, len, mask, nonce);
}

/* *****************************************************************************
Hemming Distance and bit counting
***************************************************************************** */

#if __has_builtin(__builtin_popcountll)
#define fio_popcount(n) __builtin_popcountll(n)
#else
FIO_IFUNC int fio_popcount(uint64_t n) {
  int c = 0;
  while (n) {
    ++c;
    n &= n - 1;
  }
  return c;
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

#endif
#undef FIO_BITMAP
