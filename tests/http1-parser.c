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
  fio_http1_parser_s *parser; /* set by the run_parse helpers */
  int mark_skip_body;         /* if set, on_status calls skip_body (HEAD) */
  int skips_at_status;        /* skips_body() before on_status marking */
  int skips_at_header;        /* skips_body() at the first header callback */
  int skips_at_cl;            /* skips_body() in the content-length callback */
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
  if (!s->parser) /* direct fio_http1_parse calls */
    return 0;
  s->skips_at_status = !!fio_http1_parser_skips_body(s->parser);
  if (s->mark_skip_body)
    fio_http1_parser_skip_body(s->parser);
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
  if (!s->header_count && s->parser)
    s->skips_at_header = !!fio_http1_parser_skips_body(s->parser);
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
  s->skips_at_cl = s->parser && fio_http1_parser_skips_body(s->parser);
  (void)name;
  (void)value;
  return 0;
}

static int fio_http1_on_expect(void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  ++s->expect;
  return 0;
}

static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  parser_state_s *s = (parser_state_s *)udata;
  s->body_len += chunk.len;
  return 0;
}

static size_t run_parse(parser_state_s *st, char *buf, size_t len) {
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  st->parser = &parser;
  return fio_http1_parse(&parser, FIO_BUF_INFO2(buf, len), st);
}

