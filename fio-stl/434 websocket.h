/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************

    WebSocket - Upgrade Authorization, Events, Protocol, Write, Controller

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP) && !defined(FIO___RECURSIVE_INCLUDE) &&                  \
    !defined(H___FIO_WEBSOCKET___H) &&                                         \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
#define H___FIO_WEBSOCKET___H

/* *****************************************************************************
HTTP Request handling / handling (WebSocket upgrade authorization)
***************************************************************************** */

#define FIO___HTTP_WS_DEFLATE_NEGOTIATE_SEAM 1
/**
 * Builds the Sec-WebSocket-Extensions response for a permessage-deflate
 * offer. ALWAYS forces `server_no_context_takeover` +
 * `client_no_context_takeover` (RFC 7692 §7.1.1 allows either endpoint to
 * request them unilaterally; persistent per-connection compression state is
 * ~0 by design). Honors `server_max_window_bits` when offered (8..15,
 * recorded into `*server_bits` so the compressor can clamp its distances);
 * NEVER emits window-bits parameters (`client_max_window_bits` is pointless
 * under no-takeover, and `server_max_window_bits` may only be answered <=
 * the offer — our inflater accepts any in-message distance <= 32KB anyway).
 * Returns the response length, or 0 when `out_cap` is too small.
 */
FIO_SFUNC size_t fio___http_ws_deflate_negotiate(fio_str_info_s offer,
                                                 char *out,
                                                 size_t out_cap,
                                                 int *server_bits) {
  static const char resp[] = "permessage-deflate; server_no_context_takeover"
                             "; client_no_context_takeover";
  int bits = 15;
  const char *pos = offer.buf;
  const char *end = offer.buf + offer.len;
  while (pos < end) {
    while (pos < end && (*pos == ' ' || *pos == '\t' || *pos == ';'))
      ++pos;
    const char *name = pos;
    while (pos < end && *pos != ';' && *pos != '=')
      ++pos;
    const char *name_end = pos;
    while (name_end > name && (name_end[-1] == ' ' || name_end[-1] == '\t'))
      --name_end;
    if ((size_t)(name_end - name) == 22 &&
        !FIO_MEMCMP(name, "server_max_window_bits", 22) && pos < end &&
        *pos == '=') {
      ++pos;
      int v = 0;
      int digits = 0;
      while (pos < end && *pos >= '0' && *pos <= '9') {
        v = v * 10 + (*pos - '0');
        ++pos;
        ++digits;
      }
      if (digits && v >= 8 && v <= 15)
        bits = v; /* honored: compressor clamps distances <= 2^bits */
    }
    while (pos < end && *pos != ';')
      ++pos;
  }
  if (out_cap < sizeof(resp) - 1)
    return 0;
  FIO_MEMCPY(out, resp, sizeof(resp) - 1);
  if (server_bits)
    *server_bits = bits;
  return sizeof(resp) - 1;
}

