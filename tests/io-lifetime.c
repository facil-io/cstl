/* *****************************************************************************
Test - IO reactor lifetime regressions (403 io reactor)

* `fio_io_workers(INT_MIN)` is defined (no signed overflow).
* Overlong listen URLs (argument or `ADDRESS`) fail; never another address.
* A listener stopped before its deferred attach never attaches or leaks.
* A listener surviving a reactor restart accepts in the next cycle without
  dispatching a stale (freed) IO: its dup'd fd must leave epoll before close.
* An accepted TLS connection outlives its stopped listener: its handshake
  must not read the listener's (released) TLS context. Native TLS 1.3 and,
  when available, OpenSSL.
***************************************************************************** */
#include "test-helpers.h"

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
#include FIO_INCLUDE_FILE

#include <limits.h>

#define FIO___TEST_LT_ASSERT(cond, ...)                                        \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fio_io_stop();                                                           \
      FIO_ASSERT(0, __VA_ARGS__);                                              \
    }                                                                          \
  } while (0)

/* *****************************************************************************
Worker count / URL validation (no reactor)
***************************************************************************** */

static void test_lt_workers_int_min(void) {
  uint16_t w = fio_io_workers(INT_MIN);
  FIO_ASSERT(w >= 1 && w <= 0x0FFF, "workers(INT_MIN) out of range: %u", w);
  w = fio_io_workers(INT_MIN + 1);
  FIO_ASSERT(w >= 1 && w <= 0x0FFF, "workers(INT_MIN+1) out of range: %u", w);
  FIO_ASSERT(fio_io_workers(INT_MAX) == 0x0FFF, "workers(INT_MAX) not capped");
  fprintf(stderr, "* fio_io_workers(INT_MIN) defined: OK\n");
}

static fio_io_protocol_s fio___test_lt_plain_protocol;

static void test_lt_overlong_url(void) {
  char url[2100];
  FIO_MEMSET(url, 'a', sizeof(url) - 1);
  url[sizeof(url) - 1] = 0;
  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
  fio_io_listener_s *l =
      fio_io_listen(.url = url, .protocol = &fio___test_lt_plain_protocol);
  FIO_LOG_LEVEL_SET(log_level);
  FIO_ASSERT(!l, "overlong listen URL must fail, not fall back");
#if FIO_OS_POSIX
  /* an overlong `ADDRESS` used to loop forever */
  FIO_ASSERT(!setenv("ADDRESS", url, 1), "setenv failed");
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
  l = fio_io_listen(.protocol = &fio___test_lt_plain_protocol);
  FIO_LOG_LEVEL_SET(log_level);
  unsetenv("ADDRESS");
  FIO_ASSERT(!l, "overlong ADDRESS must fail, not loop or fall back");
#endif
  fprintf(stderr, "* overlong listen URL rejected: OK\n");
}

/* *****************************************************************************
Reactor helpers
***************************************************************************** */

static void fio___test_lt_reserve_url(char *dest, size_t len) {
  fio_socket_i reserve =
      fio_sock_open2("tcp://127.0.0.1:0", FIO_SOCK_SERVER | FIO_SOCK_TCP);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(reserve), "failed to reserve loopback port");
  struct sockaddr_in address = {0};
  socklen_t address_len = sizeof(address);
  FIO_ASSERT(!getsockname(reserve, (struct sockaddr *)&address, &address_len),
             "getsockname failed while reserving loopback port");
  fio_sock_close(reserve);
  snprintf(dest,
           len,
           "tcp://127.0.0.1:%u",
           (unsigned)ntohs(address.sin_port));
}

/* *****************************************************************************
Listener stopped before its deferred attach
***************************************************************************** */

static volatile int fio___test_lt_stop_started = 0;
static volatile int fio___test_lt_stop_stopped = 0;
static volatile int fio___test_lt_stop_ticks = 0;

static void fio___test_lt_stop_on_start(fio_io_protocol_s *pr, void *u) {
  ++fio___test_lt_stop_started;
  (void)pr, (void)u;
}
static void fio___test_lt_stop_on_stop(fio_io_protocol_s *pr, void *u) {
  ++fio___test_lt_stop_stopped;
  (void)pr, (void)u;
}

