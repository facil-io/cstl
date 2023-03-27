/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_SERVER             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




            A Simple Server - Evented, Reactor based, Single-Threaded



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_SERVER) && !defined(FIO___RECURSIVE_INCLUDE) &&                \
    !defined(H___FIO_SERVER___H)
#define H___FIO_SERVER___H
/* *****************************************************************************
Server Settings

At this point, define any MACROs and customizable settings available to the
developer.
***************************************************************************** */

#ifndef FIO_SRV_BUFFER_PER_WRITE
/** Control the size of the on-stack buffer used for `write` events. */
#define FIO_SRV_BUFFER_PER_WRITE 65536U
#endif

#ifndef FIO_SRV_THROTTLE_LIMIT
/** IO will be throttled (no `on_data` events) if outgoing buffer is large. */
#define FIO_SRV_THROTTLE_LIMIT 2097152U
#endif

#ifndef FIO_SRV_TIMEOUT_MAX
/** Controls the maximum and default timeout in milliseconds. */
#define FIO_SRV_TIMEOUT_MAX 300000
#endif

#ifndef FIO_SRV_SHUTDOWN_TIMEOUT
/* Sets the hard timeout (in milliseconds) for the server's shutdown loop. */
#define FIO_SRV_SHUTDOWN_TIMEOUT 10000
#endif

/* *****************************************************************************
IO Types
***************************************************************************** */

/** The main protocol object type. See `struct fio_protocol_s`. */
typedef struct fio_protocol_s fio_protocol_s;

/** The IO functions used by the protocol object. */
typedef struct fio_io_functions_s fio_io_functions_s;

/** The main IO object type. Should be treated as an opaque pointer. */
typedef struct fio_s fio_s;

/** An opaque type used for the SSL/TLS helper functions. */
typedef struct fio_tls_s fio_tls_s;

/* *****************************************************************************
Starting / Stopping the Server
***************************************************************************** */

/** Stopping the server. */
SFUNC void fio_srv_stop(void);

/** Starts the server, using optional `workers` processes. This will BLOCK! */
SFUNC void fio_srv_start(int workers);

/** Returns true if server running and 0 if server stopped or shutting down. */
SFUNC int fio_srv_is_running(void);

/** Returns true if the current process is the server's master process. */
SFUNC int fio_srv_is_master(void);

/** Returns true if the current process is a server's worker process. */
SFUNC int fio_srv_is_worker(void);

/** Returns the number or workers the server will actually run. */
SFUNC uint16_t fio_srv_workers(int workers_requested);

/* *****************************************************************************
Listening to Incoming Connections
***************************************************************************** */

/** Arguments for the fio_listen function */
struct fio_listen_args {
  /** The binding address in URL format. Defaults to: tcp://0.0.0.0:3000 */
  const char *url;
  /**
   * Called whenever a new connection is accepted (required).
   *
   * Should either call `fio_attach` or close the connection.
   */
  void (*on_open)(int fd, void *udata);
  /** Called when the a listening socket starts to listen (update state). */
  void (*on_start)(void *udata);
  /**
   * Called when the server is done, usable for cleanup.
   *
   * This will be called separately for every process before exiting.
   */
  void (*on_finish)(void *udata);
  /** Opaque user data. */
  void *udata;
  /**
   * Selects a queue that will be used to schedule a pre-accept task.
   * May be used to test user thread stress levels before accepting connections.
   */
  fio_queue_s *queue_for_accept;
  /** If the server is forked - listen on the root process or the workers? */
  uint8_t on_root;
  /** Hides "started/stopped listening" messages from log (if set). */
  uint8_t hide_from_log;
};

/**
 * Sets up a network service on a listening socket.
 *
 * Returns 0 on success or -1 on error.
 *
 * See the `fio_listen` Macro for details.
 */
SFUNC int fio_listen(struct fio_listen_args args);
#define fio_listen(...) fio_listen((struct fio_listen_args){__VA_ARGS__})

/* *****************************************************************************
Connecting as a Client
***************************************************************************** */

/** Connects to a specific URL, returning 0 on success and -1 on error. */
FIO_IFUNC int fio_connect(const char *url,
                          fio_protocol_s *protocol,
                          void *udata,
                          void *tls);

/* *****************************************************************************
IO Operations
***************************************************************************** */

/**
 * Attaches the socket in `fd` to the facio.io engine (reactor).
 *
 * * `fd` should point to a valid socket.
 *
 * * `protocol` may be the existing protocol or NULL (for partial hijack).
 *
 * * `udata` is opaque user data and may be any value, including NULL.
 *
 * * `tls` is a context for Transport Layer (Security) and can be used to
 *   redirect read/write operations, as set by the protocol.
 *
 * Returns NULL on error. the `fio_s` pointer must NOT be used except within
 * proper callbacks.
 */
SFUNC fio_s *fio_attach_fd(int fd,
                           fio_protocol_s *protocol,
                           void *udata,
                           void *tls);

/** Sets a new protocol object. `NULL` is a valid "only-write" protocol. */
SFUNC fio_protocol_s *fio_protocol_set(fio_s *io, fio_protocol_s *protocol);

/**
 * Returns a pointer to the current protocol object.
 *
 * If `protocol` wasn't properly set, the pointer might be invalid.
 */
SFUNC fio_protocol_s *fio_protocol_get(fio_s *io);

/** Associates a new `udata` pointer with the IO, returning the old `udata` */
FIO_IFUNC void *fio_udata_set(fio_s *io, void *udata);

/** Returns the `udata` pointer associated with the IO. */
FIO_IFUNC void *fio_udata_get(fio_s *io);

/** Associates a new `tls` pointer with the IO, returning the old `tls` */
FIO_IFUNC void *fio_tls_set(fio_s *io, void *tls);

/** Returns the `tls` pointer associated with the IO. */
FIO_IFUNC void *fio_tls_get(fio_s *io);

/** Returns the socket file descriptor (fd) associated with the IO. */
SFUNC int fio_fd_get(fio_s *io);

/* Resets a socket's timeout counter. */
SFUNC void fio_touch(fio_s *io);

/**
 * Reads data to the buffer, if any data exists. Returns the number of bytes
 * read.
 *
 * NOTE: zero (`0`) is a valid return value meaning no data was available.
 */
SFUNC size_t fio_read(fio_s *io, void *buf, size_t len);

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

/**
 * Writes data to the outgoing buffer and schedules the buffer to be sent.
 */
SFUNC void fio_write2(fio_s *io, fio_write_args_s args);
#define fio_write2(io, ...) fio_write2(io, (fio_write_args_s){__VA_ARGS__})

/** Helper macro for a common fio_write2 (copies the buffer). */
#define fio_write(io, buf_, len_)                                              \
  fio_write2(io, .buf = (buf_), .len = (len_), .copy = 1)

/**
 * Sends data from a file as if it were a single atomic packet (sends up to
 * length bytes or until EOF is reached).
 *
 * Once the file was sent, the `source_fd` will be closed using `close`.
 *
 * The file will be buffered to the socket chunk by chunk, so that memory
 * consumption is capped.
 *
 * `offset` dictates the starting point for the data to be sent and length sets
 * the maximum amount of data to be sent.
 *
 * Closes the file on error.
 */
#define fio_sendfile(io, source_fd, offset_, bytes)                            \
  fio_write2((io),                                                             \
             .fd = (source_fd),                                                \
             .offset = (size_t)(offset_),                                      \
             .len = (bytes))

/** Marks the IO for closure as soon as scheduled data was sent. */
SFUNC void fio_close(fio_s *io);

/** Marks the IO for immediate closure. */
SFUNC void fio_close_now(fio_s *io);

/**
 * Increases a IO's reference count, so it won't be automatically destroyed
 * when all tasks have completed.
 *
 * Use this function in order to use the IO outside of a scheduled task.
 *
 * This function is thread-safe.
 */
SFUNC fio_s *fio_dup(fio_s *io);

/**
 * Decreases a IO's reference count, so it could be automatically destroyed
 * when all other tasks have completed.
 *
 * Use this function once finished with a IO that was `dup`-ed.
 *
 * This function is thread-safe.
 */
SFUNC void fio_undup(fio_s *io);

/** Suspends future "on_data" events for the IO. */
SFUNC void fio_suspend(fio_s *io);

/** Listens for future "on_data" events related to the IO. */
SFUNC void fio_unsuspend(fio_s *io);

/** Returns 1 if the IO handle was suspended. */
SFUNC int fio_is_suspended(fio_s *io);

/** Returns 1 if the IO handle is marked as open. */
SFUNC int fio_is_open(fio_s *io);

/* *****************************************************************************
Task Scheduling
***************************************************************************** */

/** Schedules a task for delayed execution. This function is thread-safe. */
SFUNC void fio_defer(void (*task)(void *, void *), void *udata1, void *udata2);

/** Schedules a timer bound task, see `fio_timer_schedule`. */
SFUNC void fio_run_every(fio_timer_schedule_args_s args);
/**
 * Schedules a timer bound task, see `fio_timer_schedule`.
 *
 * Possible "named arguments" (fio_timer_schedule_args_s members) include:
 *
 * * The timer function. If it returns a non-zero value, the timer stops:
 *        int (*fn)(void *, void *)
 * * Opaque user data:
 *        void *udata1
 * * Opaque user data:
 *        void *udata2
 * * Called when the timer is done (finished):
 *        void (*on_finish)(void *, void *)
 * * Timer interval, in milliseconds:
 *        uint32_t every
 * * The number of times the timer should be performed. -1 == infinity:
 *        int32_t repetitions
 */
#define fio_run_every(...)                                                     \
  fio_run_every((fio_timer_schedule_args_s){__VA_ARGS__})

/** Returns the last millisecond when the server reviewed pending IO events. */
SFUNC int64_t fio_last_tick(void);

/** Returns a pointer for the server's queue. */
SFUNC fio_queue_s *fio_srv_queue(void);

/**************************************************************************/ /**
Protocol IO Functions
============

The Protocol struct uses IO callbacks to allow an easy way to override the
system's IO functions.

This defines Transport Layer callbacks that facil.io will treat as non-blocking
system calls and allows any protocol to easily add a secure (SSL/TLS) flavor if
desired.
*/
struct fio_io_functions_s {
  /** Helper that converts a `fio_tls_s` into the implementation's context. */
  void *(*build_context)(fio_tls_s *tls, uint8_t is_client);
  /** Helper to free the context built by build_context. */
  void (*free_context)(void *context);
  /** called when a new IO is first attached to a valid protocol. */
  void (*start)(fio_s *io);
  /** Called to perform a non-blocking `read`, same as the system call. */
  ssize_t (*read)(int fd, void *buf, size_t len, void *context);
  /** Called to perform a non-blocking `write`, same as the system call. */
  ssize_t (*write)(int fd, const void *buf, size_t len, void *context);
  /** Sends any unsent internal data. Returns 0 only if all data was sent. */
  int (*flush)(int fd, void *context);
  /** Decreases a fio_tls_s object's reference count, or frees the object. */
  void (*free)(void *context);
};

