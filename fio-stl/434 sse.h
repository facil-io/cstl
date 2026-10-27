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

/** Typed `dealloc` adapter for fio_bstr payloads. */
FIO_SFUNC void fio___sse_bstr_free(void *bstr) { fio_bstr_free((char *)bstr); }

/* fio_bstr writes (fio_string_write) refuse to grow past this length. */
#define FIO___SSE_BSTR_LIMIT 0x7FFFFFFFULL

/** Returns 1 if `s` is a valid single-line field value (no CR, LF or NUL). */
FIO_SFUNC int fio___sse_field_is_valid(fio_buf_info_s s) {
  if (s.len >= FIO___SSE_BSTR_LIMIT || (!s.buf && s.len))
    return 0;
  for (size_t i = 0; i < s.len; ++i) {
    const char ch = s.buf[i];
    if (ch == '\r' || ch == '\n' || !ch)
      return 0;
  }
  return 1;
}

/**
 * Frames an SSE event as a new fio_bstr (caller owns), or returns NULL.
 *
 * `id` / `event` containing CR, LF or NUL are rejected (they would inject
 * fields / events). `data` (required) is split on CR, LF and CRLF (the SSE
 * line ends), one `data:` line per line: receivers rebuild the payload with
 * every line end normalized to LF. Never returns a partially framed event.
 */
FIO_SFUNC char *fio___sse_frame(const fio_http_sse_write_args_s *a) {
  /* bounds are checked before any scan / sum */
  if (!a->data.len || !a->data.buf || a->data.len >= FIO___SSE_BSTR_LIMIT)
    return NULL;
  if (!fio___sse_field_is_valid(a->id) || !fio___sse_field_is_valid(a->event)) {
    FIO_LOG_DEBUG2("SSE write refused: invalid / multi-line `id` or `event`");
    return NULL;
  }
  /* exact length for single-line data (each line end adds 5 or 6 bytes) */
  uint64_t expect = (uint64_t)a->data.len + 9; /* "data:" CRLF + CRLF */
  if (a->id.len)
    expect += 5 + (uint64_t)a->id.len; /* "id:" CRLF */
  if (a->event.len)
    expect += 8 + (uint64_t)a->event.len; /* "event:" CRLF */
  if (expect >= FIO___SSE_BSTR_LIMIT)
    return NULL;
  /* multi-line growth is amortized by fio_string_write (grows >= capa / 4) */
  char *payload = fio_bstr_reserve(NULL, (size_t)expect);
  if (!payload)
    return NULL;
  if (a->id.len)
    payload = fio_bstr_write2(payload,
                              FIO_STRING_WRITE_STR2("id:", 3),
                              FIO_STRING_WRITE_STR2(a->id.buf, a->id.len),
                              FIO_STRING_WRITE_STR2("\r\n", 2));
  if (a->event.len)
    payload = fio_bstr_write2(payload,
                              FIO_STRING_WRITE_STR2("event:", 6),
                              FIO_STRING_WRITE_STR2(a->event.buf, a->event.len),
                              FIO_STRING_WRITE_STR2("\r\n", 2));
  { /* one "data:" line per CR / LF / CRLF terminated line */
    const char *pos = a->data.buf;
    const char *const end = pos + a->data.len;
    /* cached next CR / LF (NULL = none left): each byte is scanned once */
    const char *cr = (const char *)FIO_MEMCHR(pos, '\r', a->data.len);
    const char *lf = (const char *)FIO_MEMCHR(pos, '\n', a->data.len);
    while (cr || lf) {
      const char *const eol = (cr && (!lf || cr < lf)) ? cr : lf;
      const size_t eol_len = 1 + (eol == cr && eol + 1 == lf); /* CRLF */
      expect += 7 - eol_len; /* the line end becomes CRLF + "data:" */
      payload = fio_bstr_write2(payload,
                                FIO_STRING_WRITE_STR2("data:", 5),
                                FIO_STRING_WRITE_STR2(pos, (size_t)(eol - pos)),
                                FIO_STRING_WRITE_STR2("\r\n", 2));
      pos = eol + eol_len;
      if (cr && cr < pos)
        cr = (const char *)FIO_MEMCHR(pos, '\r', (size_t)(end - pos));
      if (lf && lf < pos)
        lf = (const char *)FIO_MEMCHR(pos, '\n', (size_t)(end - pos));
    }
    /* last line (empty after a trailing line end, which it preserves) */
    payload = fio_bstr_write2(payload,
                              FIO_STRING_WRITE_STR2("data:", 5),
                              FIO_STRING_WRITE_STR2(pos, (size_t)(end - pos)),
                              FIO_STRING_WRITE_STR2("\r\n\r\n", 4));
  }
  if (FIO_UNLIKELY(fio_bstr_len(payload) != expect)) {
    /* allocation / length limit failure: never emit a truncated event */
    FIO_LOG_ERROR("SSE write failed: couldn't frame %zu bytes",
                  (size_t)a->data.len);
    fio_bstr_free(payload);
    return NULL;
  }
  return payload;
}

