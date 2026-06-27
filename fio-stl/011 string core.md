# String Core

```c
#define FIO_STR
#include "fio-stl.h"
```

Binary-safe string helpers for building, editing, escaping, encoding, and comparing text. This module works with the lightweight `fio_str_info_s` and `fio_buf_info_s` descriptors, plus two owned string types: copy-on-write `fio_bstr` and hash-map key string `fio_keystr_s`.

All authoring helpers take a `fio_string_realloc_fn` callback so they can grow either a stack buffer or a heap allocation.

---

## Types

#### `fio_string_realloc_fn`

```c
typedef int (*fio_string_realloc_fn)(fio_str_info_s *dest, size_t len);
```

Callback that grows `dest->buf` to at least `len + 1` bytes and updates `dest->capa`. Returns `0` on success, `-1` on failure.

#### `fio_string_write_s`

```c
typedef struct {
  size_t klass;
  union {
    struct { size_t len; const char *buf; } str;
    double f;
    int64_t i;
    uint64_t u;
  } info;
} fio_string_write_s;
```

Argument item used by `fio_string_write2` and `fio_bstr_write2`. The `klass` field selects the union member: `1` = string, `2` = signed int, `3` = unsigned int, `4` = hex, `5` = binary, `6` = double.

---

## Authorship Helpers

#### `fio_string_write`

```c
SFUNC int fio_string_write(fio_str_info_s *dest,
                           fio_string_realloc_fn reallocate,
                           const void *restrict src,
                           size_t len);
```

Appends `len` bytes from `src` to `dest`. Grows `dest` if `reallocate` is provided; otherwise truncates when full. Always NUL-terminates on success.

**Parameters:**
- `dest` — target string descriptor. Must have a valid buffer and `capa >= len`.
- `reallocate` — growth callback, may be `NULL`.
- `src` — data to append.
- `len` — bytes to append.

**Returns:** `0` on success, `-1` if reallocation was needed and failed.

#### `fio_string_replace`

```c
SFUNC int fio_string_replace(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             intptr_t start_pos,
                             size_t overwrite_len,
                             const void *src,
                             size_t len);
```

Inserts, overwrites, or deletes a slice. Negative `start_pos` counts from the end (`-1` = last byte). When `overwrite_len` is `0`, the data is inserted. When `len` is `0`, the slice is deleted.

**Parameters:**
- `start_pos` — byte offset where editing begins.
- `overwrite_len` — bytes to overwrite or remove.
- `src` — replacement data, ignored if `len == 0`.
- `len` — replacement length.

**Returns:** `0` on success, `-1` on failure.

#### `fio_string_write2`

```c
SFUNC int fio_string_write2(fio_str_info_s *restrict dest,
                            fio_string_realloc_fn reallocate,
                            const fio_string_write_s srcs[]);

#define fio_string_write2(dest, reallocate, ...)                               \
  fio_string_write2((dest),                                                    \
                    (reallocate),                                              \
                    (fio_string_write_s[]){__VA_ARGS__, {0}})
```

Appends a mixed list of strings and numbers in a single pass. The macro builds a temporary `fio_string_write_s[]` array and appends a zero terminator.

```c
fio_string_write2(&str, my_realloc,
                  FIO_STRING_WRITE_STR1("The answer is: "),
                  FIO_STRING_WRITE_NUM(42),
                  FIO_STRING_WRITE_STR2(" (0x", 4),
                  FIO_STRING_WRITE_HEX(42),
                  FIO_STRING_WRITE_STR2(")", 1));
```

#### `FIO_STRING_WRITE_*`

```c
#define FIO_STRING_WRITE_STR1(str_) /* string with FIO_STRLEN length */
#define FIO_STRING_WRITE_STR2(str_, len_) /* string with explicit length */
#define FIO_STRING_WRITE_STR_INFO(str_) /* from fio_str_info_s */
#define FIO_STRING_WRITE_NUM(num)     /* signed integer */
#define FIO_STRING_WRITE_UNUM(num)    /* unsigned integer */
#define FIO_STRING_WRITE_HEX(num)     /* hex representation */
#define FIO_STRING_WRITE_BIN(num)     /* binary representation */
#define FIO_STRING_WRITE_FLOAT(num)   /* double */
```

