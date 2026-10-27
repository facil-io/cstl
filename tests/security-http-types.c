/* *****************************************************************************
Test: HTTP types security regressions (432 http types.h)

- HTTP-01: a '=' after the first ';' (Cookie / Set-Cookie) never yields a
  wrapped length or out-of-bounds read.
- HTTP-02: 304 (If-None-Match) and 416 (bad Range) static responses close the
  file descriptor.
- HTTP-03: enabled string caches honor FIO_HTTP_CACHE_LIMIT; evicted entries
  stay valid for handles that reference them.
- HTTP-04: cookie formatting never writes past its buffer.
- HTTP-05: compression merges `accept-encoding` into an existing Vary.
- HTTP-06: the default EventSource -> on_message redirect does not clobber the
  active SSE union state; an empty / missing path resolves to the root route.

All checks use FIO_ASSERT (active under NDEBUG).
***************************************************************************** */

#define FIO_HTTP
#define FIO_HTTP_CACHE_LIMIT 8
#define FIO_HTTP_TEST_COMPRESSION_FLAGS
#include "test-helpers.h"

#define S(s) FIO_STR_INFO2((char *)(s), sizeof(s) - 1)

static int str_eq(fio_str_info_s a, const char *b) {
  size_t l = strlen(b);
  return a.len == l && (!l || !memcmp(a.buf, b, l));
}

/* *****************************************************************************
HTTP-01
***************************************************************************** */

static void test_cookie_parse(void) {
  fprintf(stderr, "  * HTTP-01 cookie pair delimiting\n");
  { /* request Cookie */
    fio_http_s *h = fio_http_new();
    fio_http_request_header_add(h, S("cookie"), S("a;b=c"));
    fio_http_request_header_add(h, S("cookie"), S("x=1;;  ;y=%41;z;w=2;"));
    fio_http_request_header_add(h, S("cookie"), S(";"));
    FIO_ASSERT(str_eq(fio_http_cookie(h, "b", 1), "c"), "a;b=c: b != c");
    FIO_ASSERT(!fio_http_cookie(h, "a", 1).len, "a;b=c: 'a' is a value");
    FIO_ASSERT(str_eq(fio_http_cookie(h, "x", 1), "1"), "x != 1");
    FIO_ASSERT(str_eq(fio_http_cookie(h, "y", 1), "A"), "y != A (decoded)");
    FIO_ASSERT(str_eq(fio_http_cookie(h, "w", 1), "2"), "w != 2");
    FIO_ASSERT(!fio_http_cookie(h, "z", 1).len, "z has no name");
    fio_http_free(h);
  }
  { /* response Set-Cookie (client side, headers not sent) */
    fio_http_s *h = fio_http_new();
    fio_http_response_header_add(h, S("set-cookie"), S("a;b=c"));
    fio_http_response_header_add(h, S("set-cookie"), S("  "));
    fio_http_response_header_add(h, S("set-cookie"), S("sid=1; Path=/"));
    fio_http_response_header_add(h, S("set-cookie"), S("__Host-k=v"));
    FIO_ASSERT(!fio_http_cookie(h, "b", 1).len, "Set-Cookie 'b' is attr");
    FIO_ASSERT(!fio_http_cookie(h, "a", 1).len, "Set-Cookie 'a' is no pair");
    FIO_ASSERT(str_eq(fio_http_cookie(h, "sid", 3), "1"), "sid != 1");
    FIO_ASSERT(str_eq(fio_http_cookie(h, "k", 1), "v"), "__Host-k != v");
    fio_http_free(h);
  }
}

/* *****************************************************************************
HTTP-02
***************************************************************************** */
#if FIO_OS_POSIX
static int next_fd(void) {
  int fd = dup(0);
  FIO_ASSERT(fd != -1, "dup failed");
  close(fd);
  return fd;
}

