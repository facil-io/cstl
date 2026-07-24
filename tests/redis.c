/* *****************************************************************************
Redis Module Correctness Tests

Fast, deterministic coverage for Redis helper behavior. Networked Redis and
multi-process pub/sub checks from ./tests-old/redis.c are intentionally left for
stress tests.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SOCK
#define FIO_TIME
#define FIO_THREADS
#define FIO_FIOBJ
#define FIO_PUBSUB
#define FIO_RESP3
#define FIO_REDIS
#include FIO_INCLUDE_FILE

static void assert_cstr_eq(fio_str_info_s actual,
                           const char *expected,
                           size_t expected_len,
                           const char *label) {
  FIO_ASSERT(actual.len == expected_len &&
                 !FIO_MEMCMP(actual.buf, expected, expected_len),
             "%s mismatch: expected %.*s, got %.*s",
             label,
             (int)expected_len,
             expected,
             (int)actual.len,
             actual.buf);
}

static void test_fiobj_command_to_resp(void) {
  fprintf(stderr, "* Testing Redis FIOBJ command serialization...\n");

  FIOBJ cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("SET", 3));
  fiobj_array_push(cmd, fiobj_str_new_cstr("key", 3));
  fiobj_array_push(cmd, fiobj_str_new_cstr("value", 5));

  FIOBJ resp = fiobj_str_new_buf(128);
  fio___redis_fiobj2resp(resp, cmd);
  assert_cstr_eq(fiobj2cstr(resp),
                 "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n",
                 FIO_STRLEN("*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n"),
                 "SET command RESP");

  fiobj_free(resp);
  fiobj_free(cmd);
}

static void test_primitive_to_resp(void) {
  fprintf(stderr, "* Testing Redis primitive serialization...\n");

  FIOBJ resp = fiobj_str_new_buf(128);
  fio___redis_fiobj2resp(resp, fiobj_null());
  fio___redis_fiobj2resp(resp, fiobj_true());
  fio___redis_fiobj2resp(resp, fiobj_false());
  fio___redis_fiobj2resp(resp, fiobj_num_new(42));

  assert_cstr_eq(fiobj2cstr(resp),
                 "$-1\r\n#t\r\n#f\r\n:42\r\n",
                 FIO_STRLEN("$-1\r\n#t\r\n#f\r\n:42\r\n"),
                 "primitive RESP");
  fiobj_free(resp);
}

static void test_pubsub_command_builder(void) {
  fprintf(stderr, "* Testing Redis PUBLISH command builder...\n");

  fio_buf_info_s channel = FIO_BUF_INFO1((char *)"chan");
  fio_buf_info_s message = FIO_BUF_INFO1((char *)"hello");
  fio_redis_cmd_s *cmd = fio___redis_publish_cmd_new(channel, message);
  FIO_ASSERT_ALLOC(cmd);
  FIO_ASSERT(cmd->cmd_len ==
                 FIO_STRLEN("*3\r\n$7\r\nPUBLISH\r\n$4\r\nchan\r\n$5\r\nhello\r\n"),
             "PUBLISH command length mismatch: %zu",
             cmd->cmd_len);
  FIO_ASSERT(!FIO_MEMCMP(cmd->cmd,
                         "*3\r\n$7\r\nPUBLISH\r\n$4\r\nchan\r\n$5\r\nhello\r\n",
                         cmd->cmd_len),
             "PUBLISH command bytes mismatch");
  FIO_MEM_FREE(cmd, sizeof(*cmd) + cmd->cmd_len);
  FIO_LEAK_COUNTER_ON_FREE(fio___redis_cmd);
}

static void test_resp3_reply_to_fiobj(void) {
  fprintf(stderr, "* Testing Redis RESP3 reply callbacks...\n");

  const char *wire = "*3\r\n$4\r\nPONG\r\n:7\r\n#t\r\n";
  fio_resp3_parser_s parser = {0};
  fio_resp3_result_s result = fio_resp3_parse(&parser,
                                              &FIO___REDIS_RESP3_CALLBACKS,
                                              wire,
                                              FIO_STRLEN(wire));
  FIO_ASSERT(!result.err, "RESP3 parser returned an error");
  FIO_ASSERT(result.consumed == FIO_STRLEN(wire),
             "RESP3 consumed mismatch: %zu",
             result.consumed);
  FIOBJ reply = (FIOBJ)result.obj;
  FIO_ASSERT(reply && FIOBJ_TYPE(reply) == FIOBJ_T_ARRAY,
             "RESP3 reply should become FIOBJ array");
  FIO_ASSERT(fiobj_array_count(reply) == 3,
             "RESP3 reply array count mismatch: %zu",
             fiobj_array_count(reply));
  assert_cstr_eq(fiobj2cstr(fiobj_array_get(reply, 0)), "PONG", 4, "PONG");
  FIO_ASSERT(fiobj2i(fiobj_array_get(reply, 1)) == 7,
             "RESP3 number mismatch");
  FIO_ASSERT(fiobj_array_get(reply, 2) == fiobj_true(),
             "RESP3 boolean mismatch");
  fiobj_free(reply);
}

static void test_nested_command_to_resp(void) {
  fprintf(stderr, "* Testing Redis nested command serialization...\n");

  FIOBJ inner = fiobj_array_new();
  fiobj_array_push(inner, fiobj_str_new_cstr("a", 1));
  fiobj_array_push(inner, fiobj_str_new_cstr("b", 1));

  FIOBJ cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("RPUSH", 5));
  fiobj_array_push(cmd, fiobj_str_new_cstr("list", 4));
  fiobj_array_push(cmd, inner);

  const char *expected =
      "*3\r\n$5\r\nRPUSH\r\n$4\r\nlist\r\n*2\r\n$1\r\na\r\n$1\r\nb\r\n";
  size_t expected_len = FIO_STRLEN(expected);

  size_t len = fio___redis_fiobj2resp_len(cmd, 0);
  FIO_ASSERT(len == expected_len,
             "nested RESP length mismatch: %zu vs %zu",
             len,
             expected_len);

  uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, len + 1, 0);
  FIO_ASSERT_ALLOC(buf);
  uint8_t *end = fio___redis_fiobj2resp_write(buf, cmd, 0);
  FIO_ASSERT((size_t)(end - buf) == len,
             "nested RESP write cursor mismatch");
  FIO_ASSERT(!FIO_MEMCMP(buf, expected, len),
             "nested RESP bytes mismatch: %.*s",
             (int)len,
             (char *)buf);

  FIO_MEM_FREE(buf, len + 1);
  fiobj_free(cmd);
}

static void test_resp_len_write_equivalence(void) {
  fprintf(stderr, "* Testing Redis RESP len/write/wrapper equivalence...\n");

  FIOBJ nested = fiobj_array_new();
  fiobj_array_push(nested, fiobj_str_new_cstr("a", 1));
  fiobj_array_push(nested, fiobj_num_new(7));

  FIOBJ hash = fiobj_hash_new();
  FIOBJ hkey = fiobj_str_new_cstr("k", 1);
  fiobj_hash_set(hash, hkey, fiobj_num_new(42), NULL);
  fiobj_free(hkey);

  FIOBJ cmd = fiobj_array_new();
  fiobj_array_push(cmd, fiobj_str_new_cstr("SET", 3));
  fiobj_array_push(cmd, fiobj_num_new(-42));
  fiobj_array_push(cmd, fiobj_float_new(1.5));
  fiobj_array_push(cmd, fiobj_true());
  fiobj_array_push(cmd, fiobj_null());
  fiobj_array_push(cmd, nested);
  fiobj_array_push(cmd, hash);

  size_t len = fio___redis_fiobj2resp_len(cmd, 0);
  FIO_ASSERT(len > 0, "RESP length should be non-zero");

  uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, len + 1, 0);
  FIO_ASSERT_ALLOC(buf);
  uint8_t *end = fio___redis_fiobj2resp_write(buf, cmd, 0);
  FIO_ASSERT((size_t)(end - buf) == len,
             "RESP len/write mismatch: %zu vs %zu",
             len,
             (size_t)(end - buf));

  FIOBJ resp = fiobj_str_new_buf(len);
  fio___redis_fiobj2resp(resp, cmd);
  assert_cstr_eq(fiobj2cstr(resp), (const char *)buf, len, "cursor vs wrapper");

  FIO_MEM_FREE(buf, len + 1);
  fiobj_free(resp);
  fiobj_free(cmd);
}

static void test_resp_depth_guard(void) {
  fprintf(stderr, "* Testing Redis RESP serialization depth guard...\n");

  FIOBJ deep = fiobj_array_new();
  FIOBJ cur = deep;
  for (size_t i = 0; i < 40; ++i) {
    FIOBJ next = fiobj_array_new();
    fiobj_array_push(cur, next);
    cur = next;
  }
  FIO_ASSERT(fio___redis_fiobj2resp_len(deep, 0) == 0,
             "depth guard should reject 40-deep nesting");
  fiobj_free(deep);

  FIOBJ ok = fiobj_array_new();
  cur = ok;
  for (size_t i = 0; i < 8; ++i) {
    FIOBJ next = fiobj_array_new();
    fiobj_array_push(cur, next);
    cur = next;
  }
  FIO_ASSERT(fio___redis_fiobj2resp_len(ok, 0) > 0,
             "8-deep nesting should serialize fine");
  fiobj_free(ok);
}

static void test_subscribe_command_builder(void) {
  fprintf(stderr, "* Testing Redis subscribe command builder...\n");

  uint8_t buf[512];
  const char *expected;
  size_t len;

  expected = "*2\r\n$9\r\nSUBSCRIBE\r\n$4\r\nchan\r\n";
  len = fio___redis_write_sub_cmd(buf,
                                  "SUBSCRIBE",
                                  9,
                                  FIO_BUF_INFO1((char *)"chan"));
  FIO_ASSERT(len == FIO_STRLEN(expected) && !FIO_MEMCMP(buf, expected, len),
             "SUBSCRIBE command mismatch: %.*s",
             (int)len,
             (char *)buf);

  expected = "*2\r\n$10\r\nPSUBSCRIBE\r\n$3\r\np.*\r\n";
  len = fio___redis_write_sub_cmd(buf,
                                  "PSUBSCRIBE",
                                  10,
                                  FIO_BUF_INFO1((char *)"p.*"));
  FIO_ASSERT(len == FIO_STRLEN(expected) && !FIO_MEMCMP(buf, expected, len),
             "PSUBSCRIBE command mismatch: %.*s",
             (int)len,
             (char *)buf);

  expected = "*2\r\n$11\r\nUNSUBSCRIBE\r\n$4\r\nchan\r\n";
  len = fio___redis_write_sub_cmd(buf,
                                  "UNSUBSCRIBE",
                                  11,
                                  FIO_BUF_INFO1((char *)"chan"));
  FIO_ASSERT(len == FIO_STRLEN(expected) && !FIO_MEMCMP(buf, expected, len),
             "UNSUBSCRIBE command mismatch: %.*s",
             (int)len,
             (char *)buf);

  expected = "*2\r\n$12\r\nPUNSUBSCRIBE\r\n$3\r\np.*\r\n";
  len = fio___redis_write_sub_cmd(buf,
                                  "PUNSUBSCRIBE",
                                  12,
                                  FIO_BUF_INFO1((char *)"p.*"));
  FIO_ASSERT(len == FIO_STRLEN(expected) && !FIO_MEMCMP(buf, expected, len),
             "PUNSUBSCRIBE command mismatch: %.*s",
             (int)len,
             (char *)buf);

  /* Long channel: exact length accounting (64-byte overhead constant) */
  char long_ch[200];
  FIO_MEMSET(long_ch, 'x', sizeof(long_ch));
  len = fio___redis_write_sub_cmd(buf,
                                  "SUBSCRIBE",
                                  9,
                                  FIO_BUF_INFO2(long_ch, sizeof(long_ch)));
  FIO_ASSERT(len == 227 && len <= FIO___REDIS_SUB_CMD_OVERHEAD +
                                     sizeof(long_ch),
             "long-channel SUBSCRIBE length mismatch: %zu",
             len);
}

