/* *****************************************************************************
Copyright: Boaz Segev, 2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___SERVER___H
/**
 * An Evented, Single-Threaded Server
 *
 * This example code can be used as a single header library for a portable
 * evented server that uses a single thread to manage all events.
 *
 * The server is NOT thread safe (cannot use multiple threads).
 *
 * Only the `fio_defer`, `fio_dup`, `fio_undup` functions are thread-safe,
 * allowing non-IO bound tasks to run in other threads before submitting the
 * result to an IO bound task using `fio_defer`.
 *
 * The server uses the `poll` system call.
 *
 * The server was NOT thoroughly tested and might not be production ready.
 *
 * The server depends on the facil.io CSTL single header library.
 *
 * To make the functions non-static, define the `FIO_SERVER_EXTERN` macro.
 *
 * To emit non-static function implementation, define `FIO_SERVER_EXTERN_IMP`
 */
#define H___SERVER___H

/* *****************************************************************************
Modules from the facil.io CSTL
***************************************************************************** */

#define FIO_ATOL
#define FIO_ATOMIC
#define FIO_QUEUE
#define FIO_SIGNAL
#define FIO_SOCK
#define FIO_TIME
#include "fio-stl.h"

#ifdef FIO_SERVER_EXTERN
#define SRV_FUNC
#else
#define SRV_FUNC FIO_SFUNC
#undef FIO_SERVER_EXTERN_IMP
#define FIO_SERVER_EXTERN_IMP 1
#endif

/* *****************************************************************************
IO Types
***************************************************************************** */

/** The main protocol object type. See `struct fio_protocol_s`. */
typedef struct fio_protocol_s fio_protocol_s;

/** The main IO object type. Should be treated as an opaque pointer. */
typedef struct fio_s fio_s;

/* *****************************************************************************
Listening to Incoming Connections
***************************************************************************** */

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

/**
 * Sets up a network service on a listening socket.
 *
 * Returns 0 on success or -1 on error.
 *
 * See the `fio_listen` Macro for details.
 */
SRV_FUNC int fio_listen(struct fio_listen_args args);
#define fio_listen(...) fio_listen((struct fio_listen_args){__VA_ARGS__})

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
SRV_FUNC fio_s *fio_attach_fd(int fd,
                              fio_protocol_s *protocol,
                              void *udata,
                              void *tls);

/** Sets a new protocol object. `NULL` is a valid "only-write" protocol. */
SRV_FUNC fio_protocol_s *fio_protocol_set(fio_s *io, fio_protocol_s *protocol);

/**
 * Returns a pointer to the current protocol object.
 *
 * If `protocol` wasn't properly set, the pointer might be invalid.
 */
SRV_FUNC fio_protocol_s *fio_protocol_get(fio_s *io);

/** Associates a new `udata` pointer with the IO, returning the old `udata` */
SRV_FUNC void *fio_udata_set(fio_s *io, void *udata);

/** Returns the `udata` pointer associated with the IO. */
SRV_FUNC void *fio_udata_get(fio_s *io);

/** Associates a new `tls` pointer with the IO, returning the old `tls` */
SRV_FUNC void *fio_tls_set(fio_s *io, void *tls);

/** Returns the `tls` pointer associated with the IO. */
SRV_FUNC void *fio_tls_get(fio_s *io);

/** Returns the socket file descriptor (fd) associated with the IO. */
SRV_FUNC int fio_fd_get(fio_s *io);

/* Resets a socket's timeout counter. */
SRV_FUNC void fio_touch(fio_s *io);

/**
 * Reads data to the buffer, if any data exists. Returns the number of bytes
 * read.
 *
 * NOTE: zero (`0`) is a valid return value meaning no data was available.
 */
SRV_FUNC size_t fio_read(fio_s *io, void *buf, size_t len);

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
SRV_FUNC void fio_write2(fio_s *io, fio_write_args_s args);
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
SRV_FUNC void fio_close(fio_s *io);

/** Marks the IO for immediate closure. */
SRV_FUNC void fio_close_now(fio_s *io);

