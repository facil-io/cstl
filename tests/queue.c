/* *****************************************************************************
Test - queue and timer queue (`102 queue.h`)
***************************************************************************** */
#include "test-helpers.h"

#define FIO_QUEUE
#include FIO_INCLUDE_FILE

#define FIO___QUEUE_TEST_COUNT (FIO_QUEUE_TASKS_PER_ALLOC * 4)

typedef struct {
  fio_queue_s *q;
  uintptr_t *counter;
  size_t remaining;
} fio___queue_test_s;

FIO_SFUNC void fio___queue_increment_task(void *counter_, void *unused_) {
  (void)unused_;
  fio_atomic_add((uintptr_t *)counter_, 1);
}

FIO_SFUNC void fio___queue_order_task(void *expected1_, void *expected2_) {
  static intptr_t counter = 0;
  if (!expected1_ && !expected2_) {
    counter = 0;
    return;
  }
  FIO_ASSERT((intptr_t)expected1_ == counter + 1,
             "udata1 value error in queued task");
  FIO_ASSERT((intptr_t)expected2_ == counter + 2,
             "udata2 value error in queued task");
  ++counter;
}

FIO_SFUNC void fio___queue_schedule_more(void *info_, void *unused_) {
  (void)unused_;
  fio___queue_test_s *info = (fio___queue_test_s *)info_;
  fio_atomic_add(info->counter, 1);
  if (!--info->remaining)
    return;
  if (info->remaining & 1) {
    FIO_ASSERT(!fio_queue_push(info->q, fio___queue_schedule_more, info, NULL),
               "queue push from task failed");
  } else {
    FIO_ASSERT(
        !fio_queue_push_urgent(info->q, fio___queue_schedule_more, info, NULL),
        "urgent queue push from task failed");
  }
}

FIO_SFUNC int fio___queue_timer_task(void *counter_, void *should_stop_) {
  fio_atomic_add((uintptr_t *)counter_, 1);
  return should_stop_ ? -1 : 0;
}

FIO_SFUNC void test_queue_basic_ordering(void) {
  fio_queue_s *q = fio_queue_new();
  FIO_ASSERT(q, "fio_queue_new returned NULL");

  fio___queue_order_task(NULL, NULL);
  for (size_t i = 0; i < FIO___QUEUE_TEST_COUNT; ++i) {
    FIO_ASSERT(!fio_queue_push(q,
                               .fn = fio___queue_order_task,
                               .udata1 = (void *)(i + 1),
                               .udata2 = (void *)(i + 2)),
               "fio_queue_push failed");
  }
  FIO_ASSERT(fio_queue_count(q) == FIO___QUEUE_TEST_COUNT,
             "queue count after pushes");
  fio_queue_perform_all(q);
  FIO_ASSERT(!fio_queue_count(q), "queue should be empty after perform_all");
  FIO_ASSERT(fio_queue_perform(q) == -1, "empty queue perform should fail");

  uintptr_t counter = 0;
  for (size_t i = 0; i < FIO___QUEUE_TEST_COUNT; ++i) {
    FIO_ASSERT(!fio_queue_push(q,
                               .fn = fio___queue_increment_task,
                               .udata1 = &counter),
               "fio_queue_push increment failed");
  }
  fio_queue_perform_all(q);
  FIO_ASSERT(counter == FIO___QUEUE_TEST_COUNT,
             "queued increment count mismatch");
  fio_queue_free(q);
}

