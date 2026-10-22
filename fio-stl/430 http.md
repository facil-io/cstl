# HTTP Module

```c
#define FIO_HTTP
#include FIO_INCLUDE_FILE
```

`FIO_HTTP` adds an HTTP/1.x server and client, with WebSocket and SSE
(Server-Sent Events) support, built on the facil.io IO reactor. It also pulls
in the HTTP handle API, the HTTP/1.x parser, and the WebSocket parser.

Related: [IO and HTTP overview](./400 io-overview.md), the
[HTTP/1.x parser](./004 http1 parser.md), the
[WebSocket parser](./004 websocket parser.md), and optional compression
support in [DEFLATE / Gzip](./162 deflate.md).

---

## What Gets Added

`FIO_HTTP` provides:

- **Settings and listeners**: `fio_http_settings_s`, `fio_http_listener_s`,
  `fio_http_listen`, and `fio_http_listener_settings`.
- **Routing**: `fio_http_route`, `fio_http_route_settings`,
  `fio_http_resource_action`, and `fio_http_resource_action_e`.
- **Clients**: `fio_http_connect` and `fio_http_websocket_connect`.
- **Upgrades**: WebSocket and SSE request / response helpers, writers, and
  pub/sub helpers.
- **Handle access**: `fio_http_io` and `fio_http_settings`.
- **The HTTP handle API**: request / response fields, headers, cookies, body
  storage, writing, static-file responses, logging, MIME lookup, and
  controller hooks.

Names prefixed with `fio___` are private and not part of the API.

---

## Connection Model

Each HTTP/1.x request (server) or response (client) is parsed into a
`fio_http_s` handle and passed to the callbacks in `fio_http_settings_s`.

### Request / response cycle

A plain HTTP cycle runs in this order:

1. The request (server) or response (client) is parsed.
2. `pre_http_body` runs before an `Expect: 100-continue` body is read.
3. `on_http` runs the application logic.
4. `fio_http_write` / `fio_http_finish` send the response.
5. `on_finish` runs when the cycle is complete.

A connection handles one request at a time. The next pipelined request is
not read until the current response is finished or the upgrade is complete.

For WebSocket and SSE requests, the server calls the matching authentication
callback instead of `on_http`. After the upgrade, the connection calls
`on_open`, the message / event callbacks, `on_ready`, `on_shutdown`, and
`on_close`. `on_finish` runs last, when the upgraded connection closes.

User callbacks run on the HTTP task queue set by `fio_http_settings_s.queue`.
Without a queue, they run on the IO queue.

### Protocol errors

Malformed requests receive a `400 Bad Request` response, then the connection
closes. This includes framing that could be misread as a smuggled pipelined
request:

- a `Transfer-Encoding` without a single, final `chunked` coding (RFC 9112
  §6.3, §7);
- an HTTP/1.1 request without exactly one `Host` header (RFC 9112 §3.2);
- an `Expect` header with a value other than `100-continue`.

Codings before the final `chunked` (e.g., `gzip, chunked`) stay visible as a
request `transfer-encoding` header (e.g., `gzip`). The body is passed through
as received, still encoded.

A response line received by a server, or an unsolicited response received by
a client, is logged as a `SECURITY` event and the connection closes.

### Persistent connections

- **HTTP/1.1** connections stay open. A request `Connection` field with the
  `close` token (case-insensitive) closes the connection once the response is
  sent. Later pipelined requests are not handled.
- **HTTP/1.0** connections close after the response, unless the request
  `Connection` field has `keep-alive` and no `close` (RFC 9112 §9.3). The
  server then adds a `connection: keep-alive` response header, unless the
  application set a `connection` header. Streamed HTTP/1.0 responses always
  close the connection.
- **Closing from the application**: send the complete response, then call
  `fio_http_close` while the handle is still valid (e.g., from `on_finish`).
  A response `Connection: close` header alone does not close the connection.

---

## Settings and Listener API

#### `fio_http_settings_s`

```c
typedef struct fio_http_settings_s {
  /* request / response callbacks */
  void (*pre_http_body)(fio_http_s *h);
  void (*on_http)(fio_http_s *h);
  void (*on_finish)(fio_http_s *h);
  /* upgrade authentication: return non-zero to deny */
  int (*on_authenticate_sse)(fio_http_s *h);
  int (*on_authenticate_websocket)(fio_http_s *h);
  /* upgraded (WebSocket / SSE) connection callbacks */
  void (*on_open)(fio_http_s *h);
  void (*on_message)(fio_http_s *h, fio_buf_info_s msg, uint8_t is_text);
  void (*on_eventsource)(fio_http_s *h,
                         fio_buf_info_s id,
                         fio_buf_info_s event,
                         fio_buf_info_s data);
  void (*on_eventsource_reconnect)(fio_http_s *h, fio_buf_info_s id);
  void (*on_ready)(fio_http_s *h);
  void (*on_shutdown)(fio_http_s *h);
  void (*on_close)(fio_http_s *h);
  /* service lifetime */
  void (*on_stop)(struct fio_http_settings_s *settings);
  /* user data, TLS and task queue */
  void *udata;
  fio_io_functions_s *tls_io_func;
  fio_io_tls_s *tls;
  fio_io_async_s *queue;
  /* static files */
  fio_str_info_s public_folder;
  size_t max_age;
  /* limits */
  uint32_t max_header_size;
  uint32_t max_line_len;
  size_t max_body_size;
  size_t max_params;
  size_t ws_max_msg_size;
  intptr_t reserved1;
  intptr_t reserved2;
  /* timeouts and flags */
  uint16_t connect_timeout;
  uint8_t timeout;
  uint8_t ws_timeout;
  uint8_t sse_timeout;
  uint8_t log;
  uint8_t compress_static;
  uint8_t compress_dynamic;
  uint8_t compress_ws;
} fio_http_settings_s;
```

Holds the callbacks, limits, and options of a listener, a route, or a client
connection. The listen, route, and connect functions take it by value through
named-argument macros, so fields are usually passed as designated
initializers. Unset (zero) callbacks and limits get defaults when the
listener, route, or client is created.

**Members (callbacks):**

| Field | Called | Default when unset |
| --- | --- | --- |
| `pre_http_body` | Before the body is read, when the request sends `Expect: 100-continue` and has a body. Send a response here (e.g., with `fio_http_send_error_response`) to refuse the body; otherwise `100 Continue` is sent. Bodies over `max_body_size` get `413` before this callback runs. | no-op |
| `on_http` | For each request (server) or final response (client). | server: `404` response; client: no-op |
| `on_finish` | When a plain request / response cycle completes. For WebSocket / SSE, after `on_close`. | no-op |
| `on_authenticate_sse` | Before an SSE upgrade. Return non-zero to deny with `403`. | server: deny; client: allow |
| `on_authenticate_websocket` | Before a WebSocket upgrade. Return non-zero to deny with `403`. | server: deny; client: allow |
| `on_open` | Once a WebSocket or SSE upgrade is complete. | no-op |
| `on_message` | For each complete WebSocket message. | no-op |
| `on_eventsource` | For each complete SSE event (client). | passes `data` to `on_message` (as text) when `on_message` is set; otherwise no-op |
| `on_eventsource_reconnect` | When an SSE request carries a `Last-Event-ID` header. | no-op |
| `on_ready` | When an upgraded connection's outgoing buffer is empty. | no-op |
| `on_shutdown` | For open upgraded connections during IO shutdown. WebSocket connections then send a `1001` (going away) close frame. | no-op |
| `on_close` | After an upgraded connection closes (for cleanup). | no-op |
| `on_stop` | Once per settings object (the listener's and each route's) when the listener stops, and for a route when it is replaced. Routes do not inherit it. | no-op |

**Members (data and options):**

| Field | Meaning |
| --- | --- |
| `udata` | Default value of `fio_http_udata(h)`. Client connections keep a `udata` already set on the handle. |
| `tls_io_func`, `tls` | Optional TLS support. Ignored by routes. |
| `queue` | Optional task queue for user callbacks (e.g., for multi-threaded responses). Defaults to the IO queue. |
| `public_folder` | Static-file root (see *Static File Service*). The folder must exist when `fio_http_listen` or `fio_http_route` is called; otherwise the setting is ignored and an error is logged. |
| `max_age` | `Cache-Control: max-age` value for static files, in seconds. `0` (default) omits the header. Not inherited by routes. |
| `max_header_size` | Maximum bytes for the request line and headers combined. Default: `FIO_HTTP_DEFAULT_MAX_HEADER_SIZE` (32768). Raised to `max_line_len` when smaller. Larger requests receive `431`. |
| `max_line_len` | Maximum bytes per request / header line. Default: `FIO_HTTP_DEFAULT_MAX_LINE_LEN` (8192). |
| `max_body_size` | Maximum request body size, in bytes. Default: `FIO_HTTP_DEFAULT_MAX_BODY_SIZE` (32 MiB). Larger bodies receive `413`. |
| `max_params` | Maximum query / form fields per params parsing call. Default: `FIO_HTTP_PARAMS_MAX_FIELDS` (4096). |
| `ws_max_msg_size` | Maximum WebSocket message size, in bytes. Default: `FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE` (262144). Larger messages close the connection with `1009` (message too big). |
| `reserved1`, `reserved2` | Reserved for future use. |
| `timeout` | HTTP/1.x connection timeout, in seconds (up to 255). Default: `FIO_HTTP_DEFAULT_TIMEOUT` (50). |
| `ws_timeout` | WebSocket timeout, in seconds. A ping is sent each time it expires. Default: `FIO_HTTP_DEFAULT_TIMEOUT_LONG` (50). |
| `sse_timeout` | SSE timeout, in seconds. A ping is sent each time it expires. Default: the `ws_timeout` value. |
| `connect_timeout` | Client connection timeout in milliseconds (up to 65535), passed to `fio_io_connect`; `0` selects the IO default (30 seconds). |
| `log` | Non-zero logs each request. |
| `compress_static` | Opt-in: create missing `.br` / `.gz` static-file variants on demand (see *Static File Service*). Effective per route. May drop to `0` after repeated failures. |
| `compress_dynamic` | Opt-in: compress dynamic responses. Connection-global: per-route values are ignored. |
| `compress_ws` | Opt-in: WebSocket `permessage-deflate`. Connection-global: per-route values are ignored. |

**Note**: compression can leak secrets (BREACH / CRIME). When a compressed
response mixes secrets (e.g., CSRF tokens or session data) with
attacker-controlled text, its length reveals the secret. For such responses,
leave `compress_dynamic` / `compress_ws` off, or set a response
`content-encoding` header to send the body as is. Responses with a
`content-encoding` header are never re-compressed.

**Note**: `compress_static` makes client requests write files into
`public_folder`. The folder must be writable and should have a disk quota.
Prefer generating variants at deploy time.

**Note**: `compress_dynamic` only compresses finished, single-buffer
responses over 1024 bytes with a compressible (text-like) `content-type`.
`br` is preferred over `gzip`, per the client's `Accept-Encoding`. An encoding
with `q=0` is never used (RFC 9110).

**Note**: with `compress_ws`, when a client offers `permessage-deflate`, the
server always answers `permessage-deflate; server_no_context_takeover;
client_no_context_takeover` (RFC 7692 lets either side request these flags). It honors `server_max_window_bits` when offered and
never sends window-bits parameters. No history is kept between messages, so
per-connection compression memory stays small. Messages under
`FIO_HTTP_WEBSOCKET_DEFLATE_MIN` (1024) bytes, and messages that compression
would not shrink, are sent uncompressed.

**Note**: `Vary: accept-encoding` is added to compressed responses. It is also
added to uncompressed responses that could have been compressed (static files
that may get a created variant, and eligible dynamic responses), unless the
application already set a `vary` header.

