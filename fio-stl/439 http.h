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
#if defined(FIO_HTTP) && !defined(H___FIO_HTTP___H) && !defined(FIO_STL_KEEP__)
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
#ifndef FIO_HTTP_PIPELINE_QUEUE
/** Limits pipelining request / response storage. NOTE: must be a power of 2. */
#define FIO_HTTP_PIPELINE_QUEUE 4
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
  struct fio_io_functions *tls_io_func;
  /** Optional SSL/TLS support. */
  void *tls;
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
  size_t max_header_size;
  /**
   * The maximum number of bytes allowed per header / request line.
   *
   * Defaults to FIO_HTTP_DEFAULT_MAX_LINE_LEN bytes.
   */
  size_t max_line_len;
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
SFUNC void fio_http_listen(const char *url, fio_http_settings_s settings);

/** Listens to HTTP / WebSockets / SSE connections on `url`. */
#define fio_http_listen(url, ...)                                              \
  fio_http_listen(url, (fio_http_settings_s){__VA_ARGS__})

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
static int fio___http___mock_noop_allow(fio_http_s *h) {
  ((void)h);
  return 0; /* TODO!: change to -1; */
}
static void http___noop_on_finish(struct fio_http_settings_s *settings) {
  ((void)settings);
}

static void http_settings_validate(fio_http_settings_s *s) {
  if (!s->on_http)
    s->on_http = fio___http___mock_noop;
  if (!s->on_upgrade2websockets)
    s->on_upgrade2websockets = fio___http___mock_noop_allow;
  if (!s->on_upgrade2sse)
    s->on_upgrade2sse = fio___http___mock_noop_allow;
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
  // if(!s->tls_io_func) s->tls_io_func = 0;
  // if(!s->tls) s->tls = 0;
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
#define FIO_STL_KEEP__ 1

typedef struct {
  fio_http_settings_s settings;
  void (*on_http_callback)(void *, void *);
  struct {
    fio_protocol_s protocol;
    fio_http_controller_s controller;
  } state[FIO___HTTP_PROTOCOL_NONE];
} fio_http_protocol_s;
#include FIO_INCLUDE_FILE

#define FIO_REF_NAME             fio_http_protocol
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
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
  size_t state;
  size_t limit;
  fio_http_s *queue[FIO_HTTP_PIPELINE_QUEUE];
  fio_http_settings_s *settings;
  void *udata;
  fio_http1_parser_s parser;
  uint32_t qrpos;
  uint32_t qwpos;
  size_t total;
  size_t len;
  char buf[];
} fio_http_connection_s;

#define FIO_REF_NAME             fio_http_connection
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
    const size_t mask = FIO_HTTP_PIPELINE_QUEUE - 1;                           \
    while (o.queue[o.qrpos & mask]) {                                          \
      fio_http_free(o.queue[o.qrpos & mask]);                                  \
      o.queue[(o.qrpos++) & mask] = NULL;                                      \
    }                                                                          \
  } while (0)
#include FIO_INCLUDE_FILE

#undef FIO_STL_KEEP__
/* *****************************************************************************
HTTP On Open - Accepting new connections
***************************************************************************** */

static void http___on_open(int fd, void *udata) {
  fio_http_protocol_s *p = fio_http_protocol_dup((fio_http_protocol_s *)udata);
  fio_http_connection_s *c = fio_http_connection_new(p->settings.max_line_len);
  *c = (fio_http_connection_s){
      .settings = &(p->settings),
      .udata = p->settings.udata,
      .limit = p->settings.max_line_len,
  };
  c->io = fio_attach_fd(fd,
                        &p->state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                        (void *)c,
                        p->settings.tls);
}

/* *****************************************************************************
HTTP Request handling / handling
***************************************************************************** */

FIO_IFUNC int fio___http_on_http_test4upgrade(fio_http_s *h,
                                              fio_http_connection_s *c) {

  if (fio_http_websockets_requested(h))
    goto websocket_requested;
  if (fio_http_sse_requested(h))
    goto sse_requested;
  return 0;
websocket_requested:
  if (c->settings->on_upgrade2websockets(h))
    goto deny_upgrade;
  /* TODO: set WebSocket response headers + send response */
  /* TODO: feed remaining data in buffer to WebSocket client */
  goto deny_upgrade; /* TODO: delete me once support is implemented */
  return -1;
sse_requested:
  if (c->settings->on_upgrade2sse(h))
    goto deny_upgrade;
  goto deny_upgrade; /* TODO: delete me once support is implemented */
  return -1;
#if 0
http2_requested:
  // Connection: Upgrade, HTTP2-Settings
  // Upgrade: h2c
  // HTTP2-Settings: <base64url encoding of HTTP/2 SETTINGS payload>
  return 0; /* allowed to ignore upgrade request */
#endif
deny_upgrade:
  fio_http_send_error_response(h, 403);
  return -1;
}

