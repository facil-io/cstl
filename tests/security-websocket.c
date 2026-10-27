/* *****************************************************************************
Test: WebSocket controller security regressions (434 websocket.h)

A live single-thread reactor (server + client modes) is driven by a raw,
blocking POSIX socket peer running on a helper thread.

- WS-01: an invalid-version upgrade never negotiates permessage-deflate and
  releases its handle; offer parameters reach the (overflow-safe) negotiator;
  a valid offer still negotiates. (Context allocation uses the global
  FIO_MEM_REALLOC, which this hook cannot fail: reviewed statically.)
- WS-02: pong payloads are parsed / logged within their length (exercised
  with FIO_WEBSOCKET_STATS).
- WS-03: invalid UTF-8 text (whole / fragmented) and close reasons fail with
  1007; code points split across fragments are accepted.
- WS-04: client mode rejects masked server frames (1002) and masks pings.
- WS-05: an empty Close is answered with an empty Close (never 1005).
- WS-07: small generic writes honor `offset`.

All checks use FIO_ASSERT (active under NDEBUG).
***************************************************************************** */

#define FIO_WEBSOCKET_STATS 1
#define FIO_HTTP
#include "test-helpers.h"

#if defined(_WIN32)
int main(void) {
  fprintf(stderr, "\t* WebSocket security test skipped (POSIX sockets)\n");
  return 0;
}
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define WS_KEY "dGhlIHNhbXBsZSBub25jZQ=="

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
Server (reactor side)
***************************************************************************** */

static unsigned server_port;
static volatile int negotiated_bits = -1;
static int path_is(fio_http_s *h, const char *p) {
  fio_str_info_s s = fio_http_path(h);
  size_t l = strlen(p);
  return s.len == l && !memcmp(s.buf, p, l);
}

static int srv_auth(fio_http_s *h) {
  (void)h;
  return 0;
}

static void srv_open(fio_http_s *h) {
  if (path_is(h, "/offset")) {
    static const char data[] = "XXXXhello";
    /* a wrapped ("negative") offset is rejected, never framed */
    fio_http_write(h, .buf = data, .len = 5, .offset = (size_t)-4);
    fio_http_write(h, .buf = data, .len = 5, .offset = 4);
  } else if (path_is(h, "/bits")) {
    fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
    negotiated_bits = c->deflate_wr ? (int)c->deflate_wr->window_bits : 0;
    fio_http_websocket_write(h, "ok", 2, 1);
  }
}

static void srv_message(fio_http_s *h, fio_buf_info_s msg, uint8_t is_text) {
  fio_http_websocket_write(h, msg.buf, msg.len, is_text);
}

/* *****************************************************************************
Client-mode target (reactor side)
***************************************************************************** */

static unsigned raw_server_port;
static volatile int cli_messages;
static volatile int cli_opened;
static void cli_open(fio_http_s *h) {
  fio_atomic_add(&cli_opened, 1);
  (void)h;
}
static void cli_message(fio_http_s *h, fio_buf_info_s msg, uint8_t is_text) {
  fio_atomic_add(&cli_messages, 1);
  (void)h, (void)msg, (void)is_text;
}
static void cli_connect_task(void *a, void *b) {
  char url[64];
  snprintf(url, sizeof(url), "ws://127.0.0.1:%u/", raw_server_port);
  fio_io_s *io = fio_http_websocket_connect(url,
                                            NULL,
                                            .on_message = cli_message,
                                            .on_open = cli_open,
                                            .ws_timeout = 1);
  CHECK(io, "fio_http_websocket_connect failed");
  (void)a, (void)b;
}

/* *****************************************************************************
Raw peer helpers (peer thread: libc only)
***************************************************************************** */

typedef struct {
  int fd;
  size_t len;
  uint8_t buf[16384];
} peer_s;

