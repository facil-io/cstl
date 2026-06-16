/* *****************************************************************************
Test: HTTP handle correctness
***************************************************************************** */
#define FIO_HTTP_HANDLE
#include "test-helpers.h"

/* ===========================================================================
   Handle lifecycle and simple properties
   ===========================================================================
 */

static void test_handle_lifecycle(void) {
  fprintf(stderr, "  * handle lifecycle\n");
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(h, "fio_http_new returned NULL");
  FIO_ASSERT(fio_http_is_clean(h), "new handle should be clean");

  fio_http_s *d = fio_http_dup(h);
  FIO_ASSERT(d, "fio_http_dup returned NULL");
  FIO_ASSERT(d == h, "dup should return same pointer for ref count 1");

  fio_http_free(d); /* drops extra ref */
  FIO_ASSERT(!fio_http_is_freeing(h), "free after dup should not free object");

  fio_http_s *copy = fio_http_new_copy_request(h);
  FIO_ASSERT(copy, "fio_http_new_copy_request returned NULL");
  FIO_ASSERT(copy != h, "copy should be a new object");
  FIO_ASSERT(fio_http_is_clean(copy), "copy should be clean");
  fio_http_free(copy);

  fio_http_free(h); /* final free */
}

static void test_handle_udata(void) {
  fprintf(stderr, "  * udata / cdata\n");
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(!fio_http_udata(h), "udata should start as NULL");
  fio_http_udata_set(h, (void *)(uintptr_t)0xAA);
  FIO_ASSERT((uintptr_t)fio_http_udata(h) == 0xAA,
             "udata roundtrip error");

  FIO_ASSERT(!fio_http_udata2(h), "udata2 should start as NULL");
  fio_http_udata2_set(h, (void *)(uintptr_t)0xBB);
  FIO_ASSERT((uintptr_t)fio_http_udata2(h) == 0xBB,
             "udata2 roundtrip error");

  FIO_ASSERT(!fio_http_cdata(h), "cdata should start as NULL");
  fio_http_cdata_set(h, (void *)(uintptr_t)0xCC);
  FIO_ASSERT((uintptr_t)fio_http_cdata(h) == 0xCC,
             "cdata roundtrip error");

  fio_http_controller_s *controller = fio_http_controller(h);
  FIO_ASSERT(controller, "controller should not be NULL (mock controller)");

  fio_http_free(h);
}

static void test_handle_status(void) {
  fprintf(stderr, "  * status\n");
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(fio_http_status(h) == 0, "status should start as 0");
  fio_http_status_set(h, 404);
  FIO_ASSERT(fio_http_status(h) == 404, "status roundtrip error");
  fio_http_status_set(h, 0); /* zero normalizes to 200 */
  FIO_ASSERT(fio_http_status(h) == 200, "status 0 should normalize to 200");
  fio_http_status_set(h, 2000); /* clamped */
  FIO_ASSERT(fio_http_status(h) == 500, "status >1023 should clamp to 500");
  fio_http_free(h);
}

static void test_handle_request_line(void) {
  fprintf(stderr, "  * request line properties\n");
  fio_http_s *h = fio_http_new();

  FIO_ASSERT(!fio_http_method(h).buf, "method should start empty");
  fio_http_method_set(h, FIO_STR_INFO1((char *)"POST"));
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_method(h),
                                FIO_STR_INFO1((char *)"POST")),
             "method roundtrip error");

  FIO_ASSERT(!fio_http_path(h).buf, "path should start empty");
  fio_http_path_set(h, FIO_STR_INFO1((char *)"/path/to/resource"));
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_path(h),
                                FIO_STR_INFO1((char *)"/path/to/resource")),
             "path roundtrip error");

  FIO_ASSERT(!fio_http_opath(h).buf, "opath should start empty");
  fio_http_opath_set(h, FIO_STR_INFO1((char *)"/original/path"));
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_opath(h),
                                FIO_STR_INFO1((char *)"/original/path")),
             "opath roundtrip error");

  FIO_ASSERT(!fio_http_query(h).buf, "query should start empty");
  fio_http_query_set(h, FIO_STR_INFO1((char *)"a=1&b=2"));
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_query(h),
                                FIO_STR_INFO1((char *)"a=1&b=2")),
             "query roundtrip error");

  FIO_ASSERT(!fio_http_version(h).buf, "version should start empty");
  fio_http_version_set(h, FIO_STR_INFO1((char *)"HTTP/1.1"));
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_version(h),
                                FIO_STR_INFO1((char *)"HTTP/1.1")),
             "version roundtrip error");

  fio_http_free(h);
}

