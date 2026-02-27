/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SOCK
#include FIO_INCLUDE_FILE

#if defined(_WIN32)
#define FIO___TEST_SOCK_ERRNO() WSAGetLastError()
#else
#define FIO___TEST_SOCK_ERRNO() errno
#endif

#if defined(_WIN32) && defined(AF_UNIX)
static int test_unix_url_open2_roundtrip(const char *url,
                                         const char *path_for_unlink,
                                         const char *label,
                                         int fail_hard) {
  static const char payload[] = "u2";
  char buf[sizeof(payload)] = {0};
  fio_socket_i srv = (fio_socket_i)-1;
  fio_socket_i cl = (fio_socket_i)-1;
  fio_socket_i accepted = (fio_socket_i)-1;

  unlink(path_for_unlink);
  errno = 0;
  srv = fio_sock_open2(url, FIO_SOCK_SERVER);
  const int srv_errno = errno;
  const int srv_sock_err = FIO___TEST_SOCK_ERRNO();
  if (!FIO_SOCK_FD_ISVALID(srv)) {
    if (fail_hard) {
      FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv),
                 "%s: server open failed (url=%s, path=%s, errno=%d, "
                 "socket_error=%d)",
                 label,
                 url,
                 path_for_unlink,
                 srv_errno,
                 srv_sock_err);
    }
    fprintf(stderr,
            "* WARNING %s: server open failed (url=%s, path=%s, errno=%d, "
            "socket_error=%d)\n",
            label,
            url,
            path_for_unlink,
            srv_errno,
            srv_sock_err);
    unlink(path_for_unlink);
    return -1;
  }

  errno = 0;
  cl = fio_sock_open2(url, FIO_SOCK_CLIENT);
  const int cl_errno = errno;
  const int cl_sock_err = FIO___TEST_SOCK_ERRNO();
  if (!FIO_SOCK_FD_ISVALID(cl)) {
    if (fail_hard) {
      FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
                 "%s: client open failed (url=%s, path=%s, errno=%d, "
                 "socket_error=%d)",
                 label,
                 url,
                 path_for_unlink,
                 cl_errno,
                 cl_sock_err);
    }
    fprintf(stderr,
            "* WARNING %s: client open failed (url=%s, path=%s, errno=%d, "
            "socket_error=%d)\n",
            label,
            url,
            path_for_unlink,
            cl_errno,
            cl_sock_err);
    fio_sock_close(srv);
    unlink(path_for_unlink);
    return -1;
  }

  accepted = fio_sock_accept(srv, NULL, NULL);
  const int accept_errno = errno;
  const int accept_sock_err = FIO___TEST_SOCK_ERRNO();
  if (!FIO_SOCK_FD_ISVALID(accepted)) {
    if (fail_hard) {
      FIO_ASSERT(
          FIO_SOCK_FD_ISVALID(accepted),
          "%s: accept failed (url=%s, path=%s, errno=%d, socket_error=%d)",
          label,
          url,
          path_for_unlink,
          accept_errno,
          accept_sock_err);
    }
    fprintf(stderr,
            "* WARNING %s: accept failed (url=%s, path=%s, errno=%d, "
            "socket_error=%d)\n",
            label,
            url,
            path_for_unlink,
            accept_errno,
            accept_sock_err);
    fio_sock_close(cl);
    fio_sock_close(srv);
    unlink(path_for_unlink);
    return -1;
  }

  ssize_t wrote = fio_sock_write(cl, payload, sizeof(payload) - 1);
  const int write_errno = errno;
  const int write_sock_err = FIO___TEST_SOCK_ERRNO();
  if (wrote != (ssize_t)(sizeof(payload) - 1)) {
    if (fail_hard) {
      FIO_ASSERT(wrote == (ssize_t)(sizeof(payload) - 1),
                 "%s: client write failed (url=%s, path=%s, errno=%d, "
                 "socket_error=%d)",
                 label,
                 url,
                 path_for_unlink,
                 write_errno,
                 write_sock_err);
    }
    fprintf(stderr,
            "* WARNING %s: client write failed (url=%s, path=%s, errno=%d, "
            "socket_error=%d)\n",
            label,
            url,
            path_for_unlink,
            write_errno,
            write_sock_err);
    fio_sock_close(accepted);
    fio_sock_close(cl);
    fio_sock_close(srv);
    unlink(path_for_unlink);
    return -1;
  }

  ssize_t readn = fio_sock_read(accepted, buf, sizeof(payload) - 1);
  const int read_errno = errno;
  const int read_sock_err = FIO___TEST_SOCK_ERRNO();
  if (readn != (ssize_t)(sizeof(payload) - 1)) {
    if (fail_hard) {
      FIO_ASSERT(readn == (ssize_t)(sizeof(payload) - 1),
                 "%s: server read failed (url=%s, path=%s, errno=%d, "
                 "socket_error=%d)",
                 label,
                 url,
                 path_for_unlink,
                 read_errno,
                 read_sock_err);
    }
    fprintf(stderr,
            "* WARNING %s: server read failed (url=%s, path=%s, errno=%d, "
            "socket_error=%d)\n",
            label,
            url,
            path_for_unlink,
            read_errno,
            read_sock_err);
    fio_sock_close(accepted);
    fio_sock_close(cl);
    fio_sock_close(srv);
    unlink(path_for_unlink);
    return -1;
  }

  if (memcmp(buf, payload, sizeof(payload) - 1)) {
    if (fail_hard)
      FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
                 "%s: payload mismatch (url=%s, path=%s)",
                 label,
                 url,
                 path_for_unlink);
    fprintf(stderr,
            "* WARNING %s: payload mismatch (url=%s, path=%s, errno=%d, "
            "socket_error=%d)\n",
            label,
            url,
            path_for_unlink,
            errno,
            FIO___TEST_SOCK_ERRNO());
    fio_sock_close(accepted);
    fio_sock_close(cl);
    fio_sock_close(srv);
    unlink(path_for_unlink);
    return -1;
  }

  fio_sock_close(accepted);
  fio_sock_close(cl);
  fio_sock_close(srv);
  unlink(path_for_unlink);
  return 0;
}
#endif