/* *****************************************************************************
Payload Budget Tests (fio___redis_parse_state_s)
***************************************************************************** */

/** Builds a `$<len>\r\n<pattern bytes>\r\n` wire image; returns total length. */
static size_t redis_test_make_blob_wire(uint8_t **out, size_t blob_len) {
  char hdr[32];
  int hpos = 0;
  hdr[hpos++] = '$';
  hpos += fio_ltoa(hdr + hpos, (int64_t)blob_len, 10);
  hdr[hpos++] = '\r';
  hdr[hpos++] = '\n';
  size_t total = (size_t)hpos + blob_len + 2;
  uint8_t *wire = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(wire);
  FIO_MEMCPY(wire, hdr, (size_t)hpos);
  for (size_t i = 0; i < blob_len; ++i)
    wire[(size_t)hpos + i] = (uint8_t)((i * 131) + 7);
  wire[(size_t)hpos + blob_len] = '\r';
  wire[(size_t)hpos + blob_len + 1] = '\n';
  *out = wire;
  return total;
}

/** Frees any partial parser state (mirrors fio___redis_connection_reset). */
static void redis_test_parser_cleanup(fio_resp3_parser_s *parser) {
  if (parser->streaming_string && parser->streaming_string_ctx &&
      parser->streaming_string_ctx != FIO___REDIS_PS_SENTINEL)
    fiobj_free((FIOBJ)parser->streaming_string_ctx);
  while (parser->depth > 0) {
    fio_resp3_frame_s *f = &parser->stack[parser->depth - 1];
    if (f->key)
      fiobj_free((FIOBJ)f->key);
    if (f->ctx)
      fiobj_free((FIOBJ)f->ctx);
    --parser->depth;
  }
}

