## Server

```c
#define FIO_SERVER
#include "fio-stl.h"
```

An IO multiplexing server - evented and single-threaded - is included when `FIO_SERVER` is defined.

All server API calls **must** be performed from the same thread used by the server to call the callbacks... that is, except for functions specifically noted as thread safe, such as `fio_defer`, `fio_dup` and `fio_undup`.

To handle IO events using other threads, first `fio_dup` the IO handle, then forward the pointer and any additional information to an external thread (possibly using a queue). Once the external thread has completed its work, call `fio_defer` to schedule a task in which the IO and all of the server's API is available. Remember to `fio_undup` when done with the IO handle. You might want to `fio_suspend` the `io` object to prevent new `on_data` related events from occurring.

**Note**: this will automatically include a large amount of the facil.io STL modules, such as once provided by defining `FIO_POLL`, `FIO_QUEUE`, `FIO_SOCK`, `FIO_TIME`, `FIO_STREAM`, `FIO_SIGNAL` and all their dependencies.

### `FIO_SERVER` API

The API depends on the opaque `fio_s` type as well as the `fio_protocol_s` type.

```c
/** The main protocol object type. See `struct fio_protocol_s`. */
typedef struct fio_protocol_s fio_protocol_s;
/** The main IO object type. Should be treated as an opaque pointer. */
typedef struct fio_s fio_s;
```

#### `fio_listen`

```c
int fio_listen(struct fio_listen_args args);
#define fio_listen(...) fio_listen((struct fio_listen_args){__VA_ARGS__})
```

Sets up a network service / listening socket that will persist until the program exists (even if the server is restarted).

Returns 0 on success or -1 on error.

Accepts the following (named) arguments:

```c
/* Arguments for the fio_listen function */
struct fio_listen_args {
  /** The binding address in URL format. Defaults to: tcp://0.0.0.0:3000 */
  const char *url;
  /**
   * Called whenever a new connection is accepted (required).
   *
   * Should either call `fio_attach` or close the connection.
   */
  void (*on_open)(int fd, void *udata);
  /**
   * Called when the server is done, usable for cleanup.
   *
   * This will be called separately for every process before exiting.
   */
  void (*on_finish)(void *udata);
  /** Opaque user data. */
  void *udata;
};
```

#### `fio_attach_fd`

```c
fio_s *fio_attach_fd(int fd,
                     fio_protocol_s *protocol,
                     void *udata,
                     void *tls);
```
Attaches the socket in `fd` to the facio.io engine (reactor).

* `fd` should point to a valid socket.

* `protocol` may be the existing protocol or `NULL` (for partial hijack).

* `udata` is opaque user data and may be any value, including `NULL`.

* `tls` is a context for Transport Layer (Security) and can be used to redirect read/write operations, as set by the protocol.

Returns `NULL` on error. the `fio_s` pointer must NOT be used except within proper callbacks.

#### `fio_udata_set`

```c
void *fio_udata_set(fio_s *io, void *udata);
```

Associates a new `udata` pointer with the IO, returning the old `udata`

#### `fio_udata_get`

```c
void *fio_udata_get(fio_s *io);
```

Returns the `udata` pointer associated with the IO.

This is thread safe only on CPUs that read / write pointer sized words in a single operations.

#### `fio_read`

```c
size_t fio_read(fio_s *io, void *buf, size_t len);
```

Reads data to the buffer, if any data exists. Returns the number of bytes read.

**Note**: zero (`0`) is a valid return value meaning no data was available.

#### `fio_write2`

```c
void fio_write2(fio_s *io, fio_write_args_s args);
#define fio_write2(io, ...) fio_write2(io, (fio_write_args_s){__VA_ARGS__})
#define fio_write(io, buf_, len_) fio_write2(io, .buf = (buf_), .len = (len_), .copy = 1)
#define fio_sendfile(io, source_fd, offset_, bytes)                            \
  fio_write2((io),                                                             \
             .fd = (source_fd),                                                \
             .offset = (size_t)(offset_),                                      \
             .len = (bytes))
```

Writes data to the outgoing buffer or file and schedules the buffer or file to be sent.

`offset` dictates the starting point for the data to be sent and length sets the maximum amount of data to be sent.

Once the file was sent, the `source_fd` will be closed using `close`.

Files will be buffered to the socket chunk by chunk, so that memory consumption is capped.

Frees the buffer or closes the file on error.

