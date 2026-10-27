/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




              HTTP Types - Internal Types, Handle and Core Implementation

See also:
https://www.rfc-editor.org/rfc/rfc9110.html



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP) && !defined(FIO___RECURSIVE_INCLUDE) &&                  \
    !defined(H___FIO_HTTP_TYPES___H) &&                                        \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
#define H___FIO_HTTP_TYPES___H

/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
HTTP Handle State and Controller Flags
***************************************************************************** */

#define FIO___HTTP_STATE_STREAMING      1
#define FIO___HTTP_STATE_FINISHED       2
#define FIO___HTTP_STATE_UPGRADED       4
#define FIO___HTTP_STATE_WEBSOCKET      8
#define FIO___HTTP_STATE_SSE            16
#define FIO___HTTP_STATE_COOKIES_PARSED 32
#define FIO___HTTP_STATE_FREEING        64

/** Controller flags (cflags) for opt-in compression features. */
#define FIO___HTTP_CFLAG_COMPRESS_DYNAMIC 1
#define FIO___HTTP_CFLAG_COMPRESS_WS      2
#define FIO___HTTP_CFLAG_COMPRESS_STATIC  4

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
static void fio___http_default_on_stop(struct fio_http_settings_s *settings) {
  ((void)settings);
}

static void fio___http_default_close(fio_http_s *h) {
  fio_io_close(fio_http_io(h));
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
/** Called when an EventSource event is received. */
static void fio___http_default_on_eventsource_redirect(fio_http_s *h,
                                                       fio_buf_info_s id,
                                                       fio_buf_info_s event,
                                                       fio_buf_info_s data);

/** Called when an EventSource reconnect event requests an ID. */
static void fio___http_default_on_eventsource_reconnect(fio_http_s *h,
                                                        fio_buf_info_s id) {
  (void)h, (void)id;
}

static void fio___http_settings_validate(fio_http_settings_s *s,
                                         int is_client) {
  if (!s->pre_http_body)
    s->pre_http_body = fio___http_default_noop;
  if (!s->on_http)
    s->on_http = is_client ? fio___http_default_noop
                           : fio___http_default_on_http_request;
  if (!s->on_finish)
    s->on_finish = fio___http_default_noop;
  if (!s->on_stop)
    s->on_stop = fio___http_default_on_stop;
  if (!s->on_authenticate_sse)
    s->on_authenticate_sse = is_client ? FIO_HTTP_AUTHENTICATE_ALLOW
                                       : fio___http_default_authenticate;
  if (!s->on_authenticate_websocket)
    s->on_authenticate_websocket = is_client ? FIO_HTTP_AUTHENTICATE_ALLOW
                                             : fio___http_default_authenticate;
  if (!s->on_open)
    s->on_open = fio___http_default_noop;
  if (!s->on_message)
    s->on_message = fio___http_default_on_message;
  if (!s->on_eventsource)
    s->on_eventsource = (s->on_message == fio___http_default_on_message
                             ? fio___http_default_on_eventsource
                             : fio___http_default_on_eventsource_redirect);
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
  if (!s->max_params)
    s->max_params = FIO_HTTP_PARAMS_MAX_FIELDS;
  if (!s->ws_max_msg_size)
    s->ws_max_msg_size = FIO_HTTP_DEFAULT_WS_MAX_MSG_SIZE;
  if (!s->timeout)
    s->timeout = FIO_HTTP_DEFAULT_TIMEOUT;
  if (!s->ws_timeout)
    s->ws_timeout = FIO_HTTP_DEFAULT_TIMEOUT_LONG;
  if (!s->sse_timeout)
    s->sse_timeout = s->ws_timeout;

  if (s->max_header_size < s->max_line_len)
    s->max_header_size = s->max_line_len;

  if (s->public_folder.buf) {
    if (s->public_folder.len > 1 &&
        s->public_folder.buf[s->public_folder.len - 1] == '/' &&
        !(s->public_folder.len == 2 && s->public_folder.buf[0] == '~'))
      --s->public_folder.len;
    if (!fio_filename_is_folder(s->public_folder.buf)) {
      FIO_LOG_ERROR("HTTP public folder not found (or not a folder), setting "
                    "ignored.\n\t%s",
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

/* *****************************************************************************
HTTP Protocol Container (vtable + settings storage)
***************************************************************************** */

typedef union fio___http_router_u {
  fio_http_settings_s s;
  void *ptr[256];
  union fio___http_router_u *map[256];
} fio___http_router_u;

FIO_SFUNC void fio___http_router_destroy(fio___http_router_u *router);

typedef struct {
  fio_http_settings_s settings;
  void (*on_http_callback)(void *, void *);
  fio_queue_s *queue;
  struct {
    fio_io_protocol_s protocol;
    fio_http_controller_s controller;
  } state[FIO___HTTP_PROTOCOL_NONE + 1];
  fio___http_router_u router;
  char public_folder_buf[];
} fio___http_protocol_s;
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

#define FIO_REF_NAME             fio___http_protocol
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
    if (o.settings.tls)                                                        \
      fio_io_tls_free(o.settings.tls);                                         \
    fio___http_router_destroy(&o.router);                                      \
  } while (0)
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

FIO_IFUNC fio___http_protocol_s *fio___http_protocol_init(
    fio___http_protocol_s *p,
    const char *url,
    fio_http_settings_s s,
    bool is_client);
/* *****************************************************************************
HTTP Connection Container
***************************************************************************** */

struct fio___http_connection_http_s {
  void (*on_http_callback)(void *, void *);
  void (*on_http)(fio_http_s *h);
  void (*on_finish)(fio_http_s *h);
  fio_http1_parser_s parser;
  fio_str_info_s buf;
  /* client: discarded 301 / 302 response body bytes (434 http1.h) */
  size_t body_skipped;
  uint32_t max_header;
  uint32_t max_line;
  uint32_t header_bytes;
};
struct fio___http_connection_ws_s {
  void (*on_message)(fio_http_s *h, fio_buf_info_s msg, uint8_t is_text);
  void (*on_ready)(fio_http_s *h);
  fio_websocket_s parser;
  /* Buffered message payload. WebSocket deliveries always use fio_bstr-backed
   * storage for predictable ownership and String semantics. */
  char *msg;
  uint16_t code;
};
struct fio___http_connection_sse_s {
  void (*on_message)(fio_http_s *h,
                     fio_buf_info_s id,
                     fio_buf_info_s event,
                     fio_buf_info_s data);
  void (*on_ready)(fio_http_s *h);
  /* Owned fio_bstr copies (434 sse.h): `id.buf`, `event.buf`, `data`. */
  fio_buf_info_s id;
  fio_buf_info_s event;
  char *data;
  /* The last byte parsed was a line-ending CR: a leading LF completes it. */
  uint8_t skip_lf;
};

/** Connection objects for managing HTTP / WebSocket connection state. */
typedef struct {
  fio_io_s *io;
  fio_http_s *h;
  fio_http_settings_s *settings;
  fio_queue_s *queue;
  void *udata;
  union {
    struct fio___http_connection_http_s http;
    struct fio___http_connection_ws_s ws;
    struct fio___http_connection_sse_s sse;
  } state;
  fio_deflate_s
      *deflate_rd; /* WS decompressor (NULL = no permessage-deflate) */
  fio_deflate_s *deflate_wr; /* WS compressor (NULL = no permessage-deflate) */
  uint32_t len;
  uint32_t capa;
  uint8_t log;
  uint8_t suspend;
  uint8_t is_client;
  uint8_t deflate_rd_reset; /* 1 = client_no_context_takeover */
  uint8_t deflate_wr_reset; /* 1 = server_no_context_takeover */
  char buf[];
} fio___http_connection_s;

/** Selects the task queue for `s` (async queue when running, else IO queue). */
FIO_IFUNC fio_queue_s *fio___http_settings_queue(fio_http_settings_s *s) {
  return ((s->queue && s->queue->q) ? s->queue->q : fio_io_queue());
}

#define FIO_REF_NAME             fio___http_connection
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_FLEX_TYPE        char
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
    if (o.deflate_rd)                                                          \
      fio_deflate_free(o.deflate_rd);                                          \
    if (o.deflate_wr)                                                          \
      fio_deflate_free(o.deflate_wr);                                          \
    fio___http_protocol_free(                                                  \
        FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, o.settings));      \
  } while (0)
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/* *****************************************************************************
HTTP Routing
***************************************************************************** */

FIO_LEAK_COUNTER_DEF(fio___http_router_u)
FIO_LEAK_COUNTER_DEF(fio___http_router_public_folder)
/* static-file on-demand compression scratch (src + dst share the counter;
 * both have multiple free paths). */
FIO_LEAK_COUNTER_DEF(fio___http_static_compress_buf)
FIO_SFUNC void fio___http_on_http_with_public_folder(void *h_, void *ignr);

void fio_http_route___(void);
/** Adds a route prefix to the HTTP handler. */
SFUNC int fio_http_route FIO_NOOP(fio_http_listener_s *l,
                                  const char *url,
                                  fio_http_settings_s s) {
  int err = 0;
  const uint8_t *u = (const uint8_t *)url;
  fio___http_protocol_s *p;
  fio___http_router_u *r;
  if (!l)
    goto invalid_listener_error;
  p = FIO_PTR_FROM_FIELD(fio___http_protocol_s,
                         state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                         fio_io_listener_protocol((fio_io_listener_s *)l));
  r = &p->router;
  if (!u || !u[0]) {
    url = "/";
    u = (const uint8_t *)url;
  }
  /* skip first `/` character, they should always exist anyway */
  u += (*u == (uint8_t)'/');
  /* skip last `/` character, to preserve memory and reduce seeking time */
  for (; *u && (*u != (uint8_t)'/' || u[1]); ++u) {
    if (*u < (sizeof(s) / sizeof(void *)))
      goto invalid_char_error;
    if (!r->map[*u])
      break;
    r = r->map[*u];
  }
  /* skip last `/` character, to preserve memory and reduce seeking time */
  for (; *u && (*u != (uint8_t)'/' || u[1]); ++u) {
    if (*u < (sizeof(s) / sizeof(void *)))
      goto invalid_char_error;
    r->map[*u] =
        (fio___http_router_u *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*r), 0);
    r = r->map[*u];
    FIO_ASSERT_ALLOC(r);
    FIO_LEAK_COUNTER_ON_ALLOC(fio___http_router_u);
    if (!FIO_MEM_REALLOC_IS_SAFE_) {
      FIO_MEMSET(r, 0, sizeof(*r));
    }
  }
  /* we are at the leaf node of the path */

  /* inherit reasonable defaults */
  if (!s.on_finish)
    s.on_finish = p->settings.on_finish;
  if (!s.udata)
    s.udata = p->settings.udata;
  // if (!s.on_stop) /* Do not inherit the `on_stop` callback */
  //   s.on_stop = p->settings.on_stop;
  if (!s.on_authenticate_sse)
    s.on_authenticate_sse = p->settings.on_authenticate_sse;
  if (!s.on_authenticate_websocket)
    s.on_authenticate_websocket = p->settings.on_authenticate_websocket;
  if (!s.max_header_size)
    s.max_header_size = p->settings.max_header_size;
  if (!s.max_line_len)
    s.max_line_len = p->settings.max_line_len;
  if (!s.max_body_size)
    s.max_body_size = p->settings.max_body_size;
  if (!s.max_params)
    s.max_params = p->settings.max_params;
  if (!s.ws_max_msg_size)
    s.ws_max_msg_size = p->settings.ws_max_msg_size;
  if (!s.timeout)
    s.timeout = p->settings.timeout;
  if (!s.ws_timeout)
    s.ws_timeout = p->settings.ws_timeout;
  if (!s.sse_timeout)
    s.sse_timeout = p->settings.sse_timeout;
  if (!s.log)
    s.log = p->settings.log;
  if (!s.compress_static)
    s.compress_static = p->settings.compress_static;
  s.tls = NULL;
  s.tls_io_func = NULL;
  /* validate settings and store */
  fio___http_settings_validate(&s, 0);
  if (!s.public_folder.buf)
    s.public_folder = p->settings.public_folder;
  else if (s.public_folder.buf && s.public_folder.len) {
    s.tls =
        (fio_io_tls_s *)FIO_MEM_REALLOC_(NULL, 0, s.public_folder.len + 1, 0);
    FIO_ASSERT_ALLOC(s.tls);
    FIO_LEAK_COUNTER_ON_ALLOC(fio___http_router_public_folder);
    FIO_MEMCPY(s.tls, s.public_folder.buf, s.public_folder.len);
    s.public_folder = FIO_STR_INFO2((char *)s.tls, s.public_folder.len);
    s.public_folder.buf[s.public_folder.len] = 0;
  }
  /* make sure we're not leaking memory when overwriting an existing route */
  if (r->s.on_http) {
    if (r->s.on_stop) /* routes never inherit `on_stop`: always stop */
      r->s.on_stop(&r->s);
    if (r->s.tls && (char *)r->s.tls == r->s.public_folder.buf) {
      FIO_LEAK_COUNTER_ON_FREE(fio___http_router_public_folder);
      FIO_MEM_FREE_(r->s.tls, r->s.public_folder.len + 1);
    }
  }
  /* if we have a route with a static file service, we need this */
  if (s.public_folder.buf && s.public_folder.len) {
    p->on_http_callback = fio___http_on_http_with_public_folder;
  }
  r->s = s;
  return err;

invalid_char_error:
  if (url)
    FIO_LOG_ERROR("Invalid character found in path URL[%zu]: %s",
                  (size_t)((const char *)u - url),
                  url);
  else
    FIO_LOG_FATAL("HTTP Router requires a non-NULL URL!");
  err = -1;
  return err;

invalid_listener_error:
  FIO_LOG_FATAL("HTTP Router requires an existing HTTP listener object.");
  err = -1;
  return err;
}

void fio___http_route_settings___(void);
FIO_SFUNC fio_http_settings_s *fio___http_route_settings(
    fio___http_router_u *route,
    fio_str_info_s *path) {
  fio_http_settings_s *r = &route->s;
  uint8_t *pos = (uint8_t *)path->buf;
  const uint8_t *n = pos;
  if (!pos || !path->len) /* missing / empty path => root route */
    return r;
  pos += (*pos == (uint8_t)'/');
  if (!*pos)
    return r;
  for (uint8_t c, hi, lo;
       route && ((c = *pos) >= (sizeof(*r) / sizeof(void *)));
       ++pos) {
    /* we have a possible match here - store before stepping into path */
    if (c == (uint8_t)'/' && route->s.on_http) {
      r = &route->s;
      n = pos;
    } else if (c == (uint8_t)'%' && (hi = fio_c2i(pos[1])) < 16) {
      if ((lo = fio_c2i(pos[2])) < 16) { /* decrypt route */
        c = (hi << 4) | lo;
        pos += 2;
        if (c < (sizeof(*r) / sizeof(void *)))
          break;
      }
    }
    route = route->map[c];
  }
  /* test if '/' may be inferred */
  if (!*pos && route && route->map[0]) {
    r = &route->s;
    path->len = 1;
    n = (uint8_t *)path->buf;
  }
  n -= (n == (uint8_t *)path->buf + 1);
  *path = FIO_STR_INFO2((char *)n, path->len - ((char *)n - path->buf));
  return r;
}

/** Returns a link to the settings handled by the handle's route. */
FIO_SFUNC fio_http_settings_s *fio___http_handle_settings(fio_http_s *h) {
  fio_http_settings_s *r = NULL;
  fio___http_connection_s *connection =
      (fio___http_connection_s *)fio_http_cdata(h);
  if (!connection)
    return r;
  fio___http_protocol_s *p =
      FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, connection->settings);
  fio_str_info_s path = fio_http_opath(h);
  r = fio___http_route_settings(&p->router, &path);
  fio_http_udata_set(h, r->udata);
  fio_http_path_set(h, path);
  connection->state.http.on_http = r->on_http;
  connection->state.http.on_finish = r->on_finish;
  return r;
}

FIO_SFUNC void fio___http_router_destroy(fio___http_router_u *r) {
  if (!r)
    return;
  if (r->s.tls && (char *)r->s.tls == r->s.public_folder.buf) {
    FIO_LEAK_COUNTER_ON_FREE(fio___http_router_public_folder);
    FIO_MEM_FREE_(r->s.tls, r->s.public_folder.len + 1);
  }
  for (size_t i = (sizeof(r->s) / sizeof(void *)); i < 256; ++i) {
    if (!r->map[i])
      continue;
    fio___http_router_destroy(r->map[i]);
    FIO_MEM_FREE_(r->map[i], sizeof(*r));
    FIO_LEAK_COUNTER_ON_FREE(fio___http_router_u);
  }
  if (r->s.on_stop)
    r->s.on_stop(&r->s);
}

SFUNC fio_http_settings_s *fio_http_route_settings(fio_http_listener_s *l,
                                                   const char *url) {
  fio_http_settings_s *r = NULL;
  fio___http_protocol_s *p;
  fio_str_info_s path = FIO_STR_INFO2((char *)url, 1);
  if (!l)
    return r;
  p = FIO_PTR_FROM_FIELD(fio___http_protocol_s,
                         state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                         fio_io_listener_protocol((fio_io_listener_s *)l));
  r = fio___http_route_settings(&p->router, &path);
  return r;
}

/* *****************************************************************************
Revisit defaults
***************************************************************************** */

/** Called when an EventSource event is received. */
static void fio___http_default_on_eventsource_redirect(fio_http_s *h,
                                                       fio_buf_info_s id,
                                                       fio_buf_info_s event,
                                                       fio_buf_info_s data) {
  /* Must NOT use `fio___http_handle_settings`: it writes `state.http`, which
   * shares a union with the active `state.sse` (clobbering `on_ready`). */
  fio_http_settings_s *s = fio_http_settings(h);
  if (s)
    s->on_message(h, data, 1);
  (void)h, (void)id, (void)event, (void)data;
}

/* *****************************************************************************
Helpers - reading time
***************************************************************************** */

#if FIO_HTTP_EXACT_LOGGING
#define FIO___HTTP_TIME_DIV  1000000
#define FIO___HTTP_TIME_UNIT "us"
FIO_IFUNC int64_t fio_http_get_timestump(void) {
  return fio_time2micro(fio_time_real());
}
#else
#define FIO___HTTP_TIME_DIV  1000
#define FIO___HTTP_TIME_UNIT "ms"
/* When the IO module is available, use its cached wall-clock ms to avoid a
 * clock_gettime syscall on every HTTP request. Falls back to a direct call
 * when the HTTP handle is used without the IO reactor (e.g., unit tests). */
FIO_IFUNC int64_t fio_http_get_timestump(void) {
#ifdef H___FIO_IO___H
  return fio_io_last_tick_time();
#else
  return fio_time2milli(fio_time_real());
#endif
}
#endif

/* date/time string caching for HTTP date header */
SFUNC fio_str_info_s fio_http_date(uint64_t now_in_seconds) {
  static char date_buf[128];
  static size_t date_len;
  static uint64_t date_buf_val;
  if (date_buf_val == now_in_seconds)
    return FIO_STR_INFO2(date_buf, date_len);
  date_len = fio_time2rfc7231(date_buf, now_in_seconds);
  date_buf[date_len] = 0;
  date_buf_val = now_in_seconds;
  return FIO_STR_INFO2(date_buf, date_len);
}

/* date/time string caching for HTTP logging */
SFUNC fio_str_info_s fio_http_log_time(uint64_t now_in_seconds) {
  static char date_buf[128];
  static size_t date_len;
  static uint64_t date_buf_val;
  if (date_buf_val == now_in_seconds)
    return FIO_STR_INFO2(date_buf, date_len);
  date_len = fio_time2log(date_buf, now_in_seconds);
  date_buf[date_len] = 0;
  date_buf_val = now_in_seconds;
  return FIO_STR_INFO2(date_buf, date_len);
}

/* *****************************************************************************
Helpers - fio_keystr_s memory allocation callbacks
***************************************************************************** */

FIO_LEAK_COUNTER_DEF(fio___http_keystr_allocator)

FIO_SFUNC void fio___http_keystr_free(void *ptr, size_t len) {
  if (!ptr)
    return;
  FIO_LEAK_COUNTER_ON_FREE(fio___http_keystr_allocator);
  FIO_MEM_FREE_(ptr, len);
  (void)len; /* if unused */
}
FIO_SFUNC void *fio___http_keystr_alloc(size_t capa) {
  void *r = FIO_MEM_REALLOC_(NULL, 0, capa, 0);
  if (r) /* count only allocations that will be freed */
    FIO_LEAK_COUNTER_ON_ALLOC(fio___http_keystr_allocator);
  return r;
}

/* *****************************************************************************
Helper Types
***************************************************************************** */
/* *****************************************************************************
String Cache
***************************************************************************** */

#define FIO_MAP_NAME fio___http_str_cache
#define FIO_MAP_LRU  FIO_HTTP_CACHE_LIMIT
#define FIO_MAP_KEY_BSTR
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

static struct {
  fio___http_str_cache_s cache;
  FIO___LOCK_TYPE lock;
} FIO___HTTP_STRING_CACHE[2] = {{.lock = FIO___LOCK_INIT},
                                {.lock = FIO___LOCK_INIT}};
#define FIO___HTTP_STR_CACHE_COOKIE 0
#define FIO___HTTP_STR_CACHE_VALUE  1

#if FIO_HTTP_PRE_CACHE_KNOWN_HEADERS

#define FIO___HTTP_STATIC_CACHE_CAPA_BITS  7
#define FIO___HTTP_STATIC_CACHE_CAPA       (1U << FIO___HTTP_STATIC_CACHE_CAPA_BITS)
#define FIO___HTTP_STATIC_CACHE_MASK       (FIO___HTTP_STATIC_CACHE_CAPA - 1)
#define FIO___HTTP_STATIC_CACHE_STEP_LIMIT 4

typedef struct FIO___HTTP_STATIC_CACHE_T {
  fio___bstr_meta_s meta;
  char str[32];
} FIO___HTTP_STATIC_CACHE_T;
static FIO___HTTP_STATIC_CACHE_T FIO___HTTP_STATIC_CACHE[] = {
#define FIO___HTTP_STATIC_CACHE_SET(s)                                         \
  {                                                                            \
    .meta = {.len = (uint32_t)(sizeof(s) - 1), .ref = ((uint32_t)(~0) >> 8)},  \
    .str = s                                                                   \
  }
    FIO___HTTP_STATIC_CACHE_SET("a-im"),
    FIO___HTTP_STATIC_CACHE_SET("accept"),
    FIO___HTTP_STATIC_CACHE_SET("accept-charset"),
    FIO___HTTP_STATIC_CACHE_SET("accept-datetime"),
    FIO___HTTP_STATIC_CACHE_SET("accept-encoding"),
    FIO___HTTP_STATIC_CACHE_SET("accept-language"),
    FIO___HTTP_STATIC_CACHE_SET("accept-ranges"),
    FIO___HTTP_STATIC_CACHE_SET("access-control-allow-origin"),
    FIO___HTTP_STATIC_CACHE_SET("access-control-request-headers"),
    FIO___HTTP_STATIC_CACHE_SET("access-control-request-method"),
    FIO___HTTP_STATIC_CACHE_SET("age"),
    FIO___HTTP_STATIC_CACHE_SET("allow"),
    FIO___HTTP_STATIC_CACHE_SET("authorization"),
    FIO___HTTP_STATIC_CACHE_SET("cache-control"),
    FIO___HTTP_STATIC_CACHE_SET("connection"),
    FIO___HTTP_STATIC_CACHE_SET("content-disposition"),
    FIO___HTTP_STATIC_CACHE_SET("content-encoding"),
    FIO___HTTP_STATIC_CACHE_SET("content-language"),
    FIO___HTTP_STATIC_CACHE_SET("content-length"),
    FIO___HTTP_STATIC_CACHE_SET("content-location"),
    FIO___HTTP_STATIC_CACHE_SET("content-range"),
    FIO___HTTP_STATIC_CACHE_SET("content-type"),
    FIO___HTTP_STATIC_CACHE_SET("cookie"),
    FIO___HTTP_STATIC_CACHE_SET("date"),
    FIO___HTTP_STATIC_CACHE_SET("dnt"),
    FIO___HTTP_STATIC_CACHE_SET("etag"),
    FIO___HTTP_STATIC_CACHE_SET("expect"),
    FIO___HTTP_STATIC_CACHE_SET("expires"),
    FIO___HTTP_STATIC_CACHE_SET("forwarded"),
    FIO___HTTP_STATIC_CACHE_SET("from"),
    FIO___HTTP_STATIC_CACHE_SET("host"),
    FIO___HTTP_STATIC_CACHE_SET("if-match"),
    FIO___HTTP_STATIC_CACHE_SET("if-modified-since"),
    FIO___HTTP_STATIC_CACHE_SET("if-none-match"),
    FIO___HTTP_STATIC_CACHE_SET("if-range"),
    FIO___HTTP_STATIC_CACHE_SET("if-unmodified-since"),
    FIO___HTTP_STATIC_CACHE_SET("last-modified"),
    FIO___HTTP_STATIC_CACHE_SET("link"),
    FIO___HTTP_STATIC_CACHE_SET("location"),
    FIO___HTTP_STATIC_CACHE_SET("max-forwards"),
    FIO___HTTP_STATIC_CACHE_SET("origin"),
    FIO___HTTP_STATIC_CACHE_SET("pragma"),
    FIO___HTTP_STATIC_CACHE_SET("proxy-authenticate"),
    FIO___HTTP_STATIC_CACHE_SET("proxy-authorization"),
    FIO___HTTP_STATIC_CACHE_SET("range"),
    FIO___HTTP_STATIC_CACHE_SET("referer"),
    FIO___HTTP_STATIC_CACHE_SET("refresh"),
    FIO___HTTP_STATIC_CACHE_SET("retry-after"),
    FIO___HTTP_STATIC_CACHE_SET("sec-ch-ua"),
    FIO___HTTP_STATIC_CACHE_SET("sec-ch-ua-mobile"),
    FIO___HTTP_STATIC_CACHE_SET("sec-ch-ua-platform"),
    FIO___HTTP_STATIC_CACHE_SET("sec-fetch-dest"),
    FIO___HTTP_STATIC_CACHE_SET("sec-fetch-mode"),
    FIO___HTTP_STATIC_CACHE_SET("sec-fetch-site"),
    FIO___HTTP_STATIC_CACHE_SET("sec-fetch-user"),
    FIO___HTTP_STATIC_CACHE_SET("server"),
    FIO___HTTP_STATIC_CACHE_SET("set-cookie"),
    FIO___HTTP_STATIC_CACHE_SET("strict-transport-security"),
    FIO___HTTP_STATIC_CACHE_SET("te"),
    FIO___HTTP_STATIC_CACHE_SET("transfer-encoding"),
    FIO___HTTP_STATIC_CACHE_SET("upgrade"),
    FIO___HTTP_STATIC_CACHE_SET("upgrade-insecure-requests"),
    FIO___HTTP_STATIC_CACHE_SET("user-agent"),
    FIO___HTTP_STATIC_CACHE_SET("vary"),
    FIO___HTTP_STATIC_CACHE_SET("via"),
    FIO___HTTP_STATIC_CACHE_SET("warning"),
    FIO___HTTP_STATIC_CACHE_SET("www-authenticate"),
    FIO___HTTP_STATIC_CACHE_SET("x-csrf-token"),
    FIO___HTTP_STATIC_CACHE_SET("x-forwarded-for"),
    FIO___HTTP_STATIC_CACHE_SET("x-forwarded-host"),
    FIO___HTTP_STATIC_CACHE_SET("x-forwarded-proto"),
    FIO___HTTP_STATIC_CACHE_SET("x-requested-with"),
    {.meta = {0}}};