static void test_payload_limit_oversize_declared(void) {
  fprintf(stderr, "* Testing Redis payload_limit oversize declaration...\n");

  /* A server-declared 17MB blob exceeds the 16MB default budget BEFORE any
   * allocation: the limit flag is set at the blob header, and the first
   * data write aborts the parse (sentinel ctx fails on_string_write). */
  fio___redis_parse_state_s ps = {
      .payload_limit = FIO___REDIS_DEFAULT_PAYLOAD_LIMIT};
  fio_resp3_parser_s parser = {.udata = &ps};

  const char *hdr = "$17825792\r\n"; /* 17MB > 16MB default */
  fio_resp3_result_s r = fio_resp3_parse(&parser,
                                         &FIO___REDIS_RESP3_CALLBACKS,
                                         hdr,
                                         FIO_STRLEN(hdr));
  FIO_ASSERT(!r.err, "header-only parse should not error yet");
  FIO_ASSERT(r.obj == NULL, "header-only parse should not complete");
  FIO_ASSERT(ps.limit_exceeded,
             "oversize declaration must breach at the header (no allocation)");
  FIO_ASSERT(parser.streaming_string_ctx == FIO___REDIS_PS_SENTINEL,
             "breach should yield the sentinel string context");

  /* First data byte: sentinel write fails -> parser error */
  r = fio_resp3_parse(&parser, &FIO___REDIS_RESP3_CALLBACKS, "x", 1);
  FIO_ASSERT(r.err, "sentinel write must abort the parse");
  redis_test_parser_cleanup(&parser);
}

static void test_payload_limit_cumulative_strings(void) {
  fprintf(stderr, "* Testing Redis payload_limit cumulative strings...\n");

  /* Two 9MB strings in one array = 18MB > 16MB budget: the FIRST string
   * streams in fully (9MB < 16MB); the breach hits at the SECOND string's
   * header (cumulative accounting), before its data is read. */
  enum { NINE_MB = 9 << 20, CHUNK = 1 << 16 };
  fio___redis_parse_state_s ps = {
      .payload_limit = FIO___REDIS_DEFAULT_PAYLOAD_LIMIT};
  fio_resp3_parser_s parser = {.udata = &ps};

  uint8_t *s1 = NULL;
  size_t s1_len = redis_test_make_blob_wire(&s1, NINE_MB);
  (void)s1_len; /* used by FIO_MEM_FREE in debug builds only */
  const char *prefix = "*2\r\n";
  const char *hdr2 = "$9437184\r\n"; /* second 9MB declaration */

  /* Feed array header + first blob header (`$9437184\r\n` = 10 bytes) */
  uint8_t *head = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, 64, 0);
  FIO_ASSERT_ALLOC(head);
  FIO_MEMCPY(head, prefix, FIO_STRLEN(prefix));
  FIO_MEMCPY(head + FIO_STRLEN(prefix), s1, 10);
  size_t head_len = FIO_STRLEN(prefix) + 10;
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &FIO___REDIS_RESP3_CALLBACKS, head, head_len);
  FIO_ASSERT(!r.err && r.consumed == head_len, "array+blob header parse");
  FIO_ASSERT(!ps.limit_exceeded, "first 9MB string fits the budget");
  FIO_ASSERT(ps.msg_total == 32 + 32 + NINE_MB,
             "first string charge mismatch: %zu",
             ps.msg_total);
  FIO_MEM_FREE(head, 64);

  /* Stream the first blob's 9MB of data + CRLF in 64KB chunks */
  size_t data_off = 10;
  size_t fed = 0;
  while (fed < NINE_MB + 2) {
    size_t step = NINE_MB + 2 - fed;
    if (step > CHUNK)
      step = CHUNK;
    r = fio_resp3_parse(&parser,
                        &FIO___REDIS_RESP3_CALLBACKS,
                        s1 + data_off + fed,
                        step);
    FIO_ASSERT(!r.err, "first blob stream error");
    FIO_ASSERT(r.consumed == step, "first blob should drain per chunk");
    FIO_ASSERT(!ps.limit_exceeded, "first string stays within budget");
    fed += step;
  }
  FIO_ASSERT(r.obj == NULL, "array needs its second element");
  FIO_MEM_FREE(s1, s1_len);

  /* Second string's header: 9437248 + 32 + 9437184 > 16MB -> breach */
  r = fio_resp3_parse(&parser,
                      &FIO___REDIS_RESP3_CALLBACKS,
                      hdr2,
                      FIO_STRLEN(hdr2));
  FIO_ASSERT(!r.err, "second header parses (sentinel, not error)");
  FIO_ASSERT(ps.limit_exceeded,
             "cumulative 18MB must breach at the second header");

  /* First data byte of the second blob aborts the parse */
  r = fio_resp3_parse(&parser, &FIO___REDIS_RESP3_CALLBACKS, "x", 1);
  FIO_ASSERT(r.err, "sentinel write must abort the parse");
  redis_test_parser_cleanup(&parser);
}

static void test_payload_limit_ok_and_reset(void) {
  fprintf(stderr, "* Testing Redis payload_limit OK + per-message reset...\n");

  fio___redis_parse_state_s ps = {
      .payload_limit = FIO___REDIS_DEFAULT_PAYLOAD_LIMIT};
  fio_resp3_parser_s parser = {.udata = &ps};

  /* 1MB string is well within the 16MB budget (streamed in chunks) */
  enum { ONE_MB = 1 << 20, CHUNK = 1 << 16 };
  uint8_t *wire = NULL;
  size_t wire_len = redis_test_make_blob_wire(&wire, ONE_MB);
  size_t data_off = 0;
  while (wire[data_off] != '\r')
    ++data_off;
  data_off += 2;

  size_t fed = 0;
  fio_resp3_result_s r = {0};
  while (fed < wire_len) {
    size_t step = wire_len - fed;
    if (step > CHUNK)
      step = CHUNK;
    r = fio_resp3_parse(&parser,
                        &FIO___REDIS_RESP3_CALLBACKS,
                        wire + fed,
                        step);
    FIO_ASSERT(!r.err, "1MB parse error");
    fed += step;
  }
  FIO_ASSERT(r.obj != NULL, "1MB string should complete");
  FIO_ASSERT(!ps.limit_exceeded, "1MB fits the 16MB budget");
  FIO_ASSERT(ps.msg_total == 32 + ONE_MB,
             "1MB charge mismatch: %zu",
             ps.msg_total);
  FIOBJ msg = (FIOBJ)r.obj;
  fio_str_info_s s = fiobj2cstr(msg);
  FIO_ASSERT(s.len == ONE_MB &&
                 !FIO_MEMCMP(s.buf, wire + data_off, ONE_MB),
             "1MB content mismatch");
  fiobj_free(msg);
  FIO_MEM_FREE(wire, wire_len);

  /* Per-message reset: two independent 600KB strings (1.2MB total) are each
   * within budget ONLY if msg_total resets between top-level messages
   * (the parse loop resets on result.obj; simulated here). */
  enum { K600 = 600 << 10 };
  ps.msg_total = 0;
  ps.limit_exceeded = 0;
  parser = (fio_resp3_parser_s){.udata = &ps};

  uint8_t *w1 = NULL, *w2 = NULL;
  size_t w1_len = redis_test_make_blob_wire(&w1, K600);
  size_t w2_len = redis_test_make_blob_wire(&w2, K600);

  r = fio_resp3_parse(&parser, &FIO___REDIS_RESP3_CALLBACKS, w1, w1_len);
  FIO_ASSERT(!r.err && r.obj && !ps.limit_exceeded, "first 600KB OK");
  fiobj_free((FIOBJ)r.obj);
  ps.msg_total = 0; /* fio___redis_parse_buffered resets on result.obj */
  r = fio_resp3_parse(&parser, &FIO___REDIS_RESP3_CALLBACKS, w2, w2_len);
  FIO_ASSERT(!r.err && r.obj && !ps.limit_exceeded,
             "second 600KB OK after reset");
  fiobj_free((FIOBJ)r.obj);

  /* Without the reset, the same two messages WOULD breach (1.2MB > 1MB) */
  ps.msg_total = 0;
  ps.limit_exceeded = 0;
  ps.payload_limit = 1 << 20;
  parser = (fio_resp3_parser_s){.udata = &ps};
  r = fio_resp3_parse(&parser, &FIO___REDIS_RESP3_CALLBACKS, w1, w1_len);
  FIO_ASSERT(!r.err && r.obj && !ps.limit_exceeded, "first 600KB OK (1MB)");
  fiobj_free((FIOBJ)r.obj);
  /* no reset - cumulative charge carries into the next message */
  r = fio_resp3_parse(&parser, &FIO___REDIS_RESP3_CALLBACKS, w2, w2_len);
  FIO_ASSERT(ps.limit_exceeded,
             "without reset, 1.2MB cumulative must breach a 1MB limit");
  fiobj_free((FIOBJ)r.obj);
  redis_test_parser_cleanup(&parser);
  FIO_MEM_FREE(w1, w1_len);
  FIO_MEM_FREE(w2, w2_len);
}

