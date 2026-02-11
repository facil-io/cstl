/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_QUEUE
#include FIO_INCLUDE_FILE

#ifndef FIO___QUEUE_TEST_PRINT
#define FIO___QUEUE_TEST_PRINT 1
#endif

#define FIO___QUEUE_TOTAL_COUNT (512 * 1024)

typedef struct {
  fio_queue_s *q;
  uintptr_t count;
  uintptr_t *counter;
} fio___queue_test_s;

FIO_SFUNC void fio___queue_test_sample_task(void *i_count, void *unused2) {
  (void)(unused2);
  fio_atomic_add((uintptr_t *)i_count, 1);
}

FIO_SFUNC void fio___queue_test_counter_task(void *i_count1, void *i_count2) {
  static intptr_t counter = 0;
  if (!i_count1 && !i_count2) {
    counter = 0;
    return;
  }
  FIO_ASSERT((intptr_t)i_count1 == (intptr_t)counter + 1,
             "udata1 value error in task");
  FIO_ASSERT((intptr_t)i_count2 == (intptr_t)counter + 2,
             "udata2 value error in task");
  ++counter;
}

FIO_SFUNC void fio___queue_test_sched_sample_task(void *t_, void *i_count) {
  fio___queue_test_s *t = (fio___queue_test_s *)t_;
  size_t i = (size_t)(uintptr_t)i_count;
  FIO_ASSERT(!fio_queue_push(t->q,
                             .fn = fio___queue_test_sample_task,
                             .udata1 = t->counter),
             "Couldn't push task!");
  --i;
  if (!i)
    return;
  if ((i & 1)) {
    FIO_ASSERT(
        !fio_queue_push(t->q, fio___queue_test_sched_sample_task, t, (void *)i),
        "Couldn't push task!");
  } else {
    FIO_ASSERT(!fio_queue_push_urgent(t->q,
                                      fio___queue_test_sched_sample_task,
                                      t,
                                      (void *)i),
               "Couldn't push task!");
  }
}

FIO_SFUNC int fio___queue_test_timer_task(void *i_count, void *unused2) {
  fio_atomic_add((uintptr_t *)i_count, 1);
  return (unused2 ? -1 : 0);
}

