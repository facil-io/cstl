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
/** The default HTTP total header size limit in bytes. */
#define FIO_HTTP_DEFAULT_MAX_HEADER_SIZE 32768 /* (1UL << 15) */
#endif
#ifndef FIO_HTTP_DEFAULT_MAX_LINE_LEN
/** The default HTTP header line limit in bytes. */
#define FIO_HTTP_DEFAULT_MAX_LINE_LEN 8192 /* (1UL << 13) */
#endif
#ifndef FIO_HTTP_DEFAULT_MAX_BODY_SIZE
/** The default HTTP payload size limit in bytes. */
#define FIO_HTTP_DEFAULT_MAX_BODY_SIZE 33554432 /* (1UL << 25) */
#endif
#ifndef FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE
/** The default WebSocket message size limit in bytes. */
#define FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE 262144 /* (1UL << 18) */
#endif
#ifndef FIO_HTTP_DEFAULT_TIMEOUT
/** The default timeout for HTTP connections. */
#define FIO_HTTP_DEFAULT_TIMEOUT 50
#endif
#ifndef FIO_HTTP_DEFAULT_TIMEOUT_LONG
/** The default timeout for long held HTTP connections (WebSockets / SSE). */
#define FIO_HTTP_DEFAULT_TIMEOUT_LONG 50
#endif

#ifndef FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER
/** Adds a "content-length" header to the HTTP handle (usually redundant). */
#define FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER 0
#endif

#ifndef FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT
/** UTF-8 validity tests will be performed only for data shorter than this. */
#define FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT ((1UL << 16) - 10UL)
#endif

#ifndef FIO_WEBSOCKET_STATS
/** If true, logs longest WebSocket round-trips (using FIO_LOG_INFO). */
#define FIO_WEBSOCKET_STATS 0
#endif

#ifndef FIO_HTTP_WEBSOCKET_DEFLATE_MIN
/** Messages smaller than this are not compressed (fits in a single TCP/IP
 * packet, compression saves no network overhead). */
#define FIO_HTTP_WEBSOCKET_DEFLATE_MIN 1024
#endif

/* *****************************************************************************
HTTP Handle Settings
***************************************************************************** */
#ifndef FIO_HTTP_EXACT_LOGGING
/**
 * By default, facil.io logs the HTTP request cycle using a fuzzy starting and
 * ending point for the time stamp.
 *
 * The fuzzy timestamp includes delays that aren't related to the HTTP request
 * and may ignore time passed due to timestamp caching.
 *
 * On the other hand, `FIO_HTTP_EXACT_LOGGING` collects exact time stamps to
 * measure the time it took to process the HTTP request (excluding time spent
 * reading / writing the data from the network).
 *
 * Due to the preference to err on the side of higher performance, fuzzy
 * time-stamping is the default.
 */
#define FIO_HTTP_EXACT_LOGGING 0
#ifndef H___FIO_IO___H
#undef FIO_HTTP_EXACT_LOGGING
#define FIO_HTTP_EXACT_LOGGING 1
#endif
#endif

#ifndef FIO_HTTP_BODY_RAM_LIMIT
/**
 * The HTTP handle automatically switches between RAM storage and file storage
 * once the HTTP body (payload) reaches a certain size. This control this point
 * of transition
 */
#define FIO_HTTP_BODY_RAM_LIMIT (1 << 17)
#endif

#ifndef FIO_HTTP_CACHE_LIMIT
/** Each of the HTTP String Caches will be limited to this String count. */
#define FIO_HTTP_CACHE_LIMIT 0 /* ((1UL << 6) + (1UL << 5)) */
#endif

#ifndef FIO_HTTP_CACHE_STR_MAX_LEN
/** The HTTP handle will avoid caching strings longer than this value. */
#define FIO_HTTP_CACHE_STR_MAX_LEN (1 << 12)
#endif

#ifndef FIO_HTTP_CACHE_USES_MUTEX
/** The HTTP cache will use a mutex to allow headers to be set concurrently. */
#define FIO_HTTP_CACHE_USES_MUTEX 1
#endif

#ifndef FIO_HTTP_PRE_CACHE_KNOWN_HEADERS
/** Adds a static cache for common HTTP header names. */
#define FIO_HTTP_PRE_CACHE_KNOWN_HEADERS 1
#endif

#ifndef FIO_HTTP_DEFAULT_INDEX_FILENAME
/** The default file name when a static file response points to a folder. */
#define FIO_HTTP_DEFAULT_INDEX_FILENAME "index"
#endif

#ifndef FIO_HTTP_STATIC_FILE_COMPLETION
/** Attempts to auto-complete static file paths with missing extensions. */
#define FIO_HTTP_STATIC_FILE_COMPLETION 1
#endif

#ifndef FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT
/** Maximum file size (in bytes) for on-disk static file compression. */
#define FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT (1UL << 21) /* 2 MiB */
#endif

#ifndef FIO_HTTP_LOG_X_REQUEST_START
#define FIO_HTTP_LOG_X_REQUEST_START 1
#endif

