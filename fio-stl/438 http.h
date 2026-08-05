/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************

          HTTP Glue - Listen / Connect, Protocol Wiring, Shared Helpers

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP) && !defined(FIO___RECURSIVE_INCLUDE) &&                  \
    !defined(H___FIO_HTTP_GLUE___H) &&                                         \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
#define H___FIO_HTTP_GLUE___H

/* *****************************************************************************
ALPN Helpers
***************************************************************************** */

FIO_SFUNC void fio___http_on_select_h1(fio_io_s *io) {
  FIO_LOG_DDEBUG2("TLS ALPN HTTP/1.1 selected for %p", io);
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  fio_io_protocol_set(
      io,
      &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
            ->state[FIO___HTTP_PROTOCOL_HTTP1]
            .protocol));
}
FIO_SFUNC void fio___http_on_select_h2(fio_io_s *io) {
  FIO_LOG_ERROR("TLS ALPN HTTP/2 not supported for %p", io);
  (void)io;
}

/* *****************************************************************************
HTTP Listen
***************************************************************************** */

static void fio___http_listen_on_start(fio_io_protocol_s *protocol, void *u) {
  (void)u;
  fio___http_protocol_s *p =
      FIO_PTR_FROM_FIELD(fio___http_protocol_s,
                         state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                         protocol);
  p->queue = ((p->settings.queue && p->settings.queue->q) ? p->settings.queue->q
                                                          : fio_io_queue());
}

static void fio___http_listen_on_stop(fio_io_protocol_s *p, void *u) {
  (void)u;
  fio___http_protocol_free(
      FIO_PTR_FROM_FIELD(fio___http_protocol_s,
                         state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                         p));
}

void fio_http_listen___(void); /* IDE marker */
SFUNC fio_http_listener_s *fio_http_listen FIO_NOOP(const char *url,
                                                    fio_http_settings_s s) {
  fio___http_settings_validate(&s, 0);
  if (url) {
    fio_url_s u = fio_url_parse(url, FIO_STRLEN(url));
    if (u.path.len)
      FIO_LOG_WARNING(
          "HTTP listener is always at the root (home folder).\n\t"
          "  Ignoring the path set in the listening instruction: %.*s",
          (int)u.path.len,
          u.path.buf);
  }
  fio___http_protocol_s *p = fio___http_protocol_new(s.public_folder.len + 1);
  fio___http_protocol_init(p, url, s, 0);
  fio_http_listener_s *listener = (fio_http_listener_s *)
      fio_io_listen(.url = url,
                    .protocol = &p->state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                    .tls = s.tls,
                    .on_start = fio___http_listen_on_start,
                    .on_stop = fio___http_listen_on_stop,
                    .queue_for_accept = p->settings.queue);
  return listener;
}

/** Returns the a pointer to the HTTP settings associated with the listener. */
SFUNC fio_http_settings_s *fio_http_listener_settings(fio_http_listener_s *l) {
  fio___http_protocol_s *p =
      FIO_PTR_FROM_FIELD(fio___http_protocol_s,
                         state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                         fio_io_listener_protocol((fio_io_listener_s *)l));
  return &p->settings;
}

/* *****************************************************************************
HTTP Connect
***************************************************************************** */

static void fio___http_connect_on_failed(fio_io_protocol_s *p, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_http_free(c->h);
  c->h = NULL;
  fio___http_connection_free(c);
  (void)p;
}

