/* *****************************************************************************
Copyright: Boaz Segev, 2019-2022
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
********************************************************************************

********************************************************************************
NOTE: this file is auto-generated from: https://github.com/facil-io/cstl
***************************************************************************** */

/** ****************************************************************************
# facil.io's C STL - Simple (type) Template Library

This file contains macros that create generic / common core types, such as:

* Linked Lists - defined by `FIO_LIST_NAME`

* Dynamic Arrays - defined by `FIO_ARRAY_NAME`

* Hash Maps / Sets - defined by `FIO_MAP_NAME`

* Binary Safe Dynamic Strings - defined by `FIO_STR_NAME` or `FIO_STR_SMALL`

* Reference counting / Type wrapper - defined by `FIO_REF_NAME` (adds atomic)

* Pointer Tagging for Types - defined by `FIO_PTR_TAG(p)`/`FIO_PTR_UNTAG(p)`

* Soft / Dynamic Types (FIOBJ) - defined by `FIO_FIOBJ`


This file also contains common helper macros / primitives, such as:

* Macro Stringifier - `FIO_MACRO2STR(macro)`

* Version Macros - i.e., `FIO_VERSION_MAJOR` / `FIO_VERSION_STRING`

* Pointer Math - i.e., `FIO_PTR_MATH_ADD` / `FIO_PTR_FROM_FIELD`

* Memory Allocation Macros - i.e., `FIO_MEM_REALLOC`

* Security Related macros - i.e., `FIO_MEM_STACK_WIPE`

* String Information Helper Type - `fio_str_info_s` / `FIO_STR_INFO_IS_EQ`

* Naming Macros - i.e., `FIO_NAME` / `FIO_NAME2` / `FIO_NAME_BL`

* OS portable Threads - defined by `FIO_THREADS`

* OS portable file helpers - defined by `FIO_FILES`

* Sleep / Thread Scheduling Macros - i.e., `FIO_THREAD_RESCHEDULE`

* Logging and Assertion (no heap allocation) - defined by `FIO_LOG`

* Atomic add/subtract/replace - defined by `FIO_ATOMIC`

* Bit-Byte Operations - defined by `FIO_BITWISE` and `FIO_BITMAP` (adds atomic)

* Data Hashing (using Risky Hash) - defined by `FIO_RISKY_HASH`

* Psedo Random Generation - defined by `FIO_RAND`

* String / Number conversion - defined by `FIO_ATOL`

* Time Helpers - defined by `FIO_TIME`

* Task / Timer Queues (Event Loop Engine) - defined by `FIO_QUEUE`

* Command Line Interface helpers - defined by `FIO_CLI`

* Socket Helpers - defined by `FIO_SOCK`

* Polling Helpers - defined by `FIO_POLL`

* Data Stream Containers - defined by `FIO_STREAM`

* Signal (pass-through) Monitors - defined by `FIO_SIGNAL`

* Custom Memory Pool / Allocation - defined by `FIO_MEMORY_NAME` / `FIO_MALLOC`,
  if `FIO_MALLOC` is used, it updates `FIO_MEM_REALLOC` etc'

* Custom JSON Parser - defined by `FIO_JSON`

However, this file does very little unless specifically requested.

To make sure this file defines a specific macro or type, it's macro should be
set.

In addition, if the `FIO_TEST_CSTL` macro is defined, the self-testing function
`fio_test_dynamic_types()` will be defined. the `fio_test_dynamic_types`
function will test the functionality of this file and, as consequence, will
define all available macros.

**Notes**:

- To make this file usable for kernel authoring, the `include` statements should
be reviewed.

- To make these functions safe for kernel authoring, the `FIO_MEM_REALLOC` and
`FIO_MEM_FREE` macros should be (re)-defined.

  These macros default to using the `realloc` and `free` functions calls. If
  `FIO_MALLOC` was defined, these macros will default to the custom memory
  allocator.

- To make the custom memory allocator safe for kernel authoring, the
  `FIO_MEM_PAGE_ALLOC`, `FIO_MEM_PAGE_REALLOC` and `FIO_MEM_PAGE_FREE` macros
  should be redefined. These macros default to using `mmap` and `munmap` (on
  linux, also `mremap`).

- The functions defined using this file default to `static` or `static
  inline`.

  To create an externally visible API, define the `FIO_EXTERN`. Define the
  `FIO_EXTERN_COMPLETE` macro to include the API's implementation as well.

- To implement a library style version guard, define the `FIO_VERSION_GUARD`
macro in a single translation unit (.c file) **before** including this STL
library for the first time.

***************************************************************************** */

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
Dynamic Endian Test
***************************************************************************** */

