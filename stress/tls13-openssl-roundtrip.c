/*****************************************************************************
Test: TLS 1.3 HTTP roundtrip with in-process fio TLS 1.3 server and OpenSSL
libssl client.

Replaces the external-process oracle tests tests-old/tls13-curl.c and
tests-old/tls13-openssl.c with a single-process, deterministic, library-only
roundtrip.  The facil.io side uses the native TLS 1.3 IO implementation
(fio_tls13_io_functions); the client uses OpenSSL's libssl directly from the
same process.

Without OpenSSL the test compiles to a no-op that exits 0.
***************************************************************************** */
#define FIO_SHA2
#define FIO_HKDF
#define FIO_AES
#define FIO_CHACHA
#define FIO_ED25519
#define FIO_P256
#define FIO_RSA
#define FIO_X509
#define FIO_IO
#define FIO_TLS13
#define FIO_HTTP
#if HAVE_OPENSSL
#define FIO_OPENSSL
#endif
#include "../tests/test-helpers.h"

#if HAVE_OPENSSL && FIO_OS_POSIX

#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/* *****************************************************************************
Shared state
***************************************************************************** */

typedef struct {
  fio_http_listener_s *listener;
  int port;
  int result;
  int done;
  pthread_t thread;
} fio___tls13_openssl_rt_state_s;

static fio___tls13_openssl_rt_state_s fio___tls13_openssl_rt_state = {0};
static int fio___tls13_openssl_rt_watchdog_fired = 0;

/* *****************************************************************************
HTTP server handler
***************************************************************************** */

static void fio___tls13_openssl_rt_on_http(fio_http_s *h) {
  fio_http_status_set(h, 200);
  fio_http_write(h, .buf = "OK", .len = 2, .finish = 1);
}

/* *****************************************************************************
OpenSSL libssl client
***************************************************************************** */

typedef struct {
  void *protocol;
  void *udata;
  void *tls_ctx;
  void *queue_for_accept;
  void *queue;
  void *io;
  void (*on_start)(void *protocol, void *udata);
  void (*on_stop)(void *protocol, void *udata);
  int owner;
  int fd;
  size_t ref_count;
  size_t url_len;
  uint8_t hide_from_log;
  char url[];
} fio___tls13_openssl_rt_listen_s;

static int fio___tls13_openssl_rt_listener_port(fio_http_listener_s *listener) {
  fio___tls13_openssl_rt_listen_s *l =
      (fio___tls13_openssl_rt_listen_s *)listener;
  struct sockaddr_in addr = {0};
  socklen_t len = sizeof(addr);
  if (getsockname(l->fd, (struct sockaddr *)&addr, &len) != 0 ||
      addr.sin_family != AF_INET)
    return -1;
  return ntohs(addr.sin_port);
}

static int fio___tls13_openssl_rt_socket_connect(const char *host, int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  FIO_ASSERT(fd >= 0, "client socket failed");

  struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons((uint16_t)port);
  if (inet_pton(AF_INET, host, &addr.sin_addr) != 1)
    FIO_ASSERT(0, "inet_pton failed");

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "client connect failed: %s\n", strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

static int fio___tls13_openssl_rt_client_run(const char *host, int port) {
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx)
    return -1;

  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
  SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
  SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

  static const unsigned char alpn[] = "\x8http/1.1";
  SSL_CTX_set_alpn_protos(ctx, alpn, sizeof(alpn) - 1);

  int fd = fio___tls13_openssl_rt_socket_connect(host, port);
  if (fd < 0) {
    SSL_CTX_free(ctx);
    return -1;
  }

  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }
  SSL_set_fd(ssl, fd);

  if (SSL_connect(ssl) <= 0) {
    int err = SSL_get_error(ssl, -1);
    fprintf(stderr, "SSL_connect failed: err=%d\n", err);
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }

  const char *version = SSL_get_version(ssl);
  if (!version || strcmp(version, "TLSv1.3") != 0) {
    fprintf(stderr,
            "TLS version mismatch: expected TLSv1.3, got %s\n",
            version ? version : "(null)");
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }

  const unsigned char *negotiated = NULL;
  unsigned int negotiated_len = 0;
  SSL_get0_alpn_selected(ssl, &negotiated, &negotiated_len);
  if (negotiated_len != 8 || FIO_MEMCMP(negotiated, "http/1.1", 8) != 0) {
    fprintf(stderr, "ALPN mismatch: expected http/1.1\n");
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }

  static const char request[] = "GET / HTTP/1.1\r\n"
                                "Host: localhost\r\n"
                                "Connection: close\r\n"
                                "\r\n";
  if (SSL_write(ssl, request, sizeof(request) - 1) <= 0) {
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }

  char response[4096];
  int total = 0;
  for (;;) {
    int n = SSL_read(ssl, response + total, (int)sizeof(response) - total - 1);
    if (n > 0) {
      total += n;
      continue;
    }
    break;
  }
  response[total] = 0;

  int ok = 1;
  if (total <= 0) {
    fprintf(stderr, "client received empty response\n");
    ok = 0;
  } else if (!strstr(response, "HTTP/1.1 200 OK")) {
    fprintf(stderr, "response status mismatch: %.*s\n", total, response);
    ok = 0;
  } else if (!strstr(response, "OK")) {
    fprintf(stderr, "response body mismatch: %.*s\n", total, response);
    ok = 0;
  }

  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(fd);
  SSL_CTX_free(ctx);
  return ok ? 0 : -1;
}

