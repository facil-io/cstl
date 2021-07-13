## Threads (portable)

The facil.io `FIO_THREADS` module provides a simple API for threading that is OS portable between POSIX systems and Windows OS.

The POSIX systems implementation uses `pthreads` under the hood.

Please note that due to thread return value and methodology differences, `FIO_THREADS` do not return any value.

The following methods are provided when the `FIO_THREADS` macro is defined before including the `fio-stl.h` header.


### Thread functions

#### `FIO_THREADS_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these thread functions are only declared, but they are **not** defined (implemented).

The implementation expects you to provide your own inline alternatives.

#### `fio_thread_create`
```c
int fio_thread_create(fio_thread_t *thread, void *(*start_function)(void *), void *arg);
```

Starts a new thread, returns 0 on success and -1 on failure.

#### `fio_thread_join`
```c
int fio_thread_join(fio_thread_t thread);
```

Waits for the thread to finish.

#### `fio_thread_detach`
```c
int fio_thread_detach(fio_thread_t thread);
```

Detaches the thread, so thread resources are freed automatically.

#### `fio_thread_exit`
```c
void fio_thread_exit(void);
```

Ends the current running thread.

#### `fio_thread_equal`
```c
int fio_thread_equal(fio_thread_t a, fio_thread_t b);
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

### Mutex functions

#### `FIO_THREADS_MUTEX_BYO`

* **BYO**: **B**ring **Y**our **O**wn.

If this macro is defined, these mutex functions are only declared, but they are **not** defined (implemented).

The implementation expects you to provide your own inline alternatives.

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



-------------------------------------------------------------------------------
