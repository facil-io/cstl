/* *****************************************************************************
Test - WebSocket parser module (`431 websocket parser.h`)

Correctness-only coverage for the pure WebSocket frame parser.
All inputs are static byte arrays; no network, no processes, no loops.
***************************************************************************** */
#define FIO_WEBSOCKET_PARSER
#include "test-helpers.h"

#include <string.h>

static int failures = 0;

typedef struct {
  int data_events;
  int control_events;
  int error_events;
  int message_end_events;
  uint8_t last_opcode;
  uint8_t last_rsv;
  uint8_t last_is_text;
  uint8_t last_is_first;
  uint8_t last_is_last;
  uint16_t last_close_code;
  char payload[16384];
  size_t payload_len;
} ws_test_ctx_s;

static void ws_ctx_reset(ws_test_ctx_s *c) { memset(c, 0, sizeof(*c)); }

static void ws_ctx_record(ws_test_ctx_s *c, fio_websocket_event_s *ev) {
  if (ev->type == FIO_WEBSOCKET_EV_MESSAGE_END) {
    ++c->message_end_events;
  } else if (ev->type == FIO_WEBSOCKET_EV_DATA_CHUNK) {
    ++c->data_events;
    c->last_opcode = ev->opcode;
    c->last_rsv = ev->rsv;
    c->last_is_text = ev->is_text;
    c->last_is_first = ev->is_first;
    c->last_is_last = ev->is_last;
    if (ev->payload.len &&
        c->payload_len + ev->payload.len <= sizeof(c->payload)) {
      memcpy(c->payload + c->payload_len, ev->payload.buf, ev->payload.len);
      c->payload_len += ev->payload.len;
    }
  } else if (ev->type == FIO_WEBSOCKET_EV_CONTROL) {
    ++c->control_events;
    c->last_opcode = ev->opcode;
    c->last_close_code = ev->close_code;
  } else if (ev->type == FIO_WEBSOCKET_EV_ERROR) {
    ++c->error_events;
    c->last_close_code = ev->close_code;
  }
}

#define WS_CHECK(cond, msg)                                                    \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s (line %d)\n", (msg), __LINE__);                \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

/* -------------------------------------------------------------------------- */
/* 1. Basic frame types: text, binary, close, ping, pong                      */
/* -------------------------------------------------------------------------- */

static void test_struct_size(void) {
  WS_CHECK(sizeof(fio_websocket_s) == 24, "parser struct stays 24 bytes");
}

