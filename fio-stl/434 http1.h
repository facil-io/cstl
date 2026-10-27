/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************

              HTTP/1.1 - Request/Response Glue, Protocol and Controller

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP) && !defined(FIO___RECURSIVE_INCLUDE) &&                  \
    !defined(H___FIO_HTTP1___H) &&                                             \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
#define H___FIO_HTTP1___H

/* *****************************************************************************
HTTP/1.1 Request / Response Completed
***************************************************************************** */

/** called when either a request or a response was received. */
static void fio_http1_on_complete(void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (!c->h) { /* detached (i.e., aborted with an error response) */
    c->state.http.header_bytes = 0;
    return;
  }
  if (c->is_client) {
    /* skip interim 1xx responses, except 101 (RFC 9110 §15.2) - the handle
     * waits for the final response (the next status line resets it). */
    const size_t status = fio_http_status(c->h);
    if ((status - 100) < 100 && status != 101) {
      c->state.http.header_bytes = 0;
      return;
    }
  }
  fio_io_dup(c->io); /* make sure the IO and its data are valid in callback */
  fio_io_suspend(c->io);
  fio_http_s *h = c->h;
  c->h = NULL;
  c->state.http.header_bytes = 0;
  c->suspend = 1;
  if (FIO_LIKELY(!fio_queue_push(fio_io_queue(),
                                 c->state.http.on_http_callback,
                                 h,
                                 NULL)))
    return;
  /* the task (owning `h` and the IO reference) was never scheduled: re-attach
   * `h` so the connection's `on_close` releases it (as for any unfinished
   * message), drop the completion-time IO reference and close. */
  FIO_LOG_ERROR("(%d) HTTP/1.1 couldn't schedule message handling for fd %d"
                " - closing connection.",
                fio_io_pid(),
                (int)fio_io_fd(c->io));
  c->h = h;
  fio_io_close(c->io);
  fio_io_free(c->io);
}

/* *****************************************************************************
HTTP/1.1 Parser callbacks
***************************************************************************** */

/** Sends an error response and detaches the request (stops processing it). */
FIO_IFUNC void fio___http_request_abort(fio___http_connection_s *c,
                                        size_t status) {
  fio_http_s *h = c->h;
  fio_io_dup(c->io); /* sending the response will result in fio_undup */
  fio_io_suspend(c->io);
  c->h = NULL;
  c->suspend = 1;
  if (fio_http_send_error_response(h, status))
    fio_io_free(c->io); /* response not sent, we need to fio_undup */
  fio_http_free(h);
}

FIO_IFUNC void fio___http_request_too_big(fio___http_connection_s *c) {
  fio___http_request_abort(c, 413);
}

/* Attaches a new request handle. Returns -1 (nothing attached) on failure. */
FIO_IFUNC int fio_http1_attach_handle(fio___http_connection_s *c) {
  c->h = fio_http_new();
  if (FIO_UNLIKELY(!c->h)) {
    /* no handle, no error response: the parser error closes the connection */
    FIO_LOG_ERROR("(%d) HTTP/1.1 couldn't allocate a request handle for fd %d"
                  " - closing connection.",
                  fio_io_pid(),
                  (int)fio_io_fd(c->io));
    return -1;
  }
  fio_http_controller_set(
      c->h,
      &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings))
           ->state[FIO___HTTP_PROTOCOL_HTTP1]
           .controller);
  fio_http_udata_set(c->h, c->udata);
  fio_http_cdata_set(c->h, fio___http_connection_dup(c));
  if (c->settings->compress_dynamic)
    fio___http_cflags_set(c->h, FIO___HTTP_CFLAG_COMPRESS_DYNAMIC);
  if (c->settings->compress_ws)
    fio___http_cflags_set(c->h, FIO___HTTP_CFLAG_COMPRESS_WS);
  return 0;
}

/* Returns 0 if a parsed value was stored as is. Otherwise (no memory, or a
 * value the handle rejects, i.e., a bare CR) fails the message: request
 * identity / authorization inputs never silently become empty or defaults. */
FIO_SFUNC int fio___http1_stored(fio___http_connection_s *c,
                                 fio_str_info_s stored,
                                 size_t expected_len) {
  if (FIO_LIKELY(stored.len == expected_len))
    return 0;
  FIO_LOG_ERROR("(%d) HTTP/1.1 couldn't store a parsed value for fd %d"
                " - closing connection.",
                fio_io_pid(),
                (int)fio_io_fd(c->io));
  if (c->h && !c->is_client) /* a client never sends error responses */
    fio___http_request_abort(c, 400);
  return -1; /* parser error: closes the connection (after the response) */
}

