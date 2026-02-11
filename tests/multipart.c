/* *****************************************************************************
MIME Multipart Parser Tests
***************************************************************************** */
#define FIO_LOG
#define FIO_MULTIPART
#include "fio-stl/include.h"

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
Streaming Test Context - tracks multiple on_file_data calls
***************************************************************************** */

typedef struct {
  size_t field_count;
  size_t file_count;
  size_t file_data_calls;      /* Number of on_file_data calls */
  size_t file_data_bytes;      /* Total bytes received */
  size_t file_start_calls;     /* Number of on_file_start calls */
  size_t file_end_calls;       /* Number of on_file_end calls */
  char accumulated_data[4096]; /* Accumulated file data */
  size_t accumulated_len;
} streaming_test_ctx_s;

static void *streaming_on_field(void *udata,
                                fio_buf_info_s name,
                                fio_buf_info_s value,
                                fio_buf_info_s content_type) {
  streaming_test_ctx_s *ctx = (streaming_test_ctx_s *)udata;
  ++ctx->field_count;
  (void)name;
  (void)value;
  (void)content_type;
  return NULL;
}

static void *streaming_on_file_start(void *udata,
                                     fio_buf_info_s name,
                                     fio_buf_info_s filename,
                                     fio_buf_info_s content_type) {
  streaming_test_ctx_s *ctx = (streaming_test_ctx_s *)udata;
  ++ctx->file_count;
  ++ctx->file_start_calls;
  (void)name;
  (void)filename;
  (void)content_type;
  return ctx;
}

static int streaming_on_file_data(void *udata,
                                  void *file_ctx,
                                  fio_buf_info_s data) {
  streaming_test_ctx_s *ctx = (streaming_test_ctx_s *)udata;
  ++ctx->file_data_calls;
  ctx->file_data_bytes += data.len;

  /* Accumulate data for verification */
  if (ctx->accumulated_len + data.len < sizeof(ctx->accumulated_data)) {
    FIO_MEMCPY(ctx->accumulated_data + ctx->accumulated_len,
               data.buf,
               data.len);
    ctx->accumulated_len += data.len;
  }

  (void)file_ctx;
  return 0;
}

static void streaming_on_file_end(void *udata, void *file_ctx) {
  streaming_test_ctx_s *ctx = (streaming_test_ctx_s *)udata;
  ++ctx->file_end_calls;
  (void)file_ctx;
}

static const fio_multipart_parser_callbacks_s streaming_callbacks = {
    .on_field = streaming_on_field,
    .on_file_start = streaming_on_file_start,
    .on_file_data = streaming_on_file_data,
    .on_file_end = streaming_on_file_end,
    .on_error = test_on_error,
};

/* *****************************************************************************
Streaming Tests
***************************************************************************** */

/**
 * Test: Parse complete multipart in multiple chunks
 *
 * This tests the basic streaming scenario where we receive data in chunks
 * and call fio_multipart_parse() multiple times, using result.consumed
 * to track progress.
 */
