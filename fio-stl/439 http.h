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

/* *****************************************************************************
HTTP Listen
***************************************************************************** */

typedef struct fio_http_settings_s {
  /** Callback for HTTP requests (server) or responses (client). */
  void (*on_http)(fio_http_s *h);
  /** Callback for EventSource (SSE) requests. */
  void (*on_upgrade2sse)(fio_http_s *h);
  /** Callback for WebSockets Upgrade requests. */
  void (*on_upgrade2websockets)(fio_http_s *h);
  /** (optional) the callback to be performed when the HTTP service closes. */
  void (*on_finish)(struct http_settings_s *settings);
  /** Opaque user data. */
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
static void http___noop_on_finish(struct http_settings_s *settings) {
  ((void)settings);
}

static void http_settings_validate(fio_http_settings_s *s) {
  if (!s->on_http)
    s->on_http = fio___http___mock_noop;
  if (!s->on_upgrade2websockets)
    s->on_upgrade2websockets = fio___http___mock_noop;
  if (!s->on_upgrade2sse)
    s->on_upgrade2sse = fio___http___mock_noop;
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
  FIO___HTTP_PROTOCOL_HTTP1 = 0,
  FIO___HTTP_PROTOCOL_WS,
  FIO___HTTP_PROTOCOL_SSE,
  FIO___HTTP_PROTOCOL_HTTP2,
  FIO___HTTP_PROTOCOL_NONE
} fio___http_protocol_selector_e;

/** Returns a facil.io protocol object with the proper protocol callbacks. */
FIO_IFUNC fio_protocol_s
fio___protocol_callbacks(fio___http_protocol_selector_e, int is_client);
/** Returns an http controller object with the proper protocol callbacks. */
FIO_IFUNC fio_http_controller_s
fio___controller_callbacks(fio___http_protocol_selector_e, int is_client);

/* *****************************************************************************
HTTP Protocol Container (vtable + settings storage)
***************************************************************************** */
#define FIO_STL_KEEP__ 1

typedef struct {
  fio_http_settings_s settings;
  void (*on_http_callback)(void *, void *);
  fio_protocol_s protocol[FIO___HTTP_PROTOCOL_NONE];
  fio_http_controller_s controller[FIO___HTTP_PROTOCOL_NONE];
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
    const size_t mask = HTTP_PIPELINE_QUEUE - 1;                               \
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
      .limit = p->settings.max_line_len,
  };
  c->io = fio_attach_fd(fd,
                        &p->protocol[FIO___HTTP_PROTOCOL_HTTP1],
                        (void *)c,
                        p->settings.tls);
}

/* *****************************************************************************
HTTP Request handling / handling
***************************************************************************** */

FIO_SFUNC void fio___http_on_http_direct(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  (void)ignr;
}

FIO_SFUNC void fio___http_on_http_with_public_folder(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  fio_http_connection_s *c = (fio_http_connection_s *)fio_http_cdata(h);
  if (fio_http_static_file_response(h,
                                    c->settings->public_folder,
                                    fio_http_path(h)))
    fio___http_on_http_direct(h_, ignr);
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
    p->protocol[i] =
        fio___protocol_callbacks((fio___http_protocol_selector_e)i, 0);
    p->controller[i] =
        fio___controller_callbacks((fio___http_protocol_selector_e)i, 0);
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
            ->controller[FIO___HTTP_PROTOCOL_HTTP1]));
  fio_http_cdata_set(c->queue[c->qwpos], fio_http_connection_dup(c));
  fio_http_method_set(c->queue[c->qwpos], FIO_BUF2STR_INFO(method));
  fio_http_status_set(c->queue[c->qwpos], 200);
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
    http_request_header_set(c->queue[c->qwpos],
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
  http_request_header_add(c->queue[c->qwpos],
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
  http_request_header_add(c->queue[c->qwpos],
                          FIO_BUF2STR_INFO(name),
                          FIO_BUF2STR_INFO(value));
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
The Protocols at play
***************************************************************************** */

/** Returns a facil.io protocol object with the proper protocol callbacks. */
FIO_IFUNC fio_protocol_s
fio___protocol_callbacks(fio___http_protocol_selector_e, int is_client);

/** Returns an http controller object with the proper protocol callbacks. */
FIO_IFUNC fio_http_controller_s
fio___controller_callbacks(fio___http_protocol_selector_e, int is_client);

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
