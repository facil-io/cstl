/* IO protocol scheduling:
 * - SCH-00: normal `fio_io_protocol_set` calls apply in call order.
 * - SCH-04: a NULL protocol over a custom transport keeps the transport. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIO_IO
#include "test-helpers.h"

#if FIO_OS_POSIX
#include <sys/socket.h>
#include <unistd.h>

static int failures;
#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fio_atomic_add(&failures, 1);                                            \
      fprintf(stderr, "    FAIL (%s:%d): ", __FILE__, __LINE__);               \
      fprintf(stderr, __VA_ARGS__);                                            \
      fprintf(stderr, "\n");                                                   \
    }                                                                          \
  } while (0)

static void drain(void) { fio_queue_perform_all(&FIO___IO.queue); }

/* ****************************************************************************
Counting protocols
**************************************************************************** */

static size_t attached[3], closed[3], order[8], order_len;
static void on_attach0(fio_io_s *io) { (void)io, ++attached[0]; }
static void on_attach1(fio_io_s *io) {
  (void)io, ++attached[1];
  if (order_len < 8)
    order[order_len++] = 1;
}
static void on_attach2(fio_io_s *io) {
  (void)io, ++attached[2];
  if (order_len < 8)
    order[order_len++] = 2;
}
static void on_close0(void *b, void *u) { (void)b, (void)u, ++closed[0]; }
static void on_close1(void *b, void *u) { (void)b, (void)u, ++closed[1]; }
static void on_close2(void *b, void *u) { (void)b, (void)u, ++closed[2]; }
static fio_io_protocol_s pr[3] = {
    {.on_attach = on_attach0, .on_close = on_close0},
    {.on_attach = on_attach1, .on_close = on_close1},
    {.on_attach = on_attach2, .on_close = on_close2},
};
static void reset_counts(void) {
  memset(attached, 0, sizeof(attached));
  memset(closed, 0, sizeof(closed));
  order_len = 0;
}

static int peer_fd = -1;
static fio_io_s *attach_pair(void) {
  int sv[2];
  FIO_ASSERT(!socketpair(AF_UNIX, SOCK_STREAM, 0, sv), "socketpair failed");
  peer_fd = sv[1];
  fio_io_s *io = fio_io_attach_fd(sv[0], pr, NULL, NULL);
  FIO_ASSERT(io, "attach failed");
  drain();
  FIO_ASSERT(attached[0] == 1 && fio_io_protocol(io) == pr, "not attached");
  return io;
}
static void peer_close(void) {
  if (peer_fd != -1)
    close(peer_fd);
  peer_fd = -1;
}
static size_t closed_total(void) { return closed[0] + closed[1] + closed[2]; }

/* ****************************************************************************
Tests
**************************************************************************** */

static void test_normal_order(void) {
  fprintf(stderr, "  * SCH-00 normal protocol sets apply in call order\n");
  reset_counts();
  fio_io_s *io = attach_pair();
  fio_io_protocol_set(io, pr + 1);
  fio_io_protocol_set(io, pr + 2);
  CHECK(fio_io_protocol(io) == pr, "protocol set performed inline");
  drain();
  CHECK(order_len == 2 && order[0] == 1 && order[1] == 2,
        "set order (%zu sets)",
        order_len);
  CHECK(fio_io_protocol(io) == pr + 2, "last set not applied");
  CHECK(fio_io_is_open(io), "normal set closed the IO");
  fio_io_close_now(io);
  drain();
  CHECK(closed[2] == 1 && closed_total() == 1,
        "on_close: %zu/%zu/%zu",
        closed[0],
        closed[1],
        closed[2]);
  peer_close();
}

/* custom transport: a NULL protocol keeps it (zombie) */
static size_t tr_cleanup;
static ssize_t tr_read(fio_socket_i fd, void *buf, size_t len, void *ctx) {
  (void)ctx;
  return read(fd, buf, len);
}
static void tr_clean(void *ctx) { (void)ctx, ++tr_cleanup; }

static void test_null_set_zombie(void) {
  fprintf(stderr, "  * SCH-04 NULL protocol zombie retains transport\n");
  pr[0].io_functions = (fio_io_functions_s){.read = tr_read,
                                            .cleanup = tr_clean};
  pr[0].reserved.flags = 0; /* re-initialize with the custom transport */
  /* control: the zombie keeps the transport (cleanup once, via zombie) */
  reset_counts();
  tr_cleanup = 0;
  fio_io_s *io = attach_pair();
  fio_io_protocol_set(io, NULL);
  drain();
  CHECK(fio_io_is_open(io) && fio_io_protocol(io) != pr,
        "zombie not installed");
  fio_io_close_now(io);
  drain();
  CHECK(tr_cleanup == 1 && closed_total() == 0,
        "zombie transport cleanup %zu / old on_close %zu",
        tr_cleanup,
        closed_total());
  peer_close();
  pr[0].io_functions = (fio_io_functions_s){0};
  pr[0].reserved.flags = 0;
}

int main(void) {
  fprintf(stderr, "Testing IO protocol scheduling\n");
  test_normal_order();
  test_null_set_zombie();
  drain();
  CHECK(!fio_queue_count(&FIO___IO.queue), "tasks left in the IO queue");
  if (failures) {
    fprintf(stderr, "FAILED: %d check(s)\n", failures);
    return 1;
  }
  fprintf(stderr, "PASSED\n");
  return 0;
}
#else
int main(void) {
  fprintf(stderr, "Skipping IO scheduling tests (POSIX only)\n");
  return 0;
}
#endif
