/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                  HTTP Implementation for FIO_SERVER




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP) && !defined(H___FIO_HTTP___H) &&                         \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_HTTP___H
/* *****************************************************************************
HTTP Setting Defaults
***************************************************************************** */

#ifndef FIO_HTTP_DEFAULT_MAX_HEADER_SIZE
#define FIO_HTTP_DEFAULT_MAX_HEADER_SIZE 32768 /* (1UL << 15) */
#endif
#ifndef FIO_HTTP_DEFAULT_MAX_LINE_LEN
#define FIO_HTTP_DEFAULT_MAX_LINE_LEN 8192 /* (1UL << 13) */
#endif
#ifndef FIO_HTTP_DEFAULT_MAX_BODY_SIZE
#define FIO_HTTP_DEFAULT_MAX_BODY_SIZE 33554432 /* (1UL << 25) */
#endif
#ifndef FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE
#define FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE 262144 /* (1UL << 18) */
#endif
#ifndef FIO_HTTP_DEFAULT_TIMEOUT
#define FIO_HTTP_DEFAULT_TIMEOUT 50
#endif
#ifndef FIO_HTTP_DEFAULT_TIMEOUT_LONG
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
/* If true, logs longest WebSocket round-trips (using FIO_LOG_INFO). */
#define FIO_WEBSOCKET_STATS 0
#endif

/* *****************************************************************************
HTTP Listen
***************************************************************************** */
typedef struct fio_http_settings_s {
  /** Called before body uploads, when a client sends an `Expect` header. */
  void (*pre_http_body)(fio_http_s *h);
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

/** Listens to HTTP / WebSockets / SSE connections on `url`. */
SFUNC void *fio_http_listen(const char *url, fio_http_settings_s settings);

/** Listens to HTTP / WebSockets / SSE connections on `url`. */
#define fio_http_listen(url, ...)                                              \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})

/** Allows all clients to connect (bypasses authentication). */
SFUNC int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h);

/** Returns the IO object associated with the HTTP object (request only). */
SFUNC fio_s *fio_http_io(fio_http_s *);

/** Macro helper for HTTP handle pub/sub subscriptions. */
#define fio_http_subscribe(h, ...)                                             \
  fio_subscribe(.io = fio_http_io(h), __VA_ARGS__)

/** TODO: Connects to HTTP / WebSockets / SSE connections on `url`. */
SFUNC fio_s *fio_http_connect(const char *url,
                              fio_http_s *h,
                              fio_http_settings_s settings);

/** Connects to HTTP / WebSockets / SSE connections on `url`. */
#define fio_http_connect(url, h, ...)                                          \
  fio_http_connect(url, h, (fio_http_settings_s){__VA_ARGS__})

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
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_msg_s *msg);
/** Optional WebSocket subscription callback - all messages are UTF-8 valid. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_msg_s *msg);
/** Optional WebSocket subscription callback - messages may be non-UTF-8. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY(fio_msg_s *msg);

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
SFUNC void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_msg_s *msg);

/* *****************************************************************************
Module Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
HTTP Settings Validation
***************************************************************************** */

static void fio___http_default_on_http_request(fio_http_s *h) {
  fio_http_send_error_response(h, 404);
}
static void fio___http_default_noop(fio_http_s *h) { ((void)h); }
static int fio___http_default_authenticate(fio_http_s *h) {
  ((void)h);
  return -1;
}

// on_queue
static void fio___http_default_on_finish(struct fio_http_settings_s *settings) {
  ((void)settings);
}

static void fio___http_default_close(fio_http_s *h) {
  fio_close(fio_http_io(h));
}

/** Called when a WebSocket message is received. */
static void fio___http_default_on_message(fio_http_s *h,
                                          fio_buf_info_s msg,
                                          uint8_t is_text) {
  (void)h, (void)msg, (void)is_text;
}
/** Called when an EventSource event is received. */
static void fio___http_default_on_eventsource(fio_http_s *h,
                                              fio_buf_info_s id,
                                              fio_buf_info_s event,
                                              fio_buf_info_s data) {
  (void)h, (void)id, (void)event, (void)data;
}
/** Called when an EventSource reconnect event requests an ID. */
static void fio___http_default_on_eventsource_reconnect(fio_http_s *h,
                                                        fio_buf_info_s id) {
  (void)h, (void)id;
}

static void http_settings_validate(fio_http_settings_s *s, int is_client) {
  if (!s->pre_http_body)
    s->pre_http_body = fio___http_default_noop;

  if (!s->on_http)
    s->on_http = is_client ? fio___http_default_noop
                           : fio___http_default_on_http_request;
  if (!s->on_finish)
    s->on_finish = fio___http_default_on_finish;
  if (!s->on_authenticate_sse)
    s->on_authenticate_sse = is_client ? FIO_HTTP_AUTHENTICATE_ALLOW
                                       : fio___http_default_authenticate;
  if (!s->on_authenticate_websocket)
    s->on_authenticate_websocket = is_client ? FIO_HTTP_AUTHENTICATE_ALLOW
                                             : fio___http_default_authenticate;
  if (!s->on_open)
    s->on_open = fio___http_default_noop;
  if (!s->on_open)
    s->on_open = fio___http_default_noop;
  if (!s->on_message)
    s->on_message = fio___http_default_on_message;
  if (!s->on_eventsource)
    s->on_eventsource = fio___http_default_on_eventsource;
  if (!s->on_eventsource_reconnect)
    s->on_eventsource_reconnect = fio___http_default_on_eventsource_reconnect;
  if (!s->on_ready)
    s->on_ready = fio___http_default_noop;
  if (!s->on_shutdown)
    s->on_shutdown = fio___http_default_noop;
  if (!s->on_close)
    s->on_close = fio___http_default_noop;
  if (!s->max_header_size)
    s->max_header_size = FIO_HTTP_DEFAULT_MAX_HEADER_SIZE;
  if (!s->max_line_len)
    s->max_line_len = FIO_HTTP_DEFAULT_MAX_LINE_LEN;
  if (!s->max_body_size)
    s->max_body_size = FIO_HTTP_DEFAULT_MAX_BODY_SIZE;
  if (!s->ws_max_msg_size)
    s->ws_max_msg_size = FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE;
  if (!s->timeout)
    s->timeout = FIO_HTTP_DEFAULT_TIMEOUT;
  if (!s->ws_timeout)
    s->ws_timeout = FIO_HTTP_DEFAULT_TIMEOUT_LONG;

  if (s->max_header_size < s->max_line_len)
    s->max_header_size = s->max_line_len;

  if (s->public_folder.buf) {
    if (s->public_folder.len > 1 &&
        s->public_folder.buf[s->public_folder.len - 1] == '/' &&
        !(s->public_folder.len == 2 && s->public_folder.buf[0] == '~'))
      --s->public_folder.len;
    if (!fio_filename_is_folder(s->public_folder.buf)) {
      FIO_LOG_ERROR(
          "HTTP public folder is not a folder, setting ignored.\n\t%s",
          s->public_folder.buf);
      s->public_folder = ((fio_str_info_s){0});
    }
  }
}

/* *****************************************************************************
HTTP Protocols used by the HTTP module
***************************************************************************** */

typedef enum fio___http_protocol_selector_e {
  FIO___HTTP_PROTOCOL_ACCEPT = 0,
  FIO___HTTP_PROTOCOL_HTTP1,
  FIO___HTTP_PROTOCOL_HTTP2,
  FIO___HTTP_PROTOCOL_WS,
  FIO___HTTP_PROTOCOL_SSE,
  FIO___HTTP_PROTOCOL_NONE
} fio___http_protocol_selector_e;

/** Returns a facil.io protocol object with the proper protocol callbacks. */
FIO_IFUNC fio_protocol_s fio___http_protocol_get(fio___http_protocol_selector_e,
                                                 int is_client);
/** Returns an http controller object with the proper protocol callbacks. */
FIO_IFUNC fio_http_controller_s
fio___http_controller_get(fio___http_protocol_selector_e, int is_client);

/* *****************************************************************************
HTTP Protocol Container (vtable + settings storage)
***************************************************************************** */
#define FIO___RECURSIVE_INCLUDE 1

