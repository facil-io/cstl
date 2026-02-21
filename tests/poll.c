/* *****************************************************************************
Test - Poll subsystem
***************************************************************************** */
#include "test-helpers.h"

#define FIO_POLL_ENGINE_POLL  /* use portable poll() backend on all platforms  \
                               */
#define FIO_SOCK
#define FIO_POLL
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Callback state — each test section resets these counters before polling.
***************************************************************************** */

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

/* *****************************************************************************
Section 1 — Engine identification + init/destroy smoke test
***************************************************************************** */

static void test_engine_and_init(void) {
  fprintf(stderr, "* Poll engine: %s\n", fio_poll_engine());
  FIO_ASSERT(!strcmp(fio_poll_engine(), FIO_POLL_ENGINE_STR),
             "fio_poll_engine() should return FIO_POLL_ENGINE_STR");

  fio_poll_s p;
  fio_poll_init(&p,
                .on_data = cb_on_data,
                .on_ready = cb_on_ready,
                .on_close = cb_on_close);
  fio_poll_destroy(&p);
  fprintf(stderr, "* init/destroy: OK\n");
}

/* *****************************************************************************
Section 2 — on_ready fires for a writable socket (one-shot verified)
***************************************************************************** */

static void test_on_ready(void) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  poll_counts_s counts = {0};
  int events;

  FIO_ASSERT(!fio_sock_socketpair(fds),
             "fio_sock_socketpair failed: %s",
             strerror(errno));

  fio_poll_s p;
  fio_poll_init(&p,
                .on_data = cb_on_data,
                .on_ready = cb_on_ready,
                .on_close = cb_on_close);

  /* Arm fds[1] for writability — a connected socket is always writable */
  FIO_ASSERT(!fio_poll_monitor(&p, fds[1], &counts, POLLOUT),
             "fio_poll_monitor failed for on_ready test");

  events = fio_poll_review(&p, 100);
  FIO_ASSERT(events >= 1,
             "fio_poll_review should have fired at least 1 event (got %d)",
             events);
  FIO_ASSERT(counts.on_ready_count >= 1,
             "on_ready should have been called (count=%d)",
             counts.on_ready_count);

  /* One-shot: second review without re-arming must fire nothing */
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

/* *****************************************************************************
Section 3 — on_data fires when data is available (one-shot verified)
***************************************************************************** */

static void test_on_data(void) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  poll_counts_s counts = {0};
  int events;

  FIO_ASSERT(!fio_sock_socketpair(fds),
             "fio_sock_socketpair failed: %s",
             strerror(errno));

  /* Write data to fds[1] so fds[0] becomes readable */
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

  events = fio_poll_review(&p, 100);
  FIO_ASSERT(events >= 1,
             "fio_poll_review should have fired at least 1 event (got %d)",
             events);
  FIO_ASSERT(counts.on_data_count >= 1,
             "on_data should have been called (count=%d)",
             counts.on_data_count);

  /* One-shot: second review without re-arming must fire nothing */
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

/* *****************************************************************************
Section 4 — some callback fires when peer closes (on_data or on_close)
***************************************************************************** */

static void test_on_close(void) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  poll_counts_s counts = {0};
  int events;

  FIO_ASSERT(!fio_sock_socketpair(fds),
             "fio_sock_socketpair failed: %s",
             strerror(errno));

  /* Close the write end — the read end should see HUP/EOF */
  fio_sock_close(fds[1]);
  fds[1] = FIO_SOCKET_INVALID;

  fio_poll_s p;
  fio_poll_init(&p,
                .on_data = cb_on_data,
                .on_ready = cb_on_ready,
                .on_close = cb_on_close);

  /* Monitor read end with POLLIN — peer close surfaces as POLLIN+EOF or HUP */
  FIO_ASSERT(!fio_poll_monitor(&p, fds[0], &counts, POLLIN),
             "fio_poll_monitor failed for on_close test");

  events = fio_poll_review(&p, 100);
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

/* *****************************************************************************
Section 5 — fio_poll_forget removes an fd from monitoring
***************************************************************************** */

static void test_forget(void) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  poll_counts_s counts = {0};
  int events;

  FIO_ASSERT(!fio_sock_socketpair(fds),
             "fio_sock_socketpair failed: %s",
             strerror(errno));

  /* Make fds[0] readable */
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

  /* forget returns 0 when the fd was monitored */
  FIO_ASSERT(!fio_poll_forget(&p, fds[0]),
             "fio_poll_forget should return 0 for a monitored fd");

  /* forget returns non-zero when the fd is not monitored */
  FIO_ASSERT(fio_poll_forget(&p, fds[0]),
             "fio_poll_forget should return non-zero for an unknown fd");

  /* review must fire nothing — fd was forgotten before review */
  events = fio_poll_review(&p, 100);
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

/* *****************************************************************************
main
***************************************************************************** */

int main(void) {
  fprintf(stderr,
          "=== Poll subsystem test (engine: %s) ===\n",
          FIO_POLL_ENGINE_STR);

  test_engine_and_init();
  test_on_ready();
  test_on_data();
  test_on_close();
  test_forget();

  fprintf(stderr, "=== All poll tests passed ===\n");
  return 0;
}
