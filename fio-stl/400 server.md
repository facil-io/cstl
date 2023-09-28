## Server

```c
#define FIO_SERVER
#include "fio-stl.h"
```

An IO multiplexing server - evented and single-threaded - is included when `FIO_SERVER` is defined.

All server API calls **must** be performed from the same thread used by the server to call the callbacks... that is, except for functions specifically noted as thread safe, such as `fio_srv_defer`, `fio_dup` and `fio_undup`.

To handle IO events using other threads, first `fio_dup` the IO handle, then forward the pointer and any additional information to an external thread (possibly using a queue). Once the external thread has completed its work, call `fio_srv_defer` to schedule a task in which the IO and all of the server's API is available. Remember to `fio_undup` when done with the IO handle. You might want to `fio_srv_suspend` the `io` object to prevent new `on_data` related events from occurring.

**Note**: this will automatically include a large amount of the facil.io STL modules, such as once provided by defining `FIO_POLL`, `FIO_QUEUE`, `FIO_SOCK`, `FIO_TIME`, `FIO_STREAM`, `FIO_SIGNAL` and all their dependencies.

### Time Server Example

The following example uses the `FIO_PUBSUB` module together with the `FIO_SERVER` module to author a very simplistic time server (with no micro-second accuracy).

the `FIO_PUBSUB` module could have been replaced with a `fio_protocol_each` approach, but using `fio_protocol_each` isn't recommended.

```c
#define FIO_LOG
#define FIO_SERVER
#define FIO_PUBSUB
#define FIO_TIME
#include "fio-stl/include.h"

/* timer callback for publishing time */
static int publish_time(void *ignore1_, void *ignore2_);
/* fio_listen callback for accepting new clients */
static void accept_time_client(int fd, void *udata);

int main(void) {
  fio_srv_run_every(.fn = publish_time, .every = 1000, .repetitions = -1);
  FIO_ASSERT(!fio_listen(.on_open = accept_time_client), "");
  printf("* Time service starting up.\n");
  printf("  Press ^C to stop server and exit.\n");
  fio_srv_start(0);
}

/***** timer protocol and publishing implementation *****/

/* timer callback for publishing time */
static int publish_time(void *ignore1_, void *ignore2_) {
  char buf[32];
  size_t len = fio_time2iso(buf, fio_time_real().tv_sec);
  buf[len++] = '\r';
  buf[len++] = '\n';
  fio_publish(.channel = FIO_BUF_INFO1("time"),
              .message = FIO_BUF_INFO2(buf, len));
  return 0;
  (void)ignore1_, (void)ignore2_;
}

/** Called when an IO is attached to a protocol. */
FIO_SFUNC void time_protocol_on_attach(fio_s *io) {
  /* .on_message is unnecessary, by default the message is sent to the IO. */
  fio_subscribe(.io = io, .channel = FIO_BUF_INFO1("time"));
}

fio_protocol_s TIME_PROTOCOL = {
    .on_attach = time_protocol_on_attach,
    /* .on_data = NULL, .on_ready = NULL, .on_close = NULL, */
    .on_timeout = fio_touch, /* never times out */
};
static void accept_time_client(int fd, void *udata) {
  fio_srv_attach_fd(fd, &TIME_PROTOCOL, udata, NULL); /* udata isn't used here */
}
```

### `FIO_SERVER` API

The API depends on the opaque `fio_s` type as well as the `fio_protocol_s` type.

```c
/** The main IO object type. Should be treated as an opaque pointer. */
typedef struct fio_s fio_s;
/** The main protocol object type. See `struct fio_protocol_s`. */
typedef struct fio_protocol_s fio_protocol_s;
/** The IO functions used by the protocol object. */
typedef struct fio_io_functions fio_io_functions;
/** An opaque type used for the SSL/TLS helper functions. */
typedef struct fio_tls_s fio_tls_s;
```

Other (optional) helper types include the `fio_io_functions` and `fio_tls_s` types.

#### `fio_srv_listen`

```c
void *fio_srv_listen(struct fio_srv_listen_args args);
#define fio_srv_listen(...)                                                    \
  fio_srv_listen((struct fio_srv_listen_args){__VA_ARGS__})
```

