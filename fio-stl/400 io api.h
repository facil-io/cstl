/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_IO                 /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




              IO Reactor - an Evented IO Reactor, Single-Threaded



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_IO) && !defined(FIO___RECURSIVE_INCLUDE) &&                    \
    !defined(H___FIO_IO___H)
#define H___FIO_IO___H
/* *****************************************************************************
IO Reactor Settings

At this point, define any MACROs and customizable settings available to the
developer.
***************************************************************************** */

#ifndef FIO_IO_BUFFER_PER_WRITE
/** Control the size of the on-stack buffer used for `write` events. */
#define FIO_IO_BUFFER_PER_WRITE 65536U
#endif

#ifndef FIO_IO_THROTTLE_LIMIT
/** IO will be throttled (no `on_data` events) if outgoing buffer is large. */
#define FIO_IO_THROTTLE_LIMIT 2097152U
#endif

#ifndef FIO_IO_TIMEOUT_MAX
/** Controls the maximum and default timeout in milliseconds (5 minutes). */
#define FIO_IO_TIMEOUT_MAX 300000
#endif

#ifndef FIO_IO_SHUTDOWN_TIMEOUT
/* Sets the hard timeout (in milliseconds) for the reactor's shutdown loop. */
#define FIO_IO_SHUTDOWN_TIMEOUT 15000
#endif

#ifndef FIO_IO_COUNT_STORAGE
#ifdef DEBUG
#define FIO_IO_COUNT_STORAGE 1
#else
#define FIO_IO_COUNT_STORAGE 0
#endif
#endif
/* *****************************************************************************
IO Types
***************************************************************************** */

/** The main protocol object type. See `struct fio_io_protocol_s`. */
typedef struct fio_io_protocol_s fio_io_protocol_s;

/** The IO functions used by the protocol object. */
typedef struct fio_io_functions_s fio_io_functions_s;

/** The main IO object type. Should be treated as an opaque pointer. */
typedef struct fio_io_s fio_io_s;

/** An opaque type used for the SSL/TLS helper functions. */
typedef struct fio_io_tls_s fio_io_tls_s;

/** Message structure, as received by the `on_message` subscription callback. */
typedef struct fio_msg_s fio_msg_s;

/** The IO Async Queue type. */
typedef struct fio_io_async_s fio_io_async_s;

/* *****************************************************************************
Starting / Stopping the IO Reactor
***************************************************************************** */

/** Stopping the IO reactor. */
SFUNC void fio_io_stop(void);

/** Adds `workers` amount of workers to the root IO reactor process. */
SFUNC void fio_io_add_workers(int workers);

/** Starts the IO reactor, using optional `workers` processes. Will BLOCK! */
SFUNC void fio_io_start(int workers);

/** Retiers all existing workers and restarts with the number of workers. */
SFUNC void fio_io_restart(int workers);

/** Sets a signal to listen to for a hot restart (see `fio_io_restart`). */
SFUNC void fio_io_restart_on_signal(int signal);

/** Returns the shutdown timeout for the reactor. */
SFUNC size_t fio_io_shutdown_timsout(void);

/** Sets the shutdown timeout for the reactor, returning the new value. */
SFUNC size_t fio_io_shutdown_timsout_set(size_t milliseconds);

/* *****************************************************************************
The IO Reactor's State
***************************************************************************** */

/** Returns true if IO reactor running and 0 if stopped or shutting down. */
SFUNC int fio_io_is_running(void);

/** Returns true if the current process is the IO reactor's master process. */
SFUNC int fio_io_is_master(void);

/** Returns true if the current process is an IO reactor's worker process. */
SFUNC int fio_io_is_worker(void);

/** Returns the number or workers the IO reactor will actually run. */
SFUNC uint16_t fio_io_workers(int workers_requested);

/** Returns current process id. */
SFUNC int fio_io_pid(void);

/** Returns the root / master process id. */
SFUNC int fio_io_root_pid(void);

/** Returns the last millisecond when the IO reactor polled for events. */
SFUNC int64_t fio_io_last_tick(void);

/* *****************************************************************************
Listening to Incoming Connections
***************************************************************************** */

