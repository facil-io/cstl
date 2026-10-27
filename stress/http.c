/* Unified live HTTP stress/integration tests.
 *
 * Preserve POST body storage/seek coverage and fragmented raw TCP coverage;
 * independently decode our on-demand and cached static Brotli responses with
 * curl, and check identity negotiation. The reactor runs on the main thread;
 * a one-shot timer starts a blocking client thread (including curl processes).
 *
 * Run: make stress/http
 * Use CURL=/path/to/brotli-enabled/curl make stress/http to require curl
 * checks. Without CURL, missing curl/Brotli support is an explicitly reported
 * skip. All network traffic stays on loopback; temporary files stay in
 * ./ai-tmp. Parser-only coverage remains in tests/http1-parser-fragmented.c.
 */
#define FIO_IO
#define FIO_HTTP
#include "../tests/test-helpers.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define HTTP_TIMEOUT_MS 60000
static int http_abort; /* accessed with acquire/release atomics */

/* POST client/server body storage roundtrips. */
#define POST_RT_CHUNK 4093 /* odd: never aligned with storage sizes */
#define POST_RT_MAX   ((FIO_HTTP_BODY_RAM_LIMIT << 1) + 12345)

static char post_rt_payload[POST_RT_MAX];

static struct {
  size_t len;
  const char *name;
  unsigned server_calls, client_calls, client_status;
} post_rt_cases[] = {
    {1500, "small (RAM)"},
    {FIO_HTTP_BODY_RAM_LIMIT, "at RAM limit (RAM)"},
    {FIO_HTTP_BODY_RAM_LIMIT + 1, "RAM limit + 1 (temp file)"},
    {POST_RT_MAX, "large (temp file)"},
};
#define POST_RT_CASES (sizeof(post_rt_cases) / sizeof(post_rt_cases[0]))

static struct {
  unsigned failures, done;
} post_rt;

#define POST_RT_FAIL(fmt, ...)                                                 \
  do {                                                                         \
    fprintf(stderr, "http-post-roundtrip: FAIL " fmt "\n", __VA_ARGS__);       \
    ++post_rt.failures;                                                        \
  } while (0)

/** Verifies a read result against the payload at `offset`. */
static int post_rt_expect(const char *ctx,
                          const char *step,
                          fio_str_info_s r,
                          size_t offset,
                          size_t want_len) {
  if (r.len != want_len) {
    POST_RT_FAIL("%s: %s: read length %zu, expected %zu (offset %zu)",
                 ctx,
                 step,
                 r.len,
                 want_len,
                 offset);
    return -1;
  }
  if (want_len && (!r.buf || memcmp(r.buf, post_rt_payload + offset, r.len))) {
    size_t i = 0;
    while (r.buf && i < r.len && r.buf[i] == post_rt_payload[offset + i])
      ++i;
    POST_RT_FAIL("%s: %s: content mismatch at body offset %zu",
                 ctx,
                 step,
                 offset + i);
    return -1;
  }
  return 0;
}

static int post_rt_expect_pos(const char *ctx,
                              const char *step,
                              fio_http_s *h,
                              size_t want) {
  size_t pos = fio_http_body_pos(h);
  if (pos == want)
    return 0;
  POST_RT_FAIL("%s: %s: position %zu, expected %zu", ctx, step, pos, want);
  return -1;
}

