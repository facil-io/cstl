/* *****************************************************************************
Test: HTTP handle correctness
***************************************************************************** */
#define FIO_HTTP
#define FIO_HTTP_TEST_COMPRESSION_FLAGS
#define FIO_DATA_NO_MMAP /* exercise read failures through the heap fallback */
#ifndef _WIN32
#define TEST_FD_HOOKS
#endif
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
  FIO_ASSERT((uintptr_t)fio_http_udata(h) == 0xAA, "udata roundtrip error");

  FIO_ASSERT(!fio_http_udata2(h), "udata2 should start as NULL");
  fio_http_udata2_set(h, (void *)(uintptr_t)0xBB);
  FIO_ASSERT((uintptr_t)fio_http_udata2(h) == 0xBB, "udata2 roundtrip error");

  FIO_ASSERT(!fio_http_cdata(h), "cdata should start as NULL");
  fio_http_cdata_set(h, (void *)(uintptr_t)0xCC);
  FIO_ASSERT((uintptr_t)fio_http_cdata(h) == 0xCC, "cdata roundtrip error");

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
  FIO_ASSERT(
      FIO_STR_INFO_IS_EQ(fio_http_method(h), FIO_STR_INFO1((char *)"POST")),
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
  FIO_ASSERT(
      FIO_STR_INFO_IS_EQ(fio_http_query(h), FIO_STR_INFO1((char *)"a=1&b=2")),
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
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"x-custom", 8),
                              FIO_STR_INFO1((char *)"value"));

  fio_http_s *copy = fio_http_new_copy_request(h);
  FIO_ASSERT(
      FIO_STR_INFO_IS_EQ(fio_http_method(copy), FIO_STR_INFO1((char *)"GET")),
      "copy method mismatch");
  FIO_ASSERT(
      FIO_STR_INFO_IS_EQ(fio_http_path(copy), FIO_STR_INFO1((char *)"/api")),
      "copy path mismatch");
  FIO_ASSERT(
      FIO_STR_INFO_IS_EQ(fio_http_query(copy), FIO_STR_INFO1((char *)"x=y")),
      "copy query mismatch");
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(
                 fio_http_request_header(copy,
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
  FIO_ASSERT(
      fio_http_request_header_each(h, test_header_each_callback, &count) == 1,
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

/* The same validation/removal contract applies to all six public setters. */
typedef struct {
  const char *category;
  fio_str_info_s (*setters[3])(fio_http_s *, fio_str_info_s, fio_str_info_s);
  fio_str_info_s (*get)(fio_http_s *, fio_str_info_s, size_t);
  size_t (*count)(fio_http_s *, fio_str_info_s);
  size_t (*each)(fio_http_s *,
                 int (*)(fio_http_s *, fio_str_info_s, fio_str_info_s, void *),
                 void *);
} test_header_api_s;

static const test_header_api_s TEST_HEADER_APIS[] = {
    {"request",
     {fio_http_request_header_set,
      fio_http_request_header_add,
      fio_http_request_header_set_if_missing},
     fio_http_request_header,
     fio_http_request_header_count,
     fio_http_request_header_each},
    {"response",
     {fio_http_response_header_set,
      fio_http_response_header_add,
      fio_http_response_header_set_if_missing},
     fio_http_response_header,
     fio_http_response_header_count,
     fio_http_response_header_each},
};

static void test_header_assert_preserved(fio_http_s *h,
                                         const test_header_api_s *api) {
  fio_str_info_s name = FIO_STR_INFO2((char *)"x-test", 6);
  FIO_ASSERT(api->count(h, FIO_STR_INFO2(NULL, 0)) == 1,
             "%s rejection changed the number of headers",
             api->category);
  FIO_ASSERT(api->count(h, name) == 2,
             "%s rejection changed existing value count",
             api->category);
  FIO_ASSERT(
      FIO_STR_INFO_IS_EQ(api->get(h, name, 0), FIO_STR_INFO1((char *)"first")),
      "%s rejection changed the first value",
      api->category);
  FIO_ASSERT(
      FIO_STR_INFO_IS_EQ(api->get(h, name, 1), FIO_STR_INFO1((char *)"second")),
      "%s rejection changed the second value",
      api->category);
}

static void test_handle_header_rejection(void) {
  fprintf(stderr, "  * request/response header name and value rejection\n");
  /* Independent of the production table: controls, SP, DEL and separators. */
  static const char forbidden[] =
      "\0\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
      " \x7f\"(),/:;<=>?@[\\]{}";
  fio_str_info_s invalid_values[] = {
      FIO_STR_INFO2((char *)"\0ok", 3),
      FIO_STR_INFO2((char *)"o\0k", 3),
      FIO_STR_INFO2((char *)"ok\0", 3),
      FIO_STR_INFO2((char *)"\rok", 3),
      FIO_STR_INFO2((char *)"o\rk", 3),
      FIO_STR_INFO2((char *)"ok\r", 3),
      FIO_STR_INFO2((char *)"\nok", 3),
      FIO_STR_INFO2((char *)"o\nk", 3),
      FIO_STR_INFO2((char *)"ok\n", 3),
      FIO_STR_INFO1((char *)"good\r\nInjected: yes"),
  };
  fio_str_info_s name = FIO_STR_INFO2((char *)"x-test", 6);
  fio_str_info_s missing = FIO_STR_INFO1((char *)"x-missing");
  fio_str_info_s good = FIO_STR_INFO1((char *)"value");

  for (size_t a = 0; a < sizeof(TEST_HEADER_APIS) / sizeof(*TEST_HEADER_APIS);
       ++a) {
    const test_header_api_s *api = &TEST_HEADER_APIS[a];
    fio_http_s *h = fio_http_new();
    FIO_ASSERT(h, "fio_http_new returned NULL");
    api->setters[1](h, name, FIO_STR_INFO1((char *)"first"));
    api->setters[1](h, name, FIO_STR_INFO1((char *)"second"));
    for (size_t op = 0; op < 3; ++op) {
      for (size_t i = 0; i < sizeof(forbidden) - 1; ++i) {
        /* Explicit length catches embedded NUL truncation to existing x-test.
         */
        char bad_name[] = {'x', '-', 't', 'e', 's', 't', forbidden[i], 'x'};
        fio_str_info_s r =
            api->setters[op](h,
                             FIO_STR_INFO2(bad_name, sizeof(bad_name)),
                             good);
        FIO_ASSERT(!r.buf && !r.len,
                   "%s setter %zu accepted forbidden name byte 0x%02x",
                   api->category,
                   op,
                   (unsigned)(uint8_t)forbidden[i]);
        test_header_assert_preserved(h, api);
        r = api->setters[op](h,
                             FIO_STR_INFO2(bad_name, sizeof(bad_name)),
                             FIO_STR_INFO2(NULL, 0));
        FIO_ASSERT(!r.buf && !r.len,
                   "%s setter %zu accepted invalid removal name",
                   api->category,
                   op);
        test_header_assert_preserved(h, api);
      }
      for (size_t i = 0; i < sizeof(invalid_values) / sizeof(*invalid_values);
           ++i) {
        fio_str_info_s r = api->setters[op](h, name, invalid_values[i]);
        FIO_ASSERT(!r.buf && !r.len,
                   "%s setter %zu accepted invalid replacement %zu",
                   api->category,
                   op,
                   i);
        test_header_assert_preserved(h, api);
        r = api->setters[op](h, missing, invalid_values[i]);
        FIO_ASSERT(!r.buf && !r.len && !api->get(h, missing, 0).buf,
                   "%s setter %zu stored invalid value %zu on missing header",
                   api->category,
                   op,
                   i);
        test_header_assert_preserved(h, api);
      }
    }
    fio_http_free(h);
  }
}

static int test_header_name_callback(fio_http_s *h,
                                     fio_str_info_s name,
                                     fio_str_info_s value,
                                     void *udata) {
  (void)h;
  (void)value;
  FIO_ASSERT(FIO_STR_INFO_IS_EQ(name, *(fio_str_info_s *)udata),
             "stored header name does not match lowercase policy");
  return 0;
}

static void test_handle_header_name_policy(void) {
  fprintf(stderr, "  * header names, ASCII lowercase and immutable input\n");
  static const char *const names[] = {
      "X-Mixed",
      ("X-Token!#$%&'*+-.^_`|~"
       "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"),
      "X-\xc3\x89-\x80\xff",
  };
#if FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
  static const char *const stored_names[] = {
      "x-mixed",
      ("x-token!#$%&'*+-.^_`|~"
       "0123456789abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"),
      "x-\xc3\x89-\x80\xff",
  };
#endif
  /* High-bit name bytes remain allowed, including non-UTF-8 bytes. */
  fio_str_info_s value = FIO_STR_INFO1((char *)"text \t\xc3\xa9\x80\xff");
  for (size_t a = 0; a < sizeof(TEST_HEADER_APIS) / sizeof(*TEST_HEADER_APIS);
       ++a) {
    const test_header_api_s *api = &TEST_HEADER_APIS[a];
    for (size_t op = 0; op < 3; ++op) {
      for (size_t i = 0; i < sizeof(names) / sizeof(*names); ++i) {
        fio_http_s *h = fio_http_new();
        FIO_ASSERT(h, "fio_http_new returned NULL");
        fio_str_info_s name = FIO_STR_INFO1((char *)names[i]);
#if FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
        fio_str_info_s stored = FIO_STR_INFO1((char *)stored_names[i]);
#else
        fio_str_info_s stored = name;
#endif
        /* A lowercase write into these literals would fault. */
        fio_str_info_s r = api->setters[op](h, name, value);
        FIO_ASSERT(FIO_STR_INFO_IS_EQ(r, value),
                   "%s setter %zu rejected valid name %zu",
                   api->category,
                   op,
                   i);
        FIO_ASSERT(FIO_STR_INFO_IS_EQ(api->get(h, name, 0), value) &&
                       FIO_STR_INFO_IS_EQ(api->get(h, stored, 0), value),
                   "%s valid header lookup mismatch",
                   api->category);
        FIO_ASSERT(api->count(h, stored) == 1 &&
                       api->each(h, test_header_name_callback, &stored) == 1,
                   "%s valid header storage mismatch",
                   api->category);
#if !FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
        FIO_ASSERT(!api->get(h, FIO_STR_INFO1((char *)"x-mixed"), 0).buf,
                   "disabled lowercase mode must preserve original case");
#endif
        char mutable_name[] = "X-Mutable";
        api->setters[op](h,
                         FIO_STR_INFO2(mutable_name, sizeof(mutable_name) - 1),
                         value);
        FIO_ASSERT(!memcmp(mutable_name, "X-Mutable", sizeof(mutable_name)),
                   "%s setter %zu modified input name",
                   api->category,
                   op);
        FIO_ASSERT(!memcmp(value.buf, "text \t\xc3\xa9\x80\xff", value.len),
                   "%s setter %zu modified input value",
                   api->category,
                   op);
        fio_http_free(h);
      }
    }
  }
}

static void test_handle_header_name_length(void) {
  fprintf(stderr, "  * 4095/4096 byte header name boundary\n");
  char name_buf[4096];
  FIO_MEMSET(name_buf, 'A', sizeof(name_buf));
  fio_str_info_s name = FIO_STR_INFO2(name_buf, 4095);
  fio_str_info_s too_long = FIO_STR_INFO2(name_buf, sizeof(name_buf));
  fio_str_info_s value = FIO_STR_INFO1((char *)"boundary");
  char stored_buf[4095];
#if FIO_HTTP_ENFORCE_LOWERCASE_HEADERS
  FIO_MEMSET(stored_buf, 'a', sizeof(stored_buf));
#else
  FIO_MEMSET(stored_buf, 'A', sizeof(stored_buf));
#endif
  fio_str_info_s stored = FIO_STR_INFO2(stored_buf, sizeof(stored_buf));
  for (size_t a = 0; a < sizeof(TEST_HEADER_APIS) / sizeof(*TEST_HEADER_APIS);
       ++a) {
    const test_header_api_s *api = &TEST_HEADER_APIS[a];
    for (size_t op = 0; op < 3; ++op) {
      fio_http_s *h = fio_http_new();
      FIO_ASSERT(h, "fio_http_new returned NULL");
      fio_str_info_s r = api->setters[op](h, name, value);
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(r, value),
                 "%s setter %zu rejected a 4095 byte name",
                 api->category,
                 op);
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(api->get(h, name, 0), value) &&
                     FIO_STR_INFO_IS_EQ(api->get(h, stored, 0), value),
                 "%s 4095 byte name lookup failed",
                 api->category);
      FIO_ASSERT(api->each(h, test_header_name_callback, &stored) == 1,
                 "%s 4095 byte name storage failed",
                 api->category);
      r = api->setters[op](h, too_long, FIO_STR_INFO1((char *)"replacement"));
      FIO_ASSERT(!r.buf && !r.len && !api->get(h, too_long, 0).buf,
                 "%s setter %zu accepted/looked up a 4096 byte name",
                 api->category,
                 op);
      size_t unique_count = api->count(h, FIO_STR_INFO2(NULL, 0));
      size_t stored_count = api->count(h, stored);
      fio_str_info_s original_value = api->get(h, name, 0);
      fio_str_info_s stored_value = api->get(h, stored, 0);
      FIO_ASSERT(unique_count == 1,
                 "%s setter %zu oversized name changed unique count to %zu",
                 api->category,
                 op,
                 unique_count);
      FIO_ASSERT(
          FIO_STR_INFO_IS_EQ(original_value, value),
          "%s setter %zu oversized name changed original lookup: %zu '%.*s'",
          api->category,
          op,
          original_value.len,
          (int)original_value.len,
          original_value.buf ? original_value.buf : "");
      FIO_ASSERT(
          FIO_STR_INFO_IS_EQ(stored_value, value),
          "%s setter %zu oversized name changed stored lookup: %zu '%.*s'",
          api->category,
          op,
          stored_value.len,
          (int)stored_value.len,
          stored_value.buf ? stored_value.buf : "");
      FIO_ASSERT(
          stored_count == 1,
          "%s setter %zu 4095 byte stored-name count is %zu (expected 1)",
          api->category,
          op,
          stored_count);
      for (size_t i = 0; i < sizeof(name_buf); ++i)
        FIO_ASSERT(name_buf[i] == 'A',
                   "header setter modified long input name");
      fio_http_free(h);
    }
  }
}

static void test_handle_header_removal(void) {
  fprintf(stderr, "  * null/empty header inputs: only set removes\n");
  fio_str_info_s name = FIO_STR_INFO2((char *)"x-test", 6);
  fio_str_info_s invalid_names[] = {
      FIO_STR_INFO2(NULL, 0),
      FIO_STR_INFO2(NULL, 5),
      FIO_STR_INFO2((char *)"", 0),
  };
  fio_str_info_s empty_values[] = {
      FIO_STR_INFO2(NULL, 0),
      FIO_STR_INFO2(NULL, 5),
      FIO_STR_INFO2((char *)"\r\n", 0),
  };
  for (size_t a = 0; a < sizeof(TEST_HEADER_APIS) / sizeof(*TEST_HEADER_APIS);
       ++a) {
    const test_header_api_s *api = &TEST_HEADER_APIS[a];
    for (size_t op = 0; op < 3; ++op) {
      for (size_t i = 0; i < sizeof(empty_values) / sizeof(*empty_values);
           ++i) {
        fio_http_s *h = fio_http_new();
        FIO_ASSERT(h, "fio_http_new returned NULL");
        api->setters[1](h, name, FIO_STR_INFO1((char *)"first"));
        api->setters[1](h, name, FIO_STR_INFO1((char *)"second"));
        for (size_t n = 0; n < sizeof(invalid_names) / sizeof(*invalid_names);
             ++n) {
          fio_str_info_s r = api->setters[op](h,
                                              invalid_names[n],
                                              FIO_STR_INFO1((char *)"value"));
          FIO_ASSERT(!r.buf && !r.len, "invalid empty/null name was accepted");
          test_header_assert_preserved(h, api);
          r = api->setters[op](h, invalid_names[n], empty_values[i]);
          FIO_ASSERT(!r.buf && !r.len,
                     "invalid empty/null removal name accepted");
          test_header_assert_preserved(h, api);
        }
        fio_str_info_s r = api->setters[op](h, name, empty_values[i]);
        if (op == 2) { /* set_if_missing: no-op, returns the existing value */
          FIO_ASSERT(r.len == 6 && !memcmp(r.buf, "second", 6),
                     "%s set_if_missing should return the existing value",
                     api->category);
        } else {
          FIO_ASSERT(!r.buf && !r.len, "empty/null value should return empty");
        }
        if (op) { /* add / set_if_missing: no-op, not removal */
          test_header_assert_preserved(h, api);
          api->setters[0](h, name, FIO_STR_INFO2(NULL, 0));
        } else {
          FIO_ASSERT(!api->get(h, name, 0).buf && api->count(h, name) == 0,
                     "%s setter %zu did not remove header",
                     api->category,
                     op);
        }
        r = api->setters[op](h, name, empty_values[i]);
        FIO_ASSERT(!r.buf && !r.len &&
                       api->count(h, FIO_STR_INFO2(NULL, 0)) == 0,
                   "%s setter %zu created header from empty/null value",
                   api->category,
                   op);
        fio_http_free(h);
      }
    }
  }
}

static void test_handle_should_close(void) {
  fprintf(stderr, "  * request Connection close tokens\n");
  static const struct {
    const char *value;
    int close;
  } cases[] = {
      {"", 0},
      {"keep-alive", 0},
      {"close", 1},
      {"CLOSE", 1},
      {"Close", 1},
      {"closE", 1},
      {"cLoSe", 1},
      {" \tClOsE\t ", 1},
      {"keep-alive, close", 1},
      {"close, keep-alive", 1},
      {", ,\t, ", 0},
      {", ,\tclose\t ,,", 1},
      {"keep-alive,,\tClOsE \t,,upgrade", 1},
      {"xclose", 0},
      {"close-extra", 0},
      {"close;q=0", 0},
      {"close ;q=0", 0},
      {"closed", 0},
      {"clos", 0},
      {"cl ose", 0},
      {"\vclose", 0},
      {"close\f", 0},
      {"xclose, close-extra, close;q=0", 0},
  };
  fio_str_info_s name = FIO_STR_INFO2((char *)"connection", 10);
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(h, "fio_http_new returned NULL");
  FIO_ASSERT(!fio___http1_should_close(h),
             "absent Connection should not close");
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    fio_str_info_s value = FIO_STR_INFO1((char *)cases[i].value);
    fio_http_request_header_set(h, name, value);
    if (value.len)
      FIO_ASSERT(FIO_STR_INFO_IS_EQ(fio_http_request_header(h, name, 0), value),
                 "Connection case %zu was not stored intact",
                 i);
    FIO_ASSERT(!!fio___http1_should_close(h) == cases[i].close,
               "Connection case %zu ('%s') close mismatch",
               i,
               cases[i].value);
  }
  fio_http_free(h);

  /* Check every stored value, including a close in the first/middle/last field.
   */
  for (size_t close_at = 0; close_at < 3; ++close_at) {
    h = fio_http_new();
    FIO_ASSERT(h, "fio_http_new returned NULL");
    for (size_t i = 0; i < 3; ++i)
      fio_http_request_header_add(
          h,
          name,
          FIO_STR_INFO1(
              (char *)(i == close_at ? " \tClOsE\t " : "keep-alive,xclose")));
    FIO_ASSERT(fio_http_request_header_count(h, name) == 3,
               "repeated Connection fields not stored");
    FIO_ASSERT(fio___http1_should_close(h),
               "close in repeated Connection field %zu not detected",
               close_at);
    fio_http_free(h);
  }

  h = fio_http_new();
  FIO_ASSERT(h, "fio_http_new returned NULL");
  fio_http_request_header_add(h, name, FIO_STR_INFO1((char *)"keep-alive"));
  fio_http_request_header_add(h, name, FIO_STR_INFO1((char *)", \t,,"));
  fio_http_request_header_add(
      h,
      name,
      FIO_STR_INFO1((char *)"xclose, close-extra, close;q=0"));
  FIO_ASSERT(!fio___http1_should_close(h),
             "repeated Connection nonmatches should not close");
  fio_http_free(h);

  h = fio_http_new();
  FIO_ASSERT(h, "fio_http_new returned NULL");
  fio_http_response_header_set(h, name, FIO_STR_INFO1((char *)"close"));
  FIO_ASSERT(!fio___http1_should_close(h),
             "response-only close must be ignored");
  fio_http_request_header_set(h, name, FIO_STR_INFO1((char *)"keep-alive"));
  FIO_ASSERT(!fio___http1_should_close(h),
             "response close must not override request keep-alive");
  fio_http_response_header_set(h, name, FIO_STR_INFO1((char *)"keep-alive"));
  fio_http_request_header_set(h, name, FIO_STR_INFO1((char *)"close"));
  FIO_ASSERT(fio___http1_should_close(h),
             "response keep-alive must not override request close");
  fio_http_free(h);
}

