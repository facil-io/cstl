/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                    Alternatives to memcpy, memchr etc'



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_MEMALT) && !defined(H___FIO_MEMALT___H)
#define H___FIO_MEMALT___H 1

/* *****************************************************************************
Memory Helpers - API
***************************************************************************** */

/**
 * A somewhat naive implementation of `memset`.
 *
 * Probably slower than the one included with your compiler's C library.
 */
SFUNC void *fio_memset(void *restrict dest, uint64_t data, size_t bytes);

/**
 * A somewhat naive implementation of `memcpy`.
 *
 * Probably slower than the one included with your compiler's C library.
 */
SFUNC void *fio_memcpy(void *dest_, const void *src_, size_t bytes);

/**
 * A token seeking function. This is a fallback for `memchr`, but `memchr`
 * should be faster.
 */
SFUNC void *fio_memchr(const void *buffer, const char token, size_t len);

/**
 * A comparison function. This is a fallback for `memcmp`, but `memcmp`
 * should be faster.
 */
SFUNC int fio_memcmp(const void *a_, const void *b_, size_t len);

/** An alternative to `strlen` - may raise Address Sanitation errors. */
SFUNC size_t fio_strlen(const char *str);

/* *****************************************************************************
Alternatives - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
FIO_MEMCPY / fio_memcpy - memcpy fallback
***************************************************************************** */

/** an unsafe memcpy (no checks + assumes no overlapping memory regions)*/
FIO_SFUNC void *fio___memcpy_buffered_x(void *restrict d_,
                                        const void *restrict s_,
                                        size_t l) {
  char *restrict d = (char *restrict)d_;
  const char *restrict s = (const char *restrict)s_;
  uint64_t t[8] FIO_ALIGN(16);
  while (l > 63) {
    fio_memcpy64(t, s);
    FIO_COMPILER_GUARD_INSTRUCTION;
    fio_memcpy64(d, t);
    l -= 64;
    d += 64;
    s += 64;
  }
#define FIO___MEMCPY_UNSAFE_STEP(bytes)                                        \
  do {                                                                         \
    fio_memcpy##bytes(t, s);                                                   \
    FIO_COMPILER_GUARD_INSTRUCTION;                                            \
    fio_memcpy##bytes(d, t);                                                   \
    (d += bytes), (s += bytes);                                                \
  } while (0)
  if (l & 32)
    FIO___MEMCPY_UNSAFE_STEP(32);
  if ((l & 16))
    FIO___MEMCPY_UNSAFE_STEP(16);
  if ((l & 8))
    FIO___MEMCPY_UNSAFE_STEP(8);
  if ((l & 4))
    FIO___MEMCPY_UNSAFE_STEP(4);
  if ((l & 2))
    FIO___MEMCPY_UNSAFE_STEP(2);
#undef FIO___MEMCPY_UNSAFE_STEP
  if ((l & 1))
    *d++ = *s;

  return (void *)d;
}

/** an unsafe memcpy (no checks + assumes no overlapping memory regions)*/
FIO_SFUNC void *fio___memcpy_buffered_reversed_x(void *d_,
                                                 const void *s_,
                                                 size_t l) {
  char *d = (char *)d_ + l;
  const char *s = (const char *)s_ + l;
  uint64_t t[8] FIO_ALIGN(16);
  while (l > 63) {
    (s -= 64), (d -= 64), (l -= 64);
    fio_memcpy64(t, s);
    FIO_COMPILER_GUARD_INSTRUCTION;
    fio_memcpy64(d, t);
  }
  if ((l & 32)) {
    (d -= 32), (s -= 32);
    fio_memcpy32(t, s);
    FIO_COMPILER_GUARD_INSTRUCTION;
    fio_memcpy32(d, t);
  }
  if ((l & 16)) {
    (d -= 16), (s -= 16);
    fio_memcpy16(t, s);
    FIO_COMPILER_GUARD_INSTRUCTION;
    fio_memcpy16(d, t);
  }
  if ((l & 8)) {
    (d -= 8), (s -= 8);
    fio_memcpy8(t, s);
    FIO_COMPILER_GUARD_INSTRUCTION;
    fio_memcpy8(d, t);
  }
  if ((l & 4)) {
    (d -= 4), (s -= 4);
    fio_memcpy4(t, s);
    fio_memcpy4(d, t);
  }
  if ((l & 2)) {
    (d -= 2), (s -= 2);
    fio_memcpy2(t, s);
    fio_memcpy2(d, t);
  }
  if ((l & 1))
    *--d = *--s;
  return (void *)d;
}

