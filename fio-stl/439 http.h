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
#if defined(FIO_HTTP) && !defined(H___FIO_HTTP___H) && !defined(FIO___STL_KEEP)
#define H___FIO_HTTP___H
/* *****************************************************************************
HTTP Setting Defaults
***************************************************************************** */

#ifndef FIO_HTTP_DEFAULT_MAX_HEADER_SIZE
#define FIO_HTTP_DEFAULT_MAX_HEADER_SIZE (1UL << 15)
#endif
#ifndef FIO_HTTP_DEFAULT_MAX_LINE_LEN
#define FIO_HTTP_DEFAULT_MAX_LINE_LEN (1UL << 13)
#endif
#ifndef FIO_HTTP_DEFAULT_MAX_BODY_SIZE
#define FIO_HTTP_DEFAULT_MAX_BODY_SIZE (1UL << 25)
#endif
#ifndef FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE
#define FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE (1UL << 18)
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

/* *****************************************************************************
HTTP Listen
***************************************************************************** */

typedef struct fio_http_settings_s {
  /** Callback for HTTP requests (server) or responses (client). */
  void (*on_http)(fio_http_s *h);
  /** Authenticate EventSource (SSE) requests, return non-zero to deny.*/
  int (*on_upgrade2sse)(fio_http_s *h);
  /** Authenticate WebSockets Upgrade requests, return non-zero to deny.*/
  int (*on_upgrade2websockets)(fio_http_s *h);
  /** (optional) the callback to be performed when the HTTP service closes. */
  void (*on_finish)(struct fio_http_settings_s *settings);
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
SFUNC int fio_http_listen(const char *url, fio_http_settings_s settings);

/** Listens to HTTP / WebSockets / SSE connections on `url`. */
#define fio_http_listen(url, ...)                                              \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})

/* *****************************************************************************
HTTP Helpers
***************************************************************************** */

/** Returns the IO object associated with the HTTP object (request only). */
SFUNC fio_s *fio_http_io(fio_http_s *);

/* *****************************************************************************
Module Implementation - inlined static functions
***************************************************************************** */
/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

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

static void fio___http___mock_noop(fio_http_s *h) { ((void)h); }
static int fio___http___mock_noop_upgrade(fio_http_s *h) {
  ((void)h);
  return 0; /* TODO!: change to -1; */
}

// on_queue
static void http___noop_on_finish(struct fio_http_settings_s *settings) {
  ((void)settings);
}

static void http_settings_validate(fio_http_settings_s *s) {
  if (!s->on_http)
    s->on_http = fio___http___mock_noop;
  if (!s->on_upgrade2websockets)
    s->on_upgrade2websockets = fio___http___mock_noop_upgrade;
  if (!s->on_upgrade2sse)
    s->on_upgrade2sse = fio___http___mock_noop_upgrade;
  if (!s->on_finish)
    s->on_finish = http___noop_on_finish;
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
  // if (!s->public_folder) s->public_folder = 0;
  // if (!s->public_folder_length) s->public_folder_length = 0;
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
#define FIO___STL_KEEP 1

typedef struct {
  fio_http_settings_s settings;
  void *tls_ctx;
  void (*on_http_callback)(void *, void *);
  fio_queue_s *queue;
  struct {
    fio_protocol_s protocol;
    fio_http_controller_s controller;
  } state[FIO___HTTP_PROTOCOL_NONE + 1];
} fio_http_protocol_s;
#include FIO_INCLUDE_FILE

#define FIO_REF_NAME             fio_http_protocol
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

/** Connection objects for managing HTTP / WebSocket connection state. */
typedef struct {
  fio_s *io;
  fio_http_s *h;
  fio_http_settings_s *settings;
  fio_queue_s *queue;
  void (*on_http_callback)(void *, void *);
  void (*on_http)(fio_http_s *h);
  void *udata;
  union {
    fio_http1_parser_s http;
  } parser;
  uint32_t len;
  uint32_t capa;
  uint32_t max_header;
  uint8_t log;
  uint8_t suspend;
  char buf[];
} fio___http_connection_s;

#define FIO_REF_NAME             fio___http_connection
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
    fio_http_protocol_free(                                                    \
        FIO_PTR_FROM_FIELD(fio_http_protocol_s, settings, o.settings));        \
  } while (0)
#include FIO_INCLUDE_FILE

#undef FIO___STL_KEEP
/* *****************************************************************************
HTTP On Open - Accepting new connections
***************************************************************************** */

FIO_SFUNC void fio___http_on_open(int fd, void *udata) {
  fio_http_protocol_s *p = fio_http_protocol_dup((fio_http_protocol_s *)udata);
  const uint32_t capa = p->settings.max_line_len;
  fio___http_connection_s *c = fio___http_connection_new(capa);
  FIO_ASSERT_ALLOC(c);
  *c = (fio___http_connection_s){
      .settings = &(p->settings),
      .queue = p->queue,
      .on_http_callback = p->on_http_callback,
      .on_http = p->settings.on_http,
      .udata = p->settings.udata,
      .capa = capa,
      .max_header = p->settings.max_header_size,
      .log = p->settings.log,
  };
  c->io = fio_attach_fd(fd,
                        &p->state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                        (void *)c,
                        p->tls_ctx);
  FIO_ASSERT_ALLOC(c->io);
#if DEBUG
  FIO_LOG_DEBUG2("(%d) HTTP accepted a new connection at fd %d,"
                 "\n\tattached to client %p (io %p)."
                 "\n\tattached protocol %p",
                 getpid(),
                 fd,
                 c,
                 c->io,
                 fio_protocol_get(c->io));
#endif
}

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
  if (FIO_LIKELY(fio_is_open(c->io)))
    cb.fn(h);
  else
    fio_http_write(h, .finish = 1);
}

