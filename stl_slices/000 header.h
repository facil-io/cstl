/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
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

* Data Stream Containers - defined by `FIO_STREAM`

* Signal (passthrough) Monitors - defined by `FIO_SIGNAL`

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
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H
#define H___FIO_CSTL_INCLUDE_ONCE_H

/* *****************************************************************************
Basic macros and included files
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

#if !defined(__clang__) && !defined(__GNUC__)
#define __thread _Thread_value
#endif

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#define FIO_HAVE_UNIX_TOOLS 1
#elif defined(__CYGWIN__) || defined(__MINGW32__)
#define FIO_HAVE_UNIX_TOOLS 2
#endif

#if FIO_HAVE_UNIX_TOOLS
#include <sys/param.h>
#include <unistd.h>
#endif

#if FIO_UNALIGNED_ACCESS &&                                                    \
    (__amd64 || __amd64__ || __x86_64 || __x86_64__ || __i386 ||               \
     __aarch64__ || _M_IX86 || _M_X64 || _M_ARM64)
#define FIO_UNALIGNED_MEMORY_ACCESS_ENABLED 1
#else
#define FIO_UNALIGNED_MEMORY_ACCESS_ENABLED 0
#endif

/* memcpy selectors / overriding */
#ifndef FIO_MEMCPY
#if __has_builtin(__builtin_memcpy)
#define FIO_MEMCPY __builtin_memcpy
#else
#define FIO_MEMCPY memcpy
#endif
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
/** BETA version: pre-version development marker. Nothing is stable. */
#define FIO_VERSION_BETA 1

#if FIO_VERSION_BETA
/** Version as a String literal (MACRO). */
#define FIO_VERSION_STRING                                                     \
  FIO_MACRO2STR(FIO_VERSION_MAJOR)                                             \
  "." FIO_MACRO2STR(FIO_VERSION_MINOR) "." FIO_MACRO2STR(                      \
      FIO_VERSION_PATCH) ".beta" FIO_MACRO2STR(FIO_VERSION_BETA)
#else
/** Version as a String literal (MACRO). */
#define FIO_VERSION_STRING                                                     \
  FIO_MACRO2STR(FIO_VERSION_MAJOR)                                             \
  "." FIO_MACRO2STR(FIO_VERSION_MINOR) "." FIO_MACRO2STR(FIO_VERSION_PATCH)
#endif

/** If implemented, returns the major version number. */
size_t fio_version_major(void);
/** If implemented, returns the minor version number. */
size_t fio_version_minor(void);
/** If implemented, returns the patch version number. */
size_t fio_version_patch(void);
/** If implemented, returns the beta version number. */
size_t fio_version_beta(void);
/** If implemented, returns the version number as a string. */
char *fio_version_string(void);

#define FIO_VERSION_VALIDATE()                                                 \
  FIO_ASSERT(fio_version_major() == FIO_VERSION_MAJOR &&                       \
                 fio_version_minor() == FIO_VERSION_MINOR &&                   \
                 fio_version_patch() == FIO_VERSION_PATCH &&                   \
                 fio_version_beta() == FIO_VERSION_BETA,                       \
             "facil.io version mismatch, not %s",                              \
             fio_version_string())

/**
 * To implement the fio_version_* functions and FIO_VERSION_VALIDATE guard, the
 * `FIO_VERSION_GUARD` must be defined (only) once per application / library.
 */
#ifdef FIO_VERSION_GUARD
size_t __attribute__((weak)) fio_version_major(void) {
  return FIO_VERSION_MAJOR;
}
size_t __attribute__((weak)) fio_version_minor(void) {
  return FIO_VERSION_MINOR;
}
size_t __attribute__((weak)) fio_version_patch(void) {
  return FIO_VERSION_PATCH;
}
size_t __attribute__((weak)) fio_version_beta(void) { return FIO_VERSION_BETA; }
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

/** Find the root object (of a struct) from it's field. */
#define FIO_PTR_FROM_FIELD(T_type, field, ptr)                                 \
  FIO_PTR_MATH_SUB(T_type, ptr, (&((T_type *)0)->field))

