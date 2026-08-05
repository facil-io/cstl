/* *****************************************************************************
Test: high-level HTTP module behavior (fio-stl/439 http.h)

Correctness-only coverage for the HTTP listener, router, resource-action
helper, static-file serving, error responses, and WebSocket/SSE upgrade
helpers that wrap the HTTP handle.

No performance loops, no external processes, no external network calls.
Loopback sockets are used only for listener creation (no reactor is run).
***************************************************************************** */
#define FIO_HTTP
#include "test-helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===========================================================================
   Helpers
   ===========================================================================
 */

static fio_http_s *test_http_make_handle(const char *method, const char *path) {
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(h, "fio_http_new returned NULL");
  fio_http_method_set(h, FIO_STR_INFO1((char *)method));
  fio_http_path_set(h, FIO_STR_INFO1((char *)path));
  return h;
}

static void test_http_noop_on_http(fio_http_s *h) { (void)h; }
static void test_http_api_on_http(fio_http_s *h) { (void)h; }
static void test_http_apiv2_on_http(fio_http_s *h) { (void)h; }

/* ===========================================================================
   Resource action detection
   ===========================================================================
 */

static void test_resource_action(void) {
  fprintf(stderr, "  * resource action detection\n");

  /* GET family */
  {
    fio_http_s *h = test_http_make_handle("GET", "/");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_INDEX,
               "GET / should be INDEX");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("GET", "/items");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_SHOW,
               "GET /items should be SHOW");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("GET", "/items/new");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_SHOW,
               "GET /items/new should be SHOW (only /new prefix is NEW)");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("GET", "/new");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_NEW,
               "GET /new should be NEW");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("GET", "/new/items");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_NEW,
               "GET /new/items should be NEW");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("GET", "/items/123/edit");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_EDIT,
               "GET /items/123/edit should be EDIT");
    fio_http_free(h);
  }

  /* POST / PUT / PATCH */
  {
    fio_http_s *h = test_http_make_handle("POST", "/");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_CREATE,
               "POST / should be CREATE");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("POST", "/items");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_UPDATE,
               "POST /items should be UPDATE (per implementation)");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("POST", "/items/123");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_UPDATE,
               "POST /items/123 should be UPDATE");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("PUT", "/items/123");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_UPDATE,
               "PUT /items/123 should be UPDATE");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("PATCH", "/items/123");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_UPDATE,
               "PATCH /items/123 should be UPDATE");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("POST", "/new");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_CREATE,
               "POST /new should be CREATE");
    fio_http_free(h);
  }

  /* DELETE */
  {
    fio_http_s *h = test_http_make_handle("DELETE", "/items/123");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_DELETE,
               "DELETE /items/123 should be DELETE");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("DELETE", "/new");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_NONE,
               "DELETE /new should map to NONE");
    fio_http_free(h);
  }

  /* Edge cases */
  {
    fio_http_s *h = test_http_make_handle("HEAD", "/items");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_NONE,
               "HEAD should map to NONE");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("DELETE", "/");
    FIO_ASSERT(fio_http_resource_action(h) == FIO_HTTP_RESOURCE_NONE,
               "DELETE / should map to NONE");
    fio_http_free(h);
  }
  FIO_ASSERT(fio_http_resource_action(NULL) == FIO_HTTP_RESOURCE_NONE,
             "NULL handle should return NONE");
}

/* ===========================================================================
   Listener, routing, and settings queries
   ===========================================================================
 */

static void test_listen_and_route(void) {
  fprintf(stderr, "  * listener and routing\n");

  fio_http_listener_s *l = fio_http_listen("tcp://127.0.0.1:0",
                                           .udata = (void *)(uintptr_t)0xABCD,
                                           .on_http = test_http_noop_on_http);
  FIO_ASSERT(l, "fio_http_listen should succeed on loopback port 0");

  fio_http_settings_s *defs = fio_http_listener_settings(l);
  FIO_ASSERT(defs, "listener settings should not be NULL");
  FIO_ASSERT(defs->udata == (void *)(uintptr_t)0xABCD,
             "listener settings udata mismatch");

  /* default route fallback */
  fio_http_settings_s *root_s = fio_http_route_settings(l, "/");
  FIO_ASSERT(root_s && root_s->udata == (void *)(uintptr_t)0xABCD,
             "root route settings should have listener default udata");
  (void)defs;

  /* add nested routes */
  FIO_ASSERT(fio_http_route(l,
                            "/api",
                            .udata = (void *)(uintptr_t)0x1111,
                            .on_http = test_http_api_on_http) == 0,
             "route /api should succeed");
  FIO_ASSERT(fio_http_route(l,
                            "/api/v2",
                            .udata = (void *)(uintptr_t)0x2222,
                            .on_http = test_http_apiv2_on_http) == 0,
             "route /api/v2 should succeed");

  /* best-prefix matching */
  fio_http_settings_s *s = fio_http_route_settings(l, "/api/v2/users");
  FIO_ASSERT(s && s->udata == (void *)(uintptr_t)0x2222,
             "/api/v2/users should match /api/v2 route");

  s = fio_http_route_settings(l, "/api/v2");
  FIO_ASSERT(s && s->udata == (void *)(uintptr_t)0x2222,
             "/api/v2 should match exact /api/v2 route");

  s = fio_http_route_settings(l, "/api");
  FIO_ASSERT(s && s->udata == (void *)(uintptr_t)0x1111,
             "/api should match exact /api route");

  s = fio_http_route_settings(l, "/api/other");
  FIO_ASSERT(s && s->udata == (void *)(uintptr_t)0x1111,
             "/api/other should match /api route");

  s = fio_http_route_settings(l, "/apiz");
  FIO_ASSERT(s && s->udata == (void *)(uintptr_t)0xABCD,
             "/apiz should fall back to default route");

  s = fio_http_route_settings(l, "/unrelated");
  FIO_ASSERT(s && s->udata == (void *)(uintptr_t)0xABCD,
             "/unrelated should fall back to default route");

  fio_io_listen_stop((fio_io_listener_s *)l);
}

static void test_settings_and_io_queries(void) {
  fprintf(stderr, "  * settings and IO queries on unconnected handle\n");

  fio_http_s *h = fio_http_new();
  FIO_ASSERT(fio_http_settings(h) == NULL,
             "unconnected handle should have no HTTP settings");
  FIO_ASSERT(fio_http_io(h) == NULL,
             "unconnected handle should have no IO object");
  fio_http_free(h);

  FIO_ASSERT(FIO_HTTP_AUTHENTICATE_ALLOW(NULL) == 0,
             "allow authentication should always return 0");
}

