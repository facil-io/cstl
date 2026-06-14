/* *****************************************************************************
TLS 1.3 interoperability test with system openssl s_client.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_TLS13
#define FIO_X509
#include "test-helpers.h"

#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile pid_t fio___test_tls13_openssl_server_pid = -1;

static void fio___test_tls13_openssl_alarm(int sig) {
  (void)sig;
  if (fio___test_tls13_openssl_server_pid > 0)
    kill(fio___test_tls13_openssl_server_pid, SIGTERM);
  _exit(1);
}

FIO_SFUNC int fio___test_tls13_openssl_read_record(int fd,
                                                uint8_t *buf,
                                                size_t capa,
                                                size_t *len) {
  size_t pos = 0;
  while (pos < 5) {
    ssize_t r = read(fd, buf + pos, 5 - pos);
    if (r <= 0)
      return -1;
    pos += (size_t)r;
  }
  size_t record_len = ((size_t)buf[3] << 8) | buf[4];
  if (record_len + 5 > capa)
    return -1;
  while (pos < record_len + 5) {
    ssize_t r = read(fd, buf + pos, record_len + 5 - pos);
    if (r <= 0)
      return -1;
    pos += (size_t)r;
  }
  *len = pos;
  return 0;
}

FIO_SFUNC int fio___test_tls13_openssl_write_all(int fd,
                                              const uint8_t *buf,
                                              size_t len) {
  size_t pos = 0;
  while (pos < len) {
    ssize_t w = write(fd, buf + pos, len - pos);
    if (w <= 0)
      return -1;
    pos += (size_t)w;
  }
  return 0;
}

FIO_SFUNC int fio___test_tls13_openssl_server(int listen_fd) {
  int fd = accept(listen_fd, NULL, NULL);
  if (fd < 0)
    return 2;

  fio_x509_keypair_s keypair;
  if (fio_x509_keypair_ed25519(&keypair) != 0)
    return 3;

  const char *san_dns[] = {"localhost"};
  fio_x509_cert_options_s opts = {
      .subject_cn = "localhost",
      .subject_cn_len = 9,
      .san_dns = san_dns,
      .san_dns_count = 1,
      .is_ca = 0,
  };
  size_t cert_size = fio_x509_self_signed_cert(NULL, 0, &keypair, &opts);
  uint8_t *cert = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, cert_size, 0);
  if (!cert)
    return 4;
  size_t cert_len = fio_x509_self_signed_cert(cert, cert_size, &keypair, &opts);
  if (!cert_len)
    return 5;

  fio_tls13_server_s server;
  fio_tls13_server_init(&server);
  const uint8_t *certs[] = {cert};
  size_t cert_lens[] = {cert_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   keypair.secret_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  uint8_t in[8192];
  uint8_t out[8192];
  size_t in_len = 0;
  size_t out_len = 0;

  if (fio___test_tls13_openssl_read_record(fd, in, sizeof(in), &in_len) != 0)
    return 6;
  int consumed = fio_tls13_server_process(&server,
                                          in,
                                          in_len,
                                          out,
                                          sizeof(out),
                                          &out_len);
  if (consumed <= 0 || out_len == 0)
    return 7;
  if (fio___test_tls13_openssl_write_all(fd, out, out_len) != 0)
    return 8;

  for (size_t i = 0; i < 8 && !fio_tls13_server_is_connected(&server) &&
                     !fio_tls13_server_is_error(&server);
       ++i) {
    if (fio___test_tls13_openssl_read_record(fd, in, sizeof(in), &in_len) != 0)
      return 9;
    consumed = fio_tls13_server_process(&server,
                                        in,
                                        in_len,
                                        out,
                                        sizeof(out),
                                        &out_len);
    if (consumed < 0)
      break;
  }
  if (!fio_tls13_server_is_connected(&server)) {
    fprintf(stderr,
            "server failed: state=%s alert=%u\n",
            fio_tls13_server_state_name(&server),
            server.alert_description);
    return 10;
  }

  if (fio___test_tls13_openssl_read_record(fd, in, sizeof(in), &in_len) == 0) {
    uint8_t plain[4096];
    (void)fio_tls13_server_decrypt(&server, plain, sizeof(plain), in, in_len);
  }

  const char response[] =
      "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
  int enc_len = fio_tls13_server_encrypt(&server,
                                         out,
                                         sizeof(out),
                                         (const uint8_t *)response,
                                         sizeof(response) - 1);
  if (enc_len <= 0)
    return 11;
  if (fio___test_tls13_openssl_write_all(fd, out, (size_t)enc_len) != 0)
    return 12;

  close(fd);
  fio_tls13_server_destroy(&server);
  fio_x509_keypair_clear(&keypair);
  FIO_MEM_FREE(cert, cert_size);
  return 0;
}

int main(void) {
  if (system("command -v openssl >/dev/null 2>&1") != 0) {
    fprintf(stderr, "SKIPPED: openssl client not available\n");
    return 0;
  }

  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  FIO_ASSERT(listen_fd >= 0, "socket failed");
  int one = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  struct sockaddr_in addr;
  FIO_MEMSET(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  FIO_ASSERT(bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0,
             "bind failed");
  FIO_ASSERT(listen(listen_fd, 1) == 0, "listen failed");

  socklen_t addr_len = sizeof(addr);
  FIO_ASSERT(getsockname(listen_fd, (struct sockaddr *)&addr, &addr_len) == 0,
             "getsockname failed");
  int port = ntohs(addr.sin_port);

  pid_t pid = fork();
  FIO_ASSERT(pid >= 0, "fork failed");
  if (pid == 0) {
    int rc = fio___test_tls13_openssl_server(listen_fd);
    close(listen_fd);
    _exit(rc);
  }
  fio___test_tls13_openssl_server_pid = pid;

  signal(SIGALRM, fio___test_tls13_openssl_alarm);
  alarm(15);

  char cmd[512];
  snprintf(cmd,
           sizeof(cmd),
           "printf 'GET / HTTP/1.1\\r\\nHost: localhost\\r\\nConnection: close\\r\\n\\r\\n' | openssl s_client -connect localhost:%d -tls1_3 -quiet -ignore_unexpected_eof >/tmp/tls13-openssl-client.out 2>&1",
           port);
  int client_rc = system(cmd);
  alarm(0);
  signal(SIGALRM, SIG_DFL);

  int status = 0;
  waitpid(pid, &status, 0);
  close(listen_fd);

  FIO_ASSERT(client_rc == 0, "openssl s_client command failed with status %d", client_rc);
  FIO_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
             "server failed with status %d",
             status);

  char *body = fio_bstr_readfile(NULL, "/tmp/tls13-openssl-client.out", 0, 0);
  FIO_ASSERT(body && fio_bstr_len(body) >= 2 &&
                 FIO_MEMCMP(body + fio_bstr_len(body) - 2, "OK", 2) == 0,
             "openssl s_client response body mismatch");
  fio_bstr_free(body);
  remove("/tmp/tls13-openssl-client.out");
  return 0;
}
