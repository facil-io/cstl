/* *****************************************************************************
Test - WebSocket frame roundtrip (`431 websocket parser.h`)

Encode frames with the public writer API and parse them back with the pure
parser. Single-process, deterministic, no network or external processes.
***************************************************************************** */
#define FIO_WEBSOCKET_PARSER
#include "test-helpers.h"

#include <string.h>

static int failures = 0;

#define WS_RT_CHECK(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s (line %d)\n", (msg), __LINE__);                \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

typedef struct {
  uint8_t opcode;
  uint8_t is_text;
  uint8_t is_first;
  uint8_t is_last;
  uint16_t close_code;
  char payload[131072];
  size_t payload_len;
} ws_rt_result_s;

static ws_rt_result_s ws_rt_parse_one(uint8_t *buf,
                                      size_t len,
                                      fio_websocket_s *p) {
  ws_rt_result_s r = {0};
  fio_websocket_event_s ev;
  size_t consumed = 0;

  while (consumed < len) {
    size_t c = fio_websocket_parse(
        p, FIO_BUF_INFO2((char *)buf + consumed, len - consumed), &ev);
    WS_RT_CHECK(c != FIO_WEBSOCKET_PARSE_ERROR, "roundtrip: parse error");
    if (c == 0)
      break;
    consumed += c;

    if (ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK) {
      r.opcode = ev.opcode;
      r.is_text = ev.is_text;
      r.is_first = ev.is_first;
      r.is_last = ev.is_last;
      if (ev.payload.len &&
          r.payload_len + ev.payload.len <= sizeof(r.payload)) {
        memcpy(r.payload + r.payload_len, ev.payload.buf, ev.payload.len);
        r.payload_len += ev.payload.len;
      }
    } else if (ev.type == FIO_WEBSOCKET_EV_CONTROL) {
      r.opcode = ev.opcode;
      r.close_code = ev.close_code;
      if (ev.payload.len &&
          r.payload_len + ev.payload.len <= sizeof(r.payload)) {
        memcpy(r.payload + r.payload_len, ev.payload.buf, ev.payload.len);
        r.payload_len += ev.payload.len;
      }
    }
  }
  (void)consumed;
  return r;
}

static void test_roundtrip_text_server(void) {
  uint8_t buf[256];
  fio_websocket_s p;
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2((char *)"hello", 5), 1, 0);
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);

  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_TEXT, "rt text server: opcode");
  WS_RT_CHECK(r.is_text, "rt text server: text flag");
  WS_RT_CHECK(r.is_first && r.is_last, "rt text server: boundaries");
  WS_RT_CHECK(r.payload_len == 5 && !memcmp(r.payload, "hello", 5),
              "rt text server: payload");
}

static void test_roundtrip_text_client(void) {
  uint8_t buf[256];
  fio_websocket_s p;
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_message_client(
      buf, FIO_BUF_INFO2((char *)"hello", 5), 1, 0, 0);
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);

  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_TEXT, "rt text client: opcode");
  WS_RT_CHECK(r.is_text, "rt text client: text flag");
  WS_RT_CHECK(r.payload_len == 5 && !memcmp(r.payload, "hello", 5),
              "rt text client: payload");
}

static void test_roundtrip_binary_server(void) {
  uint8_t buf[256];
  fio_websocket_s p;
  fio_websocket_init(&p);

  static const uint8_t data[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};
  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2((char *)data, sizeof(data)), 0, 0);
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);

  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_BINARY, "rt binary server: opcode");
  WS_RT_CHECK(!r.is_text, "rt binary server: text flag");
  WS_RT_CHECK(r.payload_len == sizeof(data) &&
                  !memcmp(r.payload, data, sizeof(data)),
              "rt binary server: payload");
}

static void test_roundtrip_binary_client(void) {
  uint8_t buf[256];
  fio_websocket_s p;
  fio_websocket_init(&p);

  static const uint8_t data[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};
  uint64_t n = fio_websocket_write_message_client(
      buf, FIO_BUF_INFO2((char *)data, sizeof(data)), 0, 0, 0);
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);

  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_BINARY, "rt binary client: opcode");
  WS_RT_CHECK(r.payload_len == sizeof(data) &&
                  !memcmp(r.payload, data, sizeof(data)),
              "rt binary client: payload");
}