#define FIO___MEMCPY_BLOCKx_NUM 255ULL

/** memcpy / memmove alternative that should work with unaligned memory */
SFUNC void *fio_memcpy(void *dest_, const void *src_, size_t bytes) {
  char *d = (char *)dest_;
  const char *s = (const char *)src_;

  if (FIO_UNLIKELY((d == s) | !bytes | !d | !s)) {
    if (bytes && (d != s))
      FIO_LOG_DEBUG2("fio_memcpy null error - ignored instruction");
    return d;
  }

  /* Fast path for small, non-overlapping copies (<=64 bytes) */
  if (bytes <= 64 && (d + bytes <= s || s + bytes <= d)) {
    if (bytes <= 8) {
      /* Handle 1-8 bytes with overlapping copies */
      if (bytes >= 4) {
        fio_memcpy4(d, s);
        fio_memcpy4(d + bytes - 4, s + bytes - 4);
      } else if (bytes >= 2) {
        fio_memcpy2(d, s);
        fio_memcpy2(d + bytes - 2, s + bytes - 2);
      } else {
        *d = *s;
      }
      return dest_;
    }
    if (bytes <= 16) {
      /* 9-16 bytes: two overlapping 8-byte copies */
      fio_memcpy8(d, s);
      fio_memcpy8(d + bytes - 8, s + bytes - 8);
      return dest_;
    }
    if (bytes <= 32) {
      /* 17-32 bytes: two overlapping 16-byte copies */
      fio_memcpy16(d, s);
      fio_memcpy16(d + bytes - 16, s + bytes - 16);
      return dest_;
    }
    /* 33-64 bytes: two overlapping 32-byte copies */
    fio_memcpy32(d, s);
    fio_memcpy32(d + bytes - 32, s + bytes - 32);
    return dest_;
  }

  /* Existing path for larger copies or overlapping memory */
  if (s + bytes <= d || d + bytes <= s ||
      (uintptr_t)d + FIO___MEMCPY_BLOCKx_NUM < (uintptr_t)s) {
    return fio___memcpy_unsafe_x(d, s, bytes);
  } else if (d < s) { /* memory overlaps at end (copy forward, use buffer) */
    return fio___memcpy_buffered_x(d, s, bytes);
  } else { /* memory overlaps at beginning, walk backwards (memmove) */
    return fio___memcpy_buffered_reversed_x(d, s, bytes);
  }
  return d;
}

#undef FIO___MEMCPY_BLOCKx_NUM

/* *****************************************************************************
FIO_MEMSET / fio_memset - memset fallbacks
***************************************************************************** */

/** an 8 byte value memset implementation. */
SFUNC void *fio_memset(void *restrict dest_, uint64_t data, size_t bytes) {
  char *d = (char *)dest_;
  if (data < 0x100) { /* if a single byte value, match memset */
    data |= (data << 8);
    data |= (data << 16);
    data |= (data << 32);
  }
  if (FIO_UNLIKELY(bytes < 32))
    goto small_memset;

  /* 32 byte groups */
  for (;;) {
    for (size_t i = 0; i < 32; i += 8) {
      fio_memcpy8(d + i, &data);
    }
    bytes -= 32;
    if (bytes < 32)
      break;
    d += 32;
  }
  /* remainder  */
  d += bytes & 31;
  data = fio_frot64(data, ((bytes & 7) << 3));
  for (size_t i = 0; i < 32; i += 8) {
    fio_memcpy8(d + i, &data);
  }
  return dest_;

small_memset:
  if (bytes & 16) {
    fio_memcpy8(d, &data);
    fio_memcpy8(d + 8, &data);
    d += 16;
  }
  if (bytes & 8) {
    fio_memcpy8(d, &data);
    d += 8;
  }
  fio_memcpy7x(d, &data, bytes);
  return dest_;
}

