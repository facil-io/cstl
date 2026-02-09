## HTTP/1.1 Parser

```c
#define FIO_HTTP1_PARSER
#include FIO_INCLUDE_FILE
```

By defining `FIO_HTTP1_PARSER`, a lightweight, zero-allocation HTTP/1.1 parser is defined and made available. The parser is implemented entirely as static functions and uses a callback-driven (SAX-style) design.

The parser handles both **requests** and **responses**, automatically detecting which is being parsed from the first line. It supports:

- **Request parsing** - method, URL, version, headers, and body
- **Response parsing** - version, status code, status text, headers, and body
- **Chunked transfer encoding** - automatic chunk size parsing and reassembly
- **Content-Length bodies** - known-length body reading
- **Trailer headers** - chunked encoding trailers with forbidden-header filtering
- **Expect: 100-continue** - callback notification for flow control
- **Incremental parsing** - feed data in arbitrary chunks; the parser resumes where it left off

**Standalone use**: this parser can be used independently in a separate translation unit from the rest of the HTTP module. To use it standalone, define `FIO_HTTP1_PARSER` and implement the required callback functions (declared as `static` prototypes in the header). The callbacks are **not** provided by the parser - they must be implemented by the user.

### HTTP/1.1 Parser Type

#### `fio_http1_parser_s`

```c
struct fio_http1_parser_s {
  int (*fn)(fio_http1_parser_s *, fio_buf_info_s *, void *);
  size_t expected;
};
```

The HTTP/1.1 parser state type.

The parser is a small state machine containing a function pointer to the current parsing stage and a counter for expected remaining body bytes.

**Members:**
- `fn` - internal function pointer to the current parsing stage (treat as opaque)
- `expected` - number of body bytes still expected, or a sentinel value for chunked/no-body states (treat as opaque)

**Note**: this type should be treated as opaque. Initialize it with `FIO_HTTP1_PARSER_INIT` and interact with it only through the provided API functions.

#### `FIO_HTTP1_PARSER_INIT`

```c
#define FIO_HTTP1_PARSER_INIT ((fio_http1_parser_s){0})
```

Zero-initialization value for the parser.

Use this macro to initialize or reset a parser instance:

```c
fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
```

### HTTP/1.1 Parser API

#### `fio_http1_parse`

```c
size_t fio_http1_parse(fio_http1_parser_s *p,
                       fio_buf_info_s buf,
                       void *udata);
```

Parses HTTP/1.x data, calling the appropriate callbacks as elements are parsed.

Data can be fed incrementally - the parser maintains its state between calls and resumes parsing where it left off. Each call processes as much data as possible from the provided buffer.

**Parameters:**
- `p` - pointer to an initialized `fio_http1_parser_s` parser state
- `buf` - a `fio_buf_info_s` containing the data to parse (pointer and length)
- `udata` - opaque user data pointer passed through to all callbacks

**Returns:** the number of bytes consumed from `buf`, or `FIO_HTTP1_PARSER_ERROR` (`(size_t)-1`) on error.

**Note**: after `fio_http1_on_complete` fires, the parser resets itself. Unconsumed bytes (i.e., `buf.len - returned_value`) may belong to a subsequent HTTP message and should be fed to the parser again.

**Note**: a return value of `0` when `buf.len > 0` means the parser needs more data to make progress (e.g., an incomplete header line).

#### `fio_http1_parser_is_empty`

```c
size_t fio_http1_parser_is_empty(fio_http1_parser_s *p);
```

Returns true (non-zero) if the parser is idle - waiting to parse a new request or response.

This is the case immediately after initialization or after a complete request/response has been parsed and the `fio_http1_on_complete` callback has fired.

**Parameters:**
- `p` - pointer to the parser state

**Returns:** non-zero if the parser is empty/idle, zero otherwise.

#### `fio_http1_parser_is_on_header`

```c
size_t fio_http1_parser_is_on_header(fio_http1_parser_s *p);
```

Returns true (non-zero) if the parser is currently reading header data.

This includes both regular headers and chunked encoding trailer headers.

**Parameters:**
- `p` - pointer to the parser state

**Returns:** non-zero if the parser is in the header-reading stage, zero otherwise.

#### `fio_http1_parser_is_on_body`

```c
size_t fio_http1_parser_is_on_body(fio_http1_parser_s *p);
```

Returns true (non-zero) if the parser is currently reading body data.

This includes both known-length bodies (Content-Length) and chunked transfer encoding bodies.

**Parameters:**
- `p` - pointer to the parser state

**Returns:** non-zero if the parser is in the body-reading stage, zero otherwise.

#### `fio_http1_expected`

```c
size_t fio_http1_expected(fio_http1_parser_s *p);
```

Returns the number of bytes of body payload still expected to be received.

For chunked transfer encoding, returns `FIO_HTTP1_EXPECTED_CHUNKED`. For methods that don't allow a body (GET, HEAD, OPTIONS) or when no body is expected, returns `0`.