typedef struct {
  fio_http_settings_s settings;
  void (*on_http_callback)(void *, void *);
  fio_queue_s *queue;
  struct {
    fio_protocol_s protocol;
    fio_http_controller_s controller;
  } state[FIO___HTTP_PROTOCOL_NONE + 1];
  char public_folder_buf[];
} fio___http_protocol_s;
#include FIO_INCLUDE_FILE

#define FIO_REF_NAME             fio___http_protocol
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
    if (o.settings.tls)                                                        \
      fio_tls_free(o.settings.tls);                                            \
    if (o.settings.on_finish)                                                  \
      o.settings.on_finish(&o.settings);                                       \
  } while (0)
#include FIO_INCLUDE_FILE

/* *****************************************************************************
HTTP Connection Container
***************************************************************************** */

struct fio___http_connection_http_s {
  void (*on_http_callback)(void *, void *);
  void (*on_http)(fio_http_s *h);
  fio_http1_parser_s parser;
  uint32_t max_header;
};
struct fio___http_connection_ws_s {
  void (*on_message)(fio_http_s *h, fio_buf_info_s msg, uint8_t is_text);
  fio_websocket_parser_s parser;
  char *msg;
  uint16_t code;
};
struct fio___http_connection_sse_s {
  void (*on_message)(fio_http_s *h, fio_buf_info_s msg, uint8_t is_text);
  void (*on_ready)(fio_http_s *h);
  void (*on_shutdown)(fio_http_s *h);
  void (*on_close)(fio_http_s *h);
};

/** Connection objects for managing HTTP / WebSocket connection state. */
typedef struct {
  fio_s *io;
  fio_http_s *h;
  fio_http_settings_s *settings;
  fio_queue_s *queue;
  void *udata;
  union {
    struct fio___http_connection_http_s http;
    struct fio___http_connection_ws_s ws;
    struct fio___http_connection_sse_s sse;
  } state;
  uint32_t len;
  uint32_t capa;
  uint8_t log;
  uint8_t suspend;
  uint8_t is_client;
  char buf[];
} fio___http_connection_s;

#define FIO_REF_NAME             fio___http_connection
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
    fio___http_protocol_free(                                                  \
        FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, o.settings));      \
  } while (0)
#include FIO_INCLUDE_FILE

#undef FIO___RECURSIVE_INCLUDE

/* *****************************************************************************
HTTP Request handling / handling
***************************************************************************** */

FIO_SFUNC void fio___http_perform_user_callback(void *cb_, void *h_) {
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.ptr = cb_};
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (FIO_LIKELY(fio_srv_is_open(c->io)))
    cb.fn(h);
  fio_http_free(h);
}

FIO_SFUNC void fio___http_perform_user_upgrade_callback_websockets(void *cb_,
                                                                   void *h_) {
  union {
    int (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.ptr = cb_};
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  struct fio___http_connection_http_s old = c->state.http;
  if (cb.fn(h))
    goto refuse_upgrade;
  if (c->h) /* request after WebSocket Upgrade? an attack vector? */
    goto refuse_upgrade;
#if HAVE_ZLIB && 0           /* TODO: logs and fix extension handling logic */
  FIO_HTTP_HEADER_EACH_VALUE(/* TODO: setup WebSocket extension */
                             h,
                             1,
                             FIO_STR_INFO2((char *)"sec-websocket-extensions",
                                           24),
                             val) {
    FIO_LOG_DDEBUG2("WebSocket extension requested: %.*s",
                    (int)val.len,
                    val.buf);
    if (!FIO_STR_INFO_IS_EQ(val,
                            FIO_STR_INFO2((char *)"permessage-deflate", 18)))
      continue;
    size_t client_bits = 0, server_bits = 0;
    FIO_HTTP_HEADER_VALUE_EACH_PROPERTY(val, p) {
      FIO_LOG_DDEBUG2("\t %.*s: %.*s",
                      (int)p.name.len,
                      p.name.buf,
                      (int)p.value.len,
                      p.value.buf);
      if (FIO_STR_INFO_IS_EQ(p.name,
                             FIO_STR_INFO2((char *)"client_max_window_bits",
                                           22))) { /* used by chrome */
        char *iptr = p.value.buf;
        client_bits = iptr ? fio_atol10u(&iptr) : 0;
        if (client_bits < 8 || client_bits > 15)
          client_bits = (size_t)-1;
      }
      if (FIO_STR_INFO_IS_EQ(
              p.name,
              FIO_STR_INFO2((char *)"server_max_window_bits", 22))) {
        char *iptr = p.value.buf;
        server_bits = iptr ? fio_atol10u(&iptr) : 0;
        if (server_bits < 8 || server_bits > 15)
          server_bits = (size_t)-1;
      }
    }
    if (client_bits)
      ; /* TODO */
    if (server_bits)
      ; /* TODO */
    break;
  } /* HAVE_ZLIB */
#endif
  fio_http_upgrade_websockets(h);
  return;

refuse_upgrade:
  c->state.http = old;
  fio_http_send_error_response(h, 403);
  fio_http_free(h);
}

FIO_SFUNC void fio___http_perform_user_upgrade_callback_sse(void *cb_,
                                                            void *h_) {
  union {
    int (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.ptr = cb_};
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c;
  if (cb.fn(h))
    goto refuse_upgrade;
  c = (fio___http_connection_s *)fio_http_cdata(h);
  if (c->h) /* request after eventsource? an attack vector? */
    goto refuse_upgrade;
  fio_http_upgrade_sse(h);
  return;

refuse_upgrade:
  fio_http_send_error_response(h, 403);
  fio_http_free(h);
}

FIO_IFUNC int fio___http_on_http_test4upgrade(fio_http_s *h,
                                              fio___http_connection_s *c) {
  union {
    int (*fn)(fio_http_s *);
    void *ptr;
  } cb;
  if (fio_http_websockets_requested(h))
    goto websocket_requested;
  if (fio_http_sse_requested(h))
    goto sse_requested;
  return 0;
websocket_requested:
  cb.fn = c->settings->on_authenticate_websocket;
  fio_queue_push(c->queue,
                 fio___http_perform_user_upgrade_callback_websockets,
                 cb.ptr,
                 (void *)h);
  return -1;
sse_requested:
  cb.fn = c->settings->on_authenticate_sse;
  fio_queue_push(c->queue,
                 fio___http_perform_user_upgrade_callback_sse,
                 cb.ptr,
                 (void *)h);
  return -1;
#if 0
http2_requested:
  // Connection: Upgrade, HTTP2-Settings
  // Upgrade: h2c
  // HTTP2-Settings: <base64url encoding of HTTP/2 SETTINGS payload>
  return 0; /* allowed to ignore upgrade request */
#endif
}

FIO_SFUNC void fio___http_on_http_direct(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  fio_http_status_set(h, 200);
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (fio___http_on_http_test4upgrade(h, c))
    return;
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.fn = c->state.http.on_http};
  fio_queue_push(c->queue, fio___http_perform_user_callback, cb.ptr, (void *)h);
  (void)ignr;
}

FIO_SFUNC void fio___http_on_http_with_public_folder(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  fio_http_status_set(h, 200);
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (fio___http_on_http_test4upgrade(h, c))
    return;
  if ((fio_http_method(h).len != 4 || (fio_buf2u32u(fio_http_method(h).buf) |
                                       0x20202020UL) != fio_buf2u32u("post")) &&
      !fio_http_static_file_response(h,
                                     c->settings->public_folder,
                                     fio_http_path(h),
                                     c->settings->max_age)) {
    fio_http_free(h);
    return;
  }
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.fn = c->state.http.on_http};
  fio_queue_push(c->queue, fio___http_perform_user_callback, cb.ptr, (void *)h);
  (void)ignr;
}

FIO_SFUNC void fio___http_on_http_client(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (fio___http_on_http_test4upgrade(h, c))
    return;
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.fn = c->state.http.on_http};
  fio_queue_push(c->queue, fio___http_perform_user_callback, cb.ptr, (void *)h);
  (void)ignr;
}

/* *****************************************************************************
ALPN Helpers
***************************************************************************** */