FIO_SFUNC void fio___test_multipart_chunked_parsing(void) {
  streaming_test_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *full_data = "------boundary\r\n"
                          "Content-Disposition: form-data; name=\"field1\"\r\n"
                          "\r\n"
                          "value1\r\n"
                          "------boundary\r\n"
                          "Content-Disposition: form-data; name=\"file1\"; "
                          "filename=\"test.txt\"\r\n"
                          "Content-Type: text/plain\r\n"
                          "\r\n"
                          "Hello, World!\r\n"
                          "------boundary--\r\n";

  size_t full_len = FIO_STRLEN(full_data);
  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  /* Simulate receiving data in chunks */
  size_t chunk_sizes[] = {50, 30, 40, 50, 100}; /* Various chunk sizes */
  size_t num_chunks = sizeof(chunk_sizes) / sizeof(chunk_sizes[0]);
  size_t total_consumed = 0;
  char buffer[1024];
  size_t buffer_len = 0;

  for (size_t i = 0; i < num_chunks && total_consumed < full_len; ++i) {
    /* Add next chunk to buffer */
    size_t chunk_size = chunk_sizes[i];
    if (total_consumed + buffer_len + chunk_size > full_len) {
      chunk_size = full_len - total_consumed - buffer_len;
    }

    FIO_MEMCPY(buffer + buffer_len,
               full_data + total_consumed + buffer_len,
               chunk_size);
    buffer_len += chunk_size;

    /* Parse available data */
    fio_multipart_result_s result = fio_multipart_parse(&streaming_callbacks,
                                                        &ctx,
                                                        boundary_buf,
                                                        buffer,
                                                        buffer_len);

    if (result.consumed > 0) {
      /* Move unconsumed data to start of buffer */
      size_t remaining = buffer_len - result.consumed;
      if (remaining > 0) {
        FIO_MEMMOVE(buffer, buffer + result.consumed, remaining);
      }
      total_consumed += result.consumed;
      buffer_len = remaining;
    }

    if (result.err == 0) {
      /* Parsing complete */
      break;
    }

    FIO_ASSERT(result.err == -2 || result.err == 0,
               "Chunked parse should return 0 or -2, got %d",
               result.err);
  }

  FIO_ASSERT(ctx.field_count == 1,
             "Should have 1 field, got %zu",
             ctx.field_count);
  FIO_ASSERT(ctx.file_count == 1,
             "Should have 1 file, got %zu",
             ctx.file_count);
  FIO_ASSERT(ctx.file_data_bytes == 13,
             "File should have 13 bytes, got %zu",
             ctx.file_data_bytes);
}

/**
 * Test: Incremental parsing - first part complete, second part incomplete
 *
 * Verifies that when we have complete parts followed by incomplete data,
 * the parser correctly processes complete parts and returns consumed count.
 */
FIO_SFUNC void fio___test_multipart_incremental_parts(void) {
  streaming_test_ctx_s ctx = {0};

  const char *boundary = "----boundary";

  /* First chunk: complete first part, incomplete second part */
  const char *chunk1 = "------boundary\r\n"
                       "Content-Disposition: form-data; name=\"field1\"\r\n"
                       "\r\n"
                       "value1\r\n"
                       "------boundary\r\n"
                       "Content-Disposition: form-data; name=\"field2\"\r\n"
                       "\r\n"
                       "partial_val"; /* Incomplete - no closing boundary */

  /* Second chunk: rest of second part */
  const char *chunk2 = "ue2\r\n"
                       "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  /* Parse first chunk */
  fio_multipart_result_s result1 = fio_multipart_parse(&streaming_callbacks,
                                                       &ctx,
                                                       boundary_buf,
                                                       chunk1,
                                                       FIO_STRLEN(chunk1));

  FIO_ASSERT(result1.err == -2,
             "First chunk should need more data, got err=%d",
             result1.err);
  FIO_ASSERT(result1.field_count == 1,
             "First chunk should have parsed 1 field, got %zu",
             result1.field_count);

  /* Build complete buffer with unconsumed data + new chunk */
  char buffer[1024];
  size_t unconsumed_len = FIO_STRLEN(chunk1) - result1.consumed;
  FIO_MEMCPY(buffer, chunk1 + result1.consumed, unconsumed_len);
  FIO_MEMCPY(buffer + unconsumed_len, chunk2, FIO_STRLEN(chunk2));
  size_t buffer_len = unconsumed_len + FIO_STRLEN(chunk2);

  /* Parse remaining data */
  fio_multipart_result_s result2 = fio_multipart_parse(&streaming_callbacks,
                                                       &ctx,
                                                       boundary_buf,
                                                       buffer,
                                                       buffer_len);

  FIO_ASSERT(result2.err == 0,
             "Second parse should succeed, got err=%d",
             result2.err);
  FIO_ASSERT(ctx.field_count == 2,
             "Should have 2 fields total, got %zu",
             ctx.field_count);
}

/**
 * Test: Large file streaming
 *
 * Tests that large file data is properly handled. Note: Current implementation
 * waits for complete part before calling on_file_data, so this tests that
 * large data is correctly passed through.
 */
