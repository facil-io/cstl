/* Regression: initialization, cleanup and concurrent registry access. */
#define FIO_STATE
#define FIO_THREADS
#include "test-helpers.h"

static void state_test_increment(void *p) { ++*(size_t *)p; }

static fio_thread_mutex_t init_mutex = FIO_THREAD_MUTEX_INIT;
static fio_thread_cond_t init_cond;
static int init_entered, init_release;
static size_t init_late_calls;

static void state_test_block_initialize(void *p) {
  (void)p;
  fio_thread_mutex_lock(&init_mutex);
  init_entered = 1;
  fio_thread_cond_broadcast(&init_cond);
  while (!init_release)
    fio_thread_cond_wait(&init_cond, &init_mutex);
  fio_thread_mutex_unlock(&init_mutex);
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
  fio_thread_t thread;
  FIO_ASSERT(!fio_thread_cond_init(&init_cond), "fio_thread_cond_init failed");
  /* Force a late registration during snapshot dispatch, not during setup. */
  fio___state_init_phase = 0;
  FIO___STATE_TASKS_ARRAY_LOCK[FIO_CALL_NEVER] = FIO_LOCK_INIT;
  fio_state_callback_add(FIO_CALL_ON_INITIALIZE,
                         state_test_block_initialize, NULL);
  FIO_ASSERT(!fio_thread_create(&thread, state_test_initialize, NULL),
             "fio_thread_create failed");
  fio_thread_mutex_lock(&init_mutex);
  while (!init_entered)
    fio_thread_cond_wait(&init_cond, &init_mutex);
  fio_thread_mutex_unlock(&init_mutex);
  fio_state_callback_add(FIO_CALL_ON_INITIALIZE,
                         state_test_late_initialize, NULL);
  FIO_ASSERT(!init_late_calls, "late callback must not overlap initialization");
  fio_thread_mutex_lock(&init_mutex);
  init_release = 1;
  fio_thread_cond_broadcast(&init_cond);
  fio_thread_mutex_unlock(&init_mutex);
  FIO_ASSERT(!fio_thread_join(&thread), "fio_thread_join failed");
  FIO_ASSERT(init_late_calls == 1, "late callback must execute once");
  /* Invalid callbacks are ignored, including after initialization. */
  fio_state_callback_add(FIO_CALL_ON_INITIALIZE, NULL, NULL);
  fio_state_callback_add(FIO_CALL_ON_INITIALIZE, state_test_increment, &count);
  FIO_ASSERT(count == 1, "late initialize callback must run exactly once");
  FIO_ASSERT(!fio_thread_create(&thread, state_test_registry, &count),
             "fio_thread_create failed");
  for (size_t i = 0; i < 2000; ++i)
    fio_state_callback_clear_all();
  fio_state_callback_print_state();
  FIO_ASSERT(!fio_thread_join(&thread), "fio_thread_join failed");
  fio_state_callback_force(FIO_CALL_ON_USER1);
  fio_thread_cond_destroy(&init_cond);
  return 0;
}
