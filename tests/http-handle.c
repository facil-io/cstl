/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"
#define FIO_HTTP_HANDLE
#include FIO_INCLUDE_FILE

/* ===========================================================================
   Body-parse test helpers
   ===========================================================================
 */

/* Maximum number of key/value pairs we collect per test */
#define TEST_BODY_MAX_PAIRS 32
#define TEST_BODY_STR_MAX   256

/* Object type tags for our simple object system */
typedef enum {
  TEST_OBJ_STRING = 0x01,
  TEST_OBJ_NUMBER = 0x02,
  TEST_OBJ_FLOAT = 0x03,
  TEST_OBJ_NULL = 0x04,
  TEST_OBJ_TRUE = 0x05,
  TEST_OBJ_FALSE = 0x06,
  TEST_OBJ_MAP = 0x07,
  TEST_OBJ_ARRAY = 0x08,
} test_obj_type_e;

/* A heap-allocated object with a type tag */
typedef struct test_obj_s {
  test_obj_type_e type;
  union {
    struct {
      size_t len;
      char buf[1];
    } str;       /* TEST_OBJ_STRING */
    int64_t num; /* TEST_OBJ_NUMBER */
    double flt;  /* TEST_OBJ_FLOAT  */
  } u;
} test_obj_s;

/* Allocate a string object */
static test_obj_s *test_obj_str_new(const void *data, size_t len) {
  test_obj_s *o =
      (test_obj_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(test_obj_s) + len + 1, 0);
  if (!o)
    return NULL;
  o->type = TEST_OBJ_STRING;
  o->u.str.len = len;
  FIO_MEMCPY(o->u.str.buf, data, len);
  o->u.str.buf[len] = '\0';
  return o;
}

/* Allocate a number object */
static test_obj_s *test_obj_num_new(int64_t num) {
  test_obj_s *o = (test_obj_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(test_obj_s), 0);
  if (!o)
    return NULL;
  o->type = TEST_OBJ_NUMBER;
  o->u.num = num;
  return o;
}

/* Allocate a float object */
static test_obj_s *test_obj_float_new(double flt) {
  test_obj_s *o = (test_obj_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(test_obj_s), 0);
  if (!o)
    return NULL;
  o->type = TEST_OBJ_FLOAT;
  o->u.flt = flt;
  return o;
}

/* Allocate a sentinel object (null/true/false/map/array) */
static test_obj_s *test_obj_sentinel_new(test_obj_type_e type) {
  test_obj_s *o = (test_obj_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(test_obj_s), 0);
  if (!o)
    return NULL;
  o->type = type;
  return o;
}

/* Free any object */
static void test_obj_free(test_obj_s *o) {
  if (!o)
    return;
  FIO_MEM_FREE(o,
               sizeof(test_obj_s) +
                   (o->type == TEST_OBJ_STRING ? o->u.str.len + 1 : 0));
}

/* ---- Collected result ---- */

typedef struct {
  char key[TEST_BODY_STR_MAX];
  char val[TEST_BODY_STR_MAX];
  int is_file;
  char filename[TEST_BODY_STR_MAX];
  char content_type[TEST_BODY_STR_MAX];
} test_body_pair_s;

typedef struct {
  test_body_pair_s pairs[TEST_BODY_MAX_PAIRS];
  size_t count;
  int got_map;
  int got_array;
  int got_null;
  int got_true;
  int got_false;
  int64_t last_number;
  double last_float;
  int err_called;
  /* For file upload tracking */
  char pending_file_name[TEST_BODY_STR_MAX];
  char pending_file_filename[TEST_BODY_STR_MAX];
  char pending_file_ct[TEST_BODY_STR_MAX];
  char pending_file_data[TEST_BODY_STR_MAX];
  size_t pending_file_data_len;
} test_body_ctx_s;

/* Helper: copy a string safely into a fixed buffer */
static void test_body_strcpy(char *dst,
                             size_t dst_size,
                             const void *src,
                             size_t src_len) {
  size_t n = src_len < dst_size - 1 ? src_len : dst_size - 1;
  FIO_MEMCPY(dst, src, n);
  dst[n] = '\0';
}

/* Helper: get string representation of an object into a buffer */
static void test_obj_to_str(test_obj_s *o, char *dst, size_t dst_size) {
  if (!o) {
    test_body_strcpy(dst, dst_size, "(null-obj)", 10);
    return;
  }
  switch (o->type) {
  case TEST_OBJ_STRING:
    test_body_strcpy(dst, dst_size, o->u.str.buf, o->u.str.len);
    break;
  case TEST_OBJ_NUMBER: {
    char tmp[32];
    int n = snprintf(tmp, sizeof(tmp), "%" PRId64, o->u.num);
    test_body_strcpy(dst, dst_size, tmp, (size_t)(n > 0 ? n : 0));
    break;
  }
  case TEST_OBJ_FLOAT: test_body_strcpy(dst, dst_size, "<float>", 7); break;
  case TEST_OBJ_NULL: test_body_strcpy(dst, dst_size, "null", 4); break;
  case TEST_OBJ_TRUE: test_body_strcpy(dst, dst_size, "true", 4); break;
  case TEST_OBJ_FALSE: test_body_strcpy(dst, dst_size, "false", 5); break;
  case TEST_OBJ_MAP: test_body_strcpy(dst, dst_size, "<map>", 5); break;
  case TEST_OBJ_ARRAY: test_body_strcpy(dst, dst_size, "<array>", 7); break;
  default: test_body_strcpy(dst, dst_size, "<unknown>", 9); break;
  }
}