/** Runs all body read checks. Returns the number of failures found. */
static unsigned post_rt_check_body(fio_http_s *h, const char *ctx, size_t len) {
  unsigned before = post_rt.failures;
  int want_fd = len > FIO_HTTP_BODY_RAM_LIMIT;

  if (fio_http_body_length(h) != len) {
    POST_RT_FAIL("%s: body length %zu, expected %zu",
                 ctx,
                 fio_http_body_length(h),
                 len);
    return post_rt.failures - before; /* nothing else is meaningful */
  }
  if ((fio_http_body_fd(h) != -1) != want_fd)
    POST_RT_FAIL("%s: storage is %s, expected %s",
                 ctx,
                 fio_http_body_fd(h) == -1 ? "RAM" : "temp file",
                 want_fd ? "temp file" : "RAM");

  /* 1. Sequential chunked reads */
  {
    size_t off = 0;
    fio_http_body_seek(h, 0);
    for (;;) {
      size_t want = len - off < POST_RT_CHUNK ? len - off : POST_RT_CHUNK;
      fio_str_info_s r = fio_http_body_read(h, POST_RT_CHUNK);
      if (post_rt_expect(ctx, "sequential read", r, off, want))
        break;
      off += r.len;
      if (post_rt_expect_pos(ctx, "sequential read", h, off) || !r.len)
        break;
    }
    if (off != len)
      POST_RT_FAIL("%s: sequential reads covered %zu of %zu", ctx, off, len);
  }

  /* 2. Rewinds */
  {
    size_t mid = len / 2;
    fio_http_body_seek(h, 0);
    post_rt_expect(ctx, "rewind to 0", fio_http_body_read(h, 17), 0, 17);
    post_rt_expect_pos(ctx, "rewind to 0", h, 17);
    fio_http_body_seek(h, (ssize_t)mid);
    post_rt_expect(ctx, "seek to middle", fio_http_body_read(h, 33), mid, 33);
    fio_http_body_seek(h, -25); /* -n == length - n */
    post_rt_expect(ctx,
                   "negative seek",
                   fio_http_body_read(h, 25),
                   len - 25,
                   25);
    post_rt_expect(ctx, "read at EOF", fio_http_body_read(h, 10), len, 0);
    if (fio_http_body_seek(h, -(ssize_t)(len + 10)) != 0)
      POST_RT_FAIL("%s: negative seek past start must clamp to 0", ctx);
    if (fio_http_body_seek(h, (ssize_t)(len + 10)) != len)
      POST_RT_FAIL("%s: seek past end must clamp to length", ctx);
    if (fio_http_body_seek(h, SSIZE_MAX) != len)
      POST_RT_FAIL("%s: seek(SSIZE_MAX) must seek to EOF", ctx);
    if (fio_http_body_seek(h, -1) != len - 1)
      POST_RT_FAIL("%s: seek(-1) must seek to the last byte", ctx);
    /* rewind after a partial read, then re-read the same bytes */
    fio_http_body_seek(h, (ssize_t)mid);
    fio_http_body_read(h, 100);
    fio_http_body_seek(h, (ssize_t)mid);
    post_rt_expect(ctx,
                   "re-read after rewind",
                   fio_http_body_read(h, 100),
                   mid,
                   100);
  }

  /* 3. Full memory dumps */
  {
    size_t third = len / 3;
    fio_http_body_seek(h, 0);
    post_rt_expect(ctx,
                   "full dump from start",
                   fio_http_body_read(h, (size_t)-1),
                   0,
                   len);
    post_rt_expect_pos(ctx, "full dump from start", h, len);
    fio_http_body_seek(h, (ssize_t)third);
    post_rt_expect(ctx,
                   "remainder dump from len/3",
                   fio_http_body_read(h, (size_t)-1),
                   third,
                   len - third);
    post_rt_expect_pos(ctx, "remainder dump from len/3", h, len);
  }
  return post_rt.failures - before;
}

static size_t post_rt_case_from_path(fio_http_s *h) {
  fio_str_info_s path = fio_http_path(h);
  if (path.len != 7 || memcmp(path.buf, "/post/", 6) ||
      (size_t)(path.buf[6] - '0') >= POST_RT_CASES)
    return (size_t)-1;
  return (size_t)(path.buf[6] - '0');
}

static void post_rt_server_on_http(fio_http_s *h) {
  size_t i = post_rt_case_from_path(h);
  if (i == (size_t)-1 || !fio_http_method_is_post(h)) {
    POST_RT_FAIL("server: unexpected request %s %s",
                 fio_http_method(h).buf,
                 fio_http_path(h).buf);
    fio_http_status_set(h, 400);
    fio_http_finish(h);
    return;
  }
  ++post_rt_cases[i].server_calls;
  char ctx[96];
  snprintf(ctx, sizeof(ctx), "server request [%s]", post_rt_cases[i].name);
  if (post_rt_check_body(h, ctx, post_rt_cases[i].len)) {
    fio_http_status_set(h, 500);
    fio_http_finish(h);
    return;
  }
  /* echo the expected payload (static memory, no copy needed) */
  fio_http_write(h,
                 .buf = post_rt_payload,
                 .len = post_rt_cases[i].len,
                 .finish = 1);
}

static void post_rt_client_on_http(fio_http_s *h) {
  size_t i = (size_t)(uintptr_t)fio_http_udata(h);
  if (i >= POST_RT_CASES) {
    POST_RT_FAIL("client: unknown case index %zu", i);
    fio_io_stop();
    return;
  }
  ++post_rt_cases[i].client_calls;
  post_rt_cases[i].client_status = (unsigned)fio_http_status(h);
  if (fio_http_status(h) == 200) {
    char ctx[96];
    snprintf(ctx, sizeof(ctx), "client response [%s]", post_rt_cases[i].name);
    post_rt_check_body(h, ctx, post_rt_cases[i].len);
  } else
    POST_RT_FAIL("client [%s]: status %zu, expected 200",
                 post_rt_cases[i].name,
                 (size_t)fio_http_status(h));
  ++post_rt.done;
}

/* Fragmented raw TCP requests. */
#define FRAG_PAUSE_US      30000 /* between writes (HttpArena uses 30ms) */
#define FRAG_RECV_TIMEOUT  5     /* seconds, per recv */
#define FRAG_LARGE_LEN     ((FIO_HTTP_BODY_RAM_LIMIT << 1) + 777)
#define FRAG_MAX_FRAGS     16
#define FRAG_MAX_RESPONSES 4
#define FRAG_BODY_ECHO_MAX 64 /* larger bodies are reported by hash */

/* *****************************************************************************
Shared helpers
***************************************************************************** */

static uint64_t frag_hash(const char *buf, size_t len) { /* FNV-1a */
  uint64_t h = 0xcbf29ce484222325ULL;
  for (size_t i = 0; i < len; ++i)
    h = (h ^ (uint8_t)buf[i]) * 0x100000001b3ULL;
  return h;
}

