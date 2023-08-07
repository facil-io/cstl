/* *****************************************************************************




                    Core Header - Stuff required by everything


Note:

The core header can't be well ordered due to cascading dependencies.
Please refer to the core documentation in the Markdown File.
***************************************************************************** */
#ifndef H___FIO_CORE___H
#define H___FIO_CORE___H

/** An empty macro, adding white space. Used to avoid function like macros. */
#define FIO_NOOP

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
#define FIO_VERSION_BUILD "alpha.06"

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

/* *****************************************************************************
Settings - Behavioral defaults
***************************************************************************** */

#ifndef FIO_USE_THREAD_MUTEX
/** Selects between facio.io's spinlocks (false) and OS mutexes (true) */
#define FIO_USE_THREAD_MUTEX 0
#endif

#ifndef FIO_UNALIGNED_ACCESS
/** Allows facil.io to attempt unaligned memory access on *some* CPU systems. */
#define FIO_UNALIGNED_ACCESS 1
#endif

#ifndef FIO_LIMIT_INTRINSIC_BUFFER
/* limits register consumption on some pseudo-intrinsics, using more loops */
#define FIO_LIMIT_INTRINSIC_BUFFER 1
#endif

#ifndef FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT
/* Memory allocations should be secure by default (facil.io allocators only) */
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT 1
#endif

#ifndef FIO_MEM_PAGE_SIZE_LOG
#define FIO_MEM_PAGE_SIZE_LOG 12 /* assumes 4096 bytes per page */
#endif

#if defined(FIO_NO_LOG) && defined(FIO_LEAK_COUNTER)
#error FIO_NO_LOG and FIO_LEAK_COUNTER are exclusive, as memory leaks print to log.
#endif

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
/* C keyword - unavailable in C++ */
#ifndef _Bool
#define _Bool bool
#endif

#endif

/* *****************************************************************************
Compiler detection, GCC / CLang features and OS dependent included files
***************************************************************************** */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if !defined(__GNUC__) && !defined(__clang__) && !defined(GNUC_BYPASS)
#ifndef __attribute__
#define __attribute__(...)
#endif
#ifndef __has_include
#define __has_include(...) 0
#endif
#ifndef __has_builtin
#define __has_builtin(...) 0
#endif
#ifndef __has_attribute
#define __has_attribute(...) 0
#endif
#define GNUC_BYPASS 1
#elif !defined(__clang__) && !defined(__has_builtin)
/* E.g: GCC < 6.0 doesn't support __has_builtin */
#define __has_builtin(...) 0
#define GNUC_BYPASS        1
#endif

#ifndef __has_include
#define __has_include(...) 0
#define GNUC_BYPASS        1
#endif

/* *****************************************************************************
Compiler Helpers - Deprecation, Alignment, Inlining, Memory Barriers
***************************************************************************** */

#ifndef DEPRECATED
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5))
/* GCC < 4.5 doesn't support deprecation reason string */
#define DEPRECATED(reason) __attribute__((deprecated))
#else
#define DEPRECATED(reason) __attribute__((deprecated(reason)))
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#define FIO_ALIGN(bytes) __attribute__((aligned(bytes)))
#elif defined(__INTEL_COMPILER) || defined(_MSC_VER)
#define FIO_ALIGN(bytes)
// #define FIO_ALIGN(bytes) __declspec(align(bytes))
#else
#define FIO_ALIGN(bytes)
#endif

#if _MSC_VER
#define inline   __inline
#define __thread __declspec(thread)
#elif !defined(__clang__) && !defined(__GNUC__)
#define __thread _Thread_local
#endif

#if defined(__clang__) || defined(__GNUC__)
/** Clobber CPU registers and prevent compiler reordering optimizations. */
#define FIO_COMPILER_GUARD             __asm__ volatile("" ::: "memory")
#define FIO_COMPILER_GUARD_INSTRUCTION __asm__ volatile("" :::)
#elif defined(_MSC_VER)
#include <intrin.h>
/** Clobber CPU registers and prevent compiler reordering optimizations. */
#define FIO_COMPILER_GUARD             _ReadWriteBarrier()
#define FIO_COMPILER_GUARD_INSTRUCTION _WriteBarrier()
#pragma message("Warning: Windows deprecated it's low-level C memory barrier.")
#else
#warning Unknown OS / compiler, some macros are poorly defined and errors might occur.
#define FIO_COMPILER_GUARD             asm volatile("" ::: "memory")
#define FIO_COMPILER_GUARD_INSTRUCTION asm volatile("" :::)
#endif

/* *****************************************************************************
Address Sanitizer Detection
***************************************************************************** */

/* Address Sanitizer Detection */
#if defined(__SANITIZE_ADDRESS__)
#define FIO___ASAN_DETECTED 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define FIO___ASAN_DETECTED 1
#endif
#endif /* address_sanitizer */

#ifdef FIO___ASAN_DETECTED
#if defined(_MSC_VER)
#define FIO___ASAN_AVOID __declspec(no_sanitize_address)
#else
#define FIO___ASAN_AVOID                                                       \
  __attribute__((no_sanitize_address)) __attribute__((no_sanitize("address")))
#endif
#else
#define FIO___ASAN_AVOID
#endif

/* *****************************************************************************
Intrinsic Availability Flags
***************************************************************************** */
#if defined(__ARM_FEATURE_CRYPTO) &&                                           \
    (defined(__ARM_NEON) || defined(__ARM_NEON__))
#include <arm_acle.h>
#include <arm_neon.h>
#define FIO___HAS_ARM_INTRIN 1
#endif

/* *****************************************************************************
Aligned Memory Access Selectors
***************************************************************************** */

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
OS Specific includes and Macros
***************************************************************************** */

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#define FIO_HAVE_UNIX_TOOLS 1
#define FIO_OS_POSIX        1
#define FIO___KILL_SELF()   kill(0, SIGINT)

