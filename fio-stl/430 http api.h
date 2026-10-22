/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                    HTTP API - Public Declarations




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP) && !defined(FIO___RECURSIVE_INCLUDE) &&                  \
    !defined(H___FIO_HTTP_API___H)
#define H___FIO_HTTP_API___H
/* *****************************************************************************
HTTP Core API
***************************************************************************** */

/* *****************************************************************************
HTTP Setting Defaults
***************************************************************************** */

#ifndef FIO_HTTP_DEFAULT_MAX_HEADER_SIZE
/** Default limit for the request / status line plus all headers, in bytes. */
#define FIO_HTTP_DEFAULT_MAX_HEADER_SIZE 32768 /* (1UL << 15) */
#endif
#ifndef FIO_HTTP_DEFAULT_MAX_LINE_LEN
/** Default limit for a single header or request line, in bytes. */
#define FIO_HTTP_DEFAULT_MAX_LINE_LEN 8192 /* (1UL << 13) */
#endif
#ifndef FIO_HTTP_DEFAULT_MAX_BODY_SIZE
/** Default limit for the HTTP body (payload), in bytes. */
#define FIO_HTTP_DEFAULT_MAX_BODY_SIZE 33554432 /* (1UL << 25) */
#endif
#ifndef FIO_HTTP_PARAMS_MAX_FIELDS
/** Default limit for URL-encoded pairs and multipart parts per parse call. */
#define FIO_HTTP_PARAMS_MAX_FIELDS 4096
#endif
#ifndef FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE
/** Default limit for a WebSocket message, in bytes. */
#define FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE 262144 /* (1UL << 18) */
#endif
#ifndef FIO_HTTP_DEFAULT_TIMEOUT
/** Default HTTP/1.x connection timeout, in seconds. */
#define FIO_HTTP_DEFAULT_TIMEOUT 50
#endif
#ifndef FIO_HTTP_DEFAULT_TIMEOUT_LONG
/** Default WebSocket / SSE connection timeout, in seconds. */
#define FIO_HTTP_DEFAULT_TIMEOUT_LONG 50
#endif

#ifndef FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER
/**
 * If true, keeps the received `content-length` header in the HTTP handle.
 *
 * Off by default: the value is redundant with `fio_http_body_length`.
 */
#define FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER 0
#endif

#ifndef FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT
/**
 * Generic WebSocket writes test data shorter than this for UTF-8 validity.
 *
 * Applies to `fio_http_write` on a WebSocket and to
 * `FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT`. Valid UTF-8 is sent as text; longer
 * data is sent as binary without testing.
 */
#define FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT ((1UL << 16) - 10UL)
#endif

#ifndef FIO_WEBSOCKET_STATS
/** If true, logs the longest WebSocket round-trips (using FIO_LOG_INFO). */
#define FIO_WEBSOCKET_STATS 0
#endif

#ifndef FIO_HTTP_WEBSOCKET_DEFLATE_MIN
/**
 * WebSocket messages smaller than this are never compressed.
 *
 * Such messages fit in a single TCP/IP packet, so compression saves nothing.
 */
#define FIO_HTTP_WEBSOCKET_DEFLATE_MIN 1024
#endif

/* *****************************************************************************
HTTP Handle Settings
***************************************************************************** */
#ifndef FIO_HTTP_EXACT_LOGGING
/**
 * If true, request timing uses exact time stamps (in microseconds).
 *
 * By default (0), timing uses the IO reactor's cached clock (in milliseconds).
 * This is faster, but the measured time may include unrelated delays or miss
 * time that passed between clock updates.
 *
 * Exact timing measures only the time spent processing the request, excluding
 * network reads and writes. It also changes the unit of
 * `fio_http_received_at` to microseconds.
 *
 * Defaults to 1 when the IO module is not included.
 */
#define FIO_HTTP_EXACT_LOGGING 0
#ifndef H___FIO_IO___H
#undef FIO_HTTP_EXACT_LOGGING
#define FIO_HTTP_EXACT_LOGGING 1
#endif
#endif

#ifndef FIO_HTTP_BODY_RAM_LIMIT
/**
 * Body size, in bytes, at which the HTTP handle moves the body to a temporary
 * file.
 *
 * Smaller bodies stay in memory.
 */
#define FIO_HTTP_BODY_RAM_LIMIT (1 << 17)
#endif

#ifndef FIO_HTTP_CACHE_LIMIT
/** Maximum strings per HTTP string cache; 0 (the default) disables caching. */
#define FIO_HTTP_CACHE_LIMIT 0 /* ((1UL << 6) + (1UL << 5)) */
#endif

#ifndef FIO_HTTP_CACHE_STR_MAX_LEN
/** Strings longer than this (in bytes) are never cached. */
#define FIO_HTTP_CACHE_STR_MAX_LEN (1 << 12)
#endif

#ifndef FIO_HTTP_CACHE_USES_MUTEX
/** If true, a mutex protects the HTTP string caches across threads. */
#define FIO_HTTP_CACHE_USES_MUTEX 1
#endif

#ifndef FIO_HTTP_PRE_CACHE_KNOWN_HEADERS
/** Adds a static cache for common HTTP header names. */
#define FIO_HTTP_PRE_CACHE_KNOWN_HEADERS 1
#endif

#ifndef FIO_HTTP_DEFAULT_INDEX_FILENAME
/** File name served when a static file request points to a folder. */
#define FIO_HTTP_DEFAULT_INDEX_FILENAME "index"
#endif

#ifndef FIO_HTTP_STATIC_FILE_COMPLETION
/**
 * If true, static file lookups try `.html`, `.htm`, `.txt` and `.md` when the
 * requested file is missing.
 */
#define FIO_HTTP_STATIC_FILE_COMPLETION 1
#endif

#ifndef FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT
/** Maximum file size (in bytes) for on-disk static file compression. */
#define FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT (1UL << 21) /* 2 MiB */
#endif

#ifndef FIO_HTTP_LOG_X_REQUEST_START
/** If true, logs the request wait time from the `x-request-start` header. */
#define FIO_HTTP_LOG_X_REQUEST_START 1
#endif

#ifndef FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
/**
 * If true, header names are converted to lower case when set or looked up.
 *
 * Otherwise header names are case-sensitive: use lower-case names.
 */
#define FIO_HTTP_ENFORCE_LOWERCASE_HEADERS 0
#endif

/* *****************************************************************************
HTTP Handle Type
***************************************************************************** */

/**
 * The HTTP handle: a reference-counted request / response object.
 *
 * The handle is NOT thread-safe.
 */
typedef struct fio_http_s fio_http_s;

/**
 * The HTTP controller: the protocol callbacks that send an HTTP handle's data.
 *
 * Controllers keep the HTTP handle protocol-agnostic (see the struct
 * definition below).
 *
 * Note: if the controller callbacks are not thread-safe, call `fio_http_write`
 * only from the thread the controller expects.
 */
typedef struct fio_http_controller_s fio_http_controller_s;

/* *****************************************************************************
HTTP Settings Type
***************************************************************************** */

/**
 * HTTP listener, route and client connection settings.
 *
 * Zero / NULL members select their defaults. Settings are copied by value.
 */