#ifndef FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
/** If true, the HTTP handle will copy input header names to lower case. */
#define FIO_HTTP_ENFORCE_LOWERCASE_HEADERS 0
#endif

/* *****************************************************************************
HTTP Handle Type
***************************************************************************** */

/**
 * The HTTP Handle type.
 *
 * Note that the type is NOT designed to be thread-safe.
 */
typedef struct fio_http_s fio_http_s;

/**
 * The HTTP Controller points to all the callbacks required by the HTTP Handler.
 *
 * This allows the HTTP Handler to be somewhat protocol agnostic.
 *
 * Note: if the controller callbacks aren't thread-safe, than the `http_write`
 * function MUST NOT be called from any thread except the thread that the
 * controller is expecting.
 */
typedef struct fio_http_controller_s fio_http_controller_s;

/* *****************************************************************************
HTTP Settings Type
***************************************************************************** */

typedef struct fio_http_settings_s {
  /** Called before body uploads, when a client sends an `Expect` header. */
  void (*pre_http_body)(fio_http_s *h);
  /** Callback for HTTP requests (server) or responses (client). */
  void (*on_http)(fio_http_s *h);
  /** Called when a request / response cycle is finished (for WebSocket /
   * SSE connections, called after `on_close`, when the connection closes). */
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
   * A public folder for file transfers - allows to circumvent any
   * application layer logic and simply serve static files.
   *
   * Static file responses are attempted for `GET` and `HEAD` requests
   * only (RFC 9110 §9.3 - a static file is not a valid response to other
   * methods); any other method is forwarded to `on_http` (which may serve
   * files explicitly by calling `fio_http_static_file_response`). On a
   * miss the request is also forwarded to `on_http`. Folders resolve to
   * their `index` file and missing extensions are auto-completed
   * (`.html`, `.htm`, `.txt`, `.md`) - see
   * `FIO_HTTP_STATIC_FILE_COMPLETION`.
   *
   * Pre-compressed variants are supported: when the client's
   * `Accept-Encoding` allows it, an up-to-date `file.br`, `file.zstd`,
   * `file.gz` or `file.zip` variant (preference order: `br`, `zstd`,
   * `gzip`, `deflate`) is served instead of the original file. With
   * `compress_static`, missing `.br` / `.gz` variants are also created on
   * demand (written into this folder). Ranged requests are always served
   * identity (no variant selection).
   *
   * The folder must exist when the listener starts, otherwise the setting
   * is ignored (with an error log).
   */
  fio_str_info_s public_folder;
  /**
   * The max-age value (in seconds) for caching static files sent from
   * `public_folder`.
   *
   * Defaults to 0 (the `Cache-Control` header is not sent).
   *
   * Note: NOT inherited by routes - a route that serves static files must
   * set its own `max_age`.
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
  /**
   * Opt-in: auto-compress static files - missing `.br` / `.gz` variants are
   * created on demand and written into the `public_folder`.
   *
   * Creation is limited to compressible (text-like) MIME types and file
   * sizes between 1024 bytes and FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT (2
   * MiB), and is skipped when compression wouldn't shrink the file.
   * Variants whose modification time is older than the original file are
   * re-created.
   *
   * Note: on-demand writes are attacker-triggerable - the public folder
   * MUST be writable and quota'd. Pre-generating variants at deploy time is
   * recommended.
   *
   * Note: per-route (read from the matching route's settings; routes
   * inherit the listener's root value at route-creation). The value is a
   * failure-memoization shift register, atomically shared by the IO
   * threads - see `fio_http_static_file_response`. Detached handles (no
   * settings) gate on the `FIO_HTTP_CFLAG_COMPRESS_STATIC` handle cflag
   * instead, with no memoization state.
   */
  uint8_t compress_static;
  /**
   * Opt-in: auto-compress dynamic HTTP responses on-the-fly.
   *
   * Applies to finished (non-streaming) single-buffer responses larger than
   * 1024 bytes with a compressible (text-like) `Content-Type`, preferring
   * `br` over `gzip` per the client's `Accept-Encoding`. Responses with a
   * pre-set `Content-Encoding` are never re-compressed. `Vary:
   * accept-encoding` is set whenever compression was considered.
   *
   * Note: connection-global (read from the listener's root settings when
   * the connection's handle is attached) - per-route values have no effect.
   */
  uint8_t compress_dynamic;
  /**
   * Opt-in: enable permessage-deflate for WebSocket connections.
   *
   * Negotiation always forces both `*_no_context_takeover` flags (RFC 7692
   * allows either endpoint to request them unilaterally), keeping
   * persistent per-connection compression state at ~0.
   *
   * Note: connection-global (read from the listener's root settings when
   * the connection's handle is attached) - per-route values have no effect.
   */
  uint8_t compress_ws;
} fio_http_settings_s;

/* *****************************************************************************
HTTP Connection Helpers
***************************************************************************** */

/** Allows all clients to connect (bypasses authentication). */
SFUNC int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h);

/** Returns the IO object associated with the HTTP object (request only). */
SFUNC fio_io_s *fio_http_io(fio_http_s *);