static void *fio___tls13_openssl_rt_thread(void *ignr_) {
  (void)ignr_;
  /* Give the listener a moment to start accepting. */
  usleep(100000);

  fio___tls13_openssl_rt_state.port = fio___tls13_openssl_rt_listener_port(
      fio___tls13_openssl_rt_state.listener);
  if (fio___tls13_openssl_rt_state.port > 0) {
    fio___tls13_openssl_rt_state.result =
        fio___tls13_openssl_rt_client_run("127.0.0.1",
                                          fio___tls13_openssl_rt_state.port);
  } else {
    fio___tls13_openssl_rt_state.result = -1;
  }
  fio___tls13_openssl_rt_state.done = 1;
  fio_io_stop();
  return NULL;
}

/* *****************************************************************************
Reactor callbacks
***************************************************************************** */

static void fio___tls13_openssl_rt_start_client(void *ignr_) {
  (void)ignr_;
  if (pthread_create(&fio___tls13_openssl_rt_state.thread,
                     NULL,
                     fio___tls13_openssl_rt_thread,
                     NULL) != 0) {
    fio___tls13_openssl_rt_state.result = -1;
    fio___tls13_openssl_rt_state.done = 1;
    fio_io_stop();
  }
}

static int fio___tls13_openssl_rt_watchdog(void *u1, void *u2) {
  (void)u1;
  (void)u2;
  if (!fio___tls13_openssl_rt_state.done) {
    fio___tls13_openssl_rt_watchdog_fired = 1;
    fio_io_stop();
  }
  return -1;
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr, "Testing TLS 1.3 HTTP roundtrip (fio TLS 1.3 + OpenSSL):\n");

  fio_io_functions_s tls13_funcs = fio_tls13_io_functions();
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls, "fio_io_tls_new failed");
  fio_io_tls_cert_add(tls, "localhost", NULL, NULL, NULL);
  fio_io_tls_alpn_add(tls, "http/1.1", NULL);

  fio___tls13_openssl_rt_state.listener =
      fio_http_listen("https://127.0.0.1:0",
                      .tls = tls,
                      .tls_io_func = &tls13_funcs,
                      .on_http = fio___tls13_openssl_rt_on_http,
                      .timeout = 5);
  fio_io_tls_free(tls);
  FIO_ASSERT(fio___tls13_openssl_rt_state.listener,
             "fio_http_listen failed on 127.0.0.1:0");

  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___tls13_openssl_rt_start_client,
                         NULL);
  fio_io_run_every(.fn = fio___tls13_openssl_rt_watchdog,
                   .every = 8000,
                   .repetitions = 1);

  fio_io_start(0);

  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___tls13_openssl_rt_start_client,
                            NULL);
  if (fio___tls13_openssl_rt_state.thread)
    pthread_join(fio___tls13_openssl_rt_state.thread, NULL);

  fio_io_listen_stop(
      (fio_io_listener_s *)fio___tls13_openssl_rt_state.listener);

  FIO_ASSERT(!fio___tls13_openssl_rt_watchdog_fired,
             "TLS 1.3 roundtrip timed out");
  FIO_ASSERT(fio___tls13_openssl_rt_state.done,
             "client thread did not complete");
  FIO_ASSERT(fio___tls13_openssl_rt_state.result == 0,
             "TLS 1.3 OpenSSL client roundtrip failed");

  fprintf(stderr, "TLS 1.3 HTTP roundtrip passed.\n");
  return 0;
}

#else /* !HAVE_OPENSSL || !FIO_OS_POSIX */

int main(void) {
  fprintf(stderr,
          "* TLS 1.3 OpenSSL roundtrip skipped (OpenSSL/POSIX unavailable).\n");
  return 0;
}

#endif /* HAVE_OPENSSL && FIO_OS_POSIX */
