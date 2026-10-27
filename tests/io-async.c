/* *****************************************************************************
Test - IO Async Queue lifecycle (402 io types / 403 io reactor)

* No async task is still running during ON_STOP or after the reactor stops.
* Tasks pushed by another thread while the reactor stops are never lost: they
  run on the next start, or at exit.
* start / stop cycles leave no worker thread groups behind.
* Forking workers with busy async threads doesn't deadlock (watchdog) and no
  async task is running at fork time; spawning workers from the first queue
  pass (`fio_io_add_workers` before start) keeps the master's async threads.
* A late task holding an IO reference runs (and frees the IO) on the next start
  and at exit; the exit drain loops until every queue is empty, including
  tasks pushed by tasks, timer `on_finish` and global env `on_close`.
* A forked child resets inherited async state (locks held by vanished parent
  threads, stale consumers) and still runs the inherited pending tasks.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_IO
#include FIO_INCLUDE_FILE

#if FIO_OS_POSIX
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

/* TSan can't start threads after a multi-threaded fork: the fork-only cases
 * (and their helpers) are compiled out, so strict builds see no unused code */
#if defined(__SANITIZE_THREAD__)
#define TEST_TSAN 1
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define TEST_TSAN 1
#endif
#endif

static fio_io_async_s ASYNC = FIO_IO_ASYN_INIT;

static volatile size_t test_ran = 0;
static volatile size_t test_slow_done = 0;

static int test_async_idle(void) {
  return ASYNC.queue.consumers.next == &ASYNC.queue.consumers;
}

static void test_count_task(void *a, void *b) {
  fio_atomic_add(&test_ran, 1);
  (void)a, (void)b;
}

static void test_slow_task(void *a, void *b) {
  FIO_THREAD_WAIT(((size_t)(uintptr_t)a) * 1000000ULL); /* milliseconds */
  fio_atomic_add(&test_slow_done, 1);
  (void)b;
}

/* stops the reactor after `u1` ticks, checking the running async state. */
static int test_stop_after(void *u1, void *u2) {
  static size_t ticks = 0;
  if (!fio_io_is_master())
    return 0;
  /* `u2`: timers may also run while spawning workers (async stopped) */
  FIO_ASSERT(u2 || (fio_io_async_queue(&ASYNC) == &ASYNC.queue &&
                    !test_async_idle()),
             "running reactor should route to async worker threads");
  if (++ticks < (size_t)(uintptr_t)u1)
    return 0;
  ticks = 0;
  fio_io_stop();
  return -1;
}

static void test_cycle_workers(size_t ticks, size_t every, int workers) {
  fio_io_run_every(.fn = test_stop_after,
                   .udata1 = (void *)(uintptr_t)ticks,
                   .udata2 = (void *)(uintptr_t)workers,
                   .every = (uint32_t)every,
                   .repetitions = -1);
  fio_io_start(workers);
  FIO_ASSERT(test_async_idle(), "stopped reactor left async worker threads");
  FIO_ASSERT(fio_io_async_queue(&ASYNC) == fio_io_queue(),
             "stopped reactor should route async tasks to the IO queue");
}
#define test_cycle(ticks, every) test_cycle_workers((ticks), (every), 0)

/* (a) + (c): stop joins running tasks; repeated cycles leave no threads */
static int test_push_slow_then_stop(void *u1, void *u2) {
  (void)u1, (void)u2;
  for (size_t i = 0; i < 2; ++i) /* one per thread: nothing left to drain */
    FIO_ASSERT(!fio_io_async(&ASYNC, test_slow_task, (void *)(uintptr_t)50),
               "async push failed");
  FIO_THREAD_WAIT(5000000); /* let the worker threads pick them up */
  fio_io_stop();
  return -1;
}

static size_t test_done_on_stop = 0;
static void test_on_stop(void *ignr) {
  test_done_on_stop = test_slow_done;
  (void)ignr;
}