/** Macro helper for HTTP handle pub/sub subscriptions. */
#define fio_http_subscribe(h, ...)                                             \
  fio_pubsub_subscribe(.io = fio_http_io(h), __VA_ARGS__)
/* *****************************************************************************
HTTP Server API
***************************************************************************** */

/* *****************************************************************************
HTTP Listen
***************************************************************************** */

/* a pointer safety type */
typedef struct fio_http_listener_s fio_http_listener_s;

/** Listens to HTTP / WebSockets / SSE connections on `url`. */
SFUNC fio_http_listener_s *fio_http_listen(const char *url,
                                           fio_http_settings_s settings);

/** Listens to HTTP / WebSockets / SSE connections on `url`. */
#define fio_http_listen(url, ...)                                              \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})

/** Returns the a pointer to the HTTP settings associated with the listener. */
SFUNC fio_http_settings_s *fio_http_listener_settings(fio_http_listener_s *l);

/* *****************************************************************************
HTTP Routing – prefix matching
***************************************************************************** */

/**
 * Adds a route prefix to the HTTP handler.
 *
 * Order of route settings is irrelevant (unless overwriting an existing route).
 *
 * Matching is performed as a best-prefix match. i.e.:
 *
 * - All paths match the route `"/"` (the default prefix).
 *
 * - The route `"/user"` will match `"/user"` and all `"/user/..."` paths but
 *   not `"/user..."`
 *
 * - Setting `"/user/new"` as well as `"/user"` (in whatever order) will route
 *   `"/user/new"` and `"/user/new/..."` to `"/user/new"`. Otherwise, the
 *   `"/user"` route will continue to behave the same.
 *
 * Note: the following properties are inherited (if missing) from the
 * default HTTP settings used to create the listener: `udata`, `on_finish`,
 * `on_stop`, `on_authenticate_sse`, `on_authenticate_websocket`,
 * `max_header_size`, `max_line_len`, `max_body_size`, `ws_max_msg_size`,
 * `timeout`, `ws_timeout`, `sse_timeout`, `log`, `compress_static` and
 * `public_folder`.
 *
 * Note: TLS options are ignored.
 *
 * Note: only `on_http`, `on_finish`, `udata`, the authentication
 * callbacks, `public_folder`, `max_age` and `compress_static` are
 * effective per route. All other per-route values are stored but unused -
 * upgraded connection callbacks (`on_open`, `on_message`, etc.), `queue`,
 * `log`, limits, timeouts, `compress_dynamic` and `compress_ws` are
 * connection-global and are read from the listener's root settings.
 * */
SFUNC int fio_http_route(fio_http_listener_s *listener,
                         const char *url,
                         fio_http_settings_s settings);
/**
 * Adds a route prefix to the HTTP handler.
 *
 * Order of route settings is irrelevant (unless overwriting an existing route).
 *
 * Matching is performed as a best-prefix match. i.e.:
 *
 * - All paths match the route `"/"` (the default prefix).
 *
 * - The route `"/user"` will match `"/user"` and all `"/user/..."` paths but
 *   not `"/user..."`
 *
 * - Setting `"/user/new"` as well as `"/user"` (in whatever order) will route
 *   `"/user/new"` and `"/user/new/..."` to `"/user/new"`. Otherwise, the
 *   `"/user"` route will continue to behave the same.
 *
 * Note: the following properties are inherited (if missing) from the
 * default HTTP settings used to create the listener: `udata`, `on_finish`,
 * `on_stop`, `on_authenticate_sse`, `on_authenticate_websocket`,
 * `max_header_size`, `max_line_len`, `max_body_size`, `ws_max_msg_size`,
 * `timeout`, `ws_timeout`, `sse_timeout`, `log`, `compress_static` and
 * `public_folder`.
 *
 * Note: TLS options are ignored.
 *
 * Note: only `on_http`, `on_finish`, `udata`, the authentication
 * callbacks, `public_folder`, `max_age` and `compress_static` are
 * effective per route. All other per-route values are stored but unused -
 * upgraded connection callbacks (`on_open`, `on_message`, etc.), `queue`,
 * `log`, limits, timeouts, `compress_dynamic` and `compress_ws` are
 * connection-global and are read from the listener's root settings.
 * */
#define fio_http_route(listener, url, ...)                                     \
  fio_http_route(listener, url, (fio_http_settings_s){__VA_ARGS__})

/** Returns a link to the settings matching `url`, as set by `fio_http_route` */
SFUNC fio_http_settings_s *fio_http_route_settings(fio_http_listener_s *l,
                                                   const char *url);

/* *****************************************************************************
HTTP Routing – CRUD
***************************************************************************** */

typedef enum {
  FIO_HTTP_RESOURCE_NONE,
  FIO_HTTP_RESOURCE_INDEX,
  FIO_HTTP_RESOURCE_SHOW,
  FIO_HTTP_RESOURCE_NEW,
  FIO_HTTP_RESOURCE_EDIT,
  FIO_HTTP_RESOURCE_CREATE,
  FIO_HTTP_RESOURCE_UPDATE,
  FIO_HTTP_RESOURCE_DELETE,
  FIO_HTTP_RESOURCE_QUERY,
} fio_http_resource_action_e;

