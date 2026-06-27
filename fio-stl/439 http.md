# HTTP Module

```c
#define FIO_HTTP
#include FIO_INCLUDE_FILE
```

`FIO_HTTP` adds the higher-level HTTP service built on the facil.io IO layer,
the HTTP handle, the HTTP/1.x parser, the WebSocket parser, and the SSE /
WebSocket glue code.

Nearby context: [IO and HTTP overview](./400 io-overview.md), the underlying
[HTTP handle header](./431 http handle.h), the [HTTP/1.x parser](./431 http1 parser.md),
the [WebSocket parser](./431 websocket parser.md), and optional compression
support in [DEFLATE / Gzip](./162 deflate.md).

---

## What Gets Added

`FIO_HTTP` exposes:

- `fio_http_settings_s` and `fio_http_listener_s`.
- server APIs: `fio_http_listen`, `fio_http_listener_settings`,
  `fio_http_route`, and `fio_http_route_settings`.
- client API: `fio_http_connect`.
- request routing helper: `fio_http_resource_action` and
  `fio_http_resource_action_e`.
- upgrade helpers for WebSocket and SSE connections.
- pub/sub helpers for upgraded connections.
- handle access helpers: `fio_http_io` and `fio_http_settings`.
- the HTTP handle API from [`431 http handle.h`](./431 http handle.h),
  including request / response fields, headers, cookies, body storage, writing,
  static-file responses, logging, MIME lookup, and controller hooks.

Implementation helpers named `fio___...` are private implementation details.

---

## Connection Model

HTTP/1.x requests and client responses are parsed into `fio_http_s` handles.
The HTTP layer suspends the IO object while the user callback is running and
resumes it after the response is finished or the upgrade is installed.

Plain HTTP cycles use:

1. request / response parsing;
2. optional `pre_http_body` before an `Expect: 100-continue` body is read;
3. `on_http` for application logic;
4. `fio_http_write` / `fio_http_finish` to send the response;
5. `on_finish` when the non-upgraded cycle is complete.

WebSocket and SSE cycles use the authentication callbacks before upgrade, then
`on_open`, message / event callbacks, `on_ready`, `on_shutdown`, `on_close`, and
finally `on_finish` when the upgraded connection closes.

User callbacks are scheduled through the selected HTTP task queue. If
`fio_http_settings_s.queue` is not supplied, the current IO queue is used.

---

## Settings and Listener API

### `fio_http_settings_s`

```c
typedef struct fio_http_settings_s fio_http_settings_s;
```

`fio_http_settings_s` is used by server listeners, routes, and client
connections. The function-style APIs are shadowed by named-argument macros, so
settings are usually passed as designated initializers.

Callback fields:

| Field | Used for |
| --- | --- |
| `pre_http_body` | Called before body upload when the request used `Expect`. |
| `on_http` | Server request callback, or client response callback. |
| `on_finish` | Called when a plain request / response cycle completes, and after upgraded close cleanup. |
| `on_authenticate_sse` | Return non-zero to reject an SSE upgrade. |
| `on_authenticate_websocket` | Return non-zero to reject a WebSocket upgrade. |
| `on_open` | Called once a WebSocket or SSE upgrade is complete. |
| `on_message` | Called with complete WebSocket messages. |
| `on_eventsource` | Called with complete SSE events. |
| `on_eventsource_reconnect` | Called for SSE requests with `Last-Event-ID`. |
| `on_ready` | Called when an upgraded connection's outgoing buffer is empty. |
| `on_shutdown` | Called for open upgraded connections during IO shutdown. |
| `on_close` | Called after an upgraded connection is closed. |
| `on_stop` | Called when HTTP service settings are being closed. |

Other settings:

| Field | Used for |
| --- | --- |
| `udata` | Default `fio_http_udata(h)` value. |
| `tls_io_func`, `tls` | Optional TLS support. |
| `queue` | Optional HTTP task queue. |
| `public_folder` | Static-file root; can serve pre-compressed `.gz` alternatives. |
| `max_age` | Static-file `Cache-Control` max-age value, in seconds. |
| `max_header_size` | Maximum combined request line and header bytes. |
| `max_line_len` | Maximum bytes per request / header line. |
| `max_body_size` | Maximum request body size. |
| `ws_max_msg_size` | Maximum buffered WebSocket message size. |
| `reserved1`, `reserved2` | Reserved for future use. |
| `timeout` | HTTP/1.x connection timeout, in seconds. |
| `ws_timeout` | WebSocket timeout; timeout pings are sent. |
| `sse_timeout` | SSE timeout; timeout pings are sent. |
| `connect_timeout` | Client connection timeout. |
| `log` | Enables HTTP request logging. |
| `compress_static` | Opt-in static-file compression. |
| `compress_dynamic` | Opt-in dynamic response compression. |
| `compress_ws` | Opt-in WebSocket `permessage-deflate`. |

Unset callbacks and limits are normalized when the listener, route, or client is
created. Server `on_http` defaults to a 404 response; client `on_http` defaults
to a no-op. Server upgrade authentication defaults to denial. Client upgrade
authentication defaults to allow.

### `fio_http_listen`

```c
fio_http_listener_s *fio_http_listen(const char *url,
                                     fio_http_settings_s settings);
#define fio_http_listen(url, ...) \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})
```

Starts an HTTP / WebSocket / SSE listener on `url`.

```c
fio_http_listener_s *l = fio_http_listen("0.0.0.0:3000",
                                         .on_http = on_request,
                                         .log = 1);
```

The listener is rooted at `/`; a path component in the listen URL is ignored.
Use `fio_http_route` for per-prefix behavior.

### `fio_http_listener_settings`

```c
fio_http_settings_s *fio_http_listener_settings(fio_http_listener_s *l);
```

Returns the listener's root settings object. Prefer `fio_http_route` for
changing root routing behavior after the listener is created.

### `FIO_HTTP_AUTHENTICATE_ALLOW`

```c
int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h);
```

Authentication callback that always allows the upgrade. Use it for
`.on_authenticate_sse` or `.on_authenticate_websocket` when no gate is needed.

### `fio_http_io` and `fio_http_settings`

```c
fio_io_s *fio_http_io(fio_http_s *h);
fio_http_settings_s *fio_http_settings(fio_http_s *h);
```

`fio_http_io` returns the IO object attached to the handle, when one exists.
`fio_http_settings` returns the route settings matching the handle's original
path, when connection data is available.

---

## Routing

### Prefix routes

```c
int fio_http_route(fio_http_listener_s *listener,
                   const char *url,
                   fio_http_settings_s settings);
#define fio_http_route(listener, url, ...) \
  fio_http_route(listener, url, (fio_http_settings_s){__VA_ARGS__})

fio_http_settings_s *fio_http_route_settings(fio_http_listener_s *l,
                                             const char *url);
```

Routes are best-prefix matches:

- `/` matches everything.
- `/user` matches `/user` and `/user/...`, but not `/user...`.
- More specific prefixes such as `/user/new` win over `/user`.

Route declaration order is not significant unless an existing route is replaced.
Route settings inherit missing listener callbacks and limits, including `udata`,
`on_finish`, `on_stop`, SSE / WebSocket authentication callbacks,
header/body/message limits, timeouts, `public_folder`, and `log`. TLS settings
are ignored on routes.

### Resource action helper

```c
typedef enum {
  FIO_HTTP_RESOURCE_NONE,
  FIO_HTTP_RESOURCE_INDEX,
  FIO_HTTP_RESOURCE_SHOW,
  FIO_HTTP_RESOURCE_NEW,
  FIO_HTTP_RESOURCE_EDIT,
  FIO_HTTP_RESOURCE_CREATE,
  FIO_HTTP_RESOURCE_UPDATE,
  FIO_HTTP_RESOURCE_DELETE,
} fio_http_resource_action_e;

fio_http_resource_action_e fio_http_resource_action(fio_http_s *h);
```

Maps the current method and routed path to a small REST-style action:

| Action | Request shape |
| --- | --- |
| `FIO_HTTP_RESOURCE_INDEX` | `GET /` |
| `FIO_HTTP_RESOURCE_SHOW` | `GET /:id` |
| `FIO_HTTP_RESOURCE_NEW` | `GET /new` |
| `FIO_HTTP_RESOURCE_EDIT` | `GET /:id/edit` |
| `FIO_HTTP_RESOURCE_CREATE` | `PUT`, `POST`, or `PATCH /` |
| `FIO_HTTP_RESOURCE_UPDATE` | `PUT`, `POST`, or `PATCH /:id` |
| `FIO_HTTP_RESOURCE_DELETE` | `DELETE /:id` |

`FIO_HTTP_RESOURCE_NONE` is returned for unsupported shapes or invalid input.

---

## HTTP Client Connections

```c
fio_io_s *fio_http_connect(const char *url,
                           fio_http_s *h,
                           fio_http_settings_s settings);
#define fio_http_connect(url, h, ...) \
  fio_http_connect(url, h, (fio_http_settings_s){__VA_ARGS__})
```

Connects as an HTTP / WebSocket / SSE client. If `h` is `NULL`, a new handle is
created. Missing method, path, query, and `Host` are filled from `url`.

Recognized upgrade schemes:

- `ws://` and `wss://` call `fio_http_websocket_set_request`.
- `sse://` and `sses://` call `fio_http_sse_set_request`.

Client `on_http` receives the response handle. If the response accepts a
WebSocket or SSE upgrade, the connection switches to the upgraded protocol and
uses the upgraded callbacks instead.

---

## WebSocket and SSE Upgrades

### Upgrade tests and request / response setup

These helpers are declared by the HTTP handle and used by the HTTP module:

```c
int  fio_http_websocket_requested(fio_http_s *h);
int  fio_http_websocket_accepted(fio_http_s *h);
void fio_http_upgrade_websocket(fio_http_s *h);
void fio_http_websocket_set_request(fio_http_s *h);

int  fio_http_sse_requested(fio_http_s *h);
int  fio_http_sse_accepted(fio_http_s *h);
void fio_http_upgrade_sse(fio_http_s *h);
void fio_http_sse_set_request(fio_http_s *h);
```

`*_requested` inspects request headers. `*_accepted` inspects client-side
response state and marks the handle as upgraded on success. `fio_http_upgrade_*`
sets the server response state for the upgrade. `*_set_request` prepares a
client request handle.

Server listeners normally call the authentication callbacks and then upgrade
automatically when a request asks for WebSocket or SSE.

### WebSocket write and callback override

```c
int fio_http_websocket_write(fio_http_s *h,
                             const void *buf,
                             size_t len,
                             uint8_t is_text);

int fio_http_on_message_set(fio_http_s *h,
                            void (*on_message)(fio_http_s *,
                                               fio_buf_info_s,
                                               uint8_t));
```

`fio_http_websocket_write` writes one WebSocket message and fails if the handle
is not an established WebSocket. `is_text` selects text vs. binary.

`fio_http_on_message_set` overrides the `on_message` callback for the current
WebSocket connection. Passing `NULL` restores the settings callback. It returns
`-1` when the handle is not ready for this change.

Incoming WebSocket messages are accumulated up to `ws_max_msg_size` and then
scheduled on the HTTP queue. Control frames are handled by the module.

### SSE write

```c
typedef struct {
  fio_buf_info_s id;
  fio_buf_info_s event;
  fio_buf_info_s data;
} fio_http_sse_write_args_s;

int fio_http_sse_write(fio_http_s *h, fio_http_sse_write_args_s args);
#define fio_http_sse_write(h, ...) \
  fio_http_sse_write((h), ((fio_http_sse_write_args_s){__VA_ARGS__}))
```

Writes one UTF-8 SSE event. `data` is required. `id` and `event` are optional.
Multiline data is emitted as repeated `data:` lines. The helper fails if the
handle is not an established SSE connection.

### Pub/sub helpers

```c
#define fio_http_subscribe(h, ...) \
  fio_pubsub_subscribe(.io = fio_http_io(h), __VA_ARGS__)

void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_pubsub_msg_s *msg);
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY(fio_pubsub_msg_s *msg);
void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);
```