static void test_handle_copy_request(void) {
  fprintf(stderr, "  * copy request data\n");
  fio_http_s *h = fio_http_new();
  fio_http_method_set(h, FIO_STR_INFO1((char *)"GET"));
  fio_http_path_set(h, FIO_STR_INFO1((char *)"/api"));
  fio_http_query_set(h, FIO_STR_INFO1((char *)"x=y"));
  fio_http_version_set(h, FIO_STR_INFO1((char *)"HTTP/1.1"));
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"x-custom", 8),
      FIO_STR_INFO1((char *)"value"));

  fio_http_s *copy = fio_http_new_copy_request(h);
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_method(copy),
                                FIO_STR_INFO1((char *)"GET")),
             "copy method mismatch");
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_path(copy),
                                FIO_STR_INFO1((char *)"/api")),
             "copy path mismatch");
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_query(copy),
                                FIO_STR_INFO1((char *)"x=y")),
             "copy query mismatch");
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_request_header(
                                    copy,
                                    FIO_STR_INFO2((char *)"x-custom", 8),
                                    0),
                                FIO_STR_INFO1((char *)"value")),
             "copy header mismatch");

  fio_http_free(copy);
  fio_http_free(h);
}

/* ===========================================================================
   Headers
   ===========================================================================
 */

static int test_header_each_callback(fio_http_s *h,
                                     fio_str_info_s name,
                                     fio_str_info_s value,
                                     void *udata) {
  (void)h;
  size_t *count = (size_t *)udata;
  (*count)++;
  (void)name;
  (void)value;
  return 0;
}

static void test_handle_request_headers(void) {
  fprintf(stderr, "  * request headers\n");
  fio_http_s *h = fio_http_new();
  fio_str_info_s name = FIO_STR_INFO2((char *)"x-test", 6);
  fio_str_info_s v1 = FIO_STR_INFO1((char *)"first");
  fio_str_info_s v2 = FIO_STR_INFO1((char *)"second");

  FIO_ASSERT(!fio_http_request_header(h, name, 0).buf,
             "missing header should be empty");
  FIO_ASSERT(fio_http_request_header_count(h, name) == 0,
             "missing header count should be 0");

  fio_http_request_header_add(h, name, v1);
  fio_http_request_header_add(h, name, v2);
  FIO_ASSERT(fio_http_request_header_count(h, name) == 2,
             "expected 2 values for x-test");
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_request_header(h, name, 0), v1),
             "first value mismatch");
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_request_header(h, name, 1), v2),
             "second value mismatch");

  fio_http_request_header_set(h, name, v1);
  FIO_ASSERT(fio_http_request_header_count(h, name) == 1,
             "set should replace all values");
  FIO_ASSERT(!fio_http_request_header(h, name, 1).buf,
             "index 1 should be empty after set");

  fio_http_request_header_set_if_missing(h, name, v2);
  FIO_ASSERT(fio_http_request_header_count(h, name) == 1,
             "set_if_missing should not overwrite");

  fio_http_request_header_add(h, name, v2);
  size_t count = 0;
  FIO_ASSERT(fio_http_request_header_each(h, test_header_each_callback, &count)
                 == 1,
             "each should return unique header count");
  FIO_ASSERT(count == 2, "each callback count mismatch");
  FIO_ASSERT(fio_http_request_header_each(h, NULL, NULL) == 1,
             "each with NULL callback should return unique header count");

  fio_http_free(h);
}

