/* *****************************************************************************
RESP3 Parser Tests - Context Stack Pattern
***************************************************************************** */
#include "test-helpers.h"

/* *****************************************************************************
Test State - Tracks parsed objects and structure
***************************************************************************** */

/* Simple test object types */
typedef enum {
  TEST_OBJ_NULL,
  TEST_OBJ_BOOL,
  TEST_OBJ_NUMBER,
  TEST_OBJ_DOUBLE,
  TEST_OBJ_BIGNUM,
  TEST_OBJ_STRING,
  TEST_OBJ_ERROR,
  TEST_OBJ_ARRAY,
  TEST_OBJ_MAP,
  TEST_OBJ_SET,
  TEST_OBJ_PUSH,
  TEST_OBJ_ATTR,
} test_obj_type_e;

/* Test object structure */
typedef struct test_obj_s {
  test_obj_type_e type;
  union {
    int bool_val;
    int64_t num_val;
    double dbl_val;
    struct {
      char *data;
      size_t len;
      uint8_t resp_type;
    } str;
    struct {
      struct test_obj_s **items;
      size_t count;
      size_t capacity;
      int64_t declared_len;
    } array;
    struct {
      struct test_obj_s **keys;
      struct test_obj_s **values;
      size_t count;
      size_t capacity;
      int64_t declared_len;
    } map;
  } data;
} test_obj_s;

/* Test state */
typedef struct {
  int null_count;
  int bool_count;
  int number_count;
  int double_count;
  int bignum_count;
  int string_count;
  int error_count;
  int array_count;
  int map_count;
  int set_count;
  int push_count;
  int attr_count;
  int array_push_count;
  int map_push_count;
  int set_push_count;
  int push_push_count;
  int attr_push_count;
  int array_done_count;
  int map_done_count;
  int set_done_count;
  int push_done_count;
  int attr_done_count;
  int free_count;
  int parser_error_count;

  int last_bool;
  int64_t last_number;
  double last_double;
  int64_t last_array_len;
  int64_t last_map_len;
  int64_t last_set_len;
  int64_t last_push_len;
  int64_t last_attr_len;

  char string_buf[4096];
  size_t string_buf_len;
  uint8_t last_string_type;

  char error_buf[1024];
  size_t error_buf_len;
  uint8_t last_error_type;

  char bignum_buf[256];
  size_t bignum_buf_len;

  /* Streaming string state */
  int start_string_count;
  int string_write_count;
  int string_done_count;
  size_t last_start_string_len;
  uint8_t last_start_string_type;
  size_t total_string_write_len;
} test_state_s;

static test_state_s test_state = {0};

static void test_state_reset(void) {
  FIO_MEMSET(&test_state, 0, sizeof(test_state));
}

/* *****************************************************************************
Test Object Management
***************************************************************************** */

static test_obj_s *test_obj_new(test_obj_type_e type) {
  test_obj_s *obj = (test_obj_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*obj), 0);
  if (!obj)
    return NULL;
  FIO_MEMSET(obj, 0, sizeof(*obj));
  obj->type = type;
  return obj;
}

static void test_obj_free(test_obj_s *obj) {
  if (!obj)
    return;

  switch (obj->type) {
  case TEST_OBJ_STRING:
  case TEST_OBJ_ERROR:
  case TEST_OBJ_BIGNUM:
    if (obj->data.str.data)
      FIO_MEM_FREE(obj->data.str.data, obj->data.str.len + 1);
    break;

  case TEST_OBJ_ARRAY:
  case TEST_OBJ_SET:
  case TEST_OBJ_PUSH:
    for (size_t i = 0; i < obj->data.array.count; ++i)
      test_obj_free(obj->data.array.items[i]);
    if (obj->data.array.items)
      FIO_MEM_FREE(obj->data.array.items,
                   obj->data.array.capacity * sizeof(test_obj_s *));
    break;

  case TEST_OBJ_MAP:
  case TEST_OBJ_ATTR:
    for (size_t i = 0; i < obj->data.map.count; ++i) {
      test_obj_free(obj->data.map.keys[i]);
      test_obj_free(obj->data.map.values[i]);
    }
    if (obj->data.map.keys)
      FIO_MEM_FREE(obj->data.map.keys,
                   obj->data.map.capacity * sizeof(test_obj_s *));
    if (obj->data.map.values)
      FIO_MEM_FREE(obj->data.map.values,
                   obj->data.map.capacity * sizeof(test_obj_s *));
    break;

  default: break;
  }

  FIO_MEM_FREE(obj, sizeof(*obj));
}

/* *****************************************************************************
RESP3 Callbacks Implementation
***************************************************************************** */

static void *test_on_null(void *udata) {
  (void)udata;
  ++test_state.null_count;
  return test_obj_new(TEST_OBJ_NULL);
}

static void *test_on_bool(void *udata, int is_true) {
  (void)udata;
  ++test_state.bool_count;
  test_state.last_bool = is_true;
  test_obj_s *obj = test_obj_new(TEST_OBJ_BOOL);
  if (obj)
    obj->data.bool_val = is_true;
  return obj;
}

static void *test_on_number(void *udata, int64_t num) {
  (void)udata;
  ++test_state.number_count;
  test_state.last_number = num;
  test_obj_s *obj = test_obj_new(TEST_OBJ_NUMBER);
  if (obj)
    obj->data.num_val = num;
  return obj;
}

static void *test_on_double(void *udata, double num) {
  (void)udata;
  ++test_state.double_count;
  test_state.last_double = num;
  test_obj_s *obj = test_obj_new(TEST_OBJ_DOUBLE);
  if (obj)
    obj->data.dbl_val = num;
  return obj;
}

static void *test_on_bignum(void *udata, const void *data, size_t len) {
  (void)udata;
  ++test_state.bignum_count;
  if (len < sizeof(test_state.bignum_buf)) {
    FIO_MEMCPY(test_state.bignum_buf, data, len);
    test_state.bignum_buf[len] = '\0';
    test_state.bignum_buf_len = len;
  }
  test_obj_s *obj = test_obj_new(TEST_OBJ_BIGNUM);
  if (obj) {
    obj->data.str.data = (char *)FIO_MEM_REALLOC(NULL, 0, len + 1, 0);
    if (obj->data.str.data) {
      FIO_MEMCPY(obj->data.str.data, data, len);
      obj->data.str.data[len] = '\0';
      obj->data.str.len = len;
    }
  }
  return obj;
}

static void *test_on_string(void *udata,
                            const void *data,
                            size_t len,
                            uint8_t type) {
  (void)udata;
  ++test_state.string_count;
  test_state.last_string_type = type;
  if (len < sizeof(test_state.string_buf)) {
    FIO_MEMCPY(test_state.string_buf, data, len);
    test_state.string_buf[len] = '\0';
    test_state.string_buf_len = len;
  }
  test_obj_s *obj = test_obj_new(TEST_OBJ_STRING);
  if (obj) {
    obj->data.str.data = (char *)FIO_MEM_REALLOC(NULL, 0, len + 1, 0);
    if (obj->data.str.data) {
      FIO_MEMCPY(obj->data.str.data, data, len);
      obj->data.str.data[len] = '\0';
      obj->data.str.len = len;
    }
    obj->data.str.resp_type = type;
  }
  return obj;
}

static void *test_on_error(void *udata,
                           const void *data,
                           size_t len,
                           uint8_t type) {
  (void)udata;
  ++test_state.error_count;
  test_state.last_error_type = type;
  if (data && len < sizeof(test_state.error_buf)) {
    FIO_MEMCPY(test_state.error_buf, data, len);
    test_state.error_buf[len] = '\0';
    test_state.error_buf_len = len;
  }
  test_obj_s *obj = test_obj_new(TEST_OBJ_ERROR);
  if (obj) {
    obj->data.str.data = (char *)FIO_MEM_REALLOC(NULL, 0, len + 1, 0);
    if (obj->data.str.data && data) {
      FIO_MEMCPY(obj->data.str.data, data, len);
      obj->data.str.data[len] = '\0';
      obj->data.str.len = len;
    }
    obj->data.str.resp_type = type;
  }
  return obj;
}

static void *test_on_array(void *udata, void *parent_ctx, int64_t len) {
  (void)udata;
  (void)parent_ctx;
  ++test_state.array_count;
  test_state.last_array_len = len;
  test_obj_s *obj = test_obj_new(TEST_OBJ_ARRAY);
  if (obj) {
    obj->data.array.declared_len = len;
    if (len > 0) {
      obj->data.array.capacity = (size_t)len;
      obj->data.array.items = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.array.capacity * sizeof(test_obj_s *),
          0);
    }
  }
  return obj;
}

static void *test_on_map(void *udata, void *parent_ctx, int64_t len) {
  (void)udata;
  (void)parent_ctx;
  ++test_state.map_count;
  test_state.last_map_len = len;
  test_obj_s *obj = test_obj_new(TEST_OBJ_MAP);
  if (obj) {
    obj->data.map.declared_len = len;
    if (len > 0) {
      obj->data.map.capacity = (size_t)len;
      obj->data.map.keys = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.map.capacity * sizeof(test_obj_s *),
          0);
      obj->data.map.values = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.map.capacity * sizeof(test_obj_s *),
          0);
    }
  }
  return obj;
}

static void *test_on_set(void *udata, void *parent_ctx, int64_t len) {
  (void)udata;
  (void)parent_ctx;
  ++test_state.set_count;
  test_state.last_set_len = len;
  test_obj_s *obj = test_obj_new(TEST_OBJ_SET);
  if (obj) {
    obj->data.array.declared_len = len;
    if (len > 0) {
      obj->data.array.capacity = (size_t)len;
      obj->data.array.items = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.array.capacity * sizeof(test_obj_s *),
          0);
    }
  }
  return obj;
}

static void *test_on_push(void *udata, void *parent_ctx, int64_t len) {
  (void)udata;
  (void)parent_ctx;
  ++test_state.push_count;
  test_state.last_push_len = len;
  test_obj_s *obj = test_obj_new(TEST_OBJ_PUSH);
  if (obj) {
    obj->data.array.declared_len = len;
    if (len > 0) {
      obj->data.array.capacity = (size_t)len;
      obj->data.array.items = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.array.capacity * sizeof(test_obj_s *),
          0);
    }
  }
  return obj;
}

static void *test_on_attr(void *udata, void *parent_ctx, int64_t len) {
  (void)udata;
  (void)parent_ctx;
  ++test_state.attr_count;
  test_state.last_attr_len = len;
  test_obj_s *obj = test_obj_new(TEST_OBJ_ATTR);
  if (obj) {
    obj->data.map.declared_len = len;
    if (len > 0) {
      obj->data.map.capacity = (size_t)len;
      obj->data.map.keys = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.map.capacity * sizeof(test_obj_s *),
          0);
      obj->data.map.values = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.map.capacity * sizeof(test_obj_s *),
          0);
    }
  }
  return obj;
}

static int test_array_push(void *udata, void *ctx, void *value) {
  (void)udata;
  ++test_state.array_push_count;
  test_obj_s *arr = (test_obj_s *)ctx;
  if (!arr || !value)
    return 0;

  if (arr->data.array.count >= arr->data.array.capacity) {
    size_t new_cap =
        arr->data.array.capacity ? arr->data.array.capacity * 2 : 4;
    test_obj_s **new_items = (test_obj_s **)FIO_MEM_REALLOC(
        arr->data.array.items,
        arr->data.array.capacity * sizeof(test_obj_s *),
        new_cap * sizeof(test_obj_s *),
        arr->data.array.count * sizeof(test_obj_s *));
    if (!new_items)
      return -1;
    arr->data.array.items = new_items;
    arr->data.array.capacity = new_cap;
  }

  arr->data.array.items[arr->data.array.count++] = (test_obj_s *)value;
  return 0;
}