FIO_SFUNC void fio___http_perform_user_upgrade_callback_websocket(void *cb_,
                                                                  void *h_) {
  union {
    int (*fn)(fio_http_s *);
    void *ptr;
  } cb = {.ptr = cb_};
  fio_http_s *h = (fio_http_s *)h_;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  struct fio___http_connection_http_s old = c->state.http;
  if (!FIO_LIKELY(fio_io_is_open(c->io)) || cb.fn(h))
    goto refuse_upgrade;
  if (c->h) /* request after WebSocket Upgrade? an attack vector? */
    goto refuse_upgrade;
  /* RFC 7692: permessage-deflate extension negotiation (no-takeover only) */
  if (FIO_LIKELY(fio_http_cflags_is_set(h, FIO_HTTP_CFLAG_COMPRESS_WS))) {
    FIO_HTTP_HEADER_EACH_VALUE(
        h,
        1,
        FIO_STR_INFO2((char *)"sec-websocket-extensions", 24),
        val) {
      FIO_LOG_DDEBUG2("WebSocket extension requested: %.*s",
                      (int)val.len,
                      val.buf);
      if (!FIO_STR_INFO_IS_EQ(val,
                              FIO_STR_INFO2((char *)"permessage-deflate", 18)))
        continue;
      /* Negotiate: ALWAYS force both no_context_takeover flags (persistent
       * compression state stays ~0 per connection); honor
       * server_max_window_bits when offered. */
      char ext_resp[80];
      int server_bits = 15;
      size_t ext_len = fio___http_ws_deflate_negotiate(val,
                                                       ext_resp,
                                                       sizeof(ext_resp),
                                                       &server_bits);
      if (!ext_len)
        continue;
      fio_http_response_header_set(
          h,
          FIO_STR_INFO2((char *)"sec-websocket-extensions", 24),
          FIO_STR_INFO2(ext_resp, ext_len));
      /* Create deflate streaming contexts (stored at connection level,
       * outside the state union, so they survive the HTTP→WS transition).
       * Compressor = our writes (server side), Decompressor = client's data.
       * Both are always reset per message (no-takeover), which keeps
       * persistent per-connection state ≈ 0. */
      c->deflate_wr = fio_deflate_new(1, 1);
      c->deflate_rd = fio_deflate_new(1, 0);
      fio_deflate_window_bits_set(c->deflate_wr, server_bits);
      c->deflate_wr_reset = 1; /* server_no_context_takeover (always) */
      c->deflate_rd_reset = 1; /* client_no_context_takeover (always) */
      FIO_LOG_DDEBUG2("WebSocket permessage-deflate negotiated "
                      "(no-context-takeover, server_max_window_bits=%d)",
                      server_bits);
      break;
    }
  }
  fio_http_upgrade_websocket(h);
  return;

refuse_upgrade:
  c->state.http = old;
  if (fio_http_send_error_response(h, 403))
    fio_io_free(c->io);
  fio_http_free(h);
}

/* *****************************************************************************
WebSocket Event Handling (`fio_websocket_*`)
***************************************************************************** */

FIO_SFUNC int fio___websocket_process_data(fio_io_s *io,
                                           fio___http_connection_s *c);

/* permessage-deflate output scratch buffer (multiple free paths). */
FIO_LEAK_COUNTER_DEF(fio___websocket_deflate_buf)

/** Resumes parsing after async message delivery and reads pipelined bytes. */
FIO_SFUNC void fio___websocket_on_message_finalize(void *c_, void *ignr_) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  c->suspend = 0;
  if (c->len)
    fio___websocket_process_data(c->io, c);
  if (!c->suspend)
    fio_io_unsuspend(c->io);
  fio_io_free(c->io);
  fio___http_connection_free(c);
  (void)ignr_;
}

FIO_IFUNC void fio___websocket_on_msg_task(fio___http_connection_s *c,
                                           uint8_t is_text) {
  fio_buf_info_s msg = fio_bstr_buf(c->state.ws.msg);
  c->state.ws.on_message(c->h, msg, is_text);
  /* `fio_bstr_free(NULL)` is a no-op. */
  fio_bstr_free(c->state.ws.msg);
  c->state.ws.msg = NULL;
  fio_io_defer(fio___websocket_on_message_finalize, c, NULL);
}

/** Delivers a complete text message to the user then schedules finalize. */
FIO_SFUNC void fio___websocket_on_text_task(void *c_, void *ignr_) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  fio___websocket_on_msg_task(c, 1);
  (void)ignr_;
}

/** Delivers a complete binary message to the user then schedules finalize. */
FIO_SFUNC void fio___websocket_on_binary_task(void *c_, void *ignr_) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  fio___websocket_on_msg_task(c, 0);
  (void)ignr_;
}

FIO_SFUNC int fio___websocket_protocol_error(fio___http_connection_s *c,
                                             uint16_t code,
                                             const char *reason) {
  char buf[32];
  FIO_LOG_DDEBUG2("WebSocket protocol error %u: %s",
                  code,
                  reason ? reason : "");
  (void)reason;
  size_t len =
      c->is_client
          ? fio_websocket_write_close_client(buf, code, (fio_buf_info_s){0}, 0)
          : fio_websocket_write_close_server(buf, code, (fio_buf_info_s){0});
  fio_io_write(c->io, buf, len);
  c->state.ws.code = code;
  fio_io_close(c->io);
  return -1;
}

