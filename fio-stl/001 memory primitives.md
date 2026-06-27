# Memory Primitives

Low-level memory helpers used everywhere else in the STL. Most are defined in [`./000 core.h`](./000%20core.h); the allocator abstraction macros live in [`./001 header.h`](./001%20header.h).

## Memory-Function Selector Macros

These macros route standard-library-style calls to either compiler builtins or libc functions. Define any of them before inclusion to override the default.

| Macro | Default | Use |
|-------|---------|-----|
| `FIO_MEMCPY(dest, src, n)` | `__builtin_memcpy` if available, else `memcpy` | Variable-length copy. |
| `FIO_MEMMOVE(dest, src, n)` | `__builtin_memmove` if available, else `memmove` | Overlap-safe copy. |
| `FIO_MEMSET(dest, c, n)` | `__builtin_memset` if available, else `memset` | Fill memory. |
| `FIO_MEMCHR(ptr, c, n)` | `__builtin_memchr` if available, else `memchr` | Find byte. |
| `FIO_STRLEN(str)` | `__builtin_strlen` if available, else `strlen` | C string length. |
| `FIO_MEMCMP(a, b, n)` | `__builtin_memcmp` if available, else `memcmp` | Compare memory. |

If `FIO_MEMALT` is defined, any selector that was not already set is routed to the matching facil.io fallback:

```c
#define FIO_MEMCPY  fio_memcpy
#define FIO_MEMMOVE fio_memcpy
#define FIO_MEMCMP  fio_memcmp
#define FIO_MEMCHR  fio_memchr
#define FIO_MEMSET  fio_memset
#define FIO_STRLEN  fio_strlen
```

Those fallback routines are documented in `./001 memalt.h` (the memalt module).

## Fixed-Size Copy Helpers

These functions copy an exact number of bytes from `src` to `dest`. They are implemented with the fixed-size `FIO_MEMCPY` path when builtins are available, otherwise with a compact byte loop.

- When `__builtin_memcpy` is available, each helper returns `dest` (the `__builtin_memcpy` return value).
- In the non-builtin fallback, each helper returns `dest + n`.

`fio_memcpy0` always returns `dest`.

```c
void *fio_memcpy0 (void *restrict dest, const void *restrict src);
void *fio_memcpy1 (void *restrict dest, const void *restrict src);
void *fio_memcpy2 (void *restrict dest, const void *restrict src);
void *fio_memcpy3 (void *restrict dest, const void *restrict src);
void *fio_memcpy4 (void *restrict dest, const void *restrict src);
void *fio_memcpy5 (void *restrict dest, const void *restrict src);
void *fio_memcpy6 (void *restrict dest, const void *restrict src);
void *fio_memcpy7 (void *restrict dest, const void *restrict src);
void *fio_memcpy8 (void *restrict dest, const void *restrict src);
void *fio_memcpy16(void *restrict dest, const void *restrict src);
void *fio_memcpy32(void *restrict dest, const void *restrict src);
void *fio_memcpy64(void *restrict dest, const void *restrict src);
void *fio_memcpy128(void *restrict dest, const void *restrict src);
void *fio_memcpy256(void *restrict dest, const void *restrict src);
void *fio_memcpy512(void *restrict dest, const void *restrict src);
void *fio_memcpy1024(void *restrict dest, const void *restrict src);
void *fio_memcpy2048(void *restrict dest, const void *restrict src);
void *fio_memcpy4096(void *restrict dest, const void *restrict src);
```

### Tail helpers

`fio_memcpyNx` copies `len & N` bytes — useful for handling the tail end of a larger copy without explicit branches.

```c
void *fio_memcpy0x  (void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy7x  (void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy15x (void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy31x (void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy63x (void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy127x(void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy255x(void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy511x(void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy1023x(void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy2047x(void *restrict dest, const void *restrict src, size_t len);
void *fio_memcpy4095x(void *restrict dest, const void *restrict src, size_t len);
```

### Internal unsafe helpers

Two internal helpers back the tail functions. They assume the regions do not overlap and perform no validation.

```c
void *fio___memcpy_unsafe_63x(void *restrict dest, const void *restrict src, size_t len);
void *fio___memcpy_unsafe_x (void *restrict dest, const void *restrict src, size_t len);
```

- `fio___memcpy_unsafe_63x` is optimized for lengths up to 63 bytes.
- `fio___memcpy_unsafe_x` handles arbitrary lengths, copying 128-byte blocks (or 256-byte blocks when `FIO_LIMIT_INTRINSIC_BUFFER` is `0`) and finishing with a 64-byte tail.

These are `static`/`static inline` helpers; do not use them directly unless you are implementing a higher-level routine and have already checked the inputs.

## Unaligned Buffer Accessors

Read and write integers from byte buffers that may not be naturally aligned. The suffix controls byte order:

- `u` — native (unspecified) byte order.
- `_le` — little-endian.
- `_be` — big-endian / network byte order.