FIO_SFUNC void fio___http_on_select_h1(fio_s *io) {
  FIO_LOG_DDEBUG2("TLS ALPN HTTP/1.1 selected for %p", io);
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  fio_protocol_set(
      io,
      &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
            ->state[FIO___HTTP_PROTOCOL_HTTP1]
            .protocol));
}
FIO_SFUNC void fio___http_on_select_h2(fio_s *io) {
  FIO_LOG_ERROR("TLS ALPN HTTP/2 not supported for %p", io);
  (void)io;
}

/* *****************************************************************************
HTTP Listen
***************************************************************************** */

static void fio___http_listen_on_finished(fio_protocol_s *p, void *u) {
  (void)u;
  fio___http_protocol_free(
      FIO_PTR_FROM_FIELD(fio___http_protocol_s,
                         state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                         p));
}

void fio_http_listen___(void); /* IDE marker */
SFUNC void *fio_http_listen FIO_NOOP(const char *url, fio_http_settings_s s) {
  http_settings_validate(&s, 0);
  fio___http_protocol_s *p = fio___http_protocol_new(s.public_folder.len + 1);
  int should_free_tls = !s.tls;
  FIO_ASSERT_ALLOC(p);
  for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE + 1; ++i) {
    p->state[i].protocol =
        fio___http_protocol_get((fio___http_protocol_selector_e)i, 0);
    p->state[i].controller =
        fio___http_controller_get((fio___http_protocol_selector_e)i, 0);
  }
  for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE; ++i)
    p->state[i].protocol.timeout = s.ws_timeout * 1000;
  p->state[FIO___HTTP_PROTOCOL_ACCEPT].protocol.timeout = s.timeout * 1000;
  p->state[FIO___HTTP_PROTOCOL_HTTP1].protocol.timeout = s.timeout * 1000;
  p->state[FIO___HTTP_PROTOCOL_NONE].protocol.timeout = s.timeout * 1000;
  if (url) {
    fio_url_s u = fio_url_parse(url, strlen(url));
    s.tls = fio_tls_from_url(s.tls, u);
    if (s.tls) {
      s.tls = fio_tls_dup(s.tls);
      /* fio_tls_alpn_add(s.tls, "h2", fio___http_on_select_h2); // not yet */
      // fio_tls_alpn_add(s.tls, "http/1.1", fio___http_on_select_h1);
      fio_io_functions_s tmp_fn = fio_tls_default_io_functions(NULL);
      if (!s.tls_io_func)
        s.tls_io_func = &tmp_fn;
      for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE + 1; ++i)
        p->state[i].protocol.io_functions = *s.tls_io_func;
    }
    if (should_free_tls)
      fio_tls_free(s.tls);
  }
  p->settings = s;
  p->on_http_callback = (p->settings.public_folder.len)
                            ? fio___http_on_http_with_public_folder
                            : fio___http_on_http_direct;
  p->settings.public_folder.buf = p->public_folder_buf;
  p->queue = p->settings.queue ? p->settings.queue->q : fio_srv_queue();
  if (s.public_folder.len)
    FIO_MEMCPY(p->public_folder_buf, s.public_folder.buf, s.public_folder.len);
  void *listener =
      fio_srv_listen(.url = url,
                     .protocol = &p->state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                     .tls = s.tls,
                     // .on_open = fio___http_on_open,
                     .on_finish = fio___http_listen_on_finished,
                     .queue_for_accept = p->queue ? p->queue : NULL);
  return listener;
}

/* *****************************************************************************
HTTP Connect
***************************************************************************** */

void fio___http_connect_on_failed(void *udata);

void fio_http_connect___(void); /* IDE Marker */
/** Connects to HTTP / WebSockets / SSE connections on `url`. */
SFUNC fio_s *fio_http_connect FIO_NOOP(const char *url,
                                       fio_http_s *h,
                                       fio_http_settings_s s) {
  http_settings_validate(&s, 1);
  fio_url_s u = (fio_url_s){0};
  if (url)
    u = fio_url_parse(url, strlen(url));

  if (!h)
    h = fio_http_new();
  if (!fio_http_path(h).len)
    fio_http_path_set(h,
                      u.path.len ? FIO_BUF2STR_INFO(u.path)
                                 : FIO_STR_INFO2("/", 1));
  if (!fio_http_query(h).len && u.query.len)
    fio_http_query_set(h, FIO_BUF2STR_INFO(u.query));
  if (!fio_http_method(h).len)
    fio_http_method_set(h, FIO_STR_INFO2("GET", 3));
  if (u.host.len)
    fio_http_request_header_set_if_missing(h,
                                           FIO_STR_INFO2("host", 4),
                                           FIO_BUF2STR_INFO(u.host));
  /* test for ws:// or wss:// - WebSocket scheme */
  if ((u.scheme.len == 2 ||
       (u.scheme.len == 3 && ((u.scheme.buf[2] | 0x20) == 's'))) &&
      (fio_buf2u16u(u.scheme.buf) | 0x2020) == fio_buf2u16u("ws"))
    fio_http_websockets_set_request(h);
  /* test for sse:// or sses:// - Server Sent Events scheme */
  else if ((u.scheme.len == 3 ||
            (u.scheme.len == 4 && ((u.scheme.buf[3] | 0x20) == 's'))) &&
           (fio_buf2u32u(u.scheme.buf) | fio_buf2u32u("\x20\x20\x20\xFF")) ==
               fio_buf2u32u("sse\xFF"))
    fio_http_sse_set_request(h);

  /* TODO: test for and attempt to re-use connection */
  if (fio_http_cdata(h)) {
  }

  fio___http_protocol_s *p = fio___http_protocol_new(u.host.len);
  int should_free_tls = !s.tls;
  FIO_ASSERT_ALLOC(p);
  FIO_MEMCPY(p->public_folder_buf, url, (u.host.buf + u.host.len) - url);
  for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE + 1; ++i) {
    p->state[i].protocol =
        fio___http_protocol_get((fio___http_protocol_selector_e)i, 1);
    p->state[i].controller =
        fio___http_controller_get((fio___http_protocol_selector_e)i, 1);
  }
  for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE; ++i)
    p->state[i].protocol.timeout = s.ws_timeout * 1000;
  p->state[FIO___HTTP_PROTOCOL_ACCEPT].protocol.timeout = s.timeout * 1000;
  p->state[FIO___HTTP_PROTOCOL_HTTP1].protocol.timeout = s.timeout * 1000;
  p->state[FIO___HTTP_PROTOCOL_NONE].protocol.timeout = s.timeout * 1000;

  s.tls = fio_tls_from_url(s.tls, u);
  if (s.tls) {
    s.tls = fio_tls_dup(s.tls);
    /* fio_tls_alpn_add(s.tls, "h2", fio___http_on_select_h2); // not yet */
    // fio_tls_alpn_add(s.tls, "http/1.1", fio___http_on_select_h1);
    fio_io_functions_s tmp_fn = fio_tls_default_io_functions(NULL);
    if (!s.tls_io_func)
      s.tls_io_func = &tmp_fn;
    for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE + 1; ++i)
      p->state[i].protocol.io_functions = *s.tls_io_func;
  }
  if (should_free_tls)
    fio_tls_free(s.tls);
  p->settings = s;
  p->settings.public_folder.buf = p->public_folder_buf;
  p->settings.public_folder.len = 0;
  p->settings.public_folder.buf[0] = 0;
  p->queue = p->settings.queue ? p->settings.queue->q : fio_srv_queue();
  p->on_http_callback = fio___http_on_http_client;
  fio___http_connection_s *c =
      fio___http_connection_new(p->settings.max_line_len);
  FIO_ASSERT_ALLOC(c);
  *c = (fio___http_connection_s){
      .io = NULL,
      .h = h,
      .settings = &(p->settings),
      .queue = p->queue,
      .udata = p->settings.udata,
      .state.http =
          {
              .on_http_callback = p->on_http_callback,
              .on_http = p->settings.on_http,
              .max_header = p->settings.max_header_size,
          },
      .capa = p->settings.max_line_len,
      .log = p->settings.log,
  };
  return fio_srv_connect(url,
                         .protocol =
                             &p->state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                         .on_failed = NULL,
                         .udata = c,
                         .tls = s.tls,
                         .timeout = s.timeout);
}

