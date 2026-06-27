/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_WEBSOCKET_PARSER   /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          WebSocket Parser v2 (RFC 6455)
               Zero-allocation, pure event parser, cache-sized.




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_WEBSOCKET_PARSER) && !defined(H___FIO_WEBSOCKET_PARSER___H) && \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)) &&                  \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_WEBSOCKET_PARSER___H

/* *****************************************************************************
Public Constants
***************************************************************************** */

/** WebSocket close codes (RFC 6455 §7.4.1). App-defined range is ≥ 3000. */
typedef enum {
  FIO_WEBSOCKET_CLOSE_OK = 1000,
  FIO_WEBSOCKET_CLOSE_GOING_AWAY = 1001,
  FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR = 1002,
  FIO_WEBSOCKET_CLOSE_UNSUPPORTED_DATA = 1003,
  FIO_WEBSOCKET_CLOSE_NO_STATUS = 1005, /* synthesised on empty close payload */
  FIO_WEBSOCKET_CLOSE_INVALID_PAYLOAD = 1007,
  FIO_WEBSOCKET_CLOSE_POLICY_VIOLATION = 1008,
  FIO_WEBSOCKET_CLOSE_MESSAGE_TOO_BIG = 1009,
  FIO_WEBSOCKET_CLOSE_MANDATORY_EXT = 1010,
  FIO_WEBSOCKET_CLOSE_INTERNAL_ERROR = 1011,
} fio_websocket_close_code_e;

/** Default per-frame payload cap (1 GiB). Override before include if needed. */
#ifndef FIO_WEBSOCKET_DEFAULT_MAX_FRAME
#define FIO_WEBSOCKET_DEFAULT_MAX_FRAME (1ULL << 30)
#endif

/** Parse error sentinel. */
#define FIO_WEBSOCKET_PARSE_ERROR ((size_t)-1)

/** RSV bit constants for the send-side API. These are the 3-bit values shifted
 *  into byte-0 bits 4..6 on the wire (0x4 → RSV1=0x40, 0x2 → RSV2=0x20, 0x1 →
 *  RSV3=0x10). */
#define FIO_WEBSOCKET_RSV1 0x4U /* byte-0 bit 6 (permessage-deflate) */
#define FIO_WEBSOCKET_RSV2 0x2U /* byte-0 bit 5 */
#define FIO_WEBSOCKET_RSV3 0x1U /* byte-0 bit 4 */

/** Event types produced by fio_websocket_parse(). */
enum {
  FIO_WEBSOCKET_EV_NONE = 0,
  FIO_WEBSOCKET_EV_DATA_CHUNK = 1,
  FIO_WEBSOCKET_EV_CONTROL = 2,
  FIO_WEBSOCKET_EV_MESSAGE_END = 3,
  FIO_WEBSOCKET_EV_ERROR = 4,
};

/* WebSocket frame opcodes (RFC 6455). */
#define FIO_WEBSOCKET_OP_CONT   0x0
#define FIO_WEBSOCKET_OP_TEXT   0x1
#define FIO_WEBSOCKET_OP_BINARY 0x2
#define FIO_WEBSOCKET_OP_CLOSE  0x8
#define FIO_WEBSOCKET_OP_PING   0x9
#define FIO_WEBSOCKET_OP_PONG   0xA

/* Parser FSM states. */
#define FIO_WEBSOCKET_STATE_HEADER  0
#define FIO_WEBSOCKET_STATE_PAYLOAD 1
#define FIO_WEBSOCKET_STATE_CLOSED  0xFE
#define FIO_WEBSOCKET_STATE_ERROR   0xFF

/* Flag bit positions in fio_websocket_s.flags. */
#define FIO_WEBSOCKET_FLAG_FIN             0x80
#define FIO_WEBSOCKET_FLAG_MASKED          0x40
#define FIO_WEBSOCKET_FLAG_OPCODE_MASK     0x3C
#define FIO_WEBSOCKET_FLAG_OPCODE_SHIFT    2
#define FIO_WEBSOCKET_FLAG_MSG_OPCODE_MASK 0x03