Sets up a network service / listening socket that will persist until the program exists (even if the server is restarted).

Returns a listener handle that can be used with `fio_srv_listen_stop`.

Accepts the following (named) arguments:

```c
/* Arguments for the fio_listen function */
struct fio_listen_args {
  /**
   * The binding address in URL format. Defaults to: tcp://0.0.0.0:3000
   *
   * Note: `.url` accept an optional query for building a TLS context.
   *
   * Possible query values include:
   *
   * - `tls` or `ssl` (no value): sets TLS as active, possibly self-signed.
   * - `tls=` or `ssl=`: value is a prefix for "key.pem" and "cert.pem".
   * - `key=` and `cert=`: file paths for ".pem" files.
   *
   * i.e.:
   *
   *     fio_srv_listen(.url = "0.0.0.0:3000/?tls", ...);
   *     fio_srv_listen(.url = "0.0.0.0:3000/?tls=./", ...);
   *     // same as:
   *     fio_srv_listen(.url = "0.0.0.0:3000/"
   *                            "?key=./key.pem"
   *                            "&cert=./cert.pem", ...);
   */
  const char *url;
  /** The `fio_protocol_s` that will be assigned to incoming connections. */
  fio_protocol_s *protocol;
  /** The default `udata` set for (new) incoming connections. */
  void *udata;
  /** TLS object used for incoming connections (ownership moved to listener). */
  fio_tls_s *tls;
  /**
   * Called when the a listening socket starts to listen.
   *
   * May be called multiple times (i.e., if the server stops and starts again).
   */
  void (*on_start)(fio_protocol_s *protocol, void *udata);
  /**
   * Called during listener cleanup.
   *
   * This will be called separately for every process before exiting.
   */
  void (*on_finish)(fio_protocol_s *protocol, void *udata);
  /**
   * Selects a queue that will be used to schedule a pre-accept task.
   * May be used to test user thread stress levels before accepting connections.
   */
  fio_queue_s *queue_for_accept;
  /** If the server is forked - listen on the root process instead of workers */
  uint8_t on_root;
  /** Hides "started/stopped listening" messages from log (if set). */
  uint8_t hide_from_log;
};
```

#### `fio_srv_listen_stop`

```c
void fio_srv_listen_stop(void *listener);
```

Accepts a listener handler returned by `fio_srv_listen` and destroys it.

Normally this function isn't called, as the `listener` handle auto-destructs during server cleanup (at exit).

#### `fio_srv_attach_fd`

```c
fio_s *fio_srv_attach_fd(int fd,
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

#### `fio_srv_connect`

```c
/** Named arguments for fio_srv_connect */
typedef struct {
  /** The URL to connect to (may contain TLS hints in query / `tls` scheme). */
  const char *url;
  /** Connection protocol (once connection established). */
  fio_protocol_s *protocol;
  /** Called in case of a failed connection, use for cleanup. */
  void (*on_failed)(void *udata);
  /** Opaque user data (set only once connection was established). */
  void *udata;
  /** TLS builder object for TLS connections. */
  fio_tls_s *tls;
  /** Connection timeout in milliseconds (defaults to 30 seconds). */
  uint32_t timeout;
} fio_srv_connect_args_s;

/** Connects to a specific URL, returning the `fio_s` IO object or `NULL`. */
SFUNC fio_s *fio_srv_connect(fio_srv_connect_args_s args);
```

Connects to a remote URL (accepting TLS hints in the URL query and scheme). The protocol is only attached if the connection was established.

**Note**: use the `on_failed` callback if cleanup is required after a failed connection. The `on_close` callback is only called if connection was successful.

`fio_srv_connect` adds some overhead in parsing the URL for TLS hints and for wrapping the connection protocol for timeout and connection validation before calling the `on_attached`. If these aren't required, it's possible to simply open a socket and attach it like so:

```c
fio_srv_attach_fd(fio_sock_open2(url, FIO_SOCK_CLIENT | FIO_SOCK_NONBLOCK), protocol_pointer, udata, tls);
```

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

If the task is more then a short action (such as more than a single `fio_write`), please consider scheduling the task using `fio_srv_defer` while properly wrapping the task with calls to `fio_dup` and `fio_undup`.

i.e.:


```c
static my_long_task_wrapper_finish(fio_s *io, void * info) {
  // ... write results to IO and possibly free info pointer?
  fio_undup(io);
}