static void test_handle_response_headers(void) {
  fprintf(stderr, "  * response headers\n");
  fio_http_s *h = fio_http_new();
  fio_str_info_s name = FIO_STR_INFO2((char *)"x-resp", 6);
  fio_str_info_s v1 = FIO_STR_INFO1((char *)"a");
  fio_str_info_s v2 = FIO_STR_INFO1((char *)"b");

  fio_http_response_header_add(h, name, v1);
  fio_http_response_header_add(h, name, v2);
  FIO_ASSERT(fio_http_response_header_count(h, name) == 2,
             "expected 2 response values");
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_response_header(h, name, 0), v1),
             "first response value mismatch");

  fio_http_response_header_set(h, name, v1);
  FIO_ASSERT(fio_http_response_header_count(h, name) == 1,
             "set should replace response values");

  fio_http_clear_response(h, 0);
  FIO_ASSERT(fio_http_response_header_count(h, name) == 0,
             "clear_response should remove response headers");
  FIO_ASSERT(fio_http_is_clean(h), "clear_response should reset state");

  fio_http_free(h);
}

/* ===========================================================================
   Body
   ===========================================================================
 */

static void test_handle_body(void) {
  fprintf(stderr, "  * body read/write/seek\n");
  fio_http_s *h = fio_http_new();

  FIO_ASSERT(fio_http_body_length(h) == 0, "new body length should be 0");
  FIO_ASSERT(fio_http_body_fd(h) == -1, "new body fd should be -1");

  fio_http_body_write(h, "Hello", 5);
  FIO_ASSERT(fio_http_body_length(h) == 5, "body length after write");

  fio_http_body_write(h, " World", 6);
  FIO_ASSERT(fio_http_body_length(h) == 11, "body length after second write");

  fio_http_body_seek(h, 0);
  fio_str_info_s r = fio_http_body_read(h, 5);
  FIO_ASSERT(r.len == 5, "read length mismatch");
  FIO_ASSERT(!memcmp(r.buf, "Hello", 5), "read content mismatch");

  fio_http_body_seek(h, -6); /* negative from end */
  r = fio_http_body_read(h, 6);
  FIO_ASSERT(r.len == 6, "negative seek read length mismatch");
  FIO_ASSERT(!memcmp(r.buf, " World", 6), "negative seek read content mismatch");

  fio_http_body_seek(h, 0);
  r = fio_http_body_read_until(h, ' ', 0);
  FIO_ASSERT(r.len == 6, "read_until length mismatch");
  FIO_ASSERT(r.buf[r.len - 1] == ' ', "read_until token mismatch");

  fio_http_free(h);
}

static void test_handle_body_file_spill(void) {
  fprintf(stderr, "  * body spills to file above RAM limit\n");
  fio_http_s *h = fio_http_new();
  size_t chunk = 4096;
  size_t total = 0;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, chunk, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMSET(buf, 'x', chunk);

  while (total < (FIO_HTTP_BODY_RAM_LIMIT << 1)) {
    fio_http_body_write(h, buf, chunk);
    total += chunk;
  }
  FIO_ASSERT(fio_http_body_length(h) == total, "file-spill body length");
  /* The implementation may switch to a temp file once RAM limit is exceeded. */
  if (fio_http_body_fd(h) != -1) {
    fio_http_body_seek(h, 0);
    fio_str_info_s r = fio_http_body_read(h, 16);
    FIO_ASSERT(r.len == 16, "file-spill read length");
    FIO_ASSERT(!memcmp(r.buf, "xxxxxxxxxxxxxxxx", 16),
               "file-spill read content");
  }

  FIO_MEM_FREE(buf, chunk);
  fio_http_free(h);
}

/* ===========================================================================
   Cookies
   ===========================================================================
 */