FIO_SFUNC void fio___test_multipart_large_file(void) {
  streaming_test_ctx_s ctx = {0};

  const char *boundary = "----boundary";

  /* Build a multipart with a larger file (1KB of data) */
  char large_data[2048];
  size_t pos = 0;

  /* Header */
  const char *header = "------boundary\r\n"
                       "Content-Disposition: form-data; name=\"bigfile\"; "
                       "filename=\"large.bin\"\r\n"
                       "Content-Type: application/octet-stream\r\n"
                       "\r\n";
  size_t header_len = FIO_STRLEN(header);
  FIO_MEMCPY(large_data + pos, header, header_len);
  pos += header_len;

  /* File content: 1KB of 'X' characters */
  size_t file_size = 1024;
  for (size_t i = 0; i < file_size; ++i) {
    large_data[pos++] = 'X';
  }

  /* Footer */
  const char *footer = "\r\n------boundary--\r\n";
  size_t footer_len = FIO_STRLEN(footer);
  FIO_MEMCPY(large_data + pos, footer, footer_len);
  pos += footer_len;

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&streaming_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      large_data,
                                                      pos);

  FIO_ASSERT(result.err == 0, "Large file parse should succeed");
  FIO_ASSERT(ctx.file_count == 1, "Should have 1 file");
  FIO_ASSERT(ctx.file_data_bytes == file_size,
             "File should have %zu bytes, got %zu",
             file_size,
             ctx.file_data_bytes);
  FIO_ASSERT(ctx.file_data_calls == 1,
             "Should have 1 file_data call (current impl), got %zu",
             ctx.file_data_calls);
}

/**
 * Test: Boundary-like content in file data
 *
 * Tests that data containing boundary-like strings doesn't confuse the parser.
 * The actual boundary must be preceded by CRLF.
 */
FIO_SFUNC void fio___test_multipart_boundary_in_content(void) {
  streaming_test_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  /* File content contains something that looks like a boundary but isn't
   * (not preceded by CRLF) */
  const char *data =
      "------boundary\r\n"
      "Content-Disposition: form-data; name=\"file1\"; "
      "filename=\"test.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "This file contains ------boundary but not as a real boundary\r\n"
      "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&streaming_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Parse should succeed");
  FIO_ASSERT(ctx.file_count == 1, "Should have 1 file");

  /* The file content should be:
   * "This file contains ------boundary but not as a real boundary"
   * (60 characters) */
  FIO_ASSERT(ctx.file_data_bytes == 60,
             "File should have 60 bytes, got %zu",
             ctx.file_data_bytes);
}

/**
 * Test: Large form field value
 *
 * Tests handling of large form field values (not files).
 * Current implementation passes entire value to on_field callback.
 */
FIO_SFUNC void fio___test_multipart_large_field_value(void) {
  /* Use streaming callbacks to track field */
  streaming_test_ctx_s ctx = {0};

  const char *boundary = "----boundary";

  /* Build multipart with large field value (2KB) */
  char large_data[4096];
  size_t pos = 0;

  const char *header = "------boundary\r\n"
                       "Content-Disposition: form-data; name=\"large_text\"\r\n"
                       "\r\n";
  size_t header_len = FIO_STRLEN(header);
  FIO_MEMCPY(large_data + pos, header, header_len);
  pos += header_len;

  /* Field value: 2KB of 'A' characters */
  size_t value_size = 2048;
  for (size_t i = 0; i < value_size; ++i) {
    large_data[pos++] = 'A';
  }

  const char *footer = "\r\n------boundary--\r\n";
  size_t footer_len = FIO_STRLEN(footer);
  FIO_MEMCPY(large_data + pos, footer, footer_len);
  pos += footer_len;

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&streaming_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      large_data,
                                                      pos);

  FIO_ASSERT(result.err == 0, "Large field parse should succeed");
  FIO_ASSERT(ctx.field_count == 1, "Should have 1 field");
  FIO_ASSERT(result.field_count == 1, "Result should show 1 field");
}

/**
 * Test: Chunk split at boundary
 *
 * Tests what happens when a chunk split occurs right in the middle of
 * a boundary marker.
 */