Accepts the following named arguments:

```c
typedef struct {
  /** The buffer with the data to send (if no file descriptor) */
  void *buf;
  /** The file descriptor to send (if no buffer) */
  intptr_t fd;
  /** The length of the data to be sent. On files, 0 = the whole file. */
  size_t len;
  /** The length of the data to be sent. On files, 0 = the whole file. */
  size_t offset;
  /**
   * If this is a buffer, the de-allocation function used to free it.
   *
   * If NULL, the buffer will NOT be de-allocated.
   */
  void (*dealloc)(void *);
  /** If non-zero, makes a copy of the buffer or keeps a file open. */
  uint8_t copy;
} fio_write_args_s;
```

**Note**: these functions are thread safe except that message ordering isn't guarantied if writing from multiple threads - i.e., multiple `fio_write2` calls from different threads will not corrupt the underlying data structure and each `write` will appear atomic, but the order in which the different `write` calls isn't guaranteed.

#### `fio_close`

```c
void fio_close(fio_s *io);
```

Marks the IO for closure as soon as scheduled data was sent.

**Note**: this function is thread-safe.

#### `fio_close_now`

```c
void fio_close_now(fio_s *io);
```

Marks the IO for immediate closure.

**Note**: unlike `fio_close` this function is **NOT** thread-safe.

#### `fio_tls_set`

```c
void *fio_tls_set(fio_s *io, void *tls);
```

Associates a new `tls` pointer with the IO, returning the old `tls`

#### `fio_tls_get`

```c
void *fio_tls_get(fio_s *io);
```

Returns the `tls` pointer associated with the IO.

#### `fio_protocol_set`

```c
fio_protocol_s *fio_protocol_set(fio_s *io, fio_protocol_s *protocol);
```

Sets a new protocol object (allows for dynamic protocol substitution). `NULL` is a valid "only-write" protocol.

#### `fio_protocol_get`

```c
fio_protocol_s *fio_protocol_get(fio_s *io);
```

Returns a pointer to the current protocol object.

#### `fio_protocol_each`

```c
size_t fio_protocol_each(fio_protocol_s *protocol,
                         void (*task)(fio_s *, void *udata2),
                         void *udata2);
```

Performs a task for each IO in the stated protocol, returning the number of tasks performed.

If the task is more then a short action (such as more than a single `fio_write`), please consider scheduling the task using `fio_defer` while properly wrapping the task with calls to `fio_dup` and `fio_undup`.

i.e.:


```c
static my_long_task_wrapper_finish(fio_s *io, void * info) {
  // ... write results to IO and possibly free info pointer?
  fio_undup(io);
}

static my_long_task(fio_s *io, void * info) {
  // ... perform action in main thread or in a worker thread (CPU heavy?)
  fio_defer(my_long_task_wrapper_finish, io, info);
}

static my_long_task_wrapper_start(fio_s *io, void * info) {
  fio_defer(my_long_task, fio_dup(io), info);
}

void some_callback(fio_s *io) {
  void * info = NULL;
  // ...
  fio_protocol_each(&my_protocol, my_long_task_wrapper_start, info);
}
```

#### `fio_fd_get`

```c
int fio_fd_get(fio_s *io);
```

Returns the socket file descriptor (fd) associated with the IO.

#### `fio_touch`

```c
void fio_touch(fio_s *io);
```

Resets a socket's timeout counter.

#### `fio_suspend`

```c
void fio_suspend(fio_s *io);
```

Suspends future "on_data" events for the IO.

#### `fio_unsuspend`

```c
void fio_unsuspend(fio_s *io);
```

Listens for future "on_data" events related to the IO.

**Note**: this function is thread safe (though `fio_suspend` is **NOT**).

#### `fio_is_suspended`

```c
int fio_is_suspended(fio_s *io);
```

Returns non-zero if the IO is suspended (this might not be reliable information).

**Note**: this function is thread safe (though `fio_suspend` is **NOT**).

#### `fio_dup`

```c
fio_s *fio_dup(fio_s *io);
```

Increases a IO's reference count, so it won't be automatically destroyed when all tasks have completed.

Use this function in order to use the IO outside of a scheduled task.

**Note**: this function is thread-safe.

#### `fio_undup`

```c
void fio_undup(fio_s *io);
```

Decreases a IO's reference count, so it could be automatically destroyed when all other tasks have completed.