static void test_text_masked(void) {
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_message_client(
      buf, FIO_BUF_INFO2((char *)"hello", 5), 1, 0, 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "text masked: all bytes consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "text masked: data event emitted");
  WS_CHECK(ev.opcode == FIO_WEBSOCKET_OP_TEXT, "text masked: opcode text");
  WS_CHECK(ev.is_first && ev.is_last, "text masked: single chunk boundaries");
  WS_CHECK(ev.is_text, "text masked: text flag set");
  WS_CHECK(ev.payload.len == 5 && !memcmp(ev.payload.buf, "hello", 5),
           "text masked: payload unmasked");
  WS_CHECK(ctx.data_events == 1, "text masked: one data event");
}

static void test_text_unmasked(void) {
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2((char *)"hi", 2), 1, 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "text unmasked: all bytes consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "text unmasked: data event emitted");
  WS_CHECK(ev.payload.len == 2 && !memcmp(ev.payload.buf, "hi", 2),
           "text unmasked: payload preserved");
  WS_CHECK(!FIO_WEBSOCKET_GET_MASKED(&p), "text unmasked: mask flag clear");
}

static void test_binary_frame(void) {
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  static const uint8_t data[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};
  uint64_t n = fio_websocket_write_message_client(
      buf, FIO_BUF_INFO2((char *)data, sizeof(data)), 0, 0, 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "binary: all bytes consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "binary: data event emitted");
  WS_CHECK(ev.opcode == FIO_WEBSOCKET_OP_BINARY, "binary: opcode binary");
  WS_CHECK(!ev.is_text, "binary: text flag clear");
  WS_CHECK(ev.payload.len == sizeof(data) &&
               !memcmp(ev.payload.buf, data, sizeof(data)),
           "binary: payload preserved");
}

static void test_ping_unmasked(void) {
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_ping_server(
      buf, FIO_BUF_INFO2((char *)"ping", 4));
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "ping unmasked: all bytes consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_CONTROL, "ping unmasked: control event");
  WS_CHECK(ev.opcode == FIO_WEBSOCKET_OP_PING, "ping unmasked: opcode ping");
  WS_CHECK(ev.payload.len == 4 && !memcmp(ev.payload.buf, "ping", 4),
           "ping unmasked: payload preserved");
}

static void test_pong_masked(void) {
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_pong_client(
      buf, FIO_BUF_INFO2((char *)"pong", 4), 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "pong masked: all bytes consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_CONTROL, "pong masked: control event");
  WS_CHECK(ev.opcode == FIO_WEBSOCKET_OP_PONG, "pong masked: opcode pong");
  WS_CHECK(ev.payload.len == 4 && !memcmp(ev.payload.buf, "pong", 4),
           "pong masked: payload unmasked");
}

static void test_close_with_reason(void) {
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_close_server(
      buf, FIO_WEBSOCKET_CLOSE_GOING_AWAY, FIO_BUF_INFO2((char *)"bye", 3));
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "close reason: all bytes consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_CONTROL, "close reason: control event");
  WS_CHECK(ev.opcode == FIO_WEBSOCKET_OP_CLOSE, "close reason: opcode close");
  WS_CHECK(ev.close_code == FIO_WEBSOCKET_CLOSE_GOING_AWAY,
           "close reason: close code parsed");
  WS_CHECK(ev.payload.len == 5 && !memcmp(ev.payload.buf + 2, "bye", 3),
           "close reason: reason preserved");
  WS_CHECK(p.state == FIO_WEBSOCKET_STATE_CLOSED,
           "close reason: parser moved to closed state");
}

static void test_close_empty(void) {
  uint8_t frame[] = {0x88, 0x00};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == sizeof(frame), "close empty: all bytes consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_CONTROL, "close empty: control event");
  WS_CHECK(ev.close_code == FIO_WEBSOCKET_CLOSE_NO_STATUS,
           "close empty: NO_STATUS synthesized");
}

/* -------------------------------------------------------------------------- */
/* 2. Payload length encodings                                                */
/* -------------------------------------------------------------------------- */

static void test_small_payload(void) {
  uint8_t buf[256];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  char payload[125];
  memset(payload, 'x', sizeof(payload));
  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2(payload, sizeof(payload)), 1, 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "small payload: all bytes consumed");
  WS_CHECK(ev.payload.len == sizeof(payload) &&
               !memcmp(ev.payload.buf, payload, sizeof(payload)),
           "small payload: payload preserved");
}

static void test_medium_payload_126(void) {
  uint8_t buf[512];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  char payload[126];
  memset(payload, 'y', sizeof(payload));
  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2(payload, sizeof(payload)), 0, 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "medium 126: all bytes consumed");
  WS_CHECK(ev.payload.len == sizeof(payload) &&
               !memcmp(ev.payload.buf, payload, sizeof(payload)),
           "medium 126: payload preserved");
}

static void test_medium_payload_16bit(void) {
  uint8_t buf[1024];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  char payload[300];
  memset(payload, 'z', sizeof(payload));
  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2(payload, sizeof(payload)), 1, 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "medium 16-bit: all bytes consumed");
  WS_CHECK(ev.payload.len == sizeof(payload) &&
               !memcmp(ev.payload.buf, payload, sizeof(payload)),
           "medium 16-bit: payload preserved");
}

static void test_large_payload_64bit(void) {
  uint8_t buf[66000];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  char payload[65536];
  for (size_t i = 0; i < sizeof(payload); ++i)
    payload[i] = (char)(i & 0xFF);
  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2(payload, sizeof(payload)), 0, 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "large 64-bit: all bytes consumed");
  WS_CHECK(ev.payload.len == sizeof(payload) &&
               !memcmp(ev.payload.buf, payload, sizeof(payload)),
           "large 64-bit: payload preserved");
}

/* -------------------------------------------------------------------------- */
/* 3. Fragmented messages and interleaved control frames                      */
/* -------------------------------------------------------------------------- */

