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

#### `FIO_CRYPT`

When `FIO_CRYPT` is defined, all hash and cryptographic modules are included.

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

Returns the number of unfreed allocattions according to the named memory leak detector.

Returned type is `size_t`

-------------------------------------------------------------------------------

## Dedicated Static Memory Allocations

The core module provides a simple way to manage a "good enough" thread safe memory allocations for short term use that never need to be freed (e.g., when the caller cannot be trusted to free the memory).

#### `FIO_STATIC_ALLOC_DEF`

```c
#ifndef FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX
/* Maximum number of threads to be expected to call the allocator at once. */
#define FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX 256
#endif
#define FIO_STATIC_ALLOC_DEF(function_name,                                             \
                             type_T,                                           \
                             size_per_allocation,                              \
                             allocations_per_thread) /* ... */ 
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

The following example uses static memory to create a (zero allocation) number to string converter and then uses it to print out a number's value for logging:

```c
// Step 1: define the allocator:
//
// This will define a static allocator,
// allocating 19 byte long strings per allocation.
// Reserving 4,864 bytes in static memory.
FIO_STATIC_ALLOC_DEF(numer2hex_allocator, char, 19, 1);

// Step 2: use the allocator
//
// example function that returns an unsigned number as a 16 digit hex string
char * ntos16(uint16_t n) {
  // allocates 19 bytes from the static memory pool.
  char * n = numer2hex_allocator(1);
  // Writes the number in hex + NUL byte.
  n[0] = '0'; n[1] = 'x';
  fio_ltoa16u(n+2, n, 16);
  n[18] = 0;
  // returns the string (a pointer to the 19 bytes).
  return n;
}

// Step 3: example use-case.
void print_number_for_debugging(char * file, int line, char * var, uint16_t n) {
  printf("(%s:%d) %s = %s", file, line, var_name, ntos16(n));
}

// Prints any number in hex, along with file location and variable name.
#define LOG_PRINT_VAL(n) print_number_for_debugging(__FILE__, __LINE__, #n, n)
```

A similar approach is use by `fiobj_num2cstr` in order to provide temporary conversions of FIOBJ to a C String without the caller having to reason about memory management.

-------------------------------------------------------------------------------

## Pointer Arithmetics

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

#### `FIO_DEF_GET_FUNC` / `FIO_DEF_GET_FUNC_DEF`

```c
#define FIO_DEF_GET_FUNC(static, namespace, T_type, F_type, field_name)        \
  /** Returns current value of property within the struct / union. */          \
  static F_type FIO_NAME(namespace, field_name)(T_type * o) {                  \
    FIO_ASSERT_DEBUG(o, "NULL " FIO_MACRO2STR(namespace) " pointer @ `get`!"); \
    return o->field_name;                                                      \
  }

#define FIO_DEF_GET_FUNC_DEF(static, namespace, T_type, F_type, field_name)    \
  /** Returns current value of property within the struct / union. */          \
  static F_type FIO_NAME(namespace, field_name)(T_type * o);
```

Defines a `get` function for a field within a struct / union.

This allows for consistent naming of `get` functions.

**Note**: Normally, unlike `_set`, this function doesn't imply an action was taken or a side-effect occurred. This is why, by default, no `_get` postix is used. A `_get` postfix implies an action may be performed, such as lazy initialization, etc'.

#### `FIO_DEF_SET_FUNC` / `FIO_DEF_SET_FUNC_DEF`

```c
#define FIO_DEF_SET_FUNC(static, namespace, T_type, F_type, F_name, on_set)    \
  /** Sets a new value, returning the old one */                               \
  static F_type FIO_NAME(FIO_NAME(namespace, field_name),                      \
                         set)(T_type * o, F_type new_value) {                  \
    FIO_ASSERT_DEBUG(o, "NULL " FIO_MACRO2STR(namespace) " pointer @ `set`!"); \
    F_type old_value = o->field_name;                                          \
    o->field_name = new_value;                                                 \
    on_set(o);                                                                 \
    return old_value;                                                          \
  }

#define FIO_DEF_SET_FUNC_DEF(static, namespace, T_type, F_type, F_name)        \
  /** Sets a new value, returning the old one */                               \
  static F_type FIO_NAME(FIO_NAME(namespace, F_name), set)(T_type * o,         \
                                                           F_type new_value);
```

Defines a `set` function for a field within a struct / union.

This allows for consistent naming of `set` functions.

#### `FIO_DEF_GETSET_FUNC`

```c
#define FIO_DEF_GETSET_FUNC(static, namespace, T_type, F_type, F_name, on_set) \
  FIO_DEF_GET_FUNC(static, namespace, T_type, F_type, F_name)                  \
  FIO_DEF_SET_FUNC(static, namespace, T_type, F_type, F_name, on_set)
```

Defines `get` / `set` functions for a field within a struct / union.

#### `FIO_IFUNC_DEF_GET`

```c
#define FIO_IFUNC_DEF_GET(namespace, T_type, F_type, field_name)               \
  FIO_DEF_GET_FUNC(FIO_IFUNC, namespace, T_type, F_type, field_name)
```

Defines a `get` function for a field within a struct / union.

This allows for consistent naming of `get` functions.

**Note**: Normally, unlike `_set`, this function doesn't imply an action was taken or a side-effect occurred. This is why, by default, no `_get` postix is used. A `_get` postfix implies an action may be performed, such as lazy initialization, etc'.

#### `FIO_IFUNC_DEF_SET`

```c
#define FIO_IFUNC_DEF_SET(namespace, T_type, F_type, F_name, on_set)               \
  FIO_DEF_SET_FUNC(FIO_IFUNC, namespace, T_type, F_type, F_name, on_set)
```

Defines a `set` function for a field within a struct / union.

The `on_set` callback accepts a pointer to the updated struct and a pointer to the old value (in this order).

If no `on_set` is required, using `FIO_NOOP_FN` as the `on_set` value.

#### `FIO_IFUNC_DEF_GETSET`

```c
#define FIO_IFUNC_DEF_GETSET(namespace, T_type, F_type, F_name, on_set)        \
  FIO_IFUNC_DEF_GET(namespace, T_type, F_type, F_name)                         \
  FIO_IFUNC_DEF_SET(namespace, T_type, F_type, F_name, on_set)
```

Shortcut for defining both a `get` and a `set` function for a field within a struct / union.

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
#ifndef FIO_STRLEN
#define FIO_STRLEN fio_strlen
#endif
#endif /* FIO_MEMALT */
```

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

    Tests if `bool == 1` (returns `a` if `true` and `b` if `false`).

- `fio_ct_if(condition, a_if_true, b_if_false)`

    Tests if `condition` is non-zero (returns `a` / `b`).

- `fio_ct_max(a, b)`

    Returns `a` if a >= `b` (performs a **signed** comparison).


- `fio_ct_is_eq(const void * a, const void * b, len)`

    Returns 1 if memory regions are equal. Should be resistant to timing attacks.

-------------------------------------------------------------------------------

## Inspecting Byte / Bit Data

**Note**: the 128 bit helpers are only available with systems / compilers that support 128 bit types.

**Note**: for mutable shared data, please consider using the atomic operations.

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
                      const size_t len)
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
                  const size_t len)
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
void fio_u16x8_add(uint16_t * dest, uint16_t * a, uint16_t *b);
void fio_u16x16_add(uint16_t * dest, uint16_t * a, uint16_t *b);
void fio_u16x32_add(uint16_t * dest, uint16_t * a, uint16_t *b);
void fio_u16x64_add(uint16_t * dest, uint16_t * a, uint16_t *b);
void fio_u16x128_add(uint16_t * dest, uint16_t * a, uint16_t *b);

void fio_u32x2_add(uint32_t * dest, uint32_t * a, uint32_t *b);
void fio_u32x4_add(uint32_t * dest, uint32_t * a, uint32_t *b);
void fio_u32x8_add(uint32_t * dest, uint32_t * a, uint32_t *b);
void fio_u32x16_add(uint32_t * dest, uint32_t * a, uint32_t *b);
void fio_u32x32_add(uint32_t * dest, uint32_t * a, uint32_t *b);
void fio_u32x64_add(uint32_t * dest, uint32_t * a, uint32_t *b);

void fio_u64x2_add(uint64_t * dest, uint64_t * a, uint64_t *b);
void fio_u64x4_add(uint64_t * dest, uint64_t * a, uint64_t *b);
void fio_u64x8_add(uint64_t * dest, uint64_t * a, uint64_t *b);
void fio_u64x16_add(uint64_t * dest, uint64_t * a, uint64_t *b);
void fio_u64x32_add(uint64_t * dest, uint64_t * a, uint64_t *b);
```

The following operations are supported: `add`, `mul`, `and`, `or`, `xor`.

#### Vectorized Mathematical Summing (`fio_u##x#_OP`)

Sum vectors up using Add, Or, XOR, and more using any of the following (or similarly named):

```c
uint8_t fio_u8x4_reduce_add(uint8_t * v);
uint8_t fio_u8x8_reduce_add(uint8_t * v);
uint8_t fio_u8x16_reduce_add(uint8_t * v);
uint8_t fio_u8x32_reduce_add(uint8_t * v);
uint8_t fio_u8x64_reduce_add(uint8_t * v);
uint8_t fio_u8x128_reduce_add(uint8_t * v);
uint8_t fio_u8x256_reduce_add(uint8_t * v);

uint16_t fio_u16x2_reduce_add(uint16_t * v);
uint16_t fio_u16x4_reduce_add(uint16_t * v);
uint16_t fio_u16x8_reduce_add(uint16_t * v);
uint16_t fio_u16x16_reduce_add(uint16_t * v);
uint16_t fio_u16x32_reduce_add(uint16_t * v);
uint16_t fio_u16x64_reduce_add(uint16_t * v);
uint16_t fio_u16x128_reduce_add(uint16_t * v);

uint32_t fio_u32x2_reduce_add(uint32_t * v);
uint32_t fio_u32x4_reduce_add(uint32_t * v);
uint32_t fio_u32x8_reduce_add(uint32_t * v);
uint32_t fio_u32x16_reduce_add(uint32_t * v);
uint32_t fio_u32x32_reduce_add(uint32_t * v);
uint32_t fio_u32x64_reduce_add(uint32_t * v);

uint64_t fio_u64x2_reduce_add(uint64_t * v);
uint64_t fio_u64x4_reduce_add(uint64_t * v);
uint64_t fio_u64x8_reduce_add(uint64_t * v);
uint64_t fio_u64x16_reduce_add(uint64_t * v);
uint64_t fio_u64x32_reduce_add(uint64_t * v);

```

The following summation operations are supported: `max`, `min`, `add`, `mul`, `and`, `or`, `xor`.

#### Reshuffling (`fio_u##x#_reshuffle`)

Reorders the words inside the vector.

```c
#define fio_u8x4_reshuffle(v, ...)     fio_u8x4_reshuffle(v,     (uint8_t[4]){__VA_ARGS__})
#define fio_u8x8_reshuffle(v, ...)     fio_u8x8_reshuffle(v,     (uint8_t[8]){__VA_ARGS__})
#define fio_u8x16_reshuffle(v, ...)    fio_u8x16_reshuffle(v,    (uint8_t[16]){__VA_ARGS__})
#define fio_u8x32_reshuffle(v, ...)    fio_u8x32_reshuffle(v,    (uint8_t[32]){__VA_ARGS__})
#define fio_u8x64_reshuffle(v, ...)    fio_u8x64_reshuffle(v,    (uint8_t[64]){__VA_ARGS__})
#define fio_u8x128_reshuffle(v, ...)   fio_u8x128_reshuffle(v,   (uint8_t[128]){__VA_ARGS__})
#define fio_u8x256_reshuffle(v, ...)   fio_u8x256_reshuffle(v,   (uint8_t[256]){__VA_ARGS__})
#define fio_u16x2_reshuffle(v, ...)    fio_u16x2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
#define fio_u16x4_reshuffle(v, ...)    fio_u16x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
#define fio_u16x8_reshuffle(v, ...)    fio_u16x8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
#define fio_u16x16_reshuffle(v, ...)   fio_u16x16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
#define fio_u16x32_reshuffle(v, ...)   fio_u16x32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
#define fio_u16x64_reshuffle(v, ...)   fio_u16x64_reshuffle(v,   (uint8_t[64]){__VA_ARGS__})
#define fio_u16x128_reshuffle(v,...)   fio_u16x128_reshuffle(v,  (uint8_t[128]){__VA_ARGS__})
#define fio_u32x2_reshuffle(v, ...)    fio_u32x2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
#define fio_u32x4_reshuffle(v, ...)    fio_u32x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
#define fio_u32x8_reshuffle(v, ...)    fio_u32x8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
#define fio_u32x16_reshuffle(v, ...)   fio_u32x16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
#define fio_u32x32_reshuffle(v, ...)   fio_u32x32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
#define fio_u32x64_reshuffle(v, ...)   fio_u32x64_reshuffle(v,   (uint8_t[64]){__VA_ARGS__})
#define fio_u64x2_reshuffle(v, ...)    fio_u64x2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
#define fio_u64x4_reshuffle(v, ...)    fio_u64x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
#define fio_u64x8_reshuffle(v, ...)    fio_u64x8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
#define fio_u64x16_reshuffle(v, ...)   fio_u64x16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
#define fio_u64x32_reshuffle(v, ...)   fio_u64x32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
#define fio_floatx2_reshuffle(v, ...)  fio_floatx2_reshuffle(v,  (uint8_t[2]){__VA_ARGS__})
#define fio_floatx4_reshuffle(v, ...)  fio_floatx4_reshuffle(v,  (uint8_t[4]){__VA_ARGS__})
#define fio_floatx8_reshuffle(v, ...)  fio_floatx8_reshuffle(v,  (uint8_t[8]){__VA_ARGS__})
#define fio_floatx16_reshuffle(v, ...) fio_floatx16_reshuffle(v, (uint8_t[16]){__VA_ARGS__})
#define fio_floatx32_reshuffle(v, ...) fio_floatx32_reshuffle(v, (uint8_t[32]){__VA_ARGS__})
#define fio_floatx64_reshuffle(v, ...) fio_floatx64_reshuffle(v, (uint8_t[64]){__VA_ARGS__})
#define fio_dblx2_reshuffle(v, ...)    fio_dblx2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
#define fio_dblx4_reshuffle(v, ...)    fio_dblx4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
#define fio_dblx8_reshuffle(v, ...)    fio_dblx8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
#define fio_dblx16_reshuffle(v, ...)   fio_dblx16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
#define fio_dblx32_reshuffle(v, ...)   fio_dblx32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
```

-------------------------------------------------------------------------------

## Atomic Operations (Core)

```c
#define FIO_ATOMIC
#include "fio-stl.h"
```

If the `FIO_ATOMIC` macro is defined than the following macros will be defined.

In general, when a function returns a value, it is always the previous value - unless the function name ends with `fetch` or `load`.

#### `fio_atomic_load(p_obj)`

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

- `fio_atomic_bit_get(void *map, size_t bit)`

- `fio_atomic_bit_set(void *map, size_t bit)`   (an **atomic** operation, thread-safe)

- `fio_atomic_bit_unset(void *map, size_t bit)` (an **atomic** operation, thread-safe)

Gets / Sets bits an atomic thread-safe way.

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

Busy waits for all sub-locks to become available - not recommended.

#### `fio_unlock_full(fio_lock_i *lock)`

Unlocks all sub-locks, no matter which thread owns which lock.

-------------------------------------------------------------------------------

## Numeral / Vector Helper Types

The following union types hold (little endian) arrays of unsigned 64 bit numbers that are accessible also as byte arrays or smaller numeral types:

- `fio_u128`
- `fio_u256`
- `fio_u512`
- `fio_u1024`
- `fio_u2048`
- `fio_u4096`

The main ones are as follows (the rest follow the same pattern):

#### `fio_u128`

```c
typedef union {
  /** unsigned native word size array, length is system dependent */
  size_t uz[16 / sizeof(size_t)];

  /** known bit word arrays */
  uint64_t u64[2];
  uint32_t u32[4];
  uint16_t u16[8];
  uint8_t u8[16];

  /** vector types, if supported */
#if FIO___HAS_ARM_INTRIN
  uint64x2_t x64[1];
  uint32x4_t x32[1];
  uint16x8_t x16[1];
  uint8x16_t x8[1];
#elif __has_attribute(vector_size)
  uint64_t x64 __attribute__((vector_size(16)));
  uint64_t x32 __attribute__((vector_size(16)));
  uint64_t x16 __attribute__((vector_size(16)));
  uint64_t x8 __attribute__((vector_size(16)));
#endif
#if defined(__SIZEOF_INT128__)
  __uint128_t alignment_for_u128_[1];
#endif
} fio_u128 FIO_ALIGN(16);
```

An unsigned 128 bit union type.

#### `fio_u256`

```c
typedef union {
  /** unsigned native word size array, length is system dependent */
  size_t uz[32 / sizeof(size_t)];

  /** known bit word arrays */
  uint64_t u64[4];
  uint32_t u32[8];
  uint16_t u16[16];
  uint8_t u8[32];

  /* previous types */
  fio_u128 u128[2];

  /** vector types, if supported */
#if FIO___HAS_ARM_INTRIN
  uint64x2_t x64[2];
  uint32x4_t x32[2];
  uint16x8_t x16[2];
  uint8x16_t x8[2];
#elif __has_attribute(vector_size)
  uint64_t x64 __attribute__((vector_size(32)));
  uint64_t x32 __attribute__((vector_size(32)));
  uint64_t x16 __attribute__((vector_size(32)));
  uint64_t x8 __attribute__((vector_size(32)));
#endif
#if defined(__SIZEOF_INT128__)
  __uint128_t alignment_for_u128_[2];
#endif
#if defined(__SIZEOF_INT256__)
  __uint256_t alignment_for_u256_[1];
#endif
} fio_u256 FIO_ALIGN(16);
```

An unsigned 256 bit union type.

#### `fio_u512`

```c
typedef union {
  /** unsigned native word size array, length is system dependent */
  size_t uz[64 / sizeof(size_t)];

  /** known bit word arrays */
  uint64_t u64[8];
  uint32_t u32[16];
  uint16_t u16[32];
  uint8_t u8[64];

  /* previous types */
  fio_u128 u128[4];
  fio_u256 u256[2];

  /** vector types, if supported */
#if FIO___HAS_ARM_INTRIN
  uint64x2_t x64[4];
  uint32x4_t x32[4];
  uint16x8_t x16[4];
  uint8x16_t x8[4];
#elif __has_attribute(vector_size)
  uint64_t x64 __attribute__((vector_size(64)));
  uint64_t x32 __attribute__((vector_size(64)));
  uint64_t x16 __attribute__((vector_size(64)));
  uint64_t x8 __attribute__((vector_size(64)));
#endif
} fio_u512 FIO_ALIGN(16);
```

An unsigned 512 bit union type.

### Numeral / Vector Helper Type Initialization

Fast and easy macros are provided for these numeral helper initialization, initializing any provided X bit words in least-significant-word ordering and initializing any remaining higher words with zero.

#### `fio_u128_init8` / `fio_u128_init16` / `fio_u128_init32` / `fio_u128_init64` ...

```c
#define fio_u128_init8(...)  ((fio_u128){.u8 = {__VA_ARGS__}})
#define fio_u128_init16(...) ((fio_u128){.u16 = {__VA_ARGS__}})
#define fio_u128_init32(...) ((fio_u128){.u32 = {__VA_ARGS__}})
#define fio_u128_init64(...) ((fio_u128){.u64 = {__VA_ARGS__}})

#define fio_u256_init8(...)  ((fio_u256){.u8 = {__VA_ARGS__}})
#define fio_u256_init16(...) ((fio_u256){.u16 = {__VA_ARGS__}})
#define fio_u256_init32(...) ((fio_u256){.u32 = {__VA_ARGS__}})
#define fio_u256_init64(...) ((fio_u256){.u64 = {__VA_ARGS__}})

#define fio_u512_init8(...)  ((fio_u512){.u8 = {__VA_ARGS__}})
#define fio_u512_init16(...) ((fio_u512){.u16 = {__VA_ARGS__}})
#define fio_u512_init32(...) ((fio_u512){.u32 = {__VA_ARGS__}})
#define fio_u512_init64(...) ((fio_u512){.u64 = {__VA_ARGS__}})

#define fio_u1024_init8(...)  ((fio_u1024){.u8 = {__VA_ARGS__}})
#define fio_u1024_init16(...) ((fio_u1024){.u16 = {__VA_ARGS__}})
#define fio_u1024_init32(...) ((fio_u1024){.u32 = {__VA_ARGS__}})
#define fio_u1024_init64(...) ((fio_u1024){.u64 = {__VA_ARGS__}})

#define fio_u2048_init8(...)  ((fio_u2048){.u8 = {__VA_ARGS__}})
#define fio_u2048_init16(...) ((fio_u2048){.u16 = {__VA_ARGS__}})
#define fio_u2048_init32(...) ((fio_u2048){.u32 = {__VA_ARGS__}})
#define fio_u2048_init64(...) ((fio_u2048){.u64 = {__VA_ARGS__}})

#define fio_u4096_init8(...)  ((fio_u4096){.u8 = {__VA_ARGS__}})
#define fio_u4096_init16(...) ((fio_u4096){.u16 = {__VA_ARGS__}})
#define fio_u4096_init32(...) ((fio_u4096){.u32 = {__VA_ARGS__}})
#define fio_u4096_init64(...) ((fio_u4096){.u64 = {__VA_ARGS__}})
```

### Numeral / Vector Helper Type Load / Store

These numerals can be stored and loaded from memory using big endian / little endian formatting:

#### `fio_u128_load`, `fio_u256_load` ...

- `fio_u128 fio_u128_load(const void *buf)`          - load in native ordering.
- `void fio_u128_store(void *buf, const fio_u128 a)` - store in native ordering.
- `fio_u128 fio_u128_load_le16(const void *buf)` - load in little endian ordering using 16 bit words.
- `fio_u128 fio_u128_load_be16(const void *buf)` - load in big endian ordering using 16 bit words.
- `fio_u128 fio_u128_bswap16(const void *buf)`   - load in and byte-swap using 16 bit words (2 bytes).
- `fio_u128 fio_u128_load_le32(const void *buf)` - load in little endian ordering using 32 bit words.
- `fio_u128 fio_u128_load_be32(const void *buf)` - load in big endian ordering using 32 bit words.
- `fio_u128 fio_u128_bswap32(const void *buf)`   - load in and byte-swap using 32 bit words (2 bytes).
- `fio_u128 fio_u128_load_le64(const void *buf)` - load in little endian ordering using 64 bit words.
- `fio_u128 fio_u128_load_be64(const void *buf)` - load in big endian ordering using 64 bit words.
- `fio_u128 fio_u128_bswap64(const void *buf)`   - load in and byte-swap using 64 bit words (2 bytes).

- `fio_u256 fio_u256_load(const void *buf)`          - load in native ordering.
- `void fio_u256_store(void *buf, const fio_u256 a)` - store in native ordering.
- ... (etc')

### Numeral / Vector Helper Type Operation

Common mathematical operations are provided for the Vector Helper Types, much like they were provided for the Native C typed vectors.

#### `fio_u128_add16`, `fio_u256_sub32`, `fio_u512_mul64` ...

These functions follow the naming scheme of `fio_u##TOTAL_BITS##_##OP##WORD_BITS`, where `TOTAL_BITS` is the total number of bits, `OP` is the name of the operation (`add`, `sub`, `mul`, etc') and `WORD_BITS` is the number of bits in each vector "word".

i.e:

- `void fio_u128_add8(fio_u128 *t, fio_u128 *a, fio_u128 *b)`   - performs a modular 8 bit ADD (`+`) operation `a+b`.
- `void fio_u256_sub16(fio_u256 *t, fio_u256 *a, fio_u256 *b)`  - performs a modular 16 bit SUB (`-`) operation `a-b`.
- `void fio_u512_mul32(fio_u512 *t, fio_u512 *a, fio_u512 *b)`  - performs a modular 32 bit MUL (`*`) operation `a*b`.
- `void fio_u1024_and(fio_u512 *t, fio_u512 *a, fio_u512 *b)`   - performs a bit AND (`&`) operation `a&b`.
- `void fio_u2048_or(fio_u512 *t, fio_u512 *a, fio_u512 *b)`    - performs a bit OR (`|`) operation `a|b`.
- `void fio_u4096_xor(fio_u512 *t, fio_u512 *a, fio_u512 *b)`   - performs a bit XOR (`^`) operation `a^b`.

Supported operations (`OP`) are: `add`, `sub`, `mul`, `and`, `or`, `xor`.

For bitwise operations, the `WORD_BITS` part of the function name is optional.


#### `fio_u128_cadd16`, `fio_u128_csub32`, `fio_u256_cmul64` ...

These functions perform an operation `a OP b` where `b` is a constant rather than a vector.

i.e.:

- `void fio_u128_cadd8(fio_u128 *t, fio_u128 *a, uint8_t b)`     - performs a modular 8 bit ADD (`+`) operation `a+b`.
- `void fio_u256_csub16(fio_u256 *t, fio_u256 *a, uint16_t b)`   - performs a modular 16 bit SUB (`-`) operation `a-b`.
- `void fio_u512_cmul32(fio_u512 *t, fio_u512 *a, uint32_t b)`   - performs a modular 32 bit MUL (`*`) operation `a*b`.
- `void fio_u1024_cand8(fio_u512 *t, fio_u512 *a, uint8_t b)`    - performs an 8 bit AND (`&`) operation `a&b`.
- `void fio_u2048_cor16(fio_u512 *t, fio_u512 *a, uint16_t b)`   - performs a 16 bit OR (`|`) operation `a|b`.
- `void fio_u4096_cxor32(fio_u512 *t, fio_u512 *a, uint32_t b)`  - performs a 32 bit XOR (`^`) operation `a^b`.


#### Multi-Precision `fio_u128_add`, `fio_u256_sub`, `fio_u512_mul` ...

These functions provide Multi-Precision operations for the Numeral / Vector Helper Types.

The functions support up to 2048 bits of multi-precision. The 4096 bit type is mostly reserved for storing the result of multiplying two 2048 bit numerals (as storing the result requires bit expansion from 2048 bits to 4096 bits).

- `bool fio_u128_add(fio_u128 *t, fio_u128 *a, fio_u128 *b)`   - performs ADD (`t=a+b`), returning the carry bit.
- `bool fio_u256_sub(fio_u256 *t, fio_u256 *a, fio_u256 *b)`   - performs SUB (`t=a-b`), returning the borrow bit.
- `void fio_u512_mul(fio_u512 *t, fio_u256 *a, fio_u256 *b)`   - performs MUL (`t=a*b`) operation.


-------------------------------------------------------------------------------

## Core Randomness

The core module provides macros for generating semi-deterministic Pseudo-Random Number Generator functions.

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

### Core UTF-8 Support

Note, these functions are for single UTF-8 character / code-point handling, they don't check for memory bounds (may overflow) and aren't considered safe.

However, they could be used for writing safe a UTF-8 implementation if used with care. Assuming your strings end with a little padding or a NUL character, these should be safe to use (when writing, check available buffer before writing a possibly multi-byte UTF-8 char.

#### `fio_utf8_code_len`

```c
size_t fio_utf8_code_len(uint32_t u);
```

Returns the number of bytes required to UTF-8 encoded a code point `u`.

#### `fio_utf8_char_len_unsafe`

```c
size_t fio_utf8_char_len_unsafe(uint8_t c);
```

Returns 1-4 (UTF-8 char length), 8 (middle of a char) or 0 (invalid).

Use only to re-collect lost length information after a successful `fio_utf8_write` or `fio_utf8_char_len` call.

#### `fio_utf8_char_len`

```c
size_t fio_utf8_char_len(const void *str);
```

Returns the number of valid UTF-8 bytes used by first char at `str`.

If `str` doesn't point to a valid UTF-8 encoded code-point, returns 0.

**Note**: This function also tests all the following bytes that are part of the asme UTF-8 character.


#### `fio_utf8_char_len`

```c
size_t fio_utf8_write(void *dest, uint32_t u);
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
uint32_t fio_utf8_peek(char *str);
```

Decodes the first UTF-8 char at `str` and returns its code point value.

Unlike `fio_utf8_read`, the pointer does not change.

## Doubly Linked Lists

```c
// initial `include` defines the `FIO_LIST_NODE` macro and type
#include "fio-stl.h"
// list element 
typedef struct {
  FIO_LIST_NODE node;
  char * data;
} my_list_s;
```

Doubly Linked Lists are an incredibly common and useful data structure.

### Linked Lists Performance

Memory overhead (on 64bit machines) is 16 bytes per node (or 8 bytes on 32 bit machines) for the `next` and `prev` pointers.

Linked Lists use pointers in order to provide fast add/remove operations with O(1) speeds. This O(1) operation ignores the object allocation time and suffers from poor memory locality, but it's still very fast.

However, Linked Lists suffer from slow seek/find and iteration operations.

Seek/find has a worst case scenario O(n) cost and iteration suffers from a high likelihood of CPU cache misses, resulting in degraded performance.

### Linked Lists Macros

Linked List Macros (and arch-type) are always defined by the CSTL and can be used to manage linked lists without creating a dedicated type.

#### `FIO_LIST_NODE` / `FIO_LIST_HEAD`

```c
/** A linked list node type */
#define FIO_LIST_NODE fio_list_node_s
/** A linked list head type */
#define FIO_LIST_HEAD fio_list_node_s
/** A linked list arch-type */
typedef struct fio_list_node_s {
  struct fio_list_node_s *next;
  struct fio_list_node_s *prev;
} fio_list_node_s;

```

These are the basic core types for a linked list node used by the Linked List macros.

#### `FIO_LIST_INIT(head)`

```c
#define FIO_LIST_INIT(obj)                                                     \
  (FIO_LIST_HEAD){ .next = &(obj), .prev = &(obj) }
```

Initializes a linked list.

#### `FIO_LIST_PUSH`

```c
#define FIO_LIST_PUSH(head, n)                                                 \
  do {                                                                         \
    (n)->prev = (head)->prev;                                                  \
    (n)->next = (head);                                                        \
    (head)->prev->next = (n);                                                  \
    (head)->prev = (n);                                                        \
  } while (0)
```

UNSAFE macro for pushing a node to a list.

Note that this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

#### `FIO_LIST_POP`

```c
#define FIO_LIST_POP(type, node_name, dest_ptr, head)                          \
  do {                                                                         \
    (dest_ptr) = FIO_PTR_FROM_FIELD(type, node_name, ((head)->next));          \
    FIO_LIST_REMOVE(&(dest_ptr)->node_name);                                   \
  } while (0)
```

UNSAFE macro for popping a node from a list.

* `type` is the underlying `struct` type of the next list member.

* `node_name` is the field name in the `type` that is the `FIO_LIST_NODE` linking type.

* `dest_prt` is the pointer that will accept the next list member.

* `head` is the head of the list.

Note that this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

Note that using this macro with an empty list will produce **undefined behavior**.

#### `FIO_LIST_REMOVE`

```c
#define FIO_LIST_REMOVE(n)                                                     \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
  } while (0)
```

UNSAFE macro for removing a node from a list.

Note that this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.


#### `FIO_LIST_REMOVE_RESET`

```c
#define FIO_LIST_REMOVE_RESET(n)                                                     \
  do {                                                                         \
    (n)->prev->next = (n)->next;                                               \
    (n)->next->prev = (n)->prev;                                               \
    (n)->next = (n)->prev = (n);                                               \
  } while (0)
```

UNSAFE macro for removing a node from a list. Resets node data so it links to itself.

Note that this macro does not test that the list / data was initialized before reading / writing to the memory pointed to by the list / node.

#### `FIO_LIST_EACH`

```c
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
```

Loops through every node in the linked list except the head.

This macro allows `pos` to point to the type that the linked list contains (rather than a pointer to the node type).

i.e.,

```c
typedef struct {
  void * data;
  FIO_LIST_HEAD node;
} ptr_list_s;

FIO_LIST_HEAD my_ptr_list = FIO_LIST_INIT(my_ptr_list);

/* ... */

FIO_LIST_EACH(ptr_list_s, node, &my_ptr_list, pos) {
  do_something_with(pos->data);
}
```

#### `FIO_LIST_IS_EMPTY`

```c
#define FIO_LIST_IS_EMPTY(head) (!(head) || (head)->next == (head)->prev)
```

Macro for testing if a list is empty.


### Indexed Linked Lists Macros (always defined):


Indexed linked lists are often used to either save memory or making it easier to reallocate the memory used for the whole list. This is performed by listing pointer offsets instead of the whole pointer, allowing the offsets to use smaller type sizes.

For example, an Indexed Linked List might be added to objects in a cache array in order to implement a "least recently used" eviction policy. If the cache holds less than 65,536 members, than a 16 bit index is all that's required, reducing the list's overhead from 2 pointers (16 bytes on 64 bit systems) to a 4 byte overhead per cache member.

#### `FIO_INDEXED_LIST##_HEAD` / `FIO_INDEXED_LIST##_NODE`

```c
/** A 32 bit indexed linked list node type */
#define FIO_INDEXED_LIST32_NODE fio_index32_node_s
#define FIO_INDEXED_LIST32_HEAD uint32_t
/** A 16 bit indexed linked list node type */
#define FIO_INDEXED_LIST16_NODE fio_index16_node_s
#define FIO_INDEXED_LIST16_HEAD uint16_t
/** An 8 bit indexed linked list node type */
#define FIO_INDEXED_LIST8_NODE fio_index8_node_s
#define FIO_INDEXED_LIST8_HEAD uint8_t

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
```

#### `FIO_INDEXED_LIST_PUSH`

```c
#define FIO_INDEXED_LIST_PUSH(root, node_name, head, i)                        \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[n__].node_name.prev = (root)[(head)].node_name.prev;                \
    (root)[n__].node_name.next = (head);                                       \
    (root)[(root)[(head)].node_name.prev].node_name.next = n__;                \
    (root)[(head)].node_name.prev = n__;                                       \
  } while (0)
```

UNSAFE macro for pushing a node to a list.

#### `FIO_INDEXED_LIST_REMOVE`

```c
#define FIO_INDEXED_LIST_REMOVE(root, node_name, i)                            \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[(root)[n__].node_name.prev].node_name.next =                        \
        (root)[n__].node_name.next;                                            \
    (root)[(root)[n__].node_name.next].node_name.prev =                        \
        (root)[n__].node_name.prev;                                            \
  } while (0)
```

UNSAFE macro for removing a node from a list.

#### `FIO_INDEXED_LIST_REMOVE_RESET`

```c
#define FIO_INDEXED_LIST_REMOVE_RESET(root, node_name, i)                            \
  do {                                                                         \
    register const size_t n__ = (i);                                           \
    (root)[(root)[n__].node_name.prev].node_name.next =                        \
        (root)[n__].node_name.next;                                            \
    (root)[(root)[n__].node_name.next].node_name.prev =                        \
        (root)[n__].node_name.prev;                                            \
    (root)[n__].node_name.next = (root)[n__].node_name.prev = n__;             \
  } while (0)
```

UNSAFE macro for removing a node from a list. Resets node data so it links to itself.

#### `FIO_INDEXED_LIST_EACH`

```c
#define FIO_INDEXED_LIST_EACH(root, node_name, head, pos)                      \
  for (size_t pos = (head), stopper___ils___ = 0; !stopper___ils___;           \
       stopper___ils___ = ((pos = (root)[pos].node_name.next) == (head)))
```

Loops through every index in the indexed list, **assuming `head` is valid**.

-------------------------------------------------------------------------------
## Logging and Assertions

```c
#define FIO_LOG
#include "fio-stl.h"
```

If the `FIO_LOG_LENGTH_LIMIT` macro is defined (it's recommended that it be greater than 128), than the `FIO_LOG2STDERR` (weak) function and the `FIO_LOG_WRITE` macro will be defined.

**Note:** `FIO_LOG` always uses `libc` functions and cannot be used for authoring apps without `libc` unless `memcpy` and `vsnprintf` are implemented separately (and shadowed by a macro before the module is included).

**Note**: in **all** of the following `msg` **must** be a string literal (`const char *`).

#### `FIO_LOG_LEVEL_GET` and `FIO_LOG_LEVEL_SET`

```c
/** Sets the Logging Level */
#define FIO_LOG_LEVEL_SET(new_level) (0)
/** Returns the Logging Level */
#define FIO_LOG_LEVEL_GET() (0)
```

An application wide integer get/set with a value of either:

- `FIO_LOG_LEVEL_NONE` (0)
- `FIO_LOG_LEVEL_FATAL` (1)
- `FIO_LOG_LEVEL_ERROR` (2)
- `FIO_LOG_LEVEL_WARNING` (3)
- `FIO_LOG_LEVEL_INFO` (4)
- `FIO_LOG_LEVEL_DEBUG` (5)

The initial value can be set using the `FIO_LOG_LEVEL_DEFAULT` macro. By default, the level is 4 (`FIO_LOG_LEVEL_INFO`) for normal compilation and 5 (`FIO_LOG_LEVEL_DEBUG`) for DEBUG compilation.

**Note:** although the integer itself is global and accessible, it shouldn't be used directly (in case `FIO_NO_LOG` is defined).

#### `FIO_LOG2STDERR(msg, ...)`

This `printf` style **function** will log a message to `stderr`, without allocating any memory on the heap for the string (`fprintf` might).

The function is defined as `weak`, allowing it to be overridden during the linking stage, so logging could be diverted... although, it's recommended to divert `stderr` rather then the logging function.

#### `FIO_LOG_WRITE(msg, ...)`

This macro routs to the `FIO_LOG2STDERR` function after prefixing the message with the file name and line number in which the error occurred.

#### `FIO_LOG_FATAL(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_FATAL`.

#### `FIO_LOG_ERROR(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_ERROR`.

#### `FIO_LOG_SECURITY(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_ERROR`.

#### `FIO_LOG_WARNING(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_WARNING`.

#### `FIO_LOG_INFO(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_INFO`.

#### `FIO_LOG_DEBUG(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_DEBUG`.

#### `FIO_LOG_DDEBUG(msg, ...)`

Same as `FIO_LOG_DEBUG` if `DEBUG` was defined. Otherwise a no-op.

#### `FIO_ASSERT(cond, msg, ...)`

Reports an error unless condition is met, printing out `msg` using `FIO_LOG_FATAL` and exiting the application using `SIGINT` followed by an `exit(-1)`.

The use of `SIGINT` should allow debuggers everywhere to pause execution before exiting the program.

#### `FIO_ASSERT_ALLOC(ptr)`

Reports a failure to allocate memory, exiting the program the same way as `FIO_ASSERT`.

#### `FIO_ASSERT_DEBUG(cond, msg, ...)`

Ignored unless `DEBUG` is defined.

Reports an error unless condition is met, printing out `msg` using `FIO_LOG_FATAL` and aborting (not exiting) the application.

Note, this macro will **only** raise a `SIGINT` signal, but will not exit the program. This is designed to allow debuggers to catch these occurrences and continue execution when possible.

#### `FIO_ASSERT_STATIC(cond, msg)`

Performs static assertion test (tested during compile time). Note that `cond` **must** be a constant expression and `msg` cannot be formatted.

-------------------------------------------------------------------------------
## String / Number conversion

```c
#define FIO_ATOL
#include "fio-stl.h"
```

If the `FIO_ATOL` macro is defined, the following functions will be defined:

**Note**: all functions that write to a buffer also write a `NUL` terminator byte.

### Signed Number / String Conversion

The most common use of number to string conversion (and string to number) relates to converting signed numbers. 

However, consider using unsigned conversion where possible.

#### `fio_atol10`

```c
int64_t fio_atol10(char **pstr);
```

Reads a signed base 10 formatted number.

#### `fio_atol`

```c
int64_t fio_atol(char **pstr);
```

A helper function that converts between String data to a signed int64_t.

Numbers are assumed to be in base 10. Octal (`0###`), Hex (`0x##`/`x##`) and binary (`0b##`/ `b##`) are recognized as well. For binary Most Significant Bit must come first.

The most significant difference between this function and `strtol` (aside of API design), is the added support for binary representations.

#### `fio_ltoa`

```c
size_t fio_ltoa(char *dest, int64_t num, uint8_t base);
```

A helper function that writes a signed int64_t to a `NUL` terminated string.

If `dest` is `NULL`, returns the number of bytes that would have been written.

No overflow guard is provided, so either allow for plenty of headroom (at least 65 bytes) or pass `NULL` first and allocate appropriately.

**Note**: special base prefixes for base 2 (binary) and base 16 (hex) are **NOT** added automatically. Consider adding any required prefix when possible (i.e.,`"0x"` for hex and `"0b"` for base 2).

Supports any base up to base 36 (using 0-9,A-Z).

An unsupported base will log an error and print zero.

Returns the number of bytes actually written (excluding the NUL terminator).

#### `fio_atof`

```c
double fio_atof(char **pstr);
```

A helper function that converts between String data to a signed double.

Currently wraps `strtod` with some special case handling.

#### `fio_ftoa`

```c
size_t fio_ftoa(char *dest, double num, uint8_t base);
```

A helper function that converts between a double to a string.

Currently wraps `snprintf` with some special case handling.

No overflow guard is provided, make sure there's at least 130 bytes available
(for base 2).

Supports base 2, base 10 and base 16. An unsupported base will silently default
to base 10. Prefixes aren't added (i.e., no "0x" or "0b" at the beginning of the
string).

Returns the number of bytes actually written (excluding the NUL terminator).

#### `fio_ltoa10`

```c
void fio_ltoa10(char *dest, int64_t i, size_t digits);
```

Writes a signed number to `dest` using `digits` bytes (+ `NUL`). See also [`fio_digits10`](#fio_digits10).

### Unsigned Number / String Conversion

#### `fio_ltoa10`

```c
void fio_ltoa10(char *dest, uint64_t i, size_t digits);
```

Writes a signed number to `dest` using `digits` bytes (+ `NUL`).

#### `fio_ltoa10u`

```c
void fio_ltoa10u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`).

#### `fio_ltoa16u`

```c
void fio_ltoa16u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`) in hex format (base 16).

Note: for hex based numeral facil.io assumes that `digits` are always even (2, 4, 6, 8, 10, 12, 14, 16).

#### `fio_ltoa_bin`

```c
void fio_ltoa_bin(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`) in binary format (base 2).

#### `fio_ltoa_xbase`

```c
void fio_ltoa_xbase(char *dest, uint64_t i, size_t digits, size_t base);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`) in `base` format (up to base 36 inclusive).

#### `fio_atol8u`

```c
uint64_t fio_atol8u(char **pstr);
```

Reads an unsigned base 8 formatted number.

#### `fio_atol10u`

```c
uint64_t fio_atol10u(char **pstr);
```

Reads an unsigned base 10 formatted number.

#### `fio_atol16u`

```c
uint64_t fio_atol16u(char **pstr);
```

Reads an unsigned hex formatted number (possibly prefixed with "0x").

#### `fio_atol_bin`

```c
uint64_t fio_atol_bin(char **pstr);
```

Reads an unsigned binary formatted number (possibly prefixed with "0b").

#### `fio_atol_xbase`

```c
uint64_t fio_atol_xbase(char **pstr, size_t base);
```

Read an unsigned number in any base up to base 36.

### Number / String Conversion Helpers

#### `fio_c2i`

```c
uint8_t fio_c2i(unsigned char c);
```

Maps characters to alphanumerical value, where numbers have their natural values (`0-9`) and `A-Z` (or `a-z`) map to the values `10-35`.

Out of bound values return 255.

This allows calculations for up to base 36.

#### `fio_u2i_limit`

```c
int64_t fio_u2i_limit(uint64_t val, size_t to_negative);
```

Converts an unsigned `val` to a signed `val`, limiting the value to provide overflow protection and limiting it to either a negative or a positive value.

#### `fio_digits10`

```c
size_t fio_digits10(int64_t i);
```

Returns the number of digits of the **signed** number when using base 10. The result includes the possible sign (`-`) digit.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits10u`

```c
size_t fio_digits10u(int64_t i);
```

Returns the number of digits of the **unsigned** number when using base 10.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits8u`

```c
size_t fio_digits8u(int64_t i);
```

Returns the number of digits of the **unsigned** number when using base 8.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits16u`

```c
size_t fio_digits16u(uint64_t i);
```

Returns the number of digits in base 16 for an **unsigned** number.

Base 16 digits are always computed in pairs (byte sized chunks). Possible values are 2,4,6,8,10,12,14 and 16.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

**Note**: facil.io always assumes all base 16 numeral representations are printed as they are represented in memory.

#### `fio_digits_bin`

```c
size_t fio_digits_bin(int64_t i);
```

Returns the number of digits of the **unsigned** number when using base 2.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits_xbase`

```c
size_t fio_digits_xbase(int64_t i);
```

Returns the number of digits of the **unsigned** number when using base `base`.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

-------------------------------------------------------------------------------
## Globe Matching

```c
#define FIO_GLOB_MATCH
#include "fio-stl.h"
```

By defining the macro `FIO_GLOB_MATCH` the following functions are defined:

#### `fio_glob_match`

```c
uint8_t fio_glob_match(fio_str_info_s pat, fio_str_info_s str);
```

This function is a **binary** glob matching helper.

Returns 1 on a match, otherwise returns 0.

The following patterns are recognized:

* `*` - matches any string, including an empty string.
		
	i.e., the following patterns will match against the string `"String"`:

    `"*"`

    `"*String*"`

    `"S*ing"`

* `?` - matches any single **byte** (does NOT support UTF-8 characters).
		
	i.e., the following patterns will match against the string `"String"`:

    `"?tring"`

    `"Strin?"`

    `"St?ing"`

* `[!...]` or `[^...]` - matches any **byte** that is **not** withing the brackets (does **not** support UTF-8 characters).

    Byte ranges are supported using `'-'` (i.e., `[!0-9]`)

	Use the backslash (`\`) to escape the special `]`, `-` and `\` characters when they are part of the list.
	
	i.e., the following patterns will match against the string `"String"`:

    `"[!a-z]tring"`

    `"[^a-z]tring"`

    `"[^F]tring"` (same as `"[!F]tring"`)

* `[...]` - matches any **byte** that **is** withing the brackets (does **not** support UTF-8 characters).

	Use the backslash (`\`) to escape the special `]`, `-` and `\` characters when they are part of the list.
	
	i.e., the following patterns will match against the string `"String"`:

    `"[A-Z]tring"`

    `"[sS]tring"`


-------------------------------------------------------------------------------
## iMap - an Index Mapped Array (Hash Map - Array Combo)

The `FIO_TYPEDEF_IMAP_ARRAY` macro is one way to design a hash map and is **used internally** for some modules (to minimize dependencies or nested inclusions).

It is used when both insertion order and iteration over the complete data set is of high priority, or when it is important to hold the same data as both an Array and a Hash Map.

**Note**: there's no memory management when objects are removed or the iMap is destroyed.

**Note**: for most use cases it is much better to create a type with the `FIO_MAP_NAME` macro.

#### `FIO_TYPEDEF_IMAP_ARRAY`

```c
#define FIO_TYPEDEF_IMAP_ARRAY(array_name,                                     \
                               array_type,                                     \
                               imap_type,                                      \
                               hash_fn,                                        \
                               cmp_fn,                                         \
                               is_valid_fn)
```

This MACRO defines the type and functions needed for an indexed array.

An indexed array is simple ordered array who's objects are indexed using an almost-hash map, allowing for easy seeking while also enjoying the advantages provided by the array structure.

The index map uses one `imap_type` (i.e., `uint64_t`) to store both the index in array and any leftover hash data (the first half being tested during the random access and the leftover during comparison). The reserved value `0` indicates a free slot. The reserved value `~0` indicates a freed item (a free slot that was previously used).

This is mostly for internal use and documentation is poor (PR, anyone?).

The macro defines the following:

- `array_name_s`        the main array container (.ary is the array itself)

- `array_name_seeker_s` is a seeker type that finds objects.
- `array_name_seek`     finds an object or its future position.

- `array_name_reserve`  reserves a minimum imap storage capacity.
- `array_name_capa`     the imap's theoretical storage capacity.

- `array_name_set`      writes or overwrites data to the array.
- `array_name_get`      returns a pointer to the object within the array.
- `array_name_remove`   removes an object and resets its memory to zero.

- `array_name_rehash`   re-builds the imap (use after sorting).


Notes:

- `hash_fn(ptr)`, `cmp_fn(a_ptr,b_ptr)` and `is_valid_fn(ptr)` accepts **pointers**  and needs to de-reference them in order to compare their content.

-------------------------------------------------------------------------------
## Multi-Precision Math

```c
#define FIO_MATH
#include "fio-stl.h"
```

When requiring more than multi-precision ADD, SUB and MUL, `FIO_MATH` attempts to provide some commonly used (yet more advanced) operations.

Note that this implementation assumes that the CPU performs MUL in constant time (which may or may not be true).

### Multi-Precision Math with Little Endian arrays

The following, somewhat naive, multi-precision math implementation focuses on constant time. It assumes an array of local endian 64bit numbers ordered within the array in little endian (word `0` contains the least significant bits and word `n-1` contains the most significant bits).


#### `fio_math_div`
```c
void fio_math_div(uint64_t *dest,
                  uint64_t *reminder,
                  const uint64_t *a,
                  const uint64_t *b,
                  const size_t number_array_length);
```

Multi-precision DIV for `len*64` bit long a, b.

This is **NOT constant time**.

The algorithm might be slow, as my math isn't that good and I couldn't understand faster division algorithms (such as Newton–Raphson division)... so this is sort of a factorized variation on long division.

#### `fio_math_shr`

```c
void fio_math_shr(uint64_t *dest,
                  uint64_t *n,
                  const size_t right_shift_bits,
                  size_t number_array_length);
```

Multi-precision shift right for `len` word number `n`.

#### `fio_math_shl`

```c
void fio_math_shl(uint64_t *dest,
                  uint64_t *n,
                  const size_t left_shift_bits,
                  const size_t number_array_length);
```

Multi-precision shift left for `len*64` bit number `n`.

#### `fio_math_inv`

```c
void fio_math_inv(uint64_t *dest, uint64_t *n, size_t len);
````

Multi-precision Inverse for `len*64` bit number `n` (i.e., turns `1` into `-1`).

#### `fio_math_msb_index`

```c
size_t fio_math_msb_index(uint64_t *n, const size_t len);
````

Multi-precision - returns the index for the most significant bit or -1.

This can be used to collect a number's bit length.

#### `fio_math_lsb_index`

```c
size_t fio_math_lsb_index(uint64_t *n, const size_t len);
````

Multi-precision - returns the index for the least significant bit or -1.

This can be used to extract an exponent value in base 2.

### Vector Math Helpers

Vector helper functions start with the type of the vector. i.e. `fio_u128`. Next the operation name and the bit grouping are stated, i.e. `fio_u256_mul64`.

When a vector operation is applied to a constant, the operation is prefixed with a `c`, i.e., `fio_u128_clrot32`.

`fio_uxxx_load`, `fio_uxxx_load_le32` and `fio_uxxx_load_le64` functions are also provided.

The following functions are available:

* `fio_uxxx_mul##(vec_a, vec_b)` - performs the `mul` operation (`*`).
* `fio_uxxx_add##(vec_a, vec_b)` - performs the `add` operation (`+`).
* `fio_uxxx_sub##(vec_a, vec_b)` - performs the `sub` operation (`-`).
* `fio_uxxx_div##(vec_a, vec_b)` - performs the `div` operation (`/`).
* `fio_uxxx_reminder##(vec_a, vec_b)` - performs the `reminder` operation (`%`).


* `fio_uxxx_cmul##(vec, single_element)` - performs the `mul` operation (`*`).
* `fio_uxxx_cadd##(vec, single_element)` - performs the `add` operation (`+`).
* `fio_uxxx_csub##(vec, single_element)` - performs the `sub` operation (`-`).
* `fio_uxxx_cdiv##(vec, single_element)` - performs the `div` operation (`/`).
* `fio_uxxx_creminder##(vec, single_element)` - performs the `reminder` operation (`%`).

* `fio_uxxx_and(vec_a, vec_b)` - performs the `and` operation (`&`) on the whole vector.
* `fio_uxxx_or(vec_a, vec_b)` - performs the `or` operation (`|`) on the whole vector.
* `fio_uxxx_xor(vec_a, vec_b)` - performs the `xor` operation (`^`) on the whole vector.

* `fio_uxxx_cand##(vec, single_element)` - performs the `and` operation (`&`) with an X bit constant.
* `fio_uxxx_cor##(vec, single_element)` - performs the `or` operation (`|`) with an X bit constant.
* `fio_uxxx_cxor##(vec, single_element)` - performs the `xor` operation (`^`) with an X bit constant.

* `fio_uxxx_flip(vec)` - performs the `flip` bit operation (`~`).

* `fio_uxxx_shuffle##(vec, index0, index1...)` - performs a limited `shuffle` operation on a single vector, reordering its members.

* `fio_uxxx_load(const void * buffer)` loads data from the buffer, returning a properly aligned vector.

* `fio_uxxx_load_le##(const void * buffer)` loads data from the buffer, returning a properly aligned vector. This variation performs a `bswap` operation on each bit group, so the data is loaded using little endian rather than local endian.

* `fio_uxxx_load_be##(const void * buffer)` loads data from the buffer, returning a properly aligned vector. This variation performs a `bswap` operation on each bit group, so the data is loaded using big endian rather than local endian.

Example use:

```c
fio_u256 const prime = {.u64 = {FIO_U64_HASH_PRIME0,
                                FIO_U64_HASH_PRIME1,
                                FIO_U64_HASH_PRIME2,
                                FIO_U64_HASH_PRIME3}};
fio_u256 u = fio_u256_load(buf), tmp;
u = fio_u256_mul64(u, prime);
tmp = fio_u256_clrot64(u, 31);
u = fio_u256_xor64(u, tmp);
// ...
```

**Note**: the implementation is portable and doesn't currently use any compiler vector builtins or types. We pray to the optimization gods instead (which don't always listen) and depend on compilation flags.

#### Numeral Array Shuffling

Numeral vector / array shuffling is available the numeral types `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`, as well as the `float` and `double` types.

Vector / array shuffling is available for any combinations of up to 256 bytes (i.e., `8x256` or `64x32`).

The naming convention is `fio_PRxL_reshuffle` where `PR` is either `u8`, `u16`, `u32`, `u64`, `float` or `dbl` and `L` is the length of the array in number of elements.

**Note**: The use of **re**shuffle denotes that the shuffling occurs in-place, replacing current data (unlike the `fio_uxxx_shuffle##` math functions).

i.e.: 

```c
void fio_u64x4_reshuffle(uint64_t * v, uint8_t[4]);
void fio_u64x8_reshuffle(uint64_t * v, uint8_t[8]);
void fio_u64x16_reshuffle(uint64_t *v, uint8_t[16]);
#define fio_u64x4_reshuffle(v, ...)  fio_u64x4_reshuffle(v,  (uint8_t[4]){__VA_ARGS__})
#define fio_u64x8_reshuffle(v, ...)  fio_u64x8_reshuffle(v,  (uint8_t[8]){__VA_ARGS__})
#define fio_u64x16_reshuffle(v, ...) fio_u64x16_reshuffle(v, (uint8_t[16]){__VA_ARGS__})
```

#### Numeral Array Reduction

Numeral vector / array reduction is available the numeral types `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`, as well as the `float` and `double` types.

Vector / array reduction is available for any combinations of up to 256 bytes (i.e., `8x256` or `64x32`).

The naming convention is `fio_PRxL_reduce_OP` where:

- `PR` is either `u8`, `u16`, `u32`, `u64`, `float` or `dbl`

-  `L` is the length of the array in number of elements.

- `OP` is the operation to be performed, one of: `add`, `mul` `xor`, `or`, `and`. Note that for `float` and `double` types, only `add` and `mul` are available.

i.e.: 

```c
uint64_t fio_u64x4_reduce_add(uint64_t * v);
uint64_t fio_u64x8_reduce_xor(uint64_t * v);
uint64_t fio_u64x16_reduce_and(uint64_t * v);
```

-------------------------------------------------------------------------------
## Pseudo Random Generation

```c
#define FIO_RAND
#include "fio-stl.h"
```

If the `FIO_RAND` macro is defined, the following, non-cryptographic psedo-random generator and hash functions will be defined.

The "random" data is initialized / seeded automatically using a small number of functional cycles that collect data and hash it, hopefully resulting in enough jitter entropy.

The data is collected using `getrusage` (or the system clock if `getrusage` is unavailable) and hashed using RiskyHash. The data is then combined with the previous state / cycle.

The CPU "jitter" within the calculation **should** effect `getrusage` in a way that makes it impossible for an attacker to determine the resulting random state (assuming jitter exists).

However, this is unlikely to prove cryptographically safe and isn't likely to produce a large number of entropy bits (even though a small number of bits have a large impact on the final state).

The facil.io random generator functions appear both faster and more random then the standard `rand` on my computer (you can test it for yours).

I designed it in the hopes of achieving a cryptographically safe PRNG, but it wasn't cryptographically analyzed, lacks a good source of entropy and should be considered as a good enough non-cryptographic PRNG for general use.

**Note**: bitwise operations (`FIO_BITWISE`), Risky Hash and Stable Hash are automatically defined along with `FIO_RAND`, since they are required by the algorithm.

### Psedo-Random Generator Functions

#### `fio_rand64`

```c
uint64_t fio_rand64(void)
```

Returns 64 random bits. Probably **not** cryptographically safe.

#### `fio_rand_bytes`

```c
void fio_rand_bytes(void *data_, size_t len)
```

Writes `len` random Bytes to the buffer pointed to by `data`. Probably **not**
cryptographically safe.

#### `fio_rand_feed2seed`

```c
static void fio_rand_feed2seed(void *buf_, size_t len);
```

An internal function (accessible from the translation unit) that allows a program to feed random data to the PRNG (`fio_rand64`).

The random data will effect the random seed on the next reseeding.

Limited to 1023 bytes of data per function call.

#### `fio_rand_reseed`

```c
void fio_rand_reseed(void);
```

Forces the random generator state to rotate.

SHOULD be called after `fork` to prevent the two processes from outputting the same random numbers (until a reseed is called automatically).

### Risky Hash / Stable Hash (data hashing):

Stable Hash is a stable block hashing algorithm that can be used to hash non-ephemeral data. The hashing speeds are competitively fast, the algorithm is fairly simple with good avalanche dispersion and minimal bias.

Risky Hash is a non-stable hashing algorithm that is aimed at ephemeral data hashing (i.e., hash maps keys) and might be updated periodically to produce different hashing results. It too aims to balance security concerns with all the features 

Both algorithms are **non-cryptographic** and produce 64 bit hashes by default (though internally both use a 256 block that could be used to produce 128bit hashes). Both pass the SMHasher test suite for hashing functions.

#### `fio_stable_hash`

```c
uint64_t fio_stable_hash(const void *data, size_t len, uint64_t seed);
```

Computes a 64 bit facil.io Stable Hash (once version 1.0 is released, this algorithm will not be updated, even if broken).

#### `fio_stable_hash128`

```c
void fio_stable_hash128(void *restrict dest,
                        const void *restrict data,
                        size_t len,
                        uint64_t seed);
```

Computes a 128 bit facil.io Stable Hash (once version 1 is released, this algorithm will not be updated, even if broken).

#### `fio_risky_hash`

```c
uint64_t fio_risky_hash(const void *data, size_t len, uint64_t seed)
```

This is a non-streaming implementation of the RiskyHash v.3 algorithm.

This function will produce a 64 bit hash for X bytes of data.

**Note**: the hashing algorithm may change at any time and the hash value should be considered ephemeral. Meant to be safe enough for use with hash maps.

**Note**: if `FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH` is defined and true, `fio_stable_hash` will be called instead.

#### `fio_risky_ptr`

```c
uint64_t fio_risky_ptr(void *ptr);
```

Adds a bit of entropy to pointer values.

**Note**: the hashing algorithm may change at any time and the hash value should be considered ephemeral. Meant to be safe enough for use with hash maps.

#### `fio_risky_num`

```c
uint64_t fio_risky_num(uint64_t number, uint64_t seed);
```

Adds a bit of entropy to numeral values.

**Note**: the hashing algorithm may change at any time and the hash value should be considered ephemeral. Meant to be safe enough for use with hash maps, but that's about it.

-------------------------------------------------------------------------------
## Signal Monitoring

```c
#define FIO_SIGNAL
#include "fio-stl.h"
```

OS signal callbacks are very limited in the actions they are allowed to take. In fact, one of the only actions they are allowed to take is to set a volatile atomic flag.

The facil.io STL offers helpers that perform this very common pattern of declaring a flag, watching a signal, setting a flag and (later) calling a callback outside of the signal handler that would handle the actual event.

When defining `FIO_SIGNAL`, the following function are defined.

#### `fio_signal_monitor`

```c
typedef struct {
  /** The signal number to listen for. */
  int sig;
  /** The callback to run - leave NULL to ignore signal. */
  void (*callback)(int sig, void *udata);
  /** Opaque user data. */
  void *udata;
  /** Should the signal propagate to existing handler(s)? */
  bool propagate;
  /** Should the callback run immediately (signal safe code) or wait for `fio_signal_review`? */
  bool immediate;
} fio_signal_monitor_args_s;

SFUNC int fio_signal_monitor(fio_signal_monitor_args_s args);
#define fio_signal_monitor(...)                                                \
  fio_signal_monitor((fio_signal_monitor_args_s){__VA_ARGS__})
```

Starts to monitor for the specified signal, setting an optional callback.

If `callback` is `NULL`, the signal will be ignored.

If the signal is already being monitored, the callback and `udata` pointers are updated.

If `propagate` is true and a previous signal handler was set, it will be called.

**Note**: `udata` stands for "user data", it is an opaque pointer that is simply passed along to the callback.

#### `fio_signal_review`

```c
int fio_signal_review(void);
```

Reviews all signals, calling any relevant callbacks.

#### `fio_signal_forget`

```c
int fio_signal_forget(int sig);
```

Stops monitoring the specified signal.

-------------------------------------------------------------------------------
## Quick Sort and Insert Sort

```c
#define FIO_SORT_NAME
#include "fio-stl.h"
```

If the `FIO_SORT_NAME` is defined (and named), the following functions will be defined.

This can be performed multiple times for multiple types.

### Sort Settings

The following macros define the behavior of the sorting algorithm.

#### `FIO_SORT_NAME`

```c
#define FIO_SORT_NAME num // will produce function names such as num_sort(...)
```

The prefix used for naming the sorting functions.

**Note**: if not defined, than `FIO_SORT_NAME` will be defined as `FIO_SORT_TYPE##_vec`.

#### `FIO_SORT_TYPE`

```c
// i.e.
#define FIO_SORT_TYPE size_t
```

The type of the array members to be sorted.

**Note**: this macro **MUST** be defined.

#### `FIO_SORT_IS_BIGGER`

```c
#define FIO_SORT_IS_BIGGER(a, b) ((a) > (b))
```

Equality test - **must** evaluate as 1 if a > b (zero if equal or smaller).

#### `FIO_SORT_SWAP`

```c
#define FIO_SORT_SWAP(a, b)                                                    \
  do {                                                                         \
    FIO_SORT_TYPE tmp__ = (a);                                                 \
    (a) = (b);                                                                 \
    (b) = tmp__;                                                               \
  } while (0)
```

Swaps array members. Usually there is no need to override the default macro.

#### `FIO_SORT_THRESHOLD`

```c
#define FIO_SORT_THRESHOLD 96
```

The threshold below which quick-sort delegates to insert sort. Usually there is no need to override the default macro.


### Sorting API

#### `FIO_SORT_sort`

```c
void FIO_SORT_sort(FIO_SORT_TYPE *array, size_t count);
```

Sorts the first `count` members of `array`.

Currently this wraps the [`FIO_SORT_qsort`](#fio_sort_qsort) function.

#### `FIO_SORT_qsort`

```c
void FIO_SORT_qsort(FIO_SORT_TYPE *array, size_t count);
```

Sorts the first `count` members of `array` using quick-sort.


#### `FIO_SORT_isort`

```c
void FIO_SORT_isort(FIO_SORT_TYPE *array, size_t count);
```

Sorts the first `count` members of `array` using insert-sort.

Use only with small arrays (unless you are a fan of inefficiency).

### Sort Example

The following example code creates an array of random strings and then sorts the array.

```c
#define FIO_STR_SMALL sstr
#define FIO_SORT_NAME      sstr
#define FIO_SORT_TYPE sstr_s
#define FIO_SORT_IS_BIGGER(a, b)                                               \
  fio_string_is_greater(sstr_info(&a), sstr_info(&b))
#define FIO_RAND
#include "fio-stl.h"

#define STRING_ARRAY_LENGTH 128
int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;
  sstr_s ary[STRING_ARRAY_LENGTH] = {{0}};
  /* fill array with random data and print state */
  for (size_t i = 0; i < STRING_ARRAY_LENGTH; ++i) {
    sstr_write_hex(ary + i, fio_rand64());
  }
  printf("Starting with array of strings as:\n");
  for (size_t i = 0; i < STRING_ARRAY_LENGTH; ++i) {
    printf("[%zu] %s\n", i, sstr2ptr(ary + i));
  }
  /* sort array and print state */
  sstr_qsort(ary, STRING_ARRAY_LENGTH);
  printf("\n\nOrdered array of strings is:\n");
  for (size_t i = 0; i < STRING_ARRAY_LENGTH; ++i) {
    printf("[%zu] %s\n", i, sstr2ptr(ary + i));
    sstr_destroy(ary + i); /* cleanup */
  }
  return 0;
}
```

-------------------------------------------------------------------------------
## Threads (portable)

```c
#define FIO_THREADS
#include "fio-stl.h"
```

The facil.io `FIO_THREADS` module provides a simple API for threading that is OS portable between POSIX systems and Windows OS.

The POSIX systems implementation uses `pthreads` under the hood.

Please note that due to thread return value and methodology differences, `FIO_THREADS` do not return any value.

The following methods are provided when the `FIO_THREADS` macro is defined before including the `fio-stl.h` header.

### Process functions

#### `FIO_THREADS_FORK_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these processes forking functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `fio_thread_fork`

```c
fio_thread_pid_t fio_thread_fork(void);
```

Behaves (or should behave) the same as the POSIX system call `fork`.

#### `fio_thread_getpid`

```c
fio_thread_pid_t fio_thread_getpid(void);
```

Behaves (or should behave) the same as the POSIX system call `getpid`.

#### `fio_thread_kill`

```c
int fio_thread_kill(fio_thread_pid_t pid, int sig);
```

Behaves (or should behave) the same as the POSIX system call `kill`.

#### `fio_thread_waitpid`

```c
int fio_thread_waitpid(fio_thread_pid_t pid, int *stat_loc, int options);
```

Behaves (or should behave) the same as the POSIX system call `waitpid`.

### Thread functions

#### `FIO_THREADS_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these thread functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `fio_thread_create`
```c
int fio_thread_create(fio_thread_t *thread, void *(*start_function)(void *), void *arg);
```

Starts a new thread, returns 0 on success and -1 on failure.

#### `fio_thread_join`
```c
int fio_thread_join(fio_thread_t *thread);
```

Waits for the thread to finish.

#### `fio_thread_detach`
```c
int fio_thread_detach(fio_thread_t *thread);
```

Detaches the thread, so thread resources are freed automatically.

#### `fio_thread_exit`
```c
void fio_thread_exit(void);
```

Ends the current running thread.

#### `fio_thread_equal`
```c
int fio_thread_equal(fio_thread_t *a, fio_thread_t *b);
```

Returns non-zero if both threads refer to the same thread.

#### `fio_thread_current`
```c
fio_thread_t fio_thread_current(void);
```

Returns the current thread.

#### `fio_thread_yield`
```c
void fio_thread_yield(void);
```

Yields thread execution.

#### `fio_thread_priority`

```c
fio_thread_priority_e fio_thread_priority(void);
```

Returns the current thread's priority level as a `fio_thread_priority_e` enum.

```c
/** Possible thread priority values. */
typedef enum {
  FIO_THREAD_PRIORITY_ERROR = -1,
  FIO_THREAD_PRIORITY_LOWEST = 0,
  FIO_THREAD_PRIORITY_LOW,
  FIO_THREAD_PRIORITY_NORMAL,
  FIO_THREAD_PRIORITY_HIGH,
  FIO_THREAD_PRIORITY_HIGHEST,
} fio_thread_priority_e;
```

#### `fio_thread_priority_set`

```c
int fio_thread_priority_set(fio_thread_priority_e pr);
```

Sets the current thread's priority level as a `fio_thread_priority_e` enum (see [`fio_thread_priority`](#fio_thread_priority)).

### Mutex functions

#### `FIO_THREADS_MUTEX_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these mutex functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `FIO_THREAD_MUTEX_INIT`

Statically initializes a Mutex.

#### `fio_thread_mutex_init`
```c
int fio_thread_mutex_init(fio_thread_mutex_t *m);
```

Initializes a simple Mutex.

Or use the static initialization value: `FIO_THREAD_MUTEX_INIT`

#### `fio_thread_mutex_lock`
```c
int fio_thread_mutex_lock(fio_thread_mutex_t *m);
```

Locks a simple Mutex, returning -1 on error.

#### `fio_thread_mutex_trylock`
```c
int fio_thread_mutex_trylock(fio_thread_mutex_t *m);
```

Attempts to lock a simple Mutex, returning zero on success.

#### `fio_thread_mutex_unlock`
```c
int fio_thread_mutex_unlock(fio_thread_mutex_t *m);
```

Unlocks a simple Mutex, returning zero on success or -1 on error.

#### `fio_thread_mutex_destroy`
```c
void fio_thread_mutex_destroy(fio_thread_mutex_t *m);
```

Destroys the simple Mutex (cleanup).


### Conditional Variable Functions

#### `FIO_THREADS_COND_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these conditional variable functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `fio_thread_cond_init`

```c
int fio_thread_cond_init(fio_thread_cond_t *c);
```

Initializes a simple conditional variable.

#### `fio_thread_cond_wait`

```c
int fio_thread_cond_wait(fio_thread_cond_t *c, fio_thread_mutex_t *m);
```

Waits on a conditional variable (MUST be previously locked).

#### `fio_thread_cond_signal`

```c
int fio_thread_cond_signal(fio_thread_cond_t *c);
```

Signals a simple conditional variable.

#### `fio_thread_cond_destroy`

```c
void fio_thread_cond_destroy(fio_thread_cond_t *c);
```

Destroys a simple conditional variable.


-------------------------------------------------------------------------------
## URL (URI) parsing

```c
#define FIO_URL
#include "fio-stl.h"
```

URIs (Universal Resource Identifier), commonly referred to as URL (Uniform Resource Locator), are a common way to describe network and file addresses.

A common use case for URIs is within the command line interface (CLI), allowing a client to point at a resource that may be local (i.e., `file:///users/etc/my.conf`) or remote (i.e. `http://example.com/conf`).

By defining `FIO_URL`, the following types and functions will be defined:

#### `fio_url_s`

```c
/** the result returned by `fio_url_parse` */
typedef struct {
  fio_buf_info_s scheme;
  fio_buf_info_s user;
  fio_buf_info_s password;
  fio_buf_info_s host;
  fio_buf_info_s port;
  fio_buf_info_s path;
  fio_buf_info_s query;
  fio_buf_info_s target;
} fio_url_s;
```

The `fio_url_s` contains a information about a URL (or, URI).

When the information is returned from `fio_url_parse`, the strings in the `fio_url_s` (i.e., `url.scheme.buf`) are **not NUL terminated**, since the parser is non-destructive, with zero-copy and zero-allocation.

#### `fio_buf_info_s` - revisited

The `fio_buf_info_s` is used to return information about the parts of the URL's string buffers detailed above. Since the `fio_url_s` does not return NUL terminated strings, this returned data structure is used.

```c
typedef struct fio_buf_info_s {
  /** The buffer's address (may be NULL if no buffer). */
  char *buf;
  /** The buffer's length, if any. */
  size_t len;
} fio_buf_info_s;
```

See [Binary Data Informational Types and Helpers](#binary-data-informational-types-and-helpers) for more details.

#### `fio_url_parse`

```c
fio_url_s fio_url_parse(const char *url, size_t len);
```

Parses the URI returning it's components and their lengths (no decoding performed, **doesn't accept decoded URIs**).

The returned string are **not NUL terminated**, they are merely locations within the original (unmodified) string.

This function attempts to accept many different formats, including any of the following:

* `/complete_path?query#target`

  i.e.: `/index.html?page=1#list`

* `host:port/complete_path?query#target`

  i.e.:
  - `example.com`
  - `example.com:8080`
  - `example.com/index.html`
  - `example.com:8080/index.html`
  - `example.com:8080/index.html?key=val#target`

* `user:password@host:port/path?query#target`

  i.e.: `user:1234@example.com:8080/index.html`

* `username[:password]@host[:port][...]`

  i.e.: `john:1234@example.com`

* `schema://user:password@host:port/path?query#target`

  i.e.: `http://example.com/index.html?page=1#list`

* `file://some/path`

  i.e.: `file://./relative/path`
  i.e.: `file:///absolute/path`

Invalid formats might produce unexpected results. No error testing performed.

The `file`, `unix` and `priv` schemas are special in the sense that they produce no `host` (only `path`) and are parsed as if they contain file path information.


#### `fio_url_is_tls`

```c
fio_url_tls_info_s fio_url_is_tls(fio_url_s u);
/* Return type for  `fio_url_is_tls` */
typedef struct {
  fio_buf_info_s key;
  fio_buf_info_s cert;
  fio_buf_info_s pass;
  bool tls; /* true if TLS is requested by according to the URL */
} fio_url_tls_info_s;
```

Returns TLS data associated with the URL.

This function supports implicit TLS by scheme data for the following possible values:

- `wss`   - Secure WebSockets.
- `sses`  - Secure SSE (Server Sent Events).
- `https` - Secure HTTP.
- `tcps`  - Secure TCP/IP.
- `tls`   - Secure TCP/IP.
- `udps`  - Secure UDP.

i.e.:

- "tls://example.com/"
- "tcps://example.com/"
- "udps://example.com/"
- "wss://example.com/"
- "https://example.com/"
- "sses://example.com/"

This function also supports explicit TLS by query data for the following possible key-pair values:

- `tls`                   - self-signed TLS (unless key / cert are provided).
- `tls=true`              - self-signed TLS (unless key / cert are provided).
- `tls=<file>`            - key and certificate files (same path / prefix, one ends with "key.pem" the other with "cert.pem").
- `key=<file/env_data>`   - path or env variable name for the private key.
- `cert=<file/env_data>`  - path or env variable name for the public certificate.
- `pass`                  - password for decrypting key data.

i.e.:

- "tcp://example.com/?tls"           (anonymous TLS)
- "udp://example.com/?tls=true"      (anonymous TLS)
- "https://example.com/?tls=key_cert_folder_or_prefix&pass=key_password"
- "https://example.com/?key=key_file_or_env_var&cert=cert_file_or_env_var&pass=key_password"
- "wss://example.com/?key=key_file_or_env_var&cert=cert_file_or_env_var&pass=key_password"
- "tcp://example.com/?key=key_file_or_env_var&cert=cert_file_or_env_var&pass=key_password"

#### `FIO_URL_QUERY_EACH`

```c
/** The type used by the `FIO_URL_QUERY_EACH` iterator macro. */
typedef struct {
  fio_buf_info_s name;
  fio_buf_info_s value;
  fio_buf_info_s private___;
} fio_url_query_each_s;

/** A helper function for the `FIO_URL_QUERY_EACH` macro implementation. */
FIO_SFUNC fio_url_query_each_s fio_url_query_each_next(fio_url_query_each_s);

/** Iterates through each of the query elements. */
#define FIO_URL_QUERY_EACH(query_buf, i)                                       \
  for (fio_url_query_each_s i = fio_url_query_each_next(                       \
           (fio_url_query_each_s){.private___ = (query_buf)});                 \
       i.name.buf;                                                             \
       i = fio_url_query_each_next(i))
```

The macro accepts a `fio_buf_info_s` argument (`query_buf`) and iterates over each `name` and `value` pair in the query buffer.

**Note**: both `i.name` and `i.value` may be empty strings, with a valid `.buf` but with `.len` set to zero.

**Note**: the iterator does not unescape URL escaped data. unescaping may be required before either `i.name` or `i.value` can be used.

-------------------------------------------------------------------------------
## File Utility Helpers

```c
#define FIO_FILES
#include "fio-stl.h"
```

By defining the macro `FIO_FILES` the following file helper functions are defined:

#### `fio_filename_open`

```c
int fio_filename_open(const char *filename, int flags);
```

Opens `filename`, returning the same as values as `open` on POSIX systems.

If `path` starts with a `"~/"` than it will be relative to the user's Home folder (on Windows, testing also for `"~\"`).

#### `fio_filename_is_unsafe`

```c
int fio_filename_is_unsafe(const char *path);
```

Returns 1 if `path` possibly folds backwards (has "/../" or "//").

#### `fio_filename_tmp`

```c
int fio_filename_tmp(void);
```

Creates a temporary file, returning its file descriptor.

Returns -1 on error.

#### `fio_file_dup`

```c
#if FIO_OS_WIN
#define fio_file_dup(fd) _dup(fd)
#else
#define fio_file_dup(fd) dup(fd)
#endif
```

Duplicates the file handle (int).

#### `fio_filename_overwrite`

```c
int fio_filename_overwrite(const char *filename, const void *buf, size_t len);
```

Overwrites `filename` with the data in the buffer.

If `path` starts with a `"~/"` than it will be relative to the user's home folder (on Windows, testing also for `"~\"`).

Returns -1 on error or 0 on success. On error, the state of the file is undefined (may be doesn't exit / nothing written / partially written).

#### `fio_fd_write`

```c
ssize_t fio_fd_write(int fd, const void *buf, size_t len);
```

Writes data to a file, returning the number of bytes written.

Returns -1 on error.

Since some systems have a limit on the number of bytes that can be written at a single time, this function fragments the system calls into smaller `write` blocks, allowing large data to be written.

If the file descriptor is non-blocking, test `errno` for `EAGAIN` / `EWOULDBLOCK`.

#### `fio_fd_read`

```c
size_t fio_fd_read(int fd, void *buf, size_t len, off_t start_at);
```


Reads up to `len` bytes from `fd` starting at `start_at` offset.

Returns the number of bytes read.

Since some systems have a limit on the number of bytes that can be read at a time, this function fragments the system calls into smaller `read` blocks, allowing larger data blocks to be read.

If the file descriptor is non-blocking, test `errno` for `EAGAIN` / `EWOULDBLOCK`.

**Note**: may (or may not) change the file's pointer (reading/writing position), depending on the OS.

#### `fio_filename_parse`

```c
fio_filename_s fio_filename_parse(const char *filename);
/** A result type for the filename parsing helper. */
typedef struct {
  fio_buf_info_s folder;   /* folder name */
  fio_buf_info_s basename; /* base file name */
  fio_buf_info_s ext;      /* extension (without '.') */
} fio_filename_s;
```

Parses a file name to folder, base name and extension (zero-copy).

#### `fio_filename_parse2`

```c
fio_filename_s fio_filename_parse2(const char *filename, size_t len);
```

Same as [`fio_filename_parse`](#fio_filename_parse), only limited to `len` characters - use in cases where the `filename` string might not end with a `NUL` character.

#### `FIO_FOLDER_SEPARATOR`

```c
#if FIO_OS_WIN
#define FIO_FOLDER_SEPARATOR '\\'
#else
#define FIO_FOLDER_SEPARATOR '/'
#endif
```

Selects the folder separation character according to the detected OS.

**Note**: on windows both separators will be tested for.

#### `fio_fd_find_next`

```c
size_t fio_fd_find_next(int fd, char token, size_t start_at);
/** End of file value for `fio_fd_find_next` */
#define FIO_FD_FIND_EOF ((size_t)-1)
/** Size on the stack used by `fio_fd_find_next` for each read cycle. */
#define FIO_FD_FIND_BLOCK 4096
```

Returns offset for the next `token` in `fd`, or -1 if reached  EOF.

This will use `FIO_FD_FIND_BLOCK` bytes on the stack to read the file in a loop.

**Pros**: limits memory use and (re)allocations, easier overflow protection.

**Cons**: may be slower, as data will most likely be copied again from the file.

-------------------------------------------------------------------------------
## Custom JSON Parser

```c
#define FIO_JSON
#include "fio-stl.h"
```

The facil.io JSON parser is a non-strict parser, with support for trailing commas in collections, new-lines in strings, extended escape characters, comments, and common numeral formats (octal, hex and binary).

The facil.io JSON parser should be considered **unsafe** as overflow protection depends on the `NUL` character appearing at the end of the string passed to the parser.

**Note:** this module depends on the `FIO_ATOL` module which will be automatically included.

#### `FIO_JSON_MAX_DEPTH`

```c
#ifndef FIO_JSON_MAX_DEPTH
/** Maximum allowed JSON nesting level. Values above 64K might fail. */
#define FIO_JSON_MAX_DEPTH 512
#endif
```

To ensure the program's stack isn't abused, the parser will limit JSON nesting levels to a customizable `FIO_JSON_MAX_DEPTH` number of nesting levels.

### JSON parser API

#### `fio_json_parser_callbacks_s`

```c
typedef struct {
  /** NULL object was detected. Returns new object as `void *`. */
  void *(*get_null)(void);
  /** TRUE object was detected. Returns new object as `void *`. */
  void *(*get_true)(void);
  /** FALSE object was detected. Returns new object as `void *`. */
  void *(*get_false)(void);
  /** Number was detected (long long). Returns new object as `void *`. */
  void *(*get_number)(int64_t i);
  /** Float was detected (double).Returns new object as `void *`.  */
  void *(*get_float)(double f);
  /** String was detected (int / float). update `pos` to point at ending */
  void *(*get_string)(const void *start, size_t len);
  /** Dictionary was detected. Returns ctx to hash map or NULL on error. */
  void *(*get_map)(void *ctx, void *at);
  /** Array was detected. Returns ctx to array or NULL on error. */
  void *(*get_array)(void *ctx, void *at);
  /** Array was detected. Returns non-zero on error. */
  int (*map_push)(void *ctx, void *key, void *value);
  /** Array was detected. Returns non-zero on error. */
  int (*array_push)(void *ctx, void *value);
  /** Called for the `key` element in case of error or NULL value. */
  void (*free_unused_object)(void *ctx);
  /** the JSON parsing encountered an error - what to do with ctx? */
  void *(*on_error)(void *ctx);
} fio_json_parser_callbacks_s;
```

The JSON parser requires certain callbacks to create objects or perform actions based on JSON data.

The following callbacks MUST be provided to the parser:

 - `void *(*get_null)(void)` - `NULL` object was detected. Returns NULL object as `void *`.

 - `void *(*get_true)(void)` - `true` object was detected. Returns TRUE object as `void *`.

 - `void *(*get_false)(void)` - `false` object was detected. Returns FALSE object as `void *`.

 - `void *(*get_number)(int64_t i)` - Number was detected (`int64_t`). Returns new number object as `void *`.

 - `void *(*get_float)(double f)` - Float was detected (`double`). Returns new float object as `void *`. 

 - `void *(*get_string)(const void *start, size_t len)` - String was detected. `start` points to a JSON escaped String (remember to unescape). Returns a new String as `void *`.

 - `void *(*get_map)(void *ctx, void *at)` - Dictionary was detected. Returns new `ctx` to hash map or `NULL` on error.

 - `void *(*get_array)(void *ctx, void *at)` - Array was detected. Returns new `ctx` to array or `NULL` on error.

 - `int (*map_push)(void *ctx, void *key, void *value)` - Pushes data to Array. Returns non-zero on error.

 - `int (*array_push)(void *ctx, void *value)` - Pushes data to Dictionary. Returns non-zero on error.

 - `void (*free_unused_object)(void *ctx)` - Called for the `key` element in case of error that caused `key` to be unused.

 - `void *(*on_error)(void *ctx)` - the JSON parsing encountered an error - what to do with ctx?


#### `fio_json_parse`

```c
/** The JSON return type. */
typedef struct {
  void *ctx;
  size_t stop_pos;
  int err;
} fio_json_result_s;

fio_json_result_s fio_json_parse(fio_json_parser_callbacks_s *callbacks,
                                 const char *json_string,
                                 const size_t len);
```


The facil.io JSON parser is a non-strict parser, with support for trailing commas in collections, new-lines in strings, extended escape characters and octal, hex and binary numbers.

The parser allows for streaming data and decouples the parsing process from the resulting data-structure by calling the requested callbacks for JSON related events.

Returns the result object which details the number of bytes consumed (stop position index `stop_pos`), if the parsing stopped due to an error (`err`) and the top most context / object in the JSON stream.

#### `fio_json_parse_update`

```c
fio_json_result_s fio_json_parse_update(fio_json_parser_callbacks_s *s,
                                        void *ctx,
                                        const char *start,
                                        const size_t len);
```

Use only when `ctx` is an object and JSON data is wrapped in an object (of the same type).

i.e., update an array or hash map.

### JSON Parsing Example - a JSON minifier

The biggest question about parsing JSON is - where do we store the resulting data?

Different parsers solve this question in different ways.

The `FIOBJ` soft-typed object system offers a very effective solution for data manipulation, as it creates a separate object for every JSON element.

However, many parsers store the result in an internal data structure that can't be separated into different elements. These parser appear faster while actually deferring a lot of the heavy lifting to a later stage.

Here is a short example that parses the data and writes it to a new minifed (compact) JSON String result.

```c
#define FIO_JSON
#define FIO_STR_NAME fio_str
#define FIO_LOG
#include "fio-stl.h"

#define FIO_CLI
#include "fio-stl.h"

typedef struct {
  fio_json_parser_s p;
  fio_str_s out;
  uint8_t counter;
  uint8_t done;
} my_json_parser_s;

#define JSON_PARSER_CAST(ptr) FIO_PTR_FROM_FIELD(my_json_parser_s, p, ptr)
#define JSON_PARSER2OUTPUT(p) (&JSON_PARSER_CAST(p)->out)

FIO_IFUNC void my_json_write_seperator(fio_json_parser_s *p) {
  my_json_parser_s *j = JSON_PARSER_CAST(p);
  if (j->counter) {
    switch (fio_json_parser_is_in_object(p)) {
    case 0: /* array */
      if (fio_json_parser_is_in_array(p))
        fio_str_write(&j->out, ",", 1);
      break;
    case 1: /* object */
      // note the reverse `if` statement due to operation ordering
      fio_str_write(&j->out, (fio_json_parser_is_key(p) ? "," : ":"), 1);
      break;
    }
  }
  j->counter |= 1;
}

/** a NULL object was detected */
FIO_JSON_CB void fio_json_on_null(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "null", 4);
}
/** a TRUE object was detected */
static inline void fio_json_on_true(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "true", 4);
}
/** a FALSE object was detected */
FIO_JSON_CB void fio_json_on_false(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "false", 4);
}
/** a Number was detected (long long). */
FIO_JSON_CB void fio_json_on_number(fio_json_parser_s *p, long long i) {
  my_json_write_seperator(p);
  fio_str_write_i(JSON_PARSER2OUTPUT(p), i);
}
/** a Float was detected (double). */
FIO_JSON_CB void fio_json_on_float(fio_json_parser_s *p, double f) {
  my_json_write_seperator(p);
  char buffer[256];
  size_t len = fio_ftoa(buffer, f, 10);
  fio_str_write(JSON_PARSER2OUTPUT(p), buffer, len);
}
/** a String was detected (int / float). update `pos` to point at ending */
FIO_JSON_CB void
fio_json_on_string(fio_json_parser_s *p, const void *start, size_t len) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "\"", 1);
  fio_str_write(JSON_PARSER2OUTPUT(p), start, len);
  fio_str_write(JSON_PARSER2OUTPUT(p), "\"", 1);
}
/** a dictionary object was detected, should return 0 unless error occurred. */
FIO_JSON_CB int fio_json_on_start_object(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "{", 1);
  JSON_PARSER_CAST(p)->counter = 0;
  return 0;
}
/** a dictionary object closure detected */
FIO_JSON_CB void fio_json_on_end_object(fio_json_parser_s *p) {
  fio_str_write(JSON_PARSER2OUTPUT(p), "}", 1);
  JSON_PARSER_CAST(p)->counter = 1;
}
/** an array object was detected, should return 0 unless error occurred. */
FIO_JSON_CB int fio_json_on_start_array(fio_json_parser_s *p) {
  my_json_write_seperator(p);
  fio_str_write(JSON_PARSER2OUTPUT(p), "[", 1);
  JSON_PARSER_CAST(p)->counter = 0;
  return 0;
}
/** an array closure was detected */
FIO_JSON_CB void fio_json_on_end_array(fio_json_parser_s *p) {
  fio_str_write(JSON_PARSER2OUTPUT(p), "]", 1);
  JSON_PARSER_CAST(p)->counter = 1;
}
/** the JSON parsing is complete */
FIO_JSON_CB void fio_json_on_json(fio_json_parser_s *p) {
  JSON_PARSER_CAST(p)->done = 1;
  (void)p;
}
/** the JSON parsing encountered an error */
FIO_JSON_CB void fio_json_on_error(fio_json_parser_s *p) {
  fio_str_write(
      JSON_PARSER2OUTPUT(p), "--- ERROR, invalid JSON after this point.\0", 42);
}

void run_my_json_minifier(char *json, size_t len) {
  my_json_parser_s p = {{0}};
  fio_json_parse(&p.p, json, len);
  if (!p.done)
    FIO_LOG_WARNING(
        "JSON parsing was incomplete, minification output is partial");
  fprintf(stderr, "%s\n", fio_str2ptr(&p.out));
  fio_str_destroy(&p.out);
}
```

-------------------------------------------------------------------------------
## Basic Socket / IO Helpers

```c
#define FIO_SOCK
#include "fio-stl.h"
```

The facil.io standard library provides a few simple IO / Sockets helpers for POSIX systems.

By defining `FIO_SOCK`, the following functions will be defined.

**Note**:

On Windows that `fd` is a 64 bit number with no promises made as to its value. On POSIX systems the `fd` is a 32 bit number which is sequential. 

Since facil.io prefers the POSIX approach, it will validate the `fd` value for overflow and might fail to open / accept sockets when their value overflows the 32bit type limit set on POSIX machines.

However, for most implementations this should be a non-issue as it seems (from observation, not knowledge) that Windows maps `fd` values to a kernel array (rather than a process specific array) and it is unlikely that any Windows machine will actually open more than 2 Giga "handles" unless it's doing something wrong.

**Note:** this module depends on the `FIO_URL` module which will be automatically included.

#### `fio_sock_open`

```c
int fio_sock_open(const char *restrict address,
                 const char *restrict port,
                 uint16_t flags);
```

Creates a new socket according to the provided flags.

The `port` string will be ignored when `FIO_SOCK_UNIX` is set.

The `address` can be NULL for Server sockets (`FIO_SOCK_SERVER`) when binding to all available interfaces (this is actually recommended unless network filtering is desired).

The `flag` integer can be a combination of any of the following flags:

*  `FIO_SOCK_TCP` - Creates a TCP/IP socket.

*  `FIO_SOCK_UDP` - Creates a UDP socket.

*  `FIO_SOCK_UNIX` - Creates a Unix socket (requires a POSIX system). If an existing file / Unix socket exists, they will be deleted and replaced.

*  `FIO_SOCK_UNIX_PRIVATE` - Same as `FIO_SOCK_UNIX`, only does not use `umask` and `chmod` to make the socket publicly available.

*  `FIO_SOCK_SERVER` - Initializes a Server socket. For TCP/IP and Unix sockets, the new socket will be listening for incoming connections (`listen` will be automatically called).

*  `FIO_SOCK_CLIENT` - Initializes a Client socket, calling `connect` using the `address` and `port` arguments.

*  `FIO_SOCK_NONBLOCK` - Sets the new socket to non-blocking mode.

If neither `FIO_SOCK_SERVER` nor `FIO_SOCK_CLIENT` are specified, the function will default to a server socket.

**Note**:

UDP Server Sockets might need to handle traffic from multiple clients, which could require a significantly larger OS buffer then the default buffer offered.

Consider (from [this SO answer](https://stackoverflow.com/questions/2090850/specifying-udp-receive-buffer-size-at-runtime-in-linux/2090902#2090902), see [this blog post](https://medium.com/@CameronSparr/increase-os-udp-buffers-to-improve-performance-51d167bb1360), [this article](http://fasterdata.es.net/network-tuning/udp-tuning/) and [this article](https://access.redhat.com/documentation/en-US/JBoss_Enterprise_Web_Platform/5/html/Administration_And_Configuration_Guide/jgroups-perf-udpbuffer.html)):

```c
int n = 32*1024*1024; /* try for 32Mb */
while (n >= (4*1024*1024) && setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1) {
  /* failed - repeat attempt at 1Mb interval */
  if (n >= (4 * 1024 * 1024)) // OS may have returned max value
    n -= 1024 * 1024;

}
```

#### `fio_sock_open2`

```c
int fio_sock_open(const char *url, uint16_t flags);
```

See [`fio_sock_open`](#fio_sock_open) for details. Accepts a single, URL style string instead of an address / port pair.

The `tcp` / `udp` information **may** appear in the URL schema if missing from the flags (i.e., `tcp://localhost:3000/`);

If a Unix socket URL is detected on a POSIX system, a `FIO_SOCK_UNIX` socket flag will override any `FIO_SOCK_TCP` or 
`FIO_SOCK_UDP` that were originally given.

**Note**: a `file://` or `unix://` (or even a simple `./file.sock`) URL will create a publicly available Unix Socket (permissions set to allow everyone RW access). To create a private Unix Socket (one with permissions equal to the processes `umask`), use a `prive://` schema (i.e., `priv://my.sock`).

#### `fio_sock_write`, `fio_sock_read`, `fio_sock_close`

```c
#define fio_sock_write(fd, data, len) write((fd), (data), (len))
#define fio_sock_read(fd, buf, len)   read((fd), (buf), (len))
#define fio_sock_close(fd)            close(fd)
/* on Windows only */
#define accept fio_sock_accept
```

Behaves the same as the POSIX function calls... however, on Windows these will be function wrappers around the WinSock2 API variants. It is better to use these macros / functions for portability.

#### `fio_sock_wait_io`

```c
short fio_sock_wait_io(int fd, short events, int timeout)
```

Uses `poll` to wait until an IO device has one or more of the evens listed in `events` (`POLLIN | POLLOUT`) or `timeout` (in milliseconds) have passed.

Returns 0 on timeout, -1 on error or the events that are valid.

Possible valid return values also include `POLLIN | POLLOUT | POLLHUP | POLLNVAL`

#### `FIO_SOCK_WAIT_RW`

```c
#define FIO_SOCK_WAIT_RW(fd, timeout_) fio_sock_wait_io(fd, POLLIN | POLLOUT, timeout_)
```

A helper macro that waits on a single IO with no callbacks (0 = no event)

#### `FIO_SOCK_WAIT_R`

```c
#define FIO_SOCK_WAIT_R(fd, timeout_) fio_sock_wait_io(fd, POLLIN, timeout_)
```

A helper macro that waits on a single IO with no callbacks (0 = no event)

#### `FIO_SOCK_WAIT_W`

```c
#define FIO_SOCK_WAIT_W(fd, timeout_) fio_sock_wait_io(fd, POLLOUT, timeout_)
```

A helper macro that waits on a single IO with no callbacks (0 = no event)

#### `FIO_SOCK_IS_OPEN`

```c
#define FIO_SOCK_IS_OPEN(fd)                                                   \
  (!(fio_sock_wait_io(fd, POLLOUT, 0) & (POLLHUP | POLLNVAL)))
```

A helper macro that tests if a socket was remotely closed.

#### `fio_sock_address_new`

```c
struct addrinfo *fio_sock_address_new(const char *restrict address,
                                      const char *restrict port,
                                      int sock_type);
```

Attempts to resolve an address to a valid IP6 / IP4 address pointer.

The `sock_type` element should be a socket type, such as `SOCK_DGRAM` (UDP) or `SOCK_STREAM` (TCP/IP).

The address should be freed using `fio_sock_address_free`.

#### `fio_sock_address_free`

```c
void fio_sock_address_free(struct addrinfo *a);
```

Frees the pointer returned by `fio_sock_address_new`.

#### `fio_sock_peer_addr`

```c
fio_buf_info_s fio_sock_peer_addr(int s);
```

Returns a human readable address representation of the socket's peer address.

On error, returns a NULL buffer with zero length.

Buffer lengths are limited to 63 bytes.

This function is limited in its thread safety to 128 threads / calls.



#### `fio_sock_set_non_block`

```c
int fio_sock_set_non_block(int fd);
```

Sets a file descriptor / socket to non blocking state.

#### `fio_sock_open_local`

```c
int fio_sock_open_local(struct addrinfo *addr);
```

Creates a new network socket and binds it to a local address.

#### `fio_sock_open_remote`

```c
int fio_sock_open_remote(struct addrinfo *addr, int nonblock);
```

Creates a new network socket and connects it to a remote address.

#### `fio_sock_open_unix`

```c
int fio_sock_open_unix(const char *address, int is_client, int nonblock);
```

Creates a new Unix socket and binds it to a local address.

**Note**: not available on all systems. On Windows, when Unix Sockets are available (which isn't always), the permissions for the socket are system defined (facil.io doesn't change them).


#### `fio_sock_maximize_limits`

```c
size_t fio_sock_maximize_limits(size_t max_limit)
```

Attempts to maximize the allowed open file limits (with values up to `max_limit`). Returns the new known limit.

#### `FIO_SOCK_AVOID_UMASK`

This compilation flag, if defined before including the `FIO_SOCK` implementation, will avoid using `umask` (only using `chmod`).

Using `umask` in multi-threaded environments could cause `umask` data corruption due to race condition (as two calls are actually required, making the operation non-atomic).

If more than one thread is expected to create Unix sockets or call `umask` at the same time, it is recommended that the `FIO_SOCK_AVOID_UMASK` be used.

This, however, may effect permissions on some systems (i.e., some Linux distributions) where calling `chmod` on a Unix socket file doesn't properly update access permissions.

**Note**: on Windows facil.io behaves as if this flag was set.

-------------------------------------------------------------------------------
## State Callbacks

The state callback API, which is also used internally by stateful modules such as the memory allocator, allows callbacks to be registered for specific changes in the state of the app.

This allows modules to react to changes in the state of the program without requiring the functions that caused the change in state to know about each of the modules that wish to react, only requiting it to publish a notification by calling `fio_state_callback_force`.

When using this module it is better if it is used as a global `FIO_EXTERN` module, so state notifications are not limited to the scope of the C file (the translation unit).

By defining the `FIO_STATE` macro, the following are defined:

**Note:** this module depends on the `FIO_RAND`, `FIO_ATOMIC`, and `FIO_IMAP_CORE` modules which will be automatically included.

#### `fio_state_callback_add`

```c
void fio_state_callback_add(fio_state_event_type_e event,
                            void (*func)(void *),
                            void *arg);
```

Adds a callback to the list of callbacks to be called for the `event`.

The callback should accept a single `void *` as an argument.

Events are performed either in the order in which they were registered or in reverse order, depending on the context.

These are the possible `event` values, note that some of them are only relevant in the context of the `FIO_SERVER` module and were designed for the server's use:

```c
typedef enum {
  /** Called once during library initialization. */
  FIO_CALL_ON_INITIALIZE,
  /** Called once before starting up the IO reactor. */
  FIO_CALL_PRE_START,
  /** Called before each time the IO reactor forks a new worker. */
  FIO_CALL_BEFORE_FORK,
  /** Called after each fork (both parent and child), before FIO_CALL_IN_XXX */
  FIO_CALL_AFTER_FORK,
  /** Called by a worker process right after forking. */
  FIO_CALL_IN_CHILD,
  /** Called by the master process after spawning a worker (after forking). */
  FIO_CALL_IN_MASTER,
  /** Called by each worker thread in a Server Async queue as it starts. */
  FIO_CALL_ON_WORKER_THREAD_START,
  /** Called every time a *Worker* process starts. */
  FIO_CALL_ON_START,
  /** Reserved for internal use. */
  FIO_CALL_RESERVED1,
  /** Reserved for internal use. */
  FIO_CALL_RESERVED2,
  /** User state event queue (unused, available for the user). */
  FIO_CALL_ON_USER1,
  /** User state event queue (unused, available for the user). */
  FIO_CALL_ON_USER2,
  /** Called when facil.io enters idling mode. */
  FIO_CALL_ON_IDLE,

  /* the following events are performed in reverse (LIFO): */

  /** A reversed user state event queue (unused, available for the user). */
  FIO_CALL_ON_USER1_REVERSE,
  /** A reversed user state event queue (unused, available for the user). */
  FIO_CALL_ON_USER2_REVERSE,
  /** Reserved for internal use. */
  FIO_CALL_RESERVED1_REVERSED,
  /** Reserved for internal use. */
  FIO_CALL_RESERVED2_REVERSED,
  /** Called before starting the shutdown sequence. */
  FIO_CALL_ON_SHUTDOWN,
  /** Called by each worker the moment it detects the master process crashed. */
  FIO_CALL_ON_PARENT_CRUSH,
  /** Called by the parent (master) after a worker process crashed. */
  FIO_CALL_ON_CHILD_CRUSH,
  /** Called by each worker thread in a Server Async queue as it ends. */
  FIO_CALL_ON_WORKER_THREAD_END,
  /** Called when wither a *Worker* or *Master* stopped. */
  FIO_CALL_ON_STOP,
  /** An alternative to the system's at_exit. */
  FIO_CALL_AT_EXIT,
  /** used for testing and array allocation - must be last. */
  FIO_CALL_NEVER
} fio_state_event_type_e;

```

#### `fio_state_callback_remove`

```c
int fio_state_callback_remove(fio_state_event_type_e,
                              void (*func)(void *),
                              void *arg);
```

Removes a callback from the list of callbacks to be called for the event.

See also [`fio_state_callback_add`](#fio_state_callback_add) for details of possible events.

#### `fio_state_callback_force`

```c
void fio_state_callback_force(fio_state_event_type_e);
```

Forces all the existing callbacks to run, as if the event occurred.

Callbacks for all initialization / idling tasks are called in order of creation (where `fio_state_event_type_e` <= `FIO_CALL_ON_IDLE`).

Callbacks for all cleanup oriented tasks are called in reverse order of creation (where `fio_state_event_type_e` >= `FIO_CALL_ON_USER1_REVERSE`).

During an event, changes to the callback list are ignored (callbacks can't add or remove other callbacks for the same event).

-------------------------------------------------------------------------------
## Time Helpers

```c
#define FIO_TIME
#include "fio-stl.h"
```

By defining `FIO_TIME` or `FIO_QUEUE`, the following time related helpers functions are defined:

**Note:** this module depends on the `FIO_ATOL` module which will be automatically included.

#### `fio_time_real`

```c
struct timespec fio_time_real();
```

Returns human (watch) time... this value isn't as safe for measurements.

#### `fio_time_mono`

```c
struct timespec fio_time_mono();
```

Returns monotonic time.

#### `fio_time_nano`

```c
uint64_t fio_time_nano();
```

Returns monotonic time in nano-seconds (now in 1 micro of a second).

#### `fio_time_micro`

```c
uint64_t fio_time_micro();
```

Returns monotonic time in micro-seconds (now in 1 millionth of a second).

#### `fio_time_milli`

```c
uint64_t fio_time_milli();
```

Returns monotonic time in milliseconds.


#### `fio_time2milli`

```c
uint64_t fio_time2milli(struct timespec t);
```

Converts a `struct timespec` to milliseconds.

#### `fio_time2gm`

```c
struct tm fio_time2gm(time_t timer);
```

A faster (yet less localized) alternative to `gmtime_r`.

See the libc `gmtime_r` documentation for details.

Returns a `struct tm` object filled with the date information.

This function is used internally for the formatting functions: , `fio_time2rfc7231`, `fio_time2rfc2109`, and `fio_time2rfc2822`.

#### `fio_gm2time`

```c
time_t fio_gm2time(struct tm tm)
```

Converts a `struct tm` to time in seconds (assuming UTC).

This function is less localized then the `mktime` / `timegm` library functions.

#### `fio_time2rfc7231`

```c
size_t fio_time2rfc7231(char *target, time_t time);
```

Writes an RFC 7231 date representation (HTTP date format) to target.

Requires 29 characters (for positive, 4 digit years).

The format is similar to DDD, dd, MON, YYYY, HH:MM:SS GMT

i.e.: Sun, 06 Nov 1994 08:49:37 GMT

#### `fio_time2rfc2109`

```c
size_t fio_time2rfc2109(char *target, time_t time);
```

Writes an RFC 2109 date representation to target (HTTP Cookie Format).

Requires 31 characters (for positive, 4 digit years).

#### `fio_time2rfc2822`

```c
size_t fio_time2rfc2822(char *target, time_t time);
```

Writes an RFC 2822 date representation to target.

Requires 28 or 29 characters (for positive, 4 digit years).

#### `fio_time2log`

```c
size_t fio_time2log(char *target, time_t time);
```

Writes a date representation to target in common log format. i.e.: `[DD/MMM/yyyy:hh:mm:ss +0000]`

Usually requires 29 characters (including square brackets and NUL).

#### `fio_time2iso`

```c
size_t fio_time2iso(char *target, time_t time);
```

Writes a date representation to target in ISO 8601 format. i.e.: `YYYY-MM-DD HH:MM:SS`

Usually requires 20 characters (including NUL).

-------------------------------------------------------------------------------
## CLI (command line interface)

```c
#define FIO_CLI
#include "fio-stl.h"
```

The facil.io library includes a CLI parser that provides a simpler API and few more features than the array iteration based `getopt`, such as:

* Auto-generation of the "help" / usage output.

* Argument type testing (String, boolean, and integer types are supported).

* Global Hash map storage and access to the parsed argument values (until `fio_cli_end` is called).

* Support for unnamed options / arguments, including adjustable limits on how many a user may input.

* Array style support and access to unnamed arguments.

By defining `FIO_CLI`, the following functions will be defined.

In addition, `FIO_CLI` automatically includes the `FIO_ATOL`, `FIO_RAND` and `FIO_IMAP`, flags, since CLI parsing and cleanup depends on them.

**Note**: the `fio_cli` is **NOT** thread-safe unless limited to reading once multi-threading had started (read is immutable, write is where things can go wrong).

#### `fio_cli_start`

```c
#define fio_cli_start(argc, argv, unnamed_min, unnamed_max, description, ...)  \
  fio_cli_start((argc), (argv), (unnamed_min), (unnamed_max), (description),   \
                (char const *[]){__VA_ARGS__, (char const *)NULL})

/* the shadowed function: */
void fio_cli_start   (int argc, char const *argv[],
                      int unnamed_min, int unnamed_max,
                      char const *description,
                      char const **names);
```

The `fio_cli_start` **macro** shadows the `fio_cli_start` function and defines the CLI interface to be parsed. i.e.,

The `fio_cli_start` macro accepts the `argc` and `argv`, as received by the `main` functions, a maximum and minimum number of unspecified CLI arguments (beneath which or after which the parser will fail), an application description string and a variable list of (specified) command line arguments.

If the minimum number of unspecified CLI arguments is `-1`, there will be no maximum limit on the number of unnamed / unrecognized arguments allowed  

The text `NAME` in the description (all capitals) will be replaced with the executable command invoking the application.

Command line arguments can be either String, Integer or Boolean. Optionally, extra data could be added to the CLI help output. CLI arguments and information is added using any of the following macros:

* `FIO_CLI_STRING("-arg [-alias] [(default_value)] desc.")`

* `FIO_CLI_INT("-arg [-alias] [(default_value)] desc.")`

* `FIO_CLI_BOOL("-arg [-alias] desc.")` (cannot accept default values)

* `FIO_CLI_PRINT_HEADER("header text (printed as a header)")`

* `FIO_CLI_PRINT("argument related line (printed as part of the previous argument)")`

* `FIO_CLI_PRINT_LINE("raw text line (printed as is, no spacing or offset)")`

**Note**: default values may optionally be provided by placing them in parenthesis immediately after the argument name and aliases. Default values that start with `(` must end with `)` (the surrounding parenthesis are ignored). Default values that start with `("` must end with `")` (the surrounding start and end markers are ignored).

```c
#define FIO_CLI
#include "fio-stl.h"
int main(int argc, char const *argv[]) {
  fio_cli_start(argc, argv, 0, -1,
                "this is a CLI example for the NAME application.\n"
                "This example allows for unlimited arguments that will be printed.",
                FIO_CLI_PRINT_HEADER("CLI type validation"),
                FIO_CLI_STRING("--str -s (my default string) any data goes here"),
                FIO_CLI_INT("--int -i (42) integer data goes here"),
                FIO_CLI_BOOL("--bool -b flag (boolean) only - no data"),
                FIO_CLI_PRINT("boolean flags cannot have default values."),
                FIO_CLI_PRINT_LINE("We hope you enjoy the NAME example.")
                );
  if (fio_cli_get("-s")) /* always true when default value is provided */
    fprintf(stderr, "String: %s\n", fio_cli_get("-s"));

  fprintf(stderr, "Integer: %d\n", (int)fio_cli_get_i("-i"));

  fprintf(stderr, "Boolean: %d\n", (int)fio_cli_get_i("-b"));

  if (fio_cli_unnamed_count()) {
    fprintf(stderr, "Printing unlisted / unrecognized arguments:\n");
    for (size_t i = 0; i < fio_cli_unnamed_count(); ++i) {
      fprintf(stderr, "%s\n", fio_cli_unnamed(i));
    }
  }

  fio_cli_end();
  return 0;
}
```

#### `fio_cli_end`

```c
void fio_cli_end(void);
```

Clears the CLI data storage.

#### `fio_cli_get`

```c
char const *fio_cli_get(char const *name);
```

Returns the argument's value as a string, or NULL if the argument wasn't provided.

#### `fio_cli_get_i`

```c
int64_t fio_cli_get_i(char const *name);
```

Returns the argument's value as an integer, or 0 if the argument wasn't provided.

**Note:** the command-line accepts integers in base 10, base 16, base 8 and binary as long as they have the appropriate prefix (i.e., none, `0x`, `0`, `0b`).

#### `fio_cli_get_bool`

```c
#define fio_cli_get_bool(name) (fio_cli_get((name)) != NULL)
```

Evaluates to true (1) if the argument was boolean and provided. Otherwise evaluated to false (0).

#### `fio_cli_unnamed_count`

```c
unsigned int fio_cli_unnamed_count(void);
```

Returns the number of unrecognized arguments (arguments unspecified, in `fio_cli_start`).

#### `fio_cli_unnamed`

```c
char const *fio_cli_unnamed(unsigned int index);
```

Returns a String containing the unrecognized argument at the stated `index` (indexes are zero based).

#### `fio_cli_set`

```c
void fio_cli_set(char const *name, char const *value);
```

Sets a value for the named argument (value will propagate to named aliases).

#### `fio_cli_set_i`

```c
void fio_cli_set_i(char const *name, int64_t i);
```

Sets a numeral value for the named argument (but **not** it's aliases).

**Note**: this basically writes a string with a base 10 representation.


#### `fio_cli_each`

```c
size_t fio_cli_each(int (*task)(fio_buf_info_s name,
                                fio_buf_info_s value,
                                fio_cli_arg_e arg_type,
                                void *udata),
                          void *udata);
```

Calls `task` for every argument received, returning the number of times `task` was called.

If `task` returns a non-zero value, iteration is stopped and `fio_cli_each` returns.

The `udata` pointer is an opaque user pointer passed along.

-------------------------------------------------------------------------------
## Local Memory Allocation

```c
#define FIO_MEMORY_NAME mem
#include "fio-stl.h"
```

The facil.io library includes a fast, concurrent, local memory allocator designed for grouping together objects with similar lifespans. [This has many advantages](https://youtu.be/nZNd5FjSquk).

Multiple allocators can be defined using `FIO_MEMORY_NAME` and including `fio-stl.h` multiple times.

The shortcut `FIO_MALLOC` MACRO will define a local memory allocator shared by any facil.io types that are defined after that macro (in multiple `include` space).

When `FIO_MEMORY_DISABLE` is defined, all custom memory allocators will route to the system's `malloc`.

**Note**: this module defines memory allocation macros for all subsequent modules in the same `include` statement.

**Note**: when a memory allocator is defined, this module requires `FIO_STATE`, `FIO_THREADS`, `FIO_ATOMIC`, `FIO_IMAP_CORE`, and `FIO_RAND`, which will be automatically included.

**Note**: Use [`fio_mmap`](#fio_mmap) or the system's `malloc` for long-term allocations.

### Memory Allocator Overview

To minimize contention, the memory allocator uses allocation "arenas" that can work independently, allowing a number of threads to allocate memory in parallel with other threads (depending on the number of arenas).

The memory allocator collects "chunks" of memory from the system.

Each chunk is divided into "blocks" or used in whole as a "big-block".

Each block is assigned to an arena. Big block allocations aren't assigned to arenas and aren't performed in parallel.

Blocks and big-blocks are "sliced" in a similar manner to `sbrk` in order to allocate the actual memory.

A block (or big-block) is returned to the allocator for reuse only when it's memory was fully freed. A leaked allocation will prevent a block / big-block from being released back to the allocator.

If all the blocks in a memory chunk were freed, the chunk is either cached or returned to the system, according to the allocator's settings.

This behavior, including the allocator's default alignment, can be tuned / changed using compile-time macros.

It should be possible to use tcmalloc or jemalloc alongside facil.io's allocator.

It's also possible to prevent facil.io's custom allocator from compiling by defining `FIO_MEMORY_DISABLE` (`-DFIO_MEMORY_DISABLE`).

### The Memory Allocator's API

The functions were designed to be a drop in replacement to the system's memory allocation functions (`malloc`, `free` and friends).

Where some improvement could be made, it was made using an added function name to add improved functionality (such as `fio_realloc2`).

**Note**: the prefix `fio` will be different according to the `FIO_MEMORY_NAME` macro, it is used here because this is the prefix defined when using the `FIO_MALLOC` shortcut macro.

#### `fio_malloc`

```c
void * fio_malloc(size_t size);
```

Allocates memory of requested size, using the defined alignment (`FIO_MEMORY_ALIGN_LOG`).

Memory is **always** zeroed out, no need to manually zero the memory after allocation.

Allocations above the allocator's per-arena limit will be redirected to a large allocation chunk if enabled (see `FIO_MEMORY_ENABLE_BIG_ALLOC`).

Allocations above the allocator limit will be redirected to `mmap`, as if `fio_mmap` was called.

**Note**: the prefix `fio` will be different according to the `FIO_MEMORY_NAME` macro, it is used here because this is the prefix defined when using the `FIO_MALLOC` shortcut macro.

#### `fio_calloc`

```c
void * fio_calloc(size_t size_per_unit, size_t unit_count);
```

Same as calling `fio_malloc(size_per_unit * unit_count)`.

**Note**: the prefix `fio` will be different according to the `FIO_MEMORY_NAME` macro, it is used here because this is the prefix defined when using the `FIO_MALLOC` shortcut macro.

#### `fio_realloc`

```c
void * fio_realloc(void *ptr, size_t new_size);
```

Re-allocates memory. An attempt to avoid copying the data is made only for memory allocations that are performed directly against the system (sizes over the allocator limit).

**Note**: when reallocating, junk data may be copied onto the new allocation unit. It is better to use `fio_realloc2`.

**Note**: the prefix `fio` will be different according to the `FIO_MEMORY_NAME` macro, it is used here because this is the prefix defined when using the `FIO_MALLOC` shortcut macro.

#### `fio_realloc2`

```c
void * fio_realloc2(void *ptr, size_t new_size, size_t copy_length);
```

Re-allocates memory. An attempt to avoid copying the data is made only for memory allocations that are performed directly against the system (sizes over the allocator limit).

This variation could be significantly faster as it will copy less data.

This variation also promises that any memory over `copy_length` is zeroed out.

**Note**: the prefix `fio` will be different according to the `FIO_MEMORY_NAME` macro, it is used here because this is the prefix defined when using the `FIO_MALLOC` shortcut macro.

#### `fio_mmap`

```c
void * fio_mmap(size_t size);
```

Allocates memory directly using `mmap`, this is preferred for objects that both require almost a page of memory (or more) and expect a long lifetime.

However, since this allocation will invoke the system call (`mmap`), it will be inherently slower.

`fio_free` can be used for deallocating the memory.

**Note**: some overhead is added to the `size` allocation (about the same size as the alignment required), in order to store the allocation size information.

**Note**: the prefix `fio` will be different according to the `FIO_MEMORY_NAME` macro, it is used here because this is the prefix defined when using the `FIO_MALLOC` shortcut macro.

#### `fio_free`

```c
void fio_free(void *ptr);
```

Frees memory that was allocated using this allocator.

If memory was allocator using a different allocator,behavior is undefined... i.e.: with some bad luck, nothing will happen, not even a memory leak, with some good luck the program will crash and expose the bug.

**Note**: the prefix `fio` will be different according to the `FIO_MEMORY_NAME` macro, it is used here because this is the prefix defined when using the `FIO_MALLOC` shortcut macro.

**Note2**: if `fio_free` is called **after** the allocator had been "destroyed" (cleanup occurred.


#### `fio_malloc_after_fork`

```c
void fio_malloc_after_fork(void);
```

Never fork a multi-threaded process. Doing so might corrupt the memory allocation system. The risk applies for child processes.

However, if forking a multi-threaded process, calling this function from the child process would perform a best attempt at mitigating any issues (at the expense of possible leaks).

Instead of calling `fio_malloc_after_fork` for each allocator, it is recommended to call the state callback for all memory allocators by calling withing the child (forked) process:

```c
fio_state_callback_force(FIO_CALL_IN_CHILD);
```

**Note**: the prefix `fio` will be different according to the `FIO_MEMORY_NAME` macro, it is used here because this is the prefix defined when using the `FIO_MALLOC` shortcut macro.


#### `fio_realloc_is_safe`

```c
size_t fio_realloc_is_safe(void);
```

Returns a non-zero value (1) if the allocator will zero out memory before passing the allocated memory to the user.

### Memory Allocator Creation MACROS

#### `FIO_MALLOC` (shortcut)

This shortcut macros defines a general allocator with the prefix `fio` (i.e., `fio_malloc`, `fio_free`, etc').

The general allocator settings consume more memory to allow for higher relative performance when using the memory pool as a generic allocator rather than an object specific memory pool.

Some setup macros are automatically defined and the `FIO_MEM_REALLOC` macro family are automatically updated.


It is similar to using:

```c
/* for a general allocator, increase system allocation size to 8Gb */
#define FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG 23
/* for a general allocator, increase cache size */
#define FIO_MEMORY_CACHE_SLOTS 8
/* set fragmentation cost at 0.5Mb blocks */
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG 5
/* support big allocations using undivided memory chunks */
#define FIO_MEMORY_ENABLE_BIG_ALLOC 1
/* secure by default */
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 1

#undef FIO_MEM_REALLOC
#undef FIO_MEM_FREE
#undef FIO_MEM_REALLOC_IS_SAFE
#undef FIO_MEM_ALIGNMENT_SIZE

/*
* Set if FIO_MEM_REALLOC copies only what was asked,
* and the rest of the memory is initialized (as if returned from calloc).
*/
#define FIO_MEM_REALLOC_IS_SAFE fio_realloc_is_safe()
/* Stores memory alignment info. */
#define FIO_MEM_ALIGNMENT_SIZE fio_malloc_alignment()

/** Reallocates memory, copying (at least) `copy_len` if necessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len) fio_realloc2((ptr), (new_size), (copy_len))
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) fio_free((ptr))
```

**Note**: this macro also (re)defines the `FIO_MEM_REALLOC_IS_SAFE` macro, allowing you to know if `fio_malloc` (and it's feature of memory being zeroed out) is available.

#### `FIO_MEMORY_NAME`

**REQUIRED**: the prefix for the memory-pool allocator.

This also automatically updates the temporary memory allocation macros (`FIO_MEM_REALLOC_`, etc') so all types defined in the same `include` statement as the allocator will use this allocator instead of the default allocator assigned using `FIO_MEM_REALLOC` (nothe the `_`).

#### `FIO_MALLOC_OVERRIDE_SYSTEM`

Overrides the system's default `malloc` to use this allocator instead.

#### `FIO_MEMORY_ALIGN_LOG`

```c
#define FIO_MEMORY_ALIGN_LOG 6
```

Sets the memory allocation alignment log. This starts with 8 byte alignment (value of 3) and accepts values up to 1024 (value of 10).

The default is 6 (64 byte alignment) which is a typical cache line alignment on many machines.

Allocation alignment, if set, **must** be >= 3 and <= 10.

This macro automatically defines the `FIO_MEMORY_ALIGN_SIZE` macro for internal use.

### Memory Allocator Configuration MACROS

The following compile time MACROS can effect the tuning and configuration of the resulting memory allocator.

#### `FIO_MEMORY_INITIALIZE_ALLOCATIONS`

```c
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT
```

If true, all allocations (including `realloc2` but excluding `realloc`) will return initialized memory and memory will be zeroed out earlier.

**Note**: when using `realloc` (vs., `realloc2`), the allocator does not know the size of the original allocation or its copy limits, so the memory isn't guaranteed to be initialized unless using `realloc2` which promises that any memory over `copy_len`is initialized.

The default value is controlled by the macro `FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT`.

#### `FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT`

```c
/* secure by default */
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT 1
```

Controls the default behavior for facil.io memory allocators (see `FIO_MEMORY_INITIALIZE_ALLOCATIONS`).

To increase performance, at the expense of the improved security and features provided by an allocator that zeros out memory early and often, set this value to 0.

#### `FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG`

```c
#define FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG 21
```

The logarithmic size of a single allocatiion "chunk" (16 blocks).

Limited to >=17 and <=24.

By default 22, which is a \~2Mb allocation per system call, resulting in a maximum allocation size of 131Kb.

This macro automatically defines the `FIO_MEMORY_SYS_ALLOCATION_SIZE` macro for internal use.

#### `FIO_MEMORY_CACHE_SLOTS`

```c
#define FIO_MEMORY_CACHE_SLOTS 4
```

The number of system allocation "chunks" to cache even if they are not in use.


#### `FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG`

```c
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG 2 /* 8 blocks per allocation */
```

The number of blocks per system allocation.

More blocks protect against fragmentation and improve memory leak detection, but lower the maximum number of bytes that can be allocated without reverting to large allocations (see `FIO_MEMORY_ENABLE_BIG_ALLOC`) or `fio_mmap`.

**Range**: 0-4

**Recommended**: depends on object allocation sizes, usually 1 or 2.

#### `FIO_MEMORY_ENABLE_BIG_ALLOC`

```c
#define FIO_MEMORY_ENABLE_BIG_ALLOC 1
```

Uses a whole system allocation to support bigger allocations.

Using big allocations could increase fragmentation costs for large long-life objects and decrease the chance of a memory-leak detection.

However, if there are no bugs in the system and objects have shot/medium lifespans, this could increase performance for larger allocations as it could avoid system calls.

#### `FIO_MEMORY_ARENA_COUNT`

```c
#define FIO_MEMORY_ARENA_COUNT -1
```

Memory arenas mitigate thread contention while using more memory.

Note that at some point arenas are statistically irrelevant (except when benchmarking contention in multi-core machines).

Zero / negative values will result in dynamic selection based on CPU core count.

#### `FIO_MEMORY_ARENA_COUNT_FALLBACK`

```c
#define FIO_MEMORY_ARENA_COUNT_FALLBACK 8
```

Used when the dynamic arena count calculations fails.

**Note**: relevant if `FIO_MEMORY_ARENA_COUNT` is zero/negative, since dynamic arena calculation is performed using CPU core calculation.

#### `FIO_MEMORY_ARENA_COUNT_MAX`

```c
#define FIO_MEMORY_ARENA_COUNT_MAX 32
```

Defines the maximum number of arenas to allocate when using dynamic arena calculation.

**Note**: relevant if `FIO_MEMORY_ARENA_COUNT` is zero/negative, since dynamic arena calculation is performed using CPU core calculation.

#### `FIO_MEMORY_USE_THREAD_MUTEX`

```c
/*
* If arena count isn't linked to the CPU count, threads might busy-spin.
* It is better to slow wait than fast busy spin when the work in the lock is longer...
* and system allocations are performed inside arena locks.
*/
#if FIO_MEMORY_ARENA_COUNT > 0
#define FIO_MEMORY_USE_THREAD_MUTEX 1
#else
#define FIO_MEMORY_USE_THREAD_MUTEX 0
#endif
```

If true, uses a `pthread mutex` instead of a `fio_lock_i` spinlock.

When setting `FIO_USE_THREAD_MUTEX_TMP` or `FIO_USE_THREAD_MUTEX` to true (`1`), than the default value of this macro will be `1`. However, this macro can be used to override the default `FIO_USE_THREAD_MUTEX` / `FIO_USE_THREAD_MUTEX_TMP`.

#### `FIO_MEMORY_WARMUP`

```c
#define FIO_MEMORY_WARMUP 0
```

If set to a number, will allocate memory on startup to the number of arenas indicated.

It is usually better to avoid this unless using a single arena.

#### `FIO_MEM_PAGE_ALLOC`, `FIO_MEM_PAGE_REALLOC` and `FIO_MEM_PAGE_FREE`

```c
#define FIO_MEM_PAGE_ALLOC(pages, alignment_log)                               \
  FIO_MEM_PAGE_ALLOC_def_func((pages), (alignment_log))

#define FIO_MEM_PAGE_REALLOC(ptr, old_pages, new_pages, alignment_log)         \
  FIO_MEM_PAGE_REALLOC_def_func((ptr), (old_pages), (new_pages), (alignment_log))

#define FIO_MEM_PAGE_FREE(ptr, pages)                                          \
  FIO_MEM_PAGE_FREE_def_func((ptr), (pages))
```

These MACROS, when all of them are defined, allow the memory allocator to collect memory from the system using an alternative method.

This allows the allocator to be used in situations where `mmap` is unavailable.

**Note:** the alignment property for the allocated memory is essential and may me quite large (see `FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG`).

### Debugging the allocator

The following functions will also be defined per-allocator. However, they should be considered experimental and unstable as they are linked to the allocator's internal workings.

#### `fio_malloc_sys_alloc_size`

```c
size_t fio_malloc_sys_alloc_size(void);
```

Returns the allocation size used when allocating memory from the system.

#### `fio_malloc_block_size`

```c
size_t fio_malloc_block_size(void);
```

Returns the block size used for allocations. Blocks are slices of system allocations used for allocating memory to the user.

#### `fio_malloc_cache_slots`

```c
size_t fio_malloc_cache_slots(void);
```

Returns the number of cache slots. Cache slots prevent memory chunks from being returned to the system, assuming that the memory will be needed again by the user.

#### `fio_malloc_alignment`

```c
size_t fio_malloc_alignment(void);
```

Returns the allocation alignment set for the allocator.

#### `fio_malloc_alignment_log`

```c
size_t fio_malloc_alignment_log(void);
```

Returns the allocation alignment log set for the allocator.

#### `fio_malloc_alloc_limit`

```c
size_t fio_malloc_alloc_limit(void);
```

Returns the per-allocation size limit, after which a call to `mmap` will be used.

#### `fio_malloc_arena_alloc_limit`

```c
size_t fio_malloc_arena_alloc_limit(void);
```

Returns the per-allocation size limit for an arena based allocation, after which a big-block allocation or `mmap` will be used.

#### `fio_malloc_print_state`

```c
void fio_malloc_print_state(void);
```

Prints information from the allocator's data structure. May be used for debugging.

#### `fio_malloc_print_settings`

```c
void fio_malloc_print_settings(void);
```

Prints the settings used to define the allocator.

### Reserved `FIO_MEMORY` related macros

The following are reserved macro names:

* `FIO_ALIGN_NEW`

* `FIO_ALIGN`

* `FIO_MEMORY_ALIGN_SIZE`

* `FIO_MEMORY_ALLOC_LIMIT`

* `FIO_MEMORY_BIG_ALLOC_LIMIT`

* `FIO_MEMORY_BLOCK_ALLOC_LIMIT`

* `FIO_MEMORY_BLOCKS_PER_ALLOCATION`

* `FIO_MEMORY_SYS_ALLOCATION_SIZE`

* `FIO_MALLOC_TMP_USE_SYSTEM`


-------------------------------------------------------------------------------
## Basic IO Polling

```c
#define FIO_POLL
#include "fio-stl.h"
```

IO polling using `kqueue`, `epoll` or the portable `poll` POSIX function is another area that's of common need and where many solutions are required.

The facil.io standard library provides a persistent polling container for evented management of (small) IO (file descriptor) collections using the "one-shot" model.

"One-Shot" means that once a specific even has "fired" (occurred), it will no longer be monitored (unless re-submitted). If the same file desciptor is waiting on multiple event, only those events that occurred will be removed from the monitored collection.

There's no real limit on the number of file descriptors that can be monitored, except possible system limits that the system may impose on the `kqueue`/`epoll`/`poll` system calls. However, performance will degrade significantly as the ratio between inactive vs. active IO objects being monitored increases when using the `poll` system call.

It is recommended to use the system specific polling "engine" (`epoll` / `kqueue`) if polling thousands of persistent file descriptors.

By defining `FIO_POLL`, the following functions will be defined.

**Note**: the same type and range limitations that apply to the Sockets implementation on Windows apply to the `poll` implementation.

**Note**: when using `epoll` then the file descriptor (`fd`) will **NOT** be passed on to the callback (as `epoll` doesn't retain the data).

### `FIO_POLL` API


#### `fio_poll_s`

```c
typedef struct fio_poll_s fio_poll_s;
```

The `fio_poll_s` type should be considered opaque and should **not** be accessed directly.

#### `fio_poll_init`

```c
fio_poll_s *fio_poll_new(fio_poll_settings_s settings);
/* named argument support */
#define fio_poll_new(...) fio_poll_new((fio_poll_settings_s){__VA_ARGS__})
```

Creates a new polling object / queue.

The settings arguments set the `on_data`, `on_ready` and `on_close` callbacks:

```c
typedef struct {
  /** callback for when data is available in the incoming buffer. */
  void (*on_data)(void *udata);
  /** callback for when the outgoing buffer allows a call to `write`. */
  void (*on_ready)(void *udata);
  /** callback for closed connections and / or connections with errors. */
  void (*on_close)(void *udata);
} fio_poll_settings_s;
```

#### `fio_poll_destroy`

```c
void fio_poll_destroy(fio_poll_s *p);
```

Destroys the polling object, freeing its resources.

**Note**: the monitored file descriptors will remain untouched (possibly open).

#### `fio_poll_monitor`

```c
int fio_poll_monitor(fio_poll_s *p, int fd, void *udata, unsigned short flags);
```

Adds a file descriptor to be monitored, adds events to be monitored or updates the monitored file's `udata`.

Possible flags are: `POLLIN` and `POLLOUT`. Other flags may be set but might be ignored.

On systems where `POLLRDHUP` is supported, it is always monitored for.

Monitoring mode is always one-shot. If an event if fired, it is removed from the monitoring state.

Returns -1 on error.

#### `fio_poll_review`

```c
int fio_poll_review(fio_poll_s *p, int timeout);
```

Reviews if any of the monitored file descriptors has any events.

`timeout` is in milliseconds.

Returns the number of events called.

**Note**:

Polling is thread safe, but has different effects on different threads.

Adding a new file descriptor from one thread while polling in a different thread will not poll that IO untill `fio_poll_review` is called again.

#### `fio_poll_forget`

```c
void *fio_poll_forget(fio_poll_s *p, int fd);
```

Stops monitoring the specified file descriptor even if some of it's event's hadn't occurred just yet, returning its `udata` (if any).

### `FIO_POLL` Compile Time Macros

#### `FIO_POLL_ENGINE`

```c
#define FIO_POLL_ENGINE_POLL   1
#define FIO_POLL_ENGINE_EPOLL  2
#define FIO_POLL_ENGINE_KQUEUE 3
```

Allows for both the detection and the manual selection (override) of the underlying IO multiplexing API.

When multiplexing a small number of IO sockets, using the `poll` engine might be faster, as it uses less system calls.

```c
#define FIO_POLL_ENGINE FIO_POLL_ENGINE_POLL
```

#### `FIO_POLL_ENGINE_STR`

```c
#if FIO_POLL_ENGINE == FIO_POLL_ENGINE_POLL
#define FIO_POLL_ENGINE_STR "poll"
#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_EPOLL
#define FIO_POLL_ENGINE_STR "epoll"
#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_KQUEUE
#define FIO_POLL_ENGINE_STR "kqueue"
#endif

```

A string MACRO representing the used IO multiplexing "engine".

-------------------------------------------------------------------------------
## Task Queue

```c
#define FIO_QUEUE
#include "fio-stl.h"
```

The facil.io library includes a simple, thread-safe, task queue based on a linked list of ring buffers.

Since delayed processing is a common task, this queue is provides an easy way to schedule and perform delayed tasks.

In addition, a Timer type allows timed events to be scheduled and moved (according to their "due date") to an existing Task Queue.

By `FIO_QUEUE`, the following task and timer related helpers are defined:

### Queue Related Types

#### `fio_queue_task_s`

```c
/** Task information */
typedef struct {
  /** The function to call */
  void (*fn)(void *, void *);
  /** User opaque data */
  void *udata1;
  /** User opaque data */
  void *udata2;
} fio_queue_task_s;
```

The `fio_queue_task_s` type contains information about a delayed task. The information is important for the `fio_queue_push` MACRO, where it is used as named arguments for the task information.

#### `fio_queue_s`

```c
/** The queue object - should be considered opaque (or, at least, read only). */
typedef struct {
  fio___task_ring_s *r;
  fio___task_ring_s *w;
  /** the number of tasks waiting to be performed (read-only). */
  size_t count;
  fio_lock_i lock; /* unless FIO_USE_THREAD_MUTEX(_TMP) is true */
  fio___task_ring_s mem;
} fio_queue_s;
```

The `fio_queue_s` object is the queue object.

This object could be placed on the stack or allocated on the heap (using [`fio_queue_new`](#fio_queue_new)).

Once the object is no longer in use call [`fio_queue_destroy`](#fio_queue_destroy) (if placed on the stack) of [`fio_queue_free`](#fio_queue_free) (if allocated using [`fio_queue_new`](#fio_queue_new)).

### Queue API

#### `fio_queue_init`

```c
/** Used to initialize a fio_queue_s object. */
void fio_queue_init(fio_queue_s *q);
```

#### `fio_queue_destroy`

```c
void fio_queue_destroy(fio_queue_s *q);
```

Destroys a queue and re-initializes it, after freeing any used resources.

**Note**:
When using the optional `pthread_mutex_t` implementation or using timers on Windows, the timer object needs to be re-initialized explicitly before re-used after being destroyed (call `fio_queue_init`).

#### `FIO_QUEUE_STATIC_INIT(queue)`

```c
#define FIO_QUEUE_STATIC_INIT(queue)                                           \
  { .r = &(queue).mem, .w = &(queue).mem, .lock = FIO_LOCK_INIT }
```

May be used to initialize global, static memory, queues.

**Note**: while the use `FIO_QUEUE_STATIC_INIT` is possible,  this macro resets a whole page of memory to zero whereas `fio_queue_init` only initializes a few bytes of memory which are the only relevant bytes during initialization.

#### `fio_queue_new`

```c
fio_queue_s *fio_queue_new(void);
```

Creates a new queue object (allocated on the heap).

#### `fio_queue_free`

```c
void fio_queue_free(fio_queue_s *q);
```

Frees a queue object after calling fio_queue_destroy.

#### `fio_queue_push`

```c
int fio_queue_push(fio_queue_s *q, fio_queue_task_s task);
#define fio_queue_push(q, ...)                                                 \
  fio_queue_push((q), (fio_queue_task_s){__VA_ARGS__})

```

Pushes a **valid** (non-NULL) task to the queue.

This function is shadowed by the `fio_queue_push` MACRO, allowing named arguments to be used.

For example:

```c
void tsk(void *, void *);
fio_queue_s q = FIO_QUEUE_INIT(q);
fio_queue_push(q, .fn = tsk);
// ...
fio_queue_destroy(q);
```

Returns 0 if `task.fn == NULL` or if the task was successfully added to the queue.

Returns -1 on error (no memory).


#### `fio_queue_push_urgent`

```c
int fio_queue_push_urgent(fio_queue_s *q, fio_queue_task_s task);
#define fio_queue_push_urgent(q, ...)                                          \
  fio_queue_push_urgent((q), (fio_queue_task_s){__VA_ARGS__})
```

Pushes a task to the head of the queue (LIFO).

Returns -1 on error (no memory).

See [`fio_queue_push`](#fio_queue_push) for details.

#### `fio_queue_pop`

```c
fio_queue_task_s fio_queue_pop(fio_queue_s *q);
```

Pops a task from the queue (FIFO).

Returns a NULL task on error (`task.fn == NULL`).

**Note**: The task isn't performed automatically, it's just returned. This is useful for queues that don't necessarily contain callable functions.

#### `fio_queue_perform`

```c
int fio_queue_perform(fio_queue_s *q);
```

Pops and performs a task from the queue (FIFO).

Returns -1 on error (queue empty).

#### `fio_queue_perform_all`

```c
void fio_queue_perform_all(fio_queue_s *q);
```

Performs all tasks in the queue.

#### `fio_queue_count`

```c
size_t fio_queue_count(fio_queue_s *q);
```

Returns the number of tasks in the queue.

### Timer Related Types

#### `fio_timer_queue_s`

```c
typedef struct {
  fio___timer_event_s *next;
  fio_lock_i lock;
} fio_timer_queue_s;
```

The `fio_timer_queue_s` struct should be considered an opaque data type and accessed only using the functions or the initialization MACRO.

To create a `fio_timer_queue_s` on the stack (or statically):

```c
fio_timer_queue_s foo_timer = FIO_TIMER_QUEUE_INIT;
```

A timer could be allocated dynamically:

```c
fio_timer_queue_s *foo_timer = malloc(sizeof(*foo_timer));
FIO_ASSERT_ALLOC(foo_timer);
*foo_timer = (fio_timer_queue_s)FIO_TIMER_QUEUE_INIT(*foo_timer);
```

#### `FIO_TIMER_QUEUE_INIT`

This is a MACRO used to statically initialize a `fio_timer_queue_s` object.

### Timer API

#### `fio_timer_schedule`

```c
void fio_timer_schedule(fio_timer_queue_s *timer_queue,
                        fio_timer_schedule_args_s args);
```

Adds a time-bound event to the timer queue.

Accepts named arguments using the following argument type and MACRO:

```c
typedef struct {
  /** The timer function. If it returns a non-zero value, the timer stops. */
  int (*fn)(void *, void *);
  /** Opaque user data. */
  void *udata1;
  /** Opaque user data. */
  void *udata2;
  /** Called when the timer is done (finished). */
  void (*on_finish)(void *, void *);
  /** Timer interval, in milliseconds. */
  uint32_t every;
  /** The number of times the timer should be performed. -1 == infinity. */
  int32_t repetitions;
  /** Millisecond at which to start. If missing, filled automatically. */
  uint64_t start_at;
} fio_timer_schedule_args_s;

#define fio_timer_schedule(timer_queue, ...)                                   \
  fio_timer_schedule((timer_queue), (fio_timer_schedule_args_s){__VA_ARGS__})
```

Note, the event will repeat every `every` milliseconds (or the same unites as `start_at` and `now`).

It the scheduler is busy or the event is otherwise delayed, its next scheduling may compensate for the delay by being scheduled sooner.

#### `fio_timer_push2queue` 

```c
/**  */
size_t fio_timer_push2queue(fio_queue_s *queue,
                            fio_timer_queue_s *timer_queue,
                            uint64_t now); // now is in milliseconds
```

Pushes due events from the timer queue to an event queue.

If `now` is `0`, than `fio_time_milli` will be called to supply `now`'s value.

**Note**: all the `start_at` values for all the events in the timer queue will be treated as if they use the same units as (and are relative to) `now`. By default, this unit should be milliseconds, to allow `now` to be zero.

Returns the number of tasks pushed to the queue. A value of `0` indicates no new tasks were scheduled.

#### `fio_timer_next_at`

```c
int64_t fio_timer_next_at(fio_timer_queue_s *timer_queue);
```

Returns the millisecond at which the next event should occur.

If no timer is due (list is empty), returns `-1`.

**Note**: Unless manually specified, millisecond timers are relative to  `fio_time_milli()`.


#### `fio_timer_destroy`

```c
void fio_timer_destroy(fio_timer_queue_s *timer_queue);
```

Clears any waiting timer bound tasks.

**Note**:

The timer queue must NEVER be freed when there's a chance that timer tasks are waiting to be performed in a `fio_queue_s`.

This is due to the fact that the tasks may try to reschedule themselves (if they repeat).

**Note 2**:
When using the optional `pthread_mutex_t` implementation or using timers on Windows, the timer object needs to be reinitialized before re-used after being destroyed.

-------------------------------------------------------------------------------
## Data Stream Container

```c
#define FIO_STREAM
#include "fio-stl.h"
```

Data Stream objects solve the issues that could arise when `write` operations don't write all the data (due to OS buffering). 

Data Streams offer a way to store / concat different data sources (static strings, dynamic strings, files) as a single data stream. This allows the data to be easily written to an IO target (socket / pipe / file) using the `write` operation.

By defining the macro `FIO_STREAM`, the following macros and functions will be defined.

#### `fio_stream_s`

```c
typedef struct {
  /* do not directly acecss! */
  fio_stream_packet_s *next;
  fio_stream_packet_s **pos;
  size_t consumed;
  size_t length;
} fio_stream_s;
```

The `fio_stream_s` type should be considered opaque and only accessed through the following API.

#### `fio_stream_packet_s`

The `fio_stream_packet_s` type should be considered opaque and only accessed through the following API.

This type is used to separate data packing with any updates made to the stream object, allowing data packing to be performed concurrently with stream reading / updating (which requires a lock in multi-threaded applications).


#### `FIO_STREAM_INIT(stream)`

```c
#define FIO_STREAM_INIT(s)                                                     \
  { .next = NULL, .pos = &(s).next }
#endif
```

Object initialization macro.

#### `fio_stream_new`

```c
fio_stream_s *fio_stream_new(void);
```

Allocates a new object on the heap and initializes it's memory.

#### `fio_stream_free`

```c
int fio_stream_free(fio_stream_s *stream);
```

Frees any internal data AND the object's container!

#### `fio_stream_destroy`

```c
void fio_stream_destroy(fio_stream_s *stream);
```

Destroys the object, reinitializing its container.

#### `fio_stream_any`

```c
uint8_t fio_stream_any(fio_stream_s *stream);
````

Returns true if there's any data in the stream.

**Note**: this isn't thread safe, but it often doesn't matter if it is.

#### `fio_stream_length`

```c
size_t fio_stream_length(fio_stream_s *stream);
````

Returns the number of bytes waiting in the stream.

**Note**: this isn't thread safe, but it often doesn't matter if it is.

### Packing data into the stream

#### `fio_stream_pack_data`

```c
fio_stream_packet_s *fio_stream_pack_data(void *buf,
                                          size_t len,
                                          size_t offset,
                                          uint8_t copy_buffer,
                                          void (*dealloc_func)(void *));
```

Packs data into a `fio_stream_packet_s` container.

Can be performed concurrently with other operations.

#### `fio_stream_pack_fd`

```c
fio_stream_packet_s * fio_stream_pack_fd(int fd, size_t len, size_t offset, uint8_t keep_open);
```

Packs a file descriptor into a `fio_stream_packet_s` container. 

#### `fio_stream_add`

```c
void fio_stream_add(fio_stream_s *stream, fio_stream_packet_s *packet);
```

Adds a packet to the stream.

**Note**: this isn't thread safe.

#### `fio_stream_pack_free`

```c
void fio_stream_pack_free(fio_stream_packet_s *packet);
```

Destroys the `fio_stream_packet_s` - call this ONLY if the packed data was never added to the stream using `fio_stream_add`. 


### Reading / Consuming data from the Stream


#### `fio_stream_read`

```c
void fio_stream_read(fio_stream_s *stream, char **buf, size_t *len);
```

Reads data from the stream (if any), leaving the data in the stream **without advancing the reading position** (see [`fio_stream_advance`](#fio_stream_advance).

`buf` MUST point to a buffer with - at least - `len` bytes. This is required in case the packed data is fragmented or references a file and needs to be copied to an available buffer.

On error, or if the stream is empty, `buf` will be set to NULL and `len` will be set to zero.

Otherwise, `buf` may retain the same value or it may point directly to a memory address within the stream's buffer (the original value may be lost) and `len` will be updated to the largest possible value for valid data that can be read from `buf`.

**Note**: this isn't thread safe.

#### `fio_stream_advance`

```c
void fio_stream_advance(fio_stream_s *stream, size_t len);
```

Advances the Stream, so the first `len` bytes are marked as consumed.

**Note**: this isn't thread safe.

### Stream configuration

Besides the (recommended) use of a local allocator using the `FIO_MEMORY` or `FIO_MEM_REALLOC` macro families, the following configuration macros are supported:

#### `FIO_STREAM_COPY_PER_PACKET`

When copying data to the stream, large memory sections will be divided into smaller allocations in order to free memory faster and minimize the direct use of `mmap`.

This macro should be set according to the specific allocator limits. By default, it is set to 96Kb (which is neither here nor there).

-------------------------------------------------------------------------------

## Binary Safe Core String Helpers

```c
#define FIO_STR
#include "fio-stl.h"
```

The following helpers are part of the String core library and they become available whenever [a String type was defined](#dynamic-strings) or when the `FIO_STR` is defined before any inclusion of the C STL header.

The main difference between using the Core String API directly and defining a String type is that String types provide a few additional optimizations, such as embedding short strings (embedded within the type data rather than allocated), optional reference counting and pointer tagging features.

**Note**: the `fio_string` functions might fail or truncate data if memory allocation fails. Test the returned value for failure (success returns `0`, failure returns `-1`).


**Note:** this module depends on the  `FIO_ATOL`,  `FIO_ATOMIC`, `FIO_RAND`, and `FIO_FILES` modules which will be automatically included.

### Core String Authorship

#### `fio_string_write`

```c
static inline int fio_string_write(fio_str_info_s *dest,
                               void (*reallocate)(fio_str_info_s *,
                                                  size_t new_capa),
                               const void *src,
                               size_t len);
```

Writes data to the end of the string in the `fio_string_s` struct, returning an updated `fio_string_s` struct.

The returned string is NUL terminated if edited.

* `dest` an `fio_string_s` struct containing the destination string.

* `reallocate` is a callback that attempts to reallocate more memory (i.e., using realloc) and returns an updated `fio_string_s` struct containing the updated capacity and buffer pointer (as well as the original length).

    On failure the original `fio_string_s` should be returned. if `reallocate` is NULL or fails, the data copied will be truncated.

* `src` is the data to be written to the end of `dest`.

* `len` is the length of the data to be written to the end of `dest`.

**Note**: this function performs only minimal checks and assumes that `dest` is fully valid - i.e., that `dest.capa >= dest.len`, that `dest.buf` is valid, etc'.

**Note**: `reallocate`, if called, will be called only once.

An example for a `reallocate` callback using the system's `realloc` function (or use `FIO_STRING_REALLOC` / `FIO_STRING_FREE`):

```c
fio_str_info_s fio_string_realloc_system(fio_str_info_s dest,
                                         size_t len) {
  /* must allocate at least len + 1 bytes. */
  const size_t new_capa = fio_string_capa4len(len);
  void *tmp = realloc(dest.buf, new_capa);
  if (!tmp)
    return dest;
  dest.capa = new_capa;
  dest.buf = (char *)tmp;
  return dest;
}
```

An example for using the function:

```c
void example(void) {
  char buf[32];
  fio_str_info_s str = FIO_STR_INFO3(buf, 0, 32);
  fio_string_write(&str, NULL, "The answer is: 0x", 17);
  str.len += fio_ltoa(str.buf + str.len, 42, 16);
  fio_string_write(&str, NULL, "!\n", 2);
  printf("%s", str.buf);
}
```

#### `fio_string_replace`

```c
int fio_string_replace(fio_str_info_s *dest,
                      void (*reallocate)(fio_str_info_s *,
                                         size_t new_capa),
                      intptr_t start_pos,
                      size_t overwrite_len,
                      const void *src,
                      size_t len);
```

Similar to `fio_string_write`, only replacing/inserting a sub-string in a specific location.

Negative `start_pos` values are calculated backwards, `-1` == end of String.

When `overwrite_len` is zero, the function will insert the data at `start_pos`, pushing existing data until after the inserted data.

If `overwrite_len` is non-zero, than `overwrite_len` bytes will be overwritten (or deleted).

If `len == 0` than `src` will be ignored and the data marked for replacement will be erased.

**Note**: `reallocate`, if called, will be called only once.

#### `fio_string_write2`

```c
int fio_string_write2(fio_str_info_s *restrict dest,
                      void (*reallocate)(fio_str_info_s *,
                                         size_t new_capa),
                      const fio_string_write_s sources[]);
/* Helper macro for fio_string_write2 */
#define fio_string_write2(dest, reallocate, ...)                               \
  fio_string_write2((dest),                                                    \
                    (reallocate),                                              \
                    (fio_string_write_s[]){__VA_ARGS__, {0}})
```

Writes a group of objects (strings, numbers, etc') to `dest`.

`dest` and `reallocate` are similar to `fio_string_write`.

**Note**: `reallocate`, if called, will be called only once.

`sources` is an array of `fio_string_write_s` structs, ending with a struct that's all set to 0. This array is usually populated using the following macros:

```c
/** Used to write raw string data to the string. */
#define FIO_STRING_WRITE_STR1(str_)                                            \
  ((fio_string_write_s){.klass = 1,                                            \
                        .info.str = {.len = strlen((str_)), .buf = (str_)}})
/** Used to write raw (possibly binary) string data to the string. */
#define FIO_STRING_WRITE_STR2(str_, len_)                                      \
  ((fio_string_write_s){.klass = 1, .info.str = {.len = (len_), .buf = (str_)}})
/** Used to write a signed number to the string. */
#define FIO_STRING_WRITE_NUM(num)                                              \
  ((fio_string_write_s){.klass = 2, .info.i = (int64_t)(num)})
/** Used to write an unsigned number to the string. */
#define FIO_STRING_WRITE_UNUM(num)                                             \
  ((fio_string_write_s){.klass = 3, .info.u = (uint64_t)(num)})
/** Used to write a hex representation of a number to the string. */
#define FIO_STRING_WRITE_HEX(num)                                              \
  ((fio_string_write_s){.klass = 4, .info.u = (uint64_t)(num)})
/** Used to write a binary representation of a number to the string. */
#define FIO_STRING_WRITE_BIN(num)                                              \
  ((fio_string_write_s){.klass = 5, .info.u = (uint64_t)(num)})
/** Used to write a double(!) to the string. */
#define FIO_STRING_WRITE_FLOAT(num)                                            \
  ((fio_string_write_s){.klass = 6, .info.f = (double)(num)})
```

Use the `fio_string_write2` macro for ease, i.e.:

```c
fio_str_info_s str = {0};
fio_string_write2(&str, my_reallocate,
                   FIO_STRING_WRITE_STR1("The answer is: "),
                   FIO_STRING_WRITE_NUM(42),
                   FIO_STRING_WRITE_STR2("(0x", 3),
                   FIO_STRING_WRITE_HEX(42),
                   FIO_STRING_WRITE_STR2(")", 1));
```

**Note**: this function might end up allocating more memory than absolutely required as it favors speed over memory savings.

For this function, the facil.io C STL reserves and defines the following type:

```c
/** Argument type used by fio_string_write2. */
typedef struct {
  size_t klass;
  union {
    fio_str_info_s str;
    double f;
    int64_t i;
    uint64_t u;
  } info;
} fio_string_write_s;
```

### Core String Numeral Helpers

#### `fio_string_write_i`

```c
static inline int fio_string_write_i(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
                                 int64_t i);
```

Writes a signed number `i` to the String.

**Note**: `reallocate`, if called, will be called only once.

#### `fio_string_write_u`

```c
static inline int fio_string_write_u(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
                                 uint64_t u);
```

Writes an unsigned number `u` to the String.

**Note**: `reallocate`, if called, will be called only once.

#### `fio_string_write_hex`

```c
static inline int fio_string_write_hex(fio_str_info_s *dest,
                                   void (*reallocate)(fio_str_info_s *,
                                                      size_t new_capa),
                                   uint64_t i);
```

Writes a hex representation of `i` to the String.

**Note**: `reallocate`, if called, will be called only once.

#### `fio_string_write_bin`

```c
static inline int fio_string_write_bin(fio_str_info_s *dest,
                                   void (*reallocate)(fio_str_info_s *,
                                                      size_t new_capa),
                                   uint64_t i);
```

Writes a binary representation of `i` to the String.

**Note**: `reallocate`, if called, will be called only once.

### Core String `printf` Helpers

#### `fio_string_printf`

```c
static int fio_string_printf(fio_str_info_s *dest,
                                void (*reallocate)(fio_str_info_s *,
                                                   size_t new_capa),
                                const char *format,
                                ...);
```

Similar to fio_string_write, only using printf semantics.

#### `fio_string_vprintf`

```c
inline int fio_string_vprintf(fio_str_info_s *dest,
                                 void (*reallocate)(fio_str_info_s *,
                                                    size_t new_capa),
                                 const char *format,
                                 va_list argv);
```

Similar to fio_string_write, only using vprintf semantics.

### Core String Authorship Memory Helpers

#### `fio_string_capa4len`

```c
size_t fio_string_capa4len(size_t new_len);
```

Calculates a 16 bytes boundary aligned capacity for `new_len`.

The Core String API always allocates 16 byte aligned memory blocks, since most memory allocators will only allocate memory in multiples of 16 or more. By requesting the full 16 byte allocation, future allocations could be avoided without increasing memory usage.

#### `FIO_STRING_REALLOC`

```c
#define FIO_STRING_REALLOC fio_string_default_reallocate
void fio_string_default_reallocate(fio_str_info_s *dest, size_t new_capa);
```

Default reallocation callback implementation

#### `FIO_STRING_ALLOC_COPY`

```c
#define FIO_STRING_ALLOC_COPY fio_string_default_allocate_copy
void fio_string_default_allocate_copy(fio_str_info_s *dest, size_t new_capa);
```

Default reallocation callback for memory that mustn't be freed.

#### `FIO_STRING_FREE`

```c
#define FIO_STRING_FREE fio_string_default_free
void fio_string_default_free(void *);
```

Frees memory that was allocated with the default callbacks.

#### `FIO_STRING_FREE2`

```c
#define FIO_STRING_FREE2 fio_string_default_free2
void fio_string_default_free2(fio_str_info_s str);
```

Frees memory that was allocated with the default callbacks.

#### `FIO_STRING_FREE_NOOP`

```c
#define FIO_STRING_FREE_NOOP fio_string_default_free_noop
void fio_string_default_free_noop(void * str);
```

Does nothing. Made available for APIs that require a callback for memory management.

#### `FIO_STRING_FREE_NOOP2`

```c
#define FIO_STRING_FREE_NOOP2 fio_string_default_free_noop2
void fio_string_default_free_noop2(fio_str_info_s str);
```

Does nothing. Made available for APIs that require a callback for memory management.

### Core String Comparison

In addition to [`FIO_STR_INFO_IS_EQ(a,b)`](#fio_str_info_is_eq) and [`FIO_BUF_INFO_IS_EQ(a,b)`](#fio_buf_info_is_eq) MACROs, the following comparisons helpers are available:

#### `fio_string_is_greater`

```c
int fio_string_is_greater(fio_str_info_s a, fio_str_info_s b);
```
Equivalent to: `memcmp(a.buf, b.buf, min(a.len, b.len)) > 0 || (!memcmp(a.buf, b.buf, min(a.len, b,len)) && a.len > b.len)`

Compares two strings, returning 1 if the data in string `a` is greater in value than the data in string `b`.

**Note**: returns 0 if string `b` is bigger than string `a` or if strings are equal, designed to be used with `FIO_SORT_IS_BIGGER(a,b)`.

**Note**: it is often faster to define `FIO_SORT_IS_BIGGER` using a `memcmp` wrapper, however the speed depends on the `clib` implementation and this function provides a good enough fallback that should be very portable.

#### `fio_string_is_greater_buf`

```c
int fio_string_is_greater_buf(fio_buf_info_s a, fio_buf_info_s b);
```

Equivalent to: `memcmp(a.buf, b.buf, min(a.len, b.len)) > 0 || (!memcmp(a.buf, b.buf, min(a.len, b,len)) && a.len > b.len)`

Compares two `fio_buf_info_s`, returning 1 if the data in buffer `a` is greater in value than the data in buffer `b`.

**Note**: returns 0 if data in `b` is greater than **or equal** to `a`, designed to be used with `FIO_SORT_IS_BIGGER(a,b)`.

**Note**: it is often faster to define `FIO_SORT_IS_BIGGER` using a `memcmp` wrapper, however the speed depends on the `clib` implementation and this function provides a good enough fallback that should be very portable.

### Core String UTF-8 Support

#### `fio_string_utf8_valid`

```c
size_t fio_string_utf8_valid(fio_str_info_s str);
```

Returns 1 if the String is UTF-8 valid and 0 if not.

#### `fio_string_utf8_len`

```c
size_t fio_string_utf8_len(fio_str_info_s str);
```

Returns the String's length in UTF-8 characters or 0 on either an error or an empty string.

#### `fio_string_utf8_select`

```c
int fio_string_utf8_select(fio_str_info_s str, intptr_t *pos, size_t *len);
```

Takes a UTF-8 character selection information (UTF-8 position and length) and updates the same variables so they reference the raw byte slice information.

If the String isn't UTF-8 valid up to the requested selection, than `pos` will be updated to `-1` otherwise values are always positive.

The returned `len` value may be shorter than the original if there wasn't enough data left to accommodate the requested length. When a `len` value of `0` is returned, this means that `pos` marks the end of the String.

if `pos` is negative, counts backwards (`-1` is the position of the last UTF-8 character).

Returns -1 on error and 0 on success.

### Core String C / JSON escaping

#### `fio_string_write_escape`

```c
int fio_string_write_escape(fio_str_info_s *restrict dest,
                            fio_string_realloc_fn reallocate,
                            const void *src,
                            size_t len);
```

Writes data at the end of the String, escaping the data using JSON semantics.

The JSON semantic are common to many programming languages, promising a UTF-8
String while making it easy to read and copy the string during debugging.

#### `fio_string_write_unescape`

```c
int fio_string_write_unescape(fio_str_info_s *dest,
                              fio_string_realloc_fn reallocate,
                              const void *src,
                              size_t len);
```

Writes an escaped data into the string after un-escaping the data.

### Core String Base64 support

#### `fio_string_write_base64enc`

```c
SFUNC int fio_string_write_base64enc(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *data,
                                     size_t data_len,
                                     uint8_t url_encoded);
```

Writes data to String using Base64 encoding.

#### `fio_string_write_base64dec`

```c
SFUNC int fio_string_write_base64dec(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *encoded,
                                     size_t encoded_len);
```

Writes decoded base64 data to String.

### Core String URL escaping support

#### `fio_string_write_url_enc`

```c
int fio_string_write_url_enc(fio_str_info_s *restrict dest,
                             fio_string_realloc_fn reallocate,
                             const void *raw,
                             size_t len);
```

Writes data to String using URL encoding (a.k.a., percent encoding). Always encodes spaces as `%20` rather than `+`.

#### `fio_string_write_url_dec`

```c
int fio_string_write_url_dec(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             const void *encoded,
                             size_t len);
```

Writes decoded URL data to String. Decodes "percent encoding" as well as spaces encoded using `+`.

**Note**: the decoding function reads the non-standard `"%uXXXX"` as UTF-8 encoded data.

#### `fio_string_write_path_dec`

```c
int fio_string_write_url_dec(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             const void *encoded,
                             size_t len);
```

Writes decoded URL data to String. Decodes "percent encoding" without converting `+` to spaces.

**Note**: the decoding function reads the non-standard `"%uXXXX"` as UTF-8 encoded data.

### Core String HTML escaping support

#### `fio_string_write_html_escape`

```c
int fio_string_write_html_escape(fio_str_info_s *restrict dest,
                                 fio_string_realloc_fn reallocate,
                                 const void *raw,
                                 size_t len);
```

Writes HTML escaped data to a String.

#### `fio_string_write_html_unescape`

```c
int fio_string_write_html_unescape(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const void *escaped,
                                   size_t len);
```

Writes HTML (mostly) un-escaped data to a String.

**Note**:

The un-escaping of HTML content includes a long list of named code-point. This list isn't handled here, instead only numerical and super-basic named code-points are supported.

The supported named code-points include a small group, among them: `&lt`, `&gt`, `&amp`, `&tab`, `&quot`, `&apos`, `&nbsp`, `&copy` (with or without a trailing `;`).

### Core String File Reading support

#### `fio_string_readfd`

```c
int fio_string_readfd(fio_str_info_s *dest,
                      fio_string_realloc_fn reallocate,
                      int fd,
                      intptr_t start_at,
                      intptr_t limit);
```

Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at`.

If `limit` is 0 (or less than 0) data will be written until EOF.

If `start_at` is negative, position will be calculated from the end of the file where `-1 == EOF`.

Note: this will fail unless used on actual files (not sockets, not pipes).

#### `fio_string_readfile`

```c
int fio_string_readfile(fio_str_info_s *dest,
                        fio_string_realloc_fn reallocate,
                        const char *filename,
                        intptr_t start_at,
                        intptr_t limit);
```

Opens the file `filename` and pastes it's contents (or a slice ot it) at the end of the String. If `limit == 0`, than the data will be read until EOF.

If the file can't be located, opened or read, or if `start_at` is beyond the EOF position, NULL is returned in the state's `data` field.


#### `fio_string_getdelim_fd`

```c
int fio_string_getdelim_fd(fio_str_info_s *dest,
                          fio_string_realloc_fn reallocate,
                          int fd,
                          intptr_t start_at,
                          char delim,
                          size_t limit);
```

Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at` and ending either at the first occurrence of `delim` or at EOF.

If `limit` is 0 (or less than 0) as much data as may be required will be written.

If `start_at` is negative, position will be calculated from the end of the file where `-1 == EOF`.

**Note**: this will fail unless used on actual seekable files (not sockets, not pipes).

#### `fio_string_getdelim_file`

```c
int fio_string_getdelim_file(fio_str_info_s *dest,
                            fio_string_realloc_fn reallocate,
                            const char *filename,
                            intptr_t start_at,
                            char delim,
                            size_t limit);
```

Opens the file `filename`, calls `fio_string_getdelim_fd` and closes the file.

-------------------------------------------------------------------------------

## C Strings with Binary Data

The facil.io C STL provides a very simple String library (`fio_bstr`) that wraps around the *Binary Safe Core String Helpers*, emulating (to some effect and degree) the behavior of the famous [Simple Dynamic Strings library](https://github.com/antirez/sds) while providing copy-on-write reference counting.

This String storage paradigm can be very effective and it is used as the default String key implementation in Maps when `FIO_MAP_KEY` is undefined.

To create a new String simply write to `NULL` and a new `char *` pointer will be returned, pointing to the first byte of the new string.

All `fio_bstr` functions that mutate the string return a pointer to the new string (**make sure to update the pointer!**).

The pointer should be freed using `fio_bstr_free`. i.e.:

```c
char * str = fio_bstr_write(NULL, "Hello World!", 12);
fprintf(stdout, "%s\n", str);
fio_bstr_free(str);
```

To copy a `fio_bstr` String use `fio_bstr_copy` - this uses a *copy-on-write* approach which can increase performance:

```c
char * str_org = fio_bstr_write(NULL, "Hello World", 11);
char * str_cpy = fio_bstr_copy(str_org);   /* str_cpy == str_org : only a reference count increase. */
str_cpy = fio_bstr_write(str_cpy, "!", 1); /* str_cpy != str_org : copy-on-write, data copied here. */
fprintf(stdout, "Original:    %s\nEdited Copy: %s\n", str_org, str_cpy);
fio_bstr_free(str_org);
fio_bstr_free(str_cpy);
```

The `fio_bstr` functions wrap all `fio_string` core API, resulting in the following available functions:

* `fio_bstr_write` - see [`fio_string_write`](#fio_string_write) for details.
* `fio_bstr_write2` (macro) - see [`fio_string_write2`](#fio_string_write2) for details.
* `fio_bstr_printf` - see [`fio_string_printf`](#fio_string_printf) for details.
* `fio_bstr_replace` - see [`fio_string_replace`](#fio_string_replace) for details.

* `fio_bstr_write_i` - see [`fio_string_write_i`](#fio_string_write_i) for details.
* `fio_bstr_write_u` - see [`fio_string_write_u`](#fio_string_write_u) for details.
* `fio_bstr_write_hex` - see [`fio_string_write_hex`](#fio_string_write_hex) for details.
* `fio_bstr_write_bin` - see [`fio_string_write_bin`](#fio_string_write_bin) for details.

* `fio_bstr_write_escape` - see [`fio_string_write_escape`](#fio_string_write_escape) for details.
* `fio_bstr_write_unescape` - see [`fio_string_write_unescape`](#fio_string_write_unescape) for details.

* `fio_bstr_write_base64enc` - see [`fio_string_write_base64enc`](#fio_string_write_base64enc) for details.
* `fio_bstr_write_base64dec` - see [`fio_string_write_base64dec`](#fio_string_write_base64dec) for details.

* `fio_bstr_write_html_escape` - see [`fio_string_write_html_escape`](#fio_string_write_html_escape) for details.
* `fio_bstr_write_html_unescape` - see [`fio_string_write_html_unescape`](#fio_string_write_html_unescape) for details.


* `fio_bstr_readfd` - see [`fio_string_readfd`](#fio_string_readfd) for details.
* `fio_bstr_readfile` - see [`fio_string_readfile`](#fio_string_readfile) for details.
* `fio_bstr_getdelim_fd` - see [`fio_string_getdelim_fd`](#fio_string_getdelim_fd) for details.
* `fio_bstr_getdelim_file` - see [`fio_string_getdelim_file`](#fio_string_getdelim_file) for details.

* `fio_bstr_is_greater` - see [`fio_string_is_greater`](#fio_string_is_greater) for details.

**Note**: the `fio_bstr` functions do not take a `reallocate` argument and their `dest` argument should be the existing `fio_bstr` pointer (`char *`).

**Note**: the `fio_bstr` functions might fail quietly if memory allocation fails. For better error handling use the `fio_bstr_info`, `fio_bstr_reallocate` and `fio_bstr_len_set` functions with the Core String API (the `.buf` in the `fio_str_info_s` struct is the `fio_bstr` pointer).

In addition, the following helpers are provided:

#### `fio_bstr_copy`

```c
char *fio_bstr_copy(char *bstr);
```

Returns a Copy-on-Write copy of the original `bstr`, increasing the original's reference count.

This approach to Copy-on-Write is **not** completely thread-safe, as a data race exists when an original string (without additional `copy`s) is being actively edited by another thread while `copy` is being called (in which case the still-in-process `write` will apply to the new copy, which may result in a re-allocation and the pointer `copy` is using being invalidated).

However, once the copy operation had completed in a thread-safe manner, further operations between the two instances do not need to be synchronized.

**Note**: This reference counter will automatically make a copy if more than 2 billion (2,147,483,648) references are counted.

**Note**: To avoid the Copy-on-Write logic, use:

```c
char *copy = fio_bstr_write(NULL, original, fio_bstr_len(original));
```

#### `fio_bstr_free`

```c
void fio_bstr_free(char *bstr);
```

Frees a binary string allocated by a `fio_bstr` function (or decreases its reference count).

#### `fio_bstr_info`

```c
fio_str_info_s fio_bstr_info(char *bstr);
```

Returns information about the `fio_bstr` using the `fio_str_info_s` struct.

#### `fio_bstr_buf`

```c
fio_buf_info_s fio_bstr_buf(char *bstr);
```

Returns information about the `fio_bstr` using the `fio_buf_info_s` struct.

#### `fio_bstr_len`

```c
size_t fio_bstr_len(char *bstr);
```

Gets the length of the `fio_bstr`.

#### `fio_bstr_len_set`

```c
char *fio_bstr_len_set(char *bstr, size_t len);
```

Sets the length of the `fio_bstr`.

**Note**: `len` **must** be less then the capacity of the `bstr`, or the function call will quietly fail.

Returns `bstr`.

#### `fio_bstr_reallocate` - for internal use

```c
int fio_bstr_reallocate(fio_str_info_s *dest, size_t len);
```

Default reallocation callback implementation. The new `fio_bstr` pointer will replace the old one in `dest->buf`.

-------------------------------------------------------------------------------

## Small Key Strings for Maps - Binary Safe

It is very common for Hash Maps to contain String keys. When the String keys are usually short, than it could be more efficient to embed the Key String data into the map itself (improve cache locality) rather than allocate memory for each separate Key String.

The `fio_keystr_s` type included with the String Core performs exactly this optimization. When the majority of the Strings are short (`len <= 14` on 64 bit machines or `len <= 10` on 32 bit machines) than the strings are stored inside the Map's memory rather than allocated separately.

See example at the end of this section. The example shows how to use the `fio_keystr_s` type and the `FIO_MAP_KEY_KSTR` MACRO.

#### `FIO_MAP_KEY_STR`

A helper macro for defining Key String keys in Hash Maps.

#### `fio_keystr_s`

```c
typedef struct fio_keystr_s fio_keystr_s;
```

a semi-opaque type used for the `fio_keystr` functions

#### `fio_keystr_buf`

```c
fio_buf_info_s fio_keystr_buf(fio_keystr_s *str);
```

Returns the Key String.

#### `fio_keystr`

```c
fio_keystr_s fio_keystr_tmp(const char *buf, uint32_t len);
```

Returns a **temporary** `fio_keystr_s` to be used as a key for a hash map.

Do **not** `fio_keystr_destroy` this key.

#### `fio_keystr_init`

```c
fio_keystr_s fio_keystr_init(fio_str_info_s str, void *(*alloc_func)(size_t len)) 
```

Returns a copy of `fio_keystr_s` - used internally by the hash map.

**Note**: when `.capa == FIO_KEYSTR_CONST` then the new `fio_keystr_s` will most likely point to the original pointer (which much remain valid in memory for the lifetime of the key string). Short enough strings are always copied to allow for improved cache locality.

#### `fio_keystr_destroy`

```c
void fio_keystr_destroy(fio_keystr_s *key, void (*free_func)(void *, size_t));
```

Destroys a copy of `fio_keystr_s` - used internally by the hash map.

#### `fio_keystr_is_eq`

```c
int fio_keystr_is_eq(fio_keystr_s a, fio_keystr_s b);
int fio_keystr_is_eq2(fio_keystr_s a, fio_str_info_s b);
int fio_keystr_is_eq3(fio_keystr_s a, fio_buf_info_s b);
```

Compares two Key Strings - used internally by the hash map.

#### `FIO_KEYSTR_CONST`

```c
#define FIO_KEYSTR_CONST ((size_t)-1LL)
```

### `fio_keystr` Example

This example maps words to numbers. Note that this will work also with binary data and dynamic strings.

```c
/* map words to numbers. */
#define FIO_MAP_KEY_KSTR
#define FIO_UMAP_NAME umap
#define FIO_MAP_VALUE uintptr_t
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)&umap_destroy)
#include "fio-stl/include.h" /* or "fio-stl.h" */

/* example adding strings to map and printing data. */
void map_keystr_example(void) {
  umap_s map = FIO_MAP_INIT;
  /* FIO_KEYSTR_CONST prevents copying of longer constant strings */
  umap_set(&map, FIO_STR_INFO3("One", 3, FIO_KEYSTR_CONST), 1, NULL);
  umap_set(&map, FIO_STR_INFO3("Two", 3, FIO_KEYSTR_CONST), 2, NULL);
  umap_set(&map, FIO_STR_INFO3("Three", 5, FIO_KEYSTR_CONST), 3, NULL);
  FIO_MAP_EACH(umap, &map, pos) {
    uintptr_t value = pos.value;
    printf("%.*s: %llu\n",
           (int)pos.key.len,
           pos.key.buf,
           (unsigned long long)value);
  }
  umap_destroy(&map);
}
```
-------------------------------------------------------------------------------
## ChaCha20 & Poly1305

```c
#define FIO_CHACHA
#include "fio-stl.h"
```

Non-streaming ChaCha20 and Poly1305 implementations are provided for cases when a cryptography library isn't available (or too heavy) but a good enough symmetric cryptographic solution is required. Please note that this implementation was not tested from a cryptographic viewpoint and although constant time was desired it might not have been achieved on all systems / CPUs.

**Note:** some CPUs do not offer constant time MUL and might leak information through side-chain attacks.

**Note:** this module depends on the `FIO_MATH` module which will be automatically included.

#### `fio_chacha20_poly1305_enc`

```c
void fio_chacha20_poly1305_enc(void *mac,
                               void *data,
                               size_t len,
                               void *ad, /* additional data */
                               size_t adlen,
                               void *key,
                               void *nonce);
```

Performs an in-place encryption of `data` using ChaCha20 with additional data, producing a 16 byte message authentication code (MAC) using Poly1305.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`  MUST point to a  96 bit long memory address (12 Bytes).
* `ad`     MAY be omitted, will NOT be encrypted.
* `data`   MAY be omitted, WILL be encrypted.
* `mac`    MUST point to a buffer with (at least) 16 available bytes.

#### `fio_chacha20_poly1305_dec`

```c
int fio_chacha20_poly1305_dec(void *mac,
                              void *data,
                              size_t len,
                              void *ad, /* additional data */
                              size_t adlen,
                              void *key,
                              void *nonce);
```

Performs an in-place decryption of `data` using ChaCha20 after authenticating the message authentication code (MAC) using Poly1305.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`  MUST point to a  96 bit long memory address (12 Bytes).
* `ad`     MAY be omitted ONLY IF originally omitted.
* `data`   MAY be omitted, WILL be decrypted.
* `mac`    MUST point to a buffer where the 16 byte MAC is placed.

Returns `-1` on error (authentication failed).

#### `fio_chacha20`

```c
void fio_chacha20(void *data,
                  size_t len,
                  void *key,
                  void *nonce,
                  uint32_t counter);
```

Performs an in-place encryption/decryption of `data` using ChaCha20.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce` MUST point to a  96 bit long memory address (12 Bytes).
* `counter` is the block counter, usually 1 unless `data` is mid-cyphertext.


#### `fio_poly1305_auth`

```c
void fio_poly1305_auth(void *mac_dest,
                       void *key256bits,
                       void *message,
                       size_t len,
                       void *additional_data,
                       size_t additional_data_len);
```

Given a Poly1305 256bit (32 byte) key, writes the Poly1305 authentication code for the message and additional data into `mac_dest`.

* `mac_dest`      MUST point to a 128 bit long memory address (16 Bytes).
* `key256bits`    MUST point to a 256 bit long memory address (32 Bytes).

-------------------------------------------------------------------------------
## SHA1

```c
#define FIO_SHA1
#include FIO_INCLUDE_FILE
```

By defining the `FIO_SHA1`, the SHA1 a (broken) Cryptographic Hash functions will be defined and made available.

Do **not** use SHA1 for security concerns, it's broken and hopefully future cryptographic libraries won't include it in their packages... however, for some reason, some protocols require SHA1 (i.e., WebSockets).

#### `fio_sha1`

```c
fio_sha1_s fio_sha1(const void *data, uint64_t len);
```

A simple, non streaming, implementation of the SHA1 hashing algorithm.

#### `fio_sha1_len`

```c
size_t fio_sha1_len(void);
```

Returns the digest length of SHA1 in bytes (which is always 20).

#### `fio_sha1_digest`

```c
uint8_t *fio_sha1_digest(fio_sha1_s *s);
```

Returns the digest of a SHA1 object. The digest is always 20 bytes long.

-------------------------------------------------------------------------------
## SHA1

```c
#define FIO_SHA2
#include FIO_INCLUDE_FILE
```

By defining the `FIO_SHA2`, the SHA2 Cryptographic Hash functions will be defined and made available.


#### `fio_sha256_s`

```c
typedef struct {
  fio_u256 hash;
  fio_u512 cache;
  uint64_t total_len;
} fio_sha256_s;
```

Streaming SHA-256 type.

#### `fio_sha512_s`

```c
typedef struct {
  fio_u512 hash;
  fio_u1024 cache;
  uint64_t total_len;
} fio_sha512_s;
```

Streaming SHA-512 type.

##### `fio_sha256`
```c
fio_u256 fio_sha256(const void *data, uint64_t len);
```

A simple, non streaming, implementation of the SHA-256 hashing algorithm.

##### `fio_sha256_init`
```c
fio_sha256_s fio_sha256_init(void);
```

Initializes a fio_u256 so the hash can consume streaming data.

##### `fio_sha256_consume`
```c
void fio_sha256_consume(fio_sha256_s *h, const void *data, uint64_t len);
```

Feed data into the hash

##### `fio_sha256_finalize`
```c
fio_u256 fio_sha256_finalize(fio_sha256_s *h);
```

Finalizes a fio_u256 with the SHA 256 hash.

##### `fio_sha512`
```c
fio_u512 fio_sha512(const void *data, uint64_t len);
```

A simple, non streaming, implementation of the SHA-512 hashing algorithm.

##### `fio_sha512_init`
```c
fio_sha512_s fio_sha512_init(void);
```

Initializes a fio_u512 so the hash can consume streaming data.

##### `fio_sha512_consume`
```c
void fio_sha512_consume(fio_sha512_s *h, const void *data, uint64_t len);
```

Feed data into the hash

##### `fio_sha512_finalize`
```c
fio_u512 fio_sha512_finalize(fio_sha512_s *h);
```

Finalizes a fio_u512 with the SHA 512 hash.


-------------------------------------------------------------------------------
## OTP

```c
#define FIO_OTP
#include FIO_INCLUDE_FILE
```

By defining the `FIO_OTP`, a small T-OTP (Time based One Time Password) helper API will be provided. This requires the `FIO_SHA1` and `FIO_STR` modules.

The following helper functions are defined:

#### `fio_otp_generate_key`

```c
fio_u128 fio_otp_generate_key(void);
```

Generates a random 128 bit key for TOTP processing.

Random keys may be required when generating a new OTP secret for possible logins.

#### `fio_otp_print_key`

```c
size_t fio_otp_print_key(char *dest, uint8_t *key, size_t len)
```

Prints out an OTP secret (big endian number) as a Byte32 encoded String.

Printing out the OTP secret can be important when providing it to authentication apps.

#### `fio_otp`

```c
uint32_t fio_otp(fio_buf_info_s secret, fio_otp_settings_s settings);
#define fio_otp(secret, ...) fio_otp(secret, (fio_otp_settings_s){__VA_ARGS__})
```

Returns a TOTP based on `secret` and the OTP settings.

This can be used to either validate an existing TOTP or generate a new one.

Possible settings include:

```c
typedef struct {
  /** The time interval for TOTP rotation. */
  size_t interval; /* defaults to 30 == Google OTP */
  /** The number of digits in the OTP. */
  size_t digits; /* defaults to 6 == Google OTP */
  /** The time offset (in `interval` units) from the current time. */
  int64_t offset; /* defaults to 0 == now */
  /** Set to true if the secret / key is is Hex instead of Byte32 encoding. */
  uint8_t is_hex;
} fio_otp_settings_s;
```

Example:

```c
uint32_t totp_now = fio_otp(FIO_BUF_INFO1("My Secret"), .offset = 0);
uint32_t previous = fio_otp(FIO_BUF_INFO1("My Secret"), .offset = 1);
```

-------------------------------------------------------------------------------
## Secrets

```c
#define FIO_SECRET
#include FIO_INCLUDE_FILE
```

By defining the `FIO_SECRET`, a small secret helper API will be provided. This requires the `FIO_SHA2` and `FIO_ATOL` modules.

### Keeping Secrets

Secrets should be kept secret. 

Secrets should **not** be kept in source files, which too often end up end up exposed.

Secrets should **not** be logged (such as in case of a crash or a core dump).

Additionally, secrets should **not** be used directly if possible. It is better to use a hashed value of the secret, possible with some time based salt or spice. This way, if somehow information leaks regarding the secret, what is exposed is actually the hashed value and not the secret itself.

For this reason, the most common place to place a secret is as a hashed value in the OS environment (often as a Hex encoded String).

**Note**: some secrets, such as TLS certificates, are often stored as system files somewhere separate from the source code.

To help with managing a program wide secret, the following helper functions are defined:


#### `fio_secret_set_at`

```c
void fio_secret_set_at(fio_u512 *secret, char *str, size_t len);
```

Sets a (possibly shared) secret and stores its (masked) SHA512 hash in `secret`.

**Note**: the SHA512 hash in `secret` is masked before it is stored, so that the final secret isn't logged in case of a core dump.

#### `fio_secret_at`

```c
fio_u512 fio_secret_at(fio_u512 *secret);
```

Gets the SHA512 of a (possibly shared) masked secret stored in `secret`.

Please store the returned value on the stack or not at all. The secret is stored masked in memory and unmasked copies should be temporary with short life-spans.

#### `fio_secret`

```c
fio_u512 fio_secret(void);
```

Returns the SHA512 of a (possibly shared) secret.

Unless updated using `fio_secret_set`, this is either a random secret or the one derived from the `SECRET` environment variable.

Please store the returned value on the stack or not at all. The secret is stored masked in memory and unmasked copies should be temporary with short life-spans.

#### `fio_secret_set`

```c
void fio_secret_set(char *secret, size_t len, bool is_random);
```

Sets a (possibly shared) secret and stores its SHA512 hash.

If `secret` is Hex encoded, it will be decoded before it is hashed and white spaces will be ignored.

**Note**: the SHA512 hash is masked before it is stored, so that the final secret isn't logged in case of a core dump.


#### `fio_secret_is_random`

```c
bool fio_secret_is_random(void);
```

Returns true if the secret was randomly generated.


-------------------------------------------------------------------------------
## Dynamic Strings

```c
#define FIO_STR_NAME fio_str
#include "fio-stl.h"
```

Dynamic Strings are extremely useful, since:

* The can safely store binary data (unlike regular C strings).

* They make it easy to edit String data. Granted, the standard C library can do this too, but this approach offers some optimizations and safety measures that the C library cannot offer due to it's historical design.

To create a dynamic string define the type name using the `FIO_STR_NAME` macro.

Alternatively, the type name could be defined using the `FIO_STR_SMALL` macro, resulting in an alternative data structure with a non-default optimization approach (see details later on).

The type (`FIO_STR_NAME_s`) and the functions will be automatically defined.

For brevities sake, in this documentation they will be listed as `STR_*` functions / types (i.e., `STR_s`, `STR_new()`, etc').

**Note:** this module depends on the  `FIO_STR` module and all the modules required by `FIO_STR` which will be automatically included.

### Optimizations / Flavors

Strings come in two main flavors, Strings optimized for mutability (default) vs. Strings optimized for memory consumption (defined using `FIO_STR_SMALL`).

Both optimizations follow specific use-case performance curves that depend on the length of the String data and effect both editing costs and reading costs differently.

#### When to use the default Dynamic Strings (`FIO_STR_NAME`)

The default optimization stores information about the allocated memory's capacity and it is likely to perform best for most generic use-cases, especially when:

* Multiple `write` operations are required.

* It's pre-known that most strings will be longer than a small container's embedded string limit (`(2 * sizeof(char*)) - 2`) and still fit within the default container's embedded string limit (`((4 + FIO_STR_OPTIMIZE_EMBEDDED) * sizeof(char*)) - 2`).

   This is because short Strings are stored directly within a String's data container, minimizing both memory indirection and memory allocation.

   Strings optimized for mutability, by nature, have a larger data container, allowing longer strings to be stored within a container.

   For example, _on 64bit systems_:

   The default (larger) container requires 32 bytes, allowing Strings of up to 30 bytes to be stored directly within the container. This is in contrast to the smaller container (16 bytes in size).

   Two bytes (2 bytes) are used for metadata and a terminating NUL character (to ensure C string safety), leaving the embedded string capacity at 30 bytes for the default container (and 14 bytes for the small one).

   If it's **pre-known** that most strings are likely to be longer than 14 bytes and shorter than 31 bytes (on 64 bit systems), than the default `FIO_STR_NAME` optimization should perform better.

   **Note**: the default container size can be extended by `sizeof(void*)` units using the `FIO_STR_OPTIMIZE_EMBEDDED` macro (i.e., `#define FIO_STR_OPTIMIZE_EMBEDDED 2` will add 16 bytes to the container on 64 bit systems).

#### Example `FIO_STR_NAME` Use-Case

```c
#define FIO_LOG
#define FIO_QUEUE
#include "fio-stl.h"

#define FIO_STR_NAME fio_str
#define FIO_REF_NAME fio_str
#define FIO_REF_CONSTRUCTOR_ONLY
#include "fio-stl.h"

/* this is NOT thread safe... just an example */
void example_task(void *str_, void *ignore_) {
  fio_str_s *str = (fio_str_s *)str_; /* C++ style cast */
  fprintf(stderr, "%s\n", fio_str_ptr(str));
  fio_str_write(str, ".", 1); /* write will sporadically allocate memory if required. */
  fio_str_free(str);          /* decreases reference count or frees object */
  (void)ignore_;
}

void example(void) {
  fio_queue_s queue = FIO_QUEUE_INIT(queue);
  fio_str_s *str = fio_str_new();
  /* writes to the String */
  fio_str_write(str, "Starting time was: ", 19);
  {
    /* reserves space and resizes String, without writing any data */
    const size_t org_len = fio_str_len(str);
    fio_str_info_s str_info = fio_str_resize(str, 29 + org_len);
    /* write data directly to the existing String buffer */
    size_t r = fio_time2rfc7231(str_info.buf + org_len, fio_time_real().tv_sec);
    FIO_ASSERT(r == 29, "this example self destructs at 9999");
  }
  for (size_t i = 0; i < 10; ++i) {
    /* allow each task to hold a reference to the object */
    fio_queue_push(&queue, .fn = example_task, .udata1 = fio_str_dup(str));
  }
  fio_str_free(str);             /* decreases reference count */
  fio_queue_perform_all(&queue); /* performs all tasks */
  fio_queue_destroy(&queue);
}
```

#### When to use the smaller Dynamic Strings (`FIO_STR_SMALL`)

The classic use-case for the smaller dynamic string type is as a `key` in a Map object. The memory "savings" in these cases could become meaningful.

In addition, the `FIO_STR_SMALL` optimization is likely to perform better than the default when Strings are likely to fit within a small container's embedded string limit (`(2 * sizeof(char*)) - 2`), or when Strings are mostly immutable and likely to be too long for the default container's embedded string limit, **and**:

* Strings are likely to require a single `write` operation; **or**

* Strings will point to static memory (`STR_init_const`).

#### Example `FIO_STR_SMALL` Use-Case

```c
#define FIO_STR_SMALL key /* results in the type name: key_s */
#include "fio-stl.h"

#define FIO_OMAP_NAME map
#define FIO_MAP_KEY key_s /* the small string type */
#define FIO_MAP_KEY_COPY(dest, src) key_init_copy2(&(dest), &(src))
#define FIO_MAP_KEY_DESTROY(k) key_destroy(&k)
#define FIO_MAP_KEY_CMP(a, b) key_is_eq(&(a), &(b))
#define FIO_MAP_VALUE uintptr_t
#include "fio-stl.h"

/* helper for setting values in the map using risky hash with a safe seed */
FIO_IFUNC uintptr_t map_set2(map_s *m, key_s key, uintptr_t value) {
  return map_set(m, key_hash(&key, (uintptr_t)m), key, value, NULL);
}

/* helper for getting values from the map using risky hash with a safe seed */
FIO_IFUNC uintptr_t map_get2(map_s *m, key_s key) {
  return map_get(m, key_hash(&key, (uintptr_t)m), key);
}

void example(void) {
  map_s m = FIO_MAP_INIT;
  /* write the long keys twice, to prove they self-destruct in the Hash-Map */
  for (size_t overwrite = 0; overwrite < 2; ++overwrite) {
    for (size_t i = 0; i < 10; ++i) {
      const char *prefix = "a long key will require memory allocation: ";
      key_s k;
      key_init_const(&k, prefix, strlen(prefix)); /* points to string literal */
      key_write_hex(&k, i); /* automatically converted into a dynamic string */
      map_set2(&m, k, (uintptr_t)i);
      key_destroy(&k);
    }
  }
  /* short keys don't allocate external memory (string embedded in the object) */
  for (size_t i = 0; i < 10; ++i) {
    /* short keys fit in pointer + length type... test assumes 64bit addresses */
    const char *prefix = "embed: ";
    key_s k;
    key_init_const(&k, prefix, strlen(prefix)); /* embeds the (short) string */
    key_write_hex(&k, i); /* automatically converted into a dynamic string */
    map_set2(&m, k, (uintptr_t)i);
    key_destroy(&k);
  }
  FIO_MAP_EACH(&m, pos) {
    fprintf(stderr,
            "[%d] %s - memory allocated: %s\n",
            (int)pos->obj.value,
            key_ptr(&pos->obj.key),
            (key_is_allocated(&pos->obj.key) ? "yes" : "no"));
  }
  map_destroy(&m);
  /* test for memory leaks using valgrind or similar */
}
```
### String Type information

#### `STR_s`

The core type, created by the macro, is the `STR_s` type - where `STR` is replaced by `FIO_STR_NAME`. i.e.:

```c
#define FIO_STR_NAME my_str
#include <fio-stl.h>
// results in: my_str_s - i.e.:
void hello(void){
  my_str_s msg = FIO_STR_INIT;
  my_str_write(&msg, "Hello World", 11);
  printf("%s\n", my_str_ptr(&msg));
  my_str_destroy(&msg);
}
```

The type should be considered **opaque** and **must never be accessed directly**.

The type's attributes should be accessed ONLY through the accessor functions: `STR_info`, `STR_len`, `STR_ptr`, `STR_capa`, etc'.

This is because: Small strings that fit into the type directly use the type itself for memory (except the first and last bytes). Larger strings use the type fields for the string's meta-data. Depending on the string's data, the type behaves differently.

#### `fio_str_info_s` - revisited

Some functions return information about a string's state using the [`fio_str_info_s` type detailed above](#fio_str_info_s). As a reminder, it looks like this:

```c
typedef struct fio_str_info_s {
  char *buf;   /* The string's buffer (pointer to first byte) or NULL on error. */
  size_t len;  /* The string's length, if any. */
  size_t capa; /* The buffer's capacity. Zero (0) indicates the buffer is read-only. */
} fio_str_info_s;
```

This information type, accessible using the `STR_info` function, allows direct access and manipulation of the string data. Changes in string length should be followed by a call to `STR_resize`.

The data in the string object is always NUL terminated. However, string data might contain binary data, where NUL is a valid character, so using C string functions isn't advised.

Equality can be tested using the [`FIO_STR_INFO_IS_EQ` macro](FIO_STR_INFO_IS_EQ).

See [Binary Data Informational Types and Helpers](#binary-data-informational-types-and-helpers) for more details.

#### String allocation alignment / `FIO_STR_NO_ALIGN`

Memory allocators have allocation alignment concerns that require minimum space to be allocated.

The default `STR_s` type makes use of this extra space for small strings, fitting them into the type.

To prevent this behavior and minimize the space used by the `STR_s` type, set the `FIO_STR_NO_ALIGN` macro to `1`.

```c
#define FIO_STR_NAME big_string
#define FIO_STR_NO_ALIGN 1
#include <fio-stl.h>
// ...
big_string_s foo = FIO_STR_INIT;
```

This could save memory when strings aren't short enough to be contained within the type.

This could also save memory, potentially, if the string type will be wrapped / embedded within other data-types (i.e., using `FIO_REF_NAME` for reference counting).

### String API - Initialization and Destruction

#### `FIO_STR_INIT`

This value should be used for initialization. It should be considered opaque, but is defined as:

```c
#define FIO_STR_INIT { .special = 1 }
```

For example:

```c
#define FIO_STR_NAME fio_str
#include <fio-stl.h>
void example(void) {
  // on the stack
  fio_str_s str = FIO_STR_INIT;
  // .. 
  fio_str_destroy(&str);
}
```

#### `FIO_STR_INIT_EXISTING`

This macro allows the container to be initialized with existing data.

```c
#define FIO_STR_INIT_EXISTING(buffer, length, capacity,)              \
  { .buf = (buffer), .len = (length), .capa = (capacity) }
```
The `capacity` value should exclude the space required for the NUL character (if exists).

Memory should be dynamically allocated using the same allocator selected for the String type (see `FIO_MALLOC` / `FIO_MEM_REALLOC` / `FIO_MEM_FREE`).

#### `FIO_STR_INIT_STATIC`

This macro allows the string container to be initialized with existing static data, that shouldn't be freed.

```c
#define FIO_STR_INIT_STATIC(buffer)                                            \
  { .special = 4, .buf = (char *)(buffer), .len = FIO_STRLEN((buffer)) }
```

#### `FIO_STR_INIT_STATIC2`

This macro allows the string container to be initialized with existing static data, that shouldn't be freed.

```c
#define FIO_STR_INIT_STATIC2(buffer, length)                                   \
  { .buf = (char *)(buffer), .len = (length) }
```


#### `STR_init_const`

```c
fio_str_info_s STR_init_const(FIO_STR_PTR s,
                              const char *str,
                              size_t len);
```

Initializes the container with a pointer to the provided static / constant string.

The string will be copied to the container **only** if it will fit in the container itself. 

Otherwise, the supplied pointer will be used as is **and must remain valid until the string is destroyed** (or written to, at which point the data is duplicated).

The final string can be safely be destroyed (using the `STR_destroy` function).

#### `STR_init_copy`

```c
fio_str_info_s STR_init_copy(FIO_STR_PTR s,
                             const char *str,
                             size_t len);
```

Initializes the container with a copy of the `src` string.

The string is always copied and the final string must be destroyed (using the `destroy` function).

#### `STR_init_copy2`

```c
fio_str_info_s STR_init_copy2(FIO_STR_PTR dest,
                             FIO_STR_PTR src);
```

Initializes the `dest` container with a copy of the `src` String object's content.

The `src` metadata, such as `freeze` state, is ignored - resulting in a mutable String object.

The string is always copied and the final string must be destroyed (using the `destroy` function).

#### `STR_destroy`

```c
void STR_destroy(FIO_STR_PTR s);
```

Frees the String's resources and reinitializes the container.

Note: if the container isn't allocated on the stack, it should be freed separately using the appropriate `free` function, such as `STR_free`.

#### `STR_new`

```c
FIO_STR_PTR STR_new(void);
```

Allocates a new String object on the heap.

#### `STR_free`

```c
void STR_free(FIO_STR_PTR s);
```

Destroys the string and frees the container (if allocated with `STR_new`).

#### `STR_detach`

```c
char * STR_detach(FIO_STR_PTR s);
```

Returns a C string with the existing data, **re-initializing** the String.

The returned C string is **always dynamic** and **must be freed** using the same memory allocator assigned to the type (i.e., `free` or `fio_free`, see [`FIO_MALLOC`](#local-memory-allocation), [`FIO_MEM_REALLOC`](#FIO_MEM_REALLOC) and [`FIO_MALLOC_TMP_USE_SYSTEM`](#FIO_MALLOC_TMP_USE_SYSTEM))

**Note**: the String data is removed from the container, but the container is **not** freed.

Returns NULL if there's no String data.

#### `STR_dealloc`
```c
void FIO_NAME(FIO_STR_NAME, dealloc)(void *ptr);
```

Frees the pointer returned by [`detach`](#str_detach).

**Note**: this might cause memory leaks if the `size` in the [`FIO_MEM_FREE` macro](#fio_mem_free) was a required parameter, as this function will always use `size == -1` (since the information about the size was lost during `detach`.

### String API - String state (data pointers, length, capacity, etc')

#### `STR_info`

```c
fio_str_info_s STR_info(const FIO_STR_PTR s);
```

Returns the String's complete state (capacity, length and pointer). 

#### `STR_len`

```c
size_t STR_len(FIO_STR_PTR s);
```

Returns the String's length in bytes.

#### `STR_ptr`

```c
char *STR_ptr(FIO_STR_PTR s);
```

Returns a pointer (`char *`) to the String's content (first character in the string).

#### `STR_capa`

```c
size_t STR_capa(FIO_STR_PTR s);
```

Returns the String's existing capacity (total used & available memory).

#### `STR_freeze`

```c
void STR_freeze(FIO_STR_PTR s);
```

Prevents further manipulations to the String's content.

#### `STR_is_frozen`

```c
uint8_t STR_is_frozen(FIO_STR_PTR s);
```

Returns true if the string is frozen.

#### `STR_is_allocated`

```c
int STR_is_allocated(const FIO_STR_PTR s);
```

Returns 1 if memory was allocated and (the String must be destroyed).

#### `STR_is_eq`

```c
int STR_is_eq(const FIO_STR_PTR str1, const FIO_STR_PTR str2);
```

Binary comparison returns `1` if both strings are equal and `0` if not.

#### `STR_hash`

```c
uint64_t STR_hash(const FIO_STR_PTR s);
```

Returns the string's Risky Hash value.

Note: Hash algorithm might change without notice.

### String API - Memory management

#### `STR_resize`

```c
fio_str_info_s STR_resize(FIO_STR_PTR s, size_t size);
```

Sets the new String size without reallocating any memory (limited by existing capacity).

Returns the updated state of the String.

Note: When shrinking, any existing data beyond the new size may be corrupted or lost.

#### `STR_compact`

```c
void STR_compact(FIO_STR_PTR s);
```

Performs a best attempt at minimizing memory consumption.

Actual effects depend on the underlying memory allocator and it's implementation. Not all allocators will free any memory.

#### `STR_reserve`

```c
fio_str_info_s STR_reserve(FIO_STR_PTR s, size_t amount);
```

Reserves at least `amount` of new bytes to be added to the string's data.

Returns the current state of the String.

**Note**: Doesn't exist for `FIO_STR_SMALL` types, since capacity can't be reserved in advance (either use `STR_resize` and write data manually or suffer a performance penalty when performing multiple `write` operations).

### String API - UTF-8 State

#### `STR_utf8_valid`

```c
size_t STR_utf8_valid(FIO_STR_PTR s);
```

Returns 1 if the String is UTF-8 valid and 0 if not.

#### `STR_utf8_len`

```c
size_t STR_utf8_len(FIO_STR_PTR s);
```

Returns the String's length in UTF-8 characters.

#### `STR_utf8_select`

```c
int STR_utf8_select(FIO_STR_PTR s, intptr_t *pos, size_t *len);
```

Takes a UTF-8 character selection information (UTF-8 position and length) and updates the same variables so they reference the raw byte slice information.

If the String isn't UTF-8 valid up to the requested selection, than `pos` will be updated to `-1` otherwise values are always positive.

The returned `len` value may be shorter than the original if there wasn't enough data left to accommodate the requested length. When a `len` value of `0` is returned, this means that `pos` marks the end of the String.

Returns -1 on error and 0 on success.

### String API - Content Manipulation and Review

#### `STR_write`

```c
fio_str_info_s STR_write(FIO_STR_PTR s, const void *src, size_t src_len);
```

Writes data at the end of the String.

#### `STR_write_i`

```c
fio_str_info_s STR_write_i(FIO_STR_PTR s, int64_t num);
```

Writes a number at the end of the String using normal base 10 notation.

#### `STR_write_hex`

```c
fio_str_info_s STR_write_hex(FIO_STR_PTR s, int64_t num);
```

Writes a number at the end of the String using Hex (base 16) notation.

**Note**: the `0x` prefix **is automatically written** before the hex numerals.

#### `STR_concat` / `STR_join`

```c
fio_str_info_s STR_concat(FIO_STR_PTR dest, FIO_STR_PTR const src);
```

Appends the `src` String to the end of the `dest` String. If `dest` is empty, the resulting Strings will be equal.

`STR_join` is an alias for `STR_concat`.


#### `STR_replace`

```c
fio_str_info_s STR_replace(FIO_STR_PTR s,
                           intptr_t start_pos,
                           size_t old_len,
                           const void *src,
                           size_t src_len);
```

Replaces the data in the String - replacing `old_len` bytes starting at `start_pos`, with the data at `src` (`src_len` bytes long).

Negative `start_pos` values are calculated backwards, `-1` == end of String.

When `old_len` is zero, the function will insert the data at `start_pos`.

If `src_len == 0` than `src` will be ignored and the data marked for replacement will be erased.

#### `STR_vprintf`

```c
fio_str_info_s STR_vprintf(FIO_STR_PTR s, const char *format, va_list argv);
```

Writes to the String using a vprintf like interface.

Data is written to the end of the String.

#### `STR_printf`

```c
fio_str_info_s STR_printf(FIO_STR_PTR s, const char *format, ...);
```

Writes to the String using a printf like interface.

Data is written to the end of the String.

#### `STR_readfd`

```c
fio_str_info_s STR_readfd(FIO_STR_PTR s,
                            int fd,
                            intptr_t start_at,
                            intptr_t limit);
```

Reads data from a file descriptor `fd` at offset `start_at` and pastes it's contents (or a slice of it) at the end of the String. If `limit == 0`, than the data will be read until EOF.

The file should be a regular file or the operation might fail (can't be used for sockets).

**Note**: the file descriptor will remain open and should be closed manually.

#### `STR_readfile`

```c
fio_str_info_s STR_readfile(FIO_STR_PTR s,
                            const char *filename,
                            intptr_t start_at,
                            intptr_t limit);
```

Opens the file `filename` and pastes it's contents (or a slice of it) at the end of the String. If `limit == 0`, than the data will be read until EOF.

If the file can't be located, opened or read, or if `start_at` is beyond the EOF position, NULL is returned in the state's `data` field.

### String API - Base64 support

#### `STR_write_base64enc`

```c
fio_str_info_s STR_write_base64enc(FIO_STR_PTR s,
                                    const void *data,
                                    size_t data_len,
                                    uint8_t url_encoded);
```

Writes data at the end of the String, encoding the data as Base64 encoded data.

#### `STR_write_base64dec`

```c
fio_str_info_s STR_write_base64dec(FIO_STR_PTR s,
                                    const void *encoded,
                                    size_t encoded_len);
```

Writes decoded Base64 data to the end of the String.


### String API - escaping / JSON encoding support

#### `STR_write_escape`

```c
fio_str_info_s STR_write_escape(FIO_STR_PTR s,
                                const void *data,
                                size_t data_len);

```

Writes data at the end of the String, escaping the data using JSON semantics.

The JSON semantic are common to many programming languages, promising a UTF-8 String while making it easy to read and copy the string during debugging.

#### `STR_write_unescape`

```c
fio_str_info_s STR_write_unescape(FIO_STR_PTR s,
                                  const void *escaped,
                                  size_t len);
```

Writes an escaped data into the string after unescaping the data.

### String API - HTML escaping support


#### `STR_write_html_escape`

```c
fio_str_info_s STR_write_html_escape(FIO_STR_PTR s,
                                     const void *data,
                                     size_t data_len);

```

Writes HTML escaped data to a String.

#### `STR_write_html_unescape`

```c
fio_str_info_s STR_write_html_unescape(FIO_STR_PTR s,
                                       const void *escaped,
                                       size_t len);
```

Writes HTML un-escaped data to a String - incomplete and minimal.

**Note**: the un-escaping of HTML content includes a long list of named code-point. This list isn't handled here, instead only numerical and super-basic named code-points are supported.

-------------------------------------------------------------------------------
## Dynamic Arrays

```c
#define FIO_ARRAY_NAME str_ary
#define FIO_ARRAY_TYPE char *
#define FIO_ARRAY_TYPE_CMP(a,b) (!strcmp((a),(b)))
#include "fio-stl.h"
```

Dynamic arrays are extremely common and useful data structures.

In essence, Arrays are blocks of memory that contain all their elements "in a row". They grow (or shrink) as more items are added (or removed).

Items are accessed using a numerical `index` indicating the element's position within the array.

Indexes are zero based (first element == 0).

**Note:** The dynamic array implementation provided limits the array's capacity to 31bits ((1<<31) - 1).

### Dynamic Array Performance

Seeking time is an extremely fast O(1). Arrays are also very fast to iterate since they enjoy high memory locality.

Adding and editing items is also a very fast O(1), especially if enough memory was previously reserved. Otherwise, memory allocation and copying will slow performance.

However, arrays suffer from slow find operations. Find has a worst case scenario O(n) cost.

They also suffer from slow item removal (except, in our case, for `pop` / `unshift` operations), since middle-element removal requires memory copying when fixing the "hole" made in the array.

A common solution is to reserve a value for "empty" elements and `set` the element's value instead of `remove` the element.

**Note**: unlike some dynamic array implementations, this STL implementation doesn't grow exponentially. Using the `ARY_reserve` function is highly encouraged for performance.


### Dynamic Array Overview

To create a dynamic array type, define the type name using the `FIO_ARRAY_NAME` macro. i.e.:

```c
#define FIO_ARRAY_NAME int_ary
```

Next (usually), define the `FIO_ARRAY_TYPE` macro with the element type. The default element type is `void *`. For example:

```c
#define FIO_ARRAY_TYPE int
```

For complex types, define any (or all) of the following macros:

```c
// set to adjust element copying 
#define FIO_ARRAY_TYPE_COPY(dest, src)  
// set for element cleanup 
#define FIO_ARRAY_TYPE_DESTROY(obj)     
// set to adjust element comparison 
#define FIO_ARRAY_TYPE_CMP(a, b)        
// to be returned when `index` is out of bounds / holes 
#define FIO_ARRAY_TYPE_INVALID 0 
// set ONLY if the invalid element is all zero bytes 
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1     
// should the object be destroyed when copied to an `old` pointer?
#define FIO_ARRAY_DESTROY_AFTER_COPY 1 
// when array memory grows, how many extra "spaces" should be allocated?
#define FIO_ARRAY_PADDING 4 
// should the array growth be exponential? (ignores FIO_ARRAY_PADDING)
#define FIO_ARRAY_EXPONENTIAL 0 
// optimizes small arrays (mostly tuplets and single item arrays).
// note: values larger than 1 add a memory allocation cost to the array container
#define FIO_ARRAY_ENABLE_EMBEDDED 1
```

To create the type and helper functions, include The facil.io library header.

For example:

```c
typedef struct {
  int i;
  float f;
} foo_s;

#define FIO_ARRAY_NAME ary
#define FIO_ARRAY_TYPE foo_s
#define FIO_ARRAY_TYPE_CMP(a,b) (a.i == b.i && a.f == b.f)
#include "fio-stl/include.h"

void example(void) {
  ary_s a = FIO_ARRAY_INIT;
  foo_s *p = ary_push(&a, (foo_s){.i = 42});
  FIO_ARRAY_EACH(ary, &a, pos) { // pos will be a pointer to the element
    fprintf(stderr, "* [%zu]: %p : %d\n", (size_t)(pos - ary2ptr(&a)), (void *)pos, pos->i);
  }
  ary_destroy(&a);
}
```

### Dynamic Arrays - API

#### The Array Type (`ARY_s`)

```c
typedef struct {
  FIO_ARRAY_TYPE *ary;
  uint32_t capa;
  uint32_t start;
  uint32_t end;
} FIO_NAME(FIO_ARRAY_NAME, s); /* ARY_s in these docs */
```

The array type should be considered opaque. Use the helper functions to updated the array's state when possible, even though the array's data is easily understood and could be manually adjusted as needed.

#### `FIO_ARRAY_INIT`

````c
#define FIO_ARRAY_INIT  {0}
````

This macro initializes an uninitialized array object.

#### `ARY_destroy`

````c
void ARY_destroy(ARY_s * ary);
````

Destroys any objects stored in the array and frees the internal state.

#### `ARY_new`

````c
ARY_s * ARY_new(void);
````

Allocates a new array object on the heap and initializes it's memory.

#### `ARY_free`

````c
void ARY_free(ARY_s * ary);
````

Frees an array's internal data AND it's container!

#### `ARY_count`

````c
uint32_t ARY_count(ARY_s * ary);
````

Returns the number of elements in the Array.

#### `ARY_capa`

````c
uint32_t ARY_capa(ARY_s * ary);
````

Returns the current, temporary, array capacity (it's dynamic).

#### `ARY_reserve`

```c
uint32_t ARY_reserve(ARY_s * ary, int64_t capa);
```

Reserves capacity for new members to be added to the array.

If `capa` is negative, new memory will be allocated at the beginning of the array rather then it's end.

Returns the array's new capacity.

#### `ARY_concat`

```c
ARY_s * ARY_concat(ARY_s * dest, ARY_s * src);
```

Adds all the items in the `src` Array to the end of the `dest` Array.

The `src` Array remain untouched.

Always returns the destination array (`dest`).

#### `ARY_set`

```c
FIO_ARRAY_TYPE * ARY_set(ARY_s * ary,
                       int64_t index,
                       FIO_ARRAY_TYPE data,
                       FIO_ARRAY_TYPE *old);
```

Sets `index` to the value in `data`.

If `index` is negative, it will be counted from the end of the Array (-1 == last element).

If `old` isn't NULL, the existing data will be copied to the location pointed to by `old` before the copy in the Array is destroyed.

Returns a pointer to the new object, or NULL on error.

#### `ARY_get`

```c
FIO_ARRAY_TYPE ARY_get(ARY_s * ary, int64_t index);
```

Returns the value located at `index` (no copying is performed).

If `index` is negative, it will be counted from the end of the Array (-1 == last element).

**Reminder**: indexes are zero based (first element == 0).

#### `ARY_find`

```c
uint32_t ARY_find(ARY_s * ary, FIO_ARRAY_TYPE data, int64_t start_at);
/* When an object can't be founds, this is the returned value. */
#define FIO_ARRAY_NOT_FOUND ((uint32_t)-1)
```

Returns the index of the object or `FIO_ARRAY_NOT_FOUND` (`(uint32_t)-1`) if the object wasn't found.

If `start_at` is negative (i.e., -1), than seeking will be performed in reverse, where -1 == last index (-2 == second to last, etc').

#### `ARY_remove`
```c
int ARY_remove(ARY_s * ary, int64_t index, FIO_ARRAY_TYPE *old);
```

Removes an object from the array, MOVING all the other objects to prevent "holes" in the data.

If `old` is set, the data is copied to the location pointed to by `old` before the data in the array is destroyed.

Returns 0 on success and -1 on error.

This action is O(n) where n in the length of the array. It could get expensive.

#### `ARY_remove2`

```c
uint32_t ARY_remove2(ARY_S * ary, FIO_ARRAY_TYPE data);
```

Removes all occurrences of an object from the array (if any), MOVING all the existing objects to prevent "holes" in the data.

Returns the number of items removed.

This action is O(n) where n in the length of the array. It could get expensive.

#### `ARY_compact`
```c
void ARY_compact(ARY_s * ary);
```

Attempts to lower the array's memory consumption.

#### `ARY_to_a`

```c
FIO_ARRAY_TYPE * ARY_to_a(ARY_s * ary);
```

Returns a pointer to the C array containing the objects.

#### `ARY_push`

```c
FIO_ARRAY_TYPE * ARY_push(ARY_s * ary, FIO_ARRAY_TYPE data);
```

 Pushes an object to the end of the Array. Returns a pointer to the new object or NULL on error.

#### `ARY_pop`

```c
int ARY_pop(ARY_s * ary, FIO_ARRAY_TYPE *old);
```

Removes an object from the end of the Array.

If `old` is set, the data is copied to the location pointed to by `old` before the data in the array is destroyed.

Returns -1 on error (Array is empty) and 0 on success.

#### `ARY_unshift`

```c
FIO_ARRAY_TYPE *ARY_unshift(ARY_s * ary, FIO_ARRAY_TYPE data);
```

Unshifts an object to the beginning of the Array. Returns a pointer to the new object or NULL on error.

This could be expensive, causing `memmove`.

#### `ARY_shift`

```c
int ARY_shift(ARY_s * ary, FIO_ARRAY_TYPE *old);
```

Removes an object from the beginning of the Array.

If `old` is set, the data is copied to the location pointed to by `old` before the data in the array is destroyed.

Returns -1 on error (Array is empty) and 0 on success.

#### `ARY_each`

```c
uint32_t ARY_each(ARY_s * ary,
                  int (*task)(ARY_each_s * info),
                  void *arg,
                  int64_t start_at);
```

Iteration using a callback for each entry in the array.

The callback task function must accept an an `ARY_each_s` pointer (name matches Array name).

If the callback returns -1, the loop is broken. Any other value is ignored.

Returns the relative "stop" position (number of items processed + starting point).

The `ARY_each_s` data structure looks like this:

```c
/** Iteration information structure passed to the callback. */
typedef ARY_each_s {
  /** The being iterated. Once set, cannot be safely changed. */
  FIO_ARRAY_PTR const parent;
  /** The current object's index */
  uint64_t index;
  /** Always 1 and may be used to allow type detection. */
  const int64_t items_at_index;
  /** The callback / task called for each index, may be updated mid-cycle. */
  int (*task)(ARY_each_s * info);
  /** The argument passed along to the task. */
  void *arg;
  /** The object / value at the current index. */
  FIO_ARRAY_TYPE value;
} ARY_each_s;
```

#### `ARY_each_next`

```c
FIO_ARRAY_TYPE ARY_each_next(ARY_s* ary,
                             FIO_ARRAY_TYPE **first,
                             FIO_ARRAY_TYPE *pos);

```

Used internally by the `FIO_ARRAY_EACH` macro.

Returns a pointer to the first object if `pos == NULL` and there are objects
in the array.

Returns a pointer to the (next) object in the array if `pos` and `first` are valid.

Returns `NULL` on error or if the array is empty.

**Note**: 
The first pointer is automatically set and it allows object insertions and memory effecting functions to be called from within the loop.

If the object in `pos` (or any object before it) were removed, consider passing `pos-1` to the function, to avoid skipping any elements while looping.

#### `FIO_ARRAY_EACH`

```c
#define FIO_ARRAY_EACH(array_name, array, pos)                                               \
  for (__typeof__(FIO_NAME2(array_name, ptr)((array)))                             \
           first___ = NULL,                                                    \
           pos = FIO_NAME(array_name, each_next)((array), &first___, NULL);    \
       pos;                                                                    \
       pos = FIO_NAME(array_name, each_next)((array), &first___, pos))
```


Iterates through the array using a `for` loop.

Access the object with the pointer `pos`. The `pos` variable can be named however you please.

It is possible to edit the array while iterating, however when deleting `pos`, or objects that are located before `pos`, using the proper array functions, the loop will skip the next item unless `pos` is set to `pos-1`.

**Note**: this macro supports automatic pointer tagging / untagging.

-------------------------------------------------------------------------------
## Hash Tables and Maps

HashMaps (a.k.a., Hash Tables) and sets are extremely useful and common mapping / dictionary primitives, also sometimes known as "**dictionaries**".

Hash maps use both a `hash` and a `key` to identify a `value`. The `hash` value is calculated by feeding the key's data to a hash function (such as Risky Hash or SipHash).

A hash map without a `value` is known as a Set or a Bag. It uses only a `hash` and a `key` to access the same `key` in the Set. Since Sets promise that all objects in the Set are unique, they offer a pretty powerful tool often used for cache collections or for filtering out duplicates from other data sources.

By default, if not defined differently, facil.io maps use String data as the `key`. If a `FIO_MAP_VALUE` type is not defined, than the default behavior is to create a Set rather than a Dictionary.

```c
/* Set the properties for the key-value Hash Map type called `dict_s` */
#define FIO_MAP_NAME                 dict
#define FIO_MAP_VALUE_BSTR /* a special macro helper to define binary Strings as values */
#define FIO_RAND           /* to provide us with a hash function. */
#include "fio-stl.h"

/* it is often more secure to "salt" the hashing function with a per-map salt, and so: */

/** set helper for consistent and secure hash values */
FIO_IFUNC fio_str_info_s dict_set2(dict_s *m, fio_str_info_s key, fio_str_info_s obj) {
  return dict_set(m, fio_risky_hash(key.buf, key.len, (uint64_t)m), key, obj, NULL);
}
/** conditional set helper for consistent and secure hash values */
FIO_IFUNC fio_str_info_s dict_set_if_missing2(dict_s *m,
                                              fio_str_info_s key,
                                              fio_str_info_s obj) {
  return dict_set_if_missing(m, fio_risky_hash(key.buf, key.len, (uint64_t)m), key, obj);
}
/** get helper for consistent and secure hash values */
FIO_IFUNC fio_str_info_s dict_get2(dict_s *m, fio_str_info_s key) {
  return dict_get(m, fio_risky_hash(key.buf, key.len, (uint64_t)m), key);
}
```

Note that this Map implementation, like all dynamic type templates, supports optional pointer tagging (`FIO_PTR_TAG`) and reference counting (`FIO_REF_NAME`).

### Defining the Map's Keys

Every map / dictionary requires a `key` type that is used for either testing uniqueness (a Set) or accessing a `value` (a Hash Map or Dictionary).

If the `key` type is left undefined (or the `FIO_MAP_KEY_BSTR` macro is defined), the map's API will expect a `fio_str_info_s` as a key and facil.io will default to a String key using the `fio_bstr` functions to allocate, manage and free strings. These strings are always `NUL` terminated and always allocated dynamically.

It is also possible to define the helper macro `FIO_MAP_KEY_KSTR` in which case the Strings internally will use the `fio_keystr` API, which has a special small string optimization for strings up to 14 bytes (on 64bit systems) before allocating memory (while adding an allocation overhead to the map itself). This could improve performance by improving cache locality.

To use a custom `key` type and control its behavior, define any (or all) of the following macros before including the C STL header library (the `FIO_MAP_KEY` macro is required in order to make changes):

#### `FIO_MAP_KEY`

```c
/* default when FIO_MAP_KEY is undefined */
#define FIO_MAP_KEY  fio_str_info_s
```

The "external" / exposed type used to define the key. The external type is the type used by the API for inputting and reviewing key values. However, `FIO_MAP_KEY_INTERNAL` may be (optionally) defined in order for the map to use a different type for storage purposes.

If undefined, keys will be a binary safe buffer / string (`fio_str_info_s`). Internally the implementation will use the `fio_bstr` API to allocate, store and free copies of each key.

#### `FIO_MAP_KEY_INTERNAL`

```c
/* default when FIO_MAP_KEY is defined */
#define FIO_MAP_KEY_INTERNAL FIO_MAP_KEY
/* default when FIO_MAP_KEY is undefined or FIO_MAP_KEY_BSTR is defined */
#define FIO_MAP_KEY_INTERNAL char *
```

The `FIO_MAP_KEY_INTERNAL`, if defined, allows the map to use an internal key storage type that is different than the type used for its external API, allowing for both a more convenient API and possible internal updates without API changes.

#### `FIO_MAP_KEY_FROM_INTERNAL`

```c
/* default when FIO_MAP_KEY is defined */
#define FIO_MAP_KEY_FROM_INTERNAL(k) k
/* default when FIO_MAP_KEY is undefined or FIO_MAP_KEY_BSTR is defined */
#define FIO_MAP_KEY_FROM_INTERNAL(k) fio_bstr_info((k))
```

This macro converts between the Map's internal `key` storage type and the API representation.


#### `FIO_MAP_KEY_COPY`

```c
/* default when FIO_MAP_KEY is defined */
#define FIO_MAP_KEY_COPY(dest, src) (dest) = (src)
/* default when FIO_MAP_KEY is undefined or FIO_MAP_KEY_BSTR is defined */
#define FIO_MAP_KEY_COPY(dest, src) (dest) = fio_bstr_write(NULL, (src).buf, (src).len)
```

This macro copies the Map's external representation of the `key` (as defined by the API) into the map's internal `key` storage.

#### `FIO_MAP_KEY_CMP`

```c
/* default when FIO_MAP_KEY is defined */
#define FIO_MAP_KEY_CMP(internal, external) (internal) == (external)
/* default when FIO_MAP_KEY is undefined or FIO_MAP_KEY_BSTR is defined */
#define FIO_MAP_KEY_CMP(internal, external) fio_bstr_is_eq2info((internal), (external))
```

This macro compares a Map's external representation of a `key` (as defined by the API) with a `key` stored in the map's internal storage.

#### `FIO_MAP_KEY_DESTROY`

```c
/* default when FIO_MAP_KEY is defined */
#define FIO_MAP_KEY_DESTROY(key)
/* default when FIO_MAP_KEY is undefined or FIO_MAP_KEY_BSTR is defined */
#define FIO_MAP_KEY_DESTROY(key) fio_bstr_free((key))
```

This macro destroys a `key` stored in the map's internal storage. This means freeing any allocated resources. The map will ignore any remaining junk data.

#### `FIO_MAP_KEY_DISCARD`

```c
/* default does nothing */
#define FIO_MAP_KEY_DISCARD(key)
```

This macro destroys an external representation of a `key` if it didn't make it into the map's internal storage.

This is useful in when the key was pre-allocated, if it's reference was increased in advance for some reason or when "transferring ownership" of the `key` to the map.


#### `FIO_MAP_KEY_KSTR`

```c
#ifdef FIO_MAP_KEY_KSTR
#define FIO_MAP_KEY                  fio_str_info_s
#define FIO_MAP_KEY_INTERNAL         fio_keystr_s
#define FIO_MAP_KEY_FROM_INTERNAL(k) fio_keystr_info(&(k))
#define FIO_MAP_KEY_COPY(dest, src)  (dest) = fio_keystr_init((src), ...)
#define FIO_MAP_KEY_CMP(a, b)        fio_keystr_is_eq2((a), (b))
#define FIO_MAP_KEY_DESTROY(key)      fio_keystr_destroy(&(key), FIO_NAME(FIO_MAP_NAME, __key_free))
#define FIO_MAP_KEY_DISCARD(key)
```

If `FIO_MAP_KEY` isn't set, or `FIO_MAP_KEY_KSTR` is explicitly defined, than a `fio_str_info_s` will be the external key type and `fio_keystr_s` will be the internal key type.

Passing a key with `key.capa == (size_t)-1` will prevent a string copy and the map will assume that the string will stay in the same memory address for the whole of the map's lifetime.

### Defining the Map's Values

Most often we want a dictionary or a hash map to retrieve a `value` based on its associated `key`.

Values and their behavior can be controlled using similar macros to the `key` macros.

#### `FIO_MAP_VALUE_BSTR`

```c
#ifdef FIO_MAP_VALUE_BSTR
#define FIO_MAP_VALUE                  fio_str_info_s
#define FIO_MAP_VALUE_INTERNAL         char *
#define FIO_MAP_VALUE_FROM_INTERNAL(v) fio_bstr_info((v))
#define FIO_MAP_VALUE_COPY(dest, src)                                     \
  (dest) = fio_bstr_write(NULL, (src).buf, (src).len)
#define FIO_MAP_VALUE_DESTROY(v) fio_bstr_free((v))
#define FIO_MAP_VALUE_DISCARD(v)
#endif
```

This is a shortcut macro that sets the values to String objects. The strings are binary safe (may contain multiple `NUL` values) and are always `NUL` terminated (for extra safety).

#### `FIO_MAP_VALUE`

```c
/* poor example */
#define FIO_MAP_VALUE void *
```

Similar to `FIO_MAP_KEY`, defines the (external) representation of a Map's `value`.

**Note**: a common `value` is the `void *` pointer. However, this does not provide type safety, and so it is better to use a specific type for the `value`.

#### `FIO_MAP_VALUE_INTERNAL`

```c
/* default when FIO_MAP_VALUE is defined */
#define FIO_MAP_VALUE_FROM_INTERNAL(o) o
```

Similar to `FIO_MAP_KEY_FROM_INTERNAL`, this macro converts between the Map's internal `value` storage type and the API representation.

#### `FIO_MAP_VALUE_COPY`

```c
/* default when FIO_MAP_VALUE is defined */
#define FIO_MAP_VALUE_COPY(internal, external) (internal) = (external)
```

Similar to `FIO_MAP_KEY_COPY`, this macro copies the Map's external representation of the `value` (as defined by the API) into the map's internal `value` storage.


#### `FIO_MAP_VALUE_DESTROY`

```c
/* default when FIO_MAP_VALUE is defined */
#define FIO_MAP_VALUE_DESTROY(o)
#define FIO_MAP_VALUE_DESTROY_SIMPLE 1
```

Similar to `FIO_MAP_KEY_DESTROY`, this macro destroys a `value` stored in the map's internal storage. This means freeing any allocated resources. The map will ignore any remaining junk data.

#### `FIO_MAP_VALUE_DISCARD`

```c
/* default when FIO_MAP_VALUE is defined */
#define FIO_MAP_VALUE_DISCARD(o)
```

Similar to `FIO_MAP_KEY_DISCARD`, this macro destroys the external representation of the `value` if it didn't make it into the map's internal storage.

### Hash Calculations and Security

The map implementation offers protection against too many full collisions or non-random hashes that can occur with poor hash functions or when the Map is attacked. When the map detects a possible "attack", it will start overwriting existing data instead of trying to resolve collisions.

This can be adjusted using the `FIO_MAP_ATTACK_LIMIT` macro which usually allows up to 16 full hash collisions before assuming the map is being attacked, thus giving leeway for faster yet less secure hashing functions.

When using unsafe input data as the Map `key`, it is still better to manually manage the hashing function by salting it with a map specific value (such as the map's pointer). Then helpers can be used to make sure the code remains DRY.

For example:

```c
/* Set the properties for the key-value Hash Map type called `dict_s` */
#define FIO_MAP_NAME                 dict
#define FIO_MAP_VALUE_BSTR /* a special macro helper to define binary Strings as values */
#define FIO_RAND           /* to provide us with a hash function. */
#include "fio-stl.h"

/* it is often more secure to "salt" the hashing function with a per-map salt, and so: */

/** set helper for consistent and secure hash values */
FIO_IFUNC fio_str_info_s dict_set2(dict_s *m, fio_str_info_s key, fio_str_info_s obj) {
  return dict_set(m, fio_risky_hash(key.buf, key.len, (uint64_t)m), key, obj, NULL);
}
/** conditional set helper for consistent and secure hash values */
FIO_IFUNC fio_str_info_s dict_set_if_missing2(dict_s *m,
                                              fio_str_info_s key,
                                              fio_str_info_s obj) {
  return dict_set_if_missing(m, fio_risky_hash(key.buf, key.len, (uint64_t)m), key, obj);
}
/** get helper for consistent and secure hash values */
FIO_IFUNC fio_str_info_s dict_get2(dict_s *m, fio_str_info_s key) {
  return dict_get(m, fio_risky_hash(key.buf, key.len, (uint64_t)m), key);
}
```

#### `FIO_MAP_HASH_FN`

However, when using safe input or a secure enough hashing function, it makes sense to simplify the API the template produces by having the template automatically calculate the hash.

We can add a lower level of security to this approach by salting the hash with a runtime constant that changes every time we restart the program, such as the memory address of one of the function in the program.

This can be done using the `FIO_MAP_HASH_FN(external_key)` macro i.e.:

```c
/* Set the properties for the key-value Hash Map type called `dict_s` */
#define FIO_MAP_NAME                 dict
#define FIO_MAP_VALUE_BSTR /* a special macro helper to define binary Strings as values. */
#define FIO_RAND           /* to provide us with a hash function. */

/* use any non-inlined function's address as a hash salt. Here `dict_destroy` is used. */
#define FIO_MAP_HASH_FN(ex_key) fio_risky_hash(ex_key.buf, ex_key.len, (uint64_t)(dict_destroy))

#include "fio-stl.h"
```

#### `FIO_MAP_RECALC_HASH`

```c
/* default: */
#define FIO_MAP_RECALC_HASH 0
```

Sometimes hashing can be very fast. A good example is when hashing pointer or integer values. In these cases, it makes sense to recalculate the hash rather than spend memory on caching it.

Since the Map always caches an 8 bits permutation of the hash, it is often possible to avoid spending the additional overhead of 8 bytes per-object by setting `FIO_MAP_RECALC_HASH` to `1` (true).

This, of course, requires that the `FIO_MAP_HASH_FN(key)` macro be defined, or the map will not know how to recalculate the hash and instead cache the information.

### Ordering and Performance

The facil.io implementation supports FIFO (First In First Out) and LRU (Least Recently Used) ordering scheme, allowing to `map_evict` any number of possibly "stale" elements, offering an initial caching solution that can be expanded upon.

Obviously these additional ordering details require more memory per object (8 additional bytes) and additional CPU cycles for ordering management. Although the performance price isn't big, by default Maps / Dictionaries are unordered.

#### `FIO_MAP_ORDERED` 

If defined without a value or with a true value, the Set / Map / Dictionary will be ordered (FIFO unless otherwise specified).

A shortcut to define on ordered map would be to use the `FIO_OMAP_NAME` and `FIO_UMAP_NAME` naming macros instead of the `FIO_MAP_NAME` naming macro.

```c
#if defined(FIO_UMAP_NAME)
#define FIO_MAP_NAME FIO_UMAP_NAME
#undef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 0
#elif defined(FIO_OMAP_NAME)
#define FIO_MAP_NAME FIO_OMAP_NAME
#undef FIO_MAP_ORDERED
#define FIO_MAP_ORDERED 1
#endif
```

#### `FIO_MAP_LRU`

If defined, the Set / Map / Dictionary will be ordered using a Least Recently Used approach. This means that iteration will start with the most important element (most recently used) while eviction will start with the most stale element (least recently used).

Auto eviction will be performed once the map reaches `FIO_MAP_LRU` elements.

i.e.,

```c
#define FIO_MAP_LRU (1ULL << 16) /* limits the map to 65,536 elements. */
```

### The Map Types

Each template implementation defines the following main types (named here assuming `FIO_MAP_NAME` is defined as `map`).

#### `map_s`

The Map's container (actual type). This should be considered an opaque type and access / mutation should be performed using the published API.

```c
typedef struct {
  uint32_t bits;
  uint32_t count;
  FIO_NAME(FIO_MAP_NAME, node_s) * map;
#if FIO_MAP_ORDERED
  FIO_INDEXED_LIST32_HEAD head;
#endif
} FIO_NAME(FIO_MAP_NAME, s);
```

#### `map_node_s`

This defines the internal object representation and should be considered to be an opaque type.

When a pointer to a node in the internal map is returned (such as when calling `map_get_ptr` or `map_set_ptr`, accessing the data in the type should be performed using the helper functions: `map_node2key(node_ptr)`, `map_node2hash(node_ptr)` and `map_node2val(node_ptr)`.

```c
typedef struct {
#if !FIO_MAP_RECALC_HASH
  uint64_t hash;
#endif
  FIO_MAP_KEY_INTERNAL key;
#ifdef FIO_MAP_VALUE
  FIO_MAP_VALUE_INTERNAL value;
#endif
#if FIO_MAP_ORDERED
  FIO_INDEXED_LIST32_NODE node;
#endif
} FIO_NAME(FIO_MAP_NAME, node_s);
```

#### `map_iterator_s`

```c
typedef struct {
  /** the key in the current position */
  FIO_MAP_KEY key;
#ifdef FIO_MAP_VALUE
  /** the value in the current position */
  FIO_MAP_VALUE value;
#endif
#if !FIO_MAP_RECALC_HASH
  /** the hash for the current position */
  uint64_t hash;
#endif
  struct {                   /* internal usage, do not access */
    uint32_t index;          /* the index in the internal map */
    uint32_t pos;            /* the position in the ordering scheme */
    uintptr_t map_validator; /* map mutation guard */
  } private_;
} FIO_NAME(FIO_MAP_NAME, iterator_s);
```

An iterator type represents a specific object and position in the Hash. The object data is valid as long as the object was not removed from the Map and the position is valid for as long as the Map didn't reallocate the internal storage (avoid adding new objects to the map while iterating).

### Construction / Deconstruction

#### `map_new`

```c
FIO_MAP_PTR map_new(void);
```

Allocates a new object on the heap and initializes it's memory.

#### `map_free`

```c
void map_free(FIO_MAP_PTR map);
```

Frees any internal data AND the object's container!

#### `map_destroy`

```c
void map_destroy(FIO_MAP_PTR map);
```

Destroys the object, re-initializing its container.

### Map State

#### `map_capa`

```c
uint32_t map_capa(FIO_MAP_PTR map);
```

Theoretical map capacity.

#### `map_count`

```c
uint32_t map_count(FIO_MAP_PTR map);
```

The number of objects in the map capacity.

#### `map_reserve`

```c
void map_reserve(FIO_MAP_PTR map, size_t capa);
```

Reserves at minimum the capacity requested for new members. May reserve more than the capacity requested.

### Adding / Removing Elements from the Map

The signature of some of these functions may change according to the template macors defined. For example, if the `FIO_MAP_HASH_FN(k)` was already defined than the Map's API will not require it as an argument. Also, since Sets do not have a `value` that is not the same as the `key` (unlike Dictionaries), than there is no reason to require an additional `value` argument.

#### `map_get`

```c
MAP_KEY_OR_VAL map_get(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                         uint64_t hash,
#endif
                                         FIO_MAP_KEY key);
```

Gets a value from the map, if exists. For Sets, the `key` is returned (since it is also the value).

#### `map_set`

```c
MAP_KEY_OR_VAL map_set(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                         uint64_t hash,
#endif
#ifdef FIO_MAP_VALUE
                         FIO_MAP_KEY key,
                         FIO_MAP_VALUE obj,
                         FIO_MAP_VALUE_INTERNAL *old
#else
                         FIO_MAP_KEY key
#endif
                        );
```

Sets a value in the map. Maps / Dictionaries will overwrite existing data if any. Sets never overwrite existing data.

#### `map_set_if_missing`

```c
MAP_KEY_OR_VAL map_set_if_missing(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                                    uint64_t hash,
#endif
                                    FIO_MAP_KEY key
#ifdef FIO_MAP_VALUE
                                  , FIO_MAP_VALUE obj
#endif
);
```

Sets a value in the map if not set previously.

#### `map_remove`

```c
int map_remove(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
              uint64_t hash,
#endif
              FIO_MAP_KEY key,
#ifdef FIO_MAP_VALUE
              FIO_MAP_VALUE_INTERNAL *old
#else
              FIO_MAP_KEY_INTERNAL *old
#endif
              );
```

Removes an object in the map, returning -1 if the object couldn't be found or 0 on success.

#### `map_evict`

```c
void map_evict(FIO_MAP_PTR map, size_t number_of_elements);
```

Evicts elements in the order defined by the template:
* If `FIO_MAP_LRU` was defined - evicts the most Least Recently Used (LRU) elements.
* If `FIO_MAP_ORDERED` is true - evicts the first elements inserted (FIFO).
* Otherwise eviction order is undefined. An almost random eviction will occur with neighboring items possibly being evicted together.

#### `map_clear`

```c
void map_clear(FIO_MAP_PTR map);
```

Removes all objects from the map, without releasing the map's resources.

#### `map_compact`

```c
void map_compact(FIO_MAP_PTR map);
```

Attempts to minimize memory use by shrinking the internally allocated memory used for the map.

#### `map_set_ptr`

```c
map_node_s * map_set_ptr(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                         uint64_t hash,
#endif
#ifdef FIO_MAP_VALUE
                         FIO_MAP_KEY key,
                         FIO_MAP_VALUE val,
                         FIO_MAP_VALUE_INTERNAL *old,
                         int overwrite
#else
                         FIO_MAP_KEY key
#endif
                        );
```

The core set function.

This function returns `NULL` on error (errors are logged).

If the map is a hash map, overwriting the value (while keeping the key) is possible. In this case the `old` pointer is optional, and if set than the old data will be copied to over during an overwrite.

If the Map is a Set (no value is defined), data is never overwritten and a new entry will be created only if missing.

**Note**: the function returns the pointer to the map's internal storage, where objects are stored using the internal types.

#### `map_get_ptr`

```c
map_node_s * map_get_ptr(FIO_MAP_PTR map,
#if !defined(FIO_MAP_HASH_FN)
                         uint64_t hash,
#endif
                         FIO_MAP_KEY key);
```

The core get function. This function returns `NULL` if the item is missing.

**Note**: the function returns the pointer to the map's internal storage, where objects are stored using the internal types.


### Map Iteration and Traversal

#### `map_get_next`

```c
map_iterator_s map_get_next(FIO_MAP_PTR map, map_iterator_s * current_pos);
```

Returns the next iterator object after `current_pos` or the first if `NULL`.

Note that adding objects to the map or rehashing between iterations could incur performance penalties when re-setting and re-seeking the previous iterator position. Depending on the ordering scheme this may disrupt the percieved order.

Adding objects to, or rehashing, an unordered map could invalidate the iterator object completely as the ordering may have changed and so the "next" object might be any object in the map.

#### `map_get_prev`

```c
map_iterator_s map_get_prev(FIO_MAP_PTR map, map_iterator_s * current_pos);
```

Returns the iterator object **before** `current_pos` or the last iterator if `NULL`.

See notes in `map_get_next`.

#### `map_iterator_is_valid`

```c
int map_iterator_is_valid( map_iterator_s * iterator);
```

Returns 1 if the iterator is out of bounds, otherwise returns 0.

#### `FIO_MAP_EACH`

```c
#define FIO_MAP_EACH(map_name, map_ptr, pos)                            \
  for (FIO_NAME(map_name, iterator_s)                                          \
           pos = FIO_NAME(map_name, get_next)(map_ptr, NULL);                  \
       FIO_NAME(map_name, iterator_is_valid)(&pos);                            \
       pos = FIO_NAME(map_name, get_next)(map_ptr, &pos))
```

Iterates through the map using an iterator object.

#### `FIO_MAP_EACH_REVERSED`

```c
#define FIO_MAP_EACH_REVERSED(map_name, map_ptr, pos)                   \
  for (FIO_NAME(map_name, iterator_s)                                          \
           pos = FIO_NAME(map_name, get_prev)(map_ptr, NULL);                  \
       FIO_NAME(map_name, iterator_is_valid)(&pos);                            \
       pos = FIO_NAME(map_name, get_prev)(map_ptr, &pos))
#endif
```

Iterates through the map using an iterator object.

#### `map_each`

```c
uint32_t map_each(FIO_MAP_PTR map,
                  int (*task)(map_each_s *),
                  void *udata,
                  ssize_t start_at);
```

Iterates through the map using a callback for each element in the map.

The callback task function must accept a `map_each_s` pointer, see detail below.

If the callback must return either `0` or `-1`. If `-1` (non-zero) is returned the loop stops.

Returns the relative "stop" position, i.e., the number of items processed + the starting point.


```c
/** Iteration information structure passed to the callback. */
typedef struct map_each_s {
  /** The being iterated. Once set, cannot be safely changed. */
  FIO_MAP_PTR const parent;
  /** The current object's index */
  uint64_t index;
  /** The callback / task called for each index, may be updated mid-cycle. */
  int (*task)(struct map_each_s * info);
  /** Opaque user data. */
  void *udata;
#ifdef FIO_MAP_VALUE
  /** The object's value at the current index. */
  FIO_MAP_VALUE value;
#endif
  /** The object's key the current index. */
  FIO_MAP_KEY key;
} map_each_s;
```

-------------------------------------------------------------------------------

## Reference Counting and Type Wrapping

```c
#define FIO_STR_SMALL fio_str
#define FIO_REF_NAME fio_str
#define FIO_REF_CONSTRUCTOR_ONLY
#include "fio-stl.h"
```

If the `FIO_REF_NAME` macro is defined, then reference counting helpers can be defined for any named type.

**Note**: requires the atomic operations to be defined (`FIO_ATOMIC`).

### Reference Counting Type Macros

The following setup Macros are supported when setting up the reference counting type helpers:

#### `FIO_REF_TYPE`

```c
#define FIO_REF_TYPE FIO_NAME(FIO_REF_NAME, s)
```

The type to be wrapped and reference counted by the `FIO_REF_NAME` wrapper API.

By default, `FIO_REF_TYPE` will equal `FIO_REF_NAME_s`, using the naming convention in this library.

#### `FIO_REF_INIT`

```c
#define FIO_REF_INIT(obj) (obj) = (FIO_REF_TYPE){0}
```

Sets up the default object initializer.

By default initializes the object's memory to zero.

If `FIO_REF_FLEX_TYPE` is defined, the variable `members` may be used during initialization. It's value is the same as the value passed on to the `REF_new` function.

**Note**:  `FIO_REF_FLEX_TYPE` should **not** be used when `FIO_MEM_FREE` macro only frees the number of bytes specified (rather than freeing the whole pointer, as `free` might do). The reference counter type does not store the data passed to the flex-`REF_new` function and frees the same number of bytes as a flex length of `0`.

#### `FIO_REF_DESTROY`

```c
#define FIO_REF_DESTROY(obj)
```

Sets up the default object cleanup. By default does nothing.

#### `FIO_REF_CONSTRUCTOR_ONLY`

By default, the reference counter generator will generate the `new2`, `free2` and `dup2` functions.

However, f the `FIO_REF_CONSTRUCTOR_ONLY` macro is defined, the reference counter will name these functions as `new`, `free` and `dup` instead, making them the type's only and primary constructor / destructor.

#### `FIO_REF_FLEX_TYPE`

If the `FIO_REF_FLEX_TYPE` macro is defined, the constructor will allocate a enough memory for both the type and a `FIO_REF_FLEX_TYPE` array consisting of the specified amount of members (as passed to the constructor's `member` argument).

This allows reference objects structures to include a flexible array of type `FIO_REF_FLEX_TYPE` at the end of the `struct`.

The `members` variable passed to the constructor will also be available to the `FIO_REF_INIT` macro.

**Note**: using `FIO_REF_FLEX_TYPE` limits the reference counter to 32 bits (rather then the native word size which **may** be 64 bits).

#### `FIO_REF_METADATA`

If defined, should be type that will be available as "meta data".

A pointer to this type sill be available using the `REF_metadata` function and will allow "hidden" data to be accessible even though it isn't part of the observable object.

#### `FIO_REF_METADATA_INIT`

```c
#define FIO_REF_METADATA_INIT(meta) (meta) = (FIO_REF_TYPE){0}
```

Sets up object's meta-data initialization (if any). Be default initializes the meta-data object's memory to zero.

#### `FIO_REF_METADATA_DESTROY`

```c
#define FIO_REF_METADATA_DESTROY(meta)
```

### Reference Counting Generated Functions

Reference counting adds the following functions:

#### `REF_new` / `REF_new2`

```c
FIO_REF_TYPE * REF_new2(void)
// or, if FIO_REF_FLEX_TYPE is defined:
FIO_REF_TYPE * REF_new2(size_t members)


// or, if FIO_REF_CONSTRUCTOR_ONLY is defined
FIO_REF_TYPE * REF_new(void) 
FIO_REF_TYPE * REF_new(size_t members) // for FIO_REF_FLEX_TYPE

```

Allocates a new reference counted object, initializing it using the `FIO_REF_INIT(object)` macro.

If `FIO_REF_METADATA` is defined, than the metadata is initialized using the `FIO_REF_METADATA_INIT(metadata)` macro.

#### `REF_dup`

```c
FIO_REF_TYPE * REF_dup(FIO_REF_TYPE * object)
```

Increases an object's reference count (an atomic operation, thread-safe).

#### `REF_free` / `REF_free2`

```c
void REF_free2(FIO_REF_TYPE * object)
// or, if FIO_REF_CONSTRUCTOR_ONLY is defined
void REF_free(FIO_REF_TYPE * object)
```

Frees an object or decreases it's reference count (an atomic operation, thread-safe).

Before the object is freed, the `FIO_REF_DESTROY(object)` macro will be called.

If `FIO_REF_METADATA` is defined, than the metadata is also destroyed using the `FIO_REF_METADATA_DESTROY(metadata)` macro.

#### `REF_metadata`

```c
FIO_REF_METADATA * REF_metadata(FIO_REF_TYPE * object)
```

If `FIO_REF_METADATA` is defined, than the metadata is accessible using this inlined function.

-------------------------------------------------------------------------------
## FIOBJ Soft Dynamic Types

```c
#define FIO_FIOBJ
#define FIOBJ_MALLOC /* an optional local memory allocator for FIOBJ types */
#include "fio-stl.h"
```

The facil.io library includes a dynamic type system that makes it a easy to handle mixed-type tasks, such as JSON object construction.

This soft type system included in the facil.io STL, it is based on the Core types mentioned above and it shares their API (Dynamic Strings, Dynamic Arrays, and Hash Maps).

The soft type system also offers an (optional) * [Local Memory allocator](#local-memory-allocation) for improved performance when defined with the `FIOBJ_MALLOC` macro defined.

The `FIOBJ` API offers type generic functions in addition to the type specific API. An objects underlying type is easily identified using `FIOBJ_TYPE(obj)` or `FIOBJ_TYPE_IS(obj, type)`.

The documentation regarding the `FIOBJ` soft-type system is divided as follows:  

* [`FIOBJ` General Considerations](#fiobj-general-considerations)

* [`FIOBJ` Types and Identification](#fiobj-types-and-identification)

* [`FIOBJ` Core Memory Management](#fiobj-core-memory-management)

* [`FIOBJ` Common Functions](#fiobj-common-functions)

* [Primitive Types](#fiobj-primitive-types)

* [Numbers (Integers)](#fiobj-integers)

* [Floats](#fiobj-floats)

* [Strings](#fiobj-strings)

* [Arrays](#fiobj-arrays)

* [Hash Maps](#fiobj-hash-maps)

* [JSON Helpers](#fiobj-json-helpers)

* [How to Extend the `FIOBJ` Type System](#how-to-extend-the-fiobj-type-system)

In the facil.io web application framework, there are extensions to the core `FIOBJ` primitives, including:

* [IO storage](fiobj_io)

* [Mustache](fiobj_mustache)

### `FIOBJ` General Considerations

1. To use the `FIOBJ` soft types, define the `FIO_FIOBJ` macro and then include the facil.io STL header.

2. To include declarations as globally available symbols (allowing the functions to be called from multiple C files), define `FIO_EXTERN` _before_ including the STL header.

    This also requires that a _single_ C file (translation unit) define `FIO_EXTERN_COMPLETE` _before_ including the header with the `FIO_EXTERN` directive.

3. The `FIOBJ` types use pointer tagging and require that the memory allocator provide allocations on 8 byte memory alignment boundaries (they also assume each byte is 8 bits).

    If the system allocator doesn't provide (at least) 8 byte memory alignment, use the facil.io memory allocator provided (`fio_malloc`).

4. The `FIOBJ` soft type system uses an "**ownership**" memory model.

    This means that Arrays "**own**" their **members** and Hash Maps "**own**" their **values** (but **not** the keys).

    Freeing an Array will free all the objects within the Array. Freeing a Hash Map will free all the values within the Hash Map (but none of the keys).

    Ownership is only transferred if the object is removed from it's container.

    i.e., `fiobj_array_get` does **not** transfer ownership (it just allows temporary "access"). Whereas, `fiobj_array_remove` **does** revoke ownership - either freeing the object or moving the ownership to the pointer provided to hold the `old` value.

### `FIOBJ` Types and Identification

`FIOBJ` objects can contain any number of possible types, including user defined types.

These are the built-in types / classes that the Core `FIOBJ` system includes (before any extensions):

* `FIOBJ_T_INVALID`: indicates an **invalid** type class / type (a `FIOBJ_INVALID` value).

* `FIOBJ_T_PRIMITIVE`: indicates a **Primitive** class / type.

* `FIOBJ_T_NUMBER`: indicates a **Number** class / type.

* `FIOBJ_T_FLOAT`: indicates a **Float** class / type.

* `FIOBJ_T_STRING`: indicates a **String** class / type.

* `FIOBJ_T_ARRAY`: indicates an **Array** class / type.

* `FIOBJ_T_HASH`: indicates a **Hash Map** class / type.

* `FIOBJ_T_OTHER`: (internal) indicates an **Other** class / type. This is designed to indicate an extension / user defined type.

The `FIOBJ_T_PRIMITIVE` class / type resolves to one of the following types:

* `FIOBJ_T_NULL`: indicates a `fiobj_null()` object.

* `FIOBJ_T_TRUE`: indicates a `fiobj_true()` object.

* `FIOBJ_T_FALSE`: indicates a `fiobj_false()` object.

In the facil.io web application framework, there are extensions to the core `FIOBJ` primitives, including:

* [`FIOBJ_T_IO`](fiobj_io)

The following functions / MACROs help identify a `FIOBJ` object's underlying type.

#### `FIOBJ_TYPE(o)`

```c
#define FIOBJ_TYPE(o) fiobj_type(o)
```

#### `FIOBJ_TYPE_IS(o)`

```c
#define FIOBJ_TYPE_IS(o, type) (fiobj_type(o) == type)
```

#### `FIOBJ_TYPE_CLASS(o)`

```c
#define FIOBJ_TYPE_CLASS(o) ((fiobj_class_en)(((uintptr_t)o) & 7UL))
```

Returns the object's type class. This is limited to one of the core types. `FIOBJ_T_PRIMITIVE` and `FIOBJ_T_OTHER` may be returned (they aren't expended to their underlying type).

**Note**: some numbers (`FIOBJ_T_NUMBER` / `FIOBJ_T_FLOAT`) may return `FIOBJ_T_OTHER` when `FIOBJ_TYPE_CLASS` is used, but return their proper type when `FIOBJ_TYPE` is used. This is due to memory optimizations being unavailable for some numerical values.

#### `FIOBJ_IS_INVALID(o)`

```c
#define FIOBJ_IS_INVALID(o) (((uintptr_t)(o)&7UL) == 0)
```

Tests if the object is (probably) a valid FIOBJ

#### `FIOBJ_IS_NULL(o)`

```c
#define FIOBJ_IS_NULL(o) (FIOBJ_IS_INVALID(o) || ((o) == FIOBJ_T_NULL))
```

Tests if the object is either a `NULL` `FIOBJ` object or an invalid object.

#### `FIOBJ_PTR_UNTAG(o)`

```c
#define FIOBJ_PTR_UNTAG(o) ((uintptr_t)o & (~7ULL))
```

Removes the `FIOBJ` type tag from a `FIOBJ` objects, allowing access to the underlying pointer and possible type.

This is made available for authoring `FIOBJ` extensions and **shouldn't** be normally used.

#### `fiobj_type`

```c
size_t fiobj_type(FIOBJ o);
```

Returns an objects type. This isn't limited to known types.

Avoid calling this function directly. Use the MACRO instead.

### `FIOBJ` Core Memory Management

`FIOBJ` objects are **copied by reference** (not by value). Once their reference count is reduced to zero, their memory is freed.

This is extremely important to note, especially in multi-threaded environments. This implied that: **access to a dynamic `FIOBJ` object is _NOT_ thread-safe** and `FIOBJ` objects that may be written to (such as Arrays, Strings and Hash Maps) should **not** be shared across threads (unless properly protected).

The `FIOBJ` soft type system uses an "**ownership**" memory model. When placing a **value** in an Array or a Hash Map, the "ownership" is moved. Freeing the Array / Hash Map will free the object (unless `fiobj_dup` was called). Hash Maps "**own**" their _values_ (but **not** the _keys_).

#### `fiobj_dup`

```c
FIOBJ fiobj_dup(FIOBJ o);
```

Increases an object's reference count and returns it.

#### `fiobj_free`

```c
void fiobj_free(FIOBJ o);
```

Decreases an object's reference count or frees it.

**Note**:

This function is **recursive** and could cause a **stack explosion** error.

In addition, recursive object structures may produce unexpected results (for example, objects are always freed).

The `FIOBJ_MAX_NESTING` nesting limit doesn't apply to `fiobj_free`, making it possible to "expload" the stack if misused.

This places the responsibility on the user / developer, not to exceed the maximum nesting limit (or errors may occur).

When accepting external data, consider using the JSON parser, as it protects against this issue, offering a measure of safety against external data attacks.

### `FIOBJ` Common Functions

#### `fiobj_is_eq`

```c
unsigned char fiobj_is_eq(FIOBJ a, FIOBJ b);
```

Compares two objects.

Note: objects that contain other objects (i.e., Hash Maps) don't support this equality check just yet (feel free to contribute a PR for this).

#### `fiobj2cstr`

```c
fio_str_info_s fiobj2cstr(FIOBJ o);
```

Returns a temporary String representation for any FIOBJ object.

For number objects and floats this is thread safe for up to 128 threads.

For printing Arrays and Hash maps, using a JSON representation will provide more information.

#### `fiobj2i`

```c
intptr_t fiobj2i(FIOBJ o);
```

Returns an integer representation for any FIOBJ object.

#### `fiobj2f`

```c
double fiobj2f(FIOBJ o);
```

Returns a float (double) representation for any FIOBJ object.


#### `fiobj_each1`

```c
uint32_t fiobj_each1(FIOBJ o, int32_t start_at,
                     int (*task)(FIOBJ child, void *arg),
                     void *arg);
```

Performs a task for each element held by the FIOBJ object **directly** (but **not** itself).

If `task` returns -1, the `each` loop will break (stop).

Returns the "stop" position - the number of elements processed + `start_at`.


#### `fiobj_each2`

```c
uint32_t fiobj_each2(FIOBJ o,
                     int (*task)(FIOBJ obj, void *arg),
                     void *arg);
```

Performs a task for each element held by the FIOBJ object (directly or indirectly), **including** itself and any nested elements (a deep task).

The order of performance is by order of appearance, as if all nesting levels were flattened.

If `task` returns -1, the `each` loop will break (stop).

Returns the number of elements processed.

**Note**:

This function is **recursive** and could cause a **stack explosion** error.

The facil.io library attempts to protect against this error by limiting recursive access to `FIOBJ_MAX_NESTING`... however, this also assumes that a user / developer doesn't exceed the maximum nesting limit (or errors may occur).

#### `fiobj_json_find`

```c
FIOBJ fiobj_json_find(FIOBJ object, fio_str_info_s notation);
```

Uses JavaScript (JSON) style notation to find data in an object structure.

For example, `"[0].name"` will return the `"name"` property of the first object in an Array object.

Returns a temporary reference to the object or `FIOBJ_INVALID` on an error.

Use `fiobj_dup` to collect an actual reference to the returned object.

**Note**:

Using the search algorithm with long object names and/or deeper nesting levels might incur a performance penalty due to the fact that the algorithm tests for all possible object name permutations.

i.e., `"name1.name2.name3"` will first be tested as the whole string (`"name1.name2.name3"`), then `"name1.name2" + "name.3"` will be tested, then `"name1" + "name2.name.3"` will be tested for and `"name1" + "name2" + "name3"` will only be attempted last (allowing all permutations to be reviewed rather than assuming a `.` is always a delimiter).

#### `fiobj_json_find2`

```c
#define fiobj_json_find2(object, str, length)                                  \
  fiobj_json_find(object, (fio_str_info_s){.buf = str, .len = length})
```

A macro helper for [`fiobj_json_find`](#fiobj_json_find).

### `FIOBJ` Primitive Types

The `true`, `false` and `null` primitive type functions (in addition to the common functions) are only their simple static constructor / accessor functions.

The primitive types are immutable.

#### `fiobj_true`

```c
FIOBJ fiobj_true(void);
```

Returns the `true` primitive.

#### `fiobj_false`

```c
FIOBJ fiobj_false(void);
```

Returns the `false` primitive.

#### `fiobj_null`

```c
FIOBJ fiobj_null(void);
```

Returns the `nil` / `null` primitive.


### `FIOBJ` Integers

#### `fiobj_num_new`

```c
FIOBJ fiobj_num_new(intptr_t i);
```

Creates a new Number object.

#### `fiobj_num2i`

```c
intptr_t fiobj_num2i(FIOBJ i);
```

Reads the number from a `FIOBJ` Number.

#### `fiobj_num2f`

```c
double fiobj_num2f(FIOBJ i);
```

Reads the number from a `FIOBJ` Number, fitting it in a double.

#### `fiobj_num2cstr`

```c
fio_str_info_s fiobj_num2cstr(FIOBJ i);
```

Returns a String representation of the number (in base 10).

#### `fiobj_num_free`

```c
void fiobj_num_free(FIOBJ i);
```

Frees a `FIOBJ` number (a type specific `fiobj_free` alternative - use only when the type was validated).


### `FIOBJ` Floats

#### `fiobj_float_new`

```c
FIOBJ fiobj_float_new(double i);
```

Creates a new Float (double) object.

#### `fiobj_float2i`

```c
intptr_t fiobj_float2i(FIOBJ i);
```

Reads the number from a `FIOBJ` Float rounding it to an integer.

#### `fiobj_float2f`

```c
double fiobj_float2f(FIOBJ i);
```

Reads the value from a `FIOBJ` Float, as a double.

#### `fiobj_float2cstr`

```c
fio_str_info_s fiobj_float2cstr(FIOBJ i);
```

Returns a String representation of the float.

#### `fiobj_float_free`

```c
void fiobj_float_free(FIOBJ i);
```

Frees a `FIOBJ` Float (a type specific `fiobj_free` alternative - use only when the type was validated).


### `FIOBJ` Strings

`FIOBJ` Strings are based on the core `STR_x` functions. This means that all these core type functions are available also for this type, using the `fiobj_str` prefix (i.e., [`STR_new`](#str_new) becomes [`fiobj_str_new`](#str_new), [`STR_write`](#str_write) becomes [`fiobj_str_write`](#str_write), etc').

In addition, the following `fiobj_str` functions and MACROs are defined:

#### `fiobj_str_new_cstr`

```c
FIOBJ fiobj_str_new_cstr(const char *ptr, size_t len);
```

Creates a new `FIOBJ` string object, copying the data to the new string.


#### `fiobj_str_new_buf`

```c
FIOBJ fiobj_str_new_buf(size_t capa);
```

Creates a new `FIOBJ` string object with (at least) the requested capacity.


#### `fiobj_str_new_copy`

```c
FIOBJ fiobj_str_new_copy(FIOBJ original);
```

Creates a new `FIOBJ` string object, copying the origin ([`fiobj2cstr`](#fiobj2cstr)).


#### `fiobj_str2cstr`

```c
fio_str_info_s fiobj_str2cstr(FIOBJ s);
```

Returns information about the string. Same as [`fiobj_str_info()`](#str_info).

#### `FIOBJ_STR_TEMP_DESTROY(name)`

```c
#define FIOBJ_STR_TEMP_DESTROY(str_name)  \
  FIO_NAME(fiobj_str, destroy)(str_name);
```

Resets a temporary `FIOBJ` String, freeing and any resources allocated.

See the following `FIOBJ_STR_TEMP_XXX` macros for creating temporary FIOBJ strings on the Stack.

#### `FIOBJ_STR_TEMP_VAR(name)`

```c
#define FIOBJ_STR_TEMP_VAR(str_name)                                   \
  struct {                                                             \
    uint64_t i1;                                                       \
    uint64_t i2;                                                       \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;               \
  } FIO_NAME(str_name, __auto_mem_tmp) = {                             \
      0x7f7f7f7f7f7f7f7fULL, 0x7f7f7f7f7f7f7f7fULL, FIO_STR_INIT};     \
  FIOBJ str_name =                                                     \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) | \
              FIOBJ_T_STRING);
```

Creates a temporary `FIOBJ` String object on the stack.

String data might be allocated dynamically, requiring the use of `FIOBJ_STR_TEMP_DESTROY`.

#### `FIOBJ_STR_TEMP_VAR_STATIC(str_name, buf, len)`

```c
#define FIOBJ_STR_TEMP_VAR_STATIC(str_name, buf_, len_)                        \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name,                                                         \
             __auto_mem_tmp) = {0x7f7f7f7f7f7f7f7fULL,                         \
                                0x7f7f7f7f7f7f7f7fULL,                         \
                                FIO_STR_INIT_STATIC2((buf_), (len_))};         \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);
```

Creates a temporary FIOBJ String object on the stack, initialized with a static string.

Editing the String data **will** cause dynamic memory allocation, use `FIOBJ_STR_TEMP_DESTROY` once done.

This variation will cause memory allocation immediately upon editing the String. The buffer _MAY_ be read only.

#### `FIOBJ_STR_TEMP_VAR_EXISTING(str_name, buf, len, capa)`

```c
#define FIOBJ_STR_TEMP_VAR_EXISTING(str_name, buf_, len_, capa_)               \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name, __auto_mem_tmp) = {                                     \
      0x7f7f7f7f7f7f7f7fULL,                                                   \
      0x7f7f7f7f7f7f7f7fULL,                                                   \
      FIO_STR_INIT_EXISTING((buf_), (len_), (capa_))};                         \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);
```

Creates a temporary FIOBJ String object on the stack for a read/write buffer with the specified capacity.

Editing the String data might cause dynamic memory allocation, use `FIOBJ_STR_TEMP_DESTROY` once done.

Remember to manage the buffer's memory once it was de-linked from the temporary string (as the FIOBJ object does **not** take ownership of the memory).

#### `FIOBJ` Strings - Core Type Functions

In addition, all the functions documented above as `STR_x`, are defined as `fiobj_str_x`:

* [`fiobj_str_new`](#str_new) - creates a new empty string.

* [`fiobj_str_free`](#str_free) - frees a FIOBJ known to be a String object.

* [`fiobj_str_destroy`](#str_destroy) - destroys / clears a String, returning it to an empty state. 

* [`fiobj_str_detach`](#str_detach) - destroys / clears a String, returning a `char *` C-String.

* [`fiobj_str_info`](#str_info) - returns information about the string.

* [`fiobj_str_len`](#str_len) - returns the string's length.

* [`fiobj_str2ptr`](#str2ptr) - returns a pointer to the string's buffer.

* [`fiobj_str_capa`](#str_capa) - returns the string's capacity.

* [`fiobj_str_freeze`](#str_freeze) - freezes a string (a soft flag, enforced only by functions).

* [`fiobj_str_is_frozen`](#str_is_frozen) - returns true if the string is frozen.

* [`fiobj_str_is_eq`](#str_is_eq) - returns true if the strings are equal.

* [`fiobj_str_hash`](#str_hash) - returns a string's Risky Hash.

* [`fiobj_str_resize`](#str_resize) - resizes a string (keeping the current buffer).

* [`fiobj_str_compact`](#str_compact) - attempts to minimize memory usage.

* [`fiobj_str_reserve`](#str_reserve) - reserves memory for future `write` operations.

* [`fiobj_str_utf8_valid`](#str_utf8_valid) - tests in a string is UTF8 valid.

* [`fiobj_str_utf8_len`](#str_utf8_len) - returns a string's length in UTF8 characters.

* [`fiobj_str_utf8_select`](#str_utf8_select) - selects a section of the string using UTF8 offsets.

* [`fiobj_str_write`](#str_write) - writes data to the string.

* [`fiobj_str_write_i`](#str_write_i) - writes a base 10 number to the string.

* [`fiobj_str_write_hex`](#str_write_hex) - writes a base 16 (hex) number to the string.

* [`fiobj_str_concat`](#str_concat-str_join) - writes an existing string to the string.

* [`fiobj_str_replace`](#str_replace) - replaces a section of the string.

* [`fiobj_str_vprintf`](#str_vprintf) - writes formatted data to the string.

* [`fiobj_str_printf`](#str_printf) - writes formatted data to the string.

* [`fiobj_str_readfd`](#str_readfd) - writes data from an open file to the string.

* [`fiobj_str_readfile`](#str_readfile) - writes data from an unopened file to the string.

* [`fiobj_str_write_b64enc`](#str_write_b64enc) - encodes and writes data to the string using base 64.

* [`fiobj_str_write_b64dec`](#str_write_b64dec) - decodes and writes data to the string using base 64.

* [`fiobj_str_write_escape`](#str_write_escape) - writes JSON style escaped data to the string.

* [`fiobj_str_write_unescape`](#str_write_unescape) - writes decoded JSON escaped data to the string.


### `FIOBJ` Arrays

`FIOBJ` Arrays are based on the core `ARY_x` functions. This means that all these core type functions are available also for this type, using the `fiobj_array` prefix (i.e., [`ARY_new`](#ary_new) becomes [`fiobj_array_new`](#ary_new), [`ARY_push`](#ary_push) becomes [`fiobj_array_push`](#ary_push), etc').

These functions include:

* [`fiobj_array_new`](#ary_new)

* [`fiobj_array_free`](#ary_free)

* [`fiobj_array_destroy`](#ary_destroy)

* [`fiobj_array_count`](#ary_count)

* [`fiobj_array_capa`](#ary_capa)

* [`fiobj_array_reserve`](#ary_reserve)

* [`fiobj_array_concat`](#ary_concat)

* [`fiobj_array_set`](#ary_set)

* [`fiobj_array_get`](#ary_get)

* [`fiobj_array_find`](#ary_find)

* [`fiobj_array_remove`](#ary_remove)

* [`fiobj_array_remove2`](#ary_remove2)

* [`fiobj_array_compact`](#ary_compact)

* [`fiobj_array_to_a`](#ary_to_a)

* [`fiobj_array_push`](#ary_push)

* [`fiobj_array_pop`](#ary_pop)

* [`fiobj_array_unshift`](#ary_unshift)

* [`fiobj_array_shift`](#ary_shift)

* [`fiobj_array_each`](#ary_each)

### `FIOBJ` Ordered Hash Maps

`FIOBJ` Ordered Hash Maps are based on the core `MAP_x` functions. This means that all these core type functions are available also for this type, using the `fiobj_hash` prefix (i.e., [`MAP_new`](#map_new) becomes [`fiobj_hash_new`](#map_new), [`MAP_set`](#map_set) becomes [`fiobj_hash_set`](#map_set), etc').

In addition, the following `fiobj_hash` functions and MACROs are defined:

#### `fiobj2hash`

```c
uint64_t fiobj2hash(FIOBJ target_hash, FIOBJ value);
```

Calculates an object's hash value for a specific hash map object.

#### `fiobj_hash_set2`

```c
FIOBJ fiobj_hash_set2(FIOBJ hash, FIOBJ key, FIOBJ value);
```

Inserts a value to a hash map, with a default hash value calculation.

#### `fiobj_hash_set_if_missing2`

```c
FIOBJ fiobj_hash_set_if_missing2(FIOBJ hash, FIOBJ key, FIOBJ value);
```

Inserts a value to a hash map, with a default hash value calculation.

If the key already exists in the Hash Map, the value will be freed instead.

#### `fiobj_hash_get2`

```c
FIOBJ fiobj_hash_get2(FIOBJ hash, FIOBJ key);
```

Finds a value in a hash map, with a default hash value calculation.

#### `fiobj_hash_remove2`

```c
int fiobj_hash_remove2(FIOBJ hash, FIOBJ key, FIOBJ *old);
```

Removes a value from a hash map, with a default hash value calculation.

#### `fiobj_hash_set3`

```c
FIOBJ fiobj_hash_set3(FIOBJ hash, const char *key, size_t len, FIOBJ value);
```

Sets a value in a hash map, allocating the key String and automatically calculating the hash value.

#### `fiobj_hash_get3`

```c
FIOBJ fiobj_hash_get3(FIOBJ hash, const char *buf, size_t len);
```

Finds a String value in a hash map, using a temporary String as the key and automatically calculating the hash value.

#### `fiobj_hash_remove3`

```c
int fiobj_hash_remove3(FIOBJ hash, const char *buf, size_t len, FIOBJ *old);
```

Removes a String value in a hash map, using a temporary String as the key and automatically calculating the hash value.

#### `FIOBJ` Hash Map - Core Type Functions

In addition, all the functions documented above as `MAP_x`, are defined as `fiobj_hash_x`:

* [`fiobj_hash_new`](#map_new)

* [`fiobj_hash_free`](#map_free)

* [`fiobj_hash_destroy`](#map_destroy)

* [`fiobj_hash_get`](#map_get-hash-map)

* [`fiobj_hash_get_ptr`](#map_get_ptr)

* [`fiobj_hash_set`](#map_set)

* [`fiobj_hash_set_ptr`](#map_set_ptr)

* [`fiobj_hash_remove`](#map_remove)

* [`fiobj_hash_evict`](#map_evict)

* [`fiobj_hash_count`](#map_count)

* [`fiobj_hash_capa`](#map_capa)

* [`fiobj_hash_reserve`](#map_reserve)

* [`fiobj_hash_compact`](#map_compact)

* [`fiobj_hash_rehash`](#map_rehash)

* [`fiobj_hash_each`](#map_each)

* [`fiobj_hash_each_get_key`](#map_each_get_key)

### `FIOBJ` JSON Helpers

Parsing, editing and outputting JSON in C can be easily accomplished using `FIOBJ` types.

`facil.io` offers the added benefit of complete parsing from JSON to object. This allows the result to be manipulated, updated, sliced or merged with ease. This is in contrast to some parsers that offer a mid-way structures or lazy (delayed) parsing for types such as `true`, `false` and Numbers.

`facil.io` also offers the added benefit of complete formatting from a framework wide object type (`FIOBJ`) to JSON, allowing the same soft type system to be used throughout the project (rather than having a JSON dedicated type system).

This is in addition to `facil.io` support to some JSON extensions such as comments, both C style (both `//` and `/* ... */` and bash style (`#`).

However, there are [faster alternatives as well as slower alternatives out there](json_performance.html) (i.e., the [Qajson4c library](https://github.com/DeHecht/qajson4c) is a wonderful alternative for embedded systems).

#### `fiobj2json`

```c
FIOBJ fiobj2json(FIOBJ dest, FIOBJ o, uint8_t beautify);
```

Returns a JSON valid FIOBJ String, representing the object.

If `dest` is an existing String, the formatted JSON data will be appended to the existing string.

```c
FIOBJ result = fiobj_json_parse2("{\"name\":\"John\",\"surname\":\"Smith\",\"ID\":1}",40, NULL);
FIO_ASSERT( fiobj2cstr(fiobj_hash_get3(result, "name", 4)).len == 4 &&
            !memcmp(fiobj2cstr(fiobj_hash_get3(result, "name", 4)).buf, "John", 4), "result error");

FIOBJ_STR_TEMP_VAR(json_str); /* places string on the stack */
fiobj2json(json_str, result, 1);
FIO_LOG_INFO("updated JSON data to look nicer:\n%s", fiobj2cstr(json_str).buf);
fiobj_free(result);
FIOBJ_STR_TEMP_DESTROY(json_str);
```

#### `fiobj_hash_update_json`

```c
size_t fiobj_hash_update_json(FIOBJ hash, fio_str_info_s str);

size_t fiobj_hash_update_json2(FIOBJ hash, char *ptr, size_t len);
```

Updates a Hash using JSON data.

Parsing errors and non-dictionary object JSON data are silently ignored, attempting to update the Hash as much as possible before any errors encountered.

Conflicting Hash data is overwritten (preferring the new over the old).

Returns the number of bytes consumed. On Error, 0 is returned and no data is consumed.

The `fiobj_hash_update_json2` function is a helper function, it calls `fiobj_hash_update_json` with the provided string information.

#### `fiobj_json_parse`

```c
FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed);

#define fiobj_json_parse2(data_, len_, consumed)                      \
  fiobj_json_parse((fio_str_info_s){.buf = data_, .len = len_}, consumed)
```

Parses a C string for JSON data. If `consumed` is not NULL, the `size_t` variable will contain the number of bytes consumed before the parser stopped (due to either error or end of a valid JSON data segment).

Returns a FIOBJ object matching the JSON valid C string `str`.

If the parsing failed (no complete valid JSON data) `FIOBJ_INVALID` is returned.

`fiobj_json_parse2` is a helper macro, it calls `fiobj_json_parse` with the provided string information.

### How to Extend the `FIOBJ` Type System

The `FIOBJ` source code includes two extensions for the `Float` and `Number` types.

In many cases, numbers and floats can be used without memory allocations. However, when memory allocation is required to store the data, the `FIOBJ_T_NUMBER` and `FIOBJ_T_FLOAT` types are extended using the same techniques described here.

#### `FIOBJ` Extension Requirements

To extend the `FIOBJ` soft type system, there are a number of requirements:

1. A **unique** type ID must be computed.

    Type IDs are `size_t` bits in length. Values under 100 are reserved. Values under 40 are illegal (might break implementation).

2. A static virtual function table object (`FIOBJ_class_vtable_s`) must be fully populated (`NULL` values may break cause a segmentation fault).

3. The unique type construct / destructor must be wrapped using the facil.io reference counting wrapper (using `FIO_REF_NAME`).

    The `FIO_REF_METADATA` should be set to a `FIOBJ_class_vtable_s` pointer and initialized for every object.

4. The unique type wrapper must use pointer tagging as described bellow (`FIO_PTR_TAG`).

5. A public API should be presented.

#### `FIOBJ` Pointer Tagging

The `FIOBJ` types is often identified by th a bit "tag" added to the pointer. All extension types **must** be tagged as `FIOBJ_T_OTHER`.

The facil.io memory allocator (`fio_malloc`), as well as most system allocators, promise a 64 bit allocation alignment. The `FIOBJ` types leverage this behavior by utilizing the least significant 3 bits that are always zero. However, this implementation might change in the future, so it's better to use the macros `FIOBJ_PTR_TAG` and `FIOBJ_PTR_UNTAG`.

The following macros should be defined for tagging an extension `FIOBJ` type, allowing the `FIO_REF_NAME` constructor / destructor to manage pointer tagging, reference counting and access to the `FIOBJ` virtual table (see later on).

```c
#define FIO_PTR_TAG(p)   FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
```

#### `FIOBJ` Virtual Function Tables

`FIOBJ` extensions use a virtual function table that is shared by all the objects of that type/class.

Basically, the virtual function table is a `struct` with the **Type ID** and function pointers.

**Type ID** values under 100 are reserved for facil.io and might cause conflicts with the existing type values if used (i.e., `FIOBJ_T_FALSE == 34`).

All function pointers must be populated (where `each1` is only called if `count` returns a non-zero value).

This is the structure of the virtual table:

```c
/** FIOBJ types can be extended using virtual function tables. */
typedef struct {
  /** A unique number to identify object type. */
  size_t type_id;
  /** Test for equality between two objects with the same `type_id` */
  unsigned char (*is_eq)(FIOBJ a, FIOBJ b);
  /** Converts an object to a String */
  fio_str_info_s (*to_s)(FIOBJ o);
  /** Converts an object to an integer */
  intptr_t (*to_i)(FIOBJ o);
  /** Converts an object to a double */
  double (*to_f)(FIOBJ o);
  /** Returns the number of exposed elements held by the object, if any. */
  uint32_t (*count)(FIOBJ o);
  /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
  uint32_t (*each1)(FIOBJ o, int32_t start_at,
                    int (*task)(FIOBJ child, void *arg), void *arg);
  /**
   * Decreases the reference count and/or frees the object, calling `free2` for
   * any nested objects.
   *
   * Returns 0 if the object is still alive or 1 if the object was freed. The
   * return value is currently ignored, but this might change in the future.
   */
  int (*free2)(FIOBJ o);
} FIOBJ_class_vtable_s;
```

#### `FIOBJ` Extension Example

For our example, let us implement a static string extension type. We will use the Type ID 100 because values under 100 are reserved.

Let's call our example header file `fiobj_static.h`, so we can find it later.

The API for this type and the header might look something like this:

```c
#ifndef FIO_STAT_STRING_HEADER_H
/* *****************************************************************************
FIOBJ Static String Extension Header Example
***************************************************************************** */
#define FIO_STAT_STRING_HEADER_H
/* *****************************************************************************
Perliminaries - include the FIOBJ extension, but not it's implementation
***************************************************************************** */
#define FIO_EXTERN
#define FIOBJ_EXTERN
#define FIO_FIOBJ
#include "fio-stl.h"

/* *****************************************************************************
Defining the Type ID and the API
***************************************************************************** */

/** The Static String Type ID */
#define FIOBJ_T_STATIC_STRING 100UL

/** Returns a new static string object. The string is considered immutable. */
FIOBJ fiobj_static_new(const char *str, size_t len);

/** Returns a pointer to the static string. */
const char *fiobj_static2ptr(FIOBJ s);

/** Returns the static strings length. */
size_t fiobj_static_len(FIOBJ s);

#endif
```

**Note**: The header assumes that _somewhere_ there's a C implementation file that includes the `FIOBJ` implementation. That C file defines the `FIO_EXTERN_COMPLETE` macro **before** including the `fio-stl.h` file (as well as defining `FIO_FIOBJ` and `FIO_EXTERN`).

The implementation may look like this.

```c
/* *****************************************************************************
FIOBJ Static String Extension Implementation Example
***************************************************************************** */
#include <fiobj_static.h> // include the header file here, whatever it's called

/* *****************************************************************************
The Virtual Function Table (definitions and table)
***************************************************************************** */

/** Test for equality between two objects with the same `type_id` */
static unsigned char static_string_is_eq(FIOBJ a, FIOBJ b);
/** Converts an object to a String */
static fio_str_info_s static_string_to_s(FIOBJ o);
/** Converts an object to an integer */
static intptr_t static_string_to_i(FIOBJ o);
/** Converts an object to a double */
static double static_string_to_f(FIOBJ o);
/** Returns the number of exposed elements held by the object, if any. */
static uint32_t static_string_count(FIOBJ o);
/** Iterates the exposed elements held by the object. See `fiobj_each1`. */
static uint32_t static_string_each1(FIOBJ o, int32_t start_at,
                                    int (*task)(FIOBJ, void *), void *arg);
/**
 * Decreases the reference count and/or frees the object, calling `free2` for
 * any nested objects (which we don't have for this type).
 *
 * Returns 0 if the object is still alive or 1 if the object was freed. The
 * return value is currently ignored, but this might change in the future.
 */
static int static_string_free2(FIOBJ o);

/** The virtual function table object. */
static const FIOBJ_class_vtable_s FIOBJ___STATIC_STRING_VTABLE = {
    .type_id = FIOBJ_T_STATIC_STRING,
    .is_eq = static_string_is_eq,
    .to_s = static_string_to_s,
    .to_i = static_string_to_i,
    .to_f = static_string_to_f,
    .count = static_string_count,
    .each1 = static_string_each1,
    .free2 = static_string_free2,
};

/* *****************************************************************************
The Static String Type (internal implementation)
***************************************************************************** */

/* leverage the small-string type to hold static string data */
#define FIO_STR_SMALL fiobj_static_string
/* add required pointer tagging */
#define FIO_PTR_TAG(p)   FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
/* add required reference counter / wrapper type */
#define FIO_REF_NAME fiobj_static_string
#define FIO_REF_CONSTRUCTOR_ONLY
/* initialization - for demonstration purposes, we don't use it here. */
#define FIO_REF_INIT(o)                                                        \
  do {                                                                         \
    o = (fiobj_static_string_s){0};                                            \
    FIOBJ_MARK_MEMORY_ALLOC(); /* mark memory allocation for debugging */      \
  } while (0)
/* cleanup - destroy the object data when the reference count reaches zero. */
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
    fiobj_static_string_destroy((FIOBJ)&o);                                    \
    FIOBJ_MARK_MEMORY_FREE(); /* mark memory deallocation for debugging */     \
  } while (0)
/* metadata (vtable) definition and initialization. */
#define FIO_REF_METADATA const FIOBJ_class_vtable_s *
/* metadata initialization - required to initialize the vtable. */
#define FIO_REF_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___STATIC_STRING_VTABLE;                                         \
  } while (0)
#include <fio-stl.h>

/* *****************************************************************************
The Public API
***************************************************************************** */

/** Returns a new static string object. The string is considered immutable. */
FIOBJ fiobj_static_new(const char *str, size_t len) {
  FIOBJ o = fiobj_static_string_new();
  FIO_ASSERT_ALLOC(FIOBJ_PTR_UNTAG(o));
  fiobj_static_string_init_const(o, str, len);
  return o;
}

/** Returns a pointer to the static string. */
const char *fiobj_static2ptr(FIOBJ o) { return fiobj_static_string2ptr(o); }

/** Returns the static strings length. */
size_t fiobj_static_len(FIOBJ o) { return fiobj_static_string_len(o); }

/* *****************************************************************************
Virtual Function Table Implementation
***************************************************************************** */

/** Test for equality between two objects with the same `type_id` */
static unsigned char static_string_is_eq(FIOBJ a, FIOBJ b) {
  fio_str_info_s ai, bi;
  ai = fiobj_static_string_info(a);
  bi = fiobj_static_string_info(b);
  return (ai.len == bi.len && !memcmp(ai.buf, bi.buf, ai.len));
}
/** Converts an object to a String */
static fio_str_info_s static_string_to_s(FIOBJ o) {
  return fiobj_static_string_info(o);
}
/** Converts an object to an integer */
static intptr_t static_string_to_i(FIOBJ o) {
  fio_str_info_s s = fiobj_static_string_info(o);
  if (s.len)
    return fio_atol(&s.buf);
  return 0;
}
/** Converts an object to a double */
static double static_string_to_f(FIOBJ o) {
  fio_str_info_s s = fiobj_static_string_info(o);
  if (s.len)
    return fio_atof(&s.buf);
  return 0.0L;
}
/** Returns the number of exposed elements held by the object, if any. */
static uint32_t static_string_count(FIOBJ o) {
  return 0;
  (void)o;
}
/** Iterates the exposed elements held by the object. See `fiobj_each1`. */
static uint32_t static_string_each1(FIOBJ o, int32_t start_at,
                                    int (*task)(FIOBJ, void *), void *arg) {
  return 0;
  (void)o; (void)start_at; (void)task; (void)arg;
}
/** Decreases the reference count and/or frees the object. */
static int static_string_free2(FIOBJ o) { return fiobj_static_string_free(o); }
```

Example usage:

```c
#define FIO_EXTERN_COMPLETE   // we will place the FIOBJ implementation here.
#include "fiobj_static.h"     // include FIOBJ extension type
int main(void) {
  FIOBJ o = fiobj_static_new("my static string", 16);
  /* example test of virtual table redirection */
  FIO_ASSERT(fiobj2cstr(o).buf == fiobj_static2ptr(o) &&
                 fiobj2cstr(o).len == fiobj_static_len(o),
             "vtable redirection error.");
  fprintf(stderr, "allocated: %s\n", fiobj_static2ptr(o));
  fprintf(stderr, "it's %zu byte long\n", fiobj_static_len(o));
  fprintf(stderr, "object type: %zu\n", FIOBJ_TYPE(o));
  fiobj_free(o);
  FIOBJ_MARK_MEMORY_PRINT(); /* only in DEBUG mode */
}
```

-------------------------------------------------------------------------------

## IO Reactor - an Evented, Single-Threaded, IO Reactor

```c
#define FIO_IO
#include FIO_INCLUDE_FILE
```

An IO reactor - evented and single-threaded - is included when `FIO_IO` is defined.

Although many IO API calls are thread safe (they actually schedule events on the main thread), some thread safety concerns still apply.

**Note**: this will automatically include a large amount of the facil.io STL modules, which you may prefer to manually include beforehand in order to choose the appropriate memory allocator per module.

### Time Server Example

The following example uses the `FIO_PUBSUB` module together with the `FIO_IO` module to author a very simplistic time server (with no micro-second accuracy).

the `FIO_PUBSUB` module could have been replaced with a `fio_protocol_each` approach, assuming a single threaded implementation. But this approach is both simpler and (usually) more powerful.

```c
#define FIO_LOG
#define FIO_IO
#define FIO_PUBSUB
#define FIO_TIME
#include "fio-stl/include.h"

/** Called when an IO is attached to a protocol. */
FIO_SFUNC void time_protocol_on_attach(fio_io_s *io) {
  /* .on_message is unnecessary, by default the message is sent to the IO. */
  fio_subscribe(.io = io, .channel = FIO_BUF_INFO1("time"));
}

fio_io_protocol_s TIME_PROTOCOL = {
    .on_attach = time_protocol_on_attach,     /* subscribe after connection */
    .on_timeout = fio_io_touch,               /* never times out */
    .on_pubsub = FIO_ON_MESSAGE_SEND_MESSAGE, /* write messages to IO */
};

/* timer callback for publishing time */
static int publish_time(void *ignore1_, void *ignore2_) {
  char buf[32];
  size_t len = fio_time2iso(buf, fio_time_real().tv_sec);
  buf[len++] = '\r';
  buf[len++] = '\n';
  fio_publish(.channel = FIO_BUF_INFO1("time"),
              .message = FIO_BUF_INFO2(buf, len));
  return 0;
  (void)ignore1_, (void)ignore2_;
}

int main(void) {
  fio_io_run_every(.fn = publish_time, .every = 1000, .repetitions = -1);
  FIO_ASSERT(fio_io_listen(.protocol = &TIME_PROTOCOL), "");
  printf("* Time service starting up.\n");
  printf("  Press ^C to stop server and exit.\n");
  fio_io_start(0);
}
```

### IO Compiler Settings

The following macros control the IO reactor's behavior during compile-time.

#### `FIO_IO_BUFFER_PER_WRITE`

```c
#define FIO_IO_BUFFER_PER_WRITE 65536U
```

Control the size of the on-stack buffer used for `write` events.


#### `FIO_IO_THROTTLE_LIMIT`

```c
#define FIO_IO_THROTTLE_LIMIT 2097152U
```

IO will be throttled (no `on_data` events) if outgoing buffer is large.


#### `FIO_IO_TIMEOUT_MAX`

```c
#define FIO_IO_TIMEOUT_MAX 300000
```

Controls the maximum and default timeout in milliseconds (5 minutes).


#### `FIO_IO_SHUTDOWN_TIMEOUT`

```c
#define FIO_IO_SHUTDOWN_TIMEOUT 10000
```

Sets the hard timeout (in milliseconds) for the reactor's shutdown loop.


#### `FIO_IO_COUNT_STORAGE`

```c
#ifdef DEBUG
#define FIO_IO_COUNT_STORAGE 1
#else
#define FIO_IO_COUNT_STORAGE 0
#endif
```

When enabled, reports total number of bytes sent / received by an IO object.

### IO Reactor Types

The following types are at the core of the IO Recator's design choices.

#### `fio_io_protocol_s`

```c
typedef struct fio_io_protocol_s fio_io_protocol_s;

struct fio_io_protocol_s {
  /**
   * Reserved / private data - used by facil.io internally.
   * MUST be initialized to zero.
   */
  struct {
    /* A linked list of currently attached IOs (ordered) - do NOT alter. */
    FIO_LIST_HEAD ios;
    /* A linked list of other protocols used by IO core - do NOT alter. */
    FIO_LIST_NODE protocols;
    /* internal flags - do NOT alter after initial initialization to zero. */
    uintptr_t flags;
  } reserved;
  /** Called when an IO is attached to the protocol. */
  void (*on_attach)(fio_io_s *io);
  /** Called when a data is available. */
  void (*on_data)(fio_io_s *io);
  /** called once all pending `fio_write` calls are finished. */
  void (*on_ready)(fio_io_s *io);

  /**
   * Called when the IO reactor is shutting down, immediately before closing the
   * connection.
   *
   * After the `on_shutdown` callback returns, the socket is marked for closure.
   *
   * Once the socket was marked for closure, facil.io will allow a limited
   * amount of time for data to be sent, after which the socket might be closed
   * even if the client did not consume all buffered data.
   */
  void (*on_shutdown)(fio_io_s *io);
  /**
   * Called when a connection's timeout was reached
   * 
   * Can be set to `fio_io_touch` if timeout is irrelevant (i.e., listening / UDP).
   */
  void (*on_timeout)(fio_io_s *io);
  /** Used as a default `on_message` when an IO object subscribes. */

  /** Called after the connection was closed (once per IO). */
  void (*on_close)(void *iobuf, void *udata);

  void (*on_pubsub)(struct fio_msg_s *msg);
  /** Allows user specific protocol agnostic callbacks. */
  void (*on_user1)(fio_io_s *io, void *user_data);
  /** Allows user specific protocol agnostic callbacks. */
  void (*on_user2)(fio_io_s *io, void *user_data);
  /** Allows user specific protocol agnostic callbacks. */
  void (*on_user3)(fio_io_s *io, void *user_data);
  /** Reserved for future protocol agnostic callbacks. */
  void (*on_reserved)(fio_io_s *io, void *user_data);
  /**
   * Defines Transport Layer callbacks that facil.io will treat as non-blocking
   * system calls.
   */
  fio_io_functions_s io_functions;
  /**
   * The timeout value in milliseconds for all connections using this protocol.
   *
   * Limited to FIO_IO_TIMEOUT_MAX seconds. Zero (0) == FIO_IO_TIMEOUT_MAX
   */
  uint32_t timeout;
  /** The number of bytes to allocate for the fio_io_buf buffer. */
  uint32_t buffer_size;
};
```

The Protocol struct defines the callbacks used for a family of connections and sets their behavior. The Protocol struct is part of the IO reactor's core design.

Protocols are usually global objects and the same protocol can be assigned to multiple IO handles. There are, of course, exceptions.

#### `fio_io_functions_s`

```c
typedef struct fio_io_functions_s fio_io_functions_s;

struct fio_io_functions_s {
  /** Helper that converts a `fio_io_tls_s` into the implementation's context.
   */
  void *(*build_context)(fio_io_tls_s *tls, uint8_t is_client);
  /** Helper to free the context built by build_context. */
  void (*free_context)(void *context);
  /** called when a new IO is first attached to a valid protocol. */
  void (*start)(fio_io_s *io);
  /** Called to perform a non-blocking `read`, same as the system call. */
  ssize_t (*read)(int fd, void *buf, size_t len, void *context);
  /** Called to perform a non-blocking `write`, same as the system call. */
  ssize_t (*write)(int fd, const void *buf, size_t len, void *context);
  /** Sends any unsent internal data. Returns 0 only if all data was sent. */
  int (*flush)(int fd, void *context);
  /** Called when the IO object finished sending all data before closure. */
  void (*finish)(int fd, void *context);
  /** Called after the IO object is closed, used to cleanup its `tls` object. */
  void (*cleanup)(void *context);
};
```

The Protocol struct uses IO callbacks to allow an easy way to override the system's IO functions and emulate POSIX style IO functions.

This defines Transport Layer callbacks that facil.io will treat as non-blocking system calls and allows any protocol to easily add a secure (SSL/TLS) flavor if desired.


#### `fio_io_s`

```c
typedef struct fio_io_s fio_io_s;
```

The main IO object type. Should be treated as an opaque pointer.

The IO handle is used instead of the system's file descriptor and protects callbacks and IO operations from sending data to incorrect clients (possible `fd` "recycling").

#### `fio_io_tls_s`

```c
typedef struct fio_io_tls_s fio_io_tls_s;
```

An opaque type used for the SSL/TLS helper functions.

#### `fio_msg_s` (Pub/Sub)

```c
typedef struct fio_msg_s fio_msg_s;

struct fio_msg_s {
  /** A connection (if any) to which the subscription belongs. */
  fio_io_s *io;
  /** The `udata` argument associated with the subscription. */
  void *udata;
  /** Message ID. */
  uint64_t id;
  /** Milliseconds since epoch. */
  uint64_t published;
  /**
   * A channel name, allowing for pub/sub patterns.
   *
   * NOTE: this is a shared copy - do NOT mutate the channel name string.
   */
  fio_buf_info_s channel;
  /**
   * The actual message.
   *
   * NOTE: this is a shared copy - do NOT mutate the message payload string.
   **/
  fio_buf_info_s message;
  /** Channel name namespace. Negative values are reserved. */
  int16_t filter;
  /** flag indicating if the message is JSON data or binary/text. */
  uint8_t is_json;
};
```

The pub/sub message structure, as received by the `on_message` subscription callback.

This is detailed in the pub/sub section, but listed here as it is used by the `fio_io_protocol_s` callback for a default pub/sub handler.

#### `fio_io_async_s`

```c
typedef struct fio_io_async_s fio_io_async_s;

struct fio_io_async_s {
  fio_queue_s *q;
  uint32_t count;
  fio_queue_s queue;
  FIO_LIST_NODE node;
};

```

The IO Async Queue type. 

IO Async Queue provides Worker Threads for non-IO tasks and is integrated into the IO reactor's start / stop life cycle.

### Starting / Stopping the IO Reactor


#### `fio_io_stop`
```c
void fio_io_stop(void);
```

Stopping the IO reactor.

#### `fio_io_add_workers`
```c
void fio_io_add_workers(int workers);
```

Adds `workers` amount of workers to the root IO reactor process.

#### `fio_io_start`
```c
void fio_io_start(int workers);
```

Starts the IO reactor, using optional `workers` processes. Will BLOCK!

#### `fio_io_is_running`
```c
int fio_io_is_running(void);
```

Returns true if IO reactor running and 0 if stopped or shutting down.

#### `fio_io_is_master`
```c
int fio_io_is_master(void);
```

Returns true if the current process is the IO reactor's master process.

#### `fio_io_is_worker`
```c
int fio_io_is_worker(void);
```

Returns true if the current process is an IO reactor's worker process.

#### `fio_io_workers`
```c
uint16_t fio_io_workers(int workers_requested);
```

Returns the number or workers the IO reactor will actually run.

#### `fio_io_pid`
```c
int fio_io_pid(void);
```

Returns current process id.

#### `fio_io_root_pid`
```c
int fio_io_root_pid(void);
```

Returns the root / master process id.

#### `fio_io_last_tick`

```c
int64_t fio_io_last_tick(void);
```

Returns the last millisecond when the IO reactor polled for events.

### Listening to Incoming Connections

```c
void *fio_io_listen(fio_io_listen_args args);
/* Named arguments using macro. */
#define fio_io_listen(...) fio_io_listen((fio_io_listen_args){__VA_ARGS__})

typedef struct fio_io_listen_args {
  /**
   * The binding address in URL format. Defaults to: tcp://0.0.0.0:3000
   *
   * Note: `.url` accept an optional query for building a TLS context.
   *
   * Possible query values include:
   *
   * - `tls` or `ssl` (no value): sets TLS as active, possibly self-signed.
   * - `tls=` or `ssl=`: value is a prefix for "key.pem" and "cert.pem".
   * - `key=` and `cert=`: file paths for ".pem" files.
   *
   * i.e.:
   *
   *     fio_io_listen(.url = "0.0.0.0:3000/?tls", ...);
   *     fio_io_listen(.url = "0.0.0.0:3000/?tls=./", ...);
   *     // same as:
   *     fio_io_listen(.url = "0.0.0.0:3000/"
   *                            "?key=./key.pem"
   *                            "&cert=./cert.pem", ...);
   */
  const char *url;
  /** The `fio_io_protocol_s` that will be assigned to incoming
   * connections. */
  fio_io_protocol_s *protocol;
  /** The default `udata` set for (new) incoming connections. */
  void *udata;
  /** TLS object used for incoming connections (ownership moved to listener). */
  fio_io_tls_s *tls;
  /**
   * Called when the a listening socket starts to listen.
   *
   * May be called multiple times (i.e., if the IO reactor stops and restarts).
   */
  void (*on_start)(fio_io_protocol_s *protocol, void *udata);
  /**
   * Called during listener cleanup.
   *
   * This will be called separately for every process before exiting.
   */
  void (*on_stop)(fio_io_protocol_s *protocol, void *udata);
  /**
   * Selects a queue that will be used to schedule a pre-accept task.
   * May be used to test user thread stress levels before accepting connections.
   */
  fio_io_async_s *queue_for_accept;
  /** When forking the IO reactor - limits `listen` to the root process. */
  uint8_t on_root;
  /** Hides "started/stopped listening" messages from log (if set). */
  uint8_t hide_from_log;
} fio_io_listen_args;
```


Sets up a network service on a listening socket.

Returns a self-destructible listener handle on success or NULL on error.

The function is shadowed by a macro, allowing it to accept named arguments. i.e.:

```c
/* uses default values for everything except the protocol. */
fio_io_listen(.protocol = &MY_PROTOCOL);
```

#### `fio_io_listen_stop`

```c
void fio_io_listen_stop(void *listener);
```

Notifies a listener to stop listening.

#### `fio_io_listener_is_tls`

```c
int fio_io_listener_is_tls(void *listener);
```

Returns true if the listener protocol has an attached TLS context.

#### `fio_io_listener_protocol`

```c
fio_io_protocol_s *fio_io_listener_protocol(void *listener);
```

Returns the listener's associated protocol.

#### `fio_io_listener_udata`

```c
void *fio_io_listener_udata(void *listener);
```

Returns the listener's associated `udata`.

#### `fio_io_listener_udata_set`

```c
void *fio_io_listener_udata_set(void *listener, void *new_udata);
```

Sets the listener's associated `udata`, returning the old value.

#### `fio_io_listener_url`

```c
fio_buf_info_s fio_io_listener_url(void *listener);
```

Returns the URL on which the listener is listening.

### Connecting as a Client


```c
fio_io_s *fio_io_connect(fio_io_connect_args_s args);

#define fio_io_connect(url_, ...)                                              \
  fio_io_connect((fio_io_connect_args_s){.url = url_, __VA_ARGS__})

/** Named arguments for fio_io_connect */
typedef struct {
  /** The URL to connect to (may contain TLS hints in query / `tls` scheme). */
  const char *url;
  /** Connection protocol (once connection established). */
  fio_io_protocol_s *protocol;
  /** Called in case of a failed connection, use for cleanup. */
  void (*on_failed)(fio_io_protocol_s *protocol, void *udata);
  /** Opaque user data (set only once connection was established). */
  void *udata;
  /** TLS builder object for TLS connections. */
  fio_io_tls_s *tls;
  /** Connection timeout in milliseconds (defaults to 30 seconds). */
  uint32_t timeout;
} fio_io_connect_args_s;
```

Connects to a specific URL, returning the `fio_io_s` IO object or `NULL`.

When the connection is established, the protocol's `on_attach` will be called. Until then the IO handle is inactive and any IO operations may have unintended results.

### IO Operations

#### `fio_io_attach_fd`

```c
fio_io_s *fio_io_attach_fd(int fd,
                           fio_io_protocol_s *protocol,
                           void *udata,
                           void *tls);
```

Attaches the socket in `fd` to the facio.io engine (reactor).

* `fd` should point to a valid socket.

* `protocol` may be the existing protocol or NULL (for partial hijack).

* `udata` is opaque user data and may be any value, including NULL.

* `tls` is a context for Transport Layer (Security) and can be used to
  redirect read/write operations, as set by the protocol.

Returns NULL on error. the `fio_io_s` pointer must NOT be used except within proper callbacks.

**Note**: the protocol's `on_attach` will be called once the attachment of the `fd` is complete. Until then the IO handle is inactive and any IO operations may have unintended results.

#### `fio_io_dup`

```c
fio_io_s *fio_io_dup(fio_io_s *io);
```

Increases a IO's reference count, so it won't be automatically destroyed
when all tasks have completed.

Use this function in order to use the IO outside of a scheduled task.

This function is thread-safe.

#### `fio_io_free`

```c
void fio_io_free(fio_io_s *io);
```

Decreases a IO's reference count, so it could be automatically destroyed
when all other tasks have completed.

Use this function once finished with a IO that was `dup`-ed.

This function is thread-safe.

#### `fio_io_protocol_set`

```c
fio_io_protocol_s *fio_io_protocol_set(fio_io_s *io,
                                       fio_io_protocol_s *protocol);
```

Sets a new protocol object. `NULL` is a valid "only-write" protocol.

**Note**: the protocol's `on_attach` will be called once the new protocol is set. Until then the IO handle remains attached to the old protocol.

**Note**: the old protocol's `on_close` is NOT called. If desired, it should be called manually.

#### `fio_io_protocol`

```c
fio_io_protocol_s *fio_io_protocol(fio_io_s *io);
```

Returns a pointer to the current protocol object.

If `protocol` wasn't properly set, the pointer might be NULL or invalid.

If `protocol` wasn't attached yet, may return the previous protocol.

#### `fio_io_buffer`

```c
void *fio_io_buffer(fio_io_s *io);
```

Returns the a pointer to the memory buffer required by the protocol.

#### `fio_io_buffer_len`

```c
size_t fio_io_buffer_len(fio_io_s *io);
```

Returns the length of the `buf` buffer.

#### `fio_io_udata_set`

```c
void *fio_io_udata_set(fio_io_s *io, void *udata);
```

Associates a new `udata` pointer with the IO, returning the old `udata`

#### `fio_io_udata`

```c
void *fio_io_udata(fio_io_s *io);
```

Returns the `udata` pointer associated with the IO.

#### `fio_io_tls_set`

```c
void *fio_io_tls_set(fio_io_s *io, void *tls);
```

Associates a new `tls` pointer with the IO, returning the old `tls`

#### `fio_io_tls`

```c
void *fio_io_tls(fio_io_s *io);
```

Returns the `tls` pointer associated with the IO.

#### `fio_io_fd`

```c
int fio_io_fd(fio_io_s *io);
```

Returns the socket file descriptor (fd) associated with the IO.

#### `fio_io_touch`

```c
void fio_io_touch(fio_io_s *io);
```

Resets a socket's timeout counter.

#### `fio_io_read`

```c
size_t fio_io_read(fio_io_s *io, void *buf, size_t len);
```

Reads data to the buffer, if any data exists. Returns the number of bytes read.

**Note**: zero (`0`) is a valid return value meaning no data was available.

#### `fio_io_write2`

```c
void fio_io_write2(fio_io_s *io, fio_io_write_args_s args);
#define fio_io_write2(io, ...)                                                 \
  fio_io_write2(io, (fio_io_write_args_s){__VA_ARGS__})

typedef struct {
  /** The buffer with the data to send (if no file descriptor) */
  void *buf;
  /** The file descriptor to send (if no buffer) */
  intptr_t fd;
  /** The length of the data to be sent. On files, 0 = the whole file. */
  size_t len;
  /** The length of the data to be sent. On files, 0 = the whole file. */
  size_t offset;
  /**
   * If this is a buffer, the de-allocation function used to free it.
   *
   * If NULL, the buffer will NOT be de-allocated.
   */
  void (*dealloc)(void *);
  /** If non-zero, makes a copy of the buffer or keeps a file open. */
  uint8_t copy;
} fio_io_write_args_s;
```

Writes data to the outgoing buffer and schedules the buffer to be sent.

#### `fio_io_write`

```c
#define fio_io_write(io, buf_, len_)                                           \
  fio_io_write2(io, .buf = (buf_), .len = (len_), .copy = 1)
```

Helper macro for a common fio_io_write2 (copies the buffer).

#### `fio_io_sendfile`

```c
#define fio_io_sendfile(io, source_fd, offset_, bytes)                         \
  fio_io_write2((io),                                                          \
                .fd = (source_fd),                                             \
                .offset = (size_t)(offset_),                                   \
                .len = (bytes))
```

Sends data from a file as if it were a single atomic packet (sends up to length bytes or until EOF is reached).

Once the file was sent, the `source_fd` will be closed using `close`.

The file will be buffered to the socket chunk by chunk, so that memory consumption is capped.

`offset` dictates the starting point for the data to be sent and length sets the maximum amount of data to be sent.

Closes the file on error.

#### `fio_io_close`

```c
void fio_io_close(fio_io_s *io);
```

Marks the IO for closure as soon as scheduled data was sent.

#### `fio_io_close_now`

```c
void fio_io_close_now(fio_io_s *io);
```

Marks the IO for immediate closure.

#### `fio_io_suspend`

```c
void fio_io_suspend(fio_io_s *io);
```

Suspends future `on_data` events for the IO and **prevents** IO from being automatically closed during shutdown process (assumes IO object is waiting on an event or has a task scheduled).

#### `fio_io_unsuspend`

```c
void fio_io_unsuspend(fio_io_s *io);
```

Listens for future `on_data` events related to the IO, if shutting down, this will call `fio_io_close`).

#### `fio_io_is_suspended`

```c
int fio_io_is_suspended(fio_io_s *io);
```

Returns 1 if the IO handle was suspended.

#### `fio_io_is_open`

```c
int fio_io_is_open(fio_io_s *io);
```

Returns 1 if the IO handle is marked as open.

#### `fio_io_backlog`

```c
size_t fio_io_backlog(fio_io_s *io);
```

Returns the approximate number of bytes in the outgoing buffer.


### Task Scheduling

#### `fio_io_defer`

```c
void fio_io_defer(void (*task)(void *, void *),
                  void *udata1,
                  void *udata2);
```

Schedules a task for delayed execution. This function is thread-safe.

#### `fio_io_run_every`

```c
void fio_io_run_every(fio_timer_schedule_args_s args);
#define fio_io_run_every(...)                                                  \
  fio_io_run_every((fio_timer_schedule_args_s){__VA_ARGS__})

```

Schedules a timer bound task, see `fio_timer_schedule`.

Possible "named arguments" (`fio_timer_schedule_args_s` members) include:

* The timer function. If it returns a non-zero value, the timer stops:
  `int (*fn)(void *, void *)`
* Opaque user data:
  `void *udata1`
* Opaque user data:
  `void *udata2`
* Called when the timer is done (finished):
  `void (*on_stop)(void *, void *)`
* Timer interval, in milliseconds:
  `uint32_t every`
* The number of times the timer should be performed. -1 == infinity:
  `int32_t repetitions`


#### `fio_io_queue`

```c
fio_queue_s *fio_io_queue(void);
```

Returns a pointer for the IO reactor's queue.

#### `fio_io_protocol_each`

```c
size_t fio_io_protocol_each(fio_io_protocol_s *protocol,
                            void (*task)(fio_io_s *, void *udata2),
                            void *udata2);
```

Performs a task for each IO in the stated protocol.

### Connection Object Links / Environment

Each IO handle contains an "environment", which is a key-value store where String keys are used to store arbitrary data that gets destroyed along with the `io` handle.

#### `fio_io_env_get`

```c
void *fio_io_env_get(fio_io_s *io, fio_io_env_get_args_s);

#define fio_io_env_get(io, ...)                                                \
  fio_io_env_get(io, (fio_io_env_get_args_s){__VA_ARGS__})

/** Named arguments for the `fio_io_env_get` / `fio_io_env_unset` function. */
typedef struct {
  /** A numerical type filter. Should be the same as used with
   * `fio_io_env_set` */
  intptr_t type;
  /** The name of the object. Should be the same as used with `fio_io_env_set`
   */
  fio_buf_info_s name;
} fio_io_env_get_args_s;
```

Returns the named `udata` associated with the IO object (or `NULL`).

#### `fio_io_env_set`

```c
void fio_io_env_set(fio_io_s *io, fio_io_env_set_args_s);

#define fio_io_env_set(io, ...)                                                \
  fio_io_env_set(io, (fio_io_env_set_args_s){__VA_ARGS__})

/** Named arguments for the `fio_io_env_set` function. */
typedef struct {
  /** A numerical type filter. Defaults to 0. Negative values are reserved. */
  intptr_t type;
  /** The name for the link. The name and type uniquely identify the object. */
  fio_buf_info_s name;
  /** The object being linked to the connection. */
  void *udata;
  /** A callback that will be called once the connection is closed. */
  void (*on_close)(void *data);
  /** Set to true (1) if the name string's life lives as long as the `env` . */
  uint8_t const_name;
} fio_io_env_set_args_s;
```

Links an object to a connection's lifetime / environment.

The `on_close` callback will be called once the connection has died.

If the `io` is NULL, the value will be set for the global environment.

#### `fio_io_env_unset`

```c
int fio_io_env_unset(fio_io_s *io, fio_io_env_get_args_s);
#define fio_io_env_unset(io, ...)                                              \
  fio_io_env_unset(io, (fio_io_env_get_args_s){__VA_ARGS__})
```

Un-links an object from the connection's lifetime, so it's `on_close` callback will NOT be called.

Returns 0 on success and -1 if the object couldn't be found.

#### `fio_io_env_remove`

```c
int fio_io_env_remove(fio_io_s *io, fio_io_env_get_args_s);

#define fio_io_env_remove(io, ...)                                             \
  fio_io_env_remove(io, (fio_io_env_get_args_s){__VA_ARGS__})
```

Removes an object from the connection's lifetime / environment, calling it's `on_close` callback as if the connection was closed.

### TLS Context Helper Types

#### `fio_io_tls_new`

```c
fio_io_tls_s *fio_io_tls_new(void);
```

Performs a `new` operation, returning a new `fio_io_tls_s` context.

#### `fio_io_tls_from_url`

```c
fio_io_tls_s *fio_io_tls_from_url(fio_io_tls_s *target_or_null, fio_url_s url);
```

Takes a parsed URL and optional TLS target and returns a TLS if needed.

#### `fio_io_tls_dup`

```c
fio_io_tls_s *fio_io_tls_dup(fio_io_tls_s *);
```

Performs a `dup` operation, increasing the object's reference count.

#### `fio_io_tls_free`

```c
void fio_io_tls_free(fio_io_tls_s *);
```

Performs a `free` operation, reducing the reference count and freeing.

#### `fio_io_tls_cert_add`

```c
fio_io_tls_s *fio_io_tls_cert_add(fio_io_tls_s *,
                                  const char *server_name,
                                  const char *public_cert_file,
                                  const char *private_key_file,
                                  const char *pk_password);
```

Adds a certificate a new SSL/TLS context / settings object (SNI support).

```c
     fio_io_tls_cert_add(tls, "www.example.com",
                           "public_key.pem",
                           "private_key.pem", NULL );
```

**Note**: Except for the `tls` and `server_name` arguments, all arguments might
be `NULL`, which a context builder (`fio_io_functions_s`) should
treat as a request for a self-signed certificate. It may be silently ignored.

#### `fio_io_tls_alpn_add`

```c
fio_io_tls_s *fio_io_tls_alpn_add(fio_io_tls_s *tls,
                                  const char *protocol_name,
                                  void (*on_selected)(fio_io_s *));
```

Adds an ALPN protocol callback to the SSL/TLS context.

The first protocol added will act as the default protocol to be selected.

A `NULL` protocol name will be silently ignored.

A `NULL` callback (`on_selected`) will be silently replaced with a no-op.

#### `fio_io_tls_alpn_select`

```c
int fio_io_tls_alpn_select(fio_io_tls_s *tls,
                           const char *protocol_name,
                           size_t name_length,
                           fio_io_s *);
```

Calls the `on_selected` callback for the `fio_io_tls_s` object.


#### `fio_io_tls_trust_add`

```c
fio_io_tls_s *fio_io_tls_trust_add(fio_io_tls_s *,
                                   const char *public_cert_file);
```

Adds a certificate to the "trust" list, which automatically adds a peer verification requirement.

If `public_cert_file` is `NULL`, implementation is expected to add the system's default trust registry.

Note: when the `fio_io_tls_s` object is used for server connections, this should limit connections to clients that connect using a trusted certificate.

```c
fio_io_tls_trust_add(tls, "google-ca.pem" );
```


#### `fio_io_tls_cert_count`

```c
uintptr_t fio_io_tls_cert_count(fio_io_tls_s *tls);
```

Returns the number of `fio_io_tls_cert_add` instructions.

This could be used when deciding if to add a NULL instruction (self-signed).

If `fio_io_tls_cert_add` was never called, zero (0) is returned.

#### `fio_io_tls_alpn_count`

```c
uintptr_t fio_io_tls_alpn_count(fio_io_tls_s *tls);
```

Returns the number of registered ALPN protocol names.

This could be used when deciding if protocol selection should be delegated to the ALPN mechanism, or whether a protocol should be immediately assigned.

If no ALPN protocols are registered, zero (0) is returned.

#### `fio_io_tls_trust_count`

```c
uintptr_t fio_io_tls_trust_count(fio_io_tls_s *tls);
```

Returns the number of `fio_io_tls_trust_add` instructions.

This could be used when deciding if to disable peer verification or not.

If `fio_io_tls_trust_add` was never called, zero (0) is returned.

#### `fio_io_tls_each`

```c
int fio_io_tls_each(fio_io_tls_each_s);

#define fio_io_tls_each(tls_, ...)                                             \
  fio_io_tls_each(((fio_io_tls_each_s){.tls = tls_, __VA_ARGS__}))

/** Named arguments (and info) for `fio_io_tls_each`. */
typedef struct fio_io_tls_each_s {
  fio_io_tls_s *tls;
  void *udata;
  void *udata2;
  int (*each_cert)(struct fio_io_tls_each_s *,
                   const char *server_name,
                   const char *public_cert_file,
                   const char *private_key_file,
                   const char *pk_password);
  int (*each_alpn)(struct fio_io_tls_each_s *,
                   const char *protocol_name,
                   void (*on_selected)(fio_io_s *));
  int (*each_trust)(struct fio_io_tls_each_s *, const char *public_cert_file);
} fio_io_tls_each_s;
```

Calls callbacks for certificate, trust certificate and ALPN added.

#### `fio_io_tls_default_functions`

```c
fio_io_functions_s fio_io_tls_default_functions(fio_io_functions_s *);
```

If `NULL` returns current default, otherwise sets it.

### IO Async Queue - Worker Threads for non-IO tasks

#### `FIO_IO_ASYN_INIT`

```c
#define FIO_IO_ASYN_INIT ((fio_io_async_s){0})
```

Initializes an IO Async Queue (multi-threaded task queue).

The queue automatically spawns threads and shuts down as the IO reactor starts or stops.

It is recommended that the `fio_io_async_s` be allocated as a static variable, as its memory must remain valid throughout the lifetime of the IO reactor's app.

#### `fio_io_async_queue`

```c
fio_queue_s *fio_io_async_queue(fio_io_async_s *q) { return q->q; }
```

Returns the current task queue associated with the IO Async Queue.


#### `fio_io_async_attach`

```c
void fio_io_async_attach(fio_io_async_s *q, uint32_t threads);
```

Attaches an IO Async Queue for use in multi-threaded (non IO) tasks.

This function can be called multiple times for the same (or other) queue, as long as the async queue (`fio_io_async_s`) was previously initialized using `FIO_IO_ASYN_INIT` or zeroed out. i.e.:

```c
static fio_io_async_s SLOW_HTTP_TASKS = FIO_IO_ASYN_INIT;
fio_io_async_attach(&SLOW_HTTP_TASKS, 32);
```

#### `fio_io_async`

```c
#define fio_io_async(q_, ...) fio_queue_push((q_)->q, __VA_ARGS__)
```

Pushes a task to an IO Async Queue (macro helper).


## Pub/Sub 

By defining `FIO_PUBSUB`, a Publisher / Subscriber extension can be added to the `FIO_SERVER`, resulting in powerful IPC and real-time data updates.

The [pub/sub paradigm](https://en.wikipedia.org/wiki/Publish–subscribe_pattern) allows for any number of real-time applications, including message-bus backends, chat applications (private / group chats), broadcasting, games, etc'.

### Paradigm

Publishers publish messages to delivery Channels, without any information about the potential recipients (if any).

Subscribers "listen" to messages from specific delivery Channels without any information about the publishers (if any).

Messages are broadcasted through delivery Channels to the different Subscribers.

Delivery Channels in facil.io are a combination of a named channel and a 16 bit numerical filter, allowing for 32,768 namespaces of named channels (negative numbers are reserved).

### Limitations

The internal Pub/Sub Letter Exchange Protocol imposes the following limitations on message exchange:

* Distribution Channel Names are limited to 2^16 bytes (65,536 bytes).

* Message payload is limited to 2^24 bytes (16,777,216 bytes == about 16Mb).

* Empty messages (no numerical filters, no channel, no message payload, no flags) are ignored.

* Subscriptions match delivery matches by both channel name (or pattern) and the numerical filter.

### Subscriptions - Receiving Messages

#### `fio_subscribe`

```c
void fio_subscribe(subscribe_args_s args);
#define fio_subscribe(...) fio_subscribe((subscribe_args_s){__VA_ARGS__})
```

Subscribes to a channel / filter pair.

The `on_unsubscribe` callback will be called on failure.

The `fio_subscribe` macro shadows the `fio_subscribe` function and allows the following named arguments to be set:

```c
typedef struct {
  /**
   * The subscription owner - if none, the subscription is owned by the system.
   *
   * Note:
   *
   * Both the system and the `io` objects each manage channel listing
   * which allows only a single subscription to the same channel.
   *
   * This means a single subscription per channel per IO and a single
   * subscription per channel for the global system unless managing the
   * subscription handle manually.
   */
  fio_s *io;
  /**
   * A named `channel` to which the subscriber subscribes.
   *
   * Subscriptions require a match by both channel name and filter.
   */
  fio_buf_info_s channel;
  /**
   * The callback to be called for each message forwarded to the subscription.
   */
  void (*on_message)(fio_msg_s *msg);
  /** An optional callback for when a subscription is canceled. */
  void (*on_unsubscribe)(void *udata);
  /** The opaque udata value is ignored and made available to the callbacks. */
  void *udata;
  /** The queue to which the callbacks should be routed. May be NULL. */
  fio_queue_s *queue;
  /**
   * OPTIONAL: subscription handle return value - should be NULL when using
   * automatic memory management with the IO or global environment.
   *
   * When set, the `io` pointer will be ignored and the subscription object
   * handle will be written to the `subscription_handle_ptr` which MUST be
   * used when unsubscribing.
   *
   * NOTE: this could cause subscriptions and memory leaks unless properly
   * handled.
   */
  uintptr_t *subscription_handle_ptr;
  /**
   * A numerical namespace `filter` subscribers need to match.
   *
   * Negative values are reserved for facil.io framework extensions.
   *
   * Filer channels are bound to the processes and workers, they are NOT
   * forwarded to engines and can be used for inter process communication (IPC).
   */
  int16_t filter;
  /** If set, pattern matching will be used (name is a pattern). */
  uint8_t is_pattern;
} subscribe_args_s;
```

The `fio_msg_s` struct in the `on_message` callback contains the following information:

```c
typedef struct fio_msg_s {
  /** A connection (if any) to which the subscription belongs. */
  fio_s *io;
  /**
   * A channel name, allowing for pub/sub patterns.
   *
   * NOTE: the channel and message strings should be considered immutable.
   */
  fio_str_info_s channel;
  /**
   * The actual message.
   *
   * NOTE: the channel and message strings should be considered immutable.
   **/
  fio_str_info_s message;
  /** The `udata` argument associated with the subscription. */
  void *udata;
  /** A unique message type. Negative values are reserved, 0 == pub/sub. */
  int16_t filter;
  /** flag indicating if the message is JSON data or binary/text. */
  uint8_t is_json;
} fio_msg_s;
```

#### `fio_unsubscribe`

```c
int fio_unsubscribe(subscribe_args_s args);
#define fio_unsubscribe(...) fio_unsubscribe((subscribe_args_s){__VA_ARGS__})
```

Cancels an existing subscriptions.

Accepts the same arguments as [`fio_subscribe`](fio_subscribe), except the `udata`, and callback details are ignored (no need to provide `udata` or callback details).

If a `subscription_handle_ptr` was provided it should contain the value of the subscription handle returned.

Returns -1 if the subscription could not be found. Otherwise returns 0.

The `fio_unsubscribe` macro shadows the `fio_unsubscribe` function and allows the same named arguments as the [`fio_subscribe`](fio_subscribe) function.

#### `fio_message_defer`

```c
void fio_message_defer(fio_msg_s *msg);
```

Defers the current callback, so it will be called again for the same message.

After calling this function, the `msg` object must NOT be accessed again.

#### `FIO_PUBSUB_PATTERN_MATCH`

```c
extern uint8_t (*FIO_PUBSUB_PATTERN_MATCH)(fio_str_info_s pattern,
                                           fio_str_info_s channel);
```

A global variable controlling the pattern matching callback used for pattern matching.

The callback set **must** return 1 on a match or 0 if the string does not match the pattern.

By default, the value is set to `fio_glob_match` (see facil.io's C STL).

### Publishing to Subscribers

#### `fio_publish`

```c
void fio_publish(fio_publish_args_s args);
#define fio_publish(...) fio_publish((fio_publish_args_s){__VA_ARGS__})
```

Publishes a message to the relevant subscribers (if any).

By default the message is sent using the `FIO_PUBSUB_DEFAULT` engine (set by default to `FIO_PUBSUB_LOCAL` which publishes to all processes, including the calling process).

If publishing to a channel with a non-zero `filter`, the pub/sub will default to `FIO_PUBSUB_LOCAL` and external engines will be ignored.

To limit the message only to other processes (exclude the calling process), use the `FIO_PUBSUB_SIBLINGS` engine.

To limit the message only to the calling process, use the `FIO_PUBSUB_PROCESS` engine.

To limit the message only to the root process, use the `FIO_PUBSUB_ROOT` engine.

The `fio_publish` macro shadows the `fio_publish` function and allows the following named arguments to be set:

```c
typedef struct fio_publish_args_s {
  /** The pub/sub engine that should be used to forward this message. */
  fio_pubsub_engine_s const *engine;
  /** If `from` is specified, it will be skipped (won't receive message)
   *  UNLESS a non-native `engine` is specified. */
  fio_s *from;
  /** The target named channel. Only published when filter == 0. */
  fio_buf_info_s channel;
  /** The message body / content. */
  fio_buf_info_s message;
  /** A numeral / internal channel. Negative values are reserved. */
  int16_t filter;
  /** A flag indicating if the message is JSON data or not. */
  uint8_t is_json;
} fio_publish_args_s;
```

### Pub/Sub Engines

The pub/sub system allows the delivery of messages through either internal or external services called "engines".

The default pub/sub engine can be set by setting the global `FIO_PUBSUB_DEFAULT` variable which is set to `FIO_PUBSUB_LOCAL` by default.

External engines are funneled to the root / master process before their `publish` function is called, which means that even if `from` is specified, it will be ignored for any external engine.

#### `FIO_PUBSUB_ROOT`

```c
extern const fio_pubsub_engine_s *const FIO_PUBSUB_ROOT;
```

Used to publish the message exclusively to the root / master process.

#### `FIO_PUBSUB_PROCESS`

```c
extern const fio_pubsub_engine_s *const FIO_PUBSUB_PROCESS;
```

Used to publish the message only within the current process.

#### `FIO_PUBSUB_SIBLINGS`

```c
extern const fio_pubsub_engine_s *const FIO_PUBSUB_SIBLINGS;
```

Used to publish the message except within the current process.

#### `FIO_PUBSUB_LOCAL`

```c
extern const fio_pubsub_engine_s *const FIO_PUBSUB_LOCAL;
```

Used to publish the message for this process, its siblings and root.

#### `fio_pubsub_engine_s`

```c
struct fio_pubsub_engine_s {
  /** Called after the engine was detached, may be used for cleanup. */
  void (*detached)(const fio_pubsub_engine_s *eng);
  /** Subscribes to a channel. Called ONLY in the Root (master) process. */
  void (*subscribe)(const fio_pubsub_engine_s *eng, fio_str_info_s channel);
  /** Unsubscribes to a channel. Called ONLY in the Root (master) process. */
  void (*unsubscribe)(const fio_pubsub_engine_s *eng, fio_str_info_s channel);
  /** Subscribes to a pattern. Called ONLY in the Root (master) process. */
  void (*psubscribe)(const fio_pubsub_engine_s *eng, fio_str_info_s channel);
  /** Unsubscribe to a pattern. Called ONLY in the Root (master) process. */
  void (*punsubscribe)(const fio_pubsub_engine_s *eng, fio_str_info_s channel);
  /** Publishes a message through the engine. Called by any worker / thread. */
  void (*publish)(const fio_pubsub_engine_s *eng,
                  fio_str_info_s channel,
                  fio_str_info_s msg,
                  uint8_t is_json);
};
```

This is the (internal) structure of a facil.io pub/sub engine.

Only messages and unfiltered subscriptions (where filter == 0) will be forwarded to these "engines".

Engines MUST provide the listed function pointers and should be attached using the `fio_pubsub_attach` function.

Engines should disconnect / detach, before being destroyed, by using the `fio_pubsub_detach` function.

When an engine received a message to publish, it should call the `pubsub_publish` function with the built-in engine to which the message is forwarded.
i.e.:

```c
pubsub_publish(
    .engine = FIO_PUBSUB_LOCAL,
    .channel = channel_name,
    .message = msg_body );
```

Since only the master process guarantees to be subscribed to all the channels in the cluster, only the master process calls the `(un)(p)subscribe` callbacks.

**Note**: The `(un)(p)subscribe` callbacks might be called by the main (master) thread, so they should never block except by scheduling an external task using `fio_srv_defer`.

#### `fio_pubsub_attach`

```c
void fio_pubsub_attach(fio_pubsub_engine_s *engine);
```

Attaches an engine, so it's callback can be called by facil.io.

The `(p)subscribe` callback will be called for every existing channel.

NOTE: the root (master) process will call `subscribe` for any channel in any process, while all the other processes will call `subscribe` only for their own channels. This allows engines to use the root (master) process as an exclusive subscription process.


#### `fio_pubsub_detach`

```c
void fio_pubsub_detach(fio_pubsub_engine_s *engine);
```

Detaches an engine, so it could be safely destroyed.

#### `fio_pubsub_resubscribe_all`

```c
void fio_pubsub_resubscribe_all(fio_pubsub_engine_s *eng);
```

Engines can ask facil.io to call the `(p)subscribe` callbacks for all active channels.

This allows engines that lost their connection to their Pub/Sub service to resubscribe to all the currently active channels with the new connection.

**CAUTION**: This is an evented task... try not to free the engine's memory while re-subscriptions are under way.

**NOTE**: the root (master) process will call `(p)subscribe` for any channel in any process, while all the other processes will call `subscribe` only for their own channels. This allows engines to use the root (master) process as an exclusive subscription process.

#### `fio_pubsub_is_attached`

```c
int fio_pubsub_is_attached(fio_pubsub_engine_s *engine);
```

Returns true (`1`) if the engine is attached to the system.

### User Defined Pub/Sub Message Metadata

#### `fio_msg_metadata_fn`

```c
typedef void *(*fio_msg_metadata_fn)(fio_str_info_s ch,
                                     fio_str_info_s msg,
                                     uint8_t is_json);
```

Pub/Sub Metadata callback type.

#### `fio_message_metadata_add`

```c
int fio_message_metadata_add(fio_msg_metadata_fn metadata_func, void (*cleanup)(void *));
```

It's possible to attach metadata to facil.io named messages (filter == 0) before they are published.

This allows, for example, messages to be encoded as network packets for outgoing protocols (i.e., encoding for WebSocket transmissions), improving performance in large network based broadcasting.

Up to `FIO_PUBSUB_METADATA_LIMIT` metadata callbacks can be attached.

The callback should return a `void *` pointer.

To remove a callback, call `fio_message_metadata_remove` with the returned value.

The cluster messaging system allows some messages to be flagged as JSON and this flag is available to the metadata callback.

Returns zero (0) on success or -1 on failure.

Multiple `fio_message_metadata_add` calls increase a reference count and should be matched by the same number of `fio_message_metadata_remove`.

#### `fio_message_metadata_remove`

```c
void fio_message_metadata_remove(fio_msg_metadata_fn metadata_func);
```


Removed the metadata callback.

Removal might be delayed if live metatdata exists.

#### `fio_message_metadata`

```c
void *fio_message_metadata(fio_msg_metadata_fn metadata_func);
```


Finds the message's metadata, returning the data or NULL.

Note: channels with non-zero filters don't have metadata attached.

### Pub/Sub Connectivity Helpers

#### `fio_pubsub_ipc_url_set`

```c
int fio_pubsub_ipc_url_set(char *str, size_t len);
```

Returns the current IPC socket address (cannot be changed after `fio_srv_start` was called).

Returns -1 on error (i.e., server is already running or length is too long).

#### `fio_pubsub_ipc_url`

```c
const char *fio_pubsub_ipc_url(void);
```

Returns the current IPC socket address (shouldn't be changed).

-------------------------------------------------------------------------------
## HTTP Server

### Listening for HTTP / WebSockets and EventSource connections

#### `fio_http_listen`

```c
void * fio_http_listen(const char *url, fio_http_settings_s settings);

#define fio_http_listen(url, ...)                                              \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})
```

Listens to HTTP / WebSockets / SSE connections on `url`.

The MACRO shadowing the function enables the used of named arguments for the `fio_http_settings_s`.

Returns a listener handle (same as `fio_srv_listen`). Listening can be stopped using `fio_srv_listen_stop`.

```c
typedef struct fio_http_settings_s {
  /** Called before body uploads, when a client sends an `Expect` header. */
  void (*pre_http_body)(fio_http_s *h);
  /** Callback for HTTP requests (server) or responses (client). */
  void (*on_http)(fio_http_s *h);
  /** Called when a request / response cycle is finished with no Upgrade. */
  void (*on_finish)(fio_http_s *h);

  /** Authenticate EventSource (SSE) requests, return non-zero to deny.*/
  int (*on_authenticate_sse)(fio_http_s *h);
  /** Authenticate WebSockets Upgrade requests, return non-zero to deny.*/
  int (*on_authenticate_websocket)(fio_http_s *h);

  /** Called once a WebSocket / SSE connection upgrade is complete. */
  void (*on_open)(fio_http_s *h);

  /** Called when a WebSocket message is received. */
  void (*on_message)(fio_http_s *h, fio_buf_info_s msg, uint8_t is_text);
  /** Called when an EventSource event is received. */
  void (*on_eventsource)(fio_http_s *h,
                         fio_buf_info_s id,
                         fio_buf_info_s event,
                         fio_buf_info_s data);
  /** Called when an EventSource reconnect event requests an ID. */
  void (*on_eventsource_reconnect)(fio_http_s *h, fio_buf_info_s id);

  /** Called for WebSocket / SSE connections when outgoing buffer is empty. */
  void (*on_ready)(fio_http_s *h);
  /** Called for open WebSocket / SSE connections during shutting down. */
  void (*on_shutdown)(fio_http_s *h);
  /** Called after a WebSocket / SSE connection is closed (for cleanup). */
  void (*on_close)(fio_http_s *h);

  /** (optional) the callback to be performed when the HTTP service closes. */
  void (*on_stop)(struct fio_http_settings_s *settings);

  /** Default opaque user data for HTTP handles (fio_http_s). */
  void *udata;

  /** Optional SSL/TLS support. */
  fio_io_functions_s *tls_io_func;
  /** Optional SSL/TLS support. */
  fio_io_tls_s *tls;
  /** Optional HTTP task queue (for multi-threading HTTP responses) */
  fio_io_async_s *queue;
  /**
   * A public folder for file transfers - allows to circumvent any application
   * layer logic and simply serve static files.
   *
   * Supports automatic `gz` pre-compressed alternatives.
   */
  fio_str_info_s public_folder;
  /**
   * The max-age value (in seconds) for caching static files send from
   * `public_folder`.
   *
   * Defaults to 0 (not sent).
   */
  size_t max_age;
  /**
   * The maximum total of bytes for the overall size of the request string and
   * headers, combined.
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_HEADER_SIZE bytes.
   */
  uint32_t max_header_size;
  /**
   * The maximum number of bytes allowed per header / request line.
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_LINE_LEN bytes.
   */
  uint32_t max_line_len;
  /**
   * The maximum size of an HTTP request's body (posting / downloading).
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_BODY_SIZE bytes.
   */
  size_t max_body_size;
  /**
   * The maximum WebSocket message size/buffer (in bytes) for Websocket
   * connections. Defaults to FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE bytes.
   */
  size_t ws_max_msg_size;
  /** reserved for future use. */
  intptr_t reserved1;
  /** reserved for future use. */
  intptr_t reserved2;
  /**
   * An HTTP/1.x connection timeout.
   *
   * Defaults to FIO_HTTP_DEFAULT_TIMEOUT seconds.
   *
   * Note: the connection might be closed (by other side) before timeout occurs.
   */
  uint8_t timeout;
  /**
   * Timeout for the WebSocket connections in seconds. Defaults to
   * FIO_HTTP_DEFAULT_TIMEOUT_LONG seconds.
   *
   * A ping will be sent whenever the timeout is reached.
   *
   * Connections are only closed when a ping cannot be sent (the network layer
   * fails). Pongs are ignored.
   */
  uint8_t ws_timeout;
  /**
   * Timeout for EventSource (SSE) connections in seconds. Defaults to
   * FIO_HTTP_DEFAULT_TIMEOUT_LONG seconds.
   *
   * A ping will be sent whenever the timeout is reached.
   *
   * Connections are only closed when a ping cannot be sent (the network layer
   * fails).
   */
  uint8_t sse_timeout;
  /** Timeout for client connections (only relevant in client mode). */
  uint8_t connect_timeout;
  /** Logging flag - set to TRUE to log HTTP requests. */
  uint8_t log;
} fio_http_settings_s;
```

#### `fio_http_route`

```c
int fio_http_route(fio_http_listener_s *listener,
                         const char *url,
                         fio_http_settings_s settings);
#define fio_http_route(listener, url, ...)                                     \
  fio_http_route(listener, url, (fio_http_settings_s){__VA_ARGS__})
```

Adds a route prefix to the HTTP handler.

The order in which `fio_http_route` are called is irrelevant (unless overwriting an existing route).

Matching is performed as a best-prefix match. i.e.:

- All paths match the prefix `"/*"` (the default prefix).

- Setting `"/user"` will match `"/user"` and all `"/user/*"` paths but not `"/user*"`

- Setting `"/user/new"` as well as `"/user"` (in whatever order) will route `"/user/new"` and `"/user/new/*"` to `"/user/new"`. Otherwise, the `"/user"` route will continue to behave the same.

**Note**: the `udata`, `on_finish`, `public_folder` and `log` properties are all inherited (if missing) from the default HTTP settings used to create the listener.

**Note**: TLS options are ignored.

#### `fio_http_route_settings`

```c
fio_http_settings_s *fio_http_route_settings(fio_http_listener_s *listener, const char *url);
```

Returns a link to the settings matching `url`, as set by `fio_http_route`.

#### `fio_http_listener_settings`

```c
fio_http_settings_s *fio_http_listener_settings(void *listener);
```

Returns the a pointer to the HTTP settings associated with the listener.

**Note**: changing the settings for the root path should be performed using `fio_http_route` and not by altering the settings directly.

#### `FIO_HTTP_AUTHENTICATE_ALLOW`

```c
int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h);
```

Allows all clients to connect to WebSockets / EventSource (SSE) connections (bypasses authentication), to be used with the `.on_authenticate_sse` and `.on_authenticate_websocket` settings options.

#### `fio_http_connect`

```c
fio_s *fio_http_connect(const char *url,
                              fio_http_s *h,
                              fio_http_settings_s settings);
/* Shadow the function for named arguments */
#define fio_http_connect(url, h, ...)                                          \
  fio_http_connect(url, h, (fio_http_settings_s){__VA_ARGS__})

```

Connects to HTTP / WebSockets / SSE connections on `url`.

Accepts named arguments for the `fio_http_settings_s` settings.

### Creating an HTTP Handle

These are Used internally by the `FIO_HTTP` module.

#### `fio_http_new`

```c
fio_http_s *fio_http_new(void);
```

Create a new `fio_http_s` handle.

#### `fio_http_new_copy_request`

```c
fio_http_s *fio_http_new_copy_request(fio_http_s *old);
```

Creates a copy of an existing handle, copying only its request data.

#### `fio_http_destroy`

```c
fio_http_s *fio_http_destroy(fio_http_s *h);
```

Destroyed the HTTP handle object, freeing all allocated resources.

#### `fio_http_start_time_set`

```c
void fio_http_start_time_set(fio_http_s *);
```

Collects an updated timestamp for logging purposes.

### HTTP Reference Counting

#### `fio_http_free`

```c
void fio_http_free(fio_http_s *);
```

Reduces an `fio_http_s` handle's reference count or frees it.

#### `fio_http_dup`

```c
fio_http_s *fio_http_dup(fio_http_s *);
```

Increases an `fio_http_s` handle's reference count.

### HTTP Request / Response Data

#### `fio_http_udata`

```c
void *fio_http_udata(fio_http_s *);
```

Gets the opaque user pointer associated with the HTTP handle.

#### `fio_http_udata_set`

```c
void *fio_http_udata_set(fio_http_s *, void *);
```

Sets the opaque user pointer associated with the HTTP handle.

#### `fio_http_udata2`

```c
void *fio_http_udata2(fio_http_s *);
```

Gets the second opaque user pointer associated with the HTTP handle.

#### `fio_http_udata2_set`

```c
void *fio_http_udata2_set(fio_http_s *, void *);
```

Sets a second opaque user pointer associated with the HTTP handle.

#### `fio_http_io`

```c
fio_s *fio_http_io(fio_http_s *);
```

Returns the IO object associated with the HTTP object (request only).


#### `fio_http_settings`

```c
fio_http_settings_s *fio_http_settings(fio_http_s *);
```

Returns the HTTP settings associated with the HTTP object, if any.

#### `fio_http_from`

```c
int fio_http_from(fio_str_info_s *dest, const fio_http_s *h);
```

Writes peer address to `dest` starting with the `forwarded` header, with a fallback to actual socket address and a final fallback to `"[unknown]"`.

If `unknown` is returned, the function returns -1. if `dest` capacity is too small, the number of bytes required will be returned.

If all goes well, this function returns 0.

**Note**: to check the actual socket peer address (not merely the address reported by the client), use [`fio_sock_peer_addr`](#fio_sock_peer_addr). If the request was routed using a proxy, this will provide the proxy's address.


### HTTP State

#### `fio_http_is_clean`

```c
int fio_http_is_clean(fio_http_s *);
```

Returns true if no HTTP headers / data was sent (a clean slate).

#### `fio_http_is_finished`

```c
int fio_http_is_finished(fio_http_s *);
```

Returns true if the HTTP handle's response was sent.

#### `fio_http_is_streaming`

```c
int fio_http_is_streaming(fio_http_s *);
```

Returns true if the HTTP handle's response is streaming.

#### `fio_http_is_upgraded`

```c
int fio_http_is_upgraded(fio_http_s *h);
```

Returns true if the HTTP connection was (or should have been) upgraded.

#### `fio_http_is_websocket`

```c
int fio_http_is_websocket(fio_http_s *);
```

Returns true if the HTTP handle refers to a WebSocket connection.

#### `fio_http_is_sse`

```c
int fio_http_is_sse(fio_http_s *);
```

Returns true if the HTTP handle refers to an EventSource connection.

### HTTP Request Data

#### `fio_http_status`

```c
size_t fio_http_status(fio_http_s *);
```

Gets the status associated with the HTTP handle (response).

#### `fio_http_status_set`

```c
size_t fio_http_status_set(fio_http_s *, size_t status);
```

Sets the status associated with the HTTP handle (response).

#### `fio_http_method`

```c
fio_str_info_s fio_http_method(fio_http_s *);
```

Gets the method information associated with the HTTP handle.

#### `fio_http_method_set`

```c
fio_str_info_s fio_http_method_set(fio_http_s *, fio_str_info_s);
```

Sets the method information associated with the HTTP handle.

#### `fio_http_path`

```c
fio_str_info_s fio_http_path(fio_http_s *);
```

Gets the path information associated with the HTTP handle.

#### `fio_http_path_set`

```c
fio_str_info_s fio_http_path_set(fio_http_s *, fio_str_info_s);
```

Sets the path information associated with the HTTP handle.

#### `fio_http_query`

```c
fio_str_info_s fio_http_query(fio_http_s *);
```

Gets the query information associated with the HTTP handle.

#### `fio_http_query_set`

```c
fio_str_info_s fio_http_query_set(fio_http_s *, fio_str_info_s);
```

Sets the query information associated with the HTTP handle.

#### `fio_http_version`

```c
fio_str_info_s fio_http_version(fio_http_s *);
```

Gets the version information associated with the HTTP handle.

#### `fio_http_version_set`

```c
fio_str_info_s fio_http_version_set(fio_http_s *, fio_str_info_s);
```

Sets the version information associated with the HTTP handle.

#### `fio_http_request_header`
```c
fio_str_info_s fio_http_request_header(fio_http_s *,
                                       fio_str_info_s name,
                                       size_t index);
```

Gets the header information associated with the HTTP handle.

Since more than a single value may be associated with a header name, the index may be used to collect subsequent values.

An empty value is returned if no header value is available (or index is exceeded).

#### `fio_http_request_header_count`

```c
size_t fio_http_request_header_count(fio_http_s *, fio_str_info_s name);
```

Returns the number of headers named `name` that were received.

If `name` buffer is `NULL`, returns the number of unique headers (not the number of unique values).


#### `fio_http_request_header_set`

```c
fio_str_info_s fio_http_request_header_set(fio_http_s *,
                                           fio_str_info_s name,
                                           fio_str_info_s value);
```

Sets the header information associated with the HTTP handle.

#### `fio_http_request_header_set_if_missing`

```c
fio_str_info_s fio_http_request_header_set_if_missing(fio_http_s *,
                                       fio_str_info_s name,
                                       fio_str_info_s value);
```

Sets the header information associated with the HTTP handle.

#### `fio_http_request_header_add`

```c
fio_str_info_s fio_http_request_header_add(fio_http_s *,
                                           fio_str_info_s name,
                                           fio_str_info_s value);
```

Adds to the header information associated with the HTTP handle.

#### `fio_http_request_header_each`

```c
size_t fio_http_request_header_each(fio_http_s *,
                                    int (*callback)(fio_http_s *,
                                                    fio_str_info_s name,
                                                    fio_str_info_s value,
                                                    void *udata),
                                    void *udata);
```

Iterates through all request headers (except cookies!).

A non-zero return will stop iteration.

Returns the number of iterations performed. If `callback` is `NULL`, returns the number of headers available (multi-value headers are counted as 1).

#### `fio_http_body_length`
```c
size_t fio_http_body_length(fio_http_s *);
```

Gets the body (payload) length associated with the HTTP handle.

#### `fio_http_body_pos`

```c
size_t fio_http_body_pos(fio_http_s *h);
```

Returns the body's reading position.

#### `fio_http_body_seek`
```c
size_t fio_http_body_seek(fio_http_s *, ssize_t pos);
```

Adjusts the body's reading position. Negative values start at the end.

If `pos == SSIZE_MAX`, returns `fio_http_body_pos`.

#### `fio_http_body_read`
```c
fio_str_info_s fio_http_body_read(fio_http_s *, size_t length);
```

Reads up to `length` of data from the body, returns nothing on EOF.

#### `fio_http_body_read_until`

```c
fio_str_info_s fio_http_body_read_until(fio_http_s *,
                                        char token,
                                        size_t limit);
```

Reads from the body until finding `token`, reaching `limit` or EOF.

Note: `limit` is ignored if zero or if the remaining data is lower than limit.

#### `fio_http_body_expect`
```c
void fio_http_body_expect(fio_http_s *, size_t expected_length);
```

Allocates a body (payload) of (at least) the `expected_length`.


#### `fio_http_body_write`
```c
void fio_http_body_write(fio_http_s *, const void *data, size_t len);
```

Writes `data` to the body (payload) associated with the HTTP handle.

#### HTTP Cookies


#### `fio_http_cookie_set` - `same_site`

```c
typedef enum fio_http_cookie_same_site_e {
  /** allow the browser to dictate this property */
  FIO_HTTP_COOKIE_SAME_SITE_BROWSER_DEFAULT = 0,
  /** The browser sends the cookie with cross-site and same-site requests. */
  FIO_HTTP_COOKIE_SAME_SITE_NONE,
  /**
   * The cookie is withheld on cross-site sub-requests.
   *
   * The cookie is sent when a user navigates to the URL from an external
   * site.
   */
  FIO_HTTP_COOKIE_SAME_SITE_LAX,
  /** The browser sends the cookie only for same-site requests. */
  FIO_HTTP_COOKIE_SAME_SITE_STRICT,
} fio_http_cookie_same_site_e;
```

Possible values for the `same_site` property in the cookie settings.

See: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie


#### `fio_http_cookie_set`
```c
int fio_http_cookie_set(fio_http_s *h, fio_http_cookie_args_s);
/** Named arguments helper. See fio_http_cookie_args_s for details. */
#define fio_http_cookie_set(http___handle, ...)                                \
  fio_http_cookie_set((http___handle), (fio_http_cookie_args_s){__VA_ARGS__})

```

Sets a response cookie.

Returns -1 on error and 0 on success.

Note: Long cookie names and long cookie values will be considered a security violation and an error will be returned. Many browsers and proxies impose limits on headers and cookies, cookies often limited to 4Kb in total for both name and value.

```c
typedef struct fio_http_cookie_args_s {
  /** The cookie's name. */
  fio_str_info_s name;
  /** The cookie's value (leave blank to delete cookie). */
  fio_str_info_s value;
  /** The cookie's domain (optional). */
  fio_str_info_s domain;
  /** The cookie's path (optional). */
  fio_str_info_s path;
  /** Max Age (how long should the cookie persist), in seconds (0 == session).*/
  int max_age;
  /** SameSite value. */
  fio_http_cookie_same_site_e same_site;
  /** Limit cookie to secure connections.*/
  unsigned secure : 1;
  /** Limit cookie to HTTP (intended to prevent JavaScript access/hijacking).*/
  unsigned http_only : 1;
  /**
   * Set the Partitioned (third party) cookie flag:
   * https://developer.mozilla.org/en-US/docs/Web/Privacy/Partitioned_cookies
   */
  unsigned partitioned : 1;
} fio_http_cookie_args_s;
```

#### `fio_http_cookie`

```c
fio_str_info_s fio_http_cookie(fio_http_s *,
                               const char *name,
                               size_t name_len);
```

Returns a cookie value (either received of newly set), if any.

#### `fio_http_cookie_each`

```c
size_t fio_http_cookie_each(fio_http_s *,
                            int (*callback)(fio_http_s *,
                                            fio_str_info_s name,
                                            fio_str_info_s value,
                                            void *udata),
                            void *udata);
```

Iterates through all cookies. A non-zero return will stop iteration.

#### fio_http_set_cookie_each

```c
size_t fio_http_set_cookie_each(fio_http_s *h,
                         int (*callback)(fio_http_s *,
                                         fio_str_info_s set_cookie_header,
                                         fio_str_info_s value,
                                         void *udata),
                         void *udata);
```

Iterates through all response set cookies.

A non-zero return value from the callback will stop iteration.

### HTTP Response Data

#### `fio_http_response_header`

```c
fio_str_info_s fio_http_response_header(fio_http_s *,
                                        fio_str_info_s name,
                                        size_t index);
```

Gets the header information associated with the HTTP handle.

Since more than a single value may be associated with a header name, the index may be used to collect subsequent values.

An empty value is returned if no header value is available (or index is exceeded).

If the response headers were already sent, the returned value is always empty.


#### `fio_http_response_header_count`

```c
size_t fio_http_response_header_count(fio_http_s *, fio_str_info_s name);
```

Returns the number of headers named `name` in the response.

If `name` buffer is `NULL`, returns the number of unique headers (not the number of unique values).

#### `fio_http_response_header_set`

```c
fio_str_info_s fio_http_response_header_set(fio_http_s *,
                                            fio_str_info_s name,
                                            fio_str_info_s value);
```

Sets the header information associated with the HTTP handle.

If the response headers were already sent, the returned value is always empty.

#### `fio_http_response_header_set_if_missing`

```c
fio_str_info_s fio_http_response_header_set_if_missing(fio_http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value);
```

Sets the header information associated with the HTTP handle.

If the response headers were already sent, the returned value is always empty.

#### `fio_http_response_header_add`

```c
fio_str_info_s fio_http_response_header_add(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);
```

Adds to the header information associated with the HTTP handle.

If the response headers were already sent, the returned value is always empty.

#### `fio_http_response_header_each`

```c
size_t fio_http_response_header_each(fio_http_s *,
                                     int (*callback)(fio_http_s *,
                                                     fio_str_info_s name,
                                                     fio_str_info_s value,
                                                     void *udata),
                                     void *udata);
```

Iterates through all response headers (except cookies!).

A non-zero return will stop iteration.

### HTTP Writing to the Connection

#### `fio_http_write`

```c
void fio_http_write(fio_http_s *, fio_http_write_args_s args);

/** Named arguments helper. See fio_http_write and fio_http_write_args_s. */
#define fio_http_write(http_handle, ...)                                       \
  fio_http_write(http_handle, (fio_http_write_args_s){__VA_ARGS__})
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)
```

Writes `data` to the response body associated with the HTTP handle after sending all headers (no further headers may be sent).

Accepts the followung (possibly named) arguments:

```c
/** Arguments for the fio_http_write function. */
typedef struct fio_http_write_args_s {
  /** The data to be written. */
  const void *buf;
  /** The length of the data to be written. */
  size_t len;
  /** The offset at which writing should begin. */
  size_t offset;
  /** If streaming a file, set this value. The file is always closed. */
  int fd;
  /** If the data is a buffer, this callback may be set to free it once sent. */
  void (*dealloc)(void *);
  /** If the data is a buffer it will be copied. */
  int copy;
  /**
   * If `finish` is set, this data marks the end of the response.
   *
   * Otherwise the response will stream the data.
   */
  int finish;
} fio_http_write_args_s;
```

**Note**: this function will route to [`fio_http_websocket_write`](#fio_http_websocket_write) and [`fio_http_sse_write`](#fio_http_sse_write) if appropriate. However, for WebSocket connections it may be more prudent to decide if a binary or UTF-8 write operation should be performed by calling [`fio_http_websocket_write`](#fio_http_websocket_write) directly.
  
#### `fio_http_finish`

```c
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)
```

Calls [`fio_http_write`](#fio_http_write) without any data to be sent.


#### `fio_http_send_error_response`

```c
void fio_http_send_error_response(fio_http_s *h, size_t status);
```

Sends the requested error message and finishes the response.

#### `fio_http_etag_is_match`

```c
int fio_http_etag_is_match(fio_http_s *h);
```

Returns true (1) if the ETag response matches an if-none-match request.

#### `fio_http_static_file_response`

```c
int fio_http_static_file_response(fio_http_s *h,
                                  fio_str_info_s root_folder,
                                  fio_str_info_s file_name,
                                  size_t max_age);
```

Attempts to send a static file from the `root` folder.

On success the response is complete and 0 is returned. Otherwise returns -1.

#### `fio_http_status2str`

```c
fio_str_info_s fio_http_status2str(size_t status);
```

Returns a human readable string related to the HTTP status number.

#### `fio_http_write_log`

```c
void fio_http_write_log(fio_http_s *h);
```

Logs an HTTP (response) to STDOUT using common log format:

```txt
[PID] ADDR - - [DATE/TIME] REQ RES_CODE BYTES_SENT TIME_SPENT_IN_APP <(wait PROXY_DELAY)>
```

See also the `FIO_HTTP_LOG_X_REQUEST_START` and `FIO_HTTP_EXACT_LOGGING` compilation flags.

### HTTP WebSocket / SSE Helpers

#### `fio_http_websocket_requested`

```c
int fio_http_websocket_requested(fio_http_s *);
```

Returns non-zero if request headers ask for a WebSockets Upgrade.

#### `fio_http_upgrade_websocket`

```c
void fio_http_upgrade_websocket(fio_http_s *);
```

Sets response data to agree to a WebSockets Upgrade.

#### `fio_http_websocket_set_request`

```c
void fio_http_websocket_set_request(fio_http_s *);
```

Sets request data to request a WebSockets Upgrade.

#### `fio_http_sse_requested`

```c
int fio_http_sse_requested(fio_http_s *);
```

Returns non-zero if request headers ask for an EventSource (SSE) Upgrade.

#### `fio_http_upgrade_sse`

```c
void fio_http_upgrade_sse(fio_http_s *);
```

Sets response data to agree to an EventSource (SSE) Upgrade.

#### `fio_http_sse_set_request`

```c
void fio_http_sse_set_request(fio_http_s *);
```

Sets request data to request an EventSource (SSE) Upgrade.

#### `fio_http_websocket_write`

```c
int fio_http_websocket_write(fio_http_s *h, const void *buf, size_t len, uint8_t is_text);
```

Writes a WebSocket message. Fails if connection wasn't upgraded yet.

**Note**: calls to the HTTP handle function `fio_http_write` may route to this function after the library performs a best guess attempt at the correct `is_text`.

#### `fio_http_on_message_set`

```c
int fio_http_on_message_set(fio_http_s *h,
                            void (*on_message)(fio_http_s *,
                                               fio_buf_info_s,
                                               uint8_t));
```

Sets a specific `on_message` callback for the WebSocket connection.

Returns `-1` on error (i.e., upgrade still being negotiated).

### EventSource (SSE) Helpers - HTTP Upgraded Connections

#### `fio_http_sse_write`

```c
int fio_http_sse_write(fio_http_s *h, fio_http_sse_write_args_s args);
#define fio_http_sse_write(h, ...)                                             \
  fio_http_sse_write((h), ((fio_http_sse_write_args_s){__VA_ARGS__}))
```

Writes an SSE message (UTF-8). Fails if connection wasn't upgraded yet.

The MACRO shadowing the function enables the used of the named arguments listed in `fio_http_sse_write_args_s`:

```c
/** Named arguments for fio_http_sse_write. */
typedef struct {
  /** The message's `id` data (if any). */
  fio_buf_info_s id;
  /** The message's `event` data (if any). */
  fio_buf_info_s event;
  /** The message's `data` data (if any). */
  fio_buf_info_s data;
} fio_http_sse_write_args_s;
```

**Note**: calls to the HTTP handle function `fio_http_write` may route to this function, in which case both `event` and `id` are omitted.

#### `fio_http_close`

```c
void fio_http_close(fio_http_s *h);
```

Closes a persistent HTTP connection (i.e., if upgraded). 



### HTTP Pub/Sub

#### `fio_http_subscribe`

```c
int fio_http_subscribe(fio_http_s *h, fio_subscribe_args_s args);

#define fio_http_subscribe(h, ...) fio_http_subscribe((h), ((fio_subscribe_args_s){__VA_ARGS__}))
```

Subscribes the HTTP handle (WebSocket / SSE) to events. Requires an Upgraded connection.

Automatically sets the correct `io` object and the default callback if none was provided (either `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT` or `FIO_HTTP_SSE_SUBSCRIBE_DIRECT`).

Using a `NULL` HTTP handle (`fio_http_s *`) is the same as calling `fio_subscribe` without an `io` object.

Returns `-1` on error (i.e., upgrade still being negotiated).

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT`

```c
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_msg_s *msg);
```

Optional WebSocket subscription callback that directly writes the content of the published message to the WebSocket connection.

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT`

```c
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_msg_s *msg);
```

Optional WebSocket subscription callback that directly writes the content of the published message to the WebSocket connection - this callback assumes that all messages are UTF-8 valid.

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY`

```c
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY(fio_msg_s *msg);
```

Optional WebSocket subscription callback that directly writes the content of the published message to the WebSocket connection - this callback assumes that all messages are binary (non-UTF-8).

#### `FIO_HTTP_SSE_SUBSCRIBE_DIRECT`

```c
void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_msg_s *msg);
```

An optional EventSource subscription callback - messages MUST be UTF-8.

This callback directly calls `fio_http_sse_write`, placing the channel name in the `.event` argument and the published message in the `.data` argument.

### HTTP Header Parsing Helpers

#### `fio_http_response_header_parse`

```c
int fio_http_response_header_parse(fio_http_s *h,
                                   fio_str_info_s *buf_parsed,
                                   fio_str_info_s header_name);
```

Copies all header data, from possibly an array of identical response headers, resulting in a parsed format outputted to `buf_parsed`.

Returns 0 on success or -1 on error (i.e., `buf_parsed.capa` wasn't enough for the parsed output).

Note that the parsed output isn't readable as a string, but is designed to work with the `FIO_HTTP_PARSED_HEADER_EACH` and `FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` property.

See also `fio_http_response_header_parse`.

#### `fio_http_request_header_parse`

```c
int fio_http_request_header_parse(fio_http_s *h,
                                  fio_str_info_s *buf_parsed,
                                  fio_str_info_s header_name);
```
Copies all header data, from possibly an array of identical response headers,
resulting in a parsed format outputted to `buf_parsed`.

Returns 0 on success or -1 on error (i.e., `buf_parsed.capa` wasn't enough
for the parsed output).

Note that the parsed output isn't readable as a string, but is designed to
work with the `FIO_HTTP_PARSED_HEADER_EACH` and
`FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` property.

i.e.:

```c
 FIO_STR_INFO_TMP_VAR(buf, 1023); // tmp buffer for the parsed output
 fio_http_s *h = fio_http_new();  // using a mock HTTP handle
 fio_http_request_header_add(
     h,
     FIO_STR_INFO2("accept", 6),
     FIO_STR_INFO1("text/html, application/json;q=0.9; d=500, image/png"));
 fio_http_request_header_add(h,
                             FIO_STR_INFO2("accept", 6),
                             FIO_STR_INFO1("text/yaml"));
 FIO_ASSERT(  // in production do NOT assert, but route to error instead!
     !fio_http_request_header_parse(h, &buf, FIO_STR_INFO2("accept", 6)),
     "parse returned error!");
 FIO_HTTP_PARSED_HEADER_EACH(buf, value) {
   printf("* processing value (%zu bytes): %s\n", value.len, value.buf);
   FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(value, prop) {
     printf("* for value %s: (%zu,%zu bytes) %s = %s\n",
            value.buf,
            prop.name.len,
            prop.value.len,
            prop.name.buf,
            prop.value.buf);
   }
 }
```

#### `FIO_HTTP_HEADER_EACH_VALUE`
```c
#define FIO_HTTP_HEADER_EACH_VALUE(/* fio_http_s */ http_handle,               \
                                   /* int / bool */ is_request,                \
                                   /* fio_str_info_s */ header_name,           \
                                   /* chosen var named */ value)               \
  for (char fio___buf__##value##__[2048], /* allocate buffer on stack */       \
           *fio___buf__##value##_ptr = NULL;                                   \
       !fio___buf__##value##_ptr;                                              \
       fio___buf__##value##_ptr = fio___buf__##value##__)                      \
    for (fio_str_info_s fio___buf__##value##__str = /* declare buffer var */   \
         FIO_STR_INFO3(fio___buf__##value##__, 0, 2048);                       \
         fio___buf__##value##__str.buf == fio___buf__##value##__;              \
         fio___buf__##value##__str.buf = fio___buf__##value##__ + 1)           \
      if (!((is_request ? fio_http_request_header_parse                        \
                        : fio_http_response_header_parse)(                     \
              http_handle, /* parse headers */                                 \
              &fio___buf__##value##__str,                                      \
              header_name)))                                                   \
  FIO_HTTP_PARSED_HEADER_EACH(fio___buf__##value##__str, value) /* loop */

/** Iterated through the properties associated with a parsed header values. */
#define FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(/* fio_str_info_s   */ value,      \
                                            /* chosen var named */ property)

/** Used internally to iterate over a parsed header buffer. */
#define FIO_HTTP_PARSED_HEADER_EACH(/* fio_str_info_s   */ buf_parsed,         \
                                    /* chosen var named */ value)
```

Parses header for multiple values and properties and iterates over all values.

This MACRO will allocate 2048 bytes on the stack for parsing the header values and properties, if more space is necessary dig deeper.

Use `FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` to iterate over a value's properties.

### HTTP Mime-Type Registry

MIME File Type Helpers - NOT thread safe!

#### `fio_http_mimetype_register`

```c
int fio_http_mimetype_register(char *file_ext,
                               size_t file_ext_len,
                               fio_str_info_s mime_type);
```

Registers a Mime-Type to be associated with the file extension.

#### `fio_http_mimetype`

```c
fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len);
```

Finds the Mime-Type associated with the file extension (if registered).

### Compilation Flags and Default HTTP Handle Behavior

#### `FIO_HTTP_EXACT_LOGGING`

```c
#ifndef FIO_HTTP_EXACT_LOGGING
#define FIO_HTTP_EXACT_LOGGING 0
#endif
```

By default, facil.io logs the HTTP request cycle using a fuzzy starting and ending point for the time stamp.

The fuzzy timestamp includes delays that aren't related to the HTTP request and may ignore time passed due to timestamp caching.

On the other hand, `FIO_HTTP_EXACT_LOGGING` collects exact time stamps to measure the time it took to process the HTTP request (excluding time spent reading / writing the data from the network).

Due to the preference to err on the side of higher performance, fuzzy time-stamping is the default.


#### `FIO_HTTP_LOG_X_REQUEST_START`

```c
#ifndef FIO_HTTP_LOG_X_REQUEST_START
#define FIO_HTTP_LOG_X_REQUEST_START 1
#endif
```

If set, logs will react to an `X-Request-Start` header that provides time in milliseconds.

An additional `(wait XXms)` data point will be provided in the logs to inform of the delay between the proxy server's `X-Request-Start` start time and the application's start time.

#### `FIO_HTTP_BODY_RAM_LIMIT`

```c
#ifndef FIO_HTTP_BODY_RAM_LIMIT
#define FIO_HTTP_BODY_RAM_LIMIT (1 << 17)
#endif
```

The HTTP handle automatically switches between RAM storage and file storage once the HTTP body (payload) reaches a certain size. This control this point of transition

#### `FIO_HTTP_CACHE_LIMIT`

```c
#ifndef FIO_HTTP_CACHE_LIMIT
#define FIO_HTTP_CACHE_LIMIT 0 /* ((1UL << 6) + (1UL << 5)) */
#endif
```

Each of the HTTP String Caches will be limited to this String count.

#### `FIO_HTTP_CACHE_STR_MAX_LEN`

```c
#ifndef FIO_HTTP_CACHE_STR_MAX_LEN
#define FIO_HTTP_CACHE_STR_MAX_LEN (1 << 12)
#endif
```

The HTTP handle will avoid caching strings longer than this value.

#### `FIO_HTTP_CACHE_USES_MUTEX`

```c
#ifndef FIO_HTTP_CACHE_USES_MUTEX
#define FIO_HTTP_CACHE_USES_MUTEX 1
#endif
```

The HTTP cache will use a mutex to allow headers to be set concurrently.

#### `FIO_HTTP_CACHE_STATIC_HEADERS`

```c
#ifndef FIO_HTTP_CACHE_STATIC_HEADERS
#define FIO_HTTP_CACHE_STATIC_HEADERS 1
#endif
```

Adds a static cache for common HTTP header names.

#### `FIO_HTTP_DEFAULT_INDEX_FILENAME`

```c
#ifndef FIO_HTTP_DEFAULT_INDEX_FILENAME
#define FIO_HTTP_DEFAULT_INDEX_FILENAME "index"
#endif
```

The default file name when a static file response points to a folder.

#### `FIO_HTTP_STATIC_FILE_COMPLETION`

```c
#ifndef FIO_HTTP_STATIC_FILE_COMPLETION
#define FIO_HTTP_STATIC_FILE_COMPLETION 1
#endif
```

Attempts to auto-complete static file paths with missing extensions.

### Compilation Flags and Default HTTP Connection Settings

#### `FIO_HTTP_DEFAULT_MAX_HEADER_SIZE`

```c
#ifndef FIO_HTTP_DEFAULT_MAX_HEADER_SIZE
#define FIO_HTTP_DEFAULT_MAX_HEADER_SIZE 32768 /* (1UL << 15) */
#endif
```
#### `FIO_HTTP_DEFAULT_MAX_LINE_LEN`

```c
#ifndef FIO_HTTP_DEFAULT_MAX_LINE_LEN
#define FIO_HTTP_DEFAULT_MAX_LINE_LEN 8192 /* (1UL << 13) */
#endif
```
#### `FIO_HTTP_DEFAULT_MAX_BODY_SIZE`

```c
#ifndef FIO_HTTP_DEFAULT_MAX_BODY_SIZE
#define FIO_HTTP_DEFAULT_MAX_BODY_SIZE 33554432 /* (1UL << 25) */
#endif
```
#### `FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE`

```c
#ifndef FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE
#define FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE 262144 /* (1UL << 18) */
#endif
```
#### `FIO_HTTP_DEFAULT_TIMEOUT`

```c
#ifndef FIO_HTTP_DEFAULT_TIMEOUT
#define FIO_HTTP_DEFAULT_TIMEOUT 50
#endif
```
#### `FIO_HTTP_DEFAULT_TIMEOUT_LONG`

```c
#ifndef FIO_HTTP_DEFAULT_TIMEOUT_LONG
#define FIO_HTTP_DEFAULT_TIMEOUT_LONG 50
#endif
```

#### `FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER`

```c
#ifndef FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER
#define FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER 0
#endif
```

Adds a "content-length" header to the HTTP handle (usually redundant).

#### `FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT`

```c
#ifndef FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT
#define FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT ((1UL << 16) - 10UL)
#endif
```

UTF-8 validity tests will be performed only for data shorter than this.

#### `FIO_WEBSOCKET_STATS`

```c
#ifndef FIO_WEBSOCKET_STATS
#define FIO_WEBSOCKET_STATS 0
#endif
```

If true, logs longest WebSocket ping-pong round-trips (using `FIO_LOG_INFO`).

-------------------------------------------------------------------------------
## Hash Function Testing

During development I tested the Hash functions using [the SMHasher testing suite (@rurban's fork)](https://github.com/rurban/smhasher). The testing suite is often growing with more tests, so I do not know what the future may bring... but attached are the test results for Risky Hash, Stable Hash (64 and 128 bit variation) as they were at the time of (re)development (March, 2023).
### Risky Hash SMHasher Results

The following results were achieved on my personal computer when testing the facil.io Risky Hash (`fio_risky_hash`).

```txt
-------------------------------------------------------------------------------
--- Testing Risky "facil.io Risky Hash" GOOD

[[[ Sanity Tests ]]]

Verification value 0x407D2C05 ....... PASS
Running sanity check 1     .......... PASS
Running AppendedZeroesTest .......... PASS

[[[ Speed Tests ]]]

Bulk speed test - 262144-byte keys
Alignment  7 - 17.861 bytes/cycle - 51100.77 MiB/sec @ 3 ghz
Alignment  6 - 17.833 bytes/cycle - 51019.28 MiB/sec @ 3 ghz
Alignment  5 - 17.832 bytes/cycle - 51018.87 MiB/sec @ 3 ghz
Alignment  4 - 17.853 bytes/cycle - 51077.60 MiB/sec @ 3 ghz
Alignment  3 - 17.844 bytes/cycle - 51051.63 MiB/sec @ 3 ghz
Alignment  2 - 17.854 bytes/cycle - 51080.37 MiB/sec @ 3 ghz
Alignment  1 - 17.912 bytes/cycle - 51245.99 MiB/sec @ 3 ghz
Alignment  0 - 19.064 bytes/cycle - 54541.55 MiB/sec @ 3 ghz
Average      - 18.006 bytes/cycle - 51517.01 MiB/sec @ 3 ghz

Small key speed test -    1-byte keys -    13.90 cycles/hash
Small key speed test -    2-byte keys -    14.59 cycles/hash
Small key speed test -    3-byte keys -    14.27 cycles/hash
Small key speed test -    4-byte keys -    14.32 cycles/hash
Small key speed test -    5-byte keys -    14.81 cycles/hash
Small key speed test -    6-byte keys -    14.99 cycles/hash
Small key speed test -    7-byte keys -    15.00 cycles/hash
Small key speed test -    8-byte keys -    14.00 cycles/hash
Small key speed test -    9-byte keys -    14.15 cycles/hash
Small key speed test -   10-byte keys -    14.00 cycles/hash
Small key speed test -   11-byte keys -    14.00 cycles/hash
Small key speed test -   12-byte keys -    14.00 cycles/hash
Small key speed test -   13-byte keys -    14.00 cycles/hash
Small key speed test -   14-byte keys -    14.11 cycles/hash
Small key speed test -   15-byte keys -    14.00 cycles/hash
Small key speed test -   16-byte keys -    14.72 cycles/hash
Small key speed test -   17-byte keys -    15.74 cycles/hash
Small key speed test -   18-byte keys -    15.62 cycles/hash
Small key speed test -   19-byte keys -    15.60 cycles/hash
Small key speed test -   20-byte keys -    15.66 cycles/hash
Small key speed test -   21-byte keys -    15.83 cycles/hash
Small key speed test -   22-byte keys -    15.69 cycles/hash
Small key speed test -   23-byte keys -    15.73 cycles/hash
Small key speed test -   24-byte keys -    15.63 cycles/hash
Small key speed test -   25-byte keys -    15.67 cycles/hash
Small key speed test -   26-byte keys -    15.66 cycles/hash
Small key speed test -   27-byte keys -    15.76 cycles/hash
Small key speed test -   28-byte keys -    15.67 cycles/hash
Small key speed test -   29-byte keys -    15.68 cycles/hash
Small key speed test -   30-byte keys -    15.65 cycles/hash
Small key speed test -   31-byte keys -    15.64 cycles/hash
Average                                    14.971 cycles/hash

[[[ 'Hashmap' Speed Tests ]]]

std::unordered_map
Init std HashMapTest:     148.866 cycles/op (466569 inserts, 1% deletions)
Running std HashMapTest:  85.492 cycles/op (1.0 stdv, found 461903)

greg7mdp/parallel-hashmap
Init fast HashMapTest:    158.746 cycles/op (466569 inserts, 1% deletions)
Running fast HashMapTest: 74.728 cycles/op (2.5 stdv, found 461903)

facil.io HashMap
Init fast fio_map_s Test:    62.909 cycles/op (466569 inserts, 1% deletions)
Running fast fio_map_s Test: 32.332 cycles/op (0.2 stdv, found 461903)
 ....... PASS

[[[ Avalanche Tests ]]]

Testing   24-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.585333%
Testing   32-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.695333%
Testing   40-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.619333%
Testing   48-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.662667%
Testing   56-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.706000%
Testing   64-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.693333%
Testing   72-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.730667%
Testing   80-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.706667%
Testing   96-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.672000%
Testing  112-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.786667%
Testing  128-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.621333%
Testing  160-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.710000%
Testing  512-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.835333%
Testing 1024-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.800000%

[[[ Keyset 'Sparse' Tests ]]]

Keyset 'Sparse' - 16-bit keys with up to 9 bits set - 50643 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (high 19-25 bits) - Worst is 19 bits: 2358/2368 (1.00x)
Testing collisions (low  32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (low  19-25 bits) - Worst is 24 bits: 90/76 (1.18x)
Testing distribution - Worst bias is the 13-bit window at bit 41 - 0.600%

Keyset 'Sparse' - 24-bit keys with up to 8 bits set - 1271626 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        188.2, actual    179 (0.95x)
Testing collisions (high 24-35 bits) - Worst is 25 bits: 23889/23794 (1.00x)
Testing collisions (low  32-bit) - Expected        188.2, actual    215 (1.14x) (27)
Testing collisions (low  24-35 bits) - Worst is 33 bits: 108/94 (1.15x)
Testing distribution - Worst bias is the 17-bit window at bit 16 - 0.075%

Keyset 'Sparse' - 32-bit keys with up to 7 bits set - 4514873 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2372.2, actual   2390 (1.01x) (18)
Testing collisions (high 25-38 bits) - Worst is 34 bits: 602/593 (1.01x)
Testing collisions (low  32-bit) - Expected       2372.2, actual   2313 (0.98x)
Testing collisions (low  25-38 bits) - Worst is 35 bits: 301/296 (1.01x)
Testing distribution - Worst bias is the 19-bit window at bit 15 - 0.051%

Keyset 'Sparse' - 40-bit keys with up to 6 bits set - 4598479 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2460.8, actual   2509 (1.02x) (49)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 42/38 (1.09x)
Testing collisions (low  32-bit) - Expected       2460.8, actual   2449 (1.00x) (-11)
Testing collisions (low  25-38 bits) - Worst is 35 bits: 316/307 (1.03x)
Testing distribution - Worst bias is the 19-bit window at bit 12 - 0.058%

Keyset 'Sparse' - 48-bit keys with up to 6 bits set - 14196869 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      23437.8, actual  23532 (1.00x) (95)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 28/22 (1.22x)
Testing collisions (low  32-bit) - Expected      23437.8, actual  23333 (1.00x) (-104)
Testing collisions (low  27-42 bits) - Worst is 40 bits: 96/91 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 16 - 0.020%

Keyset 'Sparse' - 56-bit keys with up to 5 bits set - 4216423 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2069.0, actual   2026 (0.98x)
Testing collisions (high 25-38 bits) - Worst is 34 bits: 518/517 (1.00x)
Testing collisions (low  32-bit) - Expected       2069.0, actual   2139 (1.03x) (71)
Testing collisions (low  25-38 bits) - Worst is 36 bits: 142/129 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit  8 - 0.043%

Keyset 'Sparse' - 64-bit keys with up to 5 bits set - 8303633 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8021.7, actual   8072 (1.01x) (51)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 37/31 (1.18x)
Testing collisions (low  32-bit) - Expected       8021.7, actual   8035 (1.00x) (14)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 32/31 (1.02x)
Testing distribution - Worst bias is the 20-bit window at bit 37 - 0.039%

Keyset 'Sparse' - 72-bit keys with up to 5 bits set - 15082603 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      26451.8, actual  26268 (0.99x) (-183)
Testing collisions (high 27-42 bits) - Worst is 39 bits: 225/206 (1.09x)
Testing collisions (low  32-bit) - Expected      26451.8, actual  26552 (1.00x) (101)
Testing collisions (low  27-42 bits) - Worst is 35 bits: 3381/3309 (1.02x)
Testing distribution - Worst bias is the 20-bit window at bit 36 - 0.027%

Keyset 'Sparse' - 96-bit keys with up to 4 bits set - 3469497 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1401.0, actual   1392 (0.99x) (-8)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 93/87 (1.06x)
Testing collisions (low  32-bit) - Expected       1401.0, actual   1402 (1.00x) (2)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 48/43 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit 11 - 0.051%

Keyset 'Sparse' - 160-bit keys with up to 4 bits set - 26977161 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      84546.1, actual  84403 (1.00x) (-143)
Testing collisions (high 28-44 bits) - Worst is 34 bits: 21303/21169 (1.01x)
Testing collisions (low  32-bit) - Expected      84546.1, actual  84679 (1.00x) (133)
Testing collisions (low  28-44 bits) - Worst is 43 bits: 45/41 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 49 - 0.014%

Keyset 'Sparse' - 256-bit keys with up to 3 bits set - 2796417 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        910.2, actual    930 (1.02x) (20)
Testing collisions (high 25-37 bits) - Worst is 36 bits: 71/56 (1.25x)
Testing collisions (low  32-bit) - Expected        910.2, actual    922 (1.01x) (12)
Testing collisions (low  25-37 bits) - Worst is 35 bits: 121/113 (1.06x)
Testing distribution - Worst bias is the 19-bit window at bit 52 - 0.094%

Keyset 'Sparse' - 512-bit keys with up to 3 bits set - 22370049 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      58155.4, actual  58001 (1.00x) (-154)
Testing collisions (high 28-43 bits) - Worst is 42 bits: 69/56 (1.21x)
Testing collisions (low  32-bit) - Expected      58155.4, actual  58151 (1.00x) (-4)
Testing collisions (low  28-43 bits) - Worst is 41 bits: 121/113 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit  3 - 0.015%

Keyset 'Sparse' - 1024-bit keys with up to 2 bits set - 524801 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         32.1, actual     37 (1.15x) (5)
Testing collisions (high 22-32 bits) - Worst is 31 bits: 75/64 (1.17x)
Testing collisions (low  32-bit) - Expected         32.1, actual     40 (1.25x) (8)
Testing collisions (low  22-32 bits) - Worst is 32 bits: 40/32 (1.25x)
Testing distribution - Worst bias is the 16-bit window at bit 17 - 0.136%

Keyset 'Sparse' - 2048-bit keys with up to 2 bits set - 2098177 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        512.4, actual    551 (1.08x) (39)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1105/1024 (1.08x)
Testing collisions (low  32-bit) - Expected        512.4, actual    526 (1.03x) (14)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/64 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 52 - 0.103%


[[[ Keyset 'Permutation' Tests ]]]

Combination Lowbits Tests:
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    670 (1.00x) (2)
Testing collisions (high 24-37 bits) - Worst is 37 bits: 28/20 (1.34x)
Testing collisions (low  32-bit) - Expected        668.6, actual    675 (1.01x) (7)
Testing collisions (low  24-37 bits) - Worst is 33 bits: 350/334 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit  1 - 0.086%


Combination Highbits Tests
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    620 (0.93x)
Testing collisions (high 24-37 bits) - Worst is 36 bits: 45/41 (1.08x)
Testing collisions (low  32-bit) - Expected        668.6, actual    634 (0.95x)
Testing collisions (low  24-37 bits) - Worst is 28 bits: 10778/10667 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.053%


Combination Hi-Lo Tests:
Keyset 'Combination' - up to 6 blocks from a set of 15 - 12204240 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      17322.9, actual  17547 (1.01x) (225)
Testing collisions (high 27-41 bits) - Worst is 38 bits: 283/270 (1.04x)
Testing collisions (low  32-bit) - Expected      17322.9, actual  17096 (0.99x) (-226)
Testing collisions (low  27-41 bits) - Worst is 41 bits: 37/33 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 46 - 0.035%


Combination 0x8000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8170 (1.00x) (-16)
Testing collisions (high 26-40 bits) - Worst is 33 bits: 4129/4094 (1.01x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8208 (1.00x) (22)
Testing collisions (low  26-40 bits) - Worst is 33 bits: 4128/4094 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit 21 - 0.041%


Combination 0x0000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8179 (1.00x) (-7)
Testing collisions (high 26-40 bits) - Worst is 31 bits: 16584/16362 (1.01x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8243 (1.01x) (57)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 270/255 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit  5 - 0.042%


Combination 0x800000000000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8296 (1.01x) (110)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 74/63 (1.16x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8168 (1.00x) (-18)
Testing collisions (low  26-40 bits) - Worst is 38 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 20-bit window at bit 55 - 0.035%


Combination 0x000000000000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8186 (1.00x)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 72/63 (1.13x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8238 (1.01x) (52)
Testing collisions (low  26-40 bits) - Worst is 35 bits: 1063/1023 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 63 - 0.031%


Combination 16-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8212 (1.00x) (26)
Testing collisions (high 26-40 bits) - Worst is 34 bits: 2056/2047 (1.00x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8083 (0.99x) (-103)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 259/255 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit 46 - 0.037%


Combination 16-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8052 (0.98x) (-134)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 66/63 (1.03x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8267 (1.01x) (81)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 20-bit window at bit  2 - 0.045%


Combination 32-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8136 (0.99x) (-50)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 130/127 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8138 (0.99x) (-48)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit 58 - 0.044%


Combination 32-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   7961 (0.97x)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8088 (0.99x) (-98)
Testing collisions (low  26-40 bits) - Worst is 35 bits: 1038/1023 (1.01x)
Testing distribution - Worst bias is the 20-bit window at bit 24 - 0.047%


Combination 64-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8054 (0.98x) (-132)
Testing collisions (high 26-40 bits) - Worst is 26 bits: 502359/503108 (1.00x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8309 (1.01x) (123)
Testing collisions (low  26-40 bits) - Worst is 34 bits: 2101/2047 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 14 - 0.052%


Combination 64-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8247 (1.01x) (61)
Testing collisions (high 26-40 bits) - Worst is 33 bits: 4159/4094 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8217 (1.00x) (31)
Testing collisions (low  26-40 bits) - Worst is 39 bits: 71/63 (1.11x)
Testing distribution - Worst bias is the 20-bit window at bit 31 - 0.052%


Combination 128-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8076 (0.99x) (-110)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 150/127 (1.17x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8278 (1.01x) (92)
Testing collisions (low  26-40 bits) - Worst is 35 bits: 1069/1023 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 36 - 0.041%


Combination 128-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8108 (0.99x) (-78)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8100 (0.99x) (-86)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 295/255 (1.15x)
Testing distribution - Worst bias is the 20-bit window at bit 55 - 0.049%


[[[ Keyset 'Window' Tests ]]]

Keyset 'Window' -  32-bit key,  25-bit window - 32 tests, 33554432 keys per test
Window at   0 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   1 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   2 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   3 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   4 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   5 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   6 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   7 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   8 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   9 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  10 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  11 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  12 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  13 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  14 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  15 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  16 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  17 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  18 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  19 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  20 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  21 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  22 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  23 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  24 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  25 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  26 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  27 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  28 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  29 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  30 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  31 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  32 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)

[[[ Keyset 'Cyclic' Tests ]]]

Keyset 'Cyclic' - 8 cycles of 8 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    116 (1.00x)
Testing collisions (high 23-34 bits) - Worst is 30 bits: 499/465 (1.07x)
Testing collisions (low  32-bit) - Expected        116.4, actual    102 (0.88x)
Testing collisions (low  23-34 bits) - Worst is 25 bits: 14836/14754 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 24 - 0.118%

Keyset 'Cyclic' - 8 cycles of 9 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual     96 (0.82x)
Testing collisions (high 23-34 bits) - Worst is 26 bits: 7398/7413 (1.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    116 (1.00x)
Testing collisions (low  23-34 bits) - Worst is 25 bits: 14917/14754 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 46 - 0.104%

Keyset 'Cyclic' - 8 cycles of 10 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    102 (0.88x)
Testing collisions (high 23-34 bits) - Worst is 29 bits: 934/930 (1.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 35/29 (1.20x)
Testing distribution - Worst bias is the 17-bit window at bit 39 - 0.070%

Keyset 'Cyclic' - 8 cycles of 11 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    112 (0.96x)
Testing collisions (high 23-34 bits) - Worst is 27 bits: 3790/3716 (1.02x)
Testing collisions (low  32-bit) - Expected        116.4, actual    140 (1.20x) (24)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 17-bit window at bit 39 - 0.112%

Keyset 'Cyclic' - 8 cycles of 12 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing collisions (low  32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (low  23-34 bits) - Worst is 30 bits: 483/465 (1.04x)
Testing distribution - Worst bias is the 17-bit window at bit 52 - 0.132%

Keyset 'Cyclic' - 8 cycles of 16 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    132 (1.13x) (16)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing collisions (low  32-bit) - Expected        116.4, actual    100 (0.86x)
Testing collisions (low  23-34 bits) - Worst is 29 bits: 947/930 (1.02x)
Testing distribution - Worst bias is the 17-bit window at bit 15 - 0.109%


[[[ Keyset 'TwoBytes' Tests ]]]

Keyset 'TwoBytes' - up-to-4-byte keys, 652545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         49.6, actual     38 (0.77x)
Testing collisions (high 23-33 bits) - Worst is 23 bits: 24681/24735 (1.00x)
Testing collisions (low  32-bit) - Expected         49.6, actual     44 (0.89x)
Testing collisions (low  23-33 bits) - Worst is 31 bits: 108/99 (1.09x)
Testing distribution - Worst bias is the 16-bit window at bit 22 - 0.120%

Keyset 'TwoBytes' - up-to-8-byte keys, 5471025 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       3483.1, actual   3447 (0.99x) (-36)
Testing collisions (high 26-39 bits) - Worst is 26 bits: 217954/217072 (1.00x)
Testing collisions (low  32-bit) - Expected       3483.1, actual   3344 (0.96x)
Testing collisions (low  26-39 bits) - Worst is 38 bits: 57/54 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 10 - 0.087%

Keyset 'TwoBytes' - up-to-12-byte keys, 18616785 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      40289.5, actual  40269 (1.00x) (-20)
Testing collisions (high 27-42 bits) - Worst is 38 bits: 644/630 (1.02x)
Testing collisions (low  32-bit) - Expected      40289.5, actual  40110 (1.00x) (-179)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 41/39 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 49 - 0.024%

Keyset 'TwoBytes' - up-to-16-byte keys, 44251425 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     227182.3, actual 227638 (1.00x) (456)
Testing collisions (high 29-45 bits) - Worst is 39 bits: 1794/1780 (1.01x)
Testing collisions (low  32-bit) - Expected     227182.3, actual 226966 (1.00x) (-216)
Testing collisions (low  29-45 bits) - Worst is 42 bits: 253/222 (1.14x)
Testing distribution - Worst bias is the 20-bit window at bit 46 - 0.009%

Keyset 'TwoBytes' - up-to-20-byte keys, 86536545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     865959.1, actual 866176 (1.00x) (217)
Testing collisions (high 30-47 bits) - Worst is 43 bits: 437/425 (1.03x)
Testing collisions (low  32-bit) - Expected     865959.1, actual 865959 (1.00x)
Testing collisions (low  30-47 bits) - Worst is 45 bits: 121/106 (1.14x)
Testing distribution - Worst bias is the 20-bit window at bit 45 - 0.004%


[[[ Keyset 'Text' Tests ]]]

Keyset 'Text' - keys of form "FooXXXXBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25652 (1.01x) (263)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 31/24 (1.25x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25380 (1.00x) (-9)
Testing collisions (low  27-42 bits) - Worst is 38 bits: 412/397 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 12 - 0.022%

Keyset 'Text' - keys of form "FooBarXXXX" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25507 (1.00x) (118)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 29/24 (1.17x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25496 (1.00x) (107)
Testing collisions (low  27-42 bits) - Worst is 39 bits: 204/198 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 61 - 0.021%

Keyset 'Text' - keys of form "XXXXFooBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25559 (1.01x) (170)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 60/49 (1.21x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25471 (1.00x) (82)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 35/24 (1.41x)
Testing distribution - Worst bias is the 20-bit window at bit 63 - 0.033%

Keyset 'Words' - 4000000 random keys of len 6-16 from alnum charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1822 (0.98x)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 30/29 (1.03x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1837 (0.99x) (-25)
Testing collisions (low  25-38 bits) - Worst is 25 bits: 229258/229220 (1.00x)
Testing distribution - Worst bias is the 19-bit window at bit 42 - 0.071%

Keyset 'Words' - 4000000 random keys of len 6-16 from password charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1827 (0.98x) (-35)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 131/116 (1.13x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1832 (0.98x) (-30)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 32/29 (1.10x)
Testing distribution - Worst bias is the 19-bit window at bit 24 - 0.069%

Keyset 'Words' - 466569 dict words
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         25.3, actual     27 (1.07x) (2)
Testing collisions (high 22-32 bits) - Worst is 32 bits: 27/25 (1.07x)
Testing collisions (low  32-bit) - Expected         25.3, actual     26 (1.03x) (1)
Testing collisions (low  22-32 bits) - Worst is 30 bits: 120/101 (1.18x)
Testing distribution - Worst bias is the 16-bit window at bit 48 - 0.155%


[[[ Keyset 'Zeroes' Tests ]]]

Keyset 'Zeroes' - 204800 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          4.9, actual      4 (0.82x)
Testing collisions (high 21-29 bits) - Worst is 28 bits: 83/78 (1.06x)
Testing collisions (low  32-bit) - Expected          4.9, actual      2 (0.41x)
Testing collisions (low  21-29 bits) - Worst is 27 bits: 169/156 (1.08x)
Testing distribution - Worst bias is the 15-bit window at bit 31 - 0.261%


[[[ Keyset 'Seed' Tests ]]]

Keyset 'Seed' - 5000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2909.3, actual   2808 (0.97x)
Testing collisions (high 26-39 bits) - Worst is 37 bits: 93/90 (1.02x)
Testing collisions (low  32-bit) - Expected       2909.3, actual   3032 (1.04x) (123)
Testing collisions (low  26-39 bits) - Worst is 38 bits: 64/45 (1.41x)
Testing distribution - Worst bias is the 19-bit window at bit 63 - 0.044%


[[[ Keyset 'PerlinNoise' Tests ]]]

Testing 16777216 coordinates (L2) : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      32725.4, actual  32777 (1.00x) (52)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 37/31 (1.16x)
Testing collisions (low  32-bit) - Expected      32725.4, actual  32511 (0.99x) (-214)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 36/31 (1.13x)

Testing AV variant, 128 count with 4 spacing, 4-12:
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1116.2, actual   1087 (0.97x)
Testing collisions (high 25-37 bits) - Worst is 27 bits: 35400/35452 (1.00x)
Testing collisions (low  32-bit) - Expected       1116.2, actual   1067 (0.96x)
Testing collisions (low  25-37 bits) - Worst is 34 bits: 280/279 (1.00x)


[[[ Diff 'Differential' Tests ]]]

Testing 8303632 up-to-5-bit differentials in 64-bit keys -> 64 bit hashes.
1000 reps, 8303632000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 11017632 up-to-4-bit differentials in 128-bit keys -> 64 bit hashes.
1000 reps, 11017632000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 2796416 up-to-3-bit differentials in 256-bit keys -> 64 bit hashes.
1000 reps, 2796416000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored


[[[ DiffDist 'Differential Distribution' Tests ]]]

Testing bit 0
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit  3 - 0.118%

Testing bit 1
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64220/64191 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 60 - 0.077%

Testing bit 2
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.078%

Testing bit 3
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1083/1023 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    470 (0.92x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2093/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.069%

Testing bit 4
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8315/8170 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    549 (1.07x) (38)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 52 - 0.081%

Testing bit 5
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing distribution - Worst bias is the 18-bit window at bit 40 - 0.116%

Testing bit 6
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing distribution - Worst bias is the 18-bit window at bit 14 - 0.106%

Testing bit 7
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    553 (1.08x) (42)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.095%

Testing bit 8
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 513/511 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.075%

Testing bit 9
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.113%

Testing bit 10
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32540/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 40 - 0.099%

Testing bit 11
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32534/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.059%

Testing bit 12
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 136/127 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 46/31 (1.44x)
Testing distribution - Worst bias is the 18-bit window at bit 10 - 0.058%

Testing bit 13
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8316/8170 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    553 (1.08x) (42)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.054%

Testing bit 14
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 43 - 0.088%

Testing bit 15
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 281/255 (1.10x)
Testing collisions (low  32-bit) - Expected        511.9, actual    555 (1.08x) (44)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.079%

Testing bit 16
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2084/2046 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 520/511 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 58 - 0.076%

Testing bit 17
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 260/255 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 138/127 (1.08x)
Testing distribution - Worst bias is the 17-bit window at bit 54 - 0.068%

Testing bit 18
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2114/2046 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.083%

Testing bit 19
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 529/511 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 27 bits: 16419/16298 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 29 - 0.080%

Testing bit 20
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1040/1023 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.092%

Testing bit 21
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1062/1023 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 34 - 0.079%

Testing bit 22
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 260/255 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    466 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 47 - 0.077%

Testing bit 23
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 26 bits: 32370/32429 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.064%

Testing bit 24
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 272/255 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 27 bits: 16412/16298 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 46 - 0.057%

Testing bit 25
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 269/255 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    549 (1.07x) (38)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 549/511 (1.07x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.089%

Testing bit 26
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1036/1023 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    481 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit  1 - 0.065%

Testing bit 27
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    465 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 13 - 0.101%

Testing bit 28
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    484 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 25 bits: 64180/64191 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    536 (1.05x) (25)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit  8 - 0.130%

Testing bit 29
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 126329/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1036/1023 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit  3 - 0.059%

Testing bit 30
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2107/2046 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    478 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64338/64191 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit  4 - 0.078%

Testing bit 31
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    493 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125635/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.072%

Testing bit 32
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125444/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4149/4090 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 12 - 0.094%

Testing bit 33
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 546/511 (1.07x)
Testing collisions (low  32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 139/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 45 - 0.069%

Testing bit 34
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing distribution - Worst bias is the 18-bit window at bit 52 - 0.133%

Testing bit 35
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 17-bit window at bit 31 - 0.062%

Testing bit 36
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 519/511 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2121/2046 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 37 - 0.119%

Testing bit 37
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing collisions (low  32-bit) - Expected        511.9, actual    465 (0.91x)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4123/4090 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.082%

Testing bit 38
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    542 (1.06x) (31)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1065/1023 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.058%

Testing bit 39
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    523 (1.02x) (12)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1056/1023 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    479 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125933/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 11 - 0.066%

Testing bit 40
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 47/31 (1.47x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 134/127 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.109%

Testing bit 41
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    474 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16336/16298 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1038/1023 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.067%

Testing bit 42
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 137/127 (1.07x)
Testing collisions (low  32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit  5 - 0.079%

Testing bit 43
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 38 - 0.089%

Testing bit 44
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing collisions (low  32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing distribution - Worst bias is the 18-bit window at bit 51 - 0.073%

Testing bit 45
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 529/511 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 276/255 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 47 - 0.092%

Testing bit 46
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 143/127 (1.12x)
Testing collisions (low  32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (low  24-36 bits) - Worst is 29 bits: 4172/4090 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit  9 - 0.073%

Testing bit 47
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 263/255 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    560 (1.09x) (49)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 24 - 0.073%

Testing bit 48
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4230/4090 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.069%

Testing bit 49
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 265/255 (1.04x)
Testing collisions (low  32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 49 - 0.077%

Testing bit 50
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit 53 - 0.088%

Testing bit 51
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    461 (0.90x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2065/2046 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2045/2046 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit  2 - 0.073%

Testing bit 52
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 527/511 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.083%

Testing bit 53
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 126008/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 18 - 0.083%

Testing bit 54
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 146/127 (1.14x)
Testing collisions (low  32-bit) - Expected        511.9, actual    544 (1.06x) (33)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 544/511 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 49 - 0.052%

Testing bit 55
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    454 (0.89x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125990/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8220/8170 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 21 - 0.108%

Testing bit 56
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    561 (1.10x) (50)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 148/127 (1.16x)
Testing collisions (low  32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 526/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.067%

Testing bit 57
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    468 (0.91x)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32528/32429 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing distribution - Worst bias is the 18-bit window at bit 46 - 0.111%

Testing bit 58
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 48/31 (1.50x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 264/255 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.080%

Testing bit 59
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2078/2046 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2113/2046 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.088%

Testing bit 60
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    465 (0.91x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8183/8170 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 38 - 0.067%

Testing bit 61
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 68/63 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.075%

Testing bit 62
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    572 (1.12x) (61)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 304/255 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.068%

Testing bit 63
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 529/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 39 - 0.089%


[[[ MomentChi2 Tests ]]]

Analyze hashes produced from a serie of linearly increasing numbers of 32-bit, using a step of 2 ... 
Target values to approximate : 38918200.000000 - 273633.333333 
Popcount 1 stats : 38919570.005124 - 273658.703091
Popcount 0 stats : 38918319.622388 - 273636.624134
MomentChi2 for bits 1 :   3.42946 
MomentChi2 for bits 0 :  0.0261471 

Derivative stats (transition from 2 consecutive values) : 
Popcount 1 stats : 38919286.374871 - 273651.807467
Popcount 0 stats : 38918700.222160 - 273641.544934
MomentChi2 for deriv b1 :   2.15648 
MomentChi2 for deriv b0 :  0.457215 

  Great 


[[[ Prng Tests ]]]

Generating 33554432 random numbers : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     130731.3, actual 130706 (1.00x) (-25)
Testing collisions (high 28-44 bits) - Worst is 44 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected     130731.3, actual 130394 (1.00x) (-337)
Testing collisions (low  28-44 bits) - Worst is 41 bits: 261/255 (1.02x)

[[[ BadSeeds Tests ]]]

0x0 PASS


Input vcode 0x00000001, Output vcode 0x00000001, Result vcode 0x00000001
Verification value is 0x00000001 - Testing took 681.305909 seconds
-------------------------------------------------------------------------------
```

-------------------------------------------------------------------------------
### Stable Hash 64bit SMHasher Results

The following results were achieved on my personal computer when testing the 64bit variant of the facil.io Stable Hash (`fio_stable_hash`).

```txt
-------------------------------------------------------------------------------
--- Testing Stable "facil.io Stable Hash 64bit" GOOD

[[[ Sanity Tests ]]]

Verification value 0x6993BB92 ....... PASS
Running sanity check 1     .......... PASS
Running AppendedZeroesTest .......... PASS

[[[ Speed Tests ]]]

Bulk speed test - 262144-byte keys
Alignment  7 - 12.548 bytes/cycle - 35899.15 MiB/sec @ 3 ghz
Alignment  6 - 12.560 bytes/cycle - 35934.05 MiB/sec @ 3 ghz
Alignment  5 - 12.554 bytes/cycle - 35917.07 MiB/sec @ 3 ghz
Alignment  4 - 12.560 bytes/cycle - 35935.44 MiB/sec @ 3 ghz
Alignment  3 - 12.559 bytes/cycle - 35931.79 MiB/sec @ 3 ghz
Alignment  2 - 12.557 bytes/cycle - 35926.78 MiB/sec @ 3 ghz
Alignment  1 - 12.561 bytes/cycle - 35936.39 MiB/sec @ 3 ghz
Alignment  0 - 12.678 bytes/cycle - 36273.41 MiB/sec @ 3 ghz
Average      - 12.572 bytes/cycle - 35969.26 MiB/sec @ 3 ghz

Small key speed test -    1-byte keys -    20.57 cycles/hash
Small key speed test -    2-byte keys -    21.00 cycles/hash
Small key speed test -    3-byte keys -    21.00 cycles/hash
Small key speed test -    4-byte keys -    20.99 cycles/hash
Small key speed test -    5-byte keys -    21.00 cycles/hash
Small key speed test -    6-byte keys -    21.00 cycles/hash
Small key speed test -    7-byte keys -    22.00 cycles/hash
Small key speed test -    8-byte keys -    21.00 cycles/hash
Small key speed test -    9-byte keys -    21.00 cycles/hash
Small key speed test -   10-byte keys -    21.00 cycles/hash
Small key speed test -   11-byte keys -    21.00 cycles/hash
Small key speed test -   12-byte keys -    21.00 cycles/hash
Small key speed test -   13-byte keys -    21.00 cycles/hash
Small key speed test -   14-byte keys -    21.00 cycles/hash
Small key speed test -   15-byte keys -    21.00 cycles/hash
Small key speed test -   16-byte keys -    21.00 cycles/hash
Small key speed test -   17-byte keys -    22.88 cycles/hash
Small key speed test -   18-byte keys -    22.92 cycles/hash
Small key speed test -   19-byte keys -    22.94 cycles/hash
Small key speed test -   20-byte keys -    22.93 cycles/hash
Small key speed test -   21-byte keys -    22.94 cycles/hash
Small key speed test -   22-byte keys -    22.92 cycles/hash
Small key speed test -   23-byte keys -    22.89 cycles/hash
Small key speed test -   24-byte keys -    22.89 cycles/hash
Small key speed test -   25-byte keys -    22.93 cycles/hash
Small key speed test -   26-byte keys -    22.90 cycles/hash
Small key speed test -   27-byte keys -    23.03 cycles/hash
Small key speed test -   28-byte keys -    22.81 cycles/hash
Small key speed test -   29-byte keys -    22.89 cycles/hash
Small key speed test -   30-byte keys -    23.13 cycles/hash
Small key speed test -   31-byte keys -    22.99 cycles/hash
Average                                    21.953 cycles/hash

[[[ 'Hashmap' Speed Tests ]]]

std::unordered_map
Init std HashMapTest:     155.249 cycles/op (466569 inserts, 1% deletions)
Running std HashMapTest:  93.391 cycles/op (1.6 stdv, found 461903)

greg7mdp/parallel-hashmap
Init fast HashMapTest:    143.332 cycles/op (466569 inserts, 1% deletions)
Running fast HashMapTest: 93.590 cycles/op (1.3 stdv, found 461903)

facil.io HashMap
Init fast fio_map_s Test:    87.173 cycles/op (466569 inserts, 1% deletions)
Running fast fio_map_s Test: 49.115 cycles/op (0.5 stdv, found 461903)
 ....... PASS

[[[ Avalanche Tests ]]]

Testing   24-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.632667%
Testing   32-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.712000%
Testing   40-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.704000%
Testing   48-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.696667%
Testing   56-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.686000%
Testing   64-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.712000%
Testing   72-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.636000%
Testing   80-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.630667%
Testing   96-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.690667%
Testing  112-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.670667%
Testing  128-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.760667%
Testing  160-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.773333%
Testing  512-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.825333%
Testing 1024-bit keys ->  64-bit hashes, 300000 reps.......... worst bias is 0.753333%

[[[ Keyset 'Sparse' Tests ]]]

Keyset 'Sparse' - 16-bit keys with up to 9 bits set - 50643 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (high 19-25 bits) - Worst is 23 bits: 171/152 (1.12x)
Testing collisions (low  32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (low  19-25 bits) - Worst is 21 bits: 602/606 (0.99x)
Testing distribution - Worst bias is the 13-bit window at bit 31 - 0.586%

Keyset 'Sparse' - 24-bit keys with up to 8 bits set - 1271626 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        188.2, actual    197 (1.05x) (9)
Testing collisions (high 24-35 bits) - Worst is 33 bits: 100/94 (1.06x)
Testing collisions (low  32-bit) - Expected        188.2, actual    170 (0.90x)
Testing collisions (low  24-35 bits) - Worst is 26 bits: 12071/11972 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 48 - 0.087%

Keyset 'Sparse' - 32-bit keys with up to 7 bits set - 4514873 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2372.2, actual   2345 (0.99x) (-27)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 47/37 (1.27x)
Testing collisions (low  32-bit) - Expected       2372.2, actual   2374 (1.00x) (2)
Testing collisions (low  25-38 bits) - Worst is 30 bits: 9492/9478 (1.00x)
Testing distribution - Worst bias is the 19-bit window at bit 35 - 0.057%

Keyset 'Sparse' - 40-bit keys with up to 6 bits set - 4598479 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2460.8, actual   2380 (0.97x)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 163/153 (1.06x)
Testing collisions (low  32-bit) - Expected       2460.8, actual   2490 (1.01x) (30)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 45/38 (1.17x)
Testing distribution - Worst bias is the 19-bit window at bit 50 - 0.061%

Keyset 'Sparse' - 48-bit keys with up to 6 bits set - 14196869 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      23437.8, actual  23277 (0.99x) (-160)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 28/22 (1.22x)
Testing collisions (low  32-bit) - Expected      23437.8, actual  23272 (0.99x) (-165)
Testing collisions (low  27-42 bits) - Worst is 30 bits: 93648/93442 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 29 - 0.022%

Keyset 'Sparse' - 56-bit keys with up to 5 bits set - 4216423 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2069.0, actual   2065 (1.00x) (-3)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 37/32 (1.14x)
Testing collisions (low  32-bit) - Expected       2069.0, actual   2042 (0.99x) (-26)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 40/32 (1.24x)
Testing distribution - Worst bias is the 19-bit window at bit 37 - 0.050%

Keyset 'Sparse' - 64-bit keys with up to 5 bits set - 8303633 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8021.7, actual   8020 (1.00x) (-1)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 36/31 (1.15x)
Testing collisions (low  32-bit) - Expected       8021.7, actual   8038 (1.00x) (17)
Testing collisions (low  26-40 bits) - Worst is 31 bits: 16101/16033 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 43 - 0.043%

Keyset 'Sparse' - 72-bit keys with up to 5 bits set - 15082603 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      26451.8, actual  26679 (1.01x) (228)
Testing collisions (high 27-42 bits) - Worst is 39 bits: 218/206 (1.05x)
Testing collisions (low  32-bit) - Expected      26451.8, actual  26434 (1.00x) (-17)
Testing collisions (low  27-42 bits) - Worst is 35 bits: 3324/3309 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 26 - 0.028%

Keyset 'Sparse' - 96-bit keys with up to 4 bits set - 3469497 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1401.0, actual   1462 (1.04x) (62)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 53/43 (1.21x)
Testing collisions (low  32-bit) - Expected       1401.0, actual   1406 (1.00x) (6)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 60/43 (1.37x)
Testing distribution - Worst bias is the 19-bit window at bit 12 - 0.088%

Keyset 'Sparse' - 160-bit keys with up to 4 bits set - 26977161 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      84546.1, actual  84527 (1.00x) (-19)
Testing collisions (high 28-44 bits) - Worst is 44 bits: 29/20 (1.40x)
Testing collisions (low  32-bit) - Expected      84546.1, actual  84586 (1.00x) (40)
Testing collisions (low  28-44 bits) - Worst is 41 bits: 172/165 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 39 - 0.017%

Keyset 'Sparse' - 256-bit keys with up to 3 bits set - 2796417 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        910.2, actual    947 (1.04x) (37)
Testing collisions (high 25-37 bits) - Worst is 36 bits: 63/56 (1.11x)
Testing collisions (low  32-bit) - Expected        910.2, actual    925 (1.02x) (15)
Testing collisions (low  25-37 bits) - Worst is 33 bits: 473/455 (1.04x)
Testing distribution - Worst bias is the 19-bit window at bit 50 - 0.082%

Keyset 'Sparse' - 512-bit keys with up to 3 bits set - 22370049 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      58155.4, actual  57664 (0.99x) (-491)
Testing collisions (high 28-43 bits) - Worst is 38 bits: 924/910 (1.02x)
Testing collisions (low  32-bit) - Expected      58155.4, actual  58193 (1.00x) (38)
Testing collisions (low  28-43 bits) - Worst is 39 bits: 498/455 (1.09x)
Testing distribution - Worst bias is the 20-bit window at bit 23 - 0.017%

Keyset 'Sparse' - 1024-bit keys with up to 2 bits set - 524801 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         32.1, actual     30 (0.94x)
Testing collisions (high 22-32 bits) - Worst is 28 bits: 518/512 (1.01x)
Testing collisions (low  32-bit) - Expected         32.1, actual     23 (0.72x)
Testing collisions (low  22-32 bits) - Worst is 29 bits: 294/256 (1.15x)
Testing distribution - Worst bias is the 16-bit window at bit 48 - 0.155%

Keyset 'Sparse' - 2048-bit keys with up to 2 bits set - 2098177 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        512.4, actual    499 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4139/4094 (1.01x)
Testing collisions (low  32-bit) - Expected        512.4, actual    529 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 143/128 (1.12x)
Testing distribution - Worst bias is the 18-bit window at bit 30 - 0.057%


[[[ Keyset 'Permutation' Tests ]]]

Combination Lowbits Tests:
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    670 (1.00x) (2)
Testing collisions (high 24-37 bits) - Worst is 36 bits: 44/41 (1.05x)
Testing collisions (low  32-bit) - Expected        668.6, actual    655 (0.98x)
Testing collisions (low  24-37 bits) - Worst is 34 bits: 173/167 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit  9 - 0.080%


Combination Highbits Tests
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    657 (0.98x) (-11)
Testing collisions (high 24-37 bits) - Worst is 37 bits: 24/20 (1.15x)
Testing collisions (low  32-bit) - Expected        668.6, actual    691 (1.03x) (23)
Testing collisions (low  24-37 bits) - Worst is 33 bits: 362/334 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit  9 - 0.047%


Combination Hi-Lo Tests:
Keyset 'Combination' - up to 6 blocks from a set of 15 - 12204240 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      17322.9, actual  17106 (0.99x) (-216)
Testing collisions (high 27-41 bits) - Worst is 41 bits: 37/33 (1.09x)
Testing collisions (low  32-bit) - Expected      17322.9, actual  17214 (0.99x) (-108)
Testing collisions (low  27-41 bits) - Worst is 28 bits: 273805/273271 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit  3 - 0.030%


Combination 0x8000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8288 (1.01x) (102)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8135 (0.99x) (-51)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 20-bit window at bit 52 - 0.038%


Combination 0x0000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8207 (1.00x) (21)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8100 (0.99x) (-86)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 20-bit window at bit 43 - 0.036%


Combination 0x800000000000000 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   7983 (0.98x)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 34/31 (1.06x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8152 (1.00x) (-34)
Testing collisions (low  26-40 bits) - Worst is 36 bits: 525/511 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit  4 - 0.054%


Combination 0x000000000000001 Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8201 (1.00x) (15)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 66/63 (1.03x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8064 (0.99x) (-122)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 20-bit window at bit 49 - 0.040%


Combination 16-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8156 (1.00x) (-30)
Testing collisions (high 26-40 bits) - Worst is 28 bits: 130239/129717 (1.00x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8355 (1.02x) (169)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 20-bit window at bit 44 - 0.040%


Combination 16-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8046 (0.98x) (-140)
Testing collisions (high 26-40 bits) - Worst is 36 bits: 550/511 (1.07x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8034 (0.98x) (-152)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 265/255 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 24 - 0.042%


Combination 32-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8155 (1.00x) (-31)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 84/63 (1.31x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8295 (1.01x) (109)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 41/31 (1.28x)
Testing distribution - Worst bias is the 20-bit window at bit  4 - 0.036%


Combination 32-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8248 (1.01x) (62)
Testing collisions (high 26-40 bits) - Worst is 37 bits: 279/255 (1.09x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8246 (1.01x) (60)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 20-bit window at bit 51 - 0.040%


Combination 64-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8174 (1.00x) (-12)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 136/127 (1.06x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8053 (0.98x) (-133)
Testing collisions (low  26-40 bits) - Worst is 37 bits: 269/255 (1.05x)
Testing distribution - Worst bias is the 20-bit window at bit 46 - 0.043%


Combination 64-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8175 (1.00x) (-11)
Testing collisions (high 26-40 bits) - Worst is 39 bits: 68/63 (1.06x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8116 (0.99x) (-70)
Testing collisions (low  26-40 bits) - Worst is 40 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 20-bit window at bit 10 - 0.042%


Combination 128-bytes [0-1] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8223 (1.00x) (37)
Testing collisions (high 26-40 bits) - Worst is 38 bits: 130/127 (1.02x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8294 (1.01x) (108)
Testing collisions (low  26-40 bits) - Worst is 38 bits: 149/127 (1.16x)
Testing distribution - Worst bias is the 20-bit window at bit 55 - 0.045%


Combination 128-bytes [0-last] Tests:
Keyset 'Combination' - up to 22 blocks from a set of 2 - 8388606 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8186.7, actual   8163 (1.00x) (-23)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected       8186.7, actual   8198 (1.00x) (12)
Testing collisions (low  26-40 bits) - Worst is 35 bits: 1055/1023 (1.03x)
Testing distribution - Worst bias is the 20-bit window at bit 45 - 0.041%


[[[ Keyset 'Window' Tests ]]]

Keyset 'Window' -  32-bit key,  25-bit window - 32 tests, 33554432 keys per test
Window at   0 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   1 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   2 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   3 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   4 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   5 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   6 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   7 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   8 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at   9 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  10 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  11 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  12 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  13 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  14 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  15 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  16 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  17 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  18 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  19 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  20 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  21 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  22 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  23 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  24 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  25 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  26 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  27 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  28 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  29 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  30 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  31 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Window at  32 - Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)

[[[ Keyset 'Cyclic' Tests ]]]

Keyset 'Cyclic' - 8 cycles of 8 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    104 (0.89x)
Testing collisions (high 23-34 bits) - Worst is 24 bits: 29355/29218 (1.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    107 (0.92x)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing distribution - Worst bias is the 17-bit window at bit 20 - 0.156%

Keyset 'Cyclic' - 8 cycles of 9 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    117 (1.01x) (1)
Testing collisions (high 23-34 bits) - Worst is 32 bits: 117/116 (1.01x)
Testing collisions (low  32-bit) - Expected        116.4, actual    111 (0.95x)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 17-bit window at bit 40 - 0.129%

Keyset 'Cyclic' - 8 cycles of 10 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual     95 (0.82x)
Testing collisions (high 23-34 bits) - Worst is 24 bits: 29375/29218 (1.01x)
Testing collisions (low  32-bit) - Expected        116.4, actual    136 (1.17x) (20)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 17-bit window at bit 12 - 0.166%

Keyset 'Cyclic' - 8 cycles of 11 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    110 (0.94x)
Testing collisions (high 23-34 bits) - Worst is 23 bits: 57228/57305 (1.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    128 (1.10x) (12)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 34/29 (1.17x)
Testing distribution - Worst bias is the 17-bit window at bit 40 - 0.096%

Keyset 'Cyclic' - 8 cycles of 12 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    102 (0.88x)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 30/29 (1.03x)
Testing collisions (low  32-bit) - Expected        116.4, actual    111 (0.95x)
Testing collisions (low  23-34 bits) - Worst is 24 bits: 29234/29218 (1.00x)
Testing distribution - Worst bias is the 17-bit window at bit 27 - 0.099%

Keyset 'Cyclic' - 8 cycles of 16 bytes - 1000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    128 (1.10x) (12)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing collisions (low  32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (low  23-34 bits) - Worst is 29 bits: 930/930 (1.00x)
Testing distribution - Worst bias is the 17-bit window at bit 30 - 0.148%


[[[ Keyset 'TwoBytes' Tests ]]]

Keyset 'TwoBytes' - up-to-4-byte keys, 652545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         49.6, actual     47 (0.95x)
Testing collisions (high 23-33 bits) - Worst is 33 bits: 29/24 (1.17x)
Testing collisions (low  32-bit) - Expected         49.6, actual     47 (0.95x)
Testing collisions (low  23-33 bits) - Worst is 33 bits: 27/24 (1.09x)
Testing distribution - Worst bias is the 16-bit window at bit 16 - 0.189%

Keyset 'TwoBytes' - up-to-8-byte keys, 5471025 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       3483.1, actual   3504 (1.01x) (21)
Testing collisions (high 26-39 bits) - Worst is 39 bits: 31/27 (1.14x)
Testing collisions (low  32-bit) - Expected       3483.1, actual   3493 (1.00x) (10)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 38/27 (1.40x)
Testing distribution - Worst bias is the 20-bit window at bit 52 - 0.096%

Keyset 'TwoBytes' - up-to-12-byte keys, 18616785 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      40289.5, actual  40041 (0.99x) (-248)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 45/39 (1.14x)
Testing collisions (low  32-bit) - Expected      40289.5, actual  40866 (1.01x) (577)
Testing collisions (low  27-42 bits) - Worst is 40 bits: 177/157 (1.12x)
Testing distribution - Worst bias is the 20-bit window at bit 18 - 0.021%

Keyset 'TwoBytes' - up-to-16-byte keys, 44251425 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     227182.3, actual 227276 (1.00x) (94)
Testing collisions (high 29-45 bits) - Worst is 40 bits: 938/890 (1.05x)
Testing collisions (low  32-bit) - Expected     227182.3, actual 227236 (1.00x) (54)
Testing collisions (low  29-45 bits) - Worst is 45 bits: 37/27 (1.33x)
Testing distribution - Worst bias is the 20-bit window at bit 18 - 0.006%

Keyset 'TwoBytes' - up-to-20-byte keys, 86536545 total keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     865959.1, actual 866307 (1.00x) (348)
Testing collisions (high 30-47 bits) - Worst is 37 bits: 27622/27237 (1.01x)
Testing collisions (low  32-bit) - Expected     865959.1, actual 865094 (1.00x) (-865)
Testing collisions (low  30-47 bits) - Worst is 46 bits: 68/53 (1.28x)
Testing distribution - Worst bias is the 20-bit window at bit 37 - 0.004%


[[[ Keyset 'Text' Tests ]]]

Keyset 'Text' - keys of form "FooXXXXBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25376 (1.00x) (-13)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 26/24 (1.05x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25263 (1.00x) (-126)
Testing collisions (low  27-42 bits) - Worst is 38 bits: 425/397 (1.07x)
Testing distribution - Worst bias is the 19-bit window at bit 27 - 0.018%

Keyset 'Text' - keys of form "FooBarXXXX" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25364 (1.00x) (-25)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 27/24 (1.09x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25570 (1.01x) (181)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 30/24 (1.21x)
Testing distribution - Worst bias is the 20-bit window at bit 13 - 0.031%

Keyset 'Text' - keys of form "XXXXFooBar" - 14776336 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25556 (1.01x) (167)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 63/49 (1.27x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25622 (1.01x) (233)
Testing collisions (low  27-42 bits) - Worst is 39 bits: 212/198 (1.07x)
Testing distribution - Worst bias is the 20-bit window at bit 54 - 0.031%

Keyset 'Words' - 4000000 random keys of len 6-16 from alnum charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1859 (1.00x) (-3)
Testing collisions (high 25-38 bits) - Worst is 37 bits: 70/58 (1.20x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1926 (1.03x) (64)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 19-bit window at bit 30 - 0.067%

Keyset 'Words' - 4000000 random keys of len 6-16 from password charset
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1860 (1.00x) (-2)
Testing collisions (high 25-38 bits) - Worst is 27 bits: 59197/59016 (1.00x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1829 (0.98x) (-33)
Testing collisions (low  25-38 bits) - Worst is 25 bits: 228540/229220 (1.00x)
Testing distribution - Worst bias is the 19-bit window at bit  7 - 0.042%

Keyset 'Words' - 466569 dict words
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         25.3, actual     31 (1.22x) (6)
Testing collisions (high 22-32 bits) - Worst is 32 bits: 31/25 (1.22x)
Testing collisions (low  32-bit) - Expected         25.3, actual     20 (0.79x)
Testing collisions (low  22-32 bits) - Worst is 26 bits: 1650/1618 (1.02x)
Testing distribution - Worst bias is the 16-bit window at bit  1 - 0.220%


[[[ Keyset 'Zeroes' Tests ]]]

Keyset 'Zeroes' - 204800 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          4.9, actual      5 (1.02x) (1)
Testing collisions (high 21-29 bits) - Worst is 29 bits: 44/39 (1.13x)
Testing collisions (low  32-bit) - Expected          4.9, actual      4 (0.82x)
Testing collisions (low  21-29 bits) - Worst is 29 bits: 49/39 (1.25x)
Testing distribution - Worst bias is the 15-bit window at bit 29 - 0.313%


[[[ Keyset 'Seed' Tests ]]]

Keyset 'Seed' - 5000000 keys
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2909.3, actual   2994 (1.03x) (85)
Testing collisions (high 26-39 bits) - Worst is 37 bits: 103/90 (1.13x)
Testing collisions (low  32-bit) - Expected       2909.3, actual   2973 (1.02x) (64)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 36/22 (1.58x)
Testing distribution - Worst bias is the 19-bit window at bit 10 - 0.042%


[[[ Keyset 'PerlinNoise' Tests ]]]

Testing 16777216 coordinates (L2) : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      32725.4, actual  32957 (1.01x) (232)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected      32725.4, actual  32857 (1.00x) (132)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 69/63 (1.08x)

Testing AV variant, 128 count with 4 spacing, 4-12:
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1116.2, actual   1110 (0.99x) (-6)
Testing collisions (high 25-37 bits) - Worst is 31 bits: 2271/2231 (1.02x)
Testing collisions (low  32-bit) - Expected       1116.2, actual   1068 (0.96x)
Testing collisions (low  25-37 bits) - Worst is 37 bits: 42/34 (1.20x)


[[[ Diff 'Differential' Tests ]]]

Testing 8303632 up-to-5-bit differentials in 64-bit keys -> 64 bit hashes.
1000 reps, 8303632000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 11017632 up-to-4-bit differentials in 128-bit keys -> 64 bit hashes.
1000 reps, 11017632000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 2796416 up-to-3-bit differentials in 256-bit keys -> 64 bit hashes.
1000 reps, 2796416000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored


[[[ DiffDist 'Differential Distribution' Tests ]]]

Testing bit 0
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    473 (0.92x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 145/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 23 - 0.095%

Testing bit 1
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 17-bit window at bit 23 - 0.077%

Testing bit 2
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    479 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8185/8170 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 136/127 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.062%

Testing bit 3
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    490 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2047/2046 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8326/8170 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 10 - 0.061%

Testing bit 4
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    463 (0.90x)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64159/64191 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.076%

Testing bit 5
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 135/127 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 58 - 0.071%

Testing bit 6
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    503 (0.98x) (-8)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 17-bit window at bit 28 - 0.054%

Testing bit 7
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    553 (1.08x) (42)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 278/255 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing distribution - Worst bias is the 17-bit window at bit 60 - 0.058%

Testing bit 8
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing collisions (low  32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 519/511 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.088%

Testing bit 9
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 269/255 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 147/127 (1.15x)
Testing distribution - Worst bias is the 18-bit window at bit 29 - 0.063%

Testing bit 10
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 131/127 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.081%

Testing bit 11
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 67/63 (1.05x)
Testing distribution - Worst bias is the 17-bit window at bit 45 - 0.064%

Testing bit 12
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    565 (1.10x) (54)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 79/63 (1.23x)
Testing distribution - Worst bias is the 18-bit window at bit 16 - 0.086%

Testing bit 13
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 517/511 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    471 (0.92x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125649/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 29 - 0.071%

Testing bit 14
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 66/63 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 54 - 0.104%

Testing bit 15
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 513/511 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2182/2046 (1.07x)
Testing distribution - Worst bias is the 18-bit window at bit  7 - 0.062%

Testing bit 16
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2094/2046 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.103%

Testing bit 17
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    480 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 25 bits: 64414/64191 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    480 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 30 - 0.090%

Testing bit 18
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.072%

Testing bit 19
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    556 (1.09x) (45)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 45 - 0.055%

Testing bit 20
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 283/255 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    548 (1.07x) (37)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.061%

Testing bit 21
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    489 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4170/4090 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.063%

Testing bit 22
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 135/127 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.111%

Testing bit 23
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    486 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8215/8170 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.096%

Testing bit 24
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 139/127 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.085%

Testing bit 25
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    484 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit  2 - 0.072%

Testing bit 26
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1046/1023 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 44 - 0.060%

Testing bit 27
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 12 - 0.067%

Testing bit 28
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1027/1023 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 44 - 0.072%

Testing bit 29
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    560 (1.09x) (49)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 153/127 (1.20x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 17-bit window at bit 59 - 0.067%

Testing bit 30
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    484 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4102/4090 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 516/511 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 16 - 0.084%

Testing bit 31
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    549 (1.07x) (38)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 549/511 (1.07x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit  6 - 0.070%

Testing bit 32
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 138/127 (1.08x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 31 - 0.086%

Testing bit 33
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    536 (1.05x) (25)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 271/255 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    568 (1.11x) (57)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 79/63 (1.23x)
Testing distribution - Worst bias is the 18-bit window at bit  9 - 0.075%

Testing bit 34
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    510 (1.00x) (-1)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 257/255 (1.00x)
Testing distribution - Worst bias is the 17-bit window at bit 24 - 0.047%

Testing bit 35
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 17-bit window at bit 28 - 0.061%

Testing bit 36
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    506 (0.99x) (-5)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 43/31 (1.34x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2062/2046 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 16 - 0.092%

Testing bit 37
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.085%

Testing bit 38
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    489 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    556 (1.09x) (45)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 556/511 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.045%

Testing bit 39
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing collisions (low  32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 282/255 (1.10x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.081%

Testing bit 40
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing collisions (low  32-bit) - Expected        511.9, actual    459 (0.90x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.095%

Testing bit 41
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 135/127 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 270/255 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 31 - 0.100%

Testing bit 42
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1041/1023 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    477 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2094/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.080%

Testing bit 43
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 276/255 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.091%

Testing bit 44
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2095/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 48 - 0.088%

Testing bit 45
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    559 (1.09x) (48)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    558 (1.09x) (47)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit  4 - 0.073%

Testing bit 46
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 271/255 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 50 - 0.073%

Testing bit 47
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    472 (0.92x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125700/125777 (1.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 11 - 0.084%

Testing bit 48
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 26 bits: 32601/32429 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 139/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 46 - 0.053%

Testing bit 49
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1046/1023 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    544 (1.06x) (33)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit  5 - 0.074%

Testing bit 50
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    558 (1.09x) (47)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 66/63 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 56 - 0.061%

Testing bit 51
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 62 - 0.079%

Testing bit 52
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8213/8170 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    477 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 28 - 0.110%

Testing bit 53
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit  7 - 0.073%

Testing bit 54
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 283/255 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit  5 - 0.079%

Testing bit 55
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 270/255 (1.05x)
Testing collisions (low  32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing distribution - Worst bias is the 18-bit window at bit 54 - 0.066%

Testing bit 56
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4143/4090 (1.01x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing distribution - Worst bias is the 18-bit window at bit 17 - 0.112%

Testing bit 57
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 57 - 0.142%

Testing bit 58
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    496 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 131/127 (1.02x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 269/255 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.091%

Testing bit 59
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    541 (1.06x) (30)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 146/127 (1.14x)
Testing distribution - Worst bias is the 18-bit window at bit  4 - 0.056%

Testing bit 60
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 82/63 (1.28x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.083%

Testing bit 61
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    545 (1.06x) (34)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 545/511 (1.06x)
Testing collisions (low  32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 526/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 12 - 0.083%

Testing bit 62
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 75/63 (1.17x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 37 - 0.095%

Testing bit 63
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 264/255 (1.03x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 68/63 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit  8 - 0.077%


[[[ MomentChi2 Tests ]]]

Analyze hashes produced from a serie of linearly increasing numbers of 32-bit, using a step of 2 ... 
Target values to approximate : 38918200.000000 - 273633.333333 
Popcount 1 stats : 38918998.920034 - 273638.558723
Popcount 0 stats : 38918770.531038 - 273645.996303
MomentChi2 for bits 1 :   1.16628 
MomentChi2 for bits 0 :  0.594771 

Derivative stats (transition from 2 consecutive values) : 
Popcount 1 stats : 38919257.619283 - 273628.012809
Popcount 0 stats : 38918472.697479 - 273650.071385
MomentChi2 for deriv b1 :   2.04392 
MomentChi2 for deriv b0 :  0.135878 

  Great 


[[[ Prng Tests ]]]

Generating 33554432 random numbers : 
Testing collisions ( 64-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     130731.3, actual 130884 (1.00x) (153)
Testing collisions (high 28-44 bits) - Worst is 43 bits: 75/63 (1.17x)
Testing collisions (low  32-bit) - Expected     130731.3, actual 131211 (1.00x) (480)
Testing collisions (low  28-44 bits) - Worst is 43 bits: 68/63 (1.06x)

[[[ BadSeeds Tests ]]]

0x0 PASS


Input vcode 0x00000001, Output vcode 0x00000001, Result vcode 0x00000001
Verification value is 0x00000001 - Testing took 755.424002 seconds
-------------------------------------------------------------------------------
```

-------------------------------------------------------------------------------
### Stable Hash 128bit SMHasher Results

The following results were achieved on my personal computer when testing the 128bit variant of the facil.io Stable Hash (`fio_stable_hash128`).

```txt
-------------------------------------------------------------------------------
--- Testing Stable128 "facil.io Stable Hash 128bit" GOOD

[[[ Sanity Tests ]]]

Verification value 0x97D1AB52 ....... PASS
Running sanity check 1     .......... PASS
Running AppendedZeroesTest .......... PASS

[[[ Speed Tests ]]]

Bulk speed test - 262144-byte keys
Alignment  7 - 12.633 bytes/cycle - 36143.93 MiB/sec @ 3 ghz
Alignment  6 - 12.651 bytes/cycle - 36193.49 MiB/sec @ 3 ghz
Alignment  5 - 12.646 bytes/cycle - 36179.39 MiB/sec @ 3 ghz
Alignment  4 - 12.644 bytes/cycle - 36174.76 MiB/sec @ 3 ghz
Alignment  3 - 12.656 bytes/cycle - 36208.15 MiB/sec @ 3 ghz
Alignment  2 - 12.650 bytes/cycle - 36192.29 MiB/sec @ 3 ghz
Alignment  1 - 12.644 bytes/cycle - 36175.29 MiB/sec @ 3 ghz
Alignment  0 - 12.797 bytes/cycle - 36612.89 MiB/sec @ 3 ghz
Average      - 12.665 bytes/cycle - 36235.02 MiB/sec @ 3 ghz

Small key speed test -    1-byte keys -    21.00 cycles/hash
Small key speed test -    2-byte keys -    21.81 cycles/hash
Small key speed test -    3-byte keys -    22.97 cycles/hash
Small key speed test -    4-byte keys -    21.99 cycles/hash
Small key speed test -    5-byte keys -    22.69 cycles/hash
Small key speed test -    6-byte keys -    22.00 cycles/hash
Small key speed test -    7-byte keys -    23.92 cycles/hash
Small key speed test -    8-byte keys -    21.98 cycles/hash
Small key speed test -    9-byte keys -    21.99 cycles/hash
Small key speed test -   10-byte keys -    21.99 cycles/hash
Small key speed test -   11-byte keys -    21.99 cycles/hash
Small key speed test -   12-byte keys -    21.99 cycles/hash
Small key speed test -   13-byte keys -    21.99 cycles/hash
Small key speed test -   14-byte keys -    21.99 cycles/hash
Small key speed test -   15-byte keys -    21.99 cycles/hash
Small key speed test -   16-byte keys -    22.00 cycles/hash
Small key speed test -   17-byte keys -    23.98 cycles/hash
Small key speed test -   18-byte keys -    23.98 cycles/hash
Small key speed test -   19-byte keys -    23.98 cycles/hash
Small key speed test -   20-byte keys -    23.98 cycles/hash
Small key speed test -   21-byte keys -    23.97 cycles/hash
Small key speed test -   22-byte keys -    23.98 cycles/hash
Small key speed test -   23-byte keys -    23.98 cycles/hash
Small key speed test -   24-byte keys -    23.97 cycles/hash
Small key speed test -   25-byte keys -    23.97 cycles/hash
Small key speed test -   26-byte keys -    23.97 cycles/hash
Small key speed test -   27-byte keys -    23.97 cycles/hash
Small key speed test -   28-byte keys -    23.97 cycles/hash
Small key speed test -   29-byte keys -    23.97 cycles/hash
Small key speed test -   30-byte keys -    23.98 cycles/hash
Small key speed test -   31-byte keys -    24.18 cycles/hash
Average                                    23.037 cycles/hash

[[[ 'Hashmap' Speed Tests ]]]

std::unordered_map
Init std HashMapTest:     151.297 cycles/op (466569 inserts, 1% deletions)
Running std HashMapTest:  93.079 cycles/op (1.2 stdv, found 461903)

greg7mdp/parallel-hashmap
Init fast HashMapTest:    145.520 cycles/op (466569 inserts, 1% deletions)
Running fast HashMapTest: 91.867 cycles/op (0.7 stdv, found 461903)

facil.io HashMap
Init fast fio_map_s Test:    84.812 cycles/op (466569 inserts, 1% deletions)
Running fast fio_map_s Test: 48.436 cycles/op (0.4 stdv, found 461903)
 ....... PASS

[[[ Avalanche Tests ]]]

Testing   24-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.659333%
Testing   32-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.712000%
Testing   40-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.723333%
Testing   48-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.696667%
Testing   56-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.694000%
Testing   64-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.712000%
Testing   72-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.676000%
Testing   80-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.704667%
Testing   96-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.690667%
Testing  112-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.721333%
Testing  128-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.760667%
Testing  160-bit keys -> 128-bit hashes, 300000 reps.......... worst bias is 0.773333%

[[[ Keyset 'Sparse' Tests ]]]

Keyset 'Sparse' - 16-bit keys with up to 9 bits set - 50643 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (high 19-25 bits) - Worst is 23 bits: 161/152 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          0.3, actual      0 (0.00x)
Testing collisions (low  19-25 bits) - Worst is 21 bits: 602/606 (0.99x)
Testing distribution - Worst bias is the 13-bit window at bit 31 - 0.586%

Keyset 'Sparse' - 24-bit keys with up to 8 bits set - 1271626 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        188.2, actual    190 (1.01x) (2)
Testing collisions (high 24-35 bits) - Worst is 32 bits: 190/188 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        188.2, actual    170 (0.90x)
Testing collisions (low  24-35 bits) - Worst is 26 bits: 12071/11972 (1.01x)
Testing distribution - Worst bias is the 17-bit window at bit 35 - 0.080%

Keyset 'Sparse' - 32-bit keys with up to 7 bits set - 4514873 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2372.2, actual   2355 (0.99x) (-17)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 39/37 (1.05x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2372.2, actual   2374 (1.00x) (2)
Testing collisions (low  25-38 bits) - Worst is 30 bits: 9492/9478 (1.00x)
Testing distribution - Worst bias is the 19-bit window at bit 74 - 0.065%

Keyset 'Sparse' - 40-bit keys with up to 6 bits set - 4598479 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2460.8, actual   2429 (0.99x) (-31)
Testing collisions (high 25-38 bits) - Worst is 29 bits: 19779/19637 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2460.8, actual   2490 (1.01x) (30)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 45/38 (1.17x)
Testing distribution - Worst bias is the 19-bit window at bit 82 - 0.051%

Keyset 'Sparse' - 48-bit keys with up to 6 bits set - 14196869 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      23437.8, actual  23636 (1.01x) (199)
Testing collisions (high 27-42 bits) - Worst is 42 bits: 24/22 (1.05x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      23437.8, actual  23272 (0.99x) (-165)
Testing collisions (low  27-42 bits) - Worst is 30 bits: 93648/93442 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 74 - 0.028%

Keyset 'Sparse' - 56-bit keys with up to 5 bits set - 4216423 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2069.0, actual   2045 (0.99x) (-23)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 33/32 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2069.0, actual   2042 (0.99x) (-26)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 40/32 (1.24x)
Testing distribution - Worst bias is the 19-bit window at bit 96 - 0.061%

Keyset 'Sparse' - 64-bit keys with up to 5 bits set - 8303633 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       8021.7, actual   8104 (1.01x) (83)
Testing collisions (high 26-40 bits) - Worst is 40 bits: 39/31 (1.24x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       8021.7, actual   8038 (1.00x) (17)
Testing collisions (low  26-40 bits) - Worst is 31 bits: 16101/16033 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 43 - 0.043%

Keyset 'Sparse' - 72-bit keys with up to 5 bits set - 15082603 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      26451.8, actual  26433 (1.00x) (-18)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 55/51 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      26451.8, actual  26434 (1.00x) (-17)
Testing collisions (low  27-42 bits) - Worst is 35 bits: 3324/3309 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit 26 - 0.028%

Keyset 'Sparse' - 96-bit keys with up to 4 bits set - 3469497 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1401.0, actual   1326 (0.95x)
Testing collisions (high 25-38 bits) - Worst is 38 bits: 22/21 (1.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1401.0, actual   1406 (1.00x) (6)
Testing collisions (low  25-38 bits) - Worst is 37 bits: 60/43 (1.37x)
Testing distribution - Worst bias is the 19-bit window at bit 12 - 0.088%

Keyset 'Sparse' - 160-bit keys with up to 4 bits set - 26977161 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      84546.1, actual  84181 (1.00x) (-365)
Testing collisions (high 28-44 bits) - Worst is 43 bits: 54/41 (1.31x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      84546.1, actual  84586 (1.00x) (40)
Testing collisions (low  28-44 bits) - Worst is 41 bits: 172/165 (1.04x)
Testing distribution - Worst bias is the 20-bit window at bit 39 - 0.017%

Keyset 'Sparse' - 256-bit keys with up to 3 bits set - 2796417 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        910.2, actual    920 (1.01x) (10)
Testing collisions (high 25-37 bits) - Worst is 36 bits: 65/56 (1.14x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        910.2, actual    925 (1.02x) (15)
Testing collisions (low  25-37 bits) - Worst is 33 bits: 473/455 (1.04x)
Testing distribution - Worst bias is the 19-bit window at bit 73 - 0.096%


[[[ Keyset 'Permutation' Tests ]]]

Combination Lowbits Tests:
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    735 (1.10x) (67)
Testing collisions (high 24-37 bits) - Worst is 34 bits: 207/167 (1.24x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        668.6, actual    655 (0.98x)
Testing collisions (low  24-37 bits) - Worst is 34 bits: 173/167 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit  9 - 0.080%


Combination Highbits Tests
Keyset 'Combination' - up to 7 blocks from a set of 8 - 2396744 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        668.6, actual    681 (1.02x) (13)
Testing collisions (high 24-37 bits) - Worst is 37 bits: 26/20 (1.24x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        668.6, actual    691 (1.03x) (23)
Testing collisions (low  24-37 bits) - Worst is 33 bits: 362/334 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 68 - 0.091%


Combination Hi-Lo Tests:
Keyset 'Combination' - up to 6 blocks from a set of 15 - 12204240 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      17322.9, actual  17433 (1.01x) (111)
Testing collisions (high 27-41 bits) - Worst is 34 bits: 4416/4333 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      17322.9, actual  17214 (0.99x) (-108)
Testing collisions (low  27-41 bits) - Worst is 28 bits: 273805/273271 (1.00x)
Testing distribution - Worst bias is the 20-bit window at bit  3 - 0.030%


Combination 0x8000000 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      9 (1.13x) (2)
Testing collisions (high 21-30 bits) - Worst is 28 bits: 135/127 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual     12 (1.50x) (5)
Testing collisions (low  21-30 bits) - Worst is 30 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 15-bit window at bit 48 - 0.268%


Combination 0x0000001 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      9 (1.13x) (2)
Testing collisions (high 21-30 bits) - Worst is 26 bits: 537/511 (1.05x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (low  21-30 bits) - Worst is 29 bits: 73/63 (1.14x)
Testing distribution - Worst bias is the 15-bit window at bit 119 - 0.328%


Combination 0x800000000000000 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      1 (0.13x)
Testing collisions (high 21-30 bits) - Worst is 24 bits: 2103/2037 (1.03x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (low  21-30 bits) - Worst is 30 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 15-bit window at bit 15 - 0.279%


Combination 0x000000000000001 Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      9 (1.13x) (2)
Testing collisions (high 21-30 bits) - Worst is 29 bits: 72/63 (1.13x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual     10 (1.25x) (3)
Testing collisions (low  21-30 bits) - Worst is 24 bits: 2041/2037 (1.00x)
Testing distribution - Worst bias is the 15-bit window at bit  2 - 0.203%


Combination 16-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      3 (0.38x)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 33/31 (1.03x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual     15 (1.88x) (8)
Testing collisions (low  21-30 bits) - Worst is 27 bits: 262/255 (1.02x)
Testing distribution - Worst bias is the 15-bit window at bit 21 - 0.203%


Combination 16-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (high 21-30 bits) - Worst is 28 bits: 132/127 (1.03x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (low  21-30 bits) - Worst is 25 bits: 1037/1021 (1.02x)
Testing distribution - Worst bias is the 15-bit window at bit 47 - 0.252%


Combination 32-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual     13 (1.63x) (6)
Testing collisions (high 21-30 bits) - Worst is 30 bits: 41/31 (1.28x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      6 (0.75x)
Testing collisions (low  21-30 bits) - Worst is 25 bits: 1034/1021 (1.01x)
Testing distribution - Worst bias is the 15-bit window at bit 43 - 0.259%


Combination 32-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      5 (0.63x)
Testing collisions (high 21-30 bits) - Worst is 29 bits: 67/63 (1.05x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      4 (0.50x)
Testing collisions (low  21-30 bits) - Worst is 29 bits: 70/63 (1.09x)
Testing distribution - Worst bias is the 15-bit window at bit 24 - 0.293%


Combination 64-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 22 bits: 8144/8023 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      3 (0.38x)
Testing collisions (low  21-30 bits) - Worst is 28 bits: 151/127 (1.18x)
Testing distribution - Worst bias is the 15-bit window at bit 74 - 0.329%


Combination 64-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      8 (1.00x) (1)
Testing collisions (high 21-30 bits) - Worst is 27 bits: 286/255 (1.12x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      4 (0.50x)
Testing collisions (low  21-30 bits) - Worst is 24 bits: 2098/2037 (1.03x)
Testing distribution - Worst bias is the 15-bit window at bit 118 - 0.211%


Combination 128-bytes [0-1] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (high 21-30 bits) - Worst is 26 bits: 553/511 (1.08x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual      7 (0.88x)
Testing collisions (low  21-30 bits) - Worst is 29 bits: 78/63 (1.22x)
Testing distribution - Worst bias is the 15-bit window at bit 57 - 0.323%


Combination 128-bytes [0-last] Tests:
Keyset 'Combination' - up to 17 blocks from a set of 2 - 262142 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          8.0, actual     11 (1.38x) (4)
Testing collisions (high 21-30 bits) - Worst is 28 bits: 136/127 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          8.0, actual     13 (1.63x) (6)
Testing collisions (low  21-30 bits) - Worst is 30 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 15-bit window at bit 125 - 0.223%


[[[ Keyset 'Window' Tests ]]]

Keyset 'Window' -  32-bit key,  25-bit window - 32 tests, 33554432 keys per test
Window at   0 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at   1 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at   2 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at   3 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at   4 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at   5 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at   6 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at   7 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at   8 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at   9 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  10 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  11 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  12 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  13 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  14 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  15 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  16 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  17 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  18 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  19 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  20 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  21 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  22 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  23 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  24 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  25 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  26 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  27 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  28 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  29 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  30 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  31 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Window at  32 - Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)

[[[ Keyset 'Cyclic' Tests ]]]

Keyset 'Cyclic' - 8 cycles of 16 bytes - 1000000 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (high 23-34 bits) - Worst is 33 bits: 63/58 (1.08x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (low  23-34 bits) - Worst is 29 bits: 930/930 (1.00x)
Testing distribution - Worst bias is the 17-bit window at bit 30 - 0.148%

Keyset 'Cyclic' - 8 cycles of 17 bytes - 1000000 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    115 (0.99x) (-1)
Testing collisions (high 23-34 bits) - Worst is 28 bits: 1937/1860 (1.04x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    123 (1.06x) (7)
Testing collisions (low  23-34 bits) - Worst is 31 bits: 264/232 (1.13x)
Testing distribution - Worst bias is the 17-bit window at bit 38 - 0.131%

Keyset 'Cyclic' - 8 cycles of 18 bytes - 1000000 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    104 (0.89x)
Testing collisions (high 23-34 bits) - Worst is 28 bits: 1911/1860 (1.03x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    113 (0.97x)
Testing collisions (low  23-34 bits) - Worst is 31 bits: 240/232 (1.03x)
Testing distribution - Worst bias is the 17-bit window at bit 58 - 0.123%

Keyset 'Cyclic' - 8 cycles of 19 bytes - 1000000 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    116 (1.00x)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 33/29 (1.13x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    100 (0.86x)
Testing collisions (low  23-34 bits) - Worst is 28 bits: 1891/1860 (1.02x)
Testing distribution - Worst bias is the 17-bit window at bit  4 - 0.103%

Keyset 'Cyclic' - 8 cycles of 20 bytes - 1000000 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual     97 (0.83x)
Testing collisions (high 23-34 bits) - Worst is 28 bits: 1903/1860 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    125 (1.07x) (9)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 17-bit window at bit 32 - 0.172%

Keyset 'Cyclic' - 8 cycles of 24 bytes - 1000000 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        116.4, actual    116 (1.00x)
Testing collisions (high 23-34 bits) - Worst is 34 bits: 32/29 (1.10x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        116.4, actual    131 (1.13x) (15)
Testing collisions (low  23-34 bits) - Worst is 34 bits: 38/29 (1.31x)
Testing distribution - Worst bias is the 17-bit window at bit 11 - 0.148%


[[[ Keyset 'TwoBytes' Tests ]]]

Keyset 'TwoBytes' - up-to-4-byte keys, 652545 total keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         49.6, actual     50 (1.01x) (1)
Testing collisions (high 23-33 bits) - Worst is 33 bits: 27/24 (1.09x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected         49.6, actual     47 (0.95x)
Testing collisions (low  23-33 bits) - Worst is 33 bits: 27/24 (1.09x)
Testing distribution - Worst bias is the 16-bit window at bit 16 - 0.189%

Keyset 'TwoBytes' - up-to-8-byte keys, 5471025 total keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       3483.1, actual   3479 (1.00x) (-4)
Testing collisions (high 26-39 bits) - Worst is 35 bits: 484/435 (1.11x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       3483.1, actual   3493 (1.00x) (10)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 38/27 (1.40x)
Testing distribution - Worst bias is the 20-bit window at bit 11 - 0.064%

Keyset 'TwoBytes' - up-to-12-byte keys, 18616785 total keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      40289.5, actual  39913 (0.99x) (-376)
Testing collisions (high 27-42 bits) - Worst is 41 bits: 81/78 (1.03x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      40289.5, actual  40866 (1.01x) (577)
Testing collisions (low  27-42 bits) - Worst is 40 bits: 177/157 (1.12x)
Testing distribution - Worst bias is the 20-bit window at bit 18 - 0.021%


[[[ Keyset 'Text' Tests ]]]

Keyset 'Text' - keys of form "FooXXXXBar" - 14776336 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25586 (1.01x) (197)
Testing collisions (high 27-42 bits) - Worst is 38 bits: 422/397 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25263 (1.00x) (-126)
Testing collisions (low  27-42 bits) - Worst is 38 bits: 425/397 (1.07x)
Testing distribution - Worst bias is the 20-bit window at bit 57 - 0.021%

Keyset 'Text' - keys of form "FooBarXXXX" - 14776336 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25629 (1.01x) (240)
Testing collisions (high 27-42 bits) - Worst is 34 bits: 6431/6352 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25570 (1.01x) (181)
Testing collisions (low  27-42 bits) - Worst is 42 bits: 30/24 (1.21x)
Testing distribution - Worst bias is the 20-bit window at bit 13 - 0.031%

Keyset 'Text' - keys of form "XXXXFooBar" - 14776336 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      25389.0, actual  25481 (1.00x) (92)
Testing collisions (high 27-42 bits) - Worst is 40 bits: 111/99 (1.12x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      25389.0, actual  25622 (1.01x) (233)
Testing collisions (low  27-42 bits) - Worst is 39 bits: 212/198 (1.07x)
Testing distribution - Worst bias is the 20-bit window at bit 110 - 0.026%

Keyset 'Words' - 4000000 random keys of len 6-16 from alnum charset
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1878 (1.01x) (16)
Testing collisions (high 25-38 bits) - Worst is 34 bits: 490/465 (1.05x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1926 (1.03x) (64)
Testing collisions (low  25-38 bits) - Worst is 38 bits: 36/29 (1.24x)
Testing distribution - Worst bias is the 19-bit window at bit 30 - 0.067%

Keyset 'Words' - 4000000 random keys of len 6-16 from password charset
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1862.1, actual   1898 (1.02x) (36)
Testing collisions (high 25-38 bits) - Worst is 36 bits: 138/116 (1.19x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1862.1, actual   1829 (0.98x) (-33)
Testing collisions (low  25-38 bits) - Worst is 25 bits: 228540/229220 (1.00x)
Testing distribution - Worst bias is the 19-bit window at bit 110 - 0.058%

Keyset 'Words' - 466569 dict words
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected         25.3, actual     34 (1.34x) (9)
Testing collisions (high 22-32 bits) - Worst is 32 bits: 34/25 (1.34x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected         25.3, actual     20 (0.79x)
Testing collisions (low  22-32 bits) - Worst is 26 bits: 1650/1618 (1.02x)
Testing distribution - Worst bias is the 16-bit window at bit  1 - 0.220%


[[[ Keyset 'Zeroes' Tests ]]]

Keyset 'Zeroes' - 204800 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected          4.9, actual      7 (1.43x) (3)
Testing collisions (high 21-29 bits) - Worst is 29 bits: 48/39 (1.23x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected          4.9, actual      4 (0.82x)
Testing collisions (low  21-29 bits) - Worst is 29 bits: 49/39 (1.25x)
Testing distribution - Worst bias is the 15-bit window at bit 29 - 0.313%


[[[ Keyset 'Seed' Tests ]]]

Keyset 'Seed' - 5000000 keys
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       2909.3, actual   2778 (0.95x)
Testing collisions (high 26-39 bits) - Worst is 37 bits: 96/90 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       2909.3, actual   2973 (1.02x) (64)
Testing collisions (low  26-39 bits) - Worst is 39 bits: 36/22 (1.58x)
Testing distribution - Worst bias is the 19-bit window at bit 79 - 0.055%


[[[ Keyset 'PerlinNoise' Tests ]]]

Testing 16777216 coordinates (L2) : 
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected      32725.4, actual  32905 (1.01x) (180)
Testing collisions (high 27-42 bits) - Worst is 40 bits: 142/127 (1.11x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected      32725.4, actual  32857 (1.00x) (132)
Testing collisions (low  27-42 bits) - Worst is 41 bits: 69/63 (1.08x)

Testing AV variant, 128 count with 4 spacing, 4-12:
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected       1116.2, actual   1102 (0.99x) (-14)
Testing collisions (high 25-37 bits) - Worst is 36 bits: 83/69 (1.19x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected       1116.2, actual   1068 (0.96x)
Testing collisions (low  25-37 bits) - Worst is 37 bits: 42/34 (1.20x)


[[[ Diff 'Differential' Tests ]]]

Testing 8303632 up-to-5-bit differentials in 64-bit keys -> 128 bit hashes.
1000 reps, 8303632000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 11017632 up-to-4-bit differentials in 128-bit keys -> 128 bit hashes.
1000 reps, 11017632000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored

Testing 2796416 up-to-3-bit differentials in 256-bit keys -> 128 bit hashes.
1000 reps, 2796416000 total tests, expecting 0.00 random collisions..........
0 total collisions, of which 0 single collisions were ignored


[[[ DiffDist 'Differential Distribution' Tests ]]]

Testing bit 0
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    544 (1.06x) (33)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 71/63 (1.11x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 145/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 79 - 0.123%

Testing bit 1
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2065/2046 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 99 - 0.092%

Testing bit 2
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    442 (0.86x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4169/4090 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    538 (1.05x) (27)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 136/127 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 107 - 0.080%

Testing bit 3
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    515 (1.01x) (4)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 28 bits: 8326/8170 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 113 - 0.077%

Testing bit 4
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 29 bits: 4107/4090 (1.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    463 (0.90x)
Testing collisions (low  24-36 bits) - Worst is 25 bits: 64159/64191 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.076%

Testing bit 5
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    482 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 28 bits: 8318/8170 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    534 (1.04x) (23)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 70/63 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 82 - 0.070%

Testing bit 6
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 127 - 0.104%

Testing bit 7
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16399/16298 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 142/127 (1.11x)
Testing distribution - Worst bias is the 18-bit window at bit 106 - 0.081%

Testing bit 8
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 276/255 (1.08x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 519/511 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 41 - 0.088%

Testing bit 9
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    565 (1.10x) (54)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 148/127 (1.16x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 147/127 (1.15x)
Testing distribution - Worst bias is the 18-bit window at bit 123 - 0.110%

Testing bit 10
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 89 - 0.083%

Testing bit 11
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 75/63 (1.17x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 67/63 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 68 - 0.087%

Testing bit 12
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 79/63 (1.23x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    565 (1.10x) (54)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 79/63 (1.23x)
Testing distribution - Worst bias is the 18-bit window at bit 126 - 0.095%

Testing bit 13
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 68/63 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    471 (0.92x)
Testing collisions (low  24-36 bits) - Worst is 24 bits: 125649/125777 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 119 - 0.082%

Testing bit 14
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 83/63 (1.30x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 76/63 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 53 - 0.073%

Testing bit 15
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2182/2046 (1.07x)
Testing distribution - Worst bias is the 18-bit window at bit 126 - 0.104%

Testing bit 16
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 277/255 (1.08x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    497 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 108 - 0.101%

Testing bit 17
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    480 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 69/63 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 30 - 0.090%

Testing bit 18
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 73/63 (1.14x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 44/31 (1.38x)
Testing distribution - Worst bias is the 18-bit window at bit 81 - 0.101%

Testing bit 19
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    563 (1.10x) (52)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 72/63 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 54 - 0.106%

Testing bit 20
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1041/1023 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    548 (1.07x) (37)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 61 - 0.071%

Testing bit 21
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    511 (1.00x)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 271/255 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    498 (0.97x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 121 - 0.092%

Testing bit 22
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 25 bits: 63947/64191 (1.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    535 (1.05x) (24)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 135/127 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 35 - 0.111%

Testing bit 23
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    555 (1.08x) (44)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 555/511 (1.08x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 42 - 0.096%

Testing bit 24
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    486 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 25 bits: 64468/64191 (1.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 79 - 0.109%

Testing bit 25
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    493 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    532 (1.04x) (21)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 53 - 0.081%

Testing bit 26
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    485 (0.95x)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16451/16298 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    524 (1.02x) (13)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 75 - 0.093%

Testing bit 27
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16466/16298 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    513 (1.00x) (2)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 40/31 (1.25x)
Testing distribution - Worst bias is the 18-bit window at bit 107 - 0.098%

Testing bit 28
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    514 (1.00x) (3)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1060/1023 (1.04x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (low  24-36 bits) - Worst is 31 bits: 1027/1023 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 80 - 0.107%

Testing bit 29
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    507 (0.99x) (-4)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 126291/125777 (1.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 132/127 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 126 - 0.088%

Testing bit 30
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    477 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    516 (1.01x) (5)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 516/511 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 16 - 0.084%

Testing bit 31
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    525 (1.03x) (14)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    509 (0.99x) (-2)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 144/127 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit  6 - 0.070%

Testing bit 32
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    499 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2072/2046 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 31 - 0.086%

Testing bit 33
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 147/127 (1.15x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    568 (1.11x) (57)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 79/63 (1.23x)
Testing distribution - Worst bias is the 18-bit window at bit 99 - 0.097%

Testing bit 34
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    495 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 257/255 (1.00x)
Testing distribution - Worst bias is the 18-bit window at bit 79 - 0.087%

Testing bit 35
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 269/255 (1.05x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 96 - 0.066%

Testing bit 36
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 77/63 (1.20x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    501 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2062/2046 (1.01x)
Testing distribution - Worst bias is the 18-bit window at bit 53 - 0.121%

Testing bit 37
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 121 - 0.093%

Testing bit 38
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1045/1023 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    556 (1.09x) (45)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 556/511 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 123 - 0.077%

Testing bit 39
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    470 (0.92x)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 83/63 (1.30x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 282/255 (1.10x)
Testing distribution - Worst bias is the 18-bit window at bit 27 - 0.081%

Testing bit 40
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    493 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 27 bits: 16404/16298 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    459 (0.90x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 26 - 0.095%

Testing bit 41
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    505 (0.99x) (-6)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 136/127 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    537 (1.05x) (26)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 270/255 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 31 - 0.100%

Testing bit 42
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    529 (1.03x) (18)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    477 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2094/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 99 - 0.076%

Testing bit 43
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    475 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 125780/125777 (1.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 276/255 (1.08x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.091%

Testing bit 44
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 30 bits: 2095/2046 (1.02x)
Testing distribution - Worst bias is the 18-bit window at bit 19 - 0.068%

Testing bit 45
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 271/255 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    558 (1.09x) (47)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 38/31 (1.19x)
Testing distribution - Worst bias is the 18-bit window at bit 117 - 0.078%

Testing bit 46
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1087/1023 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    533 (1.04x) (22)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 271/255 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 51 - 0.104%

Testing bit 47
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    491 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2085/2046 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    519 (1.01x) (8)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 82 - 0.085%

Testing bit 48
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    523 (1.02x) (12)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 78/63 (1.22x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    512 (1.00x) (1)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 139/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 73 - 0.081%

Testing bit 49
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2143/2046 (1.05x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    544 (1.06x) (33)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 74/63 (1.16x)
Testing distribution - Worst bias is the 18-bit window at bit 66 - 0.099%

Testing bit 50
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    494 (0.97x)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    483 (0.94x)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 66/63 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 119 - 0.103%

Testing bit 51
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    502 (0.98x) (-9)
Testing collisions (high 24-36 bits) - Worst is 35 bits: 75/63 (1.17x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    500 (0.98x)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 266/255 (1.04x)
Testing distribution - Worst bias is the 18-bit window at bit 78 - 0.090%

Testing bit 52
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    504 (0.98x) (-7)
Testing collisions (high 24-36 bits) - Worst is 30 bits: 2076/2046 (1.01x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    477 (0.93x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 36/31 (1.13x)
Testing distribution - Worst bias is the 18-bit window at bit 28 - 0.110%

Testing bit 53
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    517 (1.01x) (6)
Testing collisions (high 24-36 bits) - Worst is 34 bits: 138/127 (1.08x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 39/31 (1.22x)
Testing distribution - Worst bias is the 18-bit window at bit  7 - 0.073%

Testing bit 54
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 33/31 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 71 - 0.098%

Testing bit 55
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    527 (1.03x) (16)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 42/31 (1.31x)
Testing distribution - Worst bias is the 18-bit window at bit 87 - 0.087%

Testing bit 56
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    546 (1.07x) (35)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 45/31 (1.41x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    520 (1.02x) (9)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 41/31 (1.28x)
Testing distribution - Worst bias is the 18-bit window at bit 17 - 0.112%

Testing bit 57
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    475 (0.93x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 126113/125777 (1.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    539 (1.05x) (28)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 140/127 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 49 - 0.089%

Testing bit 58
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    557 (1.09x) (46)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 37/31 (1.16x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    528 (1.03x) (17)
Testing collisions (low  24-36 bits) - Worst is 33 bits: 269/255 (1.05x)
Testing distribution - Worst bias is the 18-bit window at bit 20 - 0.091%

Testing bit 59
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    518 (1.01x) (7)
Testing collisions (high 24-36 bits) - Worst is 31 bits: 1082/1023 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    541 (1.06x) (30)
Testing collisions (low  24-36 bits) - Worst is 34 bits: 146/127 (1.14x)
Testing distribution - Worst bias is the 18-bit window at bit 55 - 0.065%

Testing bit 60
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    508 (0.99x) (-3)
Testing collisions (high 24-36 bits) - Worst is 33 bits: 260/255 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    488 (0.95x)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 34/31 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit 15 - 0.083%

Testing bit 61
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    521 (1.02x) (10)
Testing collisions (high 24-36 bits) - Worst is 36 bits: 48/31 (1.50x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    526 (1.03x) (15)
Testing collisions (low  24-36 bits) - Worst is 32 bits: 526/511 (1.03x)
Testing distribution - Worst bias is the 18-bit window at bit 12 - 0.083%

Testing bit 62
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    492 (0.96x)
Testing collisions (high 24-36 bits) - Worst is 24 bits: 126044/125777 (1.00x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    530 (1.04x) (19)
Testing collisions (low  24-36 bits) - Worst is 36 bits: 35/31 (1.09x)
Testing distribution - Worst bias is the 18-bit window at bit 37 - 0.095%

Testing bit 63
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected        511.9, actual    543 (1.06x) (32)
Testing collisions (high 24-36 bits) - Worst is 32 bits: 543/511 (1.06x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected        511.9, actual    522 (1.02x) (11)
Testing collisions (low  24-36 bits) - Worst is 35 bits: 68/63 (1.06x)
Testing distribution - Worst bias is the 18-bit window at bit  8 - 0.077%


[[[ MomentChi2 Tests ]]]

Analyze hashes produced from a serie of linearly increasing numbers of 32-bit, using a step of 2 ... 
Target values to approximate : 38918200.000000 - 273633.333333 
Popcount 1 stats : 38918998.920034 - 273638.558723
Popcount 0 stats : 38918770.531038 - 273645.996303
MomentChi2 for bits 1 :   1.16628 
MomentChi2 for bits 0 :  0.594771 

Derivative stats (transition from 2 consecutive values) : 
Popcount 1 stats : 38919257.619283 - 273628.012809
Popcount 0 stats : 38918472.697479 - 273650.071385
MomentChi2 for deriv b1 :   2.04392 
MomentChi2 for deriv b0 :  0.135878 

  Great 


[[[ Prng Tests ]]]

Generating 33554432 random numbers : 
Testing collisions (128-bit) - Expected    0.0, actual      0 (0.00x)
Testing collisions (high 64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (high 32-bit) - Expected     130731.3, actual 131029 (1.00x) (298)
Testing collisions (high 28-44 bits) - Worst is 37 bits: 4158/4095 (1.02x)
Testing collisions (low  64-bit) - Expected          0.0, actual      0 (0.00x)
Testing collisions (low  32-bit) - Expected     130731.3, actual 130672 (1.00x) (-59)
Testing collisions (low  28-44 bits) - Worst is 43 bits: 85/63 (1.33x)

[[[ BadSeeds Tests ]]]

0x0 PASS


Input vcode 0x00000001, Output vcode 0x00000001, Result vcode 0x00000001
Verification value is 0x00000001 - Testing took 1063.513065 seconds
-------------------------------------------------------------------------------
```

-------------------------------------------------------------------------------
## Pseudo-Random Function Testing

Testing the Pseudo-Random Number Generator (PRNG) Functions isn't somewhat of a chore, as a complete test could take a week or so to complete and my laptop wasn't designed for such long running intensive tasks.

Tests were conducted by utilizing [PractRand](https://pracrand.sourceforge.net) as well as some tests adopted from the [xoshiro](http://xoshiro.di.unimi.it/hwd.php) testing code, running both up to 2TB of random data (which took about 1 night for PractRand).

Results are as follows:

-------------------------------------------------------------------------------
### `fio_rand`

The following are the tests for the core  `fio_rand64` and `fio_rand_bytes` functions provided when using `FIO_RAND`.

**The `PractRand` results**:

```txt
# ./tmp/rnd -p fio | RNG_test stdin
RNG_test using PractRand version 0.95
RNG = RNG_stdin, seed = unknown
test set = core, folding = standard(unknown format)

rng=RNG_stdin, seed=unknown
length= 256 megabytes (2^28 bytes), time= 3.0 seconds
  no anomalies in 217 test result(s)

rng=RNG_stdin, seed=unknown
length= 512 megabytes (2^29 bytes), time= 6.5 seconds
  no anomalies in 232 test result(s)

rng=RNG_stdin, seed=unknown
length= 1 gigabyte (2^30 bytes), time= 13.0 seconds
  no anomalies in 251 test result(s)

rng=RNG_stdin, seed=unknown
length= 2 gigabytes (2^31 bytes), time= 25.2 seconds
  no anomalies in 269 test result(s)

rng=RNG_stdin, seed=unknown
length= 4 gigabytes (2^32 bytes), time= 48.2 seconds
  no anomalies in 283 test result(s)

rng=RNG_stdin, seed=unknown
length= 8 gigabytes (2^33 bytes), time= 96.5 seconds
  Test Name                         Raw       Processed     Evaluation
  BCFN(2+7,13-2,T)                  R=  -9.1  p =1-3.9e-5   unusual          
  ...and 299 test result(s) without anomalies

rng=RNG_stdin, seed=unknown
length= 16 gigabytes (2^34 bytes), time= 190 seconds
  no anomalies in 315 test result(s)

rng=RNG_stdin, seed=unknown
length= 32 gigabytes (2^35 bytes), time= 376 seconds
  no anomalies in 328 test result(s)

rng=RNG_stdin, seed=unknown
length= 64 gigabytes (2^36 bytes), time= 752 seconds
  no anomalies in 344 test result(s)

rng=RNG_stdin, seed=unknown
length= 128 gigabytes (2^37 bytes), time= 1498 seconds
  no anomalies in 359 test result(s)

rng=RNG_stdin, seed=unknown
length= 256 gigabytes (2^38 bytes), time= 2978 seconds
  no anomalies in 372 test result(s)

rng=RNG_stdin, seed=unknown
length= 512 gigabytes (2^39 bytes), time= 9897 seconds
  no anomalies in 387 test result(s)

rng=RNG_stdin, seed=unknown
length= 1 terabyte (2^40 bytes), time= 21004 seconds
  no anomalies in 401 test result(s)

rng=RNG_stdin, seed=unknown
length= 2 terabytes (2^41 bytes), time= 43108 seconds
  no anomalies in 413 test result(s)
```

**The tests adopted from the `xoshiro` code base**:


```txt
# ./tmp/random fio
mix3 extreme = 1.80533 (sig = 00100000) weight 1 (16), p-value = 0.692
mix3 extreme = 2.35650 (sig = 00000102) weight 2 (112), p-value = 0.876
mix3 extreme = 3.33991 (sig = 20002100) weight 3 (448), p-value = 0.313
mix3 extreme = 4.30088 (sig = 02210200) weight 4 (1120), p-value = 0.0189
mix3 extreme = 4.03228 (sig = 01002212) weight >=5 (4864), p-value = 0.236
bits per word = 64 (analyzing bits); min category p-value = 0.0189

processed 1.11e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.0909
------

mix3 extreme = 1.84047 (sig = 20000000) weight 1 (16), p-value = 0.663
mix3 extreme = 3.12258 (sig = 00000102) weight 2 (112), p-value = 0.182
mix3 extreme = 3.47031 (sig = 20002100) weight 3 (448), p-value = 0.208
mix3 extreme = 3.61471 (sig = 12000101) weight 4 (1120), p-value = 0.286
mix3 extreme = 3.76213 (sig = 10111202) weight >=5 (4864), p-value = 0.559
bits per word = 64 (analyzing bits); min category p-value = 0.182

processed 1.29e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.634
------

mix3 extreme = 1.65805 (sig = 00000002) weight 1 (16), p-value = 0.806
mix3 extreme = 3.09464 (sig = 00000102) weight 2 (112), p-value = 0.198
mix3 extreme = 3.53210 (sig = 00102010) weight 3 (448), p-value = 0.169
mix3 extreme = 3.35900 (sig = 02210200) weight 4 (1120), p-value = 0.584
mix3 extreme = 3.58826 (sig = 02011101) weight >=5 (4864), p-value = 0.802
bits per word = 64 (analyzing bits); min category p-value = 0.169

processed 1.66e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.603
------

mix3 extreme = 1.75332 (sig = 00000001) weight 1 (16), p-value = 0.735
mix3 extreme = 2.97345 (sig = 00000102) weight 2 (112), p-value = 0.281
mix3 extreme = 3.21121 (sig = 20002100) weight 3 (448), p-value = 0.447
mix3 extreme = 2.99207 (sig = 10102020) weight 4 (1120), p-value = 0.955
mix3 extreme = 3.53616 (sig = 21120101) weight >=5 (4864), p-value = 0.861
bits per word = 64 (analyzing bits); min category p-value = 0.281

processed 1.85e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.808
------

mix3 extreme = 1.83225 (sig = 00000100) weight 1 (16), p-value = 0.67
mix3 extreme = 3.10258 (sig = 00000102) weight 2 (112), p-value = 0.194
mix3 extreme = 3.03291 (sig = 20002100) weight 3 (448), p-value = 0.663
mix3 extreme = 3.09549 (sig = 10200022) weight 4 (1120), p-value = 0.89
mix3 extreme = 3.76428 (sig = 02011101) weight >=5 (4864), p-value = 0.556
bits per word = 64 (analyzing bits); min category p-value = 0.194

processed 2.03e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.659
------

mix3 extreme = 1.84289 (sig = 00000100) weight 1 (16), p-value = 0.661
mix3 extreme = 3.57529 (sig = 00000102) weight 2 (112), p-value = 0.0384
mix3 extreme = 3.46502 (sig = 20002010) weight 3 (448), p-value = 0.211
mix3 extreme = 3.25764 (sig = 00220201) weight 4 (1120), p-value = 0.716
mix3 extreme = 4.19914 (sig = 11222212) weight >=5 (4864), p-value = 0.122
bits per word = 64 (analyzing bits); min category p-value = 0.0384

processed 2.59e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.178
------

mix3 extreme = 2.19203 (sig = 00000002) weight 1 (16), p-value = 0.369
mix3 extreme = 3.04970 (sig = 00000102) weight 2 (112), p-value = 0.227
mix3 extreme = 3.44778 (sig = 20002010) weight 3 (448), p-value = 0.224
mix3 extreme = 3.66866 (sig = 22110000) weight 4 (1120), p-value = 0.239
mix3 extreme = 3.58667 (sig = 02021021) weight >=5 (4864), p-value = 0.804
bits per word = 64 (analyzing bits); min category p-value = 0.224

processed 3.14e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.718
------

mix3 extreme = 2.51064 (sig = 00000002) weight 1 (16), p-value = 0.176
mix3 extreme = 2.57871 (sig = 20010000) weight 2 (112), p-value = 0.672
mix3 extreme = 3.85273 (sig = 20002010) weight 3 (448), p-value = 0.051
mix3 extreme = 3.18862 (sig = 01101010) weight 4 (1120), p-value = 0.799
mix3 extreme = 3.60146 (sig = 22121010) weight >=5 (4864), p-value = 0.785
bits per word = 64 (analyzing bits); min category p-value = 0.051

processed 4.07e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:21 2025

p = 0.23
------

mix3 extreme = 1.89834 (sig = 00000002) weight 1 (16), p-value = 0.613
mix3 extreme = 2.27677 (sig = 00100200) weight 2 (112), p-value = 0.924
mix3 extreme = 3.52766 (sig = 20000022) weight 3 (448), p-value = 0.171
mix3 extreme = 3.50133 (sig = 11000102) weight 4 (1120), p-value = 0.405
mix3 extreme = 3.47778 (sig = 11121011) weight >=5 (4864), p-value = 0.915
bits per word = 64 (analyzing bits); min category p-value = 0.171

processed 5.18e+08 bytes in 1 seconds (0.5177 GB/s, 1.864 TB/h). Fri Feb 21 23:22:22 2025

p = 0.609
------

mix3 extreme = 1.99814 (sig = 00000001) weight 1 (16), p-value = 0.527
mix3 extreme = 2.43362 (sig = 00100200) weight 2 (112), p-value = 0.815
mix3 extreme = 3.46309 (sig = 20002010) weight 3 (448), p-value = 0.213
mix3 extreme = 3.67295 (sig = 20110001) weight 4 (1120), p-value = 0.236
mix3 extreme = 3.50848 (sig = 00222210) weight >=5 (4864), p-value = 0.888
bits per word = 64 (analyzing bits); min category p-value = 0.213

processed 6.1e+08 bytes in 1 seconds (0.6101 GB/s, 2.196 TB/h). Fri Feb 21 23:22:22 2025

p = 0.698
------

mix3 extreme = 1.78022 (sig = 00000002) weight 1 (16), p-value = 0.713
mix3 extreme = 2.64570 (sig = 10000001) weight 2 (112), p-value = 0.6
mix3 extreme = 3.55344 (sig = 20002010) weight 3 (448), p-value = 0.157
mix3 extreme = 3.15383 (sig = 11000102) weight 4 (1120), p-value = 0.836
mix3 extreme = 4.03858 (sig = 11121011) weight >=5 (4864), p-value = 0.23
bits per word = 64 (analyzing bits); min category p-value = 0.157

processed 7.03e+08 bytes in 1 seconds (0.7026 GB/s, 2.529 TB/h). Fri Feb 21 23:22:22 2025

p = 0.573
------

mix3 extreme = 1.83987 (sig = 00000002) weight 1 (16), p-value = 0.663
mix3 extreme = 2.43194 (sig = 00000110) weight 2 (112), p-value = 0.816
mix3 extreme = 3.40962 (sig = 20000022) weight 3 (448), p-value = 0.253
mix3 extreme = 3.02964 (sig = 02012200) weight 4 (1120), p-value = 0.936
mix3 extreme = 3.75207 (sig = 20222201) weight >=5 (4864), p-value = 0.574
bits per word = 64 (analyzing bits); min category p-value = 0.253

processed 8.5e+08 bytes in 1 seconds (0.8505 GB/s, 3.062 TB/h). Fri Feb 21 23:22:22 2025

p = 0.767
------

mix3 extreme = 2.03491 (sig = 00000002) weight 1 (16), p-value = 0.496
mix3 extreme = 2.48334 (sig = 00001002) weight 2 (112), p-value = 0.769
mix3 extreme = 3.15042 (sig = 10200002) weight 3 (448), p-value = 0.519
mix3 extreme = 3.19215 (sig = 21200001) weight 4 (1120), p-value = 0.795
mix3 extreme = 3.68390 (sig = 20222201) weight >=5 (4864), p-value = 0.673
bits per word = 64 (analyzing bits); min category p-value = 0.496

processed 1.02e+09 bytes in 1 seconds (1.017 GB/s, 3.661 TB/h). Fri Feb 21 23:22:22 2025

p = 0.967
------

mix3 extreme = 1.98184 (sig = 00000001) weight 1 (16), p-value = 0.541
mix3 extreme = 2.49397 (sig = 01001000) weight 2 (112), p-value = 0.759
mix3 extreme = 2.98386 (sig = 02000011) weight 3 (448), p-value = 0.721
mix3 extreme = 3.29022 (sig = 20200220) weight 4 (1120), p-value = 0.674
mix3 extreme = 4.30382 (sig = 11122011) weight >=5 (4864), p-value = 0.0784
bits per word = 64 (analyzing bits); min category p-value = 0.0784

processed 1.26e+09 bytes in 1 seconds (1.257 GB/s, 4.526 TB/h). Fri Feb 21 23:22:22 2025

p = 0.335
------

mix3 extreme = 2.33599 (sig = 00000001) weight 1 (16), p-value = 0.27
mix3 extreme = 2.45643 (sig = 00000110) weight 2 (112), p-value = 0.795
mix3 extreme = 2.84156 (sig = 00022100) weight 3 (448), p-value = 0.867
mix3 extreme = 3.61210 (sig = 01100202) weight 4 (1120), p-value = 0.288
mix3 extreme = 3.51573 (sig = 21101210) weight >=5 (4864), p-value = 0.882
bits per word = 64 (analyzing bits); min category p-value = 0.27

processed 1.52e+09 bytes in 1 seconds (1.516 GB/s, 5.458 TB/h). Fri Feb 21 23:22:22 2025

p = 0.793
------

mix3 extreme = 2.57590 (sig = 00000001) weight 1 (16), p-value = 0.149
mix3 extreme = 2.18056 (sig = 10000002) weight 2 (112), p-value = 0.964
mix3 extreme = 2.94943 (sig = 20102000) weight 3 (448), p-value = 0.76
mix3 extreme = 3.39387 (sig = 11010002) weight 4 (1120), p-value = 0.538
mix3 extreme = 3.85744 (sig = 22210212) weight >=5 (4864), p-value = 0.427
bits per word = 64 (analyzing bits); min category p-value = 0.149

processed 1.76e+09 bytes in 1 seconds (1.756 GB/s, 6.323 TB/h). Fri Feb 21 23:22:22 2025

p = 0.552
------

mix3 extreme = 2.64879 (sig = 00000001) weight 1 (16), p-value = 0.122
mix3 extreme = 2.69778 (sig = 10000002) weight 2 (112), p-value = 0.544
mix3 extreme = 3.15759 (sig = 20102000) weight 3 (448), p-value = 0.51
mix3 extreme = 3.41194 (sig = 11010002) weight 4 (1120), p-value = 0.515
mix3 extreme = 4.19052 (sig = 11122011) weight >=5 (4864), p-value = 0.127
bits per word = 64 (analyzing bits); min category p-value = 0.122

processed 2.02e+09 bytes in 1 seconds (2.015 GB/s, 7.255 TB/h). Fri Feb 21 23:22:22 2025

p = 0.477
------

mix3 extreme = 2.10150 (sig = 00000001) weight 1 (16), p-value = 0.44
mix3 extreme = 2.31937 (sig = 00100100) weight 2 (112), p-value = 0.9
mix3 extreme = 2.98427 (sig = 20102000) weight 3 (448), p-value = 0.721
mix3 extreme = 3.62790 (sig = 21200001) weight 4 (1120), p-value = 0.274
mix3 extreme = 3.47379 (sig = 22011100) weight >=5 (4864), p-value = 0.918
bits per word = 64 (analyzing bits); min category p-value = 0.274

processed 2.51e+09 bytes in 1 seconds (2.514 GB/s, 9.052 TB/h). Fri Feb 21 23:22:22 2025

p = 0.798
------

mix3 extreme = 2.20184 (sig = 00100000) weight 1 (16), p-value = 0.362
mix3 extreme = 2.44576 (sig = 00100100) weight 2 (112), p-value = 0.804
mix3 extreme = 2.66267 (sig = 10200002) weight 3 (448), p-value = 0.969
mix3 extreme = 3.76139 (sig = 21200001) weight 4 (1120), p-value = 0.172
mix3 extreme = 3.82048 (sig = 20222201) weight >=5 (4864), p-value = 0.477
bits per word = 64 (analyzing bits); min category p-value = 0.172

processed 3.01e+09 bytes in 2 seconds (1.507 GB/s, 5.425 TB/h). Fri Feb 21 23:22:23 2025

p = 0.612
------

mix3 extreme = 2.18426 (sig = 00100000) weight 1 (16), p-value = 0.375
mix3 extreme = 2.65062 (sig = 20002000) weight 2 (112), p-value = 0.595
mix3 extreme = 2.92917 (sig = 01100200) weight 3 (448), p-value = 0.782
mix3 extreme = 3.89526 (sig = 21200001) weight 4 (1120), p-value = 0.104
mix3 extreme = 3.78679 (sig = 20220022) weight >=5 (4864), p-value = 0.524
bits per word = 64 (analyzing bits); min category p-value = 0.104

processed 4.01e+09 bytes in 2 seconds (2.006 GB/s, 7.222 TB/h). Fri Feb 21 23:22:23 2025

p = 0.423
------

mix3 extreme = 2.46216 (sig = 20000000) weight 1 (16), p-value = 0.199
mix3 extreme = 2.30564 (sig = 20002000) weight 2 (112), p-value = 0.909
mix3 extreme = 2.97563 (sig = 00100220) weight 3 (448), p-value = 0.731
mix3 extreme = 3.70435 (sig = 20110001) weight 4 (1120), p-value = 0.211
mix3 extreme = 3.47865 (sig = 22100011) weight >=5 (4864), p-value = 0.914
bits per word = 64 (analyzing bits); min category p-value = 0.199

processed 5.01e+09 bytes in 2 seconds (2.505 GB/s, 9.019 TB/h). Fri Feb 21 23:22:23 2025

p = 0.671
------

mix3 extreme = 2.48402 (sig = 00100000) weight 1 (16), p-value = 0.189
mix3 extreme = 2.32620 (sig = 00000012) weight 2 (112), p-value = 0.896
mix3 extreme = 3.13986 (sig = 00120200) weight 3 (448), p-value = 0.531
mix3 extreme = 3.56935 (sig = 20110001) weight 4 (1120), p-value = 0.33
mix3 extreme = 3.62670 (sig = 01210201) weight >=5 (4864), p-value = 0.753
bits per word = 64 (analyzing bits); min category p-value = 0.189

processed 6.01e+09 bytes in 3 seconds (2.003 GB/s, 7.211 TB/h). Fri Feb 21 23:22:24 2025

p = 0.649
------

mix3 extreme = 2.07995 (sig = 00100000) weight 1 (16), p-value = 0.458
mix3 extreme = 2.14438 (sig = 00002010) weight 2 (112), p-value = 0.974
mix3 extreme = 3.12213 (sig = 00120200) weight 3 (448), p-value = 0.553
mix3 extreme = 3.59330 (sig = 20110001) weight 4 (1120), p-value = 0.306
mix3 extreme = 3.93490 (sig = 01210201) weight >=5 (4864), p-value = 0.333
bits per word = 64 (analyzing bits); min category p-value = 0.306

processed 7.01e+09 bytes in 3 seconds (2.336 GB/s, 8.409 TB/h). Fri Feb 21 23:22:24 2025

p = 0.839
------

mix3 extreme = 1.82939 (sig = 20000000) weight 1 (16), p-value = 0.672
mix3 extreme = 2.04212 (sig = 00002010) weight 2 (112), p-value = 0.991
mix3 extreme = 3.40400 (sig = 00120200) weight 3 (448), p-value = 0.257
mix3 extreme = 3.20336 (sig = 12020002) weight 4 (1120), p-value = 0.782
mix3 extreme = 4.26991 (sig = 01210201) weight >=5 (4864), p-value = 0.0907
bits per word = 64 (analyzing bits); min category p-value = 0.0907

processed 8.5e+09 bytes in 4 seconds (2.126 GB/s, 7.654 TB/h). Fri Feb 21 23:22:25 2025

p = 0.378
------

mix3 extreme = 1.82510 (sig = 00100000) weight 1 (16), p-value = 0.676
mix3 extreme = 2.69933 (sig = 00002020) weight 2 (112), p-value = 0.542
mix3 extreme = 2.78787 (sig = 02020200) weight 3 (448), p-value = 0.908
mix3 extreme = 3.06049 (sig = 00001122) weight 4 (1120), p-value = 0.916
mix3 extreme = 4.40242 (sig = 01210201) weight >=5 (4864), p-value = 0.0507
bits per word = 64 (analyzing bits); min category p-value = 0.0507

processed 1e+10 bytes in 4 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:25 2025

p = 0.229
------

mix3 extreme = 2.20665 (sig = 00100000) weight 1 (16), p-value = 0.358
mix3 extreme = 2.66682 (sig = 00002020) weight 2 (112), p-value = 0.577
mix3 extreme = 3.18173 (sig = 02200010) weight 3 (448), p-value = 0.481
mix3 extreme = 3.39020 (sig = 01202100) weight 4 (1120), p-value = 0.543
mix3 extreme = 3.99569 (sig = 01210201) weight >=5 (4864), p-value = 0.269
bits per word = 64 (analyzing bits); min category p-value = 0.269

processed 1.25e+10 bytes in 5 seconds (2.503 GB/s, 9.012 TB/h). Fri Feb 21 23:22:26 2025

p = 0.792
------

mix3 extreme = 1.60923 (sig = 00000002) weight 1 (16), p-value = 0.838
mix3 extreme = 2.73755 (sig = 02000001) weight 2 (112), p-value = 0.501
mix3 extreme = 3.23640 (sig = 00101200) weight 3 (448), p-value = 0.419
mix3 extreme = 3.31439 (sig = 21220000) weight 4 (1120), p-value = 0.643
mix3 extreme = 3.57996 (sig = 21001101) weight >=5 (4864), p-value = 0.812
bits per word = 64 (analyzing bits); min category p-value = 0.419

processed 1.5e+10 bytes in 6 seconds (2.502 GB/s, 9.008 TB/h). Fri Feb 21 23:22:27 2025

p = 0.934
------

mix3 extreme = 1.91785 (sig = 00000002) weight 1 (16), p-value = 0.596
mix3 extreme = 2.60874 (sig = 02000001) weight 2 (112), p-value = 0.64
mix3 extreme = 3.38156 (sig = 00101200) weight 3 (448), p-value = 0.276
mix3 extreme = 3.24044 (sig = 21220000) weight 4 (1120), p-value = 0.737
mix3 extreme = 3.69590 (sig = 12211201) weight >=5 (4864), p-value = 0.656
bits per word = 64 (analyzing bits); min category p-value = 0.276

processed 1.75e+10 bytes in 7 seconds (2.501 GB/s, 9.004 TB/h). Fri Feb 21 23:22:28 2025

p = 0.801
------

mix3 extreme = 1.80532 (sig = 00000001) weight 1 (16), p-value = 0.692
mix3 extreme = 2.54416 (sig = 00020010) weight 2 (112), p-value = 0.709
mix3 extreme = 3.15400 (sig = 00101200) weight 3 (448), p-value = 0.514
mix3 extreme = 3.33906 (sig = 21220000) weight 4 (1120), p-value = 0.61
mix3 extreme = 3.56547 (sig = 00012112) weight >=5 (4864), p-value = 0.829
bits per word = 64 (analyzing bits); min category p-value = 0.514

processed 2e+10 bytes in 8 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:29 2025

p = 0.973
------

mix3 extreme = 1.94039 (sig = 00000002) weight 1 (16), p-value = 0.577
mix3 extreme = 2.27077 (sig = 01010000) weight 2 (112), p-value = 0.928
mix3 extreme = 3.19342 (sig = 00120200) weight 3 (448), p-value = 0.468
mix3 extreme = 3.51655 (sig = 21220000) weight 4 (1120), p-value = 0.387
mix3 extreme = 3.56607 (sig = 20122012) weight >=5 (4864), p-value = 0.828
bits per word = 64 (analyzing bits); min category p-value = 0.387

processed 2.5e+10 bytes in 10 seconds (2.502 GB/s, 9.005 TB/h). Fri Feb 21 23:22:31 2025

p = 0.914
------

mix3 extreme = 1.89200 (sig = 01000000) weight 1 (16), p-value = 0.619
mix3 extreme = 2.24801 (sig = 01010000) weight 2 (112), p-value = 0.938
mix3 extreme = 2.88784 (sig = 02000210) weight 3 (448), p-value = 0.825
mix3 extreme = 3.34758 (sig = 21220000) weight 4 (1120), p-value = 0.599
mix3 extreme = 3.45248 (sig = 11200111) weight >=5 (4864), p-value = 0.933
bits per word = 64 (analyzing bits); min category p-value = 0.599

processed 3e+10 bytes in 12 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:33 2025

p = 0.99
------

mix3 extreme = 1.56396 (sig = 00000200) weight 1 (16), p-value = 0.865
mix3 extreme = 1.97760 (sig = 00000022) weight 2 (112), p-value = 0.996
mix3 extreme = 3.23027 (sig = 20021000) weight 3 (448), p-value = 0.426
mix3 extreme = 3.36856 (sig = 21021000) weight 4 (1120), p-value = 0.571
mix3 extreme = 4.27622 (sig = 20122012) weight >=5 (4864), p-value = 0.0883
bits per word = 64 (analyzing bits); min category p-value = 0.0883

processed 4e+10 bytes in 16 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:37 2025

p = 0.37
------

mix3 extreme = 1.89666 (sig = 00000100) weight 1 (16), p-value = 0.615
mix3 extreme = 2.70819 (sig = 20000010) weight 2 (112), p-value = 0.532
mix3 extreme = 3.11747 (sig = 20021000) weight 3 (448), p-value = 0.559
mix3 extreme = 2.83492 (sig = 21021000) weight 4 (1120), p-value = 0.994
mix3 extreme = 3.73280 (sig = 20122012) weight >=5 (4864), p-value = 0.602
bits per word = 64 (analyzing bits); min category p-value = 0.532

processed 5e+10 bytes in 20 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:41 2025

p = 0.978
------

mix3 extreme = 1.60871 (sig = 00000001) weight 1 (16), p-value = 0.838
mix3 extreme = 2.92820 (sig = 20000010) weight 2 (112), p-value = 0.318
mix3 extreme = 3.30722 (sig = 20021000) weight 3 (448), p-value = 0.344
mix3 extreme = 3.12534 (sig = 12100002) weight 4 (1120), p-value = 0.863
mix3 extreme = 3.90592 (sig = 21101011) weight >=5 (4864), p-value = 0.367
bits per word = 64 (analyzing bits); min category p-value = 0.318

processed 6e+10 bytes in 24 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:45 2025

p = 0.852
------

mix3 extreme = 1.48010 (sig = 02000000) weight 1 (16), p-value = 0.909
mix3 extreme = 2.48996 (sig = 20000010) weight 2 (112), p-value = 0.763
mix3 extreme = 3.17803 (sig = 20021000) weight 3 (448), p-value = 0.486
mix3 extreme = 3.17420 (sig = 20101010) weight 4 (1120), p-value = 0.814
mix3 extreme = 3.79968 (sig = 21101011) weight >=5 (4864), p-value = 0.506
bits per word = 64 (analyzing bits); min category p-value = 0.486

processed 7e+10 bytes in 28 seconds (2.501 GB/s, 9.002 TB/h). Fri Feb 21 23:22:49 2025

p = 0.964
------

mix3 extreme = 2.31418 (sig = 02000000) weight 1 (16), p-value = 0.284
mix3 extreme = 2.75073 (sig = 02000001) weight 2 (112), p-value = 0.487
mix3 extreme = 3.22184 (sig = 20021000) weight 3 (448), p-value = 0.435
mix3 extreme = 2.99913 (sig = 12100002) weight 4 (1120), p-value = 0.952
mix3 extreme = 4.17272 (sig = 21101011) weight >=5 (4864), p-value = 0.136
bits per word = 64 (analyzing bits); min category p-value = 0.136

processed 8.5e+10 bytes in 35 seconds (2.429 GB/s, 8.744 TB/h). Fri Feb 21 23:22:56 2025

p = 0.519
------

mix3 extreme = 2.96167 (sig = 02000000) weight 1 (16), p-value = 0.0478
mix3 extreme = 2.15955 (sig = 02000001) weight 2 (112), p-value = 0.97
mix3 extreme = 3.37730 (sig = 20021000) weight 3 (448), p-value = 0.28
mix3 extreme = 3.52700 (sig = 12100002) weight 4 (1120), p-value = 0.376
mix3 extreme = 4.04580 (sig = 21101011) weight >=5 (4864), p-value = 0.224
bits per word = 64 (analyzing bits); min category p-value = 0.0478

processed 1e+11 bytes in 41 seconds (2.439 GB/s, 8.781 TB/h). Fri Feb 21 23:23:02 2025

p = 0.217
------

mix3 extreme = 2.47196 (sig = 02000000) weight 1 (16), p-value = 0.195
mix3 extreme = 2.51373 (sig = 02000001) weight 2 (112), p-value = 0.74
mix3 extreme = 3.39366 (sig = 20021000) weight 3 (448), p-value = 0.266
mix3 extreme = 3.21456 (sig = 12100002) weight 4 (1120), p-value = 0.769
mix3 extreme = 3.77086 (sig = 12221021) weight >=5 (4864), p-value = 0.547
bits per word = 64 (analyzing bits); min category p-value = 0.195

processed 1.25e+11 bytes in 51 seconds (2.451 GB/s, 8.824 TB/h). Fri Feb 21 23:23:12 2025

p = 0.661
------

mix3 extreme = 2.48207 (sig = 02000000) weight 1 (16), p-value = 0.19
mix3 extreme = 2.29109 (sig = 20020000) weight 2 (112), p-value = 0.917
mix3 extreme = 3.01591 (sig = 20021000) weight 3 (448), p-value = 0.683
mix3 extreme = 3.36633 (sig = 20200101) weight 4 (1120), p-value = 0.574
mix3 extreme = 3.40749 (sig = 02222012) weight >=5 (4864), p-value = 0.959
bits per word = 64 (analyzing bits); min category p-value = 0.19

processed 1.5e+11 bytes in 61 seconds (2.459 GB/s, 8.853 TB/h). Fri Feb 21 23:23:22 2025

p = 0.651
------

mix3 extreme = 2.14788 (sig = 02000000) weight 1 (16), p-value = 0.403
mix3 extreme = 2.23188 (sig = 01000002) weight 2 (112), p-value = 0.945
mix3 extreme = 3.01619 (sig = 20100002) weight 3 (448), p-value = 0.683
mix3 extreme = 3.35531 (sig = 22020010) weight 4 (1120), p-value = 0.589
mix3 extreme = 3.90091 (sig = 20120120) weight >=5 (4864), p-value = 0.373
bits per word = 64 (analyzing bits); min category p-value = 0.373

processed 1.75e+11 bytes in 71 seconds (2.465 GB/s, 8.874 TB/h). Fri Feb 21 23:23:32 2025

p = 0.903
------

mix3 extreme = 2.56990 (sig = 00010000) weight 1 (16), p-value = 0.151
mix3 extreme = 2.15879 (sig = 20020000) weight 2 (112), p-value = 0.97
mix3 extreme = 3.11637 (sig = 20100002) weight 3 (448), p-value = 0.56
mix3 extreme = 3.75031 (sig = 20200101) weight 4 (1120), p-value = 0.179
mix3 extreme = 3.82089 (sig = 11022021) weight >=5 (4864), p-value = 0.476
bits per word = 64 (analyzing bits); min category p-value = 0.151

processed 2e+11 bytes in 81 seconds (2.469 GB/s, 8.889 TB/h). Fri Feb 21 23:23:42 2025

p = 0.559
------

mix3 extreme = 2.68286 (sig = 00010000) weight 1 (16), p-value = 0.111
mix3 extreme = 2.03612 (sig = 01000002) weight 2 (112), p-value = 0.992
mix3 extreme = 2.72508 (sig = 01110000) weight 3 (448), p-value = 0.944
mix3 extreme = 3.29533 (sig = 20200101) weight 4 (1120), p-value = 0.668
mix3 extreme = 3.94062 (sig = 11101011) weight >=5 (4864), p-value = 0.327
bits per word = 64 (analyzing bits); min category p-value = 0.111

processed 2.5e+11 bytes in 102 seconds (2.451 GB/s, 8.824 TB/h). Fri Feb 21 23:24:03 2025

p = 0.444
------

mix3 extreme = 3.02806 (sig = 00010000) weight 1 (16), p-value = 0.0387
mix3 extreme = 2.64241 (sig = 12000000) weight 2 (112), p-value = 0.604
mix3 extreme = 3.14107 (sig = 12001000) weight 3 (448), p-value = 0.53
mix3 extreme = 3.10292 (sig = 10202002) weight 4 (1120), p-value = 0.883
mix3 extreme = 3.78800 (sig = 11122221) weight >=5 (4864), p-value = 0.522
bits per word = 64 (analyzing bits); min category p-value = 0.0387

processed 3e+11 bytes in 122 seconds (2.459 GB/s, 8.853 TB/h). Fri Feb 21 23:24:23 2025

p = 0.179
------

mix3 extreme = 2.11373 (sig = 00010000) weight 1 (16), p-value = 0.43
mix3 extreme = 2.53673 (sig = 00010020) weight 2 (112), p-value = 0.716
mix3 extreme = 3.82566 (sig = 01000220) weight 3 (448), p-value = 0.0568
mix3 extreme = 3.94266 (sig = 22020010) weight 4 (1120), p-value = 0.0863
mix3 extreme = 3.38864 (sig = 02212120) weight >=5 (4864), p-value = 0.967
bits per word = 64 (analyzing bits); min category p-value = 0.0568

processed 4e+11 bytes in 163 seconds (2.454 GB/s, 8.834 TB/h). Fri Feb 21 23:25:04 2025

p = 0.253
------

mix3 extreme = 2.02087 (sig = 02000000) weight 1 (16), p-value = 0.507
mix3 extreme = 3.38282 (sig = 00010020) weight 2 (112), p-value = 0.0772
mix3 extreme = 3.77275 (sig = 01000220) weight 3 (448), p-value = 0.0698
mix3 extreme = 3.96045 (sig = 22020010) weight 4 (1120), p-value = 0.0804
mix3 extreme = 3.96698 (sig = 10212100) weight >=5 (4864), p-value = 0.298
bits per word = 64 (analyzing bits); min category p-value = 0.0698

processed 5e+11 bytes in 204 seconds (2.451 GB/s, 8.824 TB/h). Fri Feb 21 23:25:45 2025

p = 0.304
------

mix3 extreme = 2.11460 (sig = 02000000) weight 1 (16), p-value = 0.429
mix3 extreme = 3.45081 (sig = 00010020) weight 2 (112), p-value = 0.0607
mix3 extreme = 4.11176 (sig = 01000220) weight 3 (448), p-value = 0.0174
mix3 extreme = 3.90641 (sig = 22020010) weight 4 (1120), p-value = 0.0996
mix3 extreme = 3.86051 (sig = 10110212) weight >=5 (4864), p-value = 0.423
bits per word = 64 (analyzing bits); min category p-value = 0.0174

processed 6e+11 bytes in 244 seconds (2.459 GB/s, 8.853 TB/h). Fri Feb 21 23:26:25 2025

p = 0.0842
------

mix3 extreme = 2.19471 (sig = 02000000) weight 1 (16), p-value = 0.367
mix3 extreme = 3.42206 (sig = 00010020) weight 2 (112), p-value = 0.0673
mix3 extreme = 3.76615 (sig = 01000220) weight 3 (448), p-value = 0.0716
mix3 extreme = 3.59051 (sig = 22020010) weight 4 (1120), p-value = 0.309
mix3 extreme = 3.55018 (sig = 20012221) weight >=5 (4864), p-value = 0.846
bits per word = 64 (analyzing bits); min category p-value = 0.0673

processed 7e+11 bytes in 285 seconds (2.456 GB/s, 8.842 TB/h). Fri Feb 21 23:27:06 2025

p = 0.294
------

mix3 extreme = 2.08568 (sig = 02000000) weight 1 (16), p-value = 0.453
mix3 extreme = 2.70372 (sig = 00010020) weight 2 (112), p-value = 0.537
mix3 extreme = 3.67441 (sig = 20021000) weight 3 (448), p-value = 0.101
mix3 extreme = 3.40600 (sig = 22020010) weight 4 (1120), p-value = 0.522
mix3 extreme = 3.74169 (sig = 10110212) weight >=5 (4864), p-value = 0.589
bits per word = 64 (analyzing bits); min category p-value = 0.101

processed 8.5e+11 bytes in 346 seconds (2.457 GB/s, 8.844 TB/h). Fri Feb 21 23:28:07 2025

p = 0.414
------

mix3 extreme = 1.70943 (sig = 00000100) weight 1 (16), p-value = 0.768
mix3 extreme = 3.16917 (sig = 00010020) weight 2 (112), p-value = 0.157
mix3 extreme = 3.64671 (sig = 02000201) weight 3 (448), p-value = 0.112
mix3 extreme = 3.82262 (sig = 11010200) weight 4 (1120), p-value = 0.137
mix3 extreme = 3.64758 (sig = 00222021) weight >=5 (4864), p-value = 0.724
bits per word = 64 (analyzing bits); min category p-value = 0.112

processed 1e+12 bytes in 407 seconds (2.457 GB/s, 8.845 TB/h). Fri Feb 21 23:29:08 2025

p = 0.448
------

mix3 extreme = 1.76773 (sig = 20000000) weight 1 (16), p-value = 0.723
mix3 extreme = 3.14942 (sig = 00002020) weight 2 (112), p-value = 0.168
mix3 extreme = 3.45782 (sig = 02000201) weight 3 (448), p-value = 0.217
mix3 extreme = 3.68367 (sig = 11010200) weight 4 (1120), p-value = 0.227
mix3 extreme = 4.05225 (sig = 00222021) weight >=5 (4864), p-value = 0.219
bits per word = 64 (analyzing bits); min category p-value = 0.168

processed 1.25e+12 bytes in 509 seconds (2.456 GB/s, 8.841 TB/h). Fri Feb 21 23:30:50 2025

p = 0.6
------

mix3 extreme = 1.78446 (sig = 00000001) weight 1 (16), p-value = 0.709
mix3 extreme = 3.00030 (sig = 00002020) weight 2 (112), p-value = 0.261
mix3 extreme = 3.68239 (sig = 02000201) weight 3 (448), p-value = 0.0983
mix3 extreme = 3.38984 (sig = 11010020) weight 4 (1120), p-value = 0.543
mix3 extreme = 4.00734 (sig = 02221002) weight >=5 (4864), p-value = 0.258
bits per word = 64 (analyzing bits); min category p-value = 0.0983

processed 1.5e+12 bytes in 611 seconds (2.455 GB/s, 8.838 TB/h). Fri Feb 21 23:32:32 2025

p = 0.404
------

mix3 extreme = 1.49970 (sig = 02000000) weight 1 (16), p-value = 0.899
mix3 extreme = 2.64430 (sig = 20000020) weight 2 (112), p-value = 0.602
mix3 extreme = 3.85954 (sig = 02000201) weight 3 (448), p-value = 0.0496
mix3 extreme = 3.76326 (sig = 11010020) weight 4 (1120), p-value = 0.171
mix3 extreme = 3.91001 (sig = 00222021) weight >=5 (4864), p-value = 0.362
bits per word = 64 (analyzing bits); min category p-value = 0.0496

processed 1.75e+12 bytes in 713 seconds (2.454 GB/s, 8.836 TB/h). Fri Feb 21 23:34:14 2025

p = 0.225
------

mix3 extreme = 1.64313 (sig = 02000000) weight 1 (16), p-value = 0.816
mix3 extreme = 3.02382 (sig = 02100000) weight 2 (112), p-value = 0.244
mix3 extreme = 3.21441 (sig = 02000201) weight 3 (448), p-value = 0.443
mix3 extreme = 4.07811 (sig = 11010020) weight 4 (1120), p-value = 0.0496
mix3 extreme = 4.50737 (sig = 00222021) weight >=5 (4864), p-value = 0.0314
bits per word = 64 (analyzing bits); min category p-value = 0.0314

processed 2e+12 bytes in 815 seconds (2.454 GB/s, 8.834 TB/h). Fri Feb 21 23:35:56 2025

p = 0.148
------

mix3 extreme = 1.57884 (sig = 20000000) weight 1 (16), p-value = 0.857
mix3 extreme = 2.68908 (sig = 00001002) weight 2 (112), p-value = 0.553
mix3 extreme = 3.22275 (sig = 01000220) weight 3 (448), p-value = 0.434
mix3 extreme = 3.84110 (sig = 11010020) weight 4 (1120), p-value = 0.128
mix3 extreme = 3.89955 (sig = 10020112) weight >=5 (4864), p-value = 0.374
bits per word = 64 (analyzing bits); min category p-value = 0.128

processed 2.5e+12 bytes in 1.02e+03 seconds (2.456 GB/s, 8.841 TB/h). Fri Feb 21 23:39:19 2025

p = 0.496
------

mix3 extreme = 1.67736 (sig = 00001000) weight 1 (16), p-value = 0.792
mix3 extreme = 2.62488 (sig = 00001002) weight 2 (112), p-value = 0.623
mix3 extreme = 3.38419 (sig = 02002002) weight 3 (448), p-value = 0.274
mix3 extreme = 3.80757 (sig = 11010020) weight 4 (1120), p-value = 0.145
mix3 extreme = 3.95015 (sig = 00211221) weight >=5 (4864), p-value = 0.316
bits per word = 64 (analyzing bits); min category p-value = 0.145

processed 3e+12 bytes in 1.22e+03 seconds (2.451 GB/s, 8.824 TB/h). Fri Feb 21 23:42:45 2025

p = 0.544
------

mix3 extreme = 1.79534 (sig = 00002000) weight 1 (16), p-value = 0.701
mix3 extreme = 2.76965 (sig = 00001002) weight 2 (112), p-value = 0.468
mix3 extreme = 3.58785 (sig = 01000220) weight 3 (448), p-value = 0.139
mix3 extreme = 3.25341 (sig = 20200012) weight 4 (1120), p-value = 0.721
mix3 extreme = 3.65576 (sig = 12111122) weight >=5 (4864), p-value = 0.713
bits per word = 64 (analyzing bits); min category p-value = 0.139

processed 4e+12 bytes in 1.63e+03 seconds (2.448 GB/s, 8.813 TB/h). Fri Feb 21 23:49:35 2025

p = 0.526
------
```

-------------------------------------------------------------------------------
### `FIO_DEFINE_RANDOM128_FN`

The following are the tests for the built-in `FIO_DEFINE_RANDOM128_FN` macro using the deterministic PRNG (where the auto-reseeding `reseed_log` is set to `0`).

**Note**: setting `reseed_log` to any practical value (such as `31`) would improve randomness make the PRNG non-deterministic, improving the security of the result.

**The `PractRand` results**:

```txt
# ./tmp/rnd -p M128 | RNG_test stdin
RNG_test using PractRand version 0.95
RNG = RNG_stdin, seed = unknown
test set = core, folding = standard(unknown format)

rng=RNG_stdin, seed=unknown
length= 256 megabytes (2^28 bytes), time= 2.4 seconds
  no anomalies in 217 test result(s)

rng=RNG_stdin, seed=unknown
length= 512 megabytes (2^29 bytes), time= 5.0 seconds
  no anomalies in 232 test result(s)

rng=RNG_stdin, seed=unknown
length= 1 gigabyte (2^30 bytes), time= 10.1 seconds
  no anomalies in 251 test result(s)

rng=RNG_stdin, seed=unknown
length= 2 gigabytes (2^31 bytes), time= 20.6 seconds
  no anomalies in 269 test result(s)

rng=RNG_stdin, seed=unknown
length= 4 gigabytes (2^32 bytes), time= 41.2 seconds
  no anomalies in 283 test result(s)

rng=RNG_stdin, seed=unknown
length= 8 gigabytes (2^33 bytes), time= 83.9 seconds
  no anomalies in 300 test result(s)

rng=RNG_stdin, seed=unknown
length= 16 gigabytes (2^34 bytes), time= 167 seconds
  no anomalies in 315 test result(s)

rng=RNG_stdin, seed=unknown
length= 32 gigabytes (2^35 bytes), time= 330 seconds
  no anomalies in 328 test result(s)

rng=RNG_stdin, seed=unknown
length= 64 gigabytes (2^36 bytes), time= 665 seconds
  no anomalies in 344 test result(s)

rng=RNG_stdin, seed=unknown
length= 128 gigabytes (2^37 bytes), time= 1329 seconds
  no anomalies in 359 test result(s)

rng=RNG_stdin, seed=unknown
length= 256 gigabytes (2^38 bytes), time= 2630 seconds
  no anomalies in 372 test result(s)

rng=RNG_stdin, seed=unknown
length= 512 gigabytes (2^39 bytes), time= 6846 seconds
  no anomalies in 387 test result(s)

rng=RNG_stdin, seed=unknown
length= 1 terabyte (2^40 bytes), time= 18633 seconds
  no anomalies in 401 test result(s)

rng=RNG_stdin, seed=unknown
length= 2 terabytes (2^41 bytes), time= 39061 seconds
  no anomalies in 413 test result(s)
```

**The tests adopted from the `xoshiro` code base**:


```txt
# ./tmp/random m128
mix3 extreme = 1.90403 (sig = 00020000) weight 1 (16), p-value = 0.608
mix3 extreme = 2.78422 (sig = 01000001) weight 2 (112), p-value = 0.453
mix3 extreme = 2.98802 (sig = 00001220) weight 3 (448), p-value = 0.716
mix3 extreme = 3.51263 (sig = 22110000) weight 4 (1120), p-value = 0.392
mix3 extreme = 3.86128 (sig = 22110200) weight >=5 (4864), p-value = 0.422
bits per word = 64 (analyzing bits); min category p-value = 0.392

processed 1.11e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.917
------

mix3 extreme = 2.33512 (sig = 00020000) weight 1 (16), p-value = 0.271
mix3 extreme = 2.60631 (sig = 00000011) weight 2 (112), p-value = 0.643
mix3 extreme = 3.20941 (sig = 00001220) weight 3 (448), p-value = 0.449
mix3 extreme = 3.23491 (sig = 10000221) weight 4 (1120), p-value = 0.744
mix3 extreme = 3.98181 (sig = 12101200) weight >=5 (4864), p-value = 0.283
bits per word = 64 (analyzing bits); min category p-value = 0.271

processed 1.29e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.794
------

mix3 extreme = 2.09795 (sig = 00020000) weight 1 (16), p-value = 0.443
mix3 extreme = 2.59859 (sig = 00000011) weight 2 (112), p-value = 0.651
mix3 extreme = 3.16919 (sig = 00020012) weight 3 (448), p-value = 0.496
mix3 extreme = 3.32522 (sig = 01002210) weight 4 (1120), p-value = 0.628
mix3 extreme = 3.87995 (sig = 20211102) weight >=5 (4864), p-value = 0.398
bits per word = 64 (analyzing bits); min category p-value = 0.398

processed 1.66e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.921
------

mix3 extreme = 1.97959 (sig = 00020000) weight 1 (16), p-value = 0.543
mix3 extreme = 2.47936 (sig = 00000011) weight 2 (112), p-value = 0.773
mix3 extreme = 3.25629 (sig = 02000022) weight 3 (448), p-value = 0.397
mix3 extreme = 3.20819 (sig = 20001012) weight 4 (1120), p-value = 0.776
mix3 extreme = 3.85604 (sig = 21111101) weight >=5 (4864), p-value = 0.429
bits per word = 64 (analyzing bits); min category p-value = 0.397

processed 1.85e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.92
------

mix3 extreme = 2.14920 (sig = 00020000) weight 1 (16), p-value = 0.402
mix3 extreme = 2.58549 (sig = 00000011) weight 2 (112), p-value = 0.665
mix3 extreme = 2.97421 (sig = 02000022) weight 3 (448), p-value = 0.732
mix3 extreme = 2.92783 (sig = 00112001) weight 4 (1120), p-value = 0.978
mix3 extreme = 3.95789 (sig = 22110200) weight >=5 (4864), p-value = 0.308
bits per word = 64 (analyzing bits); min category p-value = 0.308

processed 2.03e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.841
------

mix3 extreme = 1.47529 (sig = 00001000) weight 1 (16), p-value = 0.911
mix3 extreme = 2.73432 (sig = 10000100) weight 2 (112), p-value = 0.505
mix3 extreme = 2.85412 (sig = 00220100) weight 3 (448), p-value = 0.856
mix3 extreme = 3.71866 (sig = 20200011) weight 4 (1120), p-value = 0.201
mix3 extreme = 3.82703 (sig = 12212021) weight >=5 (4864), p-value = 0.468
bits per word = 64 (analyzing bits); min category p-value = 0.201

processed 2.59e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.674
------

mix3 extreme = 1.48360 (sig = 20000000) weight 1 (16), p-value = 0.907
mix3 extreme = 2.46121 (sig = 00002002) weight 2 (112), p-value = 0.79
mix3 extreme = 2.87026 (sig = 20000201) weight 3 (448), p-value = 0.841
mix3 extreme = 3.86069 (sig = 20200011) weight 4 (1120), p-value = 0.119
mix3 extreme = 3.76155 (sig = 21201200) weight >=5 (4864), p-value = 0.56
bits per word = 64 (analyzing bits); min category p-value = 0.119

processed 3.14e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.469
------

mix3 extreme = 1.84311 (sig = 20000000) weight 1 (16), p-value = 0.661
mix3 extreme = 2.57296 (sig = 10001000) weight 2 (112), p-value = 0.679
mix3 extreme = 3.19014 (sig = 20000201) weight 3 (448), p-value = 0.471
mix3 extreme = 3.98985 (sig = 20200011) weight 4 (1120), p-value = 0.0714
mix3 extreme = 3.85549 (sig = 22121010) weight >=5 (4864), p-value = 0.43
bits per word = 64 (analyzing bits); min category p-value = 0.0714

processed 4.07e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.309
------

mix3 extreme = 1.41079 (sig = 20000000) weight 1 (16), p-value = 0.937
mix3 extreme = 3.04738 (sig = 10001000) weight 2 (112), p-value = 0.228
mix3 extreme = 3.74043 (sig = 00220100) weight 3 (448), p-value = 0.079
mix3 extreme = 3.82980 (sig = 00101210) weight 4 (1120), p-value = 0.134
mix3 extreme = 3.95613 (sig = 12012120) weight >=5 (4864), p-value = 0.31
bits per word = 64 (analyzing bits); min category p-value = 0.079

processed 5.18e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.337
------

mix3 extreme = 1.46023 (sig = 00020000) weight 1 (16), p-value = 0.917
mix3 extreme = 2.75344 (sig = 00200100) weight 2 (112), p-value = 0.484
mix3 extreme = 3.35270 (sig = 20000201) weight 3 (448), p-value = 0.301
mix3 extreme = 3.74339 (sig = 02102100) weight 4 (1120), p-value = 0.184
mix3 extreme = 3.75002 (sig = 12012120) weight >=5 (4864), p-value = 0.577
bits per word = 64 (analyzing bits); min category p-value = 0.184

processed 6.1e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.638
------

mix3 extreme = 1.50237 (sig = 00000020) weight 1 (16), p-value = 0.898
mix3 extreme = 2.42220 (sig = 00200100) weight 2 (112), p-value = 0.825
mix3 extreme = 2.96916 (sig = 20000201) weight 3 (448), p-value = 0.738
mix3 extreme = 3.78954 (sig = 02102100) weight 4 (1120), p-value = 0.156
mix3 extreme = 3.66917 (sig = 10010121) weight >=5 (4864), p-value = 0.694
bits per word = 64 (analyzing bits); min category p-value = 0.156

processed 7.03e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.571
------

mix3 extreme = 1.78774 (sig = 00000020) weight 1 (16), p-value = 0.707
mix3 extreme = 2.70149 (sig = 00010100) weight 2 (112), p-value = 0.54
mix3 extreme = 3.27303 (sig = 01120000) weight 3 (448), p-value = 0.379
mix3 extreme = 4.21573 (sig = 02102100) weight 4 (1120), p-value = 0.0275
mix3 extreme = 3.70578 (sig = 12100210) weight >=5 (4864), p-value = 0.641
bits per word = 64 (analyzing bits); min category p-value = 0.0275

processed 8.5e+08 bytes in 0 seconds (inf GB/s, inf TB/h). Fri Feb 21 23:22:45 2025

p = 0.13
------

mix3 extreme = 2.31948 (sig = 10000000) weight 1 (16), p-value = 0.281
mix3 extreme = 2.44934 (sig = 00010100) weight 2 (112), p-value = 0.801
mix3 extreme = 3.53130 (sig = 01120000) weight 3 (448), p-value = 0.169
mix3 extreme = 4.28717 (sig = 02102100) weight 4 (1120), p-value = 0.0201
mix3 extreme = 3.66277 (sig = 12100210) weight >=5 (4864), p-value = 0.703
bits per word = 64 (analyzing bits); min category p-value = 0.0201

processed 1.02e+09 bytes in 1 seconds (1.017 GB/s, 3.661 TB/h). Fri Feb 21 23:22:46 2025

p = 0.0964
------

mix3 extreme = 2.60876 (sig = 00000020) weight 1 (16), p-value = 0.136
mix3 extreme = 2.67150 (sig = 22000000) weight 2 (112), p-value = 0.572
mix3 extreme = 3.39462 (sig = 01120000) weight 3 (448), p-value = 0.265
mix3 extreme = 3.62549 (sig = 02200120) weight 4 (1120), p-value = 0.276
mix3 extreme = 3.63707 (sig = 12100021) weight >=5 (4864), p-value = 0.739
bits per word = 64 (analyzing bits); min category p-value = 0.136

processed 1.26e+09 bytes in 1 seconds (1.257 GB/s, 4.526 TB/h). Fri Feb 21 23:22:46 2025

p = 0.518
------

mix3 extreme = 2.15590 (sig = 00000020) weight 1 (16), p-value = 0.397
mix3 extreme = 2.80570 (sig = 00001010) weight 2 (112), p-value = 0.431
mix3 extreme = 3.40481 (sig = 02020001) weight 3 (448), p-value = 0.257
mix3 extreme = 3.36693 (sig = 01100012) weight 4 (1120), p-value = 0.573
mix3 extreme = 3.51384 (sig = 12012120) weight >=5 (4864), p-value = 0.883
bits per word = 64 (analyzing bits); min category p-value = 0.257

processed 1.52e+09 bytes in 1 seconds (1.516 GB/s, 5.458 TB/h). Fri Feb 21 23:22:46 2025

p = 0.773
------

mix3 extreme = 1.90047 (sig = 00000020) weight 1 (16), p-value = 0.611
mix3 extreme = 3.23402 (sig = 00001010) weight 2 (112), p-value = 0.128
mix3 extreme = 2.96881 (sig = 20001001) weight 3 (448), p-value = 0.739
mix3 extreme = 3.52470 (sig = 01012002) weight 4 (1120), p-value = 0.378
mix3 extreme = 3.72569 (sig = 22111101) weight >=5 (4864), p-value = 0.612
bits per word = 64 (analyzing bits); min category p-value = 0.128

processed 1.76e+09 bytes in 1 seconds (1.756 GB/s, 6.323 TB/h). Fri Feb 21 23:22:46 2025

p = 0.495
------

mix3 extreme = 1.57279 (sig = 00000010) weight 1 (16), p-value = 0.86
mix3 extreme = 2.81387 (sig = 00001010) weight 2 (112), p-value = 0.423
mix3 extreme = 3.18715 (sig = 02002010) weight 3 (448), p-value = 0.475
mix3 extreme = 3.33070 (sig = 00101120) weight 4 (1120), p-value = 0.621
mix3 extreme = 4.16820 (sig = 20122202) weight >=5 (4864), p-value = 0.139
bits per word = 64 (analyzing bits); min category p-value = 0.139

processed 2.02e+09 bytes in 1 seconds (2.015 GB/s, 7.255 TB/h). Fri Feb 21 23:22:46 2025

p = 0.526
------

mix3 extreme = 1.53157 (sig = 00000020) weight 1 (16), p-value = 0.883
mix3 extreme = 2.71599 (sig = 00001010) weight 2 (112), p-value = 0.524
mix3 extreme = 3.16021 (sig = 02002010) weight 3 (448), p-value = 0.507
mix3 extreme = 3.26824 (sig = 00101120) weight 4 (1120), p-value = 0.703
mix3 extreme = 4.01251 (sig = 22021002) weight >=5 (4864), p-value = 0.253
bits per word = 64 (analyzing bits); min category p-value = 0.253

processed 2.51e+09 bytes in 1 seconds (2.514 GB/s, 9.052 TB/h). Fri Feb 21 23:22:46 2025

p = 0.768
------

mix3 extreme = 1.74122 (sig = 00000020) weight 1 (16), p-value = 0.744
mix3 extreme = 2.86200 (sig = 00010100) weight 2 (112), p-value = 0.377
mix3 extreme = 3.41797 (sig = 02002010) weight 3 (448), p-value = 0.246
mix3 extreme = 3.48775 (sig = 00101120) weight 4 (1120), p-value = 0.421
mix3 extreme = 4.32378 (sig = 22021002) weight >=5 (4864), p-value = 0.0719
bits per word = 64 (analyzing bits); min category p-value = 0.0719

processed 3.01e+09 bytes in 1 seconds (3.014 GB/s, 10.85 TB/h). Fri Feb 21 23:22:46 2025

p = 0.311
------

mix3 extreme = 1.61472 (sig = 00000020) weight 1 (16), p-value = 0.835
mix3 extreme = 3.22143 (sig = 00010100) weight 2 (112), p-value = 0.133
mix3 extreme = 3.34281 (sig = 02002010) weight 3 (448), p-value = 0.31
mix3 extreme = 3.41424 (sig = 10011100) weight 4 (1120), p-value = 0.512
mix3 extreme = 3.68273 (sig = 00122011) weight >=5 (4864), p-value = 0.675
bits per word = 64 (analyzing bits); min category p-value = 0.133

processed 4.01e+09 bytes in 1 seconds (4.012 GB/s, 14.44 TB/h). Fri Feb 21 23:22:46 2025

p = 0.511
------

mix3 extreme = 2.08971 (sig = 00000020) weight 1 (16), p-value = 0.45
mix3 extreme = 3.03226 (sig = 00010100) weight 2 (112), p-value = 0.238
mix3 extreme = 3.22814 (sig = 00010102) weight 3 (448), p-value = 0.428
mix3 extreme = 3.27577 (sig = 10011100) weight 4 (1120), p-value = 0.693
mix3 extreme = 3.67641 (sig = 12012120) weight >=5 (4864), p-value = 0.684
bits per word = 64 (analyzing bits); min category p-value = 0.238

processed 5.01e+09 bytes in 2 seconds (2.505 GB/s, 9.019 TB/h). Fri Feb 21 23:22:47 2025

p = 0.744
------

mix3 extreme = 1.91894 (sig = 10000000) weight 1 (16), p-value = 0.595
mix3 extreme = 2.77625 (sig = 00010100) weight 2 (112), p-value = 0.461
mix3 extreme = 3.06382 (sig = 00011001) weight 3 (448), p-value = 0.625
mix3 extreme = 3.14390 (sig = 21020200) weight 4 (1120), p-value = 0.846
mix3 extreme = 3.59615 (sig = 00122011) weight >=5 (4864), p-value = 0.792
bits per word = 64 (analyzing bits); min category p-value = 0.461

processed 6.01e+09 bytes in 2 seconds (3.004 GB/s, 10.82 TB/h). Fri Feb 21 23:22:47 2025

p = 0.954
------

mix3 extreme = 1.86597 (sig = 00000020) weight 1 (16), p-value = 0.641
mix3 extreme = 2.61078 (sig = 00010100) weight 2 (112), p-value = 0.638
mix3 extreme = 3.42404 (sig = 00011001) weight 3 (448), p-value = 0.242
mix3 extreme = 3.40741 (sig = 00020122) weight 4 (1120), p-value = 0.52
mix3 extreme = 3.89941 (sig = 00122011) weight >=5 (4864), p-value = 0.374
bits per word = 64 (analyzing bits); min category p-value = 0.242

processed 7.01e+09 bytes in 2 seconds (3.504 GB/s, 12.61 TB/h). Fri Feb 21 23:22:47 2025

p = 0.749
------

mix3 extreme = 1.73399 (sig = 00000020) weight 1 (16), p-value = 0.75
mix3 extreme = 2.96016 (sig = 20000200) weight 2 (112), p-value = 0.292
mix3 extreme = 3.09543 (sig = 02000022) weight 3 (448), p-value = 0.586
mix3 extreme = 3.42588 (sig = 10101001) weight 4 (1120), p-value = 0.497
mix3 extreme = 3.66808 (sig = 10110201) weight >=5 (4864), p-value = 0.695
bits per word = 64 (analyzing bits); min category p-value = 0.292

processed 8.5e+09 bytes in 3 seconds (2.835 GB/s, 10.21 TB/h). Fri Feb 21 23:22:48 2025

p = 0.822
------

mix3 extreme = 1.52914 (sig = 00200000) weight 1 (16), p-value = 0.885
mix3 extreme = 3.81757 (sig = 20000200) weight 2 (112), p-value = 0.015
mix3 extreme = 3.07953 (sig = 02201000) weight 3 (448), p-value = 0.605
mix3 extreme = 3.26619 (sig = 01102001) weight 4 (1120), p-value = 0.705
mix3 extreme = 3.73963 (sig = 02122020) weight >=5 (4864), p-value = 0.592
bits per word = 64 (analyzing bits); min category p-value = 0.015

processed 1e+10 bytes in 3 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:22:48 2025

p = 0.0727
------

mix3 extreme = 2.00898 (sig = 00020000) weight 1 (16), p-value = 0.518
mix3 extreme = 2.73872 (sig = 20000200) weight 2 (112), p-value = 0.5
mix3 extreme = 2.89865 (sig = 22000200) weight 3 (448), p-value = 0.814
mix3 extreme = 3.51139 (sig = 10002220) weight 4 (1120), p-value = 0.393
mix3 extreme = 3.46747 (sig = 21020120) weight >=5 (4864), p-value = 0.922
bits per word = 64 (analyzing bits); min category p-value = 0.393

processed 1.25e+10 bytes in 4 seconds (3.129 GB/s, 11.27 TB/h). Fri Feb 21 23:22:49 2025

p = 0.918
------

mix3 extreme = 2.54743 (sig = 00020000) weight 1 (16), p-value = 0.16
mix3 extreme = 2.66976 (sig = 00020200) weight 2 (112), p-value = 0.574
mix3 extreme = 3.21302 (sig = 02020010) weight 3 (448), p-value = 0.445
mix3 extreme = 3.17616 (sig = 10002220) weight 4 (1120), p-value = 0.812
mix3 extreme = 4.08852 (sig = 00122011) weight >=5 (4864), p-value = 0.19
bits per word = 64 (analyzing bits); min category p-value = 0.16

processed 1.5e+10 bytes in 5 seconds (3.003 GB/s, 10.81 TB/h). Fri Feb 21 23:22:50 2025

p = 0.582
------

mix3 extreme = 2.32339 (sig = 00020000) weight 1 (16), p-value = 0.278
mix3 extreme = 2.41362 (sig = 00001010) weight 2 (112), p-value = 0.832
mix3 extreme = 3.50336 (sig = 02020010) weight 3 (448), p-value = 0.186
mix3 extreme = 3.13576 (sig = 20221000) weight 4 (1120), p-value = 0.854
mix3 extreme = 4.28795 (sig = 00122011) weight >=5 (4864), p-value = 0.084
bits per word = 64 (analyzing bits); min category p-value = 0.084

processed 1.75e+10 bytes in 5 seconds (3.502 GB/s, 12.61 TB/h). Fri Feb 21 23:22:50 2025

p = 0.355
------

mix3 extreme = 2.55481 (sig = 00020000) weight 1 (16), p-value = 0.157
mix3 extreme = 2.72892 (sig = 00001010) weight 2 (112), p-value = 0.51
mix3 extreme = 3.28466 (sig = 11000001) weight 3 (448), p-value = 0.367
mix3 extreme = 3.42088 (sig = 01020102) weight 4 (1120), p-value = 0.503
mix3 extreme = 4.05525 (sig = 00122011) weight >=5 (4864), p-value = 0.216
bits per word = 64 (analyzing bits); min category p-value = 0.157

processed 2e+10 bytes in 6 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:22:51 2025

p = 0.575
------

mix3 extreme = 2.46488 (sig = 00020000) weight 1 (16), p-value = 0.198
mix3 extreme = 2.44365 (sig = 20000200) weight 2 (112), p-value = 0.806
mix3 extreme = 3.31277 (sig = 02020010) weight 3 (448), p-value = 0.339
mix3 extreme = 3.81631 (sig = 01020102) weight 4 (1120), p-value = 0.141
mix3 extreme = 4.12544 (sig = 00122011) weight >=5 (4864), p-value = 0.165
bits per word = 64 (analyzing bits); min category p-value = 0.141

processed 2.5e+10 bytes in 8 seconds (3.127 GB/s, 11.26 TB/h). Fri Feb 21 23:22:53 2025

p = 0.532
------

mix3 extreme = 2.18715 (sig = 00020000) weight 1 (16), p-value = 0.373
mix3 extreme = 2.10006 (sig = 20000020) weight 2 (112), p-value = 0.983
mix3 extreme = 3.08063 (sig = 00210002) weight 3 (448), p-value = 0.604
mix3 extreme = 3.66924 (sig = 02110010) weight 4 (1120), p-value = 0.239
mix3 extreme = 4.35154 (sig = 00122011) weight >=5 (4864), p-value = 0.0636
bits per word = 64 (analyzing bits); min category p-value = 0.0636

processed 3e+10 bytes in 9 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:22:54 2025

p = 0.28
------

mix3 extreme = 1.43572 (sig = 00020000) weight 1 (16), p-value = 0.927
mix3 extreme = 2.41760 (sig = 02000020) weight 2 (112), p-value = 0.829
mix3 extreme = 3.16026 (sig = 02100020) weight 3 (448), p-value = 0.507
mix3 extreme = 3.44650 (sig = 02220002) weight 4 (1120), p-value = 0.471
mix3 extreme = 4.35459 (sig = 00221220) weight >=5 (4864), p-value = 0.0628
bits per word = 64 (analyzing bits); min category p-value = 0.0628

processed 4e+10 bytes in 12 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:22:57 2025

p = 0.277
------

mix3 extreme = 1.73103 (sig = 02000000) weight 1 (16), p-value = 0.752
mix3 extreme = 2.59948 (sig = 02000020) weight 2 (112), p-value = 0.65
mix3 extreme = 3.24793 (sig = 02020010) weight 3 (448), p-value = 0.406
mix3 extreme = 3.18550 (sig = 00022021) weight 4 (1120), p-value = 0.802
mix3 extreme = 4.36307 (sig = 00122011) weight >=5 (4864), p-value = 0.0605
bits per word = 64 (analyzing bits); min category p-value = 0.0605

processed 5e+10 bytes in 15 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:23:00 2025

p = 0.268
------

mix3 extreme = 1.69245 (sig = 02000000) weight 1 (16), p-value = 0.781
mix3 extreme = 2.90012 (sig = 02000020) weight 2 (112), p-value = 0.342
mix3 extreme = 3.04010 (sig = 02100020) weight 3 (448), p-value = 0.654
mix3 extreme = 3.18726 (sig = 11012000) weight 4 (1120), p-value = 0.8
mix3 extreme = 4.13605 (sig = 00122011) weight >=5 (4864), p-value = 0.158
bits per word = 64 (analyzing bits); min category p-value = 0.158

processed 6e+10 bytes in 18 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:23:03 2025

p = 0.577
------

mix3 extreme = 2.02538 (sig = 02000000) weight 1 (16), p-value = 0.504
mix3 extreme = 2.69836 (sig = 20000020) weight 2 (112), p-value = 0.543
mix3 extreme = 2.74898 (sig = 00200110) weight 3 (448), p-value = 0.932
mix3 extreme = 3.09844 (sig = 11012000) weight 4 (1120), p-value = 0.887
mix3 extreme = 4.04079 (sig = 20022201) weight >=5 (4864), p-value = 0.228
bits per word = 64 (analyzing bits); min category p-value = 0.228

processed 7e+10 bytes in 21 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:23:06 2025

p = 0.726
------

mix3 extreme = 2.00762 (sig = 00020000) weight 1 (16), p-value = 0.519
mix3 extreme = 2.67485 (sig = 20000020) weight 2 (112), p-value = 0.569
mix3 extreme = 2.98340 (sig = 00110100) weight 3 (448), p-value = 0.722
mix3 extreme = 3.30873 (sig = 12210000) weight 4 (1120), p-value = 0.65
mix3 extreme = 4.04171 (sig = 20022201) weight >=5 (4864), p-value = 0.227
bits per word = 64 (analyzing bits); min category p-value = 0.227

processed 8.5e+10 bytes in 25 seconds (3.4 GB/s, 12.24 TB/h). Fri Feb 21 23:23:10 2025

p = 0.725
------

mix3 extreme = 1.98503 (sig = 00100000) weight 1 (16), p-value = 0.538
mix3 extreme = 2.83929 (sig = 20000020) weight 2 (112), p-value = 0.398
mix3 extreme = 3.24294 (sig = 00110100) weight 3 (448), p-value = 0.412
mix3 extreme = 3.26307 (sig = 00022012) weight 4 (1120), p-value = 0.709
mix3 extreme = 3.82851 (sig = 20212120) weight >=5 (4864), p-value = 0.466
bits per word = 64 (analyzing bits); min category p-value = 0.398

processed 1e+11 bytes in 30 seconds (3.333 GB/s, 12 TB/h). Fri Feb 21 23:23:15 2025

p = 0.921
------

mix3 extreme = 1.80847 (sig = 00000001) weight 1 (16), p-value = 0.69
mix3 extreme = 2.36679 (sig = 00001020) weight 2 (112), p-value = 0.868
mix3 extreme = 3.12962 (sig = 00110100) weight 3 (448), p-value = 0.544
mix3 extreme = 3.41207 (sig = 00022012) weight 4 (1120), p-value = 0.514
mix3 extreme = 3.50238 (sig = 01120021) weight >=5 (4864), p-value = 0.894
bits per word = 64 (analyzing bits); min category p-value = 0.514

processed 1.25e+11 bytes in 37 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:23:22 2025

p = 0.973
------

mix3 extreme = 1.61617 (sig = 01000000) weight 1 (16), p-value = 0.834
mix3 extreme = 2.75953 (sig = 10002000) weight 2 (112), p-value = 0.478
mix3 extreme = 3.01768 (sig = 22000200) weight 3 (448), p-value = 0.681
mix3 extreme = 3.12351 (sig = 20120020) weight 4 (1120), p-value = 0.865
mix3 extreme = 3.88895 (sig = 01201202) weight >=5 (4864), p-value = 0.387
bits per word = 64 (analyzing bits); min category p-value = 0.387

processed 1.5e+11 bytes in 45 seconds (3.334 GB/s, 12 TB/h). Fri Feb 21 23:23:30 2025

p = 0.914
------

mix3 extreme = 1.77082 (sig = 10000000) weight 1 (16), p-value = 0.721
mix3 extreme = 3.16452 (sig = 20000020) weight 2 (112), p-value = 0.16
mix3 extreme = 3.33010 (sig = 22000200) weight 3 (448), p-value = 0.322
mix3 extreme = 3.30032 (sig = 10202001) weight 4 (1120), p-value = 0.661
mix3 extreme = 3.71439 (sig = 01201202) weight >=5 (4864), p-value = 0.629
bits per word = 64 (analyzing bits); min category p-value = 0.16

processed 1.75e+11 bytes in 52 seconds (3.366 GB/s, 12.12 TB/h). Fri Feb 21 23:23:37 2025

p = 0.581
------

mix3 extreme = 1.63286 (sig = 10000000) weight 1 (16), p-value = 0.823
mix3 extreme = 3.04031 (sig = 20000020) weight 2 (112), p-value = 0.233
mix3 extreme = 3.07386 (sig = 00020120) weight 3 (448), p-value = 0.612
mix3 extreme = 3.25670 (sig = 20200101) weight 4 (1120), p-value = 0.717
mix3 extreme = 3.86559 (sig = 02121212) weight >=5 (4864), p-value = 0.417
bits per word = 64 (analyzing bits); min category p-value = 0.233

processed 2e+11 bytes in 59 seconds (3.39 GB/s, 12.2 TB/h). Fri Feb 21 23:23:44 2025

p = 0.734
------

mix3 extreme = 1.16364 (sig = 00000010) weight 1 (16), p-value = 0.989
mix3 extreme = 3.24185 (sig = 20000020) weight 2 (112), p-value = 0.125
mix3 extreme = 3.33043 (sig = 00020120) weight 3 (448), p-value = 0.322
mix3 extreme = 3.08521 (sig = 01100011) weight 4 (1120), p-value = 0.898
mix3 extreme = 3.38515 (sig = 20222110) weight >=5 (4864), p-value = 0.969
bits per word = 64 (analyzing bits); min category p-value = 0.125

processed 2.5e+11 bytes in 74 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:23:59 2025

p = 0.486
------

mix3 extreme = 1.51793 (sig = 02000000) weight 1 (16), p-value = 0.89
mix3 extreme = 3.13632 (sig = 02000020) weight 2 (112), p-value = 0.175
mix3 extreme = 4.30962 (sig = 00020120) weight 3 (448), p-value = 0.0073
mix3 extreme = 3.03490 (sig = 01000212) weight 4 (1120), p-value = 0.933
mix3 extreme = 3.69122 (sig = 12111112) weight >=5 (4864), p-value = 0.662
bits per word = 64 (analyzing bits); min category p-value = 0.0073

processed 3e+11 bytes in 89 seconds (3.371 GB/s, 12.14 TB/h). Fri Feb 21 23:24:14 2025

p = 0.036
------

mix3 extreme = 1.90308 (sig = 00002000) weight 1 (16), p-value = 0.609
mix3 extreme = 2.65558 (sig = 00022000) weight 2 (112), p-value = 0.589
mix3 extreme = 3.65368 (sig = 00020120) weight 3 (448), p-value = 0.109
mix3 extreme = 3.49934 (sig = 01000212) weight 4 (1120), p-value = 0.407
mix3 extreme = 3.96134 (sig = 12111112) weight >=5 (4864), p-value = 0.304
bits per word = 64 (analyzing bits); min category p-value = 0.109

processed 4e+11 bytes in 119 seconds (3.361 GB/s, 12.1 TB/h). Fri Feb 21 23:24:44 2025

p = 0.44
------

mix3 extreme = 2.01103 (sig = 00000010) weight 1 (16), p-value = 0.516
mix3 extreme = 2.75092 (sig = 20000020) weight 2 (112), p-value = 0.487
mix3 extreme = 3.58533 (sig = 00020120) weight 3 (448), p-value = 0.14
mix3 extreme = 3.00752 (sig = 00101101) weight 4 (1120), p-value = 0.948
mix3 extreme = 3.78108 (sig = 20222110) weight >=5 (4864), p-value = 0.532
bits per word = 64 (analyzing bits); min category p-value = 0.14

processed 5e+11 bytes in 148 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:25:13 2025

p = 0.53
------

mix3 extreme = 2.46075 (sig = 00000010) weight 1 (16), p-value = 0.2
mix3 extreme = 3.05595 (sig = 10001000) weight 2 (112), p-value = 0.222
mix3 extreme = 3.57444 (sig = 00020120) weight 3 (448), p-value = 0.146
mix3 extreme = 3.08460 (sig = 02100120) weight 4 (1120), p-value = 0.898
mix3 extreme = 3.57242 (sig = 21020202) weight >=5 (4864), p-value = 0.821
bits per word = 64 (analyzing bits); min category p-value = 0.146

processed 6e+11 bytes in 178 seconds (3.371 GB/s, 12.14 TB/h). Fri Feb 21 23:25:43 2025

p = 0.544
------

mix3 extreme = 2.39889 (sig = 00010000) weight 1 (16), p-value = 0.233
mix3 extreme = 3.05936 (sig = 20000020) weight 2 (112), p-value = 0.22
mix3 extreme = 3.35611 (sig = 00020120) weight 3 (448), p-value = 0.298
mix3 extreme = 3.06385 (sig = 12002002) weight 4 (1120), p-value = 0.914
mix3 extreme = 3.77731 (sig = 21020202) weight >=5 (4864), p-value = 0.538
bits per word = 64 (analyzing bits); min category p-value = 0.22

processed 7e+11 bytes in 207 seconds (3.382 GB/s, 12.17 TB/h). Fri Feb 21 23:26:12 2025

p = 0.712
------

mix3 extreme = 2.44670 (sig = 00010000) weight 1 (16), p-value = 0.207
mix3 extreme = 3.17448 (sig = 00101000) weight 2 (112), p-value = 0.155
mix3 extreme = 3.94606 (sig = 00020120) weight 3 (448), p-value = 0.035
mix3 extreme = 3.39578 (sig = 12002002) weight 4 (1120), p-value = 0.535
mix3 extreme = 3.46995 (sig = 22000212) weight >=5 (4864), p-value = 0.921
bits per word = 64 (analyzing bits); min category p-value = 0.035

processed 8.5e+11 bytes in 252 seconds (3.373 GB/s, 12.14 TB/h). Fri Feb 21 23:26:57 2025

p = 0.163
------

mix3 extreme = 2.60423 (sig = 00000200) weight 1 (16), p-value = 0.138
mix3 extreme = 4.14356 (sig = 02000020) weight 2 (112), p-value = 0.00382
mix3 extreme = 3.30244 (sig = 00210010) weight 3 (448), p-value = 0.349
mix3 extreme = 2.92058 (sig = 00111100) weight 4 (1120), p-value = 0.98
mix3 extreme = 3.64298 (sig = 21221022) weight >=5 (4864), p-value = 0.73
bits per word = 64 (analyzing bits); min category p-value = 0.00382

processed 1e+12 bytes in 296 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:27:41 2025

p = 0.019
------

mix3 extreme = 2.39411 (sig = 00000200) weight 1 (16), p-value = 0.236
mix3 extreme = 3.86207 (sig = 02000020) weight 2 (112), p-value = 0.0125
mix3 extreme = 3.43732 (sig = 00210010) weight 3 (448), p-value = 0.231
mix3 extreme = 3.30612 (sig = 10010102) weight 4 (1120), p-value = 0.654
mix3 extreme = 3.69570 (sig = 02122102) weight >=5 (4864), p-value = 0.656
bits per word = 64 (analyzing bits); min category p-value = 0.0125

processed 1.25e+12 bytes in 370 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:28:55 2025

p = 0.061
------

mix3 extreme = 1.91917 (sig = 00000200) weight 1 (16), p-value = 0.595
mix3 extreme = 2.88512 (sig = 10001000) weight 2 (112), p-value = 0.355
mix3 extreme = 3.79690 (sig = 00210010) weight 3 (448), p-value = 0.0635
mix3 extreme = 3.81192 (sig = 10010102) weight 4 (1120), p-value = 0.143
mix3 extreme = 3.94746 (sig = 20221222) weight >=5 (4864), p-value = 0.319
bits per word = 64 (analyzing bits); min category p-value = 0.0635

processed 1.5e+12 bytes in 444 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:30:09 2025

p = 0.28
------

mix3 extreme = 1.91653 (sig = 00000200) weight 1 (16), p-value = 0.598
mix3 extreme = 3.17062 (sig = 02000020) weight 2 (112), p-value = 0.157
mix3 extreme = 3.50267 (sig = 00210010) weight 3 (448), p-value = 0.186
mix3 extreme = 4.12447 (sig = 10010102) weight 4 (1120), p-value = 0.0408
mix3 extreme = 4.27098 (sig = 20221222) weight >=5 (4864), p-value = 0.0903
bits per word = 64 (analyzing bits); min category p-value = 0.0408

processed 1.75e+12 bytes in 518 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:31:23 2025

p = 0.188
------

mix3 extreme = 1.81906 (sig = 00000200) weight 1 (16), p-value = 0.681
mix3 extreme = 3.06798 (sig = 10100000) weight 2 (112), p-value = 0.215
mix3 extreme = 3.71332 (sig = 02020200) weight 3 (448), p-value = 0.0876
mix3 extreme = 4.23944 (sig = 10010102) weight 4 (1120), p-value = 0.0248
mix3 extreme = 4.44624 (sig = 20221222) weight >=5 (4864), p-value = 0.0416
bits per word = 64 (analyzing bits); min category p-value = 0.0248

processed 2e+12 bytes in 592 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:32:37 2025

p = 0.118
------

mix3 extreme = 1.53717 (sig = 01000000) weight 1 (16), p-value = 0.88
mix3 extreme = 3.32377 (sig = 10100000) weight 2 (112), p-value = 0.0947
mix3 extreme = 3.12416 (sig = 02200001) weight 3 (448), p-value = 0.55
mix3 extreme = 3.46863 (sig = 02110100) weight 4 (1120), p-value = 0.443
mix3 extreme = 4.84658 (sig = 20221222) weight >=5 (4864), p-value = 0.00609
bits per word = 64 (analyzing bits); min category p-value = 0.00609

processed 2.5e+12 bytes in 740 seconds (3.378 GB/s, 12.16 TB/h). Fri Feb 21 23:35:05 2025

p = 0.0301
------

mix3 extreme = 1.61173 (sig = 00000200) weight 1 (16), p-value = 0.837
mix3 extreme = 3.37008 (sig = 10100000) weight 2 (112), p-value = 0.0807
mix3 extreme = 3.93348 (sig = 00202100) weight 3 (448), p-value = 0.0368
mix3 extreme = 3.29910 (sig = 02200021) weight 4 (1120), p-value = 0.663
mix3 extreme = 4.29826 (sig = 20221222) weight >=5 (4864), p-value = 0.0803
bits per word = 64 (analyzing bits); min category p-value = 0.0368

processed 3e+12 bytes in 887 seconds (3.382 GB/s, 12.18 TB/h). Fri Feb 21 23:37:32 2025

p = 0.171
------

mix3 extreme = 2.09163 (sig = 00000200) weight 1 (16), p-value = 0.448
mix3 extreme = 2.61382 (sig = 01100000) weight 2 (112), p-value = 0.635
mix3 extreme = 3.98155 (sig = 00202100) weight 3 (448), p-value = 0.0302
mix3 extreme = 3.38573 (sig = 00002211) weight 4 (1120), p-value = 0.549
mix3 extreme = 4.15291 (sig = 20211110) weight >=5 (4864), p-value = 0.148
bits per word = 64 (analyzing bits); min category p-value = 0.0302

processed 4e+12 bytes in 1.18e+03 seconds (3.381 GB/s, 12.17 TB/h). Fri Feb 21 23:42:28 2025

p = 0.142
------
```
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
