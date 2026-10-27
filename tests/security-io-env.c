/* Environment mutex initialization, concurrent set / remove and replacement
 * cleanup (void `fio_io_env_set`, unconditional `fio_io_env_remove`). */
#include <string.h>
#define FIO_IO
#include "test-helpers.h"

static unsigned closed[3];
static int values[3];
static void on_close(void *p) {
  size_t index = (size_t)((int *)p - values);
  FIO_ASSERT(index < 3, "unexpected environment value");
  ++closed[index];
  /* The callback must run outside the environment lock. */
  (void)fio_io_env_get(NULL, .type = 71, .name = FIO_BUF_INFO1("conditional"));
}

#if FIO_OS_POSIX
#include <pthread.h>
#define ENV_THREADS 4
#define ENV_ROUNDS 2000
static fio_io_s *concurrent_io;
static size_t concurrent_closed[ENV_THREADS * ENV_ROUNDS];
static int concurrent_values[ENV_THREADS * ENV_ROUNDS];

static void concurrent_on_close(void *p) {
  const size_t i = (size_t)((int *)p - concurrent_values);
  FIO_ASSERT(i < ENV_THREADS * ENV_ROUNDS, "invalid concurrent value");
  fio_atomic_add(&concurrent_closed[i], 1);
}

static void *concurrent_worker(void *arg) {
  const size_t t = (size_t)(uintptr_t)arg;
  for (size_t i = 0; i < ENV_ROUNDS; ++i) {
    if (t & 1) {
      fio_io_env_remove(concurrent_io, .type = 72,
                         .name = FIO_BUF_INFO1("concurrent"));
    } else {
      fio_io_env_set(concurrent_io, .type = 72,
                     .name = FIO_BUF_INFO1("concurrent"),
                     .udata = &concurrent_values[t * ENV_ROUNDS + i],
                     .on_close = concurrent_on_close);
    }
  }
  return NULL;
}

static void test_concurrent_env(fio_io_s *io) {
  fio___io_env_safe_s *env = io ? &io->env : &FIO___IO.env;
  /* Zero bytes are not a portable initialized mutex (macOS returns EINVAL).
   * Check before the stress so the regression is deterministic there. */
  FIO_ASSERT(!fio_thread_mutex_lock(&env->lock), "env mutex not initialized");
  FIO_ASSERT(!fio_thread_mutex_unlock(&env->lock), "env mutex unlock failed");
  memset(concurrent_closed, 0, sizeof(concurrent_closed));
  concurrent_io = io;
  pthread_t threads[ENV_THREADS];
  for (size_t t = 0; t < ENV_THREADS; ++t)
    FIO_ASSERT(!pthread_create(threads + t, NULL, concurrent_worker,
                               (void *)(uintptr_t)t),
               "pthread_create failed");
  for (size_t t = 0; t < ENV_THREADS; ++t)
    FIO_ASSERT(!pthread_join(threads[t], NULL), "pthread_join failed");
  fio_io_env_remove(io, .type = 72, .name = FIO_BUF_INFO1("concurrent"));
  for (size_t t = 0; t < ENV_THREADS; t += 2)
    for (size_t i = 0; i < ENV_ROUNDS; ++i)
      FIO_ASSERT(concurrent_closed[t * ENV_ROUNDS + i] == 1,
                 "env value closed %zu times (global=%d)",
                 concurrent_closed[t * ENV_ROUNDS + i], !io);
}

static void test_env_initializers(void) {
  test_concurrent_env(NULL);
  int fds[2];
  FIO_ASSERT(!socketpair(AF_UNIX, SOCK_STREAM, 0, fds), "socket pair failed");
  static fio_io_protocol_s protocol = {0};
  fio_io_s *io = fio_io_attach_fd(fds[0], &protocol, NULL, NULL);
  FIO_ASSERT(io, "IO attach failed");
  fio_queue_perform_all(fio_io_queue());
  test_concurrent_env(io);
  fio_io_close_now(io);
  fio_queue_perform_all(fio_io_queue());
  fio_sock_close(fds[1]);
}
#else
static void test_env_initializers(void) {
  FIO_LOG_WARNING("SKIPPED: concurrent env regression requires POSIX threads");
}
#endif

int main(void) {
  /* Run before any destroy/reset can incidentally initialize the mutex. */
  test_env_initializers();
  fio_io_env_set(NULL, .type = 71, .name = FIO_BUF_INFO1("conditional"),
                 .udata = &values[0], .on_close = on_close);
  FIO_ASSERT(fio_io_env_get(NULL, .type = 71,
                           .name = FIO_BUF_INFO1("conditional")) == &values[0],
             "initial set failed");
  fio_io_env_set(NULL, .type = 71, .name = FIO_BUF_INFO1("conditional"),
                 .udata = &values[1], .on_close = on_close);
  FIO_ASSERT(closed[0] == 1 && !closed[1], "replacement cleanup");
  FIO_ASSERT(fio_io_env_get(NULL, .type = 71,
                           .name = FIO_BUF_INFO1("conditional")) == &values[1],
             "replacement was lost");
  FIO_ASSERT(!fio_io_env_remove(NULL, .type = 71,
                                .name = FIO_BUF_INFO1("conditional")),
             "unconditional remove failed");
  FIO_ASSERT(closed[0] == 1 && closed[1] == 1, "wrong close count");
  FIO_ASSERT(fio_io_env_remove(NULL, .type = 71,
                              .name = FIO_BUF_INFO1("conditional")) == -1,
             "absent remove");
  fio_io_env_set(NULL, .type = 71, .name = FIO_BUF_INFO1("conditional"),
                 .udata = &values[2], .on_close = on_close);
  FIO_ASSERT(!fio_io_env_remove(NULL, .type = 71,
                                .name = FIO_BUF_INFO1("conditional")),
             "unconditional remove (second value)");
  FIO_ASSERT(closed[2] == 1, "unconditional callback count");
  fio_queue_perform_all(fio_io_queue());
  fprintf(stderr, "security-io-env: passed\n");
  return 0;
}