FIO_SFUNC void fio___test_multipart_chunk_split_at_boundary(void) {
  streaming_test_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *full_data = "------boundary\r\n"
                          "Content-Disposition: form-data; name=\"field1\"\r\n"
                          "\r\n"
                          "value1\r\n"
                          "------boundary--\r\n";

  size_t full_len = FIO_STRLEN(full_data);
  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  /* Find position of second boundary and split there */
  const char *p = full_data + 20;
  size_t second_boundary_pos = 0;
  while (p < full_data + full_len - 10) {
    if (p[0] == '\r' && p[1] == '\n' && p[2] == '-' && p[3] == '-') {
      second_boundary_pos = (size_t)(p + 2 - full_data); /* Point to "--" */
      break;
    }
    ++p;
  }

  /* Split right in the middle of "------boundary" */
  size_t split_pos = second_boundary_pos + 5; /* Mid-boundary */

  /* First chunk: up to middle of boundary */
  char buffer[256];
  FIO_MEMCPY(buffer, full_data, split_pos);

  fio_multipart_result_s result1 = fio_multipart_parse(&streaming_callbacks,
                                                       &ctx,
                                                       boundary_buf,
                                                       buffer,
                                                       split_pos);

  /* Should need more data since boundary is incomplete */
  FIO_ASSERT(result1.err == -2,
             "Split at boundary should need more data, got err=%d",
             result1.err);

  /* Add rest of data */
  size_t unconsumed = split_pos - result1.consumed;
  FIO_MEMMOVE(buffer, buffer + result1.consumed, unconsumed);
  FIO_MEMCPY(buffer + unconsumed, full_data + split_pos, full_len - split_pos);
  size_t buffer_len = unconsumed + (full_len - split_pos);

  fio_multipart_result_s result2 = fio_multipart_parse(&streaming_callbacks,
                                                       &ctx,
                                                       boundary_buf,
                                                       buffer,
                                                       buffer_len);

  FIO_ASSERT(result2.err == 0,
             "Final parse should succeed, got err=%d",
             result2.err);
  FIO_ASSERT(ctx.field_count == 1,
             "Should have 1 field, got %zu",
             ctx.field_count);
}

/**
 * Test: Empty file upload
 *
 * Tests handling of file upload with zero-length content.
 * Note: In multipart format, the body is between \r\n\r\n (end of headers)
 * and \r\n--boundary. For an empty file, there's no content between them.
 */
FIO_SFUNC void fio___test_multipart_empty_file(void) {
  streaming_test_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  /* Empty file: headers end with \r\n\r\n, then immediately \r\n--boundary */
  const char *data = "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"file1\"; "
                     "filename=\"empty.txt\"\r\n"
                     "Content-Type: text/plain\r\n"
                     "\r\n" /* End of headers, body starts here */
                     /* Body is empty - next line is the boundary */
                     "\r\n------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&streaming_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Empty file parse should succeed");
  FIO_ASSERT(ctx.file_count == 1, "Should have 1 file");
  FIO_ASSERT(ctx.file_data_bytes == 0,
             "Empty file should have 0 bytes, got %zu",
             ctx.file_data_bytes);
  FIO_ASSERT(ctx.file_start_calls == 1, "Should have 1 file_start call");
  FIO_ASSERT(ctx.file_end_calls == 1, "Should have 1 file_end call");
}

/**
 * Test: Verify consumed bytes tracking
 *
 * Verifies that result.consumed accurately reflects bytes processed.
 */
FIO_SFUNC void fio___test_multipart_consumed_tracking(void) {
  streaming_test_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data = "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field1\"\r\n"
                     "\r\n"
                     "value1\r\n"
                     "------boundary--\r\n";

  size_t data_len = FIO_STRLEN(data);
  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&streaming_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      data_len);

  FIO_ASSERT(result.err == 0, "Parse should succeed");
  FIO_ASSERT(result.consumed == data_len,
             "Should consume all %zu bytes, consumed %zu",
             data_len,
             result.consumed);
}

/**
 * Test: Binary file content
 *
 * Tests that binary data (including NUL bytes) is handled correctly.
 */