/** permessage-deflate decompression (RFC 7692 §7.2.2). */
FIO_SFUNC uint16_t fio___websocket_deflate_transform(fio___http_connection_s *c,
                                                     fio_buf_info_s *msg) {
  if (!c->deflate_rd)
    return FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR;
  if (!msg->len) {
    if (c->deflate_rd_reset)
      fio_deflate_destroy(c->deflate_rd);
    return 0;
  }
  const size_t min_cap = 4U * 1024U;
  const size_t ws_max =
      c->settings->ws_max_msg_size ? c->settings->ws_max_msg_size : (size_t)-1;
  size_t clamped = (msg->len < ws_max) ? msg->len : ws_max;
  if (clamped > ((size_t)-1) >> 2)
    clamped = ((size_t)-1) >> 2;
  size_t out_cap = clamped << 2;
  if (out_cap < min_cap)
    out_cap = min_cap;
  if (out_cap > ws_max)
    out_cap = ws_max;
  char *out = fio_bstr_reserve(NULL, out_cap);
  size_t r =
      fio_deflate_push(c->deflate_rd, out, out_cap, msg->buf, msg->len, 1);
  if (!r) {
    FIO_LOG_ERROR("WebSocket permessage-deflate: corrupt compressed data");
    fio_bstr_free(out);
    return FIO_WEBSOCKET_CLOSE_INVALID_PAYLOAD;
  }
  while (r > out_cap) {
    fio_bstr_free(out);
    out_cap = r;
    if (out_cap > ws_max) {
      FIO_LOG_ERROR("WebSocket permessage-deflate: inflated size exceeds "
                    "ws_max_msg_size %zu — dropping (Message Too Big)",
                    ws_max);
      return FIO_WEBSOCKET_CLOSE_MESSAGE_TOO_BIG;
    }
    out = fio_bstr_reserve(NULL, out_cap);
    r = fio_deflate_push(c->deflate_rd, out, out_cap, NULL, 0, 1);
    if (!r) {
      FIO_LOG_ERROR("WebSocket permessage-deflate: corrupt compressed data");
      fio_bstr_free(out);
      return FIO_WEBSOCKET_CLOSE_INVALID_PAYLOAD;
    }
  }
  out = fio_bstr_len_set(out, r);
  fio_bstr_free(c->state.ws.msg);
  c->state.ws.msg = out;
  *msg = fio_bstr_buf(out);
  if (c->deflate_rd_reset)
    fio_deflate_destroy(c->deflate_rd);
  return 0;
}

/** peer ping. RFC §5.5.2 requires echoing the payload. */
FIO_SFUNC void fio___websocket_on_ping(fio___http_connection_s *c,
                                       fio_buf_info_s msg) {
  char buf[140];
  size_t len = c->is_client ? fio_websocket_write_pong_client(buf, msg, 0)
                            : fio_websocket_write_pong_server(buf, msg);
  fio_io_write2(c->io, .buf = buf, .len = len, .copy = 1);
}

/** peer pong. */
FIO_SFUNC void fio___websocket_on_pong(fio___http_connection_s *c,
                                       fio_buf_info_s msg) {
#if (defined(DEBUG) && DEBUG) ||                                               \
    (defined(FIO_WEBSOCKET_STATS) && FIO_WEBSOCKET_STATS)
  {
    char *pos = msg.buf;
    static uint64_t longest = 0;
    uint64_t ping_time = fio_io_last_tick() - fio_atol16u(&pos);
    if (ping_time < (1 << 16) && longest < ping_time) {
      longest = ping_time;
      FIO_LOG_INFO("WebSocket longest ping round-trip detected as: %zums",
                   (size_t)ping_time);
    }
  }
#endif
  FIO_LOG_DDEBUG2("Pong (%zu): %s", msg.len, msg.buf);
  (void)c;
  (void)msg;
}

