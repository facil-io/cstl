# Portable Threads

```c
#define FIO_THREADS
#include "fio-stl.h"
```

A thin, portable wrapper over POSIX pthreads and Windows threads. Provides thread creation, mutexes, condition variables, priorities, and a few process helpers. The wrappers intentionally return no thread exit value.

### Types

#### `fio_thread_t`

```c
/* POSIX */ typedef pthread_t fio_thread_t;
/* Windows */ typedef uintptr_t fio_thread_t;
```

Thread identity. On Windows this remains a numeric thread ID; the implementation retains the actual OS handle for every created, joinable thread until `fio_thread_join` or `fio_thread_detach` succeeds. Join/detach each successfully created thread exactly once. Do not join and detach the same thread concurrently. `fio_thread_current` does not allocate or acquire ownership.

#### `fio_thread_pid_t`

```c
/* POSIX */ typedef pid_t fio_thread_pid_t;
/* Windows */ typedef DWORD fio_thread_pid_t;
```

Process ID type.

#### `fio_thread_mutex_t`

```c
/* POSIX */ typedef pthread_mutex_t fio_thread_mutex_t;
/* Windows */ typedef SRWLOCK fio_thread_mutex_t;
```

Mutex type.

#### `fio_thread_cond_t`

```c
/* POSIX */ typedef pthread_cond_t fio_thread_cond_t;
/* Windows */ typedef CONDITION_VARIABLE fio_thread_cond_t;
```

Condition-variable type.

#### `fio_thread_priority_e`

```c
typedef enum {
  FIO_THREAD_PRIORITY_ERROR = -1,
  FIO_THREAD_PRIORITY_LOWEST = 0,
  FIO_THREAD_PRIORITY_LOW,
  FIO_THREAD_PRIORITY_NORMAL,
  FIO_THREAD_PRIORITY_HIGH,
  FIO_THREAD_PRIORITY_HIGHEST,
} fio_thread_priority_e;
```

Thread priority levels.

### Configuration Macros

#### `FIO_THREADS_BYO`

If defined, the thread creation/join/detach/current/yield functions are declared but not implemented — bring your own.

#### `FIO_THREADS_FORK_BYO`

If defined, the process helpers (`fio_thread_fork`, `getpid`, `kill`, `waitpid`) are declared but not implemented.

#### `FIO_THREADS_MUTEX_BYO`

If defined, the mutex functions are declared but not implemented.

#### `FIO_THREADS_COND_BYO`

If defined, the condition-variable functions are declared but not implemented.

#### `FIO_THREAD_MUTEX_INIT`

```c
/* POSIX */ #define FIO_THREAD_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
/* Windows */ #define FIO_THREAD_MUTEX_INIT { 0 }
```

Static initializer for a mutex.

### Process Functions

#### `fio_thread_fork`

```c
FIO_IFUNC fio_thread_pid_t fio_thread_fork(void);
```

Behaves like POSIX `fork`. On native Windows logs an error, sets `errno = ENOSYS`, and returns `(fio_thread_pid_t)-1`. Because the Windows PID type is unsigned, compare against the cast sentinel, not `< 0`.

After a POSIX fork in a multithreaded process, only async-signal-safe operations are safe in the child until `exec`; inherited locks can remain owned by vanished threads.

#### `fio_thread_getpid`

```c
FIO_IFUNC fio_thread_pid_t fio_thread_getpid(void);
```

Behaves like POSIX `getpid`.

#### `fio_thread_kill`

```c
FIO_IFUNC int fio_thread_kill(fio_thread_pid_t pid, int sig);
```

Behaves like POSIX `kill` on POSIX. Native Windows supports positive PIDs with signal `0` (liveness check) or `SIGTERM`/`SIGINT` (termination), plus `SIGKILL` if defined. Unsupported signals return `-1` with `ENOSYS`; invalid signal numbers use `EINVAL`. Windows group/broadcast PID forms `0` and `(fio_thread_pid_t)-1` return `ENOSYS` rather than targeting the calling process.

#### `fio_thread_waitpid`

```c
FIO_IFUNC int fio_thread_waitpid(fio_thread_pid_t pid,
                                 int *stat_loc,
                                 int options);
```

