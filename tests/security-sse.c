/* *****************************************************************************
Test: EventSource (SSE) security regressions (434 sse.h)

- SSE-01: CR / LF / CRLF line ends (a CRLF split across reads is one line
  end) and every fragmentation split are parsed within the read buffer
  (exact-size allocation, ASan visible). Spec field semantics: case sensitive
  names, no-colon / unknown fields, persistent last event ID, NUL IDs.
- SSE-02: `id` / `event` with CR / LF / NUL are refused; `data` is split on
  CR, LF and CRLF (also through FIO_HTTP_SSE_SUBSCRIBE_DIRECT); writer output
  round-trips through the parser.
- SSE-03: generic writes honor `offset` and frame `fd` data; the original
  buffer is freed once and the fd is closed (unless `copy`).
- SSE-04: `id` / `event` survive buffer compaction and later reads.
- SSE-05: writes to a closed connection fail.

A live single-thread reactor is driven by a raw POSIX socket peer thread.
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

/* *****************************************************************************
Incoming parser (direct, no IO: `c->io == NULL` makes closing a no-op)
***************************************************************************** */

static char rec[1 << 15];
static size_t rec_len;

static void rec_put(const char *s, size_t len) {
  if (len && len < sizeof(rec) - rec_len) {
    memcpy(rec + rec_len, s, len);
    rec_len += len;
  }
}

static void rec_on_event(fio_http_s *h,
                         fio_buf_info_s id,
                         fio_buf_info_s event,
                         fio_buf_info_s data) {
  rec_put("[", 1);
  if (id.buf)
    rec_put(id.buf, id.len);
  else
    rec_put("-", 1);
  rec_put("|", 1);
  if (event.buf)
    rec_put(event.buf, event.len);
  else
    rec_put("-", 1);
  rec_put("|", 1);
  rec_put(data.buf, data.len);
  rec_put("]", 1);
  (void)h;
}

static fio_http_settings_s parse_settings;

/**
 * Parses `in` in chunks of `chunk` bytes (0 = two chunks split at `split`),
 * appending events to `rec`. Returns 1 if the parser closed the connection.
 */
static int parse_run(const char *in, size_t len, size_t split, size_t chunk) {
  /* exact size: any read past the input end is visible to ASan */
  fio___http_connection_s *c =
      (fio___http_connection_s *)calloc(1, sizeof(*c) + len);
  int closed = 0;
  FIO_ASSERT(c, "allocation failed");
  c->settings = &parse_settings;
  c->capa = (uint32_t)len;
  c->state.sse.on_message = rec_on_event;
  rec_len = 0;
  for (size_t pos = 0; pos < len && !closed;) {
    size_t n = chunk ? chunk : (pos ? len - pos : split);
    if (n > len - pos)
      n = len - pos;
    memcpy(c->buf + c->len, in + pos, n);
    /* poison the free space: stale views would read it */
    memset(c->buf + c->len + n, '#', c->capa - c->len - n);
    c->len += (uint32_t)n;
    pos += n;
    closed = !!fio___sse_consume_data(c);
  }
  fio___sse_state_destroy(&c->state.sse);
  free(c);
  return closed;
}

typedef struct {
  const char *in;
  const char *expect;
  int closes;
} parse_case_s;

static void parse_expect2(const char *in,
                          size_t len,
                          const char *expect,
                          int closes) {
  const size_t elen = strlen(expect);
  for (size_t split = 0; split <= len; ++split) {
    const size_t chunk = (split == len) ? 1 : 0; /* last: byte by byte */
    const size_t s = split ? split : len;        /* split 0: one chunk */
    const int closed = parse_run(in, len, s, chunk);
    CHECK(closed == closes && rec_len == elen && !memcmp(rec, expect, elen),
          "parse(%s) split %zu: closed %d, got %.*s",
          in,
          split,
          closed,
          (int)rec_len,
          rec);
  }
}