static void test_payload_limit_container_overhead(void) {
  fprintf(stderr, "* Testing Redis payload_limit container overhead...\n");

  /* Every object costs 32 bytes of budget: an array of small numbers must
   * breach a small limit through overhead alone (no big strings). */
  fio___redis_parse_state_s ps = {.payload_limit = 1024};
  fio_resp3_parser_s parser = {.udata = &ps};

  /* *40\r\n + 40 x :1\r\n = 41 objects x 32 = 1312 > 1024 */
  uint8_t wire[8 + 40 * 4];
  uint8_t *pos = wire;
  FIO_MEMCPY(pos, "*40\r\n", 5);
  pos += 5;
  for (int i = 0; i < 40; ++i) {
    FIO_MEMCPY(pos, ":1\r\n", 4);
    pos += 4;
  }
  size_t wire_len = (size_t)(pos - wire);

  fio_resp3_result_s r = fio_resp3_parse(&parser,
                                         &FIO___REDIS_RESP3_CALLBACKS,
                                         wire,
                                         wire_len);
  FIO_ASSERT(!r.err, "container overhead: parse completes (sticky breach)");
  FIO_ASSERT(ps.limit_exceeded,
             "41 objects x 32 = 1312 must breach a 1024 budget");
  FIO_ASSERT(r.obj != NULL, "array still built (sticky closes post-parse)");
  FIO_ASSERT(ps.msg_total == 1024,
             "charging stops at the budget boundary: %zu",
             ps.msg_total);
  fiobj_free((FIOBJ)r.obj);
  redis_test_parser_cleanup(&parser);
}

/* *****************************************************************************
RESP3 Redesign Tests (HELLO handshake, frame routing, resubscribe batch)
***************************************************************************** */

static void test_hello_command_builder(void) {
  fprintf(stderr, "* Testing Redis HELLO 3 command builder...\n");

  uint8_t buf[128];
  size_t len;

  /* AUTH-less variant */
  const char *expected = "*3\r\n$5\r\nHELLO\r\n$1\r\n3\r\n";
  FIO_ASSERT(fio___redis_hello_cmd_len(0) == FIO_STRLEN(expected),
             "HELLO no-auth length mismatch");
  len = fio___redis_write_hello_cmd(buf, NULL, 0);
  FIO_ASSERT(len == FIO_STRLEN(expected) && !FIO_MEMCMP(buf, expected, len),
             "HELLO no-auth bytes mismatch: %.*s",
             (int)len,
             (char *)buf);

  /* HELLO-AUTH variant (auth folds in, user 'default') */
  expected = "*5\r\n$5\r\nHELLO\r\n$1\r\n3\r\n$4\r\nAUTH\r\n$7\r\ndefault\r\n"
             "$6\r\nsecret\r\n";
  FIO_ASSERT(fio___redis_hello_cmd_len(6) == FIO_STRLEN(expected),
             "HELLO auth length mismatch: %zu",
             fio___redis_hello_cmd_len(6));
  len = fio___redis_write_hello_cmd(buf, "secret", 6);
  FIO_ASSERT(len == FIO_STRLEN(expected) && !FIO_MEMCMP(buf, expected, len),
             "HELLO auth bytes mismatch: %.*s",
             (int)len,
             (char *)buf);
}

static void test_hello_reply_routing(void) {
  fprintf(stderr, "* Testing Redis HELLO reply consumption...\n");

  fio___redis_parse_state_s ps = {
      .payload_limit = FIO___REDIS_DEFAULT_PAYLOAD_LIMIT};
  fio_resp3_parser_s parser = {.udata = &ps};

  /* HELLO map reply: parsed as a hash, routed as a command reply (not push) */
  const char *wire = "%2\r\n$6\r\nserver\r\n$5\r\nredis\r\n$7\r\nversion\r\n$5\r\n7.2.0\r\n";
  fio_resp3_result_s r = fio_resp3_parse(&parser,
                                         &FIO___REDIS_RESP3_CALLBACKS,
                                         wire,
                                         FIO_STRLEN(wire));
  FIO_ASSERT(!r.err && r.consumed == FIO_STRLEN(wire), "HELLO map parse");
  FIO_ASSERT(r.obj && FIOBJ_TYPE((FIOBJ)r.obj) == FIOBJ_T_HASH,
             "HELLO map reply should become a FIOBJ hash");
  FIO_ASSERT(!ps.is_push, "HELLO map reply is a reply, not a push frame");
  FIOBJ hello_reply = (FIOBJ)r.obj;

  /* The HELLO callback accepts a hash reply - engine keeps running */
  fio_redis_engine_s engine = {0};
  engine.running = 1;
  fio___redis_on_hello_reply(&engine.engine, hello_reply, NULL);
  FIO_ASSERT(engine.running == 1, "HELLO success must keep the engine up");
  fiobj_free(hello_reply);

  /* HELLO failure (-ERR, e.g. Redis < 6.0): a string, NOT a hash -> hard
   * engine error (running cleared, no RESP2 fallback) */
  ps.msg_total = 0;
  parser = (fio_resp3_parser_s){.udata = &ps};
  const char *err_wire = "-ERR unknown command 'HELLO'\r\n";
  r = fio_resp3_parse(&parser,
                      &FIO___REDIS_RESP3_CALLBACKS,
                      err_wire,
                      FIO_STRLEN(err_wire));
  FIO_ASSERT(!r.err && r.obj, "HELLO -ERR parse");
  FIO_ASSERT(FIOBJ_TYPE((FIOBJ)r.obj) != FIOBJ_T_HASH,
             "HELLO failure reply is not a hash");
  FIO_ASSERT(!ps.is_push, "HELLO -ERR is a reply, not a push frame");
  fio___redis_on_hello_reply(&engine.engine, (FIOBJ)r.obj, NULL);
  FIO_ASSERT(engine.running == 0,
             "HELLO failure must be a hard engine error");
  fiobj_free((FIOBJ)r.obj);

  /* Disconnected reply (FIOBJ_INVALID) is also a hard error */
  engine.running = 1;
  fio___redis_on_hello_reply(&engine.engine, FIOBJ_INVALID, NULL);
  FIO_ASSERT(engine.running == 0, "HELLO INVALID reply -> hard error");
}