/* ===========================================================================
   Static file serving and error responses
   ===========================================================================
 */

static void test_static_file_response(void) {
  fprintf(stderr, "  * static file response\n");

  /* Build a temp directory path using the same pattern as fio_ipc_url_set. */
  char dir[512];
  const char *options[] = {"TMPDIR", "TMP", "TEMP", NULL};
  const char *tmpdir = NULL;
  for (size_t i = 0; !tmpdir && options[i]; ++i) {
    tmpdir = fio_sys_env(options[i]);
  }
  size_t tmplen = tmpdir ? FIO_STRLEN(tmpdir) : 0;
  if (!tmpdir || tmplen > 128) {
#if FIO_OS_WIN
    tmpdir = ".";
    tmplen = 1;
#else
    tmpdir = "/tmp/";
    tmplen = FIO_STRLEN(tmpdir);
#endif
  }
  FIO_ASSERT(tmplen + 48 < sizeof(dir), "temp directory path too long");
  FIO_MEMCPY(dir, tmpdir, tmplen);
  size_t len = tmplen;
  if (len && dir[len - 1] != '/' && dir[len - 1] != '\\' &&
      dir[len - 1] != FIO_FOLDER_SEPARATOR) {
    dir[len++] = FIO_FOLDER_SEPARATOR;
  }
  FIO_MEMCPY(dir + len, "http_static_test_", 17);
  len += 17;
  len += fio_ltoa(dir + len, (int64_t)fio_rand64(), 16);
  dir[len] = '\0';

  FIO_ASSERT(fio_filename_make_path(.path = dir) == 0,
             "failed to create static test directory");

  char path[512];
  snprintf(path, sizeof(path), "%s%ctest.txt", dir, FIO_FOLDER_SEPARATOR);
  const char *content = "hello static file";
  FILE *f = fopen(path, "w");
  FIO_ASSERT(f, "failed to create static test file");
  FIO_ASSERT(fwrite(content, 1, strlen(content), f) == strlen(content),
             "failed to write static test file");
  fclose(f);

  fio_http_s *h = fio_http_new();
  fio_http_status_set(h, 200);
  int r = fio_http_static_file_response(h,
                                        FIO_STR_INFO2(dir, len),
                                        FIO_STR_INFO1((char *)"/test.txt"),
                                        0);
  FIO_ASSERT(r == 0, "static_file_response should succeed for existing file");
  FIO_ASSERT(fio_http_status(h) == 200,
             "static file response should keep status 200");
  FIO_ASSERT(fio_http_is_finished(h),
             "static file response should finish the response");

  fio_str_info_s ct =
      fio_http_response_header(h, FIO_STR_INFO2((char *)"content-type", 12), 0);
  FIO_ASSERT(ct.len >= 10 && !FIO_MEMCMP(ct.buf, "text/plain", 10),
             "static .txt file should have text/plain content-type");

  fio_http_free(h);
  fio_filename_remove(.path = dir, .recursive = 1);
}

static void test_error_response(void) {
  fprintf(stderr, "  * error response helper\n");

  fio_http_s *h = fio_http_new();
  int r = fio_http_send_error_response(h, 404);
  FIO_ASSERT(r == 0, "send_error_response(404) should succeed");
  FIO_ASSERT(fio_http_status(h) == 404,
             "send_error_response should set status");
  FIO_ASSERT(fio_http_is_finished(h),
             "send_error_response should finish the response");
  fio_str_info_s ct =
      fio_http_response_header(h, FIO_STR_INFO2((char *)"content-type", 12), 0);
  FIO_ASSERT(ct.len >= 9 && !FIO_MEMCMP(ct.buf, "text/plain", 9),
             "error response fallback should be text/plain");

  fio_http_free(h);
}

/* ===========================================================================
   WebSocket / SSE upgrade helpers
   ===========================================================================
 */

static void test_websocket_upgrade_helpers(void) {
  fprintf(stderr, "  * WebSocket upgrade helpers\n");

  fio_http_s *h = fio_http_new();
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
             "valid WebSocket request headers should be detected");

  fio_http_upgrade_websocket(h);
  FIO_ASSERT(fio_http_status(h) == 101,
             "WebSocket upgrade should set status 101");
  FIO_ASSERT(fio_http_is_websocket(h),
             "handle should report WebSocket after upgrade");
  FIO_ASSERT(fio_http_is_finished(h),
             "WebSocket upgrade should finish the response");

  fio_str_info_s accept = fio_http_response_header(
      h,
      FIO_STR_INFO2((char *)"sec-websocket-accept", 20),
      0);
  FIO_ASSERT(accept.len == 28,
             "WebSocket accept header should be present (len=%zu)",
             accept.len);

  /* Without an attached connection, high-level write must fail. */
  FIO_ASSERT(fio_http_websocket_write(h, "x", 1, 1) == -1,
             "websocket_write should fail without a connection");
  FIO_ASSERT(fio_http_on_message_set(h, NULL) == -1,
             "on_message_set should fail without a connection");

  fio_http_free(h);
}

static void test_sse_upgrade_helpers(void) {
  fprintf(stderr, "  * SSE upgrade helpers\n");

  fio_http_s *h = fio_http_new();
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"accept", 6),
                              FIO_STR_INFO1((char *)"text/event-stream"));

  FIO_ASSERT(fio_http_sse_requested(h),
             "SSE request headers should be detected");

  fio_http_status_set(h, 200);
  fio_http_upgrade_sse(h);
  FIO_ASSERT(fio_http_is_sse(h), "handle should report SSE after upgrade");
  FIO_ASSERT(fio_http_status(h) == 200, "SSE upgrade should keep status 200");
  FIO_ASSERT(fio_http_is_finished(h), "SSE upgrade should finish the response");
  fio_str_info_s ct =
      fio_http_response_header(h, FIO_STR_INFO2((char *)"content-type", 12), 0);
  FIO_ASSERT(ct.len == 17 && !FIO_MEMCMP(ct.buf, "text/event-stream", 17),
             "SSE upgrade should set text/event-stream content-type");

  FIO_ASSERT(fio_http_sse_write(h, .data = FIO_BUF_INFO2((char *)"hi", 2)) ==
                 -1,
             "sse_write should fail without a connection");

  fio_http_free(h);
}

/* ===========================================================================
 * Regression test: V5 — fio_http_sse_write OOB read on newline-first data
 * (CWE-125 / CWE-787)
 *
 * When splitting SSE data into lines, the old code did
 *   pos -= (pos[-1] == '\r');
 * If args.data begins with '\n', pos equals args.data.buf and pos[-1] reads one
 * byte before the buffer. A full runtime PoC needs a live upgraded SSE
 * connection; this test at least exercises the no-connection path with the
 * triggering input pattern and verifies it returns -1 without crashing. The
 * source-level fix guards the look-behind with `pos > args.data.buf`.
 * ===========================================================================
 */
