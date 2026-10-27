/* Regression: initialization, cleanup and concurrent registry access. */
#define FIO_STATE
#include "test-helpers.h"
#include <pthread.h>

static void state_test_increment(void *p) { ++*(size_t *)p; }

static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t init_cond = PTHREAD_COND_INITIALIZER;
static int init_entered, init_release;
static size_t init_late_calls;

static void state_test_block_initialize(void *p) {
  (void)p;
  pthread_mutex_lock(&init_mutex);
  init_entered = 1;
  pthread_cond_broadcast(&init_cond);
  while (!init_release)
    pthread_cond_wait(&init_cond, &init_mutex);
  pthread_mutex_unlock(&init_mutex);
}

static void state_test_late_initialize(void *p) {
  (void)p;
  ++init_late_calls;
}

static void *state_test_initialize(void *p) {
  (void)p;
  fio_state_callback_force(FIO_CALL_ON_INITIALIZE);
  return NULL;
}

static void *state_test_registry(void *p) {
  size_t *count = (size_t *)p;
  for (size_t i = 0; i < 2000; ++i) {
    fio_state_callback_add(FIO_CALL_ON_USER1, state_test_increment, count);
    fio_state_callback_remove(FIO_CALL_ON_USER1, state_test_increment, count);
    fio_state_callback_clear(FIO_CALL_ON_USER1);
  }
  return NULL;
}

int main(void) {
  size_t count = 0;
  pthread_t thread;
  /* Force a late registration during snapshot dispatch, not during setup. */
  fio___state_init_phase = 0;
  FIO___STATE_TASKS_ARRAY_LOCK[FIO_CALL_NEVER] = FIO_LOCK_INIT;
  fio_state_callback_add(FIO_CALL_ON_INITIALIZE,
                         state_test_block_initialize, NULL);
  FIO_ASSERT(!pthread_create(&thread, NULL, state_test_initialize, NULL),
             "pthread_create failed");
  pthread_mutex_lock(&init_mutex);
  while (!init_entered)
    pthread_cond_wait(&init_cond, &init_mutex);
  pthread_mutex_unlock(&init_mutex);
  fio_state_callback_add(FIO_CALL_ON_INITIALIZE,
                         state_test_late_initialize, NULL);
  FIO_ASSERT(!init_late_calls, "late callback must not overlap initialization");
  pthread_mutex_lock(&init_mutex);
  init_release = 1;
  pthread_cond_broadcast(&init_cond);
  pthread_mutex_unlock(&init_mutex);
  FIO_ASSERT(!pthread_join(thread, NULL), "pthread_join failed");
  FIO_ASSERT(init_late_calls == 1, "late callback must execute once");
  /* Invalid callbacks are ignored, including after initialization. */
  fio_state_callback_add(FIO_CALL_ON_INITIALIZE, NULL, NULL);
  fio_state_callback_add(FIO_CALL_ON_INITIALIZE, state_test_increment, &count);
  FIO_ASSERT(count == 1, "late initialize callback must run exactly once");
  FIO_ASSERT(!pthread_create(&thread, NULL, state_test_registry, &count),
             "pthread_create failed");
  for (size_t i = 0; i < 2000; ++i)
    fio_state_callback_clear_all();
  fio_state_callback_print_state();
  FIO_ASSERT(!pthread_join(thread, NULL), "pthread_join failed");
  fio_state_callback_force(FIO_CALL_ON_USER1);
  return 0;
}
