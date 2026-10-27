/* *****************************************************************************
Test: HTTP accept / dispatch security regressions (434 http accept.h)

- ACC-01: a listener without a public folder never serves absolute paths for
  requests outside a folder-bearing route (route static service still works).
- ACC-02: a failed accept attach (`on_close(NULL, udata)`) doesn't crash.

A live single-thread reactor is driven by raw POSIX socket peers (a thread).
All checks are active under NDEBUG.
***************************************************************************** */
#include <stdlib.h>
#include <string.h>

#define FIO_HTTP
#include "test-helpers.h"

static int failures;
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
  fprintf(stderr, "\t* HTTP accept security checks require POSIX sockets\n");
  return 0;
}
#else
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define TEST_SECRET   "ACC011-TOP-SECRET"
#define TEST_PUBLIC   "ACC011-PUBLIC-FILE"

static unsigned static_port;
static fio_http_listener_s *static_listener;
static char secret_path[PATH_MAX];
static char secret_target[sizeof(secret_path) * 3]; /* percent-encoded */
static char pub_folder[PATH_MAX];

/* *****************************************************************************
Raw socket helpers (peer thread)
***************************************************************************** */

static unsigned port_of(int fd) {
  struct sockaddr_in ss;
  socklen_t slen = (socklen_t)sizeof(ss);
  FIO_ASSERT(!getsockname(fd, (struct sockaddr *)&ss, &slen), "getsockname");
  return ntohs(ss.sin_port);
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
  struct timeval tv = {.tv_sec = 3};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  if (connect(fd, (struct sockaddr *)&a, sizeof(a))) {
    close(fd);
    return -1;
  }
  return fd;
}

