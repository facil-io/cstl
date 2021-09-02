/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_QUEUE                   /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#include "101 time.h"               /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                                Task / Timer Queues
                                (Event Loop Engine)




***************************************************************************** */
#if defined(FIO_QUEUE) && !defined(H___FIO_QUEUE___H)
#define H___FIO_QUEUE___H

/* *****************************************************************************
Queue Type(s)
***************************************************************************** */

/* Note: FIO_QUEUE_TASKS_PER_ALLOC can't be more than 65535 */
#ifndef FIO_QUEUE_TASKS_PER_ALLOC
#if UINTPTR_MAX <= 0xFFFFFFFF
/* fits fio_queue_s in one page on most 32 bit machines */
#define FIO_QUEUE_TASKS_PER_ALLOC 338
#else
/* fits fio_queue_s in one page on most 64 bit machines */
#define FIO_QUEUE_TASKS_PER_ALLOC 168
#endif
#endif

/** Task information */
typedef struct {
  /** The function to call */
  void (*fn)(void *, void *);
  /** User opaque data */
  void *udata1;
  /** User opaque data */
  void *udata2;
} fio_queue_task_s;

/* internal use */
typedef struct fio___task_ring_s {
  uint16_t r;   /* reader position */
  uint16_t w;   /* writer position */
  uint16_t dir; /* direction */
  struct fio___task_ring_s *next;
  fio_queue_task_s buf[FIO_QUEUE_TASKS_PER_ALLOC];
} fio___task_ring_s;

/** The queue object - should be considered opaque (or, at least, read only). */
typedef struct {
  fio___task_ring_s *r;
  fio___task_ring_s *w;
  /** the number of tasks waiting to be performed. */
  size_t count;
  FIO___LOCK_TYPE lock;
  fio___task_ring_s mem;
} fio_queue_s;

/* *****************************************************************************
Queue API
***************************************************************************** */

#if FIO_USE_THREAD_MUTEX_TMP
/** May be used to initialize global, static memory, queues. */
#define FIO_QUEUE_STATIC_INIT(queue)                                           \
  {                                                                            \
    .r = &(queue).mem, .w = &(queue).mem,                                      \
    .lock = (fio_thread_mutex_t)FIO_THREAD_MUTEX_INIT                          \
  }
#else
/** May be used to initialize global, static memory, queues. */
#define FIO_QUEUE_STATIC_INIT(queue)                                           \
  { .r = &(queue).mem, .w = &(queue).mem, .lock = FIO_LOCK_INIT }
#endif

/** Initializes a fio_queue_s object. */
FIO_IFUNC void fio_queue_init(fio_queue_s *q);

/** Destroys a queue and re-initializes it, after freeing any used resources. */
SFUNC void fio_queue_destroy(fio_queue_s *q);

/** Creates a new queue object (allocated on the heap). */
FIO_IFUNC fio_queue_s *fio_queue_new(void);

/** Frees a queue object after calling fio_queue_destroy. */
SFUNC void fio_queue_free(fio_queue_s *q);

/** Pushes a task to the queue. Returns -1 on error. */
SFUNC int fio_queue_push(fio_queue_s *q, fio_queue_task_s task);

/**
 * Pushes a task to the queue, offering named arguments for the task.
 * Returns -1 on error.
 */
#define fio_queue_push(q, ...)                                                 \
  fio_queue_push((q), (fio_queue_task_s){__VA_ARGS__})

/** Pushes a task to the head of the queue. Returns -1 on error (no memory). */
SFUNC int fio_queue_push_urgent(fio_queue_s *q, fio_queue_task_s task);

/**
 * Pushes a task to the queue, offering named arguments for the task.
 * Returns -1 on error.
 */
#define fio_queue_push_urgent(q, ...)                                          \
  fio_queue_push_urgent((q), (fio_queue_task_s){__VA_ARGS__})

/** Pops a task from the queue (FIFO). Returns a NULL task on error. */
SFUNC fio_queue_task_s fio_queue_pop(fio_queue_s *q);

/** Performs a task from the queue. Returns -1 on error (queue empty). */
SFUNC int fio_queue_perform(fio_queue_s *q);

/** Performs all tasks in the queue. */
SFUNC void fio_queue_perform_all(fio_queue_s *q);

