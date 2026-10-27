/* *****************************************************************************
Test - TLS context ownership through fio_io_attach_fd (401-403 io, 405 TLS)

`fio_io_attach_fd` takes ownership of its `tls` context reference (success or
failure). Listeners keep their own reference and pass a `dup_context` copy to
each accepted connection; `start` moves / replaces the IO's reference with the
connection's; `cleanup` releases it. Proven per backend (native TLS 1.3 and,
when available, OpenSSL) by observing each built context's reference count
while the test holds one extra reference:

* listener accept (dup per receiver) + listener stopped while connected;
* client connect promotion (the connecting state never owns the context);
* attach failure (invalid fd) releases the transferred reference;
* an IO closed before its queued `start` runs still balances;
* a `dup_context` returning NULL is passed through; the backend `start` then
  fails closed (never serves in plain text).

At the end every context is back to exactly the test's reference.
***************************************************************************** */
#define FIO_OPENSSL
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

#if FIO_OS_POSIX
#include <netinet/in.h>
#include <sys/socket.h>

#define T_ASSERT(cond, ...)                                                    \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fio_io_stop();                                                           \
      FIO_ASSERT(0, __VA_ARGS__);                                              \
    }                                                                          \
  } while (0)

/* *****************************************************************************
Backend wrapper: records every built context and holds a test reference
***************************************************************************** */

static fio_io_functions_s T_fn;            /* backend under test */
static size_t (*T_ref)(void *ctx);         /* backend reference count */
static void *T_ctx_server, *T_ctx_client;  /* listener / connect contexts */
static void *T_ctx_nodup;                  /* dup-failure listener context */
static volatile size_t T_dups, T_dup_fails;

static void *t_build(fio_io_tls_s *tls, uint8_t is_client) {
  void *ctx = T_fn.build_context(tls, is_client);
  if (ctx) /* the test's own reference, checked and released at the end */
    (void)T_fn.dup_context(ctx);
  return ctx;
}
static void *t_build_server(fio_io_tls_s *tls, uint8_t is_client) {
  return (T_ctx_server = t_build(tls, is_client));
}
static void *t_build_client(fio_io_tls_s *tls, uint8_t is_client) {
  return (T_ctx_client = t_build(tls, is_client));
}
static void *t_build_nodup(fio_io_tls_s *tls, uint8_t is_client) {
  return (T_ctx_nodup = t_build(tls, is_client));
}
static void *t_dup(void *ctx) {
  ++T_dups;
  return T_fn.dup_context(ctx);
}
static void *t_dup_fail(void *ctx) {
  ++T_dup_fails;
  (void)ctx;
  return NULL;
}

static size_t t_ref_tls13(void *ctx) {
  return fio_atomic_add(&((fio___tls13_context_s *)ctx)->ref, 0);
}
#if defined(H___FIO_OPENSSL___H) && HAVE_OPENSSL
static size_t t_ref_openssl(void *ctx) {
  return fio_atomic_add(&((fio___openssl_context_s *)ctx)->ref, 0);
}
#endif

/* *****************************************************************************
Protocols
***************************************************************************** */

static fio_io_protocol_s T_server_pr, T_client_pr, T_raw_pr, T_nodup_pr;
static fio_io_s *T_ios[2];
static volatile int T_accepted, T_promoted, T_received, T_closed;
static volatile int T_nodup_attached, T_failed;

static void t_server_on_attach(fio_io_s *io) {
  T_ios[0] = fio_io_dup(io);
  ++T_accepted;
}
static void t_server_on_data(fio_io_s *io) {
  char buf[64];
  size_t r;
  while ((r = fio_io_read(io, buf, sizeof(buf))))
    T_received += (int)r;
}
static void t_client_on_attach(fio_io_s *io) {
  T_ios[1] = fio_io_dup(io);
  ++T_promoted; /* the connect state handed the IO to the user protocol */
  fio_io_write(io, "ping", 4);
}
static void t_client_on_data(fio_io_s *io) {
  char buf[64];
  while (fio_io_read(io, buf, sizeof(buf)))
    ;
}
static void t_on_close(void *buf, void *udata) {
  ++T_closed;
  (void)buf, (void)udata;
}
static void t_on_failed(fio_io_protocol_s *pr, void *udata) {
  T_failed = 1;
  (void)pr, (void)udata;
}
static void t_nodup_on_attach(fio_io_s *io) {
  ++T_nodup_attached; /* runs before `start`, which then closes the IO */
  (void)io;
}

/* *****************************************************************************
Driver
***************************************************************************** */

