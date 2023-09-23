/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_QUEUE              /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                Task / Timer Queues
                                (Event Loop Engine)



Copyright and License: see header file (000 copyright.h) or top of file
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
  /** task read pointer. */
  fio___task_ring_s *r;
  /** task write pointer. */
  fio___task_ring_s *w;
  /** the number of tasks waiting to be performed. */
  uint32_t count;
  /** global queue lock. */
  FIO___LOCK_TYPE lock;
  /** linked lists of consumer threads. */
  FIO_LIST_NODE consumers;
  /** main ring buffer associated with the queue. */
  fio___task_ring_s mem;
} fio_queue_s;

typedef struct {
  FIO_LIST_NODE node;
  fio_queue_s *queue;
  fio_thread_t thread;
  fio_thread_mutex_t mutex;
  fio_thread_cond_t cond;
  size_t workers;
  volatile int stop;
} fio___thread_group_s;

/* *****************************************************************************
Queue API
***************************************************************************** */

#if FIO_USE_THREAD_MUTEX_TMP
/** May be used to initialize global, static memory, queues. */
#define FIO_QUEUE_STATIC_INIT(queue)                                           \
  {                                                                            \
    .r = &(queue).mem, .w = &(queue).mem,                                      \
    .consumers = FIO_LIST_INIT((queue).consumers),                             \
    .lock = (fio_thread_mutex_t)FIO_THREAD_MUTEX_INIT                          \
  }
#else
/** May be used to initialize global, static memory, queues. */
#define FIO_QUEUE_STATIC_INIT(queue)                                           \
  {                                                                            \
    .r = &(queue).mem, .w = &(queue).mem,                                      \
    .consumers = FIO_LIST_INIT((queue).consumers), .lock = FIO_LOCK_INIT       \
  }
#endif

/** Initializes a fio_queue_s object. */
FIO_IFUNC void fio_queue_init(fio_queue_s *q);

/** Destroys a queue and re-initializes it, after freeing any used resources. */
SFUNC void fio_queue_destroy(fio_queue_s *q);

/** Creates a new queue object (allocated on the heap). */
SFUNC fio_queue_s *fio_queue_new(void);

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
FIO_IFUNC uint32_t fio_queue_count(fio_queue_s *q);

/** Adds worker / consumer threads to perform the jobs in the queue. */
SFUNC int fio_queue_workers_add(fio_queue_s *q, size_t count);

/** Signals all worker threads to stop performing tasks and terminate. */
SFUNC void fio_queue_workers_stop(fio_queue_s *q);

/** Signals all worker threads to stop, waiting for them to complete. */
SFUNC void fio_queue_workers_join(fio_queue_s *q);

/** Signals all worker threads to go back to work (new tasks added). */
SFUNC void fio_queue_workers_wake(fio_queue_s *q);

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

/** returns the number of tasks in the queue. */
FIO_IFUNC uint32_t fio_queue_count(fio_queue_s *q) { return q->count; }

/** Initializes a fio_queue_s object. */
FIO_IFUNC void fio_queue_init(fio_queue_s *q) {
  /* do this manually, we don't want to reset a whole page */
  q->r = &q->mem;
  q->w = &q->mem;
  q->count = 0;
  q->consumers = FIO_LIST_INIT(q->consumers);
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
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* task queue leak detection */
FIO___LEAK_COUNTER_DEF(fio_queue)
FIO___LEAK_COUNTER_DEF(fio_queue_task_rings)
/** Destroys a queue and re-initializes it, after freeing any used resources. */
SFUNC void fio_queue_destroy(fio_queue_s *q) {
  for (;;) {
    FIO___LOCK_LOCK(q->lock);
    while (q->r) {
      fio___task_ring_s *tmp = q->r;
      q->r = q->r->next;
      if (tmp != &q->mem)
        FIO_MEM_FREE_(tmp, sizeof(*tmp));
    }
    if (FIO_LIST_IS_EMPTY(&q->consumers)) {
      FIO___LOCK_UNLOCK(q->lock);
      break;
    }
    FIO_LIST_EACH(fio___thread_group_s, node, &q->consumers, pos) {
      pos->stop = 1;
      for (size_t i = 0; i < pos->workers; ++i)
        fio_thread_cond_signal(&pos->cond);
    }
    FIO_LIST_EACH(fio___thread_group_s, node, &q->consumers, pos) {
      FIO___LOCK_UNLOCK(q->lock);
      fio_thread_join(&pos->thread);
      FIO___LOCK_LOCK(q->lock);
    }
    FIO___LOCK_UNLOCK(q->lock);
    if (FIO_LIST_IS_EMPTY(&q->consumers))
      break;
    FIO_THREAD_RESCHEDULE();
  }
  FIO___LOCK_DESTROY(q->lock);
  fio_queue_init(q);
}

/** Creates a new queue object (allocated on the heap). */
SFUNC fio_queue_s *fio_queue_new(void) {
  fio_queue_s *q = (fio_queue_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*q), 0);
  if (!q)
    return NULL;
  fio_queue_init(q);
  FIO___LEAK_COUNTER_ON_ALLOC(fio_queue);
  return q;
}