`fio_http_subscribe` subscribes using the handle's IO object.

The direct WebSocket callbacks write published messages to the WebSocket. The
plain `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT` helper checks short payloads for
UTF-8 and selects text or binary; the explicit `_TEXT` and `_BINARY` variants
skip that guess.

`FIO_HTTP_SSE_SUBSCRIBE_DIRECT` writes an SSE event whose event name is the
channel and whose data is the published message.

---

## HTTP Handle API Pulled In by `FIO_HTTP`

The HTTP handle can also be included directly with `FIO_HTTP_HANDLE`. The full
HTTP module uses it as the request / response object exposed to callbacks.

### Lifetime

```c
typedef struct fio_http_s fio_http_s;

fio_http_s *fio_http_new(void);
fio_http_s *fio_http_new_copy_request(fio_http_s *old);
void        fio_http_free(fio_http_s *h);
fio_http_s *fio_http_dup(fio_http_s *h);
fio_http_s *fio_http_destroy(fio_http_s *h);
void        fio_http_start_time_set(fio_http_s *h);
fio_http_s *fio_http_clear_response(fio_http_s *h, bool clear_body);
void        fio_http_close(fio_http_s *h);
```

`fio_http_s` is reference-counted. It is not designed as a thread-safe mutable
object. `fio_http_close` closes the persistent connection associated with an
upgraded handle.

### User data and controller data

```c
void *fio_http_udata(fio_http_s *h);
void *fio_http_udata_set(fio_http_s *h, void *udata);
void *fio_http_udata2(fio_http_s *h);
void *fio_http_udata2_set(fio_http_s *h, void *udata);
void *fio_http_cdata(fio_http_s *h);
void *fio_http_cdata_set(fio_http_s *h, void *cdata);

fio_http_controller_s *fio_http_controller(fio_http_s *h);
fio_http_controller_s *fio_http_controller_set(fio_http_s *h,
                                               fio_http_controller_s *c);
```

`udata` and `udata2` are for application state. `cdata` and the controller are
used by the HTTP layer and custom controllers.

### Request / response properties

```c
size_t         fio_http_status(fio_http_s *h);
size_t         fio_http_status_set(fio_http_s *h, size_t status);
fio_str_info_s fio_http_method(fio_http_s *h);
fio_str_info_s fio_http_method_set(fio_http_s *h, fio_str_info_s value);
fio_str_info_s fio_http_opath(fio_http_s *h);
fio_str_info_s fio_http_opath_set(fio_http_s *h, fio_str_info_s value);
fio_str_info_s fio_http_path(fio_http_s *h);
fio_str_info_s fio_http_path_set(fio_http_s *h, fio_str_info_s value);
fio_str_info_s fio_http_query(fio_http_s *h);
fio_str_info_s fio_http_query_set(fio_http_s *h, fio_str_info_s value);
fio_str_info_s fio_http_version(fio_http_s *h);
fio_str_info_s fio_http_version_set(fio_http_s *h, fio_str_info_s value);
int64_t        fio_http_received_at(fio_http_s *h);
int64_t        fio_http_received_at_set(fio_http_s *h, int64_t value);
```

`opath` is the original path before route prefix trimming. `path` is the routed
path visible to the current callback.

`fio_http_status_set(h, 0)` normalizes to `200`; values above the internal range
normalize to `500`.

### State checks and flags

```c
int fio_http_is_clean(fio_http_s *h);
int fio_http_is_finished(fio_http_s *h);
int fio_http_is_streaming(fio_http_s *h);
int fio_http_is_upgraded(fio_http_s *h);
int fio_http_is_websocket(fio_http_s *h);
int fio_http_is_sse(fio_http_s *h);
int fio_http_is_freeing(fio_http_s *h);
```

The handle also exposes inline flag helpers generated for `cflags` and
`uflags`:

```c
uint16_t fio_http_cflags(fio_http_s *h);
uint16_t fio_http_cflags_set(fio_http_s *h, uint16_t flags);
uint16_t fio_http_cflags_unset(fio_http_s *h, uint16_t flags);
uint16_t fio_http_cflags_flip(fio_http_s *h, uint16_t flags);
uint16_t fio_http_cflags_is_set(fio_http_s *h, uint16_t flags);

uint16_t fio_http_uflags(fio_http_s *h);
uint16_t fio_http_uflags_set(fio_http_s *h, uint16_t flags);
uint16_t fio_http_uflags_unset(fio_http_s *h, uint16_t flags);
uint16_t fio_http_uflags_flip(fio_http_s *h, uint16_t flags);
uint16_t fio_http_uflags_is_set(fio_http_s *h, uint16_t flags);
```

Prefer the state helpers unless implementing protocol or controller glue.

### Headers

```c
fio_str_info_s fio_http_request_header(fio_http_s *h,
                                       fio_str_info_s name,
                                       size_t index);
size_t fio_http_request_header_count(fio_http_s *h, fio_str_info_s name);
fio_str_info_s fio_http_request_header_set(fio_http_s *h,
                                           fio_str_info_s name,
                                           fio_str_info_s value);
fio_str_info_s fio_http_request_header_set_if_missing(fio_http_s *h,
                                                      fio_str_info_s name,
                                                      fio_str_info_s value);
fio_str_info_s fio_http_request_header_add(fio_http_s *h,
                                           fio_str_info_s name,
                                           fio_str_info_s value);
size_t fio_http_request_header_each(fio_http_s *h,
                                    int (*callback)(fio_http_s *,
                                                    fio_str_info_s name,
                                                    fio_str_info_s value,
                                                    void *udata),
                                    void *udata);
```

Response headers use the same shape with `response` in the function name:

```c
fio_http_response_header;
fio_http_response_header_count;
fio_http_response_header_set;
fio_http_response_header_set_if_missing;
fio_http_response_header_add;
fio_http_response_header_each;
```

`*_header` returns an empty value if the requested value is missing. Use
`index` for repeated values. If `name.buf == NULL`, `*_header_count` returns
the number of unique header names.

Response headers cannot be changed after the response headers are sent.

### Header parsing helpers

```c
int fio_http_request_header_parse(fio_http_s *h,
                                  fio_str_info_s *buf_parsed,
                                  fio_str_info_s header_name);
int fio_http_response_header_parse(fio_http_s *h,
                                   fio_str_info_s *buf_parsed,
                                   fio_str_info_s header_name);

#define FIO_HTTP_HEADER_EACH_VALUE(http_handle, is_request, header_name, value)
#define FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(value, property)
#define FIO_HTTP_PARSED_HEADER_EACH(buf_parsed, value)
```

The parse helpers copy repeated comma-separated header values into a compact
parsed buffer. The macros iterate values and `name=value` style properties. The
macro form uses a 2048-byte stack buffer for the parse.

### Body storage and reads

```c
size_t         fio_http_body_length(fio_http_s *h);
size_t         fio_http_body_pos(fio_http_s *h);
size_t         fio_http_body_seek(fio_http_s *h, ssize_t pos);
fio_str_info_s fio_http_body_read(fio_http_s *h, size_t length);
fio_str_info_s fio_http_body_read_until(fio_http_s *h,
                                        char token,
                                        size_t limit);
void           fio_http_body_expect(fio_http_s *h, size_t expected_length);
void           fio_http_body_write(fio_http_s *h, const void *data, size_t len);
int            fio_http_body_fd(fio_http_s *h);
```

The handle stores body data in RAM until the configured threshold is crossed,
then uses a temporary file. `fio_http_body_fd` returns that file descriptor or
`-1`.

### Body parsing

```c
typedef struct fio_http_body_parse_callbacks_s fio_http_body_parse_callbacks_s;
typedef struct fio_http_body_parse_result_s fio_http_body_parse_result_s;

fio_http_body_parse_result_s fio_http_body_parse(
    fio_http_s *h,
    const fio_http_body_parse_callbacks_s *callbacks,
    void *udata);
```

The body parser auto-detects JSON, URL-encoded, and multipart/form-data input.
Callbacks build application objects for primitives, arrays, maps, and file
uploads.