static void test_push_reply_frame_routing(void) {
  fprintf(stderr, "* Testing Redis push/reply frame routing...\n");

  fio___redis_parse_state_s ps = {
      .payload_limit = FIO___REDIS_DEFAULT_PAYLOAD_LIMIT};
  fio_resp3_parser_s parser = {.udata = &ps};

  /* Interleaved push frame + `:1\r\n` command reply (lock-step): the push
   * frame must be flagged, the reply must NOT be. (Confirmation pushes are
   * used: they never publish, so no engine state is required.) */
  const char *wire = ">3\r\n$9\r\nsubscribe\r\n$4\r\nchan\r\n:1\r\n:7\r\n";
  fio_resp3_result_s r = fio_resp3_parse(&parser,
                                         &FIO___REDIS_RESP3_CALLBACKS,
                                         wire,
                                         FIO_STRLEN(wire));
  FIO_ASSERT(!r.err, "push parse error");
  FIO_ASSERT(ps.is_push, "push frame must be flagged as push");
  FIO_ASSERT(r.obj == (void *)fiobj_null(),
             "captured push frame yields fiobj_null (nothing built)");
  size_t consumed1 = r.consumed;
  fiobj_free((FIOBJ)r.obj);
  ps.msg_total = 0; /* parse loop per-message reset */
  ps.is_push = 0;
  fio___redis_capture_reset(&ps); /* parse loop capture reset (push path) */

  r = fio_resp3_parse(&parser,
                      &FIO___REDIS_RESP3_CALLBACKS,
                      wire + consumed1,
                      FIO_STRLEN(wire) - consumed1);
  FIO_ASSERT(!r.err, "reply parse error");
  FIO_ASSERT(!ps.is_push, "`:7` reply must NOT be flagged as push");
  FIO_ASSERT(r.obj && FIOBJ_TYPE((FIOBJ)r.obj) == FIOBJ_T_NUMBER &&
                 fiobj2i((FIOBJ)r.obj) == 7,
             "reply should be the number 7");
  fiobj_free((FIOBJ)r.obj);

  /* Reverse order: reply first, then push */
  ps.msg_total = 0;
  ps.is_push = 0;
  parser = (fio_resp3_parser_s){.udata = &ps};
  const char *wire2 = ":1\r\n>3\r\n$9\r\nsubscribe\r\n$4\r\nchan\r\n:1\r\n";
  r = fio_resp3_parse(&parser,
                      &FIO___REDIS_RESP3_CALLBACKS,
                      wire2,
                      FIO_STRLEN(wire2));
  FIO_ASSERT(!r.err && !ps.is_push && r.obj, "reply-first parse");
  FIO_ASSERT(FIOBJ_TYPE((FIOBJ)r.obj) == FIOBJ_T_NUMBER,
             "reply-first should be a number");
  fiobj_free((FIOBJ)r.obj);
  ps.msg_total = 0;
  ps.is_push = 0;
  size_t consumed2 = r.consumed;
  r = fio_resp3_parse(&parser,
                      &FIO___REDIS_RESP3_CALLBACKS,
                      wire2 + consumed2,
                      FIO_STRLEN(wire2) - consumed2);
  FIO_ASSERT(!r.err && ps.is_push && r.obj == (void *)fiobj_null(),
             "push after reply must be flagged (and captured)");
  fiobj_free((FIOBJ)r.obj);
  fio___redis_capture_reset(&ps);
}

static void test_push_confirmation_ignored(void) {
  fprintf(stderr, "* Testing Redis push confirmation handling...\n");

  fio___redis_parse_state_s ps = {
      .payload_limit = FIO___REDIS_DEFAULT_PAYLOAD_LIMIT};
  fio_resp3_parser_s parser = {.udata = &ps};
  fio_redis_engine_s engine = {0};
  (void)engine;

  /* Subscribe confirmation push: flagged as push (consumed by the push
   * path), never a command reply. */
  const char *conf = ">3\r\n$9\r\nsubscribe\r\n$4\r\nchan\r\n:1\r\n";
  fio_resp3_result_s r = fio_resp3_parse(&parser,
                                         &FIO___REDIS_RESP3_CALLBACKS,
                                         conf,
                                         FIO_STRLEN(conf));
  FIO_ASSERT(!r.err && ps.is_push && r.obj, "confirmation parse");
  fiobj_free((FIOBJ)r.obj);
}

static void test_resubscribe_batch_bytes(void) {
  fprintf(stderr, "* Testing Redis resubscribe batch bytes...\n");

  /* Populate the pub/sub channel maps directly (synchronous map API) */
  fio_str_info_s ckey =
      FIO_STR_INFO3((char *)"achan", 5, FIO___PUBSUB_CHANNEL_ENCODE_CAPA(0, 0));
  fio_str_info_s pkey =
      FIO_STR_INFO3((char *)"p.*", 3, FIO___PUBSUB_CHANNEL_ENCODE_CAPA(0, 1));
  fio_pubsub_channel_s **cp = fio___pubsub_channel_map_node2key_ptr(
      fio___pubsub_channel_map_set_ptr(&FIO___PUBSUB_POSTOFFICE.channels,
                                       ckey));
  fio_pubsub_channel_s **pp = fio___pubsub_channel_map_node2key_ptr(
      fio___pubsub_channel_map_set_ptr(&FIO___PUBSUB_POSTOFFICE.patterns,
                                       pkey));
  FIO_ASSERT(cp && *cp && pp && *pp, "channel map insertion failed");

  /* The batch is channels first, then patterns, in one buffer */
  const char *expected = "*2\r\n$9\r\nSUBSCRIBE\r\n$5\r\nachan\r\n"
                         "*2\r\n$10\r\nPSUBSCRIBE\r\n$3\r\np.*\r\n";
  size_t expected_len = FIO_STRLEN(expected);

  size_t total = fio___redis_resubscribe_size();
  FIO_ASSERT(total >= expected_len,
             "resubscribe batch capacity too small: %zu vs %zu",
             total,
             expected_len);

  uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  size_t written = fio___redis_resubscribe_write(buf);
  FIO_ASSERT(written == expected_len && !FIO_MEMCMP(buf, expected, written),
             "resubscribe batch bytes mismatch: %.*s",
             (int)written,
             (char *)buf);
  FIO_MEM_FREE(buf, total);

  /* Cleanup: remove from the maps (destroys the channels) */
  fio___pubsub_channel_map_remove(&FIO___PUBSUB_POSTOFFICE.channels,
                                  ckey,
                                  NULL);
  fio___pubsub_channel_map_remove(&FIO___PUBSUB_POSTOFFICE.patterns,
                                  pkey,
                                  NULL);
  FIO_ASSERT(fio___redis_resubscribe_size() == 0,
             "channel maps should be empty after removal");
}

