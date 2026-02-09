## MIME Multipart Parser

```c
#define FIO_MULTIPART
#include FIO_INCLUDE_FILE
```

By defining `FIO_MULTIPART`, a non-allocating, streaming, callback-based MIME multipart parser is defined and made available. This module implements the `multipart/form-data` format as used in HTTP file uploads ([RFC 7578](https://tools.ietf.org/html/rfc7578)).

The parser uses callbacks to handle form fields and file uploads:

- **Regular form fields** (no filename) trigger the `on_field` callback, or optionally the streaming `on_field_start` / `on_field_data` / `on_field_end` callbacks
- **File uploads** trigger `on_file_start`, `on_file_data`, `on_file_end` callbacks
- **Streaming** - call `fio_multipart_parse` repeatedly with accumulated data; the result reports how many bytes were consumed
- **Non-allocating** - the parser does not allocate memory; all data is passed as `fio_buf_info_s` references into the original buffer

### MIME Multipart Types

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

The MIME multipart parser callback collection. All callbacks are optional; unset callbacks are replaced with internal no-ops.

**Members:**

- `on_field` - called for each regular form field (no filename). Receives the field `name`, `value`, and optional `content_type`. Returns a user-defined context (can be NULL). **Ignored** if `on_field_start` is provided.
- `on_field_start` - called when a large/streaming field starts (optional). If provided, the streaming field callbacks (`on_field_start` / `on_field_data` / `on_field_end`) are used instead of `on_field`. Returns a context pointer for this field.
- `on_field_data` - called with a field data chunk. May be called multiple times per field for streaming. Returns non-zero to abort parsing.
- `on_field_end` - called when a field ends.
- `on_file_start` - called when a file upload starts. Receives the field `name`, the `filename`, and the `content_type`. Returns a context pointer for this file.
- `on_file_data` - called with a file data chunk. May be called multiple times per file for streaming. Returns non-zero to abort parsing.
- `on_file_end` - called when a file upload ends.
- `on_error` - called on parse error (optional).

**Note**: if `on_field_start` is provided, `on_field` is ignored and the streaming field callbacks are used. This enables handling of large field values without buffering the entire value in memory.

#### `fio_multipart_result_s`

```c
typedef struct {
  size_t consumed;    /* Number of bytes consumed from the input buffer. */
  size_t field_count; /* Number of form fields parsed. */
  size_t file_count;  /* Number of files parsed. */
  int err;            /* Error code: 0 = success, -1 = error, -2 = need more data. */
} fio_multipart_result_s;
```

The MIME multipart parse result type, returned by `fio_multipart_parse`.

**Members:**

- `consumed` - number of bytes consumed from the input buffer. When streaming, pass the remaining unconsumed data (plus any new data) in the next call.
- `field_count` - number of regular form fields successfully parsed in this call.
- `file_count` - number of file uploads successfully parsed in this call.
- `err` - error code:
  - `0` - success, all data was parsed and the closing boundary was found.
  - `-1` - parse error (malformed data, missing boundary, missing `Content-Disposition`, or callback aborted).
  - `-2` - need more data; call again with remaining unconsumed data plus additional data.

### MIME Multipart Functions

#### `fio_multipart_parse`

```c
fio_multipart_result_s
fio_multipart_parse(const fio_multipart_parser_callbacks_s *callbacks,
                    void *udata,
                    fio_buf_info_s boundary,
                    const char *data,
                    size_t len);
```

Parses MIME multipart data, invoking callbacks for each form field and file upload encountered.

The parser scans `data` for multipart boundaries, extracts headers (`Content-Disposition`, `Content-Type`), and dispatches to the appropriate callbacks. Parts with a `filename` parameter trigger the file callbacks; parts without trigger the field callbacks.

**Parameters:**

- `callbacks` - pointer to the callback function collection (should typically be `static const`). May be NULL, in which case all callbacks are no-ops.
- `udata` - user data pointer passed to all callbacks.
- `boundary` - the multipart boundary string **without** the leading `"--"` prefix. Must not exceed 250 bytes.
- `data` - pointer to the data to parse.
- `len` - length of the data in bytes.

**Returns:** a `fio_multipart_result_s` struct with the number of bytes consumed, field/file counts, and an error code.

**Note**: for streaming use, check `result.err`:
- If `0`, parsing is complete (closing boundary found).
- If `-2`, more data is needed. Retain the unconsumed portion (`data + result.consumed` through `data + len`) and append new data before calling again.
- If `-1`, an unrecoverable parse error occurred.

**Note**: the boundary length is limited to 250 bytes. Boundaries exceeding this limit cause an immediate error (`err = -1`).

### MIME Multipart Examples

#### Basic Field and File Parsing

```c
#define FIO_MULTIPART
#include FIO_INCLUDE_FILE

static void *my_on_field(void *udata,
                         fio_buf_info_s name,
                         fio_buf_info_s value,
                         fio_buf_info_s content_type) {
  printf("Field: %.*s = %.*s\n",
         (int)name.len, name.buf,
         (int)value.len, value.buf);
  (void)content_type;
  return NULL;
}

static void *my_on_file_start(void *udata,
                              fio_buf_info_s name,
                              fio_buf_info_s filename,
                              fio_buf_info_s content_type) {
  printf("File upload: %.*s (name=%.*s, type=%.*s)\n",
         (int)filename.len, filename.buf,
         (int)name.len, name.buf,
         (int)content_type.len, content_type.buf);
  return NULL;
}

static int my_on_file_data(void *udata,
                           void *file_ctx,
                           fio_buf_info_s data) {
  printf("  File data chunk: %zu bytes\n", data.len);
  return 0; /* return non-zero to abort */
}

static void my_on_file_end(void *udata, void *file_ctx) {
  printf("  File upload complete\n");
}

void example_multipart(const char *body, size_t body_len,
                       const char *boundary, size_t boundary_len) {
  static const fio_multipart_parser_callbacks_s callbacks = {
      .on_field = my_on_field,
      .on_file_start = my_on_file_start,
      .on_file_data = my_on_file_data,
      .on_file_end = my_on_file_end,
  };

  fio_multipart_result_s result = fio_multipart_parse(
      &callbacks,
      NULL, /* udata */
      FIO_BUF_INFO2((char *)boundary, boundary_len),
      body,
      body_len);

  if (result.err == 0) {
    printf("Parsed %zu fields and %zu files\n",
           result.field_count, result.file_count);
  } else if (result.err == -2) {
    printf("Need more data (consumed %zu of %zu bytes)\n",
           result.consumed, body_len);
  } else {
    printf("Parse error\n");
  }
}
```

#### Streaming Field Callbacks

```c
static void *my_field_start(void *udata,
                            fio_buf_info_s name,
                            fio_buf_info_s content_type) {
  printf("Field started: %.*s\n", (int)name.len, name.buf);
  return NULL;
}

static int my_field_data(void *udata,
                         void *field_ctx,
                         fio_buf_info_s data) {
  printf("  Field data: %zu bytes\n", data.len);
  return 0;
}

static void my_field_end(void *udata, void *field_ctx) {
  printf("  Field complete\n");
}

void example_streaming_fields(const char *body, size_t body_len,
                              const char *boundary, size_t boundary_len) {
  /* When on_field_start is set, on_field is ignored */
  static const fio_multipart_parser_callbacks_s callbacks = {
      .on_field_start = my_field_start,
      .on_field_data = my_field_data,
      .on_field_end = my_field_end,
      .on_file_start = my_on_file_start,
      .on_file_data = my_on_file_data,
      .on_file_end = my_on_file_end,
  };

  fio_multipart_result_s result = fio_multipart_parse(
      &callbacks,
      NULL,
      FIO_BUF_INFO2((char *)boundary, boundary_len),
      body,
      body_len);

  if (result.err == -2) {
    /* Retain unconsumed data and append new data before retrying */
    size_t remaining = body_len - result.consumed;
    /* ... accumulate more data, then call fio_multipart_parse again */
  }
}
```

### Multipart Format Reference

The `multipart/form-data` format expected by the parser:

```
--boundary\r\n
Content-Disposition: form-data; name="field1"\r\n
\r\n
value1\r\n
--boundary\r\n
Content-Disposition: form-data; name="file1"; filename="test.txt"\r\n
Content-Type: text/plain\r\n
\r\n
file content here\r\n
--boundary--\r\n
```

- Each part starts with `--` followed by the boundary string and `\r\n`
- Part headers are separated from the body by a blank line (`\r\n\r\n`)
- Parts with a `filename` parameter are treated as file uploads
- Parts without a `filename` parameter are treated as regular form fields
- The final boundary ends with `--` (e.g., `--boundary--`)

-------------------------------------------------------------------------------