/* Flag bit positions in fio_websocket_s.flags2. */
#define FIO_WEBSOCKET_FLAG2_PAUSED        0x80
#define FIO_WEBSOCKET_FLAG2_MSG_RSV_MASK  0x70
#define FIO_WEBSOCKET_FLAG2_MSG_RSV_SHIFT 4

/** Read the FIN bit from the current frame. */
#define FIO_WEBSOCKET_GET_FIN(p) (((p)->flags & FIO_WEBSOCKET_FLAG_FIN) != 0)

/** Read the MASK bit from the current frame. */
#define FIO_WEBSOCKET_GET_MASKED(p)                                            \
  (((p)->flags & FIO_WEBSOCKET_FLAG_MASKED) != 0)

/** Read the opcode from the current frame. */
#define FIO_WEBSOCKET_GET_OPCODE(p)                                            \
  (((p)->flags & FIO_WEBSOCKET_FLAG_OPCODE_MASK) >>                            \
   FIO_WEBSOCKET_FLAG_OPCODE_SHIFT)

/** Read the message opcode (1=text, 2=binary, 0=none open). */
#define FIO_WEBSOCKET_GET_MSG_OPCODE(p)                                        \
  ((p)->flags & FIO_WEBSOCKET_FLAG_MSG_OPCODE_MASK)

/** Read the paused flag (one-message-per-parse gate). */
#define FIO_WEBSOCKET_GET_PAUSED(p)                                            \
  (((p)->flags2 & FIO_WEBSOCKET_FLAG2_PAUSED) != 0)

/** Read the RSV bits from the opening frame (3-bit format). */
#define FIO_WEBSOCKET_GET_MSG_RSV(p)                                           \
  (((p)->flags2 & FIO_WEBSOCKET_FLAG2_MSG_RSV_MASK) >>                         \
   FIO_WEBSOCKET_FLAG2_MSG_RSV_SHIFT)

/* *****************************************************************************
Public Types
***************************************************************************** */

typedef struct fio_websocket_s {
  uint64_t frame_remaining;
  uint32_t mask;
  uint32_t frame_consumed;
  uint16_t close_code;
  uint8_t state;
  uint8_t flags;  /* bit 7: fin, bit 6: masked, bits 5-2: opcode, bits 1-0:
                     msg_opcode */
  uint8_t flags2; /* bit 7: paused, bits 6-4: msg_rsv */
  uint8_t reserved;
} fio_websocket_s;

typedef struct {
  uint8_t type;   /* 0=none, 1=data_chunk, 2=control_frame, 3=message_end,
                     4=error */
  uint8_t opcode; /* frame opcode */
  uint8_t is_text;
  uint8_t rsv;            /* opening-frame RSV bits (3-bit format) */
  uint8_t is_first;       /* first chunk of message */
  uint8_t is_last;        /* last chunk of message / frame */
  fio_buf_info_s payload; /* points into input buffer (unmasked in place) */
  uint16_t close_code;
} fio_websocket_event_s;

FIO_ASSERT_STATIC(sizeof(fio_websocket_s) <= 24,
                  "fio_websocket_s must stay 24 bytes");

/* *****************************************************************************
Public API — Parsing
***************************************************************************** */

FIO_IFUNC void fio_websocket_init(fio_websocket_s *p);
FIO_IFUNC void fio_websocket_reset(fio_websocket_s *p);

/** Parses WebSocket bytes from `buf`, unmasking payload in place.
 *
 * Returns bytes consumed (≤ `buf.len`) on success or
 * `FIO_WEBSOCKET_PARSE_ERROR` on protocol error (`p->close_code` is set).
 *
 * The parser is pure: it stores no callbacks, no user pointer, no control-frame
 * buffer, and no message accumulator. The caller owns policy checks (masking,
 * RSV semantics, extension transforms, message accumulation, delivery, etc.).
 */
FIO_SFUNC size_t fio_websocket_parse(fio_websocket_s *p,
                                     fio_buf_info_s buf,
                                     fio_websocket_event_s *ev);

/* *****************************************************************************
Public API — Message Writers

Server functions produce unmasked frames; client functions produce masked
frames (PRNG mask when `mask=0`, explicit mask otherwise).
***************************************************************************** */