/** peer close. */
FIO_SFUNC void fio___websocket_on_close_message(fio___http_connection_s *c,
                                                uint16_t code,
                                                fio_buf_info_s reason) {
  char buf[32];
  size_t len =
      c->is_client
          ? fio_websocket_write_close_client(buf, code, (fio_buf_info_s){0}, 0)
          : fio_websocket_write_close_server(buf, code, (fio_buf_info_s){0});
  fio_io_write(c->io, buf, len);
  c->state.ws.code = code;
  fio_io_close(c->io);
  if (reason.len)
    FIO_LOG_DDEBUG2("WebSocket %p closed with reason: %.*s",
                    c->io,
                    (int)reason.len,
                    reason.buf);
}

/* *****************************************************************************
WebSocket Protocol
***************************************************************************** */

/** Feeds `c->buf` to the pure parser and handles events in the HTTP layer. */
FIO_SFUNC int fio___websocket_process_data(fio_io_s *io,
                                           fio___http_connection_s *c) {
  (void)io;
  for (;;) {
    fio_websocket_event_s ev = {0};
    const uint8_t state_before = c->state.ws.parser.state;
    const size_t consumed = fio_websocket_parse(&c->state.ws.parser,
                                                FIO_BUF_INFO2(c->buf, c->len),
                                                &ev);
    if (consumed == FIO_WEBSOCKET_PARSE_ERROR)
      return fio___websocket_protocol_error(
          c,
          ev.close_code ? ev.close_code : c->state.ws.parser.close_code,
          "parser error");
    if (!c->is_client && state_before == FIO_WEBSOCKET_STATE_HEADER &&
        consumed && !FIO_WEBSOCKET_GET_MASKED(&c->state.ws.parser))
      return fio___websocket_protocol_error(
          c,
          FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
          "client-to-server frame must be masked");
    switch (ev.type) {
    case FIO_WEBSOCKET_EV_NONE:
      if (!consumed)
        return -1;
      c->len -= consumed;
      if (c->len)
        FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
      return -1;
    case FIO_WEBSOCKET_EV_CONTROL:
      switch (ev.opcode) {
      case FIO_WEBSOCKET_OP_PING: fio___websocket_on_ping(c, ev.payload); break;
      case FIO_WEBSOCKET_OP_PONG: fio___websocket_on_pong(c, ev.payload); break;
      case FIO_WEBSOCKET_OP_CLOSE:
        fio___websocket_on_close_message(
            c,
            ev.close_code,
            (ev.payload.len > 2)
                ? FIO_BUF_INFO2(ev.payload.buf + 2, ev.payload.len - 2)
                : FIO_BUF_INFO0);
        c->len -= consumed;
        if (c->len)
          FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
        return -1;
      }
      c->len -= consumed;
      if (c->len)
        FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
      if (!c->len)
        return 0;
      continue;
    case FIO_WEBSOCKET_EV_DATA_CHUNK: {
      const size_t ws_max = c->settings->ws_max_msg_size;
      if (ev.is_first) {
        fio_bstr_free(c->state.ws.msg);
        c->state.ws.msg = NULL;
      }
      if (ws_max) {
        const size_t existing = fio_bstr_len(c->state.ws.msg);
        if (existing > ws_max || ev.payload.len > (ws_max - existing))
          return fio___websocket_protocol_error(
              c,
              FIO_WEBSOCKET_CLOSE_MESSAGE_TOO_BIG,
              "message exceeds ws_max_msg_size");
      }
      if (ev.payload.len || c->state.ws.msg) {
        c->state.ws.msg =
            fio_bstr_write(c->state.ws.msg, ev.payload.buf, ev.payload.len);
      } else if (ev.is_last) {
        c->state.ws.msg = fio_bstr_reserve(c->state.ws.msg, 0);
      }
      if (!ev.is_last) {
        c->len -= consumed;
        if (c->len)
          FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
        if (!c->len)
          return 0;
        continue;
      }
      fio_buf_info_s msg = fio_bstr_buf(c->state.ws.msg);
      if (ev.rsv) {
        if (ev.rsv != FIO_WEBSOCKET_RSV1)
          return fio___websocket_protocol_error(
              c,
              FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
              "unexpected RSV bits");
        {
          const uint16_t code = fio___websocket_deflate_transform(c, &msg);
          if (code)
            return fio___websocket_protocol_error(
                c,
                code,
                "permessage-deflate decode failure");
        }
      }
      if (ws_max && msg.len > ws_max)
        return fio___websocket_protocol_error(
            c,
            FIO_WEBSOCKET_CLOSE_MESSAGE_TOO_BIG,
            "message exceeds ws_max_msg_size");
      c->len -= consumed;
      if (c->len)
        FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
      fio_io_dup(c->io);
      fio___http_connection_dup(c);
      fio_io_suspend(c->io);
      c->suspend = 1;
      fio_queue_push(c->queue,
                     ev.is_text ? fio___websocket_on_text_task
                                : fio___websocket_on_binary_task,
                     c,
                     NULL);
      return -1;
    }
    case FIO_WEBSOCKET_EV_MESSAGE_END:
      c->len -= consumed;
      if (c->len)
        FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
      if (!c->len)
        return 0;
      continue;
    case FIO_WEBSOCKET_EV_ERROR:
      return fio___websocket_protocol_error(
          c,
          ev.close_code ? ev.close_code : FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
          "parser error");
    }
  }
}