static int test_map_push(void *udata, void *ctx, void *key, void *value) {
  (void)udata;
  ++test_state.map_push_count;
  test_obj_s *map = (test_obj_s *)ctx;
  if (!map)
    return 0;

  if (map->data.map.count >= map->data.map.capacity) {
    size_t new_cap = map->data.map.capacity ? map->data.map.capacity * 2 : 4;
    test_obj_s **new_keys = (test_obj_s **)FIO_MEM_REALLOC(
        map->data.map.keys,
        map->data.map.capacity * sizeof(test_obj_s *),
        new_cap * sizeof(test_obj_s *),
        map->data.map.count * sizeof(test_obj_s *));
    test_obj_s **new_values = (test_obj_s **)FIO_MEM_REALLOC(
        map->data.map.values,
        map->data.map.capacity * sizeof(test_obj_s *),
        new_cap * sizeof(test_obj_s *),
        map->data.map.count * sizeof(test_obj_s *));
    if (!new_keys || !new_values)
      return -1;
    map->data.map.keys = new_keys;
    map->data.map.values = new_values;
    map->data.map.capacity = new_cap;
  }

  map->data.map.keys[map->data.map.count] = (test_obj_s *)key;
  map->data.map.values[map->data.map.count] = (test_obj_s *)value;
  ++map->data.map.count;
  return 0;
}

static int test_set_push(void *udata, void *ctx, void *value) {
  (void)udata;
  ++test_state.set_push_count;
  --test_state.array_push_count; /* Will be incremented by array_push */
  return test_array_push(udata, ctx, value);
}

static int test_push_push(void *udata, void *ctx, void *value) {
  (void)udata;
  ++test_state.push_push_count;
  --test_state.array_push_count;
  return test_array_push(udata, ctx, value);
}

static int test_attr_push(void *udata, void *ctx, void *key, void *value) {
  (void)udata;
  ++test_state.attr_push_count;
  --test_state.map_push_count;
  return test_map_push(udata, ctx, key, value);
}

static void *test_array_done(void *udata, void *ctx) {
  (void)udata;
  ++test_state.array_done_count;
  return ctx;
}

static void *test_map_done(void *udata, void *ctx) {
  (void)udata;
  ++test_state.map_done_count;
  return ctx;
}

static void *test_set_done(void *udata, void *ctx) {
  (void)udata;
  ++test_state.set_done_count;
  return ctx;
}

static void *test_push_done(void *udata, void *ctx) {
  (void)udata;
  ++test_state.push_done_count;
  return ctx;
}

static void *test_attr_done(void *udata, void *ctx) {
  (void)udata;
  ++test_state.attr_done_count;
  /* Attributes are metadata; parser discards the returned context. */
  test_obj_free((test_obj_s *)ctx);
  return NULL;
}

static void test_free_unused(void *udata, void *obj) {
  (void)udata;
  ++test_state.free_count;
  test_obj_free((test_obj_s *)obj);
}

static void *test_on_error_protocol(void *udata) {
  (void)udata;
  ++test_state.parser_error_count;
  return NULL;
}

/* Streaming string callbacks */
static void *test_on_start_string(void *udata, size_t len, uint8_t type) {
  (void)udata;
  ++test_state.start_string_count;
  test_state.last_start_string_len = len;
  test_state.last_start_string_type = type;
  /* Create a string object to accumulate data */
  test_obj_s *obj = test_obj_new(TEST_OBJ_STRING);
  if (obj)
    obj->data.str.resp_type = type;
  return obj;
}

static int test_on_string_write(void *udata,
                                void *ctx,
                                const void *data,
                                size_t len) {
  (void)udata;
  ++test_state.string_write_count;
  test_state.total_string_write_len += len;

  test_obj_s *obj = (test_obj_s *)ctx;
  if (!obj || !data || len == 0)
    return 0;

  /* Append data to the string */
  size_t old_len = obj->data.str.len;
  size_t new_len = old_len + len;
  char *new_data =
      (char *)FIO_MEM_REALLOC(obj->data.str.data, old_len + 1, new_len + 1, 0);
  if (!new_data)
    return -1;

  FIO_MEMCPY(new_data + old_len, data, len);
  new_data[new_len] = '\0';
  obj->data.str.data = new_data;
  obj->data.str.len = new_len;
  return 0;
}

static void *test_on_string_done(void *udata, void *ctx, uint8_t type) {
  (void)udata;
  (void)type;
  ++test_state.string_done_count;
  /* Copy to test state buffer for verification */
  test_obj_s *obj = (test_obj_s *)ctx;
  if (obj && obj->data.str.data &&
      obj->data.str.len < sizeof(test_state.string_buf)) {
    FIO_MEMCPY(test_state.string_buf, obj->data.str.data, obj->data.str.len);
    test_state.string_buf[obj->data.str.len] = '\0';
    test_state.string_buf_len = obj->data.str.len;
  }
  return ctx;
}

/* Include the RESP3 parser */
#define FIO_RESP3
#define FIO_ATOL
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Helper: Test callbacks (static const)
***************************************************************************** */

static const fio_resp3_callbacks_s test_callbacks = {
    .on_null = test_on_null,
    .on_bool = test_on_bool,
    .on_number = test_on_number,
    .on_double = test_on_double,
    .on_bignum = test_on_bignum,
    .on_string = test_on_string,
    .on_error = test_on_error,
    .on_array = test_on_array,
    .on_map = test_on_map,
    .on_set = test_on_set,
    .on_push = test_on_push,
    .on_attr = test_on_attr,
    .array_push = test_array_push,
    .map_push = test_map_push,
    .set_push = test_set_push,
    .push_push = test_push_push,
    .attr_push = test_attr_push,
    .array_done = test_array_done,
    .map_done = test_map_done,
    .set_done = test_set_done,
    .push_done = test_push_done,
    .attr_done = test_attr_done,
    .free_unused = test_free_unused,
    .on_error_protocol = test_on_error_protocol,
};

/* Callbacks with streaming string support */
static const fio_resp3_callbacks_s test_streaming_callbacks = {
    .on_null = test_on_null,
    .on_bool = test_on_bool,
    .on_number = test_on_number,
    .on_double = test_on_double,
    .on_bignum = test_on_bignum,
    .on_string = test_on_string,
    .on_error = test_on_error,
    .on_array = test_on_array,
    .on_map = test_on_map,
    .on_set = test_on_set,
    .on_push = test_on_push,
    .on_attr = test_on_attr,
    .array_push = test_array_push,
    .map_push = test_map_push,
    .set_push = test_set_push,
    .push_push = test_push_push,
    .attr_push = test_attr_push,
    .array_done = test_array_done,
    .map_done = test_map_done,
    .set_done = test_set_done,
    .push_done = test_push_done,
    .attr_done = test_attr_done,
    .free_unused = test_free_unused,
    .on_error_protocol = test_on_error_protocol,
    /* Streaming string callbacks */
    .on_start_string = test_on_start_string,
    .on_string_write = test_on_string_write,
    .on_string_done = test_on_string_done,
};

/* *****************************************************************************
Test Functions
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_simple_string)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "+OK\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(test_state.string_count == 1, "Should have 1 string callback");
  FIO_ASSERT(r.obj != NULL, "Should return object");
  FIO_ASSERT(strcmp(test_state.string_buf, "OK") == 0, "String should be 'OK'");
  FIO_ASSERT(test_state.last_string_type == FIO_RESP3_SIMPLE_STR,
             "Type should be simple string");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "+\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(r.consumed == strlen(data), "Empty string consumed");
  FIO_ASSERT(test_state.string_buf_len == 0, "Empty string len 0");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "+hello world\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(strcmp(test_state.string_buf, "hello world") == 0,
             "String with spaces");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_simple_error)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "-ERR unknown command\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.error_count == 1, "Should have 1 error callback");
  FIO_ASSERT(strcmp(test_state.error_buf, "ERR unknown command") == 0,
             "Error message");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_number)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = ":1234\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.number_count == 1, "Should have 1 number callback");
  FIO_ASSERT(test_state.last_number == 1234, "Number should be 1234");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = ":-9876\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(test_state.last_number == -9876, "Negative number");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = ":0\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(test_state.last_number == 0, "Zero");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_null)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "_\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.null_count == 1, "Should have 1 null callback");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_double)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = ",3.14159\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(fabs(test_state.last_double - 3.14159) < 0.00001, "Double value");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = ",inf\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(isinf(test_state.last_double) && test_state.last_double > 0,
             "Positive infinity");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = ",-inf\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(isinf(test_state.last_double) && test_state.last_double < 0,
             "Negative infinity");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = ",nan\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(isnan(test_state.last_double), "NaN");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_bool)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "#t\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(test_state.bool_count == 1, "Should have 1 bool callback");
  FIO_ASSERT(test_state.last_bool == 1, "Should be true");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "#f\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(test_state.last_bool == 0, "Should be false");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_bignum)(void) {
  FIO_LOG_DDEBUG("Testing RESP3 big number (()");

  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "(3492890328409238509324850943850943825024385\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(test_state.bignum_count == 1, "Should have 1 bignum callback");
  FIO_ASSERT(strcmp(test_state.bignum_buf,
                    "3492890328409238509324850943850943825024385") == 0,
             "Big number match");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_blob_string)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "$11\r\nhello world\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(strcmp(test_state.string_buf, "hello world") == 0, "Blob string");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "$0\r\n\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(test_state.string_buf_len == 0, "Empty blob");
  test_obj_free((test_obj_s *)r.obj);

  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "$-1\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(test_state.null_count == 1, "Null blob");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_blob_error)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "!21\r\nSYNTAX invalid syntax\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(test_state.error_count == 1, "Should have 1 error callback");
  FIO_ASSERT(strcmp(test_state.error_buf, "SYNTAX invalid syntax") == 0,
             "Blob error");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_verbatim)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "=15\r\ntxt:Some string\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(strcmp(test_state.string_buf, "txt:Some string") == 0,
             "Verbatim string");
  FIO_ASSERT(test_state.last_string_type == FIO_RESP3_VERBATIM,
             "Verbatim type");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_array)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "*3\r\n:1\r\n:2\r\n:3\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.array_count == 1, "1 array callback");
  FIO_ASSERT(test_state.array_done_count == 1, "1 array done");
  FIO_ASSERT(test_state.number_count == 3, "3 numbers");
  FIO_ASSERT(test_state.array_push_count == 3, "3 pushes");

  test_obj_s *arr = (test_obj_s *)r.obj;
  FIO_ASSERT(arr->type == TEST_OBJ_ARRAY, "Is array");
  FIO_ASSERT(arr->data.array.count == 3, "Has 3 elements");
  FIO_ASSERT(arr->data.array.items[0]->data.num_val == 1, "First is 1");
  FIO_ASSERT(arr->data.array.items[2]->data.num_val == 3, "Third is 3");
  test_obj_free(arr);

  /* Empty array */
  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "*0\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(test_state.last_array_len == 0, "Empty array");
  test_obj_free((test_obj_s *)r.obj);

  /* Null array */
  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "*-1\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(test_state.null_count == 1, "Null array");
  test_obj_free((test_obj_s *)r.obj);

  /* Nested array */
  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "*2\r\n*2\r\n:1\r\n:2\r\n#t\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(test_state.array_count == 2, "2 arrays");
  arr = (test_obj_s *)r.obj;
  FIO_ASSERT(arr->data.array.items[0]->type == TEST_OBJ_ARRAY, "Nested array");
  test_obj_free(arr);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_map)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "%2\r\n+first\r\n:1\r\n+second\r\n:2\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.map_count == 1, "1 map callback");
  FIO_ASSERT(test_state.map_push_count == 2, "2 map pushes");

  test_obj_s *map = (test_obj_s *)r.obj;
  FIO_ASSERT(map->type == TEST_OBJ_MAP, "Is map");
  FIO_ASSERT(map->data.map.count == 2, "Has 2 pairs");
  test_obj_free(map);

  /* Empty map */
  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "%0\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(test_state.map_done_count == 1, "Empty map done");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_set)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "~3\r\n+orange\r\n+apple\r\n:100\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(test_state.set_count == 1, "1 set callback");
  FIO_ASSERT(test_state.set_push_count == 3, "3 set pushes");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_push)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data =
      ">3\r\n+message\r\n+somechannel\r\n+this is the message\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(test_state.push_count == 1, "1 push callback");
  FIO_ASSERT(test_state.push_push_count == 3, "3 push pushes");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_attr)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "|1\r\n+ttl\r\n:3600\r\n:42\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.attr_count == 1, "1 attr callback");
  FIO_ASSERT(test_state.attr_done_count == 1, "1 attr done");
  FIO_ASSERT(test_state.number_count == 2, "2 numbers (attr + value)");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_array)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "*?\r\n:1\r\n:2\r\n:3\r\n.\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.array_count == 1, "1 array");
  FIO_ASSERT(test_state.array_done_count == 1, "1 done");
  FIO_ASSERT(test_state.number_count == 3, "3 numbers");
  FIO_ASSERT(test_state.last_array_len == -1, "Streaming len -1");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_map)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "%?\r\n+a\r\n:1\r\n+b\r\n:2\r\n.\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.map_count == 1, "1 map");
  FIO_ASSERT(test_state.map_done_count == 1, "1 done");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_partial)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  /* Partial blob - header complete but data incomplete */
  const char *part1 = "$11\r\nhello";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, part1, strlen(part1));

  /* Should not consume incomplete blob */
  FIO_ASSERT(r.consumed == 0, "Should not consume incomplete blob");
  FIO_ASSERT(test_state.string_count == 0, "No string yet");

  /* Now provide complete data */
  const char *complete = "$11\r\nhello world\r\n";
  r = fio_resp3_parse(&parser, &test_callbacks, complete, strlen(complete));
  FIO_ASSERT(r.consumed == strlen(complete), "Should consume complete");
  FIO_ASSERT(strcmp(test_state.string_buf, "hello world") == 0, "Got string");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_mixed_array)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "*5\r\n+hello\r\n:42\r\n_\r\n#t\r\n,3.14\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.string_count == 1, "1 string");
  FIO_ASSERT(test_state.number_count == 1, "1 number");
  FIO_ASSERT(test_state.null_count == 1, "1 null");
  FIO_ASSERT(test_state.bool_count == 1, "1 bool");
  FIO_ASSERT(test_state.double_count == 1, "1 double");

  test_obj_s *arr = (test_obj_s *)r.obj;
  FIO_ASSERT(arr->data.array.count == 5, "5 elements");
  FIO_ASSERT(arr->data.array.items[0]->type == TEST_OBJ_STRING, "0 is string");
  FIO_ASSERT(arr->data.array.items[1]->type == TEST_OBJ_NUMBER, "1 is number");
  FIO_ASSERT(arr->data.array.items[2]->type == TEST_OBJ_NULL, "2 is null");
  FIO_ASSERT(arr->data.array.items[3]->type == TEST_OBJ_BOOL, "3 is bool");
  FIO_ASSERT(arr->data.array.items[4]->type == TEST_OBJ_DOUBLE, "4 is double");
  test_obj_free(arr);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_deep_nesting)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "*1\r\n%1\r\n+key\r\n*2\r\n:1\r\n:2\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.array_count == 2, "2 arrays");
  FIO_ASSERT(test_state.map_count == 1, "1 map");

  test_obj_s *arr = (test_obj_s *)r.obj;
  test_obj_s *map = arr->data.array.items[0];
  FIO_ASSERT(map->type == TEST_OBJ_MAP, "Nested map");
  test_obj_s *inner = map->data.map.values[0];
  FIO_ASSERT(inner->type == TEST_OBJ_ARRAY, "Nested array in map");
  FIO_ASSERT(inner->data.array.items[0]->data.num_val == 1, "Inner value 1");
  test_obj_free(arr);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_error_handling)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "X invalid\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(test_state.parser_error_count == 1, "Protocol error callback");
  FIO_ASSERT(parser.error == 1, "Parser error flag");
  FIO_ASSERT(r.err == 1, "Result error flag");
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_udata)(void) {
  int custom_data = 12345;
  fio_resp3_parser_s parser = {.udata = &custom_data};

  static int udata_received = 0;
  static int *expected_ptr = NULL;

  /* Use a simple inline test */
  expected_ptr = &custom_data;
  udata_received = 0;

  /* We'll verify udata is passed by checking test_state which is set by
   * callbacks */
  test_state_reset();
  parser.udata = &test_state;

  const char *data = ":42\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  /* If callbacks were called, test_state was modified */
  test_obj_free((test_obj_s *)r.obj);
  FIO_ASSERT(test_state.number_count == 1, "Callback was called with udata");
  (void)expected_ptr;
  (void)udata_received;
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_context_stack)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "*1\r\n*1\r\n:42\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all");
  FIO_ASSERT(test_state.array_count == 2, "2 arrays");

  test_obj_s *outer = (test_obj_s *)r.obj;
  FIO_ASSERT(outer->type == TEST_OBJ_ARRAY, "Outer is array");
  FIO_ASSERT(outer->data.array.count == 1, "Outer has 1 element");

  test_obj_s *inner = outer->data.array.items[0];
  FIO_ASSERT(inner->type == TEST_OBJ_ARRAY, "Inner is array");
  FIO_ASSERT(inner->data.array.count == 1, "Inner has 1 element");
  FIO_ASSERT(inner->data.array.items[0]->data.num_val == 42, "Value is 42");

  test_obj_free(outer);
}