/** Arguments for the fio_io_listen function */
typedef struct fio_io_listen_args {
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
   *     fio_io_listen(.url = "0.0.0.0:3000/?tls", ...);
   *     fio_io_listen(.url = "0.0.0.0:3000/?tls=./", ...);
   *     // same as:
   *     fio_io_listen(.url = "0.0.0.0:3000/"
   *                            "?key=./key.pem"
   *                            "&cert=./cert.pem", ...);
   */
  const char *url;
  /** The `fio_io_protocol_s` that will be assigned to incoming
   * connections. */
  fio_io_protocol_s *protocol;
  /** The default `udata` set for (new) incoming connections. */
  void *udata;
  /** TLS object used for incoming connections (ownership moved to listener). */
  fio_io_tls_s *tls;
  /**
   * Called when the a listening socket starts to listen.
   *
   * May be called multiple times (i.e., if the IO reactor stops and restarts).
   */
  void (*on_start)(fio_io_protocol_s *protocol, void *udata);
  /**
   * Called during listener cleanup.
   *
   * This will be called separately for every process before exiting.
   */
  void (*on_stop)(fio_io_protocol_s *protocol, void *udata);
  /**
   * Selects a queue that will be used to schedule a pre-accept task.
   * May be used to test user thread stress levels before accepting connections.
   */
  fio_io_async_s *queue_for_accept;
  /** When forking the IO reactor - limits `listen` to the root process. */
  uint8_t on_root;
  /** Hides "started/stopped listening" messages from log (if set). */
  uint8_t hide_from_log;
} fio_io_listen_args;

typedef struct fio_listener_s fio_listener_s;
/**
 * Sets up a network service on a listening socket.
 *
 * Returns a self-destructible listener handle on success or NULL on error.
 */
SFUNC fio_listener_s *fio_io_listen(fio_io_listen_args args);
#define fio_io_listen(...) fio_io_listen((fio_io_listen_args){__VA_ARGS__})

/** Notifies a listener to stop listening. */
SFUNC void fio_io_listen_stop(fio_listener_s *l);

/** Returns the listener's associated protocol. */
SFUNC fio_io_protocol_s *fio_io_listener_protocol(fio_listener_s *l);

/** Returns the listener's associated `udata`. */
SFUNC void *fio_io_listener_udata(fio_listener_s *l);

/** Sets the listener's associated `udata`, returning the old value. */
SFUNC void *fio_io_listener_udata_set(fio_listener_s *l, void *new_udata);

/** Returns the URL on which the listener is listening. */
SFUNC fio_buf_info_s fio_io_listener_url(fio_listener_s *l);

/** Returns true if the listener protocol has an attached TLS context. */
SFUNC int fio_io_listener_is_tls(fio_listener_s *l);

/* *****************************************************************************
Connecting as a Client
***************************************************************************** */

/** Named arguments for fio_io_connect */
typedef struct {
  /** The URL to connect to (may contain TLS hints in query / `tls` scheme). */
  const char *url;
  /** Connection protocol (once connection established). */
  fio_io_protocol_s *protocol;
  /** Called in case of a failed connection, use for cleanup. */
  void (*on_failed)(fio_io_protocol_s *protocol, void *udata);
  /** Opaque user data (set only once connection was established). */
  void *udata;
  /** TLS builder object for TLS connections. */
  fio_io_tls_s *tls;
  /** Connection timeout in milliseconds (defaults to 30 seconds). */
  uint32_t timeout;
} fio_io_connect_args_s;

/** Connects to a specific URL, returning the `fio_io_s` IO object or `NULL`. */
SFUNC fio_io_s *fio_io_connect(fio_io_connect_args_s args);

#define fio_io_connect(url_, ...)                                              \
  fio_io_connect((fio_io_connect_args_s){.url = url_, __VA_ARGS__})

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
 * Returns NULL on error. the `fio_io_s` pointer must NOT be used except
 * within proper callbacks.
 */
SFUNC fio_io_s *fio_io_attach_fd(int fd,
                                 fio_io_protocol_s *protocol,
                                 void *udata,
                                 void *tls);

/** Sets a new protocol object. `NULL` is a valid "only-write" protocol. */
SFUNC fio_io_protocol_s *fio_io_protocol_set(fio_io_s *io,
                                             fio_io_protocol_s *protocol);

/**
 * Returns a pointer to the current protocol object.
 *
 * If `protocol` wasn't properly set, the pointer might be NULL or invalid.
 *
 * If `protocol` wasn't attached yet, may return the previous protocol.
 */
IFUNC fio_io_protocol_s *fio_io_protocol(fio_io_s *io);