static int test_cookie_each_callback(fio_http_s *h,
                                     fio_str_info_s name,
                                     fio_str_info_s value,
                                     void *udata) {
  (void)h;
  size_t *count = (size_t *)udata;
  (*count)++;
  if (name.len == 3 && !memcmp(name.buf, "sid", 3)) {
    FIO_ASSERT(value.len == 6 && !memcmp(value.buf, "abc123", 6),
               "cookie each value mismatch");
  }
  return 0;
}

static void test_handle_cookies(void) {
  fprintf(stderr, "  * cookies\n");
  fio_http_s *h = fio_http_new();

  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"cookie", 6),
      FIO_STR_INFO1((char *)"sid=abc123; theme=dark"));

  fio_str_info_s sid = fio_http_cookie(h, "sid", 3);
  FIO_ASSERT(sid.len == 6 && !memcmp(sid.buf, "abc123", 6),
             "cookie lookup mismatch");

  size_t count = 0;
  fio_http_cookie_each(h, test_cookie_each_callback, &count);
  FIO_ASSERT(count == 2, "cookie each count mismatch");

  int set_r = fio_http_cookie_set(
      h,
      .name = FIO_STR_INFO1((char *)"session"),
      .value = FIO_STR_INFO1((char *)"xyz"),
      .max_age = 3600,
      .http_only = 1,
      .same_site = FIO_HTTP_COOKIE_SAME_SITE_STRICT);
  FIO_ASSERT(set_r == 0, "cookie set should succeed");

  fio_str_info_s session = fio_http_cookie(h, "session", 7);
  FIO_ASSERT(session.len == 3 && !memcmp(session.buf, "xyz", 3),
             "set cookie lookup mismatch");

  fio_http_free(h);
}

/* ===========================================================================
   Path sections
   ===========================================================================
 */

static void test_handle_path_sections(void) {
  fprintf(stderr, "  * path sections\n");
  fio_http_s *h = fio_http_new();
  fio_http_path_set(h, FIO_STR_INFO1((char *)"/foo/bar%20baz/qux"));

  fio_str_info_s path = fio_http_path(h);
  size_t count = 0;
  FIO_HTTP_PATH_EACH(path, section) {
    switch (count++) {
    case 0:
      FIO_ASSERT(section.len == 3 && !memcmp(section.buf, "foo", 3),
                 "first path section mismatch");
      break;
    case 1:
      FIO_ASSERT(section.len == 7 && !memcmp(section.buf, "bar baz", 7),
                 "second path section should be percent-decoded");
      break;
    case 2:
      FIO_ASSERT(section.len == 3 && !memcmp(section.buf, "qux", 3),
                 "third path section mismatch");
      break;
    default:
      FIO_ASSERT(0, "unexpected path section");
    }
  }
  FIO_ASSERT(count == 3, "path section count mismatch");

  fio_http_free(h);
}

/* ===========================================================================
   Helpers
   ===========================================================================
 */

static void test_handle_helpers(void) {
  fprintf(stderr, "  * helper functions\n");
  fio_str_info_s str = fio_http_status2str(200);
  FIO_ASSERT(str.len && !memcmp(str.buf, "OK", 2), "status2str 200 mismatch");

  str = fio_http_status2str(404);
  FIO_ASSERT(str.len && !memcmp(str.buf, "Not Found", 9),
             "status2str 404 mismatch");

  str = fio_http_status2str(999);
  FIO_ASSERT(str.len, "status2str unknown should not be empty");

  str = fio_http_date(1700000000);
  FIO_ASSERT(str.len, "date should not be empty");

  str = fio_http_log_time(1700000000);
  FIO_ASSERT(str.len, "log_time should not be empty");

  fio_http_s *h = fio_http_new();
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"x-forwarded-for", 15),
      FIO_STR_INFO1((char *)"1.2.3.4"));
  char from_buf[64];
  fio_str_info_s from = FIO_STR_INFO3(from_buf, 0, sizeof(from_buf) - 1);
  int from_r = fio_http_from(&from, h);
  FIO_ASSERT(from_r == 0, "from with forwarded header should succeed");
  FIO_ASSERT(from.len == 7 && !memcmp(from.buf, "1.2.3.4", 7),
             "from forwarded value mismatch");
  fio_http_free(h);
}

