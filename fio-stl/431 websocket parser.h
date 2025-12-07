/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_BITWISE            /* Development inclusion - ignore line */
#define FIO_RAND               /* Development inclusion - ignore line */
#define FIO_WEBSOCKET_PARSER   /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                              WebSocket Parser




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_WEBSOCKET_PARSER) && !defined(H___FIO_WEBSOCKET_PARSER___H) && \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
/* *****************************************************************************
The parser provides static functions only, always as part or implementation.
***************************************************************************** */
#define H___FIO_WEBSOCKET_PARSER___H

/* *****************************************************************************
WebSocket Parsing API
***************************************************************************** */

typedef struct fio_websocket_parser_s fio_websocket_parser_s;
/**
 * Parses WebSocket data, calling any callbacks.
 *
 * Returns bytes consumed or `FIO_WEBSOCKET_PARSER_ERROR` (`(size_t)-1`) on
 * error.
 */
FIO_SFUNC size_t fio_websocket_parse(fio_websocket_parser_s *p,
                                     fio_buf_info_s buf,
                                     void *udata);

// FIO_SFUNC

/** The parsers return value on error. */
#define FIO_WEBSOCKET_PARSER_ERROR ((size_t)-1)

/**
 * Maximum allowed WebSocket frame payload length.
 * Default: 1GB (1 << 30). Override before including this header if needed.
 * Setting this helps prevent DoS attacks via memory exhaustion.
 */
#ifndef FIO_WEBSOCKET_MAX_PAYLOAD
#define FIO_WEBSOCKET_MAX_PAYLOAD ((uint64_t)(1ULL << 30))
#endif

/* *****************************************************************************
WebSocket Parsing Callbacks
***************************************************************************** */

/** Called when a message frame was received. */
FIO_SFUNC void fio_websocket_on_message(void *udata,
                                        fio_buf_info_s msg,
                                        unsigned char is_text);

/**
 * Called when the parser needs to copy the message to an external buffer.
 *
 * MUST return the external buffer, as it may need to be unmasked.
 *
 * Partial message length may be equal to zero (`partial.len == 0`).
 */
FIO_SFUNC fio_buf_info_s fio_websocket_write_partial(void *udata,
                                                     fio_buf_info_s partial,
                                                     size_t more_expected);

/** Called when the permessage-deflate extension requires decompression. */
FIO_SFUNC fio_buf_info_s fio_websocket_decompress(void *udata,
                                                  fio_buf_info_s msg);

/** Called when a `ping` message was received. */
FIO_SFUNC void fio_websocket_on_protocol_ping(void *udata, fio_buf_info_s msg);

/** Called when a `pong` message was received. */
FIO_SFUNC void fio_websocket_on_protocol_pong(void *udata, fio_buf_info_s msg);

/** Called when a `close` message was received. */
FIO_SFUNC void fio_websocket_on_protocol_close(void *udata, fio_buf_info_s msg);

/* *****************************************************************************
WebSocket Formatting API
***************************************************************************** */
/**
 * Returns the length of the buffer required to wrap a message `len` long
 *
 * Client connections should add 4 to this number to accommodate for the mask.
 */
FIO_IFUNC uint64_t fio_websocket_wrapped_len(uint64_t len);

/**
 * Wraps a WebSocket server message and writes it to the target buffer.
 *
 * The `first` and `last` flags can be used to support message fragmentation.
 *
 * * target: the target buffer to write to.
 * * msg:    the message to be wrapped.
 * * len:    the message length.
 * * opcode: set to 1 for UTF-8 message, 2 for binary, etc'.
 * * first:  set to 1 if `msg` points the beginning of the message.
 * * last:   set to 1 if `msg + len` ends the message.
 * * client: set to 1 to use client mode (data  masking).
 *
 * Further opcode values:
 * * %x0 denotes a continuation frame
 * *  %x1 denotes a text frame
 * *  %x2 denotes a binary frame
 * *  %x3-7 are reserved for further non-control frames
 * *  %x8 denotes a connection close
 * *  %x9 denotes a ping
 * *  %xA denotes a pong
 * *  %xB-F are reserved for further control frames
 *
 * Returns the number of bytes written. Always `websocket_wrapped_len(len)`
 */