/** returns the number of tasks in the queue. */
FIO_IFUNC size_t fio_queue_count(fio_queue_s *q);

/* *****************************************************************************
Timer Queue Types and API
***************************************************************************** */

typedef struct fio___timer_event_s fio___timer_event_s;

typedef struct {
  fio___timer_event_s *next;
  FIO___LOCK_TYPE lock;
} fio_timer_queue_s;

#if FIO_USE_THREAD_MUTEX_TMP
#define FIO_TIMER_QUEUE_INIT                                                   \
  { .lock = ((fio_thread_mutex_t)FIO_THREAD_MUTEX_INIT) }
#else
#define FIO_TIMER_QUEUE_INIT                                                   \
  { .lock = FIO_LOCK_INIT }
#endif

typedef struct {
  /** The timer function. If it returns a non-zero value, the timer stops. */
  int (*fn)(void *, void *);
  /** Opaque user data. */
  void *udata1;
  /** Opaque user data. */
  void *udata2;
  /** Called when the timer is done (finished). */
  void (*on_finish)(void *, void *);
  /** Timer interval, in milliseconds. */
  uint32_t every;
  /** The number of times the timer should be performed. -1 == infinity. */
  int32_t repetitions;
  /** Millisecond at which to start. If missing, filled automatically. */
  int64_t start_at;
} fio_timer_schedule_args_s;

/** Adds a time-bound event to the timer queue. */
SFUNC void fio_timer_schedule(fio_timer_queue_s *timer_queue,
                              fio_timer_schedule_args_s args);

/** A MACRO allowing named arguments to be used. See fio_timer_schedule_args_s.
 */
#define fio_timer_schedule(timer_queue, ...)                                   \
  fio_timer_schedule((timer_queue), (fio_timer_schedule_args_s){__VA_ARGS__})

/** Pushes due events from the timer queue to an event queue. */
SFUNC size_t fio_timer_push2queue(fio_queue_s *queue,
                                  fio_timer_queue_s *timer_queue,
                                  int64_t now_in_milliseconds);

/*
 * Returns the millisecond at which the next event should occur.
 *
 * If no timer is due (list is empty), returns `(uint64_t)-1`.
 *
 * NOTE: unless manually specified, millisecond timers are relative to
 * `fio_time_milli()`.
 */
FIO_IFUNC int64_t fio_timer_next_at(fio_timer_queue_s *timer_queue);

/**
 * Clears any waiting timer bound tasks.
 *
 * NOTE:
 *
 * The timer queue must NEVER be freed when there's a chance that timer tasks
 * are waiting to be performed in a `fio_queue_s`.
 *
 * This is due to the fact that the tasks may try to reschedule themselves (if
 * they repeat).
 */
SFUNC void fio_timer_destroy(fio_timer_queue_s *timer_queue);

/* *****************************************************************************
Queue Inline Helpers
***************************************************************************** */

/** Creates a new queue object (allocated on the heap). */
FIO_IFUNC fio_queue_s *fio_queue_new(void) {
  fio_queue_s *q = (fio_queue_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*q), 0);
  if (!q)
    return NULL;
  fio_queue_init(q);
  return q;
}

/** returns the number of tasks in the queue. */
FIO_IFUNC size_t fio_queue_count(fio_queue_s *q) { return q->count; }

/** Initializes a fio_queue_s object. */
FIO_IFUNC void fio_queue_init(fio_queue_s *q) {
  /* do this manually, we don't want to reset a whole page */
  q->r = &q->mem;
  q->w = &q->mem;
  q->count = 0;
  q->lock = FIO___LOCK_INIT;
  q->mem.next = NULL;
  q->mem.r = q->mem.w = q->mem.dir = 0;
}

/* *****************************************************************************
Timer Queue Inline Helpers
***************************************************************************** */

struct fio___timer_event_s {
  int (*fn)(void *, void *);
  void *udata1;
  void *udata2;
  void (*on_finish)(void *udata1, void *udata2);
  int64_t due;
  uint32_t every;
  int32_t repetitions;
  struct fio___timer_event_s *next;
};

