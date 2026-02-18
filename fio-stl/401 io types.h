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
    !defined(H___FIO_IO_TYPES___H) &&                                          \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
#define H___FIO_IO_TYPES___H

/** Use monotonic clock for the IO tick - avoids syscall overhead and NTP skew.
 */
#define FIO___IO_GET_TIME_MILLI() fio_time2milli(fio_time_mono())

/** Sets a flag in io->flag */
#define FIO___IO_FLAG_SET(io, flag_to_set)                                     \
  fio_atomic_or(&(io)->flags, flag_to_set)
/** unsets a flag in io->flag */
#define FIO___IO_FLAG_UNSET(io, flag_to_unset)                                 \
  fio_atomic_and(&(io)->flags, ~(flag_to_unset))

/* *****************************************************************************
IO environment support (`env`)
***************************************************************************** */

/** An object that can be linked to any facil.io connection (fio_s). */
typedef struct {
  void (*on_close)(void *data);
  void *udata;
} fio___io_env_obj_s;

/* unordered `env` dictionary style map */
#define FIO_UMAP_NAME fio___io_env
#define FIO_MAP_KEY_KSTR
#define FIO_MAP_VALUE fio___io_env_obj_s
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
  fio___io_env_s env;
} fio___io_env_safe_s;

#define FIO___IO_ENV_SAFE_INIT                                                 \
  { .lock = FIO_THREAD_MUTEX_INIT, .env = FIO_MAP_INIT }

FIO_IFUNC void *fio___io_env_safe_get(fio___io_env_safe_s *e,
                                      char *key_,
                                      size_t len,
                                      intptr_t type_) {
  void *r;
  fio_str_info_s key = FIO_STR_INFO3(key_, len, 0);
  const uint64_t hash = fio_risky_hash(key_, len, (uint64_t)(type_));
  fio_thread_mutex_lock(&e->lock);
  r = fio___io_env_get(&e->env, hash, key).udata;
  fio_thread_mutex_unlock(&e->lock);
  return r;
}

FIO_IFUNC void fio___io_env_safe_set(fio___io_env_safe_s *e,
                                     char *key_,
                                     size_t len,
                                     intptr_t type_,
                                     fio___io_env_obj_s val,
                                     uint8_t key_is_const) {
  fio_str_info_s key = FIO_STR_INFO3(key_, len, !key_is_const);
  const uint64_t hash = fio_risky_hash(key_, len, (uint64_t)(type_));
  fio_thread_mutex_lock(&e->lock);
  fio___io_env_set(&e->env, hash, key, val, NULL);
  fio_thread_mutex_unlock(&e->lock);
}

FIO_IFUNC int fio___io_env_safe_unset(fio___io_env_safe_s *e,
                                      char *key_,
                                      size_t len,
                                      intptr_t type_) {
  int r;
  fio_str_info_s key = FIO_STR_INFO3(key_, len, 0);
  const uint64_t hash = fio_risky_hash(key_, len, (uint64_t)(type_));
  fio___io_env_obj_s old;
  fio_thread_mutex_lock(&e->lock);
  r = fio___io_env_remove(&e->env, hash, key, &old);
  fio_thread_mutex_unlock(&e->lock);
  return r;
}

FIO_IFUNC int fio___io_env_safe_remove(fio___io_env_safe_s *e,
                                       char *key_,
                                       size_t len,
                                       intptr_t type_) {
  int r;
  fio_str_info_s key = FIO_STR_INFO3(key_, len, 0);
  const uint64_t hash = fio_risky_hash(key_, len, (uint64_t)(type_));
  fio_thread_mutex_lock(&e->lock);
  r = fio___io_env_remove(&e->env, hash, key, NULL);
  fio_thread_mutex_unlock(&e->lock);
  return r;
}

FIO_IFUNC void fio___io_env_safe_destroy(fio___io_env_safe_s *e) {
  fio___io_env_destroy(&e->env); /* no need to lock, performed in IO thread. */
  fio_thread_mutex_destroy(&e->lock);
  *e = (fio___io_env_safe_s)FIO___IO_ENV_SAFE_INIT;
}

/* *****************************************************************************
Protocol Type Initialization
***************************************************************************** */

SFUNC void fio_io_noop(fio_io_s *io) { (void)(io); }

static void fio___io_on_ev_pubsub_mock(struct fio_pubsub_msg_s *msg) {
  (void)(msg);
}
static void fio___io_on_user_mock(fio_io_s *io, void *i_) {
  (void)io, (void)i_;
}
static void fio___io_on_close_mock(void *p1, void *p2) { (void)p1, (void)p2; }

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
/** Sends any unsent internal data. Returns 0 only if all data was sent. */
static void fio___io_func_default_finish(int fd, void *tls) {
  (void)fd, (void)tls;
}
static void fio___io_func_default_cleanup(void *p1) { (void)p1; }

/** Builds a local TLS context out of the fio_io_tls_s object. */
static void *fio___io_func_default_build_context(fio_io_tls_s *tls,
                                                 uint8_t is_client) {
  if (!tls)
    return NULL;
  FIO_ASSERT(0,
             "SSL/TLS `build_context` was called, but no SSL/TLS "
             "implementation found.");
  return NULL;
  (void)tls, (void)is_client;
}
/** Builds a local TLS context out of the fio_io_tls_s object. */
static void fio___io_func_default_free_context(void *context) {
  if (!context)
    return;
  FIO_ASSERT(0,
             "SSL/TLS `free_context` was called, but no SSL/TLS "
             "implementation found.");
  (void)context;
}

static void fio___io_func_free_context_caller_task(void *fn_ptr,
                                                   void *context) {
  union {
    void (*free_context)(void *context);
    void *fn_ptr;
  } u = {.fn_ptr = fn_ptr};
  u.free_context(context);
}

static void fio___io_func_free_context_caller(void (*free_context)(void *),
                                              void *context) {
  union {
    void (*free_context)(void *context);
    void *fn_ptr;
  } u = {.free_context = free_context};
  fio_queue_push(fio_io_queue(),
                 fio___io_func_free_context_caller_task,
                 u.fn_ptr,
                 context);
}

FIO_SFUNC void fio___io_protocol_init(fio_io_protocol_s *pr, _Bool has_tls) {
  pr->reserved.protocols = FIO_LIST_INIT(pr->reserved.protocols);
  pr->reserved.ios = FIO_LIST_INIT(pr->reserved.ios);
  fio_io_functions_s io_fn = {
      .build_context = fio___io_func_default_build_context,
      .free_context = fio___io_func_default_free_context,
      .start = fio_io_noop,
      .read = fio___io_func_default_read,
      .write = fio___io_func_default_write,
      .flush = fio___io_func_default_flush,
      .finish = fio___io_func_default_finish,
      .cleanup = fio___io_func_default_cleanup,
  };
  if (has_tls)
    io_fn = fio_io_tls_default_functions(NULL);
  if (!pr->on_attach)
    pr->on_attach = fio_io_noop;
  if (!pr->on_data)
    pr->on_data = fio_io_suspend;
  if (!pr->on_ready)
    pr->on_ready = fio_io_noop;
  if (!pr->on_close)
    pr->on_close = fio___io_on_close_mock;
  if (!pr->on_shutdown)
    pr->on_shutdown = fio_io_noop;
  if (!pr->on_timeout)
    pr->on_timeout = fio_io_close_now;
  if (!pr->on_pubsub)
    pr->on_pubsub = fio___io_on_ev_pubsub_mock;
  if (!pr->on_user1)
    pr->on_user1 = fio___io_on_user_mock;
  if (!pr->on_user2)
    pr->on_user2 = fio___io_on_user_mock;
  if (!pr->on_user3)
    pr->on_user3 = fio___io_on_user_mock;
  if (!pr->on_reserved)
    pr->on_reserved = fio___io_on_user_mock;
  if (!pr->io_functions.build_context)
    pr->io_functions.build_context = io_fn.build_context;
  if (!pr->io_functions.free_context)
    pr->io_functions.free_context = io_fn.free_context;
  if (!pr->io_functions.start)
    pr->io_functions.start = io_fn.start;
  if (!pr->io_functions.read)
    pr->io_functions.read = io_fn.read;
  if (!pr->io_functions.write)
    pr->io_functions.write = io_fn.write;
  if (!pr->io_functions.flush)
    pr->io_functions.flush = io_fn.flush;
  if (!pr->io_functions.finish)
    pr->io_functions.finish = io_fn.finish;
  if (!pr->io_functions.cleanup)
    pr->io_functions.cleanup = io_fn.cleanup;
  if (!pr->timeout)
    pr->timeout = FIO_IO_TIMEOUT_MAX;
  /* round up to nearest 16 byte size */
  pr->buffer_size = ((pr->buffer_size + 15ULL) & (~15ULL));
}