static uint32_t FIO___HTTP_STATIC_CACHE_IMAP[FIO___HTTP_STATIC_CACHE_CAPA];

static uint64_t fio___http_str_cached_hash(char *str, size_t len) {
  /* use low-case hash specific for the HTTP handle (change resilient) */
  const fio_u256 primes = fio_u256_init64(((uint64_t)0x84B56B93C869EA0F),
                                          ((uint64_t)0x613A19F5CB0D98D5),
                                          ((uint64_t)0x48644F7B3959621F),
                                          ((uint64_t)0x39664DEECA23D825));
  uint64_t hash = 0;
  if (len > 32)
    return hash;
  fio_u512 s = {0};
  fio_memcpy63x(s.u8, str, len);
  for (size_t i = 0; i < 4; ++i)
    s.u64[i] = fio_ltole64(s.u64[i]);
#if 0
  fio_u256_cor64(s.u256, s.u256, 0x2020202020202020ULL); /* lower case hash */
  fio_u256_cadd16(s.u256, s.u256, len);
  for (size_t i = 0; i < 4; ++i)
    s.u64[i] = fio_math_mulc64(s.u64[i], primes.u64[i], s.u64 + 4 + i);
  hash = fio_u256_reduce_add64(s.u256 + 1);
  hash ^= fio_u256_reduce_add64(s.u256);
  fio_u256_cxor64(s.u256, s.u256, hash);
  hash += fio_u256_reduce_add64(s.u256);
#else
  FIO_MATH_UXXX_COP(s.u256[0].x64, s.u256[0].x64, 0x2020202020202020ULL, 64, |);
  FIO_MATH_UXXX_COP(s.u256[0].x16, s.u256[0].x16, (uint16_t)len, 16, +);
  for (size_t i = 0; i < 4; ++i) {
    s.u64[i] = fio_math_mulc64(s.u64[i], primes.u64[i], s.u64 + 4 + i);
  }
  uint64_t tmp;
  FIO_MATH_UXXX_REDUCE(hash, s.u256[1].u64, 64, +);
  FIO_MATH_UXXX_REDUCE(tmp, s.u256[0].u64, 64, +);
  hash ^= tmp;
  FIO_MATH_UXXX_COP(s.u256[0].x64, s.u256[0].x64, hash, 64, ^);
  FIO_MATH_UXXX_REDUCE(tmp, s.u256[0].u64, 64, +);
  hash += tmp;
#endif
  // hash += hash >> 4;
  hash += hash >> 5;
  return hash;
}

#ifndef FIO___HTTP_STATIC_CACHE_CMP_SECURE
#define FIO___HTTP_STATIC_CACHE_CMP_SECURE 0
#endif
static bool fio___http_str_cached_cmp(void *arry, void *obj, uint32_t indx) {
  FIO___HTTP_STATIC_CACHE_T *ary = (FIO___HTTP_STATIC_CACHE_T *)arry;
  if (indx >
      ((sizeof(FIO___HTTP_STATIC_CACHE) / sizeof(FIO___HTTP_STATIC_CACHE[0])) -
       1))
    return 0;
  fio_buf_info_s *s = (fio_buf_info_s *)obj;
  fio_buf_info_s t = FIO_BUF_INFO2(ary[indx].str, ary[indx].meta.len);
#if FIO___HTTP_STATIC_CACHE_CMP_SECURE
  return FIO_BUF_INFO_IS_EQ((*s), t);
#else
  return s[0].len == t.len && s[0].buf[0] == (t.buf[0] | 32);
#endif
}
#undef FIO___HTTP_STATIC_CACHE_CMP_SECURE

static char *fio___http_str_cached_static(char *str, size_t len) {
  if (len >= 32)
    return NULL;
  fio_buf_info_s obj = FIO_BUF_INFO2(str, len);
  uint64_t hash = fio___http_str_cached_hash(obj.buf, obj.len);
  fio___imap32_seeker_s pos =
      fio___imap32_seek((void *)FIO___HTTP_STATIC_CACHE,
                        FIO___HTTP_STATIC_CACHE_IMAP,
                        FIO___HTTP_STATIC_CACHE_CAPA_BITS,
                        (void *)&obj,
                        (uint32_t)hash,
                        fio___http_str_cached_cmp,
                        FIO___HTTP_STATIC_CACHE_STEP_LIMIT);
  if (!pos.is_valid)
    return NULL;
  ++FIO___HTTP_STATIC_CACHE[pos.pos].meta.ref;
  return FIO___HTTP_STATIC_CACHE[pos.pos].str;
}

static void fio___http_str_cached_init(void) {
  FIO_MEMSET(FIO___HTTP_STATIC_CACHE_IMAP,
             0,
             sizeof(FIO___HTTP_STATIC_CACHE_IMAP));

  for (size_t i = 0; FIO___HTTP_STATIC_CACHE[i].meta.ref; ++i) {
    fio_buf_info_s obj = FIO_BUF_INFO2(FIO___HTTP_STATIC_CACHE[i].str,
                                       FIO___HTTP_STATIC_CACHE[i].meta.len);
    uint64_t hash = fio___http_str_cached_hash(obj.buf, obj.len);
    fio___imap32_seeker_s pos =
        fio___imap32_seek((void *)FIO___HTTP_STATIC_CACHE,
                          FIO___HTTP_STATIC_CACHE_IMAP,
                          FIO___HTTP_STATIC_CACHE_CAPA_BITS,
                          (void *)&obj,
                          (uint32_t)hash,
                          fio___http_str_cached_cmp,
                          FIO___HTTP_STATIC_CACHE_STEP_LIMIT);
    FIO_ASSERT(!pos.is_valid && pos.ipos < FIO___HTTP_STATIC_CACHE_CAPA &&
                   !FIO___HTTP_STATIC_CACHE_IMAP[pos.ipos],
               "HTTP static cache collision / overflow @ %zu (%s)",
               i,
               obj.buf);

    pos.set_val |= i;
    fio___imap32_set(FIO___HTTP_STATIC_CACHE_IMAP, pos.ipos, pos.set_val);

    FIO_ASSERT(fio___http_str_cached_static(obj.buf, obj.len) == obj.buf,
               "HTTP static cache initialization round-trip error @ %zu (%s)",
               i,
               obj.buf);
  }
}

#undef FIO___HTTP_STATIC_CACHE_CAPA_BITS
#undef FIO___HTTP_STATIC_CACHE_CAPA
#undef FIO___HTTP_STATIC_CACHE_MASK
#undef FIO___HTTP_STATIC_CACHE_STEP_LIMIT
#else
#define fio___http_str_cached_init() (void)0
#endif /* FIO_HTTP_PRE_CACHE_KNOWN_HEADERS */

FIO_IFUNC char *fio___http_str_cached_inner(size_t group,
                                            uint64_t hash,
                                            fio_str_info_s s) {
#if !FIO_HTTP_CACHE_LIMIT
  return fio_bstr_write(NULL, s.buf, s.len);
#endif
  char *r;
  fio___http_str_cache_s *c = &FIO___HTTP_STRING_CACHE[group].cache;
  hash ^= (uint64_t)(uintptr_t)fio_http_new;
#if FIO_HTTP_CACHE_USES_MUTEX
  FIO___LOCK_LOCK(FIO___HTTP_STRING_CACHE[group].lock);
#endif
  r = fio___http_str_cache_set_if_missing(c, hash, s).buf;
  /* reference the entry before anything (eviction / other threads) drops it */
  r = fio_bstr_copy(r);
  /* enforce FIO_HTTP_CACHE_LIMIT: insertion ordered, so evict the oldest */
  if (FIO_UNLIKELY(fio___http_str_cache_count(c) > FIO_HTTP_CACHE_LIMIT))
    fio___http_str_cache_evict(c,
                               fio___http_str_cache_count(c) -
                                   FIO_HTTP_CACHE_LIMIT);
#if FIO_HTTP_CACHE_USES_MUTEX
  FIO___LOCK_UNLOCK(FIO___HTTP_STRING_CACHE[group].lock);
#endif
  return r;
}
FIO_IFUNC char *fio___http_str_cached(size_t group, fio_str_info_s s) {
#if !FIO_HTTP_CACHE_LIMIT
  return fio_bstr_write(NULL, s.buf, s.len);
#endif
  if (!s.len)
    return NULL;
  if (s.len > FIO_HTTP_CACHE_STR_MAX_LEN)
    goto avoid_caching;
  return fio___http_str_cached_inner(group, fio_risky_hash(s.buf, s.len, 0), s);
avoid_caching:
  return fio_bstr_write(NULL, s.buf, s.len);
}

FIO_IFUNC char *fio___http_str_cached_with_static(fio_str_info_s s) {
#if FIO_HTTP_PRE_CACHE_KNOWN_HEADERS
  char *tmp;
  if (!s.len)
    return NULL;
  if (s.len > FIO_HTTP_CACHE_STR_MAX_LEN)
    goto skip_cache_test;
  tmp = fio___http_str_cached_static(s.buf, s.len);
  if (tmp) {
    FIO_LOG_DDEBUG2("(%d) using statically cached header: %s",
                    fio_io_pid(),
                    tmp);
    return tmp; /* reference count increased by fio___http_str_cached_static */
  }
skip_cache_test:
#endif /* FIO_HTTP_PRE_CACHE_KNOWN_HEADERS */
  return fio_bstr_write(NULL, s.buf, s.len);
}

/* *****************************************************************************
Headers Maps
***************************************************************************** */

#define FIO_ARRAY_NAME              fio___http_sary
#define FIO_ARRAY_TYPE              char *
#define FIO_ARRAY_TYPE_DESTROY(obj) fio_bstr_free(obj)

#define FIO_MAP_NAME                 fio___http_hmap
#define FIO_MAP_KEY                  fio_str_info_s
#define FIO_MAP_KEY_INTERNAL         char *
#define FIO_MAP_KEY_FROM_INTERNAL(k) fio_bstr_info((k))
#define FIO_MAP_KEY_CMP(a, b)        fio_bstr_is_eq2info((a), (b))
#define FIO_MAP_KEY_DESTROY(key)     fio_bstr_free((key))
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  (dest) = fio___http_str_cached_with_static((src))
#define FIO_MAP_KEY_DISCARD(key)
#define FIO_MAP_VALUE fio___http_sary_s
#define FIO_MAP_VALUE_COPY(a, b)                                               \
  do {                                                                         \
    (a) = (fio___http_sary_s)FIO_ARRAY_INIT;                                   \
    (void)(b);                                                                 \
  } while (0) /*no-op*/
#define FIO_MAP_VALUE_DESTROY(o) fio___http_sary_destroy(&(o))
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)fio___http_sary_destroy)
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

#if FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
#define FIO___HTTP_ENFORCE_LOWERCASE(var_name, inpute_var)                     \
  FIO_STR_INFO_TMP_VAR(var_name, 4096);                                        \
  fio___http_hmap_key_to_lower(&var_name, &inpute_var);

/** Converts a Header key to lower-case */
FIO_IFUNC void fio___http_hmap_key_to_lower(fio_str_info_s *t,
                                            fio_str_info_s *k) {
  if (k->len >= t->capa)
    goto too_big;
  for (size_t i = 0; i < k->len; ++i) {
    uint8_t c = (uint8_t)k->buf[i];
    c |= (uint8_t)(c >= 'A' && c <= 'Z') << 5;
    t->buf[i] = c;
  }
  t->len = k->len;
  return;
too_big:
  *t = *k;
}

#else
#define FIO___HTTP_ENFORCE_LOWERCASE(var_name, inpute_var)                     \
  fio_str_info_s var_name = inpute_var;
#endif

/** set `add` to positive to add multiple values or negative to overwrite. */
FIO_IFUNC fio_str_info_s fio___http_hmap_set2(fio___http_hmap_s *map,
                                              fio_str_info_s key_input,
                                              fio_str_info_s val,
                                              int add) {
  fio_str_info_s r = {0};
  if (!key_input.buf || !key_input.len || !map || key_input.len > 4095)
    return r;
#if FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
  FIO_STR_INFO_TMP_VAR(key, 4096);
  key.len = key_input.len;
#else
  fio_str_info_s key = key_input;
#endif
  /* Validate and, when enabled, normalize in the same bounded pass. */
  for (size_t i = 0; i < key_input.len; ++i) {
    const uint8_t c = (uint8_t)key_input.buf[i];
    if (FIO_UNLIKELY(FIO___HTTP_FORBIDDEN_NAME_CHARS[c]))
      return r;
#if FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
    key.buf[i] = fio_ct_tolower((char)c);
#endif
  }
  /* Reject injection before removing/replacing or caching any data. */
  if (val.buf) {
    for (size_t i = 0; i < val.len; ++i) {
      const uint8_t c = (uint8_t)val.buf[i];
      if (FIO_UNLIKELY(c == 0 || c == '\r' || c == '\n'))
        return r;
    }
  }
  fio___http_sary_s *o;
  if (!val.buf || !val.len)
    goto remove_key;
  o = fio___http_hmap_node2val_ptr(fio___http_hmap_get_ptr(map, key));
  if (!o) {
    fio___http_sary_s va = {0};
    o = fio___http_hmap_node2val_ptr(
        fio___http_hmap_set_ptr(map, key, va, NULL, 1));
    add = 1;
  }
  if (FIO_UNLIKELY(!o)) {
    FIO_LOG_ERROR("Couldn't add value to header: %.*s:%.*s",
                  (int)key.len,
                  key.buf,
                  (int)val.len,
                  val.buf);
    return r;
  }
  if (add) {
    if (add < 0) {
      fio___http_sary_destroy(o);
    }
    char *v = fio___http_str_cached(FIO___HTTP_STR_CACHE_VALUE, val);
    /* never store an empty (failed) value: it would be sent as `name:` */
    if (FIO_UNLIKELY(!v || !fio___http_sary_push(o, v))) { /* push frees v */
      FIO_LOG_ERROR("(%d) no memory for header value: %.*s",
                    fio_io_pid(),
                    (int)key.len,
                    key.buf);
      /* an empty key must not mask a missing header (e.g. framing) */
      if (!fio___http_sary_count(o))
        fio___http_hmap_remove(map, key, NULL);
      return r;
    }
    r = fio_bstr_info(v);
    return r;
  }
  r = fio_bstr_info(fio___http_sary_get(o, -1));
  return r;

remove_key:
  if (add < 0) {
    fio___http_hmap_remove(map, key, NULL);
    return r;
  }
  if (!add) { /* set_if_missing: an empty value returns the existing value */
    o = fio___http_hmap_node2val_ptr(fio___http_hmap_get_ptr(map, key));
    if (o && fio___http_sary_count(o))
      r = fio_bstr_info(fio___http_sary_get(o, -1));
  }
  return r;
}

FIO_IFUNC fio_str_info_s fio___http_hmap_get2(fio___http_hmap_s *map,
                                              fio_str_info_s key_input,
                                              int32_t index) {
  fio_str_info_s r = {0};
  if (key_input.len > 4095)
    return r;
  FIO___HTTP_ENFORCE_LOWERCASE(key, key_input);
  fio___http_sary_s *a =
      fio___http_hmap_node2val_ptr(fio___http_hmap_get_ptr(map, key));
  if (!a)
    return r;
  const uint32_t count = fio___http_sary_count(a);
  if (!count)
    return r;
  if (index < 0) {
    index += count;
    if (index < 0)
      index = 0;
  }
  if ((uint32_t)index >= count)
    return r;
  r = fio_bstr_info(fio___http_sary_get(a, index));
  return r;
}

FIO_IFUNC size_t fio___http_hmap_count2(fio___http_hmap_s *map,
                                        fio_str_info_s key) {
  size_t r = 0;
  fio___http_sary_s *a =
      fio___http_hmap_node2val_ptr(fio___http_hmap_get_ptr(map, key));
  if (!a)
    return r;
  r = fio___http_sary_count(a);
  return r;
}

/* *****************************************************************************
Header iteration Task
***************************************************************************** */

typedef struct {
  fio_http_s *h;
  int (*callback)(fio_http_s *, fio_str_info_s, fio_str_info_s, void *);
  void *udata;
} fio___http_hmap_each_info_s;

FIO_SFUNC int fio___http_h_each_task_wrapper(fio___http_hmap_each_s *e) {
  fio___http_hmap_each_info_s *data = (fio___http_hmap_each_info_s *)(e->udata);
  FIO_ARRAY_EACH(fio___http_sary, &e->value, pos) {
    if (data->callback(data->h, e->key, fio_bstr_info(*pos.p), data->udata) ==
        -1)
      return -1;
  }
  return 0;
}

/* *****************************************************************************
Cookie Maps
***************************************************************************** */

#define FIO_MAP_NAME                 fio___http_cmap /* cached names */
#define FIO_MAP_KEY                  fio_str_info_s
#define FIO_MAP_KEY_INTERNAL         char *
#define FIO_MAP_KEY_FROM_INTERNAL(k) fio_bstr_info((k))
#define FIO_MAP_KEY_CMP(a, b)        fio_bstr_is_eq2info((a), (b))
#define FIO_MAP_KEY_DESTROY(key)     fio_bstr_free((key))
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  (dest) = fio___http_str_cached(FIO___HTTP_STR_CACHE_COOKIE, (src))
#define FIO_MAP_KEY_DISCARD(key)

#define FIO_MAP_VALUE_BSTR /* not cached */
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)fio___http_cmap_destroy)
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/* *****************************************************************************
Controller Validation
***************************************************************************** */

FIO_SFUNC int fio___mock_controller_get_fd_cb(fio_http_s *h) {
  return -1;
  (void)h;
}
FIO_SFUNC void fio___mock_controller_cb(fio_http_s *h) { (void)h; }
FIO_SFUNC void fio___mock_c_write_body(fio_http_s *h,
                                       fio_http_write_args_s args) {
  if (args.buf) {
    if (args.dealloc)
      args.dealloc((void *)args.buf);
  } else if ((unsigned)(args.fd + 1) > 1U && !args.copy &&
             args.fd != fio_http_body_fd(h)) {
    close(args.fd);
  }
  (void)h;
}

static fio_http_controller_s FIO___MOCK_CONTROLLER = {
    .on_destroyed = fio___mock_controller_cb,
    .send_headers = fio___mock_controller_cb,
    .write_body = fio___mock_c_write_body,
    .on_finish = fio___mock_controller_cb,
    .close_io = fio___mock_controller_cb,
    .get_fd = fio___mock_controller_get_fd_cb,
};

SFUNC fio_http_controller_s *fio___http_controller_validate(
    fio_http_controller_s *c) {
  if (!c)
    c = &FIO___MOCK_CONTROLLER;
  if (c->private_flags)
    return c;
  c->private_flags = 256;
  if (!c->on_destroyed)
    c->on_destroyed = fio___mock_controller_cb;
  if (!c->send_headers)
    c->send_headers = fio___mock_controller_cb;
  if (!c->write_body)
    c->write_body = fio___mock_c_write_body;
  if (!c->on_finish)
    c->on_finish = fio___mock_controller_cb;
  if (!c->close_io)
    c->close_io = fio___mock_controller_cb;
  if (!c->get_fd)
    c->get_fd = fio___mock_controller_get_fd_cb;
  return c;
}

/* *****************************************************************************
HTTP Handle Type
***************************************************************************** */

FIO_SFUNC int fio____http_write_start(fio_http_s *, fio_http_write_args_s *);
FIO_SFUNC int fio____http_write_cont(fio_http_s *, fio_http_write_args_s *);
FIO_SFUNC int fio___http_mime_is_compressible(fio_str_info_s);
FIO_SFUNC int fio___http_header_has_token(fio_str_info_s, const char *, size_t);

struct fio_http_s {
  void *udata;
  void *udata2;
  void *cdata;
  fio_http_controller_s *controller;
  int (*writer)(fio_http_s *, fio_http_write_args_s *);
  int64_t received_at;
  size_t sent;
  uint32_t cflags;
  uint16_t state;
  uint16_t status;
  fio_keystr_s method;
  fio_keystr_s opath;
  fio_keystr_s path;
  fio_keystr_s query;
  fio_keystr_s version;
  fio___http_hmap_s headers[2]; /* request, response */
  fio___http_cmap_s cookies[2]; /* read, write */
  fio_data_s body;
};
FIO_ASSERT_STATIC(offsetof(fio_http_s, udata) == 0 &&
                      offsetof(fio_http_s, udata2) == sizeof(void *) &&
                      offsetof(fio_http_s, cdata) == 2 * sizeof(void *) &&
                      offsetof(fio_http_s, controller) == 3 * sizeof(void *) &&
                      sizeof(fio_http_controller_s *) == sizeof(void *),
                  "fio_http_s pointer prefix layout changed (430 accessors)");

#define HTTP_HDR_REQUEST(h)  (h->headers + 0)
#define HTTP_HDR_RESPONSE(h) (h->headers + 1)

#define FIO_REF_NAME fio_http
#define FIO_REF_INIT(h)                                                        \
  h = (fio_http_s) {                                                           \
    .controller = &FIO___MOCK_CONTROLLER, .writer = fio____http_write_start,   \
    .received_at = fio_http_get_timestump(),                                   \
    .body = {.fd = -1, .limit = FIO_HTTP_BODY_RAM_LIMIT},                      \
  }
#define FIO_REF_DESTROY(h) fio_http_destroy(&(h))
SFUNC fio_http_s *fio_http_destroy(fio_http_s *h) {
  if (!h)
    return h;
  h->state |= FIO___HTTP_STATE_FREEING;
  h->controller->on_destroyed(h);

  fio_keystr_destroy(&h->method, fio___http_keystr_free);
  fio_keystr_destroy(&h->opath, fio___http_keystr_free);
  fio_keystr_destroy(&h->path, fio___http_keystr_free);
  fio_keystr_destroy(&h->query, fio___http_keystr_free);
  fio_keystr_destroy(&h->version, fio___http_keystr_free);
  fio___http_hmap_destroy(h->headers);
  fio___http_hmap_destroy(h->headers + 1);
  fio___http_cmap_destroy(h->cookies);
  fio___http_cmap_destroy(h->cookies + 1);
  fio_data_destroy(&h->body);
  FIO_REF_INIT(*h);
  return h;
}
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/** Clears any response data. */
SFUNC fio_http_s *fio_http_clear_response(fio_http_s *h, bool clear_body) {
  fio___http_hmap_destroy(HTTP_HDR_RESPONSE(h));
  h->state = 0;
  h->writer = fio____http_write_start;
  h->received_at = fio_http_get_timestump();
  h->status = 0;
  if (!clear_body)
    return h;
  fio_data_destroy(&h->body);
  return h;
}

/** Create a new http_s handle. */
SFUNC fio_http_s *fio_http_new(void) { return fio_http_new2(); }

/** Reduces an http_s handle's reference count or frees it. */
SFUNC void fio_http_free(fio_http_s *h) { fio_http_free2(h); }

/** Increases an http_s handle's reference count. */
SFUNC fio_http_s *fio_http_dup(fio_http_s *h) { return fio_http_dup2(h); }