/* *****************************************************************************
Security Related macros
***************************************************************************** */
#define FIO_MEM_STACK_WIPE(pages)                                              \
  do {                                                                         \
    volatile char stack_mem[(pages) << 12] = {0};                              \
    (void)stack_mem;                                                           \
  } while (0)

/* *****************************************************************************
String Information Helper Type
***************************************************************************** */

/** An information type for reporting the string's state. */
typedef struct fio_str_info_s {
  /** The string's buffer (pointer to first byte) or NULL on error. */
  char *buf;
  /** The string's length, if any. */
  size_t len;
  /** The buffer's capacity. Zero (0) indicates the buffer is read-only. */
  size_t capa;
} fio_str_info_s;

/** An information type for reporting/storing buffer data. */
typedef struct fio_buf_info_s {
  /** The string's buffer (pointer to first byte) or NULL on error. */
  char *buf;
  /** The string's length, if any. */
  size_t len;
} fio_buf_info_s;

/** Compares two `fio_str_info_s` objects for content equality. */
#define FIO_STR_INFO_IS_EQ(s1, s2)                                             \
  ((s1).len == (s2).len && (!(s1).len || (s1).buf == (s2).buf ||               \
                            !memcmp((s1).buf, (s2).buf, (s1).len)))

/* *****************************************************************************
Linked Lists Persistent Macros and Types
***************************************************************************** */

/** A common linked list node type. */
typedef struct fio___list_node_s {
  struct fio___list_node_s *next;
  struct fio___list_node_s *prev;
} fio___list_node_s;

/** A linked list node type */
#define FIO_LIST_NODE fio___list_node_s
/** A linked list head type */
#define FIO_LIST_HEAD fio___list_node_s

/** Allows initialization of FIO_LIST_HEAD objects. */
#define FIO_LIST_INIT(obj)                                                     \
  (obj) = (fio___list_node_s) { .next = &(obj), .prev = &(obj) }

#ifndef FIO_LIST_EACH
/** Loops through every node in the linked list except the head. */
#define FIO_LIST_EACH(type, node_name, head, pos)                              \
  for (type *pos = FIO_PTR_FROM_FIELD(type, node_name, (head)->next),          \
            *next____p_ls =                                                    \
                FIO_PTR_FROM_FIELD(type, node_name, (head)->next->next);       \
       pos != FIO_PTR_FROM_FIELD(type, node_name, (head));                     \
       (pos = next____p_ls),                                                   \
            (next____p_ls = FIO_PTR_FROM_FIELD(type,                           \
                                               node_name,                      \
                                               next____p_ls->node_name.next)))
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
    (n)->next = (n)->prev = NULL;                                              \
  } while (0)

/* *****************************************************************************
Indexed Linked Lists Persistent Macros and Types

Indexed Linked Lists can be used to create a linked list that uses is always
relative to some root pointer (usually the root of an array). This:

1. Allows easy reallocation of the list without requiring pointer updates.

2. Could be used for memory optimization if the array limits are known.

The "head" index is usualy validated by reserving the value of `-1` to indicate
an empty list.
***************************************************************************** */
#ifndef FIO_INDEXED_LIST_EACH
/** A common linked list node type. */
typedef struct fio___index32_node_s {
  uint32_t next;
  uint32_t prev;
} fio___index32_node_s;

/** A linked list node type */
#define FIO_INDEXED_LIST32_NODE fio___index32_node_s
#define FIO_INDEXED_LIST32_HEAD uint32_t

/** UNSAFE macro for pushing a node to a list. */
#define FIO_INDEXED_LIST_PUSH(root, node_name, head, i)                        \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[n__].node_name.prev = (root)[(head)].node_name.prev;                \
    (root)[n__].node_name.next = (head);                                       \
    (root)[(root)[(head)].node_name.prev].node_name.next = n__;                \
    (root)[(head)].node_name.prev = n__;                                       \
  } while (0)

/** UNSAFE macro for removing a node from a list. */
#define FIO_INDEXED_LIST_REMOVE(root, node_name, i)                            \
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
Sleep / Thread Scheduling Macros
***************************************************************************** */

