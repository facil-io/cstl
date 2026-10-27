/* Logging byte layout, failure policy and errno regression tests.
 * Also run with -DFIO_LOG_LENGTH_LIMIT=3, 4, 128, and 129. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *test_log_stream;
static int test_fail_format;
static int test_vsnprintf(char *dest,
                          size_t capacity,
                          const char *format,
                          va_list args) {
  if (test_fail_format) {
    errno = EILSEQ;
    return -1;
  }
  return vsnprintf(dest, capacity, format, args);
}
#undef vsnprintf
#define vsnprintf             test_vsnprintf
#define FIO_STDERR_FILE       (test_log_stream ? test_log_stream : stderr)
#define FIO_LOG_LEVEL_DEFAULT 0
#define FIO_LOG
#include "fio-stl/include.h"
#undef vsnprintf

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr,                                                          \
              "logging check failed at line %d: %s\n",                         \
              __LINE__,                                                        \
              #cond);                                                          \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

static void test_message(size_t length) {
  static const char warning[] = "...\n\t\x1B[2mWARNING:\x1B[0m TRUNCATED!";
  char message[FIO_LOG_LENGTH_LIMIT + 64];
  char actual[FIO_LOG_LENGTH_LIMIT + 128];
  char expected[sizeof(actual)];
  CHECK(length < sizeof(message));
  memset(message, 'x', length);
  message[length] = 0;
  size_t payload = length;
  size_t expected_len;
  int truncated = length >= FIO_LOG_LENGTH_LIMIT - 2;
  if (truncated) {
#if FIO_LOG_LENGTH_LIMIT > 128
    payload = FIO_LOG_LENGTH_LIMIT - 34;
#else
    payload = FIO_LOG_LENGTH_LIMIT - 3;
#endif
  }
  memset(expected, 'x', payload);
  expected_len = payload;
  if (truncated) {
    memcpy(expected + expected_len, warning, sizeof(warning) - 1);
    expected_len += sizeof(warning) - 1;
  }
  expected[expected_len++] = '\n';
  test_log_stream = tmpfile();
  CHECK(test_log_stream);
  errno = EAGAIN;
  FIO_LOG2STDERR("%s", message);
  CHECK(errno == EAGAIN);
  CHECK(!fflush(test_log_stream));
  CHECK(!fseek(test_log_stream, 0, SEEK_SET));
  size_t actual_len = fread(actual, 1, sizeof(actual), test_log_stream);
  CHECK(actual_len == expected_len);
  CHECK(!memcmp(actual, expected, expected_len));
  CHECK(!fclose(test_log_stream));
  test_log_stream = NULL;
}

static void test_format_failure(void) {
  static const char expected[] = "\x1B[1mERROR:\x1B[0m log formatting error.\n";
  char actual[sizeof(expected)];
  test_log_stream = tmpfile();
  CHECK(test_log_stream);
  test_fail_format = 1;
  errno = EAGAIN;
  FIO_LOG2STDERR("%s", "ignored");
  CHECK(errno == EAGAIN);
  test_fail_format = 0;
  CHECK(!fflush(test_log_stream));
  CHECK(!fseek(test_log_stream, 0, SEEK_SET));
  CHECK(fread(actual, 1, sizeof(actual), test_log_stream) ==
        sizeof(expected) - 1);
  CHECK(!memcmp(actual, expected, sizeof(expected) - 1));
  CHECK(!fclose(test_log_stream));
  test_log_stream = NULL;
}

static void test_write_failure(void) {
  /* A read-only stream rejects writes without requiring a special device. */
  test_log_stream = fopen(__FILE__, "r");
  CHECK(test_log_stream);
  CHECK(!setvbuf(test_log_stream, NULL, _IONBF, 0));
  errno = EAGAIN;
  FIO_LOG2STDERR("%s", "failure");
  CHECK(ferror(test_log_stream));
  CHECK(errno == EAGAIN);
  CHECK(!fclose(test_log_stream));
  test_log_stream = NULL;
}

int main(int argc, char **argv) {
#ifdef FIO_TEST_LOG_BAD_FORMAT
  /* Compile-only negative probes; never execute these calls. */
  FIO_LOG2STDERR("%s", 7);
  FIO_LOG2STDERR("%d %d", 7);
#endif
  if (argc > 1 && !strcmp(argv[1], "empty")) {
    test_message(0);
  } else if (argc > 1 && !strcmp(argv[1], "failure")) {
    test_write_failure();
  } else {
    /* Boundaries around formatter capacity, plus a long truncation. */
    const size_t lengths[] = {FIO_LOG_LENGTH_LIMIT - 3,
                              FIO_LOG_LENGTH_LIMIT - 2,
                              FIO_LOG_LENGTH_LIMIT - 1,
                              FIO_LOG_LENGTH_LIMIT,
                              FIO_LOG_LENGTH_LIMIT + 63,
                              0};
    for (size_t i = 0; i < sizeof(lengths) / sizeof(*lengths); ++i)
      test_message(lengths[i]);
    test_write_failure();
    test_format_failure();
    CHECK(FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_WARNING) == FIO_LOG_LEVEL_WARNING);
    CHECK(FIO_LOG_LEVEL_GET() == FIO_LOG_LEVEL_WARNING);
    CHECK(FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE) == FIO_LOG_LEVEL_NONE);
    FIO_LOG_INFO("must not print");
  }
  fprintf(stderr, "logging tests passed (limit %d)\n", FIO_LOG_LENGTH_LIMIT);
  return 0;
}