FIO_SFUNC void test_queue_urgent_and_recursive_tasks(void) {
  fio_queue_s q;
  fio_queue_init(&q);

  for (size_t i = 0; i < (FIO_QUEUE_TASKS_PER_ALLOC * 3); ++i) {
    FIO_ASSERT(!fio_queue_push_urgent(&q,
                                      .fn = (void (*)(void *, void *))(i + 1),
                                      .udata1 = (void *)(i + 1)),
               "fio_queue_push_urgent failed");
  }
  for (size_t i = 0; i < (FIO_QUEUE_TASKS_PER_ALLOC * 3); ++i) {
    fio_queue_task_s t = fio_queue_pop(&q);
    FIO_ASSERT(t.fn && (size_t)t.udata1 == (FIO_QUEUE_TASKS_PER_ALLOC * 3) - i,
               "urgent queue pop ordering error");
  }
  FIO_ASSERT(fio_queue_pop(&q).fn == NULL, "pop should fail on empty queue");

  uintptr_t counter = 0;
  fio___queue_test_s info = {
      .q = &q,
      .counter = &counter,
      .remaining = 128,
  };
  FIO_ASSERT(!fio_queue_push(&q, fio___queue_schedule_more, &info, NULL),
             "initial recursive task push failed");
  while (fio_queue_count(&q))
    fio_queue_perform_all(&q);
  FIO_ASSERT(counter == 128, "recursive scheduling count mismatch");

  fio_queue_destroy(&q);
}

FIO_SFUNC void test_timer_queue(void) {
  fio_queue_s q;
  fio_queue_init(&q);
  fio_timer_queue_s timers = FIO_TIMER_QUEUE_INIT;
  uintptr_t counter = 0;

  fio_timer_schedule(&timers,
                     .fn = fio___queue_timer_task,
                     .udata1 = &counter,
                     .on_finish = fio___queue_increment_task,
                     .every = 1,
                     .repetitions = -1,
                     .start_at = fio_time_milli() - 10);
  FIO_ASSERT(counter == 0, "valid timer should be scheduled, not run inline");
  for (size_t i = 0; i < 8; ++i) {
    uint64_t now = fio_time_milli();
    fio_timer_push2queue(&q, &timers, now);
    fio_timer_push2queue(&q, &timers, now);
    FIO_ASSERT(fio_queue_count(&q) == 1,
               "timer should enqueue one pending task");
    fio_queue_perform(&q);
    FIO_ASSERT(!fio_queue_count(&q), "timer target queue should be empty");
    FIO_ASSERT(counter == i + 1, "timer run count mismatch");
  }
  fio_timer_destroy(&timers);
  FIO_ASSERT(counter == 9, "timer destroy should call on_finish");

  counter = 0;
  int64_t now = fio_time_milli();
  fio_timer_schedule(&timers,
                     .fn = fio___queue_timer_task,
                     .udata1 = &counter,
                     .on_finish = fio___queue_increment_task,
                     .every = 1,
                     .repetitions = 1,
                     .start_at = now - 10);
  fio_timer_push2queue(&q, &timers, now);
  FIO_ASSERT(fio_queue_count(&q) == 1, "single-use timer not enqueued");
  fio_queue_perform(&q);
  FIO_ASSERT(counter == 2, "single-use timer plus on_finish count mismatch");
  fio_timer_destroy(&timers);
  FIO_ASSERT(!timers.next, "timer queue should be empty after destroy");

  fio_queue_destroy(&q);
}

/* *****************************************************************************
Test - queue worker threads (buildup, work, teardown)
***************************************************************************** */

/* Counts worker thread start/end events via the global state callbacks.
   The worker threads force these events on buildup and teardown, so the
   deltas assert both correct buildup and complete teardown. */
static uintptr_t fio___queue_workers_started;
static uintptr_t fio___queue_workers_ended;

FIO_SFUNC void fio___queue_worker_started_callback(void *ignore) {
  (void)ignore;
  fio_atomic_add(&fio___queue_workers_started, 1);
}

FIO_SFUNC void fio___queue_worker_ended_callback(void *ignore) {
  (void)ignore;
  fio_atomic_add(&fio___queue_workers_ended, 1);
}

typedef struct {
  fio_queue_s *q;   /* target queue (recursive spawning) */
  uintptr_t done;   /* tasks performed */
  uintptr_t sum;    /* sum of performed task IDs (detects double-runs) */
  uintptr_t issued; /* task IDs issued (recursive spawning) */
  uintptr_t total;  /* expected task count */
} fio___queue_workers_test_s;