#elif defined(_WIN32) || defined(_WIN64) || defined(WIN32) ||                  \
    defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#define FIO_OS_WIN     1
#define POSIX_C_SOURCE 200809L
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#undef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS 1
#ifndef UNICODE
#define UNICODE 1
#endif
#include <windows.h>
#endif /* WIN32_LEAN_AND_MEAN */

#include <fcntl.h>
#include <io.h>
#include <processthreadsapi.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <sysinfoapi.h>
#include <time.h>
#include <winsock2.h> /* struct timeval is here... why? Microsoft. */

#define FIO___KILL_SELF() TerminateProcess(GetCurrentProcess(), 1)

#if defined(__MINGW32__)
/* Mingw supports */
#define FIO_HAVE_UNIX_TOOLS    2
#define __USE_MINGW_ANSI_STDIO 1
#define FIO___PRINTF_STYLE(string_index, check_index)                          \
  __attribute__((format(__MINGW_PRINTF_FORMAT, string_index, check_index)))
#elif defined(__CYGWIN__)
/* TODO: cygwin support */
#define FIO_HAVE_UNIX_TOOLS    3
#define __USE_MINGW_ANSI_STDIO 1
#define FIO___PRINTF_STYLE(string_index, check_index)                          \
  __attribute__((format(__MINGW_PRINTF_FORMAT, string_index, check_index)))
#else
#define FIO_HAVE_UNIX_TOOLS 0
typedef SSIZE_T ssize_t;
#endif /* __CYGWIN__ __MINGW32__ */

#if _MSC_VER
#pragma message("Warning: some functionality is enabled by patchwork.")
#else
#warning some functionality is enabled by patchwork.
#endif

#else
#define FIO_HAVE_UNIX_TOOLS 0
#warning Unknown OS / compiler, some macros are poorly defined and errors might occur.
#endif /* OS / Compiler detection */

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

/* assume GCC / Clang style if no specific style provided. */
#ifndef FIO___PRINTF_STYLE
#define FIO___PRINTF_STYLE(string_index, check_index)                          \
  __attribute__((format(printf, string_index, check_index)))
#endif

/* *****************************************************************************
Function Attributes
***************************************************************************** */

#ifndef FIO_SFUNC
/** Marks a function as `static` and possibly unused. */
#define FIO_SFUNC static __attribute__((unused))
#endif

#ifndef FIO_IFUNC
/** Marks a function as `static`, `inline` and possibly unused. */
#define FIO_IFUNC FIO_SFUNC inline
#endif

#ifndef FIO_WEAK
/** Marks a function as weak */
#define FIO_WEAK __attribute__((weak))
#endif

/* *****************************************************************************
Constructors and Destructors
***************************************************************************** */

#if _MSC_VER
#pragma section(".CRT$XCU", read)
/** Marks a function as a constructor - if supported. */
#if _WIN64 /* MSVC linker uses different name mangling on 32bit systems */
/* clang-format off */
#define FIO_CONSTRUCTOR(fname)                                                 \
  static void fname(void);                                                     \
  __declspec(allocate(".CRT$XCU")) void (*FIO_NAME(fio___constructor, __COUNTER__))(void) = fname; \
  static void fname(void)
#else
#define FIO_CONSTRUCTOR(fname)                                                 \
  static void fname(void);                                                     \
  __declspec(allocate(".CRT$XCU")) void (*FIO_NAME(fio___constructor, __COUNTER__))(void) = fname; \
  static void fname(void)