static void test_handle_should_close_http10(void) {
  fprintf(stderr, "  * HTTP/1.0 persistence (RFC 9112 §9.3)\n");
  static const struct {
    const char *version;
    const char *value; /* NULL: no Connection header */
    int streaming;
    int close;
  } cases[] = {
      {"HTTP/1.0", NULL, 0, 1},
      {"HTTP/1.0", "upgrade", 0, 1},
      {"HTTP/1.0", "keep-alive", 0, 0},
      {"HTTP/1.0", " \tKeEp-AlIvE\t ", 0, 0},
      {"HTTP/1.0", "upgrade, Keep-Alive", 0, 0},
      {"HTTP/1.0", "keep-alive, close", 0, 1},
      {"HTTP/1.0", "keep-alive-x", 0, 1},
      {"HTTP/1.0", "keep-alive", 1, 1}, /* close-delimited stream */
      {"HTTP/0.9", NULL, 0, 0},         /* not legacy: HTTP/1.1 semantics */
      {"HTTP/0.9", NULL, 1, 0},
      {"HTTP/1.1", NULL, 0, 0},
      {"HTTP/1.1", NULL, 1, 0},
      {"HTTP/1.1", "keep-alive", 1, 0},
  };
  fio_str_info_s name = FIO_STR_INFO2((char *)"connection", 10);
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    fio_http_s *h = fio_http_new();
    FIO_ASSERT(h, "fio_http_new returned NULL");
    fio_http_version_set(h, FIO_STR_INFO1((char *)cases[i].version));
    if (cases[i].value)
      fio_http_request_header_set(h,
                                  name,
                                  FIO_STR_INFO1((char *)cases[i].value));
    if (cases[i].streaming) {
      fio_http_status_set(h, 200);
      fio_http_write(h, .buf = "x", .len = 1, .copy = 1);
      FIO_ASSERT(fio_http_is_streaming(h), "unfinished write should stream");
    }
    FIO_ASSERT(!!fio___http1_should_close(h) == cases[i].close,
               "%s Connection \"%s\" streaming %d: close should be %d",
               cases[i].version,
               cases[i].value ? cases[i].value : "(none)",
               cases[i].streaming,
               cases[i].close);
    FIO_ASSERT(fio___http1_is_chunked(h) ==
                   (cases[i].streaming && strcmp(cases[i].version, "HTTP/1.0")),
               "%s streaming %d: chunked framing mismatch",
               cases[i].version,
               cases[i].streaming);
    fio_http_free(h);
  }
}

static void test_handle_should_close_long_list(void) {
  fprintf(stderr, "  * Connection close beyond 2048 bytes\n");
  char list[4096];
  size_t len = 0;
  for (size_t i = 0; i < 256; ++i) {
    FIO_MEMCPY(list + len, "keep-alive,", 11);
    len += 11;
  }
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(h, "fio_http_new returned NULL");
  fio_str_info_s name = FIO_STR_INFO2((char *)"connection", 10);
  fio_http_request_header_set(h, name, FIO_STR_INFO2(list, len));
  FIO_ASSERT(!fio___http1_should_close(h),
             "long list without close should stay open");
  static const char tail[] = " \tClOsE\t ,";
  FIO_MEMCPY(list + len, tail, sizeof(tail) - 1);
  len += sizeof(tail) - 1;
  fio_http_request_header_set(h, name, FIO_STR_INFO2(list, len));
  FIO_ASSERT(fio_http_request_header(h, name, 0).len == len && len > 2048,
             "long Connection value was not stored intact");
  FIO_ASSERT(fio___http1_should_close(h), "close beyond 2048 bytes was missed");
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

  FIO_ASSERT(fio_http_body_seek(h, -1) == 10 && fio_http_body_pos(h) == 10,
             "seek(-1) should be the last byte");
  r = fio_http_body_read(h, 6);
  FIO_ASSERT(r.len == 1 && r.buf[0] == 'd', "seek(-1) reads the last byte");
  FIO_ASSERT(fio_http_body_seek(h, 11) == 11 && fio_http_body_pos(h) == 11,
             "seek(length) should be EOF");
  FIO_ASSERT(!fio_http_body_read(h, 6).len, "read at EOF should be empty");
  FIO_ASSERT(fio_http_body_seek(h, SSIZE_MAX) == 11, "seek(SSIZE_MAX) is EOF");
  FIO_ASSERT(fio_http_body_seek(h, -11) == 0, "seek(-length) is 0");
  FIO_ASSERT(fio_http_body_seek(h, 3) == 3 && fio_http_body_seek(h, -12) == 0,
             "seek before the start clamps to 0");
  FIO_ASSERT(fio_http_body_seek(h, -100) == 0 && fio_http_body_pos(h) == 0,
             "negative seek past the start clamps to 0");
  FIO_ASSERT(fio_http_body_seek(h, 100) == 11, "seek past EOF clamps to EOF");
  FIO_ASSERT(fio_http_body_seek(h, -6) == 5, "seek(-6) position");
  r = fio_http_body_read(h, 6); /* -n == length - n */
  FIO_ASSERT(r.len == 6, "negative seek read length mismatch");
  FIO_ASSERT(!memcmp(r.buf, " World", 6),
             "negative seek read content mismatch");

  fio_http_body_seek(h, 0);
  r = fio_http_body_read_until(h, ' ', 0);
  FIO_ASSERT(r.len == 6, "read_until length mismatch");
  FIO_ASSERT(r.buf[r.len - 1] == ' ', "read_until token mismatch");
  FIO_ASSERT(fio_http_body_pos(h) == 6, "read_until position");

  fio_http_free(h);
}

#if FIO_OS_POSIX
static void test_handle_body_write_failure(void) {
  fprintf(stderr, "  * body write failure (no temporary folder) returns -1\n");
  fio_http_s *h = fio_http_new();
  size_t chunk = 4096;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, chunk, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMSET(buf, 'x', chunk);
  char *old = test_tmpdir_set("tmp/test-http-handle-no-such-folder/nope");
  size_t total = 0;
  int r = 0;
  while (!r && total < (FIO_HTTP_BODY_RAM_LIMIT << 1)) {
    r = fio_http_body_write(h, buf, chunk);
    total += (!r) * chunk;
  }
  test_tmpdir_restore(old);
  FIO_ASSERT(r == -1, "body write past the RAM limit should fail");
  FIO_ASSERT(fio_http_body_length(h) == total &&
                 total <= FIO_HTTP_BODY_RAM_LIMIT,
             "body length should be the stored data (%zu)",
             fio_http_body_length(h));
  FIO_ASSERT(fio_http_body_write(h, "x", 1) == -1 &&
                 fio_http_body_length(h) == total,
             "body errors are sticky");
  fio_http_body_close(h);
  FIO_ASSERT(!fio_http_body_write(h, "x", 1) && fio_http_body_length(h) == 1,
             "closing the body clears the error");
  FIO_MEM_FREE(buf, chunk);
  fio_http_free(h);
}
#endif

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