/* the FIO___MOCK_PROTOCOL is used to manage hijacked / zombie connections. */
static fio_io_protocol_s FIO___IO_MOCK_PROTOCOL;

FIO_IFUNC void fio___io_protocol_init_test(fio_io_protocol_s *pr,
                                           _Bool has_tls) {
  if (!fio_atomic_or(&pr->reserved.flags, 1))
    fio___io_protocol_init(pr, has_tls);
}

/* *****************************************************************************
IO Reactor State Machine
***************************************************************************** */

#define FIO___IO_FLAG_WAKEUP   (1U)
#define FIO___IO_FLAG_CYCLING  (2U)
#define FIO___IO_FLAG_TICK_SET (4U)

typedef struct {
  FIO_LIST_NODE node;
  fio_thread_pid_t pid;
  volatile unsigned done;
  volatile unsigned stop;
} fio___io_pid_s;

static struct FIO___IO_S {
  fio_poll_s poll;
  int64_t tick;
  int64_t time_ms;
  fio_queue_s queue;
  uint32_t flags;
  uint16_t workers;
  uint8_t is_worker;
  volatile uint8_t stop;
  fio_timer_queue_s timer;
  int restart_signal;
  int wakeup_fd;
  fio_thread_pid_t root_pid;
  fio_thread_pid_t pid;
  fio___io_env_safe_s env;
  FIO_LIST_NODE protocols;
  FIO_LIST_NODE async;
  FIO_LIST_NODE pids;
  uint32_t to_spawn;
  fio_io_s *wakeup;
  FIO___LOCK_TYPE lock;
  size_t shutdown_timeout;
} FIO___IO = {
    .tick = 0,
    .wakeup_fd = -1,
    .stop = 1,
    .lock = FIO___LOCK_INIT,
    .shutdown_timeout = FIO_IO_SHUTDOWN_TIMEOUT,
};

FIO_IFUNC void fio___io_defer_no_wakeup(void (*task)(void *, void *),
                                        void *udata1,
                                        void *udata2) {
  fio_queue_push(&FIO___IO.queue, task, udata1, udata2);
}

FIO_SFUNC void fio___io_wakeup(void);
void fio_io_defer___(void);
/** Schedules a task for delayed execution. This function is thread-safe. */
SFUNC void fio_io_defer FIO_NOOP(void (*task)(void *, void *),
                                 void *udata1,
                                 void *udata2) {
  fio_queue_push(&FIO___IO.queue, task, udata1, udata2);
  fio___io_wakeup();
}

void fio_io_run_every___(void);
/** Schedules a timer bound task, see `fio_timer_schedule`. */
SFUNC void fio_io_run_every FIO_NOOP(fio_timer_schedule_args_s args) {
  args.start_at = FIO___IO.tick;
  fio_timer_schedule FIO_NOOP(&FIO___IO.timer, args);
}

/** Returns a pointer for the IO reactor's queue. */
SFUNC fio_queue_s *fio_io_queue(void) { return &FIO___IO.queue; }

/** Stopping the IO reactor. */
SFUNC void fio_io_stop(void) { fio_atomic_or_fetch(&FIO___IO.stop, 1); }

/** Returns current process id. */
SFUNC int fio_io_pid(void) { return FIO___IO.pid; }

/** Returns the root / master process id. */
SFUNC int fio_io_root_pid(void) { return FIO___IO.root_pid; }

/** Returns true if running and 0 if stopped or shutting down. */
SFUNC int fio_io_is_running(void) { return !FIO___IO.stop; }

/** Returns true if the current process is the master process. */
SFUNC int fio_io_is_master(void) { return FIO___IO.root_pid == FIO___IO.pid; }

/** Returns true if the current process is a worker process. */
SFUNC int fio_io_is_worker(void) { return FIO___IO.is_worker; }

FIO_SFUNC void fio___io_last_tick_update(void *ignr_1, void *ignr_2) {
  FIO___IO_FLAG_UNSET(&FIO___IO, FIO___IO_FLAG_TICK_SET);
  FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  FIO___IO.time_ms = fio_time2milli(fio_time_real());
  (void)ignr_1, (void)ignr_2;
}

/** Returns the last millisecond when the polled for IO events. */
SFUNC int64_t fio_io_last_tick(void) {
  if (!(FIO___IO_FLAG_SET(&FIO___IO, FIO___IO_FLAG_TICK_SET) &
        FIO___IO_FLAG_TICK_SET))
    fio___io_defer_no_wakeup(fio___io_last_tick_update, NULL, NULL);
  return FIO___IO.tick;
}

SFUNC int64_t fio_io_last_tick_time(void) { return FIO___IO.time_ms; }

/** Sets a signal to listen to for a hot restart (see `fio_io_restart`). */
SFUNC void fio_io_restart_on_signal(int signal) {
  FIO___IO.restart_signal = signal;
}

/** Returns the shutdown timeout for the reactor. */
SFUNC size_t fio_io_shutdown_timeout(void) { return FIO___IO.shutdown_timeout; }

/** Sets the shutdown timeout for the reactor, returning the new value. */
SFUNC size_t fio_io_shutdown_timeout_set(size_t milliseconds) {
  return (FIO___IO.shutdown_timeout = milliseconds);
}

/* *****************************************************************************
IO Type
***************************************************************************** */

#define FIO___IO_FLAG_OPEN         ((uint32_t)1U)
#define FIO___IO_FLAG_SUSPENDED    ((uint32_t)2U)
#define FIO___IO_FLAG_THROTTLED    ((uint32_t)4U)
#define FIO___IO_FLAG_CLOSE        ((uint32_t)8U)
#define FIO___IO_FLAG_CLOSE_REMOTE ((uint32_t)16U)
#define FIO___IO_FLAG_CLOSE_ERROR  ((uint32_t)32U)
#define FIO___IO_FLAG_CLOSED_ALL                                               \
  (FIO___IO_FLAG_CLOSE | FIO___IO_FLAG_CLOSE_REMOTE | FIO___IO_FLAG_CLOSE_ERROR)
#define FIO___IO_FLAG_TOUCH       ((uint32_t)64U)
#define FIO___IO_FLAG_WRITE_SCHD  ((uint32_t)128U)
#define FIO___IO_FLAG_POLLIN_SET  ((uint32_t)256U)
#define FIO___IO_FLAG_POLLOUT_SET ((uint32_t)512U)
#define FIO___IO_FLAG_WRITE_DIRTY ((uint32_t)1024U)

#define FIO___IO_FLAG_PREVENT_ON_DATA                                          \
  (FIO___IO_FLAG_SUSPENDED | FIO___IO_FLAG_THROTTLED)

#define FIO___IO_FLAG_POLL_SET                                                 \
  (FIO___IO_FLAG_POLLIN_SET | FIO___IO_FLAG_POLLOUT_SET)

static void fio___io_poll_on_data_schd(void *io);
static void fio___io_poll_on_ready_schd(void *io);
static void fio___io_poll_on_close_schd(void *io);
FIO_IFUNC void fio___io_free_with_flush(fio_io_s *io);

/** The main IO object type. Should be treated as an opaque pointer. */
struct fio_io_s {
  int fd;
  uint32_t flags;
  FIO_LIST_NODE node;
  void *udata;
  void *tls;
  fio_io_protocol_s *pr;
  fio_stream_s out;
  fio___io_env_safe_s env;
#if FIO_IO_COUNT_STORAGE
  size_t total_sent;
  size_t total_recieved;
#endif
  int64_t active;
};