/** called when a request method is parsed. */
static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (c->h || fio_http1_attach_handle(c))
    return -1;
  return fio___http1_stored(c,
                            fio_http_method_set(c->h, FIO_BUF2STR_INFO(method)),
                            method.len);
}
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_http_clear_response(c->h, istatus != 301 && istatus != 302);
  fio_http_status_set(c->h, istatus);
  c->state.http.body_skipped = 0;
  { /* responses to HEAD requests never have a body (RFC 9110 §9.3.2) */
    fio_str_info_s m = fio_http_method(c->h);
    if (m.len == 4 &&
        (fio_buf2u32u(m.buf) | (uint32_t)0x20202020UL) == fio_buf2u32u("head"))
      fio_http1_parser_skip_body(&c->state.http.parser);
  }
  return 0;
  (void)status;
}
/** called when a request URL is parsed. */
static int fio_http1_on_url(fio_buf_info_s url, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_url_s u = fio_url_parse(url.buf, url.len);
  if (!u.path.len || u.path.buf[0] != '/')
    return -1;
  if (fio___http1_stored(c,
                         fio_http_path_set(c->h, FIO_BUF2STR_INFO(u.path)),
                         u.path.len) ||
      fio___http1_stored(c,
                         fio_http_opath_set(c->h, FIO_BUF2STR_INFO(u.path)),
                         u.path.len))
    return -1;
  if (u.query.len &&
      fio___http1_stored(c,
                         fio_http_query_set(c->h, FIO_BUF2STR_INFO(u.query)),
                         u.query.len))
    return -1;
  if (u.host.len) { /* absolute-form: the target's authority is the Host and
                     * a received Host field is ignored (RFC 9112 §3.2.2) */
    /* a slice of `url`: restore the `[IPv6]` brackets the URL parser strips,
     * then extend to the end of the port (which follows the host) */
    const char *const url_end = url.buf + url.len;
    const size_t v6 = (size_t)(u.host.buf > url.buf && u.host.buf[-1] == '[' &&
                               u.host.buf + u.host.len < url_end &&
                               u.host.buf[u.host.len] == ']');
    fio_str_info_s authority =
        FIO_STR_INFO2((char *)u.host.buf - v6, u.host.len + (v6 << 1));
    if (u.port.len)
      authority.len = (size_t)((u.port.buf + u.port.len) - authority.buf);
    if (fio___http1_stored(
            c,
            fio_http_request_header_set(c->h,
                                        FIO_STR_INFO2((char *)"host", 4),
                                        authority),
            authority.len))
      return -1;
  }
  return 0;
}
/** called when a the HTTP/1.x version is parsed. */
static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (!c->h)
    goto unexpected_response;
  return fio___http1_stored(
      c,
      fio_http_version_set(c->h, FIO_BUF2STR_INFO(version)),
      version.len);
unexpected_response: /* (requests always create a handle in `on_method`) */
  FIO_LOG_SECURITY("(%d) HTTP/1.1 %s at fd %d - disconnecting.",
                   fio_io_pid(),
                   (c->is_client ? "unsolicited response received by client"
                                 : "response received by server"),
                   (int)fio_io_fd(c->io));
  return -1; /* parser error: the connection is closed */
}
/** called when a header is parsed. */
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (!c->h)
    return 0; /* ignore possible post-error response headers */
  const size_t line_len = name.len + value.len;
  if (FIO_UNLIKELY(line_len > c->state.http.max_line ||
                   line_len > (size_t)(c->state.http.max_header -
                                       c->state.http.header_bytes))) {
    if (!c->is_client)
      fio___http_request_abort(c, 431);
    return -1;
  }
  c->state.http.header_bytes += (uint32_t)line_len;
  if (fio_http_status(c->h))
    return fio___http1_stored(
        c,
        fio_http_response_header_add(c->h,
                                     FIO_BUF2STR_INFO(name),
                                     FIO_BUF2STR_INFO(value)),
        value.len);
  /* only an absolute-form target sets Host before the (single) Host field */
  if (name.len == 4 &&
      (fio_buf2u32u(name.buf) | (uint32_t)0x20202020UL) ==
          fio_buf2u32u("host") &&
      fio_http_request_header(c->h, FIO_STR_INFO2((char *)"host", 4), 0).buf)
    return 0;
  return fio___http1_stored(
      c,
      fio_http_request_header_add(c->h,
                                  FIO_BUF2STR_INFO(name),
                                  FIO_BUF2STR_INFO(value)),
      value.len);
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
  if (!fio_http1_parser_skips_body(&c->state.http.parser)) {
    if (content_length > c->settings->max_body_size)
      goto too_big;
    if (content_length)
      fio_http_body_expect(c->h, content_length);
    if (!FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER)
      return 0;
  }
  /* a skipped body (i.e., HEAD response) reserves nothing: its length is
   * informational, so it stays visible as a header. */
  (!(h->status) ? fio_http_request_header_add
                : fio_http_response_header_add)(h,
                                                FIO_BUF2STR_INFO(name),
                                                FIO_BUF2STR_INFO(value));
  return 0;
