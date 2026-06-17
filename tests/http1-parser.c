/* *****************************************************************************
Test: HTTP/1.x parser correctness
***************************************************************************** */
#define FIO_HTTP1_PARSER
#include "test-helpers.h"

/* Callback state collector */
typedef struct {
  int complete;
  int expect;
  fio_buf_info_s method;
  fio_buf_info_s url;
  fio_buf_info_s version;
  size_t status;
  fio_buf_info_s status_str;
  size_t header_count;
  size_t content_length;
  int cl_received;
  size_t body_len;
} parser_state_s;

static void fio_http1_on_complete(void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  s->complete = 1;
}

static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  s->method = method;
  return 0;
}

static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  s->status = istatus;
  s->status_str = status;
  return 0;
}

static int fio_http1_on_url(fio_buf_info_s path, void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  s->url = path;
  return 0;
}

static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  s->version = version;
  return 0;
}

static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  ++s->header_count;
  (void)name;
  (void)value;
  return 0;
}

static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  s->content_length = content_length;
  s->cl_received = 1;
  (void)name;
  (void)value;
  return 0;
}

static int fio_http1_on_expect(void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  s->expect = 1;
  return 0;
}

static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  s->body_len += chunk.len;
  return 0;
}

static size_t run_parse(parser_state_s *st, char *buf, size_t len) {
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  return fio_http1_parse(&parser, FIO_BUF_INFO2(buf, len), st);
}

static size_t run_parse_persist(parser_state_s *st,
                                fio_http1_parser_s *parser,
                                char *buf,
                                size_t len) {
  return fio_http1_parse(parser, FIO_BUF_INFO2(buf, len), st);
}

/* ===========================================================================
   Basic request parsing
   ===========================================================================
 */

static void test_basic_request(void) {
  fprintf(stderr, "  * basic request parsing\n");
  char req[] = "GET /index.html HTTP/1.1\r\n"
               "Host: example.com\r\n"
               "Accept: text/html\r\n"
               "\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result == sizeof(req) - 1,
             "basic request: bytes consumed mismatch");
  FIO_ASSERT(st.complete, "basic request: should be complete");
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(st.method, FIO_BUF_INFO1((char *)"GET")),
             "basic request: method mismatch");
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(st.url, FIO_BUF_INFO1((char *)"/index.html")),
             "basic request: URL mismatch");
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(st.version, FIO_BUF_INFO1((char *)"HTTP/1.1")),
             "basic request: version mismatch");
  FIO_ASSERT(st.header_count == 2,
             "basic request: expected 2 headers, got %zu",
             st.header_count);
}

static void test_lf_only(void) {
  fprintf(stderr, "  * LF-only line endings\n");
  char req[] = "GET / HTTP/1.1\nHost: x\n\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result != FIO_HTTP1_PARSER_ERROR,
             "lf_only: parser returned error");
  FIO_ASSERT(st.complete, "lf_only: should be complete");
}

static void test_leading_whitespace(void) {
  fprintf(stderr, "  * leading whitespace in request line\n");
  char req[] = "    GET / HTTP/1.1\r\n\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result != FIO_HTTP1_PARSER_ERROR,
             "leading_ws: parser returned error");
  FIO_ASSERT(st.complete, "leading_ws: should be complete");
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(st.method, FIO_BUF_INFO1((char *)"GET")),
             "leading_ws: method mismatch");
}

static void test_response_line(void) {
  fprintf(stderr, "  * response line parsing\n");
  char resp[] = "HTTP/1.1 200 OK\r\n\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)resp, sizeof(resp) - 1);
  FIO_ASSERT(result != FIO_HTTP1_PARSER_ERROR,
             "response_line: parser returned error");
  FIO_ASSERT(st.complete, "response_line: should be complete");
  FIO_ASSERT(st.status == 200,
             "response_line: status mismatch (%zu)",
             st.status);
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(st.status_str, FIO_BUF_INFO1((char *)"OK")),
             "response_line: status text mismatch");
  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(st.version, FIO_BUF_INFO1((char *)"HTTP/1.1")),
             "response_line: version mismatch");
}

/* ===========================================================================
   Content-Length body handling
   ===========================================================================
 */

static void test_content_length_body(void) {
  fprintf(stderr, "  * Content-Length body\n");
  char req[] = "POST / HTTP/1.1\r\n"
               "Content-Length: 5\r\n"
               "\r\n"
               "Hello";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result == sizeof(req) - 1, "cl body: bytes consumed mismatch");
  FIO_ASSERT(st.complete, "cl body: should be complete");
  FIO_ASSERT(st.cl_received && st.content_length == 5,
             "cl body: content-length mismatch");
  FIO_ASSERT(st.body_len == 5,
             "cl body: body length mismatch (%zu)",
             st.body_len);
}