/* *****************************************************************************
Test: Set-as-Map Fallback
When set callbacks are missing but map callbacks exist, sets should be
treated as maps where each element is both key AND value.
***************************************************************************** */

/* Callbacks for set-as-map test: only map callbacks, no set callbacks */
static void *sam_on_null(void *udata) {
  (void)udata;
  ++test_state.null_count;
  return test_obj_new(TEST_OBJ_NULL);
}

static void *sam_on_number(void *udata, int64_t num) {
  (void)udata;
  ++test_state.number_count;
  test_state.last_number = num;
  test_obj_s *obj = test_obj_new(TEST_OBJ_NUMBER);
  if (obj)
    obj->data.num_val = num;
  return obj;
}

static void *sam_on_string(void *udata,
                           const void *data,
                           size_t len,
                           uint8_t type) {
  (void)udata;
  ++test_state.string_count;
  test_state.last_string_type = type;
  if (len < sizeof(test_state.string_buf)) {
    FIO_MEMCPY(test_state.string_buf, data, len);
    test_state.string_buf[len] = '\0';
    test_state.string_buf_len = len;
  }
  test_obj_s *obj = test_obj_new(TEST_OBJ_STRING);
  if (obj) {
    obj->data.str.data = (char *)FIO_MEM_REALLOC(NULL, 0, len + 1, 0);
    if (obj->data.str.data) {
      FIO_MEMCPY(obj->data.str.data, data, len);
      obj->data.str.data[len] = '\0';
      obj->data.str.len = len;
    }
    obj->data.str.resp_type = type;
  }
  return obj;
}

static void *sam_on_array(void *udata, void *parent_ctx, int64_t len) {
  (void)udata;
  (void)parent_ctx;
  ++test_state.array_count;
  test_state.last_array_len = len;
  test_obj_s *obj = test_obj_new(TEST_OBJ_ARRAY);
  if (obj) {
    obj->data.array.declared_len = len;
    if (len > 0) {
      obj->data.array.capacity = (size_t)len;
      obj->data.array.items = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.array.capacity * sizeof(test_obj_s *),
          0);
    }
  }
  return obj;
}

static void *sam_on_map(void *udata, void *parent_ctx, int64_t len) {
  (void)udata;
  (void)parent_ctx;
  ++test_state.map_count;
  test_state.last_map_len = len;
  test_obj_s *obj = test_obj_new(TEST_OBJ_MAP);
  if (obj) {
    obj->data.map.declared_len = len;
    if (len > 0) {
      obj->data.map.capacity = (size_t)len;
      obj->data.map.keys = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.map.capacity * sizeof(test_obj_s *),
          0);
      obj->data.map.values = (test_obj_s **)FIO_MEM_REALLOC(
          NULL,
          0,
          obj->data.map.capacity * sizeof(test_obj_s *),
          0);
    }
  }
  return obj;
}

static int sam_array_push(void *udata, void *ctx, void *value) {
  (void)udata;
  ++test_state.array_push_count;
  test_obj_s *arr = (test_obj_s *)ctx;
  if (!arr || !value)
    return 0;

  if (arr->data.array.count >= arr->data.array.capacity) {
    size_t new_cap =
        arr->data.array.capacity ? arr->data.array.capacity * 2 : 4;
    test_obj_s **new_items = (test_obj_s **)FIO_MEM_REALLOC(
        arr->data.array.items,
        arr->data.array.capacity * sizeof(test_obj_s *),
        new_cap * sizeof(test_obj_s *),
        arr->data.array.count * sizeof(test_obj_s *));
    if (!new_items)
      return -1;
    arr->data.array.items = new_items;
    arr->data.array.capacity = new_cap;
  }

  arr->data.array.items[arr->data.array.count++] = (test_obj_s *)value;
  return 0;
}

static int sam_map_push(void *udata, void *ctx, void *key, void *value) {
  (void)udata;
  ++test_state.map_push_count;
  test_obj_s *map = (test_obj_s *)ctx;
  if (!map)
    return 0;

  if (map->data.map.count >= map->data.map.capacity) {
    size_t new_cap = map->data.map.capacity ? map->data.map.capacity * 2 : 4;
    test_obj_s **new_keys = (test_obj_s **)FIO_MEM_REALLOC(
        map->data.map.keys,
        map->data.map.capacity * sizeof(test_obj_s *),
        new_cap * sizeof(test_obj_s *),
        map->data.map.count * sizeof(test_obj_s *));
    test_obj_s **new_values = (test_obj_s **)FIO_MEM_REALLOC(
        map->data.map.values,
        map->data.map.capacity * sizeof(test_obj_s *),
        new_cap * sizeof(test_obj_s *),
        map->data.map.count * sizeof(test_obj_s *));
    if (!new_keys || !new_values)
      return -1;
    map->data.map.keys = new_keys;
    map->data.map.values = new_values;
    map->data.map.capacity = new_cap;
  }

  map->data.map.keys[map->data.map.count] = (test_obj_s *)key;
  map->data.map.values[map->data.map.count] = (test_obj_s *)value;
  ++map->data.map.count;
  return 0;
}

static void *sam_array_done(void *udata, void *ctx) {
  (void)udata;
  ++test_state.array_done_count;
  return ctx;
}

static void *sam_map_done(void *udata, void *ctx) {
  (void)udata;
  ++test_state.map_done_count;
  return ctx;
}

static void sam_free_unused(void *udata, void *obj) {
  (void)udata;
  ++test_state.free_count;
  test_obj_free((test_obj_s *)obj);
}