FIO_SFUNC void fio___http_on_http_direct(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  fio_http_status_set(h, 200);
  fio_http_connection_s *c = (fio_http_connection_s *)fio_http_cdata(h);
  if (fio___http_on_http_test4upgrade(h, c))
    return;
  c->settings->on_http(h);
  (void)ignr;
}

FIO_SFUNC void fio___http_on_http_with_public_folder(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  fio_http_connection_s *c = (fio_http_connection_s *)fio_http_cdata(h);
  fio_http_status_set(h, 200);
  if (fio___http_on_http_test4upgrade(h, c))
    return;
  if (fio_http_static_file_response(h,
                                    c->settings->public_folder,
                                    fio_http_path(h),
                                    c->settings->max_age))
    fio___http_on_http_direct(h_, ignr);
  c->settings->on_http(h);
  return;
}

/* *****************************************************************************
HTTP Listen
***************************************************************************** */
void fio_http_listen___(void); /* IDE marker */
SFUNC void fio_http_listen FIO_NOOP(const char *url, fio_http_settings_s s) {
  http_settings_validate(&s);
  fio_http_protocol_s *p = fio_http_protocol_new();
  for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE; ++i) {
    p->state[i].protocol =
        fio___http_protocol_get((fio___http_protocol_selector_e)i, 0);
    p->state[i].controller =
        fio___http_controller_get((fio___http_protocol_selector_e)i, 0);
  }
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
  fio_listen(.url = url,
             .on_open = http___on_open,
             .on_finish = (void (*)(void *))fio_http_protocol_free,
             .udata = (void *)p
             /* , .queue_for_accept = http___queue_for_accept // TODO! */
  );
}

/* *****************************************************************************
HTTP/1.1 Request / Response Completed
***************************************************************************** */

/** called when either a request or a response was received. */
static void fio_http1_on_complete(void *udata) {
  fio_http_connection_s *c = (fio_http_connection_s *)udata;
  if (c->qrpos == c->qwpos) {
    fio_defer((FIO_PTR_FROM_FIELD(fio_http_protocol_s, settings, c->settings)
                   ->on_http_callback),
              c->queue[c->qwpos],
              NULL);
    c->qrpos = (c->qrpos + 1) & (FIO_HTTP_PIPELINE_QUEUE - 1);
  }
  c->qwpos = (c->qwpos + 1) & (FIO_HTTP_PIPELINE_QUEUE - 1);
}

/* *****************************************************************************
HTTP/1.1 Parser callbacks
***************************************************************************** */

