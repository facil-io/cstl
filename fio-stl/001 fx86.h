/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




              Fake (Portable) X86 SIMD Intrinsics using fio_uXXX types

              Allows testing X86 NI code paths on non-X86 systems.


Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if (defined(FIO_FAKE_X86) || defined(FIO_FAKE_X86_SHADOW)) &&                 \
    !defined(H___FIO_FAKE_X86___H)
#define H___FIO_FAKE_X86___H 1

/* *****************************************************************************
Fake X86 Intrinsics - Overview

When FIO_FAKE_X86 is defined, this module provides portable implementations
of every X86 SIMD intrinsic used in the facil.io cstl library, using the
library's own fio_u128 / fio_u256 types.

All functions use the fio_fx86_ prefix.

Modes:
  - Default: fio_fx86_* functions available alongside real intrinsics (if any).
  - FIO_FAKE_X86_SHADOW: #define macros shadow real intrinsic names with
    fio_fx86_* implementations, allowing X86 code paths to compile on ARM etc.

On real X86 with intrinsics available (FIO___HAS_X86_INTRIN), the fio_fx86_*
functions are thin wrappers around the real intrinsics (native passthrough).
***************************************************************************** */

/* *****************************************************************************
Type Definitions

On non-X86 systems (or when shadowing), define __m128i / __m256i as aliases
for fio_u128 / fio_u256. On real X86, these types already exist from
<immintrin.h>.
***************************************************************************** */

#if !defined(FIO___HAS_X86_INTRIN)
typedef fio_u128 __m128i;
typedef fio_u256 __m256i;
/* _MM_HINT constants for _mm_prefetch (no-op in fake mode) */
#ifndef _MM_HINT_T0
#define _MM_HINT_T0  3
#define _MM_HINT_T1  2
#define _MM_HINT_T2  1
#define _MM_HINT_NTA 0
#endif
#endif /* !FIO___HAS_X86_INTRIN */

/* *****************************************************************************
Section A: SSE2 Intrinsics (20 functions)
***************************************************************************** */

/* --- Load / Store --- */

/** _mm_loadu_si128: load 128 bits from unaligned memory. */
FIO_IFUNC __m128i fio_fx86_loadu_si128(const void *p) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_loadu_si128((const __m128i *)p);
#else
  __m128i r;
  fio_memcpy16(&r, p);
  return r;
#endif
}

/** _mm_load_si128: load 128 bits from 16-byte aligned memory. */
FIO_IFUNC __m128i fio_fx86_load_si128(const void *p) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_load_si128((const __m128i *)p);
#else
  __m128i r;
  fio_memcpy16(&r, p);
  return r;
#endif
}

/** _mm_storeu_si128: store 128 bits to unaligned memory. */
FIO_IFUNC void fio_fx86_storeu_si128(void *p, __m128i a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  _mm_storeu_si128((__m128i *)p, a);
#else
  fio_memcpy16(p, &a);
#endif
}

/* --- Arithmetic --- */

/** _mm_add_epi32: add packed 32-bit integers. */
FIO_IFUNC __m128i fio_fx86_add_epi32(__m128i a, __m128i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_add_epi32(a, b);
#else
  __m128i r;
  for (int i = 0; i < 4; ++i)
    r.u32[i] = a.u32[i] + b.u32[i];
  return r;
#endif
}

/* --- Bitwise --- */

/** _mm_xor_si128: bitwise XOR of 128-bit values. */
FIO_IFUNC __m128i fio_fx86_xor_si128(__m128i a, __m128i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_xor_si128(a, b);
#else
  __m128i r;
  r.u64[0] = a.u64[0] ^ b.u64[0];
  r.u64[1] = a.u64[1] ^ b.u64[1];
  return r;
#endif
}

/** _mm_or_si128: bitwise OR of 128-bit values. */
FIO_IFUNC __m128i fio_fx86_or_si128(__m128i a, __m128i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_or_si128(a, b);
#else
  __m128i r;
  r.u64[0] = a.u64[0] | b.u64[0];
  r.u64[1] = a.u64[1] | b.u64[1];
  return r;
#endif
}

/** _mm_and_si128: bitwise AND of 128-bit values. */
FIO_IFUNC __m128i fio_fx86_and_si128(__m128i a, __m128i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_and_si128(a, b);
#else
  __m128i r;
  r.u64[0] = a.u64[0] & b.u64[0];
  r.u64[1] = a.u64[1] & b.u64[1];
  return r;
#endif
}

/* --- Shift (whole register) --- */

/** _mm_slli_si128: shift left by imm8 BYTES (not bits), zero-fill right. */
FIO_IFUNC __m128i fio_fx86_slli_si128(__m128i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  /* Must use a switch because the intrinsic requires a compile-time constant */
  switch (n) {
  case 0: return a;
  case 1: return _mm_slli_si128(a, 1);
  case 2: return _mm_slli_si128(a, 2);
  case 3: return _mm_slli_si128(a, 3);
  case 4: return _mm_slli_si128(a, 4);
  case 5: return _mm_slli_si128(a, 5);
  case 6: return _mm_slli_si128(a, 6);
  case 7: return _mm_slli_si128(a, 7);
  case 8: return _mm_slli_si128(a, 8);
  case 9: return _mm_slli_si128(a, 9);
  case 10: return _mm_slli_si128(a, 10);
  case 11: return _mm_slli_si128(a, 11);
  case 12: return _mm_slli_si128(a, 12);
  case 13: return _mm_slli_si128(a, 13);
  case 14: return _mm_slli_si128(a, 14);
  case 15: return _mm_slli_si128(a, 15);
  default: return _mm_setzero_si128();
  }
#else
  if (!n)
    return a;
  if (n >= 16) {
    __m128i r;
    FIO_MEMSET(&r, 0, 16);
    return r;
  }
  __m128i r;
  FIO_MEMSET(&r, 0, 16);
  FIO_MEMCPY(r.u8 + n, a.u8, (size_t)(16 - n));
  return r;
#endif
}

/** _mm_srli_si128: shift right by imm8 BYTES (not bits), zero-fill left. */
FIO_IFUNC __m128i fio_fx86_srli_si128(__m128i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  switch (n) {
  case 0: return a;
  case 1: return _mm_srli_si128(a, 1);
  case 2: return _mm_srli_si128(a, 2);
  case 3: return _mm_srli_si128(a, 3);
  case 4: return _mm_srli_si128(a, 4);
  case 5: return _mm_srli_si128(a, 5);
  case 6: return _mm_srli_si128(a, 6);
  case 7: return _mm_srli_si128(a, 7);
  case 8: return _mm_srli_si128(a, 8);
  case 9: return _mm_srli_si128(a, 9);
  case 10: return _mm_srli_si128(a, 10);
  case 11: return _mm_srli_si128(a, 11);
  case 12: return _mm_srli_si128(a, 12);
  case 13: return _mm_srli_si128(a, 13);
  case 14: return _mm_srli_si128(a, 14);
  case 15: return _mm_srli_si128(a, 15);
  default: return _mm_setzero_si128();
  }
#else
  if (!n)
    return a;
  if (n >= 16) {
    __m128i r;
    FIO_MEMSET(&r, 0, 16);
    return r;
  }
  __m128i r;
  FIO_MEMSET(&r, 0, 16);
  FIO_MEMCPY(r.u8, a.u8 + n, (size_t)(16 - n));
  return r;
#endif
}

/* --- Shift (per-element) --- */

/** _mm_slli_epi32: shift each 32-bit lane left by imm8 bits. */
FIO_IFUNC __m128i fio_fx86_slli_epi32(__m128i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  switch (n) {
  case 0: return a;
  case 1: return _mm_slli_epi32(a, 1);
  case 2: return _mm_slli_epi32(a, 2);
  case 7: return _mm_slli_epi32(a, 7);
  case 16: return _mm_slli_epi32(a, 16);
  case 25: return _mm_slli_epi32(a, 25);
  case 30: return _mm_slli_epi32(a, 30);
  case 31: return _mm_slli_epi32(a, 31);
  default: {
    uint32_t in[4], out[4];
    __m128i r;
    fio_memcpy16(in, &a);
    for (int i = 0; i < 4; ++i)
      out[i] = (n >= 32) ? 0 : (in[i] << n);
    fio_memcpy16(&r, out);
    return r;
  }
  }
#else
  __m128i r;
  if (n >= 32) {
    FIO_MEMSET(&r, 0, 16);
    return r;
  }
  for (int i = 0; i < 4; ++i)
    r.u32[i] = a.u32[i] << n;
  return r;
#endif
}

/** _mm_srli_epi32: shift each 32-bit lane right (logical) by imm8 bits. */
FIO_IFUNC __m128i fio_fx86_srli_epi32(__m128i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  switch (n) {
  case 0: return a;
  case 1: return _mm_srli_epi32(a, 1);
  case 2: return _mm_srli_epi32(a, 2);
  case 7: return _mm_srli_epi32(a, 7);
  case 16: return _mm_srli_epi32(a, 16);
  case 25: return _mm_srli_epi32(a, 25);
  case 30: return _mm_srli_epi32(a, 30);
  case 31: return _mm_srli_epi32(a, 31);
  default: {
    uint32_t in[4], out[4];
    __m128i r;
    fio_memcpy16(in, &a);
    for (int i = 0; i < 4; ++i)
      out[i] = (n >= 32) ? 0 : (in[i] >> n);
    fio_memcpy16(&r, out);
    return r;
  }
  }
#else
  __m128i r;
  if (n >= 32) {
    FIO_MEMSET(&r, 0, 16);
    return r;
  }
  for (int i = 0; i < 4; ++i)
    r.u32[i] = a.u32[i] >> n;
  return r;
#endif
}