static void test_content_length_fragmented(void) {
  fprintf(stderr, "  * Content-Length fragmented input\n");
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  parser_state_s st = {0};
  char part1[] = "POST / HTTP/1.1\r\nContent-Length: 11\r\n\r\nHel";
  char part2[] = "lo World!";
  size_t r1 = run_parse_persist(&st, &parser, (char *)part1, sizeof(part1) - 1);
  size_t r2 = run_parse_persist(&st, &parser, (char *)part2, sizeof(part2) - 1);
  FIO_ASSERT(r1 != FIO_HTTP1_PARSER_ERROR, "cl fragmented: part1 parser error");
  FIO_ASSERT(r2 != FIO_HTTP1_PARSER_ERROR, "cl fragmented: part2 parser error");
  FIO_ASSERT(st.complete, "cl fragmented: should be complete");
  FIO_ASSERT(st.body_len == 11,
             "cl fragmented: body length mismatch (%zu)",
             st.body_len);
}

static void test_empty_content_length_rejected(void) {
  fprintf(stderr, "  * empty Content-Length rejected\n");
  char req[] = "GET / HTTP/1.1\r\nContent-Length: \r\n\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result == FIO_HTTP1_PARSER_ERROR,
             "empty_cl: parser should have rejected input");
}

static void test_huge_content_length_rejected(void) {
  fprintf(stderr, "  * huge Content-Length rejected\n");
  char req[] = "GET / HTTP/1.1\r\nContent-Length: 99999999999999999999\r\n\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result == FIO_HTTP1_PARSER_ERROR,
             "huge_cl: parser should have rejected input");
}

static void test_cl_te_conflict_rejected(void) {
  fprintf(stderr, "  * Content-Length + Transfer-Encoding conflict rejected\n");
  char req[] = "POST / HTTP/1.1\r\n"
               "Content-Length: 5\r\n"
               "Transfer-Encoding: chunked\r\n"
               "\r\n"
               "5\r\nHello\r\n0\r\n\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result == FIO_HTTP1_PARSER_ERROR,
             "cl_te_conflict: parser should have rejected input");
}

/* ===========================================================================
   Chunked encoding
   ===========================================================================
 */

static void test_chunked_body(void) {
  fprintf(stderr, "  * chunked body\n");
  char req[] = "POST / HTTP/1.1\r\n"
               "Transfer-Encoding: chunked\r\n"
               "\r\n"
               "5\r\nHello\r\n"
               "1\r\n!\r\n"
               "0\r\n\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result == sizeof(req) - 1, "chunked: bytes consumed mismatch");
  FIO_ASSERT(st.complete, "chunked: should be complete");
  FIO_ASSERT(st.body_len == 6,
             "chunked: body length mismatch (%zu)",
             st.body_len);
}

static void test_chunked_fragmented(void) {
  fprintf(stderr, "  * chunked fragmented input\n");
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  parser_state_s st = {0};
  char part1[] = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n";
  char part2[] = "5\r\nHel";
  char part3[] = "lo\r\n0\r\n\r\n";
  size_t r1 = run_parse_persist(&st, &parser, (char *)part1, sizeof(part1) - 1);
  size_t r2 = run_parse_persist(&st, &parser, (char *)part2, sizeof(part2) - 1);
  size_t r3 = run_parse_persist(&st, &parser, (char *)part3, sizeof(part3) - 1);
  FIO_ASSERT(r1 != FIO_HTTP1_PARSER_ERROR,
             "fragmented_chunked part1: parser returned error");
  FIO_ASSERT(r2 != FIO_HTTP1_PARSER_ERROR,
             "fragmented_chunked part2: parser returned error");
  FIO_ASSERT(r3 != FIO_HTTP1_PARSER_ERROR,
             "fragmented_chunked part3: parser returned error");
  FIO_ASSERT(fio_http1_parser_is_empty(&parser),
             "fragmented_chunked: parser should be empty after all parts");
  FIO_ASSERT(st.body_len == 5,
             "fragmented_chunked: body length mismatch (%zu)",
             st.body_len);
}

static void test_bad_chunk_size_rejected(void) {
  fprintf(stderr, "  * bad chunk size rejected\n");
  char req[] = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\nZZZ\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result == FIO_HTTP1_PARSER_ERROR,
             "bad_chunk_size: parser should have rejected input");
}

static void test_negative_chunk_rejected(void) {
  fprintf(stderr, "  * negative chunk size rejected\n");
  char req[] = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n-1\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result == FIO_HTTP1_PARSER_ERROR,
             "negative_chunk: parser should have rejected input");
}

/* ===========================================================================
   Error / malformed inputs
   ===========================================================================
 */

static void test_nul_in_uri_rejected(void) {
  fprintf(stderr, "  * NUL in URI rejected\n");
  char prefix[] = "GET /pa";
  char suffix[] = "th HTTP/1.1\r\n\r\n";
  size_t total = (sizeof(prefix) - 1) + 1 + (sizeof(suffix) - 1);
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, prefix, sizeof(prefix) - 1);
  buf[sizeof(prefix) - 1] = '\0';
  FIO_MEMCPY(buf + sizeof(prefix), suffix, sizeof(suffix) - 1);
  parser_state_s st = {0};
  size_t result = run_parse(&st, buf, total);
  FIO_ASSERT(result == FIO_HTTP1_PARSER_ERROR,
             "nul_in_uri: parser should have rejected input");
  FIO_MEM_FREE(buf, total);
}

