## HTTP Server

### Listening for HTTP / WebSockets and EventSource connections

#### `fio_http_listen`

```c
fio_http_listener_s *fio_http_listen(const char *url, fio_http_settings_s settings);
/* Named arguments using macro. */
#define fio_http_listen(url, ...)                                              \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})

typedef struct fio_http_settings_s {
  /** Called before body uploads, when a client sends an `Expect` header. */
  void (*pre_http_body)(fio_http_s *h);
  /** Callback for HTTP requests (server) or responses (client). */
  void (*on_http)(fio_http_s *h);
  /** Called when a request / response cycle is finished with no Upgrade. */
  void (*on_finish)(fio_http_s *h);
  /** Authenticate EventSource (SSE) requests, return non-zero to deny.*/
  int (*on_authenticate_sse)(fio_http_s *h);
  /** Authenticate WebSockets Upgrade requests, return non-zero to deny.*/
  int (*on_authenticate_websocket)(fio_http_s *h);
  /** Called once a WebSocket / SSE connection upgrade is complete. */
  void (*on_open)(fio_http_s *h);
  /** Called when a WebSocket message is received. */
  void (*on_message)(fio_http_s *h, fio_buf_info_s msg, uint8_t is_text);
  /** Called when an EventSource event is received. */
  void (*on_eventsource)(fio_http_s *h,
                         fio_buf_info_s id,
                         fio_buf_info_s event,
                         fio_buf_info_s data);
  /** Called when an EventSource reconnect event requests an ID. */
  void (*on_eventsource_reconnect)(fio_http_s *h, fio_buf_info_s id);
  /** Called for WebSocket / SSE connections when outgoing buffer is empty. */
  void (*on_ready)(fio_http_s *h);
  /** Called for open WebSocket / SSE connections during shutting down. */
  void (*on_shutdown)(fio_http_s *h);
  /** Called after a WebSocket / SSE connection is closed (for cleanup). */
  void (*on_close)(fio_http_s *h);
  /** (optional) the callback to be performed when the HTTP service closes. */
  void (*on_stop)(struct fio_http_settings_s *settings);
  /** Default opaque user data for HTTP handles (fio_http_s). */
  void *udata;
  /** Optional SSL/TLS support. */
  fio_io_functions_s *tls_io_func;
  /** Optional SSL/TLS support. */
  fio_io_tls_s *tls;
  /** Optional HTTP task queue (for multi-threading HTTP responses) */
  fio_io_async_s *queue;
  /** A public folder for file transfers - serves static files. */
  fio_str_info_s public_folder;
  /** Max-age value (in seconds) for caching static files. Defaults to 0. */
  size_t max_age;
  /** Maximum total bytes for request string and headers. */
  uint32_t max_header_size;
  /** Maximum bytes allowed per header / request line. */
  uint32_t max_line_len;
  /** Maximum size of an HTTP request's body. */
  size_t max_body_size;
  /** Maximum WebSocket message size/buffer (in bytes). */
  size_t ws_max_msg_size;
  /** reserved for future use. */
  intptr_t reserved1;
  /** reserved for future use. */
  intptr_t reserved2;
  /** HTTP/1.x connection timeout in seconds. */
  uint8_t timeout;
  /** WebSocket connection timeout in seconds. */
  uint8_t ws_timeout;
  /** EventSource (SSE) connection timeout in seconds. */
  uint8_t sse_timeout;
  /** Timeout for client connections (only relevant in client mode). */
  uint8_t connect_timeout;
  /** Logging flag - set to TRUE to log HTTP requests. */
  uint8_t log;
} fio_http_settings_s;
```

Listens to HTTP / WebSockets / SSE connections on `url`.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
fio_http_listener_s *listener = fio_http_listen("0.0.0.0:3000",
                                                .on_http = my_http_handler,
                                                .log = 1);
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `pre_http_body` | `void (*)(fio_http_s *)` | Called before body uploads when client sends `Expect` header |
| `on_http` | `void (*)(fio_http_s *)` | Callback for HTTP requests (server) or responses (client) |
| `on_finish` | `void (*)(fio_http_s *)` | Called when request/response cycle finishes with no Upgrade |
| `on_authenticate_sse` | `int (*)(fio_http_s *)` | Authenticate SSE requests; return non-zero to deny |
| `on_authenticate_websocket` | `int (*)(fio_http_s *)` | Authenticate WebSocket requests; return non-zero to deny |
| `on_open` | `void (*)(fio_http_s *)` | Called once WebSocket/SSE upgrade is complete |
| `on_message` | `void (*)(fio_http_s *, fio_buf_info_s, uint8_t)` | Called when WebSocket message is received |
| `on_eventsource` | `void (*)(...)` | Called when EventSource event is received |
| `on_eventsource_reconnect` | `void (*)(fio_http_s *, fio_buf_info_s)` | Called when SSE reconnect requests an ID |
| `on_ready` | `void (*)(fio_http_s *)` | Called when outgoing buffer is empty (WS/SSE) |
| `on_shutdown` | `void (*)(fio_http_s *)` | Called for open WS/SSE connections during shutdown |
| `on_close` | `void (*)(fio_http_s *)` | Called after WS/SSE connection is closed |
| `on_stop` | `void (*)(fio_http_settings_s *)` | Called when HTTP service closes |
| `udata` | `void *` | Default opaque user data for HTTP handles |
| `tls_io_func` | `fio_io_functions_s *` | Optional SSL/TLS IO functions |
| `tls` | `fio_io_tls_s *` | Optional SSL/TLS support |
| `queue` | `fio_io_async_s *` | Optional HTTP task queue for multi-threading |
| `public_folder` | `fio_str_info_s` | Public folder for static file serving |
| `max_age` | `size_t` | Max-age for static file caching (seconds); defaults to 0 |
| `max_header_size` | `uint32_t` | Max bytes for request + headers; defaults to `FIO_HTTP_DEFAULT_MAX_HEADER_SIZE` |
| `max_line_len` | `uint32_t` | Max bytes per header line; defaults to `FIO_HTTP_DEFAULT_MAX_LINE_LEN` |
| `max_body_size` | `size_t` | Max body size; defaults to `FIO_HTTP_DEFAULT_MAX_BODY_SIZE` |
| `ws_max_msg_size` | `size_t` | Max WebSocket message size; defaults to `FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE` |
| `timeout` | `uint8_t` | HTTP/1.x timeout in seconds; defaults to `FIO_HTTP_DEFAULT_TIMEOUT` |
| `ws_timeout` | `uint8_t` | WebSocket timeout in seconds; defaults to `FIO_HTTP_DEFAULT_TIMEOUT_LONG` |
| `sse_timeout` | `uint8_t` | SSE timeout in seconds; defaults to `FIO_HTTP_DEFAULT_TIMEOUT_LONG` |
| `connect_timeout` | `uint8_t` | Client connection timeout (client mode only) |
| `log` | `uint8_t` | Set to TRUE to log HTTP requests |

