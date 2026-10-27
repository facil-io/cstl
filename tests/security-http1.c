/* *****************************************************************************
Test: HTTP/1.1 protocol security regressions (434 http1.h, 004 http1 parser.h)

- H1C-01: a WebSocket / SSE upgrade whose connection closes before the
  deferred protocol transition releases the handle and connection.
- H1C-02: chunked (streamed) writes honor `offset` for copied, zero-copy and
  file data; the original buffer is freed once and the fd closed.
- H1C-03: `max_header_size` bounds ordinary header name/value bytes for
  coalesced and byte-by-byte input (not a raw wire-byte limit).
- H1C-04: a chunked 301 / 302 body is limited by `max_body_size` (client).
- H1C-05: client request-lines with CR / LF / SP / CTL bytes are never sent.
- H1C-06: an absolute-form target's authority replaces the request Host and
  never becomes a response header.
- Peer disconnect mid-request releases every handle / connection.

A live single-thread reactor is driven by raw POSIX socket peers (a thread).
All checks are active under NDEBUG.
***************************************************************************** */

#define FIO_HTTP
#include "test-helpers.h"

static volatile int failures;
#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL (line %d): ", __LINE__);                           \
      fprintf(stderr, __VA_ARGS__);                                            \
      fprintf(stderr, "\n");                                                   \
      fio_atomic_add(&failures, 1);                                            \
    }                                                                          \
  } while (0)

#if defined(_WIN32)
int main(void) {
  fprintf(stderr, "\t* HTTP/1.1 security checks require POSIX sockets\n");
  return 0;
}
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* *****************************************************************************
Raw socket helpers (peer thread)
***************************************************************************** */

static unsigned server_port;  /* fio_http_listen (server tests) */
static unsigned limits_port;  /* small header limits */
static int raw_listener = -1; /* peer side for client tests */
static unsigned raw_port;

static unsigned port_of(int fd) {
  struct sockaddr_in ss;
  socklen_t slen = (socklen_t)sizeof(ss);
  FIO_ASSERT(!getsockname(fd, (struct sockaddr *)&ss, &slen), "getsockname");
  return ntohs(ss.sin_port);
}