static my_long_task(fio_s *io, void * info) {
  // ... perform action in main thread or in a worker thread (CPU heavy?)
  fio_srv_defer(my_long_task_wrapper_finish, io, info);
}

static my_long_task_wrapper_start(fio_s *io, void * info) {
  fio_srv_defer(my_long_task, fio_dup(io), info);
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

#### `fio_srv_suspend`

```c
void fio_srv_suspend(fio_s *io);
```

Suspends future "on_data" events for the IO.

#### `fio_srv_unsuspend`

```c
void fio_srv_unsuspend(fio_s *io);
```

Listens for future "on_data" events related to the IO.

**Note**: this function is thread safe (though `fio_srv_suspend` is **NOT**).

#### `fio_srv_is_suspended`

```c
int fio_srv_is_suspended(fio_s *io);
```

Returns non-zero if the IO is suspended (this might not be reliable information).

**Note**: this function is thread safe (though `fio_srv_suspend` is **NOT**).

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

#### `fio_protocol_s`

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
  /** Called when a data is available - MUST `fio_read` until no data is available. */
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
  struct fio_io_functions {
    /** Helper that converts a `fio_tls_s` into the implementation's context. */
    void *(*build_context)(fio_tls_s *tls, uint8_t is_client);
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

Each connection object has its own personal environment storage that allows it to get / set named objects that are linked to the connection's lifetime.

**Note**: the Environment functions are designed to be **thread safe**. However, when using the Environment associated with a specific IO object, the code must hold a valid reference to the IO object ([`fio_dup`](#fio_dup) / [`fio_undup`](#fio_undup))

#### `fio_env_get`

```c
void *fio_env_get(fio_s *io, fio_env_get_args_s);
#define fio_env_get(io, ...) fio_env_get(io, (fio_env_get_args_s){__VA_ARGS__})
```

Returns the named `udata` associated with the IO object. Returns `NULL` both if no named object is found or it's `udata` was set to `NULL`.

If the `io` is `NULL`, the global environment will be used (see `fio_env_set`).

The function is shadowed by the helper MACRO that allows the function to be called using named arguments:

```c
typedef struct {
  /** A numerical type filter. Should be the same as used with `fio_env_set` */
  intptr_t type;
  /** The name of the object. Should be the same as used with `fio_env_set` */
  fio_buf_info_s name;
} fio_env_get_args_s;
```

#### `fio_env_set`
```c
void fio_env_set(fio_s *io, fio_env_set_args_s);
#define fio_env_set(io, ...) fio_env_set(io, (fio_env_set_args_s){__VA_ARGS__})
```

Links an object to a connection's lifetime, calling the `on_close` callback once the connection has died.

If the `io` is `NULL`, the value will be set for the global environment, in which case the `on_close` callback will only be called once the process exits.

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
int fio_env_unset(fio_s *io, fio_env_get_args_s);
#define fio_env_unset(io, ...) fio_env_unset(io, (fio_env_get_args_s){__VA_ARGS__})
```

Un-links an object from the connection's lifetime, so it's `on_close` callback will **not** be called.

Returns 0 on success and -1 if the object couldn't be found.

The function is shadowed by the helper MACRO that allows the function to be called using named arguments.

**Note**: this function is thread-safe.

#### `fio_env_remove`

```c
int fio_env_remove(fio_s *io, fio_env_get_args_s);
#define fio_env_remove(io, ...) fio_env_remove(io, (fio_env_get_args_s){__VA_ARGS__})
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

#### `fio_srv_defer`

```c
void fio_srv_defer(void (*task)(void *u1, void *u2), void *udata1, void *udata2);
```

Schedules a task for delayed execution. This function schedules the task within the Server's task queue, so the task will execute within the server's thread, allowing all API calls to be made.

**Note**: this function is thread-safe.

#### `fio_srv_run_every`

```c
void fio_srv_run_every(fio_timer_schedule_args_s args);
#define fio_srv_run_every(...)                                                     \
  fio_srv_run_every((fio_timer_schedule_args_s){__VA_ARGS__})
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

#### `fio_srv_last_tick`

```c
int64_t fio_srv_last_tick(void);
```
Returns the last millisecond when the server reviewed pending IO events.

### TLS/SSL Context Builder Helpers

The facil.io doesn't include an SSL/TLS library of its own, but it does offer an gateway API to allow implementations to be more library agnostic.

I.e., using this API, an implementation should be able to switch library implementations during runtime.

#### `fio_tls_new`

```c
fio_tls_s *fio_tls_new();
```

Performs a `new` operation, returning a new `fio_tls_s` context.

#### `fio_tls_dup`

```c
fio_tls_s *fio_tls_dup(fio_tls_s *);
```

Performs a `dup` operation, increasing the object's reference count.

#### `fio_tls_free`

```c
void fio_tls_free(fio_tls_s *);
```

Performs a `free` operation, reducing the reference count and freeing.

#### `fio_tls_from_url`

```c
fio_tls_s *fio_tls_from_url(fio_tls_s *existing_tls_or_null, fio_url_s url);
```

Takes a parsed URL and optional TLS target and returns a TLS if needed.

If the target `fio_tls_s *` is `NULL` and the URL requires TLS, a new TLS object will be returned.

If the target `fio_tls_s *` is not `NULL`, it will be returned after being updated.

The following URL _schemes_ are recognized as TLS schemes:  `tls` `https`, `wss`, `sses`, `tcps`, and `udps`.

The following _query parameters_ are recognized for effecting TLS schemes:

* `tls` - without value, indicates that TLS must be used. If no additional information is given, a self signed TLS certificate will be initialized when acting as a server.

* `tls=<file-prefix>` - will treat `<file-prefix>` as a prefix for a file path that ends with both `key.pem` and `cert.pem`. An empty `<file-prefix>` is valid. i.e.: `localhost:3000?tls=./` or `localhost:3000?tls=`

* `key=<file-path>` - a complete private key file path (usually a `.pem` file).

* `cert=<file-path>` - a complete public certificate file path (usually a `.pem` file).

**Note**: both scheme and query tests are **case insensitive**. Query values may be case sensitive, depending on file system.

#### `fio_tls_cert_add`

```c
fio_tls_s *fio_tls_cert_add(fio_tls_s *,
                            const char *server_name,
                            const char *public_cert_file,
                            const char *private_key_file,
                            const char *pk_password);
```

Adds a certificate a new SSL/TLS context / settings object (SNI support). i.e.:

```c
fio_tls_cert_add(tls, "www.example.com",
                     "public_key.pem",
                     "private_key.pem", NULL);
```

**Note**: Except for the `tls` and `server_name` arguments, all arguments might be `NULL`, which a context builder (`fio_io_functions_s`) should treat as a request for a self-signed certificate. It may be silently ignored.

#### `fio_tls_alpn_add`

```c
fio_tls_s *fio_tls_alpn_add(fio_tls_s *tls,
                            const char *protocol_name,
                            void (*on_selected)(fio_s *));
```

Adds an ALPN protocol callback to the SSL/TLS context.

The first protocol added will act as the default protocol to be selected.

A `NULL` protocol name will be silently ignored.

A `NULL` callback (`on_selected`) will be silently replaced with a no-op.

#### `fio_tls_alpn_select`

```c
int fio_tls_alpn_select(fio_tls_s *tls,
                               const char *protocol_name,
                               fio_s *);
```

Calls the `on_selected` callback for the `fio_tls_s` object.

Returns -1 on error (i.e., if the ALPN selected can't be found).

Returns 0 if a callback was called.

#### `fio_tls_trust_add`

```c
fio_tls_s *fio_tls_trust_add(fio_tls_s *, const char *public_cert_file);
```
Adds a certificate to the "trust" list, which automatically adds a peer verification requirement.

If `public_cert_file` is `NULL`, implementation is expected to add the system's default trust registry.

Note: when the `fio_tls_s` object is used for server connections, this should limit connections to clients that connect using a trusted certificate.

```c
fio_tls_trust_add(tls, "google-ca.pem" );
```

#### `fio_tls_cert_count`

```c
uintptr_t fio_tls_cert_count(fio_tls_s *tls);
```
Returns the number of `fio_tls_cert_add` instructions.

This could be used when deciding if to add a NULL instruction (self-signed).

If `fio_tls_cert_add` was never called, zero (0) is returned.
#### `fio_tls_alpn_count`

```c
uintptr_t fio_tls_alpn_count(fio_tls_s *tls);
```
Returns the number of registered ALPN protocol names.

This could be used when deciding if protocol selection should be delegated to the ALPN mechanism, or whether a protocol should be immediately assigned.

If no ALPN protocols are registered, zero (0) is returned.

#### `fio_tls_trust_count`

```c
uintptr_t fio_tls_trust_count(fio_tls_s *tls);
```
Returns the number of `fio_tls_trust_add` instructions.

This could be used when deciding if to disable peer verification or not.

If `fio_tls_trust_add` was never called, zero (0) is returned.


#### `fio_tls_each`

```c
/** Arguments (and info) for `fio_tls_each`. */
typedef struct fio_tls_each_s {
  fio_tls_s *tls;
  void *udata;
  void *udata2;
  int (*each_cert)(struct fio_tls_each_s *,
                   const char *server_name,
                   const char *public_cert_file,
                   const char *private_key_file,
                   const char *pk_password);
  int (*each_alpn)(struct fio_tls_each_s *,
                   const char *protocol_name,
                   void (*on_selected)(fio_s *));
  int (*each_trust)(struct fio_tls_each_s *, const char *public_cert_file);
} fio_tls_each_s;

/** Calls callbacks for certificate, trust certificate and ALPN added. */
int fio_tls_each(fio_tls_each_s);

/** `fio_tls_each` helper macro, see `fio_tls_each_s` for named arguments. */
#define fio_tls_each(tls_, ...)                                                \
  fio_tls_each(((fio_tls_each_s){.tls = tls_, __VA_ARGS__}))
```

Calls callbacks for ID certificates, trust certificates and ALPN added. Note that these values may be zero (`0`), in which case the callbacks might never be called.

Callbacks may be `NULL`.

**Note**: should be used to implement the `fio_io_functions_s` function `build_context`. If a copy of the `fio_tls_s` should be kept, use `fio_tls_dup`.

#### `fio_tls_default_io_functions`

```c
fio_io_functions_s fio_tls_default_io_functions(fio_io_functions_s *);
```

If `NULL` returns current default, otherwise sets it.

This allows SSL/TLS libraries to register as a default option, which will allow (future) protocol objects to be initialized with an SSL/TLS layer.

### Server-Bound Async Queue for non-IO tasks

The `fio_srv_async_s` will automatically spawn as many worker threads as requested when the server starts and guaranty a best attempt at a proper shutdown for when the server stops. See `fio_srv_async_init` for details.

#### `fio_srv_async_s`

```c
typedef struct {
  fio_queue_s *q;
  uint32_t count;
  fio_queue_s queue;
} fio_srv_async_s;

#define fio_srv_async(q, ...) fio_queue_push(q->q, __VA_ARGS__)
```

The `fio_srv_async` provides a server bound queue for non-IO tasks.

The queue automatically spawns threads and shuts down as the server starts or stops.

It is useful for thread-safe code or for scheduling non-IO bound tasks that can run in parallel to the server.

**Note**: It is recommended that the `fio_srv_async_s` be used as a static variable, as its memory must remain valid throughout the lifetime of the server's app.

#### `fio_srv_async_init`

```c
void fio_srv_async_init(fio_srv_async_s *q, uint32_t threads);
```

Initializes a server - async (multi-threaded) task queue.

It is recommended that the `fio_srv_async_s` be allocated as a static variable, as its memory must remain valid throughout the lifetime of the server's app.

The queue automatically spawns threads and shuts down as the server starts or stops.

**Note**: if the spawning threads failed or the object was initialized with zero threads, than the server's IO queue will be used for the async tasks as well.

#### `fio_srv_async_queue`

```c
fio_queue_s *fio_srv_async_queue(fio_srv_async_s *q) { return q->q; }
```

Returns the async queue's actual queues.

**Note**: if the spawning threads failed or the object was initialized with zero threads, than the server's IO queue will be used for the async tasks as well.

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