static void test_nul_in_header_rejected(void) {
  fprintf(stderr, "  * NUL in header value rejected\n");
  char prefix[] = "GET / HTTP/1.1\r\nHost: x";
  char suffix[] = "y\r\n\r\n";
  size_t total = (sizeof(prefix) - 1) + 1 + (sizeof(suffix) - 1);
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, prefix, sizeof(prefix) - 1);
  buf[sizeof(prefix) - 1] = '\0';
  FIO_MEMCPY(buf + sizeof(prefix), suffix, sizeof(suffix) - 1);
  parser_state_s st = {0};
  size_t result = run_parse(&st, buf, total);
  FIO_ASSERT(result == FIO_HTTP1_PARSER_ERROR,
             "nul_in_header: parser should have rejected input");
  FIO_MEM_FREE(buf, total);
}

static void test_missing_colon_rejected(void) {
  fprintf(stderr, "  * header missing colon rejected\n");
  char req[] = "GET / HTTP/1.1\r\nHost\r\n\r\n";
  parser_state_s st = {0};
  size_t result = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(result == FIO_HTTP1_PARSER_ERROR,
             "missing_colon: parser should have rejected input");
}

static void test_many_headers(void) {
  fprintf(stderr, "  * many headers accepted\n");
  size_t header_len = 6;
  size_t count = 1000;
  size_t total = 14 + count * header_len + 2;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, "GET / HTTP/1.1\r\n", 14);
  for (size_t i = 0; i < count; i++)
    FIO_MEMCPY(buf + 14 + i * header_len, "H: v\r\n", header_len);
  FIO_MEMCPY(buf + total - 2, "\r\n", 2);
  parser_state_s st = {0};
  size_t result = run_parse(&st, buf, total);
  FIO_ASSERT(result != FIO_HTTP1_PARSER_ERROR,
             "many_headers: parser returned error");
  FIO_ASSERT(st.complete, "many_headers: should be complete");
  FIO_MEM_FREE(buf, total);
}

/* ===========================================================================
   Parser state queries
   ===========================================================================
 */

static void test_parser_state_queries(void) {
  fprintf(stderr, "  * parser state queries\n");
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  FIO_ASSERT(fio_http1_parser_is_empty(&parser),
             "fresh parser should be empty");
  char req[] = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
  parser_state_s st = {0};
  size_t r = fio_http1_parse(&parser,
                             FIO_BUF_INFO2((char *)req, sizeof(req) - 1),
                             &st);
  FIO_ASSERT(r == sizeof(req) - 1, "state query: bytes consumed mismatch");
  FIO_ASSERT(st.complete, "state query: should be complete");
  FIO_ASSERT(fio_http1_parser_is_empty(&parser),
             "parser should be empty after complete request");
}

/* ===========================================================================
 * Regression test: V4 — fio_http1_parse first-line OOB read (CWE-125)
 *
 * After skipping leading whitespace, the old code searched for '\n' using the
 * full original buffer length, reading up to `start - buf->buf` bytes past the
 * end of the buffer when no newline was present. The request is heap-allocated
 * at the exact size so AddressSanitizer detects the over-read on unpatched
 * code. After the fix the parser correctly reports that more data is needed.
 * ===========================================================================
 */
static void test_leading_whitespace_no_newline(void) {
  fprintf(stderr, "  * leading whitespace without newline\n");
  const char *r = "   GET / HTTP/1.1";
  size_t n = strlen(r);
  char *p = (char *)FIO_MEM_REALLOC(NULL, 0, n, 0);
  FIO_ASSERT_ALLOC(p);
  FIO_MEMCPY(p, r, n);

  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  parser_state_s st = {0};
  size_t result = fio_http1_parse(&parser, FIO_BUF_INFO2(p, n), &st);
  FIO_ASSERT(result != FIO_HTTP1_PARSER_ERROR,
             "leading_ws_no_newline: parser returned error");
  FIO_ASSERT(!st.complete, "leading_ws_no_newline: should not be complete");

  FIO_MEM_FREE(p, n);
}

/* ===========================================================================
   Main
   ===========================================================================
 */

int main(void) {
  fprintf(stderr, "Testing fio_http1_parse correctness:\n");
  test_basic_request();
  test_lf_only();
  test_leading_whitespace();
  test_response_line();
  test_content_length_body();
  test_content_length_fragmented();
  test_empty_content_length_rejected();
  test_huge_content_length_rejected();
  test_cl_te_conflict_rejected();
  test_chunked_body();
  test_chunked_fragmented();
  test_bad_chunk_size_rejected();
  test_negative_chunk_rejected();
  test_nul_in_uri_rejected();
  test_nul_in_header_rejected();
  test_missing_colon_rejected();
  test_many_headers();
  test_parser_state_queries();
  test_leading_whitespace_no_newline();
  fprintf(stderr, "All HTTP/1 parser tests passed!\n");
  return 0;
}