Helper macros that build `fio_string_write_s` items for `fio_string_write2`.

---

## Numeral Helpers

#### `fio_string_write_i`

```c
SFUNC int fio_string_write_i(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             int64_t i);
```

Appends a signed base-10 integer.

#### `fio_string_write_u`

```c
SFUNC int fio_string_write_u(fio_str_info_s *dest,
                             fio_string_realloc_fn reallocate,
                             uint64_t i);
```

Appends an unsigned base-10 integer.

#### `fio_string_write_hex`

```c
SFUNC int fio_string_write_hex(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               uint64_t i);
```

Appends a lowercase hex representation.

#### `fio_string_write_bin`

```c
SFUNC int fio_string_write_bin(fio_str_info_s *dest,
                               fio_string_realloc_fn reallocate,
                               uint64_t i);
```

Appends a binary representation.

---

## printf Support

#### `fio_string_printf`

```c
SFUNC FIO___PRINTF_STYLE(3, 0) int fio_string_printf(
    fio_str_info_s *dest,
    fio_string_realloc_fn reallocate,
    const char *format,
    ...);
```

Appends formatted output, growing the buffer as needed.

#### `fio_string_vprintf`

```c
SFUNC FIO___PRINTF_STYLE(3, 0) int fio_string_vprintf(
    fio_str_info_s *dest,
    fio_string_realloc_fn reallocate,
    const char *format,
    va_list argv);
```

`va_list` variant of `fio_string_printf`.

---

## Escaping and Encoding

#### `fio_string_write_escape`

```c
SFUNC int fio_string_write_escape(fio_str_info_s *restrict dest,
                                  fio_string_realloc_fn reallocate,
                                  const void *raw,
                                  size_t raw_len);
```

Escapes `raw` using JSON string rules and appends the result.

#### `fio_string_write_unescape`

```c
SFUNC int fio_string_write_unescape(fio_str_info_s *dest,
                                    fio_string_realloc_fn reallocate,
                                    const void *enscaped,
                                    size_t enscaped_len);
```

Unescapes JSON-style data and appends it.

#### `fio_string_write_base32enc` / `fio_string_write_base32dec`

```c
SFUNC int fio_string_write_base32enc(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *raw,
                                     size_t raw_len);
SFUNC int fio_string_write_base32dec(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *encoded,
                                     size_t encoded_len);
```

Base32 encode / decode.

#### `fio_string_write_base64enc` / `fio_string_write_base64dec`

```c
SFUNC int fio_string_write_base64enc(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *raw,
                                     size_t raw_len,
                                     uint8_t url_encoded);
SFUNC int fio_string_write_base64dec(fio_str_info_s *dest,
                                     fio_string_realloc_fn reallocate,
                                     const void *encoded,
                                     size_t encoded_len);
```

Base64 encode / decode. Pass `url_encoded = 1` to use URL-safe alphabet.

#### `fio_string_write_url_enc` / `fio_string_write_url_dec` / `fio_string_write_path_dec`

```c
SFUNC int fio_string_write_url_enc(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const void *raw,
                                   size_t raw_len);
SFUNC int fio_string_write_url_dec(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const void *encoded,
                                   size_t encoded_len);
SFUNC int fio_string_write_path_dec(fio_str_info_s *dest,
                                    fio_string_realloc_fn reallocate,
                                    const void *encoded,
                                    size_t encoded_len);
```

URL (percent) encoding and decoding. `url_dec` converts `+` to space; `path_dec` does not.

#### `fio_string_write_html_escape` / `fio_string_write_html_unescape`

```c
SFUNC int fio_string_write_html_escape(fio_str_info_s *dest,
                                       fio_string_realloc_fn reallocate,
                                       const void *raw,
                                       size_t raw_len);
SFUNC int fio_string_write_html_unescape(fio_str_info_s *dest,
                                         fio_string_realloc_fn reallocate,
                                         const void *enscaped,
                                         size_t enscaped_len);
```