#### `fio_http_listener_s`

```c
typedef struct fio_http_listener_s fio_http_listener_s;
```

An opaque listener handle returned by `fio_http_listen`. Pass it to
`fio_http_route`, `fio_http_route_settings`, and
`fio_http_listener_settings`.

#### `fio_http_listen`

```c
SFUNC fio_http_listener_s *fio_http_listen(const char *url,
                                           fio_http_settings_s settings);
#define fio_http_listen(url, ...) \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})
```

Listens for HTTP, WebSocket, and SSE connections on `url`. The function is
shadowed by a macro, so settings are passed as named arguments:

```c
fio_http_listener_s *l = fio_http_listen("0.0.0.0:3000",
                                         .on_http = on_request,
                                         .log = 1);
```

**Parameters:**
- `url` - the address to listen on.

**Named Arguments:** any `fio_http_settings_s` field. These become the root
(`/`) route settings.

**Returns:** the listener, or `NULL` on error. The listener stays valid until
it stops.

**Note**: the listener always serves the root path (`/`). A path in `url` is
ignored with a warning. Use `fio_http_route` for per-prefix settings.

#### `fio_http_listener_settings`

```c
SFUNC fio_http_settings_s *fio_http_listener_settings(fio_http_listener_s *l);
```

Returns a pointer to the listener's root settings. The listener owns them.

**Note**: the route table keeps its own copy of the root route. To change the
root route after the listener is created, call `fio_http_route(l, "/", ...)`.

#### `FIO_HTTP_AUTHENTICATE_ALLOW`

```c
SFUNC int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h);
```

An authentication callback that allows every upgrade. Set it as
`.on_authenticate_sse` or `.on_authenticate_websocket` when no check is
needed.

**Returns:** always `0` (allow).

#### `fio_http_io` / `fio_http_settings`

```c
SFUNC fio_io_s *fio_http_io(fio_http_s *);
SFUNC fio_http_settings_s *fio_http_settings(fio_http_s *);
```

`fio_http_io` returns the IO object attached to the handle.

`fio_http_settings` returns the route settings that match the handle's
original (un-routed) path.

**Returns:** a borrowed pointer, or `NULL` when the handle is `NULL` or is not
attached to a connection (e.g., a handle from `fio_http_new`).

---

## Routing

Routes map a path prefix to its own `fio_http_settings_s`.

### Prefix matching

Routes use the best (longest) prefix match:

- `/` matches every path.
- `/user` matches `/user` and `/user/...`, but not `/user...`.
- A more specific prefix, such as `/user/new`, wins over `/user`.

Declaration order does not matter. Adding a route with an existing prefix
replaces it. Percent-encoded path bytes are decoded before matching.

Inside callbacks, `fio_http_path` returns the path with the route prefix
removed, and `fio_http_opath` returns the original path.

#### `fio_http_route`

```c
SFUNC int fio_http_route(fio_http_listener_s *listener,
                         const char *url,
                         fio_http_settings_s settings);
#define fio_http_route(listener, url, ...) \
  fio_http_route(listener, url, (fio_http_settings_s){__VA_ARGS__})
```

Adds (or replaces) the route for the prefix `url`. The function is shadowed by
a macro, so settings are passed as named arguments:

```c
fio_http_route(listener, "/api", .on_http = on_api);
```

**Parameters:**
- `listener` - a listener returned by `fio_http_listen`.
- `url` - the path prefix. `NULL` or `""` selects the root route (`/`).

**Named Arguments:** any `fio_http_settings_s` field. Unset fields inherit the
listener's values for: `udata`, `on_finish`, both authentication
callbacks, `max_header_size`, `max_line_len`, `max_body_size`, `max_params`,
`ws_max_msg_size`, `timeout`, `ws_timeout`, `sse_timeout`, `log`,
`compress_static`, and `public_folder`. `on_stop` is never inherited, so
the listener's cleanup runs once even when routes share its `udata`. Other
unset callbacks get the
defaults listed under `fio_http_settings_s`. For example, a route without
`on_http` responds with `404`.

**Returns:** `0` on success, or `-1` when `listener` is `NULL` or `url`
contains an invalid (control) character.

**Note**: only `on_http`, `on_finish`, `udata`, the authentication callbacks,
`public_folder`, `max_age`, and `compress_static` take effect per route. All
other fields are connection-global and are read from the listener's root
settings: the upgraded-connection callbacks (`on_open`, `on_message`,
`on_ready`, `on_shutdown`, `on_close`, ...), `queue`, `log`, limits,
timeouts, `compress_dynamic`, and `compress_ws`. Per-route values for them are
stored but unused.

**Note**: `max_age` is **not** inherited. A route that serves static files
must set its own. TLS settings are ignored.

**Note**: if a route's `public_folder` does not exist, an error is logged and
the route uses the listener's folder.

#### `fio_http_route_settings`

```c
SFUNC fio_http_settings_s *fio_http_route_settings(fio_http_listener_s *l,
                                                   const char *url);
```

Returns the settings of the route that best matches `url`, using the same
prefix rules as requests. When no route matches, it returns the root route.

**Returns:** a pointer owned by the listener, or `NULL` when `l` is `NULL`.

### Resource action helper

#### `fio_http_resource_action` / `fio_http_resource_action_e`

```c
typedef enum {
  /** No known action (or an error). */
  FIO_HTTP_RESOURCE_NONE,
  /** `GET /` - list the resources. */
  FIO_HTTP_RESOURCE_INDEX,
  /** `GET /<id>` - show a resource. */
  FIO_HTTP_RESOURCE_SHOW,
  /** `GET /new` - a form for a new resource. */
  FIO_HTTP_RESOURCE_NEW,
  /** `GET /<id>/edit` - a form for editing a resource. */
  FIO_HTTP_RESOURCE_EDIT,
  /** `PUT`, `POST` or `PATCH` to `/` or `/new` - create a resource. */
  FIO_HTTP_RESOURCE_CREATE,
  /** `PUT`, `POST` or `PATCH` to `/<id>` - update a resource. */
  FIO_HTTP_RESOURCE_UPDATE,
  /** `DELETE /<id>` - delete a resource. */
  FIO_HTTP_RESOURCE_DELETE,
  /** `QUERY` (any path) - query the resources. */
  FIO_HTTP_RESOURCE_QUERY,
} fio_http_resource_action_e;

FIO_IFUNC fio_http_resource_action_e fio_http_resource_action(fio_http_s *h);
```

Maps the request method and the routed path (`fio_http_path`) to a REST-style
action. Method names are matched case-insensitively.

| Action | Request shape |
| --- | --- |
| `FIO_HTTP_RESOURCE_INDEX` | `GET /` |
| `FIO_HTTP_RESOURCE_SHOW` | `GET /:id` |
| `FIO_HTTP_RESOURCE_NEW` | `GET /new` |
| `FIO_HTTP_RESOURCE_EDIT` | `GET /:id/edit` |
| `FIO_HTTP_RESOURCE_CREATE` | `PUT`, `POST`, or `PATCH` on `/` or `/new` |
| `FIO_HTTP_RESOURCE_UPDATE` | `PUT`, `POST`, or `PATCH` on `/:id` |
| `FIO_HTTP_RESOURCE_DELETE` | `DELETE /:id` |
| `FIO_HTTP_RESOURCE_QUERY` | `QUERY` on any path |

**Returns:** the matching action, or `FIO_HTTP_RESOURCE_NONE` for other
request shapes or a `NULL` handle.

---

## Static File Service

When the matching route has a `public_folder`, `GET` and `HEAD` requests are
answered with a static file (using `fio_http_static_file_response`) **before**
`on_http` is called. Upgrade requests (WebSocket / SSE) are checked first.

These requests go to `on_http` instead, with the status already set to `200`:

- other methods (`POST`, `PUT`, `DELETE`, `OPTIONS`, ...), since a static file
  is not a valid response to them (RFC 9110 §9.3);
- misses (file not found, or an unsafe path).

The listener's root `public_folder` is joined with the original path. A
route's own `public_folder` is joined with the routed (prefix-trimmed) path.

To answer other methods with a file, call `fio_http_static_file_response`
from `on_http`. It accepts any method except `OPTIONS`.

### Path resolution and safety

- The request path is URL-decoded and joined to the folder. Paths with `..`
  segments or a double slash are rejected.
- A path to a folder is retried as the folder's index file
  (`FIO_HTTP_DEFAULT_INDEX_FILENAME`, `"index"` by default).
- With `FIO_HTTP_STATIC_FILE_COMPLETION` (default `1`), a missing file is
  retried with `.html`, `.htm`, `.txt`, and `.md` appended, so a folder
  resolves to `index.html`, `index.htm`, and so on. Only regular files and
  symlinks are served.
- The `Content-Type` comes from the MIME registry (`fio_http_mimetype`). An
  unregistered extension logs a warning.

### Pre-compressed variants

The server can send a compressed copy of a file (a variant) stored next to
the original. It tests the client's `Accept-Encoding` in this order:

| Encoding | Variant file | Created on demand |
| --- | --- | --- |
| `br` | `<file>.br` | yes, brotli quality 4 |
| `zstd` | `<file>.zstd` | no |
| `gzip` | `<file>.gz` | yes, gzip level 6 |
| `deflate` | `<file>.zip` | no |

The first accepted variant that exists and is at least as new as the original
(by modification time) is served, with `Content-Encoding` set to match.
Encodings refused with `q=0` are skipped (RFC 9110 §12.5.3). Ranged requests
always get the original file.

With `compress_static`, missing or stale `.br` / `.gz` variants are created on
demand and written into the served folder. Creation:

- reads `compress_static` from the matching route's settings (routes inherit
  the listener's value when they are created);
- only applies to compressible (text-like) MIME types and to originals from
  1024 bytes to `FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT` (2 MiB);
- is skipped when compression would not shrink the file;
- needs a writable folder;
- never happens for handles without settings (e.g., from `fio_http_new`).

You can also generate variants at deploy time with any tool. The server only
checks that the files exist and are fresh.

**Note**: any non-zero `compress_static` value enables creation. The server
sets it to `0` (disabled) after repeated consecutive failures, or after a
single filesystem-full or permission error (`ENOSPC`, `EACCES`, `EROFS`,
`EDQUOT`). Creation stays off until the settings are applied again.

`Vary: accept-encoding` is set when a variant is served. It is also set on
uncompressed responses when a variant could be created, so caches key on the
client's `Accept-Encoding`.

### Conditional, ranged, and HEAD requests

- **`ETag`** is a hash of the file's `stat` data, so it changes when the file
  changes. A matching `If-None-Match` gets a `304` response.
- **`Last-Modified`** is the file's modification time.
  `Cache-Control: max-age=<max_age>` is only sent when `max_age` is non-zero.
- **Ranges**: a single `Range: bytes=start-end` gets a `206` response with
  `Content-Range`. Open-ended (`bytes=N-`) and suffix (`bytes=-N`) forms work
  too. An `If-Range` that differs from the current `ETag` disables the range.
  An unsatisfiable range gets `416` with `Content-Range: bytes */<length>`.
  Ranged responses are never compressed and carry no `ETag`.
  `Accept-Ranges: bytes` is always sent.
- **`HEAD`** requests get the same headers as `GET`, without the body.

---

## HTTP Client Connections

#### `fio_http_connect`

```c
SFUNC fio_io_s *fio_http_connect(const char *url,
                                 fio_http_s *h,
                                 fio_http_settings_s settings);
#define fio_http_connect(url, h, ...) \
  fio_http_connect(url, h, (fio_http_settings_s){__VA_ARGS__})
```