#endif /* _WIN64 */
#define FIO_DESTRUCTOR(fname)                                                  \
  static void fname(void);                                                     \
  FIO_CONSTRUCTOR(fname##__hook) { atexit(fname); }                            \
  static void fname(void)
/* clang-format on */

#else
/** Marks a function as a constructor - if supported. */
#define FIO_CONSTRUCTOR(fname)                                                 \
  static __attribute__((constructor)) void fname(void)
/** Marks a function as a destructor - if supported. Consider using atexit() */
#define FIO_DESTRUCTOR(fname) static __attribute__((destructor)) void name(void)
#endif

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
Pointer Math
***************************************************************************** */

/** Masks a pointer's left-most bits, returning the right bits. */
#define FIO_PTR_MATH_LMASK(T_type, ptr, bits)                                  \
  ((T_type *)(((uintptr_t)(ptr)) & (((uintptr_t)1ULL << (bits)) - 1)))

/** Masks a pointer's right-most bits, returning the left bits. */
#define FIO_PTR_MATH_RMASK(T_type, ptr, bits)                                  \
  ((T_type *)(((uintptr_t)(ptr)) & ((~(uintptr_t)0ULL) << (bits))))

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
Logging Defaults (no-op)
***************************************************************************** */

/* avoid printing a full / nested path when __FILE_NAME__ is available */
#ifdef __FILE_NAME__
#define FIO__FILE__ __FILE_NAME__
#else
#define FIO__FILE__ __FILE__
#endif

/** Logging level of zero (no logging). */
#define FIO_LOG_LEVEL_NONE 0
/** Log fatal errors. */
#define FIO_LOG_LEVEL_FATAL 1
/** Log errors and fatal errors. */
#define FIO_LOG_LEVEL_ERROR 2
/** Log warnings, errors and fatal errors. */
#define FIO_LOG_LEVEL_WARNING 3
/** Log every message (info, warnings, errors and fatal errors). */
#define FIO_LOG_LEVEL_INFO 4
/** Log everything, including debug messages. */
#define FIO_LOG_LEVEL_DEBUG 5

/** Sets the Logging Level */
#define FIO_LOG_LEVEL_SET(new_level) (0)
/** Returns the Logging Level */
#define FIO_LOG_LEVEL_GET() (0)

// clang-format off
#define FIO___LOG_PRINT_LEVEL(level, ...) do { if ((level) <= FIO_LOG_LEVEL_GET()) {FIO_LOG2STDERR(__VA_ARGS__);} } while (0)
#define FIO_LOG_WRITE(...)    FIO_LOG2STDERR("(" FIO__FILE__ ":" FIO_MACRO2STR(__LINE__) "): " __VA_ARGS__)
#define FIO_LOG_FATAL(...)    FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_FATAL, "\x1B[1m\x1B[7mFATAL:\x1B[0m    " __VA_ARGS__)
#define FIO_LOG_ERROR(...)    FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_ERROR, "\x1B[1mERROR:\x1B[0m    " __VA_ARGS__)
#define FIO_LOG_SECURITY(...) FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_ERROR, "\x1B[1mSECURITY:\x1B[0m " __VA_ARGS__)
#define FIO_LOG_WARNING(...)  FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_WARNING, "\x1B[2mWARNING:\x1B[0m  " __VA_ARGS__)
#define FIO_LOG_INFO(...)     FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_INFO, "INFO:     " __VA_ARGS__)
#define FIO_LOG_DEBUG(...)    FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_DEBUG,"DEBUG:    (" FIO__FILE__ ":" FIO_MACRO2STR(__LINE__) ") " __VA_ARGS__)
#define FIO_LOG_DEBUG2(...)   FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_DEBUG, "DEBUG:    " __VA_ARGS__)
// clang-format on

#ifdef DEBUG
#define FIO_LOG_DDEBUG(...)           FIO_LOG_DEBUG(__VA_ARGS__)
#define FIO_LOG_DDEBUG2(...)          FIO_LOG_DEBUG2(__VA_ARGS__)
#define FIO_ASSERT___PERFORM_SIGNAL() FIO___KILL_SELF();
#else
#define FIO_LOG_DDEBUG(...)  ((void)(0))
#define FIO_LOG_DDEBUG2(...) ((void)(0))
#define FIO_ASSERT___PERFORM_SIGNAL()
#endif /* DEBUG */

#ifndef FIO_LOG_LENGTH_LIMIT
/** Defines a point at which logging truncates (limits stack memory use) */
#define FIO_LOG_LENGTH_LIMIT 1024
#endif

/** Prints to STDERR, attempting to use only stack allocated memory. */
#define FIO_LOG2STDERR(...)

/* *****************************************************************************
Assertions
***************************************************************************** */

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
  unsigned char data[2];
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

FIO_IFUNC unsigned int fio_is_big_endian(void) {
  return !fio_is_little_endian();
}

/* *****************************************************************************
Security Related macros
***************************************************************************** */
#define FIO_MEM_STACK_WIPE(pages)                                              \
  do {                                                                         \
    volatile char stack_mem[(pages) << 12] = {0};                              \
    (void)stack_mem;                                                           \
  } while (0)

/* *****************************************************************************
Sleep / Thread Scheduling Macros
***************************************************************************** */

#ifndef FIO_THREAD_WAIT
#if FIO_OS_WIN
/** Calls NtDelayExecution with the requested nano-second count. */
#define FIO_THREAD_WAIT(nano_sec)                                              \
  do {                                                                         \
    Sleep(((nano_sec) / 1000000) ? ((nano_sec) / 1000000) : 1);                \
  } while (0)
// https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep

#elif FIO_OS_POSIX
/** Calls nanonsleep with the requested nano-second count. */
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
String and Buffer Information Containers + Helper Macros
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
  ((s1).len == (s2).len &&                                                     \
   (!(s1).len || (s1).buf == (s2).buf ||                                       \
    ((s1).buf && (s2).buf && (s1).buf[0] == (s2).buf[0] &&                     \
     !FIO_MEMCMP((s1).buf, (s2).buf, (s1).len))))

/** Compares two `fio_buf_info_s` objects for content equality. */
#define FIO_BUF_INFO_IS_EQ(s1, s2) FIO_STR_INFO_IS_EQ((s1), (s2))

/** A NULL fio_str_info_s. */
#define FIO_STR_INFO0 ((fio_str_info_s){0})

/** Converts a C String into a fio_str_info_s. */
#define FIO_STR_INFO1(str)                                                     \
  ((fio_str_info_s){.len = ((str) ? FIO_STRLEN((str)) : 0), .buf = (str)})

/** Converts a String with a known length into a fio_str_info_s. */
#define FIO_STR_INFO2(str, length)                                             \
  ((fio_str_info_s){.len = (length), .buf = (str)})

/** Converts a String with a known length and capacity into a fio_str_info_s. */
#define FIO_STR_INFO3(str, length, capacity)                                   \
  ((fio_str_info_s){.len = (length), .buf = (str), .capa = (capacity)})

/** A NULL fio_buf_info_s. */
#define FIO_BUF_INFO0 ((fio_buf_info_s){0})

/** Converts a C String into a fio_buf_info_s. */
#define FIO_BUF_INFO1(str)                                                     \
  ((fio_buf_info_s){.len = ((str) ? FIO_STRLEN((str)) : 0), .buf = (str)})

/** Converts a String with a known length into a fio_buf_info_s. */
#define FIO_BUF_INFO2(str, length)                                             \
  ((fio_buf_info_s){.len = (length), .buf = (str)})

/** Converts a fio_buf_info_s into a fio_str_info_s. */
#define FIO_BUF2STR_INFO(buf_info)                                             \
  ((fio_str_info_s){.len = (buf_info).len, .buf = (buf_info).buf})

/** Converts a fio_buf_info_s into a fio_str_info_s. */
#define FIO_STR2BUF_INFO(str_info)                                             \
  ((fio_buf_info_s){.len = (str_info).len, .buf = (str_info).buf})

/** Creates a stack fio_str_info_s variable `name` with `capacity` bytes. */
#define FIO_STR_INFO_TMP_VAR(name, capacity)                                   \
  char fio___stack_mem___##name[(capacity) + 1];                               \
  fio___stack_mem___##name[(capacity)] = 0; /* guard */                        \
  fio_str_info_s name = (fio_str_info_s) {                                     \
    .buf = fio___stack_mem___##name, .capa = (capacity)                        \
  }

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
Settings - Memory Function Selectors
***************************************************************************** */
#ifdef FIO_MEMALT
#ifndef FIO_MEMCPY
#define FIO_MEMCPY fio_memcpy
#endif
#ifndef FIO_MEMMOVE
#define FIO_MEMMOVE fio_memcpy
#endif
#ifndef FIO_MEMCMP
#define FIO_MEMCMP fio_memcmp
#endif
#ifndef FIO_MEMCHR
#define FIO_MEMCHR fio_memchr
#endif
#ifndef FIO_MEMSET
#define FIO_MEMSET fio_memset
#endif
#ifndef FIO_STRLEN
#define FIO_STRLEN fio_strlen
#endif
#endif /* FIO_MEMALT */

/* memcpy selectors / overriding */
#ifndef FIO_MEMCPY
#if __has_builtin(__builtin_memcpy)
/** `memcpy` selector macro */
#define FIO_MEMCPY __builtin_memcpy
#else
/** `memcpy` selector macro */
#define FIO_MEMCPY memcpy
#endif
#endif /* FIO_MEMCPY */

/* memmove selectors / overriding */
#ifndef FIO_MEMMOVE
#if __has_builtin(__builtin_memmove)
/** `memmov` selector macro */
#define FIO_MEMMOVE __builtin_memmove
#else
/** `memmov` selector macro */
#define FIO_MEMMOVE memmove
#endif
#endif /* FIO_MEMMOVE */

/* memset selectors / overriding */
#ifndef FIO_MEMSET
#if __has_builtin(__builtin_memset)
/** `memset` selector macro */
#define FIO_MEMSET __builtin_memset
#else
/** `memset` selector macro */
#define FIO_MEMSET memset
#endif
#endif /* FIO_MEMSET */

/* memchr selectors / overriding */
#ifndef FIO_MEMCHR
#if __has_builtin(__builtin_memchr)
/** `memchr` selector macro */
#define FIO_MEMCHR __builtin_memchr
#else
/** `memchr` selector macro */
#define FIO_MEMCHR memchr
#endif
#endif /* FIO_MEMCHR */

/* strlen selectors / overriding */
#ifndef FIO_STRLEN
#if __has_builtin(__builtin_strlen)
/** `strlen` selector macro */
#define FIO_STRLEN __builtin_strlen
#else
/** `strlen` selector macro */
#define FIO_STRLEN strlen
#endif
#endif /* FIO_STRLEN */

/* memcmp selectors / overriding */
#ifndef FIO_MEMCMP
#if __has_builtin(__builtin_memcmp)
/** `memcmp` selector macro */
#define FIO_MEMCMP __builtin_memcmp
#else
/** `memcmp` selector macro */
#define FIO_MEMCMP memcmp
#endif
#endif /* FIO_MEMCMP */

/* *****************************************************************************
Memory Copying Primitives (the basis for unaligned memory access for numbers)
***************************************************************************** */

/* memcpy selectors / overriding */
#if __has_builtin(__builtin_memcpy)
#define FIO___MAKE_MEMCPY_FIXED(bytes)                                         \
  FIO_SFUNC void *fio_memcpy##bytes(void *restrict d,                          \
                                    const void *restrict s) {                  \
    return __builtin_memcpy(d, s, bytes);                                      \
  }
