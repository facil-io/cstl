/* *****************************************************************************
JSON Parser Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_JSON
#define FIO_STR
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Test Data Structures - Simple JSON Value Representation
***************************************************************************** */

typedef enum {
  TEST_JSON_NULL = 0,
  TEST_JSON_TRUE,
  TEST_JSON_FALSE,
  TEST_JSON_NUMBER,
  TEST_JSON_FLOAT,
  TEST_JSON_STRING,
  TEST_JSON_ARRAY,
  TEST_JSON_MAP,
} test_json_type_e;

typedef struct test_json_value_s test_json_value_s;

typedef struct {
  test_json_value_s **items;
  size_t count;
  size_t capa;
} test_json_array_s;

typedef struct {
  test_json_value_s **keys;
  test_json_value_s **values;
  size_t count;
  size_t capa;
} test_json_map_s;

struct test_json_value_s {
  test_json_type_e type;
  union {
    int64_t i;
    double f;
    struct {
      char *buf;
      size_t len;
    } str;
    test_json_array_s arr;
    test_json_map_s map;
  } data;
};

/* *****************************************************************************
Test Value Management
***************************************************************************** */

FIO_SFUNC test_json_value_s *test_json_value_new(test_json_type_e type) {
  test_json_value_s *v =
      (test_json_value_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*v), 0);
  FIO_ASSERT_ALLOC(v);
  FIO_MEMSET(v, 0, sizeof(*v));
  v->type = type;
  return v;
}

FIO_SFUNC void test_json_value_free(test_json_value_s *v) {
  if (!v)
    return;
  switch (v->type) {
  case TEST_JSON_STRING:
    if (v->data.str.buf)
      FIO_MEM_FREE(v->data.str.buf, v->data.str.len + 1);
    break;
  case TEST_JSON_ARRAY:
    for (size_t i = 0; i < v->data.arr.count; ++i)
      test_json_value_free(v->data.arr.items[i]);
    if (v->data.arr.items)
      FIO_MEM_FREE(v->data.arr.items,
                   v->data.arr.capa * sizeof(test_json_value_s *));
    break;
  case TEST_JSON_MAP:
    for (size_t i = 0; i < v->data.map.count; ++i) {
      test_json_value_free(v->data.map.keys[i]);
      test_json_value_free(v->data.map.values[i]);
    }
    if (v->data.map.keys)
      FIO_MEM_FREE(v->data.map.keys,
                   v->data.map.capa * sizeof(test_json_value_s *));
    if (v->data.map.values)
      FIO_MEM_FREE(v->data.map.values,
                   v->data.map.capa * sizeof(test_json_value_s *));
    break;
  default: break;
  }
  FIO_MEM_FREE(v, sizeof(*v));
}

/* *****************************************************************************
JSON Parser Callbacks
***************************************************************************** */

FIO_SFUNC void *test_json_on_null(void) {
  return test_json_value_new(TEST_JSON_NULL);
}

FIO_SFUNC void *test_json_on_true(void) {
  return test_json_value_new(TEST_JSON_TRUE);
}

FIO_SFUNC void *test_json_on_false(void) {
  return test_json_value_new(TEST_JSON_FALSE);
}

FIO_SFUNC void *test_json_on_number(int64_t i) {
  test_json_value_s *v = test_json_value_new(TEST_JSON_NUMBER);
  v->data.i = i;
  return v;
}

FIO_SFUNC void *test_json_on_float(double f) {
  test_json_value_s *v = test_json_value_new(TEST_JSON_FLOAT);
  v->data.f = f;
  return v;
}

FIO_SFUNC void *test_json_on_string(const void *start, size_t len) {
  test_json_value_s *v = test_json_value_new(TEST_JSON_STRING);
  /* Unescape the string */
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, len + 1, 0);
  FIO_ASSERT_ALLOC(buf);
  fio_str_info_s dest = FIO_STR_INFO3(buf, 0, len + 1);
  fio_string_write_unescape(&dest, NULL, start, len);
  v->data.str.buf = buf;
  v->data.str.len = dest.len;
  return v;
}

FIO_SFUNC void *test_json_on_string_simple(const void *start, size_t len) {
  test_json_value_s *v = test_json_value_new(TEST_JSON_STRING);
  v->data.str.buf = (char *)FIO_MEM_REALLOC(NULL, 0, len + 1, 0);
  FIO_ASSERT_ALLOC(v->data.str.buf);
  FIO_MEMCPY(v->data.str.buf, start, len);
  v->data.str.buf[len] = '\0';
  v->data.str.len = len;
  return v;
}

FIO_SFUNC void *test_json_on_map(void *ctx, void *at) {
  test_json_value_s *v = test_json_value_new(TEST_JSON_MAP);
  v->data.map.capa = 8;
  v->data.map.keys = (test_json_value_s **)FIO_MEM_REALLOC(
      NULL,
      0,
      v->data.map.capa * sizeof(test_json_value_s *),
      0);
  v->data.map.values = (test_json_value_s **)FIO_MEM_REALLOC(
      NULL,
      0,
      v->data.map.capa * sizeof(test_json_value_s *),
      0);
  FIO_ASSERT_ALLOC(v->data.map.keys);
  FIO_ASSERT_ALLOC(v->data.map.values);
  return v;
  (void)ctx;
  (void)at;
}

FIO_SFUNC void *test_json_on_array(void *ctx, void *at) {
  test_json_value_s *v = test_json_value_new(TEST_JSON_ARRAY);
  v->data.arr.capa = 8;
  v->data.arr.items = (test_json_value_s **)FIO_MEM_REALLOC(
      NULL,
      0,
      v->data.arr.capa * sizeof(test_json_value_s *),
      0);
  FIO_ASSERT_ALLOC(v->data.arr.items);
  return v;
  (void)ctx;
  (void)at;
}