#if FIO_OS_POSIX
/* Saves the body (RAM and spilled) to a file; the body is unchanged. */
static void test_handle_body_save(void) {
  fprintf(stderr, "  * body save (RAM and spilled)\n");
  char dir[128], name[160];
  snprintf(dir, sizeof(dir), "tmp/test-http-body-save-%d", (int)getpid());
  snprintf(name, sizeof(name), "%s/body.bin", dir);
  fio_filename_remove(.path = dir, .recursive = 1);
  FIO_ASSERT(!fio_filename_make_path(.path = dir), "couldn't create %s", dir);
  const size_t len = (FIO_HTTP_BODY_RAM_LIMIT << 1) + 13;
  char *src = (char *)FIO_MEM_REALLOC(NULL, 0, len, 0);
  FIO_ASSERT_ALLOC(src);
  for (size_t i = 0; i < len; ++i)
    src[i] = (char)('a' + (i % 19));
  const size_t sizes[] = {100, len};
  for (size_t t = 0; t < 2; ++t) {
    fio_http_s *h = fio_http_new();
    FIO_ASSERT(!fio_http_body_write(h, src, sizes[t]), "body save: write");
    FIO_ASSERT((fio_http_body_fd(h) != -1) == (t == 1), "body save: mode");
    fio_http_body_seek(h, 7);
    FIO_ASSERT(!fio_http_body_save(h, name), "body save %zu failed", t);
    FIO_ASSERT(fio_http_body_pos(h) == 7 && fio_http_body_length(h) == sizes[t],
               "body save: body unchanged");
    char *f = fio_bstr_readfile(NULL, name, 0, 0);
    FIO_ASSERT(fio_bstr_len(f) == sizes[t] && !memcmp(f, src, sizes[t]),
               "body save %zu: file content (%zu bytes)",
               t,
               fio_bstr_len(f));
    fio_bstr_free(f);
    fio_str_info_s r = fio_http_body_read(h, 5);
    FIO_ASSERT(r.len == 5 && !memcmp(r.buf, src + 7, 5),
               "body save: body readable after save");
    FIO_ASSERT(test_dir_count(dir) == 1, "body save: sibling files left");
    fio_http_free(h);
  }
  FIO_MEM_FREE(src, len);
  fio_filename_remove(.path = dir, .recursive = 1);
}
#endif

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

  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"cookie", 6),
                              FIO_STR_INFO1((char *)"sid=abc123; theme=dark"));

  fio_str_info_s sid = fio_http_cookie(h, "sid", 3);
  FIO_ASSERT(sid.len == 6 && !memcmp(sid.buf, "abc123", 6),
             "cookie lookup mismatch");

  size_t count = 0;
  fio_http_cookie_each(h, test_cookie_each_callback, &count);
  FIO_ASSERT(count == 2, "cookie each count mismatch");

  int set_r =
      fio_http_cookie_set(h,
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
    default: FIO_ASSERT(0, "unexpected path section");
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
  fio_http_request_header_set(h,
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

static void test_handle_mimetype_length(void) {
  fprintf(stderr, "  * MIME extension length boundary\n");
  fio_str_info_s mime = fio_http_mimetype((char *)"html", 4);
  FIO_ASSERT(mime.len == 9 && !memcmp(mime.buf, "text/html", 9),
             "built-in html MIME type mismatch");

  char seven[7] = {'t', 'e', 's', 't', '7', 'x', 'y'};
  FIO_ASSERT(!fio_http_mimetype_register(
                 seven, sizeof(seven), FIO_STR_INFO1((char *)"text/x-test7")),
             "registering a 7-byte extension failed");
  mime = fio_http_mimetype(seven, sizeof(seven));
  FIO_ASSERT(mime.len == 12 && !memcmp(mime.buf, "text/x-test7", 12),
             "registered 7-byte extension did not resolve");

  char eight[8] = {'t', 'e', 's', 't', '7', 'x', 'y', 'z'};
  FIO_ASSERT(fio_http_mimetype_register(
                 eight, sizeof(eight), FIO_STR_INFO1((char *)"text/x-test8")) ==
                 -1,
             "registering an 8-byte extension should fail");
  mime = fio_http_mimetype(eight, sizeof(eight));
  FIO_ASSERT(!mime.buf && !mime.len, "8-byte extension must not resolve");

  /* Exact-size inputs: no terminating NUL or extra accessible input byte. */
  char nine[9], sixteen[16], long_ext[65], overflow_ext[512];
  FIO_MEMSET(nine, 'n', sizeof(nine));
  FIO_MEMSET(sixteen, 's', sizeof(sixteen));
  FIO_MEMSET(long_ext, 'l', sizeof(long_ext));
  FIO_MEMSET(overflow_ext, 'o', sizeof(overflow_ext));
  struct {
    char *buf;
    size_t len;
  } cases[] = {{nine, sizeof(nine)},
               {sixteen, sizeof(sixteen)},
               {long_ext, sizeof(long_ext)},
               {overflow_ext, sizeof(overflow_ext)}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    mime = fio_http_mimetype(cases[i].buf, cases[i].len);
    FIO_ASSERT(!mime.buf && !mime.len,
               "%zu-byte extension should return an empty MIME type",
               cases[i].len);
  }
  FIO_ASSERT(!fio_http_mimetype_register(seven, sizeof(seven), FIO_STR_INFO0),
             "removing the registered MIME type failed");
}

#if FIO_OS_POSIX
static void test_handle_static_long_mimetype(void) {
  fprintf(stderr, "  * static file with long MIME extension\n");
  char dir[128], path[192];
  snprintf(dir, sizeof(dir), "tmp/test-http-mime-%d", (int)getpid());
  snprintf(path, sizeof(path), "%s/notes.markdown_old", dir);
  fio_filename_remove(.path = dir, .recursive = 1);
  FIO_ASSERT(!fio_filename_make_path(.path = dir), "could not make %s", dir);
  FILE *f = fopen(path, "wb");
  FIO_ASSERT(f, "could not create %s", path);
  FIO_ASSERT(fwrite("mime", 1, 4, f) == 4 && !fclose(f),
             "could not write static MIME test file");
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(h, "could not create HTTP handle");
  /* HEAD executes MIME lookup and file checks without requiring a socket. */
  fio_http_method_set(h, FIO_STR_INFO1((char *)"HEAD"));
  FIO_ASSERT(!fio_http_static_file_response(
                 h,
                 FIO_STR_INFO1(dir),
                 FIO_STR_INFO1((char *)"/notes.markdown_old"),
                 0),
             "existing static file with long extension should be served");
  fio_str_info_s ct = fio_http_response_header(
      h, FIO_STR_INFO1((char *)"content-type"), 0);
  FIO_ASSERT(!ct.buf && !ct.len,
             "unknown long extension should not set Content-Type");
  fio_str_info_s cl = fio_http_response_header(
      h, FIO_STR_INFO1((char *)"content-length"), 0);
  FIO_ASSERT(cl.len == 1 && cl.buf[0] == '4',
             "HEAD response should expose the file length");
  fio_http_free(h);
  FIO_ASSERT(!fio_filename_remove(.path = dir, .recursive = 1),
             "could not remove MIME test directory");
}
#endif

/* ===========================================================================
   Header set_if_missing, ETag preconditions and WebSocket version errors
   ===========================================================================
 */

static void test_handle_set_if_missing_empty(void) {
  fprintf(stderr, "  * set_if_missing with an empty value\n");
  fio_http_s *h = fio_http_new();
  fio_str_info_s name = FIO_STR_INFO1((char *)"x-test");
  fio_str_info_s r;
  /* missing header: an empty value changes nothing and returns empty */
  r = fio_http_response_header_set_if_missing(h, name, FIO_STR_INFO0);
  FIO_ASSERT(!r.buf && !r.len, "empty set_if_missing on missing header");
  FIO_ASSERT(!fio_http_response_header_count(h, name),
             "empty set_if_missing should not create a header");
  /* existing header: kept, and the existing (last) value is returned */
  fio_http_response_header_add(h, name, FIO_STR_INFO1((char *)"one"));
  fio_http_response_header_add(h, name, FIO_STR_INFO1((char *)"two"));
  r = fio_http_response_header_set_if_missing(h, name, FIO_STR_INFO0);
  FIO_ASSERT(r.len == 3 && !memcmp(r.buf, "two", 3),
             "empty set_if_missing should return the existing value");
  r = fio_http_response_header_set_if_missing(h,
                                              name,
                                              FIO_STR_INFO1((char *)""));
  FIO_ASSERT(r.len == 3 && !memcmp(r.buf, "two", 3),
             "zero-length set_if_missing should return the existing value");
  FIO_ASSERT(fio_http_response_header_count(h, name) == 2,
             "empty set_if_missing must not remove the header");
  /* request twin behaves the same */
  fio_http_request_header_set(h, name, FIO_STR_INFO1((char *)"req"));
  r = fio_http_request_header_set_if_missing(h, name, FIO_STR_INFO0);
  FIO_ASSERT(r.len == 3 && !memcmp(r.buf, "req", 3) &&
                 fio_http_request_header_count(h, name) == 1,
             "request set_if_missing with empty value must keep the header");
  /* add ignores an empty value, set removes the header */
  r = fio_http_response_header_add(h, name, FIO_STR_INFO0);
  FIO_ASSERT(!r.len && fio_http_response_header_count(h, name) == 2,
             "empty add should be ignored");
  fio_http_response_header_set(h, name, FIO_STR_INFO0);
  FIO_ASSERT(!fio_http_response_header_count(h, name),
             "empty set should remove the header");
  fio_http_free(h);
}

/* Builds a GET handle with status 200, an optional response `etag` and the
 * listed `if-none-match` request header lines (NULL terminated). */
static fio_http_s *test_etag_handle(const char *method,
                                    size_t status,
                                    const char *etag,
                                    const char **inm) {
  fio_http_s *h = fio_http_new();
  fio_http_method_set(h, FIO_STR_INFO1((char *)method));
  fio_http_status_set(h, status);
  if (etag)
    fio_http_response_header_set(h,
                                 FIO_STR_INFO1((char *)"etag"),
                                 FIO_STR_INFO1((char *)etag));
  for (; inm && *inm; ++inm)
    fio_http_request_header_add(h,
                                FIO_STR_INFO1((char *)"if-none-match"),
                                FIO_STR_INFO1((char *)*inm));
  return h;
}

static void test_handle_etag_is_match(void) {
  fprintf(stderr, "  * ETag If-None-Match evaluation (RFC 9110)\n");
  struct {
    const char *method;
    size_t status;
    const char *etag;
    const char *inm[3];
    int expected;
    const char *msg;
  } cases[] = {
      {"GET", 200, "\"abc\"", {"\"abc\""}, 1, "exact strong match"},
      {"HEAD", 200, "\"abc\"", {"\"abc\""}, 1, "HEAD match"},
      {"get", 200, "\"abc\"", {"\"abc\""}, 1, "method is case-insensitive"},
      {"GET", 200, "\"abc\"", {"\"abcd\""}, 0, "longer tag (prefix) mismatch"},
      {"GET", 200, "abc", {"abcd"}, 0, "unquoted prefix mismatch"},
      {"GET", 200, "abc", {"abcd, abc"}, 1, "unquoted list match"},
      {"GET", 200, "abc", {"abc "}, 1, "trailing whitespace"},
      {"GET", 200, "\"abc\"", {"\"x\",\t \"abc\""}, 1, "list with OWS"},
      {"GET", 200, "\"abc\"", {"\"x\", \"y\""}, 0, "list without match"},
      {"GET", 200, "\"abc\"", {"W/\"abc\""}, 1, "weak request tag"},
      {"GET", 200, "W/\"abc\"", {"\"abc\""}, 1, "weak response tag"},
      {"GET", 200, "W/\"abc\"", {"W/\"abc\""}, 1, "weak on both sides"},
      {"GET", 200, "\"a,b\"", {"\"a,b\""}, 1, "quoted comma is one tag"},
      {"GET", 200, "\"b\"", {"\"a,b\""}, 0, "quoted comma is not a list"},
      {"GET", 200, "\"abc\"", {"\"x\"", "\"abc\""}, 1, "repeated header line"},
      {"GET", 200, "\"abc\"", {"\"abc"}, 0, "unterminated quoted tag"},
      {"GET", 200, "\"abc\"", {"\"x, \"abc\""}, 0, "unterminated stops line"},
      {"GET", 200, "\"abc\"", {"*"}, 1, "star with etag"},
      {"GET", 200, NULL, {"*"}, 1, "star without etag"},
      {"GET", 200, NULL, {" * "}, 1, "star with OWS"},
      {"GET", 200, NULL, {"*, \"abc\""}, 0, "star in a list is invalid"},
      {"GET", 200, "\"abc\"", {"*, \"abc\""}, 1, "valid tag beside star"},
      {"GET", 200, NULL, {"*", "\"abc\""}, 0, "star with another line"},
      {"GET", 200, NULL, {"\"abc\""}, 0, "no etag, no star"},
      {"GET", 206, "\"abc\"", {"\"abc\""}, 1, "any 2xx status"},
      {"GET", 404, "\"abc\"", {"\"abc\""}, 0, "error status ignored"},
      {"GET", 404, NULL, {"*"}, 0, "error status ignores star"},
      {"GET", 304, "\"abc\"", {"\"abc\""}, 0, "3xx status ignored"},
      {"POST", 200, "\"abc\"", {"\"abc\""}, 0, "POST never matches"},
      {"PUT", 200, NULL, {"*"}, 0, "PUT star is not evaluated"},
      {"GET", 200, "\"abc\"", {NULL}, 0, "no if-none-match header"},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    fio_http_s *h = test_etag_handle(cases[i].method,
                                     cases[i].status,
                                     cases[i].etag,
                                     cases[i].inm);
    FIO_ASSERT(fio_http_etag_is_match(h) == cases[i].expected,
               "fio_http_etag_is_match: %s (case %zu)",
               cases[i].msg,
               i);
    fio_http_free(h);
  }
  /* a fresh handle (status 0, e.g. a client request) is never evaluated */
  {
    const char *inm[] = {"*", NULL};
    fio_http_s *h = test_etag_handle("GET", 200, NULL, inm);
    fio_http_status_set(h, 0); /* normalizes to 200 */
    FIO_ASSERT(fio_http_etag_is_match(h) == 1, "status 0 normalizes to 200");
    fio_http_free(h);
    h = fio_http_new();
    fio_http_method_set(h, FIO_STR_INFO1((char *)"GET"));
    fio_http_request_header_set(h,
                                FIO_STR_INFO1((char *)"if-none-match"),
                                FIO_STR_INFO1((char *)"*"));
    FIO_ASSERT(!fio_http_etag_is_match(h),
               "a handle without a response status is never evaluated");
    fio_http_free(h);
  }
  /* fio_http_write sends 304 (and drops the data) on a match */
  {
    const char *inm[] = {"W/\"v1\"", NULL};
    fio_http_s *h = test_etag_handle("GET", 200, "\"v1\"", inm);
    fio_http_write(h, .buf = "body", .len = 4, .finish = 1);
    FIO_ASSERT(fio_http_status(h) == 304, "matching write should send 304");
    FIO_ASSERT(fio_http_is_finished(h), "304 response should be finished");
    fio_http_free(h);
    const char *miss[] = {"\"v0\"", NULL};
    h = test_etag_handle("GET", 200, "\"v1\"", miss);
    fio_http_write(h, .buf = "body", .len = 4, .finish = 1);
    FIO_ASSERT(fio_http_status(h) == 200, "mismatch write should keep 200");
    fio_http_free(h);
  }
}

static void test_handle_upgrade_websocket_version(void) {
  fprintf(stderr, "  * WebSocket upgrade with unsupported version\n");
  const char *versions[] = {"8", "12", "14", ""};
  for (size_t i = 0; i < sizeof(versions) / sizeof(*versions); ++i) {
    fio_http_s *h = fio_http_new();
    fio_http_method_set(h, FIO_STR_INFO1((char *)"GET"));
    fio_http_status_set(h, 200);
    fio_http_request_header_set(h,
                                FIO_STR_INFO1((char *)"connection"),
                                FIO_STR_INFO1((char *)"Upgrade"));
    fio_http_request_header_set(h,
                                FIO_STR_INFO1((char *)"upgrade"),
                                FIO_STR_INFO1((char *)"websocket"));
    fio_http_request_header_set(
        h,
        FIO_STR_INFO1((char *)"sec-websocket-key"),
        FIO_STR_INFO1((char *)"dGhlIHNhbXBsZSBub25jZQ=="));
    if (versions[i][0])
      fio_http_request_header_set(
          h,
          FIO_STR_INFO1((char *)"sec-websocket-version"),
          FIO_STR_INFO1((char *)versions[i]));
    fio_http_upgrade_websocket(h);
    FIO_ASSERT(fio_http_status(h) == 400,
               "version '%s' should send 400 (got %zu)",
               versions[i],
               fio_http_status(h));
    FIO_ASSERT(!fio_http_response_header(
                    h,
                    FIO_STR_INFO1((char *)"sec-websocket-accept"),
                    0)
                    .len,
               "version '%s' must not send sec-websocket-accept",
               versions[i]);
    FIO_ASSERT(!fio_http_response_header(h, FIO_STR_INFO1((char *)"upgrade"), 0)
                    .len,
               "version '%s' must not send an upgrade header",
               versions[i]);
    fio_str_info_s v = fio_http_response_header(
        h,
        FIO_STR_INFO1((char *)"sec-websocket-version"),
        0);
    FIO_ASSERT(v.len == 2 && !memcmp(v.buf, "13", 2),
               "400 response should advertise sec-websocket-version: 13");
    FIO_ASSERT(!fio_http_is_upgraded(h), "rejected handle is not upgraded");
    fio_http_free(h);
  }
}

static void test_handle_connect_timeout_range(void) {
  fprintf(stderr, "  * connect_timeout holds values above 255 ms\n");
  fio_http_settings_s s = {.connect_timeout = 5000};
  FIO_ASSERT(s.connect_timeout == 5000, "connect_timeout truncated");
  FIO_ASSERT(sizeof(s.connect_timeout) >= 2,
             "connect_timeout must hold at least 16 bit (milliseconds)");
}

static void test_handle_path_each_trailing_escape(void) {
  fprintf(stderr, "  * FIO_HTTP_PATH_EACH decodes a %%xx at the end\n");
  struct {
    const char *path;
    const char *s[3];
  } cases[] = {
      {"/b%41", {"bA", NULL, NULL}},
      {"/a%20/b%41", {"a ", "bA", NULL}},
      {"/%41", {"A", NULL, NULL}},
      {"/x%4", {"x%4", NULL, NULL}},     /* incomplete: kept as is */
      {"/x%", {"x%", NULL, NULL}},       /* incomplete: kept as is */
      {"/x%zz/y", {"x%zz", "y", NULL}},  /* invalid hex: kept as is */
      {"/%41%42", {"AB", NULL, NULL}},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    fio_str_info_s path = FIO_STR_INFO1((char *)cases[i].path);
    size_t count = 0;
    FIO_HTTP_PATH_EACH(path, section) {
      FIO_ASSERT(count < 3 && cases[i].s[count],
                 "path %s: unexpected section %zu",
                 cases[i].path,
                 count);
      size_t len = strlen(cases[i].s[count]);
      FIO_ASSERT(section.len == len &&
                     !memcmp(section.buf, cases[i].s[count], len),
                 "path %s: section %zu is \"%.*s\", expected \"%s\"",
                 cases[i].path,
                 count,
                 (int)section.len,
                 section.buf,
                 cases[i].s[count]);
      ++count;
    }
    FIO_ASSERT(count == 3 || !cases[i].s[count],
               "path %s: only %zu sections",
               cases[i].path,
               count);
  }
}

static void test_handle_from_forwarded(void) {
  fprintf(stderr, "  * fio_http_from header sources and return values\n");
  struct {
    const char *fwd, *xff, *expect;
    int r;
  } cases[] = {
      {"for=1.2.3.4", NULL, "1.2.3.4", 0},
      {"proto=https;For=\"[2001:db8::1]:80\"", NULL, "[2001:db8::1]:80", 0},
      {"for=1.1.1.1, for=2.2.2.2", NULL, "1.1.1.1", 0},
      {"proto=https", "5.6.7.8", "5.6.7.8", 0}, /* no for= : use xff */
      {"xfor=9.9.9.9", NULL, "[unknown]", -1},  /* not a `for` parameter */
      {"for=\"\"", NULL, "[unknown]", -1},      /* empty value */
      {NULL, "\"3.3.3.3\", 4.4.4.4", "3.3.3.3", 0},
      {NULL, NULL, "[unknown]", -1}, /* mock controller: no socket */
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    fio_http_s *h = fio_http_new();
    if (cases[i].fwd)
      fio_http_request_header_set(h,
                                  FIO_STR_INFO2((char *)"forwarded", 9),
                                  FIO_STR_INFO1((char *)cases[i].fwd));
    if (cases[i].xff)
      fio_http_request_header_set(
          h,
          FIO_STR_INFO2((char *)"x-forwarded-for", 15),
          FIO_STR_INFO1((char *)cases[i].xff));
    char b[64];
    fio_str_info_s dest = FIO_STR_INFO3(b, 0, sizeof(b));
    int r = fio_http_from(&dest, h);
    size_t len = strlen(cases[i].expect);
    FIO_ASSERT(r == cases[i].r && dest.len == len &&
                   !memcmp(dest.buf, cases[i].expect, len) && !b[len],
               "case %zu: got %d \"%.*s\", expected %d \"%s\"",
               i,
               r,
               (int)dest.len,
               dest.buf,
               cases[i].r,
               cases[i].expect);
    /* too small: nothing written, address length returned */
    dest = FIO_STR_INFO3(b, 0, len);
    r = fio_http_from(&dest, h);
    FIO_ASSERT(r == (int)len && !dest.len,
               "case %zu: short dest should return %zu, got %d",
               i,
               len,
               r);
    fio_http_free(h);
  }
  { /* a long header is capped, not masked to zero */
    char big[1024 + 16];
    memcpy(big, "for=7.7.7.7;", 12);
    memset(big + 12, 'x', 1024);
    big[1036] = 0;
    fio_http_s *h = fio_http_new();
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"forwarded", 9),
                                FIO_STR_INFO2(big, 1024));
    char b[64];
    fio_str_info_s dest = FIO_STR_INFO3(b, 0, sizeof(b));
    int r = fio_http_from(&dest, h);
    FIO_ASSERT(!r && dest.len == 7 && !memcmp(b, "7.7.7.7", 7),
               "1024 byte forwarded header: got %d \"%.*s\"",
               r,
               (int)dest.len,
               dest.buf);
    fio_http_free(h);
  }
}

static void test_handle_is_clean_after_headers(void) {
  fprintf(stderr, "  * is_clean is false once headers were sent\n");
  fio_http_s *h = fio_http_new();
  fio_http_status_set(h, 200);
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"content-length", 14),
                               FIO_STR_INFO2((char *)"6", 1));
  FIO_ASSERT(fio_http_is_clean(h), "unsent response should be clean");
  fio_http_write(h, .buf = "abc", .len = 3, .copy = 1);
  FIO_ASSERT(!fio_http_is_finished(h), "response should not be finished");
  FIO_ASSERT(!fio_http_is_clean(h),
             "headers sent (preset content-length) - not clean");
  fio_http_write(h, .buf = "def", .len = 3, .copy = 1, .finish = 1);
  FIO_ASSERT(!fio_http_is_clean(h), "finished response is not clean");
  fio_http_clear_response(h, 0);
  FIO_ASSERT(fio_http_is_clean(h), "clear_response should reset is_clean");
  fio_http_free(h);
}

#if FIO_OS_POSIX
static void test_handle_write_fd_copy(void) {
  fprintf(stderr, "  * fio_http_write keeps a `copy` fd open on error\n");
  for (int copy = 0; copy < 2; ++copy) {
    int fds[2];
    FIO_ASSERT(!pipe(fds), "pipe failed");
    fio_http_s *h = fio_http_new();
    fio_http_status_set(h, 200);
    fio_http_finish(h);
    /* writing after finish is an error */
    fio_http_write(h, .fd = fds[0], .len = 1, .copy = copy, .finish = 1);
    int open = (fcntl(fds[0], F_GETFD) != -1);
    FIO_ASSERT(open == copy,
               "copy=%d: fd should be %s after a failed write",
               copy,
               copy ? "open" : "closed");
    if (open)
      close(fds[0]);
    close(fds[1]);
    fio_http_free(h);
  }
}
#endif

/* ===========================================================================
   WebSocket / SSE request detection
   ===========================================================================
 */

static void test_handle_websocket_request(void) {
  fprintf(stderr, "  * WebSocket request detection\n");
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(!fio_http_websocket_requested(h),
             "empty handle should not request websocket");

  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"connection", 10),
                              FIO_STR_INFO1((char *)"Upgrade"));
  fio_http_request_header_set(h,
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
  FIO_ASSERT(!fio_http_sse_requested(h), "empty handle should not request sse");

  fio_http_request_header_set(h,
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
  char headers[TEST_BODY_STR_MAX];
  int type_present;
  const void *map; /* the map the pair was stored in */
} test_body_pair_s;

typedef struct {
  test_body_pair_s pairs[TEST_BODY_MAX_PAIRS];
  test_obj_s *root;    /* the (latest) root created by a root on_map/on_array */
  size_t roots;        /* root on_map / on_array calls */
  void *pending_file;  /* the open upload object (until stored or freed) */
  size_t files_stored; /* uploads stored through map_push */
  size_t files_freed;  /* uploads discarded through free_unused_object */
  size_t count;
  size_t pushed;
  size_t file_starts;
  char last_key[TEST_BODY_STR_MAX];
  int got_map;
  int got_array;
  int got_null;
  int got_true;
  int got_false;
  int64_t last_number;
  double last_float;
  int callback_error;
  int string_error; /* non-zero: on_string fails with this value */
  char pending_file_name[TEST_BODY_STR_MAX];
  char pending_file_filename[TEST_BODY_STR_MAX];
  char pending_file_ct[TEST_BODY_STR_MAX];
  char pending_file_headers[TEST_BODY_STR_MAX];
  int pending_file_type_present;
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
  /* NOTE: `src` may be NULL when `src_len` is 0 (e.g., a missing multipart
   * Content-Type). Calling memcpy with a NULL pointer is UB even when the
   * length is 0, and GCC uses the call to assume `src != NULL` afterwards
   * (folding later NULL checks to true). Guard the copy to avoid that UB. */
  if (n)
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

#define TEST_OBJ_R(o)  ((fio_json_obj_result_s){.obj = (void *)(o)})
#define TEST_PUSH_R(o) ((fio_json_push_result_s){.obj = (void *)(o)})
/* On error, the result is the partial root, owned (and freed) by the caller. */
#define TEST_BODY_PARTIAL(ctx, r)                                              \
  ((r).result && (r).result == (void *)(ctx).root)

static fio_json_obj_result_s test_body_on_null(void *udata) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_null = 1;
  return TEST_OBJ_R(test_obj_sentinel_new(TEST_OBJ_NULL));
}

static fio_json_obj_result_s test_body_on_true(void *udata) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_true = 1;
  return TEST_OBJ_R(test_obj_sentinel_new(TEST_OBJ_TRUE));
}

static fio_json_obj_result_s test_body_on_false(void *udata) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_false = 1;
  return TEST_OBJ_R(test_obj_sentinel_new(TEST_OBJ_FALSE));
}

static fio_json_obj_result_s test_body_on_number(void *udata, int64_t num) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->last_number = num;
  return TEST_OBJ_R(test_obj_num_new(num));
}

static fio_json_obj_result_s test_body_on_float(void *udata, double num) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->last_float = num;
  return TEST_OBJ_R(test_obj_float_new(num));
}

static fio_json_obj_result_s test_body_on_string(void *udata,
                                                 fio_buf_info_s s) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  if (ctx->string_error)
    return (fio_json_obj_result_s){.err = ctx->string_error};
  return TEST_OBJ_R(test_obj_str_new(s.buf, s.len));
}

/* Records a stored upload (uploads are now handed back via `map_push`). */
static void test_body_record_file(test_body_ctx_s *ctx) {
  if (ctx->count < TEST_BODY_MAX_PAIRS) {
    test_body_pair_s *p = &ctx->pairs[ctx->count++];
    FIO_MEMSET(p, 0, sizeof(*p));
    p->is_file = 1;
    p->map = ctx->root;
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
    test_body_strcpy(p->headers,
                     sizeof(p->headers),
                     ctx->pending_file_headers,
                     strlen(ctx->pending_file_headers));
    p->type_present = ctx->pending_file_type_present;
    test_body_strcpy(p->val,
                     sizeof(p->val),
                     ctx->pending_file_data,
                     ctx->pending_file_data_len);
  }
}

