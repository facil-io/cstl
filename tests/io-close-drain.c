/* *****************************************************************************
Test - IO graceful close drains transport-internal output (402)

Regression: `fio_io_close` used to call `io_functions.finish` and close the
socket as soon as the IO's own stream was empty, even while the transport (for
example TLS) still held buffered, already-transformed output. That output, and
anything `finish` queued (TLS close_notify), was dropped when the socket was
full.

A fake transport buffers written data internally (like a TLS layer), appends a
trailer on `finish`, and drains through `flush`. Small socket buffers and a
peer that starts reading late force the close path to wait. A second case
checks that a stalled peer can't hold a closing IO open: the connection
timeout closes it, even if the protocol's `on_timeout` would keep it alive.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_IO
#include FIO_INCLUDE_FILE

#if FIO_OS_POSIX
#include <sys/socket.h>

/* not a multiple of the transport chunk: the last chunk exceeds the socket
 * buffers, so it is still queued in the transport when the stream empties */
#define TEST_PAYLOAD_LEN (1024 * 1024 + 40000)
#define TEST_TRANSPORT_CAP (64 * 1024)
static const char TEST_TRAILER[] = "BYE!";
#define TEST_TRAILER_LEN (sizeof(TEST_TRAILER) - 1)

/* *****************************************************************************
Fake buffering transport (one static context per test case)
***************************************************************************** */
typedef struct {
  uint8_t buf[TEST_TRANSPORT_CAP];
  size_t len;
  size_t sent;
  int finish_calls;
} test_transport_s;

static test_transport_s test_tr;

static ssize_t test_tr_drain(fio_socket_i fd, test_transport_s *t) {
  size_t total = 0;
  while (t->sent < t->len) {
    ssize_t w = fio_sock_write(fd, t->buf + t->sent, t->len - t->sent);
    if (w <= 0)
      break;
    t->sent += (size_t)w;
    total += (size_t)w;
  }
  if (t->sent == t->len)
    t->sent = t->len = 0;
  return (ssize_t)total;
}

static ssize_t test_tr_write(fio_socket_i fd,
                             const void *buf,
                             size_t len,
                             void *ctx) {
  test_transport_s *t = (test_transport_s *)ctx;
  test_tr_drain(fd, t);
  if (t->len) { /* like TLS: no new data while transformed output is queued */
    errno = EWOULDBLOCK;
    return -1;
  }
  if (len > TEST_TRANSPORT_CAP - TEST_TRAILER_LEN)
    len = TEST_TRANSPORT_CAP - TEST_TRAILER_LEN;
  FIO_MEMCPY(t->buf, buf, len);
  t->len = len;
  test_tr_drain(fd, t);
  return (ssize_t)len; /* "transformed": always accepted */
}

static int test_tr_flush(fio_socket_i fd, void *ctx) {
  test_transport_s *t = (test_transport_s *)ctx;
  ssize_t w = test_tr_drain(fd, t);
  if (w > 0)
    return (int)w;
  if (t->len) {
    errno = EWOULDBLOCK;
    return -1;
  }
  return 0;
}

static void test_tr_finish(fio_socket_i fd, void *ctx) {
  test_transport_s *t = (test_transport_s *)ctx;
  ++t->finish_calls;
  FIO_ASSERT(!t->len, "finish called while transport output was pending");
  FIO_MEMCPY(t->buf, TEST_TRAILER, TEST_TRAILER_LEN);
  t->len = TEST_TRAILER_LEN;
  test_tr_drain(fd, t);
}

static ssize_t test_tr_read(fio_socket_i fd, void *buf, size_t len, void *c) {
  (void)c;
  return fio_sock_read(fd, buf, len);
}

static void *test_tr_build(fio_io_tls_s *tls, uint8_t is_client) {
  (void)tls, (void)is_client;
  return &test_tr;
}
static void test_tr_free(void *ctx) { (void)ctx; }
static int test_tr_peer_info(fio_socket_i fd, fio_x509_cert_s *d, void *c) {
  (void)fd, (void)d, (void)c;
  return -1;
}

/* *****************************************************************************
Test state
***************************************************************************** */
static uint8_t *test_payload;
static uint8_t *test_received;
static size_t test_received_len;
static fio_socket_i test_peer = FIO_SOCKET_INVALID;
static volatile int test_closed;
static volatile int test_eof;
static volatile int test_failed;
static int64_t test_closed_at;
static int64_t test_started_at;
static int test_peer_reads; /* 0: stalled peer, never reads */
static int test_ticks;

static void test_on_attach(fio_io_s *io) {
  fio_io_write(io, test_payload, TEST_PAYLOAD_LEN);
  fio_io_close(io);
}

static void test_on_close(void *buf, void *udata) {
  test_closed = 1;
  test_closed_at = fio_io_last_tick();
  (void)buf, (void)udata;
}

