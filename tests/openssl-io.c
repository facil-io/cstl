/* *****************************************************************************
Test - OpenSSL backend over real sockets (405 openssl.h)

Loopback regressions for the custom-BIO read path:
* TLS records coalesced with the peer's FIN are all delivered before EOF.
* A malformed handshake closes the connection promptly (no retry stall).
* A TLS client dispatches the server-selected ALPN protocol callback.
***************************************************************************** */
#define FIO_OPENSSL
#include "test-helpers.h"
#define FIO_IO
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL && defined(H___FIO_OPENSSL___H) && FIO_OS_POSIX
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>

#define FIO___TEST_OIO_ASSERT(cond, ...)                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fio_io_stop();                                                           \
      FIO_ASSERT(0, __VA_ARGS__);                                              \
    }                                                                          \
  } while (0)

#define FIO___TEST_OIO_RECORDS     10
#define FIO___TEST_OIO_RECORD_SIZE 100

static char fio___test_oio_url[64];
static uint16_t fio___test_oio_port;
static volatile size_t fio___test_oio_received;
static volatile int fio___test_oio_srv_closed;
static volatile int fio___test_oio_raw_done;   /* 1 = ok, -1 = failed */
static volatile int fio___test_oio_garbage_ok; /* 1 = EOF, -1 = stalled */
static volatile int fio___test_oio_cli_x, fio___test_oio_cli_y;
static volatile int fio___test_oio_srv_y;
static volatile int fio___test_oio_cli_closed;
static int fio___test_oio_step;
static int fio___test_oio_ticks;
static fio_io_protocol_s fio___test_oio_client_pr;

/* *****************************************************************************
Server / client protocols
***************************************************************************** */

static void fio___test_oio_srv_on_data(fio_io_s *io) {
  char buf[64]; /* small: forces many reads per socket read */
  size_t r;
  while ((r = fio_io_read(io, buf, sizeof(buf))))
    fio___test_oio_received += r;
}
static void fio___test_oio_srv_on_close(void *b, void *u) {
  ++fio___test_oio_srv_closed;
  (void)b, (void)u;
}
static void fio___test_oio_srv_alpn_y(fio_io_s *io) {
  ++fio___test_oio_srv_y;
  (void)io;
}
static void fio___test_oio_cli_alpn_x(fio_io_s *io) {
  ++fio___test_oio_cli_x;
  (void)io;
}
static void fio___test_oio_cli_alpn_y(fio_io_s *io) {
  ++fio___test_oio_cli_y;
  fio_io_close(io); /* ALPN dispatched: done */
}
static void fio___test_oio_cli_on_data(fio_io_s *io) {
  char buf[64];
  while (fio_io_read(io, buf, sizeof(buf)))
    ;
}
static void fio___test_oio_cli_on_close(void *b, void *u) {
  ++fio___test_oio_cli_closed;
  (void)b, (void)u;
}

/* *****************************************************************************
Raw (blocking) peer thread
***************************************************************************** */

static int fio___test_oio_connect(void) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return -1;
  struct sockaddr_in a = {0};
  a.sin_family = AF_INET;
  a.sin_port = htons(fio___test_oio_port);
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  struct timeval tv = {.tv_sec = 5};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  if (connect(fd, (struct sockaddr *)&a, sizeof(a))) {
    close(fd);
    return -1;
  }
  return fd;
}

/** Waits for the peer's EOF (1) or times out / errors (0). */
static int fio___test_oio_wait_eof(int fd) {
  char buf[4096];
  for (;;) {
    ssize_t r = recv(fd, buf, sizeof(buf), 0);
    if (r == 0)
      return 1;
    if (r < 0)
      return 0;
  }
}