/* *****************************************************************************
HTTP/1.1 Request / Response Completed
***************************************************************************** */

/** called when either a request or a response was received. */
static void fio_http1_on_complete(void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_dup(c->io);
  fio_srv_suspend(c->io);
  fio_http_s *h = c->h;
  c->h = NULL;
  c->suspend = 1;
  fio_queue_push(fio_srv_queue(), c->state.http.on_http_callback, h);
}

/* *****************************************************************************
HTTP/1.1 Parser callbacks
***************************************************************************** */

FIO_IFUNC void fio___http_request_too_big(fio___http_connection_s *c) {
  fio_http_s *h = c->h;
  fio_dup(c->io);
  fio_srv_suspend(c->io);
  c->h = NULL;
  c->suspend = 1;
  fio_http_send_error_response(h, 413);
  fio_http_free(h);
}

FIO_IFUNC void fio_http1_attach_handle(fio___http_connection_s *c) {
  c->h = fio_http_new();
  FIO_ASSERT_ALLOC(c->h);
  fio_http_controller_set(
      c->h,
      &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings))
           ->state[FIO___HTTP_PROTOCOL_HTTP1]
           .controller);
  fio_http_udata_set(c->h, c->udata);
  fio_http_cdata_set(c->h, fio___http_connection_dup(c));
}

/** called when a request method is parsed. */
static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (c->h)
    return -1;
  fio_http1_attach_handle(c);
  fio_http_method_set(c->h, FIO_BUF2STR_INFO(method));
  return 0;
}
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (c->h) /* TODO! is this the way it goes, or do we have a request obj? */
    return -1;
  fio_http1_attach_handle(c);
  fio_http_status_set(c->h, istatus);
  return 0;
  (void)status;
}
/** called when a request URL is parsed. */
static int fio_http1_on_url(fio_buf_info_s url, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_url_s u = fio_url_parse(url.buf, url.len);
  if (!u.path.len || u.path.buf[0] != '/')
    return -1;
  fio_http_path_set(c->h, FIO_BUF2STR_INFO(u.path));
  if (u.query.len)
    fio_http_query_set(c->h, FIO_BUF2STR_INFO(u.query));
  if (u.host.len)
    (!(c->h) ? fio_http_request_header_set
             : fio_http_response_header_set)(c->h,
                                             FIO_STR_INFO1((char *)"host"),
                                             FIO_BUF2STR_INFO(u.host));
  return 0;
}
/** called when a the HTTP/1.x version is parsed. */
static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_http_version_set(c->h, FIO_BUF2STR_INFO(version));
  return 0;
}
/** called when a header is parsed. */
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (!c->h)
    return 0; /* ignore possible post-error response headers */
  (!fio_http_status(c->h)
       ? fio_http_request_header_add
       : fio_http_response_header_add)(c->h,
                                       FIO_BUF2STR_INFO(name),
                                       FIO_BUF2STR_INFO(value));
  return 0;
}
/** called when the special content-length header is parsed. */
static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_http_s *h = c->h;
  if (!h)
    return 0;
  if (content_length > c->settings->max_body_size)
    goto too_big;
  if (content_length)
    fio_http_body_expect(c->h, content_length);
#if FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER
  (!(h->status) ? fio_http_request_header_add
                : fio_http_response_header_add)(h,
                                                FIO_BUF2STR_INFO(name),
                                                FIO_BUF2STR_INFO(value));
#endif
  return 0;
too_big:
  fio___http_request_too_big(c);
  return 0; /* should we disconnect (return -1), or not? */
  (void)name, (void)value;
}
/** called when `Expect` arrives and may require a 100 continue response. */
static int fio_http1_on_expect(void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_http_s *h = c->h;
  if (!h)
    return 1;
  fio_dup(c->io);
  c->h = NULL;
  c->settings->pre_http_body(h);
  if (fio_http_status(h))
    goto response_sent;
  c->h = h;
  fio_undup(c->io);
  const fio_buf_info_s response =
      FIO_BUF_INFO1((char *)"HTTP/1.1 100 Continue\r\n\r\n");
  fio_write2(c->io, .buf = response.buf, .len = response.len, .copy = 0);
  return 0; /* TODO?: improve support for `expect` headers? */
response_sent:
  fio_http_free(h);
  return 1;
}

/** called when a body chunk is parsed. */
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (!c->h)
    return -1; /* close connection if a large payload is unstoppable */
  if (chunk.len + fio_http_body_length(c->h) > c->settings->max_body_size)
    goto too_big;
  fio_http_body_write(c->h, chunk.buf, chunk.len);
  return 0;
too_big:
  fio___http_request_too_big(c);
  return 0;
}

/* *****************************************************************************
HTTP/1.1 Accepting new connections (tests for special HTTP/2 pre-knowledge)
***************************************************************************** */

/** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___http_on_attach_accept(fio_s *io) {

  fio___http_protocol_s *p =
      FIO_PTR_FROM_FIELD(fio___http_protocol_s,
                         state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                         fio_protocol_get(io));
  fio___http_protocol_dup(p);
  const uint32_t capa = p->settings.max_line_len;
  fio___http_connection_s *c = fio___http_connection_new(capa);
  FIO_ASSERT_ALLOC(c);
  *c = (fio___http_connection_s){
      .settings = &(p->settings),
      .queue = p->queue,
      .udata = p->settings.udata,
      .io = io,
      .state.http =
          {
              .on_http_callback = p->on_http_callback,
              .on_http = p->settings.on_http,
              .max_header = p->settings.max_header_size,
          },
      .capa = capa,
      .log = p->settings.log,
  };
  fio_udata_set(io, (void *)c);
  FIO_LOG_DDEBUG2("(%d) HTTP accepted a new connection (%p)",
                  (int)fio_thread_getpid(),
                  c->io);
#if 0 /* skip pre-knowledge test? */
  fio_protocol_set(
      io,
      &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
            ->state[FIO___HTTP_PROTOCOL_HTTP1]
            .protocol));
#endif
}

/** Called when a data is available. */
FIO_SFUNC void fio___http1_accept_on_data(fio_s *io) {
  const fio_buf_info_s prior_knowledge = FIO_BUF_INFO2(
      (char *)"\x50\x52\x49\x20\x2a\x20\x48\x54\x54\x50\x2f\x32\x2e\x30"
              "\x0d\x0a\x0d\x0a\x53\x4d\x0d\x0a\x0d\x0a",
      24);
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  fio_protocol_s *phttp_new;
  size_t r = fio_read(io, c->buf + c->len, c->capa - c->len);
  if (!r) /* nothing happened */
    return;
  c->len = (uint32_t)r;
  if (prior_knowledge.buf[0] != c->buf[0] ||
      FIO_MEMCMP(
          prior_knowledge.buf,
          c->buf,
          (c->len > prior_knowledge.len ? prior_knowledge.len : c->len))) {
    /* no prior knowledge, switch to HTTP/1.1 */
    phttp_new =
        &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
              ->state[FIO___HTTP_PROTOCOL_HTTP1]
              .protocol);
    fio_protocol_set(io, phttp_new);
    return;
  }
  if (c->len < prior_knowledge.len) /* wait for more data */
    return;

  if (c->len > prior_knowledge.len)
    FIO_MEMMOVE(c->buf,
                c->buf + prior_knowledge.len,
                c->len - prior_knowledge.len);
  c->len -= prior_knowledge.len;
  phttp_new = &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
                    ->state[FIO___HTTP_PROTOCOL_HTTP2]
                    .protocol);

  fio_protocol_set(io, phttp_new);
}

FIO_SFUNC void fio___http_on_close(void *udata) {
  FIO_LOG_DDEBUG2("(%d) HTTP connection closed for %p",
                  (int)fio_thread_getpid(),
                  udata);
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  c->io = NULL;
  fio_http_free(c->h);
  fio___http_connection_free(c);
}

/* *****************************************************************************
HTTP/1.1 Protocol
***************************************************************************** */