FIO_SFUNC void fio___test_multipart_binary_content(void) {
  streaming_test_ctx_s ctx = {0};

  const char *boundary = "----boundary";

  /* Build multipart with binary content including NUL bytes */
  char binary_data[512];
  size_t pos = 0;

  const char *header =
      "------boundary\r\n"
      "Content-Disposition: form-data; name=\"bin\"; filename=\"data.bin\"\r\n"
      "Content-Type: application/octet-stream\r\n"
      "\r\n";
  size_t header_len = FIO_STRLEN(header);
  FIO_MEMCPY(binary_data + pos, header, header_len);
  pos += header_len;

  /* Binary content with NUL bytes */
  unsigned char bin_content[] =
      {0x00, 0x01, 0x02, 0xFF, 0xFE, 0x00, 0x00, 0x42};
  size_t bin_len = sizeof(bin_content);
  FIO_MEMCPY(binary_data + pos, bin_content, bin_len);
  pos += bin_len;

  const char *footer = "\r\n------boundary--\r\n";
  size_t footer_len = FIO_STRLEN(footer);
  FIO_MEMCPY(binary_data + pos, footer, footer_len);
  pos += footer_len;

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result = fio_multipart_parse(&streaming_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      binary_data,
                                                      pos);

  FIO_ASSERT(result.err == 0, "Binary content parse should succeed");
  FIO_ASSERT(ctx.file_count == 1, "Should have 1 file");
  FIO_ASSERT(ctx.file_data_bytes == bin_len,
             "Binary file should have %zu bytes, got %zu",
             bin_len,
             ctx.file_data_bytes);

  /* Verify binary content was preserved */
  FIO_ASSERT(ctx.accumulated_len == bin_len, "Accumulated length mismatch");
  FIO_ASSERT(FIO_MEMCMP(ctx.accumulated_data, bin_content, bin_len) == 0,
             "Binary content should be preserved");
}

/* *****************************************************************************
Streaming Field Callbacks Test Context
***************************************************************************** */

typedef struct {
  size_t field_count;
  size_t file_count;
  size_t field_start_calls;
  size_t field_data_calls;
  size_t field_end_calls;
  size_t field_data_bytes;
  size_t file_data_bytes;
  char accumulated_field_data[4096];
  size_t accumulated_field_len;
  char last_field_name[256];
} streaming_field_ctx_s;

static void *streaming_field_on_field_start(void *udata,
                                            fio_buf_info_s name,
                                            fio_buf_info_s content_type) {
  streaming_field_ctx_s *ctx = (streaming_field_ctx_s *)udata;
  ++ctx->field_count;
  ++ctx->field_start_calls;

  /* Store field name for verification */
  if (name.len < sizeof(ctx->last_field_name)) {
    FIO_MEMCPY(ctx->last_field_name, name.buf, name.len);
    ctx->last_field_name[name.len] = '\0';
  }

  (void)content_type;
  return ctx; /* Return context for field data */
}

static int streaming_field_on_field_data(void *udata,
                                         void *field_ctx,
                                         fio_buf_info_s data) {
  streaming_field_ctx_s *ctx = (streaming_field_ctx_s *)udata;
  ++ctx->field_data_calls;
  ctx->field_data_bytes += data.len;

  /* Accumulate data for verification */
  if (ctx->accumulated_field_len + data.len <
      sizeof(ctx->accumulated_field_data)) {
    FIO_MEMCPY(ctx->accumulated_field_data + ctx->accumulated_field_len,
               data.buf,
               data.len);
    ctx->accumulated_field_len += data.len;
  }

  (void)field_ctx;
  return 0;
}

static void streaming_field_on_field_end(void *udata, void *field_ctx) {
  streaming_field_ctx_s *ctx = (streaming_field_ctx_s *)udata;
  ++ctx->field_end_calls;
  (void)field_ctx;
}

static void *streaming_field_on_file_start(void *udata,
                                           fio_buf_info_s name,
                                           fio_buf_info_s filename,
                                           fio_buf_info_s content_type) {
  streaming_field_ctx_s *ctx = (streaming_field_ctx_s *)udata;
  ++ctx->file_count;
  (void)name;
  (void)filename;
  (void)content_type;
  return ctx;
}