/* --- Callbacks --- */

static void *test_body_on_null(void *udata) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_null = 1;
  return test_obj_sentinel_new(TEST_OBJ_NULL);
}

static void *test_body_on_true(void *udata) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_true = 1;
  return test_obj_sentinel_new(TEST_OBJ_TRUE);
}

static void *test_body_on_false(void *udata) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_false = 1;
  return test_obj_sentinel_new(TEST_OBJ_FALSE);
}

static void *test_body_on_number(void *udata, int64_t num) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->last_number = num;
  return test_obj_num_new(num);
}

static void *test_body_on_float(void *udata, double num) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->last_float = num;
  return test_obj_float_new(num);
}

static void *test_body_on_string(void *udata, const void *data, size_t len) {
  (void)udata;
  return test_obj_str_new(data, len);
}

/* Map: allocate a sentinel map object */
static void *test_body_on_map(void *udata, void *parent) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_map = 1;
  (void)parent;
  return test_obj_sentinel_new(TEST_OBJ_MAP);
}

static void *test_body_on_array(void *udata, void *parent) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_array = 1;
  (void)parent;
  return test_obj_sentinel_new(TEST_OBJ_ARRAY);
}

static int test_body_map_set(void *udata,
                             void *map_obj,
                             void *key_obj,
                             void *value_obj) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  test_obj_s *map = (test_obj_s *)map_obj;
  test_obj_s *key = (test_obj_s *)key_obj;
  test_obj_s *val = (test_obj_s *)value_obj;

  /* Only record into the top-level map (type == TEST_OBJ_MAP) */
  if (map && map->type == TEST_OBJ_MAP && ctx->count < TEST_BODY_MAX_PAIRS) {
    test_body_pair_s *p = &ctx->pairs[ctx->count++];
    FIO_MEMSET(p, 0, sizeof(*p));
    if (key && key->type == TEST_OBJ_STRING)
      test_body_strcpy(p->key, sizeof(p->key), key->u.str.buf, key->u.str.len);
    test_obj_to_str(val, p->val, sizeof(p->val));
  }

  test_obj_free(key);
  test_obj_free(val);
  return 0;
}

static int test_body_array_push(void *udata, void *array, void *value) {
  (void)udata;
  (void)array;
  test_obj_free((test_obj_s *)value);
  return 0;
}

static void test_body_array_done(void *udata, void *array) {
  (void)udata;
  (void)array;
}

static void test_body_map_done(void *udata, void *map) {
  (void)udata;
  (void)map;
}

static void *test_body_on_error(void *udata, void *partial) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->err_called = 1;
  test_obj_free((test_obj_s *)partial);
  return NULL;
}

static void test_body_free_unused(void *udata, void *obj) {
  (void)udata;
  test_obj_free((test_obj_s *)obj);
}

/* File upload callbacks */
static void *test_body_on_file(void *udata,
                               fio_str_info_s name,
                               fio_str_info_s filename,
                               fio_str_info_s content_type) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  test_body_strcpy(ctx->pending_file_name,
                   sizeof(ctx->pending_file_name),
                   name.buf,
                   name.len);
  test_body_strcpy(ctx->pending_file_filename,
                   sizeof(ctx->pending_file_filename),
                   filename.buf,
                   filename.len);
  test_body_strcpy(ctx->pending_file_ct,
                   sizeof(ctx->pending_file_ct),
                   content_type.buf,
                   content_type.len);
  ctx->pending_file_data_len = 0;
  /* Return NULL: we handle file data ourselves via on_file_data/on_file_done.
   * Returning NULL prevents the multipart adapter from inserting file_ctx
   * into the map via map_set (which would cause a double-free). */
  return NULL;
}

static int test_body_on_file_data(void *udata,
                                  void *file,
                                  fio_buf_info_s data) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  (void)file;
  size_t avail = sizeof(ctx->pending_file_data) - ctx->pending_file_data_len;
  size_t n = data.len < avail ? data.len : avail;
  FIO_MEMCPY(ctx->pending_file_data + ctx->pending_file_data_len, data.buf, n);
  ctx->pending_file_data_len += n;
  return 0;
}

