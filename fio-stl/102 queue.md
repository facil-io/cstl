# Task and Timer Queues

```c
#define FIO_QUEUE
#include "fio-stl.h"
```

A thread-safe task queue built from linked ring buffers, plus a timer queue that moves due events into a task queue. Worker threads can be attached to drain the queue in the background.

---

## Configuration

#### `FIO_QUEUE_TASKS_PER_ALLOC`

```c
#define FIO_QUEUE_TASKS_PER_ALLOC 168 /* 338 on 32-bit */
```

Tasks per ring-buffer chunk. Default is chosen so `fio_queue_s` fits in one page. Must not exceed 65535.

---

## Task Queue Types

#### `fio_queue_task_s`

```c
typedef struct {
  void (*fn)(void *, void *);
  void *udata1;
  void *udata2;
} fio_queue_task_s;
```

A single task. `fn` receives both `udata` pointers. Tasks with `fn == NULL` are ignored.

#### `fio_queue_s`

```c
typedef struct {
  fio___task_ring_s *r;
  fio___task_ring_s *w;
  uint32_t count;
  FIO___LOCK_TYPE lock;
  FIO_LIST_NODE consumers;
  fio___task_ring_s mem;
} fio_queue_s;
```

Queue object. Treat as opaque. Allocate on the stack or with `fio_queue_new`.

---

## Task Queue API

#### `FIO_QUEUE_STATIC_INIT`

```c
#define FIO_QUEUE_STATIC_INIT(queue) /* ... */
```

Static initializer for a global queue. Prefer `fio_queue_init` at runtime when possible — it initializes only the bytes that matter.

#### `fio_queue_init`

```c
FIO_IFUNC void fio_queue_init(fio_queue_s *q);
```

Initializes `q` in place.

#### `fio_queue_destroy`

```c
SFUNC void fio_queue_destroy(fio_queue_s *q);
```

Frees ring buffers, stops and joins workers, and re-initializes `q`. After destruction the queue may be reused, but on some platforms the lock may need explicit re-initialization.

#### `fio_queue_new`

```c
SFUNC fio_queue_s *fio_queue_new(void);
```

Allocates and initializes a queue on the heap.

#### `fio_queue_free`

```c
SFUNC void fio_queue_free(fio_queue_s *q);
```

Destroys `q` and frees the queue object itself.

#### `fio_queue_push`

```c
SFUNC int fio_queue_push(fio_queue_s *q, fio_queue_task_s task);
#define fio_queue_push(q, ...) \
  fio_queue_push((q), (fio_queue_task_s){__VA_ARGS__})
```

Pushes a task to the tail of the queue. The macro accepts named arguments.

**Returns:** `0` on success, `-1` on memory error.

```c
fio_queue_push(&q, .fn = my_task, .udata1 = arg);
```

#### `fio_queue_push_urgent`

```c
SFUNC int fio_queue_push_urgent(fio_queue_s *q, fio_queue_task_s task);
#define fio_queue_push_urgent(q, ...) \
  fio_queue_push_urgent((q), (fio_queue_task_s){__VA_ARGS__})
```

Pushes a task to the head of the queue (LIFO).

**Returns:** `0` on success, `-1` on memory error.

#### `fio_queue_pop`

```c
SFUNC fio_queue_task_s fio_queue_pop(fio_queue_s *q);
```

Removes and returns the next task (FIFO). The returned task has `fn == NULL` if the queue was empty.

#### `fio_queue_perform`

```c
SFUNC int fio_queue_perform(fio_queue_s *q);
```

Pops and performs one task. Returns `-1` if the queue was empty.

#### `fio_queue_perform_all`

```c
SFUNC void fio_queue_perform_all(fio_queue_s *q);
```

Performs every task currently in the queue.

#### `fio_queue_count`

```c
FIO_IFUNC uint32_t fio_queue_count(fio_queue_s *q);
```

Returns the number of pending tasks.

---

## Worker Threads

#### `fio_queue_workers_add`

```c
SFUNC int fio_queue_workers_add(fio_queue_s *q, size_t count);
```

Spawns `count` consumer threads that automatically perform tasks as they arrive. Threads sleep on a condition variable when idle.