/* ===========================================================================
   WebSocket / SSE request detection
   ===========================================================================
 */

static void test_handle_websocket_request(void) {
  fprintf(stderr, "  * WebSocket request detection\n");
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(!fio_http_websocket_requested(h),
             "empty handle should not request websocket");

  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"connection", 10),
      FIO_STR_INFO1((char *)"Upgrade"));
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"upgrade", 7),
      FIO_STR_INFO1((char *)"websocket"));
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"sec-websocket-key", 17),
      FIO_STR_INFO1((char *)"dGhlIHNhbXBsZSBub25jZQ=="));
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"sec-websocket-version", 21),
      FIO_STR_INFO1((char *)"13"));

  FIO_ASSERT(fio_http_websocket_requested(h),
             "valid websocket headers should be detected");
  fio_http_free(h);
}

static void test_handle_sse_request(void) {
  fprintf(stderr, "  * SSE request detection\n");
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(!fio_http_sse_requested(h),
             "empty handle should not request sse");

  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"accept", 6),
      FIO_STR_INFO1((char *)"text/event-stream"));
  FIO_ASSERT(fio_http_sse_requested(h), "SSE request should be detected");
  fio_http_free(h);
}

/* ===========================================================================
   Body parsing helpers and tests (ported from tests-old/http-handle.c)
   ===========================================================================
 */

#define TEST_BODY_MAX_PAIRS 32
#define TEST_BODY_STR_MAX   256

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

typedef struct test_obj_s {
  test_obj_type_e type;
  union {
    struct {
      size_t len;
      char buf[1];
    } str;
    int64_t num;
    double flt;
  } u;
} test_obj_s;

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
  char pending_file_name[TEST_BODY_STR_MAX];
  char pending_file_filename[TEST_BODY_STR_MAX];
  char pending_file_ct[TEST_BODY_STR_MAX];
  char pending_file_data[TEST_BODY_STR_MAX];
  size_t pending_file_data_len;
} test_body_ctx_s;

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

static test_obj_s *test_obj_num_new(int64_t num) {
  test_obj_s *o = (test_obj_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(test_obj_s), 0);
  if (!o)
    return NULL;
  o->type = TEST_OBJ_NUMBER;
  o->u.num = num;
  return o;
}

static test_obj_s *test_obj_float_new(double flt) {
  test_obj_s *o = (test_obj_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(test_obj_s), 0);
  if (!o)
    return NULL;
  o->type = TEST_OBJ_FLOAT;
  o->u.flt = flt;
  return o;
}

static test_obj_s *test_obj_sentinel_new(test_obj_type_e type) {
  test_obj_s *o = (test_obj_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(test_obj_s), 0);
  if (!o)
    return NULL;
  o->type = type;
  return o;
}

static void test_obj_free(test_obj_s *o) {
  if (!o)
    return;
  FIO_MEM_FREE(o,
               sizeof(test_obj_s) +
                   (o->type == TEST_OBJ_STRING ? o->u.str.len + 1 : 0));
}

static void test_body_strcpy(char *dst,
                             size_t dst_size,
                             const void *src,
                             size_t src_len) {
  size_t n = src_len < dst_size - 1 ? src_len : dst_size - 1;
  FIO_MEMCPY(dst, src, n);
  dst[n] = '\0';
}

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

static test_body_pair_s *test_body_find(test_body_ctx_s *ctx, const char *key) {
  for (size_t i = 0; i < ctx->count; ++i) {
    if (!strcmp(ctx->pairs[i].key, key))
      return &ctx->pairs[i];
  }
  return NULL;
}

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

static void test_body_free_result(fio_http_body_parse_result_s *r) {
  test_obj_free((test_obj_s *)r->result);
  r->result = NULL;
}

/* ---- URL-encoded tests ---- */

