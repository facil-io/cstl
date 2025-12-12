## Threads (portable)

```c
#define FIO_THREADS
#include "fio-stl.h"
```

The facil.io `FIO_THREADS` module provides a simple API for threading that is OS portable between POSIX systems and Windows OS.

The POSIX systems implementation uses `pthreads` under the hood.

Please note that due to thread return value and methodology differences, `FIO_THREADS` do not return any value.

The following methods are provided when the `FIO_THREADS` macro is defined before including the `fio-stl.h` header.

### Types

#### `fio_thread_t`

```c
/* POSIX */
typedef pthread_t fio_thread_t;

/* Windows */
typedef HANDLE fio_thread_t;
```

The thread type, representing a thread handle. Platform-specific implementation.

#### `fio_thread_pid_t`

```c
/* POSIX */
typedef pid_t fio_thread_pid_t;

/* Windows */
typedef DWORD fio_thread_pid_t;
```

The process ID type. Platform-specific implementation.

#### `fio_thread_mutex_t`

```c
/* POSIX */
typedef pthread_mutex_t fio_thread_mutex_t;

/* Windows */
typedef CRITICAL_SECTION fio_thread_mutex_t;
```

The mutex type. Platform-specific implementation.

#### `fio_thread_cond_t`

```c
/* POSIX */
typedef pthread_cond_t fio_thread_cond_t;

/* Windows */
typedef CONDITION_VARIABLE fio_thread_cond_t;
```

The conditional variable type. Platform-specific implementation.

#### `fio_thread_priority_e`

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

An enumeration of possible thread priority levels.

### Process Functions

#### `FIO_THREADS_FORK_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these processes forking functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `fio_thread_fork`

```c
fio_thread_pid_t fio_thread_fork(void);
```

Behaves (or should behave) the same as the POSIX system call `fork`.

**Note**: on Windows, `fork` is not natively supported. The function will log an error and return `-1`.

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

**Parameters:**
- `pid` - the process ID to send the signal to
- `sig` - the signal to send

**Returns:** `0` on success, `-1` on error.

#### `fio_thread_waitpid`

```c
int fio_thread_waitpid(fio_thread_pid_t pid, int *stat_loc, int options);
```

Behaves (or should behave) the same as the POSIX system call `waitpid`.

**Parameters:**
- `pid` - the process ID to wait for (or `-1` for any child process)
- `stat_loc` - pointer to store the status information
- `options` - options flags (e.g., `WNOHANG`)

**Returns:** the process ID of the child that changed state, `0` if `WNOHANG` was specified and no child changed state, or `-1` on error.

### Thread Functions

#### `FIO_THREADS_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these thread functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `fio_thread_create`

```c
int fio_thread_create(fio_thread_t *t, void *(*fn)(void *), void *arg);
```

Starts a new thread, returns `0` on success and `-1` on failure.

**Parameters:**
- `t` - pointer to the thread handle to initialize
- `fn` - the function to execute in the new thread
- `arg` - argument to pass to the thread function

**Returns:** `0` on success, `-1` on failure.

#### `fio_thread_join`

```c
int fio_thread_join(fio_thread_t *t);
```

Waits for the thread to finish.

**Parameters:**
- `t` - pointer to the thread handle

**Returns:** `0` on success, `-1` on error.

#### `fio_thread_detach`

```c
int fio_thread_detach(fio_thread_t *t);
```

Detaches the thread, so thread resources are freed automatically.

**Parameters:**
- `t` - pointer to the thread handle

**Returns:** `0` on success, `-1` on error.

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

**Parameters:**
- `a` - pointer to the first thread handle
- `b` - pointer to the second thread handle

**Returns:** non-zero if threads are equal, `0` otherwise.

#### `fio_thread_current`

```c
fio_thread_t fio_thread_current(void);
```

Returns the current thread.

#### `fio_thread_yield`

```c
void fio_thread_yield(void);
```

Yields thread execution, allowing other threads to run.

#### `fio_thread_priority`

```c
fio_thread_priority_e fio_thread_priority(void);
```

Returns the current thread's priority level as a `fio_thread_priority_e` enum.

**Returns:** the current thread's priority, or `FIO_THREAD_PRIORITY_ERROR` on error.

#### `fio_thread_priority_set`

```c
int fio_thread_priority_set(fio_thread_priority_e pr);
```

Sets the current thread's priority level.