int main(void) {
  /* ************** testing queue ************** */
  fio_queue_s *q = fio_queue_new();
  fio_queue_s q2;

  fprintf(stderr, "\t- size of queue object (fio_queue_s): %zu\n", sizeof(*q));
  fprintf(stderr,
          "\t- size of queue ring buffer (per allocation): %zu\n",
          sizeof(q->mem));
  fprintf(stderr,
          "\t- event slots per queue allocation: %zu\n",
          (size_t)FIO_QUEUE_TASKS_PER_ALLOC);

  /* test task user data integrity. */
  fio___queue_test_counter_task(NULL, NULL);
  for (size_t i = 0; i < (FIO_QUEUE_TASKS_PER_ALLOC << 2); ++i) {
    fio_queue_push(q,
                   .fn = fio___queue_test_counter_task,
                   .udata1 = (void *)(i + 1),
                   .udata2 = (void *)(i + 2));
  }
  fio_queue_perform_all(q);
  fio_queue_perform_all(q);
  for (size_t i = (FIO_QUEUE_TASKS_PER_ALLOC << 2);
       i < (FIO_QUEUE_TASKS_PER_ALLOC << 3);
       ++i) {
    fio_queue_push(q,
                   .fn = fio___queue_test_counter_task,
                   .udata1 = (void *)(i + 1),
                   .udata2 = (void *)(i + 2));
  }
  fio_queue_perform_all(q);
  fio_queue_perform_all(q);
  FIO_ASSERT(!fio_queue_count(q) && fio_queue_perform(q) == -1,
             "fio_queue_perform_all didn't perform all");

  const size_t max_threads = 12; // assumption / pure conjuncture...
  uintptr_t i_count;
  uint64_t start, end;
  i_count = 0;
  start = fio_time_milli();
  for (size_t i = 0; i < FIO___QUEUE_TOTAL_COUNT; i++) {
    fio___queue_test_sample_task(&i_count, NULL);
  }
  end = fio_time_milli();
  if (FIO___QUEUE_TEST_PRINT) {
    fprintf(stderr,
            "\t- Queueless (direct call) counter: %lu ms with i_count = %lu\n",
            (unsigned long)(end - start),
            (unsigned long)i_count);
  }
  size_t i_count_should_be = i_count;
  i_count = 0;
  start = fio_time_milli();
  for (size_t i = 0; i < FIO___QUEUE_TOTAL_COUNT; i++) {
    fio_queue_push(q,
                   .fn = fio___queue_test_sample_task,
                   .udata1 = (void *)&i_count);
  }
  fio_queue_perform_all(q);
  fio_queue_perform_all(q);
  fio_queue_perform_all(q);
  end = fio_time_milli();
  if (FIO___QUEUE_TEST_PRINT) {
    fprintf(stderr,
            "\t- single task counter: %lu ms with i_count = %lu\n",
            (unsigned long)(end - start),
            (unsigned long)i_count);
  }
  FIO_ASSERT(i_count == i_count_should_be, "ERROR: queue count invalid\n");

  if (FIO___QUEUE_TEST_PRINT) {
    fprintf(stderr, "\n");
  }

  for (size_t i = 1; i < 32 && FIO___QUEUE_TOTAL_COUNT >> i; ++i) {
    fio___queue_test_s info = {
        .q = q,
        .count = (uintptr_t)(FIO___QUEUE_TOTAL_COUNT >> i),
        .counter = &i_count,
    };
    const size_t tasks = 1 << i;
    i_count = 0;
    start = fio_time_milli();
    for (size_t j = 0; j < tasks; ++j) {
      fio_queue_push(q,
                     fio___queue_test_sched_sample_task,
                     (void *)&info,
                     (void *)info.count);
    }
    FIO_ASSERT(fio_queue_count(q), "tasks not counted?!");
    {
      const size_t t_count = (i % max_threads) + 1;
      if (0) {
        fio_queue_workers_add(q, t_count);
        while (!(volatile uintptr_t)i_count)
          FIO_THREAD_RESCHEDULE();
        fio_queue_workers_join(q);
      } else {
        union {
          void *(*t)(void *);
          void (*act)(fio_queue_s *);
        } thread_tasks;
        thread_tasks.act = fio_queue_perform_all;
        fio_thread_t *threads = (fio_thread_t *)
            FIO_MEM_REALLOC(NULL, 0, sizeof(*threads) * t_count, 0);
        for (size_t j = 0; j < t_count; ++j) {
          if (fio_thread_create(threads + j, thread_tasks.t, q)) {
            abort();
          }
        }
        for (size_t j = 0; j < t_count; ++j) {
          fio_thread_join(threads + j);
        }
        FIO_MEM_FREE(threads, sizeof(*threads) * t_count);
      }
    }

    end = fio_time_milli();
    if (FIO___QUEUE_TEST_PRINT) {
      fprintf(
          stderr,
          "\t- queue performed using %zu threads, %zu scheduling tasks (%zu "
          "each):\n\t"
          "    %lu ms with i_count = %lu\n",
          ((i % max_threads) + 1),
          tasks,
          info.count,
          (unsigned long)(end - start),
          (unsigned long)i_count);
    } else {
      fprintf(stderr, ".");
    }
    FIO_ASSERT(i_count == i_count_should_be, "ERROR: queue count invalid\n");
  }
  if (!(FIO___QUEUE_TEST_PRINT))
    fprintf(stderr, "\n");
  FIO_ASSERT(q->w == &q->mem,
             "queue library didn't release dynamic queue (should be static)");
  fio_queue_free(q);
  {
    fio_queue_init(&q2);
    for (size_t i = 0; i < (FIO_QUEUE_TASKS_PER_ALLOC * 3); ++i) {
      FIO_ASSERT(!fio_queue_push_urgent(&q2,
                                        .fn = (void (*)(void *, void *))(i + 1),
                                        .udata1 = (void *)(i + 1)),
                 "fio_queue_push_urgent failed");
    }
    FIO_ASSERT(q2.r->next && q2.r->next->next && !q2.r->next->next->next,
               "should have filled only three task blocks");
    for (size_t i = 0; i < (FIO_QUEUE_TASKS_PER_ALLOC * 3); ++i) {
      fio_queue_task_s t = fio_queue_pop(&q2);
      FIO_ASSERT(
          t.fn && (size_t)t.udata1 == (FIO_QUEUE_TASKS_PER_ALLOC * 3) - i,
          "fio_queue_push_urgent pop ordering error [%zu] %zu != %zu (%p)",
          i,
          (size_t)t.udata1,
          (FIO_QUEUE_TASKS_PER_ALLOC * 3) - i,
          (void *)(uintptr_t)t.fn);
    }
    FIO_ASSERT(fio_queue_pop(&q2).fn == NULL,
               "pop overflow after urgent tasks");
    fio_queue_destroy(&q2);
  }
  /* ************** testing timers ************** */
  {
    fio_queue_init(&q2);
    volatile uintptr_t tester = 0;
    fio_timer_queue_s tq = FIO_TIMER_QUEUE_INIT;

    /* test failuers */
    fio_timer_schedule(&tq,
                       .udata1 = (void *)&tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 100,
                       .repetitions = -1);
    FIO_ASSERT(tester == 1,
               "fio_timer_schedule should have called `on_finish`");
    tester = 0;
    fio_timer_schedule(NULL,
                       .fn = fio___queue_test_timer_task,
                       .udata1 = (void *)&tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 100,
                       .repetitions = -1);
    FIO_ASSERT(tester == 1,
               "fio_timer_schedule should have called `on_finish`");
    tester = 0;
    fio_timer_schedule(&tq,
                       .fn = fio___queue_test_timer_task,
                       .udata1 = (void *)&tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 0,
                       .repetitions = -1);
    FIO_ASSERT(tester == 1,
               "fio_timer_schedule should have called `on_finish`");
    /* test endless task */
    tester = 0;
    fio_timer_schedule(&tq,
                       .fn = fio___queue_test_timer_task,
                       .udata1 = (void *)&tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 1,
                       .repetitions = -1,
                       .start_at = fio_time_milli() - 10);
    FIO_ASSERT(tester == 0,
               "fio_timer_schedule should have scheduled the task.");
    for (size_t i = 0; i < 10; ++i) {
      uint64_t now = fio_time_milli();
      fio_timer_push2queue(&q2, &tq, now);
      fio_timer_push2queue(&q2, &tq, now);
      FIO_ASSERT(fio_queue_count(&q2), "task should have been scheduled");
      FIO_ASSERT(fio_queue_count(&q2) == 1,
                 "task should have been scheduled only once");
      fio_queue_perform(&q2);
      FIO_ASSERT(!fio_queue_count(&q2), "queue should be empty");
      FIO_ASSERT(tester == i + 1,
                 "task should have been performed (%zu).",
                 (size_t)tester);
    }

    tester = 0;
    fio_timer_destroy(&tq);
    FIO_ASSERT(tester == 1, "fio_timer_destroy should have called `on_finish`");

    /* test single-use task */
    tester = 0;
    int64_t milli_now = fio_time_milli();
    fio_timer_schedule(&tq,
                       .fn = fio___queue_test_timer_task,
                       .udata1 = (void *)&tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 100,
                       .repetitions = 1,
                       .start_at = milli_now - 10);
    FIO_ASSERT(tester == 0,
               "fio_timer_schedule should have scheduled the task.");
    fio_timer_schedule(&tq,
                       .fn = fio___queue_test_timer_task,
                       .udata1 = (void *)&tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 1,
                       // .repetitions = 1, // auto-value is 1
                       .start_at = milli_now - 10);
    FIO_ASSERT(tester == 0,
               "fio_timer_schedule should have scheduled the task.");
    FIO_ASSERT(fio_timer_next_at(&tq) == milli_now - 9,
               "fio_timer_next_at value error.");
    fio_timer_push2queue(&q2, &tq, milli_now);
    FIO_ASSERT(fio_queue_count(&q2) == 1,
               "task should have been scheduled (2)");
    FIO_ASSERT(fio_timer_next_at(&tq) == milli_now + 90,
               "fio_timer_next_at value error for unscheduled task.");
    fio_queue_perform(&q2);
    FIO_ASSERT(!fio_queue_count(&q2), "queue should be empty");
    FIO_ASSERT(tester == 2,
               "task should have been performed and on_finish called (%zu).",
               (size_t)tester);
    fio_timer_destroy(&tq);
    FIO_ASSERT(
        tester == 3,
        "fio_timer_destroy should have called on_finish of future task (%zu).",
        (size_t)tester);
    FIO_ASSERT(!tq.next, "timer queue should be empty.");
    fio_queue_destroy(&q2);
  }
}