static void test_fragmented_text(void) {
  uint8_t stream[64];
  size_t off = 0;
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  /* first fragment: FIN=0, opcode=TEXT, payload "abc" */
  stream[off++] = 0x01;
  stream[off++] = 0x03;
  memcpy(stream + off, "abc", 3);
  off += 3;
  /* ping interleaved */
  stream[off++] = 0x89;
  stream[off++] = 0x02;
  memcpy(stream + off, "PP", 2);
  off += 2;
  /* final fragment: FIN=1, opcode=CONT, payload "de" */
  stream[off++] = 0x80;
  stream[off++] = 0x02;
  memcpy(stream + off, "de", 2);
  off += 2;

  size_t pos = 0;
  size_t c1 = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)stream + pos, off - pos), &ev);
  pos += c1;
  ws_ctx_record(&ctx, &ev);
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "fragmented: first event data");
  WS_CHECK(ev.is_first && !ev.is_last, "fragmented: first chunk boundaries");
  WS_CHECK(ev.payload.len == 3 && !memcmp(ev.payload.buf, "abc", 3),
           "fragmented: first payload");

  size_t c2 = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)stream + pos, off - pos), &ev);
  pos += c2;
  ws_ctx_record(&ctx, &ev);
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_CONTROL,
           "fragmented: ping control event");
  WS_CHECK(ev.opcode == FIO_WEBSOCKET_OP_PING,
           "fragmented: ping opcode correct");

  size_t c3 = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)stream + pos, off - pos), &ev);
  pos += c3;
  ws_ctx_record(&ctx, &ev);
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "fragmented: final event data");
  WS_CHECK(!ev.is_first && ev.is_last, "fragmented: final chunk boundaries");
  WS_CHECK(pos == off, "fragmented: whole stream consumed");
  WS_CHECK(ctx.data_events == 2, "fragmented: two data events");
  WS_CHECK(ctx.control_events == 1, "fragmented: one control event");
  WS_CHECK(ctx.payload_len == 5 && !memcmp(ctx.payload, "abcde", 5),
           "fragmented: reassembled payload");
}

static void test_continuation_without_open(void) {
  uint8_t frame[] = {0x80, 0x00};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == FIO_WEBSOCKET_PARSE_ERROR,
           "continuation without open: parse error");
  WS_CHECK(ctx.error_events == 1, "continuation without open: error event");
}

static void test_new_data_while_open(void) {
  uint8_t stream[16];
  size_t off = 0;
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  stream[off++] = 0x01;
  stream[off++] = 0x01;
  stream[off++] = 'a';
  stream[off++] = 0x81;
  stream[off++] = 0x01;
  stream[off++] = 'b';

  size_t c1 = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)stream, off), &ev);
  ws_ctx_record(&ctx, &ev);
  WS_CHECK(c1 == 3, "new text while fragmented: first fragment consumed");
  WS_CHECK(ctx.data_events == 1, "new text while fragmented: first data event");

  size_t c2 = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)stream + c1, off - c1), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c2 == FIO_WEBSOCKET_PARSE_ERROR,
           "new text while fragmented: error event");
  WS_CHECK(ctx.error_events == 1,
           "new text while fragmented: one error event");
  WS_CHECK(ctx.last_close_code == FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
           "new text while fragmented: protocol error code");
}

/* -------------------------------------------------------------------------- */
/* 4. Masking correctness                                                     */
/* -------------------------------------------------------------------------- */

static void test_explicit_mask(void) {
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  uint32_t mask = 0x12345678;
  uint64_t n = fio_websocket_write_message_client(
      buf, FIO_BUF_INFO2((char *)"masked", 6), 1, mask, 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "explicit mask: all bytes consumed");
  WS_CHECK(FIO_WEBSOCKET_GET_MASKED(&p), "explicit mask: mask flag set");
  WS_CHECK(ev.payload.len == 6 && !memcmp(ev.payload.buf, "masked", 6),
           "explicit mask: payload unmasked");
}

static void test_mask_rotation_across_splits(void) {
  static const char original[] = "0123456789ABCDEF";
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_message_client(
      buf, FIO_BUF_INFO2((char *)original, sizeof(original) - 1), 0, 0, 0);

  size_t c1 = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, 7), &ev);
  ws_ctx_record(&ctx, &ev);
  WS_CHECK(c1 == 7, "mask rotation: first split consumed header + 1 byte");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "mask rotation: first split data event");
  WS_CHECK(ev.payload.len == 1 && !memcmp(ev.payload.buf, original, 1),
           "mask rotation: first byte unmasked");
  WS_CHECK(ev.is_first && !ev.is_last, "mask rotation: first split boundaries");

  size_t c2 = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)buf + c1, (size_t)n - c1), &ev);
  ws_ctx_record(&ctx, &ev);
  WS_CHECK(c1 + c2 == n, "mask rotation: total consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "mask rotation: second split data event");
  WS_CHECK(!ev.is_first && ev.is_last, "mask rotation: final boundaries");
  WS_CHECK(ctx.payload_len == sizeof(original) - 1 &&
               !memcmp(ctx.payload, original, sizeof(original) - 1),
           "mask rotation: full payload preserved");
}

/* -------------------------------------------------------------------------- */
/* 5. RSV bits                                                                */
/* -------------------------------------------------------------------------- */