**Parameters:**
- `pr` - the priority level to set (see [`fio_thread_priority_e`](#fio_thread_priority_e))

**Returns:** `0` on success, `-1` on error.

### Mutex Functions

#### `FIO_THREADS_MUTEX_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these mutex functions are only declared, but they are **not** defined (implemented).

The facil.io C STL implementation expects you to provide your own alternatives.

#### `FIO_THREAD_MUTEX_INIT`

```c
#define FIO_THREAD_MUTEX_INIT /* platform specific */
```

Statically initializes a Mutex.

Example:

```c
fio_thread_mutex_t my_mutex = FIO_THREAD_MUTEX_INIT;
```

#### `fio_thread_mutex_init`

```c
int fio_thread_mutex_init(fio_thread_mutex_t *m);
```

Initializes a simple Mutex.

Or use the static initialization value: `FIO_THREAD_MUTEX_INIT`

**Parameters:**
- `m` - pointer to the mutex to initialize

**Returns:** `0` on success, non-zero on error.

#### `fio_thread_mutex_lock`

```c
int fio_thread_mutex_lock(fio_thread_mutex_t *m);
```

Locks a simple Mutex, blocking until the lock is acquired.

**Parameters:**
- `m` - pointer to the mutex

**Returns:** `0` on success, `-1` on error.

#### `fio_thread_mutex_trylock`

```c
int fio_thread_mutex_trylock(fio_thread_mutex_t *m);
```

Attempts to lock a simple Mutex without blocking.

**Parameters:**
- `m` - pointer to the mutex

**Returns:** `0` on success (lock acquired), non-zero if the mutex is already locked.

#### `fio_thread_mutex_unlock`

```c
int fio_thread_mutex_unlock(fio_thread_mutex_t *m);
```

Unlocks a simple Mutex.

**Parameters:**
- `m` - pointer to the mutex

**Returns:** `0` on success, `-1` on error.

#### `fio_thread_mutex_destroy`

```c
void fio_thread_mutex_destroy(fio_thread_mutex_t *m);
```

Destroys the simple Mutex (cleanup).

**Parameters:**
- `m` - pointer to the mutex to destroy

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

**Parameters:**
- `c` - pointer to the conditional variable to initialize

**Returns:** `0` on success, non-zero on error.

#### `fio_thread_cond_wait`

```c
int fio_thread_cond_wait(fio_thread_cond_t *c, fio_thread_mutex_t *m);
```

Waits on a conditional variable. The mutex MUST be previously locked.

**Parameters:**
- `c` - pointer to the conditional variable
- `m` - pointer to the mutex (must be locked before calling)

**Returns:** `0` on success, non-zero on error.

**Note**: the mutex is atomically released while waiting and re-acquired before returning.

#### `fio_thread_cond_timedwait`

```c
int fio_thread_cond_timedwait(fio_thread_cond_t *c,
                              fio_thread_mutex_t *m,
                              size_t milliseconds);
```

Waits on a conditional variable with a timeout. The mutex MUST be previously locked.

**Parameters:**
- `c` - pointer to the conditional variable
- `m` - pointer to the mutex (must be locked before calling)
- `milliseconds` - maximum time to wait in milliseconds

**Returns:** `0` on success (signaled), non-zero on timeout or error.

**Note**: the mutex is atomically released while waiting and re-acquired before returning.

#### `fio_thread_cond_signal`

```c
int fio_thread_cond_signal(fio_thread_cond_t *c);
```

Signals a simple conditional variable, waking one waiting thread.

**Parameters:**
- `c` - pointer to the conditional variable

**Returns:** `0` on success, non-zero on error.

#### `fio_thread_cond_destroy`

```c
void fio_thread_cond_destroy(fio_thread_cond_t *c);
```

Destroys a simple conditional variable.

**Parameters:**
- `c` - pointer to the conditional variable to destroy

### Multi-Threaded Memory Copy

#### `FIO_MEMCPY_THREADS`

```c
#ifndef FIO_MEMCPY_THREADS
#define FIO_MEMCPY_THREADS 8
#endif
```

Defines the maximum number of threads to use for multi-threaded memory copy operations.

Default value is `8`.

#### `FIO_MEMCPY_THREADS___MINCPY`

```c
#ifndef FIO_MEMCPY_THREADS___MINCPY
#define FIO_MEMCPY_THREADS___MINCPY (1ULL << 23)
#endif
```

Defines the minimum number of bytes required before multi-threaded copy is used.

Default value is `8388608` bytes (8 MB). Below this threshold, a single-threaded copy is performed.

#### `fio_thread_memcpy`

```c
size_t fio_thread_memcpy(const void *restrict dest,
                         void *restrict src,
                         size_t bytes);
```

Multi-threaded memcpy using up to `FIO_MEMCPY_THREADS` threads.

For small copies (below `FIO_MEMCPY_THREADS___MINCPY` bytes), a single-threaded copy is performed.

**Parameters:**
- `dest` - destination buffer
- `src` - source buffer
- `bytes` - number of bytes to copy

**Returns:** the number of threads used for the copy operation.

Example:

```c
#define FIO_THREADS
#include "fio-stl.h"

void copy_large_buffer(void) {
  char *src = malloc(16 * 1024 * 1024);  /* 16 MB */
  char *dest = malloc(16 * 1024 * 1024);
  
  /* Fill source with data... */
  
  size_t threads_used = fio_thread_memcpy(dest, src, 16 * 1024 * 1024);
  printf("Copy completed using %zu threads\n", threads_used);
  
  free(src);
  free(dest);
}
```

**Note**: this is a naive implementation intended for very large memory copies where parallelization may provide benefits.

-------------------------------------------------------------------------------
