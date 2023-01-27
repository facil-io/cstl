/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./431 http handle.h" /* Development inclusion - ignore line */
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
#define fio_http_listen(...)                                                   \
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
HTTP Protocol Container (vtable + settings storage)
***************************************************************************** */

#define FIO___HTTP_PROTOCOL_HTTP1 0
#define FIO___HTTP_PROTOCOL_WS    1
#define FIO___HTTP_PROTOCOL_SSE   2
#define FIO___HTTP_PROTOCOL_HTTP2 3

typedef struct {
  http_settings_s settings;
  fio_protocol_s protocol[4];
  fio_http_controller_s controller[4];
} http_protocol_s;
#include FIO_INCLUDE_FILE

#define FIO_REF_NAME             http_protocol
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
    if (o.settings.on_finish)                                                  \
      o.settings.on_finish(&o.settings);                                       \
  } while (0)
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Module Testing
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