/** The server's description of a request; the client builds the same. */
static size_t frag_describe(char *dest,
                            size_t capa,
                            fio_str_info_s method,
                            fio_str_info_s path,
                            fio_str_info_s query,
                            fio_str_info_s xfrag,
                            fio_str_info_s xtrailer,
                            fio_str_info_s body) {
  int l;
  if (body.len <= FRAG_BODY_ECHO_MAX)
    l = snprintf(dest,
                 capa,
                 "M=%.*s P=%.*s Q=%.*s X=%.*s T=%.*s L=%zu B=%.*s",
                 (int)method.len,
                 method.buf,
                 (int)path.len,
                 path.buf,
                 (int)query.len,
                 query.buf,
                 (int)xfrag.len,
                 xfrag.buf,
                 (int)xtrailer.len,
                 xtrailer.buf,
                 body.len,
                 (int)body.len,
                 body.buf);
  else
    l = snprintf(dest,
                 capa,
                 "M=%.*s P=%.*s Q=%.*s X=%.*s T=%.*s L=%zu H=%016llx",
                 (int)method.len,
                 method.buf,
                 (int)path.len,
                 path.buf,
                 (int)query.len,
                 query.buf,
                 (int)xfrag.len,
                 xfrag.buf,
                 (int)xtrailer.len,
                 xtrailer.buf,
                 body.len,
                 (unsigned long long)frag_hash(body.buf, body.len));
  return (l < 0) ? 0 : ((size_t)l >= capa ? capa - 1 : (size_t)l);
}

/* *****************************************************************************
Server
***************************************************************************** */

static volatile unsigned frag_server_calls;

static void frag_server_on_http(fio_http_s *h) {
  char out[512];
  ++frag_server_calls;
  fio_http_body_seek(h, 0);
  fio_str_info_s body = fio_http_body_read(h, (size_t)-1);
  size_t len = frag_describe(
      out,
      sizeof(out),
      fio_http_method(h),
      fio_http_path(h),
      fio_http_query(h),
      fio_http_request_header(h, FIO_STR_INFO1((char *)"x-frag"), 0),
      fio_http_request_header(h, FIO_STR_INFO1((char *)"x-trailer"), 0),
      body);
  fio_http_response_header_set(h,
                               FIO_STR_INFO1((char *)"content-type"),
                               FIO_STR_INFO1((char *)"text/plain"));
  fio_http_write(h, .buf = out, .len = len, .copy = 1, .finish = 1);
}

/* *****************************************************************************
Test cases (fragments as written by the client, expected descriptions)
***************************************************************************** */

static char frag_large_body[FRAG_LARGE_LEN];
static char frag_large_expected[256];

typedef struct {
  const char *name;
  const char *frags[FRAG_MAX_FRAGS]; /* NULL terminated; "@LARGE[a,b)" */
  const char *expected[FRAG_MAX_RESPONSES];
} frag_case_s;

#define FRAG_POST_HDR(cl)                                                      \
  "POST /baseline11?a=13&b=42 HTTP/1.1\r\nHost: localhost\r\nContent-Type: "   \
  "text/plain\r\nContent-Length: " cl "\r\nConnection: close\r\n\r\n"

/* Large body fragments are written from `frag_large_body` (see below). */
#define FRAG_LARGE_MARK "\x01LARGE"

