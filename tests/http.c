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

#if FIO_OS_WIN
  FIO_ASSERT(CreateDirectoryA(dir, NULL),
             "failed to create static test directory");
#else
  FIO_ASSERT(mkdir(dir, 0755) == 0, "failed to create static test directory");
#endif

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
#if FIO_OS_WIN
  DeleteFileA(path);
  RemoveDirectoryA(dir);
#else
  unlink(path);
  rmdir(dir);
#endif
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
#if FIO_OS_WIN
  if (!CreateDirectoryA(dir, NULL))
    return 0;
#else
  if (mkdir(dir, 0755))
    return 0;
#endif

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
  char path[512];
  snprintf(path, sizeof(path), "%s%ctest.txt", dir, FIO_FOLDER_SEPARATOR);
  unlink(path);
  snprintf(path,
           sizeof(path),
           "%s%ctest.txt.gz",
           dir,
           FIO_FOLDER_SEPARATOR);
  unlink(path);
  rmdir(dir);
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
   Main
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

  fprintf(stderr, "\nAll high-level HTTP tests passed!\n");
  return 0;
}