typedef struct fio_http_settings_s {
  /**
   * Called before the body is uploaded, when a client sends an `Expect`
   * header.
   *
   * To reject the upload, send a response (e.g., with
   * `fio_http_send_error_response`). Otherwise `100 Continue` is sent.
   */
  void (*pre_http_body)(fio_http_s *h);
  /**
   * Called for each HTTP request (server) or response (client).
   *
   * Server default: responds with a 404 error.
   */
  void (*on_http)(fio_http_s *h);
  /** Called when a request / response cycle is finished (for WebSocket /
   * SSE connections, called after `on_close`, when the connection closes). */
  void (*on_finish)(fio_http_s *h);

  /**
   * Authenticates EventSource (SSE) requests. Return 0 to allow or non-zero
   * to deny (403).
   *
   * Default: denies on servers and allows on clients. Use
   * `FIO_HTTP_AUTHENTICATE_ALLOW` to allow all.
   */
  int (*on_authenticate_sse)(fio_http_s *h);
  /**
   * Authenticates WebSocket upgrade requests. Return 0 to allow or non-zero
   * to deny (403).
   *
   * Default: denies on servers and allows on clients. Use
   * `FIO_HTTP_AUTHENTICATE_ALLOW` to allow all.
   */
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

  /** Called for WebSocket / SSE connections when the outgoing buffer is
   * empty. */
  void (*on_ready)(fio_http_s *h);
  /** Called for open WebSocket / SSE connections during server shutdown. */
  void (*on_shutdown)(fio_http_s *h);
  /** Called after a WebSocket / SSE connection is closed (for cleanup). */
  void (*on_close)(fio_http_s *h);

  /**
   * Optional: called once for these settings when they are released: when
   * the listener stops, or when a route is replaced.
   *
   * Routes do not inherit the listener's `on_stop`; set it for each route that
   * needs cleanup.
   */
  void (*on_stop)(struct fio_http_settings_s *settings);

  /** Default opaque user data for HTTP handles (see `fio_http_udata`). */
  void *udata;

  /** Optional SSL/TLS IO functions. */
  fio_io_functions_s *tls_io_func;
  /** Optional SSL/TLS settings (enables TLS). */
  fio_io_tls_s *tls;
  /** Optional task queue for HTTP callbacks (for multi-threaded responses). */
  fio_io_async_s *queue;
  /**
   * A public folder for static files, served without calling `on_http`.
   *
   * Only `GET` and `HEAD` requests are served (RFC 9110 §9.3: a static file
   * is not a valid response to other methods). Other methods, and requests
   * for missing files, go to `on_http`, which may call
   * `fio_http_static_file_response` itself. Folders resolve to their `index`
   * file and missing extensions are tried (`.html`, `.htm`, `.txt`, `.md`;
   * see `FIO_HTTP_STATIC_FILE_COMPLETION`).
   *
   * Pre-compressed variants are served when the client's `Accept-Encoding`
   * allows it and the variant is up to date. Preference order: `file.br`,
   * `file.zstd`, `file.gz`, `file.zip` (`br`, `zstd`, `gzip`, `deflate`).
   * With `compress_static`, missing `.br` / `.gz` variants are also created
   * on demand (written into this folder). Range requests are always served
   * from the original file.
   *
   * The folder must exist when the listener (or route) is created; otherwise
   * the setting is ignored and an error is logged.
   */
  fio_str_info_s public_folder;
  /**
   * The `max-age` value (in seconds) for static files served from
   * `public_folder`.
   *
   * Defaults to 0 (no `Cache-Control` header is sent).
   *
   * Note: NOT inherited by routes. A route that serves static files must set
   * its own `max_age`.
   */
  size_t max_age;
  /**
   * The maximum size, in bytes, of the request line and all headers combined.
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_HEADER_SIZE. Raised to `max_line_len` if
   * smaller.
   */
  uint32_t max_header_size;
  /**
   * The maximum size, in bytes, of a single header or request line.
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_LINE_LEN.
   */
  uint32_t max_line_len;
  /**
   * The maximum size, in bytes, of an HTTP body (upload or download).
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_BODY_SIZE.
   */
  size_t max_body_size;
  /**
   * The maximum number of query and form fields (URL-encoded pairs and
   * multipart parts) per params parsing call.
   *
   * Defaults to FIO_HTTP_PARAMS_MAX_FIELDS.
   */
  size_t max_params;
  /**
   * The maximum size, in bytes, of a WebSocket message.
   *
   * Defaults to FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE.
   */
  size_t ws_max_msg_size;
  /** Reserved for future use. */
  intptr_t reserved1;
  /** Reserved for future use. */
  intptr_t reserved2;
  /**
   * Client mode only: the connection timeout, passed to `fio_io_connect` (in
   * milliseconds). 0 selects the `fio_io_connect` default.
   */
  uint16_t connect_timeout;
  /**
   * The HTTP/1.x connection timeout, in seconds.
   *
   * Defaults to FIO_HTTP_DEFAULT_TIMEOUT.
   *
   * Note: the other side may close the connection before the timeout.
   */
  uint8_t timeout;
  /**
   * The WebSocket connection timeout, in seconds.
   *
   * Defaults to FIO_HTTP_DEFAULT_TIMEOUT_LONG.
   *
   * A ping is sent whenever the timeout is reached. Connections close only
   * when a ping cannot be sent (the network layer fails). Pongs are ignored.
   */
  uint8_t ws_timeout;
  /**
   * The EventSource (SSE) connection timeout, in seconds.
   *
   * Defaults to `ws_timeout`.
   *
   * A ping is sent whenever the timeout is reached. Connections close only
   * when a ping cannot be sent (the network layer fails).
   */
  uint8_t sse_timeout;
  /** If true, logs each HTTP request (see `fio_http_write_log`). */
  uint8_t log;
  /**
   * Opt-in: creates missing `.br` / `.gz` variants of static files on demand,
   * writing them into `public_folder`.
   *
   * Only compressible (text-like) MIME types are compressed, for files from
   * 1024 bytes up to FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT (2 MiB). A variant
   * is skipped when compression does not shrink the file. Variants older than
   * the original file are re-created.
   *
   * Note: any client can trigger these writes. The public folder MUST be
   * writable and size-limited (quota). Pre-generating variants at deploy time
   * is recommended.
   *
   * Note: read per route (routes inherit the listener's value when created).
   * Handles created with `fio_http_new` (no settings) never create variants.
   *
   * Note: creation disables itself (the value drops to 0) after 8
   * consecutive failures or one filesystem-full / permission error. It stays
   * disabled until the settings are set again.
   */
  uint8_t compress_static;
  /**
   * Opt-in: compresses dynamic HTTP responses on the fly.
   *
   * Applies to finished (non-streaming) single-buffer responses larger than
   * 1024 bytes with a compressible (text-like) `Content-Type`. Prefers `br`
   * over `gzip`, per the client's `Accept-Encoding`. Responses with a
   * `Content-Encoding` header are never compressed again. `Vary:
   * accept-encoding` is set whenever compression was considered.
   *
   * Note: read once per connection from the listener's root settings.
   * Per-route values have no effect.
   */
  uint8_t compress_dynamic;
  /**
   * Opt-in: enables permessage-deflate (RFC 7692) for WebSocket connections.
   *
   * Negotiation always requires both `*_no_context_takeover` flags, so each
   * message is compressed on its own and connections keep almost no
   * compression state.
   *
   * Note: read once per connection from the listener's root settings.
   * Per-route values have no effect.
   */
  uint8_t compress_ws;
} fio_http_settings_s;

/* *****************************************************************************
HTTP Connection Helpers
***************************************************************************** */

/** Authentication callback that always allows the upgrade (returns 0). */
SFUNC int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h);

/**
 * Returns the IO object of the HTTP handle's connection.
 *
 * Returns NULL if `h` is NULL or has no connection (e.g., a handle created
 * with `fio_http_new`).
 */
SFUNC fio_io_s *fio_http_io(fio_http_s *);

/** Subscribes the HTTP handle's connection (`fio_http_io(h)`) to pub/sub. */
#define fio_http_subscribe(h, ...)                                             \
  fio_pubsub_subscribe(.io = fio_http_io(h), __VA_ARGS__)
/* *****************************************************************************
HTTP Server API
***************************************************************************** */

/* *****************************************************************************
HTTP Listen
***************************************************************************** */

/** An opaque HTTP listener handle. */
typedef struct fio_http_listener_s fio_http_listener_s;

/**
 * Listens for HTTP / WebSocket / SSE connections on `url`.
 *
 * The listener always serves the root path; a path in `url` is ignored (with a
 * warning). Use `fio_http_route` to add routes.
 *
 * Returns the listener, or NULL on error. The listener is valid until it
 * stops.
 */
SFUNC fio_http_listener_s *fio_http_listen(const char *url,
                                           fio_http_settings_s settings);

/** Listens for HTTP / WebSocket / SSE connections on `url` (named args). */
#define fio_http_listen(url, ...)                                              \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})

/** Returns a pointer to the listener's (root) HTTP settings. */
SFUNC fio_http_settings_s *fio_http_listener_settings(fio_http_listener_s *l);

/* *****************************************************************************
HTTP Routing – prefix matching
***************************************************************************** */

/**
 * Adds a route (path prefix) to an HTTP listener.
 *
 * Routes use best-prefix matching, so the order of calls does not matter
 * (unless a call replaces an existing route). For example:
 *
 * - All paths match the route `"/"` (the default route).
 *
 * - The route `"/user"` matches `"/user"` and all `"/user/..."` paths, but not
 *   `"/user..."`.
 *
 * - With both `"/user/new"` and `"/user"` set, `"/user/new"` and
 *   `"/user/new/..."` go to `"/user/new"`. All other `"/user"` paths still go
 *   to `"/user"`.
 *
 * In a routed request, `fio_http_path` returns the path after the route
 * prefix and `fio_http_opath` returns the original path.
 *
 * Missing values are inherited from the listener's settings: `udata`,
 * `on_finish`, `on_stop`, `on_authenticate_sse`, `on_authenticate_websocket`,
 * `max_header_size`, `max_line_len`, `max_body_size`, `max_params`,
 * `ws_max_msg_size`, `timeout`, `ws_timeout`, `sse_timeout`, `log`,
 * `compress_static` and `public_folder`.
 *
 * Only `on_http`, `on_finish`, `udata`, the authentication callbacks,
 * `public_folder`, `max_age` and `compress_static` take effect per route.
 * All other values are stored but unused: upgraded connection callbacks
 * (`on_open`, `on_message`, etc.), `queue`, `log`, limits, timeouts,
 * `compress_dynamic` and `compress_ws` are read from the listener's root
 * settings. TLS options are ignored.
 *
 * Returns 0 on success, or -1 if `listener` is NULL or `url` contains
 * invalid (control) characters.
 */
SFUNC int fio_http_route(fio_http_listener_s *listener,
                         const char *url,
                         fio_http_settings_s settings);
/**
 * Adds a route (path prefix) to an HTTP listener.
 *
 * Routes use best-prefix matching, so the order of calls does not matter
 * (unless a call replaces an existing route). For example:
 *
 * - All paths match the route `"/"` (the default route).
 *
 * - The route `"/user"` matches `"/user"` and all `"/user/..."` paths, but not
 *   `"/user..."`.
 *
 * - With both `"/user/new"` and `"/user"` set, `"/user/new"` and
 *   `"/user/new/..."` go to `"/user/new"`. All other `"/user"` paths still go
 *   to `"/user"`.
 *
 * In a routed request, `fio_http_path` returns the path after the route
 * prefix and `fio_http_opath` returns the original path.
 *
 * Missing values are inherited from the listener's settings: `udata`,
 * `on_finish`, `on_stop`, `on_authenticate_sse`, `on_authenticate_websocket`,
 * `max_header_size`, `max_line_len`, `max_body_size`, `max_params`,
 * `ws_max_msg_size`, `timeout`, `ws_timeout`, `sse_timeout`, `log`,
 * `compress_static` and `public_folder`.
 *
 * Only `on_http`, `on_finish`, `udata`, the authentication callbacks,
 * `public_folder`, `max_age` and `compress_static` take effect per route.
 * All other values are stored but unused: upgraded connection callbacks
 * (`on_open`, `on_message`, etc.), `queue`, `log`, limits, timeouts,
 * `compress_dynamic` and `compress_ws` are read from the listener's root
 * settings. TLS options are ignored.
 *
 * Returns 0 on success, or -1 if `listener` is NULL or `url` contains
 * invalid (control) characters.
 */
#define fio_http_route(listener, url, ...)                                     \
  fio_http_route(listener, url, (fio_http_settings_s){__VA_ARGS__})

/**
 * Returns a pointer to the settings of the route that best matches `url`.
 *
 * Returns NULL if `l` is NULL.
 */