FIO_SFUNC int test_json_map_push(void *ctx, void *key, void *value) {
  test_json_value_s *map = (test_json_value_s *)ctx;
  if (map->data.map.count >= map->data.map.capa) {
    size_t new_capa = map->data.map.capa * 2;
    map->data.map.keys = (test_json_value_s **)FIO_MEM_REALLOC(
        map->data.map.keys,
        map->data.map.capa * sizeof(test_json_value_s *),
        new_capa * sizeof(test_json_value_s *),
        map->data.map.count * sizeof(test_json_value_s *));
    map->data.map.values = (test_json_value_s **)FIO_MEM_REALLOC(
        map->data.map.values,
        map->data.map.capa * sizeof(test_json_value_s *),
        new_capa * sizeof(test_json_value_s *),
        map->data.map.count * sizeof(test_json_value_s *));
    map->data.map.capa = new_capa;
  }
  map->data.map.keys[map->data.map.count] = (test_json_value_s *)key;
  map->data.map.values[map->data.map.count] = (test_json_value_s *)value;
  map->data.map.count++;
  return 0;
}

FIO_SFUNC int test_json_array_push(void *ctx, void *value) {
  test_json_value_s *arr = (test_json_value_s *)ctx;
  if (arr->data.arr.count >= arr->data.arr.capa) {
    size_t new_capa = arr->data.arr.capa * 2;
    arr->data.arr.items = (test_json_value_s **)FIO_MEM_REALLOC(
        arr->data.arr.items,
        arr->data.arr.capa * sizeof(test_json_value_s *),
        new_capa * sizeof(test_json_value_s *),
        arr->data.arr.count * sizeof(test_json_value_s *));
    arr->data.arr.capa = new_capa;
  }
  arr->data.arr.items[arr->data.arr.count++] = (test_json_value_s *)value;
  return 0;
}

FIO_SFUNC void test_json_free_unused(void *obj) {
  test_json_value_free((test_json_value_s *)obj);
}

FIO_SFUNC void *test_json_on_error(void *ctx) {
  test_json_value_free((test_json_value_s *)ctx);
  return NULL;
}

static fio_json_parser_callbacks_s test_json_callbacks = {
    .on_null = test_json_on_null,
    .on_true = test_json_on_true,
    .on_false = test_json_on_false,
    .on_number = test_json_on_number,
    .on_float = test_json_on_float,
    .on_string = test_json_on_string,
    .on_string_simple = test_json_on_string_simple,
    .on_map = test_json_on_map,
    .on_array = test_json_on_array,
    .map_push = test_json_map_push,
    .array_push = test_json_array_push,
    .free_unused_object = test_json_free_unused,
    .on_error = test_json_on_error,
};

/* *****************************************************************************
Helper: Parse JSON string
***************************************************************************** */

FIO_SFUNC test_json_value_s *test_json_parse(const char *json, size_t len) {
  fio_json_result_s r = fio_json_parse(&test_json_callbacks, json, len);
  if (r.err)
    return NULL;
  return (test_json_value_s *)r.ctx;
}

/* *****************************************************************************
Test: Basic Types
***************************************************************************** */

