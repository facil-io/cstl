/* *****************************************************************************
C++ extern start
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
extern "C" {
/* C++ keyword was deprecated */
#ifndef register
#define register
#endif
/* C keyword - unavailable in C++ */
#ifndef restrict
#define restrict
#endif
#endif

/* *****************************************************************************




                            Constants (included once)




***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE___H
#define H___FIO_CSTL_INCLUDE_ONCE___H

/* *****************************************************************************
Aligned Memory Access
***************************************************************************** */

#ifndef FIO_UNALIGNED_ACCESS
/** Allows facil.io to attempt unaligned memory access on *some* CPU systems. */
#define FIO_UNALIGNED_ACCESS 1
#endif

#ifndef FIO_UNALIGNED_MEMORY_ACCESS_ENABLED
#if FIO_UNALIGNED_ACCESS &&                                                    \
    (__amd64 || __amd64__ || __x86_64 || __x86_64__ || __i386 ||               \
     __aarch64__ || _M_IX86 || _M_X64 || _M_ARM64 || __ARM_FEATURE_UNALIGNED)
/** True when unaligned memory is allowed. */
#define FIO_UNALIGNED_MEMORY_ACCESS_ENABLED 1
#else
#define FIO_UNALIGNED_MEMORY_ACCESS_ENABLED 0
#endif
#endif /* FIO_UNALIGNED_MEMORY_ACCESS_ENABLED */

/* *****************************************************************************
Compiler detection, GCC / CLang features and OS dependent included files
***************************************************************************** */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if !defined(__GNUC__) && !defined(__clang__) && !defined(GNUC_BYPASS)
#define __attribute__(...)
#define __has_include(...) 0
#define __has_builtin(...) 0
#define GNUC_BYPASS        1
#elif !defined(__clang__) && !defined(__has_builtin)
/* E.g: GCC < 6.0 doesn't support __has_builtin */
#define __has_builtin(...) 0
#define GNUC_BYPASS        1
#endif

#ifndef __has_include
#define __has_include(...) 0
#define GNUC_BYPASS 1
#endif

#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5))
/* GCC < 4.5 doesn't support deprecation reason string */
#define DEPRECATED(reason) __attribute__((deprecated))
#else
#define DEPRECATED(reason) __attribute__((deprecated(reason)))
#endif

#if defined(__GNUC__) || defined(__clang__)
#define FIO_ALIGN(bytes) __attribute__((aligned(bytes)))
#elif defined(__INTEL_COMPILER) || defined(_MSC_VER)
#define FIO_ALIGN(bytes) __declspec(align(bytes))
#else
#define FIO_ALIGN(bytes)
#endif

#if _MSC_VER
#define inline __inline
#define __thread __declspec(thread)
#elif !defined(__clang__) && !defined(__GNUC__)
#define __thread _Thread_local
#endif

#if defined(__clang__) || defined(__GNUC__)
/** Clobber CPU registers and prevent compiler reordering optimizations. */
#define FIO_COMPILER_GUARD __asm__ volatile("" ::: "memory")
#elif defined(_MSC_VER)
#include <intrin.h>
/** Clobber CPU registers and prevent compiler reordering optimizations. */
#define FIO_COMPILER_GUARD _ReadWriteBarrier()
#pragma message("Warning: Windows deprecated it's low-level C memory barrier.")
#else
#warning Unknown OS / compiler, some macros are poorly defined and errors might occur.
#define FIO_COMPILER_GUARD asm volatile("" ::: "memory")
#endif

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#define FIO_HAVE_UNIX_TOOLS 1
#define FIO_OS_POSIX        1
#define FIO___PRINTF_STYLE  printf
#elif defined(_WIN32) || defined(_WIN64) || defined(WIN32) ||                  \
    defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#define FIO_OS_WIN     1
#define POSIX_C_SOURCE 200809L
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#if defined(__MINGW32__)
/* Mingw supports */
#define FIO_HAVE_UNIX_TOOLS    2
#define __USE_MINGW_ANSI_STDIO 1
#define FIO___PRINTF_STYLE     __MINGW_PRINTF_FORMAT
#elif defined(__CYGWIN__)
/* TODO: cygwin support */
#define FIO_HAVE_UNIX_TOOLS    3
#define __USE_MINGW_ANSI_STDIO 1
#define FIO___PRINTF_STYLE     __MINGW_PRINTF_FORMAT
#else
#define FIO_HAVE_UNIX_TOOLS 0
typedef SSIZE_T ssize_t;
#endif /* __CYGWIN__ __MINGW32__ */
#else
#define FIO_HAVE_UNIX_TOOLS 0
#warning Unknown OS / compiler, some macros are poorly defined and errors might occur.
#endif

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif

#if FIO_HAVE_UNIX_TOOLS
#include <sys/param.h>
#include <unistd.h>
#endif

/* *****************************************************************************
Function Attributes
***************************************************************************** */

/** Marks a function as `static`, `inline` and possibly unused. */
#define FIO_IFUNC static inline __attribute__((unused))

/** Marks a function as `static` and possibly unused. */
#define FIO_SFUNC static __attribute__((unused))

/** Marks a function as weak */
#define FIO_WEAK __attribute__((weak))

#if _MSC_VER
#pragma section(".CRT$XCU", read)
#undef FIO_CONSTRUCTOR
#undef FIO_DESTRUCTOR
/** Marks a function as a constructor - if supported. */