SFUNC fio_http_settings_s *fio_http_route_settings(fio_http_listener_s *l,
                                                   const char *url);

/* *****************************************************************************
HTTP Routing – CRUD
***************************************************************************** */

/**
 * CRUD (resource) actions, as detected by `fio_http_resource_action`.
 *
 * Paths are relative to the route (see `fio_http_path`).
 */
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

/**
 * Returns the CRUD action that matches the request's method and path.
 *
 * Methods and the `new` / `edit` path segments match case-insensitively.
 *
 * Returns `FIO_HTTP_RESOURCE_NONE` if no action matches or `h` is NULL.
 */
FIO_IFUNC fio_http_resource_action_e fio_http_resource_action(fio_http_s *h);

/* *****************************************************************************
HTTP Client API
***************************************************************************** */

/* *****************************************************************************
HTTP Connect
***************************************************************************** */

/**
 * Connects to an HTTP / WebSocket / SSE server at `url` (client mode).
 *
 * `h` is the request to send. If `h` is NULL, a new `GET` request handle is
 * created. Missing request data is filled from `url`: path (default `/`),
 * query, method (`GET`) and the `host` header. The `ws://` / `wss://` schemes
 * request a WebSocket upgrade and `sse://` / `sses://` request an SSE
 * connection.
 *
 * The connection takes ownership of `h`: do not use or free `h` after this
 * call, even when NULL is returned. On failure, `h` is freed later on the IO
 * thread. The response is passed to `settings.on_http`.
 *
 * Returns the IO object, or NULL on error. As with `fio_io_connect`, the
 * returned IO object is owned by the reactor.
 */
SFUNC fio_io_s *fio_http_connect(const char *url,
                                 fio_http_s *h,
                                 fio_http_settings_s settings);

/** Connects to an HTTP / WebSocket / SSE server at `url` (named args). */
#define fio_http_connect(url, h, ...)                                          \
  fio_http_connect(url, h, (fio_http_settings_s){__VA_ARGS__})

/**
 * Connects to a WebSocket server on `url`.
 *
 * A wrapper around `fio_http_connect` that uses a WebSocket scheme: `http://`
 * becomes `ws://`, `https://` becomes `wss://` and a missing scheme becomes
 * `ws://`. Other schemes are used unchanged.
 *
 * If the server accepts the upgrade (101), the connection uses the WebSocket
 * callbacks (`on_open`, `on_message`, `on_close`). Otherwise the response is
 * passed to `settings.on_http`.
 *
 * Returns the same as `fio_http_connect`.
 */
SFUNC fio_io_s *fio_http_websocket_connect(const char *url,
                                           fio_http_s *h,
                                           fio_http_settings_s settings);

/**
 * Connects to a WebSocket server on `url` (see
 * `fio_http_websocket_connect`).
 */
#define fio_http_websocket_connect(url, h, ...)                                \
  fio_http_websocket_connect(url, h, (fio_http_settings_s){__VA_ARGS__})

/* *****************************************************************************
HTTP Handle API
***************************************************************************** */

/* *****************************************************************************
Constructor / Destructor
***************************************************************************** */

/**
 * Creates a new HTTP handle with a reference count of 1.
 *
 * The handle has no connection. Free it with `fio_http_free`.
 */
SFUNC fio_http_s *fio_http_new(void);

/**
 * Creates a new HTTP handle with a copy of `old`'s request data.
 *
 * Copies the method, path, original path, query, version, request headers and
 * received cookies. The body and response data are not copied. Free the new
 * handle with `fio_http_free`.
 */
SFUNC fio_http_s *fio_http_new_copy_request(fio_http_s *old);

/** Decreases the handle's reference count, freeing it when it reaches zero. */
SFUNC void fio_http_free(fio_http_s *);

/** Increases the handle's reference count. Returns the handle. */
SFUNC fio_http_s *fio_http_dup(fio_http_s *);

/**
 * Releases all of the handle's data and resets it to a blank state.
 *
 * Notifies the controller (`on_destroyed`) and detaches it. Does not free the
 * handle itself or change its reference count.
 *
 * Returns `h` (NULL if `h` is NULL).
 */
SFUNC fio_http_s *fio_http_destroy(fio_http_s *h);

/** Resets the handle's start time (`fio_http_received_at`) to now. */
SFUNC void fio_http_start_time_set(fio_http_s *);

/**
 * Clears the handle's response data.
 *
 * Removes the response headers, resets the status and response state, and
 * resets the start time. Response cookies set with `fio_http_cookie_set` are
 * kept. The body is released only if `clear_body` is true.
 *
 * Returns `h`.
 */
SFUNC fio_http_s *fio_http_clear_response(fio_http_s *h, bool clear_body);

/* *****************************************************************************
Opaque User and Controller Data
***************************************************************************** */

/**
 * Gets the handle's opaque user pointer.
 *
 * Handles from a listener start with the matching route's `udata` setting.
 */
FIO_IFUNC void *fio_http_udata(fio_http_s *);

/** Sets the handle's opaque user pointer. Returns the new value. */
FIO_IFUNC void *fio_http_udata_set(fio_http_s *, void *);

/** Gets the handle's second opaque user pointer. */
FIO_IFUNC void *fio_http_udata2(fio_http_s *);

/** Sets the handle's second opaque user pointer. Returns the new value. */
FIO_IFUNC void *fio_http_udata2_set(fio_http_s *, void *);

/** Gets the handle's HTTP controller. */
FIO_IFUNC fio_http_controller_s *fio_http_controller(fio_http_s *h);

/**
 * Sets the handle's HTTP controller. Returns the controller in use.
 *
 * A NULL `controller` selects a built-in no-op controller. NULL callbacks in
 * `controller` are replaced with no-op defaults, so the controller must be
 * writable and outlive the handle.
 */
FIO_IFUNC fio_http_controller_s *fio_http_controller_set(
    fio_http_s *h,
    fio_http_controller_s *controller);

/** Gets the controller's opaque data pointer (`cdata`). */
FIO_IFUNC void *fio_http_cdata(fio_http_s *h);

/**
 * Sets the controller's opaque data pointer (`cdata`). Returns the new value.
 *
 * The HTTP connection layer uses `cdata`; do not change it on handles that
 * belong to a connection.
 */
FIO_IFUNC void *fio_http_cdata_set(fio_http_s *h, void *cdata);

/* *****************************************************************************
Data associated with the Request (usually set by the HTTP protocol)
***************************************************************************** */

/** Gets the response status (0 if not set, e.g., a client request). */
SFUNC size_t fio_http_status(fio_http_s *);

/**
 * Sets the response status. Returns the status that was set.
 *
 * 0 becomes 200 and values above 1023 become 500.
 */
SFUNC size_t fio_http_status_set(fio_http_s *, size_t status);

/*
 * Getters and setters for the request line data: method, original path
 * (`opath`), path, query and version.
 *
 * Getters return a buffer owned by the handle. It stays valid until the value
 * is set again or the handle is freed. Setters copy `value` and return the
 * stored copy. Both return an empty value if `h` is NULL.
 */

/** Gets the request method. */
SFUNC fio_str_info_s fio_http_method(fio_http_s *);

/** Tests for GET, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_get(fio_http_s *h);
/** Tests for HEAD, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_head(fio_http_s *h);
/** Tests for POST, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_post(fio_http_s *h);
/** Tests for PUT, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_put(fio_http_s *h);
/** Tests for DELETE, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_delete(fio_http_s *h);
/** Tests for CONNECT, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_connect(fio_http_s *h);
/** Tests for OPTIONS, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_options(fio_http_s *h);
/** Tests for TRACE, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_trace(fio_http_s *h);
/** Tests for PATCH, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_patch(fio_http_s *h);
/** Tests for QUERY, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_query(fio_http_s *h);
/** Tests for PROPFIND, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_propfind(fio_http_s *h);
/** Tests for PROPPATCH, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_proppatch(fio_http_s *h);
/** Tests for MKCOL, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_mkcol(fio_http_s *h);
/** Tests for COPY, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_copy(fio_http_s *h);
/** Tests for MOVE, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_move(fio_http_s *h);
/** Tests for LOCK, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_lock(fio_http_s *h);
/** Tests for UNLOCK, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_unlock(fio_http_s *h);
/** Tests for SEARCH, case-insensitively; requires a non-NULL HTTP handle. */
SFUNC int fio_http_method_is_search(fio_http_s *h);

/** Sets the request method. */
SFUNC fio_str_info_s fio_http_method_set(fio_http_s *, fio_str_info_s);

/** Gets the original request path, before routing. */
SFUNC fio_str_info_s fio_http_opath(fio_http_s *);

/** Sets the original request path. */
SFUNC fio_str_info_s fio_http_opath_set(fio_http_s *, fio_str_info_s);

/** Gets the request path, relative to the matching route's prefix. */
SFUNC fio_str_info_s fio_http_path(fio_http_s *);

/** Sets the request path. */
SFUNC fio_str_info_s fio_http_path_set(fio_http_s *, fio_str_info_s);

/** Gets the query string (without the `?`). */
SFUNC fio_str_info_s fio_http_query(fio_http_s *);

/** Sets the query string. */
SFUNC fio_str_info_s fio_http_query_set(fio_http_s *, fio_str_info_s);

/** Gets the HTTP version string (e.g., `HTTP/1.1`). */
SFUNC fio_str_info_s fio_http_version(fio_http_s *);

/** Sets the HTTP version string. */
SFUNC fio_str_info_s fio_http_version_set(fio_http_s *, fio_str_info_s);

/**
 * Gets the time the request was received (used for logging).
 *
 * In milliseconds, or microseconds when `FIO_HTTP_EXACT_LOGGING` is true.
 */
SFUNC int64_t fio_http_received_at(fio_http_s *);

/** Sets the time the request was received (same unit as the getter). */
SFUNC int64_t fio_http_received_at_set(fio_http_s *, int64_t);