static void test_sse_newline_first_edge_case(void) {
  fprintf(stderr, "  * SSE newline-first data edge case\n");

  fio_http_s *h = fio_http_new();
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"accept", 6),
                              FIO_STR_INFO1((char *)"text/event-stream"));
  fio_http_status_set(h, 200);
  fio_http_upgrade_sse(h);

  /* Data starting with '\n' is the trigger for the V5 look-behind bug. */
  FIO_ASSERT(
      fio_http_sse_write(h, .data = FIO_BUF_INFO2((char *)"\nhi", 3)) == -1,
      "sse_write with newline-first data should fail without a connection");

  fio_http_free(h);
}

/* ===========================================================================
   T008 — Static file: Vary on plain responses + Range guard
   ===========================================================================
 */

/** Builds a temp directory with a compressible text file (and optionally its
 * pre-compressed .gz variant). Returns the directory path length, or 0. */
static size_t test_static_make_tree(char *dir,
                                    size_t dir_cap,
                                    const void *content,
                                    size_t content_len,
                                    int with_gz_variant) {
  const char *options[] = {"TMPDIR", "TMP", "TEMP", NULL};
  const char *tmpdir = NULL;
  for (size_t i = 0; !tmpdir && options[i]; ++i)
    tmpdir = fio_sys_env(options[i]);
  size_t tmplen = tmpdir ? FIO_STRLEN(tmpdir) : 0;
  if (!tmpdir || tmplen > 128) {
#if FIO_OS_WIN
    tmpdir = ".";
    tmplen = 1;
#else
    tmpdir = "/tmp/";
    tmplen = FIO_STRLEN(tmpdir);
#endif
  }
  if (tmplen + 48 >= dir_cap)
    return 0;
  FIO_MEMCPY(dir, tmpdir, tmplen);
  size_t len = tmplen;
  if (len && dir[len - 1] != '/' && dir[len - 1] != '\\' &&
      dir[len - 1] != FIO_FOLDER_SEPARATOR)
    dir[len++] = FIO_FOLDER_SEPARATOR;
  FIO_MEMCPY(dir + len, "http_vary_test_", 15);
  len += 15;
  len += fio_ltoa(dir + len, (int64_t)fio_rand64(), 16);
  dir[len] = '\0';
  if (fio_filename_make_path(.path = dir))
    return 0;

  char path[512];
  snprintf(path, sizeof(path), "%s%ctest.txt", dir, FIO_FOLDER_SEPARATOR);
  FILE *f = fopen(path, "wb");
  FIO_ASSERT(f, "failed to create static vary test file");
  FIO_ASSERT(fwrite(content, 1, content_len, f) == content_len,
             "failed to write static vary test file");
  fclose(f);

  if (with_gz_variant) {
    char gzpath[512];
    snprintf(gzpath,
             sizeof(gzpath),
             "%s%ctest.txt.gz",
             dir,
             FIO_FOLDER_SEPARATOR);
    size_t bound = fio_deflate_compress_bound(content_len) + 18;
    uint8_t *gz = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, bound, 0);
    FIO_ASSERT(gz, "failed to allocate gzip variant buffer");
    size_t gz_len = fio_gzip_compress(gz, bound, content, content_len, 6);
    FIO_ASSERT(gz_len > 18, "failed to gzip static vary test content");
    f = fopen(gzpath, "wb");
    FIO_ASSERT(f, "failed to create gzip variant file");
    FIO_ASSERT(fwrite(gz, 1, gz_len, f) == gz_len,
               "failed to write gzip variant file");
    fclose(f);
    FIO_MEM_FREE(gz, bound);
  }
  return len;
}

static void test_static_tree_cleanup(const char *dir) {
  fio_filename_remove(.path = dir, .recursive = 1);
}

static void test_static_vary_and_range_guards(void) {
  fprintf(stderr, "  * static vary / range compression guards\n");

  enum { CONTENT_LEN = 4096 };
  char content[CONTENT_LEN];
  for (size_t i = 0; i < CONTENT_LEN; ++i)
    content[i] = (char)('a' + (i & 15));

  char dir[512];
  size_t dir_len = test_static_make_tree(dir,
                                         sizeof(dir),
                                         content,
                                         CONTENT_LEN,
                                         1 /* with .gz variant */);
  FIO_ASSERT(dir_len > 0, "failed to create static vary test tree");

  /* 1. Client does not accept gzip → plain file, but variants exist on
   *    disk, so the response MUST carry `Vary: accept-encoding`. */
  {
    fio_http_s *h = test_http_make_handle("GET", "/test.txt");
    fio_http_cflags_set(h, FIO_HTTP_CFLAG_COMPRESS_STATIC);
    fio_http_request_header_set(
        h,
        FIO_STR_INFO2((char *)"accept-encoding", 15),
        FIO_STR_INFO1((char *)"identity"));
    int r = fio_http_static_file_response(h,
                                          FIO_STR_INFO2(dir, dir_len),
                                          FIO_STR_INFO1((char *)"/test.txt"),
                                          0);
    FIO_ASSERT(r == 0, "static plain: response should succeed");
    fio_str_info_s ce = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"content-encoding", 16),
        0);
    FIO_ASSERT(!ce.buf,
               "static plain: content-encoding must be absent (got '%.*s')",
               (int)ce.len,
               ce.buf ? ce.buf : "");
    fio_str_info_s vary = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"vary", 4),
        0);
    FIO_ASSERT(vary.len >= 15 &&
                   fio___http_header_has_token(vary,
                                               "accept-encoding",
                                               15),
               "static plain: Vary: accept-encoding required while "
               "compressed variants exist (caches must key on "
               "Accept-Encoding)");
    fio_http_free(h);
  }

  /* 2. Client accepts gzip → pre-compressed variant, with Vary. */
  {
    fio_http_s *h = test_http_make_handle("GET", "/test.txt");
    fio_http_cflags_set(h, FIO_HTTP_CFLAG_COMPRESS_STATIC);
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                FIO_STR_INFO1((char *)"gzip"));
    int r = fio_http_static_file_response(h,
                                          FIO_STR_INFO2(dir, dir_len),
                                          FIO_STR_INFO1((char *)"/test.txt"),
                                          0);
    FIO_ASSERT(r == 0, "static gzip: response should succeed");
    fio_str_info_s ce = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"content-encoding", 16),
        0);
    FIO_ASSERT(ce.len == 4 && !memcmp(ce.buf, "gzip", 4),
               "static gzip: expected content-encoding gzip");
    fio_str_info_s vary = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"vary", 4),
        0);
    FIO_ASSERT(vary.len >= 15 &&
                   fio___http_header_has_token(vary,
                                               "accept-encoding",
                                               15),
               "static gzip: Vary: accept-encoding expected");
    fio_http_free(h);
  }

  /* 3. Range request + gzip acceptance → identity (ranges over encoded
   *    bytes are broken in practice). */
  {
    fio_http_s *h = test_http_make_handle("GET", "/test.txt");
    fio_http_cflags_set(h, FIO_HTTP_CFLAG_COMPRESS_STATIC);
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                FIO_STR_INFO1((char *)"gzip"));
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"range", 5),
                                FIO_STR_INFO1((char *)"bytes=0-99"));
    int r = fio_http_static_file_response(h,
                                          FIO_STR_INFO2(dir, dir_len),
                                          FIO_STR_INFO1((char *)"/test.txt"),
                                          0);
    FIO_ASSERT(r == 0, "static range: response should succeed");
    fio_str_info_s ce = fio_http_response_header(
        h,
        FIO_STR_INFO2((char *)"content-encoding", 16),
        0);
    FIO_ASSERT(!ce.buf,
               "static range: content-encoding must be absent for ranged "
               "responses (got '%.*s')",
               (int)ce.len,
               ce.buf ? ce.buf : "");
    FIO_ASSERT(fio_http_status(h) == 206,
               "static range: expected status 206, got %u",
               (unsigned)fio_http_status(h));
    fio_http_free(h);
  }

  test_static_tree_cleanup(dir);
}