static char T_url[64], T_url_nodup[64];
static fio_io_tls_s *T_client_cfg;
static fio_io_listener_s *T_listener, *T_listener_nodup;
static fio_socket_i T_raw = FIO_SOCKET_INVALID;
static int T_step, T_ticks, T_wait;

static void t_reserve_url(char *dest, size_t len) {
  fio_socket_i r =
      fio_sock_open2("tcp://127.0.0.1:0", FIO_SOCK_SERVER | FIO_SOCK_TCP);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(r), "failed to reserve loopback port");
  struct sockaddr_in a = {0};
  socklen_t alen = sizeof(a);
  FIO_ASSERT(!getsockname(r, (struct sockaddr *)&a, &alen), "getsockname");
  fio_sock_close(r);
  snprintf(dest, len, "tcp://127.0.0.1:%u", (unsigned)ntohs(a.sin_port));
}

static int t_driver(void *u1, void *u2) {
  (void)u1, (void)u2;
  T_ASSERT(++T_ticks < 1000,
           "timed out at step %d (accepted %d, promoted %d, received %d, "
           "closed %d, failed %d)",
           T_step,
           T_accepted,
           T_promoted,
           T_received,
           T_closed,
           (int)T_failed);
  switch (T_step) {
  case 0: /* client connect (promotion) to the TLS listener */
    fio_io_connect(T_url,
                   .protocol = &T_client_pr,
                   .tls = T_client_cfg,
                   .on_failed = t_on_failed);
    T_step = 1;
    return 0;
  case 1: /* handshake done, data flowed: stop the listener while connected */
    if (T_received < 4 || !T_promoted)
      return 0;
    T_ASSERT(T_accepted == 1 && T_dups == 1,
             "listener must dup once per receiver (accepted %d, dups %zu)",
             T_accepted,
             (size_t)T_dups);
    fio_io_listen_stop(T_listener);
    T_listener = NULL;
    { /* attach failure: the transferred reference is released */
      void *ctx = T_fn.dup_context(T_ctx_server);
      T_ASSERT(!fio_io_attach_fd(FIO_SOCKET_INVALID, &T_raw_pr, NULL, ctx),
               "attach of an invalid fd must fail");
    }
    { /* IO closed before its queued `start` runs */
      fio_socket_i fds[2];
      T_ASSERT(!fio_sock_socketpair(fds), "socketpair failed");
      fio_sock_close(fds[1]);
      fio_io_s *io = fio_io_attach_fd(fds[0],
                                      &T_raw_pr,
                                      NULL,
                                      T_fn.dup_context(T_ctx_server));
      T_ASSERT(io, "socketpair attach failed");
      fio_io_close_now(io); /* `start` (fio___io_protocol_set) still queued */
    }
    /* NULL dup: passed through, `start` closes the IO (no plain text) */
    T_raw = fio_sock_open2(T_url_nodup, FIO_SOCK_CLIENT | FIO_SOCK_TCP);
    T_ASSERT(FIO_SOCK_FD_ISVALID(T_raw), "raw connect failed");
    fio_sock_set_non_block(T_raw);
    T_step = 2;
    return 0;
  case 2: { /* wait for the refused raw peer's EOF */
    char buf[16];
    ssize_t r = fio_sock_read(T_raw, buf, sizeof(buf));
    if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      return 0;
    T_ASSERT(r <= 0, "dup-failure listener served data in plain text");
    T_ASSERT(T_dup_fails >= 1 && T_nodup_attached >= 1,
             "NULL dup must reach attach and close (fails %zu, attached %d)",
             (size_t)T_dup_fails,
             (int)T_nodup_attached);
    fio_sock_close(T_raw);
    T_raw = FIO_SOCKET_INVALID;
    fio_io_listen_stop(T_listener_nodup);
    T_listener_nodup = NULL;
    for (int i = 0; i < 2; ++i) { /* close the established pair */
      if (!T_ios[i])
        continue;
      fio_io_close(T_ios[i]);
      fio_io_free(T_ios[i]);
      T_ios[i] = NULL;
    }
    T_step = 3;
    return 0;
  }
  case 3: /* let close / free_context tasks run */
    if (T_closed < 2 || ++T_wait < 5)
      return 0;
    fio_io_stop();
    return -1;
  }
  return 0;
}

/* *****************************************************************************
Per-backend run
***************************************************************************** */

static void t_check_and_release(const char *name, const char *what, void *c) {
  FIO_ASSERT(c, "%s: %s context never built", name, what);
  FIO_ASSERT(T_ref(c) == 1,
             "%s: %s context holds %zu references (expected only the test's)",
             name,
             what,
             T_ref(c));
  T_fn.free_context(c);
}

