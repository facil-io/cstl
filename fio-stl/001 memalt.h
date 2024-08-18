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
  uint64_t t[8] FIO_ALIGN(64);
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
  uint64_t t[8] FIO_ALIGN(64);
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
    if (bytes && (d != s))
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

/**
 * A token seeking function. This is a fallback for `memchr`, but `memchr`
 * should be faster.
 */
SFUNC void *fio_memchr(const void *buffer, const char token, size_t len) {
  // return (void *)memchr(buffer, token, len); /* FIXME */
  const char *r = (const char *)buffer;
  const char *e = r + (len - 127);
  uint64_t u[16] FIO_ALIGN(64) = {0};
  uint64_t flag = 0;
  size_t i;
  uint64_t umsk = ((uint64_t)((uint8_t)token));
  umsk |= (umsk << 32); /* make each byte in umsk == token */
  umsk |= (umsk << 16);
  umsk |= (umsk << 8);
  if (len < 8)
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
fio_strlen
***************************************************************************** */

SFUNC FIO___ASAN_AVOID size_t fio_strlen(const char *str) {
  if (!str)
    return 0;
  uintptr_t start = (uintptr_t)str;
  /* we must align memory, to avoid crushing when nearing last page boundary */
  uint64_t flag = 0;
  uint64_t map[8] FIO_ALIGN(64);
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
  str = FIO_PTR_MATH_RMASK(const char, str, 6);
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
  if (a_ == b_ || !len)
    return 0;
  uint64_t ua[8] FIO_ALIGN(64);
  uint64_t ub[8] FIO_ALIGN(64);
  size_t flag = 0;
  char *a = (char *)a_;
  char *b = (char *)b_;
  char *e;
  if (*a != *b)
    return (int)1 - (int)(((unsigned)b[0] > (unsigned)a[0]) << 1);
  if (len < 8)
    goto fio_memcmp_mini;
  if (len < 64)
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