`fio_http_body_parse_callbacks_s` contains primitive callbacks (`on_null`,
`on_true`, `on_false`, `on_number`, `on_float`, `on_string`), container
callbacks (`on_array`, `on_map`, `array_push`, `map_set`, `array_done`,
`map_done`), multipart callbacks (`on_file`, `on_file_data`, `on_file_done`),
and error / cleanup callbacks (`on_error`, `free_unused`).

`fio_http_body_parse_result_s` reports the top-level result, consumed bytes,
and error code.

### Cookies

```c
typedef enum fio_http_cookie_same_site_e {
  FIO_HTTP_COOKIE_SAME_SITE_BROWSER_DEFAULT = 0,
  FIO_HTTP_COOKIE_SAME_SITE_NONE,
  FIO_HTTP_COOKIE_SAME_SITE_LAX,
  FIO_HTTP_COOKIE_SAME_SITE_STRICT,
} fio_http_cookie_same_site_e;

typedef struct fio_http_cookie_args_s fio_http_cookie_args_s;

int fio_http_cookie_set(fio_http_s *h, fio_http_cookie_args_s args);
#define fio_http_cookie_set(h, ...) \
  fio_http_cookie_set((h), (fio_http_cookie_args_s){__VA_ARGS__})

fio_str_info_s fio_http_cookie(fio_http_s *h,
                               const char *name,
                               size_t name_len);
size_t fio_http_cookie_each(fio_http_s *h,
                            int (*callback)(fio_http_s *,
                                            fio_str_info_s name,
                                            fio_str_info_s value,
                                            void *udata),
                            void *udata);
size_t fio_http_set_cookie_each(fio_http_s *h,
                                int (*callback)(fio_http_s *,
                                                fio_str_info_s set_cookie,
                                                fio_str_info_s value,
                                                void *udata),
                                void *udata);
```

`fio_http_cookie_args_s` contains `name`, `value`, optional `domain` and `path`,
`max_age`, `same_site`, and the `secure`, `http_only`, and `partitioned` flags.
An empty / missing value deletes the cookie by setting a negative max-age.

### Writing responses

```c
typedef struct fio_http_write_args_s {
  const void *buf;
  size_t len;
  size_t offset;
  int fd;
  void (*dealloc)(void *);
  int copy;
  int finish;
} fio_http_write_args_s;

void fio_http_write(fio_http_s *h, fio_http_write_args_s args);
#define fio_http_write(h, ...) fio_http_write(h, (fio_http_write_args_s){__VA_ARGS__})
#define fio_http_finish(h)    fio_http_write(h, .finish = 1)
```

`fio_http_write` sends headers on the first write and then writes body data. If
`finish` is set, the response is complete. Without `finish`, the response is a
stream. File writes use `fd` and `offset`; the file descriptor is always closed
by the write path.

On upgraded handles, `fio_http_write` routes through the WebSocket or SSE
controller. For WebSocket text/binary choice, call `fio_http_websocket_write`
directly when the payload type matters.

### General helpers

```c
int            fio_http_send_error_response(fio_http_s *h, size_t status);
int            fio_http_etag_is_match(fio_http_s *h);
int            fio_http_static_file_response(fio_http_s *h,
                                             fio_str_info_s root_folder,
                                             fio_str_info_s file_name,
                                             size_t max_age);
fio_str_info_s fio_http_status2str(size_t status);
void           fio_http_write_log(fio_http_s *h);
int            fio_http_from(fio_str_info_s *dest, const fio_http_s *h);
fio_str_info_s fio_http_date(uint64_t now_in_seconds);
fio_str_info_s fio_http_log_time(uint64_t now_in_seconds);
int64_t        fio_http_get_timestump(void);
```

`fio_http_get_timestump` is the inline timestamp helper used by the handle for
`received_at` and log timing. User code usually reads timestamps with
`fio_http_received_at` or formats them with `fio_http_date` /
`fio_http_log_time`.

`fio_http_from` writes a best-effort peer address starting with the `Forwarded`
header, then socket peer address, then `"[unknown]"`.

