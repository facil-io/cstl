/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************

              HTTP Accept - Accept Path, Dispatchers, Upgrade Authorization

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP) && !defined(FIO___RECURSIVE_INCLUDE) &&                  \
    !defined(H___FIO_HTTP_ACCEPT___H) &&                                       \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
#define H___FIO_HTTP_ACCEPT___H

/* *****************************************************************************
HTTP Request handling / handling (dispatchers and upgrade authorization)
***************************************************************************** */

/* The WebSocket / SSE upgrade-auth performers remain in `439 http.h` (pending
 * their own module files); forward declarations for `test4upgrade`. */
FIO_SFUNC void fio___http_perform_user_upgrade_callback_websocket(void *cb_,
                                                                  void *h_);
FIO_SFUNC void fio___http_perform_user_upgrade_callback_sse(void *cb_,
                                                            void *h_);

FIO_SFUNC void fio___http_perform_user_callback(void *cb_, void *h_) {
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.ptr = cb_};
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);

  if (FIO_LIKELY(c && FIO_SOCK_IS_OPEN(fio_io_fd(c->io))))
    cb.fn(h);
  fio_http_free(h);
}

FIO_IFUNC int fio___http_on_http_test4upgrade(fio_http_s *h,
                                              fio___http_connection_s *c,
                                              fio_http_settings_s *s) {
  union {
    int (*fn)(fio_http_s *);
    void *ptr;
  } cb;
  if (fio_http_websocket_requested(h))
    goto websocket_requested;
  if (fio_http_sse_requested(h))
    goto sse_requested;
  return 0;

websocket_requested:
  cb.fn = s->on_authenticate_websocket;
  fio_queue_push(c->queue,
                 fio___http_perform_user_upgrade_callback_websocket,
                 cb.ptr,
                 (void *)h);
  return -1;

sse_requested:
  cb.fn = s->on_authenticate_sse;
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
  fio_http_settings_s *s = fio___http_handle_settings(h);
  if (fio___http_on_http_test4upgrade(h, c, s))
    return;
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.fn = s->on_http};
  fio_queue_push(c->queue, fio___http_perform_user_callback, cb.ptr, (void *)h);
  (void)ignr;
}

FIO_SFUNC void fio___http_on_http_with_public_folder(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  fio_http_status_set(h, 200);
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  fio_http_settings_s *s = fio___http_handle_settings(h);
  if (fio___http_on_http_test4upgrade(h, c, s))
    return;
  /* The automatic static service answers GET / HEAD requests only (RFC
   * 9110 §9.3); any other method falls through to the application's
   * `on_http` callback, which may serve files explicitly (if desired). */
  fio_str_info_s m = fio_http_method(h);
  const uint32_t m4 = (m.len >= 3) ? (fio_buf2u32u(m.buf) | 0x20202020UL) : 0UL;
  if (s->public_folder.buf &&
      ((m.len == 3 && m4 == fio_buf2u32u("get\x20")) ||
       (m.len == 4 && m4 == fio_buf2u32u("head"))) &&
      !fio_http_static_file_response(
          h,
          s->public_folder,
          (s->public_folder.buf == c->settings->public_folder.buf
               ? fio_http_opath(h)
               : fio_http_path(h)),
          s->max_age)) {
    fio_http_free(h);
    return;
  }
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.fn = s->on_http};
  fio_queue_push(c->queue, fio___http_perform_user_callback, cb.ptr, (void *)h);
  (void)ignr;
}

FIO_SFUNC void fio___http_perform_user_callback_client(void *cb_, void *h_) {
  fio_http_s *h = (fio_http_s *)h_;
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.ptr = cb_};
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  /* unlike Server mode, handle responses from closed connections */
  cb.fn(h);
  fio_http_free(h);
  fio_io_free(c->io);
}