/* Records the pair and consumes `value` (the map model is flat). */
static fio_json_obj_result_s test_body_map_push(void *udata,
                                                void *map_obj,
                                                fio_buf_info_s key,
                                                void *value_obj) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ++ctx->pushed;
  test_body_strcpy(ctx->last_key, sizeof(ctx->last_key), key.buf, key.len);
  test_obj_s *map = (test_obj_s *)map_obj;
  if (value_obj && value_obj == ctx->pending_file) {
    ctx->pending_file = NULL;
    ++ctx->files_stored;
    test_body_record_file(ctx);
  } else if (map && map->type == TEST_OBJ_MAP &&
             ctx->count < TEST_BODY_MAX_PAIRS) {
    test_body_pair_s *p = &ctx->pairs[ctx->count++];
    FIO_MEMSET(p, 0, sizeof(*p));
    p->map = map_obj;
    test_body_strcpy(p->key, sizeof(p->key), key.buf, key.len);
    test_obj_to_str((test_obj_s *)value_obj, p->val, sizeof(p->val));
  }
  test_obj_free((test_obj_s *)value_obj);
  return TEST_OBJ_R(NULL);
}

static fio_json_push_result_s test_body_on_map(void *udata,
                                               void *parent,
                                               fio_buf_info_s key) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_map = 1;
  (void)key;
  if (!parent) {
    ++ctx->roots;
    if (!ctx->root)
      ctx->root = test_obj_sentinel_new(TEST_OBJ_MAP);
    return TEST_PUSH_R(ctx->root);
  }
  return TEST_PUSH_R(test_obj_sentinel_new(TEST_OBJ_MAP));
}

/* A root `on_map` that never reuses `udata`: each root is a new map. */
static fio_json_push_result_s test_body_on_map_fresh(void *udata,
                                                     void *parent,
                                                     fio_buf_info_s key) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  if (!parent) {
    ++ctx->roots;
    ctx->root = test_obj_sentinel_new(TEST_OBJ_MAP);
    return TEST_PUSH_R(ctx->root);
  }
  return test_body_on_map(udata, parent, key);
}

static fio_json_push_result_s test_body_on_array(void *udata,
                                                 void *parent,
                                                 fio_buf_info_s key) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ctx->got_array = 1;
  (void)key;
  if (!parent) {
    ++ctx->roots;
    if (!ctx->root)
      ctx->root = test_obj_sentinel_new(TEST_OBJ_ARRAY);
    return TEST_PUSH_R(ctx->root);
  }
  return TEST_PUSH_R(test_obj_sentinel_new(TEST_OBJ_ARRAY));
}

static int test_body_array_push(void *udata, void *array, void *value) {
  (void)udata;
  (void)array;
  test_obj_free((test_obj_s *)value);
  return 0;
}

static int test_body_is_map(void *udata, void *ctx) {
  (void)udata;
  return ctx && ((test_obj_s *)ctx)->type == TEST_OBJ_MAP;
}
static int test_body_is_array(void *udata, void *ctx) {
  (void)udata;
  return ctx && ((test_obj_s *)ctx)->type == TEST_OBJ_ARRAY;
}

static void test_body_free_unused(void *udata, void *obj) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  if (obj && obj == ctx->pending_file) { /* an upload discarded on error */
    ctx->pending_file = NULL;
    ++ctx->files_freed;
  }
  test_obj_free((test_obj_s *)obj);
}

static fio_json_obj_result_s test_body_on_file(void *udata,
                                               fio_str_info_s name,
                                               fio_str_info_s filename,
                                               fio_str_info_s content_type,
                                               fio_str_info_s headers,
                                               fio_buf_info_s data) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  ++ctx->file_starts;
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
  ctx->pending_file_type_present = content_type.buf != NULL;
  test_body_strcpy(ctx->pending_file_headers,
                   sizeof(ctx->pending_file_headers),
                   headers.buf,
                   headers.len);
  size_t n = data.len < sizeof(ctx->pending_file_data)
                 ? data.len
                 : sizeof(ctx->pending_file_data);
  FIO_MEMCPY(ctx->pending_file_data, data.buf, n);
  ctx->pending_file_data_len = n;
  /* a NULL obj stores nothing; the object is stored (or freed) by the parser */
  ctx->pending_file = test_obj_str_new("<file>", 6);
  return TEST_OBJ_R(ctx->pending_file);
}

static fio_json_obj_result_s test_body_on_file_abort(void *udata,
                                                     fio_str_info_s name,
                                                     fio_str_info_s filename,
                                                     fio_str_info_s content_type,
                                                     fio_str_info_s headers,
                                                     fio_buf_info_s data) {
  ++((test_body_ctx_s *)udata)->file_starts;
  (void)name, (void)filename, (void)content_type, (void)headers, (void)data;
  /* an opaque error, returned unchanged */
  return (fio_json_obj_result_s){.err = -77};
}

/* Fails (with the configured error) for key "a"; takes `value` either way. */
static fio_json_obj_result_s test_body_map_push_error(void *udata,
                                                      void *map,
                                                      fio_buf_info_s key,
                                                      void *value) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  if (key.len == 1 && key.buf[0] == 'a') {
    test_obj_free((test_obj_s *)value);
    return (fio_json_obj_result_s){.err = ctx->callback_error};
  }
  return test_body_map_push(udata, map, key, value);
}

static const fio_http_params_parse_callbacks_s TEST_BODY_CALLBACKS = {
    .on_null = test_body_on_null,
    .on_true = test_body_on_true,
    .on_false = test_body_on_false,
    .on_number = test_body_on_number,
    .on_float = test_body_on_float,
    .on_string = test_body_on_string,
    .on_string_simple = test_body_on_string,
    .on_array = test_body_on_array,
    .on_map = test_body_on_map,
    .array_push = test_body_array_push,
    .map_push = test_body_map_push,
    .on_file = test_body_on_file,
    .is_map = test_body_is_map,
    .is_array = test_body_is_array,
    .free_unused_object = test_body_free_unused,
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

static void test_body_free_result(fio_http_params_parse_result_s *r) {
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
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
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
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
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
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
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
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
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

/* ---- Query tests ---- */

static void test_query_parse(void) {
  fprintf(stderr, "  * query_parse values are Strings\n");
  fio_http_s *h = fio_http_new();
  fio_http_query_set(
      h,
      FIO_STR_INFO1((char *)"n=null&t=TRUE&f=false&i=-42&d=1.5&s=hello+w%21"
                            "&flag&empty=&&q=%34%32x&h=0x1F&p=%2B5&o=0123"));
  test_body_ctx_s ctx = {0};
  fio_http_params_parse_result_s r =
      fio_http_query_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
  FIO_ASSERT(!r.err, "query_parse: should succeed (err=%d)", r.err);
  FIO_ASSERT(ctx.got_map, "query_parse: should create a map");
  FIO_ASSERT(ctx.count == 12,
             "query_parse: expected 12 pairs, got %zu",
             ctx.count);
  FIO_ASSERT(!ctx.got_null && !ctx.got_true && !ctx.got_false &&
                 !ctx.last_number && ctx.last_float == 0.0,
             "query_parse: no primitives should be created");
  static const char *expect[][2] = {
      {"n", "null"},
      {"t", "TRUE"},
      {"f", "false"},
      {"i", "-42"},
      {"d", "1.5"},
      {"s", "hello w!"},
      {"flag", ""},
      {"empty", ""},
      {"q", "42x"},
      {"h", "0x1F"},
      {"p", "+5"},
      {"o", "0123"},
  };
  for (size_t i = 0; i < sizeof(expect) / sizeof(expect[0]); ++i) {
    test_body_pair_s *p = test_body_find(&ctx, expect[i][0]);
    FIO_ASSERT(p, "query_parse: key '%s' missing", expect[i][0]);
    FIO_ASSERT(!strcmp(p->val, expect[i][1]),
               "query_parse: key '%s' = '%s', expected '%s'",
               expect[i][0],
               p->val,
               expect[i][1]);
  }
  test_body_free_result(&r);

  /* empty query: empty map, no error */
  fio_http_query_set(h, FIO_STR_INFO0);
  test_body_ctx_s ctx2 = {0};
  r = fio_http_query_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx2);
  FIO_ASSERT(!r.err && r.result && ctx2.got_map && !ctx2.count,
             "query_parse: empty query should produce an empty map");
  test_body_free_result(&r);

  /* Invalid and truncated percent escapes stay literal. */
  static const char bad_query[] = "a%ZZ%=b%2%GG";
  fio_http_query_set(h,
                     FIO_STR_INFO2((char *)bad_query, sizeof(bad_query) - 1));
  test_body_ctx_s ctx3 = {0};
  r = fio_http_query_parse(h,
                           .callbacks = &TEST_BODY_CALLBACKS,
                           .udata = &ctx3);
  test_body_pair_s *bad = test_body_find(&ctx3, "a%ZZ%");
  FIO_ASSERT(r.err == FIO_HTTP_PARAMS_PARSE_SUCCESS &&
                 TEST_BODY_PARTIAL(ctx3, r) && r.count == 1 &&
                 r.consumed == sizeof(bad_query) - 1 && bad &&
                 !strcmp(bad->val, "b%2%GG"),
             "query_parse: malformed percent escapes must stay literal");
  test_body_free_result(&r);

  /* invalid arguments */
  r = fio_http_query_parse(h, .callbacks = NULL, .udata = NULL);
  FIO_ASSERT(r.err && !r.result, "query_parse: NULL callbacks should fail");
  r = fio_http_query_parse(NULL, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx2);
  FIO_ASSERT(r.err && !r.result, "query_parse: NULL handle should fail");
  fio_http_free(h);
}

/* ---- JSON tests ---- */

static void test_body_parse_json_simple(void) {
  fprintf(stderr, "  * body_parse JSON simple object\n");
  const char *body = "{\"key\": \"value\", \"num\": 42}";
  fio_http_s *h = test_body_make_handle("application/json", body, strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
  FIO_ASSERT(!r.err && TEST_BODY_PARTIAL(ctx, r),
             "JSON simple: success must return the root (err=%d)",
             r.err);
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

/* JSON / urlencoded bodies larger than the RAM limit (spilled) parse whole. */
static void test_body_parse_spilled(void) {
  fprintf(stderr, "  * body_parse JSON / urlencoded spilled bodies\n");
  const size_t pad = (FIO_HTTP_BODY_RAM_LIMIT << 1);
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, pad + 128, 0);
  FIO_ASSERT_ALLOC(buf);
  for (int type = 0; type < 2; ++type) {
    size_t len = 0;
    if (type == 0) {
      FIO_MEMCPY(buf, "{\"key\": \"value\", \"pad\": \"", 25);
      len = 25;
      FIO_MEMSET(buf + len, 'x', pad);
      len += pad;
      FIO_MEMCPY(buf + len, "\", \"num\": 42}", 13);
      len += 13;
    } else {
      FIO_MEMCPY(buf, "key=value&pad=", 14);
      len = 14;
      FIO_MEMSET(buf + len, 'x', pad);
      len += pad;
      FIO_MEMCPY(buf + len, "&num=42", 7);
      len += 7;
    }
    const char *ct =
        type ? "application/x-www-form-urlencoded" : "application/json";
    for (int api = 0; api < 2; ++api) {
      fio_http_s *h = test_body_make_handle(ct, buf, len);
      FIO_ASSERT(fio_http_body_fd(h) != -1 && fio_http_body_length(h) == len,
                 "spilled parse: body should be in a file");
      test_body_ctx_s ctx = {0};
      size_t offset = len / 2;
      fio_http_body_seek(h, (ssize_t)offset);
      fio_http_params_parse_result_s r =
          api ? fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx)
              : fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
      FIO_ASSERT(!r.err, "spilled parse %d/%d failed (%d)", type, api, r.err);
      FIO_ASSERT(fio_http_body_pos(h) == offset,
                 "spilled parse %d/%d moved position",
                 type,
                 api);
      fio_str_info_s unread = fio_http_body_read(h, 8);
      FIO_ASSERT(unread.len == 8 && !memcmp(unread.buf, buf + offset, 8),
                 "spilled parse %d/%d changed unread bytes",
                 type,
                 api);
      test_body_pair_s *pk = test_body_find(&ctx, "key");
      test_body_pair_s *pn = test_body_find(&ctx, "num");
      test_body_pair_s *pp = test_body_find(&ctx, "pad");
      FIO_ASSERT(pk && !strcmp(pk->val, "value") && pn &&
                     !strcmp(pn->val, "42") && pp && pp->val[0] == 'x',
                 "spilled parse %d/%d: values",
                 type,
                 api);
      test_body_free_result(&r);
      fio_http_free(h);
    }
  }
  FIO_MEM_FREE(buf, pad + 128);
}

static void test_body_parse_json_array(void) {
  fprintf(stderr, "  * body_parse JSON array value\n");
  const char *body = "[\"a\", \"b\", \"c\"]";
  fio_http_s *h = test_body_make_handle("application/json", body, strlen(body));
  test_body_ctx_s ctx = {0};
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
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
    if (pos + _l < buf_size) {                                                 \
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
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
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
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
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

static void test_body_parse_multipart_upload_rules(void) {
  fprintf(stderr,
          "  * multipart upload filename, headers and omission rules\n");
  char body[2048];
  size_t len = test_body_build_multipart(body,
                                         sizeof(body),
                                         "rules",
                                         "one",
                                         "A",
                                         "a/b/c.txt",
                                         "text/plain",
                                         "two",
                                         "B",
                                         "C:\\x\\y.png",
                                         NULL,
                                         "three",
                                         "C",
                                         "../z",
                                         NULL,
                                         "empty",
                                         "bad",
                                         "",
                                         NULL,
                                         "trailing",
                                         "bad",
                                         "dir/",
                                         NULL,
                                         "",
                                         "bad",
                                         "lost.txt",
                                         NULL,
                                         (const char *)NULL);
  fio_http_s *h =
      test_body_make_handle("multipart/form-data; boundary=rules", body, len);
  test_body_ctx_s ctx = {0};
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
  size_t files = 0;
  for (size_t i = 0; i < ctx.count; ++i) {
    files += !!ctx.pairs[i].is_file;
    FIO_ASSERT(strcmp(ctx.pairs[i].key, "empty") &&
                   strcmp(ctx.pairs[i].key, "trailing") && ctx.pairs[i].key[0],
               "skipped file appeared as field");
  }
  FIO_ASSERT(!r.err && files == 3 && ctx.count == 3,
             "upload rules: err %d, files %zu",
             r.err,
             files);
  FIO_ASSERT(!strcmp(ctx.pairs[0].filename, "c.txt") &&
                 !strcmp(ctx.pairs[1].filename, "y.png") &&
                 !strcmp(ctx.pairs[2].filename, "z"),
             "upload rules: filenames not stripped");
  FIO_ASSERT(ctx.pairs[0].type_present &&
                 !strcmp(ctx.pairs[0].content_type, "text/plain") &&
                 !ctx.pairs[1].type_present && !ctx.pairs[2].type_present,
             "upload rules: missing type must be NULL");
  FIO_ASSERT(!strcmp(ctx.pairs[0].headers,
                     "Content-Disposition: form-data; name=\"one\"; "
                     "filename=\"a/b/c.txt\"\r\nContent-Type: text/plain") &&
                 !strcmp(ctx.pairs[1].headers,
                         "Content-Disposition: form-data; name=\"two\"; "
                         "filename=\"C:\\x\\y.png\""),
             "upload rules: raw headers not forwarded");
  FIO_ASSERT(!strcmp(ctx.pairs[0].val, "A") && !strcmp(ctx.pairs[1].val, "B") &&
                 !strcmp(ctx.pairs[2].val, "C"),
             "upload rules: file content mismatch");
  test_body_free_result(&r);
  fio_http_free(h);
}

/* ---- Multipart file skipping and streaming ---- */

static fio_json_obj_result_s test_body_on_file_skip(void *udata,
                                                    fio_str_info_s name,
                                                    fio_str_info_s filename,
                                                    fio_str_info_s content_type,
                                                    fio_str_info_s headers,
                                                    fio_buf_info_s data) {
  ((test_body_ctx_s *)udata)->got_array += 1; /* counts on_file calls */
  (void)name, (void)filename, (void)content_type, (void)headers, (void)data;
  return TEST_OBJ_R(NULL);
}

static const fio_http_params_parse_callbacks_s TEST_SKIP_FILE_CALLBACKS = {
    .on_null = test_body_on_null,
    .on_true = test_body_on_true,
    .on_false = test_body_on_false,
    .on_number = test_body_on_number,
    .on_float = test_body_on_float,
    .on_array = test_body_on_array,
    .array_push = test_body_array_push,
    .on_string = test_body_on_string,
    .on_string_simple = test_body_on_string,
    .on_map = test_body_on_map,
    .map_push = test_body_map_push,
    .on_file = test_body_on_file_skip,
    .is_map = test_body_is_map,
    .is_array = test_body_is_array,
    .free_unused_object = test_body_free_unused,
};

static void test_body_parse_multipart_skip_file(void) {
  fprintf(stderr, "  * body_parse multipart: a NULL on_file obj stores nothing\n");
  char body[1024];
  size_t body_len = test_body_build_multipart(body,
                                              sizeof(body),
                                              "skipb",
                                              "up",
                                              "file data",
                                              "a.txt",
                                              "text/plain",
                                              "f",
                                              "v",
                                              NULL,
                                              NULL,
                                              (const char *)NULL);
  fio_http_s *h = test_body_make_handle("multipart/form-data; boundary=skipb",
                                        body,
                                        body_len);
  test_body_ctx_s ctx = {0};
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_SKIP_FILE_CALLBACKS, .udata = &ctx);
  FIO_ASSERT(!r.err && ctx.got_array == 1,
             "skip file: err %d, on_file calls %d",
             r.err,
             ctx.got_array);
  FIO_ASSERT(ctx.count == 1 && !strcmp(ctx.pairs[0].key, "f") &&
                 !ctx.pending_file_data_len && !ctx.files_stored &&
                 !ctx.files_freed,
             "skip file: nothing stored (%zu pairs)",
             ctx.count);
  test_body_free_result(&r);
  fio_http_free(h);
}

typedef struct {
  const char *expect;
  size_t expect_len;
  size_t got;
  size_t calls;
  size_t max_chunk;
  int mismatch;
  int done;
  int fields;
} test_stream_ctx_s;

static fio_json_obj_result_s test_stream_on_string(void *udata,
                                                   fio_buf_info_s s) {
  (void)udata;
  return TEST_OBJ_R(test_obj_str_new(s.buf, s.len));
}
static fio_json_push_result_s test_stream_on_map(void *udata,
                                                 void *parent,
                                                 fio_buf_info_s key) {
  (void)udata, (void)parent, (void)key;
  return TEST_PUSH_R(test_obj_sentinel_new(TEST_OBJ_MAP));
}
/* The upload object (the context itself) is handed back exactly once: stored
 * here or freed through free_unused_object (`done` counts both). */
static fio_json_obj_result_s test_stream_map_push(void *udata,
                                                  void *map,
                                                  fio_buf_info_s key,
                                                  void *v) {
  test_stream_ctx_s *c = (test_stream_ctx_s *)udata;
  c->fields += (key.len == 5 && !memcmp(key.buf, "after", 5));
  if (v == (void *)c)
    ++c->done;
  else
    test_obj_free((test_obj_s *)v);
  (void)map;
  return TEST_OBJ_R(NULL);
}
/* Checks the complete upload; the context itself is the stored object. */
static fio_json_obj_result_s test_stream_on_file(void *udata,
                                                 fio_str_info_s name,
                                                 fio_str_info_s filename,
                                                 fio_str_info_s content_type,
                                                 fio_str_info_s headers,
                                                 fio_buf_info_s data) {
  test_stream_ctx_s *c = (test_stream_ctx_s *)udata;
  (void)name, (void)filename, (void)content_type, (void)headers;
  c->mismatch |= (c->got + data.len > c->expect_len) ||
                 memcmp(c->expect + c->got, data.buf, data.len);
  c->got += data.len;
  c->calls += 1;
  if (data.len > c->max_chunk)
    c->max_chunk = data.len;
  return TEST_OBJ_R(udata);
}
static void test_stream_free_unused(void *udata, void *obj) {
  if (obj == udata)
    ++((test_stream_ctx_s *)udata)->done;
  else
    test_obj_free((test_obj_s *)obj);
}

static const fio_http_params_parse_callbacks_s TEST_STREAM_CALLBACKS = {
    .on_null = test_body_on_null,
    .on_true = test_body_on_true,
    .on_false = test_body_on_false,
    .on_number = test_body_on_number,
    .on_float = test_body_on_float,
    .on_array = test_body_on_array,
    .array_push = test_body_array_push,
    .on_string = test_stream_on_string,
    .on_string_simple = test_stream_on_string,
    .on_map = test_stream_on_map,
    .map_push = test_stream_map_push,
    .on_file = test_stream_on_file,
    .free_unused_object = test_stream_free_unused,
};

static void test_body_parse_multipart_stream_large(void) {
  fprintf(stderr, "  * params_parse multipart: large upload, complete and in place\n");
  const size_t file_len = (FIO_HTTP_BODY_RAM_LIMIT * 4) + 12345;
  char *file = (char *)malloc(file_len);
  FIO_ASSERT(file, "allocation failed");
  for (size_t i = 0; i < file_len; ++i)
    file[i] = (char)(i * 7 + (i >> 9));
  const char *head = "--bigb\r\nContent-Disposition: form-data; name=\"up\"; "
                     "filename=\"big.bin\"\r\n\r\n";
  const char *tail = "\r\n--bigb\r\nContent-Disposition: form-data; "
                     "name=\"after\"\r\n\r\nok\r\n--bigb--\r\n";
  fio_http_s *h = fio_http_new();
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"content-type", 12),
      FIO_STR_INFO1((char *)"multipart/form-data; boundary=bigb"));
  fio_http_body_write(h, head, strlen(head));
  fio_http_body_write(h, file, file_len);
  fio_http_body_write(h, tail, strlen(tail));
  FIO_ASSERT(fio_http_body_fd(h) != -1, "large body should spill to a file");

  test_stream_ctx_s c = {.expect = file, .expect_len = file_len};
  fio_http_params_parse_result_s r =
      fio_http_params_parse(h, .callbacks = &TEST_STREAM_CALLBACKS, .udata = &c);
  FIO_ASSERT(!r.err && r.result, "stream large: err %d", r.err);
  FIO_ASSERT(c.got == file_len && !c.mismatch && c.done == 1,
             "stream large: got %zu / %zu (mismatch %d, done %d)",
             c.got,
             file_len,
             c.mismatch,
             c.done);
  FIO_ASSERT(c.calls == 1 && c.max_chunk == file_len,
             "stream large: expected one complete file (%zu calls, max %zu)",
             c.calls,
             c.max_chunk);
  FIO_ASSERT(c.fields == 1, "stream large: field after the file missing");
  FIO_ASSERT(r.consumed == fio_http_body_length(h),
             "stream large: consumed %zu / %zu",
             r.consumed,
             fio_http_body_length(h));
  test_obj_free((test_obj_s *)r.result);

  /* truncated body: the file never reaches on_file and the parse fails */
  fio_http_s *t = fio_http_new();
  fio_http_request_header_set(
      t,
      FIO_STR_INFO2((char *)"content-type", 12),
      FIO_STR_INFO1((char *)"multipart/form-data; boundary=bigb"));
  fio_http_body_write(t, head, strlen(head));
  fio_http_body_write(t, file, file_len / 2);
  test_stream_ctx_s c2 = {.expect = file, .expect_len = file_len};
  r = fio_http_params_parse(t, .callbacks = &TEST_STREAM_CALLBACKS, .udata = &c2);
  FIO_ASSERT(r.err == FIO_HTTP_PARAMS_PARSE_ERROR && !c2.done && !c2.calls,
             "stream truncated: err %d, done %d",
             r.err,
             c2.done);
  test_obj_free((test_obj_s *)r.result);
  fio_http_free(t);
  fio_http_free(h);
  free(file);
}