/* Callbacks with map but NO set callbacks - sets should become maps */
static const fio_resp3_callbacks_s set_as_map_callbacks = {
    .on_null = sam_on_null,
    .on_number = sam_on_number,
    .on_string = sam_on_string,
    .on_array = sam_on_array,
    .on_map = sam_on_map,
    /* No on_set, set_push, set_done - sets should fallback to maps */
    .array_push = sam_array_push,
    .map_push = sam_map_push,
    /* No set_push */
    .array_done = sam_array_done,
    .map_done = sam_map_done,
    /* No set_done */
    .free_unused = sam_free_unused,
};

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_set_as_map_fallback)(void) {
  /* Test 1: Simple set with strings should become map with key=value */
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  /* Set: ~3\r\n+apple\r\n+banana\r\n+cherry\r\n */
  const char *data = "~3\r\n+apple\r\n+banana\r\n+cherry\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &set_as_map_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(r.err == 0, "No error");
  FIO_ASSERT(r.obj != NULL, "Should return object");

  /* Should have called on_map (not on_set) */
  FIO_ASSERT(test_state.map_count == 1,
             "Should call on_map (set_as_map fallback)");
  FIO_ASSERT(test_state.set_count == 0, "Should NOT call on_set");

  /* Should have called map_push 3 times (once per element) */
  FIO_ASSERT(test_state.map_push_count == 3,
             "Should call map_push 3 times (once per set element)");
  FIO_ASSERT(test_state.set_push_count == 0, "Should NOT call set_push");

  /* Should have called map_done (not set_done) */
  FIO_ASSERT(test_state.map_done_count == 1,
             "Should call map_done (set_as_map fallback)");
  FIO_ASSERT(test_state.set_done_count == 0, "Should NOT call set_done");

  /* Verify the resulting map structure */
  test_obj_s *map = (test_obj_s *)r.obj;
  FIO_ASSERT(map->type == TEST_OBJ_MAP, "Result should be a map");
  FIO_ASSERT(map->data.map.count == 3, "Map should have 3 entries");

  /* Each key and value should be the same object (pointer equality) */
  FIO_ASSERT(map->data.map.keys[0] == map->data.map.values[0],
             "First key and value should be same object");
  FIO_ASSERT(map->data.map.keys[1] == map->data.map.values[1],
             "Second key and value should be same object");
  FIO_ASSERT(map->data.map.keys[2] == map->data.map.values[2],
             "Third key and value should be same object");

  /* Verify the string values */
  FIO_ASSERT(map->data.map.keys[0]->type == TEST_OBJ_STRING, "Key 0 is string");
  FIO_ASSERT(strcmp(map->data.map.keys[0]->data.str.data, "apple") == 0,
             "Key 0 is 'apple'");
  FIO_ASSERT(strcmp(map->data.map.keys[1]->data.str.data, "banana") == 0,
             "Key 1 is 'banana'");
  FIO_ASSERT(strcmp(map->data.map.keys[2]->data.str.data, "cherry") == 0,
             "Key 2 is 'cherry'");

  /* Free - note: since key==value, we only free once per entry */
  /* The test_obj_free for maps frees both keys and values separately,
     but since they're the same pointer, we need to be careful.
     For this test, we'll just free the map structure without double-free. */
  for (size_t i = 0; i < map->data.map.count; ++i) {
    /* Only free once since key == value */
    test_obj_free(map->data.map.keys[i]);
    map->data.map.keys[i] = NULL;
    map->data.map.values[i] = NULL;
  }
  if (map->data.map.keys)
    FIO_MEM_FREE(map->data.map.keys,
                 map->data.map.capacity * sizeof(test_obj_s *));
  if (map->data.map.values)
    FIO_MEM_FREE(map->data.map.values,
                 map->data.map.capacity * sizeof(test_obj_s *));
  FIO_MEM_FREE(map, sizeof(*map));

  /* Test 2: Empty set should become empty map */
  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "~0\r\n";
  r = fio_resp3_parse(&parser, &set_as_map_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Empty set consumed");
  FIO_ASSERT(test_state.map_count == 1, "Empty set calls on_map");
  FIO_ASSERT(test_state.map_done_count == 1, "Empty set calls map_done");
  FIO_ASSERT(test_state.set_count == 0, "Empty set does NOT call on_set");
  test_obj_free((test_obj_s *)r.obj);

  /* Test 3: Set with numbers */
  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "~2\r\n:42\r\n:100\r\n";
  r = fio_resp3_parse(&parser, &set_as_map_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Number set consumed");
  FIO_ASSERT(test_state.map_count == 1, "Number set calls on_map");
  FIO_ASSERT(test_state.map_push_count == 2, "Number set has 2 map_push calls");
  FIO_ASSERT(test_state.number_count == 2, "2 numbers parsed");

  map = (test_obj_s *)r.obj;
  FIO_ASSERT(map->data.map.keys[0] == map->data.map.values[0],
             "Number key==value");
  FIO_ASSERT(map->data.map.keys[0]->data.num_val == 42, "First number is 42");
  FIO_ASSERT(map->data.map.keys[1]->data.num_val == 100,
             "Second number is 100");

  /* Free carefully */
  for (size_t i = 0; i < map->data.map.count; ++i) {
    test_obj_free(map->data.map.keys[i]);
    map->data.map.keys[i] = NULL;
    map->data.map.values[i] = NULL;
  }
  if (map->data.map.keys)
    FIO_MEM_FREE(map->data.map.keys,
                 map->data.map.capacity * sizeof(test_obj_s *));
  if (map->data.map.values)
    FIO_MEM_FREE(map->data.map.values,
                 map->data.map.capacity * sizeof(test_obj_s *));
  FIO_MEM_FREE(map, sizeof(*map));

  /* Test 4: Nested set inside array */
  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "*2\r\n~2\r\n+a\r\n+b\r\n:99\r\n";
  r = fio_resp3_parse(&parser, &set_as_map_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Nested set consumed");
  FIO_ASSERT(test_state.array_count == 1, "1 array");
  FIO_ASSERT(test_state.map_count == 1, "Nested set becomes map");
  FIO_ASSERT(test_state.set_count == 0, "No set callbacks");

  test_obj_s *arr = (test_obj_s *)r.obj;
  FIO_ASSERT(arr->type == TEST_OBJ_ARRAY, "Outer is array");
  FIO_ASSERT(arr->data.array.count == 2, "Array has 2 elements");
  FIO_ASSERT(arr->data.array.items[0]->type == TEST_OBJ_MAP,
             "First element is map (from set)");
  FIO_ASSERT(arr->data.array.items[1]->type == TEST_OBJ_NUMBER,
             "Second element is number");

  /* Free the nested structure - need to handle the map specially */
  map = arr->data.array.items[0];
  for (size_t i = 0; i < map->data.map.count; ++i) {
    test_obj_free(map->data.map.keys[i]);
    map->data.map.keys[i] = NULL;
    map->data.map.values[i] = NULL;
  }
  if (map->data.map.keys)
    FIO_MEM_FREE(map->data.map.keys,
                 map->data.map.capacity * sizeof(test_obj_s *));
  if (map->data.map.values)
    FIO_MEM_FREE(map->data.map.values,
                 map->data.map.capacity * sizeof(test_obj_s *));
  FIO_MEM_FREE(map, sizeof(*map));
  arr->data.array.items[0] = NULL;

  test_obj_free(arr->data.array.items[1]);
  arr->data.array.items[1] = NULL;
  if (arr->data.array.items)
    FIO_MEM_FREE(arr->data.array.items,
                 arr->data.array.capacity * sizeof(test_obj_s *));
  FIO_MEM_FREE(arr, sizeof(*arr));

  /* Test 5: Streaming set (~?) should also become map */
  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "~?\r\n+x\r\n+y\r\n.\r\n";
  r = fio_resp3_parse(&parser, &set_as_map_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Streaming set consumed");
  FIO_ASSERT(test_state.map_count == 1, "Streaming set calls on_map");
  FIO_ASSERT(test_state.map_push_count == 2,
             "Streaming set has 2 map_push calls");
  FIO_ASSERT(test_state.map_done_count == 1, "Streaming set calls map_done");
  FIO_ASSERT(test_state.set_count == 0, "Streaming set does NOT call on_set");

  map = (test_obj_s *)r.obj;
  FIO_ASSERT(map->type == TEST_OBJ_MAP, "Streaming set result is map");
  FIO_ASSERT(map->data.map.count == 2, "Streaming set map has 2 entries");
  FIO_ASSERT(map->data.map.keys[0] == map->data.map.values[0],
             "Streaming set key==value");

  /* Free carefully */
  for (size_t i = 0; i < map->data.map.count; ++i) {
    test_obj_free(map->data.map.keys[i]);
    map->data.map.keys[i] = NULL;
    map->data.map.values[i] = NULL;
  }
  if (map->data.map.keys)
    FIO_MEM_FREE(map->data.map.keys,
                 map->data.map.capacity * sizeof(test_obj_s *));
  if (map->data.map.values)
    FIO_MEM_FREE(map->data.map.values,
                 map->data.map.capacity * sizeof(test_obj_s *));
  FIO_MEM_FREE(map, sizeof(*map));
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_set_with_callbacks)(void) {
  /* When set callbacks ARE provided, sets should work normally */
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "~3\r\n+apple\r\n+banana\r\n+cherry\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(test_state.set_count == 1, "Should call on_set");
  FIO_ASSERT(test_state.set_push_count == 3, "Should call set_push 3 times");
  FIO_ASSERT(test_state.set_done_count == 1, "Should call set_done");
  FIO_ASSERT(test_state.map_count == 0, "Should NOT call on_map");
  FIO_ASSERT(test_state.map_push_count == 0, "Should NOT call map_push");

  test_obj_s *set = (test_obj_s *)r.obj;
  FIO_ASSERT(set->type == TEST_OBJ_SET, "Result should be a set");
  FIO_ASSERT(set->data.array.count == 3, "Set should have 3 elements");
  test_obj_free(set);
}

/* *****************************************************************************
Test: Streaming String Callbacks
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_string_blob)(void) {
  /* Test 1: Blob string with streaming callbacks */
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "$11\r\nhello world\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(r.err == 0, "No error");
  FIO_ASSERT(r.obj != NULL, "Should return object");

  /* Should have called streaming callbacks */
  FIO_ASSERT(test_state.start_string_count == 1,
             "Should call on_start_string once");
  FIO_ASSERT(test_state.string_write_count == 1,
             "Should call on_string_write once");
  FIO_ASSERT(test_state.string_done_count == 1,
             "Should call on_string_done once");

  /* Should NOT have called on_string (streaming takes precedence) */
  FIO_ASSERT(test_state.string_count == 0,
             "Should NOT call on_string when streaming");

  /* Verify the length was passed correctly */
  FIO_ASSERT(test_state.last_start_string_len == 11,
             "on_start_string should receive length 11");
  FIO_ASSERT(test_state.last_start_string_type == FIO_RESP3_BLOB_STR,
             "on_start_string should receive BLOB_STR type");

  /* Verify the data was written correctly */
  FIO_ASSERT(test_state.total_string_write_len == 11,
             "Total written length should be 11");
  FIO_ASSERT(strcmp(test_state.string_buf, "hello world") == 0,
             "String content should be 'hello world'");

  test_obj_s *obj = (test_obj_s *)r.obj;
  FIO_ASSERT(obj->type == TEST_OBJ_STRING, "Result should be string");
  FIO_ASSERT(obj->data.str.len == 11, "String length should be 11");
  FIO_ASSERT(strcmp(obj->data.str.data, "hello world") == 0,
             "String data should match");
  test_obj_free(obj);

  /* Test 2: Empty blob string */
  test_state_reset();
  parser = (fio_resp3_parser_s){.udata = &test_state};
  data = "$0\r\n\r\n";
  r = fio_resp3_parse(&parser, &test_streaming_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Empty blob consumed");
  FIO_ASSERT(test_state.start_string_count == 1, "Empty blob calls start");
  FIO_ASSERT(test_state.string_write_count == 1,
             "Empty blob calls write (with 0 len)");
  FIO_ASSERT(test_state.string_done_count == 1, "Empty blob calls done");
  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_string_error)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "!21\r\nSYNTAX invalid syntax\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(r.err == 0, "No error");

  /* Should have called streaming callbacks */
  FIO_ASSERT(test_state.start_string_count == 1,
             "Should call on_start_string once");
  FIO_ASSERT(test_state.string_write_count == 1,
             "Should call on_string_write once");
  FIO_ASSERT(test_state.string_done_count == 1,
             "Should call on_string_done once");

  /* Verify the type was passed correctly */
  FIO_ASSERT(test_state.last_start_string_type == FIO_RESP3_BLOB_ERR,
             "on_start_string should receive BLOB_ERR type");

  /* Verify the data */
  FIO_ASSERT(strcmp(test_state.string_buf, "SYNTAX invalid syntax") == 0,
             "Error content should match");

  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_string_verbatim)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "=15\r\ntxt:Some string\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(r.err == 0, "No error");

  /* Should have called streaming callbacks */
  FIO_ASSERT(test_state.start_string_count == 1,
             "Should call on_start_string once");
  FIO_ASSERT(test_state.string_write_count == 1,
             "Should call on_string_write once");
  FIO_ASSERT(test_state.string_done_count == 1,
             "Should call on_string_done once");

  /* Verify the type was passed correctly */
  FIO_ASSERT(test_state.last_start_string_type == FIO_RESP3_VERBATIM,
             "on_start_string should receive VERBATIM type");

  /* Verify the data */
  FIO_ASSERT(strcmp(test_state.string_buf, "txt:Some string") == 0,
             "Verbatim content should match");

  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_string_chunked)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  /* Streaming blob: $?\r\n;5\r\nhello\r\n;6\r\n world\r\n;0\r\n */
  const char *data = "$?\r\n;5\r\nhello\r\n;6\r\n world\r\n;0\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(r.err == 0, "No error");
  FIO_ASSERT(r.obj != NULL, "Should return object");

  /* Should have called streaming callbacks */
  FIO_ASSERT(test_state.start_string_count == 1,
             "Should call on_start_string once");
  FIO_ASSERT(test_state.string_write_count == 2,
             "Should call on_string_write twice (2 chunks)");
  FIO_ASSERT(test_state.string_done_count == 1,
             "Should call on_string_done once");

  /* Verify streaming length was -1 */
  FIO_ASSERT(test_state.last_start_string_len == (size_t)-1,
             "Streaming string should have length -1");

  /* Verify the data was accumulated correctly */
  FIO_ASSERT(test_state.total_string_write_len == 11,
             "Total written length should be 11");
  FIO_ASSERT(strcmp(test_state.string_buf, "hello world") == 0,
             "Accumulated string should be 'hello world'");

  test_obj_s *obj = (test_obj_s *)r.obj;
  FIO_ASSERT(obj->type == TEST_OBJ_STRING, "Result should be string");
  FIO_ASSERT(strcmp(obj->data.str.data, "hello world") == 0,
             "String data should match");
  test_obj_free(obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_negative_chunk)(void) {
  /* Regression test (2026-07-31 audit, CWE-835/CWE-20): a negative streamed
   * chunk length (`$?\r\n;-1\r\n`) must be a protocol error. Unpatched code
   * rewound with err=0 on every call (infinite retry / DoS) and handed
   * SIZE_MAX to on_start_string. */
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};
  const char *data = "$?\r\n;-1\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, data, strlen(data));
  FIO_ASSERT(r.err != 0 && parser.error,
             "negative chunk length must be a protocol error");
  FIO_ASSERT(test_state.parser_error_count == 1,
             "on_error_protocol should be called once (got %d)",
             test_state.parser_error_count);
  if (parser.streaming_string_ctx)
    test_obj_free((test_obj_s *)parser.streaming_string_ctx);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_overflowing_int)(void) {
  /* Regression test (2026-07-31 audit R2, CWE-190): integers beyond the
   * uint64/int64 range must be a protocol error (E2BIG), not a silent wrap
   * that desynchronizes framing. Also: map counts that would overflow the
   * internal count*2 doubling must be rejected. */
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};
  const char *data = "$99999999999999999999\r\n"; /* > UINT64_MAX */
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));
  FIO_ASSERT(r.err != 0 && parser.error,
             "overflowing blob length must be a protocol error");
  FIO_ASSERT(test_state.parser_error_count == 1,
             "on_error_protocol should be called once");

  test_state_reset();
  fio_resp3_parser_s p2 = {.udata = &test_state};
  const char *data2 = "%4611686018427387904\r\n"; /* > INT64_MAX/2 */
  fio_resp3_result_s r2 =
      fio_resp3_parse(&p2, &test_callbacks, data2, strlen(data2));
  FIO_ASSERT(r2.err != 0 && p2.error,
             "map count exceeding INT64_MAX/2 must be a protocol error");
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_string_in_array)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  /* Array with blob strings: *2\r\n$5\r\nhello\r\n$5\r\nworld\r\n */
  const char *data = "*2\r\n$5\r\nhello\r\n$5\r\nworld\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(r.err == 0, "No error");

  /* Should have called streaming callbacks twice (once per string) */
  FIO_ASSERT(test_state.start_string_count == 2,
             "Should call on_start_string twice");
  FIO_ASSERT(test_state.string_write_count == 2,
             "Should call on_string_write twice");
  FIO_ASSERT(test_state.string_done_count == 2,
             "Should call on_string_done twice");

  FIO_ASSERT(test_state.array_count == 1, "Should have 1 array");
  FIO_ASSERT(test_state.array_push_count == 2, "Should push 2 elements");

  test_obj_s *arr = (test_obj_s *)r.obj;
  FIO_ASSERT(arr->type == TEST_OBJ_ARRAY, "Result should be array");
  FIO_ASSERT(arr->data.array.count == 2, "Array should have 2 elements");
  FIO_ASSERT(strcmp(arr->data.array.items[0]->data.str.data, "hello") == 0,
             "First element should be 'hello'");
  FIO_ASSERT(strcmp(arr->data.array.items[1]->data.str.data, "world") == 0,
             "Second element should be 'world'");
  test_obj_free(arr);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_string_fallback)(void) {
  /* When streaming callbacks are NOT provided, should fall back to on_string */
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "$11\r\nhello world\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(r.err == 0, "No error");

  /* Should NOT have called streaming callbacks */
  FIO_ASSERT(test_state.start_string_count == 0,
             "Should NOT call on_start_string");
  FIO_ASSERT(test_state.string_write_count == 0,
             "Should NOT call on_string_write");
  FIO_ASSERT(test_state.string_done_count == 0,
             "Should NOT call on_string_done");

  /* Should have called on_string */
  FIO_ASSERT(test_state.string_count == 1, "Should call on_string once");
  FIO_ASSERT(strcmp(test_state.string_buf, "hello world") == 0,
             "String content should match");

  test_obj_free((test_obj_s *)r.obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_simple_string_no_streaming)(void) {
  /* Simple strings (+) should always use on_string, not streaming callbacks */
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  const char *data = "+OK\r\n";
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, data, strlen(data));

  FIO_ASSERT(r.consumed == strlen(data), "Should consume all data");
  FIO_ASSERT(r.err == 0, "No error");

  /* Should NOT have called streaming callbacks for simple strings */
  FIO_ASSERT(test_state.start_string_count == 0,
             "Simple strings should NOT call on_start_string");
  FIO_ASSERT(test_state.string_write_count == 0,
             "Simple strings should NOT call on_string_write");
  FIO_ASSERT(test_state.string_done_count == 0,
             "Simple strings should NOT call on_string_done");

  /* Should have called on_string */
  FIO_ASSERT(test_state.string_count == 1,
             "Simple strings should call on_string");
  FIO_ASSERT(strcmp(test_state.string_buf, "OK") == 0,
             "String content should be 'OK'");

  test_obj_free((test_obj_s *)r.obj);
}