void fio_http_sse_write___(void); /* IDE Marker */
/** Writes an SSE message (UTF-8). Fails if connection wasn't upgraded yet. */
SFUNC int fio_http_sse_write FIO_NOOP(fio_http_s *h,
                                      fio_http_sse_write_args_s args) {
  if (!args.data.len || !h || !fio_http_is_sse(h))
    return -1;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c || !c->io || !fio_io_is_open(c->io))
    return -1;
  char *payload = fio___sse_frame(&args);
  if (!payload)
    return -1;
  fio_io_write2(c->io,
                .buf = payload,
                .len = fio_bstr_len(payload),
                .dealloc = fio___sse_bstr_free);
  return 0 - !fio_io_is_open(c->io);
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
EventSource / SSE Protocol (client side parser)
***************************************************************************** */

/* Event fields are owned fio_bstr copies (`id.buf` / `event.buf` / `data`):
 * the read buffer is compacted and refilled between reads, so views into it
 * would not survive a fragmented event. `id` is the last event ID, which
 * persists across events (as the SSE specification requires). */

/** Frees the per-event state (`event`, `data`); the last event ID persists. */
FIO_IFUNC void fio___sse_event_reset(struct fio___http_connection_sse_s *sse) {
  fio_bstr_free(sse->data);
  fio_bstr_free(sse->event.buf);
  sse->data = NULL;
  sse->event = FIO_BUF_INFO0;
}

/** Frees all incoming state (including the last event ID). */
FIO_IFUNC void fio___sse_state_destroy(
    struct fio___http_connection_sse_s *sse) {
  fio___sse_event_reset(sse);
  fio_bstr_free(sse->id.buf);
  sse->id = FIO_BUF_INFO0;
}

/** Copies a field value into owned storage (reusing its capacity). */
FIO_IFUNC int fio___sse_field_set(fio_buf_info_s *dest,
                                  const char *src,
                                  size_t len) {
  /* on failure, returns the original (unchanged) string */
  char *s = fio_bstr_len_set(dest->buf, len);
  if (FIO_UNLIKELY(!s || fio_bstr_len(s) != len))
    return -1;
  if (len)
    FIO_MEMCPY(s, src, len);
  *dest = FIO_BUF_INFO2(s, len);
  return 0;
}

/** Tests a (case sensitive) field name. */
#define FIO___SSE_FIELD_IS(name, len, lit)                                     \
  ((len) == sizeof(lit) - 1 && !FIO_MEMCMP((name), (lit), sizeof(lit) - 1))

/**
 * Consumes complete lines (ending in CRLF, LF or CR). Returns -1 if the
 * connection was closed.
 */
FIO_SFUNC int fio___sse_consume_data(fio___http_connection_s *c) {
  FIO_LOG_DEBUG2("SSE data processing:\n%.*s", (int)c->len, c->buf);
  struct fio___http_connection_sse_s *sse = &c->state.sse;
  const char *pos = c->buf;
  const char *const stop = c->buf + c->len;
  if (sse->skip_lf && pos < stop) { /* a CRLF split across reads */
    pos += (pos[0] == '\n');
    sse->skip_lf = 0;
  }
  /* cached next CR / LF (NULL = none left): each byte is scanned once */
  const char *cr = (const char *)FIO_MEMCHR(pos, '\r', (size_t)(stop - pos));
  const char *lf = (const char *)FIO_MEMCHR(pos, '\n', (size_t)(stop - pos));
  for (;;) {
    const char *const eol = (cr && (!lf || cr < lf)) ? cr : lf;
    if (!eol)
      break;
    const char *const line = pos;
    pos = eol + 1 + (eol == cr && eol + 1 == lf);
    /* a CR ending the buffer is a line end; an LF next to it is not a line */
    sse->skip_lf = (eol == cr && eol + 1 == stop);
    if (cr && cr < pos)
      cr = (const char *)FIO_MEMCHR(pos, '\r', (size_t)(stop - pos));
    if (lf && lf < pos)
      lf = (const char *)FIO_MEMCHR(pos, '\n', (size_t)(stop - pos));

    if (eol == line) { /* empty line - dispatch (only events with data) */
      if (sse->data)
        sse->on_message(c->h, sse->id, sse->event, fio_bstr_buf(sse->data));
      fio___sse_event_reset(sse);
      continue;
    }
    if (line[0] == ':') /* comment */
      continue;
    /* field name ends at the first ':' (none: the line, with no value) */
    const char *value =
        (const char *)FIO_MEMCHR(line, ':', (size_t)(eol - line));
    const size_t name_len = (size_t)((value ? value : eol) - line);
    if (value) {
      ++value;
      value += (value < eol && value[0] == ' '); /* one leading space */
    } else {
      value = eol;
    }
    const size_t len = (size_t)(eol - value);

    if (FIO___SSE_FIELD_IS(line, name_len, "data")) {
      const size_t sep = !!sse->data; /* LF joins `data` lines */
      const uint64_t expect = (uint64_t)fio_bstr_len(sse->data) + sep + len;
      if (expect >= FIO___SSE_BSTR_LIMIT ||
          (c->settings->ws_max_msg_size &&
           expect > (uint64_t)c->settings->ws_max_msg_size))
        goto breach;
      /* an empty `data` line is data (an empty line / LF), not absent */
      if (!sse->data && !(sse->data = fio_bstr_reserve(NULL, len)))
        goto oom;
      sse->data = fio_bstr_write2(sse->data,
                                  FIO_STRING_WRITE_STR2("\n", sep),
                                  FIO_STRING_WRITE_STR2(value, len));
      if (FIO_UNLIKELY(fio_bstr_len(sse->data) != expect))
        goto oom;
    } else if (FIO___SSE_FIELD_IS(line, name_len, "event")) {
      if (fio___sse_field_set(&sse->event, value, len))
        goto oom;
    } else if (FIO___SSE_FIELD_IS(line, name_len, "id")) {
      /* an ID containing NUL is ignored (SSE specification) */
      if (!FIO_MEMCHR(value, 0, len) &&
          fio___sse_field_set(&sse->id, value, len))
        goto oom;
    } /* other fields (`retry`, unknown) are ignored */
  }
  FIO_ASSERT(pos <= stop, "overflow on next line read");
  c->len -= (uint32_t)(pos - c->buf);
  if (c->len)
    FIO_MEMMOVE(c->buf, pos, c->len);
  return 0;

breach:
  FIO_LOG_SECURITY("SSE incoming data payload too large!");
  goto close_connection;
oom:
  FIO_LOG_ERROR("SSE incoming event dropped: allocation failed");
close_connection: /* never deliver a partial event, drop unparsed input */
  fio___sse_state_destroy(sse);
  c->len = 0;
  fio_io_close(c->io);
  return -1;
}
#undef FIO___SSE_FIELD_IS

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
    c->len += (uint32_t)r;
    if (fio___sse_consume_data(c))
      return;
  }
