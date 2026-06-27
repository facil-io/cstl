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

Thread handle. On Windows this is the numeric thread ID; the handle is opened transiently only when needed.

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

Behaves like POSIX `fork`. On Windows logs an error and returns `-1`.

#### `fio_thread_getpid`

```c
FIO_IFUNC fio_thread_pid_t fio_thread_getpid(void);
```

Behaves like POSIX `getpid`.

#### `fio_thread_kill`

```c
FIO_IFUNC int fio_thread_kill(fio_thread_pid_t pid, int sig);
```

Behaves like POSIX `kill`.

#### `fio_thread_waitpid`

```c
FIO_IFUNC int fio_thread_waitpid(fio_thread_pid_t pid,
                                 int *stat_loc,
                                 int options);
```

Behaves like POSIX `waitpid`.

### Thread Functions

#### `fio_thread_create`

```c
FIO_IFUNC int fio_thread_create(fio_thread_t *t,
                                void *(*fn)(void *),
                                void *arg);
```

Starts a new thread running `fn(arg)`. Returns `0` on success, `-1` on failure.

#### `fio_thread_join`

```c
FIO_IFUNC int fio_thread_join(fio_thread_t *t);
```

Waits for the thread to finish.

#### `fio_thread_detach`

```c
FIO_IFUNC int fio_thread_detach(fio_thread_t *t);
```

Detaches the thread so resources are reclaimed automatically.

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

Sets the current thread's priority. Returns `0` on success, `-1` on error.

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

Locks `m`, blocking if necessary.

#### `fio_thread_mutex_trylock`

```c
FIO_IFUNC int fio_thread_mutex_trylock(fio_thread_mutex_t *m);
```

Tries to lock `m` without blocking. Returns `0` on success.

#### `fio_thread_mutex_unlock`

```c
FIO_IFUNC int fio_thread_mutex_unlock(fio_thread_mutex_t *m);
```

Unlocks `m`.

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

Waits up to `milliseconds`. Returns `0` if signaled, non-zero on timeout/error.

#### `fio_thread_cond_signal`

```c
FIO_IFUNC int fio_thread_cond_signal(fio_thread_cond_t *c);
```

Wakes one waiter.

#### `fio_thread_cond_destroy`

```c
FIO_IFUNC void fio_thread_cond_destroy(fio_thread_cond_t *c);
```

Destroys `c`.

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
  fio_thread_create(&t1, worker, NULL);
  fio_thread_create(&t2, worker, NULL);
  fio_thread_join(&t1);
  fio_thread_join(&t2);
  printf("counter: %d\n", counter);
  fio_thread_mutex_destroy(&lock);
  return 0;
}
```

------------------------------------------------------------