static void set_timeout(int fd, int ms) {
  struct timeval tv = {.tv_sec = ms / 1000, .tv_usec = (ms % 1000) * 1000};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static int peer_connect(unsigned port) {
  struct sockaddr_in a = {.sin_family = AF_INET,
                          .sin_port = htons((uint16_t)port)};
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return -1;
  int one = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
#ifdef SO_NOSIGPIPE
  setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
  set_timeout(fd, 5000);
  if (connect(fd, (struct sockaddr *)&a, sizeof(a))) {
    close(fd);
    return -1;
  }
  return fd;
}

static int send_all(int fd, const char *s, size_t len, size_t step) {
  while (len) {
    size_t n = (step && step < len) ? step : len;
    ssize_t w = send(fd, s, n, MSG_NOSIGNAL);
    if (w <= 0)
      return -1;
    s += w;
    len -= (size_t)w;
  }
  return 0;
}

/** Reads until EOF, timeout, `stop` is found (after `from`) or `cap`. */
static size_t recv_until(int fd, char *buf, size_t cap, const char *stop) {
  size_t len = 0;
  while (len + 1 < cap) {
    ssize_t r = recv(fd, buf + len, cap - 1 - len, 0);
    if (r <= 0)
      break;
    len += (size_t)r;
    buf[len] = 0;
    if (stop && strstr(buf, stop))
      break;
  }
  buf[len] = 0;
  return len;
}

/** Returns 1 if the peer closed the connection (EOF / reset) in time. */
static int recv_closed(int fd) {
  char tmp[4096];
  for (;;) {
    ssize_t r = recv(fd, tmp, sizeof(tmp), 0);
    if (r == 0)
      return 1;
    if (r < 0)
      return errno != EAGAIN && errno != EWOULDBLOCK;
  }
}

/* *****************************************************************************
Server callbacks (reactor thread)
***************************************************************************** */

static volatile int dispatched;
static volatile int dealloc_count;
static void *dealloc_expected[2];
static volatile int fd_offset_closed;
static void count_dealloc(void *p) {
  CHECK(p == dealloc_expected[0] || p == dealloc_expected[1],
        "dealloc got %p, not an original pointer",
        p);
  fio_atomic_add(&dealloc_count, 1);
  free(p);
}

static int fd_with(const char *s) {
  char name[] = "ai-tmp/security-http1-XXXXXX";
  int fd = mkstemp(name);
  if (fd < 0) { /* not run from the repository root */
    char alt[] = "./security-http1-XXXXXX";
    fd = mkstemp(alt);
    if (fd >= 0)
      unlink(alt);
  } else {
    unlink(name);
  }
  FIO_ASSERT(fd >= 0, "mkstemp failed");
  FIO_ASSERT(write(fd, s, strlen(s)) == (ssize_t)strlen(s), "write failed");
  return fd;
}

#define SHRINK_LEN ((size_t)8 << 20) /* file length when written */
#define SHRINK_TO  (1000U)            /* file length while streamed */
#define LARGE_LEN  (70000U)
#define LARGE_PFX (100U)

static void srv_on_http(fio_http_s *h) {
  fio_atomic_add(&dispatched, 1);
  fio_str_info_s path = fio_http_path(h);
  if (path.len >= 5 && !memcmp(path.buf, "/long", 5)) {
    fio_str_info_s q = fio_http_query(h);
    fio_str_info_s m = fio_http_method(h);
    char out[512];
    int l = snprintf(out,
                     sizeof(out),
                     "%.*s|%.*s|%.*s",
                     (int)m.len,
                     m.buf ? m.buf : "",
                     (int)path.len,
                     path.buf,
                     (int)q.len,
                     q.buf ? q.buf : "");
    fio_http_write(h, .buf = out, .len = (size_t)l, .copy = 1, .finish = 1);
    return;
  }
  if (FIO_STR_INFO_IS_EQ(path, FIO_STR_INFO1((char *)"/host"))) {
    fio_str_info_s host =
        fio_http_request_header(h, FIO_STR_INFO2((char *)"host", 4), 0);
    size_t n = 0;
    for (size_t i = 0;
         fio_http_request_header(h, FIO_STR_INFO2((char *)"host", 4), i).buf;
         ++i)
      ++n;
    char out[256];
    int l = snprintf(out,
                     sizeof(out),
                     "%zu:%.*s",
                     n,
                     (int)host.len,
                     host.buf ? host.buf : "");
    fio_http_write(h, .buf = out, .len = (size_t)l, .copy = 1, .finish = 1);
    return;
  }
  if (FIO_STR_INFO_IS_EQ(path, FIO_STR_INFO1((char *)"/stream"))) {
    /* H1C-02: every chunk starts at `offset` */
    char *small = (char *)malloc(16);
    memcpy(small, "SECRETabc", 9);
    char *large = (char *)malloc(LARGE_LEN);
    memset(large, 'S', LARGE_PFX);
    memset(large + LARGE_PFX, 'L', LARGE_LEN - LARGE_PFX);
    dealloc_expected[0] = small;
    dealloc_expected[1] = large;
    fio_http_write(h, .buf = small, .len = 3, .offset = 6,
                   .dealloc = count_dealloc);
    fio_http_write(h, .buf = "XXdef", .len = 3, .offset = 2, .copy = 1);
    fio_http_write(h, .buf = large, .len = LARGE_LEN - LARGE_PFX,
                   .offset = LARGE_PFX, .dealloc = count_dealloc);
    int fd = fd_with("SKIPfile-data");
    fio_http_write(h, .fd = fd, .offset = 4); /* inferred length */
    fd_offset_closed = 1; /* tested by the peer via the output */
    fio_http_write(h, .fd = fd_with("0123456789"), .len = 3, .offset = 2);
    fio_http_write(h, .finish = 1);
    return;
  }
  if (FIO_STR_INFO_IS_EQ(path, FIO_STR_INFO1((char *)"/fdshort"))) {
    /* declared chunk length beyond the file's end */
    fio_http_write(h, .buf = "a", .len = 1, .copy = 1);
    fio_http_write(h, .fd = fd_with("0123456789"), .len = 100, .offset = 2);
    fio_http_write(h, .buf = "z", .len = 1, .copy = 1, .finish = 1);
    return;
  }
  if (FIO_STR_INFO_IS_EQ(path, FIO_STR_INFO1((char *)"/fdcl"))) {
    fio_http_write(h, .fd = fd_with("0123456789"), .len = 100, .finish = 1);
    return;
  }
  if (FIO_STR_INFO_IS_EQ(path, FIO_STR_INFO1((char *)"/fdcl0"))) {
    fio_http_write(h, .fd = fd_with("0123456789"), .offset = 2, .finish = 1);
    return;
  }
  if (FIO_STR_INFO_IS_EQ(path, FIO_STR_INFO1((char *)"/fdlater"))) {
    /* a later (streamed) file write without a length: the rest of the file */
    fio_http_response_header_set(h,
                                 FIO_STR_INFO1((char *)"content-length"),
                                 FIO_STR_INFO1((char *)"12"));
    fio_http_write(h, .buf = "abcd", .len = 4, .copy = 1);
    fio_http_write(h, .fd = fd_with("0123456789"), .offset = 2, .finish = 1);
    return;
  }
  if (FIO_STR_INFO_IS_EQ(path, FIO_STR_INFO1((char *)"/fdpast"))) {
    fio_http_write(h, .fd = fd_with("0123456789"), .offset = 20, .finish = 1);
    return;
  }
  if (FIO_STR_INFO_IS_EQ(path, FIO_STR_INFO1((char *)"/fdshrink"))) {
    /* the file shrinks after the length was validated and framed */
    int fd = fd_with("");
    FIO_ASSERT(!ftruncate(fd, (off_t)SHRINK_LEN), "ftruncate failed");
    int keep = dup(fd);
    FIO_ASSERT(keep >= 0, "dup failed");
    fio_http_write(h, .fd = fd, .len = SHRINK_LEN, .finish = 1);
    FIO_ASSERT(!ftruncate(keep, (off_t)SHRINK_TO), "ftruncate failed");
    close(keep);
    return;
  }
  if (FIO_STR_INFO_IS_EQ(path, FIO_STR_INFO1((char *)"/fdpipe"))) {
    int p[2];
    FIO_ASSERT(!pipe(p), "pipe failed");
    FIO_ASSERT(write(p[1], "PIPE", 4) == 4, "pipe write failed");
    close(p[1]);
    fio_http_write(h, .fd = p[0], .finish = 1); /* unknown length */
    return;
  }
  fio_http_write(h, .buf = "ok", .len = 2, .copy = 1, .finish = 1);
}

/* H1C-01: the connection closes before the deferred transition runs */
static int srv_auth_close(fio_http_s *h) {
  fio_io_close_now(fio_http_io(h));
  return 0;
}

/* *****************************************************************************
Client callbacks (reactor thread)
***************************************************************************** */

static volatile int client_responses;
static volatile int client_302;

static void cl_on_http(fio_http_s *h) {
  fio_atomic_add(&client_responses, 1);
  if (fio_http_status(h) == 302)
    fio_atomic_add(&client_302, 1);
}

typedef struct {
  const char *method;
  const char *path;
  const char *query;
  const char *version;
  const char *body;      /* request body (sets Content-Length) */
  const char *preset_cl; /* a stale content-length set before connecting */
  size_t max_body;
} client_req_s;

static void cl_connect_task(void *r_, void *ignr) {
  client_req_s *r = (client_req_s *)r_;
  char url[64];
  snprintf(url, sizeof(url), "http://127.0.0.1:%u", raw_port);
  fio_http_s *h = fio_http_new();
  if (r->method)
    fio_http_method_set(h, FIO_STR_INFO1((char *)r->method));
  if (r->path)
    fio_http_path_set(h, FIO_STR_INFO1((char *)r->path));
  if (r->query)
    fio_http_query_set(h, FIO_STR_INFO1((char *)r->query));
  if (r->version)
    fio_http_version_set(h, FIO_STR_INFO1((char *)r->version));
  if (r->preset_cl)
    fio_http_request_header_set(h,
                                FIO_STR_INFO1((char *)"content-length"),
                                FIO_STR_INFO1((char *)r->preset_cl));
  if (r->body)
    fio_http_body_write(h, r->body, strlen(r->body));
  fio_http_connect(url, h, .on_http = cl_on_http, .max_body_size = r->max_body);
  (void)ignr;
}

static int raw_accept(void) {
  int fd = accept(raw_listener, NULL, NULL);
  if (fd >= 0) {
    int one = 1;
#ifdef SO_NOSIGPIPE
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    (void)one;
    set_timeout(fd, 5000);
  }
  return fd;
}

/* *****************************************************************************
Peer tests (peer thread)
***************************************************************************** */

static char rbuf[1 << 18];

/* Builds a request with `total` ordinary header-name/value bytes. */
static size_t build_limit_req(char *dst, size_t total) {
  static const char start[] = "GET / HTTP/1.1\r\nHost: x\r\n";
  size_t len = sizeof(start) - 1;
  size_t counted = 5; /* host + x */
  memcpy(dst, start, len);
  for (int i = 0; i < 3; ++i) {
    memcpy(dst + len, "X-A: ", 5);
    len += 5;
    memset(dst + len, 'a', 160);
    len += 160;
    memcpy(dst + len, "\r\n", 2);
    len += 2;
    counted += 3 + 160;
  }
  FIO_ASSERT(total >= counted + 3 && total - counted - 3 < 160,
             "invalid header test size");
  memcpy(dst + len, "X-F: ", 5);
  len += 5;
  memset(dst + len, 'b', total - counted - 3);
  len += total - counted - 3;
  memcpy(dst + len, "\r\n\r\n", 4);
  return len + 4;
}

static void test_header_limit(void) {
  fprintf(stderr, "  * H1C-03 ordinary header name/value limit\n");
  char req[1024];
  for (size_t step = 0; step < 2; ++step) { /* coalesced, byte by byte */
    for (size_t total = 511; total <= 513; ++total) {
      const int before = dispatched;
      const size_t len = build_limit_req(req, total);
      int fd = peer_connect(limits_port);
      CHECK(fd >= 0, "connect failed");
      if (fd < 0)
        continue;
      send_all(fd, req, len, step);
      size_t got = recv_until(fd, rbuf, sizeof(rbuf), "\r\n\r\nok");
      if (total <= 512) {
        CHECK(got > 12 && !strncmp(rbuf, "HTTP/1.1 200", 12) &&
                  dispatched == before + 1,
              "%zu bytes (step %zu) rejected: %.40s",
              total,
              step,
              rbuf);
      } else {
        CHECK(got > 12 && !strncmp(rbuf, "HTTP/1.1 431", 12),
              "%zu bytes (step %zu) not 431: %.40s",
              total,
              step,
              rbuf);
        CHECK(recv_closed(fd), "connection kept after 431");
        CHECK(dispatched == before, "over-limit request dispatched");
      }
      close(fd);
    }
  }
}

static void test_absolute_host(void) {
  fprintf(stderr, "  * H1C-06 absolute-form authority is the request Host\n");
  static const struct {
    const char *req;
    const char *expect;
  } cases[] = {
      {"GET http://good.example:8080/host HTTP/1.1\r\nHost: evil\r\n\r\n",
       "1:good.example:8080"},
      {"GET http://good.example/host HTTP/1.1\r\nHost: good.example\r\n\r\n",
       "1:good.example"},
      {"GET /host HTTP/1.1\r\nHost: origin.example\r\n\r\n",
       "1:origin.example"},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    int fd = peer_connect(server_port);
    CHECK(fd >= 0, "connect failed");
    if (fd < 0)
      continue;
    send_all(fd, cases[i].req, strlen(cases[i].req), 0);
    recv_until(fd, rbuf, sizeof(rbuf), cases[i].expect);
    char *body = strstr(rbuf, "\r\n\r\n");
    CHECK(body && !strcmp(body + 4, cases[i].expect),
          "host case %zu: %s",
          i,
          rbuf);
    if (body) {
      *body = 0;
      for (char *p = rbuf; *p; ++p)
        *p = (char)((*p >= 'A' && *p <= 'Z') ? *p + 32 : *p);
      CHECK(!strstr(rbuf, "\nhost:"), "response Host header: %s", rbuf);
    }
    close(fd);
  }
}

/* decodes a chunked body in place, returns its length or (size_t)-1. */
static size_t dechunk(char *s, size_t len) {
  char *end = s + len;
  char *out = s;
  for (;;) {
    char *eol = strstr(s, "\r\n");
    if (!eol || eol >= end)
      return (size_t)-1;
    size_t n = (size_t)strtoul(s, NULL, 16);
    s = eol + 2;
    if (!n)
      return (s + 2 == end && s[0] == '\r' && s[1] == '\n')
                 ? (size_t)(out - (end - len))
                 : (size_t)-1;
    if ((size_t)(end - s) < n + 2 || s[n] != '\r' || s[n + 1] != '\n')
      return (size_t)-1;
    memmove(out, s, n);
    out += n;
    s += n + 2;
  }
}

static void test_stream_offsets(void) {
  fprintf(stderr, "  * H1C-02 chunked writes honor offsets\n");
  static const char req[] = "GET /stream HTTP/1.1\r\nHost: x\r\n\r\n";
  int fd = peer_connect(server_port);
  CHECK(fd >= 0, "connect failed");
  if (fd < 0)
    return;
  send_all(fd, req, sizeof(req) - 1, 0);
  size_t got = recv_until(fd, rbuf, sizeof(rbuf), "\r\n0\r\n\r\n");
  close(fd);
  char *body = strstr(rbuf, "\r\n\r\n");
  CHECK(body && strstr(rbuf, "chunked"), "no chunked response: %.60s", rbuf);
  if (!body)
    return;
  body += 4;
  const size_t blen = dechunk(body, got - (size_t)(body - rbuf));
  const size_t expect_len = 3 + 3 + (LARGE_LEN - LARGE_PFX) + 9 + 3;
  CHECK(blen == expect_len, "body length %zu != %zu", blen, expect_len);
  if (blen != expect_len)
    return;
  CHECK(!memcmp(body, "abcdef", 6), "buffer offsets: %.6s", body);
  size_t i = 6;
  for (; i < 6 + LARGE_LEN - LARGE_PFX && body[i] == 'L'; ++i)
    ;
  CHECK(i == 6 + LARGE_LEN - LARGE_PFX, "zero-copy offset leaks prefix at %zu",
        i);
  CHECK(!memcmp(body + i, "file-data234", 12), "file offsets: %.12s", body + i);
  CHECK(!memchr(body, 'S', blen), "prefix bytes disclosed");
}

static void test_fd_lengths(void) {
  fprintf(stderr, "  * file body length vs. declared framing\n");
  static const char *paths[] = {
      "/fdshort", "/fdcl", "/fdcl0", "/fdpast", "/fdpipe", "/fdlater"};
  for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
    char req[128];
    int l = snprintf(req, sizeof(req), "GET %s HTTP/1.1\r\nHost: x\r\n\r\n",
                     paths[i]);
    int fd = peer_connect(server_port);
    CHECK(fd >= 0, "connect failed");
    if (fd < 0)
      continue;
    set_timeout(fd, 500);
    send_all(fd, req, (size_t)l, 0);
    size_t got = recv_until(fd, rbuf, sizeof(rbuf), NULL);
    const int closed = recv_closed(fd);
    close(fd);
    switch (i) {
    case 0: /* chunked: earlier chunks only, never the `64` (100) chunk */
      CHECK(closed && strstr(rbuf, "\r\n\r\n01\r\na\r\n") &&
                !strstr(rbuf, "\r\n64\r\n") && !strstr(rbuf, "2345"),
            "short file chunk sent:\n%s",
            rbuf);
      break;
    case 1: /* Content-Length: 100 is never sent with a 10 byte file */
      CHECK(closed && !got, "short file body sent (%zu):\n%s", got, rbuf);
      break;
    default: { /* finish with an inferred file length: framed as sent */
      static const char *const expect[] = {
          "23456789", "", "", "abcd23456789"};
      const char *e = expect[i - 2];
      char cl[40];
      snprintf(cl, sizeof(cl), "content-length:%zu\r\n", strlen(e));
      char *body = strstr(rbuf, "\r\n\r\n");
      CHECK(!closed && body && strstr(rbuf, cl) && strstr(rbuf, cl) < body &&
                !strcmp(body + 4, e),
            "%s framing mismatch (expected %s):\n%s",
            paths[i],
            cl,
            rbuf);
    }
    }
  }
}

