/* Live HTTP/1.1 server test with TCP-fragmented requests.
 *
 * A raw blocking TCP client (separate thread, TCP_NODELAY) writes each
 * request in fragments with a short pause between writes, so the server's
 * read loop sees partial buffers that cannot be processed until more data
 * arrives. Splits cover the HttpArena `check_fragmented` cases (mid request
 * line, line by line, headers/body, body bytes) plus: mid method, mid header
 * name / value, between CR and LF, inside the final CRLFCRLF, chunked
 * framing (size line, data, CRLF, last chunk, trailers), Expect:
 * 100-continue, keep-alive pipelining with a fragment spanning two requests,
 * and a fragmented body large enough to use temporary file storage.
 *
 * The server describes what it parsed (method, path, query, a header, body
 * length and body or body hash) in its response; the client compares that
 * against the expected description. Requests use `Connection: close` and
 * the client reads until EOF (as HttpArena's validator does), which also
 * verifies the server closes the connection.
 *
 * Parser-only (no network) coverage: tests/http1-parser-fragmented.c
 * Run with: make stress/http-fragmented-roundtrip
 */
#define FIO_IO
#define FIO_HTTP
#include "../tests/test-helpers.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define FRAG_PAUSE_US      30000 /* between writes (HttpArena uses 30ms) */
#define FRAG_TIMEOUT_MS    30000 /* whole run */
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
  "POST /baseline11?a=13&b=42 HTTP/1.1\r\nHost: localhost\r\nContent-Type: "  \
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
     {"PO", "ST /p", "?q=", "1 HTT", "P/1.1", "\r", "\nHost: x\r\n",
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
  if (fd < 0 || connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
    error = "connect failed";
    goto done;
  }
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  for (size_t i = 0; i < FRAG_MAX_FRAGS && c->frags[i]; ++i) {
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

static void *frag_client_thread(void *ignr) {
  (void)ignr;
  for (size_t i = 0; i < FRAG_CASES; ++i)
    frag_run_case(frag_cases + i);
  __atomic_store_n(&frag_client.done, 1, __ATOMIC_RELEASE);
  return NULL;
}

/* *****************************************************************************
Reactor timers & main
***************************************************************************** */

static int frag_check_done(void *a, void *b) {
  (void)a, (void)b;
  if (!__atomic_load_n(&frag_client.done, __ATOMIC_ACQUIRE))
    return 0;
  fio_io_stop();
  return -1;
}

static volatile int frag_timed_out;
static int frag_timeout(void *a, void *b) {
  (void)a, (void)b;
  if (__atomic_load_n(&frag_client.done, __ATOMIC_ACQUIRE))
    return -1;
  frag_timed_out = 1;
  fio_io_stop();
  return -1;
}

int main(void) {
  static char large_cl[64];
  for (size_t i = 0; i < FRAG_LARGE_LEN; ++i)
    frag_large_body[i] = (char)('a' + ((i * 7 + (i >> 9)) % 26));
  snprintf(large_cl, sizeof(large_cl), "gth: %zu\r\n\r\n", (size_t)FRAG_LARGE_LEN);
  frag_cases[FRAG_CASES - 1].frags[1] = large_cl;
  snprintf(frag_large_expected,
           sizeof(frag_large_expected),
           "M=POST P=/large Q= X= T= L=%zu H=%016llx",
           (size_t)FRAG_LARGE_LEN,
           (unsigned long long)frag_hash(frag_large_body, FRAG_LARGE_LEN));

  fio_http_listener_s *l =
      fio_http_listen("tcp://127.0.0.1:0", .on_http = frag_server_on_http);
  if (!l) {
    fprintf(stderr, "http-fragmented-roundtrip: listener failed\n");
    return 1;
  }
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  if (getsockname(((fio___io_listen_s *)l)->fd,
                  (struct sockaddr *)&addr,
                  &addr_len)) {
    fprintf(stderr, "http-fragmented-roundtrip: getsockname failed\n");
    return 1;
  }
  frag_client.port = ntohs(addr.sin_port);

  pthread_t thread;
  if (pthread_create(&thread, NULL, frag_client_thread, NULL)) {
    fprintf(stderr, "http-fragmented-roundtrip: pthread_create failed\n");
    return 1;
  }
  fio_io_run_every(.fn = frag_check_done, .every = 20, .repetitions = -1);
  fio_io_run_every(.fn = frag_timeout,
                   .every = FRAG_TIMEOUT_MS,
                   .repetitions = 1);
  fio_io_start(0);
  fio_io_listen_stop((fio_io_listener_s *)l);
  if (frag_timed_out)
    fprintf(stderr, "http-fragmented-roundtrip: TIMEOUT (client stuck)\n");
  else
    pthread_join(thread, NULL);

  int bad = frag_timed_out || frag_client.failures ||
            frag_client.passed != FRAG_CASES;
  fprintf(stderr,
          "http-fragmented-roundtrip: %u/%zu passed, %u failed, server "
          "requests=%u -> %s\n",
          frag_client.passed,
          (size_t)FRAG_CASES,
          frag_client.failures,
          frag_server_calls,
          bad ? "FAILED" : "PASSED");
  return bad;
}