/*
 * Returns the millisecond at which the next event should occur.
 *
 * If no timer is due (list is empty), returns `-1`.
 *
 * NOTE: unless manually specified, millisecond timers are relative to
 * `fio_time_milli()`.
 */
FIO_IFUNC int64_t fio_timer_next_at(fio_timer_queue_s *tq) {
  int64_t v = -1;
  if (!tq)
    goto missing_tq;
  if (!tq || !tq->next)
    return v;
  FIO___LOCK_LOCK(tq->lock);
  if (tq->next)
    v = tq->next->due;
  FIO___LOCK_UNLOCK(tq->lock);
  return v;

missing_tq:
  FIO_LOG_ERROR("`fio_timer_next_at` called with a NULL timer queue!");
  return v;
}

/* *****************************************************************************
Queue Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE)

/** Destroys a queue and re-initializes it, after freeing any used resources. */
SFUNC void fio_queue_destroy(fio_queue_s *q) {
  FIO___LOCK_LOCK(q->lock);
  while (q->r) {
    fio___task_ring_s *tmp = q->r;
    q->r = q->r->next;
    if (tmp != &q->mem)
      FIO_MEM_FREE_(tmp, sizeof(*tmp));
  }
  FIO___LOCK_UNLOCK(q->lock);
  FIO___LOCK_DESTROY(q->lock);
  fio_queue_init(q);
}

/** Frees a queue object after calling fio_queue_destroy. */
SFUNC void fio_queue_free(fio_queue_s *q) {
  fio_queue_destroy(q);
  FIO_MEM_FREE_(q, sizeof(*q));
}

FIO_IFUNC int fio___task_ring_push(fio___task_ring_s *r,
                                   fio_queue_task_s task) {
  if (r->dir && r->r == r->w)
    return -1;
  r->buf[r->w] = task;
  ++(r->w);
  if (r->w == FIO_QUEUE_TASKS_PER_ALLOC) {
    r->w = 0;
    r->dir = ~r->dir;
  }
  return 0;
}

FIO_IFUNC int fio___task_ring_unpop(fio___task_ring_s *r,
                                    fio_queue_task_s task) {
  if (r->dir && r->r == r->w)
    return -1;
  if (!r->r) {
    r->r = FIO_QUEUE_TASKS_PER_ALLOC;
    r->dir = ~r->dir;
  }
  --r->r;
  r->buf[r->r] = task;
  return 0;
}

FIO_IFUNC fio_queue_task_s fio___task_ring_pop(fio___task_ring_s *r) {
  fio_queue_task_s t = {.fn = NULL};
  if (!r->dir && r->r == r->w) {
    return t;
  }
  t = r->buf[r->r];
  ++r->r;
  if (r->r == FIO_QUEUE_TASKS_PER_ALLOC) {
    r->r = 0;
    r->dir = ~r->dir;
  }
  return t;
}

int fio_queue_push___(void); /* sublime text marker */
/** Pushes a task to the queue. Returns -1 on error. */
SFUNC int fio_queue_push FIO_NOOP(fio_queue_s *q, fio_queue_task_s task) {
  if (!task.fn)
    return 0;
  FIO___LOCK_LOCK(q->lock);
  if (fio___task_ring_push(q->w, task)) {
    if (q->w != &q->mem && q->mem.next == NULL) {
      q->w->next = &q->mem;
      q->mem.w = q->mem.r = q->mem.dir = 0;
    } else {
      void *tmp = (fio___task_ring_s *)
          FIO_MEM_REALLOC_(NULL, 0, sizeof(*q->w->next), 0);
      if (!tmp)
        goto no_mem;
      q->w->next = (fio___task_ring_s *)tmp;
      if (!FIO_MEM_REALLOC_IS_SAFE_) {
        q->w->next->r = q->w->next->w = q->w->next->dir = 0;

        q->w->next->next = NULL;
      }
    }
    q->w = q->w->next;
    fio___task_ring_push(q->w, task);
  }
  ++q->count;
  FIO___LOCK_UNLOCK(q->lock);
  return 0;
no_mem:
  FIO___LOCK_UNLOCK(q->lock);
  FIO_LOG_ERROR("No memory for Queue %p to increase task ring buffer.",
                (void *)q);
  return -1;
}