static void test_body_on_file_done(void *udata, void *file) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  (void)file;
  /* Record the file as a pair */
  if (ctx->count < TEST_BODY_MAX_PAIRS) {
    test_body_pair_s *p = &ctx->pairs[ctx->count++];
    FIO_MEMSET(p, 0, sizeof(*p));
    p->is_file = 1;
    test_body_strcpy(p->key,
                     sizeof(p->key),
                     ctx->pending_file_name,
                     strlen(ctx->pending_file_name));
    test_body_strcpy(p->filename,
                     sizeof(p->filename),
                     ctx->pending_file_filename,
                     strlen(ctx->pending_file_filename));
    test_body_strcpy(p->content_type,
                     sizeof(p->content_type),
                     ctx->pending_file_ct,
                     strlen(ctx->pending_file_ct));
    test_body_strcpy(p->val,
                     sizeof(p->val),
                     ctx->pending_file_data,
                     ctx->pending_file_data_len);
  }
}

/* The full callback set used by most tests */
static const fio_http_body_parse_callbacks_s TEST_BODY_CALLBACKS = {
    .on_null = test_body_on_null,
    .on_true = test_body_on_true,
    .on_false = test_body_on_false,
    .on_number = test_body_on_number,
    .on_float = test_body_on_float,
    .on_string = test_body_on_string,
    .on_array = test_body_on_array,
    .on_map = test_body_on_map,
    .array_push = test_body_array_push,
    .map_set = test_body_map_set,
    .array_done = test_body_array_done,
    .map_done = test_body_map_done,
    .on_file = test_body_on_file,
    .on_file_data = test_body_on_file_data,
    .on_file_done = test_body_on_file_done,
    .on_error = test_body_on_error,
    .free_unused = test_body_free_unused,
};

/* Helper: find a pair by key name, returns NULL if not found */
static test_body_pair_s *test_body_find(test_body_ctx_s *ctx, const char *key) {
  for (size_t i = 0; i < ctx->count; ++i) {
    if (strcmp(ctx->pairs[i].key, key) == 0)
      return &ctx->pairs[i];
  }
  return NULL;
}

/* Helper: set up an HTTP handle with content-type and body */
static fio_http_s *test_body_make_handle(const char *content_type,
                                         const char *body,
                                         size_t body_len) {
  fio_http_s *h = fio_http_new();
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"content-type", 12),
                              FIO_STR_INFO1((char *)content_type));
  if (body && body_len)
    fio_http_body_write(h, body, body_len);
  return h;
}

/* Helper: free the result object returned by fio_http_body_parse */
static void test_body_free_result(fio_http_body_parse_result_s *r) {
  test_obj_free((test_obj_s *)r->result);
  r->result = NULL;
}

/* ===========================================================================
   Tests: application/x-www-form-urlencoded
   ===========================================================================
 */

