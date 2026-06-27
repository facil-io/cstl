# MIME Multipart Parser

```c
#define FIO_MULTIPART
#include "fio-stl.h"
```

A non-allocating, streaming, callback-based `multipart/form-data` parser. It finds parts, extracts `Content-Disposition` and `Content-Type`, then calls your callbacks for each field or file chunk. Implemented in [`./004 multipart.h`](./004%20multipart.h).

The parser does not decode transfer encodings or unescape headers — it just slices the original buffer into `fio_buf_info_s` ranges.

### Types

#### `fio_multipart_parser_callbacks_s`

```c
typedef struct {
  void *(*on_field)(void *udata,
                    fio_buf_info_s name,
                    fio_buf_info_s value,
                    fio_buf_info_s content_type);
  void *(*on_field_start)(void *udata,
                          fio_buf_info_s name,
                          fio_buf_info_s content_type);
  int (*on_field_data)(void *udata, void *field_ctx, fio_buf_info_s data);
  void (*on_field_end)(void *udata, void *field_ctx);
  void *(*on_file_start)(void *udata,
                         fio_buf_info_s name,
                         fio_buf_info_s filename,
                         fio_buf_info_s content_type);
  int (*on_file_data)(void *udata, void *file_ctx, fio_buf_info_s data);
  void (*on_file_end)(void *udata, void *file_ctx);
  void (*on_error)(void *udata);
} fio_multipart_parser_callbacks_s;
```

Callback table. All callbacks are optional; unset ones become no-ops.

**Members:**
- `on_field` - called for regular form fields (no `filename`). Ignored if `on_field_start` is provided.
- `on_field_start` - optional streaming field start. If set, `on_field` is ignored and `on_field_data` / `on_field_end` are used instead.
- `on_field_data` - called with a field data chunk. Return non-zero to abort.
- `on_field_end` - called when a streaming field ends.
- `on_file_start` - called when a file upload starts. Returns a context pointer for this file.
- `on_file_data` - called with a file data chunk. Return non-zero to abort.
- `on_file_end` - called when a file upload ends.
- `on_error` - called on parse error (optional).

#### `fio_multipart_result_s`

```c
typedef struct {
  size_t consumed;
  size_t field_count;
  size_t file_count;
  int err;
} fio_multipart_result_s;
```

Parse result.

**Members:**
- `consumed` - bytes consumed from the input buffer
- `field_count` - regular form fields parsed in this call
- `file_count` - file uploads parsed in this call
- `err` - `0` success, `-1` error, `-2` need more data

### API Functions

#### `fio_multipart_parse`

```c
SFUNC fio_multipart_result_s
fio_multipart_parse(const fio_multipart_parser_callbacks_s *callbacks,
                    void *udata,
                    fio_buf_info_s boundary,
                    const char *data,
                    size_t len);
```

Parses `data` as MIME multipart input. `boundary` is the boundary string **without** the leading `"--"` and must not exceed 250 bytes.

**Parameters:**
- `callbacks` - callback table (typically `static const`)
- `udata` - user data passed to every callback
- `boundary` - boundary value from the `Content-Type` header
- `data` - input buffer
- `len` - input length in bytes

**Returns:** parse result.

**Streaming behavior:**
- `err == 0` - parsing complete; closing boundary found.
- `err == -2` - need more data. Keep `data + result.consumed ... data + len` and append new data before calling again.
- `err == -1` - unrecoverable parse error or callback abort.

### Examples

#### Basic field and file parsing

```c
#define FIO_MULTIPART
#include "fio-stl.h"

static void *on_field(void *u, fio_buf_info_s n, fio_buf_info_s v, fio_buf_info_s ct) {
  (void)u; (void)ct;
  printf("field %.*s = %.*s\n", (int)n.len, n.buf, (int)v.len, v.buf);
  return NULL;
}
static void *on_file_start(void *u, fio_buf_info_s n, fio_buf_info_s fn, fio_buf_info_s ct) {
  (void)u; (void)ct;
  printf("file %.*s / %.*s\n", (int)n.len, n.buf, (int)fn.len, fn.buf);
  return NULL;
}
static int on_file_data(void *u, void *c, fio_buf_info_s d) {
  (void)u; (void)c;
  printf("  chunk %zu bytes\n", d.len);
  return 0;
}
static void on_file_end(void *u, void *c) { (void)u; (void)c; }

int main(void) {
  static const fio_multipart_parser_callbacks_s cb = {
    .on_field = on_field,
    .on_file_start = on_file_start,
    .on_file_data = on_file_data,
    .on_file_end = on_file_end,
  };
  const char *body =
    "--B\r\n"
    "Content-Disposition: form-data; name=\"x\"\r\n\r\n"
    "hello\r\n"
    "--B\r\n"
    "Content-Disposition: form-data; name=\"f\"; filename=\"t.txt\"\r\n"
    "Content-Type: text/plain\r\n\r\n"
    "data\r\n"
    "--B--\r\n";
  fio_multipart_result_s r = fio_multipart_parse(
      &cb, NULL, FIO_BUF_INFO2("B", 1), body, strlen(body));
  printf("fields=%zu files=%zu err=%d\n", r.field_count, r.file_count, r.err);
  return 0;
}
```

#### Streaming field callbacks

```c
static void *field_start(void *u, fio_buf_info_s n, fio_buf_info_s ct) {
  (void)u; (void)ct;
  printf("field start %.*s\n", (int)n.len, n.buf);
  return NULL;
}
static int field_data(void *u, void *c, fio_buf_info_s d) {
  (void)u; (void)c;
  printf("  field chunk %zu bytes\n", d.len);
  return 0;
}
static void field_end(void *u, void *c) { (void)u; (void)c; }

static const fio_multipart_parser_callbacks_s cb_stream = {
  .on_field_start = field_start,
  .on_field_data = field_data,
  .on_field_end = field_end,
};
```

------------------------------------------------------------