/**************************************************************************/ /**
The Protocol
============

The Protocol struct defines the callbacks used for a family of connections and
sets their behavior. The Protocol struct is part of facil.io's core design.

Protocols are usually global objects and the same protocol can be assigned to
multiple IO handles.

All the callbacks receive a IO handle, which is used instead of the system's
file descriptor and protects callbacks and IO operations from sending data to
incorrect clients (possible `fd` "recycling").
*/
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
   * system calls.
   */
  fio_io_functions_s io_functions;
  /**
   * The timeout value in seconds for all connections using this protocol.
   *
   * Limited to FIO_SRV_TIMEOUT_MAX seconds. Zero (0) == FIO_SRV_TIMEOUT_MAX
   */
  uint32_t timeout;
};

/** Performs a task for each IO in the stated protocol. */
FIO_SFUNC size_t fio_protocol_each(fio_protocol_s *protocol,
                                   void (*task)(fio_s *, void *udata2),
                                   void *udata2);

/* *****************************************************************************
Connection Object Links / Environment
***************************************************************************** */

/** Named arguments for the `fio_env_set` function. */
typedef struct {
  /** A numerical type filter. Defaults to 0. Negative values are reserved. */
  intptr_t type;
  /** The name for the link. The name and type uniquely identify the object. */
  fio_buf_info_s name;
  /** The object being linked to the connection. */
  void *udata;
  /** A callback that will be called once the connection is closed. */
  void (*on_close)(void *data);
  /** Set to true (1) if the name string's life lives as long as the `env` . */
  uint8_t const_name;
} fio_env_set_args_s;

/** Named arguments for the `fio_env_unset` function. */
typedef struct {
  /** A numerical type filter. Should be the same as used with `fio_env_set` */
  intptr_t type;
  /** The name of the object. Should be the same as used with `fio_env_set` */
  fio_buf_info_s name;
} fio_env_unset_args_s;

/**
 * Links an object to a connection's lifetime / environment.
 *
 * The `on_close` callback will be called once the connection has died.
 *
 * If the `io` is NULL, the value will be set for the global environment.
 */
SFUNC void fio_env_set(fio_s *io, fio_env_set_args_s);

/**
 * Links an object to a connection's lifetime, calling the `on_close` callback
 * once the connection has died.
 *
 * If the `io` is NULL, the value will be set for the global environment, in
 * which case the `on_close` callback will only be called once the process
 * exits.
 *
 * This is a helper MACRO that allows the function to be called using named
 * arguments.
 */
#define fio_env_set(io, ...) fio_env_set(io, (fio_env_set_args_s){__VA_ARGS__})

/**
 * Un-links an object from the connection's lifetime, so it's `on_close`
 * callback will NOT be called.
 *
 * Returns 0 on success and -1 if the object couldn't be found.
 */
SFUNC int fio_env_unset(fio_s *io, fio_env_unset_args_s);

/**
 * Un-links an object from the connection's lifetime, so it's `on_close`
 * callback will NOT be called.
 *
 * Returns 0 on success and -1 if the object couldn't be found.
 *
 * This is a helper MACRO that allows the function to be called using named
 * arguments.
 */
#define fio_env_unset(io, ...)                                                 \
  fio_env_unset(io, (fio_env_unset_args_s){__VA_ARGS__})

/**
 * Removes an object from the connection's lifetime / environment, calling it's
 * `on_close` callback as if the connection was closed.
 */
SFUNC int fio_env_remove(fio_s *io, fio_env_unset_args_s);

/**
 * Removes an object from the connection's lifetime / environment, calling it's
 * `on_close` callback as if the connection was closed.
 *
 * This is a helper MACRO that allows the function to be called using named
 * arguments.
 */
#define fio_env_remove(io, ...)                                                \
  fio_env_remove(io, (fio_env_unset_args_s){__VA_ARGS__})

/* *****************************************************************************
TLS Context Helper Types
***************************************************************************** */

/** Performs a `new` operation, returning a new `fio_tls_s` context. */
SFUNC fio_tls_s *fio_tls_new(void);

/** Performs a `dup` operation, increasing the object's reference count. */
SFUNC fio_tls_s *fio_tls_dup(fio_tls_s *);

/** Performs a `free` operation, reducing the reference count and freeing. */
SFUNC void fio_tls_free(fio_tls_s *);

/**
 * Adds a certificate a new SSL/TLS context / settings object (SNI support).
 *
 *      fio_tls_cert_add(tls, "www.example.com",
 *                            "public_key.pem",
 *                            "private_key.pem", NULL );
 *
 * NOTE: Except for the `tls` and `server_name` arguments, all arguments might
 * be `NULL`, which a context builder (`fio_io_functions_s`) should treat as a
 * request for a self-signed certificate. It may be silently ignored.
 */
SFUNC fio_tls_s *fio_tls_cert_add(fio_tls_s *,
                                  const char *server_name,
                                  const char *public_cert_file,
                                  const char *private_key_file,
                                  const char *pk_password);

/**
 * Adds an ALPN protocol callback to the SSL/TLS context.
 *
 * The first protocol added will act as the default protocol to be selected.
 *
 * A `NULL` protocol name will be silently ignored.
 *
 * A `NULL` callback (`on_selected`) will be silently replaced with a no-op.
 */
SFUNC fio_tls_s *fio_tls_alpn_add(fio_tls_s *tls,
                                  const char *protocol_name,
                                  void (*on_selected)(fio_s *));

/** Calls the `on_selected` callback for the `fio_tls_s` object. */
SFUNC int fio_tls_alpn_select(fio_tls_s *tls,
                              const char *protocol_name,
                              size_t name_length,
                              fio_s *);

/**
 * Adds a certificate to the "trust" list, which automatically adds a peer
 * verification requirement.
 *
 * If `public_cert_file` is `NULL`, implementation is expected to add the
 * system's default trust registry.
 *
 * Note: when the `fio_tls_s` object is used for server connections, this should
 * limit connections to clients that connect using a trusted certificate.
 *
 *      fio_tls_trust_add(tls, "google-ca.pem" );
 */
SFUNC fio_tls_s *fio_tls_trust_add(fio_tls_s *, const char *public_cert_file);

/**
 * Returns the number of `fio_tls_cert_add` instructions.
 *
 * This could be used when deciding if to add a NULL instruction (self-signed).
 *
 * If `fio_tls_cert_add` was never called, zero (0) is returned.
 */
SFUNC uintptr_t fio_tls_cert_count(fio_tls_s *tls);

/**
 * Returns the number of registered ALPN protocol names.
 *
 * This could be used when deciding if protocol selection should be delegated to
 * the ALPN mechanism, or whether a protocol should be immediately assigned.
 *
 * If no ALPN protocols are registered, zero (0) is returned.
 */
SFUNC uintptr_t fio_tls_alpn_count(fio_tls_s *tls);

/**
 * Returns the number of `fio_tls_trust_add` instructions.
 *
 * This could be used when deciding if to disable peer verification or not.
 *
 * If `fio_tls_trust_add` was never called, zero (0) is returned.
 */
SFUNC uintptr_t fio_tls_trust_count(fio_tls_s *tls);

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
SFUNC int fio_tls_each(fio_tls_each_s);

/** `fio_tls_each` helper macro, see `fio_tls_each_s` for named arguments. */
#define fio_tls_each(tls_, ...)                                                \
  fio_tls_each(((fio_tls_each_s){.tls = tls_, __VA_ARGS__}))

/** If `NULL` returns current default, otherwise sets it. */
SFUNC fio_io_functions_s fio_tls_default_io_functions(fio_io_functions_s *);

/* *****************************************************************************
Server Async - Worker Threads for non-IO tasks
***************************************************************************** */

/** The Server Async Queue type. */
typedef struct {
  fio_queue_s *q;
  uint32_t count;
  fio_queue_s queue;
  FIO_LIST_NODE node;
} fio_srv_async_s;

/**
 * Initializes a server - async (multi-threaded) task queue.
 *
 * It is recommended that the `fio_srv_async_s` be allocated as a static
 * variable, as its memory must remain valid throughout the lifetime of the
 * server's app.
 *
 * The queue automatically spawns threads and shuts down as the server starts or
 * stops.
 */
FIO_IFUNC fio_queue_s *fio_srv_async_queue(fio_srv_async_s *q) { return q->q; }

/**
 * Initializes an async server queue for multi-threaded (non IO) tasks.
 *
 * This can function can only be called from the server's thread (or the thread
 * that will eventually run the server).
 */
SFUNC void fio_srv_async_init(fio_srv_async_s *q, uint32_t threads);

#define fio_srv_async(q, ...) fio_queue_push(q->q, __VA_ARGS__)

/* *****************************************************************************
TODO!: Helpers fio_srv_queue_get && fio_srv_pid
***************************************************************************** */

/* *****************************************************************************
Simple Server Implementation - inlined static functions
***************************************************************************** */

/** Defines a get / set function for the property. */
#define FIO_SERVER_GETSET_FUNC(property, index)                                \
  FIO_IFUNC void *fio_##property##_set(fio_s *io, void *property) {            \
    void *old = ((void **)io)[index];                                          \
    ((void **)io)[index] = property;                                           \
    return old;                                                                \
  }                                                                            \
  FIO_IFUNC void *fio_##property##_get(fio_s *io) {                            \
    return ((void **)io)[index];                                               \
  }
FIO_SERVER_GETSET_FUNC(udata, 0)
FIO_SERVER_GETSET_FUNC(tls, 1)

/** Connects to a specific URL, returning 0 on success and -1 on error. */
FIO_IFUNC int fio_connect(const char *url,
                          fio_protocol_s *protocol,
                          void *udata,
                          void *tls) {
  int fd = fio_sock_open2(url, FIO_SOCK_CLIENT);
  if (fd == -1)
    return -1;
  return (0 - !fio_attach_fd(fd, protocol, udata, tls));
}

/* *****************************************************************************



          Simple Server Implementation - possibly externed functions.


REMEMBER: memory allocations: FIO_MEM_REALLOC_ / FIO_MEM_FREE_
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Protocol validation
***************************************************************************** */

static void fio___srv_on_ev_mock_sus(fio_s *io) { fio_suspend(io); }
static void fio___srv_on_ev_mock(fio_s *io) { (void)(io); }
static void fio___srv_on_close_mock(void *ptr) { (void)ptr; }
static void fio___srv_on_ev_on_timeout(fio_s *io) { fio_close_now(io); }
static void fio___srv_on_timeout_never(fio_s *io) { fio_touch(io); }

/* Called to perform a non-blocking `read`, same as the system call. */
static ssize_t fio___io_func_default_read(int fd,
                                          void *buf,
                                          size_t len,
                                          void *tls) {
  return fio_sock_read(fd, buf, len);
  (void)tls;
}
/** Called to perform a non-blocking `write`, same as the system call. */
static ssize_t fio___io_func_default_write(int fd,
                                           const void *buf,
                                           size_t len,
                                           void *tls) {
  return fio_sock_write(fd, buf, len);
  (void)tls;
}
/** Sends any unsent internal data. Returns 0 only if all data was sent. */
static int fio___io_func_default_flush(int fd, void *tls) {
  return 0;
  (void)fd, (void)tls;
}
/** Builds a local TLS context out of the fio_tls_s object. */
static void *fio___io_func_default_build_context(fio_tls_s *tls,
                                                 uint8_t is_client) {
  FIO_ASSERT(0,
             "SSL/TLS `build_context` was called, but no SSL/TLS "
             "implementation found.");
  return NULL;
  (void)tls, (void)is_client;
}
/** Builds a local TLS context out of the fio_tls_s object. */
static void fio___io_func_default_free_context(void *context) {
  FIO_ASSERT(0,
             "SSL/TLS `free_context` was called, but no SSL/TLS "
             "implementation found.");
  (void)context;
}

