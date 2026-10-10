/* *****************************************************************************
Test: high-level HTTP module behavior (fio-stl/439 http.h)

Correctness-only coverage for the HTTP listener, router, resource-action
helper, static-file serving, error responses, and WebSocket/SSE upgrade
helpers that wrap the HTTP handle.

No performance loops, no external processes, no external network calls.
Loopback sockets are used for listener creation; one final test runs the
reactor for an in-process client/server roundtrip.
***************************************************************************** */
#define FIO_HTTP
#include "test-helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===========================================================================
   Windows CI crash trap (permanent diagnostic net)

   The Windows CI release step sporadically hard-crashes (silent access
   violation; FIO_ASSERT never fires) in the WebSocket connect drain, on real
   NT runners only — never reproduced under Wine/macOS/Linux/sanitizers, and
   still flaky with -march=x86-64-v3. This trap prints the exception code,
   faulting RIP, and a stack walk as module+RVA pairs, so a CI crash log can
   be symbolized offline against a same-commit -gcodeview build, e.g.:
     llvm-symbolizer --obj=http.exe <preferred_base+RVA>
   (llvm-symbolizer on PE expects the link-time preferred base + RVA — a bare
   RVA yields ??:0:0; the trap prints ready-to-paste `sym=` addresses.)
   Uses WriteFile (not stdio) — keeps working even with a damaged heap.
   ===========================================================================
 */
#if defined(_WIN32)
#include <windows.h>

static void test_http_win_crash_emit(const char *msg, size_t len) {
  DWORD written = 0;
  HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
  if (err && err != INVALID_HANDLE_VALUE)
    WriteFile(err, msg, (DWORD)len, &written, NULL);
}

/* Link-time preferred image base (PDB addresses are relative to this, so
   llvm-symbolizer needs preferred_base + RVA, regardless of ASLR). */
static unsigned long long test_http_win_preferred_base(HMODULE mod) {
  const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)(uintptr_t)mod;
  const IMAGE_NT_HEADERS64 *nt;
  if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
    return 0;
  nt = (const IMAGE_NT_HEADERS64 *)(uintptr_t)((char *)mod + dos->e_lfanew);
  if (nt->Signature != IMAGE_NT_SIGNATURE)
    return 0;
  return (unsigned long long)nt->OptionalHeader.ImageBase;
}

static LONG WINAPI test_http_win_crash_trap(EXCEPTION_POINTERS *ep) {
  char buf[512];
  int n;
  HMODULE exe = GetModuleHandleA(NULL);
  const unsigned long long preferred = test_http_win_preferred_base(exe);
#define TEST_HTTP_CRASH_LOG(...)                                               \
  do {                                                                         \
    n = snprintf(buf, sizeof(buf), __VA_ARGS__);                               \
    if (n > 0)                                                                 \
      test_http_win_crash_emit(                                                \
          buf,                                                                 \
          (size_t)((size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf) - 1));    \
  } while (0)

  {
    const void *rip = (const void *)ep->ExceptionRecord->ExceptionAddress;
    HMODULE rip_mod = NULL;
    char rip_sym[64];
    rip_sym[0] = 0;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)rip,
                       &rip_mod);
    if (rip_mod == exe && preferred)
      snprintf(rip_sym,
               sizeof(rip_sym),
               " sym=0x%llX",
               preferred +
                   (unsigned long long)((uintptr_t)rip - (uintptr_t)exe));
    TEST_HTTP_CRASH_LOG(
        "\nFATAL: unhandled Windows exception 0x%08lX at RIP=%p%s%s\n",
        (unsigned long)ep->ExceptionRecord->ExceptionCode,
        rip,
        rip_sym,
        (rip_mod == exe) ? "" : " (RIP outside exe)");
  }
  if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
      ep->ExceptionRecord->NumberParameters >= 2) {
    ULONG_PTR op = ep->ExceptionRecord->ExceptionInformation[0];
    TEST_HTTP_CRASH_LOG("FATAL: access violation (%s) at data address %p\n",
                        op == 0   ? "read"
                        : op == 1 ? "write"
                        : op == 8 ? "execute"
                                  : "op?",
                        (void *)ep->ExceptionRecord->ExceptionInformation[1]);
  }
  {
    void *frames[64];
    USHORT count = RtlCaptureStackBackTrace(0, 64, frames, NULL);
    TEST_HTTP_CRASH_LOG("FATAL: module base=%p preferred=0x%llX — symbolize "
                        "with llvm-symbolizer --obj=<exe> <sym addr>\n",
                        (void *)exe,
                        preferred);
    for (USHORT i = 0; i < count; ++i) {
      HMODULE mod = NULL;
      char path[MAX_PATH];
      const char *base = "?";
      path[0] = 0;
      if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                 GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             (LPCSTR)frames[i],
                             &mod) &&
          mod) {
        if (GetModuleFileNameA(mod, path, MAX_PATH)) {
          base = path;
          for (const char *p = path; *p; ++p)
            if (*p == '\\' || *p == '/')
              base = p + 1;
        }
      }
      {
        char sym[48];
        sym[0] = 0;
        if (mod == exe && preferred)
          snprintf(sym,
                   sizeof(sym),
                   " sym=0x%llX",
                   preferred + (unsigned long long)((uintptr_t)frames[i] -
                                                    (uintptr_t)mod));
        TEST_HTTP_CRASH_LOG(
            "FATAL:   [%2u] %s+0x%llX%s\n",
            (unsigned)i,
            base,
            mod ? (unsigned long long)((uintptr_t)frames[i] - (uintptr_t)mod)
                : 0ULL,
            sym);
      }
    }
  }
  /* Fault-time register context: the stack walk beyond the fault frame has
     proven unreliable (return addresses that don't align to post-call
     boundaries), so dump the raw CONTEXT instead. In the MS x64 ABI the
     first args live in RCX/RDX/R8 — for a fault inside fio_risky_hash, RCX
     holds the hashed buffer pointer throughout the absorb loop. */
  if (ep->ContextRecord) {
    const CONTEXT *cx = ep->ContextRecord;
    TEST_HTTP_CRASH_LOG("FATAL: ctx rax=%016llX rbx=%016llX rcx=%016llX "
                        "rdx=%016llX\n",
                        (unsigned long long)cx->Rax,
                        (unsigned long long)cx->Rbx,
                        (unsigned long long)cx->Rcx,
                        (unsigned long long)cx->Rdx);
    TEST_HTTP_CRASH_LOG("FATAL: ctx rsi=%016llX rdi=%016llX rbp=%016llX "
                        "rsp=%016llX\n",
                        (unsigned long long)cx->Rsi,
                        (unsigned long long)cx->Rdi,
                        (unsigned long long)cx->Rbp,
                        (unsigned long long)cx->Rsp);
    TEST_HTTP_CRASH_LOG("FATAL: ctx r8 =%016llX r9 =%016llX r10=%016llX "
                        "r11=%016llX\n",
                        (unsigned long long)cx->R8,
                        (unsigned long long)cx->R9,
                        (unsigned long long)cx->R10,
                        (unsigned long long)cx->R11);
    TEST_HTTP_CRASH_LOG("FATAL: ctx r12=%016llX r13=%016llX r14=%016llX "
                        "r15=%016llX\n",
                        (unsigned long long)cx->R12,
                        (unsigned long long)cx->R13,
                        (unsigned long long)cx->R14,
                        (unsigned long long)cx->R15);
    /* Fault-time stack window: raw qwords beginning at RSP. Any value that
       falls inside the exe image is annotated with its preferred-base
       address (`sym=`) — a call-return candidate for offline symbolization
       (scan-based, so it works even when unwind info misleads). */
    {
      const unsigned long long *sp =
          (const unsigned long long *)(uintptr_t)cx->Rsp;
      MEMORY_BASIC_INFORMATION mbi;
      for (size_t i = 0; i < 48; ++i) {
        const unsigned long long *slot = sp + i;
        if (!VirtualQuery((LPCVOID)slot, &mbi, sizeof(mbi)) ||
            mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_GUARD) ||
            (mbi.Protect & PAGE_NOACCESS))
          break;
        {
          unsigned long long v = *slot;
          char sym[48];
          sym[0] = 0;
          if (preferred && v >= (unsigned long long)(uintptr_t)exe &&
              v < (unsigned long long)(uintptr_t)exe + (16ULL << 20))
            snprintf(sym,
                     sizeof(sym),
                     " sym=0x%llX",
                     preferred + (v - (unsigned long long)(uintptr_t)exe));
          if (sym[0] || !(i & 3)) /* image hits always; else every 4th */
            TEST_HTTP_CRASH_LOG("FATAL: stack[%2zu] rsp+0x%03zX = %016llX%s\n",
                                i,
                                i * sizeof(v),
                                v,
                                sym);
        }
      }
    }
  }