Connects to `url` as an HTTP, WebSocket, or SSE client and sends the request
in `h`. The function is shadowed by a macro, so settings are passed as named
arguments:

```c
fio_http_connect("http://example.com/", NULL, .on_http = on_response);
```

**Parameters:**
- `url` - the server URL. Its scheme selects the protocol:
  - `http://` / `https://` - a plain HTTP request;
  - `ws://` / `wss://` - a WebSocket upgrade (calls
    `fio_http_websocket_set_request` and adds an `Origin` header when
    missing);
  - `sse://` / `sses://` - an SSE request (calls `fio_http_sse_set_request`
    and adds an `Origin` header when missing).
- `h` - the request handle, or `NULL` to create one. The connection takes
  ownership of the caller's reference. Unset fields get defaults: the method
  is `GET`, the path and query come from `url` (path `/` when missing), and a
  missing `Host` header comes from `url`. A `udata` already set on `h` is
  kept.

**Named Arguments:** any `fio_http_settings_s` field. `on_http` receives the
response handle.

**Returns:** the connecting IO object, or `NULL` on error. The reactor owns it.
The pointer is valid only until the next event is processed. On failure the
connection frees `h` later, on the IO thread, even when `NULL` is returned. Do
not use or free `h` after the call.

If the server accepts a WebSocket or SSE upgrade, the connection switches
protocol and uses the upgraded callbacks (`on_open`, `on_message` /
`on_eventsource`, `on_close`) instead of `on_http`.

**Note**: interim `1xx` responses other than `101` (e.g., `100 Continue`,
`103 Early Hints`) are skipped (RFC 9110 §15.2). `on_http` runs once, for
the final response. Interim headers are discarded.

**Note**: responses to `HEAD` requests, and `204` / `304` responses, never
read a body, even with `content-length` or `transfer-encoding` headers. Those
headers stay visible as response headers. The body of a `301` / `302`
response is not stored.

#### `fio_http_websocket_connect`

```c
SFUNC fio_io_s *fio_http_websocket_connect(const char *url,
                                           fio_http_s *h,
                                           fio_http_settings_s settings);
#define fio_http_websocket_connect(url, h, ...) \
  fio_http_websocket_connect(url, h, (fio_http_settings_s){__VA_ARGS__})
```

Connects to a WebSocket server. This wrapper fixes the `url` scheme, then
calls `fio_http_connect`:

- `http://` becomes `ws://`, and `https://` becomes `wss://`;
- a URL without a scheme gets `ws://`;
- `ws://`, `wss://`, and any other scheme are used as is.

If the server accepts the upgrade (`101`), the connection switches to
WebSocket and uses `on_open`, `on_message`, and `on_close`. Any other response
goes to `on_http`.

**Parameters**, **Named Arguments**, and **Returns:** as for
`fio_http_connect`.

---

## WebSocket and SSE Upgrades

Servers upgrade automatically. When a request asks for WebSocket or SSE, the
server calls `on_authenticate_websocket` or `on_authenticate_sse` instead of
`on_http`. If the callback returns `0`, the server sends the upgrade response
and calls `on_open`. Otherwise it responds with `403`.

The helpers below test and set up upgrades by hand, e.g., for custom
controllers or client handles.

### Upgrade tests and request / response setup

#### `fio_http_websocket_requested` / `fio_http_sse_requested`

```c
SFUNC int fio_http_websocket_requested(fio_http_s *);
SFUNC int fio_http_sse_requested(fio_http_s *);
```

Test the request headers for an upgrade request.

`fio_http_websocket_requested` checks for `Connection: upgrade`,
`Upgrade: websocket`, and a 24-byte `Sec-WebSocket-Key`.
`fio_http_sse_requested` checks that `Accept` starts with
`text/event-stream`.

**Returns:** `0` when no upgrade is requested, non-zero otherwise.
`fio_http_websocket_requested` returns `-1` (still non-zero) when the
`Sec-WebSocket-Version` is not `13`.

#### `fio_http_upgrade_websocket` / `fio_http_upgrade_sse`

```c
SFUNC void fio_http_upgrade_websocket(fio_http_s *);
SFUNC void fio_http_upgrade_sse(fio_http_s *);
```

Send the server's upgrade response and finish the HTTP cycle. The server calls
these after a successful authentication callback.

`fio_http_upgrade_websocket` sends `101 Switching Protocols` with the
`Sec-WebSocket-Accept` header. It sends `400` when `Sec-WebSocket-Version` is
not `13`, and `403` when `Sec-WebSocket-Key` is invalid.

`fio_http_upgrade_sse` sends the `text/event-stream` response headers. It does
nothing if the response was already started.

#### `fio_http_websocket_set_request` / `fio_http_sse_set_request`

```c
SFUNC void fio_http_websocket_set_request(fio_http_s *);
SFUNC void fio_http_sse_set_request(fio_http_s *);
```

Set the request headers of a client handle to ask for an upgrade.
`fio_http_connect` calls them for `ws://` and `sse://` URLs.

`fio_http_websocket_set_request` sets `Connection`, `Upgrade`,
`Sec-WebSocket-Version: 13`, a random `Sec-WebSocket-Key`, no-cache headers,
and an `Origin` (from `Host`) when missing. `fio_http_sse_set_request` sets
`Accept: text/event-stream`, `Connection: keep-alive`, and
`Cache-Control: no-cache`.

#### `fio_http_websocket_accepted` / `fio_http_sse_accepted`

```c
SFUNC int fio_http_websocket_accepted(fio_http_s *h);
SFUNC int fio_http_sse_accepted(fio_http_s *h);
```

Test a client response for an accepted upgrade. On success, the handle is
marked as upgraded and finished.

`fio_http_websocket_accepted` requires status `101`, the `Connection` and
`Upgrade` headers, and a `Sec-WebSocket-Accept` that matches the request key.
`fio_http_sse_accepted` requires status `200` and a `text/event-stream`
`content-type`.

**Returns:** non-zero if the upgrade was accepted, `0` otherwise.

### WebSocket messages

Incoming messages are collected up to `ws_max_msg_size` bytes, then
`on_message` runs on the HTTP task queue. A connection delivers one message
at a time. The module handles control frames (ping, pong, close).

#### `fio_http_websocket_write`

```c
SFUNC int fio_http_websocket_write(fio_http_s *h,
                                   const void *buf,
                                   size_t len,
                                   uint8_t is_text);
```

Writes one WebSocket message. The data is copied.

**Parameters:**
- `h` - an established WebSocket handle.
- `buf`, `len` - the message payload.
- `is_text` - non-zero sends a text message; `0` sends a binary message.

**Returns:** `0` on success, or `-1` when `h` is `NULL` or is not an
established WebSocket. It may also return `-1` when the connection has
closed.

**Note**: **NOT** thread-safe per connection. Calls on the same handle share
the connection's compression state. Serialize them, e.g., with a mutex or by
sending all writes through one queue.

**Note**: with `compress_ws`, messages of `FIO_HTTP_WEBSOCKET_DEFLATE_MIN`
(1024) bytes or more are compressed when that makes them smaller.

#### `fio_http_on_message_set`

```c
SFUNC int fio_http_on_message_set(fio_http_s *h,
                                  void (*on_message)(fio_http_s *,
                                                     fio_buf_info_s,
                                                     uint8_t));
```

Replaces the `on_message` callback for this WebSocket connection only.
Passing `NULL` restores the callback from the settings.

**Returns:** `0` on success, or `-1` when `h` is `NULL` or not attached to a
connection.

**Note**: call it only on an established WebSocket handle (e.g., from
`on_open`). The function does not check the connection state.

### SSE events

#### `fio_http_sse_write`

```c
/** Named arguments for fio_http_sse_write. */
typedef struct {
  /** The message's `id` data (if any). */
  fio_buf_info_s id;
  /** The message's `event` data (if any). */
  fio_buf_info_s event;
  /** The message's `data` (required; empty data fails). */
  fio_buf_info_s data;
} fio_http_sse_write_args_s;

SFUNC int fio_http_sse_write(fio_http_s *h, fio_http_sse_write_args_s args);
#define fio_http_sse_write(h, ...) \
  fio_http_sse_write((h), ((fio_http_sse_write_args_s){__VA_ARGS__}))
```

Writes one SSE event. The function is shadowed by a macro, so it takes named
arguments:

```c
fio_http_sse_write(h, .data = FIO_BUF_INFO1((char *)"hello"));
```

**Named Arguments:**
- `id` - the event ID; optional.
- `event` - the event name; optional.
- `data` - the event data; required. Each line is sent as its own `data:`
  line.

**Returns:** `0` on success, or `-1` when `data` is empty, `h` is `NULL` or
not an established SSE connection, or the connection has closed.

**Note**: SSE is a UTF-8 text format. The data is not validated.

### Pub/sub helpers

#### `fio_http_subscribe`

```c
#define fio_http_subscribe(h, ...) \
  fio_pubsub_subscribe(.io = fio_http_io(h), __VA_ARGS__)
```

Subscribes the handle's connection to a pub/sub channel. The arguments are
`fio_pubsub_subscribe` named arguments, without `.io`.

```c
fio_http_subscribe(h, .channel = FIO_BUF_INFO1((char *)"chat"));
```

