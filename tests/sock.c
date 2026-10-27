/* *****************************************************************************
Test - Socket helpers (004 sock.h)

Correctness coverage for fio_sock_open, fio_sock_open2, fio_sock_accept,
fio_sock_read/write, fio_sock_dup, fio_sock_set_non_block, fio_sock_wait_io,
fio_sock_socketpair, fio_sock_addr, fio_sock_peer_addr, fio___sock_addr2txt,
and fio_sock_maximize_limits.

All roundtrips use in-process loopback or socketpair; no external processes.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SOCK
#define FIO_FILES
#include FIO_INCLUDE_FILE

#if defined(_WIN32)
#define FIO___TEST_SOCK_ERRNO() WSAGetLastError()
#else
#define FIO___TEST_SOCK_ERRNO() errno
#endif

/* Compare explicit lengths and the terminator, including after ring reuse.
 * Count failures so one broken format does not hide the other address cases. */
static size_t test_sock_addr2txt_expect(struct sockaddr *addr,
                                        socklen_t len,
                                        bool with_port,
                                        const char *expected,
                                        size_t expected_len) {
  fio_buf_info_s actual = fio___sock_addr2txt(addr, len, with_port);
  if (!expected) {
    if (!actual.buf && !actual.len)
      return 0;
  } else if (actual.buf && actual.len == expected_len &&
             !memcmp(actual.buf, expected, expected_len) &&
             !actual.buf[expected_len]) {
    return 0;
  }
  fprintf(stderr,
          "addr2txt mismatch (family=%u, addrlen=%zu, with_port=%u): "
          "expected \"%.*s\" (%zu bytes), got \"%.*s\" (%zu bytes)\n",
          (unsigned)addr->sa_family,
          (size_t)len,
          (unsigned)with_port,
          (int)expected_len,
          expected ? expected : "",
          expected_len,
          (int)actual.len,
          actual.buf ? actual.buf : "",
          actual.len);
  return 1;
}

/* Synthetic addresses need neither network access nor AF_UNIX runtime support.
 */
static void test_sock_addr2txt(void) {
  size_t failures = 0;
  static const struct {
    const char *host;
    uint8_t bytes[16];
    bool ipv6;
  } hosts[] = {
      {"0.0.0.0", {0}, false},
      {"127.0.0.1", {127, 0, 0, 1}, false},
      {"192.0.2.10", {192, 0, 2, 10}, false},
      {"255.255.255.255", {255, 255, 255, 255}, false},
      {"::", {0}, true},
      {"::1", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, true},
      {"2001:db8::1",
       {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       true},
      {"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
       {255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255},
       true},
  };
  static const uint16_t ports[] = {0, 1, 80, 443, 54321, 65535};

  for (size_t i = 0; i < sizeof(hosts) / sizeof(*hosts); ++i) {
    for (size_t j = 0; j < sizeof(ports) / sizeof(*ports); ++j) {
      union {
        struct sockaddr_in v4;
        struct sockaddr_in6 v6;
      } addr = {0};
      socklen_t len;
      if (hosts[i].ipv6) {
        addr.v6.sin6_family = AF_INET6;
        addr.v6.sin6_port = htons(ports[j]);
        FIO_MEMCPY(&addr.v6.sin6_addr, hosts[i].bytes, 16);
        len = sizeof(addr.v6);
#if defined(__APPLE__)
        addr.v6.sin6_len = (uint8_t)len;
#endif
      } else {
        addr.v4.sin_family = AF_INET;
        addr.v4.sin_port = htons(ports[j]);
        FIO_MEMCPY(&addr.v4.sin_addr, hosts[i].bytes, 4);
        len = sizeof(addr.v4);
#if defined(__APPLE__)
        addr.v4.sin_len = (uint8_t)len;
#endif
      }
      char expected[64];
      int n = snprintf(expected,
                       sizeof(expected),
                       hosts[i].ipv6 ? "[%s]:%u" : "%s:%u",
                       hosts[i].host,
                       (unsigned)ports[j]);
      FIO_ASSERT(n > 0 && (size_t)n < sizeof(expected),
                 "test endpoint must fit the expected buffer");
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            len,
                                            true,
                                            expected,
                                            (size_t)n);
      /* Host-only output must have neither brackets nor a stale port suffix. */
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            len,
                                            false,
                                            hosts[i].host,
                                            strlen(hosts[i].host));
    }
  }

  struct sockaddr unsupported = {0};
  unsupported.sa_family = AF_UNSPEC;
  for (unsigned with_port = 0; with_port < 2; ++with_port)
    failures += test_sock_addr2txt_expect(&unsupported,
                                          sizeof(unsupported),
                                          with_port != 0,
                                          NULL,
                                          0);