FIO_SFUNC void fio___srv_init_protocol(fio_protocol_s *pr) {
  pr->reserved.protocols = FIO_LIST_INIT(pr->reserved.protocols);
  pr->reserved.ios = FIO_LIST_INIT(pr->reserved.ios);
  if (!pr->on_attach)
    pr->on_attach = fio___srv_on_ev_mock;
  if (!pr->on_data)
    pr->on_data = fio___srv_on_ev_mock_sus;
  if (!pr->on_ready)
    pr->on_ready = fio___srv_on_ev_mock;
  if (!pr->on_close)
    pr->on_close = fio___srv_on_close_mock;
  if (!pr->on_shutdown)
    pr->on_shutdown = fio___srv_on_ev_mock;
  if (!pr->on_timeout)
    pr->on_timeout = fio___srv_on_ev_on_timeout;
  if (!pr->io_functions.build_context)
    pr->io_functions.build_context = fio___io_func_default_build_context;
  if (!pr->io_functions.free_context)
    pr->io_functions.free_context = fio___io_func_default_free_context;
  if (!pr->io_functions.start)
    pr->io_functions.start = fio___srv_on_ev_mock;
  if (!pr->io_functions.read)
    pr->io_functions.read = fio___io_func_default_read;
  if (!pr->io_functions.write)
    pr->io_functions.write = fio___io_func_default_write;
  if (!pr->io_functions.flush)
    pr->io_functions.flush = fio___io_func_default_flush;
  if (!pr->io_functions.free)
    pr->io_functions.free = fio___srv_on_close_mock;
}

/* the FIO___MOCK_PROTOCOL is used to manage hijacked / zombie connections. */
static fio_protocol_s FIO___MOCK_PROTOCOL;

FIO_IFUNC void fio___srv_init_protocol_test(fio_protocol_s *pr) {
  if (!fio_atomic_or(&pr->reserved.flags, 1))
    fio___srv_init_protocol(pr);
}

/* *****************************************************************************
Server / IO environment support (`env`)
***************************************************************************** */

/** An object that can be linked to any facil.io connection (fio_s). */
typedef struct {
  void (*on_close)(void *data);
  void *udata;
} fio___srv_env_obj_s;

/* unordered `env` dictionary style map */
#define FIO_UMAP_NAME fio___srv_env
#define FIO_MAP_KEY_KSTR
#define FIO_MAP_VALUE fio___srv_env_obj_s
#define FIO_MAP_VALUE_DESTROY(o)                                               \
  do {                                                                         \
    if ((o).on_close)                                                          \
      (o).on_close((o).udata);                                                 \
  } while (0)
#define FIO_MAP_DESTROY_AFTER_COPY 0

#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

typedef struct {
  fio_thread_mutex_t lock;
  fio___srv_env_s env;
} fio___srv_env_safe_s;

#define FIO___SRV_ENV_SAFE_INIT                                                \
  { .lock = FIO_THREAD_MUTEX_INIT, .env = FIO_MAP_INIT }

FIO_IFUNC void fio___srv_env_safe_set(fio___srv_env_safe_s *e,
                                      char *key_,
                                      size_t len,
                                      intptr_t type_,
                                      fio___srv_env_obj_s val,
                                      uint8_t key_is_const) {
  fio_str_info_s key = FIO_STR_INFO3(key_, len, !key_is_const);
  const uint64_t hash = fio_risky_hash(key_, len, (uint64_t)(type_));
  fio_thread_mutex_lock(&e->lock);
  fio___srv_env_set(&e->env, hash, key, val, NULL);
  fio_thread_mutex_unlock(&e->lock);
}

FIO_IFUNC int fio___srv_env_safe_unset(fio___srv_env_safe_s *e,
                                       char *key_,
                                       size_t len,
                                       intptr_t type_) {
  int r;
  fio_str_info_s key = FIO_STR_INFO3(key_, len, 0);
  const uint64_t hash = fio_risky_hash(key_, len, (uint64_t)(type_));
  fio___srv_env_obj_s old;
  fio_thread_mutex_lock(&e->lock);
  r = fio___srv_env_remove(&e->env, hash, key, &old);
  fio_thread_mutex_unlock(&e->lock);
  return r;
}

FIO_IFUNC int fio___srv_env_safe_remove(fio___srv_env_safe_s *e,
                                        char *key_,
                                        size_t len,
                                        intptr_t type_) {
  int r;
  fio_str_info_s key = FIO_STR_INFO3(key_, len, 0);
  const uint64_t hash = fio_risky_hash(key_, len, (uint64_t)(type_));
  fio_thread_mutex_lock(&e->lock);
  r = fio___srv_env_remove(&e->env, hash, key, NULL);
  fio_thread_mutex_unlock(&e->lock);
  return r;
}

FIO_IFUNC void fio___srv_env_safe_destroy(fio___srv_env_safe_s *e) {
  fio___srv_env_destroy(&e->env); /* no need to lock, performed in IO thread. */
  fio_thread_mutex_destroy(&e->lock);
  *e = (fio___srv_env_safe_s)FIO___SRV_ENV_SAFE_INIT;
}

/* *****************************************************************************
IO Validity Map - Type
***************************************************************************** */
#ifndef FIO_VALIDITY_MAP_USE
#define FIO_VALIDITY_MAP_USE 0
#endif

#if FIO_VALIDITY_MAP_USE
#define FIO_UMAP_NAME         fio_validity_map
#define FIO_MAP_KEY           fio_s *
#define FIO_MAP_HASH_FN(o)    fio_risky_ptr(o)
#define FIO_MAP_KEY_CMP(a, b) ((a) == (b))
#ifndef FIO_VALIDATE_IO_MUTEX
/* mostly for debugging possible threading issues. */
#define FIO_VALIDATE_IO_MUTEX 0
#endif
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
#else
typedef void *fio_validity_map_s;
#endif

/* *****************************************************************************
Global State
***************************************************************************** */

static void fio___srv_poll_on_data_schd(void *udata);
static void fio___srv_poll_on_ready_schd(void *udata);
static void fio___srv_poll_on_close_schd(void *udata);

static struct {
  FIO_LIST_HEAD protocols;
#if FIO_VALIDITY_MAP_USE
  fio_validity_map_s valid;
#if FIO_VALIDATE_IO_MUTEX
  fio_thread_mutex_t valid_lock;
#endif
#endif /* FIO_VALIDITY_MAP_USE */
  fio___srv_env_safe_s env;
  fio_poll_s poll_data;
  int64_t tick;
  pid_t root_pid;
  pid_t pid;
  fio_s *wakeup;
  int wakeup_fd;
  int wakeup_wait;
  uint16_t workers;
  uint8_t is_worker;
  volatile uint8_t stop;
  FIO_LIST_HEAD async;
} fio___srvdata = {
#if FIO_VALIDATE_IO_MUTEX && FIO_VALIDITY_MAP_USE
    .valid_lock = FIO_THREAD_MUTEX_INIT,
#endif
#if !FIO_OS_WIN
    .env = FIO___SRV_ENV_SAFE_INIT,
#endif
    .tick = 0,
    .wakeup_fd = -1,
    .stop = 1,
};

/* *****************************************************************************
Wakeup Protocol
***************************************************************************** */

FIO_SFUNC void fio___srv_wakeup_cb(fio_s *io) {
  char buf[512];
  ssize_t r = fio_sock_read(fio_fd_get(io), buf, 512);
  (void)r;
  fio___srvdata.wakeup_wait = 0;
#if DEBUG
  FIO_LOG_DEBUG2("(%d) fio___srv_wakeup called", fio___srvdata.pid);
#endif
}
FIO_SFUNC void fio___srv_wakeup_on_close(void *ignr_) {
  (void)ignr_;
  fio_sock_close(fio___srvdata.wakeup_fd);
  fio___srvdata.wakeup = NULL;
  fio___srvdata.wakeup_fd = -1;
  FIO_LOG_DEBUG2("(%d) fio___srv_wakeup destroyed", fio___srvdata.pid);
}

FIO_SFUNC void fio___srv_wakeup(void) {
  /* TODO, skip wakeup for same thread caller? */
  if (!fio___srvdata.wakeup || fio___srvdata.wakeup_wait ||
      fio_queue_count(fio_srv_queue()) > 3)
    return;
  fio___srvdata.wakeup_wait = 1;
  char buf[1] = {~0};
  ssize_t ignr = fio_sock_write(fio___srvdata.wakeup_fd, buf, 1);
  (void)ignr;
}

static fio_protocol_s FIO___SRV_WAKEUP_PROTOCOL = {
    .on_data = fio___srv_wakeup_cb,
    .on_close = fio___srv_wakeup_on_close,
    .on_timeout = fio___srv_on_timeout_never,
};

FIO_SFUNC void fio___srv_wakeup_init(void) {
  if (fio___srvdata.wakeup)
    return;
  int fds[2];
  if (pipe(fds)) {
    FIO_LOG_ERROR("(%d) couldn't open wakeup pipes, fio___srv_wakeup disabled.",
                  fio___srvdata.pid);
    return;
  }
  fio_sock_set_non_block(fds[0]);
  fio_sock_set_non_block(fds[1]);
  fio___srvdata.wakeup_fd = fds[1];
  fio___srvdata.wakeup = fio_attach_fd(fds[0],
                                       &FIO___SRV_WAKEUP_PROTOCOL,
                                       (void *)(uintptr_t)fds[1],
                                       NULL);
  FIO_LOG_DEBUG2("(%d) fio___srv_wakeup initialized", fio___srvdata.pid);
}

/* *****************************************************************************
Server Timers and Task Queues
***************************************************************************** */

static fio_timer_queue_s fio___srv_timer[1] = {FIO_TIMER_QUEUE_INIT};
static fio_queue_s fio___srv_tasks[1];

/** Returns the last millisecond when the server reviewed pending IO events. */
SFUNC int64_t fio_last_tick(void) { return fio___srvdata.tick; }

/** Schedules a task for delayed execution. This function is thread-safe. */
SFUNC void fio_defer(void (*task)(void *, void *), void *udata1, void *udata2) {
  fio_queue_push(fio___srv_tasks, task, udata1, udata2);
  fio___srv_wakeup();
}

/** Schedules a timer bound task, see `fio_timer_schedule` in the CSTL. */
SFUNC void fio_run_every FIO_NOOP(fio_timer_schedule_args_s args) {
  args.start_at += ((uint64_t)0 - !args.start_at) & fio___srvdata.tick;
  fio_timer_schedule FIO_NOOP(fio___srv_timer, args);
}

/** Returns a pointer for the server's queue. */
SFUNC fio_queue_s *fio_srv_queue(void) { return fio___srv_tasks; }

/* *****************************************************************************
IO Validity Map - Implementation
***************************************************************************** */
#if FIO_VALIDITY_MAP_USE