static void test_raw_socket_api_no_poll(void) {
  static const char *const address = "127.0.0.1";
  static const char *const port = "9447";
  static const char payload[] = "raw";
  char buf[sizeof(payload)] = {0};

  fio_socket_i srv =
      fio_sock_open(address, port, FIO_SOCK_TCP | FIO_SOCK_SERVER);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv),
             "raw socket test: server socket failed to open (%lld)",
             (long long)srv);

  fio_socket_i cl =
      fio_sock_open(address, port, FIO_SOCK_TCP | FIO_SOCK_CLIENT);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
             "raw socket test: client socket failed to open (%lld)",
             (long long)cl);

  fio_socket_i accepted = fio_sock_accept(srv, NULL, NULL);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted),
             "raw socket test: accept failed (%lld)",
             (long long)accepted);

  FIO_ASSERT(fio_sock_write(cl, payload, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "raw socket test: write failed");

  FIO_ASSERT(fio_sock_read(accepted, buf, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "raw socket test: read failed");
  FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
             "raw socket test: payload mismatch");

  fio_sock_close(accepted);
  fio_sock_close(cl);
  fio_sock_close(srv);
  fprintf(stderr, "* raw socket API (no poll): OK\n");
}

static void test_sock_dup_api(void) {
  static const char *const address = "127.0.0.1";
  static const char *const port = "9448";
  static const char payload[] = "dup-path";
  static const char reply[] = "dup-reply";
  char buf[32] = {0};

  fio_socket_i srv =
      fio_sock_open(address, port, FIO_SOCK_TCP | FIO_SOCK_SERVER);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv),
             "sock dup test: server socket failed to open (%lld)",
             (long long)srv);

  fio_socket_i cl =
      fio_sock_open(address, port, FIO_SOCK_TCP | FIO_SOCK_CLIENT);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
             "sock dup test: client socket failed to open (%lld)",
             (long long)cl);

  fio_socket_i accepted = fio_sock_accept(srv, NULL, NULL);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted),
             "sock dup test: accept failed (%lld)",
             (long long)accepted);

  fio_socket_i accepted_dup = fio_sock_dup(accepted);
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted_dup),
             "sock dup test: fio_sock_dup failed for accepted socket (%lld)",
             (long long)accepted);

  fio_sock_close(accepted);

  FIO_ASSERT(fio_sock_write(cl, payload, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "sock dup test: write to client failed");
  FIO_ASSERT(fio_sock_read(accepted_dup, buf, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "sock dup test: read from dup socket failed");
  FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
             "sock dup test: payload mismatch after dup read");

  FIO_ASSERT(fio_sock_write(accepted_dup, reply, sizeof(reply) - 1) ==
                 (ssize_t)(sizeof(reply) - 1),
             "sock dup test: write from dup socket failed");
  FIO_MEMSET(buf, 0, sizeof(buf));
  FIO_ASSERT(fio_sock_read(cl, buf, sizeof(reply) - 1) ==
                 (ssize_t)(sizeof(reply) - 1),
             "sock dup test: read from client failed");
  FIO_ASSERT(!memcmp(buf, reply, sizeof(reply) - 1),
             "sock dup test: reply mismatch after dup write");

  fio_sock_close(accepted_dup);
  fio_sock_close(cl);
  fio_sock_close(srv);

  fprintf(stderr, "* sock dup API: OK\n");
}