#ifdef AF_UNIX
  {
    struct sockaddr_un addr = {0};
    const size_t offset = offsetof(struct sockaddr_un, sun_path);
    static const char path[] = "/run/myapp/ipc.sock";
    char longest[sizeof(addr.sun_path) + 1];
    addr.sun_family = AF_UNIX;
#if defined(__APPLE__)
    addr.sun_len = sizeof(addr);
#endif
    for (unsigned with_port = 0; with_port < 2; ++with_port) {
      /* Terminated pathname with nonzero padding after the terminator. */
      FIO_MEMSET(addr.sun_path, 'x', sizeof(addr.sun_path));
      FIO_MEMCPY(addr.sun_path, path, sizeof(path));
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            sizeof(addr),
                                            with_port != 0,
                                            path,
                                            sizeof(path) - 1);
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            (socklen_t)(offset + sizeof(path)),
                                            with_port != 0,
                                            path,
                                            sizeof(path) - 1);
      /* No NUL inside the declared address: stop at the supplied length. */
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            (socklen_t)(offset + 4),
                                            with_port != 0,
                                            path,
                                            4);
      /* Unnamed endpoints must not expose bytes outside the address length. */
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            (socklen_t)offset,
                                            with_port != 0,
                                            NULL,
                                            0);
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            0,
                                            with_port != 0,
                                            NULL,
                                            0);
      /* Empty and abstract-style names have no printable filesystem path. */
      addr.sun_path[0] = '\0';
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            (socklen_t)(offset + 1),
                                            with_port != 0,
                                            NULL,
                                            0);
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            sizeof(addr),
                                            with_port != 0,
                                            NULL,
                                            0);
      /* Exercise all sun_path bytes without relying on a trailing NUL. */
      FIO_MEMSET(addr.sun_path, 'p', sizeof(addr.sun_path));
      FIO_MEMSET(longest, 'p', sizeof(longest) - 1);
      longest[sizeof(longest) - 1] = '\0';
      const bool fits = sizeof(addr.sun_path) < FIO___SOCK_ADDR_CAP;
      failures += test_sock_addr2txt_expect((struct sockaddr *)&addr,
                                            sizeof(addr),
                                            with_port != 0,
                                            fits ? longest : NULL,
                                            fits ? sizeof(longest) - 1 : 0);
    }
  }
#endif

  /* Mixed host-only and endpoint results must survive 128 shared-ring calls. */
  {
    struct sockaddr_in addr = {0};
    fio_buf_info_s results[128];
    char expected[128][32];
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0x7F000001U);
#if defined(__APPLE__)
    addr.sin_len = sizeof(addr);
#endif
    for (size_t i = 0; i < 128; ++i) {
      addr.sin_port = htons((uint16_t)i);
      int n =
          (i & 1)
              ? snprintf(expected[i],
                         sizeof(expected[i]),
                         "127.0.0.1:%u",
                         (unsigned)i)
              : snprintf(expected[i], sizeof(expected[i]), "%s", "127.0.0.1");
      FIO_ASSERT(n > 0 && (size_t)n < sizeof(expected[i]),
                 "test ring endpoint must fit");
      results[i] = fio___sock_addr2txt((struct sockaddr *)&addr,
                                       sizeof(addr),
                                       (i & 1) != 0);
    }
    for (size_t i = 0; i < 128; ++i) {
      size_t n = strlen(expected[i]);
      if (!results[i].buf || results[i].len != n ||
          memcmp(results[i].buf, expected[i], n) || results[i].buf[n]) {
        fprintf(stderr,
                "addr2txt ring result %zu incorrect or overwritten\n",
                i);
        ++failures;
      }
      for (size_t j = 0; j < i; ++j)
        FIO_ASSERT(results[i].buf != results[j].buf,
                   "addr2txt ring must provide 128 distinct slots");
    }
    fio_buf_info_s wrapped =
        fio___sock_addr2txt((struct sockaddr *)&addr, sizeof(addr), false);
    FIO_ASSERT(wrapped.buf == results[0].buf,
               "addr2txt ring should reuse the first slot on call 129");
  }

  FIO_ASSERT(!failures, "fio___sock_addr2txt failed %zu cases", failures);
  fprintf(stderr, "* fio___sock_addr2txt: OK\n");
}