FIO_IFUNC void fio___io_monitor_in(fio_io_s *io) {
  // FIO_LOG_DDEBUG2("(%d) IO monitoring Input for %d (called)",
  //                 fio_io_pid(),
  //                 io->fd);
  if (io->flags & (FIO___IO_FLAG_PREVENT_ON_DATA | FIO___IO_FLAG_CLOSED_ALL))
    return;
  if ((FIO___IO_FLAG_SET(io, FIO___IO_FLAG_POLLIN_SET) &
       FIO___IO_FLAG_POLLIN_SET)) {
    return;
  }
  fio_poll_monitor(&FIO___IO.poll, io->fd, (void *)io, POLLIN);
  // FIO_LOG_DDEBUG2("(%d) IO monitoring Input for %d", fio_io_pid(), io->fd);
}
FIO_IFUNC void fio___io_monitor_out(fio_io_s *io) {
  // FIO_LOG_DDEBUG2("(%d) IO monitoring Output for %d (called)",
  //                 fio_io_pid(),
  //                 io->fd);
  if (io->flags & FIO___IO_FLAG_WRITE_SCHD)
    return;
  if ((FIO___IO_FLAG_SET(io, FIO___IO_FLAG_POLLOUT_SET) &
       FIO___IO_FLAG_POLLOUT_SET))
    return;
  fio_poll_monitor(&FIO___IO.poll, io->fd, (void *)io, POLLOUT);
  // FIO_LOG_DDEBUG2("(%d) IO monitoring Output for %d", fio_io_pid(), io->fd);
}

FIO_IFUNC void fio___io_monitor_forget(fio_io_s *io) {
  // FIO_LOG_DDEBUG2("(%d) IO monitoring Removed for %d (called)",
  //                 fio_io_pid(),
  //                 io->fd);
  if (!(FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_POLL_SET) &
        FIO___IO_FLAG_POLL_SET))
    return;
  fio_poll_forget(&FIO___IO.poll, io->fd);
  // FIO_LOG_DDEBUG2("(%d) IO monitoring Removed for %d", fio_io_pid(), io->fd);
}

FIO_SFUNC void fio___io_destroy(fio_io_s *io) {
  fio_io_protocol_s *pr = io->pr;
  FIO_LIST_REMOVE(&io->node);
#if FIO_IO_COUNT_STORAGE
  FIO_LOG_DDEBUG2(
      "(%d) detaching and destroying %p (fd %d): %zu/%zu bytes received/sent",
      FIO___IO.pid,
      (void *)io,
      io->fd,
      io->total_recieved,
      io->total_sent);
#else
  FIO_LOG_DDEBUG2("(%d) detaching and destroying %p (fd %d).",
                  FIO___IO.pid,
                  (void *)io,
                  io->fd);
#endif
  /* store info, as it might be freed if the protocol is freed. */
  if (FIO_LIST_IS_EMPTY(&io->pr->reserved.ios))
    FIO_LIST_REMOVE_RESET(&io->pr->reserved.protocols);
  /* call on_stop / free callbacks . */
  pr->io_functions.cleanup(io->tls);
  pr->on_close((void *)(io + 1), io->udata);
  fio___io_env_safe_destroy(&io->env);
  fio_sock_close(io->fd);
  fio_stream_destroy(&io->out);
  fio___io_monitor_forget(io);
  FIO_LOG_DDEBUG2("(%d) IO closed and destroyed for fd %d",
                  fio_io_pid(),
                  io->fd);
}

#define FIO_REF_NAME            fio___io
#define FIO_REF_TYPE            fio_io_s
#define FIO_REF_FLEX_TYPE       uint8_t
#define FIO_REF_DESTROY(io)     fio___io_destroy(&io)
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

FIO_SFUNC void fio___io_protocol_set(void *io_, void *pr_) {
  fio_io_s *io = (fio_io_s *)io_;
  fio_io_protocol_s *pr = (fio_io_protocol_s *)pr_;
  fio_io_protocol_s *old = io->pr;
  if (!pr)
    pr = &FIO___IO_MOCK_PROTOCOL;
  fio___io_protocol_init_test(pr, (io->tls != NULL));
  FIO_LIST_REMOVE(&io->node);
  if (FIO_LIST_IS_EMPTY(&old->reserved.ios))
    FIO_LIST_REMOVE_RESET(&old->reserved.protocols);
  if (FIO_LIST_IS_EMPTY(&pr->reserved.ios))
    FIO_LIST_PUSH(&FIO___IO.protocols, &pr->reserved.protocols);
  FIO_LIST_PUSH(&pr->reserved.ios, &io->node);
  io->pr = pr;
  FIO_LOG_DDEBUG2("(%d) protocol set for IO with fd %d",
                  fio_io_pid(),
                  fio_io_fd(io));
  pr->on_attach(io);
  /* avoid calling `start` and setting `on_ready` more than once */
  if (old == &FIO___IO_MOCK_PROTOCOL) {
    pr->io_functions.start(io);
    fio___io_monitor_out(io);
  }
  fio___io_monitor_in(io);
  fio___io_free_with_flush(io);
}

/** Performs a task for each IO in the stated protocol. */
SFUNC size_t fio_io_protocol_each(fio_io_protocol_s *protocol,
                                  void (*task)(fio_io_s *, void *udata2),
                                  void *udata2) {
  size_t count = 0;
  if (!protocol || !protocol->reserved.protocols.next)
    return count;
  FIO_LIST_EACH(fio_io_s, node, &protocol->reserved.ios, io) {
    task(io, udata2);
    ++count;
  }
  return count;
}

/* Attaches the socket in `fd` to the facio.io engine (reactor). */
SFUNC fio_io_s *fio_io_attach_fd(int fd,
                                 fio_io_protocol_s *pr,
                                 void *udata,
                                 void *tls) {
  fio_io_s *io = NULL;
  fio_io_protocol_s cpy;
  if (fd == -1)
    goto error;
  io = fio___io_new2(pr->buffer_size);
  *io = (fio_io_s){
      .fd = fd,
      .flags = FIO___IO_FLAG_OPEN,
      .pr = &FIO___IO_MOCK_PROTOCOL,
      .node = FIO_LIST_INIT(io->node),
      .udata = udata,
      .tls = tls,
      .active = FIO___IO.tick,
  };
  fio_sock_set_non_block(fd);
  FIO_LOG_DDEBUG2("(%d) attaching fd %d to IO object %p (%zu bytes buffer)",
                  fio_io_pid(),
                  fd,
                  (void *)io,
                  fio_io_buffer_len(io));
  fio_io_defer(fio___io_protocol_set, (void *)fio___io_dup2(io), (void *)pr);
  return io;

error:
  cpy = *pr;
  cpy.on_close(NULL, udata);
  cpy.io_functions.cleanup(tls);
  return io;
}

/** Sets a new protocol object. `NULL` is a valid "only-write" protocol. */
SFUNC fio_io_protocol_s *fio_io_protocol_set(fio_io_s *io,
                                             fio_io_protocol_s *pr) {
  uintptr_t old_flags;
  if (!io)
    goto init;
  fio_io_defer(fio___io_protocol_set, (void *)fio___io_dup2(io), (void *)pr);
  return pr;
init:
  old_flags = pr->reserved.flags;
  fio___io_protocol_init_test(pr, 0);
  pr->reserved.flags = old_flags;
  return pr;
}

/**
 * Returns a pointer to the current protocol object.
 *
 * If `protocol` wasn't properly set, the pointer might be NULL or invalid.
 *
 * If `protocol` wasn't attached yet, may return the previous protocol.
 */
IFUNC fio_io_protocol_s *fio_io_protocol(fio_io_s *io) { return io->pr; }

/** Returns the a pointer to the memory buffer required by the protocol. */
IFUNC void *fio_io_buffer(fio_io_s *io) { return (void *)(io + 1); }

/** Returns the length of the `buffer` buffer. */
IFUNC size_t fio_io_buffer_len(fio_io_s *io) {
  return fio___io_metadata_flex_len(io);
}

/** Associates a new `udata` pointer with the IO, returning the old `udata` */
FIO_DEF_SET_FUNC(IFUNC, fio_io, fio_io_s, void *, udata, FIO_NOOP_FN)

/** Returns the `udata` pointer associated with the IO. */
IFUNC void *fio_io_udata(fio_io_s *io) { return io->udata; }

/** Associates a new `tls` pointer with the IO, returning the old `tls` */
IFUNC void *fio_io_tls_set(fio_io_s *io, void *tls) {
  void *old = io->tls;
  io->tls = tls;
  return old;
}

/** Returns the `tls` pointer associated with the IO. */
IFUNC void *fio_io_tls(fio_io_s *io) { return io->tls; }

/** Returns the socket file descriptor (fd) associated with the IO. */
IFUNC int fio_io_fd(fio_io_s *io) { return io->fd; }