void fio_http_connect___(void); /* IDE Marker */
/** Connects to HTTP / WebSockets / SSE connections on `url`. */
SFUNC fio_io_s *fio_http_connect FIO_NOOP(const char *url,
                                          fio_http_s *h,
                                          fio_http_settings_s s) {
  FIO_STR_INFO_TMP_VAR(origin, 4096);
  fio___http_settings_validate(&s, 1);
  fio_url_s u = (fio_url_s){0};
  if (url)
    u = fio_url_parse(url, strlen(url));

  if (!h)
    h = fio_http_new();
  if (!fio_http_path(h).len)
    fio_http_path_set(h,
                      u.path.len ? FIO_BUF2STR_INFO(u.path)
                                 : FIO_STR_INFO2((char *)"/", 1));
  if (!fio_http_query(h).len && u.query.len)
    fio_http_query_set(h, FIO_BUF2STR_INFO(u.query));
  if (!fio_http_method(h).len)
    fio_http_method_set(h, FIO_STR_INFO2((char *)"GET", 3));
  if (u.host.len) {
    fio_http_request_header_set_if_missing(h,
                                           FIO_STR_INFO2((char *)"host", 4),
                                           FIO_BUF2STR_INFO(u.host));
    /* Origin header */
    fio_string_write2(
        &origin,
        NULL,
        FIO_STRING_WRITE_STR2("https", (size_t)(4 + fio_url_is_tls(u).tls)),
        FIO_STRING_WRITE_STR2("://", 3U),
        FIO_STRING_WRITE_STR_INFO(u.host),
        FIO_STRING_WRITE_STR2(":", (size_t)(!!u.port.len)),
        FIO_STRING_WRITE_STR_INFO(u.port));
  }

  /* test for ws:// or wss:// - WebSocket scheme */
  if ((u.scheme.len == 2 ||
       (u.scheme.len == 3 && ((u.scheme.buf[2] | 0x20) == 's'))) &&
      (fio_buf2u16u(u.scheme.buf) | 0x2020) == fio_buf2u16u("ws")) {
    fio_http_request_header_set_if_missing(h,
                                           FIO_STR_INFO2((char *)"origin", 6),
                                           origin);
    fio_http_websocket_set_request(h);
  }
  /* test for sse:// or sses:// - Server Sent Events scheme */
  else if ((u.scheme.len == 3 ||
            (u.scheme.len == 4 && ((u.scheme.buf[3] | 0x20) == 's'))) &&
           (fio_buf2u32u(u.scheme.buf) | fio_buf2u32u("\x20\x20\x20\xFF")) ==
               fio_buf2u32u("sse\xFF")) {
    fio_http_request_header_set_if_missing(h,
                                           FIO_STR_INFO2((char *)"origin", 6),
                                           origin);
    fio_http_sse_set_request(h);
  }

  /* TODO: test for and attempt to re-use connection */
  // if (fio_http_cdata(h)) { }

  fio___http_protocol_s *p = fio___http_protocol_new(u.host.len);
  fio___http_protocol_init(p, url, s, 1);
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
              .on_finish = p->settings.on_finish,
              .max_header = p->settings.max_header_size,
              .max_line = p->settings.max_line_len,
          },
      .capa = p->settings.max_line_len,
      .log = p->settings.log,
      .is_client = 1,
  };
  fio_http_controller_set(h, &p->state[FIO___HTTP_PROTOCOL_HTTP1].controller);
  if (!fio_http_udata(h)) /* avoid overwriting existing `udata` if set */
    fio_http_udata_set(h, c->udata);
  fio_http_cdata_set(h, fio___http_connection_dup(c));
  return fio_io_connect(url,
                        .protocol =
                            &p->state[FIO___HTTP_PROTOCOL_HTTP1].protocol,
                        .on_failed = fio___http_connect_on_failed,
                        .udata = c,
                        .tls = s.tls,
                        .timeout = s.connect_timeout);
}

/* *****************************************************************************
HTTP WebSocket Connect (client convenience wrapper)
***************************************************************************** */

/**
 * Internal: ensures `url` carries a WebSocket scheme, writing a rewritten
 * URL into `tmp` when required. Returns either `url` (unchanged) or
 * `tmp->buf`.
 *
 * Rules: `ws://` / `wss://` pass through unchanged; `http://` becomes
 * `ws://`; `https://` becomes `wss://`; a URL with no scheme (no `://`)
 * is prefixed with `ws://`. Any other scheme passes through unchanged.
 */
