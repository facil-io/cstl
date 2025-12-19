/* URL-Encoded Parser Tests */
#define FIO_LOG
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
  (void)name;  /* Used only in debug logging */
  (void)value; /* Used only in debug logging */
  FIO_LOG_DDEBUG("  Pair %d: name='%.*s' (%zu) value='%.*s' (%zu)",
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

/* Context for value verification test */
typedef struct {
  const char *expected_name;
  size_t expected_name_len;
  const char *expected_value;
  size_t expected_value_len;
  int verified;
} verify_ctx_s;

/* Callback for value verification */
static void *on_pair_verify(void *udata,
                            fio_buf_info_s name,
                            fio_buf_info_s value) {
  verify_ctx_s *ctx = (verify_ctx_s *)udata;
  if (name.len == ctx->expected_name_len &&
      FIO_MEMCMP(name.buf, ctx->expected_name, name.len) == 0 &&
      value.len == ctx->expected_value_len &&
      FIO_MEMCMP(value.buf, ctx->expected_value, value.len) == 0) {
    ctx->verified = 1;
  }
  return NULL; /* Stop after first pair */
}

/* Context for name-only test */
typedef struct {
  int name_only_correct;
} name_only_ctx_s;

/* Callback for name-only test */
static void *on_pair_name_only(void *udata,
                               fio_buf_info_s name,
                               fio_buf_info_s value) {
  name_only_ctx_s *ctx = (name_only_ctx_s *)udata;
  /* For "nameonly", value should have len=0 */
  if (name.len == 8 && FIO_MEMCMP(name.buf, "nameonly", 8) == 0 &&
      value.len == 0) {
    ctx->name_only_correct = 1;
  }
  return udata;
}

/* Test helper macro */
#define TEST_PARSE(desc, data_str, expected_count, expected_consumed)          \
  do {                                                                         \
    test_ctx_s ctx = {.count = 0, .stop_at = -1};                              \
    const char *data = data_str;                                               \
    size_t len = strlen(data);                                                 \
    FIO_LOG_DDEBUG("Test: %s", desc);                                          \
    FIO_LOG_DDEBUG("  Input: '%s' (len=%zu)", data, len);                      \
    fio_url_encoded_result_s r =                                               \
        fio_url_encoded_parse(&callbacks, &ctx, data, len);                    \
    FIO_LOG_DDEBUG("  Result: consumed=%zu, count=%zu, err=%d",                \
                   r.consumed,                                                 \
                   r.count,                                                    \
                   r.err);                                                     \
    if (r.count != (expected_count)) {                                         \
      fprintf(stderr,                                                          \
              "  FAIL: expected count=%d, got %zu\n",                          \
              (expected_count),                                                \
              r.count);                                                        \
      return 1;                                                                \
    }                                                                          \
    if (r.consumed != (size_t)(expected_consumed)) {                           \
      fprintf(stderr,                                                          \
              "  FAIL: expected consumed=%zu, got %zu\n",                      \
              (size_t)(expected_consumed),                                     \
              r.consumed);                                                     \
      return 1;                                                                \
    }                                                                          \
    FIO_LOG_DDEBUG("  PASS");                                                  \
  } while (0)

int main(void) {
  static const fio_url_encoded_parser_callbacks_s callbacks = {
      .on_pair = on_pair_print,
  };

  FIO_LOG_DDEBUG("=== URL-Encoded Parser Tests ===");

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
    FIO_LOG_DDEBUG("Test: Callback stop after 2 pairs");
    FIO_LOG_DDEBUG("  Input: '%s'", data);
    fio_url_encoded_result_s r =
        fio_url_encoded_parse(&callbacks, &ctx, data, strlen(data));
    FIO_LOG_DDEBUG("  Result: consumed=%zu, count=%zu, err=%d",
                   r.consumed,
                   r.count,
                   r.err);
    if (r.count != 2) {
      fprintf(stderr, "  FAIL: expected count=2, got %zu\n", r.count);
      return 1;
    }
    if (r.err != 1) {
      fprintf(stderr, "  FAIL: expected err=1, got %d\n", r.err);
      return 1;
    }
    FIO_LOG_DDEBUG("  PASS");
  }

  /* Test with NULL callbacks (should use no-op) */
  {
    const char *data = "a=1&b=2";
    FIO_LOG_DDEBUG("Test: NULL callbacks (no-op)");
    FIO_LOG_DDEBUG("  Input: '%s'", data);
    fio_url_encoded_result_s r =
        fio_url_encoded_parse(NULL, NULL, data, strlen(data));
    FIO_LOG_DDEBUG("  Result: consumed=%zu, count=%zu, err=%d",
                   r.consumed,
                   r.count,
                   r.err);
    if (r.count != 2) {
      fprintf(stderr, "  FAIL: expected count=2, got %zu\n", r.count);
      return 1;
    }
    FIO_LOG_DDEBUG("  PASS");
  }

  /* Plus sign as space (parser doesn't decode, just finds boundaries) */
  TEST_PARSE("Plus sign in value (not decoded)",
             "name=hello+world&foo=bar",
             2,
             strlen("name=hello+world&foo=bar"));

  /* Special characters that should be encoded */
  TEST_PARSE("Percent-encoded special chars",
             "email=user%40example.com&msg=hello%26goodbye",
             2,
             strlen("email=user%40example.com&msg=hello%26goodbye"));

  /* Very long value */
  {
    /* Build a long value: key=<1000 'a' characters> */
    char long_data[1100];
    FIO_MEMCPY(long_data, "longkey=", 8);
    FIO_MEMSET(long_data + 8, 'a', 1000);
    long_data[1008] = '\0';

    test_ctx_s ctx = {.count = 0, .stop_at = -1};
    FIO_LOG_DDEBUG("Test: Very long value (1000 chars)");
    FIO_LOG_DDEBUG("  Input: longkey=<1000 'a' chars>");
    fio_url_encoded_result_s r =
        fio_url_encoded_parse(&callbacks, &ctx, long_data, 1008);
    FIO_LOG_DDEBUG("  Result: consumed=%zu, count=%zu, err=%d",
                   r.consumed,
                   r.count,
                   r.err);
    FIO_ASSERT(r.count == 1,
               "Very long value: expected count=1, got %zu",
               r.count);
    FIO_ASSERT(r.consumed == 1008,
               "Very long value: expected consumed=1008, got %zu",
               r.consumed);
    FIO_LOG_DDEBUG("  PASS");
  }

  /* Unicode characters (percent-encoded UTF-8) */
  /* UTF-8 for "日本" is E6 97 A5 E6 9C AC */
  TEST_PARSE("Unicode UTF-8 percent-encoded",
             "text=%E6%97%A5%E6%9C%AC&lang=ja",
             2,
             strlen("text=%E6%97%A5%E6%9C%AC&lang=ja"));

  /* Duplicate keys (same key appears multiple times) */
  TEST_PARSE("Duplicate keys",
             "color=red&color=blue&color=green",
             3,
             strlen("color=red&color=blue&color=green"));

  /* Malformed percent encoding - parser doesn't validate, just finds boundaries
   */
  TEST_PARSE("Incomplete percent encoding (%)",
             "name=hello%&foo=bar",
             2,
             strlen("name=hello%&foo=bar"));

  TEST_PARSE("Incomplete percent encoding (%2)",
             "name=hello%2&foo=bar",
             2,
             strlen("name=hello%2&foo=bar"));

  TEST_PARSE("Invalid hex in percent encoding (%ZZ)",
             "name=hello%ZZ&foo=bar",
             2,
             strlen("name=hello%ZZ&foo=bar"));

  /* Multiple equals signs in value */
  TEST_PARSE("Multiple equals signs",
             "equation=a=b=c&foo=bar",
             2,
             strlen("equation=a=b=c&foo=bar"));

  /* Value with only special characters */
  TEST_PARSE("Value with encoded special chars only",
             "special=%21%40%23%24%25",
             1,
             strlen("special=%21%40%23%24%25"));

  /* Empty key with empty value */
  TEST_PARSE("Empty key and empty value (=)",
             "=&foo=bar",
             2,
             strlen("=&foo=bar"));

  /* Just equals sign */
  TEST_PARSE("Just equals sign", "=", 1, strlen("="));

  /* Whitespace in values (should be encoded as %20 or +) */
  TEST_PARSE("Literal space (invalid but parser handles)",
             "name=hello world&foo=bar",
             2,
             strlen("name=hello world&foo=bar"));

  /* Very long key */
  {
    /* Build a long key: <500 'k' characters>=value */
    char long_key_data[520];
    FIO_MEMSET(long_key_data, 'k', 500);
    FIO_MEMCPY(long_key_data + 500, "=value", 6);
    long_key_data[506] = '\0';

    test_ctx_s ctx = {.count = 0, .stop_at = -1};
    FIO_LOG_DDEBUG("Test: Very long key (500 chars)");
    FIO_LOG_DDEBUG("  Input: <500 'k' chars>=value");
    fio_url_encoded_result_s r =
        fio_url_encoded_parse(&callbacks, &ctx, long_key_data, 506);
    FIO_LOG_DDEBUG("  Result: consumed=%zu, count=%zu, err=%d",
                   r.consumed,
                   r.count,
                   r.err);
    FIO_ASSERT(r.count == 1,
               "Very long key: expected count=1, got %zu",
               r.count);
    FIO_ASSERT(r.consumed == 506,
               "Very long key: expected consumed=506, got %zu",
               r.consumed);
    FIO_LOG_DDEBUG("  PASS");
  }

  /* Many pairs */
  {
    /* Build: a=1&b=2&c=3&...&z=26 (26 pairs) */
    char many_pairs[200];
    char *p = many_pairs;
    for (int i = 0; i < 26; i++) {
      if (i > 0)
        *p++ = '&';
      *p++ = (char)('a' + i);
      *p++ = '=';
      if (i + 1 >= 10) {
        *p++ = (char)('0' + (i + 1) / 10);
      }
      *p++ = (char)('0' + (i + 1) % 10);
    }
    *p = '\0';
    size_t many_len = (size_t)(p - many_pairs);

    test_ctx_s ctx = {.count = 0, .stop_at = -1};
    FIO_LOG_DDEBUG("Test: Many pairs (26 pairs a=1 through z=26)");
    FIO_LOG_DDEBUG("  Input: '%s'", many_pairs);
    fio_url_encoded_result_s r =
        fio_url_encoded_parse(&callbacks, &ctx, many_pairs, many_len);
    FIO_LOG_DDEBUG("  Result: consumed=%zu, count=%zu, err=%d",
                   r.consumed,
                   r.count,
                   r.err);
    FIO_ASSERT(r.count == 26,
               "Many pairs: expected count=26, got %zu",
               r.count);
    FIO_ASSERT(r.consumed == many_len,
               "Many pairs: expected consumed=%zu, got %zu",
               many_len,
               r.consumed);
    FIO_LOG_DDEBUG("  PASS");
  }

  /* Numeric keys and values */
  TEST_PARSE("Numeric keys and values",
             "123=456&789=012",
             2,
             strlen("123=456&789=012"));

  /* Mixed empty and non-empty */
  TEST_PARSE("Mixed empty and non-empty pairs",
             "&a=1&&b=&c&&d=4&",
             4,
             strlen("&a=1&&b=&c&&d=4&"));

  /* Semicolon as separator (some implementations support this, but standard
   * uses &) */
  /* Our parser should treat ; as part of the value, not a separator */
  TEST_PARSE("Semicolon in value (not a separator)",
             "name=a;b&foo=bar",
             2,
             strlen("name=a;b&foo=bar"));

  /* Test callback value verification */
  {
    static const fio_url_encoded_parser_callbacks_s verify_callbacks = {
        .on_pair = on_pair_verify,
    };

    verify_ctx_s vctx = {
        .expected_name = "test",
        .expected_name_len = 4,
        .expected_value = "value123",
        .expected_value_len = 8,
        .verified = 0,
    };

    const char *data = "test=value123&other=data";
    FIO_LOG_DDEBUG("Test: Verify callback receives correct name/value");
    FIO_LOG_DDEBUG("  Input: '%s'", data);
    fio_url_encoded_result_s r =
        fio_url_encoded_parse(&verify_callbacks, &vctx, data, strlen(data));
    FIO_LOG_DDEBUG("  Result: consumed=%zu, count=%zu, err=%d, verified=%d",
                   r.consumed,
                   r.count,
                   r.err,
                   vctx.verified);
    FIO_ASSERT(vctx.verified == 1, "Callback value verification failed");
    FIO_ASSERT(r.count == 1, "Verify test: expected count=1, got %zu", r.count);
    FIO_ASSERT(r.err == 1,
               "Verify test: expected err=1 (stopped), got %d",
               r.err);
    FIO_LOG_DDEBUG("  PASS");
  }

  /* Test that value without = has correct empty value pointer */
  {
    static const fio_url_encoded_parser_callbacks_s name_only_callbacks = {
        .on_pair = on_pair_name_only,
    };

    name_only_ctx_s nctx = {.name_only_correct = 0};
    const char *data = "nameonly";
    FIO_LOG_DDEBUG("Test: Name without = has empty value");
    FIO_LOG_DDEBUG("  Input: '%s'", data);
    fio_url_encoded_result_s r =
        fio_url_encoded_parse(&name_only_callbacks, &nctx, data, strlen(data));
    (void)r; /* Used only in debug logging */
    FIO_LOG_DDEBUG("  Result: consumed=%zu, count=%zu, err=%d, correct=%d",
                   r.consumed,
                   r.count,
                   r.err,
                   nctx.name_only_correct);
    FIO_ASSERT(nctx.name_only_correct == 1, "Name-only value check failed");
    FIO_LOG_DDEBUG("  PASS");
  }

  FIO_LOG_DDEBUG("=== All URL-Encoded Parser Tests Passed! ===");
  return 0;
}