static void test_unix_domain_socket_support(void) {
#if defined(AF_UNIX)
#if defined(P_tmpdir) && !defined(__MINGW32__)
  static const char *const path =
      P_tmpdir "/tmp_unix_testing_socket_facil_io_ci.sock";
#else
  static const char *const path = "./tmp_unix_testing_socket_facil_io_ci.sock";
#endif
  static const char payload[] = "unix";
  char buf[sizeof(payload)] = {0};

  unlink(path);

  errno = 0;
  fio_socket_i srv = fio_sock_open(path, NULL, FIO_SOCK_UNIX | FIO_SOCK_SERVER);
  const int srv_err = FIO___TEST_SOCK_ERRNO();
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv),
             "AF_UNIX test failure: server open failed (socket_error=%d). "
             "Unix-domain sockets are unsupported or broken on this platform.",
             srv_err);

  errno = 0;
  fio_socket_i cl = fio_sock_open(path, NULL, FIO_SOCK_UNIX | FIO_SOCK_CLIENT);
  const int cl_err = FIO___TEST_SOCK_ERRNO();
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
             "AF_UNIX test failure: client open failed (socket_error=%d). "
             "Unix-domain sockets are unsupported or broken on this platform.",
             cl_err);

  fio_socket_i accepted = fio_sock_accept(srv, NULL, NULL);
  const int accept_err = FIO___TEST_SOCK_ERRNO();
  FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted),
             "AF_UNIX test failure: accept failed (%lld, socket_error=%d). "
             "Unix-domain sockets are unsupported or broken on this platform.",
             (long long)accepted,
             accept_err);

  FIO_ASSERT(fio_sock_write(cl, payload, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "AF_UNIX test failure: client write failed.");
  FIO_ASSERT(fio_sock_read(accepted, buf, sizeof(payload) - 1) ==
                 (ssize_t)(sizeof(payload) - 1),
             "AF_UNIX test failure: server read failed.");
  FIO_ASSERT(!memcmp(buf, payload, sizeof(payload) - 1),
             "AF_UNIX test failure: payload mismatch.");

  fio_sock_close(accepted);
  fio_sock_close(cl);
  fio_sock_close(srv);
  unlink(path);

  fprintf(stderr, "* AF_UNIX socket API: OK\n");
#elif defined(_WIN32)
  FIO_ASSERT(0,
             "AF_UNIX test failure: AF_UNIX is unavailable in this Windows "
             "build, so Unix-domain socket support is not present.");
#else
  fprintf(
      stderr,
      "* AF_UNIX socket API: skipped (AF_UNIX unavailable at compile time)\n");
#endif
}