/** returns expected action or `FIO_HTTP_RESOURCE_NONE` on error. */
FIO_IFUNC fio_http_resource_action_e fio_http_resource_action(fio_http_s *h);

/* *****************************************************************************
HTTP Client API
***************************************************************************** */

/* *****************************************************************************
HTTP Connect
***************************************************************************** */

/** Connects to HTTP / WebSockets / SSE connections on `url`. */
SFUNC fio_io_s *fio_http_connect(const char *url,
                                 fio_http_s *h,
                                 fio_http_settings_s settings);

/** Connects to HTTP / WebSockets / SSE connections on `url`. */
#define fio_http_connect(url, h, ...)                                          \
  fio_http_connect(url, h, (fio_http_settings_s){__VA_ARGS__})

/**
 * Connects to a WebSocket server on `url`.
 *
 * A convenience wrapper around `fio_http_connect` that ensures a `ws://` or
 * `wss://` scheme is used in the URL (`http://` becomes `ws://`, `https://`
 * becomes `wss://`, a missing scheme defaults to `ws://`).
 *
 * The WebSocket upgrade request / response is handled automatically by the
 * underlying `fio_http_connect`: on acceptance (101) the connection
 * switches to the WebSocket callbacks (`on_open` / `on_message` /
 * `on_close`); on rejection the response is routed to `settings.on_http`.
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

/** Create a new fio_http_s handle. */
SFUNC fio_http_s *fio_http_new(void);

/** Creates a copy of an existing handle, copying only its request data. */
SFUNC fio_http_s *fio_http_new_copy_request(fio_http_s *old);

/** Reduces an fio_http_s handle's reference count or frees it. */
SFUNC void fio_http_free(fio_http_s *);

/** Increases an fio_http_s handle's reference count. */
SFUNC fio_http_s *fio_http_dup(fio_http_s *);

/** Destroyed the HTTP handle object, freeing all allocated resources. */
SFUNC fio_http_s *fio_http_destroy(fio_http_s *h);

/** Collects an updated timestamp for logging purposes. */
SFUNC void fio_http_start_time_set(fio_http_s *);

/** Clears any response data. */
SFUNC fio_http_s *fio_http_clear_response(fio_http_s *h, bool clear_body);

/* *****************************************************************************
Opaque User and Controller Data
***************************************************************************** */

/** Gets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata(fio_http_s *);

/** Sets the opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata_set(fio_http_s *, void *);

/** Gets the second opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata2(fio_http_s *);

/** Sets a second opaque user pointer associated with the HTTP handle. */
FIO_IFUNC void *fio_http_udata2_set(fio_http_s *, void *);

/** Gets the HTTP Controller associated with the HTTP handle. */
FIO_IFUNC fio_http_controller_s *fio_http_controller(fio_http_s *h);

/** Gets the HTTP Controller associated with the HTTP handle. */
FIO_IFUNC fio_http_controller_s *fio_http_controller_set(
    fio_http_s *h,
    fio_http_controller_s *controller);

/** Returns the existing controller data (`void *` pointer). */
FIO_IFUNC void *fio_http_cdata(fio_http_s *h);

/** Sets a new controller data (`void *` pointer). */
FIO_IFUNC void *fio_http_cdata_set(fio_http_s *h, void *cdata);

/* *****************************************************************************
Data associated with the Request (usually set by the HTTP protocol)
***************************************************************************** */

/** Gets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status(fio_http_s *);

/** Sets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status_set(fio_http_s *, size_t status);

/** Gets the method information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_method(fio_http_s *);

/** Sets the method information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_method_set(fio_http_s *, fio_str_info_s);

/** Gets the original / first path associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_opath(fio_http_s *);

/** Sets the original / first path associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_opath_set(fio_http_s *, fio_str_info_s);

/** Gets the path information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_path(fio_http_s *);

/** Sets the path information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_path_set(fio_http_s *, fio_str_info_s);

/** Gets the query information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_query(fio_http_s *);

/** Sets the query information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_query_set(fio_http_s *, fio_str_info_s);

/** Gets the version information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_version(fio_http_s *);

/** Sets the version information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_version_set(fio_http_s *, fio_str_info_s);

/** Gets the received_at timestamp (ms) associated with the HTTP handle. */
SFUNC int64_t fio_http_received_at(fio_http_s *);

/** Sets the received_at timestamp (ms) associated with the HTTP handle. */
SFUNC int64_t fio_http_received_at_set(fio_http_s *, int64_t);

/**
 * Gets the header information associated with the HTTP handle.
 *
 * Since more than a single value may be associated with a header name, the
 * index may be used to collect subsequent values.
 *
 * An empty value is returned if no header value is available (or index is
 * exceeded).
 */
SFUNC fio_str_info_s fio_http_request_header(fio_http_s *,
                                             fio_str_info_s name,
                                             size_t index);