static size_t run_parse_persist(parser_state_s *st,
                                fio_http1_parser_s *parser,
                                char *buf,
                                size_t len) {
  st->parser = parser;
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
 * Regression test (2026-07-31 audit): header-line CR trim underflow (CWE-125)
 *
 * An empty header line (end of headers) arriving at the very start of the
 * parse buffer made `eol[-1]` read one byte BEFORE the buffer. Reproduced by
 * splitting the request across two parse calls; the second buffer is an
 * exact-size heap allocation holding only '\n', so AddressSanitizer (with
 * FIO_MEMORY_DISABLE) detects any recurrence on unpatched code.
 * ===========================================================================
 */
static void test_empty_header_line_at_buffer_start(void) {
  fprintf(stderr, "  * empty header line at buffer start (split)\n");
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  parser_state_s st = {0};
  char part1[] = "GET / HTTP/1.1\nHost: a\n";
  size_t r1 = fio_http1_parse(&parser,
                              FIO_BUF_INFO2(part1, sizeof(part1) - 1),
                              &st);
  FIO_ASSERT(r1 == sizeof(part1) - 1,
             "split empty-line: part1 not fully consumed");
  char *p = (char *)FIO_MEM_REALLOC(NULL, 0, 1, 0);
  FIO_ASSERT_ALLOC(p);
  p[0] = '\n';
  size_t r2 = fio_http1_parse(&parser, FIO_BUF_INFO2(p, 1), &st);
  FIO_MEM_FREE(p, 1);
  FIO_ASSERT(r2 != FIO_HTTP1_PARSER_ERROR,
             "split empty-line: parser returned error");
  FIO_ASSERT(st.complete, "split empty-line: request should be complete");
}

/* ===========================================================================
 * Regression test (2026-07-31 audit): whitespace-skip bounds order (CWE-125)
 *
 * The request-line whitespace skip evaluated `start[0]` before the bounds
 * check, reading one byte past an all-whitespace buffer. Feed an exact-size
 * heap buffer of spaces only; AddressSanitizer (with FIO_MEMORY_DISABLE)
 * detects any recurrence. The parser must simply wait for more data.
 * ===========================================================================
 */
static void test_whitespace_only_buffer(void) {
  fprintf(stderr, "  * all-whitespace buffer\n");
  char *p = (char *)FIO_MEM_REALLOC(NULL, 0, 4, 0);
  FIO_ASSERT_ALLOC(p);
  FIO_MEMSET(p, ' ', 4);
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  parser_state_s st = {0};
  size_t r = fio_http1_parse(&parser, FIO_BUF_INFO2(p, 4), &st);
  FIO_MEM_FREE(p, 4);
  FIO_ASSERT(r != FIO_HTTP1_PARSER_ERROR, "ws only: parser returned error");
  FIO_ASSERT(!st.complete, "ws only: should not be complete");
}

/* ===========================================================================
 * Regression test (2026-07-31 audit): TE list trim loop bounds order
 *
 * `Transfer-Encoding: ,chunked` made the separator-trim loop evaluate
 * `c_start[-1]` after reaching the start of the value (logical over-read).
 * After the fix a separator-only prefix is consumed and the value is treated
 * as plain "chunked".
 * ===========================================================================
 */
static void test_te_separator_only_prefix(void) {
  fprintf(stderr, "  * TE separator-only prefix (',chunked')\n");
  char req[] = "POST / HTTP/1.1\r\nTransfer-Encoding: ,chunked\r\n\r\n0\r\n\r\n";
  parser_state_s st = {0};
  size_t r = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(r != FIO_HTTP1_PARSER_ERROR, "te prefix: parser returned error");
  FIO_ASSERT(st.complete, "te prefix: should be complete");
}

/* ===========================================================================
 * Regression test (2026-07-31 audit, CWE-444 differential parsing):
 * `Content-Length: 1_0` was accepted as 10 (fio_atol10u allows '_' digit
 * separators) - a request-smuggling primitive against strict intermediaries.
 * The strict bounded parser (fio_stol10u) stops at '_', failing the
 * value-end check.
 * ===========================================================================
 */
static void test_content_length_underscore_rejected(void) {
  fprintf(stderr, "  * Content-Length with '_' separator rejected\n");
  char req[] = "POST / HTTP/1.1\r\nContent-Length: 1_0\r\n\r\n";
  parser_state_s st = {0};
  size_t r = run_parse(&st, (char *)req, sizeof(req) - 1);
  FIO_ASSERT(r == FIO_HTTP1_PARSER_ERROR,
             "cl underscore: should be a parse error");
}

/* ===========================================================================
 * Regression test (2026-07-31 audit, CWE-444): chunk sizes accepted a `0x`
 * prefix and '_' separators (fio_atol16u semantics) - both are
 * differential-parsing smuggling primitives. fio_stol16u is strict.
 * ===========================================================================
 */
static void test_chunk_size_non_rfc_rejected(void) {
  fprintf(stderr, "  * chunk size '0x' prefix / '_' separator rejected\n");
  char req1[] = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n0x10\r\n";
  parser_state_s st1 = {0};
  size_t r1 = run_parse(&st1, (char *)req1, sizeof(req1) - 1);
  FIO_ASSERT(r1 == FIO_HTTP1_PARSER_ERROR,
             "chunk size '0x10': should be a parse error");
  char req2[] = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n1_0\r\n";
  parser_state_s st2 = {0};
  size_t r2 = run_parse(&st2, (char *)req2, sizeof(req2) - 1);
  FIO_ASSERT(r2 == FIO_HTTP1_PARSER_ERROR,
             "chunk size '1_0': should be a parse error");
}

/* ===========================================================================
   Expect: 100-continue

   The expect state is a parser flag (never function pointer identity, which
   linker identical-code-folding may merge, i.e. MSVC `/OPT:ICF`). The
   callback fires only for an accepted `Expect` header when a body may follow,
   and exactly once per message (chunked trailers finish through the same
   header-line path).
   ===========================================================================
 */

static void test_expect_100_continue(void) {
  fprintf(stderr, "  * expect: 100-continue\n");
  { /* no Expect header: never fires (GET, POST with body) */
    char get[] = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
    char post[] = "POST / HTTP/1.1\r\nContent-Length: 2\r\n\r\nhi";
    parser_state_s st = {0};
    run_parse(&st, get, sizeof(get) - 1);
    FIO_ASSERT(st.complete && !st.expect, "expect: GET without Expect fired");
    st = (parser_state_s){0};
    run_parse(&st, post, sizeof(post) - 1);
    FIO_ASSERT(st.complete && !st.expect, "expect: POST without Expect fired");
  }
  { /* Expect on a request that may not have a body: never fires */
    char get[] = "GET / HTTP/1.1\r\nExpect: 100-continue\r\n\r\n";
    parser_state_s st = {0};
    size_t r = run_parse(&st, get, sizeof(get) - 1);
    FIO_ASSERT(r == sizeof(get) - 1 && st.complete,
               "expect: GET with Expect should complete");
    FIO_ASSERT(!st.expect, "expect: GET with Expect must not fire");
  }
  { /* Expect with a Content-Length body: fires once */
    char post[] = "POST / HTTP/1.1\r\nExpect: 100-continue\r\n"
                  "Content-Length: 2\r\n\r\nhi";
    parser_state_s st = {0};
    size_t r = run_parse(&st, post, sizeof(post) - 1);
    FIO_ASSERT(r == sizeof(post) - 1 && st.complete && st.body_len == 2,
               "expect: POST with Expect should complete with body");
    FIO_ASSERT(st.expect == 1, "expect: POST should fire once (%d)", st.expect);
  }
  { /* Expect with a chunked body + trailer: fires once, not again */
    char post[] = "POST / HTTP/1.1\r\nExpect: 100-continue\r\n"
                  "Transfer-Encoding: chunked\r\n\r\n"
                  "2\r\nhi\r\n0\r\nX-Trailer: 1\r\n\r\n";
    parser_state_s st = {0};
    size_t r = run_parse(&st, post, sizeof(post) - 1);
    FIO_ASSERT(r == sizeof(post) - 1 && st.complete && st.body_len == 2,
               "expect: chunked POST with Expect should complete");
    FIO_ASSERT(st.expect == 1,
               "expect: chunked POST should fire once (%d)",
               st.expect);
  }
  { /* flag does not leak into the next message on a persistent parser */
    char msgs[] = "POST / HTTP/1.1\r\nExpect: 100-continue\r\n"
                  "Content-Length: 1\r\n\r\na"
                  "POST / HTTP/1.1\r\nContent-Length: 1\r\n\r\nb";
    fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
    parser_state_s st = {0};
    size_t r = run_parse_persist(&st, &parser, msgs, sizeof(msgs) - 1);
    FIO_ASSERT(st.complete && st.expect == 1, "expect: first message");
    st = (parser_state_s){0};
    run_parse_persist(&st, &parser, msgs + r, sizeof(msgs) - 1 - r);
    FIO_ASSERT(st.complete && !st.expect,
               "expect: flag leaked into the next message");
  }
}

/* ===========================================================================
   Messages without a body (RFC 9112 §6.3)

   The parser identifies bodyless messages from the first line, before any
   header callback (`fio_http1_parser_skips_body`): GET / HEAD / OPTIONS
   requests and 1xx / 204 / 304 responses. Responses marked with
   `fio_http1_parser_skip_body` (HEAD) behave the same. A bodyless response
   ends at the empty line: `content-length` is reported to the content-length
   callback (informational - callers must not reserve space), other framing
   headers are forwarded as regular headers, and no payload is consumed.
   ===========================================================================
 */

/** A string literal followed by its length (avoids `strlen`). */
#define TEST_LIT(s) s, sizeof(s) - 1

static void test_no_body_messages(void) {
  fprintf(stderr, "  * messages without a body (1xx / 204 / 304 / HEAD)\n");
  static const struct {
    const char *msg;
    size_t len;
    size_t headers;      /* expected regular header callbacks */
    int cl;              /* expect the content-length callback */
  } bodyless[] = {
      {TEST_LIT("HTTP/1.1 100 Continue\r\nContent-Length: 5\r\n\r\n"), 0, 1},
      {TEST_LIT("HTTP/1.1 103 Early Hints\r\nLink: </s.css>\r\n\r\n"), 1, 0},
      {TEST_LIT("HTTP/1.1 204 No Content\r\nContent-Length: 5\r\n\r\n"), 0, 1},
      {TEST_LIT("HTTP/1.1 304 Not Modified\r\n"
                "Transfer-Encoding: chunked\r\n\r\n"),
       1,
       0},
      {TEST_LIT("HTTP/1.1 304 Not Modified\r\nContent-Length: 5\r\n"
                "Transfer-Encoding: chunked\r\n\r\n"),
       1,
       1},
  };
  for (size_t i = 0; i < sizeof(bodyless) / sizeof(bodyless[0]); ++i) {
    char buf[256];
    const size_t hlen = bodyless[i].len;
    /* trailing bytes belong to the next message and must not be consumed */
    FIO_MEMCPY(buf, bodyless[i].msg, hlen);
    FIO_MEMCPY(buf + hlen, "HTTP/", 5);
    parser_state_s st = {0};
    size_t r = run_parse(&st, buf, hlen + 5);
    FIO_ASSERT(r == hlen && st.complete && !st.body_len,
               "no-body response %zu: consumed %zu of %zu (complete %d)",
               i,
               r,
               hlen,
               st.complete);
    FIO_ASSERT(st.skips_at_status,
               "no-body response %zu: skips_body should be known before "
               "the headers",
               i);
    FIO_ASSERT(st.header_count == bodyless[i].headers,
               "no-body response %zu: framing headers should be forwarded "
               "as regular headers (got %zu)",
               i,
               st.header_count);
    FIO_ASSERT(st.cl_received == bodyless[i].cl,
               "no-body response %zu: content-length callback mismatch",
               i);
    FIO_ASSERT(!st.cl_received || (st.skips_at_cl && st.content_length == 5),
               "no-body response %zu: content-length callback should see "
               "skips_body and the informational value",
               i);
  }
  { /* status codes that do carry a body still read it */
    char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
    parser_state_s st = {0};
    size_t r = run_parse(&st, resp, sizeof(resp) - 1);
    FIO_ASSERT(r == sizeof(resp) - 1 && st.complete && st.body_len == 2,
               "200 response should read its body");
    FIO_ASSERT(!st.skips_at_status && !st.skips_at_cl,
               "200 response must not report skips_body");
  }
  { /* requests: GET / HEAD / OPTIONS known bodyless before any header */
    static const struct {
      const char *msg;
      size_t len;
    } reqs[] = {
        {TEST_LIT("GET / HTTP/1.1\r\nHost: x\r\n\r\n")},
        {TEST_LIT("HEAD / HTTP/1.1\r\nHost: x\r\n\r\n")},
        {TEST_LIT("OPTIONS * HTTP/1.1\r\nHost: x\r\n\r\n")},
    };
    for (size_t i = 0; i < 3; ++i) {
      parser_state_s st = {0};
      char buf[64];
      const size_t len = reqs[i].len;
      FIO_MEMCPY(buf, reqs[i].msg, len);
      run_parse(&st, buf, len);
      FIO_ASSERT(st.complete && st.skips_at_header,
                 "request %zu: skips_body should be known before headers",
                 i);
    }
    char post[] = "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 2\r\n\r\nhi";
    parser_state_s st = {0};
    run_parse(&st, post, sizeof(post) - 1);
    FIO_ASSERT(st.complete && !st.skips_at_header && !st.skips_at_cl &&
                   st.body_len == 2,
               "POST request must not report skips_body");
  }
  { /* HEAD response: user marks the message from on_status */
    char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\n"
                  "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
    const size_t first =
        sizeof("HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\n") - 1;
    fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
    parser_state_s st = {0};
    st.mark_skip_body = 1;
    size_t r = run_parse_persist(&st, &parser, resp, sizeof(resp) - 1);
    FIO_ASSERT(r == first && st.complete && !st.body_len,
               "HEAD response: should complete at the empty line (%zu/%zu)",
               r,
               first);
    FIO_ASSERT(st.cl_received && st.skips_at_cl && st.content_length == 5,
               "HEAD response: content-length should be reported with "
               "skips_body set");
    /* the mark is per message: the next response reads its body */
    st = (parser_state_s){0};
    size_t r2 =
        run_parse_persist(&st, &parser, resp + r, sizeof(resp) - 1 - r);
    FIO_ASSERT(r2 == sizeof(resp) - 1 - r && st.complete && st.body_len == 2,
               "HEAD mark leaked into the next response");
    FIO_ASSERT(!st.skips_at_cl, "HEAD mark leaked into skips_body");
  }
  { /* interim 1xx followed by the final response on one stream */
    char resp[] = "HTTP/1.1 100 Continue\r\n\r\n"
                  "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
    fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
    parser_state_s st = {0};
    size_t r = run_parse_persist(&st, &parser, resp, sizeof(resp) - 1);
    FIO_ASSERT(st.complete && st.status == 100 && !st.body_len,
               "1xx stream: interim response should complete first");
    st = (parser_state_s){0};
    run_parse_persist(&st, &parser, resp + r, sizeof(resp) - 1 - r);
    FIO_ASSERT(st.complete && st.status == 200 && st.body_len == 2,
               "1xx stream: final response should follow with its body");
  }
}
#undef TEST_LIT

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
  test_empty_header_line_at_buffer_start();
  test_whitespace_only_buffer();
  test_te_separator_only_prefix();
  test_content_length_underscore_rejected();
  test_chunk_size_non_rfc_rejected();
  test_expect_100_continue();
  test_no_body_messages();
  fprintf(stderr, "All HTTP/1 parser tests passed!\n");
  return 0;
}