/**
 * Gets the value at `index` of the request header `name`.
 *
 * A header may have more than one value; use `index` to read each one.
 * Received header names are lower case (see
 * `FIO_HTTP_ENFORCE_LOWERCASE_HEADERS`).
 *
 * Returns a buffer owned by the handle, valid until the header changes or the
 * handle is freed. Returns an empty value if the header or index does not
 * exist.
 */
SFUNC fio_str_info_s fio_http_request_header(fio_http_s *,
                                             fio_str_info_s name,
                                             size_t index);

/**
 * Returns the number of values of the request header `name`.
 *
 * If `name.buf` is NULL, returns the number of distinct header names.
 */
SFUNC size_t fio_http_request_header_count(fio_http_s *, fio_str_info_s name);

/**
 * Request header setters. Each copies `value` and returns the stored value.
 *
 * - `fio_http_request_header_set` replaces all values of `name`.
 * - `fio_http_request_header_set_if_missing` sets `value` only if `name` has
 *   no value. Otherwise it returns the existing (last) value.
 * - `fio_http_request_header_add` adds `value` to any existing values.
 *
 * An empty `value` removes the header for `set`. `set_if_missing` ignores it
 * and returns the existing value (if any); `add` ignores it. Returns an empty value on error: an empty or invalid `name`
 * (longer than 4095 bytes or with forbidden characters) or a `value` that
 * contains NUL, CR or LF.
 */
SFUNC fio_str_info_s fio_http_request_header_set(fio_http_s *,
                                                 fio_str_info_s name,
                                                 fio_str_info_s value);

/** Sets the request header `name` if it has no value (see above). */
SFUNC fio_str_info_s
fio_http_request_header_set_if_missing(fio_http_s *,
                                       fio_str_info_s name,
                                       fio_str_info_s value);

/** Adds a value to the request header `name` (see above). */
SFUNC fio_str_info_s fio_http_request_header_add(fio_http_s *,
                                                 fio_str_info_s name,
                                                 fio_str_info_s value);

/**
 * Calls `callback` for each request header value.
 *
 * A received `cookie` header is included. Cookies held in the handle's cookie
 * store are not (see `fio_http_cookie_each`). Return -1 from `callback` to
 * stop; other values are ignored.
 *
 * Returns the number of header names processed. If `callback` is NULL,
 * returns the number of distinct header names.
 * */
SFUNC size_t fio_http_request_header_each(fio_http_s *,
                                          int (*callback)(fio_http_s *,
                                                          fio_str_info_s name,
                                                          fio_str_info_s value,
                                                          void *udata),
                                          void *udata);

/** Returns the body (payload) length, in bytes. */
SFUNC size_t fio_http_body_length(fio_http_s *);

/**
 * Adjusts the body's reading position and returns the new position.
 *
 * Negative values count back from the end: `-n` is `length - n`, so `-1` is
 * the last byte (as with `lseek`); values before the start clamp to 0. Any
 * position at or past the end is EOF (i.e., `fio_http_body_length(h)` or
 * `SSIZE_MAX`).
 */
SFUNC size_t fio_http_body_seek(fio_http_s *, ssize_t pos);

/** Returns the body's current reading position. */
SFUNC size_t fio_http_body_pos(fio_http_s *h);

/**
 * Reads up to `length` bytes from the body and advances the reading position.
 *
 * Returns an empty value at EOF.
 *
 * The returned buffer is borrowed: any later `fio_http_body_*` call on the
 * same handle MAY invalidate it.
 */
SFUNC fio_str_info_s fio_http_body_read(fio_http_s *, size_t length);

/**
 * Reads from the body until `token`, `limit` bytes or EOF, whichever comes
 * first.
 *
 * Note: `limit` is ignored if it is zero or larger than the remaining data.
 *
 * The returned buffer is borrowed: any later `fio_http_body_*` call on the
 * same handle MAY invalidate it.
 */
SFUNC fio_str_info_s fio_http_body_read_until(fio_http_s *,
                                              char token,
                                              size_t limit);

/** Reserves body (payload) storage for at least `expected_length` bytes. */
SFUNC void fio_http_body_expect(fio_http_s *, size_t expected_length);

/**
 * Appends `data` to the body (payload).
 *
 * Large bodies move to a temporary file (see `FIO_HTTP_BODY_RAM_LIMIT`).
 *
 * Returns 0 on success or -1 on error (e.g., the body could not be moved to a
 * temporary file). After an error, later writes fail until the body is closed
 * (see `fio_data_write`).
 */
SFUNC int fio_http_body_write(fio_http_s *, const void *data, size_t len);

/**
 * Returns the file descriptor of the body's temporary file.
 *
 * Returns -1 if the body is stored in memory. The handle owns the file; do
 * not close it.
 */
SFUNC int fio_http_body_fd(fio_http_s *);

/** Releases body (payload) resources, closing any temporary files. */
SFUNC void fio_http_body_close(fio_http_s *);

/**
 * Saves the whole body to `filename`, atomically replacing any existing file.
 *
 * The body and its reading position are unchanged (see `fio_data_save`).
 *
 * Returns 0 on success or -1 on error (e.g., after a body write error).
 */
SFUNC int fio_http_body_save(fio_http_s *, const char *filename);

/* *****************************************************************************
Path Section Looping
***************************************************************************** */

/**
 * Loops over each `/`-separated section of `path`, decoding percent encoding.
 *
 * - `path`: the path string (a `fio_str_info_s`, e.g., `fio_http_path(h)`).
 * - `pos`: the name of the loop variable, a `fio_buf_info_s` holding the
 *   decoded, NUL-terminated section. It is valid only for one iteration.
 *
 * The loop also declares `<pos>_reminder` (the rest of the path) and a 4096
 * byte stack buffer named `buf`, which hides any outer `buf` in the loop body.
 *
 * **Note**: the loop stops early if a section is longer than about 4063
 * bytes.
 */
#define FIO_HTTP_PATH_EACH(path, pos)

/* *****************************************************************************
Cookies
***************************************************************************** */

/**
 * Values for the cookie `same_site` property.
 *
 * See: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
 */
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

/**
 * Named arguments for `fio_http_cookie_set`. For example:
 *
 *       fio_http_cookie_set(h,
 *                           .name = FIO_STR_INFO1("my_cookie"),
 *                           .value = FIO_STR_INFO1("data"));
 *
 */
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

/**
 * Sets a response cookie (a `set-cookie` header).
 *
 * Invalid characters in the name or value are percent-encoded (with a one-time
 * warning). The new value is also visible to `fio_http_cookie`.
 *
 * Returns 0 on success, or -1 if `h` is NULL or the response headers were
 * already sent.
 *
 * Note: long cookies only log an error. Many browsers and proxies limit
 * cookies to about 4KB (name and value combined).
 */
SFUNC int fio_http_cookie_set(fio_http_s *h, fio_http_cookie_args_s);

/** Sets a response cookie (named args, see `fio_http_cookie_args_s`). */
#define fio_http_cookie_set(http___handle, ...)                                \
  fio_http_cookie_set((http___handle), (fio_http_cookie_args_s){__VA_ARGS__})

/**
 * Returns the value of the cookie `name` (received or newly set).
 *
 * Received cookies are percent-decoded. Returns an empty value if the cookie
 * does not exist. The buffer is owned by the handle.
 */
SFUNC fio_str_info_s fio_http_cookie(fio_http_s *,
                                     const char *name,
                                     size_t name_len);

/**
 * Calls `callback` for each cookie (received or newly set).
 *
 * A non-zero return from `callback` stops the loop. Returns the number of
 * cookies processed.
 */
SFUNC size_t fio_http_cookie_each(fio_http_s *,
                                  int (*callback)(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value,
                                                  void *udata),
                                  void *udata);

/**
 * Calls `callback` for each response cookie set with `fio_http_cookie_set`.
 *
 * `set_cookie_header` is always `"set-cookie"` and `value` is the full header
 * value. A non-zero return from `callback` stops the loop. Returns the number
 * of cookies processed.
 */
SFUNC size_t
fio_http_set_cookie_each(fio_http_s *h,
                         int (*callback)(fio_http_s *,
                                         fio_str_info_s set_cookie_header,
                                         fio_str_info_s value,
                                         void *udata),
                         void *udata);

/* *****************************************************************************
Responding to an HTTP event.
***************************************************************************** */

/** Returns true if no headers were sent and nothing was upgraded yet. */
SFUNC int fio_http_is_clean(fio_http_s *);

/** Returns true if the response was finished. */
SFUNC int fio_http_is_finished(fio_http_s *);

/** Returns true if the response is streaming (no `content-length`). */
SFUNC int fio_http_is_streaming(fio_http_s *);

/** Returns true if the HTTP connection was (or should have been) upgraded. */
SFUNC int fio_http_is_upgraded(fio_http_s *h);

/** Returns true if the HTTP handle refers to a WebSocket connection. */
SFUNC int fio_http_is_websocket(fio_http_s *);

/** Returns true if the HTTP handle refers to an EventSource connection. */
SFUNC int fio_http_is_sse(fio_http_s *);

/** Returns true if the handle is being freed (e.g., in `on_destroyed`). */
SFUNC int fio_http_is_freeing(fio_http_s *);

/**
 * Gets the value at `index` of the response header `name`.
 *
 * Works like `fio_http_request_header`.
 */
SFUNC fio_str_info_s fio_http_response_header(fio_http_s *,
                                              fio_str_info_s name,
                                              size_t index);
/**
 * Returns the number of values of the response header `name`.
 *
 * If `name.buf` is NULL, returns the number of distinct header names.
 */
SFUNC size_t fio_http_response_header_count(fio_http_s *, fio_str_info_s name);

/**
 * Replaces all values of the response header `name`.
 *
 * Works like `fio_http_request_header_set`. Changes made after the headers
 * were sent are not sent.
 */
SFUNC fio_str_info_s fio_http_response_header_set(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);
/**
 * Sets the response header `name` if it has no value.
 *
 * Works like `fio_http_request_header_set_if_missing`.
 */
SFUNC fio_str_info_s
fio_http_response_header_set_if_missing(fio_http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value);