/** Frees a queue object after calling fio_queue_destroy. */
SFUNC void fio_queue_free(fio_queue_s *q) {
  fio_queue_destroy(q);
  if (q) {
    FIO___LEAK_COUNTER_ON_FREE(fio_queue);
    FIO_MEM_FREE_(q, sizeof(*q));
  }
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
  r->buf[r->r] = (fio_queue_task_s){.fn = NULL};
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
      FIO___LEAK_COUNTER_ON_ALLOC(fio_queue_task_rings);
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
  if (!FIO_LIST_IS_EMPTY(&q->consumers)) {
    FIO_LIST_EACH(fio___thread_group_s, node, &q->consumers, pos) {
      fio_thread_cond_signal(&pos->cond);
    }
  }
  FIO___LOCK_UNLOCK(q->lock);
  return 0;
no_mem:
  FIO___LOCK_UNLOCK(q->lock);
  FIO_LOG_ERROR("No memory for Queue %p to increase task ring buffer.",
                (void *)q);
  return -1;
}

int fio_queue_push_urgent___(void); /* IDE marker */
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
    FIO___LEAK_COUNTER_ON_ALLOC(fio_queue_task_rings);
    tmp->next = q->r;
    q->r = tmp;
    tmp->w = 1;
    tmp->dir = tmp->r = 0;
    tmp->buf[0] = task;
  }
  ++q->count;
  if (!FIO_LIST_IS_EMPTY(&q->consumers)) {
    FIO_LIST_EACH(fio___thread_group_s, node, &q->consumers, pos) {
      fio_thread_cond_signal(&pos->cond);
    }
  }
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
      FIO___LEAK_COUNTER_ON_FREE(fio_queue_task_rings);
      FIO_MEM_FREE_(to_free, sizeof(*to_free));
    }
    to_free = q->r;
    q->r = q->w = &q->mem;
    q->mem.w = q->mem.r = q->mem.dir = 0;
  }
finish:
  FIO___LOCK_UNLOCK(q->lock);
  if (to_free && to_free != &q->mem) {
    FIO___LEAK_COUNTER_ON_FREE(fio_queue_task_rings);
    FIO_MEM_FREE_(to_free, sizeof(*to_free));
  }
  return t;
}

/** Performs a task from the queue. Returns -1 on error (queue empty). */
SFUNC int fio_queue_perform(fio_queue_s *q) {
  fio_queue_task_s t = fio_queue_pop(q);
  if (!t.fn)
    return -1;
  t.fn(t.udata1, t.udata2);
  return 0;
}

/** Performs all tasks in the queue. */
SFUNC void fio_queue_perform_all(fio_queue_s *q) {
  fio_queue_task_s t;
  while ((t = fio_queue_pop(q)).fn)
    t.fn(t.udata1, t.udata2);
}

/* *****************************************************************************
Queue Consumer Threads
***************************************************************************** */