static int fio___test_lt_stop_driver(void *u1, void *u2) {
  (void)u1, (void)u2;
  if (++fio___test_lt_stop_ticks == 1) {
    char url[64];
    fio___test_lt_reserve_url(url, sizeof(url));
    fio_io_listener_s *l =
        fio_io_listen(.url = url,
                      .protocol = &fio___test_lt_plain_protocol,
                      .on_start = fio___test_lt_stop_on_start,
                      .on_stop = fio___test_lt_stop_on_stop,
                      .hide_from_log = 1);
    FIO___TEST_LT_ASSERT(l, "listen while running failed");
    fio_io_listen_stop(l); /* before the deferred attach task runs */
    return 0;
  }
  if (fio___test_lt_stop_ticks < 5)
    return 0;
  fio_io_stop();
  return -1;
}

static void test_lt_listen_stop_before_attach(void) {
  fio_io_run_every(.fn = fio___test_lt_stop_driver,
                   .every = 5,
                   .repetitions = -1);
  fio_io_start(0);
  FIO_ASSERT(!fio___test_lt_stop_started,
             "stopped listener must not attach (on_start called)");
  FIO_ASSERT(fio___test_lt_stop_stopped == 1,
             "stopped listener should call on_stop once (%d)",
             fio___test_lt_stop_stopped);
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio_io_listen),
             "stopped listener leaked");
  fprintf(stderr, "* listener stopped before deferred attach: OK\n");
}

/* *****************************************************************************
Listener survives reactor restart (epoll: forget before close on dup'd fds)
***************************************************************************** */

static fio_io_protocol_s fio___test_lt_rs_protocol;
static char fio___test_lt_rs_url[64];
static volatile int fio___test_lt_rs_accepted = 0;
static volatile int fio___test_lt_rs_ticks = 0;
static fio_socket_i fio___test_lt_rs_clients[2] = {FIO_SOCKET_INVALID,
                                                   FIO_SOCKET_INVALID};

static void fio___test_lt_rs_on_attach(fio_io_s *io) {
  ++fio___test_lt_rs_accepted;
  (void)io;
}

static int fio___test_lt_rs_driver(void *u1, void *u2) {
  size_t cycle = (size_t)u1;
  (void)u2;
  if (++fio___test_lt_rs_ticks == 1) {
    fio___test_lt_rs_clients[cycle] =
        fio_sock_open2(fio___test_lt_rs_url, FIO_SOCK_CLIENT | FIO_SOCK_TCP);
    FIO___TEST_LT_ASSERT(FIO_SOCK_FD_ISVALID(fio___test_lt_rs_clients[cycle]),
                         "restart test client connect failed");
    return 0;
  }
  if (fio___test_lt_rs_accepted <= (int)cycle &&
      fio___test_lt_rs_ticks < 400)
    return 0;
  fio_io_stop();
  return -1;
}

static void test_lt_listener_survives_restart(void) {
  fio___test_lt_rs_protocol = (fio_io_protocol_s){
      .on_attach = fio___test_lt_rs_on_attach,
      .on_timeout = fio_io_touch,
  };
  fio___test_lt_reserve_url(fio___test_lt_rs_url,
                            sizeof(fio___test_lt_rs_url));
  fio_io_listener_s *l = fio_io_listen(.url = fio___test_lt_rs_url,
                                       .protocol = &fio___test_lt_rs_protocol,
                                       .hide_from_log = 1);
  FIO_ASSERT(l, "restart test listen failed");
  /* hold the fd numbers freed by cycle 0, so cycle 1 re-dups the listener to
   * a new number: a stale epoll entry (old number, same file) then survives */
  int placeholders[16];
  for (size_t i = 0; i < 16; ++i)
    placeholders[i] = -1;
  for (size_t cycle = 0; cycle < 2; ++cycle) {
    if (cycle)
      for (size_t i = 0; i < 16; ++i)
        placeholders[i] = dup(STDERR_FILENO);
    fio___test_lt_rs_ticks = 0;
    fio_io_run_every(.fn = fio___test_lt_rs_driver,
                     .udata1 = (void *)cycle,
                     .every = 5,
                     .repetitions = -1);
    fio_io_start(0);
    FIO_ASSERT(fio___test_lt_rs_accepted == (int)cycle + 1,
               "restart cycle %zu: listener accepted %d connections",
               cycle,
               fio___test_lt_rs_accepted);
  }
  fio_io_listen_stop(l);
  for (size_t i = 0; i < 16; ++i)
    if (placeholders[i] != -1)
      close(placeholders[i]);
  for (size_t i = 0; i < 2; ++i)
    if (FIO_SOCK_FD_ISVALID(fio___test_lt_rs_clients[i]))
      fio_sock_close(fio___test_lt_rs_clients[i]);
  fprintf(stderr, "* listener survives reactor restart: OK\n");
}

