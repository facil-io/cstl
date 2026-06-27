# Compiler Attributes

The macros in [`./000 core.h`](./000%20core.h) and [`./001 header.h`](./001%20header.h) paper over differences between C and C++, compilers, and inclusion models. This page is a quick reference for that portability scaffolding.

## C++ compatibility

Both core slices open an `extern "C"` block when compiled as C++. [`./000 core.h`](./000%20core.h) also neutralizes C-only keywords inside that block:

- `register` is defined away (deprecated in C++).
- `restrict` is defined away (not valid in C++).
- `_Bool` is mapped to `bool`.

No action is required; including the headers gives C++ code a C-compatible surface.

## Keyword shims

Some compilers need a little help with C99/C11 keywords:

- **`inline`** — mapped to `__inline` when the compiler does not support the C99 keyword directly (mainly older MSVC).
- **`__thread`** — mapped to `__declspec(thread)` on MSVC and `_Thread_local` on C11 compilers; otherwise left as `__thread` when the compiler supports it.

## Compiler detection and no-op macros

When the compiler is not GCC or Clang, `__attribute__`, `__has_include`, `__has_builtin`, and `__has_attribute` are defined as no-ops so the rest of the code can use them unconditionally.

- **`FIO_NOOP`** — empty macro that adds whitespace. Useful to stop a function-like macro from being expanded.
- **`FIO_NOOP_FN(...)`** — empty variadic macro, handy as a default callback.
- **`FIO_NOOP_FN_NAME`** — expands to `(void)`.

## OS detection

Defined in [`./000 core.h`](./000%20core.h); see [`./001 os detection.md`](./001%20os%20detection.md) for full details.

- **`FIO_OS_POSIX`** — 1 on POSIX systems.
- **`FIO_OS_WIN`** — 1 on Windows.
- **`FIO_HAVE_UNIX_TOOLS`** — 0 (none), 1 (POSIX), 2 (MinGW), or 3 (Cygwin).

## Function attributes

Defined in [`./000 core.h`](./000%20core.h) and used throughout the library.

- **`FIO_MAYBE_UNUSED`** — `__attribute__((unused))` on GCC/Clang, empty otherwise.
- **`FIO_SFUNC`** — `static FIO_MAYBE_UNUSED`. Default internal linkage.
- **`FIO_IFUNC`** — `FIO_SFUNC inline`. Static inline helper.
- **`FIO_WARN_UNUSED`** — `__attribute__((warn_unused_result))` or empty.
- **`FIO_MIFN`** — `FIO_IFUNC FIO_WARN_UNUSED`. Inline math-style function that warns if the return value is ignored.
- **`FIO_CONST`** — function result depends only on arguments; no globals or side effects.
- **`FIO_PURE`** — no side effects, but may read global memory.
- **`FIO_WEAK`** — weak symbol. Empty on MinGW.
- **`DEPRECATED(reason)`** — `__attribute__((deprecated(reason)))`, with a fallback for GCC older than 4.5.
- **`FIO_CONSTRUCTOR(fname)`** — runs before `main` on GCC/Clang; on MSVC it places a pointer in `.CRT$XCU`.
- **`FIO_DESTRUCTOR(fname)`** — runs after `main` on GCC/Clang; on MSVC it is implemented as a constructor that registers `atexit(fname)`.
- **`FIO_LIKELY(cond)`** / **`FIO_UNLIKELY(cond)`** — branch-prediction hints. No-op on unknown compilers.
- **`FIO_PREFETCH(ptr)`** / **`FIO_PREFETCH_W(ptr)`** / **`FIO_PREFETCH_NT(ptr)`** — cache prefetch hints for read, write-exclusive, and non-temporal streaming access.
- **`FIO___PRINTF_STYLE(string_index, check_index)`** — tells the compiler to check `printf`-style format arguments. Empty on pure MSVC; uses the MinGW/Cygwin format attribute on those compilers; otherwise uses the GCC/Clang `printf` format attribute.

## Variable and type attributes

Defined in [`./000 core.h`](./000%20core.h).

- **`FIO_ALIGN(bytes)`** — alignment hint. Uses `__attribute__((aligned(bytes)))` on GCC/Clang; currently empty on other compilers.
- **`FIO_WEAK_VAR`** — weak global variable. Uses `__declspec(selectany)` on Windows when selectany support is detected (MSVC or otherwise); `__attribute__((weak))` elsewhere.

## Macro stringifier

Defined in [`./000 core.h`](./000%20core.h).

- **`FIO_MACRO2STR(macro)`** — expands `macro` and stringifies the result. Uses a two-step indirection so tokens like `__LINE__` are expanded first.

## Static assertions

Defined in [`./000 core.h`](./000%20core.h).

- **`FIO_ASSERT_STATIC(cond, msg)`** — compile-time assertion. Uses C11 `_Static_assert` when available; otherwise falls back to a negative-array-size trick that produces a compile error on failure.

## Naming helpers

Defined in [`./000 core.h`](./000%20core.h).

- **`FIO_NAME(prefix, postfix)`** → `prefix_postfix`.
- **`FIO_NAME2(prefix, postfix)`** → `prefix2postfix` (for conversion functions).
- **`FIO_NAME_BL(prefix, postfix)`** → `prefix_is_postfix` (for boolean tests).
- **`FIO_NAME_TEST(prefix, postfix)`** → internal test-function name (`fio___test_prefix_postfix`).

