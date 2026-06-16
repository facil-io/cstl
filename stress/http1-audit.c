/* *****************************************************************************
Stress/Audit - HTTP/1 parser fuzz/audit harness (431 http1 parser.h)

Audit harness ported from tests-old/http1-audit.c.
Runs manually or on request. Exercises the HTTP/1 parser with oversized
tokens and parser-dependent edge cases that go beyond the correctness tests
in ./tests/http1-parser.c and ./tests/http-handle.c.

Correctness assertions already covered by ./tests/http1-parser.c (basic
requests, LF-only line endings, leading whitespace, response lines,
Content-Length and chunked bodies including fragmentation, CL+TE conflict,
bad/negative chunk sizes, NUL in URI/header, missing colon, many headers,
and parser state queries) have been removed.

No external processes are spawned.
***************************************************************************** */
#include "tests/test-helpers.h"

#define FIO_HTTP1_PARSER
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Configuration
***************************************************************************** */

#define HTTP1_AUDIT_LONG_LEN 70000
#define HTTP1_AUDIT_REPEAT   1000

/* *****************************************************************************
Callbacks
***************************************************************************** */

static void fio_http1_on_complete(void *udata) { (void)udata; }
static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  (void)method;
  (void)udata;
  return 0;
}
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata) {
  (void)istatus;
  (void)status;
  (void)udata;
  return 0;
}
static int fio_http1_on_url(fio_buf_info_s path, void *udata) {
  (void)path;
  (void)udata;
  return 0;
}
static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  (void)version;
  (void)udata;
  return 0;
}
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  (void)name;
  (void)value;
  (void)udata;
  return 0;
}
static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata) {
  (void)name;
  (void)value;
  (void)content_length;
  (void)udata;
  return 0;
}
static int fio_http1_on_expect(void *udata) {
  (void)udata;
  return 0;
}
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  (void)chunk;
  (void)udata;
  return 0;
}

/* *****************************************************************************
Helpers
***************************************************************************** */

static size_t run_parse(char *buf, size_t len) {
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  return fio_http1_parse(&parser, FIO_BUF_INFO2(buf, len), NULL);
}

static size_t run_parse_persist(fio_http1_parser_s *parser,
                                char *buf,
                                size_t len) {
  return fio_http1_parse(parser, FIO_BUF_INFO2(buf, len), NULL);
}

/* *****************************************************************************
Audit cases
***************************************************************************** */

static int audit_te_comma(void) {
  fprintf(stderr, "\t- te_comma\n");
  static const char req[] =
      "POST / HTTP/1.1\r\nTransfer-Encoding: ,,,,chunked\r\n\r\n";
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, sizeof(req) - 1, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, req, sizeof(req) - 1);
  size_t result = run_parse(buf, sizeof(req) - 1);
  /* parser-dependent: may reject or consume all */
  (void)result;
  FIO_MEM_FREE(buf, sizeof(req) - 1);
  return 0;
}

static int audit_long_method(void) {
  fprintf(stderr, "\t- long_method\n");
  size_t method_len = HTTP1_AUDIT_LONG_LEN;
  size_t total = method_len + 14;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMSET(buf, 'X', method_len);
  FIO_MEMCPY(buf + method_len, " / HTTP/1.1\r\n\r\n", 14);
  for (size_t i = 0; i < HTTP1_AUDIT_REPEAT; ++i) {
    size_t result = run_parse(buf, total);
    (void)result;
  }
  FIO_MEM_FREE(buf, total);
  return 0;
}

static int audit_long_header_name(void) {
  fprintf(stderr, "\t- long_header_name\n");
  size_t name_len = HTTP1_AUDIT_LONG_LEN;
  size_t total = 16 + name_len + 6;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, "GET / HTTP/1.1\r\n", 16);
  FIO_MEMSET(buf + 16, 'A', name_len);
  FIO_MEMCPY(buf + 16 + name_len, ": x\r\n\r\n", 6);
  for (size_t i = 0; i < HTTP1_AUDIT_REPEAT; ++i) {
    size_t result = run_parse(buf, total);
    (void)result;
  }
  FIO_MEM_FREE(buf, total);
  return 0;
}