FIO_SFUNC void fio___http_perform_user_upgrade_callback(void *cb_, void *h_) {
  union {
    int (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.ptr = cb_};
  fio_http_s *h = (fio_http_s *)h_;
  if (cb.fn(h))
    fio_http_send_error_response(h, 403);
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
  cb.fn = c->settings->on_upgrade2websockets;
  fio_queue_push(c->queue,
                 fio___http_perform_user_upgrade_callback,
                 cb.ptr,
                 (void *)h);
  return -1;
sse_requested:
  cb.fn = c->settings->on_upgrade2sse;
  fio_queue_push(c->queue,
                 fio___http_perform_user_upgrade_callback,
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
  } cb = {.fn = c->on_http};
  fio_queue_push(c->queue, fio___http_perform_user_callback, cb.ptr, (void *)h);
  (void)ignr;
}

FIO_SFUNC void fio___http_on_http_with_public_folder(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  fio_http_status_set(h, 200);
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (fio___http_on_http_test4upgrade(h, c))
    return;
  if (!fio_http_static_file_response(h,
                                     c->settings->public_folder,
                                     fio_http_path(h),
                                     c->settings->max_age)) {
    fio_http_free(h);
    return;
  }
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.fn = c->on_http};
  fio_queue_push(c->queue, fio___http_perform_user_callback, cb.ptr, (void *)h);
  (void)ignr;
}

/* *****************************************************************************
HTTP Listen
***************************************************************************** */

FIO_SFUNC void fio___http_listen_on_start(void *p_) {
  fio_http_protocol_s *p = (fio_http_protocol_s *)p_;
  p->queue = p->settings.queue ? p->settings.queue->q : fio_srv_queue();
}

void fio_http_listen___(void); /* IDE marker */
SFUNC int fio_http_listen FIO_NOOP(const char *url, fio_http_settings_s s) {
  http_settings_validate(&s);
  fio_http_protocol_s *p = fio_http_protocol_new();
  fio_tls_s *auto_tls_detected = NULL;
  FIO_ASSERT_ALLOC(p);
  for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE + 1; ++i) {
    p->state[i].protocol =
        fio___http_protocol_get((fio___http_protocol_selector_e)i, 0);
    p->state[i].controller =
        fio___http_controller_get((fio___http_protocol_selector_e)i, 0);
  }
  if (url) { /* TODO! test URL for extra information, such as `cert` and `key`*/
    fio_url_s u = fio_url_parse(url, strlen(url));
    if (u.query.len) {
      /* TODO! add query parsing logic with callbacks to "431 http handle.h" */
    }
  }
  if (s.tls) {
    s.tls = fio_tls_dup(s.tls);
    fio_io_functions_s tmp_fn = fio_tls_default_io_functions(NULL);
    if (!s.tls_io_func)
      s.tls_io_func = &tmp_fn;
    for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE + 1; ++i)
      p->state[i].protocol.io_functions = *s.tls_io_func;
    p->tls_ctx = (s.tls_io_func->build_context
                      ? s.tls_io_func->build_context
                      : fio___io_func_default_build_context)(s.tls, 0);
  }
  fio_tls_free(auto_tls_detected);

  p->settings = s;
  p->on_http_callback = (p->settings.public_folder.len)
                            ? fio___http_on_http_with_public_folder
                            : fio___http_on_http_direct;
  p->settings.public_folder.len -=
      (p->settings.public_folder.len &&
       (p->settings.public_folder.buf[p->settings.public_folder.len - 1] ==
            '/' ||
        p->settings.public_folder.buf[p->settings.public_folder.len - 1] ==
            '\\'));
  return fio_listen(.url = url,
                    .on_open = fio___http_on_open,
                    .on_start = fio___http_listen_on_start,
                    .on_finish = (void (*)(void *))fio_http_protocol_free,
                    .udata = (void *)p,
                    .queue_for_accept = s.queue ? s.queue->q : NULL);
}