Without `.on_message`, each message goes to the connection protocol's
default: `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT` for WebSocket connections and
`FIO_HTTP_SSE_SUBSCRIBE_DIRECT` for SSE connections.

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT` / `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT` / `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY`

```c
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_pubsub_msg_s *msg);
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY(fio_pubsub_msg_s *msg);
```

Subscription callbacks that write each published message to the WebSocket.

- `_TEXT` sends text messages; the payload must be valid UTF-8.
- `_BINARY` sends binary messages.
- The plain version sends a text message when the payload is valid UTF-8
  and shorter than `FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT`; otherwise
  it sends a binary message.

#### `FIO_HTTP_SSE_SUBSCRIBE_DIRECT`

```c
SFUNC void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);
```

A subscription callback that writes each published message as an SSE event.
The event name is the channel, the data is the message, and the ID is the
message ID in hex. Messages must be UTF-8.

---

## HTTP Handle API Pulled In by `FIO_HTTP`

The HTTP handle (`fio_http_s`) is the request / response object passed to
HTTP callbacks. The same handle type serves servers and clients.

### Lifetime

#### `fio_http_s`

```c
typedef struct fio_http_s fio_http_s;
```

An opaque, reference-counted HTTP handle. Use the functions below to access
it.

**Note**: the handle is **NOT** thread-safe. Access it from one thread at a
time.

#### `fio_http_new`

```c
SFUNC fio_http_s *fio_http_new(void);
```

Creates a new, empty HTTP handle with a reference count of 1.

A new handle has status `0` and a no-op controller, so writes are discarded.
The HTTP connection layer sets up the handles it passes to callbacks.

**Returns:** an owned handle (release with `fio_http_free`), or `NULL` on
allocation failure.

#### `fio_http_new_copy_request`

```c
SFUNC fio_http_s *fio_http_new_copy_request(fio_http_s *old);
```

Creates a new handle that copies the request data of `old`: method, original
path, path, query, version, request headers and received cookies. The body,
the response data, user data and the controller are not copied.

**Returns:** an owned handle (release with `fio_http_free`). Allocation failure
is fatal.

#### `fio_http_free` / `fio_http_dup`

```c
SFUNC void fio_http_free(fio_http_s *);
SFUNC fio_http_s *fio_http_dup(fio_http_s *);
```

`fio_http_dup` increases the handle's reference count and returns the same
handle. `fio_http_free` decreases the reference count and frees the handle
when it reaches zero.

Take a reference with `fio_http_dup` to keep a handle after the callback that
received it returns, for example to finish the response later.

#### `fio_http_destroy`

```c
SFUNC fio_http_s *fio_http_destroy(fio_http_s *h);
```

Releases all the data held by the handle and resets it to the state of a new
handle. The handle's own memory is not freed and its reference count is not
changed.

The controller's `on_destroyed` callback runs first.

**Returns:** `h` (or `NULL` if `h` is `NULL`).

**Note**: `fio_http_free` calls this automatically. Most applications never
need to call it.

#### `fio_http_clear_response`

```c
SFUNC fio_http_s *fio_http_clear_response(fio_http_s *h, bool clear_body);
```

Clears the response, so a new response can be written: removes the response
headers, resets the response state and the status, and updates the
`received_at` timestamp. If `clear_body` is true, the body is released too.

**Returns:** `h`.

**Note**: the status is reset to `0`. Set a status before writing a server
response; with status `0`, `fio_http_write` sends a request.

**Note**: cookies set with `fio_http_cookie_set` are kept.

#### `fio_http_start_time_set`

```c
SFUNC void fio_http_start_time_set(fio_http_s *);
```

Sets the handle's `received_at` timestamp to the current time. Request logs
measure the response time from this timestamp.

#### `fio_http_close`

```c
SFUNC void fio_http_close(fio_http_s *h);
```

Closes the connection that the handle belongs to, including an ordinary HTTP/1
connection. Output that was already scheduled is sent before the connection
closes.

Schedule the complete response before calling this (for example, in
`on_finish`). A handle without a connection ignores the call.

### User data and controller data

#### `fio_http_udata` / `fio_http_udata_set` / `fio_http_udata2` / `fio_http_udata2_set`

```c
FIO_IFUNC void *fio_http_udata(fio_http_s *);
FIO_IFUNC void *fio_http_udata_set(fio_http_s *, void *);
FIO_IFUNC void *fio_http_udata2(fio_http_s *);
FIO_IFUNC void *fio_http_udata2_set(fio_http_s *, void *);
```

Get or set two opaque pointers for application state. The handle never uses
or frees them.

**Returns:** the getters return the stored pointer; the setters return the new
pointer.

**Note**: `h` must not be `NULL`.

#### `fio_http_cdata` / `fio_http_cdata_set`

```c
FIO_IFUNC void *fio_http_cdata(fio_http_s *h);
FIO_IFUNC void *fio_http_cdata_set(fio_http_s *h, void *cdata);
```

Get or set the controller's opaque pointer. The HTTP connection layer uses it
for its own connection data; only custom controllers should set it.

**Returns:** the getter returns the stored pointer; the setter returns the new
pointer.

#### `fio_http_controller` / `fio_http_controller_set`

```c
FIO_IFUNC fio_http_controller_s *fio_http_controller(fio_http_s *h);
FIO_IFUNC fio_http_controller_s *fio_http_controller_set(
    fio_http_s *h,
    fio_http_controller_s *controller);
```

Get or set the controller that sends the handle's data (see
[Controller](#fio_http_controller_s)).

Passing `NULL` installs a no-op controller. Missing callbacks in a custom
controller are filled in with no-op defaults, so the controller object must be
writable and must outlive the handle.

**Returns:** the getter returns the current controller; the setter returns the
controller now in use.

### Request / response properties

#### `fio_http_status` / `fio_http_status_set`

```c
SFUNC size_t fio_http_status(fio_http_s *);
SFUNC size_t fio_http_status_set(fio_http_s *, size_t status);
```

Get or set the response status.

The server sets the status to `200` before calling `on_http`. A client handle
has status `0` until a response arrives. While the status is `0`,
`fio_http_write` sends a request instead of a response.

`fio_http_status_set(h, 0)` sets `200`; values above `1023` set `500`.

**Returns:** the current status (the setter returns the stored value).

#### `fio_http_method` / `fio_http_opath` / `fio_http_path` / `fio_http_query` / `fio_http_version`

```c
SFUNC fio_str_info_s fio_http_method(fio_http_s *);
SFUNC fio_str_info_s fio_http_method_set(fio_http_s *, fio_str_info_s);
SFUNC fio_str_info_s fio_http_opath(fio_http_s *);
SFUNC fio_str_info_s fio_http_opath_set(fio_http_s *, fio_str_info_s);
SFUNC fio_str_info_s fio_http_path(fio_http_s *);
SFUNC fio_str_info_s fio_http_path_set(fio_http_s *, fio_str_info_s);
SFUNC fio_str_info_s fio_http_query(fio_http_s *);
SFUNC fio_str_info_s fio_http_query_set(fio_http_s *, fio_str_info_s);
SFUNC fio_str_info_s fio_http_version(fio_http_s *);
SFUNC fio_str_info_s fio_http_version_set(fio_http_s *, fio_str_info_s);
```

Get or set the request line properties: the method, the original path
(`opath`), the routed path (`path`), the query string and the HTTP version.

`opath` is the path as received, before route prefix trimming. `path` is the
path visible to the current route's callback.

The setters copy `value`.

**Returns:** a view of the stored value, or an empty value if `h` is `NULL`.
The view is borrowed: it stays valid until the property is set again or the
handle is destroyed.

#### `fio_http_method_is_*`

```c
SFUNC int fio_http_method_is_get(fio_http_s *h);
SFUNC int fio_http_method_is_head(fio_http_s *h);
SFUNC int fio_http_method_is_post(fio_http_s *h);
SFUNC int fio_http_method_is_put(fio_http_s *h);
SFUNC int fio_http_method_is_delete(fio_http_s *h);
SFUNC int fio_http_method_is_connect(fio_http_s *h);
SFUNC int fio_http_method_is_options(fio_http_s *h);
SFUNC int fio_http_method_is_trace(fio_http_s *h);
SFUNC int fio_http_method_is_patch(fio_http_s *h);
SFUNC int fio_http_method_is_query(fio_http_s *h);
SFUNC int fio_http_method_is_propfind(fio_http_s *h);
SFUNC int fio_http_method_is_proppatch(fio_http_s *h);
SFUNC int fio_http_method_is_mkcol(fio_http_s *h);
SFUNC int fio_http_method_is_copy(fio_http_s *h);
SFUNC int fio_http_method_is_move(fio_http_s *h);
SFUNC int fio_http_method_is_lock(fio_http_s *h);
SFUNC int fio_http_method_is_unlock(fio_http_s *h);
SFUNC int fio_http_method_is_search(fio_http_s *h);
```

Test whether the request method is the named method, ignoring ASCII case.
They cover the core HTTP methods, PATCH, QUERY, and common WebDAV methods
(PROPFIND, PROPPATCH, MKCOL, COPY, MOVE, LOCK, UNLOCK and SEARCH).

Unlike `fio_http_resource_action`, these helpers only test the method. They
do not look at the path or group PUT, POST and PATCH into one action.

**Returns:** `1` on a match, `0` otherwise.

**Note**: `h` must not be `NULL`.

#### `fio_http_received_at` / `fio_http_received_at_set`

```c
SFUNC int64_t fio_http_received_at(fio_http_s *);
SFUNC int64_t fio_http_received_at_set(fio_http_s *, int64_t);
```

Get or set the time the request was received. Request logs measure the
response time from it.

The unit is milliseconds, or microseconds when `FIO_HTTP_EXACT_LOGGING` is
enabled (see [Compile-Time Knobs](#compile-time-knobs)).

**Returns:** the timestamp (the setter returns the stored value).

### State checks

#### `fio_http_is_*` state tests

```c
SFUNC int fio_http_is_clean(fio_http_s *);
SFUNC int fio_http_is_finished(fio_http_s *);
SFUNC int fio_http_is_streaming(fio_http_s *);
SFUNC int fio_http_is_upgraded(fio_http_s *h);
SFUNC int fio_http_is_websocket(fio_http_s *);
SFUNC int fio_http_is_sse(fio_http_s *);
SFUNC int fio_http_is_freeing(fio_http_s *);
```

Test the state of the request / response cycle:

- `fio_http_is_clean` - no headers were sent and nothing was upgraded yet
  (headers can still be set).
- `fio_http_is_finished` - the response (or request) is complete.
- `fio_http_is_streaming` - the response is streamed (no `content-length`).
- `fio_http_is_upgraded` - the connection was (or should have been) upgraded.
- `fio_http_is_websocket` - the handle belongs to a WebSocket connection.
- `fio_http_is_sse` - the handle belongs to an SSE (EventSource) connection.
- `fio_http_is_freeing` - the handle is being destroyed (useful in controller
  and `on_finish` callbacks).

**Returns:** non-zero if the state is set, `0` otherwise.

**Note**: `h` must not be `NULL`.

### Headers

Request and response headers use twin functions: replace `request` with
`response` in the name. Header names are case-sensitive. Received header names
are stored in lowercase, so use lowercase names (or enable
`FIO_HTTP_ENFORCE_LOWERCASE_HEADERS`).

A server receives request headers; a client receives response headers. Use
the [Cookies](#cookies) functions to read and set cookies.

#### `fio_http_request_header` / `fio_http_response_header`

```c
SFUNC fio_str_info_s fio_http_request_header(fio_http_s *,
                                             fio_str_info_s name,
                                             size_t index);
SFUNC fio_str_info_s fio_http_response_header(fio_http_s *,
                                              fio_str_info_s name,
                                              size_t index);
```

Gets the value at `index` of the header `name`. A header received more than
once has a value per field: use `index` `0`, `1`, ... to read them in order.
An `index` of `(size_t)-1` gets the last value.

**Returns:** a borrowed view of the value, or an empty value if the header or
the index doesn't exist. The view stays valid until the header is changed or
the handle is destroyed.

#### `fio_http_request_header_count` / `fio_http_response_header_count`

```c
SFUNC size_t fio_http_request_header_count(fio_http_s *, fio_str_info_s name);
SFUNC size_t fio_http_response_header_count(fio_http_s *, fio_str_info_s name);
```

**Returns:** the number of values stored for `name`. If `name.buf` is `NULL`,
returns the number of unique header names.

#### `fio_http_request_header_set` / `_set_if_missing` / `_add` (and `response` twins)

```c
SFUNC fio_str_info_s fio_http_request_header_set(fio_http_s *,
                                                 fio_str_info_s name,
                                                 fio_str_info_s value);
SFUNC fio_str_info_s
fio_http_request_header_set_if_missing(fio_http_s *,
                                       fio_str_info_s name,
                                       fio_str_info_s value);
SFUNC fio_str_info_s fio_http_request_header_add(fio_http_s *,
                                                 fio_str_info_s name,
                                                 fio_str_info_s value);
SFUNC fio_str_info_s fio_http_response_header_set(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);
SFUNC fio_str_info_s
fio_http_response_header_set_if_missing(fio_http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value);
SFUNC fio_str_info_s fio_http_response_header_add(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);
```

Change a header. The name and value are copied.

- `*_set` replaces all the values of `name` with `value`.
- `*_set_if_missing` sets `value` only if `name` has no value yet.
- `*_add` adds `value` after the existing values of `name`.

An empty `value` removes the header with `*_set`. `*_set_if_missing` ignores
it and returns the existing value (if any). `*_add` ignores it.

**Returns:** a borrowed view of the stored value. `*_set_if_missing` returns
the existing (last) value when the header already exists. Returns an empty
value when the header was removed or the input was rejected.

**Note**: for security, names longer than 4095 bytes, names containing bytes
the HTTP/1 parser forbids in names (such as whitespace, colon, NUL, CR and
LF), and values containing NUL, CR or LF are rejected. A rejected call
changes nothing. With `FIO_HTTP_ENFORCE_LOWERCASE_HEADERS`, names are stored
in lowercase; the caller's input is not modified.

**Note**: response header changes made after the headers were sent are not
sent.

#### `fio_http_request_header_each` / `fio_http_response_header_each`

```c
SFUNC size_t fio_http_request_header_each(fio_http_s *,
                                          int (*callback)(fio_http_s *,
                                                          fio_str_info_s name,
                                                          fio_str_info_s value,
                                                          void *udata),
                                          void *udata);