**Returns:** `0` on success, `-1` on thread creation failure.

#### `fio_queue_workers_stop`

```c
SFUNC void fio_queue_workers_stop(fio_queue_s *q);
```

Signals all workers to stop. Returns immediately without waiting.

#### `fio_queue_workers_join`

```c
SFUNC void fio_queue_workers_join(fio_queue_s *q);
```

Signals workers to stop and blocks until they terminate.

#### `fio_queue_workers_wake`

```c
SFUNC void fio_queue_workers_wake(fio_queue_s *q);
```

Wakes all workers to check for new tasks. Called automatically on push.

---

## Timer Queue Types

#### `fio_timer_queue_s`

```c
typedef struct {
  fio___timer_event_s *next;
  FIO___LOCK_TYPE lock;
} fio_timer_queue_s;
```

Opaque timer queue.

#### `FIO_TIMER_QUEUE_INIT`

```c
#define FIO_TIMER_QUEUE_INIT /* ... */
```

Static initializer for a timer queue.

#### `fio_timer_schedule_args_s`

```c
typedef struct {
  int (*fn)(void *, void *);
  void *udata1;
  void *udata2;
  void (*on_finish)(void *, void *);
  uint32_t every;
  int32_t repetitions;
  int64_t start_at;
} fio_timer_schedule_args_s;
```

Timer schedule arguments.

**Members:**
- `fn` — callback. Return non-zero to stop the timer.
- `udata1`, `udata2` — opaque data passed to `fn` and `on_finish`.
- `on_finish` — called when the timer stops.
- `every` — interval in milliseconds.
- `repetitions` — repeat count; `-1` means forever.
- `start_at` — base time in milliseconds; `0` uses `fio_time_milli()`.

---

## Timer Queue API

#### `fio_timer_schedule`

```c
SFUNC void fio_timer_schedule(fio_timer_queue_s *timer_queue,
                              fio_timer_schedule_args_s args);
#define fio_timer_schedule(timer_queue, ...) \
  fio_timer_schedule((timer_queue), (fio_timer_schedule_args_s){__VA_ARGS__})
```

Adds a timed event to the timer queue. The macro accepts named arguments.

```c
fio_timer_schedule(&timers,
                   .fn = tick,
                   .udata1 = ctx,
                   .every = 1000,
                   .repetitions = -1);
```

#### `fio_timer_push2queue`

```c
SFUNC size_t fio_timer_push2queue(fio_queue_s *queue,
                                  fio_timer_queue_s *timer_queue,
                                  int64_t now_in_milliseconds);
```

Moves all due timer events into `queue`. Pass `0` for `now_in_milliseconds` to use `fio_time_milli()`.

**Returns:** number of timers pushed.

#### `fio_timer_next_at`

```c
FIO_IFUNC int64_t fio_timer_next_at(fio_timer_queue_s *timer_queue);
```

Returns the due time of the next event, or `INT64_MAX` if the queue is empty.

#### `fio_timer_destroy`

```c
SFUNC void fio_timer_destroy(fio_timer_queue_s *timer_queue);
```

Cancels all pending timers. Do not free the timer queue while timer tasks may still be queued in a `fio_queue_s`, because repeating timers reschedule themselves.

---

## Example

```c
#define FIO_QUEUE
#include "fio-stl.h"

void work(void *a, void *b) {
  (void)a; (void)b;
  fprintf(stderr, "working\n");
}

int tick(void *a, void *b) {
  (void)a; (void)b;
  fprintf(stderr, "tick\n");
  return 0;
}

int main(void) {
  fio_queue_s q = {0};
  fio_queue_init(&q);

  fio_queue_push(&q, .fn = work);
  fio_queue_perform(&q);

  fio_timer_queue_s t = FIO_TIMER_QUEUE_INIT;
  fio_timer_schedule(&t, .fn = tick, .every = 100, .repetitions = 3);
  fio_timer_push2queue(&q, &t, 0);
  fio_queue_perform_all(&q);

  fio_timer_destroy(&t);
  fio_queue_destroy(&q);
  return 0;
}
```

------------------------------------------------------------