error:
  FIO_LOG_ERROR("Incoming SSE data too long (HTTP line limit set at %zu)!",
                (size_t)c->capa);
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
                  (size_t)c->len);
  if (c->len && c->is_client)
    fio___sse_consume_data(c);
}

FIO_SFUNC void fio___sse_on_timeout(fio_io_s *io) {
  char buf[32] = ":ping 0x0000000000000000\r\n\r\n";
  fio_ltoa16u(buf + 8, (uint64_t)fio_io_last_tick(), 16);
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
  FIO_LOG_DDEBUG2("(%d) SSE connection closed for %p",
                  fio_io_pid(),
                  (void *)c->io);
  c->io = NULL;
  fio___sse_state_destroy(&c->state.sse);
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
  const int has_fd = !args.buf && (unsigned)(args.fd + 1) > 1;
  if (!args.len)
    goto cleanup;
  if (args.buf) {
    /* The payload starts at `buf + offset` (public write contract). Reject
     * "negative" (wrapped) offsets and address-space overflow; the caller
     * owns the allocation bounds, which can't be known here. */
    const uintptr_t start = (uintptr_t)args.buf;
    if (args.offset > (size_t)PTRDIFF_MAX ||
        args.offset > UINTPTR_MAX - start ||
        args.len > UINTPTR_MAX - start - args.offset ||
        fio_http_sse_write(
            h,
            .data = FIO_BUF_INFO2((char *)args.buf + args.offset, args.len)))
      goto write_error;
  } else if (has_fd) { /* SSE frames text lines: read the file, then frame */
    char *tmp = NULL;
    if (args.len < FIO___SSE_BSTR_LIMIT && args.len <= SIZE_MAX - args.offset &&
        (off_t)args.offset >= 0 &&
        (uintmax_t)(off_t)args.offset == (uintmax_t)args.offset &&
        (off_t)(args.offset + args.len) >= 0 &&
        (uintmax_t)(off_t)(args.offset + args.len) ==
            (uintmax_t)(args.offset + args.len))
      tmp = fio_bstr_reserve(NULL, args.len);
    if (!tmp ||
        fio_fd_read(args.fd, tmp, args.len, (off_t)args.offset) != args.len ||
        fio_http_sse_write(h, .data = FIO_BUF_INFO2(tmp, args.len))) {
      fio_bstr_free(tmp);
      goto write_error;
    }
    fio_bstr_free(tmp);
  }
cleanup:
  if (args.dealloc && args.buf)
    args.dealloc((void *)args.buf); /* the original pointer, not `+ offset` */
  if (has_fd && !args.copy)
    close(args.fd);
  return;
write_error:
  FIO_LOG_DEBUG2("SSE generic write: couldn't send %zu bytes", args.len);
  goto cleanup;
}

/* *****************************************************************************
EventSource / SSE Finish
***************************************************************************** */
#undef FIO___SSE_BSTR_LIMIT
#endif /* FIO_HTTP */
