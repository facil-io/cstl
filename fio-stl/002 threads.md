## Threads (portable)

```c
#define FIO_THREADS
#include "fio-stl.h"
```

The facil.io `FIO_THREADS` module provides a simple API for threading that is OS portable between POSIX systems and Windows OS.

The POSIX systems implementation uses `pthreads` under the hood.

Please note that due to thread return value and methodology differences, `FIO_THREADS` do not return any value.

The following methods are provided when the `FIO_THREADS` macro is defined before including the `fio-stl.h` header.

### Process functions

#### `FIO_THREADS_FORK_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these processes forking functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `fio_thread_fork`

```c
fio_thread_pid_t fio_thread_fork(void);
```

Behaves (or should behave) the same as the POSIX system call `fork`.

#### `fio_thread_getpid`

```c
fio_thread_pid_t fio_thread_getpid(void);
```

Behaves (or should behave) the same as the POSIX system call `getpid`.

#### `fio_thread_kill`

```c
int fio_thread_kill(fio_thread_pid_t pid, int sig);
```

Behaves (or should behave) the same as the POSIX system call `kill`.

#### `fio_thread_waitpid`

```c
int fio_thread_waitpid(fio_thread_pid_t pid, int *stat_loc, int options);
```

Behaves (or should behave) the same as the POSIX system call `waitpid`.

### Thread functions

#### `FIO_THREADS_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these thread functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `fio_thread_create`
```c
int fio_thread_create(fio_thread_t *thread, void *(*start_function)(void *), void *arg);
```

Starts a new thread, returns 0 on success and -1 on failure.

#### `fio_thread_join`
```c
int fio_thread_join(fio_thread_t *thread);
```

Waits for the thread to finish.

#### `fio_thread_detach`
```c
int fio_thread_detach(fio_thread_t *thread);
```

Detaches the thread, so thread resources are freed automatically.

#### `fio_thread_exit`
```c
void fio_thread_exit(void);
```

Ends the current running thread.

#### `fio_thread_equal`
```c
int fio_thread_equal(fio_thread_t *a, fio_thread_t *b);
```

Returns non-zero if both threads refer to the same thread.

#### `fio_thread_current`
```c
fio_thread_t fio_thread_current(void);
```

Returns the current thread.

#### `fio_thread_yield`
```c
void fio_thread_yield(void);
```

Yields thread execution.

#### `fio_thread_priority`

```c
fio_thread_priority_e fio_thread_priority(void);
```

Returns the current thread's priority level as a `fio_thread_priority_e` enum.

```c
/** Possible thread priority values. */
typedef enum {
  FIO_THREAD_PRIORITY_ERROR = -1,
  FIO_THREAD_PRIORITY_LOWEST = 0,
  FIO_THREAD_PRIORITY_LOW,
  FIO_THREAD_PRIORITY_NORMAL,
  FIO_THREAD_PRIORITY_HIGH,
  FIO_THREAD_PRIORITY_HIGHEST,
} fio_thread_priority_e;
```

#### `fio_thread_priority_set`

```c
int fio_thread_priority_set(fio_thread_priority_e pr);
```

Sets the current thread's priority level as a `fio_thread_priority_e` enum (see [`fio_thread_priority`](#fio_thread_priority)).

### Mutex functions

#### `FIO_THREADS_MUTEX_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these mutex functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `FIO_THREAD_MUTEX_INIT`

Statically initializes a Mutex.

#### `fio_thread_mutex_init`
```c
int fio_thread_mutex_init(fio_thread_mutex_t *m);
```

Initializes a simple Mutex.

Or use the static initialization value: `FIO_THREAD_MUTEX_INIT`

#### `fio_thread_mutex_lock`
```c
int fio_thread_mutex_lock(fio_thread_mutex_t *m);
```

Locks a simple Mutex, returning -1 on error.

#### `fio_thread_mutex_trylock`
```c
int fio_thread_mutex_trylock(fio_thread_mutex_t *m);
```

Attempts to lock a simple Mutex, returning zero on success.

#### `fio_thread_mutex_unlock`
```c
int fio_thread_mutex_unlock(fio_thread_mutex_t *m);
```

Unlocks a simple Mutex, returning zero on success or -1 on error.

#### `fio_thread_mutex_destroy`
```c
void fio_thread_mutex_destroy(fio_thread_mutex_t *m);
```

Destroys the simple Mutex (cleanup).


### Conditional Variable Functions

#### `FIO_THREADS_COND_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these conditional variable functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `fio_thread_cond_init`

```c
int fio_thread_cond_init(fio_thread_cond_t *c);
```

Initializes a simple conditional variable.

#### `fio_thread_cond_wait`

```c
int fio_thread_cond_wait(fio_thread_cond_t *c, fio_thread_mutex_t *m);
```

Waits on a conditional variable (MUST be previously locked).

#### `fio_thread_cond_signal`

```c
int fio_thread_cond_signal(fio_thread_cond_t *c);
```

Signals a simple conditional variable.

#### `fio_thread_cond_destroy`

```c
void fio_thread_cond_destroy(fio_thread_cond_t *c);
```

Destroys a simple conditional variable.


-------------------------------------------------------------------------------