FIO_SFUNC void *fio___queue_worker_task(void *g_) {
  fio___thread_group_s *grp = (fio___thread_group_s *)g_;
  while (!grp->stop) {
    fio_queue_perform_all(grp->queue);
    fio_thread_mutex_lock(&grp->mutex);
    if (!grp->stop)
      fio_thread_cond_wait(&grp->cond, &grp->mutex);
    fio_thread_mutex_unlock(&grp->mutex);
    fio_queue_perform_all(grp->queue);
  }
  return NULL;
}
FIO_SFUNC void *fio___queue_worker_manager(void *g_) {
  fio_thread_t threads_buf[256];
  fio___thread_group_s grp = *(fio___thread_group_s *)g_;
  FIO_LIST_PUSH(&grp.queue->consumers, &grp.node);
  grp.stop = 0;
  fio_thread_t *threads =
      grp.workers > 256
          ? ((fio_thread_t *)
                 FIO_MEM_REALLOC_(NULL, 0, sizeof(*threads) * grp.workers, 0))
          : threads_buf;
  fio_thread_mutex_init(&grp.mutex);
  fio_thread_cond_init(&grp.cond);
  for (size_t i = 0; i < grp.workers; ++i) {
    fio_thread_create(threads + i, fio___queue_worker_task, (void *)&grp);
  }
  ((fio___thread_group_s *)g_)->stop = 0;
  /* from this point on, g_ is invalid! */
  for (size_t i = 0; i < grp.workers; ++i) {
    fio_thread_join(threads + i);
  }
  if (threads != threads_buf)
    FIO_MEM_FREE_(threads, sizeof(*threads) * grp.workers);
  FIO___LOCK_LOCK(grp.queue->lock);
  FIO_LIST_REMOVE(&grp.node);
  FIO___LOCK_UNLOCK(grp.queue->lock);
  fio_queue_perform_all(grp.queue);
  return NULL;
}

SFUNC int fio_queue_workers_add(fio_queue_s *q, size_t workers) {
  FIO___LOCK_LOCK(q->lock);
  if (!q->consumers.next || !q->consumers.prev) {
    q->consumers = FIO_LIST_INIT(q->consumers);
  }
  fio___thread_group_s grp = {.queue = q, .workers = workers, .stop = 1};
  if (fio_thread_create(&grp.thread, fio___queue_worker_manager, &grp))
    return -1;
  while (grp.stop)
    FIO_THREAD_RESCHEDULE();
  FIO___LOCK_UNLOCK(q->lock);
  return 0;
}

SFUNC void fio_queue_workers_stop(fio_queue_s *q) {
  if (FIO_LIST_IS_EMPTY(&q->consumers))
    return;
  FIO___LOCK_LOCK(q->lock);
  FIO_LIST_EACH(fio___thread_group_s, node, &q->consumers, pos) {
    pos->stop = 1;
    for (size_t i = 0; i < pos->workers * 2; ++i)
      fio_thread_cond_signal(&pos->cond);
  }
  FIO___LOCK_UNLOCK(q->lock);
}

/** Signals all worker threads to go back to work (new tasks were). */
SFUNC void fio_queue_workers_wake(fio_queue_s *q) {
  if (FIO_LIST_IS_EMPTY(&q->consumers))
    return;
  FIO___LOCK_LOCK(q->lock);
  FIO_LIST_EACH(fio___thread_group_s, node, &q->consumers, pos) {
    fio_thread_cond_signal(&pos->cond);
  }
  FIO___LOCK_UNLOCK(q->lock);
}

/** Signals all worker threads to stop, waiting for them to complete. */
SFUNC void fio_queue_workers_join(fio_queue_s *q) {
  fio_queue_workers_stop(q);
  FIO___LOCK_LOCK(q->lock);
  FIO_LIST_EACH(fio___thread_group_s, node, &q->consumers, pos) {
    FIO___LOCK_UNLOCK(q->lock);
    fio_thread_join(&pos->thread);
    FIO___LOCK_LOCK(q->lock);
  }
  FIO___LOCK_UNLOCK(q->lock);
}

/* *****************************************************************************
Timer Queue Implementation
***************************************************************************** */
FIO___LEAK_COUNTER_DEF(fio___timer_event_s)

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
  FIO___LEAK_COUNTER_ON_ALLOC(fio___timer_event_s);
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
  FIO___LEAK_COUNTER_ON_FREE(fio___timer_event_s);
  FIO_MEM_FREE_(t, sizeof(*t));
}

FIO_SFUNC void fio___timer_perform(void *timer_, void *t_) {
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

void fio_timer_schedule___(void); /* IDE marker */
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
Queue/Timer Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_QUEUE
#endif /* FIO_QUEUE */
