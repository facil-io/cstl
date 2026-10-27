/* Peer errors remain intact as replies, but cannot control the log terminal. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FILE *test_log_stream;
#define FIO_STDERR_FILE       (test_log_stream ? test_log_stream : stderr)
#define FIO_LOG_LEVEL_DEFAULT 0
#define FIO_LOG
#define FIO_REDIS
#include "fio-stl/include.h"

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr,                                                          \
              "redis logging check failed at line %d: %s\n",                   \
              __LINE__,                                                        \
              #cond);                                                          \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

static void test_error(const char *peer,
                       size_t peer_len,
                       const char *expected) {
  static const char prefix[] =
      "\x1B[2mWARNING:\x1B[0m  (redis) error response: ";
  char wire[2048];
  char actual[2048];
  CHECK(peer_len + 3 < sizeof(wire));
  wire[0] = '-';
  memcpy(wire + 1, peer, peer_len);
  memcpy(wire + 1 + peer_len, "\r\n", 2);
  test_log_stream = tmpfile();
  CHECK(test_log_stream);
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_WARNING);
  fio_resp3_parser_s parser = {0};
  fio_resp3_result_s result = fio_resp3_parse(&parser,
                                              &FIO___REDIS_RESP3_CALLBACKS,
                                              wire,
                                              peer_len + 3);
  CHECK(!result.err);
  CHECK(result.consumed == peer_len + 3);
  FIOBJ reply = (FIOBJ)result.obj;
  fio_str_info_s data = fiobj2cstr(reply);
  CHECK(data.len == peer_len);
  CHECK(!memcmp(data.buf, peer, peer_len));
  CHECK(!fflush(test_log_stream));
  CHECK(!fseek(test_log_stream, 0, SEEK_SET));
  size_t len = fread(actual, 1, sizeof(actual), test_log_stream);
  CHECK(len > sizeof(prefix) - 1);
  CHECK(!memcmp(actual, prefix, sizeof(prefix) - 1));
  /* The peer field is ASCII text; only trusted prefix/truncation ANSI is raw.
   */
  size_t end = len - 1;
  for (size_t i = sizeof(prefix) - 1; i < end; ++i) {
    if (actual[i] == '\n' && i >= 3 && !memcmp(actual + i - 3, "...", 3))
      break; /* logger's deliberate truncation suffix */
    CHECK((unsigned char)actual[i] >= 32 && (unsigned char)actual[i] < 127);
  }
  CHECK(actual[len - 1] == '\n');
  if (expected) {
    size_t expected_len = strlen(expected);
    CHECK(len == sizeof(prefix) - 1 + expected_len + 1);
    CHECK(!memcmp(actual + sizeof(prefix) - 1, expected, expected_len));
  }
  fiobj_free(reply);
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
  CHECK(!fclose(test_log_stream));
  test_log_stream = NULL;
}

int main(void) {
  /* RESP3 simple errors may not contain CR (strict framing rejects it). */
  {
    fio_resp3_parser_s parser = {0};
    fio_resp3_result_s r = fio_resp3_parse(&parser,
                                           &FIO___REDIS_RESP3_CALLBACKS,
                                           "-ERR a\rb\r\n",
                                           10);
    CHECK(r.err && !r.obj);
  }
  static const char controls[] = "ERR peer\bRETURN\x1B[2Jdone";
  test_error(controls, sizeof(controls) - 1, "ERR peer\\x08RETURN\\x1B[2Jdone");
  static const char binary[] =
      {'E', 0, 1, 127, 0x1B, (char)0x9B, (char)0xFF, '\\', 'x', '1', 'b'};
  test_error(binary, sizeof(binary), "E\\x00\\x01\\x7F\\x1B\\x9B\\xFF\\x5Cx1b");
  test_error("ERR normal", sizeof("ERR normal") - 1, "ERR normal");
  char long_error[1536];
  memset(long_error, 0x1B, sizeof(long_error));
  test_error(long_error, sizeof(long_error), NULL);
  fprintf(stderr, "redis logging tests passed\n");
  return 0;
}