/* ===========================================================================
   T006 — WebSocket permessage-deflate negotiation policy

   The negotiation must ALWAYS force server_no_context_takeover +
   client_no_context_takeover (RFC 7692 allows either endpoint to request
   them unilaterally), honor server_max_window_bits when offered, and never
   emit window-bits parameters. The policy lives in a pure helper
   (fio___http_ws_deflate_negotiate) so it is testable without a live
   connection; pre-fix the seam does not exist and this test fails.
   ===========================================================================
 */

/** Returns non-zero if the `;`-separated extension response contains
 * `name` as a parameter name (RFC 7692 Sec-WebSocket-Extensions grammar). */
static int test_ws_ext_has_param(fio_str_info_s resp,
                                 const char *name,
                                 size_t name_len) {
  const char *pos = resp.buf;
  const char *end = resp.buf + resp.len;
  while (pos < end) {
    while (pos < end && (*pos == ' ' || *pos == '\t' || *pos == ';'))
      ++pos;
    const char *tok = pos;
    while (pos < end && *pos != ';' && *pos != '=')
      ++pos;
    const char *tok_end = pos;
    while (tok_end > tok && (tok_end[-1] == ' ' || tok_end[-1] == '\t'))
      --tok_end;
    if ((size_t)(tok_end - tok) == name_len &&
        !FIO_MEMCMP(tok, name, name_len))
      return 1;
    while (pos < end && *pos != ';')
      ++pos;
  }
  return 0;
}

static void test_websocket_deflate_negotiation(void) {
  fprintf(stderr, "  * WebSocket permessage-deflate negotiation policy\n");
#ifdef FIO___HTTP_WS_DEFLATE_NEGOTIATE_SEAM
  char out[128];
  int bits = 0;

  /* 1. Bare offer → both no-context-takeover flags ALWAYS forced. */
  {
    bits = 0;
    size_t len = fio___http_ws_deflate_negotiate(
        FIO_STR_INFO2((char *)"permessage-deflate", 18),
        out,
        sizeof(out),
        &bits);
    FIO_ASSERT(len > 0, "negotiate: bare offer should succeed");
    fio_str_info_s resp = FIO_STR_INFO2(out, len);
    FIO_ASSERT(test_ws_ext_has_param(resp, "permessage-deflate", 18),
               "negotiate bare: response must carry permessage-deflate");
    FIO_ASSERT(test_ws_ext_has_param(resp, "server_no_context_takeover", 26),
               "negotiate bare: server_no_context_takeover must ALWAYS be "
               "forced");
    FIO_ASSERT(test_ws_ext_has_param(resp, "client_no_context_takeover", 26),
               "negotiate bare: client_no_context_takeover must ALWAYS be "
               "forced");
    FIO_ASSERT(bits == 15,
               "negotiate bare: default server window bits should be 15, "
               "got %d",
               bits);
  }

  /* 2. server_max_window_bits is honored (recorded for distance clamping)
   *    but never echoed as a parameter. */
  {
    bits = 0;
    size_t len = fio___http_ws_deflate_negotiate(
        FIO_STR_INFO2((char *)"permessage-deflate; server_max_window_bits=12",
                      45),
        out,
        sizeof(out),
        &bits);
    FIO_ASSERT(len > 0, "negotiate: server_max_window_bits offer");
    FIO_ASSERT(bits == 12,
               "negotiate: server_max_window_bits=12 must be honored, got "
               "%d",
               bits);
    fio_str_info_s resp = FIO_STR_INFO2(out, len);
    FIO_ASSERT(!test_ws_ext_has_param(resp, "server_max_window_bits", 22),
               "negotiate: response must never emit window-bits params");
    FIO_ASSERT(test_ws_ext_has_param(resp, "server_no_context_takeover", 26),
               "negotiate bits: server_no_context_takeover forced");
    FIO_ASSERT(test_ws_ext_has_param(resp, "client_no_context_takeover", 26),
               "negotiate bits: client_no_context_takeover forced");
  }

  /* 3. Browser-style offer (client_max_window_bits without value) → clean
   *    negotiation, no window-bits params echoed. */
  {
    bits = 0;
    size_t len = fio___http_ws_deflate_negotiate(
        FIO_STR_INFO2((char *)"permessage-deflate; client_max_window_bits",
                      41),
        out,
        sizeof(out),
        &bits);
    FIO_ASSERT(len > 0, "negotiate: browser-style offer should succeed");
    fio_str_info_s resp = FIO_STR_INFO2(out, len);
    FIO_ASSERT(!test_ws_ext_has_param(resp, "client_max_window_bits", 22),
               "negotiate browser: must not echo client_max_window_bits");
    FIO_ASSERT(test_ws_ext_has_param(resp, "server_no_context_takeover", 26) &&
                   test_ws_ext_has_param(resp,
                                         "client_no_context_takeover",
                                         26),
               "negotiate browser: both no-context-takeover flags forced");
  }

  /* 4. Full offer shape: no-context flags echoed by the client are still
   *    forced; smallest offered server window wins within valid range. */
  {
    static const char full_offer[] =
        "permessage-deflate; server_no_context_takeover; "
        "client_no_context_takeover; server_max_window_bits=8";
    bits = 0;
    size_t len = fio___http_ws_deflate_negotiate(
        FIO_STR_INFO2((char *)full_offer, sizeof(full_offer) - 1),
        out,
        sizeof(out),
        &bits);
    FIO_ASSERT(len > 0, "negotiate: full offer should succeed");
    FIO_ASSERT(bits == 8,
               "negotiate: server_max_window_bits=8 honored, got %d",
               bits);
  }

  /* 5. Output buffer too small → graceful 0, no partial write past cap. */
  {
    bits = 0;
    char tiny[8];
    size_t len = fio___http_ws_deflate_negotiate(
        FIO_STR_INFO2((char *)"permessage-deflate", 18),
        tiny,
        sizeof(tiny),
        &bits);
    FIO_ASSERT(len == 0,
               "negotiate: undersized output buffer must return 0");
  }
#else  /* FIO___HTTP_WS_DEFLATE_NEGOTIATE_SEAM */
  fprintf(stderr,
          "FAIL: permessage-deflate negotiation seam/policy not implemented "
          "(FIO___HTTP_WS_DEFLATE_NEGOTIATE_SEAM; see T022)\n");
  exit(1);
#endif /* FIO___HTTP_WS_DEFLATE_NEGOTIATE_SEAM */
}