#if FIO_VALIDATE_IO_MUTEX
#define FIO_VALIDATE_LOCK()   fio_thread_mutex_lock(&fio___srvdata.valid_lock)
#define FIO_VALIDATE_UNLOCK() fio_thread_mutex_unlock(&fio___srvdata.valid_lock)
#define FIO_VALIDATE_LOCK_DESTROY()                                            \
  fio_thread_mutex_destroy(&fio___srvdata.valid_lock)
#else
#define FIO_VALIDATE_LOCK()
#define FIO_VALIDATE_UNLOCK()
#define FIO_VALIDATE_LOCK_DESTROY()
#endif

FIO_IFUNC int fio_is_valid(fio_s *io) {
  FIO_VALIDATE_LOCK();
  fio_s *r = fio_validity_map_get(&fio___srvdata.valid, fio_risky_ptr(io), io);
  FIO_VALIDATE_UNLOCK();
  return r == io;
}

FIO_IFUNC void fio_set_valid(fio_s *io) {
  FIO_VALIDATE_LOCK();
  fio_validity_map_set(&fio___srvdata.valid, fio_risky_ptr(io), io, NULL);
  FIO_VALIDATE_UNLOCK();
  FIO_ASSERT_DEBUG(fio_is_valid(io),
                   "(%d) IO validity set, but map reported as invalid!",
                   (int)fio___srvdata.pid);
  FIO_LOG_DEBUG2("(%d) IO %p is now valid", (int)fio___srvdata.pid, (void *)io);
}

FIO_IFUNC void fio_set_invalid(fio_s *io) {
  fio_s *old = NULL;
  FIO_LOG_DEBUG2("(%d) IO %p is no longer valid",
                 (int)fio___srvdata.pid,
                 (void *)io);
  FIO_VALIDATE_LOCK();
  fio_validity_map_remove(&fio___srvdata.valid, fio_risky_ptr(io), io, &old);
  FIO_VALIDATE_UNLOCK();
  FIO_ASSERT_DEBUG(!old || old == io,
                   "(%d) invalidity map corruption (%p != %p)!",
                   (int)fio___srvdata.pid,
                   io,
                   old);
  FIO_ASSERT_DEBUG(!fio_is_valid(io),
                   "(%d) IO validity removed, but map reported as valid!",
                   (int)fio___srvdata.pid);
}

FIO_IFUNC void fio_invalidate_all() {
  FIO_VALIDATE_LOCK();
  fio_validity_map_destroy(&fio___srvdata.valid);
  FIO_VALIDATE_UNLOCK();
  FIO_VALIDATE_LOCK_DESTROY();
}

/** Returns an approximate number of IO objects attached. */
SFUNC size_t fio_io_count(void) { /* TODO: remove? */
  return fio_validity_map_count(&fio___srvdata.valid);
}

#undef FIO_VALIDATE_LOCK
#undef FIO_VALIDATE_UNLOCK
#undef FIO_VALIDATE_LOCK_DESTROY
#else /* FIO_VALIDITY_MAP_USE */
#define fio_is_valid(io) 1
#define fio_set_valid(io)
#define fio_set_invalid(io)
#define fio_invalidate_all()
#endif /* FIO_VALIDITY_MAP_USE */
/* *****************************************************************************
IO objects
***************************************************************************** */

struct fio_s {
  void *udata;
  void *tls;
  fio_protocol_s *pr;
  FIO_LIST_NODE node;
  fio_stream_s stream;
  fio___srv_env_safe_s env;
#ifdef DEBUG
  size_t total_sent;
#endif
  int64_t active;
  uint32_t state;
  int fd;
  /* TODO? peer address buffer */
};

#define FIO_STATE_OPEN      ((uint32_t)1U)
#define FIO_STATE_SUSPENDED ((uint32_t)2U)
#define FIO_STATE_THROTTLED ((uint32_t)4U)
#define FIO_STATE_CLOSING   ((uint32_t)8U)

FIO_SFUNC void fio_s_init(fio_s *io) {
  *io = (fio_s){
      .pr = &FIO___MOCK_PROTOCOL,
      .node = FIO_LIST_INIT(io->node),
      .stream = FIO_STREAM_INIT(io->stream),
      .env = FIO___SRV_ENV_SAFE_INIT,
      .active = fio___srvdata.tick,
      .state = FIO_STATE_OPEN,
      .fd = -1,
  };
  FIO_LIST_PUSH(&io->pr->reserved.ios, &io->node);
  FIO_LIST_REMOVE(&FIO___MOCK_PROTOCOL.reserved.protocols);
  FIO_LIST_PUSH(&fio___srvdata.protocols,
                &FIO___MOCK_PROTOCOL.reserved.protocols);
  fio_set_valid(io);
}

FIO_SFUNC void fio_s_destroy(fio_s *io) {
  fio_set_invalid(io);
  FIO_LIST_REMOVE(&io->node);
#ifdef DEBUG
  FIO_LOG_DDEBUG2("detaching and destroying %p (fd %d): %zu bytes total",
                  (void *)io,
                  io->fd,
                  io->total_sent);
#else
  FIO_LOG_DDEBUG2("detaching and destroying %p (fd %d)", (void *)io, io->fd);
#endif
  /* store info, as it might be freed if the protocol is freed. */
  if (FIO_LIST_IS_EMPTY(&io->pr->reserved.ios))
    FIO_LIST_REMOVE_RESET(&io->pr->reserved.protocols);
  /* call on_finish / free callbacks . */
  io->pr->io_functions.free(io->tls);
  io->pr->on_close(io->udata); /* may destroy protocol object! */
  fio___srv_env_safe_destroy(&io->env);
  fio_sock_close(io->fd);
  fio_stream_destroy(&io->stream);
  fio_poll_forget(&fio___srvdata.poll_data, io->fd);
}
#define FIO_REF_NAME            fio
#define FIO_REF_INIT(o)         fio_s_init(&(o))
#define FIO_REF_DESTROY(o)      fio_s_destroy(&(o))
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

static void fio___protocol_set_task(void *io_, void *old_) {
  fio_s *io = (fio_s *)io_;
  fio_protocol_s *old = (fio_protocol_s *)old_;
  FIO_LIST_REMOVE(&io->node);
  if (FIO_LIST_IS_EMPTY(&old->reserved.ios))
    FIO_LIST_REMOVE_RESET(&old->reserved.protocols);
  FIO_LIST_PUSH(&io->pr->reserved.ios, &io->node);
  if (io->node.next == io->node.prev) /* list was empty before IO was added */
    FIO_LIST_PUSH(&fio___srvdata.protocols, &io->pr->reserved.protocols);
  fio_poll_monitor(&fio___srvdata.poll_data,
                   io->fd,
                   (void *)io,
                   POLLIN | POLLOUT);
  io->pr->on_attach(io);
  if (old == &FIO___MOCK_PROTOCOL) /* avoid calling `start` more than once */
    io->pr->io_functions.start(io);
}

/** Sets a new protocol object, returning the old protocol. */
SFUNC fio_protocol_s *fio_protocol_set(fio_s *io, fio_protocol_s *pr) {
  if (!pr)
    pr = &FIO___MOCK_PROTOCOL;
  fio___srv_init_protocol_test(pr);
  fio_protocol_s *old = io->pr;
  if (pr == old)
    return NULL;
  io->pr = pr;
  // fio_queue_push(fio___srv_tasks, fio___protocol_set_task, io, old);
  fio___protocol_set_task((void *)io, (void *)old);
  return old;
}

/** Returns a pointer to the current protocol object. */
SFUNC fio_protocol_s *fio_protocol_get(fio_s *io) { return io->pr; }

/* Attaches the socket in `fd` to the facio.io engine (reactor). */
SFUNC fio_s *fio_attach_fd(int fd,
                           fio_protocol_s *protocol,
                           void *udata,
                           void *tls) {
  fio_s *io = NULL;
  fio_protocol_s *old = NULL;
  if (!protocol)
    protocol = &FIO___MOCK_PROTOCOL;
  fio___srv_init_protocol_test(protocol);
  if (fd == -1)
    goto error;
  io = fio_new2();
  FIO_ASSERT_ALLOC(io);
  FIO_LOG_DDEBUG2("attaching fd %d to IO object %p", fd, (void *)io);
  fio_sock_set_non_block(fd);
  old = io->pr;
  io->fd = fd;
  io->pr = protocol;
  io->udata = udata;
  io->tls = tls;
  fio_queue_push(fio___srv_tasks, fio___protocol_set_task, io, old);
  return io;
error:
  protocol->on_close(udata);
  protocol->io_functions.free(tls);
  return NULL;
}

/**
 * Increases a IO's reference count, so it won't be automatically destroyed
 * when all tasks have completed.
 */
SFUNC fio_s *fio_dup(fio_s *io) { return fio_dup2(io); }

static void fio_undup_task(void *io, void *ignr_) {
  (void)ignr_;
  fio_free2((fio_s *)io);
}
/**
 * Decreases a IO's reference count, so it could be automatically destroyed
 * when all other tasks have completed.
 */
SFUNC void fio_undup(fio_s *io) {
  fio_queue_push(fio___srv_tasks, fio_undup_task, io);
}

/** Performs a task for each IO in the stated protocol. */
FIO_SFUNC size_t fio_protocol_each(fio_protocol_s *protocol,
                                   void (*task)(fio_s *, void *),
                                   void *udata) {
  size_t count = 0;
  if (!protocol || !protocol->reserved.ios.next || !protocol->reserved.ios.prev)
    return count;
  FIO_LIST_EACH(fio_s, node, &protocol->reserved.ios, io) {
    if (!(io->state & FIO_STATE_OPEN))
      continue;
    task(io, udata);
    ++count;
  }
  return count;
}

/* *****************************************************************************
Connection Object Links / Environment
***************************************************************************** */

void fio_env_set___(void); /* IDE marker */
/**
 * Links an object to a connection's lifetime / environment.
 */
SFUNC void fio_env_set FIO_NOOP(fio_s *io, fio_env_set_args_s args) {
  fio___srv_env_obj_s val = {
      .udata = args.udata,
      .on_close = args.on_close,
  };
  fio___srv_env_safe_set((io ? &io->env : &fio___srvdata.env),
                         args.name.buf,
                         args.name.len,
                         args.type,
                         val,
                         args.const_name);
}

void fio_env_unset___(void); /* IDE marker */
/**
 * Un-links an object from the connection's lifetime, so it's `on_close`
 * callback will NOT be called.
 */
SFUNC int fio_env_unset FIO_NOOP(fio_s *io, fio_env_unset_args_s args) {
  return fio___srv_env_safe_unset((io ? &io->env : &fio___srvdata.env),
                                  args.name.buf,
                                  args.name.len,
                                  args.type);
}

/**
 * Removes an object from the connection's lifetime / environment, calling it's
 * `on_close` callback as if the connection was closed.
 */
SFUNC int fio_env_remove FIO_NOOP(fio_s *io, fio_env_unset_args_s args) {
  return fio___srv_env_safe_remove((io ? &io->env : &fio___srvdata.env),
                                   args.name.buf,
                                   args.name.len,
                                   args.type);
}

/* *****************************************************************************
Event handling
***************************************************************************** */