SFUNC size_t fio_http_response_header_each(fio_http_s *,
                                           int (*callback)(fio_http_s *,
                                                           fio_str_info_s name,
                                                           fio_str_info_s value,
                                                           void *udata),
                                           void *udata);
```

Calls `callback` once for each header value, with the header's name and the
value. Response cookies set with `fio_http_cookie_set` are not included (use
`fio_http_set_cookie_each`).

**Parameters:**
- `callback` - called for each value; return `-1` to stop the iteration.
- `udata` - passed to `callback`.

**Returns:** the number of header names visited. If `callback` is `NULL`,
returns the number of unique header names.

**Note**: do not change the headers during the iteration.

### Header value and property iteration

These helpers split header values without copying them. They walk repeated
header fields, the comma-separated values in each field, and the
semicolon-separated properties of each value (as in
`accept: text/html;q=0.9, */*;q=0.8`).

All slices borrow the header storage and are **not** necessarily
NUL-terminated: use `.buf` and `.len`, not C string functions. Surrounding
spaces and tabs are trimmed. Quoted strings stay quoted, escapes included.
Empty list elements and empty property names are skipped. Do not change the
headers during the iteration.

#### `fio_http_header_each_s`

```c
typedef struct fio_http_header_each_s {
  fio_buf_info_s name;       /* header name, or property name */
  fio_buf_info_s value;      /* list value, or property value */
  fio_buf_info_s properties; /* property span; cursor in property iterator */
  unsigned index;            /* index of the current repeated header field */
  unsigned is_request;
} fio_http_header_each_s;
```

The iteration cursor.

**Members:**
- `name` - the header name (value iteration) or the property name (property
  iteration).
- `value` - the current value or property value. `value.buf == NULL` marks
  the end. A property without `=` (or with an empty value) has a non-`NULL`
  `value.buf` and `value.len == 0`.
- `properties` - the current value's property span (the text after its first
  `;`, up to the next `,`), or `{0}` if it has none.
- `index`, `is_request` - iterator state; do not change them.

#### `FIO_HTTP_HEADER_EACH_VALUE` / `FIO_HTTP_HEADER_EACH_PROPERTY`

```c
#define FIO_HTTP_HEADER_EACH_VALUE(http_handle, is_request, header_name, pos)
#define FIO_HTTP_HEADER_EACH_PROPERTY(value_, pos)
```

`FIO_HTTP_HEADER_EACH_VALUE` loops over the values of the request
(`is_request = 1`) or response (`is_request = 0`) header `header_name`.
`FIO_HTTP_HEADER_EACH_PROPERTY` loops over the properties of a value cursor.

Both declare a `fio_http_header_each_s` loop variable named `pos`. `break` and
`continue` work as in any `for` loop. `http_handle` is evaluated on every step.

```c
/* illustrative */
FIO_HTTP_HEADER_EACH_VALUE(h, 1, FIO_STR_INFO1("accept"), item) {
  /* item.value is the media type, e.g. "text/html" */
  FIO_HTTP_HEADER_EACH_PROPERTY(item, prop) {
    /* prop.name is "q", prop.value is "0.9" */
  }
}
```

#### `fio_http_header_each_first` / `fio_http_header_each_next` / `fio_http_header_each_property_first` / `fio_http_header_each_property_next`

```c
FIO_IFUNC fio_http_header_each_s
fio_http_header_each_first(fio_http_s *http_handle,
                           int is_request,
                           fio_str_info_s header_name);
FIO_IFUNC void fio_http_header_each_next(fio_http_header_each_s *pos,
                                         fio_http_s *http_handle);
FIO_IFUNC fio_http_header_each_s
fio_http_header_each_property_first(const fio_buf_info_s *properties);
FIO_IFUNC void fio_http_header_each_property_next(fio_http_header_each_s *pos);
```

The functions behind the macros, for manual iteration.

- `fio_http_header_each_first` returns a cursor at the first value of
  `header_name` (a borrowed lookup key).
- `fio_http_header_each_next` moves to the next value; pass the same handle.
- `fio_http_header_each_property_first` returns a cursor at the first property
  of a value's `properties` span.
- `fio_http_header_each_property_next` moves to the next property.

**Returns:** the `*_first` functions return a cursor; its `value.buf` is
`NULL` if there is no value or property. The `*_next` functions set
`value.buf` to `NULL` after the last one.

### Body storage and reads

The handle keeps the body in RAM until it grows past `FIO_HTTP_BODY_RAM_LIMIT`,
then moves it to an anonymous temporary file. Reading works the same in both
cases. The [Data Store module](./012 data.md) (`fio_data_s`) provides this
storage and can be used on its own.

#### `fio_http_body_length` / `fio_http_body_pos`

```c
SFUNC size_t fio_http_body_length(fio_http_s *);
SFUNC size_t fio_http_body_pos(fio_http_s *h);
```

**Returns:** the body's length in bytes, or the current reading position.

#### `fio_http_body_seek`

```c
SFUNC size_t fio_http_body_seek(fio_http_s *, ssize_t pos);
```

Sets the body's reading position.

Negative values count back from the end: `-n` is `length - n`, so `-1` is the
last byte (as with `lseek(fd, -1, SEEK_END)`). Values before the start clamp to
`0`. Any position at or past the end is EOF, so
`fio_http_body_seek(h, fio_http_body_length(h))` (or `SSIZE_MAX`) seeks to the
end.

**Returns:** the new reading position.

#### `fio_http_body_read` / `fio_http_body_read_until`

```c
SFUNC fio_str_info_s fio_http_body_read(fio_http_s *, size_t length);
SFUNC fio_str_info_s fio_http_body_read_until(fio_http_s *,
                                              char token,
                                              size_t limit);