#undef TEST_HTTP_CRASH_LOG
  return EXCEPTION_CONTINUE_SEARCH; /* preserve CI failure semantics + WER */
}
#endif

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
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                FIO_STR_INFO1((char *)"identity"));
    int r = fio_http_static_file_response(h,
                                          FIO_STR_INFO2(dir, dir_len),
                                          FIO_STR_INFO1((char *)"/test.txt"),
                                          0);
    FIO_ASSERT(r == 0, "static plain: response should succeed");
    fio_str_info_s ce =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-encoding", 16),
                                 0);
    FIO_ASSERT(!ce.buf,
               "static plain: content-encoding must be absent (got '%.*s')",
               (int)ce.len,
               ce.buf ? ce.buf : "");
    fio_str_info_s vary =
        fio_http_response_header(h, FIO_STR_INFO2((char *)"vary", 4), 0);
    FIO_ASSERT(vary.len >= 15 &&
                   fio___http_header_has_token(vary, "accept-encoding", 15),
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
    fio_str_info_s ce =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-encoding", 16),
                                 0);
    FIO_ASSERT(ce.len == 4 && !memcmp(ce.buf, "gzip", 4),
               "static gzip: expected content-encoding gzip");
    fio_str_info_s vary =
        fio_http_response_header(h, FIO_STR_INFO2((char *)"vary", 4), 0);
    FIO_ASSERT(vary.len >= 15 &&
                   fio___http_header_has_token(vary, "accept-encoding", 15),
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
    fio_str_info_s ce =
        fio_http_response_header(h,
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
   T009 — Static file: HEAD must mirror GET entity headers (RFC 9110 §9.3.2)

   CDN origin revalidation issues HEAD and compares Content-Length /
   Content-Type / Content-Encoding against the cached GET metadata. The
   static handler used to short-circuit HEAD before setting Content-Type
   and sent an empty write that forced Content-Length: 0.
   ===========================================================================
 */
static void test_static_head_mirrors_get(void) {
  fprintf(stderr, "  * static HEAD mirrors GET entity headers\n");

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
  FIO_ASSERT(dir_len > 0, "failed to create static HEAD test tree");

  char expect_cl[16];
  size_t expect_cl_len = (size_t)
      snprintf(expect_cl, sizeof(expect_cl), "%u", (unsigned)CONTENT_LEN);

  /* 1. Identity GET vs HEAD: same content-length and content-type. */
  fio_str_info_s get_ct = {0};
  {
    fio_http_s *h = test_http_make_handle("GET", "/test.txt");
    fio_http_status_set(h, 200);
    int r = fio_http_static_file_response(h,
                                          FIO_STR_INFO2(dir, dir_len),
                                          FIO_STR_INFO1((char *)"/test.txt"),
                                          0);
    FIO_ASSERT(r == 0, "HEAD mirror: GET should succeed");
    fio_str_info_s cl =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-length", 14),
                                 0);
    FIO_ASSERT(cl.len == expect_cl_len &&
                   !FIO_MEMCMP(cl.buf, expect_cl, expect_cl_len),
               "HEAD mirror: GET content-length sanity (got '%.*s')",
               (int)cl.len,
               cl.buf ? cl.buf : "");
    get_ct = fio_http_response_header(h,
                                      FIO_STR_INFO2((char *)"content-type", 12),
                                      0);
    FIO_ASSERT(get_ct.buf && get_ct.len,
               "HEAD mirror: GET content-type sanity");
    /* header storage is handle-owned; copy before freeing */
    static char ct_copy[128];
    FIO_ASSERT(get_ct.len < sizeof(ct_copy), "content-type too long");
    FIO_MEMCPY(ct_copy, get_ct.buf, get_ct.len);
    get_ct.buf = ct_copy;
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("HEAD", "/test.txt");
    fio_http_status_set(h, 200);
    int r = fio_http_static_file_response(h,
                                          FIO_STR_INFO2(dir, dir_len),
                                          FIO_STR_INFO1((char *)"/test.txt"),
                                          0);
    FIO_ASSERT(r == 0, "HEAD mirror: HEAD should succeed");
    fio_str_info_s cl =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-length", 14),
                                 0);
    FIO_ASSERT(cl.len == expect_cl_len &&
                   !FIO_MEMCMP(cl.buf, expect_cl, expect_cl_len),
               "HEAD must mirror GET content-length %s (got '%.*s')",
               expect_cl,
               (int)cl.len,
               cl.buf ? cl.buf : "");
    fio_str_info_s ct =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-type", 12),
                                 0);
    FIO_ASSERT(ct.buf && ct.len == get_ct.len &&
                   !FIO_MEMCMP(ct.buf, get_ct.buf, get_ct.len),
               "HEAD must mirror GET content-type '%.*s' (got '%.*s')",
               (int)get_ct.len,
               get_ct.buf,
               (int)ct.len,
               ct.buf ? ct.buf : "");
    fio_http_free(h);
  }

  /* 2. HEAD with gzip acceptance: mirror the encoded variant's headers —
   *    Content-Encoding: gzip and the .gz length (what GET would send). */
  size_t gz_len = 0;
  {
    fio_http_s *h = test_http_make_handle("GET", "/test.txt");
    fio_http_status_set(h, 200);
    fio_http_cflags_set(h, FIO_HTTP_CFLAG_COMPRESS_STATIC);
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                FIO_STR_INFO1((char *)"gzip"));
    int r = fio_http_static_file_response(h,
                                          FIO_STR_INFO2(dir, dir_len),
                                          FIO_STR_INFO1((char *)"/test.txt"),
                                          0);
    FIO_ASSERT(r == 0, "HEAD gzip mirror: GET should succeed");
    fio_str_info_s cl =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-length", 14),
                                 0);
    FIO_ASSERT(cl.buf && cl.len, "HEAD gzip mirror: GET content-length sanity");
    static char cl_copy[16];
    FIO_ASSERT(cl.len < sizeof(cl_copy), "content-length too long");
    FIO_MEMCPY(cl_copy, cl.buf, cl.len);
    cl_copy[cl.len] = 0;
    gz_len = (size_t)atol(cl_copy);
    FIO_ASSERT(gz_len > 0 && gz_len < CONTENT_LEN,
               "HEAD gzip mirror: .gz variant should be smaller");
    fio_http_free(h);
  }
  {
    fio_http_s *h = test_http_make_handle("HEAD", "/test.txt");
    fio_http_status_set(h, 200);
    fio_http_cflags_set(h, FIO_HTTP_CFLAG_COMPRESS_STATIC);
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"accept-encoding", 15),
                                FIO_STR_INFO1((char *)"gzip"));
    int r = fio_http_static_file_response(h,
                                          FIO_STR_INFO2(dir, dir_len),
                                          FIO_STR_INFO1((char *)"/test.txt"),
                                          0);
    FIO_ASSERT(r == 0, "HEAD gzip mirror: HEAD should succeed");
    fio_str_info_s ce =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-encoding", 16),
                                 0);
    FIO_ASSERT(ce.len == 4 && !memcmp(ce.buf, "gzip", 4),
               "HEAD gzip mirror: content-encoding gzip expected");
    fio_str_info_s cl =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-length", 14),
                                 0);
    char expect_gz[16];
    size_t expect_gz_len =
        (size_t)snprintf(expect_gz, sizeof(expect_gz), "%u", (unsigned)gz_len);
    FIO_ASSERT(cl.len == expect_gz_len &&
                   !FIO_MEMCMP(cl.buf, expect_gz, expect_gz_len),
               "HEAD gzip mirror: content-length must be the .gz length "
               "%s (got '%.*s')",
               expect_gz,
               (int)cl.len,
               cl.buf ? cl.buf : "");
    fio_str_info_s ct =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-type", 12),
                                 0);
    FIO_ASSERT(ct.buf && ct.len == get_ct.len &&
                   !FIO_MEMCMP(ct.buf, get_ct.buf, get_ct.len),
               "HEAD gzip mirror: content-type must match GET");
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
    if ((size_t)(tok_end - tok) == name_len && !FIO_MEMCMP(tok, name, name_len))
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
        FIO_STR_INFO2((char *)"permessage-deflate; client_max_window_bits", 41),
        out,
        sizeof(out),
        &bits);
    FIO_ASSERT(len > 0, "negotiate: browser-style offer should succeed");
    fio_str_info_s resp = FIO_STR_INFO2(out, len);
    FIO_ASSERT(!test_ws_ext_has_param(resp, "client_max_window_bits", 22),
               "negotiate browser: must not echo client_max_window_bits");
    FIO_ASSERT(
        test_ws_ext_has_param(resp, "server_no_context_takeover", 26) &&
            test_ws_ext_has_param(resp, "client_no_context_takeover", 26),
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
    FIO_ASSERT(len == 0, "negotiate: undersized output buffer must return 0");
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
       fio___http1_on_attach_client): link the IO and the connection.
       The connect scaffold (`fio___io_connecting_s`) currently owns the
       io's udata; `fio___connecting_on_ready` never fires without a
       reactor, so release it here exactly like on_ready's tail would
       (`on_failed` must not run — the WS teardown owns the connection). */
    fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
    /* the io's current protocol lives INSIDE the connect scaffold
       (`&scaffold->protocol`), so the scaffold may only be freed after the
       WS protocol switch below detaches from it (mirrors on_ready's
       deferred cleanup); `on_failed` must not run — the WS teardown owns
       the connection. */
    fio___io_connecting_s *scaffold = (fio___io_connecting_s *)fio_io_udata(io);
    scaffold->on_failed = NULL;
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
    /* the drained protocol switch detached the io from the scaffold's
       embedded protocol — releasing it now balances the connect path
       (fio___connecting_on_ready never fires without a reactor). */
    fio___connecting_cleanup(scaffold);
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
    FIO_ASSERT(s.compress_static == 2,
               "re-seed: expected 2, got %u",
               (unsigned)s.compress_static);
    fio___http_static_compress_note_result(&s, 0);
    FIO_ASSERT(s.compress_static == 3 && (s.compress_static & 1),
               "re-seed: success must set bit 0 (expected 3, got %u)",
               (unsigned)s.compress_static);
    fio___http_static_compress_note_result(&s, EINVAL);
    FIO_ASSERT(s.compress_static == 6,
               "re-seed: expected 6, got %u",
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
  FIO_ASSERT(st, "read-only test: attached handle must resolve route settings");
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
  fio_str_info_s ce =
      fio_http_response_header(h,
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
  fio_str_info_s ce =
      fio_http_response_header(h,
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

/* ===========================================================================
   Client / server roundtrip (live reactor)

   Our own HTTP client (`fio_http_connect`) talks to our own HTTP server
   (`fio_http_listen`) over loopback. Single process: the reactor runs on the
   main (IO) thread; server and client HTTP callbacks run on one async worker
   thread (`.queue`). A 3 second
   timer guards against hangs; `fio_io_stop` ends the reactor.
   ===========================================================================
 */

#define TEST_RT_BODY "roundtrip-ok"

static struct {
  fio_thread_t io_thread;
  volatile int server_calls;
  volatile int server_on_worker;
  volatile int client_calls;
  volatile int client_on_worker;
  volatile int client_status;
  volatile int client_body_ok;
  volatile int timed_out;
  volatile int active; /* the guard timer may outlive this reactor run */
} test_rt;

static void test_rt_server_on_http(fio_http_s *h) {
  fio_thread_t self = fio_thread_current();
  test_rt.server_on_worker = !fio_thread_equal(&self, &test_rt.io_thread);
  fio_str_info_s path = fio_http_path(h);
  if (path.len == 10 && !FIO_MEMCMP(path.buf, "/roundtrip", 10))
    fio_atomic_add(&test_rt.server_calls, 1);
  fio_http_write(h,
                 .buf = (char *)TEST_RT_BODY,
                 .len = sizeof(TEST_RT_BODY) - 1,
                 .finish = 1);
}

static void test_rt_client_on_http(fio_http_s *h) {
  fio_thread_t self = fio_thread_current();
  test_rt.client_on_worker = !fio_thread_equal(&self, &test_rt.io_thread);
  test_rt.client_status = (int)fio_http_status(h);
  fio_str_info_s body = fio_http_body_read(h, (size_t)-1);
  test_rt.client_body_ok = (body.len == sizeof(TEST_RT_BODY) - 1 &&
                            !FIO_MEMCMP(body.buf, TEST_RT_BODY, body.len));
  fio_atomic_add(&test_rt.client_calls, 1);
  fio_io_stop();
}

static int test_rt_timeout(void *ignr1, void *ignr2) {
  (void)ignr1, (void)ignr2;
  if (!test_rt.active)
    return -1;
  test_rt.timed_out = 1;
  fio_io_stop();
  return -1;
}

static void test_http_client_server_roundtrip(void) {
  fprintf(stderr,
          "  * client/server roundtrip (1 IO thread + 1 worker thread)\n");
  static fio_io_async_s worker = FIO_IO_ASYN_INIT;
  fio_io_async_attach(&worker, 1);
  FIO_MEMSET(&test_rt, 0, sizeof(test_rt));
  test_rt.io_thread = fio_thread_current();

  fio_io_run_every(.fn = test_rt_timeout, .every = 3000, .repetitions = 1);

  fio_http_listener_s *l = fio_http_listen("tcp://127.0.0.1:0",
                                           .on_http = test_rt_server_on_http,
                                           .queue = &worker);
  FIO_ASSERT(l, "roundtrip: fio_http_listen failed");
  unsigned port = test_ws_listener_port(l);
  FIO_ASSERT(port, "roundtrip: listener port discovery failed");

  char url[128];
  snprintf(url, sizeof(url), "http://127.0.0.1:%u/roundtrip", port);
  fio_io_s *io = fio_http_connect(url,
                                  NULL,
                                  .on_http = test_rt_client_on_http,
                                  .queue = &worker);
  FIO_ASSERT(io, "roundtrip: fio_http_connect failed");

  test_rt.active = 1;
  fio_io_start(0); /* single process: 1 IO thread (this thread) */
  test_rt.active = 0;

  fio_io_listen_stop((fio_io_listener_s *)l);
  FIO_ASSERT(!test_rt.timed_out, "roundtrip: timed out after 3 seconds");
  FIO_ASSERT(test_rt.server_calls == 1,
             "roundtrip: server on_http should run once (got %d)",
             test_rt.server_calls);
  FIO_ASSERT(test_rt.server_on_worker,
             "roundtrip: server on_http should run on the worker thread");
  FIO_ASSERT(test_rt.client_calls == 1,
             "roundtrip: client on_http should run once (got %d)",
             test_rt.client_calls);
  FIO_ASSERT(test_rt.client_on_worker,
             "roundtrip: client on_http should run on the worker thread");
  FIO_ASSERT(test_rt.client_status == 200,
             "roundtrip: expected status 200 (got %d)",
             test_rt.client_status);
  FIO_ASSERT(test_rt.client_body_ok, "roundtrip: response body mismatch");
}
#undef TEST_RT_BODY

/* ===========================================================================
   Client: interim 1xx responses and HEAD responses (live reactor)

   A raw (non-HTTP) loopback server replies with canned bytes, so the client
   sees exactly what a third-party server may send:
   * GET  -> `100 Continue`, `103 Early Hints`, then the final `200` response.
     The client must skip interim responses (RFC 9110 §15.2) and dispatch only
     the final response (without the interim headers).
   * HEAD -> `200` with `Content-Length: 5` and no body. The client must not
     wait for a body (RFC 9110 §9.3.2); the connection is left open, so a
     regression times out.
   ===========================================================================
 */

static char test_raw_interim_reply[] =
    "HTTP/1.1 100 Continue\r\n\r\n"
    "HTTP/1.1 103 Early Hints\r\nLink: </s.css>; rel=preload\r\n\r\n"
    "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nX-Final: 1\r\n\r\nok";
static char test_raw_head_reply[] =
    "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\n";

static struct {
  volatile int get_calls;
  volatile int get_status;
  volatile int get_body_ok;
  volatile int get_final_header;
  volatile int get_interim_header;
  volatile int head_calls;
  volatile int head_status;
  volatile int head_body_len;
  volatile int head_clen_ok;
  volatile int timed_out;
  volatile int active; /* the guard timer may outlive this reactor run */
} test_raw;

static void test_raw_server_on_data(fio_io_s *io) {
  char buf[1024];
  size_t r = fio_io_read(io, buf, sizeof(buf));
  if (!r || fio_io_udata(io))
    return; /* one canned reply per connection */
  fio_io_udata_set(io, (void *)1);
  if (r >= 4 && fio_buf2u32u(buf) == fio_buf2u32u("HEAD"))
    fio_io_write(io, test_raw_head_reply, sizeof(test_raw_head_reply) - 1);
  else
    fio_io_write(io,
                 test_raw_interim_reply,
                 sizeof(test_raw_interim_reply) - 1);
}

static void test_raw_maybe_stop(void) {
  if (test_raw.get_calls && test_raw.head_calls)
    fio_io_stop();
}

static void test_raw_client_on_http(fio_http_s *h) {
  fio_str_info_s m = fio_http_method(h);
  fio_str_info_s body = fio_http_body_read(h, (size_t)-1);
  if (m.len == 4 && fio_buf2u32u(m.buf) == fio_buf2u32u("HEAD")) {
    test_raw.head_status = (int)fio_http_status(h);
    test_raw.head_body_len = (int)body.len;
    fio_str_info_s cl =
        fio_http_response_header(h,
                                 FIO_STR_INFO2((char *)"content-length", 14),
                                 0);
    test_raw.head_clen_ok = (cl.len == 1 && cl.buf[0] == '5');
    ++test_raw.head_calls;
  } else {
    test_raw.get_status = (int)fio_http_status(h);
    test_raw.get_body_ok =
        (body.len == 2 && fio_buf2u16u(body.buf) == fio_buf2u16u("ok"));
    test_raw.get_final_header =
        !!fio_http_response_header(h, FIO_STR_INFO2((char *)"x-final", 7), 0)
              .len;
    test_raw.get_interim_header =
        !!fio_http_response_header(h, FIO_STR_INFO2((char *)"link", 4), 0)
              .len;
    ++test_raw.get_calls;
  }
  test_raw_maybe_stop();
}

static int test_raw_timeout(void *ignr1, void *ignr2) {
  (void)ignr1, (void)ignr2;
  if (!test_raw.active)
    return -1;
  test_raw.timed_out = 1;
  fio_io_stop();
  return -1;
}

static void test_http_client_interim_and_head(void) {
  fprintf(stderr, "  * client skips 1xx interim responses / HEAD has no body\n");
  static fio_io_protocol_s raw_protocol;
  raw_protocol = (fio_io_protocol_s){
      .on_data = test_raw_server_on_data,
      .on_timeout = fio_io_touch,
      .timeout = 5000,
  };
  FIO_MEMSET(&test_raw, 0, sizeof(test_raw));
  fio_io_run_every(.fn = test_raw_timeout, .every = 3000, .repetitions = 1);

  fio_io_listener_s *l = fio_io_listen(.url = "tcp://127.0.0.1:0",
                                       .protocol = &raw_protocol,
                                       .hide_from_log = 1);
  FIO_ASSERT(l, "interim/head: raw listener failed");
  unsigned port = test_ws_listener_port((fio_http_listener_s *)l);
  FIO_ASSERT(port, "interim/head: listener port discovery failed");

  char url[128];
  snprintf(url, sizeof(url), "http://127.0.0.1:%u/interim", port);
  FIO_ASSERT(fio_http_connect(url, NULL, .on_http = test_raw_client_on_http),
             "interim/head: GET connect failed");
  snprintf(url, sizeof(url), "http://127.0.0.1:%u/head", port);
  fio_http_s *head = fio_http_new();
  fio_http_method_set(head, FIO_STR_INFO2((char *)"HEAD", 4));
  FIO_ASSERT(fio_http_connect(url, head, .on_http = test_raw_client_on_http),
             "interim/head: HEAD connect failed");

  test_raw.active = 1;
  fio_io_start(0);
  test_raw.active = 0;

  fio_io_listen_stop(l);
  FIO_ASSERT(!test_raw.timed_out,
             "interim/head: timed out (GET calls %d, HEAD calls %d)",
             test_raw.get_calls,
             test_raw.head_calls);
  FIO_ASSERT(test_raw.get_calls == 1,
             "interim: on_http should run once, for the final response "
             "(got %d)",
             test_raw.get_calls);
  FIO_ASSERT(test_raw.get_status == 200,
             "interim: expected final status 200 (got %d)",
             test_raw.get_status);
  FIO_ASSERT(test_raw.get_body_ok, "interim: final response body mismatch");
  FIO_ASSERT(test_raw.get_final_header,
             "interim: final response header missing");
  FIO_ASSERT(!test_raw.get_interim_header,
             "interim: 1xx headers leaked into the final response");
  FIO_ASSERT(test_raw.head_calls == 1,
             "head: on_http should run once (got %d)",
             test_raw.head_calls);
  FIO_ASSERT(test_raw.head_status == 200,
             "head: expected status 200 (got %d)",
             test_raw.head_status);
  FIO_ASSERT(!test_raw.head_body_len,
             "head: response must not have a body (got %d bytes)",
             test_raw.head_body_len);
  FIO_ASSERT(test_raw.head_clen_ok,
             "head: content-length should be visible as a response header");
}

/* ===========================================================================
   Server: a response line sent by a peer (live reactor)

   A server never expects HTTP responses. The connection must be rejected
   (logged as a SECURITY event and closed) rather than tripping a debug
   assertion. A raw client sends a response line and waits for the close.
   ===========================================================================
 */

static struct {
  volatile int closed;
  volatile int received;
  volatile int timed_out;
  volatile int active; /* the guard timer may outlive this reactor run */
} test_srv_resp;

static void test_srv_resp_on_attach(fio_io_s *io) {
  static char msg[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
  fio_io_write(io, msg, sizeof(msg) - 1);
}

static void test_srv_resp_on_data(fio_io_s *io) {
  char buf[256];
  test_srv_resp.received += (int)fio_io_read(io, buf, sizeof(buf));
}

static void test_srv_resp_on_close(void *iobuf, void *udata) {
  (void)iobuf, (void)udata;
  test_srv_resp.closed = 1;
  fio_io_stop();
}

static int test_srv_resp_timeout(void *ignr1, void *ignr2) {
  (void)ignr1, (void)ignr2;
  if (!test_srv_resp.active)
    return -1;
  test_srv_resp.timed_out = 1;
  fio_io_stop();
  return -1;
}

static void test_http_server_rejects_response(void) {
  fprintf(stderr,
          "  * server rejects a response line (expect a SECURITY log)\n");
  static fio_io_protocol_s raw_client;
  raw_client = (fio_io_protocol_s){
      .on_attach = test_srv_resp_on_attach,
      .on_data = test_srv_resp_on_data,
      .on_close = test_srv_resp_on_close,
      .on_timeout = fio_io_touch,
      .timeout = 5000,
  };
  FIO_MEMSET(&test_srv_resp, 0, sizeof(test_srv_resp));
  fio_io_run_every(.fn = test_srv_resp_timeout,
                   .every = 3000,
                   .repetitions = 1);
  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0", .on_http = test_http_noop_on_http);
  FIO_ASSERT(l, "server/response: fio_http_listen failed");
  unsigned port = test_ws_listener_port(l);
  FIO_ASSERT(port, "server/response: listener port discovery failed");
  char url[128];
  snprintf(url, sizeof(url), "tcp://127.0.0.1:%u", port);
  FIO_ASSERT(fio_io_connect(url, .protocol = &raw_client),
             "server/response: raw connect failed");

  test_srv_resp.active = 1;
  fio_io_start(0);
  test_srv_resp.active = 0;

  fio_io_listen_stop((fio_io_listener_s *)l);
  FIO_ASSERT(!test_srv_resp.timed_out,
             "server/response: connection should be closed by the server");
  FIO_ASSERT(test_srv_resp.closed, "server/response: on_close missing");
  FIO_ASSERT(!test_srv_resp.received,
             "server/response: server should not reply (got %d bytes)",
             test_srv_resp.received);
}

/* ===========================================================================
   Server: request / application close must drain without dispatching a pipeline

   Each raw client queues two requests in ONE write. Stream enough response data
   to exceed typical socket buffers, then finish with a distinct final chunk.
   Only the raw peer's close callback stops the reactor (never body receipt),
   and it checks for actual EOF while its socket is still valid. Validate the
   complete chunked wire response only after the reactor has stopped.
   ===========================================================================
 */

enum {
  TEST_HTTP_CLOSE_CHUNK_SIZE = 65536,
  TEST_HTTP_CLOSE_CHUNKS = 64,
  TEST_HTTP_CLOSE_BODY_SIZE =
      TEST_HTTP_CLOSE_CHUNK_SIZE * TEST_HTTP_CLOSE_CHUNKS
};

static char test_http_close_chunk[TEST_HTTP_CLOSE_CHUNK_SIZE];
static const char test_http_close_tail[] = "final chunk before EOF\n";

static struct {
  const char *request;
  fio_socket_i client_fd;
  uintptr_t generation;
  size_t received;
  int close_after_finish;
  int server_calls;
  int finish_calls;
  int client_closes;
  int eof;
  int bad_path;
  int unfinished;
  int close_still_open;
  int overflow;
  int connect_failed;
  int timed_out;
  int active;
  char wire[2 * (TEST_HTTP_CLOSE_BODY_SIZE + 8192)];
} test_http_close;

static void test_http_close_server_on_http(fio_http_s *h) {
  fio_str_info_s path = fio_http_path(h);
  const char *expected = test_http_close.server_calls ? "/second" : "/first";
  ++test_http_close.server_calls;
  if (path.len != strlen(expected) || FIO_MEMCMP(path.buf, expected, path.len))
    test_http_close.bad_path = 1;
  fio_http_status_set(h, 200);
  /* Large no-copy chunks (the first one follows the pending headers). */
  for (size_t i = 0; i < TEST_HTTP_CLOSE_CHUNKS; ++i)
    fio_http_write(h,
                   .buf = test_http_close_chunk,
                   .len = sizeof(test_http_close_chunk),
                   .copy = 0);
  fio_http_write(h,
                 .buf = test_http_close_tail,
                 .len = sizeof(test_http_close_tail) - 1,
                 .finish = 1);
}

static void test_http_close_server_on_finish(fio_http_s *h) {
  ++test_http_close.finish_calls;
  if (!fio_http_is_finished(h))
    test_http_close.unfinished = 1;
  if (test_http_close.close_after_finish == test_http_close.finish_calls) {
    fio_http_close(h);
    if (fio_io_is_open(fio_http_io(h)))
      test_http_close.close_still_open = 1;
  }
}

static void test_http_close_client_on_attach(fio_io_s *io) {
  test_http_close.client_fd = fio_io_fd(io);
  fio_io_write(io,
               (void *)test_http_close.request,
               strlen(test_http_close.request));
}

static void test_http_close_capture(const char *buf, size_t len) {
  if (len >= sizeof(test_http_close.wire) - test_http_close.received) {
    test_http_close.overflow = 1;
    return;
  }
  FIO_MEMCPY(test_http_close.wire + test_http_close.received, buf, len);
  test_http_close.received += len;
  test_http_close.wire[test_http_close.received] = 0;
}

static void test_http_close_client_on_data(fio_io_s *io) {
  char buf[16384];
  size_t len;
  while ((len = fio_io_read(io, buf, sizeof(buf))))
    test_http_close_capture(buf, len);
  /* Deliberately keep running after the complete body: closure is required. */
}

static void test_http_close_client_on_close(void *iobuf, void *udata) {
  (void)iobuf, (void)udata;
  char buf[16384];
  ssize_t len;
  ++test_http_close.client_closes;
  /* on_close runs before fio_sock_close. Drain any data accompanying a poll
   * hangup, and distinguish clean EOF from a reset or a local shutdown. */
  do {
    len = fio_sock_read(test_http_close.client_fd, buf, sizeof(buf));
    if (len > 0)
      test_http_close_capture(buf, (size_t)len);
  } while (len > 0);
  test_http_close.eof = (len == 0);
  if (test_http_close.active)
    fio_io_stop();
}

static void test_http_close_client_on_failed(fio_io_protocol_s *pr,
                                             void *udata) {
  (void)pr, (void)udata;
  test_http_close.connect_failed = 1;
  fio_io_stop();
}

static int test_http_close_timeout(void *generation, void *ignr) {
  (void)ignr;
  /* Earlier runs may leave their one-shot guards queued. */
  if (!test_http_close.active ||
      (uintptr_t)generation != test_http_close.generation)
    return -1;
  test_http_close.timed_out = 1;
  fio_io_stop();
  return -1;
}

static const char *test_http_close_line_end(const char *pos, const char *end) {
  for (; end - pos >= 2; ++pos)
    if (pos[0] == '\r' && pos[1] == '\n')
      return pos;
  return NULL;
}

static int test_http_close_wire_error(const char *reason,
                                      const char *pos,
                                      size_t response,
                                      size_t body_len) {
  size_t offset = (size_t)(pos - test_http_close.wire);
  fprintf(stderr,
          "    wire validation: %s; response=%zu offset=%zu body=%zu "
          "received=%zu; next bytes:",
          reason,
          response + 1,
          offset,
          body_len,
          test_http_close.received);
  for (size_t i = 0; i < 24 && i < test_http_close.received - offset; ++i)
    fprintf(stderr, " %02x", (unsigned)(unsigned char)pos[i]);
  fprintf(stderr, "\n");
  return 0;
}

/** Decode the captured chunked responses, checking every payload byte, the
 * distinct last chunk, the zero-chunk terminator, and absence of extra data. */
static int test_http_close_wire_ok(size_t expected_responses) {
  static const char status_prefix[] = "HTTP/1.1 200 ";
  const char *pos = test_http_close.wire;
  const char *end = pos + test_http_close.received;
  for (size_t response = 0; response < expected_responses; ++response) {
    if ((size_t)(end - pos) < sizeof(status_prefix) - 1 ||
        FIO_MEMCMP(pos, status_prefix, sizeof(status_prefix) - 1))
      return test_http_close_wire_error("status prefix", pos, response, 0);
    const char *header_end = pos;
    while (end - header_end >= 4 && FIO_MEMCMP(header_end, "\r\n\r\n", 4))
      ++header_end;
    if (end - header_end < 4)
      return test_http_close_wire_error("missing header terminator",
                                        header_end,
                                        response,
                                        0);
    const char *chunked = strstr(pos, "\r\ntransfer-encoding: chunked\r\n");
    if (!chunked || chunked >= header_end)
      return test_http_close_wire_error("missing chunked header",
                                        pos,
                                        response,
                                        0);
    pos = header_end + 4;
    size_t body_len = 0;
    for (;;) {
      const char *line_end = test_http_close_line_end(pos, end);
      size_t chunk_len = 0;
      if (!line_end || pos == line_end)
        return test_http_close_wire_error("missing / empty chunk length",
                                          pos,
                                          response,
                                          body_len);
      for (; pos < line_end; ++pos) {
        unsigned char c = (unsigned char)*pos;
        size_t digit;
        if (c >= '0' && c <= '9')
          digit = (size_t)(c - '0');
        else if ((c | 32U) >= 'a' && (c | 32U) <= 'f')
          digit = (size_t)((c | 32U) - 'a' + 10U);
        else
          return test_http_close_wire_error("invalid chunk hex digit",
                                            pos,
                                            response,
                                            body_len);
        if (chunk_len > (SIZE_MAX - digit) / 16)
          return test_http_close_wire_error("chunk length overflow",
                                            pos,
                                            response,
                                            body_len);
        chunk_len = chunk_len * 16 + digit;
      }
      pos = line_end + 2;
      if (end - pos < 2 || chunk_len > (size_t)(end - pos - 2))
        return test_http_close_wire_error("truncated chunk payload / trailer",
                                          pos,
                                          response,
                                          body_len);
      if (FIO_MEMCMP(pos + chunk_len, "\r\n", 2))
        return test_http_close_wire_error("invalid chunk trailer",
                                          pos + chunk_len,
                                          response,
                                          body_len);
      if (!chunk_len) {
        pos += 2;
        break;
      }
      for (size_t i = 0; i < chunk_len; ++i, ++body_len) {
        char expected;
        if (body_len < TEST_HTTP_CLOSE_BODY_SIZE)
          expected =
              test_http_close_chunk[body_len % TEST_HTTP_CLOSE_CHUNK_SIZE];
        else if (body_len - TEST_HTTP_CLOSE_BODY_SIZE <
                 sizeof(test_http_close_tail) - 1)
          expected = test_http_close_tail[body_len - TEST_HTTP_CLOSE_BODY_SIZE];
        else
          return test_http_close_wire_error("excess payload",
                                            pos + i,
                                            response,
                                            body_len);
        if (pos[i] != expected)
          return test_http_close_wire_error("payload byte mismatch",
                                            pos + i,
                                            response,
                                            body_len);
      }
      pos += chunk_len + 2;
    }
    if (body_len !=
        TEST_HTTP_CLOSE_BODY_SIZE + sizeof(test_http_close_tail) - 1)
      return test_http_close_wire_error("incomplete payload before zero chunk",
                                        pos,
                                        response,
                                        body_len);
  }
  if (pos != end)
    return test_http_close_wire_error("unexpected data after responses",
                                      pos,
                                      expected_responses,
                                      0);
  return 1;
}

static void test_http_server_close_pipeline(void) {
  fprintf(stderr, "  * server close drains response and suppresses pipeline\n");
  static const struct {
    const char *name;
    const char *request;
    int close_after_finish;
    int expected_calls;
  } cases[] = {
      {"request close",
       "GET /first HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
       "GET /second HTTP/1.1\r\nHost: localhost\r\n\r\n",
       0,
       1},
      {"request comma / mixed case close",
       "GET /first HTTP/1.1\r\nHost: localhost\r\n"
       "Connection: keep-alive, \tClOsE \t, upgrade\r\n\r\n"
       "GET /second HTTP/1.1\r\nHost: localhost\r\n\r\n",
       0,
       1},
      {"request repeated connection headers",
       "GET /first HTTP/1.1\r\nHost: localhost\r\n"
       "Connection: keep-alive\r\ncOnNeCtIoN: upgrade, CLOSE\r\n\r\n"
       "GET /second HTTP/1.1\r\nHost: localhost\r\n\r\n",
       0,
       1},
      {"application close in first on_finish",
       "GET /first HTTP/1.1\r\nHost: localhost\r\n\r\n"
       "GET /second HTTP/1.1\r\nHost: localhost\r\n\r\n",
       1,
       1},
      {"keepalive pipeline control",
       "GET /first HTTP/1.1\r\nHost: localhost\r\nConnection: "
       "keep-alive\r\n\r\n"
       "GET /second HTTP/1.1\r\nHost: localhost\r\n\r\n",
       2,
       2},
  };
  static fio_io_protocol_s raw_client;
  static uintptr_t generation;
  static fio_io_async_s worker = FIO_IO_ASYN_INIT;
  fio_io_async_attach(&worker, 1);
  for (size_t i = 0; i < sizeof(test_http_close_chunk); ++i)
    test_http_close_chunk[i] = (char)('!' + i % 90);
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    fprintf(stderr, "    - %s\n", cases[i].name);
    raw_client = (fio_io_protocol_s){
        .on_attach = test_http_close_client_on_attach,
        .on_data = test_http_close_client_on_data,
        .on_close = test_http_close_client_on_close,
        .on_timeout = fio_io_touch,
        .timeout = 10000,
    };
    FIO_MEMSET(&test_http_close, 0, sizeof(test_http_close));
    test_http_close.client_fd = FIO_SOCKET_INVALID;
    test_http_close.generation = ++generation;
    test_http_close.request = cases[i].request;
    test_http_close.close_after_finish = cases[i].close_after_finish;
    fio_io_run_every(.fn = test_http_close_timeout,
                     .udata1 = (void *)test_http_close.generation,
                     .every = 5000,
                     .repetitions = 1);
    fio_http_listener_s *l =
        fio_http_listen("tcp://127.0.0.1:0",
                        .on_http = test_http_close_server_on_http,
                        .on_finish = test_http_close_server_on_finish,
                        .queue = &worker);
    FIO_ASSERT(l, "%s: listener failed", cases[i].name);
    unsigned port = test_ws_listener_port(l);
    FIO_ASSERT(port, "%s: listener port discovery failed", cases[i].name);
    char url[128];
    snprintf(url, sizeof(url), "tcp://127.0.0.1:%u", port);
    FIO_ASSERT(fio_io_connect(url,
                              .protocol = &raw_client,
                              .on_failed = test_http_close_client_on_failed),
               "%s: raw connect failed",
               cases[i].name);
    test_http_close.active = 1;
    fio_io_start(0);
    test_http_close.active = 0;
    fio_io_listen_stop((fio_io_listener_s *)l);
    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(!test_http_close.connect_failed,
               "%s: asynchronous connect failed",
               cases[i].name);
    FIO_ASSERT(
        !test_http_close.timed_out,
        "%s: timed out waiting for EOF (server %d, finish %d, bytes %zu)",
        cases[i].name,
        test_http_close.server_calls,
        test_http_close.finish_calls,
        test_http_close.received);
    FIO_ASSERT(test_http_close.client_closes == 1 && test_http_close.eof,
               "%s: raw peer must observe clean EOF exactly once",
               cases[i].name);
    FIO_ASSERT(test_http_close.server_calls == cases[i].expected_calls,
               "%s: expected %d request callbacks, got %d",
               cases[i].name,
               cases[i].expected_calls,
               test_http_close.server_calls);
    FIO_ASSERT(test_http_close.finish_calls == cases[i].expected_calls,
               "%s: expected %d user finish callbacks, got %d",
               cases[i].name,
               cases[i].expected_calls,
               test_http_close.finish_calls);
    FIO_ASSERT(!test_http_close.bad_path && !test_http_close.unfinished,
               "%s: request order / finished-state mismatch",
               cases[i].name);
    FIO_ASSERT(!test_http_close.close_still_open,
               "%s: application close must immediately clear public is_open",
               cases[i].name);
    FIO_ASSERT(!test_http_close.overflow &&
                   test_http_close_wire_ok((size_t)cases[i].expected_calls),
               "%s: incomplete / unexpected chunked response (%zu bytes)",
               cases[i].name,
               test_http_close.received);
  }
}

/* ===========================================================================
   Server: HTTP/1.x framing / persistence rules (RFC 9112)

   A raw client writes one buffer (possibly pipelined) and records the wire
   until the server closes. A connection the server keeps open times out.
   - §6.3/§7: Transfer-Encoding without a final (single) `chunked` -> 400 +
     close, never parsing the body as a pipelined request (smuggling).
   - §3.2: HTTP/1.1 requests without exactly one Host -> 400 + close.
   - §9.3: HTTP/1.0 is not persistent without `keep-alive`.
   - §6.1: no Transfer-Encoding for HTTP/1.0 (stream until close).
   ===========================================================================
 */

static struct {
  const char *request;
  fio_socket_i client_fd;
  uintptr_t generation;
  size_t received;
  int client_closes;
  int eof;
  int connect_failed;
  int timed_out;
  int active;
  int overflow;
  char paths[256];
  char te[64];
  char wire[98304];
} test_h1f;

static void test_h1f_server_on_http(fio_http_s *h) {
  fio_str_info_s path = fio_http_path(h);
  fio_str_info_s te = fio_http_request_header(
      h,
      FIO_STR_INFO2((char *)"transfer-encoding", 17),
      0);
  if (te.len && te.len < sizeof(test_h1f.te))
    FIO_MEMCPY(test_h1f.te, te.buf, te.len);
  if (strlen(test_h1f.paths) + path.len + 2 < sizeof(test_h1f.paths)) {
    strncat(test_h1f.paths, path.buf, path.len);
    strcat(test_h1f.paths, ";");
  }
  fio_http_status_set(h, 200);
  if (path.len == 4 && !FIO_MEMCMP(path.buf, "/big", 4)) {
    /* `/big?<pad>`: header padding moves the header buffer's spare capacity */
    static char big[70000];
    fio_str_info_s q = fio_http_query(h);
    char *pos = q.buf;
    size_t pad = q.len ? (size_t)fio_atol10u(&pos) : 0;
    char padding[64];
    FIO_MEMSET(big, 'B', sizeof(big));
    FIO_MEMSET(padding, 'p', sizeof(padding));
    if (pad && pad < sizeof(padding))
      fio_http_response_header_set(h,
                                   FIO_STR_INFO1((char *)"x-pad"),
                                   FIO_STR_INFO2(padding, pad));
    fio_http_write(h, .buf = big, .len = sizeof(big), .copy = 0);
    fio_http_write(h, .buf = "end", .len = 3, .copy = 1, .finish = 1);
    return;
  }
  if (path.len > 8 && !FIO_MEMCMP(path.buf, "/status/", 8)) {
    /* `/status/<code>?<mode>`: modes `app` (app framing headers), `stream` */
    char *pos = path.buf + 8;
    fio_str_info_s mode = fio_http_query(h);
    fio_http_status_set(h, (size_t)fio_atol10u(&pos));
    if (mode.len == 3 && !FIO_MEMCMP(mode.buf, "app", 3)) {
      fio_http_response_header_set(h,
                                   FIO_STR_INFO1((char *)"content-length"),
                                   FIO_STR_INFO1((char *)"7"));
      fio_http_response_header_set(h,
                                   FIO_STR_INFO1((char *)"transfer-encoding"),
                                   FIO_STR_INFO1((char *)"chunked"));
    }
    if (mode.len == 5 && !FIO_MEMCMP(mode.buf, "empty", 5)) {
      fio_http_write(h, .buf = "", .len = 0, .copy = 1);
      fio_http_write(h, .len = 0);
      fio_http_finish(h);
      return;
    }
    if (mode.len == 4 && !FIO_MEMCMP(mode.buf, "late", 4)) {
      /* an empty write doesn't start the response: headers stay open */
      fio_http_write(h, .len = 0);
      fio_http_response_header_set(h,
                                   FIO_STR_INFO1((char *)"x-late"),
                                   FIO_STR_INFO1((char *)"1"));
      fio_http_write(h, .buf = "xy", .len = 2, .copy = 1, .finish = 1);
      return;
    }
    if (mode.len == 11 && !FIO_MEMCMP(mode.buf, "emptystream", 11)) {
      fio_http_write(h, .len = 0);
      fio_http_write(h, .buf = "ab", .len = 2, .copy = 1);
      fio_http_write(h, .len = 0);
      fio_http_finish(h);
      return;
    }
    if (mode.len == 6 && !FIO_MEMCMP(mode.buf, "stream", 6)) {
      fio_http_write(h, .buf = "ab", .len = 2, .copy = 1);
      fio_http_write(h, .buf = "cd", .len = 2, .copy = 1, .finish = 1);
      return;
    }
    fio_http_write(h, .buf = "body", .len = 4, .copy = 1, .finish = 1);
    return;
  }
  if (path.len == 7 && !FIO_MEMCMP(path.buf, "/stream", 7)) {
    fio_http_write(h, .buf = "a", .len = 1, .copy = 1);
    fio_http_write(h, .buf = "b", .len = 1, .copy = 1);
    fio_http_write(h, .buf = "c", .len = 1, .copy = 1);
    fio_http_write(h, .buf = "d", .len = 1, .copy = 1, .finish = 1);
    return;
  }
  fio_http_write(h, .buf = "ok", .len = 2, .copy = 1, .finish = 1);
}

static void test_h1f_client_on_attach(fio_io_s *io) {
  test_h1f.client_fd = fio_io_fd(io);
  fio_io_write(io, (void *)test_h1f.request, strlen(test_h1f.request));
}

static void test_h1f_capture(const char *buf, size_t len) {
  if (len >= sizeof(test_h1f.wire) - test_h1f.received) {
    test_h1f.overflow = 1;
    return;
  }
  FIO_MEMCPY(test_h1f.wire + test_h1f.received, buf, len);
  test_h1f.received += len;
  test_h1f.wire[test_h1f.received] = 0;
}

static void test_h1f_client_on_data(fio_io_s *io) {
  char buf[4096];
  size_t len;
  while ((len = fio_io_read(io, buf, sizeof(buf))))
    test_h1f_capture(buf, len);
}

static void test_h1f_client_on_close(void *iobuf, void *udata) {
  (void)iobuf, (void)udata;
  char buf[4096];
  ssize_t len;
  ++test_h1f.client_closes;
  do {
    len = fio_sock_read(test_h1f.client_fd, buf, sizeof(buf));
    if (len > 0)
      test_h1f_capture(buf, (size_t)len);
  } while (len > 0);
  test_h1f.eof = (len == 0);
  if (test_h1f.active)
    fio_io_stop();
}

static void test_h1f_client_on_failed(fio_io_protocol_s *pr, void *udata) {
  (void)pr, (void)udata;
  test_h1f.connect_failed = 1;
  fio_io_stop();
}

static int test_h1f_timeout(void *generation, void *ignr) {
  (void)ignr;
  if (!test_h1f.active || (uintptr_t)generation != test_h1f.generation)
    return -1;
  test_h1f.timed_out = 1;
  fio_io_stop();
  return -1;
}

static size_t test_h1f_count(const char *needle) {
  size_t count = 0;
  for (const char *pos = test_h1f.wire; (pos = strstr(pos, needle)); ++pos)
    ++count;
  return count;
}

static int test_h1f_ends_with(const char *tail) {
  size_t len = strlen(tail);
  return test_h1f.received >= len &&
         !FIO_MEMCMP(test_h1f.wire + test_h1f.received - len, tail, len);
}

static void test_http_server_h1_framing(void) {
  fprintf(stderr, "  * server HTTP/1.x framing and persistence (RFC 9112)\n");
#define TEST_H1F_SMUGGLE(te)                                                   \
  "POST /front HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: " te "\r\n\r\n"       \
  "GET /smuggled HTTP/1.1\r\nHost: x\r\n\r\n"
/* a valid (empty) chunked body: only header validation can reject these */
#define TEST_H1F_SMUGGLE0(te)                                                  \
  "POST /front HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: " te "\r\n\r\n"       \
  "0\r\n\r\nGET /smuggled HTTP/1.1\r\nHost: x\r\n\r\n"
  static const struct {
    const char *name;
    const char *request;
    const char *status; /* status line prefix of the first response */
    const char *paths;  /* application dispatch log */
    size_t responses;
    const char *contains; /* optional wire substring */
    const char *excludes; /* optional forbidden wire substring */
    const char *tail;     /* optional wire suffix */
    const char *te;       /* request transfer-encoding seen by the app */
    const char *excludes2; /* optional forbidden wire substrings */
    const char *excludes3;
    const char *contains2; /* additional order-independent wire substring */
  } cases[] = {
      {"TE chunked, gzip",
       TEST_H1F_SMUGGLE("chunked, gzip"),
       "HTTP/1.1 400 ",
       "",
       1},
      {"TE gzip", TEST_H1F_SMUGGLE("gzip"), "HTTP/1.1 400 ", "", 1},
      {"TE identity", TEST_H1F_SMUGGLE("identity"), "HTTP/1.1 400 ", "", 1},
      {"TE chunked, chunked",
       TEST_H1F_SMUGGLE0("chunked, chunked"),
       "HTTP/1.1 400 ",
       "",
       1},
      {"TE chunked;ext, chunked",
       TEST_H1F_SMUGGLE0("Chunked;x=1 , chunked"),
       "HTTP/1.1 400 ",
       "",
       1},
      {"TE chunked then gzip field lines",
       TEST_H1F_SMUGGLE0("chunked\r\nTransfer-Encoding: gzip"),
       "HTTP/1.1 400 ",
       "",
       1},
      {"TE chunked twice in field lines",
       TEST_H1F_SMUGGLE0("chunked\r\nTransfer-Encoding: chunked"),
       "HTTP/1.1 400 ",
       "",
       1},
      {"TE empty", TEST_H1F_SMUGGLE(""), "HTTP/1.1 400 ", "", 1},
      {"TE gzip, chunked control (gzip passed through)",
       "POST /te HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: gzip, chunked\r\n"
       "Connection: close\r\n\r\n2\r\nhi\r\n0\r\n\r\n",
       "HTTP/1.1 200 ",
       "/te;",
       1,
       NULL,
       NULL,
       "\r\n\r\nok",
       "gzip"},
      {"HTTP/1.1 missing Host",
       "GET /nohost HTTP/1.1\r\n\r\nGET /next HTTP/1.1\r\nHost: x\r\n\r\n",
       "HTTP/1.1 400 ",
       "",
       1},
      {"HTTP/1.1 duplicate Host",
       "GET /dup HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n",
       "HTTP/1.1 400 ",
       "",
       1},
      {"HTTP/1.0 closes after response",
       "GET /plain HTTP/1.0\r\n\r\nGET /second HTTP/1.0\r\n\r\n",
       "HTTP/1.0 200 ",
       "/plain;",
       1,
       NULL,
       "connection:keep-alive",
       "\r\n\r\nok"},
      {"HTTP/1.0 stream is close delimited",
       "GET /stream HTTP/1.0\r\n\r\n",
       "HTTP/1.0 200 ",
       "/stream;",
       1,
       NULL,
       "transfer-encoding",
       "\r\n\r\nabcd"},
      {"HTTP/1.0 keep-alive persists",
       "GET /plain HTTP/1.0\r\nConnection: Keep-Alive\r\n\r\n"
       "GET /second HTTP/1.0\r\n\r\n",
       "HTTP/1.0 200 ",
       "/plain;/second;",
       2,
       "connection:keep-alive",
       NULL,
       "\r\n\r\nok"},
      {"HTTP/1.0 keep-alive stream closes",
       "GET /stream HTTP/1.0\r\nConnection: keep-alive\r\n\r\n"
       "GET /second HTTP/1.0\r\n\r\n",
       "HTTP/1.0 200 ",
       "/stream;",
       1,
       NULL,
       "transfer-encoding",
       "\r\n\r\nabcd"},
#define TEST_H1F_STATUS(code, mode)                                            \
  "GET /status/" code mode " HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n"
      /* RFC 9110 §8.6 / §15: responses without content are never framed */
      {"204 drops content-length (body ignored)",
       TEST_H1F_STATUS("204", ""),
       "HTTP/1.1 204 ",
       "/status/204;",
       1,
       NULL,
       "content-length",
       "\r\n\r\n",
       .excludes2 = "transfer-encoding",
       .excludes3 = "body"},
      {"204 strips app content-length / transfer-encoding",
       TEST_H1F_STATUS("204", "?app"),
       "HTTP/1.1 204 ",
       "/status/204;",
       1,
       NULL,
       "content-length",
       "\r\n\r\n",
       .excludes2 = "transfer-encoding"},
      {"204 stream is never chunked",
       TEST_H1F_STATUS("204", "?stream"),
       "HTTP/1.1 204 ",
       "/status/204;",
       1,
       NULL,
       "content-length",
       "\r\n\r\n",
       .excludes2 = "transfer-encoding",
       .excludes3 = "\r\n\r\n0"},
      {"1xx (103) drops content-length",
       TEST_H1F_STATUS("103", "?app"),
       "HTTP/1.1 103 ",
       "/status/103;",
       1,
       NULL,
       "content-length",
       "\r\n\r\n",
       .excludes2 = "transfer-encoding"},
      {"304 adds no automatic content-length",
       TEST_H1F_STATUS("304", ""),
       "HTTP/1.1 304 ",
       "/status/304;",
       1,
       NULL,
       "content-length",
       "\r\n\r\n",
       .excludes2 = "transfer-encoding",
       .excludes3 = "body"},
      {"304 keeps app content-length",
       TEST_H1F_STATUS("304", "?app"),
       "HTTP/1.1 304 ",
       "/status/304;",
       1,
       "content-length:7\r\n",
       NULL,
       "\r\n\r\n",
       .excludes2 = "transfer-encoding"},
      {"304 stream is never chunked",
       TEST_H1F_STATUS("304", "?stream"),
       "HTTP/1.1 304 ",
       "/status/304;",
       1,
       NULL,
       "content-length",
       "\r\n\r\n",
       .excludes2 = "transfer-encoding",
       .excludes3 = "\r\n\r\n0"},
      {"205 always declares zero length",
       TEST_H1F_STATUS("205", "?app"),
       "HTTP/1.1 205 ",
       "/status/205;",
       1,
       "content-length:0\r\n",
       "content-length:7",
       "\r\n\r\n",
       .excludes2 = "transfer-encoding"},
      {"205 body is never declared",
       TEST_H1F_STATUS("205", ""),
       "HTTP/1.1 205 ",
       "/status/205;",
       1,
       "content-length:0\r\n",
       "body",
       "\r\n\r\n"},
      {"HEAD response sends no body",
       "HEAD /plain HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",
       "HTTP/1.1 200 ",
       "/plain;",
       1,
       "content-length:2\r\n",
       "\r\n\r\nok",
       "\r\n\r\n"},
      {"HEAD streamed response sends no chunks",
       "HEAD /stream HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",
       "HTTP/1.1 200 ",
       "/stream;",
       1,
       NULL,
       "transfer-encoding",
       "\r\n\r\n",
       .excludes2 = "\r\n\r\n01",
       .excludes3 = "\r\n\r\n0\r\n"},
      {"HEAD large no-copy stream sends no chunks",
       "HEAD /big HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",
       "HTTP/1.1 200 ",
       "/big;",
       1,
       NULL,
       "BBBB",
       "\r\n\r\n",
       .excludes2 = "transfer-encoding"},
      {"200 empty stream is sent with content-length:0",
       TEST_H1F_STATUS("200", "?empty"),
       "HTTP/1.1 200 ",
       "/status/200;",
       1,
       "content-length:0\r\n",
       "transfer-encoding",
       "\r\n\r\n",
       .excludes2 = "\r\n\r\n0\r\n"},
      {"200 empty write keeps headers open (content-length)",
       TEST_H1F_STATUS("200", "?late"),
       "HTTP/1.1 200 ",
       "/status/200;",
       1,
       "x-late:1\r\n",
       "transfer-encoding",
       "\r\n\r\nxy",
       .excludes2 = "transfer-encoding: chunked",
       .excludes3 = "content-length:0",
       .contains2 = "content-length:2\r\n"},
      {"200 stream starts with body data (empty writes skipped)",
       TEST_H1F_STATUS("200", "?emptystream"),
       "HTTP/1.1 200 ",
       "/status/200;",
       1,
       "transfer-encoding: chunked\r\n\r\n02\r\nab\r\n0\r\n\r\n",
       "content-length",
       "\r\n\r\n02\r\nab\r\n0\r\n\r\n"},
#define TEST_H1F_BIG(pad)                                                      \
  {"200 large no-copy first chunk (pad " pad ")",                              \
   "GET /big?" pad " HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",        \
   "HTTP/1.1 200 ",                                                            \
   "/big;",                                                                    \
   1,                                                                          \
   "\r\n\r\n011170\r\nBBBB",                                                   \
   NULL,                                                                       \
   "BBBB\r\n03\r\nend\r\n0\r\n\r\n"}
      TEST_H1F_BIG("0"), TEST_H1F_BIG("1"), TEST_H1F_BIG("2"),
      TEST_H1F_BIG("3"), TEST_H1F_BIG("4"), TEST_H1F_BIG("5"),
      TEST_H1F_BIG("6"), TEST_H1F_BIG("7"), TEST_H1F_BIG("8"),
      TEST_H1F_BIG("9"), TEST_H1F_BIG("10"), TEST_H1F_BIG("11"),
      TEST_H1F_BIG("12"), TEST_H1F_BIG("13"), TEST_H1F_BIG("14"),
      TEST_H1F_BIG("15"), TEST_H1F_BIG("16"), TEST_H1F_BIG("17"),
#undef TEST_H1F_BIG
#undef TEST_H1F_STATUS
      {"HTTP/1.1 control (persistent, chunked stream)",
       "GET /plain HTTP/1.1\r\nHost: x\r\n\r\n"
       "GET /stream HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",
       "HTTP/1.1 200 ",
       "/plain;/stream;",
       2,
       "transfer-encoding: chunked",
       "connection:keep-alive",
       "0\r\n\r\n"},
  };
#undef TEST_H1F_SMUGGLE
#undef TEST_H1F_SMUGGLE0
  static fio_io_protocol_s raw_client;
  static uintptr_t generation;
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    fprintf(stderr, "    - %s\n", cases[i].name);
    raw_client = (fio_io_protocol_s){
        .on_attach = test_h1f_client_on_attach,
        .on_data = test_h1f_client_on_data,
        .on_close = test_h1f_client_on_close,
        .on_timeout = fio_io_touch,
        .timeout = 10000,
    };
    FIO_MEMSET(&test_h1f, 0, sizeof(test_h1f));
    test_h1f.client_fd = FIO_SOCKET_INVALID;
    test_h1f.generation = ++generation;
    test_h1f.request = cases[i].request;
    fio_io_run_every(.fn = test_h1f_timeout,
                     .udata1 = (void *)test_h1f.generation,
                     .every = 2000,
                     .repetitions = 1);
    fio_http_listener_s *l =
        fio_http_listen("tcp://127.0.0.1:0",
                        .on_http = test_h1f_server_on_http,
                        .log = 0);
    FIO_ASSERT(l, "%s: listener failed", cases[i].name);
    unsigned port = test_ws_listener_port(l);
    FIO_ASSERT(port, "%s: listener port discovery failed", cases[i].name);
    char url[128];
    snprintf(url, sizeof(url), "tcp://127.0.0.1:%u", port);
    FIO_ASSERT(fio_io_connect(url,
                              .protocol = &raw_client,
                              .on_failed = test_h1f_client_on_failed),
               "%s: raw connect failed",
               cases[i].name);
    test_h1f.active = 1;
    fio_io_start(0);
    test_h1f.active = 0;
    fio_io_listen_stop((fio_io_listener_s *)l);
    fio_queue_perform_all(fio_io_queue());

    FIO_ASSERT(!test_h1f.connect_failed && !test_h1f.overflow,
               "%s: connect / capture failure",
               cases[i].name);
    FIO_ASSERT(!test_h1f.timed_out,
               "%s: server kept the connection open (paths \"%s\"):\n%s",
               cases[i].name,
               test_h1f.paths,
               test_h1f.wire);
    FIO_ASSERT(test_h1f.client_closes == 1 && test_h1f.eof,
               "%s: raw peer must observe clean EOF exactly once",
               cases[i].name);
    FIO_ASSERT(!strcmp(test_h1f.paths, cases[i].paths),
               "%s: dispatched \"%s\", expected \"%s\"",
               cases[i].name,
               test_h1f.paths,
               cases[i].paths);
    FIO_ASSERT(!strncmp(test_h1f.wire,
                        cases[i].status,
                        strlen(cases[i].status)) &&
                   test_h1f_count("HTTP/1.") == cases[i].responses,
               "%s: expected %zu response(s) starting \"%s\":\n%s",
               cases[i].name,
               cases[i].responses,
               cases[i].status,
               test_h1f.wire);
    FIO_ASSERT(!cases[i].contains || strstr(test_h1f.wire, cases[i].contains),
               "%s: missing \"%s\":\n%s",
               cases[i].name,
               cases[i].contains,
               test_h1f.wire);
    FIO_ASSERT(!cases[i].contains2 || strstr(test_h1f.wire, cases[i].contains2),
               "%s: missing \"%s\":\n%s",
               cases[i].name,
               cases[i].contains2,
               test_h1f.wire);
    FIO_ASSERT(!cases[i].excludes || !strstr(test_h1f.wire, cases[i].excludes),
               "%s: unexpected \"%s\":\n%s",
               cases[i].name,
               cases[i].excludes,
               test_h1f.wire);
    FIO_ASSERT((!cases[i].excludes2 ||
                !strstr(test_h1f.wire, cases[i].excludes2)) &&
                   (!cases[i].excludes3 ||
                    !strstr(test_h1f.wire, cases[i].excludes3)),
               "%s: unexpected \"%s\" / \"%s\":\n%s",
               cases[i].name,
               cases[i].excludes2 ? cases[i].excludes2 : "",
               cases[i].excludes3 ? cases[i].excludes3 : "",
               test_h1f.wire);
    FIO_ASSERT(!cases[i].tail || test_h1f_ends_with(cases[i].tail),
               "%s: wire must end with the body:\n%s",
               cases[i].name,
               test_h1f.wire);
    FIO_ASSERT(!cases[i].te || !strcmp(test_h1f.te, cases[i].te),
               "%s: app saw transfer-encoding \"%s\", expected \"%s\"",
               cases[i].name,
               test_h1f.te,
               cases[i].te);
  }
}

/* ===========================================================================
   Main
   ===========================================================================
 */

static void test_header_iteration(void) {
  fprintf(stderr, "  * zero-copy header value/property iteration\n");
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(h, "HTTP allocation failed");
  fio_str_info_s name = FIO_STR_INFO2((char *)"x-iterate", 9);
  fio_http_request_header_add(
      h,
      name,
      FIO_STR_INFO1((char *)"  , a  ; p = \"x\\\";y,z\" ; empty=  ,  "
                           "b\t"));
  fio_http_request_header_add(h, name, FIO_STR_INFO1((char *)"c"));
  const char *expected[] = {"a", "b", "c"};
  size_t count = 0;
  FIO_HTTP_HEADER_EACH_VALUE(h, 1, name, item) {
    FIO_ASSERT(count < 3 && item.value.len == strlen(expected[count]) &&
                   !memcmp(item.value.buf, expected[count], item.value.len),
               "header value iteration mismatch");
    if (!count) {
      size_t property_count = 0;
      FIO_HTTP_HEADER_EACH_PROPERTY(item, property) {
        if (!property_count)
          FIO_ASSERT(property.name.len == 1 && property.name.buf[0] == 'p' &&
                         property.value.len == 9 &&
                         !memcmp(property.value.buf, "\"x\\\";y,z\"", 9),
                     "quoted property mismatch");
        else
          FIO_ASSERT(property.name.len == 5 &&
                         !memcmp(property.name.buf, "empty", 5) &&
                         property.value.buf && !property.value.len,
                     "empty property value mismatch");
        ++property_count;
      }
      FIO_ASSERT(property_count == 2, "property count mismatch");
    }
    ++count;
  }
  FIO_ASSERT(count == 3, "repeated header field count mismatch");
  fio_http_response_header_add(h, name, FIO_STR_INFO1((char *)"reply"));
  count = 0;
  FIO_HTTP_HEADER_EACH_VALUE(h, 0, name, item) {
    FIO_ASSERT(item.value.len == 5 &&
                   !memcmp(item.value.buf, "reply", 5),
               "response header iteration mismatch");
    ++count;
  }
  FIO_ASSERT(count == 1, "response header count mismatch");
  fio_http_free(h);
}

int main(void) {
#if defined(_WIN32)
  /* Permanent diagnostic net: make any future Windows CI hard crash
     self-locating in the log (exception code + symbolizable module RVAs). */
  SetUnhandledExceptionFilter(test_http_win_crash_trap);
#endif
  fprintf(stderr, "Testing fio_http high-level behavior:\n");

  test_resource_action();
  test_listen_and_route();
  test_settings_and_io_queries();
  test_static_file_response();
  test_error_response();
  test_websocket_upgrade_helpers();
  test_sse_upgrade_helpers();
  test_sse_newline_first_edge_case();
  test_static_vary_and_range_guards();
  test_static_head_mirrors_get();
  test_websocket_deflate_negotiation();
  test_header_iteration();
  test_websocket_connect_wrapper();
  test_static_compress_note_result();
  test_static_compress_attached_readonly();
  test_static_compress_detached_creation();
  test_http_client_server_roundtrip();
  test_http_client_interim_and_head();
  test_http_server_rejects_response();
  test_http_server_close_pipeline();
  test_http_server_h1_framing();

  fprintf(stderr, "\nAll high-level HTTP tests passed!\n");
  return 0;
}