static void test_windows_unix_url_path_formats(void) {
#if defined(_WIN32) && defined(AF_UNIX)
  char cwd[1024] = {0};
  char cwd_slash[1024] = {0};
  char drive_path_backslash[1280] = {0};
  char drive_path_slash[1280] = {0};
  char url_drive_backslash[1536] = {0};
  char url_drive_slash[1536] = {0};
  const char *const rel_backslash_path =
      ".\\tmp\\tests\\tmp_unix_win_url_backslash.sock";
  const char *const rel_backslash_url =
      "unix://.\\tmp\\tests\\tmp_unix_win_url_backslash.sock";

  FIO_ASSERT(getcwd(cwd, sizeof(cwd)),
             "windows unix:// URL formatting test: getcwd failed.");

  FIO_MEMCPY(cwd_slash, cwd, sizeof(cwd_slash));
  for (size_t i = 0; cwd_slash[i]; ++i) {
    if (cwd_slash[i] == '\\')
      cwd_slash[i] = '/';
  }

  FIO_ASSERT((size_t)snprintf(drive_path_backslash,
                              sizeof(drive_path_backslash),
                              "%s\\tmp\\tests\\tmp_unix_win_url_drive.sock",
                              cwd) < sizeof(drive_path_backslash),
             "windows unix:// URL formatting test: drive path is too long "
             "(backslash variant). cwd=%s",
             cwd);
  FIO_ASSERT((size_t)snprintf(drive_path_slash,
                              sizeof(drive_path_slash),
                              "%s/tmp/tests/tmp_unix_win_url_drive.sock",
                              cwd_slash) < sizeof(drive_path_slash),
             "windows unix:// URL formatting test: drive path is too long "
             "(slash variant). cwd=%s",
             cwd_slash);

  FIO_ASSERT((size_t)snprintf(url_drive_backslash,
                              sizeof(url_drive_backslash),
                              "unix://%s",
                              drive_path_backslash) <
                 sizeof(url_drive_backslash),
             "windows unix:// URL formatting test: URL is too long "
             "(drive+backslash). path=%s",
             drive_path_backslash);
  FIO_ASSERT((size_t)snprintf(url_drive_slash,
                              sizeof(url_drive_slash),
                              "unix://%s",
                              drive_path_slash) < sizeof(url_drive_slash),
             "windows unix:// URL formatting test: URL is too long "
             "(drive+slash). path=%s",
             drive_path_slash);

  const int drive_backslash_ok = test_unix_url_open2_roundtrip(
      url_drive_backslash,
      drive_path_backslash,
      "windows unix:// drive-letter backslash path",
      0);
  const int rel_backslash_ok =
      test_unix_url_open2_roundtrip(rel_backslash_url,
                                    rel_backslash_path,
                                    "windows unix:// relative backslash path",
                                    0);
  test_unix_url_open2_roundtrip(url_drive_slash,
                                drive_path_slash,
                                "windows unix:// normalized slash path",
                                1);

  fprintf(stderr,
          "* windows unix:// URL path formatting probes: "
          "drive+backslash=%s, relative+backslash=%s\n",
          (drive_backslash_ok == 0 ? "OK" : "WARNING"),
          (rel_backslash_ok == 0 ? "OK" : "WARNING"));
  fprintf(stderr,
          "* windows unix:// URL path formatting: OK "
          "(normalized slash path required)\n");
#else
  fprintf(stderr,
          "* windows unix:// URL path formatting: skipped "
          "(non-Windows or AF_UNIX unavailable)\n");
#endif
}

