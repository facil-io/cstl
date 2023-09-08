/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_LOCK2              /* Development inclusion - ignore line */
#define FIO_ATOMIC             /* Development inclusion - ignore line */
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

  if ((d == s) | !bytes | !d | !s) {
    FIO_LOG_DEBUG2("fio_memcpy null error - ignored instruction");
    return d;
  }

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
  if (bytes < 32)
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

#define FIO___MEMCHR_BITMAP_TEST(group_size)                                   \
  do {                                                                         \
    uint64_t flag = 0, v, u[group_size];                                       \
    for (size_t i = 0; i < group_size; ++i) { /* partial math */               \
      fio_memcpy8(u + i, r + (i << 3));                                        \
      u[i] ^= umsk;                            /* byte match == 0x00 */        \
      v = u[i] - UINT64_C(0x0101010101010101); /* v: less than 0x80 => 0x80 */ \
      u[i] = ~u[i]; /* u[i]: if the MSB was zero (less than 0x80) */           \
      u[i] &= UINT64_C(0x8080808080808080);                                    \
      u[i] &= v; /* only 0x00 will now be 0x80  */                             \
      flag |= u[i];                                                            \
    }                                                                          \
    if (FIO_LIKELY(!flag)) {                                                   \
      r += (group_size << 3);                                                  \
      break; /* from do..while macro */                                        \
    }                                                                          \
    flag = 0;                                                                  \
    for (size_t i = 0; i < group_size; ++i) { /* combine group to bitmap  */   \
      u[i] = fio_has_byte2bitmap(u[i]);                                        \
      flag |= (u[i] << (i << 3)); /* placed packed bitmap in u64 */            \
    }                                                                          \
    return (void *)(r + fio_lsb_index_unsafe(flag));                           \
  } while (0)

/* the small fio_memchr - poor SIMD, used for up to 64 bytes */
FIO_SFUNC void *fio_memchr_small(const void *buffer,
                                 const char token,
                                 size_t len) {
  const char *r = (const char *)buffer;
  uint64_t umsk = ((uint64_t)((uint8_t)token));
  umsk |= (umsk << 32); /* make each byte in umsk == token */
  umsk |= (umsk << 16);
  umsk |= (umsk << 8);
  if (len > 15) {
    for (;;) {
      len -= 16;
      FIO___MEMCHR_BITMAP_TEST(2);
      if (!len)
        return NULL;
      if (len > 15)
        continue;
      r -= 16;
      r += len & 15;
      len = 16;
    }
  }
  if (len > 7) {
    FIO___MEMCHR_BITMAP_TEST(1);
    r -= 8;
    r += len & 7;
    FIO___MEMCHR_BITMAP_TEST(1);
    return NULL;
  }
  /* clang-format off */
  switch(len) {
  case 7: if (*r == token) return (void *)r; ++r; /* fall through */
  case 6: if (*r == token) return (void *)r; ++r; /* fall through */
  case 5: if (*r == token) return (void *)r; ++r; /* fall through */
  case 4: if (*r == token) return (void *)r; ++r; /* fall through */
  case 3: if (*r == token) return (void *)r; ++r; /* fall through */
  case 2: if (*r == token) return (void *)r; ++r; /* fall through */
  case 1: if (*r == token) return (void *)r; ++r;
  }
  /* clang-format on */
  return NULL;
}
/**
 * A token seeking function. This is a fallback for `memchr`, but `memchr`
 * should be faster.
 */
SFUNC void *fio_memchr(const void *buffer, const char token, size_t len) {
  if (!buffer || !len)
    return NULL;
  if (len < 64)
    return fio_memchr_small(buffer, token, len);
  const char *r = (const char *)buffer;
  uint64_t umsk = ((uint64_t)((uint8_t)token));
  umsk |= (umsk << 32); /* make each byte in umsk == token */
  umsk |= (umsk << 16);
  umsk |= (umsk << 8);

#if FIO_LIMIT_INTRINSIC_BUFFER
  for (const char *const e = r + (len & (~UINT64_C(127))); r < e;) {
    FIO___MEMCHR_BITMAP_TEST(8);
    FIO___MEMCHR_BITMAP_TEST(8);
  }
#else
  for (const char *const e = r + (len & (~UINT64_C(255))); r < e;) {
    FIO___MEMCHR_BITMAP_TEST(8);
    FIO___MEMCHR_BITMAP_TEST(8);
    FIO___MEMCHR_BITMAP_TEST(8);
    FIO___MEMCHR_BITMAP_TEST(8);
  }
  if ((len & 128)) {
    FIO___MEMCHR_BITMAP_TEST(8);
    FIO___MEMCHR_BITMAP_TEST(8);
  }
#endif
  if ((len & 64)) {
    FIO___MEMCHR_BITMAP_TEST(8);
  }
  if ((len & 32)) {
    FIO___MEMCHR_BITMAP_TEST(4);
  }
  r -= 32;
  r += len & 31;
  FIO___MEMCHR_BITMAP_TEST(4);
  return NULL;
}