/* Records a large field ("big") while delegating to the stream callbacks. */
static size_t test_framing_big_len;
static int test_framing_big_ok;
static fio_json_obj_result_s test_framing_map_push(void *udata,
                                                   void *map,
                                                   fio_buf_info_s key,
                                                   void *v) {
  if (key.len == 3 && !memcmp(key.buf, "big", 3) && v != udata) {
    test_obj_s *o = (test_obj_s *)v;
    test_framing_big_len = o->u.str.len;
    test_framing_big_ok = 1;
    for (size_t i = 0; i < o->u.str.len; ++i)
      test_framing_big_ok &= (o->u.str.buf[i] == (char)('a' + (i % 23)));
  }
  return test_stream_map_push(udata, map, key, v);
}

/* Builds and parses `head`, `fill` bytes of a file / field, then `tail`. */
static fio_http_params_parse_result_s test_framing_run(
    test_stream_ctx_s *c,
    const fio_http_params_parse_callbacks_s *cb,
    const char *head,
    const char *file,
    size_t fill,
    const char *tail) {
  fio_http_s *h = fio_http_new();
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"content-type", 12),
      FIO_STR_INFO1((char *)"multipart/form-data; boundary=edge"));
  fio_http_body_write(h, head, strlen(head));
  fio_http_body_write(h, file, fill);
  fio_http_body_write(h, tail, strlen(tail));
  fio_http_params_parse_result_s r =
      fio_http_params_parse(h, .callbacks = cb, .udata = c);
  test_obj_free((test_obj_s *)r.result);
  r.result = NULL;
  fio_http_free(h);
  return r;
}

static void test_body_parse_multipart_framing(void) {
  fprintf(stderr, "  * params_parse multipart: read edges and big fields\n");
  const size_t edge = (1UL << 16); /* FIO___HTTP_PARAMS_CHUNK */
  const size_t big = (FIO_HTTP_BODY_RAM_LIMIT * 2) + 777;
  char *file = (char *)malloc(big);
  FIO_ASSERT(file, "allocation failed");
  for (size_t i = 0; i < big; ++i)
    file[i] = (char)('a' + (i % 23));
  const char *head = "--edge\r\nContent-Disposition: form-data; name=\"up\"; "
                     "filename=\"e.bin\"\r\n\r\n";
  const char *close = "\r\n--edge--";
  /* the closing delimiter ends exactly at the first read's end */
  size_t fill = edge - strlen(head) - strlen(close);
  static const struct {
    const char *suffix;
    int ok;
  } cases[] = {
      {"", 1},
      {"\r\nepilogue", 1},
      {"  \t\r\n", 1},
      {"  ", 1},
      {"garbage", 0},
      {"  x", 0},
      {"\r", 0},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    char tail[64];
    snprintf(tail, sizeof(tail), "%s%s", close, cases[i].suffix);
    test_stream_ctx_s c = {.expect = file, .expect_len = fill};
    fio_http_params_parse_result_s r =
        test_framing_run(&c, &TEST_STREAM_CALLBACKS, head, file, fill, tail);
    FIO_ASSERT((cases[i].ok ? (!r.err && c.done == 1 && c.got == fill)
                            : (r.err == FIO_HTTP_PARAMS_PARSE_ERROR &&
                               !c.done && !c.got)) &&
                   !c.mismatch,
               "read edge close + \"%s\": err %d, done %d, got %zu",
               cases[i].suffix,
               r.err,
               c.done,
               c.got);
  }

  /* a field larger than a read (spilled body): parsed whole, in place */
  {
    fio_http_params_parse_callbacks_s cb = TEST_STREAM_CALLBACKS;
    cb.map_push = test_framing_map_push;
    test_stream_ctx_s c = {0};
    test_framing_big_len = 0;
    test_framing_big_ok = 0;
    fio_http_params_parse_result_s r = test_framing_run(
        &c,
        &cb,
        "--edge\r\nContent-Disposition: form-data; name=\"big\"\r\n\r\n",
        file,
        big,
        "\r\n--edge\r\nContent-Disposition: form-data; name=\"after\"\r\n\r\n"
        "ok\r\n--edge--\r\n");
    FIO_ASSERT(!r.err && test_framing_big_len == big && test_framing_big_ok &&
                   c.fields == 1,
               "big field: err %d, len %zu / %zu (ok %d), after %d",
               r.err,
               test_framing_big_len,
               big,
               test_framing_big_ok,
               c.fields);
    /* truncated big field fails */
    r = test_framing_run(
        &c,
        &cb,
        "--edge\r\nContent-Disposition: form-data; name=\"big\"\r\n\r\n",
        file,
        big,
        "");
    FIO_ASSERT(r.err == FIO_HTTP_PARAMS_PARSE_ERROR,
               "truncated big field: err %d",
               r.err);
  }
  free(file);
}

/* ---- Params (query + body) tests ---- */

static fio_http_params_parse_result_s test_params_run(test_body_ctx_s *ctx,
                                                      const char *query,
                                                      const char *ct,
                                                      const char *body,
                                                      size_t body_len) {
  fio_http_s *h =
      ct ? test_body_make_handle(ct, body, body_len) : fio_http_new();
  if (!ct && body_len)
    fio_http_body_write(h, body, body_len);
  if (query)
    fio_http_query_set(h, FIO_STR_INFO1((char *)query));
  fio_http_params_parse_result_s r =
      fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = ctx);
  fio_http_free(h);
  return r;
}

static void test_params_expect(test_body_ctx_s *ctx,
                               const char *name,
                               size_t index,
                               const char *key,
                               const char *val) {
  FIO_ASSERT(index < ctx->count, "%s: missing pair #%zu", name, index);
  FIO_ASSERT(!strcmp(ctx->pairs[index].key, key) &&
                 !strcmp(ctx->pairs[index].val, val),
             "%s: pair #%zu = '%s'=>'%s', expected '%s'=>'%s'",
             name,
             index,
             ctx->pairs[index].key,
             ctx->pairs[index].val,
             key,
             val);
}