too_big:
  if (c->is_client)
    return -1; /* parser error: close, a client never sends error responses */
  fio___http_request_too_big(c);
  return 0; /* should we disconnect (return -1), or not? */
  (void)name, (void)value;
}
/** called when `Expect` arrives and may require a 100 continue response. */
static int fio_http1_on_expect(void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  const fio_buf_info_s response =
      FIO_BUF_INFO1((char *)"HTTP/1.1 100 Continue\r\n\r\n");
  fio_http_s *h = c->h;
  if (!h)
    return 1;
  /* a non-zero return is a parser error: the connection closes (once the
   * error response is sent) instead of parsing the body as a request. */
  c->h = NULL;
  /* test for body size violation and deny request if payload too big. */
  if (FIO_HTTP1_EXPECTED_CHUNKED != fio_http1_expected(&c->state.http.parser) &&
      c->settings->max_body_size < fio_http1_expected(&c->state.http.parser))
    goto payload_too_big;
  fio_io_dup(c->io); /* completion-time reference, in case `h` finishes */
  c->settings->pre_http_body(h);
  if (fio_http_status(h))
    goto response_sent; /* `on_finish` (now or on free) releases the dup */
  fio_io_free(c->io);
  c->h = h;
  fio_io_write2(c->io, .buf = response.buf, .len = response.len, .copy = 0);
  return 0; /* TODO?: improve support for `expect` headers? */
payload_too_big:
  fio_io_dup(c->io);
  if (fio_http_send_error_response(h, 413))
    fio_io_free(c->io); /* response not sent, we need to fio_undup */
                        /* fall through */
response_sent:
  // c->h = NULL;
  fio_http_free(h);
  return 1;
}

/** called when a body chunk is parsed. */
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (!c->h)
    return -1; /* close connection if a large payload is unstoppable */
  const size_t limit = c->settings->max_body_size;
  if (c->is_client &&
      (fio_http_status(c->h) == 301 || fio_http_status(c->h) == 302)) {
    /* don't overwrite the client's payload on redirect, but still limit the
     * discarded body (chunked framing declares no total length) */
    const size_t skipped = c->state.http.body_skipped; /* <= limit */
    if (chunk.len > limit - skipped)
      return -1; /* parser error: a client closes the connection */
    c->state.http.body_skipped = skipped + chunk.len;
    return 0;
  }
  {
    const size_t stored = fio_http_body_length(c->h);
    if (stored > limit || chunk.len > limit - stored)
      goto too_big;
  }
  if (fio_http_body_write(c->h, chunk.buf, chunk.len))
    goto write_failed;
  return 0;
write_failed:
  /* the body couldn't be stored (i.e., no temporary file): the server sends
   * a 500 error response, both sides close the connection. */
  FIO_LOG_DEBUG2("(%d) HTTP/1.1 couldn't store body, closing connection %d",
                 fio_io_pid(),
                 fio_io_fd(c->io));
  if (!c->is_client)
    fio___http_request_abort(c, 500);
  return -1; /* parser error: closes the connection (after the response) */
too_big:
  if (!c->is_client) /* a client never sends error responses */
    fio___http_request_too_big(c);
  return -1; /* stop parsing: the 413 response is sent before closing */
}

/* *****************************************************************************
HTTP/1.1 Protocol
***************************************************************************** */

FIO_SFUNC int fio___http1_process_data(fio_io_s *io,
                                       fio___http_connection_s *c) {
  (void)io;
  for (;;) {
    size_t consumed = fio_http1_parse(&c->state.http.parser,
                                      FIO_BUF_INFO2(c->buf, c->len),
                                      (void *)c);
    if (!consumed)
      goto nothing_consumed;
    if (consumed == FIO_HTTP1_PARSER_ERROR)
      goto http1_error;
    c->len -= consumed;
    if (c->len)
      FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
    if (c->suspend)
      return -1;
  }
  return 0;

nothing_consumed:
  if (c->len == c->capa)
    goto http1_abuse;
  else
    return -1;

http1_error:
  FIO_LOG_DDEBUG2("(%d) HTTP/1.1 parser error! disconnecting client at %d",
                  fio_io_pid(),
                  fio_io_fd(io));
  if (c->h) {
    fio_http_s *h = c->h;
    c->h = NULL;
    if (!c->is_client) {
      fio_io_dup(c->io);
      if (fio_http_send_error_response(h, 400))
        fio_io_free(c->io);
    }
    fio_http_free(h);
  }
  fio_io_close(io);
  return -1;

http1_abuse:
  FIO_LOG_DDEBUG2(
      "(%d) HTTP/1.1 hit security limit, disconnecting client at %d",
      fio_io_pid(),
      fio_io_fd(io));
  if (c->h) {
    fio_http_s *h = c->h;
    c->h = NULL;
    if (!c->is_client) {
      fio_io_dup(c->io);
      if (fio_http_send_error_response(h, 431))
        fio_io_free(c->io);
    }
    fio_http_free(h);
  }
  fio_io_close(io);
  return -1;
}

// /** Called when a data is available. */
FIO_SFUNC void fio___http1_on_data(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  size_t r;
  for (;;) {
    if (c->capa == c->len)
      return;
    if (!(r = fio_io_read(io, c->buf + c->len, c->capa - c->len)))
      return;
    c->len += r;
    if (fio___http1_process_data(io, c))
      return;
  }
}

// /** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___http1_on_attach(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  if (c->len)
    fio___http1_process_data(io, c);
  return;
}

/* *****************************************************************************
HTTP/1.1 Client Protocol
***************************************************************************** */