static void test_stop_joins(void) {
  test_slow_done = 0;
  fio_state_callback_add(FIO_CALL_ON_STOP, test_on_stop, NULL);
  fio_io_run_every(.fn = test_push_slow_then_stop, .every = 1);
  fio_io_start(0);
  fio_state_callback_remove(FIO_CALL_ON_STOP, test_on_stop, NULL);
  FIO_ASSERT(test_done_on_stop == 2,
             "async tasks still running during ON_STOP (%zu/2)",
             test_done_on_stop);
  FIO_ASSERT(test_slow_done == 2,
             "async tasks still running after reactor stop (%zu/2)",
             (size_t)test_slow_done);
  FIO_ASSERT(test_async_idle(), "stop left async worker threads");
  for (size_t i = 0; i < 3; ++i)
    test_cycle(2, 2);
  fprintf(stderr, "* async stop joins workers; start/stop cycles idle: OK\n");
}

/* (b): a producer racing reactor stop never loses a task */
static volatile size_t test_pushed = 0;
static volatile int test_producer_stop = 0;

static void *test_producer(void *ignr) {
  while (!fio_atomic_add(&test_producer_stop, 0)) {
    if (!fio_io_async(&ASYNC, test_count_task))
      fio_atomic_add(&test_pushed, 1);
    if (!(test_pushed & 63))
      FIO_THREAD_RESCHEDULE();
  }
  return ignr;
}

static void test_racing_producer(void) {
  test_ran = test_pushed = 0;
  test_producer_stop = 0;
  fio_thread_t thr;
  FIO_ASSERT(!fio_thread_create(&thr, test_producer, NULL), "thread failed");
  test_cycle(10, 2); /* producer keeps pushing across the stop */
  fio_atomic_exchange(&test_producer_stop, 1);
  fio_thread_join(&thr);
  /* simulate a producer that read the route before the stop switched it */
  FIO_ASSERT(!fio_queue_push(&ASYNC.queue, test_count_task), "push failed");
  fio_atomic_add(&test_pushed, 1);
  test_cycle(1, 1); /* late tasks run on the next start */
  FIO_ASSERT(test_ran == test_pushed,
             "async tasks lost across reactor stop (%zu/%zu)",
             (size_t)test_ran,
             (size_t)test_pushed);
  fprintf(stderr,
          "* %zu async tasks racing reactor stop all performed: OK\n",
          (size_t)test_pushed);
}

/* (b) at exit: late pushes run during at-exit cleanup */
static int test_exit_fd = -1;
static void test_exit_task(void *a, void *b) {
  char c = (char)(uintptr_t)a;
  if (test_exit_fd != -1) /* the parent's copy of an inherited task: no pipe */
    FIO_ASSERT(write(test_exit_fd, &c, 1) == 1, "pipe write failed");
  (void)b;
}

/* reads a forked child's pipe until EOF and checks it exited cleanly */
static size_t test_child_read(pid_t child, int fd, char *buf, size_t len) {
  size_t total = 0;
  ssize_t r;
  while (total < len && (r = read(fd, buf + total, len - total)) > 0)
    total += (size_t)r;
  close(fd);
  int status = 0;
  FIO_ASSERT(waitpid(child, &status, 0) == child, "waitpid failed");
  FIO_ASSERT(WIFEXITED(status) && !WEXITSTATUS(status), "child exit failed");
  return total;
}
static int test_char_cmp(const void *a, const void *b) {
  return (int)*(const char *)a - (int)*(const char *)b;
}

/* a late task holding an IO reference: run, then release the reference */
static fio_io_protocol_s TEST_IO_PROTOCOL;
static volatile size_t test_io_task_ran = 0;
static void test_io_ref_task(void *io_, void *c) {
  fio_atomic_add(&test_io_task_ran, 1);
  test_exit_task(c, NULL);
  fio_io_free((fio_io_s *)io_);
}
/* attaches one end of a socket pair (the reactor owns it); returns the IO */
static fio_io_s *test_io_attach_pair(int *peer) {
  int sv[2];
  FIO_ASSERT(!socketpair(AF_UNIX, SOCK_STREAM, 0, sv), "socketpair failed");
  fio_io_s *io = fio_io_attach_fd(sv[0], &TEST_IO_PROTOCOL, NULL, NULL);
  FIO_ASSERT(io, "attach failed");
  *peer = sv[1];
  return io;
}

