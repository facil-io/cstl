/* *****************************************************************************
MIME Multipart Parser Tests
***************************************************************************** */
#define FIO_LOG
#define FIO_MULTIPART
#include "fio-stl.h"

#include <stdio.h>
#include <string.h>

/* *****************************************************************************
Test Callbacks
***************************************************************************** */

typedef struct {
  size_t field_count;
  size_t file_count;
  size_t file_data_bytes;
  char last_field_name[256];
  char last_field_value[256];
  char last_filename[256];
} test_multipart_ctx_s;

static void *test_on_field(void *udata,
                           fio_buf_info_s name,
                           fio_buf_info_s value,
                           fio_buf_info_s content_type) {
  test_multipart_ctx_s *ctx = (test_multipart_ctx_s *)udata;
  ++ctx->field_count;

  /* Store last field for verification */
  if (name.len < sizeof(ctx->last_field_name)) {
    FIO_MEMCPY(ctx->last_field_name, name.buf, name.len);
    ctx->last_field_name[name.len] = '\0';
  }
  if (value.len < sizeof(ctx->last_field_value)) {
    FIO_MEMCPY(ctx->last_field_value, value.buf, value.len);
    ctx->last_field_value[value.len] = '\0';
  }

  (void)content_type;
  return NULL;
}

static void *test_on_file_start(void *udata,
                                fio_buf_info_s name,
                                fio_buf_info_s filename,
                                fio_buf_info_s content_type) {
  test_multipart_ctx_s *ctx = (test_multipart_ctx_s *)udata;
  ++ctx->file_count;

  /* Store filename for verification */
  if (filename.len < sizeof(ctx->last_filename)) {
    FIO_MEMCPY(ctx->last_filename, filename.buf, filename.len);
    ctx->last_filename[filename.len] = '\0';
  }

  (void)name;
  (void)content_type;
  return ctx; /* Return context for file data */
}

static int test_on_file_data(void *udata, void *file_ctx, fio_buf_info_s data) {
  test_multipart_ctx_s *ctx = (test_multipart_ctx_s *)udata;
  ctx->file_data_bytes += data.len;
  (void)file_ctx;
  return 0;
}

static void test_on_file_end(void *udata, void *file_ctx) {
  (void)udata;
  (void)file_ctx;
}

static void test_on_error(void *udata) { (void)udata; }

static const fio_multipart_parser_callbacks_s test_callbacks = {
    .on_field = test_on_field,
    .on_file_start = test_on_file_start,
    .on_file_data = test_on_file_data,
    .on_file_end = test_on_file_end,
    .on_error = test_on_error,
};

/* *****************************************************************************
Test Cases
***************************************************************************** */

FIO_SFUNC void fio___test_multipart_basic(void) {
  fprintf(stderr, "* Testing basic multipart parsing...\n");

  test_multipart_ctx_s ctx = {0};

  const char *boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
  const char *data = "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                     "Content-Disposition: form-data; name=\"field1\"\r\n"
                     "\r\n"
                     "value1\r\n"
                     "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                     "Content-Disposition: form-data; name=\"field2\"\r\n"
                     "\r\n"
                     "value2\r\n"
                     "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&test_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Basic multipart parse should succeed");
  FIO_ASSERT(result.field_count == 2,
             "Should have 2 fields, got %zu",
             result.field_count);
  FIO_ASSERT(result.file_count == 0,
             "Should have 0 files, got %zu",
             result.file_count);
  FIO_ASSERT(ctx.field_count == 2, "Callback should see 2 fields");
}

FIO_SFUNC void fio___test_multipart_file_upload(void) {
  fprintf(stderr, "* Testing multipart file upload...\n");

  test_multipart_ctx_s ctx = {0};

  const char *boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
  const char *data = "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                     "Content-Disposition: form-data; name=\"field1\"\r\n"
                     "\r\n"
                     "value1\r\n"
                     "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                     "Content-Disposition: form-data; name=\"file1\"; "
                     "filename=\"test.txt\"\r\n"
                     "Content-Type: text/plain\r\n"
                     "\r\n"
                     "Hello, World!\r\n"
                     "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&test_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "File upload parse should succeed");
  FIO_ASSERT(result.field_count == 1,
             "Should have 1 field, got %zu",
             result.field_count);
  FIO_ASSERT(result.file_count == 1,
             "Should have 1 file, got %zu",
             result.file_count);
  FIO_ASSERT(ctx.file_data_bytes == 13,
             "File should have 13 bytes, got %zu",
             ctx.file_data_bytes);
  FIO_ASSERT(FIO_MEMCMP(ctx.last_filename, "test.txt", 8) == 0,
             "Filename should be 'test.txt', got '%s'",
             ctx.last_filename);
}

