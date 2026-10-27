/* *****************************************************************************
Test - IO cross-thread flag queries and deferred writes (402)

A user thread hammers the public flag accessors, `fio_io_write` and
`fio_io_touch` on an attached socketpair end while the IO thread processes the
resulting write / touch / on_ready tasks. Verify every written byte arrives
once and graceful close completes. Native-width flag observations use the
library's scalar-read convention; this functional test does not establish
C-language data-race freedom, and TSan can report those observations.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_IO
#include FIO_INCLUDE_FILE

#if FIO_OS_POSIX
#define TEST_ROUNDS 4096

static fio_socket_i test_peer = FIO_SOCKET_INVALID;
static fio_thread_t test_thread;
static volatile int test_thread_started;
static volatile int test_closed;
static volatile int test_failed;
static size_t test_received;
static int test_ticks;
/* user thread failure (read after join): asserting off the IO thread would
 * call `exit` while the reactor runs and could hang instead of failing */
static const char *test_error;
static size_t test_error_round;
#define TEST_CHECK(cond, msg, round)                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      test_error = (msg);                                                      \
      test_error_round = (round);                                              \
      goto done;                                                               \
    }                                                                          \
  } while (0)

/* user thread: cross-thread readers of `io->flags` next to IO-thread RMWs */
static void *test_user_thread(void *io_) {
  fio_io_s *io = (fio_io_s *)io_;
  for (size_t i = 0; i < TEST_ROUNDS; ++i) {
    TEST_CHECK(fio_io_is_open(io), "IO closed early", i);
    TEST_CHECK(!fio_io_is_suspended(io), "IO suspended", i);
    fio_io_write(io, "x", 1);
    fio_io_touch(io);
  }
  fio_io_close(io);
  TEST_CHECK(!fio_io_is_open(io),
             "closing IO still reported as open",
             (size_t)TEST_ROUNDS);
done:
  fio_io_close(io); /* no-op unless a check failed: lets the peer see EOF */
  fio_io_free(io); /* the `fio_io_dup` from `on_attach` */
  return NULL;
}

static void test_on_attach(fio_io_s *io) {
  FIO_ASSERT(!fio_thread_create(&test_thread,
                                test_user_thread,
                                (void *)fio_io_dup(io)),
             "thread creation failed");
  test_thread_started = 1;
}

static void test_on_close(void *buf, void *udata) {
  test_closed = 1;
  (void)buf, (void)udata;
}

static fio_io_protocol_s test_protocol = {
    .on_attach = test_on_attach,
    .on_close = test_on_close,
    .timeout = 10000,
};

static void test_start_task(void *u1, void *u2) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  FIO_ASSERT(!fio_sock_socketpair(fds), "socketpair failed");
  fio_sock_set_non_block(fds[1]);
  test_peer = fds[1];
  FIO_ASSERT(fio_io_attach_fd(fds[0], &test_protocol, NULL, NULL),
             "attach failed");
  (void)u1, (void)u2;
}

/* IO thread: drain the peer end until EOF (or a ~30s hard limit) */
static int test_peer_task(void *u1, void *u2) {
  char buf[4096];
  (void)u1, (void)u2;
  if (++test_ticks > 3000) {
    test_failed = 1;
    fio_io_stop();
    return -1;
  }
  for (;;) {
    ssize_t r = fio_sock_read(test_peer, buf, sizeof(buf));
    if (r > 0) {
      test_received += (size_t)r;
      continue;
    }
    if (r == 0 || (errno != EWOULDBLOCK && errno != EAGAIN)) {
      fio_io_stop();
      return -1;
    }
    return 0;
  }
}

int main(void) {
  fio_io_defer(test_start_task, NULL, NULL);
  fio_io_run_every(.fn = test_peer_task, .every = 10, .repetitions = -1);
  fio_io_start(0);
  FIO_ASSERT(test_thread_started, "IO was never attached");
  FIO_ASSERT(!fio_thread_join(&test_thread), "thread join failed");
  fio_sock_close(test_peer);
  FIO_ASSERT(!test_error,
             "%s (round %zu)",
             test_error,
             test_error_round);
  FIO_ASSERT(!test_failed, "test timed out");
  FIO_ASSERT(test_closed, "IO should be closed after the user thread ends");
  FIO_ASSERT(test_received == TEST_ROUNDS,
             "lost or duplicated writes: got %zu of %zu bytes",
             test_received,
             (size_t)TEST_ROUNDS);
  fprintf(stderr, "* IO cross-thread flag access tests passed\n");
  return 0;
}

#else
int main(void) {
  fprintf(stderr, "* IO flags race test skipped on non-POSIX platform\n");
  return 0;
}
#endif