static void peer_timeout(int fd) {
  struct timeval tv = {.tv_sec = 5};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static int peer_connect(peer_s *p, unsigned port) {
  struct sockaddr_in a = {.sin_family = AF_INET,
                          .sin_port = htons((uint16_t)port)};
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  p->len = 0;
  p->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (p->fd < 0)
    return -1;
  peer_timeout(p->fd);
  return connect(p->fd, (struct sockaddr *)&a, sizeof(a));
}

static void peer_send(peer_s *p, const void *data, size_t len) {
  const char *d = (const char *)data;
  while (len) {
    ssize_t w = send(p->fd, d, len, 0);
    if (w <= 0)
      return;
    d += w;
    len -= (size_t)w;
  }
}

/** Reads more bytes; returns 0 on EOF / timeout. */
static size_t peer_fill(peer_s *p) {
  if (p->len == sizeof(p->buf))
    return 0;
  ssize_t r = recv(p->fd, p->buf + p->len, sizeof(p->buf) - p->len, 0);
  if (r <= 0)
    return 0;
  p->len += (size_t)r;
  return (size_t)r;
}

static void peer_consume(peer_s *p, size_t n) {
  p->len -= n;
  memmove(p->buf, p->buf + n, p->len);
}

static char peer_raw_head[2048]; /* last head, before case folding */

/** Reads an HTTP response head into `head`; returns the status (0 = error). */
static int peer_http_response(peer_s *p, char *head, size_t cap) {
  for (;;) {
    for (size_t i = 3; i < p->len; ++i) {
      if (memcmp(p->buf + i - 3, "\r\n\r\n", 4))
        continue;
      size_t n = (i + 1 < cap) ? i + 1 : cap - 1;
      memcpy(head, p->buf, n);
      head[n] = 0;
      memcpy(peer_raw_head, head, n + 1);
      for (char *c = head; *c; ++c)
        *c = (char)((*c >= 'A' && *c <= 'Z') ? (*c | 32) : *c);
      peer_consume(p, i + 1);
      return (n > 12) ? atoi(head + 9) : 0;
    }
    if (!peer_fill(p))
      return 0;
  }
}

/** Reads one frame (unmasking it). Returns the opcode or -1. */
static int peer_frame(peer_s *p, uint8_t *out, size_t *out_len, int *masked) {
  for (;;) {
    if (p->len >= 2) {
      size_t hl = 2, len = p->buf[1] & 127;
      const int m = p->buf[1] >> 7;
      if (len == 126)
        hl = 4;
      else if (len == 127)
        hl = 10;
      hl += (size_t)m << 2;
      if (p->len >= hl) {
        if (len == 126)
          len = ((size_t)p->buf[2] << 8) | p->buf[3];
        else if (len == 127)
          len = (size_t)fio_buf2u64_be(p->buf + 2);
        if (len > 4096)
          return -1;
        if (p->len >= hl + len) {
          const int op = p->buf[0] & 15;
          for (size_t i = 0; i < len; ++i)
            out[i] = p->buf[hl + i] ^ (m ? p->buf[hl - 4 + (i & 3)] : 0);
          *out_len = len;
          if (masked)
            *masked = m;
          peer_consume(p, hl + len);
          return op;
        }
      }
    }
    if (!peer_fill(p))
      return -1;
  }
}

/** Appends a masked (client) frame to `dst`, returning its length. */
static size_t frame_cli(uint8_t *dst,
                        int fin,
                        int op,
                        const void *data,
                        size_t len) {
  static const uint8_t mask[4] = {0x11, 0x22, 0x33, 0x44};
  dst[0] = (uint8_t)((fin << 7) | op);
  dst[1] = (uint8_t)(0x80 | len); /* tests use payloads < 126 bytes */
  memcpy(dst + 2, mask, 4);
  for (size_t i = 0; i < len; ++i)
    dst[6 + i] = ((const uint8_t *)data)[i] ^ mask[i & 3];
  return 6 + len;
}

static void peer_send_frame(peer_s *p,
                            int fin,
                            int op,
                            const void *data,
                            size_t len) {
  uint8_t f[256];
  peer_send(p, f, frame_cli(f, fin, op, data, len));
}

/** Opens a WebSocket (server test target); returns the HTTP status. */
static int peer_ws_open(peer_s *p,
                        const char *path,
                        const char *extra,
                        char *head,
                        size_t cap) {
  char req[512];
  if (peer_connect(p, server_port))
    return 0;
  int n = snprintf(req,
                   sizeof(req),
                   "GET %s HTTP/1.1\r\nHost: localhost\r\n"
                   "Connection: Upgrade\r\nUpgrade: websocket\r\n"
                   "Sec-WebSocket-Key: " WS_KEY "\r\n%s\r\n",
                   path,
                   extra ? extra : "Sec-WebSocket-Version: 13\r\n");
  peer_send(p, req, (size_t)n);
  return peer_http_response(p, head, cap);
}

/** Expects a close frame carrying `code` (0 = empty close payload). */
static void peer_expect_close(peer_s *p, unsigned code, const char *what) {
  uint8_t pl[4096];
  size_t len = 0;
  int op = peer_frame(p, pl, &len, NULL);
  CHECK(op == 8, "%s: expected close frame, got %d", what, op);
  if (op != 8)
    return;
  if (!code) {
    CHECK(len == 0, "%s: expected empty close, got %zu bytes", what, len);
  } else {
    CHECK(len >= 2 && ((unsigned)pl[0] << 8 | pl[1]) == code,
          "%s: expected close %u, got %u",
          what,
          code,
          len >= 2 ? ((unsigned)pl[0] << 8 | pl[1]) : 0U);
  }
}

static void peer_expect_echo(peer_s *p,
                             int op,
                             const void *data,
                             size_t len,
                             const char *what) {
  uint8_t pl[4096];
  size_t got = 0;
  int r = peer_frame(p, pl, &got, NULL);
  CHECK(r == op && got == len && !memcmp(pl, data, len),
        "%s: echo mismatch (op %d, %zu bytes)",
        what,
        r,
        got);
}

/* *****************************************************************************
Scenarios (peer thread)
***************************************************************************** */

static void ws01_deflate(void) {
  fprintf(stderr,
          "  * WS-01 permessage-deflate negotiation is transactional\n");
  char head[2048];
  peer_s p;
  /* invalid version (+ offer): 400, never negotiated, repeatable */
  CHECK(!peer_connect(&p, server_port), "connect");
  {
    static const char req[] =
        "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: Upgrade\r\n"
        "Upgrade: websocket\r\nSec-WebSocket-Key: " WS_KEY "\r\n"
        "Sec-WebSocket-Version: 12\r\n"
        "Sec-WebSocket-Extensions: permessage-deflate\r\n\r\n";
    peer_send(&p, req, sizeof(req) - 1);
    peer_send(&p, req, sizeof(req) - 1);
    for (int i = 0; i < 2; ++i) {
      int st = peer_http_response(&p, head, sizeof(head));
      if (!st && i)
        break; /* the server may close after an error response */
      CHECK(st == 400, "bad version: status %d", st);
      CHECK(!strstr(head, "sec-websocket-extensions"),
            "bad version must not negotiate permessage-deflate");
      /* skip any error body (content-length) */
      const char *cl = strstr(head, "content-length:");
      size_t body = cl ? (size_t)atol(cl + 15) : 0;
      while (p.len < body && peer_fill(&p))
        ;
      peer_consume(&p, body <= p.len ? body : p.len);
    }
  }
  close(p.fd);
  /* valid offer: negotiated */
  int st = peer_ws_open(&p,
                        "/",
                        "Sec-WebSocket-Version: 13\r\n"
                        "Sec-WebSocket-Extensions: permessage-deflate\r\n",
                        head,
                        sizeof(head));
  CHECK(st == 101 && strstr(head, "sec-websocket-extensions"),
        "valid offer: status %d / extension missing",
        st);
  close(p.fd);
  /* offer parameters (header-value properties) reach the negotiator */
  st = peer_ws_open(&p,
                    "/bits",
                    "Sec-WebSocket-Version: 13\r\n"
                    "Sec-WebSocket-Extensions: permessage-deflate; "
                    "client_max_window_bits; server_max_window_bits=9\r\n",
                    head,
                    sizeof(head));
  CHECK(st == 101, "bits offer: status %d", st);
  peer_expect_echo(&p, 1, "ok", 2, "bits offer");
  CHECK(negotiated_bits == 9,
        "server_max_window_bits=9 not honored (%d)",
        negotiated_bits);
  close(p.fd);
  { /* negotiator: hostile / quoted parameter values */
    char out[128];
    int bits = 0;
    static const struct {
      const char *offer;
      int bits;
    } t[] = {
        {" server_max_window_bits=10", 10},
        {"server_max_window_bits = \"11\"", 11},
        {"server_max_window_bits=99999999999999999999999999", 15},
        {"server_max_window_bits=7", 15},
        {"", 15},
    };
    for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
      bits = 0;
      size_t len = fio___http_ws_deflate_negotiate(
          FIO_STR_INFO2((char *)t[i].offer, strlen(t[i].offer)),
          out,
          sizeof(out),
          &bits);
      CHECK(len && bits == t[i].bits,
            "negotiate(%s): bits %d != %d",
            t[i].offer,
            bits,
            t[i].bits);
    }
    bits = 0;
    CHECK(fio___http_ws_deflate_negotiate(FIO_STR_INFO2(NULL, 0),
                                          out,
                                          sizeof(out),
                                          &bits) &&
              bits == 15,
          "negotiate(NULL properties)");
  }
}