/** Collects an updated timestamp for logging purposes. */
SFUNC void fio_http_start_time_set(fio_http_s *h) {
  h->received_at = fio_http_get_timestump();
}

/** Closes the associated connection after previously scheduled output drains.
 */
SFUNC void fio_http_close(fio_http_s *h) { h->controller->close_io(h); }

/** Creates a copy of an existing handle, copying only its request data. */
SFUNC fio_http_s *fio_http_new_copy_request(fio_http_s *o) {
  fio_http_s *h = fio_http_new();
  FIO_ASSERT_ALLOC(h);
  fio_http_opath_set(h, fio_http_opath(o));
  fio_http_path_set(h, fio_http_path(o));
  fio_http_method_set(h, fio_http_method(o));
  fio_http_query_set(h, fio_http_query(o));
  fio_http_version_set(h, fio_http_version(o));
  /* copy headers */
  fio___http_hmap_reserve(h->headers, fio___http_hmap_count(o->headers));
  FIO_MAP_EACH(fio___http_hmap, o->headers, i) {
    fio___http_sary_s *a = fio___http_hmap_node2val_ptr(
        fio___http_hmap_set_ptr(h->headers,
                                i.key,
                                (fio___http_sary_s){0},
                                NULL,
                                0));
    FIO_ARRAY_EACH(fio___http_sary, &i.value, v) {
      fio___http_sary_push(a, fio_bstr_copy(*v.p));
    }
  }
  /* copy cookies */
  FIO_MAP_EACH(fio___http_cmap, o->cookies, i) {
    fio___http_cmap_set(h->cookies, i.key, i.value, NULL);
  }
  return h;
}

/* *****************************************************************************
Simple Property Set / Get
***************************************************************************** */

#define FIO___HTTP_MAKE_GET_SET(property)                                      \
  SFUNC fio_str_info_s fio_http_##property(fio_http_s *h) {                    \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    if (!h)                                                                    \
      return FIO_STR_INFO0;                                                    \
    return fio_keystr_info(&h->property);                                      \
  }                                                                            \
                                                                               \
  SFUNC fio_str_info_s fio_http_##property##_set(fio_http_s *h,                \
                                                 fio_str_info_s value) {       \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    if (!h)                                                                    \
      return FIO_STR_INFO0;                                                    \
    fio_keystr_s old = h->property; /* delay destroy in case of copy/edit. */  \
    h->property = fio_keystr_init(value, fio___http_keystr_alloc);             \
    fio_keystr_destroy(&old, fio___http_keystr_free);                          \
    return fio_keystr_info(&h->property);                                      \
  }

FIO___HTTP_MAKE_GET_SET(method)
FIO___HTTP_MAKE_GET_SET(path)
FIO___HTTP_MAKE_GET_SET(opath)
FIO___HTTP_MAKE_GET_SET(query)
FIO___HTTP_MAKE_GET_SET(version)

#undef FIO___HTTP_MAKE_GET_SET

FIO_DEF_GET_FUNC(SFUNC, fio_http, fio_http_s, size_t, status)
/* clang-format off */
FIO_DEF_GETSET_FUNC(SFUNC, fio_http, fio_http_s, int64_t, received_at, FIO_NOOP_FN)
/* clang-format on */

/** Sets the status associated with the HTTP handle (response). */
SFUNC size_t fio_http_status_set(fio_http_s *h, size_t status) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  if (status > 1023)
    status = 500;
  if (!status)
    status = 200;
  return (h->status = (uint16_t)status);
}

/* *****************************************************************************
Controller / User Flags Set / Test / Flip
***************************************************************************** */

#define FIO___HTTP_MAKE_GET_SET(flags)                                         \
  /** Returns all set flags */                                                 \
  FIO_IFUNC uint16_t fio___http_##flags(fio_http_s *h) {                       \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    return h->flags;                                                           \
  }                                                                            \
  FIO_IFUNC uint16_t fio___http_##flags##_flip(fio_http_s *h, uint16_t flg) {  \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    h->flags ^= flg;                                                           \
    return h->flags & flg;                                                     \
  }                                                                            \
  FIO_IFUNC uint16_t fio___http_##flags##_set(fio_http_s *h, uint16_t flg) {   \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    h->flags |= flg;                                                           \
    return h->flags & flg;                                                     \
  }                                                                            \
  FIO_IFUNC uint16_t fio___http_##flags##_unset(fio_http_s *h, uint16_t flg) { \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    h->flags &= ~flg;                                                          \
    return h->flags & flg;                                                     \
  }                                                                            \
  FIO_IFUNC uint16_t fio___http_##flags##_is_set(fio_http_s *h,                \
                                                 uint16_t flg) {               \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    return h->flags & flg;                                                     \
  }

FIO___HTTP_MAKE_GET_SET(cflags)

/* Test-only seam for detached handles: production callers configure
 * compression via listener/route settings, not controller flags. */
#if defined(FIO_HTTP_TEST_COMPRESSION_FLAGS)
FIO_IFUNC void fio___http_test_enable_compression(fio_http_s *h,
                                                  int is_static) {
  fio___http_cflags_set(h,
                        is_static ? FIO___HTTP_CFLAG_COMPRESS_STATIC
                                  : FIO___HTTP_CFLAG_COMPRESS_DYNAMIC);
}
#endif

#undef FIO___HTTP_MAKE_GET_SET
/* *****************************************************************************
Handler State
***************************************************************************** */

SFUNC int fio_http_is_clean(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  /* headers sent (e.g., preset `content-length`, no finish) = not clean */
  return !(h->state & (~FIO___HTTP_STATE_COOKIES_PARSED)) &&
         h->writer == fio____http_write_start;
}

/** Returns true if the HTTP handle's response was sent. */
SFUNC int fio_http_is_finished(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO___HTTP_STATE_FINISHED));
}

/** Returns true if handle is in the process of freeing itself. */
SFUNC int fio_http_is_freeing(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO___HTTP_STATE_FREEING));
}

/** Returns true if the HTTP handle's response is streaming. */
SFUNC int fio_http_is_streaming(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO___HTTP_STATE_STREAMING));
}

/** Returns true if the HTTP connection was (or should have been) upgraded. */
SFUNC int fio_http_is_upgraded(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO___HTTP_STATE_UPGRADED));
}

/** Returns true if the HTTP handle establishes a WebSocket Upgrade. */
SFUNC int fio_http_is_websocket(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO___HTTP_STATE_WEBSOCKET));
}

/** Returns true if the HTTP handle establishes an EventSource connection. */
SFUNC int fio_http_is_sse(fio_http_s *h) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");
  return (!!(h->state & FIO___HTTP_STATE_SSE));
}

/* *****************************************************************************
Method Name Matching
***************************************************************************** */

SFUNC int fio_http_method_is_get(fio_http_s *h);  /* IDE Marker*/
SFUNC int fio_http_method_is_post(fio_http_s *h); /* IDE Marker*/
SFUNC int fio_http_method_is_head(fio_http_s *h); /* IDE Marker*/
SFUNC int fio_http_method_is_put(fio_http_s *h);  /* IDE Marker*/

/* Define test for lengths 3 and 4 */
#define FIO___HTTP_DEF_METHOD_TEST(m, l)                                       \
  /** Tests if fio_http_method is m (case insensitive). */                     \
  SFUNC int fio_http_method_is_##m(fio_http_s *h) {                            \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    fio_str_info_s method = fio_http_method(h);                                \
    if (method.len != l)                                                       \
      return 0;                                                                \
    const uint32_t tst = fio_buf2u32u(#m) | 0x20202020U;                       \
    const uint32_t is = fio_buf2u32u(method.buf) | 0x20202020U;                \
    return (tst == is);                                                        \
  }

FIO___HTTP_DEF_METHOD_TEST(copy, 4)
FIO___HTTP_DEF_METHOD_TEST(get, 3)
FIO___HTTP_DEF_METHOD_TEST(head, 4)
FIO___HTTP_DEF_METHOD_TEST(lock, 4)
FIO___HTTP_DEF_METHOD_TEST(move, 4)
FIO___HTTP_DEF_METHOD_TEST(post, 4)
FIO___HTTP_DEF_METHOD_TEST(put, 3)

#undef FIO___HTTP_DEF_METHOD_TEST

/* Define test for length 5 */
#define FIO___HTTP_DEF_METHOD_TEST(m, l)                                       \
  /** Returns 1 if fio_http_method is m (case insensitive). */                 \
  SFUNC int fio_http_method_is_##m(fio_http_s *h) {                            \
    static const char *name = #m;                                              \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    fio_str_info_s method = fio_http_method(h);                                \
    if (method.len != l)                                                       \
      return 0;                                                                \
    if ((method.buf[0] | 0x20) != name[0])                                     \
      return 0;                                                                \
    const uint32_t tst = fio_buf2u32u(name + 1) | 0x20202020U;                 \
    const uint32_t is = fio_buf2u32u(method.buf + 1) | 0x20202020U;            \
    return (tst == is);                                                        \
  }

FIO___HTTP_DEF_METHOD_TEST(patch, 5)
FIO___HTTP_DEF_METHOD_TEST(query, 5)
FIO___HTTP_DEF_METHOD_TEST(trace, 5)
FIO___HTTP_DEF_METHOD_TEST(mkcol, 5)

#undef FIO___HTTP_DEF_METHOD_TEST

/* Define test for lengths 5-8 */
#define FIO___HTTP_DEF_METHOD_TEST(m, l)                                       \
  /** Returns 1 if fio_http_method is m (case insensitive). */                 \
  SFUNC int fio_http_method_is_##m(fio_http_s *h) {                            \
    static const char *name = #m;                                              \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    fio_str_info_s method = fio_http_method(h);                                \
    if (method.len != l)                                                       \
      return 0;                                                                \
    const uint32_t tst1 = fio_buf2u32u(name) | 0x20202020U;                    \
    const uint32_t is1 = fio_buf2u32u(method.buf) | 0x20202020U;               \
    const uint32_t tst2 = fio_buf2u32u(name + (l - 4)) | 0x20202020U;          \
    const uint32_t is2 = fio_buf2u32u(method.buf + (l - 4)) | 0x20202020U;     \
    return !((tst1 ^ is1) | (tst2 ^ is2));                                     \
  }

FIO___HTTP_DEF_METHOD_TEST(search, 6)
FIO___HTTP_DEF_METHOD_TEST(delete, 6)
FIO___HTTP_DEF_METHOD_TEST(options, 7)
FIO___HTTP_DEF_METHOD_TEST(connect, 7)
FIO___HTTP_DEF_METHOD_TEST(unlock, 6)
FIO___HTTP_DEF_METHOD_TEST(propfind, 8)

#undef FIO___HTTP_DEF_METHOD_TEST

/* Define test for lengths 9-16 */
#define FIO___HTTP_DEF_METHOD_TEST(m, l)                                       \
  /** Returns 1 if fio_http_method is m (case insensitive). */                 \
  SFUNC int fio_http_method_is_##m(fio_http_s *h) {                            \
    static const char *name = #m;                                              \
    FIO_ASSERT_DEBUG(h, "NULL HTTP handler!");                                 \
    fio_str_info_s method = fio_http_method(h);                                \
    if (method.len != l)                                                       \
      return 0;                                                                \
    const uint64_t tst1 =                                                      \
        fio_buf2u64u(name) | (uint64_t)0x2020202020202020ULL;                  \
    const uint64_t is1 =                                                       \
        fio_buf2u64u(method.buf) | (uint64_t)0x2020202020202020ULL;            \
    const uint32_t tst2 = fio_buf2u32u(name + (l - 8)) | 0x20202020U;          \
    const uint32_t is2 = fio_buf2u32u(method.buf + (l - 8)) | 0x20202020U;     \
    return !((tst1 ^ is1) | (uint64_t)(tst2 ^ is2));                           \
  }

FIO___HTTP_DEF_METHOD_TEST(proppatch, 9)

#undef FIO___HTTP_DEF_METHOD_TEST

/* *****************************************************************************
Header Data Management
***************************************************************************** */

#define FIO___HTTP_HEADER_SET_FN(category, name_, headers, add_val)            \
  /** Sets the header information associated with the HTTP handle. */          \
  fio_str_info_s fio_http_##category##_header_##name_(fio_http_s *h,           \
                                                      fio_str_info_s name,     \
                                                      fio_str_info_s value) {  \
    FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");                                  \
    return fio___http_hmap_set2(headers(h), name, value, add_val);             \
  }
FIO___HTTP_HEADER_SET_FN(request, set, HTTP_HDR_REQUEST, -1)
FIO___HTTP_HEADER_SET_FN(request, set_if_missing, HTTP_HDR_REQUEST, 0)
FIO___HTTP_HEADER_SET_FN(request, add, HTTP_HDR_REQUEST, 1)
FIO___HTTP_HEADER_SET_FN(response, set, HTTP_HDR_RESPONSE, -1)
FIO___HTTP_HEADER_SET_FN(response, set_if_missing, HTTP_HDR_RESPONSE, 0)
FIO___HTTP_HEADER_SET_FN(response, add, HTTP_HDR_RESPONSE, 1)
#undef FIO___HTTP_HEADER_SET_FN

fio_str_info_s fio_http_request_header(fio_http_s *h,
                                       fio_str_info_s name,
                                       size_t index) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return fio___http_hmap_get2(HTTP_HDR_REQUEST(h), name, (int32_t)index);
}
fio_str_info_s fio_http_response_header(fio_http_s *h,
                                        fio_str_info_s name,
                                        size_t index) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  return fio___http_hmap_get2(HTTP_HDR_RESPONSE(h), name, (int32_t)index);
}

/** Returns the number of headers named `name` that were received. */
SFUNC size_t fio_http_request_header_count(fio_http_s *h, fio_str_info_s name) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  if (!name.buf)
    return fio___http_hmap_count(HTTP_HDR_REQUEST(h));
  return fio___http_hmap_count2(HTTP_HDR_REQUEST(h), name);
}
/** Returns the number of headers named `name` that were received. */
SFUNC size_t fio_http_response_header_count(fio_http_s *h,
                                            fio_str_info_s name) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  if (!name.buf)
    return fio___http_hmap_count(HTTP_HDR_RESPONSE(h));
  return fio___http_hmap_count2(HTTP_HDR_RESPONSE(h), name);
}

/** Iterates through all headers. A non-zero return will stop iteration. */
size_t fio_http_request_header_each(fio_http_s *h,
                                    int (*callback)(fio_http_s *,
                                                    fio_str_info_s name,
                                                    fio_str_info_s value,
                                                    void *udata),
                                    void *udata) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  if (!callback)
    return fio___http_hmap_count(HTTP_HDR_REQUEST(h));
  fio___http_hmap_each_info_s d = {.h = h,
                                   .callback = callback,
                                   .udata = udata};
  return fio___http_hmap_each(HTTP_HDR_REQUEST(h),
                              fio___http_h_each_task_wrapper,
                              &d,
                              0);
}

/** Iterates through all headers. A non-zero return will stop iteration. */
size_t fio_http_response_header_each(
    fio_http_s *h,
    int (*callback)(fio_http_s *, fio_str_info_s, fio_str_info_s, void *),
    void *udata) {
  FIO_ASSERT_DEBUG(h, "NULL HTTP Handle!");
  if (!callback)
    return fio___http_hmap_count(HTTP_HDR_RESPONSE(h));
  fio___http_hmap_each_info_s d = {.h = h,
                                   .callback = callback,
                                   .udata = udata};
  return fio___http_hmap_each(HTTP_HDR_RESPONSE(h),
                              fio___http_h_each_task_wrapper,
                              &d,
                              0);
}

/* *****************************************************************************
Cookies
***************************************************************************** */

FIO_IFUNC void fio___http_cookie_set_if_missing_encoded(fio_http_s *h,
                                                        fio_str_info_s k,
                                                        fio_str_info_s v) {
  char *div = NULL;
  int k_in_dec = 0;
  FIO_STR_INFO_TMP_VAR(dec, 8192);
  /* test for percent encoding */
  if (k.len && (div = (char *)FIO_MEMCHR(k.buf, '%', k.len))) {
    if ((size_t)((k.buf + k.len) - div) > 2 && fio_c2i(div[1]) < 16 &&
        fio_c2i(div[2]) < 16 &&
        !fio_string_write_path_dec(&dec, FIO_STRING_ALLOC_COPY, k.buf, k.len)) {
      k = dec;
      k_in_dec = 1;
    }
  }
  if (v.len && (div = (char *)FIO_MEMCHR(v.buf, '%', v.len))) {
    fio_str_info_s old = dec;
    if ((size_t)((v.buf + v.len) - div) > 2 && fio_c2i(div[1]) < 16 &&
        fio_c2i(div[2]) < 16 &&
        !fio_string_write_path_dec(&dec,
                                   FIO_STR_INFO_TMP_IS_REALLOCATED(dec)
                                       ? FIO_STRING_REALLOC
                                       : FIO_STRING_ALLOC_COPY,
                                   v.buf,
                                   v.len)) {
      v = dec;
      v.buf += old.len;
      v.len -= old.len;
    }
    /* a heap-backed `dec` may have moved; the decoded key is its prefix */
    if (k_in_dec)
      k.buf = dec.buf;
  }
  fio___http_cmap_set_if_missing(h->cookies, k, v);
  if (FIO_STR_INFO_TMP_IS_REALLOCATED(dec))
    FIO_STRING_FREE2(dec);
}

/** (Helper) HTTP Cookie Parser */
FIO_IFUNC void fio___http_cookie_parse_cookie(fio_http_s *h, fio_str_info_s s) {
  /* loop and read Cookie: name=value; name2=value2; name3=value3 */
  while (s.len) {
    fio_str_info_s k = {0}, v = {0};
    /* remove white-space */
    while (s.len && (s.buf[0] == ' ' || s.buf[0] == '\t')) {
      ++s.buf;
      --s.len;
    }
    if (!s.len)
      return;
    /* delimit the pair first: a '=' past the ';' belongs to the next pair */
    char *end = (char *)FIO_MEMCHR(s.buf, ';', s.len);
    if (!end)
      end = s.buf + s.len;
    char *div = (char *)FIO_MEMCHR(s.buf, '=', (size_t)(end - s.buf));
    v.buf = s.buf;
    if (div) {
      /* cookie name may be an empty string */
      k.buf = s.buf;
      k.len = div - s.buf;
      v.buf = div + 1;
    }
    v.len = end - v.buf;
    s.len = (s.buf + s.len) - end;
    s.buf = end;
    /* skip the ';' if exists (if len is not zero, !!s.len == 1). */
    s.buf += !!s.len;
    s.len -= !!s.len;
    fio___http_cookie_set_if_missing_encoded(h, k, v);
  }
}

/** (Helper) HTTP Cookie Parser */
FIO_IFUNC void fio___http_cookie_parse_set_cookie(fio_http_s *h,
                                                  fio_str_info_s s) {
  fio_str_info_s k = {0}, v = {0};
  /* remove white-space */
  while (s.len && (s.buf[0] == ' ' || s.buf[0] == '\t')) {
    ++s.buf;
    --s.len;
  }
  if (!s.len)
    return;
  /* only the leading `name=value` pair matters (attributes follow ';') */
  char *end = (char *)FIO_MEMCHR(s.buf, ';', s.len);
  if (!end)
    end = s.buf + s.len;
  char *div = (char *)FIO_MEMCHR(s.buf, '=', (size_t)(end - s.buf));
  if (div == s.buf || !div)
    return;
  const uint64_t prefix_secure = fio_buf2u64u("_Secure-");
  const uint32_t prefix_host = fio_buf2u32u("Host");
  uint32_t cont;
  k.buf = s.buf;
  k.len = div - s.buf;
  v.buf = div + 1;
  v.len = end - v.buf;
  do { /* loop to clear away cookie prefixes in any order */
    cont = 0;
    if (k.len > 8 && k.buf[0] == '_' &&
        fio_buf2u64u(k.buf + 1) == prefix_secure) {
      cont = 1;
      k.len -= 9;
      k.buf += 9;
    }
    if (k.len > 6 && k.buf[0] == '_' && k.buf[1] == '_' && k.buf[6] == '-' &&
        fio_buf2u32u(k.buf + 2) == prefix_host) {
      cont = 1;
      k.len -= 7;
      k.buf += 7;
    }
  } while (cont);
  if (k.len)
    fio___http_cookie_set_if_missing_encoded(h, k, v);
}

/** (Helper) Parses all HTTP Cookies */
FIO_SFUNC void fio___http_cookie_collect(fio_http_s *h) {
  fio___http_sary_s *header = NULL;
  header = fio___http_hmap_node2val_ptr(
      fio___http_hmap_get_ptr(h->headers, FIO_STR_INFO1((char *)"cookie")));
  if (header) {
    FIO_ARRAY_EACH(fio___http_sary, header, pos) {
      fio___http_cookie_parse_cookie(h, fio_bstr_info(*pos.p));
    }
  }
  /* if headers were sent, set-cookie data might belong to the handle */
  if (h->writer != fio____http_write_start)
    return;
  header = fio___http_hmap_node2val_ptr(
      fio___http_hmap_get_ptr(h->headers + 1,
                              FIO_STR_INFO1((char *)"set-cookie")));
  if (!header)
    return;
  FIO_ARRAY_EACH(fio___http_sary, header, pos) {
    fio___http_cookie_parse_set_cookie(h, fio_bstr_info(*pos.p));
  }
  return;
}

int fio_http_cookie_set___(void); /* IDE Marker */
/* Sets a response cookie. */
SFUNC int fio_http_cookie_set FIO_NOOP(fio_http_s *h,
                                       fio_http_cookie_args_s cookie) {
  FIO_ASSERT_DEBUG(h, "Can't set cookie for NULL HTTP handler!");
  if (!h ||
      ((h->state & (FIO___HTTP_STATE_FINISHED | FIO___HTTP_STATE_STREAMING)) |
       (h->writer != fio____http_write_start)))
    return -1;
  /* promises that some warnings print only once. */
  static unsigned int warn_illegal = 0;
  size_t need2warn = 0;

  /* valid / invalid characters in cookies, create with Ruby using:
      a = []
      256.times {|i| a[i] = 1;}
      ('a'.ord..'z'.ord).each {|i| a[i] = 0;}
      ('A'.ord..'Z'.ord).each {|i| a[i] = 0;}
      ('0'.ord..'9'.ord).each {|i| a[i] = 0;}
      "!#$%&'*+-.^_`|~".bytes.each {|i| a[i] = 0;}
      puts "static const char invalid_cookie_name_char[256] = {"
      puts "#{ a.map {|i| i.to_s } .join(', ') }};"
      "!#$%&'()*+-./:<=>?@[]^_`{|}~".bytes.each {|i| a[i] = 0;} # for values
      ",\"\\".bytes.each {|i| a[i] = 0;} # commonly used, though not allowed
      puts "static const char invalid_cookie_value_char[256] = {"
      puts "#{ a.map {|i| i.to_s } .join(', ') }};"
  */
  static const char invalid_cookie_name_char[256] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  static const char invalid_cookie_value_char[256] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  /* name and value with a zero `len` are read as C strings (API contract) */
  /* a NULL `buf` has no content, whatever `len` says */
  if (!cookie.name.buf)
    cookie.name.len = 0;
  else if (!cookie.name.len)
    cookie.name.len = FIO_STRLEN(cookie.name.buf);
  if (!cookie.value.buf)
    cookie.value.len = 0;
  else if (!cookie.value.len)
    cookie.value.len = FIO_STRLEN(cookie.value.buf);
  if ((cookie.name.len + cookie.value.len + cookie.domain.len +
       cookie.path.len + 128) > 5119) {
    FIO_LOG_ERROR("cookie data too long!");
  }
  char tmp_buf[5120];
  fio_str_info_s t = FIO_STR_INFO3(tmp_buf, 0, 5119);
  int err = 0;
  { /* reserve the worst case for `name=value; ` once (each byte may expand to
     * a 3 byte %XX), so the encoding loops below write without checks. */
    if ((cookie.name.len | cookie.value.len) > (SIZE_MAX >> 3))
      return -1;
    const size_t need = ((cookie.name.len + cookie.value.len) * 3) + 3;
    if (need >= t.capa && FIO_STRING_ALLOC_COPY(&t, need + 1))
      return -1; /* t.buf is still tmp_buf, nothing to free */
  }

  /* write name and value while auto-correcting encoding issues */
#define fio___http_h_copy_cookie(ch_var)                                       \
  for (size_t tmp = 0; tmp < cookie.ch_var.len; ++tmp) {                       \
    const uint8_t ch = (uint8_t)cookie.ch_var.buf[tmp];                        \
    if (!invalid_cookie_##ch_var##_char[ch]) {                                 \
      t.buf[t.len++] = (char)ch;                                               \
      continue;                                                                \
    }                                                                          \
    need2warn = tmp + 1;                                                       \
    t.buf[t.len++] = '%';                                                      \
    t.buf[t.len++] = fio_i2c((ch >> 4) & 0x0F);                                \
    t.buf[t.len++] = fio_i2c(ch & 0x0F);                                       \
  }

  if (cookie.name.buf) {
    fio___http_h_copy_cookie(name);
    if (need2warn && !warn_illegal) {
      warn_illegal |= 1;
      FIO_LOG_WARNING("illegal char 0x%.2x in cookie name (in %.*s)\n"
                      "         automatic %% encoding applied",
                      (unsigned)(uint8_t)cookie.name.buf[need2warn - 1],
                      (int)(cookie.name.len > 64 ? 64 : cookie.name.len),
                      cookie.name.buf);
    }
  }
  t.buf[t.len++] = '=';
  if (cookie.value.buf) {
    need2warn = 0; /* index into `value`, not `name` */
    fio___http_h_copy_cookie(value);
    if (need2warn && !warn_illegal) {
      warn_illegal |= 1;
      FIO_LOG_WARNING("illegal char 0x%.2x in cookie value (in %.*s)\n"
                      "         automatic %% encoding applied",
                      (unsigned)(uint8_t)cookie.value.buf[need2warn - 1],
                      (int)(cookie.value.len > 64 ? 64 : cookie.value.len),
                      cookie.value.buf);
    }
  } else
    cookie.max_age = -1;
#undef fio___http_h_copy_cookie

  /* server cookie data */
  t.buf[t.len++] = ';';
  t.buf[t.len++] = ' ';

  if (cookie.max_age) {
    err |= fio_string_write2(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        FIO_STRING_WRITE_STR2((char *)"Max-Age=", 8),
        FIO_STRING_WRITE_NUM(cookie.max_age),
        FIO_STRING_WRITE_STR2((char *)"; ", 2));
  }

  if (cookie.domain.buf && cookie.domain.len) {
    err |= fio_string_write2(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        FIO_STRING_WRITE_STR2((char *)"domain=", 7),
        FIO_STRING_WRITE_STR2((char *)cookie.domain.buf, cookie.domain.len),
        FIO_STRING_WRITE_STR2((char *)"; ", 2));
  }
  if (cookie.path.buf && cookie.path.len) {
    err |= fio_string_write2(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        FIO_STRING_WRITE_STR2((char *)"path=", 5),
        FIO_STRING_WRITE_STR2((char *)cookie.path.buf, cookie.path.len),
        FIO_STRING_WRITE_STR2((char *)"; ", 2));
  }
  if (cookie.http_only) {
    err |= fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "HttpOnly; ",
        10);
  }
  if (cookie.secure) {
    err |= fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "secure; ",
        8);
  }
  if (cookie.partitioned) {
    err |= fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "partitioned; ",
        13);
  }
  switch (cookie.same_site) {
  case FIO_HTTP_COOKIE_SAME_SITE_BROWSER_DEFAULT: /* fall through */
  default: break;
  case FIO_HTTP_COOKIE_SAME_SITE_NONE:
    err |= fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "SameSite=None;",
        14);
    break;
  case FIO_HTTP_COOKIE_SAME_SITE_LAX:
    err |= fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "SameSite=Lax;",
        13);
    break;
  case FIO_HTTP_COOKIE_SAME_SITE_STRICT:
    err |= fio_string_write(
        &t,
        ((t.buf == tmp_buf) ? FIO_STRING_ALLOC_COPY : FIO_STRING_REALLOC),
        "SameSite=Strict;",
        16);
    break;
  }
  if (err) { /* never emit a truncated cookie (may lose Secure / HttpOnly) */
    FIO_LOG_ERROR("cookie formatting failed (out of memory?)");
    if (t.buf != tmp_buf)
      FIO_STRING_FREE2(t);
    return -1;
  }
  if (t.buf[t.len - 1] == ' ')
    --t.len;

  /* set the "write" cookie store data */
  fio___http_cmap_set(h->cookies + 1, cookie.name, t, NULL);
  /* set the "read" cookie store data */
  fio___http_cmap_set(h->cookies, cookie.name, cookie.value, NULL);
  if (t.buf != tmp_buf)
    FIO_STRING_FREE2(t);
  return 0;
}

