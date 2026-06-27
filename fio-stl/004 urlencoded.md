# URL-Encoded Parser

```c
#define FIO_URL_ENCODED
#include "fio-stl.h"
```

A non-allocating parser for `application/x-www-form-urlencoded` data. It splits `name=value&...` pairs and leaves decoding to you. No copies, no drama. Implemented in [`./004 urlencoded.h`](./004%20urlencoded.h).

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
- `on_pair` - called for each pair. `name` and `value` point into the original input and are **not decoded**. Return non-NULL to continue, or `NULL` to stop.
- `on_error` - optional and currently unused; reserved because parsers enjoy having a rainy-day pocket.

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
- `consumed` - number of input bytes consumed
- `count` - number of non-empty pairs reported
- `err` - non-zero when parsing stopped because `on_pair` returned `NULL`

### Parsing

#### `fio_url_encoded_parse`

```c
SFUNC fio_url_encoded_result_s
fio_url_encoded_parse(const fio_url_encoded_parser_callbacks_s *callbacks,
                      void *udata,
                      const char *data,
                      size_t len);
```

Parses `data[0..len)` as URL-encoded form data.

Rules from the implementation:
- pairs are separated by `&`
- the first `=` in a pair separates name and value
- `name=` is valid and has an empty value
- `name` is valid and has an empty value
- `=value` is valid and has an empty name
- empty pairs such as leading `&` or `&&` are skipped
- percent escapes are not decoded

Use a string helper such as `fio_string_write_url_dec` when you need decoded values.

**Ownership:** callback buffers borrow the original `data`; keep `data` alive while parsing and copy anything you need later.

**Thread-safety:** parsing uses only caller-provided state and stack locals.

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

  printf("pairs=%zu consumed=%zu err=%d\n", r.count, r.consumed, r.err);
  return r.err;
}
```

---