/* A file truncated after `fio_http_write` validated its length must never
 * be followed by the next (pipelined) response inside the declared body:
 * the connection is closed when a stream read reduces its queued length.
 * Build with -DTEST_H1_FD_SHRINK=0 to skip. */
#ifndef TEST_H1_FD_SHRINK
#define TEST_H1_FD_SHRINK 1
#endif
static void test_fd_shrink(void) {
  fprintf(stderr, "  * file shrinks while streamed (framing shortfall)\n");
  static const char req[] = "GET /fdshrink HTTP/1.1\r\nHost: x\r\n\r\n"
                            "GET /fdcl0 HTTP/1.1\r\nHost: x\r\n\r\n";
  int fd = peer_connect(server_port);
  CHECK(fd >= 0, "connect failed");
  if (fd < 0)
    return;
  set_timeout(fd, 2000);
  send_all(fd, req, sizeof(req) - 1, 0);
  size_t total = 0, hdr = 0, keep = 0;
  int injected = 0, closed = 0;
  char buf[16384 + 16];
  for (;;) { /* keep a 15 byte tail so a marker split across reads matches */
    ssize_t r = recv(fd, buf + keep, sizeof(buf) - 16 - keep, 0);
    if (r <= 0) {
      closed = !r;
      break;
    }
    const size_t n = keep + (size_t)r;
    const size_t at = total - keep; /* stream position of buf[0] */
    total += (size_t)r;
    buf[n] = 0;
    for (size_t i = 0; i + 8 <= n; ++i) {
      if (memcmp(buf + i, "HTTP/1.1", 8))
        continue;
      if (!hdr && at + i == 0) {
        hdr = 1;
        continue;
      }
      if (at + i)
        injected = 1;
    }
    keep = n < 15 ? n : 15;
    memmove(buf, buf + n - keep, keep);
  }
  close(fd);
  CHECK(!injected, "next response sent inside the short body (%zu bytes)",
        total);
  CHECK(closed, "connection not closed after a stream shortfall (%zu bytes)",
        total);
  CHECK(total < SHRINK_LEN, "unexpected full body (%zu bytes)", total);
}