/* Serialization buffer: `err` latches any failed (allocating) write. */
typedef struct {
  fio_str_info_s s;
  int err;
} fio___http1_wbuf_s;

/** Writes a client cookie. A non-zero return will stop iteration. */
FIO_SFUNC int fio_http1___write_client_cookie_callback(fio_http_s *h,
                                                       fio_str_info_s name,
                                                       fio_str_info_s value,
                                                       void *out_) {
  fio___http1_wbuf_s *out = (fio___http1_wbuf_s *)out_;
  return (out->err |= fio_string_write2(&out->s,
                                        FIO_STRING_REALLOC,
                                        FIO_STRING_WRITE_STR2("cookie:", 7),
                                        FIO_STRING_WRITE_STR_INFO(name),
                                        FIO_STRING_WRITE_STR2("=", 1),
                                        FIO_STRING_WRITE_STR_INFO(value),
                                        FIO_STRING_WRITE_STR2("\r\n", 2)));
  (void)h;
}

/** called by the HTTP handle for each header. */
FIO_SFUNC int fio_http1___write_header_callback(fio_http_s *h,
                                                fio_str_info_s name,
                                                fio_str_info_s value,
                                                void *out_) {
  fio___http1_wbuf_s *out = (fio___http1_wbuf_s *)out_;
  return (out->err |=
          fio_string_write2(&out->s,
                            FIO_STRING_REALLOC,
                            FIO_STRING_WRITE_STR2(name.buf, name.len),
                            FIO_STRING_WRITE_STR2(":", 1),
                            FIO_STRING_WRITE_STR2(value.buf, value.len),
                            FIO_STRING_WRITE_STR2("\r\n", 2)));
  (void)h;
}

/**
 * Returns non-zero if `l` (without CRLF) is a serialized request-line that
 * can't alter the message framing: `method SP request-target SP HTTP/D.D`,
 * where `method` is a token and the target holds no SP, CTL or DEL bytes.
 */
FIO_SFUNC int fio___http1_request_line_is_valid(const char *l, size_t len) {
  const char *const end = l + len;
  const char *pos = l;
  while (pos < end && !FIO___HTTP_FORBIDDEN_NAME_CHARS[(uint8_t)*pos])
    ++pos;
  if (pos == l || pos == end || *pos != ' ')
    return 0;
  const char *const target = ++pos;
  while (pos < end && (uint8_t)*pos > 0x20 && (uint8_t)*pos != 0x7F)
    ++pos;
  if (pos == target || end - pos != 9 || *pos != ' ')
    return 0;
  ++pos;
  return fio_buf2u32u(pos) == fio_buf2u32u("HTTP") && pos[4] == '/' &&
         (uint8_t)(pos[5] - '0') < 10 && pos[6] == '.' &&
         (uint8_t)(pos[7] - '0') < 10;
}

FIO_SFUNC void fio___http1_send_request(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c->io || !fio_io_is_open(c->io))
    return;
  fio___http1_wbuf_s buf = {.s = FIO_STR_INFO2(NULL, 0)};
  /* set Content-Length (client is never streaming) */
  if (fio_http_body_length(h)) {
    char ibuf[32];
    fio_str_info_s k = FIO_STR_INFO2((char *)"content-length", 14);
    fio_str_info_s v = FIO_STR_INFO3(ibuf, 0, 32);
    v.len = fio_digits10u(fio_http_body_length(h));
    fio_ltoa10u(v.buf, fio_http_body_length(h), v.len);
    fio_http_request_header_set(h, k, v);
    /* the body is framed by exactly this (single) stored value, or not sent:
     * a failed setter leaves the header missing (or, never, stale). */
    fio_str_info_s stored = fio_http_request_header(h, k, 0);
    if (FIO_UNLIKELY(!FIO_STR_INFO_IS_EQ(stored, v) ||
                     fio_http_request_header(h, k, 1).buf))
      goto serialize_error;
  }
  { /* set sensible defaults for common headers (Accept, User-Agent) */
    fio_http_request_header_set_if_missing(h,
                                           FIO_STR_INFO1((char *)"accept"),
                                           FIO_STR_INFO1((char *)"*/*"));
    fio_http_request_header_set_if_missing(
        h,
        FIO_STR_INFO1((char *)"user-agent"),
        FIO_STR_INFO1((char *)"facil.io/" FIO_VERSION_STRING));
  }
  { /* write and validate the request-line, as sent (before any output) */
    fio_str_info_s method = fio_http_method(h);
    fio_str_info_s path = fio_http_path(h);
    fio_str_info_s query = fio_http_query(h);
    fio_str_info_s version = fio_http_version(h);
    if (!path.len)
      path = FIO_STR_INFO1((char *)"/");
    if ((version.len - 1) > 15)
      version = FIO_STR_INFO1((char *)"HTTP/1.1");
    buf.err =
        fio_string_write2(&buf.s,
                          FIO_STRING_REALLOC,
                          FIO_STRING_WRITE_STR_INFO(method),
                          FIO_STRING_WRITE_STR2(" ", 1),
                          FIO_STRING_WRITE_STR_INFO(path),
                          FIO_STRING_WRITE_STR2("?", (size_t)(query.len > 0)),
                          FIO_STRING_WRITE_STR_INFO(query),
                          FIO_STRING_WRITE_STR2(" ", 1),
                          FIO_STRING_WRITE_STR_INFO(version),
                          FIO_STRING_WRITE_STR2("\r\n", 2));
    if (buf.err)
      goto serialize_error;
    if (!fio___http1_request_line_is_valid(buf.s.buf, buf.s.len - 2))
      goto invalid_request_line;
  }
  /* write headers */
  fio_http_request_header_each(h, fio_http1___write_header_callback, &buf);
  /* write cookies */
  if (!buf.err)
    fio_http_cookie_each(h, fio_http1___write_client_cookie_callback, &buf);
  if (buf.err || fio_string_write(&buf.s, FIO_STRING_REALLOC, "\r\n", 2))
    goto serialize_error;
  /* send data (moves memory ownership) */
  fio_io_write2(c->io,
                .buf = buf.s.buf,
                .len = buf.s.len,
                .dealloc = FIO_STRING_FREE,
                .copy = 0);
  /* make sure we listen to incoming data */
  c->suspend = 0;
  fio_io_unsuspend(c->io);
  /* Write Body */
  if (!fio_http_body_length(h))
    return;
  fio_http_body_seek(h, 0);
  if (fio_http_body_fd(h) == -1) {
    /* zero-copy: send a reference to the RAM body (its own allocation) */
    fio_io_write2(c->io,
                  .buf = fio_bstr_copy(fio_data_bstr(&h->body)),
                  .len = fio_http_body_length(h),
                  .dealloc = ((union {
                               void (*bstr)(char *);
                               void (*dealloc)(void *);
                             }){.bstr = fio_bstr_free})
                                 .dealloc);
  } else {
    fio_io_write2(c->io,
                  .fd = fio_http_body_fd(h),
                  .len = fio_http_body_length(h),
                  .copy = 1);
  }
  return;