static void test_static_fd_exits(void) {
  fprintf(stderr, "  * HTTP-02 static file 304 / 416 close the fd\n");
  char dir[128], path[192];
  snprintf(dir, sizeof(dir), "tmp/test-http-types-%d", (int)getpid());
  snprintf(path, sizeof(path), "%s/f.txt", dir);
  fio_filename_remove(.path = dir, .recursive = 1);
  FIO_ASSERT(!fio_filename_make_path(.path = dir), "mkdir %s", dir);
  FILE *f = fopen(path, "wb");
  FIO_ASSERT(f && fwrite("0123456789", 1, 10, f) == 10 && !fclose(f),
             "write %s",
             path);
  char etag[64];
  size_t etag_len;
  { /* HEAD: learn the ETag */
    fio_http_s *h = fio_http_new();
    fio_http_method_set(h, S("HEAD"));
    FIO_ASSERT(
        !fio_http_static_file_response(h, FIO_STR_INFO1(dir), S("/f.txt"), 0),
        "HEAD failed");
    fio_str_info_s e = fio_http_response_header(h, S("etag"), 0);
    FIO_ASSERT(e.len && e.len < sizeof(etag), "missing etag");
    memcpy(etag, e.buf, e.len);
    etag_len = e.len;
    fio_http_free(h);
  }
  const int base = next_fd();
  for (int i = 0; i < 4; ++i) { /* 304 */
    fio_http_s *h = fio_http_new();
    fio_http_status_set(h, 200); /* as set by the HTTP glue before routing */
    fio_http_method_set(h, S("GET"));
    fio_http_request_header_set(h,
                                S("if-none-match"),
                                FIO_STR_INFO2(etag, etag_len));
    FIO_ASSERT(
        !fio_http_static_file_response(h, FIO_STR_INFO1(dir), S("/f.txt"), 0),
        "304 path should report handled");
    FIO_ASSERT(fio_http_status(h) == 304,
               "expected 304, got %zu",
               fio_http_status(h));
    fio_http_free(h);
    FIO_ASSERT(next_fd() == base, "304 leaked a file descriptor");
  }
  for (int i = 0; i < 4; ++i) { /* 416 */
    fio_http_s *h = fio_http_new();
    fio_http_status_set(h, 200); /* as set by the HTTP glue before routing */
    fio_http_method_set(h, S("GET"));
    fio_http_request_header_set(h, S("range"), S("bytes=5-2"));
    fio_http_static_file_response(h, FIO_STR_INFO1(dir), S("/f.txt"), 0);
    FIO_ASSERT(fio_http_status(h) == 416,
               "expected 416, got %zu",
               fio_http_status(h));
    fio_http_free(h);
    FIO_ASSERT(next_fd() == base, "416 leaked a file descriptor");
  }
  FIO_ASSERT(!fio_filename_remove(.path = dir, .recursive = 1), "rm %s", dir);
}
#endif

/* *****************************************************************************
HTTP-03
***************************************************************************** */

static void test_cache_limit(void) {
  fprintf(stderr,
          "  * HTTP-03 string cache limit (%d)\n",
          FIO_HTTP_CACHE_LIMIT);
  fio___http_str_cache_s *c =
      &FIO___HTTP_STRING_CACHE[FIO___HTTP_STR_CACHE_VALUE].cache;
  fio_http_s *h = fio_http_new();
  char buf[32];
  for (int i = 0; i < 200; ++i) {
    int l = snprintf(buf, sizeof(buf), "unique-%d", i);
    fio_http_request_header_add(h, S("x-u"), FIO_STR_INFO2(buf, (size_t)l));
    FIO_ASSERT(fio___http_str_cache_count(c) <= FIO_HTTP_CACHE_LIMIT,
               "cache grew past its limit: %zu",
               (size_t)fio___http_str_cache_count(c));
  }
  for (int i = 0; i < 200; ++i) { /* evicted entries remain referenced */
    snprintf(buf, sizeof(buf), "unique-%d", i);
    FIO_ASSERT(str_eq(fio_http_request_header(h, S("x-u"), (size_t)i), buf),
               "header value %d corrupted after eviction",
               i);
  }
  fio_http_free(h);
}

/* *****************************************************************************
HTTP-04
***************************************************************************** */

static int set_cookie_cb(fio_http_s *h,
                         fio_str_info_s n,
                         fio_str_info_s v,
                         void *udata) {
  *(fio_str_info_s *)udata = v;
  (void)h, (void)n;
  return 1;
}
static fio_str_info_s set_cookie_get(fio_http_s *h) {
  fio_str_info_s r = {0};
  fio_http_set_cookie_each(h, set_cookie_cb, &r);
  return r;
}

static void test_cookie_format(void) {
  fprintf(stderr, "  * HTTP-04 cookie formatting bounds\n");
  enum { N = 3000 }; /* 3x expansion: name + value exceed the stack buffer */
  char *bad = (char *)malloc(N + 1);
  char *dom = (char *)malloc(6001);
  FIO_ASSERT(bad && dom, "malloc");
  memset(bad, 0x01, N);
  bad[N] = 0;
  memset(dom, 'd', 6000);
  dom[6000] = 0;
  { /* success: exact encoded output, C-string (zero len) name */
    fio_http_s *h = fio_http_new();
    FIO_ASSERT(!fio_http_cookie_set(h,
                                    .name = FIO_STR_INFO2((char *)"n", 0),
                                    .value = FIO_STR_INFO2(bad, N),
                                    .secure = 1),
               "cookie_set failed");
    fio_str_info_s v = set_cookie_get(h);
    FIO_ASSERT(v.len == 2 + (3 * N) + 2 + 7 && !memcmp(v.buf, "n=%01%01", 8) &&
                   !memcmp(v.buf + v.len - 9, "; secure;", 9),
               "unexpected set-cookie (%zu bytes)",
               v.len);
    FIO_ASSERT(str_eq(fio_http_cookie(h, "n", 1), bad), "read store / name");
    fio_http_free(h);
  }
  free(dom);
  free(bad);
}

/* *****************************************************************************
HTTP-05
***************************************************************************** */

static size_t vary_count(fio_http_s *h, const char *token) {
  size_t r = 0;
  fio_str_info_s v;
  for (size_t i = 0; (v = fio_http_response_header(h, S("vary"), i)).buf; ++i)
    r += (size_t)fio___http_header_has_token(v, token, strlen(token));
  return r;
}