```

Read from the reading position and advance it by the number of bytes returned.
`fio_http_body_read` reads up to `length` bytes. `fio_http_body_read_until`
stops after `token` (inclusive), after `limit` bytes (ignored if `0`), or at
EOF.

**Returns:** a borrowed view of the data, or an empty result at EOF. Any later
`fio_http_body_*` call on the same handle MAY invalidate the view.

**Note**: a body in a temporary file is memory mapped, so
`fio_http_body_read(h, fio_http_body_length(h))` returns the whole body
without copying it. Without mapping, the data is copied to a heap buffer. If
that buffer can't be allocated (or the file read is short), an error is logged
and the read returns an empty result without moving the position. Use
`fio_http_body_pos(h) < fio_http_body_length(h)` to tell this apart from EOF.

#### `fio_http_body_expect`

```c
SFUNC void fio_http_body_expect(fio_http_s *, size_t expected_length);
```

Prepares the body for `expected_length` more bytes: reserves RAM, or moves to
a temporary file early if the size crosses `FIO_HTTP_BODY_RAM_LIMIT`. It's
only a hint and never fails.

#### `fio_http_body_write`

```c
SFUNC int fio_http_body_write(fio_http_s *, const void *data, size_t len);
```

Appends `len` bytes from `data` to the body.

**Returns:** `0` on success, or `-1` on error (for example, when no temporary
file can be created).

**Note**: errors are sticky. Later writes fail until the body is closed, and
the data stored before the error stays readable. When an HTTP/1.1 server can't
store a request body, it responds with `500` and closes the connection; a
client closes the connection.

#### `fio_http_body_fd`

```c
SFUNC int fio_http_body_fd(fio_http_s *);
```

**Returns:** the body's temporary file descriptor (borrowed: do not close it),
or `-1` while the body is in RAM.

#### `fio_http_body_close`

```c
SFUNC void fio_http_body_close(fio_http_s *);
```

Releases the body's RAM buffer and temporary file (if any) and resets the body
to empty.

The handle releases its body automatically when it is destroyed. Call this
only to free resources early, once the body was consumed.

#### `fio_http_body_save`

```c
SFUNC int fio_http_body_save(fio_http_s *, const char *filename);
```

Saves the whole body to `filename`, atomically replacing any existing file
(see `fio_data_save`). A body in a temporary file is copied file to file, so a
large upload is never loaded into RAM. The body and its reading position are
unchanged. A leading `"~/"` is the home folder.

**Returns:** `0` on success, or `-1` on error (including a body in a write
error state, or a short read of the body's temporary file).

**Note**: never derive `filename` from client input without validating it.

### Params parsing (query / body)

These functions parse the query string and the request body into application
objects. They build nothing themselves: the parser calls your callbacks to
create values, maps and arrays. Supported bodies are JSON, URL-encoded and
`multipart/form-data`.

With the FIOBJ module included, `fio_http_params2FIOBJ` builds a FIOBJ Hash
(or merges into an existing one); see
[HTTP Params to FIOBJ](./440%20http%20fiobj.md).

#### `fio_http_params_parse_callbacks_s`

```c
typedef struct FIO_ALIGN(64) {
  /** JSON null: returns an owned value. */
  fio_json_obj_result_s (*on_null)(void *udata);
  /** JSON true: returns an owned value. */
  fio_json_obj_result_s (*on_true)(void *udata);
  /** JSON false: returns an owned value. */
  fio_json_obj_result_s (*on_false)(void *udata);
  /** JSON integer: returns an owned value. */
  fio_json_obj_result_s (*on_number)(void *udata, int64_t i);
  /** JSON float: returns an owned value. */
  fio_json_obj_result_s (*on_float)(void *udata, double f);
  /** JSON string with escapes (raw, still escaped): returns an owned value.
   * Form text uses it only when `on_string_simple` is NULL. */
  fio_json_obj_result_s (*on_string)(void *udata, fio_buf_info_s str);
  /** Decoded query, URL-encoded or multipart text, or a JSON string without
   * escapes: returns an owned value (falls back to `on_string` if NULL). */
  fio_json_obj_result_s (*on_string_simple)(void *udata, fio_buf_info_s str);
  /** Opens a map at `parent[key]` (`key.buf == NULL` appends to an array
   * parent; `parent == NULL` creates the owned root). Reuse an existing map to
   * merge. A replaced value is freed or returned in `old`. */
  fio_json_push_result_s (*on_map)(void *udata,
                                   void *parent,
                                   fio_buf_info_s key);
  /** Like `on_map`, opening an array (reuse an existing array to append). */
  fio_json_push_result_s (*on_array)(void *udata,
                                     void *parent,
                                     fio_buf_info_s key);
  /** Stores `value` (owned, taken even on error) at the decoded `map[key]`. A
   * replaced value is freed or returned in `obj` for conflict resolution. */
  fio_json_obj_result_s (*map_push)(void *udata,
                                    void *map,
                                    fio_buf_info_s key,
                                    void *value);
  /** Appends `value` (owned, taken even on error). Returns non-zero on error. */
  int (*array_push)(void *udata, void *array, void *value);
  /** Returns non-zero if `obj` is an array (optional, used for conflicts). */
  int (*is_array)(void *udata, void *obj);
  /** Returns non-zero if `obj` is a map (optional, used for conflicts and
   * required for `fio_http_params_parse` to merge a JSON object body). */
  int (*is_map)(void *udata, void *obj);
  /** Frees an owned unused value (never a key); `obj` may be NULL. */
  void (*free_unused_object)(void *udata, void *obj);

  /* ===== File Uploads (multipart) ===== */

  /** Called when a file upload starts. Returns an owned file object, or NULL
   * to skip the file. The object is stored at the field name once the upload
   * ends, or freed through `free_unused_object` if parsing failed. */
  void *(*on_file)(void *udata,
                   fio_str_info_s name,
                   fio_str_info_s filename,
                   fio_str_info_s content_type,
                   fio_str_info_s headers);
  /** Called for each chunk of file data. A non-zero return aborts parsing
   * (returned unchanged in `err`); the file object is then freed. */
  int (*on_file_data)(void *udata, void *file, fio_buf_info_s data);
} fio_http_params_parse_callbacks_s;
```

The parser callbacks: 16 pointers, aligned to 64 bytes (128 bytes in total).
The first 14 members match `fio_json_parser_callbacks_s` exactly (same order,
types and contract), so the table is handed to the JSON parser as is. They
follow the [JSON callback contract](./012%20json.md#fio_json_parser_callbacks_s),
which defines the result types, decoded keys, ownership, `old` and the
conflict rules. The last two members handle multipart file uploads.

**Contract:**
- **Ownership transfers:** value callbacks return owned values. `map_push` and
  `array_push` take the value they receive, **even when they fail**; the
  parser never uses or frees a value after passing it.
- **Containers:** a container placed in a parent is borrowed; a root
  (`parent == NULL`) is owned by the caller.
- **NULL is nothing:** a native `NULL` `obj` / `old` never triggers conflict
  resolution. A replaced value is either freed by the callback or handed back
  (`old`, or `map_push().obj`) for the conflict rules.
- **Opaque errors:** any non-zero callback `err` stops parsing and reaches
  `fio_http_params_parse_result_s.err` unchanged. On error, `obj` / `old` are
  ignored.
- **Results on error:** `result` holds the partial root, owned by the caller.

**Members:**
- `on_null` ... `on_float` - JSON scalars; return an owned value.
- `on_string` - JSON strings with escapes; receives the escaped bytes and
  returns an owned, decoded string.
- `on_string_simple` - decoded query, URL-encoded and multipart text values,
  and JSON strings without escapes; returns an owned value. Text values are
  never converted to numbers or booleans (use `fio_string2primitive` if
  needed). If it is `NULL`, form text goes to `on_string` (and vice versa).
- `on_map`, `on_array`, `map_push`, `array_push` - create and fill
  containers. Keys arrive decoded and are borrowed only during the call. JSON
  object keys go to these callbacks, never to the string callbacks.
- `is_map`, `is_array` - resolve conflicts with existing containers. A
  missing callback counts as "false". `fio_http_params_parse` needs `is_map`
  to merge a JSON object body into the params map.
- `free_unused_object` - frees an owned value the parser didn't use.
- `on_file` - a file upload starts; returns an owned file object, or `NULL`
  to skip the file. When the upload ends, the object is stored like any other
  value, or passed to `free_unused_object` if parsing already failed
  (including truncated uploads).
- `on_file_data` - receives the next chunk of file data; a non-zero return
  aborts parsing (returned unchanged in `err`) and frees the file object.

There is no general duplicate-key rule: a callback either frees a replaced
value itself or hands it back (see the JSON conflict rules).

The table is never written to: pass a `static const` table. Missing
`on_string` / `on_string_simple`, `is_map` and `is_array` callbacks are
handled internally.

#### `fio_http_params_parse_args_s`

```c
typedef struct {
  /** Parser callbacks (required, never written to). */
  const fio_http_params_parse_callbacks_s *callbacks;
  /** User / root context passed unchanged to all callbacks. */
  void *udata;
  /** Field budget for this call (query pairs, form fields and multipart
   * parts; JSON bodies are not counted). Zero (the default) inherits
   * `settings.max_params` (default FIO_HTTP_PARAMS_MAX_FIELDS); a non-zero
   * value fully replaces the settings value for this call. */
  size_t limit;
} fio_http_params_parse_args_s;
```

The named arguments of the parsing functions.

**Members:**
- `callbacks` - the parser callbacks; required.
- `udata` - passed to every callback.
- `limit` - the field budget for this call; `0` (the default) uses the
  `max_params` value of `fio_http_settings(h)` (default `FIO_HTTP_PARAMS_MAX_FIELDS`, 4096).
  See the field limits of the parsing functions below.

#### `fio_http_params_parse_error_e`

```c
typedef enum {
  /** Parsing succeeded. */
  FIO_HTTP_PARAMS_PARSE_SUCCESS = 0,
  /** Invalid arguments, malformed input or another error. */
  FIO_HTTP_PARAMS_PARSE_ERROR = -1,
  /** A field name or JSON value exceeded the nesting-depth limit. */
  FIO_HTTP_PARAMS_PARSE_DEPTH = -2,
  /** The HTTP body could not be read (not ordinary EOF). */
  FIO_HTTP_PARAMS_PARSE_IO = -3,
} fio_http_params_parse_error_e;
```

Named values for the errors the HTTP params parsers report themselves.
`fio_http_params_parse_result_s.err` is a plain `int`: success is `0` and
every failure is non-zero. Generic / formatting and depth values intentionally
match `FIO_JSON_ERROR_GENERIC` and `FIO_JSON_ERROR_NESTING`.

| Value | Meaning |
| --- | --- |
| `FIO_HTTP_PARAMS_PARSE_SUCCESS` | Parsing succeeded. |
| `FIO_HTTP_PARAMS_PARSE_ERROR` | Invalid arguments or callbacks, malformed or truncated input, an unsupported direct-body Content-Type, field-budget overflow, or another generic error. Ordinary EOF before a complete multipart body is an error, not an I/O failure. |
| `FIO_HTTP_PARAMS_PARSE_DEPTH` | A decoded form name path or JSON body exceeded `FIO_JSON_MAX_DEPTH`. |
| `FIO_HTTP_PARAMS_PARSE_IO` | Reading a spilled JSON, URL-encoded or multipart body failed. Ordinary EOF is not I/O failure. |

Callback errors pass through unchanged, without translation, from every
source (JSON, query, URL-encoded and multipart). A callback may return any
non-zero value, including custom positive or negative values, so a callback
error may coincide with one of the named values above.

Invalid or truncated percent escapes in query / URL-encoded input remain
literal and do not fail parsing. Field-budget overflow uses `ERROR`, not a
separate error kind.

#### `fio_http_params_parse_result_s`

```c
typedef struct {
  /** The top-level parsed object, owned by the caller (the partial root on
   * error, NULL if none was created). */
  void *result;
  /** Number of bytes consumed from the parsed input(s). */
  size_t consumed;
  /** Number of fields charged against the field budget (query pairs, form
   * fields and multipart parts, including skipped uploads). JSON bodies are
   * not counted. */
  size_t count;
  /** 0 on success, a `fio_http_params_parse_error_e` value or an unchanged
   * non-zero callback error. */
  int err;
} fio_http_params_parse_result_s;
```

The result of a parsing function.

**Members:**
- `result` - the root object, owned by the caller. `NULL` for an empty JSON or
  multipart body, or when no root was created. After an error, the partial
  root (which the caller still owns and must free).
- `consumed` - the number of input bytes parsed (query and body). Ignored body
  bytes are not counted.
- `count` - the number of fields charged against the field budget (query
  pairs, form fields and multipart parts, including skipped uploads). JSON
  bodies add `0`.
- `err` - `0` on success, a named parsing error, or an unchanged non-zero
  callback error.

#### `fio_http_body_parse` / `fio_http_query_parse` / `fio_http_params_parse`

```c
SFUNC fio_http_params_parse_result_s FIO_NOOP
fio_http_body_parse(fio_http_s *h, fio_http_params_parse_args_s args);
#define fio_http_body_parse(http___handle, ...)                                \
  fio_http_body_parse((http___handle),                                         \
                      (fio_http_params_parse_args_s){__VA_ARGS__})

SFUNC fio_http_params_parse_result_s FIO_NOOP
fio_http_query_parse(fio_http_s *h, fio_http_params_parse_args_s args);
#define fio_http_query_parse(http___handle, ...)                               \
  fio_http_query_parse((http___handle),                                        \
                       (fio_http_params_parse_args_s){__VA_ARGS__})

SFUNC fio_http_params_parse_result_s FIO_NOOP
fio_http_params_parse(fio_http_s *h, fio_http_params_parse_args_s args);
#define fio_http_params_parse(http___handle, ...)                              \
  fio_http_params_parse((http___handle),                                       \
                        (fio_http_params_parse_args_s){__VA_ARGS__})
```

Each function is shadowed by a macro, so it accepts the named arguments of
`fio_http_params_parse_args_s`:

```c
/* illustrative: my_callbacks is a `static const` callback table */
fio_http_params_parse_result_s r =
    fio_http_params_parse(h, .callbacks = &my_callbacks, .udata = my_ctx);