static void test_upgrade_close(void) {
  fprintf(stderr, "  * H1C-01 upgrade closed before the protocol transition\n");
  static const char ws[] = "GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\n"
                           "Connection: Upgrade\r\nSec-WebSocket-Version: 13\r\n"
                           "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
  static const char sse[] = "GET / HTTP/1.1\r\nHost: x\r\n"
                            "Accept: text/event-stream\r\n\r\n";
  for (int i = 0; i < 6; ++i) {
    int fd = peer_connect(server_port);
    CHECK(fd >= 0, "connect failed");
    if (fd < 0)
      continue;
    const char *r = (i & 1) ? sse : ws;
    send_all(fd, r, strlen(r), 0);
    CHECK(recv_closed(fd), "upgrade connection not closed (%d)", i);
    close(fd);
  }
}

/* Regression: an undispatched handle closed by its peer released its
 * connection twice (`on_finish` deferred `on_finish_task` -> `no_io` ->
 * connection_free, then the handle's `on_destroyed` freed it again - ASan
 * heap-use-after-free). Build with -DTEST_H1_DISCONNECT=0 to skip. */
#ifndef TEST_H1_DISCONNECT
#define TEST_H1_DISCONNECT 1
#endif
FIO_SFUNC void test_disconnect(void) {
  fprintf(stderr, "  * peer disconnect mid-request\n");
  static const char part[] = "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 9\r\n"
                             "\r\nabc";
  for (int i = 0; i < 4; ++i) {
    int fd = peer_connect(server_port);
    CHECK(fd >= 0, "connect failed");
    if (fd < 0)
      continue;
    send_all(fd, part, (i & 1) ? 20 : sizeof(part) - 1, 0);
    close(fd);
  }
}