#ifndef FIO_THREAD_WAIT
/**
 * Calls nonsleep with the requested nano-second count.
 */
#define FIO_THREAD_WAIT(nano_sec)                                              \
  do {                                                                         \
    const struct timespec tm = {.tv_sec = (time_t)((nano_sec) / 1000000000),   \
                                .tv_nsec = ((long)(nano_sec) % 1000000000)};   \
    nanosleep(&tm, (struct timespec *)NULL);                                   \
  } while (0)
#endif

#ifndef FIO_THREAD_RESCHEDULE
/**
 * Reschedules the thread by calling nanosleeps for a sinlge nano-second.
 *
 * In practice, the thread will probably sleep for 60ns or more.
 */
#define FIO_THREAD_RESCHEDULE() FIO_THREAD_WAIT(4)
#endif

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
#define FIO_LOG_DEBUG(...)
#define FIO_LOG_DEBUG2(...)
#define FIO_LOG_INFO(...)
#define FIO_LOG_WARNING(...)
#define FIO_LOG_ERROR(...)
#define FIO_LOG_SECURITY(...)
#define FIO_LOG_FATAL(...)
#define FIO_LOG2STDERR(...)
#define FIO_LOG2STDERR2(...)
#define FIO_LOG_PRINT__(...)

#ifndef FIO_LOG_LENGTH_LIMIT
/** Defines a point at which logging truncates (limited by stack memory) */
#define FIO_LOG_LENGTH_LIMIT 1024
#endif

// clang-format off
/* Asserts a condition is true, or kills the application using SIGINT. */
#define FIO_ASSERT(cond, ...)                                                  \
  if (!(cond)) {                                                               \
    FIO_LOG_FATAL("(" FIO__FILE__ ":" FIO_MACRO2STR(__LINE__) ") " __VA_ARGS__);  \
    perror("     errno");                                                      \
    kill(0, SIGINT);                                                           \
    exit(-1);                                                                  \
  }

#ifndef FIO_ASSERT_ALLOC
/** Tests for an allocation failure. The behavior can be overridden. */
#define FIO_ASSERT_ALLOC(ptr)  FIO_ASSERT((ptr), "memory allocation failed.")
#endif
// clang-format on

#ifdef DEBUG
/** If `DEBUG` is defined, acts as `FIO_ASSERT`, otherwise a NOOP. */
#define FIO_ASSERT_DEBUG(cond, ...) FIO_ASSERT(cond, __VA_ARGS__)
#else
#define FIO_ASSERT_DEBUG(...)
#endif

/** Marks a function as `static`, `inline` and possibly unused. */
#define FIO_IFUNC static inline __attribute__((unused))

/** Marks a function as `static` and possibly unused. */
#define FIO_SFUNC static __attribute__((unused))

/** Marks a function as weak */
#define FIO_WEAK __attribute__((weak))

/** Marks a function as a constructor - if supported. */
#define FIO_CONSTRUCTOR FIO_SFUNC __attribute__((constructor))

/** Marks a function as a destructor - if supported. */
#define FIO_DESTRUCTOR FIO_SFUNC __attribute__((destructor))

/* *****************************************************************************
End persistent segment (end include-once guard)
***************************************************************************** */
#endif /* H___FIO_CSTL_INCLUDE_ONCE_H */

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

#if !defined(FIO_MEM_REALLOC) || !defined(FIO_MEM_FREE)

#undef FIO_MEM_REALLOC
#undef FIO_MEM_FREE
#undef FIO_MEM_REALLOC_IS_SAFE

/* if a global allocator was previously defined route macros to fio_malloc */
#ifdef H___FIO_MALLOC___H
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
Common macros
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
#ifndef FIO_EXTERN_COMPLETE /* force implementation, emitting static data */
#define FIO_EXTERN_COMPLETE 2
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_EXTERN */

#undef SFUNC
#undef IFUNC
#define SFUNC SFUNC_
#define IFUNC IFUNC_

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

/**
 * If FIO_PTR_TAG_VALIDATE is defined, tagging will be verified before executing
 * any code.
 */