/* ===========================================================================
   T021 — WebSocket client connect wrapper (fio_http_websocket_connect)

   Correctness-only, no reactor: `fio_io_connect` without a running reactor
   opens the client socket and defers protocol attachment, so the outgoing
   handle is fully prepared (upgrade request headers, path, host) before any
   IO event fires. A loopback listener provides a connectable address; its
   bound port is discovered with `getsockname` on the listener's internal fd
   (the listener URL keeps port 0). The client completion seam
   (`fio___http_on_http_client`, 434 http accept.h) is driven directly —
   same precedent as FIO___HTTP_WS_DEFLATE_NEGOTIATE_SEAM — because running
   a canned 101 response through the HTTP/1.1 parser requires the reactor's
   read path. Seam limitation: parser-level response handling
   (`fio___http1_process_data` / `fio_http1_on_complete`) is not covered
   here; only the post-parse dispatch logic is.
   ===========================================================================
 */

static int test_ws_rec_on_http_calls = 0;
static fio_http_s *test_ws_rec_on_http_h = NULL;
static void test_ws_rec_on_http(fio_http_s *h) {
  ++test_ws_rec_on_http_calls;
  test_ws_rec_on_http_h = h;
}
static int test_ws_rec_on_open_calls = 0;
static fio_http_s *test_ws_rec_on_open_h = NULL;
static void test_ws_rec_on_open(fio_http_s *h) {
  ++test_ws_rec_on_open_calls;
  test_ws_rec_on_open_h = h;
}

/** Returns the bound port of a loopback listener created with port 0. */
static unsigned test_ws_listener_port(fio_http_listener_s *l) {
  fio___io_listen_s *li = (fio___io_listen_s *)l;
  struct sockaddr_storage ss;
  socklen_t slen = (socklen_t)sizeof(ss);
  if (getsockname(li->fd, (struct sockaddr *)&ss, &slen))
    return 0;
  if (ss.ss_family == AF_INET)
    return (unsigned)ntohs(((struct sockaddr_in *)&ss)->sin_port);
  if (ss.ss_family == AF_INET6)
    return (unsigned)ntohs(((struct sockaddr_in6 *)&ss)->sin6_port);
  return 0;
}

/** Sets response data accepting the WebSocket upgrade (server-side math). */
static void test_ws_set_accept_response(fio_http_s *h) {
  fio_http_status_set(h, 101);
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"connection", 10),
                               FIO_STR_INFO2((char *)"Upgrade", 7));
  fio_http_response_header_set(h,
                               FIO_STR_INFO2((char *)"upgrade", 7),
                               FIO_STR_INFO2((char *)"websocket", 9));
  fio_str_info_s k =
      fio_http_request_header(h,
                              FIO_STR_INFO2((char *)"sec-websocket-key", 17),
                              0);
  FIO_ASSERT(k.len == 24, "accept response: request key missing");
  FIO_STR_INFO_TMP_VAR(accept_val, 63);
  fio_string_write(&accept_val, NULL, k.buf, k.len);
  fio_string_write(&accept_val,
                   NULL,
                   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
                   36);
  fio_sha1_s sha = fio_sha1(accept_val.buf, accept_val.len);
  fio_sha1_digest(&sha);
  accept_val.len = 0;
  fio_string_write_base64enc(&accept_val,
                             NULL,
                             fio_sha1_digest(&sha),
                             fio_sha1_len(),
                             0);
  fio_http_response_header_set(
      h,
      FIO_STR_INFO2((char *)"sec-websocket-accept", 20),
      accept_val);
}

