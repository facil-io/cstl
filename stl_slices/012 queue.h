/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "011 time.h"               /* Development inclusion - ignore line */
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
  fio_lock_i lock;
  fio___task_ring_s mem;
} fio_queue_s;

/* *****************************************************************************
Queue API
***************************************************************************** */

/** Used to initialize a fio_queue_s object. */
#define FIO_QUEUE_INIT(name)                                                   \
  { .r = &(name).mem, .w = &(name).mem, .lock = FIO_LOCK_INIT }

/** Destroys a queue and reinitializes it, after freeing any used resources. */
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
  fio_lock_i lock;
} fio_timer_queue_s;

#define FIO_TIMER_QUEUE_INIT                                                   \
  { .lock = FIO_LOCK_INIT }

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
  uint64_t start_at;
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
                                  uint64_t now_in_milliseconds);

/*
 * Returns the millisecond at which the next event should occur.
 *
 * If no timer is due (list is empty), returns `(uint64_t)-1`.
 *
 * NOTE: unless manually specified, millisecond timers are relative to
 * `fio_time_milli()`.
 */
FIO_IFUNC uint64_t fio_timer_next_at(fio_timer_queue_s *timer_queue);

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
SFUNC void fio_timer_clear(fio_timer_queue_s *timer_queue);

/* *****************************************************************************
Queue Inline Helpers
***************************************************************************** */

/** Creates a new queue object (allocated on the heap). */
FIO_IFUNC fio_queue_s *fio_queue_new(void) {
  fio_queue_s *q = (fio_queue_s *)FIO_MEM_CALLOC_(sizeof(*q), 1);
  if (!q)
    return NULL;
  *q = (fio_queue_s)FIO_QUEUE_INIT(*q);
  return q;
}

/** returns the number of tasks in the queue. */
FIO_IFUNC size_t fio_queue_count(fio_queue_s *q) { return q->count; }

/* *****************************************************************************
Timer Queue Inline Helpers
***************************************************************************** */

struct fio___timer_event_s {
  int (*fn)(void *, void *);
  void *udata1;
  void *udata2;
  void (*on_finish)(void *udata1, void *udata2);
  uint64_t due;
  uint32_t every;
  int32_t repetitions;
  struct fio___timer_event_s *next;
};

/*
 * Returns the millisecond at which the next event should occur.
 *
 * If no timer is due (list is empty), returns `(uint64_t)-1`.
 *
 * NOTE: unless manually specified, millisecond timers are relative to
 * `fio_time_milli()`.
 */
FIO_IFUNC uint64_t fio_timer_next_at(fio_timer_queue_s *tq) {
  uint64_t v = (uint64_t)-1;
  if (!tq)
    goto missing_tq;
  if (!tq || !tq->next)
    return v;
  fio_lock(&tq->lock);
  if (tq->next)
    v = tq->next->due;
  fio_unlock(&tq->lock);
  return v;

missing_tq:
  FIO_LOG_ERROR("`fio_timer_next_at` called with a NULL timer queue!");
  return v;
}

