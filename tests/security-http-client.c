/* *****************************************************************************
Test: HTTP glue security regressions (438 http.h, report 015)

Reactor-less: `fio_io_connect` opens the client socket and defers attachment,
so the outgoing handle is fully prepared when the call returns. Tests hold an
extra handle reference to inspect it; teardown closes the IO and drains the
IO queue (the reactor-less close path).

- HTTPG-01: a client public folder longer than the URL host (and a NULL URL)
  fits its protocol suffix, NUL terminated (listener too).
- HTTPG-02: `fio_http_settings` on a client handle without `opath`.
- HTTPG-03: default Host / Origin carry the URL authority (port, IPv6
  brackets, no user info); caller headers are preserved.
- HTTPG-05: listen URL rejected before IO ownership vs. a later socket
  failure: the protocol is freed exactly once.
- client on_finish runs exactly once (finished via the public API, or not).
- HTTPG-07: long WebSocket URL rewrites are never truncated; `://` in a
  query isn't a scheme.

All checks use FIO_ASSERT (active under NDEBUG).
***************************************************************************** */

#define FIO_HTTP
#include "test-helpers.h"

#if defined(_WIN32)
int main(void) {
  fprintf(stderr, "\t* HTTP client security test skipped (POSIX sockets)\n");
  return 0;
}
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static unsigned port;
static size_t finish_calls;

static void client_on_http(fio_http_s *h) { (void)h; }
static void client_on_finish(fio_http_s *h) {
  ++finish_calls;
  (void)h;
}

static unsigned listener_port(fio_http_listener_s *l) {
  fio___io_listen_s *li = (fio___io_listen_s *)l;
  struct sockaddr_in ss;
  socklen_t slen = (socklen_t)sizeof(ss);
  FIO_ASSERT(!getsockname(li->fd, (struct sockaddr *)&ss, &slen),
             "getsockname failed");
  return (unsigned)ntohs(ss.sin_port);
}

static void drain(void) { fio_queue_perform_all(fio_io_queue()); }

/** Connects `url`, returning `h` with an extra (test) reference. */
static fio_http_s *hold_connect(const char *url,
                                fio_http_s *h,
                                fio_io_s **io,
                                fio_http_settings_s s,
                                int ws) {
  if (!h)
    h = fio_http_new();
  FIO_ASSERT_ALLOC(h);
  fio_http_dup(h);
  if (!s.on_http)
    s.on_http = client_on_http;
  *io = ws ? fio_http_websocket_connect FIO_NOOP(url, h, s)
           : fio_http_connect FIO_NOOP(url, h, s);
  return h;
}

/** Closes the IO, drains the close path and drops the test reference. */
static void release(fio_io_s *io, fio_http_s *h) {
  if (io)
    fio_io_close(io);
  drain();
  fio_http_free(h);
  drain();
}

static void assert_header(fio_http_s *h,
                          const char *name,
                          const char *expected,
                          const char *url) {
  fio_str_info_s v =
      fio_http_request_header(h, FIO_STR_INFO2((char *)name, strlen(name)), 0);
  if (!expected) {
    FIO_ASSERT(!v.len, "%s: unexpected %s: %.*s", url, name, (int)v.len, v.buf);
    return;
  }
  FIO_ASSERT(v.len == strlen(expected) && !FIO_MEMCMP(v.buf, expected, v.len),
             "%s: %s should be `%s`, got `%.*s`",
             url,
             name,
             expected,
             (int)v.len,
             v.buf);
}

/* HTTPG-03 + HTTPG-02 */
static void test_authority(void) {
  static const struct {
    const char *url;    /* `%u` = port (or a literal port) */
    const char *host;   /* expected Host (`%u` = port) */
    const char *origin; /* expected Origin (`%u` = port), NULL = none */
  } cases[] = {
      {"http://127.0.0.1:%u/", "127.0.0.1:%u", NULL},
      {"http://127.0.0.1/", "127.0.0.1", NULL},
      {"ws://127.0.0.1:%u/x", "127.0.0.1:%u", "http://127.0.0.1:%u"},
      {"sse://127.0.0.1:%u/x", "127.0.0.1:%u", "http://127.0.0.1:%u"},
      {"ws://user:pw@127.0.0.1:%u/", "127.0.0.1:%u", "http://127.0.0.1:%u"},
      {"ws://[::1]:%u/", "[::1]:%u", "http://[::1]:%u"},
      {"ws://[::1]/", "[::1]", "http://[::1]"},
      {"ws://localhost:%u", "localhost:%u", "http://localhost:%u"},
  };
  char url[128], host[128], origin[128];
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    snprintf(url, sizeof(url), cases[i].url, port);
    snprintf(host, sizeof(host), cases[i].host, port);
    if (cases[i].origin)
      snprintf(origin, sizeof(origin), cases[i].origin, port);
    fio_io_s *io;
    fio_http_s *h = hold_connect(url, NULL, &io, (fio_http_settings_s){0}, 0);
    assert_header(h, "host", host, url);
    assert_header(h, "origin", cases[i].origin ? origin : NULL, url);
    /* HTTPG-02: the client handle has no `opath`; settings resolve to root */
    FIO_ASSERT(!fio_http_opath(h).len, "%s: client opath precondition", url);
    fio_http_settings_s *st = fio_http_settings(h);
    FIO_ASSERT(st && st->on_http == client_on_http,
               "%s: fio_http_settings must return the client settings",
               url);
    release(io, h);
  }
  /* caller supplied Host / Origin are preserved */
  snprintf(url, sizeof(url), "ws://127.0.0.1:%u/", port);
  fio_http_s *h = fio_http_new();
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"host", 4),
                              FIO_STR_INFO2((char *)"vhost.test", 10));
  fio_http_request_header_set(h,
                              FIO_STR_INFO2((char *)"origin", 6),
                              FIO_STR_INFO2((char *)"https://o.test", 14));
  fio_io_s *io;
  h = hold_connect(url, h, &io, (fio_http_settings_s){0}, 0);
  assert_header(h, "host", "vhost.test", url);
  assert_header(h, "origin", "https://o.test", url);
  FIO_ASSERT(fio_http_websocket_requested(h), "%s: WS request missing", url);
  release(io, h);
}