/* *****************************************************************************
Fixed-Length Blob Incremental Streaming Tests (FIO_RESP3_STREAM_THRESHOLD)
***************************************************************************** */

/** Builds a `$<len>\r\n<pattern bytes>\r\n` wire image; returns total length. */
static size_t test_make_blob_wire(uint8_t **out, size_t blob_len) {
  char hdr[32];
  int hpos = 0;
  hdr[hpos++] = '$';
  hpos += fio_ltoa(hdr + hpos, (int64_t)blob_len, 10);
  hdr[hpos++] = '\r';
  hdr[hpos++] = '\n';
  size_t total = (size_t)hpos + blob_len + 2;
  uint8_t *wire = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT(wire, "wire allocation failed");
  FIO_MEMCPY(wire, hdr, (size_t)hpos);
  for (size_t i = 0; i < blob_len; ++i)
    wire[(size_t)hpos + i] = (uint8_t)((i * 131) + 7);
  wire[(size_t)hpos + blob_len] = '\r';
  wire[(size_t)hpos + blob_len + 1] = '\n';
  *out = wire;
  return total;
}

/** Returns the byte offset of the blob data within a wire image. */
static size_t test_blob_data_offset(const uint8_t *wire) {
  size_t i = 1; /* skip '$' */
  while (wire[i] != '\r')
    ++i;
  return i + 2;
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_stream_blob_1mb_split)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  enum { BLOB_LEN = 1 << 20, CHUNK = 1 << 16 };
  uint8_t *wire = NULL;
  size_t wire_len = test_make_blob_wire(&wire, BLOB_LEN);
  size_t data_off = test_blob_data_offset(wire);
  (void)wire_len; /* used by FIO_MEM_FREE in debug builds only */

  /* Feed the header alone: streaming starts immediately, nothing written */
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, wire, data_off);
  FIO_ASSERT(!r.err, "header parse error");
  FIO_ASSERT(r.consumed == data_off,
             "header consumed mismatch: %zu",
             r.consumed);
  FIO_ASSERT(r.obj == NULL, "header parse should not complete");
  FIO_ASSERT(test_state.start_string_count == 1,
             "streaming should start at header");
  FIO_ASSERT(test_state.last_start_string_len == BLOB_LEN,
             "declared len mismatch: %zu",
             test_state.last_start_string_len);
  FIO_ASSERT(test_state.string_write_count == 0, "no writes before data");
  FIO_ASSERT(test_state.string_done_count == 0, "no done before data");

  /* Feed the blob data in 16 x 64KB chunks: buffer drains per chunk */
  size_t fed = 0;
  while (fed < BLOB_LEN) {
    r = fio_resp3_parse(&parser,
                        &test_streaming_callbacks,
                        wire + data_off + fed,
                        CHUNK);
    FIO_ASSERT(!r.err, "chunk parse error");
    FIO_ASSERT(r.consumed == CHUNK,
               "chunk should fully consume (buffer drains mid-blob): %zu",
               r.consumed);
    FIO_ASSERT(r.obj == NULL, "mid-blob parse should not complete");
    FIO_ASSERT(test_state.string_done_count == 0, "done fired too early");
    fed += CHUNK;
  }
  FIO_ASSERT(test_state.string_write_count == (BLOB_LEN / CHUNK),
             "write call count mismatch: %d",
             test_state.string_write_count);
  FIO_ASSERT(test_state.total_string_write_len == BLOB_LEN,
             "total written mismatch: %zu",
             test_state.total_string_write_len);

  /* Feed the trailing CRLF: string completes exactly once */
  r = fio_resp3_parse(&parser,
                      &test_streaming_callbacks,
                      wire + data_off + BLOB_LEN,
                      2);
  FIO_ASSERT(!r.err, "crlf parse error");
  FIO_ASSERT(r.consumed == 2, "crlf consumed mismatch: %zu", r.consumed);
  FIO_ASSERT(test_state.string_done_count == 1,
             "string_done should fire exactly once");
  FIO_ASSERT(test_state.string_count == 0,
             "on_string must not fire for streamed blobs");
  FIO_ASSERT(r.obj != NULL, "completed blob should be returned");

  /* Verify chunk ordering: assembled bytes must equal the source pattern */
  test_obj_s *obj = (test_obj_s *)r.obj;
  FIO_ASSERT(obj->type == TEST_OBJ_STRING, "wrong object type");
  FIO_ASSERT(obj->data.str.len == BLOB_LEN, "assembled length mismatch");
  FIO_ASSERT(!FIO_MEMCMP(obj->data.str.data, wire + data_off, BLOB_LEN),
             "assembled content mismatch (chunks out of order?)");
  test_obj_free(obj);
  FIO_MEM_FREE(wire, wire_len);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_stream_blob_single_call)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  /* Blob above threshold, fully present in one call: one write, one done */
  enum { BLOB_LEN = 5000 };
  uint8_t *wire = NULL;
  size_t wire_len = test_make_blob_wire(&wire, BLOB_LEN);

  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, wire, wire_len);
  FIO_ASSERT(!r.err, "parse error");
  FIO_ASSERT(r.consumed == wire_len, "consumed mismatch: %zu", r.consumed);
  FIO_ASSERT(test_state.start_string_count == 1, "start count");
  FIO_ASSERT(test_state.string_write_count == 1,
             "single-call blob should be a single write: %d",
             test_state.string_write_count);
  FIO_ASSERT(test_state.total_string_write_len == BLOB_LEN, "write len");
  FIO_ASSERT(test_state.string_done_count == 1, "done count");
  FIO_ASSERT(r.obj != NULL, "obj expected");
  test_obj_s *obj = (test_obj_s *)r.obj;
  FIO_ASSERT(obj->data.str.len == BLOB_LEN &&
                 !FIO_MEMCMP(obj->data.str.data,
                             wire + test_blob_data_offset(wire),
                             BLOB_LEN),
             "content mismatch");
  test_obj_free(obj);
  FIO_MEM_FREE(wire, wire_len);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_stream_blob_small_split)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  /* Small blob (below threshold) split mid-blob: parser waits for
   * contiguity, then delivers the whole blob in a single write. */
  const char *part1 = "$11\r\nhello";
  const char *full = "$11\r\nhello world\r\n";

  fio_resp3_result_s r = fio_resp3_parse(&parser,
                                         &test_streaming_callbacks,
                                         part1,
                                         FIO_STRLEN(part1));
  FIO_ASSERT(!r.err, "part1 parse error");
  FIO_ASSERT(r.consumed == 0,
             "small blob split: nothing consumed while waiting: %zu",
             r.consumed);
  FIO_ASSERT(r.obj == NULL, "part1 should not complete");
  FIO_ASSERT(test_state.start_string_count == 0,
             "small blob must not start streaming early");

  /* Parser consumed nothing: consumer re-feeds buffered data + new bytes */
  r = fio_resp3_parse(&parser,
                      &test_streaming_callbacks,
                      full,
                      FIO_STRLEN(full));
  FIO_ASSERT(!r.err, "part2 parse error");
  FIO_ASSERT(r.consumed == FIO_STRLEN(full),
             "part2 consumed mismatch: %zu",
             r.consumed);
  FIO_ASSERT(r.obj != NULL, "small blob should complete");
  FIO_ASSERT(test_state.start_string_count == 1, "start count");
  FIO_ASSERT(test_state.last_start_string_len == 11, "declared len");
  FIO_ASSERT(test_state.string_write_count == 1,
             "small blob delivered whole in one write: %d",
             test_state.string_write_count);
  FIO_ASSERT(test_state.total_string_write_len == 11, "write len");
  FIO_ASSERT(test_state.string_done_count == 1, "done count");
  test_obj_s *obj = (test_obj_s *)r.obj;
  FIO_ASSERT(obj->data.str.len == 11 &&
                 !FIO_MEMCMP(obj->data.str.data, "hello world", 11),
             "content mismatch");
  test_obj_free(obj);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_stream_blob_no_streaming_table)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  /* Tables without streaming callbacks: unchanged wait-for-contiguity
   * behavior even for blobs above the threshold. */
  enum { BLOB_LEN = 5000 };
  uint8_t *wire = NULL;
  size_t wire_len = test_make_blob_wire(&wire, BLOB_LEN);
  size_t data_off = test_blob_data_offset(wire);
  size_t half = data_off + (BLOB_LEN / 2);

  fio_resp3_result_s r = fio_resp3_parse(&parser, &test_callbacks, wire, half);
  FIO_ASSERT(!r.err, "part1 parse error");
  FIO_ASSERT(r.consumed == 0,
             "non-streaming table must wait for contiguity: %zu",
             r.consumed);
  FIO_ASSERT(r.obj == NULL, "part1 should not complete");

  r = fio_resp3_parse(&parser, &test_callbacks, wire, wire_len);
  FIO_ASSERT(!r.err, "part2 parse error");
  FIO_ASSERT(r.consumed == wire_len, "consumed mismatch: %zu", r.consumed);
  FIO_ASSERT(test_state.start_string_count == 0,
             "streaming callbacks must not fire");
  FIO_ASSERT(test_state.string_write_count == 0, "write must not fire");
  FIO_ASSERT(test_state.string_done_count == 0, "done must not fire");
  FIO_ASSERT(test_state.string_count == 1,
             "on_string should deliver the whole blob");
  FIO_ASSERT(r.obj != NULL, "obj expected");
  test_obj_s *obj = (test_obj_s *)r.obj;
  FIO_ASSERT(obj->data.str.len == BLOB_LEN &&
                 !FIO_MEMCMP(obj->data.str.data, wire + data_off, BLOB_LEN),
             "content mismatch");
  test_obj_free(obj);
  FIO_MEM_FREE(wire, wire_len);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_stream_blob_crlf_split)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  /* The trailing CRLF may arrive split from the data - and split itself. */
  enum { BLOB_LEN = 5000 };
  uint8_t *wire = NULL;
  size_t wire_len = test_make_blob_wire(&wire, BLOB_LEN);
  size_t data_off = test_blob_data_offset(wire);
  (void)wire_len; /* used by FIO_MEM_FREE in debug builds only */

  /* header + first half of data */
  fio_resp3_result_s r = fio_resp3_parse(&parser,
                                         &test_streaming_callbacks,
                                         wire,
                                         data_off + 2500);
  FIO_ASSERT(!r.err && r.consumed == data_off + 2500, "feed 1");
  FIO_ASSERT(test_state.total_string_write_len == 2500, "feed 1 write len");

  /* second half of data, exactly to blob end (no CRLF yet) */
  r = fio_resp3_parse(&parser,
                      &test_streaming_callbacks,
                      wire + data_off + 2500,
                      BLOB_LEN - 2500);
  FIO_ASSERT(!r.err && r.consumed == BLOB_LEN - 2500, "feed 2");
  FIO_ASSERT(r.obj == NULL, "no completion before CRLF");
  FIO_ASSERT(test_state.string_done_count == 0,
             "done must wait for the trailing CRLF");

  /* CR alone */
  r = fio_resp3_parse(&parser,
                      &test_streaming_callbacks,
                      wire + data_off + BLOB_LEN,
                      1);
  FIO_ASSERT(!r.err && r.consumed == 1, "feed 3 (CR)");
  FIO_ASSERT(r.obj == NULL, "no completion on CR alone");
  FIO_ASSERT(test_state.string_done_count == 0, "done still pending");

  /* LF completes */
  r = fio_resp3_parse(&parser,
                      &test_streaming_callbacks,
                      wire + data_off + BLOB_LEN + 1,
                      1);
  FIO_ASSERT(!r.err && r.consumed == 1, "feed 4 (LF)");
  FIO_ASSERT(test_state.string_done_count == 1, "done fired");
  FIO_ASSERT(r.obj != NULL, "obj expected after LF");
  test_obj_free((test_obj_s *)r.obj);
  FIO_MEM_FREE(wire, wire_len);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_stream_blob_in_array)(void) {
  test_state_reset();
  fio_resp3_parser_s parser = {.udata = &test_state};

  /* Streamed blob nested in an array, split across reads. */
  enum { BLOB_LEN = 5000 };
  uint8_t *blob = NULL;
  size_t blob_wire_len = test_make_blob_wire(&blob, BLOB_LEN);
  size_t data_off = test_blob_data_offset(blob);
  const char *prefix = "*2\r\n";
  const char *suffix = ":42\r\n";
  size_t total = FIO_STRLEN(prefix) + blob_wire_len + FIO_STRLEN(suffix);
  uint8_t *wire = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT(wire, "wire allocation failed");
  FIO_MEMCPY(wire, prefix, FIO_STRLEN(prefix));
  FIO_MEMCPY(wire + FIO_STRLEN(prefix), blob, blob_wire_len);
  FIO_MEMCPY(wire + FIO_STRLEN(prefix) + blob_wire_len,
             suffix,
             FIO_STRLEN(suffix));
  FIO_MEM_FREE(blob, blob_wire_len);

  /* Feed 1: array header + blob header + 1000 data bytes */
  size_t feed1 = FIO_STRLEN(prefix) + data_off + 1000;
  fio_resp3_result_s r =
      fio_resp3_parse(&parser, &test_streaming_callbacks, wire, feed1);
  FIO_ASSERT(!r.err && r.consumed == feed1, "feed 1");
  FIO_ASSERT(r.obj == NULL, "array incomplete");

  /* Feed 2: remainder (rest of blob + CRLF + :42\r\n) */
  r = fio_resp3_parse(&parser,
                      &test_streaming_callbacks,
                      wire + feed1,
                      total - feed1);
  FIO_ASSERT(!r.err && r.consumed == total - feed1, "feed 2");
  FIO_ASSERT(r.obj != NULL, "array should complete");
  FIO_ASSERT(test_state.string_done_count == 1, "blob done once");
  test_obj_s *obj = (test_obj_s *)r.obj;
  FIO_ASSERT(obj->type == TEST_OBJ_ARRAY && obj->data.array.count == 2,
             "array shape mismatch");
  FIO_ASSERT(obj->data.array.items[0]->type == TEST_OBJ_STRING &&
                 obj->data.array.items[0]->data.str.len == BLOB_LEN,
             "array blob element mismatch");
  FIO_ASSERT(!FIO_MEMCMP(obj->data.array.items[0]->data.str.data,
                         wire + FIO_STRLEN(prefix) + data_off,
                         BLOB_LEN),
             "array blob content mismatch");
  FIO_ASSERT(obj->data.array.items[1]->type == TEST_OBJ_NUMBER &&
                 obj->data.array.items[1]->data.num_val == 42,
             "array number element mismatch");
  test_obj_free(obj);
  FIO_MEM_FREE(wire, total);
}