/**
 * Increases a IO's reference count, so it won't be automatically destroyed
 * when all tasks have completed.
 *
 * Use this function in order to use the IO outside of a scheduled task.
 */
SRV_FUNC fio_s *fio_dup(fio_s *io);

/**
 * Decreases a IO's reference count, so it could be automatically destroyed
 * when all other tasks have completed.
 *
 * Use this function once finished with a IO that was `dup`-ed.
 */
SRV_FUNC void fio_undup(fio_s *io);

/** Suspends future "on_data" events for the IO. */
SRV_FUNC void fio_suspend(fio_s *io);
/** Listens for future "on_data" events related to the IO. */
SRV_FUNC void fio_unsuspend(fio_s *io);

/* *****************************************************************************
Task Scheduling
***************************************************************************** */

/** Schedules a task for delayed execution. */
void fio_defer(void (*task)(void *, void *), void *udata1, void *udata2);

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
   * MUST be initialized to zero
   */
  struct {
    /* A linked list of currently attached IOs - do NOT alter. */
    FIO_LIST_HEAD ios;
    /* A linked list of other protocols used by IO core - do NOT alter. */
    FIO_LIST_NODE protocols;
    /* internal flags - do NOT alter after initial initialization to zero. */
    uintptr_t flags;
  } reserved;
  /**
   * Called when an IO was attached. Locks the IO's task lock.
   *
   * Note: this is scheduled when setting the protocol, but depending on race
   * conditions, the previous protocol, and the socket's state, it may run after
   * the `on_ready` or `on_data` are called.
   * */
  void (*on_attach)(fio_s *io);
  /** Called when a data is available. Locks the IO's task lock. */
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
   * Performed within the IO's task lock.
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
   * Defines Transport Later callbacks that facil.io will treat as non-blocking
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
/* *****************************************************************************











                                                                Implementation











***************************************************************************** */

#if defined(FIO_SERVER_EXTERN_IMP) && FIO_SERVER_EXTERN_IMP

#define FIO_MEMORY_NAME srvmem
#include "fio-stl.h"
#undef FIO_MEM_REALLOC
#undef FIO_MEM_FREE
#undef FIO_MEM_REALLOC_IS_SAFE
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  srvmem_realloc2((ptr), (new_size), (copy_len))
#define FIO_MEM_FREE(ptr, size) srvmem_free((ptr))
#define FIO_MEM_REALLOC_IS_SAFE srvmem_realloc_is_safe()

#define FIO_STREAM
#define FIO_POLL
#include "fio-stl.h"

/* *****************************************************************************
IO Registry - NOT thread safe (access from IO core thread only)
***************************************************************************** */

#ifndef FIO_SRV_VALIDATE_IO
#define FIO_SRV_VALIDATE_IO 0
#endif

#ifndef FIO_SRV_BUFFER_PER_WRITE
#define FIO_SRV_BUFFER_PER_WRITE 65536U
#endif

#ifndef FIO_SRV_THROTTLE_LIMIT
#define FIO_SRV_THROTTLE_LIMIT 2097152U
#endif

#ifndef FIO_SRV_TIMEOUT_MAX
#define FIO_SRV_TIMEOUT_MAX 300
#endif

/* *****************************************************************************
Protocol validation
***************************************************************************** */

static void srv_on_ev_mock_sus(fio_s *io) { fio_suspend(io); }
static void srv_on_ev_mock(fio_s *io) { (void)(io); }
static void srv_on_close_mock(void *ptr) { (void)ptr; }
static void srv_on_ev_on_timeout(fio_s *io) { fio_close_now(io); }
/* Called to perform a non-blocking `read`, same as the system call. */
static ssize_t io_func_default_read(int fd, void *buf, size_t len, void *tls) {
  (void)tls;
  return fio_sock_read(fd, buf, len);
}
/** Called to perform a non-blocking `write`, same as the system call. */
static ssize_t io_func_default_write(int fd,
                                     const void *buf,
                                     size_t len,
                                     void *tls) {
  (void)tls;
  return fio_sock_write(fd, buf, len);
}
/** Sends any unsent internal data. Returns 0 only if all data was sent. */
static int io_func_default_flush(int fd, void *tls) {
  return 0;
  (void)fd;
  (void)tls;
}
static void io_func_default_free(void *tls) { (void)tls; }