static void test_rsv_bits_surfaced(void) {
  uint8_t frame[] = {0xC1, 0x02, 'o', 'k'};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == sizeof(frame), "rsv: frame consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK, "rsv: data event");
  WS_CHECK(ev.rsv == FIO_WEBSOCKET_RSV1, "rsv: RSV1 surfaced");
  WS_CHECK(ev.payload.len == 2 && !memcmp(ev.payload.buf, "ok", 2),
           "rsv: payload preserved");
}

static void test_rsv_all_bits(void) {
  uint8_t frame[] = {0xF1, 0x02, 'o', 'k'};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == sizeof(frame), "rsv all: frame consumed");
  WS_CHECK(ev.rsv == (FIO_WEBSOCKET_RSV1 | FIO_WEBSOCKET_RSV2 |
                       FIO_WEBSOCKET_RSV3),
           "rsv all: all RSV bits surfaced");
}

/* -------------------------------------------------------------------------- */
/* 6. Protocol errors                                                         */
/* -------------------------------------------------------------------------- */

static void test_bad_opcode(void) {
  uint8_t frame[] = {0x83, 0x00};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == FIO_WEBSOCKET_PARSE_ERROR, "bad opcode: parse error");
  WS_CHECK(ctx.error_events == 1, "bad opcode: error event");
  WS_CHECK(ctx.last_close_code == FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
           "bad opcode: protocol error code");
}

static void test_control_frame_fin_zero(void) {
  uint8_t frame[] = {0x09, 0x00};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == FIO_WEBSOCKET_PARSE_ERROR, "control fin=0: parse error");
  WS_CHECK(ctx.last_close_code == FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
           "control fin=0: protocol error code");
}

static void test_oversized_control_payload(void) {
  uint8_t hdr[] = {0x89, 0x7E, 0x00, 0x7E};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)hdr, sizeof(hdr)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == FIO_WEBSOCKET_PARSE_ERROR,
           "oversized control: parse error");
  WS_CHECK(ctx.last_close_code == FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
           "oversized control: protocol error code");
}

static void test_close_one_byte_payload(void) {
  uint8_t frame[] = {0x88, 0x01, 0xAB};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == FIO_WEBSOCKET_PARSE_ERROR,
           "close one byte: parse error");
  WS_CHECK(ctx.last_close_code == FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
           "close one byte: protocol error code");
}

static void test_close_invalid_code(void) {
  /* 0x1388 = 5000, outside allowed close-code range */
  uint8_t frame[] = {0x88, 0x02, 0x13, 0x88};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == FIO_WEBSOCKET_PARSE_ERROR, "close invalid code: parse error");
  WS_CHECK(ctx.last_close_code == FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
           "close invalid code: protocol error code");
}

static void test_length_high_bit_set(void) {
  uint8_t frame[] = {0x82, 0x7F, 0x80, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == FIO_WEBSOCKET_PARSE_ERROR, "length high bit: parse error");
  WS_CHECK(ctx.last_close_code == FIO_WEBSOCKET_CLOSE_PROTOCOL_ERROR,
           "length high bit: protocol error code");
}

static void test_oversized_payload(void) {
  uint8_t frame[] = {0x82, 0x7F, 0x00, 0x00, 0x00, 0x01,
                     0x00, 0x00, 0x00, 0x00, 0x00};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == FIO_WEBSOCKET_PARSE_ERROR, "oversized payload: parse error");
  WS_CHECK(ctx.last_close_code == FIO_WEBSOCKET_CLOSE_MESSAGE_TOO_BIG,
           "oversized payload: message too big code");
}

/* -------------------------------------------------------------------------- */
/* 7. Partial input handling                                                  */
/* -------------------------------------------------------------------------- */

static void test_partial_header(void) {
  uint8_t frame[] = {0x81};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == 0, "partial header: nothing consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_NONE, "partial header: no event");
}

static void test_partial_masked_header(void) {
  uint8_t frame[] = {0x81, 0x85, 0x01, 0x02};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == 0, "partial masked header: nothing consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_NONE,
           "partial masked header: no event");
}

static void test_partial_data_payload(void) {
  uint8_t frame[] = {0x81, 0x05, 'h', 'e', 'l'};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == sizeof(frame), "partial data payload: consumed available");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "partial data payload: data event");
  WS_CHECK(ev.payload.len == 3 && !memcmp(ev.payload.buf, "hel", 3),
           "partial data payload: partial bytes returned");
  WS_CHECK(ev.is_first && !ev.is_last, "partial data payload: not last");
}