/* Security regressions: callbacks create owned contexts. An error leaves
 * partial aggregate/string contexts with the caller, not the parser. */
static void test_security_dispose(fio_resp3_parser_s *p, fio_resp3_result_s r) {
  for (uint32_t i = p->depth; i > 0; --i) {
    fio_resp3_frame_s *f = &p->stack[i - 1];
    if (f->key)
      test_obj_free((test_obj_s *)f->key);
    test_obj_free((test_obj_s *)f->ctx);
  }
  if (p->streaming_string_ctx)
    test_obj_free((test_obj_s *)p->streaming_string_ctx);
  if (r.obj)
    test_obj_free((test_obj_s *)r.obj);
  p->depth = 0;
  p->streaming_string_ctx = NULL;
}

/* Rejected input must be a protocol error, never a successful prefix that
 * leaves a following top-level value to be misrouted. */
static void test_security_reject(const char *wire, int streaming) {
  test_state_reset();
  fio_resp3_parser_s p = {.udata = &test_state};
  fio_resp3_result_s r =
      fio_resp3_parse(&p,
                      streaming ? &test_streaming_callbacks : &test_callbacks,
                      wire,
                      FIO_STRLEN(wire));
  int rejected = !!r.err && !!p.error;
  test_security_dispose(&p, r);
  FIO_ASSERT(rejected, "malformed RESP3 accepted: %s", wire);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_numeric)(void) {
  /* Each of ten independently parsed integer fields: empty, sign-only,
   * and suffix junk. Values/counts are deliberately small for bounded tests. */
  static const char *const bad[] = {
      ":\r\n",       ":+\r\n",       ":12x\r\n",
      "$\r\n\r\n",   "$-\r\n",       "$1x\r\na\r\n",
      "!\r\n\r\n",   "!+\r\n",       "!1x\r\na\r\n",
      "=\r\n\r\n",   "=-\r\n",       "=4x\r\ntxt:a\r\n",
      "*\r\n",       "*+\r\n",       "*1x\r\n:1\r\n",
      "%\r\n",       "%-\r\n",       "%1x\r\n+a\r\n:1\r\n",
      "~\r\n",       "~+\r\n",       "~1x\r\n:1\r\n",
      ">\r\n",       ">-\r\n",       ">1x\r\n:1\r\n",
      "|\r\n:1\r\n", "|+\r\n:1\r\n", "|1x\r\n+a\r\n:1\r\n:2\r\n",
      "$?\r\n;\r\n", "$?\r\n;+\r\n", "$?\r\n;1x\r\na\r\n;0\r\n",
  };
  for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); ++i)
    test_security_reject(bad[i], 1);
  test_security_reject("$?x\r\n;0\r\n", 1);
  test_security_reject("*?x\r\n.\r\n", 0);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_grammar)(void) {
  static const char *const malformed[] = {
      "+A\n",
      "-A\n",
      ":1\n",
      "_x\r\n",
      "_\n",
      "#x\r\n",
      "#true\r\n",
      "#t\n",
      ",1.2x\r\n",
      ",infinity\r\n",
      ",+infx\r\n",
      ",nanx\r\n",
      ",.1\r\n",
      ",1.\r\n",
      ",1e\r\n",
      ",1e+\r\n",
      ",e1\r\n",
      ",+\r\n",
      ",1e2junk\r\n",
      "(\r\n",
      "(1x\r\n",
      "(+\r\n",
      "$1\r\nAx\n",
      "$1\r\nA:7\r\n",
      "!1\r\nAx\n",
      "=4\r\nab:c\r\n",
      "=3\r\nabc\r\n",
      "=4\r\nxy:c\r\n",
      "*0\n",
      ".\r\n",
      "*1\r\n.\r\n",
      "%?\r\n+k\r\n.\r\n",
      "$?\r\n;1\r\nAx\n;0\r\n",
  };
  for (size_t i = 0; i < sizeof(malformed) / sizeof(*malformed); ++i)
    test_security_reject(malformed[i], 1);
  /* Reject chunk-state intrusion rather than producing an intermediate reply.
   */
  test_security_reject("$?\r\n:7\r\n;0\r\n", 1);
  test_security_reject("$?\r\n$0\r\n\r\n;0\r\n", 1);
  test_security_reject("$?\r\n$?\r\n;0\r\n", 1);
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_valid_specials)(void) {
  static const char *const nan_cases[] = {",nan\r\n", ",NaN\r\n", ",-NAN\r\n"};
  for (size_t i = 0; i < sizeof(nan_cases) / sizeof(*nan_cases); ++i) {
    test_state_reset();
    fio_resp3_parser_s p = {.udata = &test_state};
    fio_resp3_result_s r = fio_resp3_parse(&p,
                                           &test_callbacks,
                                           nan_cases[i],
                                           FIO_STRLEN(nan_cases[i]));
    FIO_ASSERT(!r.err && r.consumed == FIO_STRLEN(nan_cases[i]) && r.obj &&
                   isnan(test_state.last_double),
               "historical exact NaN must remain accepted (%zu)",
               i);
    test_security_dispose(&p, r);
  }
  /* RESP3 permits future three-character printable format identifiers. */
  const char *verbatim = "=5\r\nabc:X\r\n";
  for (int streaming = 0; streaming <= 1; ++streaming) {
    test_state_reset();
    fio_resp3_parser_s p = {.udata = &test_state};
    fio_resp3_result_s r =
        fio_resp3_parse(&p,
                        streaming ? &test_streaming_callbacks : &test_callbacks,
                        verbatim,
                        FIO_STRLEN(verbatim));
    test_obj_s *obj = (test_obj_s *)r.obj;
    FIO_ASSERT(!r.err && r.consumed == FIO_STRLEN(verbatim) && obj &&
                   obj->type == TEST_OBJ_STRING &&
                   obj->data.str.resp_type == FIO_RESP3_VERBATIM &&
                   obj->data.str.len == 5 &&
                   !FIO_MEMCMP(obj->data.str.data, "abc:X", 5),
               "future verbatim format remains valid (%d)",
               streaming);
    test_security_dispose(&p, r);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_incremental)(void) {
  /* A split CRLF is valid and each consumed byte must make progress. */
  const char *parts[] = {"$?\r\n;3\r\nab", "c", "\r", "\n", ";0\r\n"};
  test_state_reset();
  fio_resp3_parser_s p = {.udata = &test_state};
  fio_resp3_result_s r = {0};
  for (size_t i = 0; i < sizeof(parts) / sizeof(*parts); ++i) {
    r = fio_resp3_parse(&p,
                        &test_streaming_callbacks,
                        parts[i],
                        FIO_STRLEN(parts[i]));
    FIO_ASSERT(!r.err && r.consumed == FIO_STRLEN(parts[i]),
               "valid split chunk CRLF progress (%zu)",
               i);
    if (i + 1 != sizeof(parts) / sizeof(*parts))
      FIO_ASSERT(!r.obj, "chunk must not finish before ;0 CRLF");
  }
  FIO_ASSERT(r.obj && ((test_obj_s *)r.obj)->data.str.len == 3 &&
                 !FIO_MEMCMP(((test_obj_s *)r.obj)->data.str.data, "abc", 3),
             "split chunk result");
  test_security_dispose(&p, r);

  /* A wrong LF after a split CR must fail, not finish the blob. */
  test_state_reset();
  p = (fio_resp3_parser_s){.udata = &test_state};
  const char *partial = "$?\r\n;1\r\na\r";
  r = fio_resp3_parse(&p,
                      &test_streaming_callbacks,
                      partial,
                      FIO_STRLEN(partial));
  FIO_ASSERT(!r.err, "partial chunk CR");
  r = fio_resp3_parse(&p, &test_streaming_callbacks, "x", 1);
  int rejected = !!r.err && !!p.error;
  test_security_dispose(&p, r);
  FIO_ASSERT(rejected, "wrong split chunk LF accepted");
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_large_chunk)(void) {
  /* Valid chunk > Redis's bounded 64-KiB read buffer. Feed at most 8 KiB
   * per call, so a parser requiring a contiguous chunk stalls immediately. */
  enum { CHUNK = 65537, STEP = 8192 };
  const char header[] = "$?\r\n;65537\r\n";
  char data[STEP];
  FIO_MEMSET(data, 'q', sizeof(data));
  test_state_reset();
  fio_resp3_parser_s p = {.udata = &test_state};
  fio_resp3_result_s r = fio_resp3_parse(&p,
                                         &test_streaming_callbacks,
                                         header,
                                         sizeof(header) - 1);
  FIO_ASSERT(!r.err && r.consumed == sizeof(header) - 1,
             "chunk header progress");
  size_t sent = 0;
  while (sent < CHUNK) {
    size_t n = CHUNK - sent < STEP ? CHUNK - sent : STEP;
    r = fio_resp3_parse(&p, &test_streaming_callbacks, data, n);
    FIO_ASSERT(!r.err && r.consumed == n && !r.obj,
               "large chunk must advance in bounded read buffer at %zu",
               sent);
    sent += n;
  }
  r = fio_resp3_parse(&p, &test_streaming_callbacks, "\r", 1);
  FIO_ASSERT(!r.err && r.consumed == 1 && !r.obj, "split chunk CR");
  r = fio_resp3_parse(&p, &test_streaming_callbacks, "\n;0\r\n", 5);
  FIO_ASSERT(!r.err && r.consumed == 5 && r.obj,
             "split chunk LF and terminator complete");
  FIO_ASSERT(((test_obj_s *)r.obj)->data.str.len == CHUNK,
             "large chunk length");
  test_security_dispose(&p, r);
}

/* F1: an attribute annotates the next element; it is not an element of
 * any fixed parent. Complete metadata before separately feeding its value. */
FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_nested_attributes)(void) {
  static const struct {
    const char *prefix;
    const char *suffix;
    test_obj_type_e type;
    size_t count;
  } cases[] = {
      {"*1\r\n|1\r\n+k\r\n+v\r\n", ":7\r\n", TEST_OBJ_ARRAY, 1},
      {"~1\r\n|1\r\n+k\r\n+v\r\n", ":7\r\n", TEST_OBJ_SET, 1},
      {">1\r\n|1\r\n+k\r\n+v\r\n", ":7\r\n", TEST_OBJ_PUSH, 1},
      {"%1\r\n|1\r\n+k\r\n+v\r\n", "+key\r\n:7\r\n", TEST_OBJ_MAP, 1},
      {"%1\r\n+key\r\n|1\r\n+k\r\n+v\r\n", ":7\r\n", TEST_OBJ_MAP, 1},
      {"*1\r\n|1\r\n+k\r\n+v\r\n|1\r\n+x\r\n+y\r\n",
       ":7\r\n",
       TEST_OBJ_ARRAY,
       1},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    test_state_reset();
    fio_resp3_parser_s p = {.udata = &test_state};
    fio_resp3_result_s r = fio_resp3_parse(&p,
                                           &test_callbacks,
                                           cases[i].prefix,
                                           FIO_STRLEN(cases[i].prefix));
    FIO_ASSERT(!r.err && !r.obj && r.consumed == FIO_STRLEN(cases[i].prefix),
               "attribute metadata must not complete parent (%zu)",
               i);
    r = fio_resp3_parse(&p,
                        &test_callbacks,
                        cases[i].suffix,
                        FIO_STRLEN(cases[i].suffix));
    test_obj_s *obj = (test_obj_s *)r.obj;
    FIO_ASSERT(!r.err && obj && r.consumed == FIO_STRLEN(cases[i].suffix),
               "attribute's value must complete parent (%zu)",
               i);
    FIO_ASSERT(obj->type == cases[i].type, "parent type (%zu)", i);
    if (obj->type == TEST_OBJ_MAP) {
      FIO_ASSERT(obj->data.map.count == cases[i].count &&
                     obj->data.map.values[0]->type == TEST_OBJ_NUMBER &&
                     obj->data.map.values[0]->data.num_val == 7,
                 "map value after attribute (%zu)",
                 i);
    } else {
      FIO_ASSERT(obj->data.array.count == cases[i].count &&
                     obj->data.array.items[0]->type == TEST_OBJ_NUMBER &&
                     obj->data.array.items[0]->data.num_val == 7,
                 "sequence value after attribute (%zu)",
                 i);
    }
    FIO_ASSERT(test_state.attr_done_count == (i == 5 ? 2 : 1),
               "metadata completed once (%zu)",
               i);
    test_security_dispose(&p, r);
  }
}

