## Alternative Memory Functions

```c
#define FIO_MEMALT
#include FIO_INCLUDE_FILE
```

By defining `FIO_MEMALT`, alternative implementations of common C memory and string functions are defined and made available. These provide fallback implementations of `memcpy`, `memset`, `memchr`, `memcmp`, and `strlen` that do not depend on the compiler's C library.

Each function is a drop-in replacement for its standard library counterpart:

- **`fio_memcpy`** - copies memory (handles overlapping regions like `memmove`)
- **`fio_memset`** - fills memory with an 8-byte repeating pattern
- **`fio_memchr`** - searches for a byte in a buffer
- **`fio_memcmp`** - compares two memory regions
- **`fio_strlen`** - computes the length of a NUL-terminated string

These are intended as fallbacks for environments where the standard C library functions may not be available or when the library's internal `FIO_MEMCPY`, `FIO_MEMSET`, `FIO_MEMCHR`, and `FIO_MEMCMP` macros are redirected to use them.

**Note**: in most cases, the compiler's built-in C library implementations will be faster. These alternatives exist for portability and for use in freestanding environments.

### Memory Alternative Functions

#### `fio_memset`

```c
void *fio_memset(void *restrict dest, uint64_t data, size_t bytes);
```

Fills `bytes` of memory at `dest` with the given 8-byte pattern `data`.

If `data` is less than `0x100` (a single byte value), it is broadcast to fill all 8 bytes of the pattern, matching the behavior of the standard `memset`. For values >= `0x100`, the full 64-bit pattern is repeated across the destination buffer.

**Parameters:**
- `dest` - pointer to the destination buffer to fill
- `data` - the 8-byte pattern to fill with (values < 0x100 are broadcast to all bytes)
- `bytes` - number of bytes to fill

**Returns:** `dest`, or NULL if `dest` is NULL.

**Note**: unlike the standard `memset` which takes an `int` value, this function accepts a `uint64_t`, allowing an 8-byte repeating pattern to be set in a single call.

#### `fio_memcpy`

```c
void *fio_memcpy(void *dest, const void *src, size_t bytes);
```

Copies `bytes` of memory from `src` to `dest`. Handles overlapping memory regions correctly (like `memmove`), choosing forward or backward copying as needed.

For small non-overlapping copies (up to 64 bytes), an optimized fast path is used with overlapping fixed-size copy operations. For larger copies or overlapping regions, a 64-byte buffered copy strategy is employed to ensure correctness.

**Parameters:**
- `dest` - pointer to the destination buffer
- `src` - pointer to the source buffer
- `bytes` - number of bytes to copy

**Returns:** `dest`.

**Note**: returns immediately (no-op) if `dest == src`, if `bytes` is 0, or if either pointer is NULL. A debug log message is emitted if a NULL pointer is detected with a non-zero `bytes` value.

#### `fio_memchr`

```c
void *fio_memchr(const void *buffer, const char token, size_t len);
```

Searches the first `len` bytes of `buffer` for the byte `token`.

Uses platform-specific SIMD acceleration when available (AVX2 on x86-64, SSE2 on x86, NEON on ARM), falling back to a scalar implementation that processes 128 bytes at a time using bit-manipulation tricks.

**Parameters:**
- `buffer` - pointer to the memory region to search
- `token` - the byte value to search for
- `len` - number of bytes to search

**Returns:** a pointer to the first occurrence of `token` in `buffer`, or NULL if not found or if `buffer` is NULL or `len` is 0.

**Note**: this is a fallback for the standard `memchr`. The standard library version should generally be faster on most platforms.

#### `fio_memcmp`

```c
int fio_memcmp(const void *a, const void *b, size_t len);
```

Compares the first `len` bytes of memory regions `a` and `b`.

Processes data in 64-byte blocks for large comparisons, 8-byte blocks for medium comparisons, and byte-by-byte for small comparisons (fewer than 8 bytes).

**Parameters:**
- `a` - pointer to the first memory region
- `b` - pointer to the second memory region
- `len` - number of bytes to compare

**Returns:** `1` if `a > b`, `-1` if `a < b`, `0` if the regions are equal. Returns `0` if `a == b` (same pointer) or if `len` is 0.

**Note**: this is a fallback for the standard `memcmp`. The standard library version should generally be faster on most platforms.

#### `fio_strlen`

```c
size_t fio_strlen(const char *str);
```

Computes the length of the NUL-terminated string `str`.

Relies on the compiler to auto-vectorize the simple byte-scanning loop.

**Parameters:**
- `str` - pointer to a NUL-terminated string

**Returns:** the number of bytes before the first NUL byte, or `0` if `str` is NULL.

**Note**: this is a fallback for the standard `strlen`. The standard library version should generally be faster. This function may raise Address Sanitizer warnings due to speculative reads.

### Example

```c
#define FIO_MEMALT
#include FIO_INCLUDE_FILE

void example(void) {
  char buf[64];

  /* Fill buffer with zeros */
  fio_memset(buf, 0, sizeof(buf));

  /* Fill buffer with a repeating byte pattern */
  fio_memset(buf, 'A', 32);

  /* Copy data (handles overlapping regions) */
  const char *src = "Hello, memalt!";
  fio_memcpy(buf, src, 14);

  /* Search for a byte */
  void *found = fio_memchr(buf, ',', 14);
  /* found points to buf[5] (the comma) */

  /* Compare memory regions */
  int cmp = fio_memcmp(buf, src, 14);
  /* cmp == 0 (equal) */

  /* Get string length */
  size_t len = fio_strlen(buf);
  /* len == 14 */
}
```

-------------------------------------------------------------------------------