/* *****************************************************************************
Zero-Copy Push Capture Tests
***************************************************************************** */

/** Publish observation log (hook replaces fio_pubsub_publish in tests). */
#define PUB_LOG_MAX 8
static struct {
  size_t count;
  const uint8_t *ch_ptr[PUB_LOG_MAX], *msg_ptr[PUB_LOG_MAX];
  size_t ch_len[PUB_LOG_MAX], msg_len[PUB_LOG_MAX];
  uint8_t ch_copy[PUB_LOG_MAX][64];
  uint8_t msg_copy[PUB_LOG_MAX][128];
  uint8_t *big_copy; /* heap copy of the last message (big-payload checks) */
  size_t big_len;
} pub_log;

static void pub_log_reset(void) {
  if (pub_log.big_copy)
    FIO_MEM_FREE(pub_log.big_copy, pub_log.big_len);
  FIO_MEMSET(&pub_log, 0, sizeof(pub_log));
}

static void test_publish_hook(void *udata,
                              fio_buf_info_s channel,
                              fio_buf_info_s message) {
  (void)udata;
  size_t i = pub_log.count;
  ++pub_log.count;
  if (i < PUB_LOG_MAX) {
    pub_log.ch_ptr[i] = (const uint8_t *)channel.buf;
    pub_log.msg_ptr[i] = (const uint8_t *)message.buf;
    pub_log.ch_len[i] = channel.len;
    pub_log.msg_len[i] = message.len;
    if (channel.len <= sizeof(pub_log.ch_copy[i]))
      FIO_MEMCPY(pub_log.ch_copy[i], channel.buf, channel.len);
    if (message.len <= sizeof(pub_log.msg_copy[i]))
      FIO_MEMCPY(pub_log.msg_copy[i], message.buf, message.len);
  }
  /* Heap-copy any message (validates content after temp buffers are freed) */
  if (pub_log.big_copy)
    FIO_MEM_FREE(pub_log.big_copy, pub_log.big_len);
  pub_log.big_copy = (uint8_t *)FIO_MEM_REALLOC(NULL, 0,
                                                message.len ? message.len : 1,
                                                0);
  FIO_ASSERT_ALLOC(pub_log.big_copy);
  FIO_MEMCPY(pub_log.big_copy, message.buf, message.len);
  pub_log.big_len = message.len;
}

/** Fake engine for wire-level tests (real FIO_REF allocation, no reactor). */
static fio_redis_engine_s *test_redis_engine_new(void) {
  fio_redis_engine_s *r = fio___redis_new(0);
  FIO_ASSERT_ALLOC(r);
  FIO_LEAK_COUNTER_ON_ALLOC(fio___redis_engine);
  *r = (fio_redis_engine_s){
      .cmd_queue = FIO_LIST_INIT(r->cmd_queue),
      .running = 1,
  };
  r->conn.ps.payload_limit = FIO___REDIS_DEFAULT_PAYLOAD_LIMIT;
  r->conn.parser.udata = &r->conn.ps;
  r->conn.ps.publish_hook = test_publish_hook;
  return r;
}

/** Feeds bytes through the real parse loop (compaction included). */
static void test_redis_feed(fio_redis_engine_s *r,
                            const void *data,
                            size_t len) {
  FIO_ASSERT(len <= FIO_REDIS_READ_BUFFER - r->conn.buf_pos,
             "test feed overflow");
  FIO_MEMCPY(r->buf + r->conn.buf_pos, data, len);
  r->conn.buf_pos += (FIO___REDIS_BUF_POS_T)len;
  fio___redis_parse_buffered(r, &r->conn, r->buf, NULL);
}

static int test_ptr_in_buf(fio_redis_engine_s *r, const uint8_t *ptr) {
  return ptr >= r->buf && ptr < r->buf + FIO_REDIS_READ_BUFFER;
}

static void test_push_capture_zero_copy(void) {
  fprintf(stderr, "* Testing Redis zero-copy push capture...\n");
  pub_log_reset();
  fio_redis_engine_s *r = test_redis_engine_new();

  const char *wire = ">3\r\n$7\r\nmessage\r\n$4\r\nchan\r\n$5\r\nhello\r\n";
  test_redis_feed(r, wire, FIO_STRLEN(wire));

  FIO_ASSERT(pub_log.count == 1, "publish count: %zu", pub_log.count);
  FIO_ASSERT(pub_log.ch_len[0] == 4 &&
                 !FIO_MEMCMP(pub_log.ch_ptr[0], "chan", 4),
             "channel content mismatch");
  FIO_ASSERT(pub_log.msg_len[0] == 5 &&
                 !FIO_MEMCMP(pub_log.msg_ptr[0], "hello", 5),
             "payload content mismatch");
  /* Pointer-equality: channel + payload were published DIRECTLY from the
   * connection read buffer (zero copies, no allocation). */
  FIO_ASSERT(pub_log.ch_ptr[0] == r->buf + 21,
             "channel not zero-copy (expected buf+21)");
  FIO_ASSERT(pub_log.msg_ptr[0] == r->buf + 31,
             "payload not zero-copy (expected buf+31)");
  /* last_channel: owned bytes, updated on message */
  FIO_ASSERT(r->last_channel_len == 4 &&
                 !FIO_MEMCMP(r->last_channel, "chan", 4),
             "last_channel mismatch");
  FIO_ASSERT(r->conn.ps.msg_total == 0, "budget reset after frame");
  FIO_ASSERT(!r->conn.ps.capture, "capture state reset after frame");

  pub_log_reset();
  fio___redis_free(r);
}

static void test_push_capture_split_payload(void) {
  fprintf(stderr, "* Testing Redis push capture across split reads...\n");
  pub_log_reset();
  fio_redis_engine_s *r = test_redis_engine_new();

  /* Split mid-payload: the channel is captured in the first event (zero-copy
   * read-buffer pointer), then the buffer compacts - the freeze step must
   * move the channel into owned storage before the memmove. */
  const char *part1 = ">3\r\n$7\r\nmessage\r\n$4\r\nchan\r\n$5\r\nhe";
  const char *part2 = "llo\r\n";
  test_redis_feed(r, part1, FIO_STRLEN(part1));
  FIO_ASSERT(pub_log.count == 0, "no publish before payload completes");
  FIO_ASSERT(r->conn.ps.capture, "capture still active mid-frame");

  test_redis_feed(r, part2, FIO_STRLEN(part2));
  FIO_ASSERT(pub_log.count == 1, "publish count: %zu", pub_log.count);
  FIO_ASSERT(pub_log.ch_len[0] == 4 &&
                 !FIO_MEMCMP(pub_log.ch_copy[0], "chan", 4),
             "channel content mismatch after freeze");
  FIO_ASSERT(pub_log.msg_len[0] == 5 &&
                 !FIO_MEMCMP(pub_log.msg_copy[0], "hello", 5),
             "payload content mismatch");
  /* The channel pointer was frozen (owned), NOT a stale read-buffer ptr */
  FIO_ASSERT(!test_ptr_in_buf(r, pub_log.ch_ptr[0]),
             "channel pointer must have been frozen out of the read buffer");
  /* The payload completed contiguously in the second event: zero-copy */
  FIO_ASSERT(test_ptr_in_buf(r, pub_log.msg_ptr[0]),
             "payload should be zero-copy from the read buffer");

  pub_log_reset();
  fio___redis_free(r);
}