/** _mm_slli_epi64: shift each 64-bit lane left by imm8 bits. */
FIO_IFUNC __m128i fio_fx86_slli_epi64(__m128i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  switch (n) {
  case 0: return a;
  case 1: return _mm_slli_epi64(a, 1);
  case 2: return _mm_slli_epi64(a, 2);
  case 7: return _mm_slli_epi64(a, 7);
  case 57: return _mm_slli_epi64(a, 57);
  case 62: return _mm_slli_epi64(a, 62);
  case 63: return _mm_slli_epi64(a, 63);
  default: {
    uint64_t in[2], out[2];
    __m128i r;
    fio_memcpy16(in, &a);
    for (int i = 0; i < 2; ++i)
      out[i] = (n >= 64) ? 0 : (in[i] << n);
    fio_memcpy16(&r, out);
    return r;
  }
  }
#else
  __m128i r;
  if (n >= 64) {
    FIO_MEMSET(&r, 0, 16);
    return r;
  }
  for (int i = 0; i < 2; ++i)
    r.u64[i] = a.u64[i] << n;
  return r;
#endif
}

/** _mm_srli_epi64: shift each 64-bit lane right (logical) by imm8 bits. */
FIO_IFUNC __m128i fio_fx86_srli_epi64(__m128i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  switch (n) {
  case 0: return a;
  case 1: return _mm_srli_epi64(a, 1);
  case 2: return _mm_srli_epi64(a, 2);
  case 7: return _mm_srli_epi64(a, 7);
  case 57: return _mm_srli_epi64(a, 57);
  case 62: return _mm_srli_epi64(a, 62);
  case 63: return _mm_srli_epi64(a, 63);
  default: {
    uint64_t in[2], out[2];
    __m128i r;
    fio_memcpy16(in, &a);
    for (int i = 0; i < 2; ++i)
      out[i] = (n >= 64) ? 0 : (in[i] >> n);
    fio_memcpy16(&r, out);
    return r;
  }
  }
#else
  __m128i r;
  if (n >= 64) {
    FIO_MEMSET(&r, 0, 16);
    return r;
  }
  for (int i = 0; i < 2; ++i)
    r.u64[i] = a.u64[i] >> n;
  return r;
#endif
}

/* --- Shuffle / Permute --- */

/** _mm_shuffle_epi32: shuffle 32-bit lanes according to imm8 control. */
FIO_IFUNC __m128i fio_fx86_shuffle_epi32(__m128i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  switch (n) {
  case 0x0E: return _mm_shuffle_epi32(a, 0x0E);
  case 0x1B: return _mm_shuffle_epi32(a, 0x1B);
  case 0xAA: return _mm_shuffle_epi32(a, 0xAA);
  case 0xB1: return _mm_shuffle_epi32(a, 0xB1);
  case 0xFF: return _mm_shuffle_epi32(a, 0xFF);
  default: {
    uint32_t in[4], out[4];
    __m128i r;
    fio_memcpy16(in, &a);
    out[0] = in[(n >> 0) & 3];
    out[1] = in[(n >> 2) & 3];
    out[2] = in[(n >> 4) & 3];
    out[3] = in[(n >> 6) & 3];
    fio_memcpy16(&r, out);
    return r;
  }
  }
#else
  __m128i r;
  r.u32[0] = a.u32[(n >> 0) & 3];
  r.u32[1] = a.u32[(n >> 2) & 3];
  r.u32[2] = a.u32[(n >> 4) & 3];
  r.u32[3] = a.u32[(n >> 6) & 3];
  return r;
#endif
}

/* --- Compare --- */

/** _mm_cmpeq_epi8: compare packed 8-bit integers for equality. */
FIO_IFUNC __m128i fio_fx86_cmpeq_epi8(__m128i a, __m128i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_cmpeq_epi8(a, b);
#else
  __m128i r;
  for (int i = 0; i < 16; ++i)
    r.u8[i] = (a.u8[i] == b.u8[i]) ? 0xFF : 0x00;
  return r;
#endif
}

/* --- Movemask --- */

/** _mm_movemask_epi8: create 16-bit mask from MSBs of each byte. */
FIO_IFUNC int fio_fx86_movemask_epi8(__m128i a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_movemask_epi8(a);
#else
  uint32_t r = 0;
  for (int i = 0; i < 16; ++i)
    r |= ((uint32_t)((a.u8[i] >> 7) & 1U) << i);
  return (int)r;
#endif
}

/* --- Set --- */

/** _mm_set_epi32: set packed 32-bit integers (high to low order). */
FIO_IFUNC __m128i fio_fx86_set_epi32(int e3, int e2, int e1, int e0) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_set_epi32(e3, e2, e1, e0);
#else
  __m128i r;
  r.u32[0] = (uint32_t)e0;
  r.u32[1] = (uint32_t)e1;
  r.u32[2] = (uint32_t)e2;
  r.u32[3] = (uint32_t)e3;
  return r;
#endif
}

/** _mm_set_epi64x: set packed 64-bit integers (high, low). */
FIO_IFUNC __m128i fio_fx86_set_epi64x(int64_t e1, int64_t e0) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_set_epi64x(e1, e0);
#else
  __m128i r;
  r.u64[0] = (uint64_t)e0;
  r.u64[1] = (uint64_t)e1;
  return r;
#endif
}

/** _mm_set_epi8: set packed 8-bit integers (high to low: e15..e0). */
FIO_IFUNC __m128i fio_fx86_set_epi8(char e15,
                                    char e14,
                                    char e13,
                                    char e12,
                                    char e11,
                                    char e10,
                                    char e9,
                                    char e8,
                                    char e7,
                                    char e6,
                                    char e5,
                                    char e4,
                                    char e3,
                                    char e2,
                                    char e1,
                                    char e0) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_set_epi8(e15,
                      e14,
                      e13,
                      e12,
                      e11,
                      e10,
                      e9,
                      e8,
                      e7,
                      e6,
                      e5,
                      e4,
                      e3,
                      e2,
                      e1,
                      e0);
#else
  __m128i r;
  r.u8[0] = (uint8_t)e0;
  r.u8[1] = (uint8_t)e1;
  r.u8[2] = (uint8_t)e2;
  r.u8[3] = (uint8_t)e3;
  r.u8[4] = (uint8_t)e4;
  r.u8[5] = (uint8_t)e5;
  r.u8[6] = (uint8_t)e6;
  r.u8[7] = (uint8_t)e7;
  r.u8[8] = (uint8_t)e8;
  r.u8[9] = (uint8_t)e9;
  r.u8[10] = (uint8_t)e10;
  r.u8[11] = (uint8_t)e11;
  r.u8[12] = (uint8_t)e12;
  r.u8[13] = (uint8_t)e13;
  r.u8[14] = (uint8_t)e14;
  r.u8[15] = (uint8_t)e15;
  return r;
#endif
}

/** _mm_set1_epi8: broadcast 8-bit integer to all 16 bytes. */
FIO_IFUNC __m128i fio_fx86_set1_epi8(char a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_set1_epi8(a);
#else
  __m128i r;
  FIO_MEMSET(&r, (uint8_t)a, 16);
  return r;
#endif
}

/** _mm_setzero_si128: return zero vector. */
FIO_IFUNC __m128i fio_fx86_setzero_si128(void) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_setzero_si128();
#else
  __m128i r;
  FIO_MEMSET(&r, 0, 16);
  return r;
#endif
}

/** _mm_cvtsi32_si128: set low 32 bits from int, zero upper bits. */
FIO_IFUNC __m128i fio_fx86_cvtsi32_si128(int a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return _mm_cvtsi32_si128(a);
#else
  __m128i r;
  FIO_MEMSET(&r, 0, 16);
  r.u32[0] = (uint32_t)a;
  return r;
#endif
}

/* *****************************************************************************
Section B: SSSE3 Intrinsics (2 functions)
***************************************************************************** */

/** _mm_shuffle_epi8: byte-level shuffle using control mask. */
FIO_IFUNC __m128i fio_fx86_shuffle_epi8(__m128i a, __m128i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSSE3__)
  return _mm_shuffle_epi8(a, b);
#else
  __m128i r;
  for (int i = 0; i < 16; ++i) {
    if (b.u8[i] & 0x80)
      r.u8[i] = 0;
    else
      r.u8[i] = a.u8[b.u8[i] & 0x0F];
  }
  return r;
#endif
}

/** _mm_alignr_epi8: concatenate a:b, shift right by imm8 bytes. */
FIO_IFUNC __m128i fio_fx86_alignr_epi8(__m128i a, __m128i b, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSSE3__)
  switch (n) {
  case 4: return _mm_alignr_epi8(a, b, 4);
  case 8: return _mm_alignr_epi8(a, b, 8);
  default: {
    /* Fallback for other values — pad to 48 bytes for imm8 17..31 safety */
    uint8_t tmp[48];
    fio_memcpy16(tmp, &b);
    fio_memcpy16(tmp + 16, &a);
    FIO_MEMSET(tmp + 32, 0, 16);
    __m128i r;
    if (n >= 32)
      FIO_MEMSET(&r, 0, 16);
    else
      fio_memcpy16(&r, tmp + n);
    return r;
  }
  }
#else
  /* Pad to 48 bytes so imm8 values 17..31 don't read out of bounds */
  uint8_t tmp[48];
  fio_memcpy16(tmp, &b);
  fio_memcpy16(tmp + 16, &a);
  FIO_MEMSET(tmp + 32, 0, 16);
  __m128i r;
  if (n >= 32) {
    FIO_MEMSET(&r, 0, 16);
  } else {
    fio_memcpy16(&r, tmp + n);
  }
  return r;
#endif
}

/* *****************************************************************************
Section C: SSE4.1 Intrinsics (2 functions)
***************************************************************************** */

/** _mm_blend_epi16: blend 16-bit lanes from a and b using imm8 mask. */
FIO_IFUNC __m128i fio_fx86_blend_epi16(__m128i a, __m128i b, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE4_1__)
  switch (n) {
  case 0xF0: return _mm_blend_epi16(a, b, 0xF0);
  default: {
    uint16_t va[8], vb[8], out[8];
    __m128i r;
    fio_memcpy16(va, &a);
    fio_memcpy16(vb, &b);
    for (int i = 0; i < 8; ++i)
      out[i] = ((n >> i) & 1U) ? vb[i] : va[i];
    fio_memcpy16(&r, out);
    return r;
  }
  }
#else
  __m128i r;
  for (int i = 0; i < 8; ++i)
    r.u16[i] = ((n >> i) & 1U) ? b.u16[i] : a.u16[i];
  return r;
#endif
}