FIO_SFUNC void fio___srv_init_protocol(fio_protocol_s *pr) {
  pr->reserved.protocols = FIO_LIST_INIT(pr->reserved.protocols);
  pr->reserved.ios = FIO_LIST_INIT(pr->reserved.ios);
  if (!pr->on_attach)
    pr->on_attach = srv_on_ev_mock;
  if (!pr->on_data)
    pr->on_data = srv_on_ev_mock_sus;
  if (!pr->on_ready)
    pr->on_ready = srv_on_ev_mock;
  if (!pr->on_close)
    pr->on_close = srv_on_close_mock;
  if (!pr->on_shutdown)
    pr->on_shutdown = srv_on_ev_mock;
  if (!pr->on_timeout)
    pr->on_timeout = srv_on_ev_on_timeout;
  if (!pr->io_functions.read)
    pr->io_functions.read = io_func_default_read;
  if (!pr->io_functions.write)
    pr->io_functions.write = io_func_default_write;
  if (!pr->io_functions.flush)
    pr->io_functions.flush = io_func_default_flush;
  if (!pr->io_functions.free)
    pr->io_functions.free = io_func_default_free;
}

fio_protocol_s MOCK_PROTOCOL;

FIO_IFUNC void fio___srv_init_protocol_test(fio_protocol_s *pr) {
  if (!fio_atomic_or(&pr->reserved.flags, 1))
    fio___srv_init_protocol(pr);
}

/* *****************************************************************************
Global State
***************************************************************************** */

static void fio___srv_poll_on_data_schd(int fd, void *udata);
static void fio___srv_poll_on_ready_schd(int fd, void *udata);
static void fio___srv_poll_on_close_schd(int fd, void *udata);

static struct {
  FIO_LIST_HEAD protocols;
  fio_poll_s fds;
  int64_t tick;
  volatile uint8_t running;
} fio___srvdata = {
    .fds = FIO_POLL_INIT(fio___srv_poll_on_data_schd,
                         fio___srv_poll_on_ready_schd,
                         fio___srv_poll_on_close_schd),
};

static fio_queue_s fio___srv_tasks[1];

/* *****************************************************************************
IO objects
***************************************************************************** */

struct fio_s {
  void *udata;
  void *tls;
  fio_protocol_s *pr;
  FIO_LIST_NODE node;
  fio_stream_s stream;
  int64_t active;
  uint32_t state;
  int fd;
};

#define FIO_STATE_OPEN      ((uint32_t)1U)
#define FIO_STATE_SUSPENDED ((uint32_t)2U)
#define FIO_STATE_THROTTLED ((uint32_t)4U)
#define FIO_STATE_CLOSING   ((uint32_t)8U)

FIO_SFUNC void fio_s_on_close_task(void *func_, void *udata) {
  union {
    void (*func)(void *);
    void *ptr;
  } u = {.ptr = func_};
  u.func(udata);
}

FIO_SFUNC void fio_s_init(fio_s *io) {
  *io = (fio_s){
      .pr = &MOCK_PROTOCOL,
      .node = FIO_LIST_INIT(io->node),
      .active = fio___srvdata.tick,
      .state = FIO_STATE_OPEN,
      .fd = -1,
  };
  FIO_LIST_PUSH(&io->pr->reserved.ios, &io->node);
  FIO_LIST_REMOVE(&MOCK_PROTOCOL.reserved.protocols);
  FIO_LIST_PUSH(&fio___srvdata.protocols, &MOCK_PROTOCOL.reserved.protocols);
}