FIO_SFUNC uint64_t fio_websocket_server_wrap(void *target,
                                             const void *msg,
                                             uint64_t len,
                                             unsigned char opcode,
                                             unsigned char first,
                                             unsigned char last,
                                             unsigned char rsv);

/**
 * Wraps a WebSocket client message and writes it to the target buffer.
 *
 * The `first` and `last` flags can be used to support message fragmentation.
 *
 * * target: the target buffer to write to.
 * * msg:    the message to be wrapped.
 * * len:    the message length.
 * * opcode: set to 1 for UTF-8 message, 2 for binary, etc'.
 * * first:  set to 1 if `msg` points the beginning of the message.
 * * last:   set to 1 if `msg + len` ends the message.
 * * client: set to 1 to use client mode (data  masking).
 *
 * Returns the number of bytes written. Always `websocket_wrapped_len(len) + 4`
 */
FIO_SFUNC uint64_t fio_websocket_client_wrap(void *target,
                                             const void *msg,
                                             uint64_t len,
                                             unsigned char opcode,
                                             unsigned char first,
                                             unsigned char last,
                                             unsigned char rsv);

/* *****************************************************************************
API - Parsing (unwrapping)
***************************************************************************** */

/* *****************************************************************************

                                Implementation

***************************************************************************** */

/** returns the length of the buffer required to wrap a message `len` long */
FIO_IFUNC uint64_t fio_websocket_wrapped_len(uint64_t len) {
  return len + 2ULL + ((len > 125) << 1) +
         ((0ULL - (len > ((1UL << 16) - 1))) & 6ULL);
}

/* *****************************************************************************
Message Wrapping
***************************************************************************** */

FIO_IFUNC uint64_t fio_websocket_header(void *target,
                                        uint64_t message_len,
                                        uint32_t mask,
                                        unsigned char opcode,
                                        unsigned char first,
                                        unsigned char last,
                                        unsigned char rsv) {
  ((uint8_t *)target)[0] = 0U |
                           /*fin*/ ((last & 1U) << 7) |
                           /* opcode */ ((16U - !!first) & (opcode & 15U)) |
                           /* rsv */ ((rsv & 7) << 4);
  ((uint8_t *)target)[1] = ((!!mask) << 7U);
  size_t mask_l = ((!!mask) << 2);
  if (message_len < 126) {
    ((uint8_t *)target)[1] |= message_len;
    if (mask)
      fio_u2buf32u(((uint8_t *)target + 2), mask);
    return 2 + mask_l;
  } else if (message_len < (1UL << 16)) {
    /* head is 4 bytes */
    ((uint8_t *)target)[1] |= 126;
    fio_u2buf16_be(((uint8_t *)target + 2), message_len);
    if (mask)
      fio_u2buf32u(((uint8_t *)target + 4), mask);
    return 4 + mask_l;
  } else {
    /* Really Long Message  */
    ((uint8_t *)target)[1] |= 127;
    fio_u2buf64_be(((uint8_t *)target + 2), message_len);
    if (mask)
      fio_u2buf32u(((uint8_t *)target + 10), mask);
    return 10 + mask_l;
  }
}

/**
 * Wraps a WebSocket server message and writes it to the target buffer.
 *
 * The `first` and `last` flags can be used to support message fragmentation.
 *
 * * target: the target buffer to write to.
 * * msg:    the message to be wrapped.
 * * len:    the message length.
 * * opcode: set to 1 for UTF-8 message, 2 for binary, etc'.
 * * first:  set to 1 if `msg` points the beginning of the message.
 * * last:   set to 1 if `msg + len` ends the message.
 * * client: set to 1 to use client mode (data  masking).
 *
 * Further opcode values:
 * * %x0 denotes a continuation frame
 * *  %x1 denotes a text frame
 * *  %x2 denotes a binary frame
 * *  %x3-7 are reserved for further non-control frames
 * *  %x8 denotes a connection close
 * *  %x9 denotes a ping
 * *  %xA denotes a pong
 * *  %xB-F are reserved for further control frames
 *
 * Returns the number of bytes written. Always `websocket_wrapped_len(len)`
 */
