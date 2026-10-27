/* IO teardown and TLS context ownership checks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIO_IO
#include "test-helpers.h"

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
Counting TLS implementation hooks / callbacks
**************************************************************************** */

static char ctx_token;
static size_t ctx_built, ctx_freed, stopped;
static void *ctx_build(fio_io_tls_s *tls, uint8_t is_client) {
  (void)tls, (void)is_client;
  ++ctx_built;
  return &ctx_token;
}
static void ctx_free(void *ctx) {
  CHECK(ctx == &ctx_token, "wrong context freed");
  ++ctx_freed;
}
static void on_stop(fio_io_protocol_s *pr, void *udata) {
  (void)pr, (void)udata;
  ++stopped;
}
static void reset_counts(void) { ctx_built = ctx_freed = stopped = 0; }

static fio_io_protocol_s listen_pr = {
    .io_functions = {.build_context = ctx_build, .free_context = ctx_free},
};

static fio_io_listener_s *listen_local(void) {
  fio_io_listener_s *l = fio_io_listen(.url = "127.0.0.1:0",
                                       .protocol = &listen_pr,
                                       .on_stop = on_stop,
                                       .hide_from_log = 1);
  FIO_ASSERT(l, "listen failed");
  FIO_ASSERT(ctx_built == 1, "listener context not built");
  return l;
}

/* ****************************************************************************
Tests
**************************************************************************** */

static void test_normal_paths(void) {
  fprintf(stderr, "  * CLN-00 normal paths stay deferred\n");
  reset_counts();
  ++ctx_built;
  fio___io_func_free_context_caller(ctx_free, &ctx_token);
  CHECK(!ctx_freed, "free_context performed inline");
  drain();
  CHECK(ctx_freed == 1, "free_context lost / repeated (%zu)", ctx_freed);

  reset_counts();
  fio_io_listener_s *l = listen_local();
  fio_io_listen_stop(l); /* never attached: synchronous release */
  CHECK(stopped == 1, "on_stop not synchronous (%zu)", stopped);
  drain();
  CHECK(stopped == 1 && ctx_freed == 1,
        "normal stop: on_stop %zu, context %zu",
        stopped,
        ctx_freed);
}

static void test_null_protocol_tls_ownership(void) {
  fio_io_functions_s saved = fio_io_tls_default_functions(NULL);
  fio_io_functions_s custom = {.build_context = ctx_build,
                               .free_context = ctx_free};
  fio_io_tls_default_functions(&custom);
  reset_counts();
  CHECK(!fio_io_attach_fd(FIO_SOCKET_INVALID, NULL, NULL, &ctx_token),
        "invalid descriptor attach must fail");
  drain();
  CHECK(ctx_freed == 1, "NULL protocol failure lost the default TLS context");
#if FIO_OS_POSIX
  fio_socket_i fds[2];
  FIO_ASSERT(!fio_sock_socketpair(fds), "socketpair failed");
  fio_io_s *io = fio_io_attach_fd(fds[0], NULL, NULL, &ctx_token);
  FIO_ASSERT(io, "NULL protocol attach failed");
  fio_sock_close(fds[1]);
  fio_io_close_now(io);
  drain();
  CHECK(ctx_freed == 2, "NULL protocol close lost the default TLS context");
#endif
  fio_io_tls_default_functions(&saved);
}

static void test_custom_tls_missing_dup(void) {
  fio_io_protocol_s pr = {
      .io_functions = {.build_context = ctx_build, .free_context = ctx_free},
  };
  fio___io_protocol_init_test(&pr, 1);
  CHECK(pr.io_functions.dup_context == fio___io_func_default_dup_context,
        "custom TLS transport must not inherit another backend's context dup");
}

int main(void) {
  test_custom_tls_missing_dup();
  test_null_protocol_tls_ownership();
  fprintf(stderr, "Testing IO teardown\n");
  test_normal_paths();
  drain();
  CHECK(!fio_queue_count(&FIO___IO.queue), "tasks left in the IO queue");
  if (failures) {
    fprintf(stderr, "FAILED: %d check(s)\n", failures);
    return 1;
  }
  fprintf(stderr, "PASSED\n");
  return 0;
}