static int streaming_field_on_file_data(void *udata,
                                        void *file_ctx,
                                        fio_buf_info_s data) {
  streaming_field_ctx_s *ctx = (streaming_field_ctx_s *)udata;
  ctx->file_data_bytes += data.len;
  (void)file_ctx;
  return 0;
}

static void streaming_field_on_file_end(void *udata, void *file_ctx) {
  (void)udata;
  (void)file_ctx;
}

static const fio_multipart_parser_callbacks_s streaming_field_callbacks = {
    .on_field = NULL, /* Not used when streaming callbacks are provided */
    .on_field_start = streaming_field_on_field_start,
    .on_field_data = streaming_field_on_field_data,
    .on_field_end = streaming_field_on_field_end,
    .on_file_start = streaming_field_on_file_start,
    .on_file_data = streaming_field_on_file_data,
    .on_file_end = streaming_field_on_file_end,
    .on_error = test_on_error,
};

/* *****************************************************************************
Streaming Field Callback Tests
***************************************************************************** */

/**
 * Test: Streaming field callbacks with small field
 *
 * Verifies that streaming field callbacks work correctly for small fields.
 */
FIO_SFUNC void fio___test_multipart_streaming_field_small(void) {
  streaming_field_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data = "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"username\"\r\n"
                     "\r\n"
                     "john_doe\r\n"
                     "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result =
      fio_multipart_parse(&streaming_field_callbacks,
                          &ctx,
                          boundary_buf,
                          data,
                          FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Streaming field parse should succeed");
  FIO_ASSERT(result.field_count == 1,
             "Should have 1 field, got %zu",
             result.field_count);
  FIO_ASSERT(ctx.field_start_calls == 1,
             "Should have 1 field_start call, got %zu",
             ctx.field_start_calls);
  FIO_ASSERT(ctx.field_data_calls == 1,
             "Should have 1 field_data call, got %zu",
             ctx.field_data_calls);
  FIO_ASSERT(ctx.field_end_calls == 1,
             "Should have 1 field_end call, got %zu",
             ctx.field_end_calls);
  FIO_ASSERT(ctx.field_data_bytes == 8,
             "Field should have 8 bytes ('john_doe'), got %zu",
             ctx.field_data_bytes);
  FIO_ASSERT(FIO_MEMCMP(ctx.accumulated_field_data, "john_doe", 8) == 0,
             "Field data should be 'john_doe'");
  FIO_ASSERT(FIO_MEMCMP(ctx.last_field_name, "username", 8) == 0,
             "Field name should be 'username'");
}

/**
 * Test: Streaming field callbacks with large field (2KB+)
 *
 * Verifies that streaming field callbacks work correctly for large fields.
 */
FIO_SFUNC void fio___test_multipart_streaming_field_large(void) {
  streaming_field_ctx_s ctx = {0};

  const char *boundary = "----boundary";

  /* Build multipart with large field value (2KB) */
  char large_data[4096];
  size_t pos = 0;

  const char *header = "------boundary\r\n"
                       "Content-Disposition: form-data; name=\"large_text\"\r\n"
                       "\r\n";
  size_t header_len = FIO_STRLEN(header);
  FIO_MEMCPY(large_data + pos, header, header_len);
  pos += header_len;

  /* Field value: 2KB of 'B' characters */
  size_t value_size = 2048;
  for (size_t i = 0; i < value_size; ++i) {
    large_data[pos++] = 'B';
  }

  const char *footer = "\r\n------boundary--\r\n";
  size_t footer_len = FIO_STRLEN(footer);
  FIO_MEMCPY(large_data + pos, footer, footer_len);
  pos += footer_len;

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result =
      fio_multipart_parse(&streaming_field_callbacks,
                          &ctx,
                          boundary_buf,
                          large_data,
                          pos);

  FIO_ASSERT(result.err == 0, "Large streaming field parse should succeed");
  FIO_ASSERT(result.field_count == 1, "Should have 1 field");
  FIO_ASSERT(ctx.field_start_calls == 1, "Should have 1 field_start call");
  FIO_ASSERT(ctx.field_data_calls == 1,
             "Should have 1 field_data call (current impl)");
  FIO_ASSERT(ctx.field_end_calls == 1, "Should have 1 field_end call");
  FIO_ASSERT(ctx.field_data_bytes == value_size,
             "Field should have %zu bytes, got %zu",
             value_size,
             ctx.field_data_bytes);

  /* Verify all data is 'B' */
  for (size_t i = 0; i < value_size && i < ctx.accumulated_field_len; ++i) {
    FIO_ASSERT(ctx.accumulated_field_data[i] == 'B',
               "Field data byte %zu should be 'B'",
               i);
  }
}

/**
 * Test: Backward compatibility - on_field still works when streaming callbacks
 * not provided
 *
 * Verifies that the legacy on_field callback is used when on_field_start is
 * NULL.
 */
FIO_SFUNC void fio___test_multipart_streaming_field_backward_compat(void) {
  test_multipart_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data = "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field1\"\r\n"
                     "\r\n"
                     "value1\r\n"
                     "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field2\"\r\n"
                     "\r\n"
                     "value2\r\n"
                     "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  /* Use test_callbacks which has on_field but no streaming callbacks */
  fio_multipart_result_s result = fio_multipart_parse(&test_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Backward compat parse should succeed");
  FIO_ASSERT(result.field_count == 2,
             "Should have 2 fields, got %zu",
             result.field_count);
  FIO_ASSERT(ctx.field_count == 2,
             "on_field callback should be called 2 times, got %zu",
             ctx.field_count);
  FIO_ASSERT(FIO_MEMCMP(ctx.last_field_name, "field2", 6) == 0,
             "Last field name should be 'field2'");
  FIO_ASSERT(FIO_MEMCMP(ctx.last_field_value, "value2", 6) == 0,
             "Last field value should be 'value2'");
}

/**
 * Test: Mixed usage - fields use streaming, files use streaming
 *
 * Verifies that streaming field callbacks work alongside file streaming
 * callbacks.
 */
FIO_SFUNC void fio___test_multipart_streaming_field_mixed(void) {
  streaming_field_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data = "------boundary\r\n"
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
                     "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result =
      fio_multipart_parse(&streaming_field_callbacks,
                          &ctx,
                          boundary_buf,
                          data,
                          FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Mixed streaming parse should succeed");
  FIO_ASSERT(result.field_count == 2,
             "Should have 2 fields, got %zu",
             result.field_count);
  FIO_ASSERT(result.file_count == 1,
             "Should have 1 file, got %zu",
             result.file_count);
  FIO_ASSERT(ctx.field_start_calls == 2,
             "Should have 2 field_start calls, got %zu",
             ctx.field_start_calls);
  FIO_ASSERT(ctx.field_data_calls == 2,
             "Should have 2 field_data calls, got %zu",
             ctx.field_data_calls);
  FIO_ASSERT(ctx.field_end_calls == 2,
             "Should have 2 field_end calls, got %zu",
             ctx.field_end_calls);
  FIO_ASSERT(ctx.file_count == 1,
             "Should have 1 file, got %zu",
             ctx.file_count);
  FIO_ASSERT(ctx.file_data_bytes == 14,
             "File should have 14 bytes, got %zu",
             ctx.file_data_bytes);
}

/**
 * Test: Streaming field abort
 *
 * Verifies that returning non-zero from on_field_data aborts parsing.
 */
static int aborting_field_data(void *udata,
                               void *field_ctx,
                               fio_buf_info_s data) {
  (void)udata;
  (void)field_ctx;
  (void)data;
  return 1; /* Abort */
}

FIO_SFUNC void fio___test_multipart_streaming_field_abort(void) {
  streaming_field_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data = "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field1\"\r\n"
                     "\r\n"
                     "value1\r\n"
                     "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field2\"\r\n"
                     "\r\n"
                     "value2\r\n"
                     "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  /* Create callbacks with aborting field_data */
  fio_multipart_parser_callbacks_s abort_callbacks = {
      .on_field = NULL,
      .on_field_start = streaming_field_on_field_start,
      .on_field_data = aborting_field_data,
      .on_field_end = streaming_field_on_field_end,
      .on_file_start = streaming_field_on_file_start,
      .on_file_data = streaming_field_on_file_data,
      .on_file_end = streaming_field_on_file_end,
      .on_error = test_on_error,
  };

  fio_multipart_result_s result = fio_multipart_parse(&abort_callbacks,
                                                      &ctx,
                                                      boundary_buf,
                                                      data,
                                                      FIO_STRLEN(data));

  FIO_ASSERT(result.err == -1, "Aborted parse should return error -1");
  FIO_ASSERT(ctx.field_start_calls == 1,
             "Should have 1 field_start call before abort");
}

/**
 * Test: Multiple fields with streaming callbacks
 *
 * Verifies that multiple fields are handled correctly with streaming callbacks.
 */
FIO_SFUNC void fio___test_multipart_streaming_field_multiple(void) {
  streaming_field_ctx_s ctx = {0};

  const char *boundary = "----boundary";
  const char *data = "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field1\"\r\n"
                     "\r\n"
                     "value1\r\n"
                     "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field2\"\r\n"
                     "\r\n"
                     "value2\r\n"
                     "------boundary\r\n"
                     "Content-Disposition: form-data; name=\"field3\"\r\n"
                     "\r\n"
                     "value3\r\n"
                     "------boundary--\r\n";

  fio_buf_info_s boundary_buf = FIO_BUF_INFO1((char *)boundary);

  fio_multipart_result_s result =
      fio_multipart_parse(&streaming_field_callbacks,
                          &ctx,
                          boundary_buf,
                          data,
                          FIO_STRLEN(data));

  FIO_ASSERT(result.err == 0, "Multiple fields parse should succeed");
  FIO_ASSERT(result.field_count == 3,
             "Should have 3 fields, got %zu",
             result.field_count);
  FIO_ASSERT(ctx.field_start_calls == 3,
             "Should have 3 field_start calls, got %zu",
             ctx.field_start_calls);
  FIO_ASSERT(ctx.field_data_calls == 3,
             "Should have 3 field_data calls, got %zu",
             ctx.field_data_calls);
  FIO_ASSERT(ctx.field_end_calls == 3,
             "Should have 3 field_end calls, got %zu",
             ctx.field_end_calls);
  /* Total bytes: "value1" + "value2" + "value3" = 6 + 6 + 6 = 18 */
  FIO_ASSERT(ctx.field_data_bytes == 18,
             "Total field bytes should be 18, got %zu",
             ctx.field_data_bytes);
}

/* *****************************************************************************
Main Test Entry Point
***************************************************************************** */

void fio_test_multipart(void) {
  /* Basic tests */
  fio___test_multipart_basic();
  fio___test_multipart_file_upload();
  fio___test_multipart_empty();
  fio___test_multipart_quoted_filename();
  fio___test_multipart_need_more_data();
  fio___test_multipart_multiple_files();
  fio___test_multipart_mixed_content();
  fio___test_multipart_with_crlf_prefix();

  /* Streaming tests */
  fio___test_multipart_chunked_parsing();
  fio___test_multipart_incremental_parts();
  fio___test_multipart_large_file();
  fio___test_multipart_boundary_in_content();
  fio___test_multipart_large_field_value();
  fio___test_multipart_chunk_split_at_boundary();
  fio___test_multipart_empty_file();
  fio___test_multipart_consumed_tracking();
  fio___test_multipart_binary_content();

  /* Streaming field callback tests */
  fio___test_multipart_streaming_field_small();
  fio___test_multipart_streaming_field_large();
  fio___test_multipart_streaming_field_backward_compat();
  fio___test_multipart_streaming_field_mixed();
  fio___test_multipart_streaming_field_abort();
  fio___test_multipart_streaming_field_multiple();
}

#ifndef FIO_TEST_ALL
int main(void) {
  fio_test_multipart();
  return 0;
}
#endif