/** Returns a cookie value (either received of newly set), if any. */
SFUNC fio_str_info_s fio_http_cookie(fio_http_s *h,
                                     const char *name,
                                     size_t name_len) {
  if (!(fio_atomic_or(&h->state, FIO___HTTP_STATE_COOKIES_PARSED) &
        FIO___HTTP_STATE_COOKIES_PARSED))
    fio___http_cookie_collect(h);
  fio_str_info_s r =
      fio___http_cmap_get(h->cookies, FIO_STR_INFO2((char *)name, name_len));
  return r;
}

/** Iterates through all cookies. A non-zero return will stop iteration. */
SFUNC size_t fio_http_cookie_each(fio_http_s *h,
                                  int (*callback)(fio_http_s *,
                                                  fio_str_info_s name,
                                                  fio_str_info_s value,
                                                  void *udata),
                                  void *udata) {
  if (!(fio_atomic_or(&h->state, FIO___HTTP_STATE_COOKIES_PARSED) &
        FIO___HTTP_STATE_COOKIES_PARSED))
    fio___http_cookie_collect(h);
  size_t i = 0;
  FIO_MAP_EACH(fio___http_cmap, h->cookies, pos) {
    ++i;
    if (callback(h, pos.key, pos.value, udata))
      return i;
  }
  return i;
}

/**
 * Iterates through all response set cookies.
 *
 * A non-zero return value from the callback will stop iteration.
 */
SFUNC size_t
fio_http_set_cookie_each(fio_http_s *h,
                         int (*callback)(fio_http_s *h,
                                         fio_str_info_s set_cookie_header,
                                         fio_str_info_s value,
                                         void *udata),
                         void *udata) {
  size_t i = 0;
  fio___http_cmap_s *set_cookies = h->cookies + 1;
  fio_str_info_s header_name = FIO_STR_INFO2((char *)"set-cookie", 10);
  FIO_MAP_EACH(fio___http_cmap, set_cookies, pos) {
    ++i;
    if (callback(h, header_name, pos.value, udata))
      return i;
  }
  return i;
}

/* *****************************************************************************
Peer Address
***************************************************************************** */

/* Returns the address at the start of `v` (after an optional `"`), or an
 * empty buffer. Ends at `"`, `,`, `;`, SP / HTAB or after 48 bytes. */
FIO_SFUNC fio_buf_info_s fio___http_from_value(fio_str_info_s v) {
  fio_buf_info_s r = FIO_BUF_INFO2(v.buf, 0);
  if (v.len && v.buf[0] == '"')
    r.buf = ++v.buf, --v.len;
  while (r.len < v.len && r.len < 48 && v.buf[r.len] != '"' &&
         v.buf[r.len] != ',' && v.buf[r.len] != ';' && v.buf[r.len] != ' ' &&
         v.buf[r.len] != '\t')
    ++r.len;
  return r;
}

/* Returns the `for=` value of a `forwarded` header, or an empty buffer. */
FIO_SFUNC fio_buf_info_s fio___http_from_forwarded(fio_str_info_s v) {
  if (v.len > 1023)
    v.len = 1023; /* limit possible attack surface */
  for (size_t i = 0; i + 4 < v.len; ++i) {
    if ((v.buf[i] | 32) != 'f' || (v.buf[i + 1] | 32) != 'o' ||
        (v.buf[i + 2] | 32) != 'r' || v.buf[i + 3] != '=')
      continue;
    if (i && v.buf[i - 1] != ';' && v.buf[i - 1] != ',' &&
        v.buf[i - 1] != ' ' && v.buf[i - 1] != '\t')
      continue; /* part of another name (e.g., `xfor=`) */
    return fio___http_from_value(FIO_STR_INFO2(v.buf + i + 4, v.len - i - 4));
  }
  return FIO_BUF_INFO2(NULL, 0);
}

/**
 * Writes peer address to `dest`: the `forwarded` header's `for=` value, then
 * the first `x-forwarded-for` address, then the socket address, and finally
 * `"[unknown]"`.
 *
 * Returns 0 if a forwarded header was used, -1 if not. If `dest` is too small,
 * nothing is written and the address length is returned.
 */
SFUNC int fio_http_from(fio_str_info_s *dest, const fio_http_s *h) {
  int r = 0;
  fio_buf_info_s buf = fio___http_from_forwarded(
      fio_http_request_header((fio_http_s *)h,
                              FIO_STR_INFO2((char *)"forwarded", 9),
                              -1));
  if (!buf.len) {
    fio_str_info_s xff =
        fio_http_request_header((fio_http_s *)h,
                                FIO_STR_INFO2((char *)"x-forwarded-for", 15),
                                -1);
    if (xff.len > 1023)
      xff.len = 1023;
    buf = fio___http_from_value(xff);
  }
  if (!buf.len) {
    r = -1;
#if defined(H___FIO_SOCK___H)
    buf = fio_sock_peer_addr(
        fio_http_controller((fio_http_s *)h)->get_fd((fio_http_s *)h),
        false);
#endif
    if (!buf.len)
      buf = FIO_BUF_INFO1((char *)"[unknown]");
  }
  if (dest->capa > dest->len + buf.len) { /* enough space? */
    FIO_MEMCPY(dest->buf + dest->len, buf.buf, buf.len);
    dest->len += buf.len;
    dest->buf[dest->len] = 0;
  } else
    r = (int)buf.len;
  return r;
}

/* *****************************************************************************
Body Management - Public API (delegates to `fio_data_s`)
***************************************************************************** */

/** Gets the body (payload) length associated with the HTTP handle. */
SFUNC size_t fio_http_body_length(fio_http_s *h) {
  return fio_data_len(&h->body);
}

/**
 * If the body is stored in a temporary file, returns the file's handle.
 *
 * Otherwise returns -1.
 */
SFUNC int fio_http_body_fd(fio_http_s *h) { return fio_data_fd(&h->body); }

/** Releases body (payload) resources, closing any temporary files. */
SFUNC void fio_http_body_close(fio_http_s *h) { fio_data_destroy(&h->body); }

/** Saves the whole body to `filename` (atomic). Returns 0 or -1. */
SFUNC int fio_http_body_save(fio_http_s *h, const char *filename) {
  return fio_data_save(&h->body, filename);
}

/**
 * Adjusts the body's reading position and returns the new position.
 *
 * `-n` is `length - n` (`-1` is the last byte), clamped to 0; any position at
 * or past the end is EOF.
 */
SFUNC size_t fio_http_body_seek(fio_http_s *h, ssize_t pos) {
  return fio_data_seek(&h->body, pos);
}

/** Returns the body's reading position. */
SFUNC size_t fio_http_body_pos(fio_http_s *h) { return fio_data_pos(&h->body); }

/** Reads up to `length` of data from the body, returns nothing on EOF. */
SFUNC fio_str_info_s fio_http_body_read(fio_http_s *h, size_t length) {
  return fio_data_read(&h->body, length);
}

/**
 * Reads from the body until finding `token`, reaching `limit` or EOF.
 *
 * Note: `limit` is ignored if zero or larger than remaining data.
 */
SFUNC fio_str_info_s fio_http_body_read_until(fio_http_s *h,
                                              char token,
                                              size_t limit) {
  return fio_data_read_until(&h->body, token, limit);
}

/** Allocates a body (payload) of (at least) the `expected_length`. */
SFUNC void fio_http_body_expect(fio_http_s *h, size_t expected_length) {
  fio_data_expect(&h->body, expected_length);
}

/** Writes `data` to the body (payload). Returns 0 on success, -1 on error. */
SFUNC int fio_http_body_write(fio_http_s *h, const void *data, size_t len) {
  return fio_data_write(&h->body, data, len);
}

/* *****************************************************************************
A Response Payload
***************************************************************************** */

/** ETag Helper */
FIO_IFUNC int fio___http_response_etag_if_none_match(fio_http_s *h);

/**
 * Adds `accept-encoding` to the response `Vary` header (RFC 9110 §12.5.5)
 * while preserving any dimensions the application already set. Nothing is
 * added if `accept-encoding` or `*` is already listed.
 */
FIO_SFUNC void fio___http_response_vary_accept_encoding(fio_http_s *h) {
  fio___http_sary_s *vary = fio___http_hmap_node2val_ptr(
      fio___http_hmap_get_ptr(HTTP_HDR_RESPONSE(h),
                              FIO_STR_INFO2((char *)"vary", 4)));
  if (vary) {
    FIO_ARRAY_EACH(fio___http_sary, vary, pos) {
      fio_str_info_s v = fio_bstr_info(*pos.p);
      if (fio___http_header_has_token(v, "accept-encoding", 15) ||
          fio___http_header_has_token(v, "*", 1))
        return;
    }
  }
  fio_http_response_header_add(h,
                               FIO_STR_INFO2((char *)"vary", 4),
                               FIO_STR_INFO2((char *)"accept-encoding", 15));
}

FIO_SFUNC int fio____http_write_done(fio_http_s *h,
                                     fio_http_write_args_s *args) {
  return -1;
  (void)h, (void)args;
}

FIO_SFUNC int fio____http_write_upgraded(fio_http_s *h,
                                         fio_http_write_args_s *args) {
  h->controller->write_body(h, *args);
  h->sent += args->len;
  return 0;
}

/* Internal state: the response omits all content (HEAD / no-content status).
 * Set once by `fio____http_write_start`; body writes are then discarded and
 * the response is never streamed, so controllers never frame a body. */
#define FIO___HTTP_STATE_NO_CONTENT 128

/* Responses that never carry content: 1xx, 204, 205, 304 (RFC 9110 §15). */
FIO_IFUNC int fio___http_status_has_no_content(size_t status) {
  return ((status - 100) < 100) | (status == 204) | (status == 205) |
         (status == 304);
}

/* Framing headers for a response without content (no streaming / coding). */
FIO_SFUNC void fio___http_no_content_headers(fio_http_s *h,
                                             fio___http_hmap_s *hdrs) {
  fio___http_hmap_set2(hdrs,
                       FIO_STR_INFO2((char *)"transfer-encoding", 17),
                       FIO_STR_INFO0,
                       -1);
  if (h->status == 304) /* may describe the selected representation (§8.6) */
    return;
  /* 1xx / 204 MUST NOT send Content-Length (§8.6); 205 MUST indicate zero
   * length (§15.3.6) - any other value would desynchronize the client. */
  fio___http_hmap_set2(hdrs,
                       FIO_STR_INFO2((char *)"content-length", 14),
                       (h->status == 205) ? FIO_STR_INFO2((char *)"0", 1)
                                          : FIO_STR_INFO0,
                       -1);
}

/* A file write without a length is the rest of the file, from `offset`. It is
 * resolved for every write (first or later), before any framing
 * (Content-Length, ETag, chunk size, `sent`) depends on `len`. A length that
 * can't be known (not a regular file) or an empty range sends no file data: the
 * fd is released here (unless `copy`, where the caller keeps it) and the write
 * carries no data. */
FIO_IFUNC void fio___http_write_args_resolve_fd(fio_http_write_args_s *args) {
  if (args->buf || args->len || ((uint32_t)args->fd + 1U) <= 1U)
    return;
  const int regular = (fio_fd_type(args->fd) == S_IFREG);
  const size_t size = regular ? fio_fd_size(args->fd) : 0;
  if (size > args->offset) {
    args->len = size - args->offset;
    return;
  }
  if (!regular)
    FIO_LOG_ERROR("(%d) HTTP file body (fd %d) has no known length - not sent.",
                  fio_io_pid(),
                  args->fd);
  if (!args->copy)
    close(args->fd);
  args->fd = -1;
}

FIO_SFUNC int fio____http_write_start(fio_http_s *h,
                                      fio_http_write_args_s *args) {
  /* no content and no finish: nothing to frame yet. The response (headers,
   * Content-Length vs. streaming) starts with the first body data or finish,
   * so an empty stream becomes a `content-length: 0` response. */
  if (!args->len && !args->finish && (uint32_t)(args->fd + 1) <= 1U) {
    if (args->dealloc && args->buf)
      args->dealloc((void *)args->buf);
    return 0;
  }
  /* if response has an `etag` header matching `if-none-match`, skip */
  fio___http_hmap_s *hdrs = h->headers + (!!h->status);
  if (h->status) {
    if (args->len && fio___http_response_etag_if_none_match(h))
      return -1;
    if (!args->len && args->finish && h->status >= 400) {
      fio_http_send_error_response(h, h->status);
      return 0;
    }
    /* validate Date header */
    fio___http_hmap_set2(
        hdrs,
        FIO_STR_INFO2((char *)"date", 4),
        fio_http_date(fio_http_get_timestump() / FIO___HTTP_TIME_DIV),
        0);
    if (FIO_UNLIKELY(fio___http_status_has_no_content(h->status))) {
      h->state |= FIO___HTTP_STATE_NO_CONTENT;
      fio___http_no_content_headers(h, hdrs);
      goto send_headers; /* never compressed, framed or streamed */
    }
    { /* HEAD: GET's headers, no content (RFC 9110 §9.3.2) */
      fio_str_info_s m = fio_keystr_info(&h->method);
      if (FIO_UNLIKELY(m.len == 4 && (fio_buf2u32u(m.buf) | 0x20202020UL) ==
                                         fio_buf2u32u("head")))
        h->state |= FIO___HTTP_STATE_NO_CONTENT;
    }
  }
#if 1
  if (args->finish &&
      FIO_LIKELY(
          fio___http_cflags_is_set(h, FIO___HTTP_CFLAG_COMPRESS_DYNAMIC)) &&
      args->len > 1024 && args->buf) {
    /* Never double-compress: the application already encoded this response. */
    if (fio___http_hmap_get_ptr(hdrs,
                                FIO_STR_INFO2((char *)"content-encoding", 16)))
      goto compression_done;
    /* only compress text-like content types */
    if (!fio___http_mime_is_compressible(
            fio_http_response_header(h,
                                     FIO_STR_INFO2((char *)"content-type", 12),
                                     0)))
      goto compression_done;
    fio_str_info_s ac =
        fio_http_request_header(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                0);
    if (!ac.len) {
      /* No Accept-Encoding in the request: identity is served, but the
       * representation WAS selected by considering Accept-Encoding (a
       * gzip-capable request would receive a compressed variant), so
       * caches must key on it — RFC 9110 §12.5.5. */
      fio___http_response_vary_accept_encoding(h);
      goto compression_done;
    }
    /* try encodings in preference order (brotli > gzip) */
    struct {
      fio_str_info_s token;
      fio_str_info_s encoding;
      size_t (*compress)(void *, size_t, const void *, size_t, int);
      size_t (*bound)(size_t);
      size_t extra; /* added to bound (e.g. gzip header+trailer) */
    } encodings[] = {{FIO_STR_INFO2((char *)"br", 2),
                      FIO_STR_INFO2((char *)"br", 2),
                      fio_brotli_compress,
                      fio_brotli_compress_bound,
                      0},
                     {FIO_STR_INFO2((char *)"gzip", 4),
                      FIO_STR_INFO2((char *)"gzip", 4),
                      fio_gzip_compress,
                      fio_deflate_compress_bound,
                      18},
                     {.token = {0}}};
    fio_str_info_s comp = {0};
    fio_str_info_s enc_name = {0};
    for (size_t i = 0; encodings[i].token.buf; ++i) {
      if (!fio___http_header_has_token(ac,
                                       encodings[i].token.buf,
                                       encodings[i].token.len))
        continue;
      size_t bound = encodings[i].bound(args->len) + encodings[i].extra;
      comp = (fio_str_info_s){0};
      if (FIO_STRING_REALLOC(&comp, bound))
        continue;
      comp.len =
          encodings[i].compress(comp.buf, comp.capa, args->buf, args->len, 1);
      if (comp.len && comp.len < args->len) {
        enc_name = encodings[i].encoding;
        break;
      }
      FIO_STRING_FREE2(comp);
      comp = (fio_str_info_s){0};
    }
    if (comp.buf) {
      /* free original buffer if owned */
      if (args->dealloc && args->buf)
        args->dealloc((void *)args->buf);
      args->buf = comp.buf;
      args->len = comp.len;
      args->offset = 0;
      args->copy = 0;
      args->dealloc = FIO_STRING_FREE;
      /* update Content-Length */
      {
        char ibuf[32];
        fio_str_info_s v = FIO_STR_INFO3(ibuf, 0, 32);
        v.len = fio_digits10u(comp.len);
        fio_ltoa10u(v.buf, comp.len, v.len);
        fio___http_hmap_set2(hdrs,
                             FIO_STR_INFO2((char *)"content-length", 14),
                             v,
                             -1);
      }
      fio_http_response_header_set(
          h,
          FIO_STR_INFO2((char *)"content-encoding", 16),
          enc_name);
    }
    /* Compressed, or identity won (e.g. `q=0`, no common token, no gain):
     * either way caches must key on Accept-Encoding. */
    fio___http_response_vary_accept_encoding(h);
  compression_done:; /* skipped paths rejoin here (Content-Length must
                         still be validated for finished responses) */
  }
#endif
  /* test if streaming / single body response */
  if (!fio___http_hmap_get_ptr(hdrs,
                               FIO_STR_INFO2((char *)"content-length", 14))) {
    if (args->finish) {
      /* validate / set Content-Length (not streaming) */
      char ibuf[32];
      fio_str_info_s k = FIO_STR_INFO2((char *)"content-length", 14);
      fio_str_info_s v = FIO_STR_INFO3(ibuf, 0, 32);
      v.len = fio_digits10u(args->len);
      fio_ltoa10u(v.buf, args->len, v.len);
      /* no memory for Content-Length: stream (framed without allocation) */
      if (FIO_UNLIKELY(!fio___http_hmap_set2(hdrs, k, v, -1).buf) &&
          !(h->state & FIO___HTTP_STATE_NO_CONTENT))
        h->state |= FIO___HTTP_STATE_STREAMING;
    } else if (!(h->state & FIO___HTTP_STATE_NO_CONTENT)) {
      h->state |= FIO___HTTP_STATE_STREAMING;
    }
  }
send_headers:
  /* start a response, unless status == 0 (which starts a request). */
  h->controller->send_headers(h);
  return (h->writer = fio____http_write_cont)(h, args);
}

FIO_SFUNC int fio____http_write_cont(fio_http_s *h,
                                     fio_http_write_args_s *args) {
  int r = 0 - !!(h->state & FIO___HTTP_STATE_NO_CONTENT);
  if (!r) {
    h->controller->write_body(h, *args);
    h->sent += args->len;
  }
  if (args->finish) {
    h->state |= FIO___HTTP_STATE_FINISHED;
    h->writer = (h->state & FIO___HTTP_STATE_UPGRADED)
                    ? fio____http_write_upgraded
                    : fio____http_write_done;
    h->controller->on_finish(h);
  }
  return r;
}

void fio_http_write___(void); /* IDE Marker */
/**
 * Writes `data` to the response body associated with the HTTP handle after
 * sending all headers (no further headers may be sent).
 */
SFUNC void fio_http_write FIO_NOOP(fio_http_s *h, fio_http_write_args_s args) {
  if (!h || !h->controller)
    goto handle_error;
  fio___http_write_args_resolve_fd(&args);
  if (h->writer(h, &args))
    goto handle_error;
  return;

handle_error:
  if ((unsigned)(args.fd + 1) > 1U && !args.copy)
    close(args.fd);
  if (args.dealloc && args.buf)
    args.dealloc((void *)args.buf);
}

/** ETag Helper */
FIO_IFUNC int fio___http_response_etag_if_none_match(fio_http_s *h) {
  if (!fio_http_etag_is_match(h))
    return 0;
  h->status = 304;
  fio___http_hmap_set2(HTTP_HDR_RESPONSE(h),
                       FIO_STR_INFO2((char *)"content-length", 14),
                       FIO_STR_INFO0,
                       -1);

  h->controller->send_headers(h);
  h->state |= FIO___HTTP_STATE_FINISHED;
  h->writer = fio____http_write_done;
  h->controller->on_finish(h);
  return -1;
}

/* *****************************************************************************
WebSocket / SSE Helpers
***************************************************************************** */

/** Returns non-zero if request headers ask for a WebSockets Upgrade.*/
SFUNC int fio_http_websocket_requested(fio_http_s *h) {
  fio_str_info_s val =
      fio_http_request_header(h, FIO_STR_INFO2((char *)"connection", 10), 0);
  /* test for "Connection: Upgrade" (TODO? allow for multi-value?) */
  if (val.len < 7 ||
      !(((fio_buf2u32u(val.buf) | 0x20202020UL) == fio_buf2u32u("upgr")) ||
        ((fio_buf2u32u(val.buf + 3) | 0x20202020) == fio_buf2u32u("rade"))))
    return 0;
  /* test for "Upgrade: websocket" (TODO? allow for multi-value?) */
  val = fio_http_request_header(h, FIO_STR_INFO2((char *)"upgrade", 7), 0);
  if (val.len < 7 ||
      !(((fio_buf2u64u(val.buf) | 0x2020202020202020ULL) ==
         fio_buf2u64u("websocke")) ||
        ((fio_buf2u32u(val.buf + 5) | 0x20202020UL) == fio_buf2u32u("cket"))))
    return 0;
  val = fio_http_request_header(h,
                                FIO_STR_INFO2((char *)"sec-websocket-key", 17),
                                0);
  if (val.len != 24)
    return 0;
  /* test for version value */
  val = fio_http_request_header(
      h,
      FIO_STR_INFO2((char *)"sec-websocket-version", 21),
      0);
  if (val.len != 2 || (val.buf[0] != '1' || val.buf[1] != '3'))
    return -1; /* note the error value is still true, requested WebSocket... */
  return 1;
}