Behaves like POSIX `waitpid` on POSIX. Native Windows supports a positive child PID or `(fio_thread_pid_t)-1` and options `0` or `WNOHANG`. Returns the selected child's PID on success, `0` for a live-child poll, or `-1` on error. A `0` or error result leaves `stat_loc` unchanged. Unknown/unsupported option bits set `EINVAL`; PID `0` sets `ENOSYS`; PIDs above `INT_MAX` cannot fit the return type and set `EOVERFLOW`.

**Note**: Windows discovers children through a process snapshot; it does not implement POSIX zombie retention or reaping. Already-exited children can disappear before discovery, and snapshots do not guarantee identity across PID reuse. Blocking any-child waits select a snapshot child, not necessarily the first child to exit. Retain the `CreateProcess` handle and use native waits when durable process identity or exit status is required.

### Thread Functions

#### `fio_thread_create`

```c
FIO_IFUNC int fio_thread_create(fio_thread_t *t,
                                void *(*fn)(void *),
                                void *arg);
```

Starts a new thread running `fn(arg)`. Returns `0` on success, `-1` on failure with `errno` identifying the error. `t` must point to writable storage and `fn` must be non-NULL. Failure transfers no ownership; do not use the output as a thread identity. Success creates one join/detach obligation. Keep `arg` and any data it references alive until the callback stops using them. Callback return values are discarded. Windows uses a CRT-compatible startup trampoline, including on 32-bit builds.

#### `fio_thread_join`

```c
FIO_IFUNC int fio_thread_join(fio_thread_t *t);
```

Waits for a successfully created, joinable thread to finish. Returns `0` on completion and nonzero on error. An error is **not** evidence that the thread stopped: retain its argument/buffer lifetimes. Windows retains ownership after a wait error so the operation can be retried; it releases the OS handle and zeroes `*t` only on success. POSIX returns the native `pthread_join` error number. Joining oneself, joining a detached thread, and concurrent joins/detaches of the same thread are unsupported.

#### `fio_thread_detach`

```c
FIO_IFUNC int fio_thread_detach(fio_thread_t *t);
```

Detaches a successfully created, joinable thread so its resources can be reclaimed without joining. Returns `0` on success, nonzero on error. On Windows this closes the retained joinable handle and zeroes `*t`; on POSIX it returns the native `pthread_detach` result. Detach does not wait for callback completion: keep callback data alive independently.

#### `fio_thread_exit`

```c
FIO_IFUNC void fio_thread_exit(void);
```

Exits the current thread.

#### `fio_thread_equal`

```c
FIO_IFUNC int fio_thread_equal(fio_thread_t *a, fio_thread_t *b);
```

Returns non-zero if `a` and `b` refer to the same thread.

#### `fio_thread_current`

```c
FIO_IFUNC fio_thread_t fio_thread_current(void);
```

Returns a handle for the current thread.

#### `fio_thread_nid`

```c
FIO_IFUNC uintptr_t fio_thread_nid(void);
```

Returns a process-local numeral ID for the current thread. The value is stable
for the thread's lifetime and unique among live threads, but may be reused after
the thread exits. It is not guaranteed to be an OS-visible kernel thread ID.

#### `fio_thread_yield`

```c
FIO_IFUNC void fio_thread_yield(void);
```

Yields the CPU.

#### `fio_thread_priority`

```c
FIO_SFUNC fio_thread_priority_e fio_thread_priority(void);
```

Returns the current thread's priority, or `FIO_THREAD_PRIORITY_ERROR` on failure.

#### `fio_thread_priority_set`

```c
FIO_SFUNC int fio_thread_priority_set(fio_thread_priority_e pr);
```

Sets the current thread's priority. Returns `0` on success, `-1` on error. Values outside `FIO_THREAD_PRIORITY_LOWEST` through `FIO_THREAD_PRIORITY_HIGHEST`, including `FIO_THREAD_PRIORITY_ERROR`, fail with `errno = EINVAL`. OS policy/permissions can reject valid priorities; POSIX preserves the native error in `errno`.

### Mutex Functions

#### `fio_thread_mutex_init`

```c
FIO_IFUNC int fio_thread_mutex_init(fio_thread_mutex_t *m);
```

Initializes a mutex. You can also use `FIO_THREAD_MUTEX_INIT` for static initialization.

#### `fio_thread_mutex_lock`

```c
FIO_IFUNC int fio_thread_mutex_lock(fio_thread_mutex_t *m);
```