FIO_SFUNC uintptr_t fio___queue_workers_load(uintptr_t *p) {
  uintptr_t r;
  fio_atomic_load(r, p);
  return r;
}

/* Busy-waits (yielding) until `*counter` reaches `target`.
   Returns -1 on timeout so a broken queue fails the test instead of hanging. */
FIO_SFUNC int fio___queue_workers_wait(uintptr_t *counter,
                                       uintptr_t target,
                                       int64_t timeout_ms) {
  int64_t deadline = fio_time_milli() + timeout_ms;
  while (fio___queue_workers_load(counter) < target) {
    if (fio_time_milli() > deadline)
      return -1;
    fio_thread_yield();
  }
  return 0;
}

FIO_SFUNC void fio___queue_workers_counted_task(void *id_, void *info_) {
  fio___queue_workers_test_s *info = (fio___queue_workers_test_s *)info_;
  fio_atomic_add(&info->sum, (uintptr_t)id_);
  fio_atomic_add(&info->done, 1);
}

FIO_SFUNC uintptr_t fio___queue_workers_sum_up_to(uintptr_t n) {
  return n * (n + 1) / 2;
}

FIO_SFUNC void fio___queue_workers_assert_lifecycle(uintptr_t started,
                                                    uintptr_t ended) {
  FIO_ASSERT(!fio___queue_workers_wait(&fio___queue_workers_started,
                                       started,
                                       30000),
             "worker thread buildup timed out (%zu/%zu started)",
             (size_t)fio___queue_workers_load(&fio___queue_workers_started),
             (size_t)started);
  FIO_ASSERT(!fio___queue_workers_wait(&fio___queue_workers_ended,
                                       ended,
                                       30000),
             "worker thread teardown timed out (%zu/%zu ended)",
             (size_t)fio___queue_workers_load(&fio___queue_workers_ended),
             (size_t)ended);
}

FIO_SFUNC void test_queue_workers_noop_without_consumers(void) {
  fio_queue_s q;
  fio_queue_init(&q);
  /* stop / wake / join on a queue that never had workers must be no-ops */
  fio_queue_workers_stop(&q);
  fio_queue_workers_wake(&q);
  fio_queue_workers_join(&q);
  uintptr_t counter = 0;
  FIO_ASSERT(!fio_queue_push(&q, fio___queue_increment_task, &counter, NULL),
             "push after worker no-ops failed");
  FIO_ASSERT(!fio_queue_perform(&q), "queue unusable after worker no-ops");
  FIO_ASSERT(counter == 1, "task not performed after worker no-ops");
  /* adding zero workers is a valid buildup/teardown edge case */
  FIO_ASSERT(!fio_queue_workers_add(&q, 0), "fio_queue_workers_add(0) failed");
  fio_queue_workers_join(&q);
  FIO_ASSERT(!fio_queue_push(&q, fio___queue_increment_task, &counter, NULL),
             "push after zero-worker cycle failed");
  FIO_ASSERT(!fio_queue_perform(&q), "queue unusable after zero-worker cycle");
  FIO_ASSERT(counter == 2, "task not performed after zero-worker cycle");
  fio_queue_destroy(&q);
}