int fio_queue_push_urgent___(void); /* sublimetext marker */
/** Pushes a task to the head of the queue. Returns -1 on error (no memory). */
SFUNC int fio_queue_push_urgent FIO_NOOP(fio_queue_s *q,
                                         fio_queue_task_s task) {
  if (!task.fn)
    return 0;
  FIO___LOCK_LOCK(q->lock);
  if (fio___task_ring_unpop(q->r, task)) {
    /* such a shame... but we must allocate a while task block for one task */
    fio___task_ring_s *tmp =
        (fio___task_ring_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*q->w->next), 0);
    if (!tmp)
      goto no_mem;
    tmp->next = q->r;
    q->r = tmp;
    tmp->w = 1;
    tmp->dir = tmp->r = 0;
    tmp->buf[0] = task;
  }
  ++q->count;
  FIO___LOCK_UNLOCK(q->lock);
  return 0;
no_mem:
  FIO___LOCK_UNLOCK(q->lock);
  FIO_LOG_ERROR("No memory for Queue %p to increase task ring buffer.",
                (void *)q);
  return -1;
}

/** Pops a task from the queue (FIFO). Returns a NULL task on error. */
SFUNC fio_queue_task_s fio_queue_pop(fio_queue_s *q) {
  fio_queue_task_s t = {.fn = NULL};
  fio___task_ring_s *to_free = NULL;
  if (!q->count)
    return t;
  FIO___LOCK_LOCK(q->lock);
  if (!q->count)
    goto finish;
  if (!(t = fio___task_ring_pop(q->r)).fn) {
    to_free = q->r;
    q->r = to_free->next;
    to_free->next = NULL;
    t = fio___task_ring_pop(q->r);
  }
  if (t.fn && !(--q->count) && q->r != &q->mem) {
    if (to_free && to_free != &q->mem) { // edge case
      FIO_MEM_FREE_(to_free, sizeof(*to_free));
    }
    to_free = q->r;
    q->r = q->w = &q->mem;
    q->mem.w = q->mem.r = q->mem.dir = 0;
  }
finish:
  FIO___LOCK_UNLOCK(q->lock);
  if (to_free && to_free != &q->mem) {
    FIO_MEM_FREE_(to_free, sizeof(*to_free));
  }
  return t;
}

/** Performs a task from the queue. Returns -1 on error (queue empty). */
SFUNC int fio_queue_perform(fio_queue_s *q) {
  fio_queue_task_s t = fio_queue_pop(q);
  if (t.fn) {
    t.fn(t.udata1, t.udata2);
    return 0;
  }
  return -1;
}

/** Performs all tasks in the queue. */
SFUNC void fio_queue_perform_all(fio_queue_s *q) {
  fio_queue_task_s t;
  while ((t = fio_queue_pop(q)).fn)
    t.fn(t.udata1, t.udata2);
}

/* *****************************************************************************
Timer Queue Implementation
***************************************************************************** */

FIO_IFUNC void fio___timer_insert(fio___timer_event_s **pos,
                                  fio___timer_event_s *e) {
  while (*pos && e->due >= (*pos)->due)
    pos = &((*pos)->next);
  e->next = *pos;
  *pos = e;
}

FIO_IFUNC fio___timer_event_s *fio___timer_pop(fio___timer_event_s **pos,
                                               int64_t due) {
  if (!*pos || (*pos)->due > due)
    return NULL;
  fio___timer_event_s *t = *pos;
  *pos = t->next;
  return t;
}

FIO_IFUNC fio___timer_event_s *fio___timer_event_new(
    fio_timer_schedule_args_s args) {
  fio___timer_event_s *t = NULL;
  t = (fio___timer_event_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*t), 0);
  if (!t)
    goto init_error;
  if (!args.repetitions)
    args.repetitions = 1;
  *t = (fio___timer_event_s){
      .fn = args.fn,
      .udata1 = args.udata1,
      .udata2 = args.udata2,
      .on_finish = args.on_finish,
      .due = args.start_at + args.every,
      .every = args.every,
      .repetitions = args.repetitions,
  };
  return t;
init_error:
  if (args.on_finish)
    args.on_finish(args.udata1, args.udata2);
  return NULL;
}

