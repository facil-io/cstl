---
title: facil.io - C STL - a Simple Template Library for C
sidebar: 0.8.x/_sidebar.md
---
# facil.io - C STL - a Simple Template Library for C

At the core of the [facil.io library](https://facil.io) is its powerful Simple Template Library for C (and C++).

The Simple Template Library is a "swiss-army-knife" library, that uses MACROS to generate code for different common types, such as Hash Maps, Arrays, Linked Lists, Binary-Safe Strings, etc'.

The Simple Template Library also offers common functional primitives and helpers, such as bit operations, atomic operations, CLI parsing, JSON, task queues, and a custom memory allocator.

In other words, all the common building blocks one could need in a C project are placed in this single header file.

The header could be included multiple times with different results, creating different types or exposing different functionality.

**Note**: facil.io Web Application Developers get many of the features of the C STL through including the `fio.h` header. See the [facil.io IO Core documentation](fio) for more information.

## Simple Template Library (STL) Overview

The core Simple Template Library (STL) is a single file header library (`fio-stl.h`).

The header includes a Simple Template Library for the following common types:

* [Linked Lists](#linked-lists) - defined by `FIO_LIST_NAME`

* [Dynamic Arrays](#dynamic-arrays) - defined by `FIO_ARRAY_NAME`

* [Hash Maps / Sets](#hash-tables-and-maps) - defined by `FIO_MAP_NAME`

* [Binary Safe Dynamic Strings](#dynamic-strings) - defined by `FIO_STR_NAME` / `FIO_STR_SMALL`

* [Reference counting / Type wrapper](#reference-counting-and-type-wrapping) - defined by `FIO_REF_NAME`

* [Soft / Dynamic Types (FIOBJ)](#fiobj-soft-dynamic-types) - defined by `FIO_FIOBJ`

In addition, the core Simple Template Library (STL) includes helpers for common tasks, such as:

* [Pointer Arithmetics](#pointer-arithmetics) (included by default)

* [Pointer Tagging](#pointer-tagging-support) - defined by `FIO_PTR_TAG(p)`/`FIO_PTR_UNTAG(p)`

* [Logging and Assertion (without heap allocation)](#logging-and-assertions) - defined by `FIO_LOG`

* [Atomic operations](#atomic-operations) - defined by `FIO_ATOMIC`

* [Bit-Byte Operations](#bit-byte-operations) - defined by `FIO_BITWISE`

* [Bitmap helpers](#bitmap-helpers) - defined by `FIO_BITMAP`

* [Data Hashing (using Risky Hash)](#risky-hash-data-hashing) - defined by `FIO_RISKY_HASH`

* [Pseudo Random Generation](#pseudo-random-generation) - defined by `FIO_RAND`

* [String / Number conversion](#string-number-conversion) - defined by `FIO_ATOL`

* [Time Helpers](#time-helpers) - defined by `FIO_TIME`

* [Task Queue](#task-queue) - defined by `FIO_QUEUE`

* [Command Line Interface helpers](#cli-command-line-interface) - defined by `FIO_CLI`

* [URL (URI) parsing](#url-uri-parsing) - defined by `FIO_URL`

* [Custom JSON Parser](#custom-json-parser) - defined by `FIO_JSON`

* [Basic Socket / IO Helpers](#basic-socket-io-helpers) - defined by `FIO_SOCK`

* [Data Stream Containers](#data-stream-container) - defined by `FIO_STREAM`

* [Signal (pass-through) Monitoring](#signal-monitoring) - defined by `FIO_SIGNAL`

* [Local Memory Allocation](#local-memory-allocation) - defined by `FIO_MEMORY` / `FIO_MALLOC`

## Testing the Library (`FIO_TEST_CSTL`)

To test the library, define the `FIO_TEST_CSTL` macro and include the header. A testing function called `fio_test_dynamic_types` will be defined. Call that function in your code to test the library.

## Compilation Modes

The Simple Template Library types and functions could be compiled as either static or extern ("global"), either limiting their scope to a single C file (compilation unit) or exposing them throughout the program.

#### Static Functions by Default

By default, the Simple Template Library will generate static functions where possible.

To change this behavior, `FIO_EXTERN` and `FIO_EXTERN_COMPLETE` could be used to generate externally visible code.

#### `FIO_EXTERN`

If defined, the the Simple Template Library will generate non-static code.

If `FIO_EXTERN` is defined alone, only function declarations and inline functions will be generated.

If `FIO_EXTERN_COMPLETE` is defined, the function definition (the implementation code) will also be generated.

**Note**: the `FIO_EXTERN` will be **automatically undefined** each time the Simple Template Library header is included.

For example, in the header (i.e., `mymem.h`), use:

```c
#define FIO_EXTERN
#define FIO_MALLOC
#include "fio-stl.h"
```

Later, in the implementation file, use:

```c
#define FIO_EXTERN_COMPLETE 1
#include "mymem.h"
#undef FIO_EXTERN_COMPLETE
```

#### `FIO_EXTERN_COMPLETE`

When defined, this macro will force full code generation.

If `FIO_EXTERN_COMPLETE` is set to the value `2`, it will automatically self-destruct (it will undefine itself once used).

-------------------------------------------------------------------------------

## Version and Common Helper Macros

The facil.io C STL (Simple Template Library) offers a number of common helper macros that are also used internally. These are automatically included once the `fio-stl.h` is included.

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

#### `FIO_VERSION_BETA`

Translates to the STL's beta version number.

#### `FIO_VERSION_STRING`

Translates to the STL's version as a String (i.e., 0.8.0.beta1.

#### `FIO_VERSION_GUARD`

If the `FIO_VERSION_GUARD` macro is defined in **a single** translation unit (C file) **before** including `fio-stl.h` for the first time, then the version macros become available using functions as well: `fio_version_major`, `fio_version_minor`, etc'.

#### `FIO_VERSION_VALIDATE`

By adding the `FIO_VERSION_GUARD` functions, a version test could be performed during runtime (which can be used for static libraries), using the macro `FIO_VERSION_VALIDATE()`.

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

When defined, temporarily bypasses the `FIO_MEM_REALLOC` macros and uses the system's `realloc` and `free` functions.

### Naming and Misc. Macros

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

#### `FIO_MACRO2STR`

```c
#define FIO_MACRO2STR(macro) FIO_MACRO2STR_STEP2(macro)
```

Converts a macro's content to a string literal.

#### `FIO_NAME`

```c
#define FIO_NAME(prefix, postfix)
```

Used for naming functions and variables resulting in: prefix_postfix

i.e.:

```c
// typedef struct { long l; } number_s
typedef struct { long l; } FIO_NAME(number, s)

// number_s number_add(number_s a, number_s b)
FIO_NAME(number, s) FIO_NAME(number, add)(FIO_NAME(number, s) a, FIO_NAME(number, s) b) {
  a.l += b.l;
  return a;
}
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

Sets naming convention for boolean testing functions, i.e.: foo_is_true

i.e.:

```c
// typedef struct { long l; } number_s
typedef struct { long l; } FIO_NAME(number, s)

// int number_is_zero(number_s n)
int FIO_NAME2(number, zero)(FIO_NAME(number, s) n) {
  return (!n.l);
}
```

-------------------------------------------------------------------------------

## Pointer Tagging Support:

Pointer tagging allows types created using this library to have their pointers "tagged".

This is when creating / managing dynamic types, where some type data could be written to the pointer data itself.

**Note**: pointer tagging can't automatically tag "pointers" to objects placed on the stack.

#### `FIO_PTR_TAG`

Supports embedded pointer tagging / untagging for the included types.

Should resolve to a tagged pointer value. i.e.: `((uintptr_t)(p) | 1)`

#### `FIO_PTR_UNTAG`

Supports embedded pointer tagging / untagging for the included types.

Should resolve to an untagged pointer value. i.e.: `((uintptr_t)(p) | ~1UL)`

**Note**: `FIO_PTR_UNTAG` might be called more then once or on untagged pointers. For this reason, `FIO_PTR_UNTAG` should always return the valid pointer, even if called on an untagged pointer.

#### `FIO_PTR_TAG_TYPE`

If the FIO_PTR_TAG_TYPE is defined, then functions returning a type's pointer will return a pointer of the specified type instead.

-------------------------------------------------------------------------------