**Returns:** a listener handle (`fio_http_listener_s *`) on success, or NULL on error. The listener can be used with `fio_http_route` to add route-specific handlers.

#### `fio_http_route`

```c
int fio_http_route(fio_http_listener_s *listener,
                   const char *url,
                   fio_http_settings_s settings);
/* Named arguments using macro. */
#define fio_http_route(listener, url, ...)                                     \
  fio_http_route(listener, url, (fio_http_settings_s){__VA_ARGS__})
```

Adds a route prefix to the HTTP handler.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
fio_http_route(listener, "/api",
               .on_http = my_api_handler,
               .log = 1);
```

The order in which `fio_http_route` is called is irrelevant (unless overwriting an existing route).

Matching is performed as a best-prefix match:

- All paths match the route `"/"` (the default prefix).

- Partial URL matches are only valid if the `/` character is the one following the partial match. For example: setting `"/user"` will match `"/user"` and all `"/user/..."` paths but not `"/user..."`

- Setting `"/user/new"` as well as `"/user"` (in whatever order) will route `"/user/new"` and `"/user/new/..."` to `"/user/new"`. Otherwise, the `"/user"` route will continue to behave the same.

**Returns:** 0 on success, -1 on error.

**Note**: the `udata`, `on_finish`, `public_folder` and `log` properties are all inherited (if missing) from the default HTTP settings used to create the listener.

**Note**: TLS options are ignored.

#### `fio_http_route_settings`

```c
fio_http_settings_s *fio_http_route_settings(fio_http_listener_s *listener, const char *url);
```

Returns a link to the settings matching `url`, as set by `fio_http_route`.


#### `fio_http_resource_action`

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

Returns the resource action expected by the request details in the HTTP handle `h`.

If no REST / CRUD style action is detected, FIO_HTTP_RESOURCE_NONE is returned.

- `FIO_HTTP_RESOURCE_INDEX`: will be returned on `GET` `/`.

    Should show the list of available items.

- `FIO_HTTP_RESOURCE_SHOW`: will be returned on `GET` `/:id`.

    Should show selected item(s).

- `FIO_HTTP_RESOURCE_NEW`: will be returned on `GET` `/new`.

    Should return a form for creating an item.

- `FIO_HTTP_RESOURCE_EDIT`: will be returned on `GET` `/:id/edit`.

    Should show a form for editing the selected item(s).

- `FIO_HTTP_RESOURCE_CREATE`: will be returned on `PUT`/`POST`/`PATCH` `/`.

    Should create **or update** an item (if `id` is provided).

- `FIO_HTTP_RESOURCE_UPDATE`: will be returned on `PUT`/`POST`/`PATCH` `/:id`.

    Should update selected item.

- `FIO_HTTP_RESOURCE_DELETE`: will be returned on `DELETE` `/:id`.

    Should delete selected item.

#### `fio_http_listener_settings`

```c
fio_http_settings_s *fio_http_listener_settings(fio_http_listener_s *listener);
```

Returns a pointer to the HTTP settings associated with the listener.

**Note**: changing the settings for the root path should be performed using `fio_http_route` and not by altering the settings directly.

#### `FIO_HTTP_AUTHENTICATE_ALLOW`

```c
int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h);
```

Allows all clients to connect to WebSockets / EventSource (SSE) connections (bypasses authentication), to be used with the `.on_authenticate_sse` and `.on_authenticate_websocket` settings options.

#### `fio_http_connect`

```c
fio_io_s *fio_http_connect(const char *url,
                           fio_http_s *h,
                           fio_http_settings_s settings);
/* Named arguments using macro. */
#define fio_http_connect(url, h, ...)                                          \
  fio_http_connect(url, h, (fio_http_settings_s){__VA_ARGS__})
```

Connects to HTTP / WebSockets / SSE connections on `url`.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
fio_io_s *io = fio_http_connect("wss://example.com/ws", NULL,
                                .on_open = my_on_open,
                                .on_message = my_on_message);
```

- `url` - The URL to connect to. Supports `http://`, `https://`, `ws://`, `wss://`, `sse://`, `sses://` schemes.
- `h` - An optional pre-configured HTTP handle. If NULL, a new handle is created.
- `settings` - Connection settings (see `fio_http_settings_s`).

**Returns:** an IO handle (`fio_io_s *`) on success, or NULL on error.

**Note**: For WebSocket connections, use `ws://` or `wss://` scheme. For SSE connections, use `sse://` or `sses://` scheme.

### Creating an HTTP Handle

These are Used internally by the `FIO_HTTP` module.

#### `fio_http_new`

```c
fio_http_s *fio_http_new(void);
```

Create a new `fio_http_s` handle.

#### `fio_http_new_copy_request`

```c
fio_http_s *fio_http_new_copy_request(fio_http_s *old);
```

Creates a copy of an existing handle, copying only its request data.

#### `fio_http_destroy`

```c
fio_http_s *fio_http_destroy(fio_http_s *h);
```

Destroys the HTTP handle object, freeing all allocated resources.

#### `fio_http_start_time_set`

```c
void fio_http_start_time_set(fio_http_s *);
```

Collects an updated timestamp for logging purposes.

### HTTP Reference Counting

#### `fio_http_free`

```c
void fio_http_free(fio_http_s *);
```

Reduces an `fio_http_s` handle's reference count or frees it.

#### `fio_http_dup`

```c
fio_http_s *fio_http_dup(fio_http_s *);
```

Increases an `fio_http_s` handle's reference count.

### HTTP Request / Response Data

#### `fio_http_udata`

```c
void *fio_http_udata(fio_http_s *);
```

Gets the opaque user pointer associated with the HTTP handle.