static fio_io_protocol_s test_protocol = {
    .on_attach = test_on_attach,
    .on_close = test_on_close,
    .on_timeout = fio_io_touch, /* keep-alive style: must not stall closing */
    .timeout = 1000,
    .io_functions =
        {
            /* all set: a non-NULL tls pointer would fill gaps from TLS */
            .build_context = test_tr_build,
            .free_context = test_tr_free,
            .start = fio_io_noop,
            .cleanup = test_tr_free,
            .peer_info_next = test_tr_peer_info,
            .read = test_tr_read,
            .write = test_tr_write,
            .flush = test_tr_flush,
            .finish = test_tr_finish,
        },
};

static void test_small_buffers(fio_socket_i fd) {
  int sz = 4096;
  setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sz, sizeof(sz));
  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz));
}

static void test_start_task(void *u1, void *u2) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  FIO_ASSERT(!fio_sock_socketpair(fds), "socketpair failed");
  test_small_buffers(fds[0]);
  test_small_buffers(fds[1]);
  fio_sock_set_non_block(fds[1]);
  test_peer = fds[1];
  test_started_at = fio_io_last_tick();
  FIO_ASSERT(fio_io_attach_fd(fds[0], &test_protocol, NULL, &test_tr),
             "attach failed");
  (void)u1, (void)u2;
}

/* The peer starts reading only after 300ms, so the close path must wait. */
static int test_peer_task(void *u1, void *u2) {
  (void)u1, (void)u2;
  ++test_ticks;
  if (test_ticks > 1500) { /* ~15s hard limit */
    test_failed = 1;
    fio_io_stop();
    return -1;
  }
  if (!test_peer_reads) {
    if (test_closed) {
      fio_io_stop();
      return -1;
    }
    return 0;
  }
  if (test_ticks < 30)
    return 0;
  for (int i = 0; i < 64; ++i) {
    size_t cap = TEST_PAYLOAD_LEN + 64 - test_received_len;
    ssize_t r = fio_sock_read(test_peer, test_received + test_received_len, cap);
    if (r > 0) {
      test_received_len += (size_t)r;
      continue;
    }
    if (r == 0 || (errno != EWOULDBLOCK && errno != EAGAIN)) {
      test_eof = 1;
      fio_io_stop();
      return -1;
    }
    break;
  }
  return 0;
}

static void test_run(int peer_reads) {
  test_received_len = 0;
  test_closed = test_eof = test_failed = 0;
  test_ticks = 0;
  test_peer_reads = peer_reads;
  FIO_MEMSET(&test_tr, 0, sizeof(test_tr));
  fio_io_defer(test_start_task, NULL, NULL);
  fio_io_run_every(.fn = test_peer_task, .every = 10, .repetitions = -1);
  fio_io_start(0);
  fio_sock_close(test_peer);
  test_peer = FIO_SOCKET_INVALID;
  FIO_ASSERT(!test_failed, "test timed out (peer_reads=%d)", peer_reads);
}

int main(void) {
  test_payload = (uint8_t *)malloc(TEST_PAYLOAD_LEN);
  test_received = (uint8_t *)malloc(TEST_PAYLOAD_LEN + 64);
  FIO_ASSERT(test_payload && test_received, "allocation failed");
  for (size_t i = 0; i < TEST_PAYLOAD_LEN; ++i)
    test_payload[i] = (uint8_t)(i * 7 + (i >> 11));

  /* late reader: every byte and the finish trailer arrive, in order */
  test_run(1);
  FIO_ASSERT(test_eof && test_closed, "peer should see EOF after close");
  FIO_ASSERT(test_received_len == TEST_PAYLOAD_LEN + TEST_TRAILER_LEN,
             "graceful close dropped output: got %zu of %zu bytes",
             test_received_len,
             (size_t)(TEST_PAYLOAD_LEN + TEST_TRAILER_LEN));
  FIO_ASSERT(!FIO_MEMCMP(test_received, test_payload, TEST_PAYLOAD_LEN),
             "payload mismatch");
  FIO_ASSERT(!FIO_MEMCMP(test_received + TEST_PAYLOAD_LEN,
                         TEST_TRAILER,
                         TEST_TRAILER_LEN),
             "finish trailer must be the last output");
  FIO_ASSERT(test_tr.finish_calls == 1,
             "finish must be called exactly once (%d)",
             test_tr.finish_calls);

  /* stalled peer: the closing IO times out despite on_timeout = touch */
  test_run(0);
  FIO_ASSERT(test_closed, "stalled closing IO must be closed by its timeout");
  FIO_ASSERT(test_closed_at - test_started_at < 5000,
             "stalled close took too long (%lld ms)",
             (long long)(test_closed_at - test_started_at));
  FIO_ASSERT(!test_tr.finish_calls,
             "finish must not run while output is still pending");

  free(test_payload);
  free(test_received);
  fprintf(stderr, "* IO graceful close drain tests passed\n");
  return 0;
}

#else
int main(void) {
  fprintf(stderr, "* IO close drain test skipped on non-POSIX platform\n");
  return 0;
}
#endif