static frag_case_s frag_cases[] = {
    /* --- HttpArena validate.sh check_fragmented cases --- */
    {"split request line (HttpArena 1)",
     {"GET /baseli",
      "ne11?a=13&b=42 HTTP/1.1\r\n",
      "Host: localhost\r\nConnection: close\r\n\r\n"},
     {"M=GET P=/baseline11 Q=a=13&b=42 X= T= L=0 B="}},
    {"split before headers (HttpArena 2)",
     {"GET /baseline11?a=13&b=42 HTTP/1.1\r\n",
      "Host: localhost\r\n",
      "User-Agent: arena-frag/1.0\r\n",
      "Connection: close\r\n\r\n"},
     {"M=GET P=/baseline11 Q=a=13&b=42 X= T= L=0 B="}},
    {"split headers/body (HttpArena 3)",
     {FRAG_POST_HDR("2"), "20"},
     {"M=POST P=/baseline11 Q=a=13&b=42 X= T= L=2 B=20"}},
    {"split body bytes (HttpArena 4)",
     {FRAG_POST_HDR("2"), "2", "0"},
     {"M=POST P=/baseline11 Q=a=13&b=42 X= T= L=2 B=20"}},

    /* --- splits where nothing can be processed until more data arrives --- */
    {"split mid method / before any line ends",
     {"PO",
      "ST /p",
      "?q=",
      "1 HTT",
      "P/1.1",
      "\r",
      "\nHost: x\r\n",
      "Content-Length: 3\r\nConnection: close\r\n\r\nabc"},
     {"M=POST P=/p Q=q=1 X= T= L=3 B=abc"}},
    {"split header name, value, CR|LF and final CRLFCRLF",
     {"POST /h HTTP/1.1\r\nHost: x\r\nX-Fr",
      "ag:  split-",
      "value  \r",
      "\nContent-Len",
      "gth: 1",
      "1\r\nConnection: close\r",
      "\n\r",
      "\n",
      "hello",
      " world"},
     {"M=POST P=/h Q= X=split-value T= L=11 B=hello world"}},
    {"chunked framing (size line, data, CRLF, last chunk, trailer)",
     {"POST /chunked HTTP/1.1\r\nHost: x\r\nConnection: close\r\n"
      "Transfer-Encoding: chunked\r\n\r\n",
      "1",
      "A\r",
      "\nabcdefghij",
      "klmnopqrstuvwxyz\r",
      "\n3\r\nAB",
      "C",
      "\r\n0\r",
      "\nX-Trai",
      "ler: yes\r\n",
      "\r",
      "\n"},
     {"M=POST P=/chunked Q= X= T=yes L=29 B=abcdefghijklmnopqrstuvwxyzABC"}},
    {"Expect: 100-continue, body after pause",
     {"POST /expect HTTP/1.1\r\nHost: x\r\nExpect: 100-",
      "continue\r\nContent-Length: 4\r\nConnection: close\r\n\r\n",
      "da",
      "ta"},
     {"M=POST P=/expect Q= X= T= L=4 B=data"}},
    {"pipelined keep-alive, fragment spans both requests",
     {"POST /first HTTP/1.1\r\nHost: x\r\nX-Frag: one\r\nContent-Le",
      "ngth: 5\r\n\r\n12",
      "345GET /second?x=2 HT",
      "TP/1.1\r\nHost: x\r\nX-Frag: two\r\nConnection: close\r\n",
      "\r\n"},
     {"M=POST P=/first Q= X=one T= L=5 B=12345",
      "M=GET P=/second Q=x=2 X=two T= L=0 B="}},
    {"large body (temp file storage) in uneven fragments",
     {"POST /large HTTP/1.1\r\nHost: x\r\nConnection: close\r\nContent-Len",
      NULL /* filled at startup: "gth: <n>\r\n\r\n" */,
      FRAG_LARGE_MARK "0",
      FRAG_LARGE_MARK "1",
      FRAG_LARGE_MARK "2",
      FRAG_LARGE_MARK "3"},
     {frag_large_expected}},
};
#define FRAG_CASES (sizeof(frag_cases) / sizeof(frag_cases[0]))

/* large body is written as: 1 byte, 70000 bytes, 1 byte, remainder */
static void frag_large_slice(int part, const char **buf, size_t *len) {
  static const size_t starts[] = {0, 1, 70001, 70002, FRAG_LARGE_LEN};
  *buf = frag_large_body + starts[part];
  *len = starts[part + 1] - starts[part];
}

/* *****************************************************************************
Client (blocking sockets, separate thread)
***************************************************************************** */

static struct {
  unsigned short port;
  volatile int done;
  unsigned failures;
  unsigned passed;
} frag_client;

static int frag_write_all(int fd, const char *buf, size_t len) {
  while (len) {
    ssize_t w = send(fd, buf, len, 0);
    if (w <= 0)
      return -1;
    buf += w;
    len -= (size_t)w;
  }
  return 0;
}

/** Case-insensitive header lookup inside a header block. */
static int frag_header_value(const char *head,
                             size_t head_len,
                             const char *name,
                             const char **val,
                             size_t *val_len) {
  size_t nlen = strlen(name);
  const char *end = head + head_len;
  const char *line = memchr(head, '\n', head_len);
  while (line && ++line < end) {
    const char *eol = memchr(line, '\n', (size_t)(end - line));
    if (!eol)
      eol = end;
    if ((size_t)(eol - line) > nlen && line[nlen] == ':' &&
        !strncasecmp(line, name, nlen)) {
      const char *v = line + nlen + 1;
      while (v < eol && (*v == ' ' || *v == '\t'))
        ++v;
      const char *ve = eol;
      while (ve > v && (ve[-1] == '\r' || ve[-1] == ' '))
        --ve;
      *val = v;
      *val_len = (size_t)(ve - v);
      return 1;
    }
    line = eol;
  }
  return 0;
}

