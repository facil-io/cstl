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
WebSocket Parser Settings
***************************************************************************** */
#ifndef WEBSOCKET_CLIENT_MUST_MASK
/** According to the RFC, a client WebSocket MUST mask messages. */
#define WEBSOCKET_CLIENT_MUST_MASK 1
#endif

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

/** The parsers return value on error. */
#define FIO_WEBSOCKET_PARSER_ERROR ((size_t)-1)

/* *****************************************************************************
WebSocket Parsing Callbacks
***************************************************************************** */

typedef struct fio_websocket_frame_s {
  /** message buffer (in received buffer, the parser is non-copy) */
  fio_buf_info_s msg;
  /** opaque user data passed to the `fio_websocket_parse` function */
  void *udata;
  /** This is the beginning of a new message. */
  unsigned char is_start;
  /** This is the finishes the a message. */
  unsigned char is_finish;
  /** The message is in text format. */
  unsigned char is_text;
  /** The extra RSV data attached to the message (extensions). */
  unsigned char rsv;
} fio_websocket_frame_s;

/** Called when a message frame was received. */
FIO_SFUNC void fio_websocket_on_frame(fio_websocket_frame_s frame);

/**
 * Called when the parser needs to copy the message to an external buffer.
 *
 * MUST return the external buffer, as it may need to be unmasked.
 */
FIO_SFUNC fio_buf_info_s fio_websocket_write_partial(fio_buf_info_s partial,
                                                     size_t total_expected);

/** Called when a `ping` message was received. */
FIO_SFUNC void fio_websocket_on_protocol_ping(void *udata,
                                              void *msg,
                                              uint64_t len);

/** Called when a `pong` message was received. */
FIO_SFUNC void fio_websocket_on_protocol_pong(void *udata,
                                              void *msg,
                                              uint64_t len);

/** Called when a `close` message was received. */
FIO_SFUNC void fio_websocket_on_protocol_close_frame(void *udata);
FIO_SFUNC void fio_websocket_on_protocol_error(void *udata);

/* *****************************************************************************
WebSocket Formatting API
***************************************************************************** */
/** returns the length of the buffer required to wrap a message `len` long */
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
                                             void *msg,
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
                                             void *msg,
                                             uint64_t len,
                                             unsigned char opcode,
                                             unsigned char first,
                                             unsigned char last,
                                             unsigned char rsv);

/* *****************************************************************************
API - Parsing (unwrapping)
***************************************************************************** */

/** the returned value for `websocket_buffer_required` */
struct fio_websocket_packet_info_s {
  /** the expected packet length */
  uint64_t packet_length;
  /** a 64 bit extended packet mask value (extended from 32 bits) */
  uint64_t mask;
  /** the packet's "head" size (before the data) */
  uint8_t head_length;
};

/**
 * Returns all known information regarding the upcoming message.
 *
 * @returns a struct fio_websocket_packet_info_s.
 *
 * On protocol error, the `head_length` value is 0 (no valid head detected).
 */
FIO_IFUNC struct fio_websocket_packet_info_s fio_websocket_buffer_peek(
    void *buffer,
    uint64_t len);

/**
 * Consumes the data in the buffer, calling any callbacks required.
 *
 * Returns the remaining data in the existing buffer (can be 0).
 *
 * Notice: if there's any data in the buffer that can't be parsed
 * just yet, `memmove` is used to place the data at the beginning of the buffer.
 */
FIO_IFUNC uint64_t fio_websocket_consume(void *buffer,
                                         uint64_t len,
                                         void *udata,
                                         uint8_t require_masking);

/* *****************************************************************************

                                Implementation

***************************************************************************** */

/** returns the length of the buffer required to wrap a message `len` long */
FIO_IFUNC uint64_t fio_websocket_wrapped_len(uint64_t len) {
  if (len < 126)
    return len + 2;
  if (len < (1UL << 16))
    return len + 4;
  return len + 10;
}