/**
 * Adds a value to the response header `name`.
 *
 * Works like `fio_http_request_header_add`.
 */
SFUNC fio_str_info_s fio_http_response_header_add(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);

/**
 * Calls `callback` for each response header value.
 *
 * Cookies set with `fio_http_cookie_set` are not included (see
 * `fio_http_set_cookie_each`). Return -1 from `callback` to stop; other values
 * are ignored.
 *
 * Returns the number of header names processed. If `callback` is NULL,
 * returns the number of distinct header names.
 * */
SFUNC size_t fio_http_response_header_each(fio_http_s *,
                                           int (*callback)(fio_http_s *,
                                                           fio_str_info_s name,
                                                           fio_str_info_s value,
                                                           void *udata),
                                           void *udata);

/** Named arguments for `fio_http_write`. */
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

/**
 * Writes data to the response (or request) body.
 *
 * The first write sends the headers; headers set later are not sent. A
 * finishing first write sets `content-length`; other first writes start a
 * streamed response. A write with no data that does not finish is ignored,
 * so headers can still change.
 *
 * Also handles: a `304` response when the `etag` header matches
 * `if-none-match`, an error page for an empty finished response with status
 * 400 or above, and dropping the body of `HEAD` and no-content (1xx, 204,
 * 205, 304) responses. See `compress_dynamic` for compression.
 *
 * Ownership: `buf` (with `dealloc`) and `fd` (without `copy`) pass to this
 * function, even on error (e.g., after the response finished).
 */
SFUNC void fio_http_write(fio_http_s *, fio_http_write_args_s args);

/** Writes to the body (named args, see `fio_http_write_args_s`). */
#define fio_http_write(http_handle, ...)                                       \
  fio_http_write(http_handle, (fio_http_write_args_s){__VA_ARGS__})
/** Finishes the response (or request) with no more data. */
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)

/**
 * Closes the connection after previously scheduled output is sent.
 *
 * Schedule the complete response before calling this (e.g., in `on_finish`).
 */
SFUNC void fio_http_close(fio_http_s *h);

/* *****************************************************************************
MIME File Type Helpers - NOT thread safe!
***************************************************************************** */

/**
 * Registers the MIME type for a file extension (without the dot).
 *
 * An empty `mime_type` removes the extension. NOT thread-safe: register types
 * before the server starts.
 *
 * Returns 0 on success, or -1 if `file_ext_len` exceeds 7 bytes or
 * `mime_type` exceeds 117 bytes.
 */
SFUNC int fio_http_mimetype_register(char *file_ext,
                                     size_t file_ext_len,
                                     fio_str_info_s mime_type);

/**
 * Returns the MIME type registered for a file extension (without the dot).
 *
 * `file_ext_len` MUST NOT exceed 7 bytes. Returns an empty value if no type
 * is registered. The registry owns the returned buffer; do not free it.
 */
SFUNC fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len);

/* *****************************************************************************
HTTP Params Parsing Helpers (query / body)
***************************************************************************** */

/**
 * Callbacks that build the result of the HTTP params parsers (query and body).
 *
 * The first 14 members match `fio_json_parser_callbacks_s` exactly (same
 * order, types and contract, including key conflicts), so the table is passed
 * to the JSON parser as is. The contract:
 *
 * - Value callbacks return owned values. `map_push` and `array_push` take the
 *   value they receive, even when they fail; callers never free it afterwards.
 * - Containers placed in a parent are borrowed; a root (`parent == NULL`) is
 *   owned by the caller. Decoded keys are borrowed only during the callback.
 * - A replaced value is either freed by the callback or handed back (`old` /
 *   `map_push().obj`) for conflict resolution. A native NULL `obj` / `old` is
 *   "nothing" and never triggers conflict resolution.
 * - `err` is opaque: any non-zero callback value aborts parsing and reaches
 *   `fio_http_params_parse_result_s.err` unchanged.
 * - On error, the parse result holds the partial root, owned by the caller.
 *
 * Required: `on_null`, `on_true`, `on_false`, `on_number`, `on_float`,
 * `on_string` or `on_string_simple`, `on_map`, `on_array`, `map_push`,
 * `array_push` and `free_unused_object`. The parsers fail if one is missing.
 * The table is never written to.
 */
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
  /**
   * Opens a map at `parent[key]` (`key.buf == NULL` appends to an array
   * parent; `parent == NULL` creates the owned root). Reuse an existing map to
   * merge. A replaced value is freed or returned in `old`.
   */
  fio_json_push_result_s (*on_map)(void *udata,
                                   void *parent,
                                   fio_buf_info_s key);
  /** Like `on_map`, opening an array (reuse an existing array to append). */
  fio_json_push_result_s (*on_array)(void *udata,
                                     void *parent,
                                     fio_buf_info_s key);
  /**
   * Stores `value` (owned, taken even on error) at the decoded `map[key]`. A
   * replaced value is freed or returned in `obj` for conflict resolution.
   */
  fio_json_obj_result_s (*map_push)(void *udata,
                                    void *map,
                                    fio_buf_info_s key,
                                    void *value);
  /** Appends `value` (owned, taken even on error). Non-zero on error. */
  int (*array_push)(void *udata, void *array, void *value);
  /** Returns non-zero if `obj` is an array (optional, used for conflicts). */
  int (*is_array)(void *udata, void *obj);
  /** Returns non-zero if `obj` is a map (optional, used for conflicts and
   * required for `fio_http_params_parse` to merge a JSON object body). */
  int (*is_map)(void *udata, void *obj);
  /** Frees an owned unused value (never a key); `obj` may be NULL. */
  void (*free_unused_object)(void *udata, void *obj);

  /* ===== File Uploads (multipart) ===== */

  /**
   * Called when a file upload starts.
   *
   * Returns an owned object for this file (e.g., fd, stream, buffer); NULL
   * skips the file (`on_file_data` won't be called for it). Once the upload
   * ends, the object is stored at the field name like any other value, or
   * passed to `free_unused_object` if parsing failed (including truncated
   * uploads).
   *
   * File data is streamed in chunks (the body is never fully loaded), so large
   * uploads can be written to storage as they are parsed. Filename is stripped
   * of path components; empty filenames and empty field names are skipped.
   * Missing Content-Type has a NULL buf. Raw part headers (without the blank
   * line) are borrowed only for this call.
   */
  void *(*on_file)(void *udata,
                   fio_str_info_s name,
                   fio_str_info_s filename,
                   fio_str_info_s content_type,
                   fio_str_info_s headers);
  /** Called for each chunk of file data. A non-zero return aborts parsing
   * (returned unchanged in `err`); the file object is then freed. */
  int (*on_file_data)(void *udata, void *file, fio_buf_info_s data);
} fio_http_params_parse_callbacks_s;

/** Named arguments for the HTTP params parsing functions. */
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

/**
 * Named HTTP params parsing error values (generic / depth values match JSON).
 *
 * `fio_http_params_parse_result_s.err` is a plain `int`: callback errors pass
 * through unchanged and may coincide with these values.
 */
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

/** HTTP params parse result. */
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

/**
 * Parses the HTTP request body, calling `callbacks` for each element found.
 *
 * Supports JSON, URL-encoded and multipart/form-data bodies, selected by the
 * `Content-Type` header. The type matches case-insensitively, ignoring
 * parameters and leading spaces / tabs. JSON types: `application/json`,
 * `application/<non-empty subtype>+json`, `text/json` and
 * `application/x-json`; `text/plain` is never JSON. Other types report an
 * error without calling callbacks (`result` is NULL).
 *
 * URL-encoded pairs and multipart parts are limited by `.limit` (default:
 * `settings.max_params`, i.e., FIO_HTTP_PARAMS_MAX_FIELDS); exceeding it
 * reports an error.
 *
 * Form name paths and JSON bodies allow up to FIO_JSON_MAX_DEPTH levels
 * (default 128); the root is level 1. Exceeding it reports an error. Invalid
 * name paths stay literal.
 *
 * On error, `result` is the partial root (or NULL), owned by the caller, and
 * `err` holds the error (callback errors pass through unchanged).
 *
 * Parsing preserves the body's reading position; a closed body contributes
 * nothing. A NULL `h` or `callbacks` sets `err` without calling callbacks.
 *
 * @param h    The HTTP handle.
 * @param args Named arguments (see the macro below).
 * @return     Parse result with top-level object, status and the number
 *             of fields charged against the budget (`count`).
 */
SFUNC fio_http_params_parse_result_s FIO_NOOP
fio_http_body_parse(fio_http_s *h, fio_http_params_parse_args_s args);

/** Named arguments helper. See fio_http_body_parse and
 * fio_http_params_parse_args_s. */
#define fio_http_body_parse(http___handle, ...)                                \
  fio_http_body_parse((http___handle),                                         \
                      (fio_http_params_parse_args_s){__VA_ARGS__})

/**
 * Parses the HTTP query string (URL-encoded) into a map.
 *
 * All values are Strings (as with URL-encoded bodies); use
 * `fio_string2primitive` to convert a value when appropriate. Missing values
 * (`key` or `key=`) produce empty Strings. Keys and values are percent / `+`
 * decoded.
 *
 * At most `.limit` pairs are accepted (default: settings.max_params, i.e.,
 * FIO_HTTP_PARAMS_MAX_FIELDS); exceeding it reports an error.
 *
 * Name paths allow up to FIO_JSON_MAX_DEPTH levels (default 128); the root is
 * level 1, with each nested map or terminal [] array adding a level.
 * Exceeding it reports an error. Invalid paths stay literal.
 *
 * On error, `result` is the partial map (or NULL), owned by the caller, and
 * `err` holds the error (callback errors pass through unchanged). A NULL `h`
 * or `callbacks` sets `err` without calling callbacks.
 *
 * @param h    The HTTP handle.
 * @param args Named arguments (see the macro below).
 * @return     Parse result with top-level map, status and the number of
 *             fields charged against the budget (`count`).
 */
SFUNC fio_http_params_parse_result_s FIO_NOOP
fio_http_query_parse(fio_http_s *h, fio_http_params_parse_args_s args);

