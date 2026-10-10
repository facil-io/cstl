/* *****************************************************************************
Test: HTTP handle correctness
***************************************************************************** */
#define FIO_HTTP
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
  fprintf(stderr, "  * null/empty header inputs preserve removal semantics\n");
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
        FIO_ASSERT(!r.buf && !r.len, "empty/null value should return empty");
        if (op == 1) { /* add: no-op, not removal */
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
      {"HTTP/0.9", NULL, 0, 0}, /* not legacy: HTTP/1.1 semantics */
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
    if (cases[i].streaming)
      h->state |= FIO_HTTP_STATE_STREAMING;
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
    h->state &= ~FIO_HTTP_STATE_STREAMING;
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

/* Verify full callback payloads while they are live and again after parse returns. */
typedef struct {
  const char *name;
  size_t name_len;
  const char *value;
  size_t value_len;
  size_t seen;
} test_decode_ctx_s;

static void *test_decode_string(void *udata, const void *data, size_t len) {
  (void)udata;
  return test_obj_str_new(data, len);
}

static void *test_decode_map(void *udata, void *parent) {
  (void)udata;
  (void)parent;
  return test_obj_sentinel_new(TEST_OBJ_MAP);
}

static int test_decode_set(void *udata, void *map, void *key, void *value) {
  test_decode_ctx_s *ctx = (test_decode_ctx_s *)udata;
  test_obj_s *k = (test_obj_s *)key;
  test_obj_s *v = (test_obj_s *)value;
  FIO_ASSERT(map && k && v && k->type == TEST_OBJ_STRING &&
                 v->type == TEST_OBJ_STRING,
             "decoded pair: unexpected callback objects");
  FIO_ASSERT(k->u.str.len == ctx->name_len &&
                 !memcmp(k->u.str.buf, ctx->name, ctx->name_len),
             "decoded key mismatch (got %zu, expected %zu)",
             k->u.str.len, ctx->name_len);
  FIO_ASSERT(v->u.str.len == ctx->value_len &&
                 !memcmp(v->u.str.buf, ctx->value, ctx->value_len),
             "decoded value mismatch (got %zu, expected %zu)",
             v->u.str.len, ctx->value_len);
  ++ctx->seen;
  test_obj_free(k);
  test_obj_free(v);
  return 0;
}

static void *test_decode_error(void *udata, void *partial) {
  (void)udata;
  test_obj_free((test_obj_s *)partial);
  return NULL;
}

static const fio_http_body_parse_callbacks_s TEST_DECODE_CALLBACKS = {
    .on_string = test_decode_string,
    .on_map = test_decode_map,
    .map_set = test_decode_set,
    .map_done = test_body_map_done,
    .on_error = test_decode_error,
    .free_unused = test_body_free_unused,
};

static void test_decode_parse(const char *content_type,
                              const char *body,
                              size_t body_len,
                              test_decode_ctx_s *ctx) {
  fio_http_s *h = test_body_make_handle(content_type, body, body_len);
  FIO_ASSERT(h, "decode regression: handle allocation failed");
  fio_http_body_parse_result_s r =
      fio_http_body_parse(h, &TEST_DECODE_CALLBACKS, ctx);
  FIO_ASSERT(!r.err && ctx->seen == 1,
             "decode regression: parse err=%d pairs=%zu", r.err, ctx->seen);
  test_body_free_result(&r);
  fio_http_free(h);
}

static void test_body_decode_boundaries(void) {
  fprintf(stderr, "  * JSON escape and URL form decode allocation boundaries\n");
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
  /* Test-only isolation: let ASan reach JSON cases without the known form OOB. */
  if (getenv("FIO_TEST_JSON_ONLY"))
    return;
  for (size_t i = 0; i < sizeof(form_name_sizes) / sizeof(*form_name_sizes); ++i) {
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
                      body, nl + vl + 5, &ctx);
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
    FIO_ASSERT(!fio_thread_create(&threads[i], test_decode_thread,
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
  FIO_ASSERT(
      !fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;q=0", 8),
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
  FIO_ASSERT(
      !fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;Q=0", 8),
                                   "gzip",
                                   4),
      "gzip;Q=0 (uppercase) must not match gzip");
  /* any q > 0 still matches */
  FIO_ASSERT(
      fio___http_header_has_token(FIO_STR_INFO2((char *)"gzip;q=0.001", 12),
                                  "gzip",
                                  4),
      "gzip;q=0.001 should match gzip");
  FIO_ASSERT(fio___http_header_has_token(
                 FIO_STR_INFO2((char *)"gzip;q=0.5", 10),
                 "gzip",
                 4),
             "gzip;q=0.5 should match gzip");
  FIO_ASSERT(fio___http_header_has_token(
                 FIO_STR_INFO2((char *)"gzip;q=1", 8),
                 "gzip",
                 4),
             "gzip;q=1 should match gzip");
  FIO_ASSERT(fio___http_header_has_token(
                 FIO_STR_INFO2((char *)"gzip", 4),
                 "gzip",
                 4),
             "bare gzip should match gzip");
  /* multi-value headers: q=0 forbids only its own token */
  FIO_ASSERT(fio___http_header_has_token(
                 FIO_STR_INFO2((char *)"br, gzip;q=0", 12),
                 "br",
                 2),
             "br should match in 'br, gzip;q=0'");
  FIO_ASSERT(!fio___http_header_has_token(
                 FIO_STR_INFO2((char *)"br, gzip;q=0", 12),
                 "gzip",
                 4),
             "gzip must not match in 'br, gzip;q=0'");
  /* unrelated parameters after a valid token are not q-values */
  FIO_ASSERT(fio___http_header_has_token(
                 FIO_STR_INFO2((char *)"gzip;foo=bar", 12),
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
  fio_http_cflags_set(h, FIO_HTTP_CFLAG_COMPRESS_DYNAMIC);
  fio_http_method_set(h, FIO_STR_INFO1((char *)"GET"));
  fio_http_status_set(h, 200);
  if (accept_encoding)
    fio_http_request_header_set(
        h,
        FIO_STR_INFO2((char *)"accept-encoding", 15),
        FIO_STR_INFO1((char *)accept_encoding));
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"content-type", 12),
                               FIO_STR_INFO1((char *)"text/plain"));
  fio_http_write(h, .buf = (void *)body, .len = len, .finish = 1, .copy = 1);
  return fio_http_response_header(h,
                                  FIO_STR_INFO2((char *)"content-encoding",
                                                16),
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
    fio_str_info_s vary = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"vary", 4),
        0);
    FIO_ASSERT(vary.len && fio___http_header_has_token(vary,
                                                        "accept-encoding",
                                                        15),
               "dynamic no-AE: Vary: accept-encoding required (selection "
               "depends on Accept-Encoding even when the header is absent)");
    fio_http_free(h);
  }
  /* incompressible MIME: negotiation impossible, Vary must NOT appear */
  {
    fio_http_s *h = fio_http_new();
    fio_http_cflags_set(h, FIO_HTTP_CFLAG_COMPRESS_DYNAMIC);
    fio_http_method_set(h, FIO_STR_INFO1((char *)"GET"));
    fio_http_status_set(h, 200);
    fio_http_request_header_set(
        h,
        FIO_STR_INFO2((char *)"accept-encoding", 15),
        FIO_STR_INFO1((char *)"gzip"));
    fio_http_response_header_set(h,
                                 FIO_STR_INFO2((char *)"content-type", 12),
                                 FIO_STR_INFO1((char *)"image/png"));
    fio_http_write(h, .buf = (void *)body, .len = BODY_LEN, .finish = 1,
                   .copy = 1);
    fio_str_info_s ce = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"content-encoding", 16),
        0);
    FIO_ASSERT(!ce.buf,
               "dynamic png: must stay identity (incompressible MIME)");
    fio_str_info_s vary = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"vary", 4),
        0);
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
    fio_str_info_s vary = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"vary", 4),
        0);
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
                                 FIO_STR_INFO2((char *)"content-encoding",
                                               16),
                                 FIO_STR_INFO1((char *)"identity"));
    fio_str_info_s ce = test_dynamic_write(h, "gzip, br", body, BODY_LEN);
    FIO_ASSERT(ce.len == 8 && !memcmp(ce.buf, "identity", 8),
               "dynamic preset content-encoding: must stay 'identity', got "
               "'%.*s' (double compression)",
               (int)ce.len,
               ce.buf ? ce.buf : "");
    fio_str_info_s cl = fio_http_response_header(
        h,
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
  test_body_decode_boundaries();
  test_body_decode_concurrent();

  fprintf(stderr, "\nTesting accept-encoding / dynamic compression:\n");
  test_accept_encoding_qvalues();
  test_dynamic_vary_semantics();
  test_dynamic_compress_guards();

  fprintf(stderr, "\nAll HTTP handle tests passed!\n");
  return 0;
}
