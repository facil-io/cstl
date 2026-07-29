/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************

   EventSource (SSE) - Upgrade Authorization, Helpers, Protocol, Controller

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP) && !defined(FIO___RECURSIVE_INCLUDE) &&                  \
    !defined(H___FIO_SSE___H) &&                                               \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
#define H___FIO_SSE___H

/* *****************************************************************************
HTTP Request handling / handling (SSE upgrade authorization)
***************************************************************************** */

FIO_SFUNC void fio___http_perform_user_upgrade_callback_sse(void *cb_,
                                                            void *h_) {
  union {
    int (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.ptr = cb_};
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!FIO_LIKELY(fio_io_is_open(c->io)) || cb.fn(h))
    goto refuse_upgrade;
  if (c->h) /* request after eventsource? an attack vector? */
    goto refuse_upgrade;
  fio_http_upgrade_sse(h);
  return;

refuse_upgrade:
  if (fio_http_send_error_response(h, 403))
    fio_io_free(c->io);
  fio_http_free(h);
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
    char *pos;
    while (args.data.len &&
           (pos = (char *)FIO_MEMCHR(args.data.buf, '\n', args.data.len))) {
      const size_t len = (pos + 1) - args.data.buf;
      pos -= (pos > args.data.buf && pos[-1] == '\r');
      payload = fio_bstr_write2(
          payload,
          FIO_STRING_WRITE_STR2("data:", 5),
          FIO_STRING_WRITE_STR2(args.data.buf, (size_t)(pos - args.data.buf)),
          FIO_STRING_WRITE_STR2("\r\n", 2));
      args.data.buf += len;
      args.data.len -= len;
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
  fio_io_write2(c->io,
                .buf = payload,
                .len = fio_bstr_len(payload),
                .dealloc = (void (*)(void *))fio_bstr_free);
  return 0;
}

/** Optional EventSource subscription callback - messages MUST be UTF-8. */
SFUNC void FIO_HTTP_SSE_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(msg->io);
  if (!c)
    return;
  FIO_STR_INFO_TMP_VAR(id_str, 64);
  fio_string_write_hex(&id_str, NULL, msg->id);
  fio_http_sse_write(c->h,
                     .id = FIO_STR2BUF_INFO(id_str),
                     .event = FIO_STR2BUF_INFO(msg->channel),
                     .data = FIO_STR2BUF_INFO(msg->message));
}

/* *****************************************************************************
EventSource / SSE Protocol (TODO!)
***************************************************************************** */