/* *****************************************************************************
FIO_MEMCHR / fio_memchr - memchr fallbacks
***************************************************************************** */

/* Scalar implementation - fallback for all platforms */
FIO_SFUNC void *fio___memchr_scalar(const void *buffer,
                                    const char token,
                                    size_t len) {
  const char *r = (const char *)buffer;
  const char *e = r + (len - 127);
  uint64_t u[16] FIO_ALIGN(16) = {0};
  uint64_t flag = 0;
  size_t i;
  uint64_t umsk = ((uint64_t)((uint8_t)token));
  umsk |= (umsk << 32); /* make each byte in umsk == token */
  umsk |= (umsk << 16);
  umsk |= (umsk << 8);
  if (FIO_UNLIKELY(len < 8))
    goto small_memchr;
  while (r < e) {
    fio_memcpy128(u, r);
    for (i = 0; i < 16; ++i) {
      u[i] ^= umsk;
      flag |= (u[i] = fio_has_zero_byte64(u[i]));
    }
    if (flag)
      goto found_in_map;
    r += 128;
  }
  e += 120;
  i = 0;
  while (r < e) {
    fio_memcpy8(u, r);
    u[0] ^= umsk;
    flag = fio_has_zero_byte64(u[0]);
    if (flag)
      goto found_in_8;
    r += 8;
  }
small_memchr:
  switch ((len & 7)) { /* clang-format off */
    case 7: if (*r == token) return (void *)r; ++r; /* fall through */
    case 6: if (*r == token) return (void *)r; ++r; /* fall through */
    case 5: if (*r == token) return (void *)r; ++r; /* fall through */
    case 4: if (*r == token) return (void *)r; ++r; /* fall through */
    case 3: if (*r == token) return (void *)r; ++r; /* fall through */
    case 2: if (*r == token) return (void *)r; ++r; /* fall through */
    case 1: if (*r == token) return (void *)r; ++r;
    } /* clang-format on */
  return NULL;
found_in_map:
  flag = 0;
  for (i = 0; !u[i]; ++i)
    ;
  flag = u[i];
  r += i << 3;
found_in_8:
  flag = fio_has_byte2bitmap(flag);
  return (void *)(r + fio_lsb_index_unsafe(flag));
}

/* *****************************************************************************
FIO_MEMCHR - SIMD implementations
***************************************************************************** */