/** Named arguments helper. See fio_http_query_parse and
 * fio_http_params_parse_args_s. */
#define fio_http_query_parse(http___handle, ...)                               \
  fio_http_query_parse((http___handle),                                        \
                       (fio_http_params_parse_args_s){__VA_ARGS__})

/**
 * Parses the query string and then the body (if any) into a single map.
 *
 * Query pairs are stored first, followed by body fields using JSON conflict
 * rules. JSON object bodies update the map in place (it is passed to
 * `fio_json_parse` as the root, which requires `is_map`); other JSON roots
 * are ignored. A JSON body counts as an object only if it starts with `{`
 * after an optional BOM and whitespace (leading comments are not skipped).
 * Bodies with a missing / unknown Content-Type are ignored. Query and form
 * fields share a single `.limit` budget (default: settings.max_params,
 * i.e., FIO_HTTP_PARAMS_MAX_FIELDS); exceeding it reports an error.
 * JSON bodies are not counted.
 * Name paths and JSON bodies allow up to FIO_JSON_MAX_DEPTH levels (default
 * 128); the root is level 1. A name with N segments opens N containers, plus
 * one for terminal []; exceeding the bound reports an error.
 * Invalid name paths stay literal.
 * Parsing preserves the body's reading position; a closed body contributes
 * nothing.
 *
 * The result is always the params map, owned by the caller; on error it is the
 * partial map (NULL if the root could not be created) and `err` holds the
 * error (callback errors pass through unchanged). `consumed` is the sum of the
 * query and body bytes consumed; `count` is the combined number of query and
 * form fields charged against the budget, so a caller can deduct it from a
 * shared application budget.
 *
 * @param h    The HTTP handle.
 * @param args Named arguments (see the macro below).
 * @return     Parse result with top-level map, status and field `count`.
 */
SFUNC fio_http_params_parse_result_s FIO_NOOP
fio_http_params_parse(fio_http_s *h, fio_http_params_parse_args_s args);

/** Named arguments helper. See fio_http_params_parse and
 * fio_http_params_parse_args_s. */
#define fio_http_params_parse(http___handle, ...)                              \
  fio_http_params_parse((http___handle),                                       \
                        (fio_http_params_parse_args_s){__VA_ARGS__})

/* *****************************************************************************
Header Value / Property Iteration
***************************************************************************** */

/**
 * A zero-copy cursor over repeated header fields, comma-separated values and
 * semicolon-separated properties. `name` / `value` are length-delimited slices
 * borrowed from the HTTP handle, NOT necessarily NUL-terminated. Quotes and
 * escapes are retained, not decoded. Do not mutate headers during iteration.
 * `properties` holds the bounded property span (or remaining properties).
 * `index` and `is_request` are iterator state; do not change them.
 * `value.buf == NULL` marks the end; a property may have a non-NULL value
 * with zero length. Empty list elements and empty property names are skipped.
 */
typedef struct fio_http_header_each_s {
  fio_buf_info_s name;       /* header name, or property name */
  fio_buf_info_s value;      /* list value, or property value */
  fio_buf_info_s properties; /* property span; cursor in property iterator */
  unsigned index;            /* index of the current repeated header field */
  unsigned is_request;
} fio_http_header_each_s;

/** Starts iterating a header's values. `header_name` is a borrowed lookup key.
 * Returns an iterator with `value.buf == NULL` when no value is available. */
FIO_IFUNC fio_http_header_each_s
fio_http_header_each_first(fio_http_s *http_handle,
                           int is_request,
                           fio_str_info_s header_name);

/** Advances to the next value; `http_handle` must be the original handle.
 * Sets `value.buf` to NULL after the last value. */
FIO_IFUNC void fio_http_header_each_next(fio_http_header_each_s *pos,
                                         fio_http_s *http_handle);

/** Starts iterating a value's bounded property span. `properties` is borrowed.
 * Returns an iterator with `value.buf == NULL` when no property exists. */
FIO_IFUNC fio_http_header_each_s
fio_http_header_each_property_first(const fio_buf_info_s *properties);

/** Advances to the next property. A property without `=` has an empty value.
 * Sets `value.buf` to NULL after the last property. */
FIO_IFUNC void fio_http_header_each_property_next(fio_http_header_each_s *pos);

/** Iterate repeated request (`is_request = 1`) or response (`0`) header
 * fields and their comma-separated values; `pos` is a chosen cursor name.
 * SP / HTAB around each value is trimmed. Use `.value.buf` and `.value.len`,
 * not string functions, for the non-NUL-terminated slice. Do not mutate the
 * handle while iterating. `break` / `continue` work as in a regular for loop.
 * Example:
 *   FIO_HTTP_HEADER_EACH_VALUE(h, 1, FIO_STR_INFO1("accept"), item) {
 *     FIO_HTTP_HEADER_EACH_PROPERTY(item, property) {
 *       // property.name and property.value are length-delimited slices.
 *     }
 *   }
 */
#define FIO_HTTP_HEADER_EACH_VALUE(http_handle, is_request, header_name, pos)  \
  for (fio_http_header_each_s pos = fio_http_header_each_first((http_handle),  \
                                                               (is_request),   \
                                                               (header_name)); \
       pos.value.buf;                                                          \
       fio_http_header_each_next(&pos, (http_handle)))

/** Iterate semicolon-separated properties for a value cursor; `pos` is a
 * chosen name. Names and values are trimmed of surrounding SP / HTAB.
 * A missing `=` or an empty right-hand side yields `value.len == 0` with a
 * non-NULL `value.buf`; stop iteration only when `value.buf == NULL`.
 * Quoted strings remain quoted, without decoding. `break` / `continue` work.
 */
#define FIO_HTTP_HEADER_EACH_PROPERTY(value_, pos)                             \
  for (fio_http_header_each_s pos =                                            \
           fio_http_header_each_property_first(&(value_).properties);          \
       pos.value.buf;                                                          \
       fio_http_header_each_property_next(&pos))

/* *****************************************************************************
General Helpers
***************************************************************************** */

/**
 * Sends an error response with `status` and finishes the response.
 *
 * A `status` of 0 or above 1000 becomes 404. The body is the file
 * `<status>.html` from `./` or from the route's `public_folder`, if found.
 * Otherwise a plain text body is sent: `Error <status>: <status text>.`
 *
 * Returns 0 on success, or -1 if `h` is NULL or headers were already sent.
 */
SFUNC int fio_http_send_error_response(fio_http_s *h, size_t status);

/**
 * Returns 1 if the request's `If-None-Match` condition matches the response,
 * so a `304 Not Modified` should be sent. Otherwise returns 0.
 *
 * Follows RFC 9110 §13.1.2: only `GET` and `HEAD` requests with a 2xx
 * response status are evaluated (§13.2.1); entity tags use the weak
 * comparison (a `W/` prefix is ignored); every listed tag and every
 * `If-None-Match` header line is checked. A field value of `*` matches any
 * 2xx `GET` / `HEAD` response, even without an `etag` header. A `*` mixed
 * with other tags is invalid and never matches.
 */
SFUNC int fio_http_etag_is_match(fio_http_s *h);

/**
 * Attempts to send a static file from the `root_folder` folder.
 *
 * `file_name` is URL-decoded and appended to `root_folder` (path traversal
 * is rejected). Folders resolve to their `index` file and missing
 * extensions are auto-completed (`.html`, `.htm`, `.txt`, `.md` -
 * `FIO_HTTP_STATIC_FILE_COMPLETION`). `OPTIONS` requests are refused (a
 * static file is not a valid `OPTIONS` response); other methods are not
 * checked.
 *
 * `root_folder` MUST name an existing folder (use `"."` for the CWD):
 * the path-traversal guard rejects `..` folding, not absolute paths, so
 * an empty root would expose absolute paths such as `/etc/passwd`.
 * Settings-provided `public_folder` values are validated on listen/route
 * creation (non-existent folders are rejected); direct callers MUST pass
 * a valid, non-empty root. Symlinks inside the root are followed (like
 * nginx's default) - applications requiring strict containment should
 * keep the root free of symlinks.
 *
 * Handles conditional requests (`ETag` / `If-None-Match` -> 304), single
 * `Range` requests (206 / 416, always served identity), and `HEAD`
 * requests; sets `Last-Modified`, `Accept-Ranges: bytes`, and (when
 * `max_age` is non-zero) `Cache-Control: max-age=...`.
 *
 * Pre-compressed variants (`file.br`, `file.zstd`, `file.gz`, `file.zip`,
 * in this preference order) are served when accepted by the client,
 * present on disk, and fresh; with `compress_static` enabled in the matching
 * route's settings, missing `.br` / `.gz` variants are also created on
 * demand.
 *
 * On success the response is complete and 0 is returned. On failure -1 is
 * returned and the application should handle the request itself.
 */
SFUNC int fio_http_static_file_response(fio_http_s *h,
                                        fio_str_info_s root_folder,
                                        fio_str_info_s file_name,
                                        size_t max_age);

/**
 * Returns the reason phrase for an HTTP status code (e.g., `"Not Found"`).
 *
 * The string is static; do not free it. Unknown codes return `"Unknown"`.
 */
SFUNC fio_str_info_s fio_http_status2str(size_t status);

/**
 * Writes one access log line for the request / response to `stdout`.
 *
 * The line includes the peer address (see `fio_http_from`), the request line,
 * status, bytes sent and the time since `fio_http_received_at`. Afterwards
 * `received_at` is reset to the current time.
 */
SFUNC void fio_http_write_log(fio_http_s *h);

/**
 * Appends the peer address to `dest` and NUL-terminates it.
 *
 * Sources, in order: the `for=` value of the last `forwarded` header, the
 * first address in the last `x-forwarded-for` header, the socket's peer
 * address, and finally `"[unknown]"`.
 *
 * Returns 0 if the address came from a forwarded header (client supplied).
 * Returns -1 if no forwarded header was found; the socket address (or
 * `"[unknown]"`) was written. If `dest` lacks capacity
 * (`dest->capa <= dest->len + address length`), nothing is written and the
 * address length is returned.
 *
 * **Note**: forwarded headers are set by the client or a proxy and can be
 * spoofed. Trust them only behind a proxy you control.
 */
