/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        fio_http_s Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_HTTP_HANDLE_TEST___H)
#define H___FIO_HTTP_HANDLE_TEST___H
// #ifndef H___FIO_MODULE_NAME___H
// #define FIO_MODULE_NAME
// #define FIO___TEST_REINCLUDE
// #include FIO_INCLUDE_FILE
// #undef FIO___TEST_REINCLUDE
// #endif

FIO_SFUNC void FIO_NAME_TEST(stl, http_s)(void) {
  fprintf(stderr, "* Testing HTTP handle (fio_http_s).\n");
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
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