static void ws02_pong(void) {
  fprintf(stderr, "  * WS-02 pong payloads stay length-bound\n");
  char head[1024];
  peer_s p;
  int st = peer_ws_open(&p, "/", NULL, head, sizeof(head));
  CHECK(st == 101, "status %d", st);
  uint8_t f[512];
  size_t n = 0;
  /* empty, short, wrong-length, non-hex and valid-format pongs, followed by
   * hex-looking bytes in the same read buffer */
  n += frame_cli(f + n, 1, 10, "", 0);
  n += frame_cli(f + n, 1, 10, "0x", 2);
  n += frame_cli(f + n, 1, 10, "0xFFFFFFFFFFFFFFFFFFFF", 22);
  n += frame_cli(f + n, 1, 10, "0x00000000000000zz", 18);
  n += frame_cli(f + n, 1, 10, "0x0000000000000001", 18);
  n += frame_cli(f + n, 1, 9, "abcdef", 6); /* ping => pong */
  peer_send(&p, f, n);
  peer_expect_echo(&p, 10, "abcdef", 6, "ping after pongs");
  close(p.fd);
}

static void ws03_utf8(void) {
  fprintf(stderr, "  * WS-03 text / close reasons are UTF-8 validated\n");
  char head[1024];
  peer_s p;
  int st = peer_ws_open(&p, "/", NULL, head, sizeof(head));
  CHECK(st == 101, "status %d", st);
  peer_send_frame(&p, 1, 1, "hello", 5);
  peer_expect_echo(&p, 1, "hello", 5, "valid text");
  peer_send_frame(&p, 0, 1, "\xC3", 1); /* U+00E9 split over two frames */
  peer_send_frame(&p, 1, 0, "\xA9", 1);
  peer_expect_echo(&p, 1, "\xC3\xA9", 2, "split code point");
  peer_send_frame(&p, 1, 2, "\xFF\xFE", 2); /* binary: never validated */
  peer_expect_echo(&p, 2, "\xFF\xFE", 2, "binary");
  peer_send_frame(&p, 1, 1, "\xC0\xAF", 2); /* overlong '/' */
  peer_expect_close(&p, 1007, "invalid text");
  close(p.fd);

  st = peer_ws_open(&p, "/", NULL, head, sizeof(head));
  CHECK(st == 101, "status %d", st);
  peer_send_frame(&p, 0, 1, "ok", 2);
  peer_send_frame(&p, 1, 0, "\xED\xA0\x80", 3); /* surrogate, fragmented */
  peer_expect_close(&p, 1007, "invalid fragmented text");
  close(p.fd);

  st = peer_ws_open(&p, "/", NULL, head, sizeof(head));
  CHECK(st == 101, "status %d", st);
  peer_send_frame(&p, 1, 8, "\x03\xE8\xFF", 3);
  peer_expect_close(&p, 1007, "invalid close reason");
  close(p.fd);

  st = peer_ws_open(&p, "/", NULL, head, sizeof(head));
  CHECK(st == 101, "status %d", st);
  peer_send_frame(&p,
                  1,
                  8,
                  "\x03\xE8"
                  "bye",
                  5);
  peer_expect_close(&p, 1000, "valid close reason");
  close(p.fd);
}