## Pointer math

All defined in [`./000 core.h`](./000%20core.h).

- **`FIO_PTR_MATH_LMASK(T, ptr, bits)`** — keep the low `bits` of an address.
- **`FIO_PTR_MATH_RMASK(T, ptr, bits)`** — keep the high bits, zero the low `bits`.
- **`FIO_PTR_MATH_ADD(T, ptr, offset)`** — add `offset` bytes and cast to `T *`.
- **`FIO_PTR_MATH_SUB(T, ptr, offset)`** — subtract `offset` bytes and cast to `T *`.
- **`FIO_PTR_FIELD_OFFSET(T, field)`** — byte offset of `field` inside `T`. Uses `0xFF00` as a base address to keep AddressSanitizer happy.
- **`FIO_PTR_FROM_FIELD(T, field, ptr)`** — recover the containing `T` object from a pointer to one of its fields.

## Pointer tagging

Defined in [`./001 header.h`](./001%20header.h); see [`./001 version and settings.md`](./001%20version%20and%20settings.md) for full details.

- **`FIO_PTR_TAG(p)`** / **`FIO_PTR_UNTAG(p)`** — default no-ops; override to embed tags in pointers.
- **`FIO_PTR_TAG_TYPE`** — when defined, pointer-returning functions use this type instead.
- **`FIO_PTR_TAG_VALIDATE(ptr)`** — validates a tagged pointer; default accepts non-NULL.
- **`FIO_PTR_TAG_VALID_OR_RETURN(tagged_ptr, value)`** — return `value` if validation fails.
- **`FIO_PTR_TAG_VALID_OR_RETURN_VOID(tagged_ptr)`** — return void if validation fails.
- **`FIO_PTR_TAG_VALID_OR_GOTO(tagged_ptr, label)`** — jump to `label` if validation fails.
- **`FIO_PTR_TAG_GET_UNTAGGED(untagged_type, tagged_ptr)`** — cast `FIO_PTR_UNTAG(tagged_ptr)` to `untagged_type *`.

## Get/set helpers

Generate typed getters and setters for struct fields.

- **`FIO_DEF_GETSET_FUNC(static, namespace, T_type, F_type, F_name, on_set)`** — defines both `namespace_F_name` (getter) and `namespace_F_name_set` (setter).
- **`FIO_DEF_GETSET_FUNC_DEC(static, namespace, T_type, F_type, F_name)`** — declarations only.
- **`FIO_IFUNC_DEF_GETSET(namespace, T_type, F_type, F_name, on_set)`** — same, but uses `FIO_IFUNC` as the linkage. `FIO_IFUNC_DEF_GET` and `FIO_IFUNC_DEF_SET` exist for the individual pieces.

The setter returns the old value and calls `on_set(o)` after assignment.

## Recursive inclusion and linkage

facil.io headers are designed to be included more than once to generate code. [`./001 header.h`](./001%20header.h) manages whether generated functions are `static` or `extern`.

- **`SFUNC_`** / **`IFUNC_`** — internal linkage selectors. On the first include they become `FIO_SFUNC`/`FIO_IFUNC` unless `FIO_EXTERN` is defined, in which case they are empty.
- **`SFUNC`** / **`IFUNC`** — public aliases for the current linkage selectors.
- **`FIO_EXTERN`** — define before including a module to emit non-static declarations and inline functions.
- **`FIO___RECURSIVE_INCLUDE`** — internal flag. Setting it to `99` before a recursive `#include` preserves the original `SFUNC`/`IFUNC` linkage instead of forcing it back to `static`.

## Compiler memory barriers

- **`FIO_COMPILER_GUARD`** — clobber memory and prevent the compiler from reordering instructions around it.
- **`FIO_COMPILER_GUARD_INSTRUCTION`** — prevent instruction reordering without the full memory clobber.

On GCC/Clang these are inline `asm volatile` barriers; on MSVC they map to `_ReadWriteBarrier` and `_WriteBarrier`.

## Thread scheduling

Defined in [`./000 core.h`](./000%20core.h); see [`./001 atomics and locks.md`](./001%20atomics%20and%20locks.md) for full details.

- **`FIO_THREAD_WAIT(nano_sec)`** — sleep for about `nano_sec` nanoseconds (`Sleep` on Windows, `nanosleep` on POSIX).
- **`FIO_THREAD_YIELD()`** — yield the CPU (`pause`/`yield` instruction or `sched_yield`).
- **`FIO_THREAD_RESCHEDULE()`** — short sleep via `FIO_THREAD_WAIT(4)`.

## Address-sanitizer scaffolding

- **`FIO___ASAN_DETECTED`** — set when AddressSanitizer is active.
- **`FIO___ASAN_AVOID`** — expands to `no_sanitize_address` when ASan is detected; otherwise empty.

## Loop helpers

- **`FIO_FOR(i, count)`** — `for (size_t i = 0; i < (count); ++i)`.
- **`FIO_FOR_UNROLL(iterations, size_of_loop, i, action)`** — splits a loop into a small remainder and 256-byte chunks to nudge the compiler toward auto-vectorization. Used heavily by the memory primitives and vector-math templates. The SIMD file covers the portable x86 intrinsics that build on these helpers.