SFUNC int fio_http_from(fio_str_info_s *dest, const fio_http_s *h);

/**
 * Returns `now_in_seconds` as an HTTP `date` header value (RFC 7231 format).
 *
 * The result points to a static buffer cached per second. **NOT**
 * thread-safe; copy the result before another call.
 */
SFUNC fio_str_info_s fio_http_date(uint64_t now_in_seconds);

/**
 * Returns `now_in_seconds` formatted for the access log.
 *
 * The result points to a static buffer cached per second. **NOT**
 * thread-safe; copy the result before another call.
 */
SFUNC fio_str_info_s fio_http_log_time(uint64_t now_in_seconds);
/* *****************************************************************************
The HTTP Controller
***************************************************************************** */

/**
 * The HTTP Controller holds the callbacks the HTTP handle uses to send
 * requests and responses. Missing (NULL) callbacks get no-op defaults.
 */
struct fio_http_controller_s {
  /** Reserved. MUST be initialized to zero. */
  uintptr_t private_flags;
  /** Called when an HTTP handle is freed. */
  void (*on_destroyed)(fio_http_s *h);
  /** Informs the controller that request / response headers must be sent. */
  void (*send_headers)(fio_http_s *h);
  /** Called for each body chunk, or to finish a response. */
  void (*write_body)(fio_http_s *h, fio_http_write_args_s args);
  /** Called once a request / response has finished. */
  void (*on_finish)(fio_http_s *h);
  /** Called to close the HTTP connection. */
  void (*close_io)(fio_http_s *h);
  /** Called when the file descriptor is directly required. */
  int (*get_fd)(fio_http_s *h);
};

/* *****************************************************************************
WebSocket / SSE Helpers
***************************************************************************** */

/**
 * Returns non-zero if the request headers ask for a WebSocket upgrade.
 *
 * Returns 1 for a valid request, -1 (still non-zero) if the
 * `sec-websocket-version` is not 13, or 0 if no upgrade was requested.
 */
SFUNC int fio_http_websocket_requested(fio_http_s *);

/**
 * Client side: returns non-zero if the response accepts the WebSocket
 * upgrade request. On success the handle is marked as upgraded.
 */
SFUNC int fio_http_websocket_accepted(fio_http_s *h);

/**
 * Server side: sends a `101` response that accepts a WebSocket upgrade.
 *
 * Sends a 400 error for an unsupported version, or a 403 error for an
 * invalid `sec-websocket-key`. The server calls this automatically after
 * `on_authenticate_websocket` allows the request.
 */
SFUNC void fio_http_upgrade_websocket(fio_http_s *);

/** Client side: sets the request headers that ask for a WebSocket upgrade. */
SFUNC void fio_http_websocket_set_request(fio_http_s *);

/** Returns non-zero if the `accept` header asks for `text/event-stream`. */
SFUNC int fio_http_sse_requested(fio_http_s *);

/**
 * Client side: returns non-zero if the response accepts the SSE request.
 * On success the handle is marked as upgraded.
 */
SFUNC int fio_http_sse_accepted(fio_http_s *h);

/**
 * Server side: sends the response headers that accept an EventSource (SSE)
 * request and finishes the response. Does nothing if the response has
 * already started.
 */
SFUNC void fio_http_upgrade_sse(fio_http_s *);

/** Client side: sets the request headers that ask for an SSE stream. */
SFUNC void fio_http_sse_set_request(fio_http_s *);

/* *****************************************************************************
WebSocket Helpers - HTTP Upgraded Connections
***************************************************************************** */

/**
 * Writes a WebSocket message; `is_text` selects a text or binary frame.
 *
 * The message is compressed when permessage-deflate was negotiated and `len`
 * is at least `FIO_HTTP_WEBSOCKET_DEFLATE_MIN` bytes.
 *
 * Returns 0 on success, or -1 if `h` is not an upgraded WebSocket handle or
 * has no connection.
 */
SFUNC int fio_http_websocket_write(fio_http_s *h,
                                   const void *buf,
                                   size_t len,
                                   uint8_t is_text);

/**
 * Sets the `on_message` callback for this connection only.
 *
 * A NULL `on_message` restores the callback from the settings. Returns 0 on
 * success, or -1 if `h` is NULL or has no connection.
 */
SFUNC int fio_http_on_message_set(fio_http_s *h,
                                  void (*on_message)(fio_http_s *,
                                                     fio_buf_info_s,
                                                     uint8_t));

/**
 * Optional subscription callback: forwards each pub/sub message to the
 * WebSocket. Valid UTF-8 messages shorter than
 * `FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT` are sent as text; all others
 * are sent as binary.
 */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);
/** Optional subscription callback: forwards messages as text frames.
 * Use only when every message is valid UTF-8. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_pubsub_msg_s *msg);
/** Optional subscription callback: forwards messages as binary frames. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY(fio_pubsub_msg_s *msg);

/* *****************************************************************************
EventSource (SSE) Helpers - HTTP Upgraded Connections
***************************************************************************** */

/** Named arguments for fio_http_sse_write. */
typedef struct {
  /** The message's `id` data (if any). */
  fio_buf_info_s id;
  /** The message's `event` data (if any). */
  fio_buf_info_s event;
  /** The message's `data` (required; empty data fails). */
  fio_buf_info_s data;
} fio_http_sse_write_args_s;

/**
 * Writes an SSE message (UTF-8). Multi-line `data` is sent as one `data:`
 * line per line.
 *
 * Returns 0 on success, or -1 if `data` is empty, `h` is not an upgraded SSE
 * handle, or the connection is gone.
 */
SFUNC int fio_http_sse_write(fio_http_s *h, fio_http_sse_write_args_s args);

/** Writes an SSE message (named args, see `fio_http_sse_write_args_s`). */
#define fio_http_sse_write(h, ...)                                             \
  fio_http_sse_write((h), ((fio_http_sse_write_args_s){__VA_ARGS__}))

/**
 * Optional subscription callback: forwards each pub/sub message as an SSE
 * event. The `id` is the message id in hex and the `event` is the channel
 * name. Messages MUST be UTF-8.
 */
SFUNC void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);

/* *****************************************************************************
HTTP Settings Resolver
***************************************************************************** */

/**
 * Returns the settings of the route matching the handle's original path.
 *
 * Returns NULL if `h` is NULL or has no server connection.
 */
SFUNC fio_http_settings_s *fio_http_settings(fio_http_s *);

/* *****************************************************************************







                          Inline / Macro Implementations







***************************************************************************** */

/* *****************************************************************************
Header Value / Property Iterator - Inline Implementation
***************************************************************************** */

/* Returns a trimmed slice (HTTP OWS is SP / HTAB only). */
FIO_IFUNC fio_buf_info_s fio___http_header_each_trim(fio_buf_info_s slice) {
  if (!slice.buf)
    return slice;
  while (slice.len && (*slice.buf == ' ' || *slice.buf == '\t')) {
    ++slice.buf;
    --slice.len;
  }
  while (slice.len &&
         (slice.buf[slice.len - 1] == ' ' || slice.buf[slice.len - 1] == '\t'))
    --slice.len;
  return slice;
}

/* Scan NUL-terminated header storage without decoding quoted strings.
 * An escaped quote/delimiter inside quotes is not a separator. */
FIO_IFUNC const char *fio___http_header_each_find(const char *p,
                                                  char first,
                                                  char second) {
  for (unsigned quoted = 0;
       *p && (quoted | ((unsigned)(*p != first) & (unsigned)(*p != second)));
       ++p) {
    quoted ^= (unsigned)(*p == '"');
    p += (unsigned)(quoted && *p == '\\' && p[1]);
  }
  return p;
}

/* Scan a property span, stopping at end or NUL. Never read beyond end,
 * including when a backslash is its final byte. */
FIO_IFUNC const char *fio___http_header_each_find_bounded(const char *p,
                                                          const char *end,
                                                          char first,
                                                          char second) {
  for (unsigned quoted = 0;
       p < end && *p &&
       (quoted | ((unsigned)(*p != first) & (unsigned)(*p != second)));
       ++p) {
    quoted ^= (unsigned)(*p == '"');
    p += (unsigned)(quoted && *p == '\\' && p + 1 < end && p[1]);
  }
  return p;
}

/* Value iteration uses the NUL byte supplied by HTTP header storage.
 * name retains the header lookup key; index changes only between fields.
 * value.buf == NULL means end, not an empty token (empty list items skip).
 * The handle and its headers must stay alive and unmodified during iteration.
 */
FIO_IFUNC void fio_http_header_each_next(fio_http_header_each_s *pos,
                                         fio_http_s *http_handle) {
  if (!pos)
    return;
  if (!http_handle) {
    pos->value = FIO_BUF_INFO0;
    return;
  }
  const char *p = NULL;
  if (pos->value.buf) {
    /* The property span ends at the current comma or the field's NUL.
     * Without properties, resume just after the trimmed value. */
    p = pos->properties.buf ? pos->properties.buf + pos->properties.len
                            : pos->value.buf + pos->value.len;
    p = fio___http_header_each_find(p, ',', ',');
    if (*p == ',')
      ++p;
    else {
      if (pos->index == INT32_MAX) {
        pos->value = FIO_BUF_INFO0;
        return;
      }
      ++pos->index;
      p = NULL;
    }
  }
  for (;;) {
    if (!p) {
      fio_str_info_s name = FIO_STR_INFO2(pos->name.buf, pos->name.len);
      fio_str_info_s field =
          pos->is_request
              ? fio_http_request_header(http_handle, name, pos->index)
              : fio_http_response_header(http_handle, name, pos->index);
      if (!field.buf || !field.len) {
        pos->value = FIO_BUF_INFO0;
        pos->properties = FIO_BUF_INFO0;
        return;
      }
      p = field.buf;
    }
    const char *separator = fio___http_header_each_find(p, ',', ';');
    const char *end = separator;
    if (*separator == ';')
      end = fio___http_header_each_find(separator + 1, ',', ',');
    pos->value = fio___http_header_each_trim(
        FIO_BUF_INFO2((char *)p, (size_t)(separator - p)));
    pos->properties = (*separator == ';')
                          ? FIO_BUF_INFO2((char *)separator + 1,
                                          (size_t)(end - separator - 1))
                          : FIO_BUF_INFO0;
    if (pos->value.len)
      return;
    if (*end == ',') {
      p = end + 1;
      continue;
    }
    if (pos->index == INT32_MAX) {
      pos->value = FIO_BUF_INFO0;
      return;
    }
    ++pos->index;
    p = NULL;
  }
}