/** called when a request method is parsed. */
static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  fio_http_connection_s *c = (fio_http_connection_s *)udata;
  if (c->queue[c->qwpos])
    return -1;
  c->queue[c->qwpos] = fio_http_new();
  FIO_ASSERT_ALLOC(c->queue[c->qwpos]);
  fio_http_controller_set(
      c->queue[c->qwpos],
      &(fio_http_protocol_dup(
            FIO_PTR_FROM_FIELD(fio_http_protocol_s, settings, c->settings))
            ->state[FIO___HTTP_PROTOCOL_HTTP1]
            .controller));
  fio_http_udata_set(c->queue[c->qwpos], c->udata);
  fio_http_cdata_set(c->queue[c->qwpos], fio_http_connection_dup(c));
  fio_http_method_set(c->queue[c->qwpos], FIO_BUF2STR_INFO(method));
  return 0;
}
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata) {
  FIO_LOG_ERROR("response received instead of a request. Silently ignored.");
  return -1; /* do not accept responses */
  (void)istatus, (void)status, (void)udata;
}
/** called when a request URL is parsed. */
static int fio_http1_on_url(fio_buf_info_s url, void *udata) {
  fio_http_connection_s *c = (fio_http_connection_s *)udata;
  fio_url_s u = fio_url_parse(url.buf, url.len);
  if (!u.path.len || u.path.buf[0] != '/')
    return -1;
  fio_http_path_set(c->queue[c->qwpos], FIO_BUF2STR_INFO(u.path));
  if (u.query.len)
    fio_http_query_set(c->queue[c->qwpos], FIO_BUF2STR_INFO(u.query));
  if (u.host.len)
    (!(c->queue[c->qwpos])
         ? fio_http_request_header_set
         : fio_http_response_header_set)(c->queue[c->qwpos],
                                         FIO_STR_INFO1((char *)"host"),
                                         FIO_BUF2STR_INFO(u.host));
  return 0;
}
/** called when a the HTTP/1.x version is parsed. */
static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  fio_http_connection_s *c = (fio_http_connection_s *)udata;
  fio_http_version_set(c->queue[c->qwpos], FIO_BUF2STR_INFO(version));
  return 0;
}
/** called when a header is parsed. */
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  fio_http_connection_s *c = (fio_http_connection_s *)udata;
  (!(c->queue[c->qwpos])
       ? fio_http_request_header_add
       : fio_http_response_header_add)(c->queue[c->qwpos],
                                       FIO_BUF2STR_INFO(name),
                                       FIO_BUF2STR_INFO(value));
  return 0;
}
/** called when the special content-length header is parsed. */
static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata) {
  fio_http_connection_s *c = (fio_http_connection_s *)udata;
  if (content_length > c->settings->max_body_size)
    goto too_big;
  if (content_length)
    fio_http_body_expect(c->queue[c->qwpos], content_length);
#if FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER
  (!(c->queue[c->qwpos])
       ? fio_http_request_header_add
       : fio_http_response_header_add)(c->queue[c->qwpos],
                                       FIO_BUF2STR_INFO(name),
                                       FIO_BUF2STR_INFO(value));
#endif
  return 0;
too_big:
  fio_http_send_error_response(c->queue[c->qwpos], 413);
  return -1; /* TODO: send "payload too big" response */
  (void)name, (void)value;
}
/** called when `Expect` arrives and may require a 100 continue response. */
static int fio_http1_on_expect(fio_buf_info_s expected, void *udata) {
  fio_http_connection_s *c = (fio_http_connection_s *)udata;
  fio_write2(c->io, .buf = (char *)"100 Continue\r\n", .len = 14, .copy = 0);
  return 0; /* TODO?: improve support for `expect` */
  (void)expected;
}

/** called when a body chunk is parsed. */
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  fio_http_connection_s *c = (fio_http_connection_s *)udata;
  if (chunk.len + fio_http_body_length(c->queue[c->qwpos]) >
      c->settings->max_body_size)
    return -1;
  fio_http_body_write(c->queue[c->qwpos], chunk.buf, chunk.len);
  return 0;
}

/* *****************************************************************************
HTTP/1.1 Accepting new connections (tests for special HTTP/2 pre-knoledge)
***************************************************************************** */

// /** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___http1_accept_on_attach_client(fio_s *io) {
  fio_http_connection_s *c = (fio_http_connection_s *)fio_udata_get(io);
  fio_protocol_set(
      io,
      &(FIO_PTR_FROM_FIELD(fio_http_protocol_s, settings, c->settings)
            ->state[FIO___HTTP_PROTOCOL_HTTP1]
            .protocol));
}

// /** Called when a data is available. */
FIO_SFUNC void fio___http1_accept_on_data(fio_s *io) {
  const fio_buf_info_s prior_knowledge = FIO_BUF_INFO2(
      (char *)"\x50\x52\x49\x20\x2a\x20\x48\x54\x54\x50\x2f\x32\x2e\x30"
              "\x0d\x0a\x0d\x0a\x53\x4d\x0d\x0a\x0d\x0a",
      24);
  fio_http_connection_s *c = (fio_http_connection_s *)fio_udata_get(io);
  fio_protocol_s *phttp_new;
  size_t r = fio_read(io, c->buf + c->len, c->limit - c->len);
  if (!r)
    return;
  fio_buf_info_s tmp = FIO_BUF_INFO2(
      c->buf,
      (c->len > prior_knowledge.len) ? prior_knowledge.len : c->len);
  if (FIO_MEMCMP(prior_knowledge.buf, tmp.buf, tmp.len)) {
    /* no prior knowledge, switch to HTTP 1 */
    phttp_new = &(FIO_PTR_FROM_FIELD(fio_http_protocol_s, settings, c->settings)
                      ->state[FIO___HTTP_PROTOCOL_HTTP1]
                      .protocol);
    fio_protocol_set(io, phttp_new);
    return;
  }
  if (tmp.len != prior_knowledge.len)
    return;
  if (c->len != prior_knowledge.len)
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
  fio_http_connection_free((fio_http_connection_s *)udata);
}

/* *****************************************************************************
HTTP/1.1 Protocol (TODO!)
***************************************************************************** */