FIO_IFUNC uint64_t fio_websocket_write_len(uint64_t payload_len, _Bool masked);

/** Writes a complete (FIN=1) data message.
 *  `rsv` is the 3-bit RSV field — normally 0; pass `FIO_WEBSOCKET_RSV1`
 *  (0x4) to mark the message as permessage-deflate-compressed
 *  (RFC 7692 §7.2.3.1). */
FIO_IFUNC uint64_t fio_websocket_write_message_server(void *target,
                                                      fio_buf_info_s msg,
                                                      _Bool is_text,
                                                      uint8_t rsv);
FIO_IFUNC uint64_t fio_websocket_write_message_client(void *target,
                                                      fio_buf_info_s msg,
                                                      _Bool is_text,
                                                      uint32_t mask,
                                                      uint8_t rsv);
FIO_IFUNC uint64_t fio_websocket_write_ping_server(void *t, fio_buf_info_s p);
FIO_IFUNC uint64_t fio_websocket_write_ping_client(void *t,
                                                   fio_buf_info_s p,
                                                   uint32_t mask);
FIO_IFUNC uint64_t fio_websocket_write_pong_server(void *t, fio_buf_info_s p);
FIO_IFUNC uint64_t fio_websocket_write_pong_client(void *t,
                                                   fio_buf_info_s p,
                                                   uint32_t mask);
FIO_IFUNC uint64_t fio_websocket_write_close_server(void *target,
                                                    uint16_t code,
                                                    fio_buf_info_s reason);
FIO_IFUNC uint64_t fio_websocket_write_close_client(void *target,
                                                    uint16_t code,
                                                    fio_buf_info_s reason,
                                                    uint32_t mask);

/* *****************************************************************************




                                 Implementation




***************************************************************************** */

/** Set the FIN bit in the current frame. */
#define FIO___WEBSOCKET_SET_FIN(p, v)                                          \
  ((p)->flags =                                                                \
       (uint8_t)(((p)->flags & ~FIO_WEBSOCKET_FLAG_FIN) | ((!!(v)) << 7)))

/** Set the MASK bit in the current frame. */
#define FIO___WEBSOCKET_SET_MASKED(p, v)                                       \
  ((p)->flags =                                                                \
       (uint8_t)(((p)->flags & ~FIO_WEBSOCKET_FLAG_MASKED) | ((!!(v)) << 6)))

/** Set the opcode for the current frame. */
#define FIO___WEBSOCKET_SET_OPCODE(p, v)                                       \
  ((p)->flags = (uint8_t)(((p)->flags & ~FIO_WEBSOCKET_FLAG_OPCODE_MASK) |     \
                          (((v)&0x0F) << FIO_WEBSOCKET_FLAG_OPCODE_SHIFT)))

/** Set the message opcode (1=text, 2=binary, 0=none). */
#define FIO___WEBSOCKET_SET_MSG_OPCODE(p, v)                                   \
  ((p)->flags = (uint8_t)(((p)->flags & ~FIO_WEBSOCKET_FLAG_MSG_OPCODE_MASK) | \
                          ((v)&0x03)))

/** Set the paused flag (one-message-per-parse gate). */
#define FIO___WEBSOCKET_SET_PAUSED(p, v)                                       \
  ((p)->flags2 = (uint8_t)(((p)->flags2 & ~FIO_WEBSOCKET_FLAG2_PAUSED) |       \
                           ((!!(v)) << 7)))

/** Set the RSV bits from the opening frame. */
#define FIO___WEBSOCKET_SET_MSG_RSV(p, v)                                      \
  ((p)->flags2 = (uint8_t)(((p)->flags2 & ~FIO_WEBSOCKET_FLAG2_MSG_RSV_MASK) | \
                           (((v)&0x07) << FIO_WEBSOCKET_FLAG2_MSG_RSV_SHIFT)))

/* *****************************************************************************
Frame Writers
***************************************************************************** */

FIO_IFUNC uint64_t fio_websocket_write_len(uint64_t payload_len, _Bool masked) {
  return payload_len + 2ULL + ((payload_len > 125) << 1) +
         ((0ULL - (payload_len > 0xFFFFULL)) & 6ULL) + ((!!masked) << 2);
}