/* *****************************************************************************
Accepted TLS connection outlives its stopped listener
***************************************************************************** */

static fio_io_protocol_s fio___test_lt_tls_server;
static fio_io_protocol_s fio___test_lt_tls_client;
static fio_io_listener_s *fio___test_lt_tls_listener;
static fio_io_tls_s *fio___test_lt_tls_client_cfg;
static fio_socket_i fio___test_lt_tls_raw = FIO_SOCKET_INVALID;
static char fio___test_lt_tls_url[64];
static volatile int fio___test_lt_tls_accepted = 0;
static volatile int fio___test_lt_tls_received = 0;
static volatile int fio___test_lt_tls_closed = 0;
static volatile int fio___test_lt_tls_step = 0;
static volatile int fio___test_lt_tls_ticks = 0;
static volatile int fio___test_lt_tls_wait = 0;
static fio_io_s *fio___test_lt_tls_ios[2];

static void fio___test_lt_tls_server_on_attach(fio_io_s *io) {
  fio___test_lt_tls_ios[0] = fio_io_dup(io);
  ++fio___test_lt_tls_accepted;
}
static void fio___test_lt_tls_server_on_data(fio_io_s *io) {
  char buf[64];
  size_t r;
  while ((r = fio_io_read(io, buf, sizeof(buf))))
    fio___test_lt_tls_received += (int)r;
}
static void fio___test_lt_tls_client_on_attach(fio_io_s *io) {
  fio___test_lt_tls_ios[1] = fio_io_dup(io);
  fio_io_write(io, "ping", 4);
}
static void fio___test_lt_tls_client_on_data(fio_io_s *io) {
  char buf[64];
  while (fio_io_read(io, buf, sizeof(buf)))
    ;
}
static void fio___test_lt_tls_on_close(void *buffer, void *udata) {
  ++fio___test_lt_tls_closed;
  (void)buffer, (void)udata;
}

static int fio___test_lt_tls_driver(void *u1, void *u2) {
  (void)u1, (void)u2;
  FIO___TEST_LT_ASSERT(++fio___test_lt_tls_ticks < 500,
                       "TLS lifetime: timed out at step %d (accepted %d, "
                       "received %d, closed %d)",
                       fio___test_lt_tls_step,
                       fio___test_lt_tls_accepted,
                       fio___test_lt_tls_received,
                       fio___test_lt_tls_closed);
  switch (fio___test_lt_tls_step) {
  case 0: /* raw TCP connect: the server starts TLS, the client stays silent */
    fio___test_lt_tls_raw =
        fio_sock_open2(fio___test_lt_tls_url,
                       FIO_SOCK_CLIENT | FIO_SOCK_TCP | FIO_SOCK_NONBLOCK);
    FIO___TEST_LT_ASSERT(FIO_SOCK_FD_ISVALID(fio___test_lt_tls_raw),
                         "raw client connect failed");
    fio___test_lt_tls_step = 1;
    return 0;
  case 1: /* accepted: stop the listener, releasing its TLS context */
    if (!fio___test_lt_tls_accepted)
      return 0;
    fio_io_listen_stop(fio___test_lt_tls_listener);
    fio___test_lt_tls_listener = NULL;
    fio___test_lt_tls_step = 2;
    return 0;
  case 2: /* let the context release tasks run, then handshake */
    if (++fio___test_lt_tls_wait < 5)
      return 0;
    {
      void *ctx = fio___test_lt_tls_client.io_functions.build_context(
          fio___test_lt_tls_client_cfg,
          1);
      FIO___TEST_LT_ASSERT(ctx, "client TLS context build failed");
      fio_io_attach_fd(fio___test_lt_tls_raw,
                       &fio___test_lt_tls_client,
                       NULL,
                       ctx);
      fio___test_lt_tls_raw = FIO_SOCKET_INVALID; /* `ctx` transferred too */
    }
    fio___test_lt_tls_step = 3;
    return 0;
  case 3:
    if (fio___test_lt_tls_received < 4)
      return 0;
    for (size_t i = 0; i < 2; ++i) {
      if (!fio___test_lt_tls_ios[i])
        continue;
      fio_io_close(fio___test_lt_tls_ios[i]);
      fio_io_free(fio___test_lt_tls_ios[i]);
      fio___test_lt_tls_ios[i] = NULL;
    }
    fio___test_lt_tls_step = 4;
    return 0;
  case 4:
    if (fio___test_lt_tls_closed < 2)
      return 0;
    fio_io_stop();
    return -1;
  }
  return 0;
}