invalid_request_line:
  FIO_LOG_SECURITY("(%d) HTTP/1.1 client request-line (method, path, query "
                   "or version) contains illegal bytes - not sent.",
                   fio_io_pid());
  goto close_connection;
serialize_error:
  FIO_LOG_ERROR("(%d) HTTP/1.1 client couldn't serialize request - not sent.",
                fio_io_pid());
close_connection:
  /* nothing was written: the connection's `on_close` releases `h` */
  FIO_STRING_FREE2(buf.s);
  fio_io_close(c->io);
}

FIO_SFUNC void fio___http1_on_attach_client(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  // c->io = fio_io_dup(io);
  c->io = io;
  /* async queues resolve once the reactor runs - select it on attach */
  c->queue = fio___http_settings_queue(c->settings);
  fio___http1_send_request(c->h);
  if (c->len)
    fio___http1_process_data(io, c);
  return;
}

/* *****************************************************************************
HTTP/1 Controller
***************************************************************************** */
FIO_SFUNC int fio___http_controller_get_fd(fio_http_s *h) {
  return fio_io_fd(fio_http_io(h));
}

/* Returns true for HTTP/1.0 handles (anything else is HTTP/1.1). */
FIO_IFUNC int fio___http1_is_legacy(fio_http_s *h) {
  return fio_http1_version_is_legacy(FIO_STR2BUF_INFO(fio_http_version(h)));
}

/* Streamed responses use chunked framing only for HTTP/1.1+ (RFC 9112 §6.1).
 * HTTP/1.0 streams are delimited by closing the connection. */
FIO_IFUNC int fio___http1_is_chunked(fio_http_s *h) {
  return fio_http_is_streaming(h) && !fio___http1_is_legacy(h);
}

/* Request `Connection` options (RFC 9110 §7.6.1) as bit flags. */
#define FIO___HTTP1_CONN_CLOSE      1
#define FIO___HTTP1_CONN_KEEP_ALIVE 2

/* Collects request Connection options (defined below). */
FIO_SFUNC int fio___http1_connection_options(fio_http_s *h);