FIO_SFUNC void fio___http_on_http_client(void *h_, void *ignr) {
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  size_t pr = FIO___HTTP_PROTOCOL_WS;
  union {
    void (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.fn = c->state.http.on_http};

  /* TODO! review WS and SSE responses. */
  if (fio_http_websocket_accepted(h))
    goto websocket_accepted;
  if (fio_http_sse_accepted(h))
    goto sse_accepted;
  fio_queue_push(c->queue,
                 fio___http_perform_user_callback_client,
                 cb.ptr,
                 (void *)h);
  return;
  (void)ignr;

sse_accepted:
  pr = FIO___HTTP_PROTOCOL_SSE;

websocket_accepted:
  c->h = h; /* was set to NULL in `on_http_complete` */
  fio_http_controller_set(
      c->h,
      &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
            ->state[pr]
            .controller));
  fio_io_protocol_set(
      c->io,
      &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
            ->state[pr]
            .protocol));

  FIO_LOG_DDEBUG2("(%d) Client %s upgrade complete for fd %d",
                  fio_io_pid(),
                  (fio_http_is_websocket(h) ? "WebSocket" : "SSE"),
                  fio_io_fd(c->io));

  fio_io_free(c->io); /* fio_dup called by fio_http1_on_complete */
  c->suspend = 0;
  fio_io_unsuspend(c->io);
}

/* *****************************************************************************
HTTP/1.1 Accepting new connections (tests for special HTTP/2 pre-knowledge)
***************************************************************************** */

/** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___http_on_attach_accept(fio_io_s *io) {

  fio___http_protocol_s *p =
      FIO_PTR_FROM_FIELD(fio___http_protocol_s,
                         state[FIO___HTTP_PROTOCOL_ACCEPT].protocol,
                         fio_io_protocol(io));
  fio___http_protocol_dup(p);
  // p->queue = fio_io_queue();

  const uint32_t capa = p->settings.max_line_len;
  fio___http_connection_s *c = fio___http_connection_new(capa);
  FIO_ASSERT_ALLOC(c);
  *c = (fio___http_connection_s){
      .io = io,
      .settings = &(p->settings),
      .queue =
          ((p->settings.queue && p->settings.queue->q) ? p->settings.queue->q
                                                       : fio_io_queue()),
      .udata = p->settings.udata,
      .state.http =
          {
              .on_http_callback = p->on_http_callback,
              .on_http = p->settings.on_http,
              .on_finish = p->settings.on_finish,
              .max_header = p->settings.max_header_size,
              .max_line = p->settings.max_line_len,
          },
      .capa = capa,
      .log = p->settings.log,
  };
  fio_io_udata_set(io, (void *)c);
  FIO_LOG_DDEBUG2("(%d) HTTP accepted a new connection (%p)",
                  (int)fio_thread_getpid(),
                  c->io);
#if 0 /* skip pre-knowledge test? */
  fio_io_protocol_set(
      io,
      &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
            ->state[FIO___HTTP_PROTOCOL_HTTP1]
            .protocol));
#endif
}

/** Called when a data is available. */
FIO_SFUNC void fio___http1_accept_on_data(fio_io_s *io) {
  const fio_buf_info_s prior_knowledge = FIO_BUF_INFO2(
      (char *)"\x50\x52\x49\x20\x2a\x20\x48\x54\x54\x50\x2f\x32\x2e\x30"
              "\x0d\x0a\x0d\x0a\x53\x4d\x0d\x0a\x0d\x0a",
      24);
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  fio_io_protocol_s *phttp_new;
  size_t r = fio_io_read(io, c->buf + c->len, c->capa - c->len);
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
    fio_io_protocol_set(io, phttp_new);
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

  fio_io_protocol_set(io, phttp_new);
}

FIO_SFUNC void fio___http_on_close(void *buf, void *udata) {
  FIO_LOG_DDEBUG2("(%d) HTTP connection closed for %p",
                  (int)fio_thread_getpid(),
                  udata);
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  c->io = NULL;
  fio_http_free(c->h);
  fio___http_connection_free(c);
  (void)buf;
}

/* *****************************************************************************
Authentication Helper
***************************************************************************** */

/** Allows all clients to connect (bypasses authentication). */
SFUNC int FIO_HTTP_AUTHENTICATE_ALLOW(fio_http_s *h) {
  ((void)h);
  return 0;
}

/* *****************************************************************************
HTTP Accept Finish
***************************************************************************** */
#endif /* FIO_HTTP */