static void fio___srv_poll_on_data(void *io_, void *ignr_) {
  (void)ignr_;
  fio_s *io = (fio_s *)io_;
  if (io->state == FIO_STATE_OPEN) {
    /* this also tests for the suspended / throttled / closing flags */
    io->pr->on_data(io);
    if (io->state == FIO_STATE_OPEN) {
      fio_poll_monitor(&fio___srvdata.poll_data, io->fd, io, POLLIN);
    }
  } else if ((io->state & FIO_STATE_OPEN)) {
    fio_poll_monitor(&fio___srvdata.poll_data, io->fd, io, POLLOUT);
  }
  fio_free2(io);
  return;
}

static void fio___srv_poll_on_ready(void *io_, void *ignr_) {
  (void)ignr_;
#if DEBUG
  errno = 0;
#endif
  fio_s *io = (fio_s *)io_;
  char buf_mem[FIO_SRV_BUFFER_PER_WRITE];
  size_t total = 0;
  if (!(io->state & FIO_STATE_OPEN))
    goto finish;
  for (;;) {
    size_t len = FIO_SRV_BUFFER_PER_WRITE;
    char *buf = buf_mem;
    fio_stream_read(&io->stream, &buf, &len);
    if (!len)
      break;
    ssize_t r = io->pr->io_functions.write(io->fd, buf, len, io->tls);
    if (r > 0) {
      total += r;
      fio_stream_advance(&io->stream, r);
      continue;
    } else if ((r == -1) & ((errno == EWOULDBLOCK) | (errno == EAGAIN) |
                            (errno == EINTR))) {
      break;
    } else {
#if DEBUG
      if (fio_stream_any(&io->stream))
        FIO_LOG_DDEBUG2(
            "IO write failed (%d), disconnecting: %p (fd %d)\n\tError: %s",
            errno,
            (void *)io,
            io->fd,
            strerror(errno));
#endif
      fio_close_now(io);
      goto finish;
    }
  }
  if (total) {
    fio_touch(io);
#ifdef DEBUG
    io->total_sent += total;
#endif
  }
  if (!fio_stream_any(&io->stream) &&
      !io->pr->io_functions.flush(io->fd, io->tls)) {
    if ((io->state & FIO_STATE_CLOSING)) {
      fio_close_now(io);
    } else {
      if ((io->state & FIO_STATE_THROTTLED)) {
        fio_atomic_and(&io->state, ~FIO_STATE_THROTTLED);
        fio_poll_monitor(&fio___srvdata.poll_data, io->fd, io, POLLIN);
      }
      FIO_LOG_DDEBUG2("calling on_ready for %p (fd %d)", (void *)io, io->fd);
      io->pr->on_ready(io);
    }
  } else {
    if (fio_stream_length(&io->stream) >= FIO_SRV_THROTTLE_LIMIT) {
      if (!(io->state & FIO_STATE_THROTTLED))
        FIO_LOG_DDEBUG2("throttled IO %p (fd %d)", (void *)io, io->fd);
      fio_atomic_or(&io->state, FIO_STATE_THROTTLED);
    }
    fio_poll_monitor(&fio___srvdata.poll_data, io->fd, io, POLLOUT);
  }
finish:
  fio_free2(io);
}

static void fio___srv_poll_on_close(void *io_, void *ignr_) {
  (void)ignr_;
  fio_s *io = (fio_s *)io_;
  fio_close_now(io);
  fio_free2(io);
}

static void fio___srv_poll_on_timeout(void *io_, void *ignr_) {
  (void)ignr_;
  fio_s *io = (fio_s *)io_;
  io->pr->on_timeout(io);
  fio_free2(io);
}

/* *****************************************************************************
Event scheduling
***************************************************************************** */

static void fio___srv_poll_on_data_schd(void *io) {
  if (!fio_is_valid(io))
    return;
  fio_queue_push(fio___srv_tasks,
                 fio___srv_poll_on_data,
                 fio_dup2((fio_s *)io));
}
static void fio___srv_poll_on_ready_schd(void *io) {
  if (!fio_is_valid(io))
    return;
  fio_queue_push(fio___srv_tasks,
                 fio___srv_poll_on_ready,
                 fio_dup2((fio_s *)io));
}
static void fio___srv_poll_on_close_schd(void *io) {
  if (!fio_is_valid(io))
    return;
  fio_queue_push(fio___srv_tasks,
                 fio___srv_poll_on_close,
                 fio_dup2((fio_s *)io));
}

/* *****************************************************************************
Timeout Review
***************************************************************************** */

/** Schedules the timeout event for any timed out IO object */
static int fio___srv_review_timeouts(void) {
  int c = 0;
  static time_t last_to_review = 0;
  /* test timeouts at whole second intervals */
  if (last_to_review + 1000 > fio___srvdata.tick)
    return c;
  last_to_review = fio___srvdata.tick;
  const int64_t now_milli = fio___srvdata.tick;

  FIO_LIST_EACH(fio_protocol_s,
                reserved.protocols,
                &fio___srvdata.protocols,
                pr) {
    FIO_ASSERT_DEBUG(pr->reserved.flags, "protocol object flags unmarked?!");
    if (!pr->timeout || pr->timeout > FIO_SRV_TIMEOUT_MAX)
      pr->timeout = FIO_SRV_TIMEOUT_MAX;
    int64_t limit = now_milli - ((int64_t)pr->timeout);
    FIO_LIST_EACH(fio_s, node, &pr->reserved.ios, io) {
      FIO_ASSERT_DEBUG(io->pr == pr, "IO protocol ownership error");
      if (io->active >= limit)
        break;
      FIO_LOG_DDEBUG2("scheduling timeout for %p (fd %d)", (void *)io, io->fd);
      fio_queue_push(fio___srv_tasks, fio___srv_poll_on_timeout, fio_dup2(io));
      ++c;
    }
  }
  return c;
}

/* *****************************************************************************
Reactor cycling
***************************************************************************** */
static void fio___srv_signal_handle(int sig, void *flg) {
  ((uint8_t *)flg)[0] = 1;
  (void)sig;
}

FIO_SFUNC void fio___srv_tick(int timeout) {
  static size_t performed_idle = 0;
  if (fio_poll_review(&fio___srvdata.poll_data, timeout) > 0) {
    performed_idle = 0;
  } else if (timeout) {
    if (!performed_idle)
      fio_state_callback_force(FIO_CALL_ON_IDLE);
    performed_idle = 1;
  }
  fio___srvdata.tick = fio_time_milli();
  fio_timer_push2queue(fio___srv_tasks, fio___srv_timer, fio___srvdata.tick);
  fio_queue_perform_all(fio___srv_tasks);
  if (fio___srv_review_timeouts())
    fio_queue_perform_all(fio___srv_tasks);
  fio_signal_review();
}

FIO_SFUNC void fio___srv_run_async_as_sync(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  unsigned repeat = 0;
  FIO_LIST_EACH(fio_srv_async_s, node, &fio___srvdata.async, pos) {
    fio_queue_task_s t = fio_queue_pop(&pos->queue);
    if (!t.fn)
      continue;
    t.fn(t.udata1, t.udata2);
    repeat = 1;
  }
  if (repeat)
    fio_queue_push(fio___srv_tasks, fio___srv_run_async_as_sync);
}

FIO_SFUNC void fio___srv_shutdown_task(void *shutdown_start_, void *a2) {
  intptr_t shutdown_start = (intptr_t)shutdown_start_;
  if (shutdown_start + FIO_SRV_SHUTDOWN_TIMEOUT < fio___srvdata.tick ||
      FIO_LIST_IS_EMPTY(&fio___srvdata.protocols))
    return;
  fio___srv_tick(fio_queue_count(fio___srv_tasks) ? 0 : 100);
  fio_queue_push(fio___srv_tasks, fio___srv_run_async_as_sync);
  fio_queue_push(fio___srv_tasks, fio___srv_shutdown_task, shutdown_start_, a2);
}

FIO_SFUNC void fio___srv_shutdown(void) {
  /* collect tick for shutdown start, to monitor for possible timeout */
  int64_t shutdown_start = fio___srvdata.tick = fio_time_milli();
  size_t connected = 0;
  /* first notify that shutdown is starting */
  fio_state_callback_force(FIO_CALL_ON_SHUTDOWN);
  /* preform on_shutdown callback for each connection and close */
  FIO_LIST_EACH(fio_protocol_s,
                reserved.protocols,
                &fio___srvdata.protocols,
                pr) {
    FIO_LIST_EACH(fio_s, node, &pr->reserved.ios, io) {
      pr->on_shutdown(io); /* TODO / FIX: move callback to task? */
      fio_close(io);       /* TODO / FIX: skip close on return value? */
      ++connected;
    }
  }
  FIO_LOG_DEBUG2("Server shutting down with %zu connected clients", connected);
  /* cycle while connections exist. */
  fio_queue_push(fio___srv_tasks,
                 fio___srv_shutdown_task,
                 (void *)(intptr_t)shutdown_start,
                 NULL);
  fio_queue_perform_all(fio___srv_tasks);
  /* in case of timeout, force close remaining connections. */
  connected = 0;
  FIO_LIST_EACH(fio_protocol_s,
                reserved.protocols,
                &fio___srvdata.protocols,
                pr) {
    FIO_LIST_EACH(fio_s, node, &pr->reserved.ios, io) {
      fio_close_now(io);
      ++connected;
    }
  }
  FIO_LOG_DEBUG("Server shutdown timed out with %zu clients", connected);
  /* perform remaining tasks. */
  fio_queue_perform_all(fio___srv_tasks);
}

FIO_SFUNC void fio___srv_work_task(void *ignr_1, void *ignr_2) {
  if (fio___srvdata.stop)
    return;
  fio___srv_tick(fio_queue_count(fio___srv_tasks) ? 0 : 500);
  fio_queue_push(fio___srv_tasks, fio___srv_work_task, ignr_1, ignr_2);
}

FIO_SFUNC void fio___srv_work(int is_worker) {
  fio___srvdata.is_worker = is_worker;
  fio_queue_perform_all(fio___srv_tasks);
  if (is_worker) {
    fio_state_callback_force(FIO_CALL_ON_START);
  }
  fio___srv_wakeup_init();
  fio_queue_push(fio___srv_tasks, fio___srv_work_task);
  fio_queue_perform_all(fio___srv_tasks);
  fio___srv_shutdown();
  fio_state_callback_force(FIO_CALL_ON_FINISH);
  fio_queue_perform_all(fio___srv_tasks);
  fio___srvdata.workers = 0;
}

/* *****************************************************************************
Worker Forking
***************************************************************************** */
static void fio___srv_spawn_worker(void *ignr_1, void *ignr_2);

static void fio___srv_wait_for_worker(void *thr_) {
  fio_thread_t t = (fio_thread_t)thr_;
  fio_thread_join(&t);
}