/* a late task holding an IO reference runs on the next start (IO freed) */
static void test_late_io_task_next_start(void) {
  int peer;
  test_io_task_ran = 0;
  fio_io_s *io = test_io_attach_pair(&peer);
  /* a stale producer's push to the stopped async queue */
  FIO_ASSERT(!fio_queue_push(&ASYNC.queue, test_io_ref_task, fio_io_dup(io)),
             "push failed");
  test_cycle(2, 1);
  close(peer);
  FIO_ASSERT(test_io_task_ran == 1,
             "late IO task not performed on the next start (%zu)",
             (size_t)test_io_task_ran);
  FIO_ASSERT(!FIO_LEAK_COUNTER_COUNT(fio___io),
             "late IO task: IO leaked after the next start (%zu)",
             (size_t)FIO_LEAK_COUNTER_COUNT(fio___io));
  fprintf(stderr, "* late async task holding an IO runs on next start: OK\n");
}

/* exit drain: tasks pushing tasks (to either queue) and late callbacks */
static void test_chain_c(void *a, void *b) {
  test_exit_task((void *)(uintptr_t)'C', NULL);
  fio_io_async(&ASYNC, test_exit_task, (void *)(uintptr_t)'D'); /* routed */
  (void)a, (void)b;
}
static void test_chain_b(void *a, void *b) {
  test_exit_task((void *)(uintptr_t)'B', NULL);
  fio_queue_push(&ASYNC.queue, test_chain_c); /* stale async pointer */
  (void)a, (void)b;
}
static void test_chain_a(void *a, void *b) {
  test_exit_task((void *)(uintptr_t)'A', NULL);
  fio_io_defer(test_chain_b, NULL, NULL);
  (void)a, (void)b;
}
static void test_env_on_close(void *io) { /* global env: destroyed last */
  fio_queue_push(&ASYNC.queue, test_io_ref_task, io, (void *)(uintptr_t)'F');
}
static int test_timer_never(void *a, void *b) {
  (void)a, (void)b;
  return 0;
}
/* async timers are destroyed at exit (`on_finish`) */
static void test_timer_on_finish(void *a, void *b) {
  fio_queue_push(&ASYNC.queue, test_exit_task, (void *)(uintptr_t)'H');
  (void)a, (void)b;
}
static void test_exit_leaks(void *ignr) { /* AFTER_EXIT: after the drain */
  char c = (FIO_LEAK_COUNTER_COUNT(fio___io) ||
            FIO_LEAK_COUNTER_COUNT(fio_queue_task_rings))
               ? 'L'
               : 'K';
  test_exit_task((void *)(uintptr_t)c, NULL);
  (void)ignr;
}

static void test_exit_drains_all(void) {
  int fds[2];
  FIO_ASSERT(!pipe(fds), "pipe failed");
  pid_t child = fork();
  FIO_ASSERT(child != -1, "fork failed");
  if (!child) {
    close(fds[0]);
    test_exit_fd = fds[1];
    int peer1, peer2;
    fio_io_s *io1 = test_io_attach_pair(&peer1);
    fio_io_s *io2 = test_io_attach_pair(&peer2);
    fio_queue_push(&ASYNC.queue,
                   test_io_ref_task,
                   fio_io_dup(io1),
                   (void *)(uintptr_t)'i');
    fio_queue_push(&ASYNC.queue, test_chain_a);
    fio_io_env_set(NULL,
                   .name = FIO_BUF_INFO1((char *)"test-exit-drain"),
                   .udata = fio_io_dup(io2),
                   .on_close = test_env_on_close);
    fio_io_async_every(&ASYNC,
                       .fn = test_timer_never,
                       .on_finish = test_timer_on_finish,
                       .every = (1U << 30),
                       .repetitions = -1);
    fio_state_callback_add(FIO_CALL_AFTER_EXIT, test_exit_leaks, NULL);
    exit(0);
  }
  close(fds[1]);
  char buf[16] = {0};
  size_t total = test_child_read(child, fds[0], buf, sizeof(buf) - 1);
  qsort(buf, total, 1, test_char_cmp);
  FIO_ASSERT(total == 8 && !memcmp(buf, "ABCDFHKi", 8),
             "exit drain dropped tasks or leaked (got \"%s\", expected "
             "\"ABCDFHKi\")",
             buf);
  fprintf(stderr, "* exit drains every queue (tasks re-push, IO refs): OK\n");
}