static void test_websocket_connect_wrapper(void) {
  fprintf(stderr,
          "  * WebSocket connect wrapper (scheme normalization, client "
          "seams)\n");

  /* Loopback listener, so `fio_io_connect` can open a client socket.
     No reactor is started; the listener never accepts the connection. */
  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0", .on_http = test_http_noop_on_http);
  FIO_ASSERT(l, "wrapper test: fio_http_listen failed");
  unsigned port = test_ws_listener_port(l);
  FIO_ASSERT(port, "wrapper test: listener port discovery failed");

  /* (a) Scheme normalization: http://, https://, ws:// and scheme-less URLs
   *     must all produce a handle carrying the WebSocket upgrade request. */
  {
    static const struct {
      const char *fmt;
      const char *path;
      const char *query;
      const char *host;
    } cases[] = {
        {"http://127.0.0.1:%u/ws-path?x=1", "/ws-path", "x=1", "127.0.0.1"},
        {"https://127.0.0.1:%u/p", "/p", "", "127.0.0.1"},
        {"ws://127.0.0.1:%u/p", "/p", "", "127.0.0.1"},
        {"localhost:%u/p", "/p", "", "localhost"},
    };
    for (size_t i = 0; i < 4; ++i) {
      char url[256];
      snprintf(url, sizeof(url), cases[i].fmt, port);
      test_ws_rec_on_http_calls = test_ws_rec_on_open_calls = 0;
      fio_http_s *h = fio_http_new();
      fio_io_s *io = fio_http_websocket_connect(url,
                                                h,
                                                .on_http = test_ws_rec_on_http,
                                                .on_open = test_ws_rec_on_open);
      FIO_ASSERT(io, "websocket_connect(%s): failed to create an IO", url);
      FIO_ASSERT(fio_http_websocket_requested(h) == 1,
                 "websocket_connect(%s): handle must carry the WS upgrade "
                 "request (upgrade + sec-websocket-key + version)",
                 url);
      fio_str_info_s m = fio_http_method(h);
      FIO_ASSERT(m.len == 3 && !FIO_MEMCMP(m.buf, "GET", 3),
                 "websocket_connect(%s): method should default to GET",
                 url);
      fio_str_info_s path = fio_http_path(h);
      FIO_ASSERT(path.len == strlen(cases[i].path) &&
                     !FIO_MEMCMP(path.buf, cases[i].path, path.len),
                 "websocket_connect(%s): path should be %s",
                 url,
                 cases[i].path);
      fio_str_info_s query = fio_http_query(h);
      FIO_ASSERT(query.len == strlen(cases[i].query) &&
                     !FIO_MEMCMP(query.buf, cases[i].query, query.len),
                 "websocket_connect(%s): query mismatch",
                 url);
      fio_str_info_s host =
          fio_http_request_header(h, FIO_STR_INFO2((char *)"host", 4), 0);
      FIO_ASSERT(host.len == strlen(cases[i].host) &&
                     !FIO_MEMCMP(host.buf, cases[i].host, host.len),
                 "websocket_connect(%s): host header should be %s",
                 url,
                 cases[i].host);
      FIO_ASSERT(!test_ws_rec_on_http_calls && !test_ws_rec_on_open_calls,
                 "websocket_connect(%s): no callback may fire before the "
                 "response is processed",
                 url);
      /* teardown: close the IO and drain the deferred tasks (reactor-less
         equivalents of the reactor's close path). */
      fio_io_close(io);
      fio_queue_perform_all(fio_io_queue());
    }
  }

  /* (b) Acceptance: a 101 response switches the handle to the WebSocket
   *     protocol / controller and fires `on_open` (not `on_http`). */
  {
    char url[256];
    snprintf(url, sizeof(url), "ws://127.0.0.1:%u/ws", port);
    test_ws_rec_on_http_calls = test_ws_rec_on_open_calls = 0;
    test_ws_rec_on_open_h = NULL;
    fio_http_s *h = fio_http_new();
    fio_io_s *io = fio_http_websocket_connect(url,
                                              h,
                                              .on_http = test_ws_rec_on_http,
                                              .on_open = test_ws_rec_on_open);
    FIO_ASSERT(io, "acceptance: connect failed");
    FIO_ASSERT(fio_http_websocket_requested(h) == 1,
               "acceptance: upgrade request missing");
    test_ws_set_accept_response(h);
    FIO_ASSERT(fio_http_websocket_accepted(h) == 1,
               "acceptance: crafted 101 response should be accepted");
    /* mimic the reactor-time wiring (fio___connecting_on_ready +
       fio___http1_on_attach_client): link the IO and the connection */
    fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
    c->io = io;
    fio_io_udata_set(io, c);
    /* drive the client completion seam directly */
    fio___http_on_http_client(h, NULL);
    fio___http_protocol_s *p =
        FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings);
    FIO_ASSERT(fio_http_controller(h) ==
                   &p->state[FIO___HTTP_PROTOCOL_WS].controller,
               "acceptance: handle controller should switch to WebSocket");
    FIO_ASSERT(!test_ws_rec_on_open_calls,
               "acceptance: on_open must wait for the protocol switch");
    /* the deferred protocol switch attaches the WS protocol (on_open) and
       tears everything down; the WS controller frees the connection in a
       single pass, so no balancing reference is required here */
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(test_ws_rec_on_open_calls == 1 && test_ws_rec_on_open_h == h,
               "acceptance: on_open should fire once the WS protocol "
               "attaches (got %d)",
               test_ws_rec_on_open_calls);
    FIO_ASSERT(!test_ws_rec_on_http_calls,
               "acceptance: on_http must not fire for a 101 response");
  }

  /* (c) Rejection: a non-101 response is routed to `settings.on_http`
   *     with the response handle (`on_open` must not fire). */
  {
    char url[256];
    snprintf(url, sizeof(url), "ws://127.0.0.1:%u/ws", port);
    test_ws_rec_on_http_calls = test_ws_rec_on_open_calls = 0;
    test_ws_rec_on_http_h = NULL;
    fio_http_s *h = fio_http_new();
    fio_io_s *io = fio_http_websocket_connect(url,
                                              h,
                                              .on_http = test_ws_rec_on_http,
                                              .on_open = test_ws_rec_on_open);
    FIO_ASSERT(io, "rejection: connect failed");
    fio_http_status_set(h, 200);
    FIO_ASSERT(!fio_http_websocket_accepted(h),
               "rejection: a 200 response must not be accepted as an "
               "upgrade");
    /* drive the client completion seam directly */
    fio___http_on_http_client(h, NULL);
    /* the queued user callback frees the handle when drained */
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(test_ws_rec_on_http_calls == 1 && test_ws_rec_on_http_h == h,
               "rejection: on_http should fire once with the response "
               "handle (got %d)",
               test_ws_rec_on_http_calls);
    FIO_ASSERT(!test_ws_rec_on_open_calls,
               "rejection: on_open must not fire for a non-101 response");
    /* the IO outlived the handle; close it and drain the close path */
    fio_io_close(io);
    fio_queue_perform_all(fio_io_queue());
  }

  fio_io_listen_stop((fio_io_listener_s *)l);
}

/* ===========================================================================
   T032 — compress_static failure-memoization tests

   Covers the note-result seam (`fio___http_static_compress_note_result`)
   directly, the attached-handle gate against a read-only public folder
   (EACCES → immediate permanent disable, no retry), and the unchanged
   detached-handle path (CFLAG-gated on-demand `.br` creation).
   ===========================================================================
 */

