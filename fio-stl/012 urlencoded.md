## URL-Encoded Parser

```c
#define FIO_URL_ENCODED
#include "fio-stl.h"
```

A callback-based parser for `application/x-www-form-urlencoded` data. It splits `name=value&...` pairs and decodes each name and value before invoking the callback, without modifying the input. Temporary buffers use stack storage with heap fallback for long fields. Implemented in [`./012 urlencoded.h`](./012%20urlencoded.h).

### Types

#### `fio_url_encoded_parser_callbacks_s`

```c
typedef struct {
  int (*on_pair)(void *udata, fio_buf_info_s name, fio_buf_info_s value);
} fio_url_encoded_parser_callbacks_s;
```

Parser callbacks.

**Members:**
- `on_pair` - called for each non-empty pair with the decoded `name` and `value`. Buffers are borrowed and valid **only during the callback**; copy anything needed later and do not free them. Lengths are in bytes and may include embedded NUL bytes. Return `0` to continue. Any other value stops parsing and is returned unchanged in `result.err`.

If `callbacks` or `on_pair` is `NULL`, the parser only counts pairs: nothing is decoded and nothing is allocated.

#### `fio_url_encoded_result_s`

```c
typedef struct {
  size_t consumed;
  size_t count;
  int err;
} fio_url_encoded_result_s;
```

Parse result. On error, `consumed` and `count` cover only the pairs processed without error; the failed pair is excluded from both.

**Members:**
- `consumed` - number of input bytes processed without error, including skipped empty pairs and separators. On success this equals `len`. On error, it is the offset where the failed pair starts.
- `count` - number of non-empty pairs processed without error.
- `err` - `0` on success, `-1` if a temporary buffer could not be allocated, or the non-zero value returned by `on_pair`.

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
- `callbacks` - borrowed callback table, valid for the call. `NULL` (or a `NULL` `on_pair`) selects counting-only mode.
- `udata` - caller-owned context passed unchanged to `on_pair`; may be `NULL`.
- `data` - borrowed readable buffer containing at least `len` bytes; not modified. A trailing NUL is not required. May be `NULL` only when `len` is zero.
- `len` - number of input bytes to parse; may be zero. Empty input returns an all-zero result without invoking callbacks.

**Returns:** the bytes consumed, the pairs processed, and the error. An error stops parsing before the failed pair, so `data + result.consumed` is where that pair starts. Retrying from the beginning repeats earlier callback side effects.

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
- the non-standard `%uXXXX` extension is also decoded, following `fio_string_write_url_dec`
- invalid or incomplete percent escapes remain literal

**Note**: this is a permissive decoder, not a strict form-encoding validator. Perform security checks on the decoded names and values. A separate filter that only decodes `%XX` will interpret `%u0061dmin` differently: this parser decodes it to `admin`.

**Ownership:** the caller retains `data`, `callbacks`, and `udata`. The parser owns temporary decoded buffers and releases any heap fallback using `FIO_STRING_FREE2` after the callback returns, including when the callback stops parsing. No buffer ownership is transferred to callbacks.

**Thread-safety:** independent calls are reentrant and share no parser state; callback state and any configured allocator must support the caller's concurrency requirements. Work is linear in the input length, with temporary space proportional to the largest pair.

### Example

```c
#define FIO_URL_ENCODED
#include "fio-stl.h"
#include <stdio.h>
#include <string.h>

static int on_pair(void *udata, fio_buf_info_s name, fio_buf_info_s value) {
  printf("%.*s = %.*s\n",
         (int)name.len, name.buf,
         (int)value.len, value.buf);
  (void)udata;
  return 0; /* non-zero stops parsing and becomes r.err */
}

int main(void) {
  const char form[] = "name=Bo&empty=&encoded=a%20b&&solo";
  static const fio_url_encoded_parser_callbacks_s cb = {
      .on_pair = on_pair,
  };

  fio_url_encoded_result_s r =
      fio_url_encoded_parse(&cb, NULL, form, strlen(form));

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