FIO_SFUNC void fio_s_destroy(fio_s *io) {
  FIO_LIST_REMOVE(&io->node);
  fio_sock_close(io->fd);
  fio_stream_destroy(&io->stream);
  fio_poll_forget(&fio___srvdata.fds, io->fd);
  FIO_LOG_DDEBUG2("detaching and destroying %p (fd %d)", (void *)io, io->fd);
  union {
    void (*func)(void *);
    void *ptr;
  } u;
  u.func = io->pr->on_close;
  fio_queue_push(fio___srv_tasks, fio_s_on_close_task, u.ptr, io->udata);
  u.func = io->pr->io_functions.free;
  fio_queue_push(fio___srv_tasks, fio_s_on_close_task, u.ptr, io->tls);

  if (FIO_LIST_IS_EMPTY(&io->pr->reserved.ios))
    FIO_LIST_REMOVE(&io->pr->reserved.protocols);
}

#define FIO_REF_NAME       fio
#define FIO_REF_INIT(o)    fio_s_init(&(o))
#define FIO_REF_DESTROY(o) fio_s_destroy(&(o))
#include "fio-stl.h"

static void fio___protocol_set_task(void *io_, void *old_) {
  fio_s *io = (fio_s *)io_;
  fio_protocol_s *old = (fio_protocol_s *)old_;
  FIO_LIST_REMOVE(&io->node);
  if (FIO_LIST_IS_EMPTY(&io->pr->reserved.ios))
    FIO_LIST_PUSH(&fio___srvdata.protocols, &io->pr->reserved.protocols);
  FIO_LIST_PUSH(&io->pr->reserved.ios, &io->node);
  fio_poll_monitor(&fio___srvdata.fds, io->fd, (void *)io, POLLIN | POLLOUT);
  /* TODO / FIX ? should call `on_close` for old protocol?*/
  if (FIO_LIST_IS_EMPTY(&old->reserved.ios))
    FIO_LIST_REMOVE(&old->reserved.protocols);
  io->pr->on_attach(io);
}

/** Sets a new protocol object, returning the old protocol. */
SRV_FUNC fio_protocol_s *fio_protocol_set(fio_s *io, fio_protocol_s *pr) {
  if (!pr)
    pr = &MOCK_PROTOCOL;
  fio_protocol_s *old = io->pr;
  if (pr == old)
    return NULL;
  fio___srv_init_protocol_test(pr);
  io->pr = pr;
  fio___protocol_set_task((void *)io, (void *)old);
  return old;
}

/** Returns a pointer to the current protocol object. */
SRV_FUNC fio_protocol_s *fio_protocol_get(fio_s *io) { return io->pr; }

/* Attaches the socket in `fd` to the facio.io engine (reactor). */
SRV_FUNC fio_s *fio_attach_fd(int fd,
                              fio_protocol_s *protocol,
                              void *udata,
                              void *tls) {
  if (fd == -1)
    return NULL;
  if (!protocol)
    protocol = &MOCK_PROTOCOL;
  fio___srv_init_protocol_test(protocol);
  fio_s *io = fio_new2();
  FIO_ASSERT_ALLOC(io);
  FIO_LOG_DDEBUG2("attaching fd %d to IO object %p", fd, (void *)io);
  fio_sock_set_non_block(fd);
  fio_protocol_s *old = io->pr;
  io->fd = fd;
  io->pr = protocol;
  io->udata = udata;
  io->tls = tls;
  fio_queue_push(fio___srv_tasks, fio___protocol_set_task, io, old);
  return io;
}

/**
 * Increases a IO's reference count, so it won't be automatically destroyed
 * when all tasks have completed.
 */
SRV_FUNC fio_s *fio_dup(fio_s *io) { return fio_dup2(io); }

static void fio_undup_task(void *io, void *ignr_) {
  (void)ignr_;
  fio_free2((fio_s *)io);
}
/**
 * Decreases a IO's reference count, so it could be automatically destroyed
 * when all other tasks have completed.
 */
SRV_FUNC void fio_undup(fio_s *io) {
  fio_queue_push(fio___srv_tasks, fio_undup_task, io);
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
      fio_poll_monitor(&fio___srvdata.fds, io->fd, io, POLLIN);
    }
  } else if ((io->state & FIO_STATE_OPEN)) {
    fio_poll_monitor(&fio___srvdata.fds, io->fd, io, POLLOUT);
  }
  fio_free2(io);
  return;
}