/* *****************************************************************************
HTTP/1.1 Request / Response Completed
***************************************************************************** */

/** called when either a request or a response was received. */
static void fio_http1_on_complete(void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_dup(c->io);
  fio_suspend(c->io);
  fio_http_s *h = c->h;
  c->h = NULL;
  c->suspend = 1;
  fio_queue_push(fio_srv_queue(), c->on_http_callback, h);
}

/* *****************************************************************************
HTTP/1.1 Parser callbacks
***************************************************************************** */

FIO_IFUNC void fio_http1_attach_handle(fio___http_connection_s *c) {
  c->h = fio_http_new();
  FIO_ASSERT_ALLOC(c->h);
  fio_http_controller_set(
      c->h,
      &(FIO_PTR_FROM_FIELD(fio_http_protocol_s, settings, c->settings))
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
  if (content_length > c->settings->max_body_size)
    goto too_big;
  if (content_length)
    fio_http_body_expect(c->h, content_length);
#if FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER
  (!(c->h) ? fio_http_request_header_add
           : fio_http_response_header_add)(c->h,
                                           FIO_BUF2STR_INFO(name),
                                           FIO_BUF2STR_INFO(value));
#endif
  return 0;
too_big:
  fio_http_send_error_response(c->h, 413);
  return -1;
  (void)name, (void)value;
}
/** called when `Expect` arrives and may require a 100 continue response. */
static int fio_http1_on_expect(fio_buf_info_s expected, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_write2(c->io, .buf = (char *)"100 Continue\r\n", .len = 14, .copy = 0);
  return 0; /* TODO?: improve support for `expect` headers? */
  (void)expected;
}

/** called when a body chunk is parsed. */
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (chunk.len + fio_http_body_length(c->h) > c->settings->max_body_size)
    return -1;
  fio_http_body_write(c->h, chunk.buf, chunk.len);
  return 0;
}

/* *****************************************************************************
HTTP/1.1 Accepting new connections (tests for special HTTP/2 pre-knoledge)
***************************************************************************** */

/** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___http1_accept_on_attach_client_tmp(fio_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_udata_get(io);
  fio_protocol_set(
      io,
      &(FIO_PTR_FROM_FIELD(fio_http_protocol_s, settings, c->settings)
            ->state[FIO___HTTP_PROTOCOL_HTTP1]
            .protocol));
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
  c->len = r;
  if (FIO_MEMCMP(
          prior_knowledge.buf,
          c->buf,
          (c->len > prior_knowledge.len ? prior_knowledge.len : c->len))) {
    /* no prior knowledge, switch to HTTP/1.1 */
    phttp_new = &(FIO_PTR_FROM_FIELD(fio_http_protocol_s, settings, c->settings)
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
  phttp_new = &(FIO_PTR_FROM_FIELD(fio_http_protocol_s, settings, c->settings)
                    ->state[FIO___HTTP_PROTOCOL_HTTP2]
                    .protocol);

  fio_protocol_set(io, phttp_new);
}

FIO_SFUNC void fio___http_on_close(void *udata) {
#if DEBUG
  FIO_LOG_DEBUG2("(%d) HTTP connection closed for %p", getpid(), udata);
#endif
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  c->io = NULL;
  fio_http_free(c->h);
  fio___http_connection_free(c);
}

/* *****************************************************************************
HTTP/1.1 Protocol (TODO!)
***************************************************************************** */