#### `fio_http_udata_set`

```c
void *fio_http_udata_set(fio_http_s *, void *);
```

Sets the opaque user pointer associated with the HTTP handle.

#### `fio_http_udata2`

```c
void *fio_http_udata2(fio_http_s *);
```

Gets the second opaque user pointer associated with the HTTP handle.

#### `fio_http_udata2_set`

```c
void *fio_http_udata2_set(fio_http_s *, void *);
```

Sets a second opaque user pointer associated with the HTTP handle.

#### `fio_http_cdata`

```c
void *fio_http_cdata(fio_http_s *h);
```

Returns the existing controller data (`void *` pointer).

This is used internally by the HTTP module to store connection-specific data.

#### `fio_http_cdata_set`

```c
void *fio_http_cdata_set(fio_http_s *h, void *cdata);
```

Sets a new controller data (`void *` pointer).

This is used internally by the HTTP module to store connection-specific data.

#### `fio_http_controller`

```c
fio_http_controller_s *fio_http_controller(fio_http_s *h);
```

Gets the HTTP Controller associated with the HTTP handle.

#### `fio_http_controller_set`

```c
fio_http_controller_s *fio_http_controller_set(fio_http_s *h,
                                               fio_http_controller_s *controller);
```

Sets the HTTP Controller associated with the HTTP handle.

#### `fio_http_io`

```c
fio_io_s *fio_http_io(fio_http_s *);
```

Returns the IO object associated with the HTTP object (request only).


#### `fio_http_settings`

```c
fio_http_settings_s *fio_http_settings(fio_http_s *);
```

Returns the HTTP settings associated with the HTTP object, if any.

#### `fio_http_from`

```c
int fio_http_from(fio_str_info_s *dest, const fio_http_s *h);
```

Writes peer address to `dest` starting with the `forwarded` header, with a fallback to actual socket address and a final fallback to `"[unknown]"`.

If `unknown` is returned, the function returns -1. if `dest` capacity is too small, the number of bytes required will be returned.

If all goes well, this function returns 0.