static void fio___srv_poll_on_ready(void *io_, void *ignr_) {
  (void)ignr_;
  fio_s *io = (fio_s *)io_;
  if ((io->state & FIO_STATE_OPEN)) {
    char buf_mem[FIO_SRV_BUFFER_PER_WRITE];
    size_t total = 0;
    for (;;) {
      size_t len = FIO_SRV_BUFFER_PER_WRITE;
      char *buf = buf_mem;
      fio_stream_read(&io->stream, &buf, &len);
      if (!len)
        break;
      ssize_t r = io->pr->io_functions.write(io->fd, buf, len, io->tls);
      if (r > 0) {
        total += r;
        fio_stream_advance(&io->stream, len);
        continue;
      } else if ((r == -1) & ((errno == EWOULDBLOCK) | (errno == EAGAIN) |
                              (errno == EINTR))) {
        break;
      } else {
        fio_close_now(io);
        goto finish;
      }
    }
    if (total)
      fio_touch(io);
    if (!fio_stream_any(&io->stream) &&
        !io->pr->io_functions.flush(io->fd, io->tls)) {
      if ((io->state & FIO_STATE_CLOSING)) {
        fio_close_now(io);
      } else {
        if ((io->state & FIO_STATE_THROTTLED)) {
          fio_atomic_and(&io->state, ~FIO_STATE_THROTTLED);
          fio_poll_monitor(&fio___srvdata.fds, io->fd, io, POLLIN);
        }
        io->pr->on_ready(io);
      }
    } else if ((io->state & FIO_STATE_OPEN)) {
      if (fio_stream_length(&io->stream) >= FIO_SRV_THROTTLE_LIMIT) {
        if (!(io->state & FIO_STATE_THROTTLED))
          FIO_LOG_DDEBUG2("throttled IO %p (fd %d)", (void *)io, io->fd);
        fio_atomic_or(&io->state, FIO_STATE_THROTTLED);
      }
      fio_poll_monitor(&fio___srvdata.fds, io->fd, io, POLLOUT);
    }
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

static void fio___srv_poll_on_data_schd(int fd, void *io) {
  (void)fd;
  fio_queue_push(fio___srv_tasks,
                 fio___srv_poll_on_data,
                 fio_dup2((fio_s *)io));
}
static void fio___srv_poll_on_ready_schd(int fd, void *io) {
  (void)fd;
  fio_queue_push(fio___srv_tasks,
                 fio___srv_poll_on_ready,
                 fio_dup2((fio_s *)io));
}
static void fio___srv_poll_on_close_schd(int fd, void *io) {
  (void)fd;
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
    int64_t limit = now_milli - ((int64_t)pr->timeout * 1000);
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

SRV_FUNC void fio_srv_tick(int timeout) {
  fio_poll_review(&fio___srvdata.fds, timeout);
  fio___srvdata.tick = fio_time_milli();
  fio_queue_perform_all(fio___srv_tasks);
  if (fio___srv_review_timeouts())
    fio_queue_perform_all(fio___srv_tasks);
}

SRV_FUNC void fio_srv_shutdown(void) {
  int64_t shutting_down = fio___srvdata.tick = fio_time_milli();
  FIO_LIST_EACH(fio_protocol_s,
                reserved.protocols,
                &fio___srvdata.protocols,
                pr) {
    FIO_LIST_EACH(fio_s, node, &pr->reserved.ios, io) {
      io->pr->on_shutdown(io);
      fio_close(io);
    }
  }
  while (shutting_down + 10000 >= fio___srvdata.tick &&
         !FIO_LIST_IS_EMPTY(&fio___srvdata.protocols)) {
    fio_srv_tick(100);
  }
  FIO_LIST_EACH(fio_protocol_s,
                reserved.protocols,
                &fio___srvdata.protocols,
                pr) {
    FIO_LIST_EACH(fio_s, node, &pr->reserved.ios, io) { fio_close_now(io); }
  }
  fio_queue_perform_all(fio___srv_tasks);
  fio_poll_destroy(&fio___srvdata.fds);
  fio_queue_destroy(fio___srv_tasks);
}

SRV_FUNC void fio_srv_run(void) {
  volatile uint8_t stop = 0;
  fio_sock_maximize_limits();
  fio_signal_monitor(SIGINT, fio___srv_signal_handle, (void *)&stop);
  fio_signal_monitor(SIGTERM, fio___srv_signal_handle, (void *)&stop);
#ifdef SIGPIPE
  fio_signal_monitor(SIGPIPE, NULL, NULL);
#endif
  fio___srvdata.tick = fio_time_milli();
  fio_queue_perform_all(fio___srv_tasks);
  do {
    fio_srv_tick(250);
    fio_signal_review();
  } while (!stop);
  fio_srv_shutdown();
  fio_signal_forget(SIGINT);
  fio_signal_forget(SIGTERM);
#ifdef SIGPIPE
  fio_signal_forget(SIGPIPE);
#endif
}

/* *****************************************************************************
IO API
***************************************************************************** */

/** Associates a new `udata` pointer with the IO, returning the old `udata` */
SRV_FUNC void *fio_udata_set(fio_s *io, void *udata) {
  void *old = io->udata;
  io->udata = udata;
  return old;
}

/** Returns the `udata` pointer associated with the IO. */
SRV_FUNC void *fio_udata_get(fio_s *io) { return io->udata; }

/** Associates a new `tls` pointer with the IO, returning the old `tls` */
SRV_FUNC void *fio_tls_set(fio_s *io, void *tls) {
  void *old = io->tls;
  io->tls = tls;
  return old;
}

/** Returns the `tls` pointer associated with the IO. */
SRV_FUNC void *fio_tls_get(fio_s *io) { return io->tls; }

/** Returns the socket file descriptor (fd) associated with the IO. */
SRV_FUNC int fio_fd_get(fio_s *io) { return io->fd; }

/* Resets a socket's timeout counter. */
SRV_FUNC void fio_touch(fio_s *io) {
  io->active = fio___srvdata.tick;
  FIO_LIST_REMOVE(&io->node);
  FIO_LIST_PUSH(&io->pr->reserved.ios, &io->node);
}

/**
 * Reads data to the buffer, if any data exists. Returns the number of bytes
 * read.
 *
 * NOTE: zero (`0`) is a valid return value meaning no data was available.
 */
SRV_FUNC size_t fio_read(fio_s *io, void *buf, size_t len) {
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

/**
 * Writes data to the outgoing buffer and schedules the buffer to be sent.
 */
SRV_FUNC void fio_write2 FIO_NOOP(fio_s *io, fio_write_args_s args) {
  fio_stream_packet_s *packet = NULL;
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
  if (!io ||
      ((io->state & (FIO_STATE_OPEN | FIO_STATE_CLOSING)) ^ FIO_STATE_OPEN))
    goto io_error;
  fio_stream_add(&io->stream, packet);
  fio_queue_push(fio___srv_tasks,
                 fio___srv_poll_on_ready,
                 fio_dup2((fio_s *)io));
  return;
error:
  FIO_LOG_ERROR("couldn't create user-packet for IO %p", (void *)io);
  if (args.dealloc)
    args.dealloc(args.buf);
  return;
io_error:
  fio_stream_pack_free(packet);
  if (!io)
    FIO_LOG_ERROR("Invalid IO (NULL) for user-packet");
  return;
}

/** Marks the IO for closure as soon as scheduled data was sent. */
SRV_FUNC void fio_close(fio_s *io) {
  if (io && (io->state & FIO_STATE_OPEN) &&
      !(fio_atomic_or(&io->state, FIO_STATE_CLOSING) & FIO_STATE_CLOSING)) {
    FIO_LOG_DDEBUG2("scheduling IO %p (fd %d) for closure", (void *)io, io->fd);
    fio_queue_push(fio___srv_tasks,
                   fio___srv_poll_on_ready,
                   fio_dup2((fio_s *)io));
  }
}

/** Marks the IO for immediate closure. */
SRV_FUNC void fio_close_now(fio_s *io) {
  io->state |= FIO_STATE_CLOSING;
  fio_stream_destroy(&io->stream);
  fio_queue_push(fio___srv_tasks,
                 fio___srv_poll_on_ready,
                 fio_dup2((fio_s *)io));
  if ((fio_atomic_and(&io->state, ~FIO_STATE_OPEN) & FIO_STATE_OPEN))
    fio_free2(io);
}

/** Suspends future "on_data" events for the IO. */
SRV_FUNC void fio_suspend(fio_s *io) { io->state |= FIO_STATE_SUSPENDED; }
/** Listens for future "on_data" events related to the IO. */
SRV_FUNC void fio_unsuspend(fio_s *io) {
  if ((fio_atomic_and(&io->state, ~FIO_STATE_SUSPENDED) & FIO_STATE_SUSPENDED))
    fio_poll_monitor(&fio___srvdata.fds, io->fd, (void *)io, POLLIN);
}

/* *****************************************************************************
Listening
***************************************************************************** */

static void fio___srv_listen_on_data(fio_s *io) {
  int fd;
  struct fio_listen_args *s = (struct fio_listen_args *)(io->udata);
  while ((fd = accept(fio_fd_get(io), NULL, NULL)) != -1) {
    s->on_open(fd, s->udata);
  }
}
static void fio___srv_listen_on_close(void *settings_) {
  struct fio_listen_args *s = (struct fio_listen_args *)settings_;
  if (s->on_finish)
    s->on_finish(s->udata);
  FIO_LOG_DEBUG2("Stopped listening on %s", s->url);
  free(s);
}
static void fio___srv_listen_on_timeout(fio_s *io) { fio_touch(io); }

static fio_protocol_s FIO___LISTEN_PROTOCOL = {
    .on_data = fio___srv_listen_on_data,
    .on_close = fio___srv_listen_on_close,
    .on_timeout = fio___srv_listen_on_timeout,
};

SRV_FUNC int fio_listen FIO_NOOP(struct fio_listen_args args) {
  static int64_t port = 3000;
  if (!args.on_open) {
    FIO_LOG_ERROR("fio_listen missing `on_open` callback.");
    return -1;
  }
  size_t len = args.url ? strlen(args.url) : 0;
  len += (!len) << 4;
  struct fio_listen_args *cpy =
      (struct fio_listen_args *)malloc(sizeof(*cpy) + len);
  FIO_ASSERT_ALLOC(cpy);
  *cpy = args;
  cpy->url = (char *)(cpy + 1);
  if (args.url) {
    FIO_MEMCPY((void *)(cpy->url), args.url, len + 1);
  } else {
    FIO_MEMCPY((void *)(cpy->url), "0.0.0.0:", 8);
    ((char *)(cpy->url))[fio_ltoa((char *)(cpy->url) + 8, (port++), 10) + 8] =
        0;
  }
  int fd = fio_sock_open2(cpy->url, FIO_SOCK_SERVER | FIO_SOCK_TCP);
  FIO_LOG_DEBUG2("Started listening on %s", cpy->url);
  if (fd == -1)
    goto fd_error;
  fio_attach_fd(fd, &FIO___LISTEN_PROTOCOL, (void *)cpy, NULL);
fd_error:
  return 0;
  free(cpy);
  return -1;
}

/* *****************************************************************************
Initializing Server State
***************************************************************************** */
FIO_CONSTRUCTOR(fio___srv) {
  fio_queue_init(fio___srv_tasks);
  fio___srv_init_protocol_test(&MOCK_PROTOCOL);
  fio___srv_init_protocol_test(&FIO___LISTEN_PROTOCOL);
  fio___srvdata.protocols = FIO_LIST_INIT(fio___srvdata.protocols);
  fio___srvdata.tick = fio_time_milli();
}

#endif /* FIO_SERVER_EXTERN_IMP */
#endif /* H___SERVER___H */