/** Splits a byte stream into final (non-1xx) response bodies. */
static size_t frag_parse_responses(char *buf,
                                   size_t len,
                                   fio_str_info_s *out,
                                   size_t max,
                                   const char **error) {
  size_t n = 0;
  *error = NULL;
  while (len) {
    char *hend = NULL;
    for (size_t i = 0; i + 3 < len; ++i)
      if (!memcmp(buf + i, "\r\n\r\n", 4)) {
        hend = buf + i + 4;
        break;
      }
    if (!hend || len < 12 || memcmp(buf, "HTTP/1.", 7)) {
      *error = "incomplete or malformed response head";
      return n;
    }
    size_t head_len = (size_t)(hend - buf);
    int status = atoi(buf + 9);
    const char *v;
    size_t vlen;
    fio_str_info_s body = FIO_STR_INFO2(hend, 0);
    size_t used = head_len;
    if (status >= 100 && status < 200) {
      /* interim (100 Continue) - skip */
    } else if (frag_header_value(buf,
                                 head_len,
                                 "transfer-encoding",
                                 &v,
                                 &vlen)) {
      /* chunked: decode in place */
      char *r = hend, *w = hend, *end = buf + len;
      for (;;) {
        char *nl = memchr(r, '\n', (size_t)(end - r));
        if (!nl) {
          *error = "truncated chunked response";
          return n;
        }
        size_t sz = strtoul(r, NULL, 16);
        r = nl + 1;
        if (!sz) {
          r += (r + 1 < end && r[0] == '\r') ? 2 : 0;
          break;
        }
        if ((size_t)(end - r) < sz + 2) {
          *error = "truncated chunked response";
          return n;
        }
        memmove(w, r, sz);
        w += sz;
        r += sz + 2;
      }
      body.len = (size_t)(w - hend);
      used = (size_t)(r - buf);
    } else if (frag_header_value(buf, head_len, "content-length", &v, &vlen)) {
      body.len = strtoul(v, NULL, 10);
      if (head_len + body.len > len) {
        *error = "truncated response body";
        return n;
      }
      used = head_len + body.len;
    } else {
      body.len = len - head_len; /* read until close */
      used = len;
    }
    if (status < 100 || status >= 200) {
      if (status != 200) {
        *error = "non-200 status";
        return n;
      }
      if (n < max)
        out[n] = body;
      ++n;
    }
    buf += used;
    len -= used;
  }
  return n;
}

static void frag_run_case(frag_case_s *c) {
  static char resp[1 << 16];
  size_t resp_len = 0;
  size_t sent = 0;
  const char *error = NULL;
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(frag_client.port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int one = 1;
  struct timeval tv = {.tv_sec = FRAG_RECV_TIMEOUT};
  if (fd < 0) {
    error = "socket failed";
    goto done;
  }
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
    error = "connect failed";
    goto done;
  }
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  for (size_t i = 0; i < FRAG_MAX_FRAGS && c->frags[i]; ++i) {
    if (__atomic_load_n(&http_abort, __ATOMIC_ACQUIRE)) {
      error = "test aborted";
      goto done;
    }
    const char *buf = c->frags[i];
    size_t len = strlen(buf);
    if (!strncmp(buf, FRAG_LARGE_MARK, sizeof(FRAG_LARGE_MARK) - 1))
      frag_large_slice(buf[sizeof(FRAG_LARGE_MARK) - 1] - '0', &buf, &len);
    if (i)
      usleep(FRAG_PAUSE_US);
    if (frag_write_all(fd, buf, len)) {
      error = "send failed";
      goto done;
    }
    sent += len;
  }
  for (;;) { /* read until the server closes the connection */
    if (__atomic_load_n(&http_abort, __ATOMIC_ACQUIRE)) {
      error = "test aborted";
      goto done;
    }
    if (resp_len == sizeof(resp)) {
      error = "response too long";
      goto done;
    }
    ssize_t r = recv(fd, resp + resp_len, sizeof(resp) - resp_len, 0);
    if (!r)
      break;
    if (r < 0) {
      error = "recv failed / timed out (server never closed?)";
      goto done;
    }
    resp_len += (size_t)r;
  }
  {
    fio_str_info_s bodies[FRAG_MAX_RESPONSES];
    size_t expected_n = 0;
    while (expected_n < FRAG_MAX_RESPONSES && c->expected[expected_n])
      ++expected_n;
    size_t n = frag_parse_responses(resp,
                                    resp_len,
                                    bodies,
                                    FRAG_MAX_RESPONSES,
                                    &error);
    if (error)
      goto done;
    if (n != expected_n) {
      error = "unexpected number of responses";
      goto done;
    }
    for (size_t i = 0; i < n; ++i) {
      size_t el = strlen(c->expected[i]);
      if (bodies[i].len != el || memcmp(bodies[i].buf, c->expected[i], el)) {
        fprintf(stderr,
                "  FAIL [%s] response %zu:\n    expected: %s\n    got:      "
                "%.*s\n",
                c->name,
                i + 1,
                c->expected[i],
                (int)bodies[i].len,
                bodies[i].buf);
        ++frag_client.failures;
        goto close_socket;
      }
    }
  }
done:
  if (error) {
    fprintf(stderr,
            "  FAIL [%s]: %s (sent %zu bytes, received %zu)\n",
            c->name,
            error,
            sent,
            resp_len);
    if (resp_len)
      fprintf(stderr, "    received: %.*s\n", (int)resp_len, resp);
    ++frag_client.failures;
  } else {
    fprintf(stderr, "  PASS [%s]\n", c->name);
    ++frag_client.passed;
  }
close_socket:
  if (fd >= 0)
    close(fd);
}

/* Independent static-file Brotli decoding with curl. */
extern char **environ;
static char http_curl_dir[128];
static char http_curl_source[192];
static char http_curl_br[sizeof(http_curl_source) + sizeof(".br") - 1];
static char http_curl_body[192], http_curl_headers[192], http_curl_version[192];
static const char *http_curl_program;
static unsigned http_curl_passed, http_curl_failures, http_curl_skipped;
static int http_curl_done;