/* Handshake, then deliver several records + close_notify + FIN at once. */
static int fio___test_oio_coalesced(void) {
  int ok = 0;
  int fd = fio___test_oio_connect();
  if (fd < 0)
    return 0;
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  SSL *ssl = ctx ? SSL_new(ctx) : NULL;
  if (!ssl || !SSL_set_fd(ssl, fd) || SSL_connect(ssl) != 1)
    goto finish;
  {
    BIO *mem = BIO_new(BIO_s_mem());
    if (!mem)
      goto finish;
    SSL_set0_wbio(ssl, mem); /* encrypt into memory, send in one write */
    char rec[FIO___TEST_OIO_RECORD_SIZE];
    FIO_MEMSET(rec, 'r', sizeof(rec));
    for (int i = 0; i < FIO___TEST_OIO_RECORDS; ++i)
      if (SSL_write(ssl, rec, (int)sizeof(rec)) != (int)sizeof(rec))
        goto finish;
    SSL_shutdown(ssl); /* close_notify */
    char *data = NULL;
    long len = BIO_get_mem_data(mem, &data);
    if (len <= 0 || send(fd, data, (size_t)len, 0) != (ssize_t)len)
      goto finish;
    shutdown(fd, SHUT_WR); /* FIN right behind the records */
  }
  ok = fio___test_oio_wait_eof(fd);
finish:
  if (ssl)
    SSL_free(ssl);
  if (ctx)
    SSL_CTX_free(ctx);
  close(fd);
  return ok;
}

/* Send a non-TLS "handshake" without FIN: the server must close promptly. */
static int fio___test_oio_garbage(void) {
  static const char req[] = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
  int fd = fio___test_oio_connect();
  if (fd < 0)
    return 0;
  int ok = (send(fd, req, sizeof(req) - 1, 0) == (ssize_t)(sizeof(req) - 1)) &&
           fio___test_oio_wait_eof(fd);
  close(fd);
  return ok;
}

static void *fio___test_oio_raw_thread(void *ignr_) {
  fio___test_oio_raw_done = fio___test_oio_coalesced() ? 1 : -1;
  fio___test_oio_garbage_ok = fio___test_oio_garbage() ? 1 : -1;
  return ignr_;
}

/* *****************************************************************************
Driver
***************************************************************************** */

static fio_io_tls_s *fio___test_oio_client_cfg;

static int fio___test_oio_driver(void *u1, void *u2) {
  (void)u1, (void)u2;
  FIO___TEST_OIO_ASSERT(++fio___test_oio_ticks < 1000,
                        "OpenSSL IO: timed out at step %d (1: client ALPN "
                        "not dispatched; 2: raw peer stalled)",
                        fio___test_oio_step);
  switch (fio___test_oio_step) {
  case 0: { /* reactor client with ALPN [x, y]; the server knows only y */
    fio_socket_i fd =
        fio_sock_open2(fio___test_oio_url,
                       FIO_SOCK_CLIENT | FIO_SOCK_TCP | FIO_SOCK_NONBLOCK);
    FIO___TEST_OIO_ASSERT(FIO_SOCK_FD_ISVALID(fd), "client connect failed");
    void *ctx = fio___test_oio_client_pr.io_functions.build_context(
        fio___test_oio_client_cfg,
        1);
    FIO___TEST_OIO_ASSERT(ctx, "client TLS context build failed");
    fio_io_attach_fd(fd, &fio___test_oio_client_pr, NULL, ctx); /* owns ctx */
    fio___test_oio_step = 1;
    return 0;
  }
  case 1:
    if (!fio___test_oio_cli_closed)
      return 0;
    fio___test_oio_step = 2;
    return 0;
  case 2:
    if (!fio___test_oio_raw_done || !fio___test_oio_garbage_ok)
      return 0;
    fio_io_stop();
    return -1;
  }
  return 0;
}