static void test_params_parse(void) {
  fprintf(stderr, "  * params_parse query + body\n");
  fio_http_params_parse_result_s r;
  { /* query then urlencoded body, same map, query first */
    test_body_ctx_s ctx = {0};
    const char *body = "c=TRUE&a=2";
    r = test_params_run(&ctx,
                        "a=1&b",
                        "application/x-www-form-urlencoded",
                        body,
                        strlen(body));
    FIO_ASSERT(!r.err && r.result && ctx.count == 4,
               "params urlencoded: err=%d count=%zu",
               r.err,
               ctx.count);
    FIO_ASSERT(r.consumed == 5 + strlen(body),
               "params urlencoded: consumed %zu",
               r.consumed);
    test_params_expect(&ctx, "params urlencoded", 0, "a", "1");
    test_params_expect(&ctx, "params urlencoded", 1, "b", "");
    test_params_expect(&ctx, "params urlencoded", 2, "c", "TRUE");
    test_params_expect(&ctx, "params urlencoded", 3, "a", "2");
    test_body_free_result(&r);
  }
  { /* JSON object body merges into the query map */
    test_body_ctx_s ctx = {0};
    const char *body = "{\"j\": \"x\", \"n\": 7}";
    r = test_params_run(&ctx, "q=1", "application/json", body, strlen(body));
    FIO_ASSERT(!r.err && r.result && ctx.count == 3 &&
                   TEST_BODY_PARTIAL(ctx, r),
               "params JSON object: success must return the query map "
               "(err=%d count=%zu)",
               r.err,
               ctx.count);
    test_params_expect(&ctx, "params JSON object", 0, "q", "1");
    test_params_expect(&ctx, "params JSON object", 1, "j", "x");
    test_params_expect(&ctx, "params JSON object", 2, "n", "7");
    test_body_free_result(&r);
  }
  { /* Non-object JSON bodies contribute nothing. */
    test_body_ctx_s ctx = {0};
    r = test_params_run(&ctx, "q=1", "application/json", "[1,2]", 5);
    FIO_ASSERT(!r.err && r.result && ctx.count == 1 && !ctx.got_array,
               "params JSON array: err=%d count=%zu",
               r.err,
               ctx.count);

    test_body_free_result(&r);
    test_body_ctx_s ctx2 = {0};
    r = test_params_run(&ctx2, NULL, "application/json", "42", 2);
    FIO_ASSERT(!r.err && r.result && ctx2.count == 0,
               "params JSON primitive: err=%d count=%zu",
               r.err,
               ctx2.count);

    test_body_free_result(&r);
  }
  { /* multipart body */
    test_body_ctx_s ctx = {0};
    char body[1024];
    size_t len = test_body_build_multipart(body,
                                           sizeof(body),
                                           "pb123",
                                           "f",
                                           "v",
                                           NULL,
                                           NULL,
                                           NULL);
    r = test_params_run(&ctx,
                        "q=1",
                        "multipart/form-data; boundary=pb123",
                        body,
                        len);
    FIO_ASSERT(!r.err && r.result && ctx.count == 2,
               "params multipart: err=%d count=%zu",
               r.err,
               ctx.count);
    test_params_expect(&ctx, "params multipart", 0, "q", "1");
    test_params_expect(&ctx, "params multipart", 1, "f", "v");
    test_body_free_result(&r);
  }
  { /* unknown / missing Content-Type body is ignored */
    test_body_ctx_s ctx = {0};
    r = test_params_run(&ctx, "q=1", "text/plain", "a=b", 3);
    FIO_ASSERT(!r.err && r.result && ctx.count == 1 &&
                   TEST_BODY_PARTIAL(ctx, r),
               "params unknown content-type: err=%d count=%zu",
               r.err,
               ctx.count);
    test_body_free_result(&r);
    test_body_ctx_s ctx2 = {0};
    r = test_params_run(&ctx2, NULL, NULL, "a=b", 3);
    FIO_ASSERT(!r.err && r.result && !ctx2.count && ctx2.got_map,
               "params missing content-type: should be an empty map");
    test_body_free_result(&r);
  }
  { /* nothing at all: empty map */
    test_body_ctx_s ctx = {0};
    r = test_params_run(&ctx, NULL, NULL, NULL, 0);
    FIO_ASSERT(!r.err && r.result && !ctx.count,
               "params empty: should be an empty map");
    test_body_free_result(&r);
  }
  { /* JSON errors return the partial params map to the caller (no leaks) */
    test_body_ctx_s ctx = {0};
    const char *bad[] = {"{\"a\": [1, {\"b\":", "{\"a\": [1, 2", "{\"a\" 1}"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
      ctx = (test_body_ctx_s){0};
      r = test_params_run(&ctx,
                          "q=1",
                          "application/json",
                          bad[i],
                          strlen(bad[i]));
      FIO_ASSERT(r.err && TEST_BODY_PARTIAL(ctx, r) && ctx.roots == 1 &&
                     test_body_find(&ctx, "q"),
                 "params JSON error #%zu: err=%d partial=%d",
                 i,
                 r.err,
                 TEST_BODY_PARTIAL(ctx, r));
      test_body_free_result(&r); /* the caller owns the partial map */
    }
  }
  { /* invalid arguments */
    fio_http_s *h = fio_http_new();
    r = fio_http_params_parse(h, .callbacks = NULL, .udata = NULL);
    FIO_ASSERT(r.err && !r.result, "params: NULL callbacks should fail");
    fio_http_free(h);
    test_body_ctx_s ctx = {0};
    r = fio_http_params_parse(NULL, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
    FIO_ASSERT(r.err && !r.result, "params: NULL handle should fail");
  }
}

static void test_params_depth(void) {
  fprintf(stderr, "  * name and JSON depth boundaries\n");
  char name[2 * FIO_JSON_MAX_DEPTH + 8];
  char body[8 * FIO_JSON_MAX_DEPTH + 32];
  for (size_t n = FIO_JSON_MAX_DEPTH; n <= FIO_JSON_MAX_DEPTH + 1; ++n) {
    size_t len = 0, json_len = 0;
    for (size_t i = 0; i < n; ++i) {
      if (i)
        name[len++] = '.';
      name[len++] = 'a';
      json_len +=
          (size_t)snprintf(body + json_len, sizeof(body) - json_len, "{\"a\":");
    }
    memcpy(name + len, "=1", 3);
    body[json_len++] = '1';
    for (size_t i = 0; i < n; ++i)
      body[json_len++] = '}';
    body[json_len] = 0;
    for (int type = 0; type < 4; ++type) {
      test_body_ctx_s ctx = {0};
      char multipart[2 * FIO_JSON_MAX_DEPTH + 256];
      int mlen = snprintf(multipart,
                          sizeof(multipart),
                          "--X\r\nContent-Disposition: form-data; "
                          "name=\"%.*s\"; filename=\"x\"\r\n\r\nx\r\n--X--\r\n",
                          (int)len,
                          name);
      FIO_ASSERT(mlen > 0 && (size_t)mlen < sizeof(multipart),
                 "multipart depth buffer");
      fio_http_s *h =
          test_body_make_handle(type == 1 ? "application/x-www-form-urlencoded"
                                : type == 2 ? "multipart/form-data; boundary=X"
                                : type == 3 ? "application/json"
                                            : NULL,
                                type == 0   ? NULL
                                : type == 1 ? name
                                : type == 2 ? multipart
                                            : body,
                                type == 0   ? 0
                                : type == 1 ? len + 2
                                : type == 2 ? (size_t)mlen
                                            : json_len);
      if (type == 0)
        fio_http_query_set(h, FIO_STR_INFO2(name, len + 2));
      else
        fio_http_body_seek(h, 1);
      fio_http_params_parse_result_s r =
          fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
      FIO_ASSERT(r.err == (n > FIO_JSON_MAX_DEPTH
                               ? FIO_HTTP_PARAMS_PARSE_DEPTH
                               : FIO_HTTP_PARAMS_PARSE_SUCCESS) &&
                     TEST_BODY_PARTIAL(ctx, r),
                 "depth n=%zu type=%d err=%d partial=%d",
                 n,
                 type,
                 r.err,
                 TEST_BODY_PARTIAL(ctx, r));
      if (n > FIO_JSON_MAX_DEPTH && type != 3)
        FIO_ASSERT(!ctx.pushed && !ctx.count,
                   "deep name stored a field (type=%d)",
                   type);
      if (n <= FIO_JSON_MAX_DEPTH)
        FIO_ASSERT(ctx.pushed,
                   "accepted depth stored no field (type=%d)",
                   type);
      if (n > FIO_JSON_MAX_DEPTH && type == 2)
        FIO_ASSERT(!ctx.file_starts, "deep file reached on_file");
      if (type)
        FIO_ASSERT(fio_http_body_pos(h) == 1,
                   "depth n=%zu type=%d: body position changed",
                   n,
                   type);
      test_body_free_result(&r);
      fio_http_free(h);
    }
  }
}

static fio_http_params_parse_result_s test_params_parse_api(
    int combined,
    fio_http_s *h,
    const fio_http_params_parse_callbacks_s *callbacks,
    test_body_ctx_s *ctx) {
  return combined
             ? fio_http_params_parse(h, .callbacks = callbacks, .udata = ctx)
             : fio_http_body_parse(h, .callbacks = callbacks, .udata = ctx);
}

static void test_params_typed_json_errors(void) {
  fprintf(stderr, "  * typed JSON errors (direct and combined)\n");
  char deep[8 * (FIO_JSON_MAX_DEPTH + 1) + 32];
  size_t deep_len = 0;
  for (size_t i = 0; i <= FIO_JSON_MAX_DEPTH; ++i)
    deep_len +=
        (size_t)snprintf(deep + deep_len, sizeof(deep) - deep_len, "{\"a\":");
  deep[deep_len++] = '1';
  for (size_t i = 0; i <= FIO_JSON_MAX_DEPTH; ++i)
    deep[deep_len++] = '}';
  deep[deep_len] = 0;

  const int expected[] = {
      FIO_HTTP_PARAMS_PARSE_ERROR,
      FIO_HTTP_PARAMS_PARSE_DEPTH,
      -73,
  };
  for (int kind = 0; kind < 3; ++kind) {
    const char *body = kind == 0 ? "{\"a\":" : kind == 1 ? deep : "{\"a\":1}";
    size_t len = kind == 1 ? deep_len : strlen(body);
    for (int combined = 0; combined < 2; ++combined) {
      fio_http_s *h = test_body_make_handle("application/json", body, len);
      if (combined && kind != 2)
        fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1"));
      fio_http_body_seek(h, 1);
      test_body_ctx_s ctx = {.callback_error = kind == 2 ? -73 : 0};
      fio_http_params_parse_callbacks_s callbacks = TEST_BODY_CALLBACKS;
      if (kind == 2)
        callbacks.map_push = test_body_map_push_error;
      fio_http_params_parse_result_s r =
          test_params_parse_api(combined, h, &callbacks, &ctx);
      FIO_ASSERT(r.err == expected[kind] && TEST_BODY_PARTIAL(ctx, r) &&
                     r.count == (size_t)(combined && kind != 2),
                 "JSON typed error kind=%d combined=%d: err=%d count=%zu "
                 "partial=%d",
                 kind,
                 combined,
                 r.err,
                 r.count,
                 TEST_BODY_PARTIAL(ctx, r));
      FIO_ASSERT(fio_http_body_pos(h) == 1,
                 "JSON typed error kind=%d combined=%d moved body position",
                 kind,
                 combined);
      test_body_free_result(&r);
      fio_http_free(h);
    }
  }

  /* JSON callback values propagate without an HTTP translation layer. */
  static const int codes[] = {-2, -1, -77, 39};
  for (int combined = 0; combined < 2; ++combined) {
    for (size_t ci = 0; ci < sizeof(codes) / sizeof(codes[0]); ++ci) {
      const int code = codes[ci];
      const char *body = "{\"a\":1}";
      fio_http_s *h =
          test_body_make_handle("application/json", body, strlen(body));
      if (combined)
        fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1"));
      fio_http_body_seek(h, 1);
      test_body_ctx_s ctx = {.callback_error = code};
      fio_http_params_parse_callbacks_s callbacks = TEST_BODY_CALLBACKS;
      callbacks.map_push = test_body_map_push_error;
      fio_http_params_parse_result_s r =
          test_params_parse_api(combined, h, &callbacks, &ctx);
      FIO_ASSERT(r.err == code && TEST_BODY_PARTIAL(ctx, r) &&
                     r.count == (size_t)combined,
                 "JSON callback error code=%d combined=%d: "
                 "err=%d count=%zu partial=%d",
                 code,
                 combined,
                 r.err,
                 r.count,
                 TEST_BODY_PARTIAL(ctx, r));
      FIO_ASSERT(fio_http_body_pos(h) == 1,
                 "JSON callback error moved body position");
      test_body_free_result(&r);
      fio_http_free(h);
    }
  }
}

static void test_params_typed_multipart_errors(void) {
  fprintf(stderr, "  * typed multipart errors (direct and combined)\n");
  const char *truncated =
      "--X\r\nContent-Disposition: form-data; name=\"a\"\r\n\r\nx";
  for (int combined = 0; combined < 2; ++combined) {
    fio_http_s *h = test_body_make_handle("multipart/form-data; boundary=X",
                                          truncated,
                                          strlen(truncated));
    if (combined)
      fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1"));
    fio_http_body_seek(h, 1);
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        test_params_parse_api(combined, h, &TEST_BODY_CALLBACKS, &ctx);
    FIO_ASSERT(r.err == FIO_HTTP_PARAMS_PARSE_ERROR &&
                   TEST_BODY_PARTIAL(ctx, r) && r.count == (size_t)combined &&
                   fio_http_body_pos(h) == 1,
               "truncated multipart combined=%d: err=%d count=%zu",
               combined,
               r.err,
               r.count);
    test_body_free_result(&r);
    fio_http_free(h);
  }

  char name[2 * FIO_JSON_MAX_DEPTH + 8];
  size_t name_len = 0;
  for (size_t i = 0; i <= FIO_JSON_MAX_DEPTH; ++i) {
    if (i)
      name[name_len++] = '.';
    name[name_len++] = 'a';
  }
  name[name_len] = 0;
  for (int file = 0; file < 2; ++file) {
    char body[2 * FIO_JSON_MAX_DEPTH + 384];
    int len = snprintf(body,
                       sizeof(body),
                       "--X\r\nContent-Disposition: form-data; name=\"%.*s\"%s"
                       "\r\n\r\nx\r\n--X--\r\n",
                       (int)name_len,
                       name,
                       file ? "; filename=\"x\"" : "");
    FIO_ASSERT(len > 0 && (size_t)len < sizeof(body),
               "typed multipart depth buffer");
    for (int combined = 0; combined < 2; ++combined) {
      fio_http_s *h = test_body_make_handle("multipart/form-data; boundary=X",
                                            body,
                                            (size_t)len);
      if (combined)
        fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1"));
      fio_http_body_seek(h, 1);
      test_body_ctx_s ctx = {0};
      fio_http_params_parse_result_s r =
          test_params_parse_api(combined, h, &TEST_BODY_CALLBACKS, &ctx);
      FIO_ASSERT(r.err == FIO_HTTP_PARAMS_PARSE_DEPTH &&
                     TEST_BODY_PARTIAL(ctx, r) &&
                     r.count == (size_t)(1 + combined) &&
                     ctx.count == (size_t)combined && fio_http_body_pos(h) == 1,
                 "multipart depth file=%d combined=%d: err=%d count=%zu "
                 "stored=%zu",
                 file,
                 combined,
                 r.err,
                 r.count,
                 ctx.count);
      if (file)
        FIO_ASSERT(!ctx.file_starts, "deep file reached on_file");
      test_body_free_result(&r);
      fio_http_free(h);
    }
  }

  const char *aborted = "--X\r\nContent-Disposition: form-data; name=\"up\"; "
                        "filename=\"x\"\r\n\r\nx\r\n--X--\r\n";
  for (int combined = 0; combined < 2; ++combined) {
    fio_http_s *h = test_body_make_handle("multipart/form-data; boundary=X",
                                          aborted,
                                          strlen(aborted));
    if (combined)
      fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1"));
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_callbacks_s callbacks = TEST_BODY_CALLBACKS;
    callbacks.on_file = test_body_on_file_abort;
    fio_http_params_parse_result_s r =
        test_params_parse_api(combined, h, &callbacks, &ctx);
    FIO_ASSERT(r.err == -77 && TEST_BODY_PARTIAL(ctx, r) &&
                   r.count == (size_t)(1 + combined) && !ctx.files_stored &&
                   ctx.file_starts == 1 && !ctx.files_freed &&
                   !test_body_find(&ctx, "up"),
               "multipart callback abort combined=%d: no upload object may "
               "be stored (err=%d count=%zu)",
               combined,
               r.err,
               r.count);
    test_body_free_result(&r);
    fio_http_free(h);
  }

  /* a file part cut off before its boundary never reaches on_file */
  const char *cut = "--X\r\nContent-Disposition: form-data; name=\"up\"; "
                    "filename=\"x\"\r\n\r\nxyz";
  for (int combined = 0; combined < 2; ++combined) {
    fio_http_s *h = test_body_make_handle("multipart/form-data; boundary=X",
                                          cut,
                                          strlen(cut));
    if (combined)
      fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1"));
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        test_params_parse_api(combined, h, &TEST_BODY_CALLBACKS, &ctx);
    FIO_ASSERT(r.err == FIO_HTTP_PARAMS_PARSE_ERROR &&
                   TEST_BODY_PARTIAL(ctx, r) && !ctx.files_stored &&
                   !ctx.file_starts && !ctx.files_freed &&
                   !test_body_find(&ctx, "up"),
               "truncated upload combined=%d: no on_file (err=%d "
               "stored=%zu freed=%zu)",
               combined,
               r.err,
               (size_t)ctx.files_stored,
               (size_t)ctx.files_freed);
    test_body_free_result(&r);
    fio_http_free(h);
  }
}

#ifndef _WIN32
static void test_params_typed_io_errors(void) {
  fprintf(stderr, "  * typed body-read I/O errors (direct and combined)\n");
  static const struct {
    const char *content_type;
    const char *body;
  } cases[] = {
      {"application/json", "{\"a\":1}"},
      {"application/x-www-form-urlencoded", "a=1"},
      {"multipart/form-data; boundary=X",
       "--X\r\nContent-Disposition: form-data; name=\"a\"\r\n\r\n"
       "x\r\n--X--\r\n"},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    for (int combined = 0; combined < 2; ++combined) {
      fio_http_s *h = test_body_make_handle(cases[i].content_type, NULL, 0);
      fio_http_body_expect(h, FIO_HTTP_BODY_RAM_LIMIT + 1);
      FIO_ASSERT(
          fio_http_body_fd(h) != -1 &&
              !fio_http_body_write(h, cases[i].body, strlen(cases[i].body)),
          "I/O test body should spill and remain writable");
      if (combined)
        fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1"));
      fio_http_body_seek(h, 1);
      test_body_ctx_s ctx = {0};
      test_fd_hooks_reset();
      test_fd_read.fail_at = 0;
      test_fd_read.fail_errno = EIO;
      fio_http_params_parse_result_s r =
          test_params_parse_api(combined, h, &TEST_BODY_CALLBACKS, &ctx);
      size_t read_calls = test_fd_read.calls;
      test_fd_hooks_reset();
      FIO_ASSERT(r.err == FIO_HTTP_PARAMS_PARSE_IO && read_calls &&
                     r.count == (size_t)combined && fio_http_body_pos(h) == 1,
                 "body I/O case=%zu combined=%d: err=%d count=%zu reads=%zu",
                 i,
                 combined,
                 r.err,
                 r.count,
                 read_calls);
      test_body_free_result(&r);
      fio_http_free(h);
    }
  }
}
#endif

/* Fails at the root (configured error); nested maps open normally. */
static fio_json_push_result_s test_body_on_map_root_error(void *udata,
                                                          void *parent,
                                                          fio_buf_info_s key) {
  test_body_ctx_s *ctx = (test_body_ctx_s *)udata;
  if (!parent)
    return (fio_json_push_result_s){.err = ctx->callback_error};
  return test_body_on_map(udata, parent, key);
}

/* Callback errors reach `err` unchanged from every params source. */
static void test_params_callback_errors(void) {
  fprintf(stderr, "  * callback errors pass through unchanged (all sources)\n");
  static const int codes[] = {-77, 39};
  const char *multipart = "--X\r\nContent-Disposition: form-data; "
                          "name=\"f\"\r\n\r\nv\r\n--X--\r\n";
  for (size_t ci = 0; ci < sizeof(codes) / sizeof(codes[0]); ++ci) {
    const int code = codes[ci];
    fio_http_params_parse_callbacks_s failing_push = TEST_BODY_CALLBACKS;
    failing_push.map_push = test_body_map_push_error;
    fio_http_params_parse_callbacks_s failing_root = TEST_BODY_CALLBACKS;
    failing_root.on_map = test_body_on_map_root_error;
    { /* query map_push */
      fio_http_s *h = fio_http_new();
      fio_http_query_set(h, FIO_STR_INFO1((char *)"x=1&a=2"));
      test_body_ctx_s ctx = {.callback_error = code};
      fio_http_params_parse_result_s r =
          fio_http_query_parse(h, .callbacks = &failing_push, .udata = &ctx);
      /* consumed excludes the failed pair (a=2). */
      FIO_ASSERT(r.err == code && TEST_BODY_PARTIAL(ctx, r) && r.count == 2 &&
                     r.consumed == 4 && test_body_find(&ctx, "x"),
                 "query map_push error %d: err=%d consumed=%zu",
                 code,
                 r.err,
                 r.consumed);
      test_body_free_result(&r);
      /* query string error through the combined parser */
      ctx = (test_body_ctx_s){.string_error = code};
      r = fio_http_params_parse(h,
                                .callbacks = &TEST_BODY_CALLBACKS,
                                .udata = &ctx);
      FIO_ASSERT(r.err == code && TEST_BODY_PARTIAL(ctx, r) && !ctx.count,
                 "params query string error %d: err=%d",
                 code,
                 r.err);
      test_body_free_result(&r);
      /* root creation error: no partial root exists */
      ctx = (test_body_ctx_s){.callback_error = code};
      r = fio_http_query_parse(h, .callbacks = &failing_root, .udata = &ctx);
      FIO_ASSERT(r.err == code && !r.result && !ctx.root,
                 "query root error %d: err=%d",
                 code,
                 r.err);
      ctx = (test_body_ctx_s){.callback_error = code};
      r = fio_http_params_parse(h, .callbacks = &failing_root, .udata = &ctx);
      FIO_ASSERT(r.err == code && !r.result && !ctx.root,
                 "params root error %d: err=%d",
                 code,
                 r.err);
      fio_http_free(h);
    }
    for (int combined = 0; combined < 2; ++combined) {
      /* URL-encoded body map_push */
      fio_http_s *h = test_body_make_handle("application/x-www-form-urlencoded",
                                            "x=1&a=2",
                                            7);
      test_body_ctx_s ctx = {.callback_error = code};
      fio_http_params_parse_result_s r =
          test_params_parse_api(combined, h, &failing_push, &ctx);
      FIO_ASSERT(r.err == code && TEST_BODY_PARTIAL(ctx, r) && r.count == 2 &&
                     r.consumed == 4,
                 "urlencoded map_push error %d combined=%d: err=%d "
                 "consumed=%zu",
                 code,
                 combined,
                 r.err,
                 r.consumed);
      test_body_free_result(&r);
      ctx = (test_body_ctx_s){.callback_error = code};
      r = test_params_parse_api(combined, h, &failing_root, &ctx);
      FIO_ASSERT(r.err == code && !r.result && !ctx.root,
                 "urlencoded root error %d combined=%d: err=%d",
                 code,
                 combined,
                 r.err);
      fio_http_free(h);
      /* multipart field string error */
      h = test_body_make_handle("multipart/form-data; boundary=X",
                                multipart,
                                strlen(multipart));
      ctx = (test_body_ctx_s){.string_error = code};
      r = test_params_parse_api(combined, h, &TEST_BODY_CALLBACKS, &ctx);
      FIO_ASSERT(r.err == code && TEST_BODY_PARTIAL(ctx, r) && !ctx.count,
                 "multipart string error %d combined=%d: err=%d",
                 code,
                 combined,
                 r.err);
      test_body_free_result(&r);
      fio_http_free(h);
    }
  }
}

/* A JSON object body updates the query map even when the root callbacks never
 * reuse `udata` (each root `on_map` creates a new map). */
static void test_params_json_fresh_root(void) {
  fprintf(stderr, "  * params JSON body with non-reusing root callbacks\n");
  fio_http_params_parse_callbacks_s callbacks = TEST_BODY_CALLBACKS;
  callbacks.on_map = test_body_on_map_fresh;
  static const char *bodies[] = {"{\"j\": \"x\", \"n\": 7}", "{\"j\": \"x\", "};
  for (size_t i = 0; i < sizeof(bodies) / sizeof(bodies[0]); ++i) {
    fio_http_s *h =
        test_body_make_handle("application/json", bodies[i], strlen(bodies[i]));
    fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1&k=2"));
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_params_parse(h, .callbacks = &callbacks, .udata = &ctx);
    FIO_ASSERT(!r.err == !i && ctx.roots == 1 && TEST_BODY_PARTIAL(ctx, r),
               "fresh root #%zu: err=%d roots=%zu",
               i,
               r.err,
               ctx.roots);
    FIO_ASSERT(ctx.count == (i ? 3U : 4U),
               "fresh root #%zu: stored %zu fields",
               i,
               ctx.count);
    for (size_t p = 0; p < ctx.count; ++p)
      FIO_ASSERT(ctx.pairs[p].map == r.result,
                 "fresh root #%zu: field '%s' stored outside the result",
                 i,
                 ctx.pairs[p].key);
    test_params_expect(&ctx, "fresh root", 0, "q", "1");
    test_params_expect(&ctx, "fresh root", 1, "k", "2");
    test_params_expect(&ctx, "fresh root", 2, "j", "x");
    if (!i)
      test_params_expect(&ctx, "fresh root", 3, "n", "7");
    test_body_free_result(&r); /* the one and only root */
    fio_http_free(h);
  }
}

static void test_params_typed_errors(void) {
  FIO_ASSERT(
      FIO_HTTP_PARAMS_PARSE_SUCCESS == 0 && FIO_HTTP_PARAMS_PARSE_ERROR == -1 &&
          FIO_HTTP_PARAMS_PARSE_DEPTH == -2 && FIO_HTTP_PARAMS_PARSE_IO == -3,
      "params errors must keep simple JSON-compatible values");
  fio_http_s *h = fio_http_new();
  fio_http_query_set(h, FIO_STR_INFO1((char *)"a=1"));
  test_body_ctx_s ctx = {0};
  fio_http_params_parse_result_s r =
      fio_http_query_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
  FIO_ASSERT(r.err == FIO_HTTP_PARAMS_PARSE_SUCCESS && r.count == 1,
             "typed success: err=%d count=%zu",
             r.err,
             r.count);
  test_body_free_result(&r);
  fio_http_free(h);

  test_params_typed_json_errors();
  test_params_typed_multipart_errors();
#ifndef _WIN32
  test_params_typed_io_errors();
#endif
  test_params_callback_errors();
  test_params_json_fresh_root();
}

static void test_params_body_position(void) {
  fprintf(stderr, "  * body and params parsing preserve reading position\n");
  static const struct {
    const char *ct;
    const char *body;
    int error;
    int ignored;
  } cases[] = {
      {"application/x-www-form-urlencoded", "a=1&b=2", 0, 0},
      {"multipart/form-data; boundary=X",
       "--X\r\nContent-Disposition: form-data; name=\"a\"\r\n\r\n"
       "1\r\n--X--\r\n",
       0,
       0},
      {"multipart/form-data; boundary=X", "--X\r\ninvalid", 1, 0},
      {"application/json", "{\"a\":1}", 0, 0},
      {"application/json", "{\"a\":", 1, 0},
      {"application/json", "  [1]", 0, 1},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    for (int api = 0; api < 2; ++api) {
      if (!api && cases[i].ignored)
        continue;
      fio_http_s *h = test_body_make_handle(cases[i].ct,
                                            cases[i].body,
                                            strlen(cases[i].body));
      test_body_ctx_s ctx = {0};
      const size_t offset = 2;
      fio_http_body_seek(h, (ssize_t)offset);
      fio_http_params_parse_result_s r =
          api ? fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx)
              : fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
      FIO_ASSERT((r.err != 0) == cases[i].error,
                 "position case %zu API %d: unexpected err %d",
                 i,
                 api,
                 r.err);
      FIO_ASSERT(fio_http_body_pos(h) == offset,
                 "position case %zu API %d: moved",
                 i,
                 api);
      size_t unread_len = strlen(cases[i].body) - offset;
      if (unread_len > 4)
        unread_len = 4;
      fio_str_info_s unread = fio_http_body_read(h, unread_len);
      FIO_ASSERT(unread.len == unread_len &&
                     !memcmp(unread.buf, cases[i].body + offset, unread_len),
                 "position case %zu API %d: unread bytes differ",
                 i,
                 api);
      if (api && cases[i].ignored)
        FIO_ASSERT(ctx.count == 0 && r.consumed == 0,
                   "non-object JSON must be ignored");
      test_body_free_result(&r);
      fio_http_free(h);
    }
  }
}