FIO_SFUNC void fio___io_touch(void *io_, void *ignr_) {
  fio_io_s *io = (fio_io_s *)io_;
  fio_atomic_and(&io->flags, ~FIO___IO_FLAG_TOUCH);
  io->active = FIO___IO.tick;
  FIO_LIST_REMOVE(&io->node); /* timeout IO ordering */
  FIO_LIST_PUSH(&io->pr->reserved.ios, &io->node);
  fio___io_free2(io);
  (void)ignr_;
}

/* Resets a socket's timeout counter. */
SFUNC void fio_io_touch(fio_io_s *io) {
  if (!(fio_atomic_or(&io->flags, FIO___IO_FLAG_TOUCH) & FIO___IO_FLAG_TOUCH))
    fio_queue_push_urgent(&FIO___IO.queue, fio___io_touch, fio___io_dup2(io));
}

/**
 * Reads data to the buffer, if any data exists. Returns the number of bytes
 * read.
 *
 * NOTE: zero (`0`) is a valid return value meaning no data was available.
 */
SFUNC size_t fio_io_read(fio_io_s *io, void *buf, size_t len) {
  if (!io)
    return 0;
  ssize_t r = io->pr->io_functions.read(io->fd, buf, len, io->tls);
  if (r > 0) {
#if FIO_IO_COUNT_STORAGE
    io->total_recieved += r;
#endif
    fio_io_touch(io);
    return r;
  }
  if ((unsigned)(!len) |
      ((unsigned)(r == -1) & ((unsigned)(errno == EAGAIN) |
                              (errno == EWOULDBLOCK) | (errno == EINTR))))
    return 0;
  fio_io_close(io);
  return 0;
}

FIO_SFUNC void fio___io_write2_dealloc_task(void *fn, void *data) {
  union {
    void *ptr;
    void (*fn)(void *);
  } u = {.ptr = fn};
  u.fn(data);
}

FIO_SFUNC void fio___io_write2(void *io_, void *packet_) {
  fio_io_s *io = (fio_io_s *)io_;
  fio_stream_packet_s *packet = (fio_stream_packet_s *)packet_;
  if (!(io->flags & FIO___IO_FLAG_OPEN))
    goto io_closed;

  fio_stream_add(&io->out, packet);
  fio___io_poll_on_ready_schd((void *)io);
  fio___io_free2(io);
  return;

io_closed:
  FIO_LOG_DEBUG2("(%d) write task to closed IO %d failed (task too late).",
                 fio_io_pid(),
                 fio_io_fd(io));
  fio_stream_packet_free(packet);
  fio___io_free2(io);
}

void fio_io_write2___(void);
/**
 * Writes data to the outgoing buffer and schedules the buffer to be sent.
 */
SFUNC void fio_io_write2 FIO_NOOP(fio_io_s *io, fio_io_write_args_s args) {
  fio_stream_packet_s *packet = NULL;
  if (!io)
    goto io_error_null;
  if (args.buf) {
    packet = fio_stream_pack_data(args.buf,
                                  args.len,
                                  args.offset,
                                  args.copy,
                                  args.dealloc);
  } else if ((unsigned)(args.fd + 1) > 1) {
    packet = fio_stream_pack_fd((int)args.fd, args.len, args.offset, args.copy);
  } else /* fio_io_write2 called without data */
    goto do_nothing;
  if (!packet)
    goto error;
  if ((io->flags & FIO___IO_FLAG_CLOSE))
    goto write_called_after_close;
  FIO___IO_FLAG_SET(io, FIO___IO_FLAG_WRITE_DIRTY);
  fio___io_defer_no_wakeup(fio___io_write2,
                           (void *)fio___io_dup2(io),
                           (void *)packet);
  return;

error: /* note: `dealloc` already called by the `fio_stream` error handler. */
  FIO_LOG_ERROR(
      "(%d) couldn't create %zu bytes long user-packet for IO %p (%d)",
      fio_io_pid(),
      args.len,
      (void *)io,
      (io ? io->fd : -1));
  return;

write_called_after_close:
  FIO_LOG_DEBUG2("(%d) `write` called after `close` was called for IO.",
                 fio_io_pid());
  {
    union {
      void *ptr;
      void (*fn)(fio_stream_packet_s *);
    } u = {.fn = fio_stream_pack_free};
    fio___io_defer_no_wakeup(fio___io_write2_dealloc_task, u.ptr, packet);
  }
  return;

io_error_null:
  FIO_LOG_ERROR("(%d) `fio_write2` called for invalid IO (NULL)", FIO___IO.pid);
do_nothing:
  if (args.dealloc) {
    union {
      void *ptr;
      void (*fn)(void *);
    } u = {.fn = args.dealloc};
    fio___io_defer_no_wakeup(fio___io_write2_dealloc_task, u.ptr, args.buf);
    if ((unsigned)(args.fd + 1) > 1)
      close((int)args.fd);
  }
}

/** Marks the IO for closure as soon as scheduled data was sent. */
SFUNC void fio_io_close(fio_io_s *io) {
  if (io && (io->flags & FIO___IO_FLAG_OPEN) &&
      !(FIO___IO_FLAG_SET(io, FIO___IO_FLAG_CLOSE) & FIO___IO_FLAG_CLOSE)) {
    FIO_LOG_DDEBUG2("(%d) scheduling IO %p (fd %d) for closure",
                    fio_io_pid(),
                    (void *)io,
                    io->fd);
    fio___io_poll_on_ready_schd((void *)io);
  }
}

/** Marks the IO for immediate closure. */
SFUNC void fio_io_close_now(fio_io_s *io) {
  if (!io)
    return;
  FIO_LOG_DDEBUG2("(%d) pre-destruction close called for fd %d",
                  fio_io_pid(),
                  fio_io_fd(io));
  FIO___IO_FLAG_SET(io, FIO___IO_FLAG_CLOSE);
  if ((FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_OPEN) & FIO___IO_FLAG_OPEN))
    fio_io_free(io);
}

/**
 * Increases a IO's reference count, so it won't be automatically destroyed
 * when all tasks have completed.
 *
 * Use this function in order to use the IO outside of a scheduled task.
 *
 * This function is thread-safe.
 */
SFUNC fio_io_s *fio_io_dup(fio_io_s *io) { return fio___io_dup2(io); }

SFUNC void fio___io_free_task(void *io_, void *ignr_) {
  fio___io_free2((fio_io_s *)io_);
  (void)ignr_;
}
/** Free IO (reference) - thread-safe, flushes pending writes. */
SFUNC void fio_io_free(fio_io_s *io) {
  if (FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_WRITE_DIRTY) &
      FIO___IO_FLAG_WRITE_DIRTY) {
    fio___io_poll_on_ready_schd((void *)io);
    fio___io_wakeup();
  }
  fio___io_defer_no_wakeup(fio___io_free_task, (void *)io, NULL);
}

/** IO-thread free that flushes dirty writes (schedules on_ready if needed). */
FIO_IFUNC void fio___io_free_with_flush(fio_io_s *io) {
  if (FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_WRITE_DIRTY) &
      FIO___IO_FLAG_WRITE_DIRTY) {
    fio___io_poll_on_ready_schd((void *)io);
  }
  fio___io_free2(io);
}

/** Suspends future "on_data" events for the IO. */
SFUNC void fio_io_suspend(fio_io_s *io) {
  FIO___IO_FLAG_SET(io, FIO___IO_FLAG_SUSPENDED);
}

SFUNC void fio___io_unsuspend(void *io_, void *ignr_) {
  fio_io_s *io = (fio_io_s *)io_;
  if (FIO___IO.stop)
    fio_io_close(io);
  else
    fio___io_monitor_in(io);
  return;
  (void)ignr_;
}

/** Listens for future "on_data" events related to the IO. */
SFUNC void fio_io_unsuspend(fio_io_s *io) {
  if ((FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_SUSPENDED) &
       FIO___IO_FLAG_SUSPENDED))
    fio_io_defer(fio___io_unsuspend, (void *)io, NULL);
}

/** Returns 1 if the IO handle was suspended. */
SFUNC int fio_io_is_suspended(fio_io_s *io) {
  return (int)((io->flags & FIO___IO_FLAG_SUSPENDED) / FIO___IO_FLAG_SUSPENDED);
}

/** Returns 1 if the IO handle is marked as open. */
SFUNC int fio_io_is_open(fio_io_s *io) {
  return (int)((io->flags & FIO___IO_FLAG_OPEN) / FIO___IO_FLAG_OPEN);
}

/** Returns the approximate number of bytes in the outgoing buffer. */
SFUNC size_t fio_io_backlog(fio_io_s *io) {
  return fio_stream_length(&io->out);
}