FIO_SFUNC void test_queue_workers_basic(void) {
  const uintptr_t total = FIO___QUEUE_TEST_COUNT;
  fio_queue_s *q = fio_queue_new();
  FIO_ASSERT(q, "fio_queue_new returned NULL");
  uintptr_t started = fio___queue_workers_load(&fio___queue_workers_started);
  uintptr_t ended = fio___queue_workers_load(&fio___queue_workers_ended);

  /* buildup */
  FIO_ASSERT(!fio_queue_workers_add(q, 4), "fio_queue_workers_add failed");
  fio___queue_workers_assert_lifecycle(started + 4, ended);

  /* work */
  fio___queue_workers_test_s info = {.total = total};
  for (uintptr_t i = 1; i <= total; ++i) {
    FIO_ASSERT(!fio_queue_push(q,
                               .fn = fio___queue_workers_counted_task,
                               .udata1 = (void *)i,
                               .udata2 = &info),
               "worker queue push failed");
  }
  FIO_ASSERT(!fio___queue_workers_wait(&info.done, total, 30000),
             "workers did not drain the queue (%zu/%zu)",
             (size_t)fio___queue_workers_load(&info.done),
             (size_t)total);
  FIO_ASSERT(fio___queue_workers_load(&info.sum) ==
                 fio___queue_workers_sum_up_to(total),
             "task sum mismatch - task(s) lost or performed twice");
  FIO_ASSERT(!fio_queue_count(q), "queue not empty after workers drained it");
  fio_queue_workers_wake(q); /* must be safe while workers are idle */

  /* teardown */
  fio_queue_workers_stop(q);
  fio_queue_workers_join(q);
  fio___queue_workers_assert_lifecycle(started + 4, ended + 4);

  /* queue remains usable after worker teardown */
  FIO_ASSERT(!fio_queue_push(q,
                             .fn = fio___queue_workers_counted_task,
                             .udata1 = (void *)0,
                             .udata2 = &info),
             "push after worker teardown failed");
  FIO_ASSERT(!fio_queue_perform(q), "perform after worker teardown failed");
  FIO_ASSERT(fio___queue_workers_load(&info.done) == total + 1,
             "task after worker teardown not performed");
  fio_queue_free(q);
}

FIO_SFUNC void test_queue_workers_backlog(void) {
  const uintptr_t total = 2048;
  fio_queue_s q;
  fio_queue_init(&q);
  fio___queue_workers_test_s info = {.total = total};
  /* push all tasks BEFORE the workers exist (backlog buildup) */
  for (uintptr_t i = 1; i <= total; ++i) {
    FIO_ASSERT(!fio_queue_push(&q,
                               .fn = fio___queue_workers_counted_task,
                               .udata1 = (void *)i,
                               .udata2 = &info),
               "backlog push failed");
  }
  uintptr_t started = fio___queue_workers_load(&fio___queue_workers_started);
  uintptr_t ended = fio___queue_workers_load(&fio___queue_workers_ended);
  FIO_ASSERT(!fio_queue_workers_add(&q, 3), "fio_queue_workers_add failed");
  FIO_ASSERT(!fio___queue_workers_wait(&info.done, total, 30000),
             "workers did not drain the backlog (%zu/%zu)",
             (size_t)fio___queue_workers_load(&info.done),
             (size_t)total);
  FIO_ASSERT(fio___queue_workers_load(&info.sum) ==
                 fio___queue_workers_sum_up_to(total),
             "backlog sum mismatch - task(s) lost or performed twice");
  fio_queue_workers_stop(&q);
  fio_queue_workers_join(&q);
  fio___queue_workers_assert_lifecycle(started + 3, ended + 3);
  fio_queue_destroy(&q);
}

/* NOTE: this test uses TWO worker groups (two manager threads) on one queue.
   It originally exposed a library defect (use-after-free in
   fio_queue_workers_join: joiners accessed thread-group objects on the
   manager threads' own stacks after the managers had returned). Fixed in
   the library via the stop-bit handshake + manager-side object lifetime.
   History: ./ai-research/2026-10-07 000 queue worker thread lifecycle.md */
