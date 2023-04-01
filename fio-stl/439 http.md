## HTTP Server

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

### Listening for HTTP / WebSockets and EventSource connections


#### `fio_http_listen`

```c
int fio_http_listen(const char *url, fio_http_settings_s settings);

#define fio_http_listen(url, ...)                                              \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})
```

Listens to HTTP / WebSockets / SSE connections on `url`.

The MACRO shadowing the function enables the used of named arguments for the `fio_http_settings_s`.


```c
typedef struct fio_http_settings_s {
  /** Callback for HTTP requests (server) or responses (client). */
  void (*on_http)(fio_http_s *h);
  /** (optional) the callback to be performed when the HTTP service closes. */
  void (*on_finish)(struct fio_http_settings_s *settings);

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

  /** Default opaque user data for HTTP handles (fio_http_s). */
  void *udata;
  /** Optional SSL/TLS support. */
  fio_io_functions_s *tls_io_func;
  /** Optional SSL/TLS support. */
  fio_tls_s *tls;
  /** Optional HTTP task queue (for multi-threading HTTP responses) */
  fio_srv_async_s *queue;
  /**
   * A public folder for file transfers - allows to circumvent any application
   * layer logic and simply serve static files.
   *
   * Supports automatic `gz` pre-compressed alternatives.
   */
  fio_str_info_s public_folder;
  /**
   * The max-age value (in seconds) for possibly caching static files from the
   * public folder specified.
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
   * The maximum websocket message size/buffer (in bytes) for Websocket
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
   * Timeout for the WebSocket connections, a ping will be sent whenever the
   * timeout is reached. Defaults to FIO_HTTP_DEFAULT_TIMEOUT_LONG seconds.
   *
   * Connections are only closed when a ping cannot be sent (the network layer
   * fails). Pongs are ignored.
   */
  uint8_t ws_timeout;
  /**
   * Timeout for EventSource (SSE) connections, a ping will be sent whenever the
   * timeout is reached. Defaults to FIO_HTTP_DEFAULT_TIMEOUT_LONG seconds.
   *
   * Connections are only closed when a ping cannot be sent (the network layer
   * fails).
   */
  uint8_t sse_timeout;
  /** Logging flag - set to TRUE to log HTTP requests. */
  uint8_t log;
} fio_http_settings_s;
```

#### `fio_http_io`

```c
fio_s *fio_http_io(fio_http_s *);
```

Returns the IO object associated with the HTTP object (request only).

#### `fio_http_subscribe`

```c
int fio_http_subscribe(fio_http_s *h, fio_subscribe_args_s args);

#define fio_http_subscribe(h, ...) fio_http_subscribe((h), ((fio_subscribe_args_s){__VA_ARGS__}))
```

Subscribes the HTTP handle (WebSocket / SSE) to events. Requires an Upgraded connection.

Automatically sets the correct `io` object and the default callback if none was provided (either `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT` or `FIO_HTTP_SSE_SUBSCRIBE_DIRECT`).

Using a `NULL` HTTP handle (`fio_http_s *`) is the same as calling `fio_subscribe` without an `io` object.

Returns `-1` on error (i.e., upgrade still being negotiated).

#### `FIO_HTTP_AUTHENTICATE_ALLOW`

```c
int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h);
```

Allows all clients to connect to WebSockets / EventSource (SSE) connections (bypasses authentication), to be used with the `.on_authenticate_sse` and `.on_authenticate_websocket` settings options.


### WebSocket Helpers - HTTP Upgraded Connections

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

#### `FIO_HTTP_SSE_SUBSCRIBE_DIRECT`

```c
void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_msg_s *msg);
```

An optional EventSource subscription callback - messages MUST be UTF-8.

This callback directly calls `fio_http_sse_write`, placing the channel name in the `.event` argument and the published message in the `.data` argument.