/** Sets response data to agree to a WebSockets Upgrade.*/
SFUNC void fio_http_upgrade_websocket(fio_http_s *h) {
  { /* validate WebSocket version */
    fio_str_info_s val = fio_http_request_header(
        h,
        FIO_STR_INFO2((char *)"sec-websocket-version", 21),
        0);
    if (val.len != 2 || (val.buf[0] != '1' || val.buf[1] != '3'))
      goto version_error;
  }
  h->status = 101;
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"connection", 10),
                               FIO_STR_INFO2((char *)"Upgrade", 7));
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"upgrade", 7),
                               FIO_STR_INFO2((char *)"websocket", 9));
  { /* Sec-WebSocket-Accept */
    fio_str_info_s k =
        fio_http_request_header(h,
                                FIO_STR_INFO2((char *)"sec-websocket-key", 17),
                                0);
    FIO_STR_INFO_TMP_VAR(accept_val, 63);
    if (k.len != 24)
      goto handshake_error;
    fio_string_write(&accept_val, NULL, k.buf, k.len);
    fio_string_write(&accept_val,
                     NULL,
                     "258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
                     36);
    fio_sha1_s sha = fio_sha1(accept_val.buf, accept_val.len);
    fio_sha1_digest(&sha);
    accept_val.len = 0;
    fio_string_write_base64enc(&accept_val,
                               NULL,
                               fio_sha1_digest(&sha),
                               fio_sha1_len(),
                               0);
    fio_http_response_header_set(
        h,
        FIO_STR_INFO2((char *)"sec-websocket-accept", 20),
        accept_val);
  }
  { /* finish up */
    h->state |= FIO___HTTP_STATE_UPGRADED | FIO___HTTP_STATE_WEBSOCKET;
    fio_http_write_args_s args = {.finish = 1};
    fio_http_write FIO_NOOP(h, args);
  }
  return;
handshake_error:
  fio_http_send_error_response(h, 403);
  return;
version_error:
  fio_http_response_header_set(
      h,
      FIO_STR_INFO2((char *)"sec-websocket-version", 21),
      FIO_STR_INFO2((char *)"13", 2));
  fio_http_send_error_response(h, 400);
  return;
}

/** Sets request data to request a WebSockets Upgrade.*/
SFUNC void fio_http_websocket_set_request(fio_http_s *h) {
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"connection", 10),
                              FIO_STR_INFO2((char *)"Upgrade", 7));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"pragma", 6),
                              FIO_STR_INFO2((char *)"no-cache", 8));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"cache-control", 13),
                              FIO_STR_INFO2((char *)"no-cache", 8));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"upgrade", 7),
                              FIO_STR_INFO2((char *)"websocket", 9));
  {
    fio_http_request_header_set_if_missing(
        h,
        FIO_STR_INFO2((char *)"origin", 6),
        fio_http_request_header(h, FIO_STR_INFO2((char *)"host", 4), 0));
  }
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"sec-websocket-version", 21),
      FIO_STR_INFO2((char *)"13", 2));

  {
    uint64_t tmp[2] = {fio_rand64(), fio_rand64()};
    FIO_STR_INFO_TMP_VAR(key, 64);
    fio_string_write_base64enc(&key, NULL, tmp, 16, 0);
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"sec-websocket-key", 17),
                                key);
  }
  /* sec-websocket-extensions ? */
  /* send request? */
}

/** Returns non-zero if the response accepts a WebSocket upgrade request. */
SFUNC int fio_http_websocket_accepted(fio_http_s *h) {
  if (h->status != 101)
    return 0;
  if (!fio_http_websocket_requested(h))
    return 0;
  fio_str_info_s tst =
      fio_http_response_header(h, FIO_STR_INFO2((char *)"connection", 10), 0);
  if (tst.len < 7 ||
      (fio_buf2u64_le(tst.buf) | (uint64_t)0x20202020202020FFULL) !=
          (fio_buf2u64_le("upgrade") | (uint64_t)0x20202020202020FFULL))
    return 0;
  tst = fio_http_response_header(h, FIO_STR_INFO2((char *)"upgrade", 7), 0);
  if (tst.len < 9 || (tst.buf[0] | 32) != 'w' ||
      (fio_buf2u64u(tst.buf + 1) | (uint64_t)0x2020202020202020ULL) !=
          fio_buf2u64u("ebsocket"))
    return 0;
  { /* Sec-WebSocket-Accept */
    tst = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"sec-websocket-accept", 20),
        0);
    if (!tst.len)
      return 0;

    fio_str_info_s k =
        fio_http_request_header(h,
                                FIO_STR_INFO2((char *)"sec-websocket-key", 17),
                                0);
    FIO_STR_INFO_TMP_VAR(accept_val, 63);
    if (k.len != 24)
      return 0;
    fio_string_write(&accept_val, NULL, k.buf, k.len);
    fio_string_write(&accept_val,
                     NULL,
                     "258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
                     36);
    fio_sha1_s sha = fio_sha1(accept_val.buf, accept_val.len);
    fio_sha1_digest(&sha);
    accept_val.len = 0;
    fio_string_write_base64enc(&accept_val,
                               NULL,
                               fio_sha1_digest(&sha),
                               fio_sha1_len(),
                               0);
    if (!FIO_STR_INFO_IS_EQ(tst, accept_val)) {
      FIO_LOG_DDEBUG2(
          "(%d) sec-websocket-key invalid, WebSocket handshake failed.\n\t"
          "%s != %s",
          getpid(),
          tst.buf,
          accept_val.buf);
      return 0;
    }
  }
  h->state |= (FIO___HTTP_STATE_UPGRADED | FIO___HTTP_STATE_WEBSOCKET |
               FIO___HTTP_STATE_FINISHED);
  h->writer = fio____http_write_upgraded;
  return 1;
}

/** Returns non-zero if request headers ask for an EventSource (SSE) Upgrade.*/
SFUNC int fio_http_sse_requested(fio_http_s *h) {
  fio_str_info_s val =
      fio_http_request_header(h, FIO_STR_INFO2((char *)"accept", 6), 0);
  if (val.len < 17)
    return 0;
  if ((val.buf[0] | 32) != 't')
    return 0;
  uint64_t t0 = fio_buf2u64u(val.buf + 1) | (uint64_t)0x2020202020202020ULL;
  uint64_t t1 = fio_buf2u64u(val.buf + 9) | (uint64_t)0x2020202020202020ULL;
  if ((t0 != fio_buf2u64u("ext/even")) || (t1 != fio_buf2u64u("t-stream")))
    return 0; /* note that '/' and '-' both have 32 (bit[5]) set */
  FIO_LOG_DDEBUG2("(%d) EventSource connection requested.",
                  fio_thread_getpid());
  return 1;
}

/** Returns non-zero if the response accepts an SSE request. */
SFUNC int fio_http_sse_accepted(fio_http_s *h) {
  if (!fio_http_sse_requested(h))
    return 0;
  if (h->status != 200)
    return 0;
  fio_str_info_s val =
      fio_http_request_header(h, FIO_STR_INFO2((char *)"accept", 6), 0);
  for (size_t i = 0; i < 2; ++i) {
    if (val.len < 17)
      return 0;
    if ((val.buf[0] | 32) != 't')
      return 0;
    uint64_t t0 = fio_buf2u64u(val.buf + 1) | (uint64_t)0x2020202020202020ULL;
    uint64_t t1 = fio_buf2u64u(val.buf + 9) | (uint64_t)0x2020202020202020ULL;
    if ((t0 != fio_buf2u64u("ext/even")) || (t1 != fio_buf2u64u("t-stream")))
      return 0; /* note that '/' and '-' both have 32 (bit[5]) set */
    val = fio_http_response_header(h,
                                   FIO_STR_INFO2((char *)"content-type", 12),
                                   0);
  }
  h->state |= (FIO___HTTP_STATE_UPGRADED | FIO___HTTP_STATE_SSE |
               FIO___HTTP_STATE_FINISHED);
  h->writer = fio____http_write_upgraded;
  FIO_LOG_DDEBUG2("EventSource connection accepted.");
  return 1;
}

/** Sets response data to agree to an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_upgrade_sse(fio_http_s *h) {
  if (h->state)
    return;
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"content-type", 12),
                               FIO_STR_INFO2((char *)"text/event-stream", 17));
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"cache-control", 13),
                               FIO_STR_INFO2((char *)"no-store", 8));
  fio___http_hmap_remove(HTTP_HDR_RESPONSE(h),
                         FIO_STR_INFO2((char *)"content-length", 14),
                         NULL);
  h->state |= FIO___HTTP_STATE_FINISHED | FIO___HTTP_STATE_UPGRADED |
              FIO___HTTP_STATE_SSE;
  h->controller->send_headers(h);
  h->writer = fio____http_write_upgraded;
  h->controller->on_finish(h);
}

/** Sets request data to request an EventSource (SSE) Upgrade.*/
SFUNC void fio_http_sse_set_request(fio_http_s *h) {
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"accept", 6),
                              FIO_STR_INFO2((char *)"text/event-stream", 17));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"connection", 10),
                              FIO_STR_INFO2((char *)"keep-alive", 10));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"cache-control", 13),
                              FIO_STR_INFO2((char *)"no-cache", 8));
}

/* *****************************************************************************
Error Handling
***************************************************************************** */

/** Sends the requested error message and finishes the response. */
SFUNC int fio_http_send_error_response(fio_http_s *h, size_t status) {
  if (!h || h->writer != fio____http_write_start)
    return -1;
  if (!status || status > 1000)
    status = 404;
  h->status = (uint16_t)status;
  char *body = NULL;
  fio_http_settings_s *st = fio_http_settings(h);
  fio_buf_info_s folders[] = {
      FIO_BUF_INFO2((char *)"./", 2),
      (st ? FIO_BUF_INFO2(st->public_folder.buf, st->public_folder.len)
          : FIO_BUF_INFO0),
      FIO_BUF_INFO0,
  };
  FIO_STR_INFO_TMP_VAR(filename, 1023);

  for (size_t i = 0; !body && folders[i].len; ++i) {
    filename.len = 0;
    fio_string_write2(&filename,
                      NULL,
                      FIO_STRING_WRITE_STR2(folders[i].buf, folders[i].len),
                      FIO_STRING_WRITE_STR2(
                          "/",
                          (size_t)(folders[i].buf[folders[i].len - 1] != '/' &&
                                   folders[i].buf[folders[i].len - 1] !=
                                       FIO_FOLDER_SEPARATOR)),
                      FIO_STRING_WRITE_UNUM(status),
                      FIO_STRING_WRITE_STR2(".html", 5));
    body = fio_bstr_readfile(NULL, filename.buf, 0, 0);
  }
  fio_http_write_args_s args = {.buf = body,
                                .len = fio_bstr_len(body),
                                .dealloc = (void (*)(void *))fio_bstr_free,
                                .finish = 1};
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"content-type", 12),
                               body ? FIO_STR_INFO2((char *)"text/html", 9)
                                    : FIO_STR_INFO2((char *)"text/plain", 10));
  if (!body) { /* write a short error response (plain text fallback) */
    fio_str_info_s status_str = fio_http_status2str(status);
    filename.len = 0;
    fio_string_write2(&filename,
                      NULL,
                      FIO_STRING_WRITE_STR2("Error ", 6),
                      FIO_STRING_WRITE_UNUM(status),
                      FIO_STRING_WRITE_STR2(": ", 2),
                      FIO_STRING_WRITE_STR_INFO(status_str),
                      FIO_STRING_WRITE_STR2(".", 1));
    args.buf = filename.buf;
    args.len = filename.len;
    args.copy = 1;
    args.dealloc = NULL;
  }
  fio_http_write FIO_NOOP(h, args);
  return 0;
}

/* *****************************************************************************
HTTP Logging
***************************************************************************** */

FIO_SFUNC void fio___http_write_pid(fio_str_info_s *dest) {
  static int last_pid = 0;
  static char buf[64];
  static size_t len = 0;
#ifdef H___FIO_SERVER___H
  int pid = fio_srv_pid();
#else
  int pid = fio_thread_getpid();
#endif
  if (last_pid != pid)
    goto rewrite;
copy:
  if (len)
    FIO_MEMCPY(dest->buf + dest->len, buf, len);
  dest->len += len;
  return;
rewrite:
  len = 0;
  buf[len++] = '[';
  if (pid > 0) {
    size_t d = fio_digits10u((uint64_t)pid);
    fio_ltoa10u(buf + 1, (uint64_t)pid, d);
    len += d;
  }
  buf[len++] = ']';
  last_pid = pid;
  goto copy;
}
/** Logs an HTTP (response) to STDOUT. */
SFUNC void fio_http_write_log(fio_http_s *h) {
  FIO_STR_INFO_TMP_VAR(buf, 1023);
  intptr_t bytes_sent = h->sent;
  uint64_t time_start, time_end, time_proxy = 0;
  time_start = h->received_at;
  time_end = fio_http_get_timestump();
  fio_str_info_s date = fio_http_log_time(time_end / FIO___HTTP_TIME_DIV);
  fio_string_write_s to_write[16] = {
      FIO_STRING_WRITE_STR_INFO(fio_keystr_info(&h->method)),
      FIO_STRING_WRITE_STR2((const char *)" ", 1),
      FIO_STRING_WRITE_STR_INFO(fio_keystr_info(&h->opath)),
      FIO_STRING_WRITE_STR2((const char *)" ", 1),
      FIO_STRING_WRITE_STR_INFO(fio_keystr_info(&h->version)),
      FIO_STRING_WRITE_STR2((const char *)"\" ", 2),
      FIO_STRING_WRITE_NUM(h->status),
      FIO_STRING_WRITE_STR2(" ", 1),
      ((bytes_sent > 0) ? (FIO_STRING_WRITE_UNUM(bytes_sent))
                        : (FIO_STRING_WRITE_STR2((const char *)"---", 3))),
      FIO_STRING_WRITE_STR2((const char *)" ", 1),
      FIO_STRING_WRITE_NUM(time_end - time_start),
      FIO_STRING_WRITE_STR2((const char *)(FIO___HTTP_TIME_UNIT "\r\n"), 4),
  };
  if (FIO_HTTP_LOG_X_REQUEST_START) {
    /* log request wait time using x-request-start header */
    fio_str_info_s xstart =
        fio_http_request_header(h,
                                FIO_STR_INFO2((char *)"x-request-start", 15),
                                0);
    unsigned step =
        (xstart.len > 1 && (xstart.buf[0] | 32) == 't' && xstart.buf[1] == '=');
    step <<= 1;
    xstart.buf += step;
    xstart.len -= step;
    time_proxy = fio_atol(&xstart.buf);
    time_proxy *= (FIO___HTTP_TIME_DIV / 1000); /* assumes info in ms */
    time_proxy = time_start - time_proxy;
    if (time_proxy < (512 * FIO___HTTP_TIME_DIV)) { /* was ms? */
      to_write[11] =
          FIO_STRING_WRITE_STR2((const char *)(FIO___HTTP_TIME_UNIT " (wait "),
                                9);
      to_write[12] = FIO_STRING_WRITE_NUM(time_proxy);
      to_write[13] =
          FIO_STRING_WRITE_STR2((const char *)(FIO___HTTP_TIME_UNIT ")\r\n"),
                                5);
    }
  }

  /* Write log line to buffer */
  fio___http_write_pid(&buf);
  buf.buf[buf.len++] = ' ';
  fio_http_from(&buf, h);
  FIO_MEMCPY(buf.buf + buf.len, " - - ", 5);
  FIO_MEMCPY(buf.buf + buf.len + 5, date.buf, date.len);
  buf.len += date.len + 5;
  buf.buf[buf.len++] = ' ';
  buf.buf[buf.len++] = '\"';
  fio_string_write2 FIO_NOOP(&buf, NULL, to_write);

  if (buf.buf[buf.len - 1] != '\n')
    buf.buf[buf.len++] = '\n'; /* log was truncated, data too long */

  /* Write log line to STDOUT */
  fwrite(buf.buf, 1, buf.len, stdout);
  h->received_at = time_end;
}

/* *****************************************************************************
ETag helper
***************************************************************************** */

/** Drops the weak `W/` prefix, leaving the opaque-tag (RFC 9110 §8.8.3). */
FIO_IFUNC fio_str_info_s fio___http_etag_opaque(fio_str_info_s t) {
  if (t.len > 2 && t.buf[0] == 'W' && t.buf[1] == '/') {
    t.buf += 2;
    t.len -= 2;
  }
  return t;
}

/**
 * Evaluates `If-None-Match` for a `GET` / `HEAD` request (RFC 9110 §13.1.2).
 *
 * Uses the weak comparison function (a `W/` prefix is ignored on both sides)
 * and checks every list member of every `If-None-Match` line. A field value of
 * `*` matches any current representation (even without an `etag`); `*` mixed
 * with other members is invalid and never matches. Evaluated only when the
 * response status is 2xx (RFC 9110 §13.2.1).
 */
SFUNC int fio_http_etag_is_match(fio_http_s *h) {
  if (!h || h->status < 200 || h->status > 299)
    return 0;
  if (!fio_http_method_is_get(h) && !fio_http_method_is_head(h))
    return 0;
  fio_str_info_s etag = fio___http_etag_opaque(
      fio___http_hmap_get2(HTTP_HDR_RESPONSE(h),
                           FIO_STR_INFO2((char *)"etag", 4),
                           0));
  { /* `*` is only valid as the whole field value */
    fio_str_info_s v =
        fio___http_hmap_get2(HTTP_HDR_REQUEST(h),
                             FIO_STR_INFO2((char *)"if-none-match", 13),
                             0);
    while (v.len && (v.buf[0] == ' ' || v.buf[0] == '\t')) {
      ++v.buf;
      --v.len;
    }
    while (v.len && (v.buf[v.len - 1] == ' ' || v.buf[v.len - 1] == '\t'))
      --v.len;
    if (v.len == 1 && v.buf[0] == '*')
      return fio_http_request_header_count(
                 h,
                 FIO_STR_INFO2((char *)"if-none-match", 13)) == 1;
  }
  for (int32_t i = 0;; ++i) {
    fio_str_info_s cond =
        fio___http_hmap_get2(HTTP_HDR_REQUEST(h),
                             FIO_STR_INFO2((char *)"if-none-match", 13),
                             i);
    if (!cond.buf)
      return 0;
    char *pos = cond.buf;
    char *end = cond.buf + cond.len;
    while (pos < end) {
      if (pos[0] == ',' || pos[0] == ' ' || pos[0] == '\t') {
        ++pos;
        continue;
      }
      char *tag = pos;
      if (end - pos > 2 && pos[0] == 'W' && pos[1] == '/' && pos[2] == '"')
        pos += 2;
      if (pos[0] == '"') { /* quoted opaque-tag: may contain commas */
        pos = (char *)FIO_MEMCHR(pos + 1, '"', (size_t)(end - (pos + 1)));
        if (!pos)
          break; /* malformed: unterminated entity-tag */
        ++pos;
      } else { /* a lenient unquoted tag (or an invalid `*` list member) */
        while (pos < end && pos[0] != ',' && pos[0] != ' ' && pos[0] != '\t')
          ++pos;
      }
      fio_str_info_s member =
          fio___http_etag_opaque(FIO_STR_INFO2(tag, (size_t)(pos - tag)));
      if (etag.len && member.len == etag.len &&
          !FIO_MEMCMP(member.buf, etag.buf, etag.len))
        return 1;
      while (pos < end && pos[0] != ',') /* skip junk up to the next member */
        ++pos;
    }
  }
}

/* *****************************************************************************
HTTP Body Parsing - Content-Type Detection
***************************************************************************** */

/** Content-type enum for body parsing. */
typedef enum {
  FIO___HTTP_BODY_CONTENT_TYPE_UNKNOWN = 0,
  FIO___HTTP_BODY_CONTENT_TYPE_JSON,
  FIO___HTTP_BODY_CONTENT_TYPE_URLENCODED,
  FIO___HTTP_BODY_CONTENT_TYPE_MULTIPART,
} fio___http_body_content_type_e;

/** Detects the content type from the Content-Type header. */
FIO_SFUNC fio___http_body_content_type_e
fio___http_body_detect_content_type(fio_http_s *h) {
  fio_str_info_s ct =
      fio_http_request_header(h, FIO_STR_INFO2((char *)"content-type", 12), 0);
  if (!ct.len)
    return FIO___HTTP_BODY_CONTENT_TYPE_UNKNOWN;

  while (ct.len && (*ct.buf == ' ' || *ct.buf == '\t')) {
    ++ct.buf;
    --ct.len;
  }
  size_t len = 0;
  while (len < ct.len && ct.buf[len] != ';' && ct.buf[len] != ' ' &&
         ct.buf[len] != '\t')
    ++len;
  ct.len = len;

  /* application/json */
  if (ct.len == 16) {
    /* Compare the complete JSON media type case-insensitively. */
    uint64_t w0 = fio_buf2u64u(ct.buf) | 0x2020202020202020ULL;
    uint64_t w1 = fio_buf2u64u(ct.buf + 8) | 0x2020202020202020ULL;
    if (w0 == (fio_buf2u64u("applicat") | 0x2020202020202020ULL) &&
        w1 == (fio_buf2u64u("ion/json") | 0x2020202020202020ULL))
      return FIO___HTTP_BODY_CONTENT_TYPE_JSON;
  }

  /* text/json */
  if (ct.len == 9 &&
      (fio_buf2u64u(ct.buf) | 0x2020202020202020ULL) ==
          fio_buf2u64u("text/jso") &&
      (ct.buf[8] | 0x20) == 'n')
    return FIO___HTTP_BODY_CONTENT_TYPE_JSON;

  /* application/x-json */
  if (ct.len == 18 &&
      (fio_buf2u64u(ct.buf) | 0x2020202020202020ULL) ==
          fio_buf2u64u("applicat") &&
      (fio_buf2u64u(ct.buf + 8) | 0x2020202020202020ULL) ==
          fio_buf2u64u("ion/x-js") &&
      (fio_buf2u16u(ct.buf + 16) | 0x2020) == fio_buf2u16u("on"))
    return FIO___HTTP_BODY_CONTENT_TYPE_JSON;

  /* application/x-www-form-urlencoded */
  if (ct.len == 33) {
    uint64_t w0 = fio_buf2u64u(ct.buf) | 0x2020202020202020ULL;
    uint64_t w1 = fio_buf2u64u(ct.buf + 8) | 0x2020202020202020ULL;
    uint64_t w2 = fio_buf2u64u(ct.buf + 16) | 0x2020202020202020ULL;
    uint64_t w3 = fio_buf2u64u(ct.buf + 24) | 0x2020202020202020ULL;
    if (w0 == (fio_buf2u64u("applicat") | 0x2020202020202020ULL) &&
        w1 == (fio_buf2u64u("ion/x-ww") | 0x2020202020202020ULL) &&
        w2 == (fio_buf2u64u("w-form-u") | 0x2020202020202020ULL) &&
        w3 == (fio_buf2u64u("rlencoded") | 0x2020202020202020ULL))
      return FIO___HTTP_BODY_CONTENT_TYPE_URLENCODED;
  }

  /* multipart/form-data */
  if (ct.len == 19) {
    uint64_t w0 = fio_buf2u64u(ct.buf) | 0x2020202020202020ULL;
    uint64_t w1 = fio_buf2u64u(ct.buf + 8) | 0x2020202020202020ULL;
    if (w0 == (fio_buf2u64u("multipar") | 0x2020202020202020ULL) &&
        w1 == (fio_buf2u64u("t/form-d") | 0x2020202020202020ULL) &&
        ((fio_buf2u16u(ct.buf + 16) | 0x2020) ==
         (fio_buf2u16u("at") | 0x2020)) &&
        ((ct.buf[18] | 0x20) == 'a'))
      return FIO___HTTP_BODY_CONTENT_TYPE_MULTIPART;
  }

  /* application/<non-empty subtype>+json */
  if (ct.len > 17) {
    const char *prefix = "application/";
    const char *suffix = "+json";
    size_t i = 0;
    for (; i < 12 && ((ct.buf[i] | 0x20) == prefix[i]); ++i)
      ;
    if (i == 12) {
      for (i = 0; i < 5 && ((ct.buf[ct.len - 5 + i] | 0x20) == suffix[i]); ++i)
        ;
      if (i == 5)
        return FIO___HTTP_BODY_CONTENT_TYPE_JSON;
    }
  }

  return FIO___HTTP_BODY_CONTENT_TYPE_UNKNOWN;
}

/* *****************************************************************************
Params Parsing - Shared Map Helpers
***************************************************************************** */

typedef struct {
  fio_buf_info_s seg[FIO_JSON_MAX_DEPTH]; /* slices into `name` */
  uint32_t count;                         /* >= 1 on success */
  uint8_t append; /* 1 if the name ended with a terminal `[]` */
} fio___http_params_path_s;

/** Splits a decoded name; invalid paths remain literal, -1 means too deep. */
FIO_SFUNC int fio___http_params_path_parse(fio___http_params_path_s *p,
                                           fio_buf_info_s name) {
  size_t at = 0;
  int too_deep = 0;
  p->count = 0;
  p->append = 0;
  if (!name.len)
    goto literal;
  for (;;) {
    size_t start;
    if (at && name.buf[at] == '[') {
      ++at;
      start = at;
      while (at < name.len && name.buf[at] != ']') {
        if (name.buf[at] == '[')
          goto literal;
        ++at;
      }
      if (at == name.len)
        goto literal;
      if (at == start) {
        if (at + 1 == name.len && p->count) {
          p->append = 1;
          return too_deep || p->count >= FIO_JSON_MAX_DEPTH ? -1 : 0;
        }
        goto literal;
      }
      if (p->count >= FIO_JSON_MAX_DEPTH)
        too_deep = 1;
      else
        p->seg[p->count++] =
            (fio_buf_info_s){.buf = name.buf + start, .len = at - start};
      ++at; /* closing ']' */
      if (at < name.len && name.buf[at] != '.' && name.buf[at] != '[')
        goto literal;
    } else {
      if (at && name.buf[at] != '.')
        goto literal;
      if (at)
        ++at;
      start = at;
      while (at < name.len && name.buf[at] != '.' && name.buf[at] != '[') {
        if (name.buf[at] == ']')
          goto literal;
        ++at;
      }
      if (at == start)
        goto literal;
      if (p->count >= FIO_JSON_MAX_DEPTH)
        too_deep = 1;
      else
        p->seg[p->count++] =
            (fio_buf_info_s){.buf = name.buf + start, .len = at - start};
    }
    if (at == name.len)
      return too_deep ? -1 : 0;
  }
literal:
  p->seg[0] = name;
  p->count = 1;
  p->append = 0;
  return 0;
}

FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, on_null) ==
                      offsetof(fio_json_parser_callbacks_s, on_null),
                  "HTTP / JSON callback offset mismatch: on_null");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, on_true) ==
                      offsetof(fio_json_parser_callbacks_s, on_true),
                  "HTTP / JSON callback offset mismatch: on_true");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, on_false) ==
                      offsetof(fio_json_parser_callbacks_s, on_false),
                  "HTTP / JSON callback offset mismatch: on_false");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, on_number) ==
                      offsetof(fio_json_parser_callbacks_s, on_number),
                  "HTTP / JSON callback offset mismatch: on_number");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, on_float) ==
                      offsetof(fio_json_parser_callbacks_s, on_float),
                  "HTTP / JSON callback offset mismatch: on_float");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, on_string) ==
                      offsetof(fio_json_parser_callbacks_s, on_string),
                  "HTTP / JSON callback offset mismatch: on_string");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s,
                           on_string_simple) ==
                      offsetof(fio_json_parser_callbacks_s, on_string_simple),
                  "HTTP / JSON callback offset mismatch: on_string_simple");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, on_map) ==
                      offsetof(fio_json_parser_callbacks_s, on_map),
                  "HTTP / JSON callback offset mismatch: on_map");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, on_array) ==
                      offsetof(fio_json_parser_callbacks_s, on_array),
                  "HTTP / JSON callback offset mismatch: on_array");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, map_push) ==
                      offsetof(fio_json_parser_callbacks_s, map_push),
                  "HTTP / JSON callback offset mismatch: map_push");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, array_push) ==
                      offsetof(fio_json_parser_callbacks_s, array_push),
                  "HTTP / JSON callback offset mismatch: array_push");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, is_array) ==
                      offsetof(fio_json_parser_callbacks_s, is_array),
                  "HTTP / JSON callback offset mismatch: is_array");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, is_map) ==
                      offsetof(fio_json_parser_callbacks_s, is_map),
                  "HTTP / JSON callback offset mismatch: is_map");
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s,
                           free_unused_object) ==
                      offsetof(fio_json_parser_callbacks_s, free_unused_object),
                  "HTTP / JSON callback offset mismatch: free_unused_object");
/* The HTTP table is passed to the JSON parser through a pointer cast; its
 * head MUST keep the JSON table layout. */
FIO_ASSERT_STATIC(offsetof(fio_http_params_parse_callbacks_s, on_file) ==
                      offsetof(fio_json_parser_callbacks_s,
                               free_unused_object) +
                          sizeof(void *),
                  "HTTP / JSON callback head size mismatch");
/* 15 callbacks in a 64 byte aligned table: 2 cache lines (1 on 32 bit) */
FIO_ASSERT_STATIC(sizeof(fio_http_params_parse_callbacks_s) ==
                      ((15 * sizeof(void *) + 63) & ~(size_t)63),
                  "HTTP params callback table should be two cache lines");

/* Views the HTTP callback table as the JSON table it starts with. */
#define FIO___HTTP_PARAMS_JSON_CB(cb)                                          \
  ((const fio_json_parser_callbacks_s *)(cb))

/* Creates the root map. Returns 0 or an unchanged callback error. */
FIO_SFUNC int fio___http_params_map_new(
    const fio_http_params_parse_callbacks_s *cb,
    void *udata,
    void **map) {
  fio_json_obj_result_s r =
      fio_json_resolve_on_map(FIO___HTTP_PARAMS_JSON_CB(cb),
                              udata,
                              NULL,
                              (fio_buf_info_s){0});
  *map = r.err ? NULL : r.obj;
  return r.err;
}

/* Creates an owned String value for decoded form text. */
FIO_IFUNC fio_json_obj_result_s
fio___http_params_string(const fio_http_params_parse_callbacks_s *cb,
                         void *udata,
                         fio_buf_info_s text) {
  return (cb->on_string_simple ? cb->on_string_simple : cb->on_string)(udata,
                                                                       text);
}

/* Stores an owned form value (taken even on error) at its decoded, possibly
 * nested field path. Returns 0 or an error (callback errors unchanged). */
FIO_SFUNC int fio___http_params_store(
    const fio_http_params_parse_callbacks_s *cb,
    void *udata,
    void *root,
    fio_buf_info_s name,
    void *value) {
  const fio_json_parser_callbacks_s *json = FIO___HTTP_PARAMS_JSON_CB(cb);
  fio_json_obj_result_s c = {.obj = root};
  fio___http_params_path_s path;
  if (fio___http_params_path_parse(&path, name)) {
    FIO_LOG_SECURITY("HTTP params name depth limit exceeded (%u)",
                     (unsigned)FIO_JSON_MAX_DEPTH);
    c.err = FIO_HTTP_PARAMS_PARSE_DEPTH;
    goto error;
  }
  for (uint32_t i = 0; i + 1 < path.count; ++i) {
    c = fio_json_resolve_on_map(json, udata, c.obj, path.seg[i]);
    if (c.err)
      goto error;
  }
  if (path.append) {
    c = fio_json_resolve_on_array(json, udata, c.obj, path.seg[path.count - 1]);
    if (c.err)
      goto error;
    return cb->array_push(udata, c.obj, value); /* takes `value` */
  }
  return fio_json_resolve_map_push(json,
                                   udata,
                                   c.obj,
                                   path.seg[path.count - 1],
                                   value); /* takes `value` */
error:
  cb->free_unused_object(udata, value);
  return c.err;
}

/* Builds a parse result; on error `map` is the caller-owned partial root. */
FIO_IFUNC fio_http_params_parse_result_s
fio___http_params_finish(void *map, size_t consumed, size_t count, int err) {
  fio_http_params_parse_result_s r = {
      .result = map,
      .consumed = consumed,
      .count = count,
      .err = err,
  };
  return r;
}

/**
 * Reads the whole body (from its start) to `body`.
 *
 * A spilled body is memory mapped where possible; a short read (i.e., low
 * memory without mapping) returns -1.
 */
FIO_SFUNC int fio___http_body_read_all(fio_http_s *h, fio_str_info_s *body) {
  size_t len = fio_http_body_length(h);
  fio_http_body_seek(h, 0);
  *body = fio_http_body_read(h, len);
  if (body->len == len)
    return 0;
  FIO_LOG_ERROR("HTTP body couldn't be read whole for parsing (%zu of %zu)",
                body->len,
                len);
  return -1;
}

/* RAM bodies are guarded bstrs; file views need a terminated copy. */
FIO_SFUNC fio_json_result_s
fio___http_json_parse_guarded(fio_http_s *h,
                              const fio_json_parser_callbacks_s *callbacks,
                              void *udata,
                              void *root,
                              fio_str_info_s body) {
  char storage[256];
  fio_str_info_s copy = FIO_STR_INFO3(storage, 0, sizeof(storage));
  if (fio_data_fd(&h->body) != -1) {
    if (fio_string_write(&copy, FIO_STRING_ALLOC_COPY, body.buf, body.len))
      return (fio_json_result_s){.ctx = root, .err = FIO_HTTP_PARAMS_PARSE_IO};
    body.buf = copy.buf;
  }
  fio_json_result_s r =
      fio_json_parse(callbacks, udata, root, body.buf, body.len);
  if (copy.buf != storage)
    FIO_STRING_FREE2(copy);
  return r;
}

FIO_SFUNC fio_http_params_parse_result_s
fio___http_body_parse_json(fio_http_s *h,
                           const fio_http_params_parse_callbacks_s *callbacks,
                           void *udata) {
  fio_http_params_parse_result_s result = {.result = NULL,
                                           .consumed = 0,
                                           .err = 0};
  fio_str_info_s body;
  if (fio___http_body_read_all(h, &body)) {
    result.err = FIO_HTTP_PARAMS_PARSE_IO;
    return result;
  }
  if (!body.len || !body.buf)
    return result; /* empty body - nothing to parse */
  fio_json_result_s r =
      fio___http_json_parse_guarded(h,
                                    FIO___HTTP_PARAMS_JSON_CB(callbacks),
                                    udata,
                                    NULL,
                                    body);
  result.result = r.ctx; /* caller owned, the partial root on error */
  result.consumed = r.stop_pos;
  result.err = r.err;
  return result;
}

/** Checks the JSON prefix only. Returns -1 on a failed body read. */
FIO_SFUNC int fio___http_json_is_object(fio_http_s *h) {
  size_t remaining = fio_http_body_length(h);
  int first = 1;
  fio_http_body_seek(h, 0);
  while (remaining) {
    size_t wanted = remaining < 256 ? remaining : 256;
    fio_str_info_s body = fio_http_body_read(h, wanted);
    if (body.len != wanted || !body.buf)
      return -1;
    const unsigned char *p = (const unsigned char *)body.buf;
    size_t i = 0;
    if (first && body.len >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF)
      i = 3;
    first = 0;
    for (; i < body.len; ++i) {
      if (p[i] != ' ' && p[i] != '\t' && p[i] != '\r' && p[i] != '\n')
        return p[i] == '{';
    }
    remaining -= body.len;
  }
  return 0;
}

/* *****************************************************************************
URL-Encoded Body Parsing - Adapter Implementation
***************************************************************************** */

/** Per-call field count and configured limit. */
typedef struct {
  size_t count;
  size_t limit;
} fio___http_params_field_state_s;

FIO_SFUNC size_t fio___http_params_limit(fio_http_s *h) {
  fio_http_settings_s *s = fio_http_settings(h);
  return s && s->max_params ? s->max_params : FIO_HTTP_PARAMS_MAX_FIELDS;
}

FIO_SFUNC int fio___http_params_overflow(
    fio___http_params_field_state_s *fields) {
  FIO_LOG_WARNING("HTTP params field limit exceeded (%zu)", fields->limit);
  return FIO_HTTP_PARAMS_PARSE_ERROR;
}

/** Adapter context for URL-encoded parsing. */
typedef struct {
  const fio_http_params_parse_callbacks_s *callbacks;
  void *udata;
  void *map;
  fio___http_params_field_state_s *fields;
} fio___http_urlenc_adapter_s;

/** Adapter callback for URL-encoded on_pair (errors pass through). */
FIO_SFUNC int fio___http_urlenc_on_pair(void *udata_,
                                        fio_buf_info_s name,
                                        fio_buf_info_s value) {
  fio___http_urlenc_adapter_s *a = (fio___http_urlenc_adapter_s *)udata_;
  if (a->fields->count >= a->fields->limit)
    return fio___http_params_overflow(a->fields);
  ++a->fields->count;
  /* Decoded names are borrowed only for this callback. */
  fio_json_obj_result_s v =
      fio___http_params_string(a->callbacks, a->udata, value);
  return v.err ? v.err
               : fio___http_params_store(a->callbacks,
                                         a->udata,
                                         a->map,
                                         name,
                                         v.obj);
}

/** Static adapter callbacks for URL-encoded parsing. */
static const fio_url_encoded_parser_callbacks_s
    fio___http_urlenc_adapter_callbacks = {
        .on_pair = fio___http_urlenc_on_pair,
};

/** Parses URL-encoded data into an existing map. Returns non-zero on error. */
FIO_SFUNC int fio___http_params_fill_urlencoded(
    fio_str_info_s data,
    const fio_http_params_parse_callbacks_s *callbacks,
    void *udata,
    void *map,
    size_t *consumed,
    fio___http_params_field_state_s *fields) {
  if (!data.len || !data.buf)
    return FIO_HTTP_PARAMS_PARSE_SUCCESS;
  fio___http_urlenc_adapter_s adapter = {
      .callbacks = callbacks,
      .udata = udata,
      .map = map,
      .fields = fields,
  };
  fio_url_encoded_result_s r =
      fio_url_encoded_parse(&fio___http_urlenc_adapter_callbacks,
                            &adapter,
                            data.buf,
                            data.len);
  *consumed += r.consumed;
  return r.err;
}

/** Parses URL-encoded data into a new map (shared by body and query). */
FIO_SFUNC fio_http_params_parse_result_s fio___http_parse_urlencoded_buf(
    fio_str_info_s data,
    const fio_http_params_parse_callbacks_s *callbacks,
    void *udata,
    size_t limit) {
  size_t consumed = 0;
  fio___http_params_field_state_s fields = {.limit = limit};
  void *map = NULL;
  if (!callbacks)
    return (fio_http_params_parse_result_s){.err = FIO_HTTP_PARAMS_PARSE_ERROR};
  int err = fio___http_params_map_new(callbacks, udata, &map);
  if (!err)
    err = fio___http_params_fill_urlencoded(data,
                                            callbacks,
                                            udata,
                                            map,
                                            &consumed,
                                            &fields);
  return fio___http_params_finish(map, consumed, fields.count, err);
}

FIO_SFUNC fio_http_params_parse_result_s fio___http_body_parse_urlencoded(
    fio_http_s *h,
    const fio_http_params_parse_callbacks_s *callbacks,
    void *udata,
    size_t limit) {
  fio_str_info_s body;
  if (fio___http_body_read_all(h, &body))
    return (fio_http_params_parse_result_s){.err = FIO_HTTP_PARAMS_PARSE_IO};
  return fio___http_parse_urlencoded_buf(body, callbacks, udata, limit);
}

/* *****************************************************************************
Multipart Body Parsing - Adapter Context
***************************************************************************** */

/** Context passed to multipart parser callbacks. */
typedef struct {
  const fio_http_params_parse_callbacks_s *callbacks;
  /* `callbacks->on_file`, or a no-op default (set once) */
  fio_json_obj_result_s (*on_file)(void *udata,
                                   fio_str_info_s name,
                                   fio_str_info_s filename,
                                   fio_str_info_s content_type,
                                   fio_str_info_s headers,
                                   fio_buf_info_s data);
  void *udata;
  void *map; /* The result map being built */
  fio___http_params_field_state_s *fields;
} fio___http_multipart_adapter_s;

/** Default HTTP `on_file`: skips the upload (stores nothing). */
FIO_SFUNC fio_json_obj_result_s
fio___http_multipart_on_file_noop(void *udata,
                                  fio_str_info_s name,
                                  fio_str_info_s filename,
                                  fio_str_info_s content_type,
                                  fio_str_info_s headers,
                                  fio_buf_info_s data) {
  (void)udata, (void)name, (void)filename, (void)content_type, (void)headers,
      (void)data;
  return (fio_json_obj_result_s){0};
}

/* *****************************************************************************
Multipart Body Parsing - Adapter Callbacks (errors propagate unchanged)
***************************************************************************** */

/** Called for each regular form field (complete value). */
FIO_SFUNC int fio___http_multipart_on_string(void *udata,
                                             fio_buf_info_s name,
                                             fio_buf_info_s value,
                                             fio_buf_info_s content_type) {
  fio___http_multipart_adapter_s *a = (fio___http_multipart_adapter_s *)udata;
  if (a->fields->count >= a->fields->limit)
    return fio___http_params_overflow(a->fields);
  ++a->fields->count;
  fio_json_obj_result_s v =
      fio___http_params_string(a->callbacks, a->udata, value);
  (void)content_type;
  return v.err ? v.err
               : fio___http_params_store(a->callbacks,
                                         a->udata,
                                         a->map,
                                         name,
                                         v.obj);
}

/** Called for each complete file upload. */
FIO_SFUNC int fio___http_multipart_on_file(void *udata,
                                           fio_buf_info_s name,
                                           fio_buf_info_s filename,
                                           fio_buf_info_s content_type,
                                           fio_buf_info_s headers,
                                           fio_buf_info_s data) {
  fio___http_multipart_adapter_s *a = (fio___http_multipart_adapter_s *)udata;
  if (a->fields->count >= a->fields->limit)
    return fio___http_params_overflow(a->fields);
  ++a->fields->count;
  fio___http_params_path_s path;
  if (fio___http_params_path_parse(&path, name)) {
    FIO_LOG_WARNING("HTTP params name depth limit exceeded (%u)",
                    (unsigned)FIO_JSON_MAX_DEPTH);
    return FIO_HTTP_PARAMS_PARSE_DEPTH;
  }
  if (!name.buf || !name.len || !filename.buf)
    return 0; /* skip unusable uploads */
  for (size_t i = filename.len; i; --i) {
    if (filename.buf[i - 1] == '/' || filename.buf[i - 1] == '\\') {
      filename.buf += i;
      filename.len -= i;
      break;
    }
  }
  if (!filename.len)
    return 0;
  fio_json_obj_result_s f = a->on_file(a->udata,
                                       FIO_BUF2STR_INFO(name),
                                       FIO_BUF2STR_INFO(filename),
                                       FIO_BUF2STR_INFO(content_type),
                                       FIO_BUF2STR_INFO(headers),
                                       data);
  if (f.err)
    return f.err;
  if (!f.obj) /* nothing to store (handled by the callback) */
    return 0;
  return fio___http_params_store(a->callbacks, a->udata, a->map, name, f.obj);
}

/** Static callback adapter mapping multipart parser to HTTP body parse. */
static const fio_multipart_parser_callbacks_s
    fio___http_multipart_adapter_callbacks = {
        .on_string = fio___http_multipart_on_string,
        .on_file = fio___http_multipart_on_file,
};

/* *****************************************************************************
Multipart Body Parsing - Helper: Extract boundary from Content-Type
***************************************************************************** */

/**
 * Extracts the boundary parameter from a Content-Type header value.
 * Returns empty fio_buf_info_s if boundary not found.
 */
FIO_SFUNC fio_buf_info_s
fio___http_multipart_extract_boundary(fio_str_info_s content_type) {
  fio_buf_info_s boundary = FIO_BUF_INFO0;
  if (!content_type.buf || !content_type.len)
    return boundary;

  /* Search for "boundary=" (case-insensitive search for parameter) */
  const char *p = content_type.buf;
  const char *end = content_type.buf + content_type.len;

  while (p < end) {
    /* Skip to next semicolon or start */
    while (p < end && *p != ';')
      ++p;
    if (p >= end)
      break;
    ++p; /* Skip ';' */

    /* Skip whitespace */
    while (p < end && (*p == ' ' || *p == '\t'))
      ++p;

    /* Check for "boundary" (case-insensitive, 8 chars) */
    if ((size_t)(end - p) >= 9) { /* "boundary=" is 9 chars */
      char c0 = p[0] | 0x20;
      char c1 = p[1] | 0x20;
      char c2 = p[2] | 0x20;
      char c3 = p[3] | 0x20;
      char c4 = p[4] | 0x20;
      char c5 = p[5] | 0x20;
      char c6 = p[6] | 0x20;
      char c7 = p[7] | 0x20;
      if (c0 == 'b' && c1 == 'o' && c2 == 'u' && c3 == 'n' && c4 == 'd' &&
          c5 == 'a' && c6 == 'r' && c7 == 'y' && p[8] == '=') {
        p += 9; /* Skip "boundary=" */

        /* Skip optional whitespace after '=' */
        while (p < end && (*p == ' ' || *p == '\t'))
          ++p;

        if (p >= end)
          break;

        /* Extract value - quoted or unquoted */
        if (*p == '"') {
          ++p;
          const char *value_start = p;
          while (p < end && *p != '"')
            ++p;
          boundary.buf = (char *)value_start;
          boundary.len = (size_t)(p - value_start);
        } else {
          const char *value_start = p;
          while (p < end && *p != ';' && *p != ' ' && *p != '\t' &&
                 *p != '\r' && *p != '\n')
            ++p;
          boundary.buf = (char *)value_start;
          boundary.len = (size_t)(p - value_start);
        }
        break;
      }
    }
  }

  return boundary;
}

/* *****************************************************************************
Multipart Body Parsing - Main Implementation
***************************************************************************** */

#ifndef FIO___HTTP_PARAMS_CHUNK
/** Multipart bodies are fed to the parser in reads of (at least) this size. */
#define FIO___HTTP_PARAMS_CHUNK (1UL << 16)
#endif

FIO_ASSERT_STATIC(FIO_MULTIPART_ERROR_GENERIC == FIO_HTTP_PARAMS_PARSE_ERROR,
                  "multipart errors are returned as HTTP params errors");

/**
 * Parses a multipart body into an existing map.
 *
 * The body is read straight from its store: RAM and memory mapped bodies are
 * never copied (pointer math only); without a mapping, the body's own read
 * buffer is used. Each round reads the unconsumed (pending) part again plus
 * more; the read grows geometrically while a part is incomplete, and the
 * parser resumes its search, so total work is linear. A read is borrowed until
 * the next body call, so it's parsed before reading on.
 *
 * Returns non-zero on error.
 */
FIO_SFUNC int fio___http_params_fill_multipart(
    fio_http_s *h,
    const fio_http_params_parse_callbacks_s *callbacks,
    void *udata,
    void *map,
    size_t *consumed,
    fio___http_params_field_state_s *fields) {
  fio_multipart_s parser;
  if (fio_multipart_init(
          &parser,
          fio___http_multipart_extract_boundary(
              fio_http_request_header(h,
                                      FIO_STR_INFO2((char *)"content-type", 12),
                                      0))))
    return FIO_HTTP_PARAMS_PARSE_ERROR;
  fio___http_multipart_adapter_s adapter = {
      .callbacks = callbacks,
      .on_file = callbacks->on_file ? callbacks->on_file
                                    : fio___http_multipart_on_file_noop,
      .udata = udata,
      .map = map,
      .fields = fields,
  };
  const size_t total = fio_http_body_length(h);
  size_t base = 0; /* body offset of the first unconsumed byte */
  size_t want = FIO___HTTP_PARAMS_CHUNK;
  for (;;) {
    fio_str_info_s in = {0};
    if (base < total) {
      fio_http_body_seek(h, (ssize_t)base);
      in = fio_http_body_read(h, want);
      if (!in.len)
        return FIO_HTTP_PARAMS_PARSE_IO;
    }
    const int eof = (base + in.len >= total);
    fio_multipart_result_s r =
        fio_multipart_parse(&parser,
                            &fio___http_multipart_adapter_callbacks,
                            &adapter,
                            in.buf,
                            in.len,
                            eof);
    *consumed += r.consumed;
    if (r.err) /* malformed or truncated input (ERROR), or a callback's error */
      return r.err;
    if (eof)
      return FIO_HTTP_PARAMS_PARSE_SUCCESS;
    base += r.consumed;
    size_t keep = in.len - r.consumed;
    want = keep +
           (keep > FIO___HTTP_PARAMS_CHUNK ? keep : FIO___HTTP_PARAMS_CHUNK);
  }
}

FIO_SFUNC fio_http_params_parse_result_s fio___http_body_parse_multipart(
    fio_http_s *h,
    const fio_http_params_parse_callbacks_s *callbacks,
    void *udata,
    size_t limit) {
  size_t consumed = 0;
  fio___http_params_field_state_s fields = {.limit = limit};
  void *map = NULL;
  if (!fio_http_body_length(h))
    return (fio_http_params_parse_result_s){0}; /* empty body - not an error */
  int err = fio___http_params_map_new(callbacks, udata, &map);
  if (!err)
    err = fio___http_params_fill_multipart(h,
                                           callbacks,
                                           udata,
                                           map,
                                           &consumed,
                                           &fields);
  return fio___http_params_finish(map, consumed, fields.count, err);
}

/* *****************************************************************************
HTTP Body Parsing - Main Implementation
***************************************************************************** */

/**
 * Parses the HTTP request body, auto-detecting content type.
 *
 * Supports JSON, URL-encoded, and multipart/form-data bodies.
 * Calls the appropriate callbacks for each element found.
 */
SFUNC fio_http_params_parse_result_s fio_http_body_parse
FIO_NOOP(fio_http_s *h, fio_http_params_parse_args_s args) {
  fio_http_params_parse_result_s result = {.result = NULL,
                                           .consumed = 0,
                                           .err = 0};
  const fio_http_params_parse_callbacks_s *callbacks = args.callbacks;
  void *udata = args.udata;

  if (!h || !callbacks) {
    result.err = FIO_HTTP_PARAMS_PARSE_ERROR;
    return result;
  }
  size_t position = fio_http_body_pos(h);
  const size_t limit = args.limit ? args.limit : fio___http_params_limit(h);

  if (fio_json_callbacks_validate(FIO___HTTP_PARAMS_JSON_CB(callbacks))) {
    result.err = FIO_HTTP_PARAMS_PARSE_ERROR;
    goto done;
  }

  /* Detect content type from header */
  fio___http_body_content_type_e content_type =
      fio___http_body_detect_content_type(h);

  /* Route to appropriate parser */
  switch (content_type) {
  case FIO___HTTP_BODY_CONTENT_TYPE_JSON:
    result = fio___http_body_parse_json(h, callbacks, udata);
    break;

  case FIO___HTTP_BODY_CONTENT_TYPE_URLENCODED:
    result = fio___http_body_parse_urlencoded(h, callbacks, udata, limit);
    break;

  case FIO___HTTP_BODY_CONTENT_TYPE_MULTIPART:
    result = fio___http_body_parse_multipart(h, callbacks, udata, limit);
    break;

  case FIO___HTTP_BODY_CONTENT_TYPE_UNKNOWN: /* fall through */
  default:
    /* Unknown content type - return error (no root was created) */
    result.err = FIO_HTTP_PARAMS_PARSE_ERROR;
    break;
  }
done:
  fio_http_body_seek(h, (ssize_t)position);
  return result;
}