/** _mm_extract_epi32: extract 32-bit integer from lane imm8. */
FIO_IFUNC int fio_fx86_extract_epi32(__m128i a, int imm8) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE4_1__)
  switch (imm8 & 3) {
  case 0: return _mm_extract_epi32(a, 0);
  case 1: return _mm_extract_epi32(a, 1);
  case 2: return _mm_extract_epi32(a, 2);
  default: return _mm_extract_epi32(a, 3);
  }
#else
  return (int)a.u32[imm8 & 3];
#endif
}

/* *****************************************************************************
Section D: Prefetch (1 function)
***************************************************************************** */

/** _mm_prefetch: prefetch data into cache. No-op in fake mode. */
FIO_IFUNC void fio_fx86_prefetch(const void *p, int hint) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  switch (hint) {
  case _MM_HINT_T0: _mm_prefetch((const char *)p, _MM_HINT_T0); break;
  case _MM_HINT_T1: _mm_prefetch((const char *)p, _MM_HINT_T1); break;
  case _MM_HINT_T2: _mm_prefetch((const char *)p, _MM_HINT_T2); break;
  default: _mm_prefetch((const char *)p, _MM_HINT_NTA); break;
  }
#else
  (void)p;
  (void)hint;
#endif
}

/* *****************************************************************************
Section E: AES-NI Intrinsics (3 functions)

Portable AES round implementations using lookup-table S-box.
These are correct but NOT constant-time (table lookups are cache-dependent).
For testing/portability only — production should use real AES-NI or the
library's bitsliced implementation.
***************************************************************************** */

/* AES forward S-box (FIPS 197, Figure 7) */
static const uint8_t fio___fx86_aes_sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
    0xb0, 0x54, 0xbb, 0x16};

#if !defined(FIO___HAS_X86_INTRIN)

/* GF(2^8) multiply by 2 (xtime) for MixColumns. */
FIO_IFUNC uint8_t fio___fx86_xtime(uint8_t x) {
  return (uint8_t)((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

/* SubBytes: apply S-box to each byte. */
FIO_IFUNC void fio___fx86_sub_bytes(uint8_t state[16]) {
  for (int i = 0; i < 16; ++i)
    state[i] = fio___fx86_aes_sbox[state[i]];
}

/* ShiftRows: cyclically shift rows of the 4x4 state matrix.
 * AES state is column-major: byte index = row + 4*col.
 * Row 0: no shift. Row 1: shift left 1. Row 2: shift left 2. Row 3: shift
 * left 3. */
FIO_IFUNC void fio___fx86_shift_rows(uint8_t s[16]) {
  uint8_t t;
  /* Row 1: shift left by 1 */
  t = s[1];
  s[1] = s[5];
  s[5] = s[9];
  s[9] = s[13];
  s[13] = t;
  /* Row 2: shift left by 2 */
  t = s[2];
  s[2] = s[10];
  s[10] = t;
  t = s[6];
  s[6] = s[14];
  s[14] = t;
  /* Row 3: shift left by 3 (= right by 1) */
  t = s[15];
  s[15] = s[11];
  s[11] = s[7];
  s[7] = s[3];
  s[3] = t;
}

/* MixColumns: mix each column of the state matrix. */
FIO_IFUNC void fio___fx86_mix_columns(uint8_t s[16]) {
  for (int c = 0; c < 4; ++c) {
    int i = c * 4;
    uint8_t a0 = s[i], a1 = s[i + 1], a2 = s[i + 2], a3 = s[i + 3];
    uint8_t t = a0 ^ a1 ^ a2 ^ a3;
    s[i + 0] = a0 ^ fio___fx86_xtime(a0 ^ a1) ^ t;
    s[i + 1] = a1 ^ fio___fx86_xtime(a1 ^ a2) ^ t;
    s[i + 2] = a2 ^ fio___fx86_xtime(a2 ^ a3) ^ t;
    s[i + 3] = a3 ^ fio___fx86_xtime(a3 ^ a0) ^ t;
  }
}

#endif /* !FIO___HAS_X86_INTRIN */

/**
 * _mm_aesenc_si128: one AES encryption round.
 * Performs SubBytes -> ShiftRows -> MixColumns -> AddRoundKey.
 */
FIO_IFUNC __m128i fio_fx86_aesenc_si128(__m128i a, __m128i roundkey) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AES__)
  return _mm_aesenc_si128(a, roundkey);
#else
  uint8_t state[16];
  fio_memcpy16(state, &a);
  fio___fx86_sub_bytes(state);
  fio___fx86_shift_rows(state);
  fio___fx86_mix_columns(state);
  /* AddRoundKey */
  for (int i = 0; i < 16; ++i)
    state[i] ^= ((const uint8_t *)&roundkey)[i];
  __m128i r;
  fio_memcpy16(&r, state);
  return r;
#endif
}

/**
 * _mm_aesenclast_si128: last AES encryption round.
 * Performs SubBytes -> ShiftRows -> AddRoundKey (no MixColumns).
 */
FIO_IFUNC __m128i fio_fx86_aesenclast_si128(__m128i a, __m128i roundkey) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AES__)
  return _mm_aesenclast_si128(a, roundkey);
#else
  uint8_t state[16];
  fio_memcpy16(state, &a);
  fio___fx86_sub_bytes(state);
  fio___fx86_shift_rows(state);
  /* AddRoundKey (no MixColumns) */
  for (int i = 0; i < 16; ++i)
    state[i] ^= ((const uint8_t *)&roundkey)[i];
  __m128i r;
  fio_memcpy16(&r, state);
  return r;
#endif
}

/**
 * _mm_aeskeygenassist_si128: AES key generation assist.
 * Applies SubWord to columns 2 and 3 of the input, rotates column 3,
 * and XORs with the round constant (rcon) in the appropriate position.
 *
 * dst[31:0]   = SubWord(src[63:32])
 * dst[63:32]  = RotWord(SubWord(src[63:32])) XOR rcon
 * dst[95:64]  = SubWord(src[127:96])
 * dst[127:96] = RotWord(SubWord(src[127:96])) XOR rcon
 */
FIO_IFUNC __m128i fio_fx86_aeskeygenassist_si128(__m128i a, int imm8) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AES__)
  switch (imm8) {
  case 0x00: return _mm_aeskeygenassist_si128(a, 0x00);
  case 0x01: return _mm_aeskeygenassist_si128(a, 0x01);
  case 0x02: return _mm_aeskeygenassist_si128(a, 0x02);
  case 0x04: return _mm_aeskeygenassist_si128(a, 0x04);
  case 0x08: return _mm_aeskeygenassist_si128(a, 0x08);
  case 0x10: return _mm_aeskeygenassist_si128(a, 0x10);
  case 0x1B: return _mm_aeskeygenassist_si128(a, 0x1B);
  case 0x20: return _mm_aeskeygenassist_si128(a, 0x20);
  case 0x36: return _mm_aeskeygenassist_si128(a, 0x36);
  case 0x40: return _mm_aeskeygenassist_si128(a, 0x40);
  case 0x80: return _mm_aeskeygenassist_si128(a, 0x80);
  default: {
    /* General fallback — should not be reached for AES-128/256 */
    uint8_t rcon = (uint8_t)imm8;
    const uint8_t *src = (const uint8_t *)&a;
    uint8_t x1[4], x3[4];
    for (int i = 0; i < 4; ++i) {
      x1[i] = fio___fx86_aes_sbox[src[4 + i]];
      x3[i] = fio___fx86_aes_sbox[src[12 + i]];
    }
    __m128i r;
    uint8_t *dst = (uint8_t *)&r;
    /* dst[31:0] = SubWord(src[63:32]) */
    dst[0] = x1[0];
    dst[1] = x1[1];
    dst[2] = x1[2];
    dst[3] = x1[3];
    /* dst[63:32] = RotWord(SubWord(src[63:32])) XOR rcon
     * RotWord({x0,x1,x2,x3}) = {x1,x2,x3,x0}, rcon XORs byte 0 */
    dst[4] = x1[1] ^ rcon;
    dst[5] = x1[2];
    dst[6] = x1[3];
    dst[7] = x1[0];
    /* dst[95:64] = SubWord(src[127:96]) */
    dst[8] = x3[0];
    dst[9] = x3[1];
    dst[10] = x3[2];
    dst[11] = x3[3];
    /* dst[127:96] = RotWord(SubWord(src[127:96])) XOR rcon */
    dst[12] = x3[1] ^ rcon;
    dst[13] = x3[2];
    dst[14] = x3[3];
    dst[15] = x3[0];
    return r;
  }
  }
#else
  uint8_t rcon = (uint8_t)imm8;
  const uint8_t *src = (const uint8_t *)&a;
  /* SubWord on column 1 (bytes 4-7) */
  uint8_t x1[4], x3[4];
  for (int i = 0; i < 4; ++i) {
    x1[i] = fio___fx86_aes_sbox[src[4 + i]];
    x3[i] = fio___fx86_aes_sbox[src[12 + i]];
  }
  __m128i r;
  uint8_t *dst = (uint8_t *)&r;
  /* dst[31:0] = SubWord(src[63:32]) */
  dst[0] = x1[0];
  dst[1] = x1[1];
  dst[2] = x1[2];
  dst[3] = x1[3];
  /* dst[63:32] = RotWord(SubWord(src[63:32])) XOR rcon
   * RotWord({x0,x1,x2,x3}) = {x1,x2,x3,x0}, rcon XORs byte 0 */
  dst[4] = x1[1] ^ rcon;
  dst[5] = x1[2];
  dst[6] = x1[3];
  dst[7] = x1[0];
  /* dst[95:64] = SubWord(src[127:96]) */
  dst[8] = x3[0];
  dst[9] = x3[1];
  dst[10] = x3[2];
  dst[11] = x3[3];
  /* dst[127:96] = RotWord(SubWord(src[127:96])) XOR rcon */
  dst[12] = x3[1] ^ rcon;
  dst[13] = x3[2];
  dst[14] = x3[3];
  dst[15] = x3[0];
  return r;
#endif
}

/* *****************************************************************************
Section F: PCLMULQDQ Intrinsic (1 function)

Carry-less (GF(2)) multiplication of selected 64-bit halves.
***************************************************************************** */

