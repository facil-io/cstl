## URL-Encoded Parser

```c
#define FIO_URL_ENCODED
#include "fio-stl.h"
```

A callback-based parser for `application/x-www-form-urlencoded` data. It splits `name=value&...` pairs and decodes each name and value before invoking the callback, without modifying the input. Temporary buffers use stack storage with heap fallback for long fields. Implemented in [`./004 urlencoded.h`](./004%20urlencoded.h).

### Types

#### `fio_url_encoded_parser_callbacks_s`

```c
typedef struct {
  void *(*on_pair)(void *udata, fio_buf_info_s name, fio_buf_info_s value);
  void (*on_error)(void *udata);
} fio_url_encoded_parser_callbacks_s;
```

Parser callbacks. `udata` is passed to callbacks and updated from `on_pair`'s return value.

**Members:**
- `on_pair` - called for each non-empty pair with decoded `name` and `value`. Buffers are borrowed and valid **only during the callback**; copy anything needed later and do not free them. Lengths are in bytes and may include embedded NUL bytes. Return the (possibly updated) non-NULL `udata` to continue, or `NULL` to stop.
- `on_error` - optional; called once with the current `udata` if decoding cannot allocate a temporary buffer. Buffers are freed before this callback, and the failed pair is not passed to `on_pair`. Not called when `on_pair` returns `NULL`.

If `callbacks` or a member is `NULL`, an internal no-op callback is used.

#### `fio_url_encoded_result_s`

```c
typedef struct {
  size_t consumed;
  size_t count;
  int err;
} fio_url_encoded_result_s;
```

Parse result.

**Members:**
- `consumed` - number of original input bytes consumed. On success this equals `len`. When `on_pair` returns `NULL`, includes that pair but excludes its following `&`. On decoding failure, excludes the failed pair.
- `count` - number of non-empty pairs passed to `on_pair`, including a pair whose callback returned `NULL`.
- `err` - zero on success; non-zero when decoding failed or `on_pair` returned `NULL`.

### Parsing

#### `fio_url_encoded_parse`

```c
SFUNC fio_url_encoded_result_s
fio_url_encoded_parse(const fio_url_encoded_parser_callbacks_s *callbacks,
                      void *udata,
                      const char *data,
                      size_t len);
```

Parses `data[0..len)` as URL-encoded form data, decoding each name and value exactly once using `fio_string_write_url_dec`.

**Parameters:**
- `callbacks` - borrowed callback table, or `NULL` for no-op callbacks; valid for the call.
- `udata` - caller-owned context passed to callbacks, possibly `NULL`; updated from each `on_pair` return value. A real `on_pair` callback must return non-NULL to continue.
- `data` - borrowed readable buffer containing at least `len` bytes; not modified. A trailing NUL is not required.
- `len` - number of input bytes to parse; may be zero.

**Returns:** parse result containing original input bytes consumed, pairs reported, and error status. Allocation failure stops parsing without reporting the failed pair; callers may retry from `data + result.consumed` after resolving the allocation failure. Retrying from the beginning repeats earlier callback side effects.

Rules from the implementation:
- pairs are separated by `&`
- the first `=` in a pair separates name and value
- `name=` is valid and has an empty value
- `name` is valid and has an empty value
- `=value` is valid and has an empty name
- empty pairs such as leading `&` or `&&` are skipped
- pair boundaries are determined before decoding, so encoded `&` and `=` remain part of their fields
- percent escapes are decoded and `+` becomes a space in both names and values
- `%2B` decodes to a literal `+`, not a space; `%2526` decodes to `%26`, not `&`
- invalid or incomplete percent escapes remain literal, following `fio_string_write_url_dec` (including its `%uXXXX` extension)

**Ownership:** the caller retains `data`, `callbacks`, and `udata`. The parser owns temporary decoded buffers and releases any heap fallback using `FIO_STRING_FREE2` after the callback returns, including when the callback stops parsing. No buffer ownership is transferred to callbacks.

**Thread-safety:** independent calls are reentrant and share no parser state; callback state and any configured allocator must support the caller's concurrency requirements. Work is linear in the input length, with temporary space proportional to the largest pair.

**Note**: callbacks now receive decoded data rather than slices of the encoded input. Remove caller-side URL decoding to avoid decoding twice.

### Example

```c
#define FIO_URL_ENCODED
#include "fio-stl.h"
#include <stdio.h>
#include <string.h>

static void *on_pair(void *udata, fio_buf_info_s name, fio_buf_info_s value) {
  printf("%.*s = %.*s\n",
         (int)name.len, name.buf,
         (int)value.len, value.buf);
  return udata;
}

int main(void) {
  const char form[] = "name=Bo&empty=&encoded=a%20b&&solo";
  static const fio_url_encoded_parser_callbacks_s cb = {
      .on_pair = on_pair,
  };

  fio_url_encoded_result_s r =
      fio_url_encoded_parse(&cb, (void *)1, form, strlen(form));

  /* The callback sees encoded=a b, not encoded=a%20b. */
  if (r.err) {
    fprintf(stderr, "Parsing stopped after %zu bytes\n", r.consumed);
    return 1;
  }
  printf("pairs=%zu consumed=%zu\n", r.count, r.consumed);
  return 0;
}
```

---