#if _WIN64 /* MSVC linker uses different name mangling on 32bit systems */
#define FIO___CONSTRUCTOR_INTERNAL(fname)                                      \
  static void fname(void);                                                     \
  __pragma(comment(linker, "/include:" #fname "__")); /* and next.... */       \
  __declspec(allocate(".CRT$XCU")) void (*fname##__)(void) = fname;            \
  static void fname(void)
#else
#define FIO___CONSTRUCTOR_INTERNAL(fname)                                      \
  static void fname(void);                                                     \
  __declspec(allocate(".CRT$XCU")) void (*fname##__)(void) = fname;            \
  __pragma(comment(linker, "/include:_" #fname "__")); /* and next.... */      \
  static void fname(void)
#endif
#define FIO_CONSTRUCTOR(fname) FIO___CONSTRUCTOR_INTERNAL(fname)

#define FIO_DESTRUCTOR_INTERNAL(fname)                                         \
  static void fname(void);                                                     \
  FIO_CONSTRUCTOR(fname##__hook) { atexit(fname); }                            \
  static void fname(void)
#define FIO_DESTRUCTOR(fname) FIO_DESTRUCTOR_INTERNAL(fname)

#else
/** Marks a function as a constructor - if supported. */
#define FIO_CONSTRUCTOR(fname)                                                 \
  static __attribute__((constructor)) void fname(void)

/** Marks a function as a destructor - if supported. Consider using atexit() */
#define FIO_DESTRUCTOR(fname)                                                  \
  static __attribute__((destructor)) void fname(void)
#endif

/* *****************************************************************************
Static Endian Test
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
#define FIO_LITTLE_ENDIAN_TEST 0x31323334UL
#define FIO_BIG_ENDIAN_TEST    0x34333231UL
#define FIO_ENDIAN_ORDER_TEST  ('1234')
#if ENDIAN_ORDER_TEST == LITTLE_ENDIAN_TEST
#define __BIG_ENDIAN__    0
#define __LITTLE_ENDIAN__ 1
#elif ENDIAN_ORDER_TEST == BIG_ENDIAN_TEST
#define __BIG_ENDIAN__    1
#define __LITTLE_ENDIAN__ 0
#else
#error Could not detect byte order on this system.
#endif

#endif /* predefined / test endianess */

/* *****************************************************************************
Dynamic Endian Testing
***************************************************************************** */

FIO_IFUNC unsigned int fio_is_little_endian(void) {
  union {
    unsigned long ul;
    unsigned char u8[sizeof(unsigned long)];
  } u = {.ul = 1};
  return (unsigned int)u.u8[0];
}

FIO_IFUNC size_t fio_is_big_endian(void) { return !fio_is_little_endian(); }

/* *****************************************************************************
Memory Copying Primitives
***************************************************************************** */

#ifndef FIO_MEMCPYX_UNROLL
/** If set, manual unrolling is used for the fio___memcpy7x helper. */
#define FIO_MEMCPYX_UNROLL 1
#endif
#if __has_builtin(__builtin_memset)
#ifndef FIO_MEMSET
#define FIO_MEMSET __builtin_memset
#endif
#else
#ifndef FIO_MEMSET
#define FIO_MEMSET memset
#endif
#endif

/* memcpy selectors / overriding */
#if __has_builtin(__builtin_memcpy)
#ifndef FIO_MEMCPY
#define FIO_MEMCPY __builtin_memcpy
#endif
#define FIO_MEMCPY1(dest, src)    __builtin_memcpy((dest), (src), 1)
#define FIO_MEMCPY2(dest, src)    __builtin_memcpy((dest), (src), 2)
#define FIO_MEMCPY4(dest, src)    __builtin_memcpy((dest), (src), 4)
#define FIO_MEMCPY8(dest, src)    __builtin_memcpy((dest), (src), 8)
#define FIO_MEMCPY16(dest, src)   __builtin_memcpy((dest), (src), 16)
#define FIO_MEMCPY32(dest, src)   __builtin_memcpy((dest), (src), 32)
#define FIO_MEMCPY64(dest, src)   __builtin_memcpy((dest), (src), 64)
#define FIO_MEMCPY128(dest, src)  __builtin_memcpy((dest), (src), 128)
#define FIO_MEMCPY256(dest, src)  __builtin_memcpy((dest), (src), 256)
#define FIO_MEMCPY512(dest, src)  __builtin_memcpy((dest), (src), 512)
#define FIO_MEMCPY1024(dest, src) __builtin_memcpy((dest), (src), 1024)
#define FIO_MEMCPY2048(dest, src) __builtin_memcpy((dest), (src), 2048)
#define FIO_MEMCPY4096(dest, src) __builtin_memcpy((dest), (src), 4096)
#define FIO_MEMCPY___PARTIAL(bytes)                                            \
  if ((l & bytes)) {                                                           \
    __builtin_memcpy(d, s, bytes);                                             \
    d += bytes;                                                                \
    s += bytes;                                                                \
  }
#else
#ifndef FIO_MEMCPY
#define FIO_MEMCPY memcpy
#endif
#define FIO_MEMCPY1(dest, src)    fio___memcpy1((dest), (src))
#define FIO_MEMCPY2(dest, src)    fio___memcpy2((dest), (src))
#define FIO_MEMCPY4(dest, src)    fio___memcpy4((dest), (src))
#define FIO_MEMCPY8(dest, src)    fio___memcpy8((dest), (src))
#define FIO_MEMCPY16(dest, src)   fio___memcpy16((dest), (src))
#define FIO_MEMCPY32(dest, src)   fio___memcpy32((dest), (src))
#define FIO_MEMCPY64(dest, src)   fio___memcpy64((dest), (src))
#define FIO_MEMCPY128(dest, src)  fio___memcpy128((dest), (src))
#define FIO_MEMCPY256(dest, src)  fio___memcpy256((dest), (src))
#define FIO_MEMCPY512(dest, src)  fio___memcpy512((dest), (src))
#define FIO_MEMCPY1024(dest, src) fio___memcpy1024((dest), (src))
#define FIO_MEMCPY2048(dest, src) fio___memcpy2048((dest), (src))
#define FIO_MEMCPY4096(dest, src) fio___memcpy4096((dest), (src))
#define FIO_MEMCPY___PARTIAL(bytes)                                            \
  if ((l & bytes)) {                                                           \
    fio___memcpy##bytes(d, s);                                                 \
    d += bytes;                                                                \
    s += bytes;                                                                \
  }

#define FIO___MAKE_MEMCPY_FIXED(bytes)                                         \
  FIO_IFUNC void fio___memcpy##bytes(void *restrict dest,                      \
                                     const void *restrict src) {               \
    struct fio___memcpy##bytes##_s {                                           \
      unsigned char data[bytes];                                               \
    };                                                                         \
    union {                                                                    \
      const void *restrict ptr;                                                \
      struct fio___memcpy##bytes##_s *restrict grp;                            \
    } d = {.ptr = dest}, s = {.ptr = src};                                     \
    *d.grp = *s.grp;                                                           \
  }
FIO___MAKE_MEMCPY_FIXED(1)
FIO___MAKE_MEMCPY_FIXED(2)
FIO___MAKE_MEMCPY_FIXED(4)
FIO___MAKE_MEMCPY_FIXED(8)
FIO___MAKE_MEMCPY_FIXED(16)
FIO___MAKE_MEMCPY_FIXED(32)
FIO___MAKE_MEMCPY_FIXED(64)
FIO___MAKE_MEMCPY_FIXED(128)
FIO___MAKE_MEMCPY_FIXED(256)
FIO___MAKE_MEMCPY_FIXED(512)
FIO___MAKE_MEMCPY_FIXED(1024)
FIO___MAKE_MEMCPY_FIXED(2048)
FIO___MAKE_MEMCPY_FIXED(4096)
#undef FIO___MAKE_MEMCPY_FIXED

#endif /* __has_builtin(__builtin_memcpy) */

#define FIO_MEMCPY7x(dest, src, len)    fio___memcpy7x((dest), (src), (len))
#define FIO_MEMCPY15x(dest, src, len)   fio___memcpy15x((dest), (src), (len))
#define FIO_MEMCPY31x(dest, src, len)   fio___memcpy31x((dest), (src), (len))
#define FIO_MEMCPY63x(dest, src, len)   fio___memcpy63x((dest), (src), (len))
#define FIO_MEMCPY127x(dest, src, len)  fio___memcpy127x((dest), (src), (len))
#define FIO_MEMCPY255x(dest, src, len)  fio___memcpy255x((dest), (src), (len))
#define FIO_MEMCPY4095x(dest, src, len) fio___memcpy4095x((dest), (src), (len))

/** Copies up to 7 bytes to `dest` from `src`, calculated by `len & 7`. */
FIO_IFUNC void fio___memcpy7x(void *restrict d_,
                              const void *restrict s_,
                              size_t l) {
  char *restrict d = (char *)d_;
  char *restrict s = (char *)s_;
#if FIO_MEMCPYX_UNROLL
  FIO_MEMCPY___PARTIAL(4);
  FIO_MEMCPY___PARTIAL(2);
  if ((l & 1))
    *d = *s;
#else
  l &= 7;
  while (l--) {
    *(d++) = *(s++);
  }
#endif
}

/** Copies up to 15 bytes to `dest` from `src`, calculated by `len & 15`. */
FIO_IFUNC void fio___memcpy15x(void *restrict dest_,
                               const void *restrict src_,
                               size_t l) {
  char *restrict d = (char *)dest_;
  const char *restrict s = (const char *)src_;
  FIO_MEMCPY___PARTIAL(8);
  fio___memcpy7x(d, s, l);
}
/** Copies up to 31 bytes to `dest` from `src`, calculated by `len & 31`. */
FIO_IFUNC void fio___memcpy31x(void *restrict dest_,
                               const void *restrict src_,
                               size_t l) {
  char *restrict d = (char *)dest_;
  const char *restrict s = (const char *)src_;
  FIO_MEMCPY___PARTIAL(16);
  FIO_MEMCPY___PARTIAL(8);
  fio___memcpy7x(d, s, l);
}
/** Copies up to 63 bytes to `dest` from `src`, calculated by `len & 63`. */
FIO_IFUNC void fio___memcpy63x(void *restrict dest_,
                               const void *restrict src_,
                               size_t l) {
  char *restrict d = (char *)dest_;
  const char *restrict s = (const char *)src_;
  FIO_MEMCPY___PARTIAL(32);
  FIO_MEMCPY___PARTIAL(16);
  FIO_MEMCPY___PARTIAL(8);
  fio___memcpy7x(d, s, l);
}
/** Copies up to 127 bytes to `dest` from `src`, calculated by `len & 127`. */
FIO_IFUNC void fio___memcpy127x(void *restrict dest_,
                                const void *restrict src_,
                                size_t l) {
  char *restrict d = (char *)dest_;
  const char *restrict s = (const char *)src_;
  FIO_MEMCPY___PARTIAL(64);
  FIO_MEMCPY___PARTIAL(32);
  FIO_MEMCPY___PARTIAL(16);
  FIO_MEMCPY___PARTIAL(8);
  fio___memcpy7x(d, s, l);
}
/** Copies up to 255 bytes to `dest` from `src`, calculated by `len & 255`. */
FIO_IFUNC void fio___memcpy255x(void *restrict dest_,
                                const void *restrict src_,
                                size_t l) {
  char *restrict d = (char *)dest_;
  const char *restrict s = (const char *)src_;
  FIO_MEMCPY___PARTIAL(128);
  FIO_MEMCPY___PARTIAL(64);
  FIO_MEMCPY___PARTIAL(32);
  FIO_MEMCPY___PARTIAL(16);
  FIO_MEMCPY___PARTIAL(8);
  fio___memcpy7x(d, s, l);
}
/** Copies up to 4095 bytes to `dest` from `src`, calculated by `len & 4095`. */
FIO_IFUNC void fio___memcpy4095x(void *restrict dest_,
                                 const void *restrict src_,
                                 size_t l) {
  char *restrict d = (char *)dest_;
  const char *restrict s = (const char *)src_;
  FIO_MEMCPY___PARTIAL(2048);
  FIO_MEMCPY___PARTIAL(1024);
  FIO_MEMCPY___PARTIAL(512);
  FIO_MEMCPY___PARTIAL(256);
  FIO_MEMCPY___PARTIAL(128);
  FIO_MEMCPY___PARTIAL(64);
  FIO_MEMCPY___PARTIAL(32);
  FIO_MEMCPY___PARTIAL(16);
  FIO_MEMCPY___PARTIAL(8);
  fio___memcpy7x(d, s, l);
}
#undef FIO_MEMCPY___PARTIAL
/* *****************************************************************************
Conditional Likelihood
***************************************************************************** */
#if defined(__clang__) || defined(__GNUC__)
#define FIO_LIKELY(cond)   __builtin_expect((cond), 1)
#define FIO_UNLIKELY(cond) __builtin_expect((cond), 0)
#else
#define FIO_LIKELY(cond)   (cond)
#define FIO_UNLIKELY(cond) (cond)
#endif

/* *****************************************************************************
Macro Stringifier
***************************************************************************** */
#ifndef FIO_MACRO2STR
#define FIO_MACRO2STR_STEP2(macro) #macro
/** Converts a macro's content to a string literal. */
#define FIO_MACRO2STR(macro) FIO_MACRO2STR_STEP2(macro)
#endif

/* *****************************************************************************
Naming Macros
***************************************************************************** */
/* Used for naming functions and types */
#define FIO_NAME_FROM_MACRO_STEP2(prefix, postfix, div) prefix##div##postfix
#define FIO_NAME_FROM_MACRO_STEP1(prefix, postfix, div)                        \
  FIO_NAME_FROM_MACRO_STEP2(prefix, postfix, div)

/** Used for naming functions and variables resulting in: prefix_postfix */
#define FIO_NAME(prefix, postfix) FIO_NAME_FROM_MACRO_STEP1(prefix, postfix, _)

/** Sets naming convention for conversion functions, i.e.: foo2bar */
#define FIO_NAME2(prefix, postfix) FIO_NAME_FROM_MACRO_STEP1(prefix, postfix, 2)

/** Sets naming convention for boolean testing functions, i.e.: foo_is_true */
#define FIO_NAME_BL(prefix, postfix)                                           \
  FIO_NAME_FROM_MACRO_STEP1(prefix, postfix, _is_)

/** Used internally to name test functions. */
#define FIO_NAME_TEST(prefix, postfix)                                         \
  FIO_NAME(fio___test, FIO_NAME(prefix, postfix))

/* *****************************************************************************
Version Macros

The facil.io C STL library follows [semantic versioning](https://semver.org) and
supports macros that will help detect and validate it's version.
***************************************************************************** */

/** MAJOR version: API/ABI breaking changes. */
#define FIO_VERSION_MAJOR 0
/** MINOR version: Deprecation, or significant features added. May break ABI. */
#define FIO_VERSION_MINOR 8
/** PATCH version: Bug fixes, minor features may be added. */
#define FIO_VERSION_PATCH 0
/** Build version: optional build info (string), i.e. "beta.02" */
#define FIO_VERSION_BUILD "alpha.3"

#ifdef FIO_VERSION_BUILD
/** Version as a String literal (MACRO). */
#define FIO_VERSION_STRING                                                     \
  FIO_MACRO2STR(FIO_VERSION_MAJOR)                                             \
  "." FIO_MACRO2STR(FIO_VERSION_MINOR) "." FIO_MACRO2STR(                      \
      FIO_VERSION_PATCH) "-" FIO_VERSION_BUILD
#else
/** Version as a String literal (MACRO). */
#define FIO_VERSION_STRING                                                     \
  FIO_MACRO2STR(FIO_VERSION_MAJOR)                                             \
  "." FIO_MACRO2STR(FIO_VERSION_MINOR) "." FIO_MACRO2STR(FIO_VERSION_PATCH)
#define FIO_VERSION_BUILD ""
#endif

/** If implemented, returns the major version number. */
int fio_version_major(void);
/** If implemented, returns the minor version number. */
int fio_version_minor(void);
/** If implemented, returns the patch version number. */
int fio_version_patch(void);
/** If implemented, returns the build version string. */
const char *fio_version_build(void);
/** If implemented, returns the version number as a string. */
char *fio_version_string(void);

#if FIO_VERSION_MAJOR
#define FIO_VERSION_VALIDATE()                                                 \
  FIO_ASSERT(fio_version_major() == FIO_VERSION_MAJOR &&                       \
                 fio_version_minor() == FIO_VERSION_MINOR,                     \
             "facil.io version mismatch, not %s",                              \
             fio_version_string())
#else
#define FIO_VERSION_VALIDATE()                                                 \
  FIO_ASSERT(fio_version_major() == FIO_VERSION_MAJOR &&                       \
                 fio_version_minor() == FIO_VERSION_MINOR &&                   \
                 fio_version_patch() == FIO_VERSION_PATCH,                     \
             "facil.io version mismatch, not %s",                              \
             fio_version_string())
#endif
/**
 * To implement the fio_version_* functions and FIO_VERSION_VALIDATE guard, the
 * `FIO_VERSION_GUARD` must be defined (only) once per application / library.
 */
#ifdef FIO_VERSION_GUARD
int __attribute__((weak)) fio_version_major(void) { return FIO_VERSION_MAJOR; }
int __attribute__((weak)) fio_version_minor(void) { return FIO_VERSION_MINOR; }
int __attribute__((weak)) fio_version_patch(void) { return FIO_VERSION_PATCH; }
const char *__attribute__((weak)) fio_version_build(void) {
  return FIO_VERSION_BUILD;
}
char *__attribute__((weak)) fio_version_string(void) {
  return FIO_VERSION_STRING;
}
#undef FIO_VERSION_GUARD
#endif /* FIO_VERSION_GUARD */

#if !defined(FIO_NO_COOKIE)
/** If implemented, does stuff. */
void __attribute__((weak)) fio___(void) {
  volatile uint8_t tmp[] =
      "\xA8\x94\x9A\x10\x99\x92\x93\x96\x9C\x1D\x96\x9F\x10\x9C\x96\x91\xB1\x92"
      "\xB1\xB6\x10\xBB\x92\xB3\x10\x92\xBA\xB8\x94\x9F\xB1\x9A\x98\x10\x91\xB6"
      "\x10\x81\x9F\x92\xB5\x10\xA3\x9A\x9B\x9A\xB9\x1D\x05\x10\x10\x10\x10\x8C"
      "\x96\xB9\x9A\x10\x9C\x9F\x9D\x9B\x10\x92\x9D\x98\x10\xB0\xB1\x9F\xB3\xB0"
      "\x9A\xB1\x1D";
  for (size_t i = 0; tmp[i]; ++i) {
    tmp[i] = ((tmp[i] & 0x55) << 1) | ((tmp[i] & 0xaa) >> 1);
  }
  fprintf(stderr, "%s\n", tmp);
}
#endif

/* *****************************************************************************
Pointer Math
***************************************************************************** */

/** Masks a pointer's left-most bits, returning the right bits. */
#define FIO_PTR_MATH_LMASK(T_type, ptr, bits)                                  \
  ((T_type *)((uintptr_t)(ptr) & (((uintptr_t)1 << (bits)) - 1)))

/** Masks a pointer's right-most bits, returning the left bits. */
#define FIO_PTR_MATH_RMASK(T_type, ptr, bits)                                  \
  ((T_type *)((uintptr_t)(ptr) & ((~(uintptr_t)0) << (bits))))

/** Add offset bytes to pointer, updating the pointer's type. */
#define FIO_PTR_MATH_ADD(T_type, ptr, offset)                                  \
  ((T_type *)((uintptr_t)(ptr) + (uintptr_t)(offset)))

/** Subtract X bytes from pointer, updating the pointer's type. */
#define FIO_PTR_MATH_SUB(T_type, ptr, offset)                                  \
  ((T_type *)((uintptr_t)(ptr) - (uintptr_t)(offset)))

/** Find the root object (of a struct) from it's field (with sanitizer fix). */
#define FIO_PTR_FROM_FIELD(T_type, field, ptr)                                 \
  FIO_PTR_MATH_SUB(T_type,                                                     \
                   ptr,                                                        \
                   (uintptr_t)(&((T_type *)0xFF00)->field) - 0xFF00)

/* *****************************************************************************
Security Related macros
***************************************************************************** */
#define FIO_MEM_STACK_WIPE(pages)                                              \
  do {                                                                         \
    volatile char stack_mem[(pages) << 12] = {0};                              \
    (void)stack_mem;                                                           \
  } while (0)

/* *****************************************************************************
Static Assertions
***************************************************************************** */
#if __STDC_VERSION__ >= 201112L
#define FIO_ASSERT_STATIC(cond, msg) _Static_assert((cond), msg)
#else
#define FIO_ASSERT_STATIC(cond, msg)                                           \
  static const char *FIO_NAME(fio_static_assertion_failed,                     \
                              __LINE__)[(((cond) << 1) - 1)] = {(char *)msg}
#endif

typedef struct {
  char data[2];
} fio___padding_char_struct_test_s;

FIO_ASSERT_STATIC(CHAR_BIT == 8, "facil.io requires an 8bit wide char");
FIO_ASSERT_STATIC(sizeof(uint8_t) == 1,
                  "facil.io requires an 8bit wide uint8_t");
FIO_ASSERT_STATIC(sizeof(uint16_t) == 2,
                  "facil.io requires a 16bit wide uint16_t");
FIO_ASSERT_STATIC(sizeof(uint32_t) == 4,
                  "facil.io requires a 32bit wide uint32_t");
FIO_ASSERT_STATIC(sizeof(uint64_t) == 8,
                  "facil.io requires a 64bit wide uint64_t");
FIO_ASSERT_STATIC(sizeof(fio___padding_char_struct_test_s) == 2,
                  "compiler adds padding to fio___memcpyX, creating memory "
                  "alignment issues.");

/* *****************************************************************************
Miscellaneous helper macros
***************************************************************************** */

/* avoid printing a full / nested path when __FILE_NAME__ is available */
#ifdef __FILE_NAME__
#define FIO__FILE__ __FILE_NAME__
#else
#define FIO__FILE__ __FILE__
#endif

/** An empty macro, adding white space. Used to avoid function like macros. */
#define FIO_NOOP
/* allow logging to quitely fail unless enabled */
#define FIO_LOG2STDERR(...)   ((void)0)
#define FIO_LOG2STDERR2(...)  ((void)0)
#define FIO_LOG_PRINT__(...)  ((void)0)
#define FIO_LOG_FATAL(...)    ((void)0)
#define FIO_LOG_ERROR(...)    ((void)0)
#define FIO_LOG_SECURITY(...) ((void)0)
#define FIO_LOG_WARNING(...)  ((void)0)
#define FIO_LOG_INFO(...)     ((void)0)
#define FIO_LOG_DEBUG(...)    ((void)0)
#define FIO_LOG_DEBUG2(...)   ((void)0)

#ifdef DEBUG
#define FIO_LOG_DDEBUG(...)           FIO_LOG_DEBUG(__VA_ARGS__)
#define FIO_LOG_DDEBUG2(...)          FIO_LOG_DEBUG2(__VA_ARGS__)
#define FIO_ASSERT___PERFORM_SIGNAL() kill(0, SIGINT);
#else
#define FIO_LOG_DDEBUG(...)  ((void)(0))
#define FIO_LOG_DDEBUG2(...) ((void)(0))
#define FIO_ASSERT___PERFORM_SIGNAL()
#endif /* DEBUG */

#ifndef FIO_LOG_LENGTH_LIMIT
/** Defines a point at which logging truncates (limited by stack memory) */
#define FIO_LOG_LENGTH_LIMIT 1024
#endif

/* Asserts a condition is true, or kills the application using SIGINT. */
#define FIO_ASSERT(cond, ...)                                                  \
  do {                                                                         \
    if (!(cond)) {                                                             \
      FIO_LOG_FATAL(__VA_ARGS__);                                              \
      FIO_LOG_FATAL("     errno(%d): %s\n", errno, strerror(errno));           \
      FIO_ASSERT___PERFORM_SIGNAL();                                           \
      exit(-1);                                                                \
    }                                                                          \
  } while (0)

#ifndef FIO_ASSERT_ALLOC
/** Tests for an allocation failure. The behavior can be overridden. */
#define FIO_ASSERT_ALLOC(ptr) FIO_ASSERT((ptr), "memory allocation failed.")
#endif

#ifdef DEBUG
/** If `DEBUG` is defined, raises SIGINT if assertion fails, otherwise NOOP. */
#define FIO_ASSERT_DEBUG(cond, ...)                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      FIO_LOG_FATAL("(" FIO__FILE__                                            \
                    ":" FIO_MACRO2STR(__LINE__) ") " __VA_ARGS__);             \
      FIO_LOG_FATAL("     errno(%d): %s\n", errno, strerror(errno));           \
      FIO_ASSERT___PERFORM_SIGNAL();                                           \
      exit(-1);                                                                \
    }                                                                          \
  } while (0)
#else
#define FIO_ASSERT_DEBUG(...)
#endif

/* *****************************************************************************
Linked Lists Persistent Macros and Types
***************************************************************************** */

/** A linked list arch-type */
typedef struct fio_list_node_s {
  struct fio_list_node_s *next;
  struct fio_list_node_s *prev;
} fio_list_node_s;

/** A linked list node type */
#define FIO_LIST_NODE fio_list_node_s
/** A linked list head type */
#define FIO_LIST_HEAD fio_list_node_s

/** Allows initialization of FIO_LIST_HEAD objects. */
#define FIO_LIST_INIT(obj)                                                     \
  (fio_list_node_s) { .next = &(obj), .prev = &(obj) }

#ifndef FIO_LIST_EACH
/** Loops through every node in the linked list except the head. */
#define FIO_LIST_EACH(type, node_name, head, pos)                              \
  for (type *pos = FIO_PTR_FROM_FIELD(type, node_name, (head)->next),          \
            *next____p_ls_##pos =                                              \
                FIO_PTR_FROM_FIELD(type, node_name, (head)->next->next);       \
       pos != FIO_PTR_FROM_FIELD(type, node_name, (head));                     \
       (pos = next____p_ls_##pos),                                             \
            (next____p_ls_##pos =                                              \
                 FIO_PTR_FROM_FIELD(type,                                      \
                                    node_name,                                 \
                                    next____p_ls_##pos->node_name.next)))
/** Loops through every node in the linked list except the head. */
#define FIO_LIST_EACH_REVERSED(type, node_name, head, pos)                     \
  for (type *pos = FIO_PTR_FROM_FIELD(type, node_name, (head)->prev),          \
            *next____p_ls_##pos =                                              \
                FIO_PTR_FROM_FIELD(type, node_name, (head)->next->prev);       \
       pos != FIO_PTR_FROM_FIELD(type, node_name, (head));                     \
       (pos = next____p_ls_##pos),                                             \
            (next____p_ls_##pos =                                              \
                 FIO_PTR_FROM_FIELD(type,                                      \
                                    node_name,                                 \
                                    next____p_ls_##pos->node_name.prev)))
#endif

/** UNSAFE macro for pushing a node to a list. */
#define FIO_LIST_PUSH(head, n)                                                 \
  do {                                                                         \
    (n)->prev = (head)->prev;                                                  \
    (n)->next = (head);                                                        \
    (head)->prev->next = (n);                                                  \
    (head)->prev = (n);                                                        \
  } while (0)

/** UNSAFE macro for removing a node from a list. */
#define FIO_LIST_REMOVE(n)                                                     \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
  } while (0)

/** UNSAFE macro for removing a node from a list. Resets node data. */
#define FIO_LIST_REMOVE_RESET(n)                                               \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
    (n)->next = (n)->prev = (n);                                               \
  } while (0)

/** UNSAFE macro for popping a node to a list. */
#define FIO_LIST_POP(type, node_name, dest_ptr, head)                          \
  do {                                                                         \
    (dest_ptr) = FIO_PTR_FROM_FIELD(type, node_name, ((head)->next));          \
    FIO_LIST_REMOVE(&(dest_ptr)->node_name);                                   \
  } while (0)

/** UNSAFE macro for testing if a list is empty. */
#define FIO_LIST_IS_EMPTY(head)                                                \
  ((!(head)) || ((!(head)->next) | ((head)->next == (head))))

/* *****************************************************************************
Indexed Linked Lists Persistent Macros and Types

Indexed Linked Lists can be used to create a linked list that uses is always
relative to some root pointer (usually the root of an array). This:

1. Allows easy reallocation of the list without requiring pointer updates.

2. Could be used for memory optimization if the array limits are known.

The "head" index is usually validated by reserving the value of `-1` to indicate
an empty list.
***************************************************************************** */
#ifndef FIO_INDEXED_LIST_EACH

/** A 32 bit indexed linked list node type */
typedef struct fio_index32_node_s {
  uint32_t next;
  uint32_t prev;
} fio_index32_node_s;

/** A 16 bit indexed linked list node type */
typedef struct fio_index16_node_s {
  uint16_t next;
  uint16_t prev;
} fio_index16_node_s;

/** An 8 bit indexed linked list node type */
typedef struct fio_index8_node_s {
  uint8_t next;
  uint8_t prev;
} fio_index8_node_s;

/** A 32 bit indexed linked list node type */
#define FIO_INDEXED_LIST32_NODE fio_index32_node_s
#define FIO_INDEXED_LIST32_HEAD uint32_t
/** A 16 bit indexed linked list node type */
#define FIO_INDEXED_LIST16_NODE fio_index16_node_s
#define FIO_INDEXED_LIST16_HEAD uint16_t
/** An 8 bit indexed linked list node type */
#define FIO_INDEXED_LIST8_NODE fio_index8_node_s
#define FIO_INDEXED_LIST8_HEAD uint8_t

/** UNSAFE macro for pushing a node to a list. */
#define FIO_INDEXED_LIST_PUSH(root, node_name, head, i)                        \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[n__].node_name.prev = (root)[(head)].node_name.prev;                \
    (root)[n__].node_name.next = (head);                                       \
    (root)[(root)[(head)].node_name.prev].node_name.next = n__;                \
    (root)[(head)].node_name.prev = n__;                                       \
  } while (0)

/** UNSAFE macro for adding a node to the begging of the list. */
#define FIO_INDEXED_LIST_UNSHIFT(root, node_name, head, i)                     \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[n__].node_name.next = (root)[(head)].node_name.next;                \
    (root)[n__].node_name.prev = (head);                                       \
    (root)[(root)[(head)].node_name.next].node_name.prev = n__;                \
    (root)[(head)].node_name.next = n__;                                       \
    (head) = n__;                                                              \
  } while (0)

/** UNSAFE macro for removing a node from a list. */
#define FIO_INDEXED_LIST_REMOVE(root, node_name, i)                            \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[(root)[n__].node_name.prev].node_name.next =                        \
        (root)[n__].node_name.next;                                            \
    (root)[(root)[n__].node_name.next].node_name.prev =                        \
        (root)[n__].node_name.prev;                                            \
  } while (0)

/** UNSAFE macro for removing a node from a list. Resets node data. */
#define FIO_INDEXED_LIST_REMOVE_RESET(root, node_name, i)                      \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[(root)[n__].node_name.prev].node_name.next =                        \
        (root)[n__].node_name.next;                                            \
    (root)[(root)[n__].node_name.next].node_name.prev =                        \
        (root)[n__].node_name.prev;                                            \
    (root)[n__].node_name.next = (root)[n__].node_name.prev = n__;             \
  } while (0)

/** Loops through every index in the indexed list, assuming `head` is valid. */
#define FIO_INDEXED_LIST_EACH(root, node_name, head, pos)                      \
  for (size_t pos = (head), stopper___ils___ = 0; !stopper___ils___;           \
       stopper___ils___ = ((pos = (root)[pos].node_name.next) == (head)))

/** Loops through every index in the indexed list, assuming `head` is valid. */
#define FIO_INDEXED_LIST_EACH_REVERSED(root, node_name, head, pos)             \
  for (size_t pos = ((root)[head].node_name.prev), stopper___ils___ = 0;       \
       !stopper___ils___;                                                      \
       ((stopper___ils___ = (pos == head)),                                    \
        (pos = (root)[pos].node_name.prev)))
#endif

/* *****************************************************************************
Common String Helpers
***************************************************************************** */

/** An information type for reporting the string's state. */
typedef struct fio_str_info_s {
  /** The string's length, if any. */
  size_t len;
  /** The string's buffer (pointer to first byte) or NULL on error. */
  char *buf;
  /** The buffer's capacity. Zero (0) indicates the buffer is read-only. */
  size_t capa;
} fio_str_info_s;

/** An information type for reporting/storing buffer data (no `capa`). */
typedef struct fio_buf_info_s {
  /** The buffer's length, if any. */
  size_t len;
  /** The buffer's address (may be NULL if no buffer). */
  char *buf;
} fio_buf_info_s;

/** Compares two `fio_str_info_s` objects for content equality. */
#define FIO_STR_INFO_IS_EQ(s1, s2)                                             \
  ((s1).len == (s2).len && (!(s1).len || (s1).buf == (s2).buf ||               \
                            !memcmp((s1).buf, (s2).buf, (s1).len)))

/** Compares two `fio_buf_info_s` objects for content equality. */
#define FIO_BUF_INFO_IS_EQ(s1, s2) FIO_STR_INFO_IS_EQ((s1), (s2))

/** Converts a C String into a fio_str_info_s. */
#define FIO_STR_INFO1(str)                                                     \
  ((fio_str_info_s){.len = strlen((str)), .buf = (str)})

/** Converts a String with a known length into a fio_str_info_s. */
#define FIO_STR_INFO2(str, length)                                             \
  ((fio_str_info_s){.len = (length), .buf = (str)})

/** Converts a String with a known length and capacity into a fio_str_info_s. */
#define FIO_STR_INFO3(str, length, capacity)                                   \
  ((fio_str_info_s){.len = (length), .buf = (str), .capa = (capacity)})

/** Converts a C String into a fio_buf_info_s. */
#define FIO_BUF_INFO1(str)                                                     \
  ((fio_buf_info_s){.len = strlen((str)), .buf = (str)})

/** Converts a String with a known length into a fio_buf_info_s. */
#define FIO_BUF_INFO2(str, length)                                             \
  ((fio_buf_info_s){.len = (length), .buf = (str)})

/** Converts a fio_buf_info_s into a fio_str_info_s. */
#define FIO_BUF2STR_INFO(buf_info)                                             \
  ((fio_str_info_s){.len = (buf_info).len, .buf = (buf_info).buf})

/** Converts a fio_buf_info_s into a fio_str_info_s. */
#define FIO_STR2BUF_INFO(str_info)                                             \
  ((fio_buf_info_s){.len = (str_info).len, .buf = (str_info).buf})

/* *****************************************************************************
Sleep / Thread Scheduling Macros
***************************************************************************** */

#ifndef FIO_THREAD_WAIT
#if FIO_OS_WIN
/**
 * Calls NtDelayExecution with the requested nano-second count.
 */
#define FIO_THREAD_WAIT(nano_sec)                                              \
  do {                                                                         \
    Sleep(((nano_sec) / 1000000) ? ((nano_sec) / 1000000) : 1);                \
  } while (0)
// https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep

#elif FIO_OS_POSIX
/**
 * Calls nanonsleep with the requested nano-second count.
 */
#define FIO_THREAD_WAIT(nano_sec)                                              \
  do {                                                                         \
    const struct timespec tm = {.tv_sec = (time_t)((nano_sec) / 1000000000),   \
                                .tv_nsec = ((long)(nano_sec) % 1000000000)};   \
    nanosleep(&tm, (struct timespec *)NULL);                                   \
  } while (0)

#endif
#endif

#ifndef FIO_THREAD_RESCHEDULE
/**
 * Reschedules the thread by calling nanosleeps for a single nano-second.
 *
 * In practice, the thread will probably sleep for 60ns or more.
 */
#define FIO_THREAD_RESCHEDULE() FIO_THREAD_WAIT(4)
#endif

/* *****************************************************************************


Patch for OSX version < 10.12 from https://stackoverflow.com/a/9781275/4025095

Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#if (defined(__MACH__) && !defined(CLOCK_REALTIME))
#warning fio_time functions defined using gettimeofday patch.
#include <sys/time.h>
#define CLOCK_REALTIME 0
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif
#define clock_gettime fio_clock_gettime
// clock_gettime is not implemented on older versions of OS X (< 10.12).
// If implemented, CLOCK_MONOTONIC will have already been defined.
FIO_IFUNC int fio_clock_gettime(int clk_id, struct timespec *t) {
  struct timeval now;
  int rv = gettimeofday(&now, NULL);
  if (rv)
    return rv;
  t->tv_sec = now.tv_sec;
  t->tv_nsec = now.tv_usec * 1000;
  return 0;
  (void)clk_id;
}

#endif
/* *****************************************************************************




Patches for Windows




***************************************************************************** */
#if FIO_OS_WIN
#if _MSC_VER
#pragma message("warning: some functionality is enabled by patchwork.")
#else
#warning some functionality is enabled by patchwork.
#endif
#include <fcntl.h>
#include <io.h>
#include <processthreadsapi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sysinfoapi.h>
#include <time.h>
#include <winsock2.h> /* struct timeval is here... why? Microsoft. */

/* *****************************************************************************
Windows initialization
***************************************************************************** */

/* Enable console colors */
FIO_CONSTRUCTOR(fio___windows_startup_housekeeping) {
  HANDLE c = GetStdHandle(STD_OUTPUT_HANDLE);
  if (c) {
    DWORD mode = 0;
    if (GetConsoleMode(c, &mode)) {
      mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      SetConsoleMode(c, mode);
    }
  }
  c = GetStdHandle(STD_ERROR_HANDLE);
  if (c) {
    DWORD mode = 0;
    if (GetConsoleMode(c, &mode)) {
      mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      SetConsoleMode(c, mode);
    }
  }
}

/* *****************************************************************************
Inlined patched and MACRO statements
***************************************************************************** */

FIO_IFUNC struct tm *gmtime_r(const time_t *timep, struct tm *result) {
  struct tm *t = gmtime(timep);
  if (t && result)
    *result = *t;
  return result;
}

FIO_IFUNC int strcasecmp(const char *s1, const char *s2) {
  return _stricmp(s1, s2);
}

FIO_IFUNC int write(int fd, const void *b, unsigned int l) {
  return _write(fd, b, l);
}

FIO_IFUNC int read(int const fd, void *const b, unsigned const l) {
  return _read(fd, b, l);
}

#if !defined(fstat)
#define fstat _fstat
#endif /* fstat */
#if !defined(stat)
#define stat _stat
#endif /* stat */

#define O_APPEND      _O_APPEND
#define O_BINARY      _O_BINARY
#define O_CREAT       _O_CREAT
#define O_CREAT       _O_CREAT
#define O_SHORT_LIVED _O_SHORT_LIVED
#define O_CREAT       _O_CREAT
#define O_TEMPORARY   _O_TEMPORARY
#define O_CREAT       _O_CREAT
#define O_EXCL        _O_EXCL
#define O_NOINHERIT   _O_NOINHERIT
#define O_RANDOM      _O_RANDOM
#define O_RDONLY      _O_RDONLY
#define O_RDWR        _O_RDWR
#define O_SEQUENTIAL  _O_SEQUENTIAL
#define O_TEXT        _O_TEXT
#define O_TRUNC       _O_TRUNC
#define O_WRONLY      _O_WRONLY
#define O_U16TEXT     _O_U16TEXT
#define O_U8TEXT      _O_U8TEXT
#define O_WTEXT       _O_WTEXT
#define S_IREAD       _S_IREAD
#define S_IWRITE      _S_IWRITE
#define S_IRUSR       _S_IREAD
#define S_IWUSR       _S_IWRITE

#ifndef O_TMPFILE
#define O_TMPFILE O_TEMPORARY
#endif

#if defined(CLOCK_REALTIME) && defined(CLOCK_MONOTONIC) &&                     \
    CLOCK_REALTIME == CLOCK_MONOTONIC
#undef CLOCK_MONOTONIC
#undef CLOCK_REALTIME
#endif

#ifndef CLOCK_REALTIME
#ifdef CLOCK_MONOTONIC
#define CLOCK_REALTIME (CLOCK_MONOTONIC + 1)
#else
#define CLOCK_REALTIME 0
#endif
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

/** patch for clock_gettime */
FIO_SFUNC int fio_clock_gettime(const uint32_t clk_type, struct timespec *tv);
/** patch for pread */
FIO_SFUNC ssize_t fio_pread(int fd, void *buf, size_t count, off_t offset);
/** patch for pwrite */
FIO_SFUNC ssize_t fio_pwrite(int fd,
                             const void *buf,
                             size_t count,
                             off_t offset);
FIO_SFUNC int fio_kill(int pid, int signum);

#define kill   fio_kill
#define pread  fio_pread
#define pwrite fio_pwrite

#if !FIO_HAVE_UNIX_TOOLS
/* patch clock_gettime */
#define clock_gettime fio_clock_gettime
#define pipe(fds)     _pipe(fds, 65536, _O_BINARY)
#endif

/* *****************************************************************************
Patched functions
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* based on:
 * https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
 */
/** patch for clock_gettime */
FIO_SFUNC int fio_clock_gettime(const uint32_t clk_type, struct timespec *tv) {
  if (!tv)
    return -1;
  static union {
    uint64_t u;
    LARGE_INTEGER li;
  } freq = {.u = 0};
  static double tick2n = 0;
  union {
    uint64_t u;
    FILETIME ft;
    LARGE_INTEGER li;
  } tu;

  switch (clk_type) {
  case CLOCK_REALTIME:
  realtime_clock:
    GetSystemTimePreciseAsFileTime(&tu.ft);
    tv->tv_sec = tu.u / 10000000;
    tv->tv_nsec = tu.u - (tv->tv_sec * 10000000);
    return 0;

#ifdef CLOCK_PROCESS_CPUTIME_ID
  case CLOCK_PROCESS_CPUTIME_ID:
#endif
#ifdef CLOCK_THREAD_CPUTIME_ID
  case CLOCK_THREAD_CPUTIME_ID:
#endif
  case CLOCK_MONOTONIC:
    if (!QueryPerformanceCounter(&tu.li))
      goto realtime_clock;
    if (!freq.u)
      QueryPerformanceFrequency(&freq.li);
    if (!freq.u) {
      tick2n = 0;
      freq.u = 1;
    } else {
      tick2n = (double)1000000000 / freq.u;
    }
    tv->tv_sec = tu.u / freq.u;
    tv->tv_nsec =
        (uint64_t)(0ULL + ((double)(tu.u - (tv->tv_sec * freq.u)) * tick2n));
    return 0;
  }
  return -1;
}

/** patch for pread */
FIO_SFUNC ssize_t fio_pread(int fd, void *buf, size_t count, off_t offset) {
  /* Credit to Jan Biedermann (GitHub: @janbiedermann) */
  ssize_t bytes_read = 0;
  HANDLE handle = (HANDLE)_get_osfhandle(fd);
  if (handle == INVALID_HANDLE_VALUE)
    goto bad_file;
  OVERLAPPED overlapped = {0};
  if (offset > 0)
    overlapped.Offset = offset;
  if (ReadFile(handle, buf, count, (u_long *)&bytes_read, &overlapped))
    return bytes_read;
  if (GetLastError() == ERROR_HANDLE_EOF)
    return bytes_read;
  errno = EIO;
  return -1;
bad_file:
  errno = EBADF;
  return -1;
}

/** patch for pwrite */
FIO_SFUNC ssize_t fio_pwrite(int fd,
                             const void *buf,
                             size_t count,
                             off_t offset) {
  /* Credit to Jan Biedermann (GitHub: @janbiedermann) */
  ssize_t bytes_written = 0;
  HANDLE handle = (HANDLE)_get_osfhandle(fd);
  if (handle == INVALID_HANDLE_VALUE)
    goto bad_file;
  OVERLAPPED overlapped = {0};
  if (offset > 0)
    overlapped.Offset = offset;
  if (WriteFile(handle, buf, count, (u_long *)&bytes_written, &overlapped))
    return bytes_written;
  errno = EIO;
  return -1;
bad_file:
  errno = EBADF;
  return -1;
}

/** patch for kill */
FIO_SFUNC int fio_kill(int pid, int sig) {
  /* Credit to Jan Biedermann (GitHub: @janbiedermann) */
  HANDLE handle;
  DWORD status;
  if (sig < 0 || sig >= NSIG) {
    errno = EINVAL;
    return -1;
  }
#ifdef SIGCONT
  if (sig == SIGCONT) {
    errno = ENOSYS;
    return -1;
  }
#endif

  if (pid == -1)
    pid = 0;

  if (!pid)
    handle = GetCurrentProcess();
  else
    handle =
        OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, pid);
  if (!handle)
    goto something_went_wrong;

  switch (sig) {
#ifdef SIGKILL
  case SIGKILL:
#endif
  case SIGTERM:
  case SIGINT: /* terminate */
    if (!TerminateProcess(handle, 1))
      goto something_went_wrong;
    break;
  case 0: /* check status */
    if (!GetExitCodeProcess(handle, &status))
      goto something_went_wrong;
    if (status != STILL_ACTIVE) {
      errno = ESRCH;
      goto cleanup_after_error;
    }
    break;
  default: /* not supported? */ errno = ENOSYS; goto cleanup_after_error;
  }

  if (pid) {
    CloseHandle(handle);
  }
  return 0;

something_went_wrong:

  switch (GetLastError()) {
  case ERROR_INVALID_PARAMETER: errno = ESRCH; break;
  case ERROR_ACCESS_DENIED:
    errno = EPERM;
    if (handle && GetExitCodeProcess(handle, &status) && status != STILL_ACTIVE)
      errno = ESRCH;
    break;
  default: errno = GetLastError();
  }
cleanup_after_error:
  if (handle && pid)
    CloseHandle(handle);
  return -1;
}

#endif /* FIO_EXTERN_COMPLETE */

/* *****************************************************************************



Patches for POSIX



***************************************************************************** */
#elif FIO_OS_POSIX /* POSIX patches */
#endif
/* *****************************************************************************
Done with Patches
***************************************************************************** */

/* *****************************************************************************
End persistent segment (end include-once guard)
***************************************************************************** */
#endif /* H___FIO_CSTL_INCLUDE_ONCE___H */

/* *****************************************************************************




                          Common internal Macros




***************************************************************************** */

/* *****************************************************************************
Memory allocation macros
***************************************************************************** */

#ifndef FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT
/* secure by default */
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT 1
#endif

#if defined(FIO_MEM_REST) || !defined(FIO_MEM_REALLOC) || !defined(FIO_MEM_FREE)

#undef FIO_MEM_REALLOC
#undef FIO_MEM_FREE
#undef FIO_MEM_REALLOC_IS_SAFE
#undef FIO_MEM_REST

/* if a global allocator was previously defined route macros to fio_malloc */
#if defined(H___FIO_MALLOC___H)
/** Reallocates memory, copying (at least) `copy_len` if necessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  fio_realloc2((ptr), (new_size), (copy_len))
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) fio_free((ptr))
/** Set to true of internall allocator is used (memory returned set to zero). */
#define FIO_MEM_REALLOC_IS_SAFE 1

#else
/** Reallocates memory, copying (at least) `copy_len` if necessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  realloc((ptr), (new_size))
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) free((ptr))
/** Set to true of internall allocator is used (memory returned set to zero). */
#define FIO_MEM_REALLOC_IS_SAFE 0
#endif /* H___FIO_MALLOC___H */

#endif /* defined(FIO_MEM_REALLOC) */

/* *****************************************************************************
Locking selector
***************************************************************************** */

#ifndef FIO_USE_THREAD_MUTEX
#define FIO_USE_THREAD_MUTEX 0
#endif

#ifndef FIO_USE_THREAD_MUTEX_TMP
#define FIO_USE_THREAD_MUTEX_TMP FIO_USE_THREAD_MUTEX
#endif

#if FIO_USE_THREAD_MUTEX_TMP
#define FIO_THREAD
#define FIO___LOCK_NAME          "OS mutex"
#define FIO___LOCK_TYPE          fio_thread_mutex_t
#define FIO___LOCK_INIT          ((FIO___LOCK_TYPE)FIO_THREAD_MUTEX_INIT)
#define FIO___LOCK_DESTROY(lock) fio_thread_mutex_destroy(&(lock))
#define FIO___LOCK_LOCK(lock)                                                  \
  do {                                                                         \
    if (fio_thread_mutex_lock(&(lock)))                                        \
      FIO_LOG_ERROR("Couldn't lock mutex @ %s:%d - error (%d): %s",            \
                    __FILE__,                                                  \
                    __LINE__,                                                  \
                    errno,                                                     \
                    strerror(errno));                                          \
  } while (0)
#define FIO___LOCK_TRYLOCK(lock) fio_thread_mutex_trylock(&(lock))
#define FIO___LOCK_UNLOCK(lock)                                                \
  do {                                                                         \
    if (fio_thread_mutex_unlock(&(lock))) {                                    \
      FIO_LOG_ERROR("Couldn't release mutex @ %s:%d - error (%d): %s",         \
                    __FILE__,                                                  \
                    __LINE__,                                                  \
                    errno,                                                     \
                    strerror(errno));                                          \
    }                                                                          \
  } while (0)

#else
#define FIO___LOCK_NAME          "facil.io spinlocks"
#define FIO___LOCK_TYPE          fio_lock_i
#define FIO___LOCK_INIT          ((FIO___LOCK_TYPE)FIO_LOCK_INIT)
#define FIO___LOCK_DESTROY(lock) ((lock) = FIO___LOCK_INIT)
#define FIO___LOCK_LOCK(lock)    fio_lock(&(lock))
#define FIO___LOCK_TRYLOCK(lock) fio_trylock(&(lock))
#define FIO___LOCK_UNLOCK(lock)  fio_unlock(&(lock))
#endif

/* *****************************************************************************
Special `extern` support for FIO_EVERYTHING - Everything, and the Kitchen Sink
***************************************************************************** */
#if defined(FIO_EVERYTHING) && !defined(H___FIO_EVERYTHING___H) &&             \
    !defined(FIO_STL_KEEP__)
#if defined(FIO_EXTERN) && ((FIO_EXTERN + 1) < 3)
#undef FIO_EXTERN
#define FIO_EXTERN                     2
#define FIO_EVERYTHING___REMOVE_EXTERN 1
#endif
#if defined(FIO_EXTERN_COMPLETE) && ((FIO_EXTERN_COMPLETE + 1) < 3)
#undef FIO_EXTERN_COMPLETE
#define FIO_EXTERN_COMPLETE                     2
#define FIO_EVERYTHING___REMOVE_EXTERN_COMPLETE 1
#endif
#endif
/* *****************************************************************************
Recursive inclusion management
***************************************************************************** */
#ifndef SFUNC_ /* if we aren't in a recursive #include statement */

#ifdef FIO_EXTERN
#define SFUNC_
#define IFUNC_

#else /* !FIO_EXTERN */
#undef SFUNC
#undef IFUNC
#define SFUNC_ static __attribute__((unused))
#define IFUNC_ static inline __attribute__((unused))
#endif /* FIO_EXTERN */

#undef SFUNC
#undef IFUNC
#define SFUNC SFUNC_
#define IFUNC IFUNC_

#elif !defined(FIO_STL_KEEP__) || (FIO_STL_KEEP__ + 1 != 100)
/* SFUNC_ - internal helper types are always `static` */
#undef SFUNC
#undef IFUNC
#define SFUNC FIO_SFUNC
#define IFUNC FIO_IFUNC
#endif /* SFUNC_ vs FIO_STL_KEEP__*/

/* *****************************************************************************
Pointer Tagging
***************************************************************************** */
#ifndef FIO_PTR_TAG
/**
 * Supports embedded pointer tagging / untagging for the included types.
 *
 * Should resolve to a tagged pointer value. i.e.: ((uintptr_t)(p) | 1)
 */
#define FIO_PTR_TAG(p) (p)
#endif

#ifndef FIO_PTR_UNTAG
/**
 * Supports embedded pointer tagging / untagging for the included types.
 *
 * Should resolve to an untagged pointer value. i.e.: ((uintptr_t)(p) | ~1UL)
 */
#define FIO_PTR_UNTAG(p) (p)
#endif

/**
 * If FIO_PTR_TAG_TYPE is defined, then functions returning a type's pointer
 * will return a pointer of the specified type instead.
 */
#ifndef FIO_PTR_TAG_TYPE
#endif

#ifndef FIO_PTR_TAG_VALIDATE
/**
 * If FIO_PTR_TAG_VALIDATE is defined, tagging will be verified before executing
 * any code.
 *
 * FIO_PTR_TAG_VALIDATE must fail on NULL pointers.
 */
#define FIO_PTR_TAG_VALIDATE(ptr) ((ptr) != NULL)
#endif

#undef FIO_PTR_TAG_VALID_OR_RETURN
#define FIO_PTR_TAG_VALID_OR_RETURN(tagged_ptr, value)                         \
  do {                                                                         \
    if (!(FIO_PTR_TAG_VALIDATE(tagged_ptr))) {                                 \
      FIO_LOG_DEBUG("pointer tag (type) mismatch in function call.");          \
      return (value);                                                          \
    }                                                                          \
  } while (0)
#undef FIO_PTR_TAG_VALID_OR_RETURN_VOID
#define FIO_PTR_TAG_VALID_OR_RETURN_VOID(tagged_ptr)                           \
  do {                                                                         \
    if (!(FIO_PTR_TAG_VALIDATE(tagged_ptr))) {                                 \
      FIO_LOG_DEBUG("pointer tag (type) mismatch in function call.");          \
      return;                                                                  \
    }                                                                          \
  } while (0)
#undef FIO_PTR_TAG_VALID_OR_GOTO
#define FIO_PTR_TAG_VALID_OR_GOTO(tagged_ptr, lable)                           \
  do {                                                                         \
    if (!(FIO_PTR_TAG_VALIDATE(tagged_ptr))) {                                 \
      /* Log error since GOTO indicates cleanup or other side-effects. */      \
      FIO_LOG_ERROR("(" FIO__FILE__ ":" FIO_MACRO2STR(                         \
          __LINE__) ") pointer tag (type) mismatch in function call.");        \
      goto lable;                                                              \
    }                                                                          \
  } while (0)

#define FIO_PTR_TAG_GET_UNTAGGED(untagged_type, tagged_ptr)                    \
  ((untagged_type *)(FIO_PTR_UNTAG(tagged_ptr)))

/* *****************************************************************************



                          Internal Dependencies



***************************************************************************** */
/* Testing Dependencies */
#if defined(FIO_TEST_CSTL)
#ifndef FIO_TEST_REPEAT
#define FIO_TEST_REPEAT 4096
#endif
#ifndef FIO_LOG
#define FIO_LOG
#endif
#endif

/* Modules that require FIO_SERVER */
#if defined(FIO_PUBSUB)
#ifndef FIO_SERVER
#define FIO_SERVER
#endif
#endif

/* Modules that require FIO_GLOB_MATCH */
#if defined(FIO_PUBSUB)
#ifndef FIO_GLOB_MATCH
#define FIO_GLOB_MATCH
#endif
#endif

/* Modules that require FIO_POLL */
#if defined(FIO_SERVER)
#ifndef FIO_POLL
#define FIO_POLL
#endif
#endif

/* Modules that require FIO_SIGNAL */
#if defined(FIO_SERVER)
#ifndef FIO_SIGNAL
#define FIO_SIGNAL
#endif
#endif

/* Modules that require FIO_STATE */
#if defined(FIO_MEMORY_NAME) || defined(FIO_MALLOC) ||                         \
    defined(FIOBJ_MALLOC) || defined(FIO_POLL)
#ifndef FIO_STATE
#define FIO_STATE
#endif
#endif

/* Modules that require FIO_STREAM */
#if defined(FIO_SERVER)
#ifndef FIO_STREAM
#define FIO_STREAM
#endif
#endif

/* Modules that require FIO_SOCK */
#if defined(FIO_POLL)
#ifndef FIO_SOCK
#define FIO_SOCK
#endif
#endif

/* Modules that require FIO_QUEUE */
#if defined(FIO_POLL)
#ifndef FIO_QUEUE
#define FIO_QUEUE
#endif
#endif

/* Modules that require FIO_URL */
#if defined(FIO_SOCK)
#ifndef FIO_URL
#define FIO_URL
#endif
#endif

/* Modules that require Threads API */
#if (defined(FIO_QUEUE) && defined(FIO_TEST_CSTL)) ||                          \
    defined(FIO_MEMORY_NAME) || defined(FIO_MALLOC) ||                         \
    defined(FIO_USE_THREAD_MUTEX_TMP)
#ifndef FIO_THREADS
#define FIO_THREADS
#endif
#endif

/* Modules that require the String Core API */
#if defined(FIO_STR_NAME) || defined(FIO_STR_SMALL) ||                         \
    defined(FIO_MAP_KEYSTR) || !defined(FIO_MAP_KEY) ||                        \
    defined(FIO_MAP_VALUE_BSTR) || defined(FIO_SERVER) || defined(FIO_FIOBJ)
#ifndef FIO_STR
#define FIO_STR
#endif
#endif

/* Modules that require File Utils */
#if defined(FIO_STR)
#ifndef FIO_FILES
#define FIO_FILES
#endif
#endif

/* Modules that require randomness */
#if defined(FIO_MEMORY_NAME) || defined(FIO_MALLOC) || defined(FIO_FILES) ||   \
    defined(FIO_STATE) || defined(FIO_STR_NAME) || defined(FIO_STR_SMALL) ||   \
    defined(FIO_CLI) || defined(FIO_MEMORY_NAME) || defined(FIO_MALLOC) ||     \
    defined(FIO_POLL) || defined(FIO_TEST_CSTL)
#ifndef FIO_RAND
#define FIO_RAND
#endif
#endif /* FIO_MALLOC */

/* Modules that require FIO_TIME */
#if defined(FIO_QUEUE) || defined(FIO_RAND)
#ifndef FIO_TIME
#define FIO_TIME
#endif
#endif /* FIO_QUEUE */

/* Modules that require FIO_MATH */
#if defined(FIO_RAND) || defined(FIO_CHACHA) || defined(FIO_TEST_CSTL)
#ifndef FIO_MATH
#define FIO_MATH
#endif
#endif

/* Modules that require FIO_BITMAP */
#if defined(FIO_JSON) || defined(FIO_TEST_CSTL)
#ifndef FIO_BITMAP
#define FIO_BITMAP
#endif
#endif /* FIO_BITMAP */

/* Modules that require FIO_IMAP_CORE */
#if defined(FIO_STATE)
#ifndef FIO_IMAP_CORE
#define FIO_IMAP_CORE
#endif
#endif /* FIO_IMAP_CORE */

/* Modules that require FIO_BITWISE (includes FIO_RAND requirements) */
#if defined(FIO_STR_NAME) || defined(FIO_RAND) || defined(FIO_JSON) ||         \
    defined(FIO_MAP_NAME) || defined(FIO_UMAP_NAME) || defined(FIO_SHA1) ||    \
    defined(FIO_MATH) || defined(FIO_CHACHA)
#ifndef FIO_BITWISE
#define FIO_BITWISE
#endif
#endif /* FIO_BITWISE */

/* Modules that require FIO_ATOMIC */
#if defined(FIO_BITMAP) || defined(FIO_REF_NAME) || defined(FIO_LOCK2) ||      \
    defined(FIO_STATE) || (defined(FIO_POLL) && !FIO_USE_THREAD_MUTEX_TMP) ||  \
    (defined(FIO_MEMORY_NAME) || defined(FIO_MALLOC)) ||                       \
    (defined(FIO_QUEUE) && !FIO_USE_THREAD_MUTEX_TMP) || defined(FIO_JSON) ||  \
    defined(FIO_SIGNAL) || defined(FIO_BITMAP) || defined(FIO_THREADS) ||      \
    defined(FIO_FIOBJ)
#ifndef FIO_ATOMIC
#define FIO_ATOMIC
#endif
#endif /* FIO_ATOMIC */

/* Modules that require FIO_ATOL */
#if defined(FIO_STR) || defined(FIO_QUEUE) || defined(FIO_TIME) ||             \
    defined(FIO_CLI) || defined(FIO_JSON) || defined(FIO_FILES) ||             \
    defined(FIO_FIOBJ) || defined(FIO_TEST_CSTL)
#ifndef FIO_ATOL
#define FIO_ATOL
#endif
#endif /* FIO_ATOL */