static void test_roundtrip_ping_pong(void) {
  uint8_t buf[256];
  fio_websocket_s p;
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_ping_server(
      buf, FIO_BUF_INFO2((char *)"ping", 4));
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);
  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_PING, "rt ping server: opcode");
  WS_RT_CHECK(r.payload_len == 4 && !memcmp(r.payload, "ping", 4),
              "rt ping server: payload");

  fio_websocket_init(&p);
  n = fio_websocket_write_ping_client(
      buf, FIO_BUF_INFO2((char *)"ping", 4), 0);
  r = ws_rt_parse_one(buf, (size_t)n, &p);
  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_PING, "rt ping client: opcode");
  WS_RT_CHECK(r.payload_len == 4 && !memcmp(r.payload, "ping", 4),
              "rt ping client: payload");

  fio_websocket_init(&p);
  n = fio_websocket_write_pong_server(
      buf, FIO_BUF_INFO2((char *)"pong", 4));
  r = ws_rt_parse_one(buf, (size_t)n, &p);
  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_PONG, "rt pong server: opcode");

  fio_websocket_init(&p);
  n = fio_websocket_write_pong_client(
      buf, FIO_BUF_INFO2((char *)"pong", 4), 0);
  r = ws_rt_parse_one(buf, (size_t)n, &p);
  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_PONG, "rt pong client: opcode");
}

static void test_roundtrip_close(void) {
  uint8_t buf[256];
  fio_websocket_s p;

  fio_websocket_init(&p);
  uint64_t n = fio_websocket_write_close_server(
      buf, FIO_WEBSOCKET_CLOSE_OK, FIO_BUF_INFO2((char *)"bye", 3));
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);
  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_CLOSE, "rt close server: opcode");
  WS_RT_CHECK(r.close_code == FIO_WEBSOCKET_CLOSE_OK,
              "rt close server: close code");
  WS_RT_CHECK(r.payload_len == 5 && !memcmp(r.payload + 2, "bye", 3),
              "rt close server: reason");

  fio_websocket_init(&p);
  n = fio_websocket_write_close_client(
      buf, FIO_WEBSOCKET_CLOSE_GOING_AWAY, FIO_BUF_INFO2((char *)"go", 2), 0);
  r = ws_rt_parse_one(buf, (size_t)n, &p);
  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_CLOSE, "rt close client: opcode");
  WS_RT_CHECK(r.close_code == FIO_WEBSOCKET_CLOSE_GOING_AWAY,
              "rt close client: close code");
  WS_RT_CHECK(r.payload_len == 4 && !memcmp(r.payload + 2, "go", 2),
              "rt close client: reason");
}

static void test_roundtrip_medium_126(void) {
  uint8_t buf[512];
  char payload[126];
  fio_websocket_s p;
  fio_websocket_init(&p);

  memset(payload, 'm', sizeof(payload));
  uint64_t n = fio_websocket_write_message_client(
      buf, FIO_BUF_INFO2(payload, sizeof(payload)), 1, 0, 0);
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);

  WS_RT_CHECK(r.payload_len == sizeof(payload) &&
                  !memcmp(r.payload, payload, sizeof(payload)),
              "rt medium 126: payload");
}

static void test_roundtrip_16bit_length(void) {
  uint8_t buf[1024];
  char payload[400];
  fio_websocket_s p;
  fio_websocket_init(&p);

  for (size_t i = 0; i < sizeof(payload); ++i)
    payload[i] = (char)('A' + (i & 3));
  uint64_t n = fio_websocket_write_message_client(
      buf, FIO_BUF_INFO2(payload, sizeof(payload)), 0, 0, 0);
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);

  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_BINARY, "rt 16-bit: opcode");
  WS_RT_CHECK(r.payload_len == sizeof(payload) &&
                  !memcmp(r.payload, payload, sizeof(payload)),
              "rt 16-bit: payload");
}