#undef FIO___MEMCHR_BITMAP_TEST
#define FIO___MEMCHR_BITMAP_TEST(group_size)                                   \
  do {                                                                         \
    uint64_t flag = 0, v, u[group_size];                                       \
    for (size_t i = 0; i < group_size; ++i) {  /* per 8 byte group */          \
      u[i] = uptr.u64[i];                      /* avoid fio_memcpy8 (ASAN) */  \
      u[i] ^= umsk;                            /* byte match == 0x00 */        \
      v = u[i] - UINT64_C(0x0101010101010101); /* v: less than 0x80 => 0x80 */ \
      u[i] = ~u[i]; /* u[i]: if the MSB was zero (less than 0x80) */           \
      u[i] &= UINT64_C(0x8080808080808080);                                    \
      u[i] &= v; /* only 0x00 will now be 0x80  */                             \
      flag |= u[i];                                                            \
    }                                                                          \
    if (FIO_LIKELY(!flag)) {                                                   \
      uptr.u64 += group_size;                                                  \
      break; /* from do..while macro */                                        \
    }                                                                          \
    flag = 0;                                                                  \
    for (size_t i = 0; i < group_size; ++i) { /* combine group to bitmap  */   \
      u[i] = fio_has_byte2bitmap(u[i]);                                        \
      flag |= (u[i] << (i << 3)); /* placed packed bitmap in u64 */            \
    }                                                                          \
    return (void *)(uptr.i8 + fio_lsb_index_unsafe(flag));                     \
  } while (0)

/** A token seeking function. */
FIO_SFUNC FIO___ASAN_AVOID void *fio_rawmemchr(const void *buffer,
                                               const char token) {
  if (!buffer)
    return NULL;

  union {
    const char *i8;
    const uint64_t *u64;
  } uptr = {.i8 = (const char *)buffer};

  /* we must align memory, to avoid crushing when nearing last page boundary */
  switch (((uintptr_t)uptr.i8 & 7)) {
#define FIO___MEMCHR_UNSAFE_STEP()                                             \
  if (*uptr.i8 == token)                                                       \
    return (void *)uptr.i8;                                                    \
  ++uptr.i8
  case 1: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 2: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 3: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 4: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 5: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 6: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 7: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
#undef FIO___MEMCHR_UNSAFE_STEP
  }
  uint64_t umsk = ((uint64_t)((uint8_t)token));
  umsk |= (umsk << 32); /* make each byte in umsk == token */
  umsk |= (umsk << 16);
  umsk |= (umsk << 8);

  for (size_t aligner = 0; aligner < 8; ++aligner)
    FIO___MEMCHR_BITMAP_TEST(1);
  uptr.i8 = FIO_PTR_MATH_RMASK(const char, uptr.i8, 5); /* loop alignment */
  for (size_t aligner = 0; aligner < 5; ++aligner)
    FIO___MEMCHR_BITMAP_TEST(4);
  uptr.i8 = FIO_PTR_MATH_RMASK(const char, uptr.i8, 7); /* loop alignment */
  for (;;) {
    FIO___MEMCHR_BITMAP_TEST(8);
    FIO___MEMCHR_BITMAP_TEST(8);
  }
}
#undef FIO___MEMCHR_BITMAP_TEST

/* *****************************************************************************
fio_strlen
***************************************************************************** */