#else
#define FIO___MAKE_MEMCPY_FIXED(bytes)                                         \
  FIO_SFUNC void *fio_memcpy##bytes(void *restrict d,                          \
                                    const void *restrict s) {                  \
    void *const r = (char *)d + bytes;                                         \
    for (size_t i = 0; i < bytes; ++i) /* compiler, please vectorize */        \
      ((char *)d)[i] = ((const char *)s)[i];                                   \
    return r;                                                                  \
  }
#endif /* __has_builtin(__builtin_memcpy) */

/** No-op (completes the name space). */
FIO_SFUNC void *fio_memcpy0(void *restrict d, const void *restrict s) {
  ((void)s);
  return d;
}
/** Copies 1 byte from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(1)
/** Copies 2 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(2)
/** Copies 3 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(3)
/** Copies 4 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(4)
/** Copies 5 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(5)
/** Copies 6 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(6)
/** Copies 7 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(7)
/** Copies 8 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(8)
/** Copies 16 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(16)
/** Copies 32 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(32)
/** Copies 64 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(64)
/** Copies 128 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(128)
/** Copies 256 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(256)
/** Copies 512 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(512)
/** Copies 1024 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(1024)
/** Copies 2048 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(2048)
/** Copies 4096 bytes from `src` (`s`) to `dest` (`d`). */
FIO___MAKE_MEMCPY_FIXED(4096)
#undef FIO___MAKE_MEMCPY_FIXED