static void test_push_capture_temp_1mb(void) {
  fprintf(stderr, "* Testing Redis push capture 1MB temp assembly...\n");
  pub_log_reset();
  fio_redis_engine_s *r = test_redis_engine_new();

  enum { PAYLOAD = 1 << 20, CHUNK = 1 << 14 };
  const char *hdr = ">3\r\n$7\r\nmessage\r\n$4\r\nchan\r\n$1048576\r\n";
  test_redis_feed(r, hdr, FIO_STRLEN(hdr));
  FIO_ASSERT(pub_log.count == 0, "no publish before payload");

  /* Stream the 1MB payload in 64 x 16KB chunks (each is a separate event) */
  uint8_t *chunk = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, CHUNK, 0);
  FIO_ASSERT_ALLOC(chunk);
  for (size_t i = 0; i < PAYLOAD / CHUNK; ++i) {
    for (size_t j = 0; j < CHUNK; ++j)
      chunk[j] = (uint8_t)(((i * CHUNK + j) * 131) + 7);
    test_redis_feed(r, chunk, CHUNK);
    FIO_ASSERT(pub_log.count == 0, "no publish mid-payload");
  }
  FIO_MEM_FREE(chunk, CHUNK);
  FIO_ASSERT(r->conn.ps.cur.temp != NULL,
             "large split payload must assemble into a temp buffer");

  test_redis_feed(r, "\r\n", 2);
  FIO_ASSERT(pub_log.count == 1, "publish count: %zu", pub_log.count);
  FIO_ASSERT(pub_log.ch_len[0] == 4 &&
                 !FIO_MEMCMP(pub_log.ch_copy[0], "chan", 4),
             "channel mismatch");
  FIO_ASSERT(pub_log.msg_len[0] == PAYLOAD,
             "payload length mismatch: %zu",
             pub_log.msg_len[0]);
  /* Temp buffer was published (not the 32KB read buffer), then freed */
  FIO_ASSERT(!test_ptr_in_buf(r, pub_log.msg_ptr[0]),
             "1MB payload must publish from the temp buffer");
  FIO_ASSERT(r->conn.ps.cur.temp == NULL,
             "temp buffer must be freed after the synchronous publish");
  /* Content integrity via the hook's heap copy */
  size_t bad = 0;
  for (size_t i = 0; i < PAYLOAD; ++i)
    bad += (pub_log.big_copy[i] != (uint8_t)((i * 131) + 7));
  FIO_ASSERT(bad == 0, "payload content mismatch (%zu bytes)", bad);
  FIO_ASSERT(r->conn.ps.msg_total == 0, "budget reset after frame");

  pub_log_reset();
  fio___redis_free(r);
}

static void test_push_capture_mid_zero_copy(void) {
  fprintf(stderr, "* Testing Redis mid-size push capture (fits buffer)...\n");
  pub_log_reset();
  fio_redis_engine_s *r = test_redis_engine_new();

  /* A payload of FIO_REDIS_READ_BUFFER/2 (32KB with the 64KB default:
   * bigger than the OLD 32KB buffer, bigger than FIO_RESP3_STREAM_THRESHOLD)
   * arrives in a single read: streamed start, but the first write completes
   * the string -> zero-copy publish, NO temp buffer allocation. */
  enum { PAYLOAD = FIO_REDIS_READ_BUFFER / 2 };
  char hdr[64];
  size_t hdr_len = 0;
  FIO_MEMCPY(hdr, ">3\r\n$7\r\nmessage\r\n$4\r\nchan\r\n$", 28);
  hdr_len += 28;
  hdr_len += (size_t)fio_ltoa(hdr + hdr_len, (int64_t)PAYLOAD, 10);
  hdr[hdr_len++] = '\r';
  hdr[hdr_len++] = '\n';
  size_t wire_len = (size_t)hdr_len + PAYLOAD + 2;
  FIO_ASSERT(wire_len <= FIO_REDIS_READ_BUFFER,
             "frame must fit the read buffer");
  uint8_t *wire = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, wire_len, 0);
  FIO_ASSERT_ALLOC(wire);
  FIO_MEMCPY(wire, hdr, (size_t)hdr_len);
  for (size_t i = 0; i < PAYLOAD; ++i)
    wire[(size_t)hdr_len + i] = (uint8_t)((i * 131) + 7);
  wire[(size_t)hdr_len + PAYLOAD] = '\r';
  wire[(size_t)hdr_len + PAYLOAD + 1] = '\n';

  test_redis_feed(r, wire, wire_len);

  FIO_ASSERT(pub_log.count == 1, "publish count: %zu", pub_log.count);
  FIO_ASSERT(pub_log.msg_len[0] == PAYLOAD,
             "payload length mismatch: %zu",
             pub_log.msg_len[0]);
  FIO_ASSERT(test_ptr_in_buf(r, pub_log.msg_ptr[0]),
             "mid-size payload in a single read must publish zero-copy");
  FIO_ASSERT(pub_log.msg_ptr[0] == r->buf + (size_t)hdr_len,
             "payload pointer must equal the read-buffer data offset");
  FIO_ASSERT(r->conn.ps.cur.temp == NULL,
             "no temp buffer should be allocated for single-read payloads");
  size_t bad = 0;
  for (size_t i = 0; i < PAYLOAD; ++i)
    bad += (pub_log.big_copy[i] != (uint8_t)((i * 131) + 7));
  FIO_ASSERT(bad == 0, "payload content mismatch (%zu bytes)", bad);

  FIO_MEM_FREE(wire, wire_len);
  pub_log_reset();
  fio___redis_free(r);
}