/**
 * _mm_clmulepi64_si128: carry-less multiply of two 64-bit values.
 * imm8 selects which 64-bit halves to multiply:
 *   0x00: a.lo * b.lo
 *   0x01: a.lo * b.hi
 *   0x10: a.hi * b.lo
 *   0x11: a.hi * b.hi
 * Result is a 128-bit product in GF(2).
 */
FIO_IFUNC __m128i fio_fx86_clmulepi64_si128(__m128i a, __m128i b, int imm8) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__PCLMUL__)
  switch (imm8) {
  case 0x00: return _mm_clmulepi64_si128(a, b, 0x00);
  case 0x01: return _mm_clmulepi64_si128(a, b, 0x01);
  case 0x10: return _mm_clmulepi64_si128(a, b, 0x10);
  case 0x11: return _mm_clmulepi64_si128(a, b, 0x11);
  default: return _mm_clmulepi64_si128(a, b, 0x00);
  }
#else
  /* Select 64-bit lanes: bit 0 of imm8 selects a, bit 4 selects b */
  uint64_t va = a.u64[imm8 & 1];
  uint64_t vb = b.u64[(imm8 >> 4) & 1];
  /* Carry-less (GF(2)) multiplication: schoolbook algorithm.
   * NOT the same as fio_math_mulc64 which is regular arithmetic multiply. */
  uint64_t lo = 0, hi = 0;
  for (int i = 0; i < 64; ++i) {
    if ((vb >> i) & 1) {
      lo ^= va << i;
      if (i > 0)
        hi ^= va >> (64 - i);
    }
  }
  __m128i r;
  r.u64[0] = lo;
  r.u64[1] = hi;
  return r;
#endif
}

/* *****************************************************************************
Section G: SHA-NI Intrinsics (7 functions)

Portable implementations of Intel SHA Extensions for SHA-1 and SHA-256.
These implement the actual SHA round function logic.
***************************************************************************** */

/* --- SHA-256 intrinsics --- */

/**
 * _mm_sha256rnds2_epu32: perform 2 rounds of SHA-256.
 *
 * State layout: cdgh = {c, d, g, h}, abef = {a, b, e, f}
 * msg contains the round message+constant in the low 2 dwords.
 * Performs rounds using msg[0] then msg[1].
 */
FIO_IFUNC __m128i fio_fx86_sha256rnds2_epu32(__m128i cdgh,
                                             __m128i abef,
                                             __m128i msg) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA__)
  return _mm_sha256rnds2_epu32(cdgh, abef, msg);
#else
  /* Extract state: abef = {a, b, e, f}, cdgh = {c, d, g, h} */
  uint32_t a = abef.u32[3], b = abef.u32[2];
  uint32_t c = cdgh.u32[3], d = cdgh.u32[2];
  uint32_t e = abef.u32[1], f = abef.u32[0];
  uint32_t g = cdgh.u32[1], h = cdgh.u32[0];

  /* Two rounds */
  for (int i = 0; i < 2; ++i) {
    uint32_t wk = msg.u32[i]; /* Wi + Ki already combined */
    /* Sigma1(e) */
    uint32_t s1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^
                  ((e >> 25) | (e << 7));
    /* Ch(e, f, g) */
    uint32_t ch = (e & f) ^ ((~e) & g);
    /* temp1 */
    uint32_t t1 = h + s1 + ch + wk;
    /* Sigma0(a) */
    uint32_t s0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^
                  ((a >> 22) | (a << 10));
    /* Maj(a, b, c) */
    uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    /* temp2 */
    uint32_t t2 = s0 + maj;
    /* Shift state */
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }

  /* Pack result as new abef: {a, b, e, f} */
  __m128i r;
  r.u32[3] = a;
  r.u32[2] = b;
  r.u32[1] = e;
  r.u32[0] = f;
  return r;
#endif
}

/**
 * _mm_sha256msg1_epu32: SHA-256 message schedule part 1.
 * Computes sigma0 on msg1 elements and adds to msg0.
 * sigma0(x) = ROTR(7,x) ^ ROTR(18,x) ^ SHR(3,x)
 */
FIO_IFUNC __m128i fio_fx86_sha256msg1_epu32(__m128i msg0, __m128i msg1) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA__)
  return _mm_sha256msg1_epu32(msg0, msg1);
#else
  __m128i r;
  /* sigma0 inputs: {msg0[1], msg0[2], msg0[3], msg1[0]}
   * i.e. W[i+1]..W[i+3] from msg0, W[i+4] from msg1. */
  for (int i = 0; i < 4; ++i) {
    uint32_t x = (i < 3) ? msg0.u32[i + 1] : msg1.u32[0];
    uint32_t s0 = ((x >> 7) | (x << 25)) ^ ((x >> 18) | (x << 14)) ^ (x >> 3);
    r.u32[i] = msg0.u32[i] + s0;
  }
  return r;
#endif
}

/**
 * _mm_sha256msg2_epu32: SHA-256 message schedule part 2.
 * Computes sigma1 on the last two elements and propagates.
 * sigma1(x) = ROTR(17,x) ^ ROTR(19,x) ^ SHR(10,x)
 */
FIO_IFUNC __m128i fio_fx86_sha256msg2_epu32(__m128i msg4, __m128i msg1) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA__)
  return _mm_sha256msg2_epu32(msg4, msg1);
#else
  /* msg1 provides W[i-2] and W[i-1] in positions [2] and [3].
   * msg4 is the partially computed result from sha256msg1.
   * We add sigma1(W[i-2]) to msg4[0], sigma1(W[i-1]) to msg4[1],
   * then use the new values for msg4[2] and msg4[3]. */
  __m128i r;
  uint32_t w14 = msg1.u32[2]; /* W[i-2] */
  uint32_t w15 = msg1.u32[3]; /* W[i-1] */

  uint32_t s1_14 =
      ((w14 >> 17) | (w14 << 15)) ^ ((w14 >> 19) | (w14 << 13)) ^ (w14 >> 10);
  r.u32[0] = msg4.u32[0] + s1_14;

  uint32_t s1_15 =
      ((w15 >> 17) | (w15 << 15)) ^ ((w15 >> 19) | (w15 << 13)) ^ (w15 >> 10);
  r.u32[1] = msg4.u32[1] + s1_15;

  /* Now use the newly computed values for the next two */
  uint32_t w16 = r.u32[0]; /* W[i] just computed */
  uint32_t s1_16 =
      ((w16 >> 17) | (w16 << 15)) ^ ((w16 >> 19) | (w16 << 13)) ^ (w16 >> 10);
  r.u32[2] = msg4.u32[2] + s1_16;

  uint32_t w17 = r.u32[1]; /* W[i+1] just computed */
  uint32_t s1_17 =
      ((w17 >> 17) | (w17 << 15)) ^ ((w17 >> 19) | (w17 << 13)) ^ (w17 >> 10);
  r.u32[3] = msg4.u32[3] + s1_17;
  return r;
#endif
}

/* --- SHA-1 intrinsics --- */

/**
 * _mm_sha1rnds4_epu32: perform 4 rounds of SHA-1.
 * func selects the round function: 0=Ch, 1=Parity, 2=Maj, 3=Parity.
 * abcd = {a, b, c, d}, e0 has e in the high dword [3].
 */
FIO_IFUNC __m128i fio_fx86_sha1rnds4_epu32(__m128i abcd, __m128i e0, int func) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA__)
  switch (func) {
  case 0: return _mm_sha1rnds4_epu32(abcd, e0, 0);
  case 1: return _mm_sha1rnds4_epu32(abcd, e0, 1);
  case 2: return _mm_sha1rnds4_epu32(abcd, e0, 2);
  default: return _mm_sha1rnds4_epu32(abcd, e0, 3);
  }
#else
  /* State: a=abcd[3], b=abcd[2], c=abcd[1], d=abcd[0] */
  uint32_t a = abcd.u32[3], b = abcd.u32[2], c = abcd.u32[1], d = abcd.u32[0];
  uint32_t e =
      0; /* initialized for rounds 1-3; round 0 skips e (W0+E pre-combined) */
  /* e0 contains {W[0]+E, W[1], W[2], W[3]} where W[0]+E is pre-combined */
  uint32_t w[4] = {e0.u32[3], e0.u32[2], e0.u32[1], e0.u32[0]};

  /* SHA-1 round constants */
  static const uint32_t K[4] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};
  uint32_t k = K[func & 3];

  for (int i = 0; i < 4; ++i) {
    uint32_t f;
    switch (func & 3) {
    case 0: f = (b & c) ^ ((~b) & d); break;        /* Ch */
    case 1: f = b ^ c ^ d; break;                   /* Parity */
    case 2: f = (b & c) ^ (b & d) ^ (c & d); break; /* Maj */
    default: f = b ^ c ^ d; break;                  /* Parity */
    }
    /* Round 0: w[0] already has E added (W0+E), so don't add e again.
     * Rounds 1-3: e comes from the state shift (previous d). */
    uint32_t t;
    if (i == 0)
      t = ((a << 5) | (a >> 27)) + f + k + w[0];
    else
      t = ((a << 5) | (a >> 27)) + f + e + k + w[i];
    e = d;
    d = c;
    c = (b << 30) | (b >> 2);
    b = a;
    a = t;
  }

  __m128i r;
  r.u32[3] = a;
  r.u32[2] = b;
  r.u32[1] = c;
  r.u32[0] = d;
  return r;
#endif
}

/**
 * _mm_sha1nexte_epu32: SHA-1 next E value.
 * Rotates e (from a[3]) left by 30 and adds to b[3].
 * Returns {ROL30(a[3])+b[3], b[2], b[1], b[0]}.
 */
FIO_IFUNC __m128i fio_fx86_sha1nexte_epu32(__m128i a, __m128i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA__)
  return _mm_sha1nexte_epu32(a, b);
#else
  __m128i r;
  uint32_t e = a.u32[3];
  uint32_t rotated = (e << 30) | (e >> 2);
  r.u32[3] = rotated + b.u32[3];
  r.u32[2] = b.u32[2];
  r.u32[1] = b.u32[1];
  r.u32[0] = b.u32[0];
  return r;
#endif
}