#if defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
/* SSE2 implementation - 16 bytes at a time */
FIO_SFUNC void *fio___memchr_sse2(const void *buffer,
                                  const char token,
                                  size_t len) {
  const uint8_t *r = (const uint8_t *)buffer;
  const uint8_t *e = r + len;
  /* Broadcast token to all 16 bytes */
  const __m128i needle = _mm_set1_epi8(token);

  /* Handle unaligned head - process byte by byte until aligned */
  while (((uintptr_t)r & 15) && r < e) {
    if (*r == (uint8_t)token)
      return (void *)r;
    ++r;
  }
  if (r >= e)
    return NULL;

  /* Main loop: process 64 bytes (4x16) at a time for better throughput */
  const uint8_t *e64 = e - 63;
  while (r < e64) {
    __m128i v0 = _mm_load_si128((const __m128i *)(r));
    __m128i v1 = _mm_load_si128((const __m128i *)(r + 16));
    __m128i v2 = _mm_load_si128((const __m128i *)(r + 32));
    __m128i v3 = _mm_load_si128((const __m128i *)(r + 48));
    __m128i cmp0 = _mm_cmpeq_epi8(v0, needle);
    __m128i cmp1 = _mm_cmpeq_epi8(v1, needle);
    __m128i cmp2 = _mm_cmpeq_epi8(v2, needle);
    __m128i cmp3 = _mm_cmpeq_epi8(v3, needle);
    /* Combine results to check if any match found */
    __m128i or01 = _mm_or_si128(cmp0, cmp1);
    __m128i or23 = _mm_or_si128(cmp2, cmp3);
    __m128i orall = _mm_or_si128(or01, or23);
    int mask = _mm_movemask_epi8(orall);
    if (mask) {
      /* Found a match - determine which 16-byte chunk */
      int m0 = _mm_movemask_epi8(cmp0);
      if (m0)
        return (void *)(r + __builtin_ctz(m0));
      int m1 = _mm_movemask_epi8(cmp1);
      if (m1)
        return (void *)(r + 16 + __builtin_ctz(m1));
      int m2 = _mm_movemask_epi8(cmp2);
      if (m2)
        return (void *)(r + 32 + __builtin_ctz(m2));
      int m3 = _mm_movemask_epi8(cmp3);
      return (void *)(r + 48 + __builtin_ctz(m3));
    }
    r += 64;
  }

  /* Process remaining 16-byte chunks */
  const uint8_t *e16 = e - 15;
  while (r < e16) {
    __m128i v = _mm_load_si128((const __m128i *)r);
    __m128i cmp = _mm_cmpeq_epi8(v, needle);
    int mask = _mm_movemask_epi8(cmp);
    if (mask)
      return (void *)(r + __builtin_ctz(mask));
    r += 16;
  }

  /* Handle tail bytes */
  while (r < e) {
    if (*r == (uint8_t)token)
      return (void *)r;
    ++r;
  }
  return NULL;
}
#endif /* SSE2 */

#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
/* AVX2 implementation - 32 bytes at a time */
FIO_SFUNC void *fio___memchr_avx2(const void *buffer,
                                  const char token,
                                  size_t len) {
  const uint8_t *r = (const uint8_t *)buffer;
  const uint8_t *e = r + len;
  /* Broadcast token to all 32 bytes */
  const __m256i needle = _mm256_set1_epi8(token);

  /* Handle unaligned head - process byte by byte until 32-byte aligned */
  while (((uintptr_t)r & 31) && r < e) {
    if (*r == (uint8_t)token)
      return (void *)r;
    ++r;
  }
  if (r >= e)
    return NULL;

  /* Main loop: process 128 bytes (4x32) at a time for better throughput */
  const uint8_t *e128 = e - 127;
  while (r < e128) {
    __m256i v0 = _mm256_load_si256((const __m256i *)(r));
    __m256i v1 = _mm256_load_si256((const __m256i *)(r + 32));
    __m256i v2 = _mm256_load_si256((const __m256i *)(r + 64));
    __m256i v3 = _mm256_load_si256((const __m256i *)(r + 96));
    __m256i cmp0 = _mm256_cmpeq_epi8(v0, needle);
    __m256i cmp1 = _mm256_cmpeq_epi8(v1, needle);
    __m256i cmp2 = _mm256_cmpeq_epi8(v2, needle);
    __m256i cmp3 = _mm256_cmpeq_epi8(v3, needle);
    /* Combine results to check if any match found */
    __m256i or01 = _mm256_or_si256(cmp0, cmp1);
    __m256i or23 = _mm256_or_si256(cmp2, cmp3);
    __m256i orall = _mm256_or_si256(or01, or23);
    int mask = _mm256_movemask_epi8(orall);
    if (mask) {
      /* Found a match - determine which 32-byte chunk */
      int m0 = _mm256_movemask_epi8(cmp0);
      if (m0)
        return (void *)(r + __builtin_ctz(m0));
      int m1 = _mm256_movemask_epi8(cmp1);
      if (m1)
        return (void *)(r + 32 + __builtin_ctz(m1));
      int m2 = _mm256_movemask_epi8(cmp2);
      if (m2)
        return (void *)(r + 64 + __builtin_ctz(m2));
      int m3 = _mm256_movemask_epi8(cmp3);
      return (void *)(r + 96 + __builtin_ctz(m3));
    }
    r += 128;
  }

  /* Process remaining 32-byte chunks */
  const uint8_t *e32 = e - 31;
  while (r < e32) {
    __m256i v = _mm256_load_si256((const __m256i *)r);
    __m256i cmp = _mm256_cmpeq_epi8(v, needle);
    int mask = _mm256_movemask_epi8(cmp);
    if (mask)
      return (void *)(r + __builtin_ctz(mask));
    r += 32;
  }

  /* Handle tail bytes - fall through to SSE2 or scalar for remaining */
  while (r < e) {
    if (*r == (uint8_t)token)
      return (void *)r;
    ++r;
  }
  return NULL;
}
#endif /* AVX2 */

