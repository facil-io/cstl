# MIME Multipart Parser

```c
#define FIO_MULTIPART
#include "fio-stl.h"
```

A non-allocating, callback-based `multipart/form-data` parser. It reports each part once, complete, with `Content-Disposition` and `Content-Type` extracted: a regular field calls `on_string`, a file calls `on_file`. Implemented in [`./004 multipart.h`](./004%20multipart.h).

The parser doesn't decode transfer encodings or unescape headers. It slices the original data into `fio_buf_info_s` ranges (no copies), borrowed only during the callback.

Data can be parsed all at once or fed block by block. The parser state keeps a copy of the boundary and how far the pending part was searched, so bytes are never searched twice and only one (incomplete) part is kept in memory at a time.

### Types

#### `fio_multipart_parser_callbacks_s`

```c
typedef struct {
  int (*on_string)(void *udata,
                   fio_buf_info_s key,
                   fio_buf_info_s value,
                   fio_buf_info_s content_type);
  int (*on_file)(void *udata,
                 fio_buf_info_s key,
                 fio_buf_info_s filename,
                 fio_buf_info_s content_type,
                 fio_buf_info_s headers,
                 fio_buf_info_s data);
} fio_multipart_parser_callbacks_s;
```

Callback table. Both callbacks are optional: a NULL callback is replaced by a no-op that skips its parts, which are still validated.

**Members:**
- `on_string` - a complete regular form field (no `filename` attribute).
- `on_file` - a complete file part: any part with a `filename` attribute, including `filename=""` (non-NULL `filename.buf`, zero `filename.len`). `headers` holds the raw part headers, excluding the terminating `\r\n\r\n`; `data` holds the whole file.

A missing `Content-Type` has a NULL `buf`. All arguments are borrowed only during the call. A non-zero return value aborts parsing and is returned unchanged in `err` (opaque, like the JSON parser's callback errors). Incomplete, truncated or malformed parts never reach a callback.

#### `fio_multipart_s`

```c
typedef struct { /* private */ } fio_multipart_s;
```

The parser state, set by `fio_multipart_init`. It holds a copy of the boundary (so the caller's boundary needn't stay valid) and how far the pending part was searched. Not thread-safe; callbacks must not use the same parser.

#### `fio_multipart_result_s`

```c
typedef struct {
  size_t consumed;
  int err;
} fio_multipart_result_s;
```

**Members:**
- `consumed` - bytes consumed: complete parts (and, once complete, all remaining data). Without an error, `consumed == 0` means more data is required.
- `err` - `0`, `FIO_MULTIPART_ERROR_GENERIC` (`-1`, malformed or truncated input) or an unchanged callback error.

### API Functions

#### `fio_multipart_init`

```c
SFUNC int fio_multipart_init(fio_multipart_s *parser, fio_buf_info_s boundary);
```

Initializes (or resets) `parser` for `boundary` (the `Content-Type` boundary value, **without** the leading `"--"`), copying it. Returns `-1` if the boundary is empty or longer than `FIO_MULTIPART_MAX_BOUNDARY` (250); the parser then fails every call.

#### `fio_multipart_parse`

```c
SFUNC fio_multipart_result_s
fio_multipart_parse(fio_multipart_s *parser,
                    const fio_multipart_parser_callbacks_s *callbacks,
                    void *udata,
                    const char *data,
                    size_t len,
                    int eof);
```

Parses `data`, reporting every part it completes.

- **All data at once:** call with `eof = 1`. `err == 0` means the body was complete and every part was reported.
- **Block by block:** call with `eof = 0`, keep `data + consumed ... data + len`, append the next block and call again with the same `parser`. Call with `eof = 1` once the input ends. `data` must always start at the first unconsumed byte (if it's shorter than what was already searched, the pending part is searched again).

**Framing rules:**
- The body starts with an optional `CRLF` and the first delimiter (no preamble).
- A part is reported only after the delimiter line that ends it is validated: either `CRLF` (another part follows) or the closing `--`, optional spaces / tabs (transport padding), then `CRLF` or the end of the input (`eof`). Anything else is an error.
- Without `eof`, a closing line reaching the end of `data` waits for more data (its part stays pending), so a closing line split across blocks is validated, not trusted.
- After the closing line, the rest of the data (the epilogue) is consumed and ignored, by this and later calls.
- Part headers longer than `FIO_MULTIPART_MAX_HEADER_SIZE` (default 8192 bytes), closing transport padding longer than that, and parts without a `Content-Disposition` header are errors. Only the spaces/tabs following the closing delimiter's two hyphens count as padding; the hyphens and optional trailing `CRLF` do not.
- At `eof`, anything incomplete is an error (truncation).
- After an error, every call fails.

### Example

```c
#define FIO_MULTIPART
#include "fio-stl.h"

static int on_string(void *u, fio_buf_info_s k, fio_buf_info_s v,
                     fio_buf_info_s ct) {
  (void)u; (void)ct;
  printf("field %.*s = %.*s\n", (int)k.len, k.buf, (int)v.len, v.buf);
  return 0;
}
static int on_file(void *u, fio_buf_info_s k, fio_buf_info_s fn,
                   fio_buf_info_s ct, fio_buf_info_s headers,
                   fio_buf_info_s data) {
  (void)u; (void)ct; (void)headers;
  printf("file %.*s / %.*s: %zu bytes\n", (int)k.len, k.buf, (int)fn.len,
         fn.buf, data.len);
  return 0; /* non-zero aborts and is returned in `err` */
}

int main(void) {
  static const fio_multipart_parser_callbacks_s cb = {
    .on_string = on_string,
    .on_file = on_file,
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
  fio_multipart_s parser;
  if (fio_multipart_init(&parser, FIO_BUF_INFO2((char *)"B", 1)))
    return 1;
  fio_multipart_result_s r =
      fio_multipart_parse(&parser, &cb, NULL, body, strlen(body), 1);
  printf("err=%d\n", r.err);
  return 0;
}
```

------------------------------------------------------------