/**
 * Returns the number of headers named `name` that were received.
 *
 * If `name` buffer is `NULL`, returns the number of unique headers (not the
 * number of unique values).
 */
SFUNC size_t fio_http_request_header_count(fio_http_s *, fio_str_info_s name);

/** Sets the header information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_request_header_set(fio_http_s *,
                                                 fio_str_info_s name,
                                                 fio_str_info_s value);

/** Sets the header information associated with the HTTP handle. */
SFUNC fio_str_info_s
fio_http_request_header_set_if_missing(fio_http_s *,
                                       fio_str_info_s name,
                                       fio_str_info_s value);

/** Adds to the header information associated with the HTTP handle. */
SFUNC fio_str_info_s fio_http_request_header_add(fio_http_s *,
                                                 fio_str_info_s name,
                                                 fio_str_info_s value);

/**
 * Iterates through all request headers (except cookies!).
 *
 * A non-zero return will stop iteration.
 *
 * Returns the number of iterations performed. If `callback` is `NULL`, returns
 * the number of headers available (multi-value headers are counted as 1).
 * */
SFUNC size_t fio_http_request_header_each(fio_http_s *,
                                          int (*callback)(fio_http_s *,
                                                          fio_str_info_s name,
                                                          fio_str_info_s value,
                                                          void *udata),
                                          void *udata);

/** Gets the body (payload) length associated with the HTTP handle. */
SFUNC size_t fio_http_body_length(fio_http_s *);

/**
 * Adjusts the body's reading position. Negative values start at the end.
 *
 * If `pos == SSIZE_MAX`, returns `fio_http_body_pos`.
 */
SFUNC size_t fio_http_body_seek(fio_http_s *, ssize_t pos);

/** Returns the body's reading position. */
SFUNC size_t fio_http_body_pos(fio_http_s *h);

/** Reads up to `length` of data from the body, returns nothing on EOF. */
SFUNC fio_str_info_s fio_http_body_read(fio_http_s *, size_t length);

/**
 * Reads from the body until finding `token`, reaching `limit` or EOF.
 *
 * Note: `limit` is ignored if zero or if the remaining data is lower than
 * limit.
 */
SFUNC fio_str_info_s fio_http_body_read_until(fio_http_s *,
                                              char token,
                                              size_t limit);

/** Allocates a body (payload) of (at least) the `expected_length`. */
SFUNC void fio_http_body_expect(fio_http_s *, size_t expected_length);

/** Writes `data` to the body (payload) associated with the HTTP handle. */
SFUNC void fio_http_body_write(fio_http_s *, const void *data, size_t len);

/**
 * If the body is stored in a temporary file, returns the file's handle.
 *
 * Otherwise returns -1.
 */
SFUNC int fio_http_body_fd(fio_http_s *);

/* *****************************************************************************
Path Section Looping
***************************************************************************** */

/**
 * Loops over each section of the path, decrypting percent encoding as
 * necessary.
 *
 * The macro accepts the following:
 *
 * - `path`: the path string - accessible using fio_http_path(h).
 * - `pos` : the name of the variable to use for accessing the section.
 *
 * The variable `pos` is a `fio_buf_info_s`.
 *
 * **Note**: the macro will break if a path's section length is greater than
 *           (about) 4063 bytes.
 */
#define FIO_HTTP_PATH_EACH(path, pos)

/* *****************************************************************************
Cookies
***************************************************************************** */

/**
 * Possible values for the `same_site` property in the cookie settings.
 *
 * See: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
 */
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

/**
 * This is a helper for setting cookie data.
 *
 * This struct is used together with the `fio_http_cookie_set` macro. i.e.:
 *
 *       fio_http_set_cookie(h,
 *                      .name = FIO_STR_INFO1("my_cookie"),
 *                      .value = FIO_STR_INFO1("data"));
 *
 */
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

/**
 * Sets a response cookie.
 *
 * Returns -1 on error and 0 on success.
 *
 * Note: Long cookie names and long cookie values will be considered a security
 * violation and an error will be returned. Many browsers and proxies impose
 * limits on headers and cookies, cookies often limited to 4Kb in total for both
 * name and value.
 */
SFUNC int fio_http_cookie_set(fio_http_s *h, fio_http_cookie_args_s);

/** Named arguments helper. See fio_http_cookie_args_s for details. */
#define fio_http_cookie_set(http___handle, ...)                                \
  fio_http_cookie_set((http___handle), (fio_http_cookie_args_s){__VA_ARGS__})

/** Returns a cookie value (either received of newly set), if any. */
SFUNC fio_str_info_s fio_http_cookie(fio_http_s *,
                                     const char *name,
                                     size_t name_len);

/** Iterates through all cookies. A non-zero return will stop iteration. */
SFUNC size_t fio_http_cookie_each(fio_http_s *,
                                  int (*callback)(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value,
                                                  void *udata),
                                  void *udata);

/**
 * Iterates through all response set cookies.
 *
 * A non-zero return value from the callback will stop iteration.
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

/** Returns true if no HTTP headers / data was sent (a clean slate). */
SFUNC int fio_http_is_clean(fio_http_s *);