/** Worker sentinel */
static void *fio___srv_worker_sentinel(void *pid_data) {
  pid_t pid = (pid_t)(uintptr_t)pid_data;
  int status = 0;
  (void)status;
  fio_thread_t thr = fio_thread_current();
  fio_state_callback_add(FIO_CALL_ON_FINISH,
                         fio___srv_wait_for_worker,
                         (void *)thr);
  if (waitpid(pid, &status, 0) != pid && !fio___srvdata.stop)
    FIO_LOG_ERROR("waitpid failed, worker re-spawning might fail.");
  if (!WIFEXITED(status) || WEXITSTATUS(status)) {
    FIO_LOG_WARNING("abnormal worker exit detected");
    fio_state_callback_force(FIO_CALL_ON_CHILD_CRUSH);
  }
  if (!fio___srvdata.stop) {
    FIO_ASSERT_DEBUG(
        0,
        "DEBUG mode prevents worker re-spawning, now crashing parent.");
    fio_state_callback_remove(FIO_CALL_ON_FINISH,
                              fio___srv_wait_for_worker,
                              (void *)thr);
    fio_thread_detach(&thr);
    fio_queue_push(fio___srv_tasks, fio___srv_spawn_worker, (void *)thr);
  }
  return NULL;
}

static void fio___srv_spawn_worker(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  fio_thread_t t;

  if (fio___srvdata.root_pid != fio___srvdata.pid)
    return;
  if (fio_atomic_or_fetch(&fio___srvdata.stop, 2) != 2)
    return;

  fio_state_callback_force(FIO_CALL_BEFORE_FORK);
  /* do not allow master tasks to run in worker */
  fio_queue_perform_all(fio___srv_tasks);
  /* perform actual fork */
  pid_t pid = fio_thread_fork();
  FIO_ASSERT(pid != (pid_t)-1, "system call `fork` failed.");
  if (!pid)
    goto is_worker_process;
  fio_state_callback_force(FIO_CALL_AFTER_FORK);
  fio_state_callback_force(FIO_CALL_IN_MASTER);
  if (fio_thread_create(&t,
                        fio___srv_worker_sentinel,
                        (void *)(uintptr_t)pid)) {
    FIO_LOG_FATAL(
        "sentinel thread creation failed, no worker will be spawned.");
    fio_srv_stop();
  }
  if (!fio_atomic_xor_fetch(&fio___srvdata.stop, 2))
    fio_queue_push(fio___srv_tasks, fio___srv_work_task);
  return;

is_worker_process:
  fio___srvdata.pid = getpid();
  fio___srvdata.is_worker = 1;
  FIO_LOG_INFO("(%d) worker starting up.", (int)fio___srvdata.pid);
  fio_state_callback_force(FIO_CALL_AFTER_FORK);
  fio_state_callback_force(FIO_CALL_IN_CHILD);
  if (!fio_atomic_xor_fetch(&fio___srvdata.stop, 2))
    fio___srv_work(1);
  FIO_LOG_INFO("(%d) worker exiting.", (int)fio___srvdata.pid);
  exit(0);
}

/* *****************************************************************************
Starting the Server
***************************************************************************** */

/* Stopping the server. */
SFUNC void fio_srv_stop(void) { fio_atomic_or(&fio___srvdata.stop, 1); }

/* Returns true if server running and 0 if server stopped or shutting down. */
SFUNC int fio_srv_is_running(void) { return !fio___srvdata.stop; }

/* Returns true if the current process is the server's master process. */
SFUNC int fio_srv_is_master(void) {
  return fio___srvdata.root_pid == fio___srvdata.pid;
}

/* Returns true if the current process is a server's worker process. */
SFUNC int fio_srv_is_worker(void) { return fio___srvdata.is_worker; }

/* Returns the number or workers the server will actually run. */
SFUNC uint16_t fio_srv_workers(int workers) {
  if (workers < 0) {
    int cores = -1;
#ifdef _SC_NPROCESSORS_ONLN
    cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif /* _SC_NPROCESSORS_ONLN */
    if (cores == -1) {
      cores = 8;
      FIO_LOG_WARNING("fio_srv_start called with negative value for worker "
                      "count, but auto-detect failed, assuming %d CPU cores",
                      cores);
    }
    workers = cores / (0 - workers);
    workers += !workers;
  }
  return workers;
}

/* Starts the server, using optional `workers` processes. This will BLOCK! */
SFUNC void fio_srv_start(int workers) {
  fio___srvdata.stop = 0;
  fio___srvdata.workers = fio_srv_workers(workers);
  workers = (int)fio___srvdata.workers;
  fio___srvdata.is_worker = !workers;
  fio_sock_maximize_limits(0);
  fio_state_callback_force(FIO_CALL_PRE_START);
  fio_queue_perform_all(fio___srv_tasks);
  fio_signal_monitor(SIGINT,
                     fio___srv_signal_handle,
                     (void *)&fio___srvdata.stop);
  fio_signal_monitor(SIGTERM,
                     fio___srv_signal_handle,
                     (void *)&fio___srvdata.stop);
#ifdef SIGPIPE
  fio_signal_monitor(SIGPIPE, NULL, NULL);
#endif
  fio___srvdata.tick = fio_time_milli();
  if (workers) {
    FIO_LOG_INFO("(%d) spawning %d workers.", fio___srvdata.root_pid, workers);
    for (int i = 0; i < workers; ++i) {
      fio___srv_spawn_worker(NULL, NULL);
    }
  } else {
    FIO_LOG_DEBUG2("(%d) starting facil.io server in single process mode.",
                   fio___srvdata.root_pid);
  }
  fio___srv_work(!workers);
  fio_signal_forget(SIGINT);
  fio_signal_forget(SIGTERM);
#ifdef SIGPIPE
  fio_signal_forget(SIGPIPE);
#endif
  fio_queue_perform_all(fio___srv_tasks);
}

/* *****************************************************************************
IO API
***************************************************************************** */

/** Returns the socket file descriptor (fd) associated with the IO. */
SFUNC int fio_fd_get(fio_s *io) { return io->fd; }

FIO_SFUNC void fio_touch___task(void *io_, void *ignr_) {
  (void)ignr_;
  fio_s *io = (fio_s *)io_;
  io->active = fio___srvdata.tick;
  FIO_LIST_REMOVE(&io->node);
  FIO_LIST_PUSH(&io->pr->reserved.ios, &io->node);
  fio_free2(io);
}

/* Resets a socket's timeout counter. */
SFUNC void fio_touch(fio_s *io) {
  fio_queue_push_urgent(fio___srv_tasks, fio_touch___task, fio_dup(io));
}

/**
 * Reads data to the buffer, if any data exists. Returns the number of bytes
 * read.
 *
 * NOTE: zero (`0`) is a valid return value meaning no data was available.
 */
SFUNC size_t fio_read(fio_s *io, void *buf, size_t len) {
  ssize_t r = io->pr->io_functions.read(io->fd, buf, len, io->tls);
  if (r > 0) {
    fio_touch(io);
    return r;
  }
  if ((!len) | ((r == -1) & ((errno == EAGAIN) | (errno == EWOULDBLOCK) |
                             (errno == EINTR))))
    return 0;
  fio_close(io);
  return 0;
}

FIO_SFUNC void fio_write2___task(void *io_, void *packet_) {
  fio_s *io = (fio_s *)io_;
  fio_stream_packet_s *packet = (fio_stream_packet_s *)packet_;
  if (!(io->state & FIO_STATE_OPEN))
    goto io_error;
  fio_stream_add(&io->stream, packet);
  fio_queue_push(fio___srv_tasks,
                 fio___srv_poll_on_ready,
                 io); /* no dup/undup, already done.*/
  return;
io_error:
  fio_stream_pack_free(packet);
  fio_free2(io); /* undup the IO object since it isn't moved to on_ready */
}

void fio_write2___(void); /* IDE marker*/
/**
 * Writes data to the outgoing buffer and schedules the buffer to be sent.
 */
SFUNC void fio_write2 FIO_NOOP(fio_s *io, fio_write_args_s args) {
  fio_stream_packet_s *packet = NULL;
  if (!io)
    goto io_error_null;
  if (args.buf) {
    packet = fio_stream_pack_data(args.buf,
                                  args.len,
                                  args.offset,
                                  args.copy,
                                  args.dealloc);
  } else if (args.fd != -1) {
    packet = fio_stream_pack_fd(args.fd, args.len, args.offset, args.copy);
  }
  if (!packet)
    goto error;
  if (io && (io->state & FIO_STATE_CLOSING))
    goto write_called_after_close;
  fio_defer(fio_write2___task, fio_dup2(io), packet);
  return;
error: /* note: `dealloc` is called by the `fio_stream` API error handler. */
  FIO_LOG_ERROR("couldn't create %zu bytes long user-packet for IO %p (%d)",
                args.len,
                (void *)io,
                (io ? io->fd : -1));
  return;
write_called_after_close:
  FIO_LOG_WARNING("`write` called after `close` was called for IO.");
  fio_stream_pack_free(packet);
  return;
io_error_null:
  FIO_LOG_ERROR("(%d) `fio_write2` called for invalid IO (NULL)",
                fio___srvdata.pid);
  if (args.dealloc)
    args.dealloc(args.buf);
}

/** Marks the IO for closure as soon as scheduled data was sent. */
SFUNC void fio_close(fio_s *io) {
  if (io && (io->state & FIO_STATE_OPEN) &&
      !(fio_atomic_or(&io->state, FIO_STATE_CLOSING) & FIO_STATE_CLOSING)) {
    FIO_LOG_DDEBUG2("scheduling IO %p (fd %d) for closure", (void *)io, io->fd);
    fio_queue_push(fio___srv_tasks,
                   fio___srv_poll_on_ready,
                   fio_dup2((fio_s *)io));
  }
}

/** Marks the IO for immediate closure. */
SFUNC void fio_close_now(fio_s *io) {
  fio_atomic_or(&io->state, FIO_STATE_CLOSING);
  if ((fio_atomic_and(&io->state, ~FIO_STATE_OPEN) & FIO_STATE_OPEN))
    fio_free2(io);
}

/** Suspends future "on_data" events for the IO. */
SFUNC void fio_suspend(fio_s *io) { io->state |= FIO_STATE_SUSPENDED; }

/** Listens for future "on_data" events related to the IO. */
SFUNC void fio_unsuspend(fio_s *io) {
  if ((fio_atomic_and(&io->state, ~FIO_STATE_SUSPENDED) &
       FIO_STATE_SUSPENDED)) {
    fio_poll_monitor(&fio___srvdata.poll_data, io->fd, (void *)io, POLLIN);
  }
}

/** Returns 1 if the IO handle was suspended. */
SFUNC int fio_is_suspended(fio_s *io) {
  return (io->state & FIO_STATE_SUSPENDED);
}

/** Returns 1 if the IO handle is marked as open. */
SFUNC int fio_is_open(fio_s *io) {
  return (io->state & FIO_STATE_OPEN) && !(io->state & FIO_STATE_CLOSING);
}

/* *****************************************************************************
Listening
***************************************************************************** */

static void fio___srv_listen_on_data_task(void *io_, void *ignr_) {
  (void)ignr_;
  fio_s *io = (fio_s *)io_;
  int fd;
  struct fio_listen_args *l = (struct fio_listen_args *)(io->udata);
  while ((fd = accept(fio_fd_get(io), NULL, NULL)) != -1) {
    l->on_open(fd, l->udata);
  }
  fio_free2(io);
}
static void fio___srv_listen_on_data_task_reschd(void *io_, void *ignr_) {
  fio_queue_push(fio___srv_tasks, fio___srv_listen_on_data_task, io_, ignr_);
}