```c
uint8_t  fio_buf2u8u (const void *buf);
uint8_t  fio_buf2u8_le(const void *buf);
uint8_t  fio_buf2u8_be(const void *buf);
void     fio_u2buf8u (void *buf, uint8_t  v);
void     fio_u2buf8_le(void *buf, uint8_t  v);
void     fio_u2buf8_be(void *buf, uint8_t  v);

uint16_t fio_buf2u16u (const void *buf);
uint16_t fio_buf2u16_le(const void *buf);
uint16_t fio_buf2u16_be(const void *buf);
void     fio_u2buf16u (void *buf, uint16_t v);
void     fio_u2buf16_le(void *buf, uint16_t v);
void     fio_u2buf16_be(void *buf, uint16_t v);

uint32_t fio_buf2u24u (const void *buf);
uint32_t fio_buf2u24_le(const void *buf);
uint32_t fio_buf2u24_be(const void *buf);
void     fio_u2buf24u (void *buf, uint32_t v);
void     fio_u2buf24_le(void *buf, uint32_t v);
void     fio_u2buf24_be(void *buf, uint32_t v);

uint32_t fio_buf2u32u (const void *buf);
uint32_t fio_buf2u32_le(const void *buf);
uint32_t fio_buf2u32_be(const void *buf);
void     fio_u2buf32u (void *buf, uint32_t v);
void     fio_u2buf32_le(void *buf, uint32_t v);
void     fio_u2buf32_be(void *buf, uint32_t v);

uint64_t fio_buf2u64u (const void *buf);
uint64_t fio_buf2u64_le(const void *buf);
uint64_t fio_buf2u64_be(const void *buf);
void     fio_u2buf64u (void *buf, uint64_t v);
void     fio_u2buf64_le(void *buf, uint64_t v);
void     fio_u2buf64_be(void *buf, uint64_t v);

size_t   fio_buf2zu(const void *buf);
void     fio_u2bufzu(void *buf, size_t v);
```

`fio_buf2u24u` / `fio_u2buf24u` and `fio_buf2zu` / `fio_u2bufzu` select the correct implementation at compile time based on host endianness and `size_t` width.

Example:

```c
uint8_t packet[8] = {0};
fio_u2buf32_be(packet + 2, 0x12345678);
uint32_t n = fio_buf2u32_be(packet + 2);  // 0x12345678
```

## Memory Allocator Abstraction

The STL types use these macros instead of calling `malloc`/`free` directly, so a custom allocator can be plugged in by defining them before inclusion. They are reset and redefined in [`./001 header.h`](./001%20header.h) for each inclusion cycle.

```c
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)  realloc((ptr), (new_size))
#define FIO_MEM_FREE(ptr, size)                             free((ptr))
#define FIO_MEM_ALIGNMENT_SIZE                              sizeof(long double)
#define FIO_MEM_REALLOC_IS_SAFE                             0
```

- `FIO_MEM_REALLOC` — reallocates `ptr` from `old_size` to `new_size`, copying at least `copy_len` bytes if the object moves. `NULL` `ptr` acts like `malloc`; `new_size == 0` acts like `free`.
- `FIO_MEM_FREE` — frees memory allocated with `FIO_MEM_REALLOC`.
- `FIO_MEM_ALIGNMENT_SIZE` — the alignment the allocator guarantees.
- `FIO_MEM_REALLOC_IS_SAFE` — non-zero when the allocator zero-initializes returned memory.

If the custom `fio_malloc` allocator has already been included (`H___FIO_MALLOC___H` is defined), the defaults are routed to it instead:

```c
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)  fio_realloc2((ptr), (new_size), (copy_len))
#define FIO_MEM_FREE(ptr, size)                             fio_free((ptr))
#define FIO_MEM_ALIGNMENT_SIZE                              fio_malloc_alignment()
#define FIO_MEM_REALLOC_IS_SAFE                             fio_realloc_is_safe()
```

Define `FIO_MEM_RESET` before inclusion to force these defaults to be re-evaluated. Define `FIO_MEMORY_DISABLE` or `FIO_MALLOC_TMP_USE_SYSTEM` to force the system allocator for an inclusion cycle.

### Recursive variants

The underscore-suffixed macros exist for use during recursive `#include` cycles:

```c
FIO_MEM_REALLOC_
FIO_MEM_FREE_
FIO_MEM_ALIGNMENT_SIZE_
FIO_MEM_REALLOC_IS_SAFE_
```

They normally mirror the non-underscore versions. When `FIO_MALLOC_TMP_USE_SYSTEM` is set they are forced to the system `realloc`/`free` for that cycle only, while the non-underscore macros keep their outer meaning.

## Unaligned Memory Access Knob

```c
#define FIO_UNALIGNED_MEMORY_ACCESS_ENABLED 1
```

Defined in [`./000 core.h`](./000%20core.h). It is auto-detected: if `FIO_UNALIGNED_ACCESS` is `1` (the default) and the target CPU is known to tolerate unaligned access — x86, x86_64, ARM64, or another architecture that defines `__ARM_FEATURE_UNALIGNED` — then `FIO_UNALIGNED_MEMORY_ACCESS_ENABLED` becomes `1`. Otherwise it is `0`.

Set `FIO_UNALIGNED_ACCESS` to `0` before inclusion to disable the detection attempt, or define `FIO_UNALIGNED_MEMORY_ACCESS_ENABLED` directly to override the result.

## See Also

- `./001 memalt.h` (the memalt module) — variable-length fallback memory routines (`fio_memcpy`, `fio_memset`, `fio_memcmp`, `fio_memchr`, `fio_strlen`).
- [`./001 endian.md`](./001%20endian.md) — byte-order detection and swap helpers.
- [`./000 core.h`](./000%20core.h) — source of truth for copy helpers and the unaligned-access knob.
- [`./001 header.h`](./001%20header.h) — source of truth for the allocator macros.