#ifndef TEST_TSAN /* fork-only cases (see TEST_TSAN) */
/* forked child: inherited async state is reset, pending tasks still run */
static volatile int test_gate = 0;
static volatile int test_lock_held = 0;
static void test_gate_task(void *a, void *b) {
  while (!fio_atomic_add(&test_gate, 0))
    FIO_THREAD_RESCHEDULE();
  (void)a, (void)b;
}
#if FIO_USE_THREAD_MUTEX /* the queue lock type (see FIO___LOCK_TYPE) */
#define TEST_QUEUE_LOCK(l)   fio_thread_mutex_lock(&(l))
#define TEST_QUEUE_UNLOCK(l) fio_thread_mutex_unlock(&(l))
#else
#define TEST_QUEUE_LOCK(l)   fio_lock(&(l))
#define TEST_QUEUE_UNLOCK(l) fio_unlock(&(l))
#endif
static void *test_lock_holder(void *ignr) {
  TEST_QUEUE_LOCK(ASYNC.queue.lock);
  fio_atomic_exchange(&test_lock_held, 1);
  while (fio_atomic_add(&test_lock_held, 0) == 1)
    FIO_THREAD_RESCHEDULE();
  TEST_QUEUE_UNLOCK(ASYNC.queue.lock);
  return ignr;
}

static void test_fork_child_async_state(void) {
  test_gate = 0;
  fio___io_async_start(&ASYNC); /* as if the reactor were running */
  FIO_ASSERT(!test_async_idle(), "async workers should be running");
  for (size_t i = 0; i < 2; ++i) /* occupy both workers */
    FIO_ASSERT(!fio_io_async(&ASYNC, test_gate_task), "push failed");
  while (fio_queue_count(&ASYNC.queue))
    FIO_THREAD_RESCHEDULE();
  /* pending at fork: inherited by the child */
  FIO_ASSERT(!fio_io_async(&ASYNC, test_exit_task, (void *)(uintptr_t)'T'),
             "push failed");
  fio_thread_t holder;
  FIO_ASSERT(!fio_thread_create(&holder, test_lock_holder, NULL), "thread");
  while (!fio_atomic_add(&test_lock_held, 0))
    FIO_THREAD_RESCHEDULE();
  int fds[2];
  FIO_ASSERT(!pipe(fds), "pipe failed");
  pid_t child = fork();
  FIO_ASSERT(child != -1, "fork failed");
  if (!child) {
    alarm(20); /* a held lock or stale consumer would hang the child */
    close(fds[0]);
    test_exit_fd = fds[1];
    fio_state_callback_force(FIO_CALL_IN_CHILD);
    FIO_ASSERT(test_async_idle() &&
                   fio_io_async_queue(&ASYNC) == fio_io_queue(),
               "child: async queue should be stopped");
    fio_queue_push(&ASYNC.queue, test_exit_task, (void *)(uintptr_t)'S');
    fio_io_async(&ASYNC, test_exit_task, (void *)(uintptr_t)'R');
    /* leak counters of the parent's live worker threads are inherited */
    FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL);
    exit(0);
  }
  close(fds[1]);
  char buf[8] = {0};
  size_t total = test_child_read(child, fds[0], buf, sizeof(buf) - 1);
  fio_atomic_exchange(&test_lock_held, 2);
  fio_thread_join(&holder);
  fio_atomic_exchange(&test_gate, 1);
  fio___io_async_stop(&ASYNC); /* the parent runs its own copy of 'T' */
  FIO_ASSERT(test_async_idle(), "async stop left worker threads");
  test_lock_held = 0;
  qsort(buf, total, 1, test_char_cmp);
  FIO_ASSERT(total == 3 && !memcmp(buf, "RST", 3),
             "forked child dropped async tasks (got \"%s\", expected \"RST\")",
             buf);
  fprintf(stderr, "* forked child resets async state, runs inherited: OK\n");
}

/* spawning from the first queue pass keeps the master's async threads */
static void test_add_workers_before_start(void) {
  fio_io_add_workers(1);
  test_cycle_workers(3, 5, 0); /* asserts the master's route while running */
  fprintf(stderr, "* workers added before start keep async threads: OK\n");
}
#endif /* TEST_TSAN */