HTML escape / unescape. The unescape implementation is minimal and incomplete.

---

## File Reading

#### `fio_string_readfd`

```c
SFUNC int fio_string_readfd(fio_str_info_s *dest,
                            fio_string_realloc_fn reallocate,
                            int fd,
                            intptr_t start_at,
                            size_t limit);
```

Reads up to `limit` bytes from seekable file descriptor `fd` starting at `start_at`. Negative `start_at` counts from EOF. `limit == 0` reads to EOF.

#### `fio_string_readfile`

```c
SFUNC int fio_string_readfile(fio_str_info_s *dest,
                              fio_string_realloc_fn reallocate,
                              const char *filename,
                              intptr_t start_at,
                              size_t limit);
```

Opens `filename` and reads its contents (or a slice) into `dest`.

#### `fio_string_getdelim_fd`

```c
SFUNC int fio_string_getdelim_fd(fio_str_info_s *dest,
                                 fio_string_realloc_fn reallocate,
                                 int fd,
                                 intptr_t start_at,
                                 char delim,
                                 size_t limit);
```

Reads from `fd` until `delim` or EOF.

#### `fio_string_getdelim_file`

```c
SFUNC int fio_string_getdelim_file(fio_str_info_s *dest,
                                   fio_string_realloc_fn reallocate,
                                   const char *filename,
                                   intptr_t start_at,
                                   char delim,
                                   size_t limit);
```

Same as `fio_string_getdelim_fd` but opens and closes the file.

---

## Memory Helpers

#### `fio_string_capa4len`

```c
FIO_IFUNC size_t fio_string_capa4len(size_t new_len);
```

Returns a 16-byte-aligned capacity for `new_len`.

#### Default Allocator Callbacks

```c
#define FIO_STRING_SYS_REALLOC   fio_string_sys_reallocate
#define FIO_STRING_REALLOC       fio_string_default_reallocate
#define FIO_STRING_ALLOC_COPY    fio_string_default_allocate_copy
#define FIO_STRING_ALLOC_KEY     fio_string_default_key_alloc
#define FIO_STRING_FREE          fio_string_default_free
#define FIO_STRING_FREE2         fio_string_default_free2
#define FIO_STRING_FREE_KEY      fio_string_default_free_key
#define FIO_STRING_FREE_NOOP     fio_string_default_free_noop
#define FIO_STRING_FREE_NOOP2    fio_string_default_free_noop2
```

Convenience macros pointing to the default callbacks.

#### `fio_string_default_reallocate`

```c
SFUNC int fio_string_default_reallocate(fio_str_info_s *dst, size_t len);
```

Grows `dst` using the current facil.io allocator (`FIO_MEM_REALLOC_`).

#### `fio_string_default_allocate_copy`

```c
SFUNC int fio_string_default_allocate_copy(fio_str_info_s *dest, size_t new_capa);
```

Allocates a new buffer and copies existing data into it.

#### `fio_string_default_free`

```c
SFUNC void fio_string_default_free(void *);
```

Frees a buffer allocated with the default callbacks.

---

## UTF-8 Helpers

#### `fio_string_utf8_valid`

```c
SFUNC bool fio_string_utf8_valid(fio_str_info_s str);
```

Returns `true` if `str` is valid UTF-8.

#### `fio_string_utf8_len`

```c
SFUNC size_t fio_string_utf8_len(fio_str_info_s str);
```

Returns the number of Unicode code points, or `0` if invalid.

#### `fio_string_utf8_valid_code_point`

```c
SFUNC size_t fio_string_utf8_valid_code_point(const void *u8c, size_t buf_len);
```

Returns the byte length of the UTF-8 code point at `u8c` (`1`–`4`), or `0` if invalid.

#### `fio_string_utf8_select`

```c
SFUNC int fio_string_utf8_select(fio_str_info_s str,
                                 intptr_t *pos,
                                 size_t *len);
```

Converts a UTF-8 character range (`pos`, `len`) into raw byte offsets. Returns `-1` on error and sets `pos` to `-1` if the string is invalid before the selection.

---

## Comparison Helpers