/* *****************************************************************************
Queue Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE)

/** Destroys a queue and reinitializes it, after freeing any used resources. */
SFUNC void fio_queue_destroy(fio_queue_s *q) {
  fio_lock(&q->lock);
  while (q->r) {
    fio___task_ring_s *tmp = q->r;
    q->r = q->r->next;
    if (tmp != &q->mem)
      FIO_MEM_FREE_(tmp, sizeof(*tmp));
  }
  *q = (fio_queue_s)FIO_QUEUE_INIT(*q);
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
  ++r->w;
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

int fio_queue_push___(void); /* sublimetext marker */
/** Pushes a task to the queue. Returns -1 on error. */
SFUNC int fio_queue_push FIO_NOOP(fio_queue_s *q, fio_queue_task_s task) {
  if (!task.fn)
    return 0;
  fio_lock(&q->lock);
  if (fio___task_ring_push(q->w, task)) {
    if (q->w != &q->mem && q->mem.next == NULL) {
      q->w->next = &q->mem;
      q->mem.w = q->mem.r = q->mem.dir = 0;
    } else {
      q->w->next = (fio___task_ring_s *)FIO_MEM_CALLOC_(sizeof(*q->w->next), 1);
      if (!q->w->next)
        goto no_mem;
    }
    q->w = q->w->next;
    fio___task_ring_push(q->w, task);
  }
  ++q->count;
  fio_unlock(&q->lock);
  return 0;
no_mem:
  fio_unlock(&q->lock);
  return -1;
}

int fio_queue_push_urgent___(void); /* sublimetext marker */
/** Pushes a task to the head of the queue. Returns -1 on error (no memory). */
SFUNC int fio_queue_push_urgent FIO_NOOP(fio_queue_s *q,
                                         fio_queue_task_s task) {
  if (!task.fn)
    return 0;
  fio_lock(&q->lock);
  if (fio___task_ring_unpop(q->r, task)) {
    /* such a shame... but we must allocate a while task block for one task */
    fio___task_ring_s *tmp =
        (fio___task_ring_s *)FIO_MEM_CALLOC_(sizeof(*q->w->next), 1);
    if (!tmp)
      goto no_mem;
    tmp->next = q->r;
    q->r = tmp;
    tmp->w = 1;
    tmp->dir = tmp->r = 0;
    tmp->buf[0] = task;
  }
  ++q->count;
  fio_unlock(&q->lock);
  return 0;
no_mem:
  fio_unlock(&q->lock);
  return -1;
}

/** Pops a task from the queue (FIFO). Returns a NULL task on error. */
SFUNC fio_queue_task_s fio_queue_pop(fio_queue_s *q) {
  fio_queue_task_s t = {.fn = NULL};
  fio___task_ring_s *to_free = NULL;
  if (!q->count)
    return t;
  fio_lock(&q->lock);
  if (!q->count)
    goto finish;
  if (!(t = fio___task_ring_pop(q->r)).fn) {
    to_free = q->r;
    q->r = to_free->next;
    to_free->next = NULL;
    t = fio___task_ring_pop(q->r);
  }
  if (t.fn && !(--q->count) && q->r != &q->mem) {
    if (to_free && to_free != &q->mem) { // edge case? never happens?
      FIO_MEM_FREE_(to_free, sizeof(*to_free));
    }
    to_free = q->r;
    q->r = q->w = &q->mem;
    q->mem.w = q->mem.r = q->mem.dir = 0;
  }
finish:
  fio_unlock(&q->lock);
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
                                               uint64_t due) {
  if (!*pos || (*pos)->due > due)
    return NULL;
  fio___timer_event_s *t = *pos;
  *pos = t->next;
  return t;
}

FIO_IFUNC fio___timer_event_s *
fio___timer_event_new(fio_timer_schedule_args_s args) {
  fio___timer_event_s *t = NULL;
  t = (fio___timer_event_s *)FIO_MEM_CALLOC_(sizeof(*t), 1);
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
    fio_lock(&tq->lock);
    fio___timer_insert(&tq->next, t);
    fio_unlock(&tq->lock);
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
  fio___timer_event_free(tq, t);
}

/** Pushes due events from the timer queue to an event queue. */
SFUNC size_t fio_timer_push2queue(fio_queue_s *queue, fio_timer_queue_s *timer,
                                  uint64_t start_at) {
  size_t r = 0;
  if (!start_at)
    start_at = fio_time_milli();
  if (fio_trylock(&timer->lock))
    return 0;
  fio___timer_event_s *t;
  while ((t = fio___timer_pop(&timer->next, start_at))) {
    fio_queue_push(queue, .fn = fio___timer_perform, .udata1 = timer,
                   .udata2 = t);
    ++r;
  }
  fio_unlock(&timer->lock);
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
  fio_lock(&timer->lock);
  fio___timer_insert(&timer->next, t);
  fio_unlock(&timer->lock);
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
SFUNC void fio_timer_clear(fio_timer_queue_s *tq) {
  fio___timer_event_s *next;
  fio_lock(&tq->lock);
  next = tq->next;
  tq->next = NULL;
  fio_unlock(&tq->lock);
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
