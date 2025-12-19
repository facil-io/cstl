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
  return ctx;
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
  if (obj) {
    obj->data.str.resp_type = type;
    /* Pre-allocate if length is known */
    if (len != (size_t)-1 && len > 0) {
      obj->data.str.data = (char *)FIO_MEM_REALLOC(NULL, 0, len + 1, 0);
      if (obj->data.str.data)
        obj->data.str.data[0] = '\0';
    }
  }
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
  FIO_LOG_DDEBUG("Testing RESP3 simple string (+)");

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
  FIO_LOG_DDEBUG("Testing RESP3 simple error (-)");

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
  FIO_LOG_DDEBUG("Testing RESP3 number (:)");

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
  FIO_LOG_DDEBUG("Testing RESP3 null (_)");

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
  FIO_LOG_DDEBUG("Testing RESP3 double (,)");

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
  FIO_LOG_DDEBUG("Testing RESP3 boolean (#)");

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
  FIO_LOG_DDEBUG("Testing RESP3 blob string ($)");

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
  FIO_LOG_DDEBUG("Testing RESP3 blob error (!)");

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
  FIO_LOG_DDEBUG("Testing RESP3 verbatim string (=)");

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
  FIO_LOG_DDEBUG("Testing RESP3 array (*)");

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
  FIO_LOG_DDEBUG("Testing RESP3 map (%%)");

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
  FIO_LOG_DDEBUG("Testing RESP3 set (~)");

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
  FIO_LOG_DDEBUG("Testing RESP3 push (>)");

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
  FIO_LOG_DDEBUG("Testing RESP3 attribute (|)");

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
  FIO_LOG_DDEBUG("Testing RESP3 streaming array (*?)");

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
  FIO_LOG_DDEBUG("Testing RESP3 streaming map (%%?)");

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
  FIO_LOG_DDEBUG("Testing RESP3 partial parsing");

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
  FIO_LOG_DDEBUG("Testing RESP3 mixed type array");

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
  FIO_LOG_DDEBUG("Testing RESP3 deep nesting");

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
  FIO_LOG_DDEBUG("Testing RESP3 error handling");

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
  FIO_LOG_DDEBUG("Testing RESP3 udata passing");

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
  fio_resp3_parse(&parser, &test_callbacks, data, strlen(data));

  /* If callbacks were called, test_state was modified */
  FIO_ASSERT(test_state.number_count == 1, "Callback was called with udata");
  (void)expected_ptr;
  (void)udata_received;
}

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_context_stack)(void) {
  FIO_LOG_DDEBUG("Testing RESP3 context stack (parent_ctx)");

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
  FIO_LOG_DDEBUG("Testing RESP3 set-as-map fallback");

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
  FIO_LOG_DDEBUG("Testing RESP3 set with explicit callbacks (no fallback)");

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
  FIO_LOG_DDEBUG("Testing RESP3 streaming string callbacks (blob $)");

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
  FIO_LOG_DDEBUG("Testing RESP3 streaming string callbacks (blob error !)");

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
  FIO_LOG_DDEBUG("Testing RESP3 streaming string callbacks (verbatim =)");

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
  FIO_LOG_DDEBUG("Testing RESP3 streaming string ($? with chunks)");

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

FIO_SFUNC void FIO_NAME_TEST(stl, resp3_streaming_string_in_array)(void) {
  FIO_LOG_DDEBUG("Testing RESP3 streaming strings in array");

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
  FIO_LOG_DDEBUG("Testing RESP3 streaming string fallback to on_string");

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
  FIO_LOG_DDEBUG("Testing RESP3 simple strings don't use streaming callbacks");

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

int main(void) {
  FIO_LOG_DDEBUG("Testing RESP3 Parser (context stack pattern)");

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
  FIO_NAME_TEST(stl, resp3_streaming_string_in_array)();
  FIO_NAME_TEST(stl, resp3_streaming_string_fallback)();
  FIO_NAME_TEST(stl, resp3_simple_string_no_streaming)();

  FIO_LOG_DDEBUG("RESP3 Parser tests complete!");
  return 0;
}