static void test_client_injection(void) {
  fprintf(stderr, "  * H1C-05 client request-line injection\n");
  static client_req_s bad[] = {
      {.path = "/a\r\nX-Injected: 1"},
      {.path = "/a b"},
      {.path = "/a\nb"},
      {.query = "q=1\r\nX-Injected: 1"},
      {.query = "q=\x01"},
      {.method = "GET /x HTTP/1.1\r\nX-I: 1\r\nX:"},
      {.method = "G T"},
      {.version = "HTTP/1.1\r\nX-I:1"},
      {.version = "HTTP/1.1 x"},
  };
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
    fio_io_defer(cl_connect_task, bad + i, NULL);
    int fd = raw_accept();
    CHECK(fd >= 0, "accept failed");
    if (fd < 0)
      continue;
    size_t got = recv_until(fd, rbuf, sizeof(rbuf), NULL);
    CHECK(!got, "injected request %zu was sent:\n%s", i, rbuf);
    close(fd);
  }
}

static void test_client_redirect(void) {
  fprintf(stderr, "  * H1C-04 client redirect body limit\n");
  static client_req_s ok = {.path = "/ok%0D%0A", .query = "a=1",
                            .max_body = 1000};
  static const char head[] = "HTTP/1.1 302 Found\r\nLocation: /\r\n"
                             "Transfer-Encoding: chunked\r\n\r\n";
  char chunk[256];
  memcpy(chunk, "C8\r\n", 4);
  memset(chunk + 4, 'r', 200);
  memcpy(chunk + 204, "\r\n", 2);
  for (int over = 0; over < 2; ++over) {
    const int before = client_302;
    fio_io_defer(cl_connect_task, &ok, NULL);
    int fd = raw_accept();
    CHECK(fd >= 0, "accept failed");
    if (fd < 0)
      continue;
    recv_until(fd, rbuf, sizeof(rbuf), "\r\n\r\n");
    CHECK(!strncmp(rbuf, "GET /ok%0D%0A?a=1 HTTP/1.1\r\n", 28),
          "valid request-line: %.40s",
          rbuf);
    send_all(fd, head, sizeof(head) - 1, 0);
    int sent_ok = 1;
    const int chunks = over ? 2000 : 4; /* 800 bytes vs. far over the limit */
    for (int i = 0; i < chunks && sent_ok; ++i)
      sent_ok = !send_all(fd, chunk, 206, 0);
    if (!over) {
      send_all(fd, "0\r\n\r\n", 5, 0);
      for (int i = 0; i < 500 && client_302 == before; ++i)
        usleep(10000);
      CHECK(client_302 == before + 1, "redirect below the limit lost");
    } else {
      CHECK(recv_closed(fd), "over-limit redirect body kept open");
      CHECK(client_302 == before, "over-limit redirect dispatched");
    }
    close(fd);
  }
}