FIO_SFUNC void fio___test_multipart_empty(void) {
  fprintf(stderr, "* Testing empty multipart...\n");

  test_multipart_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data = "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&test_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Empty multipart parse should succeed");
  FIO_ASSERT(result.field_count == 0, "Should have 0 fields");
  FIO_ASSERT(result.file_count == 0, "Should have 0 files");
}

FIO_SFUNC void fio___test_multipart_quoted_filename(void) {
  fprintf(stderr, "* Testing quoted filename with special chars...\n");

  test_multipart_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data = "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"file\"; "
                     "filename=\"my file.txt\"\r\n"
                     "Content-Type: text/plain\r\n"
                     "\r\n"
                     "content\r\n"
                     "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&test_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Quoted filename parse should succeed");
  FIO_ASSERT(result.file_count == 1, "Should have 1 file");
  FIO_ASSERT(FIO_MEMCMP(ctx.last_filename, "my file.txt", 11) == 0,
             "Filename should be 'my file.txt', got '%s'",
             ctx.last_filename);
}

FIO_SFUNC void fio___test_multipart_need_more_data(void) {
  fprintf(stderr, "* Testing partial data (need more)...\n");

  test_multipart_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  /* Incomplete data - missing closing boundary */
  const char *data = "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field1\"\r\n"
                     "\r\n"
                     "value1";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&test_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == -2, "Partial data should return -2 (need more)");
}

FIO_SFUNC void fio___test_multipart_multiple_files(void) {
  fprintf(stderr, "* Testing multiple file uploads...\n");

  test_multipart_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data =
      "------boundary\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "File A content\r\n"
      "------boundary\r\n"
      "Content-Disposition: form-data; name=\"file2\"; filename=\"b.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "File B content\r\n"
      "------boundary\r\n"
      "Content-Disposition: form-data; name=\"file3\"; filename=\"c.txt\"\r\n"
      "Content-Type: application/octet-stream\r\n"
      "\r\n"
      "File C content\r\n"
      "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&test_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Multiple files parse should succeed");
  FIO_ASSERT(result.file_count == 3,
             "Should have 3 files, got %zu",
             result.file_count);
  FIO_ASSERT(result.field_count == 0, "Should have 0 fields");
}

FIO_SFUNC void fio___test_multipart_mixed_content(void) {
  fprintf(stderr, "* Testing mixed fields and files...\n");

  test_multipart_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data =
      "------boundary\r\n"
      "Content-Disposition: form-data; name=\"username\"\r\n"
      "\r\n"
      "john_doe\r\n"
      "------boundary\r\n"
      "Content-Disposition: form-data; name=\"avatar\"; "
      "filename=\"photo.jpg\"\r\n"
      "Content-Type: image/jpeg\r\n"
      "\r\n"
      "JPEG_DATA_HERE\r\n"
      "------boundary\r\n"
      "Content-Disposition: form-data; name=\"email\"\r\n"
      "\r\n"
      "john@example.com\r\n"
      "------boundary\r\n"
      "Content-Disposition: form-data; name=\"resume\"; filename=\"cv.pdf\"\r\n"
      "Content-Type: application/pdf\r\n"
      "\r\n"
      "PDF_DATA_HERE\r\n"
      "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&test_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Mixed content parse should succeed");
  FIO_ASSERT(result.field_count == 2,
             "Should have 2 fields, got %zu",
             result.field_count);
  FIO_ASSERT(result.file_count == 2,
             "Should have 2 files, got %zu",
             result.file_count);
}

FIO_SFUNC void fio___test_multipart_with_crlf_prefix(void) {
  fprintf(stderr, "* Testing multipart with CRLF prefix...\n");

  test_multipart_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  /* Some implementations send CRLF before the first boundary */
  const char *data = "\r\n------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field1\"\r\n"
                     "\r\n"
                     "value1\r\n"
                     "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&test_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "CRLF prefix parse should succeed");
  FIO_ASSERT(result.field_count == 1, "Should have 1 field");
}

/* *****************************************************************************
Main Test Entry Point
***************************************************************************** */

void fio_test_multipart(void) {
  fprintf(stderr, "=== Testing MIME Multipart Parser ===\n");

  fio___test_multipart_basic();
  fio___test_multipart_file_upload();
  fio___test_multipart_empty();
  fio___test_multipart_quoted_filename();
  fio___test_multipart_need_more_data();
  fio___test_multipart_multiple_files();
  fio___test_multipart_mixed_content();
  fio___test_multipart_with_crlf_prefix();

  fprintf(stderr, "=== MIME Multipart Parser Tests PASSED ===\n\n");
}

#ifndef FIO_TEST_ALL
int main(void) {
  fio_test_multipart();
  return 0;
}
#endif