static void test_params_prefix_and_closed(void) {
  fprintf(stderr, "  * JSON prefix and closed body\n");
  static const struct {
    const char *body;
    size_t len;
    int object;
  } cases[] = {
      {"\xEF\xBB\xBF \t\r\n{\"a\":1}", 14, 1},
      {" \t[1]", 5, 0},
      {"", 0, 0},
      {" \t\r\n", 4, 0},
      {"/* comment */{\"a\":1}", 20, 0},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    fio_http_s *h =
        test_body_make_handle("application/json", cases[i].body, cases[i].len);
    fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1"));
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
    FIO_ASSERT(!r.err && test_body_find(&ctx, "q") &&
                   (!!test_body_find(&ctx, "a") == cases[i].object),
               "JSON prefix case %zu",
               i);
    test_body_free_result(&r);
    fio_http_free(h);
  }
  for (int api = 0; api < 2; ++api) {
    fio_http_s *h = test_body_make_handle("application/json", "{\"a\":1}", 7);
    fio_http_query_set(h, FIO_STR_INFO1((char *)"q=1"));
    fio_http_body_close(h);
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        api ? fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx)
            : fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
    FIO_ASSERT(!r.err && r.consumed == (api ? 3U : 0U) &&
                   !fio_http_body_length(h) && !fio_http_body_pos(h) &&
                   fio_http_body_fd(h) == -1 && !fio_http_body_read(h, 1).len,
               "closed body API %d",
               api);
    if (api)
      FIO_ASSERT(test_body_find(&ctx, "q") && !test_body_find(&ctx, "a"),
                 "closed body loses query");
    else
      FIO_ASSERT(!r.result, "closed JSON body must return empty result");
    test_body_free_result(&r);
    fio_http_free(h);
  }
}

static void test_params_media_types(void) {
  fprintf(stderr, "  * params media types\n");
  const char *json[] = {"Application/JSON; charset=utf-8",
                        " application/json",
                        "application/vnd.api+json",
                        "text/json",
                        "TEXT/JSON; charset=utf-8",
                        "application/x-json"};
  for (size_t i = 0; i < sizeof(json) / sizeof(*json); ++i) {
    for (int api = 0; api < 2; ++api) {
      fio_http_s *h = test_body_make_handle(json[i], "{\"a\":1}", 7);
      test_body_ctx_s ctx = {0};
      fio_http_params_parse_result_s r =
          api ? fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx)
              : fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
      FIO_ASSERT(!r.err && r.result && ctx.pushed == 1 &&
                     TEST_BODY_PARTIAL(ctx, r),
                 "JSON type %s via %s",
                 json[i],
                 api ? "params_parse" : "body_parse");
      test_params_expect(&ctx, json[i], 0, "a", "1");
      test_body_free_result(&r);
      fio_http_free(h);
    }
  }
  {
    test_body_ctx_s ctx = {0};
    const char *ct = "APPLICATION/X-WWW-FORM-URLENCODED;charset=x";
    fio_http_params_parse_result_s r =
        test_params_run(&ctx, NULL, ct, "a=b", 3);
    FIO_ASSERT(!r.err && ctx.pushed == 1, "URL-encoded type");
    test_body_free_result(&r);
  }
  {
    test_body_ctx_s ctx = {0};
    const char *body = "--b\r\nContent-Disposition: form-data; "
                       "name=\"a\"\r\n\r\nb\r\n--b--\r\n";
    fio_http_params_parse_result_s r =
        test_params_run(&ctx,
                        NULL,
                        "multipart/form-data; BOUNDARY=\"b\"",
                        body,
                        strlen(body));
    FIO_ASSERT(!r.err && ctx.pushed == 1, "quoted uppercase boundary");
    test_body_free_result(&r);
  }
  const char *bad[] = {"application/jsonx",
                       "application/+json",
                       "text/plain",
                       "text/jsonx"};
  for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); ++i) {
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        test_params_run(&ctx, NULL, bad[i], "{\"a\":1}", 7);
    FIO_ASSERT(!r.err && r.result && !ctx.pushed && TEST_BODY_PARTIAL(ctx, r),
               "unknown type %s",
               bad[i]);
    test_body_free_result(&r);
  }
}

static void test_params_field_limit(void) {
  fprintf(stderr, "  * params per-call field limit\n");
  static char query[(FIO_HTTP_PARAMS_MAX_FIELDS + 2) * 12];
  static char body[(FIO_HTTP_PARAMS_MAX_FIELDS + 2) * 80];
  char want[32];
  size_t qlen = 0;
  query[0] = 0;
  for (size_t i = 0; i < FIO_HTTP_PARAMS_MAX_FIELDS; ++i) {
    int n = snprintf(query + qlen,
                     sizeof(query) - qlen,
                     "%sk%zu=v",
                     i ? "&" : "",
                     i);
    FIO_ASSERT(n > 0 && (size_t)n < sizeof(query) - qlen, "query buffer");
    qlen += (size_t)n;
  }
  {
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        test_params_run(&ctx, query, NULL, NULL, 0);
    snprintf(want,
             sizeof(want),
             "k%zu",
             (size_t)FIO_HTTP_PARAMS_MAX_FIELDS - 1);
    FIO_ASSERT(!r.err && TEST_BODY_PARTIAL(ctx, r) &&
                   ctx.pushed == FIO_HTTP_PARAMS_MAX_FIELDS &&
                   !strcmp(ctx.last_key, want),
               "exact query field limit");
    test_body_free_result(&r);
  }
  {
    test_body_ctx_s ctx = {0};
    FIO_MEMCPY(query + qlen, "&extra=v", 9);
    fio_http_params_parse_result_s r =
        test_params_run(&ctx, query, NULL, NULL, 0);
    FIO_ASSERT(r.err && TEST_BODY_PARTIAL(ctx, r) &&
                   ctx.pushed == FIO_HTTP_PARAMS_MAX_FIELDS,
               "standalone handle overflow reports partial map");
    test_body_free_result(&r);
  }
  query[0] = 0;
  qlen = 0;
  /* query + body share the count: 24 body fields fit (b0..b23) */
  for (size_t i = 0; i < FIO_HTTP_PARAMS_MAX_FIELDS - 24; ++i) {
    int n = snprintf(query + qlen,
                     sizeof(query) - qlen,
                     "%sk%zu=v",
                     i ? "&" : "",
                     i);
    FIO_ASSERT(n > 0 && (size_t)n < sizeof(query) - qlen, "query buffer");
    qlen += (size_t)n;
  }
  size_t blen = 0;
  for (size_t i = 0; i < 100; ++i) {
    int n =
        snprintf(body + blen, sizeof(body) - blen, "%sb%zu=v", i ? "&" : "", i);
    FIO_ASSERT(n > 0 && (size_t)n < sizeof(body) - blen, "body buffer");
    blen += (size_t)n;
  }
  {
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        test_params_run(&ctx,
                        query,
                        "application/x-www-form-urlencoded",
                        body,
                        blen);
    FIO_ASSERT(r.err && TEST_BODY_PARTIAL(ctx, r) &&
                   ctx.pushed == FIO_HTTP_PARAMS_MAX_FIELDS &&
                   !strcmp(ctx.last_key, "b23"),
               "combined field limit");
    test_body_free_result(&r);
  }
  { /* Overflow must restore the saved body reading position. */
    fio_http_s *h =
        test_body_make_handle("application/x-www-form-urlencoded", body, blen);
    fio_http_query_set(h, FIO_STR_INFO2(query, FIO_STRLEN(query)));
    fio_http_body_seek(h, 2);
    size_t position = fio_http_body_pos(h);
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
    FIO_ASSERT(r.err && TEST_BODY_PARTIAL(ctx, r) &&
                   fio_http_body_pos(h) == position,
               "overflow restores body reading position");
    test_body_free_result(&r);
    fio_http_free(h);
  }
  blen = 0;
  for (size_t i = 0; i <= FIO_HTTP_PARAMS_MAX_FIELDS; ++i) {
    int n = snprintf(body + blen,
                     sizeof(body) - blen,
                     "--b\r\nContent-Disposition: form-data; "
                     "name=\"m%zu\"%s\r\n\r\nv\r\n",
                     i,
                     i == FIO_HTTP_PARAMS_MAX_FIELDS ? "; filename=\"skip.txt\""
                                                     : "");
    FIO_ASSERT(n > 0 && (size_t)n < sizeof(body) - blen, "multipart buffer");
    blen += (size_t)n;
  }
  FIO_MEMCPY(body + blen, "--b--\r\n", 7);
  blen += 7;
  {
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        test_params_run(&ctx,
                        NULL,
                        "multipart/form-data; boundary=b",
                        body,
                        blen);
    snprintf(want,
             sizeof(want),
             "m%zu",
             (size_t)FIO_HTTP_PARAMS_MAX_FIELDS - 1);
    FIO_ASSERT(r.err && TEST_BODY_PARTIAL(ctx, r) &&
                   ctx.pushed == FIO_HTTP_PARAMS_MAX_FIELDS &&
                   !ctx.file_starts && !strcmp(ctx.last_key, want),
               "multipart file overflow");
    test_body_free_result(&r);
  }
}