FIO_IFUNC unsigned int fio_is_little_endian(void) {
  union {
    unsigned long ul;
    unsigned char u8[sizeof(size_t)];
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
#define FIO_MEMCPY1(dest, src)  __builtin_memcpy((dest), (src), 1)
#define FIO_MEMCPY2(dest, src)  __builtin_memcpy((dest), (src), 2)
#define FIO_MEMCPY4(dest, src)  __builtin_memcpy((dest), (src), 4)
#define FIO_MEMCPY8(dest, src)  __builtin_memcpy((dest), (src), 8)
#define FIO_MEMCPY16(dest, src) __builtin_memcpy((dest), (src), 16)
#define FIO_MEMCPY32(dest, src) __builtin_memcpy((dest), (src), 32)
#define FIO_MEMCPY64(dest, src) __builtin_memcpy((dest), (src), 64)
#else
#ifndef FIO_MEMCPY
#define FIO_MEMCPY memcpy
#endif
#define FIO_MEMCPY1(dest, src)  fio___memcpy1((dest), (src))
#define FIO_MEMCPY2(dest, src)  fio___memcpy2((dest), (src))
#define FIO_MEMCPY4(dest, src)  fio___memcpy4((dest), (src))
#define FIO_MEMCPY8(dest, src)  fio___memcpy8((dest), (src))
#define FIO_MEMCPY16(dest, src) fio___memcpy16((dest), (src))
#define FIO_MEMCPY32(dest, src) fio___memcpy32((dest), (src))
#define FIO_MEMCPY64(dest, src) fio___memcpy64((dest), (src))

#define FIO___MAKE_MEMCPY_FIXED(bytes)                                         \
  FIO_IFUNC void fio___memcpy##bytes(void *dest, const void *src) {            \
    struct fio___memcpy##bytes##_s {                                           \
      char data[bytes];                                                        \
    };                                                                         \
    union {                                                                    \
      const void *ptr;                                                         \
      struct fio___memcpy##bytes##_s *grp;                                     \
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
#undef FIO___MAKE_MEMCPY_FIXED

#endif /* __has_builtin(__builtin_memcpy) */

#define FIO_MEMCPY7x(dest, src, len)  fio___memcpy7x((dest), (src), (len))
#define FIO_MEMCPY15x(dest, src, len) fio___memcpy15x((dest), (src), (len))
#define FIO_MEMCPY31x(dest, src, len) fio___memcpy31x((dest), (src), (len))
#define FIO_MEMCPY63x(dest, src, len) fio___memcpy63x((dest), (src), (len))

/** Copies up to 7 bytes to `dest` from `src`, calculated by `len & 7`. */
FIO_IFUNC void fio___memcpy7x(void *restrict d_,
                              const void *restrict s_,
                              size_t l) {
  char *d = (char *)d_, *s = (char *)s_;
  size_t i = 0;
#if FIO_MEMCPYX_UNROLL
  switch ((l & 7)) {
  case 7: d[i] = s[i]; ++i; /* fall through */
  case 6: d[i] = s[i]; ++i; /* fall through */
  case 5: d[i] = s[i]; ++i; /* fall through */
  case 4: d[i] = s[i]; ++i; /* fall through */
  case 3: d[i] = s[i]; ++i; /* fall through */
  case 2: d[i] = s[i]; ++i; /* fall through */
  case 1: d[i] = s[i]; ++i; /* fall through */
  }
#else
  l &= 7;
  for (;;) {
    if (i == l)
      break;
    d[i] = s[i];
    ++i;
  }
#endif
}

/** Copies up to 15 bytes to `dest` from `src`, calculated by `len & 15`. */
FIO_IFUNC void fio___memcpy15x(void *restrict dest_,
                               const void *restrict src_,
                               size_t len) {
  char *dest = (char *)dest_;
  const char *src = (const char *)src_;
  if ((len & 8)) {
    FIO_MEMCPY8(dest, src);
    dest += 8;
    src += 8;
  }
  fio___memcpy7x(dest, src, len);
}
/** Copies up to 31 bytes to `dest` from `src`, calculated by `len & 31`. */
FIO_IFUNC void fio___memcpy31x(void *restrict dest_,
                               const void *restrict src_,
                               size_t len) {
  char *dest = (char *)dest_;
  const char *src = (const char *)src_;
  if ((len & 16)) {
    FIO_MEMCPY16(dest, src);
    dest += 16;
    src += 16;
  }
  if ((len & 8)) {
    FIO_MEMCPY8(dest, src);
    dest += 8;
    src += 8;
  }
  fio___memcpy7x(dest, src, len);
}
/** Copies up to 63 bytes to `dest` from `src`, calculated by `len & 63`. */
FIO_IFUNC void fio___memcpy63x(void *restrict dest_,
                               const void *restrict src_,
                               size_t len) {
  char *dest = (char *)dest_;
  const char *src = (const char *)src_;
  if ((len & 32)) {
    FIO_MEMCPY32(dest, src);
    dest += 32;
    src += 32;
  }
  if ((len & 16)) {
    FIO_MEMCPY16(dest, src);
    dest += 16;
    src += 16;
  }
  if ((len & 8)) {
    FIO_MEMCPY8(dest, src);
    dest += 8;
    src += 8;
  }
  fio___memcpy7x(dest, src, len);
}

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
    (n)->next = (n)->prev = (n);                                               \
  } while (0)

/** UNSAFE macro for removing a node from a list. Resets node data. */
#define FIO_LIST_REMOVE_RESET(n)                                               \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
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
#define FIO___LOCK_INIT          (FIO_LOCK_INIT)
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
/* SFUNC_ - internal helper types are `static` */
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
    defined(FIOBJ_MALLOC) || defined(FIO_POLL) || defined(FIO_SERVER)
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
#if defined(FIO_POLL) || defined(FIO_SERVER)
#ifndef FIO_SOCK
#define FIO_SOCK
#endif
#endif

/* Modules that require FIO_QUEUE */
#if defined(FIO_POLL) || defined(FIO_SERVER)
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

/* Modules that require Threads data */
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
    defined(FIO_MAP_VALUE_BSTR) || defined(FIO_SERVER)
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
    defined(FIO_TEST_CSTL)
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

/* Modules that require FIO_RISKY_HASH */
#if defined(FIO_RAND) || defined(FIO_STATE) || defined(FIO_STR_NAME) ||        \
    defined(FIO_STR_SMALL) || defined(FIO_CLI) || defined(FIO_MEMORY_NAME) ||  \
    defined(FIO_MALLOC) || defined(FIO_POLL) || defined(FIO_TEST_CSTL)
#ifndef FIO_RISKY_HASH
#define FIO_RISKY_HASH
#endif
#endif /* FIO_RISKY_HASH */

/* Modules that require FIO_MATH */
#if defined(FIO_RISKY_HASH) || defined(FIO_CHACHA) || defined(FIO_TEST_CSTL)
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

/* Modules that require FIO_BITWISE (includes FIO_RISKY_HASH requirements) */
#if defined(FIO_STR_NAME) || defined(FIO_RISKY_HASH) || defined(FIO_JSON) ||   \
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