static void test_ownership(const char *name,
                           fio_io_functions_s fn,
                           size_t (*ref)(void *)) {
  T_fn = fn;
  T_ref = ref;
  T_ctx_server = T_ctx_client = T_ctx_nodup = NULL;
  T_dups = T_dup_fails = 0;
  T_accepted = T_promoted = T_received = T_closed = 0;
  T_nodup_attached = T_failed = 0;
  T_step = T_ticks = T_wait = 0;
  T_ios[0] = T_ios[1] = NULL;

  fio_io_functions_s wrapped = fn;
  wrapped.dup_context = t_dup;
  T_server_pr = (fio_io_protocol_s){.on_attach = t_server_on_attach,
                                    .on_data = t_server_on_data,
                                    .on_close = t_on_close,
                                    .on_timeout = fio_io_touch,
                                    .io_functions = wrapped};
  T_server_pr.io_functions.build_context = t_build_server;
  T_client_pr = (fio_io_protocol_s){.on_attach = t_client_on_attach,
                                    .on_data = t_client_on_data,
                                    .on_close = t_on_close,
                                    .on_timeout = fio_io_touch,
                                    .io_functions = wrapped};
  T_client_pr.io_functions.build_context = t_build_client;
  T_raw_pr = (fio_io_protocol_s){.on_timeout = fio_io_touch,
                                 .io_functions = wrapped};
  T_nodup_pr = (fio_io_protocol_s){.on_attach = t_nodup_on_attach,
                                   .on_timeout = fio_io_touch,
                                   .io_functions = wrapped};
  T_nodup_pr.io_functions.build_context = t_build_nodup;
  T_nodup_pr.io_functions.dup_context = t_dup_fail;

  t_reserve_url(T_url, sizeof(T_url));
  t_reserve_url(T_url_nodup, sizeof(T_url_nodup));
  fio_io_tls_s *server_cfg = fio_io_tls_new();
  fio_io_tls_cert_add(server_cfg, "localhost", NULL, NULL, NULL);
  fio_io_tls_s *nodup_cfg = fio_io_tls_dup(server_cfg);
  T_client_cfg = fio_io_tls_new(); /* no trust: no verify */

  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE); /* expected warnings / errors */
  T_listener = fio_io_listen(.url = T_url,
                             .protocol = &T_server_pr,
                             .tls = server_cfg,
                             .hide_from_log = 1);
  T_listener_nodup = fio_io_listen(.url = T_url_nodup,
                                   .protocol = &T_nodup_pr,
                                   .tls = nodup_cfg,
                                   .hide_from_log = 1);
  FIO_ASSERT(T_listener && T_listener_nodup, "%s: TLS listen failed", name);
  fio_io_run_every(.fn = t_driver, .every = 10, .repetitions = -1);
  fio_io_start(0);
  FIO_LOG_LEVEL_SET(log_level);
  if (T_listener)
    fio_io_listen_stop(T_listener);
  if (T_listener_nodup)
    fio_io_listen_stop(T_listener_nodup);
  fio_queue_perform_all(&FIO___IO.queue);
  /* `server_cfg`, `nodup_cfg` and `T_client_cfg` were owned (freed) by
   * `fio_io_listen` / `fio_io_connect`. */

  FIO_ASSERT(!T_failed && T_promoted == 1 && T_received >= 4,
             "%s: TLS connection failed",
             name);
  t_check_and_release(name, "listener", T_ctx_server);
  t_check_and_release(name, "client", T_ctx_client);
  t_check_and_release(name, "dup-failure listener", T_ctx_nodup);
  fio_queue_perform_all(&FIO___IO.queue);
  fprintf(stderr,
          "* %s: attach owns TLS context (accept dup, listener stop, connect "
          "promotion, attach failure, queued start, dup failure): OK\n",
          name);
}

int main(void) {
  test_ownership("TLS 1.3 (native)", fio_tls13_io_functions(), t_ref_tls13);
#if defined(H___FIO_OPENSSL___H) && HAVE_OPENSSL
  test_ownership("OpenSSL", fio_openssl_io_functions(), t_ref_openssl);
#else
  fprintf(stderr, "* OpenSSL TLS ownership skipped (no OpenSSL)\n");
#endif
  return 0;
}

#else
int main(void) {
  fprintf(stderr, "* TLS ownership tests skipped on non-POSIX\n");
  return 0;
}
#endif