/** Returns true if the HTTP handle's response was sent. */
SFUNC int fio_http_is_finished(fio_http_s *);

/** Returns true if the HTTP handle's response is streaming. */
SFUNC int fio_http_is_streaming(fio_http_s *);

/** Returns true if the HTTP connection was (or should have been) upgraded. */
SFUNC int fio_http_is_upgraded(fio_http_s *h);

/** Returns true if the HTTP handle refers to a WebSocket connection. */
SFUNC int fio_http_is_websocket(fio_http_s *);

/** Returns true if the HTTP handle refers to an EventSource connection. */
SFUNC int fio_http_is_sse(fio_http_s *);

/** Returns true if handle is in the process of freeing itself. */
SFUNC int fio_http_is_freeing(fio_http_s *);

/**
 * Gets the header information associated with the HTTP handle.
 *
 * Since more than a single value may be associated with a header name, the
 * index may be used to collect subsequent values.
 *
 * An empty value is returned if no header value is available (or index is
 * exceeded).
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
SFUNC fio_str_info_s fio_http_response_header(fio_http_s *,
                                              fio_str_info_s name,
                                              size_t index);
/**
 * Returns the number of headers named `name` in the response.
 *
 * If `name` buffer is `NULL`, returns the number of unique headers (not the
 * number of unique values).
 */
SFUNC size_t fio_http_response_header_count(fio_http_s *, fio_str_info_s name);

/**
 * Sets the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
SFUNC fio_str_info_s fio_http_response_header_set(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);
/**
 * Sets the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
SFUNC fio_str_info_s
fio_http_response_header_set_if_missing(fio_http_s *,
                                        fio_str_info_s name,
                                        fio_str_info_s value);

/**
 * Adds to the header information associated with the HTTP handle.
 *
 * If the response headers were already sent, the returned value is always
 * empty.
 */
SFUNC fio_str_info_s fio_http_response_header_add(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value);

/**
 * Iterates through all response headers (except cookies!).
 *
 * A non-zero return will stop iteration.
 * */
SFUNC size_t fio_http_response_header_each(fio_http_s *,
                                           int (*callback)(fio_http_s *,
                                                           fio_str_info_s name,
                                                           fio_str_info_s value,
                                                           void *udata),
                                           void *udata);

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
  /** If the data is a buffer / a file - should it be copied? */
  int copy;
  /**
   * If `finish` is set, this data marks the end of the response.
   *
   * Otherwise the response will stream the data.
   */
  int finish;
} fio_http_write_args_s;

/**
 * Writes `data` to the response body associated with the HTTP handle after
 * sending all headers (no further headers may be sent).
 */
SFUNC void fio_http_write(fio_http_s *, fio_http_write_args_s args);

/** Named arguments helper. See fio_http_write and fio_http_write_args_s. */
#define fio_http_write(http_handle, ...)                                       \
  fio_http_write(http_handle, (fio_http_write_args_s){__VA_ARGS__})
#define fio_http_finish(http_handle) fio_http_write(http_handle, .finish = 1)

/** Closes a persistent HTTP connection (i.e., if upgraded). */
SFUNC void fio_http_close(fio_http_s *h);

/* *****************************************************************************
MIME File Type Helpers - NOT thread safe!
***************************************************************************** */

/** Registers a Mime-Type to be associated with the file extension. */
SFUNC int fio_http_mimetype_register(char *file_ext,
                                     size_t file_ext_len,
                                     fio_str_info_s mime_type);

/** Finds the Mime-Type associated with the file extension (if registered). */
SFUNC fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len);

/* *****************************************************************************
HTTP Body Parsing Helpers
***************************************************************************** */

/**
 * HTTP body parser callbacks.
 *
 * All callbacks receive `udata` as first parameter.
 * Primitive callbacks return the created object as `void *`.
 * Container callbacks return a context for that container.
 */
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

  /**
   * Called when a file upload starts.
   *
   * Return context for this file (e.g., fd, stream, buffer).
   * Return NULL to skip this file (on_file_data/on_file_done won't be called).
   */
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

/** HTTP body parse result. */
typedef struct {
  /** Top-level parsed object (caller responsible for freeing). */
  void *result;
  /** Number of bytes consumed from body. */
  size_t consumed;
  /** Error code: 0 = success. */
  int err;
} fio_http_body_parse_result_s;

/**
 * Parses the HTTP request body, auto-detecting content type.
 *
 * Supports JSON, URL-encoded, and multipart/form-data bodies.
 * Calls the appropriate callbacks for each element found.
 *
 * @param h         The HTTP handle.
 * @param callbacks Parser callbacks (designed to be static const).
 * @param udata     User context passed to all callbacks.
 * @return          Parse result with top-level object and status.
 */
SFUNC fio_http_body_parse_result_s
fio_http_body_parse(fio_http_s *h,
                    const fio_http_body_parse_callbacks_s *callbacks,
                    void *udata);

/* *****************************************************************************
Header Parsing Helpers
***************************************************************************** */

