# facil.io - C Real-Time Web Application Framework

[facil.io](http://facil.io) focuses on providing solutions for real-time web applications. facil.io includes:

* A fast HTTP/1.1, Websocket and SSE application server.
* A zero-dependency Pub/Sub cluster-enabled message bus API.
* Performant JSON parsing and formatting for easy network communication.
* Dynamic types designed with web applications in mind (Strings, Hashes, Arrays etc').
* Support for custom network protocols for both server and client connections.
* An easy solution to [the C10K problem](http://www.kegel.com/c10k.html).
* Optional connectivity with Redis.

This header library is the **C Server Toolbox Library** (C STL) that makes it all happen.

## The C STL Design Goal

> The [facil.io](http://facil.io) C STL aims to provide C developers with easy-to-use tools to write memory safe and performant programs.

## OS Support

The library in written and tested on POSIX systems. Windows support was added afterwards, leaving the library with a POSIX oriented design.

Please note I cannot continually test the windows support as I avoid the OS... hence, Windows OS support should be considered unstable.

## Server Toolbox Library (STL) Overview

At the core of the [facil.io library](https://facil.io) is its powerful Server Toolbox Library for C (and C++).

The Server Toolbox Library is a "swiss-army-knife" library, that uses MACROS to generate code for different common types, such as Hash Maps, Arrays, Linked Lists, Binary-Safe Strings, etc'.

The [testable](#testing-the-library-fio_test_cstl) header library includes a Server Toolbox Library for the following common types:

* [Binary Safe Dynamic Strings](#dynamic-strings) - defined by `FIO_STR` / `FIO_STR_NAME` / `FIO_STR_SMALL`

* [Dynamic Arrays](#dynamic-arrays) - defined by `FIO_ARRAY_NAME`

* [Hash Maps / Sets](#hash-tables-and-maps) - defined by `FIO_MAP_NAME`

* [Reference counting / Type wrapper](#reference-counting-and-type-wrapping) - defined by `FIO_REF_NAME`

* [Soft / Dynamic Types (FIOBJ)](#fiobj-soft-dynamic-types) - defined by `FIO_FIOBJ`


In addition, the core Server Toolbox Library (STL) includes helpers for common tasks, such as:

* [Atomic operations](#atomic-operations)

* [Pointer Arithmetics](#pointer-arithmetics)

* [Fast String / Number conversion](#string-number-conversion) - defined by `FIO_ATOL`

* [Logging and Assertion (without heap allocation)](#logging-and-assertions) - defined by `FIO_LOG`

* [Binary Safe String Helpers](#binary-safe-core-string-helpers) - defined by `FIO_STR`

* [Time Helpers](#time-helpers) - defined by `FIO_TIME`

* [File Utility Helpers](#file-utility-helpers) - defined by `FIO_FILES`

* [Command Line Interface helpers](#cli-command-line-interface) - defined by `FIO_CLI`

* [Task Queues and Timers](#task-queue) - defined by `FIO_QUEUE`

* [Local Memory Allocation](#local-memory-allocation) - defined by `FIO_MEMORY` / `FIO_MALLOC`

* [An Evented IO Reactor](#io-reactor---an-evented-single-threaded-io-reactor) - defined by `FIO_IO`

* And much more...

-------------------------------------------------------------------------------

## How to Use

Simply copy the `fio-stl` folder to your project (or the combined `fio-stl.h` file).

Then include the file in your project to use the facil.io CSTL library.

The library can be included more than once and produce different results depending on the MACROS predefined before each inclusion.

#### `FIO_INCLUDE_FILE`

The facil.io C STL can be used as either a single header library (`fio-stl.h`) or a multi-header library (`fio-stl/include.h`).

The `FIO_INCLUDE_FILE` macro will remember which approach was first used and will use the same approach for subsequent inclusions.

-------------------------------------------------------------------------------

## Compilation Modes

The Server Toolbox Library types and functions could be compiled as either static or extern ("global"), either limiting their scope to a single C file (compilation unit) or exposing them throughout the program.

### Static Functions by Default

By default, facil.io will generate static functions where possible.

To change this behavior, `FIO_EXTERN` and `FIO_EXTERN_COMPLETE` could be used to generate externally visible code.

#### `FIO_EXTERN`

If defined, the the Server Toolbox Library will generate non-static code.

If `FIO_EXTERN` is defined alone, only function declarations and inline functions will be generated.

If `FIO_EXTERN_COMPLETE` is defined, the function definition (the implementation code) will also be generated.

**Note**: the `FIO_EXTERN` will be **automatically undefined** each time the Server Toolbox Library header is included, **unless** the `FIO_EXTERN` is defined with a **numerical** value other than `1` (a compiler default value in some cases), in which case the `FIO_EXTERN` definition will remain in force until manually removed.

For example, in the header (i.e., `mymem.h`), use:

```c
#define FIO_EXTERN
#define FIO_MALLOC
#include FIO_INCLUDE_FILE
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

**Note**: the `FIO_EXTERN_COMPLETE` will be **automatically undefined** each time the Server Toolbox Library header is included, **unless** the `FIO_EXTERN_COMPLETE` is defined with a **numerical** value other than `1` (a compiler default value in some cases), in which case the `FIO_EXTERN_COMPLETE` definition will remain in force until manually removed.

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

#### `FIO_CRYPTO`

When `FIO_CRYPTO` is defined, all hash and cryptographic modules are included.

**Note**: do NOT use these cryptographic unless you have no other choice. Always prefer tested cryptographic libraries such as OpenSSL.

#### `FIO_EVERYTHING`

Adds all the code facil.io C STL has to offer.

Custom types (templates) can't be created without specific instruction, but all functionality that can be included is included.

Note, `FIO_EVERYTHING` functions will always be `static` unless `FIO_EXTERN` was defined for specific functionality or `FIO_EXTERN` was defined in a persistent way (with a numerical value of `2` or greater).


-------------------------------------------------------------------------------

## Testing the Library (`FIO_TEST_ALL`)

To test the library, define the `FIO_TEST_ALL` macro and include the header. A testing function called `fio_test_dynamic_types` will be defined. Call that function in your code to test the library.

#### `FIO_TEST_ALL`

Defined the `fio_test_dynamic_types` and enables as many testing features as possible, such as the `FIO_LEAK_COUNTER`.

-------------------------------------------------------------------------------

## Version and Common Helper Macros

The facil.io C STL (Server Toolbox Library) offers a number of common helper macros that are also used internally. These are automatically included once the `fio-stl.h` is included.

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

-------------------------------------------------------------------------------

## OS Detection Macros

#### `FIO_OS_POSIX`

Set to `1` on POSIX systems (Linux, macOS, BSD, etc.), `0` otherwise.

#### `FIO_OS_WIN`

Set to `1` on Windows systems, `0` otherwise.

#### `FIO_HAVE_UNIX_TOOLS`

Set to a non-zero value when Unix-like tools are available. Values:
- `0` - No Unix tools
- `1` - Full POSIX
- `2` - MinGW
- `3` - Cygwin

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

## Memory Leak Detection

#### `FIO_LEAK_COUNTER`

Unless defined as zero (`0`), facil.io will count allocations and deallocations for custom memory allocators and reference counted types - allowing memory leaks to be detected with a high degree of certainty.

This may also print some minimal usage information about each allocator when exiting the program (when logging using `FIO_LOG_LEVEL_DEBUG`. 

**Note**: enabling leak detection automatically adds the `FIO_LOG` module (to print errors), the `FIO_ATOMIC` module (for atomic counters) and the `FIO_STATE` module (for more predictable `at_exit` callbacks).

#### `FIO_LEAK_COUNTER_DEF`, `FIO_LEAK_COUNTER_ON_ALLOC` and `FIO_LEAK_COUNTER_ON_FREE`

These macros require `FIO_LEAK_COUNTER` to be true, otherwise they do nothing.

- `FIO_LEAK_COUNTER_DEF(name)` - defines the named memory leak counter / detection functions.

- `FIO_LEAK_COUNTER_ON_ALLOC(name)` - adds an allocation to the named memory leak counter.

- `FIO_LEAK_COUNTER_ON_FREE(name)` - subtracts an allocation from the named memory leak counter and tests if `free` was called more than `malloc` for this named allocation counter.

For example:

```c
typedef struct { int i; } my_type_s;
/* define the allocation counter */
FIO_LEAK_COUNTER_DEF(my_type_s)
/* allocation function */
my_type_s * my_type_new() {
  my_type_s *t = malloc(sizeof(*t));
  if(!t)
    return t;
  /* count allocation */
  FIO_LEAK_COUNTER_ON_ALLOC(my_type_s);
  *t = (my_type_s){0};
  return t;
}
/* deallocation function */
void my_type_free(my_type_s * t) {
  if(!t)
    return;
  /* count deallocation before freeing object - tests excessive calls to free) */
  FIO_LEAK_COUNTER_ON_FREE(my_type_s);
  free(t);
  FIO_LOG_DEBUG("We now have only %zu my_type_s objects left.",
                FIO_LEAK_COUNTER_COUNT(my_type_s));
}
```

**Note**: the `FIO_REF` reference counting module does this automatically when `FIO_LEAK_COUNTER` is defined as true.

#### `FIO_LEAK_COUNTER_COUNT`

```c
#define FIO_LEAK_COUNTER_COUNT(name)
```

Returns the number of unfreed allocations according to the named memory leak detector.

Returned type is `size_t`

-------------------------------------------------------------------------------

## Dedicated Static Memory Allocations

The core module provides a simple way to manage a "good enough" thread safe memory allocations for short term use that never need to be freed (e.g., when the caller cannot be trusted to free the memory).

#### `FIO_STATIC_ALLOC_DEF`

```c
#define FIO_STATIC_ALLOC_DEF(function_name, type_T, size_per_allocation, allocations_per_thread)
```

Defines a simple (almost naive) static memory allocator named `function_name` which accepts a single `size_t` argument `count`.

The defined function returns a `type_T` pointer (`type_T *`) containing `sizeof(type_T) * count * size_per_allocation` in correct memory alignment for the requested type.

```c
static type_T *function_name(size_t count);
```

That memory is statically allocated, allowing it to be returned without ever needing to be freed.

The functions can safely allocate the following number of bytes before the function returns the same memory block to another caller:

    FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX * allocations_per_thread *
        sizeof(type_T) * size_per_allocation

Example:

```c
// Step 1: define the allocator:
FIO_STATIC_ALLOC_DEF(numer2hex_allocator, char, 19, 1);

// Step 2: use the allocator
char * ntos16(uint16_t n) {
  char * buf = numer2hex_allocator(1);
  buf[0] = '0'; buf[1] = 'x';
  fio_ltoa16u(buf+2, n, 16);
  buf[18] = 0;
  return buf;
}
```

-------------------------------------------------------------------------------

## Pointer Arithmetics

#### `FIO_PTR_MATH_LMASK`

```c
#define FIO_PTR_MATH_LMASK(T_type, ptr, bits)                                  \
  ((T_type *)(((uintptr_t)(ptr)) & (((uintptr_t)1ULL << (bits)) - 1)))
```

Masks a pointer's left-most bits, returning the right bits (i.e., `0x000000FF`).

#### `FIO_PTR_MATH_RMASK`

```c
#define FIO_PTR_MATH_RMASK(T_type, ptr, bits)                                  \
  ((T_type *)(((uintptr_t)(ptr)) & ((~(uintptr_t)0ULL) << (bits))))
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

#### `FIO_PTR_FIELD_OFFSET`

```c
#define FIO_PTR_FIELD_OFFSET(T_type, field)                                    \
  ((uintptr_t)((&((T_type *)0xFF00)->field)) - 0xFF00)
```

Calculates the byte offset of a field within a struct type.

**Note**: Uses `0xFF00` as a base address instead of `0` to avoid undefined behavior with address sanitizers and strict aliasing rules.

Example:

```c
typedef struct {
  int a;
  char b;
  double c;
} my_struct_s;

size_t offset = FIO_PTR_FIELD_OFFSET(my_struct_s, c);
// offset now contains the byte offset of field 'c' within my_struct_s
```

#### `FIO_PTR_FROM_FIELD`

```c
#define FIO_PTR_FROM_FIELD(T_type, field, ptr)                                 \
  FIO_PTR_MATH_SUB(T_type, ptr, FIO_PTR_FIELD_OFFSET(T_type, field))
```

Find the root object (of a `struct`) from a pointer to its field's address.

**Note**: Uses `FIO_PTR_FIELD_OFFSET` internally, which uses `0xFF00` as a base address to avoid undefined behavior with address sanitizers.

Example:

```c
typedef struct {
  int id;
  fio_list_node_s node;
  char *name;
} my_item_s;

// Given a pointer to the 'node' field, get the containing struct
fio_list_node_s *node_ptr = /* ... */;
my_item_s *item = FIO_PTR_FROM_FIELD(my_item_s, node, node_ptr);
```

-------------------------------------------------------------------------------

## Pointer Tagging Support

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

If `FIO_PTR_TAG_VALIDATE` is defined, tagging will be verified before executing any code.

`FIO_PTR_TAG_VALIDATE` **must** fail on NULL pointers.

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

#### `FIO_MIFN`

```c
#define FIO_MIFN FIO_IFUNC __attribute__((warn_unused_result))
```

Marks a function as `static`, `inline`, possibly unused, and warns if the return value is unused.

#### `FIO_WEAK`

```c
#define FIO_WEAK __attribute__((weak))
```

Marks a function as weak.

#### `FIO_ALIGN`

```c
#define FIO_ALIGN(bytes) __attribute__((aligned(bytes)))
```

Aligns a type or variable to the specified byte boundary.

#### `DEPRECATED`

```c
#define DEPRECATED(reason) __attribute__((deprecated(reason)))
```

Marks a function or type as deprecated with an optional reason string.

#### `FIO_CONSTRUCTOR(fname)`

```c
#define FIO_CONSTRUCTOR(fname) static __attribute__((constructor)) void fname(void)
```

Marks a function as a _constructor_ - **if supported**.

When supported by the compiler (i.e., `gcc` / `clang`), this function will execute when the library is loaded or, if statically linked, before `main` is called.

#### `FIO_DESTRUCTOR(fname)`

```c
#define FIO_DESTRUCTOR(fname) static __attribute__((destructor)) void fname(void)
```

Marks a function as a _destructor_ - **if supported**.

When supported by the compiler (i.e., `gcc` / `clang`), this function will execute when the library is loaded or, if statically linked, after `main` returns.

#### `FIO_LIKELY` / `FIO_UNLIKELY`

```c
#define FIO_LIKELY(cond)   __builtin_expect((cond), 1)
#define FIO_UNLIKELY(cond) __builtin_expect((cond), 0)
```

Branch prediction hints for the compiler. Use `FIO_LIKELY` when a condition is expected to be true most of the time, and `FIO_UNLIKELY` when it's expected to be false.

#### `FIO_NOOP`

```c
#define FIO_NOOP
```

An empty macro, adding white space.

This is useful when a function name is shadowed by a macro (such as when named arguments are used). The additional `FIO_NOOP` macro and white space that follows prevents the named argument macro from being expanded by the preprocessor.

#### `FIO_NOOP_FN`

```c
#define FIO_NOOP_FN(...)
```

An empty macro that does nothing.

This is useful for creating macros that can have optional callbacks (`FIO_NOOP_FN` can be used instead of a callback in these cases).

#### `FIO_NOOP_FN_NAME`

```c
#define FIO_NOOP_FN_NAME (void)
```

A macro for a No-Op function name that casts to void.

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

#### `FIO_NAME_TEST`

```c
#define FIO_NAME_TEST(prefix, postfix) FIO_NAME(fio___test, FIO_NAME(prefix, postfix))
```

Used internally to name test functions.

#### `FIO_FOR`

```c
#define FIO_FOR(i, count) for (size_t i = 0; i < (count); ++i)
```

Helper for simple `for` loops, where `i` is the variable name to use.

-------------------------------------------------------------------------------

## Compiler Guards and Memory Barriers

#### `FIO_COMPILER_GUARD`

```c
#define FIO_COMPILER_GUARD __asm__ volatile("" ::: "memory")
```

Clobber CPU registers and prevent compiler reordering optimizations.

#### `FIO_COMPILER_GUARD_INSTRUCTION`

```c
#define FIO_COMPILER_GUARD_INSTRUCTION __asm__ volatile("" :::)
```

Prevent compiler instruction reordering without clobbering memory.

-------------------------------------------------------------------------------

## Thread Scheduling Macros

#### `FIO_THREAD_WAIT`

```c
#define FIO_THREAD_WAIT(nano_sec)
```

Calls the system's sleep function with the requested nano-second count.

On POSIX systems, calls `nanosleep`. On Windows, calls `Sleep`.

#### `FIO_THREAD_YIELD`

```c
#define FIO_THREAD_YIELD()
```

Yields the thread, hinting to the processor about a spinlock loop.

Uses CPU-specific instructions (`pause` on x86, `yield` on ARM) when available.

#### `FIO_THREAD_RESCHEDULE`

```c
#define FIO_THREAD_RESCHEDULE() FIO_THREAD_WAIT(4)
```

Reschedules the thread by calling nanosleep for a few nanoseconds.

In practice, the thread will probably sleep for 60ns or more. Seems to be faster than `FIO_THREAD_YIELD`, perhaps it prevents de-prioritization of the thread.

-------------------------------------------------------------------------------

## Assertions

#### `FIO_ASSERT`

```c
#define FIO_ASSERT(cond, ...)
```

Asserts a condition is true, or kills the application using SIGINT.

Prints the error message and errno information before aborting.

#### `FIO_ASSERT_ALLOC`

```c
#define FIO_ASSERT_ALLOC(ptr) FIO_ASSERT((ptr), "memory allocation failed.")
```

Tests for an allocation failure. The behavior can be overridden.

#### `FIO_ASSERT_DEBUG`

```c
#define FIO_ASSERT_DEBUG(cond, ...)
```

If `DEBUG` is defined, raises SIGINT if assertion fails, otherwise NOOP.

#### `FIO_ASSERT_STATIC`

```c
#define FIO_ASSERT_STATIC(cond, msg) _Static_assert((cond), msg)
```

Compile-time assertion. Uses C11 `_Static_assert` when available.

-------------------------------------------------------------------------------

## Memory Copying, Seeking and Setting

The following macros are defined to allow for memory copying primitives of set sizes.

In addition an overridable `FIO_MEMCPY` macro is provided that allows routing any variable sized memory copying to a different routine.

#### `FIO_MEMCPY`

```c
#define FIO_MEMCPY memcpy // or __builtin_memcpy if available
```

This macro makes it easy to override the `memcpy` implementation used by the library.

By default this will be set to either `memcpy` or `__builtin_memcpy` (if available). It can also be set to `fio_memcpy` if need be.

#### `FIO_MEMMOVE`

```c
#define FIO_MEMMOVE memmove // or __builtin_memmove if available
```

This macro makes it easy to override the `memmove` implementation used by the library.

By default this will be set to either `memmove` or `__builtin_memmove` (if available). It can also be set to `fio_memcpy` if need be.

#### `fio_memcpy##`

```c
static void * fio_memcpy0(void *restrict dest, const void *restrict src); /* no-op */
static void * fio_memcpy1(void *restrict dest, const void *restrict src);
static void * fio_memcpy2(void *restrict dest, const void *restrict src);
static void * fio_memcpy3(void *restrict dest, const void *restrict src);
static void * fio_memcpy4(void *restrict dest, const void *restrict src);
static void * fio_memcpy8(void *restrict dest, const void *restrict src);
static void * fio_memcpy16(void *restrict dest, const void *restrict src);
static void * fio_memcpy32(void *restrict dest, const void *restrict src);
static void * fio_memcpy64(void *restrict dest, const void *restrict src);
// ... void * fio_memcpy4096
```

Copies a pre-defined `n` bytes from `src` to `dest` where `n` is a power of 2 between 1 and 4096 (including).

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than libc, depending on the compiler and available instruction sets / optimizations.

#### `fio_memcpy##x`

```c
static void * fio_memcpy7x(void *restrict dest, const void *restrict src, size_t length);
static void * fio_memcpy15x(void *restrict dest, const void *restrict src, size_t length);
static void * fio_memcpy31x(void *restrict dest, const void *restrict src, size_t length);
static void * fio_memcpy63x(void *restrict dest, const void *restrict src, size_t length);
// ... void * fio_memcpy4095x
```

Copies up to `n-1` bytes from `src` to `dest` where `n` is a power of 2 between 1 and 4096 (including).

This is provided to allow for easy "tail" processing.

**Note**: Implementation relies heavily on compiler auto-vectorization. Resulting code may run faster or slower than libc, depending on the compiler and available instruction sets / optimizations.

#### `fio_memcpy`

See [Alternative Memory Functions](001 memalt.md) for full documentation.

#### `FIO_MEMSET`

```c
#define FIO_MEMSET memset // or __builtin_memset if available
```

This macro makes it easy to override the `memset` implementation used by the library.

By default this will be set to either `memset` or `__builtin_memset` (if available). It can also be set to `fio_memset` if need be.

#### `fio_memset`

See [Alternative Memory Functions](001 memalt.md) for full documentation.

#### `FIO_MEMCHR`

```c
#define FIO_MEMCHR memchr // or __builtin_memchr if available
```

This macro makes it easy to override the `memchr` implementation used by the library.

By default this will be set to either `memchr` or `__builtin_memchr` (if available). It can also be set to `fio_memchr` if need be.

#### `fio_memchr`

See [Alternative Memory Functions](001 memalt.md) for full documentation.

#### `FIO_MEMCMP`

```c
#define FIO_MEMCMP memcmp // or __builtin_memcmp if available
```

This macro makes it easy to override the `memcmp` implementation used by the library.

By default this will be set to either `memcmp` or `__builtin_memcmp` (if available). It can also be set to `fio_memcmp` if need be.

#### `fio_memcmp`

See [Alternative Memory Functions](001 memalt.md) for full documentation.

#### `FIO_STRLEN`

```c
#define FIO_STRLEN strlen // or __builtin_strlen if available
```

This macro makes it easy to override the `strlen` implementation used by the library.

By default this will be set to either `strlen` or `__builtin_strlen` (if available). It can also be set to `fio_strlen` if need be.

#### `fio_strlen`

See [Alternative Memory Functions](001 memalt.md) for full documentation.

#### `FIO_MEMALT`

If defined, defines all previously undefined memory macros to use facil.io's fallback options.

Note that this will also cause `__builtin_memcpy` to be bypassed for the fixed `fio_memcpy##` functions.

-------------------------------------------------------------------------------

## Security Related Functions

#### `fio_secure_zero`

```c
void fio_secure_zero(void *a_, size_t bytes);
```

Securely zeros memory, preventing compiler optimizations from removing the operation.

Use this to clear sensitive data (passwords, keys, etc.) from memory.

#### `FIO_MEM_STACK_WIPE`

```c
#define FIO_MEM_STACK_WIPE(pages)
```

Wipes stack memory by allocating and zeroing a volatile array of the specified number of pages (each page is 4096 bytes).

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
- `fio_buf2u24_be(buffer)`
- `fio_buf2u32_be(buffer)`
- `fio_buf2u64_be(buffer)`
- `fio_buf2u128_be(buffer)`

**Little Endian**:

- `fio_buf2u16_le(buffer)`
- `fio_buf2u24_le(buffer)`
- `fio_buf2u32_le(buffer)`
- `fio_buf2u64_le(buffer)`
- `fio_buf2u128_le(buffer)`

**Native (Unspecified) Byte Order**:

- `fio_buf2u16u(buffer)`
- `fio_buf2u24u(buffer)`
- `fio_buf2u32u(buffer)`
- `fio_buf2u64u(buffer)`
- `fio_buf2u128u(buffer)`

#### Numbers to Bytes (native / reversed / network ordered)

Writes a number to an unaligned memory buffer. The number or bits written to the buffer is indicated by the name of the function.

**Big Endian (default)**:

- `fio_u2buf16_be(buffer, i)`
- `fio_u2buf24_be(buffer, i)`
- `fio_u2buf32_be(buffer, i)`
- `fio_u2buf64_be(buffer, i)`
- `fio_u2buf128_be(buffer, i)`

**Little Endian**:

- `fio_u2buf16_le(buffer, i)`
- `fio_u2buf24_le(buffer, i)`
- `fio_u2buf32_le(buffer, i)`
- `fio_u2buf64_le(buffer, i)`
- `fio_u2buf128_le(buffer, i)`

**Native (Unspecified) Byte Order**:

- `fio_u2buf16u(buffer, i)`
- `fio_u2buf24u(buffer, i)`
- `fio_u2buf32u(buffer, i)`
- `fio_u2buf64u(buffer, i)`
- `fio_u2buf128u(buffer, i)`

#### `fio_xmask`

```c
void fio_xmask(char *buf, size_t len, uint64_t mask);
```

Masks data using a 64 bit mask.

The function may perform significantly better when the buffer's memory is aligned.

#### `fio_xmask_cpy`

```c
void fio_xmask_cpy(char *restrict dest, const char *src, size_t len, uint64_t mask);
```

Masks data using a 64 bit mask while copying from `src` to `dest`.

If `dest == src`, behaves like `fio_xmask`.

-------------------------------------------------------------------------------

## Constant Time Helpers

Performs the operation indicated in constant time.

#### `fio_ct_true`

```c
uintmax_t fio_ct_true(uintmax_t cond);
```

Returns 1 if `cond` is non-zero, 0 otherwise.

#### `fio_ct_false`

```c
uintmax_t fio_ct_false(uintmax_t cond);
```

Returns 1 if `cond` is zero, 0 otherwise.

#### `fio_ct_if_bool`

```c
uintmax_t fio_ct_if_bool(uintmax_t cond, uintmax_t a, uintmax_t b);
```

Returns `a` if `cond == 1`, returns `b` otherwise.

#### `fio_ct_if`

```c
uintmax_t fio_ct_if(uintmax_t cond, uintmax_t a, uintmax_t b);
```

Returns `a` if `cond` is non-zero, returns `b` otherwise.

#### `fio_ct_max`

```c
intmax_t fio_ct_max(intmax_t a, intmax_t b);
```

Returns `a` if `a >= b` (performs a **signed** comparison).

#### `fio_ct_min`

```c
intmax_t fio_ct_min(intmax_t a, intmax_t b);
```

Returns `a` if `a <= b` (performs a **signed** comparison).

#### `fio_ct_abs`

```c
uintmax_t fio_ct_abs(intmax_t i);
```

Returns the absolute value of `i`.

#### `fio_ct_is_eq`

```c
_Bool fio_ct_is_eq(const void *a, const void *b, size_t bytes);
```

Returns 1 if memory regions are equal. Should be resistant to timing attacks.

-------------------------------------------------------------------------------

## Constant-Time Bitwise Selection Functions

These functions perform bitwise selection operations in constant time. They are commonly used in cryptographic algorithms (SHA, AES, etc.) and can also be used for branchless programming.

#### `fio_ct_mux32` / `fio_ct_mux64`

```c
uint32_t fio_ct_mux32(uint32_t x, uint32_t y, uint32_t z);
uint64_t fio_ct_mux64(uint64_t x, uint64_t y, uint64_t z);
```

Bitwise "choose": for each bit, if x is set, return y's bit, else z's bit.

Formula: `z ^ (x & (y ^ z))`

Used in: SHA-1 (Ch), SHA-256 (Ch), SHA-512 (Ch), AES, etc.

#### `fio_ct_maj32` / `fio_ct_maj64`

```c
uint32_t fio_ct_maj32(uint32_t x, uint32_t y, uint32_t z);
uint64_t fio_ct_maj64(uint64_t x, uint64_t y, uint64_t z);
```

Bitwise "majority": for each bit position, return 1 if 2+ inputs have 1.

Formula: `(x & y) | (z & (x | y))`

Used in: SHA-1 (Maj), SHA-256 (Maj), SHA-512 (Maj), etc.

#### `fio_ct_xor3_32` / `fio_ct_xor3_64`

```c
uint32_t fio_ct_xor3_32(uint32_t x, uint32_t y, uint32_t z);
uint64_t fio_ct_xor3_64(uint64_t x, uint64_t y, uint64_t z);
```

Bitwise "parity": XOR of all three inputs (1 if odd number of 1s).

Formula: `x ^ y ^ z`

Used in: SHA-1 (Parity function for rounds 20-39 and 60-79)

-------------------------------------------------------------------------------

## Combined Rotation-XOR Operations

These combine multiple rotations with XOR, which is a common pattern in SHA-2, BLAKE2, and other hash functions.

#### `fio_xor_rrot3_32` / `fio_xor_rrot3_64`

```c
uint32_t fio_xor_rrot3_32(uint32_t x, uint8_t a, uint8_t b, uint8_t c);
uint64_t fio_xor_rrot3_64(uint64_t x, uint8_t a, uint8_t b, uint8_t c);
```

XOR of three right rotations: `ROTR(x,a) ^ ROTR(x,b) ^ ROTR(x,c)`

Common in SHA-256/SHA-512 Sigma functions.

#### `fio_xor_rrot2_shr_32` / `fio_xor_rrot2_shr_64`

```c
uint32_t fio_xor_rrot2_shr_32(uint32_t x, uint8_t a, uint8_t b, uint8_t c);
uint64_t fio_xor_rrot2_shr_64(uint64_t x, uint8_t a, uint8_t b, uint8_t c);
```

XOR of two right rotations and a right shift: `ROTR(x,a) ^ ROTR(x,b) ^ SHR(x,c)`

Common in SHA-256/SHA-512 sigma (lowercase) functions for message schedule.

-------------------------------------------------------------------------------

## Inspecting Byte / Bit Data

**Note**: the 128 bit helpers are only available with systems / compilers that support 128 bit types.

**Note**: for mutable shared data, please consider using the atomic operations.

### Bitmap helpers

#### `fio_bit_get`

```c
uint8_t fio_bit_get(void *map, size_t bit);
```

Gets the state of a bit in a bitmap.

#### `fio_bit_set`

```c
void fio_bit_set(void *map, size_t bit);
```

Sets a bit in a bitmap (sets to 1). **Non-atomic**, not thread-safe.

#### `fio_bit_unset`

```c
void fio_bit_unset(void *map, size_t bit);
```

Unsets a bit in a bitmap (sets to 0). **Non-atomic**, not thread-safe.

#### `fio_bit_flip`

```c
void fio_bit_flip(void *map, size_t bit);
```

Flips a bit in a bitmap (sets to 0 if 1, sets to 1 if 0). **Non-atomic**, not thread-safe.

### Bit Isolation and Indexing

#### `fio_bits_lsb`

```c
uint64_t fio_bits_lsb(uint64_t i);
```

Isolates the least significant (lowest) bit.

#### `fio_bits_msb`

```c
uint64_t fio_bits_msb(uint64_t i);
```

Isolates the most significant (highest) bit.

#### `fio_bits_lsb_index`

```c
size_t fio_bits_lsb_index(uint64_t i);
```

Returns the index of the least significant (lowest) bit. Returns `(size_t)-1` if `i` is 0.

#### `fio_bits_msb_index`

```c
size_t fio_bits_msb_index(uint64_t i);
```

Returns the index of the most significant (highest) bit. Returns `(size_t)-1` if `i` is 0.

#### `fio_lsb_index_unsafe`

```c
size_t fio_lsb_index_unsafe(uint64_t i);
```

Returns the index of the least significant (lowest) bit. Undefined behavior if `i` is 0.

#### `fio_msb_index_unsafe`

```c
size_t fio_msb_index_unsafe(uint64_t i);
```

Returns the index of the most significant (highest) bit. Undefined behavior if `i` is 0.

### Popcount and Hamming Distance

#### `fio_popcount`

```c
int fio_popcount(uint64_t n);
```

Returns the number of set bits in the number `n`.

#### `fio_hemming_dist`

```c
#define fio_hemming_dist(n1, n2) fio_popcount(((uint64_t)(n1) ^ (uint64_t)(n2)))
```

Returns the Hamming Distance between the number `n1` and the number `n2`.

Hamming Distance is the number of bits that need to be "flipped" in order for both numbers to be equal.

### Byte Detection in Vectors

#### `fio_has_full_byte32`

```c
uint32_t fio_has_full_byte32(uint32_t row);
```

Detects a byte where all the bits are set (`255`) within a 4 byte vector.

#### `fio_has_zero_byte32`

```c
uint32_t fio_has_zero_byte32(uint32_t row);
```

Detects a byte where no bits are set (0) within a 4 byte vector.

#### `fio_has_byte32`

```c
uint32_t fio_has_byte32(uint32_t row, uint8_t byte);
```

Detects if `byte` exists within a 4 byte vector.

#### `fio_has_full_byte64`

```c
uint64_t fio_has_full_byte64(uint64_t row);
```

Detects a byte where all the bits are set (`255`) within an 8 byte vector.

#### `fio_has_zero_byte64`

```c
uint64_t fio_has_zero_byte64(uint64_t row);
```

Detects a byte where no bits are set (byte == 0) within an 8 byte vector.

#### `fio_has_zero_byte_alt64`

```c
uint64_t fio_has_zero_byte_alt64(uint64_t row);
```

Alternative version of `fio_has_zero_byte64`. Should NOT be used to build a bitmap, but may be used to detect the first occurrence.

#### `fio_has_byte64`

```c
uint64_t fio_has_byte64(uint64_t row, uint8_t byte);
```

Detects if `byte` exists within an 8 byte vector.

#### `fio_has_byte2bitmap`

```c
uint64_t fio_has_byte2bitmap(uint64_t result);
```

Converts a `fio_has_byteX` result to a bitmap.

#### `fio_has_full_byte128`

```c
__uint128_t fio_has_full_byte128(__uint128_t row);
```

Detects a byte where all the bits are set (`255`) within a 16 byte vector.

#### `fio_has_zero_byte128`

```c
__uint128_t fio_has_zero_byte128(__uint128_t row);
```

Detects a byte where no bits are set (0) within a 16 byte vector.

#### `fio_has_byte128`

```c
__uint128_t fio_has_byte128(__uint128_t row, uint8_t byte);
```

Detects if `byte` exists within a 16 byte vector.

-------------------------------------------------------------------------------

## Multi-Precision Math Building Blocks

The following simple operations can be used to build your own multi-precision implementation.

The following, somewhat naive, multi-precision math implementation focuses on constant time. It assumes an array of local endian 64bit numbers ordered within the array in little endian (word `0` contains the least significant bits and word `n-1` contains the most significant bits).


#### `fio_math_addc64`

```c
uint64_t fio_math_addc64(uint64_t a,
                         uint64_t b,
                         uint64_t carry_in,
                         uint64_t *carry_out);
```

Add with carry.

#### `fio_math_add`

```c
bool fio_math_add(uint64_t *dest,
                  const uint64_t *a,
                  const uint64_t *b,
                  const size_t len);
```

Multi-precision ADD for `len` 64 bit words a + b. Returns the carry.

This assumes all the pointers point to memory blocks that are the same-length `len` (use zero padding for uneven word lengths).

This assume LSW (Least Significant Word) ordering.

#### `fio_math_subc64`

```c
uint64_t fio_math_subc64(uint64_t a,
                         uint64_t b,
                         uint64_t carry_in,
                         uint64_t *carry_out);
```

Subtract with carry.

#### `fio_math_sub`

```c
uint64_t fio_math_sub(uint64_t *dest,
                      const uint64_t *a,
                      const uint64_t *b,
                      const size_t len);
```

Multi-precision SUB for `len` 64 bit words a + b. Returns the borrow.

This assumes all the pointers point to memory blocks that are the same-length `len` (use zero padding for uneven word lengths).

This assume LSW (Least Significant Word) ordering.

#### `fio_math_mulc64`
```c
uint64_t fio_math_mulc64(uint64_t a, uint64_t b, uint64_t *carry_out);
```

Multiply with carry out.


#### `fio_math_mul`

```c
void fio_math_mul(uint64_t *restrict dest,
                  const uint64_t *a,
                  const uint64_t *b,
                  const size_t len);
```

Multi-precision MUL for `len` 64 bit words.

`dest` must be `len * 2` long to hold the result.

`a` and `b` must be 64 bit word arrays of equal `len`.

This assume LSW (Least Significant Word) ordering.

#### `fio_math_mod_mul64`

```c
uint64_t fio_math_mod_mul64(uint64_t a, uint64_t b, uint64_t mod);
```

Perform modular multiplication for numbers with up to 64 bits.

#### `fio_math_mod_expo64`

```c
uint64_t fio_math_mod_expo64(uint64_t base, uint64_t exp, uint64_t mod);
```

Perform modular exponentiation for numbers with up to 64 bits.

-------------------------------------------------------------------------------

## 64 bit Prime Helpers

### Detecting primes

Sometimes it is important to know if a number is a prime. the facil.io CSTL provides helpers for detecting if a 64 bit number is likely to be a prime.

#### `fio_math_is_uprime`

```c
bool fio_math_is_uprime(uint64_t n);
```

Tests if an unsigned 64 bit number is (probably) a prime. For numbers up to 1023 this is deterministic.

#### `fio_math_is_iprime`

```c
bool fio_math_is_iprime(int64_t n);
```

Tests if the absolute value of a signed 64 bit number is (probably) a prime. For numbers up to 1023 this is deterministic.

### Constants

The importance of primes for some aspects of programming is difficult to exaggerate. For this reason, facil.io provides the following helper MACROS that represent somewhat randomly pre-selected primes out of a prime pool that fit the requirements (about half of the bits set).

Each group of macros contains 32 primes (indexed from `0` to `31`), where the post-fix of the macro correlates to its index.

#### `FIO_U8_HASH_PRIME0` ... `FIO_U8_HASH_PRIME31`

These set of macros, with post-fixes from `0` to `31`, represent a prime number that would fit inside a `uint8_t` type and has between 3 and 5 bits set (out of 8).

#### `FIO_U16_HASH_PRIME0` ... `FIO_U16_HASH_PRIME31`

These set of macros, with post-fixes from `0` to `31`, represent a prime number that would fit inside a `uint16_t` type and has only half of its bits set.

#### `FIO_U32_HASH_PRIME0` ... `FIO_U32_HASH_PRIME31`

These set of macros, with post-fixes from `0` to `31`, represent a prime number that would fit inside a `uint32_t` type and has only half of its bits set.

#### `FIO_U64_HASH_PRIME0` ... `FIO_U64_HASH_PRIME31`

These set of macros, with post-fixes from `0` to `31`, represent a prime number that would fit inside a `uint64_t` type and has only half of its bits set.

-------------------------------------------------------------------------------

## Native Numeral Vector Operation

These are operations defined on native C types, written with the hope that the compiler will replace these somewhat naive implementations with SIMD instructions where possible.

These operate on common, power of 2, collections of numbers

#### Vectorized Mathematical Operations (`fio_u##x#_OP`)

Add, Multiply, Subtract and more using any of the following (or similarly named):

```c
void fio_u8x4_add(uint8_t * dest, uint8_t * a, uint8_t *b);
void fio_u8x8_add(uint8_t * dest, uint8_t * a, uint8_t *b);
void fio_u8x16_add(uint8_t * dest, uint8_t * a, uint8_t *b);
void fio_u8x32_add(uint8_t * dest, uint8_t * a, uint8_t *b);
void fio_u8x64_add(uint8_t * dest, uint8_t * a, uint8_t *b);
void fio_u8x128_add(uint8_t * dest, uint8_t * a, uint8_t *b);
void fio_u8x256_add(uint8_t * dest, uint8_t * a, uint8_t *b);

void fio_u16x2_add(uint16_t * dest, uint16_t * a, uint16_t *b);
void fio_u16x4_add(uint16_t * dest, uint16_t * a, uint16_t *b);
// ... etc.

void fio_u32x2_add(uint32_t * dest, uint32_t * a, uint32_t *b);
// ... etc.

void fio_u64x2_add(uint64_t * dest, uint64_t * a, uint64_t *b);
// ... etc.
```

The following operations are supported: `add`, `sub`, `mul`, `and`, `or`, `xor`.

#### Vectorized Mathematical Summing (`fio_u##x#_reduce_OP`)

Sum vectors up using Add, Or, XOR, and more using any of the following (or similarly named):

```c
uint8_t fio_u8x4_reduce_add(uint8_t * v);
uint8_t fio_u8x8_reduce_add(uint8_t * v);
// ... etc.

uint16_t fio_u16x2_reduce_add(uint16_t * v);
// ... etc.

uint32_t fio_u32x2_reduce_add(uint32_t * v);
// ... etc.

uint64_t fio_u64x2_reduce_add(uint64_t * v);
// ... etc.
```

The following summation operations are supported: `max`, `min`, `add`, `mul`, `and`, `or`, `xor`.

#### Reshuffling (`fio_u##x#_reshuffle`)

Reorders the words inside the vector.

```c
#define fio_u8x4_reshuffle(v, ...)     fio_u8x4_reshuffle(v,     (uint8_t[4]){__VA_ARGS__})
#define fio_u8x8_reshuffle(v, ...)     fio_u8x8_reshuffle(v,     (uint8_t[8]){__VA_ARGS__})
// ... etc.
```

-------------------------------------------------------------------------------

## Atomic Operations (Core)

If the `FIO_ATOMIC` macro is defined than the following macros will be defined.

In general, when a function returns a value, it is always the previous value - unless the function name ends with `fetch` or `load`.

#### `fio_atomic_load(dest, p_obj)`

Atomically loads and returns the value stored in the object pointed to by `p_obj`.

#### `fio_atomic_exchange(p_obj, value)`

Atomically sets the object pointer to by `p_obj` to `value`, returning the
previous value.

#### `fio_atomic_add(p_obj, value)`

A MACRO / function that performs `add` atomically.

Returns the previous value.

#### `fio_atomic_sub(p_obj, value)`

A MACRO / function that performs `sub` atomically.

Returns the previous value.

#### `fio_atomic_and(p_obj, value)`

A MACRO / function that performs `and` atomically.

Returns the previous value.

#### `fio_atomic_xor(p_obj, value)`

A MACRO / function that performs `xor` atomically.

Returns the previous value.

#### `fio_atomic_or(p_obj, value)`

A MACRO / function that performs `or` atomically.

Returns the previous value.

#### `fio_atomic_nand(p_obj, value)`

A MACRO / function that performs `nand` atomically.

Returns the previous value.

#### `fio_atomic_add_fetch(p_obj, value)`

A MACRO / function that performs `add` atomically.

Returns the new value.

#### `fio_atomic_sub_fetch(p_obj, value)`

A MACRO / function that performs `sub` atomically.

Returns the new value.

#### `fio_atomic_and_fetch(p_obj, value)`

A MACRO / function that performs `and` atomically.

Returns the new value.

#### `fio_atomic_xor_fetch(p_obj, value)`

A MACRO / function that performs `xor` atomically.

Returns the new value.

#### `fio_atomic_or_fetch(p_obj, value)`

A MACRO / function that performs `or` atomically.

Returns the new value.

#### `fio_atomic_nand_fetch(p_obj, value)`

A MACRO / function that performs `nand` atomically.

Returns the new value.

#### `fio_atomic_compare_exchange_p(p_obj, p_expected, p_desired)`

A MACRO / function that performs a system specific `fio_atomic_compare_exchange` using pointers.

The behavior of this instruction is compiler / CPU architecture specific, where `p_expected` **SHOULD** be overwritten with the latest value of `p_obj`, but **MAY NOT**, depending on system and compiler implementations.

Returns 1 for successful exchange or 0 for failure.

#### Atomic Bitmap helpers

- `fio_atomic_bit_get(void *map, size_t bit)` - Gets the state of a bit atomically.

- `fio_atomic_bit_set(void *map, size_t bit)` - Sets a bit atomically (an **atomic** operation, thread-safe).

- `fio_atomic_bit_unset(void *map, size_t bit)` - Unsets a bit atomically (an **atomic** operation, thread-safe).

- `fio_atomic_bit_flip(void *map, size_t bit)` - Flips a bit atomically (an **atomic** operation, thread-safe).

-------------------------------------------------------------------------------

## a SpinLock style MultiLock

Atomic operations lend themselves easily to implementing spinlocks, so the facil.io STL includes one.

Spinlocks are effective for very short critical sections or when a a failure to acquire a lock allows the program to redirect itself to other pending tasks. 

However, in general, spinlocks should be avoided when a task might take a longer time to complete or when the program might need to wait for a high contention lock to become available.

#### `fio_lock_i`

A spinlock type based on a volatile unsigned char.

**Note**: the spinlock contains one main / default lock (`sub == 0`) and 7 sub-locks (`sub >= 1 && sub <= 7`), which could be managed:

- Separately / Jointly: using the `fio_trylock_group`, `fio_lock_group` and `fio_unlock_group` functions.
- Collectively: using the `fio_trylock_full`, `fio_lock_full` and `fio_unlock_full` functions.

#### `fio_lock(fio_lock_i *)`

Busy waits for the default lock (sub-lock `0`) to become available.

#### `fio_trylock(fio_lock_i *)`

Attempts to acquire the default lock (sub-lock `0`). Returns 0 on success and 1 on failure.

#### `fio_unlock(fio_lock_i *)`

Unlocks the default lock (sub-lock `0`), no matter which thread owns the lock.

#### `fio_is_locked(fio_lock_i *)`

Returns 1 if the (main) lock is engaged. Otherwise returns 0.

#### `uint8_t fio_trylock_group(fio_lock_i *lock, const uint8_t group)`

Tries to lock a group of sub-locks.

Combine a number of sub-locks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
macro. i.e.:

```c
if(fio_trylock_group(&lock,
                     FIO_LOCK_SUBLOCK(1) |
                     FIO_LOCK_SUBLOCK(2)) == 0) {
  // act in lock and then release the SAME lock with:
  fio_unlock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2));
}
```

Returns 0 on success and 1 on failure.

#### `void fio_lock_group(fio_lock_i *lock, uint8_t group)`

Busy waits for a group lock to become available - not recommended.

See `fio_trylock_group` for details.

#### `void fio_unlock_group(fio_lock_i *lock, uint8_t group)`

Unlocks a sub-lock group, no matter which thread owns which sub-lock.

#### `fio_is_group_locked(fio_lock_i *, uint8_t group)`

Returns 1 if the specified group-lock is engaged. Otherwise returns 0.

#### `fio_trylock_full(fio_lock_i *lock)`

Tries to lock all sub-locks. Returns 0 on success and 1 on failure.

#### `fio_lock_full(fio_lock_i *lock)`

Busy waits for all sub lock to become available - not recommended.

#### `fio_unlock_full(fio_lock_i *lock)`

Unlocks all sub locks, no matter which thread owns which lock.

-------------------------------------------------------------------------------

## Linked Lists

The facil.io C STL provides a doubly-linked list implementation using macros. These are intrusive linked lists, meaning the list node is embedded within your data structure.

#### `fio_list_node_s`

```c
typedef struct fio_list_node_s {
  struct fio_list_node_s *next;
  struct fio_list_node_s *prev;
} fio_list_node_s;
```

A linked list arch-type. This struct should be embedded in your data structures to enable linked list functionality.

#### `FIO_LIST_NODE` / `FIO_LIST_HEAD`

```c
#define FIO_LIST_NODE fio_list_node_s
#define FIO_LIST_HEAD fio_list_node_s
```

Type aliases for linked list nodes and heads. Both are the same type - the distinction is semantic (a "head" is a sentinel node that marks the beginning/end of the list).

#### `FIO_LIST_INIT`

```c
#define FIO_LIST_INIT(obj) (fio_list_node_s) { .next = &(obj), .prev = &(obj) }
```

Initializes a `FIO_LIST_HEAD` object to an empty list state (pointing to itself).

Example:

```c
FIO_LIST_HEAD my_list = FIO_LIST_INIT(my_list);
```

#### `FIO_LIST_PUSH`

```c
#define FIO_LIST_PUSH(head, n)
```

UNSAFE macro for pushing a node to the end of a list (before the head).

**Parameters:**
- `head` - pointer to the list head (`fio_list_node_s *`)
- `n` - pointer to the node to push (`fio_list_node_s *`)

Example:

```c
typedef struct {
  int value;
  fio_list_node_s node;
} my_item_s;

FIO_LIST_HEAD list = FIO_LIST_INIT(list);
my_item_s item = {.value = 42};
FIO_LIST_PUSH(&list, &item.node);
```

#### `FIO_LIST_POP`

```c
#define FIO_LIST_POP(type, node_name, dest_ptr, head)
```

UNSAFE macro for popping a node from the front of a list.

**Parameters:**
- `type` - the type of the containing struct
- `node_name` - the name of the `fio_list_node_s` field within the struct
- `dest_ptr` - a pointer variable that will receive the popped item
- `head` - pointer to the list head (`fio_list_node_s *`)

Example:

```c
my_item_s *popped;
FIO_LIST_POP(my_item_s, node, popped, &list);
// popped now points to the first item (or the head if list was empty)
```

#### `FIO_LIST_REMOVE`

```c
#define FIO_LIST_REMOVE(n)
```

UNSAFE macro for removing a node from a list. Does not reset the node's pointers.

**Parameters:**
- `n` - pointer to the node to remove (`fio_list_node_s *`)

#### `FIO_LIST_REMOVE_RESET`

```c
#define FIO_LIST_REMOVE_RESET(n)
```

UNSAFE macro for removing a node from a list and resetting its pointers to point to itself.

**Parameters:**
- `n` - pointer to the node to remove (`fio_list_node_s *`)

#### `FIO_LIST_IS_EMPTY`

```c
#define FIO_LIST_IS_EMPTY(head) \
  ((!(head)) || ((!(head)->next) | ((head)->next == (head))))
```

UNSAFE macro for testing if a list is empty.

**Parameters:**
- `head` - pointer to the list head (`fio_list_node_s *`)

**Returns:** non-zero if the list is empty, zero otherwise.

#### `FIO_LIST_EACH`

```c
#define FIO_LIST_EACH(type, node_name, head, pos)
```

Loops through every node in the linked list except the head, from first to last.

**Parameters:**
- `type` - the type of the containing struct
- `node_name` - the name of the `fio_list_node_s` field within the struct
- `head` - pointer to the list head (`fio_list_node_s *`)
- `pos` - the variable name to use for the current item pointer

**Note**: Safe to use when removing the current node during iteration.

Example:

```c
typedef struct {
  int value;
  fio_list_node_s node;
} my_item_s;

FIO_LIST_HEAD list = FIO_LIST_INIT(list);
// ... add items to list ...

FIO_LIST_EACH(my_item_s, node, &list, item) {
  printf("Value: %d\n", item->value);
  // Safe to remove current item:
  // FIO_LIST_REMOVE(&item->node);
}
```

#### `FIO_LIST_EACH_REVERSED`

```c
#define FIO_LIST_EACH_REVERSED(type, node_name, head, pos)
```

Loops through every node in the linked list in reverse order (from last to first), except the head.

**Parameters:**
- `type` - the type of the containing struct
- `node_name` - the name of the `fio_list_node_s` field within the struct
- `head` - pointer to the list head (`fio_list_node_s *`)
- `pos` - the variable name to use for the current item pointer

**Note**: Safe to use when removing the current node during iteration.

Example:

```c
FIO_LIST_EACH_REVERSED(my_item_s, node, &list, item) {
  printf("Value (reversed): %d\n", item->value);
}
```

-------------------------------------------------------------------------------

## Indexed Linked Lists

Indexed Linked Lists can be used to create a linked list that uses indices relative to some root pointer (usually the root of an array) instead of absolute pointers. This:

1. Allows easy reallocation of the list without requiring pointer updates.
2. Could be used for memory optimization if the array limits are known.

The "head" index is usually validated by reserving the value of `-1` to indicate an empty list.

#### `fio_index32_node_s` / `fio_index16_node_s` / `fio_index8_node_s`

```c
typedef struct fio_index32_node_s { uint32_t next; uint32_t prev; } fio_index32_node_s;
typedef struct fio_index16_node_s { uint16_t next; uint16_t prev; } fio_index16_node_s;
typedef struct fio_index8_node_s { uint8_t next; uint8_t prev; } fio_index8_node_s;
```

Indexed linked list node types for different index sizes (32-bit, 16-bit, and 8-bit indices).

#### `FIO_INDEXED_LIST32_NODE` / `FIO_INDEXED_LIST16_NODE` / `FIO_INDEXED_LIST8_NODE`

```c
#define FIO_INDEXED_LIST32_NODE fio_index32_node_s
#define FIO_INDEXED_LIST32_HEAD uint32_t
#define FIO_INDEXED_LIST16_NODE fio_index16_node_s
#define FIO_INDEXED_LIST16_HEAD uint16_t
#define FIO_INDEXED_LIST8_NODE fio_index8_node_s
#define FIO_INDEXED_LIST8_HEAD uint8_t
```

Type aliases for indexed linked list nodes and heads.

#### `FIO_INDEXED_LIST_PUSH`

```c
#define FIO_INDEXED_LIST_PUSH(root, node_name, head, i)
```

UNSAFE macro for pushing a node to the end of an indexed list (before the head).

**Parameters:**
- `root` - pointer to the array of structs containing the indexed list nodes
- `node_name` - the name of the indexed list node field within the struct
- `head` - the index of the list head
- `i` - the index of the node to push

#### `FIO_INDEXED_LIST_UNSHIFT`

```c
#define FIO_INDEXED_LIST_UNSHIFT(root, node_name, head, i)
```

UNSAFE macro for adding a node to the beginning of an indexed list (updates head).

**Parameters:**
- `root` - pointer to the array of structs containing the indexed list nodes
- `node_name` - the name of the indexed list node field within the struct
- `head` - the index of the list head (will be updated to `i`)
- `i` - the index of the node to add

#### `FIO_INDEXED_LIST_REMOVE`

```c
#define FIO_INDEXED_LIST_REMOVE(root, node_name, i)
```

UNSAFE macro for removing a node from an indexed list.

**Parameters:**
- `root` - pointer to the array of structs containing the indexed list nodes
- `node_name` - the name of the indexed list node field within the struct
- `i` - the index of the node to remove

#### `FIO_INDEXED_LIST_REMOVE_RESET`

```c
#define FIO_INDEXED_LIST_REMOVE_RESET(root, node_name, i)
```

UNSAFE macro for removing a node from an indexed list and resetting its indices to point to itself.

**Parameters:**
- `root` - pointer to the array of structs containing the indexed list nodes
- `node_name` - the name of the indexed list node field within the struct
- `i` - the index of the node to remove

#### `FIO_INDEXED_LIST_EACH`

```c
#define FIO_INDEXED_LIST_EACH(root, node_name, head, pos)
```

Loops through every index in the indexed list, assuming `head` is valid.

**Parameters:**
- `root` - pointer to the array of structs containing the indexed list nodes
- `node_name` - the name of the indexed list node field within the struct
- `head` - the index of the list head
- `pos` - the variable name to use for the current index

#### `FIO_INDEXED_LIST_EACH_REVERSED`

```c
#define FIO_INDEXED_LIST_EACH_REVERSED(root, node_name, head, pos)
```

Loops through every index in the indexed list in reverse order, assuming `head` is valid.

**Parameters:**
- `root` - pointer to the array of structs containing the indexed list nodes
- `node_name` - the name of the indexed list node field within the struct
- `head` - the index of the list head
- `pos` - the variable name to use for the current index

-------------------------------------------------------------------------------

## Numeral / Vector Helper Types

The following union types hold (little endian) arrays of unsigned 64 bit numbers that are accessible also as byte arrays or smaller numeral types:

- `fio_u128`
- `fio_u256`
- `fio_u512`
- `fio_u1024`
- `fio_u2048`
- `fio_u4096`

#### `fio_u128`

```c
typedef union {
  size_t uz[16 / sizeof(size_t)];
  uint64_t u64[2];
  uint32_t u32[4];
  uint16_t u16[8];
  uint8_t u8[16];
  int64_t i64[2];
  int32_t i32[4];
  // ... plus vector types if supported
} fio_u128 FIO_ALIGN(16);
```

An unsigned 128 bit union type.

#### `fio_u256`

An unsigned 256 bit union type. Same structure as `fio_u128` but with larger arrays.

#### `fio_u512`

An unsigned 512 bit union type.

### Numeral / Vector Helper Type Initialization

Fast and easy macros are provided for these numeral helper initialization, initializing any provided X bit words in least-significant-word ordering and initializing any remaining higher words with zero.

#### `fio_u128_init8` / `fio_u128_init16` / `fio_u128_init32` / `fio_u128_init64` ...

```c
#define fio_u128_init8(...)  ((fio_u128){.u8 = {__VA_ARGS__}})
#define fio_u128_init16(...) ((fio_u128){.u16 = {__VA_ARGS__}})
#define fio_u128_init32(...) ((fio_u128){.u32 = {__VA_ARGS__}})
#define fio_u128_init64(...) ((fio_u128){.u64 = {__VA_ARGS__}})
// ... similar for fio_u256, fio_u512, etc.
```

### Numeral / Vector Helper Type Load / Store

These numerals can be stored and loaded from memory using big endian / little endian formatting:

#### `fio_u128_load`, `fio_u256_load` ...

- `fio_u128 fio_u128_load(const void *buf)` - load in native ordering.
- `void fio_u128_store(void *buf, const fio_u128 a)` - store in native ordering.
- `fio_u128 fio_u128_load_le16(const void *buf)` - load in little endian ordering using 16 bit words.
- `fio_u128 fio_u128_load_be16(const void *buf)` - load in big endian ordering using 16 bit words.
- `fio_u128 fio_u128_bswap16(const void *buf)` - load in and byte-swap using 16 bit words.
- ... similar for 32 and 64 bit words, and for larger types.

### Numeral / Vector Helper Type Operation

Common mathematical operations are provided for the Vector Helper Types.

#### `fio_u128_add16`, `fio_u256_sub32`, `fio_u512_mul64` ...

These functions follow the naming scheme of `fio_u##TOTAL_BITS##_##OP##WORD_BITS`, where `TOTAL_BITS` is the total number of bits, `OP` is the name of the operation (`add`, `sub`, `mul`, etc') and `WORD_BITS` is the number of bits in each vector "word".

Supported operations (`OP`) are: `add`, `sub`, `mul`, `and`, `or`, `xor`.

#### Multi-Precision `fio_u128_add`, `fio_u256_sub`, `fio_u512_mul` ...

These functions provide Multi-Precision operations for the Numeral / Vector Helper Types.

- `bool fio_u128_add(fio_u128 *t, fio_u128 *a, fio_u128 *b)` - performs ADD (`t=a+b`), returning the carry bit.
- `bool fio_u256_sub(fio_u256 *t, fio_u256 *a, fio_u256 *b)` - performs SUB (`t=a-b`), returning the borrow bit.
- `void fio_u512_mul(fio_u512 *t, fio_u256 *a, fio_u256 *b)` - performs MUL (`t=a*b`) operation.

-------------------------------------------------------------------------------

## Core Randomness

The core module provides macros for generating semi-deterministic Pseudo-Random Number Generator functions.

#### `fio_cycle_counter`

```c
uint64_t fio_cycle_counter(void);
```

Returns the CPU cycle counter. Uses `rdtsc` on x86, `cntvct_el0` on ARM64, or returns 0 on unsupported platforms.

#### `FIO_DEFINE_RANDOM128_FN`

```c
#define FIO_DEFINE_RANDOM128_FN(extern, name, reseed_log, seed_offset)
```

Defines a semi-deterministic Pseudo-Random 128 bit Number Generator function.

The following functions will be defined:

```c
extern fio_u128 name##128(void); // returns 128 bits
extern uint64_t name##64(void);  // returns 64 bits (simply half of the 128 bit result)
extern void name##_bytes(void *buffer, size_t len); // fills a buffer
extern void name##_reset(void); // resets the state of the PRNG
extern void name##_reseed(void); // reseeds the PRNG using time and jitter
extern void name##_on_fork(void *is_null); // reseeds the PRNG (for fork safety)
```

If `reseed_log` is non-zero and less than 64, the PNGR is no longer deterministic, as it will automatically re-seeds itself every `1 << reseed_log` iterations using a loop measuring both time and CPU 'jitter'.

If `extern` is `static` or `FIO_SFUNC`, a `static` function will be defined.

-------------------------------------------------------------------------------

## Core Binary Strings and Buffer Helpers

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

#### `FIO_STR_INFO0`

```c
#define FIO_STR_INFO0 ((fio_str_info_s){0})
```

A NULL fio_str_info_s.

#### `FIO_STR_INFO_IS_EQ`

```c
#define FIO_STR_INFO_IS_EQ(s1, s2)
```

This helper MACRO compares two `fio_str_info_s` / `fio_buf_info_s` objects for content equality.

#### `FIO_BUF_INFO_IS_EQ`

```c
#define FIO_BUF_INFO_IS_EQ(s1, s2) FIO_STR_INFO_IS_EQ((s1), (s2))
```

This helper MACRO compares two `fio_buf_info_s` objects for content equality.

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

#### `FIO_BUF_INFO0`

```c
#define FIO_BUF_INFO0 ((fio_buf_info_s){0})
```

A NULL fio_buf_info_s.

#### `FIO_BUF_INFO1`

```c
#define FIO_BUF_INFO1(str)                                                     \
  ((fio_buf_info_s){.len = ((str) ? FIO_STRLEN((str)) : 0), .buf = (str)})
```

Converts a C String into a `fio_buf_info_s`.

#### `FIO_BUF_INFO2`

```c
#define FIO_BUF_INFO2(str, length)                                             \
  ((fio_buf_info_s){.len = (length), .buf = (str)})
```

Converts a String with a known length into a `fio_buf_info_s`.

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

Converts a `fio_str_info_s` into a `fio_buf_info_s`.

#### `FIO_STR_INFO_TMP_VAR(name, capacity)`

```c
#define FIO_STR_INFO_TMP_VAR(name, capacity)                                   \
  char fio___stack_mem___##name[(capacity) + 1];                               \
  fio___stack_mem___##name[0] = 0; /* guard */                                 \
  fio_str_info_s name = (fio_str_info_s) {                                     \
    .buf = fio___stack_mem___##name, .capa = (capacity)                        \
  }
```

Creates a stack fio_str_info_s variable `name` with `capacity` bytes (including 1 extra byte for a `NUL` guard).

#### `FIO_STR_INFO_TMP_IS_REALLOCATED`

```c
#define FIO_STR_INFO_TMP_IS_REALLOCATED(name) (fio___stack_mem___##name != name.buf)
```

Tests to see if memory reallocation happened for a `FIO_STR_INFO_TMP_VAR`.

-------------------------------------------------------------------------------

### Core UTF-8 Support

Note, these functions are for single UTF-8 character / code-point handling, they don't check for memory bounds (may overflow) and aren't considered safe.

However, they could be used for writing safe a UTF-8 implementation if used with care. Assuming your strings end with a little padding or a NUL character, these should be safe to use (when writing, check available buffer before writing a possibly multi-byte UTF-8 char.

#### `fio_utf8_code_len`

```c
unsigned fio_utf8_code_len(uint32_t u);
```

Returns the number of bytes required to UTF-8 encoded a code point `u`.

#### `fio_utf8_char_len_unsafe`

```c
unsigned fio_utf8_char_len_unsafe(uint8_t c);
```

Returns 1-4 (UTF-8 char length), 8 (middle of a char) or 0 (invalid).

Use only to re-collect lost length information after a successful `fio_utf8_write` or `fio_utf8_char_len` call.

#### `fio_utf8_char_len`

```c
unsigned fio_utf8_char_len(const void *str);
```

Returns the number of valid UTF-8 bytes used by first char at `str`.

If `str` doesn't point to a valid UTF-8 encoded code-point, returns 0.

**Note**: This function also tests all the following bytes that are part of the same UTF-8 character.


#### `fio_utf8_write`

```c
unsigned fio_utf8_write(void *dest, uint32_t u);
```

Writes code point to `dest` using UFT-8. Returns number of bytes written.

Possible use pattern will be:

```c
dest += fio_utf8_write(dest, u);
```


#### `fio_utf8_read`

```c
uint32_t fio_utf8_read(char **str);
```

Decodes the first UTF-8 char at `str` and returns its code point value.

Advances the pointer at `str` by the number of bytes consumed (read).

#### `fio_utf8_peek`

```c
uint32_t fio_utf8_peek(const char *str);
```

Decodes the first UTF-8 char at `str` and returns its code point value.

Unlike `fio_utf8_read`, the pointer does not change.

-------------------------------------------------------------------------------
