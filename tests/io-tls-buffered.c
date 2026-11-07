/* *****************************************************************************
Test - TLS input buffered beyond the kernel socket is delivered (405 TLS)

A TLS backend that decrypts a record holds plaintext the poller can't see. If
`on_data` reads less than is buffered and returns, the backend must schedule
another `on_data` (`fio_io_on_data_schedule`) or the rest stalls until more
data arrives on the socket. The server below reads a single byte per
`on_data`; the client sends one burst. Native TLS 1.3 and, when available,
OpenSSL.
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

#define FIO___TEST_TB_LEN 300

static fio_io_protocol_s fio___test_tb_server;
static fio_io_protocol_s fio___test_tb_client;
static fio_io_listener_s *fio___test_tb_listener;
static char fio___test_tb_url[64];
static volatile int fio___test_tb_received;
static volatile int fio___test_tb_ticks;

static void fio___test_tb_reserve_url(char *dest, size_t len) {
  fio_socket_i reserve =
      fio_sock_open2("tcp://127.0.0.1:0", FIO_SOCK_SERVER | FIO_SOCK_TCP);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(reserve), "failed to reserve loopback port");
  struct sockaddr_in address = {0};
  socklen_t address_len = sizeof(address);
  FIO_ASSERT(!getsockname(reserve, (struct sockaddr *)&address, &address_len),
             "getsockname failed while reserving loopback port");
  fio_sock_close(reserve);
  snprintf(dest, len, "tcp://127.0.0.1:%u", (unsigned)ntohs(address.sin_port));
}

/* one byte per event: the remaining plaintext stays buffered in the TLS layer
 */
static void fio___test_tb_server_on_data(fio_io_s *io) {
  char c;
  fio___test_tb_received += (int)fio_io_read(io, &c, 1);
}
static void fio___test_tb_client_on_attach(fio_io_s *io) {
  char buf[FIO___TEST_TB_LEN];
  FIO_MEMSET(buf, 'x', sizeof(buf));
  fio_io_write(io, buf, sizeof(buf));
}
static void fio___test_tb_client_on_data(fio_io_s *io) {
  char buf[64];
  while (fio_io_read(io, buf, sizeof(buf)))
    ;
}

static int fio___test_tb_driver(void *u1, void *u2) {
  (void)u1, (void)u2;
  if (fio___test_tb_received < FIO___TEST_TB_LEN && ++fio___test_tb_ticks < 200)
    return 0; /* 2 seconds max */
  fio_io_stop();
  return -1;
}

static void test_tb_run(const char *name, fio_io_functions_s fn) {
  fio___test_tb_received = fio___test_tb_ticks = 0;
  fio___test_tb_server = (fio_io_protocol_s){
      .on_data = fio___test_tb_server_on_data,
      .on_timeout = fio_io_touch,
      .io_functions = fn,
  };
  fio___test_tb_client = (fio_io_protocol_s){
      .on_attach = fio___test_tb_client_on_attach,
      .on_data = fio___test_tb_client_on_data,
      .on_timeout = fio_io_touch,
      .io_functions = fn,
  };
  fio___test_tb_reserve_url(fio___test_tb_url, sizeof(fio___test_tb_url));
  fio_io_tls_s *server_cfg = fio_io_tls_new();
  fio_io_tls_cert_add(server_cfg, "localhost", NULL, NULL, NULL);
  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_WARNING); /* SECURITY: no verification */
  fio___test_tb_listener = fio_io_listen(.url = fio___test_tb_url,
                                         .protocol = &fio___test_tb_server,
                                         .tls = server_cfg,
                                         .hide_from_log = 1);
  FIO_ASSERT(fio___test_tb_listener, "%s: TLS listen failed", name);
  fio_io_connect(fio___test_tb_url,
                 .protocol = &fio___test_tb_client,
                 .tls = fio_io_tls_new()); /* owned by connect; no verify */
  fio_io_run_every(.fn = fio___test_tb_driver, .every = 10, .repetitions = -1);
  fio_io_start(0);
  FIO_LOG_LEVEL_SET(log_level);
  fio_io_listen_stop(fio___test_tb_listener);
  FIO_ASSERT(fio___test_tb_received == FIO___TEST_TB_LEN,
             "%s: buffered TLS input stalled (%d of %d bytes delivered)",
             name,
             fio___test_tb_received,
             FIO___TEST_TB_LEN);
  fprintf(stderr, "* %s: buffered TLS input delivered: OK\n", name);
}

int main(void) {
#if FIO_OS_POSIX
  test_tb_run("TLS 1.3 (native)", fio_tls13_io_functions());
#if defined(H___FIO_OPENSSL___H) && HAVE_OPENSSL
  test_tb_run("OpenSSL", fio_openssl_io_functions());
#endif
#else
  fprintf(stderr, "* TLS buffered input tests skipped on non-POSIX\n");
#endif
  return 0;
}