**Parameters:**
- `p` - pointer to the parser state

**Returns:** number of remaining body bytes, `FIO_HTTP1_EXPECTED_CHUNKED` for chunked bodies, or `0` if no body is expected.

### HTTP/1.1 Parser Constants

#### `FIO_HTTP1_PARSER_ERROR`

```c
#define FIO_HTTP1_PARSER_ERROR ((size_t)-1)
```

The error return value for `fio_http1_parse`.

When `fio_http1_parse` returns this value, the HTTP data was malformed and the connection should be closed. The parser state is undefined after an error.

#### `FIO_HTTP1_EXPECTED_CHUNKED`

```c
#define FIO_HTTP1_EXPECTED_CHUNKED ((size_t)(-2))
```

A return value for `fio_http1_expected` indicating that the body uses chunked transfer encoding.

When chunked encoding is active, the total body size is not known in advance.

### Callbacks (User-Implemented)

The HTTP/1.1 parser requires the user to implement the following `static` callback functions. These are declared as prototypes in the parser header and **must be defined** in the same translation unit where the parser is included.

All callbacks that return `int` should return `0` on success or `-1` (non-zero) to signal an error and abort parsing.

#### `fio_http1_on_complete`

```c
static void fio_http1_on_complete(void *udata);
```

Called when a complete HTTP request or response has been parsed (all headers and body received).

After this callback fires, the parser automatically resets itself and is ready to parse the next message on the same connection (HTTP keep-alive).

**Parameters:**
- `udata` - the opaque user data pointer passed to `fio_http1_parse`

#### `fio_http1_on_method`

```c
static int fio_http1_on_method(fio_buf_info_s method, void *udata);
```

Called when a request method is parsed (e.g., `GET`, `POST`, `PUT`).

This callback is only called for HTTP requests, not responses.

**Parameters:**
- `method` - a `fio_buf_info_s` containing the method string (not NUL-terminated)
- `udata` - the opaque user data pointer

**Returns:** `0` on success, `-1` to abort parsing.

**Note**: the parser automatically detects GET, HEAD, and OPTIONS methods and marks them as not allowing a body (unless overridden by Content-Length or Transfer-Encoding headers).

#### `fio_http1_on_status`

```c
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata);
```

Called when a response status line is parsed.

This callback is only called for HTTP responses, not requests.

**Parameters:**
- `istatus` - the numeric HTTP status code (e.g., `200`, `404`)
- `status` - a `fio_buf_info_s` containing the status text without the numeric prefix (e.g., `"OK"`, `"Not Found"`)
- `udata` - the opaque user data pointer

**Returns:** `0` on success, `-1` to abort parsing.

#### `fio_http1_on_url`

```c
static int fio_http1_on_url(fio_buf_info_s path, void *udata);
```

Called when a request URL/path is parsed.

This callback is only called for HTTP requests, not responses.

**Parameters:**
- `path` - a `fio_buf_info_s` containing the URL/path string (not NUL-terminated)
- `udata` - the opaque user data pointer

**Returns:** `0` on success, `-1` to abort parsing.

#### `fio_http1_on_version`

```c
static int fio_http1_on_version(fio_buf_info_s version, void *udata);
```

Called when the HTTP version string is parsed (e.g., `HTTP/1.1`).

Called for both requests and responses. For requests, the version appears as the third token on the first line. For responses, it appears as the first token.

**Parameters:**
- `version` - a `fio_buf_info_s` containing the version string (not NUL-terminated, clamped to 14 bytes max)
- `udata` - the opaque user data pointer

**Returns:** `0` on success, `-1` to abort parsing.

#### `fio_http1_on_header`

```c
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata);
```

Called for each parsed header.

Header names are automatically converted to lowercase by the parser. This callback is also used for chunked encoding trailer headers (with forbidden headers filtered out by the parser).

**Parameters:**
- `name` - a `fio_buf_info_s` containing the lowercase header name (not NUL-terminated)
- `value` - a `fio_buf_info_s` containing the header value with leading/trailing whitespace trimmed (not NUL-terminated; `buf` is NULL if value is empty)
- `udata` - the opaque user data pointer

**Returns:** `0` on success, `-1` to abort parsing.

**Note**: the `content-length` and `transfer-encoding: chunked` headers are processed internally by the parser. The `content-length` header is reported through `fio_http1_on_header_content_length` instead of this callback. The `transfer-encoding` header is reported through this callback only if it contains values other than `chunked` (the `chunked` token is stripped).

#### `fio_http1_on_header_content_length`

```c
static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata);
```

Called when the special `content-length` header is parsed.

This callback allows the user to validate or reject the content length (e.g., enforce maximum body size limits).

**Parameters:**
- `name` - a `fio_buf_info_s` containing the header name (`"content-length"`)
- `value` - a `fio_buf_info_s` containing the raw header value string
- `content_length` - the parsed numeric value of the Content-Length header
- `udata` - the opaque user data pointer