/* *****************************************************************************
Connection Object Links / Environment
***************************************************************************** */

void *fio_io_env_get___(void); /* IDE Marker */
/** Returns the named `udata` associated with the IO object (or `NULL`). */
SFUNC void *fio_io_env_get FIO_NOOP(fio_io_s *io, fio_io_env_get_args_s a) {
  fio___io_env_safe_s *e = io ? &io->env : &FIO___IO.env;
  return fio___io_env_safe_get(e, a.name.buf, a.name.len, a.type);
}

void fio_io_env_set___(void); /* IDE Marker */
/** Links an object to a connection's lifetime / environment. */
SFUNC void fio_io_env_set FIO_NOOP(fio_io_s *io, fio_io_env_set_args_s a) {
  fio___io_env_safe_s *e = io ? &io->env : &FIO___IO.env;
  fio___io_env_safe_set(e,
                        a.name.buf,
                        a.name.len,
                        a.type,
                        (fio___io_env_obj_s){.on_close = a.on_close, a.udata},
                        a.const_name);
}

int fio_io_env_unset___(void); /* IDE Marker */
/** Un-links an object from the connection's lifetime, so it's `on_close` */
SFUNC int fio_io_env_unset FIO_NOOP(fio_io_s *io, fio_io_env_get_args_s a) {
  fio___io_env_safe_s *e = io ? &io->env : &FIO___IO.env;
  return fio___io_env_safe_unset(e, a.name.buf, a.name.len, a.type);
}

int fio_io_env_remove___(void); /* IDE Marker */
/**
 * Removes an object from the connection's lifetime / environment, calling it's
 * `on_close` callback as if the connection was closed.
 */
SFUNC int fio_io_env_remove FIO_NOOP(fio_io_s *io, fio_io_env_get_args_s a) {
  fio___io_env_safe_s *e = io ? &io->env : &FIO___IO.env;
  return fio___io_env_safe_remove(e, a.name.buf, a.name.len, a.type);
}

/* *****************************************************************************
Event handling
***************************************************************************** */

static void fio___io_poll_on_data(void *io_, void *ignr_) {
  (void)ignr_;
  fio_io_s *io = (fio_io_s *)io_;
  FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_POLLIN_SET);
  if (!(io->flags & FIO___IO_FLAG_PREVENT_ON_DATA)) {
    /* this also tests for the suspended / throttled flags, allows closed */
    io->pr->on_data(io);
    fio___io_monitor_in(io);
  } else if ((io->flags & FIO___IO_FLAG_OPEN)) {
    fio___io_monitor_out(io);
  }
  fio___io_free_with_flush(io);
  return;
}

static void fio___io_poll_on_ready(void *io_, void *ignr_) {
  (void)ignr_;
#if defined(DEBUG) && DEBUG
  errno = 0;
#endif
  fio_io_s *io = (fio_io_s *)io_;
  char buf_mem[FIO_IO_BUFFER_PER_WRITE];
  size_t total = 0;
  FIO___IO_FLAG_UNSET(io,
                      (FIO___IO_FLAG_POLLOUT_SET | FIO___IO_FLAG_WRITE_SCHD));
  // FIO_LOG_DDEBUG2("(%d) poll_on_ready callback for fd %d",
  //                 fio_io_pid(),
  //                 fio_io_fd(io));
  if (!(io->flags & FIO___IO_FLAG_OPEN))
    goto finish;
  for (;;) {
    ssize_t r = io->pr->io_functions.flush(io->fd, io->tls);
    size_t len = FIO_IO_BUFFER_PER_WRITE;
    char *buf = buf_mem;
    if ((!r)) {
      fio_stream_read(&io->out, &buf, &len);
      if (!len)
        goto finish_loop;
      r = io->pr->io_functions.write(io->fd, buf, len, io->tls);
    }
    switch ((size_t)(r + 1)) {
    case 0:
      if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
        goto finish_loop;
      if (errno == EINTR)
        continue;
      /* fallthrough */
    case 1: goto connection_error;
    default:
      FIO_LOG_DDEBUG2("(%d) written %zu bytes to fd %d",
                      FIO___IO.pid,
                      (size_t)r,
                      io->fd);
      total += r;
      fio_stream_advance(&io->out, r);
      continue;
    }
  }
finish_loop:
  if (total) {
    fio___io_touch((void *)fio___io_dup2(io), NULL);
#if FIO_IO_COUNT_STORAGE
    io->total_sent += total;
#endif
  }
  if (fio_stream_any(&io->out)) {
    if (fio_stream_length(&io->out) >= FIO_IO_THROTTLE_LIMIT) {
      if (!(io->flags & FIO___IO_FLAG_THROTTLED))
        FIO_LOG_DDEBUG2("(%d), throttled IO %p (fd %d)",
                        FIO___IO.pid,
                        (void *)io,
                        io->fd);
      FIO___IO_FLAG_SET(io, FIO___IO_FLAG_THROTTLED);
    }
    fio___io_monitor_out(io);
  } else if ((io->flags & FIO___IO_FLAG_CLOSE)) {
    io->pr->io_functions.finish(io->fd, io->tls);
    fio_io_close_now(io);
  } else {
    if ((io->flags & FIO___IO_FLAG_THROTTLED)) {
      FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_THROTTLED);
      fio___io_monitor_in(io);
    }
    // FIO_LOG_DDEBUG2("(%d) calling on_ready for %p (fd %d) - %zu data left.",
    //                 FIO___IO.pid,
    //                 (void *)io,
    //                 io->fd,
    //                 fio_stream_length(&io->out));
    io->pr->on_ready(io);
  }

finish:
  fio___io_free_with_flush(io);
  return;

connection_error:
#if defined(DEBUG) && DEBUG
  if (fio_stream_any(&io->out))
    FIO_LOG_DERROR(
        "(%d) IO write failed (%d), disconnecting: %p (fd %d)\n\tError: %s",
        FIO___IO.pid,
        errno,
        (void *)io,
        io->fd,
        strerror(errno));
#endif
  fio_io_close_now(io);
  fio___io_free_with_flush(io);
}

// static void fio___io_poll_on_close_task(void *io_, void *ignr_) {
//   (void)ignr_;
//   fio_io_s *io = (fio_io_s *)io_;
//   fio_io_close_now(io);
//   fio___io_free2(io);
// }

static void fio___io_poll_on_close(void *io_, void *ignr_) {
  (void)ignr_;
  fio_io_s *io = (fio_io_s *)io_;
  if (!(io->flags & FIO___IO_FLAG_CLOSE)) {
    FIO___IO_FLAG_SET(io, FIO___IO_FLAG_CLOSE_REMOTE);
    FIO_LOG_DEBUG2("(%d) fd %d closed by remote peer", FIO___IO.pid, io->fd);
  }
  /* allow on_data tasks to complete before closing? */
  fio_io_close_now(io);
  fio___io_free2(io);
}

static void fio___io_poll_on_timeout(void *io_, void *ignr_) {
  (void)ignr_;
  fio_io_s *io = (fio_io_s *)io_;
  io->pr->on_timeout(io);
  fio___io_free_with_flush(io);
}

/* *****************************************************************************
Event scheduling
***************************************************************************** */

static void fio___io_poll_on_data_schd(void *io) {
  // FIO_LOG_DDEBUG2("(%d) `on_data` scheduled for fd %d.",
  //                 fio_io_pid(),
  //                 fio_io_fd((fio_io_s *)io));
  // FIO___IO_FLAG_POLLIN_SET
  fio___io_defer_no_wakeup(fio___io_poll_on_data,
                           (void *)fio___io_dup2((fio_io_s *)io),
                           NULL);
}
static void fio___io_poll_on_ready_schd(void *io) {
  if (!(FIO___IO_FLAG_SET((fio_io_s *)io, FIO___IO_FLAG_WRITE_SCHD) &
        FIO___IO_FLAG_WRITE_SCHD)) {
    // FIO_LOG_DDEBUG2("(%d) `on_ready` scheduled for fd %d.",
    //                 fio_io_pid(),
    //                 fio_io_fd((fio_io_s *)io));
    fio___io_defer_no_wakeup(fio___io_poll_on_ready,
                             (void *)fio___io_dup2((fio_io_s *)io),
                             NULL);
  }
}
static void fio___io_poll_on_close_schd(void *io) {
  // FIO_LOG_DDEBUG2("(%d) remote closure for fd %d.",
  //                 fio_io_pid(),
  //                 fio_io_fd((fio_io_s *)io));
  fio___io_defer_no_wakeup(fio___io_poll_on_close,
                           (void *)fio___io_dup2((fio_io_s *)io),
                           NULL);
}