`fio_http_static_file_response` sends a file from a root folder and finishes the
response on success.

### Path and MIME helpers

```c
#define FIO_HTTP_PATH_EACH(path, pos)

int fio_http_mimetype_register(char *file_ext,
                               size_t file_ext_len,
                               fio_str_info_s mime_type);
fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len);
```

`FIO_HTTP_PATH_EACH` iterates decoded path sections using a 4096-byte stack
buffer. The MIME registry is not thread-safe.

### Controller

```c
typedef struct fio_http_controller_s fio_http_controller_s;

struct fio_http_controller_s {
  uintptr_t private_flags;
  void (*on_destroyed)(fio_http_s *h);
  void (*send_headers)(fio_http_s *h);
  void (*write_body)(fio_http_s *h, fio_http_write_args_s args);
  void (*on_finish)(fio_http_s *h);
  void (*close_io)(fio_http_s *h);
  int  (*get_fd)(fio_http_s *h);
};
```

Controllers allow the handle to write through different protocols. Initialize
custom controllers to zero before use. If controller callbacks are not
thread-safe, call `fio_http_write` only from the controller's expected thread.

---

## Compile-Time Knobs

HTTP module defaults:

```c
FIO_HTTP_DEFAULT_MAX_HEADER_SIZE      /* 32768 */
FIO_HTTP_DEFAULT_MAX_LINE_LEN         /* 8192 */
FIO_HTTP_DEFAULT_MAX_BODY_SIZE        /* 33554432 */
FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE      /* 262144 */
FIO_HTTP_DEFAULT_TIMEOUT              /* 50 */
FIO_HTTP_DEFAULT_TIMEOUT_LONG         /* 50 */
FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER   /* 0 */
FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT
FIO_WEBSOCKET_STATS                   /* 0 */
FIO_HTTP_WEBSOCKET_DEFLATE_MIN        /* 1024 */
```

HTTP handle defaults:

```c
FIO_HTTP_EXACT_LOGGING                /* fuzzy timestamps by default with IO */
FIO_HTTP_BODY_RAM_LIMIT               /* 1 << 17 */
FIO_HTTP_CACHE_LIMIT                  /* 0 */
FIO_HTTP_CACHE_STR_MAX_LEN            /* 1 << 12 */
FIO_HTTP_CACHE_USES_MUTEX             /* 1 */
FIO_HTTP_PRE_CACHE_KNOWN_HEADERS      /* 1 */
FIO_HTTP_DEFAULT_INDEX_FILENAME       /* "index" */
FIO_HTTP_STATIC_FILE_COMPLETION       /* 1 */
FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT   /* 1 << 21 */
FIO_HTTP_LOG_X_REQUEST_START          /* 1 */
FIO_HTTP_ENFORCE_LOWERCASE_HEADERS    /* 0 */
```

State and controller flag macros are exposed for protocol glue:

```c
FIO_HTTP_STATE_STREAMING
FIO_HTTP_STATE_FINISHED
FIO_HTTP_STATE_UPGRADED
FIO_HTTP_STATE_WEBSOCKET
FIO_HTTP_STATE_SSE
FIO_HTTP_STATE_COOKIES_PARSED
FIO_HTTP_STATE_FREEING

FIO_HTTP_CFLAG_COMPRESS_DYNAMIC
FIO_HTTP_CFLAG_COMPRESS_WS
FIO_HTTP_CFLAG_COMPRESS_STATIC
```

Prefer public helper functions over reading state bits directly in application
code.

---

## Minimal Server

```c
#define FIO_HTTP
#include FIO_INCLUDE_FILE

static void on_http(fio_http_s *h) {
  fio_http_response_header_set(h,
                               FIO_STR_INFO1("content-type"),
                               FIO_STR_INFO1("text/plain"));
  fio_http_write(h, .buf = "hello\n", .len = 6, .finish = 1);
}

void start_http(void) {
  fio_http_listen("0.0.0.0:3000", .on_http = on_http, .log = 1);
}
```

For WebSocket or SSE routes, provide the relevant authentication and upgraded
connection callbacks in the route settings.