/** Returns the a pointer to the memory buffer required by the protocol. */
IFUNC void *fio_io_buffer(fio_io_s *io);

/** Returns the length of the `buf` buffer. */
IFUNC size_t fio_io_buffer_len(fio_io_s *io);

/** Associates a new `udata` pointer with the IO, returning the old `udata` */
IFUNC void *fio_io_udata_set(fio_io_s *io, void *udata);

/** Returns the `udata` pointer associated with the IO. */
IFUNC void *fio_io_udata(fio_io_s *io);

/** Associates a new `tls` pointer with the IO, returning the old `tls` */
IFUNC void *fio_io_tls_set(fio_io_s *io, void *tls);

/** Returns the `tls` pointer associated with the IO. */
IFUNC void *fio_io_tls(fio_io_s *io);

/** Returns the socket file descriptor (fd) associated with the IO. */
IFUNC int fio_io_fd(fio_io_s *io);

/** Resets a socket's timeout counter. */
SFUNC void fio_io_touch(fio_io_s *io);

/**
 * Reads data to the buffer, if any data exists. Returns the number of bytes
 * read.
 *
 * NOTE: zero (`0`) is a valid return value meaning no data was available.
 */
SFUNC size_t fio_io_read(fio_io_s *io, void *buf, size_t len);

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
} fio_io_write_args_s;

/**
 * Writes data to the outgoing buffer and schedules the buffer to be sent.
 */
SFUNC void fio_io_write2(fio_io_s *io, fio_io_write_args_s args);
#define fio_io_write2(io, ...)                                                 \
  fio_io_write2(io, (fio_io_write_args_s){__VA_ARGS__})

/** Helper macro for a common fio_io_write2 (copies the buffer). */
#define fio_io_write(io, buf_, len_)                                           \
  fio_io_write2(io, .buf = (buf_), .len = (len_), .copy = 1)

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
#define fio_io_sendfile(io, source_fd, offset_, bytes)                         \
  fio_io_write2((io),                                                          \
                .fd = (source_fd),                                             \
                .offset = (size_t)(offset_),                                   \
                .len = (bytes))

/** Marks the IO for closure as soon as scheduled data was sent. */
SFUNC void fio_io_close(fio_io_s *io);

/** Marks the IO for immediate closure. */
SFUNC void fio_io_close_now(fio_io_s *io);

/**
 * Increases a IO's reference count, so it won't be automatically destroyed
 * when all tasks have completed.
 *
 * Use this function in order to use the IO outside of a scheduled task.
 *
 * This function is thread-safe.
 */
SFUNC fio_io_s *fio_io_dup(fio_io_s *io);

/**
 * Decreases a IO's reference count, so it could be automatically destroyed
 * when all other tasks have completed.
 *
 * Use this function once finished with a IO that was `dup`-ed.
 *
 * This function is thread-safe.
 */
SFUNC void fio_io_free(fio_io_s *io);

/** Suspends future `on_data` events for the IO. */
SFUNC void fio_io_suspend(fio_io_s *io);

/** Listens for future `on_data` events related to the IO. */
SFUNC void fio_io_unsuspend(fio_io_s *io);

/** Returns 1 if the IO handle was suspended. */
SFUNC int fio_io_is_suspended(fio_io_s *io);

/** Returns 1 if the IO handle is marked as open. */
SFUNC int fio_io_is_open(fio_io_s *io);

/** Returns the approximate number of bytes in the outgoing buffer. */
SFUNC size_t fio_io_backlog(fio_io_s *io);

/** Does nothing. */
SFUNC void fio_io_noop(fio_io_s *io);

/* *****************************************************************************
Task Scheduling
***************************************************************************** */

/** Schedules a task for delayed execution. This function is thread-safe. */
SFUNC void fio_io_defer(void (*task)(void *, void *),
                        void *udata1,
                        void *udata2);

/** Schedules a timer bound task, see `fio_timer_schedule`. */
SFUNC void fio_io_run_every(fio_timer_schedule_args_s args);
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
 *        void (*on_stop)(void *, void *)
 * * Timer interval, in milliseconds:
 *        uint32_t every
 * * The number of times the timer should be performed. -1 == infinity:
 *        int32_t repetitions
 */
#define fio_io_run_every(...)                                                  \
  fio_io_run_every((fio_timer_schedule_args_s){__VA_ARGS__})

