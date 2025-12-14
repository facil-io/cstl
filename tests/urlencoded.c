/* URL-Encoded Parser Tests */
#define FIO_URL_ENCODED
#include "fio-stl.h"

#include <stdio.h>

/* Test context */
typedef struct {
  int count;
  int stop_at; /* -1 = don't stop */
} test_ctx_s;

/* Callback that prints pairs */
static void *on_pair_print(void *udata,
                           fio_buf_info_s name,
                           fio_buf_info_s value) {
  test_ctx_s *ctx = (test_ctx_s *)udata;
  printf("  Pair %d: name='%.*s' (%zu) value='%.*s' (%zu)\n",
         ctx->count,
         (int)name.len,
         name.buf,
         name.len,
         (int)value.len,
         value.buf,
         value.len);
  ctx->count++;
  if (ctx->stop_at >= 0 && ctx->count >= ctx->stop_at)
    return NULL; /* Stop parsing */
  return udata;
}

/* Test helper macro */
#define TEST_PARSE(desc, data_str, expected_count, expected_consumed)          \
  do {                                                                         \
    test_ctx_s ctx = {.count = 0, .stop_at = -1};                              \
    const char *data = data_str;                                               \
    size_t len = strlen(data);                                                 \
    printf("Test: %s\n", desc);                                                \
    printf("  Input: '%s' (len=%zu)\n", data, len);                            \
    fio_url_encoded_result_s r =                                               \
        fio_url_encoded_parse(&callbacks, &ctx, data, len);                    \
    printf("  Result: consumed=%zu, count=%zu, err=%d\n",                      \
           r.consumed,                                                         \
           r.count,                                                            \
           r.err);                                                             \
    if (r.count != (expected_count)) {                                         \
      printf("  FAIL: expected count=%d, got %zu\n",                           \
             (expected_count),                                                 \
             r.count);                                                         \
      return 1;                                                                \
    }                                                                          \
    if (r.consumed != (expected_consumed)) {                                   \
      printf("  FAIL: expected consumed=%d, got %zu\n",                        \
             (expected_consumed),                                              \
             r.consumed);                                                      \
      return 1;                                                                \
    }                                                                          \
    printf("  PASS\n\n");                                                      \
  } while (0)

int main(void) {
  static const fio_url_encoded_parser_callbacks_s callbacks = {
      .on_pair = on_pair_print,
  };

  printf("=== URL-Encoded Parser Tests ===\n\n");

  /* Basic parsing */
  TEST_PARSE("Basic name=value pairs",
             "name=value&foo=bar&baz=qux",
             3,
             strlen("name=value&foo=bar&baz=qux"));

  /* Empty value */
  TEST_PARSE("Empty value (name=)",
             "name=&foo=bar",
             2,
             strlen("name=&foo=bar"));

  /* Missing = (value should be empty) */
  TEST_PARSE("Missing = (name only)",
             "name&foo=bar",
             2,
             strlen("name&foo=bar"));

  /* Empty name */
  TEST_PARSE("Empty name (=value)",
             "=value&foo=bar",
             2,
             strlen("=value&foo=bar"));

  /* Double && (empty pair should be skipped) */
  TEST_PARSE("Double && (empty pair)",
             "name=value&&foo=bar",
             2,
             strlen("name=value&&foo=bar"));

  /* URL-encoded data (not decoded by parser) */
  TEST_PARSE("URL-encoded data (not decoded)",
             "name=hello%20world&foo=bar%26baz",
             2,
             strlen("name=hello%20world&foo=bar%26baz"));

  /* Leading & */
  TEST_PARSE("Leading &", "&name=value", 1, strlen("&name=value"));

  /* Trailing & */
  TEST_PARSE("Trailing &", "name=value&", 1, strlen("name=value&"));

  /* Empty string */
  TEST_PARSE("Empty string", "", 0, 0);

  /* Single pair */
  TEST_PARSE("Single pair", "name=value", 1, strlen("name=value"));

  /* Single name without value */
  TEST_PARSE("Single name without =", "name", 1, strlen("name"));

  /* Multiple empty pairs */
  TEST_PARSE("Multiple empty pairs", "&&&", 0, strlen("&&&"));

  /* Complex URL-encoded form data */
  TEST_PARSE(
      "Complex form data",
      "username=john%40example.com&password=secret%21%23&remember=on",
      3,
      strlen("username=john%40example.com&password=secret%21%23&remember=on"));

  /* Test callback stopping */
  {
    test_ctx_s ctx = {.count = 0, .stop_at = 2};
    const char *data = "a=1&b=2&c=3&d=4";
    printf("Test: Callback stop after 2 pairs\n");
    printf("  Input: '%s'\n", data);
    fio_url_encoded_result_s r =
        fio_url_encoded_parse(&callbacks, &ctx, data, strlen(data));
    printf("  Result: consumed=%zu, count=%zu, err=%d\n",
           r.consumed,
           r.count,
           r.err);
    if (r.count != 2) {
      printf("  FAIL: expected count=2, got %zu\n", r.count);
      return 1;
    }
    if (r.err != 1) {
      printf("  FAIL: expected err=1, got %d\n", r.err);
      return 1;
    }
    printf("  PASS\n\n");
  }

  /* Test with NULL callbacks (should use no-op) */
  {
    const char *data = "a=1&b=2";
    printf("Test: NULL callbacks (no-op)\n");
    printf("  Input: '%s'\n", data);
    fio_url_encoded_result_s r =
        fio_url_encoded_parse(NULL, NULL, data, strlen(data));
    printf("  Result: consumed=%zu, count=%zu, err=%d\n",
           r.consumed,
           r.count,
           r.err);
    if (r.count != 2) {
      printf("  FAIL: expected count=2, got %zu\n", r.count);
      return 1;
    }
    printf("  PASS\n\n");
  }

  printf("=== All URL-Encoded Parser Tests Passed! ===\n");
  return 0;
}