/** Called when a data is available. */
FIO_SFUNC void fio___websocket_on_data(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  size_t r;
  for (;;) {
    if (c->capa == c->len)
      return;
    if (!(r = fio_io_read(io, c->buf + c->len, c->capa - c->len)))
      return;
    c->len += r;
    if (fio___websocket_process_data(io, c))
      return;
  }
}

FIO_SFUNC void fio___websocket_on_ready(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  fio_http_s *h = c->h;
  if (!h)
    return;
  c->state.ws.on_ready(h);
}

FIO_SFUNC void fio___websocket_on_timeout(fio_io_s *io) {
  char buf[32];
  char tm[20] = "0x00000000000000000";
  fio_ltoa16u(tm + 2, fio_io_last_tick(), 16);
  size_t len = fio_websocket_write_ping_server(buf, FIO_BUF_INFO2(tm, 18));
  fio_io_write(io, buf, len);
}

FIO_SFUNC void fio___websocket_on_shutdown(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  c->settings->on_shutdown(c->h);
  /* Send our own close frame (code 1001 "going away") and close the IO. */
  char buf[32];
  size_t len =
      c->is_client
          ? fio_websocket_write_close_client(buf,
                                             FIO_WEBSOCKET_CLOSE_GOING_AWAY,
                                             (fio_buf_info_s){0},
                                             0)
          : fio_websocket_write_close_server(buf,
                                             FIO_WEBSOCKET_CLOSE_GOING_AWAY,
                                             (fio_buf_info_s){0});
  fio_io_write(c->io, buf, len);
  c->state.ws.code = FIO_WEBSOCKET_CLOSE_GOING_AWAY;
  fio_io_close(c->io);
}

/** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___websocket_on_attach(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  fio_http_s *h = c->h;
  c->state.ws = (struct fio___http_connection_ws_s){
      .on_message = c->settings->on_message,
      .on_ready = c->settings->on_ready,
  };
  fio_websocket_init(&c->state.ws.parser);
  c->settings->on_open(h);
  fio___websocket_process_data(io, c);
}

/** Called after the connection was closed, and pending tasks completed. */
FIO_SFUNC void fio___websocket_on_close(void *buf, void *udata) {
  FIO_LOG_DDEBUG2("(%d) WebSocket connection closed for %p",
                  (int)fio_thread_getpid(),
                  udata);
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  c->io = NULL;
  fio_bstr_free(c->state.ws.msg);
  /* Free permessage-deflate contexts */
  if (c->deflate_rd) {
    fio_deflate_free(c->deflate_rd);
    c->deflate_rd = NULL;
  }
  if (c->deflate_wr) {
    fio_deflate_free(c->deflate_wr);
    c->deflate_wr = NULL;
  }
  if (c->h) {
    fio_http_status_set(c->h, (size_t)(c->state.ws.code));
    c->settings->on_close(c->h);
    c->settings->on_finish(c->h);
    fio_http_free(c->h);
  }
  fio___http_connection_free(c);
  (void)buf;
}