/**
 * _mm_sha1msg1_epu32: SHA-1 message schedule part 1.
 * Per Intel pseudocode:
 *   dst[3] = msg0[3] XOR msg0[1]
 *   dst[2] = msg0[2] XOR msg0[0]
 *   dst[1] = msg0[1] XOR msg1[3]
 *   dst[0] = msg0[0] XOR msg1[2]
 */
FIO_IFUNC __m128i fio_fx86_sha1msg1_epu32(__m128i msg0, __m128i msg1) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA__)
  return _mm_sha1msg1_epu32(msg0, msg1);
#else
  __m128i r;
  r.u32[3] = msg0.u32[3] ^ msg0.u32[1];
  r.u32[2] = msg0.u32[2] ^ msg0.u32[0];
  r.u32[1] = msg0.u32[1] ^ msg1.u32[3];
  r.u32[0] = msg0.u32[0] ^ msg1.u32[2];
  return r;
#endif
}

/**
 * _mm_sha1msg2_epu32: SHA-1 message schedule part 2.
 * Per Intel pseudocode:
 *   t3 = msg5[3] XOR msg1[2]
 *   t2 = msg5[2] XOR msg1[1]
 *   t1 = msg5[1] XOR msg1[0]
 *   dst[3] = ROL1(t3)
 *   dst[2] = ROL1(t2)
 *   dst[1] = ROL1(t1)
 *   t0 = msg5[0] XOR dst[3]
 *   dst[0] = ROL1(t0)
 */
FIO_IFUNC __m128i fio_fx86_sha1msg2_epu32(__m128i msg5, __m128i msg1) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA__)
  return _mm_sha1msg2_epu32(msg5, msg1);
#else
  __m128i r;
  uint32_t t3 = msg5.u32[3] ^ msg1.u32[2];
  uint32_t t2 = msg5.u32[2] ^ msg1.u32[1];
  uint32_t t1 = msg5.u32[1] ^ msg1.u32[0];
  r.u32[3] = (t3 << 1) | (t3 >> 31);
  r.u32[2] = (t2 << 1) | (t2 >> 31);
  r.u32[1] = (t1 << 1) | (t1 >> 31);
  uint32_t t0 = msg5.u32[0] ^ r.u32[3];
  r.u32[0] = (t0 << 1) | (t0 >> 31);
  return r;
#endif
}

/* *****************************************************************************
Section H: AVX2 Intrinsics (32 functions)
***************************************************************************** */

/* --- Load / Store --- */

/** _mm256_loadu_si256: load 256 bits from unaligned memory. */
FIO_IFUNC __m256i fio_fx86_256_loadu_si256(const void *p) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_loadu_si256((const __m256i *)p);
#else
  __m256i r;
  fio_memcpy32(&r, p);
  return r;
#endif
}

/** _mm256_load_si256: load 256 bits from 32-byte aligned memory. */
FIO_IFUNC __m256i fio_fx86_256_load_si256(const void *p) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_load_si256((const __m256i *)p);
#else
  __m256i r;
  fio_memcpy32(&r, p);
  return r;
#endif
}

/** _mm256_storeu_si256: store 256 bits to unaligned memory. */
FIO_IFUNC void fio_fx86_256_storeu_si256(void *p, __m256i a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  _mm256_storeu_si256((__m256i *)p, a);
#else
  fio_memcpy32(p, &a);
#endif
}

/* --- Arithmetic (16-bit) --- */

/** _mm256_add_epi16: add packed 16-bit integers. */
FIO_IFUNC __m256i fio_fx86_256_add_epi16(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_add_epi16(a, b);
#else
  __m256i r;
  for (int i = 0; i < 16; ++i)
    r.u16[i] = a.u16[i] + b.u16[i];
  return r;
#endif
}

/** _mm256_sub_epi16: subtract packed 16-bit integers. */
FIO_IFUNC __m256i fio_fx86_256_sub_epi16(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_sub_epi16(a, b);
#else
  __m256i r;
  for (int i = 0; i < 16; ++i)
    r.u16[i] = a.u16[i] - b.u16[i];
  return r;
#endif
}

/** _mm256_mullo_epi16: multiply packed 16-bit integers, keep low 16 bits. */
FIO_IFUNC __m256i fio_fx86_256_mullo_epi16(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_mullo_epi16(a, b);
#else
  __m256i r;
  for (int i = 0; i < 16; ++i)
    r.u16[i] = (uint16_t)((uint32_t)a.u16[i] * (uint32_t)b.u16[i]);
  return r;
#endif
}

/* --- Arithmetic (32-bit) --- */

/** _mm256_add_epi32: add packed 32-bit integers. */
FIO_IFUNC __m256i fio_fx86_256_add_epi32(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_add_epi32(a, b);
#else
  __m256i r;
  for (int i = 0; i < 8; ++i)
    r.u32[i] = a.u32[i] + b.u32[i];
  return r;
#endif
}

/** _mm256_sub_epi32: subtract packed 32-bit integers. */
FIO_IFUNC __m256i fio_fx86_256_sub_epi32(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_sub_epi32(a, b);
#else
  __m256i r;
  for (int i = 0; i < 8; ++i)
    r.u32[i] = a.u32[i] - b.u32[i];
  return r;
#endif
}

/** _mm256_mullo_epi32: multiply packed 32-bit integers, keep low 32 bits. */
FIO_IFUNC __m256i fio_fx86_256_mullo_epi32(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_mullo_epi32(a, b);
#else
  __m256i r;
  for (int i = 0; i < 8; ++i)
    r.u32[i] = a.u32[i] * b.u32[i];
  return r;
#endif
}

/* --- Arithmetic (64-bit) --- */

/** _mm256_add_epi64: add packed 64-bit integers. */
FIO_IFUNC __m256i fio_fx86_256_add_epi64(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_add_epi64(a, b);
#else
  __m256i r;
  for (int i = 0; i < 4; ++i)
    r.u64[i] = a.u64[i] + b.u64[i];
  return r;
#endif
}

/** _mm256_sub_epi64: subtract packed 64-bit integers. */
FIO_IFUNC __m256i fio_fx86_256_sub_epi64(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_sub_epi64(a, b);
#else
  __m256i r;
  for (int i = 0; i < 4; ++i)
    r.u64[i] = a.u64[i] - b.u64[i];
  return r;
#endif
}

/* --- Shift (per-element 32-bit) --- */

/** _mm256_slli_epi32: shift each 32-bit lane left by imm8 bits. */
FIO_IFUNC __m256i fio_fx86_256_slli_epi32(__m256i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  switch (n) {
  case 16: return _mm256_slli_epi32(a, 16);
  default: {
    uint32_t in[8], out[8];
    __m256i r;
    fio_memcpy32(in, &a);
    for (int i = 0; i < 8; ++i)
      out[i] = (n >= 32) ? 0 : (in[i] << n);
    fio_memcpy32(&r, out);
    return r;
  }
  }
#else
  __m256i r;
  if (n >= 32) {
    FIO_MEMSET(&r, 0, 32);
    return r;
  }
  for (int i = 0; i < 8; ++i)
    r.u32[i] = a.u32[i] << n;
  return r;
#endif
}

/** _mm256_srai_epi32: arithmetic shift right each 32-bit lane by imm8. */
FIO_IFUNC __m256i fio_fx86_256_srai_epi32(__m256i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  switch (n) {
  case 16: return _mm256_srai_epi32(a, 16);
  case 26: return _mm256_srai_epi32(a, 26);
  default: {
    int32_t in[8], out[8];
    __m256i r;
    fio_memcpy32(in, &a);
    for (int i = 0; i < 8; ++i)
      out[i] = (n >= 32) ? (in[i] >> 31) : (in[i] >> n);
    fio_memcpy32(&r, out);
    return r;
  }
  }
#else
  __m256i r;
  for (int i = 0; i < 8; ++i)
    r.i32[i] = (n >= 32) ? (a.i32[i] >> 31) : (a.i32[i] >> n);
  return r;
#endif
}

/* --- Bitwise --- */

/** _mm256_xor_si256: bitwise XOR of 256-bit values. */
FIO_IFUNC __m256i fio_fx86_256_xor_si256(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_xor_si256(a, b);
#else
  __m256i r;
  for (int i = 0; i < 4; ++i)
    r.u64[i] = a.u64[i] ^ b.u64[i];
  return r;
#endif
}

/** _mm256_and_si256: bitwise AND of 256-bit values. */
FIO_IFUNC __m256i fio_fx86_256_and_si256(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_and_si256(a, b);
#else
  __m256i r;
  for (int i = 0; i < 4; ++i)
    r.u64[i] = a.u64[i] & b.u64[i];
  return r;
#endif
}

/** _mm256_or_si256: bitwise OR of 256-bit values. */
FIO_IFUNC __m256i fio_fx86_256_or_si256(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_or_si256(a, b);
#else
  __m256i r;
  for (int i = 0; i < 4; ++i)
    r.u64[i] = a.u64[i] | b.u64[i];
  return r;
#endif
}

/* --- Compare --- */

/** _mm256_cmpeq_epi8: compare packed 8-bit integers for equality. */
FIO_IFUNC __m256i fio_fx86_256_cmpeq_epi8(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_cmpeq_epi8(a, b);
#else
  __m256i r;
  for (int i = 0; i < 32; ++i)
    r.u8[i] = (a.u8[i] == b.u8[i]) ? 0xFF : 0x00;
  return r;
#endif
}

/* --- Movemask --- */

/** _mm256_movemask_epi8: create 32-bit mask from MSBs of each byte. */
FIO_IFUNC int fio_fx86_256_movemask_epi8(__m256i a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_movemask_epi8(a);
#else
  uint32_t r = 0;
  for (int i = 0; i < 32; ++i)
    r |= ((uint32_t)((a.u8[i] >> 7) & 1U) << i);
  return (int)r;
#endif
}

/* --- Set --- */

/** _mm256_set1_epi8: broadcast 8-bit integer to all 32 bytes. */
FIO_IFUNC __m256i fio_fx86_256_set1_epi8(char a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_set1_epi8(a);
#else
  __m256i r;
  FIO_MEMSET(&r, (uint8_t)a, 32);
  return r;
#endif
}