/** Sends `req`, reads until EOF / timeout / `stop`. Returns bytes read. */
static size_t roundtrip(unsigned port,
                        const char *req,
                        char *buf,
                        size_t cap,
                        const char *stop,
                        int *closed) {
  size_t len = 0;
  int fd = peer_connect(port);
  *closed = 0;
  buf[0] = 0;
  if (fd < 0)
    return 0;
  if (req && send(fd, req, strlen(req), MSG_NOSIGNAL) < 0) {
    close(fd);
    return 0;
  }
  while (len + 1 < cap) {
    ssize_t r = recv(fd, buf + len, cap - 1 - len, 0);
    if (r == 0 || (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
      *closed = 1;
      break;
    }
    if (r < 0)
      break; /* timeout */
    len += (size_t)r;
    buf[len] = 0;
    if (stop && strstr(buf, stop))
      break;
  }
  buf[len] = 0;
  close(fd);
  return len;
}

/* *****************************************************************************
Server callbacks
***************************************************************************** */

static void srv_on_http(fio_http_s *h) {
  fio_http_write(h, .buf = (char *)"app-answer", .len = 10, .finish = 1);
}

/* *****************************************************************************
Tests (peer thread)
***************************************************************************** */

static char rbuf[1 << 16];

static void test_static_root(void) {
  fprintf(stderr, "  * ACC-01 route-only public folder, absolute paths\n");
  static const char *methods[] = {"GET", "HEAD"};
  char req[sizeof(secret_target) + 64];
  int closed;
  for (size_t i = 0; i < 2; ++i) {
    snprintf(req,
             sizeof(req),
             "%s %s HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",
             methods[i],
             secret_target);
    roundtrip(static_port, req, rbuf, sizeof(rbuf), NULL, &closed);
    CHECK(!strstr(rbuf, TEST_SECRET), "%s disclosed %s", methods[i], rbuf);
    CHECK(!strncmp(rbuf, "HTTP/1.1 200", 12) &&
              (i || strstr(rbuf, "app-answer")),
          "%s not answered by the application:\n%s",
          methods[i],
          rbuf);
    CHECK(!strstr(rbuf, "content-length: 18") &&
              !strstr(rbuf, "Content-Length: 18"),
          "%s exposed the secret's length:\n%s",
          methods[i],
          rbuf);
  }
  /* route static service still works */
  roundtrip(static_port,
            "GET /pub/acc011-public.txt HTTP/1.1\r\nHost: x\r\n"
            "Connection: close\r\n\r\n",
            rbuf,
            sizeof(rbuf),
            TEST_PUBLIC,
            &closed);
  CHECK(strstr(rbuf, TEST_PUBLIC), "route static file not served:\n%s", rbuf);
}

static void attach_invalid_task(void *l_, void *ignr) {
  (void)ignr;
  fio_io_protocol_s *pr = fio_io_listener_protocol((fio_io_listener_s *)l_);
  /* attach failure schedules `on_close(NULL, NULL)` (no connection exists) */
  fio_io_attach_fd(FIO_SOCKET_INVALID, pr, NULL, NULL);
}

static void test_attach_failure(void) {
  fprintf(stderr, "  * ACC-02 failed accept attach (NULL connection)\n");
  for (int i = 0; i < 3; ++i)
    fio_io_defer(attach_invalid_task, (void *)static_listener, NULL);
  usleep(100000);
  int closed;
  roundtrip(static_port,
            "GET /x HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",
            rbuf,
            sizeof(rbuf),
            "app-answer",
            &closed);
  CHECK(strstr(rbuf, "app-answer"), "server unusable after attach failure");
}

static void *peer_main(void *ignr) {
  test_static_root();
  test_attach_failure();
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

/* Resolve into owned storage, then explicitly check the fixture buffer. */
static void resolve_fixture_path(char dst[PATH_MAX], const char *name) {
  char *resolved = realpath(name, NULL);
  FIO_ASSERT(resolved, "realpath failed for %s", name);
  size_t len = strlen(resolved);
  FIO_ASSERT(len < PATH_MAX, "fixture path too long: %s", name);
  memcpy(dst, resolved, len + 1);
  free(resolved);
}

static void write_file(const char *name, const char *data) {
  FILE *f = fopen(name, "wb");
  FIO_ASSERT(f, "couldn't create %s", name);
  FIO_ASSERT(fwrite(data, 1, strlen(data), f) == strlen(data), "write");
  fclose(f);
}

int main(void) {
  signal(SIGPIPE, SIG_IGN);
  const int log_level = FIO_LOG_LEVEL_GET();
  if (!getenv("ACC_VERBOSE"))
    FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected errors are logged */
  /* fixtures: a secret outside any public folder and a route folder */
  mkdir("tmp", 0700);
  mkdir("tmp/acc011-pub", 0700);
  write_file("tmp/acc011-secret.txt", TEST_SECRET);
  write_file("tmp/acc011-pub/acc011-public.txt", TEST_PUBLIC);
  resolve_fixture_path(secret_path, "tmp/acc011-secret.txt");
  resolve_fixture_path(pub_folder, "tmp/acc011-pub");
  { /* request-target form of the absolute path (paths may hold spaces) */
    char *t = secret_target;
    for (const char *s = secret_path; *s; ++s) {
      if (*s == '/' || (*s >= '0' && *s <= '9') || ((*s | 32) >= 'a' &&
                                                    (*s | 32) <= 'z') ||
          *s == '-' || *s == '.' || *s == '_') {
        *t++ = *s;
        continue;
      }
      t += snprintf(t, 4, "%%%02X", (unsigned)(unsigned char)*s);
    }
    *t = 0;
  }

  static_listener =
      fio_http_listen("tcp://127.0.0.1:0", .on_http = srv_on_http);
  FIO_ASSERT(static_listener, "fio_http_listen failed");
  FIO_ASSERT(!fio_http_route(static_listener,
                             "/pub",
                             .on_http = srv_on_http,
                             .public_folder = FIO_STR_INFO1(pub_folder)),
             "fio_http_route failed");
  static_port = listener_port(static_listener);

  fio_io_run_every(.fn = guard_timeout, .every = 60000, .repetitions = 1);
  pthread_t t;
  FIO_ASSERT(!pthread_create(&t, NULL, peer_main, NULL), "pthread_create");
  fio_io_start(0);
  pthread_join(t, NULL);
  FIO_LOG_LEVEL_SET(log_level);
  unlink("tmp/acc011-secret.txt");
  unlink("tmp/acc011-pub/acc011-public.txt");
  rmdir("tmp/acc011-pub");
  CHECK(!fio___leak_counter_fio_http(0),
        "%zu HTTP handle(s) leaked",
        fio___leak_counter_fio_http(0));
  CHECK(!fio___leak_counter_fio___io_connecting_s(0),
        "%zu IO connect state(s) leaked",
        fio___leak_counter_fio___io_connecting_s(0));
  CHECK(!fio___leak_counter_fio___http_connection(0),
        "%zu HTTP connection(s) leaked",
        fio___leak_counter_fio___http_connection(0));
  FIO_ASSERT(!failures, "%d HTTP accept security check(s) failed", failures);
  fprintf(stderr, "\t* HTTP accept security checks passed\n");
  return 0;
}
#endif