FIO_IFUNC uint64_t fio___websocket_hdr(uint8_t *dst,
                                       uint64_t len,
                                       uint32_t mask,
                                       uint8_t opcode,
                                       uint8_t rsv) {
  dst[0] = (uint8_t)(0x80U | ((rsv & 0x07U) << 4) | (opcode & 0x0FU));
  const uint8_t mb = (uint8_t)((!!mask) << 7);
  uint64_t h;
  if (FIO_LIKELY(len < 126)) {
    dst[1] = (uint8_t)(mb | (uint8_t)len);
    h = 2;
  } else if (len < 0x10000ULL) {
    dst[1] = (uint8_t)(mb | 126);
    fio_u2buf16_be(dst + 2, (uint16_t)len);
    h = 4;
  } else {
    dst[1] = (uint8_t)(mb | 127);
    fio_u2buf64_be(dst + 2, len);
    h = 10;
  }
  if (mask) {
    fio_u2buf32u(dst + h, mask);
    h += 4;
  }
  return h;
}

FIO_IFUNC uint64_t fio___websocket_write_srv(void *target,
                                             fio_buf_info_s payload,
                                             uint8_t opcode,
                                             uint8_t rsv) {
  uint64_t h =
      fio___websocket_hdr((uint8_t *)target, payload.len, 0, opcode, rsv);
  if (payload.len)
    FIO_MEMCPY((char *)target + h, payload.buf, payload.len);
  return h + payload.len;
}

FIO_IFUNC uint64_t fio___websocket_write_cli(void *target,
                                             fio_buf_info_s payload,
                                             uint8_t opcode,
                                             uint32_t mask,
                                             uint8_t rsv) {
  if (!mask)
    mask = (uint32_t)(fio_rand64() | 0x01020408U);
  uint64_t h =
      fio___websocket_hdr((uint8_t *)target, payload.len, mask, opcode, rsv);
  if (payload.len)
    fio_xmask_cpy((char *)target + h,
                  payload.buf,
                  payload.len,
                  ((uint64_t)mask << 32) | (uint64_t)mask);
  return h + payload.len;
}

FIO_ASSERT_STATIC(FIO_WEBSOCKET_OP_BINARY == 2 && FIO_WEBSOCKET_OP_TEXT == 1,
                  "branchless text/binary writer relies on TEXT=1, BINARY=2");

FIO_IFUNC uint64_t fio_websocket_write_message_server(void *t,
                                                      fio_buf_info_s msg,
                                                      _Bool is_text,
                                                      uint8_t rsv) {
  return fio___websocket_write_srv(
      t,
      msg,
      (uint8_t)(FIO_WEBSOCKET_OP_BINARY >> (unsigned)is_text),
      rsv);
}
FIO_IFUNC uint64_t fio_websocket_write_message_client(void *t,
                                                      fio_buf_info_s msg,
                                                      _Bool is_text,
                                                      uint32_t mask,
                                                      uint8_t rsv) {
  return fio___websocket_write_cli(
      t,
      msg,
      (uint8_t)(FIO_WEBSOCKET_OP_BINARY >> (unsigned)is_text),
      mask,
      rsv);
}
FIO_IFUNC uint64_t fio_websocket_write_ping_server(void *t, fio_buf_info_s p) {
  return fio___websocket_write_srv(t, p, FIO_WEBSOCKET_OP_PING, 0);
}
FIO_IFUNC uint64_t fio_websocket_write_ping_client(void *t,
                                                   fio_buf_info_s p,
                                                   uint32_t mask) {
  return fio___websocket_write_cli(t, p, FIO_WEBSOCKET_OP_PING, mask, 0);
}
FIO_IFUNC uint64_t fio_websocket_write_pong_server(void *t, fio_buf_info_s p) {
  return fio___websocket_write_srv(t, p, FIO_WEBSOCKET_OP_PONG, 0);
}
FIO_IFUNC uint64_t fio_websocket_write_pong_client(void *t,
                                                   fio_buf_info_s p,
                                                   uint32_t mask) {
  return fio___websocket_write_cli(t, p, FIO_WEBSOCKET_OP_PONG, mask, 0);
}