int main(void) {
  /* reserve a loopback port */
  {
    fio_socket_i r =
        fio_sock_open2("tcp://127.0.0.1:0", FIO_SOCK_SERVER | FIO_SOCK_TCP);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(r), "failed to reserve loopback port");
    struct sockaddr_in a = {0};
    socklen_t alen = sizeof(a);
    FIO_ASSERT(!getsockname(r, (struct sockaddr *)&a, &alen), "getsockname");
    fio_sock_close(r);
    fio___test_oio_port = ntohs(a.sin_port);
    snprintf(fio___test_oio_url,
             sizeof(fio___test_oio_url),
             "tcp://127.0.0.1:%u",
             (unsigned)fio___test_oio_port);
  }
  fio_io_functions_s fn = fio_openssl_io_functions();
  static fio_io_protocol_s server_pr;
  server_pr = (fio_io_protocol_s){
      .on_data = fio___test_oio_srv_on_data,
      .on_close = fio___test_oio_srv_on_close,
      .on_timeout = fio_io_touch,
      .io_functions = fn,
  };
  fio___test_oio_client_pr = (fio_io_protocol_s){
      .on_data = fio___test_oio_cli_on_data,
      .on_close = fio___test_oio_cli_on_close,
      .on_timeout = fio_io_touch,
      .io_functions = fn,
  };
  fio_io_tls_s *server_cfg = fio_io_tls_new();
  fio_io_tls_cert_add(server_cfg, "localhost", NULL, NULL, NULL);
  fio_io_tls_alpn_add(server_cfg, "y", fio___test_oio_srv_alpn_y);
  fio___test_oio_client_cfg = fio_io_tls_new(); /* no trust: no verify */
  fio_io_tls_alpn_add(fio___test_oio_client_cfg, "x", fio___test_oio_cli_alpn_x);
  fio_io_tls_alpn_add(fio___test_oio_client_cfg, "y", fio___test_oio_cli_alpn_y);

  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_WARNING); /* SECURITY: no verification */
  fio_io_listener_s *l = fio_io_listen(.url = fio___test_oio_url,
                                       .protocol = &server_pr,
                                       .tls = server_cfg,
                                       .hide_from_log = 1);
  FIO_ASSERT(l, "TLS listen failed");
  pthread_t raw;
  FIO_ASSERT(!pthread_create(&raw, NULL, fio___test_oio_raw_thread, NULL),
             "raw peer thread failed");
  fio_io_run_every(.fn = fio___test_oio_driver, .every = 10, .repetitions = -1);
  fio_io_start(0);
  pthread_join(raw, NULL);
  FIO_LOG_LEVEL_SET(log_level);
  fio_io_listen_stop(l);
  fio_io_tls_free(fio___test_oio_client_cfg); /* listen owned server_cfg */

  FIO_ASSERT(fio___test_oio_cli_y == 1 && !fio___test_oio_cli_x,
             "client ALPN callback not dispatched (x=%d, y=%d)",
             fio___test_oio_cli_x,
             fio___test_oio_cli_y);
  FIO_ASSERT(fio___test_oio_srv_y >= 1, "server ALPN not selected");
  fprintf(stderr, "* OpenSSL client ALPN dispatch: OK\n");
  FIO_ASSERT(fio___test_oio_raw_done == 1 &&
                 fio___test_oio_received ==
                     FIO___TEST_OIO_RECORDS * FIO___TEST_OIO_RECORD_SIZE,
             "records + FIN: received %zu of %d bytes",
             (size_t)fio___test_oio_received,
             FIO___TEST_OIO_RECORDS * FIO___TEST_OIO_RECORD_SIZE);
  fprintf(stderr, "* OpenSSL coalesced records + FIN: OK\n");
  FIO_ASSERT(fio___test_oio_garbage_ok == 1,
             "malformed handshake did not close promptly");
  fprintf(stderr, "* OpenSSL malformed handshake closes: OK\n");
  return 0;
}

#else
int main(void) {
  fprintf(stderr, "* OpenSSL IO tests skipped (no OpenSSL / POSIX)\n");
  return 0;
}
#endif