#if defined(FIO___HAS_ARM_INTRIN)
/* ARM NEON implementation - 16 bytes at a time */
FIO_SFUNC void *fio___memchr_neon(const void *buffer,
                                  const char token,
                                  size_t len) {
  const uint8_t *r = (const uint8_t *)buffer;
  const uint8_t *e = r + len;
  /* Broadcast token to all 16 bytes */
  const uint8x16_t needle = vdupq_n_u8((uint8_t)token);

  /* Handle unaligned head - process byte by byte until aligned */
  while (((uintptr_t)r & 15) && r < e) {
    if (*r == (uint8_t)token)
      return (void *)r;
    ++r;
  }
  if (r >= e)
    return NULL;

  /* Main loop: process 64 bytes (4x16) at a time for better throughput */
  const uint8_t *e64 = e - 63;
  while (r < e64) {
    uint8x16_t v0 = vld1q_u8(r);
    uint8x16_t v1 = vld1q_u8(r + 16);
    uint8x16_t v2 = vld1q_u8(r + 32);
    uint8x16_t v3 = vld1q_u8(r + 48);
    uint8x16_t cmp0 = vceqq_u8(v0, needle);
    uint8x16_t cmp1 = vceqq_u8(v1, needle);
    uint8x16_t cmp2 = vceqq_u8(v2, needle);
    uint8x16_t cmp3 = vceqq_u8(v3, needle);
    /* Combine results using OR to check if any match found */
    uint8x16_t or01 = vorrq_u8(cmp0, cmp1);
    uint8x16_t or23 = vorrq_u8(cmp2, cmp3);
    uint8x16_t orall = vorrq_u8(or01, or23);
    /* Check if any byte is non-zero (match found) */
    uint64x2_t orall64 = vreinterpretq_u64_u8(orall);
    if (vgetq_lane_u64(orall64, 0) | vgetq_lane_u64(orall64, 1)) {
      /* Found a match - determine which 16-byte chunk and position */
      uint64x2_t cmp0_64 = vreinterpretq_u64_u8(cmp0);
      if (vgetq_lane_u64(cmp0_64, 0) | vgetq_lane_u64(cmp0_64, 1)) {
        /* Match in first chunk - find exact position */
        for (size_t i = 0; i < 16; ++i)
          if (r[i] == (uint8_t)token)
            return (void *)(r + i);
      }
      uint64x2_t cmp1_64 = vreinterpretq_u64_u8(cmp1);
      if (vgetq_lane_u64(cmp1_64, 0) | vgetq_lane_u64(cmp1_64, 1)) {
        for (size_t i = 0; i < 16; ++i)
          if (r[16 + i] == (uint8_t)token)
            return (void *)(r + 16 + i);
      }
      uint64x2_t cmp2_64 = vreinterpretq_u64_u8(cmp2);
      if (vgetq_lane_u64(cmp2_64, 0) | vgetq_lane_u64(cmp2_64, 1)) {
        for (size_t i = 0; i < 16; ++i)
          if (r[32 + i] == (uint8_t)token)
            return (void *)(r + 32 + i);
      }
      for (size_t i = 0; i < 16; ++i)
        if (r[48 + i] == (uint8_t)token)
          return (void *)(r + 48 + i);
    }
    r += 64;
  }

  /* Process remaining 16-byte chunks */
  const uint8_t *e16 = e - 15;
  while (r < e16) {
    uint8x16_t v = vld1q_u8(r);
    uint8x16_t cmp = vceqq_u8(v, needle);
    uint64x2_t cmp64 = vreinterpretq_u64_u8(cmp);
    if (vgetq_lane_u64(cmp64, 0) | vgetq_lane_u64(cmp64, 1)) {
      for (size_t i = 0; i < 16; ++i)
        if (r[i] == (uint8_t)token)
          return (void *)(r + i);
    }
    r += 16;
  }

  /* Handle tail bytes */
  while (r < e) {
    if (*r == (uint8_t)token)
      return (void *)r;
    ++r;
  }
  return NULL;
}
#endif /* ARM NEON */