static void test_late_push_runs_at_exit(void) {
  int fds[2];
  FIO_ASSERT(!pipe(fds), "pipe failed");
  pid_t child = fork();
  FIO_ASSERT(child != -1, "fork failed");
  if (!child) {
    close(fds[0]);
    test_exit_fd = fds[1];
    /* a stale producer pushing to the stopped queue, and a routed push */
    fio_queue_push(&ASYNC.queue, test_exit_task, (void *)(uintptr_t)'a');
    fio_io_async(&ASYNC, test_exit_task, (void *)(uintptr_t)'b');
    exit(0);
  }
  close(fds[1]);
  char buf[4] = {0};
  ssize_t total = 0, r;
  while ((r = read(fds[0], buf + total, sizeof(buf) - (size_t)total)) > 0)
    total += r;
  close(fds[0]);
  int status = 0;
  FIO_ASSERT(waitpid(child, &status, 0) == child, "waitpid failed");
  FIO_ASSERT(WIFEXITED(status) && !WEXITSTATUS(status), "child exit failed");
  FIO_ASSERT(total == 2, "late async tasks not performed at exit (%zd)", total);
  fprintf(stderr, "* late async tasks performed at exit: OK\n");
}

#ifndef TEST_TSAN /* fork-only cases (see TEST_TSAN) */
/* forking workers while async threads are busy */
static volatile size_t test_fork_checks = 0;
static volatile size_t test_slow_pushed = 0;

static void test_fork_in_master(void *ignr) {
  FIO_ASSERT(test_slow_done == test_slow_pushed && test_async_idle(),
             "async tasks running at fork (%zu/%zu)",
             (size_t)test_slow_done,
             (size_t)test_slow_pushed);
  ++test_fork_checks;
  (void)ignr;
}
static void test_fork_worker_start(void *ignr) {
  for (size_t i = 0; i < 16; ++i) /* busy async threads at SIGTERM */
    fio_io_async(&ASYNC, test_slow_task, (void *)(uintptr_t)10);
  (void)ignr;
}
/* adds a worker process while the master's async threads are busy */
static int test_fork_driver(void *u1, void *u2) {
  (void)u1, (void)u2;
  if (!fio_io_is_master())
    return 0;
  for (size_t i = 0; i < 40; ++i)
    if (!fio_io_async(&ASYNC, test_slow_task, (void *)(uintptr_t)3))
      fio_atomic_add(&test_slow_pushed, 1);
  fio_io_add_workers(1);
  return -1;
}

static void test_fork_workers(void) {
  test_slow_done = test_slow_pushed = 0;
  fio_state_callback_add(FIO_CALL_IN_MASTER, test_fork_in_master, NULL);
  fio_state_callback_add(FIO_CALL_ON_START, test_fork_worker_start, NULL);
  fio_io_run_every(.fn = test_fork_driver, .every = 5);
  test_cycle_workers(30, 10, 1);
  fio_state_callback_remove(FIO_CALL_IN_MASTER, test_fork_in_master, NULL);
  fio_state_callback_remove(FIO_CALL_ON_START, test_fork_worker_start, NULL);
  FIO_ASSERT(test_fork_checks == 2,
             "expected 2 forked workers (%zu)",
             (size_t)test_fork_checks);
  FIO_ASSERT(test_slow_pushed == 40 && test_slow_done == 40,
             "master async tasks lost (%zu/%zu)",
             (size_t)test_slow_done,
             (size_t)test_slow_pushed);
  fprintf(stderr, "* forking workers with busy async threads: OK\n");
}
#endif /* TEST_TSAN */

int main(void) {
  alarm(60); /* watchdog: a deadlock fails the test */
  fio_io_async_attach(&ASYNC, 2);
  FIO_ASSERT(fio_io_async_queue(&ASYNC) == fio_io_queue(),
             "idle reactor should route async tasks to the IO queue");
  test_stop_joins();
  test_racing_producer();
  test_late_push_runs_at_exit();
  test_late_io_task_next_start();
  test_exit_drains_all();
#ifndef TEST_TSAN /* TSan can't start threads after a multi-threaded fork */
  test_fork_workers();
  test_add_workers_before_start();
  test_fork_child_async_state();
#else
  fprintf(stderr, "* forking workers skipped under ThreadSanitizer\n");
#endif
  return 0;
}
#else
int main(void) {
  fprintf(stderr, "* IO async lifecycle tests skipped on non-POSIX\n");
  return 0;
}
#endif