/** Informs the controller that request / response headers must be sent. */
FIO_SFUNC void fio___http_controller_http1_send_headers(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c->io || !fio_io_is_open(c->io))
    return;
  fio___http1_wbuf_s buf = {.s = FIO_STR_INFO2(NULL, 0)};
  { /* write status string */
    fio_str_info_s ver = fio_http_version(h);
    fio_str_info_s status = fio_http_status2str(fio_http_status(h));
    if ((ver.len - 1) > 14) { /* missing (early error) or too long */
      if (ver.len)
        FIO_LOG_ERROR("HTTP/1.1 client version string too long!");
      ver = FIO_STR_INFO1((char *)"HTTP/1.1");
    }
    buf.err = fio_string_write2(&buf.s,
                                FIO_STRING_REALLOC,
                                FIO_STRING_WRITE_STR2(ver.buf, ver.len),
                                FIO_STRING_WRITE_STR2(" ", 1),
                                FIO_STRING_WRITE_NUM(fio_http_status(h)),
                                FIO_STRING_WRITE_STR2(" ", 1),
                                FIO_STRING_WRITE_STR2(status.buf, status.len),
                                FIO_STRING_WRITE_STR2("\r\n", 2));
  }
  /* write headers */
  if (!buf.err)
    fio_http_response_header_each(h, fio_http1___write_header_callback, &buf);
  /* write cookies */
  if (!buf.err)
    fio_http_set_cookie_each(h, fio_http1___write_header_callback, &buf);
  /* add streaming headers? */
  if (buf.err)
    goto serialize_error;
  /* 205 MUST indicate zero length (RFC 9110 §15.3.6): when no value was
   * stored (no memory), frame it here without the header map */
  if (FIO_UNLIKELY(fio_http_status(h) == 205) &&
      !fio_http_response_header(h,
                                FIO_STR_INFO2((char *)"content-length", 14),
                                0)
           .buf &&
      fio_string_write(&buf.s, FIO_STRING_REALLOC, "content-length:0\r\n", 18))
    goto serialize_error;
  if (fio___http1_is_chunked(h))
    buf.err = fio_string_write(&buf.s,
                               FIO_STRING_REALLOC,
                               "transfer-encoding: chunked\r\n",
                               28);
  /* HTTP/1.0 persistence must be confirmed (RFC 9112 §C.2.2) */
  else if (fio___http1_is_legacy(h) && !c->is_client &&
           !fio_http_is_streaming(h) &&
           fio___http1_connection_options(h) == FIO___HTTP1_CONN_KEEP_ALIVE &&
           !fio_http_response_header(h,
                                     FIO_STR_INFO2((char *)"connection", 10),
                                     0)
                .buf)
    buf.err = fio_string_write(&buf.s,
                               FIO_STRING_REALLOC,
                               "connection:keep-alive\r\n",
                               23);
  if (buf.err || fio_string_write(&buf.s, FIO_STRING_REALLOC, "\r\n", 2))
    goto serialize_error;
  /* sent with the first body write or when the response finishes */
  c->state.http.buf = buf.s;
  return;

serialize_error:
  /* never emit a partial header block: discard it and close (output written
   * before this response still drains, later writes are discarded). */
  FIO_LOG_ERROR("(%d) HTTP/1.1 couldn't serialize response headers for fd %d"
                " - closing connection.",
                fio_io_pid(),
                (int)fio_io_fd(c->io));
  FIO_STRING_FREE2(buf.s);
  fio_io_close(c->io);
}

/* Sends (moves) the pending output buffer, if any. */
FIO_IFUNC void fio___http1_flush(fio___http_connection_s *c) {
  if (!c->state.http.buf.buf)
    return;
  fio_io_write2(c->io,
                .buf = (void *)c->state.http.buf.buf,
                .len = c->state.http.buf.len,
                .dealloc = FIO_STRING_FREE);
  c->state.http.buf = FIO_STR_INFO0;
}

/* Sends pending response headers (if any) followed by a chunk-size line.
 * The header buffer is grown as needed: it may have no spare capacity.
 * Returns -1 (writing nothing) on failure. */
FIO_SFUNC int fio___http1_write_chunk_header(fio___http_connection_s *c,
                                             size_t len) {
  const size_t org = c->state.http.buf.len;
  if (fio_string_write2(&c->state.http.buf,
                        FIO_STRING_REALLOC,
                        FIO_STRING_WRITE_HEX(len),         /* chunk length */
                        FIO_STRING_WRITE_STR2("\r\n", 2))) /* chunk EOL */
    goto error;
  fio___http1_flush(c);
  return 0;
error:
  c->state.http.buf.len = org; /* drop any truncated (partial) output */
  return -1;
}

/* Returns -1 if a regular file holds less than `len` bytes at `offset`.
 * Other file types can't be checked (0). A file truncated after this check is
 * still cut short by the IO stream. */
FIO_SFUNC int fio___http1_fd_range_check(int fd, size_t offset, size_t len) {
  if (fio_fd_type(fd) != S_IFREG)
    return 0;
  const size_t size = fio_fd_size(fd);
  return 0 - (offset > size || len > size - offset);
}

/** called by the HTTP handle for each body chunk (or to finish a response. */
FIO_SFUNC void fio___http_controller_http1_write_body(
    fio_http_s *h,
    fio_http_write_args_s args) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c->io || !fio_io_is_open(c->io))
    goto no_write_err;
  if (fio___http1_is_chunked(h))
    goto stream_chunk;
  /* a file shorter than its declared length would desynchronize the peer */
  if (!args.buf && args.len &&
      fio___http1_fd_range_check(args.fd, args.offset, args.len))
    goto length_err;
  if (c->state.http.buf.len) {
    if (args.buf && args.len) { /* coalesce small writes with the headers */
      const size_t org = c->state.http.buf.len;
      if (!fio_string_write(&c->state.http.buf,
                            FIO_STRING_REALLOC,
                            (char *)args.buf + args.offset,
                            args.len)) {
        if (args.dealloc)
          args.dealloc((void *)args.buf);
        fio___http1_flush(c);
        return;
      }
      c->state.http.buf.len = org; /* failed: send the data separately */
    }
    fio___http1_flush(c);
  }

  fio_io_write2(c->io,
                .buf = (void *)args.buf,
                .fd = args.fd,
                .len = args.len,
                .offset = args.offset,
                .dealloc = args.dealloc,
                .copy = (uint8_t)args.copy);
  return;