/* Spawn without a shell or user curlrc. Reap every child, including timeouts.
 * posix_spawn avoids running allocator/library code in a forked reactor child.
 */
static int http_curl_spawn(char *const argv[], const char *capture) {
  posix_spawn_file_actions_t actions;
  pid_t pid = 0;
  int fd = -1, status = 0, result = -1;
  int rc = posix_spawn_file_actions_init(&actions);
  if (rc)
    return -1;
  if (capture) {
    fd = open(capture, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, 0600);
    if (fd < 0 ||
        posix_spawn_file_actions_adddup2(&actions, fd, STDOUT_FILENO) ||
        (fd != STDOUT_FILENO &&
         posix_spawn_file_actions_addclose(&actions, fd)))
      goto cleanup;
  }
  rc = posix_spawnp(&pid, http_curl_program, &actions, NULL, argv, environ);
  if (rc) {
    errno = rc;
    goto cleanup;
  }
  struct timespec start, now;
  if (clock_gettime(CLOCK_MONOTONIC, &start))
    goto kill_child;
  for (;;) {
    pid_t r = waitpid(pid, &status, WNOHANG);
    if (r == pid) {
      result = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
      goto cleanup;
    }
    if (r < 0 && errno != EINTR)
      goto kill_child;
    if (clock_gettime(CLOCK_MONOTONIC, &now) ||
        now.tv_sec - start.tv_sec >= 10 ||
        __atomic_load_n(&http_abort, __ATOMIC_ACQUIRE))
      goto kill_child;
    struct timespec pause = {.tv_nsec = 10000000};
    nanosleep(&pause, NULL);
  }
kill_child:
  kill(pid, SIGKILL);
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
  }
cleanup:
  if (fd >= 0)
    close(fd);
  posix_spawn_file_actions_destroy(&actions);
  return result;
}

static int http_curl_prepare(void) {
  if (mkdir("ai-tmp", 0700) && errno != EEXIST)
    return -1;
  snprintf(http_curl_dir, sizeof(http_curl_dir), "ai-tmp/http-stress-XXXXXX");
  if (!mkdtemp(http_curl_dir)) {
    http_curl_dir[0] = 0;
    return -1;
  }
  snprintf(http_curl_source,
           sizeof(http_curl_source),
           "%s/test.txt",
           http_curl_dir);
  snprintf(http_curl_br, sizeof(http_curl_br), "%s.br", http_curl_source);
  snprintf(http_curl_body, sizeof(http_curl_body), "%s/body", http_curl_dir);
  snprintf(http_curl_headers,
           sizeof(http_curl_headers),
           "%s/headers",
           http_curl_dir);
  snprintf(http_curl_version,
           sizeof(http_curl_version),
           "%s/version",
           http_curl_dir);
  FILE *f = fopen(http_curl_source, "wb");
  if (!f)
    return -1;
  const char line[] =
      "Live Brotli static-file interoperability: curl decodes our bytes.\n";
  int failed = 0;
  for (size_t i = 0; i < 512; ++i)
    if (fwrite(line, 1, sizeof(line) - 1, f) != sizeof(line) - 1) {
      failed = 1;
      break;
    }
  failed |= fclose(f) != 0;
  return failed ? -1 : 0;
}

static void http_curl_cleanup(void) {
  const char *paths[] = {http_curl_source,
                         http_curl_br,
                         http_curl_body,
                         http_curl_headers,
                         http_curl_version};
  for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
    if (paths[i][0])
      unlink(paths[i]);
  if (http_curl_dir[0])
    rmdir(http_curl_dir);
}

static int http_curl_body_equal(void) {
  FILE *expected = fopen(http_curl_source, "rb");
  FILE *actual = fopen(http_curl_body, "rb");
  int equal = 0;
  if (!expected || !actual)
    goto cleanup;
  for (;;) {
    char a[4096], b[4096];
    size_t n = fread(a, 1, sizeof(a), expected);
    size_t m = fread(b, 1, sizeof(b), actual);
    if (n != m || FIO_MEMCMP(a, b, n))
      break;
    if (n < sizeof(a)) {
      equal = !ferror(expected) && !ferror(actual);
      break;
    }
  }
cleanup:
  if (expected)
    fclose(expected);
  if (actual)
    fclose(actual);
  return equal;
}

static int http_curl_check_headers(int brotli) {
  char buf[8192];
  FILE *f = fopen(http_curl_headers, "rb");
  if (!f)
    return 0;
  size_t n = fread(buf, 1, sizeof(buf) - 1, f);
  int valid = !ferror(f) && feof(f);
  fclose(f);
  buf[n] = 0;
  if (!valid || n < 13 || FIO_MEMCMP(buf, "HTTP/1.1 200", 12) ||
      (buf[12] != ' ' && buf[12] != '\r'))
    return 0;
  const char *v;
  size_t len;
  int encoded = frag_header_value(buf, n, "content-encoding", &v, &len);
  if (brotli ? (!encoded || len != 2 || strncasecmp(v, "br", 2)) : encoded)
    return 0;
  if (!frag_header_value(buf, n, "vary", &v, &len))
    return 0;
  /* This static response sets one Vary token. */
  return len == 15 && !strncasecmp(v, "accept-encoding", 15);
}