static void parse_expect(const char *in, const char *expect, int closes) {
  parse_expect2(in, strlen(in), expect, closes);
}

static void test_parser(void) {
  fprintf(stderr, "  * SSE-01 / SSE-04 parser bounds and fragmentation\n");
  parse_settings.ws_max_msg_size = 1024;
  static const parse_case_s cases[] = {
      {"\n\n", "", 0},
      {"\r\n\r\n\n\r\r", "", 0},
      {"data:x\n\n", "[-|-|x]", 0},
      {"data:x\r\n\r\n", "[-|-|x]", 0},
      /* a consumed CRLF must not suppress the next (blank) LF line */
      {"data:x\r\n\n", "[-|-|x]", 0},
      {"data:x\r\n\ndata:y\n\n", "[-|-|x][-|-|y]", 0},
      {"data:x\r\r\n", "[-|-|x]", 0},
      /* a CRLF split across reads is one line end, never an extra line */
      {"data:a\r\ndata:b\r\n\r\n", "[-|-|a\nb]", 0},
      /* bare CR line ends: immediate, even as the very last byte */
      {"data:x\r\r", "[-|-|x]", 0},
      {"data:a\r\ndata:b\r\r", "[-|-|a\nb]", 0},
      {"data:a\r\n\r\ndata:b\n\r\r\n", "[-|-|a][-|-|b]", 0},
      {"data:a\rdata:b\r\rdata:c\r\r\n", "[-|-|a\nb][-|-|c]", 0},
      {"id:7\nevent:ev\ndata:a\ndata: b\n\n", "[7|ev|a\nb]", 0},
      {"id:1\r\n:c\r\nevent:e\r\ndata:z\r\n\r\nid:2\ndata:y\n\n",
       "[1|e|z][2|-|y]",
       0},
      /* field names are case sensitive; unknown / no-colon lines ignored */
      {"ID:1\nData:x\nEVENT:e\ndata:y\n\n", "[-|-|y]", 0},
      {"data:x\ngarbage\nretry:10\n\n", "[-|-|x]", 0},
      {"\n\nx\n", "", 0},
      /* no colon: the field has an empty value */
      {"data\ndata\n\n", "[-|-|\n]", 0},
      /* only one leading space is removed, tabs are kept */
      {"data:  x\ndata:\ty\n\n", "[-|-| x\n\ty]", 0},
      {"id:\ndata:q\n\n", "[|-|q]", 0},
      {"data:\ndata:x\ndata:\n\n", "[-|-|\nx\n]", 0},
      {"data:\n\n", "[-|-|]", 0},
      /* no data: no dispatch; `event` resets, the last event ID persists */
      {"id:abcdef\nevent:ghijkl\n\ndata:m\n\n", "[abcdef|-|m]", 0},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    parse_expect(cases[i].in, cases[i].expect, cases[i].closes);

  /* the LF joining `data` lines counts against the limit */
  parse_settings.ws_max_msg_size = 5;
  parse_expect("data:ab\ndata:cd\n\n", "[-|-|ab\ncd]", 0);
  parse_settings.ws_max_msg_size = 4;
  parse_expect("data:ab\ndata:cd\n\n", "", 1);
  parse_settings.ws_max_msg_size = 0; /* 0 = unlimited (as WebSocket) */
  parse_expect("data:abcdef\n\n", "[-|-|abcdef]", 0);
  parse_settings.ws_max_msg_size = 1024;
  { /* an ID containing NUL is ignored */
    static const char nul[] = "id:1\ndata:a\n\nid:x\0y\ndata:b\n\n";
    parse_expect2(nul, sizeof(nul) - 1, "[1|-|a][1|-|b]", 0);
  }

}

/* *****************************************************************************
Writer framing (direct)
***************************************************************************** */

static char *frame(const char *id, const char *event, const char *data) {
  fio_http_sse_write_args_s a = {
      .id = FIO_BUF_INFO2((char *)id, id ? strlen(id) : 0),
      .event = FIO_BUF_INFO2((char *)event, event ? strlen(event) : 0),
      .data = FIO_BUF_INFO2((char *)data, data ? strlen(data) : 0),
  };
  return fio___sse_frame(&a);
}

static void frame_expect(const char *id,
                         const char *event,
                         const char *data,
                         const char *expect) {
  char *s = frame(id, event, data);
  if (!expect) {
    CHECK(!s, "frame(%s) should fail, got %s", data, s);
  } else {
    CHECK(s && fio_bstr_len(s) == strlen(expect) && !strcmp(s, expect),
          "frame(%s): got %s",
          data,
          s ? s : "(null)");
  }
  fio_bstr_free(s);
}

/** Frames `data`, parses the frame, and requires LF-normalized `data`. */
static void roundtrip(const char *data, size_t len) {
  fio_http_sse_write_args_s a = {.data = FIO_BUF_INFO2((char *)data, len)};
  char *s = fio___sse_frame(&a);
  CHECK(s, "roundtrip frame failed");
  if (!s)
    return;
  char norm[512];
  size_t nl = 0;
  for (size_t i = 0; i < len; ++i) {
    if (data[i] == '\r' && i + 1 < len && data[i + 1] == '\n')
      continue;
    norm[nl++] = (data[i] == '\r') ? '\n' : data[i];
  }
  parse_run(s, fio_bstr_len(s), fio_bstr_len(s), 0);
  CHECK(rec_len == nl + 6 && !memcmp(rec + 5, norm, nl),
        "roundtrip(%.*s) got %.*s",
        (int)len,
        data,
        (int)rec_len,
        rec);
  fio_bstr_free(s);
}

static void test_frame(void) {
  fprintf(stderr, "  * SSE-02 writer framing / injection\n");
  frame_expect(NULL, NULL, "x", "data:x\r\n\r\n");
  frame_expect(NULL, NULL, "x\n", "data:x\r\ndata:\r\n\r\n");
  frame_expect(NULL, NULL, "\r\n", "data:\r\ndata:\r\n\r\n");
  frame_expect(NULL,
               NULL,
               "a\rb\r\nc\nd\n\re",
               "data:a\r\ndata:b\r\ndata:c\r\ndata:d\r\ndata:\r\ndata:e"
               "\r\n\r\n");
  frame_expect("1", "e", "x", "id:1\r\nevent:e\r\ndata:x\r\n\r\n");
  frame_expect("1\nretry:0", NULL, "x", NULL);
  frame_expect("1\r", NULL, "x", NULL);
  frame_expect(NULL, "a\nb", "x", NULL);
  frame_expect(NULL, "a\rdata:b", "x", NULL);
  frame_expect(NULL, NULL, "", NULL);
  { /* NUL in `id`, NULL buffers with a length */
    fio_http_sse_write_args_s a = {.id = FIO_BUF_INFO2((char *)"a\0b", 3),
                                   .data = FIO_BUF_INFO2((char *)"x", 1)};
    CHECK(!fio___sse_frame(&a), "NUL in id accepted");
    a.id = FIO_BUF_INFO2(NULL, 3);
    CHECK(!fio___sse_frame(&a), "NULL id buffer accepted");
    a.id = FIO_BUF_INFO0;
    a.data = FIO_BUF_INFO2(NULL, 3);
    CHECK(!fio___sse_frame(&a), "NULL data buffer accepted");
  }
  { /* many lines: exact output (geometric growth path) */
    static char many[20001];
    for (size_t i = 0; i < 20000; ++i)
      many[i] = (i & 1) ? '\n' : 'a';
    char *s = frame(NULL, NULL, many);
    CHECK(s && fio_bstr_len(s) == 10000 * 8 + 7 + 2,
          "many lines length %zu",
          fio_bstr_len(s));
    for (size_t i = 0; s && i < 10000; ++i)
      CHECK(!memcmp(s + i * 8, "data:a\r\n", 8), "many lines line %zu", i);
    fio_bstr_free(s);
  }
  static const char *const rt[] = {"x",
                                   "x\n",
                                   "\nx",
                                   "a\r\n\r\nb",
                                   "\r\r\n\n\r",
                                   "a\rb\nc\r\nd",
                                   "retry: 1\nid: 2\n\nevent: x"};
  for (size_t i = 0; i < sizeof(rt) / sizeof(rt[0]); ++i)
    roundtrip(rt[i], strlen(rt[i]));

}

/* *****************************************************************************
Live server: writer, generic controller and Pub/Sub direct callback
***************************************************************************** */

#if defined(_WIN32)
int main(void) {
  test_parser();
  test_frame();
  FIO_ASSERT(!failures, "%d SSE security check(s) failed", failures);
  return 0;
}
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static unsigned server_port;
static volatile int dealloc_count;
static void *dealloc_expected;
static volatile int fd_used = -1;
static volatile int fd_kept = -1;
static volatile int fd_used_closed;
static volatile int closed_write_result;
static volatile int open_results_ok;

static void count_dealloc(void *p) {
  CHECK(p == dealloc_expected, "dealloc got %p, not the original", p);
  fio_atomic_add(&dealloc_count, 1);
  free(p);
}

static int fd_with(const char *s) {
  char name[] = "ai-tmp/security-sse-XXXXXX";
  int fd = mkstemp(name);
  if (fd < 0) { /* not run from the repository root */
    char alt[] = "./security-sse-XXXXXX";
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

static void srv_open(fio_http_s *h) {
  int ok = 1;
  /* SSE-02: refused (nothing written) */
  ok &= fio_http_sse_write(h,
                           .id = FIO_BUF_INFO1((char *)"1\r\nretry: 1"),
                           .data = FIO_BUF_INFO1((char *)"x")) == -1;
  ok &= fio_http_sse_write(h,
                           .event = FIO_BUF_INFO1((char *)"e\rdata: y"),
                           .data = FIO_BUF_INFO1((char *)"x")) == -1;
  /* SSE-02: CR splits data lines */
  ok &= !fio_http_sse_write(h,
                            .id = FIO_BUF_INFO1((char *)"7"),
                            .data = FIO_BUF_INFO1((char *)"a\rretry: 1"));
  /* SSE-03: offset, original pointer freed once */
  char *buf = (char *)malloc(16);
  memcpy(buf, "SECRETmsg", 9);
  dealloc_expected = buf;
  fio_http_write(h,
                 .buf = buf,
                 .len = 3,
                 .offset = 6,
                 .dealloc = count_dealloc);
  /* SSE-03: fd data at offset, closed after use */
  fd_used = fd_with("skipfile\ndata");
  fio_http_write(h, .fd = fd_used, .len = 9, .offset = 4);
  fd_used_closed = fcntl(fd_used, F_GETFD) == -1; /* before fd reuse */
  /* `copy` keeps the fd open */
  fd_kept = fd_with("kept");
  fio_http_write(h, .fd = fd_kept, .len = 4, .copy = 1);
  /* Pub/Sub direct: the channel can't inject, the message is split */
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  fio_pubsub_msg_s msg = {.io = c->io,
                          .id = 0xAB,
                          .channel = FIO_BUF_INFO1((char *)"ch\r\nretry: 1"),
                          .message = FIO_BUF_INFO1((char *)"bad")};
  FIO_HTTP_SSE_SUBSCRIBE_DIRECT(&msg);
  msg.channel = FIO_BUF_INFO1((char *)"ch");
  msg.message = FIO_BUF_INFO1((char *)"m\nid: 9");
  FIO_HTTP_SSE_SUBSCRIBE_DIRECT(&msg);
  ok &= !fio_http_sse_write(h, .data = FIO_BUF_INFO1((char *)"end"));
  open_results_ok = ok;
}

static void srv_close(fio_http_s *h) {
  closed_write_result = fio_http_sse_write(h, .data = FIO_BUF_INFO1("late"));
}

static int peer_connect(void) {
  struct sockaddr_in a = {.sin_family = AF_INET,
                          .sin_port = htons((uint16_t)server_port)};
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return -1;
  struct timeval tv = {.tv_sec = 5};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  if (connect(fd, (struct sockaddr *)&a, sizeof(a))) {
    close(fd);
    return -1;
  }
  return fd;
}

static void *peer_main(void *ignr) {
  static const char req[] = "GET / HTTP/1.1\r\nHost: localhost\r\n"
                            "Accept: text/event-stream\r\n\r\n";
  static const char expect[] = "id:7\r\ndata:a\r\ndata:retry: 1\r\n\r\n"
                               "data:msg\r\n\r\n"
                               "data:file\r\ndata:data\r\n\r\n"
                               "data:kept\r\n\r\n"
                               "id:AB\r\nevent:ch\r\ndata:m\r\ndata:id: 9"
                               "\r\n\r\n"
                               "data:end\r\n\r\n";
  static char buf[8192];
  size_t len = 0;
  char *body = NULL;
  int fd = peer_connect();
  CHECK(fd >= 0, "connect failed");
  if (fd < 0)
    goto done;
  CHECK(send(fd, req, sizeof(req) - 1, 0) == (ssize_t)(sizeof(req) - 1),
        "send failed");
  for (;;) {
    ssize_t r = recv(fd, buf + len, sizeof(buf) - 1 - len, 0);
    if (r <= 0)
      break;
    len += (size_t)r;
    buf[len] = 0;
    body = strstr(buf, "\r\n\r\n");
    if (body && strstr(body + 4, "data:end\r\n\r\n"))
      break;
  }
  CHECK(!strncmp(buf, "HTTP/1.1 200", 12), "no SSE response: %.40s", buf);
  CHECK(body && !strcmp(body + 4, expect),
        "wire output:\n%s",
        body ? body : buf);
  close(fd);
done:
  fio_io_stop();
  return ignr;
}

static int guard_timeout(void *a, void *b) {
  (void)a, (void)b;
  CHECK(0, "test timed out");
  fio_io_stop();
  return -1;
}

int main(void) {
  test_parser();
  test_frame();
  fprintf(stderr, "  * SSE-02 / SSE-03 / SSE-05 live writes\n");
  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0",
                      .on_authenticate_sse = FIO_HTTP_AUTHENTICATE_ALLOW,
                      .on_open = srv_open,
                      .on_close = srv_close);
  FIO_ASSERT(l, "fio_http_listen failed");
  {
    fio___io_listen_s *li = (fio___io_listen_s *)l;
    struct sockaddr_in ss;
    socklen_t slen = (socklen_t)sizeof(ss);
    FIO_ASSERT(!getsockname(li->fd, (struct sockaddr *)&ss, &slen),
               "getsockname");
    server_port = ntohs(ss.sin_port);
  }
  fio_io_run_every(.fn = guard_timeout, .every = 30000, .repetitions = 1);
  pthread_t t;
  FIO_ASSERT(!pthread_create(&t, NULL, peer_main, NULL), "pthread_create");
  fio_io_start(0);
  pthread_join(t, NULL);
  fio_io_listen_stop((fio_io_listener_s *)l);
  CHECK(open_results_ok, "unexpected fio_http_sse_write results");
  CHECK(dealloc_count == 1, "dealloc called %d times", dealloc_count);
  CHECK(fd_used_closed, "fd left open");
  CHECK(fd_kept >= 0 && fcntl(fd_kept, F_GETFD) != -1, "`copy` fd closed");
  CHECK(closed_write_result == -1,
        "write after close returned %d",
        closed_write_result);
  if (fd_kept >= 0)
    close(fd_kept);
  FIO_ASSERT(!failures, "%d SSE security check(s) failed", failures);
  fprintf(stderr, "\t* SSE security checks passed\n");
  return 0;
}
#endif