if (r.err) {
  /* r.result is the partial params map (or NULL): the caller still owns it */
  my_free(r.result);
  return;
}
```

- `fio_http_query_parse` parses the URL-encoded query into a map. Keys and
  values are percent / `+` decoded. Both `key` and `key=` give an empty string
  value; an empty query gives an empty map. Invalid percent escapes stay
  literal rather than failing.
- `fio_http_body_parse` parses the body by its Content-Type. A JSON body
  returns its root, whether map, array or scalar. URL-encoded and multipart
  bodies return a map. A missing or unknown Content-Type is an error.
- `fio_http_params_parse` parses the query and the body into one map, query
  fields first. A body with a missing or unknown Content-Type is ignored
  without error. A JSON body is merged only if its first non-whitespace byte
  (after an optional UTF-8 BOM) is `{`; other JSON bodies are ignored, even if
  malformed.

For combined query + JSON object parsing, the params map built from the query
is passed to `fio_json_parse` as the explicit `root`, so the body updates that
map in place (`on_map` is not called again at the root, and `is_map` must
recognize the map). The root callbacks may reuse `udata` as the map or create
a new one; either way the query and body fields end up in the one result.

**Named Arguments:** see `fio_http_params_parse_args_s`.

**Returns:** a `fio_http_params_parse_result_s`. The caller owns `result` in
all cases. On success, `err == 0`. When an error happens after the root was
created, `result` is the partial root (free it, or keep it if the root came
from `udata`). A `NULL` handle, an invalid callback table, an unsupported
direct-body Content-Type or a failure to create the root returns an error with
a `NULL` `result`.

**Content-Type matching:** leading spaces / tabs are ignored, and the media
type (before any `;` or whitespace) matches exactly, ignoring case. JSON media
types are `application/json`, `application/<subtype>+json` (with a non-empty
subtype), `text/json` and `application/x-json`. `text/plain` is never JSON.

**Field names and nesting:** decoded query and form field names can address
nested keys. `user[name]` and `user.name` address the same nested key, `tags[]`
appends to an array, and `user[tags][]` appends inside a nested map. Only a
**terminal** `[]` appends. Invalid paths (such as `a..b` or `items[][id]`)
and empty names stay single literal keys. JSON object keys are always literal,
even if they contain dots or brackets.

**Depth limit:** name paths and JSON bodies share `FIO_JSON_MAX_DEPTH`
(default 128). Define it before including the library to change both. Up to
`FIO_JSON_MAX_DEPTH` levels of open containers are allowed, and the root is
level 1. A name with N segments opens N containers (the root plus N-1 nested
maps), plus one array for a terminal `[]`. So N can be at most
`FIO_JSON_MAX_DEPTH`, or one less with `[]`. Going deeper reports `DEPTH`, and
`result` holds the partial root.

**Field limits:** each call accepts at most `.limit` URL-encoded pairs and
multipart parts. The next field fails parsing with `ERROR`: it is not delivered
(nor are its file callbacks), and `result` holds the partial root. Skipped
uploads still count. `fio_http_params_parse` shares one budget between the
query and the body, and its `count` is the combined total. JSON bodies don't count
toward the field limit; they are bounded by `FIO_JSON_MAX_DEPTH` instead.

A non-zero `.limit` replaces the settings value for that call. This lets an
application charge several parses against one budget; for example, parse the
body with `.limit = budget - r.count` after parsing the query.

**Multipart uploads:** malformed syntax and ordinary EOF before the closing
boundary report `ERROR`; a failed body read reports `IO`. File data is
streamed in chunks to `on_file_data`, so large uploads can be written to
storage while they are parsed. The `headers` argument of `on_file` holds the
raw part headers (without the blank line) and
is valid only during the call. The filename is cut to the bytes after its last
`/` or `\`. A file with an empty (cut) filename or an empty field name is
skipped without calling `on_file`. A part without a Content-Type gives a
`NULL` `content_type.buf`; a declared empty type gives a non-`NULL` buffer.

**Body handling:** both body parsers restore the body's reading position,
even on errors. A closed body adds nothing; `fio_http_params_parse` still
returns the query fields. JSON and URL-encoded bodies are read whole, and
a read failure reports `IO`. Multipart reads also distinguish this failure
from ordinary EOF (truncation), which reports `ERROR`.

**Note**: the `{` check of `fio_http_params_parse` does not skip JSON
comments. A JSON body that starts with a comment is ignored by
`fio_http_params_parse` but accepted by `fio_http_body_parse`. This may
change.

### Cookies

#### `fio_http_cookie_same_site_e`

```c
typedef enum fio_http_cookie_same_site_e {
  /** No `SameSite` attribute: the browser decides. */
  FIO_HTTP_COOKIE_SAME_SITE_BROWSER_DEFAULT = 0,
  /** The browser sends the cookie with cross-site and same-site requests. */
  FIO_HTTP_COOKIE_SAME_SITE_NONE,
  /**
   * The cookie is withheld on cross-site sub-requests.
   *
   * The cookie is sent when a user navigates to the URL from an external
   * site.
   */
  FIO_HTTP_COOKIE_SAME_SITE_LAX,
  /** The browser sends the cookie only for same-site requests. */
  FIO_HTTP_COOKIE_SAME_SITE_STRICT,
} fio_http_cookie_same_site_e;
```

The `SameSite` attribute of a cookie (see
[MDN: Set-Cookie](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie)).
The default (`0`) omits the attribute and lets the browser decide.

#### `fio_http_cookie_set`

```c
SFUNC int fio_http_cookie_set(fio_http_s *h, fio_http_cookie_args_s);
/* Named arguments using macro. */
#define fio_http_cookie_set(http___handle, ...)                                \
  fio_http_cookie_set((http___handle), (fio_http_cookie_args_s){__VA_ARGS__})

typedef struct fio_http_cookie_args_s {
  /** The cookie's name (a zero `len` reads `buf` as a C string). */
  fio_str_info_s name;
  /** The cookie's value (a NULL `buf` deletes the cookie). */
  fio_str_info_s value;
  /** The cookie's domain (optional). */
  fio_str_info_s domain;
  /** The cookie's path (optional). */
  fio_str_info_s path;
  /** Max-Age, in seconds (0 = a session cookie). */
  int max_age;
  /** The SameSite value. */
  fio_http_cookie_same_site_e same_site;
  /** Sends the cookie only over secure (HTTPS) connections. */
  unsigned secure : 1;
  /** Hides the cookie from JavaScript (HttpOnly). */
  unsigned http_only : 1;
  /**
   * Sets the Partitioned (third-party) cookie flag:
   * https://developer.mozilla.org/en-US/docs/Web/Privacy/Partitioned_cookies
   */
  unsigned partitioned : 1;
} fio_http_cookie_args_s;
```

Sets a response cookie (a `Set-Cookie` header). The function is shadowed by a
macro, so it accepts named arguments:

```c
/* illustrative */
fio_http_cookie_set(h,
                    .name = FIO_STR_INFO1("session"),
                    .value = FIO_STR_INFO1("abc123"),
                    .max_age = 3600,
                    .http_only = 1,
                    .secure = 1);
```

The new value is also visible to `fio_http_cookie` on the same handle.

**Named Arguments:**
- `name` - the cookie's name.
- `value` - the cookie's value. A `NULL` `value.buf` deletes the cookie (sends
  a negative `Max-Age`).
- `domain`, `path` - optional cookie scope.
- `max_age` - lifetime in seconds; `0` (the default) makes a session cookie.
- `same_site` - the `SameSite` attribute; see `fio_http_cookie_same_site_e`.
- `secure` - send the cookie only over secure connections.
- `http_only` - hide the cookie from JavaScript.
- `partitioned` - set the
  [Partitioned](https://developer.mozilla.org/en-US/docs/Web/Privacy/Partitioned_cookies)
  (third-party) flag.

**Returns:** `0` on success, or `-1` if `h` is `NULL` or the response already
started (headers sent).

**Note**: a `name` or `value` with a non-`NULL` `buf` and `len == 0` is read
as a NUL-terminated string.

**Note**: bytes that aren't allowed in a cookie name or value are
percent-encoded, and a warning is logged (once). Cookie data longer than
about 5KB logs an error. Many browsers and proxies limit a cookie (name and
value together) to about 4KB.

#### `fio_http_cookie`

```c
SFUNC fio_str_info_s fio_http_cookie(fio_http_s *,
                                     const char *name,
                                     size_t name_len);
```

Gets the value of the cookie `name`: a received cookie, or one set with
`fio_http_cookie_set`. Received cookie names and values are percent-decoded.

**Returns:** a borrowed view of the value, or an empty value if there is no
such cookie.

#### `fio_http_cookie_each`

```c
SFUNC size_t fio_http_cookie_each(fio_http_s *,
                                  int (*callback)(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value,
                                                  void *udata),
                                  void *udata);
```

Calls `callback` for each cookie, with the same values `fio_http_cookie`
returns.

**Parameters:**
- `callback` - called for each cookie (must not be `NULL`); return non-zero
  to stop.
- `udata` - passed to `callback`.

**Returns:** the number of times `callback` was called.

#### `fio_http_set_cookie_each`

```c
SFUNC size_t
fio_http_set_cookie_each(fio_http_s *h,
                         int (*callback)(fio_http_s *,
                                         fio_str_info_s set_cookie_header,
                                         fio_str_info_s value,
                                         void *udata),
                         void *udata);
```

Calls `callback` for each cookie set with `fio_http_cookie_set`.
`set_cookie_header` is always `"set-cookie"` and `value` is the full header
value. Controllers use this to send the cookies.

**Parameters:**
- `callback` - called for each response cookie (must not be `NULL`); return
  non-zero to stop.
- `udata` - passed to `callback`.

**Returns:** the number of times `callback` was called.

### Writing responses

#### `fio_http_write` / `fio_http_finish`

```c
SFUNC void fio_http_write(fio_http_s *, fio_http_write_args_s args);
/* Named arguments using macro. */
#define fio_http_write(http_handle, ...)                                       \
  fio_http_write(http_handle, (fio_http_write_args_s){__VA_ARGS__})
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)

typedef struct fio_http_write_args_s {
  /** The data to write (or NULL when writing from `fd`). */
  const void *buf;
  /** The number of bytes to write. */
  size_t len;
  /** The offset into `buf` or `fd` at which writing starts. */
  size_t offset;
  /**
   * A file to send (0 = none). The file is closed once sent (or on error),
   * unless `copy` is set.
   */
  int fd;
  /** Optional: frees `buf` once sent (also called on error). */
  void (*dealloc)(void *);
  /** If set, copies `buf` (or keeps `fd` open) instead of taking ownership. */
  int copy;
  /**
   * If set, this write ends the response.
   *
   * Otherwise the response is streamed, unless a `content-length` header was
   * set.
   */
  int finish;
} fio_http_write_args_s;
```

Writes body data. The first write sends the headers; after that, header
changes are not sent. `fio_http_finish(h)` finishes the response without more
data.

The function is shadowed by a macro, so it accepts named arguments:

```c
/* illustrative */
fio_http_write(h, .buf = "Hello", .len = 5, .finish = 1);
```

With status `0` (a client handle), the same calls write a request.

**Named Arguments:**
- `buf`, `len` - the data to write.
- `offset` - where writing starts, in `buf` or in the file.
- `fd` - a file to send instead of a buffer. Set `len` too: a finished
  response uses it as its `content-length`. `fd` is closed once sent, or on
  error. With `copy` set, `fd` is never closed and stays the caller's.
- `dealloc` - frees `buf` once it was sent. Setting it passes ownership of
  `buf` to `fio_http_write`, which frees it even on error. Without `dealloc`
  or `copy`, `buf` must stay valid until it is sent.
- `copy` - copy `buf` before returning (or keep `fd` open), so the caller
  keeps ownership.
- `finish` - this write completes the response. Without it, the response is
  streamed.

**Note**: an empty write without `finish` does nothing, so headers can still
be set. The response starts with the first write that has data (a buffer or a
file) or `finish`.

**Response framing:**

- A finished response without a `content-length` header gets one
  automatically. A response finished without any data is sent with
  `content-length: 0`, not as an empty stream.
- A streamed HTTP/1.1 response uses `transfer-encoding: chunked`. HTTP/1.0
  responses never use a transfer coding (RFC 9112 §6.1), so an HTTP/1.0
  stream is sent unframed and ends when the connection closes, even if
  `keep-alive` was requested.
- A `date` header is added if missing.
- With `compress_dynamic` enabled, a finished buffer response may be
  compressed (see [`fio_http_settings_s`](#fio_http_settings_s)).

**Special responses:**

- If `fio_http_etag_is_match` returns `1` (the request's `if-none-match`
  matches a 2xx `GET` / `HEAD` response), the first write with data sends
  `304 Not Modified` instead and discards the data.
- Finishing a response with status `400` or above and no data sends the
  default error response (see `fio_http_send_error_response`).
- Responses without content (`1xx`, `204`, `205`, `304`, RFC 9110 §15) never
  send data, are never streamed or compressed, and never send
  `transfer-encoding` (an application value is removed). Their
  `content-length` follows RFC 9110 §8.6:
  - `1xx` / `204`: removed, never added.
  - `205`: always `content-length: 0` (§15.3.6), replacing any other value.
  - `304`: an application value is kept (it describes the selected
    representation); none is added.
- Responses to `HEAD` requests send the headers a `GET` would (including the
  automatic `content-length`) but never send content (RFC 9110 §9.3.2). Body
  writes are discarded and the response is never streamed.

**Upgraded handles:** on a WebSocket or SSE handle, `fio_http_write` sends
through the WebSocket or SSE controller. A WebSocket buffer shorter than
`FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT` is sent as text if it is valid
UTF-8, and as binary otherwise; longer data is sent as binary. Call
`fio_http_websocket_write` to choose the message type.

**Note**: on a handle that isn't upgraded, writes after the response finished
fail: `fd` is closed and `buf` is freed if `dealloc` is set.

### General helpers

#### `fio_http_send_error_response`

```c
SFUNC int fio_http_send_error_response(fio_http_s *h, size_t status);
```

Sends an error response with `status` and finishes the response. A `status`
of `0` or above `1000` sends `404`.

The body is the first file found of `./<status>.html` (relative to the
current working folder) and `<public_folder>/<status>.html`. Otherwise it is a
short plain text message, such as `Error 404: Not Found.`.

**Returns:** `0` on success, or `-1` if `h` is `NULL` or the response already
started.

#### `fio_http_etag_is_match`

```c
SFUNC int fio_http_etag_is_match(fio_http_s *h);
```

Tests whether the client's cached copy is current, following RFC 9110
§13.1.2. It compares the response's `etag` header with every entity tag in
the request's `if-none-match` header lines, using the weak comparison: a
`W/` prefix is ignored on both sides.

Only `GET` and `HEAD` requests with a 2xx response status are evaluated
(RFC 9110 §13.2.1).

**Note**: `If-None-Match: *` matches any such response, even without an
`etag` header, so it gets a `304`. This is what the RFC requires: `*` asks for
the content only if no current representation exists. A `*` mixed with other
tags is invalid and never matches.

`fio_http_write` runs this test itself on the first data write and sends
`304` on a match (see [Writing Responses](#writing-responses)).

**Returns:** `1` on a match, `0` otherwise.

#### `fio_http_static_file_response`

```c
SFUNC int fio_http_static_file_response(fio_http_s *h,
                                        fio_str_info_s root_folder,
                                        fio_str_info_s file_name,
                                        size_t max_age);
```

Sends the file `file_name` (URL-decoded) from `root_folder` and finishes the
response. A non-zero `max_age` sets `cache-control: max-age=<max_age>`.

See [Static File Service](#static-file-service) for path resolution,
pre-compressed variants, ranges and caching.

**Returns:** `0` if the response was sent, or `-1` if the application should
handle the request itself.

**Note**: `root_folder` MUST name an existing folder (use `"."` for the
current working folder). An empty root would expose any absolute path on the
system.

#### `fio_http_status2str`

```c
SFUNC fio_str_info_s fio_http_status2str(size_t status);
```

**Returns:** the reason phrase of `status` (such as `"Not Found"`), or
`"Unknown"`. The string is static: never free it.

#### `fio_http_write_log`

```c
SFUNC void fio_http_write_log(fio_http_s *h);
```

Writes a log line for the request / response cycle to `stdout`. The line
holds the process ID, the peer address (see `fio_http_from`), the time, the
request line, the status, the bytes sent and the time since
`fio_http_received_at`. Lines longer than about 1KB are truncated.

The server calls this for each response when `.log` is set. With
`FIO_HTTP_LOG_X_REQUEST_START`, the line also shows the wait time (see
[Compile-Time Knobs](#compile-time-knobs)).

**Note**: this resets `received_at` to the current time.

#### `fio_http_from`

```c
SFUNC int fio_http_from(fio_str_info_s *dest, const fio_http_s *h);
```

Appends the peer's address to `dest` and NUL-terminates it. The address is
taken from the last `forwarded` header (its `for=` value), then the first
address in the last `x-forwarded-for` header, then the socket's peer address.
If all fail, it writes `"[unknown]"`.

**Parameters:**
- `dest` - the string to append to; it needs room for the address and a NUL
  byte (`dest->capa > dest->len + address length`).
- `h` - the HTTP handle.

**Returns:**
- `0` - the address came from a `forwarded` or `x-forwarded-for` header.
- `-1` - no forwarded header was found; the socket's address (or
  `"[unknown]"`) was written. The return value tells a direct peer apart from
  a forwarded address.
- the address length (positive) - `dest` is too small; nothing was written.

**Note**: clients can send any `forwarded` or `x-forwarded-for` value. Trust
the result only behind a proxy that sets these headers.

#### `fio_http_date` / `fio_http_log_time`

```c
SFUNC fio_str_info_s fio_http_date(uint64_t now_in_seconds);
SFUNC fio_str_info_s fio_http_log_time(uint64_t now_in_seconds);
```

Format a Unix time (in seconds) for the HTTP `date` header (RFC 7231 format)
or for log lines.

**Returns:** a view of a static buffer that is reused while the second stays
the same and overwritten when it changes.

**Note**: the buffers are shared and not synchronized. Copy the result if you
need to keep it.

### Path and MIME helpers

#### `FIO_HTTP_PATH_EACH`

```c
#define FIO_HTTP_PATH_EACH(path, pos)
```

Loops over each section of `path` (such as `fio_http_path(h)`), decoding
percent encoding.

The loop declares `pos`, a `fio_buf_info_s` holding the decoded section, and
`pos_reminder`, a `fio_buf_info_s` holding the rest of the path. `pos` points
to a 4096-byte buffer on the stack, valid only inside the loop.

```c
/* illustrative */
FIO_HTTP_PATH_EACH(fio_http_path(h), section) {
  /* "/users/42" visits "users", then "42" */
}
```

**Note**: the loop stops early at a section longer than about 4063 bytes.

#### `fio_http_mimetype_register` / `fio_http_mimetype`

```c
SFUNC int fio_http_mimetype_register(char *file_ext,
                                     size_t file_ext_len,
                                     fio_str_info_s mime_type);
SFUNC fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len);
```

Register or look up the MIME type of a file extension (without the dot, such
as `"html"`). Common types are registered at startup. Registering an empty
`mime_type` removes the extension.

**Parameters:**
- `file_ext`, `file_ext_len` - the extension; at most 7 bytes. Matching is
  case-sensitive.
- `mime_type` - at most 117 bytes; copied.

**Returns:** `fio_http_mimetype_register` returns `0` on success or `-1` on
error (for example, a value too long). `fio_http_mimetype` returns a borrowed
view of the registered type, or an empty value.

**Note**: the MIME registry is **NOT** thread-safe. Register types before
starting the server.

### Controller

#### `fio_http_controller_s`

```c
typedef struct fio_http_controller_s fio_http_controller_s;