FIO_IFUNC void fio___timer_event_free(fio_timer_queue_s *tq,
                                      fio___timer_event_s *t) {
  if (tq && (t->repetitions < 0 || fio_atomic_sub_fetch(&t->repetitions, 1))) {
    FIO___LOCK_LOCK(tq->lock);
    fio___timer_insert(&tq->next, t);
    FIO___LOCK_UNLOCK(tq->lock);
    return;
  }
  if (t->on_finish)
    t->on_finish(t->udata1, t->udata2);
  FIO_MEM_FREE_(t, sizeof(*t));
}

SFUNC void fio___timer_perform(void *timer_, void *t_) {
  fio_timer_queue_s *tq = (fio_timer_queue_s *)timer_;
  fio___timer_event_s *t = (fio___timer_event_s *)t_;
  if (t->fn(t->udata1, t->udata2))
    tq = NULL;
  t->due += t->every;
  fio___timer_event_free(tq, t);
}

/** Pushes due events from the timer queue to an event queue. */
SFUNC size_t fio_timer_push2queue(fio_queue_s *queue,
                                  fio_timer_queue_s *timer,
                                  int64_t start_at) {
  size_t r = 0;
  if (!start_at)
    start_at = fio_time_milli();
  if (FIO___LOCK_TRYLOCK(timer->lock))
    return 0;
  fio___timer_event_s *t;
  while ((t = fio___timer_pop(&timer->next, start_at))) {
    fio_queue_push(queue,
                   .fn = fio___timer_perform,
                   .udata1 = timer,
                   .udata2 = t);
    ++r;
  }
  FIO___LOCK_UNLOCK(timer->lock);
  return r;
}

void fio_timer_schedule___(void); /* sublimetext marker */
/** Adds a time-bound event to the timer queue. */
SFUNC void fio_timer_schedule FIO_NOOP(fio_timer_queue_s *timer,
                                       fio_timer_schedule_args_s args) {
  fio___timer_event_s *t = NULL;
  if (!timer || !args.fn || !args.every)
    goto no_timer_queue;
  if (!args.start_at)
    args.start_at = fio_time_milli();
  t = fio___timer_event_new(args);
  if (!t)
    return;
  FIO___LOCK_LOCK(timer->lock);
  fio___timer_insert(&timer->next, t);
  FIO___LOCK_UNLOCK(timer->lock);
  return;
no_timer_queue:
  if (args.on_finish)
    args.on_finish(args.udata1, args.udata2);
  FIO_LOG_ERROR("fio_timer_schedule called with illegal arguments.");
}

/**
 * Clears any waiting timer bound tasks.
 *
 * NOTE:
 *
 * The timer queue must NEVER be freed when there's a chance that timer tasks
 * are waiting to be performed in a `fio_queue_s`.
 *
 * This is due to the fact that the tasks may try to reschedule themselves (if
 * they repeat).
 */
SFUNC void fio_timer_destroy(fio_timer_queue_s *tq) {
  fio___timer_event_s *next;
  FIO___LOCK_LOCK(tq->lock);
  next = tq->next;
  tq->next = NULL;
  FIO___LOCK_UNLOCK(tq->lock);
  FIO___LOCK_DESTROY(tq->lock);
  while (next) {
    fio___timer_event_s *tmp = next;

    next = next->next;
    fio___timer_event_free(NULL, tmp);
  }
}

/* *****************************************************************************
Queue - test
***************************************************************************** */
#ifdef FIO_TEST_CSTL

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

