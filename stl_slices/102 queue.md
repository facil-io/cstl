## Task Queue

The Simple Template Library includes a simple, thread-safe, task queue based on a linked list of ring buffers.

Since delayed processing is a common task, this queue is provides an easy way to schedule and perform delayed tasks.

In addition, a Timer type allows timed events to be scheduled and moved (according to their "due date") to an existing Task Queue.

By `FIO_QUEUE`, the following task and timer related helpers are defined:

### Queue Related Types

#### `fio_queue_task_s`

```c
/** Task information */
typedef struct {
  /** The function to call */
  void (*fn)(void *, void *);
  /** User opaque data */
  void *udata1;
  /** User opaque data */
  void *udata2;
} fio_queue_task_s;
```

The `fio_queue_task_s` type contains information about a delayed task. The information is important for the `fio_queue_push` MACRO, where it is used as named arguments for the task information.

#### `fio_queue_s`

```c
/** The queue object - should be considered opaque (or, at least, read only). */
typedef struct {
  fio___task_ring_s *r;
  fio___task_ring_s *w;
  /** the number of tasks waiting to be performed (read-only). */
  size_t count;
  fio_lock_i lock;
  fio___task_ring_s mem;
} fio_queue_s;
```

The `fio_queue_s` object is the queue object.

This object could be placed on the stack or allocated on the heap (using [`fio_queue_new`](#fio_queue_new)).

Once the object is no longer in use call [`fio_queue_destroy`](#fio_queue_destroy) (if placed on the stack) of [`fio_queue_free`](#fio_queue_free) (if allocated using [`fio_queue_new`](#fio_queue_new)).

### Queue API

#### `FIO_QUEUE_INIT(queue)`

```c
/** Used to initialize a fio_queue_s object. */
#define FIO_QUEUE_INIT(name)                                                   \
  { .r = &(name).mem, .w = &(name).mem, .lock = FIO_LOCK_INIT }
```

#### `fio_queue_destroy`

```c
void fio_queue_destroy(fio_queue_s *q);
```

Destroys a queue and reinitializes it, after freeing any used resources.

#### `fio_queue_new`

```c
fio_queue_s *fio_queue_new(void);
```

Creates a new queue object (allocated on the heap).

#### `fio_queue_free`

```c
void fio_queue_free(fio_queue_s *q);
```

Frees a queue object after calling fio_queue_destroy.

#### `fio_queue_push`

```c
int fio_queue_push(fio_queue_s *q, fio_queue_task_s task);
#define fio_queue_push(q, ...)                                                 \
  fio_queue_push((q), (fio_queue_task_s){__VA_ARGS__})

```

Pushes a **valid** (non-NULL) task to the queue.

This function is shadowed by the `fio_queue_push` MACRO, allowing named arguments to be used.

For example:

```c
void tsk(void *, void *);
fio_queue_s q = FIO_QUEUE_INIT(q);
fio_queue_push(q, .fn = tsk);
// ...
fio_queue_destroy(q);
```

Returns 0 if `task.fn == NULL` or if the task was successfully added to the queue.

Returns -1 on error (no memory).


#### `fio_queue_push_urgent`

```c
int fio_queue_push_urgent(fio_queue_s *q, fio_queue_task_s task);
#define fio_queue_push_urgent(q, ...)                                          \
  fio_queue_push_urgent((q), (fio_queue_task_s){__VA_ARGS__})
```

Pushes a task to the head of the queue (LIFO).

Returns -1 on error (no memory).

See [`fio_queue_push`](#fio_queue_push) for details.

#### `fio_queue_pop`

```c
fio_queue_task_s fio_queue_pop(fio_queue_s *q);
```

Pops a task from the queue (FIFO).

Returns a NULL task on error (`task.fn == NULL`).

**Note**: The task isn't performed automatically, it's just returned. This is useful for queues that don't necessarily contain callable functions.

#### `fio_queue_perform`

```c
int fio_queue_perform(fio_queue_s *q);
```

Pops and performs a task from the queue (FIFO).

Returns -1 on error (queue empty).

#### `fio_queue_perform_all`

```c
void fio_queue_perform_all(fio_queue_s *q);
```

Performs all tasks in the queue.

#### `fio_queue_count`

```c
size_t fio_queue_count(fio_queue_s *q);
```

Returns the number of tasks in the queue.

### Timer Related Types

#### `fio_timer_queue_s`

```c
typedef struct {
  fio___timer_event_s *next;
  fio_lock_i lock;
} fio_timer_queue_s;
```

The `fio_timer_queue_s` struct should be considered an opaque data type and accessed only using the functions or the initialization MACRO.

To create a `fio_timer_queue_s` on the stack (or statically):

```c
fio_timer_queue_s foo_timer = FIO_TIMER_QUEUE_INIT;
```

A timer could be allocated dynamically:

```c
fio_timer_queue_s *foo_timer = malloc(sizeof(*foo_timer));
FIO_ASSERT_ALLOC(foo_timer);
*foo_timer = (fio_timer_queue_s)FIO_TIMER_QUEUE_INIT;
```

#### `FIO_TIMER_QUEUE_INIT`

This is a MACRO used to initialize a `fio_timer_queue_s` object.

### Timer API

#### `fio_timer_schedule`

```c
void fio_timer_schedule(fio_timer_queue_s *timer_queue,
                        fio_timer_schedule_args_s args);
```

Adds a time-bound event to the timer queue.

Accepts named arguments using the following argument type and MACRO:

```c
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

#define fio_timer_schedule(timer_queue, ...)                                   \
  fio_timer_schedule((timer_queue), (fio_timer_schedule_args_s){__VA_ARGS__})
```

Note, the event will repeat every `every` milliseconds (or the same unites as `start_at` and `now`).

It the scheduler is busy or the event is otherwise delayed, its next scheduling may compensate for the delay by being scheduled sooner.

#### `fio_timer_push2queue` 

```c
/**  */
size_t fio_timer_push2queue(fio_queue_s *queue,
                            fio_timer_queue_s *timer_queue,
                            uint64_t now); // now is in milliseconds
```

Pushes due events from the timer queue to an event queue.

If `now` is `0`, than `fio_time_milli` will be called to supply `now`'s value.

**Note**: all the `start_at` values for all the events in the timer queue will be treated as if they use the same units as (and are relative to) `now`. By default, this unit should be milliseconds, to allow `now` to be zero.

Returns the number of tasks pushed to the queue. A value of `0` indicates no new tasks were scheduled.

#### `fio_timer_next_at`

```c
uint64_t fio_timer_next_at(fio_timer_queue_s *timer_queue);
```

Returns the millisecond at which the next event should occur.

If no timer is due (list is empty), returns `(uint64_t)-1`.

**Note**: Unless manually specified, millisecond timers are relative to  `fio_time_milli()`.


#### `fio_timer_clear`

```c
void fio_timer_clear(fio_timer_queue_s *timer_queue);
```

Clears any waiting timer bound tasks.

**Note**:

The timer queue must NEVER be freed when there's a chance that timer tasks are waiting to be performed in a `fio_queue_s`.

This is due to the fact that the tasks may try to reschedule themselves (if they repeat).

-------------------------------------------------------------------------------