Locks `m`, blocking if necessary. Returns `0` on success, `-1` on error; POSIX preserves the native error in `errno`.

#### `fio_thread_mutex_trylock`

```c
FIO_IFUNC int fio_thread_mutex_trylock(fio_thread_mutex_t *m);
```

Tries to lock `m` without blocking. Returns `0` on success.

#### `fio_thread_mutex_unlock`

```c
FIO_IFUNC int fio_thread_mutex_unlock(fio_thread_mutex_t *m);
```

Unlocks `m`. Returns `0` on success, `-1` on error; POSIX preserves the native error in `errno`.

#### `fio_thread_mutex_destroy`

```c
FIO_IFUNC void fio_thread_mutex_destroy(fio_thread_mutex_t *m);
```

Destroys `m`.

### Condition Variable Functions

#### `fio_thread_cond_init`

```c
FIO_IFUNC int fio_thread_cond_init(fio_thread_cond_t *c);
```

Initializes a condition variable.

#### `fio_thread_cond_wait`

```c
FIO_IFUNC int fio_thread_cond_wait(fio_thread_cond_t *c,
                                   fio_thread_mutex_t *m);
```

Waits on `c`. `m` must be locked; it is released while waiting and re-acquired on return.

#### `fio_thread_cond_timedwait`

```c
FIO_IFUNC int fio_thread_cond_timedwait(fio_thread_cond_t *c,
                                        fio_thread_mutex_t *m,
                                        size_t milliseconds);
```

Waits up to `milliseconds`, with the same mutex and predicate-loop requirements as `fio_thread_cond_wait`. Returns `0` on wake (which can be spurious), nonzero on timeout/error. POSIX computes an absolute `CLOCK_REALTIME` deadline, preserving fractional precision; clock failure returns its error code without waiting, and an unrepresentable deadline returns `EOVERFLOW`. Realtime clock adjustments can alter the elapsed wait. Windows clamps durations at `INFINITE - 1` milliseconds, the largest finite OS wait, so a finite duration never wraps or becomes `INFINITE`. The mutex remains held on pre-wait validation/clock failures.

#### `fio_thread_cond_signal`

```c
FIO_IFUNC int fio_thread_cond_signal(fio_thread_cond_t *c);
```

Wakes one waiter.

#### `fio_thread_cond_broadcast`

```c
FIO_IFUNC int fio_thread_cond_broadcast(fio_thread_cond_t *c);
```

Wakes all waiters.

#### `fio_thread_cond_destroy`

```c
FIO_IFUNC void fio_thread_cond_destroy(fio_thread_cond_t *c);
```

Destroys `c`.

### Multi-threaded Copy

#### `fio_thread_memcpy`

```c
FIO_SFUNC size_t fio_thread_memcpy(void *restrict dest,
                                   const void *restrict src,
                                   size_t bytes);
```

Copies `bytes` between valid, non-overlapping buffers, returning the number of participating threads (including the caller). Both buffers must remain valid for the call. Small copies run in the caller; large copies use up to `FIO_MEMCPY_THREADS` threads (default `8`). Creation failures fall back to copying the remaining bytes in the caller. A worker join failure is fatal: returning while a worker might still access stack state or buffers would be unsafe. This is not a general-purpose performance replacement for a tuned single-threaded copy; benchmark your workload.

### Example

```c
#define FIO_THREADS
#include "fio-stl.h"

static fio_thread_mutex_t lock = FIO_THREAD_MUTEX_INIT;
static int counter = 0;

static void *worker(void *arg) {
  (void)arg;
  for (int i = 0; i < 100000; ++i) {
    fio_thread_mutex_lock(&lock);
    ++counter;
    fio_thread_mutex_unlock(&lock);
  }
  return NULL;
}

int main(void) {
  fio_thread_t t1, t2;
  if (fio_thread_create(&t1, worker, NULL))
    return 1;
  if (fio_thread_create(&t2, worker, NULL)) {
    if (fio_thread_join(&t1))
      abort(); /* do not continue without establishing worker completion */
    fio_thread_mutex_destroy(&lock);
    return 1;
  }
  if (fio_thread_join(&t1) || fio_thread_join(&t2))
    abort();
  printf("counter: %d\n", counter);
  fio_thread_mutex_destroy(&lock);
  return 0;
}
```

------------------------------------------------------------