static void test_vary_merge(void) {
  fprintf(stderr, "  * HTTP-05 compression preserves Vary\n");
  enum { BODY_LEN = 2048 };
  char body[BODY_LEN];
  for (size_t i = 0; i < BODY_LEN; ++i)
    body[i] = (char)('a' + (i & 15));
  const char *offers[] = {"gzip", "gzip;q=0", NULL};
  for (size_t o = 0; o < 3; ++o) {
    fio_http_s *h = fio_http_new();
    fio___http_test_enable_compression(h, 0);
    fio_http_method_set(h, S("GET"));
    fio_http_status_set(h, 200);
    if (offers[o])
      fio_http_request_header_set(h,
                                  S("accept-encoding"),
                                  FIO_STR_INFO1((char *)offers[o]));
    fio_http_response_header_set(h, S("content-type"), S("text/plain"));
    fio_http_response_header_add(h, S("vary"), S("Cookie, Origin"));
    fio_http_write(h, .buf = body, .len = BODY_LEN, .finish = 1, .copy = 1);
    FIO_ASSERT(vary_count(h, "cookie") == 1 && vary_count(h, "origin") == 1,
               "offer %zu: existing Vary dimensions dropped",
               o);
    FIO_ASSERT(vary_count(h, "accept-encoding") == 1,
               "offer %zu: accept-encoding missing or duplicated",
               o);
    if (!o)
      FIO_ASSERT(fio_http_response_header(h, S("content-encoding"), 0).len,
                 "gzip offer should compress");
    fio_http_free(h);
  }
  { /* Vary: * stays as is */
    fio_http_s *h = fio_http_new();
    fio___http_test_enable_compression(h, 0);
    fio_http_method_set(h, S("GET"));
    fio_http_status_set(h, 200);
    fio_http_request_header_set(h, S("accept-encoding"), S("gzip"));
    fio_http_response_header_set(h, S("content-type"), S("text/plain"));
    fio_http_response_header_set(h, S("vary"), S("*"));
    fio_http_write(h, .buf = body, .len = BODY_LEN, .finish = 1, .copy = 1);
    FIO_ASSERT(fio_http_response_header_count(h, S("vary")) == 1 &&
                   str_eq(fio_http_response_header(h, S("vary"), 0), "*"),
               "Vary: * must be left unchanged");
    fio_http_free(h);
  }
}

/* *****************************************************************************
HTTP-06
***************************************************************************** */

static size_t sse_redirect_calls;
static void sse_on_message(fio_http_s *h, fio_buf_info_s msg, uint8_t is_text) {
  ++sse_redirect_calls;
  FIO_ASSERT(msg.len == 4 && !memcmp(msg.buf, "data", 4) && is_text,
             "redirected message mismatch");
  (void)h;
}
static void sse_on_ready(fio_http_s *h) { (void)h; }

static void test_sse_redirect(void) {
  fprintf(stderr, "  * HTTP-06 SSE redirect keeps the SSE union state\n");
  fio___http_protocol_s *p = (fio___http_protocol_s *)calloc(1, sizeof(*p));
  fio___http_connection_s *c = (fio___http_connection_s *)calloc(1, sizeof(*c));
  FIO_ASSERT(p && c, "calloc");
  p->router.s.on_message = sse_on_message;
  c->settings = &p->settings;
  char id[] = "id-1";
  c->state.sse.on_message = fio___http_default_on_eventsource_redirect;
  c->state.sse.on_ready = sse_on_ready;
  c->state.sse.id = FIO_BUF_INFO2(id, 4);
  fio_http_s *h = fio_http_new();
  fio_http_cdata_set(h, c);
  /* empty / missing path => root route (no NULL dereference) */
  FIO_ASSERT(fio_http_settings(h) == &p->router.s, "empty path != root");
  fio_str_info_s empty = FIO_STR_INFO2(NULL, 0);
  FIO_ASSERT(fio___http_route_settings(&p->router, &empty) == &p->router.s,
             "NULL path != root");
  fio_http_opath_set(h, S("/"));
  for (int i = 0; i < 3; ++i) {
    c->state.sse.on_message(h,
                            FIO_BUF_INFO2(id, 4),
                            FIO_BUF_INFO2(NULL, 0),
                            FIO_BUF_INFO2((char *)"data", 4));
    FIO_ASSERT(c->state.sse.on_ready == sse_on_ready,
               "redirect clobbered state.sse.on_ready");
    FIO_ASSERT(c->state.sse.id.buf == id && c->state.sse.id.len == 4,
               "redirect clobbered state.sse.id");
  }
  FIO_ASSERT(sse_redirect_calls == 3, "on_message not called");
  fio_http_cdata_set(h, NULL);
  fio_http_free(h);
  free(c);
  free(p);
}

int main(void) {
  fprintf(stderr, "Testing HTTP types security regressions\n");
  test_cookie_parse();
#if FIO_OS_POSIX
  test_static_fd_exits();
#endif
  test_cache_limit();
  test_cookie_format();
  test_vary_merge();
  test_sse_redirect();
  fprintf(stderr, "HTTP types security regressions passed\n");
  return 0;
}