/** _mm256_set1_epi16: broadcast 16-bit integer to all 16 lanes. */
FIO_IFUNC __m256i fio_fx86_256_set1_epi16(short a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_set1_epi16(a);
#else
  __m256i r;
  for (int i = 0; i < 16; ++i)
    r.u16[i] = (uint16_t)a;
  return r;
#endif
}

/** _mm256_set1_epi32: broadcast 32-bit integer to all 8 lanes. */
FIO_IFUNC __m256i fio_fx86_256_set1_epi32(int a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_set1_epi32(a);
#else
  __m256i r;
  for (int i = 0; i < 8; ++i)
    r.u32[i] = (uint32_t)a;
  return r;
#endif
}

/** _mm256_set1_epi64x: broadcast 64-bit integer to all 4 lanes. */
FIO_IFUNC __m256i fio_fx86_256_set1_epi64x(long long a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_set1_epi64x(a);
#else
  __m256i r;
  for (int i = 0; i < 4; ++i)
    r.u64[i] = (uint64_t)a;
  return r;
#endif
}

/** _mm256_set_epi64x: set packed 64-bit integers (high to low: e3..e0). */
FIO_IFUNC __m256i fio_fx86_256_set_epi64x(long long e3,
                                          long long e2,
                                          long long e1,
                                          long long e0) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_set_epi64x(e3, e2, e1, e0);
#else
  __m256i r;
  r.u64[0] = (uint64_t)e0;
  r.u64[1] = (uint64_t)e1;
  r.u64[2] = (uint64_t)e2;
  r.u64[3] = (uint64_t)e3;
  return r;
#endif
}

/** _mm256_set_epi8: set packed 8-bit integers (high to low: e31..e0). */
FIO_IFUNC __m256i fio_fx86_256_set_epi8(char e31,
                                        char e30,
                                        char e29,
                                        char e28,
                                        char e27,
                                        char e26,
                                        char e25,
                                        char e24,
                                        char e23,
                                        char e22,
                                        char e21,
                                        char e20,
                                        char e19,
                                        char e18,
                                        char e17,
                                        char e16,
                                        char e15,
                                        char e14,
                                        char e13,
                                        char e12,
                                        char e11,
                                        char e10,
                                        char e9,
                                        char e8,
                                        char e7,
                                        char e6,
                                        char e5,
                                        char e4,
                                        char e3,
                                        char e2,
                                        char e1,
                                        char e0) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_set_epi8(e31,
                         e30,
                         e29,
                         e28,
                         e27,
                         e26,
                         e25,
                         e24,
                         e23,
                         e22,
                         e21,
                         e20,
                         e19,
                         e18,
                         e17,
                         e16,
                         e15,
                         e14,
                         e13,
                         e12,
                         e11,
                         e10,
                         e9,
                         e8,
                         e7,
                         e6,
                         e5,
                         e4,
                         e3,
                         e2,
                         e1,
                         e0);
#else
  __m256i r;
  r.u8[0] = (uint8_t)e0;
  r.u8[1] = (uint8_t)e1;
  r.u8[2] = (uint8_t)e2;
  r.u8[3] = (uint8_t)e3;
  r.u8[4] = (uint8_t)e4;
  r.u8[5] = (uint8_t)e5;
  r.u8[6] = (uint8_t)e6;
  r.u8[7] = (uint8_t)e7;
  r.u8[8] = (uint8_t)e8;
  r.u8[9] = (uint8_t)e9;
  r.u8[10] = (uint8_t)e10;
  r.u8[11] = (uint8_t)e11;
  r.u8[12] = (uint8_t)e12;
  r.u8[13] = (uint8_t)e13;
  r.u8[14] = (uint8_t)e14;
  r.u8[15] = (uint8_t)e15;
  r.u8[16] = (uint8_t)e16;
  r.u8[17] = (uint8_t)e17;
  r.u8[18] = (uint8_t)e18;
  r.u8[19] = (uint8_t)e19;
  r.u8[20] = (uint8_t)e20;
  r.u8[21] = (uint8_t)e21;
  r.u8[22] = (uint8_t)e22;
  r.u8[23] = (uint8_t)e23;
  r.u8[24] = (uint8_t)e24;
  r.u8[25] = (uint8_t)e25;
  r.u8[26] = (uint8_t)e26;
  r.u8[27] = (uint8_t)e27;
  r.u8[28] = (uint8_t)e28;
  r.u8[29] = (uint8_t)e29;
  r.u8[30] = (uint8_t)e30;
  r.u8[31] = (uint8_t)e31;
  return r;
#endif
}

/** _mm256_setzero_si256: return zero vector. */
FIO_IFUNC __m256i fio_fx86_256_setzero_si256(void) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_setzero_si256();
#else
  __m256i r;
  FIO_MEMSET(&r, 0, 32);
  return r;
#endif
}

/* --- Shuffle / Permute --- */

/** _mm256_shuffle_epi8: byte-level shuffle within each 128-bit lane. */
FIO_IFUNC __m256i fio_fx86_256_shuffle_epi8(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_shuffle_epi8(a, b);
#else
  __m256i r;
  /* Low 128-bit lane */
  for (int i = 0; i < 16; ++i) {
    if (b.u8[i] & 0x80)
      r.u8[i] = 0;
    else
      r.u8[i] = a.u8[b.u8[i] & 0x0F];
  }
  /* High 128-bit lane */
  for (int i = 16; i < 32; ++i) {
    if (b.u8[i] & 0x80)
      r.u8[i] = 0;
    else
      r.u8[i] = a.u8[16 + (b.u8[i] & 0x0F)];
  }
  return r;
#endif
}

/** _mm256_alignr_epi8: concatenate and shift within each 128-bit lane. */
FIO_IFUNC __m256i fio_fx86_256_alignr_epi8(__m256i a, __m256i b, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  switch (n) {
  case 8: return _mm256_alignr_epi8(a, b, 8);
  default: {
    /* Per-lane concatenate and shift — pad to 48 for imm8 17..31 safety */
    uint8_t lo[48], hi[48];
    fio_memcpy16(lo, &b);
    fio_memcpy16(lo + 16, &a);
    FIO_MEMSET(lo + 32, 0, 16);
    fio_memcpy16(hi, (const uint8_t *)&b + 16);
    fio_memcpy16(hi + 16, (const uint8_t *)&a + 16);
    FIO_MEMSET(hi + 32, 0, 16);
    __m256i r;
    if (n >= 32) {
      FIO_MEMSET(&r, 0, 32);
    } else {
      fio_memcpy16(&r, lo + n);
      fio_memcpy16((uint8_t *)&r + 16, hi + n);
    }
    return r;
  }
  }
#else
  /* Per-lane: concatenate b_lane:a_lane — pad to 48 for imm8 17..31 safety */
  uint8_t lo[48], hi[48];
  fio_memcpy16(lo, &b);
  fio_memcpy16(lo + 16, &a);
  FIO_MEMSET(lo + 32, 0, 16);
  fio_memcpy16(hi, (const uint8_t *)&b + 16);
  fio_memcpy16(hi + 16, (const uint8_t *)&a + 16);
  FIO_MEMSET(hi + 32, 0, 16);
  __m256i r;
  if (n >= 32) {
    FIO_MEMSET(&r, 0, 32);
  } else {
    fio_memcpy16(&r, lo + n);
    fio_memcpy16((uint8_t *)&r + 16, hi + n);
  }
  return r;
#endif
}

/** _mm256_permute4x64_epi64: permute 64-bit lanes across full 256 bits. */
FIO_IFUNC __m256i fio_fx86_256_permute4x64_epi64(__m256i a, int imm8) {
  uint8_t const n = (uint8_t)imm8;
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  switch (n) {
  case 0x4E: return _mm256_permute4x64_epi64(a, 0x4E);
  case 0xD8: return _mm256_permute4x64_epi64(a, 0xD8);
  default: {
    uint64_t in[4], out[4];
    __m256i r;
    fio_memcpy32(in, &a);
    out[0] = in[(n >> 0) & 3];
    out[1] = in[(n >> 2) & 3];
    out[2] = in[(n >> 4) & 3];
    out[3] = in[(n >> 6) & 3];
    fio_memcpy32(&r, out);
    return r;
  }
  }
#else
  __m256i r;
  r.u64[0] = a.u64[(n >> 0) & 3];
  r.u64[1] = a.u64[(n >> 2) & 3];
  r.u64[2] = a.u64[(n >> 4) & 3];
  r.u64[3] = a.u64[(n >> 6) & 3];
  return r;
#endif
}

/* --- Pack / Convert --- */

/** _mm256_packs_epi32: pack 32-bit to 16-bit with signed saturation. */
FIO_IFUNC __m256i fio_fx86_256_packs_epi32(__m256i a, __m256i b) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_packs_epi32(a, b);
#else
  /* Per 128-bit lane: interleave a_lane and b_lane with saturation.
   * Lane 0: a[0..3] saturated to i16, then b[0..3] saturated to i16.
   * Lane 1: a[4..7] saturated to i16, then b[4..7] saturated to i16. */
  __m256i r;
  for (int lane = 0; lane < 2; ++lane) {
    int base_a = lane * 4;
    int base_b = lane * 4;
    int base_r = lane * 8;
    for (int i = 0; i < 4; ++i) {
      int32_t va = a.i32[base_a + i];
      int32_t vb = b.i32[base_b + i];
      if (va > 32767)
        va = 32767;
      if (va < -32768)
        va = -32768;
      if (vb > 32767)
        vb = 32767;
      if (vb < -32768)
        vb = -32768;
      r.i16[base_r + i] = (int16_t)va;
      r.i16[base_r + 4 + i] = (int16_t)vb;
    }
  }
  return r;
#endif
}

/** _mm256_cvtepi16_epi32: sign-extend 8 packed 16-bit to 32-bit.
 * Takes the low 128 bits of a (8 x int16) and extends to 8 x int32. */
FIO_IFUNC __m256i fio_fx86_256_cvtepi16_epi32(__m128i a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_cvtepi16_epi32(a);
#else
  __m256i r;
  for (int i = 0; i < 8; ++i)
    r.i32[i] = (int32_t)a.i16[i];
  return r;
#endif
}