static void test_sock_invalid_wait(void) {
  short ev = fio_sock_wait_io(FIO_SOCKET_INVALID, POLLIN | POLLOUT, 0);
#if defined(_WIN32)
  FIO_ASSERT(ev == POLLNVAL,
             "Windows invalid socket should return POLLNVAL (got %d)",
             (int)ev);
#else
  FIO_ASSERT(ev == 0 || ev == POLLNVAL,
             "invalid socket should not report readiness (got %d)",
             (int)ev);
#endif
  FIO_ASSERT(!FIO_SOCK_IS_OPEN(FIO_SOCKET_INVALID),
             "invalid socket must not appear open");
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

  /* Query ports independently: the client's source port is not the listener's
   * port. Both are assigned by the OS, so neither can be hard-coded. */
  char endpoints[2][32]; /* server, client */
  size_t endpoint_lengths[2];
  int n =
      snprintf(endpoints[0], sizeof(endpoints[0]), "%s:%s", address, port_str);
  FIO_ASSERT(n > 0 && (size_t)n < sizeof(endpoints[0]),
             "server endpoint must fit");
  endpoint_lengths[0] = (size_t)n;
  addrlen = sizeof(addr);
  FIO_ASSERT(!getsockname(cl, (struct sockaddr *)&addr, &addrlen),
             "client getsockname failed: %s",
             strerror(errno));
  FIO_ASSERT(addr.ss_family == AF_INET, "expected IPv4 client socket");
  n = snprintf(endpoints[1],
               sizeof(endpoints[1]),
               "%s:%u",
               address,
               (unsigned)ntohs(((struct sockaddr_in *)&addr)->sin_port));
  FIO_ASSERT(n > 0 && (size_t)n < sizeof(endpoints[1]),
             "client endpoint must fit");
  endpoint_lengths[1] = (size_t)n;

  const struct {
    fio_socket_i fd;
    size_t endpoint;
    bool peer;
  } checks[] = {
      {srv, 0, false},
      {accepted, 0, false},
      {cl, 1, false},
      {cl, 0, true},
      {accepted, 1, true},
  };
  for (size_t i = 0; i < sizeof(checks) / sizeof(*checks); ++i) {
    fio_buf_info_s r = checks[i].peer ? fio_sock_peer_addr(checks[i].fd, true)
                                      : fio_sock_addr(checks[i].fd, true);
    const size_t endpoint = checks[i].endpoint;
    FIO_ASSERT(r.buf && r.len == endpoint_lengths[endpoint] &&
                   !memcmp(r.buf, endpoints[endpoint], r.len) && !r.buf[r.len],
               "%s result %zu should be \"%s\", got \"%.*s\"",
               checks[i].peer ? "peer address" : "local address",
               i,
               endpoints[endpoint],
               (int)r.len,
               r.buf ? r.buf : "");
  }
  /* Host-only callers must not receive a port or IPv6 endpoint brackets. */
  fio_buf_info_s host = fio_sock_addr(srv, false);
  FIO_ASSERT(host.buf && host.len == strlen(address) &&
                 !memcmp(host.buf, address, host.len) && !host.buf[host.len],
             "local host-only address should omit port");
  host = fio_sock_peer_addr(cl, false);
  FIO_ASSERT(host.buf && host.len == strlen(address) &&
                 !memcmp(host.buf, address, host.len) && !host.buf[host.len],
             "peer host-only address should omit port");
  fio_buf_info_s invalid = fio_sock_addr(FIO_SOCKET_INVALID, false);
  FIO_ASSERT(!invalid.buf && !invalid.len,
             "invalid socket has no local address");
  invalid = fio_sock_peer_addr(FIO_SOCKET_INVALID, true);
  FIO_ASSERT(!invalid.buf && !invalid.len,
             "invalid socket has no peer address");

  { /* Both functions share 128 separate (non-overlapping) output slots. */
    fio_buf_info_s r[128];
    for (size_t i = 0; i < 128; ++i) {
      switch (i & 3) {
      case 0: r[i] = fio_sock_addr(srv, true); break;
      case 1: r[i] = fio_sock_peer_addr(cl, true); break;
      case 2: r[i] = fio_sock_addr(cl, true); break;
      case 3: r[i] = fio_sock_peer_addr(accepted, true); break;
      }
    }
    for (size_t i = 0; i < 128; ++i) {
      const size_t endpoint = (i >> 1) & 1;
      FIO_ASSERT(r[i].buf && r[i].len == endpoint_lengths[endpoint] &&
                     !memcmp(r[i].buf, endpoints[endpoint], r[i].len) &&
                     !r[i].buf[r[i].len],
                 "address result %zu incorrect or overwritten: \"%.*s\"",
                 i,
                 (int)r[i].len,
                 r[i].buf ? r[i].buf : "");
      for (size_t j = 0; j < i; ++j) {
        size_t d = (size_t)(r[i].buf > r[j].buf ? r[i].buf - r[j].buf
                                                : r[j].buf - r[i].buf);
        FIO_ASSERT(d >= FIO___SOCK_ADDR_CAP,
                   "address results %zu and %zu overlap (%zu bytes apart)",
                   j,
                   i,
                   d);
      }
    }
  }

  fio_sock_close(accepted);
  fio_sock_close(cl);
  fio_sock_close(srv);
  fprintf(stderr, "* TCP open/read/write/local and peer endpoints: OK\n");
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
  /* Headers can advertise AF_UNIX even when the Winsock provider (e.g. Wine)
   * cannot create one. Only skip that unsupported runtime case. */
  fio_socket_i probe = socket(AF_UNIX, SOCK_STREAM, 0);
  if (!FIO_SOCK_FD_ISVALID(probe)) {
#if defined(_WIN32)
    if (WSAGetLastError() == WSAEAFNOSUPPORT ||
        WSAGetLastError() == WSAEPROTONOSUPPORT) {
      fprintf(stderr, "* AF_UNIX socket: skipped (runtime unsupported)\n");
      return;
    }
#endif
    FIO_ASSERT(0, "AF_UNIX probe failed (socket_error=%d)",
               FIO___TEST_SOCK_ERRNO());
  }
  fio_sock_close(probe);
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

#if defined(AF_UNIX) && !defined(_WIN32)
#include <grp.h>
#include <sys/stat.h>
#include <sys/wait.h>

/**
 * Connects to the Unix socket at `path` from a child process (optionally
 * running as `uid` / `gid`; -1 keeps the current ids). Returns 0 if connected,
 * otherwise the child's `errno` (or 255 if the child couldn't run / drop ids).
 */
static int test_sock_unix_connect_as(const char *path, long uid, long gid) {
  pid_t pid = fork();
  FIO_ASSERT(pid != -1, "fork failed: %s", strerror(errno));
  if (!pid) {
    if (uid != -1) {
      if (setgroups(0, NULL) || setgid((gid_t)gid) || setuid((uid_t)uid))
        _exit(255);
    }
    struct sockaddr_un a = {0};
    a.sun_family = AF_UNIX;
    FIO_MEMCPY(a.sun_path, path, strlen(path) + 1);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1)
      _exit(255);
    int r = connect(fd, (struct sockaddr *)&a, sizeof(a));
    int e = r ? (errno & 127) : 0;
    close(fd);
    _exit(r ? (e ? e : 254) : 0);
  }
  int status = 0;
  while (waitpid(pid, &status, 0) == -1 && errno == EINTR)
    ;
  return WIFEXITED(status) ? WEXITSTATUS(status) : 255;
}

