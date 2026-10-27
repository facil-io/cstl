/* Deterministic OS-error and deadline regressions; no real long waits. */
#if !defined(_WIN32)
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#if defined(__APPLE__)
#include <sys/qos.h>
static int test_qos(qos_class_t qos, int relative) {
  (void)qos;
  (void)relative;
  return EPERM;
}
#define pthread_set_qos_class_self_np test_qos
#endif
#if !defined(__APPLE__)
static int test_sched_get(pthread_t thread,
                          int *policy,
                          struct sched_param *p) {
  (void)thread;
  *policy = 0;
  p->sched_priority = 0;
  return 0;
}
static int test_sched_set(pthread_t thread,
                          int policy,
                          const struct sched_param *p) {
  (void)thread;
  (void)policy;
  (void)p;
  return EPERM;
}
static int test_sched_min(int policy) {
  (void)policy;
  return 0;
}
static int test_sched_max(int policy) {
  (void)policy;
  return 0;
}
#define pthread_getschedparam  test_sched_get
#define pthread_setschedparam  test_sched_set
#define sched_get_priority_min test_sched_min
#define sched_get_priority_max test_sched_max
#endif
static int test_create(pthread_t *t,
                       const pthread_attr_t *a,
                       void *(*fn)(void *),
                       void *arg) {
  (void)t;
  (void)a;
  (void)fn;
  (void)arg;
  return EAGAIN;
}
static int test_lock(pthread_mutex_t *m) {
  (void)m;
  return EDEADLK;
}
static int test_unlock(pthread_mutex_t *m) {
  (void)m;
  return EPERM;
}
static int clock_error;
static int clock_at_limit;
static int waits;
static struct timespec deadline;
static int test_clock(clockid_t c, struct timespec *t) {
  (void)c;
  if (clock_error) {
    errno = EIO;
    return -1;
  }
  *t = (struct timespec){100, 999999999};
  if (clock_at_limit) {
    uintmax_t limit =
        ((time_t)-1 > 0)
            ? (uintmax_t)(time_t)-1
            : (((uintmax_t)1 << (sizeof(time_t) * CHAR_BIT - 1)) - 1);
    t->tv_sec = (time_t)limit;
  }
  return 0;
}
static int test_wait(pthread_cond_t *c,
                     pthread_mutex_t *m,
                     const struct timespec *t) {
  (void)c;
  (void)m;
  ++waits;
  deadline = *t;
  return ETIMEDOUT;
}
#define pthread_create         test_create
#define pthread_mutex_lock     test_lock
#define pthread_mutex_unlock   test_unlock
#define clock_gettime          test_clock
#define pthread_cond_timedwait test_wait
#endif
#define FIO_CORE
#define FIO_THREADS
#include "fio-stl/include.h"

int main(void) {
#if FIO_OS_POSIX || defined(__CYGWIN__)
  fio_thread_t t;
  fio_thread_mutex_t m = FIO_THREAD_MUTEX_INIT;
  fio_thread_cond_t c = {0};
  FIO_ASSERT(fio_thread_create(&t, NULL, NULL) == -1 && errno == EAGAIN,
             "create must report -1 and preserve native error");
  FIO_ASSERT(fio_thread_mutex_lock(&m) == -1 && errno == EDEADLK,
             "lock error contract");
  FIO_ASSERT(fio_thread_mutex_unlock(&m) == -1 && errno == EPERM,
             "unlock error contract");
  FIO_ASSERT(fio_thread_cond_timedwait(&c, &m, 1) != 0 &&
                 deadline.tv_sec == 101 && deadline.tv_nsec == 999999,
             "deadline must preserve submillisecond precision");
  waits = 0;
  clock_error = 1;
  FIO_ASSERT(fio_thread_cond_timedwait(&c, &m, 1) != 0 && !waits,
             "clock failure must not call timed wait");
  clock_error = 0;
  waits = 0;
  int r = fio_thread_cond_timedwait(&c, &m, SIZE_MAX);
  FIO_ASSERT(r != 0 && (!waits || deadline.tv_sec > 100),
             "huge finite timeout must not wrap into the past");
  clock_at_limit = 1;
  waits = 0;
  FIO_ASSERT(fio_thread_cond_timedwait(&c, &m, 1000) == EOVERFLOW && !waits,
             "unrepresentable time_t deadline must fail without waiting");
  FIO_ASSERT(fio_thread_priority_set(FIO_THREAD_PRIORITY_NORMAL) == -1 &&
                 errno == EPERM,
             "priority setter must normalize native failure");
#endif
  FIO_ASSERT(fio_thread_priority_set((fio_thread_priority_e)-2) == -1,
             "invalid negative priority must fail");
  FIO_ASSERT(fio_thread_priority_set((fio_thread_priority_e)INT_MAX) == -1,
             "invalid positive priority must fail");
  FIO_ASSERT(fio_thread_priority_set(FIO_THREAD_PRIORITY_ERROR) == -1,
             "error priority must fail");
  return 0;
}