/* *****************************************************************************
Timeout Review
***************************************************************************** */

/** Schedules the timeout event for any timed out IO object */
static int fio___io_review_timeouts(void) {
  int c = 0;
  static time_t last_to_review = 0;
  /* test timeouts at whole second intervals */
  if (last_to_review + 1000 > FIO___IO.tick)
    return c;
  last_to_review = FIO___IO.tick;
  const int64_t now_milli = FIO___IO.tick;

  FIO_LIST_EACH(fio_io_protocol_s,
                reserved.protocols,
                &FIO___IO.protocols,
                pr) {
    FIO_ASSERT_DEBUG(pr->reserved.flags, "protocol object flags unmarked?!");
    if (!pr->timeout || pr->timeout > FIO_IO_TIMEOUT_MAX)
      pr->timeout = FIO_IO_TIMEOUT_MAX;
    int64_t limit = now_milli - ((int64_t)pr->timeout);
    FIO_LIST_EACH(fio_io_s, node, &pr->reserved.ios, io) {
      FIO_ASSERT_DEBUG(io->pr == pr, "IO protocol ownership error");
      if (io->active >= limit)
        break;
      // FIO_LOG_DDEBUG2("(%d) scheduling timeout for %p (fd %d)",
      //                 FIO___IO.pid,
      //                 (void *)io,
      //                 io->fd);
      fio___io_defer_no_wakeup(fio___io_poll_on_timeout,
                               (void *)fio___io_dup2(io),
                               NULL);
      ++c;
    }
  }
  return c;
}

/* *****************************************************************************
Wakeup Protocol
***************************************************************************** */

FIO_SFUNC void fio___io_wakeup_cb(fio_io_s *io) {
  char buf[512];
  ssize_t r = fio_sock_read(fio_io_fd(io), buf, 512);
  (void)r;
  FIO_LOG_DDEBUG2("(%d) fio___io_wakeup called", FIO___IO.pid);
  FIO___IO_FLAG_UNSET(&FIO___IO, FIO___IO_FLAG_WAKEUP);
}
FIO_SFUNC void fio___io_wakeup_on_close(void *ignr1_, void *ignr2_) {
  fio_sock_close(FIO___IO.wakeup_fd);
  FIO___IO.wakeup = NULL;
  FIO___IO.wakeup_fd = -1;
  FIO_LOG_DDEBUG2("(%d) fio___io_wakeup destroyed", FIO___IO.pid);
  (void)ignr1_, (void)ignr2_;
}

FIO_SFUNC void fio___io_wakeup(void) {
  if (!FIO___IO.wakeup || (FIO___IO_FLAG_SET(&FIO___IO, FIO___IO_FLAG_WAKEUP) &
                           FIO___IO_FLAG_WAKEUP))
    return;
  char buf[1] = {(char)~0};
  ssize_t ignr = fio_sock_write(FIO___IO.wakeup_fd, buf, 1);
  (void)ignr;
}

static fio_io_protocol_s FIO___IO_WAKEUP_PROTOCOL = {
    .on_data = fio___io_wakeup_cb,
    .on_close = fio___io_wakeup_on_close,
    .on_timeout = fio_io_touch,
};

FIO_SFUNC void fio___io_wakeup_init(void) {
  if (FIO___IO.wakeup)
    return;
  int fds[2];
  if (pipe(fds)) {
    FIO_LOG_ERROR("(%d) couldn't open wakeup pipes, fio___io_wakeup disabled.",
                  FIO___IO.pid);
    return;
  }
  fio_sock_set_non_block(fds[0]);
  fio_sock_set_non_block(fds[1]);
  FIO___IO.wakeup_fd = fds[1];
  FIO___IO.wakeup = fio_io_attach_fd(fds[0],
                                     &FIO___IO_WAKEUP_PROTOCOL,
                                     (void *)(uintptr_t)fds[1],
                                     NULL);
  FIO_LOG_DDEBUG2("(%d) fio___io_wakeup initialized", FIO___IO.pid);
}

/* *****************************************************************************
TLS Context Type and Helpers
***************************************************************************** */

typedef struct {
  fio_keystr_s nm;
  void (*fn)(fio_io_s *);
} fio___io_tls_alpn_s;

typedef struct {
  fio_keystr_s nm;
  fio_keystr_s public_cert_file;
  fio_keystr_s private_key_file;
  fio_keystr_s pk_password;
} fio___io_tls_cert_s;

typedef struct {
  fio_keystr_s nm;
} fio___io_tls_trust_s;

#undef FIO_TYPEDEF_IMAP_REALLOC
#undef FIO_TYPEDEF_IMAP_FREE
#undef FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE
#define FIO_TYPEDEF_IMAP_REALLOC(p, size_old, size, copy) realloc(p, size)
#define FIO_TYPEDEF_IMAP_FREE(ptr, len)                   free(ptr)
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE                  0

#define FIO___IO_ALPN_HASH(o)   ((uint16_t)fio_keystr_hash(o->nm))
#define FIO___IO_ALPN_CMP(a, b) fio_keystr_is_eq(a->nm, b->nm)
#define FIO___IO_ALPN_VALID(o)  fio_keystr_buf(&o->nm).len

FIO_TYPEDEF_IMAP_ARRAY(fio___io_tls_alpn_map,
                       fio___io_tls_alpn_s,
                       uint16_t,
                       FIO___IO_ALPN_HASH,
                       FIO___IO_ALPN_CMP,
                       FIO___IO_ALPN_VALID)
FIO_TYPEDEF_IMAP_ARRAY(fio___io_tls_trust_map,
                       fio___io_tls_trust_s,
                       uint16_t,
                       FIO___IO_ALPN_HASH,
                       FIO___IO_ALPN_CMP,
                       FIO___IO_ALPN_VALID)
FIO_TYPEDEF_IMAP_ARRAY(fio___io_tls_cert_map,
                       fio___io_tls_cert_s,
                       uint16_t,
                       FIO___IO_ALPN_HASH,
                       FIO___IO_ALPN_CMP,
                       FIO_IMAP_ALWAYS_VALID)

#undef FIO___IO_ALPN_HASH
#undef FIO___IO_ALPN_CMP
#undef FIO___IO_ALPN_VALID
#undef FIO_TYPEDEF_IMAP_REALLOC
#undef FIO_TYPEDEF_IMAP_FREE
#undef FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE
#define FIO_TYPEDEF_IMAP_REALLOC         FIO_MEM_REALLOC
#define FIO_TYPEDEF_IMAP_FREE            FIO_MEM_FREE
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE FIO_MEM_REALLOC_IS_SAFE

struct fio_io_tls_s {
  fio___io_tls_cert_map_s cert;
  fio___io_tls_alpn_map_s alpn;
  fio___io_tls_trust_map_s trust;
  uint8_t trust_sys; /** Set to 1 if system certificate registry is trusted */
};

#define FIO___RECURSIVE_INCLUDE 1
#define FIO_REF_NAME            fio_io_tls
#define FIO_REF_DESTROY(tls)                                                   \
  do {                                                                         \
    FIO_IMAP_EACH(fio___io_tls_alpn_map, &tls.alpn, i) {                       \
      fio_keystr_destroy(&tls.alpn.ary[i].nm, FIO_STRING_FREE_KEY);            \
    }                                                                          \
    FIO_IMAP_EACH(fio___io_tls_trust_map, &tls.trust, i) {                     \
      fio_keystr_destroy(&tls.trust.ary[i].nm, FIO_STRING_FREE_KEY);           \
    }                                                                          \
    FIO_IMAP_EACH(fio___io_tls_cert_map, &tls.cert, i) {                       \
      fio_keystr_destroy(&tls.cert.ary[i].nm, FIO_STRING_FREE_KEY);            \
      fio_keystr_destroy(&tls.cert.ary[i].public_cert_file,                    \
                         FIO_STRING_FREE_KEY);                                 \
      fio_keystr_destroy(&tls.cert.ary[i].private_key_file,                    \
                         FIO_STRING_FREE_KEY);                                 \
      fio_keystr_destroy(&tls.cert.ary[i].pk_password, FIO_STRING_FREE_KEY);   \
    }                                                                          \
    fio___io_tls_alpn_map_destroy(&tls.alpn);                                  \
    fio___io_tls_trust_map_destroy(&tls.trust);                                \
    fio___io_tls_cert_map_destroy(&tls.cert);                                  \
  } while (0)
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE

/** Performs a `new` operation, returning a new `fio_io_tls_s` context. */
SFUNC fio_io_tls_s *fio_io_tls_new(void) {
  fio_io_tls_s *r = fio_io_tls_new2();
  FIO_ASSERT_ALLOC(r);
  *r = (fio_io_tls_s){.trust_sys = 0};
  return r;
}

/** Performs a `dup` operation, increasing the object's reference count. */
SFUNC fio_io_tls_s *fio_io_tls_dup(fio_io_tls_s *tls) {
  return fio_io_tls_dup2(tls);
}

/** Performs a `free` operation, reducing the reference count and freeing. */
SFUNC void fio_io_tls_free(fio_io_tls_s *tls) {
  if (!tls)
    return;
  fio_io_tls_free2(tls);
}

/** Takes a parsed URL and optional TLS target and returns a TLS if needed. */
SFUNC fio_io_tls_s *fio_io_tls_from_url(fio_io_tls_s *tls, fio_url_s url) {
  /* test for TLS info in URL */
  fio_url_tls_info_s tls_info = fio_url_is_tls(url);
  if (!tls_info.tls)
    return tls;

  if (!tls && tls_info.tls)
    tls = fio_io_tls_new();

  if (tls_info.key.buf && tls_info.cert.buf) {
    const char *tmp = NULL;
    FIO_STR_INFO_TMP_VAR(host_tmp, 512);
    FIO_STR_INFO_TMP_VAR(key_tmp, 128);
    FIO_STR_INFO_TMP_VAR(cert_tmp, 128);
    FIO_STR_INFO_TMP_VAR(pass_tmp, 128);
    if (url.host.len < 512 && url.host.buf)
      fio_string_write(&host_tmp, NULL, url.host.buf, url.host.len);
    else
      host_tmp.buf = NULL;

    if (tls_info.key.len < 124 && tls_info.cert.len < 124 &&
        tls_info.pass.len < 124) {
      fio_string_write(&key_tmp, NULL, tls_info.key.buf, tls_info.key.len);
      fio_string_write(&cert_tmp, NULL, tls_info.cert.buf, tls_info.cert.len);
      if (tls_info.pass.len)
        fio_string_write(&pass_tmp, NULL, tls_info.pass.buf, tls_info.pass.len);
      else
        pass_tmp.buf = NULL;

      if (tls_info.key.buf ==
          tls_info.cert.buf) { /* assume value is prefix / folder */
        if ((tmp = getenv(cert_tmp.buf))) {
          fio_buf_info_s buf_tmp = FIO_BUF_INFO1((char *)tmp);
          if (buf_tmp.len < 124) {
            key_tmp.len = cert_tmp.len = buf_tmp.len;
            FIO_MEMCPY(key_tmp.buf, buf_tmp.buf, buf_tmp.len);
            FIO_MEMCPY(cert_tmp.buf, buf_tmp.buf, buf_tmp.len);
          }
        }
        fio_string_write(&key_tmp, NULL, "key.pem", 7);
        fio_string_write(&cert_tmp, NULL, "cert.pem", 8);
      } else {
        if ((tmp = getenv(key_tmp.buf))) {
          fio_buf_info_s buf_tmp = FIO_BUF_INFO1((char *)tmp);
          if (buf_tmp.len < 124) {
            key_tmp.len = buf_tmp.len;
            FIO_MEMCPY(key_tmp.buf, buf_tmp.buf, buf_tmp.len);
          }
        }

        if ((tmp = getenv(cert_tmp.buf))) {
          fio_buf_info_s buf_tmp = FIO_BUF_INFO1((char *)tmp);
          if (buf_tmp.len < 124) {
            cert_tmp.len = buf_tmp.len;
            FIO_MEMCPY(cert_tmp.buf, buf_tmp.buf, buf_tmp.len);
          }
        }

        if (tls_info.key.len < 5 ||
            (fio_buf2u32u(tls_info.key.buf + (tls_info.key.len - 4)) |
             0x20202020UL) != fio_buf2u32u(".pem")) {
          fio_string_write(&key_tmp, NULL, ".pem", 4);
        }
        if (tls_info.cert.len < 5 ||
            (fio_buf2u32u(tls_info.cert.buf + (tls_info.cert.len - 4)) |
             0x20202020UL) != fio_buf2u32u(".pem")) {
          fio_string_write(&cert_tmp, NULL, ".pem", 4);
        }
      }
      fio_io_tls_cert_add(tls,
                          host_tmp.buf,
                          cert_tmp.buf,
                          key_tmp.buf,
                          pass_tmp.buf);
    } else {
      FIO_LOG_ERROR("TLS files in `fio_io_listen` URL too long, "
                    "construct TLS object separately");
    }
  }
  return tls;
}

