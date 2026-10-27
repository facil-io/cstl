/* *****************************************************************************
Test: HTTP module cleanup unit (fio-stl/439 http.h) — inclusion contract

Regression net for the security review of `439 http.h` (report 016):
- every private HTTP state / compression-flag macro and `FIO_HTTP` itself are
  evicted after the outer (non-recursive) HTTP pass;
- a later, separate FIOBJ pass still compiles the HTTP <=> FIOBJ bridge (the
  bridge keys on header guards, not on the already-consumed `FIO_HTTP`), i.e.
  the HTTP-first order complementing tests/http-fiobj.c (FIOBJ-first);
- a later template pass (which re-enters the include master) is unaffected
  and re-leaks nothing.
***************************************************************************** */
#define FIO_HTTP
#include "test-helpers.h"

/* All 11 private macros owned by the cleanup unit, plus FIO_HTTP. */
#if defined(FIO_HTTP) || defined(FIO___HTTP_STATE_STREAMING) ||                \
    defined(FIO___HTTP_STATE_FINISHED) || defined(FIO___HTTP_STATE_UPGRADED) || \
    defined(FIO___HTTP_STATE_WEBSOCKET) || defined(FIO___HTTP_STATE_SSE) ||    \
    defined(FIO___HTTP_STATE_COOKIES_PARSED) ||                                \
    defined(FIO___HTTP_STATE_FREEING) ||                                       \
    defined(FIO___HTTP_STATE_NO_CONTENT) ||                                    \
    defined(FIO___HTTP_CFLAG_COMPRESS_DYNAMIC) ||                              \
    defined(FIO___HTTP_CFLAG_COMPRESS_WS) ||                                   \
    defined(FIO___HTTP_CFLAG_COMPRESS_STATIC)
#error HTTP cleanup unit leaked private state after the HTTP pass
#endif

#if !defined(H___FIO_HTTP_API___H) || defined(H___FIO_HTTP_FIOBJ___H)
#error HTTP pass must define the HTTP API and not yet the FIOBJ bridge
#endif

/* HTTP-first order: the bridge must appear once FIOBJ is added later. */
#define FIO_FIOBJ
#include FIO_INCLUDE_FILE

#if !defined(H___FIO_HTTP_FIOBJ___H)
#error HTTP <=> FIOBJ bridge missing in HTTP-first inclusion order
#endif

/* A later template pass must neither break nor resurrect HTTP state. */
#define FIO_STR_NAME test_http_cleanup_str
#include FIO_INCLUDE_FILE

#if defined(FIO_HTTP) || defined(FIO___HTTP_STATE_STREAMING) ||                \
    defined(FIO___HTTP_STATE_NO_CONTENT) ||                                    \
    defined(FIO___HTTP_CFLAG_COMPRESS_STATIC)
#error HTTP private state resurrected by a later inclusion pass
#endif

#include <string.h>

int main(void) {
  /* Bridge from the HTTP-first order works at runtime. */
  fio_http_s *h = fio_http_new();
  FIO_ASSERT(h, "fio_http_new failed");
  fio_http_query_set(h, FIO_STR_INFO1((char *)"a=1&b=x"));
  FIOBJ o = fio_http_params2FIOBJ(FIOBJ_INVALID, h);
  fio_http_free(h);
  FIO_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_HASH, "params must parse to a Hash");
  FIOBJ json = fiobj2json(FIOBJ_INVALID, o, 0);
  fio_str_info_s s = fiobj2cstr(json);
  static const char expected[] = "{\"a\":\"1\",\"b\":\"x\"}";
  FIO_ASSERT(s.len == sizeof(expected) - 1 && !memcmp(s.buf, expected, s.len),
             "bridge JSON mismatch: %.*s",
             (int)s.len,
             s.buf);
  fiobj_free(json);
  fiobj_free(o);

  /* Template instantiated after the HTTP pass is usable. */
  test_http_cleanup_str_s str = FIO_STR_INIT;
  test_http_cleanup_str_write(&str, "ok", 2);
  FIO_ASSERT(test_http_cleanup_str_len(&str) == 2, "template pass broken");
  test_http_cleanup_str_destroy(&str);

  fprintf(stderr, "* HTTP cleanup-unit inclusion contract: passed.\n");
  return 0;
}