FIO_IFUNC fio_http_header_each_s
fio_http_header_each_first(fio_http_s *http_handle,
                           int is_request,
                           fio_str_info_s header_name) {
  fio_http_header_each_s pos = {0};
  if (!http_handle || !header_name.buf || !header_name.len)
    return pos;
  pos.name = FIO_BUF_INFO2(header_name.buf, header_name.len);
  pos.is_request = !!is_request;
  fio_http_header_each_next(&pos, http_handle);
  return pos;
}

/* Property iteration is bounded by properties.len: the next comma is not a
 * property separator. Empty property values have a non-NULL pointer. */
FIO_IFUNC void fio_http_header_each_property_next(fio_http_header_each_s *pos) {
  if (!pos)
    return;
  pos->value = FIO_BUF_INFO0;
  while (pos->properties.len) {
    const char *start = pos->properties.buf;
    const char *limit = start + pos->properties.len;
    const char *semi =
        fio___http_header_each_find_bounded(start, limit, ';', ';');
    const char *eq = fio___http_header_each_find_bounded(start, semi, '=', '=');
    pos->name = fio___http_header_each_trim(
        FIO_BUF_INFO2((char *)start, (size_t)(eq - start)));
    pos->value =
        (eq < semi)
            ? fio___http_header_each_trim(
                  FIO_BUF_INFO2((char *)eq + 1, (size_t)(semi - eq - 1)))
            : FIO_BUF_INFO2((char *)semi, 0);
    pos->properties.buf = (char *)semi + (semi < limit);
    pos->properties.len = (size_t)(limit - semi) - (semi < limit);
    if (pos->name.len)
      return;
  }
  pos->value = FIO_BUF_INFO0;
}

FIO_IFUNC fio_http_header_each_s
fio_http_header_each_property_first(const fio_buf_info_s *properties) {
  fio_http_header_each_s pos = {0};
  if (properties && properties->buf && properties->len) {
    pos.properties = *properties;
    fio_http_header_each_property_next(&pos);
  }
  return pos;
}

/* *****************************************************************************
HTTP Handle Implementation - inlined static functions
***************************************************************************** */

#define FIO___HTTP_GETSET_PTR(type, name, index_, pre_set_code)                \
  /** Get the name property at its known pointer index. */                     \
  FIO_IFUNC type *fio_http_##name(fio_http_s *h) {                             \
    return ((type **)h)[index_];                                               \
  }                                                                            \
  /** Set the name property at its known pointer index. */                     \
  FIO_IFUNC type *fio_http_##name##_set(fio_http_s *h, type *ptr) {            \
    pre_set_code;                                                              \
    return (((type **)h)[index_] = ptr);                                       \
  }

SFUNC fio_http_controller_s *fio___http_controller_validate(
    fio_http_controller_s *c);

/** fio_http_udata / fio_http_udata_set */
FIO___HTTP_GETSET_PTR(void, udata, 0, (void)0)
/** fio_http_udata2 / fio_http_udata2_set */
FIO___HTTP_GETSET_PTR(void, udata2, 1, (void)0)
/** fio_http_cdata / fio_http_cdata_set */
FIO___HTTP_GETSET_PTR(void, cdata, 2, (void)0)
/** fio_http_controller / fio_http_controller_set */
FIO___HTTP_GETSET_PTR(fio_http_controller_s,
                      controller,
                      3,
                      ptr = fio___http_controller_validate(ptr))

#undef FIO___HTTP_GETSET_PTR

/* *****************************************************************************
Path Section Looping
***************************************************************************** */

#undef FIO_HTTP_PATH_EACH

/**
 * Loops over each `/`-separated section of `path`, decoding percent encoding.
 *
 * - `path`: the path string (a `fio_str_info_s`, e.g., `fio_http_path(h)`).
 * - `pos`: the name of the loop variable, a `fio_buf_info_s` holding the
 *   decoded, NUL-terminated section. It is valid only for one iteration.
 *
 * The loop also declares `<pos>_reminder` (the rest of the path) and a 4096
 * byte stack buffer named `buf`, which hides any outer `buf` in the loop body.
 *
 * **Note**: the loop stops early if a section is longer than about 4063
 * bytes.
 */
#define FIO_HTTP_PATH_EACH(path, pos)                                          \
  for (fio_buf_info_s pos##_reminder = FIO_STR2BUF_INFO(path),                 \
                      pos,                                                     \
                      buf[(4096 / sizeof(fio_buf_info_s)) - 2];                \
       fio___http_path_each_next(&pos, &pos##_reminder, buf);)

/* used internally for the macro FIO_HTTP_PATH_EACH */
FIO_IFUNC bool fio___http_path_each_next(fio_buf_info_s *restrict section,
                                         fio_buf_info_s *restrict path,
                                         void *restrict mem) {
  if (path->len < 2)
    return 0;
  const char *p = path->buf;
  const char *e = p + path->len;
  char *w = (char *)mem;
  const char *w_start = w;
  const char *w_end = w + (4096 - ((sizeof(fio_buf_info_s) * 2) + 1));
  p += (*p == '/');
  while (p < e && *p != '/' && w < w_end) {
    while (p < e && *p != '%' && *p != '/' && w < w_end)
      *(w++) = *(p++);
    if (p == e || *p == '/')
      break;
    uint8_t hi, lo;
    if (p + 3 > e || ((hi = fio_c2i((unsigned char)p[1])) > 15) ||
        ((lo = fio_c2i((unsigned char)p[2])) > 15)) {
      *(w++) = *(p++);
      continue;
    }
    p += 3;
    *(w++) = (char)((hi << 4) | lo);
  }
  *w = 0;
  *section = FIO_BUF_INFO2((char *)w_start, (size_t)(w - w_start));
  *path = FIO_BUF_INFO2((char *)p, (size_t)(e - p));
  return (p == e || *p == '/');
}

/* *****************************************************************************
Module Implementation - HTTP Routing – CRUD
***************************************************************************** */

/** returns expected action or `FIO_HTTP_RESOURCE_NONE` on error. */
FIO_IFUNC fio_http_resource_action_e fio_http_resource_action(fio_http_s *h) {
  fio_http_resource_action_e r = FIO_HTTP_RESOURCE_NONE;
  if (!h)
    return r;
  const uint32_t new_s = fio_buf2u32u("/new") | 0x20202020U;
  const uint32_t edit_s = fio_buf2u32u("edit");
  const uint32_t get = fio_buf2u32u("get\x20");
  const uint32_t put = fio_buf2u32u("put\x20");
  const uint32_t post = fio_buf2u32u("post");
  const uint32_t patc = fio_buf2u32u("patc");
  const uint32_t dele = fio_buf2u32u("dele");
  const uint32_t lete = fio_buf2u32u("lete");
  const uint32_t query = fio_buf2u32u("quer");
  fio_str_info_s method = fio_http_method(h);
  fio_str_info_s path = fio_http_path(h);
  bool path_ends_with_dash = (path.len && path.buf[path.len - 1] == '/');
  bool path_is_new = ((path.len == 4 || (path.len > 4 && path.buf[4] == '/')) &&
                      ((fio_buf2u32u(path.buf) | 0x20202020U) == new_s));
  if (method.len < 3)
    return r;
  uint32_t tmp = fio_buf2u32u(method.buf) | 0x20202020U; /* down-case */
  /* GET */
  if (tmp == get) {
    bool path_is_edit =
        ((path.len > 6) &&
         path.buf[path.len - (5 + path_ends_with_dash)] == '/' &&
         ((fio_buf2u32u((path.buf + path.len) - (4 + path_ends_with_dash)) |
           0x20202020U) == edit_s));
    /* index vs show */
    r = (fio_http_resource_action_e)((unsigned)FIO_HTTP_RESOURCE_INDEX +
                                     (path.len > 1));
    /* show vs new */
    r = (fio_http_resource_action_e)((unsigned)r +
                                     (path_is_new & (!path_is_edit)));
    /* show vs edit */
    r = (fio_http_resource_action_e)((unsigned)r +
                                     ((unsigned)path_is_edit << 1));
    /* new/edit collision */
    r = (fio_http_resource_action_e)((unsigned)r -
                                     (((unsigned)(r == FIO_HTTP_RESOURCE_EDIT) &
                                       path_is_new) *
                                      FIO_HTTP_RESOURCE_EDIT));
    /* PUT/POST/PATCH */
  } else if (tmp == put || (tmp == post && method.len == 4) ||
             (tmp == patc && ((method.buf[4] | 32) == 'h') &&
              method.len == 5)) {
    /* create vs edit */
    r = (fio_http_resource_action_e)((unsigned)FIO_HTTP_RESOURCE_CREATE +
                                     (path.len > 1 && !path_is_new));
    /* DELETE */
  } else if (path.len > 1 && !path_is_new && method.len == 6 && tmp == dele &&
             (fio_buf2u32u(method.buf + 2) | 0x20202020U) == lete) {
    r = FIO_HTTP_RESOURCE_DELETE;
  } else if (tmp == query && method.len == 5 && (method.buf[4] | 32) == 'y') {
    r = FIO_HTTP_RESOURCE_QUERY;
  }
  return r;
}

/* *****************************************************************************
HTTP API Finish
***************************************************************************** */
#endif /* FIO_HTTP */