FIO_IFUNC const char *fio___http_websocket_normalize_url(const char *url,
                                                         fio_str_info_s *tmp) {
  const char *sep = strstr(url, "://");
  if (!sep) {
    /* no scheme present - prepend "ws://" */
    fio_string_write2(tmp,
                      NULL,
                      FIO_STRING_WRITE_STR2("ws://", 5),
                      FIO_STRING_WRITE_STR2(url, FIO_STRLEN(url)));
    return tmp->buf;
  }
  const size_t scheme_len = (size_t)(sep - url);
  const uint32_t scheme4 =
      (scheme_len >= 4 ? (fio_buf2u32u(url) | (uint32_t)0x20202020UL) : 0UL);
  if (scheme4 == fio_buf2u32u("http") &&
      (scheme_len == 4 || (scheme_len == 5 && ((url[4] | 0x20) == 's')))) {
    /* http(s):// -> ws(s):// */
    fio_string_write2(tmp,
                      NULL,
                      FIO_STRING_WRITE_STR2("ws", 2),
                      FIO_STRING_WRITE_STR2("s", (size_t)(scheme_len == 5)),
                      FIO_STRING_WRITE_STR2(sep, FIO_STRLEN(sep)));
    return tmp->buf;
  }
  /* ws://, wss:// and any other scheme pass through unchanged */
  return url;
}

void fio_http_websocket_connect___(void); /* IDE Marker */
/**
 * Connects to a WebSocket server on `url`.
 *
 * A convenience wrapper around `fio_http_connect` that ensures a `ws://` or
 * `wss://` scheme is used in the URL (`http://` becomes `ws://`, `https://`
 * becomes `wss://`, a missing scheme defaults to `ws://`). The WebSocket
 * upgrade request / response is handled automatically by the underlying
 * `fio_http_connect`: on acceptance (101) the connection switches to the
 * WebSocket callbacks (`on_open` / `on_message` / `on_close`), otherwise
 * the response is routed to `settings.on_http`.
 */
SFUNC fio_io_s *fio_http_websocket_connect FIO_NOOP(const char *url,
                                                    fio_http_s *h,
                                                    fio_http_settings_s s) {
  FIO_STR_INFO_TMP_VAR(nurl, 4096);
  if (url)
    url = fio___http_websocket_normalize_url(url, &nurl);
  return fio_http_connect FIO_NOOP(url, h, s);
}

/* *****************************************************************************
HTTP/2 Protocol (disconnect, as HTTP/2 is unsupported)
***************************************************************************** */

// /** Called when an IO is attached to a protocol. */
// void (*on_attach)(fio_io_s *io);
// /** Called when a data is available. */
// void (*on_data)(fio_io_s *io);
// /** called once all pending `fio_io_write` calls are finished. */
// void (*on_ready)(fio_io_s *io);
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
Connection Lost
***************************************************************************** */

FIO_SFUNC void fio___http_controller_on_destroyed_task(void *c_, void *ignr_) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  fio___http_connection_free(c);
  (void)ignr_;
}

FIO_SFUNC void fio___http_controller_http1_on_finish_client_task(void *c_,
                                                                 void *h_) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  fio_http_s *h = (fio_http_s *)h_;
  c->settings->on_finish(h);
  fio_http_free(h);
  fio___http_connection_free(c);
}

FIO_SFUNC void fio___http_controller_http1_on_finish_client(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  /* on_finish should be called after the `on_close` or after on_http */
  if (!fio_http_is_upgraded(h)) {
    /* on_finish always manually called here */
    fio_io_defer(fio___http_controller_http1_on_finish_client_task,
                 (void *)fio___http_connection_dup(c),
                 (void *)fio_http_dup(h));
  }
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
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (c->state.http.buf.buf)
    FIO_STRING_FREE2(c->state.http.buf);
  c->state.http.buf = FIO_STR_INFO0;
  fio_queue_push(fio_io_queue(),
                 fio___http_controller_on_destroyed_task,
                 fio_http_cdata(h));
}

/** Called when an HTTP handle is freed (no auto-finish, post upgrade). */
FIO_SFUNC void fio__http_controller_on_destroyed2(fio_http_s *h) {
  fio_queue_push(fio_io_queue(),
                 fio___http_controller_on_destroyed_task,
                 fio_http_cdata(h));
}

/** Called when an HTTP handle is freed. */
FIO_SFUNC void fio__http_controller_on_destroyed_client(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  c->state.http.on_finish(h);
  if (c->state.http.buf.buf)
    FIO_STRING_FREE2(c->state.http.buf);
  c->state.http.buf = FIO_STR_INFO0;
  c->h = NULL;
  if (c->io)
    fio_io_close(c->io);
  fio_queue_push(fio_io_queue(), fio___http_controller_on_destroyed_task, c);
}