/** an unsafe memcpy (no checks + no overlapping memory regions) up to 63B */
FIO_IFUNC void *fio___memcpy_unsafe_63x(void *restrict d_,
                                        const void *restrict s_,
                                        size_t l) {
  char *restrict d = (char *restrict)d_;
  const char *restrict s = (const char *restrict)s_;
#define FIO___MEMCPY_XX_GROUP(bytes)                                           \
  do {                                                                         \
    fio_memcpy##bytes(d, s);                                                   \
    d += l & (bytes - 1);                                                      \
    s += l & (bytes - 1);                                                      \
    fio_memcpy##bytes(d, s);                                                   \
    return (void *)(d += bytes);                                               \
  } while (0)
  if (l > 31)
    FIO___MEMCPY_XX_GROUP(32);
  if (l > 15)
    FIO___MEMCPY_XX_GROUP(16);
  if (l > 7)
    FIO___MEMCPY_XX_GROUP(8);
#undef FIO___MEMCPY_XX_GROUP
  if ((l & 4)) {
    fio_memcpy4(d, s);
    (d += 4), (s += 4);
  }
  if ((l & 2)) {
    fio_memcpy2(d, s);
    (d += 2), (s += 2);
  }
  if ((l & 1))
    *d++ = *s;
  return (void *)d;
}
/** an unsafe memcpy (no checks + assumes no overlapping memory regions) */
FIO_SFUNC void *fio___memcpy_unsafe_x(void *restrict d_,
                                      const void *restrict s_,
                                      size_t l) {
  char *restrict d = (char *restrict)d_;
  const char *restrict s = (const char *restrict)s_;
  if (l < 64)
    return fio___memcpy_unsafe_63x(d_, s_, l);
#define FIO___MEMCPY_UNSAFE_STEP(bytes)                                        \
  do {                                                                         \
    fio_memcpy##bytes(d, s);                                                   \
    (l -= bytes), (d += bytes), (s += bytes);                                  \
  } while (0)

#if FIO_LIMIT_INTRINSIC_BUFFER
  while (l > 127)
    FIO___MEMCPY_UNSAFE_STEP(128);
#else
  while (l > 255)
    FIO___MEMCPY_UNSAFE_STEP(256);
  if (l & 128)
    FIO___MEMCPY_UNSAFE_STEP(128);
#endif
  if (l & 64)
    FIO___MEMCPY_UNSAFE_STEP(64);
#undef FIO___MEMCPY_UNSAFE_STEP
  d -= 64;
  s -= 64;
  d += l & 63U;
  s += l & 63U;
  fio_memcpy64(d, s);
  return (void *)(d += 64);
}

#define FIO___MEMCPYX_MAKER(lim, fn)                                           \
  FIO_SFUNC void *fio_memcpy##lim##x(void *restrict d,                         \
                                     const void *restrict s,                   \
                                     size_t l) {                               \
    return fn(d, s, (l & lim));                                                \
  }

/** No-op (completes the name space). */
FIO_SFUNC void *fio_memcpy0x(void *d, const void *s, size_t l) {
  ((void)s), ((void)l);
  return d;
}
/** Copies up to (len & 7) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(7, fio___memcpy_unsafe_63x)
/** Copies up to (len & 15) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(15, fio___memcpy_unsafe_63x)
/** Copies up to (len & 31) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(31, fio___memcpy_unsafe_63x)
/** Copies up to (len & 63) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(63, fio___memcpy_unsafe_63x)
/** Copies up to (len & 127) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(127, fio___memcpy_unsafe_x)
/** Copies up to (len & 255) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(255, fio___memcpy_unsafe_x)
/** Copies up to (len & 511) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(511, fio___memcpy_unsafe_x)
/** Copies up to (len & 1023) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(1023, fio___memcpy_unsafe_x)
/** Copies up to (len & 2047) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(2047, fio___memcpy_unsafe_x)
/** Copies up to (len & 4095) bytes from `src` (`s`) to `dest` (`d`). */
FIO___MEMCPYX_MAKER(4095, fio___memcpy_unsafe_x)
#undef FIO___MEMCPYX_MAKER

/* *****************************************************************************
Swapping byte's order (`bswap` variations)
***************************************************************************** */

/* avoid special cases by defining for all sizes */
#define fio_bswap8(i) (i)

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

#ifdef __SIZEOF_INT128__
#if __has_builtin(__builtin_bswap128)
#define fio_bswap128(i) __builtin_bswap128((__uint128_t)(i))
#else
FIO_IFUNC __uint128_t fio_bswap128(__uint128_t i) {
  return ((__uint128_t)fio_bswap64(i) << 64) | fio_bswap64(i >> 64);
}
#endif
#endif /* __SIZEOF_INT128__ */

/* *****************************************************************************
Switching Endian Ordering
***************************************************************************** */

#define fio_ltole8(i) (i) /* avoid special cases by defining for all sizes */
#define fio_lton8(i)  (i) /* avoid special cases by defining for all sizes */
#define fio_ntol8(i)  (i) /* avoid special cases by defining for all sizes */

#if __BIG_ENDIAN__

/** Local byte order to Network byte order, 16 bit integer */
#define fio_lton16(i) (i)
/** Local byte order to Network byte order, 32 bit integer */
#define fio_lton32(i) (i)
/** Local byte order to Network byte order, 62 bit integer */
#define fio_lton64(i) (i)

/** Local byte order to Little Endian byte order, 16 bit integer */
#define fio_ltole16(i) fio_bswap16((i))
/** Local byte order to Little Endian byte order, 32 bit integer */
#define fio_ltole32(i) fio_bswap32((i))
/** Local byte order to Little Endian byte order, 62 bit integer */
#define fio_ltole64(i) fio_bswap64((i))

/** Network byte order to Local byte order, 16 bit integer */
#define fio_ntol16(i) (i)
/** Network byte order to Local byte order, 32 bit integer */
#define fio_ntol32(i) (i)
/** Network byte order to Local byte order, 62 bit integer */
#define fio_ntol64(i) (i)

#ifdef __SIZEOF_INT128__
/** Network byte order to Local byte order, 128 bit integer */
#define fio_ntol128(i) (i)
/** Local byte order to Little Endian byte order, 128 bit integer */
#define fio_ltole128(i) fio_bswap128((i))

/** An endianess dependent shift operation, moves bytes forwards. */
#define FIO_SHIFT_FORWARDS(i, bits) ((i) >> (bits))
/** An endianess dependent shift operation, moves bytes backwards. */
#define FIO_SHIFT_BACKWARDS(i, bits) ((i) << (bits))