int main(void) {
  test_raw_socket_api_no_poll();
  test_sock_dup_api();
  test_unix_domain_socket_support();
  test_windows_unix_url_path_formats();
  struct {
    const char *address;
    const char *port;
    const char *msg;
    uint16_t flag;
  } server_tests[] = {
    {"127.0.0.1", "9437", "TCP", FIO_SOCK_TCP},
#ifdef AF_UNIX
#if defined(P_tmpdir) && !defined(__MINGW32__)
    {P_tmpdir "/tmp_unix_testing_socket_facil_io.sock",
     NULL,
     "Unix",
     FIO_SOCK_UNIX},
#else
    {"./tmp_unix_testing_socket_facil_io.sock", NULL, "Unix", FIO_SOCK_UNIX},
#endif
#endif
    /* accept doesn't work with UDP, not like this... UDP test is seperate */
    // {"127.0.0.1", "9437", "UDP", FIO_SOCK_UDP},
    {.address = NULL},
  };
  for (size_t i = 0; server_tests[i].address; ++i) {
    short ev = (short)-1;
    errno = 0;
    fio_socket_i srv = fio_sock_open(server_tests[i].address,
                                     server_tests[i].port,
                                     server_tests[i].flag | FIO_SOCK_SERVER);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv),
               "server socket failed to open: %s",
               strerror(errno));
    ev = fio_sock_wait_io(-1, POLLIN | POLLOUT, 0);
    FIO_ASSERT(!ev, "no error should have been returned for IO -1 (%d)", ev);
    ev = fio_sock_wait_io(srv, POLLIN, 0);
    FIO_ASSERT(!ev, "no events should have been returned (%d)", ev);
    fio_socket_i cl = fio_sock_open(server_tests[i].address,
                                    server_tests[i].port,
                                    server_tests[i].flag | FIO_SOCK_CLIENT);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
               "client socket failed to open (%lld)",
               (long long)cl);
    ev = fio_sock_wait_io(cl, POLLIN /* | POLLOUT <= OS dependent */, 0);
    FIO_ASSERT(!ev,
               "no events should have been returned for connecting client(%d)",
               ev);
    ev = fio_sock_wait_io(srv, POLLIN, 200);
    FIO_ASSERT((ev & POLLIN),
               "incoming connection should have been detected (%lld : %u)",
               (long long)srv,
               (unsigned)ev);
    fio_socket_i accepted = fio_sock_accept(srv, NULL, NULL);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(accepted),
               "accepted socket failed to open (%zd)",
               (ssize_t)accepted);
    ev = fio_sock_wait_io(cl, POLLIN | POLLOUT, 0);
    FIO_ASSERT(ev == POLLOUT,
               "POLLOUT should have been returned for connected client(%d)",
               ev);
    ev = fio_sock_wait_io(accepted, POLLIN | POLLOUT, 0);
    FIO_ASSERT(ev == POLLOUT,
               "POLLOUT should have been returned for connected client 2(%d)",
               ev);
    if (fio_sock_write(accepted, "hello", 5) > 0) {
      // wait for read
      FIO_ASSERT(
          fio_sock_wait_io(cl, POLLIN, 10) != -1 &&
              ((fio_sock_wait_io(cl, POLLIN | POLLOUT, 0) & POLLIN)),
          "fio_sock_wait_io should have returned a POLLIN event for client.");
      {
        char buf[64];
        errno = 0;
        FIO_ASSERT(fio_sock_read(cl, buf, 64) > 0,
                   "Read should have read some data...\n\t"
                   "error: %s",
                   strerror(errno));
      }
      FIO_ASSERT(!fio_sock_wait_io(cl, POLLIN, 0),
                 "No events should have occurred here! (%zu)",
                 ev);
    } else {
      FIO_ASSERT(0,
                 "send(fd:%lld) failed! error: %s",
                 (long long)accepted,
                 strerror(errno));
    }
    fio_sock_close(accepted);
    fio_sock_close(cl);
    fio_sock_close(srv);
    FIO_ASSERT((fio_sock_wait_io(cl, POLLIN | POLLOUT, 0) & POLLNVAL),
               "POLLNVAL should have been returned for closed socket (%d & %d) "
               "(POLLERR == %d)",
               fio_sock_wait_io(cl, POLLIN | POLLOUT, 0),
               (int)POLLNVAL,
               (int)POLLERR);
#ifdef AF_UNIX
    if (FIO_SOCK_UNIX == server_tests[i].flag)
      unlink(server_tests[i].address);
#endif
  }
  {
    /* UDP semi test */
    fio_socket_i srv =
        fio_sock_open("127.0.0.1", "9437", FIO_SOCK_UDP | FIO_SOCK_SERVER);
    int n = 0;
    socklen_t sn = sizeof(n);
    if (-1 != getsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, &sn) &&
        sizeof(n) == sn)
      fprintf(stderr, "\t\t- UDP default receive buffer is %d bytes\n", n);
    n = 32 * 1024 * 1024; /* try for 32Mb */
    sn = sizeof(n);
    while (setsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, sn) == -1) {
      /* failed - repeat attempt at 0.5Mb interval */
      if (n >= (1024 * 1024)) // OS may have returned max value
        n -= 512 * 1024;
      else
        break;
    }
    do {
      n += 16 * 1024; /* at 16Kb at a time */
      if (n >= 32 * 1024 * 1024)
        break;
    } while (setsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, sn) != -1);
    if (-1 != getsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, &sn) &&
        sizeof(n) == sn)
      fprintf(stderr, "\t\t- UDP receive buffer could be set to %d bytes\n", n);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(srv),
               "Couldn't open UDP server socket: %s",
               strerror(errno));
    fio_socket_i cl =
        fio_sock_open("127.0.0.1", "9437", FIO_SOCK_UDP | FIO_SOCK_CLIENT);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
               "Couldn't open UDP client socket: %s",
               strerror(errno));
    FIO_ASSERT(fio_sock_write(cl, "hello", 5) != -1,
               "couldn't send datagram from client");
    char buf[64];
    FIO_ASSERT(recvfrom(srv, buf, 64, 0, NULL, NULL) != -1,
               "couldn't read datagram");
    FIO_ASSERT(!memcmp(buf, "hello", 5), "transmission error");
    fio_sock_close(srv);
    fio_sock_close(cl);
  }
}