/* *****************************************************************************
Message Wrapping
***************************************************************************** */

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
                                             void *msg,
                                             uint64_t len,
                                             unsigned char opcode,
                                             unsigned char first,
                                             unsigned char last,
                                             unsigned char rsv) {
  ((uint8_t *)target)[0] = 0 |
                           /* opcode */ (((first ? opcode : 0) & 15)) |
                           /* rsv */ ((rsv & 7) << 4) |
                           /*fin*/ ((last & 1) << 7);
  if (len < 126) {
    ((uint8_t *)target)[1] = len;
    FIO_MEMCPY(((uint8_t *)target) + 2, msg, len);
    return len + 2;
  } else if (len < (1UL << 16)) {
    /* head is 4 bytes */
    ((uint8_t *)target)[1] = 126;
    fio_u2buf16(((uint8_t *)target + 2), len);
    FIO_MEMCPY((uint8_t *)target + 4, msg, len);
    return len + 4;
  }
  /* Really Long Message  */
  ((uint8_t *)target)[1] = 127;
  fio_u2buf64(((uint8_t *)target + 2), len);
  FIO_MEMCPY((uint8_t *)target + 10, msg, len);
  return len + 10;
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
FIO_SFUNC uint64_t fio_websocket_client_wrap(void *target,
                                             void *msg,
                                             uint64_t len,
                                             unsigned char opcode,
                                             unsigned char first,
                                             unsigned char last,
                                             unsigned char rsv) {
  uint64_t mask = (fio_rand64() | 0x01020408ULL) & 0xFFFFFFFF; /* safer */
  mask |= mask << 32;
  ((uint8_t *)target)[0] = 0 |
                           /* opcode */ (((first ? opcode : 0) & 15)) |
                           /* rsv */ ((rsv & 7) << 4) |
                           /*fin*/ ((last & 1) << 7);
  if (len < 126) {
    ((uint8_t *)target)[1] = len | 128;
    fio_u2buf32((void *)((uint8_t *)target + 2), (uint32_t)mask);
    FIO_MEMCPY(((uint8_t *)target) + 6, msg, len);
    fio_xmask((char *)target + 6, len, mask);
    return len + 6;
  } else if (len < (1UL << 16)) {
    /* head is 4 bytes */
    ((uint8_t *)target)[1] = 126 | 128;
    fio_u2buf16((void *)((uint8_t *)target + 2), len);
    fio_u2buf32((void *)((uint8_t *)target + 4), (uint32_t)mask);
    FIO_MEMCPY((uint8_t *)target + 8, msg, len);
    fio_xmask((char *)target + 8, len, mask);
    return len + 8;
  }
  /* Really Long Message  */
  ((uint8_t *)target)[1] = 255;
  fio_u2buf64((void *)((uint8_t *)target + 2), len);
  fio_u2buf32((void *)((uint8_t *)target + 10), (uint32_t)mask);
  FIO_MEMCPY((uint8_t *)target + 14, msg, len);
  fio_xmask((char *)target + 14, len, mask);
  return len + 14;
}

/* *****************************************************************************
Message Unwrapping
***************************************************************************** */

/* *****************************************************************************
WebSocket Parser Type
***************************************************************************** */

/** The WebSocket parser type implementation */
struct fio_websocket_parser_s {
  int (*fn)(fio_websocket_parser_s *, fio_buf_info_s *, void *);
  size_t expected;
};

/* *****************************************************************************
Main Parsing Loop
***************************************************************************** */

FIO_SFUNC size_t fio_websocket_parse(fio_websocket_parser_s *p,
                                     fio_buf_info_s buf,
                                     void *udata) {
  //   int i = 0;
  //   char *buf_start = buf.buf;
  //   if (!buf.len)
  //     return 0;
  //   if (!p->fn)
  //     p->fn = fio_http1___start;
  //   while (!(i = p->fn(p, &buf, udata)))
  //     ;
  //   if (i < 0)
  //     return FIO_WEBSOCKET_PARSER_ERROR;
  //   return buf.buf - buf_start;
  // }
  // #define WEBSOCKET___EXPECTED_CHUNKED ((size_t)(-1))
}

/* *****************************************************************************
Reading the first line
***************************************************************************** */

/* *****************************************************************************
Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, FIO_MODULE_NAME)(void) {
  /*
   * TODO: test WebSocket parser here
   */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Cleanup
***************************************************************************** */

#undef FIO_WEBSOCKET_PARSER
#endif /* FIO_WEBSOCKET_PARSER && FIO_EXTERN_COMPLETE */