#### `fio_string_is_greater_buf`

```c
SFUNC int fio_string_is_greater_buf(fio_buf_info_s a, fio_buf_info_s b);
```

Returns `1` if `a` is lexicographically greater than `b`; `0` otherwise (including equality).

#### `fio_string_is_greater`

```c
FIO_IFUNC int fio_string_is_greater(fio_str_info_s a, fio_str_info_s b);
```

Same comparison for `fio_str_info_s`.

---

## Binary String (`fio_bstr`)

A reference-counted, copy-on-write, NUL-terminated binary string. The public pointer points at the first character; metadata lives just before it.

#### `fio_bstr_reserve`

```c
FIO_IFUNC char *fio_bstr_reserve(char *bstr, size_t len);
```

Ensures at least `len` additional bytes are available.

#### `fio_bstr_copy`

```c
FIO_IFUNC char *fio_bstr_copy(char *bstr);
```

Returns a shallow copy, incrementing the reference count. Falls back to a real copy if the reference count overflows.

#### `fio_bstr_free`

```c
FIO_IFUNC void fio_bstr_free(char *bstr);
```

Decrements the reference count and frees memory when it reaches zero.

#### `fio_bstr_info` / `fio_bstr_buf` / `fio_bstr_len` / `fio_bstr_len_set`

```c
FIO_IFUNC fio_str_info_s fio_bstr_info(const char *bstr);
FIO_IFUNC fio_buf_info_s fio_bstr_buf(const char *bstr);
FIO_IFUNC size_t fio_bstr_len(const char *bstr);
FIO_IFUNC char *fio_bstr_len_set(char *bstr, size_t len);
```

Inspect or resize the string. `fio_bstr_len_set` may reallocate if the string is shared or too small.

#### `fio_bstr_write` and Friends

```c
FIO_IFUNC char *fio_bstr_write(char *bstr, const void *src, size_t len);
FIO_IFUNC char *fio_bstr_replace(char *bstr, intptr_t start_pos,
                                 size_t overwrite_len, const void *src,
                                 size_t len);
FIO_IFUNC char *fio_bstr_write2(char *bstr, const fio_string_write_s srcs[]);
#define fio_bstr_write2(bstr, ...) \
  fio_bstr_write2(bstr, (fio_string_write_s[]){__VA_ARGS__, {0}})
FIO_IFUNC char *fio_bstr_write_i(char *bstr, int64_t num);
FIO_IFUNC char *fio_bstr_write_u(char *bstr, uint64_t num);
FIO_IFUNC char *fio_bstr_write_hex(char *bstr, uint64_t num);
FIO_IFUNC char *fio_bstr_write_bin(char *bstr, uint64_t num);
FIO_IFUNC char *fio_bstr_write_escape(char *bstr, const void *src, size_t len);
FIO_IFUNC char *fio_bstr_write_unescape(char *bstr, const void *src, size_t len);
FIO_IFUNC char *fio_bstr_write_base64enc(char *bstr, const void *src,
                                         size_t len, uint8_t url_encoded);
FIO_IFUNC char *fio_bstr_write_base64dec(char *bstr, const void *src, size_t len);
FIO_IFUNC char *fio_bstr_write_url_enc(char *bstr, const void *data, size_t len);
FIO_IFUNC char *fio_bstr_write_url_dec(char *bstr, const void *encoded, size_t len);
FIO_IFUNC char *fio_bstr_write_html_escape(char *bstr, const void *raw, size_t len);
FIO_IFUNC char *fio_bstr_write_html_unescape(char *bstr, const void *escaped,
                                             size_t len);
FIO_IFUNC char *fio_bstr_readfd(char *bstr, int fd, intptr_t start_at,
                                intptr_t limit);
FIO_IFUNC char *fio_bstr_readfile(char *bstr, const char *filename,
                                  intptr_t start_at, intptr_t limit);
FIO_IFUNC char *fio_bstr_getdelim_file(char *bstr, const char *filename,
                                       intptr_t start_at, char delim,
                                       size_t limit);
FIO_IFUNC char *fio_bstr_getdelim_fd(char *bstr, int fd, intptr_t start_at,
                                     char delim, size_t limit);
FIO_IFUNC char *fio_bstr_printf(char *bstr, const char *format, ...);
```

