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

int main(void) {
  test_fiobj_command_to_resp();
  test_primitive_to_resp();
  test_pubsub_command_builder();
  test_resp3_reply_to_fiobj();
  return 0;
}