#endif /* __SIZEOF_INT128__ */

#else /* Little Endian */

/** Local byte order to Network byte order, 16 bit integer */
#define fio_lton16(i)  fio_bswap16((i))
/** Local byte order to Network byte order, 32 bit integer */
#define fio_lton32(i)  fio_bswap32((i))
/** Local byte order to Network byte order, 62 bit integer */
#define fio_lton64(i)  fio_bswap64((i))

/** Local byte order to Little Endian byte order, 16 bit integer */
#define fio_ltole16(i) (i)
/** Local byte order to Little Endian byte order, 32 bit integer */
#define fio_ltole32(i) (i)
/** Local byte order to Little Endian byte order, 62 bit integer */
#define fio_ltole64(i) (i)

/** Network byte order to Local byte order, 16 bit integer */
#define fio_ntol16(i)  fio_bswap16((i))
/** Network byte order to Local byte order, 32 bit integer */
#define fio_ntol32(i)  fio_bswap32((i))
/** Network byte order to Local byte order, 62 bit integer */
#define fio_ntol64(i)  fio_bswap64((i))

#ifdef __SIZEOF_INT128__
/** Local byte order to Network byte order, 128 bit integer */
#define fio_lton128(i)  fio_bswap128((i))
/** Network byte order to Local byte order, 128 bit integer */
#define fio_ntol128(i)  fio_bswap128((i))
/** Local byte order to Little Endian byte order, 128 bit integer */
#define fio_ltole128(i) (i)
#endif /* __SIZEOF_INT128__ */

/** An endianess dependent shift operation, moves bytes forwards. */
#define FIO_SHIFT_FORWARDS(i, bits)  ((i) << (bits))
/** An endianess dependent shift operation, moves bytes backwards. */
#define FIO_SHIFT_BACKWARDS(i, bits) ((i) >> (bits))

#endif /* __BIG_ENDIAN__ */

/* *****************************************************************************
Unaligned memory read / write operations
***************************************************************************** */

/** Converts an unaligned byte stream to an 8 bit number. */
FIO_IFUNC uint8_t fio_buf2u8u(const void *c) { return *(const uint8_t *)c; }
/** Writes a local 8 bit number to an unaligned buffer. */
FIO_IFUNC void fio_u2buf8u(void *buf, uint8_t i) { *((uint8_t *)buf) = i; }
/** Converts an unaligned byte stream to an 8 bit number. */
FIO_IFUNC uint8_t fio_buf2u8_le(const void *c) { return *(const uint8_t *)c; }
/** Writes a local 8 bit number to an unaligned buffer. */
FIO_IFUNC void fio_u2buf8_le(void *buf, uint8_t i) { *((uint8_t *)buf) = i; }
/** Converts an unaligned byte stream to an 8 bit number. */
FIO_IFUNC uint8_t fio_buf2u8_be(const void *c) { return *(const uint8_t *)c; }
/** Writes a local 8 bit number to an unaligned buffer. */
FIO_IFUNC void fio_u2buf8_be(void *buf, uint8_t i) { *((uint8_t *)buf) = i; }

#define FIO___U2U_NOOP(i) (i)
#define FIO___MEMBUF_FN(bytes, n_bits, bits, wrapper, postfix)                 \
  /** Converts an unaligned byte stream to a bits bit number. */               \
  FIO_IFUNC uint##bits##_t fio_buf2u##n_bits##postfix(const void *c) {         \
    uint##bits##_t tmp;                                                        \
    fio_memcpy##bytes(&tmp, c);                                                \
    return wrapper(tmp);                                                       \
  }                                                                            \
  /** Writes a bits bit number to an unaligned buffer. */                      \
  FIO_IFUNC void fio_u2buf##n_bits##postfix(void *buf, uint##bits##_t i) {     \
    i = wrapper(i);                                                            \
    fio_memcpy##bytes(buf, &i);                                                \
  }
/* unspecified byte order (native ordering) */
FIO___MEMBUF_FN(2, 16, 16, FIO___U2U_NOOP, u)
FIO___MEMBUF_FN(4, 32, 32, FIO___U2U_NOOP, u)
FIO___MEMBUF_FN(8, 64, 64, FIO___U2U_NOOP, u)
/* little endian byte order (native ordering) */
FIO___MEMBUF_FN(2, 16, 16, fio_ltole16, _le)
FIO___MEMBUF_FN(4, 32, 32, fio_ltole32, _le)
FIO___MEMBUF_FN(8, 64, 64, fio_ltole64, _le)
/* big / network endian byte order (native ordering) */
FIO___MEMBUF_FN(2, 16, 16, fio_lton16, _be)
FIO___MEMBUF_FN(4, 32, 32, fio_lton32, _be)
FIO___MEMBUF_FN(8, 64, 64, fio_lton64, _be)
#undef FIO___MEMBUF_FN

/** Converts an unaligned byte stream to a 24 bit number. */
FIO_IFUNC uint32_t fio_buf2u24u(const void *c) {
  uint32_t tmp = 0;
  fio_memcpy3(&tmp, c);
#if __BIG_ENDIAN__
  c = c >> 8;
#endif
  return tmp;
} /** Writes a 24 bit number to an unaligned buffer. */
FIO_IFUNC void fio_u2buf24u(void *buf, uint32_t i) {
#if __BIG_ENDIAN__
  i = i << 8;
#endif
  fio_memcpy3(buf, &i);
}