**Returns:** `0` to accept the content length, `-1` to reject (aborts parsing).

**Note**: the parser enforces that duplicate `content-length` headers must have the same value (CL.CL attack prevention). It also rejects `content-length` values that collide with internal sentinel values or that overflow.

#### `fio_http1_on_expect`

```c
static int fio_http1_on_expect(void *udata);
```

Called when an `Expect: 100-continue` header is received and all headers have been parsed.

This allows the server to send a `100 Continue` interim response before the client sends the body, or to reject the request with a `417 Expectation Failed` response.

**Parameters:**
- `udata` - the opaque user data pointer

**Returns:** `0` to continue parsing (accept the body), non-zero to stop (the parser resets, no `fio_http1_on_complete` is called).

**Note**: the parser only recognizes the exact value `100-continue` for the `Expect` header. Any other value causes a parse error.

#### `fio_http1_on_body_chunk`

```c
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata);
```

Called for each chunk of body data received.

For Content-Length bodies, this may be called multiple times if data arrives incrementally, with chunks summing to the total content length. For chunked transfer encoding, this is called once per decoded chunk (without the chunk framing).

**Parameters:**
- `chunk` - a `fio_buf_info_s` containing the body data chunk
- `udata` - the opaque user data pointer

**Returns:** `0` on success, `-1` to abort parsing.

### Security Features

The parser includes several built-in protections:

- **CL.CL prevention** - duplicate `content-length` headers with different values cause a parse error
- **CL.TE prevention** - conflicting `content-length` and `transfer-encoding: chunked` headers cause a parse error
- **Forbidden trailer headers** - the following headers are rejected in chunked encoding trailers: `authorization`, `cache-control`, `content-encoding`, `content-length`, `content-range`, `content-type`, `expect`, `host`, `max-forwards`, `set-cookie`, `te`, `trailer`, `transfer-encoding`
- **Header name validation** - forbidden characters in header names cause a parse error
- **Content-Length overflow protection** - values that would overflow or collide with sentinel values are rejected

### Standalone Usage Example

To use the HTTP/1.1 parser independently from the full HTTP module, define the required callbacks in your translation unit:

```c
#define FIO_HTTP1_PARSER
#include FIO_INCLUDE_FILE

/* --- Implement all required callbacks --- */

static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  printf("Method: %.*s\n", (int)method.len, method.buf);
  return 0;
}

static int fio_http1_on_url(fio_buf_info_s path, void *udata) {
  printf("URL: %.*s\n", (int)path.len, path.buf);
  return 0;
}

static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  printf("Version: %.*s\n", (int)version.len, version.buf);
  return 0;
}

static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata) {
  printf("Status: %zu %.*s\n", istatus, (int)status.len, status.buf);
  return 0;
}

static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  printf("Header: %.*s: %.*s\n",
         (int)name.len, name.buf,
         (int)value.len, value.buf);
  return 0;
}

static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata) {
  printf("Content-Length: %zu\n", content_length);
  /* Reject bodies larger than 1MB */
  return (content_length > (1UL << 20)) ? -1 : 0;
}

static int fio_http1_on_expect(void *udata) {
  /* Accept 100-continue */
  return 0;
}

static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  printf("Body chunk: %zu bytes\n", chunk.len);
  return 0;
}

static void fio_http1_on_complete(void *udata) {
  printf("Request/Response complete!\n");
}

/* --- Use the parser --- */

void parse_http_data(char *data, size_t len) {
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  fio_buf_info_s buf = FIO_BUF_INFO2(data, len);
  size_t consumed = fio_http1_parse(&parser, buf, NULL);
  if (consumed == FIO_HTTP1_PARSER_ERROR) {
    printf("Parse error!\n");
    return;
  }
  printf("Consumed %zu of %zu bytes\n", consumed, len);
}
```

### Incremental Parsing Example

The parser supports feeding data in arbitrary-sized chunks:

```c
void example_incremental_parsing(int fd) {
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  char buffer[4096];
  size_t pending = 0;

  for (;;) {
    /* Read more data into buffer after any unconsumed bytes */
    ssize_t nread = read(fd, buffer + pending, sizeof(buffer) - pending);
    if (nread <= 0)
      break;
    pending += nread;

    /* Parse available data */
    fio_buf_info_s buf = FIO_BUF_INFO2(buffer, pending);
    size_t consumed = fio_http1_parse(&parser, buf, NULL);
    if (consumed == FIO_HTTP1_PARSER_ERROR) {
      /* Malformed HTTP - close connection */
      break;
    }

    /* Move unconsumed data to front of buffer */
    if (consumed && consumed < pending) {
      memmove(buffer, buffer + consumed, pending - consumed);
    }
    pending -= consumed;
  }
}
```

------------------------------------------------------------