static void test_body_parse_urlencoded_simple(void) {
  fprintf(stderr, "  * body_parse urlencoded simple\n");
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
             "urlencoded simple: key1 value mismatch");
  test_body_pair_s *p2 = test_body_find(&ctx, "key2");
  FIO_ASSERT(p2, "urlencoded simple: key2 not found");
  FIO_ASSERT(p2 && strcmp(p2->val, "val2") == 0,
             "urlencoded simple: key2 value mismatch");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_urlencoded_encoded(void) {
  fprintf(stderr, "  * body_parse urlencoded percent/plus encoding\n");
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
  FIO_ASSERT(pname && strcmp(pname->val, "hello world") == 0,
             "urlencoded encoded: 'name' value mismatch");
  test_body_pair_s *pcity = test_body_find(&ctx, "city");
  FIO_ASSERT(pcity && strcmp(pcity->val, "New York") == 0,
             "urlencoded encoded: 'city' value mismatch");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_urlencoded_empty_value(void) {
  fprintf(stderr, "  * body_parse urlencoded empty value\n");
  const char *body = "key=&other=val";
  fio_http_s *h = test_body_make_handle("application/x-www-form-urlencoded",
                                        body,
                                        strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err, "urlencoded empty value: parse should succeed");
  test_body_pair_s *pk = test_body_find(&ctx, "key");
  FIO_ASSERT(pk && pk->val[0] == '\0',
             "urlencoded empty value: 'key' should have empty value");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_urlencoded_multi_value(void) {
  fprintf(stderr, "  * body_parse urlencoded multiple values same key\n");
  const char *body = "color=red&color=blue";
  fio_http_s *h = test_body_make_handle("application/x-www-form-urlencoded",
                                        body,
                                        strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err, "urlencoded multi-value: parse should succeed");
  size_t color_count = 0;
  for (size_t i = 0; i < ctx.count; ++i) {
    if (!strcmp(ctx.pairs[i].key, "color"))
      ++color_count;
  }
  FIO_ASSERT(color_count == 2,
             "urlencoded multi-value: expected 2 'color' entries, got %zu",
             color_count);
  test_body_free_result(&r);
  fio_http_free(h);
}

/* ---- JSON tests ---- */

static void test_body_parse_json_simple(void) {
  fprintf(stderr, "  * body_parse JSON simple object\n");
  const char *body = "{\"key\": \"value\", \"num\": 42}";
  fio_http_s *h = test_body_make_handle("application/json", body, strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err, "JSON simple: parse should succeed (err=%d)", r.err);
  FIO_ASSERT(ctx.got_map, "JSON simple: should create a map");
  test_body_pair_s *pk = test_body_find(&ctx, "key");
  FIO_ASSERT(pk && strcmp(pk->val, "value") == 0,
             "JSON simple: 'key' value mismatch");
  FIO_ASSERT(ctx.last_number == 42,
             "JSON simple: expected last_number=42, got %" PRId64,
             ctx.last_number);
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_json_array(void) {
  fprintf(stderr, "  * body_parse JSON array value\n");
  const char *body = "[\"a\", \"b\", \"c\"]";
  fio_http_s *h = test_body_make_handle("application/json", body, strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(!r.err, "JSON array: parse should succeed");
  FIO_ASSERT(ctx.got_array, "JSON array: should create an array");
  test_body_free_result(&r);
  fio_http_free(h);
}

/* ---- Multipart tests ---- */

static size_t test_body_build_multipart(char *buf,
                                        size_t buf_size,
                                        const char *boundary,
                                        ...) {
  size_t pos = 0;
  va_list ap;
  va_start(ap, boundary);
  for (;;) {
    const char *name = va_arg(ap, const char *);
    if (!name)
      break;
    const char *value = va_arg(ap, const char *);
    const char *filename = va_arg(ap, const char *);
    const char *ct = va_arg(ap, const char *);

#define MP_APPEND(s)                                                           \
  do {                                                                         \
    size_t _l = strlen(s);                                                     \
    if (pos + _l < buf_size) {                                                \
      FIO_MEMCPY(buf + pos, s, _l);                                            \
      pos += _l;                                                               \
    }                                                                          \
  } while (0)

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
  fprintf(stderr, "  * body_parse multipart simple text field\n");
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
  FIO_ASSERT(pf && strcmp(pf->val, "hello") == 0,
             "multipart simple field: 'field1' value mismatch");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_multipart_file_upload(void) {
  fprintf(stderr, "  * body_parse multipart file upload\n");
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
  test_body_pair_s *pf = NULL;
  for (size_t i = 0; i < ctx.count; ++i) {
    if (ctx.pairs[i].is_file && !strcmp(ctx.pairs[i].key, "upload")) {
      pf = &ctx.pairs[i];
      break;
    }
  }
  FIO_ASSERT(pf, "multipart file upload: 'upload' file entry not found");
  FIO_ASSERT(pf && strcmp(pf->filename, "test.txt") == 0,
             "multipart file upload: filename mismatch");
  FIO_ASSERT(pf && strcmp(pf->val, "file content here") == 0,
             "multipart file upload: file data mismatch");
  test_body_free_result(&r);
  fio_http_free(h);
}

/* ---- Edge cases ---- */

static void test_body_parse_empty_body(void) {
  fprintf(stderr, "  * body_parse empty body\n");
  fio_http_s *h = fio_http_new();
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"content-type", 12),
      FIO_STR_INFO1((char *)"application/x-www-form-urlencoded"));
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(ctx.count == 0,
             "empty body: should have 0 pairs (got %zu)",
             ctx.count);
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_no_content_type(void) {
  fprintf(stderr, "  * body_parse no Content-Type\n");
  fio_http_s *h = fio_http_new();
  fio_http_body_write(h, "key=val", 7);
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(r.err != 0,
             "no content-type: should return error for unknown type");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_unknown_content_type(void) {
  fprintf(stderr, "  * body_parse unknown Content-Type\n");
  fio_http_s *h = test_body_make_handle("text/plain", "hello world", 11);
  test_body_ctx_s ctx = {0};
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_BODY_CALLBACKS, &ctx);
  FIO_ASSERT(r.err != 0,
             "unknown content-type: should return error, not crash");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_null_callbacks(void) {
  fprintf(stderr, "  * body_parse NULL callbacks\n");
  fio_http_s *h = test_body_make_handle("application/json", "{\"k\":1}", 7);
  fio_http_body_parse_result_s r = fio_http_body_parse(h, NULL, NULL);
  FIO_ASSERT(r.err != 0, "null callbacks: should return error");
  fio_http_free(h);
}

static void test_body_parse_null_handle(void) {
  fprintf(stderr, "  * body_parse NULL handle\n");
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
  fprintf(stderr, "Testing fio_http_handle correctness:\n");

  test_handle_lifecycle();
  test_handle_udata();
  test_handle_status();
  test_handle_request_line();
  test_handle_copy_request();
  test_handle_request_headers();
  test_handle_response_headers();
  test_handle_body();
  test_handle_body_file_spill();
  test_handle_cookies();
  test_handle_path_sections();
  test_handle_helpers();
  test_handle_websocket_request();
  test_handle_sse_request();

  fprintf(stderr, "\nTesting fio_http_body_parse correctness:\n");
  test_body_parse_urlencoded_simple();
  test_body_parse_urlencoded_encoded();
  test_body_parse_urlencoded_empty_value();
  test_body_parse_urlencoded_multi_value();
  test_body_parse_json_simple();
  test_body_parse_json_array();
  test_body_parse_multipart_simple_field();
  test_body_parse_multipart_file_upload();
  test_body_parse_empty_body();
  test_body_parse_no_content_type();
  test_body_parse_unknown_content_type();
  test_body_parse_null_callbacks();
  test_body_parse_null_handle();

  fprintf(stderr, "\nAll HTTP handle tests passed!\n");
  return 0;
}
