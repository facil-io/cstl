---
title: facil.io - C real-time web application framework
sidebar: 0.8.x/_sidebar.md
---
# facil.io - C Real-Time Web Application Framework

[facil.io](http://facil.io) focuses on providing solutions for real-time web applications. facil.io includes:

* A fast HTTP/1.1, Websocket and SSE application server.
* A zero-dependency Pub/Sub cluster-enabled message bus API.
* Performant JSON parsing and formatting for easy network communication.
* Dynamic types designed with web applications in mind (Strings, Hashes, Arrays etc').
* Support for custom network protocols for both server and client connections.
* An easy solution to [the C10K problem](http://www.kegel.com/c10k.html).
* Optional connectivity with Redis.

This header library is the **C Server Toolbox Library** that makes it all happen.

## OS Support

The library in written and tested on POSIX systems. Windows support was added afterwards, leaving the library with a POSIX oriented design.

Please note I cannot continually test the windows support as I avoid the OS... hence, Windows OS support should be considered unstable.

## Server Toolbox lIbrary (STL) Overview

At the core of the [facil.io library](https://facil.io) is its powerful Server Toolbox lIbrary for C (and C++).

The Server Toolbox lIbrary is a "swiss-army-knife" library, that uses MACROS to generate code for different common types, such as Hash Maps, Arrays, Linked Lists, Binary-Safe Strings, etc'.

The [testable](#testing-the-library-fio_test_cstl) header library includes a Server Toolbox lIbrary for the following common types:

* [Binary Safe Dynamic Strings](#dynamic-strings) - defined by `FIO_STR_NAME` / `FIO_STR_SMALL`

* [Dynamic Arrays](#dynamic-arrays) - defined by `FIO_ARRAY_NAME`

* [Hash Maps / Sets](#hash-tables-and-maps) - defined by `FIO_MAP_NAME`

* [Reference counting / Type wrapper](#reference-counting-and-type-wrapping) - defined by `FIO_REF_NAME`

* [Soft / Dynamic Types (FIOBJ)](#fiobj-soft-dynamic-types) - defined by `FIO_FIOBJ`


In addition, the core Server Toolbox lIbrary (STL) includes helpers for common tasks, such as:

* [Fast String / Number conversion](#string-number-conversion) - defined by `FIO_ATOL`

* [Logging and Assertion (without heap allocation)](#logging-and-assertions) - defined by `FIO_LOG`

* [Binary Safe String Helpers](#binary-safe-core-string-helpers) - defined by `FIO_STR`

* [Time Helpers](#time-helpers) - defined by `FIO_TIME`

* [File Utility Helpers](#file-utility-helpers) - defined by `FIO_FILES`

* [Command Line Interface helpers](#cli-command-line-interface) - defined by `FIO_CLI`

* [Task Queues and Timers](#task-queue) - defined by `FIO_QUEUE`

* [Local Memory Allocation](#local-memory-allocation) - defined by `FIO_MEMORY` / `FIO_MALLOC`

* [Atomic operations](#atomic-operations) - defined by `FIO_ATOMIC`

* [Pointer Arithmetics](#pointer-arithmetics) (included by default)

* And much more...

-------------------------------------------------------------------------------

## Compilation Modes

The Server Toolbox lIbrary types and functions could be compiled as either static or extern ("global"), either limiting their scope to a single C file (compilation unit) or exposing them throughout the program.

### Static Functions by Default

By default, facil.io will generate static functions where possible.

To change this behavior, `FIO_EXTERN` and `FIO_EXTERN_COMPLETE` could be used to generate externally visible code.

#### `FIO_EXTERN`

If defined, the the Server Toolbox lIbrary will generate non-static code.

If `FIO_EXTERN` is defined alone, only function declarations and inline functions will be generated.

If `FIO_EXTERN_COMPLETE` is defined, the function definition (the implementation code) will also be generated.

**Note**: the `FIO_EXTERN` will be **automatically undefined** each time the Server Toolbox lIbrary header is included, **unless** the `FIO_EXTERN` is defined with a **numerical** value other than `1` (a compiler default value in some cases), in which case the `FIO_EXTERN` definition will remain in force until manually removed.

For example, in the header (i.e., `mymem.h`), use:

```c
#define FIO_EXTERN
#define FIO_MALLOC
#include "fio-stl/include.h" /* or "fio-stl.h" */
/* FIO_EXTERN automatically undefined in this case */
```

Later, in the implementation file, use:

```c
#define FIO_EXTERN_COMPLETE 2
#include "mymem.h"
#undef FIO_EXTERN_COMPLETE
/* FIO_EXTERN_COMPLETE needed to be manually undefined in this case */
```

#### `FIO_EXTERN_COMPLETE`

When defined, this macro will force full code generation.

**Note**: the `FIO_EXTERN_COMPLETE` will be **automatically undefined** each time the Server Toolbox lIbrary header is included, **unless** the `FIO_EXTERN_COMPLETE` is defined with a **numerical** value other than `1` (a compiler default value in some cases), in which case the `FIO_EXTERN_COMPLETE` definition will remain in force until manually removed.

-------------------------------------------------------------------------------

## Thread Safety

Some modules require thread safety locks, such as the timer module, queue module, memory allocator and socket polling. The facil.io library will default to it's own spin-lock based implementation unless `FIO_USE_THREAD_MUTEX` or `FIO_USE_THREAD_MUTEX_TMP` are defined.

#### `FIO_USE_THREAD_MUTEX` and `FIO_USE_THREAD_MUTEX_TMP`

This default choice can be changed so facil.io uses the OS's native `mutex` type (`pthread_mutex_t` on POSIX systems) by setting the `FIO_USE_THREAD_MUTEX` or `FIO_USE_THREAD_MUTEX_TMP` to true (`1`).

The `FIO_USE_THREAD_MUTEX_TMP` macro will alter the default behavior for only a single `include` statement.

The `FIO_USE_THREAD_MUTEX` macro will alter the default behavior for all future `include` statements.

-------------------------------------------------------------------------------

## Unaligned Memory Access

By default facil.io attempts to automatically detect systems that allow for unaligned memory access and use optimizations that require this feature. This can be changed by setting `FIO_UNALIGNED_ACCESS` to `0`.

#### `FIO_UNALIGNED_ACCESS`

If set to true (`1`) this MACRO will attempt to detect support of unaligned memory access and if support is detected the `FIO_UNALIGNED_MEMORY_ACCESS_ENABLED` will be set to true (`1`).

#### `FIO_UNALIGNED_MEMORY_ACCESS_ENABLED`

If set to true (`1`) this MACRO will indicate that the facil.io library should allow for unaligned memory access, skipping memory alignment requirements in some cases (such as the in the `fio_buf2uXX` function implementation).

-------------------------------------------------------------------------------

## Multi-Module Inclusion Helpers

#### `FIO_CORE`

When `FIO_CORE` is defined, all core modules are included, such as `FIO_STR`, `FIO_ATOL`, and non-typed helpers.

#### `FIO_BASIC`

When `FIO_BASIC` is defined, the `FIOBJ` types, multi threading, and CLI modules are included in addition to the core modules.

#### `FIO_CRYPT`

When `FIO_CRYPT` is defined, all hash and cryptographic modules are included.

#### `FIO_SERVER_COMPLETE`

When `FIO_SERVER_COMPLETE` is defined all Server related modules are included.

#### `FIO_EVERYTHING`

Adds all the code facil.io C STL has to offer.

Custom types (templates) can't be created without specific instruction, but all functionality that can be included is included.

Note, `FIO_EVERYTHING` functions will always be `static` unless `FIO_EXTERN` was defined for specific functionality or `FIO_EXTERN` was defined in a persistent way (with a numerical value of `2` or greater).


-------------------------------------------------------------------------------

## Testing the Library (`FIO_TEST_ALL`)

To test the library, define the `FIO_TEST_ALL` macro and include the header. A testing function called `fio_test_dynamic_types` will be defined. Call that function in your code to test the library.

#### `FIO_TEST_ALL`

Defined the `fio_test_dynamic_types` and enables as many testing features as possible, such as the `FIO_LEAK_COUNTER`.

#### `FIO_LEAK_COUNTER`

If defined, facil.io will count allocations and deallocations for custom memory allocators and reference counted types - allowing memory leaks to be detected with certainty.

This also prints out some minimal usage information about each allocator when exiting the program. 

**Note**: enabling leak detection automatically adds the `FIO_LOG` module (to print errors), the `FIO_ATOMIC` module (for atomic counters) and the `FIO_STATE` module (for more predictable `at_exit` callbacks).

-------------------------------------------------------------------------------

## Version and Common Helper Macros

The facil.io C STL (Server Toolbox lIbrary) offers a number of common helper macros that are also used internally. These are automatically included once the `fio-stl.h` is included.

### Version Macros

The facil.io C STL library follows [semantic versioning](https://semver.org) and supports macros that will help detect and validate it's version.

#### `FIO_VERSION_MAJOR`

Translates to the STL's major version number.

MAJOR version upgrades require a code review and possibly significant changes. Even functions with the same name might change their behavior.

#### `FIO_VERSION_MINOR`

Translates to the STL's minor version number.

Please review your code before adopting a MINOR version upgrade.

#### `FIO_VERSION_PATCH`

Translates to the STL's patch version number.

PATCH versions should be adopted as soon as possible (they contain bug fixes).

#### `FIO_VERSION_BUILD`

Translates to the STL's build version **string** (i.e., `"beta.1"`), if any.

#### `FIO_VERSION_STRING`

Translates to the STL's version as a string (i.e., `"0.8.0-beta.1"`).

### Inclusion Macros

#### `FIO_INCLUDE_FILE`

The facil.io C STL can be used as either a single header library (`fio-stl.h`) or a multi-header library (`fio-stl/include.h`). The `FIO_INCLUDE_FILE` macro will remember which approach was first used and will use the same approach for subsequent inclusions.

-------------------------------------------------------------------------------

### Pointer Arithmetics

#### `FIO_PTR_MATH_LMASK`

```c
#define FIO_PTR_MATH_LMASK(T_type, ptr, bits)                                  \
  ((T_type *)((uintptr_t)(ptr) & (((uintptr_t)1 << (bits)) - 1)))
```

Masks a pointer's left-most bits, returning the right bits (i.e., `0x000000FF`).

#### `FIO_PTR_MATH_RMASK`

```c
#define FIO_PTR_MATH_RMASK(T_type, ptr, bits)                                  \
  ((T_type *)((uintptr_t)(ptr) & ((~(uintptr_t)0) << bits)))
```

Masks a pointer's right-most bits, returning the left bits (i.e., `0xFFFFFF00`).

#### `FIO_PTR_MATH_ADD`

```c
#define FIO_PTR_MATH_ADD(T_type, ptr, offset)                                  \
  ((T_type *)((uintptr_t)(ptr) + (uintptr_t)(offset)))
```

Add offset bytes to pointer's address, updating the pointer's type.

#### `FIO_PTR_MATH_SUB`

```c
#define FIO_PTR_MATH_SUB(T_type, ptr, offset)                                  \
  ((T_type *)((uintptr_t)(ptr) - (uintptr_t)(offset)))
```

Subtract X bytes from pointer's address, updating the pointer's type.

#### `FIO_PTR_FROM_FIELD`

```c
#define FIO_PTR_FROM_FIELD(T_type, field, ptr)                                 \
  FIO_PTR_MATH_SUB(T_type, ptr, (&((T_type *)0)->field))
```

Find the root object (of a `struct`) from a pointer to its field's (the field's address).

-------------------------------------------------------------------------------

### Default Memory Allocation

By setting these macros, the memory allocator used by facil.io could be changed from the default allocator (either the custom allocator or, if missing, the system's allocator).

When facil.io's memory allocator is defined (using `FIO_MALLOC`), **these macros will be automatically overwritten to use the custom memory allocator**. To use a different allocator, you may redefine the macros.

#### `FIO_MEM_REALLOC`

```c
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len) realloc((ptr), (new_size))
```

Reallocates memory, copying (at least) `copy_len` if necessary.

If `ptr` is `NULL`, behaves like `malloc`.

If `new_size` is 0, behaves like `free`.

#### `FIO_MEM_FREE`

```c
#define FIO_MEM_FREE(ptr, size) free((ptr))
```

Frees allocated memory.

#### `FIO_MALLOC_TMP_USE_SYSTEM`

When defined, temporarily bypasses the `FIO_MEM_REALLOC` macros and uses the system's `realloc` and `free` functions for newly created types.

#### `FIO_MEMORY_DISABLE`

When `FIO_MEMORY_DISABLE` is defined, all (future) custom memory allocators will route to the system's `malloc`. Set this when compiling to test the effects of all custom memory allocators working together.

-------------------------------------------------------------------------------

## Pointer Tagging Support:

Pointer tagging allows types created using this library to have their pointers "tagged".

This is when creating / managing dynamic types, where some type data could be written to the pointer data itself.

**Note**: pointer tagging can't automatically tag "pointers" to objects placed on the stack.

#### `FIO_PTR_TAG`

```c
#define FIO_PTR_TAG(p) (p)
```

Supports embedded pointer tagging / untagging for the included types.

Should resolve to a tagged pointer value. i.e.: `((uintptr_t)(p) | 1)`

#### `FIO_PTR_UNTAG`

```c
#define FIO_PTR_UNTAG(p) (p)
```

Supports embedded pointer tagging / untagging for the included types.

Should resolve to an untagged pointer value. i.e.: `((uintptr_t)(p) | ~1UL)`

**Note**: `FIO_PTR_UNTAG` might be called more then once or on untagged pointers. For this reason, `FIO_PTR_UNTAG` should always return the valid pointer, even if called on an untagged pointer.

#### `FIO_PTR_TAG_TYPE`

If the FIO_PTR_TAG_TYPE is defined, then functions returning a type's pointer will return a pointer of the specified type instead.


#### `FIO_PTR_TAG_VALIDATE`

```c
#define FIO_PTR_TAG_VALIDATE(ptr) ((ptr) != NULL)
```

If `FIO_PTR_TAG_VALIDATE` is defined, tagging will be verified before executing
any code.

`FIO_PTR_TAG_VALIDATE` **must** fail on NULL pointers.

-------------------------------------------------------------------------------

## Binary Data Informational Types and Helper Macros

Some informational types and helpers are always defined (similarly to the [Linked Lists Macros](#linked-lists-macros)). These include:

#### `fio_str_info_s`

Some functions use the `fio_str_info_s` type to either collect or return string related information. This helper type is defined as:

```c
typedef struct fio_str_info_s {
  size_t len;  /* The string's length, if any. */
  char *buf;   /* The string's buffer (pointer to first byte) or NULL on error. */
  size_t capa; /* The buffer's capacity. Zero (0) indicates the buffer is read-only. */
} fio_str_info_s;
```

Note that it is often the case that the data in the string object could be binary, where NUL is a valid character, so using C string functions isn't advised.

Also, note that `capa` might be `0` or otherwise less than `len`. This would indicate the data might be non-mutable (overwriting the string might break things).


#### `fio_buf_info_s`

```c
typedef struct fio_buf_info_s {
  size_t len; /* The buffer's length, if any. */
  char *buf;  /* The buffer's address (may be NULL if no buffer). */
} fio_buf_info_s;
```

An information type for reporting/storing buffer data (no `capa`). Note that the buffer may contain binary data and is **not** likely to be NUL terminated.

#### `FIO_STR_INFO_IS_EQ`

```c
#define FIO_STR_INFO_IS_EQ(s1, s2)                                             \
  ((s1).len == (s2).len && (!(s1).len || (s1).buf == (s2).buf ||               \
                            !memcmp((s1).buf, (s2).buf, (s1).len)))
```

This helper MACRO compares two `fio_str_info_s` / `fio_buf_info_s` objects for content content equality.

#### `FIO_STR_INFO1`

```c
#define FIO_STR_INFO1(str)                                                     \
  ((fio_str_info_s){.len = FIO_STRLEN((str)), .buf = (str)})
```

Converts a C String into a `fio_str_info_s`.

#### `FIO_STR_INFO2`

```c
#define FIO_STR_INFO2(str, length)                                             \
  ((fio_str_info_s){.len = (length), .buf = (str)})
```

Converts a String with a known length into a `fio_str_info_s`.

#### `FIO_STR_INFO3`

```c
#define FIO_STR_INFO3(str, length, capacity)                                   \
  ((fio_str_info_s){.len = (length), .buf = (str), .capa = (capacity)})
```

Converts a String with a known length and capacity into a `fio_str_info_s`.

#### `FIO_BUF2STR_INFO`

```c
#define FIO_BUF2STR_INFO(buf_info)                                             \
  ((fio_str_info_s){.len = (buf_info).len, .buf = (buf_info).buf})
```

Converts a `fio_buf_info_s` into a `fio_str_info_s`.

#### `FIO_STR2BUF_INFO`

```c
#define FIO_STR2BUF_INFO(str_info)                                             \
  ((fio_buf_info_s){.len = (str_info).len, .buf = (str_info).buf})
```

Converts a `fio_buf_info_s` into a `fio_str_info_s`.

#### `FIO_STR_INFO_TMP_VAR(name, capacity)`

```c
#define FIO_STR_INFO_TMP_VAR(name, capacity)                                   \
  char fio___stack_mem___##name[(capacity) + 1];                               \
  fio___stack_mem___##name[(capacity)] = 0; /* guard */                        \
  fio_str_info_s name = (fio_str_info_s) {                                     \
    .buf = fio___stack_mem___##name, .capa = (capacity)                        \
  }
```

Creates a stack fio_str_info_s variable `name` with `capacity` bytes (including 1 extra byte for a `NUL` guard).

-------------------------------------------------------------------------------

## Memory Copying, Seeking and Setting

The following macros are defined to allow for memory copying primitives of set sizes.

In addition an overrideble `FIO_MEMCPY` macro is provided that allows routing any variable sized memory copying to a different routine.

#### `FIO_MEMCPY`

```c
#ifndef
#define FIO_MEMCPY memcpy // or __builtin_memcpy if available
#endif
```

This macro makes it easy to override the `memcpy` implementation used by the library.

By default this will be set to either `memcpy` or `__builtin_memcpy` (if available). It can also be set to `fio_memcpy` if need be.

#### `FIO_MEMMOVE`

```c
#ifndef
#define FIO_MEMMOVE memmove // or __builtin_memmove if available
#endif
```

This macro makes it easy to override the `memmove` implementation used by the library.

By default this will be set to either `memmove` or `__builtin_memmove` (if available). It can also be set to `fio_memcpy` if need be.

#### `fio_memcpy##`

```c
static void * fio_memcpy0(void *restrict dest, const void *restrict src); /* no-op */
static void * fio_memcpy1(void *restrict dest, const void *restrict src);
static void * fio_memcpy2(void *restrict dest, const void *restrict src);
static void * fio_memcpy4(void *restrict dest, const void *restrict src);
static void * fio_memcpy8(void *restrict dest, const void *restrict src);
static void * fio_memcpy16(void *restrict dest, const void *restrict src);
static void * fio_memcpy32(void *restrict dest, const void *restrict src);
static void * fio_memcpy64(void *restrict dest, const void *restrict src);
// ... void * fio_memcpy4096
```

Copies a pre-defined `n` bytes from `src` to `dest` whhere `n` is a power of 2 between 1 and 4096 (including).

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than libc, depending on the compiler and available instruction sets / optimizations.

#### `fio_memcpy##x`

```c
static void * fio_memcpy7x(void *restrict dest, const void *restrict src), size_t length;
static void * fio_memcpy15x(void *restrict dest, const void *restrict src, size_t length);
static void * fio_memcpy31x(void *restrict dest, const void *restrict src, size_t length);
static void * fio_memcpy63x(void *restrict dest, const void *restrict src, size_t length);
// ... void * fio_memcpy4095x
```

Copies up to `n-1` bytes from `src` to `dest` where `n` is a power of 2 between 1 and 4096 (including).

This is provided to allow for easy "tail" processing.

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than libc, depending on the compiler and available instruction sets / optimizations.

#### `fio_memcpy`

```c
static void *fio_memcpy(void *dest, const void *src, size_t length);
```

A fallback for `memcpy` and `memmove`, copies `length` bytes from `src` to `dest`.

Behaves as `memmove`, allowing for copy between overlapping memory buffers. 

On most of `libc` implementations the library call will be faster. On embedded systems, test before deciding.

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than libc, depending on the compiler and available instruction sets / optimizations.

#### `FIO_MEMSET`

```c
#ifndef
#define FIO_MEMSET memset // or __builtin_memset if available
#endif
```

This macro makes it easy to override the `memset` implementation used by the library.

By default this will be set to either `memset` or `__builtin_memset` (if available). It can also be set to `fio_memset` if need be.

#### `fio_memset`

```c
static void *fio_memset(void *restrict dest, uint64_t token, size_t length);
```

A fallback for `memset`. Sets `length` bytes in the `dest` buffer to `token`.

The `token` can be either a single byte - in which case all bytes in `dest` will be set to `token` - or a 64 bit value which will be written repeatedly all over `dest` in local endian format (last copy may be partial).

On most of `libc` implementations the library call will be faster. On embedded systems, test before deciding.

Returns `dest` (the pointer originally received).

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than `libc`, depending on the compiler and available instruction sets / optimizations.

#### `FIO_MEMCHR`

```c
#ifndef
#define FIO_MEMCHR memchr // or __builtin_memchr if available
#endif
```

This macro makes it easy to override the `memchr` implementation used by the library.

By default this will be set to either `memchr` or `__builtin_memchr` (if available). It can also be set to `fio_memchr` if need be.

#### `fio_memchr`

```c
static void *fio_memchr(const void *buffer, const char token, size_t len);
```

A fallback for `memchr`, seeking a `token` in the number of `bytes` starting at the address of `mem`.

If `token` is found, returns the address of the token's first appearance. Otherwise returns `NULL`.

On most of `libc` implementations the library call will be faster. Test before deciding.

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than `libc`, depending on the compiler and available instruction sets / optimizations.

#### `fio_rawmemchr`

```c
static void *fio_rawmemchr(const void *buffer, const char token);
```

A fallback for `rawmemchr` (GNU), seeking a `token` that **must** (for certain) be in the memory starting at address `mem`.

If `token` is found, returns the address of the token's first appearance. Otherwise anything could happen, including the computer becoming sentient and trying to save humanity.

A fallback for `memchr_unsafe`, seeking a `token` in the number of `bytes` starting at the address of `mem`.

If `token` is found, returns the address of the token's first appearance. Otherwise returns `NULL`.

On most of `libc` implementations the library call will be faster. On embedded systems, test before deciding.

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than `libc`, depending on the compiler and available instruction sets / optimizations.

#### `FIO_MEMCMP`

```c
#ifndef
#define FIO_MEMCMP memcmp // or __builtin_memcmp if available
#endif
```

This macro makes it easy to override the `memcmp` implementation used by the library.

By default this will be set to either `memcmp` or `__builtin_memcmp` (if available). It can also be set to `fio_memcmp` if need be.

#### `fio_memcmp`

```c
static int fio_memcmp(const void *a, const void *b, size_t len);
```

A fallback for `memcmp`, comparing two memory regions by byte values.

Returns 1 if `a > b`, -1 if `a < b` and 0 if `a == b`.

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than `libc`, depending on the compiler and available instruction sets / optimizations.

#### `FIO_STRLEN`

```c
#ifndef
#define FIO_MEMCMP strlen // or __builtin_memstrlen if available
#endif
```

This macro makes it easy to override the `strlen` implementation used by the library.

By default this will be set to either `strlen` or `__builtin_strlen` (if available). It can also be set to `fio_strlen` if need be.

#### `fio_strlen`

```c
static size_t fio_strlen(const char *str);
```

A fallback for `strlen`, returning the length of the string.

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than `libc`, depending on the compiler and available instruction sets / optimizations.

#### `FIO_MEMALT`

If defined, defines all previously undefined memory macros to use facil.io's fallback options.

Note that this will also cause `__builtin_memcpy` to be bypassed for the fixed `fio_memcpy##` functions.

```c
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
```

-------------------------------------------------------------------------------

## Naming and Misc. Macros

#### `FIO_IFUNC`

```c
#define FIO_IFUNC static inline __attribute__((unused))
```

Marks a function as `static`, `inline` and possibly unused.

#### `FIO_SFUNC`

```c
#define FIO_SFUNC static __attribute__((unused))
```

Marks a function as `static` and possibly unused.

#### `FIO_WEAK`

```c
#define FIO_WEAK __attribute__((weak))
```

Marks a function as weak

#### `FIO_CONSTRUCTOR(fname)`

```c
#define FIO_CONSTRUCTOR(fname) FIO_SFUNC __attribute__((constructor)) void fname (void)
```

Marks a function as a _constructor_ - **if supported**.

When supported by the compiler (i.e., `gcc` / `clang`), this function will execute when the library is loaded or, if statically linked, before `main` is called.

#### `FIO_DESTRUCTOR(fname)`

```c
#define FIO_DESTRUCTOR(fname) FIO_SFUNC __attribute__((destructor)) void fname (void)
```

Marks a function as a _destructor_ - **if supported**.

When supported by the compiler (i.e., `gcc` / `clang`), this function will execute when the library is loaded or, if statically linked, after `main` returns.

#### `FIO_MACRO2STR`

```c
#define FIO_MACRO2STR(macro) FIO_MACRO2STR_STEP2(macro)
```

Converts a macro's content to a string literal.

#### `FIO_NAME`

```c
#define FIO_NAME(prefix, postfix)
```

Used for naming functions and variables resulting in: `prefix_postfix`

This allows macros to be used for naming types and functions.

i.e.:

```c
// the type's name
#define NUM number
// typedef struct { long l; } number_s
typedef struct { long l; } FIO_NAME(NUM, s)

// number_s number_add(number_s a, number_s b)
FIO_NAME(NUM, s) FIO_NAME(NUM, add)(FIO_NAME(NUM, s) a, FIO_NAME(NUM, s) b) {
  a.l += b.l;
  return a;
}
#undef NUM
```

#### `FIO_NAME2`

```c
#define FIO_NAME2(prefix, postfix)
```

Sets naming convention for conversion functions, i.e.: foo2bar

i.e.:

```c
// int64_t a2l(const char * buf)
int64_t FIO_NAME2(a, l)(const char * buf) {
  return fio_atol(&buf);
}
```

#### `FIO_NAME_BL`

```c
#define FIO_NAME_BL(prefix, postfix) 
```

Sets naming convention for boolean functions, i.e.: foo_is_true

i.e.:

```c
// typedef struct { long l; } number_s
typedef struct { long l; } FIO_NAME(number, s)

// int number_is_zero(number_s n)
int FIO_NAME2(number, zero)(FIO_NAME(number, s) n) {
  return (!n.l);
}
```

#### `FIO_NAME_TEST`

```c
#define FIO_NAME_TEST(prefix, postfix) FIO_NAME(fio___test, FIO_NAME(prefix, postfix))
```

Used internally to name test functions.

-------------------------------------------------------------------------------

## Byte Ordering and Copying - Little Endian vs. Big Endian

To help with byte ordering on different systems, the following macros and functions are defined. Note that there's no built-in support for mixed endian systems.

#### `__BIG_ENDIAN__`

Defined and set to either 1 (on big endian systems) or 0 (little endian systems)

#### `__LITTLE_ENDIAN__`

Defined and set to either 1 (on little endian systems) or 0 (big endian systems)

#### `fio_is_little_endian`, `fio_is_big_endian`

```c
unsigned int fio_is_little_endian(void);
unsigned int fio_is_big_endian(void);
```

These functions perform runtime tests for endianess ... but may be optimized away by the compiler.

#### `fio_bswap`

Returns a number of the indicated type with it's byte representation swapped.

- `fio_bswap16(i)`
- `fio_bswap32(i)`
- `fio_bswap64(i)`
- `fio_bswap128(i)` (only on compilers that support this type)

#### `fio_lton`, `fio_ntol`

On big-endian systems, the following macros a NOOPs. On little-endian systems these macros flip the byte order.

- `fio_lton16(i)`
- `fio_ntol16(i)`
- `fio_lton32(i)`
- `fio_ntol32(i)`
- `fio_lton64(i)`
- `fio_ntol64(i)`
- `fio_lton128(i)`
- `fio_ntol128(i)`

#### `fio_ltole`

Converts a local number to little-endian. On big-endian systems, these macros flip the byte order. On little-endian systems these macros are a NOOP.

- `fio_ltole16(i)`
- `fio_ltole32(i)`
- `fio_ltole64(i)`
- `fio_ltole128(i)`

#### Bit rotation (left / right)

Returns a number with it's bits left rotated (`lrot`) or right rotated (`rrot`) according to the type width specified (i.e., `fio_rrot64` indicates a **r**ight rotation for `uint64_t`).

- `fio_lrot8(i, bits)`
- `fio_rrot8(i, bits)`
- `fio_lrot16(i, bits)`
- `fio_rrot16(i, bits)`
- `fio_lrot32(i, bits)`
- `fio_rrot32(i, bits)`
- `fio_lrot64(i, bits)`
- `fio_rrot64(i, bits)`
- `fio_lrot128(i, bits)`
- `fio_rrot128(i, bits)`

- `FIO_LROT(i, bits)` (MACRO, can be used with any type size)
- `FIO_RROT(i, bits)` (MACRO, can be used with any type size)

#### Bytes to Numbers

Reads a number from an unaligned memory buffer. The number or **bits** read from the buffer is indicated by the name of the function.

**Big Endian**:

- `fio_buf2u16_be(buffer)`
- `fio_buf2u32_be(buffer)`
- `fio_buf2u64_be(buffer)`
- `fio_buf2u128_be(buffer)`

**Little Endian**:

- `fio_buf2u16_le(buffer)`
- `fio_buf2u32_le(buffer)`
- `fio_buf2u64_le(buffer)`
- `fio_buf2u128_le(buffer)`

**Native (Unspecified) Byte Order**:

- `fio_buf2u16u(buffer)`
- `fio_buf2u32u(buffer)`
- `fio_buf2u64u(buffer)`
- `fio_buf2u128u(buffer)`

#### Numbers to Bytes (native / reversed / network ordered)

Writes a number to an unaligned memory buffer. The number or bits written to the buffer is indicated by the name of the function.

**Big Endian (default)**:

- `fio_u2buf16_be(buffer, i)`
- `fio_u2buf32_be(buffer, i)`
- `fio_u2buf64_be(buffer, i)`
- `fio_u2buf128_be(buffer, i)`

**Little Endian**:

- `fio_u2buf16_le(buffer, i)`
- `fio_u2buf32_le(buffer, i)`
- `fio_u2buf64_le(buffer, i)`
- `fio_u2buf128_le(buffer, i)`

**Native (Unspecified) Byte Order**:

- `fio_u2buf16u(buffer, i)`
- `fio_u2buf32u(buffer, i)`
- `fio_u2buf64u(buffer, i)`
- `fio_u2buf128u(buffer, i)`

#### `fio_xmask`

```c
void fio_xmask(char *buf, size_t len, uint64_t mask);
```

Masks data using a 64 bit mask.

The function may perform significantly better when the buffer's memory is aligned.

#### Constant Time Helpers

Performs the operation indicated in constant time.

- `fio_ct_true(condition)`

    Tests if `condition` is non-zero (returns `1` / `0`).

- `fio_ct_false(condition)`

    Tests if `condition` is zero (returns `1` / `0`).

- `fio_ct_if_bool(bool, a_if_true, b_if_false)`

    Tests if `bool == 1` (returns `a` / `b`).

- `fio_ct_if(condition, a_if_true, b_if_false)`

    Tests if `condition` is non-zero (returns `a` / `b`).

- `fio_ct_max(a, b)`

    Returns `a` if a >= `b` (performs a **signed** comparison).


- `fio_ct_is_eq(const void * a, const void * b, len)`

    Returns 1 if memory regions are equal. Should be resistant to timing attacks.

-------------------------------------------------------------------------------

## Inspecting Byte / Bit Data

**Note**: the 128 bit helpers are only available with systems / compilers that support 128 bit types.

#### Bitmap helpers

- `fio_bit_get(void *map, size_t bit)`

- `fio_bit_set(void *map, size_t bit)`   (a **non-atomic** operation, **not** thread-safe)

- `fio_bit_unset(void *map, size_t bit)` (a **non-atomic** operation, **not** thread-safe)

Sets and un-sets bits in an arbitrary bitmap - non-atomic, **not** thread-safe.

#### `fio_popcount` and Hemming 

```c
int fio_popcount(uint64_t n);
```

Returns the number of set bits in the number `n`.

#### `fio_hemming_dist`

```c
#define fio_hemming_dist(n1, n2) fio_popcount(((uint64_t)(n1) ^ (uint64_t)(n2)))
```

Returns the Hemming Distance between the number `n1` and the number `n2`.

Hemming Distance is the number of bits that need to be "flipped" in order for both numbers to be equal.


#### `fio_has_full_byte32`

```c
uint32_t fio_has_full_byte32(uint32_t row)
```

  Detects a byte where all the bits are set (`255`) within a 4 byte vector.

#### `fio_has_zero_byte32`

```c
uint32_t fio_has_zero_byte32(uint32_t row)
```

  Detects a byte where no bits are set (0) within a 4 byte vector.

#### `fio_has_byte32`

```c
uint32_t fio_has_byte32(uint32_t row, uint8_t byte)
```

  Detects if `byte` exists within a 4 byte vector.

#### `fio_has_full_byte64`

```c
uint64_t fio_has_full_byte64(uint64_t row)
```

  Detects a byte where all the bits are set (`255`) within an 8 byte vector.

#### `fio_has_zero_byte64`

```c
uint64_t fio_has_zero_byte64(uint64_t row)
```

  Detects a byte where no bits are set (byte == 0) within an 8 byte vector.

#### `fio_has_byte64`

```c
uint64_t fio_has_byte64(uint64_t row, uint8_t byte)
```

  Detects if `byte` exists within an 8 byte vector.

#### `fio_has_full_byte128`

```c
__uint128_t fio_has_full_byte128(__uint128_t row)
```

    Detects a byte where all the bits are set (`255`) within an 8 byte vector.

#### `fio_has_zero_byte128`

```c
__uint128_t fio_has_zero_byte128(__uint128_t row)
```

    Detects a byte where no bits are set (0) within an 8 byte vector.

#### `fio_has_byte128`

```c
__uint128_t fio_has_byte128(__uint128_t row, uint8_t byte)
```

    Detects if `byte` exists within an 8 byte vector.

-------------------------------------------------------------------------------

## Multi-Precision Math Building Blocks

The following simple operations can be used to build your own multi-precision implementation.

#### `fio_math_addc64`

```c
uint64_t fio_math_addc64(uint64_t a,
                         uint64_t b,
                         uint64_t carry_in,
                         uint64_t *carry_out);
```

Add with carry.

#### `fio_math_subc64`

```c
uint64_t fio_math_subc64(uint64_t a,
                         uint64_t b,
                         uint64_t carry_in,
                         uint64_t *carry_out);
```

Subtract with carry.

#### `fio_math_mulc64`
```c
uint64_t fio_math_mulc64(uint64_t a, uint64_t b, uint64_t *carry_out);
```

Multiply with carry out.

-------------------------------------------------------------------------------