static void test_body_parse_urlencoded_simple(void) {
  fprintf(stderr, "  * fio_http_body_parse: urlencoded simple key=value\n");
  const char *body = "key1=val1&key2=val2";
  fio_http_s *h = test_body_make_handle("application/x-www-form-urlencoded",
                                        body,
                                        strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err, "urlencoded simple: parse should succeed (err=%d)", r.err);
  FIO_ASSERT(ctx.got_map, "urlencoded simple: should create a map");
  test_body_pair_s *p1 = test_body_find(&ctx, "key1");
  FIO_ASSERT(p1, "urlencoded simple: key1 not found");
  FIO_ASSERT(p1 && strcmp(p1->val, "val1") == 0,
             "urlencoded simple: key1 value mismatch (got '%s')",
             p1 ? p1->val : "(null)");
  test_body_pair_s *p2 = test_body_find(&ctx, "key2");
  FIO_ASSERT(p2, "urlencoded simple: key2 not found");
  FIO_ASSERT(p2 && strcmp(p2->val, "val2") == 0,
             "urlencoded simple: key2 value mismatch (got '%s')",
             p2 ? p2->val : "(null)");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_urlencoded_encoded(void) {
  fprintf(stderr,
          "  * fio_http_body_parse: urlencoded percent/plus encoding\n");
  const char *body = "name=hello+world&city=New%20York";
  fio_http_s *h = test_body_make_handle("application/x-www-form-urlencoded",
                                        body,
                                        strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err,
             "urlencoded encoded: parse should succeed (err=%d)",
             r.err);
  test_body_pair_s *pname = test_body_find(&ctx, "name");
  FIO_ASSERT(pname, "urlencoded encoded: 'name' key not found");
  FIO_ASSERT(pname && strcmp(pname->val, "hello world") == 0,
             "urlencoded encoded: 'name' value mismatch (got '%s')",
             pname ? pname->val : "(null)");
  test_body_pair_s *pcity = test_body_find(&ctx, "city");
  FIO_ASSERT(pcity, "urlencoded encoded: 'city' key not found");
  FIO_ASSERT(pcity && strcmp(pcity->val, "New York") == 0,
             "urlencoded encoded: 'city' value mismatch (got '%s')",
             pcity ? pcity->val : "(null)");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_urlencoded_empty_value(void) {
  fprintf(stderr, "  * fio_http_body_parse: urlencoded empty value\n");
  const char *body = "key=&other=val";
  fio_http_s *h = test_body_make_handle("application/x-www-form-urlencoded",
                                        body,
                                        strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err,
             "urlencoded empty value: parse should succeed (err=%d)",
             r.err);
  test_body_pair_s *pk = test_body_find(&ctx, "key");
  FIO_ASSERT(pk, "urlencoded empty value: 'key' not found");
  FIO_ASSERT(pk && pk->val[0] == '\0',
             "urlencoded empty value: 'key' should have empty value (got '%s')",
             pk ? pk->val : "(null)");
  test_body_pair_s *po = test_body_find(&ctx, "other");
  FIO_ASSERT(po, "urlencoded empty value: 'other' not found");
  FIO_ASSERT(po && strcmp(po->val, "val") == 0,
             "urlencoded empty value: 'other' value mismatch (got '%s')",
             po ? po->val : "(null)");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_urlencoded_key_only(void) {
  fprintf(stderr, "  * fio_http_body_parse: urlencoded key-only (no '=')\n");
  const char *body = "key&other=val";
  fio_http_s *h = test_body_make_handle("application/x-www-form-urlencoded",
                                        body,
                                        strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err,
             "urlencoded key-only: parse should succeed (err=%d)",
             r.err);
  test_body_pair_s *pk = test_body_find(&ctx, "key");
  FIO_ASSERT(pk, "urlencoded key-only: 'key' not found");
  FIO_ASSERT(pk && pk->val[0] == '\0',
             "urlencoded key-only: 'key' should have empty value (got '%s')",
             pk ? pk->val : "(null)");
  test_body_pair_s *po = test_body_find(&ctx, "other");
  FIO_ASSERT(po, "urlencoded key-only: 'other' not found");
  FIO_ASSERT(po && strcmp(po->val, "val") == 0,
             "urlencoded key-only: 'other' value mismatch (got '%s')",
             po ? po->val : "(null)");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_urlencoded_multi_value(void) {
  fprintf(stderr,
          "  * fio_http_body_parse: urlencoded multiple values same key\n");
  const char *body = "color=red&color=blue";
  fio_http_s *h = test_body_make_handle("application/x-www-form-urlencoded",
                                        body,
                                        strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err,
             "urlencoded multi-value: parse should succeed (err=%d)",
             r.err);
  /* Both pairs should be present (map_set called twice for "color") */
  size_t color_count = 0;
  for (size_t i = 0; i < ctx.count; ++i) {
    if (strcmp(ctx.pairs[i].key, "color") == 0)
      ++color_count;
  }
  FIO_ASSERT(color_count == 2,
             "urlencoded multi-value: expected 2 'color' entries, got %zu",
             color_count);
  test_body_free_result(&r);
  fio_http_free(h);
}

/* ===========================================================================
   Tests: application/json
   ===========================================================================
 */

static void test_body_parse_json_simple(void) {
  fprintf(stderr, "  * fio_http_body_parse: JSON simple object\n");
  const char *body = "{\"key\": \"value\", \"num\": 42}";
  fio_http_s *h = test_body_make_handle("application/json", body, strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err, "JSON simple: parse should succeed (err=%d)", r.err);
  FIO_ASSERT(ctx.got_map, "JSON simple: should create a map");
  test_body_pair_s *pk = test_body_find(&ctx, "key");
  FIO_ASSERT(pk, "JSON simple: 'key' not found");
  FIO_ASSERT(pk && strcmp(pk->val, "value") == 0,
             "JSON simple: 'key' value mismatch (got '%s')",
             pk ? pk->val : "(null)");
  FIO_ASSERT(ctx.last_number == 42,
             "JSON simple: expected last_number=42, got %" PRId64,
             ctx.last_number);
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_json_nested(void) {
  fprintf(stderr, "  * fio_http_body_parse: JSON nested object\n");
  const char *body = "{\"outer\": {\"inner\": \"deep\"}}";
  fio_http_s *h = test_body_make_handle("application/json", body, strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err, "JSON nested: parse should succeed (err=%d)", r.err);
  FIO_ASSERT(ctx.got_map, "JSON nested: should create a map");
  /* The outer key "outer" should be recorded with value "<map>" */
  test_body_pair_s *po = test_body_find(&ctx, "outer");
  FIO_ASSERT(po, "JSON nested: 'outer' key not found");
  FIO_ASSERT(po && strcmp(po->val, "<map>") == 0,
             "JSON nested: 'outer' should have map value (got '%s')",
             po ? po->val : "(null)");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_json_array(void) {
  fprintf(stderr, "  * fio_http_body_parse: JSON array value\n");
  const char *body = "[\"a\", \"b\", \"c\"]";
  fio_http_s *h = test_body_make_handle("application/json", body, strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err, "JSON array: parse should succeed (err=%d)", r.err);
  FIO_ASSERT(ctx.got_array, "JSON array: should create an array");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_json_invalid(void) {
  fprintf(stderr, "  * fio_http_body_parse: JSON invalid (should not crash)\n");
  const char *body = "{invalid json!!!}";
  fio_http_s *h = test_body_make_handle("application/json", body, strlen(body));
  test_body_ctx_s ctx = {0};
  /* Must not crash - result may have err set */
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  /* We only require no crash; err may or may not be set */
  test_body_free_result(&r);
  fio_http_free(h);
  fprintf(stderr, "    (invalid JSON did not crash - OK, err=%d)\n", r.err);
}

/* ===========================================================================
   Tests: multipart/form-data
   ===========================================================================
 */

/* Build a multipart body with a given boundary.
   Variadic args: name, value, filename (or NULL), content_type (or NULL),
   terminated by NULL name. */
static size_t test_body_build_multipart(char *buf,
                                        size_t buf_size,
                                        const char *boundary,
                                        ...) {
  size_t pos = 0;
#define MP_APPEND(s)                                                           \
  do {                                                                         \
    size_t _l = strlen(s);                                                     \
    if (pos + _l < buf_size) {                                                 \
      FIO_MEMCPY(buf + pos, s, _l);                                            \
      pos += _l;                                                               \
    }                                                                          \
  } while (0)

  va_list ap;
  va_start(ap, boundary);
  for (;;) {
    const char *name = va_arg(ap, const char *);
    if (!name)
      break;
    const char *value = va_arg(ap, const char *);
    const char *filename = va_arg(ap, const char *); /* NULL for plain field */
    const char *ct = va_arg(ap, const char *);       /* NULL for plain field */

    MP_APPEND("--");
    MP_APPEND(boundary);
    MP_APPEND("\r\n");
    MP_APPEND("Content-Disposition: form-data; name=\"");
    MP_APPEND(name);
    MP_APPEND("\"");
    if (filename) {
      MP_APPEND("; filename=\"");
      MP_APPEND(filename);
      MP_APPEND("\"");
    }
    MP_APPEND("\r\n");
    if (ct) {
      MP_APPEND("Content-Type: ");
      MP_APPEND(ct);
      MP_APPEND("\r\n");
    }
    MP_APPEND("\r\n");
    MP_APPEND(value);
    MP_APPEND("\r\n");
  }
  va_end(ap);

  MP_APPEND("--");
  MP_APPEND(boundary);
  MP_APPEND("--\r\n");

  if (pos < buf_size)
    buf[pos] = '\0';
  return pos;
#undef MP_APPEND
}

static void test_body_parse_multipart_simple_field(void) {
  fprintf(stderr, "  * fio_http_body_parse: multipart simple text field\n");
  const char *boundary = "testboundary123";
  char body[1024];
  size_t body_len = test_body_build_multipart(body,
                                              sizeof(body),
                                              boundary,
                                              "field1",
                                              "hello",
                                              (const char *)NULL,
                                              (const char *)NULL,
                                              (const char *)NULL);

  char ct[128];
  snprintf(ct, sizeof(ct), "multipart/form-data; boundary=%s", boundary);
  fio_http_s *h = test_body_make_handle(ct, body, body_len);
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err,
             "multipart simple field: parse should succeed (err=%d)",
             r.err);
  FIO_ASSERT(ctx.got_map, "multipart simple field: should create a map");
  test_body_pair_s *pf = test_body_find(&ctx, "field1");
  FIO_ASSERT(pf, "multipart simple field: 'field1' not found");
  FIO_ASSERT(pf && strcmp(pf->val, "hello") == 0,
             "multipart simple field: 'field1' value mismatch (got '%s')",
             pf ? pf->val : "(null)");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_multipart_multiple_fields(void) {
  fprintf(stderr, "  * fio_http_body_parse: multipart multiple fields\n");
  const char *boundary = "multiboundary456";
  char body[2048];
  size_t body_len = test_body_build_multipart(body,
                                              sizeof(body),
                                              boundary,
                                              "name",
                                              "Alice",
                                              (const char *)NULL,
                                              (const char *)NULL,
                                              "email",
                                              "alice@example.com",
                                              (const char *)NULL,
                                              (const char *)NULL,
                                              (const char *)NULL);

  char ct[128];
  snprintf(ct, sizeof(ct), "multipart/form-data; boundary=%s", boundary);
  fio_http_s *h = test_body_make_handle(ct, body, body_len);
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err,
             "multipart multiple fields: parse should succeed (err=%d)",
             r.err);
  test_body_pair_s *pn = test_body_find(&ctx, "name");
  FIO_ASSERT(pn, "multipart multiple fields: 'name' not found");
  FIO_ASSERT(pn && strcmp(pn->val, "Alice") == 0,
             "multipart multiple fields: 'name' mismatch (got '%s')",
             pn ? pn->val : "(null)");
  test_body_pair_s *pe = test_body_find(&ctx, "email");
  FIO_ASSERT(pe, "multipart multiple fields: 'email' not found");
  FIO_ASSERT(pe && strcmp(pe->val, "alice@example.com") == 0,
             "multipart multiple fields: 'email' mismatch (got '%s')",
             pe ? pe->val : "(null)");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_multipart_file_upload(void) {
  fprintf(stderr, "  * fio_http_body_parse: multipart file upload\n");
  const char *boundary = "fileboundary789";
  char body[2048];
  size_t body_len = test_body_build_multipart(body,
                                              sizeof(body),
                                              boundary,
                                              "upload",
                                              "file content here",
                                              "test.txt",
                                              "text/plain",
                                              (const char *)NULL);

  char ct[128];
  snprintf(ct, sizeof(ct), "multipart/form-data; boundary=%s", boundary);
  fio_http_s *h = test_body_make_handle(ct, body, body_len);
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err,
             "multipart file upload: parse should succeed (err=%d)",
             r.err);
  /* Find the file entry */
  test_body_pair_s *pf = NULL;
  for (size_t i = 0; i < ctx.count; ++i) {
    if (ctx.pairs[i].is_file && strcmp(ctx.pairs[i].key, "upload") == 0) {
      pf = &ctx.pairs[i];
      break;
    }
  }
  FIO_ASSERT(pf, "multipart file upload: 'upload' file entry not found");
  FIO_ASSERT(pf && strcmp(pf->filename, "test.txt") == 0,
             "multipart file upload: filename mismatch (got '%s')",
             pf ? pf->filename : "(null)");
  FIO_ASSERT(pf && strcmp(pf->val, "file content here") == 0,
             "multipart file upload: file data mismatch (got '%s')",
             pf ? pf->val : "(null)");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_multipart_boundary_parsing(void) {
  fprintf(stderr, "  * fio_http_body_parse: multipart boundary parsing\n");
  /* Test that boundary is correctly extracted from Content-Type with quotes */
  const char *boundary = "my-special-boundary";
  char body[1024];
  size_t body_len = test_body_build_multipart(body,
                                              sizeof(body),
                                              boundary,
                                              "x",
                                              "y",
                                              (const char *)NULL,
                                              (const char *)NULL,
                                              (const char *)NULL);

  char ct[128];
  snprintf(ct, sizeof(ct), "multipart/form-data; boundary=\"%s\"", boundary);
  fio_http_s *h = test_body_make_handle(ct, body, body_len);
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err,
             "multipart boundary parsing (quoted): parse should succeed "
             "(err=%d)",
             r.err);
  test_body_pair_s *px = test_body_find(&ctx, "x");
  FIO_ASSERT(px, "multipart boundary parsing: 'x' not found");
  FIO_ASSERT(px && strcmp(px->val, "y") == 0,
             "multipart boundary parsing: 'x' value mismatch (got '%s')",
             px ? px->val : "(null)");
  test_body_free_result(&r);
  fio_http_free(h);
}

/* ===========================================================================
   Tests: Edge cases
   ===========================================================================
 */

static void test_body_parse_empty_body(void) {
  fprintf(stderr, "  * fio_http_body_parse: empty body\n");
  fio_http_s *h = fio_http_new();
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"content-type", 12),
      FIO_STR_INFO1((char *)"application/x-www-form-urlencoded"));
  /* No body written */
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  /* Should not crash; empty body is valid (0 pairs) */
  FIO_ASSERT(ctx.count == 0,
             "empty body: should have 0 pairs (got %zu)",
             ctx.count);
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_no_content_type(void) {
  fprintf(stderr,
          "  * fio_http_body_parse: no Content-Type header (unknown type)\n");
  fio_http_s *h = fio_http_new();
  fio_http_body_write(h, "key=val", 7);
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  /* Unknown content type - should return err, not crash */
  FIO_ASSERT(r.err != 0,
             "no content-type: should return error for unknown type");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_unknown_content_type(void) {
  fprintf(stderr,
          "  * fio_http_body_parse: unknown Content-Type (no-op, no crash)\n");
  fio_http_s *h = test_body_make_handle("text/plain", "hello world", 11);
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  /* Should not crash; should return err for unknown type */
  FIO_ASSERT(r.err != 0,
             "unknown content-type: should return error, not crash");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_body_not_parsed(void) {
  fprintf(stderr, "  * fio_http_body_parse: body set but parse not called\n");
  fio_http_s *h =
      test_body_make_handle("application/x-www-form-urlencoded", "key=val", 7);
  /* Do NOT call fio_http_body_parse */
  /* Verify body is accessible */
  fio_http_body_seek(h, 0);
  fio_str_info_s body = fio_http_body_read(h, (size_t)-1);
  FIO_ASSERT(body.len == 7,
             "body not parsed: body should still be readable (len=%zu)",
             body.len);
  fio_http_free(h);
}

static void test_body_parse_null_callbacks(void) {
  fprintf(stderr,
          "  * fio_http_body_parse: NULL callbacks (should return error)\n");
  fio_http_s *h = test_body_make_handle("application/json", "{\"k\":1}", 7);
  fio_http_body_parse_result_s r = fio_http_body_parse(h, NULL, NULL);
  FIO_ASSERT(r.err != 0, "null callbacks: should return error");
  fio_http_free(h);
}

static void test_body_parse_null_handle(void) {
  fprintf(stderr,
          "  * fio_http_body_parse: NULL handle (should return error)\n");
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(NULL, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(r.err != 0, "null handle: should return error");
}

/* ===========================================================================
   Main
   ===========================================================================
 */

int main(void) {
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(!fio_http_cdata(h), "fio_http_cdata should start as NULL");
  fio_http_cdata_set(h, (void *)(uintptr_t)42);
  FIO_ASSERT((uintptr_t)fio_http_cdata(h) == 42,
             "fio_http_cdata roundtrip error");
  FIO_ASSERT(!fio_http_udata(h), "fio_http_udata should start as NULL");
  fio_http_udata_set(h, (void *)(uintptr_t)43);
  FIO_ASSERT((uintptr_t)fio_http_udata(h) == 43,
             "fio_http_udata roundtrip error");
  FIO_ASSERT(!fio_http_udata2(h), "fio_http_udata2 should start as NULL");
  fio_http_udata2_set(h, (void *)(uintptr_t)44);
  FIO_ASSERT((uintptr_t)fio_http_udata2(h) == 44,
             "fio_http_udata2 roundtrip error");

  FIO_ASSERT(!fio_http_status(h), "fio_http_status should start as NULL");
  fio_http_status_set(h, 101);
  FIO_ASSERT((uintptr_t)fio_http_status(h) == 101,
             "fio_http_status roundtrip error");

  FIO_ASSERT(!fio_http_method(h).buf, "fio_http_method should start as empty");
  fio_http_method_set(h, FIO_STR_INFO1((char *)"POST"));
  FIO_ASSERT(
      FIO_STR_INFO_IS_EQ(fio_http_method(h), FIO_STR_INFO1((char *)"POST")),
      "fio_http_method roundtrip error");

  FIO_ASSERT(!fio_http_path(h).buf, "fio_http_path should start as empty");
  fio_http_path_set(h, FIO_STR_INFO1((char *)"/path"));
  FIO_ASSERT(
      FIO_STR_INFO_IS_EQ(fio_http_path(h), FIO_STR_INFO1((char *)"/path")),
      "fio_http_path roundtrip error");

  FIO_ASSERT(!fio_http_query(h).buf, "fio_http_query should start as empty");
  fio_http_query_set(h, FIO_STR_INFO1((char *)"query=null"));
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_query(h),
                                FIO_STR_INFO1((char *)"query=null")),
             "fio_http_query roundtrip error");

  FIO_ASSERT(!fio_http_version(h).buf,
             "fio_http_version should start as empty");
  fio_http_version_set(h, FIO_STR_INFO1((char *)"HTTP/1.1"));
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_version(h),
                                FIO_STR_INFO1((char *)"HTTP/1.1")),
             "fio_http_version roundtrip error");

  { /* test multiple header support */
    fio_str_info_s test_data[] = {
        FIO_STR_INFO1((char *)"header-name"),
        FIO_STR_INFO1(
            (char *)"a long enough value to require memory allocation 001"),
        FIO_STR_INFO1(
            (char *)"a long enough value to require memory allocation 002"),
        FIO_STR_INFO1(
            (char *)"a long enough value to require memory allocation 003"),
        FIO_STR_INFO1(
            (char *)"a long enough value to require memory allocation 004"),
        FIO_STR_INFO1(
            (char *)"a long enough value to require memory allocation 005"),
        FIO_STR_INFO1(
            (char *)"a long enough value to require memory allocation 006"),
        FIO_STR_INFO1(
            (char *)"a long enough value to require memory allocation 007"),
        FIO_STR_INFO1(
            (char *)"a long enough value to require memory allocation 008"),
    };
    size_t count = sizeof(test_data) / sizeof(test_data[0]);
    FIO_ASSERT(!fio_http_request_header(h, test_data[0], 0).buf,
               "fio_http_request_header should start as empty");
    FIO_ASSERT(!fio_http_response_header(h, test_data[0], 0).buf,
               "fio_http_response_header should start as empty");
    for (size_t i = 1; i < count; ++i) {
      FIO_ASSERT(!fio_http_request_header(h, test_data[0], i - 1ULL).buf,
                 "fio_http_request_header index (%zu) should start as empty",
                 (size_t)(i - 1ULL));
      FIO_ASSERT(!fio_http_response_header(h, test_data[0], i - 1ULL).buf,
                 "fio_http_response_header index (%zu) should start as empty",
                 (size_t)(i - 1ULL));
      fio_str_info_s req_h =
          fio_http_request_header_add(h, test_data[0], test_data[i]);
      fio_str_info_s res_h =
          fio_http_response_header_add(h, test_data[0], test_data[i]);
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(req_h, test_data[i]),
                 "fio_http_request_header_set error");
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(res_h, test_data[i]),
                 "fio_http_response_header_set error");
      req_h = fio_http_request_header(h, test_data[0], i - 1ULL);
      res_h = fio_http_response_header(h, test_data[0], i - 1ULL);
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(req_h, test_data[i]),
                 "fio_http_request_header_set error");
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(res_h, test_data[i]),
                 "fio_http_response_header_set error");
    }
    for (size_t i = 0; i < count - 1; ++i) {
      fio_str_info_s req_h = fio_http_request_header(h, test_data[0], i);
      fio_str_info_s res_h = fio_http_response_header(h, test_data[0], i);
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(req_h, test_data[i + 1]),
                 "fio_http_request_header_set error");
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(res_h, test_data[i + 1]),
                 "fio_http_response_header_set error");
    }
    fio_http_request_header_set(h, test_data[0], test_data[1]);
    fio_http_response_header_set(h, test_data[0], test_data[1]);
    FIO_ASSERT(!fio_http_request_header(h, test_data[0], 1ULL).buf,
               "fio_http_request_header_set index should reset header values");
    FIO_ASSERT(!fio_http_response_header(h, test_data[0], 1ULL).buf,
               "fio_http_response_header_set index should reset header values");
  }
  { /* test body writer */
    size_t written = 0;
    do {
      union {
        char buf[32];
        uint64_t u64[4];
        void *p[4];
      } w, r;
      fio_rand_bytes(r.buf, sizeof(r.buf));
      fio_http_body_write(h, r.buf, sizeof(r.buf));
      fio_http_body_seek(h, written);
      fio_str_info_s got = fio_http_body_read(h, sizeof(r.buf));
      FIO_MEMSET(w.buf, 0, sizeof(w.buf));
      FIO_MEMCPY(w.buf, got.buf, got.len);
      written += sizeof(r.buf);
      FIO_ASSERT(written == fio_http_body_length(h),
                 "fio_http_body_length error (%zu != %zu)",
                 fio_http_body_length(h),
                 written);
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(FIO_STR_INFO2(r.buf, sizeof(r.buf)), got),
                 "fio_http_body_write-fio_http_body_read roundtrip error @ %zu"
                 "\n\t expected (32):\t%p%p%p%p)"
                 "\n\t got (%zu):     \t%p%p%p%p)",
                 written - sizeof(r.buf),
                 r.p[0],
                 r.p[1],
                 r.p[2],
                 r.p[3],
                 got.len,
                 w.p[0],
                 w.p[1],
                 w.p[2],
                 w.p[3]);
    } while (written < (FIO_HTTP_BODY_RAM_LIMIT << 1));
    fio_http_body_seek(h, 0);
    fio_http_body_write(h, "\n1234", 5);
    fio_str_info_s ln = fio_http_body_read_until(h, '\n', 0);
    FIO_ASSERT(ln.buf && ln.len && ln.buf[ln.len - 1] == '\n',
               "fio_http_body_read_until token error");
  }

  /* almost done, just make sure reference counting doesn't destroy object */
  fio_http_free(fio_http_dup(h));
  FIO_ASSERT(
      (uintptr_t)fio_http_udata2(h) == 44 &&
          FIO_STR_INFO_IS_EQ(fio_http_method(h), FIO_STR_INFO1((char *)"POST")),
      "fio_http_s reference counting shouldn't object");

  fio_http_free(h);

  /* ===========================================================================
     Body parsing tests
     ===========================================================================
   */
  fprintf(stderr, "\nTesting fio_http_body_parse:\n");

  /* URL-encoded tests */
  test_body_parse_urlencoded_simple();
  test_body_parse_urlencoded_encoded();
  test_body_parse_urlencoded_empty_value();
  test_body_parse_urlencoded_key_only();
  test_body_parse_urlencoded_multi_value();

  /* JSON tests */
  test_body_parse_json_simple();
  test_body_parse_json_nested();
  test_body_parse_json_array();
  test_body_parse_json_invalid();

  /* Multipart tests */
  test_body_parse_multipart_simple_field();
  test_body_parse_multipart_multiple_fields();
  test_body_parse_multipart_file_upload();
  test_body_parse_multipart_boundary_parsing();

  /* Edge case tests */
  test_body_parse_empty_body();
  test_body_parse_no_content_type();
  test_body_parse_unknown_content_type();
  test_body_parse_body_not_parsed();
  test_body_parse_null_callbacks();
  test_body_parse_null_handle();

  fprintf(stderr, "\nAll fio_http_body_parse tests passed!\n");
}