FIO_SFUNC int fio___http1_process_data(fio_s *io, fio___http_connection_s *c) {
  (void)io, (void)c;
  size_t consumed = fio_http1_parse(&c->state.http.parser,
                                    FIO_BUF_INFO2(c->buf, c->len),
                                    (void *)c);
  if (!consumed)
    return -1;
  if (consumed == FIO_HTTP1_PARSER_ERROR)
    goto http1_error;
  c->len -= consumed;
  if (c->len)
    FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
  if (c->suspend)
    return -1;
  return 0;

http1_error:
  if (c->h) {
    fio_http_s *h = c->h;
    c->h = NULL;
    fio_dup(c->io);
    fio_http_send_error_response(h, 400);
    fio_http_free(h);
  }
  fio_close(io);
  return -1;
}

// /** Called when a data is available. */
FIO_SFUNC void fio___http1_on_data(fio_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  size_t r;
  for (;;) {
    if (c->capa == c->len)
      return;
    if (!(r = fio_read(io, c->buf + c->len, c->capa - c->len)))
      return;
    c->len += r;
    if (fio___http1_process_data(io, c))
      return;
  }
}

// /** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___http1_on_attach(fio_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  if (c->len)
    fio___http1_process_data(io, c);
  return;
}

/* *****************************************************************************
HTTP/1 Controller
***************************************************************************** */

/** called by the HTTP handle for each header. */
FIO_SFUNC int fio_http1___write_header_callback(fio_http_s *h,
                                                fio_str_info_s name,
                                                fio_str_info_s value,
                                                void *out_) {
  (void)h;
  /* manually copy, as this is an "all or nothing" copy (no truncation) */
  fio_str_info_s *out = (fio_str_info_s *)out_;
  return fio_string_write2(out,
                           FIO_STRING_REALLOC,
                           FIO_STRING_WRITE_STR2(name.buf, name.len),
                           FIO_STRING_WRITE_STR2(":", 1),
                           FIO_STRING_WRITE_STR2(value.buf, value.len),
                           FIO_STRING_WRITE_STR2("\r\n", 2));
}

/** Informs the controller that request / response headers must be sent. */
FIO_SFUNC void fio___http_controller_http1_send_headers(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c->io || !fio_srv_is_open(c->io))
    return;
  fio_str_info_s buf = FIO_STR_INFO2(NULL, 0);
  { /* write status string */
    fio_str_info_s ver = fio_http_version(h);
    fio_str_info_s status = fio_http_status2str(fio_http_status(h));
    if (ver.len > 15) {
      FIO_LOG_ERROR("HTTP/1.1 client version string too long!");
      ver = FIO_STR_INFO1((char *)"HTTP/1.1");
    }
    fio_string_write2(&buf,
                      FIO_STRING_REALLOC,
                      FIO_STRING_WRITE_STR2(ver.buf, ver.len),
                      FIO_STRING_WRITE_STR2(" ", 1),
                      FIO_STRING_WRITE_NUM(fio_http_status(h)),
                      FIO_STRING_WRITE_STR2(" ", 1),
                      FIO_STRING_WRITE_STR2(status.buf, status.len),
                      FIO_STRING_WRITE_STR2("\r\n", 2));
  }
  /* write headers */
  fio_http_response_header_each(h, fio_http1___write_header_callback, &buf);
  /* write cookies */
  fio_http_set_cookie_each(h, fio_http1___write_header_callback, &buf);
  /* add streaming headers? */
  if (fio_http_is_streaming(h))
    fio_string_write(&buf,
                     FIO_STRING_REALLOC,
                     "transfer-encoding: chunked\r\n",
                     28);
  fio_string_write(&buf, FIO_STRING_REALLOC, "\r\n", 2);
  /* send data (move memory ownership) */
  fio_write2(c->io,
             .buf = buf.buf,
             .len = buf.len,
             .copy = 0,
             .dealloc = FIO_STRING_FREE);
}
/** called by the HTTP handle for each body chunk (or to finish a response. */
FIO_SFUNC void fio___http_controller_http1_write_body(
    fio_http_s *h,
    fio_http_write_args_s args) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c->io || !fio_srv_is_open(c->io))
    goto no_write_err;
  if (fio_http_is_streaming(h))
    goto stream_chunk;
  fio_write2(c->io,
             .buf = (void *)args.buf,
             .len = args.len,
             .fd = args.fd,
             .offset = args.offset,
             .dealloc = args.dealloc,
             .copy = (uint8_t)args.copy);
  return;
stream_chunk:
  if (args.len) { /* print chunk header */
    char buf[24];
    fio_str_info_s i = FIO_STR_INFO3(buf, 0, 24);
    fio_string_write_hex(&i, NULL, args.len);
    fio_string_write(&i, NULL, "\r\n", 2);
    fio_write2(c->io, .buf = (void *)i.buf, .len = i.len, .copy = 1);
  } else {
    FIO_LOG_ERROR("HTTP1 streaming requires a correctly pre-determined "
                  "length per chunk.");
  }
  fio_write2(c->io,
             .buf = (void *)args.buf,
             .len = args.len,
             .fd = args.fd,
             .offset = args.offset,
             .dealloc = args.dealloc,
             .copy = (uint8_t)args.copy);
  /* print chunk trailer */
  {
    fio_buf_info_s trailer = FIO_BUF_INFO2((char *)"\r\n", 2);
    fio_write2(c->io, .buf = trailer.buf, .len = trailer.len, .copy = 1);
  }
  return;
no_write_err:
  if (args.buf) {
    if (args.dealloc)
      args.dealloc((void *)args.buf);
  } else if (args.fd != -1) {
    close(args.fd);
  }
}

FIO_SFUNC void fio___http_controller_http1_on_finish_task(void *c_,
                                                          void *upgraded) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  c->suspend = 0;
  if (upgraded)
    goto upgraded;
  if (fio_srv_is_open(c->io)) {
    /* TODO: test for connection:close header and h->status values */
    fio___http1_process_data(c->io, c);
  }
  if (!c->suspend)
    fio_srv_unsuspend(c->io);
  fio_undup(c->io);
  return;

upgraded:
  if (c->h || !fio_srv_is_open(c->io))
    goto something_is_wrong;
  c->h = (fio_http_s *)upgraded;
  {
    const size_t pr_i = fio_http_is_websocket(c->h) ? FIO___HTTP_PROTOCOL_WS
                                                    : FIO___HTTP_PROTOCOL_SSE;
    fio_http_controller_set(
        c->h,
        &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
              ->state[pr_i]
              .controller));
    fio_protocol_set(
        c->io,
        &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
              ->state[pr_i]
              .protocol));
    if (pr_i == FIO___HTTP_PROTOCOL_SSE) {
      fio_str_info_s last_id =
          fio_http_request_header(c->h,
                                  FIO_STR_INFO2((char *)"last-event-id", 13),
                                  0);
      if (last_id.buf)
        c->settings->on_eventsource_reconnect(c->h, FIO_STR2BUF_INFO(last_id));
    }
  }
  fio_srv_unsuspend(c->io);
  fio_undup(c->io);
  return;

something_is_wrong:
  fio_protocol_set(c->io, NULL); /* make zombie, timeout will clear it. */
  fio_undup(c->io);
  fio___http_connection_free(c); /* free HTTP connection element */
}

/** called once a request / response had finished */
FIO_SFUNC void fio___http_controller_http1_on_finish(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (fio_http_is_streaming(h))
    fio_write2(c->io, .buf = (char *)"0\r\n\r\n", .len = 5, .copy = 1);
  if (c->log)
    fio_http_write_log(h, FIO_BUF_INFO2(NULL, 0)); /* TODO: get_peer_addr */
  fio_srv_defer(fio___http_controller_http1_on_finish_task,
                (void *)(c),
                fio_http_is_upgraded(h) ? (void *)h : NULL);
}

/* *****************************************************************************
HTTP/2 Protocol (disconnect, as HTTP/2 is unsupported)
***************************************************************************** */