/* HTTPG-01 */
static void test_public_folder(void) {
  /* a real folder, much longer than the "127.0.0.1" host */
  static const char folder[] = "tests/../tests/../tests/../tests/../tests";
  const size_t len = sizeof(folder) - 1;
  char url[128];
  snprintf(url, sizeof(url), "http://127.0.0.1:%u/", port);
  fio_io_s *io;
  fio_http_s *h = hold_connect(
      url,
      NULL,
      &io,
      (fio_http_settings_s){.public_folder = FIO_STR_INFO2((char *)folder, len)},
      0);
  fio_http_settings_s *st = fio_http_settings(h);
  FIO_ASSERT(st && st->public_folder.len == len &&
                 !FIO_MEMCMP(st->public_folder.buf, folder, len) &&
                 !st->public_folder.buf[len],
             "client public folder copy mismatch");
  release(io, h);
  /* NULL URL (zero length host): fails asynchronously, frees `h` later */
  FIO_ASSERT(!fio_http_connect(NULL,
                               NULL,
                               .on_http = client_on_http,
                               .public_folder =
                                   FIO_STR_INFO2((char *)folder, len)),
             "NULL URL connect should fail");
  drain();
  /* listener copy is NUL terminated as well */
  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0",
                      .on_http = client_on_http,
                      .public_folder = FIO_STR_INFO2((char *)folder, len));
  FIO_ASSERT(l, "folder listener failed");
  st = fio_http_listener_settings(l);
  FIO_ASSERT(st->public_folder.len == len &&
                 !FIO_MEMCMP(st->public_folder.buf, folder, len) &&
                 !st->public_folder.buf[len],
             "listener public folder copy mismatch");
  fio_io_listen_stop((fio_io_listener_s *)l);
  drain();
}

/* live allocation accounting: raw blocks, strings, handles, connections,
 * protocols (the leak counters exist when FIO_LEAK_COUNTER is set) */
typedef struct {
  size_t s, k, h, c, p;
} live_counts_s;
static live_counts_s live_counts(void) {
  live_counts_s r = {0};
#if FIO_LEAK_COUNTER
  r.s = FIO_LEAK_COUNTER_COUNT(fio_string_default_allocations);
  r.k = FIO_LEAK_COUNTER_COUNT(fio___http_keystr_allocator);
  r.h = FIO_LEAK_COUNTER_COUNT(fio_http);
  r.c = FIO_LEAK_COUNTER_COUNT(fio___http_connection);
  r.p = FIO_LEAK_COUNTER_COUNT(fio___http_protocol);
#endif
  return r;
}

/* HTTPG-07 */
static void test_long_urls(void) {
  const size_t plen = 6000;
  char *url = (char *)malloc(plen + 128);
  FIO_ASSERT_ALLOC(url);
  const char *prefix[] = {"http://127.0.0.1:%u/", "127.0.0.1:%u/"};
  for (size_t i = 0; i < 2; ++i) {
    int n = snprintf(url, 64, prefix[i], port);
    FIO_ASSERT(n > 0, "snprintf");
    memset(url + n, 'a', plen);
    memcpy(url + n + plen, "?q=1", 5);
    fio_io_s *io;
    fio_http_s *h = hold_connect(url, NULL, &io, (fio_http_settings_s){0}, 1);
    fio_str_info_s path = fio_http_path(h);
    fio_str_info_s query = fio_http_query(h);
    FIO_ASSERT(path.len == plen + 1 && path.buf[0] == '/' &&
                   path.buf[plen] == 'a',
               "long WS URL path truncated (%zu)",
               path.len);
    FIO_ASSERT(query.len == 3 && !FIO_MEMCMP(query.buf, "q=1", 3),
               "long WS URL query lost");
    FIO_ASSERT(fio_http_websocket_requested(h), "long WS URL: no upgrade");
    release(io, h);
  }
  free(url);
  /* a `://` inside the query of a scheme-less URL is not a scheme */
  {
    char u2[128];
    snprintf(u2, sizeof(u2), "127.0.0.1:%u/p?next=http://x", port);
    fio_io_s *io;
    fio_http_s *h = hold_connect(u2, NULL, &io, (fio_http_settings_s){0}, 1);
    FIO_ASSERT(fio_http_websocket_requested(h),
               "%s: scheme-less URL must become ws://",
               u2);
    fio_str_info_s query = fio_http_query(h);
    FIO_ASSERT(query.len == 13 && !FIO_MEMCMP(query.buf, "next=http://x", 13),
               "%s: query mismatch",
               u2);
    release(io, h);
  }
}