FIO_IFUNC size_t fio___websocket_close_body(uint8_t *scratch,
                                            uint16_t code,
                                            fio_buf_info_s reason) {
  size_t n = 2 + reason.len;
  if (n > 125)
    n = 125;
  fio_u2buf16_be(scratch, code);
  if (n > 2 && reason.buf)
    FIO_MEMCPY(scratch + 2, reason.buf, n - 2);
  return n;
}
FIO_IFUNC uint64_t fio_websocket_write_close_server(void *target,
                                                    uint16_t code,
                                                    fio_buf_info_s reason) {
  uint8_t scratch[125];
  size_t n = fio___websocket_close_body(scratch, code, reason);
  return fio___websocket_write_srv(target,
                                   FIO_BUF_INFO2((char *)scratch, n),
                                   FIO_WEBSOCKET_OP_CLOSE,
                                   0);
}
FIO_IFUNC uint64_t fio_websocket_write_close_client(void *target,
                                                    uint16_t code,
                                                    fio_buf_info_s reason,
                                                    uint32_t mask) {
  uint8_t scratch[125];
  size_t n = fio___websocket_close_body(scratch, code, reason);
  return fio___websocket_write_cli(target,
                                   FIO_BUF_INFO2((char *)scratch, n),
                                   FIO_WEBSOCKET_OP_CLOSE,
                                   mask,
                                   0);
}

/* *****************************************************************************
Lifecycle
***************************************************************************** */

FIO_IFUNC void fio_websocket_init(fio_websocket_s *p) {
  *p = (fio_websocket_s){.state = FIO_WEBSOCKET_STATE_HEADER};
}

FIO_IFUNC void fio_websocket_reset(fio_websocket_s *p) {
  *p = (fio_websocket_s){.state = FIO_WEBSOCKET_STATE_HEADER};
}

/* *****************************************************************************
Helpers
***************************************************************************** */

FIO_SFUNC size_t fio___websocket_fail(fio_websocket_s *p,
                                      fio_websocket_event_s *ev,
                                      uint16_t code) {
  p->state = FIO_WEBSOCKET_STATE_ERROR;
  p->close_code = code;
  *ev = (fio_websocket_event_s){.type = FIO_WEBSOCKET_EV_ERROR,
                                .close_code = code};
  return FIO_WEBSOCKET_PARSE_ERROR;
}

FIO_IFUNC uint32_t fio___websocket_rotate_mask(uint32_t mask,
                                               uint32_t consumed) {
  uint8_t m[4], r[4];
  fio_u2buf32u(m, mask);
  const uint32_t n = (uint32_t)(consumed & 3U);
  r[0] = m[(n + 0) & 3];
  r[1] = m[(n + 1) & 3];
  r[2] = m[(n + 2) & 3];
  r[3] = m[(n + 3) & 3];
  return fio_buf2u32u(r);
}

FIO_IFUNC _Bool fio___websocket_opcode_valid(uint8_t opcode) {
  switch (opcode) {
  case FIO_WEBSOCKET_OP_CONT:
  case FIO_WEBSOCKET_OP_TEXT:
  case FIO_WEBSOCKET_OP_BINARY:
  case FIO_WEBSOCKET_OP_CLOSE:
  case FIO_WEBSOCKET_OP_PING:
  case FIO_WEBSOCKET_OP_PONG: return 1;
  }
  return 0;
}

/** RFC 6455 §7.4.1 + IANA registry: codes that may appear on the wire.
 *  1004 / 1005 / 1006 / 1015 are reserved (must not be transmitted);
 *  1016-2999 are unassigned; 3000-4999 are app/library use. */
FIO_IFUNC _Bool fio___websocket_close_code_valid(uint16_t c) {
  switch (c) {
  case 1000:
  case 1001:
  case 1002:
  case 1003:
  case 1007:
  case 1008:
  case 1009:
  case 1010:
  case 1011:
  case 1012:
  case 1013:
  case 1014: return 1;
  }
  return (c >= 3000) && (c <= 4999);
}

/* *****************************************************************************
Main Parse Loop
***************************************************************************** */