static void ws05_empty_close(void) {
  fprintf(stderr, "  * WS-05 empty Close is answered without 1005\n");
  char head[1024];
  peer_s p;
  int st = peer_ws_open(&p, "/", NULL, head, sizeof(head));
  CHECK(st == 101, "status %d", st);
  peer_send_frame(&p, 1, 8, "", 0);
  peer_expect_close(&p, 0, "empty close");
  close(p.fd);
}

static void ws07_offset(void) {
  fprintf(stderr, "  * WS-07 small generic writes honor offset\n");
  char head[1024];
  peer_s p;
  int st = peer_ws_open(&p, "/offset", NULL, head, sizeof(head));
  CHECK(st == 101, "status %d", st);
  peer_expect_echo(&p, 1, "hello", 5, "offset write");
  close(p.fd);
}

static void ws04_client_mode(void) {
  fprintf(stderr, "  * WS-04 client mode masking policy\n");
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in a = {.sin_family = AF_INET};
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  socklen_t al = sizeof(a);
  CHECK(lfd >= 0 && !bind(lfd, (struct sockaddr *)&a, sizeof(a)) &&
            !listen(lfd, 1) && !getsockname(lfd, (struct sockaddr *)&a, &al),
        "raw listener");
  raw_server_port = ntohs(a.sin_port);
  fio_io_defer(cli_connect_task, NULL, NULL);
  peer_s p = {.fd = accept(lfd, NULL, NULL)};
  close(lfd);
  CHECK(p.fd >= 0, "accept");
  if (p.fd < 0)
    return;
  peer_timeout(p.fd);
  char head[2048];
  peer_http_response(&p, head, sizeof(head));
  const char *k = strstr(head, "\r\nsec-websocket-key:");
  CHECK(k, "client request lacks a key");
  if (!k) {
    close(p.fd);
    return;
  }
  k = peer_raw_head + (k - head) + 20; /* base64 is case sensitive */
  while (*k == ' ')
    ++k;
  size_t kl = 0;
  while (k[kl] && k[kl] != '\r')
    ++kl;
  FIO_STR_INFO_TMP_VAR(acc, 127); /* stack only: no allocation */
  fio_string_write(&acc, NULL, k, kl);
  fio_string_write(&acc, NULL, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
  fio_sha1_s sha = fio_sha1(acc.buf, acc.len);
  acc.len = 0;
  fio_string_write_base64enc(&acc, NULL, fio_sha1_digest(&sha), 20, 0);
  char res[256];
  int n = snprintf(res,
                   sizeof(res),
                   "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\n"
                   "Upgrade: websocket\r\nSec-WebSocket-Accept: %.*s\r\n\r\n",
                   (int)acc.len,
                   acc.buf);
  peer_send(&p, res, (size_t)n);
  /* the idle client pings (ws_timeout = 1): it MUST be masked */
  uint8_t pl[4096];
  size_t len = 0;
  int masked = 0;
  int op = peer_frame(&p, pl, &len, &masked);
  CHECK(op == 9 && masked && len == 18,
        "client ping: op %d masked %d len %zu",
        op,
        masked,
        len);
  /* a masked server frame MUST fail the client connection (1002) */
  peer_send_frame(&p, 1, 1, "hi", 2);
  op = peer_frame(&p, pl, &len, &masked);
  CHECK(op == 8 && masked && len >= 2 && ((unsigned)pl[0] << 8 | pl[1]) == 1002,
        "client must close masked server frame with 1002 (op %d, masked %d)",
        op,
        masked);
  CHECK(cli_opened, "client on_open not called");
  CHECK(!cli_messages, "masked server frame delivered");
  close(p.fd);
}

static void *peer_main(void *ignr) {
  ws01_deflate();
  ws02_pong();
  ws03_utf8();
  ws05_empty_close();
  ws07_offset();
  ws04_client_mode();
  fio_io_stop();
  return ignr;
}

/* Exercise the current streaming dependency through the actual controller:
 * multi-block input must survive overflow/retry, and the decoded-size limit
 * must reject expansion before allocating the full decoded message. */
static void deflate_dependency_checks(void) {
  char input[16000];
  char compressed[17000];
  for (size_t i = 0; i < sizeof(input); ++i)
    input[i] = (char)('a' + (i % 23));
  fio_deflate_s *wr = fio_deflate_new(1, 1);
  fio_deflate_s *rd = fio_deflate_new(1, 0);
  FIO_ASSERT(wr && rd, "deflate test contexts");
  size_t n = fio_deflate_push(wr, compressed, sizeof(compressed),
                              input, sizeof(input), 1);
  FIO_ASSERT(n >= 4 && n <= sizeof(compressed), "compress test payload");
  n -= 4; /* RFC 7692 removes the trailing sync-flush marker. */
  fio_http_settings_s settings = {.ws_max_msg_size = sizeof(input)};
  fio___http_connection_s c = {.settings = &settings, .deflate_rd = rd,
                               .deflate_rd_reset = 1};
  c.state.ws.msg = fio_bstr_write(NULL, compressed, n);
  fio_buf_info_s msg = fio_bstr_buf(c.state.ws.msg);
  FIO_ASSERT(!fio___websocket_deflate_transform(&c, &msg) &&
                 msg.len == sizeof(input) &&
                 !memcmp(msg.buf, input, sizeof(input)),
             "deflate controller retry lost decoded bytes");
  fio_bstr_free(c.state.ws.msg);
  c.state.ws.msg = fio_bstr_write(NULL, compressed, n);
  msg = fio_bstr_buf(c.state.ws.msg);
  settings.ws_max_msg_size = 128;
  FIO_ASSERT(fio___websocket_deflate_transform(&c, &msg) ==
                 FIO_WEBSOCKET_CLOSE_MESSAGE_TOO_BIG,
             "compressed expansion bypassed decoded-size limit");
  fio_bstr_free(c.state.ws.msg);
  fio_deflate_destroy(rd);
  c.state.ws.msg = fio_bstr_write(NULL, "\x07", 1); /* reserved block type */
  msg = fio_bstr_buf(c.state.ws.msg);
  FIO_ASSERT(fio___websocket_deflate_transform(&c, &msg) ==
                 FIO_WEBSOCKET_CLOSE_INVALID_PAYLOAD,
             "corrupt compressed payload accepted");
  fio_bstr_free(c.state.ws.msg);
  fio_deflate_free(rd);
  fio_deflate_free(wr);
}

static int guard_timeout(void *a, void *b) {
  (void)a, (void)b;
  CHECK(0, "test timed out");
  fio_io_stop();
  return -1;
}

int main(void) {
  deflate_dependency_checks();
  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0",
                      .on_authenticate_websocket = srv_auth,
                      .on_open = srv_open,
                      .on_message = srv_message,
                      .compress_ws = 1);
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
  FIO_ASSERT(!failures, "%d WebSocket security check(s) failed", failures);
#if FIO_LEAK_COUNTER
  /* rejected upgrades must release the handle and any deflate context */
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio_deflate_s),
             "%zu deflate context(s) leaked",
             (size_t)FIO_LEAK_COUNTER_COUNT(fio_deflate_s));
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio_http),
             "%zu HTTP handle(s) leaked",
             (size_t)FIO_LEAK_COUNTER_COUNT(fio_http));
#endif
  fprintf(stderr, "\t* WebSocket security checks passed\n");
  return 0;
}
#endif