static void test_partial_control_payload(void) {
  uint8_t frame[] = {0x89, 0x03, 'h', 'e'};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == 2, "partial control payload: header consumed only");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_NONE,
           "partial control payload: no event yet");
}

/* -------------------------------------------------------------------------- */
/* 8. Lifecycle and edge cases                                                */
/* -------------------------------------------------------------------------- */

static void test_zero_payload_data(void) {
  uint8_t frame[] = {0x81, 0x00};
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)frame, sizeof(frame)), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == sizeof(frame), "zero payload: frame consumed");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK, "zero payload: data event");
  WS_CHECK(ev.is_first && ev.is_last, "zero payload: boundaries correct");
  WS_CHECK(ev.payload.len == 0, "zero payload: empty payload");
}

static void test_multiple_frames_one_buffer(void) {
  uint8_t stream[32];
  size_t off = 0;
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  /* text "a" */
  stream[off++] = 0x81;
  stream[off++] = 0x01;
  stream[off++] = 'a';
  /* ping "b" */
  stream[off++] = 0x89;
  stream[off++] = 0x01;
  stream[off++] = 'b';

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)stream, off), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == 3, "multi frame: consumed first frame only");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "multi frame: first event data");
  WS_CHECK(ev.payload.len == 1 && ev.payload.buf[0] == 'a',
           "multi frame: first payload");

  c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)stream + 3, off - 3), &ev);
  ws_ctx_record(&ctx, &ev);
  WS_CHECK(c == 3, "multi frame: consumed second frame");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_CONTROL,
           "multi frame: second event control");
  WS_CHECK(ev.payload.len == 1 && ev.payload.buf[0] == 'b',
           "multi frame: ping payload");
}

static void test_reset_after_error(void) {
  uint8_t bad[] = {0x83, 0x00};
  uint8_t good[64];
  fio_websocket_s p;
  fio_websocket_event_s ev;
  ws_test_ctx_s ctx;
  ws_ctx_reset(&ctx);
  fio_websocket_init(&p);

  size_t c = fio_websocket_parse(
      &p, FIO_BUF_INFO2((char *)bad, sizeof(bad)), &ev);
  ws_ctx_record(&ctx, &ev);
  WS_CHECK(c == FIO_WEBSOCKET_PARSE_ERROR, "reset after error: first parse errors");

  fio_websocket_reset(&p);
  ws_ctx_reset(&ctx);

  uint64_t n = fio_websocket_write_message_server(
      good, FIO_BUF_INFO2((char *)"ok", 2), 1, 0);
  c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)good, n), &ev);
  ws_ctx_record(&ctx, &ev);

  WS_CHECK(c == n, "reset after error: parser usable after reset");
  WS_CHECK(ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK,
           "reset after error: event emitted");
  WS_CHECK(ev.payload.len == 2 && !memcmp(ev.payload.buf, "ok", 2),
           "reset after error: payload correct");
}

static void test_null_event_pointer(void) {
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2((char *)"x", 1), 1, 0);
  size_t c = fio_websocket_parse(&p, FIO_BUF_INFO2((char *)buf, n), NULL);

  WS_CHECK(c == n, "null event: parse accepts NULL event pointer");
}

/* -------------------------------------------------------------------------- */
/* Main                                                                       */
/* -------------------------------------------------------------------------- */

int main(void) {
  test_struct_size();
  test_text_masked();
  test_text_unmasked();
  test_binary_frame();
  test_ping_unmasked();
  test_pong_masked();
  test_close_with_reason();
  test_close_empty();
  test_small_payload();
  test_medium_payload_126();
  test_medium_payload_16bit();
  test_large_payload_64bit();
  test_fragmented_text();
  test_continuation_without_open();
  test_new_data_while_open();
  test_explicit_mask();
  test_mask_rotation_across_splits();
  test_rsv_bits_surfaced();
  test_rsv_all_bits();
  test_bad_opcode();
  test_control_frame_fin_zero();
  test_oversized_control_payload();
  test_close_one_byte_payload();
  test_close_invalid_code();
  test_length_high_bit_set();
  test_oversized_payload();
  test_partial_header();
  test_partial_masked_header();
  test_partial_data_payload();
  test_partial_control_payload();
  test_zero_payload_data();
  test_multiple_frames_one_buffer();
  test_reset_after_error();
  test_null_event_pointer();

  if (failures) {
    fprintf(stderr, "\n%d test failure(s)\n", failures);
    return 1;
  }
  fprintf(stderr, "\nAll WebSocket parser tests passed\n");
  return 0;
}