static void test_roundtrip_64bit_length(void) {
  uint8_t buf[66000];
  char payload[65536];
  fio_websocket_s p;
  fio_websocket_init(&p);

  for (size_t i = 0; i < sizeof(payload); ++i)
    payload[i] = (char)(i & 0xFF);
  uint64_t n = fio_websocket_write_message_client(
      buf, FIO_BUF_INFO2(payload, sizeof(payload)), 1, 0, 0);
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);

  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_TEXT, "rt 64-bit: opcode");
  WS_RT_CHECK(r.payload_len == sizeof(payload) &&
                  !memcmp(r.payload, payload, sizeof(payload)),
              "rt 64-bit: payload");
}

static void test_roundtrip_zero_payload(void) {
  uint8_t buf[64];
  fio_websocket_s p;
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2(NULL, 0), 1, 0);
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);

  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_TEXT, "rt zero payload: opcode");
  WS_RT_CHECK(r.payload_len == 0, "rt zero payload: empty");
  WS_RT_CHECK(r.is_first && r.is_last, "rt zero payload: boundaries");
}

static void test_roundtrip_rsv1(void) {
  uint8_t buf[256];
  fio_websocket_s p;
  fio_websocket_init(&p);

  uint64_t n = fio_websocket_write_message_server(
      buf, FIO_BUF_INFO2((char *)"compress", 8), 1, FIO_WEBSOCKET_RSV1);
  ws_rt_result_s r = ws_rt_parse_one(buf, (size_t)n, &p);

  WS_RT_CHECK(r.opcode == FIO_WEBSOCKET_OP_TEXT, "rt rsv1: opcode");
  WS_RT_CHECK(r.payload_len == 8 && !memcmp(r.payload, "compress", 8),
              "rt rsv1: payload");
}

static void test_roundtrip_multiple_messages(void) {
  uint8_t buf[512];
  size_t len = 0;
  fio_websocket_s p;
  fio_websocket_init(&p);

  len += fio_websocket_write_message_server(
      buf + len, FIO_BUF_INFO2((char *)"one", 3), 1, 0);
  len += fio_websocket_write_message_server(
      buf + len, FIO_BUF_INFO2((char *)"two", 3), 1, 0);

  ws_rt_result_s r1 = {0};
  ws_rt_result_s r2 = {0};
  int got = 0;
  size_t consumed = 0;
  fio_websocket_event_s ev;

  while (consumed < len && got < 2) {
    size_t c = fio_websocket_parse(
        &p, FIO_BUF_INFO2((char *)buf + consumed, len - consumed), &ev);
    WS_RT_CHECK(c != FIO_WEBSOCKET_PARSE_ERROR, "rt multi: parse error");
    if (c == 0)
      break;
    consumed += c;
    if (ev.type == FIO_WEBSOCKET_EV_DATA_CHUNK) {
      ws_rt_result_s *r = got ? &r2 : &r1;
      r->opcode = ev.opcode;
      r->is_text = ev.is_text;
      if (ev.payload.len &&
          r->payload_len + ev.payload.len <= sizeof(r->payload)) {
        memcpy(r->payload + r->payload_len, ev.payload.buf, ev.payload.len);
        r->payload_len += ev.payload.len;
      }
      ++got;
    }
  }

  WS_RT_CHECK(got == 2, "rt multi: two messages parsed");
  WS_RT_CHECK(r1.payload_len == 3 && !memcmp(r1.payload, "one", 3),
              "rt multi: first payload");
  WS_RT_CHECK(r2.payload_len == 3 && !memcmp(r2.payload, "two", 3),
              "rt multi: second payload");
}

int main(void) {
  test_roundtrip_text_server();
  test_roundtrip_text_client();
  test_roundtrip_binary_server();
  test_roundtrip_binary_client();
  test_roundtrip_ping_pong();
  test_roundtrip_close();
  test_roundtrip_medium_126();
  test_roundtrip_16bit_length();
  test_roundtrip_64bit_length();
  test_roundtrip_zero_payload();
  test_roundtrip_rsv1();
  test_roundtrip_multiple_messages();

  if (failures) {
    fprintf(stderr, "\n%d roundtrip test failure(s)\n", failures);
    return 1;
  }
  fprintf(stderr, "\nAll WebSocket roundtrip tests passed\n");
  return 0;
}