/* *****************************************************************************
HTTP Query Parsing
***************************************************************************** */

/** Parses the HTTP query string as URL-encoded data into a map. */
SFUNC fio_http_params_parse_result_s fio_http_query_parse
FIO_NOOP(fio_http_s *h, fio_http_params_parse_args_s args) {
  if (!h || !args.callbacks)
    return (fio_http_params_parse_result_s){.err = FIO_HTTP_PARAMS_PARSE_ERROR};
  if (fio_json_callbacks_validate(FIO___HTTP_PARAMS_JSON_CB(args.callbacks)))
    return (fio_http_params_parse_result_s){.err = FIO_HTTP_PARAMS_PARSE_ERROR};
  return fio___http_parse_urlencoded_buf(
      fio_http_query(h),
      args.callbacks,
      args.udata,
      args.limit ? args.limit : fio___http_params_limit(h));
}

/* *****************************************************************************
HTTP Params Parsing (query + body)
***************************************************************************** */

/** Parses the query and then the body (if any) into a single map. A JSON
 * object body updates the map in place (the map is the explicit JSON root). */
SFUNC fio_http_params_parse_result_s fio_http_params_parse
FIO_NOOP(fio_http_s *h, fio_http_params_parse_args_s args) {
  size_t consumed = 0;
  fio___http_params_field_state_s fields = {0};
  void *map = NULL;
  int err;
  const fio_http_params_parse_callbacks_s *callbacks = args.callbacks;
  void *udata = args.udata;
  if (!h || !callbacks)
    return (fio_http_params_parse_result_s){.err = FIO_HTTP_PARAMS_PARSE_ERROR};
  fields.limit = args.limit ? args.limit : fio___http_params_limit(h);
  size_t position = fio_http_body_pos(h);
  if (fio_json_callbacks_validate(FIO___HTTP_PARAMS_JSON_CB(callbacks))) {
    err = FIO_HTTP_PARAMS_PARSE_ERROR;
    goto finish;
  }
  err = fio___http_params_map_new(callbacks, udata, &map);
  if (err)
    goto finish;
  err = fio___http_params_fill_urlencoded(fio_http_query(h),
                                          callbacks,
                                          udata,
                                          map,
                                          &consumed,
                                          &fields);
  if (err || !fio_http_body_length(h))
    goto finish;

  switch (fio___http_body_detect_content_type(h)) {
  case FIO___HTTP_BODY_CONTENT_TYPE_JSON: {
    int is_object = fio___http_json_is_object(h);
    if (is_object < 0)
      err = FIO_HTTP_PARAMS_PARSE_IO;
    else if (is_object > 0) {
      fio_str_info_s body;
      if (fio___http_body_read_all(h, &body)) {
        err = FIO_HTTP_PARAMS_PARSE_IO;
        break;
      }
      /* `map` is the explicit root: updated in place, kept on error. */
      fio_json_result_s r =
          fio___http_json_parse_guarded(h,
                                        FIO___HTTP_PARAMS_JSON_CB(callbacks),
                                        udata,
                                        map,
                                        body);
      consumed += r.stop_pos;
      err = r.err;
    }
    break;
  }
  case FIO___HTTP_BODY_CONTENT_TYPE_URLENCODED: {
    fio_str_info_s body;
    if (fio___http_body_read_all(h, &body))
      err = FIO_HTTP_PARAMS_PARSE_IO;
    else
      err = fio___http_params_fill_urlencoded(body,
                                              callbacks,
                                              udata,
                                              map,
                                              &consumed,
                                              &fields);
    break;
  }
  case FIO___HTTP_BODY_CONTENT_TYPE_MULTIPART:
    err = fio___http_params_fill_multipart(h,
                                           callbacks,
                                           udata,
                                           map,
                                           &consumed,
                                           &fields);
    break;
  case FIO___HTTP_BODY_CONTENT_TYPE_UNKNOWN: /* fall through */
  default: break; /* unknown / missing Content-Type: body is not params */
  }
finish:
  fio_http_body_seek(h, (ssize_t)position);
  return fio___http_params_finish(map, consumed, fields.count, err);
}

/* *****************************************************************************
Static file helper
***************************************************************************** */

/** Returns non-zero if the qvalue at `p` (pointing just past "q=") is
 * exactly zero in any zero-padded form (0, 0.0, 0.00, 0.000). */
FIO_SFUNC int fio___http_qvalue_is_zero(const char *p, const char *end) {
  if (p >= end || *p != '0')
    return 0;
  ++p;
  if (p < end && *p == '.') {
    ++p;
    while (p < end && *p >= '0' && *p <= '9') {
      if (*p != '0')
        return 0;
      ++p;
    }
  }
  return 1;
}

/**
 * Returns non-zero if `header` (a comma-separated header value) contains
 * `token` as a complete, standalone value.
 *
 * Handles formats like: "gzip, deflate, br" or "br;q=1.0, gzip;q=0.8"
 * Matching is case-insensitive. Quality parameters are honored per RFC 9110
 * §12.5.3: `q=0` (in any zero-padded form) FORBIDS the value (no match).
 * All current callers match against Accept-Encoding; other `;params` are
 * ignored rather than treated as q-values.
 */
FIO_SFUNC int fio___http_header_has_token(fio_str_info_s header,
                                          const char *token,
                                          size_t token_len) {
  if (!header.len || !header.buf || !token_len)
    return 0;
  const char *pos = header.buf;
  const char *end = header.buf + header.len;
  while (pos < end) {
    /* skip leading whitespace */
    while (pos < end && (*pos == ' ' || *pos == '\t'))
      ++pos;
    /* find end of this token (before comma or semicolon) */
    const char *tok_start = pos;
    while (pos < end && *pos != ',' && *pos != ';')
      ++pos;
    /* trim trailing whitespace from token */
    const char *tok_end = pos;
    while (tok_end > tok_start && (tok_end[-1] == ' ' || tok_end[-1] == '\t'))
      --tok_end;
    /* compare (case-insensitive) */
    if ((size_t)(tok_end - tok_start) == token_len) {
      size_t j = 0;
      for (; j < token_len; ++j) {
        if ((tok_start[j] | 32) != (token[j] | 32))
          break;
      }
      if (j == token_len) {
        /* token matched — honor `q=0` in its parameters (RFC 9110) */
        int forbidden = 0;
        const char *pp = pos; /* at ';' or ',' or end */
        while (pp < end && *pp != ',') {
          while (pp < end && (*pp == ';' || *pp == ' ' || *pp == '\t'))
            ++pp;
          if (pp + 1 < end && (pp[0] | 32) == 'q' && pp[1] == '=') {
            if (fio___http_qvalue_is_zero(pp + 2, end))
              forbidden = 1;
            break;
          }
          while (pp < end && *pp != ';' && *pp != ',')
            ++pp;
        }
        if (!forbidden)
          return 1;
      }
    }
    /* skip past comma (and any ;q=... parameters) */
    while (pos < end && *pos != ',')
      ++pos;
    if (pos < end)
      ++pos; /* skip the comma */
  }
  return 0;
}

/** Returns non-zero if the mime type is compressible (text-like).
 *
 * Handles: text types, application json/javascript/xml/wasm,
 *          +json and +xml suffixes, image svg+xml.
 *          Parameters (;charset=...) are ignored. Case-insensitive.
 */
FIO_SFUNC int fio___http_mime_is_compressible(fio_str_info_s mime) {
  if (!mime.len)
    return 0;
  /* text/ — all text types are compressible */
  if (mime.len >= 5 &&
      (fio_buf2u32u(mime.buf) | 0x20202020UL) ==
          (fio_buf2u32u("text") | 0x20202020UL) &&
      mime.buf[4] == '/')
    return 1;
  /* image/svg+xml (case-insensitive: 8 bytes "image/sv" + 5 bytes "g+xml") */
  if (mime.len >= 13 &&
      (fio_buf2u64u(mime.buf) | (uint64_t)0x2020202020202020ULL) ==
          fio_buf2u64u("image/sv") &&
      (fio_buf2u32u(mime.buf + 8) | 0x20202020UL) ==
          (fio_buf2u32u("g+xm") | 0x20202020UL) &&
      (mime.buf[12] | 32) == 'l' && (mime.len == 13 || mime.buf[13] == ';'))
    return 1;
  /* application/ — check exact subtype or +suffix */
  if (mime.len <= 12 || mime.buf[11] != '/')
    return 0;
  if ((fio_buf2u64u(mime.buf) | (uint64_t)0x2020202020202020ULL) !=
      fio_buf2u64u("applicat"))
    return 0;
  /* also verify "ion/" at bytes 8..11 */
  if ((fio_buf2u32u(mime.buf + 8) | 0x20202020UL) !=
      (fio_buf2u32u("ion/") | 0x20202020UL))
    return 0;
  /* extract subtype: from after '/' to ';' or end */
  const char *sub = mime.buf + 12;
  const char *end = mime.buf + mime.len;
  const char *sep = sub;
  while (sep < end && *sep != ';')
    ++sep;
  size_t sub_len = (size_t)(sep - sub);
  /* exact subtype matches (case-insensitive) */
  /* "json" / "wasm" — 4 bytes */
  if (sub_len == 4 && ((fio_buf2u32u(sub) | 0x20202020UL) ==
                           (fio_buf2u32u("json") | 0x20202020UL) ||
                       (fio_buf2u32u(sub) | 0x20202020UL) ==
                           (fio_buf2u32u("wasm") | 0x20202020UL)))
    return 1;
  /* "xml" — 3 bytes: u16 "xm" + byte 'l' */
  if (sub_len == 3 &&
      (fio_buf2u16u(sub) | 0x2020) == (fio_buf2u16u("xm") | 0x2020) &&
      (sub[2] | 32) == 'l')
    return 1;
  /* "javascript" — 10 bytes: u64 "javascri" + u16 "pt" */
  if (sub_len == 10 &&
      (fio_buf2u64u(sub) | (uint64_t)0x2020202020202020ULL) ==
          fio_buf2u64u("javascri") &&
      (fio_buf2u16u(sub + 8) | 0x2020) == (fio_buf2u16u("pt") | 0x2020))
    return 1;
  /* +json suffix (e.g. ld+json, vnd.api+json) */
  if (sub_len >= 5 && sub[sub_len - 5] == '+' &&
      (fio_buf2u32u(sub + sub_len - 4) | 0x20202020UL) ==
          (fio_buf2u32u("json") | 0x20202020UL))
    return 1;
  /* +xml suffix (e.g. xhtml+xml, atom+xml) */
  if (sub_len >= 4 && sub[sub_len - 4] == '+' &&
      (fio_buf2u16u(sub + sub_len - 3) | 0x2020) ==
          (fio_buf2u16u("xm") | 0x2020) &&
      (sub[sub_len - 1] | 32) == 'l')
    return 1;
  return 0;
}

/**
 * Static-file compression failure memoization seam.
 *
 * Updates `settings->compress_static` (a failure-memoization shift
 * register) after an on-demand compression attempt:
 *
 * - `result == 0` (success): `value |= 1` — re-seeds bit 0, restoring the
 *   8-failure runway.
 * - `result` is ENOSPC, EACCES, EROFS or EDQUOT (filesystem cannot accept
 *   new files): `value = 0` — immediate permanent disable (until the
 *   settings are re-applied by the user).
 * - any other non-zero `result` (an errno value describing the failure):
 *   `value <<= 1` — 1→2→…→128; the 8th consecutive failure shifts out of
 *   the `uint8_t` and the value becomes 0 (disabled).
 *
 * All updates are atomic — IO threads share the listener/route settings.
 * NULL `settings` is a no-op (detached handles carry no memoization
 * state).
 */
FIO_SFUNC void fio___http_static_compress_note_result(
    fio_http_settings_s *settings,
    int result) {
  if (!settings)
    return;
  uint8_t *p = &settings->compress_static;
  if (!result) {
    /* compression success — re-seed the failure runway */
    fio_atomic_or(p, (uint8_t)1);
    return;
  }
  if (result == ENOSPC || result == EACCES || result == EROFS
#ifdef EDQUOT
      || result == EDQUOT
#endif
  ) {
    /* the filesystem cannot accept new files — disable on-demand creation */
    fio_atomic_and(p, (uint8_t)0);
    return;
  }
  /* consecutive-failure shift register (CAS loop, keeps the 8-bit width) */
  uint8_t cur = *p;
  for (;;) {
    if (!cur)
      return; /* already disabled — stays disabled */
    uint8_t next = (uint8_t)(cur << 1);
    if (fio_atomic_compare_exchange_p(p, &cur, &next))
      return;
    /* on failure, `cur` was updated to the current value */
  }
}

/**
 * Attempts to send a static file from the `root` folder. On success the
 * response is complete and 0 is returned. Otherwise returns -1.
 *
 * Variant selection (skipped for ranged requests, which are always served
 * identity): the client's `Accept-Encoding` is tested in preference order
 * `br` -> `zstd` -> `gzip` -> `deflate` (files `file.br`, `file.zstd`,
 * `file.gz`, `file.zip`); the first accepted variant that exists on disk
 * and is at least as fresh as the original file is served. `zstd` and
 * `deflate` variants are only ever served pre-generated, never created.
 *
 * With `compress_static` enabled (`fio_http_settings_s`), missing or stale
 * `.br` / `.gz` variants are created on demand and written into the `root`
 * folder — limited to compressible (text-like) MIME types, 1024 byte to
 * FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT originals, and only when compression
 * shrinks the file. `.br` variants are compressed at brotli quality 4 (the
 * benchmark-reviewed fast-path ceiling — q5+ engages the slow hash-chain
 * path) and `.gz` variants at gzip level 6. The folder MUST be writable.
 * `Vary: accept-encoding` is set whenever variants may exist (including on
 * identity responses).
 *
 * `compress_static` is a failure-memoization shift register: any non-zero
 * value enables on-demand creation. A compression success re-seeds bit 0
 * (`value |= 1`); any other failure shifts the value left (`value <<= 1`),
 * so 8 consecutive failures shift out of the `uint8_t` and disable
 * creation, as does a single filesystem-full / permission error (ENOSPC,
 * EACCES, EROFS, EDQUOT — `value = 0`). Once 0, creation stays disabled
 * until the settings are re-applied. Routes inherit the value from the
 * listener settings at route-creation. Detached handles (no settings)
 * gate on the `FIO___HTTP_CFLAG_COMPRESS_STATIC` cflag instead, with no
 * memoization state.
 */
SFUNC int fio_http_static_file_response(fio_http_s *h,
                                        fio_str_info_s rt,
                                        fio_str_info_s fnm,
                                        size_t max_age) {
  int fd = -1;
  size_t file_length = 0;
  /* combine public folder with path to get file name.
   * NOTE: `rt` MUST name an existing folder (validated for settings-based
   * callers; direct callers pass "." for the CWD) - the traversal guard
   * rejects `..` folding, not absolute paths. */
  fio_str_info_s mime_type = {0};
  FIO_STR_INFO_TMP_VAR(etag, 31);
  FIO_STR_INFO_TMP_VAR(filename, (FIO_FILENAME_PATH_CAPA - 1));
  { /* test for HEAD and OPTIONS requests */
    fio_str_info_s m = fio_keystr_info(&h->method);
    if ((m.len == 7 && (fio_buf2u64u(m.buf) | 0x2020202020202020ULL) ==
                           (fio_buf2u64u("options") | 0x2020202020202020ULL)))
      goto file_not_found;
  }
  rt.len -= ((rt.len > 0) && (fnm.len > 0 && fnm.buf[0] == '/') &&
             (rt.buf[rt.len - 1] == '/' ||
              rt.buf[rt.len - 1] == FIO_FOLDER_SEPARATOR));
  if (rt.len + fnm.len > (FIO_FILENAME_PATH_CAPA - 16))
    goto file_not_found;
  fio_string_write(&filename, NULL, rt.buf, rt.len);
  fio_string_write_url_dec(&filename, NULL, fnm.buf, fnm.len);
  if (fio_filename_is_unsafe_url(filename.buf))
    goto file_not_found;

  { /* Test for incomplete file name */
    size_t file_type = fio_filename_type(filename.buf);
#if defined(S_IFDIR) && defined(FIO_HTTP_DEFAULT_INDEX_FILENAME)
    if (file_type == S_IFDIR) {
      filename.len -= (filename.buf[filename.len - 1] == '/' ||
                       filename.buf[filename.len - 1] == FIO_FOLDER_SEPARATOR);
#if FIO_HTTP_STATIC_FILE_COMPLETION
      fio_string_write(&filename,
                       NULL,
                       "/" FIO_HTTP_DEFAULT_INDEX_FILENAME,
                       sizeof(FIO_HTTP_DEFAULT_INDEX_FILENAME));
      file_type = 0;
#else
      fio_string_write(
          &filename,
          NULL, /* note that sizeof will count NUL, so we skip 1 char: */
          "/" FIO_HTTP_DEFAULT_INDEX_FILENAME ".html",
          sizeof(FIO_HTTP_DEFAULT_INDEX_FILENAME ".html"));
      file_type = fio_filename_type(filename.buf);
#endif /* FIO_HTTP_STATIC_FILE_COMPLETION */
    }
#endif /* S_IFDIR */
#if FIO_HTTP_STATIC_FILE_COMPLETION
    const fio_buf_info_s extensions[] = {FIO_BUF_INFO1((char *)".html"),
                                         FIO_BUF_INFO1((char *)".htm"),
                                         FIO_BUF_INFO1((char *)".txt"),
                                         FIO_BUF_INFO1((char *)".md"),
                                         FIO_BUF_INFO0};
    const fio_buf_info_s *pext = extensions;
    while (!file_type) {
      fio_string_write(&filename, NULL, pext->buf, pext->len);
      file_type = fio_filename_type(filename.buf);
      if (file_type)
        break;
      filename.len -= pext->len;
      ++pext;
      if (!pext->buf)
        goto file_not_found;
    }
    switch (file_type) {
    case S_IFREG: break;
#ifdef S_IFLNK
    case S_IFLNK: break;
#endif
    default: goto file_not_found;
    }
#else  /* FIO_HTTP_STATIC_FILE_COMPLETION */
    if (!file_type)
      goto file_not_found;
#endif /* FIO_HTTP_STATIC_FILE_COMPLETION */
  }
  {
    /* find mime type if registered */
    char *end = filename.buf + filename.len;
    char *ext = end;
    do {
      --ext;
    } while (ext[0] != '.' && ext[0] != '/');
    if ((ext++)[0] == '.') {
      mime_type = fio_http_mimetype(ext, end - ext);
      if (!mime_type.len)
        FIO_LOG_WARNING("missing mime-type for extension %s (not registered).",
                        ext);
    }
  }
  {
    fio_str_info_s ac =
        fio_http_request_header(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                0);
    /* Ranges over encoded bytes are broken in practice: ranged responses
     * are always served identity (no compressed variant selection). */
    if (fio_http_request_header(h, FIO_STR_INFO2((char *)"range", 5), 0).len)
      goto accept_encoding_header_test_done;
    /* stat the original file for staleness comparison and creation */
    struct stat orig_st;
    int have_orig_st = !fio_filename_stat(filename.buf, &orig_st);
    size_t orig_len = filename.len; /* remember unextended length */
    /* Attached handles gate on the route/listener settings (a failure-
     * memoization shift register); detached handles (no settings) gate on
     * the handle cflag exactly as before. */
    fio_http_settings_s *st = fio_http_settings(h);
    int settings_may_compress = st && st->compress_static;
    int can_create =
        have_orig_st &&
        (st ? settings_may_compress
            : fio___http_cflags_is_set(h, FIO___HTTP_CFLAG_COMPRESS_STATIC)) &&
        fio___http_mime_is_compressible(mime_type) &&
        (size_t)orig_st.st_size >= 1024 &&
        (size_t)orig_st.st_size <= FIO_HTTP_STATIC_FILE_COMPRESS_LIMIT;
    if (can_create)
      /* Compressed variants exist (or may be created) for this resource:
       * even the UNCOMPRESSED response must carry Vary so caches key on
       * Accept-Encoding. */
      fio___http_response_vary_accept_encoding(h);
    if (!ac.len)
      goto accept_encoding_header_test_done;
    struct {
      fio_buf_info_s value;
      fio_buf_info_s ext;
      size_t (*compress)(void *, size_t, const void *, size_t, int);
      size_t (*bound)(size_t);
      size_t extra; /* added to bound (e.g. gzip header+trailer) */
      int level;    /* compression level */
    } options[] = {{
                       .value = FIO_BUF_INFO2((char *)"br", 2),
                       .ext = FIO_BUF_INFO2((char *)".br", 3),
                       .compress = fio_brotli_compress,
                       .bound = fio_brotli_compress_bound,
                       /* q4 = highest fast-path quality (q5+ engages the
                        * slow hash-chain path — an attacker-triggerable DoS
                        * surface). See ai-task/http-compress-level.md */
                       .level = 4,
                   },
                   {
                       .value = FIO_BUF_INFO2((char *)"zstd", 4),
                       .ext = FIO_BUF_INFO2((char *)".zstd", 5),
                   },
                   {
                       .value = FIO_BUF_INFO2((char *)"gzip", 4),
                       .ext = FIO_BUF_INFO2((char *)".gz", 3),
                       .compress = fio_gzip_compress,
                       .bound = fio_deflate_compress_bound,
                       .extra = 18,
                       .level = 6,
                   },
                   {
                       .value = FIO_BUF_INFO2((char *)"deflate", 7),
                       .ext = FIO_BUF_INFO2((char *)".zip", 4),
                   },
                   {.level = 0}};
    for (size_t i = 0; options[i].value.buf; ++i) {
      if (!fio___http_header_has_token(ac,
                                       options[i].value.buf,
                                       options[i].value.len))
        continue;
      fio_string_write(&filename, NULL, options[i].ext.buf, options[i].ext.len);
      /* check if compressed variant exists on disk and is fresh */
      struct stat comp_st;
      int variant_exists = !fio_filename_stat(filename.buf, &comp_st);
      if (variant_exists && have_orig_st && comp_st.st_mtime < orig_st.st_mtime)
        variant_exists = 0; /* stale — original was modified */
      if (!variant_exists) {
        /* no usable variant on disk — try to create one */
        filename.len = orig_len;
        filename.buf[filename.len] = 0;
        if (!can_create || !options[i].compress)
          continue;
        /* read original file */
        int orig_fd = fio_filename_open(filename.buf, O_RDONLY);
        if (orig_fd == -1)
          continue;
        void *src = FIO_MEM_REALLOC(NULL, 0, (size_t)orig_st.st_size, 0);
        if (!src) {
          close(orig_fd);
          continue;
        }
        FIO_LEAK_COUNTER_ON_ALLOC(fio___http_static_compress_buf);
        size_t rd = fio_fd_read(orig_fd, src, (size_t)orig_st.st_size, 0);
        close(orig_fd);
        if (rd != (size_t)orig_st.st_size) {
          FIO_LEAK_COUNTER_ON_FREE(fio___http_static_compress_buf);
          FIO_MEM_FREE(src, (size_t)orig_st.st_size);
          continue;
        }
        /* compress */
        size_t bound = options[i].bound(rd) + options[i].extra;
        void *dst = FIO_MEM_REALLOC(NULL, 0, bound, 0);
        if (!dst) {
          FIO_LEAK_COUNTER_ON_FREE(fio___http_static_compress_buf);
          FIO_MEM_FREE(src, (size_t)orig_st.st_size);
          continue;
        }
        FIO_LEAK_COUNTER_ON_ALLOC(fio___http_static_compress_buf);
        size_t comp_len =
            options[i].compress(dst, bound, src, rd, options[i].level);
        FIO_LEAK_COUNTER_ON_FREE(fio___http_static_compress_buf);
        FIO_MEM_FREE(src, (size_t)orig_st.st_size);
        if (!comp_len || comp_len >= rd) {
          /* compression failed or didn't shrink the file */
          fio___http_static_compress_note_result(st, EINVAL);
          FIO_LEAK_COUNTER_ON_FREE(fio___http_static_compress_buf);
          FIO_MEM_FREE(dst, bound);
          continue;
        }
        /* write compressed variant to disk */
        fio_string_write(&filename,
                         NULL,
                         options[i].ext.buf,
                         options[i].ext.len);
        if (fio_filename_overwrite(filename.buf, dst, comp_len)) {
          /* write failure — errno preserved by fio_filename_overwrite */
          fio___http_static_compress_note_result(st, errno);
          FIO_LEAK_COUNTER_ON_FREE(fio___http_static_compress_buf);
          FIO_MEM_FREE(dst, bound);
          filename.len = orig_len;
          filename.buf[filename.len] = 0;
          continue;
        }
        /* variant written — compression success */
        fio___http_static_compress_note_result(st, 0);
        FIO_LEAK_COUNTER_ON_FREE(fio___http_static_compress_buf);
        FIO_MEM_FREE(dst, bound);
      }
      /* compressed variant is ready — set response headers */
      fio___http_response_vary_accept_encoding(h);
      fio_http_response_header_set(
          h,
          FIO_STR_INFO2((char *)"content-encoding", 16),
          FIO_BUF2STR_INFO(options[i].value));
      break;
    }
  }

accept_encoding_header_test_done:
  /* attempt to open file */
  fd = fio_filename_open(filename.buf, O_RDONLY);
  if (fd == -1)
    goto file_not_found;

  { /* test / validate etag */
    struct stat stt;
    if (fstat(fd, &stt))
      goto file_not_found;
    uint64_t etag_hash = fio_risky_hash(&stt, sizeof(stt), 0);
    fio_string_write_hex(&etag, NULL, etag_hash);
    fio_http_response_header_set(h, FIO_STR_INFO2((char *)"etag", 4), etag);
    filename.len = 0;
    filename.len = fio_time2rfc7231(filename.buf, stt.st_mtime);
    fio_http_response_header_set(h,
                                 FIO_STR_INFO1((char *)"last-modified"),
                                 filename);
    if (max_age) {
      filename.len = 0;
      fio_string_write2(&filename,
                        NULL,
                        FIO_STRING_WRITE_STR2("max-age=", 8),
                        FIO_STRING_WRITE_UNUM(max_age));
      fio_http_response_header_set(h,
                                   FIO_STR_INFO1((char *)"cache-control"),
                                   filename);
    }
    file_length = stt.st_size;
    filename.capa = 0;
    if (fio___http_response_etag_if_none_match(h)) {
      close(fd); /* 304 sent - the descriptor is never handed to the writer */
      return 0;
    }
  }
  /* test for range requests. */
  {
    /* test / validate range requests */
    fio_str_info_s rng =
        fio_http_request_header(h, FIO_STR_INFO2((char *)"range", 5), 0);
    if (!rng.len)
      goto range_request_review_finished;
    {
      fio_str_info_s ifrng =
          fio_http_request_header(h, FIO_STR_INFO2((char *)"if-range", 8), 0);
      if (ifrng.len && !FIO_STR_INFO_IS_EQ(ifrng, etag))
        goto range_request_review_finished;
    }
    if (rng.len < 7 || fio_buf2u32u(rng.buf) != fio_buf2u32u("byte") ||
        fio_buf2u16u(rng.buf + 4) != fio_buf2u16u("s="))
      goto range_request_review_finished;
    char *ipos = rng.buf + 6;
    size_t start_range = fio_atol10u(&ipos);
    if (ipos == rng.buf + 6)
      start_range = (size_t)-1;
    if (*ipos != '-')
      goto range_request_review_finished;
    ++ipos;
    size_t end_range = fio_atol10u(&ipos);
    if (end_range > file_length)
      goto range_request_review_finished;
    if (!end_range)
      end_range = file_length - 1;
    if (start_range == (size_t)-1) {
      start_range = file_length - end_range;
      end_range = file_length - 1;
    }
    if (start_range > end_range || end_range > file_length)
      goto invalid_range;
    if (!start_range && end_range + 1 == file_length)
      goto range_request_review_finished;
    /* update response headers and info */
    h->status = 206;
    filename.len = 0;
    filename.capa = 1024;
    fio_string_write2(&filename,
                      NULL,
                      FIO_STRING_WRITE_STR2("bytes ", 6),
                      FIO_STRING_WRITE_UNUM(start_range),
                      FIO_STRING_WRITE_STR2("-", 1),
                      FIO_STRING_WRITE_UNUM((end_range)),
                      FIO_STRING_WRITE_STR2("/", 1),
                      FIO_STRING_WRITE_UNUM(file_length));
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"content-range", 13),
                                 filename);
    file_length = (end_range - start_range) + 1;
    filename.capa = start_range;
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"etag", 4),
                                 FIO_STR_INFO2(NULL, 0));
  }