/**
 * Copies all header data, from possibly an array of identical response headers,
 * resulting in a parsed format outputted to `buf_parsed`.
 *
 * Returns 0 on success or -1 on error (i.e., `buf_parsed.capa` wasn't enough
 * for the parsed output).
 *
 * Note that the parsed output isn't readable as a string, but is designed to
 * work with the `FIO_HTTP_PARSED_HEADER_EACH` and
 * `FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` property.
 *
 * See also `fio_http_response_header_parse`.
 */
SFUNC int fio_http_response_header_parse(fio_http_s *h,
                                         fio_str_info_s *buf_parsed,
                                         fio_str_info_s header_name);

/**
 * Copies all header data, from possibly an array of identical response headers,
 * resulting in a parsed format outputted to `buf_parsed`.
 *
 * Returns 0 on success or -1 on error (i.e., `buf_parsed.capa` wasn't enough
 * for the parsed output).
 *
 * Note that the parsed output isn't readable as a string, but is designed to
 * work with the `FIO_HTTP_PARSED_HEADER_EACH` and
 * `FIO_HTTP_HEADER_VALUE_EACH_PROPERTY` property.
 *
 * i.e.:
 *
 * ```c
 *  FIO_STR_INFO_TMP_VAR(buf, 1023); // tmp buffer for the parsed output
 *  fio_http_s *h = fio_http_new();  // using a mock HTTP handle
 *  fio_http_request_header_add(
 *      h,
 *      FIO_STR_INFO2("accept", 6),
 *      FIO_STR_INFO1("text/html, application/json;q=0.9; d=500, image/png"));
 *  fio_http_request_header_add(h,
 *                              FIO_STR_INFO2("accept", 6),
 *                              FIO_STR_INFO1("text/yaml"));
 *  FIO_ASSERT(  // in production do NOT assert, but route to error instead!
 *      !fio_http_request_header_parse(h, &buf, FIO_STR_INFO2("accept", 6)),
 *      "parse returned error!");
 *  FIO_HTTP_PARSED_HEADER_EACH(buf, value) {
 *    printf("* processing value (%zu bytes): %s\n", value.len, value.buf);
 *    FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(value, prop) {
 *      printf("* for value %s: (%zu,%zu bytes) %s = %s\n",
 *             value.buf,
 *             prop.name.len,
 *             prop.value.len,
 *             prop.name.buf,
 *             prop.value.buf);
 *    }
 *  }
 * ```
 */
SFUNC int fio_http_request_header_parse(fio_http_s *h,
                                        fio_str_info_s *buf_parsed,
                                        fio_str_info_s header_name);

/**
 * Parses header for multiple values and properties and iterates over all
 * values.
 *
 * This MACRO will allocate 2048 bytes on the stack for parsing the header
 * values and properties, if more space is necessary dig deeper.
 *
 * Use FIO_HTTP_HEADER_VALUE_EACH_PROPERTY to iterate over a value's properties.
 */
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
  FIO_HTTP_PARSED_HEADER_EACH(fio___buf__##value##__str, value) /* loop        \
                                                                 */

/** Iterated through the properties associated with a parsed header values. */
#define FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(/* fio_str_info_s   */ value,      \
                                            /* chosen var named */ property)

/** Used internally to iterate over a parsed header buffer. */
#define FIO_HTTP_PARSED_HEADER_EACH(/* fio_str_info_s   */ buf_parsed,         \
                                    /* chosen var named */ value)

/* *****************************************************************************
General Helpers
***************************************************************************** */

/** Sends the requested error message and finishes the response. */
SFUNC int fio_http_send_error_response(fio_http_s *h, size_t status);

/** Returns true (1) if the ETag response matches an if-none-match request. */
SFUNC int fio_http_etag_is_match(fio_http_s *h);

/**
 * Attempts to send a static file from the `root_folder` folder.
 *
 * `file_name` is URL-decoded and appended to `root_folder` (path traversal
 * is rejected). Folders resolve to their `index` file and missing
 * extensions are auto-completed (`.html`, `.htm`, `.txt`, `.md` -
 * `FIO_HTTP_STATIC_FILE_COMPLETION`). `OPTIONS` requests are refused (a
 * static file is not a valid `OPTIONS` response).
 *
 * Handles conditional requests (`ETag` / `If-None-Match` -> 304), single
 * `Range` requests (206 / 416, always served identity), and `HEAD`
 * requests; sets `Last-Modified`, `Accept-Ranges: bytes`, and (when
 * `max_age` is non-zero) `Cache-Control: max-age=...`.
 *
 * Pre-compressed variants (`file.br`, `file.zstd`, `file.gz`, `file.zip`,
 * in this preference order) are served when accepted by the client,
 * present on disk, and fresh; with `compress_static` enabled (the matching
 * route's settings, or the `FIO_HTTP_CFLAG_COMPRESS_STATIC` cflag on
 * detached handles), missing `.br` / `.gz` variants are also created on
 * demand.
 *
 * On success the response is complete and 0 is returned. On failure -1 is
 * returned and the application should handle the request itself.
 */