FIO_SFUNC void fio___sse_consume_data(fio___http_connection_s *c) {
  FIO_LOG_DEBUG2("SSE data processing:\n%.*s", (int)c->len, c->buf);
  struct fio___http_connection_sse_s *sse = &c->state.sse;
  const char *next_line = c->buf;
  const char *stop = c->buf + c->len;
  for (; next_line < stop;) {
    char *line = (char *)next_line;
    const char *eol =
        (const char *)FIO_MEMCHR(next_line, '\n', stop - next_line);
    if (!eol)
      break;
    next_line = eol + 1;
    eol -= (eol > c->buf && eol[-1] == '\n');
    eol -= (eol > c->buf && eol[-1] == '\r');
    if (eol == line) { /* empty line, end of input? */
      if (sse->data || sse->event.buf || sse->id.buf) {
        sse->on_message(c->h, sse->id, sse->event, fio_bstr_buf(sse->data));
        fio_bstr_free(sse->data);
        sse->data = NULL;
        sse->event = sse->id = FIO_BUF_INFO0;
      }
      continue;
    }
    if (line[0] == ':') /* comment */
      continue;
    const size_t line_len = (size_t)(eol - line);
    if (line_len > 2 && line[2] == ':') { /* id */
      const char *start = line + 3;
      start += (start[0] == ' ' || start[0] == '\t');
      if ((line[0] |= 32) == 'i' && (line[1] |= 32) == 'd')
        sse->id = FIO_BUF_INFO2((char *)start, (size_t)(eol - start));

    } else if (line_len > 4 && line[4] == ':') { /* data */
      const char *start = line + 5;
      start += (start[0] == ' ' || start[0] == '\t');
      if ((fio_buf2u32u(line) | 0x20202020U) == fio_buf2u32u("data")) {
        if (fio_bstr_len(sse->data) + (size_t)(eol - start) >
            c->settings->ws_max_msg_size)
          goto breach;
        sse->data = fio_bstr_write2(
            sse->data,
            FIO_STRING_WRITE_STR2("\n", ((size_t) !!sse->data)),
            FIO_STRING_WRITE_STR2(start, (size_t)(eol - start)));
      }

    } else if (line_len > 5 && line[5] == ':') { /* event */
      const char *start = line + 6;
      start += (start[0] == ' ' || start[0] == '\t');
      if ((line[0] |= 32) == 'e' &&
          (fio_buf2u32u(line + 1) | 0x20202020U) == fio_buf2u32u("vent"))
        sse->event = FIO_BUF_INFO2((char *)start, (size_t)(eol - start));

    } else if (!FIO_MEMCHR(line, ':', line_len))
      goto error;
  }
  FIO_ASSERT(next_line <= stop, "overflow on next line read");
  if (next_line > stop)
    next_line = stop;
  c->len -= next_line - c->buf;
  if (c->len)
    FIO_MEMMOVE(c->buf, next_line, c->len);
  return;

error:
  FIO_LOG_ERROR("SSE incoming data malformed!");
  FIO_LOG_DEBUG2("data dump:\n%.*s", (int)c->len, c->buf);
  fio_io_close(c->io);
  return;

breach:
  FIO_LOG_SECURITY("SSE incoming data payload too large!");
  fio_io_close(c->io);
}

/** Called when a data is available. */
FIO_SFUNC void fio___sse_on_data(fio_io_s *io) {
  FIO_LOG_DDEBUG2("(%d) Reading SSE data from socket", fio_io_pid());
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  size_t r;
  for (;;) {
    if (c->len + 2 > c->capa)
      goto error;
    if (!(r = fio_io_read(io, c->buf + c->len, c->capa - c->len)))
      return;
    c->len += r;
    fio___sse_consume_data(c);
  }
error:
  FIO_LOG_ERROR("Incoming SSE data too long (HTTP line limit set at %zu)!",
                c->capa);
  fio_io_close(io);
}

/** Called when an IO is attached to a protocol. */
static void fio___sse_on_attach(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  fio_http_s *h = c->h;
  c->state.sse = (struct fio___http_connection_sse_s){
      .on_message = c->settings->on_eventsource,
      .on_ready = c->settings->on_ready,
  };
  c->settings->on_open(h);
  FIO_LOG_DDEBUG2("(%d) SSE attached; buffer length (unread): %zu",
                  fio_io_pid(),
                  c->len);
  if (c->len && c->is_client)
    fio___sse_consume_data(c);
}

FIO_SFUNC void fio___sse_on_timeout(fio_io_s *io) {
  char buf[32] = ":ping 0x0000000000000000\r\n\r\n";
  fio_ltoa16u(buf + 8, fio_io_last_tick(), 16);
  buf[24] = '\r'; /* overwrite written NUL character */
  fio_io_write(io, buf, 28);
}

FIO_SFUNC void fio___sse_on_shutdown(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  c->settings->on_shutdown(c->h);
}

/** Called after the connection was closed, and pending tasks completed. */
FIO_SFUNC void fio___sse_on_close(void *buf, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  FIO_LOG_DDEBUG2("(%d) SSE connection closed for %p", fio_io_pid(), c->io);
  c->io = NULL;
  fio_bstr_free(c->state.sse.data);
  if (c->h) {
    c->settings->on_close(c->h);
    c->settings->on_finish(c->h);
    fio_http_free(c->h);
  }
  fio___http_connection_free(c);
  (void)buf;
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
  if (!args.buf && (unsigned)(args.fd + 1) > 1)
    close(args.fd);
}

/* *****************************************************************************
EventSource / SSE Finish
***************************************************************************** */
#endif /* FIO_HTTP */
