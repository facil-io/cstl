/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_SOCK Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_SOCK_TEST___H)
#define H___FIO_SOCK_TEST___H
#ifndef H___FIO_SOCK___H
#define FIO_SOCK
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

FIO_SFUNC void FIO_NAME_TEST(stl, sock)(void) {
  fprintf(stderr,
          "* Testing socket helpers (FIO_SOCK) - partial tests only!\n");
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
    fprintf(stderr, "* Testing %s socket API\n", server_tests[i].msg);
    int srv = fio_sock_open(server_tests[i].address,
                            server_tests[i].port,
                            server_tests[i].flag | FIO_SOCK_SERVER);
    FIO_ASSERT(srv != -1, "server socket failed to open: %s", strerror(errno));
    ev = fio_sock_wait_io(-1, POLLIN | POLLOUT, 0);
    FIO_ASSERT(!ev, "no error should have been returned for IO -1 (%d)", ev);
    ev = fio_sock_wait_io(srv, POLLIN, 0);
    FIO_ASSERT(!ev, "no events should have been returned (%d)", ev);
    int cl = fio_sock_open(server_tests[i].address,
                           server_tests[i].port,
                           server_tests[i].flag | FIO_SOCK_CLIENT);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(cl),
               "client socket failed to open (%d)",
               cl);
    ev = fio_sock_wait_io(cl, POLLIN /* | POLLOUT <= OS dependent */, 0);
    FIO_ASSERT(!ev,
               "no events should have been returned for connecting client(%d)",
               ev);
    ev = fio_sock_wait_io(srv, POLLIN, 200);
    FIO_ASSERT((ev & POLLIN),
               "incoming connection should have been detected (%d : %u)",
               srv,
               (unsigned)ev);
    intptr_t accepted = accept(srv, NULL, NULL);
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
                 "send(fd:%ld) failed! error: %s",
                 accepted,
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
    fprintf(stderr, "* Testing UDP socket (abbreviated test)\n");
    int srv =
        fio_sock_open("127.0.0.1", "9437", FIO_SOCK_UDP | FIO_SOCK_SERVER);
    int n = 0; /* try for 32Mb */
    socklen_t sn = sizeof(n);
    if (-1 != getsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, &sn) &&
        sizeof(n) == sn)
      fprintf(stderr, "\t- UDP default receive buffer is %d bytes\n", n);
    n = 32 * 1024 * 1024; /* try for 32Mb */
    sn = sizeof(n);
    while (setsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, sn) == -1) {
      /* failed - repeat attempt at 0.5Mb interval */
      if (n >= (1024 * 1024)) // OS may have returned max value
        n -= 512 * 1024;
      else
        break;
    }
    if (-1 != getsockopt(srv, SOL_SOCKET, SO_RCVBUF, (void *)&n, &sn) &&
        sizeof(n) == sn)
      fprintf(stderr, "\t- UDP receive buffer could be set to %d bytes\n", n);
    FIO_ASSERT(srv != -1,
               "Couldn't open UDP server socket: %s",
               strerror(errno));
    FIO_LOG_INFO("Opening client UDP socket.");
    int cl = fio_sock_open("127.0.0.1", "9437", FIO_SOCK_UDP | FIO_SOCK_CLIENT);
    FIO_ASSERT(cl != -1,
               "Couldn't open UDP client socket: %s",
               strerror(errno));
    FIO_LOG_INFO("Starting UDP roundtrip.");
    FIO_ASSERT(fio_sock_write(cl, "hello", 5) != -1,
               "couldn't send datagram from client");
    char buf[64];
    FIO_LOG_INFO("Receiving UDP msg.");
    FIO_ASSERT(recvfrom(srv, buf, 64, 0, NULL, NULL) != -1,
               "couldn't read datagram");
    FIO_ASSERT(!memcmp(buf, "hello", 5), "transmission error");
    FIO_LOG_INFO("cleaning up UDP sockets.");
    fio_sock_close(srv);
    fio_sock_close(cl);
  }
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