// /** Called when an IO is attached to a protocol. */
// void (*on_attach)(fio_s *io);
// /** Called when a data is available. */
// void (*on_data)(fio_s *io);
// /** called once all pending `fio_write` calls are finished. */
// void (*on_ready)(fio_s *io);
// /** Called after the connection was closed, and pending tasks
// completed.
// */ void (*on_close)(void *udata);

/* *****************************************************************************
HTTP/2 Controller (TODO!)
***************************************************************************** */

// /** Called when an HTTP handle is freed. */
// void (*on_destroyed)(fio_http_s *h, void *cdata);
// /** Informs the controller that request / response headers must be
// sent.
// */ void (*send_headers)(fio_http_s *h);
// /** called by the HTTP handle for each body chunk (or to finish a
// response.
// */ void (*write_body)(fio_http_s *h, fio_http_write_args_s args);
// /** called once a request / response had finished */
// void (*on_finish)(fio_http_s *h);

/* *****************************************************************************
Authentication Helper
***************************************************************************** */

/** Allows all clients to connect (bypasses authentication). */
SFUNC int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h) {
  ((void)h);
  return 0;
}

/* *****************************************************************************
WebSocket Parser Callbacks
***************************************************************************** */

FIO_SFUNC int fio___websocket_process_data(fio_s *io,
                                           fio___http_connection_s *c);

FIO_SFUNC void fio___websocket_on_message_finalize(void *c_, void *ignr_) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  fio_bstr_free(c->state.ws.msg);
  c->state.ws.msg = NULL;
  c->suspend = 0;
  fio___websocket_process_data(c->io, c);
  if (!c->suspend)
    fio_srv_unsuspend(c->io);
  fio_undup(c->io);
  (void)ignr_;
}

FIO_SFUNC void fio___websocket_on_message_task(void *c_, void *is_text) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  c->state.ws.on_message(c->h,
                         fio_bstr_buf(c->state.ws.msg),
                         (uint8_t)(uintptr_t)is_text);
  fio_srv_defer(fio___websocket_on_message_finalize, c, NULL);
}

/** Called when a message frame was received. */
FIO_SFUNC void fio_websocket_on_message(void *udata,
                                        fio_buf_info_s msg,
                                        unsigned char is_text) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  c->state.ws.on_message(c->h,
                         fio_bstr_buf(c->state.ws.msg),
                         (uint8_t)(uintptr_t)is_text);
  fio_bstr_free(c->state.ws.msg);
  c->state.ws.msg = NULL;
  return; /* TODO: FIXME! */
  fio_dup(c->io);
  fio_srv_suspend(c->io);
  c->suspend = 1;
  fio_queue_push(c->queue,
                 fio___websocket_on_message_task,
                 udata,
                 (void *)(uintptr_t)is_text);
  (void)msg;
}

/**
 * Called when the parser needs to copy the message to an external buffer.
 *
 * MUST return the external buffer, as it may need to be unmasked.
 *
 * Partial message length may be equal to zero (`partial.len == 0`).
 */
FIO_SFUNC fio_buf_info_s fio_websocket_write_partial(void *udata,
                                                     fio_buf_info_s partial,
                                                     size_t more_expected) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (!c->state.ws.msg && more_expected)
    c->state.ws.msg = fio_bstr_reserve(NULL, more_expected + partial.len);
  c->state.ws.msg = fio_bstr_write(c->state.ws.msg, partial.buf, partial.len);
  return fio_bstr_buf(c->state.ws.msg);
}

/** Called when the permessage-deflate extension requires decompression. */
FIO_SFUNC fio_buf_info_s fio_websocket_decompress(void *udata,
                                                  fio_buf_info_s msg) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  FIO_LOG_ERROR("WebSocket permessage-deflate not yet implemented!");
  (void)c;
  return msg;
}

/** Called when a `ping` message was received. */
FIO_SFUNC void fio_websocket_on_protocol_ping(void *udata, fio_buf_info_s msg) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (msg.len < 252) {
    char buf[256];
    size_t len =
        fio_websocket_server_wrap(buf, msg.buf, msg.len, 0x0A, 1, 1, 0);
    fio_write2(c->io, .buf = buf, .len = len, .copy = 1);
  } else {
    char *pong = fio_bstr_reserve(NULL, msg.len + 11);
    size_t len =
        fio_websocket_server_wrap(pong, msg.buf, msg.len, 0x0A, 1, 1, 0);
    pong = fio_bstr_len_set(pong, len);
    fio_write2(c->io,
               .buf = pong,
               .len = len,
               .dealloc = (void (*)(void *))fio_bstr_free);
  }
  fio_bstr_free(c->state.ws.msg);
  c->state.ws.msg = NULL;
}

/** Called when a `pong` message was received. */
FIO_SFUNC void fio_websocket_on_protocol_pong(void *udata, fio_buf_info_s msg) {
#if (DEBUG - 1 + 1) || (FIO_WEBSOCKET_STATS - 1 + 1)
  {
    char *pos = msg.buf;
    static uint64_t longest = 0;
    uint64_t ping_time = fio_srv_last_tick() - fio_atol16u(&pos);
    if (ping_time < (1 << 16) && longest < ping_time) {
      longest = ping_time;
      FIO_LOG_INFO("WebSocket longest ping round-trip detected as: %zums",
                   (size_t)ping_time);
    }
  }
#endif
  FIO_LOG_DDEBUG2("Pong (%zu): %s", msg.len, msg.buf);
  (void)msg; /* do nothing */
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_bstr_free(c->state.ws.msg);
  c->state.ws.msg = NULL;
}

/** Called when a `close` message was received. */
FIO_SFUNC void fio_websocket_on_protocol_close(void *udata,
                                               fio_buf_info_s msg) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  char buf[32];
  size_t len = fio_websocket_server_wrap(buf, NULL, 0, 0x08, 1, 1, 0);
  fio_write(c->io, buf, len);
  if (msg.len > 1)
    c->state.ws.code = fio_buf2u16_be(msg.buf);
  fio_close(c->io);
  if (msg.len > 2)
    FIO_LOG_DDEBUG2("WebSocket %p closed with error message: %s",
                    c->io,
                    msg.buf + 2);
  (void)msg;
}

/* *****************************************************************************
WebSocket Protocol
***************************************************************************** */

FIO_SFUNC int fio___websocket_process_data(fio_s *io,
                                           fio___http_connection_s *c) {
  (void)io, (void)c;
  size_t consumed = fio_websocket_parse(&c->state.ws.parser,
                                        FIO_BUF_INFO2(c->buf, c->len),
                                        (void *)c);
  if (!consumed)
    return -1;
  if (consumed == FIO_WEBSOCKET_PARSER_ERROR)
    goto ws_error;
  c->len -= consumed;
  if (c->len)
    FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
  if (c->suspend)
    return -1;
  return 0;

ws_error:
  FIO_LOG_DDEBUG2("WebSocket protocol error?");
  fio_websocket_on_protocol_close((void *)c, ((fio_buf_info_s){0}));
  return -1;
}

// /** Called when a data is available. */
FIO_SFUNC void fio___websocket_on_data(fio_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  size_t r;
  for (;;) {
    if (c->capa == c->len)
      return;
    if (!(r = fio_read(io, c->buf + c->len, c->capa - c->len)))
      return;
    c->len += r;
    if (fio___websocket_process_data(io, c))
      return;
  }
}

FIO_SFUNC void fio___websocket_on_timeout(fio_s *io) {
  char buf[32];
  char tm[20] = "0x00000000000000000";
  fio_ltoa16u(tm + 2, fio_srv_last_tick(), 16);
  size_t len = fio_websocket_server_wrap(buf, tm, 18, 0x09, 1, 1, 0);
  fio_write(io, buf, len);
}

FIO_SFUNC void fio___websocket_on_shutdown(fio_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  c->settings->on_shutdown(c->h);
  fio_websocket_on_protocol_close(c, ((fio_buf_info_s){0}));
}

/** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___websocket_on_attach(fio_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  fio_http_s *h = c->h;
  c->state.ws = (struct fio___http_connection_ws_s){
      .on_message = c->settings->on_message,
  };
  c->settings->on_open(h);
  fio___websocket_process_data(io, c);
}

/** Called after the connection was closed, and pending tasks completed. */
FIO_SFUNC void fio___websocket_on_close(void *udata) {
  FIO_LOG_DDEBUG2("(%d) WebSocket connection closed for %p",
                  (int)fio_thread_getpid(),
                  udata);
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  c->io = NULL;
  fio_bstr_free(c->state.ws.msg);
  fio_http_status_set(c->h, (size_t)(c->state.ws.code));
  c->settings->on_close(c->h);
  fio_http_free(c->h);
  fio___http_connection_free(c);
}

/**
 * Sets a specific on_message callback for this connection.
 *
 * Returns -1 on error (i.e., upgrade still in negotiation).
 */
SFUNC int fio_http_on_message_set(fio_http_s *h,
                                  void (*on_message)(fio_http_s *,
                                                     fio_buf_info_s,
                                                     uint8_t)) {
  if (!h)
    return -1;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c)
    return -1;
  if (!on_message)
    on_message = c->settings->on_message;
  c->state.ws.on_message = on_message;
  return 0;
}

/* *****************************************************************************
WebSocket Writing / Subscription Helpers
***************************************************************************** */

FIO_IFUNC void fio___http_websocket_subscribe_imp(fio_msg_s *msg,
                                                  uint8_t is_text) {
  fio___http_connection_s *c =
      (fio___http_connection_s *)fio_udata_get(msg->io);
  if (!c)
    return;
  fio_http_websocket_write(c->h, msg->message.buf, msg->message.len, is_text);
}

/** Optional WebSocket subscription callback - all messages are UTF-8 valid. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_msg_s *msg) {
  fio___http_websocket_subscribe_imp(msg, 1);
}
/** Optional WebSocket subscription callback - messages may be non-UTF-8. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY(fio_msg_s *msg) {
  fio___http_websocket_subscribe_imp(msg, 0);
}

/** Optional WebSocket subscription callback. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_msg_s *msg) {
  ((msg->message.len < FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT) &&
           (fio_string_utf8_valid(
               FIO_STR_INFO2((char *)msg->message.buf, msg->message.len)))
       ? FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT
       : FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY)(msg);
}

/* *****************************************************************************
EventSource (SSE) Helpers - HTTP Upgraded Connections
***************************************************************************** */

void fio_http_sse_write___(void); /* IDE Marker */
/** Writes an SSE message (UTF-8). Fails if connection wasn't upgraded yet. */
SFUNC int fio_http_sse_write FIO_NOOP(fio_http_s *h,
                                      fio_http_sse_write_args_s args) {
  if (!args.data.len || !h || !fio_http_is_sse(h))
    return -1;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c || !c->io)
    return -1;
  char *payload =
      fio_bstr_reserve(NULL, args.id.len + args.event.len + args.data.len + 22);
  if (args.id.len)
    payload = fio_bstr_write2(payload,
                              FIO_STRING_WRITE_STR2("id:", 3),
                              FIO_STRING_WRITE_STR2(args.id.buf, args.id.len),
                              FIO_STRING_WRITE_STR2("\r\n", 2));
  if (args.event.len)
    payload =
        fio_bstr_write2(payload,
                        FIO_STRING_WRITE_STR2("event:", 6),
                        FIO_STRING_WRITE_STR2(args.event.buf, args.event.len),
                        FIO_STRING_WRITE_STR2("\r\n", 2));
  { /* separate lines (add "data:" at beginning of each new line) */
    char *pos = (char *)FIO_MEMCHR(args.data.buf, '\n', args.data.len);
    while (pos) {
      size_t len = pos - args.data.buf;
      args.data.buf += len + 1;
      args.data.len -= len + 1;
      --len;
      len -= (pos > args.data.buf && pos[-1] == '\r');
      payload =
          fio_bstr_write2(payload,
                          FIO_STRING_WRITE_STR2("data:", 5),
                          FIO_STRING_WRITE_STR2(args.data.buf, args.data.len),
                          FIO_STRING_WRITE_STR2("\r\n", 2));
    }
  }
  /* write reminder */
  if (args.data.len)
    payload =
        fio_bstr_write2(payload,
                        FIO_STRING_WRITE_STR2("data:", 5),
                        FIO_STRING_WRITE_STR2(args.data.buf, args.data.len),
                        FIO_STRING_WRITE_STR2("\r\n", 2));
  /* event ends on empty line */
  payload = fio_bstr_write(payload, "\r\n", 2);
  fio_write2(c->io,
             .buf = payload,
             .len = fio_bstr_len(payload),
             .dealloc = (void (*)(void *))fio_bstr_free);
  return 0;
}

/** Optional EventSource subscription callback - messages MUST be UTF-8. */
SFUNC void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_msg_s *msg) {
  fio___http_connection_s *c =
      (fio___http_connection_s *)fio_udata_get(msg->io);
  if (!c)
    return;
  fio_http_sse_write(c->h,
                     .event = FIO_STR2BUF_INFO(msg->channel),
                     .data = FIO_STR2BUF_INFO(msg->message));
}

/* *****************************************************************************
WebSocket Writing / Subscription Helpers
***************************************************************************** */

SFUNC int fio_http_websocket_write(fio_http_s *h,
                                   const void *buf,
                                   size_t len,
                                   uint8_t is_text) {
  if (!h || !(h->state & FIO_HTTP_STATE_WEBSOCKET))
    return -1;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c)
    return -1;
  is_text = (!!is_text);
  is_text |= (!is_text) << 1;
  uint8_t rsv = 0;
  if (len < 512) {
    char tmp[520];
    size_t wlen =
        (c->is_client
             ? fio_websocket_client_wrap
             : fio_websocket_server_wrap)(tmp, buf, len, is_text, 1, 1, rsv);
    fio_write2(c->io, .buf = tmp, .len = wlen, .copy = 1);
    return 0;
  }
#if HAVE_ZLIB /* TODO: compress? */
  // if(len > 512 && c->state.ws.deflate) ;
#endif
  char *payload =
      fio_bstr_reserve(NULL,
                       fio_websocket_wrapped_len(len) + (c->is_client << 2));
  payload = fio_bstr_len_set(
      payload,
      (c->is_client
           ? fio_websocket_client_wrap
           : fio_websocket_server_wrap)(payload, buf, len, is_text, 1, 1, rsv));
  fio_write2(c->io,
             .buf = payload,
             .len = fio_bstr_len(payload),
             .dealloc = (void (*)(void *))fio_bstr_free);
  return 0 - !fio_srv_is_open(c->io);
}

/* *****************************************************************************
WebSocket Controller
***************************************************************************** */

FIO_SFUNC void fio___http_controller_ws_on_finish_task(void *h_, void *ignr_) {
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  fio_protocol_set(c->io, NULL); /* make zombie, timeout will clear it. */
  fio___http_connection_free(c);
  (void)ignr_;
}

/** called once a request / response had finished */
FIO_SFUNC void fio___http_controller_ws_on_finish(fio_http_s *h) {
  fio_srv_defer(fio___http_controller_ws_on_finish_task, (void *)(h), NULL);
}

/* called by the HTTP handle for each body chunk (or to finish a response. */
FIO_SFUNC void fio___http_controller_ws_write_body(fio_http_s *h,
                                                   fio_http_write_args_s args) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (args.buf && args.len < FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT) {
    unsigned char is_text =
        !!fio_string_utf8_valid(FIO_STR_INFO2((char *)args.buf, args.len));
    fio_http_websocket_write(h, (void *)args.buf, args.len, is_text);
    if (args.dealloc)
      args.dealloc((void *)args.buf);
    return;
  }
  char header[16];
  ((uint8_t *)header)[0] = 0 | 2 | 128;
  if (args.len < 126) {
    ((uint8_t *)header)[1] = args.len;
    fio_write(c->io, header, 2);
  } else if (args.len < (1UL << 16)) {
    /* head is 4 bytes */
    ((uint8_t *)header)[1] = 126 | ((!!c->is_client) << 7);
    fio_u2buf16_be(((uint8_t *)header + 2), args.len);
    fio_write(c->io, header, 4);
  } else {
    /* Really Long Message  */
    ((uint8_t *)header)[1] = 127 | ((!!c->is_client) << 7);
    fio_u2buf64_be(((uint8_t *)header + 2), args.len);
    fio_write(c->io, header, 10);
  }
  fio_write2(c->io,
             .buf = (void *)args.buf,
             .fd = args.fd,
             .len = args.len,
             .offset = args.offset,
             .dealloc = args.dealloc,
             .copy = (uint8_t)args.copy);
}