static void http_curl_test(void) {
  const char *configured = getenv("CURL");
  int required = configured && configured[0];
  http_curl_program = required ? configured : "curl";
  char *version_argv[] = {(char *)http_curl_program, "-q", "--version", NULL};
  int rc = http_curl_spawn(version_argv, http_curl_version);
  char version[4096] = {0};
  FILE *f = fopen(http_curl_version, "rb");
  if (f) {
    size_t n = fread(version, 1, sizeof(version) - 1, f);
    if (ferror(f))
      rc = -1;
    version[n] = 0;
    fclose(f);
  }
  if (__atomic_load_n(&http_abort, __ATOMIC_ACQUIRE)) {
    fprintf(stderr, "http: FAIL curl probe aborted\n");
    ++http_curl_failures;
    return;
  }
  if (rc || !strstr(version, "brotli")) {
    fprintf(stderr,
            "http: %s curl Brotli checks (%s lacks Brotli or is unavailable)\n",
            required ? "FAIL" : "SKIP",
            http_curl_program);
    if (required)
      ++http_curl_failures;
    else
      ++http_curl_skipped;
    http_curl_done = 1;
    return;
  }
  fprintf(stderr,
          "http: independent decoder: %.*s\n",
          (int)strcspn(version, "\r\n"),
          version);
  char url[128];
  snprintf(url, sizeof(url), "http://127.0.0.1:%u/test.txt", frag_client.port);
  const char *names[] = {"on-demand br", "cached br", "identity"};
  struct stat first = {0};
  for (unsigned i = 0; i < 3; ++i) {
    if (__atomic_load_n(&http_abort, __ATOMIC_ACQUIRE)) {
      ++http_curl_failures;
      break;
    }
    struct stat st;
    if (!i && (!stat(http_curl_br, &st) || errno != ENOENT)) {
      fprintf(stderr, "http: FAIL unexpected existing .br sidecar\n");
      ++http_curl_failures;
      break;
    }
    char *argv[] = {(char *)http_curl_program,
                    "-q",
                    "--noproxy",
                    "*",
                    "--http1.1",
                    "--compressed",
                    "-H",
                    i < 2 ? "Accept-Encoding: br" : "Accept-Encoding: identity",
                    "--fail",
                    "--silent",
                    "--show-error",
                    "--connect-timeout",
                    "2",
                    "--max-time",
                    "5",
                    "--dump-header",
                    http_curl_headers,
                    "--output",
                    http_curl_body,
                    url,
                    NULL};
    unlink(http_curl_body);
    unlink(http_curl_headers);
    rc = http_curl_spawn(argv, NULL);
    int ok = !rc && http_curl_check_headers(i < 2) && http_curl_body_equal();
    if (!i) {
      struct stat source;
      if (stat(http_curl_br, &first) || stat(http_curl_source, &source) ||
          first.st_size <= 0 || first.st_size >= source.st_size)
        ok = 0;
    } else if (i == 1) {
      if (stat(http_curl_br, &st) || st.st_ino != first.st_ino ||
          st.st_size != first.st_size || st.st_mtime != first.st_mtime)
        ok = 0;
    }
    fprintf(stderr,
            "http: %s curl %s (exit %d)\n",
            ok ? "PASS" : "FAIL",
            names[i],
            rc);
    if (ok)
      ++http_curl_passed;
    else
      ++http_curl_failures;
  }
  http_curl_done = 1;
}

/* Shared server/reactor lifecycle. */
static void http_server_on_http(fio_http_s *h) {
  fio_str_info_s path = fio_http_path(h);
  if (path.len >= 6 && !FIO_MEMCMP(path.buf, "/post/", 6))
    post_rt_server_on_http(h);
  else
    frag_server_on_http(h);
}

static pthread_t http_client;
static int http_client_started;
static int http_timed_out;

static void *http_client_thread(void *arg) {
  (void)arg;
  for (size_t i = 0; i < FRAG_CASES; ++i) {
    if (__atomic_load_n(&http_abort, __ATOMIC_ACQUIRE))
      break;
    frag_run_case(frag_cases + i);
  }
  if (!__atomic_load_n(&http_abort, __ATOMIC_ACQUIRE))
    http_curl_test();
  __atomic_store_n(&frag_client.done, 1, __ATOMIC_RELEASE);
  return NULL;
}

static int http_start_client(void *a, void *b) {
  (void)a, (void)b;
  if (pthread_create(&http_client, NULL, http_client_thread, NULL)) {
    fprintf(stderr, "http: client thread creation failed\n");
    ++frag_client.failures;
    __atomic_store_n(&frag_client.done, 1, __ATOMIC_RELEASE);
  } else {
    http_client_started = 1;
  }
  return -1;
}

static int http_check_done(void *a, void *b) {
  (void)a, (void)b;
  if (!__atomic_load_n(&frag_client.done, __ATOMIC_ACQUIRE) ||
      post_rt.done != POST_RT_CASES)
    return 0;
  fio_io_stop();
  return -1;
}