/** Returns the socket file's permission bits (or -1). */
static int test_sock_unix_mode(const char *path) {
  struct stat st;
  if (stat(path, &st) || !S_ISSOCK(st.st_mode))
    return -1;
  return (int)(st.st_mode & 0777);
}

/*
 * Public Unix sockets (FIO_SOCK_UNIX) are 0777: any process / user may connect.
 * Private ones (FIO_SOCK_UNIX_PRIVATE) keep the process umask. File saves
 * (fio_filename_overwrite / fio_filename_tmp) never touch the umask or socket
 * modes. When running as root, a second uid (nobody, 65534) checks access.
 */
static void test_sock_unix_permissions(void) {
#if defined(P_tmpdir) && !defined(__MINGW32__)
  static const char *const path = P_tmpdir "/tmp_sock_perm_facil_io_cstl.sock";
  static const char *const file = P_tmpdir "/tmp_sock_perm_facil_io_cstl.txt";
#else
  static const char *const path = "./tmp_sock_perm_facil_io_cstl.sock";
  static const char *const file = "./tmp_sock_perm_facil_io_cstl.txt";
#endif
  const long other = 65534; /* nobody */
  const int as_root = !geteuid();
  const mode_t um_org = umask(077); /* owner-only: private must deny others */
  unlink(path);
  unlink(file);

  /* file helpers before / after sockets keep the umask and file modes */
  FIO_ASSERT(!fio_filename_overwrite(file, "x", 1), "overwrite failed");
  int tmp = fio_filename_tmp();
  FIO_ASSERT(tmp != -1, "fio_filename_tmp failed");
  close(tmp);
  FIO_ASSERT(umask(077) == 077, "file helpers changed the umask");

  /* public socket */
  fio_socket_i srv = fio_sock_open(path, NULL, FIO_SOCK_UNIX | FIO_SOCK_SERVER);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv), "public unix server failed");
  FIO_ASSERT(umask(077) == 077, "public unix socket leaked a umask change");
  int mode = test_sock_unix_mode(path);
  FIO_ASSERT(mode == 0777, "public unix socket mode %o != 777", mode);
  FIO_ASSERT(!fio_filename_overwrite(file, "xy", 2), "overwrite (2) failed");
  FIO_ASSERT(test_sock_unix_mode(path) == 0777,
             "file save changed the public socket mode");
  struct stat st;
  FIO_ASSERT(!stat(file, &st) && (st.st_mode & 0777) == 0600,
             "saved file mode %o != 600 (0666 & umask 077)",
             (unsigned)(st.st_mode & 0777));
  int r = test_sock_unix_connect_as(path, -1, -1);
  FIO_ASSERT(!r, "public: other process (same uid) can't connect (%d)", r);
  if (as_root) {
    r = test_sock_unix_connect_as(path, other, other);
    FIO_ASSERT(!r, "public: uid %ld can't connect (%d)", other, r);
  }
  fprintf(stderr,
          "* AF_UNIX public socket: mode %o, same uid: connected, "
          "uid %ld: %s\n",
          mode,
          other,
          as_root ? "connected" : "skipped (not root)");
  fio_sock_close(srv);
  unlink(path);

  /* private socket */
  srv = fio_sock_open(path, NULL, FIO_SOCK_UNIX_PRIVATE | FIO_SOCK_SERVER);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv), "private unix server failed");
  mode = test_sock_unix_mode(path);
  FIO_ASSERT(mode == 0700, "private unix socket mode %o != 700", mode);
  r = test_sock_unix_connect_as(path, -1, -1);
  FIO_ASSERT(!r, "private: owner (other process) can't connect (%d)", r);
  if (as_root) {
    r = test_sock_unix_connect_as(path, other, other);
    FIO_ASSERT(r == EACCES,
               "private: uid %ld should get EACCES (got %d)",
               other,
               r);
  }
  fprintf(stderr,
          "* AF_UNIX private socket: mode %o, owner: connected, "
          "uid %ld: %s\n",
          mode,
          other,
          as_root ? "EACCES (denied)" : "skipped (not root)");
  fio_sock_close(srv);
  unlink(path);
  unlink(file);
  umask(um_org);
}
#else
static void test_sock_unix_permissions(void) {
  fprintf(stderr, "* AF_UNIX permissions: skipped on this platform\n");
}
#endif

static void test_sock_maximize_limits(void) {
  size_t limit = fio_sock_maximize_limits(0);
  FIO_ASSERT(limit > 0, "fio_sock_maximize_limits should return > 0");
  fprintf(stderr, "* fio_sock_maximize_limits: %zu\n", limit);
}

int main(void) {
  test_sock_addr2txt();
  test_sock_invalid_wait();
  test_sock_open_and_readwrite();
  test_sock_open2_url();
  test_sock_dup();
  test_sock_nonblock();
  test_sock_socketpair();
  test_sock_udp();
  test_sock_unix();
  test_sock_unix_permissions();
  test_sock_maximize_limits();
  return 0;
}