FIO_SFUNC void test_queue_workers_multiple_groups(void) {
  const uintptr_t total = 2048;
  fio_queue_s q;
  fio_queue_init(&q);
  uintptr_t started = fio___queue_workers_load(&fio___queue_workers_started);
  uintptr_t ended = fio___queue_workers_load(&fio___queue_workers_ended);
  /* two worker groups (two manager threads) consume the same queue */
  FIO_ASSERT(!fio_queue_workers_add(&q, 2), "first group add failed");
  FIO_ASSERT(!fio_queue_workers_add(&q, 2), "second group add failed");
  fio___queue_workers_assert_lifecycle(started + 4, ended);
  fio___queue_workers_test_s info = {.total = total};
  for (uintptr_t i = 1; i <= total; ++i) {
    FIO_ASSERT(!fio_queue_push(&q,
                               .fn = fio___queue_workers_counted_task,
                               .udata1 = (void *)i,
                               .udata2 = &info),
               "multi-group push failed");
  }
  FIO_ASSERT(!fio___queue_workers_wait(&info.done, total, 30000),
             "worker groups did not drain the queue (%zu/%zu)",
             (size_t)fio___queue_workers_load(&info.done),
             (size_t)total);
  FIO_ASSERT(fio___queue_workers_load(&info.sum) ==
                 fio___queue_workers_sum_up_to(total),
             "multi-group sum mismatch - task(s) lost or performed twice");
  /* stop + join must terminate BOTH manager threads (list iteration) */
  fio_queue_workers_stop(&q);
  fio_queue_workers_join(&q);
  fio___queue_workers_assert_lifecycle(started + 4, ended + 4);
  FIO_ASSERT(FIO_LIST_IS_EMPTY(&q.consumers),
             "consumer list not empty after joining all worker groups");
  fio_queue_destroy(&q);
}

/* A task that performs its own bookkeeping and then spawns the next task,
   so workers produce and consume concurrently until `total` tasks ran. */
FIO_SFUNC void fio___queue_workers_spawn_task(void *id_, void *info_) {
  fio___queue_workers_test_s *info = (fio___queue_workers_test_s *)info_;
  fio_atomic_add(&info->sum, (uintptr_t)id_);
  fio_atomic_add(&info->done, 1);
  uintptr_t id = fio_atomic_add_fetch(&info->issued, 1);
  if (id > info->total)
    return;
  int err = (id & 1)
                ? fio_queue_push(info->q,
                                 .fn = fio___queue_workers_spawn_task,
                                 .udata1 = (void *)id,
                                 .udata2 = info)
                : fio_queue_push_urgent(info->q,
                                        .fn = fio___queue_workers_spawn_task,
                                        .udata1 = (void *)id,
                                        .udata2 = info);
  FIO_ASSERT(!err, "recursive worker push failed");
}

FIO_SFUNC void test_queue_workers_concurrent_producers(void) {
  const uintptr_t total = (1ULL << 14);
  const uintptr_t seed = 64;
  fio_queue_s q;
  fio_queue_init(&q);
  fio___queue_workers_test_s info = {.total = total, .issued = seed, .q = &q};
  uintptr_t started = fio___queue_workers_load(&fio___queue_workers_started);
  uintptr_t ended = fio___queue_workers_load(&fio___queue_workers_ended);
  FIO_ASSERT(!fio_queue_workers_add(&q, 8), "fio_queue_workers_add failed");
  fio___queue_workers_assert_lifecycle(started + 8, ended);
  for (uintptr_t i = 1; i <= seed; ++i) {
    FIO_ASSERT(!fio_queue_push(&q,
                               .fn = fio___queue_workers_spawn_task,
                               .udata1 = (void *)i,
                               .udata2 = &info),
               "seed push failed");
  }
  FIO_ASSERT(!fio___queue_workers_wait(&info.done, total, 60000),
             "concurrent workers did not complete all tasks (%zu/%zu)",
             (size_t)fio___queue_workers_load(&info.done),
             (size_t)total);
  FIO_ASSERT(fio___queue_workers_load(&info.sum) ==
                 fio___queue_workers_sum_up_to(total),
             "concurrent sum mismatch - task(s) lost or performed twice");
  FIO_ASSERT(!fio_queue_count(&q),
             "queue not empty after concurrent drain (count: %u)",
             fio_queue_count(&q));
  fio_queue_workers_stop(&q);
  fio_queue_workers_join(&q);
  fio___queue_workers_assert_lifecycle(started + 8, ended + 8);
  fio_queue_destroy(&q);
}

