## HTTP Server

### Listening for HTTP / WebSockets and EventSource connections

#### `fio_http_listen`

```c
void * fio_http_listen(const char *url, fio_http_settings_s settings);

#define fio_http_listen(url, ...)                                              \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})
```

Listens to HTTP / WebSockets / SSE connections on `url`.

The MACRO shadowing the function enables the used of named arguments for the `fio_http_settings_s`.

Returns a listener handle (same as `fio_srv_listen`). Listening can be stopped using `fio_srv_listen_stop`.

```c
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
  /**
   * A public folder for file transfers - allows to circumvent any application
   * layer logic and simply serve static files.
   *
   * Supports automatic `gz` pre-compressed alternatives.
   */
  fio_str_info_s public_folder;
  /**
   * The max-age value (in seconds) for caching static files send from
   * `public_folder`.
   *
   * Defaults to 0 (not sent).
   */
  size_t max_age;
  /**
   * The maximum total of bytes for the overall size of the request string and
   * headers, combined.
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_HEADER_SIZE bytes.
   */
  uint32_t max_header_size;
  /**
   * The maximum number of bytes allowed per header / request line.
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_LINE_LEN bytes.
   */
  uint32_t max_line_len;
  /**
   * The maximum size of an HTTP request's body (posting / downloading).
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_BODY_SIZE bytes.
   */
  size_t max_body_size;
  /**
   * The maximum WebSocket message size/buffer (in bytes) for Websocket
   * connections. Defaults to FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE bytes.
   */
  size_t ws_max_msg_size;
  /** reserved for future use. */
  intptr_t reserved1;
  /** reserved for future use. */
  intptr_t reserved2;
  /**
   * An HTTP/1.x connection timeout.
   *
   * Defaults to FIO_HTTP_DEFAULT_TIMEOUT seconds.
   *
   * Note: the connection might be closed (by other side) before timeout occurs.
   */
  uint8_t timeout;
  /**
   * Timeout for the WebSocket connections in seconds. Defaults to
   * FIO_HTTP_DEFAULT_TIMEOUT_LONG seconds.
   *
   * A ping will be sent whenever the timeout is reached.
   *
   * Connections are only closed when a ping cannot be sent (the network layer
   * fails). Pongs are ignored.
   */
  uint8_t ws_timeout;
  /**
   * Timeout for EventSource (SSE) connections in seconds. Defaults to
   * FIO_HTTP_DEFAULT_TIMEOUT_LONG seconds.
   *
   * A ping will be sent whenever the timeout is reached.
   *
   * Connections are only closed when a ping cannot be sent (the network layer
   * fails).
   */
  uint8_t sse_timeout;
  /** Timeout for client connections (only relevant in client mode). */
  uint8_t connect_timeout;
  /** Logging flag - set to TRUE to log HTTP requests. */
  uint8_t log;
} fio_http_settings_s;
```

#### `fio_http_route`

```c
int fio_http_route(fio_http_listener_s *listener,
                         const char *url,
                         fio_http_settings_s settings);
#define fio_http_route(listener, url, ...)                                     \
  fio_http_route(listener, url, (fio_http_settings_s){__VA_ARGS__})
```

Adds a route prefix to the HTTP handler.

The order in which `fio_http_route` are called is irrelevant (unless overwriting an existing route).

Matching is performed as a best-prefix match. i.e.:

- All paths match the prefix `"/"` (the default prefix).

- Setting `"/user"` will match all `"/user/..."` paths but not `"/userX..."`

**Note**: the `udata`, `on_finish` and `public_folder` properties are all inherited (if missing) from the default HTTP settings used to create the listener.

**Note**: TLS options are ignored.

#### `fio_http_listener_settings`

```c
fio_http_settings_s *fio_http_listener_settings(void *listener);
```

Returns the a pointer to the HTTP settings associated with the listener.

**Note**: changing the settings for the root path should be performed using `fio_http_route` and not by altering the settings directly.

#### `FIO_HTTP_AUTHENTICATE_ALLOW`

```c
int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h);
```

Allows all clients to connect to WebSockets / EventSource (SSE) connections (bypasses authentication), to be used with the `.on_authenticate_sse` and `.on_authenticate_websocket` settings options.

#### `fio_http_connect`

```c
fio_s *fio_http_connect(const char *url,
                              fio_http_s *h,
                              fio_http_settings_s settings);
/* Shadow the function for named arguments */
#define fio_http_connect(url, h, ...)                                          \
  fio_http_connect(url, h, (fio_http_settings_s){__VA_ARGS__})

```

Connects to HTTP / WebSockets / SSE connections on `url`.

Accepts named arguments for the `fio_http_settings_s` settings.

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

Destroyed the HTTP handle object, freeing all allocated resources.

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

#### `fio_http_io`

```c
fio_s *fio_http_io(fio_http_s *);
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

### HTTP Request Data

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

#### fio_http_set_cookie_each

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

Accepts the followung (possibly named) arguments:

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
void fio_http_send_error_response(fio_http_s *h, size_t status);
```

Sends the requested error message and finishes the response.

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

### HTTP WebSocket / SSE Helpers

#### `fio_http_websocket_requested`

```c
int fio_http_websocket_requested(fio_http_s *);
```

Returns non-zero if request headers ask for a WebSockets Upgrade.

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
int fio_http_subscribe(fio_http_s *h, fio_subscribe_args_s args);

#define fio_http_subscribe(h, ...) fio_http_subscribe((h), ((fio_subscribe_args_s){__VA_ARGS__}))
```

Subscribes the HTTP handle (WebSocket / SSE) to events. Requires an Upgraded connection.

Automatically sets the correct `io` object and the default callback if none was provided (either `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT` or `FIO_HTTP_SSE_SUBSCRIBE_DIRECT`).

Using a `NULL` HTTP handle (`fio_http_s *`) is the same as calling `fio_subscribe` without an `io` object.

Returns `-1` on error (i.e., upgrade still being negotiated).

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT`

```c
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_msg_s *msg);
```

Optional WebSocket subscription callback that directly writes the content of the published message to the WebSocket connection.

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT`

```c
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_msg_s *msg);
```

Optional WebSocket subscription callback that directly writes the content of the published message to the WebSocket connection - this callback assumes that all messages are UTF-8 valid.

#### `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY`

```c
void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY(fio_msg_s *msg);
```

Optional WebSocket subscription callback that directly writes the content of the published message to the WebSocket connection - this callback assumes that all messages are binary (non-UTF-8).

#### `FIO_HTTP_SSE_SUBSCRIBE_DIRECT`

```c
void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_msg_s *msg);
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

/** Iterated through the properties associated with a parsed header values. */
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

#### `FIO_HTTP_CACHE_STATIC_HEADERS`

```c
#ifndef FIO_HTTP_CACHE_STATIC_HEADERS
#define FIO_HTTP_CACHE_STATIC_HEADERS 1
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

If true, logs longest WebSocket ping-pong round-trips (using `FIO_LOG_INFO`).

-------------------------------------------------------------------------------