/** Adds a certificate a new SSL/TLS context / settings object (SNI support). */
SFUNC fio_io_tls_s *fio_io_tls_cert_add(fio_io_tls_s *t,
                                        const char *server_name,
                                        const char *public_cert_file,
                                        const char *private_key_file,
                                        const char *pk_password) {
  if (!t)
    return t;
  fio___io_tls_cert_s o = {
      .nm = fio_keystr_init(FIO_STR_INFO1((char *)server_name),
                            FIO_STRING_ALLOC_KEY),
      .public_cert_file =
          fio_keystr_init(FIO_STR_INFO1((char *)public_cert_file),
                          FIO_STRING_ALLOC_KEY),
      .private_key_file =
          fio_keystr_init(FIO_STR_INFO1((char *)private_key_file),
                          FIO_STRING_ALLOC_KEY),
      .pk_password = fio_keystr_init(FIO_STR_INFO1((char *)pk_password),
                                     FIO_STRING_ALLOC_KEY),
  };
  fio___io_tls_cert_s *old = fio___io_tls_cert_map_get(&t->cert, o);
  if (old)
    goto replace_old;
  fio___io_tls_cert_map_set(&t->cert, o, 1);
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
SFUNC fio_io_tls_s *fio_io_tls_alpn_add(fio_io_tls_s *t,
                                        const char *protocol_name,
                                        void (*on_selected)(fio_io_s *)) {
  if (!t || !protocol_name)
    return t;
  if (!on_selected)
    on_selected = fio_io_noop;
  size_t pr_name_len = strlen(protocol_name);
  if (pr_name_len > 255) {
    FIO_LOG_ERROR(
        "fio_io_tls_alpn_add called with name longer than 255 chars!");
    return t;
  }
  fio___io_tls_alpn_s o = {
      .nm = fio_keystr_init(FIO_STR_INFO2((char *)protocol_name, pr_name_len),
                            FIO_STRING_ALLOC_KEY),
      .fn = on_selected,
  };
  fio___io_tls_alpn_s *old = fio___io_tls_alpn_map_get(&t->alpn, o);
  if (old)
    goto replace_old;
  fio___io_tls_alpn_map_set(&t->alpn, o, 1);
  return t;
replace_old:
  fio_keystr_destroy(&old->nm, FIO_STRING_FREE_KEY);
  *old = o;
  return t;
}

/** Calls the `on_selected` callback for the `fio_io_tls_s` object. */
SFUNC int fio_io_tls_alpn_select(fio_io_tls_s *t,
                                 const char *protocol_name,
                                 size_t name_length,
                                 fio_io_s *io) {
  if (!t || !protocol_name)
    return -1;
  fio___io_tls_alpn_s seeking = {
      .nm = fio_keystr_tmp(protocol_name, (uint32_t)name_length)};
  fio___io_tls_alpn_s *alpn = fio___io_tls_alpn_map_get(&t->alpn, seeking);
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
 * Note: when the `fio_io_tls_s` object is used for server connections, this
 * will limit connections to clients that connect using a trusted certificate.
 *
 *      fio_io_tls_trust_add(tls, "google-ca.pem" );
 */
SFUNC fio_io_tls_s *fio_io_tls_trust_add(fio_io_tls_s *t,
                                         const char *public_cert_file) {
  if (!t)
    return t;
  if (!public_cert_file) {
    t->trust_sys = 1;
    return t;
  }
  fio___io_tls_trust_s o = {
      .nm = fio_keystr_init(FIO_STR_INFO1((char *)public_cert_file),
                            FIO_STRING_ALLOC_KEY),
  };
  fio___io_tls_trust_s *old = fio___io_tls_trust_map_get(&t->trust, o);
  if (old)
    goto replace_old;
  fio___io_tls_trust_map_set(&t->trust, o, 1);
  return t;
replace_old:
  fio_keystr_destroy(&old->nm, FIO_STRING_FREE_KEY);
  *old = o;
  return t;
}

/**
 * Returns the number of `fio_io_tls_cert_add` instructions.
 *
 * This could be used when deciding if to add a NULL instruction (self-signed).
 *
 * If `fio_io_tls_cert_add` was never called, zero (0) is returned.
 */
SFUNC uintptr_t fio_io_tls_cert_count(fio_io_tls_s *tls) {
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
SFUNC uintptr_t fio_io_tls_alpn_count(fio_io_tls_s *tls) {
  return tls ? tls->alpn.count : 0;
}

/**
 * Returns the number of `fio_io_tls_trust_add` instructions.
 *
 * This could be used when deciding if to disable peer verification or not.
 *
 * If `fio_io_tls_trust_add` was never called, zero (0) is returned.
 */
SFUNC uintptr_t fio_io_tls_trust_count(fio_io_tls_s *tls) {
  return tls ? tls->trust.count : 0;
}

/** Calls callbacks for certificate, trust certificate and ALPN added. */
void fio_io_tls_each___(void); /* IDE Marker*/
SFUNC int fio_io_tls_each FIO_NOOP(fio_io_tls_each_s a) {
  if (!a.tls)
    return -1;
  if (a.each_cert) {
    FIO_IMAP_EACH(fio___io_tls_cert_map, &a.tls->cert, i) {
      if (a.each_cert(&a,
                      fio_keystr_buf(&a.tls->cert.ary[i].nm).buf,
                      fio_keystr_buf(&a.tls->cert.ary[i].public_cert_file).buf,
                      fio_keystr_buf(&a.tls->cert.ary[i].private_key_file).buf,
                      fio_keystr_buf(&a.tls->cert.ary[i].pk_password).buf))
        return -1;
    }
  }
  if (a.each_alpn) {
    FIO_IMAP_EACH(fio___io_tls_alpn_map, &a.tls->alpn, i) {
      if (a.each_alpn(&a,
                      fio_keystr_buf(&a.tls->alpn.ary[i].nm).buf,
                      a.tls->alpn.ary[i].fn))
        return -1;
    }
  }
  if (a.each_trust) {
    if (a.tls->trust_sys && a.each_trust(&a, NULL))
      return -1;
    FIO_IMAP_EACH(fio___io_tls_trust_map, &a.tls->trust, i) {
      if (a.each_trust(&a, fio_keystr_buf(&a.tls->trust.ary[i].nm).buf))
        return -1;
    }
  }
  return 0;
}

/** If `NULL` returns current default, otherwise sets it. */
SFUNC fio_io_functions_s fio_io_tls_default_functions(fio_io_functions_s *f) {
  static fio_io_functions_s default_io_functions = {
      .build_context = fio___io_func_default_build_context,
      .start = fio_io_noop,
      .read = fio___io_func_default_read,
      .write = fio___io_func_default_write,
      .flush = fio___io_func_default_flush,
      .finish = fio___io_func_default_finish,
      .cleanup = fio___io_func_default_cleanup,
  };
  if (!f)
    return default_io_functions;
  if (!f->build_context)
    f->build_context = fio___io_func_default_build_context;
  if (!f->start)
    f->start = fio_io_noop;
  if (!f->read)
    f->read = fio___io_func_default_read;
  if (!f->write)
    f->write = fio___io_func_default_write;
  if (!f->flush)
    f->flush = fio___io_func_default_flush;
  if (!f->finish)
    f->finish = fio___io_func_default_finish;
  if (!f->cleanup)
    f->cleanup = fio___io_func_default_cleanup;
  default_io_functions = *f;
  return default_io_functions;
}

/* *****************************************************************************
IO Async Queues - Worker Threads for non-IO tasks
***************************************************************************** */

FIO_SFUNC void fio___io_async_start(fio_io_async_s *q) {
  if (!q->count)
    goto no_worker_threads;
  q->q = &q->queue;
  if (q->count > 4095)
    goto failed;
  fio_queue_workers_stop(&q->queue);
  if (fio_queue_workers_add(&q->queue, (size_t)q->count))
    goto failed;
  return;

failed:
  FIO_LOG_ERROR("IO Async Queue couldn't spawn threads!");
no_worker_threads:
  q->q = fio_io_queue();
  fio_queue_perform_all(&q->queue);
}
FIO_SFUNC void fio___io_async_stop(fio_io_async_s *q) {
  q->q = fio_io_queue();
  fio_queue_workers_stop(&q->queue);
  fio_queue_perform_all(&q->queue);
  fio_queue_destroy(&q->queue);
}

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
SFUNC void fio_io_async_attach(fio_io_async_s *q, uint32_t threads) {
  if (!q)
    return;
  if (!q->node.next) {
    *q = (fio_io_async_s){
        .q = fio_io_queue(),
        .count = threads,
        .queue = FIO_QUEUE_STATIC_INIT(q->queue),
        .timers = FIO_TIMER_QUEUE_INIT,
        .node = FIO_LIST_INIT(q->node),
    };
    FIO_LIST_PUSH(&FIO___IO.async, &q->node);
  }
  q->count = threads;
  if (fio_io_is_running())
    fio___io_async_start(q);
}

void fio_io_async_every___(void); /* IDE Mark */
/** Schedules a timer bound task for the async queue (`fio_timer_schedule`). */
SFUNC void fio_io_async_every FIO_NOOP(fio_io_async_s *q,
                                       fio_timer_schedule_args_s a) {
  a.start_at = FIO___IO.tick;
  fio_timer_schedule FIO_NOOP(&q->timers, a);
}

/* *****************************************************************************
Managing data after a fork
***************************************************************************** */
FIO_SFUNC void fio___io_after_fork(void *ignr_) {
  (void)ignr_;
  FIO___IO.pid = fio_thread_getpid();
  FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  fio_queue_perform_all(&FIO___IO.queue);
  FIO_LIST_EACH(fio_io_protocol_s,
                reserved.protocols,
                &FIO___IO.protocols,
                pr) {
    FIO_LIST_EACH(fio_io_s, node, &pr->reserved.ios, io) {
      fio_io_close_now(io);
    }
  }
  fio_queue_perform_all(&FIO___IO.queue);
  fio_queue_destroy(&FIO___IO.queue);
  FIO___IO.pids = FIO_LIST_INIT(FIO___IO.pids);
}

FIO_SFUNC void fio___io_cleanup_at_exit(void *ignr_) {
#ifdef SIGKILL
  FIO_LIST_EACH(fio___io_pid_s, node, &FIO___IO.pids, w) {
    fio_thread_kill(w->pid, SIGKILL);
  }
#endif /* SIGKILL */
  FIO___LOCK_DESTROY(FIO___IO.lock);
  fio___io_after_fork(ignr_);
  fio_poll_destroy(&FIO___IO.poll);
  FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  fio_queue_perform_all(&FIO___IO.queue);
  fio_timer_destroy(&FIO___IO.timer);
  fio_queue_perform_all(&FIO___IO.queue);
  fio___io_env_safe_destroy(&FIO___IO.env);
  fio_queue_perform_all(&FIO___IO.queue);
}

/* *****************************************************************************
Initializing IO Reactor State
***************************************************************************** */
FIO_CONSTRUCTOR(fio___io) {
  fio_queue_init(&FIO___IO.queue);
  FIO___IO.protocols = FIO_LIST_INIT(FIO___IO.protocols);
  FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  FIO___IO.root_pid = FIO___IO.pid = fio_thread_getpid();
  FIO___IO.async = FIO_LIST_INIT(FIO___IO.async);
  FIO___IO.pids = FIO_LIST_INIT(FIO___IO.pids);
  fio___io_protocol_init(&FIO___IO_MOCK_PROTOCOL, 0);
  fio_poll_init(&FIO___IO.poll,
                .on_data = fio___io_poll_on_data_schd,
                .on_ready = fio___io_poll_on_ready_schd,
                .on_close = fio___io_poll_on_close_schd);
  fio___io_protocol_init_test(&FIO___IO_MOCK_PROTOCOL, 0);
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___io_after_fork, NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___io_cleanup_at_exit, NULL);
}

/* *****************************************************************************
IO Types Finish
***************************************************************************** */
#endif /* FIO_IO */