/* --- Extract / Cast --- */

/** _mm256_castsi256_si128: extract low 128 bits (no instruction, just cast). */
FIO_IFUNC __m128i fio_fx86_256_castsi256_si128(__m256i a) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return _mm256_castsi256_si128(a);
#else
  __m128i r;
  fio_memcpy16(&r, &a);
  return r;
#endif
}

/** _mm256_extracti128_si256: extract 128-bit lane (0=low, 1=high). */
FIO_IFUNC __m128i fio_fx86_256_extracti128_si256(__m256i a, int imm8) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  switch (imm8 & 1) {
  case 0: return _mm256_extracti128_si256(a, 0);
  default: return _mm256_extracti128_si256(a, 1);
  }
#else
  __m128i r;
  fio_memcpy16(&r, (const uint8_t *)&a + ((imm8 & 1) * 16));
  return r;
#endif
}

/* *****************************************************************************
Section I: SHA-512 NI Intrinsics (3 functions)

Intel SHA-512 extensions (Arrow Lake / Lunar Lake 2024+).
These operate on __m256i (4 x 64-bit lanes).
***************************************************************************** */

/**
 * _mm256_sha512rnds2_epi64: perform 2 rounds of SHA-512.
 *
 * State layout: state0 = {a, b, c, d}, state1 = {e, f, g, h}
 * wk contains W[i]+K[i] in the low 128 bits (2 x 64-bit).
 */
FIO_IFUNC __m256i fio_fx86_256_sha512rnds2_epi64(__m256i state1,
                                                 __m256i state0,
                                                 __m256i wk) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA512__)
  return _mm256_sha512rnds2_epi64(state1, state0, wk);
#else
  /* Extract state: state0 = {a, b, c, d}, state1 = {e, f, g, h} */
  uint64_t a = state0.u64[3], b = state0.u64[2];
  uint64_t c = state0.u64[1], d = state0.u64[0];
  uint64_t e = state1.u64[3], f = state1.u64[2];
  uint64_t g = state1.u64[1], h = state1.u64[0];

  for (int i = 0; i < 2; ++i) {
    uint64_t wi = wk.u64[i]; /* W[i] + K[i] already combined */
    /* Sigma1(e) */
    uint64_t s1 = ((e >> 14) | (e << 50)) ^ ((e >> 18) | (e << 46)) ^
                  ((e >> 41) | (e << 23));
    /* Ch(e, f, g) */
    uint64_t ch = (e & f) ^ ((~e) & g);
    /* temp1 */
    uint64_t t1 = h + s1 + ch + wi;
    /* Sigma0(a) */
    uint64_t s0 = ((a >> 28) | (a << 36)) ^ ((a >> 34) | (a << 30)) ^
                  ((a >> 39) | (a << 25));
    /* Maj(a, b, c) */
    uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
    /* temp2 */
    uint64_t t2 = s0 + maj;
    /* Shift state */
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }

  /* Return updated state1 = {e, f, g, h} */
  __m256i r;
  r.u64[3] = e;
  r.u64[2] = f;
  r.u64[1] = g;
  r.u64[0] = h;
  return r;
#endif
}

/**
 * _mm256_sha512msg1_epi64: SHA-512 message schedule part 1.
 * Computes sigma0 on elements from w1 and adds to w0.
 * sigma0(x) = ROTR(1,x) ^ ROTR(8,x) ^ SHR(7,x)
 *
 * Per Intel: dst[i] = w0[i] + sigma0(w1_shifted[i])
 * where the sigma0 inputs come from {w0[1], w0[2], w0[3], w1[0]}.
 */
FIO_IFUNC __m256i fio_fx86_256_sha512msg1_epi64(__m256i w0, __m256i w1) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA512__)
  return _mm256_sha512msg1_epi64(w0, w1);
#else
  __m256i r;
  /* sigma0 inputs: {w0[1], w0[2], w0[3], w1[0]} */
  uint64_t src[4] = {w0.u64[1], w0.u64[2], w0.u64[3], w1.u64[0]};
  for (int i = 0; i < 4; ++i) {
    uint64_t x = src[i];
    uint64_t s0 = ((x >> 1) | (x << 63)) ^ ((x >> 8) | (x << 56)) ^ (x >> 7);
    r.u64[i] = w0.u64[i] + s0;
  }
  return r;
#endif
}

/**
 * _mm256_sha512msg2_epi64: SHA-512 message schedule part 2.
 * Computes sigma1 on elements from w1 and adds to partially computed w0.
 * sigma1(x) = ROTR(19,x) ^ ROTR(61,x) ^ SHR(6,x)
 *
 * Per Intel: uses w1[2], w1[3] for first two, then cascades.
 */
FIO_IFUNC __m256i fio_fx86_256_sha512msg2_epi64(__m256i w0, __m256i w1) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__SHA512__)
  return _mm256_sha512msg2_epi64(w0, w1);
#else
  __m256i r;
  uint64_t w14 = w1.u64[2]; /* W[t-2] */
  uint64_t w15 = w1.u64[3]; /* W[t-1] */

  uint64_t s1_14 =
      ((w14 >> 19) | (w14 << 45)) ^ ((w14 >> 61) | (w14 << 3)) ^ (w14 >> 6);
  r.u64[0] = w0.u64[0] + s1_14;

  uint64_t s1_15 =
      ((w15 >> 19) | (w15 << 45)) ^ ((w15 >> 61) | (w15 << 3)) ^ (w15 >> 6);
  r.u64[1] = w0.u64[1] + s1_15;

  /* Cascade: use newly computed values */
  uint64_t w16 = r.u64[0];
  uint64_t s1_16 =
      ((w16 >> 19) | (w16 << 45)) ^ ((w16 >> 61) | (w16 << 3)) ^ (w16 >> 6);
  r.u64[2] = w0.u64[2] + s1_16;

  uint64_t w17 = r.u64[1];
  uint64_t s1_17 =
      ((w17 >> 19) | (w17 << 45)) ^ ((w17 >> 61) | (w17 << 3)) ^ (w17 >> 6);
  r.u64[3] = w0.u64[3] + s1_17;
  return r;
#endif
}

/* *****************************************************************************
Section J: Shadow Mode

When FIO_FAKE_X86_SHADOW is defined, replace real X86 intrinsic names with
fio_fx86_* implementations via macros. This allows X86 code paths to compile
and run (slowly) on non-X86 systems.
***************************************************************************** */

#ifdef FIO_FAKE_X86_SHADOW

/* Suppress ARM hardware intrinsic paths so fake X86 paths are used instead.
 * arm_neon.h / arm_acle.h are already included (types remain available),
 * but downstream algorithm files will see these macros undefined and skip
 * ARM-accelerated code paths in favor of the shadowed X86 intrinsic paths.
 * Do NOT undef __aarch64__ / __arm__ — they affect struct layout, thread
 * yield, and other non-intrinsic code. */

/* FIO-internal ARM guard macros */
#ifdef FIO___HAS_ARM_INTRIN
#undef FIO___HAS_ARM_INTRIN
#endif
#ifdef FIO___HAS_ARM_SHA3_INTRIN
#undef FIO___HAS_ARM_SHA3_INTRIN
#endif
#ifdef FIO___HAS_ARM_SHA512_INTRIN
#undef FIO___HAS_ARM_SHA512_INTRIN
#endif
#ifdef FIO___HAS_ARM_AES_INTRIN
#undef FIO___HAS_ARM_AES_INTRIN
#endif

/* Compiler-provided ARM feature macros checked in code guards */
#ifdef __ARM_FEATURE_CRYPTO
#undef __ARM_FEATURE_CRYPTO
#endif
#ifdef __ARM_FEATURE_SHA3
#undef __ARM_FEATURE_SHA3
#endif
#ifdef __ARM_FEATURE_SHA512
#undef __ARM_FEATURE_SHA512
#endif
#ifdef __ARM_FEATURE_CRC32
#undef __ARM_FEATURE_CRC32
#endif
#ifdef __ARM_FEATURE_PMULL
#undef __ARM_FEATURE_PMULL
#endif

/* ARM NEON macros */
#ifdef __ARM_NEON
#undef __ARM_NEON
#endif
#ifdef __ARM_NEON__
#undef __ARM_NEON__
#endif

/* --- SSE2 --- */
#define _mm_loadu_si128(p)            fio_fx86_loadu_si128(p)
#define _mm_load_si128(p)             fio_fx86_load_si128(p)
#define _mm_storeu_si128(p, a)        fio_fx86_storeu_si128((p), (a))
#define _mm_add_epi32(a, b)           fio_fx86_add_epi32((a), (b))
#define _mm_xor_si128(a, b)           fio_fx86_xor_si128((a), (b))
#define _mm_or_si128(a, b)            fio_fx86_or_si128((a), (b))
#define _mm_and_si128(a, b)           fio_fx86_and_si128((a), (b))
#define _mm_slli_si128(a, imm8)       fio_fx86_slli_si128((a), (imm8))
#define _mm_srli_si128(a, imm8)       fio_fx86_srli_si128((a), (imm8))
#define _mm_slli_epi32(a, imm8)       fio_fx86_slli_epi32((a), (imm8))
#define _mm_srli_epi32(a, imm8)       fio_fx86_srli_epi32((a), (imm8))
#define _mm_slli_epi64(a, imm8)       fio_fx86_slli_epi64((a), (imm8))
#define _mm_srli_epi64(a, imm8)       fio_fx86_srli_epi64((a), (imm8))
#define _mm_shuffle_epi32(a, imm8)    fio_fx86_shuffle_epi32((a), (imm8))
#define _mm_cmpeq_epi8(a, b)          fio_fx86_cmpeq_epi8((a), (b))
#define _mm_movemask_epi8(a)          fio_fx86_movemask_epi8(a)
#define _mm_set_epi32(e3, e2, e1, e0) fio_fx86_set_epi32((e3), (e2), (e1), (e0))
#define _mm_set_epi64x(e1, e0)        fio_fx86_set_epi64x((e1), (e0))
#define _mm_set_epi8(e15,                                                      \
                     e14,                                                      \
                     e13,                                                      \
                     e12,                                                      \
                     e11,                                                      \
                     e10,                                                      \
                     e9,                                                       \
                     e8,                                                       \
                     e7,                                                       \
                     e6,                                                       \
                     e5,                                                       \
                     e4,                                                       \
                     e3,                                                       \
                     e2,                                                       \
                     e1,                                                       \
                     e0)                                                       \
  fio_fx86_set_epi8((e15),                                                     \
                    (e14),                                                     \
                    (e13),                                                     \
                    (e12),                                                     \
                    (e11),                                                     \
                    (e10),                                                     \
                    (e9),                                                      \
                    (e8),                                                      \
                    (e7),                                                      \
                    (e6),                                                      \
                    (e5),                                                      \
                    (e4),                                                      \
                    (e3),                                                      \
                    (e2),                                                      \
                    (e1),                                                      \
                    (e0))