/* client on_finish runs exactly once when user code finishes the response
 * handle (reachable from a client `on_http`, see the report artifact) */
static void test_client_finish_once(void) {
  char url[128];
  snprintf(url, sizeof(url), "http://127.0.0.1:%u/", port);
  fio_io_s *io;
  finish_calls = 0;
  fio_http_s *h = hold_connect(url,
                               NULL,
                               &io,
                               (fio_http_settings_s){.on_finish =
                                                         client_on_finish},
                               0);
  fio_http_finish(h); /* public API: client controller's on_finish */
  release(io, h);    /* last reference: client destruction */
  FIO_ASSERT(finish_calls == 1,
             "client on_finish must run exactly once (%zu)",
             finish_calls);
  /* never finished: destruction calls on_finish (once) */
  finish_calls = 0;
  h = hold_connect(url,
                   NULL,
                   &io,
                   (fio_http_settings_s){.on_finish = client_on_finish},
                   0);
  release(io, h);
  FIO_ASSERT(finish_calls == 1,
             "unfinished client on_finish must run once (%zu)",
             finish_calls);
}

/* HTTPG-05 */
static void test_listen_failures(void) {
  char *url = (char *)malloc(2201);
  FIO_ASSERT_ALLOC(url);
  memcpy(url, "tcp://127.0.0.1:0/", 18);
  memset(url + 18, 'p', 2200 - 18);
  url[2200] = 0;
  FIO_ASSERT(!fio_http_listen(url, .on_http = client_on_http),
             "over-long listen URL must be rejected");
  free(url);
  /* a later socket failure (bind to a TEST-NET-3 address, not local): the IO
   * layer owned the protocol (on_stop) - freed exactly once, no leak */
  {
    live_counts_s was = live_counts();
    FIO_ASSERT(!fio_http_listen("tcp://203.0.113.1:0",
                                .on_http = client_on_http),
               "bind to a non-local address must fail");
    drain();
    FIO_ASSERT(live_counts().h == was.h && live_counts().c == was.c &&
                   live_counts().p == was.p, "failed listen leaked");
  }
  char busy[64];
  snprintf(busy, sizeof(busy), "tcp://127.0.0.1:%u", port);
  fio_http_listener_s *l = fio_http_listen(busy, .on_http = client_on_http);
  if (l) /* the platform allowed a shared port: stop it normally */
    fio_io_listen_stop((fio_io_listener_s *)l);
  drain();
}

int main(void) {
#if FIO_LEAK_COUNTER
  const size_t base_h = FIO_LEAK_COUNTER_COUNT(fio_http);
  const size_t base_c = FIO_LEAK_COUNTER_COUNT(fio___http_connection);
  const size_t base_p = FIO_LEAK_COUNTER_COUNT(fio___http_protocol);
#endif
  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0", .on_http = client_on_http);
  FIO_ASSERT(l, "fio_http_listen failed");
  port = listener_port(l);
  fprintf(stderr, "\t* HTTPG-03/02 Host / Origin authority, client settings\n");
  test_authority();
  fprintf(stderr, "\t* HTTPG-01 client public folder sizing\n");
  test_public_folder();
  fprintf(stderr, "\t* HTTPG-07 long WebSocket URL rewrites\n");
  test_long_urls();
  fprintf(stderr, "\t* client on_finish exactly once\n");
  test_client_finish_once();
  fprintf(stderr, "\t* HTTPG-05 listen failure ownership\n");
  test_listen_failures();
  fio_io_listen_stop((fio_io_listener_s *)l);
  drain();
#if FIO_LEAK_COUNTER
  FIO_ASSERT(FIO_LEAK_COUNTER_COUNT(fio_http) == base_h,
             "fio_http leaked (%zu)",
             FIO_LEAK_COUNTER_COUNT(fio_http) - base_h);
  FIO_ASSERT(FIO_LEAK_COUNTER_COUNT(fio___http_connection) == base_c,
             "HTTP connection leaked (%zu)",
             FIO_LEAK_COUNTER_COUNT(fio___http_connection) - base_c);
  FIO_ASSERT(FIO_LEAK_COUNTER_COUNT(fio___http_protocol) == base_p,
             "HTTP protocol leaked (%zu)",
             FIO_LEAK_COUNTER_COUNT(fio___http_protocol) - base_p);
#endif
  fprintf(stderr, "\t* HTTP client security checks passed\n");
  return 0;
}
#endif /* _WIN32 */