static void test_lt_tls_outlives_listener(const char *name,
                                          fio_io_functions_s fn) {
  fio___test_lt_tls_accepted = fio___test_lt_tls_received = 0;
  fio___test_lt_tls_closed = fio___test_lt_tls_step = 0;
  fio___test_lt_tls_ticks = fio___test_lt_tls_wait = 0;
  fio___test_lt_tls_ios[0] = fio___test_lt_tls_ios[1] = NULL;
  fio___test_lt_tls_server = (fio_io_protocol_s){
      .on_attach = fio___test_lt_tls_server_on_attach,
      .on_data = fio___test_lt_tls_server_on_data,
      .on_close = fio___test_lt_tls_on_close,
      .on_timeout = fio_io_touch,
      .io_functions = fn,
  };
  fio___test_lt_tls_client = (fio_io_protocol_s){
      .on_attach = fio___test_lt_tls_client_on_attach,
      .on_data = fio___test_lt_tls_client_on_data,
      .on_close = fio___test_lt_tls_on_close,
      .on_timeout = fio_io_touch,
      .io_functions = fn,
  };
  fio___test_lt_reserve_url(fio___test_lt_tls_url,
                            sizeof(fio___test_lt_tls_url));
  fio_io_tls_s *server_cfg = fio_io_tls_new();
  fio_io_tls_cert_add(server_cfg, "localhost", NULL, NULL, NULL);
  fio___test_lt_tls_client_cfg = fio_io_tls_new(); /* no trust: no verify */
  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_WARNING); /* SECURITY: no verification */
  fio___test_lt_tls_listener = fio_io_listen(.url = fio___test_lt_tls_url,
                                             .protocol =
                                                 &fio___test_lt_tls_server,
                                             .tls = server_cfg,
                                             .hide_from_log = 1);
  FIO_ASSERT(fio___test_lt_tls_listener, "TLS listen failed");
  fio_io_run_every(.fn = fio___test_lt_tls_driver,
                   .every = 10,
                   .repetitions = -1);
  fio_io_start(0);
  FIO_LOG_LEVEL_SET(log_level);
  if (fio___test_lt_tls_listener)
    fio_io_listen_stop(fio___test_lt_tls_listener);
  if (FIO_SOCK_FD_ISVALID(fio___test_lt_tls_raw))
    fio_sock_close(fio___test_lt_tls_raw);
  fio_io_tls_free(fio___test_lt_tls_client_cfg); /* listen owned server_cfg */
  FIO_ASSERT(fio___test_lt_tls_received >= 4,
             "%s: handshake after listener stop failed",
             name);
  fprintf(stderr, "* %s: accepted TLS outlives stopped listener: OK\n", name);
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  fio___test_lt_plain_protocol = (fio_io_protocol_s){
      .on_timeout = fio_io_touch,
  };
  test_lt_workers_int_min();
  test_lt_overlong_url();
#if FIO_OS_POSIX
  test_lt_listen_stop_before_attach();
  test_lt_listener_survives_restart();
  test_lt_tls_outlives_listener("TLS 1.3 (native)", fio_tls13_io_functions());
#if defined(H___FIO_OPENSSL___H) && HAVE_OPENSSL
  test_lt_tls_outlives_listener("OpenSSL", fio_openssl_io_functions());
#endif
#else
  fprintf(stderr, "* IO lifetime reactor tests skipped on non-POSIX\n");
#endif
  return 0;
}
