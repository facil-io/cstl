/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SOCK
#include FIO_INCLUDE_FILE

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

int main(void) {
  test_raw_socket_api_no_poll();
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