FIO_SFUNC uint64_t fio_websocket_server_wrap(void *restrict target,
                                             const void *restrict msg,
                                             uint64_t len,
                                             unsigned char opcode,
                                             unsigned char first,
                                             unsigned char last,
                                             unsigned char rsv) {
  uint64_t r = fio_websocket_header(target, len, 0, opcode, first, last, rsv);
  FIO_MEMCPY(((uint8_t *)target) + r, msg, len);
  r += len;
  return r;
}

/**
 * Wraps a WebSocket client message and writes it to the target buffer.
 *
 * The `first` and `last` flags can be used to support message fragmentation.
 *
 * * target: the target buffer to write to.
 * * msg:    the message to be wrapped.
 * * len:    the message length.
 * * opcode: set to 1 for UTF-8 message, 2 for binary, etc'.
 * * first:  set to 1 if `msg` points the beginning of the message.
 * * last:   set to 1 if `msg + len` ends the message.
 *
 * Returns the number of bytes written. Always `websocket_wrapped_len(len) +
 * 4`
 */
FIO_SFUNC uint64_t fio_websocket_client_wrap(void *restrict target,
                                             const void *restrict msg,
                                             uint64_t len,
                                             unsigned char opcode,
                                             unsigned char first,
                                             unsigned char last,
                                             unsigned char rsv) {
  /* WebSocket masking per RFC 6455 - prevents proxy cache poisoning attacks.
   * PRNG is acceptable here as masking is not for cryptographic security;
   * it's specifically to prevent intermediary interpretation of frame data. */
  uint64_t mask = (fio_rand64() | 0x01020408ULL) & 0xFFFFFFFFULL; /* non-zero */
  mask |= mask << 32;
  uint64_t r = fio_websocket_header(target,
                                    len,
                                    (uint32_t)mask,
                                    opcode,
                                    first,
                                    last,
                                    rsv);
  fio_xmask_cpy((((char *)target) + r), (const char *)msg, len, mask);
  r += len;
  return r;
}

/* *****************************************************************************
WebSocket Parser Type
***************************************************************************** */

/** The WebSocket parser type implementation */
struct fio_websocket_parser_s {
  int (*fn)(fio_websocket_parser_s *, fio_buf_info_s *, void *);
  uint64_t start_at;
  uint64_t expect;
  uint32_t mask;
  uint8_t first;
  uint8_t current;
  uint8_t must_mask;
};

/* *****************************************************************************
Frame Consumption
***************************************************************************** */
FIO_SFUNC int fio___websocket_consume_header(fio_websocket_parser_s *p,
                                             fio_buf_info_s *buf,
                                             void *udata);

FIO_SFUNC int fio___websocket_consume_frame_partial(fio_websocket_parser_s *p,
                                                    fio_buf_info_s *buf,
                                                    void *udata) {
  fio_websocket_write_partial(udata, *buf, (p->expect -= buf->len));
  buf->buf += buf->len;
  buf->len = 0;
  return 1;
}

FIO_SFUNC int fio___websocket_consume_frame_finish(fio_websocket_parser_s *p,
                                                   fio_buf_info_s *buf,
                                                   void *udata) {
  fio_buf_info_s msg = FIO_BUF_INFO2(buf->buf, p->expect);
  buf->buf += p->expect;
  buf->len -= p->expect;
  p->expect = 0;
  msg = fio_websocket_write_partial(udata, msg, 0);
  if (!msg.buf) /* protocol error response from callback */
    return -1;
  fio_xmask(msg.buf + p->start_at,
            msg.len - p->start_at,
            (((uint64_t)p->mask) << 32) | (uint64_t)p->mask);
  p->start_at += msg.len;
  p->fn = fio___websocket_consume_header;
  if (!(p->current & 128)) /* done? if not, consume next frame */
    return 0;
  /* done */
  if (p->first & 64) { /* RSV1 set: decompress */
    msg = fio_websocket_decompress(udata, msg);
    if (!msg.buf)
      return -1;
  }
  size_t cond = (p->first & 15);
  *p = (fio_websocket_parser_s){.fn = fio___websocket_consume_header};
  switch (cond) {
  case 0: return -1;         /* continuation - error? */
  case 1: /* fall through */ /* text / data frame */
  case 2: fio_websocket_on_message(udata, msg, (cond & 1)); return 1;
  case 8: fio_websocket_on_protocol_close(udata, msg); return 1;
  case 9: fio_websocket_on_protocol_ping(udata, msg); return 1;
  case 10: fio_websocket_on_protocol_pong(udata, msg); return 1;
  default:
    FIO_LOG_DDEBUG2("ERROR: WebSocket protocol error - unknown opcode %u\n",
                    (unsigned int)(p->first & 15));
    return -1;
  }
  return 1;
}