**Note**: to check the actual socket peer address (not merely the address reported by the client), use [`fio_sock_peer_addr`](#fio_sock_peer_addr). If the request was routed using a proxy, this will provide the proxy's address.


### HTTP State

#### `fio_http_is_clean`

```c
int fio_http_is_clean(fio_http_s *);
```

Returns true if no HTTP headers / data was sent (a clean slate).

#### `fio_http_is_finished`

```c
int fio_http_is_finished(fio_http_s *);
```

Returns true if the HTTP handle's response was sent.

#### `fio_http_is_streaming`

```c
int fio_http_is_streaming(fio_http_s *);
```

Returns true if the HTTP handle's response is streaming.

#### `fio_http_is_upgraded`

```c
int fio_http_is_upgraded(fio_http_s *h);
```

Returns true if the HTTP connection was (or should have been) upgraded.

#### `fio_http_is_websocket`

```c
int fio_http_is_websocket(fio_http_s *);
```

Returns true if the HTTP handle refers to a WebSocket connection.

#### `fio_http_is_sse`

```c
int fio_http_is_sse(fio_http_s *);
```

Returns true if the HTTP handle refers to an EventSource connection.

#### `fio_http_is_freeing`

```c
int fio_http_is_freeing(fio_http_s *);
```

Returns true if the HTTP handle is in the process of freeing itself.

### HTTP Request Data

#### `fio_http_received_at`

```c
int64_t fio_http_received_at(fio_http_s *);
```

Gets the received_at timestamp (ms) associated with the HTTP handle.

**Note**: when `FIO_HTTP_EXACT_LOGGING` is true, this will be in micro-seconds (1/1000000), otherwise milliseconds are used (1/1000). 

#### `fio_http_received_at_set`

```c
int64_t fio_http_received_at_set(fio_http_s *, int64_t);
```

Sets the received_at timestamp (ms) associated with the HTTP handle.

**Note**: this is automatically set by the [`fio_http_new`](#fio_http_new) constructor. **There is no need to call this function**.

**Note**: when `FIO_HTTP_EXACT_LOGGING` is true, this should be set in micro-seconds (1/1000000), otherwise milliseconds should be used (1/1000).

#### `fio_http_status`

```c
size_t fio_http_status(fio_http_s *);
```

Gets the status associated with the HTTP handle (response).

#### `fio_http_status_set`

```c
size_t fio_http_status_set(fio_http_s *, size_t status);
```

Sets the status associated with the HTTP handle (response).

#### `fio_http_method`

```c
fio_str_info_s fio_http_method(fio_http_s *);
```

Gets the method information associated with the HTTP handle.

#### `fio_http_method_set`

```c
fio_str_info_s fio_http_method_set(fio_http_s *, fio_str_info_s);
```

Sets the method information associated with the HTTP handle.

#### `fio_http_path`

```c
fio_str_info_s fio_http_path(fio_http_s *);
```

Gets the path information associated with the HTTP handle.

#### `fio_http_path_set`

```c
fio_str_info_s fio_http_path_set(fio_http_s *, fio_str_info_s);
```

Sets the path information associated with the HTTP handle.

#### `fio_http_opath`

```c
fio_str_info_s fio_http_opath(fio_http_s *);
```

Gets the original / first path associated with the HTTP handle.

This is the path before any routing modifications were applied.

#### `fio_http_opath_set`

```c
fio_str_info_s fio_http_opath_set(fio_http_s *, fio_str_info_s);
```

Sets the original / first path associated with the HTTP handle.

#### `FIO_HTTP_PATH_EACH`

```c
#define FIO_HTTP_PATH_EACH(path, pos)
```

Loops over each section of `path`, decoding percent encoding as necessary. This uses 4096 bytes on the stack.

The macro accepts the following:

- `path`: the path string as a `fio_str_info_s` or `fio_buf_info_s` object (see [`fio_http_path(h)`](#fio_http_path)).

- `pos` : the name of the variable to use for accessing the section. The variable `pos` is a `fio_buf_info_s`.

- `pos_reminder` : automatically available inside the loop body, this `fio_buf_info_s` variable points to the rest of the path (the portion not yet iterated).

**Note**: the macro will break if any path's section length is greater than (about) 4063 bytes.

#### `fio_http_query`

```c
fio_str_info_s fio_http_query(fio_http_s *);
```

Gets the query information associated with the HTTP handle.

#### `fio_http_query_set`

```c
fio_str_info_s fio_http_query_set(fio_http_s *, fio_str_info_s);
```

Sets the query information associated with the HTTP handle.

#### `fio_http_version`

```c
fio_str_info_s fio_http_version(fio_http_s *);
```

Gets the version information associated with the HTTP handle.

#### `fio_http_version_set`

```c
fio_str_info_s fio_http_version_set(fio_http_s *, fio_str_info_s);
```

Sets the version information associated with the HTTP handle.

#### `fio_http_request_header`
```c
fio_str_info_s fio_http_request_header(fio_http_s *,
                                       fio_str_info_s name,
                                       size_t index);
```

Gets the header information associated with the HTTP handle.

Since more than a single value may be associated with a header name, the index may be used to collect subsequent values.

An empty value is returned if no header value is available (or index is exceeded).

#### `fio_http_request_header_count`

```c
size_t fio_http_request_header_count(fio_http_s *, fio_str_info_s name);
```

Returns the number of headers named `name` that were received.

If `name` buffer is `NULL`, returns the number of unique headers (not the number of unique values).


#### `fio_http_request_header_set`

```c
fio_str_info_s fio_http_request_header_set(fio_http_s *,
                                           fio_str_info_s name,
                                           fio_str_info_s value);
```

Sets the header information associated with the HTTP handle.

#### `fio_http_request_header_set_if_missing`

```c
fio_str_info_s fio_http_request_header_set_if_missing(fio_http_s *,
                                       fio_str_info_s name,
                                       fio_str_info_s value);
```

Sets the header information associated with the HTTP handle.

#### `fio_http_request_header_add`

```c
fio_str_info_s fio_http_request_header_add(fio_http_s *,
                                           fio_str_info_s name,
                                           fio_str_info_s value);
```

Adds to the header information associated with the HTTP handle.

#### `fio_http_request_header_each`

```c
size_t fio_http_request_header_each(fio_http_s *,
                                    int (*callback)(fio_http_s *,
                                                    fio_str_info_s name,
                                                    fio_str_info_s value,
                                                    void *udata),
                                    void *udata);
```

Iterates through all request headers (except cookies!).

A non-zero return will stop iteration.

Returns the number of iterations performed. If `callback` is `NULL`, returns the number of headers available (multi-value headers are counted as 1).

#### `fio_http_body_length`
```c
size_t fio_http_body_length(fio_http_s *);
```

Gets the body (payload) length associated with the HTTP handle.

#### `fio_http_body_pos`

```c
size_t fio_http_body_pos(fio_http_s *h);
```

Returns the body's reading position.

#### `fio_http_body_seek`
```c
size_t fio_http_body_seek(fio_http_s *, ssize_t pos);
```

Adjusts the body's reading position. Negative values start at the end.

If `pos == SSIZE_MAX`, returns `fio_http_body_pos`.

#### `fio_http_body_read`
```c
fio_str_info_s fio_http_body_read(fio_http_s *, size_t length);
```

Reads up to `length` of data from the body, returns nothing on EOF.

#### `fio_http_body_read_until`

```c
fio_str_info_s fio_http_body_read_until(fio_http_s *,
                                        char token,
                                        size_t limit);
```

Reads from the body until finding `token`, reaching `limit` or EOF.

Note: `limit` is ignored if zero or if the remaining data is lower than limit.

#### `fio_http_body_expect`
```c
void fio_http_body_expect(fio_http_s *, size_t expected_length);
```

Allocates a body (payload) of (at least) the `expected_length`.


#### `fio_http_body_write`
```c
void fio_http_body_write(fio_http_s *, const void *data, size_t len);
```

Writes `data` to the body (payload) associated with the HTTP handle.

#### `fio_http_body_fd`

```c
int fio_http_body_fd(fio_http_s *);
```

If the body is stored in a temporary file, returns the file's handle.

Otherwise returns -1.

### HTTP Body Parsing

#### `fio_http_body_parse`

```c
fio_http_body_parse_result_s fio_http_body_parse(
    fio_http_s *h,
    const fio_http_body_parse_callbacks_s *callbacks,
    void *udata);
```

Parses the HTTP request body, auto-detecting content type.

Supports JSON, URL-encoded, and multipart/form-data bodies. Calls the appropriate callbacks for each element found.

- `h` - The HTTP handle.
- `callbacks` - Parser callbacks (designed to be static const).
- `udata` - User context passed to all callbacks.

**Returns:** Parse result with top-level object and status.

#### `fio_http_body_parse_callbacks_s`

```c
typedef struct {
  /* ===== Primitives ===== */
  /** NULL / nil was detected. Returns new object. */
  void *(*on_null)(void *udata);
  /** TRUE was detected. Returns new object. */
  void *(*on_true)(void *udata);
  /** FALSE was detected. Returns new object. */
  void *(*on_false)(void *udata);
  /** Number was detected. Returns new object. */
  void *(*on_number)(void *udata, int64_t num);
  /** Float was detected. Returns new object. */
  void *(*on_float)(void *udata, double num);
  /** String was detected. Returns new object. */
  void *(*on_string)(void *udata, const void *data, size_t len);

  /* ===== Containers ===== */
  /** Array was detected. Returns context for this array. */
  void *(*on_array)(void *udata, void *parent);
  /** Map / Object was detected. Returns context for this map. */
  void *(*on_map)(void *udata, void *parent);
  /** Push value to array. Returns non-zero on error. */
  int (*array_push)(void *udata, void *array, void *value);
  /** Set key-value pair in map. Returns non-zero on error. */
  int (*map_set)(void *udata, void *map, void *key, void *value);
  /** Called when array parsing is complete. */
  void (*array_done)(void *udata, void *array);
  /** Called when map parsing is complete. */
  void (*map_done)(void *udata, void *map);

  /* ===== File Uploads (multipart) ===== */
  /** Called when a file upload starts. Return NULL to skip this file. */
  void *(*on_file)(void *udata,
                   fio_str_info_s name,
                   fio_str_info_s filename,
                   fio_str_info_s content_type);
  /** Called for each chunk of file data. Return non-zero to abort. */
  int (*on_file_data)(void *udata, void *file, fio_buf_info_s data);
  /** Called when file upload is complete. */
  void (*on_file_done)(void *udata, void *file);

  /* ===== Error Handling ===== */
  /** Called on parse error. `partial` is the incomplete result, if any. */
  void *(*on_error)(void *udata, void *partial);
  /** Called to free an unused object (e.g., key when map_set fails). */
  void (*free_unused)(void *udata, void *obj);
} fio_http_body_parse_callbacks_s;
```

HTTP body parser callbacks. All callbacks receive `udata` as first parameter.

#### `fio_http_body_parse_result_s`

```c
typedef struct {
  /** Top-level parsed object (caller responsible for freeing). */
  void *result;
  /** Number of bytes consumed from body. */
  size_t consumed;
  /** Error code: 0 = success. */
  int err;
} fio_http_body_parse_result_s;
```

HTTP body parse result.

#### HTTP Cookies


#### `fio_http_cookie_set` - `same_site`

```c
typedef enum fio_http_cookie_same_site_e {
  /** allow the browser to dictate this property */
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

Possible values for the `same_site` property in the cookie settings.

See: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie


#### `fio_http_cookie_set`
```c
int fio_http_cookie_set(fio_http_s *h, fio_http_cookie_args_s);
/** Named arguments helper. See fio_http_cookie_args_s for details. */
#define fio_http_cookie_set(http___handle, ...)                                \
  fio_http_cookie_set((http___handle), (fio_http_cookie_args_s){__VA_ARGS__})

```

Sets a response cookie.

Returns -1 on error and 0 on success.

Note: Long cookie names and long cookie values will be considered a security violation and an error will be returned. Many browsers and proxies impose limits on headers and cookies, cookies often limited to 4Kb in total for both name and value.

```c
typedef struct fio_http_cookie_args_s {
  /** The cookie's name. */
  fio_str_info_s name;
  /** The cookie's value (leave blank to delete cookie). */
  fio_str_info_s value;
  /** The cookie's domain (optional). */
  fio_str_info_s domain;
  /** The cookie's path (optional). */
  fio_str_info_s path;
  /** Max Age (how long should the cookie persist), in seconds (0 == session).*/
  int max_age;
  /** SameSite value. */
  fio_http_cookie_same_site_e same_site;
  /** Limit cookie to secure connections.*/
  unsigned secure : 1;
  /** Limit cookie to HTTP (intended to prevent JavaScript access/hijacking).*/
  unsigned http_only : 1;
  /**
   * Set the Partitioned (third party) cookie flag:
   * https://developer.mozilla.org/en-US/docs/Web/Privacy/Partitioned_cookies
   */
  unsigned partitioned : 1;
} fio_http_cookie_args_s;
```

#### `fio_http_cookie`

```c
fio_str_info_s fio_http_cookie(fio_http_s *,
                               const char *name,
                               size_t name_len);
```

Returns a cookie value (either received of newly set), if any.

#### `fio_http_cookie_each`

```c
size_t fio_http_cookie_each(fio_http_s *,
                            int (*callback)(fio_http_s *,
                                            fio_str_info_s name,
                                            fio_str_info_s value,
                                            void *udata),
                            void *udata);
```

Iterates through all cookies. A non-zero return will stop iteration.

#### `fio_http_set_cookie_each`

```c
size_t fio_http_set_cookie_each(fio_http_s *h,
                         int (*callback)(fio_http_s *,
                                         fio_str_info_s set_cookie_header,
                                         fio_str_info_s value,
                                         void *udata),
                         void *udata);
```

Iterates through all response set cookies.

A non-zero return value from the callback will stop iteration.

### HTTP Response Data

#### `fio_http_response_header`

```c
fio_str_info_s fio_http_response_header(fio_http_s *,
                                        fio_str_info_s name,
                                        size_t index);
```

Gets the header information associated with the HTTP handle.

Since more than a single value may be associated with a header name, the index may be used to collect subsequent values.

An empty value is returned if no header value is available (or index is exceeded).

If the response headers were already sent, the returned value is always empty.


#### `fio_http_response_header_count`

```c
size_t fio_http_response_header_count(fio_http_s *, fio_str_info_s name);
```

Returns the number of headers named `name` in the response.

If `name` buffer is `NULL`, returns the number of unique headers (not the number of unique values).

#### `fio_http_response_header_set`

```c
fio_str_info_s fio_http_response_header_set(fio_http_s *,
                                            fio_str_info_s name,
                                            fio_str_info_s value);
```

Sets the header information associated with the HTTP handle.

If the response headers were already sent, the returned value is always empty.

#### `fio_http_response_header_set_if_missing`

```c
fio_str_info_s fio_http_response_header_set_if_missing(fio_http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value);
```

Sets the header information associated with the HTTP handle.

If the response headers were already sent, the returned value is always empty.

#### `fio_http_response_header_add`

```c
fio_str_info_s fio_http_response_header_add(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);
```

Adds to the header information associated with the HTTP handle.

If the response headers were already sent, the returned value is always empty.

#### `fio_http_response_header_each`

```c
size_t fio_http_response_header_each(fio_http_s *,
                                     int (*callback)(fio_http_s *,
                                                     fio_str_info_s name,
                                                     fio_str_info_s value,
                                                     void *udata),
                                     void *udata);
```

Iterates through all response headers (except cookies!).

A non-zero return will stop iteration.

### HTTP Writing to the Connection

#### `fio_http_write`

```c
void fio_http_write(fio_http_s *, fio_http_write_args_s args);

/** Named arguments helper. See fio_http_write and fio_http_write_args_s. */
#define fio_http_write(http_handle, ...)                                       \
  fio_http_write(http_handle, (fio_http_write_args_s){__VA_ARGS__})
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)
```

Writes `data` to the response body associated with the HTTP handle after sending all headers (no further headers may be sent).

Accepts the following (possibly named) arguments:

```c
/** Arguments for the fio_http_write function. */
typedef struct fio_http_write_args_s {
  /** The data to be written. */
  const void *buf;
  /** The length of the data to be written. */
  size_t len;
  /** The offset at which writing should begin. */
  size_t offset;
  /** If streaming a file, set this value. The file is always closed. */
  int fd;
  /** If the data is a buffer, this callback may be set to free it once sent. */
  void (*dealloc)(void *);
  /** If the data is a buffer it will be copied. */
  int copy;
  /**
   * If `finish` is set, this data marks the end of the response.
   *
   * Otherwise the response will stream the data.
   */
  int finish;
} fio_http_write_args_s;
```

**Note**: this function will route to [`fio_http_websocket_write`](#fio_http_websocket_write) and [`fio_http_sse_write`](#fio_http_sse_write) if appropriate. However, for WebSocket connections it may be more prudent to decide if a binary or UTF-8 write operation should be performed by calling [`fio_http_websocket_write`](#fio_http_websocket_write) directly.
  
#### `fio_http_finish`

```c
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)
```

Calls [`fio_http_write`](#fio_http_write) without any data to be sent.


#### `fio_http_send_error_response`

```c
int fio_http_send_error_response(fio_http_s *h, size_t status);
```

Sends the requested error message and finishes the response.

**Returns:** 0 on success, -1 on error (e.g., if headers were already sent).

#### `fio_http_etag_is_match`

```c
int fio_http_etag_is_match(fio_http_s *h);
```

Returns true (1) if the ETag response matches an if-none-match request.

#### `fio_http_static_file_response`

```c
int fio_http_static_file_response(fio_http_s *h,
                                  fio_str_info_s root_folder,
                                  fio_str_info_s file_name,
                                  size_t max_age);
```

Attempts to send a static file from the `root` folder.

On success the response is complete and 0 is returned. Otherwise returns -1.

#### `fio_http_status2str`

```c
fio_str_info_s fio_http_status2str(size_t status);
```

Returns a human readable string related to the HTTP status number.

#### `fio_http_write_log`

```c
void fio_http_write_log(fio_http_s *h);
```

Logs an HTTP (response) to STDOUT using common log format:

```txt
[PID] ADDR - - [DATE/TIME] REQ RES_CODE BYTES_SENT TIME_SPENT_IN_APP <(wait PROXY_DELAY)>
```

See also the `FIO_HTTP_LOG_X_REQUEST_START` and `FIO_HTTP_EXACT_LOGGING` compilation flags.

#### `fio_http_date`

```c
fio_str_info_s fio_http_date(uint64_t now_in_seconds);
```

Returns a cached date/time string for HTTP date headers (RFC 7231 format).

The string is cached and updated when the timestamp changes.

#### `fio_http_log_time`

```c
fio_str_info_s fio_http_log_time(uint64_t now_in_seconds);
```

Returns a cached date/time string for HTTP logging.

The string is cached and updated when the timestamp changes.

#### `fio_http_clear_response`

```c
fio_http_s *fio_http_clear_response(fio_http_s *h, bool clear_body);
```

Clears any response data from the HTTP handle.

If `clear_body` is true, also clears the body data.

Returns the HTTP handle.

### HTTP WebSocket / SSE Helpers

#### `fio_http_websocket_requested`

```c
int fio_http_websocket_requested(fio_http_s *);
```

Returns non-zero if request headers ask for a WebSockets Upgrade.

#### `fio_http_websocket_accepted`

```c
int fio_http_websocket_accepted(fio_http_s *h);
```

Returns non-zero if the response accepts a WebSocket upgrade request.

This is useful for client-side code to check if the server accepted the WebSocket upgrade.

#### `fio_http_upgrade_websocket`

```c
void fio_http_upgrade_websocket(fio_http_s *);
```

Sets response data to agree to a WebSockets Upgrade.

#### `fio_http_websocket_set_request`

```c
void fio_http_websocket_set_request(fio_http_s *);
```

Sets request data to request a WebSockets Upgrade.

#### `fio_http_sse_requested`

```c
int fio_http_sse_requested(fio_http_s *);
```

Returns non-zero if request headers ask for an EventSource (SSE) Upgrade.

#### `fio_http_sse_accepted`

```c
int fio_http_sse_accepted(fio_http_s *h);
```

Returns non-zero if the response accepts an SSE request.

This is useful for client-side code to check if the server accepted the SSE upgrade.

#### `fio_http_upgrade_sse`

```c
void fio_http_upgrade_sse(fio_http_s *);
```

Sets response data to agree to an EventSource (SSE) Upgrade.

#### `fio_http_sse_set_request`

```c
void fio_http_sse_set_request(fio_http_s *);
```

Sets request data to request an EventSource (SSE) Upgrade.

#### `fio_http_websocket_write`

```c
int fio_http_websocket_write(fio_http_s *h, const void *buf, size_t len, uint8_t is_text);
```

Writes a WebSocket message. Fails if connection wasn't upgraded yet.

**Note**: calls to the HTTP handle function `fio_http_write` may route to this function after the library performs a best guess attempt at the correct `is_text`.

#### `fio_http_on_message_set`

```c
int fio_http_on_message_set(fio_http_s *h,
                            void (*on_message)(fio_http_s *,
                                               fio_buf_info_s,
                                               uint8_t));
```

Sets a specific `on_message` callback for the WebSocket connection.

Returns `-1` on error (i.e., upgrade still being negotiated).

### EventSource (SSE) Helpers - HTTP Upgraded Connections

#### `fio_http_sse_write`

```c
int fio_http_sse_write(fio_http_s *h, fio_http_sse_write_args_s args);
#define fio_http_sse_write(h, ...)                                             \
  fio_http_sse_write((h), ((fio_http_sse_write_args_s){__VA_ARGS__}))
```

Writes an SSE message (UTF-8). Fails if connection wasn't upgraded yet.

The MACRO shadowing the function enables the used of the named arguments listed in `fio_http_sse_write_args_s`:

```c
/** Named arguments for fio_http_sse_write. */
typedef struct {
  /** The message's `id` data (if any). */
  fio_buf_info_s id;
  /** The message's `event` data (if any). */
  fio_buf_info_s event;
  /** The message's `data` data (if any). */
  fio_buf_info_s data;
} fio_http_sse_write_args_s;
```

**Note**: calls to the HTTP handle function `fio_http_write` may route to this function, in which case both `event` and `id` are omitted.

#### `fio_http_close`

```c
void fio_http_close(fio_http_s *h);
```

Closes a persistent HTTP connection (i.e., if upgraded). 



### HTTP Pub/Sub

#### `fio_http_subscribe`

```c
#define fio_http_subscribe(h, ...)                                             \
  fio_pubsub_subscribe(.io = fio_http_io(h), __VA_ARGS__)
```

Macro helper for HTTP handle pub/sub subscriptions.

This macro wraps `fio_pubsub_subscribe`, automatically setting the `io` argument to the IO object associated with the HTTP handle.

Example:

```c
fio_http_subscribe(h, .channel = FIO_STR_INFO1("chat"),
                      .on_message = my_on_message_callback);
```

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT`

```c
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);
```

Optional WebSocket subscription callback that directly writes the content of the published message to the WebSocket connection.

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT`

```c
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_pubsub_msg_s *msg);
```

Optional WebSocket subscription callback that directly writes the content of the published message to the WebSocket connection - this callback assumes that all messages are UTF-8 valid.

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY`

```c
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY(fio_pubsub_msg_s *msg);
```

Optional WebSocket subscription callback that directly writes the content of the published message to the WebSocket connection - this callback assumes that all messages are binary (non-UTF-8).

#### `FIO_HTTP_SSE_SUBSCRIBE_DIRECT`

```c
void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);
```

An optional EventSource subscription callback - messages MUST be UTF-8.

This callback directly calls `fio_http_sse_write`, placing the channel name in the `.event` argument and the published message in the `.data` argument.

### HTTP Header Parsing Helpers

#### `fio_http_response_header_parse`

```c
int fio_http_response_header_parse(fio_http_s *h,
                                   fio_str_info_s *buf_parsed,
                                   fio_str_info_s header_name);
```

Copies all header data, from possibly an array of identical response headers, resulting in a parsed format outputted to `buf_parsed`.

Returns 0 on success or -1 on error (i.e., `buf_parsed.capa` wasn't enough for the parsed output).

Note that the parsed output isn't readable as a string, but is designed to work with the `FIO_HTTP_PARSED_HEADER_EACH` and `FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` property.

See also `fio_http_response_header_parse`.

#### `fio_http_request_header_parse`

```c
int fio_http_request_header_parse(fio_http_s *h,
                                  fio_str_info_s *buf_parsed,
                                  fio_str_info_s header_name);
```
Copies all header data, from possibly an array of identical response headers,
resulting in a parsed format outputted to `buf_parsed`.

Returns 0 on success or -1 on error (i.e., `buf_parsed.capa` wasn't enough
for the parsed output).

Note that the parsed output isn't readable as a string, but is designed to
work with the `FIO_HTTP_PARSED_HEADER_EACH` and
`FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` property.

i.e.:

```c
 FIO_STR_INFO_TMP_VAR(buf, 1023); // tmp buffer for the parsed output
 fio_http_s *h = fio_http_new();  // using a mock HTTP handle
 fio_http_request_header_add(
     h,
     FIO_STR_INFO2("accept", 6),
     FIO_STR_INFO1("text/html, application/json;q=0.9; d=500, image/png"));
 fio_http_request_header_add(h,
                             FIO_STR_INFO2("accept", 6),
                             FIO_STR_INFO1("text/yaml"));
 FIO_ASSERT(  // in production do NOT assert, but route to error instead!
     !fio_http_request_header_parse(h, &buf, FIO_STR_INFO2("accept", 6)),
     "parse returned error!");
 FIO_HTTP_PARSED_HEADER_EACH(buf, value) {
   printf("* processing value (%zu bytes): %s\n", value.len, value.buf);
   FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(value, prop) {
     printf("* for value %s: (%zu,%zu bytes) %s = %s\n",
            value.buf,
            prop.name.len,
            prop.value.len,
            prop.name.buf,
            prop.value.buf);
   }
 }
```

#### `FIO_HTTP_HEADER_EACH_VALUE`
```c
#define FIO_HTTP_HEADER_EACH_VALUE(/* fio_http_s */ http_handle,               \
                                   /* int / bool */ is_request,                \
                                   /* fio_str_info_s */ header_name,           \
                                   /* chosen var named */ value)               \
  for (char fio___buf__##value##__[2048], /* allocate buffer on stack */       \
           *fio___buf__##value##_ptr = NULL;                                   \
       !fio___buf__##value##_ptr;                                              \
       fio___buf__##value##_ptr = fio___buf__##value##__)                      \
    for (fio_str_info_s fio___buf__##value##__str = /* declare buffer var */   \
         FIO_STR_INFO3(fio___buf__##value##__, 0, 2048);                       \
         fio___buf__##value##__str.buf == fio___buf__##value##__;              \
         fio___buf__##value##__str.buf = fio___buf__##value##__ + 1)           \
      if (!((is_request ? fio_http_request_header_parse                        \
                        : fio_http_response_header_parse)(                     \
              http_handle, /* parse headers */                                 \
              &fio___buf__##value##__str,                                      \
              header_name)))                                                   \
  FIO_HTTP_PARSED_HEADER_EACH(fio___buf__##value##__str, value) /* loop */

/** Iterates through the properties associated with a parsed header value. */
#define FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(/* fio_str_info_s   */ value,      \
                                            /* chosen var named */ property)

/** Used internally to iterate over a parsed header buffer. */
#define FIO_HTTP_PARSED_HEADER_EACH(/* fio_str_info_s   */ buf_parsed,         \
                                    /* chosen var named */ value)
```

Parses header for multiple values and properties and iterates over all values.

This MACRO will allocate 2048 bytes on the stack for parsing the header values and properties, if more space is necessary dig deeper.

Use `FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` to iterate over a value's properties.

### HTTP Mime-Type Registry

MIME File Type Helpers - NOT thread safe!

#### `fio_http_mimetype_register`

```c
int fio_http_mimetype_register(char *file_ext,
                               size_t file_ext_len,
                               fio_str_info_s mime_type);
```

Registers a Mime-Type to be associated with the file extension.

#### `fio_http_mimetype`

```c
fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len);
```

Finds the Mime-Type associated with the file extension (if registered).

### HTTP Controller

The HTTP Controller manages all the callbacks required by the HTTP Handler in order for HTTP responses and requests to be sent.

This allows the HTTP Handler to be somewhat protocol agnostic.

#### `fio_http_controller_s`

```c
struct fio_http_controller_s {
  /* MUST be initialized to zero, used internally by the HTTP Handle. */
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

**Members:**

- `private_flags` - Internal use only; must be initialized to zero.
- `on_destroyed` - Called when an HTTP handle is freed.
- `send_headers` - Informs the controller that request/response headers must be sent.
- `write_body` - Called by the HTTP handle for each body chunk, or to finish a response.
- `on_finish` - Called once a request/response had finished.
- `close_io` - Called to close an HTTP connection.
- `get_fd` - Called when the file descriptor is directly required.

**Note**: if the controller callbacks aren't thread-safe, then the `fio_http_write` function MUST NOT be called from any thread except the thread that the controller is expecting.

### Compilation Flags and Default HTTP Handle Behavior

#### `FIO_HTTP_EXACT_LOGGING`

```c
#ifndef FIO_HTTP_EXACT_LOGGING
#define FIO_HTTP_EXACT_LOGGING 0
#endif
```

By default, facil.io logs the HTTP request cycle using a fuzzy starting and ending point for the time stamp.

The fuzzy timestamp includes delays that aren't related to the HTTP request and may ignore time passed due to timestamp caching.

On the other hand, `FIO_HTTP_EXACT_LOGGING` collects exact time stamps to measure the time it took to process the HTTP request (excluding time spent reading / writing the data from the network).

Due to the preference to err on the side of higher performance, fuzzy time-stamping is the default.


#### `FIO_HTTP_LOG_X_REQUEST_START`

```c
#ifndef FIO_HTTP_LOG_X_REQUEST_START
#define FIO_HTTP_LOG_X_REQUEST_START 1
#endif
```

If set, logs will react to an `X-Request-Start` header that provides time in milliseconds.

An additional `(wait XXms)` data point will be provided in the logs to inform of the delay between the proxy server's `X-Request-Start` start time and the application's start time.

#### `FIO_HTTP_BODY_RAM_LIMIT`

```c
#ifndef FIO_HTTP_BODY_RAM_LIMIT
#define FIO_HTTP_BODY_RAM_LIMIT (1 << 17)
#endif
```

The HTTP handle automatically switches between RAM storage and file storage once the HTTP body (payload) reaches a certain size. This control this point of transition

#### `FIO_HTTP_CACHE_LIMIT`

```c
#ifndef FIO_HTTP_CACHE_LIMIT
#define FIO_HTTP_CACHE_LIMIT 0 /* ((1UL << 6) + (1UL << 5)) */
#endif
```

Each of the HTTP String Caches will be limited to this String count.

#### `FIO_HTTP_CACHE_STR_MAX_LEN`

```c
#ifndef FIO_HTTP_CACHE_STR_MAX_LEN
#define FIO_HTTP_CACHE_STR_MAX_LEN (1 << 12)
#endif
```

The HTTP handle will avoid caching strings longer than this value.

#### `FIO_HTTP_CACHE_USES_MUTEX`

```c
#ifndef FIO_HTTP_CACHE_USES_MUTEX
#define FIO_HTTP_CACHE_USES_MUTEX 1
#endif
```

The HTTP cache will use a mutex to allow headers to be set concurrently.

#### `FIO_HTTP_PRE_CACHE_KNOWN_HEADERS`

```c
#ifndef FIO_HTTP_PRE_CACHE_KNOWN_HEADERS
#define FIO_HTTP_PRE_CACHE_KNOWN_HEADERS 1
#endif
```

Adds a static cache for common HTTP header names.

#### `FIO_HTTP_DEFAULT_INDEX_FILENAME`

```c
#ifndef FIO_HTTP_DEFAULT_INDEX_FILENAME
#define FIO_HTTP_DEFAULT_INDEX_FILENAME "index"
#endif
```

The default file name when a static file response points to a folder.

#### `FIO_HTTP_STATIC_FILE_COMPLETION`

```c
#ifndef FIO_HTTP_STATIC_FILE_COMPLETION
#define FIO_HTTP_STATIC_FILE_COMPLETION 1
#endif
```

Attempts to auto-complete static file paths with missing extensions.

#### `FIO_HTTP_ENFORCE_LOWERCASE_HEADERS`

```c
#ifndef FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
#define FIO_HTTP_ENFORCE_LOWERCASE_HEADERS 0
#endif
```

If true, the HTTP handle will copy input header names to lower case.

### Compilation Flags and Default HTTP Connection Settings

#### `FIO_HTTP_DEFAULT_MAX_HEADER_SIZE`

```c
#ifndef FIO_HTTP_DEFAULT_MAX_HEADER_SIZE
#define FIO_HTTP_DEFAULT_MAX_HEADER_SIZE 32768 /* (1UL << 15) */
#endif
```
#### `FIO_HTTP_DEFAULT_MAX_LINE_LEN`

```c
#ifndef FIO_HTTP_DEFAULT_MAX_LINE_LEN
#define FIO_HTTP_DEFAULT_MAX_LINE_LEN 8192 /* (1UL << 13) */
#endif
```
#### `FIO_HTTP_DEFAULT_MAX_BODY_SIZE`

```c
#ifndef FIO_HTTP_DEFAULT_MAX_BODY_SIZE
#define FIO_HTTP_DEFAULT_MAX_BODY_SIZE 33554432 /* (1UL << 25) */
#endif
```
#### `FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE`

```c
#ifndef FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE
#define FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE 262144 /* (1UL << 18) */
#endif
```
#### `FIO_HTTP_DEFAULT_TIMEOUT`

```c
#ifndef FIO_HTTP_DEFAULT_TIMEOUT
#define FIO_HTTP_DEFAULT_TIMEOUT 50
#endif
```
#### `FIO_HTTP_DEFAULT_TIMEOUT_LONG`

```c
#ifndef FIO_HTTP_DEFAULT_TIMEOUT_LONG
#define FIO_HTTP_DEFAULT_TIMEOUT_LONG 50
#endif
```

#### `FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER`

```c
#ifndef FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER
#define FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER 0
#endif
```

Adds a "content-length" header to the HTTP handle (usually redundant).

#### `FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT`

```c
#ifndef FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT
#define FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT ((1UL << 16) - 10UL)
#endif
```

UTF-8 validity tests will be performed only for data shorter than this.

#### `FIO_WEBSOCKET_STATS`

```c
#ifndef FIO_WEBSOCKET_STATS
#define FIO_WEBSOCKET_STATS 0
#endif
```

If true, logs longest WebSocket round-trips (using `FIO_LOG_INFO`).

------------------------------------------------------------
