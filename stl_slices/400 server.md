## Simple Server

```c
#define FIO_SERVER
#include "fio-stl.h"
```

A simple server - `poll` based, evented and single-threaded - is included when `FIO_SERVER` is defined.

All API calls **must** be performed from the same thread used by the server to call the callbacks... that is, except for `fio_defer`, `fio_dup`, `fio_undup`, and `fio_udata_get`.

To handle IO events using other threads, first `fio_dup` the IO handle, then forward the pointer and any information to an external thread (possibly using a queue), then call `fio_defer` to schedule a task where a response can be written to the IO and all of the server's API is available. Remember to `fio_undup` when done with the IO handle. You might want to `fio_suspend` the `io` object to prevent new events from occurring.

**Note**: the `poll` system call becomes slow with only a few thousand sockets... if you expect thousands or more concurrent connections, please use the facil.io IO library (that uses `epoll` / `kqueue`).

**Note**: this will automatically include the API and features provided by defining `FIO_POLL`, `FIO_QUEUE`, `FIO_SOCK`, `FIO_TIME`, `FIO_STREAM`, `FIO_SIGNAL` and all their dependencies.

### `FIO_SERVER` API

The API depends on the opaque `fio_s` type as well as the `fio_protocol_s` type.

```c
/** The main protocol object type. See `struct fio_protocol_s`. */
typedef struct fio_protocol_s fio_protocol_s;
/** The main IO object type. Should be treated as an opaque pointer. */
typedef struct fio_s fio_s;
```

```c
SFUNC int fio_listen(struct fio_listen_args args);
#define fio_listen(...) fio_listen((struct fio_listen_args){__VA_ARGS__})
```

Sets up a network service / listening socket. The listening service will be destroyed when the server stops (unlike the listening service created using the facil.io IO library).

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
SFUNC fio_s *fio_attach_fd(int fd,
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

#### `fio_read`

```c
size_t fio_read(fio_s *io, void *buf, size_t len);
```

Reads data to the buffer, if any data exists. Returns the number of bytes read.
NOTE: zero (`0`) is a valid return value meaning no data was available.


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

#### `fio_close`

```c
void fio_close(fio_s *io);
```

Marks the IO for closure as soon as scheduled data was sent.

#### `fio_close_now`

```c
void fio_close_now(fio_s *io);
```

Marks the IO for immediate closure.

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

#### `fio_is_suspended`

```c
int fio_is_suspended(fio_s *io);
```

Returns non-zero if the IO is suspended (this might not be reliable information).

#### `fio_dup`

```c
fio_s *fio_dup(fio_s *io);
```

Increases a IO's reference count, so it won't be automatically destroyed when all tasks have completed.

Use this function in order to use the IO outside of a scheduled task.

This function is thread-safe.

#### `fio_undup`

```c
void fio_undup(fio_s *io);
```

Decreases a IO's reference count, so it could be automatically destroyed when all other tasks have completed.

Use this function once finished with a IO that was `dup`-ed.

This function is thread-safe.

#### `fio_defer`

```c
void fio_defer(void (*task)(void *u1, void *u2), void *udata1, void *udata2);
```

Schedules a task for delayed execution. This function is thread-safe and schedules the task within the Server's task queue.


### `fio_protocol_s`

The Protocol struct defines the callbacks used for a family of connections and sets their behavior. The Protocol struct is part of facil.io's core design both for the Simple Server and the fully featured IO library.

Protocols are usually global objects and the same protocol can be assigned to multiple IO handles.

All the callbacks receive an IO handle (except `on_close`), which is used instead of the system's file descriptor and protects callbacks and IO operations from sending data to incorrect clients (possible `fd` "recycling").


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
  /**
   * Called when the connection was closed, and all pending tasks are complete.
   */
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
   * Limited to FIO_SRV_TIMEOUT_MAX seconds. The value 0 will be the same as the
   * timeout limit.
   */
  uint32_t timeout;
};
```

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
#define FIO_SRV_TIMEOUT_MAX 300
```

Controls the maximum timeout in seconds (i.e., when timeout is not set).


-------------------------------------------------------------------------------
