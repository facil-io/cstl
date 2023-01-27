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
HTTP Protocol Container (vtable + settings storage)
***************************************************************************** */
#define FIO_STL_KEEP__ 1

typedef enum fio___http_protocol_selector_e {
  FIO___HTTP_PROTOCOL_HTTP1 = 0,
  FIO___HTTP_PROTOCOL_WS,
  FIO___HTTP_PROTOCOL_SSE,
  FIO___HTTP_PROTOCOL_HTTP2,
  FIO___HTTP_PROTOCOL_NONE
} fio___http_protocol_selector_e;

/** Returns a facil.io protocol object with the proper protocol callbacks. */
FIO_IFUNC fio_protocol_s
    fio___protocol_callbacks(fio___http_protocol_selector_e);
/** Returns an http controller object with the proper protocol callbacks. */
FIO_IFUNC fio_http_controller_s
    fio___controller_callbacks(fio___http_protocol_selector_e);

typedef struct {
  fio_http_settings_s settings;
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
  size_t state;
  size_t limit;
  union {
    fio_http1_parser_s http;
  } parser;
  fio_http_s *queue[FIO_HTTP_PIPELINE_QUEUE];
  size_t qrpos;
  size_t qwpos;
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
  c->limit = p->settings.max_line_len;
  fio_attach_fd(fd,
                &p->protocol[FIO___HTTP_PROTOCOL_HTTP1],
                (void *)c,
                p->settings.tls);
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
        fio___protocol_callbacks((fio___http_protocol_selector_e)i);
    p->controller[i] =
        fio___controller_callbacks((fio___http_protocol_selector_e)i);
  }
  p->settings = s;
  fio_listen(.url = url,
             .on_open = http___on_open,
             .on_finish = (void (*)(void *))fio_http_protocol_free,
             .udata = (void *)p
             /* , .queue_for_accept = http___queue_for_accept // TODO! */
  );
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