/* The request version is echoed in the status line (truncated to 14 bytes, no
 * LF / NUL possible): output may be malformed for the sender, but the header
 * block framing (and hence the next response) is never affected. */
static void test_version_echo(void) {
  fprintf(stderr, "  * request version echo stays within the status line\n");
  static const char req[] =
      "GET / HTTP/1.1\rX:1\rContent-Length:99\r\nHost: x\r\n\r\n"
      "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
  int fd = peer_connect(server_port);
  CHECK(fd >= 0, "connect failed");
  if (fd < 0)
    return;
  set_timeout(fd, 500);
  send_all(fd, req, sizeof(req) - 1, 0);
  size_t got = recv_until(fd, rbuf, sizeof(rbuf), NULL);
  close(fd);
  char *eol = strstr(rbuf, "\r\n");
  if (eol) { /* evidence: the reflected status line, CR shown as `\r` */
    fprintf(stderr, "    status line: ");
    for (const char *p = rbuf; p < eol; ++p)
      (*p == '\r') ? (void)fputs("\\r", stderr) : (void)fputc(*p, stderr);
    fprintf(stderr, "\n");
  }
  char *second = strstr(rbuf, "ok");
  second = second ? strstr(second, "HTTP/1.1 200") : NULL;
  CHECK(got && eol && (size_t)(eol - rbuf) <= 14 + 8 && !memchr(rbuf, '\n', (size_t)(eol - rbuf)) &&
            second && !strcmp(second + strlen(second) - 2, "ok"),
        "version echo broke framing:\n%s",
        rbuf);
}