/**
 * Sets a specific on_message callback for this connection.
 *
 * Returns -1 on error (i.e., upgrade still in negotiation).
 */
SFUNC int fio_http_on_message_set(fio_http_s *h,
                                  void (*on_message)(fio_http_s *,
                                                     fio_buf_info_s,
                                                     uint8_t)) {
  if (!h)
    return -1;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c)
    return -1;
  if (!on_message)
    on_message = c->settings->on_message;
  c->state.ws.on_message = on_message;
  return 0;
}

/* *****************************************************************************
WebSocket Writing / Subscription Helpers
***************************************************************************** */

FIO_IFUNC void fio___http_websocket_subscribe_imp(fio_pubsub_msg_s *msg,
                                                  uint8_t is_text) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(msg->io);
  if (!c)
    return;
  fio_http_websocket_write(c->h, msg->message.buf, msg->message.len, is_text);
}

/** Optional WebSocket subscription callback - all messages are UTF-8 valid. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT(fio_pubsub_msg_s *msg) {
  FIO_LOG_DEBUG2("forwarding pubsub text message to Websocket: %s (%zu bytes)",
                 (char *)msg->message.buf,
                 msg->message.len);
  fio___http_websocket_subscribe_imp(msg, 1);
}
/** Optional WebSocket subscription callback - messages may be non-UTF-8. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY(fio_pubsub_msg_s *msg) {
  FIO_LOG_DEBUG2("forwarding pubsub binary message to Websocket (%zu bytes)",
                 msg->message.len);
  fio___http_websocket_subscribe_imp(msg, 0);
}

/** Optional WebSocket subscription callback. */
SFUNC void FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT(fio_pubsub_msg_s *msg) {
  ((msg->message.len < FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT) &&
           (fio_string_utf8_valid(
               FIO_STR_INFO2((char *)msg->message.buf, msg->message.len)))
       ? FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_TEXT
       : FIO_HTTP_WEBSOCKET_SUBSCRIBE_DIRECT_BINARY)(msg);
}

/* *****************************************************************************
WebSocket Write (`fio_http_websocket_write`)
***************************************************************************** */