/** Returns a pointer for the IO reactor's queue. */
SFUNC fio_queue_s *fio_io_queue(void);

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
  /** Helper that converts a `fio_io_tls_s` into the implementation's context.
   */
  void *(*build_context)(fio_io_tls_s *tls, uint8_t is_client);
  /** Helper to free the context built by build_context. */
  void (*free_context)(void *context);
  /** called when a new IO is first attached to a valid protocol. */
  void (*start)(fio_io_s *io);
  /** Called to perform a non-blocking `read`, same as the system call. */
  ssize_t (*read)(int fd, void *buf, size_t len, void *context);
  /** Called to perform a non-blocking `write`, same as the system call. */
  ssize_t (*write)(int fd, const void *buf, size_t len, void *context);
  /** Sends any unsent internal data. Returns 0 only if all data was sent. */
  int (*flush)(int fd, void *context);
  /** Called when the IO object finished sending all data before closure. */
  void (*finish)(int fd, void *context);
  /** Called after the IO object is closed, used to cleanup its `tls` object. */
  void (*cleanup)(void *context);
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
struct fio_io_protocol_s {
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
  /** Called when an IO is attached to the protocol. */
  void (*on_attach)(fio_io_s *io);
  /** Called when a data is available. */
  void (*on_data)(fio_io_s *io);
  /** called once all pending `fio_write` calls are finished. */
  void (*on_ready)(fio_io_s *io);

  /**
   * Called when the IO reactor is shutting down, immediately before closing the
   * connection.
   *
   * After the `on_shutdown` callback returns, the socket is marked for closure.
   *
   * Once the socket was marked for closure, facil.io will allow a limited
   * amount of time for data to be sent, after which the socket might be closed
   * even if the client did not consume all buffered data.
   */
  void (*on_shutdown)(fio_io_s *io);
  /**
   * Called when a connection's timeout was reached
   *
   * Can be set to `fio_io_touch` if timeout is irrelevant (i.e., UDP).
   */
  void (*on_timeout)(fio_io_s *io);
  /** Used as a default `on_message` when an IO object subscribes. */

  /** Called after the connection was closed (once per IO). */
  void (*on_close)(void *iobuf, void *udata);

  void (*on_pubsub)(struct fio_msg_s *msg);
  /** Allows user specific protocol agnostic callbacks. */
  void (*on_user1)(fio_io_s *io, void *user_data);
  /** Allows user specific protocol agnostic callbacks. */
  void (*on_user2)(fio_io_s *io, void *user_data);
  /** Allows user specific protocol agnostic callbacks. */
  void (*on_user3)(fio_io_s *io, void *user_data);
  /** Reserved for future protocol agnostic callbacks. */
  void (*on_reserved)(fio_io_s *io, void *user_data);
  /**
   * Defines Transport Layer callbacks that facil.io will treat as non-blocking
   * system calls.
   */
  fio_io_functions_s io_functions;
  /**
   * The timeout value in milliseconds for all connections using this protocol.
   *
   * Limited to FIO_IO_TIMEOUT_MAX seconds. Zero (0) == FIO_IO_TIMEOUT_MAX
   */
  uint32_t timeout;
  /** The number of bytes to allocate for the fio_io_buf buffer. */
  uint32_t buffer_size;
};

/** Performs a task for each IO in the stated protocol. */
SFUNC size_t fio_io_protocol_each(fio_io_protocol_s *protocol,
                                  void (*task)(fio_io_s *, void *udata2),
                                  void *udata2);

/* *****************************************************************************
Connection Object Links / Environment
***************************************************************************** */

/** Named arguments for the `fio_io_env_set` function. */
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
} fio_io_env_set_args_s;

/** Named arguments for the `fio_io_env_unset` function. */
typedef struct {
  /** A numerical type filter. Should be the same as used with
   * `fio_io_env_set` */
  intptr_t type;
  /** The name of the object. Should be the same as used with `fio_io_env_set`
   */
  fio_buf_info_s name;
} fio_io_env_get_args_s;

/** Returns the named `udata` associated with the IO object (or `NULL`). */
SFUNC void *fio_io_env_get(fio_io_s *io, fio_io_env_get_args_s);

/** Returns the named `udata` associated with the IO object (or `NULL`). */
#define fio_io_env_get(io, ...)                                                \
  fio_io_env_get(io, (fio_io_env_get_args_s){__VA_ARGS__})