static void fio___srv_listen_on_data(fio_s *io) {
  int fd;
  struct fio_listen_args *l = (struct fio_listen_args *)(io->udata);
  if (l->queue_for_accept) {
    fio_queue_push(l->queue_for_accept,
                   fio___srv_listen_on_data_task_reschd,
                   fio_dup2(io));
    return;
  }
  while ((fd = accept(fio_fd_get(io), NULL, NULL)) != -1) {
    l->on_open(fd, l->udata);
  }
}
static void fio___srv_listen_on_close(void *settings_) {
  struct fio_listen_args *l = (struct fio_listen_args *)settings_;
  if (((!l->on_root && fio_srv_is_worker()) ||
       (l->on_root && fio_srv_is_master()))) {
    if (l->hide_from_log)
      FIO_LOG_DEBUG2("(%d) stopped listening on %s", fio___srvdata.pid, l->url);
    else
      FIO_LOG_INFO("(%d) stopped listening on %s", fio___srvdata.pid, l->url);
  }
}

FIO_SFUNC void fio___srv_listen_cleanup_task(void *udata) {
  struct fio_listen_args *l = (struct fio_listen_args *)udata;
  int *pfd = (int *)(l + 1);
  if (l->on_finish)
    l->on_finish(l->udata);
  fio_sock_close(*pfd);
#ifdef AF_UNIX
  /* delete the unix socket file, if any. */
  fio_url_s u = fio_url_parse(l->url, FIO_STRLEN(l->url));
  if (fio_srv_is_master() && !u.host.buf && !u.port.buf && u.path.buf) {
    unlink(u.path.buf);
  }
#endif
  fio_state_callback_remove(FIO_CALL_AT_EXIT,
                            fio___srv_listen_cleanup_task,
                            udata);
  FIO_MEM_FREE_(l, sizeof(*l) + sizeof(int) + FIO_STRLEN(l->url) + 1);
}

static fio_protocol_s FIO___LISTEN_PROTOCOL = {
    .on_data = fio___srv_listen_on_data,
    .on_close = fio___srv_listen_on_close,
    .on_timeout = fio___srv_on_timeout_never,
};

FIO_SFUNC void fio___srv_listen_attach_task(void *udata) {
  struct fio_listen_args *l = (struct fio_listen_args *)udata;
  int *pfd = (int *)(l + 1);
  int fd = fio_sock_dup(*pfd);
  FIO_ASSERT(fd != -1, "listening socket failed to `dup`");
  FIO_LOG_DEBUG2("(%d) Called dup(%d) to attach %d as a listening socket.",
                 (int)fio___srvdata.pid,
                 *pfd,
                 fd);
  fio_attach_fd(fd, &FIO___LISTEN_PROTOCOL, l, NULL);
  if (l->on_start)
    l->on_start(l->udata);
  if (l->hide_from_log)
    FIO_LOG_DEBUG2("(%d) started listening on %s", fio___srvdata.pid, l->url);
  else
    FIO_LOG_INFO("(%d) started listening on %s", fio___srvdata.pid, l->url);
}

FIO_SFUNC void fio___srv_listen_attach_task_deferred(void *udata, void *ignr_) {
  (void)ignr_;
  fio___srv_listen_attach_task(udata);
}

void fio_listen___(void); /* IDE Marker */
SFUNC int fio_listen FIO_NOOP(struct fio_listen_args args) {
  static int64_t port = 0;
  size_t len = args.url ? FIO_STRLEN(args.url) + 1 : 0;
  struct fio_listen_args *cpy = NULL;
  fio_str_info_s adr, tmp;
  int *fd_store;
  int fd;
  if (!args.on_open) {
    FIO_LOG_ERROR("fio_listen missing `on_open` callback.");
    goto other_error;
  }
  len += (!len) << 6;
  cpy = (struct fio_listen_args *)
      FIO_MEM_REALLOC_(NULL, 0, (sizeof(*cpy) + sizeof(int) + len), 0);
  FIO_ASSERT_ALLOC(cpy);
  *cpy = args;
  cpy->url = (char *)(cpy + 1) + sizeof(int);
  fd_store = (int *)(cpy + 1);
  if (args.url) {
    FIO_MEMCPY((void *)(cpy->url), args.url, len);
  } else {
    if (!port) {
      char *port_env = getenv("PORT");
      if (port_env)
        port = fio_atol10(&port_env);
      if (!port | ((uint64_t)port > 65535))
        port = 3000;
    }
    tmp = FIO_STR_INFO3((char *)cpy->url, 0, len);
    if (!(adr.buf = getenv("ADDRESS")) ||
        (adr.len = FIO_STRLEN(adr.buf)) > 58) {
      adr = FIO_STR_INFO2((char *)"0.0.0.0:", 8);
    }
    fio_string_write2(&tmp,
                      NULL,
                      FIO_STRING_WRITE_STR2(adr.buf, adr.len),
                      FIO_STRING_WRITE_UNUM(port));
    ++port;
  }
  fd = fio_sock_open2(cpy->url, FIO_SOCK_SERVER | FIO_SOCK_TCP);
  if (fd == -1)
    goto fd_error;
  *fd_store = fd;
  if (fio_srv_is_running()) {
    fio_defer(fio___srv_listen_attach_task_deferred, cpy, NULL);
  } else {
    fio_state_callback_add(
        (args.on_root ? FIO_CALL_PRE_START : FIO_CALL_ON_START),
        fio___srv_listen_attach_task,
        (void *)cpy);
  }
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___srv_listen_cleanup_task, cpy);
  return 0;
fd_error:
  FIO_MEM_FREE_(cpy, (sizeof(*cpy) + len));
other_error:
  if (args.on_finish)
    args.on_finish(args.udata);
  return -1;
}

/* *****************************************************************************
Managing data after a fork
***************************************************************************** */
FIO_SFUNC void fio___srv_after_fork(void *ignr_) {
  (void)ignr_;
  fio___srvdata.pid = getpid();
  fio_queue_perform_all(fio___srv_tasks);
  FIO_LIST_EACH(fio_protocol_s,
                reserved.protocols,
                &fio___srvdata.protocols,
                pr) {
    FIO_LIST_EACH(fio_s, node, &pr->reserved.ios, io) { fio_close_now(io); }
  }
  fio_queue_perform_all(fio___srv_tasks);
  fio_invalidate_all();
  fio_queue_perform_all(fio___srv_tasks);
  fio_queue_destroy(fio___srv_tasks);
}

FIO_SFUNC void fio___srv_cleanup_at_exit(void *ignr_) {
  fio___srv_after_fork(ignr_);
  fio_poll_destroy(&fio___srvdata.poll_data);
  fio___srv_env_safe_destroy(&fio___srvdata.env);
}

/* *****************************************************************************
Initializing Server State
***************************************************************************** */
FIO_CONSTRUCTOR(fio___srv) {
  fio_queue_init(fio___srv_tasks);
  fio___srvdata.protocols = FIO_LIST_INIT(fio___srvdata.protocols);
  fio___srvdata.tick = fio_time_milli();
  fio___srvdata.root_pid = fio___srvdata.pid = getpid();
  fio___srvdata.async = FIO_LIST_INIT(fio___srvdata.async);
  fio_poll_init(&fio___srvdata.poll_data,
                .on_data = fio___srv_poll_on_data_schd,
                .on_ready = fio___srv_poll_on_ready_schd,
                .on_close = fio___srv_poll_on_close_schd);
  fio___srv_init_protocol_test(&FIO___MOCK_PROTOCOL);
  fio___srv_init_protocol_test(&FIO___LISTEN_PROTOCOL);
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___srv_after_fork, NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___srv_cleanup_at_exit, NULL);
}

/* *****************************************************************************
TLS Context Type and Helpers
***************************************************************************** */

typedef struct {
  fio_keystr_s nm;
  void (*fn)(fio_s *);
} fio___tls_alpn_s;

typedef struct {
  fio_keystr_s nm;
  fio_keystr_s public_cert_file;
  fio_keystr_s private_key_file;
  fio_keystr_s pk_password;
} fio___tls_cert_s;

typedef struct {
  fio_keystr_s nm;
} fio___tls_trust_s;

#undef FIO_TYPEDEF_IMAP_REALLOC
#undef FIO_TYPEDEF_IMAP_FREE
#undef FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE
#define FIO_TYPEDEF_IMAP_REALLOC(p, size_old, size, copy) realloc(p, size)
#define FIO_TYPEDEF_IMAP_FREE(ptr, len)                   free(ptr)
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE                  0

#define FIO___ALPN_HASH(o)   ((uint16_t)fio_keystr_hash(o->nm))
#define FIO___ALPN_CMP(a, b) fio_keystr_is_eq(a->nm, b->nm)
#define FIO___ALPN_VALID(o)  fio_keystr_buf(&o->nm).len

FIO_TYPEDEF_IMAP_ARRAY(fio___tls_alpn_map,
                       fio___tls_alpn_s,
                       uint16_t,
                       FIO___ALPN_HASH,
                       FIO___ALPN_CMP,
                       FIO___ALPN_VALID)
FIO_TYPEDEF_IMAP_ARRAY(fio___tls_trust_map,
                       fio___tls_trust_s,
                       uint16_t,
                       FIO___ALPN_HASH,
                       FIO___ALPN_CMP,
                       FIO___ALPN_VALID)
FIO_TYPEDEF_IMAP_ARRAY(fio___tls_cert_map,
                       fio___tls_cert_s,
                       uint16_t,
                       FIO___ALPN_HASH,
                       FIO___ALPN_CMP,
                       FIO_IMAP_ALWAYS_VALID)

#undef FIO___ALPN_HASH
#undef FIO___ALPN_CMP
#undef FIO___ALPN_VALID
#undef FIO_TYPEDEF_IMAP_REALLOC
#undef FIO_TYPEDEF_IMAP_FREE
#undef FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE
#define FIO_TYPEDEF_IMAP_REALLOC         FIO_MEM_REALLOC
#define FIO_TYPEDEF_IMAP_FREE            FIO_MEM_FREE
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE FIO_MEM_REALLOC_IS_SAFE

struct fio_tls_s {
  fio___tls_cert_map_s cert;
  fio___tls_alpn_map_s alpn;
  fio___tls_trust_map_s trust;
  uint8_t trust_sys; /** Set to 1 if system certificate registry is trusted */
};

#define FIO___RECURSIVE_INCLUDE 1
#define FIO_REF_NAME            fio_tls
#define FIO_REF_DESTROY(tls)                                                   \
  do {                                                                         \
    FIO_IMAP_EACH(fio___tls_alpn_map, &tls.alpn, i) {                          \
      fio_keystr_destroy(&tls.alpn.ary[i].nm, FIO_STRING_FREE_KEY);            \
    }                                                                          \
    FIO_IMAP_EACH(fio___tls_trust_map, &tls.trust, i) {                        \
      fio_keystr_destroy(&tls.trust.ary[i].nm, FIO_STRING_FREE_KEY);           \
    }                                                                          \
    FIO_IMAP_EACH(fio___tls_cert_map, &tls.cert, i) {                          \
      fio_keystr_destroy(&tls.cert.ary[i].nm, FIO_STRING_FREE_KEY);            \
      fio_keystr_destroy(&tls.cert.ary[i].public_cert_file,                    \
                         FIO_STRING_FREE_KEY);                                 \
      fio_keystr_destroy(&tls.cert.ary[i].private_key_file,                    \
                         FIO_STRING_FREE_KEY);                                 \
      fio_keystr_destroy(&tls.cert.ary[i].pk_password, FIO_STRING_FREE_KEY);   \
    }                                                                          \
    fio___tls_alpn_map_destroy(&tls.alpn);                                     \
    fio___tls_trust_map_destroy(&tls.trust);                                   \
    fio___tls_cert_map_destroy(&tls.cert);                                     \
  } while (0)
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/** Performs a `new` operation, returning a new `fio_tls_s` context. */
SFUNC fio_tls_s *fio_tls_new(void) {
  fio_tls_s *r = fio_tls_new2();
  FIO_ASSERT_ALLOC(r);
  *r = (fio_tls_s){.trust_sys = 0};
  return r;
}