static int audit_long_url(void) {
  fprintf(stderr, "\t- long_url\n");
  size_t path_len = HTTP1_AUDIT_LONG_LEN;
  size_t total = 5 + path_len + 13;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, "GET /", 5);
  FIO_MEMSET(buf + 5, 'a', path_len);
  FIO_MEMCPY(buf + 5 + path_len, " HTTP/1.1\r\n\r\n", 13);
  for (size_t i = 0; i < HTTP1_AUDIT_REPEAT; ++i) {
    size_t result = run_parse(buf, total);
    (void)result;
  }
  FIO_MEM_FREE(buf, total);
  return 0;
}

static int audit_long_header_value(void) {
  fprintf(stderr, "\t- long_header_value\n");
  size_t value_len = HTTP1_AUDIT_LONG_LEN;
  size_t total = 19 + value_len + 4;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, "GET / HTTP/1.1\r\nH: ", 19);
  FIO_MEMSET(buf + 19, 'v', value_len);
  FIO_MEMCPY(buf + 19 + value_len, "\r\n\r\n", 4);
  for (size_t i = 0; i < HTTP1_AUDIT_REPEAT; ++i) {
    size_t result = run_parse(buf, total);
    (void)result;
  }
  FIO_MEM_FREE(buf, total);
  return 0;
}

static int audit_many_small_chunks(void) {
  fprintf(stderr, "\t- many_small_chunks\n");
  size_t chunk_count = 10000;
  size_t chunk_size = 1;
  size_t chunk_line_len = 3; /* "1\r\n" */
  size_t total = 40 + chunk_count * (chunk_line_len + chunk_size + 2) + 5;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n", 40);
  size_t pos = 40;
  for (size_t i = 0; i < chunk_count; ++i) {
    FIO_MEMCPY(buf + pos, "1\r\n", 3);
    pos += 3;
    buf[pos++] = 'x';
    FIO_MEMCPY(buf + pos, "\r\n", 2);
    pos += 2;
  }
  FIO_MEMCPY(buf + pos, "0\r\n\r\n", 5);
  pos += 5;
  size_t result = run_parse(buf, pos);
  (void)result;
  FIO_MEM_FREE(buf, total);
  return 0;
}

static int audit_chunk_extensions(void) {
  fprintf(stderr, "\t- chunk_extensions\n");
  static const char req[] =
      "POST / HTTP/1.1\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "5;ext=val\r\nHello\r\n"
      "0\r\n\r\n";
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, sizeof(req) - 1, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, req, sizeof(req) - 1);
  size_t result = run_parse(buf, sizeof(req) - 1);
  (void)result;
  FIO_MEM_FREE(buf, sizeof(req) - 1);
  return 0;
}

static int audit_mixed_line_endings(void) {
  fprintf(stderr, "\t- mixed_line_endings\n");
  static const char req[] = "GET / HTTP/1.1\r\nHost: x\n\r\n";
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, sizeof(req) - 1, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, req, sizeof(req) - 1);
  size_t result = run_parse(buf, sizeof(req) - 1);
  (void)result;
  FIO_MEM_FREE(buf, sizeof(req) - 1);
  return 0;
}

static int audit_many_requests_persist(void) {
  fprintf(stderr, "\t- many_requests_persist\n");
  static const char req[] = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
  fio_http1_parser_s parser = FIO_HTTP1_PARSER_INIT;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, sizeof(req) - 1, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, req, sizeof(req) - 1);
  for (size_t i = 0; i < HTTP1_AUDIT_REPEAT; ++i) {
    size_t result = run_parse_persist(&parser, buf, sizeof(req) - 1);
    (void)result;
  }
  FIO_MEM_FREE(buf, sizeof(req) - 1);
  return 0;
}