static void test_params_parse_limit_count(void) {
  fprintf(stderr, "  * params parse .limit argument and result .count\n");
  fio_http_s *h = fio_http_new();
  fio_http_query_set(h, FIO_STR_INFO1((char *)"a=1&b=2&c=3&d=4"));
  { /* default limit applies, count is reported */
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_query_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
    FIO_ASSERT(!r.err && r.count == 4 && ctx.count == 4,
               "limit/count: default query parse (err=%d, count=%zu)",
               r.err,
               r.count);
    test_body_free_result(&r);
  }
  { /* .limit = 0 inherits the default */
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_query_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx, .limit = 0);
    FIO_ASSERT(!r.err && r.count == 4,
               "limit/count: .limit=0 inherits (err=%d, count=%zu)",
               r.err,
               r.count);
    test_body_free_result(&r);
  }
  { /* .limit caps the fields; count stops at the limit */
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_query_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx, .limit = 2);
    FIO_ASSERT(r.err && r.count == 2 && ctx.count == 2 &&
                   TEST_BODY_PARTIAL(ctx, r),
               "limit/count: query .limit=2 (err=%d, count=%zu, stored=%zu)",
               r.err,
               r.count,
               ctx.count);
    test_body_free_result(&r);
  }
  fio_http_free(h);

  /* budget deduction across separate query and body parses */
  h = test_body_make_handle("application/x-www-form-urlencoded", "c=3&d=4", 7);
  fio_http_query_set(h, FIO_STR_INFO1((char *)"a=1&b=2"));
  size_t budget = 3;
  {
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_query_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx, .limit = budget);
    FIO_ASSERT(!r.err && r.count == 2,
               "limit/count: query spends 2 of 3 (err=%d, count=%zu)",
               r.err,
               r.count);
    budget -= r.count;
    test_body_free_result(&r);
  }
  { /* only 1 field left: body stops after 'c' */
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r = fio_http_body_parse(h,
                                                           .callbacks =
                                                               &TEST_BODY_CALLBACKS,
                                                           .udata = &ctx,
                                                           .limit = budget);
    FIO_ASSERT(r.err && r.count == 1 && ctx.count == 1 &&
                   test_body_find(&ctx, "c") && !test_body_find(&ctx, "d"),
               "limit/count: body limited to remaining budget "
               "(err=%d, count=%zu, stored=%zu)",
               r.err,
               r.count,
               ctx.count);
    test_body_free_result(&r);
  }
  { /* a sufficient limit parses the same body fully */
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx, .limit = 2);
    FIO_ASSERT(!r.err && r.count == 2 && test_body_find(&ctx, "d"),
               "limit/count: body .limit=2 succeeds (err=%d, count=%zu)",
               r.err,
               r.count);
    test_body_free_result(&r);
  }
  fio_http_free(h);

  /* JSON bodies are not charged against the field budget */
  {
    const char *body = "{\"x\":1,\"y\":2}";
    h = test_body_make_handle("application/json", body, strlen(body));
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx, .limit = 1);
    FIO_ASSERT(!r.err && r.count == 0,
               "limit/count: JSON body not counted (err=%d, count=%zu)",
               r.err,
               r.count);
    test_body_free_result(&r);
    fio_http_free(h);
  }

  /* multipart parts, including file uploads, are charged against .limit */
  {
    const char *boundary = "limitboundary";
    char body[1024];
    size_t body_len = test_body_build_multipart(body,
                                                sizeof(body),
                                                boundary,
                                                "field1",
                                                "value1",
                                                (const char *)NULL,
                                                (const char *)NULL,
                                                "upload",
                                                "data",
                                                "f.txt",
                                                "text/plain",
                                                (const char *)NULL);
    char ct[128];
    snprintf(ct, sizeof(ct), "multipart/form-data; boundary=%s", boundary);
    h = test_body_make_handle(ct, body, body_len);
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx, .limit = 1);
    FIO_ASSERT(r.err && r.count == 1 && ctx.count == 1 && !ctx.file_starts &&
                   test_body_find(&ctx, "field1"),
               "limit/count: multipart .limit=1 (err=%d, count=%zu)",
               r.err,
               r.count);
    test_body_free_result(&r);
    fio_http_free(h);
  }

  /* fio_http_params_parse shares one .limit across query and body */
  h = test_body_make_handle("application/x-www-form-urlencoded", "c=3&d=4", 7);
  fio_http_query_set(h, FIO_STR_INFO1((char *)"a=1&b=2"));
  {
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx, .limit = 3);
    FIO_ASSERT(r.err && r.count == 3 && ctx.count == 3 &&
                   test_body_find(&ctx, "c") && !test_body_find(&ctx, "d"),
               "limit/count: combined .limit=3 (err=%d, count=%zu)",
               r.err,
               r.count);
    test_body_free_result(&r);
  }
  { /* default: combined count is reported */
    test_body_ctx_s ctx = {0};
    fio_http_params_parse_result_s r =
        fio_http_params_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
    FIO_ASSERT(!r.err && r.count == 4 && ctx.count == 4,
               "limit/count: combined default count (err=%d, count=%zu)",
               r.err,
               r.count);
    test_body_free_result(&r);
  }
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
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
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
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
  FIO_ASSERT(r.err != 0,
             "no content-type: should return error for unknown type");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_unknown_content_type(void) {
  fprintf(stderr, "  * body_parse unknown Content-Type\n");
  fio_http_s *h = test_body_make_handle("text/plain", "hello world", 11);
  test_body_ctx_s ctx = {0};
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
  FIO_ASSERT(r.err != 0,
             "unknown content-type: should return error, not crash");
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_parse_null_callbacks(void) {
  fprintf(stderr, "  * body_parse NULL callbacks\n");
  fio_http_s *h = test_body_make_handle("application/json", "{\"k\":1}", 7);
  fio_http_params_parse_result_s r = fio_http_body_parse(h, .callbacks = NULL, .udata = NULL);
  FIO_ASSERT(r.err != 0, "null callbacks: should return error");
  fio_http_free(h);
}

static void test_body_parse_null_handle(void) {
  fprintf(stderr, "  * body_parse NULL handle\n");
  test_body_ctx_s ctx = {0};
  fio_http_params_parse_result_s r =
      fio_http_body_parse(NULL, .callbacks = &TEST_BODY_CALLBACKS, .udata = &ctx);
  FIO_ASSERT(r.err != 0, "null handle: should return error");
}

/* Verify full callback payloads while they are live and again after parse
 * returns. */
typedef struct {
  const char *name;
  size_t name_len;
  const char *value;
  size_t value_len;
  size_t seen;
} test_decode_ctx_s;

static fio_json_obj_result_s test_decode_string(void *udata, fio_buf_info_s s) {
  (void)udata;
  return TEST_OBJ_R(test_obj_str_new(s.buf, s.len));
}

static fio_json_obj_result_s test_decode_escaped(void *udata,
                                                 fio_buf_info_s s) {
  (void)udata;
  fio_str_info_s decoded = FIO_STR_INFO0;
  fio_string_write_unescape(&decoded, FIO_STRING_ALLOC_COPY, s.buf, s.len);
  test_obj_s *result = test_obj_str_new(decoded.buf, decoded.len);
  FIO_STRING_FREE2(decoded);
  return TEST_OBJ_R(result);
}

static fio_json_push_result_s test_decode_map(void *udata,
                                              void *parent,
                                              fio_buf_info_s key) {
  (void)udata;
  (void)parent, (void)key;
  return TEST_PUSH_R(test_obj_sentinel_new(TEST_OBJ_MAP));
}

static fio_json_obj_result_s test_decode_set(void *udata,
                                             void *map,
                                             fio_buf_info_s key,
                                             void *value) {
  test_decode_ctx_s *ctx = (test_decode_ctx_s *)udata;
  test_obj_s *v = (test_obj_s *)value;
  FIO_ASSERT(map && v && v->type == TEST_OBJ_STRING,
             "decoded pair: unexpected callback objects");
  FIO_ASSERT(key.len == ctx->name_len &&
                 !memcmp(key.buf, ctx->name, ctx->name_len),
             "decoded key mismatch (got %zu, expected %zu)",
             key.len,
             ctx->name_len);
  FIO_ASSERT(v->u.str.len == ctx->value_len &&
                 !memcmp(v->u.str.buf, ctx->value, ctx->value_len),
             "decoded value mismatch (got %zu, expected %zu)",
             v->u.str.len,
             ctx->value_len);
  ++ctx->seen;
  test_obj_free(v);
  return TEST_OBJ_R(NULL);
}

static void test_decode_free(void *udata, void *obj) {
  (void)udata;
  test_obj_free((test_obj_s *)obj);
}

static const fio_http_params_parse_callbacks_s TEST_DECODE_CALLBACKS = {
    .on_null = test_body_on_null,
    .on_true = test_body_on_true,
    .on_false = test_body_on_false,
    .on_number = test_body_on_number,
    .on_float = test_body_on_float,
    .on_array = test_body_on_array,
    .array_push = test_body_array_push,
    .on_string = test_decode_escaped,
    .on_string_simple = test_decode_string,
    .on_map = test_decode_map,
    .map_push = test_decode_set,
    .free_unused_object = test_decode_free,
};

static void test_decode_parse(const char *content_type,
                              const char *body,
                              size_t body_len,
                              test_decode_ctx_s *ctx) {
  fio_http_s *h = test_body_make_handle(content_type, body, body_len);
  FIO_ASSERT(h, "decode regression: handle allocation failed");
  fio_http_params_parse_result_s r =
      fio_http_body_parse(h, .callbacks = &TEST_DECODE_CALLBACKS, .udata = ctx);
  FIO_ASSERT(!r.err && ctx->seen == 1,
             "decode regression: parse err=%d pairs=%zu",
             r.err,
             ctx->seen);
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_decode_boundaries(void) {
  fprintf(stderr,
          "  * JSON escape and URL form decode allocation boundaries\n");
  static const size_t json_sizes[] = {4094, 4095, 4096, 4097, 8192};
  static const size_t form_name_sizes[] = {1023, 1024, 1025, 2048};
  static const size_t form_value_sizes[] = {4095, 4096, 4097, 8192};
  for (size_t i = 0; i < sizeof(json_sizes) / sizeof(*json_sizes); ++i) {
    size_t n = json_sizes[i];
    char *expected = (char *)malloc(n + 1);
    char *body = (char *)malloc(n + 32);
    FIO_ASSERT(expected && body, "JSON fixture allocation failed");
    memset(expected, 'a', n);
    expected[n - 1] = '\n';
    expected[n] = 0;
    /* Escape crosses the 4095-byte decoded-buffer boundary at n=4096. */
    memcpy(body, "{\"k\":\"", 6);
    memset(body + 6, 'a', n - 1);
    memcpy(body + 6 + n - 1, "\\n\"}", 4);
    test_decode_ctx_s ctx = {"k", 1, expected, n, 0};
    test_decode_parse("application/json", body, n + 9, &ctx);
    /* Also test an escaped key with a large decoded value. */
    memcpy(body, "{\"\\u006b\":\"", 11);
    memset(body + 11, 'a', n - 1);
    memcpy(body + 11 + n - 1, "\\n\"}", 4);
    ctx.seen = 0;
    test_decode_parse("application/json", body, n + 14, &ctx);
    free(body);
    free(expected);
  }
  /* Test-only isolation: let ASan reach JSON cases without the known form OOB.
   */
  if (getenv("FIO_TEST_JSON_ONLY"))
    return;
  for (size_t i = 0; i < sizeof(form_name_sizes) / sizeof(*form_name_sizes);
       ++i) {
    size_t nl = form_name_sizes[i];
    size_t vl = form_value_sizes[i];
    char *name = (char *)malloc(nl + 1);
    char *value = (char *)malloc(vl + 1);
    char *body = (char *)malloc(nl + vl + 16);
    FIO_ASSERT(name && value && body, "form fixture allocation failed");
    memset(name, 'n', nl);
    memset(value, 'v', vl);
    name[0] = 'A';
    value[0] = 'B';
    name[nl] = value[vl] = 0;
    memcpy(body, "%41", 3);
    memset(body + 3, 'n', nl - 1);
    body[nl + 2] = '=';
    memcpy(body + nl + 3, "%42", 3);
    memset(body + nl + 6, 'v', vl - 1);
    test_decode_ctx_s ctx = {name, nl, value, vl, 0};
    test_decode_parse("application/x-www-form-urlencoded",
                      body,
                      nl + vl + 5,
                      &ctx);
    free(body);
    free(value);
    free(name);
  }
}

/* Concurrent escaped JSON exercises the shared static allocator's ownership. */
static void *test_decode_thread(void *arg) {
  size_t id = (size_t)(uintptr_t)arg;
  char *expected = (char *)malloc(5001);
  char *body = (char *)malloc(5016);
  FIO_ASSERT(expected && body, "concurrent fixture allocation failed");
  memset(expected, (int)('a' + id), 4999);
  expected[4999] = '\n';
  expected[5000] = 0;
  memcpy(body, "{\"k\":\"", 6);
  memset(body + 6, (int)('a' + id), 4999);
  memcpy(body + 5005, "\\n\"}", 4);
  for (size_t i = 0; i < 1; ++i) {
    test_decode_ctx_s ctx = {"k", 1, expected, 5000, 0};
    test_decode_parse("application/json", body, 5009, &ctx);
  }
  free(body);
  free(expected);
  return NULL;
}

static void test_body_decode_concurrent(void) {
  fprintf(stderr, "  * concurrent escaped JSON decode\n");
  fio_thread_t threads[8];
  for (size_t i = 0; i < 8; ++i)
    FIO_ASSERT(!fio_thread_create(&threads[i],
                                  test_decode_thread,
                                  (void *)(uintptr_t)i),
               "decode thread creation failed");
  for (size_t i = 0; i < 8; ++i)
    FIO_ASSERT(!fio_thread_join(&threads[i]), "decode thread join failed");
}

/* ===========================================================================
   T008 — Accept-Encoding q-values and dynamic compression guards
   ===========================================================================
 */

static void test_accept_encoding_qvalues(void) {
  fprintf(stderr, "  * accept-encoding q-value handling\n");
  /* q=0 (in any zero-padded form) FORBIDS the encoding (RFC 9110 §12.5.3). */
  FIO_ASSERT(!fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;q=0", 8),
                                          "gzip",
                                          4),
             "gzip;q=0 must not match gzip");
  FIO_ASSERT(
      !fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;q=0.0", 10),
                                   "gzip",
                                   4),
      "gzip;q=0.0 must not match gzip");
  FIO_ASSERT(
      !fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;q=0.00", 11),
                                   "gzip",
                                   4),
      "gzip;q=0.00 must not match gzip");
  FIO_ASSERT(
      !fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;q=0.000", 12),
                                   "gzip",
                                   4),
      "gzip;q=0.000 must not match gzip");
  FIO_ASSERT(
      !fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip ; q=0", 10),
                                   "gzip",
                                   4),
      "gzip ; q=0 (whitespace) must not match gzip");
  FIO_ASSERT(!fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;Q=0", 8),
                                          "gzip",
                                          4),
             "gzip;Q=0 (uppercase) must not match gzip");
  /* any q > 0 still matches */
  FIO_ASSERT(
      fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;q=0.001", 12),
                                  "gzip",
                                  4),
      "gzip;q=0.001 should match gzip");
  FIO_ASSERT(
      fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;q=0.5", 10),
                                  "gzip",
                                  4),
      "gzip;q=0.5 should match gzip");
  FIO_ASSERT(fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;q=1", 8),
                                         "gzip",
                                         4),
             "gzip;q=1 should match gzip");
  FIO_ASSERT(
      fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip", 4), "gzip", 4),
      "bare gzip should match gzip");
  /* multi-value headers: q=0 forbids only its own token */
  FIO_ASSERT(
      fio___http_header_has_token(FIO_STR_INFO2((char *)"br, gzip;q=0", 12),
                                  "br",
                                  2),
      "br should match in 'br, gzip;q=0'");
  FIO_ASSERT(
      !fio___http_header_has_token(FIO_STR_INFO2((char *)"br, gzip;q=0", 12),
                                   "gzip",
                                   4),
      "gzip must not match in 'br, gzip;q=0'");
  /* unrelated parameters after a valid token are not q-values */
  FIO_ASSERT(
      fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;foo=bar", 12),
                                  "gzip",
                                  4),
      "gzip;foo=bar should match gzip (not a q-value)");
}

/** Writes `len` text bytes with finish=1 on a COMPRESS_DYNAMIC handle whose
 * request carries `accept_encoding`; returns the response content-encoding
 * header state for assertions. */
static fio_str_info_s test_dynamic_write(fio_http_s *h,
                                         const char *accept_encoding,
                                         const void *body,
                                         size_t len) {
  fio___http_test_enable_compression(h, 0);
  fio_http_method_set(h, FIO_STR_INFO1((char *)"GET"));
  fio_http_status_set(h, 200);
  if (accept_encoding)
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                FIO_STR_INFO1((char *)accept_encoding));
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"content-type", 12),
                               FIO_STR_INFO1((char *)"text/plain"));
  fio_http_write(h, .buf = (void *)body, .len = len, .finish = 1, .copy = 1);
  return fio_http_response_header(h,
                                  FIO_STR_INFO2((char *)"content-encoding", 16),
                                  0);
}

/** Vary semantics (RFC 9110 §12.5.5): a compressible resource negotiates
 * on Accept-Encoding, so even identity responses must carry
 * `Vary: accept-encoding` — including requests without the header (a
 * gzip-capable request would have received a different representation).
 * Conversely, Vary must NOT appear when negotiation is impossible. */
static void test_dynamic_vary_semantics(void) {
  fprintf(stderr, "  * dynamic compression Vary semantics\n");
  enum { BODY_LEN = 2048 };
  char body[BODY_LEN];
  for (size_t i = 0; i < BODY_LEN; ++i)
    body[i] = (char)('a' + (i & 15)); /* compressible text */

  /* no Accept-Encoding request header: identity body, but Vary required */
  {
    fio_http_s *h = fio_http_new();
    fio_str_info_s ce = test_dynamic_write(h, NULL, body, BODY_LEN);
    FIO_ASSERT(!ce.buf,
               "dynamic no-AE: response must be identity (got '%.*s')",
               (int)ce.len,
               ce.buf ? ce.buf : "");
    fio_str_info_s vary =
        fio_http_response_header(h, FIO_STR_INFO2((char *)"vary", 4), 0);
    FIO_ASSERT(vary.len &&
                   fio___http_header_has_token(vary, "accept-encoding", 15),
               "dynamic no-AE: Vary: accept-encoding required (selection "
               "depends on Accept-Encoding even when the header is absent)");
    fio_http_free(h);
  }
  /* incompressible MIME: negotiation impossible, Vary must NOT appear */
  {
    fio_http_s *h = fio_http_new();
    fio___http_test_enable_compression(h, 0);
    fio_http_method_set(h, FIO_STR_INFO1((char *)"GET"));
    fio_http_status_set(h, 200);
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                FIO_STR_INFO1((char *)"gzip"));
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"content-type", 12),
                                 FIO_STR_INFO1((char *)"image/png"));
    fio_http_write(h,
                   .buf = (void *)body,
                   .len = BODY_LEN,
                   .finish = 1,
                   .copy = 1);
    fio_str_info_s ce =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-encoding", 16),
                                 0);
    FIO_ASSERT(!ce.buf,
               "dynamic png: must stay identity (incompressible MIME)");
    fio_str_info_s vary =
        fio_http_response_header(h, FIO_STR_INFO2((char *)"vary", 4), 0);
    FIO_ASSERT(!vary.buf,
               "dynamic png: Vary must be absent when negotiation is "
               "impossible (got '%.*s')",
               (int)vary.len,
               vary.buf ? vary.buf : "");
    fio_http_free(h);
  }
}

static void test_dynamic_compress_guards(void) {
  fprintf(stderr, "  * dynamic compression guards\n");
  enum { BODY_LEN = 2048 };
  char body[BODY_LEN];
  for (size_t i = 0; i < BODY_LEN; ++i)
    body[i] = (char)('a' + (i & 15)); /* compressible text */

  /* sanity: plain gzip offer compresses */
  {
    fio_http_s *h = fio_http_new();
    fio_str_info_s ce = test_dynamic_write(h, "gzip", body, BODY_LEN);
    FIO_ASSERT(ce.len == 4 && !memcmp(ce.buf, "gzip", 4),
               "dynamic gzip offer: expected content-encoding gzip");
    fio_http_free(h);
  }
  /* q=0 forbids gzip → identity */
  {
    fio_http_s *h = fio_http_new();
    fio_str_info_s ce = test_dynamic_write(h, "gzip;q=0", body, BODY_LEN);
    FIO_ASSERT(!ce.buf,
               "dynamic gzip;q=0: response must not be compressed "
               "(content-encoding present: %.*s)",
               (int)ce.len,
               ce.buf ? ce.buf : "");
    fio_str_info_s vary =
        fio_http_response_header(h, FIO_STR_INFO2((char *)"vary", 4), 0);
    FIO_ASSERT(vary.len,
               "dynamic gzip;q=0: Vary: accept-encoding expected when "
               "compression was evaluated but skipped");
    fio_http_free(h);
  }
  /* all acceptable encodings forbidden → identity */
  {
    fio_http_s *h = fio_http_new();
    fio_str_info_s ce =
        test_dynamic_write(h, "br;q=0, gzip;q=0", body, BODY_LEN);
    FIO_ASSERT(!ce.buf,
               "dynamic br;q=0, gzip;q=0: response must not be compressed");
    fio_http_free(h);
  }
  /* app already set content-encoding → dynamic path must not double-compress */
  {
    fio_http_s *h = fio_http_new();
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"content-encoding", 16),
                                 FIO_STR_INFO1((char *)"identity"));
    fio_str_info_s ce = test_dynamic_write(h, "gzip, br", body, BODY_LEN);
    FIO_ASSERT(ce.len == 8 && !memcmp(ce.buf, "identity", 8),
               "dynamic preset content-encoding: must stay 'identity', got "
               "'%.*s' (double compression)",
               (int)ce.len,
               ce.buf ? ce.buf : "");
    fio_str_info_s cl =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-length", 14),
                                 0);
    char expect[16];
    int expect_len = snprintf(expect, sizeof(expect), "%u", (unsigned)BODY_LEN);
    FIO_ASSERT(cl.len == (size_t)expect_len &&
                   !memcmp(cl.buf, expect, (size_t)expect_len),
               "dynamic preset content-encoding: body must pass through "
               "unmodified (content-length %.*s)",
               (int)cl.len,
               cl.buf ? cl.buf : "");
    fio_http_free(h);
  }
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
  test_handle_header_rejection();
  test_handle_header_name_policy();
  test_handle_header_name_length();
  test_handle_header_removal();
  test_handle_should_close();
  test_handle_should_close_long_list();
  test_handle_should_close_http10();
  test_handle_body();
  test_handle_body_file_spill();
#if FIO_OS_POSIX
  test_handle_body_save();
#endif
#if FIO_OS_POSIX
  test_handle_body_write_failure();
#endif
  test_handle_cookies();
  test_handle_path_sections();
  test_handle_helpers();
  test_handle_mimetype_length();
#if FIO_OS_POSIX
  test_handle_static_long_mimetype();
#endif
  test_handle_set_if_missing_empty();
  test_handle_etag_is_match();
  test_handle_upgrade_websocket_version();
  test_handle_connect_timeout_range();
  test_handle_path_each_trailing_escape();
  test_handle_from_forwarded();
  test_handle_is_clean_after_headers();
#if FIO_OS_POSIX
  test_handle_write_fd_copy();
#endif
  test_handle_websocket_request();
  test_handle_sse_request();

  fprintf(stderr, "\nTesting fio_http_body_parse correctness:\n");
  test_body_parse_urlencoded_simple();
  test_body_parse_urlencoded_encoded();
  test_body_parse_urlencoded_empty_value();
  test_body_parse_urlencoded_multi_value();
  test_query_parse();
  test_params_parse();
  test_params_depth();
  test_params_typed_errors();
  test_params_body_position();
  test_params_prefix_and_closed();
  test_params_media_types();
  test_params_field_limit();
  test_params_parse_limit_count();
  test_body_parse_multipart_skip_file();
  test_body_parse_multipart_stream_large();
  test_body_parse_multipart_framing();
  test_body_parse_json_simple();
  test_body_parse_spilled();
  test_body_parse_json_array();
  test_body_parse_multipart_simple_field();
  test_body_parse_multipart_file_upload();
  test_body_parse_multipart_upload_rules();
  test_body_parse_empty_body();
  test_body_parse_no_content_type();
  test_body_parse_unknown_content_type();
  test_body_parse_null_callbacks();
  test_body_parse_null_handle();
  test_body_decode_boundaries();
  test_body_decode_concurrent();

  fprintf(stderr, "\nTesting accept-encoding / dynamic compression:\n");
  test_accept_encoding_qvalues();
  test_dynamic_vary_semantics();
  test_dynamic_compress_guards();

  fprintf(stderr, "\nAll HTTP handle tests passed!\n");
  return 0;
}
