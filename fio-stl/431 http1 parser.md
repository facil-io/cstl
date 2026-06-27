# HTTP/1.x Parser

```c
#define FIO_HTTP1_PARSER
#include FIO_INCLUDE_FILE
```

Small parser, sharp teeth. Define `FIO_HTTP1_PARSER` to add the static
HTTP/1.x request / response parser used by the HTTP layer. It performs no heap
allocations, stores only parser state, and reports parsed data through callbacks
implemented by the including translation unit.

Nearby context: [IO and HTTP overview](./400 io-overview.md), the higher-level
[HTTP module header](./439 http.h), and the neighboring
[WebSocket parser header](./431 websocket parser.h).

---

## What Gets Added

`FIO_HTTP1_PARSER` exposes:

- `fio_http1_parser_s` — parser state.
- `FIO_HTTP1_PARSER_INIT` — zero-initializer / reset value.
- `fio_http1_parse` — incremental parser entry point.
- parser state helpers:
  - `fio_http1_parser_is_empty`
  - `fio_http1_parser_is_on_header`
  - `fio_http1_parser_is_on_body`
  - `fio_http1_expected`
- parse result / expected-body constants:
  - `FIO_HTTP1_PARSER_ERROR`
  - `FIO_HTTP1_EXPECTED_CHUNKED`
- required user callbacks named `fio_http1_on_*`.

The implementation also declares internal parsing stages named with
`fio_http1___...`; these are private implementation details.

---

## Parser State

### `fio_http1_parser_s`

```c
typedef struct fio_http1_parser_s fio_http1_parser_s;

struct fio_http1_parser_s {
  int (*fn)(fio_http1_parser_s *, fio_buf_info_s *, void *);
  size_t expected;
};
```

The parser state is intentionally tiny: one function pointer for the current
state-machine stage and one `expected` byte counter / sentinel.

Treat both fields as opaque. Allocate the struct wherever it fits your lifetime
(stack, connection object, arena, etc.), initialize it with
`FIO_HTTP1_PARSER_INIT`, and use the helper functions to inspect state.

### `FIO_HTTP1_PARSER_INIT`

```c
#define FIO_HTTP1_PARSER_INIT ((fio_http1_parser_s){0})
```

Zero-initializes a parser:

```c
fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
```

The parser also resets itself to this empty state after a complete message is
reported with `fio_http1_on_complete`.

---

## Parsing API

### `fio_http1_parse`

```c
FIO_SFUNC size_t fio_http1_parse(fio_http1_parser_s *p,
                                 fio_buf_info_s buf,
                                 void *udata);
```

Parses as much HTTP/1.x data as currently possible and invokes callbacks as
fields are discovered.

- `p` is the parser state.
- `buf` is the current readable bytes.
- `udata` is passed unchanged to every callback.

Returns the number of bytes consumed from `buf`, or
`FIO_HTTP1_PARSER_ERROR` (`(size_t)-1`) on parse / callback error.

A successful return may consume fewer bytes than supplied. Any unconsumed bytes
belong to a later parse step or to the next HTTP message on a keep-alive
connection.

If the parser needs more bytes before it can make progress, it returns
successfully with the bytes consumed so far, which can be `0`.

### State Helpers

```c
FIO_IFUNC size_t fio_http1_parser_is_empty(fio_http1_parser_s *p);
FIO_IFUNC size_t fio_http1_parser_is_on_header(fio_http1_parser_s *p);
FIO_IFUNC size_t fio_http1_parser_is_on_body(fio_http1_parser_s *p);
FIO_IFUNC size_t fio_http1_expected(fio_http1_parser_s *p);
```

- `fio_http1_parser_is_empty` returns non-zero when the parser is waiting for a
  new request / response line.
- `fio_http1_parser_is_on_header` returns non-zero while reading regular
  headers or chunked trailer headers.
- `fio_http1_parser_is_on_body` returns non-zero while reading a known-length
  body or while the chunked parser is ready to read the next chunk frame. During
  a split chunk payload, the internal chunk-read stage may report false even
  though body bytes are still being drained.
- `fio_http1_expected` returns the parser's current expected byte count, returns
  `FIO_HTTP1_EXPECTED_CHUNKED` after a chunked body is detected and before the
  next chunk-size line is parsed, and returns `0` when the parser's internal
  state marks the message as having no body. During chunked payload delivery,
  it may expose the current chunk size / remaining chunk bytes.