/** Performs a `dup` operation, increasing the object's reference count. */
SFUNC fio_tls_s *fio_tls_dup(fio_tls_s *tls) { return fio_tls_dup2(tls); }

/** Performs a `free` operation, reducing the reference count and freeing. */
SFUNC void fio_tls_free(fio_tls_s *tls) {
  if (!tls)
    return;
  fio_tls_free2(tls);
}

/** Adds a certificate a new SSL/TLS context / settings object (SNI support). */
SFUNC fio_tls_s *fio_tls_cert_add(fio_tls_s *t,
                                  const char *server_name,
                                  const char *public_cert_file,
                                  const char *private_key_file,
                                  const char *pk_password) {
  if (!t)
    return t;
  fio___tls_cert_s o = {
      .nm = fio_keystr_copy(FIO_STR_INFO1((char *)server_name),
                            FIO_STRING_ALLOC_KEY),
      .public_cert_file =
          fio_keystr_copy(FIO_STR_INFO1((char *)public_cert_file),
                          FIO_STRING_ALLOC_KEY),
      .private_key_file =
          fio_keystr_copy(FIO_STR_INFO1((char *)private_key_file),
                          FIO_STRING_ALLOC_KEY),
      .pk_password = fio_keystr_copy(FIO_STR_INFO1((char *)pk_password),
                                     FIO_STRING_ALLOC_KEY),
  };
  fio___tls_cert_s *old = fio___tls_cert_map_get(&t->cert, o);
  if (old)
    goto replace_old;
  fio___tls_cert_map_set(&t->cert, o, 1);
  return t;
replace_old:
  fio_keystr_destroy(&old->nm, FIO_STRING_FREE_KEY);
  fio_keystr_destroy(&old->public_cert_file, FIO_STRING_FREE_KEY);
  fio_keystr_destroy(&old->private_key_file, FIO_STRING_FREE_KEY);
  fio_keystr_destroy(&old->pk_password, FIO_STRING_FREE_KEY);
  *old = o;
  return t;
}

/**
 * Adds an ALPN protocol callback to the SSL/TLS context.
 *
 * The first protocol added will act as the default protocol to be selected.
 *
 * Except for the `tls` and `protocol_name` arguments, all arguments can be
 * NULL.
 */
SFUNC fio_tls_s *fio_tls_alpn_add(fio_tls_s *t,
                                  const char *protocol_name,
                                  void (*on_selected)(fio_s *)) {
  if (!t || !protocol_name)
    return t;
  if (!on_selected)
    on_selected = fio___srv_on_ev_mock;
  size_t pr_name_len = strlen(protocol_name);
  if (pr_name_len > 255) {
    FIO_LOG_ERROR("fio_tls_alpn_add called with name longer than 255 chars!");
    return t;
  }
  fio___tls_alpn_s o = {
      .nm = fio_keystr_copy(FIO_STR_INFO2((char *)protocol_name, pr_name_len),
                            FIO_STRING_ALLOC_KEY),
      .fn = on_selected,
  };
  fio___tls_alpn_s *old = fio___tls_alpn_map_get(&t->alpn, o);
  if (old)
    goto replace_old;
  fio___tls_alpn_map_set(&t->alpn, o, 1);
  return t;
replace_old:
  fio_keystr_destroy(&old->nm, FIO_STRING_FREE_KEY);
  *old = o;
  return t;
}

/** Calls the `on_selected` callback for the `fio_tls_s` object. */
SFUNC int fio_tls_alpn_select(fio_tls_s *t,
                              const char *protocol_name,
                              size_t name_length,
                              fio_s *io) {
  if (!t || !protocol_name)
    return -1;
  fio___tls_alpn_s seeking = {
      .nm = fio_keystr(protocol_name, (uint32_t)name_length)};
  fio___tls_alpn_s *alpn = fio___tls_alpn_map_get(&t->alpn, seeking);
  if (!alpn) {
    FIO_LOG_DDEBUG2("TLS ALPN %.*s not found in %zu long list",
                    (int)name_length,
                    protocol_name,
                    t->alpn.count);
    return -1;
  }
  alpn->fn(io);
  return 0;
}

/**
 * Adds a certificate to the "trust" list, which automatically adds a peer
 * verification requirement.
 *
 * If `public_cert_file` is `NULL`, adds the system's default trust registry.
 *
 * Note: when the `fio_tls_s` object is used for server connections, this will
 * limit connections to clients that connect using a trusted certificate.
 *
 *      fio_tls_trust_add(tls, "google-ca.pem" );
 */
SFUNC fio_tls_s *fio_tls_trust_add(fio_tls_s *t, const char *public_cert_file) {
  if (!t)
    return t;
  if (!public_cert_file) {
    t->trust_sys = 1;
    return t;
  }
  fio___tls_trust_s o = {
      .nm = fio_keystr_copy(FIO_STR_INFO1((char *)public_cert_file),
                            FIO_STRING_ALLOC_KEY),
  };
  fio___tls_trust_s *old = fio___tls_trust_map_get(&t->trust, o);
  if (old)
    goto replace_old;
  fio___tls_trust_map_set(&t->trust, o, 1);
  return t;
replace_old:
  fio_keystr_destroy(&old->nm, FIO_STRING_FREE_KEY);
  *old = o;
  return t;
}

/**
 * Returns the number of `fio_tls_cert_add` instructions.
 *
 * This could be used when deciding if to add a NULL instruction (self-signed).
 *
 * If `fio_tls_cert_add` was never called, zero (0) is returned.
 */
SFUNC uintptr_t fio_tls_cert_count(fio_tls_s *tls) {
  return tls ? tls->cert.count : 0;
}

/**
 * Returns the number of registered ALPN protocol names.
 *
 * This could be used when deciding if protocol selection should be delegated to
 * the ALPN mechanism, or whether a protocol should be immediately assigned.
 *
 * If no ALPN protocols are registered, zero (0) is returned.
 */
SFUNC uintptr_t fio_tls_alpn_count(fio_tls_s *tls) {
  return tls ? tls->alpn.count : 0;
}

/**
 * Returns the number of `fio_tls_trust_add` instructions.
 *
 * This could be used when deciding if to disable peer verification or not.
 *
 * If `fio_tls_trust_add` was never called, zero (0) is returned.
 */
SFUNC uintptr_t fio_tls_trust_count(fio_tls_s *tls) {
  return tls ? tls->trust.count : 0;
}

/** Calls callbacks for certificate, trust certificate and ALPN added. */
void fio_tls_each___(void); /* IDE Marker*/
SFUNC int fio_tls_each FIO_NOOP(fio_tls_each_s a) {
  if (!a.tls)
    return -1;
  if (a.each_cert) {
    FIO_IMAP_EACH(fio___tls_cert_map, &a.tls->cert, i) {
      if (a.each_cert(&a,
                      fio_keystr_buf(&a.tls->cert.ary[i].nm).buf,
                      fio_keystr_buf(&a.tls->cert.ary[i].public_cert_file).buf,
                      fio_keystr_buf(&a.tls->cert.ary[i].private_key_file).buf,
                      fio_keystr_buf(&a.tls->cert.ary[i].pk_password).buf))
        return -1;
    }
  }
  if (a.each_alpn) {
    FIO_IMAP_EACH(fio___tls_alpn_map, &a.tls->alpn, i) {
      if (a.each_alpn(&a,
                      fio_keystr_buf(&a.tls->alpn.ary[i].nm).buf,
                      a.tls->alpn.ary[i].fn))
        return -1;
    }
  }
  if (a.each_trust) {
    if (a.tls->trust_sys && a.each_trust(&a, NULL))
      return -1;
    FIO_IMAP_EACH(fio___tls_trust_map, &a.tls->trust, i) {
      if (a.each_trust(&a, fio_keystr_buf(&a.tls->trust.ary[i].nm).buf))
        return -1;
    }
  }
  return 0;
}

/** If `NULL` returns current default, otherwise sets it. */
SFUNC fio_io_functions_s fio_tls_default_io_functions(fio_io_functions_s *f) {
  static fio_io_functions_s default_io_functions = {
      .build_context = fio___io_func_default_build_context,
      .start = fio___srv_on_ev_mock,
      .read = fio___io_func_default_read,
      .write = fio___io_func_default_write,
      .flush = fio___io_func_default_flush,
      .free = fio___srv_on_close_mock,
  };
  if (!f)
    return default_io_functions;
  if (!f->build_context)
    f->build_context = fio___io_func_default_build_context;
  if (!f->start)
    f->start = fio___srv_on_ev_mock;
  if (!f->read)
    f->read = fio___io_func_default_read;
  if (!f->write)
    f->write = fio___io_func_default_write;
  if (!f->flush)
    f->flush = fio___io_func_default_flush;
  if (!f->free)
    f->free = fio___srv_on_close_mock;
  default_io_functions = *f;
  return default_io_functions;
}

/* *****************************************************************************
Server Async - Worker Threads for non-IO tasks
***************************************************************************** */

FIO_SFUNC void fio___srv_async_start(void *q_) {
  fio_srv_async_s *q = (fio_srv_async_s *)q_;
  q->q = &q->queue;
  if (fio_queue_workers_add(&q->queue, (size_t)q->count))
    goto failed;
  return;

failed:
  FIO_LOG_ERROR("Server Async Queue couldn't spawn threads!");
  q->q = fio_srv_queue();
}
FIO_SFUNC void fio___srv_async_finish(void *q_) {
  fio_srv_async_s *q = (fio_srv_async_s *)q_;
  q->q = fio___srv_tasks;
  fio_queue_workers_stop(&q->queue);
  fio_queue_perform_all(&q->queue);
  fio_queue_destroy(&q->queue);
}

/** Initializes an async server queue for multo-threaded (non IO) tasks. */
SFUNC void fio_srv_async_init(fio_srv_async_s *q, uint32_t threads) {
  *q = (fio_srv_async_s){
      .queue = FIO_QUEUE_STATIC_INIT(q->queue),
      .count = threads,
      .q = fio_srv_queue(),
      .node = FIO_LIST_INIT(q->node),
  };
  if (!threads || threads > 4095)
    return;
  q->q = &q->queue;
  FIO_LIST_PUSH(&fio___srvdata.async, &q->node);
  fio_state_callback_add(FIO_CALL_ON_START, fio___srv_async_start, q);
  fio_state_callback_add(FIO_CALL_ON_SHUTDOWN, fio___srv_async_finish, q);
}

/* *****************************************************************************
Done with Server code
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_SERVER */