#ifdef FIO_PTR_TAG_VALIDATE
#define FIO_PTR_TAG_VALID_OR_RETURN(tagged_ptr, value)                         \
  do {                                                                         \
    if (!(FIO_PTR_TAG_VALIDATE(tagged_ptr))) {                                 \
      FIO_LOG_DEBUG("pointer tag (type) mismatch in function call.");          \
      return (value);                                                          \
    }                                                                          \
  } while (0)
#define FIO_PTR_TAG_VALID_OR_RETURN_VOID(tagged_ptr)                           \
  do {                                                                         \
    if (!(FIO_PTR_TAG_VALIDATE(tagged_ptr))) {                                 \
      FIO_LOG_DEBUG("pointer tag (type) mismatch in function call.");          \
      return;                                                                  \
    }                                                                          \
  } while (0)
#define FIO_PTR_TAG_VALID_OR_GOTO(tagged_ptr, lable)                           \
  do {                                                                         \
    if (!(FIO_PTR_TAG_VALIDATE(tagged_ptr))) {                                 \
      /* Log error since GOTO indicates cleanup or other side-effects. */      \
      FIO_LOG_ERROR("(" FIO__FILE__ ":" FIO_MACRO2STR(                         \
          __LINE__) ") pointer tag (type) mismatch in function call.");        \
      goto lable;                                                              \
    }                                                                          \
  } while (0)
#else
#define FIO_PTR_TAG_VALIDATE(tagged_ptr) 1
#define FIO_PTR_TAG_VALID_OR_RETURN(tagged_ptr, value)
#define FIO_PTR_TAG_VALID_OR_RETURN_VOID(tagged_ptr)
#define FIO_PTR_TAG_VALID_OR_GOTO(tagged_ptr, lable)                           \
  while (0) {                                                                  \
    goto lable;                                                                \
  }
#endif

#else /* SFUNC_ - internal helper types are `static` */
#undef SFUNC
#undef IFUNC
#define SFUNC FIO_SFUNC
#define IFUNC FIO_IFUNC
#endif /* SFUNC_ vs FIO_STL_KEEP__*/

/* *****************************************************************************



                          Internal Dependencies



***************************************************************************** */

/* FIO_MEMORY_NAME dependencies */
#if defined(FIO_MEMORY_NAME) || defined(FIO_MALLOC)
#ifndef FIO_LOG
#define FIO_LOG
#endif
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
#if defined(FIO_RAND) || defined(FIO_STR_NAME) || defined(FIO_STR_SMALL) ||    \
    defined(FIO_CLI)
#ifndef FIO_RISKY_HASH
#define FIO_RISKY_HASH
#endif
#endif /* FIO_RISKY_HASH */

/* Modules that require FIO_BITMAP */
#if defined(FIO_JSON)
#ifndef FIO_BITMAP
#define FIO_BITMAP
#endif
#endif /* FIO_BITMAP */

/* Modules that require FIO_BITWISE (includes FIO_RISKY_HASH requirements) */
#if defined(FIO_RISKY_HASH) || defined(FIO_JSON) || defined(FIO_MAP_NAME) ||   \
    defined(FIO_UMAP_NAME) || defined(FIO_SHA1)
#ifndef FIO_BITWISE
#define FIO_BITWISE
#endif
#endif /* FIO_BITWISE */

/* Modules that require FIO_ATOMIC */
#if defined(FIO_BITMAP) || defined(FIO_REF_NAME) || defined(FIO_LOCK2) ||      \
    defined(FIO_POLL) || defined(FIO_MEMORY_NAME) || defined(FIO_MALLOC) ||    \
    defined(FIO_QUEUE) || defined(FIO_JSON) || defined(FIO_SIGNAL) ||          \
    defined(FIO_BITMAP)
#ifndef FIO_ATOMIC
#define FIO_ATOMIC
#endif
#endif /* FIO_ATOMIC */

/* Modules that require FIO_ATOL */
#if defined(FIO_STR_NAME) || defined(FIO_STR_SMALL) || defined(FIO_QUEUE) ||   \
    defined(FIO_TIME) || defined(FIO_CLI) || defined(FIO_JSON)
#ifndef FIO_ATOL
#define FIO_ATOL
#endif
#endif /* FIO_ATOL */