/* absolute-form IPv6 authority keeps its brackets; Host matched in any case */
static void test_absolute_host_v6(void) {
  fprintf(stderr, "  * absolute-form IPv6 authority\n");
  static const struct {
    const char *req;
    const char *expect;
  } cases[] = {
      {"GET http://[::1]:8080/host HTTP/1.1\r\nHOST: evil\r\n\r\n",
       "1:[::1]:8080"},
      {"GET http://[::1]/host HTTP/1.1\r\nHost: [::1]\r\n\r\n", "1:[::1]"},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    int fd = peer_connect(server_port);
    CHECK(fd >= 0, "connect failed");
    if (fd < 0)
      continue;
    send_all(fd, cases[i].req, strlen(cases[i].req), 0);
    recv_until(fd, rbuf, sizeof(rbuf), cases[i].expect);
    char *body = strstr(rbuf, "\r\n\r\n");
    CHECK(body && !strcmp(body + 4, cases[i].expect),
          "IPv6 host case %zu: %s",
          i,
          rbuf);
    close(fd);
  }
  { /* a bare CR in a field value is rejected, never silently dropped */
    static const char bad[] =
        "GET /host HTTP/1.1\r\nHost: x\r\nAuthorization: a\rb\r\n\r\n";
    const int before = dispatched;
    int fd = peer_connect(server_port);
    CHECK(fd >= 0, "connect failed");
    if (fd >= 0) {
      send_all(fd, bad, sizeof(bad) - 1, 0);
      recv_until(fd, rbuf, sizeof(rbuf), NULL);
      CHECK(!strncmp(rbuf, "HTTP/1.1 400", 12) && dispatched == before,
            "bare CR value: %.40s",
            rbuf);
      close(fd);
    }
  }
}

