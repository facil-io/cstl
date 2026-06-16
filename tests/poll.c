/* *****************************************************************************
Test - POSIX portable polling (102 poll api.h + backends)

Correctness coverage for fio_poll_init, fio_poll_destroy, fio_poll_engine,
fio_poll_monitor, fio_poll_review, and fio_poll_forget.

Uses fio_sock_socketpair for in-process deterministic roundtrips.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SOCK
#define FIO_POLL
#include FIO_INCLUDE_FILE

typedef struct {
  int on_data_count;
  int on_ready_count;
  int on_close_count;
} poll_counts_s;

static void cb_on_data(void *udata) {
  ((poll_counts_s *)udata)->on_data_count++;
}
static void cb_on_ready(void *udata) {
  ((poll_counts_s *)udata)->on_ready_count++;
}
static void cb_on_close(void *udata) {
  ((poll_counts_s *)udata)->on_close_count++;
}

static void test_poll_engine_and_init(void) {
  fprintf(stderr, "* Poll engine: %s\n", fio_poll_engine());
  FIO_ASSERT(!strcmp(fio_poll_engine(), FIO_POLL_ENGINE_STR),
             "fio_poll_engine() should return %s", FIO_POLL_ENGINE_STR);

  fio_poll_s p;
  fio_poll_init(&p,
                .on_data = cb_on_data,
                .on_ready = cb_on_ready,
                .on_close = cb_on_close);
  fio_poll_destroy(&p);
  fprintf(stderr, "* init/destroy: OK\n");
}

static void test_poll_on_ready(void) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  poll_counts_s counts = {0};

  FIO_ASSERT(!fio_sock_socketpair(fds),
             "fio_sock_socketpair failed: %s",
             strerror(errno));

  fio_poll_s p;
  fio_poll_init(&p,
                .on_data = cb_on_data,
                .on_ready = cb_on_ready,
                .on_close = cb_on_close);

  FIO_ASSERT(!fio_poll_monitor(&p, fds[1], &counts, POLLOUT),
             "fio_poll_monitor failed for on_ready test");

  int events = fio_poll_review(&p, 100);
  FIO_ASSERT(events >= 1,
             "fio_poll_review should have fired at least 1 event (got %d)",
             events);
  FIO_ASSERT(counts.on_ready_count >= 1,
             "on_ready should have been called (count=%d)",
             counts.on_ready_count);

  counts = (poll_counts_s){0};
  events = fio_poll_review(&p, 0);
  FIO_ASSERT(events == 0,
             "one-shot: second fio_poll_review should return 0 (got %d)",
             events);
  FIO_ASSERT(counts.on_ready_count == 0,
             "one-shot: on_ready must not fire again (count=%d)",
             counts.on_ready_count);

  fio_poll_destroy(&p);
  fio_sock_close(fds[0]);
  fio_sock_close(fds[1]);
  fprintf(stderr, "* on_ready + one-shot: OK\n");
}

static void test_poll_on_data(void) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  poll_counts_s counts = {0};

  FIO_ASSERT(!fio_sock_socketpair(fds),
             "fio_sock_socketpair failed: %s",
             strerror(errno));
  FIO_ASSERT(fio_sock_write(fds[1], "hello", 5) == 5,
             "fio_sock_write failed: %s",
             strerror(errno));

  fio_poll_s p;
  fio_poll_init(&p,
                .on_data = cb_on_data,
                .on_ready = cb_on_ready,
                .on_close = cb_on_close);

  FIO_ASSERT(!fio_poll_monitor(&p, fds[0], &counts, POLLIN),
             "fio_poll_monitor failed for on_data test");

  int events = fio_poll_review(&p, 100);
  FIO_ASSERT(events >= 1,
             "fio_poll_review should have fired at least 1 event (got %d)",
             events);
  FIO_ASSERT(counts.on_data_count >= 1,
             "on_data should have been called (count=%d)",
             counts.on_data_count);

  counts = (poll_counts_s){0};
  events = fio_poll_review(&p, 0);
  FIO_ASSERT(events == 0,
             "one-shot: second fio_poll_review should return 0 (got %d)",
             events);
  FIO_ASSERT(counts.on_data_count == 0,
             "one-shot: on_data must not fire again (count=%d)",
             counts.on_data_count);

  fio_poll_destroy(&p);
  fio_sock_close(fds[0]);
  fio_sock_close(fds[1]);
  fprintf(stderr, "* on_data + one-shot: OK\n");
}

static void test_poll_on_close(void) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  poll_counts_s counts = {0};

  FIO_ASSERT(!fio_sock_socketpair(fds),
             "fio_sock_socketpair failed: %s",
             strerror(errno));
  fio_sock_close(fds[1]);
  fds[1] = FIO_SOCKET_INVALID;

  fio_poll_s p;
  fio_poll_init(&p,
                .on_data = cb_on_data,
                .on_ready = cb_on_ready,
                .on_close = cb_on_close);

  FIO_ASSERT(!fio_poll_monitor(&p, fds[0], &counts, POLLIN),
             "fio_poll_monitor failed for on_close test");

  int events = fio_poll_review(&p, 100);
  FIO_ASSERT(events >= 1,
             "fio_poll_review should have fired at least 1 event (got %d)",
             events);
  FIO_ASSERT((counts.on_data_count + counts.on_close_count) >= 1,
             "on_data or on_close should have been called "
             "(on_data=%d, on_close=%d)",
             counts.on_data_count,
             counts.on_close_count);

  fio_poll_destroy(&p);
  fio_sock_close(fds[0]);
  fprintf(stderr, "* on_close (peer close): OK\n");
}

static void test_poll_forget(void) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  poll_counts_s counts = {0};

  FIO_ASSERT(!fio_sock_socketpair(fds),
             "fio_sock_socketpair failed: %s",
             strerror(errno));
  FIO_ASSERT(fio_sock_write(fds[1], "x", 1) == 1,
             "fio_sock_write failed: %s",
             strerror(errno));

  fio_poll_s p;
  fio_poll_init(&p,
                .on_data = cb_on_data,
                .on_ready = cb_on_ready,
                .on_close = cb_on_close);

  FIO_ASSERT(!fio_poll_monitor(&p, fds[0], &counts, POLLIN),
             "fio_poll_monitor failed for forget test");

  /* Forget the monitored fd. Return-value semantics vary by backend
   * (kqueue deletes both READ and WRITE filters, so it may report ENOENT
   * for a filter that was never added). We only assert the functional
   * outcome: after forget, no events fire for that fd. */
  fio_poll_forget(&p, fds[0]);
  fio_poll_forget(&p, fds[0]);

  int events = fio_poll_review(&p, 100);
  FIO_ASSERT(events == 0,
             "fio_poll_review should return 0 after forget (got %d)",
             events);
  FIO_ASSERT(counts.on_data_count == 0,
             "on_data must not fire after forget (count=%d)",
             counts.on_data_count);

  fio_poll_destroy(&p);
  fio_sock_close(fds[0]);
  fio_sock_close(fds[1]);
  fprintf(stderr, "* fio_poll_forget: OK\n");
}

static void test_poll_empty_review(void) {
  fio_poll_s p;
  fio_poll_init(&p,
                .on_data = cb_on_data,
                .on_ready = cb_on_ready,
                .on_close = cb_on_close);
  int events = fio_poll_review(&p, 0);
  FIO_ASSERT(events == 0,
             "empty poll review should return 0 (got %d)",
             events);
  fio_poll_destroy(&p);
  fprintf(stderr, "* empty review: OK\n");
}

int main(void) {
  fprintf(stderr,
          "=== Poll subsystem test (engine: %s) ===\n",
          FIO_POLL_ENGINE_STR);

  test_poll_engine_and_init();
  test_poll_on_ready();
  test_poll_on_data();
  test_poll_on_close();
  test_poll_forget();
  test_poll_empty_review();

  fprintf(stderr, "=== All poll tests passed ===\n");
  return 0;
}