FIO_SFUNC void fio___test_json_null(void) {
  fprintf(stderr, "\t* Testing JSON null parsing.\n");
  test_json_value_s *v = test_json_parse("null", 4);
  FIO_ASSERT(v, "JSON null parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NULL, "JSON null type mismatch");
  test_json_value_free(v);

  /* Test case insensitivity */
  v = test_json_parse("NULL", 4);
  FIO_ASSERT(v, "JSON NULL (uppercase) parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NULL, "JSON NULL type mismatch");
  test_json_value_free(v);

  v = test_json_parse("Null", 4);
  FIO_ASSERT(v, "JSON Null (mixed case) parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NULL, "JSON Null type mismatch");
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_booleans(void) {
  fprintf(stderr, "\t* Testing JSON boolean parsing.\n");

  test_json_value_s *v = test_json_parse("true", 4);
  FIO_ASSERT(v, "JSON true parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_TRUE, "JSON true type mismatch");
  test_json_value_free(v);

  v = test_json_parse("false", 5);
  FIO_ASSERT(v, "JSON false parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FALSE, "JSON false type mismatch");
  test_json_value_free(v);

  /* Test case insensitivity */
  v = test_json_parse("TRUE", 4);
  FIO_ASSERT(v, "JSON TRUE (uppercase) parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_TRUE, "JSON TRUE type mismatch");
  test_json_value_free(v);

  v = test_json_parse("FALSE", 5);
  FIO_ASSERT(v, "JSON FALSE (uppercase) parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FALSE, "JSON FALSE type mismatch");
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_numbers(void) {
  fprintf(stderr, "\t* Testing JSON number parsing.\n");

  /* Positive integers */
  test_json_value_s *v = test_json_parse("0", 1);
  FIO_ASSERT(v, "JSON 0 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON 0 type mismatch");
  FIO_ASSERT(v->data.i == 0,
             "JSON 0 value mismatch: got %lld",
             (long long)v->data.i);
  test_json_value_free(v);

  v = test_json_parse("42", 2);
  FIO_ASSERT(v, "JSON 42 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON 42 type mismatch");
  FIO_ASSERT(v->data.i == 42,
             "JSON 42 value mismatch: got %lld",
             (long long)v->data.i);
  test_json_value_free(v);

  v = test_json_parse("123456789", 9);
  FIO_ASSERT(v, "JSON 123456789 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON 123456789 type mismatch");
  FIO_ASSERT(v->data.i == 123456789,
             "JSON 123456789 value mismatch: got %lld",
             (long long)v->data.i);
  test_json_value_free(v);

  /* Negative integers */
  v = test_json_parse("-1", 2);
  FIO_ASSERT(v, "JSON -1 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON -1 type mismatch");
  FIO_ASSERT(v->data.i == -1,
             "JSON -1 value mismatch: got %lld",
             (long long)v->data.i);
  test_json_value_free(v);

  v = test_json_parse("-42", 3);
  FIO_ASSERT(v, "JSON -42 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON -42 type mismatch");
  FIO_ASSERT(v->data.i == -42,
             "JSON -42 value mismatch: got %lld",
             (long long)v->data.i);
  test_json_value_free(v);

  /* Large numbers */
  v = test_json_parse("9223372036854775807", 19); /* INT64_MAX */
  FIO_ASSERT(v, "JSON INT64_MAX parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON INT64_MAX type mismatch");
  FIO_ASSERT(v->data.i == INT64_MAX,
             "JSON INT64_MAX value mismatch: got %lld",
             (long long)v->data.i);
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_floats(void) {
  fprintf(stderr, "\t* Testing JSON float parsing.\n");

  test_json_value_s *v = test_json_parse("3.14", 4);
  FIO_ASSERT(v, "JSON 3.14 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FLOAT, "JSON 3.14 type mismatch");
  FIO_ASSERT(v->data.f > 3.13 && v->data.f < 3.15,
             "JSON 3.14 value mismatch: got %f",
             v->data.f);
  test_json_value_free(v);

  v = test_json_parse("-2.5", 4);
  FIO_ASSERT(v, "JSON -2.5 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FLOAT, "JSON -2.5 type mismatch");
  FIO_ASSERT(v->data.f > -2.6 && v->data.f < -2.4,
             "JSON -2.5 value mismatch: got %f",
             v->data.f);
  test_json_value_free(v);

  /* Scientific notation */
  v = test_json_parse("1e10", 4);
  FIO_ASSERT(v, "JSON 1e10 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FLOAT, "JSON 1e10 type mismatch");
  FIO_ASSERT(v->data.f > 9e9 && v->data.f < 1.1e10,
             "JSON 1e10 value mismatch: got %e",
             v->data.f);
  test_json_value_free(v);

  v = test_json_parse("1E-5", 4);
  FIO_ASSERT(v, "JSON 1E-5 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FLOAT, "JSON 1E-5 type mismatch");
  FIO_ASSERT(v->data.f > 9e-6 && v->data.f < 1.1e-5,
             "JSON 1E-5 value mismatch: got %e",
             v->data.f);
  test_json_value_free(v);

  v = test_json_parse("2.5e+3", 6);
  FIO_ASSERT(v, "JSON 2.5e+3 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FLOAT, "JSON 2.5e+3 type mismatch");
  FIO_ASSERT(v->data.f > 2499 && v->data.f < 2501,
             "JSON 2.5e+3 value mismatch: got %f",
             v->data.f);
  test_json_value_free(v);

  /* Zero with decimal */
  v = test_json_parse("0.0", 3);
  FIO_ASSERT(v, "JSON 0.0 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FLOAT, "JSON 0.0 type mismatch");
  FIO_ASSERT(v->data.f == 0.0, "JSON 0.0 value mismatch: got %f", v->data.f);
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_special_floats(void) {
  fprintf(stderr, "\t* Testing JSON special float values (NaN, Infinity).\n");

  /* NaN - non-standard but supported */
  test_json_value_s *v = test_json_parse("NaN", 3);
  FIO_ASSERT(v, "JSON NaN parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FLOAT, "JSON NaN type mismatch");
  FIO_ASSERT(isnan(v->data.f), "JSON NaN value should be NaN");
  test_json_value_free(v);

  /* Infinity - non-standard but supported */
  v = test_json_parse("Infinity", 8);
  FIO_ASSERT(v, "JSON Infinity parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FLOAT, "JSON Infinity type mismatch");
  FIO_ASSERT(isinf(v->data.f) && v->data.f > 0,
             "JSON Infinity value should be +Infinity");
  test_json_value_free(v);

  v = test_json_parse("-Infinity", 9);
  FIO_ASSERT(v, "JSON -Infinity parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_FLOAT, "JSON -Infinity type mismatch");
  FIO_ASSERT(isinf(v->data.f) && v->data.f < 0,
             "JSON -Infinity value should be -Infinity");
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_strings(void) {
  fprintf(stderr, "\t* Testing JSON string parsing.\n");

  /* Empty string */
  test_json_value_s *v = test_json_parse("\"\"", 2);
  FIO_ASSERT(v, "JSON empty string parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING, "JSON empty string type mismatch");
  FIO_ASSERT(v->data.str.len == 0, "JSON empty string length mismatch");
  test_json_value_free(v);

  /* Simple string */
  v = test_json_parse("\"hello\"", 7);
  FIO_ASSERT(v, "JSON 'hello' parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING, "JSON 'hello' type mismatch");
  FIO_ASSERT(v->data.str.len == 5, "JSON 'hello' length mismatch");
  FIO_ASSERT(!FIO_MEMCMP(v->data.str.buf, "hello", 5),
             "JSON 'hello' value mismatch");
  test_json_value_free(v);

  /* String with spaces */
  v = test_json_parse("\"hello world\"", 13);
  FIO_ASSERT(v, "JSON 'hello world' parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING, "JSON 'hello world' type mismatch");
  FIO_ASSERT(v->data.str.len == 11, "JSON 'hello world' length mismatch");
  FIO_ASSERT(!FIO_MEMCMP(v->data.str.buf, "hello world", 11),
             "JSON 'hello world' value mismatch");
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_escaped_strings(void) {
  fprintf(stderr, "\t* Testing JSON escaped string parsing.\n");

  /* Escaped quote */
  test_json_value_s *v = test_json_parse("\"hello\\\"world\"", 14);
  FIO_ASSERT(v, "JSON escaped quote parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING, "JSON escaped quote type mismatch");
  FIO_ASSERT(v->data.str.len == 11,
             "JSON escaped quote length mismatch: got %zu",
             v->data.str.len);
  FIO_ASSERT(!FIO_MEMCMP(v->data.str.buf, "hello\"world", 11),
             "JSON escaped quote value mismatch: got '%s'",
             v->data.str.buf);
  test_json_value_free(v);

  /* Escaped backslash */
  v = test_json_parse("\"hello\\\\world\"", 14);
  FIO_ASSERT(v, "JSON escaped backslash parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING,
             "JSON escaped backslash type mismatch");
  FIO_ASSERT(v->data.str.len == 11,
             "JSON escaped backslash length mismatch: got %zu",
             v->data.str.len);
  FIO_ASSERT(!FIO_MEMCMP(v->data.str.buf, "hello\\world", 11),
             "JSON escaped backslash value mismatch");
  test_json_value_free(v);

  /* Escaped newline */
  v = test_json_parse("\"hello\\nworld\"", 14);
  FIO_ASSERT(v, "JSON escaped newline parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING, "JSON escaped newline type mismatch");
  FIO_ASSERT(v->data.str.len == 11,
             "JSON escaped newline length mismatch: got %zu",
             v->data.str.len);
  FIO_ASSERT(!FIO_MEMCMP(v->data.str.buf, "hello\nworld", 11),
             "JSON escaped newline value mismatch");
  test_json_value_free(v);

  /* Escaped tab */
  v = test_json_parse("\"hello\\tworld\"", 14);
  FIO_ASSERT(v, "JSON escaped tab parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING, "JSON escaped tab type mismatch");
  FIO_ASSERT(v->data.str.len == 11,
             "JSON escaped tab length mismatch: got %zu",
             v->data.str.len);
  FIO_ASSERT(!FIO_MEMCMP(v->data.str.buf, "hello\tworld", 11),
             "JSON escaped tab value mismatch");
  test_json_value_free(v);

  /* Multiple escapes */
  v = test_json_parse("\"\\t\\n\\r\"", 8);
  FIO_ASSERT(v, "JSON multiple escapes parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING,
             "JSON multiple escapes type mismatch");
  FIO_ASSERT(v->data.str.len == 3,
             "JSON multiple escapes length mismatch: got %zu",
             v->data.str.len);
  FIO_ASSERT(!FIO_MEMCMP(v->data.str.buf, "\t\n\r", 3),
             "JSON multiple escapes value mismatch");
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_unicode_strings(void) {
  fprintf(stderr, "\t* Testing JSON unicode escape sequences.\n");

  /* Simple unicode escape - ASCII 'A' */
  test_json_value_s *v = test_json_parse("\"\\u0041\"", 8);
  FIO_ASSERT(v, "JSON unicode \\u0041 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING, "JSON unicode type mismatch");
  FIO_ASSERT(v->data.str.len >= 1,
             "JSON unicode length mismatch: got %zu",
             v->data.str.len);
  FIO_ASSERT(v->data.str.buf[0] == 'A',
             "JSON unicode value mismatch: expected 'A', got '%c'",
             v->data.str.buf[0]);
  test_json_value_free(v);

  /* Euro sign (U+20AC) - should be encoded as UTF-8 */
  v = test_json_parse("\"\\u20AC\"", 8);
  FIO_ASSERT(v, "JSON unicode euro sign parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_STRING, "JSON unicode euro type mismatch");
  /* Euro sign in UTF-8 is 3 bytes: 0xE2 0x82 0xAC */
  FIO_ASSERT(v->data.str.len == 3,
             "JSON unicode euro length mismatch: got %zu",
             v->data.str.len);
  test_json_value_free(v);
}

/* *****************************************************************************
Test: Arrays
***************************************************************************** */

FIO_SFUNC void fio___test_json_empty_array(void) {
  fprintf(stderr, "\t* Testing JSON empty array parsing.\n");

  test_json_value_s *v = test_json_parse("[]", 2);
  FIO_ASSERT(v, "JSON empty array parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY, "JSON empty array type mismatch");
  FIO_ASSERT(v->data.arr.count == 0, "JSON empty array count mismatch");
  test_json_value_free(v);

  /* With whitespace */
  v = test_json_parse("[  ]", 4);
  FIO_ASSERT(v, "JSON empty array with whitespace parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY,
             "JSON empty array with whitespace type mismatch");
  FIO_ASSERT(v->data.arr.count == 0,
             "JSON empty array with whitespace count mismatch");
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_simple_array(void) {
  fprintf(stderr, "\t* Testing JSON simple array parsing.\n");

  /* Single element */
  test_json_value_s *v = test_json_parse("[1]", 3);
  FIO_ASSERT(v, "JSON single element array parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY,
             "JSON single element array type mismatch");
  FIO_ASSERT(v->data.arr.count == 1,
             "JSON single element array count mismatch");
  FIO_ASSERT(v->data.arr.items[0]->type == TEST_JSON_NUMBER,
             "JSON array element type mismatch");
  FIO_ASSERT(v->data.arr.items[0]->data.i == 1,
             "JSON array element value mismatch");
  test_json_value_free(v);

  /* Multiple elements */
  v = test_json_parse("[1, 2, 3]", 9);
  FIO_ASSERT(v, "JSON multi-element array parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY,
             "JSON multi-element array type mismatch");
  FIO_ASSERT(v->data.arr.count == 3,
             "JSON multi-element array count mismatch: got %zu",
             v->data.arr.count);
  for (size_t i = 0; i < 3; ++i) {
    FIO_ASSERT(v->data.arr.items[i]->type == TEST_JSON_NUMBER,
               "JSON array element %zu type mismatch",
               i);
    FIO_ASSERT(v->data.arr.items[i]->data.i == (int64_t)(i + 1),
               "JSON array element %zu value mismatch",
               i);
  }
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_mixed_array(void) {
  fprintf(stderr, "\t* Testing JSON mixed type array parsing.\n");

  test_json_value_s *v =
      test_json_parse("[1, \"hello\", true, null, 3.14]", 30);
  FIO_ASSERT(v, "JSON mixed array parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY, "JSON mixed array type mismatch");
  FIO_ASSERT(v->data.arr.count == 5,
             "JSON mixed array count mismatch: got %zu",
             v->data.arr.count);

  FIO_ASSERT(v->data.arr.items[0]->type == TEST_JSON_NUMBER,
             "JSON mixed array[0] type mismatch");
  FIO_ASSERT(v->data.arr.items[0]->data.i == 1,
             "JSON mixed array[0] value mismatch");

  FIO_ASSERT(v->data.arr.items[1]->type == TEST_JSON_STRING,
             "JSON mixed array[1] type mismatch");
  FIO_ASSERT(!FIO_MEMCMP(v->data.arr.items[1]->data.str.buf, "hello", 5),
             "JSON mixed array[1] value mismatch");

  FIO_ASSERT(v->data.arr.items[2]->type == TEST_JSON_TRUE,
             "JSON mixed array[2] type mismatch");

  FIO_ASSERT(v->data.arr.items[3]->type == TEST_JSON_NULL,
             "JSON mixed array[3] type mismatch");

  FIO_ASSERT(v->data.arr.items[4]->type == TEST_JSON_FLOAT,
             "JSON mixed array[4] type mismatch");

  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_nested_array(void) {
  fprintf(stderr, "\t* Testing JSON nested array parsing.\n");

  test_json_value_s *v = test_json_parse("[[1, 2], [3, 4]]", 16);
  FIO_ASSERT(v, "JSON nested array parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY, "JSON nested array type mismatch");
  FIO_ASSERT(v->data.arr.count == 2,
             "JSON nested array count mismatch: got %zu",
             v->data.arr.count);

  FIO_ASSERT(v->data.arr.items[0]->type == TEST_JSON_ARRAY,
             "JSON nested array[0] type mismatch");
  FIO_ASSERT(v->data.arr.items[0]->data.arr.count == 2,
             "JSON nested array[0] count mismatch");

  FIO_ASSERT(v->data.arr.items[1]->type == TEST_JSON_ARRAY,
             "JSON nested array[1] type mismatch");
  FIO_ASSERT(v->data.arr.items[1]->data.arr.count == 2,
             "JSON nested array[1] count mismatch");

  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_trailing_comma_array(void) {
  fprintf(stderr, "\t* Testing JSON array with trailing comma (non-strict).\n");

  /* facil.io JSON parser supports trailing commas */
  test_json_value_s *v = test_json_parse("[1, 2, 3,]", 10);
  FIO_ASSERT(v, "JSON array with trailing comma parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY,
             "JSON array with trailing comma type mismatch");
  FIO_ASSERT(v->data.arr.count == 3,
             "JSON array with trailing comma count mismatch: got %zu",
             v->data.arr.count);
  test_json_value_free(v);
}

/* *****************************************************************************
Test: Objects
***************************************************************************** */

FIO_SFUNC void fio___test_json_empty_object(void) {
  fprintf(stderr, "\t* Testing JSON empty object parsing.\n");

  test_json_value_s *v = test_json_parse("{}", 2);
  FIO_ASSERT(v, "JSON empty object parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP, "JSON empty object type mismatch");
  FIO_ASSERT(v->data.map.count == 0, "JSON empty object count mismatch");
  test_json_value_free(v);

  /* With whitespace */
  v = test_json_parse("{  }", 4);
  FIO_ASSERT(v, "JSON empty object with whitespace parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP,
             "JSON empty object with whitespace type mismatch");
  FIO_ASSERT(v->data.map.count == 0,
             "JSON empty object with whitespace count mismatch");
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_simple_object(void) {
  fprintf(stderr, "\t* Testing JSON simple object parsing.\n");

  /* Single key-value */
  test_json_value_s *v = test_json_parse("{\"key\": 42}", 11);
  FIO_ASSERT(v, "JSON single key-value object parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP,
             "JSON single key-value object type mismatch");
  FIO_ASSERT(v->data.map.count == 1,
             "JSON single key-value object count mismatch");
  FIO_ASSERT(v->data.map.keys[0]->type == TEST_JSON_STRING,
             "JSON object key type mismatch");
  FIO_ASSERT(!FIO_MEMCMP(v->data.map.keys[0]->data.str.buf, "key", 3),
             "JSON object key value mismatch");
  FIO_ASSERT(v->data.map.values[0]->type == TEST_JSON_NUMBER,
             "JSON object value type mismatch");
  FIO_ASSERT(v->data.map.values[0]->data.i == 42, "JSON object value mismatch");
  test_json_value_free(v);

  /* Multiple key-values */
  v = test_json_parse("{\"a\": 1, \"b\": 2, \"c\": 3}", 24);
  FIO_ASSERT(v, "JSON multi key-value object parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP,
             "JSON multi key-value object type mismatch");
  FIO_ASSERT(v->data.map.count == 3,
             "JSON multi key-value object count mismatch: got %zu",
             v->data.map.count);
  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_nested_object(void) {
  fprintf(stderr, "\t* Testing JSON nested object parsing.\n");

  test_json_value_s *v = test_json_parse("{\"outer\": {\"inner\": 42}}", 24);
  FIO_ASSERT(v, "JSON nested object parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP, "JSON nested object type mismatch");
  FIO_ASSERT(v->data.map.count == 1, "JSON nested object count mismatch");

  FIO_ASSERT(v->data.map.values[0]->type == TEST_JSON_MAP,
             "JSON nested inner object type mismatch");
  FIO_ASSERT(v->data.map.values[0]->data.map.count == 1,
             "JSON nested inner object count mismatch");
  FIO_ASSERT(v->data.map.values[0]->data.map.values[0]->type ==
                 TEST_JSON_NUMBER,
             "JSON nested inner value type mismatch");
  FIO_ASSERT(v->data.map.values[0]->data.map.values[0]->data.i == 42,
             "JSON nested inner value mismatch");

  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_trailing_comma_object(void) {
  fprintf(stderr,
          "\t* Testing JSON object with trailing comma (non-strict).\n");

  /* facil.io JSON parser supports trailing commas */
  test_json_value_s *v = test_json_parse("{\"a\": 1, \"b\": 2,}", 17);
  FIO_ASSERT(v, "JSON object with trailing comma parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP,
             "JSON object with trailing comma type mismatch");
  FIO_ASSERT(v->data.map.count == 2,
             "JSON object with trailing comma count mismatch: got %zu",
             v->data.map.count);
  test_json_value_free(v);
}

/* *****************************************************************************
Test: Mixed Nested Structures
***************************************************************************** */

FIO_SFUNC void fio___test_json_object_with_array(void) {
  fprintf(stderr, "\t* Testing JSON object containing array.\n");

  test_json_value_s *v = test_json_parse("{\"numbers\": [1, 2, 3]}", 22);
  FIO_ASSERT(v, "JSON object with array parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP, "JSON object with array type mismatch");
  FIO_ASSERT(v->data.map.count == 1, "JSON object with array count mismatch");

  FIO_ASSERT(v->data.map.values[0]->type == TEST_JSON_ARRAY,
             "JSON object's array value type mismatch");
  FIO_ASSERT(v->data.map.values[0]->data.arr.count == 3,
             "JSON object's array count mismatch");

  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_array_with_object(void) {
  fprintf(stderr, "\t* Testing JSON array containing objects.\n");

  test_json_value_s *v = test_json_parse("[{\"a\": 1}, {\"b\": 2}]", 20);
  FIO_ASSERT(v, "JSON array with objects parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY,
             "JSON array with objects type mismatch");
  FIO_ASSERT(v->data.arr.count == 2, "JSON array with objects count mismatch");

  FIO_ASSERT(v->data.arr.items[0]->type == TEST_JSON_MAP,
             "JSON array's object[0] type mismatch");
  FIO_ASSERT(v->data.arr.items[1]->type == TEST_JSON_MAP,
             "JSON array's object[1] type mismatch");

  test_json_value_free(v);
}

FIO_SFUNC void fio___test_json_complex_nested(void) {
  fprintf(stderr, "\t* Testing JSON complex nested structure.\n");

  const char *json = "{"
                     "\"name\": \"test\","
                     "\"values\": [1, 2, {\"nested\": true}],"
                     "\"config\": {\"enabled\": false, \"count\": 42}"
                     "}";
  size_t len = FIO_STRLEN(json);

  test_json_value_s *v = test_json_parse(json, len);
  FIO_ASSERT(v, "JSON complex nested parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP, "JSON complex nested type mismatch");
  FIO_ASSERT(v->data.map.count == 3,
             "JSON complex nested count mismatch: got %zu",
             v->data.map.count);

  test_json_value_free(v);
}

/* *****************************************************************************
Test: Whitespace Handling
***************************************************************************** */

FIO_SFUNC void fio___test_json_whitespace(void) {
  fprintf(stderr, "\t* Testing JSON whitespace handling.\n");

  /* Leading whitespace */
  test_json_value_s *v = test_json_parse("   42", 5);
  FIO_ASSERT(v, "JSON with leading whitespace parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER,
             "JSON with leading whitespace type mismatch");
  FIO_ASSERT(v->data.i == 42, "JSON with leading whitespace value mismatch");
  test_json_value_free(v);

  /* Newlines and tabs - note: whitespace before colon may not be supported */
  {
    const char *ws_json = "{\n\t\"key\":\n\t42\n}";
    v = test_json_parse(ws_json, FIO_STRLEN(ws_json));
    FIO_ASSERT(v, "JSON with newlines/tabs parsing failed");
    FIO_ASSERT(v->type == TEST_JSON_MAP,
               "JSON with newlines/tabs type mismatch");
    test_json_value_free(v);
  }

  /* Multiple spaces between elements */
  v = test_json_parse("[  1  ,  2  ,  3  ]", 19);
  FIO_ASSERT(v, "JSON with extra spaces parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY,
             "JSON with extra spaces type mismatch");
  FIO_ASSERT(v->data.arr.count == 3, "JSON with extra spaces count mismatch");
  test_json_value_free(v);
}

/* *****************************************************************************
Test: Comments (non-standard)
***************************************************************************** */

FIO_SFUNC void fio___test_json_comments(void) {
  fprintf(stderr, "\t* Testing JSON comment support (non-standard).\n");

  /* C-style single line comment */
  test_json_value_s *v = test_json_parse("// comment\n42", 13);
  FIO_ASSERT(v, "JSON with // comment parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON with // comment type mismatch");
  FIO_ASSERT(v->data.i == 42, "JSON with // comment value mismatch");
  test_json_value_free(v);

  /* C-style block comment */
  v = test_json_parse("/* comment */42", 15);
  FIO_ASSERT(v, "JSON with /* */ comment parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER,
             "JSON with /* */ comment type mismatch");
  FIO_ASSERT(v->data.i == 42, "JSON with /* */ comment value mismatch");
  test_json_value_free(v);

  /* Hash comment (Ruby/Python style) */
  v = test_json_parse("# comment\n42", 12);
  FIO_ASSERT(v, "JSON with # comment parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON with # comment type mismatch");
  FIO_ASSERT(v->data.i == 42, "JSON with # comment value mismatch");
  test_json_value_free(v);
}

/* *****************************************************************************
Test: Hex and Binary Numbers (non-standard)
***************************************************************************** */

FIO_SFUNC void fio___test_json_hex_binary(void) {
  fprintf(stderr,
          "\t* Testing JSON hex/binary number support (non-standard).\n");

  /* Hex number */
  test_json_value_s *v = test_json_parse("0xFF", 4);
  FIO_ASSERT(v, "JSON hex 0xFF parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON hex type mismatch");
  FIO_ASSERT(v->data.i == 255,
             "JSON hex value mismatch: got %lld",
             (long long)v->data.i);
  test_json_value_free(v);

  /* Binary number */
  v = test_json_parse("0b1010", 6);
  FIO_ASSERT(v, "JSON binary 0b1010 parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON binary type mismatch");
  FIO_ASSERT(v->data.i == 10,
             "JSON binary value mismatch: got %lld",
             (long long)v->data.i);
  test_json_value_free(v);
}

/* *****************************************************************************
Test: Error Cases
***************************************************************************** */

FIO_SFUNC void fio___test_json_errors(void) {
  fprintf(stderr, "\t* Testing JSON error handling.\n");

  /* Empty input */
  test_json_value_s *v = test_json_parse("", 0);
  FIO_ASSERT(!v, "JSON empty input should fail");

  /* Incomplete string */
  v = test_json_parse("\"hello", 6);
  FIO_ASSERT(!v, "JSON incomplete string should fail");

  /* Incomplete array */
  v = test_json_parse("[1, 2", 5);
  FIO_ASSERT(!v, "JSON incomplete array should fail");

  /* Incomplete object */
  v = test_json_parse("{\"key\":", 7);
  FIO_ASSERT(!v, "JSON incomplete object should fail");

  /* Invalid token */
  v = test_json_parse("undefined", 9);
  FIO_ASSERT(!v, "JSON 'undefined' should fail");

  /* Missing colon in object */
  v = test_json_parse("{\"key\" 42}", 10);
  FIO_ASSERT(!v, "JSON missing colon should fail");

  /* Truncated null */
  v = test_json_parse("nul", 3);
  FIO_ASSERT(!v, "JSON truncated 'nul' should fail");

  /* Truncated true */
  v = test_json_parse("tru", 3);
  FIO_ASSERT(!v, "JSON truncated 'tru' should fail");

  /* Truncated false */
  v = test_json_parse("fals", 4);
  FIO_ASSERT(!v, "JSON truncated 'fals' should fail");
}

/* *****************************************************************************
Test: BOM Handling
***************************************************************************** */

FIO_SFUNC void fio___test_json_bom(void) {
  fprintf(stderr, "\t* Testing JSON BOM (Byte Order Mark) handling.\n");

  /* UTF-8 BOM followed by JSON */
  const char json_with_bom[] = "\xEF\xBB\xBF"
                               "42";
  test_json_value_s *v = test_json_parse(json_with_bom, 5);
  FIO_ASSERT(v, "JSON with BOM parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_NUMBER, "JSON with BOM type mismatch");
  FIO_ASSERT(v->data.i == 42, "JSON with BOM value mismatch");
  test_json_value_free(v);

  /* BOM only (should succeed with no value) */
  const char bom_only[] = "\xEF\xBB\xBF";
  fio_json_result_s r = fio_json_parse(&test_json_callbacks, bom_only, 3);
  FIO_ASSERT(!r.err, "JSON BOM-only should not error");
  if (r.ctx)
    test_json_value_free((test_json_value_s *)r.ctx);
}

/* *****************************************************************************
Test: Stop Position
***************************************************************************** */

FIO_SFUNC void fio___test_json_stop_position(void) {
  fprintf(stderr, "\t* Testing JSON stop position reporting.\n");

  /* Parse stops after first complete value */
  const char *json = "42 extra";
  fio_json_result_s r = fio_json_parse(&test_json_callbacks, json, 8);
  FIO_ASSERT(!r.err, "JSON parsing should succeed");
  FIO_ASSERT(r.stop_pos == 2,
             "JSON stop position mismatch: expected 2, got %zu",
             r.stop_pos);
  if (r.ctx)
    test_json_value_free((test_json_value_s *)r.ctx);

  /* Array parsing */
  json = "[1,2,3]more";
  r = fio_json_parse(&test_json_callbacks, json, 11);
  FIO_ASSERT(!r.err, "JSON array parsing should succeed");
  FIO_ASSERT(r.stop_pos == 7,
             "JSON array stop position mismatch: expected 7, got %zu",
             r.stop_pos);
  if (r.ctx)
    test_json_value_free((test_json_value_s *)r.ctx);
}

/* *****************************************************************************
Test: Depth Limits
***************************************************************************** */

FIO_SFUNC void fio___test_json_depth(void) {
  fprintf(stderr, "\t* Testing JSON nesting depth handling.\n");

  /* Moderately nested structure (should succeed) */
  const char *nested_10 = "[[[[[[[[[[42]]]]]]]]]]";
  test_json_value_s *v = test_json_parse(nested_10, FIO_STRLEN(nested_10));
  FIO_ASSERT(v, "JSON 10-level nested array parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY, "JSON nested array type mismatch");
  test_json_value_free(v);

  /* Deeply nested objects (should succeed within limit) */
  const char *nested_obj = "{\"a\":{\"b\":{\"c\":{\"d\":{\"e\":42}}}}}";
  v = test_json_parse(nested_obj, FIO_STRLEN(nested_obj));
  FIO_ASSERT(v, "JSON 5-level nested object parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP, "JSON nested object type mismatch");
  test_json_value_free(v);
}

/* *****************************************************************************
Test: Large Arrays
***************************************************************************** */

FIO_SFUNC void fio___test_json_large_array(void) {
  fprintf(stderr, "\t* Testing JSON large array parsing.\n");

  /* Build a large array string */
  char buf[4096];
  fio_str_info_s s = FIO_STR_INFO3(buf, 0, sizeof(buf));
  fio_string_write(&s, NULL, "[", 1);
  for (int i = 0; i < 100; ++i) {
    if (i > 0)
      fio_string_write(&s, NULL, ",", 1);
    fio_string_write_i(&s, NULL, i);
  }
  fio_string_write(&s, NULL, "]", 1);

  test_json_value_s *v = test_json_parse(buf, s.len);
  FIO_ASSERT(v, "JSON large array parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_ARRAY, "JSON large array type mismatch");
  FIO_ASSERT(v->data.arr.count == 100,
             "JSON large array count mismatch: got %zu",
             v->data.arr.count);

  /* Verify first and last elements */
  FIO_ASSERT(v->data.arr.items[0]->data.i == 0,
             "JSON large array first element mismatch");
  FIO_ASSERT(v->data.arr.items[99]->data.i == 99,
             "JSON large array last element mismatch");

  test_json_value_free(v);
}

/* *****************************************************************************
Test: Real-world JSON Examples
***************************************************************************** */

FIO_SFUNC void fio___test_json_realworld(void) {
  fprintf(stderr, "\t* Testing JSON real-world examples.\n");

  /* Package.json style */
  const char *package_json = "{"
                             "\"name\": \"my-package\","
                             "\"version\": \"1.0.0\","
                             "\"dependencies\": {"
                             "\"lodash\": \"^4.17.21\""
                             "},"
                             "\"scripts\": {"
                             "\"test\": \"echo test\","
                             "\"build\": \"echo build\""
                             "}"
                             "}";

  test_json_value_s *v =
      test_json_parse(package_json, FIO_STRLEN(package_json));
  FIO_ASSERT(v, "JSON package.json style parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP, "JSON package.json type mismatch");
  FIO_ASSERT(v->data.map.count == 4,
             "JSON package.json count mismatch: got %zu",
             v->data.map.count);
  test_json_value_free(v);

  /* API response style */
  const char *api_response = "{"
                             "\"status\": \"success\","
                             "\"code\": 200,"
                             "\"data\": {"
                             "\"users\": ["
                             "{\"id\": 1, \"name\": \"Alice\"},"
                             "{\"id\": 2, \"name\": \"Bob\"}"
                             "],"
                             "\"total\": 2"
                             "},"
                             "\"meta\": {"
                             "\"page\": 1,"
                             "\"per_page\": 10"
                             "}"
                             "}";

  v = test_json_parse(api_response, FIO_STRLEN(api_response));
  FIO_ASSERT(v, "JSON API response style parsing failed");
  FIO_ASSERT(v->type == TEST_JSON_MAP, "JSON API response type mismatch");
  test_json_value_free(v);
}

/* *****************************************************************************
Main Test Entry Point
***************************************************************************** */

FIO_SFUNC void fio_test_json(void) {
  fprintf(stderr, "* Testing JSON parser.\n");

  /* Basic types */
  fio___test_json_null();
  fio___test_json_booleans();
  fio___test_json_numbers();
  fio___test_json_floats();
  fio___test_json_special_floats();
  fio___test_json_strings();
  fio___test_json_escaped_strings();
  fio___test_json_unicode_strings();

  /* Arrays */
  fio___test_json_empty_array();
  fio___test_json_simple_array();
  fio___test_json_mixed_array();
  fio___test_json_nested_array();
  fio___test_json_trailing_comma_array();

  /* Objects */
  fio___test_json_empty_object();
  fio___test_json_simple_object();
  fio___test_json_nested_object();
  fio___test_json_trailing_comma_object();

  /* Mixed nested structures */
  fio___test_json_object_with_array();
  fio___test_json_array_with_object();
  fio___test_json_complex_nested();

  /* Whitespace and formatting */
  fio___test_json_whitespace();

  /* Non-standard extensions */
  fio___test_json_comments();
  fio___test_json_hex_binary();

  /* Error handling */
  fio___test_json_errors();

  /* Special cases */
  fio___test_json_bom();
  fio___test_json_stop_position();

  /* Stress tests */
  fio___test_json_depth();
  fio___test_json_large_array();

  /* Real-world examples */
  fio___test_json_realworld();

  fprintf(stderr, "* JSON parser tests complete.\n");
}

#ifndef FIO_TEST_ALL
int main(void) {
  fio_test_json();
  return 0;
}
#endif
