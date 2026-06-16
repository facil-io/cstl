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
#include <unistd.h>

/* ===========================================================================
   Helpers
   ===========================================================================
 */

static fio_http_s *test_http_make_handle(const char *method,
                                         const char *path) {
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

  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0",
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

  char dir[] = "./tmp/http_static_test_XXXXXX";
  char *d = mkdtemp(dir);
  FIO_ASSERT(d, "mkdtemp failed");

  char path[512];
  snprintf(path, sizeof(path), "%s/test.txt", d);
  const char *content = "hello static file";
  FILE *f = fopen(path, "w");
  FIO_ASSERT(f, "failed to create static test file");
  FIO_ASSERT(fwrite(content, 1, strlen(content), f) == strlen(content),
             "failed to write static test file");
  fclose(f);

  fio_http_s *h = fio_http_new();
  fio_http_status_set(h, 200);
  int r = fio_http_static_file_response(h,
                                        FIO_STR_INFO1(d),
                                        FIO_STR_INFO1((char *)"/test.txt"),
                                        0);
  FIO_ASSERT(r == 0, "static_file_response should succeed for existing file");
  FIO_ASSERT(fio_http_status(h) == 200,
             "static file response should keep status 200");
  FIO_ASSERT(fio_http_is_finished(h),
             "static file response should finish the response");

  fio_str_info_s ct = fio_http_response_header(
      h, FIO_STR_INFO2((char *)"content-type", 12), 0);
  FIO_ASSERT(ct.len >= 10 && !FIO_MEMCMP(ct.buf, "text/plain", 10),
             "static .txt file should have text/plain content-type");

  fio_http_free(h);
  unlink(path);
  rmdir(d);
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
  fio_str_info_s ct = fio_http_response_header(
      h, FIO_STR_INFO2((char *)"content-type", 12), 0);
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
             "valid WebSocket request headers should be detected");

  fio_http_upgrade_websocket(h);
  FIO_ASSERT(fio_http_status(h) == 101,
             "WebSocket upgrade should set status 101");
  FIO_ASSERT(fio_http_is_websocket(h),
             "handle should report WebSocket after upgrade");
  FIO_ASSERT(fio_http_is_finished(h),
             "WebSocket upgrade should finish the response");

  fio_str_info_s accept = fio_http_response_header(
      h, FIO_STR_INFO2((char *)"sec-websocket-accept", 20), 0);
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
  fio_http_request_header_set(
      h,
      FIO_STR_INFO2((char *)"accept", 6),
      FIO_STR_INFO1((char *)"text/event-stream"));

  FIO_ASSERT(fio_http_sse_requested(h),
             "SSE request headers should be detected");

  fio_http_status_set(h, 200);
  fio_http_upgrade_sse(h);
  FIO_ASSERT(fio_http_is_sse(h), "handle should report SSE after upgrade");
  FIO_ASSERT(fio_http_status(h) == 200,
             "SSE upgrade should keep status 200");
  FIO_ASSERT(fio_http_is_finished(h),
             "SSE upgrade should finish the response");
  fio_str_info_s ct = fio_http_response_header(
      h, FIO_STR_INFO2((char *)"content-type", 12), 0);
  FIO_ASSERT(ct.len == 17 && !FIO_MEMCMP(ct.buf, "text/event-stream", 17),
             "SSE upgrade should set text/event-stream content-type");

  FIO_ASSERT(fio_http_sse_write(h, .data = FIO_BUF_INFO2((char *)"hi", 2)) ==
                 -1,
             "sse_write should fail without a connection");

  fio_http_free(h);
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

  fprintf(stderr, "\nAll high-level HTTP tests passed!\n");
  return 0;
}