static void test_push_capture_pmessage_dedup(void) {
  fprintf(stderr, "* Testing Redis push capture pmessage dedup...\n");
  pub_log_reset();
  fio_redis_engine_s *r = test_redis_engine_new();

  /* message(chan) -> publish; pmessage(p.*, chan) -> dedup skip;
   * pmessage(p.*, other) -> publish; message(chan) -> publish. */
  const char *wire = ">3\r\n$7\r\nmessage\r\n$4\r\nchan\r\n$2\r\nm1\r\n"
                     ">4\r\n$8\r\npmessage\r\n$3\r\np.*\r\n$4\r\nchan\r\n$2\r\nm2\r\n"
                     ">4\r\n$8\r\npmessage\r\n$3\r\np.*\r\n$5\r\nother\r\n$2\r\nm3\r\n"
                     ">3\r\n$7\r\nmessage\r\n$4\r\nchan\r\n$2\r\nm4\r\n";
  test_redis_feed(r, wire, FIO_STRLEN(wire));

  FIO_ASSERT(pub_log.count == 3,
             "dedup should suppress the pmessage double-delivery: %zu",
             pub_log.count);
  FIO_ASSERT(pub_log.ch_len[0] == 4 && pub_log.msg_len[0] == 2 &&
                 !FIO_MEMCMP(pub_log.ch_copy[0], "chan", 4) &&
                 !FIO_MEMCMP(pub_log.msg_copy[0], "m1", 2),
             "publish 1 mismatch");
  FIO_ASSERT(pub_log.ch_len[1] == 5 && pub_log.msg_len[1] == 2 &&
                 !FIO_MEMCMP(pub_log.ch_copy[1], "other", 5) &&
                 !FIO_MEMCMP(pub_log.msg_copy[1], "m3", 2),
             "publish 2 mismatch");
  FIO_ASSERT(pub_log.ch_len[2] == 4 && pub_log.msg_len[2] == 2 &&
                 !FIO_MEMCMP(pub_log.ch_copy[2], "chan", 4) &&
                 !FIO_MEMCMP(pub_log.msg_copy[2], "m4", 2),
             "publish 3 mismatch");
  /* pmessage must NOT update last_channel (message owns it) */
  FIO_ASSERT(r->last_channel_len == 4 &&
                 !FIO_MEMCMP(r->last_channel, "chan", 4),
             "last_channel must come from message frames only");

  pub_log_reset();
  fio___redis_free(r);
}

static void test_push_capture_confirmation_other(void) {
  fprintf(stderr, "* Testing Redis push capture confirmations/other...\n");
  pub_log_reset();
  fio_redis_engine_s *r = test_redis_engine_new();

  /* Subscribe confirmation (push, ignored) + keyspace invalidation with a
   * nested array (unknown push type, ignored) */
  const char *wire = ">3\r\n$9\r\nsubscribe\r\n$4\r\nchan\r\n:1\r\n"
                     ">2\r\n$10\r\ninvalidate\r\n*1\r\n$3\r\nkey\r\n"
                     ">1\r\n$4\r\npong\r\n";
  test_redis_feed(r, wire, FIO_STRLEN(wire));

  FIO_ASSERT(pub_log.count == 0,
             "confirmations/unknown push types must not publish");
  FIO_ASSERT(!r->conn.ps.capture, "capture state reset");
  FIO_ASSERT(r->conn.ps.msg_total == 0, "budget reset");
  FIO_ASSERT(r->conn.parser.depth == 0, "parser stack drained");
  FIO_ASSERT(r->last_channel == NULL, "no channel recorded");

  pub_log_reset();
  fio___redis_free(r);
}

static void test_push_capture_budget_breach(void) {
  fprintf(stderr, "* Testing Redis push capture budget breach...\n");

  /* Capture-mode strings charge the same cumulative budget: a payload
   * declaration that overflows the limit breaches at its header (sentinel),
   * and the first data write aborts the parse - before any temp allocation. */
  fio___redis_parse_state_s ps = {.payload_limit = 1024};
  fio_resp3_parser_s parser = {.udata = &ps};

  const char *wire = ">3\r\n$7\r\nmessage\r\n$4\r\nchan\r\n$5000\r\n";
  fio_resp3_result_s r = fio_resp3_parse(&parser,
                                         &FIO___REDIS_RESP3_CALLBACKS,
                                         wire,
                                         FIO_STRLEN(wire));
  FIO_ASSERT(!r.err, "payload header parses (sentinel, not error)");
  FIO_ASSERT(ps.limit_exceeded,
             "capture budget breach at the payload declaration");
  FIO_ASSERT(parser.streaming_string_ctx == FIO___REDIS_PS_SENTINEL,
             "breach yields the sentinel (no temp allocation)");
  FIO_ASSERT(ps.cur.temp == NULL, "no temp allocated on breach");

  /* First payload byte: sentinel write fails -> parser error */
  r = fio_resp3_parse(&parser, &FIO___REDIS_RESP3_CALLBACKS, "x", 1);
  FIO_ASSERT(r.err, "sentinel write must abort the parse");
  fio___redis_capture_reset(&ps); /* cleanup (channel ptr not owned) */
}

static void test_push_capture_max_batch(void) {
  fprintf(stderr, "* Testing Redis MAX_BATCH push flood deferral...\n");
  pub_log_reset();
  fio_redis_engine_s *r = test_redis_engine_new();

  /* 130 push frames in one event: at most FIO_REDIS_MAX_BATCH (128) are
   * processed, the rest are deferred (buf_pos remainder + held engine ref
   * for the continuation task). */
  const char *frame = ">3\r\n$7\r\nmessage\r\n$4\r\nchan\r\n$2\r\nm1\r\n";
  size_t frame_len = FIO_STRLEN(frame);
  size_t total = 130 * frame_len;
  FIO_ASSERT(total <= FIO_REDIS_READ_BUFFER, "flood must fit the buffer");
  for (size_t i = 0; i < 130; ++i)
    FIO_MEMCPY(r->buf + i * frame_len, frame, frame_len);
  r->conn.buf_pos = (FIO___REDIS_BUF_POS_T)total;
  fio___redis_parse_buffered(r, &r->conn, r->buf, NULL);

  FIO_ASSERT(pub_log.count == FIO_REDIS_MAX_BATCH,
             "should process exactly MAX_BATCH: %zu",
             pub_log.count);
  FIO_ASSERT(r->conn.buf_pos == 2 * frame_len,
             "two frames should remain buffered: %u",
             (unsigned)r->conn.buf_pos);
  FIO_ASSERT(fio___redis_references(r) == 2,
             "continuation task should hold an engine reference");

  /* The deferred continuation processes the remainder when pumped (same
   * drain pattern as tests/io.c). A non-NULL io placeholder passes its
   * gate; the clean remaining frames never touch it. */
  r->conn.io = (fio_io_s *)r;
  fio_queue_perform_all(fio_io_queue());
  r->conn.io = NULL;
  FIO_ASSERT(pub_log.count == 130,
             "continuation should process the deferred frames: %zu",
             pub_log.count);
  FIO_ASSERT(r->conn.buf_pos == 0, "buffer fully drained");
  FIO_ASSERT(fio___redis_references(r) == 1,
             "continuation released its reference");

  pub_log_reset();
  fio___redis_free(r);
}

int main(void) {
  test_fiobj_command_to_resp();
  test_primitive_to_resp();
  test_nested_command_to_resp();
  test_resp_len_write_equivalence();
  test_resp_depth_guard();
  test_pubsub_command_builder();
  test_subscribe_command_builder();
  test_resp3_reply_to_fiobj();
  test_payload_limit_oversize_declared();
  test_payload_limit_cumulative_strings();
  test_payload_limit_ok_and_reset();
  test_payload_limit_container_overhead();
  test_hello_command_builder();
  test_hello_reply_routing();
  test_push_reply_frame_routing();
  test_push_confirmation_ignored();
  test_resubscribe_batch_bytes();
  test_push_capture_zero_copy();
  test_push_capture_split_payload();
  test_push_capture_temp_1mb();
  test_push_capture_mid_zero_copy();
  test_push_capture_pmessage_dedup();
  test_push_capture_confirmation_other();
  test_push_capture_budget_breach();
  test_push_capture_max_batch();
  return 0;
}