/**
 * A token seeking function. This is a fallback for `memchr`, but `memchr`
 * should be faster.
 */
SFUNC void *fio_memchr(const void *buffer, const char token, size_t len) {
#if defined(FIO___HAS_X86_INTRIN) && defined(__AVX2__)
  return fio___memchr_avx2(buffer, token, len);
#elif defined(FIO___HAS_X86_INTRIN) && defined(__SSE2__)
  return fio___memchr_sse2(buffer, token, len);
#elif defined(FIO___HAS_ARM_INTRIN)
  return fio___memchr_neon(buffer, token, len);
#else
  return fio___memchr_scalar(buffer, token, len);
#endif
}

/* *****************************************************************************
fio_strlen
***************************************************************************** */

SFUNC FIO___ASAN_AVOID size_t fio_strlen(const char *str) {
  if (FIO_UNLIKELY(!str))
    return 0;
  uintptr_t start = (uintptr_t)str;
  /* we must align memory, to avoid crushing when nearing last page boundary */
  uint64_t flag = 0;
  uint64_t map[8] FIO_ALIGN(16);
  /* align to 8 bytes - most likely skipped */
  switch (start & 7) { // clang-format off
  case 1: if(*str == 0) return (uintptr_t)str - start; ++str; /* fall through */
  case 2: if(*str == 0) return (uintptr_t)str - start; ++str; /* fall through */
  case 3: if(*str == 0) return (uintptr_t)str - start; ++str; /* fall through */
  case 4: if(*str == 0) return (uintptr_t)str - start; ++str; /* fall through */
  case 5: if(*str == 0) return (uintptr_t)str - start; ++str; /* fall through */
  case 6: if(*str == 0) return (uintptr_t)str - start; ++str; /* fall through */
  case 7: if(*str == 0) return (uintptr_t)str - start; ++str;
  } // clang-format on
  /* align to 64 bytes */
  for (size_t i = 0; i < 9; ++i) {
    if ((flag = fio_has_zero_byte64(*(uint64_t *)str)))
      goto found_nul_byte0;
    str += 8;
  }
  str = FIO_PTR_MATH_RMASK(const char, str, 6); /* compiler hint */
  /* loop endlessly */
  for (;;) {
    for (size_t i = 0; i < 8; ++i) {
      flag |= (map[i] = fio_has_zero_byte64(((uint64_t *)str)[i]));
    }
    if (flag)
      goto found_nul_byte8;
    str += 64;
  }

found_nul_byte8:
  flag = 0;
  for (size_t i = 0; i < 8; ++i) {
    map[i] = fio_has_byte2bitmap(map[i]);
    flag |= (map[i] << (i << 3)); /* pack bitmap */
  }
  str += fio_lsb_index_unsafe(flag);
  return (uintptr_t)str - start;

found_nul_byte0:
  str += fio_lsb_index_unsafe(fio_has_byte2bitmap(flag));
  return (uintptr_t)str - start;
}