static void test_static_compress_note_result(void) {
  fprintf(stderr, "  * static compress failure-memoization seam\n");

  fio_http_settings_s s;

  /* (a) From 1, eight consecutive failures shift the runway out of the
   *     uint8_t: 1→2→4→…→128→0; once 0, further failures keep it 0. */
  {
    static const uint8_t walk[8] = {2, 4, 8, 16, 32, 64, 128, 0};
    FIO_MEMSET(&s, 0, sizeof(s));
    s.compress_static = 1;
    for (size_t i = 0; i < 8; ++i) {
      fio___http_static_compress_note_result(&s, EINVAL);
      FIO_ASSERT(s.compress_static == walk[i],
                 "runway step %zu: expected %u, got %u",
                 i,
                 (unsigned)walk[i],
                 (unsigned)s.compress_static);
    }
    fio___http_static_compress_note_result(&s, EINVAL);
    FIO_ASSERT(!s.compress_static,
               "disabled (0) must stay disabled on further failures");
    fio___http_static_compress_note_result(&s, EINVAL);
    FIO_ASSERT(!s.compress_static,
               "disabled (0) must stay disabled on further failures");
  }

  /* (b) Success re-seeds bit 0, restoring the 8-failure runway:
   *     1 → fail → 2 → success → 3 → fail → 6 → success → 7. */
  {
    FIO_MEMSET(&s, 0, sizeof(s));
    s.compress_static = 1;
    fio___http_static_compress_note_result(&s, EINVAL);
    FIO_ASSERT(s.compress_static == 2, "re-seed: expected 2, got %u",
               (unsigned)s.compress_static);
    fio___http_static_compress_note_result(&s, 0);
    FIO_ASSERT(s.compress_static == 3 && (s.compress_static & 1),
               "re-seed: success must set bit 0 (expected 3, got %u)",
               (unsigned)s.compress_static);
    fio___http_static_compress_note_result(&s, EINVAL);
    FIO_ASSERT(s.compress_static == 6, "re-seed: expected 6, got %u",
               (unsigned)s.compress_static);
    fio___http_static_compress_note_result(&s, 0);
    FIO_ASSERT(s.compress_static == 7 && (s.compress_static & 1),
               "re-seed: success must set bit 0 (expected 7, got %u)",
               (unsigned)s.compress_static);
  }

  /* (c) Fatal errnos (filesystem cannot accept new files) disable
   *     immediately from any non-zero value. */
  {
    FIO_MEMSET(&s, 0, sizeof(s));
    s.compress_static = 3;
    fio___http_static_compress_note_result(&s, ENOSPC);
    FIO_ASSERT(!s.compress_static, "ENOSPC must disable immediately");
    s.compress_static = 3;
    fio___http_static_compress_note_result(&s, EACCES);
    FIO_ASSERT(!s.compress_static, "EACCES must disable immediately");
    s.compress_static = 3;
    fio___http_static_compress_note_result(&s, EROFS);
    FIO_ASSERT(!s.compress_static, "EROFS must disable immediately");
#ifdef EDQUOT
    s.compress_static = 3;
    fio___http_static_compress_note_result(&s, EDQUOT);
    FIO_ASSERT(!s.compress_static, "EDQUOT must disable immediately");
#endif
  }

  /* (d) NULL settings is a no-op (detached handles carry no state). */
  {
    fio___http_static_compress_note_result(NULL, 0);
    fio___http_static_compress_note_result(NULL, EINVAL);
    fio___http_static_compress_note_result(NULL, EACCES);
  }

  /* (e) No mapping: the shift keeps the 8-bit width (200<<1 mod 256 = 144);
   *     success only ORs bit 0 (200 → 201). */
  {
    FIO_MEMSET(&s, 0, sizeof(s));
    s.compress_static = 200;
    fio___http_static_compress_note_result(&s, EINVAL);
    FIO_ASSERT(s.compress_static == 144,
               "8-bit shift: expected 144, got %u",
               (unsigned)s.compress_static);
    s.compress_static = 200;
    fio___http_static_compress_note_result(&s, 0);
    FIO_ASSERT(s.compress_static == 201,
               "success re-seed: expected 201, got %u",
               (unsigned)s.compress_static);
  }
}

static void test_static_compress_attached_readonly(void) {
  fprintf(stderr,
          "  * static compress memoization (attached handle, read-only "
          "folder)\n");
#if FIO_OS_WIN
  fprintf(stderr,
          "    (skipped on Windows — POSIX folder permission semantics "
          "required)\n");
#else
  enum { CONTENT_LEN = 4096 };
  char content[CONTENT_LEN];
  for (size_t i = 0; i < CONTENT_LEN; ++i)
    content[i] = (char)('a' + (i & 15));

  char dir[512];
  size_t dir_len =
      test_static_make_tree(dir, sizeof(dir), content, CONTENT_LEN, 0);
  FIO_ASSERT(dir_len > 0, "read-only test: failed to create test tree");

  /* Attached handle via the client-connection precedent (see
     test_websocket_connect_wrapper): `fio_http_settings(h)` resolves the
     route settings, so the settings gate (not the cflag gate) applies. */
  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0", .on_http = test_http_noop_on_http);
  FIO_ASSERT(l, "read-only test: fio_http_listen failed");
  unsigned port = test_ws_listener_port(l);
  FIO_ASSERT(port, "read-only test: listener port discovery failed");
  char url[256];
  snprintf(url, sizeof(url), "ws://127.0.0.1:%u/file", port);
  fio_http_s *h = fio_http_new();
  fio_io_s *io =
      fio_http_websocket_connect(url, h, .on_http = test_http_noop_on_http);
  FIO_ASSERT(io, "read-only test: failed to create an attached handle");
  /* the client wrapper sets `path` only; the settings resolver routes on
     `opath` (set by the server parser on a live request) */
  fio_http_opath_set(h, fio_http_path(h));
  fio_http_settings_s *st = fio_http_settings(h);
  FIO_ASSERT(st,
             "read-only test: attached handle must resolve route settings");
  st->compress_static = 1;
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"accept-encoding", 15),
                              FIO_STR_INFO1((char *)"br, gzip"));

  /* Make the folder read-only; skip gracefully if permissions are not
     enforced (root, or a filesystem that ignores mode bits). */
  int skipped = 0;
  if (chmod(dir, 0555)) {
    skipped = 1;
  } else {
    char probe[512];
    snprintf(probe, sizeof(probe), "%s%cprobe.tmp", dir, FIO_FOLDER_SEPARATOR);
    FILE *pf = fopen(probe, "wb");
    if (pf) {
      fclose(pf);
      fio_filename_remove(.path = probe);
      skipped = 1; /* create succeeded — permissions not enforced */
    }
  }
  if (skipped) {
    fprintf(stderr,
            "    (skipped — folder permissions not enforced, e.g. running "
            "as root)\n");
    chmod(dir, 0755);
    fio_io_close(io);
    fio_queue_perform_all(fio_io_queue());
    fio_io_listen_stop((fio_io_listener_s *)l);
    test_static_tree_cleanup(dir);
    return;
  }

  /* Request 1: on-demand creation is attempted, the write fails with
   * EACCES, the original file is served identity, and the settings value
   * self-disables to 0 (fatal errno → immediate permanent disable). */
  fio_http_status_set(h, 200); /* the status is caller/protocol supplied */
  int r = fio_http_static_file_response(h,
                                        FIO_STR_INFO2(dir, dir_len),
                                        FIO_STR_INFO1((char *)"/test.txt"),
                                        0);
  FIO_ASSERT(r == 0, "read-only req1: response should succeed");
  FIO_ASSERT(fio_http_status(h) == 200,
             "read-only req1: expected status 200, got %u",
             (unsigned)fio_http_status(h));
  fio_str_info_s ce = fio_http_response_header(
      h,
      FIO_STR_INFO2((char *)"content-encoding", 16),
      0);
  FIO_ASSERT(!ce.buf,
             "read-only req1: content-encoding must be absent (identity "
             "response; got '%.*s')",
             (int)ce.len,
             ce.buf ? ce.buf : "");
  uint8_t cv;
  fio_atomic_load(cv, &st->compress_static);
  FIO_ASSERT(!cv,
             "read-only req1: EACCES must disable compress_static "
             "immediately (got %u)",
             (unsigned)cv);

  /* Request 2: the memoized failure must prevent any retry — the value
   * stays 0 and no `.br` / `.gz` variant appears on disk. */
  fio_http_clear_response(h, 1);
  fio_http_status_set(h, 200);
  r = fio_http_static_file_response(h,
                                    FIO_STR_INFO2(dir, dir_len),
                                    FIO_STR_INFO1((char *)"/test.txt"),
                                    0);
  FIO_ASSERT(r == 0, "read-only req2: response should succeed");
  FIO_ASSERT(fio_http_status(h) == 200,
             "read-only req2: expected status 200, got %u",
             (unsigned)fio_http_status(h));
  ce = fio_http_response_header(h,
                                FIO_STR_INFO2((char *)"content-encoding", 16),
                                0);
  FIO_ASSERT(!ce.buf,
             "read-only req2: content-encoding must be absent (got '%.*s')",
             (int)ce.len,
             ce.buf ? ce.buf : "");
  fio_atomic_load(cv, &st->compress_static);
  FIO_ASSERT(!cv,
             "read-only req2: compress_static must remain disabled (got %u)",
             (unsigned)cv);
  {
    struct stat vst;
    char vpath[512];
    snprintf(vpath,
             sizeof(vpath),
             "%s%ctest.txt.br",
             dir,
             FIO_FOLDER_SEPARATOR);
    FIO_ASSERT(fio_filename_stat(vpath, &vst),
               "read-only req2: no .br variant may be created after "
               "memoized failure");
    snprintf(vpath,
             sizeof(vpath),
             "%s%ctest.txt.gz",
             dir,
             FIO_FOLDER_SEPARATOR);
    FIO_ASSERT(fio_filename_stat(vpath, &vst),
               "read-only req2: no .gz variant may be created after "
               "memoized failure");
  }

  /* teardown: close the client connection, then restore perms and clean
     up. */
  fio_io_close(io);
  fio_queue_perform_all(fio_io_queue());
  fio_io_listen_stop((fio_io_listener_s *)l);
  chmod(dir, 0755);
  test_static_tree_cleanup(dir);