SFUNC int fio_http_static_file_response(fio_http_s *h,
                                        fio_str_info_s root_folder,
                                        fio_str_info_s file_name,
                                        size_t max_age);

/** Returns a human readable string related to the HTTP status number. */
SFUNC fio_str_info_s fio_http_status2str(size_t status);

/** Logs an HTTP (response) to STDOUT. */
SFUNC void fio_http_write_log(fio_http_s *h);

/**
 * Writes peer address to `dest` starting with the `forwarded` header, with a
 * fallback to actual socket address and a final fallback to `"[unknown]"`.
 *
 * If `unknown` is returned, the function returns -1. if `dest` capacity is too
 * small, the number of bytes required will be returned.
 *
 * If all goes well, this function returns 0.
 */
SFUNC int fio_http_from(fio_str_info_s *dest, const fio_http_s *h);

/* date/time string caching for HTTP date header */
SFUNC fio_str_info_s fio_http_date(uint64_t now_in_seconds);

/* date/time string caching for HTTP logging */
SFUNC fio_str_info_s fio_http_log_time(uint64_t now_in_seconds);
/* *****************************************************************************
The HTTP Controller
***************************************************************************** */

/**
 * The HTTP Controller manages all the callbacks required by the HTTP Handler in
 * order for HTTP responses and requests to be sent.
 */
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

/* *****************************************************************************
WebSocket / SSE Helpers
***************************************************************************** */

/** Returns non-zero if request headers ask for a WebSockets Upgrade.*/
SFUNC int fio_http_websocket_requested(fio_http_s *);

/** Returns non-zero if the response accepts a WebSocket upgrade request. */
SFUNC int fio_http_websocket_accepted(fio_http_s *h);

/** Sets response data to agree to a WebSockets Upgrade.*/
SFUNC void fio_http_upgrade_websocket(fio_http_s *);

/** Sets request data to request a WebSockets Upgrade.*/
SFUNC void fio_http_websocket_set_request(fio_http_s *);

/** Returns non-zero if request headers ask for an EventSource (SSE) Upgrade.*/
SFUNC int fio_http_sse_requested(fio_http_s *);

/** Returns non-zero if the response accepts an SSE request. */
SFUNC int fio_http_sse_accepted(fio_http_s *h);

/** Sets response data to agree to an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_upgrade_sse(fio_http_s *);

/** Sets request data to request an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_set_request(fio_http_s *);

/* *****************************************************************************
HTTP Handle State and Controller Flags
***************************************************************************** */

#define FIO_HTTP_STATE_STREAMING      1
#define FIO_HTTP_STATE_FINISHED       2
#define FIO_HTTP_STATE_UPGRADED       4
#define FIO_HTTP_STATE_WEBSOCKET      8
#define FIO_HTTP_STATE_SSE            16
#define FIO_HTTP_STATE_COOKIES_PARSED 32
#define FIO_HTTP_STATE_FREEING        64

/** Controller flags (cflags) for opt-in compression features. */
#define FIO_HTTP_CFLAG_COMPRESS_DYNAMIC 1
#define FIO_HTTP_CFLAG_COMPRESS_WS      2
#define FIO_HTTP_CFLAG_COMPRESS_STATIC  4

/* *****************************************************************************
WebSocket Helpers - HTTP Upgraded Connections
***************************************************************************** */

/** Writes a WebSocket message. Fails if connection wasn't upgraded yet. */
SFUNC int fio_http_websocket_write(fio_http_s *h,
                                   const void *buf,
                                   size_t len,
                                   uint8_t is_text);

/**
 * Sets a specific on_message callback for this connection.
 *
 * Returns -1 on error (i.e., upgrade still in negotiation).
 */
SFUNC int fio_http_on_message_set(fio_http_s *h,
                                  void (*on_message)(fio_http_s *,
                                                     fio_buf_info_s,
                                                     uint8_t));

/** Optional WebSocket subscription callback. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);
/** Optional WebSocket subscription callback - all messages are UTF-8 valid. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_pubsub_msg_s *msg);
/** Optional WebSocket subscription callback - messages may be non-UTF-8. */
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
  /** The message's `data` data (if any). */
  fio_buf_info_s data;
} fio_http_sse_write_args_s;

/** Writes an SSE message (UTF-8). Fails if connection wasn't upgraded yet. */
SFUNC int fio_http_sse_write(fio_http_s *h, fio_http_sse_write_args_s args);

/** Writes an SSE message (UTF-8). Fails if connection wasn't upgraded yet. */
#define fio_http_sse_write(h, ...)                                             \
  fio_http_sse_write((h), ((fio_http_sse_write_args_s){__VA_ARGS__}))

/** Optional EventSource subscription callback - messages MUST be UTF-8. */
SFUNC void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg);

/* *****************************************************************************
HTTP Settings Resolver
***************************************************************************** */

/** Returns the HTTP settings associated with the HTTP object, if any. */
SFUNC fio_http_settings_s *fio_http_settings(fio_http_s *);

/* *****************************************************************************
HTTP API Finish
***************************************************************************** */
#endif /* FIO_HTTP */