struct fio_http_controller_s {
  /** Reserved. MUST be initialized to zero. */
  uintptr_t private_flags;
  /** Called when an HTTP handle is freed. */
  void (*on_destroyed)(fio_http_s *h);
  /** Informs the controller that request / response headers must be sent. */
  void (*send_headers)(fio_http_s *h);
  /** called by the HTTP handle for each body chunk, or to finish a response. */
  void (*write_body)(fio_http_s *h, fio_http_write_args_s args);
  /** called once a request / response had finished */
  void (*on_finish)(fio_http_s *h);
  /** called to close an HTTP connection */
  void (*close_io)(fio_http_s *h);
  /** called when the file descriptor is directly required */
  int (*get_fd)(fio_http_s *h);
};
```

A controller connects the handle to a protocol: the handle calls it to send
headers and body data. The HTTP module provides controllers for HTTP/1,
WebSocket and SSE; custom controllers are rarely needed.

**Members:**
- `private_flags` - reserved; MUST be zero.
- `on_destroyed` - the handle is being destroyed.
- `send_headers` - send the request / response headers.
- `write_body` - send body data; takes ownership of `args` (free `buf` with
  `dealloc`, close `fd`).
- `on_finish` - the request / response is complete.
- `close_io` - close the connection (see `fio_http_close`).
- `get_fd` - return the connection's file descriptor, or `-1`.

Install a controller with `fio_http_controller_set`. Missing callbacks are
filled with no-op defaults when the controller is set.

**Note**: if the controller's callbacks aren't thread-safe, call
`fio_http_write` only from the thread the controller expects.

---

## Compile-Time Knobs

Define any of these macros before including the library to change its
default. Each is guarded by `#ifndef`.

**Limits and timeouts** (the `DEFAULT_` values apply when the matching
`fio_http_settings_s` field is 0):

| Macro | Default | Meaning |
|---|---|---|
| `FIO_HTTP_DEFAULT_MAX_HEADER_SIZE` | `32768` | Request / status line plus all headers, in bytes. |
| `FIO_HTTP_DEFAULT_MAX_LINE_LEN` | `8192` | A single header or request line, in bytes. |
| `FIO_HTTP_DEFAULT_MAX_BODY_SIZE` | `33554432` | HTTP body (payload), in bytes. |
| `FIO_HTTP_PARAMS_MAX_FIELDS` | `4096` | URL-encoded pairs and multipart parts per parse call. |
| `FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE` | `262144` | A single WebSocket message, in bytes. |
| `FIO_HTTP_DEFAULT_TIMEOUT` | `50` | HTTP/1.x connection timeout, in seconds. |
| `FIO_HTTP_DEFAULT_TIMEOUT_LONG` | `50` | WebSocket / SSE connection timeout, in seconds. |

**WebSocket behavior:**

| Macro | Default | Meaning |
|---|---|---|
| `FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT` | `((1UL << 16) - 10UL)` | `fio_http_write` on a WebSocket tests shorter data for UTF-8 validity: valid data is sent as text, the rest as binary. Longer data is sent as binary without testing. |
| `FIO_HTTP_WEBSOCKET_DEFLATE_MIN` | `1024` | Messages smaller than this are never compressed. |
| `FIO_WEBSOCKET_STATS` | `0` | If true, logs the longest WebSocket round-trips (`FIO_LOG_INFO`). |

**HTTP handle behavior:**

| Macro | Default | Meaning |
|---|---|---|
| `FIO_HTTP_EXACT_LOGGING` | `0` (`1` without the IO module) | If true, request timing uses exact microsecond time stamps and `fio_http_received_at` returns microseconds. Otherwise it uses the IO reactor's cached millisecond clock, which is faster but less exact. |
| `FIO_HTTP_BODY_RAM_LIMIT` | `(1 << 17)` | Bodies larger than this (in bytes) move to a temporary file. |
| `FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER` | `0` | If true, keeps the received `content-length` header in the header map. Off by default, because `fio_http_body_length` gives the same value. |
| `FIO_HTTP_ENFORCE_LOWERCASE_HEADERS` | `0` | If true, header names are lower-cased when set or looked up. Otherwise names are case-sensitive: use lower-case names. |
| `FIO_HTTP_CACHE_LIMIT` | `0` | Maximum strings per HTTP string cache; `0` disables caching. |
| `FIO_HTTP_CACHE_STR_MAX_LEN` | `(1 << 12)` | Strings longer than this (in bytes) are never cached. |
| `FIO_HTTP_CACHE_USES_MUTEX` | `1` | If true, a mutex protects the string caches across threads. |
| `FIO_HTTP_PRE_CACHE_KNOWN_HEADERS` | `1` | Adds a static cache for common header names. |

**Static files and logging:**

| Macro | Default | Meaning |
|---|---|---|
| `FIO_HTTP_DEFAULT_INDEX_FILENAME` | `"index"` | File name served when a static file request points to a folder. |
| `FIO_HTTP_STATIC_FILE_COMPLETION` | `1` | If true, a missing static file is retried with `.html`, `.htm`, `.txt` and `.md` extensions. |
| `FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT` | `(1UL << 21)` | Maximum file size (2 MiB) for on-disk static file compression. |
| `FIO_HTTP_LOG_X_REQUEST_START` | `1` | If true, request logs include the wait time from the `x-request-start` header (see below). |

With `FIO_HTTP_LOG_X_REQUEST_START`, request logs (`.log = 1`) include the
request's wait time when the client or proxy sends an `X-Request-Start`
header holding a millisecond timestamp (optionally prefixed with `t=`). The
wait time is omitted when the header is missing or the wait is 512 seconds or
more.

---

## Minimal Server

This program answers every request with `hello` and logs each request.

```c
#define FIO_HTTP
#include FIO_INCLUDE_FILE

static void on_http(fio_http_s *h) {
  fio_http_response_header_set(h,
                               FIO_STR_INFO1("content-type"),
                               FIO_STR_INFO1("text/plain"));
  fio_http_write(h, .buf = "hello\n", .len = 6, .finish = 1);
}

int main(void) {
  if (!fio_http_listen("0.0.0.0:3000", .on_http = on_http, .log = 1))
    return 1;
  fio_io_start(0); /* runs until stopped, e.g., by SIGINT */
  return 0;
}
```

For WebSocket or SSE, add the upgrade authentication and connection callbacks
to the listener or route settings (see
[WebSocket and SSE Upgrades](#websocket-and-sse-upgrades)).

---
