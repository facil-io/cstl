# Memory Alternatives and OS Patches

Defines portable fallback memory routines and small OS compatibility patches. Implemented in [`./001 memalt.h`](./001%20memalt.h) and [`./001 patches.h`](./001%20patches.h); wired into the selector macros in [`./000 core.h`](./000%20core.h).

## When `FIO_MEMALT` is used

Define `FIO_MEMALT` before including the STL to substitute standard-library memory and string calls with facil.io fallbacks:

```c
#define FIO_MEMALT
#include "fio-stl/include.h"
```

This is useful for freestanding or restricted environments, or when you want every memory call to go through the same portable implementation. The fallbacks are correct but usually slower than a good libc, so the default is to use compiler builtins or libc instead.

## Selector wiring

When `FIO_MEMALT` is defined, [`./000 core.h`](./000%20core.h) routes any unset selector to the matching fallback:

| Selector | Fallback |
|----------|----------|
| `FIO_MEMCPY`  | `fio_memcpy` |
| `FIO_MEMMOVE` | `fio_memcpy` |
| `FIO_MEMCMP`  | `fio_memcmp` |
| `FIO_MEMCHR`  | `fio_memchr` |
| `FIO_MEMSET`  | `fio_memset` |
| `FIO_STRLEN`  | `fio_strlen` |

If `FIO_MEMALT` is not defined, the selectors default to `__builtin_*` when available, otherwise to the standard `memcpy`, `memset`, `memchr`, `memcmp`, and `strlen`. You can also override any selector directly without enabling `FIO_MEMALT`.

## Fallback memory routines

All routines are declared in [`./001 memalt.h`](./001%20memalt.h).

### `fio_memcpy`

```c
void *fio_memcpy(void *dest, const void *src, size_t bytes);
```

Copies `bytes` from `src` to `dest`. Handles overlapping regions like `memmove`. Returns `dest` immediately if `dest == src`, `bytes` is zero, or either pointer is `NULL`.

For small, non-overlapping copies (≤64 bytes) it uses overlapping fixed-width copies. For larger or overlapping regions it uses a 64-byte buffered copy, choosing forward or backward direction as needed.

### `fio_memset`

```c
void *fio_memset(void *restrict dest, uint64_t data, size_t bytes);
```

Fills `bytes` at `dest` with the 8-byte pattern `data`. If `data` is below `0x100`, it is broadcast to every byte to match classic `memset` behavior. Returns `dest`, or `NULL` if `dest` is `NULL`.

### `fio_memchr`

```c
void *fio_memchr(const void *buffer, const char token, size_t len);
```

Finds the first byte equal to `token` in the first `len` bytes of `buffer`. Returns a pointer to the match, or `NULL` if not found, `buffer` is `NULL`, or `len` is zero.

Picks the best available implementation at compile time:

- AVX2 on x86-64 with intrinsics
- SSE2 on x86 with intrinsics
- NEON on ARM with intrinsics
- Scalar 128-bit block fallback otherwise

### `fio_memcmp`

```c
int fio_memcmp(const void *a, const void *b, size_t len);
```

Compares the first `len` bytes of `a` and `b`. Returns `1` if `a > b`, `-1` if `a < b`, and `0` if equal. Returns `0` immediately if `a == b` or `len` is zero.

Processes 64-byte blocks for large buffers, 8-byte blocks for medium buffers, and byte-by-byte for the tail.

### `fio_strlen`

```c
size_t fio_strlen(const char *str);
```

Returns the length of the NUL-terminated string `str`, or `0` if `str` is `NULL`. The implementation is a simple byte loop the compiler can auto-vectorize. Building with Address Sanitizer may flag speculative reads.

## OS patch environment helpers

[`./001 patches.h`](./001%20patches.h) provides `fio_sys_env` and `fio_sys_env_set` as portable replacements for `getenv` and `setenv`.

```c
char *fio_sys_env(const char *name);
int   fio_sys_env_set(const char *name, const char *value, int overwrite);
```

- `fio_sys_env(name)` returns the value of the environment variable, or `NULL`.
- `fio_sys_env_set(name, value, overwrite)` sets the variable. If `overwrite` is zero and the variable already exists, it is left unchanged. Returns `0` on success, `-1` on failure.

On POSIX these are thin wrappers around `getenv`/`setenv`. On Windows they call `GetEnvironmentVariableA` and `SetEnvironmentVariableA` directly, bypassing macro redefinitions such as the one in Ruby's `win32.h` and avoiding `wchar_t` conversion issues.

## Other OS patches

[`./001 patches.h`](./001%20patches.h) also fills small portability gaps:

- **macOS < 10.12**: defines a `clock_gettime` fallback using `gettimeofday`, plus `CLOCK_REALTIME` and `CLOCK_MONOTONIC` placeholders.
- **Windows startup**: enables virtual terminal processing for stdout/stderr and sets binary file mode by default.
- **Windows symbol gaps**: supplies inline patches for `strcasecmp`, `write`, `read`, `clock_gettime`, and `pwrite`; maps `O_*`/`S_*` constants, `fstat`/`stat`/`lseek`/`unlink`, and defines `PATH_MAX`.
- **POSIX branch**: present but currently has no extra patches beyond the environment helpers.