FIO_SFUNC void FIO_NAME_TEST(stl, queue)(void) {
  fprintf(stderr, "* Testing facil.io task scheduling (fio_queue)\n");
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
  for (intptr_t i = 0; i < (FIO_QUEUE_TASKS_PER_ALLOC << 2); ++i) {
    fio_queue_push(q,
                   .fn = fio___queue_test_counter_task,
                   .udata1 = (void *)(i + 1),
                   .udata2 = (void *)(i + 2));
  }
  fio_queue_perform_all(q);
  fio_queue_perform_all(q);
  for (intptr_t i = (FIO_QUEUE_TASKS_PER_ALLOC << 2);
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
  clock_t start, end;
  i_count = 0;
  start = clock();
  for (size_t i = 0; i < FIO___QUEUE_TOTAL_COUNT; i++) {
    fio___queue_test_sample_task(&i_count, NULL);
  }
  end = clock();
  if (FIO___QUEUE_TEST_PRINT) {
    fprintf(
        stderr,
        "\t- Queueless (direct call) counter: %lu cycles with i_count = %lu\n",
        (unsigned long)(end - start),
        (unsigned long)i_count);
  }
  size_t i_count_should_be = i_count;
  i_count = 0;
  start = clock();
  for (size_t i = 0; i < FIO___QUEUE_TOTAL_COUNT; i++) {
    fio_queue_push(q,
                   .fn = fio___queue_test_sample_task,
                   .udata1 = (void *)&i_count);
  }
  fio_queue_perform_all(q);
  fio_queue_perform_all(q);
  fio_queue_perform_all(q);
  end = clock();
  if (FIO___QUEUE_TEST_PRINT) {
    fprintf(stderr,
            "\t- single task counter: %lu cycles with i_count = %lu\n",
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
    start = clock();
    for (size_t j = 0; j < tasks; ++j) {
      fio_queue_push(q,
                     fio___queue_test_sched_sample_task,
                     (void *)&info,
                     (void *)info.count);
    }
    FIO_ASSERT(fio_queue_count(q), "tasks not counted?!");
    {
      const size_t t_count = (i % max_threads) + 1;
      union {
        void *(*t)(void *);
        void (*act)(fio_queue_s *);
      } thread_tasks;
      thread_tasks.act = fio_queue_perform_all;
      fio_thread_t *threads = (fio_thread_t *)
          FIO_MEM_REALLOC_(NULL, 0, sizeof(*threads) * t_count, 0);
      for (size_t j = 0; j < t_count; ++j) {
        if (fio_thread_create(threads + j, thread_tasks.t, q)) {
          abort();
        }
      }
      for (size_t j = 0; j < t_count; ++j) {
        fio_thread_join(threads[j]);
      }
      FIO_MEM_FREE(threads, sizeof(*threads) * t_count);
    }

    end = clock();
    if (FIO___QUEUE_TEST_PRINT) {
      fprintf(stderr,
              "- queue performed using %zu threads, %zu scheduling tasks (%zu "
              "each):\n"
              "    %lu cycles with i_count = %lu\n",
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
    fprintf(stderr, "* testing urgent insertion\n");
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
    fprintf(stderr,
            "* Testing facil.io timer scheduling (fio_timer_queue_s)\n");
    fprintf(stderr, "  Note: Errors SHOULD print out to the log.\n");
    fio_queue_init(&q2);
    uintptr_t tester = 0;
    fio_timer_queue_s tq = FIO_TIMER_QUEUE_INIT;

    /* test failuers */
    fio_timer_schedule(&tq,
                       .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 100,
                       .repetitions = -1);
    FIO_ASSERT(tester == 1,
               "fio_timer_schedule should have called `on_finish`");
    tester = 0;
    fio_timer_schedule(NULL,
                       .fn = fio___queue_test_timer_task,
                       .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 100,
                       .repetitions = -1);
    FIO_ASSERT(tester == 1,
               "fio_timer_schedule should have called `on_finish`");
    tester = 0;
    fio_timer_schedule(&tq,
                       .fn = fio___queue_test_timer_task,
                       .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 0,
                       .repetitions = -1);
    FIO_ASSERT(tester == 1,
               "fio_timer_schedule should have called `on_finish`");
    fprintf(stderr, "  Note: no more errors should pront for this test.\n");

    /* test endless task */
    tester = 0;
    fio_timer_schedule(&tq,
                       .fn = fio___queue_test_timer_task,
                       .udata1 = &tester,
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
                       .udata1 = &tester,
                       .on_finish = fio___queue_test_sample_task,
                       .every = 100,
                       .repetitions = 1,
                       .start_at = milli_now - 10);
    FIO_ASSERT(tester == 0,
               "fio_timer_schedule should have scheduled the task.");
    fio_timer_schedule(&tq,
                       .fn = fio___queue_test_timer_task,
                       .udata1 = &tester,
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
  fprintf(stderr, "* passed.\n");
}
#endif /* FIO_TEST_CSTL */

/* *****************************************************************************
Queue/Timer Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_QUEUE
#endif /* FIO_QUEUE */