Use this function once finished with an IO that was `fio_dup`-ed.

**Note**: this function is thread-safe.

### Protocol Callbacks and Settings

The Protocol struct (`fio_protocol_s`) defines the callbacks used for a family of connections and sets their behavior. The Protocol struct is part of facil.io's core Server design.

Protocols are usually global objects and the same protocol can be assigned to multiple IO handles.

All the callbacks (except `on_close`) receive an IO handle, which is used instead of the system's file descriptor and protects callbacks and IO operations from sending data to incorrect clients (possible `fd` "recycling").

```c
struct fio_protocol_s {
  /**
   * Reserved / private data - used by facil.io internally.
   * MUST be initialized to zero.
   */
  struct {
    /* A linked list of currently attached IOs (ordered) - do NOT alter. */
    FIO_LIST_HEAD ios;
    /* A linked list of other protocols used by IO core - do NOT alter. */
    FIO_LIST_NODE protocols;
    /* internal flags - do NOT alter after initial initialization to zero. */
    uintptr_t flags;
  } reserved;
  /** Called when an IO is attached to a protocol. */
  void (*on_attach)(fio_s *io);
  /** Called when a data is available. */
  void (*on_data)(fio_s *io);
  /** called once all pending `fio_write` calls are finished. */
  void (*on_ready)(fio_s *io);
  /** Called after the connection was closed, and pending tasks completed. */
  void (*on_close)(void *udata);
  /**
   * Called when the server is shutting down, immediately before closing the
   * connection.
   *
   * After the `on_shutdown` callback returns, the socket is marked for closure.
   *
   * Once the socket was marked for closure, facil.io will allow a limited
   * amount of time for data to be sent, after which the socket might be closed
   * even if the client did not consume all buffered data.
   */
  void (*on_shutdown)(fio_s *io);
  /** Called when a connection's timeout was reached */
  void (*on_timeout)(fio_s *io);
  /**
   * Defines Transport Layer callbacks that facil.io will treat as non-blocking
   * system calls
   * */
  struct {
    /** Called to perform a non-blocking `read`, same as the system call. */
    ssize_t (*read)(int fd, void *buf, size_t len, void *tls);
    /** Called to perform a non-blocking `write`, same as the system call. */
    ssize_t (*write)(int fd, const void *buf, size_t len, void *tls);
    /** Sends any unsent internal data. Returns 0 only if all data was sent. */
    int (*flush)(int fd, void *tls);
    /** Decreases a fio_tls_s object's reference count, or frees the object. */
    void (*free)(void *tls);
  } io_functions;
  /**
   * The timeout value in seconds for all connections using this protocol.
   *
   * Limited to FIO_SRV_TIMEOUT_MAX seconds.
   * 
   * The zero value (0) is the same as the timeout limit (FIO_SRV_TIMEOUT_MAX).
   */
  uint32_t timeout;
};
```

### `FIO_SERVER` Connection Environment

Each connection object has its own personal environment storage that allows it to store named objects that are linked to the connection's lifetime.

#### `fio_env_set`
```c
void fio_env_set(fio_s *io, fio_env_set_args_s);
#define fio_env_set(io, ...) fio_env_set(io, (fio_env_set_args_s){__VA_ARGS__})
```

Links an object to a connection's lifetime, calling the `on_close` callback once the connection has died.

If the `io` is NULL, the value will be set for the global environment, in which case the `on_close` callback will only be called once the process exits.

The function is shadowed by the helper MACRO that allows the function to be called using named arguments:

```c
typedef struct {
  /** A numerical type filter. Defaults to 0. Negative values are reserved. */
  intptr_t type;
  /** The name of the object. The name and type uniquely identify the object. */
  fio_buf_info_s name;
  /** The object being linked to the connection. */
  void *udata;
  /** A callback that will be called once the connection is closed. */
  void (*on_close)(void *data);
  /** Set to true (1) if the name string's life lives as long as the `env` . */
  uint8_t const_name;
} fio_env_set_args_s;
```

**Note**: this function is thread-safe.

#### `fio_env_unset`

```c
int fio_env_unset(fio_s *io, fio_env_unset_args_s);
#define fio_env_unset(io, ...) fio_env_unset(io, (fio_env_unset_args_s){__VA_ARGS__})
```

Un-links an object from the connection's lifetime, so it's `on_close` callback will **not** be called.

Returns 0 on success and -1 if the object couldn't be found.