SFUNC FIO___ASAN_AVOID size_t fio_strlen(const char *str) {
  // const char *nul = (const char *)fio_rawmemchr(str, 0);
  // return (size_t)(nul - str);
  if (!str)
    return 0;
  uintptr_t start = (uintptr_t)str;
  /* we must align memory, to avoid crushing when nearing last page boundary */
  switch ((start & 7)) {
#define FIO___MEMCHR_UNSAFE_STEP()                                             \
  if (!str[0])                                                                 \
    return (uintptr_t)str - start;                                             \
  ++str
  case 1: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 2: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 3: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 4: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 5: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 6: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
  case 7: FIO___MEMCHR_UNSAFE_STEP(); /* fall through */
#undef FIO___MEMCHR_UNSAFE_STEP
  }

  /* 8 byte aligned */
  uint64_t flag = 0;
  uint64_t map[8] FIO_ALIGN(16) = {0};
  uint64_t tmp[8] FIO_ALIGN(16) = {0};

#define FIO___STRLEN_CYCLE(i)                                                  \
  do {                                                                         \
    map[i] = (*(const uint64_t *)(str + (i << 3)));                            \
    tmp[i] =                                                                   \
        map[i] - UINT64_C(0x0101010101010101); /* is 0 or >= 0x80 --> 0x8X */  \
    map[i] = ~map[i];                          /* is < 0x80) --> 0x8X */       \
    map[i] &= UINT64_C(0x8080808080808080);                                    \
    map[i] &= tmp[i]; /* only 0x00 will now be 0x80  */                        \
    flag |= map[i];                                                            \
  } while (0)

  for (size_t aligner = 0; aligner < 8; ++aligner) {
    FIO___STRLEN_CYCLE(0);
    if (flag)
      goto found_nul_byte0;
    str += 8;
  }
  str = FIO_PTR_MATH_RMASK(const char, str, 6); /* new loop alignment */
  for (;;) { /* loop while aligned on 64 byte boundary */
    for (size_t i = 0; i < 8; ++i)
      FIO___STRLEN_CYCLE(i);
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
  flag = fio_has_byte2bitmap(map[0]);
  str += fio_lsb_index_unsafe(flag);
  return (uintptr_t)str - start;
}

/* *****************************************************************************
fio_memcmp
***************************************************************************** */
FIO_SFUNC int fio___memcmp_mini(char *restrict a,
                                char *restrict b,
                                size_t len) {
  uint64_t ua = 0, ub = 0;
  fio_memcpy7x(&ua, a, len);
  fio_memcpy7x(&ub, b, len);
  ua = fio_lton64(ua); /* fix cmp order */
  ub = fio_lton64(ub);
  if (ua != ub)
    return (int)1 - (int)((ub > ua) << 1);
  return 0;
}

#define FIO___MEMCMP_BYTES(bytes, test_for_non_even)                           \
  /** Compares at least bytes and no more than `len` byte long buffers. */     \
  FIO_IFUNC int fio___memcmp##bytes(char *restrict a,                          \
                                    char *restrict b,                          \
                                    size_t len) {                              \
    uint64_t ua[bytes / 8] FIO_ALIGN(16);                                      \
    uint64_t ub[bytes / 8] FIO_ALIGN(16);                                      \
    uint64_t flag = 0;                                                         \
    if (!test_for_non_even || (len & (bytes - 1))) {                           \
      for (size_t i = 0; i < (bytes / 8); ++i) {                               \
        fio_memcpy8(ua + i, a + (i << 3));                                     \
        fio_memcpy8(ub + i, b + (i << 3));                                     \
        flag |= (ua[i] ^ ub[i]);                                               \
      }                                                                        \
      if (flag)                                                                \
        goto review_diff;                                                      \
      a += len & (bytes - 1);                                                  \
      b += len & (bytes - 1);                                                  \
      len -= len & (bytes - 1);                                                \
    }                                                                          \
    do {                                                                       \
      for (size_t i = 0; i < (bytes / 8); ++i) {                               \
        fio_memcpy8(ua + i, a + (i << 3));                                     \
        fio_memcpy8(ub + i, b + (i << 3));                                     \
        flag |= (ua[i] ^ ub[i]);                                               \
      }                                                                        \
      if (flag)                                                                \
        goto review_diff;                                                      \
      len -= bytes;                                                            \
      a += bytes;                                                              \
      b += bytes;                                                              \
    } while (len);                                                             \
    return 0;                                                                  \
  review_diff:                                                                 \
    for (size_t i = ((bytes / 8) - 1); i--;) {                                 \
      if (ua[i] != ub[i]) {                                                    \
        ua[(bytes / 8) - 1] = ua[i];                                           \
        ub[(bytes / 8) - 1] = ub[i];                                           \
      }                                                                        \
    }                                                                          \
    ua[(bytes / 8) - 1] = fio_lton64(ua[(bytes / 8) - 1]); /* fix cmp order */ \
    ub[(bytes / 8) - 1] = fio_lton64(ub[(bytes / 8) - 1]);                     \
    return (int)1 - (int)((ub[(bytes / 8) - 1] > ua[(bytes / 8) - 1]) << 1);   \
  }

FIO___MEMCMP_BYTES(8, 0)
FIO___MEMCMP_BYTES(16, 0)
FIO___MEMCMP_BYTES(32, 0)
FIO___MEMCMP_BYTES(64, 1)
FIO___MEMCMP_BYTES(128, 1)
FIO___MEMCMP_BYTES(256, 1)

/** Same as `memcmp`. Returns 1 if `a > b`, -1 if `a < b` and 0 if `a == b`. */
SFUNC int fio_memcmp(const void *a_, const void *b_, size_t len) {
  if (a_ == b_ || !len)
    return 0;
  char *a = (char *)a_;
  char *b = (char *)b_;
  if (len < 8)
    return fio___memcmp_mini(a, b, len);
  if (len < 16)
    return fio___memcmp8(a, b, len);
  if (len < 32)
    return fio___memcmp16(a, b, len);
  if (len < 1024)
    return fio___memcmp32(a, b, len);
  if (len < 2048)
    return fio___memcmp64(a, b, len);
#if FIO_LIMIT_INTRINSIC_BUFFER
  return fio___memcmp128(a, b, len);
#else
  if (len < 4096)
    return fio___memcmp128(a, b, len);
  return fio___memcmp256(a, b, len);
#endif /* FIO_LIMIT_INTRINSIC_BUFFER */
}

/* *****************************************************************************
Alternatives - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN */
#endif /* FIO_MEMALT */