/** Converts an unaligned byte stream to a 24 bit number. */
FIO_IFUNC uint32_t fio_buf2u24_le(const void *c) {
  uint32_t tmp = ((uint32_t)((uint8_t *)c)[0]) |
                 ((uint32_t)((uint8_t *)c)[1] << 8) |
                 ((uint32_t)((uint8_t *)c)[2] << 16);
  return tmp;
} /** Writes a 24 bit number to an unaligned buffer. */
FIO_IFUNC void fio_u2buf24_le(void *buf, uint32_t i) {
  ((uint8_t *)buf)[0] = i & 0xFFU;
  ((uint8_t *)buf)[1] = (i >> 8) & 0xFFU;
  ((uint8_t *)buf)[2] = (i >> 16) & 0xFFU;
}
/** Converts an unaligned byte stream to a 24 bit number. */
FIO_IFUNC uint32_t fio_buf2u24_be(const void *c) {
  uint32_t tmp = ((uint32_t)((uint8_t *)c)[0] << 16) |
                 ((uint32_t)((uint8_t *)c)[1] << 8) |
                 ((uint32_t)((uint8_t *)c)[2]);
  return tmp;
} /** Writes a 24 bit number to an unaligned buffer. */
FIO_IFUNC void fio_u2buf24_be(void *buf, uint32_t i) {
  ((uint8_t *)buf)[0] = (i >> 16) & 0xFFU;
  ((uint8_t *)buf)[1] = (i >> 8) & 0xFFU;
  ((uint8_t *)buf)[2] = (i)&0xFFU;
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

#if __LITTLE_ENDIAN__
/** Rotates the bits Forwards (endian specific). */
#define fio_frot16 fio_rrot16
/** Rotates the bits Forwards (endian specific). */
#define fio_frot32 fio_rrot32
/** Rotates the bits Forwards (endian specific). */
#define fio_frot64 fio_rrot64
#else
/** Rotates the bits Forwards (endian specific). */
#define fio_frot16 fio_lrot16
/** Rotates the bits Forwards (endian specific). */
#define fio_frot32 fio_lrot32
/** Rotates the bits Forwards (endian specific). */
#define fio_frot64 fio_lrot64
#endif

/* *****************************************************************************
Byte masking (XOR)
***************************************************************************** */

/**
 * Masks data using a persistent 64 bit mask.
 *
 * When the buffer's memory is aligned, the function may perform significantly
 * better.
 */
FIO_IFUNC void fio_xmask(char *buf_, size_t len, uint64_t mask) {
  register char *buf = (char *)buf_;
  for (size_t i = 31; i < len; i += 32) {
    for (size_t g = 0; g < 4; ++g) {
      fio_u2buf64u(buf, (fio_buf2u64u(buf) ^ mask));
      buf += 8;
    }
  }
  if (len & 16)
    for (size_t g = 0; g < 2; ++g) {
      fio_u2buf64u(buf, (fio_buf2u64u(buf) ^ mask));
      buf += 8;
    }
  if (len & 8) {
    fio_u2buf64u(buf, (fio_buf2u64u(buf) ^ mask));
    buf += 8;
  }
  if (len & 7) {
    uint64_t tmp;
    fio_memcpy7x(&tmp, buf, len);
    tmp ^= mask;
    fio_memcpy7x(buf, &tmp, len);
  }
}

/**
 * Masks data using a persistent 64 bit mask.
 *
 * When the buffer's memory is aligned, the function may perform significantly
 * better.
 */
FIO_IFUNC void fio_xmask_cpy(char *restrict dest,
                             const char *src,
                             size_t len,
                             uint64_t mask) {
  if (dest == src) {
    fio_xmask(dest, len, mask);
    return;
  }
  for (size_t i = 31; i < len; i += 32) {
    for (size_t g = 0; g < 4; ++g) {
      fio_u2buf64u(dest, (fio_buf2u64u(src) ^ mask));
      dest += 8;
      src += 8;
    }
  }
  if (len & 16)
    for (size_t g = 0; g < 2; ++g) {
      fio_u2buf64u(dest, (fio_buf2u64u(src) ^ mask));
      dest += 8;
      src += 8;
    }
  if (len & 8) {
    fio_u2buf64u(dest, (fio_buf2u64u(src) ^ mask));
    dest += 8;
    src += 8;
  }
  if (len & 7) {
    uint64_t tmp;
    fio_memcpy7x(&tmp, src, len);
    tmp ^= mask;
    fio_memcpy7x(dest, &tmp, len);
  }
}

/* *****************************************************************************
Popcount (set bit counting) and Hemming Distance
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
Bit Mapping (placed here to avoid dependency between FIO_MEMALT and FIO_MATH)
***************************************************************************** */
#if !defined(__has_builtin) || !__has_builtin(__builtin_ctzll) ||              \
    !__has_builtin(__builtin_clzll)
FIO_SFUNC size_t fio___single_bit_index_unsafe(uint64_t i) {
  switch (i) {
  case UINT64_C(0x1): return 0;
  case UINT64_C(0x2): return 1;
  case UINT64_C(0x4): return 2;
  case UINT64_C(0x8): return 3;
  case UINT64_C(0x10): return 4;
  case UINT64_C(0x20): return 5;
  case UINT64_C(0x40): return 6;
  case UINT64_C(0x80): return 7;
  case UINT64_C(0x100): return 8;
  case UINT64_C(0x200): return 9;
  case UINT64_C(0x400): return 10;
  case UINT64_C(0x800): return 11;
  case UINT64_C(0x1000): return 12;
  case UINT64_C(0x2000): return 13;
  case UINT64_C(0x4000): return 14;
  case UINT64_C(0x8000): return 15;
  case UINT64_C(0x10000): return 16;
  case UINT64_C(0x20000): return 17;
  case UINT64_C(0x40000): return 18;
  case UINT64_C(0x80000): return 19;
  case UINT64_C(0x100000): return 20;
  case UINT64_C(0x200000): return 21;
  case UINT64_C(0x400000): return 22;
  case UINT64_C(0x800000): return 23;
  case UINT64_C(0x1000000): return 24;
  case UINT64_C(0x2000000): return 25;
  case UINT64_C(0x4000000): return 26;
  case UINT64_C(0x8000000): return 27;
  case UINT64_C(0x10000000): return 28;
  case UINT64_C(0x20000000): return 29;
  case UINT64_C(0x40000000): return 30;
  case UINT64_C(0x80000000): return 31;
  case UINT64_C(0x100000000): return 32;
  case UINT64_C(0x200000000): return 33;
  case UINT64_C(0x400000000): return 34;
  case UINT64_C(0x800000000): return 35;
  case UINT64_C(0x1000000000): return 36;
  case UINT64_C(0x2000000000): return 37;
  case UINT64_C(0x4000000000): return 38;
  case UINT64_C(0x8000000000): return 39;
  case UINT64_C(0x10000000000): return 40;
  case UINT64_C(0x20000000000): return 41;
  case UINT64_C(0x40000000000): return 42;
  case UINT64_C(0x80000000000): return 43;
  case UINT64_C(0x100000000000): return 44;
  case UINT64_C(0x200000000000): return 45;
  case UINT64_C(0x400000000000): return 46;
  case UINT64_C(0x800000000000): return 47;
  case UINT64_C(0x1000000000000): return 48;
  case UINT64_C(0x2000000000000): return 49;
  case UINT64_C(0x4000000000000): return 50;
  case UINT64_C(0x8000000000000): return 51;
  case UINT64_C(0x10000000000000): return 52;
  case UINT64_C(0x20000000000000): return 53;
  case UINT64_C(0x40000000000000): return 54;
  case UINT64_C(0x80000000000000): return 55;
  case UINT64_C(0x100000000000000): return 56;
  case UINT64_C(0x200000000000000): return 57;
  case UINT64_C(0x400000000000000): return 58;
  case UINT64_C(0x800000000000000): return 59;
  case UINT64_C(0x1000000000000000): return 60;
  case UINT64_C(0x2000000000000000): return 61;
  case UINT64_C(0x4000000000000000): return 62;
  case UINT64_C(0x8000000000000000): return 63;
  }
  return (0ULL - 1ULL);
}
#endif /* __builtin_ctzll || __builtin_clzll */

/** Returns the index of the least significant (lowest) bit. */
FIO_SFUNC size_t fio_lsb_index_unsafe(uint64_t i) {
#if defined(__has_builtin) && __has_builtin(__builtin_ctzll)
  return __builtin_ctzll(i);
#else
  return fio___single_bit_index_unsafe(i & ((~i) + 1));
#endif /* __builtin vs. map */
}

/** Returns the index of the most significant (highest) bit. */
FIO_SFUNC size_t fio_msb_index_unsafe(uint64_t i) {
#if defined(__has_builtin) && __has_builtin(__builtin_clzll)
  return 63 - __builtin_clzll(i);
#else
  i |= i >> 1;
  i |= i >> 2;
  i |= i >> 4;
  i |= i >> 8;
  i |= i >> 16;
  i |= i >> 32;
  i = ((i + 1) >> 1) | (i & ((uint64_t)1ULL << 63));
  return fio___single_bit_index_unsafe(i);
#endif /* __builtin vs. map */
}

/* *****************************************************************************
Byte Value helpers
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

/** Converts a `fio_has_byteX` result to a bitmap. */
FIO_IFUNC uint64_t fio_has_byte2bitmap(uint64_t result) {
  result = fio_ltole64(result); /* map little endian to bitmap */
  result >>= 7;                 /* move all 0x80 to 0x01 */
  result |= result >> 7;        /* pack all 0x80 bits into one byte */
  result |= result >> 14;
  result |= result >> 28;
  result &= 0xFFU;
  return result;
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
  return fio_msb_index_unsafe(i);
zero:
  return (size_t)-1;
}

/** Returns the index of the least significant (lowest) bit. */
FIO_IFUNC size_t fio_bits_lsb_index(uint64_t i) {
  if (!i)
    goto zero;
  return fio_lsb_index_unsafe(i);
zero:
  return (size_t)-1;
}

/* *****************************************************************************
Bitmap access / manipulation
***************************************************************************** */

/** Gets the state of a bit in a bitmap. */
FIO_IFUNC uint8_t fio_bit_get(void *map, size_t bit) {
  return ((((uint8_t *)(map))[(bit) >> 3] >> ((bit)&7)) & 1);
}

/** Sets the a bit in a bitmap (sets to 1). */
FIO_IFUNC void fio_bit_set(void *map, size_t bit) {
  ((uint8_t *)map)[bit >> 3] |= (uint8_t)(1UL << (bit & 7));
}

/** Unsets the a bit in a bitmap (sets to 0). */
FIO_IFUNC void fio_bit_unset(void *map, size_t bit) {
  ((uint8_t *)map)[bit >> 3] &= (uint8_t)(~(1UL << (bit & 7)));
}

/** Flips the a bit in a bitmap (sets to 0 if 1, sets to 1 if 0). */
FIO_IFUNC void fio_bit_flip(void *map, size_t bit) {
  ((uint8_t *)map)[bit >> 3] ^= (uint8_t)((1UL << (bit & 7)));
}

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
  FIO_ASSERT_DEBUG(carry_out, "fio_math_mulc64 requires a carry pointer");
#if defined(__SIZEOF_INT128__)
  __uint128_t r = (__uint128_t)a * b;
  *carry_out = (uint64_t)(r >> 64U);
#else /* At this point long multiplication makes sense... */
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
#endif
  return (uint64_t)r;
}

/* *****************************************************************************
C++ extern end
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
}
#endif

/* *****************************************************************************
End persistent segment (end include-once guard)
***************************************************************************** */
#endif /* H___FIO_CORE___H */