static int audit_whitespace_in_header_name(void) {
  fprintf(stderr, "\t- whitespace_in_header_name\n");
  static const char req[] = "GET / HTTP/1.1\r\nHost Name: x\r\n\r\n";
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, sizeof(req) - 1, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, req, sizeof(req) - 1);
  size_t result = run_parse(buf, sizeof(req) - 1);
  (void)result;
  FIO_MEM_FREE(buf, sizeof(req) - 1);
  return 0;
}

static int audit_duplicate_content_length(void) {
  fprintf(stderr, "\t- duplicate_content_length\n");
  static const char req[] =
      "POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 5\r\n\r\nHello";
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, sizeof(req) - 1, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, req, sizeof(req) - 1);
  size_t result = run_parse(buf, sizeof(req) - 1);
  (void)result;
  FIO_MEM_FREE(buf, sizeof(req) - 1);
  return 0;
}

static int audit_content_length_whitespace(void) {
  fprintf(stderr, "\t- content_length_whitespace\n");
  static const char req[] =
      "POST / HTTP/1.1\r\nContent-Length:   5  \r\n\r\nHello";
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, sizeof(req) - 1, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMCPY(buf, req, sizeof(req) - 1);
  size_t result = run_parse(buf, sizeof(req) - 1);
  (void)result;
  FIO_MEM_FREE(buf, sizeof(req) - 1);
  return 0;
}

static int audit_many_empty_lines(void) {
  fprintf(stderr, "\t- many_empty_lines\n");
  size_t empty_count = 10000;
  size_t total = 16 + empty_count * 2;
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, total, 0);
  FIO_ASSERT_ALLOC(buf);
  size_t pos = 0;
  for (size_t i = 0; i < empty_count; ++i) {
    FIO_MEMCPY(buf + pos, "\r\n", 2);
    pos += 2;
  }
  FIO_MEMCPY(buf + pos, "GET / HTTP/1.1\r\n\r\n", 16);
  size_t result = run_parse(buf, total);
  (void)result;
  FIO_MEM_FREE(buf, total);
  return 0;
}

/* *****************************************************************************
Case registry
***************************************************************************** */

typedef struct {
  const char *name;
  int (*fn)(void);
} http1_audit_case_s;

static const http1_audit_case_s http1_audit_cases[] = {
    {.name = "te_comma", .fn = audit_te_comma},
    {.name = "long_method", .fn = audit_long_method},
    {.name = "long_header_name", .fn = audit_long_header_name},
    {.name = "long_url", .fn = audit_long_url},
    {.name = "long_header_value", .fn = audit_long_header_value},
    {.name = "many_small_chunks", .fn = audit_many_small_chunks},
    {.name = "chunk_extensions", .fn = audit_chunk_extensions},
    {.name = "mixed_line_endings", .fn = audit_mixed_line_endings},
    {.name = "many_requests_persist", .fn = audit_many_requests_persist},
    {.name = "whitespace_in_header_name", .fn = audit_whitespace_in_header_name},
    {.name = "duplicate_content_length", .fn = audit_duplicate_content_length},
    {.name = "content_length_whitespace", .fn = audit_content_length_whitespace},
    {.name = "many_empty_lines", .fn = audit_many_empty_lines},
    {NULL, NULL},
};

/* *****************************************************************************
Main entry point
***************************************************************************** */

int main(int argc, char **argv) {
  if (FIO_LOG_LEVEL == FIO_LOG_LEVEL_INFO)
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING;

  fprintf(stderr, "=== HTTP/1 audit harness ===\n");

  int r = 0;
  if (argc > 1) {
    for (const http1_audit_case_s *c = http1_audit_cases; c->name; ++c) {
      if (!strcmp(argv[1], c->name)) {
        r = c->fn();
        fprintf(stderr, "=== HTTP/1 audit harness %s ===\n",
                r ? "FAILED" : "passed");
        return r;
      }
    }
    fprintf(stderr, "unknown case: %s\n", argv[1]);
    return 2;
  }

  for (const http1_audit_case_s *c = http1_audit_cases; c->name; ++c) {
    if (c->fn()) {
      r = 1;
    }
  }

  fprintf(stderr,
          "=== HTTP/1 audit harness %s ===\n",
          r ? "FAILED" : "passed");
  return r;
}