/* *****************************************************************************
EventSource / SSE Protocol (TODO!)
***************************************************************************** */

/** Called when an IO is attached to a protocol. */
static void fio___sse_on_attach(fio_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  fio_http_s *h = c->h;
  c->state.sse = (struct fio___http_connection_sse_s){
      .on_message = c->settings->on_message,
  };
  c->settings->on_open(h);
  // fio___websocket_process_data(io, c); /* TODO: SSE client mode */
}

FIO_SFUNC void fio___sse_on_timeout(fio_s *io) {
  char buf[32] = ":ping 0x0000000000000000\r\n\r\n";
  fio_ltoa16u(buf + 8, fio_srv_last_tick(), 16);
  buf[24] = '\r'; /* overwrite written NUL character */
  fio_write(io, buf, 28);
}

FIO_SFUNC void fio___sse_on_shutdown(fio_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  c->settings->on_shutdown(c->h);
  // fio_websocket_on_protocol_close(c, ((fio_buf_info_s){0}));
}

/** Called after the connection was closed, and pending tasks completed. */
FIO_SFUNC void fio___sse_on_close(void *udata) {
  FIO_LOG_DDEBUG2("(%d) SSE connection closed for %p",
                  (int)fio_thread_getpid(),
                  udata);
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  c->io = NULL;
  // fio_bstr_free(c->state.sse.msg);
  fio_http_free(c->h);
  fio___http_connection_free(c);
}

/* *****************************************************************************
EventSource / SSE Controller (TODO!)
***************************************************************************** */

/* called by the HTTP handle for each body chunk (or to finish a response. */
FIO_SFUNC void fio___http_controller_sse_write_body(
    fio_http_s *h,
    fio_http_write_args_s args) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (args.buf && args.len) {
    fio_http_sse_write(c->h, .data = FIO_BUF_INFO2((char *)args.buf, args.len));
  }
  if (args.dealloc && args.buf)
    args.dealloc((void *)args.buf);
  if (args.fd != -1)
    close(args.fd);
}
/* *****************************************************************************
Connection Lost
***************************************************************************** */

FIO_SFUNC void fio___http_controller_on_destroyed_task(void *c_, void *ignr_) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  fio___http_connection_free(c);
  (void)ignr_;
}

/** Called when an HTTP handle is freed. */
FIO_SFUNC void fio__http_controller_on_destroyed(fio_http_s *h) {
  if (!(fio_http_is_upgraded(h) | fio_http_is_finished(h))) {
    /* auto-finish if freed without finishing */
    if (!fio_http_status(h))
      fio_http_status_set(h, 500); /* ignored if headers already sent */
    fio_http_write_args_s args = {.finish = 1}; /* never sets upgrade flag */
    fio_http_write FIO_NOOP(h, args);
  }
  fio_queue_push(fio_srv_queue(),
                 fio___http_controller_on_destroyed_task,
                 fio_http_cdata(h));
}

/** Called when an HTTP handle is freed. */
FIO_SFUNC void fio__http_controller_on_destroyed2(fio_http_s *h) {
  fio_queue_push(fio_srv_queue(),
                 fio___http_controller_on_destroyed_task,
                 fio_http_cdata(h));
}

/* *****************************************************************************
The Protocols at play
***************************************************************************** */

/** Returns a facil.io protocol object with the proper protocol callbacks. */
FIO_IFUNC fio_protocol_s FIO_NOOP
fio___http_protocol_get(fio___http_protocol_selector_e s, int is_client) {
  fio_protocol_s r = {0};
  (void)is_client, (void)s;
  switch (s) {
  case FIO___HTTP_PROTOCOL_ACCEPT:
    r = (fio_protocol_s){.on_attach = fio___http_on_attach_accept,
                         .on_data = fio___http1_accept_on_data,
                         .on_close = fio___http_on_close};
    return r;
  case FIO___HTTP_PROTOCOL_HTTP1:
    r = (fio_protocol_s){.on_attach = fio___http1_on_attach,
                         .on_data = fio___http1_on_data,
                         .on_close = fio___http_on_close};
    return r;
  case FIO___HTTP_PROTOCOL_HTTP2:
    r = (fio_protocol_s){.on_close = fio___http_on_close};
    return r;
  case FIO___HTTP_PROTOCOL_WS:
    r = (fio_protocol_s){
        .on_attach = fio___websocket_on_attach,
        .on_data = fio___websocket_on_data,
        .on_timeout = fio___websocket_on_timeout,
        .on_shutdown = fio___websocket_on_shutdown,
        .on_close = fio___websocket_on_close,
        .on_pubsub = FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT,
    };
    return r;
  case FIO___HTTP_PROTOCOL_SSE:
    r = (fio_protocol_s){
        .on_attach = fio___sse_on_attach,
        .on_timeout = fio___sse_on_timeout,
        .on_shutdown = fio___sse_on_shutdown,
        .on_close = fio___sse_on_close,
        .on_pubsub = FIO_HTTP_SSE_SUBSCRIBE_DIRECT,
    };
    return r;
  case FIO___HTTP_PROTOCOL_NONE: /* fall through*/
    r = (fio_protocol_s){.on_close = fio___http_on_close};
    return r;
  default:
    FIO_LOG_ERROR("internal function `fio___http_protocol_get` called with "
                  "illegal arguments!");
    return r;
  }
}

/** Returns an http controller object with the proper protocol callbacks. */
FIO_IFUNC fio_http_controller_s
fio___http_controller_get(fio___http_protocol_selector_e s, int is_client) {
  fio_http_controller_s r = {0};
  (void)is_client, (void)s;
  switch (s) {
  case FIO___HTTP_PROTOCOL_ACCEPT:
    r = (fio_http_controller_s){
        .send_headers = fio___http_controller_http1_send_headers,
        .write_body = fio___http_controller_http1_write_body,
        .on_finish = fio___http_controller_http1_on_finish,
        .on_destroyed = fio__http_controller_on_destroyed,
        .close = fio___http_default_close,
    };
    return r;
  case FIO___HTTP_PROTOCOL_HTTP1:
    r = (fio_http_controller_s){
        .send_headers = fio___http_controller_http1_send_headers,
        .write_body = fio___http_controller_http1_write_body,
        .on_finish = fio___http_controller_http1_on_finish,
        .on_destroyed = fio__http_controller_on_destroyed,
        .close = fio___http_default_close,
    };
    return r;
  case FIO___HTTP_PROTOCOL_HTTP2:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed,
        .close = fio___http_default_close,
    };
    return r;
  case FIO___HTTP_PROTOCOL_WS:
    r = (fio_http_controller_s){
        .on_finish = fio___http_controller_ws_on_finish,
        .write_body = fio___http_controller_ws_write_body,
        .on_destroyed = fio__http_controller_on_destroyed2,
        .close = fio___http_default_close,
    };
    return r;
  case FIO___HTTP_PROTOCOL_SSE:
    r = (fio_http_controller_s){
        .write_body = fio___http_controller_sse_write_body,
        .on_finish = fio___http_controller_ws_on_finish,
        .on_destroyed = fio__http_controller_on_destroyed2,
        .close = fio___http_default_close,
    };
    return r;
  case FIO___HTTP_PROTOCOL_NONE:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed2,
        .close = fio___http_default_close,
    };
    return r;
  default:
    FIO_LOG_ERROR("internal function `fio___http_controller_get` called with "
                  "illegal arguments!");
    return r;
  }
}

/* *****************************************************************************
HTTP Helpers
***************************************************************************** */

/** Returns the IO object associated with the HTTP object (request only). */
SFUNC fio_s *fio_http_io(fio_http_s *h) {
  if (!h)
    return NULL;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  return c->io;
}

/* *****************************************************************************
Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_HTTP
#endif /* FIO_HTTP */
