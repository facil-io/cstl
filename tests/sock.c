/* *****************************************************************************
Test - Socket helpers (004 sock.h)

Correctness coverage for fio_sock_open, fio_sock_open2, fio_sock_accept,
fio_sock_read/write, fio_sock_dup, fio_sock_set_non_block, fio_sock_wait_io,
fio_sock_socketpair, fio_sock_peer_addr, and fio_sock_maximize_limits.

All roundtrips use in-process loopback or socketpair; no external processes.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SOCK
#include FIO_INCLUDE_FILE

#if defined(_WIN32)
#define FIO___TEST_SOCK_ERRNO() WSAGetLastError()
#else
#define FIO___TEST_SOCK_ERRNO() errno
#endif

static void test_sock_invalid_wait(void) {
  short ev = fio_sock_wait_io(FIO_SOCKET_INVALID, POLLIN | POLLOUT, 0);
  FIO_ASSERT(ev == 0,
             "fio_sock_wait_io on invalid fd should return 0 (got %d)",
             (int)ev);
}

static void test_sock_open_and_readwrite(void) {
  static const char *const address = "127.0.0.1";
  static const char payload[] = "raw";
  char buf[sizeof(payload)] = {0};

  fio_socket_i srv =
      fio_sock_open(address, "0", FIO_SOCK_TCP | FIO_SOCK_SERVER);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv),
             "server socket failed to open (%lld)",
             (long long)srv);

  struct sockaddr_storage addr = {0};
  socklen_t addrlen = sizeof(addr);
  FIO_ASSERT(!getsockname(srv, (struct sockaddr *)&addr, &addrlen),
             "getsockname failed: %s",
             strerror(errno));
  char port_str[8];
  FIO_ASSERT(addr.ss_family == AF_INET, "expected IPv4 listening socket");
  snprintf(port_str,
           sizeof(port_str),
           "%u",
           ntohs(((struct sockaddr_in *)&addr)->sin_port));

  fio_socket_i cl =
      fio_sock_open(address, port_str, FIO_SOCK_TCP | FIO_SOCK_CLIENT);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
             "client socket failed to open (%lld)",
             (long long)cl);

  fio_socket_i accepted = fio_sock_accept(srv, NULL, NULL);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted),
             "accept failed (%lld)",
             (long long)accepted);

  FIO_ASSERT(fio_sock_write(cl, payload, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "write failed");

  FIO_ASSERT(fio_sock_wait_io(accepted, POLLIN, 500) & POLLIN,
             "accepted socket should be readable");

  FIO_ASSERT(fio_sock_read(accepted, buf, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "read failed");
  FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
             "payload mismatch");

  fio_buf_info_s peer = fio_sock_peer_addr(cl);
  FIO_ASSERT(peer.buf && peer.len, "peer address should be available");

  fio_sock_close(accepted);
  fio_sock_close(cl);
  fio_sock_close(srv);
  fprintf(stderr, "* TCP open/read/write/peer_addr: OK\n");
}

static void test_sock_open2_url(void) {
  static const char payload[] = "url2";
  char buf[sizeof(payload)] = {0};

  fio_socket_i srv = fio_sock_open2("tcp://127.0.0.1:0", FIO_SOCK_SERVER);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv),
             "fio_sock_open2 server failed (%lld)",
             (long long)srv);

  struct sockaddr_storage addr = {0};
  socklen_t addrlen = sizeof(addr);
  FIO_ASSERT(!getsockname(srv, (struct sockaddr *)&addr, &addrlen),
             "getsockname failed");
  char url[64];
  snprintf(url,
           sizeof(url),
           "tcp://127.0.0.1:%u",
           ntohs(((struct sockaddr_in *)&addr)->sin_port));

  fio_socket_i cl = fio_sock_open2(url, FIO_SOCK_CLIENT);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
             "fio_sock_open2 client failed (%lld)",
             (long long)cl);

  fio_socket_i accepted = fio_sock_accept(srv, NULL, NULL);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted), "accept failed");

  FIO_ASSERT(fio_sock_write(cl, payload, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "write failed");
  FIO_ASSERT(fio_sock_read(accepted, buf, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "read failed");
  FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
             "payload mismatch");

  fio_sock_close(accepted);
  fio_sock_close(cl);
  fio_sock_close(srv);
  fprintf(stderr, "* fio_sock_open2 URL: OK\n");
}

static void test_sock_dup(void) {
  static const char payload[] = "dup-path";
  static const char reply[] = "dup-reply";
  char buf[32] = {0};

  fio_socket_i srv =
      fio_sock_open("127.0.0.1", "0", FIO_SOCK_TCP | FIO_SOCK_SERVER);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv), "server socket failed to open");

  struct sockaddr_storage addr = {0};
  socklen_t addrlen = sizeof(addr);
  FIO_ASSERT(!getsockname(srv, (struct sockaddr *)&addr, &addrlen),
             "getsockname failed");
  char port_str[8];
  snprintf(port_str,
           sizeof(port_str),
           "%u",
           ntohs(((struct sockaddr_in *)&addr)->sin_port));

  fio_socket_i cl =
      fio_sock_open("127.0.0.1", port_str, FIO_SOCK_TCP | FIO_SOCK_CLIENT);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl), "client socket failed to open");

  fio_socket_i accepted = fio_sock_accept(srv, NULL, NULL);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted), "accept failed");

  fio_socket_i accepted_dup = fio_sock_dup(accepted);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted_dup), "fio_sock_dup failed");

  fio_sock_close(accepted);

  FIO_ASSERT(fio_sock_write(cl, payload, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "write to client failed");
  FIO_ASSERT(fio_sock_read(accepted_dup, buf, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "read from dup socket failed");
  FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
             "payload mismatch after dup read");

  FIO_ASSERT(fio_sock_write(accepted_dup, reply, sizeof(reply) - 1) ==
                 (ssize_t)(sizeof(reply) - 1),
             "write from dup socket failed");
  FIO_MEMSET(buf, 0, sizeof(buf));
  FIO_ASSERT(fio_sock_read(cl, buf, sizeof(reply) - 1) ==
                 (ssize_t)(sizeof(reply) - 1),
             "read from client failed");
  FIO_ASSERT(!memcmp(buf, reply, sizeof(reply) - 1),
             "reply mismatch after dup write");

  fio_sock_close(accepted_dup);
  fio_sock_close(cl);
  fio_sock_close(srv);
  fprintf(stderr, "* fio_sock_dup: OK\n");
}

static void test_sock_nonblock(void) {
  fio_socket_i srv =
      fio_sock_open("127.0.0.1", "0", FIO_SOCK_TCP | FIO_SOCK_SERVER);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv), "server socket failed to open");

  FIO_ASSERT(fio_sock_set_non_block(srv) == 0,
             "fio_sock_set_non_block should succeed (%d)",
             FIO___TEST_SOCK_ERRNO());
  FIO_ASSERT(fio_sock_set_non_block(srv) == 0,
             "fio_sock_set_non_block should be idempotent");
  fio_sock_close(srv);

  FIO_ASSERT(fio_sock_set_non_block(FIO_SOCKET_INVALID) == -1,
             "fio_sock_set_non_block on invalid fd should fail");

  fprintf(stderr, "* fio_sock_set_non_block: OK\n");
}

static void test_sock_socketpair(void) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  FIO_ASSERT(!fio_sock_socketpair(fds), "fio_sock_socketpair failed");
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(fds[0]) && FIO_SOCK_FD_ISVALID(fds[1]),
             "socketpair returned invalid fds");

  static const char payload[] = "pair";
  char buf[sizeof(payload)] = {0};
  FIO_ASSERT(fio_sock_write(fds[1], payload, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "socketpair write failed");
  FIO_ASSERT(fio_sock_read(fds[0], buf, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "socketpair read failed");
  FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
             "socketpair payload mismatch");

  fio_sock_close(fds[0]);
  fio_sock_close(fds[1]);
  fprintf(stderr, "* fio_sock_socketpair: OK\n");
}

static void test_sock_udp(void) {
  fio_socket_i srv =
      fio_sock_open("127.0.0.1", "0", FIO_SOCK_UDP | FIO_SOCK_SERVER);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv), "UDP server socket failed to open");

  struct sockaddr_storage addr = {0};
  socklen_t addrlen = sizeof(addr);
  FIO_ASSERT(!getsockname(srv, (struct sockaddr *)&addr, &addrlen),
             "getsockname failed");
  char port_str[8];
  snprintf(port_str,
           sizeof(port_str),
           "%u",
           ntohs(((struct sockaddr_in *)&addr)->sin_port));

  fio_socket_i cl =
      fio_sock_open("127.0.0.1", port_str, FIO_SOCK_UDP | FIO_SOCK_CLIENT);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl), "UDP client socket failed to open");

  static const char payload[] = "udp!";
  char buf[64] = {0};
  FIO_ASSERT(fio_sock_write(cl, payload, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "UDP write failed");
  FIO_ASSERT(fio_sock_wait_io(srv, POLLIN, 500) & POLLIN,
             "UDP server should be readable");
  FIO_ASSERT(fio_sock_read(srv, buf, sizeof(buf) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "UDP read failed");
  FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
             "UDP payload mismatch");

  fio_sock_close(cl);
  fio_sock_close(srv);
  fprintf(stderr, "* UDP socket: OK\n");
}

static void test_sock_unix(void) {
#if defined(AF_UNIX)
#if defined(P_tmpdir) && !defined(__MINGW32__)
  static const char *const path =
      P_tmpdir "/tmp_sock_testing_facil_io_cstl.sock";
#else
  static const char *const path = "./tmp_sock_testing_facil_io_cstl.sock";
#endif
  static const char payload[] = "unix";
  char buf[sizeof(payload)] = {0};

  unlink(path);

  fio_socket_i srv = fio_sock_open(path, NULL, FIO_SOCK_UNIX | FIO_SOCK_SERVER);
  const int srv_err = FIO___TEST_SOCK_ERRNO();
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv),
             "AF_UNIX server open failed (socket_error=%d)",
             srv_err);

  fio_socket_i cl = fio_sock_open(path, NULL, FIO_SOCK_UNIX | FIO_SOCK_CLIENT);
  const int cl_err = FIO___TEST_SOCK_ERRNO();
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
             "AF_UNIX client open failed (socket_error=%d)",
             cl_err);

  fio_socket_i accepted = fio_sock_accept(srv, NULL, NULL);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted), "AF_UNIX accept failed");

  FIO_ASSERT(fio_sock_write(cl, payload, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "AF_UNIX write failed");
  FIO_ASSERT(fio_sock_read(accepted, buf, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "AF_UNIX read failed");
  FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
             "AF_UNIX payload mismatch");

  fio_sock_close(accepted);
  fio_sock_close(cl);
  fio_sock_close(srv);
  unlink(path);
  fprintf(stderr, "* AF_UNIX socket: OK\n");
#elif defined(_WIN32)
  FIO_LOG_WARNING("SKIPPED");
  fprintf(stderr, "* AF_UNIX socket: skipped on this Windows build\n");
#else
  fprintf(stderr, "* AF_UNIX socket: skipped (AF_UNIX unavailable)\n");
#endif
}

static void test_sock_maximize_limits(void) {
  size_t limit = fio_sock_maximize_limits(0);
  FIO_ASSERT(limit > 0, "fio_sock_maximize_limits should return > 0");
  fprintf(stderr, "* fio_sock_maximize_limits: %zu\n", limit);
}

int main(void) {
  test_sock_invalid_wait();
  test_sock_open_and_readwrite();
  test_sock_open2_url();
  test_sock_dup();
  test_sock_nonblock();
  test_sock_socketpair();
  test_sock_udp();
  test_sock_unix();
  test_sock_maximize_limits();
  return 0;
}