FIO_SFUNC void test_queue_workers_repeated_cycles(void) {
  fio_queue_s q;
  fio_queue_init(&q);
  uintptr_t started = fio___queue_workers_load(&fio___queue_workers_started);
  uintptr_t ended = fio___queue_workers_load(&fio___queue_workers_ended);
  for (uintptr_t round = 0; round < 6; ++round) {
    const uintptr_t total = 512;
    fio___queue_workers_test_s info = {.total = total};
    FIO_ASSERT(!fio_queue_workers_add(&q, 2),
               "fio_queue_workers_add failed on round %zu",
               (size_t)round);
    for (uintptr_t i = 1; i <= total; ++i) {
      FIO_ASSERT(!fio_queue_push(&q,
                                 .fn = fio___queue_workers_counted_task,
                                 .udata1 = (void *)i,
                                 .udata2 = &info),
                 "cycle push failed");
    }
    FIO_ASSERT(!fio___queue_workers_wait(&info.done, total, 30000),
               "cycle %zu: workers did not drain the queue (%zu/%zu)",
               (size_t)round,
               (size_t)fio___queue_workers_load(&info.done),
               (size_t)total);
    FIO_ASSERT(fio___queue_workers_load(&info.sum) ==
                   fio___queue_workers_sum_up_to(total),
               "cycle %zu: sum mismatch - task(s) lost or performed twice",
               (size_t)round);
    fio_queue_workers_stop(&q);
    fio_queue_workers_join(&q);
  }
  /* 6 rounds x 2 workers must have started AND ended, nothing left behind */
  fio___queue_workers_assert_lifecycle(started + 12, ended + 12);
  FIO_ASSERT(FIO_LIST_IS_EMPTY(&q.consumers),
             "consumer list not empty after repeated worker cycles");
  fio_queue_destroy(&q);
}

FIO_SFUNC void test_queue_workers_destroy_teardown(void) {
  const uintptr_t total = 1024;
  fio_queue_s *q = fio_queue_new();
  FIO_ASSERT(q, "fio_queue_new returned NULL");
  uintptr_t started = fio___queue_workers_load(&fio___queue_workers_started);
  uintptr_t ended = fio___queue_workers_load(&fio___queue_workers_ended);
  FIO_ASSERT(!fio_queue_workers_add(q, 3), "fio_queue_workers_add failed");
  fio___queue_workers_test_s info = {.total = total};
  for (uintptr_t i = 1; i <= total; ++i) {
    FIO_ASSERT(!fio_queue_push(q,
                               .fn = fio___queue_workers_counted_task,
                               .udata1 = (void *)i,
                               .udata2 = &info),
               "push failed");
  }
  FIO_ASSERT(!fio___queue_workers_wait(&info.done, total, 30000),
             "workers did not drain the queue (%zu/%zu)",
             (size_t)fio___queue_workers_load(&info.done),
             (size_t)total);
  /* teardown without explicit stop/join: fio_queue_free (via
     fio_queue_destroy) must stop and join the idle worker group itself. */
  fio_queue_free(q);
  fio___queue_workers_assert_lifecycle(started + 3, ended + 3);
}

int main(void) {
  fio_state_callback_add(FIO_CALL_ON_WORKER_THREAD_START,
                         fio___queue_worker_started_callback,
                         NULL);
  fio_state_callback_add(FIO_CALL_ON_WORKER_THREAD_END,
                         fio___queue_worker_ended_callback,
                         NULL);
  test_queue_basic_ordering();
  test_queue_urgent_and_recursive_tasks();
  test_timer_queue();
  test_queue_workers_noop_without_consumers();
  test_queue_workers_basic();
  test_queue_workers_backlog();
  test_queue_workers_concurrent_producers();
  test_queue_workers_repeated_cycles();
  test_queue_workers_destroy_teardown();
  FIO_ASSERT(fio___queue_workers_load(&fio___queue_workers_started) ==
                 fio___queue_workers_load(&fio___queue_workers_ended),
             "worker thread lifecycle leak: %zu started vs %zu ended",
             (size_t)fio___queue_workers_load(&fio___queue_workers_started),
             (size_t)fio___queue_workers_load(&fio___queue_workers_ended));
  test_queue_workers_multiple_groups();
  return 0;
}