The function is shadowed by the helper MACRO that allows the function to be called using named arguments.

```c
typedef struct {
  /** A numerical type filter. Should be the same as used with `fio_env_set` */
  intptr_t type;
  /** The name of the object. Should be the same as used with `fio_env_set` */
  fio_buf_info_s name;
} fio_env_unset_args_s;
```

**Note**: this function is thread-safe.

#### `fio_env_remove`

```c
int fio_env_remove(fio_s *io, fio_env_unset_args_s);
#define fio_env_remove(io, ...) fio_env_remove(io, (fio_env_unset_args_s){__VA_ARGS__})
```

Removes an object from the connection's lifetime / environment, calling it's `on_close` callback as if the connection was closed.

The function is shadowed by the helper MACRO that allows the function to be called using named arguments.

**Note**: this function is thread-safe.

### Sarting / Stopping the Server

#### `fio_srv_start`

```c
void fio_srv_start(int workers);
```

Starts the server, using optional `workers` processes.

The function returns after the server stops either through a signal (`SIGINT` / `SIGTERM`) or by a call to `fio_srv_stop`.

**Note**: this function will block the current thread, using it as the main thread for the server.

Note: worker processes can be stopped and re-spawned by send the workers a `SIGINT` / `SIGTERM` or calling `fio_srv_stop` within the workers (i.e., by using a timer or sending a pub/sub message).

#### `fio_srv_stop`

```c
void fio_srv_stop(void);
```

Stopping the server.

#### `fio_srv_is_running`

```c
int fio_srv_is_running();
```

Returns true if server running and 0 if server stopped or shutting down.

#### `fio_srv_workers`

```c
uint16_t fio_srv_workers(int workers_requested);
```

Returns the number or workers the server will actually run.


#### `fio_srv_is_master`

```c
int fio_srv_is_master();
```

Returns true if the current process is the server's master process.

#### `fio_srv_is_worker`

```c
int fio_srv_is_worker();
```

Returns true if the current process is a server's worker process (it may, if not using any workers, also be the master process).

### Server Task Scheduling

#### `fio_defer`

```c
void fio_defer(void (*task)(void *u1, void *u2), void *udata1, void *udata2);
```

Schedules a task for delayed execution. This function schedules the task within the Server's task queue, so the task will execute within the server's thread, allowing all API calls to be made.

**Note**: this function is thread-safe.

#### `fio_run_every`

```c
void fio_run_every(fio_timer_schedule_args_s args);
#define fio_run_every(...)                                                     \
  fio_run_every((fio_timer_schedule_args_s){__VA_ARGS__})
```

Schedules a timer bound task, see [`fio_timer_schedule`](#fio_timer_schedule).

Possible "named arguments" (`fio_timer_schedule_args_s` members) include:

* The timer function. If it returns a non-zero value, the timer stops:

    ```c
    int (*fn)(void *, void *)
    ```

* Opaque user data:

    ```c
    void *udata1
    ```

* Opaque user data:

    ```c
    void *udata2
    ```

* Called when the timer is done (finished):

    ```c
    void (*on_finish)(void *, void *)
    ```

* Timer interval, in milliseconds:

    ```c
    uint32_t every
    ```

* The number of times the timer should be performed. -1 == infinity:

    ```c
    int32_t repetitions
    ```

#### `fio_last_tick`

```c
int64_t fio_last_tick(void);
```
Returns the last millisecond when the server reviewed pending IO events.

### `FIO_SERVER` Compile Time Macros


#### `FIO_SRV_BUFFER_PER_WRITE`

```c
#define FIO_SRV_BUFFER_PER_WRITE 65536U
```

Control the size of the on-stack buffer used for `write` events.

#### `FIO_SRV_THROTTLE_LIMIT`

```c
#define FIO_SRV_THROTTLE_LIMIT 2097152U
```

IO will be throttled (no `on_data` events) if outgoing buffer is large.

#### `FIO_SRV_TIMEOUT_MAX`

```c
#define FIO_SRV_TIMEOUT_MAX 300000
```

Controls the maximum timeout in milliseconds, as well as the default timeout when it isn't set.

#### `FIO_SRV_SHUTDOWN_TIMEOUT`

```c
#define FIO_SRV_SHUTDOWN_TIMEOUT 10000
```

Sets the hard timeout (in milliseconds) for the server's shutdown loop.

-------------------------------------------------------------------------------