#endif /* FIO_OS_WIN */
}

static void test_static_compress_detached_creation(void) {
  fprintf(stderr,
          "  * static compress on-demand creation (detached handle, "
          "writable folder)\n");

  enum { CONTENT_LEN = 4096 };
  char content[CONTENT_LEN];
  for (size_t i = 0; i < CONTENT_LEN; ++i)
    content[i] = (char)('a' + (i & 15));

  char dir[512];
  size_t dir_len =
      test_static_make_tree(dir, sizeof(dir), content, CONTENT_LEN, 0);
  FIO_ASSERT(dir_len > 0, "detached creation: failed to create test tree");

  /* Detached handle (no route settings): the legacy CFLAG gate still
   * enables on-demand creation — a missing `.br` variant is created,
   * written into the folder, and served with content-encoding: br. */
  fio_http_s *h = test_http_make_handle("GET", "/test.txt");
  FIO_ASSERT(!fio_http_settings(h),
             "detached creation: handle must be detached (no settings)");
  fio_http_cflags_set(h, FIO_HTTP_CFLAG_COMPRESS_STATIC);
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"accept-encoding", 15),
                              FIO_STR_INFO1((char *)"br"));
  fio_http_status_set(h, 200); /* the status is caller/protocol supplied */
  int r = fio_http_static_file_response(h,
                                        FIO_STR_INFO2(dir, dir_len),
                                        FIO_STR_INFO1((char *)"/test.txt"),
                                        0);
  FIO_ASSERT(r == 0, "detached creation: response should succeed");
  FIO_ASSERT(fio_http_status(h) == 200,
             "detached creation: expected status 200, got %u",
             (unsigned)fio_http_status(h));
  fio_str_info_s ce = fio_http_response_header(
      h,
      FIO_STR_INFO2((char *)"content-encoding", 16),
      0);
  FIO_ASSERT(ce.len == 2 && !memcmp(ce.buf, "br", 2),
             "detached creation: expected content-encoding br, got '%.*s'",
             (int)ce.len,
             ce.buf ? ce.buf : "");
  {
    struct stat vst;
    char vpath[512];
    snprintf(vpath,
             sizeof(vpath),
             "%s%ctest.txt.br",
             dir,
             FIO_FOLDER_SEPARATOR);
    FIO_ASSERT(!fio_filename_stat(vpath, &vst) && vst.st_size > 0,
               "detached creation: the .br variant must be created on "
               "disk");
    fio_filename_remove(.path = vpath);
  }
  fio_http_free(h);
  test_static_tree_cleanup(dir);
}

/* ===========================================================================   Main
   ===========================================================================
 */

int main(void) {
  fprintf(stderr, "Testing fio_http high-level behavior:\n");

  test_resource_action();
  test_listen_and_route();
  /* In-process reactor roundtrip omitted: running fio_io_start inside a
     correctness test reliably crashes during reactor shutdown, and the crash
     is in reactor/connection cleanup rather than HTTP logic. The listen and
     routing test above already exercises fio_http_listen creation. */
  test_settings_and_io_queries();
  test_static_file_response();
  test_error_response();
  test_websocket_upgrade_helpers();
  test_sse_upgrade_helpers();
  test_sse_newline_first_edge_case();
  test_static_vary_and_range_guards();
  test_websocket_deflate_negotiation();
  test_websocket_connect_wrapper();
  test_static_compress_note_result();
  test_static_compress_attached_readonly();
  test_static_compress_detached_creation();

  fprintf(stderr, "\nAll high-level HTTP tests passed!\n");
  return 0;
}