### Constants

```c
#define FIO_HTTP1_PARSER_ERROR ((size_t)-1)
#define FIO_HTTP1_EXPECTED_CHUNKED ((size_t)(-2))
```

`FIO_HTTP1_PARSER_ERROR` is the error return value from `fio_http1_parse`.
After a parse error, close / discard the stream state rather than attempting to
recover the same parser instance.

`FIO_HTTP1_EXPECTED_CHUNKED` is the parser's sentinel after
`transfer-encoding: chunked` is accepted and before a chunk-size line is parsed.
Once chunk parsing starts, `fio_http1_expected` may instead report the current
chunk size or remaining chunk bytes.

The header also defines `FIO___HTTP1_BODY_NOT_ALLOWED` as an internal sentinel
for methods / states where a body should not be read. User code should rely on
`fio_http1_expected(p) == 0` instead of using that internal macro.

---

## Callback Contract

The parser declares these callbacks as `static` prototypes. The including
translation unit must define them.

For portable user code, every `int` callback should return only `0` to continue
or `-1` to reject the parse.

The parser's internal checks are not identical for every callback:

- `fio_http1_on_method`, `fio_http1_on_url`, `fio_http1_on_version`,
  `fio_http1_on_status`, and `fio_http1_on_body_chunk` treat any non-zero return
  as a parse error.
- `fio_http1_on_header` during normal headers and
  `fio_http1_on_header_content_length` reject only an exact `-1` return.
- `fio_http1_on_header` during chunked trailers is passed through; negative
  values become parse errors and positive values stop the current parse as
  incomplete.
- `fio_http1_on_expect` is special: any non-zero return rejects the expectation,
  resets the parser, and stops the current parse without calling
  `fio_http1_on_complete`.

All `fio_buf_info_s` values point into the `buf` memory passed to
`fio_http1_parse`. They are not NUL-terminated unless the input happened to be.
Copy or retain the data before the callback returns if it must outlive the input
buffer.

> Important: the parser lowercases header names in-place. Feed it writable
> memory, not a string literal or read-only mapping.

### Completion

```c
static void fio_http1_on_complete(void *udata);
```

Called after the request / response line, headers, and any body have been fully
parsed. The parser is reset before this callback is invoked, so it is ready for
the next message on the same connection.

### Request Line Callbacks

```c
static int fio_http1_on_method(fio_buf_info_s method, void *udata);
static int fio_http1_on_url(fio_buf_info_s path, void *udata);
static int fio_http1_on_version(fio_buf_info_s version, void *udata);
```

For request lines, the parser calls the callbacks in this order:

1. `fio_http1_on_method`
2. `fio_http1_on_url`
3. `fio_http1_on_version`

The version slice is clamped to at most 14 bytes. `GET`, `HEAD`, and `OPTIONS`
are recognized case-insensitively and mark the parser as not expecting a body;
body-bearing headers for these methods conflict with that marker and are
rejected.

### Response Line Callbacks

```c
static int fio_http1_on_version(fio_buf_info_s version, void *udata);
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata);
```

For response lines, the parser calls `fio_http1_on_version` first and then
`fio_http1_on_status`.

`istatus` is parsed from the numeric status token. `status` is the remaining
status text slice after the numeric token. The version slice is clamped to at
most 14 bytes.

The parser decides whether the first line is a response by checking whether the
second token starts with a decimal digit.

### Headers

```c
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata);

static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata);
```

For ordinary headers, `fio_http1_on_header` receives:

- `name` lowercased in-place.
- `value` trimmed of leading and trailing spaces / tabs.
- an empty value as `{ .buf = NULL, .len = 0 }`.

Header names must contain a valid `:` separator and may not contain the
forbidden characters encoded by the parser. NUL bytes in header values are
rejected.

`content-length` is special:

- empty values are rejected;
- non-decimal / overflowing values are rejected;
- values colliding with internal sentinels are rejected;
- duplicate `content-length` headers must agree;
- conflicting `content-length` and final `transfer-encoding: chunked` are
  rejected;
- the first non-zero accepted value calls `fio_http1_on_header_content_length`
  instead of the generic header callback; a repeated matching value is accepted
  without calling the content-length callback again.