Every `fio_bstr_*` function returns the possibly-reallocated string pointer. Always assign the result back.

#### `fio_bstr_is_greater` / `fio_bstr_is_eq` / `fio_bstr_is_eq2info` / `fio_bstr_is_eq2buf`

```c
FIO_SFUNC int fio_bstr_is_greater(const char *a, const char *b);
FIO_SFUNC int fio_bstr_is_eq(const char *a_, const char *b_);
FIO_SFUNC int fio_bstr_is_eq2info(const char *a_, fio_str_info_s b);
FIO_SFUNC int fio_bstr_is_eq2buf(const char *a_, fio_buf_info_s b);
```

Comparison helpers.

#### `fio_bstr_reallocate`

```c
SFUNC int fio_bstr_reallocate(fio_str_info_s *dest, size_t len);
```

Default reallocate callback for `fio_bstr`.

---

## Key String (`fio_keystr_s`)

A compact key type used by hash maps. Small strings are embedded in the struct; larger ones store a pointer and length. Not NUL-terminated.

#### `fio_keystr_tmp`

```c
FIO_IFUNC fio_keystr_s fio_keystr_tmp(const char *buf, uint32_t len);
```

Returns a temporary key referencing `buf`/`len`.

#### `fio_keystr_init`

```c
FIO_SFUNC fio_keystr_s fio_keystr_init(fio_str_info_s str,
                                       void *(*alloc_func)(size_t len));
```

Copies `str` into a persistent key. Strings short enough are embedded; otherwise `alloc_func` is used. Pass `capa == FIO_KEYSTR_CONST` to store a borrowed pointer.

#### `fio_keystr_destroy`

```c
FIO_SFUNC void fio_keystr_destroy(fio_keystr_s *key,
                                  void (*free_func)(void *, size_t));
```

Releases a key allocated by `fio_keystr_init`.

#### `fio_keystr_buf` / `fio_keystr_info` / `fio_keystr_is_eq` / `fio_keystr_is_eq2` / `fio_keystr_is_eq3` / `fio_keystr_hash`

```c
FIO_IFUNC fio_buf_info_s fio_keystr_buf(fio_keystr_s *str);
FIO_IFUNC fio_str_info_s fio_keystr_info(fio_keystr_s *str);
FIO_IFUNC int fio_keystr_is_eq(fio_keystr_s a, fio_keystr_s b);
FIO_IFUNC int fio_keystr_is_eq2(fio_keystr_s a_, fio_str_info_s b);
FIO_IFUNC int fio_keystr_is_eq3(fio_keystr_s a_, fio_buf_info_s b);
FIO_IFUNC uint64_t fio_keystr_hash(fio_keystr_s a);
```

Inspect, compare, and hash key strings.

#### `FIO_KEYSTR_CONST`

```c
#define FIO_KEYSTR_CONST ((size_t)-1LL)
```

Magic `capa` value for `fio_keystr_init` that keeps a borrowed pointer instead of allocating.

---

## Example

```c
#define FIO_STR
#include "fio-stl.h"

int my_realloc(fio_str_info_s *dest, size_t len) {
  size_t capa = fio_string_capa4len(len);
  char *tmp = realloc(dest->buf, capa);
  if (!tmp) return -1;
  dest->buf = tmp;
  dest->capa = capa;
  return 0;
}

int main(void) {
  char buf[32];
  fio_str_info_s str = FIO_STR_INFO3(buf, 0, 32);

  fio_string_write(&str, my_realloc, "answer: ", 8);
  fio_string_write_i(&str, my_realloc, 42);
  fprintf(stderr, "%s\n", str.buf);

  char *b = NULL;
  b = fio_bstr_write(b, "hello ", 6);
  b = fio_bstr_write(b, "world", 5);
  fprintf(stderr, "%s (len=%zu)\n", b, fio_bstr_len(b));
  fio_bstr_free(b);
  return 0;
}
```

------------------------------------------------------------