stream_chunk:
  if (args.len && args.buf) { /* String (data starts at `offset`) */
    if (args.copy || args.len < (1 << 16)) {
      const size_t org = c->state.http.buf.len;
      if (fio_string_write2(
              &c->state.http.buf,
              FIO_STRING_REALLOC,
              FIO_STRING_WRITE_HEX(args.len),   /* chunk header - length */
              FIO_STRING_WRITE_STR2("\r\n", 2), /* chunk header - EOL */
              FIO_STRING_WRITE_STR2((char *)args.buf + args.offset, args.len),
              FIO_STRING_WRITE_STR2("\r\n", 2))) { /* chunk trailer - EOL */
        c->state.http.buf.len = org;
        goto serialize_err;
      }
      fio___http1_flush(c);
      if (args.dealloc)
        args.dealloc((void *)args.buf); /* the original pointer */
      return;
    } else { /* avoid copying the incoming data if possible */
      if (fio___http1_write_chunk_header(c, args.len))
        goto serialize_err;
      fio_io_write2(c->io,
                    .buf = (void *)args.buf,
                    .offset = args.offset,
                    .len = args.len,
                    .dealloc = args.dealloc);
      /* chunk trailer - EOL */
      fio_io_write2(c->io, .buf = (void *)"\r\n", .len = 2);
      return;
    }
  } else if ((uint32_t)(args.fd + 1) > 1U) { /* File? */
    if (!args.len) {
      /* the rest of a regular file, from `offset` (as for non-chunked IO) */
      if (fio_fd_type(args.fd) != S_IFREG)
        goto no_length_err;
      const size_t size = fio_fd_size(args.fd);
      if (size <= args.offset)
        goto no_length_err;
      args.len = size - args.offset;
    } else if (fio___http1_fd_range_check(args.fd, args.offset, args.len))
      goto length_err;
    if (fio___http1_write_chunk_header(c, args.len))
      goto serialize_err;
    fio_io_write2(c->io,
                  .fd = args.fd,
                  .offset = args.offset,
                  .len = args.len,
                  .copy = (bool)args.copy,
                  .dealloc = args.dealloc);
    /* chunk trailer - EOL */
    fio_io_write2(c->io, .buf = (void *)"\r\n", .len = 2);
  } else
    goto no_write_err;
  return;
length_err:
  /* the declared framing can't be honored: never send a short body */
  FIO_LOG_ERROR("(%d) HTTP/1.1 file body (fd %d) is shorter than its declared"
                " length - closing connection.",
                fio_io_pid(),
                (int)args.fd);
  FIO_STRING_FREE2(c->state.http.buf); /* unsent headers (if any) */
  c->state.http.buf = FIO_STR_INFO0;
  fio_io_close(c->io);
  goto no_write_err;
serialize_err:
  /* a chunk can't be skipped (the stream would be corrupt): close */
  FIO_LOG_ERROR("(%d) HTTP/1.1 couldn't frame a body chunk for fd %d"
                " - closing connection.",
                fio_io_pid(),
                (int)fio_io_fd(c->io));
  fio_io_close(c->io);
  goto no_write_err;
no_length_err:
  FIO_LOG_ERROR("HTTP1 streaming requires a correctly pre-determined "
                "length per chunk.");
no_write_err:
  if (args.buf) {
    if (args.dealloc)
      args.dealloc((void *)args.buf);
  } else if ((uint32_t)(args.fd + 1) > 1U && !args.copy) {
    close(args.fd);
  }
}

FIO_SFUNC void fio___http_controller_http1_on_finish_task(void *c_,
                                                          void *upgraded) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  c->suspend = 0;

  if (upgraded)
    goto upgraded;

  if (FIO_UNLIKELY(!c->io)) /* never queued without an IO reference */
    return;

  if (fio_io_is_open(c->io)) {
    fio___http1_process_data(c->io, c);
    if (!c->suspend && fio_io_is_open(c->io))
      fio_io_unsuspend(c->io);
  }
  fio_io_free(c->io);
  return;