range_request_review_finished:
  /* allow interrupted downloads to resume */
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"accept-ranges", 13),
                               FIO_STR_INFO2((char *)"bytes", 5));
  /* test for HEAD requests */
  {
    fio_str_info_s m = fio_keystr_info(&h->method);
    if ((m.len == 4 && (fio_buf2u32u(m.buf) | 0x20202020UL) ==
                           (fio_buf2u32u("head") | 0x20202020UL)))
      goto head_request;
  }

  /* finish up (set mime type and send file) */
  if (mime_type.len)
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"content-type", 12),
                                 mime_type);
  { /* send response (avoid macro for C++ compatibility) */
    fio_http_write_args_s args = {
        .len = file_length,
        .offset = filename.capa, /* now holds starting offset */
        .fd = fd,
        .finish = 1};
    fio_http_write FIO_NOOP(h, args);
  }
  return 0;

file_not_found:
  if (fd != -1)
    close(fd);
  return -1;

head_request:
  if (fd != -1)
    close(fd);
  /* RFC 9110 §9.3.2: the server SHOULD send the same header fields in
   * response to a HEAD request as it would have sent to GET — only the
   * content is omitted. CDN origin revalidation depends on Content-Length
   * / Content-Type / ETag matching the cached GET metadata. Note
   * `file_length` and `mime_type` already reflect any selected compressed
   * variant or 206 range, exactly like GET. */
  if (mime_type.len)
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"content-type", 12),
                                 mime_type);
  {
    char ibuf[32];
    fio_str_info_s v = FIO_STR_INFO3(ibuf, 0, 32);
    v.len = fio_digits10u(file_length);
    fio_ltoa10u(v.buf, file_length, v.len);
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"content-length", 14),
                                 v);
  }
  {
    fio_http_write_args_s args = {.finish = 1};
    fio_http_write FIO_NOOP(h, args);
  }
  return 0;

invalid_range:
  close(fd); /* 416 - the descriptor is never handed to the writer */
  filename.len = 0;
  filename.capa = 1024;
  fio_string_write2(&filename,
                    NULL,
                    FIO_STRING_WRITE_STR2("bytes */", 8),
                    FIO_STRING_WRITE_UNUM(file_length));
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"content-range", 13),
                               filename);
  return fio_http_send_error_response(h, 416);
}

/* *****************************************************************************
Status Strings
***************************************************************************** */

/** Returns a human readable string related to the HTTP status number. */
SFUNC fio_str_info_s fio_http_status2str(size_t status) {
  fio_str_info_s r = {0};
#define HTTP_RETURN_STATUS(str)                                                \
  do {                                                                         \
    r.len = FIO_STRLEN(str);                                                   \
    r.buf = (char *)str;                                                       \
    return r;                                                                  \
  } while (0);
  switch (status) {
  // clang-format off
  case 100: HTTP_RETURN_STATUS("Continue");
  case 101: HTTP_RETURN_STATUS("Switching Protocols");
  case 102: HTTP_RETURN_STATUS("Processing");
  case 103: HTTP_RETURN_STATUS("Early Hints");
  case 110: HTTP_RETURN_STATUS("Response is Stale"); /* caching code*/
  case 111: HTTP_RETURN_STATUS("Re-validation Failed"); /* caching code*/
  case 112: HTTP_RETURN_STATUS("Disconnected Operation"); /* caching code*/
  case 113: HTTP_RETURN_STATUS("Heuristic Expiration"); /* caching code*/
  case 199: HTTP_RETURN_STATUS("Miscellaneous Warning"); /* caching code*/
  case 200: HTTP_RETURN_STATUS("OK");
  case 201: HTTP_RETURN_STATUS("Created");
  case 202: HTTP_RETURN_STATUS("Accepted");
  case 203: HTTP_RETURN_STATUS("Non-Authoritative Information");
  case 204: HTTP_RETURN_STATUS("No Content");
  case 205: HTTP_RETURN_STATUS("Reset Content");
  case 206: HTTP_RETURN_STATUS("Partial Content");
  case 207: HTTP_RETURN_STATUS("Multi-Status");
  case 208: HTTP_RETURN_STATUS("Already Reported");
  case 214: HTTP_RETURN_STATUS("Transformation Applied"); /* caching code*/
  case 218: HTTP_RETURN_STATUS("This is fine (Apache Web Server)"); /* unofficial */
  case 226: HTTP_RETURN_STATUS("IM Used");
  case 299: HTTP_RETURN_STATUS("Miscellaneous Persistent Warning"); /* caching code*/
  case 300: HTTP_RETURN_STATUS("Multiple Choices");
  case 301: HTTP_RETURN_STATUS("Moved Permanently");
  case 302: HTTP_RETURN_STATUS("Found");
  case 303: HTTP_RETURN_STATUS("See Other");
  case 304: HTTP_RETURN_STATUS("Not Modified");
  case 305: HTTP_RETURN_STATUS("Use Proxy");
  case 307: HTTP_RETURN_STATUS("Temporary Redirect");
  case 308: HTTP_RETURN_STATUS("Permanent Redirect");
  case 400: HTTP_RETURN_STATUS("Bad Request");
  case 401: HTTP_RETURN_STATUS("Unauthorized");
  case 402: HTTP_RETURN_STATUS("Payment Required");
  case 403: HTTP_RETURN_STATUS("Forbidden");
  case 404: HTTP_RETURN_STATUS("Not Found");
  case 405: HTTP_RETURN_STATUS("Method Not Allowed");
  case 406: HTTP_RETURN_STATUS("Not Acceptable");
  case 407: HTTP_RETURN_STATUS("Proxy Authentication Required");
  case 408: HTTP_RETURN_STATUS("Request Timeout");
  case 409: HTTP_RETURN_STATUS("Conflict");
  case 410: HTTP_RETURN_STATUS("Gone");
  case 411: HTTP_RETURN_STATUS("Length Required");
  case 412: HTTP_RETURN_STATUS("Precondition Failed");
  case 413: HTTP_RETURN_STATUS("Content Too Large");
  case 414: HTTP_RETURN_STATUS("URI Too Long");
  case 415: HTTP_RETURN_STATUS("Unsupported Media Type");
  case 416: HTTP_RETURN_STATUS("Range Not Satisfiable");
  case 417: HTTP_RETURN_STATUS("Expectation Failed");
  case 418: HTTP_RETURN_STATUS("I am a Teapot"); /* April Fool's Day, 1998 */
  case 419: HTTP_RETURN_STATUS("Page Expired (Laravel Framework)"); /* unofficial */
  case 420: HTTP_RETURN_STATUS("Enhance Your Calm (Twitter) - Method Failure (Spring Framework)"); /* unofficial */
  case 421: HTTP_RETURN_STATUS("Misdirected Request");
  case 422: HTTP_RETURN_STATUS("Unprocessable Content");
  case 423: HTTP_RETURN_STATUS("Locked");
  case 424: HTTP_RETURN_STATUS("Failed Dependency");
  case 425: HTTP_RETURN_STATUS("Too Early");
  case 426: HTTP_RETURN_STATUS("Upgrade Required");
  case 427: HTTP_RETURN_STATUS("Unassigned");
  case 428: HTTP_RETURN_STATUS("Precondition Required");
  case 429: HTTP_RETURN_STATUS("Too Many Requests");
  case 430: HTTP_RETURN_STATUS("Request Header Fields Too Large (Shopify)"); /* unofficial */
  case 431: HTTP_RETURN_STATUS("Request Header Fields Too Large");
  case 444: HTTP_RETURN_STATUS("No Response"); /* nginx code */
  case 450: HTTP_RETURN_STATUS("Blocked by Windows Parental Controls (Microsoft)"); /* unofficial */
  case 451: HTTP_RETURN_STATUS("Unavailable For Legal Reasons");
  case 494: HTTP_RETURN_STATUS("Request header too large"); /* nginx code */
  case 495: HTTP_RETURN_STATUS("SSL Certificate Error"); /* nginx code */
  case 496: HTTP_RETURN_STATUS("SSL Certificate Required"); /* nginx code */
  case 497: HTTP_RETURN_STATUS("HTTP Request Sent to HTTPS Port"); /* nginx code */
  case 498: HTTP_RETURN_STATUS("Invalid Token (Esri)"); /* unofficial */
  case 499: HTTP_RETURN_STATUS("Client Closed Request"); /* nginx code */
  case 500: HTTP_RETURN_STATUS("Internal Server Error");
  case 501: HTTP_RETURN_STATUS("Not Implemented");
  case 502: HTTP_RETURN_STATUS("Bad Gateway");
  case 503: HTTP_RETURN_STATUS("Service Unavailable");
  case 504: HTTP_RETURN_STATUS("Gateway Timeout");
  case 505: HTTP_RETURN_STATUS("HTTP Version Not Supported");
  case 506: HTTP_RETURN_STATUS("Variant Also Negotiates");
  case 507: HTTP_RETURN_STATUS("Insufficient Storage");
  case 508: HTTP_RETURN_STATUS("Loop Detected");
  case 509: HTTP_RETURN_STATUS("Bandwidth Limit Exceeded (Apache Web Server/cPanel)"); /* unofficial */
  case 510: HTTP_RETURN_STATUS("Not Extended");
  case 511: HTTP_RETURN_STATUS("Network Authentication Required");
  case 529: HTTP_RETURN_STATUS("Site is overloaded (Qualys)"); /* unofficial */
  case 530: HTTP_RETURN_STATUS("Site is frozen (Pantheon web)"); /* unofficial */
  case 598: HTTP_RETURN_STATUS("Network read timeout error"); /* unofficial */
    // clang-format on
  }
  HTTP_RETURN_STATUS("Unknown");
#undef HTTP_RETURN_STATUS
}

/* *****************************************************************************
MIME File Type Helpers
***************************************************************************** */

typedef struct {
  uint64_t ext;
  uint16_t len;
  char mime[118]; /* all together 128 bytes per node */
} fio___http_mime_info_s;

#define FIO___HTTP_MIME_IS_VALID(o) ((o)->ext != 0)
#define FIO___HTTP_MIME_CMP(a, b)   ((a)->ext == (b)->ext)
#define FIO___HTTP_MIME_HASH(o)     fio_risky_num(((o)->ext), 0)

#undef FIO_TYPEDEF_IMAP_REALLOC
#define FIO_TYPEDEF_IMAP_REALLOC(ptr, old_size, new_size, copy_len)            \
  realloc(ptr, new_size)
#undef FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE 0
#undef FIO_TYPEDEF_IMAP_FREE
#define FIO_TYPEDEF_IMAP_FREE(ptr, len) free(ptr)

FIO_TYPEDEF_IMAP_ARRAY(fio___http_mime_map,
                       fio___http_mime_info_s,
                       uint32_t,
                       FIO___HTTP_MIME_HASH,
                       FIO___HTTP_MIME_CMP,
                       FIO___HTTP_MIME_IS_VALID)

static fio___http_mime_map_s FIO___HTTP_MIMETYPES;
#undef FIO___HTTP_MIME_IS_VALID
#undef FIO___HTTP_MIME_CMP
#undef FIO___HTTP_MIME_HASH

#undef FIO_TYPEDEF_IMAP_REALLOC
#define FIO_TYPEDEF_IMAP_REALLOC FIO_MEM_REALLOC
#undef FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE FIO_MEM_REALLOC_IS_SAFE
#undef FIO_TYPEDEF_IMAP_FREE
#define FIO_TYPEDEF_IMAP_FREE FIO_MEM_FREE

/** Registers a Mime-Type to be associated with the file extension. */
SFUNC int fio_http_mimetype_register(char *file_ext,
                                     size_t file_ext_len,
                                     fio_str_info_s mime_type) {
  fio___http_mime_info_s tmp = {0}, *old;
  if (file_ext_len > 7 || mime_type.len > 117)
    return -1;
  tmp.ext = 0;
  FIO_MEMCPY(&tmp.ext, file_ext, file_ext_len);
  if (!mime_type.len)
    goto remove_mime;
  FIO_MEMCPY(&tmp.mime, mime_type.buf, mime_type.len);
  tmp.len = mime_type.len;
  /* tmp.mime[mime_type.len] = 0; // not required, the whole buffer is zero */
  old = fio___http_mime_map_get(&FIO___HTTP_MIMETYPES, tmp);
  if (old && old->len == tmp.len && !FIO_MEMCMP(old->mime, tmp.mime, tmp.len)) {
    FIO_LOG_WARNING("mime-type collision: %.*s was %s, now %s",
                    (int)file_ext_len,
                    file_ext,
                    old->mime,
                    tmp.mime);
  }
  fio___http_mime_map_set(&FIO___HTTP_MIMETYPES, tmp, 1);
  return 0;

remove_mime:
  return fio___http_mime_map_remove(&FIO___HTTP_MIMETYPES, tmp);
}

/** Finds the Mime-Type associated with the file extension (if registered). */
SFUNC fio_str_info_s fio_http_mimetype(char *file_ext, size_t file_ext_len) {
  fio_str_info_s r = {0};
  if (file_ext_len > 7)
    return r;
  fio___http_mime_info_s tmp, *val;
  tmp.ext = 0;
  FIO_MEMCPY(&tmp.ext, file_ext, file_ext_len);
  val = fio___http_mime_map_get(&FIO___HTTP_MIMETYPES, tmp);
  if (!val)
    return r;
  r.len = val->len;
  r.buf = val->mime;
  return r;
}

#define REGISTER_MIME(ext, type)                                               \
  fio_http_mimetype_register((char *)ext,                                      \
                             sizeof(ext) - 1,                                  \
                             FIO_STR_INFO2((char *)type, sizeof(type) - 1))

/** Registers known mime-types that aren't often used by Web Servers. */
FIO_SFUNC void fio_http_mime_register_essential(void) {
  /* clang-format off */
  REGISTER_MIME("3ds", "image/x-3ds");
  REGISTER_MIME("3g2", "video/3gpp");
  REGISTER_MIME("3gp", "video/3gpp");
  REGISTER_MIME("7z", "application/x-7z-compressed");
  REGISTER_MIME("aac", "audio/aac");
  REGISTER_MIME("abw", "application/x-abiword");
  REGISTER_MIME("aif", "audio/x-aiff");
  REGISTER_MIME("aifc", "audio/x-aiff");
  REGISTER_MIME("aiff", "audio/x-aiff");
  REGISTER_MIME("arc", "application/x-freearc");
  REGISTER_MIME("atom", "application/atom+xml");
  REGISTER_MIME("avi", "video/x-msvideo");
  REGISTER_MIME("avif", "image/avif");
  REGISTER_MIME("azw", "application/vnd.amazon.ebook");
  REGISTER_MIME("bin", "application/octet-stream");
  REGISTER_MIME("bmp", "image/bmp");
  REGISTER_MIME("bz", "application/x-bzip");
  REGISTER_MIME("bz2", "application/x-bzip2");
  REGISTER_MIME("cda", "application/x-cdf");
  REGISTER_MIME("csh", "application/x-csh");
  REGISTER_MIME("css", "text/css");
  REGISTER_MIME("csv", "text/csv");
  REGISTER_MIME("dmg", "application/x-apple-diskimage");
  REGISTER_MIME("doc", "application/msword");
  REGISTER_MIME("docx", "application/" "vnd.openxmlformats-officedocument.wordprocessingml.document");
  REGISTER_MIME("eot", "application/vnd.ms-fontobject");
  REGISTER_MIME("epub", "application/epub+zip");
  REGISTER_MIME("gif", "image/gif");
  REGISTER_MIME("gz", "application/gzip");
  REGISTER_MIME("htm", "text/html");
  REGISTER_MIME("html", "text/html");
  REGISTER_MIME("ico", "image/vnd.microsoft.icon");
  REGISTER_MIME("ics", "text/calendar");
  REGISTER_MIME("iso", "application/x-iso9660-image");
  REGISTER_MIME("jar", "application/java-archive");
  REGISTER_MIME("jpe", "image/jpeg");
  REGISTER_MIME("jpeg", "image/jpeg");
  REGISTER_MIME("jpg", "image/jpeg");
  REGISTER_MIME("jpgm", "video/jpm");
  REGISTER_MIME("jpgv", "video/jpeg");
  REGISTER_MIME("jpm", "video/jpm");
  REGISTER_MIME("js", "application/javascript");
  REGISTER_MIME("json", "application/json");
  REGISTER_MIME("jsonld", "application/ld+json");
  REGISTER_MIME("jsonml", "application/jsonml+json");
  REGISTER_MIME("md", "text/markdown");  
  REGISTER_MIME("mid", "audio/midi");
  REGISTER_MIME("midi", "audio/midi");
  REGISTER_MIME("mjs", "text/javascript");
  REGISTER_MIME("mp3", "audio/mpeg");
  REGISTER_MIME("mp4", "video/mp4");
  REGISTER_MIME("m4a", "audio/mp4");
  REGISTER_MIME("m4v", "video/mp4");
  REGISTER_MIME("mpeg", "video/mpeg");
  REGISTER_MIME("mpkg", "application/vnd.apple.installer+xml");
  REGISTER_MIME("odp", "application/vnd.oasis.opendocument.presentation");
  REGISTER_MIME("ods", "application/vnd.oasis.opendocument.spreadsheet");
  REGISTER_MIME("odt", "application/vnd.oasis.opendocument.text");
  REGISTER_MIME("oga", "audio/ogg");
  REGISTER_MIME("ogv", "video/ogg");
  REGISTER_MIME("ogx", "application/ogg");
  REGISTER_MIME("opus", "audio/opus");
  REGISTER_MIME("otf", "font/otf");
  REGISTER_MIME("pdf", "application/pdf");
  REGISTER_MIME("php", "application/x-httpd-php");
  REGISTER_MIME("png", "image/png");
  REGISTER_MIME("ppt", "application/vnd.ms-powerpoint");
  REGISTER_MIME("pptx","application/""vnd.openxmlformats-officedocument.presentationml.presentation");
  REGISTER_MIME("rar", "application/vnd.rar");
  REGISTER_MIME("rtf", "application/rtf");
  REGISTER_MIME("sh", "application/x-sh");
  REGISTER_MIME("svg", "image/svg+xml");
  REGISTER_MIME("svgz", "image/svg+xml");
  REGISTER_MIME("tar", "application/x-tar");
  REGISTER_MIME("tif", "image/tiff");
  REGISTER_MIME("tiff", "image/tiff");
  REGISTER_MIME("ts", "video/mp2t");
  REGISTER_MIME("ttf", "font/ttf");
  REGISTER_MIME("txt", "text/plain");
  REGISTER_MIME("vsd", "application/vnd.visio");
  REGISTER_MIME("wav", "audio/wav");
  REGISTER_MIME("weba", "audio/webm");
  REGISTER_MIME("webm", "video/webm");
  REGISTER_MIME("webp", "image/webp");
  REGISTER_MIME("woff", "font/woff");
  REGISTER_MIME("woff2", "font/woff2");
  REGISTER_MIME("xhtml", "application/xhtml+xml");
  REGISTER_MIME("xls", "application/vnd.ms-excel");
  REGISTER_MIME("xlsx","application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
  REGISTER_MIME("xml", "application/xml");
  REGISTER_MIME("xul", "application/vnd.mozilla.xul+xml");
  REGISTER_MIME("zip", "application/zip");
  /* clang-format on */
}

#undef REGISTER_MIME

/* *****************************************************************************
Constructor / Destructor
***************************************************************************** */

FIO_SFUNC void fio___http_cleanup(void *ignr_) {
  (void)ignr_;
#if FIO_HTTP_CACHE_LIMIT
  for (size_t i = 0; i < 2; ++i) {
    const char *names[] = {"cookie names", "header values"};
    FIO_LOG_DEBUG2(
        "freeing %zu strings from %s cache (capacity was: %zu)",
        (size_t)fio___http_str_cache_count(&FIO___HTTP_STRING_CACHE[i].cache),
        names[i],
        (size_t)fio___http_str_cache_capa(&FIO___HTTP_STRING_CACHE[i].cache));
#ifdef FIO_LOG_LEVEL_DEBUG
    if (FIO_LOG_LEVEL_DEBUG == FIO_LOG_LEVEL) {
      FIO_MAP_EACH(fio___http_str_cache,
                   (&FIO___HTTP_STRING_CACHE[i].cache),
                   pos) {
        fprintf(stderr, "\t \"%s\" (%zu bytes)\n", pos.key.buf, pos.key.len);
      }
    }
#endif
    fio___http_str_cache_destroy(&FIO___HTTP_STRING_CACHE[i].cache);
    FIO___LOCK_DESTROY(FIO___HTTP_STRING_CACHE[i].lock);
    (void)names; /* if unused */
  }
#endif /* FIO_HTTP_CACHE_LIMIT */
  FIO_LOG_DEBUG2("(%d) HTTP MIME hash storage count/capa: %zu / %zu",
                 fio_getpid(),
                 (size_t)FIO___HTTP_MIMETYPES.count,
                 fio___http_mime_map_capa(&FIO___HTTP_MIMETYPES));
  fio___http_mime_map_destroy(&FIO___HTTP_MIMETYPES);
}

FIO_CONSTRUCTOR(fio___http_str_cache_static_builder) {
  fio___http_str_cached_init();
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___http_cleanup, NULL);
  fio_http_mime_register_essential();
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#undef FIO___HTTP_TIME_DIV
#undef FIO___HTTP_TIME_UNIT

/* *****************************************************************************
HTTP Types Finish
***************************************************************************** */
#endif /* FIO_HTTP */