FIO_SFUNC int fio___websocket_consume_frame(fio_websocket_parser_s *p,
                                            fio_buf_info_s *buf,
                                            void *udata) {
  return (p->expect > buf->len
              ? fio___websocket_consume_frame_partial
              : fio___websocket_consume_frame_finish)(p, buf, udata);
}

/* *****************************************************************************
Header Consumption
***************************************************************************** */
FIO_SFUNC int fio___websocket_consume_header(fio_websocket_parser_s *p,
                                             fio_buf_info_s *buf,
                                             void *udata) {
  if (buf->len < 2)
    return 1;
  const uint8_t mask_f = (((uint8_t *)buf->buf)[1] >> 7) & 1;
  const uint8_t mask_l = (mask_f << 2);
  const uint8_t info = (uint8_t)(buf->buf[0]);
  uint8_t len_indicator = ((((uint8_t *)buf->buf)[1]) & 127U);
  switch (len_indicator) {
  case 126:
    if (buf->len < 8UL)
      return 1;
    p->expect = fio_buf2u16_be(buf->buf + 2);
    p->mask = (0ULL - mask_f) & fio_buf2u32u(buf->buf + 4);
    buf->buf += 4 + mask_l;
    buf->len -= 4 + mask_l;
    break;

  case 127:
    if (buf->len < 14UL)
      return 1;
    p->expect = fio_buf2u64_be(buf->buf + 2);
    /* RFC 6455: most significant bit MUST be 0, and enforce max payload limit
     * to prevent memory exhaustion attacks */
    if ((p->expect & 0x8000000000000000ULL) ||
        (p->expect > FIO_WEBSOCKET_MAX_PAYLOAD))
      return -1;
    p->mask = (0ULL - mask_f) & fio_buf2u32u(buf->buf + 10);
    buf->buf += 10 + mask_l;
    buf->len -= 10 + mask_l;
    break;

  default:
    if (buf->len < (2ULL + mask_l))
      return 1;
    p->expect = len_indicator;
    p->mask = mask_f ? fio_buf2u32u(buf->buf + 2) : 0;
    buf->buf += 2 + mask_l;
    buf->len -= 2 + mask_l;
    break;
  }
  if (p->first) {
    p->current = info;
    if ((info & 15)) /* continuation frame == 0 ; is it missing? */
      return -1;
  } else {
    p->first = p->current = info;
    p->start_at = 0;
    if (!(info & 15)) /* continuation frame == 0 ; where's the first? */
      return -1;
  }
  if (p->must_mask && !p->mask)
    return -1;
  return (p->fn = fio___websocket_consume_frame)(p, buf, udata);
}
/* *****************************************************************************
Main Parsing Loop
***************************************************************************** */

FIO_SFUNC size_t fio_websocket_parse(fio_websocket_parser_s *p,
                                     fio_buf_info_s buf,
                                     void *udata) {
  int i = 0;
  char *buf_start = buf.buf;
  if (!buf.len)
    return 0;
  if (!p->fn)
    p->fn = fio___websocket_consume_header;
  while (!(i = p->fn(p, &buf, udata)))
    ;
  if (i < 0)
    return FIO_WEBSOCKET_PARSER_ERROR;
  return buf.buf - buf_start;
}

/* *****************************************************************************
Reading the first line
***************************************************************************** */

/* *****************************************************************************
Cleanup
***************************************************************************** */

#undef FIO_WEBSOCKET_PARSER
#endif /* FIO_WEBSOCKET_PARSER && FIO_EXTERN_COMPLETE */