upgraded:
  /* this task owns the `upgraded` handle reference (and the completion-time
   * IO reference) on every path. */
  if (c->h || !fio_io_is_open(c->io))
    goto something_is_wrong;
  c->h = (fio_http_s *)upgraded;
  { /* TODO! test if safe to move to user thread for callback execution? */
    const size_t pr_i = fio_http_is_websocket(c->h) ? FIO___HTTP_PROTOCOL_WS
                                                    : FIO___HTTP_PROTOCOL_SSE;
    fio_http_controller_set(
        c->h,
        &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
              ->state[pr_i]
              .controller));
    fio_io_protocol_set(
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
  fio_io_unsuspend(c->io);
  fio_io_free(c->io);
  return;

something_is_wrong:
  /* the transition never happened (i.e., the peer closed first). The HTTP/1
   * protocol stays in place, so `on_close` releases `c->h` and the IO's
   * connection reference; freeing the (upgraded, finished) handle releases
   * its connection reference without any further output. */
  if (fio_io_is_open(c->io))
    FIO_LOG_DEBUG2("(%d) Connection upgrade went wrong for fd %d - closing",
                   fio_io_pid(),
                   fio_io_fd(c->io));
  fio_io_close(c->io);
  fio_http_free((fio_http_s *)upgraded);
  fio_io_free(c->io);
}

/* Collects the `close` / `keep-alive` options of all request Connection
 * fields. */
FIO_SFUNC int fio___http1_connection_options(fio_http_s *h) {
  int r = 0;
  fio___http_sary_s *a = fio___http_hmap_node2val_ptr(
      fio___http_hmap_get_ptr(HTTP_HDR_REQUEST(h),
                              FIO_STR_INFO2((char *)"connection", 10)));
  if (!a)
    return r;
  FIO_ARRAY_EACH(fio___http_sary, a, pos) {
    fio_buf_info_s v = fio_bstr_buf(*pos.p);
    while (v.len) {
      char *comma = (char *)FIO_MEMCHR(v.buf, ',', v.len);
      const size_t consumed = comma ? (size_t)(comma - v.buf) + 1 : v.len;
      size_t first = 0;
      size_t last = comma ? consumed - 1 : consumed;
      while (first < last && (v.buf[first] == ' ' || v.buf[first] == '\t'))
        ++first;
      while (last > first &&
             (v.buf[last - 1] == ' ' || v.buf[last - 1] == '\t'))
        --last;
      if (last - first == 5 && (v.buf[first] | 0x20) == 'c' &&
          (fio_buf2u32u(v.buf + first + 1) | (uint32_t)0x20202020UL) ==
              fio_buf2u32u("lose"))
        r |= FIO___HTTP1_CONN_CLOSE;
      else if (last - first == 10 &&
               (fio_buf2u64u(v.buf + first) |
                (uint64_t)0x2020202020202020ULL) ==
                   (fio_buf2u64u("keep-alive") |
                    (uint64_t)0x2020202020202020ULL) &&
               (fio_buf2u16u(v.buf + first + 8) | (uint16_t)0x2020U) ==
                   fio_buf2u16u("ve"))
        r |= FIO___HTTP1_CONN_KEEP_ALIVE;
      v.buf += consumed;
      v.len -= consumed;
    }
  }
  return r;
}

/* Returns true if the server must close the connection after the response.
 *
 * HTTP/1.1: only when a request Connection field contains `close`.
 * HTTP/1.0: unless `keep-alive` was requested (RFC 9112 §9.3) - and always
 * for streamed responses, which are delimited by the connection close. */
FIO_SFUNC int fio___http1_should_close(fio_http_s *h) {
  if (!fio___http1_is_legacy(h))
    return fio___http1_connection_options(h) & FIO___HTTP1_CONN_CLOSE;
  if (fio_http_is_streaming(h))
    return 1; /* close delimited, no header scan required */
  return fio___http1_connection_options(h) != FIO___HTTP1_CONN_KEEP_ALIVE;
}

/** called once a request / response had finished */
FIO_SFUNC void fio___http_controller_http1_on_finish(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  {
    size_t last_chunk = (size_t)fio___http1_is_chunked(h);
    if (c->state.http.buf.len) {
      const size_t org = c->state.http.buf.len;
      if (last_chunk && !fio_string_write(&c->state.http.buf,
                                          FIO_STRING_REALLOC,
                                          "0\r\n\r\n",
                                          5))
        last_chunk = 0;
      else
        c->state.http.buf.len = org; /* no truncated last-chunk */
      fio___http1_flush(c);
    }
    if (last_chunk) /* (sent separately if appending failed) */
      fio_io_write2(c->io, .buf = (char *)"0\r\n\r\n", .len = 5, .copy = 1);
  }
  if (c->log)
    fio_http_write_log(h);
  if (fio_http_is_upgraded(h))
    goto upgraded;
  /* once the function returns, `h` may be freed (auto-finish on free).
   * so we must call this callback here (sync), no matter the thread */
  const int close_connection = fio___http1_should_close(h);
  c->state.http.on_finish(h);
  if (FIO_UNLIKELY(!c->io))
    return; /* undispatched handle freed by `on_close` (IO destroyed): no
               completion-time IO reference exists, nothing to balance */
  if (close_connection) {
    fio_io_close(c->io);
    fio_io_free(c->io); /* balance the completion-time fio_io_dup */
    return;
  }
  h = NULL;
upgraded:
  /* the task owns the completion-time IO reference (and an upgraded `h`) */
  fio_io_defer(fio___http_controller_http1_on_finish_task,
               (void *)(c),
               (void *)h);
}

/* *****************************************************************************
HTTP/1.1 Finish
***************************************************************************** */
#endif /* FIO_HTTP */