/**
 * Links an object to a connection's lifetime / environment.
 *
 * The `on_close` callback will be called once the connection has died.
 *
 * If the `io` is NULL, the value will be set for the global environment.
 */
SFUNC void fio_io_env_set(fio_io_s *io, fio_io_env_set_args_s);

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
#define fio_io_env_set(io, ...)                                                \
  fio_io_env_set(io, (fio_io_env_set_args_s){__VA_ARGS__})

/**
 * Un-links an object from the connection's lifetime, so it's `on_close`
 * callback will NOT be called.
 *
 * Returns 0 on success and -1 if the object couldn't be found.
 */
SFUNC int fio_io_env_unset(fio_io_s *io, fio_io_env_get_args_s);

/**
 * Un-links an object from the connection's lifetime, so it's `on_close`
 * callback will NOT be called.
 *
 * Returns 0 on success and -1 if the object couldn't be found.
 *
 * This is a helper MACRO that allows the function to be called using named
 * arguments.
 */
#define fio_io_env_unset(io, ...)                                              \
  fio_io_env_unset(io, (fio_io_env_get_args_s){__VA_ARGS__})

/**
 * Removes an object from the connection's lifetime / environment, calling it's
 * `on_close` callback as if the connection was closed.
 */
SFUNC int fio_io_env_remove(fio_io_s *io, fio_io_env_get_args_s);

/**
 * Removes an object from the connection's lifetime / environment, calling it's
 * `on_close` callback as if the connection was closed.
 *
 * This is a helper MACRO that allows the function to be called using named
 * arguments.
 */
#define fio_io_env_remove(io, ...)                                             \
  fio_io_env_remove(io, (fio_io_env_get_args_s){__VA_ARGS__})

/* *****************************************************************************
TLS Context Helper Types
***************************************************************************** */

/** Performs a `new` operation, returning a new `fio_io_tls_s` context. */
SFUNC fio_io_tls_s *fio_io_tls_new(void);

/** Takes a parsed URL and optional TLS target and returns a TLS if needed. */
SFUNC fio_io_tls_s *fio_io_tls_from_url(fio_io_tls_s *target_or_null,
                                        fio_url_s url);

/** Performs a `dup` operation, increasing the object's reference count. */
SFUNC fio_io_tls_s *fio_io_tls_dup(fio_io_tls_s *);

/** Performs a `free` operation, reducing the reference count and freeing. */
SFUNC void fio_io_tls_free(fio_io_tls_s *);

/**
 * Adds a certificate a new SSL/TLS context / settings object (SNI support).
 *
 *      fio_io_tls_cert_add(tls, "www.example.com",
 *                            "public_key.pem",
 *                            "private_key.pem", NULL );
 *
 * NOTE: Except for the `tls` and `server_name` arguments, all arguments might
 * be `NULL`, which a context builder (`fio_io_functions_s`) should
 * treat as a request for a self-signed certificate. It may be silently ignored.
 */