/* *****************************************************************************
The Protocols at play
***************************************************************************** */

/** Returns a facil.io protocol object with the proper protocol callbacks. */
FIO_IFUNC fio_io_protocol_s FIO_NOOP
fio___http_protocol_get(fio___http_protocol_selector_e s, int is_client) {
  fio_io_protocol_s r = {0};
  (void)is_client, (void)s;
  switch (s) {
  case FIO___HTTP_PROTOCOL_ACCEPT:
    r = (fio_io_protocol_s){.on_attach = fio___http_on_attach_accept,
                            .on_data = fio___http1_accept_on_data,
                            .on_close = fio___http_on_close};
    return r;
  case FIO___HTTP_PROTOCOL_HTTP1:
    if (is_client) {
      r = (fio_io_protocol_s){.on_attach = fio___http1_on_attach_client,
                              .on_data = fio___http1_on_data,
                              .on_close = fio___http_on_close};
    } else {
      r = (fio_io_protocol_s){.on_attach = fio___http1_on_attach,
                              .on_data = fio___http1_on_data,
                              .on_close = fio___http_on_close};
    }
    return r;
  case FIO___HTTP_PROTOCOL_HTTP2:
    r = (fio_io_protocol_s){.on_close = fio___http_on_close};
    return r;
  case FIO___HTTP_PROTOCOL_WS:
    r = (fio_io_protocol_s){
        .on_attach = fio___websocket_on_attach,
        .on_data = fio___websocket_on_data,
        .on_ready = fio___websocket_on_ready,
        .on_close = fio___websocket_on_close,
        .on_shutdown = fio___websocket_on_shutdown,
        .on_timeout = fio___websocket_on_timeout,
        .on_pubsub = FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT,
    };
    return r;
  case FIO___HTTP_PROTOCOL_SSE:
    r = (fio_io_protocol_s){
        .on_attach = fio___sse_on_attach,
        .on_data = (is_client ? fio___sse_on_data : NULL),
        .on_ready = fio___websocket_on_ready,
        .on_close = fio___sse_on_close,
        .on_shutdown = fio___sse_on_shutdown,
        .on_timeout = fio___sse_on_timeout,
        .on_pubsub = FIO_HTTP_SSE_SUBSCRIBE_DIRECT,
    };
    return r;
  case FIO___HTTP_PROTOCOL_NONE: /* fall through*/
    r = (fio_io_protocol_s){.on_close = fio___http_on_close};
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
        .on_destroyed = fio__http_controller_on_destroyed,
        .send_headers = fio___http_controller_http1_send_headers,
        .write_body = fio___http_controller_http1_write_body,
        .on_finish = fio___http_controller_http1_on_finish,
        .close_io = fio___http_default_close,
        .get_fd = fio___http_controller_get_fd,
    };
    return r;
  case FIO___HTTP_PROTOCOL_HTTP1:
    if (is_client) {
      r = (fio_http_controller_s){
          .on_destroyed = fio__http_controller_on_destroyed_client,
          .on_finish = fio___http_controller_http1_on_finish_client,
          .close_io = fio___http_default_close,
          .get_fd = fio___http_controller_get_fd,
      };
    } else {
      r = (fio_http_controller_s){
          .on_destroyed = fio__http_controller_on_destroyed,
          .send_headers = fio___http_controller_http1_send_headers,
          .write_body = fio___http_controller_http1_write_body,
          .on_finish = fio___http_controller_http1_on_finish,
          .close_io = fio___http_default_close,
          .get_fd = fio___http_controller_get_fd,
      };
    }
    return r;
  case FIO___HTTP_PROTOCOL_HTTP2:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed,
        .close_io = fio___http_default_close,
        .get_fd = fio___http_controller_get_fd,
    };
    return r;
  case FIO___HTTP_PROTOCOL_WS:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed2,
        .write_body = fio___http_controller_ws_write_body,
        .close_io = fio___http_default_close,
        .get_fd = fio___http_controller_get_fd,
    };
    return r;
  case FIO___HTTP_PROTOCOL_SSE:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed2,
        .write_body = fio___http_controller_sse_write_body,
        .close_io = fio___http_default_close,
        .get_fd = fio___http_controller_get_fd,
    };
    return r;
  case FIO___HTTP_PROTOCOL_NONE:
    r = (fio_http_controller_s){
        .on_destroyed = fio__http_controller_on_destroyed2,
        .close_io = fio___http_default_close,
        .get_fd = fio___http_controller_get_fd,
    };
    return r;
  default:
    FIO_LOG_ERROR("internal function `fio___http_controller_get` called with "
                  "illegal arguments!");
    return r;
  }
}