/* *****************************************************************************
fio_memcmp
***************************************************************************** */

/** Same as `memcmp`. Returns 1 if `a > b`, -1 if `a < b` and 0 if `a == b`. */
SFUNC int fio_memcmp(const void *a_, const void *b_, size_t len) {
  if (FIO_UNLIKELY(a_ == b_ || !len))
    return 0;
  uint64_t ua[8] FIO_ALIGN(16);
  uint64_t ub[8] FIO_ALIGN(16);
  size_t flag = 0;
  char *a = (char *)a_;
  char *b = (char *)b_;
  char *e;
  if (*a != *b)
    return (int)1 - (int)(((unsigned)b[0] > (unsigned)a[0]) << 1);
  if (FIO_UNLIKELY(len < 8))
    goto fio_memcmp_mini;
  if (FIO_UNLIKELY(len < 64))
    goto fio_memcmp_small;

  e = a + len - 63;
  do {
    fio_memcpy64(ua, a);
    fio_memcpy64(ub, b);
    for (size_t i = 0; i < 8; ++i)
      flag |= (ua[i] ^ ub[i]);
    if (flag)
      goto fio_memcmp_found;
    a += 64;
    b += 64;
  } while (a < e);
  a += len & 63;
  b += len & 63;
  a -= 64;
  b -= 64;
  fio_memcpy64(ua, a);
  fio_memcpy64(ub, b);
  for (size_t i = 0; i < 8; ++i)
    flag |= (ua[i] ^ ub[i]);
  if (flag)
    goto fio_memcmp_found;
  return 0;

fio_memcmp_found:
  if (ua[0] == ub[0])
    for (size_t i = 8; --i;)
      if (ua[i] != ub[i]) {
        ua[0] = ua[i];
        ub[0] = ub[i];
      }
  goto fio_memcmp_small_found;

fio_memcmp_small:
  e = a + len - 7;
  do {
    fio_memcpy8(ua, a);
    fio_memcpy8(ub, b);
    if (ua[0] != ub[0])
      goto fio_memcmp_small_found;
    a += 8;
    b += 8;
  } while (a < e);
  a += len & 7;
  b += len & 7;
  a -= 8;
  b -= 8;
  fio_memcpy8(ua, a);
  fio_memcpy8(ub, b);
  if (ua[0] != ub[0])
    goto fio_memcmp_small_found;
  return 0;

fio_memcmp_small_found:
  ua[0] = fio_lton64(ua[0]);
  ub[0] = fio_lton64(ub[0]);
  return (int)1 - (int)((ub[0] > ua[0]) << 1);

fio_memcmp_mini:
  switch ((len & 7)) { /* clang-format off */
    case 7: if (*a != *b) return (int)1 - (int)(((unsigned)b[0] > (unsigned)a[0]) << 1); ++a; ++b; /* fall through */
    case 6: if (*a != *b) return (int)1 - (int)(((unsigned)b[0] > (unsigned)a[0]) << 1); ++a; ++b; /* fall through */
    case 5: if (*a != *b) return (int)1 - (int)(((unsigned)b[0] > (unsigned)a[0]) << 1); ++a; ++b; /* fall through */
    case 4: if (*a != *b) return (int)1 - (int)(((unsigned)b[0] > (unsigned)a[0]) << 1); ++a; ++b; /* fall through */
    case 3: if (*a != *b) return (int)1 - (int)(((unsigned)b[0] > (unsigned)a[0]) << 1); ++a; ++b; /* fall through */
    case 2: if (*a != *b) return (int)1 - (int)(((unsigned)b[0] > (unsigned)a[0]) << 1); ++a; ++b; /* fall through */
    case 1: if (*a != *b) return (int)1 - (int)(((unsigned)b[0] > (unsigned)a[0]) << 1); ++a; ++b;
    } /* clang-format on */
  return 0;
}

/* *****************************************************************************
Alternatives - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN */
#endif /* FIO_MEMALT */