SFUNC fio_io_tls_s *fio_io_tls_cert_add(fio_io_tls_s *,
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
SFUNC fio_io_tls_s *fio_io_tls_alpn_add(fio_io_tls_s *tls,
                                        const char *protocol_name,
                                        void (*on_selected)(fio_io_s *));

/** Calls the `on_selected` callback for the `fio_io_tls_s` object. */
SFUNC int fio_io_tls_alpn_select(fio_io_tls_s *tls,
                                 const char *protocol_name,
                                 size_t name_length,
                                 fio_io_s *);

/**
 * Adds a certificate to the "trust" list, which automatically adds a peer
 * verification requirement.
 *
 * If `public_cert_file` is `NULL`, implementation is expected to add the
 * system's default trust registry.
 *
 * Note: when the `fio_io_tls_s` object is used for server connections, this
 * should limit connections to clients that connect using a trusted certificate.
 *
 *      fio_io_tls_trust_add(tls, "google-ca.pem" );
 */
SFUNC fio_io_tls_s *fio_io_tls_trust_add(fio_io_tls_s *,
                                         const char *public_cert_file);

/**
 * Returns the number of `fio_io_tls_cert_add` instructions.
 *
 * This could be used when deciding if to add a NULL instruction (self-signed).
 *
 * If `fio_io_tls_cert_add` was never called, zero (0) is returned.
 */
SFUNC uintptr_t fio_io_tls_cert_count(fio_io_tls_s *tls);

/**
 * Returns the number of registered ALPN protocol names.
 *
 * This could be used when deciding if protocol selection should be delegated to
 * the ALPN mechanism, or whether a protocol should be immediately assigned.
 *
 * If no ALPN protocols are registered, zero (0) is returned.
 */
SFUNC uintptr_t fio_io_tls_alpn_count(fio_io_tls_s *tls);

/**
 * Returns the number of `fio_io_tls_trust_add` instructions.
 *
 * This could be used when deciding if to disable peer verification or not.
 *
 * If `fio_io_tls_trust_add` was never called, zero (0) is returned.
 */
SFUNC uintptr_t fio_io_tls_trust_count(fio_io_tls_s *tls);

/** Arguments (and info) for `fio_io_tls_each`. */
typedef struct fio_io_tls_each_s {
  fio_io_tls_s *tls;
  void *udata;
  void *udata2;
  int (*each_cert)(struct fio_io_tls_each_s *,
                   const char *server_name,
                   const char *public_cert_file,
                   const char *private_key_file,
                   const char *pk_password);
  int (*each_alpn)(struct fio_io_tls_each_s *,
                   const char *protocol_name,
                   void (*on_selected)(fio_io_s *));
  int (*each_trust)(struct fio_io_tls_each_s *, const char *public_cert_file);
} fio_io_tls_each_s;

/** Calls callbacks for certificate, trust certificate and ALPN added. */
SFUNC int fio_io_tls_each(fio_io_tls_each_s);

/** `fio_io_tls_each` helper macro, see `fio_io_tls_each_s` for named
 * arguments. */
#define fio_io_tls_each(tls_, ...)                                             \
  fio_io_tls_each(((fio_io_tls_each_s){.tls = tls_, __VA_ARGS__}))

/** If `NULL` returns current default, otherwise sets it. */
SFUNC fio_io_functions_s fio_io_tls_default_functions(fio_io_functions_s *);

/* *****************************************************************************
IO Async Queue - Worker Threads for non-IO tasks
***************************************************************************** */

/** The IO Async Queue type. */
struct fio_io_async_s {
  fio_queue_s *q;
  uint32_t count;
  fio_queue_s queue;
  fio_timer_queue_s timers;
  FIO_LIST_NODE node;
};

/**
 * Initializes an IO Async Queue (multi-threaded task queue).
 *
 * The queue automatically spawns threads and shuts down as the IO reactor
 * starts or stops.
 *
 * It is recommended that the `fio_io_async_s` be allocated as a static
 * variable, as its memory must remain valid throughout the lifetime of the
 * IO reactor's app.
 */
#define FIO_IO_ASYN_INIT ((fio_io_async_s){0})

/** Returns the current task queue associated with the IO Async Queue. */
FIO_IFUNC fio_queue_s *fio_io_async_queue(fio_io_async_s *q) { return q->q; }

/**
 * Attaches an IO Async Queue for use in multi-threaded (non IO) tasks.
 *
 * This function can be called multiple times for the same (or other) queue, as
 * long as the async queue (`fio_io_async_s`) was previously initialized using
 * `FIO_IO_ASYN_INIT` or zeroed out. i.e.:
 *
 *     static fio_io_async_s SLOW_HTTP_TASKS = FIO_IO_ASYN_INIT;
 *     fio_io_async_attach(&SLOW_HTTP_TASKS, 32);
 */
SFUNC void fio_io_async_attach(fio_io_async_s *q, uint32_t threads);

/** Pushes a task to an IO Async Queue (macro helper). */
#define fio_io_async(q_, ...) fio_queue_push((q_)->q, __VA_ARGS__)

/** Schedules a timer bound task for the async queue (`fio_timer_schedule`). */
SFUNC void fio_io_async_every(fio_io_async_s *q, fio_timer_schedule_args_s);

/**
 * Schedules a timer bound task, for the async queue, see `fio_timer_schedule`.
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
 *        void (*on_stop)(void *, void *)
 * * Timer interval, in milliseconds:
 *        uint32_t every
 * * The number of times the timer should be performed. -1 == infinity:
 *        int32_t repetitions
 */
#define fio_io_async_every(async, ...)                                         \
  fio_io_async_every(async, (fio_timer_schedule_args_s){__VA_ARGS__})

/* *****************************************************************************
IO API Finish
***************************************************************************** */
#endif /* FIO_IO */