#define _mm_set1_epi8(a)     fio_fx86_set1_epi8(a)
#define _mm_setzero_si128()  fio_fx86_setzero_si128()
#define _mm_cvtsi32_si128(a) fio_fx86_cvtsi32_si128(a)

/* --- SSSE3 --- */
#define _mm_shuffle_epi8(a, b)      fio_fx86_shuffle_epi8((a), (b))
#define _mm_alignr_epi8(a, b, imm8) fio_fx86_alignr_epi8((a), (b), (imm8))

/* --- SSE4.1 --- */
#define _mm_blend_epi16(a, b, imm8) fio_fx86_blend_epi16((a), (b), (imm8))
#define _mm_extract_epi32(a, imm8)  fio_fx86_extract_epi32((a), (imm8))

/* --- Prefetch --- */
#define _mm_prefetch(p, hint) fio_fx86_prefetch((p), (hint))

/* --- AES-NI --- */
#define _mm_aesenc_si128(a, rk)     fio_fx86_aesenc_si128((a), (rk))
#define _mm_aesenclast_si128(a, rk) fio_fx86_aesenclast_si128((a), (rk))
#define _mm_aeskeygenassist_si128(a, imm8)                                     \
  fio_fx86_aeskeygenassist_si128((a), (imm8))

/* --- PCLMULQDQ --- */
#define _mm_clmulepi64_si128(a, b, imm8)                                       \
  fio_fx86_clmulepi64_si128((a), (b), (imm8))

/* --- SHA-NI (SHA-256) --- */
#define _mm_sha256rnds2_epu32(cdgh, abef, msg)                                 \
  fio_fx86_sha256rnds2_epu32((cdgh), (abef), (msg))
#define _mm_sha256msg1_epu32(a, b) fio_fx86_sha256msg1_epu32((a), (b))
#define _mm_sha256msg2_epu32(a, b) fio_fx86_sha256msg2_epu32((a), (b))

/* --- SHA-NI (SHA-1) --- */
#define _mm_sha1rnds4_epu32(abcd, e0, func)                                    \
  fio_fx86_sha1rnds4_epu32((abcd), (e0), (func))
#define _mm_sha1nexte_epu32(a, b) fio_fx86_sha1nexte_epu32((a), (b))
#define _mm_sha1msg1_epu32(a, b)  fio_fx86_sha1msg1_epu32((a), (b))
#define _mm_sha1msg2_epu32(a, b)  fio_fx86_sha1msg2_epu32((a), (b))

/* --- AVX2 --- */
#define _mm256_loadu_si256(p)      fio_fx86_256_loadu_si256(p)
#define _mm256_load_si256(p)       fio_fx86_256_load_si256(p)
#define _mm256_storeu_si256(p, a)  fio_fx86_256_storeu_si256((p), (a))
#define _mm256_add_epi16(a, b)     fio_fx86_256_add_epi16((a), (b))
#define _mm256_sub_epi16(a, b)     fio_fx86_256_sub_epi16((a), (b))
#define _mm256_mullo_epi16(a, b)   fio_fx86_256_mullo_epi16((a), (b))
#define _mm256_add_epi32(a, b)     fio_fx86_256_add_epi32((a), (b))
#define _mm256_sub_epi32(a, b)     fio_fx86_256_sub_epi32((a), (b))
#define _mm256_mullo_epi32(a, b)   fio_fx86_256_mullo_epi32((a), (b))
#define _mm256_add_epi64(a, b)     fio_fx86_256_add_epi64((a), (b))
#define _mm256_sub_epi64(a, b)     fio_fx86_256_sub_epi64((a), (b))
#define _mm256_slli_epi32(a, imm8) fio_fx86_256_slli_epi32((a), (imm8))
#define _mm256_srai_epi32(a, imm8) fio_fx86_256_srai_epi32((a), (imm8))
#define _mm256_xor_si256(a, b)     fio_fx86_256_xor_si256((a), (b))
#define _mm256_and_si256(a, b)     fio_fx86_256_and_si256((a), (b))
#define _mm256_or_si256(a, b)      fio_fx86_256_or_si256((a), (b))
#define _mm256_cmpeq_epi8(a, b)    fio_fx86_256_cmpeq_epi8((a), (b))
#define _mm256_movemask_epi8(a)    fio_fx86_256_movemask_epi8(a)
#define _mm256_set1_epi8(a)        fio_fx86_256_set1_epi8(a)
#define _mm256_set1_epi16(a)       fio_fx86_256_set1_epi16(a)
#define _mm256_set1_epi32(a)       fio_fx86_256_set1_epi32(a)
#define _mm256_set1_epi64x(a)      fio_fx86_256_set1_epi64x(a)
#define _mm256_set_epi64x(e3, e2, e1, e0)                                      \
  fio_fx86_256_set_epi64x((e3), (e2), (e1), (e0))
#define _mm256_set_epi8(e31,                                                   \
                        e30,                                                   \
                        e29,                                                   \
                        e28,                                                   \
                        e27,                                                   \
                        e26,                                                   \
                        e25,                                                   \
                        e24,                                                   \
                        e23,                                                   \
                        e22,                                                   \
                        e21,                                                   \
                        e20,                                                   \
                        e19,                                                   \
                        e18,                                                   \
                        e17,                                                   \
                        e16,                                                   \
                        e15,                                                   \
                        e14,                                                   \
                        e13,                                                   \
                        e12,                                                   \
                        e11,                                                   \
                        e10,                                                   \
                        e9,                                                    \
                        e8,                                                    \
                        e7,                                                    \
                        e6,                                                    \
                        e5,                                                    \
                        e4,                                                    \
                        e3,                                                    \
                        e2,                                                    \
                        e1,                                                    \
                        e0)                                                    \
  fio_fx86_256_set_epi8((e31),                                                 \
                        (e30),                                                 \
                        (e29),                                                 \
                        (e28),                                                 \
                        (e27),                                                 \
                        (e26),                                                 \
                        (e25),                                                 \
                        (e24),                                                 \
                        (e23),                                                 \
                        (e22),                                                 \
                        (e21),                                                 \
                        (e20),                                                 \
                        (e19),                                                 \
                        (e18),                                                 \
                        (e17),                                                 \
                        (e16),                                                 \
                        (e15),                                                 \
                        (e14),                                                 \
                        (e13),                                                 \
                        (e12),                                                 \
                        (e11),                                                 \
                        (e10),                                                 \
                        (e9),                                                  \
                        (e8),                                                  \
                        (e7),                                                  \
                        (e6),                                                  \
                        (e5),                                                  \
                        (e4),                                                  \
                        (e3),                                                  \
                        (e2),                                                  \
                        (e1),                                                  \
                        (e0))
#define _mm256_setzero_si256()    fio_fx86_256_setzero_si256()
#define _mm256_shuffle_epi8(a, b) fio_fx86_256_shuffle_epi8((a), (b))
#define _mm256_alignr_epi8(a, b, imm8)                                         \
  fio_fx86_256_alignr_epi8((a), (b), (imm8))
#define _mm256_permute4x64_epi64(a, imm8)                                      \
  fio_fx86_256_permute4x64_epi64((a), (imm8))
#define _mm256_packs_epi32(a, b)  fio_fx86_256_packs_epi32((a), (b))
#define _mm256_cvtepi16_epi32(a)  fio_fx86_256_cvtepi16_epi32(a)
#define _mm256_castsi256_si128(a) fio_fx86_256_castsi256_si128(a)
#define _mm256_extracti128_si256(a, imm8)                                      \
  fio_fx86_256_extracti128_si256((a), (imm8))

/* --- SHA-512 NI --- */
#define _mm256_sha512rnds2_epi64(s1, s0, wk)                                   \
  fio_fx86_256_sha512rnds2_epi64((s1), (s0), (wk))
#define _mm256_sha512msg1_epi64(w0, w1)                                        \
  fio_fx86_256_sha512msg1_epi64((w0), (w1))
#define _mm256_sha512msg2_epi64(w0, w1)                                        \
  fio_fx86_256_sha512msg2_epi64((w0), (w1))

/* When shadowing, also define the feature-detection macros so that
 * guarded code paths are compiled. */
#ifndef FIO___HAS_X86_INTRIN
#define FIO___HAS_X86_INTRIN 1
#endif
#ifndef __SSE2__
#define __SSE2__ 1
#endif
#ifndef __SSSE3__
#define __SSSE3__ 1
#endif
#ifndef __SSE4_1__
#define __SSE4_1__ 1
#endif
#ifndef __SSE4_2__
#define __SSE4_2__ 1
#endif
#ifndef __AES__
#define __AES__ 1
#endif
#ifndef __PCLMUL__
#define __PCLMUL__ 1
#endif
#ifndef __SHA__
#define __SHA__ 1
#endif
#ifndef __SHA512__
#define __SHA512__ 1
#endif
#ifndef __AVX2__
#define __AVX2__ 1
#endif
#ifndef FIO___HAS_X86_SHA_INTRIN
#define FIO___HAS_X86_SHA_INTRIN 1
#endif
#ifndef FIO___HAS_X86_SHA512_INTRIN
#define FIO___HAS_X86_SHA512_INTRIN 1
#endif
#ifndef FIO___HAS_X86_AES_INTRIN
#define FIO___HAS_X86_AES_INTRIN 1
#endif

#endif /* FIO_FAKE_X86_SHADOW */

#endif /* FIO_FAKE_X86 && !H___FIO_FAKE_X86___H */