/* F7: only count callback invocations; the sentinel belongs to no heap.
 * This keeps huge count and depth tests bounded even against pre-fix code. */
static void *test_security_count_container(void *udata,
                                           void *parent,
                                           int64_t count) {
  (void)parent;
  (void)count;
  ++*(unsigned *)udata;
  return (void *)(uintptr_t)1;
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_reject_before_callback)(void) {
  const fio_resp3_callbacks_s callbacks = {
      .on_array = test_security_count_container,
      .on_map = test_security_count_container,
      .on_set = test_security_count_container,
      .on_push = test_security_count_container,
      .on_attr = test_security_count_container,
  };
  static const char *const excessive[] = {"%4611686018427387904\r\n",
                                          "|4611686018427387904\r\n"};
  for (size_t i = 0; i < sizeof(excessive) / sizeof(*excessive); ++i) {
    unsigned callbacks_called = 0;
    fio_resp3_parser_s p = {.udata = &callbacks_called};
    fio_resp3_result_s r =
        fio_resp3_parse(&p, &callbacks, excessive[i], FIO_STRLEN(excessive[i]));
    FIO_ASSERT(r.err && p.error && !r.obj && !callbacks_called,
               "reject pair overflow before callback (%zu): %u",
               i,
               callbacks_called);
  }

  /* Fill exactly the allowed depth with small registered frames. The next
   * start must error without producing an unregistered callback context. */
  static const char *const starts[] = {"*1\r\n",
                                       "%1\r\n",
                                       "~1\r\n",
                                       ">1\r\n",
                                       "|1\r\n"};
  for (size_t i = 0; i < sizeof(starts) / sizeof(*starts); ++i) {
    unsigned callbacks_called = 0;
    fio_resp3_parser_s p = {.udata = &callbacks_called};
    for (size_t n = 0; n < FIO_RESP3_MAX_NESTING; ++n) {
      fio_resp3_result_s r =
          fio_resp3_parse(&p, &callbacks, "*1\r\n", FIO_STRLEN("*1\r\n"));
      FIO_ASSERT(!r.err && !r.obj && r.consumed == FIO_STRLEN("*1\r\n"),
                 "fill nesting depth (%zu, %zu)",
                 i,
                 n);
    }
    FIO_ASSERT(p.depth == FIO_RESP3_MAX_NESTING &&
                   callbacks_called == FIO_RESP3_MAX_NESTING,
               "depth setup (%zu)",
               i);
    fio_resp3_result_s r =
        fio_resp3_parse(&p, &callbacks, starts[i], FIO_STRLEN(starts[i]));
    FIO_ASSERT(r.err && p.error && !r.obj &&
                   callbacks_called == FIO_RESP3_MAX_NESTING,
               "reject depth before callback (%zu): %u",
               i,
               callbacks_called);
    /* All frames hold the non-owning sentinel. Parser and caller cleanup
     * intentionally have nothing to free here. */
  }
}

/* Refeed unconsumed bytes after every split point. Aggregate callbacks
 * must produce the same semantic result as a one-shot parse. */
FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_split_matrix)(void) {
  static const struct {
    const char *wire;
    test_obj_type_e type;
    size_t count;
    int attrs;
  } cases[] = {
      {"*1\r\n|1\r\n+k\r\n+v\r\n:7\r\n", TEST_OBJ_ARRAY, 1, 1},
      {"%1\r\n+k\r\n|1\r\n+x\r\n+y\r\n:7\r\n", TEST_OBJ_MAP, 1, 1},
      {"~1\r\n|1\r\n+k\r\n+v\r\n:7\r\n", TEST_OBJ_SET, 1, 1},
      {">1\r\n|1\r\n+k\r\n+v\r\n:7\r\n", TEST_OBJ_PUSH, 1, 1},
      {"*1\r\n|1\r\n+k\r\n|1\r\n+x\r\n+y\r\n+v\r\n:7\r\n",
       TEST_OBJ_ARRAY,
       1,
       2},
      {"*?\r\n|1\r\n+k\r\n+v\r\n:7\r\n.\r\n", TEST_OBJ_ARRAY, 1, 1},
      {"%?\r\n+k\r\n|1\r\n+x\r\n+y\r\n:7\r\n.\r\n", TEST_OBJ_MAP, 1, 1},
      {"~?\r\n|1\r\n+k\r\n+v\r\n:7\r\n.\r\n", TEST_OBJ_SET, 1, 1},
      {"*2\r\n$3\r\nabc\r\n=5\r\nabc:X\r\n", TEST_OBJ_ARRAY, 2, 0},
      {"*1\r\n$?\r\n;3\r\nabc\r\n;0\r\n", TEST_OBJ_ARRAY, 1, 0},
  };
  for (int streaming = 0; streaming <= 1; ++streaming) {
    const fio_resp3_callbacks_s *cb =
        streaming ? &test_streaming_callbacks : &test_callbacks;
    for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
      if (!streaming && i == 9)
        continue; /* Unknown-length blob requires streaming callbacks. */
      const char *wire = cases[i].wire;
      const size_t len = FIO_STRLEN(wire);
      for (size_t split = 0; split <= len; ++split) {
        test_state_reset();
        fio_resp3_parser_s p = {.udata = &test_state};
        fio_resp3_result_s first = fio_resp3_parse(&p, cb, wire, split);
        FIO_ASSERT(!first.err && first.consumed <= split &&
                       (!!first.obj == (split == len)),
                   "split prefix (%d,%zu,%zu)",
                   streaming,
                   i,
                   split);
        if (split == len) {
          FIO_ASSERT(first.consumed == len,
                     "complete first feed (%d,%zu)",
                     streaming,
                     i);
          test_security_dispose(&p, first);
          continue;
        }
        size_t available = len - first.consumed;
        fio_resp3_result_s r =
            fio_resp3_parse(&p, cb, wire + first.consumed, available);
        test_obj_s *obj = (test_obj_s *)r.obj;
        FIO_ASSERT(!r.err && r.consumed == available && obj &&
                       obj->type == cases[i].type,
                   "split completion (%d,%zu,%zu)",
                   streaming,
                   i,
                   split);
        if (obj->type == TEST_OBJ_MAP)
          FIO_ASSERT(obj->data.map.count == cases[i].count &&
                         obj->data.map.values[0]->type == TEST_OBJ_NUMBER &&
                         obj->data.map.values[0]->data.num_val == 7,
                     "split map semantic (%d,%zu,%zu)",
                     streaming,
                     i,
                     split);
        else {
          FIO_ASSERT(obj->data.array.count == cases[i].count,
                     "split sequence count (%d,%zu,%zu)",
                     streaming,
                     i,
                     split);
          if (i != 8 && i != 9)
            FIO_ASSERT(obj->data.array.items[0]->type == TEST_OBJ_NUMBER &&
                           obj->data.array.items[0]->data.num_val == 7,
                       "split sequence semantic (%d,%zu,%zu)",
                       streaming,
                       i,
                       split);
          else
            FIO_ASSERT(obj->data.array.items[0]->type == TEST_OBJ_STRING &&
                           obj->data.array.items[0]->data.str.len == 3 &&
                           !FIO_MEMCMP(obj->data.array.items[0]->data.str.data,
                                       "abc",
                                       3),
                       "split blob semantic (%d,%zu,%zu)",
                       streaming,
                       i,
                       split);
        }
        if (i == 8)
          FIO_ASSERT(obj->data.array.items[1]->type == TEST_OBJ_STRING &&
                         obj->data.array.items[1]->data.str.resp_type ==
                             FIO_RESP3_VERBATIM &&
                         obj->data.array.items[1]->data.str.len == 5 &&
                         !FIO_MEMCMP(obj->data.array.items[1]->data.str.data,
                                     "abc:X",
                                     5),
                     "split future verbatim (%d,%zu)",
                     streaming,
                     split);
        FIO_ASSERT(test_state.attr_done_count == cases[i].attrs,
                   "split attribute count (%d,%zu,%zu)",
                   streaming,
                   i,
                   split);
        test_security_dispose(&p, r);
      }
    }
  }
}

/* A small deterministic mutation smoke test uses only non-owning sentinel
 * callbacks. It makes no validity assumptions about mutated input; it only
 * checks framing bounds and bounded termination on both feeding styles. */
static void *test_security_smoke_start(void *udata, size_t len, uint8_t type) {
  (void)udata;
  (void)len;
  (void)type;
  return (void *)(uintptr_t)1;
}
static int test_security_smoke_write(void *udata,
                                     void *ctx,
                                     const void *bytes,
                                     size_t len) {
  (void)udata;
  (void)ctx;
  (void)bytes;
  (void)len;
  return 0;
}
static void *test_security_smoke_done(void *udata, void *ctx, uint8_t type) {
  (void)udata;
  (void)type;
  return ctx;
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_mutation_smoke)(void) {
  static const fio_resp3_callbacks_s callbacks = {
      .on_start_string = test_security_smoke_start,
      .on_string_write = test_security_smoke_write,
      .on_string_done = test_security_smoke_done,
  };
  static const char *const corpus[] = {
      ":7\r\n",
      "$3\r\nabc\r\n",
      "$?\r\n;3\r\nabc\r\n;0\r\n",
      "*1\r\n|1\r\n+k\r\n+v\r\n:7\r\n",
      "%?\r\n+k\r\n:7\r\n.\r\n",
      "=5\r\nabc:X\r\n",
  };
  static const char alphabet[] = "\r\n0123456789+-?;.$!*%|:XYZ";
  uint32_t seed = UINT32_C(0x5eed1234);
  for (size_t trial = 0; trial < 192; ++trial) {
    char wire[96];
    size_t source = trial % (sizeof(corpus) / sizeof(*corpus));
    size_t n = FIO_STRLEN(corpus[source]);
    FIO_ASSERT(n < sizeof(wire), "mutation bounded by wire buffer");
    FIO_MEMCPY(wire, corpus[source], n);
    seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
    if (trial >= sizeof(corpus) / sizeof(*corpus)) {
      size_t at = (seed >> 8) % n;
      wire[at] = alphabet[(seed >> 19) % (sizeof(alphabet) - 1)];
    }
    for (int incremental = 0; incremental <= 1; ++incremental) {
      fio_resp3_parser_s p = {0};
      size_t consumed = 0;
      size_t supplied = 0;
      unsigned calls = 0;
      /* A bounded caller buffer: append 1-7 bytes at a time, retain
       * unconsumed bytes, and stop at first top-level object or error. */
      while (calls++ < 128) {
        if (supplied < n) {
          size_t step = incremental ? 1 + ((seed >> (calls & 15)) % 7) : n;
          if (step > n - supplied)
            step = n - supplied;
          supplied += step;
        }
        FIO_ASSERT(consumed <= supplied, "caller bookkeeping");
        size_t available = supplied - consumed;
        fio_resp3_result_s r =
            fio_resp3_parse(&p, &callbacks, wire + consumed, available);
        FIO_ASSERT(r.consumed <= available,
                   "parse consumed beyond buffer (%zu,%d,%u)",
                   trial,
                   incremental,
                   calls);
        consumed += r.consumed;
        if (r.err || r.obj || supplied == n)
          break;
      }
      FIO_ASSERT(calls <= 128,
                 "bounded smoke parse (%zu,%d)",
                 trial,
                 incremental);
      /* p owns no heap objects: a fresh parser starts each trial. */
    }
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_security_size_bounds)(void) {
  /* Header only: 32-bit size_t must reject narrowing before invoking a
   * callback or calculating an out-of-object pointer. On 64-bit the header
   * can remain incomplete; no huge allocation or buffer is supplied. */
#if SIZE_MAX <= UINT32_MAX
  static const char *const huge[] = {"$4294967297\r\nA\r\n",
                                     "!4294967297\r\nA\r\n",
                                     "=4294967297\r\nA\r\n",
                                     "$?\r\n;4294967297\r\nA\r\n"};
  for (size_t i = 0; i < sizeof(huge) / sizeof(*huge); ++i)
    test_security_reject(huge[i], 1);
#else
  static const char *const huge[] = {"$4294967297\r\n",
                                     "!4294967297\r\n",
                                     "=4294967297\r\n"};
  for (size_t i = 0; i < sizeof(huge) / sizeof(*huge); ++i) {
    test_state_reset();
    fio_resp3_parser_s p = {.udata = &test_state};
    fio_resp3_result_s r =
        fio_resp3_parse(&p, &test_callbacks, huge[i], FIO_STRLEN(huge[i]));
    FIO_ASSERT(!r.err && !r.obj && !r.consumed && !p.depth,
               "64-bit large length remains incomplete");
  }
#endif
}

int main(void) {
  FIO_NAME_TEST(stl, resp3_simple_string)();
  FIO_NAME_TEST(stl, resp3_simple_error)();
  FIO_NAME_TEST(stl, resp3_number)();
  FIO_NAME_TEST(stl, resp3_null)();
  FIO_NAME_TEST(stl, resp3_double)();
  FIO_NAME_TEST(stl, resp3_bool)();
  FIO_NAME_TEST(stl, resp3_bignum)();
  FIO_NAME_TEST(stl, resp3_blob_string)();
  FIO_NAME_TEST(stl, resp3_blob_error)();
  FIO_NAME_TEST(stl, resp3_verbatim)();
  FIO_NAME_TEST(stl, resp3_array)();
  FIO_NAME_TEST(stl, resp3_map)();
  FIO_NAME_TEST(stl, resp3_set)();
  FIO_NAME_TEST(stl, resp3_push)();
  FIO_NAME_TEST(stl, resp3_attr)();
  FIO_NAME_TEST(stl, resp3_streaming_array)();
  FIO_NAME_TEST(stl, resp3_streaming_map)();
  FIO_NAME_TEST(stl, resp3_partial)();
  FIO_NAME_TEST(stl, resp3_mixed_array)();
  FIO_NAME_TEST(stl, resp3_deep_nesting)();
  FIO_NAME_TEST(stl, resp3_error_handling)();
  FIO_NAME_TEST(stl, resp3_udata)();
  FIO_NAME_TEST(stl, resp3_context_stack)();
  FIO_NAME_TEST(stl, resp3_set_as_map_fallback)();
  FIO_NAME_TEST(stl, resp3_set_with_callbacks)();
  FIO_NAME_TEST(stl, resp3_streaming_string_blob)();
  FIO_NAME_TEST(stl, resp3_streaming_string_error)();
  FIO_NAME_TEST(stl, resp3_streaming_string_verbatim)();
  FIO_NAME_TEST(stl, resp3_streaming_string_chunked)();
  FIO_NAME_TEST(stl, resp3_streaming_negative_chunk)();
  FIO_NAME_TEST(stl, resp3_overflowing_int)();
  FIO_NAME_TEST(stl, resp3_streaming_string_in_array)();
  FIO_NAME_TEST(stl, resp3_streaming_string_fallback)();
  FIO_NAME_TEST(stl, resp3_simple_string_no_streaming)();
  FIO_NAME_TEST(stl, resp3_stream_blob_1mb_split)();
  FIO_NAME_TEST(stl, resp3_stream_blob_single_call)();
  FIO_NAME_TEST(stl, resp3_stream_blob_small_split)();
  FIO_NAME_TEST(stl, resp3_stream_blob_no_streaming_table)();
  FIO_NAME_TEST(stl, resp3_stream_blob_crlf_split)();
  FIO_NAME_TEST(stl, resp3_stream_blob_in_array)();
  FIO_NAME_TEST(stl, resp3_security_numeric)();
  FIO_NAME_TEST(stl, resp3_security_grammar)();
  FIO_NAME_TEST(stl, resp3_security_valid_specials)();
  FIO_NAME_TEST(stl, resp3_security_incremental)();
  FIO_NAME_TEST(stl, resp3_security_large_chunk)();
  FIO_NAME_TEST(stl, resp3_security_size_bounds)();
  FIO_NAME_TEST(stl, resp3_security_nested_attributes)();
  FIO_NAME_TEST(stl, resp3_security_reject_before_callback)();
  FIO_NAME_TEST(stl, resp3_security_split_matrix)();
  FIO_NAME_TEST(stl, resp3_security_mutation_smoke)();
  return 0;
}