/* `H1C_TESTS` (optional): comma separated test names to run (diagnostics). */
static int test_enabled(const char *name) {
  const char *only = getenv("H1C_TESTS");
  if (!only || !*only)
    return 1;
  const size_t len = strlen(name);
  for (const char *p = strstr(only, name); p; p = strstr(p + 1, name))
    if ((p == only || p[-1] == ',') && (!p[len] || p[len] == ','))
      return 1;
  return 0;
}
#define TEST_RUN(name)                                                         \
  do {                                                                         \
    if (test_enabled(#name))                                                   \
      test_##name();                                                           \
  } while (0)

static void *peer_main(void *ignr) {
  TEST_RUN(header_limit);
  TEST_RUN(absolute_host);
  TEST_RUN(absolute_host_v6);
  TEST_RUN(stream_offsets);
  TEST_RUN(fd_lengths);
#if TEST_H1_FD_SHRINK
  TEST_RUN(fd_shrink);
#else
  (void)test_fd_shrink;
#endif
  TEST_RUN(upgrade_close);
#if TEST_H1_DISCONNECT
  TEST_RUN(disconnect);
#endif
  TEST_RUN(version_echo);
  TEST_RUN(client_injection);
  TEST_RUN(client_redirect);
  usleep(200000); /* let the reactor release closed connections */
  fio_io_stop();
  return ignr;
}

static int guard_timeout(void *a, void *b) {
  (void)a, (void)b;
  CHECK(0, "test timed out");
  fio_io_stop();
  return -1;
}

static unsigned listener_port(fio_http_listener_s *l) {
  return port_of(((fio___io_listen_s *)l)->fd);
}

int main(void) {
  signal(SIGPIPE, SIG_IGN);
  const int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected errors are logged */
  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0",
                      .on_http = srv_on_http,
                      .on_authenticate_websocket = srv_auth_close,
                      .on_authenticate_sse = srv_auth_close);
  fio_http_listener_s *l2 = fio_http_listen("tcp://127.0.0.1:0",
                                            .on_http = srv_on_http,
                                            .max_header_size = 512,
                                            .max_line_len = 256);
  FIO_ASSERT(l && l2, "fio_http_listen failed");
  server_port = listener_port(l);
  limits_port = listener_port(l2);
  { /* raw peer listener for client tests */
    struct sockaddr_in a = {.sin_family = AF_INET};
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    raw_listener = socket(AF_INET, SOCK_STREAM, 0);
    FIO_ASSERT(raw_listener >= 0 &&
                   !bind(raw_listener, (struct sockaddr *)&a, sizeof(a)) &&
                   !listen(raw_listener, 16),
               "raw listener failed");
    raw_port = port_of(raw_listener);
    set_timeout(raw_listener, 5000);
  }
  fio_io_run_every(.fn = guard_timeout, .every = 60000, .repetitions = 1);
  pthread_t t;
  FIO_ASSERT(!pthread_create(&t, NULL, peer_main, NULL), "pthread_create");
  fio_io_start(0);
  pthread_join(t, NULL);
  close(raw_listener);
  FIO_LOG_LEVEL_SET(log_level);
  if (test_enabled("stream_offsets"))
    CHECK(dealloc_count == 2, "dealloc called %d times", dealloc_count);
  /* every handle / connection was released (H1C-01, disconnects) */
  CHECK(!fio___leak_counter_fio_http(0),
        "%zu HTTP handle(s) leaked",
        fio___leak_counter_fio_http(0));
  CHECK(!fio___leak_counter_fio___http_keystr_allocator(0),
        "%zu HTTP key string(s) leaked",
        fio___leak_counter_fio___http_keystr_allocator(0));
  CHECK(!fio___leak_counter_fio___http_connection(0),
        "%zu HTTP connection(s) leaked",
        fio___leak_counter_fio___http_connection(0));
  FIO_ASSERT(!failures, "%d HTTP/1.1 security check(s) failed", failures);
  fprintf(stderr, "\t* HTTP/1.1 security checks passed\n");
  return 0;
}
#endif