FIO_SFUNC size_t fio_websocket_parse(fio_websocket_s *p,
                                     fio_buf_info_s buf,
                                     fio_websocket_event_s *ev_) {
  fio_websocket_event_s local = {0};
  fio_websocket_event_s *const ev = ev_ ? ev_ : &local;
  *ev = (fio_websocket_event_s){0};
  if (FIO_UNLIKELY(!p || p->state >= FIO_WEBSOCKET_STATE_CLOSED))
    return FIO_WEBSOCKET_PARSE_ERROR;
  const char *const start = buf.buf;
  for (;;) {
    if (p->state == FIO_WEBSOCKET_STATE_HEADER) {
      if (buf.len < 2)
        return (size_t)(buf.buf - start);
      const uint8_t b0 = (uint8_t)buf.buf[0];
      const uint8_t b1 = (uint8_t)buf.buf[1];
      const uint8_t fin = (uint8_t)(b0 >> 7);
      const uint8_t wire_rsv = (uint8_t)((b0 >> 4) & 0x07U);
      const uint8_t opcode = (uint8_t)(b0 & 0x0FU);
      const uint8_t masked = (uint8_t)(b1 >> 7);
      const uint8_t len7 = (uint8_t)(b1 & 0x7FU);
      const _Bool is_control = (opcode & 0x08U) != 0;
      const size_t mask_len = (size_t)(masked << 2);
      uint64_t payload_len = 0;
      size_t header_len = 0;

      if (FIO_UNLIKELY(!fio___websocket_opcode_valid(opcode)))
        return fio___websocket_fail(p, ev, FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR);
      if (FIO_UNLIKELY(is_control && !fin))
        return fio___websocket_fail(p, ev, FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR);
      if (FIO_UNLIKELY(opcode == FIO_WEBSOCKET_OP_CONT &&
                       !FIO_WEBSOCKET_GET_MSG_OPCODE(p)))
        return fio___websocket_fail(p, ev, FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR);
      if (FIO_UNLIKELY(!is_control && opcode != FIO_WEBSOCKET_OP_CONT &&
                       FIO_WEBSOCKET_GET_MSG_OPCODE(p)))
        return fio___websocket_fail(p, ev, FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR);

      if (FIO_LIKELY(len7 < 126)) {
        if (buf.len < (size_t)(2 + mask_len))
          return (size_t)(buf.buf - start);
        payload_len = len7;
        header_len = 2;
      } else if (len7 == 126) {
        if (buf.len < (size_t)(4 + mask_len))
          return (size_t)(buf.buf - start);
        payload_len = fio_buf2u16_be(buf.buf + 2);
        header_len = 4;
      } else {
        if (buf.len < (size_t)(10 + mask_len))
          return (size_t)(buf.buf - start);
        payload_len = fio_buf2u64_be(buf.buf + 2);
        if (FIO_UNLIKELY(payload_len >> 63))
          return fio___websocket_fail(p,
                                      ev,
                                      FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR);
        header_len = 10;
      }
      if (FIO_UNLIKELY(is_control && payload_len > 125))
        return fio___websocket_fail(p, ev, FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR);
      if (FIO_UNLIKELY(payload_len > FIO_WEBSOCKET_DEFAULT_MAX_FRAME))
        return fio___websocket_fail(p, ev, FIO_WEBSOCKET_CLOSE_MESSAGE_TOO_BIG);

      p->frame_remaining = payload_len;
      p->frame_consumed = 0;
      p->mask = masked ? fio_buf2u32u(buf.buf + header_len) : 0U;
      FIO___WEBSOCKET_SET_FIN(p, fin);
      FIO___WEBSOCKET_SET_MASKED(p, masked);
      FIO___WEBSOCKET_SET_OPCODE(p, opcode);
      FIO___WEBSOCKET_SET_PAUSED(p, 0);
      p->state = FIO_WEBSOCKET_STATE_PAYLOAD;
      if (!is_control && opcode != FIO_WEBSOCKET_OP_CONT) {
        FIO___WEBSOCKET_SET_MSG_OPCODE(p, opcode);
        FIO___WEBSOCKET_SET_MSG_RSV(p, wire_rsv);
      }
      buf.buf += header_len + mask_len;
      buf.len -= header_len + mask_len;
    }

    {
      const uint8_t opcode = FIO_WEBSOCKET_GET_OPCODE(p);
      const uint8_t msg_opcode = FIO_WEBSOCKET_GET_MSG_OPCODE(p);
      const uint8_t msg_rsv = FIO_WEBSOCKET_GET_MSG_RSV(p);
      const _Bool is_control = (opcode & 0x08U) != 0;
      const _Bool first_chunk_of_frame = (p->frame_consumed == 0);
      const _Bool first_chunk_of_msg =
          (first_chunk_of_frame && opcode != FIO_WEBSOCKET_OP_CONT);
      const size_t chunk_len =
          (buf.len < p->frame_remaining) ? buf.len : (size_t)p->frame_remaining;
      const _Bool last_chunk_of_frame = (chunk_len == p->frame_remaining);
      const _Bool last_chunk_of_msg =
          (last_chunk_of_frame && FIO_WEBSOCKET_GET_FIN(p));

      if (is_control) {
        if (p->frame_remaining && buf.len < p->frame_remaining)
          return (size_t)(buf.buf - start);
        if (chunk_len && FIO_WEBSOCKET_GET_MASKED(p)) {
          const uint32_t rot =
              fio___websocket_rotate_mask(p->mask, p->frame_consumed);
          fio_xmask(buf.buf, chunk_len, ((uint64_t)rot << 32) | (uint64_t)rot);
        }
        ev->type = FIO_WEBSOCKET_EV_CONTROL;
        ev->opcode = opcode;
        ev->payload = FIO_BUF_INFO2(buf.buf, chunk_len);
        if (opcode == FIO_WEBSOCKET_OP_CLOSE) {
          uint16_t code = FIO_WEBSOCKET_CLOSE_NO_STATUS;
          if (FIO_UNLIKELY(chunk_len == 1))
            return fio___websocket_fail(p,
                                        ev,
                                        FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR);
          if (chunk_len >= 2) {
            code = fio_buf2u16_be(buf.buf);
            if (FIO_UNLIKELY(!fio___websocket_close_code_valid(code)))
              return fio___websocket_fail(p,
                                          ev,
                                          FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR);
          }
          p->close_code = code;
          ev->close_code = code;
          p->state = FIO_WEBSOCKET_STATE_CLOSED;
        } else {
          p->state = FIO_WEBSOCKET_STATE_HEADER;
        }
        buf.buf += chunk_len;
        buf.len -= chunk_len;
        p->frame_consumed += (uint32_t)chunk_len;
        p->frame_remaining -= chunk_len;
        return (size_t)(buf.buf - start);
      }

      if (!chunk_len && p->frame_remaining)
        return (size_t)(buf.buf - start);
      if (chunk_len && FIO_WEBSOCKET_GET_MASKED(p)) {
        const uint32_t rot =
            fio___websocket_rotate_mask(p->mask, p->frame_consumed);
        fio_xmask(buf.buf, chunk_len, ((uint64_t)rot << 32) | (uint64_t)rot);
      }
      ev->type = FIO_WEBSOCKET_EV_DATA_CHUNK;
      ev->opcode = opcode;
      ev->is_text = (uint8_t)(msg_opcode == FIO_WEBSOCKET_OP_TEXT);
      ev->rsv = msg_rsv;
      ev->is_first = (uint8_t)first_chunk_of_msg;
      ev->is_last = (uint8_t)last_chunk_of_msg;
      ev->payload = FIO_BUF_INFO2(buf.buf, chunk_len);

      buf.buf += chunk_len;
      buf.len -= chunk_len;
      p->frame_consumed += (uint32_t)chunk_len;
      p->frame_remaining -= chunk_len;
      if (last_chunk_of_frame)
        p->state = FIO_WEBSOCKET_STATE_HEADER;
      if (last_chunk_of_msg) {
        FIO___WEBSOCKET_SET_MSG_OPCODE(p, 0);
        FIO___WEBSOCKET_SET_MSG_RSV(p, 0);
      }
      return (size_t)(buf.buf - start);
    }
  }
}

#undef FIO_WEBSOCKET_PARSER
#endif /* FIO_WEBSOCKET_PARSER */