static int http_timeout(void *a, void *b) {
  (void)a, (void)b;
  http_timed_out = 1;
  __atomic_store_n(&http_abort, 1, __ATOMIC_RELEASE);
  fio_io_stop();
  return -1;
}

int main(void) {
  int bad = 0;
  fio_http_listener_s *l = NULL;
  for (size_t i = 0; i < POST_RT_MAX; ++i)
    post_rt_payload[i] = (char)((i * 131U + (i >> 8) * 7U + (i >> 16)) & 255U);
  static char large_cl[64];
  for (size_t i = 0; i < FRAG_LARGE_LEN; ++i)
    frag_large_body[i] = (char)('a' + ((i * 7 + (i >> 9)) % 26));
  snprintf(large_cl,
           sizeof(large_cl),
           "gth: %zu\r\n\r\n",
           (size_t)FRAG_LARGE_LEN);
  frag_cases[FRAG_CASES - 1].frags[1] = large_cl;
  snprintf(frag_large_expected,
           sizeof(frag_large_expected),
           "M=POST P=/large Q= X= T= L=%zu H=%016llx",
           (size_t)FRAG_LARGE_LEN,
           (unsigned long long)frag_hash(frag_large_body, FRAG_LARGE_LEN));

  if (http_curl_prepare()) {
    fprintf(stderr, "http: static fixture setup failed: %s\n", strerror(errno));
    bad = 1;
    goto cleanup;
  }
  l = fio_http_listen("tcp://127.0.0.1:0",
                      .on_http = http_server_on_http,
                      .public_folder = FIO_STR_INFO1(http_curl_dir),
                      .compress_static = 1);
  if (!l) {
    fprintf(stderr, "http: listener failed\n");
    bad = 1;
    goto cleanup;
  }
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  if (getsockname(((fio___io_listen_s *)l)->fd,
                  (struct sockaddr *)&addr,
                  &addr_len)) {
    fprintf(stderr, "http: getsockname failed\n");
    bad = 1;
    goto cleanup;
  }
  /* No public bound-port getter exists. */
  frag_client.port = ntohs(addr.sin_port);
  for (size_t i = 0; i < POST_RT_CASES; ++i) {
    char url[96];
    snprintf(url,
             sizeof(url),
             "http://127.0.0.1:%u/post/%zu",
             (unsigned)frag_client.port,
             i);
    fio_http_s *h = fio_http_new();
    fio_http_method_set(h, FIO_STR_INFO2((char *)"POST", 4));
    fio_http_udata_set(h, (void *)(uintptr_t)i);
    fio_http_body_write(h, post_rt_payload, post_rt_cases[i].len);
    /* client side storage must match too (exercises both send paths) */
    char ctx[96];
    snprintf(ctx, sizeof(ctx), "client request [%s]", post_rt_cases[i].name);
    post_rt_check_body(h, ctx, post_rt_cases[i].len);
    if (!fio_http_connect(url,
                          h,
                          .on_http = post_rt_client_on_http,
                          .udata = (void *)(uintptr_t)i)) {
      fprintf(stderr, "http-post-roundtrip: connect failed (%s)\n", url);
      bad = 1;
      goto cleanup;
    }
  }

  fio_io_run_every(.fn = http_start_client, .every = 1, .repetitions = 1);
  fio_io_run_every(.fn = http_check_done, .every = 20, .repetitions = -1);
  fio_io_run_every(.fn = http_timeout,
                   .every = HTTP_TIMEOUT_MS,
                   .repetitions = 1);
  fio_io_start(0);
  __atomic_store_n(&http_abort, 1, __ATOMIC_RELEASE);
  if (http_client_started)
    pthread_join(http_client, NULL);
  bad |= http_timed_out || frag_client.failures ||
         frag_client.passed != FRAG_CASES || http_curl_failures ||
         !http_curl_done || (!http_curl_skipped && http_curl_passed != 3);
  bad |= post_rt.failures != 0;
  for (size_t i = 0; i < POST_RT_CASES; ++i) {
    fprintf(stderr,
            "http-post-roundtrip: %-26s %7zu bytes: server=%u client=%u "
            "status=%u\n",
            post_rt_cases[i].name,
            post_rt_cases[i].len,
            post_rt_cases[i].server_calls,
            post_rt_cases[i].client_calls,
            post_rt_cases[i].client_status);
    bad |= post_rt_cases[i].server_calls != 1 ||
           post_rt_cases[i].client_calls != 1 ||
           post_rt_cases[i].client_status != 200;
  }
  fprintf(stderr,
          "http: fragmented=%u/%zu curl=%u/%u%s timeout=%d -> %s\n",
          frag_client.passed,
          (size_t)FRAG_CASES,
          http_curl_passed,
          http_curl_passed + http_curl_failures,
          http_curl_skipped ? " (curl skipped)" : "",
          http_timed_out,
          bad ? "FAILED" : "PASSED");
cleanup:
  if (l)
    fio_io_listen_stop((fio_io_listener_s *)l);
  http_curl_cleanup();
  return bad != 0;
}
