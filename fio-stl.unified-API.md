# facil.io STL API Reference

Generated automatically from code documentation comments in `./fio-stl/*.h`. Do not hand-edit generated output.

The [`fio-stl.md`](fio-stl) contains logic and explanations, here are listed all the public symbols detected (correctly or incorrectly), allowing for a quick reference (using your browser's / editor's search capabilities).

Total symbols: 3105.

## Contents

- [`./fio-stl/000 copyright.h`](#fio-stl-000-copyright-h) — 1
- [`./fio-stl/000 core.h`](#fio-stl-000-core-h) — 1758
- [`./fio-stl/001 header.h`](#fio-stl-001-header-h) — 8
- [`./fio-stl/001 logging.h`](#fio-stl-001-logging-h) — 2
- [`./fio-stl/001 memalt.h`](#fio-stl-001-memalt-h) — 5
- [`./fio-stl/001 patches.h`](#fio-stl-001-patches-h) — 2
- [`./fio-stl/002 atol.h`](#fio-stl-002-atol-h) — 46
- [`./fio-stl/002 crc32.h`](#fio-stl-002-crc32-h) — 1
- [`./fio-stl/002 glob matching.h`](#fio-stl-002-glob-matching-h) — 1
- [`./fio-stl/002 imap.h`](#fio-stl-002-imap-h) — 7
- [`./fio-stl/002 math.h`](#fio-stl-002-math-h) — 6
- [`./fio-stl/002 random.h`](#fio-stl-002-random-h) — 15
- [`./fio-stl/002 signals.h`](#fio-stl-002-signals-h) — 8
- [`./fio-stl/002 sort.h`](#fio-stl-002-sort-h) — 3
- [`./fio-stl/002 threads.h`](#fio-stl-002-threads-h) — 31
- [`./fio-stl/002 url.h`](#fio-stl-002-url-h) — 7
- [`./fio-stl/003 entities.h`](#fio-stl-003-entities-h) — 1
- [`./fio-stl/004 files.h`](#fio-stl-004-files-h) — 21
- [`./fio-stl/004 json.h`](#fio-stl-004-json-h) — 5
- [`./fio-stl/004 multipart.h`](#fio-stl-004-multipart-h) — 3
- [`./fio-stl/004 resp3.h`](#fio-stl-004-resp3-h) — 23
- [`./fio-stl/004 sock.h`](#fio-stl-004-sock-h) — 35
- [`./fio-stl/004 state callbacks.h`](#fio-stl-004-state-callbacks-h) — 4
- [`./fio-stl/004 time.h`](#fio-stl-004-time-h) — 17
- [`./fio-stl/004 urlencoded.h`](#fio-stl-004-urlencoded-h) — 3
- [`./fio-stl/005 cli.h`](#fio-stl-005-cli-h) — 23
- [`./fio-stl/010 mem.h`](#fio-stl-010-mem-h) — 46
- [`./fio-stl/011 string core.h`](#fio-stl-011-string-core-h) — 103
- [`./fio-stl/012 gfm.h`](#fio-stl-012-gfm-h) — 10
- [`./fio-stl/102 poll api.h`](#fio-stl-102-poll-api-h) — 16
- [`./fio-stl/102 queue.h`](#fio-stl-102-queue-h) — 28
- [`./fio-stl/102 stream.h`](#fio-stl-102-stream-h) — 16
- [`./fio-stl/103 md2html.h`](#fio-stl-103-md2html-h) — 2
- [`./fio-stl/104 mustache.h`](#fio-stl-104-mustache-h) — 14
- [`./fio-stl/150 crypto core.h`](#fio-stl-150-crypto-core-h) — 2
- [`./fio-stl/152 blake2.h`](#fio-stl-152-blake2-h) — 14
- [`./fio-stl/152 chacha20poly1305.h`](#fio-stl-152-chacha20poly1305-h) — 7
- [`./fio-stl/152 sha1.h`](#fio-stl-152-sha1-h) — 4
- [`./fio-stl/152 sha2.h`](#fio-stl-152-sha2-h) — 10
- [`./fio-stl/152 sha2z hkdf.h`](#fio-stl-152-sha2z-hkdf-h) — 5
- [`./fio-stl/152 sha3.h`](#fio-stl-152-sha3-h) — 17
- [`./fio-stl/153 aes.h`](#fio-stl-153-aes-h) — 4
- [`./fio-stl/154 ed25519.h`](#fio-stl-154-ed25519-h) — 13
- [`./fio-stl/154 p256.h`](#fio-stl-154-p256-h) — 5
- [`./fio-stl/154 p384.h`](#fio-stl-154-p384-h) — 2
- [`./fio-stl/155 asn1.h`](#fio-stl-155-asn1-h) — 51
- [`./fio-stl/155 rsa.h`](#fio-stl-155-rsa-h) — 9
- [`./fio-stl/155 x509.h`](#fio-stl-155-x509-h) — 18
- [`./fio-stl/156 mlkem.h`](#fio-stl-156-mlkem-h) — 17
- [`./fio-stl/156 pem.h`](#fio-stl-156-pem-h) — 8
- [`./fio-stl/159 argon2.h`](#fio-stl-159-argon2-h) — 6
- [`./fio-stl/159 lyra2.h`](#fio-stl-159-lyra2-h) — 5
- [`./fio-stl/159 otp.h`](#fio-stl-159-otp-h) — 7
- [`./fio-stl/159 secret.h`](#fio-stl-159-secret-h) — 5
- [`./fio-stl/162 brotli.h`](#fio-stl-162-brotli-h) — 4
- [`./fio-stl/162 deflate.h`](#fio-stl-162-deflate-h) — 11
- [`./fio-stl/190 tls13.h`](#fio-stl-190-tls13-h) — 60
- [`./fio-stl/201 string.h`](#fio-stl-201-string-h) — 47
- [`./fio-stl/202 array.h`](#fio-stl-202-array-h) — 30
- [`./fio-stl/210 map.h`](#fio-stl-210-map-h) — 30
- [`./fio-stl/210 map2.h`](#fio-stl-210-map2-h) — 3
- [`./fio-stl/249 reference counter.h`](#fio-stl-249-reference-counter-h) — 17
- [`./fio-stl/250 fiobj.h`](#fio-stl-250-fiobj-h) — 66
- [`./fio-stl/401 io api.h`](#fio-stl-401-io-api-h) — 102
- [`./fio-stl/404 ipc.h`](#fio-stl-404-ipc-h) — 41
- [`./fio-stl/405 tls13.h`](#fio-stl-405-tls13-h) — 1
- [`./fio-stl/420 pubsub.h`](#fio-stl-420-pubsub-h) — 26
- [`./fio-stl/422 redis.h`](#fio-stl-422-redis-h) — 7
- [`./fio-stl/431 http handle.h`](#fio-stl-431-http-handle-h) — 118
- [`./fio-stl/431 http1 parser.h`](#fio-stl-431-http1-parser-h) — 3
- [`./fio-stl/431 websocket parser.h`](#fio-stl-431-websocket-parser-h) — 29
- [`./fio-stl/439 http.h`](#fio-stl-439-http-h) — 30

-----------------------------------------------------

## <a id="fio-stl-000-copyright-h"></a> `./fio-stl/000 copyright.h`

1 public symbols.

### Macros

#### `FIO_INCLUDE_FILE`

```c
#define FIO_INCLUDE_FILE "fio-stl.h"
```

Persists the include file name (single file vs. folder)

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-000-core-h"></a> `./fio-stl/000 core.h`

1758 public symbols.

### Definition / Code Generation Macros

#### `FIO_DEF_GET_FUNC_DEC`

```c
#define FIO_DEF_GET_FUNC_DEC(static, namespace, T_type, F_type, field_name)   \
  /** Returns current value of property within the struct / union. */   \
  static F_type FIO_NAME(namespace, field_name)(T_type * o);
```

Declares (names) a `get` function for a field within a struct / union.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_DEF_GET_FUNC`

```c
#define FIO_DEF_GET_FUNC(static, namespace, T_type, F_type, field_name)   \
  /** Returns current value of property within the struct / union. */   \
  static F_type FIO_NAME(namespace, field_name)(T_type * o) {   \
    FIO_ASSERT_DEBUG(o, "NULL " FIO_MACRO2STR(namespace) " pointer @ `get`!");   \
    return o->field_name;   \
  }
```

Defines a `get` function for a field within a struct / union.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_DEF_SET_FUNC_DEC`

```c
#define FIO_DEF_SET_FUNC_DEC(static, namespace, T_type, F_type, F_name)   \
  /** Sets a new value, returning the old one */   \
  static F_type FIO_NAME(FIO_NAME(namespace, F_name), set)(T_type * o,   \
                                                           F_type new_value);
```

Declares (names) a `set` function for a field within a struct / union.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_DEF_SET_FUNC`

```c
#define FIO_DEF_SET_FUNC(static, namespace, T_type, F_type, F_name, on_set)   \
  /** Sets a new value, returning the old one */   \
  static F_type FIO_NAME(FIO_NAME(namespace, F_name), set)(T_type * o,   \
                                                           F_type new_value) {   \
    FIO_ASSERT_DEBUG(o, "NULL " FIO_MACRO2STR(namespace) " pointer @ `set`!");   \
    F_type old_value = o->F_name;   \
    o->F_name = new_value;   \
    on_set(o);   \
    return old_value;   \
  }
```

Defines a `set` function for a field within a struct / union.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_DEF_GETSET_FUNC`

```c
#define FIO_DEF_GETSET_FUNC(static, namespace, T_type, F_type, F_name, on_set)   \
  FIO_DEF_GET_FUNC(static, namespace, T_type, F_type, F_name)   \
  FIO_DEF_SET_FUNC(static, namespace, T_type, F_type, F_name, on_set)
```

Defines `get/set` functions for a field within a struct / union.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_DEF_GETSET_FUNC_DEC`

```c
#define FIO_DEF_GETSET_FUNC_DEC(static, namespace, T_type, F_type, F_name)   \
  FIO_DEF_GET_FUNC_DEC(static, namespace, T_type, F_type, F_name)   \
  FIO_DEF_SET_FUNC_DEC(static, namespace, T_type, F_type, F_name)
```

Declares (names) `get/set` functions for a field within a struct / union.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_IFUNC_DEF_GET`

```c
#define FIO_IFUNC_DEF_GET(namespace, T_type, F_type, field_name)   \
  FIO_DEF_GET_FUNC(FIO_IFUNC, namespace, T_type, F_type, field_name)
```

Defines a `get` function for a field within a struct / union.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_IFUNC_DEF_SET`

```c
#define FIO_IFUNC_DEF_SET(namespace, T_type, F_type, F_name, on_set)   \
  FIO_DEF_SET_FUNC(FIO_IFUNC, namespace, T_type, F_type, F_name, on_set)
```

Defines a `set` function for a field within a struct / union.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_IFUNC_DEF_GETSET`

```c
#define FIO_IFUNC_DEF_GETSET(namespace, T_type, F_type, F_name, on_set)   \
  FIO_IFUNC_DEF_GET(namespace, T_type, F_type, F_name)   \
  FIO_IFUNC_DEF_SET(namespace, T_type, F_type, F_name, on_set)
```

Defines `get/set` functions for a field within a struct / union.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_STATIC_ALLOC_DEF`

```c
#define FIO_STATIC_ALLOC_DEF(name,   \
                             type_T,   \
                             size_per_allocation,   \
                             allocations_per_thread)   \
  FIO_SFUNC FIO_WARN_UNUSED type_T *name(size_t count) {   \
    static type_T name##buffer[sizeof(type_T) *   \
                               FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX *   \
                               size_per_allocation * allocations_per_thread];   \
    static size_t pos;   \
    size_t at = fio_atomic_add(&pos, count);   \
    at %= FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX * allocations_per_thread;   \
    return (at * size_per_allocation) + name##buffer;   \
  }
```

Defines a simple (almost naive) static memory allocator named `name`.

This defines a memory allocation function named `name` that accepts a
single input `count` and returns a `type_T` pointer (`type_T *`) containing
`sizeof(type_T) * count * size_per_allocation` in correct memory alignment.

```c
static type_T *name(size_t allocation_count);
```

That memory is statically allocated, allowing it be returned and never
needing to be freed.

The functions can safely allocate the following number of bytes before
the function returns the same memory block to another caller:

```c
FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX * allocations_per_thread *
        sizeof(type_T) * size_per_allocation
```

Example use:

```c
// defined a static allocator for 32 byte long strings
FIO_STATIC_ALLOC_DEF(numer2hex_allocator, char, 19, 1);
// a function that returns an unsigned number as a 16 digit hex string
char * ntos16(uint16_t n) {
  char * n = numer2hex_allocator(1);
  n[0] = '0'; n[1] = 'x';
  fio_ltoa16u(n+2, n, 16);
  n[18] = 0;
  return n;
}
```

A similar approach is use by `fiobj_num2cstr` in order to provide temporary
conversions of FIOBJ to a C String that doesn't require memory management.

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_UXXX_X64_DEF`

```c
#define FIO_UXXX_X64_DEF(name, bits)   \
  uint64x2_t name[(bits / 128) + (bits < 128)]
```

Defines a `bits` long vector using unsigned 64bit words

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_UXXX_X32_DEF`

```c
#define FIO_UXXX_X32_DEF(name, bits)   \
  uint32x4_t name[(bits / 128) + (bits < 128)]
```

Defines a `bits` long vector using unsigned 32bit words

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_UXXX_X16_DEF`

```c
#define FIO_UXXX_X16_DEF(name, bits)   \
  uint16x8_t name[(bits / 128) + (bits < 128)]
```

Defines a `bits` long vector using unsigned 16bit words

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

#### `FIO_UXXX_X8_DEF`

```c
#define FIO_UXXX_X8_DEF(name, bits) uint8x16_t name[(bits / 128) + (bits < 128)]
```

Defines a `bits` long vector using unsigned 8bit words

_Note:_  this MACRO defines or declares code.

_Symbol type:_ `macro`

### Macros

#### `FIO_NOOP`

```c
#define FIO_NOOP
```

An empty macro, adding white space. Used to avoid function like macros.

_Symbol type:_ `macro`

#### `FIO_NOOP_FN`

```c
#define FIO_NOOP_FN(...)
```

A no-op macro that takes arguments. i.e., for use in `FIO_DEF_SET_FUNC`

_Symbol type:_ `macro`

#### `FIO_NOOP_FN_NAME`

```c
#define FIO_NOOP_FN_NAME (void)
```

Macro for a No-Op function name (void).

_Symbol type:_ `macro`

#### `FIO_VERSION_MAJOR`

```c
#define FIO_VERSION_MAJOR 0
```

MAJOR version: API/ABI breaking changes.

_Symbol type:_ `macro`

#### `FIO_VERSION_MINOR`

```c
#define FIO_VERSION_MINOR 8
```

MINOR version: Deprecation, or significant features added. May break ABI.

_Symbol type:_ `macro`

#### `FIO_VERSION_PATCH`

```c
#define FIO_VERSION_PATCH 0
```

PATCH version: Bug fixes, minor features may be added.

_Symbol type:_ `macro`

#### `FIO_VERSION_BUILD`

```c
#define FIO_VERSION_BUILD "rc.02"
```

Build version: optional build info (string), i.e. "beta.02"

_Symbol type:_ `macro`

#### `FIO_VERSION_STRING`

```c
#define FIO_VERSION_STRING   \
  FIO_MACRO2STR(FIO_VERSION_MAJOR)   \
  "." FIO_MACRO2STR(FIO_VERSION_MINOR) "." FIO_MACRO2STR(   \
      FIO_VERSION_PATCH) "-" FIO_VERSION_BUILD
```

Version as a String literal (MACRO).

_Symbol type:_ `macro`

#### `FIO_LEAK_COUNTER`

```c
#define FIO_LEAK_COUNTER 1
```

Enables memory leak detection. Disable by setting this to zero.

_Symbol type:_ `macro`

#### `FIO_USE_THREAD_MUTEX`

```c
#define FIO_USE_THREAD_MUTEX 0
```

Selects between facio.io's spinlocks (false) and OS mutexes (true)

_Symbol type:_ `macro`

#### `FIO_UNALIGNED_ACCESS`

```c
#define FIO_UNALIGNED_ACCESS 1
```

Allows facil.io to attempt unaligned memory access on *some* CPU systems.

_Symbol type:_ `macro`

#### `FIO_LIMIT_INTRINSIC_BUFFER`

```c
#define FIO_LIMIT_INTRINSIC_BUFFER 1
```

Limits register consumption on some pseudo-intrinsics, using more loops

_Symbol type:_ `macro`

#### `FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT`

```c
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT 1
```

Memory allocations should be secure by default (facil.io allocators only)

_Symbol type:_ `macro`

#### `FIO_MEM_PAGE_SIZE_LOG`

```c
#define FIO_MEM_PAGE_SIZE_LOG 12 /* assumes 4096 bytes per page */
```

Compile-time memory page size log (12 == 4096 bytes).

_Symbol type:_ `macro`

#### `FIO_MAP_WARNING_BITSIZE`

```c
#define FIO_MAP_WARNING_BITSIZE ((size_t)24)
```

iMap and Map allocation size warning (log2), 24 == 16Mb.

_Symbol type:_ `macro`

#### `FIO_ALIGN`

```c
#define FIO_ALIGN(bytes) __attribute__((aligned(bytes)))
```

Sets alignment is supported by compiler.

_Symbol type:_ `macro`

#### `FIO_COMPILER_GUARD`

```c
#define FIO_COMPILER_GUARD __asm__ volatile("" ::: "memory")
```

Clobber CPU registers and prevent compiler reordering optimizations.

_Symbol type:_ `macro`

#### `FIO_COMPILER_GUARD_INSTRUCTION`

```c
#define FIO_COMPILER_GUARD_INSTRUCTION __asm__ volatile("" :::)
```

Prevent compiler reordering optimizations.

_Symbol type:_ `macro`

#### `FIO_UNALIGNED_MEMORY_ACCESS_ENABLED`

```c
#define FIO_UNALIGNED_MEMORY_ACCESS_ENABLED 1
```

True when unaligned memory is allowed.

_Symbol type:_ `macro`

#### `FIO_OS_POSIX`

```c
#define FIO_OS_POSIX 0
```

Always defined, true on POSIX systems.

_Symbol type:_ `macro`

#### `FIO_OS_WIN`

```c
#define FIO_OS_WIN 0
```

Always defined, true on Windows systems.

_Symbol type:_ `macro`

#### `FIO_KILL_SELF`

```c
#define FIO_KILL_SELF() GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0)
```

Sends a signal to kill the current process (emulates CTRL + C).

_Symbol type:_ `macro`

#### `FIO_HAVE_UNIX_TOOLS`

```c
#define FIO_HAVE_UNIX_TOOLS    2
```

True if POSIX headers are available.

_Symbol type:_ `macro`

#### `FIO_SIZE_T_ERROR`

```c
#define FIO_SIZE_T_ERROR (~(size_t)0)
```

The `-1` value for a `size_t` type, often used as an error value.

_Symbol type:_ `macro`

#### `FIO_MAYBE_UNUSED`

```c
#define FIO_MAYBE_UNUSED __attribute__((unused))
```

Marks a variable / function as possibly unused (if compiler supported).

_Symbol type:_ `macro`

#### `FIO_SFUNC`

```c
#define FIO_SFUNC static FIO_MAYBE_UNUSED
```

static, possibly unused, function.

_Symbol type:_ `macro`

#### `FIO_IFUNC`

```c
#define FIO_IFUNC FIO_SFUNC inline
```

Marks a function as `static`, `inline` and possibly unused.

_Symbol type:_ `macro`

#### `FIO_WARN_UNUSED`

```c
#define FIO_WARN_UNUSED __attribute__((warn_unused_result))
```

Attribute for functions whose return value should not be ignored.

_Symbol type:_ `macro`

#### `FIO_MIFN`

```c
#define FIO_MIFN FIO_IFUNC FIO_WARN_UNUSED
```

Marks a function as inline with warn_unused_result (for math functions).

_Symbol type:_ `macro`

#### `FIO_CONST`

```c
#define FIO_CONST __attribute__((const))
```

Marks a function as const (no side effects, result depends only on args)

_Symbol type:_ `macro`

#### `FIO_PURE`

```c
#define FIO_PURE __attribute__((pure))
```

Marks a function as pure (no side effects, may read global memory)

_Symbol type:_ `macro`

#### `FIO_WEAK`

```c
#define FIO_WEAK __attribute__((weak))
```

Marks a function as weak

_Symbol type:_ `macro`

#### `FIO_WEAK_VAR`

```c
#define FIO_WEAK_VAR __declspec(selectany)
```

Marks a global variable as weak

_Symbol type:_ `macro`

#### `FIO_CONSTRUCTOR`

```c
#define FIO_CONSTRUCTOR(fname)   \
  static __attribute__((constructor)) void fname(void)
```

Marks a function as a constructor - if supported.

_Symbol type:_ `macro`

#### `FIO_DESTRUCTOR`

```c
#define FIO_DESTRUCTOR(fname)   \
  static __attribute__((destructor)) void fname(void)
```

Marks a function as a destructor - if supported. Consider using atexit()

_Symbol type:_ `macro`

#### `FIO_LIKELY`

```c
#define FIO_LIKELY(cond) __builtin_expect((cond), 1)
```

Provides a compiler hint for value likelihood, if possible.

_Symbol type:_ `macro`

#### `FIO_UNLIKELY`

```c
#define FIO_UNLIKELY(cond) __builtin_expect((cond), 0)
```

Provides a compiler hint for value likelihood, if possible.

_Symbol type:_ `macro`

#### `FIO_PREFETCH`

```c
#define FIO_PREFETCH(ptr) __builtin_prefetch((ptr), 0, 3)
```

Prefetch data for reading into all cache levels.

_Symbol type:_ `macro`

#### `FIO_PREFETCH_W`

```c
#define FIO_PREFETCH_W(ptr) __builtin_prefetch((ptr), 1, 3)
```

Prefetch data for writing (exclusive cache line ownership).

_Symbol type:_ `macro`

#### `FIO_PREFETCH_NT`

```c
#define FIO_PREFETCH_NT(ptr) __builtin_prefetch((ptr), 0, 0)
```

Prefetch non-temporal (streaming data, minimize cache pollution).

_Symbol type:_ `macro`

#### `FIO_MACRO2STR`

```c
#define FIO_MACRO2STR(macro) FIO___MACRO2STR_STEP2(macro)
```

Converts a macro's content to a string literal.

_Symbol type:_ `macro`

#### `FIO_NAME`

```c
#define FIO_NAME(prefix, postfix)   \
  FIO___NAME_FROM_MACRO_STEP1(prefix, postfix, _)
```

Used for naming functions and variables resulting in: prefix_postfix

_Symbol type:_ `macro`

#### `FIO_NAME2`

```c
#define FIO_NAME2(prefix, postfix)   \
  FIO___NAME_FROM_MACRO_STEP1(prefix, postfix, 2)
```

Sets naming convention for conversion functions, i.e.: foo2bar

_Symbol type:_ `macro`

#### `FIO_NAME_BL`

```c
#define FIO_NAME_BL(prefix, postfix)   \
  FIO___NAME_FROM_MACRO_STEP1(prefix, postfix, _is_)
```

Sets naming convention for boolean testing functions, i.e.: foo_is_true

_Symbol type:_ `macro`

#### `FIO_NAME_TEST`

```c
#define FIO_NAME_TEST(prefix, postfix)   \
  FIO_NAME(fio___test, FIO_NAME(prefix, postfix))
```

Used internally to name test functions.

_Symbol type:_ `macro`

#### `FIO_PTR_MATH_LMASK`

```c
#define FIO_PTR_MATH_LMASK(T_type, ptr, bits)   \
  ((T_type *)(((uintptr_t)(ptr)) & (((uintptr_t)1ULL << (bits)) - 1)))
```

Masks a pointer's left-most bits, returning the right bits.

_Symbol type:_ `macro`

#### `FIO_PTR_MATH_RMASK`

```c
#define FIO_PTR_MATH_RMASK(T_type, ptr, bits)   \
  ((T_type *)(((uintptr_t)(ptr)) & ((~(uintptr_t)0ULL) << (bits))))
```

Masks a pointer's right-most bits, returning the left bits.

_Symbol type:_ `macro`

#### `FIO_PTR_MATH_ADD`

```c
#define FIO_PTR_MATH_ADD(T_type, ptr, offset)   \
  ((T_type *)((uintptr_t)(ptr) + (uintptr_t)(offset)))
```

Add offset bytes to pointer, updating the pointer's type.

_Symbol type:_ `macro`

#### `FIO_PTR_MATH_SUB`

```c
#define FIO_PTR_MATH_SUB(T_type, ptr, offset)   \
  ((T_type *)((uintptr_t)(ptr) - (uintptr_t)(offset)))
```

Subtract X bytes from pointer, updating the pointer's type.

_Symbol type:_ `macro`

#### `FIO_PTR_FIELD_OFFSET`

```c
#define FIO_PTR_FIELD_OFFSET(T_type, field)   \
  ((uintptr_t)((&((T_type *)0xFF00)->field)) - 0xFF00)
```

Find the root object (of a struct) from it's field (with sanitizer fix).

_Symbol type:_ `macro`

#### `FIO_PTR_FROM_FIELD`

```c
#define FIO_PTR_FROM_FIELD(T_type, field, ptr)   \
  FIO_PTR_MATH_SUB(T_type, ptr, FIO_PTR_FIELD_OFFSET(T_type, field))
```

Find the root object (of a struct) from it's field (with sanitizer fix).

_Symbol type:_ `macro`

#### `FIO_THREAD_WAIT`

```c
#define FIO_THREAD_WAIT(nano_sec)   \
  do {   \
    Sleep(((nano_sec) / 1000000) ? ((nano_sec) / 1000000) : 1);   \
  } while (0)
```

Calls NtDelayExecution with the requested nano-second count.

_Symbol type:_ `macro`

#### `FIO_THREAD_YIELD`

```c
#define FIO_THREAD_YIELD() __asm__ __volatile__("pause" ::: "memory")
```

Yields the thread, hinting to the processor about spinlock loop.

_Symbol type:_ `macro`

#### `FIO_THREAD_RESCHEDULE`

```c
#define FIO_THREAD_RESCHEDULE() FIO_THREAD_WAIT(4)
```

Reschedules the thread by calling nanosleeps for nano-seconds.

In practice, the thread will probably sleep for 60ns or more.

Seems to be faster then thread_yield, perhaps it prevents de-prioritization
of the thread.

_Symbol type:_ `macro`

#### `FIO_LOCK_INIT`

```c
#define FIO_LOCK_INIT 0
```

Initialization value for a spin lock.

_Symbol type:_ `macro`

#### `FIO_LOCK_SUBLOCK`

```c
#define FIO_LOCK_SUBLOCK(sub) ((uint8_t)(1U) << ((sub)&7))
```

A numbered sub-lock index, must be less than 8. Zero == the master lock.

_Symbol type:_ `macro`

#### `FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX`

```c
#define FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX 256
```

The multiplier used to set the maximum number of safe concurrent calls.

_Symbol type:_ `macro`

#### `FIO_LOG_LEVEL_NONE`

```c
#define FIO_LOG_LEVEL_NONE 0
```

Logging level of zero (no logging).

_Symbol type:_ `macro`

#### `FIO_LOG_LEVEL_FATAL`

```c
#define FIO_LOG_LEVEL_FATAL 1
```

Log fatal errors.

_Symbol type:_ `macro`

#### `FIO_LOG_LEVEL_ERROR`

```c
#define FIO_LOG_LEVEL_ERROR 2
```

Log errors and fatal errors.

_Symbol type:_ `macro`

#### `FIO_LOG_LEVEL_WARNING`

```c
#define FIO_LOG_LEVEL_WARNING 3
```

Log warnings, errors and fatal errors.

_Symbol type:_ `macro`

#### `FIO_LOG_LEVEL_INFO`

```c
#define FIO_LOG_LEVEL_INFO 4
```

Log every message (info, warnings, errors and fatal errors).

_Symbol type:_ `macro`

#### `FIO_LOG_LEVEL_DEBUG`

```c
#define FIO_LOG_LEVEL_DEBUG 5
```

Log everything, including debug messages.

_Symbol type:_ `macro`

#### `FIO_LOG_LEVEL_SET`

```c
#define FIO_LOG_LEVEL_SET(new_level) (0)
```

Sets the Logging Level

_Symbol type:_ `macro`

#### `FIO_LOG_LEVEL_GET`

```c
#define FIO_LOG_LEVEL_GET() (0)
```

Returns the Logging Level

_Symbol type:_ `macro`

#### `FIO_LOG_WRITE`

```c
#define FIO_LOG_WRITE(...)    FIO_LOG2STDERR("(" FIO___FILE__ ":" FIO_MACRO2STR(__LINE__) "): " __VA_ARGS__)
```

Writes a message to the log, including the file name and line number.

_Symbol type:_ `macro`

#### `FIO_LOG_FATAL`

```c
#define FIO_LOG_FATAL(...)    FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_FATAL, "\x1B[1m\x1B[7mFATAL:\x1B[0m    " __VA_ARGS__)
```

Writes a Fatal message to the log.

_Symbol type:_ `macro`

#### `FIO_LOG_ERROR`

```c
#define FIO_LOG_ERROR(...)    FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_ERROR, "\x1B[1mERROR:\x1B[0m    " __VA_ARGS__)
```

Writes a Error message to the log.

_Symbol type:_ `macro`

#### `FIO_LOG_SECURITY`

```c
#define FIO_LOG_SECURITY(...) FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_ERROR, "\x1B[1mSECURITY:\x1B[0m " __VA_ARGS__)
```

Writes a Security message to the log.

_Symbol type:_ `macro`

#### `FIO_LOG_WARNING`

```c
#define FIO_LOG_WARNING(...)  FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_WARNING, "\x1B[2mWARNING:\x1B[0m  " __VA_ARGS__)
```

Writes a Warning message to the log.

_Symbol type:_ `macro`

#### `FIO_LOG_INFO`

```c
#define FIO_LOG_INFO(...)     FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_INFO, "INFO:     " __VA_ARGS__)
```

Writes a Info message to the log.

_Symbol type:_ `macro`

#### `FIO_LOG_DEBUG`

```c
#define FIO_LOG_DEBUG(...)    FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_DEBUG,"DEBUG:    (" FIO___FILE__ ":" FIO_MACRO2STR(__LINE__) ") " __VA_ARGS__)
```

Writes a Debug message to the log, including the file name and line number.

_Symbol type:_ `macro`

#### `FIO_LOG_DEBUG2`

```c
#define FIO_LOG_DEBUG2(...)   FIO___LOG_PRINT_LEVEL(FIO_LOG_LEVEL_DEBUG, "DEBUG:    " __VA_ARGS__)
```

Writes a Debug message to the log.

_Symbol type:_ `macro`

#### `FIO_LOG_DDEBUG`

```c
#define FIO_LOG_DDEBUG(...) FIO_LOG_DEBUG(__VA_ARGS__)
```

Writes a Debug message to the log only if `DEBUG` was defined.

_Symbol type:_ `macro`

#### `FIO_LOG_DDEBUG2`

```c
#define FIO_LOG_DDEBUG2(...) FIO_LOG_DEBUG2(__VA_ARGS__)
```

Writes a Debug2 message to the log only if `DEBUG` was defined.

_Symbol type:_ `macro`

#### `FIO_LOG_DERROR`

```c
#define FIO_LOG_DERROR(...) FIO_LOG_ERROR(__VA_ARGS__)
```

Writes a Error message to the log only if `DEBUG` was defined.

_Symbol type:_ `macro`

#### `FIO_LOG_DSECURITY`

```c
#define FIO_LOG_DSECURITY(...) FIO_LOG_SECURITY(__VA_ARGS__)
```

Writes a Security message to the log only if `DEBUG` was defined.

_Symbol type:_ `macro`

#### `FIO_LOG_DWARNING`

```c
#define FIO_LOG_DWARNING(...) FIO_LOG_WARNING(__VA_ARGS__)
```

Writes a Warning message to the log only if `DEBUG` was defined.

_Symbol type:_ `macro`

#### `FIO_LOG_DINFO`

```c
#define FIO_LOG_DINFO(...)            FIO_LOG_INFO(__VA_ARGS__)
```

Writes a Info message to the log only if `DEBUG` was defined.

_Symbol type:_ `macro`

#### `FIO_LOG_LENGTH_LIMIT`

```c
#define FIO_LOG_LENGTH_LIMIT 1024
```

Defines a point at which logging truncates (limits stack memory use)

_Symbol type:_ `macro`

#### `FIO_LOG2STDERR`

```c
#define FIO_LOG2STDERR(...)
```

Prints to STDERR, attempting to use only stack allocated memory.

_Symbol type:_ `macro`

#### `FIO_ASSERT`

```c
#define FIO_ASSERT(cond, ...)   \
  do {   \
    if (FIO_UNLIKELY(!(cond))) {   \
      FIO_LOG_FATAL(__VA_ARGS__);   \
      FIO_LOG_FATAL("     errno(%d): %s\n", errno, strerror(errno));   \
      FIO___ASSERT_PERFORM_SIGNAL();   \
      exit(-1);   \
    }   \
  } while (0)
```

Asserts a condition is true, or kills the application using SIGINT.

_Symbol type:_ `macro`

#### `FIO_ASSERT_ALLOC`

```c
#define FIO_ASSERT_ALLOC(ptr) FIO_ASSERT((ptr), "memory allocation failed.")
```

Tests for an allocation failure. The behavior can be overridden.

_Symbol type:_ `macro`

#### `FIO_ASSERT_DEBUG`

```c
#define FIO_ASSERT_DEBUG(cond, ...)   \
  do {   \
    if (!(cond)) {   \
      FIO_LOG_FATAL("(" FIO___FILE__   \
                    ":" FIO_MACRO2STR(__LINE__) ") " __VA_ARGS__);   \
      FIO_LOG_FATAL("     errno(%d): %s\n", errno, strerror(errno));   \
      FIO___ASSERT_PERFORM_SIGNAL();   \
      exit(-1);   \
    }   \
  } while (0)
```

If `DEBUG` is defined, raises SIGINT if assertion fails, otherwise NOOP.

_Symbol type:_ `macro`

#### `FIO_ASSERT_STATIC`

```c
#define FIO_ASSERT_STATIC(cond, msg) _Static_assert((cond), msg)
```

Perform a static (build time) assertion.

_Symbol type:_ `macro`

#### `FIO_MEM_STACK_WIPE`

```c
#define FIO_MEM_STACK_WIPE(pages)   \
  do {   \
    volatile char stack_mem[(pages) << 12] = {0};   \
    (void)stack_mem;   \
  } while (0)
```

Places `pages` x 4096 bytes of zero (`0`) on the stack.

_Symbol type:_ `macro`

#### `FIO_MEMCPY`

```c
#define FIO_MEMCPY fio_memcpy
```

Use this macro instead of `memcpy`, for easy implementation overriding.

_Symbol type:_ `macro`

#### `FIO_MEMMOVE`

```c
#define FIO_MEMMOVE fio_memcpy
```

Use this macro instead of `memmove`, for easy implementation overriding.

_Symbol type:_ `macro`

#### `FIO_MEMCMP`

```c
#define FIO_MEMCMP fio_memcmp
```

Use this macro instead of `memcmp`, for easy implementation overriding.

_Symbol type:_ `macro`

#### `FIO_MEMCHR`

```c
#define FIO_MEMCHR fio_memchr
```

Use this macro instead of `memchr`, for easy implementation overriding.

_Symbol type:_ `macro`

#### `FIO_MEMSET`

```c
#define FIO_MEMSET fio_memset
```

Use this macro instead of `memset`, for easy implementation overriding.

_Symbol type:_ `macro`

#### `FIO_STRLEN`

```c
#define FIO_STRLEN fio_strlen
```

Use this macro instead of `strlen`, for easy implementation overriding.

_Symbol type:_ `macro`

#### `FIO_FOR`

```c
#define FIO_FOR(i, count) for (size_t i = 0; i < (count); ++i)
```

Helper for simple `for` loops, where `i` is the variable name to use.

_Symbol type:_ `macro`

#### `FIO_FOR_UNROLL`

```c
#define FIO_FOR_UNROLL(iterations, size_of_loop, i, action)   \
  do {   \
    size_t i = 0;   \
    const size_t fio___unroll_remainder__ =   \
        ((iterations) & ((FIO___SIMD_BYTES / (size_of_loop)) - 1));   \
    /* handle odd length vectors, not multiples of FIO___LOG2V */   \
    if (fio___unroll_remainder__ && ((iterations) + 1))   \
      for (; i < fio___unroll_remainder__; ++i) {   \
        action;   \
      }   \
    if (iterations)   \
      for (; !((iterations) + 1) || (i < (iterations));)   \
        for (size_t j__loop__ = 0;   \
             j__loop__ < (FIO___SIMD_BYTES / (size_of_loop));   \
             ++j__loop__, ++i) /* dear compiler, please vectorize */   \
        {   \
          action;   \
        }   \
  } while (0)
```

Unrolled `for` loop - separates for loops to make it easier for the compiler
to optimize.

**Parameters:**
- `iterations` - the number of loop iterations to perform
- `size_of_loop` - the number of bytes consumed by each `action`
- `i` - the loop index variable name to use (accessible by `action`)
- `action` - an action to be performed each iteration (can be a macro)

_Symbol type:_ `macro`

#### `FIO_LEAK_COUNTER_SKIP_EXIT`

```c
#define FIO_LEAK_COUNTER_SKIP_EXIT 0
```

If set, no `FIO_CALL_AFTER_EXIT` leak reporting event is registered.

_Symbol type:_ `macro`

#### `FIO_SHIFT_FORWARDS`

```c
#define FIO_SHIFT_FORWARDS(i, bits) ((i) >> (bits))
```

An endianess dependent shift operation, moves bits forwards.

_Symbol type:_ `macro`

#### `FIO_SHIFT_BACKWARDS`

```c
#define FIO_SHIFT_BACKWARDS(i, bits) ((i) << (bits))
```

An endianess dependent shift operation, moves bits backwards.

_Symbol type:_ `macro`

#### `FIO_LIST_NODE`

```c
#define FIO_LIST_NODE fio_list_node_s
```

A linked list node type

_Symbol type:_ `macro`

#### `FIO_LIST_HEAD`

```c
#define FIO_LIST_HEAD fio_list_node_s
```

A linked list head type

_Symbol type:_ `macro`

#### `FIO_LIST_INIT`

```c
#define FIO_LIST_INIT(obj)   \
  (fio_list_node_s) { .next = &(obj), .prev = &(obj) }
```

Allows initialization of FIO_LIST_HEAD objects.

_Symbol type:_ `macro`

#### `FIO_LIST_EACH`

```c
#define FIO_LIST_EACH(type, node_name, head, pos)   \
  for (type *pos = FIO_PTR_FROM_FIELD(type, node_name, (head)->next),   \
            *next____p_ls_##pos =   \
                FIO_PTR_FROM_FIELD(type, node_name, (head)->next->next);   \
       pos != FIO_PTR_FROM_FIELD(type, node_name, (head));   \
       (pos = next____p_ls_##pos),   \
            (next____p_ls_##pos =   \
                 FIO_PTR_FROM_FIELD(type,   \
                                    node_name,   \
                                    next____p_ls_##pos->node_name.next)))
```

Loops through every node in the linked list except the head.

_Symbol type:_ `macro`

#### `FIO_LIST_EACH_REVERSED`

```c
#define FIO_LIST_EACH_REVERSED(type, node_name, head, pos)   \
  for (type *pos = FIO_PTR_FROM_FIELD(type, node_name, (head)->prev),   \
            *next____p_ls_##pos =   \
                FIO_PTR_FROM_FIELD(type, node_name, (head)->next->prev);   \
       pos != FIO_PTR_FROM_FIELD(type, node_name, (head));   \
       (pos = next____p_ls_##pos),   \
            (next____p_ls_##pos =   \
                 FIO_PTR_FROM_FIELD(type,   \
                                    node_name,   \
                                    next____p_ls_##pos->node_name.prev)))
```

Loops through every node in the linked list except the head.

_Symbol type:_ `macro`

#### `FIO_LIST_PUSH`

```c
#define FIO_LIST_PUSH(head, n)   \
  do {   \
    (n)->prev = (head)->prev;   \
    (n)->next = (head);   \
    (head)->prev->next = (n);   \
    (head)->prev = (n);   \
  } while (0)
```

UNSAFE macro for pushing a node to a list.

_Symbol type:_ `macro`

#### `FIO_LIST_REMOVE`

```c
#define FIO_LIST_REMOVE(n)   \
  do {   \
    (n)->prev->next = (n)->next;   \
    (n)->next->prev = (n)->prev;   \
  } while (0)
```

UNSAFE macro for removing a node from a list.

_Symbol type:_ `macro`

#### `FIO_LIST_REMOVE_RESET`

```c
#define FIO_LIST_REMOVE_RESET(n)   \
  do {   \
    (n)->prev->next = (n)->next;   \
    (n)->next->prev = (n)->prev;   \
    (n)->next = (n)->prev = (n);   \
  } while (0)
```

UNSAFE macro for removing a node from a list. Resets node data.

_Symbol type:_ `macro`

#### `FIO_LIST_POP`

```c
#define FIO_LIST_POP(type, node_name, dest_ptr, head)   \
  do {   \
    (dest_ptr) = FIO_PTR_FROM_FIELD(type, node_name, ((head)->next));   \
    FIO_LIST_REMOVE(&(dest_ptr)->node_name);   \
  } while (0)
```

UNSAFE macro for popping a node to a list.

_Symbol type:_ `macro`

#### `FIO_LIST_IS_EMPTY`

```c
#define FIO_LIST_IS_EMPTY(head)   \
  ((!(head)) || ((!(head)->next) | ((head)->next == (head))))
```

UNSAFE macro for testing if a list is empty.

_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST32_NODE`

```c
#define FIO_INDEXED_LIST32_NODE fio_index32_node_s
```

A 32 bit indexed linked list node type

_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST32_HEAD`

```c
#define FIO_INDEXED_LIST32_HEAD uint32_t
```



_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST16_NODE`

```c
#define FIO_INDEXED_LIST16_NODE fio_index16_node_s
```

A 16 bit indexed linked list node type

_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST16_HEAD`

```c
#define FIO_INDEXED_LIST16_HEAD uint16_t
```



_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST8_NODE`

```c
#define FIO_INDEXED_LIST8_NODE fio_index8_node_s
```

An 8 bit indexed linked list node type

_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST8_HEAD`

```c
#define FIO_INDEXED_LIST8_HEAD uint8_t
```



_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST_PUSH`

```c
#define FIO_INDEXED_LIST_PUSH(root, node_name, head, i)   \
  do {   \
    register const size_t n__ = (i);   \
    (root)[n__].node_name.prev = (root)[(head)].node_name.prev;   \
    (root)[n__].node_name.next = (head);   \
    (root)[(root)[(head)].node_name.prev].node_name.next = (n__);   \
    (root)[(head)].node_name.prev = (n__);   \
  } while (0)
```

UNSAFE macro for pushing a node to a list.

_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST_UNSHIFT`

```c
#define FIO_INDEXED_LIST_UNSHIFT(root, node_name, head, i)   \
  do {   \
    register const size_t n__ = (i);   \
    (root)[n__].node_name.next = (root)[(head)].node_name.next;   \
    (root)[n__].node_name.prev = (head);   \
    (root)[(root)[(head)].node_name.next].node_name.prev = (n__);   \
    (root)[(head)].node_name.next = (n__);   \
    (head) = (n__);   \
  } while (0)
```

UNSAFE macro for adding a node to the begging of the list.

_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST_REMOVE`

```c
#define FIO_INDEXED_LIST_REMOVE(root, node_name, i)   \
  do {   \
    register const size_t n__ = (i);   \
    (root)[(root)[n__].node_name.prev].node_name.next =   \
        (root)[n__].node_name.next;   \
    (root)[(root)[n__].node_name.next].node_name.prev =   \
        (root)[n__].node_name.prev;   \
  } while (0)
```

UNSAFE macro for removing a node from a list.

_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST_REMOVE_RESET`

```c
#define FIO_INDEXED_LIST_REMOVE_RESET(root, node_name, i)   \
  do {   \
    register const size_t n__ = (i);   \
    (root)[(root)[n__].node_name.prev].node_name.next =   \
        (root)[n__].node_name.next;   \
    (root)[(root)[n__].node_name.next].node_name.prev =   \
        (root)[n__].node_name.prev;   \
    (root)[n__].node_name.next = (root)[n__].node_name.prev = (n__);   \
  } while (0)
```

UNSAFE macro for removing a node from a list. Resets node data.

_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST_EACH`

```c
#define FIO_INDEXED_LIST_EACH(root, node_name, head, pos)   \
  for (size_t pos = (head),   \
              stooper___hd = (head),   \
              stopper___ils___ = 0,   \
              pos##___nxt = (root)[(head)].node_name.next;   \
       !stopper___ils___;   \
       (stopper___ils___ = ((pos = pos##___nxt) == stooper___hd)),   \
              pos##___nxt = (root)[pos].node_name.next)
```

Loops through every index in the indexed list, assuming `head` is valid.

_Symbol type:_ `macro`

#### `FIO_INDEXED_LIST_EACH_REVERSED`

```c
#define FIO_INDEXED_LIST_EACH_REVERSED(root, node_name, head, pos)   \
  for (size_t pos = ((root)[(head)].node_name.prev),   \
              pos##___nxt =   \
                  ((root)[((root)[(head)].node_name.prev)].node_name.prev),   \
              stooper___hd = (head),   \
              stopper___ils___ = 0;   \
       !stopper___ils___;   \
       ((stopper___ils___ = (pos == stooper___hd)),   \
        (pos = pos##___nxt),   \
        (pos##___nxt = (root)[pos##___nxt].node_name.prev)))
```

Loops through every index in the indexed list, assuming `head` is valid.

_Symbol type:_ `macro`

#### `FIO_LROT`

```c
#define FIO_LROT(i, bits)   \
  (((i) << ((bits) & ((sizeof((i)) << 3) - 1))) |   \
   ((i) >> (((uint8_t)(-(bits))) & ((sizeof((i)) << 3) - 1))))
```

Left rotation for an unknown size element, inlined.

_Symbol type:_ `macro`

#### `FIO_RROT`

```c
#define FIO_RROT(i, bits)   \
  (((i) >> ((bits) & ((sizeof((i)) << 3) - 1))) |   \
   ((i) << (((uint8_t)(-(bits))) & ((sizeof((i)) << 3) - 1))))
```

Right rotation for an unknown size element, inlined.

_Symbol type:_ `macro`

#### `FIO_HAS_ZERO_BYTE64`

```c
#define FIO_HAS_ZERO_BYTE64(row)   \
  (((row)-UINT64_C(0x0101010101010101)) &   \
   ((~(row)) & UINT64_C(0x8080808080808080)))
```



_Symbol type:_ `macro`

#### `FIO_HAS_FULL_BYTE64`

```c
#define FIO_HAS_FULL_BYTE64(row)   \
  ((((row)&UINT64_C(0x7F7F7F7F7F7F7F7F)) + UINT64_C(0x0101010101010101)) &   \
   (row)&UINT64_C(0x8080808080808080))
```



_Symbol type:_ `macro`

#### `FIO_HAS_BYTE2BITMAP`

```c
#define FIO_HAS_BYTE2BITMAP(result, bit_index)   \
  do {   \
    (result) = fio_ltole64((result)); /* map little endian to bitmap */   \
    (result) >>= bit_index;           /* move bit index to 0x01 */   \
    (result) |= (result) >> 7;        /* pack all 0x80 bits into one byte */   \
    (result) |= (result) >> 14;   \
    (result) |= (result) >> 28;   \
    (result) &= 0xFFU;   \
  } while (0)
```

Converts a FIO_HAS_FULL_BYTE64 result to relative position bitmap.

_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME0`

```c
#define FIO_U8_HASH_PRIME0   0x17U   /* (4/6) 00010111 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME1`

```c
#define FIO_U8_HASH_PRIME1   0x1DU   /* (4/6) 00011101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME2`

```c
#define FIO_U8_HASH_PRIME2   0x25U   /* (3/6) 00100101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME3`

```c
#define FIO_U8_HASH_PRIME3   0x29U   /* (3/6) 00101001 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME4`

```c
#define FIO_U8_HASH_PRIME4   0x2BU   /* (4/6) 00101011 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME5`

```c
#define FIO_U8_HASH_PRIME5   0x35U   /* (4/6) 00110101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME6`

```c
#define FIO_U8_HASH_PRIME6   0x3BU   /* (5/6) 00111011 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME7`

```c
#define FIO_U8_HASH_PRIME7   0x43U   /* (3/8) 01000011 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME8`

```c
#define FIO_U8_HASH_PRIME8   0x47U   /* (4/8) 01000111 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME9`

```c
#define FIO_U8_HASH_PRIME9   0x49U   /* (3/8) 01001001 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME10`

```c
#define FIO_U8_HASH_PRIME10  0x53U   /* (4/8) 01010011 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME11`

```c
#define FIO_U8_HASH_PRIME11  0x59U   /* (4/8) 01011001 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME12`

```c
#define FIO_U8_HASH_PRIME12  0x61U   /* (3/8) 01100001 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME13`

```c
#define FIO_U8_HASH_PRIME13  0x65U   /* (4/8) 01100101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME14`

```c
#define FIO_U8_HASH_PRIME14  0x67U   /* (5/8) 01100111 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME15`

```c
#define FIO_U8_HASH_PRIME15  0x6BU   /* (5/8) 01101011 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME16`

```c
#define FIO_U8_HASH_PRIME16  0x6DU   /* (5/8) 01101101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME17`

```c
#define FIO_U8_HASH_PRIME17  0x83U   /* (3/8) 10000011 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME18`

```c
#define FIO_U8_HASH_PRIME18  0x89U   /* (3/8) 10001001 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME19`

```c
#define FIO_U8_HASH_PRIME19  0x8BU   /* (4/8) 10001011 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME20`

```c
#define FIO_U8_HASH_PRIME20  0x95U   /* (4/8) 10010101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME21`

```c
#define FIO_U8_HASH_PRIME21  0x97U   /* (5/8) 10010111 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME22`

```c
#define FIO_U8_HASH_PRIME22  0x9DU   /* (5/8) 10011101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME23`

```c
#define FIO_U8_HASH_PRIME23  0xA3U   /* (4/8) 10100011 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME24`

```c
#define FIO_U8_HASH_PRIME24  0xA7U   /* (5/8) 10100111 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME25`

```c
#define FIO_U8_HASH_PRIME25  0xADU   /* (5/8) 10101101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME26`

```c
#define FIO_U8_HASH_PRIME26  0xB3U   /* (5/8) 10110011 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME27`

```c
#define FIO_U8_HASH_PRIME27  0xB5U   /* (5/8) 10110101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME28`

```c
#define FIO_U8_HASH_PRIME28  0xC1U   /* (3/8) 11000001 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME29`

```c
#define FIO_U8_HASH_PRIME29  0xC5U   /* (4/8) 11000101 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME30`

```c
#define FIO_U8_HASH_PRIME30  0xC7U   /* (5/8) 11000111 */
```



_Symbol type:_ `macro`

#### `FIO_U8_HASH_PRIME31`

```c
#define FIO_U8_HASH_PRIME31  0xD3U   /* (5/8) 11010011 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME0`

```c
#define FIO_U16_HASH_PRIME0  0x631DU /* 0110001100011101 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME1`

```c
#define FIO_U16_HASH_PRIME1  0x4F19U /* 0100111100011001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME2`

```c
#define FIO_U16_HASH_PRIME2  0xA91BU /* 1010100100011011 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME3`

```c
#define FIO_U16_HASH_PRIME3  0xDF01U /* 1101111100000001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME4`

```c
#define FIO_U16_HASH_PRIME4  0x8C5DU /* 1000110001011101 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME5`

```c
#define FIO_U16_HASH_PRIME5  0xF941U /* 1111100101000001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME6`

```c
#define FIO_U16_HASH_PRIME6  0xC49DU /* 1100010010011101 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME7`

```c
#define FIO_U16_HASH_PRIME7  0xA32BU /* 1010001100101011 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME8`

```c
#define FIO_U16_HASH_PRIME8  0x7859U /* 0111100001011001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME9`

```c
#define FIO_U16_HASH_PRIME9  0xC4F1U /* 1100010011110001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME10`

```c
#define FIO_U16_HASH_PRIME10 0x74E1U /* 0111010011100001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME11`

```c
#define FIO_U16_HASH_PRIME11 0xD433U /* 1101010000110011 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME12`

```c
#define FIO_U16_HASH_PRIME12 0xCB29U /* 1100101100101001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME13`

```c
#define FIO_U16_HASH_PRIME13 0xC2A7U /* 1100001010100111 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME14`

```c
#define FIO_U16_HASH_PRIME14 0xC317U /* 1100001100010111 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME15`

```c
#define FIO_U16_HASH_PRIME15 0x92B9U /* 1001001010111001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME16`

```c
#define FIO_U16_HASH_PRIME16 0x7D03U /* 0111110100000011 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME17`

```c
#define FIO_U16_HASH_PRIME17 0x5CD1U /* 0101110011010001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME18`

```c
#define FIO_U16_HASH_PRIME18 0x73C1U /* 0111001111000001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME19`

```c
#define FIO_U16_HASH_PRIME19 0x69A3U /* 0110100110100011 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME20`

```c
#define FIO_U16_HASH_PRIME20 0xA2B3U /* 1010001010110011 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME21`

```c
#define FIO_U16_HASH_PRIME21 0x521FU /* 0101001000011111 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME22`

```c
#define FIO_U16_HASH_PRIME22 0x4E53U /* 0100111001010011 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME23`

```c
#define FIO_U16_HASH_PRIME23 0xFC41U /* 1111110001000001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME24`

```c
#define FIO_U16_HASH_PRIME24 0x5F09U /* 0101111100001001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME25`

```c
#define FIO_U16_HASH_PRIME25 0x605FU /* 0110000001011111 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME26`

```c
#define FIO_U16_HASH_PRIME26 0xA715U /* 1010011100010101 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME27`

```c
#define FIO_U16_HASH_PRIME27 0x6C65U /* 0110110001100101 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME28`

```c
#define FIO_U16_HASH_PRIME28 0x65C5U /* 0110010111000101 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME29`

```c
#define FIO_U16_HASH_PRIME29 0x85D3U /* 1000010111010011 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME30`

```c
#define FIO_U16_HASH_PRIME30 0xDE41U /* 1101111001000001 */
```



_Symbol type:_ `macro`

#### `FIO_U16_HASH_PRIME31`

```c
#define FIO_U16_HASH_PRIME31 0xCA8DU /* 1100101010001101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME0`

```c
#define FIO_U32_HASH_PRIME0  0x618E9735 /* 01100001100011101001011100110101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME1`

```c
#define FIO_U32_HASH_PRIME1  0xD9E8E033 /* 11011001111010001110000000110011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME2`

```c
#define FIO_U32_HASH_PRIME2  0x50F116F9 /* 01010000111100010001011011111001 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME3`

```c
#define FIO_U32_HASH_PRIME3  0x6E098F4B /* 01101110000010011000111101001011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME4`

```c
#define FIO_U32_HASH_PRIME4  0x8CC87A6B /* 10001100110010000111101001101011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME5`

```c
#define FIO_U32_HASH_PRIME5  0x59E16F03 /* 01011001111000010110111100000011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME6`

```c
#define FIO_U32_HASH_PRIME6  0xBB838C63 /* 10111011100000111000110001100011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME7`

```c
#define FIO_U32_HASH_PRIME7  0x8532FF05 /* 10000101001100101111111100000101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME8`

```c
#define FIO_U32_HASH_PRIME8  0x44FEC4A5 /* 01000100111111101100010010100101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME9`

```c
#define FIO_U32_HASH_PRIME9  0x9B3350D5 /* 10011011001100110101000011010101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME10`

```c
#define FIO_U32_HASH_PRIME10 0x64BE6287 /* 01100100101111100110001010000111 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME11`

```c
#define FIO_U32_HASH_PRIME11 0x57C2370B /* 01010111110000100011011100001011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME12`

```c
#define FIO_U32_HASH_PRIME12 0x9E724F41 /* 10011110011100100100111101000001 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME13`

```c
#define FIO_U32_HASH_PRIME13 0xF4A8A173 /* 11110100101010001010000101110011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME14`

```c
#define FIO_U32_HASH_PRIME14 0x6C4560FD /* 01101100010001010110000011111101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME15`

```c
#define FIO_U32_HASH_PRIME15 0xD8558C3D /* 11011000010101011000110000111101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME16`

```c
#define FIO_U32_HASH_PRIME16 0xC2F29157 /* 11000010111100101001000101010111 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME17`

```c
#define FIO_U32_HASH_PRIME17 0xF4D03789 /* 11110100110100000011011110001001 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME18`

```c
#define FIO_U32_HASH_PRIME18 0x9FB01857 /* 10011111101100000001100001010111 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME19`

```c
#define FIO_U32_HASH_PRIME19 0xE9513C0F /* 11101001010100010011110000001111 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME20`

```c
#define FIO_U32_HASH_PRIME20 0x89862FD3 /* 10001001100001100010111111010011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME21`

```c
#define FIO_U32_HASH_PRIME21 0xB742A51D /* 10110111010000101010010100011101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME22`

```c
#define FIO_U32_HASH_PRIME22 0xB3A681B9 /* 10110011101001101000000110111001 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME23`

```c
#define FIO_U32_HASH_PRIME23 0xC44899F7 /* 11000100010010001001100111110111 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME24`

```c
#define FIO_U32_HASH_PRIME24 0x67DE8341 /* 01100111110111101000001101000001 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME25`

```c
#define FIO_U32_HASH_PRIME25 0xF453213B /* 11110100010100110010000100111011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME26`

```c
#define FIO_U32_HASH_PRIME26 0xD22F9855 /* 11010010001011111001100001010101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME27`

```c
#define FIO_U32_HASH_PRIME27 0x8B3E807D /* 10001011001111101000000001111101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME28`

```c
#define FIO_U32_HASH_PRIME28 0x59DD1C23 /* 01011001110111010001110000100011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME29`

```c
#define FIO_U32_HASH_PRIME29 0xEE548C8B /* 11101110010101001000110010001011 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME30`

```c
#define FIO_U32_HASH_PRIME30 0xD2E74E05 /* 11010010111001110100111000000101 */
```



_Symbol type:_ `macro`

#### `FIO_U32_HASH_PRIME31`

```c
#define FIO_U32_HASH_PRIME31 0x4E55788D /* 01001110010101010111100010001101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME0`

```c
#define FIO_U64_HASH_PRIME0  ((uint64_t)0x39664DEECA23D825) /* 0011100101100110010011011110111011001010001000111101100000100101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME1`

```c
#define FIO_U64_HASH_PRIME1  ((uint64_t)0x48644F7B3959621F) /* 0100100001100100010011110111101100111001010110010110001000011111 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME2`

```c
#define FIO_U64_HASH_PRIME2  ((uint64_t)0x613A19F5CB0D98D5) /* 0110000100111010000110011111010111001011000011011001100011010101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME3`

```c
#define FIO_U64_HASH_PRIME3  ((uint64_t)0x84B56B93C869EA0F) /* 1000010010110101011010111001001111001000011010011110101000001111 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME4`

```c
#define FIO_U64_HASH_PRIME4  ((uint64_t)0x8EE38D13E0D95A8D) /* 1000111011100011100011010001001111100000110110010101101010001101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME5`

```c
#define FIO_U64_HASH_PRIME5  ((uint64_t)0x92E99EC981F0E279) /* 1001001011101001100111101100100110000001111100001110001001111001 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME6`

```c
#define FIO_U64_HASH_PRIME6  ((uint64_t)0xDDC3100BEF158BB1) /* 1101110111000011000100000000101111101111000101011000101110110001 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME7`

```c
#define FIO_U64_HASH_PRIME7  ((uint64_t)0x918F4D38049F78BD) /* 1001000110001111010011010011100000000100100111110111100010111101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME8`

```c
#define FIO_U64_HASH_PRIME8  ((uint64_t)0xB6C9F8032A35E2D9) /* 1011011011001001111110000000001100101010001101011110001011011001 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME9`

```c
#define FIO_U64_HASH_PRIME9  ((uint64_t)0xFA2A5F16D2A128D5) /* 1111101000101010010111110001011011010010101000010010100011010101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME10`

```c
#define FIO_U64_HASH_PRIME10 ((uint64_t)0x5823C958ED5547D9) /* 0101100000100011110010010101100011101101010101010100011111011001 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME11`

```c
#define FIO_U64_HASH_PRIME11 ((uint64_t)0xE8AB702EEE09CB43) /* 1110100010101011011100000010111011101110000010011100101101000011 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME12`

```c
#define FIO_U64_HASH_PRIME12 ((uint64_t)0xEBD609356421F13D) /* 1110101111010110000010010011010101100100001000011111000100111101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME13`

```c
#define FIO_U64_HASH_PRIME13 ((uint64_t)0x43D0C330AF5B1F17) /* 0100001111010000110000110011000010101111010110110001111100010111 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME14`

```c
#define FIO_U64_HASH_PRIME14 ((uint64_t)0xFEAE66D234871807) /* 1111111010101110011001101101001000110100100001110001100000000111 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME15`

```c
#define FIO_U64_HASH_PRIME15 ((uint64_t)0xEE54B43A52941D6B) /* 1110111001010100101101000011101001010010100101000001110101101011 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME16`

```c
#define FIO_U64_HASH_PRIME16 ((uint64_t)0x874E9DE46F15E205) /* 1000011101001110100111011110010001101111000101011110001000000101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME17`

```c
#define FIO_U64_HASH_PRIME17 ((uint64_t)0xFC7CA51A8A2E9171) /* 1111110001111100101001010001101010001010001011101001000101110001 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME18`

```c
#define FIO_U64_HASH_PRIME18 ((uint64_t)0x83A70617F71F3C21) /* 1000001110100111000001100001011111110111000111110011110000100001 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME19`

```c
#define FIO_U64_HASH_PRIME19 ((uint64_t)0x50FA705D6D99C11D) /* 0101000011111010011100000101110101101101100110011100000100011101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME20`

```c
#define FIO_U64_HASH_PRIME20 ((uint64_t)0x5362B5E6CF64814B) /* 0101001101100010101101011110011011001111011001001000000101001011 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME21`

```c
#define FIO_U64_HASH_PRIME21 ((uint64_t)0xA7A178389B0F3077) /* 1010011110100001011110000011100010011011000011110011000001110111 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME22`

```c
#define FIO_U64_HASH_PRIME22 ((uint64_t)0x779D78921199BA45) /* 0111011110011101011110001001001000010001100110011011101001000101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME23`

```c
#define FIO_U64_HASH_PRIME23 ((uint64_t)0xB42FD16A9AE90F81) /* 1011010000101111110100010110101010011010111010010000111110000001 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME24`

```c
#define FIO_U64_HASH_PRIME24 ((uint64_t)0xA2B4538A3C95576D) /* 1010001010110100010100111000101000111100100101010101011101101101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME25`

```c
#define FIO_U64_HASH_PRIME25 ((uint64_t)0x5E23D8E445F94E0D) /* 0101111000100011110110001110010001000101111110010100111000001101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME26`

```c
#define FIO_U64_HASH_PRIME26 ((uint64_t)0xE7CA493CD6444F07) /* 1110011111001010010010010011110011010110010001000100111100000111 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME27`

```c
#define FIO_U64_HASH_PRIME27 ((uint64_t)0x734719A6A1873CB5) /* 0111001101000111000110011010011010100001100001110011110010110101 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME28`

```c
#define FIO_U64_HASH_PRIME28 ((uint64_t)0x56CCB954143A3AB7) /* 0101011011001100101110010101010000010100001110100011101010110111 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME29`

```c
#define FIO_U64_HASH_PRIME29 ((uint64_t)0xFA5BC5A72480BF81) /* 1111101001011011110001011010011100100100100000001011111110000001 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME30`

```c
#define FIO_U64_HASH_PRIME30 ((uint64_t)0xD9B97D090C09F789) /* 1101100110111001011111010000100100001100000010011111011110001001 */
```



_Symbol type:_ `macro`

#### `FIO_U64_HASH_PRIME31`

```c
#define FIO_U64_HASH_PRIME31 ((uint64_t)0x9F74D0B9972E404B) /* 1001111101110100110100001011100110010111001011100100000001001011 */
```



_Symbol type:_ `macro`

#### `FIO_PRIME_TABLE_LIMIT`

```c
#define FIO_PRIME_TABLE_LIMIT 1024
```



_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_OP`

```c
#define FIO_MATH_UXXX_OP(t, a, b, bits, op)   \
  do {   \
    for (size_t i__ = 0; i__ < (sizeof((t)) / sizeof((t)[0])); ++i__)   \
      (t)[i__] = (a)[i__] op(b)[i__];   \
  } while (0)
```

Performs `a op b` (+,-, *, etc') using easily vectorized loop.

_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_COP`

```c
#define FIO_MATH_UXXX_COP(t, a, b, bits, op)   \
  do {   \
    for (size_t i__ = 0; i__ < (sizeof((t)) / sizeof((t)[0])); ++i__)   \
      (t)[i__] = (a)[i__] op(b);   \
  } while (0)
```

Performs `a op b` (+,-, *, etc'), where `b` is a constant.

_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_SOP`

```c
#define FIO_MATH_UXXX_SOP(t, a, bits, op)   \
  do {   \
    for (size_t i__ = 0; i__ < (sizeof((t)) / sizeof((t)[0])); ++i__)   \
      (t)[i__] = op(a)[i__];   \
  } while (0)
```

Performs `t = op (a)`.

_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_OP_RROT`

```c
#define FIO_MATH_UXXX_OP_RROT(t, a, b, bits)   \
  do {   \
    for (size_t i__ = 0; i__ < (sizeof((t)) / sizeof((t)[0])); ++i__)   \
      (t)[i__] = ((a)[i__] >> (b)[i__]) |   \
                 ((a)[i__] << ((bits - (b)[i__]) & ((bits)-1)));   \
  } while (0)
```

Performs `(a >> b) | (a << (bits - b))` (right rotation) in a loop.

_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_OP_CRROT`

```c
#define FIO_MATH_UXXX_OP_CRROT(t, a, c, bits)   \
  do {   \
    for (size_t i__ = 0; i__ < (sizeof((t)) / sizeof((t)[0])); ++i__)   \
      (t)[i__] =   \
          ((a)[i__] >> (c)) | ((a)[i__] << ((bits - (c)) & ((bits)-1)));   \
  } while (0)
```

Performs `(a >> c) | (a << (bits - c))` (const right rotation) in a loop.

_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_OP_LROT`

```c
#define FIO_MATH_UXXX_OP_LROT(t, a, b, bits)   \
  do {   \
    for (size_t i__ = 0; i__ < (sizeof((t)) / sizeof((t)[0])); ++i__)   \
      (t)[i__] = ((a)[i__] << (b)[i__]) |   \
                 ((a)[i__] >> ((bits - (b)[i__]) & ((bits)-1)));   \
  } while (0)
```

Performs `(a << b) | (a >> (bits - b))` (left rotation) in a loop.

_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_OP_CLROT`

```c
#define FIO_MATH_UXXX_OP_CLROT(t, a, c, bits)   \
  do {   \
    for (size_t i__ = 0; i__ < (sizeof((t)) / sizeof((t)[0])); ++i__)   \
      (t)[i__] =   \
          ((a)[i__] << (c)) | ((a)[i__] >> ((bits - (c)) & ((bits)-1)));   \
  } while (0)
```

Performs `(a << c) | (a >> (bits - c))` (const left rotation) in a loop.

_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_TOP`

```c
#define FIO_MATH_UXXX_TOP(t, a, b, c, bits, expr)   \
  do {   \
    for (size_t i__ = 0; i__ < (sizeof((t)) / sizeof((t)[0])); ++i__)   \
      (t)[i__] = expr((a)[i__], (b)[i__], (c)[i__]);   \
  } while (0)
```

Performs ternary `t = f(a, b, c)` lane-wise using easily vectorized loop.

_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_REDUCE`

```c
#define FIO_MATH_UXXX_REDUCE(t, a, bits, op)   \
  do {   \
    t = (a)[0];   \
    for (size_t i__ = 1; i__ < (sizeof((a)) / sizeof((a)[0])); ++i__)   \
      (t) = (t)op(a)[i__];   \
  } while (0)
```

Performs vector reduction for using `op` (+,-, *, etc'), storing to `t`.

_Symbol type:_ `macro`

#### `FIO_MATH_UXXX_SUFFLE`

```c
#define FIO_MATH_UXXX_SUFFLE(var, bits, ...)   \
  do {   \
    uint##bits##_t t____[sizeof((var)) / sizeof((var)[0])];   \
    const uint8_t shuf____[sizeof((var)) / sizeof((var)[0])] = {__VA_ARGS__};   \
    for (size_t i___ = 0; i___ < (sizeof((var)) / sizeof((var)[0])); ++i___)   \
      t____[i___] = (var)[shuf____[i___]];   \
    for (size_t i___ = 0; i___ < (sizeof((var)) / sizeof((var)[0])); ++i___)   \
      (var)[i___] = t____[i___];   \
  } while (0)
```

Performs vector shuffling (reordering) of `var`.

_Symbol type:_ `macro`

#### `FIO_VEC_SIMPLE_OP`

```c
#define FIO_VEC_SIMPLE_OP(result, a, b, len, op)   \
  FIO_FOR_UNROLL(len, sizeof(*a), i__, result[i__] = a[i__] op b[i__])
```

A math operation `op` between correlating vector positions of same length

_Symbol type:_ `macro`

#### `FIO_VEC_SCALAR_OP`

```c
#define FIO_VEC_SCALAR_OP(result, v, sclr, len, op)   \
  FIO_FOR_UNROLL(len, sizeof(*(v)), i__, (result)[i__] = (v)[i__] op(sclr))
```

A math operation `op` between a scalar and a vector

_Symbol type:_ `macro`

#### `FIO_VEC_REDUCE_OP`

```c
#define FIO_VEC_REDUCE_OP(result, vec, len, op)   \
  FIO_FOR_UNROLL(len, sizeof(*vec), i__, result = result op vec[i__])
```

A math operation `op` between all vector members

_Symbol type:_ `macro`

#### `FIO_VEC_DOT`

```c
#define FIO_VEC_DOT(result, a, b, len)   \
  FIO_FOR_UNROLL(len, sizeof(*a), i__, result += a[i__] * b[i__])
```

The dot product of two vectors (sum(a[i]*b[i])).

_Symbol type:_ `macro`

#### `FIO_VEC_ADD`

```c
#define FIO_VEC_ADD(result, a, b, len) FIO_VEC_SIMPLE_OP(result, a, b, len, +)
```

Add two vectors of same length (adding corresponding positions)

_Symbol type:_ `macro`

#### `FIO_VEC_SUB`

```c
#define FIO_VEC_SUB(result, a, b, len) FIO_VEC_SIMPLE_OP(result, a, b, len, -)
```

Subtracting two vectors of same length

_Symbol type:_ `macro`

#### `FIO_VEC_MUL`

```c
#define FIO_VEC_MUL(result, a, b, len) FIO_VEC_SIMPLE_OP(result, a, b, len, *)
```

Multiplying two vectors (multiplying corresponding positions - half dot)

_Symbol type:_ `macro`

#### `FIO_VEC_SCALAR_ADD`

```c
#define FIO_VEC_SCALAR_ADD(result, vec, scalar, vlen)   \
  FIO_VEC_SCALAR_OP(result, vec, scalar, vlen, +)
```

Add scalar to vector

_Symbol type:_ `macro`

#### `FIO_VEC_SCALAR_SUB`

```c
#define FIO_VEC_SCALAR_SUB(result, vec, scalar, vlen)   \
  FIO_VEC_SCALAR_OP(result, vec, scalar, vlen, -)
```

Subtract scalar from vector

_Symbol type:_ `macro`

#### `FIO_VEC_SCALAR_MUL`

```c
#define FIO_VEC_SCALAR_MUL(result, vec, scalar, vlen)   \
  FIO_VEC_SCALAR_OP(result, vec, scalar, vlen, *)
```

Multiply scalar from vector

_Symbol type:_ `macro`

#### `FIO_VEC_REDUCE_ADD`

```c
#define FIO_VEC_REDUCE_ADD(result, vec, vlen)   \
  FIO_VEC_REDUCE_OP(result, vec, vlen, +)
```

Add all members of a vector

_Symbol type:_ `macro`

#### `FIO_VEC_REDUCE_SUB`

```c
#define FIO_VEC_REDUCE_SUB(result, vec, vlen)   \
  FIO_VEC_REDUCE_OP(result, vec, vlen, -)
```

Subtract all members of a vector

_Symbol type:_ `macro`

#### `FIO_VEC_REDUCE_MUL`

```c
#define FIO_VEC_REDUCE_MUL(result, vec, vlen)   \
  FIO_VEC_REDUCE_OP(result, vec, vlen, *)
```

Multiply all members of a vector

_Symbol type:_ `macro`

#### `FIO_DEFINE_RANDOM128_FN`

```c
#define FIO_DEFINE_RANDOM128_FN(extern, name, reseed_log, seed_offset)   \
  static uint64_t name##___state[9] FIO_ALIGN(64) = {   \
      0x9c65875be1fce7b9ULL + seed_offset,   \
      0x7cc568e838f6a40dULL,   \
      0x4bb8d885a0fe47d5ULL + seed_offset,   \
      0x95561f0927ad7ecdULL,   \
      0};   \
  extern FIO_MAYBE_UNUSED void name##_reset(void) {   \
    name##___state[0] = 0x9c65875be1fce7b9ULL + seed_offset;   \
    name##___state[1] = 0x7cc568e838f6a40dULL;   \
    name##___state[2] = 0x4bb8d885a0fe47d5ULL + seed_offset;   \
    name##___state[3] = 0x95561f0927ad7ecdULL;   \
    name##___state[8] = name##___state[4] = 0;   \
  }   \
  extern void name##_reseed(void) {   \
    const size_t jitter_samples = 16 | (name##___state[0] & 15);   \
    for (size_t i = 0; i < jitter_samples; ++i) {   \
      struct timespec t;   \
      clock_gettime(CLOCK_MONOTONIC, &t);   \
      uint64_t clk[2] = {(uint64_t)(uintptr_t)&clk + (uint64_t)(uintptr_t) &   \
                             (name##_reseed),   \
                         0};   \
      clk[0] += (uint64_t)((t.tv_sec << 30) + (int64_t)t.tv_nsec);   \
      clk[0] = fio_math_mulc64(clk[0], FIO_U64_HASH_PRIME0, clk + 1);   \
      clk[1] += FIO_U64_HASH_PRIME0;   \
      clk[0] += fio_lrot64(clk[0], 27);   \
      clk[0] += fio_lrot64(clk[0], 49);   \
      clk[1] += fio_lrot64(clk[1], 27);   \
      clk[1] += fio_lrot64(clk[1], 49);   \
      name##___state[0] += clk[0] + fio_cycle_counter();   \
      name##___state[1] += clk[1] + fio_cycle_counter();   \
      name##___state[2] += clk[0] + fio_cycle_counter();   \
      name##___state[3] += clk[1] + fio_cycle_counter();   \
    }   \
    name##___state[8] += ((name##___state[8] & 7) + (name##___state[0] & 30));   \
  }   \
  /** Re-seeds the PNGR so forked processes don't match. */   \
  extern FIO_MAYBE_UNUSED void name##_on_fork(void *is_null) {   \
    (void)is_null;   \
    name##_reseed();   \
  }   \
  /** Returns a 128 bit pseudo-random number. */   \
  extern FIO_MAYBE_UNUSED fio_u128 name##128(void) {   \
    fio_u256 r;   \
    if (!(fio_atomic_add(name##___state + 4, 1) &   \
          ((1ULL << reseed_log) - 1)) &&   \
        ((size_t)(reseed_log - 1) < 63))   \
      name##_reseed();   \
    uint64_t s1[4];   \
    { /* load state to registers and roll, mul, add */   \
      const uint64_t cycles =   \
          reseed_log ? fio_cycle_counter() + (uint64_t)(uintptr_t)&cycles   \
                     : 0xB5ULL;   \
      const uint64_t variation =   \
          0x4E55788DULL +   \
          (reseed_log ? (uint64_t)(uintptr_t)&name##_reseed : 0);   \
      const uint64_t s0[] = {(name##___state[0] + cycles),   \
                             (name##___state[1] + cycles),   \
                             (name##___state[2] + cycles),   \
                             (name##___state[3] + cycles)};   \
      const uint64_t mulp[] = {0x37701261ED6C16C7ULL,   \
                               0x764DBBB75F3B3E0DULL,   \
                               ~(0x37701261ED6C16C7ULL),   \
                               ~(0x764DBBB75F3B3E0DULL)};   \
      const uint64_t addc[] = {name##___state[4],   \
                               seed_offset + 0x59DD1C23ULL,   \
                               name##___state[4] + cycles,   \
                               variation};   \
      for (size_t i = 0; i < 4; ++i) {   \
        s1[i] = fio_lrot64(s0[i], 33);   \
        s1[i] += addc[i];   \
        s1[i] *= mulp[i];   \
        s1[i] += s0[i];   \
      }   \
    }   \
    for (size_t i = 0; i < 4; ++i) /* store to memory */   \
      name##___state[i] = s1[i];   \
    {   \
      const uint8_t rotc[] = {31, 29, 27, 30};   \
      for (size_t i = 0; i < 4; ++i)   \
        r.u64[i] = fio_lrot64(s1[i], rotc[i]);   \
    }   \
    r.u64[0] += r.u64[2];   \
    r.u64[1] += r.u64[3];   \
    return r.u128[0];   \
  }   \
  /** Returns a 64 bit pseudo-random number. */   \
  extern FIO_MAYBE_UNUSED uint64_t name##64(void) {   \
    static fio_u128 r;   \
    size_t counter = fio_atomic_add(name##___state + 8, 1);   \
    if (!(counter & 1))   \
      r = name##128();   \
    return r.u64[counter & 1];   \
  }   \
  /** Fills the `dest` buffer with pseudo-random noise. */   \
  extern FIO_MAYBE_UNUSED void name##_bytes(void *dest, size_t len) {   \
    if (!dest || !len)   \
      return;   \
    uint8_t *d = (uint8_t *)dest;   \
    for (unsigned i = 15; i < len; i += 16) {   \
      fio_u128 r = name##128();   \
      fio_memcpy16(d, r.u8);   \
      d += 16;   \
    }   \
    if (len & 15) {   \
      fio_u128 r = name##128();   \
      fio_memcpy15x(d, r.u8, len);   \
    }   \
  }
```

Defines a semi-deterministic Pseudo-Random 128 bit Number Generator function.

The following functions will be defined:

- extern fio_u128 name##128(void); // returns 128 bits
- extern uint64_t name##64(void); // returns 64 bits (simply half the result)
- extern void name##_bytes(void *buffer, size_t len); // fills a buffer
- extern void name##_reset(void); // resets the state of the PRNG
- extern void name##_on_fork(void * is_null); // reseeds the PRNG

If `reseed_log` is non-zero and less than 64, the PNGR is no longer
deterministic, as it will automatically re-seeds itself every 2^reseed_log
iterations.

If `extern` is `static` or `FIO_SFUNC`, static function will be defined.

_Symbol type:_ `macro`

#### `FIO_STR_INFO_IS_EQ`

```c
#define FIO_STR_INFO_IS_EQ(s1, s2)   \
  ((s1).len == (s2).len &&   \
   (!(s1).len || (s1).buf == (s2).buf ||   \
    ((s1).buf && (s2).buf && (s1).buf[0] == (s2).buf[0] &&   \
     !FIO_MEMCMP((s1).buf, (s2).buf, (s1).len))))
```

Compares two `fio_str_info_s` objects for content equality.

_Symbol type:_ `macro`

#### `FIO_BUF_INFO_IS_EQ`

```c
#define FIO_BUF_INFO_IS_EQ(s1, s2) FIO_STR_INFO_IS_EQ((s1), (s2))
```

Compares two `fio_buf_info_s` objects for content equality.

_Symbol type:_ `macro`

#### `FIO_STR_INFO0`

```c
#define FIO_STR_INFO0 ((fio_str_info_s){0})
```

A NULL fio_str_info_s.

_Symbol type:_ `macro`

#### `FIO_STR_INFO1`

```c
#define FIO_STR_INFO1(str)   \
  ((fio_str_info_s){.len = ((str) ? FIO_STRLEN((str)) : 0), .buf = (str)})
```

Converts a C String into a fio_str_info_s.

_Symbol type:_ `macro`

#### `FIO_STR_INFO2`

```c
#define FIO_STR_INFO2(str, length)   \
  ((fio_str_info_s){.len = (length), .buf = (str)})
```

Converts a String with a known length into a fio_str_info_s.

_Symbol type:_ `macro`

#### `FIO_STR_INFO3`

```c
#define FIO_STR_INFO3(str, length, capacity)   \
  ((fio_str_info_s){.len = (length), .buf = (str), .capa = (capacity)})
```

Converts a String with a known length and capacity into a fio_str_info_s.

_Symbol type:_ `macro`

#### `FIO_BUF_INFO0`

```c
#define FIO_BUF_INFO0 ((fio_buf_info_s){0})
```

A NULL fio_buf_info_s.

_Symbol type:_ `macro`

#### `FIO_BUF_INFO1`

```c
#define FIO_BUF_INFO1(str)   \
  ((fio_buf_info_s){.len = ((str) ? FIO_STRLEN((str)) : 0), .buf = (str)})
```

Converts a C String into a fio_buf_info_s.

_Symbol type:_ `macro`

#### `FIO_BUF_INFO2`

```c
#define FIO_BUF_INFO2(str, length)   \
  ((fio_buf_info_s){.len = (length), .buf = (str)})
```

Converts a String with a known length into a fio_buf_info_s.

_Symbol type:_ `macro`

#### `FIO_BUF2STR_INFO`

```c
#define FIO_BUF2STR_INFO(buf_info)   \
  ((fio_str_info_s){.len = (buf_info).len, .buf = (buf_info).buf})
```

Converts a fio_buf_info_s into a fio_str_info_s.

_Symbol type:_ `macro`

#### `FIO_STR2BUF_INFO`

```c
#define FIO_STR2BUF_INFO(str_info)   \
  ((fio_buf_info_s){.len = (str_info).len, .buf = (str_info).buf})
```

Converts a fio_buf_info_s into a fio_str_info_s.

_Symbol type:_ `macro`

#### `FIO_STR_INFO_TMP_VAR`

```c
#define FIO_STR_INFO_TMP_VAR(name, capacity)   \
  char fio___stack_mem___##name[(capacity) + 1];   \
  fio___stack_mem___##name[0] = 0; /* guard */   \
  fio_str_info_s name = (fio_str_info_s) {   \
    .buf = fio___stack_mem___##name, .capa = (capacity)   \
  }
```

Creates a stack fio_str_info_s variable `name` with `capacity` bytes.

_Symbol type:_ `macro`

#### `FIO_STR_INFO_TMP_IS_REALLOCATED`

```c
#define FIO_STR_INFO_TMP_IS_REALLOCATED(name)   \
  (fio___stack_mem___##name != name.buf)
```

Tests to see if memory reallocation happened.

_Symbol type:_ `macro`

#### `FIO_UTF8_ALLOW_IF`

```c
#define FIO_UTF8_ALLOW_IF 1
```



_Symbol type:_ `macro`

### Types

#### `fio_lock_i`

```c
typedef volatile unsigned char fio_lock_i
```

A spin lock type, must be initialized to `FIO_LOCK_INIT` before use.

_Symbol type:_ `type`

#### `fio_list_node_s`

```c
struct fio_list_node_s {
struct fio_list_node_s *next;
struct fio_list_node_s *prev;
}
```

A linked list arch-type

_Symbol type:_ `type`

#### `fio_index32_node_s`

```c
struct fio_index32_node_s {
uint32_t next;
uint32_t prev;
}
```

A 32 bit indexed linked list node type

_Symbol type:_ `type`

#### `fio_index16_node_s`

```c
struct fio_index16_node_s {
uint16_t next;
uint16_t prev;
}
```

A 16 bit indexed linked list node type

_Symbol type:_ `type`

#### `fio_index8_node_s`

```c
struct fio_index8_node_s {
uint8_t next;
uint8_t prev;
}
```

An 8 bit indexed linked list node type

_Symbol type:_ `type`

#### `fio_u128`

```c
union fio_u128 {
FIO___UXXX_UGRP_DEF(128);
#if defined(__SIZEOF_INT128__)
__uint128_t alignment_for_u128_[1];
#endif
}
```

An unsigned 128bit union type.

_Symbol type:_ `type`

#### `fio_u256`

```c
union fio_u256 {
fio_u128 u128[2];
FIO___UXXX_UGRP_DEF(256);
#if defined(__SIZEOF_INT256__)
__uint256_t alignment_for_u256_[1];
#endif
}
```

An unsigned 256bit union type.

_Symbol type:_ `type`

#### `fio_u512`

```c
union fio_u512 {
fio_u128 u128[4];
fio_u256 u256[2];
FIO___UXXX_UGRP_DEF(512);
}
```

An unsigned 512bit union type.

_Symbol type:_ `type`

#### `fio_u1024`

```c
union fio_u1024 {
fio_u128 u128[8];
fio_u256 u256[4];
fio_u512 u512[2];
FIO___UXXX_UGRP_DEF(1024);
}
```

An unsigned 1024bit union type.

_Symbol type:_ `type`

#### `fio_u2048`

```c
union fio_u2048 {
fio_u128 u128[16];
fio_u256 u256[8];
fio_u512 u512[4];
fio_u1024 u1024[2];
FIO___UXXX_UGRP_DEF(2048);
}
```

An unsigned 2048bit union type.

_Symbol type:_ `type`

#### `fio_u4096`

```c
union fio_u4096 {
fio_u128 u128[32];
fio_u256 u256[16];
fio_u512 u512[8];
fio_u1024 u1024[4];
fio_u2048 u2048[2];
FIO___UXXX_UGRP_DEF(4096);
}
```

An unsigned 4096bit union type.

_Symbol type:_ `type`

#### `fio_str_info_s`

```c
struct fio_str_info_s {
/** The string's length, if any. */
size_t len;
/** The string's buffer (pointer to first byte) or NULL on error. */
char *buf;
/** The buffer's capacity. Zero (0) indicates the buffer is read-only. */
size_t capa;
}
```

An information type for reporting the string's state.

_Symbol type:_ `type`

#### `fio_buf_info_s`

```c
struct fio_buf_info_s {
/** The buffer's length, if any. */
size_t len;
/** The buffer's address (may be NULL if no buffer). */
char *buf;
}
```

An information type for reporting/storing buffer data (no `capa`).

_Symbol type:_ `type`

### Functions

#### `fio_getpid`

```c
#define fio_getpid _getpid
```

Calls the `getpid` provided by the system.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_load`

```c
#define fio_atomic_load(dest, p_obj)   \
  do {   \
    dest = __atomic_load_n((p_obj), __ATOMIC_SEQ_CST);   \
  } while (0)
```

An atomic load operation, returns value in pointer.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_compare_exchange_p`

```c
#define fio_atomic_compare_exchange_p(p_obj, p_expected, p_desired) __atomic_compare_exchange((p_obj), (p_expected), (p_desired), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
```

An atomic compare and exchange operation, returns true if an exchange occured. `p_expected` MAY be overwritten with the existing value (system specific).

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_exchange`

```c
#define fio_atomic_exchange(p_obj, value) __atomic_exchange_n((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic exchange operation, returns previous value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_add`

```c
#define fio_atomic_add(p_obj, value) __atomic_fetch_add((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic addition operation, returns previous value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_sub`

```c
#define fio_atomic_sub(p_obj, value) __atomic_fetch_sub((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic subtraction operation, returns previous value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_and`

```c
#define fio_atomic_and(p_obj, value) __atomic_fetch_and((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic AND (&) operation, returns previous value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_xor`

```c
#define fio_atomic_xor(p_obj, value) __atomic_fetch_xor((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic XOR (^) operation, returns previous value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_or`

```c
#define fio_atomic_or(p_obj, value) __atomic_fetch_or((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic OR (|) operation, returns previous value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_nand`

```c
#define fio_atomic_nand(p_obj, value) __atomic_fetch_nand((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic NOT AND ((~)&) operation, returns previous value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_add_fetch`

```c
#define fio_atomic_add_fetch(p_obj, value) __atomic_add_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic addition operation, returns new value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_sub_fetch`

```c
#define fio_atomic_sub_fetch(p_obj, value) __atomic_sub_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic subtraction operation, returns new value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_and_fetch`

```c
#define fio_atomic_and_fetch(p_obj, value) __atomic_and_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic AND (&) operation, returns new value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_xor_fetch`

```c
#define fio_atomic_xor_fetch(p_obj, value) __atomic_xor_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic XOR (^) operation, returns new value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_or_fetch`

```c
#define fio_atomic_or_fetch(p_obj, value) __atomic_or_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic OR (|) operation, returns new value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_atomic_nand_fetch`

```c
#define fio_atomic_nand_fetch(p_obj, value) __atomic_nand_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
```

An atomic NOT AND ((~)&) operation, returns new value

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_trylock_group`

```c
inline uint8_t fio_trylock_group(fio_lock_i *lock, uint8_t group)
```

Tries to lock a group of sublocks.

Combine a number of sublocks using OR (`|`) and the FIO_LOCK_SUBLOCK(i)
macro. i.e.:

```c
if(!fio_trylock_group(&lock,
                      FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2))) {
   // act in lock
}
fio_unlock_group(&lock, FIO_LOCK_SUBLOCK(1) | FIO_LOCK_SUBLOCK(2)));
```

Returns 0 on success and non-zero on failure.

_Symbol type:_ `function`

#### `fio_lock_group`

```c
inline void fio_lock_group(fio_lock_i *lock, uint8_t group)
```

Busy waits for a group lock to become available - not recommended.

See `fio_trylock_group` for details.

_Symbol type:_ `function`

#### `fio_unlock_group`

```c
inline void fio_unlock_group(fio_lock_i *lock, uint8_t group)
```

Unlocks a sublock group, no matter which thread owns which sublock.

_Symbol type:_ `function`

#### `fio_trylock_full`

```c
inline uint8_t fio_trylock_full(fio_lock_i *lock)
```

Tries to lock all sublocks. Returns 0 on success and 1 on failure.

_Symbol type:_ `function`

#### `fio_lock_full`

```c
inline void fio_lock_full(fio_lock_i *lock)
```

Busy waits for all sub lock to become available - not recommended.

_Symbol type:_ `function`

#### `fio_unlock_full`

```c
inline void fio_unlock_full(fio_lock_i *lock)
```

Unlocks all sub locks, no matter which thread owns the lock.

_Symbol type:_ `function`

#### `fio_trylock`

```c
inline uint8_t fio_trylock(fio_lock_i *lock)
```

Tries to acquire the default lock (sublock 0).

Returns 0 on success and 1 on failure.

_Symbol type:_ `function`

#### `fio_lock`

```c
inline void fio_lock(fio_lock_i *lock)
```

Busy waits for the default lock to become available - not recommended.

_Symbol type:_ `function`

#### `fio_unlock`

```c
inline void fio_unlock(fio_lock_i *lock)
```

Unlocks the default lock, no matter which thread owns the lock.

_Symbol type:_ `function`

#### `fio_is_locked`

```c
inline uint8_t fio_is_locked(fio_lock_i *lock)
```

Returns 1 if the lock is locked, 0 otherwise.

_Symbol type:_ `function`

#### `fio_is_group_locked`

```c
inline uint8_t fio_is_group_locked(fio_lock_i *lock, uint8_t group)
```

Returns 1 if the lock is locked, 0 otherwise.

_Symbol type:_ `function`

#### `fio_atomic_bit_get`

```c
inline uint8_t fio_atomic_bit_get(void *map, size_t bit)
```

Gets the state of a bit in a bitmap.

_Symbol type:_ `function`

#### `fio_atomic_bit_set`

```c
inline void fio_atomic_bit_set(void *map, size_t bit)
```

Sets the a bit in a bitmap (sets to 1).

_Symbol type:_ `function`

#### `fio_atomic_bit_unset`

```c
inline void fio_atomic_bit_unset(void *map, size_t bit)
```

Unsets the a bit in a bitmap (sets to 0).

_Symbol type:_ `function`

#### `fio_atomic_bit_flip`

```c
inline void fio_atomic_bit_flip(void *map, size_t bit)
```

Flips the a bit in a bitmap (sets to 0 if 1, sets to 1 if 0).

_Symbol type:_ `function`

#### `fio_is_little_endian`

```c
inline unsigned int fio_is_little_endian(void)
```

Dynamically test if we are on a little endian machine.

_Symbol type:_ `function`

#### `fio_is_big_endian`

```c
inline unsigned int fio_is_big_endian(void)
```

Dynamically test if we are on a big endian machine.

_Symbol type:_ `function`

#### `fio_memcpy0`

```c
FIO_SFUNC void *fio_memcpy0(void *restrict d, const void *restrict s)
```

No-op (completes the name space).

_Symbol type:_ `function`

#### `fio_memcpy1`

```c
void * fio_memcpy1(void *restrict d, const void *restrict s)
```

Copies 1 byte from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy2`

```c
void * fio_memcpy2(void *restrict d, const void *restrict s)
```

Copies 2 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy3`

```c
void * fio_memcpy3(void *restrict d, const void *restrict s)
```

Copies 3 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy4`

```c
void * fio_memcpy4(void *restrict d, const void *restrict s)
```

Copies 4 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy5`

```c
void * fio_memcpy5(void *restrict d, const void *restrict s)
```

Copies 5 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy6`

```c
void * fio_memcpy6(void *restrict d, const void *restrict s)
```

Copies 6 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy7`

```c
void * fio_memcpy7(void *restrict d, const void *restrict s)
```

Copies 7 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy8`

```c
void * fio_memcpy8(void *restrict d, const void *restrict s)
```

Copies 8 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy16`

```c
void * fio_memcpy16(void *restrict d, const void *restrict s)
```

Copies 16 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy32`

```c
void * fio_memcpy32(void *restrict d, const void *restrict s)
```

Copies 32 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy64`

```c
void * fio_memcpy64(void *restrict d, const void *restrict s)
```

Copies 64 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy128`

```c
void * fio_memcpy128(void *restrict d, const void *restrict s)
```

Copies 128 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy256`

```c
void * fio_memcpy256(void *restrict d, const void *restrict s)
```

Copies 256 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy512`

```c
void * fio_memcpy512(void *restrict d, const void *restrict s)
```

Copies 512 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy1024`

```c
void * fio_memcpy1024(void *restrict d, const void *restrict s)
```

Copies 1024 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy2048`

```c
void * fio_memcpy2048(void *restrict d, const void *restrict s)
```

Copies 2048 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy4096`

```c
void * fio_memcpy4096(void *restrict d, const void *restrict s)
```

Copies 4096 bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy0x`

```c
FIO_SFUNC void *fio_memcpy0x(void *d, const void *s, size_t l)
```

No-op (completes the name space).

_Symbol type:_ `function`

#### `fio_memcpy7x`

```c
void * fio_memcpy7x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy15x`

```c
void * fio_memcpy15x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy31x`

```c
void * fio_memcpy31x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy63x`

```c
void * fio_memcpy63x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy127x`

```c
void * fio_memcpy127x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy255x`

```c
void * fio_memcpy255x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy511x`

```c
void * fio_memcpy511x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy1023x`

```c
void * fio_memcpy1023x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy2047x`

```c
void * fio_memcpy2047x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_memcpy4095x`

```c
void * fio_memcpy4095x(void *restrict d, const void *restrict s, size_t len)
```

Copies up to `len & lim` bytes from `src` (`s`) to `dest` (`d`).

_Symbol type:_ `function`

#### `fio_bswap8`

```c
#define fio_bswap8(i) (i)
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_bswap16`

```c
#define fio_bswap16(i) __builtin_bswap16((uint16_t)(i))
```

Byte swap a 16 bit integer, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_bswap32`

```c
#define fio_bswap32(i) __builtin_bswap32((uint32_t)(i))
```

Byte swap a 32 bit integer, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_bswap64`

```c
#define fio_bswap64(i) __builtin_bswap64((uint64_t)(i))
```

Byte swap a 64 bit integer, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_bswap128`

```c
#define fio_bswap128(i) __builtin_bswap128((__uint128_t)(i))
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_bswap128`

```c
inline FIO_CONST __uint128_t fio_bswap128(__uint128_t i)
```



_Symbol type:_ `function`

#### `fio_ltole8`

```c
#define fio_ltole8(i) (i) /* avoid special cases by defining for all sizes */
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lton8`

```c
#define fio_lton8(i)  (i) /* avoid special cases by defining for all sizes */
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ntol8`

```c
#define fio_ntol8(i)  (i) /* avoid special cases by defining for all sizes */
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lton16`

```c
#define fio_lton16(i) (i)
```

Local byte order to Network byte order, 16 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lton32`

```c
#define fio_lton32(i) (i)
```

Local byte order to Network byte order, 32 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lton64`

```c
#define fio_lton64(i) (i)
```

Local byte order to Network byte order, 62 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ltole16`

```c
#define fio_ltole16(i) fio_bswap16((i))
```

Local byte order to Little Endian byte order, 16 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ltole32`

```c
#define fio_ltole32(i) fio_bswap32((i))
```

Local byte order to Little Endian byte order, 32 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ltole64`

```c
#define fio_ltole64(i) fio_bswap64((i))
```

Local byte order to Little Endian byte order, 62 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ntol16`

```c
#define fio_ntol16(i) (i)
```

Network byte order to Local byte order, 16 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ntol32`

```c
#define fio_ntol32(i) (i)
```

Network byte order to Local byte order, 32 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ntol64`

```c
#define fio_ntol64(i) (i)
```

Network byte order to Local byte order, 62 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ntol128`

```c
#define fio_ntol128(i) (i)
```

Network byte order to Local byte order, 128 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ltole128`

```c
#define fio_ltole128(i) fio_bswap128((i))
```

Local byte order to Little Endian byte order, 128 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lton128`

```c
#define fio_lton128(i)  fio_bswap128((i))
```

Local byte order to Network byte order, 128 bit integer

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_buf2u8u`

```c
inline uint8_t fio_buf2u8u(const void *c)
```

Converts an unaligned byte stream to an 8 bit number.

_Symbol type:_ `function`

#### `fio_u2buf8u`

```c
inline void fio_u2buf8u(void *buf, uint8_t i)
```

Writes a local 8 bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u8_le`

```c
inline uint8_t fio_buf2u8_le(const void *c)
```

Converts an unaligned byte stream to an 8 bit number.

_Symbol type:_ `function`

#### `fio_u2buf8_le`

```c
inline void fio_u2buf8_le(void *buf, uint8_t i)
```

Writes a local 8 bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u8_be`

```c
inline uint8_t fio_buf2u8_be(const void *c)
```

Converts an unaligned byte stream to an 8 bit number.

_Symbol type:_ `function`

#### `fio_u2buf8_be`

```c
inline void fio_u2buf8_be(void *buf, uint8_t i)
```

Writes a local 8 bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u16u`

```c
uint16_t fio_buf2u16u(const void *c)
```

Converts an unaligned byte stream to a bits bit number.

_Symbol type:_ `function`

#### `fio_u2buf16u`

```c
void fio_u2buf16u(void *buf, uint16_t i)
```

Writes a bits bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u32u`

```c
uint32_t fio_buf2u32u(const void *c)
```

Converts an unaligned byte stream to a bits bit number.

_Symbol type:_ `function`

#### `fio_u2buf32u`

```c
void fio_u2buf32u(void *buf, uint32_t i)
```

Writes a bits bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u64u`

```c
uint64_t fio_buf2u64u(const void *c)
```

Converts an unaligned byte stream to a bits bit number.

_Symbol type:_ `function`

#### `fio_u2buf64u`

```c
void fio_u2buf64u(void *buf, uint64_t i)
```

Writes a bits bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u16_le`

```c
uint16_t fio_buf2u16_le(const void *c)
```

Converts an unaligned byte stream to a bits bit number.

_Symbol type:_ `function`

#### `fio_u2buf16_le`

```c
void fio_u2buf16_le(void *buf, uint16_t i)
```

Writes a bits bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u32_le`

```c
uint32_t fio_buf2u32_le(const void *c)
```

Converts an unaligned byte stream to a bits bit number.

_Symbol type:_ `function`

#### `fio_u2buf32_le`

```c
void fio_u2buf32_le(void *buf, uint32_t i)
```

Writes a bits bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u64_le`

```c
uint64_t fio_buf2u64_le(const void *c)
```

Converts an unaligned byte stream to a bits bit number.

_Symbol type:_ `function`

#### `fio_u2buf64_le`

```c
void fio_u2buf64_le(void *buf, uint64_t i)
```

Writes a bits bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u16_be`

```c
uint16_t fio_buf2u16_be(const void *c)
```

Converts an unaligned byte stream to a bits bit number.

_Symbol type:_ `function`

#### `fio_u2buf16_be`

```c
void fio_u2buf16_be(void *buf, uint16_t i)
```

Writes a bits bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u32_be`

```c
uint32_t fio_buf2u32_be(const void *c)
```

Converts an unaligned byte stream to a bits bit number.

_Symbol type:_ `function`

#### `fio_u2buf32_be`

```c
void fio_u2buf32_be(void *buf, uint32_t i)
```

Writes a bits bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u64_be`

```c
uint64_t fio_buf2u64_be(const void *c)
```

Converts an unaligned byte stream to a bits bit number.

_Symbol type:_ `function`

#### `fio_u2buf64_be`

```c
void fio_u2buf64_be(void *buf, uint64_t i)
```

Writes a bits bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u24_le`

```c
inline uint32_t fio_buf2u24_le(const void *c)
```

Converts an unaligned byte stream to a 24 bit number.

_Symbol type:_ `function`

#### `fio_u2buf24_le`

```c
inline void fio_u2buf24_le(void *buf, uint32_t i)
```

Writes a 24 bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u24_be`

```c
inline uint32_t fio_buf2u24_be(const void *c)
```

Converts an unaligned byte stream to a 24 bit number.

_Symbol type:_ `function`

#### `fio_u2buf24_be`

```c
inline void fio_u2buf24_be(void *buf, uint32_t i)
```

Writes a 24 bit number to an unaligned buffer.

_Symbol type:_ `function`

#### `fio_buf2u24u`

```c
inline uint32_t fio_buf2u24u(const void *c)
```

Converts an unaligned byte stream to a 24 bit number - local endieness.

_Symbol type:_ `function`

#### `fio_u2buf24u`

```c
inline void fio_u2buf24u(void *buf, uint32_t i)
```

Writes a 24 bit number to an unaligned buffer - in local endieness.

_Symbol type:_ `function`

#### `fio_buf2zu`

```c
inline size_t fio_buf2zu(const void *c)
```

Converts an unaligned byte stream to a size_t - local endieness.

_Symbol type:_ `function`

#### `fio_u2bufzu`

```c
inline void fio_u2bufzu(void *buf, size_t i)
```

Writes a size_t to an unaligned buffer - in local endieness.

_Symbol type:_ `function`

#### `fio_u8x4_add`

```c
void fio_u8x4_add(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x4_and`

```c
void fio_u8x4_and(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x4_mul`

```c
void fio_u8x4_mul(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x4_or`

```c
void fio_u8x4_or(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x4_reduce_add`

```c
uint8_t fio_u8x4_reduce_add(uint8_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x4_reduce_and`

```c
uint8_t fio_u8x4_reduce_and(uint8_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x4_reduce_max`

```c
uint8_t fio_u8x4_reduce_max(uint8_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x4_reduce_min`

```c
uint8_t fio_u8x4_reduce_min(uint8_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x4_reduce_mul`

```c
uint8_t fio_u8x4_reduce_mul(uint8_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x4_reduce_or`

```c
uint8_t fio_u8x4_reduce_or(uint8_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x4_reduce_sub`

```c
uint8_t fio_u8x4_reduce_sub(uint8_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x4_reduce_xor`

```c
uint8_t fio_u8x4_reduce_xor(uint8_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x4_reshuffle`

```c
void fio_u8x4_reshuffle(uint8_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u8x4_sub`

```c
void fio_u8x4_sub(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x4_xor`

```c
void fio_u8x4_xor(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x8_add`

```c
void fio_u8x8_add(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x8_and`

```c
void fio_u8x8_and(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x8_mul`

```c
void fio_u8x8_mul(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x8_or`

```c
void fio_u8x8_or(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x8_reduce_add`

```c
uint8_t fio_u8x8_reduce_add(uint8_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x8_reduce_and`

```c
uint8_t fio_u8x8_reduce_and(uint8_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x8_reduce_max`

```c
uint8_t fio_u8x8_reduce_max(uint8_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x8_reduce_min`

```c
uint8_t fio_u8x8_reduce_min(uint8_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x8_reduce_mul`

```c
uint8_t fio_u8x8_reduce_mul(uint8_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x8_reduce_or`

```c
uint8_t fio_u8x8_reduce_or(uint8_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x8_reduce_sub`

```c
uint8_t fio_u8x8_reduce_sub(uint8_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x8_reduce_xor`

```c
uint8_t fio_u8x8_reduce_xor(uint8_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x8_reshuffle`

```c
void fio_u8x8_reshuffle(uint8_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u8x8_sub`

```c
void fio_u8x8_sub(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x8_xor`

```c
void fio_u8x8_xor(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x16_add`

```c
void fio_u8x16_add(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x16_and`

```c
void fio_u8x16_and(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x16_mul`

```c
void fio_u8x16_mul(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x16_or`

```c
void fio_u8x16_or(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x16_reduce_add`

```c
uint8_t fio_u8x16_reduce_add(uint8_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x16_reduce_and`

```c
uint8_t fio_u8x16_reduce_and(uint8_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x16_reduce_max`

```c
uint8_t fio_u8x16_reduce_max(uint8_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x16_reduce_min`

```c
uint8_t fio_u8x16_reduce_min(uint8_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x16_reduce_mul`

```c
uint8_t fio_u8x16_reduce_mul(uint8_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x16_reduce_or`

```c
uint8_t fio_u8x16_reduce_or(uint8_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x16_reduce_sub`

```c
uint8_t fio_u8x16_reduce_sub(uint8_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x16_reduce_xor`

```c
uint8_t fio_u8x16_reduce_xor(uint8_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x16_reshuffle`

```c
void fio_u8x16_reshuffle(uint8_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u8x16_sub`

```c
void fio_u8x16_sub(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x16_xor`

```c
void fio_u8x16_xor(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x32_add`

```c
void fio_u8x32_add(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x32_and`

```c
void fio_u8x32_and(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x32_mul`

```c
void fio_u8x32_mul(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x32_or`

```c
void fio_u8x32_or(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x32_reduce_add`

```c
uint8_t fio_u8x32_reduce_add(uint8_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x32_reduce_and`

```c
uint8_t fio_u8x32_reduce_and(uint8_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x32_reduce_max`

```c
uint8_t fio_u8x32_reduce_max(uint8_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x32_reduce_min`

```c
uint8_t fio_u8x32_reduce_min(uint8_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x32_reduce_mul`

```c
uint8_t fio_u8x32_reduce_mul(uint8_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x32_reduce_or`

```c
uint8_t fio_u8x32_reduce_or(uint8_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x32_reduce_sub`

```c
uint8_t fio_u8x32_reduce_sub(uint8_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x32_reduce_xor`

```c
uint8_t fio_u8x32_reduce_xor(uint8_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x32_reshuffle`

```c
void fio_u8x32_reshuffle(uint8_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u8x32_sub`

```c
void fio_u8x32_sub(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x32_xor`

```c
void fio_u8x32_xor(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x64_add`

```c
void fio_u8x64_add(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x64_and`

```c
void fio_u8x64_and(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x64_mul`

```c
void fio_u8x64_mul(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x64_or`

```c
void fio_u8x64_or(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x64_reduce_add`

```c
uint8_t fio_u8x64_reduce_add(uint8_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x64_reduce_and`

```c
uint8_t fio_u8x64_reduce_and(uint8_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x64_reduce_max`

```c
uint8_t fio_u8x64_reduce_max(uint8_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x64_reduce_min`

```c
uint8_t fio_u8x64_reduce_min(uint8_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x64_reduce_mul`

```c
uint8_t fio_u8x64_reduce_mul(uint8_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x64_reduce_or`

```c
uint8_t fio_u8x64_reduce_or(uint8_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x64_reduce_sub`

```c
uint8_t fio_u8x64_reduce_sub(uint8_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x64_reduce_xor`

```c
uint8_t fio_u8x64_reduce_xor(uint8_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x64_reshuffle`

```c
void fio_u8x64_reshuffle(uint8_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u8x64_sub`

```c
void fio_u8x64_sub(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x64_xor`

```c
void fio_u8x64_xor(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x128_add`

```c
void fio_u8x128_add(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x128_and`

```c
void fio_u8x128_and(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x128_mul`

```c
void fio_u8x128_mul(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x128_or`

```c
void fio_u8x128_or(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x128_reduce_add`

```c
uint8_t fio_u8x128_reduce_add(uint8_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x128_reduce_and`

```c
uint8_t fio_u8x128_reduce_and(uint8_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x128_reduce_max`

```c
uint8_t fio_u8x128_reduce_max(uint8_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x128_reduce_min`

```c
uint8_t fio_u8x128_reduce_min(uint8_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x128_reduce_mul`

```c
uint8_t fio_u8x128_reduce_mul(uint8_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x128_reduce_or`

```c
uint8_t fio_u8x128_reduce_or(uint8_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x128_reduce_sub`

```c
uint8_t fio_u8x128_reduce_sub(uint8_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x128_reduce_xor`

```c
uint8_t fio_u8x128_reduce_xor(uint8_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x128_reshuffle`

```c
void fio_u8x128_reshuffle(uint8_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u8x128_sub`

```c
void fio_u8x128_sub(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x128_xor`

```c
void fio_u8x128_xor(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x256_add`

```c
void fio_u8x256_add(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x256_and`

```c
void fio_u8x256_and(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x256_mul`

```c
void fio_u8x256_mul(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x256_or`

```c
void fio_u8x256_or(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x256_reduce_add`

```c
uint8_t fio_u8x256_reduce_add(uint8_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x256_reduce_and`

```c
uint8_t fio_u8x256_reduce_and(uint8_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x256_reduce_max`

```c
uint8_t fio_u8x256_reduce_max(uint8_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x256_reduce_min`

```c
uint8_t fio_u8x256_reduce_min(uint8_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u8x256_reduce_mul`

```c
uint8_t fio_u8x256_reduce_mul(uint8_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x256_reduce_or`

```c
uint8_t fio_u8x256_reduce_or(uint8_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x256_reduce_sub`

```c
uint8_t fio_u8x256_reduce_sub(uint8_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x256_reduce_xor`

```c
uint8_t fio_u8x256_reduce_xor(uint8_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u8x256_reshuffle`

```c
void fio_u8x256_reshuffle(uint8_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u8x256_sub`

```c
void fio_u8x256_sub(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u8x256_xor`

```c
void fio_u8x256_xor(uint8_t *dest, uint8_t *a, uint8_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x2_add`

```c
void fio_u16x2_add(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x2_and`

```c
void fio_u16x2_and(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x2_mul`

```c
void fio_u16x2_mul(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x2_or`

```c
void fio_u16x2_or(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x2_reduce_add`

```c
uint16_t fio_u16x2_reduce_add(uint16_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x2_reduce_and`

```c
uint16_t fio_u16x2_reduce_and(uint16_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x2_reduce_max`

```c
uint16_t fio_u16x2_reduce_max(uint16_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x2_reduce_min`

```c
uint16_t fio_u16x2_reduce_min(uint16_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x2_reduce_mul`

```c
uint16_t fio_u16x2_reduce_mul(uint16_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x2_reduce_or`

```c
uint16_t fio_u16x2_reduce_or(uint16_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x2_reduce_sub`

```c
uint16_t fio_u16x2_reduce_sub(uint16_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x2_reduce_xor`

```c
uint16_t fio_u16x2_reduce_xor(uint16_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x2_reshuffle`

```c
void fio_u16x2_reshuffle(uint16_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u16x2_sub`

```c
void fio_u16x2_sub(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x2_xor`

```c
void fio_u16x2_xor(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x4_add`

```c
void fio_u16x4_add(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x4_and`

```c
void fio_u16x4_and(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x4_mul`

```c
void fio_u16x4_mul(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x4_or`

```c
void fio_u16x4_or(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x4_reduce_add`

```c
uint16_t fio_u16x4_reduce_add(uint16_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x4_reduce_and`

```c
uint16_t fio_u16x4_reduce_and(uint16_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x4_reduce_max`

```c
uint16_t fio_u16x4_reduce_max(uint16_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x4_reduce_min`

```c
uint16_t fio_u16x4_reduce_min(uint16_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x4_reduce_mul`

```c
uint16_t fio_u16x4_reduce_mul(uint16_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x4_reduce_or`

```c
uint16_t fio_u16x4_reduce_or(uint16_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x4_reduce_sub`

```c
uint16_t fio_u16x4_reduce_sub(uint16_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x4_reduce_xor`

```c
uint16_t fio_u16x4_reduce_xor(uint16_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x4_reshuffle`

```c
void fio_u16x4_reshuffle(uint16_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u16x4_sub`

```c
void fio_u16x4_sub(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x4_xor`

```c
void fio_u16x4_xor(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x8_add`

```c
void fio_u16x8_add(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x8_and`

```c
void fio_u16x8_and(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x8_mul`

```c
void fio_u16x8_mul(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x8_or`

```c
void fio_u16x8_or(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x8_reduce_add`

```c
uint16_t fio_u16x8_reduce_add(uint16_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x8_reduce_and`

```c
uint16_t fio_u16x8_reduce_and(uint16_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x8_reduce_max`

```c
uint16_t fio_u16x8_reduce_max(uint16_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x8_reduce_min`

```c
uint16_t fio_u16x8_reduce_min(uint16_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x8_reduce_mul`

```c
uint16_t fio_u16x8_reduce_mul(uint16_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x8_reduce_or`

```c
uint16_t fio_u16x8_reduce_or(uint16_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x8_reduce_sub`

```c
uint16_t fio_u16x8_reduce_sub(uint16_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x8_reduce_xor`

```c
uint16_t fio_u16x8_reduce_xor(uint16_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x8_reshuffle`

```c
void fio_u16x8_reshuffle(uint16_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u16x8_sub`

```c
void fio_u16x8_sub(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x8_xor`

```c
void fio_u16x8_xor(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x16_add`

```c
void fio_u16x16_add(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x16_and`

```c
void fio_u16x16_and(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x16_mul`

```c
void fio_u16x16_mul(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x16_or`

```c
void fio_u16x16_or(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x16_reduce_add`

```c
uint16_t fio_u16x16_reduce_add(uint16_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x16_reduce_and`

```c
uint16_t fio_u16x16_reduce_and(uint16_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x16_reduce_max`

```c
uint16_t fio_u16x16_reduce_max(uint16_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x16_reduce_min`

```c
uint16_t fio_u16x16_reduce_min(uint16_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x16_reduce_mul`

```c
uint16_t fio_u16x16_reduce_mul(uint16_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x16_reduce_or`

```c
uint16_t fio_u16x16_reduce_or(uint16_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x16_reduce_sub`

```c
uint16_t fio_u16x16_reduce_sub(uint16_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x16_reduce_xor`

```c
uint16_t fio_u16x16_reduce_xor(uint16_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x16_reshuffle`

```c
void fio_u16x16_reshuffle(uint16_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u16x16_sub`

```c
void fio_u16x16_sub(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x16_xor`

```c
void fio_u16x16_xor(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x32_add`

```c
void fio_u16x32_add(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x32_and`

```c
void fio_u16x32_and(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x32_mul`

```c
void fio_u16x32_mul(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x32_or`

```c
void fio_u16x32_or(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x32_reduce_add`

```c
uint16_t fio_u16x32_reduce_add(uint16_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x32_reduce_and`

```c
uint16_t fio_u16x32_reduce_and(uint16_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x32_reduce_max`

```c
uint16_t fio_u16x32_reduce_max(uint16_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x32_reduce_min`

```c
uint16_t fio_u16x32_reduce_min(uint16_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x32_reduce_mul`

```c
uint16_t fio_u16x32_reduce_mul(uint16_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x32_reduce_or`

```c
uint16_t fio_u16x32_reduce_or(uint16_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x32_reduce_sub`

```c
uint16_t fio_u16x32_reduce_sub(uint16_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x32_reduce_xor`

```c
uint16_t fio_u16x32_reduce_xor(uint16_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x32_reshuffle`

```c
void fio_u16x32_reshuffle(uint16_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u16x32_sub`

```c
void fio_u16x32_sub(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x32_xor`

```c
void fio_u16x32_xor(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x64_add`

```c
void fio_u16x64_add(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x64_and`

```c
void fio_u16x64_and(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x64_mul`

```c
void fio_u16x64_mul(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x64_or`

```c
void fio_u16x64_or(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x64_reduce_add`

```c
uint16_t fio_u16x64_reduce_add(uint16_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x64_reduce_and`

```c
uint16_t fio_u16x64_reduce_and(uint16_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x64_reduce_max`

```c
uint16_t fio_u16x64_reduce_max(uint16_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x64_reduce_min`

```c
uint16_t fio_u16x64_reduce_min(uint16_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x64_reduce_mul`

```c
uint16_t fio_u16x64_reduce_mul(uint16_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x64_reduce_or`

```c
uint16_t fio_u16x64_reduce_or(uint16_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x64_reduce_sub`

```c
uint16_t fio_u16x64_reduce_sub(uint16_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x64_reduce_xor`

```c
uint16_t fio_u16x64_reduce_xor(uint16_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x64_reshuffle`

```c
void fio_u16x64_reshuffle(uint16_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u16x64_sub`

```c
void fio_u16x64_sub(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x64_xor`

```c
void fio_u16x64_xor(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x128_add`

```c
void fio_u16x128_add(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x128_and`

```c
void fio_u16x128_and(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x128_mul`

```c
void fio_u16x128_mul(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x128_or`

```c
void fio_u16x128_or(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x128_reduce_add`

```c
uint16_t fio_u16x128_reduce_add(uint16_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x128_reduce_and`

```c
uint16_t fio_u16x128_reduce_and(uint16_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x128_reduce_max`

```c
uint16_t fio_u16x128_reduce_max(uint16_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x128_reduce_min`

```c
uint16_t fio_u16x128_reduce_min(uint16_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u16x128_reduce_mul`

```c
uint16_t fio_u16x128_reduce_mul(uint16_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x128_reduce_or`

```c
uint16_t fio_u16x128_reduce_or(uint16_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x128_reduce_sub`

```c
uint16_t fio_u16x128_reduce_sub(uint16_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x128_reduce_xor`

```c
uint16_t fio_u16x128_reduce_xor(uint16_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u16x128_reshuffle`

```c
void fio_u16x128_reshuffle(uint16_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u16x128_sub`

```c
void fio_u16x128_sub(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u16x128_xor`

```c
void fio_u16x128_xor(uint16_t *dest, uint16_t *a, uint16_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x2_add`

```c
void fio_u32x2_add(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x2_and`

```c
void fio_u32x2_and(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x2_mul`

```c
void fio_u32x2_mul(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x2_or`

```c
void fio_u32x2_or(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x2_reduce_add`

```c
uint32_t fio_u32x2_reduce_add(uint32_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x2_reduce_and`

```c
uint32_t fio_u32x2_reduce_and(uint32_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x2_reduce_max`

```c
uint32_t fio_u32x2_reduce_max(uint32_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x2_reduce_min`

```c
uint32_t fio_u32x2_reduce_min(uint32_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x2_reduce_mul`

```c
uint32_t fio_u32x2_reduce_mul(uint32_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x2_reduce_or`

```c
uint32_t fio_u32x2_reduce_or(uint32_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x2_reduce_sub`

```c
uint32_t fio_u32x2_reduce_sub(uint32_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x2_reduce_xor`

```c
uint32_t fio_u32x2_reduce_xor(uint32_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x2_reshuffle`

```c
void fio_u32x2_reshuffle(uint32_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u32x2_sub`

```c
void fio_u32x2_sub(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x2_xor`

```c
void fio_u32x2_xor(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x4_add`

```c
void fio_u32x4_add(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x4_and`

```c
void fio_u32x4_and(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x4_mul`

```c
void fio_u32x4_mul(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x4_or`

```c
void fio_u32x4_or(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x4_reduce_add`

```c
uint32_t fio_u32x4_reduce_add(uint32_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x4_reduce_and`

```c
uint32_t fio_u32x4_reduce_and(uint32_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x4_reduce_max`

```c
uint32_t fio_u32x4_reduce_max(uint32_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x4_reduce_min`

```c
uint32_t fio_u32x4_reduce_min(uint32_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x4_reduce_mul`

```c
uint32_t fio_u32x4_reduce_mul(uint32_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x4_reduce_or`

```c
uint32_t fio_u32x4_reduce_or(uint32_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x4_reduce_sub`

```c
uint32_t fio_u32x4_reduce_sub(uint32_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x4_reduce_xor`

```c
uint32_t fio_u32x4_reduce_xor(uint32_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x4_reshuffle`

```c
void fio_u32x4_reshuffle(uint32_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u32x4_sub`

```c
void fio_u32x4_sub(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x4_xor`

```c
void fio_u32x4_xor(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x8_add`

```c
void fio_u32x8_add(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x8_and`

```c
void fio_u32x8_and(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x8_mul`

```c
void fio_u32x8_mul(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x8_or`

```c
void fio_u32x8_or(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x8_reduce_add`

```c
uint32_t fio_u32x8_reduce_add(uint32_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x8_reduce_and`

```c
uint32_t fio_u32x8_reduce_and(uint32_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x8_reduce_max`

```c
uint32_t fio_u32x8_reduce_max(uint32_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x8_reduce_min`

```c
uint32_t fio_u32x8_reduce_min(uint32_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x8_reduce_mul`

```c
uint32_t fio_u32x8_reduce_mul(uint32_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x8_reduce_or`

```c
uint32_t fio_u32x8_reduce_or(uint32_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x8_reduce_sub`

```c
uint32_t fio_u32x8_reduce_sub(uint32_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x8_reduce_xor`

```c
uint32_t fio_u32x8_reduce_xor(uint32_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x8_reshuffle`

```c
void fio_u32x8_reshuffle(uint32_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u32x8_sub`

```c
void fio_u32x8_sub(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x8_xor`

```c
void fio_u32x8_xor(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x16_add`

```c
void fio_u32x16_add(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x16_and`

```c
void fio_u32x16_and(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x16_mul`

```c
void fio_u32x16_mul(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x16_or`

```c
void fio_u32x16_or(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x16_reduce_add`

```c
uint32_t fio_u32x16_reduce_add(uint32_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x16_reduce_and`

```c
uint32_t fio_u32x16_reduce_and(uint32_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x16_reduce_max`

```c
uint32_t fio_u32x16_reduce_max(uint32_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x16_reduce_min`

```c
uint32_t fio_u32x16_reduce_min(uint32_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x16_reduce_mul`

```c
uint32_t fio_u32x16_reduce_mul(uint32_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x16_reduce_or`

```c
uint32_t fio_u32x16_reduce_or(uint32_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x16_reduce_sub`

```c
uint32_t fio_u32x16_reduce_sub(uint32_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x16_reduce_xor`

```c
uint32_t fio_u32x16_reduce_xor(uint32_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x16_reshuffle`

```c
void fio_u32x16_reshuffle(uint32_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u32x16_sub`

```c
void fio_u32x16_sub(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x16_xor`

```c
void fio_u32x16_xor(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x32_add`

```c
void fio_u32x32_add(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x32_and`

```c
void fio_u32x32_and(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x32_mul`

```c
void fio_u32x32_mul(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x32_or`

```c
void fio_u32x32_or(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x32_reduce_add`

```c
uint32_t fio_u32x32_reduce_add(uint32_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x32_reduce_and`

```c
uint32_t fio_u32x32_reduce_and(uint32_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x32_reduce_max`

```c
uint32_t fio_u32x32_reduce_max(uint32_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x32_reduce_min`

```c
uint32_t fio_u32x32_reduce_min(uint32_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x32_reduce_mul`

```c
uint32_t fio_u32x32_reduce_mul(uint32_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x32_reduce_or`

```c
uint32_t fio_u32x32_reduce_or(uint32_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x32_reduce_sub`

```c
uint32_t fio_u32x32_reduce_sub(uint32_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x32_reduce_xor`

```c
uint32_t fio_u32x32_reduce_xor(uint32_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x32_reshuffle`

```c
void fio_u32x32_reshuffle(uint32_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u32x32_sub`

```c
void fio_u32x32_sub(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x32_xor`

```c
void fio_u32x32_xor(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x64_add`

```c
void fio_u32x64_add(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x64_and`

```c
void fio_u32x64_and(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x64_mul`

```c
void fio_u32x64_mul(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x64_or`

```c
void fio_u32x64_or(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x64_reduce_add`

```c
uint32_t fio_u32x64_reduce_add(uint32_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x64_reduce_and`

```c
uint32_t fio_u32x64_reduce_and(uint32_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x64_reduce_max`

```c
uint32_t fio_u32x64_reduce_max(uint32_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x64_reduce_min`

```c
uint32_t fio_u32x64_reduce_min(uint32_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u32x64_reduce_mul`

```c
uint32_t fio_u32x64_reduce_mul(uint32_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x64_reduce_or`

```c
uint32_t fio_u32x64_reduce_or(uint32_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x64_reduce_sub`

```c
uint32_t fio_u32x64_reduce_sub(uint32_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x64_reduce_xor`

```c
uint32_t fio_u32x64_reduce_xor(uint32_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u32x64_reshuffle`

```c
void fio_u32x64_reshuffle(uint32_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u32x64_sub`

```c
void fio_u32x64_sub(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u32x64_xor`

```c
void fio_u32x64_xor(uint32_t *dest, uint32_t *a, uint32_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x2_add`

```c
void fio_u64x2_add(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x2_and`

```c
void fio_u64x2_and(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x2_mul`

```c
void fio_u64x2_mul(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x2_or`

```c
void fio_u64x2_or(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x2_reduce_add`

```c
uint64_t fio_u64x2_reduce_add(uint64_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x2_reduce_and`

```c
uint64_t fio_u64x2_reduce_and(uint64_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x2_reduce_max`

```c
uint64_t fio_u64x2_reduce_max(uint64_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x2_reduce_min`

```c
uint64_t fio_u64x2_reduce_min(uint64_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x2_reduce_mul`

```c
uint64_t fio_u64x2_reduce_mul(uint64_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x2_reduce_or`

```c
uint64_t fio_u64x2_reduce_or(uint64_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x2_reduce_sub`

```c
uint64_t fio_u64x2_reduce_sub(uint64_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x2_reduce_xor`

```c
uint64_t fio_u64x2_reduce_xor(uint64_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x2_reshuffle`

```c
void fio_u64x2_reshuffle(uint64_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u64x2_sub`

```c
void fio_u64x2_sub(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x2_xor`

```c
void fio_u64x2_xor(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x4_add`

```c
void fio_u64x4_add(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x4_and`

```c
void fio_u64x4_and(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x4_mul`

```c
void fio_u64x4_mul(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x4_or`

```c
void fio_u64x4_or(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x4_reduce_add`

```c
uint64_t fio_u64x4_reduce_add(uint64_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x4_reduce_and`

```c
uint64_t fio_u64x4_reduce_and(uint64_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x4_reduce_max`

```c
uint64_t fio_u64x4_reduce_max(uint64_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x4_reduce_min`

```c
uint64_t fio_u64x4_reduce_min(uint64_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x4_reduce_mul`

```c
uint64_t fio_u64x4_reduce_mul(uint64_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x4_reduce_or`

```c
uint64_t fio_u64x4_reduce_or(uint64_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x4_reduce_sub`

```c
uint64_t fio_u64x4_reduce_sub(uint64_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x4_reduce_xor`

```c
uint64_t fio_u64x4_reduce_xor(uint64_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x4_reshuffle`

```c
void fio_u64x4_reshuffle(uint64_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u64x4_sub`

```c
void fio_u64x4_sub(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x4_xor`

```c
void fio_u64x4_xor(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x8_add`

```c
void fio_u64x8_add(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x8_and`

```c
void fio_u64x8_and(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x8_mul`

```c
void fio_u64x8_mul(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x8_or`

```c
void fio_u64x8_or(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x8_reduce_add`

```c
uint64_t fio_u64x8_reduce_add(uint64_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x8_reduce_and`

```c
uint64_t fio_u64x8_reduce_and(uint64_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x8_reduce_max`

```c
uint64_t fio_u64x8_reduce_max(uint64_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x8_reduce_min`

```c
uint64_t fio_u64x8_reduce_min(uint64_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x8_reduce_mul`

```c
uint64_t fio_u64x8_reduce_mul(uint64_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x8_reduce_or`

```c
uint64_t fio_u64x8_reduce_or(uint64_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x8_reduce_sub`

```c
uint64_t fio_u64x8_reduce_sub(uint64_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x8_reduce_xor`

```c
uint64_t fio_u64x8_reduce_xor(uint64_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x8_reshuffle`

```c
void fio_u64x8_reshuffle(uint64_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u64x8_sub`

```c
void fio_u64x8_sub(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x8_xor`

```c
void fio_u64x8_xor(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x16_add`

```c
void fio_u64x16_add(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x16_and`

```c
void fio_u64x16_and(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x16_mul`

```c
void fio_u64x16_mul(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x16_or`

```c
void fio_u64x16_or(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x16_reduce_add`

```c
uint64_t fio_u64x16_reduce_add(uint64_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x16_reduce_and`

```c
uint64_t fio_u64x16_reduce_and(uint64_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x16_reduce_max`

```c
uint64_t fio_u64x16_reduce_max(uint64_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x16_reduce_min`

```c
uint64_t fio_u64x16_reduce_min(uint64_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x16_reduce_mul`

```c
uint64_t fio_u64x16_reduce_mul(uint64_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x16_reduce_or`

```c
uint64_t fio_u64x16_reduce_or(uint64_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x16_reduce_sub`

```c
uint64_t fio_u64x16_reduce_sub(uint64_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x16_reduce_xor`

```c
uint64_t fio_u64x16_reduce_xor(uint64_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x16_reshuffle`

```c
void fio_u64x16_reshuffle(uint64_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u64x16_sub`

```c
void fio_u64x16_sub(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x16_xor`

```c
void fio_u64x16_xor(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x32_add`

```c
void fio_u64x32_add(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x32_and`

```c
void fio_u64x32_and(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a & b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x32_mul`

```c
void fio_u64x32_mul(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x32_or`

```c
void fio_u64x32_or(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a | b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x32_reduce_add`

```c
uint64_t fio_u64x32_reduce_add(uint64_t *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x32_reduce_and`

```c
uint64_t fio_u64x32_reduce_and(uint64_t *v)
```

Performs `and` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x32_reduce_max`

```c
uint64_t fio_u64x32_reduce_max(uint64_t *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x32_reduce_min`

```c
uint64_t fio_u64x32_reduce_min(uint64_t *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_u64x32_reduce_mul`

```c
uint64_t fio_u64x32_reduce_mul(uint64_t *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x32_reduce_or`

```c
uint64_t fio_u64x32_reduce_or(uint64_t *v)
```

Performs `or` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x32_reduce_sub`

```c
uint64_t fio_u64x32_reduce_sub(uint64_t *v)
```

Performs `sub` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x32_reduce_xor`

```c
uint64_t fio_u64x32_reduce_xor(uint64_t *v)
```

Performs `xor` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_u64x32_reshuffle`

```c
void fio_u64x32_reshuffle(uint64_t *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u64x32_sub`

```c
void fio_u64x32_sub(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a - b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_u64x32_xor`

```c
void fio_u64x32_xor(uint64_t *dest, uint64_t *a, uint64_t *b)
```

Performs operation `a ^ b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx2_add`

```c
void fio_floatx2_add(float *dest, float *a, float *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx2_mul`

```c
void fio_floatx2_mul(float *dest, float *a, float *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx2_reduce_add`

```c
float fio_floatx2_reduce_add(float *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx2_reduce_max`

```c
float fio_floatx2_reduce_max(float *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx2_reduce_min`

```c
float fio_floatx2_reduce_min(float *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx2_reduce_mul`

```c
float fio_floatx2_reduce_mul(float *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx2_reshuffle`

```c
void fio_floatx2_reshuffle(float *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_floatx4_add`

```c
void fio_floatx4_add(float *dest, float *a, float *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx4_mul`

```c
void fio_floatx4_mul(float *dest, float *a, float *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx4_reduce_add`

```c
float fio_floatx4_reduce_add(float *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx4_reduce_max`

```c
float fio_floatx4_reduce_max(float *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx4_reduce_min`

```c
float fio_floatx4_reduce_min(float *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx4_reduce_mul`

```c
float fio_floatx4_reduce_mul(float *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx4_reshuffle`

```c
void fio_floatx4_reshuffle(float *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_floatx8_add`

```c
void fio_floatx8_add(float *dest, float *a, float *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx8_mul`

```c
void fio_floatx8_mul(float *dest, float *a, float *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx8_reduce_add`

```c
float fio_floatx8_reduce_add(float *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx8_reduce_max`

```c
float fio_floatx8_reduce_max(float *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx8_reduce_min`

```c
float fio_floatx8_reduce_min(float *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx8_reduce_mul`

```c
float fio_floatx8_reduce_mul(float *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx8_reshuffle`

```c
void fio_floatx8_reshuffle(float *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_floatx16_add`

```c
void fio_floatx16_add(float *dest, float *a, float *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx16_mul`

```c
void fio_floatx16_mul(float *dest, float *a, float *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx16_reduce_add`

```c
float fio_floatx16_reduce_add(float *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx16_reduce_max`

```c
float fio_floatx16_reduce_max(float *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx16_reduce_min`

```c
float fio_floatx16_reduce_min(float *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx16_reduce_mul`

```c
float fio_floatx16_reduce_mul(float *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx16_reshuffle`

```c
void fio_floatx16_reshuffle(float *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_floatx32_add`

```c
void fio_floatx32_add(float *dest, float *a, float *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx32_mul`

```c
void fio_floatx32_mul(float *dest, float *a, float *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx32_reduce_add`

```c
float fio_floatx32_reduce_add(float *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx32_reduce_max`

```c
float fio_floatx32_reduce_max(float *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx32_reduce_min`

```c
float fio_floatx32_reduce_min(float *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx32_reduce_mul`

```c
float fio_floatx32_reduce_mul(float *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx32_reshuffle`

```c
void fio_floatx32_reshuffle(float *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_floatx64_add`

```c
void fio_floatx64_add(float *dest, float *a, float *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx64_mul`

```c
void fio_floatx64_mul(float *dest, float *a, float *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_floatx64_reduce_add`

```c
float fio_floatx64_reduce_add(float *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx64_reduce_max`

```c
float fio_floatx64_reduce_max(float *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx64_reduce_min`

```c
float fio_floatx64_reduce_min(float *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_floatx64_reduce_mul`

```c
float fio_floatx64_reduce_mul(float *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_floatx64_reshuffle`

```c
void fio_floatx64_reshuffle(float *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_dblx2_add`

```c
void fio_dblx2_add(double *dest, double *a, double *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx2_mul`

```c
void fio_dblx2_mul(double *dest, double *a, double *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx2_reduce_add`

```c
double fio_dblx2_reduce_add(double *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx2_reduce_max`

```c
double fio_dblx2_reduce_max(double *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx2_reduce_min`

```c
double fio_dblx2_reduce_min(double *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx2_reduce_mul`

```c
double fio_dblx2_reduce_mul(double *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx2_reshuffle`

```c
void fio_dblx2_reshuffle(double *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_dblx4_add`

```c
void fio_dblx4_add(double *dest, double *a, double *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx4_mul`

```c
void fio_dblx4_mul(double *dest, double *a, double *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx4_reduce_add`

```c
double fio_dblx4_reduce_add(double *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx4_reduce_max`

```c
double fio_dblx4_reduce_max(double *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx4_reduce_min`

```c
double fio_dblx4_reduce_min(double *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx4_reduce_mul`

```c
double fio_dblx4_reduce_mul(double *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx4_reshuffle`

```c
void fio_dblx4_reshuffle(double *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_dblx8_add`

```c
void fio_dblx8_add(double *dest, double *a, double *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx8_mul`

```c
void fio_dblx8_mul(double *dest, double *a, double *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx8_reduce_add`

```c
double fio_dblx8_reduce_add(double *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx8_reduce_max`

```c
double fio_dblx8_reduce_max(double *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx8_reduce_min`

```c
double fio_dblx8_reduce_min(double *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx8_reduce_mul`

```c
double fio_dblx8_reduce_mul(double *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx8_reshuffle`

```c
void fio_dblx8_reshuffle(double *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_dblx16_add`

```c
void fio_dblx16_add(double *dest, double *a, double *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx16_mul`

```c
void fio_dblx16_mul(double *dest, double *a, double *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx16_reduce_add`

```c
double fio_dblx16_reduce_add(double *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx16_reduce_max`

```c
double fio_dblx16_reduce_max(double *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx16_reduce_min`

```c
double fio_dblx16_reduce_min(double *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx16_reduce_mul`

```c
double fio_dblx16_reduce_mul(double *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx16_reshuffle`

```c
void fio_dblx16_reshuffle(double *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_dblx32_add`

```c
void fio_dblx32_add(double *dest, double *a, double *b)
```

Performs operation `a + b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx32_mul`

```c
void fio_dblx32_mul(double *dest, double *a, double *b)
```

Performs operation `a * b`, storing the result in `dest`.

_Symbol type:_ `function`

#### `fio_dblx32_reduce_add`

```c
double fio_dblx32_reduce_add(double *v)
```

Performs `add` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx32_reduce_max`

```c
double fio_dblx32_reduce_max(double *v)
```

Returns the maximum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx32_reduce_min`

```c
double fio_dblx32_reduce_min(double *v)
```

Returns the minimum value in a vector.

_Symbol type:_ `function`

#### `fio_dblx32_reduce_mul`

```c
double fio_dblx32_reduce_mul(double *v)
```

Performs `mul` in order along the vector, returning the result.

_Symbol type:_ `function`

#### `fio_dblx32_reshuffle`

```c
void fio_dblx32_reshuffle(double *v, uint8_t *indx)
```

Reorders (shuffles) the vectors using the indexes in `indx`.

_Symbol type:_ `function`

#### `fio_u8x4_reshuffle`

```c
#define fio_u8x4_reshuffle(v, ...)     fio_u8x4_reshuffle(v,     (uint8_t[4]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u8x8_reshuffle`

```c
#define fio_u8x8_reshuffle(v, ...)     fio_u8x8_reshuffle(v,     (uint8_t[8]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u8x16_reshuffle`

```c
#define fio_u8x16_reshuffle(v, ...)    fio_u8x16_reshuffle(v,    (uint8_t[16]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u8x32_reshuffle`

```c
#define fio_u8x32_reshuffle(v, ...)    fio_u8x32_reshuffle(v,    (uint8_t[32]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u8x64_reshuffle`

```c
#define fio_u8x64_reshuffle(v, ...)    fio_u8x64_reshuffle(v,    (uint8_t[64]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u8x128_reshuffle`

```c
#define fio_u8x128_reshuffle(v, ...)   fio_u8x128_reshuffle(v,   (uint8_t[128]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u8x256_reshuffle`

```c
#define fio_u8x256_reshuffle(v, ...)   fio_u8x256_reshuffle(v,   (uint8_t[256]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u16x2_reshuffle`

```c
#define fio_u16x2_reshuffle(v, ...)    fio_u16x2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u16x4_reshuffle`

```c
#define fio_u16x4_reshuffle(v, ...)    fio_u16x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u16x8_reshuffle`

```c
#define fio_u16x8_reshuffle(v, ...)    fio_u16x8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u16x16_reshuffle`

```c
#define fio_u16x16_reshuffle(v, ...)   fio_u16x16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u16x32_reshuffle`

```c
#define fio_u16x32_reshuffle(v, ...)   fio_u16x32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u16x64_reshuffle`

```c
#define fio_u16x64_reshuffle(v, ...)   fio_u16x64_reshuffle(v,   (uint8_t[64]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u16x128_reshuffle`

```c
#define fio_u16x128_reshuffle(v,...)   fio_u16x128_reshuffle(v,  (uint8_t[128]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u32x2_reshuffle`

```c
#define fio_u32x2_reshuffle(v, ...)    fio_u32x2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u32x4_reshuffle`

```c
#define fio_u32x4_reshuffle(v, ...)    fio_u32x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u32x8_reshuffle`

```c
#define fio_u32x8_reshuffle(v, ...)    fio_u32x8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u32x16_reshuffle`

```c
#define fio_u32x16_reshuffle(v, ...)   fio_u32x16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u32x32_reshuffle`

```c
#define fio_u32x32_reshuffle(v, ...)   fio_u32x32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u32x64_reshuffle`

```c
#define fio_u32x64_reshuffle(v, ...)   fio_u32x64_reshuffle(v,   (uint8_t[64]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u64x2_reshuffle`

```c
#define fio_u64x2_reshuffle(v, ...)    fio_u64x2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u64x4_reshuffle`

```c
#define fio_u64x4_reshuffle(v, ...)    fio_u64x4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u64x8_reshuffle`

```c
#define fio_u64x8_reshuffle(v, ...)    fio_u64x8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u64x16_reshuffle`

```c
#define fio_u64x16_reshuffle(v, ...)   fio_u64x16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u64x32_reshuffle`

```c
#define fio_u64x32_reshuffle(v, ...)   fio_u64x32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_floatx2_reshuffle`

```c
#define fio_floatx2_reshuffle(v, ...)  fio_floatx2_reshuffle(v,  (uint8_t[2]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_floatx4_reshuffle`

```c
#define fio_floatx4_reshuffle(v, ...)  fio_floatx4_reshuffle(v,  (uint8_t[4]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_floatx8_reshuffle`

```c
#define fio_floatx8_reshuffle(v, ...)  fio_floatx8_reshuffle(v,  (uint8_t[8]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_floatx16_reshuffle`

```c
#define fio_floatx16_reshuffle(v, ...) fio_floatx16_reshuffle(v, (uint8_t[16]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_floatx32_reshuffle`

```c
#define fio_floatx32_reshuffle(v, ...) fio_floatx32_reshuffle(v, (uint8_t[32]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_floatx64_reshuffle`

```c
#define fio_floatx64_reshuffle(v, ...) fio_floatx64_reshuffle(v, (uint8_t[64]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_dblx2_reshuffle`

```c
#define fio_dblx2_reshuffle(v, ...)    fio_dblx2_reshuffle(v,    (uint8_t[2]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_dblx4_reshuffle`

```c
#define fio_dblx4_reshuffle(v, ...)    fio_dblx4_reshuffle(v,    (uint8_t[4]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_dblx8_reshuffle`

```c
#define fio_dblx8_reshuffle(v, ...)    fio_dblx8_reshuffle(v,    (uint8_t[8]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_dblx16_reshuffle`

```c
#define fio_dblx16_reshuffle(v, ...)   fio_dblx16_reshuffle(v,   (uint8_t[16]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_dblx32_reshuffle`

```c
#define fio_dblx32_reshuffle(v, ...)   fio_dblx32_reshuffle(v,   (uint8_t[32]){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ct_true`

```c
inline FIO_CONST uintmax_t fio_ct_true(uintmax_t cond)
```

Returns 1 if the expression is true (input isn't zero).

_Symbol type:_ `function`

#### `fio_ct_false`

```c
inline FIO_CONST uintmax_t fio_ct_false(uintmax_t cond)
```

Returns 1 if the expression is false (input is zero).

_Symbol type:_ `function`

#### `fio_ct_if_bool`

```c
inline FIO_CONST uintmax_t fio_ct_if_bool(uintmax_t cond, uintmax_t a, uintmax_t b)
```

Returns `a` if `cond` is boolean and true, returns b otherwise.

_Symbol type:_ `function`

#### `fio_ct_if`

```c
inline FIO_CONST uintmax_t fio_ct_if(uintmax_t cond, uintmax_t a, uintmax_t b)
```

Returns `a` if `cond` isn't zero (uses fio_ct_true), returns b otherwise.

_Symbol type:_ `function`

#### `fio_ct_max`

```c
inline FIO_CONST intmax_t fio_ct_max(intmax_t a_, intmax_t b_)
```

Returns `a` if a >= `b`.

_Symbol type:_ `function`

#### `fio_ct_min`

```c
inline FIO_CONST intmax_t fio_ct_min(intmax_t a_, intmax_t b_)
```

Returns `a` if a >= `b`.

_Symbol type:_ `function`

#### `fio_ct_abs`

```c
inline FIO_CONST uintmax_t fio_ct_abs(intmax_t i_)
```

Returns absolute value.

_Symbol type:_ `function`

#### `fio_ct_tolower`

```c
inline char fio_ct_tolower(char c)
```



_Symbol type:_ `function`

#### `fio_ct_mux32`

```c
inline FIO_CONST uint32_t fio_ct_mux32(uint32_t x, uint32_t y, uint32_t z)
```

Bitwise "choose": for each bit, if x is set, return y's bit, else z's bit.
Formula: (x & y) ^ (~x & z)  -or equivalently-  z ^ (x & (y ^ z))
Used in: SHA-1 (Ch), SHA-256 (Ch), SHA-512 (Ch), AES, etc.

_Symbol type:_ `function`

#### `fio_ct_mux64`

```c
inline FIO_CONST uint64_t fio_ct_mux64(uint64_t x, uint64_t y, uint64_t z)
```

64-bit version of bitwise choose/mux.

_Symbol type:_ `function`

#### `fio_ct_maj32`

```c
inline FIO_CONST uint32_t fio_ct_maj32(uint32_t x, uint32_t y, uint32_t z)
```

Bitwise "majority": for each bit position, return 1 if 2+ inputs have 1.
Formula: (x & y) ^ (x & z) ^ (y & z)  -or-  (x & y) | (z & (x | y))
Used in: SHA-1 (Maj), SHA-256 (Maj), SHA-512 (Maj), etc.

_Symbol type:_ `function`

#### `fio_ct_maj64`

```c
inline FIO_CONST uint64_t fio_ct_maj64(uint64_t x, uint64_t y, uint64_t z)
```

64-bit version of bitwise majority.

_Symbol type:_ `function`

#### `fio_ct_xor3_32`

```c
inline FIO_CONST uint32_t fio_ct_xor3_32(uint32_t x, uint32_t y, uint32_t z)
```

Bitwise "parity": XOR of all three inputs (1 if odd number of 1s).
Formula: x ^ y ^ z
Used in: SHA-1 (Parity function for rounds 20-39 and 60-79)

_Symbol type:_ `function`

#### `fio_ct_xor3_64`

```c
inline FIO_CONST uint64_t fio_ct_xor3_64(uint64_t x, uint64_t y, uint64_t z)
```

64-bit version of 3-way XOR.

_Symbol type:_ `function`

#### `fio_ct_is_eq`

```c
FIO_SFUNC _Bool fio_ct_is_eq(const void *a_, const void *b_, size_t bytes)
```

A timing attack resistant memory comparison function.

_Symbol type:_ `function`

#### `fio_secure_zero`

```c
inline void fio_secure_zero(void *a_, size_t bytes)
```

A timing attack resistant memory zeroing function.

_Symbol type:_ `function`

#### `fio_lrot8`

```c
#define fio_lrot8(i, bits) __builtin_rotateleft8(i, bits)
```

8Bit left rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lrot16`

```c
#define fio_lrot16(i, bits) __builtin_rotateleft16(i, bits)
```

16Bit left rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lrot32`

```c
#define fio_lrot32(i, bits) __builtin_rotateleft32(i, bits)
```

32Bit left rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lrot64`

```c
#define fio_lrot64(i, bits) __builtin_rotateleft64(i, bits)
```

64Bit left rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_rrot8`

```c
#define fio_rrot8(i, bits) __builtin_rotateright8(i, bits)
```

8Bit right rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_rrot8`

```c
inline FIO_CONST uint8_t fio_rrot8(uint8_t i, uint8_t bits)
```

8Bit right rotation, inlined.

_Symbol type:_ `function`

#### `fio_rrot16`

```c
#define fio_rrot16(i, bits) __builtin_rotateright16(i, bits)
```

16Bit right rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_rrot32`

```c
#define fio_rrot32(i, bits) __builtin_rotateright32(i, bits)
```

32Bit right rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_rrot64`

```c
#define fio_rrot64(i, bits) __builtin_rotateright64(i, bits)
```

64Bit right rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lrot128`

```c
#define fio_lrot128(i, bits) __builtin_rotateleft128(i, bits)
```

128Bit left rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_rrot128`

```c
#define fio_rrot128(i, bits) __builtin_rotateright128(i, bits)
```

128Bit right rotation, inlined.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lrot128`

```c
inline FIO_CONST __uint128_t fio_lrot128(__uint128_t i, uint8_t bits)
```

128Bit left rotation, inlined.

_Symbol type:_ `function`

#### `fio_rrot128`

```c
inline FIO_CONST __uint128_t fio_rrot128(__uint128_t i, uint8_t bits)
```

128Bit right rotation, inlined.

_Symbol type:_ `function`

#### `fio_frot16`

```c
#define fio_frot16 fio_rrot16
```

Rotates the bits Forwards (endian specific).

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_frot32`

```c
#define fio_frot32 fio_rrot32
```

Rotates the bits Forwards (endian specific).

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_frot64`

```c
#define fio_frot64 fio_rrot64
```

Rotates the bits Forwards (endian specific).

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_xor_rrot3_32`

```c
inline FIO_CONST uint32_t fio_xor_rrot3_32(uint32_t x, uint8_t a, uint8_t b, uint8_t c)
```

XOR of three right rotations: ROTR(x,a) ^ ROTR(x,b) ^ ROTR(x,c)
Common in SHA-256/SHA-512 Sigma functions.

_Symbol type:_ `function`

#### `fio_xor_rrot3_64`

```c
inline FIO_CONST uint64_t fio_xor_rrot3_64(uint64_t x, uint8_t a, uint8_t b, uint8_t c)
```

64-bit version of triple rotation XOR.

_Symbol type:_ `function`

#### `fio_xor_rrot2_shr_32`

```c
inline FIO_CONST uint32_t fio_xor_rrot2_shr_32(uint32_t x, uint8_t a, uint8_t b, uint8_t c)
```

XOR of two right rotations and a right shift: ROTR(x,a) ^ ROTR(x,b) ^
SHR(x,c) Common in SHA-256/SHA-512 sigma (lowercase) functions for message
schedule.

_Symbol type:_ `function`

#### `fio_xor_rrot2_shr_64`

```c
inline FIO_CONST uint64_t fio_xor_rrot2_shr_64(uint64_t x, uint8_t a, uint8_t b, uint8_t c)
```

64-bit version of double rotation + shift XOR.

_Symbol type:_ `function`

#### `fio_xmask`

```c
inline void fio_xmask(void *buf_, size_t len, uint64_t mask)
```

Masks data using a persistent 64 bit mask.

When the buffer's memory is aligned, the function may perform significantly
better.

_Symbol type:_ `function`

#### `fio_xmask_cpy`

```c
inline void fio_xmask_cpy(char *restrict dest, const char *src, size_t len, uint64_t mask)
```

Masks data using a persistent 64 bit mask.

When the buffer's memory is aligned, the function may perform significantly
better.

_Symbol type:_ `function`

#### `fio_popcount`

```c
#define fio_popcount(n) __builtin_popcountll(n)
```

performs a `popcount` operation to count the set bits.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_hemming_dist`

```c
#define fio_hemming_dist(n1, n2) fio_popcount(((uint64_t)(n1) ^ (uint64_t)(n2)))
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lsb_index_unsafe`

```c
FIO_SFUNC FIO_CONST size_t fio_lsb_index_unsafe(uint64_t i)
```

Returns the index of the least significant (lowest) bit.

_Symbol type:_ `function`

#### `fio_msb_index_unsafe`

```c
FIO_SFUNC FIO_CONST size_t fio_msb_index_unsafe(uint64_t i)
```

Returns the index of the most significant (highest) bit.

_Symbol type:_ `function`

#### `fio_has_zero_byte32`

```c
inline FIO_CONST uint32_t fio_has_zero_byte32(uint32_t row)
```

Detects a byte where no bits are set (0) within a 4 byte vector.

The zero byte will be be set to 0x80, all other bytes will be 0x0.

_Symbol type:_ `function`

#### `fio_has_byte32`

```c
inline FIO_CONST uint32_t fio_has_byte32(uint32_t row, uint8_t byte)
```

Detects if `byte` exists within a 4 byte vector.

The requested byte will be be set to 0x80, all other bytes will be 0x0.

_Symbol type:_ `function`

#### `fio_has_full_byte32`

```c
inline FIO_CONST uint32_t fio_has_full_byte32(uint32_t row)
```

Detects a byte where all the bits are set (255) within a 4 byte vector.

The full byte will be be set to 0x80, all other bytes will be 0x0.

_Symbol type:_ `function`

#### `fio_has_zero_byte64`

```c
inline FIO_CONST uint64_t fio_has_zero_byte64(uint64_t row)
```

Detects a byte where no bits are set (0) within an 8 byte vector.

The zero byte will be be set to 0x80, all other bytes will be 0x0.

_Symbol type:_ `function`

#### `fio_has_zero_byte_alt64`

```c
inline FIO_CONST uint64_t fio_has_zero_byte_alt64(uint64_t row)
```

Detects a byte where no bits are set (0) within an 8 byte vector.

This variation should NOT be used to build a bitmap, but May be used to
detect the first occurrence.

_Symbol type:_ `function`

#### `fio_has_byte64`

```c
inline FIO_CONST uint64_t fio_has_byte64(uint64_t row, uint8_t byte)
```

Detects if `byte` exists within an 8 byte vector.

The requested byte will be be set to 0x80, all other bytes will be 0x0.

_Symbol type:_ `function`

#### `fio_has_full_byte64`

```c
inline FIO_CONST uint64_t fio_has_full_byte64(uint64_t row)
```

Detects a byte where all the bits are set (255) within an 8 byte vector.

The full byte will be be set to 0x80, all other bytes will be 0x0.

_Symbol type:_ `function`

#### `fio_has_byte2bitmap`

```c
inline FIO_CONST uint64_t fio_has_byte2bitmap(uint64_t result)
```

Converts a `fio_has_byteX` result to a bitmap.

_Symbol type:_ `function`

#### `fio_bits_lsb`

```c
inline FIO_CONST uint64_t fio_bits_lsb(uint64_t i)
```

Isolates the least significant (lowest) bit.

_Symbol type:_ `function`

#### `fio_bits_msb`

```c
inline FIO_CONST uint64_t fio_bits_msb(uint64_t i)
```

Isolates the most significant (highest) bit.

_Symbol type:_ `function`

#### `fio_bits_msb_index`

```c
inline FIO_CONST size_t fio_bits_msb_index(uint64_t i)
```

Returns the index of the most significant (highest) bit.

_Symbol type:_ `function`

#### `fio_bits_lsb_index`

```c
inline FIO_CONST size_t fio_bits_lsb_index(uint64_t i)
```

Returns the index of the least significant (lowest) bit.

_Symbol type:_ `function`

#### `fio_bit_get`

```c
inline uint8_t fio_bit_get(void *map, size_t bit)
```

Gets the state of a bit in a bitmap.

_Symbol type:_ `function`

#### `fio_bit_set`

```c
inline void fio_bit_set(void *map, size_t bit)
```

Sets the a bit in a bitmap (sets to 1).

_Symbol type:_ `function`

#### `fio_bit_unset`

```c
inline void fio_bit_unset(void *map, size_t bit)
```

Unsets the a bit in a bitmap (sets to 0).

_Symbol type:_ `function`

#### `fio_bit_flip`

```c
inline void fio_bit_flip(void *map, size_t bit)
```

Flips the a bit in a bitmap (sets to 0 if 1, sets to 1 if 0).

_Symbol type:_ `function`

#### `fio_math_mod_mul64`

```c
inline uint64_t fio_math_mod_mul64(uint64_t a, uint64_t b, uint64_t mod)
```

Perform modular multiplication for numbers with up to 64 bits.

_Symbol type:_ `function`

#### `fio_math_mod_expo64`

```c
inline uint64_t fio_math_mod_expo64(uint64_t base, uint64_t exp, uint64_t mod)
```

Perform modular exponentiation

_Symbol type:_ `function`

#### `fio_math_is_uprime`

```c
FIO_SFUNC bool fio_math_is_uprime(uint64_t n)
```

Tests if an unsigned 64 bit number is (probably) a prime.

For numbers up to 1023 this is deterministic.

_Symbol type:_ `function`

#### `fio_math_is_iprime`

```c
inline bool fio_math_is_iprime(int64_t n)
```

Tests if the absolute value of a signed 64 bit number is (probably) a prime.

For numbers up to 1023 this is deterministic.

_Symbol type:_ `function`

#### `fio_math_addc64`

```c
FIO_MIFN uint64_t fio_math_addc64(uint64_t a, uint64_t b, uint64_t carry_in, uint64_t *carry_out)
```

Add with carry.

_Symbol type:_ `function`

#### `fio_math_add`

```c
FIO_MIFN bool fio_math_add(uint64_t *dest, const uint64_t *a, const uint64_t *b, const size_t len)
```

Multi-precision ADD for `len` 64 bit words a + b. Returns the carry.

_Symbol type:_ `function`

#### `fio_math_addc128`

```c
FIO_MIFN __uint128_t fio_math_addc128(const __uint128_t a, const __uint128_t b, bool carry_in, bool *carry_out)
```

Multi-precision ADD for `bits` long a + b. Returns the carry.

_Symbol type:_ `function`

#### `fio_math_add2`

```c
FIO_MIFN bool fio_math_add2(__uint128_t *dest, const __uint128_t *a, const __uint128_t *b, const size_t len)
```



_Symbol type:_ `function`

#### `fio_math_subc64`

```c
FIO_MIFN uint64_t fio_math_subc64(uint64_t a, uint64_t b, uint64_t borrow_in, uint64_t *borrow_out)
```

Subtract with borrow.

_Symbol type:_ `function`

#### `fio_math_sub`

```c
FIO_MIFN uint64_t fio_math_sub(uint64_t *dest, const uint64_t *a, const uint64_t *b, const size_t len)
```

Multi-precision SUB for `len` 64 bit words a + b. Returns the borrow.

_Symbol type:_ `function`

#### `fio_math_mulc64`

```c
FIO_MIFN uint64_t fio_math_mulc64(uint64_t a, uint64_t b, uint64_t *carry_out)
```

Multiply with carry out.

_Symbol type:_ `function`

#### `fio_math_mul`

```c
inline void fio_math_mul(uint64_t *restrict dest, const uint64_t *a, const uint64_t *b, const size_t len)
```

Multi-precision MUL for `len` 64 bit words.

`dest` must be `len * 2` long to hold the result.

`a` and `b` must be of equal `len`.

_Symbol type:_ `function`

#### `fio_u128_init8`

```c
#define fio_u128_init8(...)  ((fio_u128){.u8 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u128_init16`

```c
#define fio_u128_init16(...) ((fio_u128){.u16 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u128_init32`

```c
#define fio_u128_init32(...) ((fio_u128){.u32 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u128_init64`

```c
#define fio_u128_init64(...) ((fio_u128){.u64 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u256_init8`

```c
#define fio_u256_init8(...)  ((fio_u256){.u8 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u256_init16`

```c
#define fio_u256_init16(...) ((fio_u256){.u16 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u256_init32`

```c
#define fio_u256_init32(...) ((fio_u256){.u32 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u256_init64`

```c
#define fio_u256_init64(...) ((fio_u256){.u64 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u512_init8`

```c
#define fio_u512_init8(...)  ((fio_u512){.u8 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u512_init16`

```c
#define fio_u512_init16(...) ((fio_u512){.u16 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u512_init32`

```c
#define fio_u512_init32(...) ((fio_u512){.u32 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u512_init64`

```c
#define fio_u512_init64(...) ((fio_u512){.u64 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u1024_init8`

```c
#define fio_u1024_init8(...)  ((fio_u1024){.u8 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u1024_init16`

```c
#define fio_u1024_init16(...) ((fio_u1024){.u16 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u1024_init32`

```c
#define fio_u1024_init32(...) ((fio_u1024){.u32 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u1024_init64`

```c
#define fio_u1024_init64(...) ((fio_u1024){.u64 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u2048_init8`

```c
#define fio_u2048_init8(...)  ((fio_u2048){.u8 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u2048_init16`

```c
#define fio_u2048_init16(...) ((fio_u2048){.u16 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u2048_init32`

```c
#define fio_u2048_init32(...) ((fio_u2048){.u32 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u2048_init64`

```c
#define fio_u2048_init64(...) ((fio_u2048){.u64 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u4096_init8`

```c
#define fio_u4096_init8(...)  ((fio_u4096){.u8 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u4096_init16`

```c
#define fio_u4096_init16(...) ((fio_u4096){.u16 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u4096_init32`

```c
#define fio_u4096_init32(...) ((fio_u4096){.u32 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u4096_init64`

```c
#define fio_u4096_init64(...) ((fio_u4096){.u64 = {__VA_ARGS__}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_u128_bswap16`

```c
fio_u128 fio_u128_bswap16(fio_u128 a)
```



_Symbol type:_ `function`

#### `fio_u128_bswap32`

```c
fio_u128 fio_u128_bswap32(fio_u128 a)
```



_Symbol type:_ `function`

#### `fio_u128_bswap64`

```c
fio_u128 fio_u128_bswap64(fio_u128 a)
```



_Symbol type:_ `function`

#### `fio_u128_load`

```c
fio_u128 fio_u128_load(const void *buf)
```

Loads from memory using local-endian.

_Symbol type:_ `function`

#### `fio_u128_load_be16`

```c
fio_u128 fio_u128_load_be16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u128_load_be32`

```c
fio_u128 fio_u128_load_be32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u128_load_be64`

```c
fio_u128 fio_u128_load_be64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u128_load_le16`

```c
fio_u128 fio_u128_load_le16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u128_load_le32`

```c
fio_u128 fio_u128_load_le32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u128_load_le64`

```c
fio_u128 fio_u128_load_le64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u128_store`

```c
void fio_u128_store(void *buf, const fio_u128 a)
```

Stores to memory using local-endian.

_Symbol type:_ `function`

#### `fio_u256_bswap16`

```c
fio_u256 fio_u256_bswap16(fio_u256 a)
```



_Symbol type:_ `function`

#### `fio_u256_bswap32`

```c
fio_u256 fio_u256_bswap32(fio_u256 a)
```



_Symbol type:_ `function`

#### `fio_u256_bswap64`

```c
fio_u256 fio_u256_bswap64(fio_u256 a)
```



_Symbol type:_ `function`

#### `fio_u256_load`

```c
fio_u256 fio_u256_load(const void *buf)
```

Loads from memory using local-endian.

_Symbol type:_ `function`

#### `fio_u256_load_be16`

```c
fio_u256 fio_u256_load_be16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u256_load_be32`

```c
fio_u256 fio_u256_load_be32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u256_load_be64`

```c
fio_u256 fio_u256_load_be64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u256_load_le16`

```c
fio_u256 fio_u256_load_le16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u256_load_le32`

```c
fio_u256 fio_u256_load_le32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u256_load_le64`

```c
fio_u256 fio_u256_load_le64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u256_store`

```c
void fio_u256_store(void *buf, const fio_u256 a)
```

Stores to memory using local-endian.

_Symbol type:_ `function`

#### `fio_u512_bswap16`

```c
fio_u512 fio_u512_bswap16(fio_u512 a)
```



_Symbol type:_ `function`

#### `fio_u512_bswap32`

```c
fio_u512 fio_u512_bswap32(fio_u512 a)
```



_Symbol type:_ `function`

#### `fio_u512_bswap64`

```c
fio_u512 fio_u512_bswap64(fio_u512 a)
```



_Symbol type:_ `function`

#### `fio_u512_load`

```c
fio_u512 fio_u512_load(const void *buf)
```

Loads from memory using local-endian.

_Symbol type:_ `function`

#### `fio_u512_load_be16`

```c
fio_u512 fio_u512_load_be16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u512_load_be32`

```c
fio_u512 fio_u512_load_be32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u512_load_be64`

```c
fio_u512 fio_u512_load_be64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u512_load_le16`

```c
fio_u512 fio_u512_load_le16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u512_load_le32`

```c
fio_u512 fio_u512_load_le32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u512_load_le64`

```c
fio_u512 fio_u512_load_le64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u512_store`

```c
void fio_u512_store(void *buf, const fio_u512 a)
```

Stores to memory using local-endian.

_Symbol type:_ `function`

#### `fio_u1024_bswap16`

```c
fio_u1024 fio_u1024_bswap16(fio_u1024 a)
```



_Symbol type:_ `function`

#### `fio_u1024_bswap32`

```c
fio_u1024 fio_u1024_bswap32(fio_u1024 a)
```



_Symbol type:_ `function`

#### `fio_u1024_bswap64`

```c
fio_u1024 fio_u1024_bswap64(fio_u1024 a)
```



_Symbol type:_ `function`

#### `fio_u1024_load`

```c
fio_u1024 fio_u1024_load(const void *buf)
```

Loads from memory using local-endian.

_Symbol type:_ `function`

#### `fio_u1024_load_be16`

```c
fio_u1024 fio_u1024_load_be16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u1024_load_be32`

```c
fio_u1024 fio_u1024_load_be32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u1024_load_be64`

```c
fio_u1024 fio_u1024_load_be64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u1024_load_le16`

```c
fio_u1024 fio_u1024_load_le16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u1024_load_le32`

```c
fio_u1024 fio_u1024_load_le32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u1024_load_le64`

```c
fio_u1024 fio_u1024_load_le64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u1024_store`

```c
void fio_u1024_store(void *buf, const fio_u1024 a)
```

Stores to memory using local-endian.

_Symbol type:_ `function`

#### `fio_u2048_bswap16`

```c
fio_u2048 fio_u2048_bswap16(fio_u2048 a)
```



_Symbol type:_ `function`

#### `fio_u2048_bswap32`

```c
fio_u2048 fio_u2048_bswap32(fio_u2048 a)
```



_Symbol type:_ `function`

#### `fio_u2048_bswap64`

```c
fio_u2048 fio_u2048_bswap64(fio_u2048 a)
```



_Symbol type:_ `function`

#### `fio_u2048_load`

```c
fio_u2048 fio_u2048_load(const void *buf)
```

Loads from memory using local-endian.

_Symbol type:_ `function`

#### `fio_u2048_load_be16`

```c
fio_u2048 fio_u2048_load_be16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u2048_load_be32`

```c
fio_u2048 fio_u2048_load_be32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u2048_load_be64`

```c
fio_u2048 fio_u2048_load_be64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u2048_load_le16`

```c
fio_u2048 fio_u2048_load_le16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u2048_load_le32`

```c
fio_u2048 fio_u2048_load_le32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u2048_load_le64`

```c
fio_u2048 fio_u2048_load_le64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u2048_store`

```c
void fio_u2048_store(void *buf, const fio_u2048 a)
```

Stores to memory using local-endian.

_Symbol type:_ `function`

#### `fio_u4096_bswap16`

```c
fio_u4096 fio_u4096_bswap16(fio_u4096 a)
```



_Symbol type:_ `function`

#### `fio_u4096_bswap32`

```c
fio_u4096 fio_u4096_bswap32(fio_u4096 a)
```



_Symbol type:_ `function`

#### `fio_u4096_bswap64`

```c
fio_u4096 fio_u4096_bswap64(fio_u4096 a)
```



_Symbol type:_ `function`

#### `fio_u4096_load`

```c
fio_u4096 fio_u4096_load(const void *buf)
```

Loads from memory using local-endian.

_Symbol type:_ `function`

#### `fio_u4096_load_be16`

```c
fio_u4096 fio_u4096_load_be16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u4096_load_be32`

```c
fio_u4096 fio_u4096_load_be32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u4096_load_be64`

```c
fio_u4096 fio_u4096_load_be64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u4096_load_le16`

```c
fio_u4096 fio_u4096_load_le16(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u4096_load_le32`

```c
fio_u4096 fio_u4096_load_le32(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u4096_load_le64`

```c
fio_u4096 fio_u4096_load_le64(const void *buf)
```



_Symbol type:_ `function`

#### `fio_u4096_store`

```c
void fio_u4096_store(void *buf, const fio_u4096 a)
```

Stores to memory using local-endian.

_Symbol type:_ `function`

#### `fio_u128_3xor`

```c
void fio_u128_3xor(fio_u128 *target, const fio_u128 *a, const fio_u128 *b, const fio_u128 *c)
```



_Symbol type:_ `function`

#### `fio_u128_3xor32`

```c
void fio_u128_3xor32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b, const fio_u128 *c)
```



_Symbol type:_ `function`

#### `fio_u128_3xor64`

```c
void fio_u128_3xor64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b, const fio_u128 *c)
```



_Symbol type:_ `function`

#### `fio_u128_add16`

```c
void fio_u128_add16(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_add32`

```c
void fio_u128_add32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_add64`

```c
void fio_u128_add64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_add8`

```c
void fio_u128_add8(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_and`

```c
void fio_u128_and(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_and16`

```c
void fio_u128_and16(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_and32`

```c
void fio_u128_and32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_and64`

```c
void fio_u128_and64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_and8`

```c
void fio_u128_and8(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_cadd16`

```c
void fio_u128_cadd16(fio_u128 *target, const fio_u128 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cadd32`

```c
void fio_u128_cadd32(fio_u128 *target, const fio_u128 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cadd64`

```c
void fio_u128_cadd64(fio_u128 *target, const fio_u128 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cadd8`

```c
void fio_u128_cadd8(fio_u128 *target, const fio_u128 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cand16`

```c
void fio_u128_cand16(fio_u128 *target, const fio_u128 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cand32`

```c
void fio_u128_cand32(fio_u128 *target, const fio_u128 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cand64`

```c
void fio_u128_cand64(fio_u128 *target, const fio_u128 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cand8`

```c
void fio_u128_cand8(fio_u128 *target, const fio_u128 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u128_clrot16`

```c
void fio_u128_clrot16(fio_u128 *target, const fio_u128 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u128_clrot32`

```c
void fio_u128_clrot32(fio_u128 *target, const fio_u128 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u128_clrot64`

```c
void fio_u128_clrot64(fio_u128 *target, const fio_u128 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u128_clrot8`

```c
void fio_u128_clrot8(fio_u128 *target, const fio_u128 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u128_cmul16`

```c
void fio_u128_cmul16(fio_u128 *target, const fio_u128 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cmul32`

```c
void fio_u128_cmul32(fio_u128 *target, const fio_u128 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cmul64`

```c
void fio_u128_cmul64(fio_u128 *target, const fio_u128 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cmul8`

```c
void fio_u128_cmul8(fio_u128 *target, const fio_u128 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cor16`

```c
void fio_u128_cor16(fio_u128 *target, const fio_u128 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cor32`

```c
void fio_u128_cor32(fio_u128 *target, const fio_u128 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cor64`

```c
void fio_u128_cor64(fio_u128 *target, const fio_u128 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cor8`

```c
void fio_u128_cor8(fio_u128 *target, const fio_u128 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u128_crrot16`

```c
void fio_u128_crrot16(fio_u128 *target, const fio_u128 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u128_crrot32`

```c
void fio_u128_crrot32(fio_u128 *target, const fio_u128 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u128_crrot64`

```c
void fio_u128_crrot64(fio_u128 *target, const fio_u128 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u128_crrot8`

```c
void fio_u128_crrot8(fio_u128 *target, const fio_u128 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u128_csub16`

```c
void fio_u128_csub16(fio_u128 *target, const fio_u128 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u128_csub32`

```c
void fio_u128_csub32(fio_u128 *target, const fio_u128 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u128_csub64`

```c
void fio_u128_csub64(fio_u128 *target, const fio_u128 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u128_csub8`

```c
void fio_u128_csub8(fio_u128 *target, const fio_u128 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u128_ct_swap_if`

```c
void fio_u128_ct_swap_if(bool cond, fio_u128 *restrict a, fio_u128 *restrict b)
```



_Symbol type:_ `function`

#### `fio_u128_cxor16`

```c
void fio_u128_cxor16(fio_u128 *target, const fio_u128 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cxor32`

```c
void fio_u128_cxor32(fio_u128 *target, const fio_u128 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cxor64`

```c
void fio_u128_cxor64(fio_u128 *target, const fio_u128 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u128_cxor8`

```c
void fio_u128_cxor8(fio_u128 *target, const fio_u128 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u128_inv`

```c
void fio_u128_inv(fio_u128 *target, const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_is_eq`

```c
bool fio_u128_is_eq(const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_lrot16`

```c
void fio_u128_lrot16(fio_u128 *target, const fio_u128 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u128_lrot32`

```c
void fio_u128_lrot32(fio_u128 *target, const fio_u128 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u128_lrot64`

```c
void fio_u128_lrot64(fio_u128 *target, const fio_u128 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u128_lrot8`

```c
void fio_u128_lrot8(fio_u128 *target, const fio_u128 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u128_maj`

```c
void fio_u128_maj(fio_u128 *target, const fio_u128 *a, const fio_u128 *b, const fio_u128 *c)
```



_Symbol type:_ `function`

#### `fio_u128_maj32`

```c
void fio_u128_maj32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b, const fio_u128 *c)
```



_Symbol type:_ `function`

#### `fio_u128_maj64`

```c
void fio_u128_maj64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b, const fio_u128 *c)
```



_Symbol type:_ `function`

#### `fio_u128_mul16`

```c
void fio_u128_mul16(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_mul32`

```c
void fio_u128_mul32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_mul64`

```c
void fio_u128_mul64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_mul8`

```c
void fio_u128_mul8(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_mux`

```c
void fio_u128_mux(fio_u128 *target, const fio_u128 *a, const fio_u128 *b, const fio_u128 *c)
```



_Symbol type:_ `function`

#### `fio_u128_mux32`

```c
void fio_u128_mux32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b, const fio_u128 *c)
```



_Symbol type:_ `function`

#### `fio_u128_mux64`

```c
void fio_u128_mux64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b, const fio_u128 *c)
```



_Symbol type:_ `function`

#### `fio_u128_or`

```c
void fio_u128_or(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_or16`

```c
void fio_u128_or16(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_or32`

```c
void fio_u128_or32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_or64`

```c
void fio_u128_or64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_or8`

```c
void fio_u128_or8(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_add16`

```c
uint16_t fio_u128_reduce_add16(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_add32`

```c
uint32_t fio_u128_reduce_add32(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_add64`

```c
uint64_t fio_u128_reduce_add64(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_add8`

```c
uint8_t fio_u128_reduce_add8(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_and16`

```c
uint16_t fio_u128_reduce_and16(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_and32`

```c
uint32_t fio_u128_reduce_and32(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_and64`

```c
uint64_t fio_u128_reduce_and64(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_and8`

```c
uint8_t fio_u128_reduce_and8(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_mul16`

```c
uint16_t fio_u128_reduce_mul16(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_mul32`

```c
uint32_t fio_u128_reduce_mul32(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_mul64`

```c
uint64_t fio_u128_reduce_mul64(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_mul8`

```c
uint8_t fio_u128_reduce_mul8(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_or16`

```c
uint16_t fio_u128_reduce_or16(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_or32`

```c
uint32_t fio_u128_reduce_or32(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_or64`

```c
uint64_t fio_u128_reduce_or64(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_or8`

```c
uint8_t fio_u128_reduce_or8(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_sub16`

```c
uint16_t fio_u128_reduce_sub16(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_sub32`

```c
uint32_t fio_u128_reduce_sub32(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_sub64`

```c
uint64_t fio_u128_reduce_sub64(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_sub8`

```c
uint8_t fio_u128_reduce_sub8(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_xor16`

```c
uint16_t fio_u128_reduce_xor16(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_xor32`

```c
uint32_t fio_u128_reduce_xor32(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_xor64`

```c
uint64_t fio_u128_reduce_xor64(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_reduce_xor8`

```c
uint8_t fio_u128_reduce_xor8(const fio_u128 *a)
```



_Symbol type:_ `function`

#### `fio_u128_rrot16`

```c
void fio_u128_rrot16(fio_u128 *target, const fio_u128 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u128_rrot32`

```c
void fio_u128_rrot32(fio_u128 *target, const fio_u128 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u128_rrot64`

```c
void fio_u128_rrot64(fio_u128 *target, const fio_u128 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u128_rrot8`

```c
void fio_u128_rrot8(fio_u128 *target, const fio_u128 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u128_sub16`

```c
void fio_u128_sub16(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_sub32`

```c
void fio_u128_sub32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_sub64`

```c
void fio_u128_sub64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_sub8`

```c
void fio_u128_sub8(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_xor`

```c
void fio_u128_xor(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_xor16`

```c
void fio_u128_xor16(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_xor32`

```c
void fio_u128_xor32(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_xor64`

```c
void fio_u128_xor64(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u128_xor8`

```c
void fio_u128_xor8(fio_u128 *target, const fio_u128 *a, const fio_u128 *b)
```



_Symbol type:_ `function`

#### `fio_u256_3xor`

```c
void fio_u256_3xor(fio_u256 *target, const fio_u256 *a, const fio_u256 *b, const fio_u256 *c)
```



_Symbol type:_ `function`

#### `fio_u256_3xor32`

```c
void fio_u256_3xor32(fio_u256 *target, const fio_u256 *a, const fio_u256 *b, const fio_u256 *c)
```



_Symbol type:_ `function`

#### `fio_u256_3xor64`

```c
void fio_u256_3xor64(fio_u256 *target, const fio_u256 *a, const fio_u256 *b, const fio_u256 *c)
```



_Symbol type:_ `function`

#### `fio_u256_add16`

```c
void fio_u256_add16(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_add32`

```c
void fio_u256_add32(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_add64`

```c
void fio_u256_add64(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_add8`

```c
void fio_u256_add8(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_and`

```c
void fio_u256_and(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_and16`

```c
void fio_u256_and16(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_and32`

```c
void fio_u256_and32(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_and64`

```c
void fio_u256_and64(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_and8`

```c
void fio_u256_and8(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_cadd16`

```c
void fio_u256_cadd16(fio_u256 *target, const fio_u256 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cadd32`

```c
void fio_u256_cadd32(fio_u256 *target, const fio_u256 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cadd64`

```c
void fio_u256_cadd64(fio_u256 *target, const fio_u256 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cadd8`

```c
void fio_u256_cadd8(fio_u256 *target, const fio_u256 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cand16`

```c
void fio_u256_cand16(fio_u256 *target, const fio_u256 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cand32`

```c
void fio_u256_cand32(fio_u256 *target, const fio_u256 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cand64`

```c
void fio_u256_cand64(fio_u256 *target, const fio_u256 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cand8`

```c
void fio_u256_cand8(fio_u256 *target, const fio_u256 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u256_clrot16`

```c
void fio_u256_clrot16(fio_u256 *target, const fio_u256 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u256_clrot32`

```c
void fio_u256_clrot32(fio_u256 *target, const fio_u256 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u256_clrot64`

```c
void fio_u256_clrot64(fio_u256 *target, const fio_u256 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u256_clrot8`

```c
void fio_u256_clrot8(fio_u256 *target, const fio_u256 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u256_cmul16`

```c
void fio_u256_cmul16(fio_u256 *target, const fio_u256 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cmul32`

```c
void fio_u256_cmul32(fio_u256 *target, const fio_u256 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cmul64`

```c
void fio_u256_cmul64(fio_u256 *target, const fio_u256 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cmul8`

```c
void fio_u256_cmul8(fio_u256 *target, const fio_u256 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cor16`

```c
void fio_u256_cor16(fio_u256 *target, const fio_u256 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cor32`

```c
void fio_u256_cor32(fio_u256 *target, const fio_u256 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cor64`

```c
void fio_u256_cor64(fio_u256 *target, const fio_u256 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cor8`

```c
void fio_u256_cor8(fio_u256 *target, const fio_u256 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u256_crrot16`

```c
void fio_u256_crrot16(fio_u256 *target, const fio_u256 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u256_crrot32`

```c
void fio_u256_crrot32(fio_u256 *target, const fio_u256 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u256_crrot64`

```c
void fio_u256_crrot64(fio_u256 *target, const fio_u256 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u256_crrot8`

```c
void fio_u256_crrot8(fio_u256 *target, const fio_u256 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u256_csub16`

```c
void fio_u256_csub16(fio_u256 *target, const fio_u256 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u256_csub32`

```c
void fio_u256_csub32(fio_u256 *target, const fio_u256 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u256_csub64`

```c
void fio_u256_csub64(fio_u256 *target, const fio_u256 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u256_csub8`

```c
void fio_u256_csub8(fio_u256 *target, const fio_u256 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u256_ct_swap_if`

```c
void fio_u256_ct_swap_if(bool cond, fio_u256 *restrict a, fio_u256 *restrict b)
```



_Symbol type:_ `function`

#### `fio_u256_cxor16`

```c
void fio_u256_cxor16(fio_u256 *target, const fio_u256 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cxor32`

```c
void fio_u256_cxor32(fio_u256 *target, const fio_u256 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cxor64`

```c
void fio_u256_cxor64(fio_u256 *target, const fio_u256 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u256_cxor8`

```c
void fio_u256_cxor8(fio_u256 *target, const fio_u256 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u256_inv`

```c
void fio_u256_inv(fio_u256 *target, const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_is_eq`

```c
bool fio_u256_is_eq(const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_lrot16`

```c
void fio_u256_lrot16(fio_u256 *target, const fio_u256 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u256_lrot32`

```c
void fio_u256_lrot32(fio_u256 *target, const fio_u256 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u256_lrot64`

```c
void fio_u256_lrot64(fio_u256 *target, const fio_u256 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u256_lrot8`

```c
void fio_u256_lrot8(fio_u256 *target, const fio_u256 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u256_maj`

```c
void fio_u256_maj(fio_u256 *target, const fio_u256 *a, const fio_u256 *b, const fio_u256 *c)
```



_Symbol type:_ `function`

#### `fio_u256_maj32`

```c
void fio_u256_maj32(fio_u256 *target, const fio_u256 *a, const fio_u256 *b, const fio_u256 *c)
```



_Symbol type:_ `function`

#### `fio_u256_maj64`

```c
void fio_u256_maj64(fio_u256 *target, const fio_u256 *a, const fio_u256 *b, const fio_u256 *c)
```



_Symbol type:_ `function`

#### `fio_u256_mul16`

```c
void fio_u256_mul16(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_mul32`

```c
void fio_u256_mul32(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_mul64`

```c
void fio_u256_mul64(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_mul8`

```c
void fio_u256_mul8(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_mux`

```c
void fio_u256_mux(fio_u256 *target, const fio_u256 *a, const fio_u256 *b, const fio_u256 *c)
```



_Symbol type:_ `function`

#### `fio_u256_mux32`

```c
void fio_u256_mux32(fio_u256 *target, const fio_u256 *a, const fio_u256 *b, const fio_u256 *c)
```



_Symbol type:_ `function`

#### `fio_u256_mux64`

```c
void fio_u256_mux64(fio_u256 *target, const fio_u256 *a, const fio_u256 *b, const fio_u256 *c)
```



_Symbol type:_ `function`

#### `fio_u256_or`

```c
void fio_u256_or(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_or16`

```c
void fio_u256_or16(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_or32`

```c
void fio_u256_or32(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_or64`

```c
void fio_u256_or64(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_or8`

```c
void fio_u256_or8(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_add16`

```c
uint16_t fio_u256_reduce_add16(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_add32`

```c
uint32_t fio_u256_reduce_add32(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_add64`

```c
uint64_t fio_u256_reduce_add64(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_add8`

```c
uint8_t fio_u256_reduce_add8(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_and16`

```c
uint16_t fio_u256_reduce_and16(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_and32`

```c
uint32_t fio_u256_reduce_and32(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_and64`

```c
uint64_t fio_u256_reduce_and64(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_and8`

```c
uint8_t fio_u256_reduce_and8(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_mul16`

```c
uint16_t fio_u256_reduce_mul16(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_mul32`

```c
uint32_t fio_u256_reduce_mul32(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_mul64`

```c
uint64_t fio_u256_reduce_mul64(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_mul8`

```c
uint8_t fio_u256_reduce_mul8(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_or16`

```c
uint16_t fio_u256_reduce_or16(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_or32`

```c
uint32_t fio_u256_reduce_or32(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_or64`

```c
uint64_t fio_u256_reduce_or64(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_or8`

```c
uint8_t fio_u256_reduce_or8(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_sub16`

```c
uint16_t fio_u256_reduce_sub16(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_sub32`

```c
uint32_t fio_u256_reduce_sub32(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_sub64`

```c
uint64_t fio_u256_reduce_sub64(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_sub8`

```c
uint8_t fio_u256_reduce_sub8(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_xor16`

```c
uint16_t fio_u256_reduce_xor16(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_xor32`

```c
uint32_t fio_u256_reduce_xor32(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_xor64`

```c
uint64_t fio_u256_reduce_xor64(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_reduce_xor8`

```c
uint8_t fio_u256_reduce_xor8(const fio_u256 *a)
```



_Symbol type:_ `function`

#### `fio_u256_rrot16`

```c
void fio_u256_rrot16(fio_u256 *target, const fio_u256 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u256_rrot32`

```c
void fio_u256_rrot32(fio_u256 *target, const fio_u256 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u256_rrot64`

```c
void fio_u256_rrot64(fio_u256 *target, const fio_u256 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u256_rrot8`

```c
void fio_u256_rrot8(fio_u256 *target, const fio_u256 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u256_sub16`

```c
void fio_u256_sub16(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_sub32`

```c
void fio_u256_sub32(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_sub64`

```c
void fio_u256_sub64(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_sub8`

```c
void fio_u256_sub8(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_xor`

```c
void fio_u256_xor(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_xor16`

```c
void fio_u256_xor16(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_xor32`

```c
void fio_u256_xor32(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_xor64`

```c
void fio_u256_xor64(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u256_xor8`

```c
void fio_u256_xor8(fio_u256 *target, const fio_u256 *a, const fio_u256 *b)
```



_Symbol type:_ `function`

#### `fio_u512_3xor`

```c
void fio_u512_3xor(fio_u512 *target, const fio_u512 *a, const fio_u512 *b, const fio_u512 *c)
```



_Symbol type:_ `function`

#### `fio_u512_3xor32`

```c
void fio_u512_3xor32(fio_u512 *target, const fio_u512 *a, const fio_u512 *b, const fio_u512 *c)
```



_Symbol type:_ `function`

#### `fio_u512_3xor64`

```c
void fio_u512_3xor64(fio_u512 *target, const fio_u512 *a, const fio_u512 *b, const fio_u512 *c)
```



_Symbol type:_ `function`

#### `fio_u512_add16`

```c
void fio_u512_add16(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_add32`

```c
void fio_u512_add32(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_add64`

```c
void fio_u512_add64(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_add8`

```c
void fio_u512_add8(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_and`

```c
void fio_u512_and(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_and16`

```c
void fio_u512_and16(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_and32`

```c
void fio_u512_and32(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_and64`

```c
void fio_u512_and64(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_and8`

```c
void fio_u512_and8(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_cadd16`

```c
void fio_u512_cadd16(fio_u512 *target, const fio_u512 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cadd32`

```c
void fio_u512_cadd32(fio_u512 *target, const fio_u512 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cadd64`

```c
void fio_u512_cadd64(fio_u512 *target, const fio_u512 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cadd8`

```c
void fio_u512_cadd8(fio_u512 *target, const fio_u512 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cand16`

```c
void fio_u512_cand16(fio_u512 *target, const fio_u512 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cand32`

```c
void fio_u512_cand32(fio_u512 *target, const fio_u512 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cand64`

```c
void fio_u512_cand64(fio_u512 *target, const fio_u512 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cand8`

```c
void fio_u512_cand8(fio_u512 *target, const fio_u512 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u512_clrot16`

```c
void fio_u512_clrot16(fio_u512 *target, const fio_u512 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u512_clrot32`

```c
void fio_u512_clrot32(fio_u512 *target, const fio_u512 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u512_clrot64`

```c
void fio_u512_clrot64(fio_u512 *target, const fio_u512 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u512_clrot8`

```c
void fio_u512_clrot8(fio_u512 *target, const fio_u512 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u512_cmul16`

```c
void fio_u512_cmul16(fio_u512 *target, const fio_u512 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cmul32`

```c
void fio_u512_cmul32(fio_u512 *target, const fio_u512 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cmul64`

```c
void fio_u512_cmul64(fio_u512 *target, const fio_u512 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cmul8`

```c
void fio_u512_cmul8(fio_u512 *target, const fio_u512 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cor16`

```c
void fio_u512_cor16(fio_u512 *target, const fio_u512 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cor32`

```c
void fio_u512_cor32(fio_u512 *target, const fio_u512 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cor64`

```c
void fio_u512_cor64(fio_u512 *target, const fio_u512 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cor8`

```c
void fio_u512_cor8(fio_u512 *target, const fio_u512 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u512_crrot16`

```c
void fio_u512_crrot16(fio_u512 *target, const fio_u512 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u512_crrot32`

```c
void fio_u512_crrot32(fio_u512 *target, const fio_u512 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u512_crrot64`

```c
void fio_u512_crrot64(fio_u512 *target, const fio_u512 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u512_crrot8`

```c
void fio_u512_crrot8(fio_u512 *target, const fio_u512 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u512_csub16`

```c
void fio_u512_csub16(fio_u512 *target, const fio_u512 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u512_csub32`

```c
void fio_u512_csub32(fio_u512 *target, const fio_u512 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u512_csub64`

```c
void fio_u512_csub64(fio_u512 *target, const fio_u512 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u512_csub8`

```c
void fio_u512_csub8(fio_u512 *target, const fio_u512 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u512_ct_swap_if`

```c
void fio_u512_ct_swap_if(bool cond, fio_u512 *restrict a, fio_u512 *restrict b)
```



_Symbol type:_ `function`

#### `fio_u512_cxor16`

```c
void fio_u512_cxor16(fio_u512 *target, const fio_u512 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cxor32`

```c
void fio_u512_cxor32(fio_u512 *target, const fio_u512 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cxor64`

```c
void fio_u512_cxor64(fio_u512 *target, const fio_u512 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u512_cxor8`

```c
void fio_u512_cxor8(fio_u512 *target, const fio_u512 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u512_inv`

```c
void fio_u512_inv(fio_u512 *target, const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_is_eq`

```c
bool fio_u512_is_eq(const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_lrot16`

```c
void fio_u512_lrot16(fio_u512 *target, const fio_u512 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u512_lrot32`

```c
void fio_u512_lrot32(fio_u512 *target, const fio_u512 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u512_lrot64`

```c
void fio_u512_lrot64(fio_u512 *target, const fio_u512 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u512_lrot8`

```c
void fio_u512_lrot8(fio_u512 *target, const fio_u512 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u512_maj`

```c
void fio_u512_maj(fio_u512 *target, const fio_u512 *a, const fio_u512 *b, const fio_u512 *c)
```



_Symbol type:_ `function`

#### `fio_u512_maj32`

```c
void fio_u512_maj32(fio_u512 *target, const fio_u512 *a, const fio_u512 *b, const fio_u512 *c)
```



_Symbol type:_ `function`

#### `fio_u512_maj64`

```c
void fio_u512_maj64(fio_u512 *target, const fio_u512 *a, const fio_u512 *b, const fio_u512 *c)
```



_Symbol type:_ `function`

#### `fio_u512_mul16`

```c
void fio_u512_mul16(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_mul32`

```c
void fio_u512_mul32(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_mul64`

```c
void fio_u512_mul64(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_mul8`

```c
void fio_u512_mul8(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_mux`

```c
void fio_u512_mux(fio_u512 *target, const fio_u512 *a, const fio_u512 *b, const fio_u512 *c)
```



_Symbol type:_ `function`

#### `fio_u512_mux32`

```c
void fio_u512_mux32(fio_u512 *target, const fio_u512 *a, const fio_u512 *b, const fio_u512 *c)
```



_Symbol type:_ `function`

#### `fio_u512_mux64`

```c
void fio_u512_mux64(fio_u512 *target, const fio_u512 *a, const fio_u512 *b, const fio_u512 *c)
```



_Symbol type:_ `function`

#### `fio_u512_or`

```c
void fio_u512_or(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_or16`

```c
void fio_u512_or16(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_or32`

```c
void fio_u512_or32(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_or64`

```c
void fio_u512_or64(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_or8`

```c
void fio_u512_or8(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_add16`

```c
uint16_t fio_u512_reduce_add16(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_add32`

```c
uint32_t fio_u512_reduce_add32(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_add64`

```c
uint64_t fio_u512_reduce_add64(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_add8`

```c
uint8_t fio_u512_reduce_add8(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_and16`

```c
uint16_t fio_u512_reduce_and16(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_and32`

```c
uint32_t fio_u512_reduce_and32(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_and64`

```c
uint64_t fio_u512_reduce_and64(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_and8`

```c
uint8_t fio_u512_reduce_and8(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_mul16`

```c
uint16_t fio_u512_reduce_mul16(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_mul32`

```c
uint32_t fio_u512_reduce_mul32(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_mul64`

```c
uint64_t fio_u512_reduce_mul64(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_mul8`

```c
uint8_t fio_u512_reduce_mul8(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_or16`

```c
uint16_t fio_u512_reduce_or16(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_or32`

```c
uint32_t fio_u512_reduce_or32(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_or64`

```c
uint64_t fio_u512_reduce_or64(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_or8`

```c
uint8_t fio_u512_reduce_or8(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_sub16`

```c
uint16_t fio_u512_reduce_sub16(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_sub32`

```c
uint32_t fio_u512_reduce_sub32(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_sub64`

```c
uint64_t fio_u512_reduce_sub64(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_sub8`

```c
uint8_t fio_u512_reduce_sub8(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_xor16`

```c
uint16_t fio_u512_reduce_xor16(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_xor32`

```c
uint32_t fio_u512_reduce_xor32(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_xor64`

```c
uint64_t fio_u512_reduce_xor64(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_reduce_xor8`

```c
uint8_t fio_u512_reduce_xor8(const fio_u512 *a)
```



_Symbol type:_ `function`

#### `fio_u512_rrot16`

```c
void fio_u512_rrot16(fio_u512 *target, const fio_u512 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u512_rrot32`

```c
void fio_u512_rrot32(fio_u512 *target, const fio_u512 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u512_rrot64`

```c
void fio_u512_rrot64(fio_u512 *target, const fio_u512 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u512_rrot8`

```c
void fio_u512_rrot8(fio_u512 *target, const fio_u512 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u512_sub16`

```c
void fio_u512_sub16(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_sub32`

```c
void fio_u512_sub32(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_sub64`

```c
void fio_u512_sub64(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_sub8`

```c
void fio_u512_sub8(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_xor`

```c
void fio_u512_xor(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_xor16`

```c
void fio_u512_xor16(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_xor32`

```c
void fio_u512_xor32(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_xor64`

```c
void fio_u512_xor64(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u512_xor8`

```c
void fio_u512_xor8(fio_u512 *target, const fio_u512 *a, const fio_u512 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_3xor`

```c
void fio_u1024_3xor(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *c)
```



_Symbol type:_ `function`

#### `fio_u1024_3xor32`

```c
void fio_u1024_3xor32(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *c)
```



_Symbol type:_ `function`

#### `fio_u1024_3xor64`

```c
void fio_u1024_3xor64(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *c)
```



_Symbol type:_ `function`

#### `fio_u1024_add16`

```c
void fio_u1024_add16(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_add32`

```c
void fio_u1024_add32(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_add64`

```c
void fio_u1024_add64(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_add8`

```c
void fio_u1024_add8(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_and`

```c
void fio_u1024_and(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_and16`

```c
void fio_u1024_and16(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_and32`

```c
void fio_u1024_and32(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_and64`

```c
void fio_u1024_and64(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_and8`

```c
void fio_u1024_and8(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_cadd16`

```c
void fio_u1024_cadd16(fio_u1024 *target, const fio_u1024 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cadd32`

```c
void fio_u1024_cadd32(fio_u1024 *target, const fio_u1024 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cadd64`

```c
void fio_u1024_cadd64(fio_u1024 *target, const fio_u1024 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cadd8`

```c
void fio_u1024_cadd8(fio_u1024 *target, const fio_u1024 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cand16`

```c
void fio_u1024_cand16(fio_u1024 *target, const fio_u1024 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cand32`

```c
void fio_u1024_cand32(fio_u1024 *target, const fio_u1024 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cand64`

```c
void fio_u1024_cand64(fio_u1024 *target, const fio_u1024 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cand8`

```c
void fio_u1024_cand8(fio_u1024 *target, const fio_u1024 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_clrot16`

```c
void fio_u1024_clrot16(fio_u1024 *target, const fio_u1024 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u1024_clrot32`

```c
void fio_u1024_clrot32(fio_u1024 *target, const fio_u1024 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u1024_clrot64`

```c
void fio_u1024_clrot64(fio_u1024 *target, const fio_u1024 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u1024_clrot8`

```c
void fio_u1024_clrot8(fio_u1024 *target, const fio_u1024 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u1024_cmul16`

```c
void fio_u1024_cmul16(fio_u1024 *target, const fio_u1024 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cmul32`

```c
void fio_u1024_cmul32(fio_u1024 *target, const fio_u1024 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cmul64`

```c
void fio_u1024_cmul64(fio_u1024 *target, const fio_u1024 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cmul8`

```c
void fio_u1024_cmul8(fio_u1024 *target, const fio_u1024 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cor16`

```c
void fio_u1024_cor16(fio_u1024 *target, const fio_u1024 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cor32`

```c
void fio_u1024_cor32(fio_u1024 *target, const fio_u1024 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cor64`

```c
void fio_u1024_cor64(fio_u1024 *target, const fio_u1024 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cor8`

```c
void fio_u1024_cor8(fio_u1024 *target, const fio_u1024 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_crrot16`

```c
void fio_u1024_crrot16(fio_u1024 *target, const fio_u1024 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u1024_crrot32`

```c
void fio_u1024_crrot32(fio_u1024 *target, const fio_u1024 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u1024_crrot64`

```c
void fio_u1024_crrot64(fio_u1024 *target, const fio_u1024 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u1024_crrot8`

```c
void fio_u1024_crrot8(fio_u1024 *target, const fio_u1024 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u1024_csub16`

```c
void fio_u1024_csub16(fio_u1024 *target, const fio_u1024 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_csub32`

```c
void fio_u1024_csub32(fio_u1024 *target, const fio_u1024 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_csub64`

```c
void fio_u1024_csub64(fio_u1024 *target, const fio_u1024 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_csub8`

```c
void fio_u1024_csub8(fio_u1024 *target, const fio_u1024 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_ct_swap_if`

```c
void fio_u1024_ct_swap_if(bool cond, fio_u1024 *restrict a, fio_u1024 *restrict b)
```



_Symbol type:_ `function`

#### `fio_u1024_cxor16`

```c
void fio_u1024_cxor16(fio_u1024 *target, const fio_u1024 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cxor32`

```c
void fio_u1024_cxor32(fio_u1024 *target, const fio_u1024 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cxor64`

```c
void fio_u1024_cxor64(fio_u1024 *target, const fio_u1024 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_cxor8`

```c
void fio_u1024_cxor8(fio_u1024 *target, const fio_u1024 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u1024_inv`

```c
void fio_u1024_inv(fio_u1024 *target, const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_is_eq`

```c
bool fio_u1024_is_eq(const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_lrot16`

```c
void fio_u1024_lrot16(fio_u1024 *target, const fio_u1024 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u1024_lrot32`

```c
void fio_u1024_lrot32(fio_u1024 *target, const fio_u1024 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u1024_lrot64`

```c
void fio_u1024_lrot64(fio_u1024 *target, const fio_u1024 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u1024_lrot8`

```c
void fio_u1024_lrot8(fio_u1024 *target, const fio_u1024 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u1024_maj`

```c
void fio_u1024_maj(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *c)
```



_Symbol type:_ `function`

#### `fio_u1024_maj32`

```c
void fio_u1024_maj32(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *c)
```



_Symbol type:_ `function`

#### `fio_u1024_maj64`

```c
void fio_u1024_maj64(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *c)
```



_Symbol type:_ `function`

#### `fio_u1024_mul16`

```c
void fio_u1024_mul16(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_mul32`

```c
void fio_u1024_mul32(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_mul64`

```c
void fio_u1024_mul64(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_mul8`

```c
void fio_u1024_mul8(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_mux`

```c
void fio_u1024_mux(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *c)
```



_Symbol type:_ `function`

#### `fio_u1024_mux32`

```c
void fio_u1024_mux32(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *c)
```



_Symbol type:_ `function`

#### `fio_u1024_mux64`

```c
void fio_u1024_mux64(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *c)
```



_Symbol type:_ `function`

#### `fio_u1024_or`

```c
void fio_u1024_or(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_or16`

```c
void fio_u1024_or16(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_or32`

```c
void fio_u1024_or32(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_or64`

```c
void fio_u1024_or64(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_or8`

```c
void fio_u1024_or8(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_add16`

```c
uint16_t fio_u1024_reduce_add16(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_add32`

```c
uint32_t fio_u1024_reduce_add32(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_add64`

```c
uint64_t fio_u1024_reduce_add64(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_add8`

```c
uint8_t fio_u1024_reduce_add8(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_and16`

```c
uint16_t fio_u1024_reduce_and16(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_and32`

```c
uint32_t fio_u1024_reduce_and32(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_and64`

```c
uint64_t fio_u1024_reduce_and64(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_and8`

```c
uint8_t fio_u1024_reduce_and8(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_mul16`

```c
uint16_t fio_u1024_reduce_mul16(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_mul32`

```c
uint32_t fio_u1024_reduce_mul32(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_mul64`

```c
uint64_t fio_u1024_reduce_mul64(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_mul8`

```c
uint8_t fio_u1024_reduce_mul8(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_or16`

```c
uint16_t fio_u1024_reduce_or16(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_or32`

```c
uint32_t fio_u1024_reduce_or32(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_or64`

```c
uint64_t fio_u1024_reduce_or64(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_or8`

```c
uint8_t fio_u1024_reduce_or8(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_sub16`

```c
uint16_t fio_u1024_reduce_sub16(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_sub32`

```c
uint32_t fio_u1024_reduce_sub32(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_sub64`

```c
uint64_t fio_u1024_reduce_sub64(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_sub8`

```c
uint8_t fio_u1024_reduce_sub8(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_xor16`

```c
uint16_t fio_u1024_reduce_xor16(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_xor32`

```c
uint32_t fio_u1024_reduce_xor32(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_xor64`

```c
uint64_t fio_u1024_reduce_xor64(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_reduce_xor8`

```c
uint8_t fio_u1024_reduce_xor8(const fio_u1024 *a)
```



_Symbol type:_ `function`

#### `fio_u1024_rrot16`

```c
void fio_u1024_rrot16(fio_u1024 *target, const fio_u1024 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u1024_rrot32`

```c
void fio_u1024_rrot32(fio_u1024 *target, const fio_u1024 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u1024_rrot64`

```c
void fio_u1024_rrot64(fio_u1024 *target, const fio_u1024 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u1024_rrot8`

```c
void fio_u1024_rrot8(fio_u1024 *target, const fio_u1024 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u1024_sub16`

```c
void fio_u1024_sub16(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_sub32`

```c
void fio_u1024_sub32(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_sub64`

```c
void fio_u1024_sub64(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_sub8`

```c
void fio_u1024_sub8(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_xor`

```c
void fio_u1024_xor(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_xor16`

```c
void fio_u1024_xor16(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_xor32`

```c
void fio_u1024_xor32(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_xor64`

```c
void fio_u1024_xor64(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u1024_xor8`

```c
void fio_u1024_xor8(fio_u1024 *target, const fio_u1024 *a, const fio_u1024 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_3xor`

```c
void fio_u2048_3xor(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *c)
```



_Symbol type:_ `function`

#### `fio_u2048_3xor32`

```c
void fio_u2048_3xor32(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *c)
```



_Symbol type:_ `function`

#### `fio_u2048_3xor64`

```c
void fio_u2048_3xor64(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *c)
```



_Symbol type:_ `function`

#### `fio_u2048_add16`

```c
void fio_u2048_add16(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_add32`

```c
void fio_u2048_add32(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_add64`

```c
void fio_u2048_add64(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_add8`

```c
void fio_u2048_add8(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_and`

```c
void fio_u2048_and(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_and16`

```c
void fio_u2048_and16(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_and32`

```c
void fio_u2048_and32(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_and64`

```c
void fio_u2048_and64(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_and8`

```c
void fio_u2048_and8(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_cadd16`

```c
void fio_u2048_cadd16(fio_u2048 *target, const fio_u2048 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cadd32`

```c
void fio_u2048_cadd32(fio_u2048 *target, const fio_u2048 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cadd64`

```c
void fio_u2048_cadd64(fio_u2048 *target, const fio_u2048 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cadd8`

```c
void fio_u2048_cadd8(fio_u2048 *target, const fio_u2048 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cand16`

```c
void fio_u2048_cand16(fio_u2048 *target, const fio_u2048 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cand32`

```c
void fio_u2048_cand32(fio_u2048 *target, const fio_u2048 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cand64`

```c
void fio_u2048_cand64(fio_u2048 *target, const fio_u2048 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cand8`

```c
void fio_u2048_cand8(fio_u2048 *target, const fio_u2048 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_clrot16`

```c
void fio_u2048_clrot16(fio_u2048 *target, const fio_u2048 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u2048_clrot32`

```c
void fio_u2048_clrot32(fio_u2048 *target, const fio_u2048 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u2048_clrot64`

```c
void fio_u2048_clrot64(fio_u2048 *target, const fio_u2048 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u2048_clrot8`

```c
void fio_u2048_clrot8(fio_u2048 *target, const fio_u2048 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u2048_cmul16`

```c
void fio_u2048_cmul16(fio_u2048 *target, const fio_u2048 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cmul32`

```c
void fio_u2048_cmul32(fio_u2048 *target, const fio_u2048 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cmul64`

```c
void fio_u2048_cmul64(fio_u2048 *target, const fio_u2048 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cmul8`

```c
void fio_u2048_cmul8(fio_u2048 *target, const fio_u2048 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cor16`

```c
void fio_u2048_cor16(fio_u2048 *target, const fio_u2048 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cor32`

```c
void fio_u2048_cor32(fio_u2048 *target, const fio_u2048 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cor64`

```c
void fio_u2048_cor64(fio_u2048 *target, const fio_u2048 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cor8`

```c
void fio_u2048_cor8(fio_u2048 *target, const fio_u2048 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_crrot16`

```c
void fio_u2048_crrot16(fio_u2048 *target, const fio_u2048 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u2048_crrot32`

```c
void fio_u2048_crrot32(fio_u2048 *target, const fio_u2048 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u2048_crrot64`

```c
void fio_u2048_crrot64(fio_u2048 *target, const fio_u2048 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u2048_crrot8`

```c
void fio_u2048_crrot8(fio_u2048 *target, const fio_u2048 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u2048_csub16`

```c
void fio_u2048_csub16(fio_u2048 *target, const fio_u2048 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_csub32`

```c
void fio_u2048_csub32(fio_u2048 *target, const fio_u2048 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_csub64`

```c
void fio_u2048_csub64(fio_u2048 *target, const fio_u2048 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_csub8`

```c
void fio_u2048_csub8(fio_u2048 *target, const fio_u2048 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_ct_swap_if`

```c
void fio_u2048_ct_swap_if(bool cond, fio_u2048 *restrict a, fio_u2048 *restrict b)
```



_Symbol type:_ `function`

#### `fio_u2048_cxor16`

```c
void fio_u2048_cxor16(fio_u2048 *target, const fio_u2048 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cxor32`

```c
void fio_u2048_cxor32(fio_u2048 *target, const fio_u2048 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cxor64`

```c
void fio_u2048_cxor64(fio_u2048 *target, const fio_u2048 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_cxor8`

```c
void fio_u2048_cxor8(fio_u2048 *target, const fio_u2048 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u2048_inv`

```c
void fio_u2048_inv(fio_u2048 *target, const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_is_eq`

```c
bool fio_u2048_is_eq(const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_lrot16`

```c
void fio_u2048_lrot16(fio_u2048 *target, const fio_u2048 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u2048_lrot32`

```c
void fio_u2048_lrot32(fio_u2048 *target, const fio_u2048 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u2048_lrot64`

```c
void fio_u2048_lrot64(fio_u2048 *target, const fio_u2048 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u2048_lrot8`

```c
void fio_u2048_lrot8(fio_u2048 *target, const fio_u2048 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u2048_maj`

```c
void fio_u2048_maj(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *c)
```



_Symbol type:_ `function`

#### `fio_u2048_maj32`

```c
void fio_u2048_maj32(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *c)
```



_Symbol type:_ `function`

#### `fio_u2048_maj64`

```c
void fio_u2048_maj64(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *c)
```



_Symbol type:_ `function`

#### `fio_u2048_mul16`

```c
void fio_u2048_mul16(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_mul32`

```c
void fio_u2048_mul32(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_mul64`

```c
void fio_u2048_mul64(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_mul8`

```c
void fio_u2048_mul8(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_mux`

```c
void fio_u2048_mux(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *c)
```



_Symbol type:_ `function`

#### `fio_u2048_mux32`

```c
void fio_u2048_mux32(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *c)
```



_Symbol type:_ `function`

#### `fio_u2048_mux64`

```c
void fio_u2048_mux64(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *c)
```



_Symbol type:_ `function`

#### `fio_u2048_or`

```c
void fio_u2048_or(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_or16`

```c
void fio_u2048_or16(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_or32`

```c
void fio_u2048_or32(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_or64`

```c
void fio_u2048_or64(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_or8`

```c
void fio_u2048_or8(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_add16`

```c
uint16_t fio_u2048_reduce_add16(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_add32`

```c
uint32_t fio_u2048_reduce_add32(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_add64`

```c
uint64_t fio_u2048_reduce_add64(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_add8`

```c
uint8_t fio_u2048_reduce_add8(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_and16`

```c
uint16_t fio_u2048_reduce_and16(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_and32`

```c
uint32_t fio_u2048_reduce_and32(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_and64`

```c
uint64_t fio_u2048_reduce_and64(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_and8`

```c
uint8_t fio_u2048_reduce_and8(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_mul16`

```c
uint16_t fio_u2048_reduce_mul16(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_mul32`

```c
uint32_t fio_u2048_reduce_mul32(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_mul64`

```c
uint64_t fio_u2048_reduce_mul64(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_mul8`

```c
uint8_t fio_u2048_reduce_mul8(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_or16`

```c
uint16_t fio_u2048_reduce_or16(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_or32`

```c
uint32_t fio_u2048_reduce_or32(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_or64`

```c
uint64_t fio_u2048_reduce_or64(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_or8`

```c
uint8_t fio_u2048_reduce_or8(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_sub16`

```c
uint16_t fio_u2048_reduce_sub16(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_sub32`

```c
uint32_t fio_u2048_reduce_sub32(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_sub64`

```c
uint64_t fio_u2048_reduce_sub64(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_sub8`

```c
uint8_t fio_u2048_reduce_sub8(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_xor16`

```c
uint16_t fio_u2048_reduce_xor16(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_xor32`

```c
uint32_t fio_u2048_reduce_xor32(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_xor64`

```c
uint64_t fio_u2048_reduce_xor64(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_reduce_xor8`

```c
uint8_t fio_u2048_reduce_xor8(const fio_u2048 *a)
```



_Symbol type:_ `function`

#### `fio_u2048_rrot16`

```c
void fio_u2048_rrot16(fio_u2048 *target, const fio_u2048 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u2048_rrot32`

```c
void fio_u2048_rrot32(fio_u2048 *target, const fio_u2048 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u2048_rrot64`

```c
void fio_u2048_rrot64(fio_u2048 *target, const fio_u2048 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u2048_rrot8`

```c
void fio_u2048_rrot8(fio_u2048 *target, const fio_u2048 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u2048_sub16`

```c
void fio_u2048_sub16(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_sub32`

```c
void fio_u2048_sub32(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_sub64`

```c
void fio_u2048_sub64(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_sub8`

```c
void fio_u2048_sub8(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_xor`

```c
void fio_u2048_xor(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_xor16`

```c
void fio_u2048_xor16(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_xor32`

```c
void fio_u2048_xor32(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_xor64`

```c
void fio_u2048_xor64(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u2048_xor8`

```c
void fio_u2048_xor8(fio_u2048 *target, const fio_u2048 *a, const fio_u2048 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_3xor`

```c
void fio_u4096_3xor(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b, const fio_u4096 *c)
```



_Symbol type:_ `function`

#### `fio_u4096_3xor32`

```c
void fio_u4096_3xor32(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b, const fio_u4096 *c)
```



_Symbol type:_ `function`

#### `fio_u4096_3xor64`

```c
void fio_u4096_3xor64(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b, const fio_u4096 *c)
```



_Symbol type:_ `function`

#### `fio_u4096_add16`

```c
void fio_u4096_add16(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_add32`

```c
void fio_u4096_add32(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_add64`

```c
void fio_u4096_add64(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_add8`

```c
void fio_u4096_add8(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_and`

```c
void fio_u4096_and(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_and16`

```c
void fio_u4096_and16(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_and32`

```c
void fio_u4096_and32(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_and64`

```c
void fio_u4096_and64(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_and8`

```c
void fio_u4096_and8(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_cadd16`

```c
void fio_u4096_cadd16(fio_u4096 *target, const fio_u4096 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cadd32`

```c
void fio_u4096_cadd32(fio_u4096 *target, const fio_u4096 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cadd64`

```c
void fio_u4096_cadd64(fio_u4096 *target, const fio_u4096 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cadd8`

```c
void fio_u4096_cadd8(fio_u4096 *target, const fio_u4096 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cand16`

```c
void fio_u4096_cand16(fio_u4096 *target, const fio_u4096 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cand32`

```c
void fio_u4096_cand32(fio_u4096 *target, const fio_u4096 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cand64`

```c
void fio_u4096_cand64(fio_u4096 *target, const fio_u4096 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cand8`

```c
void fio_u4096_cand8(fio_u4096 *target, const fio_u4096 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_clrot16`

```c
void fio_u4096_clrot16(fio_u4096 *target, const fio_u4096 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u4096_clrot32`

```c
void fio_u4096_clrot32(fio_u4096 *target, const fio_u4096 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u4096_clrot64`

```c
void fio_u4096_clrot64(fio_u4096 *target, const fio_u4096 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u4096_clrot8`

```c
void fio_u4096_clrot8(fio_u4096 *target, const fio_u4096 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u4096_cmul16`

```c
void fio_u4096_cmul16(fio_u4096 *target, const fio_u4096 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cmul32`

```c
void fio_u4096_cmul32(fio_u4096 *target, const fio_u4096 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cmul64`

```c
void fio_u4096_cmul64(fio_u4096 *target, const fio_u4096 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cmul8`

```c
void fio_u4096_cmul8(fio_u4096 *target, const fio_u4096 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cor16`

```c
void fio_u4096_cor16(fio_u4096 *target, const fio_u4096 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cor32`

```c
void fio_u4096_cor32(fio_u4096 *target, const fio_u4096 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cor64`

```c
void fio_u4096_cor64(fio_u4096 *target, const fio_u4096 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cor8`

```c
void fio_u4096_cor8(fio_u4096 *target, const fio_u4096 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_crrot16`

```c
void fio_u4096_crrot16(fio_u4096 *target, const fio_u4096 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u4096_crrot32`

```c
void fio_u4096_crrot32(fio_u4096 *target, const fio_u4096 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u4096_crrot64`

```c
void fio_u4096_crrot64(fio_u4096 *target, const fio_u4096 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u4096_crrot8`

```c
void fio_u4096_crrot8(fio_u4096 *target, const fio_u4096 *a, const uint8_t bts)
```



_Symbol type:_ `function`

#### `fio_u4096_csub16`

```c
void fio_u4096_csub16(fio_u4096 *target, const fio_u4096 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_csub32`

```c
void fio_u4096_csub32(fio_u4096 *target, const fio_u4096 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_csub64`

```c
void fio_u4096_csub64(fio_u4096 *target, const fio_u4096 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_csub8`

```c
void fio_u4096_csub8(fio_u4096 *target, const fio_u4096 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_ct_swap_if`

```c
void fio_u4096_ct_swap_if(bool cond, fio_u4096 *restrict a, fio_u4096 *restrict b)
```



_Symbol type:_ `function`

#### `fio_u4096_cxor16`

```c
void fio_u4096_cxor16(fio_u4096 *target, const fio_u4096 *a, uint16_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cxor32`

```c
void fio_u4096_cxor32(fio_u4096 *target, const fio_u4096 *a, uint32_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cxor64`

```c
void fio_u4096_cxor64(fio_u4096 *target, const fio_u4096 *a, uint64_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_cxor8`

```c
void fio_u4096_cxor8(fio_u4096 *target, const fio_u4096 *a, uint8_t b)
```



_Symbol type:_ `function`

#### `fio_u4096_inv`

```c
void fio_u4096_inv(fio_u4096 *target, const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_is_eq`

```c
bool fio_u4096_is_eq(const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_lrot16`

```c
void fio_u4096_lrot16(fio_u4096 *target, const fio_u4096 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u4096_lrot32`

```c
void fio_u4096_lrot32(fio_u4096 *target, const fio_u4096 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u4096_lrot64`

```c
void fio_u4096_lrot64(fio_u4096 *target, const fio_u4096 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u4096_lrot8`

```c
void fio_u4096_lrot8(fio_u4096 *target, const fio_u4096 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u4096_maj`

```c
void fio_u4096_maj(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b, const fio_u4096 *c)
```



_Symbol type:_ `function`

#### `fio_u4096_maj32`

```c
void fio_u4096_maj32(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b, const fio_u4096 *c)
```



_Symbol type:_ `function`

#### `fio_u4096_maj64`

```c
void fio_u4096_maj64(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b, const fio_u4096 *c)
```



_Symbol type:_ `function`

#### `fio_u4096_mul16`

```c
void fio_u4096_mul16(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_mul32`

```c
void fio_u4096_mul32(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_mul64`

```c
void fio_u4096_mul64(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_mul8`

```c
void fio_u4096_mul8(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_mux`

```c
void fio_u4096_mux(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b, const fio_u4096 *c)
```



_Symbol type:_ `function`

#### `fio_u4096_mux32`

```c
void fio_u4096_mux32(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b, const fio_u4096 *c)
```



_Symbol type:_ `function`

#### `fio_u4096_mux64`

```c
void fio_u4096_mux64(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b, const fio_u4096 *c)
```



_Symbol type:_ `function`

#### `fio_u4096_or`

```c
void fio_u4096_or(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_or16`

```c
void fio_u4096_or16(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_or32`

```c
void fio_u4096_or32(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_or64`

```c
void fio_u4096_or64(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_or8`

```c
void fio_u4096_or8(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_add16`

```c
uint16_t fio_u4096_reduce_add16(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_add32`

```c
uint32_t fio_u4096_reduce_add32(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_add64`

```c
uint64_t fio_u4096_reduce_add64(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_add8`

```c
uint8_t fio_u4096_reduce_add8(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_and16`

```c
uint16_t fio_u4096_reduce_and16(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_and32`

```c
uint32_t fio_u4096_reduce_and32(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_and64`

```c
uint64_t fio_u4096_reduce_and64(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_and8`

```c
uint8_t fio_u4096_reduce_and8(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_mul16`

```c
uint16_t fio_u4096_reduce_mul16(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_mul32`

```c
uint32_t fio_u4096_reduce_mul32(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_mul64`

```c
uint64_t fio_u4096_reduce_mul64(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_mul8`

```c
uint8_t fio_u4096_reduce_mul8(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_or16`

```c
uint16_t fio_u4096_reduce_or16(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_or32`

```c
uint32_t fio_u4096_reduce_or32(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_or64`

```c
uint64_t fio_u4096_reduce_or64(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_or8`

```c
uint8_t fio_u4096_reduce_or8(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_sub16`

```c
uint16_t fio_u4096_reduce_sub16(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_sub32`

```c
uint32_t fio_u4096_reduce_sub32(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_sub64`

```c
uint64_t fio_u4096_reduce_sub64(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_sub8`

```c
uint8_t fio_u4096_reduce_sub8(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_xor16`

```c
uint16_t fio_u4096_reduce_xor16(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_xor32`

```c
uint32_t fio_u4096_reduce_xor32(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_xor64`

```c
uint64_t fio_u4096_reduce_xor64(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_reduce_xor8`

```c
uint8_t fio_u4096_reduce_xor8(const fio_u4096 *a)
```



_Symbol type:_ `function`

#### `fio_u4096_rrot16`

```c
void fio_u4096_rrot16(fio_u4096 *target, const fio_u4096 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u4096_rrot32`

```c
void fio_u4096_rrot32(fio_u4096 *target, const fio_u4096 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u4096_rrot64`

```c
void fio_u4096_rrot64(fio_u4096 *target, const fio_u4096 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u4096_rrot8`

```c
void fio_u4096_rrot8(fio_u4096 *target, const fio_u4096 *a, const uint8_t *rotations)
```



_Symbol type:_ `function`

#### `fio_u4096_sub16`

```c
void fio_u4096_sub16(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_sub32`

```c
void fio_u4096_sub32(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_sub64`

```c
void fio_u4096_sub64(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_sub8`

```c
void fio_u4096_sub8(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_xor`

```c
void fio_u4096_xor(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_xor16`

```c
void fio_u4096_xor16(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_xor32`

```c
void fio_u4096_xor32(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_xor64`

```c
void fio_u4096_xor64(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u4096_xor8`

```c
void fio_u4096_xor8(fio_u4096 *target, const fio_u4096 *a, const fio_u4096 *b)
```



_Symbol type:_ `function`

#### `fio_u128_addv64`

```c
fio_u128 fio_u128_addv64(fio_u128 a, fio_u128 b)
```

ADD two values as 64-bit lanes, returning result by value.

_Symbol type:_ `function`

#### `fio_u128_andv`

```c
fio_u128 fio_u128_andv(fio_u128 a, fio_u128 b)
```

AND two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u128_orv`

```c
fio_u128 fio_u128_orv(fio_u128 a, fio_u128 b)
```

OR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u128_xorv`

```c
fio_u128 fio_u128_xorv(fio_u128 a, fio_u128 b)
```

XOR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u256_addv64`

```c
fio_u256 fio_u256_addv64(fio_u256 a, fio_u256 b)
```

ADD two values as 64-bit lanes, returning result by value.

_Symbol type:_ `function`

#### `fio_u256_andv`

```c
fio_u256 fio_u256_andv(fio_u256 a, fio_u256 b)
```

AND two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u256_orv`

```c
fio_u256 fio_u256_orv(fio_u256 a, fio_u256 b)
```

OR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u256_xorv`

```c
fio_u256 fio_u256_xorv(fio_u256 a, fio_u256 b)
```

XOR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u512_addv64`

```c
fio_u512 fio_u512_addv64(fio_u512 a, fio_u512 b)
```

ADD two values as 64-bit lanes, returning result by value.

_Symbol type:_ `function`

#### `fio_u512_andv`

```c
fio_u512 fio_u512_andv(fio_u512 a, fio_u512 b)
```

AND two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u512_orv`

```c
fio_u512 fio_u512_orv(fio_u512 a, fio_u512 b)
```

OR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u512_xorv`

```c
fio_u512 fio_u512_xorv(fio_u512 a, fio_u512 b)
```

XOR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u1024_addv64`

```c
fio_u1024 fio_u1024_addv64(fio_u1024 a, fio_u1024 b)
```

ADD two values as 64-bit lanes, returning result by value.

_Symbol type:_ `function`

#### `fio_u1024_andv`

```c
fio_u1024 fio_u1024_andv(fio_u1024 a, fio_u1024 b)
```

AND two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u1024_orv`

```c
fio_u1024 fio_u1024_orv(fio_u1024 a, fio_u1024 b)
```

OR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u1024_xorv`

```c
fio_u1024 fio_u1024_xorv(fio_u1024 a, fio_u1024 b)
```

XOR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u2048_addv64`

```c
fio_u2048 fio_u2048_addv64(fio_u2048 a, fio_u2048 b)
```

ADD two values as 64-bit lanes, returning result by value.

_Symbol type:_ `function`

#### `fio_u2048_andv`

```c
fio_u2048 fio_u2048_andv(fio_u2048 a, fio_u2048 b)
```

AND two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u2048_orv`

```c
fio_u2048 fio_u2048_orv(fio_u2048 a, fio_u2048 b)
```

OR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u2048_xorv`

```c
fio_u2048 fio_u2048_xorv(fio_u2048 a, fio_u2048 b)
```

XOR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u4096_addv64`

```c
fio_u4096 fio_u4096_addv64(fio_u4096 a, fio_u4096 b)
```

ADD two values as 64-bit lanes, returning result by value.

_Symbol type:_ `function`

#### `fio_u4096_andv`

```c
fio_u4096 fio_u4096_andv(fio_u4096 a, fio_u4096 b)
```

AND two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u4096_orv`

```c
fio_u4096 fio_u4096_orv(fio_u4096 a, fio_u4096 b)
```

OR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u4096_xorv`

```c
fio_u4096 fio_u4096_xorv(fio_u4096 a, fio_u4096 b)
```

XOR two values, returning result by value.

_Symbol type:_ `function`

#### `fio_u128_add`

```c
uint64_t fio_u128_add(fio_u128 *result, const fio_u128 *a, const fio_u128 *b)
```

Performs A+B, storing in `result`. Return the carry bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u128_cmp`

```c
int fio_u128_cmp(const fio_u128 *a, const fio_u128 *b)
```

Returns -1, 0, or 1 if a < b, a == b or a > a (respectively).

_Symbol type:_ `function`

#### `fio_u128_sub`

```c
uint64_t fio_u128_sub(fio_u128 *result, const fio_u128 *a, const fio_u128 *b)
```

Performs A-B, storing in `result`. Returns the borrow bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u256_add`

```c
uint64_t fio_u256_add(fio_u256 *result, const fio_u256 *a, const fio_u256 *b)
```

Performs A+B, storing in `result`. Return the carry bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u256_cmp`

```c
int fio_u256_cmp(const fio_u256 *a, const fio_u256 *b)
```

Returns -1, 0, or 1 if a < b, a == b or a > a (respectively).

_Symbol type:_ `function`

#### `fio_u256_sub`

```c
uint64_t fio_u256_sub(fio_u256 *result, const fio_u256 *a, const fio_u256 *b)
```

Performs A-B, storing in `result`. Returns the borrow bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u512_add`

```c
uint64_t fio_u512_add(fio_u512 *result, const fio_u512 *a, const fio_u512 *b)
```

Performs A+B, storing in `result`. Return the carry bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u512_cmp`

```c
int fio_u512_cmp(const fio_u512 *a, const fio_u512 *b)
```

Returns -1, 0, or 1 if a < b, a == b or a > a (respectively).

_Symbol type:_ `function`

#### `fio_u512_sub`

```c
uint64_t fio_u512_sub(fio_u512 *result, const fio_u512 *a, const fio_u512 *b)
```

Performs A-B, storing in `result`. Returns the borrow bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u1024_add`

```c
uint64_t fio_u1024_add(fio_u1024 *result, const fio_u1024 *a, const fio_u1024 *b)
```

Performs A+B, storing in `result`. Return the carry bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u1024_cmp`

```c
int fio_u1024_cmp(const fio_u1024 *a, const fio_u1024 *b)
```

Returns -1, 0, or 1 if a < b, a == b or a > a (respectively).

_Symbol type:_ `function`

#### `fio_u1024_sub`

```c
uint64_t fio_u1024_sub(fio_u1024 *result, const fio_u1024 *a, const fio_u1024 *b)
```

Performs A-B, storing in `result`. Returns the borrow bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u2048_add`

```c
uint64_t fio_u2048_add(fio_u2048 *result, const fio_u2048 *a, const fio_u2048 *b)
```

Performs A+B, storing in `result`. Return the carry bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u2048_cmp`

```c
int fio_u2048_cmp(const fio_u2048 *a, const fio_u2048 *b)
```

Returns -1, 0, or 1 if a < b, a == b or a > a (respectively).

_Symbol type:_ `function`

#### `fio_u2048_sub`

```c
uint64_t fio_u2048_sub(fio_u2048 *result, const fio_u2048 *a, const fio_u2048 *b)
```

Performs A-B, storing in `result`. Returns the borrow bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u4096_add`

```c
uint64_t fio_u4096_add(fio_u4096 *result, const fio_u4096 *a, const fio_u4096 *b)
```

Performs A+B, storing in `result`. Return the carry bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u4096_cmp`

```c
int fio_u4096_cmp(const fio_u4096 *a, const fio_u4096 *b)
```

Returns -1, 0, or 1 if a < b, a == b or a > a (respectively).

_Symbol type:_ `function`

#### `fio_u4096_sub`

```c
uint64_t fio_u4096_sub(fio_u4096 *result, const fio_u4096 *a, const fio_u4096 *b)
```

Performs A-B, storing in `result`. Returns the borrow bit (1 or 0).

_Symbol type:_ `function`

#### `fio_u128_montgomery_mul`

```c
void fio_u128_montgomery_mul(fio_u128 *result, const fio_u128 *a, const fio_u128 *b, const fio_u128 *N, const fio_u128 *N_dash)
```



_Symbol type:_ `function`

#### `fio_u128_mul`

```c
void fio_u128_mul(fio_u256 *result, const fio_u128 *a, const fio_u128 *b)
```

Multiplies A and B, storing the result in `result`.

_Symbol type:_ `function`

#### `fio_u256_montgomery_mul`

```c
void fio_u256_montgomery_mul(fio_u256 *result, const fio_u256 *a, const fio_u256 *b, const fio_u256 *N, const fio_u256 *N_dash)
```



_Symbol type:_ `function`

#### `fio_u256_mul`

```c
void fio_u256_mul(fio_u512 *result, const fio_u256 *a, const fio_u256 *b)
```

Multiplies A and B, storing the result in `result`.

_Symbol type:_ `function`

#### `fio_u512_montgomery_mul`

```c
void fio_u512_montgomery_mul(fio_u512 *result, const fio_u512 *a, const fio_u512 *b, const fio_u512 *N, const fio_u512 *N_dash)
```



_Symbol type:_ `function`

#### `fio_u512_mul`

```c
void fio_u512_mul(fio_u1024 *result, const fio_u512 *a, const fio_u512 *b)
```

Multiplies A and B, storing the result in `result`.

_Symbol type:_ `function`

#### `fio_u1024_montgomery_mul`

```c
void fio_u1024_montgomery_mul(fio_u1024 *result, const fio_u1024 *a, const fio_u1024 *b, const fio_u1024 *N, const fio_u1024 *N_dash)
```



_Symbol type:_ `function`

#### `fio_u1024_mul`

```c
void fio_u1024_mul(fio_u2048 *result, const fio_u1024 *a, const fio_u1024 *b)
```

Multiplies A and B, storing the result in `result`.

_Symbol type:_ `function`

#### `fio_u2048_montgomery_mul`

```c
void fio_u2048_montgomery_mul(fio_u2048 *result, const fio_u2048 *a, const fio_u2048 *b, const fio_u2048 *N, const fio_u2048 *N_dash)
```



_Symbol type:_ `function`

#### `fio_u2048_mul`

```c
void fio_u2048_mul(fio_u4096 *result, const fio_u2048 *a, const fio_u2048 *b)
```

Multiplies A and B, storing the result in `result`.

_Symbol type:_ `function`

#### `fio_cycle_counter`

```c
inline uint64_t fio_cycle_counter(void)
```



_Symbol type:_ `function`

#### `fio_utf8_code_len`

```c
inline FIO_CONST unsigned fio_utf8_code_len(uint32_t u)
```



_Symbol type:_ `function`

#### `fio_utf8_char_len_unsafe`

```c
inline FIO_CONST unsigned fio_utf8_char_len_unsafe(uint8_t c)
```

Returns 1-4 (UTF-8 char length), 8 (middle of a char) or 0 (invalid).

_Symbol type:_ `function`

#### `fio_utf8_char_len`

```c
inline unsigned fio_utf8_char_len(const void *str_)
```

Returns the number of valid UTF-8 bytes used by first char at `str`.

_Symbol type:_ `function`

#### `fio_utf8_write`

```c
inline unsigned fio_utf8_write(void *dest_, uint32_t u)
```

Writes code point to `dest` using UFT-8. Returns number of bytes written.

_Symbol type:_ `function`

#### `fio_utf8_read`

```c
inline uint32_t fio_utf8_read(char **str)
```

Decodes the first UTF-8 char at `str` and returns its code point value.

Advances the pointer at `str` by the number of bytes consumed (read).

_Symbol type:_ `function`

#### `fio_utf8_peek`

```c
inline uint32_t fio_utf8_peek(const char *str)
```

Decodes the first UTF-8 char at `str` and returns its code point value.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-001-header-h"></a> `./fio-stl/001 header.h`

8 public symbols.

### Macros

#### `FIO_MEM_REALLOC`

```c
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)   \
  fio_realloc2((ptr), (new_size), (copy_len))
```

Reallocates memory, copying (at least) `copy_len` if necessary.

_Symbol type:_ `macro`

#### `FIO_MEM_FREE`

```c
#define FIO_MEM_FREE(ptr, size) fio_free((ptr))
```

Frees allocated memory.

_Symbol type:_ `macro`

#### `FIO_MEM_REALLOC_IS_SAFE`

```c
#define FIO_MEM_REALLOC_IS_SAFE fio_realloc_is_safe()
```

Set to true of internall allocator is used (memory returned set to zero).

_Symbol type:_ `macro`

#### `FIO_MEM_ALIGNMENT_SIZE`

```c
#define FIO_MEM_ALIGNMENT_SIZE fio_malloc_alignment()
```

Detect allocator allignment dynamically.

_Symbol type:_ `macro`

#### `FIO_PTR_TAG`

```c
#define FIO_PTR_TAG(p) (p)
```

Supports embedded pointer tagging / untagging for the included types.

Should resolve to a tagged pointer value. i.e.: ((uintptr_t)(p) | 1)

_Symbol type:_ `macro`

#### `FIO_PTR_UNTAG`

```c
#define FIO_PTR_UNTAG(p) (p)
```

Supports embedded pointer tagging / untagging for the included types.

Should resolve to an untagged pointer value. i.e.: ((uintptr_t)(p) | ~1UL)

_Symbol type:_ `macro`

#### `FIO_PTR_TAG_VALIDATE`

```c
#define FIO_PTR_TAG_VALIDATE(ptr) ((ptr) != NULL)
```

If FIO_PTR_TAG_VALIDATE is defined, tagging will be verified before executing
any code.

FIO_PTR_TAG_VALIDATE must fail on NULL pointers.

_Symbol type:_ `macro`

#### `FIO_PTR_TAG_GET_UNTAGGED`

```c
#define FIO_PTR_TAG_GET_UNTAGGED(untagged_type, tagged_ptr)   \
  ((untagged_type *)(FIO_PTR_UNTAG((tagged_ptr))))
```



_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-001-logging-h"></a> `./fio-stl/001 logging.h`

2 public symbols.

### Macros

#### `FIO_STDERR_FILE`

```c
#define FIO_STDERR_FILE stderr
```



_Symbol type:_ `macro`

#### `FIO_LOG_LEVEL_DEFAULT`

```c
#define FIO_LOG_LEVEL_DEFAULT FIO_LOG_LEVEL_DEBUG
```



_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-001-memalt-h"></a> `./fio-stl/001 memalt.h`

5 public symbols.

### Functions

#### `fio_memset`

```c
void *fio_memset(void *restrict dest, uint64_t data, size_t bytes)
```

A somewhat naive implementation of `memset`.

Probably slower than the one included with your compiler's C library.

_Symbol type:_ `function`

#### `fio_memcpy`

```c
void *fio_memcpy(void *dest_, const void *src_, size_t bytes)
```

A somewhat naive implementation of `memcpy`.

Probably slower than the one included with your compiler's C library.

_Symbol type:_ `function`

#### `fio_memchr`

```c
void *fio_memchr(const void *buffer, const char token, size_t len)
```

A token seeking function. This is a fallback for `memchr`, but `memchr`
should be faster.

_Symbol type:_ `function`

#### `fio_memcmp`

```c
int fio_memcmp(const void *a_, const void *b_, size_t len)
```

A comparison function. This is a fallback for `memcmp`, but `memcmp`
should be faster.

_Symbol type:_ `function`

#### `fio_strlen`

```c
size_t fio_strlen(const char *str)
```

An alternative to `strlen` - may raise Address Sanitation errors.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-001-patches-h"></a> `./fio-stl/001 patches.h`

2 public symbols.

### Functions

#### `fio_sys_env`

```c
inline char *fio_sys_env(const char *name)
```

Portable getenv: returns the value of environment variable `name`, or NULL.
Thin wrapper around getenv for API uniformity across platforms.

_Symbol type:_ `function`

#### `fio_sys_env_set`

```c
inline int fio_sys_env_set(const char *name, const char *value, int overwrite)
```

Portable setenv: sets environment variable `name` to `value`.
If `overwrite` is zero and the variable already exists, does nothing.
Returns 0 on success, -1 on failure.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-002-atol-h"></a> `./fio-stl/002 atol.h`

46 public symbols.

### Macros

#### `FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER`

```c
#define FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER 1
```



_Symbol type:_ `macro`

#### `FIO_MATH_DBL_MANT_MASK`

```c
#define FIO_MATH_DBL_MANT_MASK (((uint64_t)1ULL << 52) - 1)
```



_Symbol type:_ `macro`

#### `FIO_MATH_DBL_EXPO_MASK`

```c
#define FIO_MATH_DBL_EXPO_MASK ((uint64_t)2047ULL << 52)
```



_Symbol type:_ `macro`

#### `FIO_MATH_DBL_SIGN_MASK`

```c
#define FIO_MATH_DBL_SIGN_MASK ((uint64_t)1ULL << 63)
```



_Symbol type:_ `macro`

### Types

#### `fio_aton_s`

```c
typedef struct {
union {
int64_t i;
double f;
uint64_t u;
};
int is_float;
int err;
} fio_aton_s
```

Result type for fio_aton

_Symbol type:_ `type`

### Functions

#### `fio_aton`

```c
FIO_SFUNC fio_aton_s fio_aton(char **pstr)
```

Converts a String to a number - either an integer or a float (double).

Skips white space at the beginning of the string.

Auto detects binary and hex formats when prefix is provided (0x / 0b).

Auto detects octal when number starts with zero.

Auto detects the Strings "inf", "infinity" and "nan" as float values.

The number's format and type are returned in the return type.

If a numerical overflow or format error occurred, the `.err` flag is set.

Note: rounding errors may occur, as this is not an `strtod` exact match.

_Symbol type:_ `function`

#### `fio_atol`

```c
int64_t fio_atol(char **pstr)
```

A helper function that converts between String data to a signed int64_t.

Numbers are assumed to be in base 10. Octal (`0###`), Hex (`0x##`/`x##`) and
binary (`0b##`/ `b##`) are recognized as well. For binary Most Significant
Bit must come first.

The most significant difference between this function and `strtol` (aside of
API design), is the added support for binary representations.

_Symbol type:_ `function`

#### `fio_atof`

```c
double fio_atof(char **pstr)
```

A helper function that converts between String data to a signed double.

_Symbol type:_ `function`

#### `fio_ltoa`

```c
size_t fio_ltoa(char *dest, int64_t num, uint8_t base)
```

A helper function that writes a signed int64_t to a string.

No overflow guard is provided, make sure there's at least 68 bytes available
(for base 2).

Offers special support for base 2 (binary), base 8 (octal), base 10 and base
16 (hex) where prefixes are automatically added if required (i.e.,`"0x"` for
hex and `"0b"` for base 2, and `"0"` for octal).

Supports any base up to base 36 (using 0-9,A-Z).

An unsupported base will log an error and print zero.

Returns the number of bytes actually written (excluding the NUL terminator).

_Symbol type:_ `function`

#### `fio_ftoa`

```c
size_t fio_ftoa(char *dest, double num, uint8_t base)
```

A helper function that converts between a double to a string.

No overflow guard is provided, make sure there's at least 130 bytes
available (for base 2).

Supports base 2, base 10 and base 16. An unsupported base will silently
default to base 10. Prefixes aren't added (i.e., no "0x" or "0b" at the
beginning of the string).

Returns the number of bytes actually written (excluding the NUL
terminator).

_Symbol type:_ `function`

#### `fio_c2i`

```c
uint8_t fio_c2i(unsigned char c)
```

Maps characters to alphanumerical value, where numbers have their natural
values (0-9) and `A-Z` (or `a-z`) are the values 10-35.

Out of bound values return 255.

This allows parsing of numeral strings for up to base 36.

_Symbol type:_ `function`

#### `fio_i2c`

```c
uint8_t fio_i2c(unsigned char i)
```

Maps numeral values to alphanumerical characters, where numbers have their
natural values (0-9) and `A-Z` are the values 10-35.

Accepts values up to 63. Returns zero for values over 35. Out of bound values
produce undefined behavior.

This allows printing of numerals for up to base 36.

_Symbol type:_ `function`

#### `fio_digits10`

```c
inline size_t fio_digits10(int64_t i)
```

Returns the number of digits in base 10.

_Symbol type:_ `function`

#### `fio_digits10u`

```c
FIO_SFUNC size_t fio_digits10u(uint64_t i)
```

Returns the number of digits in base 10 for an unsigned number.

_Symbol type:_ `function`

#### `fio_digits8u`

```c
FIO_SFUNC size_t fio_digits8u(uint64_t i)
```

Returns the number of digits in base 8 for an unsigned number.

_Symbol type:_ `function`

#### `fio_digits16u`

```c
FIO_SFUNC size_t fio_digits16u(uint64_t i)
```

Returns the number of digits in base 16 for an unsigned number.

_Symbol type:_ `function`

#### `fio_digits_bin`

```c
FIO_SFUNC size_t fio_digits_bin(uint64_t i)
```

Returns the number of digits in base 2 for an unsigned number.

_Symbol type:_ `function`

#### `fio_digits_xbase`

```c
FIO_SFUNC size_t fio_digits_xbase(uint64_t i, size_t base)
```

Returns the number of digits in any base X<65 for an unsigned number.

_Symbol type:_ `function`

#### `fio_ltoa10`

```c
inline void fio_ltoa10(char *dest, int64_t i, size_t digits)
```

Writes a signed number to `dest` using `digits` bytes (+ `NUL`)

_Symbol type:_ `function`

#### `fio_atol10`

```c
int64_t fio_atol10(char **pstr)
```

Reads a signed base 10 formatted number.

_Symbol type:_ `function`

#### `fio_ltoa10u`

```c
inline void fio_ltoa10u(char *dest, uint64_t i, size_t digits)
```

Writes unsigned number to `dest` using `digits` bytes (+ `NUL`)

_Symbol type:_ `function`

#### `fio_ltoa16u`

```c
inline void fio_ltoa16u(char *dest, uint64_t i, size_t digits)
```

Writes unsigned number to `dest` using `digits` bytes (+ `NUL`)

_Symbol type:_ `function`

#### `fio_ltoa_bin`

```c
inline void fio_ltoa_bin(char *dest, uint64_t i, size_t digits)
```

Writes unsigned number to `dest` using `digits` bytes (+ `NUL`)

_Symbol type:_ `function`

#### `fio_ltoa_xbase`

```c
inline void fio_ltoa_xbase(char *dest, uint64_t i, size_t digits, size_t base)
```

Writes unsigned number to `dest` using `digits` bytes (+ `NUL`)

_Symbol type:_ `function`

#### `fio_atol8u`

```c
uint64_t fio_atol8u(char **pstr)
```

Reads a signed base 8 formatted number - may overflow buffer.

_Symbol type:_ `function`

#### `fio_atol10u`

```c
uint64_t fio_atol10u(char **pstr)
```

Reads a signed base 10 formatted number - may overflow buffer.

_Symbol type:_ `function`

#### `fio_atol16u`

```c
uint64_t fio_atol16u(char **pstr)
```

Reads an unsigned hex formatted number (possibly prefixed with "0x").

_Symbol type:_ `function`

#### `fio_atol_bin`

```c
uint64_t fio_atol_bin(char **pstr)
```

Reads an unsigned binary formatted number (possibly prefixed with "0b").

_Symbol type:_ `function`

#### `fio_atol_xbase`

```c
uint64_t fio_atol_xbase(char **pstr, size_t base)
```

Read an unsigned number in any base up to base 36.

_Symbol type:_ `function`

#### `fio_u2i_limit`

```c
inline int64_t fio_u2i_limit(uint64_t val, size_t invert)
```

Converts an unsigned `val` to a signed `val`, with overflow protection.

_Symbol type:_ `function`

#### `fio_i2d`

```c
inline double fio_i2d(int64_t mant, int64_t exponent_in_base_2)
```

Converts a 64 bit integer to an IEEE 754 formatted double.

_Symbol type:_ `function`

#### `fio_u2d`

```c
inline double fio_u2d(uint64_t mant, int64_t exponent_in_base_2)
```

Converts a 64 bit unsigned integer to an IEEE 754 formatted double.

_Symbol type:_ `function`

#### `fio_u128_hex_read`

```c
fio_u128 fio_u128_hex_read(char **pstr)
```

Reads a hex numeral string and initializes the numeral.

_Symbol type:_ `function`

#### `fio_u256_hex_read`

```c
fio_u256 fio_u256_hex_read(char **pstr)
```

Reads a hex numeral string and initializes the numeral.

_Symbol type:_ `function`

#### `fio_u512_hex_read`

```c
fio_u512 fio_u512_hex_read(char **pstr)
```

Reads a hex numeral string and initializes the numeral.

_Symbol type:_ `function`

#### `fio_u1024_hex_read`

```c
fio_u1024 fio_u1024_hex_read(char **pstr)
```

Reads a hex numeral string and initializes the numeral.

_Symbol type:_ `function`

#### `fio_u2048_hex_read`

```c
fio_u2048 fio_u2048_hex_read(char **pstr)
```

Reads a hex numeral string and initializes the numeral.

_Symbol type:_ `function`

#### `fio_u4096_hex_read`

```c
fio_u4096 fio_u4096_hex_read(char **pstr)
```

Reads a hex numeral string and initializes the numeral.

_Symbol type:_ `function`

#### `fio_u128_hex_write`

```c
size_t fio_u128_hex_write(char *dest, const fio_u128 *)
```

Prints out the underlying 64 bit array (for debugging).

_Symbol type:_ `function`

#### `fio_u256_hex_write`

```c
size_t fio_u256_hex_write(char *dest, const fio_u256 *)
```

Prints out the underlying 64 bit array (for debugging).

_Symbol type:_ `function`

#### `fio_u512_hex_write`

```c
size_t fio_u512_hex_write(char *dest, const fio_u512 *)
```

Prints out the underlying 64 bit array (for debugging).

_Symbol type:_ `function`

#### `fio_u1024_hex_write`

```c
size_t fio_u1024_hex_write(char *dest, const fio_u1024 *)
```

Prints out the underlying 64 bit array (for debugging).

_Symbol type:_ `function`

#### `fio_u2048_hex_write`

```c
size_t fio_u2048_hex_write(char *dest, const fio_u2048 *)
```

Prints out the underlying 64 bit array (for debugging).

_Symbol type:_ `function`

#### `fio_u4096_hex_write`

```c
size_t fio_u4096_hex_write(char *dest, const fio_u4096 *)
```

Prints out the underlying 64 bit array (for debugging).

_Symbol type:_ `function`

#### `fio_ltoa8u`

```c
inline void fio_ltoa8u(char *dest, uint64_t i, size_t digits)
```



_Symbol type:_ `function`

#### `fio_d2expo`

```c
inline int fio_d2expo(double d)
```



_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-002-crc32-h"></a> `./fio-stl/002 crc32.h`

1 public symbols.

### Functions

#### `fio_crc32`

```c
uint32_t fio_crc32(const void *data, size_t len, uint32_t initial_crc)
```

Computes CRC32 (ITU-T V.42 / ISO 3309 / gzip polynomial 0xEDB88320).

`data`        - pointer to input bytes.
`len`         - number of bytes to process.
`initial_crc` - pass 0 for a new computation, or a previous return value
                to continue an incremental CRC32 over multiple buffers.

Returns the CRC32 checksum.

Uses slicing-by-16 internally (~8-16x faster than byte-at-a-time).

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-002-glob-matching-h"></a> `./fio-stl/002 glob matching.h`

1 public symbols.

### Functions

#### `fio_glob_match`

```c
uint8_t fio_glob_match(fio_str_info_s pattern, fio_str_info_s string)
```

A binary glob matching helper. Returns 1 on match, otherwise returns 0.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-002-imap-h"></a> `./fio-stl/002 imap.h`

7 public symbols.

### Macros

#### `FIO_IMAP_ALWAYS_VALID`

```c
#define FIO_IMAP_ALWAYS_VALID(o) (1)
```

Helper macro for simple iMap array types - always valid.

_Symbol type:_ `macro`

#### `FIO_IMAP_VALID_NON_ZERO`

```c
#define FIO_IMAP_VALID_NON_ZERO(o) (!!((o)[0]))
```

Helper macro for simple iMap array types - valid if nonzero.

_Symbol type:_ `macro`

#### `FIO_IMAP_ALWAYS_CMP_TRUE`

```c
#define FIO_IMAP_ALWAYS_CMP_TRUE(a, b) (1)
```

Helper macro for simple iMap array types - type comparison always true.

_Symbol type:_ `macro`

#### `FIO_IMAP_ALWAYS_CMP_FALSE`

```c
#define FIO_IMAP_ALWAYS_CMP_FALSE(a, b) (0)
```

Helper macro for simple iMap array types - type comparison always false.

_Symbol type:_ `macro`

#### `FIO_IMAP_SIMPLE_CMP`

```c
#define FIO_IMAP_SIMPLE_CMP(a, b) ((a)[0] == (b)[0])
```

Helper macro for simple iMap array types - simple comparison.

_Symbol type:_ `macro`

#### `FIO_IMAP_EACH`

```c
#define FIO_IMAP_EACH(array_name, map_ptr, i)   \
  for (size_t i = 0; i < (map_ptr)->w; ++i)   \
    if (!FIO_NAME(array_name, is_valid)((map_ptr)->ary + i))   \
      continue;   \
    else
```

Helper macro for simple iMap array types.

_Symbol type:_ `macro`

#### `FIO_TYPEDEF_IMAP_ARRAY`

```c
#define FIO_TYPEDEF_IMAP_ARRAY(array_name,   \
                               array_type,   \
                               imap_type,   \
                               hash_fn,   \
                               cmp_fn,   \
                               is_valid_fn)   \
  FIO_LEAK_COUNTER_DEF(FIO_NAME(array_name, s))   \
  typedef struct {   \
    array_type *ary;   \
    imap_type count;   \
    imap_type w;   \
    uint32_t capa_bits;   \
  } FIO_NAME(array_name, s);   \
  typedef struct {   \
    imap_type pos;   \
    imap_type ipos;   \
    imap_type set_val;   \
  } FIO_NAME(array_name, seeker_s);   \
  /** Returns the theoretical capacity for the indexed array. */   \
  FIO_IFUNC int FIO_NAME(array_name, is_valid)(array_type * pobj) {   \
    return pobj && (!!is_valid_fn(pobj));   \
  }   \
  /** Returns the theoretical capacity for the indexed array. */   \
  FIO_IFUNC size_t FIO_NAME(array_name, capa)(FIO_NAME(array_name, s) * a) {   \
    if (!a || !a->capa_bits)   \
      return 0;   \
    return ((size_t)1ULL << a->capa_bits);   \
  }   \
  /** Returns a pointer to the index map. */   \
  FIO_IFUNC imap_type *FIO_NAME(array_name,   \
                                imap)(FIO_NAME(array_name, s) * a) {   \
    return (imap_type *)(a->ary + ((imap_type)1ULL << a->capa_bits));   \
  }   \
  /** Deallocates dynamic memory. */   \
  FIO_IFUNC void FIO_NAME(array_name, destroy)(FIO_NAME(array_name, s) * a) {   \
    size_t capa = FIO_NAME(array_name, capa)(a);   \
    if (a->ary) {   \
      FIO_LEAK_COUNTER_ON_FREE(FIO_NAME(array_name, s));   \
      FIO_TYPEDEF_IMAP_FREE(   \
          a->ary,   \
          (capa * (sizeof(*a->ary)) + (capa * (sizeof(imap_type)))));   \
    }   \
    *a = (FIO_NAME(array_name, s)){0};   \
    (void)capa; /* if unused */   \
  }   \
  /** Allocates dynamic memory. */   \
  FIO_IFUNC int FIO_NAME(array_name, __alloc)(FIO_NAME(array_name, s) * a,   \
                                              size_t bits) {   \
    if (!bits || bits > ((sizeof(imap_type) << 3) - 2))   \
      return -1;   \
    size_t capa = 1ULL << bits;   \
    if (bits > (size_t)(FIO_MAP_WARNING_BITSIZE - 1))   \
      FIO_LOG_WARNING(   \
          "The " #array_name "_s map is now using a LOT of memory - %zu Mb!",   \
          (capa >> 20) * (sizeof(array_type) + sizeof(imap_type)));   \
    size_t old_capa = FIO_NAME(array_name, capa)(a);   \
    array_type *tmp = (array_type *)FIO_TYPEDEF_IMAP_REALLOC(   \
        a->ary,   \
        (a->capa_bits ? (old_capa * (sizeof(array_type)) +   \
                         (old_capa * (sizeof(imap_type))))   \
                      : 0),   \
        (capa * (sizeof(array_type)) + (capa * (sizeof(imap_type)))),   \
        (a->w * (sizeof(array_type))));   \
    (void)old_capa; /* if unused */   \
    if (!tmp)   \
      return -1;   \
    if (!a->ary)   \
      FIO_LEAK_COUNTER_ON_ALLOC(FIO_NAME(array_name, s));   \
    a->capa_bits = (uint32_t)bits;   \
    a->ary = tmp;   \
    if (!FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE) {   \
      FIO_MEMSET((tmp + a->w), 0, ((capa - a->w) * (sizeof(*tmp))));   \
      FIO_MEMSET((tmp + capa), 0, (capa * (sizeof(imap_type))));   \
    }   \
    return 0;   \
  }   \
  /** Returns the index map position and array position of a value, if any. */   \
  FIO_SFUNC FIO_NAME(array_name, seeker_s)   \
      FIO_NAME(array_name, seek)(FIO_NAME(array_name, s) * a,   \
                                 array_type * pobj) {   \
    FIO_NAME(array_name, seeker_s)   \
    r = {0, ((imap_type) ~(imap_type)0), ((imap_type) ~(imap_type)0)};   \
    if (!a || ((!a->capa_bits) | (!a->ary)))   \
      return r;   \
    r.pos = a->w;   \
    imap_type capa = (imap_type)1UL << a->capa_bits;   \
    imap_type *imap = (imap_type *)(a->ary + capa);   \
    const imap_type pos_mask = (imap_type)(capa - (imap_type)1);   \
    const imap_type hash_mask = (imap_type)~pos_mask;   \
    const imap_type hash = (imap_type)hash_fn(pobj);   \
    imap_type tester = (hash & hash_mask); /* hides lower bits for `tester` */   \
    imap_type pos = hash + (hash >> a->capa_bits); /* use more bits for pos */   \
    tester += (imap_type)((!tester) << a->capa_bits);   \
    tester -= (imap_type)((hash_mask == tester) << a->capa_bits);   \
    size_t attempts = 11;   \
    for (;;) {   \
      /* tests up to 3 groups of 4 bytes (uint32_t) within a 64 byte group */   \
      for (size_t mini_steps = 0;;) {   \
        pos &= pos_mask;   \
        const imap_type pos_hash = imap[pos] & hash_mask;   \
        const imap_type pos_index = imap[pos] & pos_mask;   \
        if ((pos_hash == tester) && cmp_fn((a->ary + pos_index), pobj)) {   \
          r.ipos = pos;   \
          r.pos = pos_index;   \
          r.set_val = tester | pos_index;   \
          return r;   \
        }   \
        if (!imap[pos]) {   \
          r.ipos = pos;   \
          r.set_val = tester | r.pos; /* r.pos == a->w */   \
          return r;   \
        }   \
        if (imap[pos] == (imap_type)(~(imap_type)0)) {   \
          r.ipos = pos;   \
          r.set_val = tester | r.pos; /* r.pos == a->w */   \
        }   \
        if (!((--attempts)))   \
          return r;   \
        if (mini_steps == 2)   \
          break;   \
        pos += 3 + mini_steps; /* 0, 3, 7 =  max of 56 byte distance */   \
        ++mini_steps;   \
      }   \
      pos += (imap_type)0xC19F5985UL; /* big step */   \
    }   \
  }   \
  /** fills an empty imap with the info about existing elements. */   \
  FIO_SFUNC int FIO_NAME(array_name,   \
                         __fill_imap)(FIO_NAME(array_name, s) * a) {   \
    if (!a->count) {   \
      a->w = 0;   \
      return 0;   \
    }   \
    imap_type *imap = FIO_NAME(array_name, imap)(a);   \
    if (a->count != a->w) {   \
      a->count = 0;   \
      for (size_t i = 0; i < a->w; ++i) {   \
        if (!is_valid_fn((a->ary + i)))   \
          continue;   \
        if (a->count != i)   \
          a->ary[a->count] = a->ary[i];   \
        ++a->count;   \
      }   \
    }   \
    for (a->w = 0; a->w < a->count; ++(a->w)) {   \
      FIO_NAME(array_name, seeker_s)   \
      s = FIO_NAME(array_name, seek)(a, a->ary + a->w);   \
      if (s.pos != a->w || s.ipos == (imap_type)(~(imap_type)0)) {   \
        a->w = a->count;   \
        return -1; /* destination not big enough to contain collisions! */   \
      }   \
      imap[s.ipos] = s.set_val;   \
    }   \
    a->w = a->count;   \
    return 0;   \
  }   \
  /** expands the existing array & imap storage capacity. */   \
  FIO_IFUNC int FIO_NAME(array_name, __expand)(FIO_NAME(array_name, s) * a) {   \
    for (;;) {   \
      if (FIO_NAME(array_name, __alloc)(a,   \
                                        a->capa_bits + 1 + (!a->capa_bits)))   \
        return -1;   \
      if (!FIO_NAME(array_name, __fill_imap)(a))   \
        return 0;   \
    }   \
  }   \
  /** Reserves a minimum imap storage capacity. */   \
  FIO_IFUNC int FIO_NAME(array_name, reserve)(FIO_NAME(array_name, s) * a,   \
                                              imap_type min) {   \
    imap_type bits = 2;   \
    if (min > ((imap_type)~0ULL) >> 1)   \
      return -1;   \
    while ((1ULL << bits) < min)   \
      ++bits;   \
    if (bits <= a->capa_bits)   \
      return 0;   \
    if (FIO_NAME(array_name, __alloc)(a, bits))   \
      return -1;   \
    if (!FIO_NAME(array_name, __fill_imap)(a))   \
      return 0;   \
    return FIO_NAME(array_name, __expand)(a);   \
  }   \
  /** Rehashes the array and fills the imap (use after sorting). */   \
  FIO_IFUNC int FIO_NAME(array_name, rehash)(FIO_NAME(array_name, s) * a) {   \
    if (!a || !a->ary)   \
      return -1;   \
    size_t bytes = sizeof(imap_type) * ((size_t)1ULL << a->capa_bits);   \
    imap_type *imap = FIO_NAME(array_name, imap)(a);   \
    FIO_MEMSET(imap, 0, bytes);   \
    if (!FIO_NAME(array_name, __fill_imap)(a))   \
      return -1;   \
    return 0;   \
  }   \
  /** Sets an object in the Array. Optionally overwrites existing data. */   \
  FIO_IFUNC array_type *FIO_NAME(array_name, set)(FIO_NAME(array_name, s) * a,   \
                                                  array_type obj,   \
                                                  int overwrite) {   \
    if (!a || !is_valid_fn((&obj)))   \
      return NULL;   \
    {   \
      size_t capa = FIO_NAME(array_name, capa)(a);   \
      if (a->w == capa)   \
        FIO_NAME(array_name, __expand)(a);   \
      else if (a->count != a->w &&   \
               (a->w + (a->w >> 1)) > FIO_NAME(array_name, capa)(a)) {   \
        FIO_MEMSET((a->ary + capa), 0, (capa * (sizeof(imap_type))));   \
        FIO_NAME(array_name, __fill_imap)(a);   \
      }   \
    }   \
    for (;;) {   \
      FIO_NAME(array_name, seeker_s) s = FIO_NAME(array_name, seek)(a, &obj);   \
      if (s.ipos == (imap_type)(~(imap_type)0)) { /* no room in the imap */   \
        FIO_NAME(array_name, __expand)(a);   \
        continue;   \
      }   \
      if (s.pos == a->w) { /* new object */   \
        a->ary[a->w] = obj;   \
        ++a->w;   \
        ++a->count;   \
        FIO_NAME(array_name, imap)(a)[s.ipos] = s.set_val;   \
        return a->ary + s.pos;   \
      }   \
      FIO_ASSERT_DEBUG(s.pos < a->w && s.ipos < FIO_NAME(array_name, capa)(a),   \
                       "WTF?");   \
      if (!overwrite)   \
        return a->ary + s.pos;   \
      a->ary[s.pos] = obj;   \
      return a->ary + s.pos;   \
    }   \
  }   \
  /** Finds an object in the Array using the index map. */   \
  FIO_IFUNC array_type *FIO_NAME(array_name, get)(FIO_NAME(array_name, s) * a,   \
                                                  array_type obj) {   \
    if (!a || !is_valid_fn((&obj)))   \
      return NULL;   \
    FIO_NAME(array_name, seeker_s) s = FIO_NAME(array_name, seek)(a, &obj);   \
    if (s.pos >= a->w)   \
      return NULL;   \
    return a->ary + s.pos;   \
  }   \
  /** Removes an object in the Array's index map, zeroing out its memory. */   \
  FIO_IFUNC int FIO_NAME(array_name, remove)(FIO_NAME(array_name, s) * a,   \
                                             array_type obj) {   \
    if (!a || !is_valid_fn((&obj)))   \
      return -1;   \
    FIO_NAME(array_name, seeker_s) s = FIO_NAME(array_name, seek)(a, &obj);   \
    if (s.pos >= a->w)   \
      return -1;   \
    a->ary[s.pos] = (array_type){0};   \
    FIO_NAME(array_name, imap)(a)[s.ipos] = (imap_type)(~(imap_type)0);   \
    --a->count;   \
    while (a->w && !is_valid_fn((a->ary + a->w - 1)))   \
      --a->w;   \
    return 0;   \
  }
```

This MACRO defines the type and functions needed for an indexed array.

This is used internally and documentation is poor.

An indexed array is simple ordered array who's objects are indexed using an
almost-hash map, allowing for easy seeking while also enjoying an array's
advantages.

The index map uses one `imap_type` (i.e., `uint64_t`) to store both the array
index and any leftover hash data (the first half being tested during the
random access and the leftover during comparison). The reserved value `0`
indicates a free slot. The reserved value `~0` indicates a freed item (a free
slot that was previously used).

- `array_name_s`        the main array container (.ary is the array itself)
- `array_name_seeker_s` is a seeker type that finds objects.
- `array_name_seek`     finds an object or its future position.

- `array_name_capa`     the imap's theoretical storage capacity.
- `array_name_set`      writes or overwrites data to the array.
- `array_name_get`      returns a pointer to the object within the array.
- `array_name_remove`   removes an object and resets its memory to zero.
- `array_name_reserve`  reserves a minimum imap storage capacity.
- `array_name_rehash`   re-builds the imap (use after sorting).
- `array_name_destroy`  de-allocates internal dynamic memory.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-002-math-h"></a> `./fio-stl/002 math.h`

6 public symbols.

### Functions

#### `fio_math_div`

```c
inline void fio_math_div(uint64_t *dest, uint64_t *reminder, const uint64_t *a, const uint64_t *b, const size_t number_array_length)
```

Multi-precision DIV for `len*64` bit long a, b.

This is NOT constant time.

The algorithm might be slow, as my math isn't that good and I couldn't
understand faster division algorithms (such as Newton–Raphson division)... so
this is sort of a factorized variation on long division.

_Symbol type:_ `function`

#### `fio_math_shr`

```c
inline void fio_math_shr(uint64_t *dest, uint64_t *n, const size_t right_shift_bits, size_t number_array_length)
```

Multi-precision shift right for `len` word number `n`.

_Symbol type:_ `function`

#### `fio_math_shl`

```c
inline void fio_math_shl(uint64_t *dest, uint64_t *n, const size_t left_shift_bits, const size_t number_array_length)
```

Multi-precision shift left for `len*64` bit number `n`.

_Symbol type:_ `function`

#### `fio_math_inv`

```c
inline void fio_math_inv(uint64_t *dest, uint64_t *n, size_t len)
```

Multi-precision Inverse for `len*64` bit number `n` (turn `1` into `-1`).

_Symbol type:_ `function`

#### `fio_math_msb_index`

```c
FIO_MIFN size_t fio_math_msb_index(uint64_t *n, const size_t len)
```

Multi-precision - returns the index for the most significant bit or -1.

_Symbol type:_ `function`

#### `fio_math_lsb_index`

```c
FIO_MIFN size_t fio_math_lsb_index(uint64_t *n, const size_t len)
```

Multi-precision - returns the index for the least significant bit or -1.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-002-random-h"></a> `./fio-stl/002 random.h`

15 public symbols.

### Macros

#### `FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH`

```c
#define FIO_USE_STABLE_HASH_WHEN_CALLING_RISKY_HASH 0
```



_Symbol type:_ `macro`

### Functions

#### `fio_rand64`

```c
uint64_t fio_rand64(void)
```

Returns 64 psedo-random bits. Probably not cryptographically safe.

_Symbol type:_ `function`

#### `fio_rand128`

```c
fio_u128 fio_rand128(void)
```

Returns 64 psedo-random bits. Probably not cryptographically safe.

_Symbol type:_ `function`

#### `fio_rand_bytes`

```c
void fio_rand_bytes(void *target, size_t len)
```

Writes `len` bytes of psedo-random bits to the target buffer.

_Symbol type:_ `function`

#### `fio_rand_bytes_secure`

```c
int fio_rand_bytes_secure(void *target, size_t len)
```

Writes `len` bytes of cryptographically secure random data to `target`.

Uses system CSPRNG: getrandom() on Linux, arc4random_buf() on BSD/macOS,
or /dev/urandom as fallback. Returns 0 on success, -1 on failure.

IMPORTANT: Use this for security-sensitive operations like key generation.

_Symbol type:_ `function`

#### `fio_rand_reseed`

```c
void fio_rand_reseed(void)
```

Reseeds the random engine using system state (rusage / jitter).

_Symbol type:_ `function`

#### `fio_risky_hash`

```c
uint64_t fio_risky_hash(const void *buf, size_t len, uint64_t seed)
```

Computes a facil.io Risky Hash (Risky v.3).

_Symbol type:_ `function`

#### `fio_risky_ptr`

```c
inline uint64_t fio_risky_ptr(void *ptr)
```

Adds a bit of entropy to pointer values. Designed to be unsafe.

_Symbol type:_ `function`

#### `fio_risky_num`

```c
inline uint64_t fio_risky_num(uint64_t number, uint64_t seed)
```

Adds a bit of entropy to numeral values. Designed to be unsafe.

_Symbol type:_ `function`

#### `fio_stable_hash`

```c
uint64_t fio_stable_hash(const void *data, size_t len, uint64_t seed)
```

Computes a facil.io Stable Hash (will not be updated, even if broken).

_Symbol type:_ `function`

#### `fio_stable_hash128`

```c
void fio_stable_hash128(void *restrict dest, const void *restrict data, size_t len, uint64_t seed)
```

Computes a facil.io Stable Hash (will not be updated, even if broken).

_Symbol type:_ `function`

#### `fio_risky256`

```c
fio_u256 fio_risky256(const void *data, uint64_t len)
```

Computes a 256-bit non-cryptographic hash (RiskyHash 256).

Based on the A3 (Zero-Copy ILP) design: 2-state multiply-fold with
cross-lane mixing, 128 bytes/iteration, zero-copy XOR from input.

Returns a `fio_u256` (32 bytes). Passes strict avalanche (50.0%),
collision, differential, and length independence tests.

_Symbol type:_ `function`

#### `fio_risky512`

```c
fio_u512 fio_risky512(const void *data, uint64_t len)
```

Computes a 512-bit non-cryptographic hash (RiskyHash 512).

SHAKE-style extension of fio_risky256: the first 256 bits of the 512-bit
output are identical to fio_risky256 (truncation-safe). The second 256 bits
come from an additional squeeze round.

Returns a `fio_u512` (64 bytes).

_Symbol type:_ `function`

#### `fio_risky256_hmac`

```c
fio_u256 fio_risky256_hmac(const void *key, uint64_t key_len, const void *msg, uint64_t msg_len)
```

Computes HMAC-RiskyHash-256 (RFC 2104 construction, 32-byte digest).

Uses fio_risky256 as the underlying hash with a 64-byte block size.
If `key_len > 64`, the key is hashed first with fio_risky256.

NOTE: The underlying hash is non-cryptographic, so standard HMAC
security proofs do not apply. For cryptographic HMAC, use blake2.

_Symbol type:_ `function`

#### `fio_risky512_hmac`

```c
fio_u512 fio_risky512_hmac(const void *key, uint64_t key_len, const void *msg, uint64_t msg_len)
```

Computes HMAC-RiskyHash-512 (RFC 2104 construction, 64-byte digest).

Uses fio_risky512 as the underlying hash with a 64-byte block size.
If `key_len > 64`, the key is hashed first with fio_risky512.

NOTE: The underlying hash is non-cryptographic, so standard HMAC
security proofs do not apply. For cryptographic HMAC, use blake2.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-002-signals-h"></a> `./fio-stl/002 signals.h`

8 public symbols.

### Macros

#### `FIO_SIGNAL_USER1`

```c
#define FIO_SIGNAL_USER1             SIGUSR1
```



_Symbol type:_ `macro`

#### `FIO_SIGNAL_USER2`

```c
#define FIO_SIGNAL_USER2             SIGUSR2
```



_Symbol type:_ `macro`

#### `FIO_SIGNAL_USER_UNREGISTERED`

```c
#define FIO_SIGNAL_USER_UNREGISTERED SIGUSR2
```



_Symbol type:_ `macro`

### Types

#### `fio_signal_monitor_args_s`

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
/** Call (safe) callback immediately? or wait for `fio_signal_review`? */
bool immediate;
} fio_signal_monitor_args_s
```



_Symbol type:_ `type`

### Functions

#### `fio_signal_monitor`

```c
int fio_signal_monitor(fio_signal_monitor_args_s args)
```

Starts to monitor for the specified signal, setting an optional callback.

_Symbol type:_ `function`

#### `fio_signal_monitor`

```c
#define fio_signal_monitor(...)   \
  fio_signal_monitor((fio_signal_monitor_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_signal_review`

```c
int fio_signal_review(void)
```

Reviews all signals, calling any relevant callbacks.

_Symbol type:_ `function`

#### `fio_signal_forget`

```c
int fio_signal_forget(int sig)
```

Stops monitoring the specified signal.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-002-sort-h"></a> `./fio-stl/002 sort.h`

3 public symbols.

### Macros

#### `FIO_SORT_THRESHOLD`

```c
#define FIO_SORT_THRESHOLD 96
```

The default threshold below which quicksort delegates to insert sort.

_Symbol type:_ `macro`

#### `FIO_SORT_SWAP`

```c
#define FIO_SORT_SWAP(a, b)   \
  do {   \
    FIO_SORT_TYPE tmp__ = (a);   \
    (a) = (b);   \
    (b) = tmp__;   \
  } while (0)
```

Default swap operation assumes an array and swaps array members

_Symbol type:_ `macro`

#### `FIO_SORT_IS_BIGGER`

```c
#define FIO_SORT_IS_BIGGER(a, b) ((a) > (b))
```

MUST evaluate as 1 if a > b (zero if equal or smaller).

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-002-threads-h"></a> `./fio-stl/002 threads.h`

31 public symbols.

### Macros

#### `FIO_THREAD_MUTEX_INIT`

```c
#define FIO_THREAD_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
```

Used this macro for static initialization.

_Symbol type:_ `macro`

#### `FIO_MEMCPY_THREADS`

```c
#define FIO_MEMCPY_THREADS 8
```



_Symbol type:_ `macro`

### Types

#### `fio_thread_t`

```c
typedef pthread_t fio_thread_t
```



_Symbol type:_ `type`

#### `fio_thread_pid_t`

```c
typedef pid_t fio_thread_pid_t
```



_Symbol type:_ `type`

#### `fio_thread_mutex_t`

```c
typedef pthread_mutex_t fio_thread_mutex_t
```



_Symbol type:_ `type`

#### `fio_thread_cond_t`

```c
typedef pthread_cond_t fio_thread_cond_t
```



_Symbol type:_ `type`

#### `fio_thread_priority_e`

```c
typedef enum {
FIO_THREAD_PRIORITY_ERROR = -1,
FIO_THREAD_PRIORITY_LOWEST = 0,
FIO_THREAD_PRIORITY_LOW,
FIO_THREAD_PRIORITY_NORMAL,
FIO_THREAD_PRIORITY_HIGH,
FIO_THREAD_PRIORITY_HIGHEST,
} fio_thread_priority_e
```

Possible thread priority values.

_Symbol type:_ `type`

### Functions

#### `fio_thread_fork`

```c
inline fio_thread_pid_t fio_thread_fork(void)
```

Should behave the same as the POSIX system call `fork`.

_Symbol type:_ `function`

#### `fio_thread_getpid`

```c
inline fio_thread_pid_t fio_thread_getpid(void)
```

Should behave the same as the POSIX system call `getpid`.

_Symbol type:_ `function`

#### `fio_thread_kill`

```c
inline int fio_thread_kill(fio_thread_pid_t pid, int sig)
```

Should behave the same as the POSIX system call `kill`.

_Symbol type:_ `function`

#### `fio_thread_waitpid`

```c
inline int fio_thread_waitpid(fio_thread_pid_t pid, int *stat_loc, int options)
```

Should behave the same as the POSIX system call `waitpid`.

_Symbol type:_ `function`

#### `fio_thread_create`

```c
inline int fio_thread_create(fio_thread_t *t, void *(*fn)(void *), void *arg)
```

Starts a new thread, returns 0 on success and -1 on failure.

_Symbol type:_ `function`

#### `fio_thread_join`

```c
inline int fio_thread_join(fio_thread_t *t)
```

Waits for the thread to finish.

_Symbol type:_ `function`

#### `fio_thread_detach`

```c
inline int fio_thread_detach(fio_thread_t *t)
```

Detaches the thread, so thread resources are freed automatically.

_Symbol type:_ `function`

#### `fio_thread_exit`

```c
inline void fio_thread_exit(void)
```

Ends the current running thread.

_Symbol type:_ `function`

#### `fio_thread_equal`

```c
inline int fio_thread_equal(fio_thread_t *a, fio_thread_t *b)
```



_Symbol type:_ `function`

#### `fio_thread_current`

```c
inline fio_thread_t fio_thread_current(void)
```

Returns the current thread.

_Symbol type:_ `function`

#### `fio_thread_yield`

```c
inline void fio_thread_yield(void)
```

Yields thread execution.

_Symbol type:_ `function`

#### `fio_thread_priority`

```c
FIO_SFUNC fio_thread_priority_e fio_thread_priority(void)
```

Returns a thread's priority level.

_Symbol type:_ `function`

#### `fio_thread_priority_set`

```c
FIO_SFUNC int fio_thread_priority_set(fio_thread_priority_e)
```

Sets a thread's priority level.

_Symbol type:_ `function`

#### `fio_thread_mutex_init`

```c
inline int fio_thread_mutex_init(fio_thread_mutex_t *m)
```

Initializes a simple Mutex.

Or use the static initialization value: FIO_THREAD_MUTEX_INIT

_Symbol type:_ `function`

#### `fio_thread_mutex_lock`

```c
inline int fio_thread_mutex_lock(fio_thread_mutex_t *m)
```

Locks a simple Mutex, returning -1 on error.

_Symbol type:_ `function`

#### `fio_thread_mutex_trylock`

```c
inline int fio_thread_mutex_trylock(fio_thread_mutex_t *m)
```

Attempts to lock a simple Mutex, returning zero on success.

_Symbol type:_ `function`

#### `fio_thread_mutex_unlock`

```c
inline int fio_thread_mutex_unlock(fio_thread_mutex_t *m)
```

Unlocks a simple Mutex, returning zero on success or -1 on error.

_Symbol type:_ `function`

#### `fio_thread_mutex_destroy`

```c
inline void fio_thread_mutex_destroy(fio_thread_mutex_t *m)
```

Destroys the simple Mutex (cleanup).

_Symbol type:_ `function`

#### `fio_thread_cond_init`

```c
inline int fio_thread_cond_init(fio_thread_cond_t *c)
```

Initializes a simple conditional variable.

_Symbol type:_ `function`

#### `fio_thread_cond_wait`

```c
inline int fio_thread_cond_wait(fio_thread_cond_t *c, fio_thread_mutex_t *m)
```

Waits on a conditional variable (MUST be previously locked).

_Symbol type:_ `function`

#### `fio_thread_cond_timedwait`

```c
inline int fio_thread_cond_timedwait(fio_thread_cond_t *c, fio_thread_mutex_t *m, size_t milliseconds)
```

Waits on a conditional variable (MUST be previously locked).

_Symbol type:_ `function`

#### `fio_thread_cond_signal`

```c
inline int fio_thread_cond_signal(fio_thread_cond_t *c)
```

Signals a simple conditional variable.

_Symbol type:_ `function`

#### `fio_thread_cond_destroy`

```c
inline void fio_thread_cond_destroy(fio_thread_cond_t *c)
```

Destroys a simple conditional variable.

_Symbol type:_ `function`

#### `fio_thread_memcpy`

```c
FIO_SFUNC size_t fio_thread_memcpy(void *restrict dest, const void *restrict src, size_t bytes)
```

Multi-threaded memcpy using up to FIO_MEMCPY_THREADS threads

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-002-url-h"></a> `./fio-stl/002 url.h`

7 public symbols.

### Macros

#### `FIO_URL_QUERY_EACH`

```c
#define FIO_URL_QUERY_EACH(query_buf, i)   \
  for (fio_url_query_each_s i = fio_url_query_each_next(   \
           (fio_url_query_each_s){.private___ = (query_buf)});   \
       i.name.buf;   \
       i = fio_url_query_each_next(i))
```

Iterates through each of the query elements.

_Symbol type:_ `macro`

### Types

#### `fio_url_s`

```c
typedef struct {
fio_buf_info_s scheme;
fio_buf_info_s user;
fio_buf_info_s password;
fio_buf_info_s host;
fio_buf_info_s port;
fio_buf_info_s path;
fio_buf_info_s query;
fio_buf_info_s target;
} fio_url_s
```

the result returned by `fio_url_parse`

_Symbol type:_ `type`

#### `fio_url_query_each_s`

```c
typedef struct {
fio_buf_info_s name;
fio_buf_info_s value;
fio_buf_info_s private___;
} fio_url_query_each_s
```

The type used by the `FIO_URL_QUERY_EACH` iterator macro.

_Symbol type:_ `type`

#### `fio_url_tls_info_s`

```c
typedef struct {
fio_buf_info_s key;
fio_buf_info_s cert;
fio_buf_info_s pass;
bool tls;
} fio_url_tls_info_s
```



_Symbol type:_ `type`

### Functions

#### `fio_url_parse`

```c
fio_url_s fio_url_parse(const char *url, size_t len)
```

Parses the URI returning it's components and their lengths (no decoding
performed, doesn't accept decoded URIs).

The returned string are NOT NUL terminated, they are merely locations within
the original string.

This function attempts to accept many different formats, including any of the
following:

* `/complete_path?query#target`

  i.e.: /index.html?page=1#list

* `host:port/complete_path?query#target`

  i.e.:
     example.com
     example.com:8080
     example.com/index.html
     example.com:8080/index.html
     example.com:8080/index.html?key=val#target

* `user:password@host:port/path?query#target`

  i.e.: user:1234@example.com:8080/index.html

* `username[:password]@host[:port][...]`

  i.e.: john:1234@example.com

* `schema://user:password@host:port/path?query#target`

  i.e.: http://example.com/index.html?page=1#list

For performance reasons, no format validation is performed. Function assumes
that the `url` string is `len` long and contains a valid URL. Invalid formats
might produce unexpected results.

NOTE: the `unix`, `file` and `priv` schemas are reserved for file paths.

_Symbol type:_ `function`

#### `fio_url_query_each_next`

```c
FIO_SFUNC fio_url_query_each_s fio_url_query_each_next(fio_url_query_each_s)
```

A helper function for the `FIO_URL_QUERY_EACH` macro implementation.

_Symbol type:_ `function`

#### `fio_url_is_tls`

```c
fio_url_tls_info_s fio_url_is_tls(fio_url_s u)
```

Returns TLS data associated with the URL.

This function supports implicit TLS by scheme data for the following possible
values:

- `wss`   - Secure WebSockets.
- `sses`  - Secure SSE (Server Sent Events).
- `https` - Secure HTTP.
- `tcps`  - Secure TCP/IP.
- `tls`   - Secure TCP/IP.
- `udps`  - Secure UDP.

i.e.:
    tls://example.com/
    tcps://example.com/
    udps://example.com/

    wss://example.com/
    https://example.com/
    sses://example.com/

This function also supports explicit TLS by query data for the following
possible key-pair values:

- `tls`                   - self-signed TLS (unless key / cert are provided).
- `tls=true`              - self-signed TLS (unless key / cert are provided).
- `tls=<file>`            - key and certificate files (same path, different
                            file extensions).
- `key=<file/env_data>`   - path or env variable name for the private key.
- `cert=<file/env_data>`  - path or env variable name for the public
                            certificate.

- `pass`                  - password for decrypting key / cert data.

i.e.:

    tcp://example.com/?tls          (anonymous TLS)
    udp://example.com/?tls=true

    https://example.com/?tls=key_cert_folder_or_prefix&pass=key_password

    https://example.com/?key=key_file_or_env_var&cert=cert_file_or_env_var&pass=key_password
    wss://example.com/?key=key_file_or_env_var&cert=cert_file_or_env_var&pass=key_password
    tcp://example.com/?key=key_file_or_env_var&cert=cert_file_or_env_var&pass=key_password

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-003-entities-h"></a> `./fio-stl/003 entities.h`

1 public symbols.

### Functions

#### `fio_entity`

```c
size_t fio_entity(char *dest, const char *src, size_t len)
```

Decodes a single ML entity from `src` (length `len`, starting at '&').

Writes decoded UTF-8 bytes to `dest`, which MUST be at least 8 bytes.
A NUL terminator is written only if the decoded result is shorter than
8 bytes.

Returns the number of bytes written to `dest` (excluding NUL), or 0 if
`src` does not start with a valid entity.

Supported forms:
  - Named:   `&name;`   (case-insensitive, ~40 common entities)
  - Decimal: `&#digits;`
  - Hex:     `&#xhex;` or `&#Xhex;`

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-004-files-h"></a> `./fio-stl/004 files.h`

21 public symbols.

### Macros

#### `FIO_FD_FIND_EOF`

```c
#define FIO_FD_FIND_EOF ((size_t)-1)
```

End of file value for `fio_fd_find_next`

_Symbol type:_ `macro`

#### `FIO_FD_FIND_BLOCK`

```c
#define FIO_FD_FIND_BLOCK 4096
```

Size on the stack used by `fio_fd_find_next` for each read cycle.

_Symbol type:_ `macro`

#### `FIO_FOLDER_SEPARATOR`

```c
#define FIO_FOLDER_SEPARATOR '\\'
```



_Symbol type:_ `macro`

### Types

#### `fio_filename_s`

```c
typedef struct {
fio_buf_info_s folder;
fio_buf_info_s basename;
fio_buf_info_s ext;
} fio_filename_s
```

A result type for the filename parsing helper.

_Symbol type:_ `type`

### Functions

#### `fio_filename_open`

```c
int fio_filename_open(const char *filename, int flags)
```

Opens `filename`, returning the same as values as `open` on POSIX systems.

If `path` starts with a `"~/"` than it will be relative to the user's home
folder (on Windows, testing for `"~\"`).

_Symbol type:_ `function`

#### `fio_filename_is_unsafe`

```c
int fio_filename_is_unsafe(const char *path)
```

Returns 1 if `path` does folds backwards (OS separator dependent).

_Symbol type:_ `function`

#### `fio_filename_is_unsafe_url`

```c
int fio_filename_is_unsafe_url(const char *path)
```

Returns 1 if `path` does folds backwards (has "/../" or "//").

_Symbol type:_ `function`

#### `fio_filename_tmp`

```c
int fio_filename_tmp(void)
```

Creates a temporary file, returning its file descriptor.

_Symbol type:_ `function`

#### `fio_filename_overwrite`

```c
inline int fio_filename_overwrite(const char *filename, const void *buf, size_t len)
```

Overwrites `filename` with the data in the buffer.

If `path` starts with a `"~/"` than it will be relative to the user's home
folder (on Windows, testing for `"~\"`).

Returns -1 on error or 0 on success. On error, the state of the file is
undefined (may be doesn't exit / nothing written / partially written).

_Symbol type:_ `function`

#### `fio_filename_size`

```c
inline size_t fio_filename_size(const char *filename)
```

Returns the file size (or 0 on both error / empty file).

_Symbol type:_ `function`

#### `fio_fd_size`

```c
inline size_t fio_fd_size(int fd)
```

Returns the file size (or 0 on both error / empty file).

_Symbol type:_ `function`

#### `fio_filename_type`

```c
inline size_t fio_filename_type(const char *filename)
```

Returns the file type (or 0 on both error).

See: https://www.man7.org/linux/man-pages/man7/inode.7.html

_Symbol type:_ `function`

#### `fio_filename_stat`

```c
inline int fio_filename_stat(const char *filename, struct stat *stat_buf)
```

Populates `stat_buf` with the file's metadata. Returns 0 on success.

_Symbol type:_ `function`

#### `fio_fd_type`

```c
inline size_t fio_fd_type(int fd)
```

Returns the file type (or 0 on both error).

See: https://www.man7.org/linux/man-pages/man7/inode.7.html

_Symbol type:_ `function`

#### `fio_filename_is_folder`

```c
#define fio_filename_is_folder(filename)   \
  (fio_filename_type((filename)) == S_IFDIR)
```

Tests if `filename` references a folder. Returns -1 on error.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_fd_write`

```c
inline ssize_t fio_fd_write(int fd, const void *buf, size_t len)
```

Writes data to a file handle, returning the number of bytes written.

Returns -1 on error.

Since some systems have a limit on the number of bytes that can be written at
a time, this function fragments the system calls into smaller `write` blocks,
allowing large data to be written.

If the file descriptor is non-blocking, test errno for EAGAIN / EWOULDBLOCK.

_Symbol type:_ `function`

#### `fio_fd_read`

```c
inline size_t fio_fd_read(int fd, void *buf, size_t len, off_t start_at)
```

Reads up to `len` bytes from `fd`, returning the number of bytes read.

Returns 0 if no bytes were read or on error.

Since some systems have a limit on the number of bytes that can be read at
a time, this function fragments the system calls into smaller `read` blocks,
allowing large data to be read.

If the file descriptor is non-blocking, test errno for EAGAIN / EWOULDBLOCK.

_Symbol type:_ `function`

#### `fio_filename_parse`

```c
fio_filename_s fio_filename_parse(const char *filename)
```

Parses a file name to folder, base name and extension (zero-copy).

_Symbol type:_ `function`

#### `fio_filename_parse2`

```c
fio_filename_s fio_filename_parse2(const char *filename, size_t len)
```

Parses a file name to folder, base name and extension (zero-copy).

_Symbol type:_ `function`

#### `fio_fd_find_next`

```c
size_t fio_fd_find_next(int fd, char token, size_t start_at)
```

Returns offset for the next `token` in `fd`, or -1 if reached  EOF.

This will use `FIO_FD_FIND_BLOCK` bytes on the stack to read the file in a
loop.

Pros: limits memory use and (re)allocations, easier overflow protection.

Cons: may be slower, as data will most likely be copied again from the file.

_Symbol type:_ `function`

#### `fio_file_dup`

```c
#define fio_file_dup(fd) _dup(fd)
```

Duplicates the file handle (int)

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-004-json-h"></a> `./fio-stl/004 json.h`

5 public symbols.

### Macros

#### `FIO_JSON_MAX_DEPTH`

```c
#define FIO_JSON_MAX_DEPTH 128
```

Maximum allowed JSON nesting level. MUST be less then 65536

Values above 65536 might cause the stack to overflow and cause a failure.

_Symbol type:_ `macro`

#### `FIO_JSON_USE_FIO_ATON`

```c
#define FIO_JSON_USE_FIO_ATON 0
```



_Symbol type:_ `macro`

### Types

#### `fio_json_parser_callbacks_s`

```c
typedef struct {
/** NULL object was detected. Returns new object as `void *`. */
void *(*on_null)(void *udata);
/** TRUE object was detected. Returns new object as `void *`. */
void *(*on_true)(void *udata);
/** FALSE object was detected. Returns new object as `void *`. */
void *(*on_false)(void *udata);
/** Number was detected (long long). Returns new object as `void *`. */
void *(*on_number)(void *udata, int64_t i);
/** Float was detected (double).Returns new object as `void *`. */
void *(*on_float)(void *udata, double f);
/** (escaped) String was detected. Returns a new String as `void *`. */
void *(*on_string)(void *udata, const void *start, size_t len);
/** (unescaped) String was detected. Returns a new String as `void *`. */
void *(*on_string_simple)(void *udata, const void *start, size_t len);
/** Dictionary was detected. Returns ctx to hash map or NULL on error. */
void *(*on_map)(void *udata, void *ctx, void *at);
/** Array was detected. Returns ctx to array or NULL on error. */
void *(*on_array)(void *udata, void *ctx, void *at);
/** Map entry detected. Returns non-zero on error. Owns key and value. */
int (*map_push)(void *udata, void *ctx, void *key, void *value);
/** Array entry detected. Returns non-zero on error. Owns value. */
int (*array_push)(void *udata, void *ctx, void *value);
/** Called when an array object (`ctx`) appears done. */
int (*array_finished)(void *udata, void *ctx);
/** Called when a map object (`ctx`) appears done. */
int (*map_finished)(void *udata, void *ctx);
/** Called when context is expected to be an array (i.e., fio_json_update). */
int (*is_array)(void *udata, void *ctx);
/** Called when context is expected to be a map (i.e., fio_json_update). */
int (*is_map)(void *udata, void *ctx);
/** Called for unused objects (e.g., key on error). Must free the object. */
void (*free_unused_object)(void *udata, void *ctx);
/** The JSON parsing encountered an error. Owns ctx, should free or return. */
void *(*on_error)(void *udata, void *ctx);
} fio_json_parser_callbacks_s
```

The JSON parser settings (callbacks).

**Ownership**: Callbacks that return `void *` objects transfer ownership to
the parser. The parser will either pass these objects to `map_push` /
`array_push` (transferring ownership to the container), or call
`free_unused_object` if the object is not used (e.g., on error or NULL map
key). The `on_error` callback receives ownership of any partial result.

_Symbol type:_ `type`

#### `fio_json_result_s`

```c
typedef struct {
void *ctx;
size_t stop_pos;
int err;
} fio_json_result_s
```

The JSON return type.

_Symbol type:_ `type`

### Functions

#### `fio_json_parse`

```c
fio_json_result_s fio_json_parse(fio_json_parser_callbacks_s *settings, void *udata, const char *json_string, const size_t len)
```

The facil.io JSON parser is a non-strict parser, with support for trailing
commas in collections, new-lines in strings, extended escape characters and
octal, hex and binary numbers.

The parser allows for streaming data and decouples the parsing process from
the resulting data-structure by calling static callbacks for JSON related
events.

Returns the number of bytes consumed before parsing stopped (due to either
error or end of data). Stops as close as possible to the end of the buffer or
once an object parsing was completed.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-004-multipart-h"></a> `./fio-stl/004 multipart.h`

3 public symbols.

### Types

#### `fio_multipart_parser_callbacks_s`

```c
typedef struct {
/**
* Called for each regular form field (no filename).
* Returns user-defined context (can be NULL).
*
* Note: If on_field_start is provided, this callback is ignored and the
* streaming field callbacks (on_field_start/on_field_data/on_field_end)
* are used instead.
*/
void *(*on_field)(void *udata,
fio_buf_info_s name,
fio_buf_info_s value,
fio_buf_info_s content_type);
/**
* Called when a large field starts (optional).
* If NULL but on_field exists, on_field is used instead (backward
* compatible). Returns context for this field (passed to
* on_field_data/on_field_end).
*/
void *(*on_field_start)(void *udata,
fio_buf_info_s name,
fio_buf_info_s content_type);
/**
* Called with field data chunk.
* May be called multiple times per field for streaming.
* Returns non-zero to abort parsing.
*/
int (*on_field_data)(void *udata, void *field_ctx, fio_buf_info_s data);
/**
* Called when field ends.
*/
void (*on_field_end)(void *udata, void *field_ctx);
/**
* Called when a file upload starts.
* Returns context for this file (passed to on_file_data/on_file_end).
*/
void *(*on_file_start)(void *udata,
fio_buf_info_s name,
fio_buf_info_s filename,
fio_buf_info_s content_type);
/**
* Called with file data chunk.
* May be called multiple times per file for streaming.
* Returns non-zero to abort parsing.
*/
int (*on_file_data)(void *udata, void *file_ctx, fio_buf_info_s data);
/**
* Called when file upload ends.
*/
void (*on_file_end)(void *udata, void *file_ctx);
/**
* Called on parse error (optional).
*/
void (*on_error)(void *udata);
} fio_multipart_parser_callbacks_s
```

The MIME multipart parser callbacks.

_Symbol type:_ `type`

#### `fio_multipart_result_s`

```c
typedef struct {
/** Number of bytes consumed from the input buffer. */
size_t consumed;
/** Number of form fields parsed. */
size_t field_count;
/** Number of files parsed. */
size_t file_count;
/** Error code: 0 = success, -1 = error, -2 = need more data. */
int err;
} fio_multipart_result_s
```

The MIME multipart parse result type.

_Symbol type:_ `type`

### Functions

#### `fio_multipart_parse`

```c
fio_multipart_result_s fio_multipart_parse(const fio_multipart_parser_callbacks_s *callbacks, void *udata, fio_buf_info_s boundary, const char *data, size_t len)
```

Parse MIME multipart data.

`callbacks` contains the callback functions (should be static const).
`udata` is user data passed to all callbacks.
`boundary` is the multipart boundary string (without leading "--").
`data` is the data to parse.
`len` is the length of the data.

Returns a result struct containing:
- `consumed`: Number of bytes consumed from the buffer
- `field_count`: Number of form fields parsed
- `file_count`: Number of files parsed
- `err`: 0 = success, -1 = error, -2 = need more data

For streaming, call again with remaining data appended to unconsumed data.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-004-resp3-h"></a> `./fio-stl/004 resp3.h`

23 public symbols.

### Macros

#### `FIO_RESP3_MAX_NESTING`

```c
#define FIO_RESP3_MAX_NESTING 32
```



_Symbol type:_ `macro`

#### `FIO_RESP3_SIMPLE_STR`

```c
#define FIO_RESP3_SIMPLE_STR '+'
```

Simple String: `+<string>\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_SIMPLE_ERR`

```c
#define FIO_RESP3_SIMPLE_ERR '-'
```

Simple Error: `-<string>\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_NUMBER`

```c
#define FIO_RESP3_NUMBER ':'
```

Number: `:<number>\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_NULL`

```c
#define FIO_RESP3_NULL '_'
```

Null: `_\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_DOUBLE`

```c
#define FIO_RESP3_DOUBLE ','
```

Double: `,<floating-point-number>\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_BOOL`

```c
#define FIO_RESP3_BOOL '#'
```

Boolean: `#t\r\n` or `#f\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_BIGNUM`

```c
#define FIO_RESP3_BIGNUM '('
```

Big Number: `(<big number>\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_BLOB_STR`

```c
#define FIO_RESP3_BLOB_STR '$'
```

Blob String: `$<length>\r\n<bytes>\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_BLOB_ERR`

```c
#define FIO_RESP3_BLOB_ERR '!'
```

Blob Error: `!<length>\r\n<bytes>\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_VERBATIM`

```c
#define FIO_RESP3_VERBATIM '='
```

Verbatim String: `=<length>\r\n<type:><bytes>\r\n`

_Symbol type:_ `macro`

#### `FIO_RESP3_ARRAY`

```c
#define FIO_RESP3_ARRAY '*'
```

Array: `*<count>\r\n...elements...`

_Symbol type:_ `macro`

#### `FIO_RESP3_MAP`

```c
#define FIO_RESP3_MAP '%'
```

Map: `%<count>\r\n...key-value pairs...`

_Symbol type:_ `macro`

#### `FIO_RESP3_SET`

```c
#define FIO_RESP3_SET '~'
```

Set: `~<count>\r\n...elements...`

_Symbol type:_ `macro`

#### `FIO_RESP3_PUSH`

```c
#define FIO_RESP3_PUSH '>'
```

Push: `><count>\r\n...elements...`

_Symbol type:_ `macro`

#### `FIO_RESP3_ATTR`

```c
#define FIO_RESP3_ATTR '|'
```

Attribute: `|<count>\r\n...key-value pairs...`

_Symbol type:_ `macro`

#### `FIO_RESP3_STREAM_CHUNK`

```c
#define FIO_RESP3_STREAM_CHUNK ';'
```

Streamed string chunk: `;`

_Symbol type:_ `macro`

#### `FIO_RESP3_STREAM_END`

```c
#define FIO_RESP3_STREAM_END '.'
```

Streamed aggregate end: `.`

_Symbol type:_ `macro`

### Types

#### `fio_resp3_frame_s`

```c
typedef struct {
/** Context for this container (returned by on_array/on_map/etc) */
void *ctx;
/** For maps: the pending key waiting for its value */
void *key;
/** Expected remaining elements */
int64_t remaining;
/** Type of this frame */
uint8_t type;
/** Is this a streaming type? */
uint8_t streaming;
/** For maps: are we expecting a key (0) or value (1)? */
uint8_t expecting_value;
/** Set treated as map: need to duplicate values as key+value */
uint8_t set_as_map;
} fio_resp3_frame_s
```

Parser frame for tracking nested structures

_Symbol type:_ `type`

#### `fio_resp3_parser_s`

```c
typedef struct {
/** User data passed to all callbacks */
void *udata;
/** Current nesting depth */
uint32_t depth;
/** Protocol error flag */
uint8_t error;
/** Streaming string in progress flag */
uint8_t streaming_string;
/** Streaming string type (FIO_RESP3_BLOB_STR, FIO_RESP3_BLOB_ERR, etc.) */
uint8_t streaming_string_type;
/** Reserved */
uint8_t reserved[1];
/** Context for streaming string (from on_start_string) */
void *streaming_string_ctx;
/** Stack for nested structures */
fio_resp3_frame_s stack[FIO_RESP3_MAX_NESTING];
} fio_resp3_parser_s
```

RESP3 parser state

_Symbol type:_ `type`

#### `fio_resp3_callbacks_s`

```c
typedef struct {
/* ===== Primitive Callbacks - return the created object ===== */
/** Called when NULL (`_`) is received. Returns new object. */
void *(*on_null)(void *udata);
/** Called when Boolean (`#t` or `#f`) is received. Returns new object. */
void *(*on_bool)(void *udata, int is_true);
/** Called when a Number (`:`) is parsed. Returns new object. */
void *(*on_number)(void *udata, int64_t num);
/** Called when a Double (`,`) is parsed. Returns new object. */
void *(*on_double)(void *udata, double num);
/** Called when a Big Number (`(`) is parsed. Returns new object. */
void *(*on_bignum)(void *udata, const void *data, size_t len);
/**
* Called when a complete String is received.
* `type` is FIO_RESP3_SIMPLE_STR, FIO_RESP3_BLOB_STR, or FIO_RESP3_VERBATIM.
* Returns new object.
*/
void *(*on_string)(void *udata, const void *data, size_t len, uint8_t type);
/**
* Called when an error message is received (simple `-` or blob `!`).
* `type` is FIO_RESP3_SIMPLE_ERR or FIO_RESP3_BLOB_ERR.
* Returns new object.
*/
void *(*on_error)(void *udata, const void *data, size_t len, uint8_t type);
/* ===== Container Callbacks - receive parent ctx, return new ctx ===== */
/** Called when an Array starts. Returns new array context. */
void *(*on_array)(void *udata, void *parent_ctx, int64_t len);
/** Called when a Map starts. Returns new map context. */
void *(*on_map)(void *udata, void *parent_ctx, int64_t len);
/** Called when a Set starts. Returns new set context. */
void *(*on_set)(void *udata, void *parent_ctx, int64_t len);
/** Called when a Push message starts. Returns new push context. */
void *(*on_push)(void *udata, void *parent_ctx, int64_t len);
/** Called when an Attribute starts. Returns new attribute context. */
void *(*on_attr)(void *udata, void *parent_ctx, int64_t len);
/* ===== Push Callbacks - add child to container ===== */
/** Add value to array. Returns non-zero on error. */
int (*array_push)(void *udata, void *ctx, void *value);
/** Add key-value pair to map. Returns non-zero on error. */
int (*map_push)(void *udata, void *ctx, void *key, void *value);
/** Add value to set. Returns non-zero on error. */
int (*set_push)(void *udata, void *ctx, void *value);
/** Add value to push message. Returns non-zero on error. */
int (*push_push)(void *udata, void *ctx, void *value);
/** Add key-value pair to attribute. Returns non-zero on error. */
int (*attr_push)(void *udata, void *ctx, void *key, void *value);
/* ===== Done Callbacks (optional) - finalize container ===== */
/** Called when array is complete. Returns final object. */
void *(*array_done)(void *udata, void *ctx);
/** Called when map is complete. Returns final object. */
void *(*map_done)(void *udata, void *ctx);
/** Called when set is complete. Returns final object. */
void *(*set_done)(void *udata, void *ctx);
/** Called when push is complete. Returns final object. */
void *(*push_done)(void *udata, void *ctx);
/** Called when attribute is complete. Returns final object. */
void *(*attr_done)(void *udata, void *ctx);
/* ===== Error Handling ===== */
/** Free an unused object (e.g., orphaned key on error). */
void (*free_unused)(void *udata, void *obj);
/** Called on protocol error. */
void *(*on_error_protocol)(void *udata);
/* ===== Streaming String Callbacks (optional) ===== */
/**
* Called when a blob string starts (before data arrives).
* `len` is the declared length of the string ((size_t)-1 for streaming).
* `type` is FIO_RESP3_BLOB_STR, FIO_RESP3_BLOB_ERR, or FIO_RESP3_VERBATIM.
* Returns a context for the string being built (e.g., a string buffer).
* If NULL is returned, falls back to buffering and calling on_string when
* complete.
*/
void *(*on_start_string)(void *udata, size_t len, uint8_t type);
/**
* Called with partial string data (may be called multiple times).
* `ctx` is the context returned by on_start_string.
* Returns 0 on success, non-zero to abort parsing.
*/
int (*on_string_write)(void *udata, void *ctx, const void *data, size_t len);
/**
* Called when the string is complete.
* `ctx` is the context returned by on_start_string.
* Returns the final string object to be used as a value.
*/
void *(*on_string_done)(void *udata, void *ctx, uint8_t type);
} fio_resp3_callbacks_s
```

The RESP3 parser callbacks (designed to be static const).

All callbacks receive `udata` from the parser state as their first argument.

_Symbol type:_ `type`

#### `fio_resp3_result_s`

```c
typedef struct {
/** The parsed top-level object (or NULL on error/incomplete) */
void *obj;
/** Number of bytes consumed from the buffer */
size_t consumed;
/** Non-zero if an error occurred */
int err;
} fio_resp3_result_s
```

The RESP3 parse result type.

_Symbol type:_ `type`

### Functions

#### `fio_resp3_parse`

```c
fio_resp3_result_s fio_resp3_parse(fio_resp3_parser_s *parser, const fio_resp3_callbacks_s *callbacks, const void *buf, size_t len)
```

Parse RESP3 data from buffer.

`parser` is the parser state. Initialize with `{.udata = my_data}` for first
         call. For continuation after partial parse, pass the same parser.
`callbacks` contains the callback functions (should be static const).
`buf` is the data to parse.
`len` is the length of the data.

Returns a result struct containing:
- `obj`: The parsed top-level object (NULL if incomplete or error)
- `consumed`: Number of bytes consumed from the buffer
- `err`: Non-zero if a protocol error occurred

For partial data, the parser state is preserved. Call again with remaining
data appended to unconsumed data.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-004-sock-h"></a> `./fio-stl/004 sock.h`

35 public symbols.

### Macros

#### `FIO_SOCKET_INVALID`

```c
#define FIO_SOCKET_INVALID INVALID_SOCKET
```

Sentinel value for an invalid socket handle.

_Symbol type:_ `macro`

#### `FIO_SOCK_FD_ISVALID`

```c
#define FIO_SOCK_FD_ISVALID(fd) ((fio_socket_i)(fd) != FIO_SOCKET_INVALID)
```



_Symbol type:_ `macro`

#### `FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT`

```c
#define FIO_SOCK_DEFAULT_MAXIMIZE_LIMIT (1ULL << 24)
```



_Symbol type:_ `macro`

#### `FIO_SOCK_UNIX`

```c
#define FIO_SOCK_UNIX         0
```



_Symbol type:_ `macro`

#### `FIO_SOCK_UNIX_PRIVATE`

```c
#define FIO_SOCK_UNIX_PRIVATE 0
```



_Symbol type:_ `macro`

#### `FIO_SOCK_WAIT_RW`

```c
#define FIO_SOCK_WAIT_RW(fd, timeout_)   \
  fio_sock_wait_io(fd, POLLIN | POLLOUT, timeout_)
```

A helper macro that waits on a single IO with no callbacks (0 = no event)

_Symbol type:_ `macro`

#### `FIO_SOCK_WAIT_R`

```c
#define FIO_SOCK_WAIT_R(fd, timeout_) fio_sock_wait_io(fd, POLLIN, timeout_)
```

A helper macro that waits on a single IO with no callbacks (0 = no event)

_Symbol type:_ `macro`

#### `FIO_SOCK_WAIT_W`

```c
#define FIO_SOCK_WAIT_W(fd, timeout_) fio_sock_wait_io(fd, POLLOUT, timeout_)
```

A helper macro that waits on a single IO with no callbacks (0 = no event)

_Symbol type:_ `macro`

#### `FIO_SOCK_IS_OPEN`

```c
#define FIO_SOCK_IS_OPEN(fd)   \
  (!(fio_sock_wait_io(fd, (POLLOUT | POLLRDHUP), 0) &   \
     (POLLRDHUP | POLLHUP | POLLNVAL)))
```

A helper macro that tests if a socket was closed.

_Symbol type:_ `macro`

### Types

#### `fio_socket_i`

```c
typedef int fio_socket_i
```

Native socket handle type: int on POSIX.

_Symbol type:_ `type`

#### `fio_sock_open_flags_e`

```c
typedef enum {
FIO_SOCK_SERVER = 0,
FIO_SOCK_CLIENT = 1,
FIO_SOCK_NONBLOCK = 2,
FIO_SOCK_TCP = 4,
FIO_SOCK_UDP = 8,
#ifdef AF_UNIX
FIO_SOCK_UNIX = 16,
FIO_SOCK_UNIX_PRIVATE = (16 | 32),
#else
#define FIO_SOCK_UNIX 0
#define FIO_SOCK_UNIX_PRIVATE 0
#endif
} fio_sock_open_flags_e
```

Socket type flags

_Symbol type:_ `type`

### Functions

#### `fio_sock_write`

```c
inline ssize_t fio_sock_write(fio_socket_i fd, const void *buf, size_t len)
```

Acts as POSIX write. Use this function for portability with WinSock2.

_Symbol type:_ `function`

#### `fio_sock_read`

```c
inline ssize_t fio_sock_read(fio_socket_i fd, void *buf, size_t len)
```

Acts as POSIX read. Use this function for portability with WinSock2.

_Symbol type:_ `function`

#### `fio_sock_sendto`

```c
inline ssize_t fio_sock_sendto(fio_socket_i fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen)
```

Acts as POSIX sendto. Use this function for portability with WinSock2.

_Symbol type:_ `function`

#### `fio_sock_recvfrom`

```c
inline ssize_t fio_sock_recvfrom(fio_socket_i fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen)
```

Acts as POSIX recvfrom. Use this function for portability with WinSock2.

_Symbol type:_ `function`

#### `fio_sock_dup`

```c
inline fio_socket_i fio_sock_dup(fio_socket_i fd)
```

Acts as POSIX dup. Sets O_CLOEXEC on the new fd.

_Symbol type:_ `function`

#### `fio_sock_close`

```c
inline int fio_sock_close(fio_socket_i fd)
```

Acts as POSIX close. Use this function for portability with WinSock2.

_Symbol type:_ `function`

#### `fio_sock_accept`

```c
#define fio_sock_accept(fd, addr, addrlen) accept(fd, addr, addrlen)
```

Acts as POSIX accept. Use this macro for portability with WinSock2.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_sock_bind`

```c
inline int fio_sock_bind(fio_socket_i fd, const struct sockaddr *addr, socklen_t addrlen)
```

Portable bind. POSIX version.

_Symbol type:_ `function`

#### `fio_sock_connect`

```c
inline int fio_sock_connect(fio_socket_i fd, const struct sockaddr *addr, socklen_t addrlen)
```

Portable connect. POSIX version.

_Symbol type:_ `function`

#### `fio_sock_listen`

```c
inline int fio_sock_listen(fio_socket_i fd, int backlog)
```

Portable listen. POSIX version.

_Symbol type:_ `function`

#### `fio_sock_setsockopt`

```c
inline int fio_sock_setsockopt(fio_socket_i fd, int level, int optname, const void *optval, socklen_t optlen)
```

Portable setsockopt. POSIX version.

_Symbol type:_ `function`

#### `fio_sock_socketpair`

```c
inline int fio_sock_socketpair(fio_socket_i fds[2])
```

Creates a connected socket pair using POSIX socketpair().

_Symbol type:_ `function`

#### `fio_sock_pipe`

```c
inline int fio_sock_pipe(fio_socket_i fds[2])
```

Creates a POSIX pipe. fds[0] = read end, fds[1] = write end.
On Windows, fio_sock_pipe() is defined in the FIO_OS_WIN block above and
delegates to fio_sock_socketpair() (loopback TCP).

_Symbol type:_ `function`

#### `fio_sock_open`

```c
inline fio_socket_i fio_sock_open(const char *restrict address, const char *restrict port, uint16_t flags)
```

Creates a new socket according to the provided flags.

The `port` string will be ignored when `FIO_SOCK_UNIX` is set.

_Symbol type:_ `function`

#### `fio_sock_open2`

```c
fio_socket_i fio_sock_open2(const char *url, uint16_t flags)
```

Creates a new socket, according to the provided flags.

_Symbol type:_ `function`

#### `fio_sock_address_new`

```c
inline struct addrinfo *fio_sock_address_new(const char *restrict address, const char *restrict port, int sock_type)
```

Attempts to resolve an address to a valid IP6 / IP4 address pointer.

The `sock_type` element should be a socket type, such as `SOCK_DGRAM` (UDP)
or `SOCK_STREAM` (TCP/IP).

The address should be freed using `fio_sock_address_free`.

_Symbol type:_ `function`

#### `fio_sock_address_free`

```c
inline void fio_sock_address_free(struct addrinfo *a)
```

Frees the pointer returned by `fio_sock_address_new`.

_Symbol type:_ `function`

#### `fio_sock_peer_addr`

```c
fio_buf_info_s fio_sock_peer_addr(fio_socket_i s)
```

Returns a human readable address representation of the socket's peer address.

On error, returns a NULL buffer with zero length.

Buffer lengths are limited to 63 bytes.

This function is limited in its thread safety to 128 threads / calls.

_Symbol type:_ `function`

#### `fio_sock_open_local`

```c
fio_socket_i fio_sock_open_local(struct addrinfo *addr, int nonblock)
```

Creates a new network socket and binds it to a local address.

_Symbol type:_ `function`

#### `fio_sock_open_remote`

```c
fio_socket_i fio_sock_open_remote(struct addrinfo *addr, int nonblock)
```

Creates a new network socket and connects it to a remote address.

_Symbol type:_ `function`

#### `fio_sock_open_unix`

```c
fio_socket_i fio_sock_open_unix(const char *address, uint16_t flags)
```

Creates a new Unix socket and binds it to a local address.

_Symbol type:_ `function`

#### `fio_sock_set_non_block`

```c
int fio_sock_set_non_block(fio_socket_i fd)
```

Sets a file descriptor / socket to non blocking state.

_Symbol type:_ `function`

#### `fio_sock_maximize_limits`

```c
size_t fio_sock_maximize_limits(size_t maximum_limit)
```

Attempts to maximize the allowed open file limits. returns known limit

_Symbol type:_ `function`

#### `fio_sock_wait_io`

```c
short fio_sock_wait_io(fio_socket_i fd, short events, int timeout)
```

Returns 0 on timeout, -1 on error or the events that are valid.

A zero timeout returns immediately.

Possible events include POLLIN | POLLOUT

Possible return values include POLLIN | POLLOUT | POLLHUP | POLLNVAL

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-004-state-callbacks-h"></a> `./fio-stl/004 state callbacks.h`

4 public symbols.

### Types

#### `fio_state_event_type_e`

```c
typedef enum {
/** Called once during library initialization. */
FIO_CALL_ON_INITIALIZE,
/** Called once before starting up the IO reactor. */
FIO_CALL_PRE_START,
/** Called before forking starts for a worker group. */
FIO_CALL_BEFORE_FORK,
/** Called by a child worker process right after it was forked. */
FIO_CALL_IN_CHILD,
/** Called by the master process right after spawning a worker. */
FIO_CALL_IN_MASTER,
/** Called after forking ends for a worker group. */
FIO_CALL_AFTER_FORK,
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
/** Use after cleanup (e.g., leak testing). */
FIO_CALL_AFTER_EXIT,
/** used for testing and array allocation - must be last. */
FIO_CALL_NEVER
} fio_state_event_type_e
```

a callback type enum

_Symbol type:_ `type`

### Functions

#### `fio_state_callback_add`

```c
void fio_state_callback_add(fio_state_event_type_e, void (*func)(void *), void *arg)
```

Adds a callback to the list of callbacks to be called for the event.

_Symbol type:_ `function`

#### `fio_state_callback_remove`

```c
int fio_state_callback_remove(fio_state_event_type_e, void (*func)(void *), void *arg)
```

Removes a callback from the list of callbacks to be called for the event.

_Symbol type:_ `function`

#### `fio_state_callback_force`

```c
void fio_state_callback_force(fio_state_event_type_e)
```

Forces all the existing callbacks to run, as if the event occurred.

Callbacks for all initialization / idling tasks are called in order of
creation (where fio_state_event_type_e <= FIO_CALL_ON_IDLE).

Callbacks for all cleanup oriented tasks are called in reverse order of
creation (where fio_state_event_type_e >= FIO_CALL_ON_SHUTDOWN).

During an event, changes to the callback list are ignored (callbacks can't
add or remove other callbacks for the same event).

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-004-time-h"></a> `./fio-stl/004 time.h`

17 public symbols.

### Functions

#### `fio_time_real`

```c
inline struct timespec fio_time_real(void)
```

Returns human (watch) time... this value isn't as safe for measurements.

_Symbol type:_ `function`

#### `fio_time_mono`

```c
inline struct timespec fio_time_mono(void)
```

Returns monotonic time.

_Symbol type:_ `function`

#### `fio_time_nano`

```c
inline int64_t fio_time_nano(void)
```

Returns monotonic time in nano-seconds (now in 1 billionth of a second).

_Symbol type:_ `function`

#### `fio_time_micro`

```c
inline int64_t fio_time_micro(void)
```

Returns monotonic time in micro-seconds (now in 1 millionth of a second).

_Symbol type:_ `function`

#### `fio_time_milli`

```c
inline int64_t fio_time_milli(void)
```

Returns monotonic time in milliseconds.

_Symbol type:_ `function`

#### `fio_time2milli`

```c
inline int64_t fio_time2milli(struct timespec)
```

Converts a `struct timespec` to milliseconds.

_Symbol type:_ `function`

#### `fio_time2micro`

```c
inline int64_t fio_time2micro(struct timespec)
```

Converts a `struct timespec` to microseconds.

_Symbol type:_ `function`

#### `fio_time2gm`

```c
struct tm fio_time2gm(time_t time)
```

A faster (yet less localized) alternative to `gmtime_r`.

See the libc `gmtime_r` documentation for details.

Falls back to `gmtime_r` for dates before epoch.

_Symbol type:_ `function`

#### `fio_gm2time`

```c
time_t fio_gm2time(struct tm tm)
```

Converts a `struct tm` to time in seconds (assuming UTC).

_Symbol type:_ `function`

#### `fio_time2rfc7231`

```c
size_t fio_time2rfc7231(char *target, time_t time)
```

Writes an RFC 7231 date representation (HTTP date format) to target.

Usually requires 29 characters, although this may vary.

_Symbol type:_ `function`

#### `fio_time2rfc2109`

```c
size_t fio_time2rfc2109(char *target, time_t time)
```

Writes an RFC 2109 date representation to target (HTTP Cookie Format).

Usually requires 31 characters, although this may vary.

_Symbol type:_ `function`

#### `fio_time2rfc2822`

```c
size_t fio_time2rfc2822(char *target, time_t time)
```

Writes an RFC 2822 date representation to target (Internet Message Format).

Usually requires 28 to 29 characters, although this may vary.

_Symbol type:_ `function`

#### `fio_time2log`

```c
size_t fio_time2log(char *target, time_t time)
```

Writes a date representation to target in common log format. i.e.,

        [DD/MMM/yyyy:hh:mm:ss +0000]

Usually requires 29 characters (including square brackets and NUL).

_Symbol type:_ `function`

#### `fio_time2iso`

```c
size_t fio_time2iso(char *target, time_t time)
```

Writes a date representation to target in ISO 8601 format. i.e.,

        YYYY-MM-DD HH:MM:SS

Usually requires 20 characters (including NUL).

_Symbol type:_ `function`

#### `fio_time_add`

```c
inline struct timespec fio_time_add(struct timespec t, struct timespec t2)
```

Adds two `struct timespec` objects.

_Symbol type:_ `function`

#### `fio_time_add_milli`

```c
inline struct timespec fio_time_add_milli(struct timespec t, int64_t milli)
```

Adds milliseconds to a `struct timespec` object.

_Symbol type:_ `function`

#### `fio_time_cmp`

```c
inline int fio_time_cmp(struct timespec t1, struct timespec t2)
```

Compares two `struct timespec` objects.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-004-urlencoded-h"></a> `./fio-stl/004 urlencoded.h`

3 public symbols.

### Types

#### `fio_url_encoded_parser_callbacks_s`

```c
typedef struct {
/**
* Called for each name=value pair found.
*
* `name` and `value` point directly into the original input data.
* The data is NOT decoded - caller must decode if needed.
*
* Returns the (possibly updated) udata, or NULL to stop parsing.
*/
void *(*on_pair)(void *udata, fio_buf_info_s name, fio_buf_info_s value);
/**
* Called on parsing error (optional).
*
* Currently not used since URL-encoded parsing is very permissive,
* but reserved for future use.
*/
void (*on_error)(void *udata);
} fio_url_encoded_parser_callbacks_s
```

The URL-encoded parser callbacks.

Callbacks receive `udata` as their first argument.

_Symbol type:_ `type`

#### `fio_url_encoded_result_s`

```c
typedef struct {
/** Number of bytes consumed from the buffer. */
size_t consumed;
/** Number of name=value pairs found. */
size_t count;
/** Non-zero if an error occurred (callback returned NULL). */
int err;
} fio_url_encoded_result_s
```

The URL-encoded parse result type.

_Symbol type:_ `type`

### Functions

#### `fio_url_encoded_parse`

```c
fio_url_encoded_result_s fio_url_encoded_parse(const fio_url_encoded_parser_callbacks_s *callbacks, void *udata, const char *data, size_t len)
```

Parse URL-encoded data from buffer.

`callbacks` contains the callback functions (should be static const).
`udata` is user data passed to callbacks.
`data` is the URL-encoded data to parse.
`len` is the length of the data.

Returns a result struct containing:
- `consumed`: Number of bytes consumed from the buffer
- `count`: Number of name=value pairs found
- `err`: Non-zero if parsing was stopped (callback returned NULL)

Parsing rules:
- Pairs are separated by `&`
- Name and value are separated by `=`
- Empty value is valid: `name=` → value.len = 0
- Missing `=` means value is empty: `name` → name="name", value.len = 0
- Empty name with value: `=value` → name.len = 0, value="value"
- Empty pairs (`&&`) are skipped

Note: The parser does NOT decode percent-encoded characters.
Use `fio_string_write_url_dec` to decode if needed.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-005-cli-h"></a> `./fio-stl/005 cli.h`

23 public symbols.

### Macros

#### `FIO_CLI_ARG_NONE`

```c
#define FIO_CLI_ARG_NONE FIO_CLI_ARG_PRINT_HEADER
```



_Symbol type:_ `macro`

#### `FIO_CLI_STRING`

```c
#define FIO_CLI_STRING(line)   \
  ((fio___cli_line_s){.t = FIO_CLI_ARG_STRING, .l = line})
```

Indicates the CLI argument should be a String (default).

_Symbol type:_ `macro`

#### `FIO_CLI_BOOL`

```c
#define FIO_CLI_BOOL(line)   \
  ((fio___cli_line_s){.t = FIO_CLI_ARG_BOOL, .l = line})
```

Indicates the CLI argument is a Boolean value.

_Symbol type:_ `macro`

#### `FIO_CLI_INT`

```c
#define FIO_CLI_INT(line) ((fio___cli_line_s){.t = FIO_CLI_ARG_INT, .l = line})
```

Indicates the CLI argument should be an Integer (numerical).

_Symbol type:_ `macro`

#### `FIO_CLI_PRINT`

```c
#define FIO_CLI_PRINT(line)   \
  ((fio___cli_line_s){.t = FIO_CLI_ARG_PRINT, .l = line})
```

Indicates the CLI string should be printed as is with proper offset.

_Symbol type:_ `macro`

#### `FIO_CLI_PRINT_LINE`

```c
#define FIO_CLI_PRINT_LINE(line)   \
  ((fio___cli_line_s){.t = FIO_CLI_ARG_PRINT_LINE, .l = line})
```

Indicates the CLI string should be printed as is with no offset.

_Symbol type:_ `macro`

#### `FIO_CLI_PRINT_HEADER`

```c
#define FIO_CLI_PRINT_HEADER(line)   \
  ((fio___cli_line_s){.t = FIO_CLI_ARG_PRINT_HEADER, .l = line})
```

Indicates the CLI string should be printed as a header.

_Symbol type:_ `macro`

### Types

#### `fio_cli_arg_e`

```c
typedef enum {
/** A String CLI argument */
FIO_CLI_ARG_STRING,
/** A Boolean CLI argument */
FIO_CLI_ARG_BOOL,
/** An integer CLI argument */
FIO_CLI_ARG_INT,
FIO_CLI_ARG_PRINT,
FIO_CLI_ARG_PRINT_LINE,
FIO_CLI_ARG_PRINT_HEADER,
} fio_cli_arg_e
```

Used internally.

_Symbol type:_ `type`

### Functions

#### `fio_cli_start`

```c
#define fio_cli_start(argc, argv, unnamed_min, unnamed_max, description, ...)   \
  fio_cli_start((argc),   \
                (argv),   \
                (unnamed_min),   \
                (unnamed_max),   \
                (description),   \
                (fio___cli_line_s[]){__VA_ARGS__, {0}})
```

This function parses the Command Line Interface (CLI), creating a temporary
"dictionary" that allows easy access to the CLI using their names or aliases.

Command line arguments may be typed. If an optional type requirement is
provided and the provided arument fails to match the required type, execution
will end and an error message will be printed along with a short "help".

The function / macro accepts the following arguments:
- `argc`: command line argument count.
- `argv`: command line argument list (array).
- `unnamed_min`: the required minimum of un-named arguments.
- `unnamed_max`: the maximum limit of un-named arguments.
- `description`: a C string containing the program's description.
- named arguments list: a list of C strings describing named arguments.

The following optional type requirements are:

* FIO_CLI_STRING(desc_line)       - (default) string argument.
* FIO_CLI_BOOL(desc_line)         - boolean argument (no value).
* FIO_CLI_INT(desc_line)          - integer argument.
* FIO_CLI_PRINT_HEADER(desc_line) - extra header for output.
* FIO_CLI_PRINT(desc_line)        - extra information for output.

Argument names MUST start with the '-' character. The first word starting
without the '-' character will begin the description for the CLI argument.

The arguments "-?", "-h", "-help" and "--help" are automatically handled
unless overridden.

Un-named arguments shouldn't be listed in the named arguments list.

Example use:

   fio_cli_start(argc, argv, 0, 0, "The NAME example accepts the following:",
                       FIO_CLI_PRINT_HREADER("Concurrency:"),
                       FIO_CLI_INT("-t -thread number of threads to run."),
                       FIO_CLI_INT("-w -workers number of workers to run."),
                       FIO_CLI_PRINT_HREADER("Address Binding:"),
                       "-b, -address the address to bind to.",
                       FIO_CLI_INT("-p,-port the port to bind to."),
                       FIO_CLI_PRINT("\t\tset port to zero (0) for Unix s."),
                       FIO_CLI_PRINT_HREADER("Logging:"),
                       FIO_CLI_BOOL("-v -log enable logging.")
                 );

This would allow access to the named arguments:

     fio_cli_get("-b") == fio_cli_get("-address");

Once all the data was accessed, free the parsed data dictionary using:

     fio_cli_end();

It should be noted, arguments will be recognized in a number of forms, i.e.:

     app -t=1 -p3000 -a localhost

This function is NOT thread safe.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_cli_start`

```c
void fio_cli_start FIO_NOOP(int argc, char const *argv[], int unnamed_min, int unnamed_max, char const *description, fio___cli_line_s *arguments)
```

Never use the function directly, always use the MACRO, because the macro
attaches a NULL marker at the end of the `names` argument collection.

_Symbol type:_ `function`

#### `fio_cli_end`

```c
void fio_cli_end(void)
```

Clears the memory used by the CLI dictionary, removing all parsed data.

This function is NOT thread safe.

_Symbol type:_ `function`

#### `fio_cli_get`

```c
char const *fio_cli_get(char const *name)
```

Returns the argument's value as a NUL terminated C String.

_Symbol type:_ `function`

#### `fio_cli_get_str`

```c
fio_buf_info_s fio_cli_get_str(char const *name)
```

Returns the argument's value as a NUL terminated `fio_buf_info_s`.

_Symbol type:_ `function`

#### `fio_cli_get_i`

```c
int64_t fio_cli_get_i(char const *name)
```

Returns the argument's value as an integer.

_Symbol type:_ `function`

#### `fio_cli_get_bool`

```c
#define fio_cli_get_bool(name) (fio_cli_get((name)) != NULL)
```

This MACRO returns the argument's value as a boolean.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_cli_unnamed_count`

```c
unsigned int fio_cli_unnamed_count(void)
```

Returns the number of unnamed argument.

_Symbol type:_ `function`

#### `fio_cli_unnamed`

```c
char const *fio_cli_unnamed(unsigned int index)
```

Returns the unnamed argument using a 0 based `index`.

_Symbol type:_ `function`

#### `fio_cli_unnamed_str`

```c
fio_buf_info_s fio_cli_unnamed_str(unsigned int index)
```

Returns the unnamed argument using a 0 based `index`.

_Symbol type:_ `function`

#### `fio_cli_set`

```c
void fio_cli_set(char const *name, char const *value)
```

Sets the argument's value as a NUL terminated C String.

    fio_cli_set("-p", "hello");

This function is NOT thread safe.

_Symbol type:_ `function`

#### `fio_cli_set_i`

```c
void fio_cli_set_i(char const *name, int64_t i)
```

Sets the argument's value as a NUL terminated C String.

    fio_cli_start(argc, argv,
                 "this is example accepts the following options:",
                 "-p -port the port to bind to", FIO_CLI_INT;

    fio_cli_set("-p", "hello"); // fio_cli_get("-p") == fio_cli_get("-port");

This function is NOT thread safe.

_Symbol type:_ `function`

#### `fio_cli_set_unnamed`

```c
unsigned int fio_cli_set_unnamed(unsigned int index, const char *)
```

Sets / adds an unnamed argument to the 0 based array of unnamed elements.

_Symbol type:_ `function`

#### `fio_cli_each`

```c
size_t fio_cli_each(int (*task)(fio_buf_info_s name, fio_buf_info_s value, fio_cli_arg_e arg_type, void *udata), void *udata)
```

Calls `task` for every argument received.

_Symbol type:_ `function`

#### `fio_cli_type`

```c
fio_cli_arg_e fio_cli_type(char const *name)
```

Returns the argument's expected content type.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-010-mem-h"></a> `./fio-stl/010 mem.h`

46 public symbols.

### Macros

#### `FIO_MEMORY_ALIGN_LOG`

```c
#define FIO_MEMORY_ALIGN_LOG 6
```

Allocation alignment, MUST be >= 3 and <= 10

_Symbol type:_ `macro`

#### `FIO_MEMORY_ALIGN_SIZE`

```c
#define FIO_MEMORY_ALIGN_SIZE (1UL << (FIO_MEMORY_ALIGN_LOG))
```

The minimal allocation size & alignment.

_Symbol type:_ `macro`

#### `FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG`

```c
#define FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG 21
```

The logarithmic size of a single allocation "chunk" (16 blocks).

Limited to >=17 and <=24.

By default 21, which is a ~2Mb allocation per system call, resulting in a
maximum allocation size of 64Kb.

_Symbol type:_ `macro`

#### `FIO_MEMORY_CACHE_SLOTS`

```c
#define FIO_MEMORY_CACHE_SLOTS 4
```

The number of system allocation "chunks" to cache even if they are not in
use.

_Symbol type:_ `macro`

#### `FIO_MEMORY_INITIALIZE_ALLOCATIONS`

```c
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS   \
  FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT
```

Forces the allocator to zero out memory early and often, so allocations
return initialized memory (bytes are all zeros).

This will make the realloc2 safe for use (all data not copied is zero).

_Symbol type:_ `macro`

#### `FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG`

```c
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG 2
```

The number of blocks per system allocation.

More blocks protect against fragmentation, but lower the maximum number that
can be allocated without reverting to mmap.

Range: 0-4
Recommended: depends on object allocation sizes, usually 1 or 2.

_Symbol type:_ `macro`

#### `FIO_MEMORY_ENABLE_BIG_ALLOC`

```c
#define FIO_MEMORY_ENABLE_BIG_ALLOC 1
```

Uses a whole system allocation to support bigger allocations.

Could increase fragmentation costs.

_Symbol type:_ `macro`

#### `FIO_MEMORY_ARENA_COUNT`

```c
#define FIO_MEMORY_ARENA_COUNT -1
```

Memory arenas mitigate thread contention while using more memory.

Note that at some point arenas are statistically irrelevant... except when
benchmarking contention in multi-core machines.

Negative values will result in dynamic selection based on CPU core count.

_Symbol type:_ `macro`

#### `FIO_MEMORY_ARENA_COUNT_FALLBACK`

```c
#define FIO_MEMORY_ARENA_COUNT_FALLBACK 24
```



_Symbol type:_ `macro`

#### `FIO_MEMORY_ARENA_COUNT_MAX`

```c
#define FIO_MEMORY_ARENA_COUNT_MAX 64
```



_Symbol type:_ `macro`

#### `FIO_MEMORY_USE_THREAD_MUTEX`

```c
#define FIO_MEMORY_USE_THREAD_MUTEX 1
```

If arena count isn't linked to the CPU count, threads might busy-spin.
It is better to slow wait than fast busy spin when the work in the lock is
longer... and system allocations are performed inside arena locks.

_Symbol type:_ `macro`

#### `FIO_MEMORY_BLOCKS_PER_ALLOCATION`

```c
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION   \
  (1UL << FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG)
```

the number of allocation blocks per system allocation.

_Symbol type:_ `macro`

#### `FIO_MEMORY_SYS_ALLOCATION_SIZE`

```c
#define FIO_MEMORY_SYS_ALLOCATION_SIZE   \
  (1UL << FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG)
```

the total number of bytes consumed per system allocation.

_Symbol type:_ `macro`

#### `FIO_MEMORY_BLOCK_ALLOC_LIMIT`

```c
#define FIO_MEMORY_BLOCK_ALLOC_LIMIT   \
  (FIO_MEMORY_SYS_ALLOCATION_SIZE >> (FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG + 2))
```

The maximum allocation size, after which a big/system allocation is used.

_Symbol type:_ `macro`

#### `FIO_MEMORY_BIG_ALLOC_LIMIT`

```c
#define FIO_MEMORY_BIG_ALLOC_LIMIT   \
  (FIO_MEMORY_SYS_ALLOCATION_SIZE >>   \
   (FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG > 3   \
        ? 3   \
        : FIO_MEMORY_BLOCKS_PER_ALLOCATION_LOG))
```

the limit of a big allocation, if enabled

_Symbol type:_ `macro`

#### `FIO_MEMORY_ALLOC_LIMIT`

```c
#define FIO_MEMORY_ALLOC_LIMIT FIO_MEMORY_BIG_ALLOC_LIMIT
```



_Symbol type:_ `macro`

#### `FIO_MEM_BYTES2PAGES`

```c
#define FIO_MEM_BYTES2PAGES(size)   \
  ((size > (SIZE_MAX - ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)))   \
       ? size   \
       : (((size_t)(size) + ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)) &   \
          ((~(size_t)0) << FIO_MEM_PAGE_SIZE_LOG)))
```



_Symbol type:_ `macro`

#### `FIO_PAGE_ALIGN`

```c
#define FIO_PAGE_ALIGN   \
  __attribute__((assume_aligned((1UL << FIO_MEM_PAGE_SIZE_LOG))))
```



_Symbol type:_ `macro`

#### `FIO_PAGE_ALIGN_NEW`

```c
#define FIO_PAGE_ALIGN_NEW   \
  __attribute__((malloc, assume_aligned((1UL << FIO_MEM_PAGE_SIZE_LOG))))
```



_Symbol type:_ `macro`

#### `FIO_MEMORY_HEADER_SIZE`

```c
#define FIO_MEMORY_HEADER_SIZE   \
  ((sizeof(FIO_NAME(FIO_MEMORY_NAME, __mem_chunk_s)) +   \
    (FIO_MEMORY_ALIGN_SIZE - 1)) &   \
   (~(FIO_MEMORY_ALIGN_SIZE - 1)))
```



_Symbol type:_ `macro`

#### `FIO_MEMORY_UNITS_PER_BLOCK`

```c
#define FIO_MEMORY_UNITS_PER_BLOCK   \
  (FIO_MEMORY_BLOCK_SIZE / FIO_MEMORY_ALIGN_SIZE)
```



_Symbol type:_ `macro`

#### `FIO_MEMORY_STATE_SIZE`

```c
#define FIO_MEMORY_STATE_SIZE(arena_count)   \
  FIO_MEM_BYTES2PAGES(   \
      (sizeof(*FIO_NAME(FIO_MEMORY_NAME, __mem_state)) +   \
       (sizeof(FIO_NAME(FIO_MEMORY_NAME, __mem_arena_s)) * (arena_count))))
```



_Symbol type:_ `macro`

#### `FIO_MEMORY_BIG_BLOCK_MARKER`

```c
#define FIO_MEMORY_BIG_BLOCK_MARKER ((~(uint32_t)0) << 2)
```



_Symbol type:_ `macro`

#### `FIO_MEMORY_BIG_BLOCK_HEADER_SIZE`

```c
#define FIO_MEMORY_BIG_BLOCK_HEADER_SIZE   \
  (((sizeof(FIO_NAME(FIO_MEMORY_NAME, __mem_big_block_s)) +   \
     ((FIO_MEMORY_ALIGN_SIZE - 1))) &   \
    ((~(0UL)) << FIO_MEMORY_ALIGN_LOG)))
```



_Symbol type:_ `macro`

#### `FIO_MEMORY_BIG_BLOCK_SIZE`

```c
#define FIO_MEMORY_BIG_BLOCK_SIZE   \
  (FIO_MEMORY_SYS_ALLOCATION_SIZE - FIO_MEMORY_BIG_BLOCK_HEADER_SIZE)
```



_Symbol type:_ `macro`

#### `FIO_MEMORY_UNITS_PER_BIG_BLOCK`

```c
#define FIO_MEMORY_UNITS_PER_BIG_BLOCK   \
  (FIO_MEMORY_BIG_BLOCK_SIZE / FIO_MEMORY_ALIGN_SIZE)
```



_Symbol type:_ `macro`

#### `FIO_TEST_MULTI_THREADED`

```c
#define FIO_TEST_MULTI_THREADED 0
```



_Symbol type:_ `macro`

### Functions

#### `fio_malloc`

```c
void *FIO_MEM_ALIGN_NEW fio_malloc(size_t size)
```

Allocates memory using a per-CPU core block memory pool.
Memory is zeroed out.

Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT will be redirected to `mmap`,
as if `mempool_mmap` was called.

`mempool_malloc` promises a best attempt at providing locality between
consecutive calls, but locality can't be guaranteed.

_Symbol type:_ `function`

#### `fio_calloc`

```c
void *FIO_MEM_ALIGN_NEW fio_calloc(size_t size_per_unit, size_t unit_count)
```

same as calling `fio_malloc(size_per_unit * unit_count)`;

Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT will be redirected to `mmap`,
as if `mempool_mmap` was called.

_Symbol type:_ `function`

#### `fio_free`

```c
void fio_free(void *ptr)
```

Frees memory that was allocated using this library.

_Symbol type:_ `function`

#### `fio_realloc`

```c
void *FIO_MEM_ALIGN fio_realloc(void *ptr, size_t new_size)
```

Re-allocates memory. An attempt to avoid copying the data is made only for
big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).

_Symbol type:_ `function`

#### `fio_realloc2`

```c
void *FIO_MEM_ALIGN fio_realloc2(void *ptr, size_t new_size, size_t copy_len)
```

Re-allocates memory. An attempt to avoid copying the data is made only for
big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).

This variation can perform better, as it might copy less data.

_Symbol type:_ `function`

#### `fio_mmap`

```c
void *FIO_MEM_ALIGN_NEW fio_mmap(size_t size)
```

Allocates memory directly using `mmap`, this is preferred for objects that
both require almost a page of memory (or more) and expect a long lifetime.

However, since this allocation will invoke the system call (`mmap`), it will
be inherently slower.

`mempoll_free` can be used for deallocating the memory.

_Symbol type:_ `function`

#### `fio_malloc_after_fork`

```c
void fio_malloc_after_fork(void)
```

When forking is called manually, call this function to reset the facil.io
memory allocator's locks.

This is provided in case of forking in a multi-threaded environment, which
some people do even though it's bad.

_Symbol type:_ `function`

#### `fio_malloc_arenas`

```c
size_t fio_malloc_arenas(void)
```

Arena count for the allocator.

_Symbol type:_ `function`

#### `fio_malloc_block_size`

```c
size_t fio_malloc_block_size(void)
```



_Symbol type:_ `function`

#### `fio_malloc_print_state`

```c
void fio_malloc_print_state(void)
```

Prints the allocator's data structure. May be used for debugging.

_Symbol type:_ `function`

#### `fio_malloc_print_free_block_list`

```c
void fio_malloc_print_free_block_list(void)
```

Prints the allocator's free block list. May be used for debugging.

_Symbol type:_ `function`

#### `fio_malloc_print_settings`

```c
void fio_malloc_print_settings(void)
```

Prints the settings used to define the allocator.

_Symbol type:_ `function`

#### `fio_malloc_sys_alloc_size`

```c
inline size_t fio_malloc_sys_alloc_size(void)
```

System allocation sizes (bytes per system allocation).

_Symbol type:_ `function`

#### `fio_malloc_cache_slots`

```c
inline size_t fio_malloc_cache_slots(void)
```

Cached system allocations (free, but held on to).

_Symbol type:_ `function`

#### `fio_malloc_alignment`

```c
inline size_t fio_malloc_alignment(void)
```

Allocations alignment.

_Symbol type:_ `function`

#### `fio_malloc_alignment_log`

```c
inline size_t fio_malloc_alignment_log(void)
```

Allocations alignment log (base 2).

_Symbol type:_ `function`

#### `fio_malloc_alloc_limit`

```c
inline size_t fio_malloc_alloc_limit(void)
```

Allocation limit (at which point do we switch to system allocations?).

_Symbol type:_ `function`

#### `fio_malloc_arena_alloc_limit`

```c
inline size_t fio_malloc_arena_alloc_limit(void)
```

Allocation limit for arena based allocation (vs. big allocations).

_Symbol type:_ `function`

#### `fio_realloc_is_safe`

```c
inline size_t fio_realloc_is_safe(void)
```



_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-011-string-core-h"></a> `./fio-stl/011 string core.h`

103 public symbols.

### Macros

#### `FIO_STRING_WRITE_STR1`

```c
#define FIO_STRING_WRITE_STR1(str_)   \
  ((fio_string_write_s){   \
      .klass = 1,   \
      .info.str = {.len = (size_t)FIO_STRLEN((str_)), .buf = (str_)}})
```

A macro to add a String to `fio_string_write2`.

_Symbol type:_ `macro`

#### `FIO_STRING_WRITE_STR2`

```c
#define FIO_STRING_WRITE_STR2(str_, len_)   \
  ((fio_string_write_s){.klass = 1, .info.str = {.len = (len_), .buf = (str_)}})
```

A macro to add a String with known length to `fio_string_write2`.

_Symbol type:_ `macro`

#### `FIO_STRING_WRITE_STR_INFO`

```c
#define FIO_STRING_WRITE_STR_INFO(str_)   \
  ((fio_string_write_s){.klass = 1,   \
                        .info.str = {.len = (str_).len, .buf = (str_).buf}})
```

A macro to add a String with known length to `fio_string_write2`.

_Symbol type:_ `macro`

#### `FIO_STRING_WRITE_NUM`

```c
#define FIO_STRING_WRITE_NUM(num)   \
  ((fio_string_write_s){.klass = 2, .info.i = (int64_t)(num)})
```

A macro to add a signed number to `fio_string_write2`.

_Symbol type:_ `macro`

#### `FIO_STRING_WRITE_UNUM`

```c
#define FIO_STRING_WRITE_UNUM(num)   \
  ((fio_string_write_s){.klass = 3, .info.u = (uint64_t)(num)})
```

A macro to add an unsigned number to `fio_string_write2`.

_Symbol type:_ `macro`

#### `FIO_STRING_WRITE_HEX`

```c
#define FIO_STRING_WRITE_HEX(num)   \
  ((fio_string_write_s){.klass = 4, .info.u = (uint64_t)(num)})
```

A macro to add a hex representation to `fio_string_write2`.

_Symbol type:_ `macro`

#### `FIO_STRING_WRITE_BIN`

```c
#define FIO_STRING_WRITE_BIN(num)   \
  ((fio_string_write_s){.klass = 5, .info.u = (uint64_t)(num)})
```

A macro to add a binary representation to `fio_string_write2`.

_Symbol type:_ `macro`

#### `FIO_STRING_WRITE_FLOAT`

```c
#define FIO_STRING_WRITE_FLOAT(num)   \
  ((fio_string_write_s){.klass = 6, .info.f = (double)(num)})
```

A macro to add a float (double) to `fio_string_write2`.

_Symbol type:_ `macro`

#### `FIO_STRING_SYS_REALLOC`

```c
#define FIO_STRING_SYS_REALLOC fio_string_sys_reallocate
```

Default reallocation callback implementation using libc `realloc`.

_Symbol type:_ `macro`

#### `FIO_STRING_REALLOC`

```c
#define FIO_STRING_REALLOC fio_string_default_reallocate
```

Default reallocation callback implementation using the default allocator

_Symbol type:_ `macro`

#### `FIO_STRING_ALLOC_COPY`

```c
#define FIO_STRING_ALLOC_COPY fio_string_default_allocate_copy
```

Default reallocation callback for memory that mustn't be freed.

_Symbol type:_ `macro`

#### `FIO_STRING_ALLOC_KEY`

```c
#define FIO_STRING_ALLOC_KEY fio_string_default_key_alloc
```

default allocator for the fio_keystr_s string data..

_Symbol type:_ `macro`

#### `FIO_STRING_FREE`

```c
#define FIO_STRING_FREE fio_string_default_free
```

Frees memory that was allocated with the default callbacks.

_Symbol type:_ `macro`

#### `FIO_STRING_FREE2`

```c
#define FIO_STRING_FREE2 fio_string_default_free2
```

Frees memory that was allocated with the default callbacks.

_Symbol type:_ `macro`

#### `FIO_STRING_FREE_KEY`

```c
#define FIO_STRING_FREE_KEY fio_string_default_free_key
```

Frees memory that was allocated for a key string.

_Symbol type:_ `macro`

#### `FIO_STRING_FREE_NOOP`

```c
#define FIO_STRING_FREE_NOOP fio_string_default_free_noop
```

Does nothing.

_Symbol type:_ `macro`

#### `FIO_STRING_FREE_NOOP2`

```c
#define FIO_STRING_FREE_NOOP2 fio_string_default_free_noop2
```

Does nothing.

_Symbol type:_ `macro`

#### `FIO_KEYSTR_CONST`

```c
#define FIO_KEYSTR_CONST ((size_t)-1LL)
```



_Symbol type:_ `macro`

### Types

#### `fio_string_realloc_fn`

```c
typedef int (*fio_string_realloc_fn)(fio_str_info_s *dest, size_t len)
```

A reallocation callback type for buffers in a `fio_str_info_s`.

The callback MUST allocate at least `len + 1` bytes, setting the new capacity
in `dest->capa`.

Returns 0 on success, -1 on error.

_Symbol type:_ `type`

#### `fio_string_write_s`

```c
typedef struct {
size_t klass;
union {
struct {
size_t len;
const char *buf;
} str;
double f;
int64_t i;
uint64_t u;
} info;
} fio_string_write_s
```

Argument type used by fio_string_write2.

_Symbol type:_ `type`

#### `fio_keystr_s`

```c
struct fio_keystr_s
```

a semi-opaque type used for the `fio_keystr` functions

_Symbol type:_ `type`

### Functions

#### `fio_string_write`

```c
FIO_SFUNC int fio_string_write(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *restrict src, size_t len)
```

Writes data to the end of the string in the `fio_string_s` struct,
returning an updated `fio_string_s` struct.

The returned string is NUL terminated if edited.

* `dest` an `fio_string_s` struct containing the destination string.

* `reallocate` is a callback that attempts to reallocate more memory (i.e.,
using `realloc`) and returns an updated `fio_string_s` struct containing the
  updated capacity and buffer pointer (as well as the original length).

  On failure the original `fio_string_s` should be returned. if
`reallocate` is NULL or fails, the data copied will be truncated.

* `src` is the data to be written to the end of `dest`.

* `len` is the length of the data to be written to the end of `dest`.

Note: this function performs only minimal checks and assumes that `dest` is
      fully valid - i.e., that `dest.capa >= dest.len`, that `dest.buf` is
      valid, etc'.

An example for a `reallocate` callback using the system's `realloc` function:

     int fio_string_realloc_system(fio_str_info_s *dest, size_t len_no_nul) {
      const size_t new_capa = fio_string_capa4len(len_no_nul);
      void *tmp = realloc(dest.buf, new_capa);
      if (!tmp)
        return -1;
      dest.capa = new_capa;
      dest.buf = (char *)tmp;
      return 0;
    }

An example for using the function:

    void example(void) {
      char buf[32];
      fio_str_info_s str = FIO_STR_INFO3(buf, 0, 32);
      fio_string_write(&str, NULL, "The answer is: 0x", 17);
      str.len += fio_ltoa(str.buf + str.len, 42, 16);
      fio_string_write(&str, NULL, "!\n", 2);
      printf("%s", str.buf);
    }

_Symbol type:_ `function`

#### `fio_string_replace`

```c
int fio_string_replace(fio_str_info_s *dest, fio_string_realloc_fn reallocate, intptr_t start_pos, size_t overwrite_len, const void *src, size_t len)
```

Similar to `fio_string_write`, only replacing/inserting a sub-string in a
specific location.

Negative `start_pos` values are calculated backwards, `-1` == end of String.

When `overwrite_len` is zero, the function will insert the data at
`start_pos`, pushing existing data until after the inserted data.

If `overwrite_len` is non-zero, than `overwrite_len` bytes will be
overwritten (or deleted).

If `len == 0` than `src` will be ignored and the data marked for replacement
will be erased.

_Symbol type:_ `function`

#### `fio_string_write2`

```c
int fio_string_write2(fio_str_info_s *restrict dest, fio_string_realloc_fn reallocate, const fio_string_write_s srcs[])
```

Writes a group of objects (strings, numbers, etc') to `dest`.

`dest` and `reallocate` are similar to `fio_string_write`.

`src` is an array of `fio_string_write_s` structs, ending with a struct
that's all set to 0.

Use the `fio_string_write2` macro for ease, i.e.:

   fio_str_info_s str = {0};
   fio_string_write2(&str, my_reallocate,
                       FIO_STRING_WRITE_STR1("The answer is: "),
                       FIO_STRING_WRITE_NUM(42),
                       FIO_STRING_WRITE_STR2("(0x", 3),
                       FIO_STRING_WRITE_HEX(42),
                       FIO_STRING_WRITE_STR2(")", 1));

Note: this function might end up allocating more memory than absolutely
required as it favors fast performance over memory savings. It performs only
a single allocation (if any) and computes numeral string length only when
writing the numbers to the string.

_Symbol type:_ `function`

#### `fio_string_write2`

```c
#define fio_string_write2(dest, reallocate, ...)   \
  fio_string_write2((dest),   \
                    (reallocate),   \
                    (fio_string_write_s[]){__VA_ARGS__, {0}})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_string_write_i`

```c
int fio_string_write_i(fio_str_info_s *dest, fio_string_realloc_fn reallocate, int64_t i)
```



_Symbol type:_ `function`

#### `fio_string_write_u`

```c
int fio_string_write_u(fio_str_info_s *dest, fio_string_realloc_fn reallocate, uint64_t i)
```



_Symbol type:_ `function`

#### `fio_string_write_hex`

```c
int fio_string_write_hex(fio_str_info_s *dest, fio_string_realloc_fn reallocate, uint64_t i)
```



_Symbol type:_ `function`

#### `fio_string_write_bin`

```c
int fio_string_write_bin(fio_str_info_s *dest, fio_string_realloc_fn reallocate, uint64_t i)
```



_Symbol type:_ `function`

#### `fio_string_printf`

```c
FIO___PRINTF_STYLE(3, 0) int fio_string_printf( fio_str_info_s *dest, fio_string_realloc_fn reallocate, const char *format, ...)
```

Similar to fio_string_write, only using printf semantics.

_Symbol type:_ `function`

#### `fio_string_vprintf`

```c
FIO___PRINTF_STYLE(3, 0) int fio_string_vprintf( fio_str_info_s *dest, fio_string_realloc_fn reallocate, const char *format, va_list argv)
```

Similar to fio_string_write, only using vprintf semantics.

_Symbol type:_ `function`

#### `fio_string_write_escape`

```c
int fio_string_write_escape(fio_str_info_s *restrict dest, fio_string_realloc_fn reallocate, const void *raw, size_t raw_len)
```

Writes data at the end of the String, escaping the data using JSON semantics.

The JSON semantic are common to many programming languages, promising a UTF-8
String while making it easy to read and copy the string during debugging.

_Symbol type:_ `function`

#### `fio_string_write_unescape`

```c
int fio_string_write_unescape(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *enscaped, size_t enscaped_len)
```

Writes an escaped data into the string after un-escaping the data.

_Symbol type:_ `function`

#### `fio_string_write_base32enc`

```c
int fio_string_write_base32enc(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *raw, size_t raw_len)
```

Writes data to String using base64 encoding.

_Symbol type:_ `function`

#### `fio_string_write_base32dec`

```c
int fio_string_write_base32dec(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *encoded, size_t encoded_len)
```

Writes decoded base64 data to String.

_Symbol type:_ `function`

#### `fio_string_write_base64enc`

```c
int fio_string_write_base64enc(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *raw, size_t raw_len, uint8_t url_encoded)
```

Writes data to String using base64 encoding.

_Symbol type:_ `function`

#### `fio_string_write_base64dec`

```c
int fio_string_write_base64dec(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *encoded, size_t encoded_len)
```

Writes decoded base64 data to String.

_Symbol type:_ `function`

#### `fio_string_write_url_enc`

```c
int fio_string_write_url_enc(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *raw, size_t raw_len)
```

Writes data to String using URL encoding (a.k.a., percent encoding).

_Symbol type:_ `function`

#### `fio_string_write_url_dec`

```c
int fio_string_write_url_dec(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *encoded, size_t encoded_len)
```

Writes decoded URL data to String, decoding + to spaces.

_Symbol type:_ `function`

#### `fio_string_write_path_dec`

```c
int fio_string_write_path_dec(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *encoded, size_t encoded_len)
```

Writes decoded URL data to String, without decoding + to spaces.

_Symbol type:_ `function`

#### `fio_string_write_html_escape`

```c
int fio_string_write_html_escape(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *raw, size_t raw_len)
```

Writes HTML escaped data to a String.

_Symbol type:_ `function`

#### `fio_string_write_html_unescape`

```c
int fio_string_write_html_unescape(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const void *enscaped, size_t enscaped_len)
```

Writes HTML un-escaped data to a String - incomplete and minimal.

_Symbol type:_ `function`

#### `fio_string_readfd`

```c
int fio_string_readfd(fio_str_info_s *dest, fio_string_realloc_fn reallocate, int fd, intptr_t start_at, size_t limit)
```

Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at`.

If `limit` is 0 (or less than 0) data will be written until EOF.

If `start_at` is negative, position will be calculated from the end of the
file where `-1 == EOF`.

Note: this will fail unless used on actual files (not sockets, not pipes).

_Symbol type:_ `function`

#### `fio_string_readfile`

```c
int fio_string_readfile(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const char *filename, intptr_t start_at, size_t limit)
```

Opens the file `filename` and pastes it's contents (or a slice ot it) at
the end of the String. If `limit == 0`, than the data will be read until
EOF.

If the file can't be located, opened or read, or if `start_at` is beyond
the EOF position, NULL is returned in the state's `data` field.

_Symbol type:_ `function`

#### `fio_string_getdelim_fd`

```c
int fio_string_getdelim_fd(fio_str_info_s *dest, fio_string_realloc_fn reallocate, int fd, intptr_t start_at, char delim, size_t limit)
```

Writes up to `limit` bytes from `fd` into `dest`, starting at `start_at` and
ending either at the first occurrence of `delim` or at EOF.

If `limit` is 0 (or less than 0) as much data as may be required will be
written.

If `start_at` is negative, position will be calculated from the end of the
file where `-1 == EOF`.

Note: this will fail unless used on actual seekable files (not sockets, not
pipes).

_Symbol type:_ `function`

#### `fio_string_getdelim_file`

```c
int fio_string_getdelim_file(fio_str_info_s *dest, fio_string_realloc_fn reallocate, const char *filename, intptr_t start_at, char delim, size_t limit)
```

Opens the file `filename`, calls `fio_string_getdelim_fd` and closes the
file.

_Symbol type:_ `function`

#### `fio_string_capa4len`

```c
inline size_t fio_string_capa4len(size_t new_len)
```



_Symbol type:_ `function`

#### `fio_string_default_reallocate`

```c
int fio_string_default_reallocate(fio_str_info_s *dst, size_t len)
```

default reallocation callback implementation.

_Symbol type:_ `function`

#### `fio_string_default_allocate_copy`

```c
int fio_string_default_allocate_copy(fio_str_info_s *dest, size_t new_capa)
```

default reallocation callback for memory that mustn't be freed.

_Symbol type:_ `function`

#### `fio_string_default_free`

```c
void fio_string_default_free(void *)
```

frees memory that was allocated with the default callbacks.

_Symbol type:_ `function`

#### `fio_string_default_free2`

```c
void fio_string_default_free2(fio_str_info_s str)
```

frees memory that was allocated with the default callbacks.

_Symbol type:_ `function`

#### `fio_string_default_free_noop`

```c
void fio_string_default_free_noop(void *)
```

does nothing.

_Symbol type:_ `function`

#### `fio_string_default_free_noop2`

```c
void fio_string_default_free_noop2(fio_str_info_s str)
```

does nothing.

_Symbol type:_ `function`

#### `fio_string_default_key_alloc`

```c
void *fio_string_default_key_alloc(size_t len)
```

default allocator for the fio_keystr_s string data..

_Symbol type:_ `function`

#### `fio_string_default_free_key`

```c
void fio_string_default_free_key(void *, size_t)
```

frees a fio_keystr_s memory that was allocated with the default callback.

_Symbol type:_ `function`

#### `fio_string_utf8_valid`

```c
bool fio_string_utf8_valid(fio_str_info_s str)
```

Returns 1 if the String is UTF-8 valid and 0 if not.

_Symbol type:_ `function`

#### `fio_string_utf8_len`

```c
size_t fio_string_utf8_len(fio_str_info_s str)
```

Returns the String's length in UTF-8 characters or 0 if invalid.

_Symbol type:_ `function`

#### `fio_string_utf8_valid_code_point`

```c
size_t fio_string_utf8_valid_code_point(const void *u8c, size_t buf_len)
```

Returns 0 if non-UTF-8 or returns 1-4 (UTF-8 if a valid char).

_Symbol type:_ `function`

#### `fio_string_utf8_select`

```c
int fio_string_utf8_select(fio_str_info_s str, intptr_t *pos, size_t *len)
```

Takes a UTF-8 character selection information (UTF-8 position and length)
and updates the same variables so they reference the raw byte slice
information.

If the String isn't UTF-8 valid up to the requested selection, than `pos`
will be updated to `-1` otherwise values are always positive.

The returned `len` value may be shorter than the original if there wasn't
enough data left to accommodate the requested length. When a `len` value of
`0` is returned, this means that `pos` marks the end of the String.

Returns -1 on error and 0 on success.

_Symbol type:_ `function`

#### `fio_string_is_greater_buf`

```c
int fio_string_is_greater_buf(fio_buf_info_s a, fio_buf_info_s b)
```

Compares two `fio_buf_info_s`, returning 1 if data in a is bigger than b.

Note: returns 0 if data in b is bigger than or equal(!).

_Symbol type:_ `function`

#### `fio_string_is_greater`

```c
inline int fio_string_is_greater(fio_str_info_s a, fio_str_info_s b)
```

Compares two strings, returning 1 if string a is bigger than string b.

Note: returns 0 if string b is bigger than string a or if strings are equal.

_Symbol type:_ `function`

#### `fio_bstr_reserve`

```c
inline char *fio_bstr_reserve(char *bstr, size_t len)
```

Reserves `len` for future `write` operations (used to minimize realloc).

_Symbol type:_ `function`

#### `fio_bstr_copy`

```c
inline char *fio_bstr_copy(char *bstr)
```

Copies a `fio_bstr` using "copy on write".

_Symbol type:_ `function`

#### `fio_bstr_free`

```c
inline void fio_bstr_free(char *bstr)
```

Frees a binary string allocated by a `fio_bstr` function. Returns NULL.

_Symbol type:_ `function`

#### `fio_bstr_info`

```c
inline fio_str_info_s fio_bstr_info(const char *bstr)
```

Returns information about the fio_bstr.

_Symbol type:_ `function`

#### `fio_bstr_buf`

```c
inline fio_buf_info_s fio_bstr_buf(const char *bstr)
```

Returns information about the fio_bstr.

_Symbol type:_ `function`

#### `fio_bstr_len`

```c
inline size_t fio_bstr_len(const char *bstr)
```

Gets the length of the fio_bstr. `bstr` MUST NOT be NULL.

_Symbol type:_ `function`

#### `fio_bstr_len_set`

```c
inline char *fio_bstr_len_set(char *bstr, size_t len)
```

Sets the length of the fio_bstr. `bstr` MUST NOT be NULL.

_Symbol type:_ `function`

#### `fio_bstr_is_greater`

```c
FIO_SFUNC int fio_bstr_is_greater(const char *a, const char *b)
```

Compares to see if fio_bstr a is greater than fio_bstr b (for FIO_SORT).

_Symbol type:_ `function`

#### `fio_bstr_is_eq`

```c
FIO_SFUNC int fio_bstr_is_eq(const char *a, const char *b)
```

Compares to see if fio_bstr a is equal to another fio_bstr.

_Symbol type:_ `function`

#### `fio_bstr_is_eq2info`

```c
FIO_SFUNC int fio_bstr_is_eq2info(const char *a_, fio_str_info_s b)
```

Compares to see if fio_bstr a is equal to another String.

_Symbol type:_ `function`

#### `fio_bstr_is_eq2buf`

```c
FIO_SFUNC int fio_bstr_is_eq2buf(const char *a_, fio_buf_info_s b)
```

Compares to see if fio_bstr a is equal to another String.

_Symbol type:_ `function`

#### `fio_bstr_write`

```c
inline char *fio_bstr_write(char *bstr, const void *restrict src, size_t len)
```

Writes data to a fio_bstr, returning the address of the new fio_bstr.
Returns existing string on reallocation error (true for all fio_bstr_write).

_Symbol type:_ `function`

#### `fio_bstr_replace`

```c
inline char *fio_bstr_replace(char *bstr, intptr_t start_pos, size_t overwrite_len, const void *src, size_t len)
```

Replaces data in a fio_bstr, returning the address of the new fio_bstr.

_Symbol type:_ `function`

#### `fio_bstr_write2`

```c
inline char *fio_bstr_write2(char *bstr, const fio_string_write_s srcs[])
```

Writes data to a fio_bstr, returning the address of the new fio_bstr.

_Symbol type:_ `function`

#### `fio_bstr_write2`

```c
#define fio_bstr_write2(bstr, ...)   \
  fio_bstr_write2(bstr, (fio_string_write_s[]){__VA_ARGS__, {0}})
```

Writes data to a fio_bstr, returning the address of the new fio_bstr.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_bstr_write_i`

```c
inline char *fio_bstr_write_i(char *bstr, int64_t num)
```

Writes number to a fio_bstr, returning the address of the new fio_bstr.

_Symbol type:_ `function`

#### `fio_bstr_write_u`

```c
inline char *fio_bstr_write_u(char *bstr, uint64_t num)
```

Writes number to a fio_bstr, returning the address of the new fio_bstr.

_Symbol type:_ `function`

#### `fio_bstr_write_hex`

```c
inline char *fio_bstr_write_hex(char *bstr, uint64_t num)
```

Writes number to a fio_bstr, returning the address of the new fio_bstr.

_Symbol type:_ `function`

#### `fio_bstr_write_bin`

```c
inline char *fio_bstr_write_bin(char *bstr, uint64_t num)
```

Writes number to a fio_bstr, returning the address of the new fio_bstr.

_Symbol type:_ `function`

#### `fio_bstr_write_escape`

```c
inline char *fio_bstr_write_escape(char *bstr, const void *src, size_t len)
```

Writes escaped data to a fio_bstr, returning its new address.

_Symbol type:_ `function`

#### `fio_bstr_write_unescape`

```c
inline char *fio_bstr_write_unescape(char *bstr, const void *src, size_t len)
```

Un-escapes and writes data to a fio_bstr, returning its new address.

_Symbol type:_ `function`

#### `fio_bstr_write_base64enc`

```c
inline char *fio_bstr_write_base64enc(char *bstr, const void *src, size_t len, uint8_t url_encoded)
```

Writes base64 encoded data to a fio_bstr, returning its new address.

_Symbol type:_ `function`

#### `fio_bstr_write_base64dec`

```c
inline char *fio_bstr_write_base64dec(char *bstr, const void *src, size_t len)
```

Decodes base64 data and writes to a fio_bstr, returning its new address.

_Symbol type:_ `function`

#### `fio_bstr_write_url_enc`

```c
inline char *fio_bstr_write_url_enc(char *bstr, const void *data, size_t len)
```

Writes data to String using URL encoding (a.k.a., percent encoding).

_Symbol type:_ `function`

#### `fio_bstr_write_url_dec`

```c
inline char *fio_bstr_write_url_dec(char *bstr, const void *encoded, size_t len)
```

Writes decoded URL data to String.

_Symbol type:_ `function`

#### `fio_bstr_write_html_escape`

```c
inline char *fio_bstr_write_html_escape(char *bstr, const void *raw, size_t len)
```

Writes HTML escaped data to a String.

_Symbol type:_ `function`

#### `fio_bstr_write_html_unescape`

```c
inline char *fio_bstr_write_html_unescape(char *bstr, const void *escaped, size_t len)
```

Writes HTML un-escaped data to a String - incomplete and minimal.

_Symbol type:_ `function`

#### `fio_bstr_readfd`

```c
inline char *fio_bstr_readfd(char *bstr, int fd, intptr_t start_at, intptr_t limit)
```

Writes to the String from a regular file `fd`.

_Symbol type:_ `function`

#### `fio_bstr_readfile`

```c
inline char *fio_bstr_readfile(char *bstr, const char *filename, intptr_t start_at, intptr_t limit)
```

Writes to the String from a regular file named `filename`.

_Symbol type:_ `function`

#### `fio_bstr_getdelim_file`

```c
inline char *fio_bstr_getdelim_file(char *bstr, const char *filename, intptr_t start_at, char delim, size_t limit)
```

Writes to the String from a regular file named `filename`.

_Symbol type:_ `function`

#### `fio_bstr_getdelim_fd`

```c
inline char *fio_bstr_getdelim_fd(char *bstr, int fd, intptr_t start_at, char delim, size_t limit)
```

Writes to the String from a regular file `fd`.

_Symbol type:_ `function`

#### `fio_bstr_printf`

```c
inline FIO___PRINTF_STYLE(2, 0) char *fio_bstr_printf(char *bstr, const char *format, ...)
```

Writes a `fio_bstr` in `printf` style.

_Symbol type:_ `function`

#### `fio_bstr_reallocate`

```c
int fio_bstr_reallocate(fio_str_info_s *dest, size_t len)
```

default reallocation callback implementation - mostly for internal use.

_Symbol type:_ `function`

#### `fio_keystr_buf`

```c
inline fio_buf_info_s fio_keystr_buf(fio_keystr_s *str)
```

returns the Key String. NOTE: Key Strings are NOT NUL TERMINATED!

_Symbol type:_ `function`

#### `fio_keystr_info`

```c
inline fio_str_info_s fio_keystr_info(fio_keystr_s *str)
```

returns the Key String. NOTE: Key Strings are NOT NUL TERMINATED!

_Symbol type:_ `function`

#### `fio_keystr_tmp`

```c
inline fio_keystr_s fio_keystr_tmp(const char *buf, uint32_t len)
```

Returns a TEMPORARY `fio_keystr_s`.

_Symbol type:_ `function`

#### `fio_keystr_init`

```c
FIO_SFUNC fio_keystr_s fio_keystr_init(fio_str_info_s str, void *(*alloc_func)(size_t len))
```

Returns an initialized `fio_keystr_s` containing a copy of `str`.

_Symbol type:_ `function`

#### `fio_keystr_destroy`

```c
FIO_SFUNC void fio_keystr_destroy(fio_keystr_s *key, void (*free_func)(void *, size_t))
```

Destroys an initialized `fio_keystr_s`.

_Symbol type:_ `function`

#### `fio_keystr_is_eq`

```c
inline int fio_keystr_is_eq(fio_keystr_s a, fio_keystr_s b)
```

Compares two Key Strings.

_Symbol type:_ `function`

#### `fio_keystr_is_eq2`

```c
inline int fio_keystr_is_eq2(fio_keystr_s a_, fio_str_info_s b)
```

Compares a Key String to any String - used internally by the hash map.

_Symbol type:_ `function`

#### `fio_keystr_is_eq3`

```c
inline int fio_keystr_is_eq3(fio_keystr_s a_, fio_buf_info_s b)
```

Compares a Key String to any String - used internally by the hash map.

_Symbol type:_ `function`

#### `fio_keystr_hash`

```c
inline uint64_t fio_keystr_hash(fio_keystr_s a)
```

Returns a good-enough `fio_keystr_s` risky hash.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-012-gfm-h"></a> `./fio-stl/012 gfm.h`

10 public symbols.

### Macros

#### `FIO_GFM_MAX_DEPTH`

```c
#define FIO_GFM_MAX_DEPTH 255
```

Maximum container nesting depth (blockquote + list).

_Symbol type:_ `macro`

#### `FIO_GFM_MAX_TABLE_COLUMNS`

```c
#define FIO_GFM_MAX_TABLE_COLUMNS 64
```

Maximum table columns (2 bits each → 4 columns per byte).

_Symbol type:_ `macro`

#### `FIO_GFM_REF_CACHE_SIZE`

```c
#define FIO_GFM_REF_CACHE_SIZE 128
```

Maximum reference definitions held in the fast cache.

_Symbol type:_ `macro`

#### `FIO_GFM_ERR_GENERIC`

```c
#define FIO_GFM_ERR_GENERIC -1
```

Error codes — negative values are parser-generated.

_Symbol type:_ `macro`

#### `FIO_GFM_ERR_DEPTH`

```c
#define FIO_GFM_ERR_DEPTH   -2
```



_Symbol type:_ `macro`

#### `FIO_GFM_ERR_INPUT`

```c
#define FIO_GFM_ERR_INPUT   -3
```



_Symbol type:_ `macro`

#### `FIO_GFM_F_TIGHT`

```c
#define FIO_GFM_F_TIGHT ((uint8_t)1U << 0)
```

List is tight (no blank lines between items).

_Symbol type:_ `macro`

#### `FIO_GFM_F_LOOSE_SEEN`

```c
#define FIO_GFM_F_LOOSE_SEEN ((uint8_t)1U << 1)
```

A blank line was seen between items — list is loose.

_Symbol type:_ `macro`

#### `FIO_GFM_F_TASK`

```c
#define FIO_GFM_F_TASK ((uint8_t)1U << 2)
```

GFM task-list marker present.

_Symbol type:_ `macro`

#### `FIO_GFM_F_TASK_CHECKED`

```c
#define FIO_GFM_F_TASK_CHECKED ((uint8_t)1U << 3)
```

GFM task-list marker is checked.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-102-poll-api-h"></a> `./fio-stl/102 poll api.h`

16 public symbols.

### Macros

#### `FIO_POLL_POSSIBLE_FLAGS`

```c
#define FIO_POLL_POSSIBLE_FLAGS (POLLIN | POLLOUT | POLLPRI)
```

The user flags IO events recognize

_Symbol type:_ `macro`

#### `FIO_POLL_MAX_EVENTS`

```c
#define FIO_POLL_MAX_EVENTS 256
```

relevant only for epoll and kqueue - maximum number of events per review

_Symbol type:_ `macro`

#### `FIO_POLL_ENGINE_EPOLL`

```c
#define FIO_POLL_ENGINE_EPOLL
```



_Symbol type:_ `macro`

#### `FIO_POLL_ENGINE_KQUEUE`

```c
#define FIO_POLL_ENGINE_KQUEUE
```



_Symbol type:_ `macro`

#### `FIO_POLL_ENGINE_POLL`

```c
#define FIO_POLL_ENGINE_POLL
```



_Symbol type:_ `macro`

#### `FIO_POLL_ENGINE_STR`

```c
#define FIO_POLL_ENGINE_STR "epoll"
```



_Symbol type:_ `macro`

#### `FIO_POLL_VALIDATE`

```c
#define FIO_POLL_VALIDATE(settings_dest)   \
  if (!(settings_dest).on_data)   \
    (settings_dest).on_data = fio___poll_ev_mock;   \
  if (!(settings_dest).on_ready)   \
    (settings_dest).on_ready = fio___poll_ev_mock;   \
  if (!(settings_dest).on_close)   \
    (settings_dest).on_close = fio___poll_ev_mock;
```



_Symbol type:_ `macro`

### Types

#### `fio_poll_s`

```c
struct fio_poll_s
```

the `fio_poll_s` type should be considered opaque.

_Symbol type:_ `type`

#### `fio_poll_settings_s`

```c
typedef struct {
/** callback for when data is available in the incoming buffer. */
void (*on_data)(void *udata);
/** callback for when the outgoing buffer allows a call to `write`. */
void (*on_ready)(void *udata);
/** callback for closed connections and / or connections with errors. */
void (*on_close)(void *udata);
} fio_poll_settings_s
```



_Symbol type:_ `type`

### Functions

#### `fio_poll_init`

```c
inline void fio_poll_init(fio_poll_s *p, fio_poll_settings_s)
```

Initializes the polling object, allocating its resources.

_Symbol type:_ `function`

#### `fio_poll_init`

```c
#define fio_poll_init(p, ...)   \
  fio_poll_init((p), (fio_poll_settings_s){__VA_ARGS__})
```

Initializes the polling object, allocating its resources.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_poll_destroy`

```c
inline void fio_poll_destroy(fio_poll_s *p)
```

Destroys the polling object, freeing its resources.

_Symbol type:_ `function`

#### `fio_poll_engine`

```c
inline const char *fio_poll_engine(void)
```

returns the system call used for polling as a constant string.

_Symbol type:_ `function`

#### `fio_poll_monitor`

```c
int fio_poll_monitor(fio_poll_s *p, fio_socket_i fd, void *udata, unsigned short flags)
```

Adds a file descriptor to be monitored, adds events to be monitored or
updates the monitored file's `udata`.

Possible flags are: `POLLIN` and `POLLOUT`. Other flags may be set but might
be ignored.

Monitoring mode is always one-shot. If an event if fired, it is removed from
the monitoring state.

Returns -1 on error.

_Symbol type:_ `function`

#### `fio_poll_review`

```c
int fio_poll_review(fio_poll_s *p, size_t timeout)
```

Reviews if any of the monitored file descriptors has any events.

`timeout` is in milliseconds.

Returns the number of events called.

Polling is thread safe, but has different effects on different threads.

Adding a new file descriptor from one thread while polling in a different
thread will not poll that IO until `fio_poll_review` is called again.

_Symbol type:_ `function`

#### `fio_poll_forget`

```c
int fio_poll_forget(fio_poll_s *p, fio_socket_i fd)
```

Stops monitoring the specified file descriptor (if monitoring).

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-102-queue-h"></a> `./fio-stl/102 queue.h`

28 public symbols.

### Macros

#### `FIO_QUEUE_TASKS_PER_ALLOC`

```c
#define FIO_QUEUE_TASKS_PER_ALLOC 338
```



_Symbol type:_ `macro`

#### `FIO_QUEUE_STATIC_INIT`

```c
#define FIO_QUEUE_STATIC_INIT(queue)   \
  {   \
    .r = &(queue).mem, .w = &(queue).mem,   \
    .lock = (fio_thread_mutex_t)FIO_THREAD_MUTEX_INIT,   \
    .consumers = FIO_LIST_INIT((queue).consumers),   \
  }
```

May be used to initialize global, static memory, queues.

_Symbol type:_ `macro`

#### `FIO_TIMER_QUEUE_INIT`

```c
#define FIO_TIMER_QUEUE_INIT   \
  { .lock = ((fio_thread_mutex_t)FIO_THREAD_MUTEX_INIT) }
```



_Symbol type:_ `macro`

### Types

#### `fio_queue_task_s`

```c
typedef struct {
/** The function to call */
void (*fn)(void *, void *);
/** User opaque data */
void *udata1;
/** User opaque data */
void *udata2;
} fio_queue_task_s
```

Task information

_Symbol type:_ `type`

#### `fio_queue_s`

```c
typedef struct {
/** task read pointer. */
fio___task_ring_s *r;
/** task write pointer. */
fio___task_ring_s *w;
/** the number of tasks waiting to be performed. */
uint32_t count;
/** global queue lock. */
FIO___LOCK_TYPE lock;
/** linked lists of consumer threads. */
FIO_LIST_NODE consumers;
/** main ring buffer associated with the queue. */
fio___task_ring_s mem;
} fio_queue_s
```

The queue object - should be considered opaque (or, at least, read only).

_Symbol type:_ `type`

#### `fio_timer_queue_s`

```c
typedef struct {
fio___timer_event_s *next;
FIO___LOCK_TYPE lock;
} fio_timer_queue_s
```



_Symbol type:_ `type`

#### `fio_timer_schedule_args_s`

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
int64_t start_at;
} fio_timer_schedule_args_s
```



_Symbol type:_ `type`

### Functions

#### `fio_queue_init`

```c
inline void fio_queue_init(fio_queue_s *q)
```

Initializes a fio_queue_s object.

_Symbol type:_ `function`

#### `fio_queue_destroy`

```c
void fio_queue_destroy(fio_queue_s *q)
```

Destroys a queue and re-initializes it, after freeing any used resources.

_Symbol type:_ `function`

#### `fio_queue_new`

```c
fio_queue_s *fio_queue_new(void)
```

Creates a new queue object (allocated on the heap).

_Symbol type:_ `function`

#### `fio_queue_free`

```c
void fio_queue_free(fio_queue_s *q)
```

Frees a queue object after calling fio_queue_destroy.

_Symbol type:_ `function`

#### `fio_queue_push`

```c
int fio_queue_push(fio_queue_s *q, fio_queue_task_s task)
```

Pushes a task to the queue. Returns -1 on error.

_Symbol type:_ `function`

#### `fio_queue_push`

```c
#define fio_queue_push(q, ...)   \
  fio_queue_push((q), (fio_queue_task_s){__VA_ARGS__})
```

Pushes a task to the queue, offering named arguments for the task.
Returns -1 on error.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_queue_push_urgent`

```c
int fio_queue_push_urgent(fio_queue_s *q, fio_queue_task_s task)
```

Pushes a task to the head of the queue. Returns -1 on error (no memory).

_Symbol type:_ `function`

#### `fio_queue_push_urgent`

```c
#define fio_queue_push_urgent(q, ...)   \
  fio_queue_push_urgent((q), (fio_queue_task_s){__VA_ARGS__})
```

Pushes a task to the queue, offering named arguments for the task.
Returns -1 on error.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_queue_pop`

```c
fio_queue_task_s fio_queue_pop(fio_queue_s *q)
```

Pops a task from the queue (FIFO). Returns a NULL task on error.

_Symbol type:_ `function`

#### `fio_queue_perform`

```c
int fio_queue_perform(fio_queue_s *q)
```

Performs a task from the queue. Returns -1 on error (queue empty).

_Symbol type:_ `function`

#### `fio_queue_perform_all`

```c
void fio_queue_perform_all(fio_queue_s *q)
```

Performs all tasks in the queue.

_Symbol type:_ `function`

#### `fio_queue_count`

```c
inline uint32_t fio_queue_count(fio_queue_s *q)
```

returns the number of tasks in the queue.

_Symbol type:_ `function`

#### `fio_queue_workers_add`

```c
int fio_queue_workers_add(fio_queue_s *q, size_t count)
```

Adds worker / consumer threads to perform the jobs in the queue.

_Symbol type:_ `function`

#### `fio_queue_workers_stop`

```c
void fio_queue_workers_stop(fio_queue_s *q)
```

Signals all worker threads to stop performing tasks and terminate.

_Symbol type:_ `function`

#### `fio_queue_workers_join`

```c
void fio_queue_workers_join(fio_queue_s *q)
```

Signals all worker threads to stop, waiting for them to complete.

_Symbol type:_ `function`

#### `fio_queue_workers_wake`

```c
void fio_queue_workers_wake(fio_queue_s *q)
```

Signals all worker threads to go back to work (new tasks added).

_Symbol type:_ `function`

#### `fio_timer_schedule`

```c
void fio_timer_schedule(fio_timer_queue_s *timer_queue, fio_timer_schedule_args_s args)
```

Adds a time-bound event to the timer queue.

_Symbol type:_ `function`

#### `fio_timer_schedule`

```c
#define fio_timer_schedule(timer_queue, ...)   \
  fio_timer_schedule((timer_queue), (fio_timer_schedule_args_s){__VA_ARGS__})
```

A MACRO allowing named arguments to be used. See fio_timer_schedule_args_s.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_timer_push2queue`

```c
size_t fio_timer_push2queue(fio_queue_s *queue, fio_timer_queue_s *timer_queue, int64_t now_in_milliseconds)
```

Pushes due events from the timer queue to an event queue.

_Symbol type:_ `function`

#### `fio_timer_next_at`

```c
inline int64_t fio_timer_next_at(fio_timer_queue_s *timer_queue)
```



_Symbol type:_ `function`

#### `fio_timer_destroy`

```c
void fio_timer_destroy(fio_timer_queue_s *timer_queue)
```

Clears any waiting timer bound tasks.

NOTE:

The timer queue must NEVER be freed when there's a chance that timer tasks
are waiting to be performed in a `fio_queue_s`.

This is due to the fact that the tasks may try to reschedule themselves (if
they repeat).

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-102-stream-h"></a> `./fio-stl/102 stream.h`

16 public symbols.

### Macros

#### `FIO_STREAM_COPY_PER_PACKET`

```c
#define FIO_STREAM_COPY_PER_PACKET 98304
```

Break apart large memory blocks into smaller pieces. by default 96Kb

_Symbol type:_ `macro`

#### `FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN`

```c
#define FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN 116
```

If the data added is less than said bytes, copy is preferred (locality).

_Symbol type:_ `macro`

#### `FIO_STREAM_INIT`

```c
#define FIO_STREAM_INIT(s)   \
  { .next = NULL, .pos = &(s).next }
```



_Symbol type:_ `macro`

### Types

#### `fio_stream_packet_s`

```c
struct fio_stream_packet_s
```



_Symbol type:_ `type`

#### `fio_stream_s`

```c
typedef struct {
/* do not directly access! */
fio_stream_packet_s *next;
fio_stream_packet_s **pos;
size_t consumed;
size_t length;
} fio_stream_s
```



_Symbol type:_ `type`

### Functions

#### `fio_stream_new`

```c
inline fio_stream_s *fio_stream_new(void)
```



_Symbol type:_ `function`

#### `fio_stream_free`

```c
inline int fio_stream_free(fio_stream_s *stream)
```



_Symbol type:_ `function`

#### `fio_stream_destroy`

```c
void fio_stream_destroy(fio_stream_s *stream)
```

Destroys the object, re-initializing its container.

_Symbol type:_ `function`

#### `fio_stream_pack_data`

```c
fio_stream_packet_s *fio_stream_pack_data(void *buf, size_t len, size_t offset, uint8_t copy_buffer, void (*dealloc_func)(void *))
```

Packs data into a fio_stream_packet_s container.

_Symbol type:_ `function`

#### `fio_stream_pack_fd`

```c
fio_stream_packet_s *fio_stream_pack_fd(int fd, size_t len, size_t offset, uint8_t keep_open)
```

Packs a file descriptor into a fio_stream_packet_s container.

_Symbol type:_ `function`

#### `fio_stream_add`

```c
void fio_stream_add(fio_stream_s *stream, fio_stream_packet_s *packet)
```

Adds a packet to the stream. This isn't thread safe.

_Symbol type:_ `function`

#### `fio_stream_pack_free`

```c
void fio_stream_pack_free(fio_stream_packet_s *packet)
```

Destroys the fio_stream_packet_s - call this ONLY if unused.

_Symbol type:_ `function`

#### `fio_stream_read`

```c
void fio_stream_read(fio_stream_s *stream, char **buf, size_t *len)
```

Reads data from the stream (if any), leaving it in the stream.

`buf` MUST point to a buffer with - at least - `len` bytes. This is required
in case the packed data is fragmented or references a file and needs to be
copied to an available buffer.

On error, or if the stream is empty, `buf` will be set to NULL and `len` will
be set to zero.

Otherwise, `buf` may retain the same value or it may point directly to a
memory address within the stream's buffer (the original value may be lost)
and `len` will be updated to the largest possible value for valid data that
can be read from `buf`.

Note: this isn't thread safe.

_Symbol type:_ `function`

#### `fio_stream_advance`

```c
void fio_stream_advance(fio_stream_s *stream, size_t len)
```

Advances the Stream, so the first `len` bytes are marked as consumed.

Note: this isn't thread safe.

_Symbol type:_ `function`

#### `fio_stream_any`

```c
inline uint8_t fio_stream_any(fio_stream_s *stream)
```

Returns true if there's any data in the stream.

Note: this isn't truly thread safe.

_Symbol type:_ `function`

#### `fio_stream_length`

```c
inline size_t fio_stream_length(fio_stream_s *stream)
```

Returns the number of bytes waiting in the stream.

Note: this isn't truly thread safe.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-103-md2html-h"></a> `./fio-stl/103 md2html.h`

2 public symbols.

### Macros

#### `FIO_MD2HTML`

```c
#define FIO_MD2HTML            /* Development inclusion - ignore line */
```



_Symbol type:_ `macro`

#### `FIO_MD2HTML_ERR_ALLOC`

```c
#define FIO_MD2HTML_ERR_ALLOC 1
```

Markdown-to-HTML renderer callback error: output allocation failed.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-104-mustache-h"></a> `./fio-stl/104 mustache.h`

14 public symbols.

### Macros

#### `FIO_MUSTACHE_MAX_DEPTH`

```c
#define FIO_MUSTACHE_MAX_DEPTH 128
```

The maximum depth of a template's context

_Symbol type:_ `macro`

#### `FIO_MUSTACHE_PRESERVE_PADDING`

```c
#define FIO_MUSTACHE_PRESERVE_PADDING 0
```

Preserves padding for stand-alone variables and partial templates

_Symbol type:_ `macro`

#### `FIO_MUSTACHE_LAMBDA_SUPPORT`

```c
#define FIO_MUSTACHE_LAMBDA_SUPPORT 0
```

Supports raw text for lambda style languages.

_Symbol type:_ `macro`

#### `FIO_MUSTACHE_ISOLATE_PARTIALS`

```c
#define FIO_MUSTACHE_ISOLATE_PARTIALS 1
```

Limits the scope of partial templates to the context of their section.

_Symbol type:_ `macro`

#### `FIO_MUSTACHE_SECURE_PATH`

```c
#define FIO_MUSTACHE_SECURE_PATH 1
```



_Symbol type:_ `macro`

### Types

#### `fio_mustache_s`

```c
struct fio_mustache_s
```



_Symbol type:_ `type`

#### `fio_mustache_bargs_s`

```c
struct fio_mustache_bargs_s
```



_Symbol type:_ `type`

#### `fio_mustache_load_args_s`

```c
typedef struct {
/** The file's content (if pre-loaded) */
fio_buf_info_s data;
/** The file's name (even if preloaded, used for partials load paths) */
fio_buf_info_s filename;
/** Loads the file's content, returning a `fio_buf_info_s` structure. */
fio_buf_info_s (*load_file_data)(fio_buf_info_s filename, void *udata);
/** Frees the file's content from its `fio_buf_info_s` structure. */
void (*free_file_data)(fio_buf_info_s file_data, void *udata);
/** Called when YAML front matter data was found. */
void (*on_yaml_front_matter)(fio_buf_info_s yaml_front_matter, void *udata);
/** Opaque user data. */
void *udata;
} fio_mustache_load_args_s
```



_Symbol type:_ `type`

### Functions

#### `fio_mustache_load`

```c
fio_mustache_s *fio_mustache_load(fio_mustache_load_args_s settings)
```



_Symbol type:_ `function`

#### `fio_mustache_load`

```c
#define fio_mustache_load(...)   \
  fio_mustache_load((fio_mustache_load_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_mustache_free`

```c
void fio_mustache_free(fio_mustache_s *m)
```



_Symbol type:_ `function`

#### `fio_mustache_dup`

```c
fio_mustache_s *fio_mustache_dup(fio_mustache_s *m)
```

Increases the mustache template's reference count.

_Symbol type:_ `function`

#### `fio_mustache_build`

```c
void *fio_mustache_build(fio_mustache_s *m, fio_mustache_bargs_s)
```

Builds the template, returning the final value of `udata` (or NULL).

_Symbol type:_ `function`

#### `fio_mustache_build`

```c
#define fio_mustache_build(m, ...)   \
  fio_mustache_build((m), ((fio_mustache_bargs_s){__VA_ARGS__}))
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-150-crypto-core-h"></a> `./fio-stl/150 crypto core.h`

2 public symbols.

### Types

#### `fio_crypto_enc_fn`

```c
typedef void(fio_crypto_enc_fn)(void *restrict mac, void *restrict data, size_t len, const void *ad, /* additional data */ size_t adlen, const void *key, const void *nonce)
```



_Symbol type:_ `type`

#### `fio_crypto_dec_fn`

```c
typedef int(fio_crypto_dec_fn)(void *restrict mac, void *restrict data, size_t len, const void *ad, /* additional data */ size_t adlen, const void *key, const void *nonce)
```



_Symbol type:_ `type`

-----------------------------------------------------

## <a id="fio-stl-152-blake2-h"></a> `./fio-stl/152 blake2.h`

14 public symbols.

### Types

#### `fio_blake2b_s`

```c
typedef struct {
uint64_t h[8]; /* state */
uint64_t t[2]; /* total bytes processed (128-bit counter) */
uint64_t f[2]; /* finalization flags */
uint8_t buf[128]; /* input buffer */
size_t buflen; /* bytes in buffer */
size_t outlen; /* digest length */
} fio_blake2b_s
```

Streaming BLAKE2b type (64-bit, up to 64-byte digest).

_Symbol type:_ `type`

#### `fio_blake2s_s`

```c
typedef struct {
uint32_t h[8]; /* state */
uint32_t t[2]; /* total bytes processed (64-bit counter) */
uint32_t f[2]; /* finalization flags */
uint8_t buf[64]; /* input buffer */
size_t buflen; /* bytes in buffer */
size_t outlen; /* digest length */
} fio_blake2s_s
```

Streaming BLAKE2s type (32-bit, up to 32-byte digest).

_Symbol type:_ `type`

### Functions

#### `fio_blake2b`

```c
inline fio_u512 fio_blake2b(const void *data, uint64_t len)
```

One-shot BLAKE2b hash with max-length (64 byte) digest.

_Symbol type:_ `function`

#### `fio_blake2b_hash`

```c
void fio_blake2b_hash(void *restrict out, size_t outlen, const void *restrict data, size_t len, const void *restrict key, size_t keylen)
```

Flexible-output BLAKE2b hash.

`out` must point to a buffer of at least `outlen` bytes.
`outlen` must be between 1 and 64 (default 64 if 0).
`key` and `keylen` are optional (set to NULL/0 for unkeyed hashing).

_Symbol type:_ `function`

#### `fio_blake2b_hmac`

```c
fio_u512 fio_blake2b_hmac(const void *key, uint64_t key_len, const void *msg, uint64_t msg_len)
```

HMAC-BLAKE2b (64 byte key, 64 byte digest), compatible with SHA HMAC.

_Symbol type:_ `function`

#### `fio_blake2b_init`

```c
fio_blake2b_s fio_blake2b_init(size_t outlen, const void *key, size_t keylen)
```

Initialize a BLAKE2b streaming context. outlen: 1-64 (default 64).

_Symbol type:_ `function`

#### `fio_blake2b_consume`

```c
void fio_blake2b_consume(fio_blake2b_s *restrict h, const void *restrict data, size_t len)
```

Feed data into BLAKE2b hash.

_Symbol type:_ `function`

#### `fio_blake2b_finalize`

```c
fio_u512 fio_blake2b_finalize(fio_blake2b_s *h)
```

Finalize BLAKE2b hash. Returns result in fio_u512 (valid bytes = outlen).

_Symbol type:_ `function`

#### `fio_blake2s`

```c
inline fio_u256 fio_blake2s(const void *data, uint64_t len)
```

One-shot BLAKE2s hash with max-length (32 byte) digest.

_Symbol type:_ `function`

#### `fio_blake2s_hash`

```c
void fio_blake2s_hash(void *restrict out, size_t outlen, const void *restrict data, size_t len, const void *restrict key, size_t keylen)
```

Flexible-output BLAKE2s hash.

`out` must point to a buffer of at least `outlen` bytes.
`outlen` must be between 1 and 32 (default 32 if 0).
`key` and `keylen` are optional (set to NULL/0 for unkeyed hashing).

_Symbol type:_ `function`

#### `fio_blake2s_hmac`

```c
fio_u256 fio_blake2s_hmac(const void *key, uint64_t key_len, const void *msg, uint64_t msg_len)
```

HMAC-BLAKE2s (32 byte key, 32 byte digest), compatible with SHA HMAC.

_Symbol type:_ `function`

#### `fio_blake2s_init`

```c
fio_blake2s_s fio_blake2s_init(size_t outlen, const void *key, size_t keylen)
```

Initialize a BLAKE2s streaming context. outlen: 1-32 (default 32).

_Symbol type:_ `function`

#### `fio_blake2s_consume`

```c
void fio_blake2s_consume(fio_blake2s_s *restrict h, const void *restrict data, size_t len)
```

Feed data into BLAKE2s hash.

_Symbol type:_ `function`

#### `fio_blake2s_finalize`

```c
fio_u256 fio_blake2s_finalize(fio_blake2s_s *h)
```

Finalize BLAKE2s hash. Returns result in fio_u256 (valid bytes = outlen).

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-152-chacha20poly1305-h"></a> `./fio-stl/152 chacha20poly1305.h`

7 public symbols.

### Functions

#### `fio_chacha20_poly1305_enc`

```c
void fio_chacha20_poly1305_enc(void *restrict mac, void *restrict data, size_t len, const void *ad, /* additional data */ size_t adlen, const void *key, const void *nonce)
```

Performs an in-place encryption of `data` using ChaCha20 with additional
data, producing a 16 byte message authentication code (MAC) using Poly1305.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce` MUST point to a  96 bit long memory address (12 Bytes).
* `ad`     MAY be omitted, will NOT be encrypted.
* `data`   MAY be omitted, WILL be encrypted.
* `mac`    MUST point to a buffer with (at least) 16 available bytes.

_Symbol type:_ `function`

#### `fio_chacha20_poly1305_dec`

```c
int fio_chacha20_poly1305_dec(void *restrict mac, void *restrict data, size_t len, const void *ad, /* additional data */ size_t adlen, const void *key, const void *nonce)
```

Performs an in-place decryption of `data` using ChaCha20 after authenticating
the message authentication code (MAC) using Poly1305.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce` MUST point to a  96 bit long memory address (12 Bytes).
* `ad`     MAY be omitted ONLY IF originally omitted.
* `data`   MAY be omitted, WILL be decrypted.
* `mac`    MUST point to a buffer where the 16 byte MAC is placed.

Returns `-1` on error (authentication failed).

_Symbol type:_ `function`

#### `fio_xchacha20_poly1305_enc`

```c
void fio_xchacha20_poly1305_enc(void *restrict mac, void *restrict data, size_t len, const void *ad, /* additional data */ size_t adlen, const void *key, const void *nonce)
```

Performs an in-place encryption of `data` using XChaCha20 with additional
data, producing a 16 byte message authentication code (MAC) using Poly1305.

XChaCha20 uses a 192-bit (24 byte) nonce, making it safe to use with
randomly-generated nonces without risk of nonce collision.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`  MUST point to a 192 bit long memory address (24 Bytes).
* `ad`     MAY be omitted, will NOT be encrypted.
* `data`   MAY be omitted, WILL be encrypted.
* `mac`    MUST point to a buffer with (at least) 16 available bytes.

_Symbol type:_ `function`

#### `fio_xchacha20_poly1305_dec`

```c
int fio_xchacha20_poly1305_dec(void *restrict mac, void *restrict data, size_t len, const void *ad, /* additional data */ size_t adlen, const void *key, const void *nonce)
```

Performs an in-place decryption of `data` using XChaCha20 after
authenticating the message authentication code (MAC) using Poly1305.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`  MUST point to a 192 bit long memory address (24 Bytes).
* `ad`     MAY be omitted ONLY IF originally omitted.
* `data`   MAY be omitted, WILL be decrypted.
* `mac`    MUST point to a buffer where the 16 byte MAC is placed.

Returns `-1` on error (authentication failed).

_Symbol type:_ `function`

#### `fio_xchacha20`

```c
void fio_xchacha20(void *restrict data, size_t len, const void *key, const void *nonce, uint32_t counter)
```

Performs an in-place encryption/decryption of `data` using XChaCha20.

XChaCha20 uses a 192-bit (24 byte) nonce, making it safe to use with
randomly-generated nonces without risk of nonce collision.

* `key`     MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`   MUST point to a 192 bit long memory address (24 Bytes).
* `counter` is the block counter, usually 0 unless `data` is mid-cyphertext.

_Symbol type:_ `function`

#### `fio_chacha20`

```c
void fio_chacha20(void *restrict data, size_t len, const void *key, const void *nonce, uint32_t counter)
```

Performs an in-place encryption/decryption of `data` using ChaCha20.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce` MUST point to a  96 bit long memory address (12 Bytes).
* `counter` is the block counter, usually 1 unless `data` is mid-cyphertext.

_Symbol type:_ `function`

#### `fio_poly1305_auth`

```c
void fio_poly1305_auth(void *restrict mac_dest, void *restrict message, size_t len, const void *ad, size_t ad_len, const void *key256bits)
```

Given a Poly1305 256bit (32 byte) key, writes the authentication code for the
poly message and additional data into `mac_dest`.

* `mac_dest` MUST point to a buffer with (at least) 16 available bytes.
* `message`  MAY be omitted.
* `ad`       MAY be omitted (additional data).
* `key`      MUST point to a 256 bit long memory address (32 Bytes).

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-152-sha1-h"></a> `./fio-stl/152 sha1.h`

4 public symbols.

### Types

#### `fio_sha1_s`

```c
typedef union {
#ifdef __SIZEOF_INT128__
__uint128_t align__;
#else
uint64_t align__;
#endif
uint32_t v[5];
uint8_t digest[20];
} fio_sha1_s
```

The data type containing the SHA1 digest (result).

_Symbol type:_ `type`

### Functions

#### `fio_sha1`

```c
fio_sha1_s fio_sha1(const void *data, uint64_t len)
```

A simple, non streaming, implementation of the SHA1 hashing algorithm.

Do NOT use - SHA1 is broken... but for some reason some protocols still
require it's use (i.e., WebSockets), so it's here for your convenience.

_Symbol type:_ `function`

#### `fio_sha1_len`

```c
inline size_t fio_sha1_len(void)
```

Returns the digest length of SHA1 in bytes (20 bytes)

_Symbol type:_ `function`

#### `fio_sha1_digest`

```c
inline uint8_t *fio_sha1_digest(fio_sha1_s *s)
```

Returns the 20 Byte long digest of a SHA1 object.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-152-sha2-h"></a> `./fio-stl/152 sha2.h`

10 public symbols.

### Types

#### `fio_sha256_s`

```c
typedef struct {
fio_u256 hash;
fio_u512 cache;
uint64_t total_len;
} fio_sha256_s
```

Streaming SHA-256 type.

_Symbol type:_ `type`

#### `fio_sha512_s`

```c
typedef struct {
fio_u512 hash;
fio_u1024 cache;
uint64_t total_len;
} fio_sha512_s
```

Streaming SHA-512 type.

_Symbol type:_ `type`

### Functions

#### `fio_sha256`

```c
inline fio_u256 fio_sha256(const void *data, uint64_t len)
```

A simple, non streaming, implementation of the SHA-256 hashing algorithm.

_Symbol type:_ `function`

#### `fio_sha256_init`

```c
inline fio_sha256_s fio_sha256_init(void)
```

initializes a fio_u256 so the hash can consume streaming data.

_Symbol type:_ `function`

#### `fio_sha256_consume`

```c
void fio_sha256_consume(fio_sha256_s *h, const void *data, uint64_t len)
```

Feed data into the hash

_Symbol type:_ `function`

#### `fio_sha256_finalize`

```c
fio_u256 fio_sha256_finalize(fio_sha256_s *h)
```

finalizes a fio_u256 with the SHA 256 hash.

_Symbol type:_ `function`

#### `fio_sha512`

```c
inline fio_u512 fio_sha512(const void *data, uint64_t len)
```

A simple, non streaming, implementation of the SHA-512 hashing algorithm.

_Symbol type:_ `function`

#### `fio_sha512_init`

```c
inline fio_sha512_s fio_sha512_init(void)
```

initializes a fio_u512 so the hash can consume streaming data.

_Symbol type:_ `function`

#### `fio_sha512_consume`

```c
void fio_sha512_consume(fio_sha512_s *h, const void *data, uint64_t len)
```

Feed data into the hash

_Symbol type:_ `function`

#### `fio_sha512_finalize`

```c
fio_u512 fio_sha512_finalize(fio_sha512_s *h)
```

finalizes a fio_u512 with the SHA 512 hash.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-152-sha2z-hkdf-h"></a> `./fio-stl/152 sha2z hkdf.h`

5 public symbols.

### Macros

#### `FIO_HKDF_SHA256_HASH_LEN`

```c
#define FIO_HKDF_SHA256_HASH_LEN 32
```

SHA-256 hash length (32 bytes).

_Symbol type:_ `macro`

#### `FIO_HKDF_SHA384_HASH_LEN`

```c
#define FIO_HKDF_SHA384_HASH_LEN 48
```

SHA-384 hash length (48 bytes).

_Symbol type:_ `macro`

### Functions

#### `fio_hkdf_extract`

```c
void fio_hkdf_extract(void *restrict prk, const void *restrict salt, size_t salt_len, const void *restrict ikm, size_t ikm_len, int use_sha384)
```

HKDF-Extract: PRK = HMAC-Hash(salt, IKM)

Extracts a pseudorandom key (PRK) from input keying material (IKM).

**Parameters:**
- `prk` - Output buffer (32 bytes for SHA-256, 48 for SHA-384)
- `salt` - Optional salt (if NULL, uses zeros of hash length)
- `salt_len` - Salt length in bytes
- `ikm` - Input keying material
- `ikm_len` - IKM length in bytes
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_hkdf_expand`

```c
void fio_hkdf_expand(void *restrict okm, size_t okm_len, const void *restrict prk, size_t prk_len, const void *restrict info, size_t info_len, int use_sha384)
```

HKDF-Expand: OKM = HKDF-Expand(PRK, info, L)

Expands a pseudorandom key (PRK) into output keying material (OKM).

**Parameters:**
- `okm` - Output keying material buffer
- `okm_len` - Desired output length (max 255 * hash_len)
- `prk` - Pseudorandom key from Extract (32 or 48 bytes)
- `prk_len` - PRK length (32 for SHA-256, 48 for SHA-384)
- `info` - Optional context/application info
- `info_len` - Info length in bytes
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_hkdf`

```c
void fio_hkdf(void *restrict okm, size_t okm_len, const void *restrict salt, size_t salt_len, const void *restrict ikm, size_t ikm_len, const void *restrict info, size_t info_len, int use_sha384)
```

Combined HKDF (Extract + Expand) - RFC 5869 Section 2.

Derives keying material from input keying material using HKDF.

**Parameters:**
- `okm` - Output keying material buffer
- `okm_len` - Desired output length (max 255 * hash_len)
- `salt` - Optional salt (if NULL, uses zeros of hash length)
- `salt_len` - Salt length in bytes
- `ikm` - Input keying material
- `ikm_len` - IKM length in bytes
- `info` - Optional context/application info
- `info_len` - Info length in bytes
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-152-sha3-h"></a> `./fio-stl/152 sha3.h`

17 public symbols.

### Types

#### `fio_sha3_s`

```c
typedef struct {
uint64_t state[25];
uint8_t buf[200]; /* rate buffer (max rate = 1600 - 2*capacity) */
size_t buflen; /* bytes in buffer */
size_t rate; /* rate in bytes */
size_t outlen; /* output length in bytes (0 for SHAKE) */
uint8_t delim; /* domain separation byte */
} fio_sha3_s
```

Keccak state (1600 bits = 200 bytes = 25 x 64-bit words)

_Symbol type:_ `type`

### Functions

#### `fio_sha3_224_init`

```c
inline fio_sha3_s fio_sha3_224_init(void)
```

Initialize SHA3-224 (28-byte output).

_Symbol type:_ `function`

#### `fio_sha3_256_init`

```c
inline fio_sha3_s fio_sha3_256_init(void)
```

Initialize SHA3-256 (32-byte output).

_Symbol type:_ `function`

#### `fio_sha3_384_init`

```c
inline fio_sha3_s fio_sha3_384_init(void)
```

Initialize SHA3-384 (48-byte output).

_Symbol type:_ `function`

#### `fio_sha3_512_init`

```c
inline fio_sha3_s fio_sha3_512_init(void)
```

Initialize SHA3-512 (64-byte output).

_Symbol type:_ `function`

#### `fio_sha3_consume`

```c
void fio_sha3_consume(fio_sha3_s *restrict h, const void *restrict data, size_t len)
```

Feed data into SHA3 hash.

_Symbol type:_ `function`

#### `fio_sha3_finalize`

```c
void fio_sha3_finalize(fio_sha3_s *restrict h, void *restrict out)
```

Finalize SHA3 hash. Writes h->outlen bytes to out.

_Symbol type:_ `function`

#### `fio_sha3_224`

```c
void fio_sha3_224(void *restrict out, const void *restrict data, size_t len)
```

Simple SHA3-224 (28-byte output).

_Symbol type:_ `function`

#### `fio_sha3_256`

```c
void fio_sha3_256(void *restrict out, const void *restrict data, size_t len)
```

Simple SHA3-256 (32-byte output).

_Symbol type:_ `function`

#### `fio_sha3_384`

```c
void fio_sha3_384(void *restrict out, const void *restrict data, size_t len)
```

Simple SHA3-384 (48-byte output).

_Symbol type:_ `function`

#### `fio_sha3_512`

```c
void fio_sha3_512(void *restrict out, const void *restrict data, size_t len)
```

Simple SHA3-512 (64-byte output).

_Symbol type:_ `function`

#### `fio_shake128_init`

```c
inline fio_sha3_s fio_shake128_init(void)
```

Initialize SHAKE128 (variable output length).

_Symbol type:_ `function`

#### `fio_shake256_init`

```c
inline fio_sha3_s fio_shake256_init(void)
```

Initialize SHAKE256 (variable output length).

_Symbol type:_ `function`

#### `fio_shake_consume`

```c
#define fio_shake_consume fio_sha3_consume
```

Feed data into SHAKE.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_shake_squeeze`

```c
void fio_shake_squeeze(fio_sha3_s *restrict h, void *restrict out, size_t outlen)
```

Squeeze output from SHAKE. Can be called multiple times.

_Symbol type:_ `function`

#### `fio_shake128`

```c
void fio_shake128(void *restrict out, size_t outlen, const void *restrict data, size_t len)
```

Simple SHAKE128 with specified output length.

_Symbol type:_ `function`

#### `fio_shake256`

```c
void fio_shake256(void *restrict out, size_t outlen, const void *restrict data, size_t len)
```

Simple SHAKE256 with specified output length.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-153-aes-h"></a> `./fio-stl/153 aes.h`

4 public symbols.

### Functions

#### `fio_aes128_gcm_enc`

```c
void fio_aes128_gcm_enc(void *restrict mac, void *restrict data, size_t len, const void *ad, size_t adlen, const void *key, const void *nonce)
```

Performs in-place AES-128-GCM encryption with authentication.

* `mac`    MUST point to a buffer with (at least) 16 available bytes.
* `key`    MUST point to a 128 bit key (16 Bytes).
* `nonce`  MUST point to a 96 bit nonce (12 Bytes).
* `ad`     MAY be omitted, will NOT be encrypted.
* `data`   MAY be omitted, WILL be encrypted.

_Symbol type:_ `function`

#### `fio_aes128_gcm_dec`

```c
int fio_aes128_gcm_dec(void *restrict mac, void *restrict data, size_t len, const void *ad, size_t adlen, const void *key, const void *nonce)
```

Performs in-place AES-128-GCM decryption with authentication verification.

* `mac`    MUST point to a buffer with the 16 byte MAC to verify.
* `key`    MUST point to a 128 bit key (16 Bytes).
* `nonce`  MUST point to a 96 bit nonce (12 Bytes).
* `ad`     MAY be omitted ONLY IF originally omitted.
* `data`   MAY be omitted, WILL be decrypted.

Returns `-1` on error (authentication failed).

_Symbol type:_ `function`

#### `fio_aes256_gcm_enc`

```c
void fio_aes256_gcm_enc(void *restrict mac, void *restrict data, size_t len, const void *ad, size_t adlen, const void *key, const void *nonce)
```

Performs in-place AES-256-GCM encryption with authentication.

* `mac`    MUST point to a buffer with (at least) 16 available bytes.
* `key`    MUST point to a 256 bit key (32 Bytes).
* `nonce`  MUST point to a 96 bit nonce (12 Bytes).
* `ad`     MAY be omitted, will NOT be encrypted.
* `data`   MAY be omitted, WILL be encrypted.

_Symbol type:_ `function`

#### `fio_aes256_gcm_dec`

```c
int fio_aes256_gcm_dec(void *restrict mac, void *restrict data, size_t len, const void *ad, size_t adlen, const void *key, const void *nonce)
```

Performs in-place AES-256-GCM decryption with authentication verification.

* `mac`    MUST point to a buffer with the 16 byte MAC to verify.
* `key`    MUST point to a 256 bit key (32 Bytes).
* `nonce`  MUST point to a 96 bit nonce (12 Bytes).
* `ad`     MAY be omitted ONLY IF originally omitted.
* `data`   MAY be omitted, WILL be decrypted.

Returns `-1` on error (authentication failed).

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-154-ed25519-h"></a> `./fio-stl/154 ed25519.h`

13 public symbols.

### Macros

#### `FIO_X25519_CIPHERTEXT_LEN`

```c
#define FIO_X25519_CIPHERTEXT_LEN(message_len) ((message_len) + 48)
```

Returns the ciphertext length for a given plaintext length.
Ciphertext = ephemeral_pk (32) + mac (16) + encrypted_message (message_len)

_Symbol type:_ `macro`

#### `FIO_X25519_PLAINTEXT_LEN`

```c
#define FIO_X25519_PLAINTEXT_LEN(ciphertext_len)   \
  ((ciphertext_len) > 48 ? ((ciphertext_len)-48) : 0)
```

Returns the plaintext length for a given ciphertext length.
Returns 0 if ciphertext_len < 48 (invalid ciphertext).

_Symbol type:_ `macro`

### Functions

#### `fio_ed25519_keypair`

```c
void fio_ed25519_keypair(uint8_t secret_key[32], uint8_t public_key[32])
```

Generates a new random Ed25519 key pair.

The secret key must be kept secret and securely erased when no longer
needed. The public key can be freely shared.

_Symbol type:_ `function`

#### `fio_ed25519_public_key`

```c
void fio_ed25519_public_key(uint8_t public_key[32], const uint8_t secret_key[32])
```

Derives the public key from an Ed25519 secret key.

Useful when the secret key is loaded from storage and the public key
needs to be recomputed.

_Symbol type:_ `function`

#### `fio_ed25519_sign`

```c
int fio_ed25519_sign(uint8_t signature[64], const void *message, size_t len, const uint8_t secret_key[32], const uint8_t public_key[32])
```

Signs a message using Ed25519.

The signature is 64 bytes and is deterministic (same message + key = same
signature).

_Symbol type:_ `function`

#### `fio_ed25519_verify`

```c
int fio_ed25519_verify(const uint8_t signature[64], const void *message, size_t len, const uint8_t public_key[32])
```

Verifies an Ed25519 signature.

Returns 0 on success (valid signature), -1 on failure (invalid signature).

_Symbol type:_ `function`

#### `fio_x25519_keypair`

```c
void fio_x25519_keypair(uint8_t secret_key[32], uint8_t public_key[32])
```

Generates a new random X25519 key pair.

The secret key must be kept secret. The public key can be shared with
the other party for key exchange.

_Symbol type:_ `function`

#### `fio_x25519_public_key`

```c
void fio_x25519_public_key(uint8_t public_key[32], const uint8_t secret_key[32])
```

Derives the public key from an X25519 secret key.

This performs scalar multiplication of the secret key with the base point.

_Symbol type:_ `function`

#### `fio_x25519_shared_secret`

```c
int fio_x25519_shared_secret(uint8_t shared_secret[32], const uint8_t secret_key[32], const uint8_t their_public_key[32])
```

Computes a shared secret using X25519 (ECDH).

Both parties compute the same shared secret:
  shared = X25519(my_secret, their_public)

The shared secret should be passed through a KDF (e.g., HKDF with SHA-256)
before being used as an encryption key.

Returns 0 on success, -1 on failure (e.g., if their_public is a low-order
point, which would result in an all-zero shared secret).

_Symbol type:_ `function`

#### `fio_ed25519_sk_to_x25519`

```c
void fio_ed25519_sk_to_x25519(uint8_t x_secret_key[32], const uint8_t ed_secret_key[32])
```

Converts an Ed25519 secret key to an X25519 secret key.

This allows using an Ed25519 signing key for X25519 key exchange.

_Symbol type:_ `function`

#### `fio_ed25519_pk_to_x25519`

```c
void fio_ed25519_pk_to_x25519(uint8_t x_public_key[32], const uint8_t ed_public_key[32])
```

Converts an Ed25519 public key to an X25519 public key.

This allows encrypting to someone who has only shared their Ed25519
signing public key.

_Symbol type:_ `function`

#### `fio_x25519_encrypt`

```c
int fio_x25519_encrypt(uint8_t *ciphertext, const void *message, size_t message_len, fio_crypto_enc_fn encryption_function, const uint8_t recipient_pk[32])
```

Encrypts a message using the recipient's X25519 public key.

The ciphertext includes:
- 32 bytes: ephemeral public key (for key agreement)
- 16 bytes: authentication tag (MAC)
- N bytes:  encrypted message

Total ciphertext size = message_len + 48 bytes

**Parameters:**
- `ciphertext` - Output buffer (must be at least message_len + 48 bytes)
- `message` - The plaintext message to encrypt
- `message_len` - Length of the message
- `encryption_function` - Encryption function (fio_chacha20_poly1305_enc)
- `recipient_pk` - The recipient's X25519 public key (32 bytes)

**Returns:**
- 0 on success, -1 on failure

_Symbol type:_ `function`

#### `fio_x25519_decrypt`

```c
int fio_x25519_decrypt(uint8_t *plaintext, const uint8_t *ciphertext, size_t ciphertext_len, fio_crypto_dec_fn decryption_function, const uint8_t recipient_sk[32])
```

Decrypts a message using the recipient's X25519 secret key.

**Parameters:**
- `plaintext` - Output buffer (must be at least ciphertext_len - 48 bytes)
- `ciphertext` - The ciphertext (ephemeral_pk || mac || encrypted_data)
- `ciphertext_len` - Length of the ciphertext (must be >= 48)
- `decryption_function` - Decryption function (fio_chacha20_poly1305_dec)
- `recipient_sk` - The recipient's X25519 secret key (32 bytes)

**Returns:**
- 0 on success, -1 on failure (authentication failed or invalid input)

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-154-p256-h"></a> `./fio-stl/154 p256.h`

5 public symbols.

### Functions

#### `fio_ecdsa_p256_verify`

```c
int fio_ecdsa_p256_verify(const uint8_t *sig, size_t sig_len, const uint8_t *msg_hash, const uint8_t *pubkey, size_t pubkey_len)
```

Verifies an ECDSA P-256 signature.

**Parameters:**
- `sig` - DER-encoded signature (SEQUENCE { r INTEGER, s INTEGER })
- `sig_len` - Length of signature in bytes
- `msg_hash` - SHA-256 hash of the message (32 bytes)
- `pubkey` - Uncompressed public key (65 bytes: 0x04 || x || y)
- `pubkey_len` - Length of public key (must be 65)

**Returns:**
- 0 on success (valid signature), -1 on failure (invalid signature)

_Symbol type:_ `function`

#### `fio_ecdsa_p256_sign`

```c
int fio_ecdsa_p256_sign(uint8_t *sig, size_t *sig_len, size_t sig_capacity, const uint8_t msg_hash[32], const uint8_t secret_key[32])
```

Signs a message hash using ECDSA P-256.

**Parameters:**
- `sig` - Output: DER-encoded signature (max 72 bytes)
- `sig_len` - Output: actual signature length
- `sig_capacity` - Capacity of signature buffer (should be >= 72)
- `msg_hash` - SHA-256 hash of the message (32 bytes)
- `secret_key` - 32-byte secret key (scalar, big-endian)

**Returns:**
- 0 on success, -1 on failure

_Symbol type:_ `function`

#### `fio_ecdsa_p256_verify_raw`

```c
int fio_ecdsa_p256_verify_raw(const uint8_t r[32], const uint8_t s[32], const uint8_t msg_hash[32], const uint8_t pubkey_x[32], const uint8_t pubkey_y[32])
```

Verifies an ECDSA P-256 signature with raw r,s values.

**Parameters:**
- `r` - The r component of the signature (32 bytes, big-endian)
- `s` - The s component of the signature (32 bytes, big-endian)
- `msg_hash` - SHA-256 hash of the message (32 bytes)
- `pubkey_x` - X coordinate of public key (32 bytes, big-endian)
- `pubkey_y` - Y coordinate of public key (32 bytes, big-endian)

**Returns:**
- 0 on success (valid signature), -1 on failure (invalid signature)

_Symbol type:_ `function`

#### `fio_p256_keypair`

```c
int fio_p256_keypair(uint8_t secret_key[32], uint8_t public_key[65])
```

Generates a P-256 keypair for ECDHE key exchange.

**Parameters:**
- `secret_key` - Output: 32-byte secret key (scalar, big-endian)
- `public_key` - Output: 65-byte uncompressed public key (0x04 || x || y)

**Returns:**
- 0 on success, -1 on failure

_Symbol type:_ `function`

#### `fio_p256_shared_secret`

```c
int fio_p256_shared_secret(uint8_t shared_secret[32], const uint8_t secret_key[32], const uint8_t *their_public_key, size_t their_public_key_len)
```

Computes P-256 ECDH shared secret.

**Parameters:**
- `shared_secret` - Output: 32-byte shared secret (x-coordinate of result)
- `secret_key` - 32-byte secret key (scalar, big-endian)
- `their_public_key` - Their public key (uncompressed 65 bytes, or compressed 33 bytes)
- `their_public_key_len` - Length of their public key (33 or 65)

**Returns:**
- 0 on success, -1 on failure (invalid key, point at infinity, etc.)

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-154-p384-h"></a> `./fio-stl/154 p384.h`

2 public symbols.

### Functions

#### `fio_ecdsa_p384_verify`

```c
int fio_ecdsa_p384_verify(const uint8_t *sig, size_t sig_len, const uint8_t *msg_hash, const uint8_t *pubkey, size_t pubkey_len)
```

Verifies an ECDSA P-384 signature.

**Parameters:**
- `sig` - DER-encoded signature (SEQUENCE { r INTEGER, s INTEGER })
- `sig_len` - Length of signature in bytes
- `msg_hash` - SHA-384 hash of the message (48 bytes)
- `pubkey` - Uncompressed public key (97 bytes: 0x04 || x || y)
- `pubkey_len` - Length of public key (must be 97)

**Returns:**
- 0 on success (valid signature), -1 on failure (invalid signature)

_Symbol type:_ `function`

#### `fio_ecdsa_p384_verify_raw`

```c
int fio_ecdsa_p384_verify_raw(const uint8_t r[48], const uint8_t s[48], const uint8_t msg_hash[48], const uint8_t pubkey_x[48], const uint8_t pubkey_y[48])
```

Verifies an ECDSA P-384 signature with raw r,s values.

**Parameters:**
- `r` - The r component of the signature (48 bytes, big-endian)
- `s` - The s component of the signature (48 bytes, big-endian)
- `msg_hash` - SHA-384 hash of the message (48 bytes)
- `pubkey_x` - X coordinate of public key (48 bytes, big-endian)
- `pubkey_y` - Y coordinate of public key (48 bytes, big-endian)

**Returns:**
- 0 on success (valid signature), -1 on failure (invalid signature)

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-155-asn1-h"></a> `./fio-stl/155 asn1.h`

51 public symbols.

### Macros

#### `FIO_OID_SHA256_WITH_RSA`

```c
#define FIO_OID_SHA256_WITH_RSA   "1.2.840.113549.1.1.11"
```



_Symbol type:_ `macro`

#### `FIO_OID_SHA384_WITH_RSA`

```c
#define FIO_OID_SHA384_WITH_RSA   "1.2.840.113549.1.1.12"
```



_Symbol type:_ `macro`

#### `FIO_OID_SHA512_WITH_RSA`

```c
#define FIO_OID_SHA512_WITH_RSA   "1.2.840.113549.1.1.13"
```



_Symbol type:_ `macro`

#### `FIO_OID_RSA_PSS`

```c
#define FIO_OID_RSA_PSS           "1.2.840.113549.1.1.10"
```



_Symbol type:_ `macro`

#### `FIO_OID_ECDSA_WITH_SHA256`

```c
#define FIO_OID_ECDSA_WITH_SHA256 "1.2.840.10045.4.3.2"
```



_Symbol type:_ `macro`

#### `FIO_OID_ECDSA_WITH_SHA384`

```c
#define FIO_OID_ECDSA_WITH_SHA384 "1.2.840.10045.4.3.3"
```



_Symbol type:_ `macro`

#### `FIO_OID_ECDSA_WITH_SHA512`

```c
#define FIO_OID_ECDSA_WITH_SHA512 "1.2.840.10045.4.3.4"
```



_Symbol type:_ `macro`

#### `FIO_OID_ED25519`

```c
#define FIO_OID_ED25519           "1.3.101.112"
```



_Symbol type:_ `macro`

#### `FIO_OID_ED448`

```c
#define FIO_OID_ED448             "1.3.101.113"
```



_Symbol type:_ `macro`

#### `FIO_OID_RSA_ENCRYPTION`

```c
#define FIO_OID_RSA_ENCRYPTION "1.2.840.113549.1.1.1"
```



_Symbol type:_ `macro`

#### `FIO_OID_EC_PUBLIC_KEY`

```c
#define FIO_OID_EC_PUBLIC_KEY  "1.2.840.10045.2.1"
```



_Symbol type:_ `macro`

#### `FIO_OID_SECP256R1`

```c
#define FIO_OID_SECP256R1 "1.2.840.10045.3.1.7"
```



_Symbol type:_ `macro`

#### `FIO_OID_SECP384R1`

```c
#define FIO_OID_SECP384R1 "1.3.132.0.34"
```



_Symbol type:_ `macro`

#### `FIO_OID_SECP521R1`

```c
#define FIO_OID_SECP521R1 "1.3.132.0.35"
```



_Symbol type:_ `macro`

#### `FIO_OID_X25519`

```c
#define FIO_OID_X25519    "1.3.101.110"
```



_Symbol type:_ `macro`

#### `FIO_OID_X448`

```c
#define FIO_OID_X448      "1.3.101.111"
```



_Symbol type:_ `macro`

#### `FIO_OID_SUBJECT_KEY_ID`

```c
#define FIO_OID_SUBJECT_KEY_ID    "2.5.29.14"
```



_Symbol type:_ `macro`

#### `FIO_OID_KEY_USAGE`

```c
#define FIO_OID_KEY_USAGE         "2.5.29.15"
```



_Symbol type:_ `macro`

#### `FIO_OID_SUBJECT_ALT_NAME`

```c
#define FIO_OID_SUBJECT_ALT_NAME  "2.5.29.17"
```



_Symbol type:_ `macro`

#### `FIO_OID_BASIC_CONSTRAINTS`

```c
#define FIO_OID_BASIC_CONSTRAINTS "2.5.29.19"
```



_Symbol type:_ `macro`

#### `FIO_OID_CRL_DIST_POINTS`

```c
#define FIO_OID_CRL_DIST_POINTS   "2.5.29.31"
```



_Symbol type:_ `macro`

#### `FIO_OID_CERT_POLICIES`

```c
#define FIO_OID_CERT_POLICIES     "2.5.29.32"
```



_Symbol type:_ `macro`

#### `FIO_OID_AUTH_KEY_ID`

```c
#define FIO_OID_AUTH_KEY_ID       "2.5.29.35"
```



_Symbol type:_ `macro`

#### `FIO_OID_EXT_KEY_USAGE`

```c
#define FIO_OID_EXT_KEY_USAGE     "2.5.29.37"
```



_Symbol type:_ `macro`

#### `FIO_OID_EKU_SERVER_AUTH`

```c
#define FIO_OID_EKU_SERVER_AUTH "1.3.6.1.5.5.7.3.1"
```



_Symbol type:_ `macro`

#### `FIO_OID_EKU_CLIENT_AUTH`

```c
#define FIO_OID_EKU_CLIENT_AUTH "1.3.6.1.5.5.7.3.2"
```



_Symbol type:_ `macro`

#### `FIO_OID_COMMON_NAME`

```c
#define FIO_OID_COMMON_NAME  "2.5.4.3"
```



_Symbol type:_ `macro`

#### `FIO_OID_COUNTRY`

```c
#define FIO_OID_COUNTRY      "2.5.4.6"
```



_Symbol type:_ `macro`

#### `FIO_OID_LOCALITY`

```c
#define FIO_OID_LOCALITY     "2.5.4.7"
```



_Symbol type:_ `macro`

#### `FIO_OID_STATE`

```c
#define FIO_OID_STATE        "2.5.4.8"
```



_Symbol type:_ `macro`

#### `FIO_OID_ORGANIZATION`

```c
#define FIO_OID_ORGANIZATION "2.5.4.10"
```



_Symbol type:_ `macro`

#### `FIO_OID_ORG_UNIT`

```c
#define FIO_OID_ORG_UNIT     "2.5.4.11"
```



_Symbol type:_ `macro`

### Types

#### `fio_asn1_tag_e`

```c
typedef enum {
FIO_ASN1_EOC = 0x00, /**< End-of-contents */
FIO_ASN1_BOOLEAN = 0x01, /**< Boolean */
FIO_ASN1_INTEGER = 0x02, /**< Integer */
FIO_ASN1_BIT_STRING = 0x03, /**< Bit String */
FIO_ASN1_OCTET_STRING = 0x04, /**< Octet String */
FIO_ASN1_NULL = 0x05, /**< Null */
FIO_ASN1_OID = 0x06, /**< Object Identifier */
FIO_ASN1_OBJECT_DESCRIPTOR = 0x07, /**< Object Descriptor */
FIO_ASN1_EXTERNAL = 0x08, /**< External */
FIO_ASN1_REAL = 0x09, /**< Real (float) */
FIO_ASN1_ENUMERATED = 0x0A, /**< Enumerated */
FIO_ASN1_EMBEDDED_PDV = 0x0B, /**< Embedded PDV */
FIO_ASN1_UTF8_STRING = 0x0C, /**< UTF-8 String */
FIO_ASN1_RELATIVE_OID = 0x0D, /**< Relative OID */
FIO_ASN1_SEQUENCE = 0x10, /**< Sequence (0x30 with constructed bit) */
FIO_ASN1_SET = 0x11, /**< Set (0x31 with constructed bit) */
FIO_ASN1_NUMERIC_STRING = 0x12, /**< Numeric String */
FIO_ASN1_PRINTABLE_STRING = 0x13, /**< Printable String */
FIO_ASN1_T61_STRING = 0x14, /**< T61 String (Teletex) */
FIO_ASN1_VIDEOTEX_STRING = 0x15, /**< Videotex String */
FIO_ASN1_IA5_STRING = 0x16, /**< IA5 String (ASCII) */
FIO_ASN1_UTC_TIME = 0x17, /**< UTC Time */
FIO_ASN1_GENERALIZED_TIME = 0x18, /**< Generalized Time */
FIO_ASN1_GRAPHIC_STRING = 0x19, /**< Graphic String */
FIO_ASN1_VISIBLE_STRING = 0x1A, /**< Visible String */
FIO_ASN1_GENERAL_STRING = 0x1B, /**< General String */
FIO_ASN1_UNIVERSAL_STRING = 0x1C, /**< Universal String */
FIO_ASN1_BMP_STRING = 0x1E, /**< BMP String (UCS-2) */
/* Context-specific tags (0x80 | tag_number) with constructed bit (0x20) */
FIO_ASN1_CONTEXT_0 = 0xA0, /**< [0] EXPLICIT/IMPLICIT */
FIO_ASN1_CONTEXT_1 = 0xA1, /**< [1] EXPLICIT/IMPLICIT */
FIO_ASN1_CONTEXT_2 = 0xA2, /**< [2] EXPLICIT/IMPLICIT */
FIO_ASN1_CONTEXT_3 = 0xA3, /**< [3] EXPLICIT/IMPLICIT */
} fio_asn1_tag_e
```

ASN.1 Universal Tag Types

_Symbol type:_ `type`

#### `fio_asn1_class_e`

```c
typedef enum {
FIO_ASN1_CLASS_UNIVERSAL = 0, /**< Universal (built-in types) */
FIO_ASN1_CLASS_APPLICATION = 1, /**< Application-specific */
FIO_ASN1_CLASS_CONTEXT = 2, /**< Context-specific */
FIO_ASN1_CLASS_PRIVATE = 3, /**< Private */
} fio_asn1_class_e
```

ASN.1 Tag Class (bits 7-6 of tag byte)

_Symbol type:_ `type`

#### `fio_asn1_element_s`

```c
typedef struct {
const uint8_t *data; /**< Pointer to element content (after tag+length) */
size_t len; /**< Length of content */
uint8_t tag; /**< Raw tag byte */
uint8_t is_constructed; /**< 1 if constructed (contains other elements) */
uint8_t tag_class; /**< 0=Universal, 1=Application, 2=Context, 3=Private */
uint8_t tag_number; /**< Tag number (bits 4-0, or extended) */
} fio_asn1_element_s
```

Parsed ASN.1 DER element

_Symbol type:_ `type`

#### `fio_asn1_iterator_s`

```c
typedef struct {
const uint8_t *pos; /**< Current position */
const uint8_t *end; /**< End of sequence */
} fio_asn1_iterator_s
```

Iterator for SEQUENCE or SET contents

_Symbol type:_ `type`

### Functions

#### `fio_asn1_parse`

```c
const uint8_t *fio_asn1_parse(fio_asn1_element_s *elem, const uint8_t *data, size_t data_len)
```

Parse one ASN.1 element from DER-encoded data.

**Parameters:**
- `elem` - Output structure to fill with parsed element info
- `data` - Pointer to DER-encoded data
- `data_len` - Length of data buffer

**Returns:**
- Pointer to next element (after this one), or NULL on error

_Symbol type:_ `function`

#### `fio_asn1_element_total_len`

```c
inline size_t fio_asn1_element_total_len(const fio_asn1_element_s *elem, const uint8_t *data)
```

Get the total encoded length of an ASN.1 element (tag + length + content).

**Parameters:**
- `elem` - Parsed element
- `data` - Original data pointer where element was parsed from

**Returns:**
- Total bytes used by the element encoding

_Symbol type:_ `function`

#### `fio_asn1_parse_integer`

```c
int fio_asn1_parse_integer(const fio_asn1_element_s *elem, uint64_t *value)
```

Parse an ASN.1 INTEGER element.

For small integers (<= 64-bit), sets *value.
For large integers (RSA modulus), use elem->data/len directly.
Leading zero bytes for positive numbers are handled correctly.

**Parameters:**
- `elem` - Parsed element (must be INTEGER type)
- `value` - Output for integer value (can be NULL for large integers)

**Returns:**
- 0 on success, -1 on error

_Symbol type:_ `function`

#### `fio_asn1_parse_bit_string`

```c
int fio_asn1_parse_bit_string(const fio_asn1_element_s *elem, const uint8_t **bits, size_t *bit_len, uint8_t *unused_bits)
```

Parse an ASN.1 BIT STRING element.

**Parameters:**
- `elem` - Parsed element (must be BIT STRING type)
- `bits` - Output pointer to bit data (first byte is unused bits count)
- `bit_len` - Output length of bit data in bytes
- `unused_bits` - Output number of unused bits in last byte (0-7)

**Returns:**
- 0 on success, -1 on error

_Symbol type:_ `function`

#### `fio_asn1_parse_oid`

```c
int fio_asn1_parse_oid(const fio_asn1_element_s *elem, char *buf, size_t buf_len)
```

Parse an ASN.1 OID into a dot-separated string.

Example output: "1.2.840.113549.1.1.11"

**Parameters:**
- `elem` - Parsed element (must be OID type)
- `buf` - Output buffer for string
- `buf_len` - Buffer size

**Returns:**
- Number of chars written (excluding NUL), or -1 on error

_Symbol type:_ `function`

#### `fio_asn1_oid_eq`

```c
int fio_asn1_oid_eq(const fio_asn1_element_s *elem, const char *oid_string)
```

Compare an ASN.1 OID element to a known OID string.

**Parameters:**
- `elem` - Parsed element (must be OID type)
- `oid_string` - OID in dot notation (e.g., "1.2.840.113549.1.1.11")

**Returns:**
- 1 if match, 0 if no match

_Symbol type:_ `function`

#### `fio_asn1_parse_time`

```c
int fio_asn1_parse_time(const fio_asn1_element_s *elem, int64_t *unix_time)
```

Parse an ASN.1 time (UTC Time or Generalized Time) to Unix timestamp.

**Parameters:**
- `elem` - Parsed element (must be UTC_TIME or GENERALIZED_TIME type)
- `unix_time` - Output Unix timestamp (seconds since 1970-01-01 00:00:00

UTC)
**Returns:**
- 0 on success, -1 on error

_Symbol type:_ `function`

#### `fio_asn1_parse_string`

```c
inline const char *fio_asn1_parse_string(const fio_asn1_element_s *elem, size_t *len)
```

Parse an ASN.1 string element.

Supports UTF8String, PrintableString, IA5String, etc.
Returns pointer directly into the element data (no copy).

**Parameters:**
- `elem` - Parsed element (must be a string type)
- `len` - Output length of string

**Returns:**
- Pointer to string data, or NULL on error

_Symbol type:_ `function`

#### `fio_asn1_parse_boolean`

```c
inline int fio_asn1_parse_boolean(const fio_asn1_element_s *elem, int *value)
```

Parse an ASN.1 BOOLEAN element.

**Parameters:**
- `elem` - Parsed element (must be BOOLEAN type)
- `value` - Output boolean value (0 = false, non-zero = true)

**Returns:**
- 0 on success, -1 on error

_Symbol type:_ `function`

#### `fio_asn1_iterator_init`

```c
inline void fio_asn1_iterator_init(fio_asn1_iterator_s *it, const fio_asn1_element_s *sequence)
```

Initialize an iterator for a SEQUENCE or SET element.

**Parameters:**
- `it` - Iterator to initialize
- `sequence` - Parsed element (must be SEQUENCE or SET)

_Symbol type:_ `function`

#### `fio_asn1_iterator_next`

```c
int fio_asn1_iterator_next(fio_asn1_iterator_s *it, fio_asn1_element_s *elem)
```

Get the next element from an iterator.

**Parameters:**
- `it` - Iterator (updated to point to next element)
- `elem` - Output for parsed element

**Returns:**
- 0 if element available, -1 if end or error

_Symbol type:_ `function`

#### `fio_asn1_iterator_has_next`

```c
inline int fio_asn1_iterator_has_next(const fio_asn1_iterator_s *it)
```

Check if iterator has more elements.

**Parameters:**
- `it` - Iterator

**Returns:**
- 1 if more elements available, 0 otherwise

_Symbol type:_ `function`

#### `fio_asn1_is_tag`

```c
inline int fio_asn1_is_tag(const fio_asn1_element_s *elem, uint8_t tag)
```

Check if an element is a specific tag type.

**Parameters:**
- `elem` - Parsed element
- `tag` - Expected tag (e.g., FIO_ASN1_INTEGER)

**Returns:**
- 1 if match, 0 otherwise

_Symbol type:_ `function`

#### `fio_asn1_is_context_tag`

```c
inline int fio_asn1_is_context_tag(const fio_asn1_element_s *elem, uint8_t tag_num)
```

Check if an element is a context-specific tag.

**Parameters:**
- `elem` - Parsed element
- `tag_num` - Context tag number (0-31)

**Returns:**
- 1 if match, 0 otherwise

_Symbol type:_ `function`

#### `fio_asn1_tag_number`

```c
inline uint8_t fio_asn1_tag_number(const fio_asn1_element_s *elem)
```

Get the tag number from an element.

For universal tags, returns the tag value (0-30).
For context-specific tags, returns the context number.

**Parameters:**
- `elem` - Parsed element

**Returns:**
- Tag number

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-155-rsa-h"></a> `./fio-stl/155 rsa.h`

9 public symbols.

### Macros

#### `FIO_RSA_MAX_BITS`

```c
#define FIO_RSA_MAX_BITS 4096
```

Maximum RSA key size in bits

_Symbol type:_ `macro`

#### `FIO_RSA_MAX_BYTES`

```c
#define FIO_RSA_MAX_BYTES (FIO_RSA_MAX_BITS / 8)
```

Maximum RSA key size in bytes

_Symbol type:_ `macro`

#### `FIO_RSA_MAX_WORDS`

```c
#define FIO_RSA_MAX_WORDS (FIO_RSA_MAX_BYTES / 8)
```

Maximum RSA key size in 64-bit words

_Symbol type:_ `macro`

### Types

#### `fio_rsa_hash_e`

```c
typedef enum {
FIO_RSA_HASH_SHA256 = 0, /**< SHA-256 (32 bytes) */
FIO_RSA_HASH_SHA384 = 1, /**< SHA-384 (48 bytes) */
FIO_RSA_HASH_SHA512 = 2, /**< SHA-512 (64 bytes) */
} fio_rsa_hash_e
```

Hash algorithm identifiers for RSA verification

_Symbol type:_ `type`

#### `fio_rsa_pubkey_s`

```c
typedef struct {
const uint8_t *n; /**< Modulus (big-endian) */
size_t n_len; /**< Modulus length in bytes */
const uint8_t *e; /**< Public exponent (big-endian) */
size_t e_len; /**< Exponent length in bytes */
} fio_rsa_pubkey_s
```

RSA public key for signature verification.

The modulus (n) and exponent (e) are stored as big-endian byte arrays.
This matches the DER encoding used in X.509 certificates.

_Symbol type:_ `type`

#### `fio_rsa_privkey_s`

```c
typedef struct {
const uint8_t *n; /**< Modulus (big-endian) */
size_t n_len; /**< Modulus length in bytes (256, 384, or 512) */
const uint8_t *d; /**< Private exponent (big-endian) */
size_t d_len; /**< Private exponent length in bytes */
const uint8_t *e; /**< Public exponent (big-endian), optional, for blinding */
size_t e_len;
const uint8_t *p; /**< Prime p (big-endian), optional, for CRT */
size_t p_len;
const uint8_t *q; /**< Prime q (big-endian), optional, for CRT */
size_t q_len;
const uint8_t *dP; /**< d mod (p-1) (big-endian), optional, for CRT */
size_t dP_len;
const uint8_t *dQ; /**< d mod (q-1) (big-endian), optional, for CRT */
size_t dQ_len;
const uint8_t *qInv; /**< q^-1 mod p (big-endian), optional, for CRT */
size_t qInv_len;
} fio_rsa_privkey_s
```

RSA private key for signature generation.

The modulus (n) and private exponent (d) are stored as big-endian byte
arrays. This matches the DER encoding used in PKCS#8 private keys.

Optional CRT parameters (p, q, dP, dQ, qInv) and the public exponent (e)
may be provided. When CRT parameters are available, signing uses CRT with
message blinding for better side-channel resistance. When only n and d are
available, signing falls back to a non-CRT (still constant-time) path.

All optional fields are indicated by a non-zero length. Missing CRT
parameters may be derived from p, q, and d when p and q are present.

_Symbol type:_ `type`

### Functions

#### `fio_rsa_verify_pkcs1`

```c
int fio_rsa_verify_pkcs1(const uint8_t *sig, size_t sig_len, const uint8_t *msg_hash, size_t hash_len, fio_rsa_hash_e hash_alg, const fio_rsa_pubkey_s *key)
```

Verify an RSA PKCS#1 v1.5 signature.

This verifies signatures with DigestInfo encoding as used in:
- sha256WithRSAEncryption (OID 1.2.840.113549.1.1.11)
- sha384WithRSAEncryption (OID 1.2.840.113549.1.1.12)
- sha512WithRSAEncryption (OID 1.2.840.113549.1.1.13)

**Parameters:**
- `sig` - Signature bytes (same length as modulus)
- `sig_len` - Signature length in bytes
- `msg_hash` - Pre-computed hash of the message
- `hash_len` - Hash length (32, 48, or 64 bytes)
- `hash_alg` - Hash algorithm used (FIO_RSA_HASH_SHA256, etc.)
- `key` - RSA public key

**Returns:**
- 0 on success (valid signature), -1 on failure

_Symbol type:_ `function`

#### `fio_rsa_verify_pss`

```c
int fio_rsa_verify_pss(const uint8_t *sig, size_t sig_len, const uint8_t *msg_hash, size_t hash_len, fio_rsa_hash_e hash_alg, const fio_rsa_pubkey_s *key)
```

Verify an RSA-PSS signature (required for TLS 1.3).

RSA-PSS uses probabilistic padding and is the mandatory signature scheme
for TLS 1.3 CertificateVerify messages with RSA keys.

This implementation uses:
- MGF1 with the same hash function
- Salt length = hash length (as required by TLS 1.3)
- Trailer field = 0xBC

**Parameters:**
- `sig` - Signature bytes (same length as modulus)
- `sig_len` - Signature length in bytes
- `msg_hash` - Pre-computed hash of the message
- `hash_len` - Hash length (32, 48, or 64 bytes)
- `hash_alg` - Hash algorithm used
- `key` - RSA public key

**Returns:**
- 0 on success (valid signature), -1 on failure

_Symbol type:_ `function`

#### `fio_rsa_sign_pss`

```c
int fio_rsa_sign_pss(uint8_t *signature, size_t *sig_len, const uint8_t *msg_hash, size_t hash_len, fio_rsa_hash_e hash_alg, const fio_rsa_privkey_s *key)
```

Generate an RSA-PSS signature (RSASSA-PSS-SIGN per RFC 8017 Section 8.1.1).

This is required for TLS 1.3 server CertificateVerify messages when using
RSA certificates. PKCS#1 v1.5 signatures are NOT allowed for
CertificateVerify in TLS 1.3.

This implementation uses:
- MGF1 with the same hash function
- Salt length = hash length (as required by TLS 1.3)
- Trailer field = 0xBC

**Parameters:**
- `signature` - Output buffer for signature (must be key->n_len bytes)
- `sig_len` - Output: actual signature length (equals key->n_len)
- `msg_hash` - Pre-computed hash of the message to sign
- `hash_len` - Hash length (32, 48, or 64 bytes)
- `hash_alg` - Hash algorithm (FIO_RSA_HASH_SHA256, etc.)
- `key` - RSA private key

**Returns:**
- 0 on success, -1 on error

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-155-x509-h"></a> `./fio-stl/155 x509.h`

18 public symbols.

### Macros

#### `FIO_TLS_CERT_PARSE_ERROR`

```c
#define FIO_TLS_CERT_PARSE_ERROR ((size_t)-1)
```

Error value for fio_tls_parse_certificate_message

_Symbol type:_ `macro`

#### `FIO_X509_MAX_CHAIN_DEPTH`

```c
#define FIO_X509_MAX_CHAIN_DEPTH 10
```



_Symbol type:_ `macro`

### Types

#### `fio_x509_key_type_e`

```c
typedef enum {
FIO_X509_KEY_UNKNOWN = 0,
FIO_X509_KEY_RSA = 1, /**< RSA (any key size) */
FIO_X509_KEY_ECDSA_P256 = 2, /**< ECDSA with P-256/secp256r1 */
FIO_X509_KEY_ECDSA_P384 = 3, /**< ECDSA with P-384/secp384r1 */
FIO_X509_KEY_ED25519 = 4, /**< Ed25519 (EdDSA) */
} fio_x509_key_type_e
```

Public key algorithm types

_Symbol type:_ `type`

#### `fio_x509_sig_alg_e`

```c
typedef enum {
FIO_X509_SIG_UNKNOWN = 0,
FIO_X509_SIG_RSA_PKCS1_SHA256 = 1, /**< sha256WithRSAEncryption */
FIO_X509_SIG_RSA_PKCS1_SHA384 = 2, /**< sha384WithRSAEncryption */
FIO_X509_SIG_RSA_PKCS1_SHA512 = 3, /**< sha512WithRSAEncryption */
FIO_X509_SIG_RSA_PSS_SHA256 = 4, /**< RSA-PSS with SHA-256 */
FIO_X509_SIG_RSA_PSS_SHA384 = 5, /**< RSA-PSS with SHA-384 */
FIO_X509_SIG_RSA_PSS_SHA512 = 6, /**< RSA-PSS with SHA-512 */
FIO_X509_SIG_ECDSA_SHA256 = 7, /**< ecdsa-with-SHA256 */
FIO_X509_SIG_ECDSA_SHA384 = 8, /**< ecdsa-with-SHA384 */
FIO_X509_SIG_ED25519 = 9, /**< Ed25519 */
} fio_x509_sig_alg_e
```

Signature algorithm types

_Symbol type:_ `type`

#### `fio_x509_key_usage_e`

```c
typedef enum {
FIO_X509_KU_DIGITAL_SIGNATURE = 0x0080, /* bit 0 = MSB of byte 0 */
FIO_X509_KU_NON_REPUDIATION = 0x0040, /* bit 1 */
FIO_X509_KU_KEY_ENCIPHERMENT = 0x0020, /* bit 2 */
FIO_X509_KU_DATA_ENCIPHERMENT = 0x0010, /* bit 3 */
FIO_X509_KU_KEY_AGREEMENT = 0x0008, /* bit 4 */
FIO_X509_KU_KEY_CERT_SIGN = 0x0004, /* bit 5 */
FIO_X509_KU_CRL_SIGN = 0x0002, /* bit 6 */
FIO_X509_KU_ENCIPHER_ONLY = 0x0001, /* bit 7 */
FIO_X509_KU_DECIPHER_ONLY = 0x8000, /* bit 8 = MSB of byte 1 */
} fio_x509_key_usage_e
```

Key Usage bit flags (RFC 5280 Section 4.2.1.3)

ASN.1 BIT STRING uses MSB-first bit ordering:
- Bit 0 = MSB of first byte (0x80)
- Bit 1 = 0x40, Bit 2 = 0x20, etc.
- Bits 8+ are in the second byte

_Symbol type:_ `type`

#### `fio_x509_error_e`

```c
typedef enum {
FIO_X509_OK = 0, /**< Validation successful */
FIO_X509_ERR_PARSE = -1, /**< Failed to parse certificate */
FIO_X509_ERR_EXPIRED = -2, /**< Certificate expired */
FIO_X509_ERR_NOT_YET_VALID = -3, /**< Certificate not yet valid */
FIO_X509_ERR_SIGNATURE = -4, /**< Signature verification failed */
FIO_X509_ERR_ISSUER_MISMATCH = -5, /**< Issuer DN doesn't match subject DN */
FIO_X509_ERR_NOT_CA = -6, /**< Issuer is not a CA certificate */
FIO_X509_ERR_NO_TRUST_ANCHOR = -7, /**< Certificate not in trust store */
FIO_X509_ERR_HOSTNAME_MISMATCH = -8, /**< Hostname doesn't match cert */
FIO_X509_ERR_EMPTY_CHAIN = -9, /**< Empty certificate chain */
FIO_X509_ERR_CHAIN_TOO_LONG = -10, /**< Chain exceeds maximum depth */
} fio_x509_error_e
```

X.509 chain validation error codes

_Symbol type:_ `type`

#### `fio_x509_trust_store_s`

```c
typedef struct {
const uint8_t **roots; /**< Array of root CA certificate DER data */
const size_t *root_lens; /**< Array of root CA certificate lengths */
size_t root_count; /**< Number of root CAs */
} fio_x509_trust_store_s
```

Trust store for root CA certificates

_Symbol type:_ `type`

#### `fio_tls_cert_entry_s`

```c
typedef struct {
const uint8_t *cert; /**< DER-encoded certificate data */
size_t cert_len; /**< Certificate length */
} fio_tls_cert_entry_s
```

TLS certificate entry (parsed from Certificate message)

_Symbol type:_ `type`

#### `fio_x509_cert_s`

```c
typedef struct {
/** Certificate version (0=v1, 1=v2, 2=v3) */
int version;
/** Validity period (Unix timestamps) */
int64_t not_before;
int64_t not_after;
/** Subject Distinguished Name (raw DER for comparison) */
const uint8_t *subject_der;
size_t subject_der_len;
/** Issuer Distinguished Name (raw DER for comparison) */
const uint8_t *issuer_der;
size_t issuer_der_len;
/** Subject Common Name (if present, pointer into DER data) */
const char *subject_cn;
size_t subject_cn_len;
/** Public Key Type */
fio_x509_key_type_e key_type;
/** Public Key Data (union based on key_type) */
union {
struct {
const uint8_t *n; /**< RSA modulus (big-endian) */
size_t n_len;
const uint8_t *e; /**< RSA exponent (big-endian) */
size_t e_len;
} rsa;
struct {
const uint8_t *point; /**< Uncompressed EC point (04 || x || y) */
size_t point_len;
} ecdsa;
struct {
const uint8_t *key; /**< 32-byte Ed25519 public key */
} ed25519;
} pubkey;
/** Signature Algorithm */
fio_x509_sig_alg_e sig_alg;
/** Signature value (pointer into DER data) */
const uint8_t *signature;
size_t signature_len;
/** TBS Certificate (for signature verification) */
const uint8_t *tbs_data;
size_t tbs_len;
/** Basic Constraints: is CA */
int is_ca;
/** Key Usage extension present */
int has_key_usage;
/** Key Usage bits */
uint16_t key_usage;
/** Subject Alternative Name: first DNS name (if present) */
const char *san_dns;
size_t san_dns_len;
/** Subject Alternative Name extension raw data (for iterating all SANs) */
const uint8_t *san_ext_data;
size_t san_ext_len;
} fio_x509_cert_s
```

Parsed X.509 certificate structure

_Symbol type:_ `type`

### Functions

#### `fio_x509_parse`

```c
int fio_x509_parse(fio_x509_cert_s *cert, const uint8_t *der_data, size_t der_len)
```

Parse a DER-encoded X.509 certificate.

The cert structure will contain pointers into the original DER data,
so the DER data must remain valid while the cert is in use.

**Parameters:**
- `cert` - Output certificate structure (will be zeroed first)
- `der_data` - Pointer to DER-encoded certificate
- `der_len` - Length of DER data in bytes

**Returns:**
- 0 on success, -1 on error

_Symbol type:_ `function`

#### `fio_x509_verify_signature`

```c
int fio_x509_verify_signature(const fio_x509_cert_s *cert, const fio_x509_cert_s *issuer)
```

Verify certificate signature using issuer's public key.

This verifies that the certificate was signed by the issuer.

**Parameters:**
- `cert` - Certificate to verify
- `issuer` - Certificate of the issuer (contains the public key)

**Returns:**
- 0 if valid, -1 if invalid or error

_Symbol type:_ `function`

#### `fio_x509_check_validity`

```c
inline int fio_x509_check_validity(const fio_x509_cert_s *cert, int64_t current_time)
```

Check if certificate is currently valid (not expired, not yet valid).

**Parameters:**
- `cert` - Certificate to check
- `current_time` - Current Unix timestamp (seconds since epoch)

**Returns:**
- 0 if valid, -1 if expired or not yet valid

_Symbol type:_ `function`

#### `fio_x509_match_hostname`

```c
int fio_x509_match_hostname(const fio_x509_cert_s *cert, const char *hostname, size_t hostname_len)
```

Check if hostname matches certificate (CN or SAN).

Supports wildcard matching (*.example.com).
Per RFC 6125, wildcards only match one label.

**Parameters:**
- `cert` - Certificate to check
- `hostname` - Hostname to match
- `hostname_len` - Length of hostname

**Returns:**
- 0 if match, -1 if no match

_Symbol type:_ `function`

#### `fio_x509_dn_equals`

```c
inline int fio_x509_dn_equals(const uint8_t *dn1, size_t dn1_len, const uint8_t *dn2, size_t dn2_len)
```

Compare two Distinguished Names for equality.

Used for checking if issuer DN matches subject DN.

**Parameters:**
- `dn1` - First DN (DER-encoded)
- `dn1_len` - Length of first DN
- `dn2` - Second DN (DER-encoded)
- `dn2_len` - Length of second DN

**Returns:**
- 0 if equal, non-zero if different

_Symbol type:_ `function`

#### `fio_x509_verify_chain`

```c
int fio_x509_verify_chain(const uint8_t **certs, const size_t *cert_lens, size_t cert_count, const char *hostname, int64_t current_time, fio_x509_trust_store_s *trust_store)
```

Validate a certificate chain for TLS 1.3.

The chain should be ordered from end-entity to closest-to-root:
  - certs[0] = server's certificate (end-entity)
  - certs[1] = intermediate CA (signed certs[0])
  - certs[n-1] = closest to root (may be root or intermediate)

Validation performs:
  1. Parse all certificates
  2. Check validity period for all certificates
  3. Verify hostname matches end-entity certificate (if hostname provided)
  4. Verify each certificate's signature using the next certificate's key
  5. Verify issuer DNs match subject DNs in the chain
  6. Verify intermediate/root certificates have CA:TRUE
  7. Verify the chain terminates at a trusted root (if trust store provided)

**Parameters:**
- `certs` - Array of DER-encoded certificates
- `cert_lens` - Array of certificate lengths
- `cert_count` - Number of certificates in chain
- `hostname` - Expected hostname for end-entity (NULL to skip check)
- `current_time` - Current Unix timestamp for validity checking
- `trust_store` - Root CA certificates (NULL to skip trust check)

**Returns:**
- FIO_X509_OK (0) on success, or error code on failure

_Symbol type:_ `function`

#### `fio_x509_is_trusted`

```c
int fio_x509_is_trusted(const fio_x509_cert_s *cert, fio_x509_trust_store_s *trust_store)
```

Check if a certificate is in the trust store.

Comparison is done by matching subject DN.

**Parameters:**
- `cert` - Certificate to check
- `trust_store` - Trust store to search

**Returns:**
- 0 if trusted, -1 if not found

_Symbol type:_ `function`

#### `fio_tls_parse_certificate_message`

```c
size_t fio_tls_parse_certificate_message(fio_tls_cert_entry_s *entries, size_t max_entries, const uint8_t *data, size_t data_len)
```

Parse TLS 1.3 Certificate message into individual certificates.

TLS 1.3 Certificate message format (RFC 8446):
  certificate_request_context<0..2^8-1>
  certificate_list<0..2^24-1>:
    CertificateEntry:
      cert_data<1..2^24-1>
      extensions<0..2^16-1>

**Parameters:**
- `entries` - Output array for certificate entries
- `max_entries` - Maximum entries to parse
- `data` - Raw Certificate message data (after handshake header)
- `data_len` - Length of Certificate message data

**Returns:**
- Number of certificates parsed, or FIO_TLS_CERT_PARSE_ERROR on error

_Symbol type:_ `function`

#### `fio_x509_error_str`

```c
inline const char *fio_x509_error_str(int error)
```

Get human-readable error string for X.509 validation error code.

**Parameters:**
- `error` - Error code from fio_x509_verify_chain

**Returns:**
- Static string describing the error

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-156-mlkem-h"></a> `./fio-stl/156 mlkem.h`

17 public symbols.

### Macros

#### `FIO_MLKEM768_PUBLICKEYBYTES`

```c
#define FIO_MLKEM768_PUBLICKEYBYTES  1184
```

ML-KEM-768 constants

_Symbol type:_ `macro`

#### `FIO_MLKEM768_SECRETKEYBYTES`

```c
#define FIO_MLKEM768_SECRETKEYBYTES  2400
```



_Symbol type:_ `macro`

#### `FIO_MLKEM768_CIPHERTEXTBYTES`

```c
#define FIO_MLKEM768_CIPHERTEXTBYTES 1088
```



_Symbol type:_ `macro`

#### `FIO_MLKEM768_SSBYTES`

```c
#define FIO_MLKEM768_SSBYTES         32
```



_Symbol type:_ `macro`

#### `FIO_MLKEM768_SYMBYTES`

```c
#define FIO_MLKEM768_SYMBYTES        32
```



_Symbol type:_ `macro`

#### `FIO_X25519MLKEM768_PUBLICKEYBYTES`

```c
#define FIO_X25519MLKEM768_PUBLICKEYBYTES  (32 + 1184) /* 1216 */
```

X25519MLKEM768 hybrid constants

_Symbol type:_ `macro`

#### `FIO_X25519MLKEM768_SECRETKEYBYTES`

```c
#define FIO_X25519MLKEM768_SECRETKEYBYTES  (32 + 2400) /* 2432 */
```



_Symbol type:_ `macro`

#### `FIO_X25519MLKEM768_CIPHERTEXTBYTES`

```c
#define FIO_X25519MLKEM768_CIPHERTEXTBYTES (32 + 1088) /* 1120 */
```



_Symbol type:_ `macro`

#### `FIO_X25519MLKEM768_SSBYTES`

```c
#define FIO_X25519MLKEM768_SSBYTES         64
```



_Symbol type:_ `macro`

### Functions

#### `fio_mlkem768_keypair`

```c
int fio_mlkem768_keypair(uint8_t pk[1184], uint8_t sk[2400])
```

Generate ML-KEM-768 keypair.

Generates a random keypair using the system CSPRNG.
Returns 0 on success, -1 on failure.

_Symbol type:_ `function`

#### `fio_mlkem768_keypair_derand`

```c
int fio_mlkem768_keypair_derand(uint8_t pk[1184], uint8_t sk[2400], const uint8_t coins[64])
```

Generate ML-KEM-768 keypair from deterministic seed.

The coins buffer must be exactly 64 bytes (d || z).
Returns 0 on success, -1 on failure.

_Symbol type:_ `function`

#### `fio_mlkem768_encaps`

```c
int fio_mlkem768_encaps(uint8_t ct[1088], uint8_t ss[32], const uint8_t pk[1184])
```

Encapsulate: generate ciphertext and shared secret from public key.

Uses system CSPRNG for randomness.
Returns 0 on success, -1 on failure.

_Symbol type:_ `function`

#### `fio_mlkem768_encaps_derand`

```c
int fio_mlkem768_encaps_derand(uint8_t ct[1088], uint8_t ss[32], const uint8_t pk[1184], const uint8_t coins[32])
```

Encapsulate with deterministic randomness.

The coins buffer must be exactly 32 bytes.
Returns 0 on success, -1 on failure.

_Symbol type:_ `function`

#### `fio_mlkem768_decaps`

```c
int fio_mlkem768_decaps(uint8_t ss[32], const uint8_t ct[1088], const uint8_t sk[2400])
```

Decapsulate: recover shared secret from ciphertext and secret key.

Uses implicit rejection: if the ciphertext is invalid, a pseudorandom
shared secret is returned (derived from the secret key and ciphertext)
rather than an error, preventing chosen-ciphertext attacks.

Returns 0 on success (always succeeds for well-formed inputs).

_Symbol type:_ `function`

#### `fio_x25519mlkem768_keypair`

```c
int fio_x25519mlkem768_keypair(uint8_t pk[1216], uint8_t sk[2432])
```

Generate X25519MLKEM768 hybrid keypair.

Generates both X25519 and ML-KEM-768 keypairs using system CSPRNG.
The public key is ML-KEM-768_ek (1184) || X25519_pk (32) = 1216 bytes.
The secret key is ML-KEM-768_dk (2400) || X25519_sk (32) = 2432 bytes.

Returns 0 on success, -1 on failure.

_Symbol type:_ `function`

#### `fio_x25519mlkem768_encaps`

```c
int fio_x25519mlkem768_encaps(uint8_t ct[1120], uint8_t ss[64], const uint8_t pk[1216])
```

X25519MLKEM768 hybrid encapsulation.

Performs both X25519 key exchange and ML-KEM-768 encapsulation.
The ciphertext is ML-KEM-768_ct (1088) || X25519_ephemeral_pk (32) = 1120
bytes. The shared secret is ML-KEM-768_ss (32) || X25519_ss (32) = 64 bytes.

Returns 0 on success, -1 on failure.

_Symbol type:_ `function`

#### `fio_x25519mlkem768_decaps`

```c
int fio_x25519mlkem768_decaps(uint8_t ss[64], const uint8_t ct[1120], const uint8_t sk[2432])
```

X25519MLKEM768 hybrid decapsulation.

Performs both X25519 shared secret derivation and ML-KEM-768 decapsulation.
The shared secret is ML-KEM-768_ss (32) || X25519_ss (32) = 64 bytes.

Returns 0 on success, -1 if X25519 shared secret computation fails
(low-order point). ML-KEM-768 uses implicit rejection for invalid
ciphertexts.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-156-pem-h"></a> `./fio-stl/156 pem.h`

8 public symbols.

### Types

#### `fio_pem_key_type_e`

```c
typedef enum {
FIO_PEM_KEY_UNKNOWN = 0,
FIO_PEM_KEY_RSA = 1,
FIO_PEM_KEY_ECDSA_P256 = 2,
FIO_PEM_KEY_ED25519 = 3,
} fio_pem_key_type_e
```

Private key algorithm types

_Symbol type:_ `type`

#### `fio_pem_s`

```c
typedef struct {
const uint8_t *der; /**< Pointer to decoded DER data */
size_t der_len; /**< Length of DER data */
const char *label; /**< PEM label (e.g., "CERTIFICATE", "PRIVATE KEY") */
size_t label_len; /**< Length of label */
} fio_pem_s
```

Parsed PEM block

_Symbol type:_ `type`

#### `fio_pem_private_key_s`

```c
typedef struct {
fio_pem_key_type_e type;
union {
struct {
uint8_t n[FIO_RSA_MAX_BYTES]; /**< RSA modulus (big-endian) */
size_t n_len;
uint8_t e[FIO_RSA_MAX_BYTES]; /**< RSA public exponent (big-endian) */
size_t e_len;
uint8_t d[FIO_RSA_MAX_BYTES]; /**< RSA private exponent (big-endian) */
size_t d_len;
uint8_t p[FIO_RSA_MAX_BYTES]; /**< RSA prime p (optional) */
size_t p_len;
uint8_t q[FIO_RSA_MAX_BYTES]; /**< RSA prime q (optional) */
size_t q_len;
uint8_t dP[FIO_RSA_MAX_BYTES]; /**< d mod (p-1) (optional) */
size_t dP_len;
uint8_t dQ[FIO_RSA_MAX_BYTES]; /**< d mod (q-1) (optional) */
size_t dQ_len;
uint8_t qInv[FIO_RSA_MAX_BYTES]; /**< q^-1 mod p (optional) */
size_t qInv_len;
} rsa;
struct {
uint8_t private_key[32]; /**< P-256 scalar (32 bytes) */
uint8_t public_key[65]; /**< Uncompressed point (optional, can derive) */
int has_public_key; /**< 1 if public_key is populated */
} ecdsa_p256;
struct {
uint8_t private_key[32]; /**< Ed25519 seed (32 bytes) */
uint8_t public_key[32]; /**< Ed25519 public key (optional) */
int has_public_key; /**< 1 if public_key is populated */
} ed25519;
};
} fio_pem_private_key_s
```

Parsed private key structure

_Symbol type:_ `type`

### Functions

#### `fio_pem_parse`

```c
size_t fio_pem_parse(fio_pem_s *out, uint8_t *der_buf, size_t der_buf_len, const char *pem_data, size_t pem_len)
```

Parse a single PEM block from data.

Finds the next -----BEGIN <label>----- and -----END <label>----- markers,
base64 decodes the content between them, and returns the DER data.

**Parameters:**
- `out` - Output structure to fill with parsed PEM block info
- `der_buf` - Buffer to store decoded DER data (caller-provided)
- `der_buf_len` - Size of der_buf
- `pem_data` - PEM-encoded data
- `pem_len` - Length of PEM data

**Returns:**
- Number of bytes consumed from pem_data, or 0 on error

_Symbol type:_ `function`

#### `fio_pem_parse_certificate`

```c
int fio_pem_parse_certificate(fio_x509_cert_s *cert, const char *pem_data, size_t pem_len)
```

Parse certificate from PEM file content.

Handles "CERTIFICATE" label and parses the X.509 certificate.

**Parameters:**
- `cert` - Output certificate structure (from fio_x509.h)
- `pem_data` - PEM-encoded certificate data
- `pem_len` - Length of PEM data

**Returns:**
- 0 on success, -1 on error

_Symbol type:_ `function`

#### `fio_pem_parse_private_key`

```c
int fio_pem_parse_private_key(fio_pem_private_key_s *key, const char *pem_data, size_t pem_len)
```

Parse private key from PEM file content.

Supports:
- "PRIVATE KEY" (PKCS#8 PrivateKeyInfo)
- "RSA PRIVATE KEY" (PKCS#1 RSAPrivateKey)
- "EC PRIVATE KEY" (SEC1 ECPrivateKey)

**Parameters:**
- `key` - Output private key structure
- `pem_data` - PEM-encoded private key data
- `pem_len` - Length of PEM data

**Returns:**
- 0 on success, -1 on error

_Symbol type:_ `function`

#### `fio_pem_get_certificate_der`

```c
size_t fio_pem_get_certificate_der(uint8_t *der_out, size_t der_out_len, const char *pem_data, size_t pem_len)
```

Get the DER-encoded certificate from PEM data.

This is a convenience function that extracts just the DER bytes
without parsing the X.509 structure.

**Parameters:**
- `der_out` - Output buffer for DER data
- `der_out_len` - Size of output buffer
- `pem_data` - PEM-encoded certificate data
- `pem_len` - Length of PEM data

**Returns:**
- Length of DER data written, or 0 on error

_Symbol type:_ `function`

#### `fio_pem_private_key_clear`

```c
inline void fio_pem_private_key_clear(fio_pem_private_key_s *key)
```

Securely clear a private key structure.

**Parameters:**
- `key` - Private key to clear

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-159-argon2-h"></a> `./fio-stl/159 argon2.h`

6 public symbols.

### Types

#### `fio_argon2_type_e`

```c
typedef enum {
FIO_ARGON2D = 0,
FIO_ARGON2I = 1,
FIO_ARGON2ID = 2,
} fio_argon2_type_e
```

Argon2 type selector.

_Symbol type:_ `type`

#### `fio_argon2_args_s`

```c
typedef struct {
/** The password (message P). */
fio_buf_info_s password;
/** The salt (nonce S). */
fio_buf_info_s salt;
/** Optional secret key K. */
fio_buf_info_s secret;
/** Optional associated data X. */
fio_buf_info_s ad;
/** Time cost (number of passes t, minimum 1). */
uint32_t t_cost;
/** Memory cost in KiB (minimum 8*parallelism). */
uint32_t m_cost;
/** Degree of parallelism (number of lanes, minimum 1). */
uint32_t parallelism;
/** Desired output (tag) length in bytes (minimum 4, default 32). */
uint32_t outlen;
/** Argon2 variant: FIO_ARGON2D, FIO_ARGON2I, or FIO_ARGON2ID. */
fio_argon2_type_e type;
} fio_argon2_args_s
```

Argon2 named arguments.

_Symbol type:_ `type`

### Functions

#### `fio_argon2`

```c
fio_u512 fio_argon2(fio_argon2_args_s args)
```

Computes Argon2 password hash (RFC 9106).

Supports Argon2d, Argon2i, and Argon2id variants.
Output is written to the returned buffer (up to 64 bytes via fio_u512).
For outlen > 64, use fio_argon2_hash() instead.

Use `fio_ct_is_eq` to compare output hashes in constant time.

_Symbol type:_ `function`

#### `fio_argon2`

```c
#define fio_argon2(...) fio_argon2((fio_argon2_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_argon2_hash`

```c
int fio_argon2_hash(void *out, fio_argon2_args_s args)
```

Computes Argon2 password hash into a caller-provided buffer.
Supports arbitrary output lengths (minimum 4 bytes).
Returns 0 on success, -1 on error.

_Symbol type:_ `function`

#### `fio_argon2_hash`

```c
#define fio_argon2_hash(out, ...)   \
  fio_argon2_hash(out, (fio_argon2_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-159-lyra2-h"></a> `./fio-stl/159 lyra2.h`

5 public symbols.

### Types

#### `fio_lyra2_args_s`

```c
typedef struct {
/** The password to hash. */
fio_buf_info_s password;
/** The salt for the hash. */
fio_buf_info_s salt;
/** Time cost (number of rounds, minimum 1). */
uint64_t t_cost;
/** Memory cost (number of rows in the matrix, minimum 3). */
uint64_t m_cost;
/** Desired output length in bytes (default 32 if 0). */
size_t outlen;
/** Number of columns (default 256 if 0). */
size_t n_cols;
} fio_lyra2_args_s
```

Lyra2 named arguments.

_Symbol type:_ `type`

### Functions

#### `fio_lyra2`

```c
fio_u512 fio_lyra2(fio_lyra2_args_s args)
```

Computes Lyra2 password hash.

Uses Blake2b as the underlying sponge/hash function.
Matches the reference C implementation (nPARALLEL==1, SPONGE==0, RHO==1).

Output is written to the returned buffer (up to 512 bytes via fio_u512).
For outlen > 64, use fio_lyra2_hash() instead.

Use `fio_ct_is_eq` to compare output hashes in constant time.

_Symbol type:_ `function`

#### `fio_lyra2`

```c
#define fio_lyra2(...) fio_lyra2((fio_lyra2_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_lyra2_hash`

```c
int fio_lyra2_hash(void *out, fio_lyra2_args_s args)
```

Computes Lyra2 password hash into a caller-provided buffer.
Supports arbitrary output lengths.
Returns 0 on success, -1 on error.

_Symbol type:_ `function`

#### `fio_lyra2_hash`

```c
#define fio_lyra2_hash(out, ...)   \
  fio_lyra2_hash(out, (fio_lyra2_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-159-otp-h"></a> `./fio-stl/159 otp.h`

7 public symbols.

### Types

#### `fio_otp_settings_s`

```c
typedef struct {
/** The time interval for TOTP rotation. */
size_t interval; /* 30 == Google OTP */
/** The number of digits in the OTP. */
size_t digits; /* 6 == Google OTP */
/** The time offset (in `interval` units) from the current time. */
int64_t offset; /* 0 == Google OTP */
/** Set to true if the secret / key is in Hex instead of Byte32 encoding. */
uint8_t is_hex;
/** Set to true if the secret / key is raw bit data (no encoding). */
uint8_t is_raw;
} fio_otp_settings_s
```



_Symbol type:_ `type`

### Functions

#### `fio_otp_generate_key`

```c
inline fio_u128 fio_otp_generate_key(void)
```

Generates a random 128 bit key for TOTP processing.

_Symbol type:_ `function`

#### `fio_otp_print_key`

```c
inline size_t fio_otp_print_key(char *dest, uint8_t *key, size_t len)
```

Prints out an OTP secret (big endian number) as a Byte32 encoded String.

_Symbol type:_ `function`

#### `fio_otp`

```c
uint32_t fio_otp(fio_buf_info_s secret, fio_otp_settings_s settings)
```

Returns a TOTP based on `secret` and the otp settings.

_Symbol type:_ `function`

#### `fio_otp`

```c
#define fio_otp(secret, ...) fio_otp(secret, (fio_otp_settings_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_otp_at`

```c
uint32_t fio_otp_at(fio_buf_info_s secret, uint64_t unix_time, fio_otp_settings_s settings)
```

Returns a TOTP for a specific unix timestamp (for testing/verification).
This is useful for verifying OTPs at specific times or for RFC test vectors.

_Symbol type:_ `function`

#### `fio_otp_at`

```c
#define fio_otp_at(secret, unix_time, ...)   \
  fio_otp_at(secret, unix_time, (fio_otp_settings_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-159-secret-h"></a> `./fio-stl/159 secret.h`

5 public symbols.

### Functions

#### `fio_secret_is_random`

```c
bool fio_secret_is_random(void)
```

Returns true if the secret was randomly generated.

_Symbol type:_ `function`

#### `fio_secret`

```c
fio_u512 fio_secret(void)
```

Gets the SHA512 of a (possibly shared) secret.

_Symbol type:_ `function`

#### `fio_secret_set`

```c
void fio_secret_set(char *str, size_t len, bool is_random)
```

Sets a (possibly shared) secret and stores its SHA512 hash.

_Symbol type:_ `function`

#### `fio_secret_set_at`

```c
void fio_secret_set_at(fio_u512 *secret, char *str, size_t len)
```

Sets a (possibly shared) secret and stores its SHA512 hash.

_Symbol type:_ `function`

#### `fio_secret_at`

```c
fio_u512 fio_secret_at(fio_u512 *secret)
```

Gets the SHA512 of a (possibly shared) secret.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-162-brotli-h"></a> `./fio-stl/162 brotli.h`

4 public symbols.

### Functions

#### `fio_brotli_decompress`

```c
size_t fio_brotli_decompress(void *out, size_t out_len, const void *in, size_t in_len)
```

Decompresses Brotli-compressed data (RFC 7932).

Returns:
 - On success: decompressed byte count (<= out_len).
 - On buffer too small: the REQUIRED buffer size (> out_len).
 - On corrupt/invalid data: 0.
 - When out==NULL or out_len==0: performs a header scan summing MLEN
   values and returns the required size (0 if data is corrupt).
   This allows callers to query the decompressed size without allocating.

_Symbol type:_ `function`

#### `fio_brotli_decompress_bound`

```c
inline size_t fio_brotli_decompress_bound(size_t in_len)
```

Conservative upper bound on decompressed size for a Brotli stream.

_Symbol type:_ `function`

#### `fio_brotli_compress`

```c
size_t fio_brotli_compress(void *out, size_t out_len, const void *in, size_t in_len, int quality)
```

Compresses data using Brotli (RFC 7932).

Returns compressed length on success, 0 on error.
quality: 1-6 (1=fast greedy, 3-4=lazy matching, 5-6=hash chain + context
modeling + static dictionary). Caller provides output buffer sized via
fio_brotli_compress_bound().

_Symbol type:_ `function`

#### `fio_brotli_compress_bound`

```c
inline size_t fio_brotli_compress_bound(size_t in_len)
```

Upper bound on Brotli compressed output size.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-162-deflate-h"></a> `./fio-stl/162 deflate.h`

11 public symbols.

### Types

#### `fio_deflate_s`

```c
struct fio_deflate_s
```

Opaque streaming deflate/inflate state.

_Symbol type:_ `type`

### Functions

#### `fio_deflate_decompress`

```c
size_t fio_deflate_decompress(void *out, size_t out_len, const void *in, size_t in_len)
```

Decompresses raw DEFLATE data (no zlib/gzip headers).

Returns:
 - On success: decompressed byte count (<= out_len).
 - On buffer too small: the REQUIRED buffer size (> out_len).
 - On corrupt/invalid data: 0.
 - When out==NULL or out_len==0: performs a full decode pass counting
   output bytes and returns the required size (0 if data is corrupt).
   This allows callers to query the decompressed size without allocating.

_Symbol type:_ `function`

#### `fio_deflate_decompress_bound`

```c
inline size_t fio_deflate_decompress_bound(size_t in_len)
```

Conservative upper bound on decompressed size.

_Symbol type:_ `function`

#### `fio_deflate_compress`

```c
size_t fio_deflate_compress(void *out, size_t out_len, const void *in, size_t in_len, int level)
```

Compresses data using raw DEFLATE (no zlib/gzip headers).

Returns compressed length on success, 0 on error.
level: 0=store, 1-3=fast, 4-6=normal, 7-9=best compression.

_Symbol type:_ `function`

#### `fio_deflate_compress_bound`

```c
inline size_t fio_deflate_compress_bound(size_t in_len)
```

Upper bound on compressed output size.

_Symbol type:_ `function`

#### `fio_gzip_compress`

```c
size_t fio_gzip_compress(void *out, size_t out_len, const void *in, size_t in_len, int level)
```

Compresses data with gzip wrapper (for HTTP Content-Encoding).

Returns total output length on success, 0 on error.

_Symbol type:_ `function`

#### `fio_gzip_decompress`

```c
size_t fio_gzip_decompress(void *out, size_t out_len, const void *in, size_t in_len)
```

Decompresses gzip data.

Returns:
 - On success: decompressed byte count (<= out_len).
 - On buffer too small: the REQUIRED buffer size (> out_len).
 - On corrupt/invalid data: 0.
 - When out==NULL or out_len==0: returns required size (0 if corrupt).

_Symbol type:_ `function`

#### `fio_deflate_new`

```c
fio_deflate_s *fio_deflate_new(int level, int is_compress)
```

Creates a new streaming deflate/inflate state.

`level`:       compression level 1-9 (ignored for decompression).
`is_compress`: non-zero for compression, 0 for decompression.

Returns NULL on allocation failure.

_Symbol type:_ `function`

#### `fio_deflate_free`

```c
void fio_deflate_free(fio_deflate_s *s)
```

Frees a streaming deflate/inflate state.

_Symbol type:_ `function`

#### `fio_deflate_destroy`

```c
void fio_deflate_destroy(fio_deflate_s *s)
```

Resets a deflate streaming context (keeps allocated memory, clears state).

_Symbol type:_ `function`

#### `fio_deflate_push`

```c
size_t fio_deflate_push(fio_deflate_s *s, void *out, size_t out_len, const void *in, size_t in_len, int flush)
```

Streaming compress/decompress.

Processes `in_len` bytes from `in`, writing output to `out` (max `out_len`).
`flush`: 0=normal, 1=sync_flush (for WebSocket frame boundaries).

Decompression returns:
- Decompressed byte count on success (<= out_len).
- Required buffer size when buffer too small (> out_len). Internal buffer is
  preserved for retry with a larger output buffer.
- 0 on corrupt/invalid data or allocation failure.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-190-tls13-h"></a> `./fio-stl/190 tls13.h`

60 public symbols.

### Macros

#### `FIO_TLS13_SHA256_HASH_LEN`

```c
#define FIO_TLS13_SHA256_HASH_LEN 32
```

SHA-256 hash length (32 bytes).

_Symbol type:_ `macro`

#### `FIO_TLS13_SHA384_HASH_LEN`

```c
#define FIO_TLS13_SHA384_HASH_LEN 48
```

SHA-384 hash length (48 bytes).

_Symbol type:_ `macro`

#### `FIO_TLS13_MAX_HASH_LEN`

```c
#define FIO_TLS13_MAX_HASH_LEN 48
```

Maximum hash length supported.

_Symbol type:_ `macro`

#### `FIO_TLS13_AES128_KEY_LEN`

```c
#define FIO_TLS13_AES128_KEY_LEN 16
```

AES-128-GCM key length.

_Symbol type:_ `macro`

#### `FIO_TLS13_AES256_KEY_LEN`

```c
#define FIO_TLS13_AES256_KEY_LEN 32
```

AES-256-GCM key length.

_Symbol type:_ `macro`

#### `FIO_TLS13_CHACHA_KEY_LEN`

```c
#define FIO_TLS13_CHACHA_KEY_LEN 32
```

ChaCha20-Poly1305 key length.

_Symbol type:_ `macro`

#### `FIO_TLS13_IV_LEN`

```c
#define FIO_TLS13_IV_LEN 12
```

IV length for all AEAD ciphers.

_Symbol type:_ `macro`

#### `FIO_TLS13_RECORD_HEADER_LEN`

```c
#define FIO_TLS13_RECORD_HEADER_LEN 5
```

TLS record header length (5 bytes)

_Symbol type:_ `macro`

#### `FIO_TLS13_MAX_PLAINTEXT_LEN`

```c
#define FIO_TLS13_MAX_PLAINTEXT_LEN 16384
```

Maximum plaintext fragment length (2^14 = 16384 bytes)

_Symbol type:_ `macro`

#### `FIO_TLS13_MAX_CIPHERTEXT_LEN`

```c
#define FIO_TLS13_MAX_CIPHERTEXT_LEN (16384 + 256)
```

Maximum ciphertext length (plaintext + padding + tag)

_Symbol type:_ `macro`

#### `FIO_TLS13_TAG_LEN`

```c
#define FIO_TLS13_TAG_LEN 16
```

AEAD authentication tag length (16 bytes for all TLS 1.3 ciphers)

_Symbol type:_ `macro`

#### `FIO_TLS13_LEGACY_VERSION_MAJOR`

```c
#define FIO_TLS13_LEGACY_VERSION_MAJOR 0x03
```

Legacy TLS version bytes (0x0303 = TLS 1.2)

_Symbol type:_ `macro`

#### `FIO_TLS13_LEGACY_VERSION_MINOR`

```c
#define FIO_TLS13_LEGACY_VERSION_MINOR 0x03
```



_Symbol type:_ `macro`

#### `FIO_TLS13_VERSION_TLS12`

```c
#define FIO_TLS13_VERSION_TLS12 0x0303
```

TLS 1.3 Protocol Version Constants

_Symbol type:_ `macro`

#### `FIO_TLS13_VERSION_TLS13`

```c
#define FIO_TLS13_VERSION_TLS13 0x0304
```



_Symbol type:_ `macro`

#### `FIO_TLS13_HRR_RANDOM`

```c
#define FIO_TLS13_HRR_RANDOM   \
  "\xCF\x21\xAD\x74\xE5\x9A\x61\x11\xBE\x1D\x8C\x02\x1E\x65\xB8\x91"   \
  "\xC2\xA2\x11\x16\x7A\xBB\x8C\x5E\x07\x9E\x09\xE2\xC8\xA8\x33\x9C"
```

HelloRetryRequest magic random value (RFC 8446 Section 4.1.3)

_Symbol type:_ `macro`

### Types

#### `fio_tls13_content_type_e`

```c
typedef enum {
FIO_TLS13_CONTENT_INVALID = 0,
FIO_TLS13_CONTENT_CHANGE_CIPHER_SPEC = 20, /* Legacy, ignored in TLS 1.3 */
FIO_TLS13_CONTENT_ALERT = 21,
FIO_TLS13_CONTENT_HANDSHAKE = 22,
FIO_TLS13_CONTENT_APPLICATION_DATA = 23,
} fio_tls13_content_type_e
```

TLS 1.3 content types

_Symbol type:_ `type`

#### `fio_tls13_handshake_type_e`

```c
typedef enum {
FIO_TLS13_HS_CLIENT_HELLO = 1,
FIO_TLS13_HS_SERVER_HELLO = 2,
FIO_TLS13_HS_NEW_SESSION_TICKET = 4,
FIO_TLS13_HS_END_OF_EARLY_DATA = 5,
FIO_TLS13_HS_ENCRYPTED_EXTENSIONS = 8,
FIO_TLS13_HS_CERTIFICATE = 11,
FIO_TLS13_HS_CERTIFICATE_REQUEST = 13,
FIO_TLS13_HS_CERTIFICATE_VERIFY = 15,
FIO_TLS13_HS_FINISHED = 20,
FIO_TLS13_HS_KEY_UPDATE = 24,
FIO_TLS13_HS_MESSAGE_HASH = 254,
} fio_tls13_handshake_type_e
```

TLS 1.3 Handshake Message Types

_Symbol type:_ `type`

#### `fio_tls13_key_update_request_e`

```c
typedef enum {
FIO_TLS13_KEY_UPDATE_NOT_REQUESTED = 0,
FIO_TLS13_KEY_UPDATE_REQUESTED = 1,
} fio_tls13_key_update_request_e
```

TLS 1.3 KeyUpdate Request Types (RFC 8446 Section 4.6.3)

_Symbol type:_ `type`

#### `fio_tls13_extension_type_e`

```c
typedef enum {
FIO_TLS13_EXT_SERVER_NAME = 0, /* SNI */
FIO_TLS13_EXT_SUPPORTED_GROUPS = 10, /* Key exchange groups */
FIO_TLS13_EXT_SIGNATURE_ALGORITHMS = 13, /* Signature schemes */
FIO_TLS13_EXT_ALPN = 16, /* Application-Layer Protocol Negotiation */
FIO_TLS13_EXT_SUPPORTED_VERSIONS = 43, /* TLS version negotiation */
FIO_TLS13_EXT_COOKIE = 44, /* Cookie for HRR (RFC 8446 4.2.2) */
FIO_TLS13_EXT_CERTIFICATE_AUTHORITIES =
47, /* Acceptable CAs (RFC 8446 4.2.4) */
FIO_TLS13_EXT_SIGNATURE_ALGORITHMS_CERT = 50, /* Cert chain sig algs */
FIO_TLS13_EXT_KEY_SHARE = 51, /* ECDHE key shares */
} fio_tls13_extension_type_e
```

TLS 1.3 Extension Types (RFC 8446 Section 4.2)

_Symbol type:_ `type`

#### `fio_tls13_cipher_suite_e`

```c
typedef enum {
FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256 = 0x1301,
FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384 = 0x1302,
FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256 = 0x1303,
} fio_tls13_cipher_suite_e
```

TLS 1.3 Cipher Suites (RFC 8446 Section B.4)

_Symbol type:_ `type`

#### `fio_tls13_named_group_e`

```c
typedef enum {
FIO_TLS13_GROUP_SECP256R1 = 23, /* P-256 */
FIO_TLS13_GROUP_SECP384R1 = 24, /* P-384 */
FIO_TLS13_GROUP_X25519 = 29, /* Curve25519 */
FIO_TLS13_GROUP_X25519MLKEM768 =
0x11ec, /* X25519 + ML-KEM-768 hybrid (PQC) */
} fio_tls13_named_group_e
```

TLS 1.3 Named Groups (RFC 8446 Section 4.2.7, draft-ietf-tls-hybrid-design)

_Symbol type:_ `type`

#### `fio_tls13_signature_scheme_e`

```c
typedef enum {
FIO_TLS13_SIG_RSA_PKCS1_SHA256 = 0x0401,
FIO_TLS13_SIG_RSA_PKCS1_SHA384 = 0x0501,
FIO_TLS13_SIG_RSA_PKCS1_SHA512 = 0x0601,
FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256 = 0x0403,
FIO_TLS13_SIG_ECDSA_SECP384R1_SHA384 = 0x0503,
FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256 = 0x0804,
FIO_TLS13_SIG_RSA_PSS_RSAE_SHA384 = 0x0805,
FIO_TLS13_SIG_ED25519 = 0x0807,
} fio_tls13_signature_scheme_e
```

TLS 1.3 Signature Algorithms (RFC 8446 Section 4.2.3)

_Symbol type:_ `type`

#### `fio_tls13_server_hello_s`

```c
typedef struct {
uint8_t random[32]; /* Server random */
uint16_t cipher_suite; /* Selected cipher suite */
uint8_t
key_share[1120]; /* Server's key share (max size for X25519MLKEM768) */
uint16_t key_share_len; /* Length of key share */
uint16_t key_share_group; /* Selected group */
int is_hello_retry_request; /* 1 if HRR */
} fio_tls13_server_hello_s
```

Parsed ServerHello message

_Symbol type:_ `type`

#### `fio_tls13_encrypted_extensions_s`

```c
typedef struct {
int has_server_name; /* Server acknowledged SNI */
char alpn_selected[256]; /* Selected ALPN protocol (null-terminated) */
size_t alpn_selected_len; /* Length of selected ALPN protocol */
} fio_tls13_encrypted_extensions_s
```

Parsed EncryptedExtensions message

_Symbol type:_ `type`

#### `fio_tls13_certificate_s`

```c
typedef struct {
const uint8_t *cert_data; /* Pointer to first certificate */
size_t cert_len; /* Length of first certificate */
} fio_tls13_certificate_s
```

Parsed Certificate message (minimal - first cert only)

_Symbol type:_ `type`

#### `fio_tls13_certificate_verify_s`

```c
typedef struct {
uint16_t signature_scheme;
const uint8_t *signature;
size_t signature_len;
} fio_tls13_certificate_verify_s
```

Parsed CertificateVerify message

_Symbol type:_ `type`

#### `fio_tls13_certificate_request_s`

```c
typedef struct {
uint8_t certificate_request_context[255]; /* Opaque context */
size_t certificate_request_context_len; /* Context length (0-255) */
uint16_t signature_algorithms[16]; /* Required signature algorithms */
size_t signature_algorithm_count; /* Number of signature algorithms */
uint16_t signature_algorithms_cert[16]; /* Cert chain sig algs (optional) */
size_t signature_algorithms_cert_count; /* Number of cert sig algs */
/* Certificate authorities (optional, pointers into original data) */
const uint8_t *certificate_authorities; /* Raw CA DNs data */
size_t certificate_authorities_len; /* Total CA DNs length */
} fio_tls13_certificate_request_s
```

Parsed CertificateRequest message (RFC 8446 Section 4.3.2)

_Symbol type:_ `type`

#### `fio_tls13_cipher_type_e`

```c
typedef enum {
FIO_TLS13_CIPHER_AES_128_GCM = 0, /* TLS_AES_128_GCM_SHA256 */
FIO_TLS13_CIPHER_AES_256_GCM = 1, /* TLS_AES_256_GCM_SHA384 */
FIO_TLS13_CIPHER_CHACHA20_POLY1305 = 2, /* TLS_CHACHA20_POLY1305_SHA256 */
} fio_tls13_cipher_type_e
```

Supported AEAD cipher types for TLS 1.3

_Symbol type:_ `type`

#### `fio_tls13_plaintext_header_s`

```c
typedef struct {
uint8_t content_type; /* ContentType */
uint8_t legacy_version[2]; /* 0x03, 0x03 (TLS 1.2) */
uint16_t length; /* Fragment length (big-endian) */
/* Fragment follows (up to 2^14 bytes) */
} fio_tls13_plaintext_header_s
```

TLSPlaintext header structure (RFC 8446 Section 5.1)

_Symbol type:_ `type`

#### `fio_tls13_ciphertext_header_s`

```c
typedef struct {
uint8_t opaque_type; /* Always 23 (application_data) */
uint8_t legacy_version[2]; /* 0x03, 0x03 */
uint16_t length; /* Encrypted length + tag (big-endian) */
/* Encrypted content follows */
} fio_tls13_ciphertext_header_s
```

TLSCiphertext header structure (RFC 8446 Section 5.2)

_Symbol type:_ `type`

#### `fio_tls13_record_keys_s`

```c
typedef struct {
uint8_t key[32]; /* Write key (16 or 32 bytes depending on cipher) */
uint8_t iv[12]; /* Write IV (always 12 bytes) */
uint64_t sequence_number; /* Per-record sequence number (starts at 0) */
uint8_t key_len; /* 16 for AES-128, 32 for AES-256/ChaCha20 */
uint8_t cipher_type; /* fio_tls13_cipher_type_e */
} fio_tls13_record_keys_s
```

Record encryption context (per-direction keys)

_Symbol type:_ `type`

### Functions

#### `fio_tls13_hkdf_expand_label`

```c
void fio_tls13_hkdf_expand_label(void *restrict out, size_t out_len, const void *restrict secret, size_t secret_len, const char *label, size_t label_len, const void *restrict context, size_t context_len, int use_sha384)
```

TLS 1.3 HKDF-Expand-Label function.

Derives keying material using the TLS 1.3 specific label format.

**Parameters:**
- `out` - Output buffer for derived key material
- `out_len` - Desired output length (max 255)
- `secret` - The secret to expand (PRK from HKDF-Extract)
- `secret_len` - Secret length (32 for SHA-256, 48 for SHA-384)
- `label` - The label string (without "tls13 " prefix)
- `label_len` - Label length (max 249 to fit in 255 with prefix)
- `context` - Optional context (transcript hash or empty)
- `context_len` - Context length (max 255)
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_tls13_derive_secret`

```c
void fio_tls13_derive_secret(void *restrict out, const void *restrict secret, size_t secret_len, const char *label, size_t label_len, const void *restrict transcript_hash, size_t hash_len, int use_sha384)
```

TLS 1.3 Derive-Secret function.

Derives a secret from a base secret and transcript hash.

**Parameters:**
- `out` - Output buffer (32 bytes for SHA-256, 48 for SHA-384)
- `secret` - The base secret
- `secret_len` - Secret length
- `label` - The label string (e.g., "c hs traffic")
- `label_len` - Label length
- `transcript_hash` - Hash of handshake messages (or empty hash for "")
- `hash_len` - Hash length (32 or 48)
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_tls13_derive_early_secret`

```c
void fio_tls13_derive_early_secret(void *restrict early_secret, const void *restrict psk, size_t psk_len, int use_sha384)
```

Derive the Early Secret from PSK.

Early Secret = HKDF-Extract(salt=0, IKM=PSK)

**Parameters:**
- `early_secret` - Output buffer (32 or 48 bytes)
- `psk` - Pre-shared key (or NULL/zeros for no PSK)
- `psk_len` - PSK length (0 if no PSK)
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_tls13_derive_handshake_secret`

```c
void fio_tls13_derive_handshake_secret(void *restrict handshake_secret, const void *restrict early_secret, const void *restrict ecdhe_secret, size_t ecdhe_len, int use_sha384)
```

Derive the Handshake Secret from ECDHE shared secret.

Handshake Secret = HKDF-Extract(
    salt=Derive-Secret(Early Secret, "derived", ""),
    IKM=ECDHE shared secret
)

**Parameters:**
- `handshake_secret` - Output buffer (32 or 48 bytes)
- `early_secret` - The early secret (from fio_tls13_derive_early_secret)
- `ecdhe_secret` - The ECDHE shared secret (e.g., from X25519)
- `ecdhe_len` - ECDHE secret length (32 for X25519)
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_tls13_derive_master_secret`

```c
void fio_tls13_derive_master_secret(void *restrict master_secret, const void *restrict handshake_secret, int use_sha384)
```

Derive the Master Secret.

Master Secret = HKDF-Extract(
    salt=Derive-Secret(Handshake Secret, "derived", ""),
    IKM=0
)

**Parameters:**
- `master_secret` - Output buffer (32 or 48 bytes)
- `handshake_secret` - The handshake secret
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_tls13_derive_traffic_keys`

```c
void fio_tls13_derive_traffic_keys(void *restrict key, size_t key_len, void *restrict iv, const void *restrict traffic_secret, int use_sha384)
```

Derive traffic keys and IV from a traffic secret.

**Parameters:**
- `key` - Output buffer for write key
- `key_len` - Key length (16 for AES-128, 32 for AES-256/ChaCha20)
- `iv` - Output buffer for write IV (12 bytes)
- `traffic_secret` - The traffic secret (client/server handshake/app)
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_tls13_derive_finished_key`

```c
void fio_tls13_derive_finished_key(void *restrict finished_key, const void *restrict traffic_secret, int use_sha384)
```

Derive the Finished key from a traffic secret.

finished_key = HKDF-Expand-Label(BaseKey, "finished", "", Hash.length)

**Parameters:**
- `finished_key` - Output buffer (32 or 48 bytes)
- `traffic_secret` - The handshake traffic secret
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_tls13_compute_finished`

```c
void fio_tls13_compute_finished(void *restrict verify_data, const void *restrict finished_key, const void *restrict transcript_hash, int use_sha384)
```

Compute the Finished verify_data.

verify_data = HMAC(finished_key, Transcript-Hash(Handshake Context))

**Parameters:**
- `verify_data` - Output buffer (32 or 48 bytes)
- `finished_key` - The finished key (from fio_tls13_derive_finished_key)
- `transcript_hash` - Hash of handshake messages up to this point
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_tls13_update_traffic_secret`

```c
void fio_tls13_update_traffic_secret(void *restrict new_secret, const void *restrict current_secret, int use_sha384)
```

Update application traffic secret for key update.

application_traffic_secret_N+1 =
    HKDF-Expand-Label(application_traffic_secret_N, "traffic upd", "",
Hash.length)

**Parameters:**
- `new_secret` - Output buffer (32 or 48 bytes)
- `current_secret` - Current application traffic secret
- `use_sha384` - If non-zero, use SHA-384; otherwise SHA-256

_Symbol type:_ `function`

#### `fio_tls13_build_nonce`

```c
inline void fio_tls13_build_nonce(uint8_t nonce[12], const uint8_t iv[12], uint64_t seq)
```

Build per-record nonce by XORing sequence number with IV.

Per RFC 8446 Section 5.3:
- Pad 64-bit sequence number to 12 bytes (big-endian, left-padded with zeros)
- XOR with the static IV derived from traffic secret

**Parameters:**
- `nonce` - Output buffer (must be 12 bytes)
- `iv` - Static IV from key derivation (12 bytes)
- `seq` - 64-bit sequence number

_Symbol type:_ `function`

#### `fio_tls13_record_parse_header`

```c
const uint8_t *fio_tls13_record_parse_header( const uint8_t *data, size_t data_len, fio_tls13_content_type_e *content_type, size_t *payload_len)
```

Parse a TLS record header.

**Parameters:**
- `data` - Input buffer containing record data
- `data_len` - Length of input buffer
- `content_type` - Output: content type from header
- `payload_len` - Output: payload length from header

**Returns:**
- Pointer to payload data, or NULL if incomplete/invalid

_Symbol type:_ `function`

#### `fio_tls13_record_encrypt`

```c
int fio_tls13_record_encrypt(uint8_t *out, size_t out_capacity, const uint8_t *plaintext, size_t plaintext_len, fio_tls13_content_type_e content_type, fio_tls13_record_keys_s *keys)
```

Encrypt a TLS 1.3 record.

Per RFC 8446 Section 5.2:
- Output format: 5-byte header + encrypted(plaintext + content_type) + tag
- AAD is the 5-byte record header
- Sequence number is incremented after encryption

**Parameters:**
- `out` - Output buffer for encrypted record
- `out_capacity` - Capacity of output buffer
- `plaintext` - Plaintext data to encrypt
- `plaintext_len` - Length of plaintext
- `content_type` - Content type (appended to plaintext before encryption)
- `keys` - Encryption keys (sequence number will be incremented)

**Returns:**
- Total output length (header + ciphertext + tag), or -1 on error

_Symbol type:_ `function`

#### `fio_tls13_record_decrypt`

```c
int fio_tls13_record_decrypt(uint8_t *out, size_t out_capacity, fio_tls13_content_type_e *content_type, const uint8_t *ciphertext, size_t ciphertext_len, fio_tls13_record_keys_s *keys)
```

Decrypt a TLS 1.3 record.

Per RFC 8446 Section 5.2:
- Input includes 5-byte header
- Decrypts and verifies AEAD tag
- Scans backwards to find real content type (removes padding)
- Sequence number is incremented after successful decryption

Note: decryption is performed in-place in the ciphertext buffer, so the
ciphertext buffer will be modified. The output buffer only needs to hold
the plaintext (not the internal content type byte).

**Parameters:**
- `out` - Output buffer for decrypted plaintext
- `out_capacity` - Capacity of output buffer (>= plaintext length)
- `content_type` - Output: actual content type from inner plaintext
- `ciphertext` - Input ciphertext (includes 5-byte header, modified)
- `ciphertext_len` - Total length including header
- `keys` - Decryption keys (sequence number will be incremented)

**Returns:**
- Plaintext length (excluding padding and content type), or -1 on error

_Symbol type:_ `function`

#### `fio_tls13_record_keys_init`

```c
void fio_tls13_record_keys_init(fio_tls13_record_keys_s *keys, const uint8_t *key, uint8_t key_len, const uint8_t iv[12], fio_tls13_cipher_type_e cipher_type)
```

Initialize record keys structure.

**Parameters:**
- `keys` - Keys structure to initialize
- `key` - Key material (16 or 32 bytes)
- `key_len` - Key length (16 for AES-128, 32 for AES-256/ChaCha20)
- `iv` - IV material (12 bytes)
- `cipher_type` - Cipher type (fio_tls13_cipher_type_e)

_Symbol type:_ `function`

#### `fio_tls13_record_keys_clear`

```c
inline void fio_tls13_record_keys_clear(fio_tls13_record_keys_s *keys)
```

Clear sensitive key material from memory.

**Parameters:**
- `keys` - Keys structure to clear

_Symbol type:_ `function`

#### `fio_tls13_parse_handshake_header`

```c
const uint8_t *fio_tls13_parse_handshake_header( const uint8_t *data, size_t data_len, fio_tls13_handshake_type_e *msg_type, size_t *body_len)
```

Parse handshake header, return message type and body pointer.

Returns pointer to message body, or NULL on error.
Sets msg_type and body_len on success.

_Symbol type:_ `function`

#### `fio_tls13_write_handshake_header`

```c
void fio_tls13_write_handshake_header(uint8_t *out, fio_tls13_handshake_type_e msg_type, size_t body_len)
```

Write handshake header (4 bytes).

Format: HandshakeType (1 byte) + uint24 length (3 bytes)

_Symbol type:_ `function`

#### `fio_tls13_build_client_hello`

```c
int fio_tls13_build_client_hello(uint8_t *out, size_t out_capacity, const uint8_t random[32], const char *server_name, const uint8_t *x25519_pubkey, const uint16_t *cipher_suites, size_t cipher_suite_count)
```

Build a ClientHello message.

Returns: message length on success, -1 on error.

Parameters:
- out: output buffer
- out_capacity: size of output buffer
- random: 32-byte client random
- server_name: SNI hostname (NULL if not used)
- x25519_pubkey: 32-byte X25519 public key
- cipher_suites: array of cipher suites to offer
- cipher_suite_count: number of cipher suites

_Symbol type:_ `function`

#### `fio_tls13_parse_server_hello`

```c
int fio_tls13_parse_server_hello(fio_tls13_server_hello_s *out, const uint8_t *data, size_t data_len)
```

Parse ServerHello message.

Returns: 0 on success, -1 on error.

Note: data should point to the handshake body (after the 4-byte header).

_Symbol type:_ `function`

#### `fio_tls13_parse_encrypted_extensions`

```c
int fio_tls13_parse_encrypted_extensions( fio_tls13_encrypted_extensions_s *out, const uint8_t *data, size_t data_len)
```

Parse EncryptedExtensions message.

Returns: 0 on success, -1 on error.

_Symbol type:_ `function`

#### `fio_tls13_parse_certificate`

```c
int fio_tls13_parse_certificate(fio_tls13_certificate_s *out, const uint8_t *data, size_t data_len)
```

Parse Certificate message (extracts first certificate only).

Returns: 0 on success, -1 on error.

_Symbol type:_ `function`

#### `fio_tls13_parse_certificate_verify`

```c
int fio_tls13_parse_certificate_verify( fio_tls13_certificate_verify_s *out, const uint8_t *data, size_t data_len)
```

Parse CertificateVerify message.

Returns: 0 on success, -1 on error.

_Symbol type:_ `function`

#### `fio_tls13_build_finished`

```c
int fio_tls13_build_finished(uint8_t *out, size_t out_capacity, const uint8_t *verify_data, size_t verify_data_len)
```

Build Finished message.

verify_data = HMAC(finished_key, Transcript-Hash(Handshake Context))

Returns: message length on success, -1 on error.

_Symbol type:_ `function`

#### `fio_tls13_parse_finished`

```c
int fio_tls13_parse_finished(const uint8_t *data, size_t data_len, const uint8_t *expected_verify_data, size_t verify_data_len)
```

Parse and verify Finished message.

Returns: 0 on success (MAC matches), -1 on error.

_Symbol type:_ `function`

#### `fio_tls13_build_alert`

```c
inline int fio_tls13_build_alert(uint8_t *out, size_t out_capacity, uint8_t alert_level, uint8_t alert_desc)
```

Build an alert message (unencrypted, for use before handshake keys).

Alert format: [level:1][description:1]

**Parameters:**
- `out` - Output buffer for alert message (2 bytes minimum)
- `out_capacity` - Capacity of output buffer
- `alert_level` - Alert level (1=warning, 2=fatal)
- `alert_desc` - Alert description code

**Returns:**
- Alert message length (2), or -1 on error

_Symbol type:_ `function`

#### `fio_tls13_send_alert`

```c
int fio_tls13_send_alert(uint8_t *out, size_t out_capacity, uint8_t alert_level, uint8_t alert_desc, fio_tls13_record_keys_s *keys)
```

Build an encrypted alert record.

Per RFC 8446 Section 6, alerts are encrypted after handshake keys are
established. In TLS 1.3, all alerts except close_notify are effectively
fatal and the connection must be closed after sending.

**Parameters:**
- `out` - Output buffer for encrypted alert record
- `out_capacity` - Capacity of output buffer
- `alert_level` - Alert level (1=warning, 2=fatal)
- `alert_desc` - Alert description code
- `keys` - Encryption keys (sequence number will be incremented)

**Returns:**
- Encrypted record length, or -1 on error

_Symbol type:_ `function`

#### `fio_tls13_send_alert_plaintext`

```c
int fio_tls13_send_alert_plaintext(uint8_t *out, size_t out_capacity, uint8_t alert_level, uint8_t alert_desc)
```

Build an unencrypted alert record (for use before encryption is enabled).

**Parameters:**
- `out` - Output buffer for alert record
- `out_capacity` - Capacity of output buffer
- `alert_level` - Alert level (1=warning, 2=fatal)
- `alert_desc` - Alert description code

**Returns:**
- Record length (7 bytes: 5 header + 2 alert), or -1 on error

_Symbol type:_ `function`

#### `fio_tls13_alert_name`

```c
inline const char *fio_tls13_alert_name(uint8_t alert_desc)
```

Get human-readable name for an alert description.

**Parameters:**
- `alert_desc` - Alert description code

**Returns:**
- Static string with alert name

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-201-string-h"></a> `./fio-stl/201 string.h`

47 public symbols.

### Macros

#### `FIO_STR_OPTIMIZE_EMBEDDED`

```c
#define FIO_STR_OPTIMIZE_EMBEDDED 0
```

For each unit (0 by default), adds `sizeof(char *)` bytes to the type size,
increasing the amount of strings that could be embedded within the type
without additional memory allocation.

For example, when using a reference counter wrapper on a 64bit system, it
would make sense to set this value to 1 - allowing the type size to fully
utilize a 16 byte memory allocation alignment.

_Symbol type:_ `macro`

#### `FIO_STR_OPTIMIZE4IMMUTABILITY`

```c
#define FIO_STR_OPTIMIZE4IMMUTABILITY 0
```

Minimizes the struct size, storing only string length and pointer.

By avoiding extra (mutable related) data, such as the allocated memory's
capacity, strings require less memory. However, this does introduce a
performance penalty when editing the string data.

_Symbol type:_ `macro`

#### `FIO_STR_INIT`

```c
#define FIO_STR_INIT   \
  { .special = 0 }
```

This value should be used for initialization. For example:

     // on the stack
     fio_str_s str = FIO_STR_INIT;

     // or on the heap
     fio_str_s *str = malloc(sizeof(*str));
     *str = FIO_STR_INIT;

Remember to cleanup:

     // on the stack
     fio_str_destroy(&str);

     // or on the heap
     fio_str_free(str);
     free(str);

_Symbol type:_ `macro`

#### `FIO_STR_INIT_EXISTING`

```c
#define FIO_STR_INIT_EXISTING(buffer, length, capacity)   \
  { .capa = (capacity), .len = (length), .buf = (buffer) }
```

This macro allows the container to be initialized with existing data, as long
as it's memory was allocated with the same allocator (`malloc` /
`fio_malloc`).

The `capacity` value should exclude the NUL character (if exists).

NOTE: This macro isn't valid for FIO_STR_SMALL (or strings with the
FIO_STR_OPTIMIZE4IMMUTABILITY optimization)

_Symbol type:_ `macro`

#### `FIO_STR_INIT_STATIC`

```c
#define FIO_STR_INIT_STATIC(buffer)   \
  {   \
    .special = 4, .capa = FIO_STRLEN((buffer)), .len = FIO_STRLEN((buffer)),   \
    .buf = (char *)(buffer)   \
  }
```

This macro allows the container to be initialized with existing static data,
that shouldn't be freed.

NOTE: This macro isn't valid for FIO_STR_SMALL (or strings with the
FIO_STR_OPTIMIZE4IMMUTABILITY optimization)

_Symbol type:_ `macro`

#### `FIO_STR_INIT_STATIC2`

```c
#define FIO_STR_INIT_STATIC2(buffer, length)   \
  { .special = 4, .capa = (length), .len = (length), .buf = (char *)(buffer) }
```

This macro allows the container to be initialized with existing static data,
that shouldn't be freed.

NOTE: This macro isn't valid for FIO_STR_SMALL (or strings with the
FIO_STR_OPTIMIZE4IMMUTABILITY optimization)

_Symbol type:_ `macro`

#### `FIO_STR_WRITE2`

```c
#define FIO_STR_WRITE2(str_name, dest, ...)   \
  FIO_NAME(str_name, __write2)(dest, (fio_string_write_s[]){__VA_ARGS__, {0}})
```



_Symbol type:_ `macro`

### Types

#### `fiobj_str_s`

```c
typedef struct {
/* String flags:
*
* bit 1: small string.
* bit 2: frozen string.
* bit 3: static (non allocated) string (big strings only).
* bit 3-8: small string length (up to 64 bytes).
*/
uint8_t special;
uint8_t reserved[(sizeof(void *) * (1 + FIO_STR_OPTIMIZE_EMBEDDED)) -
(sizeof(uint8_t))]; /* padding length */
#if !FIO_STR_OPTIMIZE4IMMUTABILITY
size_t capa; /* known capacity for longer Strings */
size_t len; /* String length for longer Strings */
#endif /* FIO_STR_OPTIMIZE4IMMUTABILITY */
char *buf; /* pointer for longer Strings */
} fiobj_str_s
```

The `fio_str_s` type should be considered opaque.

The type's attributes should be accessed ONLY through the accessor
functions: `fio_str2cstr`, `fio_str_len`, `fio_str2ptr`, `fio_str_capa`,
etc'.

Note: when the `small` flag is present, the structure is ignored and used
as raw memory for a small String (no additional allocation). This changes
the String's behavior drastically and requires that the accessor functions
be used.

_Symbol type:_ `type`

### Functions

#### `fiobj_str_init_const`

```c
inline fio_str_info_s fiobj_str_init_const(FIO_STR_PTR s, const char *str, size_t len)
```

Initializes the container with the provided static / constant string.

The string will be copied to the container **only** if it will fit in the
container itself. Otherwise, the supplied pointer will be used as is and it
should remain valid until the string is destroyed.

The final string can be safely be destroyed (using the `destroy` function).

_Symbol type:_ `function`

#### `fiobj_str_init_copy`

```c
inline fio_str_info_s fiobj_str_init_copy(FIO_STR_PTR s, const char *str, size_t len)
```

Initializes the container with a copy of the provided dynamic string.

The string is always copied and the final string must be destroyed (using the
`destroy` function).

_Symbol type:_ `function`

#### `fiobj_str_init_copy2`

```c
inline fio_str_info_s fiobj_str_init_copy2(FIO_STR_PTR dest, FIO_STR_PTR src)
```

Initializes the container with a copy of an existing String object.

The string is always copied and the final string must be destroyed (using the
`destroy` function).

_Symbol type:_ `function`

#### `fiobj_str_destroy`

```c
inline void fiobj_str_destroy(FIO_STR_PTR s)
```

Frees the String's resources and re-initializes the container.

Note: if the container isn't allocated on the stack, it should be freed
separately using the appropriate `free` function.

_Symbol type:_ `function`

#### `fiobj_str_detach`

```c
inline char *fiobj_str_detach(FIO_STR_PTR s)
```

Returns a C string with the existing data, re-initializing the String.

Note: the String data is removed from the container, but the container
isn't freed.

Returns NULL if there's no String data.

NOTE: Returned string is ALWAYS dynamically allocated. Remember to free.

_Symbol type:_ `function`

#### `fiobj_str_dealloc`

```c
inline void fiobj_str_dealloc(void *ptr)
```

Frees the pointer returned by `detach`.

_Symbol type:_ `function`

#### `fiobj_str_info`

```c
inline fio_str_info_s fiobj_str_info(const FIO_STR_PTR s)
```

Returns the String's complete state (capacity, length and pointer).

_Symbol type:_ `function`

#### `fiobj_str_buf`

```c
inline fio_buf_info_s fiobj_str_buf(const FIO_STR_PTR s)
```

Returns the String's partial state (length and pointer).

_Symbol type:_ `function`

#### `fiobj_str_ptr`

```c
inline char *fiobj_str_ptr(FIO_STR_PTR s)
```

Returns a pointer (`char *`) to the String's content.

_Symbol type:_ `function`

#### `fiobj_str_len`

```c
inline size_t fiobj_str_len(FIO_STR_PTR s)
```

Returns the String's length in bytes.

_Symbol type:_ `function`

#### `fiobj_str_capa`

```c
inline size_t fiobj_str_capa(FIO_STR_PTR s)
```

Returns the String's existing capacity (total used & available memory).

_Symbol type:_ `function`

#### `fiobj_str_freeze`

```c
inline void fiobj_str_freeze(FIO_STR_PTR s)
```

Prevents further manipulations to the String's content.

_Symbol type:_ `function`

#### `fiobj_str_is_frozen`

```c
inline uint8_t fiobj_str_is_frozen(FIO_STR_PTR s)
```

Returns true if the string is frozen.

_Symbol type:_ `function`

#### `fiobj_str_is_allocated`

```c
inline int fiobj_str_is_allocated(const FIO_STR_PTR s)
```

Returns 1 if memory was allocated (and the String must be destroyed).

_Symbol type:_ `function`

#### `fiobj_str_is_eq`

```c
inline int fiobj_str_is_eq(const FIO_STR_PTR str1, const FIO_STR_PTR str2)
```

Binary comparison returns `1` if both strings are equal and `0` if not.

_Symbol type:_ `function`

#### `fiobj_str_hash`

```c
inline uint64_t fiobj_str_hash(const FIO_STR_PTR s, uint64_t seed)
```

Returns the string's Risky Hash value.

Note: Hash algorithm might change without notice.

_Symbol type:_ `function`

#### `fiobj_str_resize`

```c
inline fio_str_info_s fiobj_str_resize(FIO_STR_PTR s, size_t size)
```

Sets the new String size without reallocating any memory (limited by
existing capacity).

Returns the updated state of the String.

Note: When shrinking, any existing data beyond the new size may be
corrupted.

_Symbol type:_ `function`

#### `fiobj_str_compact`

```c
inline void fiobj_str_compact(FIO_STR_PTR s)
```

Performs a best attempt at minimizing memory consumption.

Actual effects depend on the underlying memory allocator and it's
implementation. Not all allocators will free any memory.

_Symbol type:_ `function`

#### `fiobj_str_reserve`

```c
fio_str_info_s fiobj_str_reserve(FIO_STR_PTR s, size_t amount)
```

Reserves (at least) `amount` of bytes for the string's data.

`amount` is in addition to existing String length.

Make sure to call `resize` with the updated information once the editing is
done.

Returns the updated state of the String.

_Symbol type:_ `function`

#### `fiobj_str_utf8_valid`

```c
size_t fiobj_str_utf8_valid(FIO_STR_PTR s)
```

Returns 1 if the String is UTF-8 valid and 0 if not.

_Symbol type:_ `function`

#### `fiobj_str_utf8_len`

```c
size_t fiobj_str_utf8_len(FIO_STR_PTR s)
```

Returns the String's length in UTF-8 characters.

_Symbol type:_ `function`

#### `fiobj_str_utf8_select`

```c
int fiobj_str_utf8_select(FIO_STR_PTR s, intptr_t *pos, size_t *len)
```

Takes a UTF-8 character selection information (UTF-8 position and length)
and updates the same variables so they reference the raw byte slice
information.

If the String isn't UTF-8 valid up to the requested selection, than `pos`
will be updated to `-1` otherwise values are always positive.

The returned `len` value may be shorter than the original if there wasn't
enough data left to accommodate the requested length. When a `len` value of
`0` is returned, this means that `pos` marks the end of the String.

Returns -1 on error and 0 on success.

_Symbol type:_ `function`

#### `fiobj_str_write`

```c
inline fio_str_info_s fiobj_str_write(FIO_STR_PTR s, const void *src, size_t src_len)
```

Writes data at the end of the String.

_Symbol type:_ `function`

#### `fiobj_str_concat`

```c
fio_str_info_s fiobj_str_concat(FIO_STR_PTR dest, FIO_STR_PTR const src)
```

Appends the `src` String to the end of the `dest` String.

If `dest` is empty, the resulting Strings will be equal.

_Symbol type:_ `function`

#### `fiobj_str_join`

```c
inline fio_str_info_s fiobj_str_join(FIO_STR_PTR dest, FIO_STR_PTR const src)
```

Alias for fio_str_concat

_Symbol type:_ `function`

#### `fiobj_str_replace`

```c
fio_str_info_s fiobj_str_replace(FIO_STR_PTR s, intptr_t start_pos, size_t old_len, const void *src, size_t src_len)
```

Replaces the data in the String - replacing `old_len` bytes starting at
`start_pos`, with the data at `src` (`src_len` bytes long).

Negative `start_pos` values are calculated backwards, `-1` == end of
String.

When `old_len` is zero, the function will insert the data at `start_pos`.

If `src_len == 0` than `src` will be ignored and the data marked for
replacement will be erased.

_Symbol type:_ `function`

#### `fiobj_str_write_i`

```c
fio_str_info_s fiobj_str_write_i(FIO_STR_PTR s, int64_t num)
```

Writes a number at the end of the String using normal base 10 notation.

_Symbol type:_ `function`

#### `fiobj_str_write_hex`

```c
fio_str_info_s fiobj_str_write_hex(FIO_STR_PTR s, int64_t num)
```

Writes a number at the end of the String using Hex (base 16) notation.

_Symbol type:_ `function`

#### `fiobj_str_write_bin`

```c
fio_str_info_s fiobj_str_write_bin(FIO_STR_PTR s, int64_t num)
```



_Symbol type:_ `function`

#### `fiobj_str_vprintf`

```c
fio_str_info_s fiobj_str_vprintf(FIO_STR_PTR s, const char *format, va_list argv)
```

Writes to the String using a vprintf like interface.

Data is written to the end of the String.

_Symbol type:_ `function`

#### `fiobj_str_printf`

```c
fio_str_info_s fiobj_str_printf(FIO_STR_PTR s, const char *format, ...)
```

Writes to the String using a printf like interface.

Data is written to the end of the String.

_Symbol type:_ `function`

#### `fiobj_str_write_escape`

```c
fio_str_info_s fiobj_str_write_escape(FIO_STR_PTR s, const void *data, size_t data_len)
```

Writes data at the end of the String, escaping the data using JSON semantics.

The JSON semantic are common to many programming languages, promising a UTF-8
String while making it easy to read and copy the string during debugging.

_Symbol type:_ `function`

#### `fiobj_str_write_unescape`

```c
fio_str_info_s fiobj_str_write_unescape(FIO_STR_PTR s, const void *escaped, size_t len)
```

Writes an escaped data into the string after unescaping the data.

_Symbol type:_ `function`

#### `fiobj_str_write_base64enc`

```c
fio_str_info_s fiobj_str_write_base64enc(FIO_STR_PTR s, const void *data, size_t data_len, uint8_t url_encoded)
```

Writes data at the end of the String, encoding the data as Base64 encoded
data.

_Symbol type:_ `function`

#### `fiobj_str_write_base64dec`

```c
fio_str_info_s fiobj_str_write_base64dec(FIO_STR_PTR s, const void *encoded, size_t encoded_len)
```

Writes decoded base64 data to the end of the String.

_Symbol type:_ `function`

#### `fiobj_str_write_html_escape`

```c
fio_str_info_s fiobj_str_write_html_escape(FIO_STR_PTR s, const void *raw, size_t len)
```

Writes HTML escaped data to a String.

_Symbol type:_ `function`

#### `fiobj_str_write_html_unescape`

```c
fio_str_info_s fiobj_str_write_html_unescape(FIO_STR_PTR s, const void *escaped, size_t len)
```

Writes HTML un-escaped data to a String - incomplete and minimal.

_Symbol type:_ `function`

#### `fiobj_str_readfd`

```c
fio_str_info_s fiobj_str_readfd(FIO_STR_PTR s, int fd, intptr_t start_at, intptr_t limit)
```

Reads data from a file descriptor `fd` at offset `start_at` and pastes it's
contents (or a slice of it) at the end of the String. If `limit == 0`, than
the data will be read until EOF.

The file should be a regular file or the operation might fail (can't be used
for sockets).

The file descriptor will remain open and should be closed manually.

_Symbol type:_ `function`

#### `fiobj_str_readfile`

```c
fio_str_info_s fiobj_str_readfile(FIO_STR_PTR s, const char *filename, intptr_t start_at, intptr_t limit)
```

Opens the file `filename` and pastes it's contents (or a slice ot it) at
the end of the String. If `limit == 0`, than the data will be read until
EOF.

If the file can't be located, opened or read, or if `start_at` is beyond
the EOF position, NULL is returned in the state's `data` field.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-202-array-h"></a> `./fio-stl/202 array.h`

30 public symbols.

### Macros

#### `FIO_ARRAY_NOT_FOUND`

```c
#define FIO_ARRAY_NOT_FOUND ((uint32_t)-1)
```



_Symbol type:_ `macro`

#### `FIO_ARRAY_TYPE`

```c
#define FIO_ARRAY_TYPE void *
```

The type for array elements (an array of FIO_ARRAY_TYPE)

_Symbol type:_ `macro`

#### `FIO_ARRAY_TYPE_INVALID`

```c
#define FIO_ARRAY_TYPE_INVALID        NULL
```

An invalid value for that type (if any).

_Symbol type:_ `macro`

#### `FIO_ARRAY_TYPE_INVALID_SIMPLE`

```c
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 0
```

Is the FIO_ARRAY_TYPE_INVALID object memory is all zero? (yes = 1)

_Symbol type:_ `macro`

#### `FIO_ARRAY_TYPE_COPY`

```c
#define FIO_ARRAY_TYPE_COPY(dest, src) (dest) = (src)
```

Handles a copy operation for an array's element.

_Symbol type:_ `macro`

#### `FIO_ARRAY_TYPE_DESTROY`

```c
#define FIO_ARRAY_TYPE_DESTROY(obj)
```

Handles a destroy / free operation for an array's element.

_Symbol type:_ `macro`

#### `FIO_ARRAY_TYPE_CMP`

```c
#define FIO_ARRAY_TYPE_CMP(a, b) (a) == (b)
```

Handles a comparison operation for an array's element.

_Symbol type:_ `macro`

#### `FIO_ARRAY_INIT`

```c
#define FIO_ARRAY_INIT   \
  { 0 }
```



_Symbol type:_ `macro`

#### `FIO_ARRAY_EACH`

```c
#define FIO_ARRAY_EACH(array_name, array, pos)   \
  for (FIO_NAME(array_name, ____type_t)   \
           *first___ai = NULL,   \
           *pos = FIO_NAME(array_name, each_next)((array), &first___ai, NULL);   \
       pos;   \
       pos = FIO_NAME(array_name, each_next)((array), &first___ai, pos))
```

Iterates through the array using a `for` loop.

Access the object with the pointer `pos`. The `pos` variable can be named
however you please.

Avoid editing the array during a FOR loop, although I hope it's possible, I
wouldn't count on it.

**Note**: this variant supports automatic pointer tagging / untagging.

_Symbol type:_ `macro`

### Types

#### `fiobj_array_s`

```c
struct fiobj_array_s {
/* start common header (with embedded array type) */
/** the offset to the first item. */
uint32_t start;
/** The offset to the first empty location the array. */
uint32_t end;
/* end common header (with embedded array type) */
/** The array's capacity - limited to 32bits, but we use the extra padding. */
uint32_t capa;
/** a pointer to the array's memory (if not embedded) */
FIO_ARRAY_TYPE *ary;
#if FIO_ARRAY_ENABLE_EMBEDDED > 1
/** Do we wanted larger small-array optimizations? */
FIO_ARRAY_TYPE
extra_memory_for_embedded_arrays[(FIO_ARRAY_ENABLE_EMBEDDED - 1)]
#endif
}
```

an Array type.

_Symbol type:_ `type`

#### `fiobj_array_each_s`

```c
struct fiobj_array_each_s {
/** The array iterated. Once set, cannot be safely changed. */
FIO_ARRAY_PTR const parent;
/** The current object's index */
uint64_t index;
/** The callback / task called for each index, may be updated mid-cycle. */
int (*task)(struct FIO_NAME(FIO_ARRAY_NAME, each_s) * info);
/** Opaque user data. */
void *udata;
/** The object / value at the current index. */
FIO_ARRAY_TYPE value;
/* memory padding used for FIOBJ */
uint64_t padding;
}
```

Iteration information structure passed to the callback.

_Symbol type:_ `type`

### Functions

#### `fiobj_array_destroy`

```c
void fiobj_array_destroy(FIO_ARRAY_PTR ary)
```



_Symbol type:_ `function`

#### `fiobj_array_count`

```c
inline uint32_t fiobj_array_count(FIO_ARRAY_PTR ary)
```

Returns the number of elements in the Array.

_Symbol type:_ `function`

#### `fiobj_array_capa`

```c
inline uint32_t fiobj_array_capa(FIO_ARRAY_PTR ary)
```

Returns the current, temporary, array capacity (it's dynamic).

_Symbol type:_ `function`

#### `fiobj_array_is_embedded`

```c
inline int fiobj_array_is_embedded(FIO_ARRAY_PTR ary)
```

Returns 1 if the array is embedded, 0 if it has memory allocated and -1 on an
error.

_Symbol type:_ `function`

#### `fiobj_array2ptr`

```c
inline FIO_ARRAY_TYPE *fiobj_array2ptr(FIO_ARRAY_PTR ary)
```

Returns a pointer to the C array containing the objects.

_Symbol type:_ `function`

#### `fiobj_array_reserve`

```c
uint32_t fiobj_array_reserve(FIO_ARRAY_PTR ary, int64_t capa)
```

Reserves a minimal capacity for additional elements to be added to the array.

If `capa` is negative, new memory will be allocated at the beginning of the
array rather then it's end.

Returns the array's new capacity.

_Symbol type:_ `function`

#### `fiobj_array_concat`

```c
FIO_ARRAY_PTR fiobj_array_concat(FIO_ARRAY_PTR dest, FIO_ARRAY_PTR src)
```

Adds all the items in the `src` Array to the end of the `dest` Array.

The `src` Array remain untouched.

Always returns the destination array (`dest`).

_Symbol type:_ `function`

#### `fiobj_array_set`

```c
FIO_ARRAY_TYPE *fiobj_array_set(FIO_ARRAY_PTR ary, int64_t index, FIO_ARRAY_TYPE data, FIO_ARRAY_TYPE *old)
```

Sets `index` to the value in `data`.

If `index` is negative, it will be counted from the end of the Array (-1 ==
last element).

If `old` isn't NULL, the existing data will be copied to the location pointed
to by `old` before the copy in the Array is destroyed.

Returns a pointer to the new object, or NULL on error.

_Symbol type:_ `function`

#### `fiobj_array_get`

```c
inline FIO_ARRAY_TYPE fiobj_array_get(FIO_ARRAY_PTR ary, int64_t index)
```

Returns the value located at `index` (no copying is performed).

If `index` is negative, it will be counted from the end of the Array (-1 ==
last element).

_Symbol type:_ `function`

#### `fiobj_array_find`

```c
uint32_t fiobj_array_find(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE data, int64_t start_at)
```

Returns the index of the object or (uint32_t)-1 if the object wasn't found.

If `start_at` is negative (i.e., -1), than seeking will be performed in
reverse, where -1 == last index (-2 == second to last, etc').

_Symbol type:_ `function`

#### `fiobj_array_remove`

```c
int fiobj_array_remove(FIO_ARRAY_PTR ary, int64_t index, FIO_ARRAY_TYPE *old)
```

Removes an object from the array, MOVING all the other objects to prevent
"holes" in the data.

If `old` is set, the data is copied to the location pointed to by `old`
before the data in the array is destroyed.

Returns 0 on success and -1 on error.

This action is O(n) where n in the length of the array.
It could get expensive.

_Symbol type:_ `function`

#### `fiobj_array_remove2`

```c
uint32_t fiobj_array_remove2(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE data)
```

Removes all occurrences of an object from the array (if any), MOVING all the
existing objects to prevent "holes" in the data.

Returns the number of items removed.

This action is O(n) where n in the length of the array.
It could get expensive.

_Symbol type:_ `function`

#### `fiobj_array_compact`

```c
void fiobj_array_compact(FIO_ARRAY_PTR ary)
```

Attempts to lower the array's memory consumption.

_Symbol type:_ `function`

#### `fiobj_array_push`

```c
FIO_ARRAY_TYPE *fiobj_array_push(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE data)
```

Pushes an object to the end of the Array. Returns a pointer to the new object
or NULL on error.

_Symbol type:_ `function`

#### `fiobj_array_pop`

```c
int fiobj_array_pop(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE *old)
```

Removes an object from the end of the Array.

If `old` is set, the data is copied to the location pointed to by `old`
before the data in the array is destroyed.

Returns -1 on error (Array is empty) and 0 on success.

_Symbol type:_ `function`

#### `fiobj_array_unshift`

```c
FIO_ARRAY_TYPE *fiobj_array_unshift(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE data)
```

Unshifts an object to the beginning of the Array. Returns a pointer to the
new object or NULL on error.

This could be expensive, causing `memmove`.

_Symbol type:_ `function`

#### `fiobj_array_shift`

```c
int fiobj_array_shift(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE *old)
```

Removes an object from the beginning of the Array.

If `old` is set, the data is copied to the location pointed to by `old`
before the data in the array is destroyed.

Returns -1 on error (Array is empty) and 0 on success.

_Symbol type:_ `function`

#### `fiobj_array_each`

```c
uint32_t fiobj_array_each(FIO_ARRAY_PTR ary, int (*task)(FIO_NAME(FIO_ARRAY_NAME, each_s) * info), void *udata, int64_t start_at)
```

Iteration using a callback for each entry in the array.

The callback task function must accept an each_s pointer, see above.

If the callback returns -1, the loop is broken. Any other value is ignored.

Returns the relative "stop" position, i.e., the number of items processed +
the starting point.

_Symbol type:_ `function`

#### `fiobj_array_each_next`

```c
inline FIO_ARRAY_TYPE *fiobj_array_each_next(FIO_ARRAY_PTR ary, FIO_ARRAY_TYPE **first, FIO_ARRAY_TYPE *pos)
```

Returns a pointer to the (next) object in the array.

Returns a pointer to the first object if `pos == NULL` and there are objects
in the array.

The first pointer is automatically set and it allows object insertions and
memory effecting functions to be called from within the loop.

If the object in `pos` (or an object before it) were removed, consider
passing `pos-1` to the function, to avoid skipping any elements while
looping.

Returns the next object if both `first` and `pos` are valid.

Returns NULL if `pos` was the last object or no object exist.

Returns the first object if either `first` or `pos` are invalid.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-210-map-h"></a> `./fio-stl/210 map.h`

30 public symbols.

### Macros

#### `FIO_MAP_INIT`

```c
#define FIO_MAP_INIT   \
  { 0 }
```



_Symbol type:_ `macro`

#### `FIO_MAP_EACH`

```c
#define FIO_MAP_EACH(map_name, map_ptr, i)   \
  for (FIO_NAME(map_name, iterator_s)   \
           i = FIO_NAME(map_name, get_next)(map_ptr, NULL);   \
       FIO_NAME(map_name, iterator_is_valid)(&i);   \
       i = FIO_NAME(map_name, get_next)(map_ptr, &i))
```

Iterates through the map using an iterator object.

_Symbol type:_ `macro`

#### `FIO_MAP_EACH_REVERSED`

```c
#define FIO_MAP_EACH_REVERSED(map_name, map_ptr, i)   \
  for (FIO_NAME(map_name, iterator_s)   \
           i = FIO_NAME(map_name, get_prev)(map_ptr, NULL);   \
       FIO_NAME(map_name, iterator_is_valid)(&i);   \
       i = FIO_NAME(map_name, get_prev)(map_ptr, &i))
```

Iterates through the map using an iterator object.

_Symbol type:_ `macro`

### Types

#### `fiobj_hash_node_s`

```c
struct fiobj_hash_node_s
```

internal object data representation

_Symbol type:_ `type`

#### `fiobj_hash_s`

```c
struct fiobj_hash_s {
uint32_t bits;
uint32_t count;
FIO_NAME(FIO_MAP_NAME, node_s) * map;
#if FIO_MAP_ORDERED
FIO_INDEXED_LIST32_HEAD head;
#endif
}
```

A Hash Map / Set type

_Symbol type:_ `type`

#### `fiobj_hash_iterator_s`

```c
typedef struct {
/** the node in the internal map */
fiobj_hash_iterator_s * node;
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
struct { /* internal usage, do not access */
uint32_t index; /* the index in the internal map */
uint32_t pos; /* the position in the ordering scheme */
uintptr_t map_validator; /* map mutation guard */
} private_;
} fiobj_hash_iterator_s
```

Map iterator type

_Symbol type:_ `type`

#### `fiobj_hash_each_s`

```c
struct fiobj_hash_each_s {
/** The being iterated. Once set, cannot be safely changed. */
FIO_MAP_PTR const parent;
/** The current object's index */
uint64_t index;
/** The callback / task called for each index, may be updated mid-cycle. */
int (*task)(struct FIO_NAME(FIO_MAP_NAME, each_s) * info);
/** Opaque user data. */
void *udata;
#ifdef FIO_MAP_VALUE
/** The object's value at the current index. */
FIO_MAP_VALUE value;
#endif
/** The object's key the current index. */
FIO_MAP_KEY key;
}
```

Iteration information structure passed to the callback.

_Symbol type:_ `type`

### Functions

#### `fiobj_hash_destroy`

```c
void fiobj_hash_destroy(FIO_MAP_PTR map)
```

Destroys the object, re-initializing its container.

_Symbol type:_ `function`

#### `fiobj_hash_capa`

```c
inline uint32_t fiobj_hash_capa(FIO_MAP_PTR map)
```

Theoretical map capacity.

_Symbol type:_ `function`

#### `fiobj_hash_count`

```c
inline uint32_t fiobj_hash_count(FIO_MAP_PTR map)
```

The number of objects in the map capacity.

_Symbol type:_ `function`

#### `fiobj_hash_reserve`

```c
void fiobj_hash_reserve(FIO_MAP_PTR map, size_t capa)
```

Reserves at minimum the capacity requested.

_Symbol type:_ `function`

#### `fiobj_hash_node2key`

```c
inline FIO_MAP_KEY fiobj_hash_node2key(FIO_NAME(FIO_MAP_NAME, node_s) * node)
```

Returns the key value associated with the node's pointer (see set_ptr).

_Symbol type:_ `function`

#### `fiobj_hash_node2hash`

```c
inline uint64_t fiobj_hash_node2hash(FIO_NAME(FIO_MAP_NAME, node_s) * node)
```

Returns the hash value associated with the node's pointer (see set_ptr).

_Symbol type:_ `function`

#### `fiobj_hash_node2val`

```c
inline FIO_MAP_VALUE fiobj_hash_node2val(FIO_NAME(FIO_MAP_NAME, node_s) * node)
```

Returns the value associated with the node's pointer (see set_ptr).

_Symbol type:_ `function`

#### `fiobj_hash_node2key_ptr`

```c
inline FIO_MAP_KEY_INTERNAL *fiobj_hash_node2key_ptr( FIO_NAME(FIO_MAP_NAME, node_s) * node)
```

Returns the key value associated with the node's pointer (see set_ptr).

_Symbol type:_ `function`

#### `fiobj_hash_node2val_ptr`

```c
inline FIO_MAP_VALUE_INTERNAL *fiobj_hash_node2val_ptr( FIO_NAME(FIO_MAP_NAME, node_s) * node)
```

Returns the value associated with the node's pointer (see set_ptr).

_Symbol type:_ `function`

#### `fiobj_hash_remove`

```c
int fiobj_hash_remove(FIO_MAP_PTR map, #if !defined(FIO_MAP_HASH_FN) uint64_t hash, #endif FIO_MAP_KEY key, #if defined(FIO_MAP_VALUE) FIO_MAP_VALUE_INTERNAL *old #else FIO_MAP_KEY_INTERNAL *old #endif )
```

Removes an object in the map, returning a pointer to the map data.

_Symbol type:_ `function`

#### `fiobj_hash_evict`

```c
void fiobj_hash_evict(FIO_MAP_PTR map, size_t number_of_elements)
```

Evicts elements in order least recently used (LRU), FIFO or undefined.

_Symbol type:_ `function`

#### `fiobj_hash_clear`

```c
void fiobj_hash_clear(FIO_MAP_PTR map)
```

Removes all objects from the map, without releasing the map's resources.

_Symbol type:_ `function`

#### `fiobj_hash_compact`

```c
void fiobj_hash_compact(FIO_MAP_PTR map)
```

Attempts to minimize memory use.

_Symbol type:_ `function`

#### `fiobj_hash_get`

```c
inline FIO_MAP_GET_T fiobj_hash_get(FIO_MAP_PTR map, #if !defined(FIO_MAP_HASH_FN) uint64_t hash, #endif FIO_MAP_KEY key)
```

Gets a value from the map, if exists.

_Symbol type:_ `function`

#### `fiobj_hash_set`

```c
inline FIO_MAP_GET_T fiobj_hash_set(FIO_MAP_PTR map, #if !defined(FIO_MAP_HASH_FN) uint64_t hash, #endif #ifdef FIO_MAP_VALUE FIO_MAP_KEY key, FIO_MAP_VALUE obj, FIO_MAP_VALUE_INTERNAL *old #else FIO_MAP_KEY key #endif )
```

Sets a value in the map, hash maps will overwrite existing data if any.

_Symbol type:_ `function`

#### `fiobj_hash_set_if_missing`

```c
inline FIO_MAP_GET_T fiobj_hash_set_if_missing(FIO_MAP_PTR map, #if !defined(FIO_MAP_HASH_FN) uint64_t hash, #endif FIO_MAP_KEY key #ifdef FIO_MAP_VALUE , FIO_MAP_VALUE obj #endif )
```

Sets a value in the map if not set previously.

_Symbol type:_ `function`

#### `fiobj_hash_set_ptr`

```c
fiobj_hash_set_ptr * FIO_NAME(FIO_MAP_NAME, set_ptr)(FIO_MAP_PTR map, #if !defined(FIO_MAP_HASH_FN) uint64_t hash, #endif #ifdef FIO_MAP_VALUE FIO_MAP_KEY key, FIO_MAP_VALUE val, FIO_MAP_VALUE_INTERNAL *old, int overwrite #else FIO_MAP_KEY key #endif )
```

The core set function.

This function returns `NULL` on error (errors are logged).

If the map is a hash map, overwriting the value (while keeping the key) is
possible. In this case the `old` pointer is optional, and if set than the old
data will be copied to over during an overwrite.

NOTE: the function returns a pointer to the map's internal storage.

_Symbol type:_ `function`

#### `fiobj_hash_get_ptr`

```c
fiobj_hash_get_ptr * FIO_NAME(FIO_MAP_NAME, get_ptr)(FIO_MAP_PTR map, #if !defined(FIO_MAP_HASH_FN) uint64_t hash, #endif FIO_MAP_KEY key)
```

The core get function. This function returns NULL if item is missing.

NOTE: the function returns a pointer to the map's internal storage.

_Symbol type:_ `function`

#### `fiobj_hash_each`

```c
uint32_t fiobj_hash_each(FIO_MAP_PTR map, int (*task)(FIO_NAME(FIO_MAP_NAME, each_s) *), void *udata, ssize_t start_at)
```

Iteration using a callback for each element in the map.

The callback task function must accept an each_s pointer, see above.

If the callback returns -1, the loop is broken. Any other value is ignored.

Returns the relative "stop" position, i.e., the number of items processed +
the starting point.

_Symbol type:_ `function`

#### `fiobj_hash_get_next`

```c
fiobj_hash_get_next FIO_NAME(FIO_MAP_NAME, get_next)(FIO_MAP_PTR map, FIO_NAME(FIO_MAP_NAME, iterator_s) * current_pos)
```

Returns the next iterator object after `current_pos` or the first if `NULL`.

Note that adding objects to the map or rehashing between iterations could
incur performance penalties when re-setting and re-seeking the previous
iterator position.

Adding objects to, or rehashing, an unordered maps could invalidate the
iterator object completely as the ordering may have changed and so the "next"
object might be any object in the map.

_Symbol type:_ `function`

#### `fiobj_hash_get_prev`

```c
fiobj_hash_get_prev FIO_NAME(FIO_MAP_NAME, get_prev)(FIO_MAP_PTR map, FIO_NAME(FIO_MAP_NAME, iterator_s) * current_pos)
```

Returns the next iterator object after `current_pos` or the last if `NULL`.

See notes in `get_next`.

_Symbol type:_ `function`

#### `fiobj_hash_iterator_is_valid`

```c
inline int fiobj_hash_iterator_is_valid(FIO_NAME(FIO_MAP_NAME, iterator_s) * iterator)
```

Returns 1 if the iterator is out of bounds, otherwise returns 0.

_Symbol type:_ `function`

#### `fiobj_hash_iterator2node`

```c
inline fiobj_hash_iterator2node * FIO_NAME(FIO_MAP_NAME, iterator2node)(FIO_MAP_PTR map, FIO_NAME(FIO_MAP_NAME, iterator_s) * iterator)
```

Returns a pointer to the node object in the internal map.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-210-map2-h"></a> `./fio-stl/210 map2.h`

3 public symbols.

### Macros

#### `FIO_MAP2_INIT`

```c
#define FIO_MAP2_INIT   \
  { 0 }
```



_Symbol type:_ `macro`

#### `FIO_MAP2_EACH`

```c
#define FIO_MAP2_EACH(map_name, map_ptr, i)   \
  for (FIO_NAME(map_name, iterator_s)   \
           i = FIO_NAME(map_name, get_next)(map_ptr, NULL);   \
       FIO_NAME(map_name, iterator_is_valid)(&i);   \
       i = FIO_NAME(map_name, get_next)(map_ptr, &i))
```

Iterates through the map using an iterator object.

_Symbol type:_ `macro`

#### `FIO_MAP2_EACH_REVERSED`

```c
#define FIO_MAP2_EACH_REVERSED(map_name, map_ptr, i)   \
  for (FIO_NAME(map_name, iterator_s)   \
           i = FIO_NAME(map_name, get_prev)(map_ptr, NULL);   \
       FIO_NAME(map_name, iterator_is_valid)(&i);   \
       i = FIO_NAME(map_name, get_prev)(map_ptr, &i))
```

Iterates through the map using an iterator object.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-249-reference-counter-h"></a> `./fio-stl/249 reference counter.h`

17 public symbols.

### Functions

#### `fiobj_array_new`

```c
FIO_REF_TYPE_PTR fiobj_array_new(void)
```



_Symbol type:_ `function`

#### `fiobj_hash_new`

```c
FIO_REF_TYPE_PTR fiobj_hash_new(void)
```



_Symbol type:_ `function`

#### `fiobj_object_new`

```c
FIO_REF_TYPE_PTR fiobj_object_new(void)
```



_Symbol type:_ `function`

#### `fiobj_str_new`

```c
FIO_REF_TYPE_PTR fiobj_str_new(void)
```



_Symbol type:_ `function`

#### `fiobj_array_dup`

```c
inline FIO_REF_TYPE_PTR fiobj_array_dup(const FIO_REF_TYPE_PTR wrapped)
```

Increases the reference count.

_Symbol type:_ `function`

#### `fiobj_hash_dup`

```c
inline FIO_REF_TYPE_PTR fiobj_hash_dup(const FIO_REF_TYPE_PTR wrapped)
```

Increases the reference count.

_Symbol type:_ `function`

#### `fiobj_object_dup`

```c
inline FIO_REF_TYPE_PTR fiobj_object_dup(const FIO_REF_TYPE_PTR wrapped)
```

Increases the reference count.

_Symbol type:_ `function`

#### `fiobj_str_dup`

```c
inline FIO_REF_TYPE_PTR fiobj_str_dup(const FIO_REF_TYPE_PTR wrapped)
```

Increases the reference count.

_Symbol type:_ `function`

#### `fiobj_array_free`

```c
void fiobj_array_free(FIO_REF_TYPE_PTR wrapped)
```

Frees a reference counted object (or decreases the reference count).

_Symbol type:_ `function`

#### `fiobj_hash_free`

```c
void fiobj_hash_free(FIO_REF_TYPE_PTR wrapped)
```

Frees a reference counted object (or decreases the reference count).

_Symbol type:_ `function`

#### `fiobj_object_free`

```c
void fiobj_object_free(FIO_REF_TYPE_PTR wrapped)
```

Frees a reference counted object (or decreases the reference count).

_Symbol type:_ `function`

#### `fiobj_str_free`

```c
void fiobj_str_free(FIO_REF_TYPE_PTR wrapped)
```

Frees a reference counted object (or decreases the reference count).

_Symbol type:_ `function`

#### `fiobj_object_metadata`

```c
FIO_REF_METADATA *fiobj_object_metadata(FIO_REF_TYPE_PTR wrapped)
```

Returns a pointer to the object's metadata, if defined.

_Symbol type:_ `function`

#### `fiobj_array_references`

```c
inline size_t fiobj_array_references(FIO_REF_TYPE_PTR wrapped_)
```

Debugging helper, do not use for data, as returned value is unstable.

_Symbol type:_ `function`

#### `fiobj_hash_references`

```c
inline size_t fiobj_hash_references(FIO_REF_TYPE_PTR wrapped_)
```

Debugging helper, do not use for data, as returned value is unstable.

_Symbol type:_ `function`

#### `fiobj_object_references`

```c
inline size_t fiobj_object_references(FIO_REF_TYPE_PTR wrapped_)
```

Debugging helper, do not use for data, as returned value is unstable.

_Symbol type:_ `function`

#### `fiobj_str_references`

```c
inline size_t fiobj_str_references(FIO_REF_TYPE_PTR wrapped_)
```

Debugging helper, do not use for data, as returned value is unstable.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-250-fiobj-h"></a> `./fio-stl/250 fiobj.h`

66 public symbols.

### Macros

#### `FIOBJ_MAX_NESTING`

```c
#define FIOBJ_MAX_NESTING 512
```

Sets the limit on nesting level transversal by recursive functions.

This effects JSON output / input and the `fiobj_each2` function since they
are recursive.

HOWEVER: this value will NOT effect the recursive `fiobj_free` which could
(potentially) expload the stack if given melformed input such as cyclic data
structures.

Values should be less than 32K.

_Symbol type:_ `macro`

#### `FIOBJ_JSON_APPEND`

```c
#define FIOBJ_JSON_APPEND 1
```



_Symbol type:_ `macro`

#### `FIOBJ_MARK_MEMORY_ALLOC`

```c
#define FIOBJ_MARK_MEMORY_ALLOC()   \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_ALLOC_COUNTER, 1)
```



_Symbol type:_ `macro`

#### `FIOBJ_MARK_MEMORY_FREE`

```c
#define FIOBJ_MARK_MEMORY_FREE()   \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_FREE_COUNTER, 1)
```



_Symbol type:_ `macro`

#### `FIOBJ_MARK_MEMORY_PRINT`

```c
#define FIOBJ_MARK_MEMORY_PRINT()   \
  FIO___LOG_PRINT_LEVEL(   \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)   \
           ? 4 /* FIO_LOG_LEVEL_INFO */   \
           : 3 /* FIO_LOG_LEVEL_WARNING */),   \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)   \
           ? "INFO: total remaining FIOBJ allocations: %zu (%zu - %zu)"   \
           : "WARNING: LEAKED! FIOBJ allocations: %zu (%zu - %zu)"),   \
      FIOBJ_MARK_MEMORY_ALLOC_COUNTER - FIOBJ_MARK_MEMORY_FREE_COUNTER,   \
      FIOBJ_MARK_MEMORY_ALLOC_COUNTER,   \
      FIOBJ_MARK_MEMORY_FREE_COUNTER)
```



_Symbol type:_ `macro`

#### `FIOBJ_MARK_MEMORY_ENABLED`

```c
#define FIOBJ_MARK_MEMORY_ENABLED 1
```

If true, FIOBJ memory allocation counting is enabled.

_Symbol type:_ `macro`

#### `FIOBJ_MARK_MEMORY_ALLOC_COUNTER`

```c
#define FIOBJ_MARK_MEMORY_ALLOC_COUNTER 0 /* when testing unmarked FIOBJ */
```



_Symbol type:_ `macro`

#### `FIOBJ_MARK_MEMORY_FREE_COUNTER`

```c
#define FIOBJ_MARK_MEMORY_FREE_COUNTER  0 /* when testing unmarked FIOBJ */
```



_Symbol type:_ `macro`

#### `FIOBJ_T_NULL`

```c
#define FIOBJ_T_NULL  2  /* 0b010 a lonely second bit signifies a primitive */
```



_Symbol type:_ `macro`

#### `FIOBJ_T_TRUE`

```c
#define FIOBJ_T_TRUE  18 /* 0b010 010 - primitive value */
```



_Symbol type:_ `macro`

#### `FIOBJ_T_FALSE`

```c
#define FIOBJ_T_FALSE 34 /* 0b100 010 - primitive value */
```



_Symbol type:_ `macro`

#### `FIOBJ_TYPE`

```c
#define FIOBJ_TYPE(o) fiobj_type(o)
```

Use the macros to avoid future API changes.

_Symbol type:_ `macro`

#### `FIOBJ_TYPE_IS`

```c
#define FIOBJ_TYPE_IS(o, type) (fiobj_type(o) == type)
```

Use the macros to avoid future API changes.

_Symbol type:_ `macro`

#### `FIOBJ_T_INVALID`

```c
#define FIOBJ_T_INVALID 0
```

Identifies an invalid type identifier (returned from FIOBJ_TYPE(o)

_Symbol type:_ `macro`

#### `FIOBJ_INVALID`

```c
#define FIOBJ_INVALID 0
```

Identifies an invalid object

_Symbol type:_ `macro`

#### `FIOBJ_IS_INVALID`

```c
#define FIOBJ_IS_INVALID(o)     (((uintptr_t)(o)&7UL) == 0)
```

Tests if the object is (probably) a valid FIOBJ

_Symbol type:_ `macro`

#### `FIOBJ_IS_NULL`

```c
#define FIOBJ_IS_NULL(o)        (FIOBJ_IS_INVALID(o) || ((o) == FIOBJ_T_NULL))
```



_Symbol type:_ `macro`

#### `FIOBJ_TYPE_CLASS`

```c
#define FIOBJ_TYPE_CLASS(o)     ((fiobj_class_en)(((uintptr_t)(o)) & 7UL))
```



_Symbol type:_ `macro`

#### `FIOBJ_PTR_TAG`

```c
#define FIOBJ_PTR_TAG(o, klass) ((uintptr_t)(((uintptr_t)(o)) | (klass)))
```



_Symbol type:_ `macro`

#### `FIOBJ_PTR_UNTAG`

```c
#define FIOBJ_PTR_UNTAG(o)      ((uintptr_t)(((uintptr_t)(o)) & (~7ULL)))
```



_Symbol type:_ `macro`

#### `FIOBJ_STR_TEMP_VAR`

```c
#define FIOBJ_STR_TEMP_VAR(str_name)   \
  struct {   \
    uint64_t i1;   \
    uint64_t i2;   \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;   \
  } FIO_NAME(str_name, __auto_mem_tmp) = {0x7f7f7f7f7f7f7f7fULL,   \
                                          0x7f7f7f7f7f7f7f7fULL,   \
                                          FIO_STR_INIT};   \
  FIOBJ str_name =   \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |   \
              FIOBJ_T_STRING);
```

Creates a temporary FIOBJ String object on the stack.

String data might be allocated dynamically.

_Symbol type:_ `macro`

#### `FIOBJ_STR_TEMP_VAR_STATIC`

```c
#define FIOBJ_STR_TEMP_VAR_STATIC(str_name, buf_, len_)   \
  struct {   \
    uint64_t i1;   \
    uint64_t i2;   \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;   \
  } FIO_NAME(str_name,   \
             __auto_mem_tmp) = {0x7f7f7f7f7f7f7f7fULL,   \
                                0x7f7f7f7f7f7f7f7fULL,   \
                                FIO_STR_INIT_STATIC2((buf_), (len_))};   \
  FIOBJ str_name =   \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |   \
              FIOBJ_T_STRING);
```

Creates a temporary FIOBJ String object on the stack, initialized with a
static string.

String data might be allocated dynamically.

_Symbol type:_ `macro`

#### `FIOBJ_STR_TEMP_VAR_EXISTING`

```c
#define FIOBJ_STR_TEMP_VAR_EXISTING(str_name, buf_, len_, capa_)   \
  struct {   \
    uint64_t i1;   \
    uint64_t i2;   \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;   \
  } FIO_NAME(str_name, __auto_mem_tmp) = {   \
      0x7f7f7f7f7f7f7f7fULL,   \
      0x7f7f7f7f7f7f7f7fULL,   \
      FIO_STR_INIT_EXISTING((buf_), (len_), (capa_))};   \
  FIOBJ str_name =   \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |   \
              FIOBJ_T_STRING);
```

Creates a temporary FIOBJ String object on the stack, initialized with a
static string.

String data might be allocated dynamically.

_Symbol type:_ `macro`

#### `FIOBJ_STR_TEMP_DESTROY`

```c
#define FIOBJ_STR_TEMP_DESTROY(str_name)   \
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), destroy)(str_name);
```

Resets a temporary FIOBJ String, freeing and any resources allocated.

_Symbol type:_ `macro`

### Types

#### `fiobj_class_en`

```c
typedef enum {
FIOBJ_T_NUMBER = 0x01, /* 0b001 3 bits taken for small numbers */
FIOBJ_T_PRIMITIVE = 2, /* 0b010 a lonely second bit signifies a primitive */
FIOBJ_T_STRING = 3, /* 0b011 */
FIOBJ_T_ARRAY = 4, /* 0b100 */
FIOBJ_T_HASH = 5, /* 0b101 */
FIOBJ_T_FLOAT = 6, /* 0b110 */
FIOBJ_T_OTHER = 7, /* 0b111 dynamic type - test content */
} fiobj_class_en
```

FIOBJ type enum for common / primitive types.

_Symbol type:_ `type`

#### `fiobj_each_s`

```c
struct fiobj_each_s {
/** The being iterated. Once set, cannot be safely changed. */
FIOBJ const parent;
/** The index to start at / the current object's index */
uint64_t index;
/** The callback / task called for each index, may be updated mid-cycle. */
int (*task)(struct fiobj_each_s *info);
/** The argument passed along to the task. */
void *udata;
/** The value of the current object in the Array or Hash Map */
FIOBJ value;
/* The key, if a Hash Map */
FIOBJ key;
}
```

Iteration information structure passed to the callback.

_Symbol type:_ `type`

### Functions

#### `fiobj_type`

```c
inline size_t fiobj_type(FIOBJ o)
```

Returns an objects type. This isn't limited to known types.

_Symbol type:_ `function`

#### `fiobj_dup`

```c
inline FIOBJ fiobj_dup(FIOBJ o)
```

Increases an object's reference count (or copies) and returns it.

_Symbol type:_ `function`

#### `fiobj_free`

```c
inline void fiobj_free(FIOBJ o)
```

Decreases an object's reference count or frees it.

_Symbol type:_ `function`

#### `fiobj_is_eq`

```c
inline unsigned char fiobj_is_eq(FIOBJ a, FIOBJ b)
```

Compares two objects.

_Symbol type:_ `function`

#### `fiobj2cstr`

```c
inline fio_str_info_s fiobj2cstr(FIOBJ o)
```

Returns a temporary String representation for any FIOBJ object.

_Symbol type:_ `function`

#### `fiobj2i`

```c
inline intptr_t fiobj2i(FIOBJ o)
```

Returns an integer representation for any FIOBJ object.

_Symbol type:_ `function`

#### `fiobj2f`

```c
inline double fiobj2f(FIOBJ o)
```

Returns a float (double) representation for any FIOBJ object.

_Symbol type:_ `function`

#### `fiobj2hash`

```c
inline uint64_t fiobj2hash(FIOBJ object_key)
```

Calculates an object's hash value for a specific hash map object.

_Symbol type:_ `function`

#### `fiobj_each1`

```c
FIO_SFUNC uint32_t fiobj_each1(FIOBJ o, int (*task)(fiobj_each_s *info), void *udata, int32_t start_at)
```

Performs a task for each element held by the FIOBJ object.

If `task` returns -1, the `each` loop will break (stop).

Returns the "stop" position - the number of elements processed + `start_at`.

_Symbol type:_ `function`

#### `fiobj_each2`

```c
uint32_t fiobj_each2(FIOBJ o, int (*task)(fiobj_each_s *info), void *udata)
```

Performs a task for the object itself and each element held by the FIOBJ
object or any of it's elements (a deep task).

The order of performance is by order of appearance, as if all nesting levels
were flattened.

If `task` returns -1, the `each` loop will break (stop).

Returns the number of elements processed.

_Symbol type:_ `function`

#### `fiobj_true`

```c
inline FIOBJ fiobj_true(void)
```

Returns the `true` primitive.

_Symbol type:_ `function`

#### `fiobj_false`

```c
inline FIOBJ fiobj_false(void)
```

Returns the `false` primitive.

_Symbol type:_ `function`

#### `fiobj_null`

```c
inline FIOBJ fiobj_null(void)
```

Returns the `nil` / `null` primitive.

_Symbol type:_ `function`

#### `fiobj_num_new`

```c
inline FIOBJ FIO_NAME(fiobj_num_new, new)(intptr_t i)
```

Creates a new Number object.

_Symbol type:_ `function`

#### `fiobj_num2i`

```c
inline intptr_t FIO_NAME2(fiobj_num2i, i)(FIOBJ i)
```

Reads the number from a FIOBJ Number.

_Symbol type:_ `function`

#### `fiobj_num2f`

```c
inline double FIO_NAME2(fiobj_num2f, f)(FIOBJ i)
```

Reads the number from a FIOBJ Number, fitting it in a double.

_Symbol type:_ `function`

#### `fiobj_num2cstr`

```c
fio_str_info_s FIO_NAME2(fiobj_num2cstr, cstr)(FIOBJ i)
```

Returns a String representation of the number (in base 10).

_Symbol type:_ `function`

#### `fiobj_num_free`

```c
inline void FIO_NAME(fiobj_num_free, free)(FIOBJ i)
```

Frees a FIOBJ number.

_Symbol type:_ `function`

#### `fiobj_float_new`

```c
inline FIOBJ FIO_NAME(fiobj_float_new, new)(double i)
```

Creates a new Float (double) object.

_Symbol type:_ `function`

#### `fiobj_float2i`

```c
inline intptr_t FIO_NAME2(fiobj_float2i, i)(FIOBJ i)
```

Reads the number from a FIOBJ Float rounding it to an integer.

_Symbol type:_ `function`

#### `fiobj_float2f`

```c
inline double FIO_NAME2(fiobj_float2f, f)(FIOBJ i)
```

Reads the value from a FIOBJ Float, as a double.

_Symbol type:_ `function`

#### `fiobj_float2cstr`

```c
fio_str_info_s FIO_NAME2(fiobj_float2cstr, cstr)(FIOBJ i)
```

Returns a String representation of the float.

_Symbol type:_ `function`

#### `fiobj_float_free`

```c
inline void FIO_NAME(fiobj_float_free, free)(FIOBJ i)
```

Frees a FIOBJ Float.

_Symbol type:_ `function`

#### `fiobj_str_new_cstr`

```c
inline FIOBJ FIO_NAME(fiobj_str_new_cstr, new_cstr)(const char *ptr, size_t len)
```



_Symbol type:_ `function`

#### `fiobj_str_new_buf`

```c
inline FIOBJ FIO_NAME(fiobj_str_new_buf, new_buf)(size_t capa)
```



_Symbol type:_ `function`

#### `fiobj_str_new_copy`

```c
inline FIOBJ FIO_NAME(fiobj_str_new_copy, new_copy)(FIOBJ original)
```



_Symbol type:_ `function`

#### `fiobj_str2cstr`

```c
inline fio_str_info_s FIO_NAME2(fiobj_str2cstr, cstr)(FIOBJ s)
```

Returns information about the string. Same as fiobj_str_info().

_Symbol type:_ `function`

#### `fiobj_hash_set2`

```c
inline FIOBJ FIO_NAME(fiobj_hash_set2, set2)(FIOBJ hash, const char *key, size_t len, FIOBJ value)
```

Sets a value in a hash map, allocating the key String and automatically
calculating the hash value.

_Symbol type:_ `function`

#### `fiobj_hash_get2`

```c
inline FIOBJ FIO_NAME(fiobj_hash_get2, get2)(FIOBJ hash, const char *buf, size_t len)
```

Finds a value in the hash map, using a temporary String and automatically
calculating the hash value.

_Symbol type:_ `function`

#### `fiobj_hash_remove2`

```c
inline int FIO_NAME(fiobj_hash_remove2, remove2)(FIOBJ hash, const char *buf, size_t len, FIOBJ *old)
```

Removes a value in a hash map, using a temporary String and automatically
calculating the hash value.

_Symbol type:_ `function`

#### `fiobj2json`

```c
inline FIOBJ fiobj2json(FIOBJ dest, FIOBJ o, uint8_t beautify)
```

Returns a JSON valid FIOBJ String, representing the object.

If `dest` is an existing String, the formatted JSON data will be appended to
the existing string.

_Symbol type:_ `function`

#### `fiobj_hash_update_json`

```c
size_t FIO_NAME(fiobj_hash_update_json, update_json)(FIOBJ hash, fio_str_info_s str)
```

Updates a Hash using JSON data.

Parsing errors and non-dictionary object JSON data are silently ignored,
attempting to update the Hash as much as possible before any errors
encountered.

Conflicting Hash data is overwritten (preferring the new over the old).

Returns the number of bytes consumed. On Error, 0 is returned and no data is
consumed.

_Symbol type:_ `function`

#### `fiobj_hash_update_json2`

```c
inline size_t FIO_NAME(fiobj_hash_update_json2, update_json2)(FIOBJ hash, char *ptr, size_t len)
```

Helper function, calls `fiobj_hash_update_json` with string information

_Symbol type:_ `function`

#### `fiobj_json_parse`

```c
FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed)
```

Parses a C string for JSON data. If `consumed` is not NULL, the `size_t`
variable will contain the number of bytes consumed before the parser stopped
(due to either error or end of a valid JSON data segment).

Returns a FIOBJ object matching the JSON valid C string `str`.

If the parsing failed (no complete valid JSON data) `FIOBJ_INVALID` is
returned.

_Symbol type:_ `function`

#### `fiobj_json_parse2`

```c
#define fiobj_json_parse2(data_, len_, consumed)   \
  fiobj_json_parse(FIO_STR_INFO2(data_, len_), consumed)
```

Helper macro, calls `fiobj_json_parse` with string information

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fiobj_json_find`

```c
FIOBJ fiobj_json_find(FIOBJ object, fio_str_info_s notation)
```

Uses JavaScript style notation to find data in an object structure.

For example, "[0].name" will return the "name" property of the first object
in an array object.

Returns a temporary reference to the object or FIOBJ_INVALID on an error.

Use `fiobj_dup` to collect an actual reference to the returned object.

_Symbol type:_ `function`

#### `fiobj_json_find2`

```c
#define fiobj_json_find2(object, str, length)   \
  fiobj_json_find(object, FIO_STR_INFO2(str, length))
```

Uses JavaScript style notation to find data in an object structure.

For example, "[0].name" will return the "name" property of the first object
in an array object.

Returns a temporary reference to the object or FIOBJ_INVALID on an error.

Use `fiobj_dup` to collect an actual reference to the returned object.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fiobj_mustache_build`

```c
inline FIOBJ fiobj_mustache_build(fio_mustache_s *m, FIOBJ ctx)
```

Builds a Mustache template using a FIOBJ context (usually a Hash).

Returns a FIOBJ String with the rendered template. May return `FIOBJ_INVALID`
if nothing was written.

_Symbol type:_ `function`

#### `fiobj_mustache_build2`

```c
inline FIOBJ fiobj_mustache_build2(fio_mustache_s *m, FIOBJ dest, FIOBJ ctx)
```

Builds a Mustache template using a FIOBJ context (usually a Hash).

Writes output to `dest` string (may be `FIOBJ_INVALID` / `NULL`).

Returns `dest` (or a new String). May return `FIOBJ_INVALID` if nothing was
written and `dest` was empty.

_Symbol type:_ `function`

#### `fiobj_hash_update`

```c
inline void FIO_NAME(fiobj_hash_update, update)(FIOBJ dest, FIOBJ src)
```

Updates a hash using information from another Hash.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-401-io-api-h"></a> `./fio-stl/401 io api.h`

102 public symbols.

### Macros

#### `FIO_IO_BUFFER_PER_WRITE`

```c
#define FIO_IO_BUFFER_PER_WRITE 65536U
```

Control the size of the on-stack buffer used for `write` events.

_Symbol type:_ `macro`

#### `FIO_IO_THROTTLE_LIMIT`

```c
#define FIO_IO_THROTTLE_LIMIT 2097152U
```

IO will be throttled (no `on_data` events) if outgoing buffer is large.

_Symbol type:_ `macro`

#### `FIO_IO_TIMEOUT_MAX`

```c
#define FIO_IO_TIMEOUT_MAX 300000
```

Controls the maximum and default timeout in milliseconds (5 minutes).

_Symbol type:_ `macro`

#### `FIO_IO_SHUTDOWN_TIMEOUT`

```c
#define FIO_IO_SHUTDOWN_TIMEOUT 15000
```



_Symbol type:_ `macro`

#### `FIO_IO_COUNT_STORAGE`

```c
#define FIO_IO_COUNT_STORAGE 1
```



_Symbol type:_ `macro`

#### `FIO_IO_ASYN_INIT`

```c
#define FIO_IO_ASYN_INIT ((fio_io_async_s){0})
```

Initializes an IO Async Queue (multi-threaded task queue).

The queue automatically spawns threads and shuts down as the IO reactor
starts or stops.

It is recommended that the `fio_io_async_s` be allocated as a static
variable, as its memory must remain valid throughout the lifetime of the
IO reactor's app.

_Symbol type:_ `macro`

### Types

#### `fio_io_protocol_s`

```c
struct fio_io_protocol_s
```

The main protocol object type. See `struct fio_io_protocol_s`.

_Symbol type:_ `type`

#### `fio_io_functions_s`

```c
struct fio_io_functions_s
```

The IO functions used by the protocol object.

_Symbol type:_ `type`

#### `fio_io_s`

```c
struct fio_io_s
```

The main IO object type. Should be treated as an opaque pointer.

_Symbol type:_ `type`

#### `fio_io_tls_s`

```c
struct fio_io_tls_s
```

An opaque type used for the SSL/TLS helper functions.

_Symbol type:_ `type`

#### `fio_pubsub_msg_s`

```c
struct fio_pubsub_msg_s
```

Message structure, as received by the `on_message` subscription callback.

_Symbol type:_ `type`

#### `fio_io_async_s`

```c
struct fio_io_async_s
```

The IO Async Queue type.

_Symbol type:_ `type`

#### `fio_io_listen_args_s`

```c
struct fio_io_listen_args_s {
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
* fio_io_listen(.url = "0.0.0.0:3000/?tls", ...);
* fio_io_listen(.url = "0.0.0.0:3000/?tls=./", ...);
* // same as:
* fio_io_listen(.url = "0.0.0.0:3000/"
* "?key=./key.pem"
* "&cert=./cert.pem", ...);
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
}
```

Arguments for the fio_io_listen function

_Symbol type:_ `type`

#### `fio_io_listener_s`

```c
struct fio_io_listener_s
```



_Symbol type:_ `type`

#### `fio_io_connect_args_s`

```c
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
} fio_io_connect_args_s
```

Named arguments for fio_io_connect

_Symbol type:_ `type`

#### `fio_io_write_args_s`

```c
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
* If `copy == 0`, and `dealloc` is set, `write` will take ownership.
*
* If NULL, the buffer will NOT be de-allocated.
*/
void (*dealloc)(void *);
/** If non-zero, makes a copy of the buffer or keeps a file open. */
uint8_t copy;
} fio_io_write_args_s
```



_Symbol type:_ `type`

#### `fio_io_env_set_args_s`

```c
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
} fio_io_env_set_args_s
```

Named arguments for the `fio_io_env_set` function.

_Symbol type:_ `type`

#### `fio_io_env_get_args_s`

```c
typedef struct {
/** A numerical type filter. Should be the same as used with
* `fio_io_env_set` */
intptr_t type;
/** The name of the object. Should be the same as used with `fio_io_env_set`
*/
fio_buf_info_s name;
} fio_io_env_get_args_s
```

Named arguments for the `fio_io_env_unset` function.

_Symbol type:_ `type`

#### `fio_io_tls_each_s`

```c
struct fio_io_tls_each_s {
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
}
```

Arguments (and info) for `fio_io_tls_each`.

_Symbol type:_ `type`

### Functions

#### `fio_io_stop`

```c
void fio_io_stop(void)
```

Stopping the IO reactor.

_Symbol type:_ `function`

#### `fio_io_add_workers`

```c
void fio_io_add_workers(int workers)
```

Adds `workers` amount of workers to the root IO reactor process.

_Symbol type:_ `function`

#### `fio_io_start`

```c
void fio_io_start(int workers)
```

Starts the IO reactor, using optional `workers` processes. Will BLOCK!

_Symbol type:_ `function`

#### `fio_io_restart`

```c
void fio_io_restart(int workers)
```

Retiers all existing workers and restarts with the number of workers.

_Symbol type:_ `function`

#### `fio_io_restart_on_signal`

```c
void fio_io_restart_on_signal(int signal)
```

Sets a signal to listen to for a hot restart (see `fio_io_restart`).

_Symbol type:_ `function`

#### `fio_io_shutdown_timeout`

```c
size_t fio_io_shutdown_timeout(void)
```

Returns the shutdown timeout for the reactor.

_Symbol type:_ `function`

#### `fio_io_shutdown_timeout_set`

```c
size_t fio_io_shutdown_timeout_set(size_t milliseconds)
```

Sets the shutdown timeout for the reactor, returning the new value.

_Symbol type:_ `function`

#### `fio_io_is_running`

```c
int fio_io_is_running(void)
```

Returns true if IO reactor running and 0 if stopped or shutting down.

_Symbol type:_ `function`

#### `fio_io_is_master`

```c
int fio_io_is_master(void)
```

Returns true if the current process is the IO reactor's master process.

_Symbol type:_ `function`

#### `fio_io_is_worker`

```c
int fio_io_is_worker(void)
```

Returns true if the current process is an IO reactor's worker process.

_Symbol type:_ `function`

#### `fio_io_workers`

```c
uint16_t fio_io_workers(int workers_requested)
```

Returns the number or workers the IO reactor will actually run.

_Symbol type:_ `function`

#### `fio_io_pid`

```c
int fio_io_pid(void)
```

Returns current process id.

_Symbol type:_ `function`

#### `fio_io_root_pid`

```c
int fio_io_root_pid(void)
```

Returns the root / master process id.

_Symbol type:_ `function`

#### `fio_io_last_tick`

```c
int64_t fio_io_last_tick(void)
```

Returns the last millisecond when the IO reactor polled for events.

_Symbol type:_ `function`

#### `fio_io_last_tick_time`

```c
int64_t fio_io_last_tick_time(void)
```

Returns a cached real-time (wall-clock) timestamp in milliseconds,
updated each IO tick alongside `fio_io_last_tick`. Use this to avoid
repeated `clock_gettime` syscalls when an approximate wall-clock ms is
sufficient (e.g., HTTP Date headers, access-log timestamps).

_Symbol type:_ `function`

#### `fio_io_listen`

```c
fio_io_listener_s *fio_io_listen(fio_io_listen_args_s args)
```

Sets up a network service on a listening socket.

Returns a self-destructible listener handle on success or NULL on error.

NOTE: this schedules a task and should NOT be called within a PRE_START or
ON_START state callback.

_Symbol type:_ `function`

#### `fio_io_listen`

```c
#define fio_io_listen(...) fio_io_listen((fio_io_listen_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_listen_stop`

```c
void fio_io_listen_stop(fio_io_listener_s *l)
```

Notifies a listener to stop listening.

_Symbol type:_ `function`

#### `fio_io_listener_protocol`

```c
fio_io_protocol_s *fio_io_listener_protocol(fio_io_listener_s *l)
```

Returns the listener's associated protocol.

_Symbol type:_ `function`

#### `fio_io_listener_udata`

```c
void *fio_io_listener_udata(fio_io_listener_s *l)
```

Returns the listener's associated `udata`.

_Symbol type:_ `function`

#### `fio_io_listener_udata_set`

```c
void *fio_io_listener_udata_set(fio_io_listener_s *l, void *new_udata)
```

Sets the listener's associated `udata`, returning the old value.

_Symbol type:_ `function`

#### `fio_io_listener_url`

```c
fio_buf_info_s fio_io_listener_url(fio_io_listener_s *l)
```

Returns the URL on which the listener is listening.

_Symbol type:_ `function`

#### `fio_io_listener_is_tls`

```c
int fio_io_listener_is_tls(fio_io_listener_s *l)
```

Returns true if the listener protocol has an attached TLS context.

_Symbol type:_ `function`

#### `fio_io_connect`

```c
fio_io_s *fio_io_connect(fio_io_connect_args_s args)
```

Connects to a specific URL, returning the `fio_io_s` IO object or `NULL`.

_Symbol type:_ `function`

#### `fio_io_connect`

```c
#define fio_io_connect(url_, ...)   \
  fio_io_connect((fio_io_connect_args_s){.url = url_, __VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_attach_fd`

```c
fio_io_s *fio_io_attach_fd(fio_socket_i fd, fio_io_protocol_s *protocol, void *udata, void *tls)
```

Attaches the socket in `fd` to the facio.io engine (reactor).

* `fd` should point to a valid socket.

* `protocol` may be the existing protocol or NULL (for partial hijack).

* `udata` is opaque user data and may be any value, including NULL.

* `tls` is a context for Transport Layer (Security) and can be used to
  redirect read/write operations, as set by the protocol.

Returns NULL on error. the `fio_io_s` pointer must NOT be used except
within proper callbacks.

_Symbol type:_ `function`

#### `fio_io_protocol_set`

```c
fio_io_protocol_s *fio_io_protocol_set(fio_io_s *io, fio_io_protocol_s *protocol)
```

Sets a new protocol object. `NULL` is a valid "only-write" protocol.

_Symbol type:_ `function`

#### `fio_io_protocol`

```c
fio_io_protocol_s *fio_io_protocol(fio_io_s *io)
```

Returns a pointer to the current protocol object.

If `protocol` wasn't properly set, the pointer might be NULL or invalid.

If `protocol` wasn't attached yet, may return the previous protocol.

_Symbol type:_ `function`

#### `fio_io_buffer`

```c
void *fio_io_buffer(fio_io_s *io)
```

Returns the a pointer to the memory buffer required by the protocol.

_Symbol type:_ `function`

#### `fio_io_buffer_len`

```c
size_t fio_io_buffer_len(fio_io_s *io)
```

Returns the length of the `buf` buffer.

_Symbol type:_ `function`

#### `fio_io_udata_set`

```c
void *fio_io_udata_set(fio_io_s *io, void *udata)
```

Associates a new `udata` pointer with the IO, returning the old `udata`

_Symbol type:_ `function`

#### `fio_io_udata`

```c
void *fio_io_udata(fio_io_s *io)
```

Returns the `udata` pointer associated with the IO.

_Symbol type:_ `function`

#### `fio_io_tls_set`

```c
void *fio_io_tls_set(fio_io_s *io, void *tls)
```

Associates a new `tls` pointer with the IO, returning the old `tls`

_Symbol type:_ `function`

#### `fio_io_tls`

```c
void *fio_io_tls(fio_io_s *io)
```

Returns the `tls` pointer associated with the IO.

_Symbol type:_ `function`

#### `fio_io_fd`

```c
fio_socket_i fio_io_fd(fio_io_s *io)
```

Returns the socket file descriptor (fd) associated with the IO.

_Symbol type:_ `function`

#### `fio_io_touch`

```c
void fio_io_touch(fio_io_s *io)
```

Resets a socket's timeout counter.

_Symbol type:_ `function`

#### `fio_io_read`

```c
size_t fio_io_read(fio_io_s *io, void *buf, size_t len)
```

Reads data to the buffer, if any data exists. Returns the number of bytes
read.

NOTE: zero (`0`) is a valid return value meaning no data was available.

_Symbol type:_ `function`

#### `fio_io_write2`

```c
void fio_io_write2(fio_io_s *io, fio_io_write_args_s args)
```

Writes data to the outgoing buffer and schedules the buffer to be sent.

_Symbol type:_ `function`

#### `fio_io_write2`

```c
#define fio_io_write2(io, ...)   \
  fio_io_write2(io, (fio_io_write_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_write`

```c
#define fio_io_write(io, buf_, len_)   \
  fio_io_write2(io, .buf = (buf_), .len = (len_), .copy = 1)
```

Helper macro for a common fio_io_write2 (copies the buffer).

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_sendfile`

```c
#define fio_io_sendfile(io, source_fd, offset_, bytes)   \
  fio_io_write2((io),   \
                .fd = (source_fd),   \
                .offset = (size_t)(offset_),   \
                .len = (bytes))
```

Sends data from a file as if it were a single atomic packet (sends up to
length bytes or until EOF is reached).

Once the file was sent, the `source_fd` will be closed using `close`.

The file will be buffered to the socket chunk by chunk, so that memory
consumption is capped.

`offset` dictates the starting point for the data to be sent and length sets
the maximum amount of data to be sent.

Closes the file on error.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_close`

```c
void fio_io_close(fio_io_s *io)
```

Marks the IO for closure as soon as scheduled data was sent.

_Symbol type:_ `function`

#### `fio_io_close_now`

```c
void fio_io_close_now(fio_io_s *io)
```

Marks the IO for immediate closure.

_Symbol type:_ `function`

#### `fio_io_dup`

```c
fio_io_s *fio_io_dup(fio_io_s *io)
```

Increases a IO's reference count, so it won't be automatically destroyed
when all tasks have completed.

Use this function in order to use the IO outside of a scheduled task.

This function is thread-safe.

_Symbol type:_ `function`

#### `fio_io_free`

```c
void fio_io_free(fio_io_s *io)
```

Decreases a IO's reference count, so it could be automatically destroyed
when all other tasks have completed.

Use this function once finished with a IO that was `dup`-ed.

This function is thread-safe.

_Symbol type:_ `function`

#### `fio_io_suspend`

```c
void fio_io_suspend(fio_io_s *io)
```

Suspends future `on_data` events for the IO.

_Symbol type:_ `function`

#### `fio_io_unsuspend`

```c
void fio_io_unsuspend(fio_io_s *io)
```

Listens for future `on_data` events related to the IO.

_Symbol type:_ `function`

#### `fio_io_is_suspended`

```c
int fio_io_is_suspended(fio_io_s *io)
```

Returns 1 if the IO handle was suspended.

_Symbol type:_ `function`

#### `fio_io_is_open`

```c
int fio_io_is_open(fio_io_s *io)
```

Returns 1 if the IO handle is marked as open.

_Symbol type:_ `function`

#### `fio_io_backlog`

```c
size_t fio_io_backlog(fio_io_s *io)
```

Returns the approximate number of bytes in the outgoing buffer.

_Symbol type:_ `function`

#### `fio_io_noop`

```c
void fio_io_noop(fio_io_s *io)
```

Does nothing.

_Symbol type:_ `function`

#### `fio_io_defer`

```c
void fio_io_defer(void (*task)(void *, void *), void *udata1, void *udata2)
```

Schedules a task for delayed execution. This function is thread-safe.

_Symbol type:_ `function`

#### `fio_io_run_every`

```c
void fio_io_run_every(fio_timer_schedule_args_s args)
```

Schedules a timer bound task, see `fio_timer_schedule`.

_Symbol type:_ `function`

#### `fio_io_run_every`

```c
#define fio_io_run_every(...)   \
  fio_io_run_every((fio_timer_schedule_args_s){__VA_ARGS__})
```

Schedules a timer bound task, see `fio_timer_schedule`.

Possible "named arguments" (fio_timer_schedule_args_s members) include:

* The timer function. If it returns a non-zero value, the timer stops:
       int (*fn)(void *, void *)
* Opaque user data:
       void *udata1
* Opaque user data:
       void *udata2
* Called when the timer is done (finished):
       void (*on_stop)(void *, void *)
* Timer interval, in milliseconds:
       uint32_t every
* The number of times the timer should be performed. -1 == infinity:
       int32_t repetitions

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_queue`

```c
fio_queue_s *fio_io_queue(void)
```

Returns a pointer for the IO reactor's queue.

_Symbol type:_ `function`

#### `fio_io_protocol_each`

```c
size_t fio_io_protocol_each(fio_io_protocol_s *protocol, void (*task)(fio_io_s *, void *udata2), void *udata2)
```

Performs a task for each IO in the stated protocol.

Call ONLY from the main IO thread (consider using `fio_io_defer`).

_Symbol type:_ `function`

#### `fio_io_env_get`

```c
void *fio_io_env_get(fio_io_s *io, fio_io_env_get_args_s)
```

Returns the named `udata` associated with the IO object (or `NULL`).

_Symbol type:_ `function`

#### `fio_io_env_get`

```c
#define fio_io_env_get(io, ...)   \
  fio_io_env_get(io, (fio_io_env_get_args_s){__VA_ARGS__})
```

Returns the named `udata` associated with the IO object (or `NULL`).

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_env_set`

```c
void fio_io_env_set(fio_io_s *io, fio_io_env_set_args_s)
```

Links an object to a connection's lifetime / environment.

The `on_close` callback will be called once the connection has died.

If the `io` is NULL, the value will be set for the global environment.

_Symbol type:_ `function`

#### `fio_io_env_set`

```c
#define fio_io_env_set(io, ...)   \
  fio_io_env_set(io, (fio_io_env_set_args_s){__VA_ARGS__})
```

Links an object to a connection's lifetime, calling the `on_close` callback
once the connection has died.

If the `io` is NULL, the value will be set for the global environment, in
which case the `on_close` callback will only be called once the process
exits.

This is a helper MACRO that allows the function to be called using named
arguments.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_env_unset`

```c
int fio_io_env_unset(fio_io_s *io, fio_io_env_get_args_s)
```

Un-links an object from the connection's lifetime, so it's `on_close`
callback will NOT be called.

Returns 0 on success and -1 if the object couldn't be found.

_Symbol type:_ `function`

#### `fio_io_env_unset`

```c
#define fio_io_env_unset(io, ...)   \
  fio_io_env_unset(io, (fio_io_env_get_args_s){__VA_ARGS__})
```

Un-links an object from the connection's lifetime, so it's `on_close`
callback will NOT be called.

Returns 0 on success and -1 if the object couldn't be found.

This is a helper MACRO that allows the function to be called using named
arguments.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_env_remove`

```c
int fio_io_env_remove(fio_io_s *io, fio_io_env_get_args_s)
```

Removes an object from the connection's lifetime / environment, calling it's
`on_close` callback as if the connection was closed.

_Symbol type:_ `function`

#### `fio_io_env_remove`

```c
#define fio_io_env_remove(io, ...)   \
  fio_io_env_remove(io, (fio_io_env_get_args_s){__VA_ARGS__})
```

Removes an object from the connection's lifetime / environment, calling it's
`on_close` callback as if the connection was closed.

This is a helper MACRO that allows the function to be called using named
arguments.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_tls_new`

```c
fio_io_tls_s *fio_io_tls_new(void)
```

Performs a `new` operation, returning a new `fio_io_tls_s` context.

_Symbol type:_ `function`

#### `fio_io_tls_from_url`

```c
fio_io_tls_s *fio_io_tls_from_url(fio_io_tls_s *target_or_null, fio_url_s url)
```

Takes a parsed URL and optional TLS target and returns a TLS if needed.

_Symbol type:_ `function`

#### `fio_io_tls_dup`

```c
fio_io_tls_s *fio_io_tls_dup(fio_io_tls_s *)
```

Performs a `dup` operation, increasing the object's reference count.

_Symbol type:_ `function`

#### `fio_io_tls_free`

```c
void fio_io_tls_free(fio_io_tls_s *)
```

Performs a `free` operation, reducing the reference count and freeing.

_Symbol type:_ `function`

#### `fio_io_tls_cert_add`

```c
fio_io_tls_s *fio_io_tls_cert_add(fio_io_tls_s *, const char *server_name, const char *public_cert_file, const char *private_key_file, const char *pk_password)
```

Adds a certificate a new SSL/TLS context / settings object (SNI support).

     fio_io_tls_cert_add(tls, "www.example.com",
                           "public_key.pem",
                           "private_key.pem", NULL );

NOTE: Except for the `tls` and `server_name` arguments, all arguments might
be `NULL`, which a context builder (`fio_io_functions_s`) should
treat as a request for a self-signed certificate. It may be silently ignored.

_Symbol type:_ `function`

#### `fio_io_tls_alpn_add`

```c
fio_io_tls_s *fio_io_tls_alpn_add(fio_io_tls_s *tls, const char *protocol_name, void (*on_selected)(fio_io_s *))
```

Adds an ALPN protocol callback to the SSL/TLS context.

The first protocol added will act as the default protocol to be selected.

A `NULL` protocol name will be silently ignored.

A `NULL` callback (`on_selected`) will be silently replaced with a no-op.

_Symbol type:_ `function`

#### `fio_io_tls_alpn_select`

```c
int fio_io_tls_alpn_select(fio_io_tls_s *tls, const char *protocol_name, size_t name_length, fio_io_s *)
```

Calls the `on_selected` callback for the `fio_io_tls_s` object.

_Symbol type:_ `function`

#### `fio_io_tls_trust_add`

```c
fio_io_tls_s *fio_io_tls_trust_add(fio_io_tls_s *, const char *public_cert_file)
```

Adds a certificate to the "trust" list, which automatically adds a peer
verification requirement.

If `public_cert_file` is `NULL`, implementation is expected to add the
system's default trust registry.

Note: when the `fio_io_tls_s` object is used for server connections, this
should limit connections to clients that connect using a trusted certificate.

     fio_io_tls_trust_add(tls, "google-ca.pem" );

_Symbol type:_ `function`

#### `fio_io_tls_cert_count`

```c
uintptr_t fio_io_tls_cert_count(fio_io_tls_s *tls)
```

Returns the number of `fio_io_tls_cert_add` instructions.

This could be used when deciding if to add a NULL instruction (self-signed).

If `fio_io_tls_cert_add` was never called, zero (0) is returned.

_Symbol type:_ `function`

#### `fio_io_tls_alpn_count`

```c
uintptr_t fio_io_tls_alpn_count(fio_io_tls_s *tls)
```

Returns the number of registered ALPN protocol names.

This could be used when deciding if protocol selection should be delegated to
the ALPN mechanism, or whether a protocol should be immediately assigned.

If no ALPN protocols are registered, zero (0) is returned.

_Symbol type:_ `function`

#### `fio_io_tls_trust_count`

```c
uintptr_t fio_io_tls_trust_count(fio_io_tls_s *tls)
```

Returns the number of `fio_io_tls_trust_add` instructions.

This could be used when deciding if to disable peer verification or not.

If `fio_io_tls_trust_add` was never called, zero (0) is returned.

_Symbol type:_ `function`

#### `fio_io_tls_each`

```c
int fio_io_tls_each(fio_io_tls_each_s)
```

Calls callbacks for certificate, trust certificate and ALPN added.

_Symbol type:_ `function`

#### `fio_io_tls_each`

```c
#define fio_io_tls_each(tls_, ...)   \
  fio_io_tls_each(((fio_io_tls_each_s){.tls = tls_, __VA_ARGS__}))
```

`fio_io_tls_each` helper macro, see `fio_io_tls_each_s` for named
arguments.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_tls_default_functions`

```c
fio_io_functions_s fio_io_tls_default_functions(fio_io_functions_s *)
```

If `NULL` returns current default, otherwise sets it. Set before start.

_Symbol type:_ `function`

#### `fio_io_async_queue`

```c
inline fio_queue_s *fio_io_async_queue(fio_io_async_s *q)
```

Returns the current task queue associated with the IO Async Queue.

_Symbol type:_ `function`

#### `fio_io_async_attach`

```c
void fio_io_async_attach(fio_io_async_s *q, uint32_t threads)
```

Attaches an IO Async Queue for use in multi-threaded (non IO) tasks.

This function can be called multiple times for the same (or other) queue, as
long as the async queue (`fio_io_async_s`) was previously initialized using
`FIO_IO_ASYN_INIT` or zeroed out. i.e.:

    static fio_io_async_s SLOW_HTTP_TASKS = FIO_IO_ASYN_INIT;
    fio_io_async_attach(&SLOW_HTTP_TASKS, 32);

_Symbol type:_ `function`

#### `fio_io_async`

```c
#define fio_io_async(q_, ...) fio_queue_push((q_)->q, __VA_ARGS__)
```

Pushes a task to an IO Async Queue (macro helper).

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_io_async_every`

```c
void fio_io_async_every(fio_io_async_s *q, fio_timer_schedule_args_s)
```

Schedules a timer bound task for the async queue (`fio_timer_schedule`).

_Symbol type:_ `function`

#### `fio_io_async_every`

```c
#define fio_io_async_every(async, ...)   \
  fio_io_async_every(async, (fio_timer_schedule_args_s){__VA_ARGS__})
```

Schedules a timer bound task, for the async queue, see `fio_timer_schedule`.

Possible "named arguments" (fio_timer_schedule_args_s members) include:

* The timer function. If it returns a non-zero value, the timer stops:
       int (*fn)(void *, void *)
* Opaque user data:
       void *udata1
* Opaque user data:
       void *udata2
* Called when the timer is done (finished):
       void (*on_stop)(void *, void *)
* Timer interval, in milliseconds:
       uint32_t every
* The number of times the timer should be performed. -1 == infinity:
       int32_t repetitions

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-404-ipc-h"></a> `./fio-stl/404 ipc.h`

41 public symbols.

### Macros

#### `FIO_IPC_URL_MAX_LENGTH`

```c
#define FIO_IPC_URL_MAX_LENGTH 1024
```



_Symbol type:_ `macro`

#### `FIO_IPC_MAX_LENGTH`

```c
#define FIO_IPC_MAX_LENGTH (128ULL * 1024U * 1024U)
```



_Symbol type:_ `macro`

#### `FIO_IPC_DATA`

```c
#define FIO_IPC_DATA(...)   \
  (fio_buf_info_s[]) {   \
    __VA_ARGS__, { .len = ((size_t)-1) }   \
  }
```

A helper macro for composing multiple buffers into the request. Fill with
fio_buf_info_s - will stop when a buffer has zero length. Use:

    .data = FIO_IPC_DATA(FIO_BUF_INFO1((char *)"example:"),
                         FIO_BUF_INFO2(&number, sizeof(number)))

_Symbol type:_ `macro`

#### `FIO_IPC_EXCLUDE_SELF`

```c
#define FIO_IPC_EXCLUDE_SELF ((fio_io_s *)((char *)-1LL))
```

Excludes current process from fio_ipc_local (to be set in ipc->from)

_Symbol type:_ `macro`

#### `FIO_IPC_FLAG_ENCRYPTED`

```c
#define FIO_IPC_FLAG_ENCRYPTED ((uint16_t)1UL << 0)
```

Set if message is in its encrypted state - do NOT edit manually

_Symbol type:_ `macro`

#### `FIO_IPC_FLAG_DONE`

```c
#define FIO_IPC_FLAG_DONE ((uint16_t)1UL << 1)
```

If set, calls `on_done` rather than `call` - requires FIO_IPC_FLAG_REPLY

_Symbol type:_ `macro`

#### `FIO_IPC_FLAG_OPCODE`

```c
#define FIO_IPC_FLAG_OPCODE ((uint16_t)1UL << 2)
```

Set if message callbacks are mapped using op-codes

_Symbol type:_ `macro`

#### `FIO_IPC_FLAG_WORKERS`

```c
#define FIO_IPC_FLAG_WORKERS ((uint16_t)1UL << 3)
```

If set, delivered to worker processes as well as master

_Symbol type:_ `macro`

#### `FIO_IPC_FLAG_CLUSTER`

```c
#define FIO_IPC_FLAG_CLUSTER ((uint16_t)1UL << 4)
```

If set, delivered to remote machines - requires FIO_IPC_FLAG_OPCODE

_Symbol type:_ `macro`

#### `FIO_IPC_FLAG_REPLY`

```c
#define FIO_IPC_FLAG_REPLY ((uint16_t)1UL << 5)
```

If set, calls `on_reply` rather than `call`

_Symbol type:_ `macro`

#### `FIO_IPC_FLAG_PING`

```c
#define FIO_IPC_FLAG_PING ((uint16_t)1UL << 6)
```

If set, this is an internal ping/keepalive message (not dispatched to user)

_Symbol type:_ `macro`

#### `FIO_IPC_FLAG_TEST`

```c
#define FIO_IPC_FLAG_TEST(msg, flag) (((msg)->routing_flags & flag) == flag)
```

If flag is set == 1, otherwise zero

_Symbol type:_ `macro`

#### `FIO_IPC_FLAG_IF`

```c
#define FIO_IPC_FLAG_IF(bcond, flag) (((uint16_t)0UL - (bcond)) & flag)
```

If flag is set == flag, otherwise zero

_Symbol type:_ `macro`

### Types

#### `fio_ipc_s`

```c
struct fio_ipc_s {
fio_io_s *from; /* IO to caller - set by receiver (unsent) */
/* ----- wire format starts here ----- */
uint32_t len; /* Length of data[] (AAD - authenticated, unencrypted) */
uint16_t flags; /* User settable flags (AAD - authenticated, unencrypted) */
uint16_t routing_flags; /* Internal (AAD - authenticated, unencrypted) */
uint64_t timestamp; /* timestamp (unencrypted, used for nonce) */
uint64_t id; /* 8 random bytes (unencrypted, used for nonce) */
union {
void (*call)(struct fio_ipc_s *); /* function pointer to call (encrypted) */
uint32_t opcode; /* op-code to execute*/
};
void (*on_reply)(struct fio_ipc_s *); /* run on caller (encrypted) */
void (*on_done)(struct fio_ipc_s *); /* run on caller (encrypted) */
void *udata; /* opaque, valid only in caller (encrypted) */
char data[]; /* Variable-length data + 16-byte MAC at end (encrypted) */
}
```

IPC message structure (reference counted)

_Symbol type:_ `type`

#### `fio_ipc_args_s`

```c
typedef struct {
void (*call)(fio_ipc_s *); /* function to call */
void (*on_reply)(fio_ipc_s *); /* (optional) reply callback */
void (*on_done)(fio_ipc_s *); /* (optional) reply finished callback */
fio_io_s *exclude; /* (optional) IO to exclude from delivery */
uint64_t timestamp; /* (optional) to force timestamp */
uint64_t id; /* (optional) to force an id value */
uint32_t opcode; /* replaces `call` with op-code if non-zero */
uint16_t flags; /* (optional) user-opaque flags */
bool cluster; /* if set, this is intended for all machines in cluster */
bool workers; /* if set, this is intended for master + workers */
void *udata; /* opaque pointer data for reply */
fio_buf_info_s *data; /* payload (see FIO_IPC_DATA) */
} fio_ipc_args_s
```

IPC call arguments

_Symbol type:_ `type`

#### `fio_ipc_opcode_s`

```c
struct fio_ipc_opcode_s {
uint32_t opcode; /* Unique op-code value */
void (*call)(struct fio_ipc_s *); /* function to call */
void (*on_reply)(struct fio_ipc_s *); /* (optional) reply callback */
void (*on_done)(struct fio_ipc_s *); /* (optional) reply finished callback */
void *udata; /* opaque, valid only in caller (encrypted) */
}
```



_Symbol type:_ `type`

#### `fio_ipc_reply_args_s`

```c
typedef struct {
fio_ipc_s *ipc;
fio_buf_info_s *data;
uint64_t timestamp; /* (optional) override timestamp, 0 = use current time */
uint64_t id; /* (optional) override id, 0 = use original request id */
uint16_t flags; /* (optional) override flags, 0 = use original request */
uint8_t done;
uint8_t flags_set; /* set to 1 if flags should be used (allows flags=0) */
} fio_ipc_reply_args_s
```

IPC reply arguments

_Symbol type:_ `type`

### Functions

#### `fio_ipc_url`

```c
const char *fio_ipc_url(void)
```

Returns the IPC url to listen to (for incoming connections).

_Symbol type:_ `function`

#### `fio_ipc_url_set`

```c
int fio_ipc_url_set(const char *url)
```

Sets the IPC url to listen to (for incoming connections).

Can only be called on the master process and only before the IO reactor.

_Symbol type:_ `function`

#### `fio_ipc_opcode_register`

```c
int fio_ipc_opcode_register(fio_ipc_opcode_s opcode)
```

Registers an op-code for message routing.

There are two types of messages:
1. Fast Path - function pointers in the message payload (local IPC only).
2. Safe Path - Op-Code in the `call` payload (multi-machine RCP).

Op-Codes MUST be non-zero uint32_t values.
RESERVED: op-codes >= 0xFF000000 are reserved for internal use.

Note: Thread safety requires that this be called before fio_io_start.

Returns -1 or failure (not on master process / already registered)

_Symbol type:_ `function`

#### `fio_ipc_opcode_register`

```c
#define fio_ipc_opcode_register(...)   \
  fio_ipc_opcode_register((fio_ipc_opcode_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ipc_opcode`

```c
const fio_ipc_opcode_s *fio_ipc_opcode(uint32_t opcode)
```

Returns a pointer to a registered op-code, or NULL if missing.

_Symbol type:_ `function`

#### `fio_ipc_call`

```c
#define fio_ipc_call(...) fio_ipc_send(fio_ipc_new(__VA_ARGS__))
```

Call arbitrary code in master process (worker → master / master → master).

The `call` function pointer is executed on the master process.

Replies are sent back to the caller via `on_reply` callback.
When all replies are done, `on_done` is called.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ipc_local`

```c
#define fio_ipc_local(...) fio_ipc_send(fio_ipc_new(.workers = 1, __VA_ARGS__))
```

Call arbitrary code in master process and workers (all local).

Replies can be sent back to the caller only from its local master process.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ipc_cluster`

```c
#define fio_ipc_cluster(...)   \
  fio_ipc_send(fio_ipc_new(.cluster = 1, __VA_ARGS__))
```

Call arbitrary code in master process of every machine in cluster.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ipc_broadcast`

```c
#define fio_ipc_broadcast(...)   \
  fio_ipc_send(fio_ipc_new(.workers = 1, .cluster = 1, __VA_ARGS__))
```

Call arbitrary code in master process of every machine in cluster.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ipc_reply`

```c
void fio_ipc_reply(fio_ipc_reply_args_s args)
```

Send a response to the caller process (master → caller).

Can be called multiple times for streaming responses.
Set `done = 1` on the last reply.

_Symbol type:_ `function`

#### `fio_ipc_reply`

```c
#define fio_ipc_reply(r, ...)   \
  fio_ipc_reply((fio_ipc_reply_args_s){.ipc = (r), __VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ipc_new`

```c
fio_ipc_s *fio_ipc_new(fio_ipc_args_s args)
```

Authors a message without sending it.

Used internally but available for "faking" IPC or when composing a unified
code path for local execution.

_Symbol type:_ `function`

#### `fio_ipc_new`

```c
#define fio_ipc_new(...) fio_ipc_new((fio_ipc_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_ipc_dup`

```c
fio_ipc_s *fio_ipc_dup(fio_ipc_s *msg)
```

Duplicate a message (increment reference count).

Use when storing messages for later processing.
Every dup() must be matched with a free().

_Symbol type:_ `function`

#### `fio_ipc_free`

```c
void fio_ipc_free(fio_ipc_s *msg)
```

Free a message (decrement reference count).

Message is destroyed when reference count reaches zero.

_Symbol type:_ `function`

#### `fio_ipc_detach`

```c
void fio_ipc_detach(fio_ipc_s *msg)
```

Detaches the IPC message from it's originating IO.

Call if storing or performing non-IPC actions using the IPC message.

_Symbol type:_ `function`

#### `fio_ipc_send`

```c
void fio_ipc_send(fio_ipc_s *ipc)
```

Encrypts the IPC message(!), sends it for execution and frees it.

Message will be sent according to the flags set (see fio_ipc_new):
- Only to Master (fio_ipc_call);
- To Master and Workers (fio_ipc_local);
- To Master on Every Machine (fio_ipc_cluster);
- To Master and Workers on Every Machine (fio_ipc_broadcast);

Note: Takes ownership of the message's memory.

Note: overwrites after_send unless `ipc->from == FIO_IPC_EXCLUDE_SELF`

Note: excludes ipc->from.

Note: The message is encrypted and unusable once call returns - pass it the
last reference.

_Symbol type:_ `function`

#### `fio_ipc_send_to`

```c
void fio_ipc_send_to(fio_io_s *to, fio_ipc_s *ipc)
```

Encrypts the IPC message(!) and sends it to target IO - frees the message.

Note: Takes ownership of the message's memory.

Note: The message is encrypted and unusable until the message was sent and
either freed or the `after_send` callback was called.

_Symbol type:_ `function`

#### `fio_ipc_after_send`

```c
void fio_ipc_after_send(fio_ipc_s *ipc, void (*fn)(fio_ipc_s *, void *), void *udata)
```



_Symbol type:_ `function`

#### `fio_ipc_encrypt`

```c
void fio_ipc_encrypt(fio_ipc_s *m)
```

Encrypt IPC message before sending them anywhere.

_Symbol type:_ `function`

#### `fio_ipc_decrypt`

```c
inline int fio_ipc_decrypt(fio_ipc_s *m)
```

Decrypt IPC message when received.

_Symbol type:_ `function`

#### `fio_ipc_cluster_listen`

```c
fio_io_listener_s *fio_ipc_cluster_listen(uint16_t port)
```

Listens to cluster connections on the port listed, auto-connects to peers.

This does NOT improve message exchange or pub/sub performance. This is
designed for downtime mitigation (rotating pods) / data tunneling and client
load balancing (without message load balancing).

All server instances get all `cluster` messages (e.g., pub/sub cluster).

Note: uses the environment's (shared) secret for rudimentary encryption
without forward secrecy. Rotate secrets when possible (requires restart).
Good for trusted data centers, Kubernetes pods, etc'.

_Symbol type:_ `function`

#### `fio_ipc_cluster_connect`

```c
void fio_ipc_cluster_connect(const char *url)
```

Manually connects to cluster peers. Usually unnecessary.

_Symbol type:_ `function`

#### `fio_ipc_cluster_port`

```c
uint16_t fio_ipc_cluster_port(void)
```

Returns the last port number passed to either fio_ipc_cluster_listen or
fio_ipc_cluster_connect - zero if none.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-405-tls13-h"></a> `./fio-stl/405 tls13.h`

1 public symbols.

### Functions

#### `fio_tls13_io_functions`

```c
fio_io_functions_s fio_tls13_io_functions(void)
```

Returns the TLS 1.3 IO functions.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-420-pubsub-h"></a> `./fio-stl/420 pubsub.h`

26 public symbols.

### Macros

#### `FIO_PUBSUB_FUTURE_LIMIT_MS`

```c
#define FIO_PUBSUB_FUTURE_LIMIT_MS 60000ULL
```

Maximum time in milliseconds to allow "future" messages to be delivered.

If the module detects someone publishing a future message, it will refuse to
deliver the message to subscribers.

Instead, the module will notify the history engines, allowing them to buffer
future messages for future delivery.

_Symbol type:_ `macro`

#### `FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT`

```c
#define FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT (1ULL << 28)
```

The default cache limit - 256Mb

_Symbol type:_ `macro`

### Types

#### `fio_pubsub_subscribe_args_s`

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
fio_io_s *io;
/**
* A named `channel` to which the message was sent.
*
* Subscriptions require a match by both channel name and namespace filter.
*/
fio_buf_info_s channel;
/**
* The callback to be called for each message forwarded to the subscription.
*/
void (*on_message)(fio_pubsub_msg_s *msg);
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
/** Replay cached messages (if any) since supplied time in milliseconds. */
uint64_t replay_since;
/** A numerical namespace `filter` subscribers need to match. */
int16_t filter;
/** If set, pattern matching will be used (name is a pattern). */
uint8_t is_pattern;
/** If set, subscription will be limited to the root / master process. */
uint8_t master_only;
} fio_pubsub_subscribe_args_s
```



_Symbol type:_ `type`

#### `fio_pubsub_publish_args_s`

```c
typedef struct {
struct fio_pubsub_engine_s const *engine;
fio_io_s *from;
uint64_t id;
uint64_t timestamp;
fio_buf_info_s channel;
fio_buf_info_s message;
int16_t filter;
} fio_pubsub_publish_args_s
```

Publish arguments

_Symbol type:_ `type`

#### `fio_pubsub_engine_s`

```c
struct fio_pubsub_engine_s {
/** Called when engine is detached */
void (*detached)(const struct fio_pubsub_engine_s *eng);
/** Called when a subscription is created */
void (*subscribe)(const struct fio_pubsub_engine_s *eng,
const fio_buf_info_s channel,
int16_t filter);
/** Called when a pattern subscription is created */
void (*psubscribe)(const struct fio_pubsub_engine_s *eng,
const fio_buf_info_s channel,
int16_t filter);
/** Called when a subscription is removed */
void (*unsubscribe)(const struct fio_pubsub_engine_s *eng,
const fio_buf_info_s channel,
int16_t filter);
/** Called when a pattern subscription is removed */
void (*punsubscribe)(const struct fio_pubsub_engine_s *eng,
const fio_buf_info_s channel,
int16_t filter);
/** Called when a message is published */
void (*publish)(const struct fio_pubsub_engine_s *eng,
const fio_pubsub_msg_s *msg);
}
```

Engine structure for external pub/sub backends.

EXECUTION CONTEXT:
- Publish callback can be called from any thread / process.
- Subscription callbacks are called from the MASTER process only
- Subscription callbacks are called from the main event loop thread
- Callbacks MUST NOT block (defer long operations)

_Symbol type:_ `type`

#### `fio_pubsub_history_s`

```c
struct fio_pubsub_history_s {
/** Cleanup callback - called when history manager is detached */
void (*detached)(const struct fio_pubsub_history_s *hist);
/**
* Stores a message in history.
*
* Returns 0 on success, -1 on error.
* Called when a message is published (master only).
*/
int (*push)(const struct fio_pubsub_history_s *hist, fio_pubsub_msg_s *msg);
/**
* Replay messages since timestamp by calling callback for each.
*
* Returns 0 if replay was handled, -1 if this manager cannot replay.
*
* MUST call the `on_done` callback to handle possible cleanup.
*/
int (*replay)(const struct fio_pubsub_history_s *hist,
fio_buf_info_s channel,
int16_t filter,
uint64_t since,
void (*on_message)(fio_pubsub_msg_s *msg, void *udata),
void (*on_done)(void *udata),
void *udata);
/**
* Get oldest available timestamp for a channel.
*
* Returns UINT64_MAX if no history available.
*/
uint64_t (*oldest)(const struct fio_pubsub_history_s *hist,
fio_buf_info_s channel,
int16_t filter);
}
```

History storage interface - completely separate from engines.

EXECUTION CONTEXT:
- All callbacks are called from the MASTER process only
- Callbacks are called from the main event loop thread
- Callbacks MUST NOT block (defer long operations)
- Callbacks SHOULD NOT be called directly by the user

_Symbol type:_ `type`

### Functions

#### `fio_pubsub_subscribe`

```c
void fio_pubsub_subscribe(fio_pubsub_subscribe_args_s args)
```

Subscribe to a channel.

In worker processes, this uses IPC to notify the master.

_Symbol type:_ `function`

#### `fio_pubsub_subscribe`

```c
#define fio_pubsub_subscribe(...)   \
  fio_pubsub_subscribe((fio_pubsub_subscribe_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_pubsub_unsubscribe`

```c
int fio_pubsub_unsubscribe(fio_pubsub_subscribe_args_s args)
```

Unsubscribe from a channel.

Returns 0 on success, -1 if subscription not found.

_Symbol type:_ `function`

#### `fio_pubsub_unsubscribe`

```c
#define fio_pubsub_unsubscribe(...)   \
  fio_pubsub_unsubscribe((fio_pubsub_subscribe_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_pubsub_publish`

```c
void fio_pubsub_publish(fio_pubsub_publish_args_s args)
```

Publish a message to a channel.

In worker processes, this uses IPC to notify the master.

_Symbol type:_ `function`

#### `fio_pubsub_publish`

```c
#define fio_pubsub_publish(...)   \
  fio_pubsub_publish((fio_pubsub_publish_args_s){__VA_ARGS__})
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_pubsub_defer`

```c
void fio_pubsub_defer(fio_pubsub_msg_s *msg)
```

Pushes execution of the on_message callback to the end of the queue.

_Symbol type:_ `function`

#### `fio_pubsub_match_fn_set`

```c
void fio_pubsub_match_fn_set(uint8_t (*match_cb)(fio_str_info_s, fio_str_info_s))
```



_Symbol type:_ `function`

#### `fio_pubsub_msg2ipc`

```c
fio_ipc_s *fio_pubsub_msg2ipc(fio_pubsub_msg_s *msg)
```

Returns the underlying IPC message buffer carrying the message data.

This allows message deferral (use fio_ipc_dup) and tighter control over the
message's lifetime.

_Symbol type:_ `function`

#### `fio_pubsub_ipc2msg`

```c
fio_pubsub_msg_s fio_pubsub_ipc2msg(fio_ipc_s *ipc)
```

Extract pub/sub message from IPC message

_Symbol type:_ `function`

#### `fio_pubsub_engine_attach`

```c
void fio_pubsub_engine_attach(fio_pubsub_engine_s *engine)
```

Attach an engine to the pub/sub system - moves ownership to the system.

_Symbol type:_ `function`

#### `fio_pubsub_engine_detach`

```c
void fio_pubsub_engine_detach(fio_pubsub_engine_s *engine)
```

Detach an engine from the pub/sub system - frees system's reference.

_Symbol type:_ `function`

#### `fio_pubsub_engine_ipc`

```c
fio_pubsub_engine_s const *fio_pubsub_engine_ipc(void)
```

Returns the builtin engine for publishing to the process group (IPC).

_Symbol type:_ `function`

#### `fio_pubsub_engine_cluster`

```c
fio_pubsub_engine_s const *fio_pubsub_engine_cluster(void)
```

Returns the builtin engine for multi-machine cluster publishing (RPC).

_Symbol type:_ `function`

#### `fio_pubsub_engine_default`

```c
fio_pubsub_engine_s const *fio_pubsub_engine_default(void)
```

Returns the current default engine associated with the pub/sub system.

_Symbol type:_ `function`

#### `fio_pubsub_engine_default_set`

```c
fio_pubsub_engine_s const *fio_pubsub_engine_default_set( fio_pubsub_engine_s const *engine)
```

Sets the current default engine associated with the pub/sub system.

_Symbol type:_ `function`

#### `fio_pubsub_history_attach`

```c
int fio_pubsub_history_attach(const fio_pubsub_history_s *manager, uint8_t priority)
```

Attach a history manager with the given priority.

Multiple history managers can be attached. All managers receive push()
calls when messages are published. For replay(), managers are tried in
priority order (highest first) until one can handle the request.

_Symbol type:_ `function`

#### `fio_pubsub_history_detach`

```c
void fio_pubsub_history_detach(const fio_pubsub_history_s *manager)
```

Detach a history manager.

_Symbol type:_ `function`

#### `fio_pubsub_history_push_all`

```c
void fio_pubsub_history_push_all(fio_pubsub_msg_s *msg)
```

Pushes a pub/sub message to all history containers.

Use ONLY by an engine, if the message needs to be saved to the history
managers in the process but is never delivered locally.

_Symbol type:_ `function`

#### `fio_pubsub_history_cache`

```c
fio_pubsub_history_s const *fio_pubsub_history_cache(size_t size_limit)
```

Get the built-in in-memory history manager and set it's byte-size limit.

A zero value size_limit will be replaced with the following default:
- Environment value of WEBSITE_MEMORY_LIMIT_MB * 1024 * 1024
- Environment value of WEBSITE_MEMORY_LIMIT_KB
- Environment value of WEBSITE_MEMORY_LIMIT
- FIO_PUBSUB_HISTORY_DEFAULT_CACHE_SIZE_LIMIT

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-422-redis-h"></a> `./fio-stl/422 redis.h`

7 public symbols.

### Macros

#### `FIO_REDIS_READ_BUFFER`

```c
#define FIO_REDIS_READ_BUFFER 32768
```

Size of the read buffer for Redis connections

_Symbol type:_ `macro`

### Types

#### `fio_redis_args_s`

```c
typedef struct {
/**
* Redis server URL.
*
* Supported formats:
* - "redis://host:port"
* - "redis://host" (default port 6379)
* - "host:port" (no scheme)
* - "host" (no scheme, default port 6379)
* - NULL or empty → defaults to "localhost:6379"
*/
const char *url;
/** Redis server's password, if any (for AUTH command) */
const char *auth;
/** Length of auth string (0 = auto-detect with strlen) */
size_t auth_len;
/** Ping interval in seconds (0 = default 300 seconds) */
uint8_t ping_interval;
} fio_redis_args_s
```

Arguments for creating a Redis engine

_Symbol type:_ `type`

### Functions

#### `fio_redis_new`

```c
fio_pubsub_engine_s *fio_redis_new(fio_redis_args_s args)
```

Creates a Redis pub/sub engine with reference count = 1.

The engine is active only after the IO reactor starts running.

The caller owns the returned reference and must call fio_redis_free()
when done. Attaching to pub/sub does NOT transfer ownership.

Returns a pointer to the engine or NULL on error.

_Symbol type:_ `function`

#### `fio_redis_new`

```c
#define fio_redis_new(...) fio_redis_new((fio_redis_args_s){__VA_ARGS__})
```

Creates a Redis pub/sub engine (named arguments helper macro).

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_redis_dup`

```c
fio_pubsub_engine_s *fio_redis_dup(fio_pubsub_engine_s *engine)
```

Increments the reference count and returns the engine.

Use this when you need to share the engine across multiple owners.
Each call to fio_redis_dup() must be balanced with fio_redis_free().

_Symbol type:_ `function`

#### `fio_redis_free`

```c
void fio_redis_free(fio_pubsub_engine_s *engine)
```

Releases the caller's reference to the engine.

This function simply decrements the reference count. If the ref reaches 0
(no other refs held), fio___redis_destroy() fires immediately:
sets running=0, closes connections, drains the command queue, frees memory.

The engine stays alive as long as any other ref is held (e.g. the pub/sub
system's ref taken via subscribe/psubscribe). The pub/sub system releases
its ref via the on_detached callback when fio_pubsub_engine_detach() fires.

The caller does NOT need to call fio_pubsub_engine_detach() before freeing.
Calling fio_pubsub_engine_detach() before fio_redis_free() is also safe
(detach releases the system ref; free releases the caller ref).

Safe to call with NULL (no-op).

_Symbol type:_ `function`

#### `fio_redis_send`

```c
int fio_redis_send(fio_pubsub_engine_s *engine, FIOBJ command, void (*callback)(fio_pubsub_engine_s *e, FIOBJ reply, void *udata), void *udata)
```

Sends a Redis command through the engine's connection.

The response will be sent back using the optional callback. `udata` is passed
along untouched.

The `command` should be a FIOBJ array containing the command and arguments.

Note: NEVER call Pub/Sub commands (SUBSCRIBE, PSUBSCRIBE, UNSUBSCRIBE,
PUNSUBSCRIBE) using this function, as it will violate the Redis connection's
protocol.

Returns 0 on success, -1 on error.

_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-431-http-handle-h"></a> `./fio-stl/431 http handle.h`

118 public symbols.

### Macros

#### `FIO_HTTP_EXACT_LOGGING`

```c
#define FIO_HTTP_EXACT_LOGGING 0
```

By default, facil.io logs the HTTP request cycle using a fuzzy starting and
ending point for the time stamp.

The fuzzy timestamp includes delays that aren't related to the HTTP request
and may ignore time passed due to timestamp caching.

On the other hand, `FIO_HTTP_EXACT_LOGGING` collects exact time stamps to
measure the time it took to process the HTTP request (excluding time spent
reading / writing the data from the network).

Due to the preference to err on the side of higher performance, fuzzy
time-stamping is the default.

_Symbol type:_ `macro`

#### `FIO_HTTP_BODY_RAM_LIMIT`

```c
#define FIO_HTTP_BODY_RAM_LIMIT (1 << 17)
```

The HTTP handle automatically switches between RAM storage and file storage
once the HTTP body (payload) reaches a certain size. This control this point
of transition

_Symbol type:_ `macro`

#### `FIO_HTTP_CACHE_LIMIT`

```c
#define FIO_HTTP_CACHE_LIMIT 0 /* ((1UL << 6) + (1UL << 5)) */
```

Each of the HTTP String Caches will be limited to this String count.

_Symbol type:_ `macro`

#### `FIO_HTTP_CACHE_STR_MAX_LEN`

```c
#define FIO_HTTP_CACHE_STR_MAX_LEN (1 << 12)
```

The HTTP handle will avoid caching strings longer than this value.

_Symbol type:_ `macro`

#### `FIO_HTTP_CACHE_USES_MUTEX`

```c
#define FIO_HTTP_CACHE_USES_MUTEX 1
```

The HTTP cache will use a mutex to allow headers to be set concurrently.

_Symbol type:_ `macro`

#### `FIO_HTTP_PRE_CACHE_KNOWN_HEADERS`

```c
#define FIO_HTTP_PRE_CACHE_KNOWN_HEADERS 1
```

Adds a static cache for common HTTP header names.

_Symbol type:_ `macro`

#### `FIO_HTTP_DEFAULT_INDEX_FILENAME`

```c
#define FIO_HTTP_DEFAULT_INDEX_FILENAME "index"
```

The default file name when a static file response points to a folder.

_Symbol type:_ `macro`

#### `FIO_HTTP_STATIC_FILE_COMPLETION`

```c
#define FIO_HTTP_STATIC_FILE_COMPLETION 1
```

Attempts to auto-complete static file paths with missing extensions.

_Symbol type:_ `macro`

#### `FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT`

```c
#define FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT (1UL << 21) /* 2 MiB */
```

Maximum file size (in bytes) for on-disk static file compression.

_Symbol type:_ `macro`

#### `FIO_HTTP_LOG_X_REQUEST_START`

```c
#define FIO_HTTP_LOG_X_REQUEST_START 1
```



_Symbol type:_ `macro`

#### `FIO_HTTP_ENFORCE_LOWERCASE_HEADERS`

```c
#define FIO_HTTP_ENFORCE_LOWERCASE_HEADERS 0
```

If true, the HTTP handle will copy input header names to lower case.

_Symbol type:_ `macro`

#### `FIO_HTTP_PATH_EACH`

```c
#define FIO_HTTP_PATH_EACH(path, pos)
```

Loops over each section of the path, decrypting percent encoding as
necessary.

The macro accepts the following:

- `path`: the path string - accessible using fio_http_path(h).
- `pos` : the name of the variable to use for accessing the section.

The variable `pos` is a `fio_buf_info_s`.

**Note**: the macro will break if a path's section length is greater than
          (about) 4063 bytes.

_Symbol type:_ `macro`

#### `FIO_HTTP_HEADER_EACH_VALUE`

```c
#define FIO_HTTP_HEADER_EACH_VALUE(/* fio_http_s */ http_handle,   \
                                   /* int / bool */ is_request,   \
                                   /* fio_str_info_s */ header_name,   \
                                   /* chosen var named */ value)   \
  for (char fio___buf__##value##__[2048], /* allocate buffer on stack */   \
           *fio___buf__##value##_ptr = NULL;   \
       !fio___buf__##value##_ptr;   \
       fio___buf__##value##_ptr = fio___buf__##value##__)   \
    for (fio_str_info_s fio___buf__##value##__str = /* declare buffer var */   \
         FIO_STR_INFO3(fio___buf__##value##__, 0, 2048);   \
         fio___buf__##value##__str.buf == fio___buf__##value##__;   \
         fio___buf__##value##__str.buf = fio___buf__##value##__ + 1)   \
      if (!((is_request ? fio_http_request_header_parse   \
                        : fio_http_response_header_parse)(   \
              http_handle, /* parse headers */   \
              &fio___buf__##value##__str,   \
              header_name)))   \
  FIO_HTTP_PARSED_HEADER_EACH(fio___buf__##value##__str, value) /* loop   \
                                                                 */
```

Parses header for multiple values and properties and iterates over all
values.

This MACRO will allocate 2048 bytes on the stack for parsing the header
values and properties, if more space is necessary dig deeper.

Use FIO_HTTP_HEADER_VALUE_EACH_PROPERTY to iterate over a value's properties.

_Symbol type:_ `macro`

#### `FIO_HTTP_HEADER_VALUE_EACH_PROPERTY`

```c
#define FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(/* fio_str_info_s   */ value,   \
                                            /* chosen var named */ property)
```

Iterated through the properties associated with a parsed header values.

_Symbol type:_ `macro`

#### `FIO_HTTP_PARSED_HEADER_EACH`

```c
#define FIO_HTTP_PARSED_HEADER_EACH(/* fio_str_info_s   */ buf_parsed,   \
                                    /* chosen var named */ value)
```

Used internally to iterate over a parsed header buffer.

_Symbol type:_ `macro`

#### `FIO_HTTP_STATE_STREAMING`

```c
#define FIO_HTTP_STATE_STREAMING      1
```



_Symbol type:_ `macro`

#### `FIO_HTTP_STATE_FINISHED`

```c
#define FIO_HTTP_STATE_FINISHED       2
```



_Symbol type:_ `macro`

#### `FIO_HTTP_STATE_UPGRADED`

```c
#define FIO_HTTP_STATE_UPGRADED       4
```



_Symbol type:_ `macro`

#### `FIO_HTTP_STATE_WEBSOCKET`

```c
#define FIO_HTTP_STATE_WEBSOCKET      8
```



_Symbol type:_ `macro`

#### `FIO_HTTP_STATE_SSE`

```c
#define FIO_HTTP_STATE_SSE            16
```



_Symbol type:_ `macro`

#### `FIO_HTTP_STATE_COOKIES_PARSED`

```c
#define FIO_HTTP_STATE_COOKIES_PARSED 32
```



_Symbol type:_ `macro`

#### `FIO_HTTP_STATE_FREEING`

```c
#define FIO_HTTP_STATE_FREEING        64
```



_Symbol type:_ `macro`

#### `FIO_HTTP_CFLAG_COMPRESS_DYNAMIC`

```c
#define FIO_HTTP_CFLAG_COMPRESS_DYNAMIC 1
```

Controller flags (cflags) for opt-in compression features.

_Symbol type:_ `macro`

#### `FIO_HTTP_CFLAG_COMPRESS_WS`

```c
#define FIO_HTTP_CFLAG_COMPRESS_WS      2
```



_Symbol type:_ `macro`

#### `FIO_HTTP_CFLAG_COMPRESS_STATIC`

```c
#define FIO_HTTP_CFLAG_COMPRESS_STATIC  4
```



_Symbol type:_ `macro`

### Types

#### `fio_http_s`

```c
struct fio_http_s
```

The HTTP Handle type.

Note that the type is NOT designed to be thread-safe.

_Symbol type:_ `type`

#### `fio_http_controller_s`

```c
struct fio_http_controller_s
```

The HTTP Controller points to all the callbacks required by the HTTP Handler.

This allows the HTTP Handler to be somewhat protocol agnostic.

Note: if the controller callbacks aren't thread-safe, than the `http_write`
function MUST NOT be called from any thread except the thread that the
controller is expecting.

_Symbol type:_ `type`

#### `fio_http_cookie_same_site_e`

```c
enum fio_http_cookie_same_site_e {
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
}
```

Possible values for the `same_site` property in the cookie settings.

See: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie

_Symbol type:_ `type`

#### `fio_http_cookie_args_s`

```c
struct fio_http_cookie_args_s {
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
}
```

This is a helper for setting cookie data.

This struct is used together with the `fio_http_cookie_set` macro. i.e.:

      fio_http_set_cookie(h,
                     .name = FIO_STR_INFO1("my_cookie"),
                     .value = FIO_STR_INFO1("data"));

_Symbol type:_ `type`

#### `fio_http_write_args_s`

```c
struct fio_http_write_args_s {
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
/** If the data is a buffer / a file - should it be copied? */
int copy;
/**
* If `finish` is set, this data marks the end of the response.
*
* Otherwise the response will stream the data.
*/
int finish;
}
```

Arguments for the fio_http_write function.

_Symbol type:_ `type`

#### `fio_http_body_parse_callbacks_s`

```c
typedef struct {
/* ===== Primitives ===== */
/** NULL / nil was detected. Returns new object. */
void *(*on_null)(void *udata);
/** TRUE was detected. Returns new object. */
void *(*on_true)(void *udata);
/** FALSE was detected. Returns new object. */
void *(*on_false)(void *udata);
/** Number was detected. Returns new object. */
void *(*on_number)(void *udata, int64_t num);
/** Float was detected. Returns new object. */
void *(*on_float)(void *udata, double num);
/** String was detected. Returns new object. */
void *(*on_string)(void *udata, const void *data, size_t len);
/* ===== Containers ===== */
/** Array was detected. Returns context for this array. */
void *(*on_array)(void *udata, void *parent);
/** Map / Object was detected. Returns context for this map. */
void *(*on_map)(void *udata, void *parent);
/** Push value to array. Returns non-zero on error. */
int (*array_push)(void *udata, void *array, void *value);
/** Set key-value pair in map. Returns non-zero on error. */
int (*map_set)(void *udata, void *map, void *key, void *value);
/** Called when array parsing is complete. */
void (*array_done)(void *udata, void *array);
/** Called when map parsing is complete. */
void (*map_done)(void *udata, void *map);
/* ===== File Uploads (multipart) ===== */
/**
* Called when a file upload starts.
*
* Return context for this file (e.g., fd, stream, buffer).
* Return NULL to skip this file (on_file_data/on_file_done won't be called).
*/
void *(*on_file)(void *udata,
fio_str_info_s name,
fio_str_info_s filename,
fio_str_info_s content_type);
/** Called for each chunk of file data. Return non-zero to abort. */
int (*on_file_data)(void *udata, void *file, fio_buf_info_s data);
/** Called when file upload is complete. */
void (*on_file_done)(void *udata, void *file);
/* ===== Error Handling ===== */
/** Called on parse error. `partial` is the incomplete result, if any. */
void *(*on_error)(void *udata, void *partial);
/** Called to free an unused object (e.g., key when map_set fails). */
void (*free_unused)(void *udata, void *obj);
} fio_http_body_parse_callbacks_s
```

HTTP body parser callbacks.

All callbacks receive `udata` as first parameter.
Primitive callbacks return the created object as `void *`.
Container callbacks return a context for that container.

_Symbol type:_ `type`

#### `fio_http_body_parse_result_s`

```c
typedef struct {
/** Top-level parsed object (caller responsible for freeing). */
void *result;
/** Number of bytes consumed from body. */
size_t consumed;
/** Error code: 0 = success. */
int err;
} fio_http_body_parse_result_s
```

HTTP body parse result.

_Symbol type:_ `type`

### Functions

#### `fio_http_new`

```c
fio_http_s *fio_http_new(void)
```

Create a new fio_http_s handle.

_Symbol type:_ `function`

#### `fio_http_new_copy_request`

```c
fio_http_s *fio_http_new_copy_request(fio_http_s *old)
```

Creates a copy of an existing handle, copying only its request data.

_Symbol type:_ `function`

#### `fio_http_free`

```c
void fio_http_free(fio_http_s *)
```

Reduces an fio_http_s handle's reference count or frees it.

_Symbol type:_ `function`

#### `fio_http_dup`

```c
fio_http_s *fio_http_dup(fio_http_s *)
```

Increases an fio_http_s handle's reference count.

_Symbol type:_ `function`

#### `fio_http_destroy`

```c
fio_http_s *fio_http_destroy(fio_http_s *h)
```

Destroyed the HTTP handle object, freeing all allocated resources.

_Symbol type:_ `function`

#### `fio_http_start_time_set`

```c
void fio_http_start_time_set(fio_http_s *)
```

Collects an updated timestamp for logging purposes.

_Symbol type:_ `function`

#### `fio_http_clear_response`

```c
fio_http_s *fio_http_clear_response(fio_http_s *h, bool clear_body)
```

Clears any response data.

_Symbol type:_ `function`

#### `fio_http_udata`

```c
inline void *fio_http_udata(fio_http_s *)
```

Gets the opaque user pointer associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_udata_set`

```c
inline void *fio_http_udata_set(fio_http_s *, void *)
```

Sets the opaque user pointer associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_udata2`

```c
inline void *fio_http_udata2(fio_http_s *)
```

Gets the second opaque user pointer associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_udata2_set`

```c
inline void *fio_http_udata2_set(fio_http_s *, void *)
```

Sets a second opaque user pointer associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_controller`

```c
inline fio_http_controller_s *fio_http_controller(fio_http_s *h)
```

Gets the HTTP Controller associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_controller_set`

```c
inline fio_http_controller_s *fio_http_controller_set( fio_http_s *h, fio_http_controller_s *controller)
```

Gets the HTTP Controller associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_cdata`

```c
inline void *fio_http_cdata(fio_http_s *h)
```

Returns the existing controller data (`void *` pointer).

_Symbol type:_ `function`

#### `fio_http_cdata_set`

```c
inline void *fio_http_cdata_set(fio_http_s *h, void *cdata)
```

Sets a new controller data (`void *` pointer).

_Symbol type:_ `function`

#### `fio_http_status`

```c
size_t fio_http_status(fio_http_s *)
```

Gets the status associated with the HTTP handle (response).

_Symbol type:_ `function`

#### `fio_http_status_set`

```c
size_t fio_http_status_set(fio_http_s *, size_t status)
```

Sets the status associated with the HTTP handle (response).

_Symbol type:_ `function`

#### `fio_http_method`

```c
fio_str_info_s fio_http_method(fio_http_s *)
```

Gets the method information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_method_set`

```c
fio_str_info_s fio_http_method_set(fio_http_s *, fio_str_info_s)
```

Sets the method information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_opath`

```c
fio_str_info_s fio_http_opath(fio_http_s *)
```

Gets the original / first path associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_opath_set`

```c
fio_str_info_s fio_http_opath_set(fio_http_s *, fio_str_info_s)
```

Sets the original / first path associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_path`

```c
fio_str_info_s fio_http_path(fio_http_s *)
```

Gets the path information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_path_set`

```c
fio_str_info_s fio_http_path_set(fio_http_s *, fio_str_info_s)
```

Sets the path information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_query`

```c
fio_str_info_s fio_http_query(fio_http_s *)
```

Gets the query information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_query_set`

```c
fio_str_info_s fio_http_query_set(fio_http_s *, fio_str_info_s)
```

Sets the query information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_version`

```c
fio_str_info_s fio_http_version(fio_http_s *)
```

Gets the version information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_version_set`

```c
fio_str_info_s fio_http_version_set(fio_http_s *, fio_str_info_s)
```

Sets the version information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_received_at`

```c
int64_t fio_http_received_at(fio_http_s *)
```

Gets the received_at timestamp (ms) associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_received_at_set`

```c
int64_t fio_http_received_at_set(fio_http_s *, int64_t)
```

Sets the received_at timestamp (ms) associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_request_header`

```c
fio_str_info_s fio_http_request_header(fio_http_s *, fio_str_info_s name, size_t index)
```

Gets the header information associated with the HTTP handle.

Since more than a single value may be associated with a header name, the
index may be used to collect subsequent values.

An empty value is returned if no header value is available (or index is
exceeded).

_Symbol type:_ `function`

#### `fio_http_request_header_count`

```c
size_t fio_http_request_header_count(fio_http_s *, fio_str_info_s name)
```

Returns the number of headers named `name` that were received.

If `name` buffer is `NULL`, returns the number of unique headers (not the
number of unique values).

_Symbol type:_ `function`

#### `fio_http_request_header_set`

```c
fio_str_info_s fio_http_request_header_set(fio_http_s *, fio_str_info_s name, fio_str_info_s value)
```

Sets the header information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_request_header_set_if_missing`

```c
fio_str_info_s fio_http_request_header_set_if_missing(fio_http_s *, fio_str_info_s name, fio_str_info_s value)
```

Sets the header information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_request_header_add`

```c
fio_str_info_s fio_http_request_header_add(fio_http_s *, fio_str_info_s name, fio_str_info_s value)
```

Adds to the header information associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_request_header_each`

```c
size_t fio_http_request_header_each(fio_http_s *, int (*callback)(fio_http_s *, fio_str_info_s name, fio_str_info_s value, void *udata), void *udata)
```

Iterates through all request headers (except cookies!).

A non-zero return will stop iteration.

Returns the number of iterations performed. If `callback` is `NULL`, returns
the number of headers available (multi-value headers are counted as 1).

_Symbol type:_ `function`

#### `fio_http_body_length`

```c
size_t fio_http_body_length(fio_http_s *)
```

Gets the body (payload) length associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_body_seek`

```c
size_t fio_http_body_seek(fio_http_s *, ssize_t pos)
```

Adjusts the body's reading position. Negative values start at the end.

If `pos == SSIZE_MAX`, returns `fio_http_body_pos`.

_Symbol type:_ `function`

#### `fio_http_body_pos`

```c
size_t fio_http_body_pos(fio_http_s *h)
```

Returns the body's reading position.

_Symbol type:_ `function`

#### `fio_http_body_read`

```c
fio_str_info_s fio_http_body_read(fio_http_s *, size_t length)
```

Reads up to `length` of data from the body, returns nothing on EOF.

_Symbol type:_ `function`

#### `fio_http_body_read_until`

```c
fio_str_info_s fio_http_body_read_until(fio_http_s *, char token, size_t limit)
```

Reads from the body until finding `token`, reaching `limit` or EOF.

Note: `limit` is ignored if zero or if the remaining data is lower than
limit.

_Symbol type:_ `function`

#### `fio_http_body_expect`

```c
void fio_http_body_expect(fio_http_s *, size_t expected_length)
```

Allocates a body (payload) of (at least) the `expected_length`.

_Symbol type:_ `function`

#### `fio_http_body_write`

```c
void fio_http_body_write(fio_http_s *, const void *data, size_t len)
```

Writes `data` to the body (payload) associated with the HTTP handle.

_Symbol type:_ `function`

#### `fio_http_body_fd`

```c
int fio_http_body_fd(fio_http_s *)
```

If the body is stored in a temporary file, returns the file's handle.

Otherwise returns -1.

_Symbol type:_ `function`

#### `fio_http_cookie_set`

```c
int fio_http_cookie_set(fio_http_s *h, fio_http_cookie_args_s)
```

Sets a response cookie.

Returns -1 on error and 0 on success.

Note: Long cookie names and long cookie values will be considered a security
violation and an error will be returned. Many browsers and proxies impose
limits on headers and cookies, cookies often limited to 4Kb in total for both
name and value.

_Symbol type:_ `function`

#### `fio_http_cookie_set`

```c
#define fio_http_cookie_set(http___handle, ...)   \
  fio_http_cookie_set((http___handle), (fio_http_cookie_args_s){__VA_ARGS__})
```

Named arguments helper. See fio_http_cookie_args_s for details.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_http_cookie`

```c
fio_str_info_s fio_http_cookie(fio_http_s *, const char *name, size_t name_len)
```

Returns a cookie value (either received of newly set), if any.

_Symbol type:_ `function`

#### `fio_http_cookie_each`

```c
size_t fio_http_cookie_each(fio_http_s *, int (*callback)(fio_http_s *, fio_str_info_s name, fio_str_info_s value, void *udata), void *udata)
```

Iterates through all cookies. A non-zero return will stop iteration.

_Symbol type:_ `function`

#### `fio_http_set_cookie_each`

```c
size_t fio_http_set_cookie_each(fio_http_s *h, int (*callback)(fio_http_s *, fio_str_info_s set_cookie_header, fio_str_info_s value, void *udata), void *udata)
```

Iterates through all response set cookies.

A non-zero return value from the callback will stop iteration.

_Symbol type:_ `function`

#### `fio_http_is_clean`

```c
int fio_http_is_clean(fio_http_s *)
```

Returns true if no HTTP headers / data was sent (a clean slate).

_Symbol type:_ `function`

#### `fio_http_is_finished`

```c
int fio_http_is_finished(fio_http_s *)
```

Returns true if the HTTP handle's response was sent.

_Symbol type:_ `function`

#### `fio_http_is_streaming`

```c
int fio_http_is_streaming(fio_http_s *)
```

Returns true if the HTTP handle's response is streaming.

_Symbol type:_ `function`

#### `fio_http_is_upgraded`

```c
int fio_http_is_upgraded(fio_http_s *h)
```

Returns true if the HTTP connection was (or should have been) upgraded.

_Symbol type:_ `function`

#### `fio_http_is_websocket`

```c
int fio_http_is_websocket(fio_http_s *)
```

Returns true if the HTTP handle refers to a WebSocket connection.

_Symbol type:_ `function`

#### `fio_http_is_sse`

```c
int fio_http_is_sse(fio_http_s *)
```

Returns true if the HTTP handle refers to an EventSource connection.

_Symbol type:_ `function`

#### `fio_http_is_freeing`

```c
int fio_http_is_freeing(fio_http_s *)
```

Returns true if handle is in the process of freeing itself.

_Symbol type:_ `function`

#### `fio_http_response_header`

```c
fio_str_info_s fio_http_response_header(fio_http_s *, fio_str_info_s name, size_t index)
```

Gets the header information associated with the HTTP handle.

Since more than a single value may be associated with a header name, the
index may be used to collect subsequent values.

An empty value is returned if no header value is available (or index is
exceeded).

If the response headers were already sent, the returned value is always
empty.

_Symbol type:_ `function`

#### `fio_http_response_header_count`

```c
size_t fio_http_response_header_count(fio_http_s *, fio_str_info_s name)
```

Returns the number of headers named `name` in the response.

If `name` buffer is `NULL`, returns the number of unique headers (not the
number of unique values).

_Symbol type:_ `function`

#### `fio_http_response_header_set`

```c
fio_str_info_s fio_http_response_header_set(fio_http_s *, fio_str_info_s name, fio_str_info_s value)
```

Sets the header information associated with the HTTP handle.

If the response headers were already sent, the returned value is always
empty.

_Symbol type:_ `function`

#### `fio_http_response_header_set_if_missing`

```c
fio_str_info_s fio_http_response_header_set_if_missing(fio_http_s *, fio_str_info_s name, fio_str_info_s value)
```

Sets the header information associated with the HTTP handle.

If the response headers were already sent, the returned value is always
empty.

_Symbol type:_ `function`

#### `fio_http_response_header_add`

```c
fio_str_info_s fio_http_response_header_add(fio_http_s *, fio_str_info_s name, fio_str_info_s value)
```

Adds to the header information associated with the HTTP handle.

If the response headers were already sent, the returned value is always
empty.

_Symbol type:_ `function`

#### `fio_http_response_header_each`

```c
size_t fio_http_response_header_each(fio_http_s *, int (*callback)(fio_http_s *, fio_str_info_s name, fio_str_info_s value, void *udata), void *udata)
```

Iterates through all response headers (except cookies!).

A non-zero return will stop iteration.

_Symbol type:_ `function`

#### `fio_http_write`

```c
void fio_http_write(fio_http_s *, fio_http_write_args_s args)
```

Writes `data` to the response body associated with the HTTP handle after
sending all headers (no further headers may be sent).

_Symbol type:_ `function`

#### `fio_http_write`

```c
#define fio_http_write(http_handle, ...)   \
  fio_http_write(http_handle, (fio_http_write_args_s){__VA_ARGS__})
```

Named arguments helper. See fio_http_write and fio_http_write_args_s.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_http_finish`

```c
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)
```



_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_http_close`

```c
void fio_http_close(fio_http_s *h)
```

Closes a persistent HTTP connection (i.e., if upgraded).

_Symbol type:_ `function`

#### `fio_http_websocket_requested`

```c
int fio_http_websocket_requested(fio_http_s *)
```

Returns non-zero if request headers ask for a WebSockets Upgrade.

_Symbol type:_ `function`

#### `fio_http_websocket_accepted`

```c
int fio_http_websocket_accepted(fio_http_s *h)
```

Returns non-zero if the response accepts a WebSocket upgrade request.

_Symbol type:_ `function`

#### `fio_http_upgrade_websocket`

```c
void fio_http_upgrade_websocket(fio_http_s *)
```

Sets response data to agree to a WebSockets Upgrade.

_Symbol type:_ `function`

#### `fio_http_websocket_set_request`

```c
void fio_http_websocket_set_request(fio_http_s *)
```

Sets request data to request a WebSockets Upgrade.

_Symbol type:_ `function`

#### `fio_http_sse_requested`

```c
int fio_http_sse_requested(fio_http_s *)
```

Returns non-zero if request headers ask for an EventSource (SSE) Upgrade.

_Symbol type:_ `function`

#### `fio_http_sse_accepted`

```c
int fio_http_sse_accepted(fio_http_s *h)
```

Returns non-zero if the response accepts an SSE request.

_Symbol type:_ `function`

#### `fio_http_upgrade_sse`

```c
void fio_http_upgrade_sse(fio_http_s *)
```

Sets response data to agree to an EventSource (SSE) Upgrade.

_Symbol type:_ `function`

#### `fio_http_sse_set_request`

```c
void fio_http_sse_set_request(fio_http_s *)
```

Sets request data to request an EventSource (SSE) Upgrade.

_Symbol type:_ `function`

#### `fio_http_mimetype_register`

```c
int fio_http_mimetype_register(char *file_ext, size_t file_ext_len, fio_str_info_s mime_type)
```

Registers a Mime-Type to be associated with the file extension.

_Symbol type:_ `function`

#### `fio_http_mimetype`

```c
fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len)
```

Finds the Mime-Type associated with the file extension (if registered).

_Symbol type:_ `function`

#### `fio_http_body_parse`

```c
fio_http_body_parse_result_s fio_http_body_parse(fio_http_s *h, const fio_http_body_parse_callbacks_s *callbacks, void *udata)
```

Parses the HTTP request body, auto-detecting content type.

Supports JSON, URL-encoded, and multipart/form-data bodies.
Calls the appropriate callbacks for each element found.

**Parameters:**
- `h` - The HTTP handle.
- `callbacks` - Parser callbacks (designed to be static const).
- `udata` - User context passed to all callbacks.

**Returns:**
- Parse result with top-level object and status.

_Symbol type:_ `function`

#### `fio_http_response_header_parse`

```c
int fio_http_response_header_parse(fio_http_s *h, fio_str_info_s *buf_parsed, fio_str_info_s header_name)
```

Copies all header data, from possibly an array of identical response headers,
resulting in a parsed format outputted to `buf_parsed`.

Returns 0 on success or -1 on error (i.e., `buf_parsed.capa` wasn't enough
for the parsed output).

Note that the parsed output isn't readable as a string, but is designed to
work with the `FIO_HTTP_PARSED_HEADER_EACH` and
`FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` property.

See also `fio_http_response_header_parse`.

_Symbol type:_ `function`

#### `fio_http_request_header_parse`

```c
int fio_http_request_header_parse(fio_http_s *h, fio_str_info_s *buf_parsed, fio_str_info_s header_name)
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

_Symbol type:_ `function`

#### `fio_http_send_error_response`

```c
int fio_http_send_error_response(fio_http_s *h, size_t status)
```

Sends the requested error message and finishes the response.

_Symbol type:_ `function`

#### `fio_http_etag_is_match`

```c
int fio_http_etag_is_match(fio_http_s *h)
```

Returns true (1) if the ETag response matches an if-none-match request.

_Symbol type:_ `function`

#### `fio_http_static_file_response`

```c
int fio_http_static_file_response(fio_http_s *h, fio_str_info_s root_folder, fio_str_info_s file_name, size_t max_age)
```

Attempts to send a static file from the `root` folder. On success the
response is complete and 0 is returned. Otherwise returns -1.

_Symbol type:_ `function`

#### `fio_http_status2str`

```c
fio_str_info_s fio_http_status2str(size_t status)
```

Returns a human readable string related to the HTTP status number.

_Symbol type:_ `function`

#### `fio_http_write_log`

```c
void fio_http_write_log(fio_http_s *h)
```

Logs an HTTP (response) to STDOUT.

_Symbol type:_ `function`

#### `fio_http_from`

```c
int fio_http_from(fio_str_info_s *dest, const fio_http_s *h)
```

Writes peer address to `dest` starting with the `forwarded` header, with a
fallback to actual socket address and a final fallback to `"[unknown]"`.

If `unknown` is returned, the function returns -1. if `dest` capacity is too
small, the number of bytes required will be returned.

If all goes well, this function returns 0.

_Symbol type:_ `function`

#### `fio_http_date`

```c
fio_str_info_s fio_http_date(uint64_t now_in_seconds)
```



_Symbol type:_ `function`

#### `fio_http_log_time`

```c
fio_str_info_s fio_http_log_time(uint64_t now_in_seconds)
```



_Symbol type:_ `function`

-----------------------------------------------------

## <a id="fio-stl-431-http1-parser-h"></a> `./fio-stl/431 http1 parser.h`

3 public symbols.

### Macros

#### `FIO_HTTP1_PARSER_INIT`

```c
#define FIO_HTTP1_PARSER_INIT ((fio_http1_parser_s){0})
```

Initialization value for the parser

_Symbol type:_ `macro`

#### `FIO_HTTP1_PARSER_ERROR`

```c
#define FIO_HTTP1_PARSER_ERROR ((size_t)-1)
```

The error return value for fio_http1_parse.

_Symbol type:_ `macro`

#### `FIO_HTTP1_EXPECTED_CHUNKED`

```c
#define FIO_HTTP1_EXPECTED_CHUNKED ((size_t)(-2))
```

A return value for `fio_http1_expected` when chunked data is expected.

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-431-websocket-parser-h"></a> `./fio-stl/431 websocket parser.h`

29 public symbols.

### Macros

#### `FIO_WEBSOCKET_DEFAULT_MAX_FRAME`

```c
#define FIO_WEBSOCKET_DEFAULT_MAX_FRAME (1ULL << 30)
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_PARSE_ERROR`

```c
#define FIO_WEBSOCKET_PARSE_ERROR ((size_t)-1)
```

Parse error sentinel.

_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_RSV1`

```c
#define FIO_WEBSOCKET_RSV1 0x4U /* byte-0 bit 6 (permessage-deflate) */
```

RSV bit constants for the send-side API. These are the 3-bit values shifted
 into byte-0 bits 4..6 on the wire (0x4 → RSV1=0x40, 0x2 → RSV2=0x20, 0x1 →
 RSV3=0x10).

_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_RSV2`

```c
#define FIO_WEBSOCKET_RSV2 0x2U /* byte-0 bit 5 */
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_RSV3`

```c
#define FIO_WEBSOCKET_RSV3 0x1U /* byte-0 bit 4 */
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_OP_CONT`

```c
#define FIO_WEBSOCKET_OP_CONT   0x0
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_OP_TEXT`

```c
#define FIO_WEBSOCKET_OP_TEXT   0x1
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_OP_BINARY`

```c
#define FIO_WEBSOCKET_OP_BINARY 0x2
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_OP_CLOSE`

```c
#define FIO_WEBSOCKET_OP_CLOSE  0x8
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_OP_PING`

```c
#define FIO_WEBSOCKET_OP_PING   0x9
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_OP_PONG`

```c
#define FIO_WEBSOCKET_OP_PONG   0xA
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_STATE_HEADER`

```c
#define FIO_WEBSOCKET_STATE_HEADER  0
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_STATE_PAYLOAD`

```c
#define FIO_WEBSOCKET_STATE_PAYLOAD 1
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_STATE_CLOSED`

```c
#define FIO_WEBSOCKET_STATE_CLOSED  0xFE
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_STATE_ERROR`

```c
#define FIO_WEBSOCKET_STATE_ERROR   0xFF
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_FLAG_FIN`

```c
#define FIO_WEBSOCKET_FLAG_FIN             0x80
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_FLAG_MASKED`

```c
#define FIO_WEBSOCKET_FLAG_MASKED          0x40
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_FLAG_OPCODE_MASK`

```c
#define FIO_WEBSOCKET_FLAG_OPCODE_MASK     0x3C
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_FLAG_OPCODE_SHIFT`

```c
#define FIO_WEBSOCKET_FLAG_OPCODE_SHIFT    2
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_FLAG_MSG_OPCODE_MASK`

```c
#define FIO_WEBSOCKET_FLAG_MSG_OPCODE_MASK 0x03
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_FLAG2_PAUSED`

```c
#define FIO_WEBSOCKET_FLAG2_PAUSED        0x80
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_FLAG2_MSG_RSV_MASK`

```c
#define FIO_WEBSOCKET_FLAG2_MSG_RSV_MASK  0x70
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_FLAG2_MSG_RSV_SHIFT`

```c
#define FIO_WEBSOCKET_FLAG2_MSG_RSV_SHIFT 4
```



_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_GET_FIN`

```c
#define FIO_WEBSOCKET_GET_FIN(p) (((p)->flags & FIO_WEBSOCKET_FLAG_FIN) != 0)
```

Read the FIN bit from the current frame.

_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_GET_MASKED`

```c
#define FIO_WEBSOCKET_GET_MASKED(p)   \
  (((p)->flags & FIO_WEBSOCKET_FLAG_MASKED) != 0)
```

Read the MASK bit from the current frame.

_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_GET_OPCODE`

```c
#define FIO_WEBSOCKET_GET_OPCODE(p)   \
  (((p)->flags & FIO_WEBSOCKET_FLAG_OPCODE_MASK) >>   \
   FIO_WEBSOCKET_FLAG_OPCODE_SHIFT)
```

Read the opcode from the current frame.

_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_GET_MSG_OPCODE`

```c
#define FIO_WEBSOCKET_GET_MSG_OPCODE(p)   \
  ((p)->flags & FIO_WEBSOCKET_FLAG_MSG_OPCODE_MASK)
```

Read the message opcode (1=text, 2=binary, 0=none open).

_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_GET_PAUSED`

```c
#define FIO_WEBSOCKET_GET_PAUSED(p)   \
  (((p)->flags2 & FIO_WEBSOCKET_FLAG2_PAUSED) != 0)
```

Read the paused flag (one-message-per-parse gate).

_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_GET_MSG_RSV`

```c
#define FIO_WEBSOCKET_GET_MSG_RSV(p)   \
  (((p)->flags2 & FIO_WEBSOCKET_FLAG2_MSG_RSV_MASK) >>   \
   FIO_WEBSOCKET_FLAG2_MSG_RSV_SHIFT)
```

Read the RSV bits from the opening frame (3-bit format).

_Symbol type:_ `macro`

-----------------------------------------------------

## <a id="fio-stl-439-http-h"></a> `./fio-stl/439 http.h`

30 public symbols.

### Macros

#### `FIO_HTTP_DEFAULT_MAX_HEADER_SIZE`

```c
#define FIO_HTTP_DEFAULT_MAX_HEADER_SIZE 32768 /* (1UL << 15) */
```

The default HTTP total header size limit in bytes.

_Symbol type:_ `macro`

#### `FIO_HTTP_DEFAULT_MAX_LINE_LEN`

```c
#define FIO_HTTP_DEFAULT_MAX_LINE_LEN 8192 /* (1UL << 13) */
```

The default HTTP header line limit in bytes.

_Symbol type:_ `macro`

#### `FIO_HTTP_DEFAULT_MAX_BODY_SIZE`

```c
#define FIO_HTTP_DEFAULT_MAX_BODY_SIZE 33554432 /* (1UL << 25) */
```

The default HTTP payload size limit in bytes.

_Symbol type:_ `macro`

#### `FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE`

```c
#define FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE 262144 /* (1UL << 18) */
```

The default WebSocket message size limit in bytes.

_Symbol type:_ `macro`

#### `FIO_HTTP_DEFAULT_TIMEOUT`

```c
#define FIO_HTTP_DEFAULT_TIMEOUT 50
```

The default timeout for HTTP connections.

_Symbol type:_ `macro`

#### `FIO_HTTP_DEFAULT_TIMEOUT_LONG`

```c
#define FIO_HTTP_DEFAULT_TIMEOUT_LONG 50
```

The default timeout for long held HTTP connections (WebSockets / SSE).

_Symbol type:_ `macro`

#### `FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER`

```c
#define FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER 0
```

Adds a "content-length" header to the HTTP handle (usually redundant).

_Symbol type:_ `macro`

#### `FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT`

```c
#define FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT ((1UL << 16) - 10UL)
```

UTF-8 validity tests will be performed only for data shorter than this.

_Symbol type:_ `macro`

#### `FIO_WEBSOCKET_STATS`

```c
#define FIO_WEBSOCKET_STATS 0
```

If true, logs longest WebSocket round-trips (using FIO_LOG_INFO).

_Symbol type:_ `macro`

#### `FIO_HTTP_WEBSOCKET_DEFLATE_MIN`

```c
#define FIO_HTTP_WEBSOCKET_DEFLATE_MIN 1024
```

Messages smaller than this are not compressed (fits in a single TCP/IP
packet, compression saves no network overhead).

_Symbol type:_ `macro`

### Types

#### `fio_http_settings_s`

```c
struct fio_http_settings_s {
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
/** Opt-in: auto-compress static files (save .br/.gz to disk). */
uint8_t compress_static;
/** Opt-in: auto-compress dynamic HTTP responses on-the-fly. */
uint8_t compress_dynamic;
/** Opt-in: enable permessage-deflate for WebSocket connections. */
uint8_t compress_ws;
}
```



_Symbol type:_ `type`

#### `fio_http_listener_s`

```c
struct fio_http_listener_s
```



_Symbol type:_ `type`

#### `fio_http_resource_action_e`

```c
typedef enum {
FIO_HTTP_RESOURCE_NONE,
FIO_HTTP_RESOURCE_INDEX,
FIO_HTTP_RESOURCE_SHOW,
FIO_HTTP_RESOURCE_NEW,
FIO_HTTP_RESOURCE_EDIT,
FIO_HTTP_RESOURCE_CREATE,
FIO_HTTP_RESOURCE_UPDATE,
FIO_HTTP_RESOURCE_DELETE,
} fio_http_resource_action_e
```



_Symbol type:_ `type`

#### `fio_http_sse_write_args_s`

```c
typedef struct {
/** The message's `id` data (if any). */
fio_buf_info_s id;
/** The message's `event` data (if any). */
fio_buf_info_s event;
/** The message's `data` data (if any). */
fio_buf_info_s data;
} fio_http_sse_write_args_s
```

Named arguments for fio_http_sse_write.

_Symbol type:_ `type`

### Functions

#### `fio_http_listen`

```c
fio_http_listener_s *fio_http_listen(const char *url, fio_http_settings_s settings)
```

Listens to HTTP / WebSockets / SSE connections on `url`.

_Symbol type:_ `function`

#### `fio_http_listen`

```c
#define fio_http_listen(url, ...)   \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})
```

Listens to HTTP / WebSockets / SSE connections on `url`.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_http_listener_settings`

```c
fio_http_settings_s *fio_http_listener_settings(fio_http_listener_s *l)
```

Returns the a pointer to the HTTP settings associated with the listener.

_Symbol type:_ `function`

#### `fio_http_io`

```c
fio_io_s *fio_http_io(fio_http_s *)
```

Returns the IO object associated with the HTTP object (request only).

_Symbol type:_ `function`

#### `fio_http_subscribe`

```c
#define fio_http_subscribe(h, ...)   \
  fio_pubsub_subscribe(.io = fio_http_io(h), __VA_ARGS__)
```

Macro helper for HTTP handle pub/sub subscriptions.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_http_connect`

```c
fio_io_s *fio_http_connect(const char *url, fio_http_s *h, fio_http_settings_s settings)
```

Connects to HTTP / WebSockets / SSE connections on `url`.

_Symbol type:_ `function`

#### `fio_http_connect`

```c
#define fio_http_connect(url, h, ...)   \
  fio_http_connect(url, h, (fio_http_settings_s){__VA_ARGS__})
```

Connects to HTTP / WebSockets / SSE connections on `url`.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_http_settings`

```c
fio_http_settings_s *fio_http_settings(fio_http_s *)
```

Returns the HTTP settings associated with the HTTP object, if any.

_Symbol type:_ `function`

#### `fio_http_route`

```c
int fio_http_route(fio_http_listener_s *listener, const char *url, fio_http_settings_s settings)
```

Adds a route prefix to the HTTP handler.

Order of route settings is irrelevant (unless overwriting an existing route).

Matching is performed as a best-prefix match. i.e.:

- All paths match the route `"/"` (the default prefix).

- The route `"/user"` will match `"/user"` and all `"/user/..."` paths but
  not `"/user..."`

- Setting `"/user/new"` as well as `"/user"` (in whatever order) will route
  `"/user/new"` and `"/user/new/..."` to `"/user/new"`. Otherwise, the
  `"/user"` route will continue to behave the same.

Note: the `udata`, `on_finish`, `public_folder` and `log` properties are all
inherited (if missing) from the default HTTP settings used to create the
listener.

Note: TLS options are ignored.

_Symbol type:_ `function`

#### `fio_http_route`

```c
#define fio_http_route(listener, url, ...)   \
  fio_http_route(listener, url, (fio_http_settings_s){__VA_ARGS__})
```

Adds a route prefix to the HTTP handler.

Order of route settings is irrelevant (unless overwriting an existing route).

Matching is performed as a best-prefix match. i.e.:

- All paths match the route `"/"` (the default prefix).

- The route `"/user"` will match `"/user"` and all `"/user/..."` paths but
  not `"/user..."`

- Setting `"/user/new"` as well as `"/user"` (in whatever order) will route
  `"/user/new"` and `"/user/new/..."` to `"/user/new"`. Otherwise, the
  `"/user"` route will continue to behave the same.

Note: the `udata`, `on_finish`, `public_folder` and `log` properties are all
inherited (if missing) from the default HTTP settings used to create the
listener.

Note: TLS options are ignored.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

#### `fio_http_route_settings`

```c
fio_http_settings_s *fio_http_route_settings(fio_http_listener_s *l, const char *url)
```

Returns a link to the settings matching `url`, as set by `fio_http_route`

_Symbol type:_ `function`

#### `fio_http_resource_action`

```c
inline fio_http_resource_action_e fio_http_resource_action(fio_http_s *h)
```

returns expected action or `FIO_HTTP_RESOURCE_NONE` on error.

_Symbol type:_ `function`

#### `fio_http_websocket_write`

```c
int fio_http_websocket_write(fio_http_s *h, const void *buf, size_t len, uint8_t is_text)
```

Writes a WebSocket message. Fails if connection wasn't upgraded yet.

_Symbol type:_ `function`

#### `fio_http_on_message_set`

```c
int fio_http_on_message_set(fio_http_s *h, void (*on_message)(fio_http_s *, fio_buf_info_s, uint8_t))
```

Sets a specific on_message callback for this connection.

Returns -1 on error (i.e., upgrade still in negotiation).

_Symbol type:_ `function`

#### `fio_http_sse_write`

```c
int fio_http_sse_write(fio_http_s *h, fio_http_sse_write_args_s args)
```

Writes an SSE message (UTF-8). Fails if connection wasn't upgraded yet.

_Symbol type:_ `function`

#### `fio_http_sse_write`

```c
#define fio_http_sse_write(h, ...)   \
  fio_http_sse_write((h), ((fio_http_sse_write_args_s){__VA_ARGS__}))
```

Writes an SSE message (UTF-8). Fails if connection wasn't upgraded yet.

_Note:_ this may be a macro only / macro wrapper for a function.

_Symbol type:_ `macro`

-----------------------------------------------------