SFUNC int fio_http_websocket_write(fio_http_s *h,
                                   const void *buf,
                                   size_t len,
                                   uint8_t is_text) {
  if (!h || !fio_http_is_websocket(h))
    return -1;
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c)
    return -1;
  const _Bool text_flag = !!is_text;
  uint8_t rsv = 0;

  /* RFC 7692: compress with permessage-deflate if negotiated. */
  const void *send_buf = buf;
  size_t send_len = len;
  char *comp_buf = NULL;
  size_t comp_alloc = 0;
  if (c->deflate_wr && len >= FIO_HTTP_WEBSOCKET_DEFLATE_MIN) {
    /* Output bound: input + 12.5% + 32B overhead. */
    comp_alloc = len + (len >> 3) + 32;
    comp_buf = (char *)FIO_MEM_REALLOC(NULL, 0, comp_alloc, 0);
    if (comp_buf)
      FIO_LEAK_COUNTER_ON_ALLOC(fio___websocket_deflate_buf);
    if (comp_buf) {
      size_t comp_len =
          fio_deflate_push(c->deflate_wr, comp_buf, comp_alloc, buf, len, 1);
      if (comp_len >= 4) {
        /* RFC 7692 §7.2.1: strip trailing 00 00 FF FF sync marker. */
        const uint8_t *tail = (const uint8_t *)comp_buf + comp_len - 4;
        if (tail[0] == 0x00 && tail[1] == 0x00 && tail[2] == 0xFF &&
            tail[3] == 0xFF) {
          comp_len -= 4;
        }
        if (comp_len >= len) {
          /* Negative gain: compression expanded the payload — send the
           * original uncompressed (RSV1 stays clear) instead. */
          FIO_LEAK_COUNTER_ON_FREE(fio___websocket_deflate_buf);
          FIO_MEM_FREE(comp_buf, comp_alloc);
          comp_buf = NULL;
          comp_alloc = 0;
        } else {
          send_buf = comp_buf;
          send_len = comp_len;
          /* RSV1 (byte-0 bit 6 = 0x40) marks compressed; the write API
           * takes the 3-bit rsv value shifted into 4..6, so RSV1 = 0x4
           * (NOT 0x1 — that would set RSV3 and every RFC-compliant peer
           * closes with protocol error 1002 on an unnegotiated RSV). */
          rsv = FIO_WEBSOCKET_RSV1;
        }
      } else {
        /* Compression failed — fall back to uncompressed. */
        FIO_LEAK_COUNTER_ON_FREE(fio___websocket_deflate_buf);
        FIO_MEM_FREE(comp_buf, comp_alloc);
        comp_buf = NULL;
        comp_alloc = 0;
      }
      /* Reset compressor if server_no_context_takeover. */
      if (c->deflate_wr_reset)
        fio_deflate_destroy(c->deflate_wr);
    }
  }

  const fio_buf_info_s msg =
      FIO_BUF_INFO2((char *)(uintptr_t)send_buf, send_len);

  if (send_len < 512) {
    /* Small message → stack buffer. */
    char tmp[520];
    size_t wlen =
        c->is_client
            ? fio_websocket_write_message_client(tmp, msg, text_flag, 0, rsv)
            : fio_websocket_write_message_server(tmp, msg, text_flag, rsv);
    if (comp_buf) {
      FIO_LEAK_COUNTER_ON_FREE(fio___websocket_deflate_buf);
      FIO_MEM_FREE(comp_buf, comp_alloc);
    }
    fio_io_write2(c->io, .buf = tmp, .len = wlen, .copy = 1);
    return 0;
  }

  char *payload =
      fio_bstr_reserve(NULL, fio_websocket_write_len(send_len, c->is_client));
  payload = fio_bstr_len_set(
      payload,
      c->is_client
          ? fio_websocket_write_message_client(payload, msg, text_flag, 0, rsv)
          : fio_websocket_write_message_server(payload, msg, text_flag, rsv));
  if (comp_buf) {
    FIO_LEAK_COUNTER_ON_FREE(fio___websocket_deflate_buf);
    FIO_MEM_FREE(comp_buf, comp_alloc);
  }
  fio_io_write2(c->io,
                .buf = payload,
                .len = fio_bstr_len(payload),
                .dealloc = (void (*)(void *))fio_bstr_free);
  return 0 - !fio_io_is_open(c->io);
}

/* *****************************************************************************
WebSocket Controller
***************************************************************************** */

/* Called by the HTTP handle for each body chunk (or to finish a response). */
FIO_SFUNC void fio___http_controller_ws_write_body(fio_http_s *h,
                                                   fio_http_write_args_s args) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (args.buf && args.len < FIO_HTTP_WEBSOCKET_WRITE_VALIDITY_TEST_LIMIT) {
    unsigned char is_text =
        !!fio_string_utf8_valid(FIO_STR_INFO2((char *)args.buf, args.len));
    fio_http_websocket_write(h, (void *)args.buf, args.len, is_text);
    if (args.dealloc)
      args.dealloc((void *)args.buf);
    return;
  }
  char header[16];
  ((uint8_t *)header)[0] = 0 | 2 | 128;
  if (args.len < 126) {
    ((uint8_t *)header)[1] = args.len;
    fio_io_write(c->io, header, 2);
  } else if (args.len < (1UL << 16)) {
    /* head is 4 bytes */
    ((uint8_t *)header)[1] = 126 | ((!!c->is_client) << 7);
    fio_u2buf16_be(((uint8_t *)header + 2), args.len);
    fio_io_write(c->io, header, 4);
  } else {
    /* Really Long Message  */
    ((uint8_t *)header)[1] = 127 | ((!!c->is_client) << 7);
    fio_u2buf64_be(((uint8_t *)header + 2), args.len);
    fio_io_write(c->io, header, 10);
  }
  fio_io_write2(c->io,
                .buf = (void *)args.buf,
                .fd = args.fd,
                .len = args.len,
                .offset = args.offset,
                .dealloc = args.dealloc,
                .copy = (uint8_t)args.copy);
}

/* *****************************************************************************
WebSocket Finish
***************************************************************************** */
#endif /* FIO_HTTP */