FIO_SFUNC int fio___http1_process_data(fio_s *io, fio_http_connection_s *c) {
  (void)io, (void)c;
  size_t consumed =
      fio_http1_parse(&c->parser, FIO_BUF_INFO2(c->buf, c->len), (void *)c);
  if (!consumed)
    return -1;
  if (consumed == FIO_HTTP1_PARSER_ERROR)
    goto http1_error;
  c->len -= consumed;
  if (c->len)
    FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
  return 0;
http1_error:
  if (c->queue[c->qwpos])
    ;
  return -1;
}

// /** Called when a data is available. */
FIO_SFUNC void fio___http1_on_data(fio_s *io) {
  fio_http_connection_s *c = (fio_http_connection_s *)fio_udata_get(io);
  size_t r;
  while (c->limit > c->len &&
         (r = fio_read(io, c->buf + c->len, c->limit - c->len)) &&
         !fio___http1_process_data(io, c))
    ;
}

// /** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___http1_on_attach(fio_s *io) {
  fio_http_connection_s *c = (fio_http_connection_s *)fio_udata_get(io);
  if (c->len && fio___http1_process_data(io, c))
    fio_suspend(io);
  return;
}

/* *****************************************************************************
HTTP/1 Controller (TODO!)
***************************************************************************** */

// /** Called when an HTTP handle is freed. */
// void (*on_destroyed)(fio_http_s *h, void *cdata);
// /** Informs the controller that request / response headers must be sent. */
// void (*send_headers)(fio_http_s *h);
// /** called by the HTTP handle for each body chunk (or to finish a response.
// */ void (*write_body)(fio_http_s *h, fio_http_write_args_s args);
// /** called once a request / response had finished */
// void (*on_finish)(fio_http_s *h);

/* *****************************************************************************
HTTP/2 Protocol (disconnect, as HTTP/2 is unsupported)
***************************************************************************** */

// /** Called when an IO is attached to a protocol. */
// void (*on_attach)(fio_s *io);
// /** Called when a data is available. */
// void (*on_data)(fio_s *io);
// /** called once all pending `fio_write` calls are finished. */
// void (*on_ready)(fio_s *io);
// /** Called after the connection was closed, and pending tasks completed. */
// void (*on_close)(void *udata);

/* *****************************************************************************
HTTP/2 Controller (TODO!)
***************************************************************************** */

// /** Called when an HTTP handle is freed. */
// void (*on_destroyed)(fio_http_s *h, void *cdata);
// /** Informs the controller that request / response headers must be sent. */
// void (*send_headers)(fio_http_s *h);
// /** called by the HTTP handle for each body chunk (or to finish a response.
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
// /** Called after the connection was closed, and pending tasks completed. */
// void (*on_close)(void *udata);

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
// /** Called after the connection was closed, and pending tasks completed. */
// void (*on_close)(void *udata);

/* *****************************************************************************
EventSource / SSE Controller (TODO!)
***************************************************************************** */

// /** Called when an HTTP handle is freed. */
// void (*on_destroyed)(fio_http_s *h, void *cdata);
// /** Informs the controller that request / response headers must be sent. */
// void (*send_headers)(fio_http_s *h);
// /** called by the HTTP handle for each body chunk (or to finish a response.
// */ void (*write_body)(fio_http_s *h, fio_http_write_args_s args);
// /** called once a request / response had finished */
// void (*on_finish)(fio_http_s *h);

/* *****************************************************************************
The Protocols at play
***************************************************************************** */

/** Returns a facil.io protocol object with the proper protocol callbacks. */
FIO_IFUNC fio_protocol_s
fio___http_protocol_get(fio___http_protocol_selector_e s, int is_client) {
  fio_protocol_s r = {0};
  (void)is_client, (void)s;
  switch (s) {
  case FIO___HTTP_PROTOCOL_ACCEPT:
    r = (fio_protocol_s){.on_attach = fio___http1_accept_on_attach_client,
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
  default: return r;
  }
}

/** Returns an http controller object with the proper protocol callbacks. */
FIO_IFUNC fio_http_controller_s
fio___http_controller_get(fio___http_protocol_selector_e s, int is_client) {
  fio_http_controller_s r = {0};
  (void)is_client, (void)s;
  switch (s) {
  case FIO___HTTP_PROTOCOL_ACCEPT: r = (fio_http_controller_s){0}; return r;
  case FIO___HTTP_PROTOCOL_HTTP1: /* fall through */
  case FIO___HTTP_PROTOCOL_HTTP2: /* fall through */
  case FIO___HTTP_PROTOCOL_WS:    /* fall through */
  case FIO___HTTP_PROTOCOL_SSE:   /* fall through */
  case FIO___HTTP_PROTOCOL_NONE:  /* fall through */
  default: return r;
  }
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
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_HTTP
#endif /* FIO_HTTP */