FIO_SFUNC int fio___http1_process_data(fio_s *io, fio___http_connection_s *c) {
  (void)io, (void)c;
  size_t consumed = fio_http1_parse(&c->parser.http,
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
  if (c->h)
    fio_http_send_error_response(c->h, 400);
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
HTTP/1 Controller (TODO!)
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
  if (!c->io || !fio_is_open(c->io))
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
  if (!c->io || !fio_is_open(c->io))
    goto no_write_err;
  if (fio_http_is_streaming(h))
    goto stream_chunk;
  fio_write2(c->io,
             .buf = (void *)args.data,
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
             .buf = (void *)args.data,
             .len = args.len,
             .fd = args.fd,
             .offset = args.offset,
             .dealloc = args.dealloc,
             .copy = (uint8_t)args.copy);
  /* print chunk trailer */
  fio_write2(c->io, .buf = (char *)"\r\n", .len = 2, .copy = 1);
  return;
no_write_err:
  if (args.data) {
    if (args.dealloc)
      args.dealloc((void *)args.data);
  } else if (args.fd != -1) {
    close(args.fd);
  }
}

FIO_SFUNC void fio___http_controller_http1_on_finish_task(void *h_,
                                                          void *ignr_) {
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  c->suspend = 0;
  if (c->log)
    fio_http_write_log(h, FIO_BUF_INFO2(NULL, 0)); /* TODO: get_peer_addr */
  if (fio_is_open(c->io)) {
    if (fio_http_is_streaming(h)) {
      fio_write2(c->io, .buf = (char *)"0\r\n\r\n", .len = 5, .copy = 1);
    }
    fio___http1_process_data(c->io, c);
  }
  fio_http_free(h);
  if (!c->suspend)
    fio_unsuspend(c->io);
  fio_undup(c->io);
  (void)ignr_;
}

/** called once a request / response had finished */
FIO_SFUNC void fio___http_controller_http1_on_finish(fio_http_s *h) {
  fio_defer(fio___http_controller_http1_on_finish_task, (void *)h, NULL);
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
WebSocket Protocol (TODO!)
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
WebSocket Controller (TODO!)
***************************************************************************** */

/* *****************************************************************************
EventSource / SSE Protocol (TODO!)
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
EventSource / SSE Controller (TODO!)
***************************************************************************** */

// /** Called when an HTTP handle is freed. */
// void (*on_destroyed)(fio_http_s *h, void *cdata);
// /** called by the HTTP handle for each body chunk (or to finish a
// response.
// */ void (*write_body)(fio_http_s *h, fio_http_write_args_s args);
// /** called once a request / response had finished */
// void (*on_finish)(fio_http_s *h);

/* *****************************************************************************
Connection Lost
***************************************************************************** */

FIO_SFUNC void fio___http_controller_on_destroyed_task(void *c_, void *ignr_) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  fio___http_connection_free(c);
  (void)ignr_;
}

// /** Called when an HTTP handle is freed. */
FIO_SFUNC void fio__http_controller_on_destroyed(fio_http_s *h) {
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
    r = (fio_protocol_s){.on_attach = fio___http1_accept_on_attach_client_tmp,
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
    r = (fio_protocol_s){.on_close = fio___http_on_close};
    return r;
  case FIO___HTTP_PROTOCOL_SSE:
    r = (fio_protocol_s){.on_close = fio___http_on_close};
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
    };
    return r;
  case FIO___HTTP_PROTOCOL_HTTP1:
    r = (fio_http_controller_s){
        .send_headers = fio___http_controller_http1_send_headers,
        .write_body = fio___http_controller_http1_write_body,
        .on_finish = fio___http_controller_http1_on_finish,
        .on_destroyed = fio__http_controller_on_destroyed,
    };
    return r;
  case FIO___HTTP_PROTOCOL_HTTP2:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed,
    };
    return r;
  case FIO___HTTP_PROTOCOL_WS:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed,
    };
    return r;
  case FIO___HTTP_PROTOCOL_SSE:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed,
    };
    return r;
  case FIO___HTTP_PROTOCOL_NONE:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed,
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
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  return c->io;
}

/* *****************************************************************************
HTTP Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, http_listen)(void) {
  /*
   * TODO: test module here
   */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
*****************************************************************************
*/

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_HTTP
#endif /* FIO_HTTP */