A `content-length: 0` value marks the message as having no body and does not
call `fio_http1_on_header_content_length`.

`transfer-encoding` is also special when its final token is `chunked`
(case-insensitive):

- the parser switches to chunked body decoding;
- if the value is exactly `chunked`, no generic header callback is made;
- if other transfer-coding text appears before the final `chunked` token, the
  final `chunked` token and adjacent separators are stripped before the
  remaining value is passed to `fio_http1_on_header`;
- malformed separators before the final `chunked` token are rejected.

`expect` is special when its value is exactly `100-continue`. Any other
`Expect` value is rejected.

### Expect: 100-continue

```c
static int fio_http1_on_expect(void *udata);
```

Called after headers when an accepted `Expect: 100-continue` header requires a
post-header decision and the parser has a non-zero body expectation marker.
Return `0` to continue into the body / completion flow. Return non-zero to reset
the parser and stop the current parse without calling `fio_http1_on_complete`.

### Body Chunks

```c
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata);
```

Called with decoded body bytes.

For `Content-Length` bodies, callback chunks follow the supplied input chunks
and sum to the accepted content length.

For chunked bodies, framing bytes are removed before callback delivery. Large or
split HTTP chunks may be delivered through more than one callback if the input
arrives in smaller pieces.

---

## Parser Flow

```text
start line
  ├─ request  -> method -> url -> version
  └─ response -> version -> status
headers
  ├─ no body / body not allowed -> complete
  ├─ content-length body        -> body chunks -> complete
  └─ chunked body               -> chunk chunks -> trailers -> complete
```

Details worth keeping in mind:

- Leading spaces, `\r`, and `\n` before the first line are skipped.
- First lines shorter than the parser's minimum accepted shape are rejected.
- NUL bytes in the first line are rejected.
- Header and first-line parsing waits for a newline before making progress.
- The parser accepts `\n` line endings and handles an optional preceding `\r`.
- Chunk size lines are hexadecimal, capped by the implementation, and do not
  support chunk extensions.
- A zero-size chunk either completes immediately when followed by an empty line
  or enters trailer parsing.

---

## Chunked Trailers

Allowed trailer headers are reported through `fio_http1_on_header` after the
body's terminating zero-size chunk.

The parser rejects the following trailer names:

- `authorization`
- `cache-control`
- `content-encoding`
- `content-length`
- `content-range`
- `content-type`
- `expect`
- `host`
- `max-forwards`
- `set-cookie`
- `te`
- `trailer`
- `transfer-encoding`

---

## Ownership and Lifetime

- The parser allocates no memory.
- The parser does not copy callback data.
- The parser mutates header names in the input buffer to lowercase.
- Callback slices are valid only while the input buffer remains valid and
  unchanged.
- `udata` is never owned by the parser; it is simply forwarded.
- The parser state may live inside a connection object and be reused for
  keep-alive messages. It resets automatically on complete messages.
- After `FIO_HTTP1_PARSER_ERROR`, discard the parser / connection state.

---

## Minimal Skeleton

```c
#define FIO_HTTP1_PARSER
#include FIO_INCLUDE_FILE

static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  (void)method;
  (void)udata;
  return 0;
}

static int fio_http1_on_url(fio_buf_info_s path, void *udata) {
  (void)path;
  (void)udata;
  return 0;
}

static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  (void)version;
  (void)udata;
  return 0;
}

static int fio_http1_on_status(size_t status_code,
                               fio_buf_info_s status,
                               void *udata) {
  (void)status_code;
  (void)status;
  (void)udata;
  return 0;
}

static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  (void)name;
  (void)value;
  (void)udata;
  return 0;
}

static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata) {
  (void)name;
  (void)value;
  (void)udata;
  return content_length > (1UL << 20) ? -1 : 0;
}

static int fio_http1_on_expect(void *udata) {
  (void)udata;
  return 0;
}

static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  (void)chunk;
  (void)udata;
  return 0;
}

static void fio_http1_on_complete(void *udata) {
  (void)udata;
}

size_t parse_some_http(char *data, size_t len, void *udata) {
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  return fio_http1_parse(&parser, FIO_BUF_INFO2(data, len), udata);
}
```

For real incremental parsing, keep `fio_http1_parser_s` with the connection and
preserve / retry unconsumed bytes when `fio_http1_parse` returns less than the
available buffer length.