FIO_IFUNC fio___http_protocol_s *fio___http_protocol_init(
    fio___http_protocol_s *p,
    const char *url,
    fio_http_settings_s s,
    bool is_client) {
  int should_free_tls = !s.tls;
  FIO_ASSERT_ALLOC(p);
  /* zero everything out and build from there */
  *p = (fio___http_protocol_s){0};
  /* fill in protocol and controller callbacks */
  for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE + 1; ++i) {
    p->state[i].protocol =
        fio___http_protocol_get((fio___http_protocol_selector_e)i, is_client);
    p->state[i].controller =
        fio___http_controller_get((fio___http_protocol_selector_e)i, is_client);
  }
  /* fill in timeouts */
  for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE; ++i)
    p->state[i].protocol.timeout = (unsigned)s.ws_timeout * 1000U;
  p->state[FIO___HTTP_PROTOCOL_SSE].protocol.timeout =
      (unsigned)s.sse_timeout * 1000U;
  p->state[FIO___HTTP_PROTOCOL_ACCEPT].protocol.timeout =
      (unsigned)s.timeout * 1000U;
  p->state[FIO___HTTP_PROTOCOL_HTTP1].protocol.timeout =
      (unsigned)s.timeout * 1000U;
  p->state[FIO___HTTP_PROTOCOL_NONE].protocol.timeout =
      (unsigned)s.timeout * 1000U;
  /* fill in TLS data */
  if (url) {
    fio_url_s u = fio_url_parse(url, strlen(url));
    s.tls = fio_io_tls_from_url(s.tls, u);
    if (s.tls) {
      s.tls = fio_io_tls_dup(s.tls);
      /* fio_io_tls_alpn_add(s.tls, "h2", fio___http_on_select_h2); // not yet
       */
      // fio_io_tls_alpn_add(s.tls, "http/1.1", fio___http_on_select_h1);
      fio_io_functions_s tmp_fn = fio_io_tls_default_functions(NULL);
      if (!s.tls_io_func)
        s.tls_io_func = &tmp_fn;
      for (size_t i = 0; i < FIO___HTTP_PROTOCOL_NONE + 1; ++i)
        p->state[i].protocol.io_functions = *s.tls_io_func;
      if (should_free_tls)
        fio_io_tls_free(s.tls);
    }
  }
  /* fill in settings, callbacks and public folders */
  p->settings = s;
  p->on_http_callback = is_client ? fio___http_on_http_client
                        : (p->settings.public_folder.len)
                            ? fio___http_on_http_with_public_folder
                            : fio___http_on_http_direct;
  p->settings.public_folder.buf = p->public_folder_buf;
  /* queue selector is performed later (on_start) */
  p->queue = fio_io_queue();
  /* initialize initial router */
  p->router.s = p->settings;

  if (s.public_folder.len)
    FIO_MEMCPY(p->public_folder_buf, s.public_folder.buf, s.public_folder.len);
  return p;
}
/* *****************************************************************************
HTTP Helpers
***************************************************************************** */

/** Returns the IO object associated with the HTTP object (request only). */
SFUNC fio_io_s *fio_http_io(fio_http_s *h) {
  if (!h)
    return NULL;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c)
    return NULL;
  return c->io;
}

/** Returns the HTTP settings associated with the HTTP object, if any. */
SFUNC fio_http_settings_s *fio_http_settings(fio_http_s *h) {
  fio_http_settings_s *r = NULL;
  if (!h)
    return r;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c)
    return r;
  fio___http_protocol_s *p =
      FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings);
  fio_str_info_s path = fio_http_opath(h);
  r = fio___http_route_settings(&p->router, &path);
  return r;
}

/* *****************************************************************************
HTTP Glue Finish
***************************************************************************** */
#endif /* FIO_HTTP */
