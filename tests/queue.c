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

int main(void) {
  test_queue_basic_ordering();
  test_queue_urgent_and_recursive_tasks();
  test_timer_queue();
  return 0;
}
