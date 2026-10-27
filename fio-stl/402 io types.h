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

FIO_IFUNC FIO___TSAN_AVOID uint32_t fio___flag(uint32_t *flg) { return *flg; }
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
  fio___io_env_obj_s old = {0};
  fio_thread_mutex_lock(&e->lock);
  fio___io_env_set(&e->env, hash, key, val, &old);
  fio_thread_mutex_unlock(&e->lock);
  /* destroy outside the lock: `on_close` may re-enter this environment */
  if (old.on_close)
    old.on_close(old.udata);
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
  fio___io_env_obj_s old = {0};
  fio_thread_mutex_lock(&e->lock);
  r = fio___io_env_remove(&e->env, hash, key, &old);
  fio_thread_mutex_unlock(&e->lock);
  /* destroy outside the lock: `on_close` may re-enter this environment */
  if (old.on_close)
    old.on_close(old.udata);
  return r;
}

FIO_IFUNC void fio___io_env_safe_destroy(fio___io_env_safe_s *e) {
  /* Detach the map before running `on_close` callbacks, so callbacks that
   * re-enter the environment see a stable map and never self-deadlock. */
  for (;;) {
    fio___io_env_s tmp;
    fio_thread_mutex_lock(&e->lock);
    tmp = e->env;
    e->env = (fio___io_env_s)FIO_MAP_INIT;
    fio_thread_mutex_unlock(&e->lock);
    if (!tmp.map)
      break;
    fio___io_env_destroy(&tmp);
  }
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
static void fio___io_on_close_noop(void *p1, void *p2) { (void)p1, (void)p2; }

/* Called to perform a non-blocking `read`, same as the system call. */
static ssize_t fio___io_func_default_read(fio_socket_i fd,
                                          void *buf,
                                          size_t len,
                                          void *tls) {
  return fio_sock_read(fd, buf, len);
  (void)tls;
}
/** Called to perform a non-blocking `write`, same as the system call. */
static ssize_t fio___io_func_default_write(fio_socket_i fd,
                                           const void *buf,
                                           size_t len,
                                           void *tls) {
  return fio_sock_write(fd, buf, len);
  (void)tls;
}
/** Sends any unsent internal data. Returns 0 only if all data was sent. */
static int fio___io_func_default_flush(fio_socket_i fd, void *tls) {
  return 0;
  (void)fd, (void)tls;
}
/** Sends any unsent internal data. Returns 0 only if all data was sent. */
static void fio___io_func_default_finish(fio_socket_i fd, void *tls) {
  (void)fd, (void)tls;
}
static void fio___io_func_default_cleanup(void *p1) { (void)p1; }

/** Default peer_info_next: no peer information available. */
static int fio___io_func_default_peer_info_next(fio_socket_i fd,
                                                fio_x509_cert_s *dest,
                                                void *tls) {
  return -1;
  (void)fd, (void)dest, (void)tls;
}

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
/** No `dup_context`: no context reference is shared (NULL). */
static void *fio___io_func_default_dup_context(void *context) {
  (void)context;
  return NULL;
}

static void fio___io_func_free_context_caller_task(void *fn_ptr,
                                                   void *context) {
  union {
    void (*free_context)(void *context);
    void *fn_ptr;
  } u = {.fn_ptr = fn_ptr};
  u.free_context(context);
}

/* Releases a built TLS context behind the queued IO tasks. With no memory for
 * the task, calls `free_context` inline instead (no guarantee is made for any
 * work `free_context` itself defers): `free_context` must already allow
 * IOs started from the context to be pending or open (see `free_context` and
 * `fio_io_attach_fd`), so no ordering it relies on is broken. Call threads:
 * connect cleanup - the IO thread; a listener's last release - the releasing
 * thread, which already performs the IO queue inline (`fio___io_listen_free`);
 * `fio_io_listen`'s allocation failure - the thread that ran `build_context`;
 * a failed `fio_io_attach_fd` - the attaching thread. */
static void fio___io_func_free_context_caller(void (*free_context)(void *),
                                              void *context) {
  union {
    void (*free_context)(void *context);
    void *fn_ptr;
  } u = {.free_context = free_context};
  if (FIO_LIKELY(!fio_queue_push(fio_io_queue(),
                                 fio___io_func_free_context_caller_task,
                                 u.fn_ptr,
                                 context)))
    return;
  free_context(context);
}

FIO_SFUNC void fio___io_protocol_init(fio_io_protocol_s *pr, _Bool has_tls) {
  pr->reserved.protocols = FIO_LIST_INIT(pr->reserved.protocols);
  pr->reserved.ios = FIO_LIST_INIT(pr->reserved.ios);
  fio_io_functions_s io_fn = {
      .build_context = fio___io_func_default_build_context,
      .free_context = fio___io_func_default_free_context,
      .dup_context = fio___io_func_default_dup_context,
      .start = fio_io_noop,
      .read = fio___io_func_default_read,
      .write = fio___io_func_default_write,
      .flush = fio___io_func_default_flush,
      .finish = fio___io_func_default_finish,
      .cleanup = fio___io_func_default_cleanup,
      .peer_info_next = fio___io_func_default_peer_info_next,
  };
  if (has_tls) {
    io_fn = fio_io_tls_default_functions(NULL);
    if (pr->io_functions.read == fio___io_func_default_read)
      pr->io_functions = io_fn;
  }
  if (!pr->on_attach)
    pr->on_attach = fio_io_noop;
  if (!pr->on_data)
    pr->on_data = fio_io_suspend;
  if (!pr->on_ready)
    pr->on_ready = fio_io_noop;
  if (!pr->on_close)
    pr->on_close = fio___io_on_close_noop;
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
  if (!pr->io_functions.dup_context)
    pr->io_functions.dup_context = fio___io_func_default_dup_context;
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
  if (!pr->io_functions.peer_info_next)
    pr->io_functions.peer_info_next = io_fn.peer_info_next;
  if (!pr->timeout)
    pr->timeout = FIO_IO_TIMEOUT_MAX;
  /* round up to nearest 16 byte size */
  pr->buffer_size = ((pr->buffer_size + 15ULL) & (~15ULL));
  if (pr->buffer_size > 0x3FFFFF)
    FIO_LOG_WARNING("A very large protocol buffer was requested... %zu per IO",
                    (size_t)pr->buffer_size);
}

/* The mock protocol hasn't started a transport, so its TLS value is still
 * a built context from the default backend, not per-connection state. */
static void fio___io_mock_tls_cleanup(void *context) {
  if (context)
    fio___io_func_free_context_caller(
        fio_io_tls_default_functions(NULL).free_context,
        context);
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
#define FIO___IO_FLAG_ASYNC    (8U) /* async queues started by the reactor */

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
  fio_socket_i wakeup_fd;
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
  /* the recorded IO (reactor) thread's numeral ID - the only thread allowed
   * to perform the main IO queue (DEBUG tripwire, FIO___IO_ASSERT_IO_THREAD).
   */
  uintptr_t io_thread;
} FIO___IO = {
    .tick = 0,
    .wakeup_fd = FIO_SOCKET_INVALID,
    .stop = 1,
    .lock = FIO___LOCK_INIT,
    .env = FIO___IO_ENV_SAFE_INIT,
    .shutdown_timeout = FIO_IO_SHUTDOWN_TIMEOUT,
    .io_thread = 0,
};

#if defined(DEBUG)
/* The IO layer is single-threaded per process: exactly one reactor thread
 * performs the main queue (`FIO___IO.queue`) - polling, deferred tasks and
 * the final `fio___io_free2` calls. The queue, not a lock, is the
 * synchronization. Record that thread on first IO-layer use and trip on any
 * access from a different thread (e.g., worker threads attached to the main
 * queue, or direct IO access from application threads). */
FIO_IFUNC void fio___io_assert_io_thread(void) {
  const uintptr_t t = fio_thread_nid();
  if (FIO_UNLIKELY(!FIO___IO.io_thread))
    FIO___IO.io_thread = t; /* first touch records the reactor thread */
  FIO_ASSERT_DEBUG(FIO___IO.io_thread == t,
                   "IO reactor accessed from a non-IO thread! "
                   "(the main IO queue must only be performed by the single "
                   "reactor thread of each process)");
}
#define FIO___IO_ASSERT_IO_THREAD() fio___io_assert_io_thread()
#else
#define FIO___IO_ASSERT_IO_THREAD() ((void)0)
#endif

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
  args.start_at = fio_io_last_tick();
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

FIO_SFUNC FIO___TSAN_AVOID void fio___io_last_tick_update(void *ignr_1,
                                                          void *ignr_2) {
  FIO___IO_FLAG_UNSET(&FIO___IO, FIO___IO_FLAG_TICK_SET);
  FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  FIO___IO.time_ms = fio_time2milli(fio_time_real());
  (void)ignr_1, (void)ignr_2;
}

/** Returns the last millisecond when the reactor polled for IO events (the
 * current time while the reactor is stopped). */
SFUNC FIO___TSAN_AVOID int64_t fio_io_last_tick(void) {
  if (!fio_io_is_running()) {
    fio___io_last_tick_update(NULL, NULL);
    return FIO___IO.tick;
  }
  if (!(FIO___IO_FLAG_SET(&FIO___IO, FIO___IO_FLAG_TICK_SET) &
        FIO___IO_FLAG_TICK_SET))
    fio_queue_push(&FIO___IO.queue, fio___io_last_tick_update, NULL, NULL);
  return FIO___IO.tick;
}

SFUNC FIO___TSAN_AVOID int64_t fio_io_last_tick_time(void) {
  if (!fio_io_is_running()) {
    fio___io_last_tick_update(NULL, NULL);
    return FIO___IO.time_ms;
  }
  if (!(FIO___IO_FLAG_SET(&FIO___IO, FIO___IO_FLAG_TICK_SET) &
        FIO___IO_FLAG_TICK_SET))
    fio_queue_push(&FIO___IO.queue, fio___io_last_tick_update, NULL, NULL);
  return FIO___IO.time_ms;
}

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
#define FIO___IO_FLAG_DATA_SCHD   ((uint32_t)2048U)
#define FIO___IO_FLAG_FINISHED                                                 \
  ((uint32_t)4096U) /* io_functions.finish called */

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
  fio_io_protocol_s *pr;
  void *udata;
  void *tls;
  fio___io_env_safe_s env;
  uint8_t pad___[64 - (63 & (sizeof(fio___io_env_safe_s) + sizeof(void *) +
                             sizeof(void *) + sizeof(fio_io_protocol_s *)))];
  fio_socket_i fd;
  uint32_t flags;
  FIO_LIST_NODE node;
  fio_stream_s out;
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
  if (fio_poll_monitor(&FIO___IO.poll, io->fd, (void *)io, POLLIN)) {
    FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_POLLIN_SET);
    /* No event can arrive for a failed registration. Schedule a retained
     * close task rather than leaving the connection permanently stalled. */
    if (!(FIO___IO_FLAG_SET(io, FIO___IO_FLAG_CLOSE) & FIO___IO_FLAG_CLOSE))
      fio___io_poll_on_close_schd(io);
  }
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
  if (fio_poll_monitor(&FIO___IO.poll, io->fd, (void *)io, POLLOUT)) {
    FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_POLLOUT_SET);
    if (!(FIO___IO_FLAG_SET(io, FIO___IO_FLAG_CLOSE) & FIO___IO_FLAG_CLOSE))
      fio___io_poll_on_close_schd(io);
  }
  // FIO_LOG_DDEBUG2("(%d) IO monitoring Output for %d", fio_io_pid(), io->fd);
}

FIO_IFUNC void fio___io_monitor_forget(fio_io_s *io) {
  /* Always forget: the poll backend retains consumed one-shot entries (and
   * may hold surviving armed flags) keyed by fd with the raw udata pointer.
   * Skipping the forget leaves a stale entry that a later review can
   * dispatch (POLLNVAL after close, or an fd-reused event) - resurrecting a
   * freed IO. Forgetting an unmonitored fd is harmless (backend returns -1). */
  fio_poll_forget(&FIO___IO.poll, io->fd);
  FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_POLL_SET);
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
  /* forget before close: epoll keeps a closed fd registered while any dup
   * (e.g., a listener's original fd) holds the open file description. */
  fio___io_monitor_forget(io);
  fio_sock_close(io->fd);
  fio_stream_destroy(&io->out);
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

FIO_LEAK_COUNTER_DEF(fio___io_protocol_zombie)

static void fio___io_on_close_zombie(void *p1, void *p2) {
  (void)p2;
  if (!p1)
    return;
  fio_io_s *io = ((fio_io_s *)p1) - 1;
  if (!io || !io->pr)
    return;
  FIO_MEM_FREE_(io->pr, sizeof(*io->pr));
  FIO_LEAK_COUNTER_ON_FREE(fio___io_protocol_zombie);
}

FIO_SFUNC void fio___io_protocol_set(void *io_, void *pr_) {
  fio_io_s *io = (fio_io_s *)io_;
  fio_io_protocol_s *pr = (fio_io_protocol_s *)pr_;
  fio_io_protocol_s *old = io->pr;
  if (!pr) {
    pr = &FIO___IO_MOCK_PROTOCOL;
    if (old->on_close == fio___io_on_close_zombie)
      pr = old;
    else if (old->io_functions.read != fio___io_func_default_read) {
      fio_io_protocol_s zero = {0};
      pr = FIO_MEM_REALLOC_(NULL, 0, sizeof(fio_io_protocol_s), 0);
      if (FIO_UNLIKELY(!pr)) { /* fail closed: `old` keeps its transport */
        FIO_LOG_ERROR("(%d) no memory: protocol switch failed, closing fd %d",
                      fio_io_pid(),
                      (int)io->fd);
        fio_io_close_now(io);
        goto finish;
      }
      FIO_LEAK_COUNTER_ON_ALLOC(fio___io_protocol_zombie);
      *pr = FIO___IO_MOCK_PROTOCOL;
      pr->reserved = zero.reserved;
      pr->on_close = fio___io_on_close_zombie;
      pr->io_functions = old->io_functions;
    }
  }
  fio___io_protocol_init_test(pr, (io->tls != NULL));
  if (pr == old)
    goto finish;
  FIO_LIST_REMOVE(&io->node);
  if (FIO_LIST_IS_EMPTY(&old->reserved.ios))
    FIO_LIST_REMOVE_RESET(&old->reserved.protocols);
  if (FIO_LIST_IS_EMPTY(&pr->reserved.ios))
    FIO_LIST_PUSH(&FIO___IO.protocols, &pr->reserved.protocols);
  FIO_LIST_PUSH(&pr->reserved.ios, &io->node);
  io->pr = pr;
  FIO_LOG_DEBUG2("(%d) protocol set for IO with fd %d",
                 fio_io_pid(),
                 fio_io_fd(io));
  pr->on_attach(io);

  /* avoid calling `start` and setting `on_ready` more than once */
  if (old == &FIO___IO_MOCK_PROTOCOL) {
    pr->io_functions.start(io);
    fio___io_monitor_out(io);
  } else if (old->on_close == fio___io_on_close_zombie) {
    FIO_MEM_FREE_(old,
                  sizeof(*old)); /* zombie revived: transport carries over */
    FIO_LEAK_COUNTER_ON_FREE(fio___io_protocol_zombie);
  }
  fio___io_monitor_in(io);
finish:
  FIO_LOG_DEBUG2("(%d) attached IO with fd %d", fio_io_pid(), fio_io_fd(io));
  fio___io_free_with_flush(io); /* may destroy `io` (e.g., closed meanwhile) */
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
SFUNC fio_io_s *fio_io_attach_fd(fio_socket_i fd,
                                 fio_io_protocol_s *pr,
                                 void *udata,
                                 void *tls) {
  fio_io_s *io = NULL;
  fio_io_protocol_s cpy;
  if (!pr)
    pr = &FIO___IO_MOCK_PROTOCOL;
  if (!FIO_SOCK_FD_ISVALID(fd))
    goto error;
  /* a blocking descriptor would stall the whole reactor */
  if (fio_sock_set_non_block(fd)) {
    FIO_LOG_ERROR("(%d) couldn't set fd %d to non-blocking, attach failed.",
                  fio_io_pid(),
                  (int)fd);
    goto error_close;
  }
  io = fio___io_new2(pr->buffer_size);
  if (!io) {
    FIO_LOG_ERROR("(%d) IO allocation failed for fd %d, attach failed.",
                  fio_io_pid(),
                  (int)fd);
    goto error_close;
  }
  *io = (fio_io_s){
      .fd = fd,
      .flags = FIO___IO_FLAG_OPEN,
      .pr = &FIO___IO_MOCK_PROTOCOL,
      .node = FIO_LIST_INIT(io->node),
      .udata = udata,
      .tls = tls,
      .active = fio_io_last_tick(), /* fresh even while stopped */
      .env = FIO___IO_ENV_SAFE_INIT,
  };
  FIO_LOG_DEBUG2("(%d) attaching fd %d to IO object %p (%zu bytes buffer)",
                 fio_io_pid(),
                 fd,
                 (void *)io,
                 fio_io_buffer_len(io));
  fio_io_defer(fio___io_protocol_set, (void *)fio___io_dup2(io), (void *)pr);
  return io;

error_close: /* the reactor owns `fd` once attach is called */
  fio_sock_close(fd);
error:
  if (tls) { /* owned by the reactor: release this reference */
    fio___io_protocol_init_test(pr, 1);
    void (*release)(void *) =
        pr == &FIO___IO_MOCK_PROTOCOL
            ? fio_io_tls_default_functions(NULL).free_context
            : pr->io_functions.free_context;
    fio___io_func_free_context_caller(release, tls);
  }
  cpy = *pr;
  if (cpy.on_close)
    fio___io_defer_no_wakeup((void (*)(void *, void *))cpy.on_close,
                             NULL,
                             udata);
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
  /* Pre-reactor initialization: a complete (plaintext) protocol, left
   * un-marked so the first real use re-runs initialization with the known TLS
   * state (which replaces plaintext default IO functions). The IO lists are
   * still empty, as every real use marks the protocol. */
  if (!pr)
    return pr;
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

/* Get/Set the IO's `udata` pointer */
FIO_DEF_GETSET_FUNC(IFUNC, fio_io, fio_io_s, void *, udata, FIO_NOOP_FN)
/* Get/Set the IO's `tls` context pointer */
FIO_DEF_GETSET_FUNC(IFUNC, fio_io, fio_io_s, void *, tls, FIO_NOOP_FN)

/** Returns the socket file descriptor (fd) associated with the IO. */
IFUNC fio_socket_i fio_io_fd(fio_io_s *io) { return io->fd; }

/** Returns the next peer information item for the connection. */
IFUNC int fio_io_peer_info_next(fio_io_s *io, fio_x509_cert_s *dest) {
  if (!io || !io->pr || !io->pr->io_functions.peer_info_next)
    return -1;
  return io->pr->io_functions.peer_info_next(fio_io_fd(io), dest, io->tls);
}

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
    fio_queue_push_urgent(&FIO___IO.queue,
                          fio___io_touch,
                          fio___io_dup2(io),
                          NULL);
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
  } else if (((unsigned)args.fd + 1) > 1 /* int file descriptor, not sock */) {
    packet = fio_stream_pack_fd((int)args.fd, args.len, args.offset, args.copy);
  } else /* fio_io_write2 called without data */
    goto do_nothing;
  if (!packet)
    goto error;
  if ((io->flags & FIO___IO_FLAG_CLOSE))
    goto write_called_after_close;
  FIO___IO_FLAG_SET(io, FIO___IO_FLAG_WRITE_DIRTY);
  if (FIO_LIKELY(!fio_queue_push(&FIO___IO.queue,
                                 fio___io_write2,
                                 (void *)fio___io_dup2(io),
                                 (void *)packet)))
    return;
  /* no memory for the task: the data is lost, so the stream must end. */
  fio_stream_packet_free(packet);
  fio_io_close(io);
  fio_io_free(io); /* the task's reference: release scheduled through the
                      ordinary queue (may also fail under OOM) */
  goto log_error;

error: /* note: `dealloc` already called by the `fio_stream` error handler. */
  fio_io_close(io); /* no data after a gap (earlier data may be truncated) */
log_error:
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
    if (fio_queue_push(&FIO___IO.queue,
                       fio___io_write2_dealloc_task,
                       u.ptr,
                       packet))
      fio_stream_packet_free(packet); /* no memory: caller's thread (as when
                                         packing fails) */
  }
  return;

io_error_null:
  FIO_LOG_ERROR("(%d) `fio_write2` called for invalid IO (NULL)", FIO___IO.pid);
do_nothing:
  /* CRT file fd — never a socket; close always correct */
  if (!args.buf && (unsigned)(args.fd + 1) > 1 && !args.copy)
    close((int)args.fd);
  /* De-alloc if required */
  if (args.dealloc) {
    union {
      void *ptr;
      void (*fn)(void *);
    } u = {.fn = args.dealloc};
    fio___io_defer_no_wakeup(fio___io_write2_dealloc_task, u.ptr, args.buf);
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

FIO_SFUNC void fio___io_free_task(void *io_, void *ignr_) {
  FIO___IO_ASSERT_IO_THREAD();
  fio_io_s *io = (fio_io_s *)io_;
  if (FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_WRITE_DIRTY) &
      FIO___IO_FLAG_WRITE_DIRTY) {
    fio___io_poll_on_ready_schd((void *)io);
  }
  fio___io_free2(io);
  (void)ignr_;
}
/** Free IO (reference) - thread-safe, flushes pending writes. */
SFUNC void fio_io_free(fio_io_s *io) {
  if (!io)
    return;
  /* push, then always wake (coalesced): a foreign thread's release (and
   * flush) never waits for another event; `io->flags` isn't read (racy) */
  fio_io_defer(fio___io_free_task, (void *)io, NULL);
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

FIO_SFUNC void fio___io_unsuspend(void *io_, void *ignr_) {
  fio_io_s *io = (fio_io_s *)io_;
  if (FIO___IO.stop)
    fio_io_close(io);
  else
    fio_io_on_data_schedule(io);
  fio___io_free2(io); /* reference taken by fio_io_unsuspend */
  (void)ignr_;
}

/** Listens for future "on_data" events related to the IO. */
SFUNC void fio_io_unsuspend(fio_io_s *io) {
  if ((FIO___IO_FLAG_UNSET(io, FIO___IO_FLAG_SUSPENDED) &
       FIO___IO_FLAG_SUSPENDED))
    fio_io_defer(fio___io_unsuspend, (void *)fio___io_dup2(io), NULL);
}

/** Returns 1 if the IO handle was suspended. */
SFUNC int fio_io_is_suspended(fio_io_s *io) {
  return (int)((io->flags & FIO___IO_FLAG_SUSPENDED) / FIO___IO_FLAG_SUSPENDED);
}

/** Returns 1 if the IO handle is open and not marked for closure. */
SFUNC int fio_io_is_open(fio_io_s *io) {
  return (io->flags & (FIO___IO_FLAG_OPEN | FIO___IO_FLAG_CLOSED_ALL)) ==
         FIO___IO_FLAG_OPEN;
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
  FIO___IO_FLAG_UNSET(io, (FIO___IO_FLAG_POLLIN_SET | FIO___IO_FLAG_DATA_SCHD));
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

FIO_STATIC_ALLOC_DEF(fio___on_ready_buf_new, char, FIO_IO_BUFFER_PER_WRITE, 4)
static void fio___io_poll_on_ready(void *io_, void *ignr_) {
  (void)ignr_;
#if defined(DEBUG) && DEBUG
  errno = 0;
#endif
  fio_io_s *io = (fio_io_s *)io_;
  char *buf_mem = fio___on_ready_buf_new(1);
  size_t total = 0;
  int pending = 0; /* last flush reported internal (e.g. TLS) output left */
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
    pending = (r != 0);
    switch ((size_t)(r + 1)) {
    case 0:
      if ((errno != EWOULDBLOCK) && (errno != EAGAIN))
        goto connection_error;
      break;
    case 1: break;
    default: total += (size_t)r; continue; /* flush until 0 / EAGAIN */
    }
    const size_t queued = fio_stream_length(&io->out);
    fio_stream_read(&io->out, &buf, &len);
    /* A read doesn't consume bytes. A reduced length means a short file
     * range; don't send later data under the original framing. */
    if (FIO_UNLIKELY(fio_stream_length(&io->out) < queued))
      goto stream_failed;
    if (!len)
      goto finish_loop;
    r = io->pr->io_functions.write(io->fd, buf, len, io->tls);
    switch (((size_t)r + 1)) {
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
  } else if ((io->flags & (FIO___IO_FLAG_CLOSE | FIO___IO_FLAG_WRITE_DIRTY))) {
    /* Queued writes may not have reached io->out yet. The finish path clears
     * WRITE_DIRTY and schedules another ready task behind those writes. */
    if (io->flags & FIO___IO_FLAG_WRITE_DIRTY)
      goto finish;
    /* Internal output (e.g. TLS records) drains before `finish`, and any
     * output `finish` adds (e.g. close_notify) drains before closing. The
     * wait is bounded by the timeout review (closing IOs time out). */
    if (!pending && !(FIO___IO_FLAG_SET(io, FIO___IO_FLAG_FINISHED) &
                      FIO___IO_FLAG_FINISHED)) {
      io->pr->io_functions.finish(io->fd, io->tls);
      int r = io->pr->io_functions.flush(io->fd, io->tls);
      if (r < 0 && errno != EWOULDBLOCK && errno != EAGAIN)
        goto connection_error;
      pending = (r != 0);
    }
    if (pending) {
      fio___io_monitor_out(io);
      goto finish;
    }
    fio_io_close_now(io);
  } else if (pending) {
    /* plaintext drained, but internal (e.g. TLS) output remains: wait for
     * writability and retry the flush before reporting `on_ready`. */
    fio___io_monitor_out(io);
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
  return;

stream_failed:
  FIO_LOG_ERROR("(%d) IO file data shorter than declared (file changed?), "
                "disconnecting: %p (fd %d)",
                FIO___IO.pid,
                (void *)io,
                io->fd);
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
  if ((io->flags & FIO___IO_FLAG_CLOSE))
    fio_io_close_now(io); /* stalled graceful close: drop unsent output */
  else
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
SFUNC void fio_io_on_data_schedule(fio_io_s *io) {
  if (!io || (io->flags & FIO___IO_FLAG_CLOSED_ALL))
    return;
  if (!(FIO___IO_FLAG_SET(io, FIO___IO_FLAG_DATA_SCHD) &
        FIO___IO_FLAG_DATA_SCHD)) {
    fio___io_defer_no_wakeup(fio___io_poll_on_data,
                             (void *)fio___io_dup2(io),
                             NULL);
  }
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
  static int64_t last_to_review = 0;
  /* test timeouts at whole second intervals */
  if (FIO___IO.tick - last_to_review < 1000)
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
      /* IO thread (single consumer): the task can't run before the dup. On
       * no memory, the (still expired) IO is reviewed again next time. */
      if (FIO_UNLIKELY(fio_queue_push(&FIO___IO.queue,
                                      fio___io_poll_on_timeout,
                                      (void *)io,
                                      NULL)))
        return c;
      fio___io_dup2(io);
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
  fio_socket_i fd = FIO___IO.wakeup_fd;
  FIO___IO.wakeup_fd = FIO_SOCKET_INVALID;
  if (FIO_SOCK_FD_ISVALID(fd))
    fio_sock_close(fd);
  FIO___IO.wakeup = NULL;
  FIO_LOG_DDEBUG2("(%d) fio___io_wakeup destroyed", FIO___IO.pid);
  (void)ignr1_, (void)ignr2_;
}

/* The flag coalesces wakeups; a closed pipe's writer is ignored. */
FIO_SFUNC void fio___io_wakeup(void) {
  if ((fio___flag(&FIO___IO.flags) & FIO___IO_FLAG_WAKEUP))
    return;
  fio_socket_i fd = FIO___IO.wakeup_fd;
  if (FIO_SOCK_FD_ISVALID(fd) &&
      !(FIO___IO_FLAG_SET(&FIO___IO, FIO___IO_FLAG_WAKEUP) &
        FIO___IO_FLAG_WAKEUP)) {
    char buf[1] = {(char)~0};
    ssize_t ignr = fio_sock_write(fd, buf, 1);
    (void)ignr;
  }
}

static fio_io_protocol_s FIO___IO_WAKEUP_PROTOCOL = {
    .on_data = fio___io_wakeup_cb,
    .on_close = fio___io_wakeup_on_close,
    .on_timeout = fio_io_touch,
};

FIO_SFUNC void fio___io_wakeup_init(void) {
  if (FIO___IO.wakeup)
    return;
  fio_socket_i fds[2];
  if (fio_sock_socketpair(fds)) {
    FIO_LOG_ERROR("(%d) couldn't open wakeup pipes, fio___io_wakeup disabled.",
                  FIO___IO.pid);
    return;
  }
  if (fio_sock_set_non_block(fds[1])) {
    /* a blocking write end could stall wakeup callers; `fds[0]` is set (or
     * rejected and closed) by `fio_io_attach_fd`. */
    FIO_LOG_ERROR("(%d) couldn't set wakeup pipe to non-blocking, "
                  "fio___io_wakeup disabled.",
                  FIO___IO.pid);
    fio_sock_close(fds[0]);
    fio_sock_close(fds[1]);
    return;
  }
  /* a flag left set by the previous pipe (unread) would swallow wakeups */
  FIO___IO_FLAG_UNSET(&FIO___IO, FIO___IO_FLAG_WAKEUP);
  FIO___IO.wakeup_fd = fds[1]; /* bytes wait in fds[0] */
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
  /** Client's expected server name: verified, and sent as SNI unless it is an
   * IP literal (empty = no SNI / no host name check). */
  fio_keystr_s sni;
  uint8_t trust_sys; /** Set to 1 if system certificate registry is trusted */
  /** Set to 1 if a requested trust entry couldn't be added (fail closed). */
  uint8_t trust_failed;
};

/** Destroys a key string that may hold a secret, wiping owned copies. */
FIO_IFUNC void fio___io_tls_keystr_destroy_secret(fio_keystr_s *k) {
  if (!k->info && k->buf) /* owned heap copy (const / tmp have info 0xFF) */
    fio_secure_zero((void *)k->buf, k->len);
  fio_keystr_destroy(k, FIO_STRING_FREE_KEY);
  fio_secure_zero(k, sizeof(*k)); /* embedded (short) strings */
}

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
      fio___io_tls_keystr_destroy_secret(&tls.cert.ary[i].pk_password);        \
    }                                                                          \
    fio_keystr_destroy(&tls.sni, FIO_STRING_FREE_KEY);                         \
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
        if ((tmp = fio_sys_env(cert_tmp.buf))) {
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
        if ((tmp = fio_sys_env(key_tmp.buf))) {
          fio_buf_info_s buf_tmp = FIO_BUF_INFO1((char *)tmp);
          if (buf_tmp.len < 124) {
            key_tmp.len = buf_tmp.len;
            FIO_MEMCPY(key_tmp.buf, buf_tmp.buf, buf_tmp.len);
          }
        }

        if ((tmp = fio_sys_env(cert_tmp.buf))) {
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
      if (pass_tmp.buf)
        fio_secure_zero(pass_tmp.buf, pass_tmp.len);
    } else {
      FIO_LOG_ERROR("TLS files in `fio_io_listen` URL too long, "
                    "construct TLS object separately");
    }
  }

  if (tls_info.trust.len) {
    if (((tls_info.trust.len == 6 &&
          ((fio_buf2u32u(tls_info.trust.buf + 2) | 0x20202020UL) ==
           fio_buf2u32u((char *)"stem"))) ||
         (tls_info.trust.len == 3)) &&
        ((tls_info.trust.buf[0] | 32) == 's') &&
        ((tls_info.trust.buf[1] | 32) == 'y') &&
        ((tls_info.trust.buf[2] | 32) == 's')) {
      fio_io_tls_trust_add(tls, NULL);
    } else if (tls_info.trust.len < 124) {
      FIO_STR_INFO_TMP_VAR(trust_tmp, 128);
      fio_string_write(&trust_tmp,
                       NULL,
                       tls_info.trust.buf,
                       tls_info.trust.len);
      if (tls_info.trust.len < 5 ||
          (fio_buf2u32u(tls_info.trust.buf + (tls_info.trust.len - 4)) |
           0x20202020UL) != fio_buf2u32u(".pem")) {
        fio_string_write(&trust_tmp, NULL, ".pem", 4);
      }
      fio_io_tls_trust_add(tls, trust_tmp.buf);
    } else {
      FIO_LOG_ERROR("TLS trust file in `fio_io_listen` URL too long, "
                    "construct TLS object separately");
      tls->trust_failed = 1; /* never silently drop peer verification */
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
  /* server names are case-insensitive: store them lowercased (SNI index) */
  char nm_buf[256];
  size_t nm_len = server_name ? FIO_STRLEN(server_name) : 0;
  if (nm_len > 255) {
    FIO_LOG_ERROR("fio_io_tls_cert_add: server name too long (max 255)");
    return t;
  }
  for (size_t i = 0; i < nm_len; ++i)
    nm_buf[i] = (char)(server_name[i] |
                       ((server_name[i] >= 'A' && server_name[i] <= 'Z') << 5));
  fio___io_tls_cert_s o = {
      .nm =
          fio_keystr_init(FIO_STR_INFO2(nm_buf, nm_len), FIO_STRING_ALLOC_KEY),
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
  if (fio___io_tls_cert_map_set(&t->cert, o, 1))
    return t;
  FIO_LOG_ERROR("fio_io_tls_cert_add failed to store certificate!");
  fio_keystr_destroy(&o.nm, FIO_STRING_FREE_KEY);
  fio_keystr_destroy(&o.public_cert_file, FIO_STRING_FREE_KEY);
  fio_keystr_destroy(&o.private_key_file, FIO_STRING_FREE_KEY);
  fio___io_tls_keystr_destroy_secret(&o.pk_password);
  return t;
replace_old:
  fio_keystr_destroy(&old->nm, FIO_STRING_FREE_KEY);
  fio_keystr_destroy(&old->public_cert_file, FIO_STRING_FREE_KEY);
  fio_keystr_destroy(&old->private_key_file, FIO_STRING_FREE_KEY);
  fio___io_tls_keystr_destroy_secret(&old->pk_password);
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
  size_t pr_name_len = FIO_STRLEN(protocol_name);
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
  if (!fio___io_tls_alpn_map_set(&t->alpn, o, 1)) {
    FIO_LOG_ERROR("fio_io_tls_alpn_add failed to store protocol!");
    fio_keystr_destroy(&o.nm, FIO_STRING_FREE_KEY);
  }
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
  if (!t || !protocol_name || name_length > 0xFF)
    return -1;
  fio___io_tls_alpn_s seeking = {
      .nm = fio_keystr_tmp(protocol_name, (uint32_t)name_length)};
  fio___io_tls_alpn_s *alpn = fio___io_tls_alpn_map_get(&t->alpn, seeking);
  if (!alpn) {
    FIO_LOG_DDEBUG2("TLS ALPN %.*s not found in %zu long list",
                    (int)name_length,
                    protocol_name,
                    (size_t)t->alpn.count);
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
  if (fio___io_tls_trust_map_set(&t->trust, o, 1))
    return t;
  /* fail closed: a dropped trust entry must never disable verification */
  FIO_LOG_ERROR("fio_io_tls_trust_add failed to store trust certificate!");
  fio_keystr_destroy(&o.nm, FIO_STRING_FREE_KEY);
  t->trust_failed = 1;
  return t;
replace_old:
  fio_keystr_destroy(&old->nm, FIO_STRING_FREE_KEY);
  *old = o;
  return t;
}

/** Returns true if `host` is an IP literal rather than a DNS host name. */
FIO_IFUNC int fio___io_tls_is_ip_literal(const char *host, size_t len) {
  /* RFC 6066, section 3: SNI carries DNS host names only, never IP literals. */
  if (!host || !len)
    return 1;
  if (host[0] == '[')
    return 1; /* bracketed IPv6 */
  int has_colon = 0, has_name_char = 0;
  for (size_t i = 0; i < len; ++i) {
    const char c = (char)(host[i] | 32);
    has_colon |= (host[i] == ':');
    has_name_char |= (c >= 'a' && c <= 'z');
  }
  if (has_colon)
    return 1; /* IPv6 */
  /* only digits and dots => IPv4 (a DNS name has at least one letter) */
  return !has_name_char;
}

/**
 * Sets the SNI (Server Name Indication) host name a TLS client should send
 * during the handshake (RFC 6066).
 *
 * `fio_io_connect` sets this automatically from the URL's host name, so
 * manual calls are rarely needed.
 *
 * NULL / empty host names and IP literals (IPv4 / IPv6) are silently
 * ignored, leaving any previously set name untouched.
 */
SFUNC fio_io_tls_s *fio_io_tls_sni_set(fio_io_tls_s *t,
                                       const char *hostname,
                                       size_t len) {
  if (!t || !hostname || !len || len > 255 ||
      fio___io_tls_is_ip_literal(hostname, len))
    return t;
  fio_keystr_s nm = fio_keystr_init(FIO_STR_INFO2((char *)hostname, len),
                                    FIO_STRING_ALLOC_KEY);
  fio_keystr_destroy(&t->sni, FIO_STRING_FREE_KEY);
  t->sni = nm;
  return t;
}

/**
 * Internal: sets a client's expected server identity (the URL host).
 *
 * Unlike `fio_io_tls_sni_set`, IP literals are stored and any previous name is
 * always replaced, so a reused TLS object never verifies a stale identity.
 * Backends verify the peer against this name and send it as SNI only when it
 * is a DNS host name (never an IP literal, RFC 6066).
 */
FIO_SFUNC void fio___io_tls_peer_name_set(fio_io_tls_s *t,
                                          const char *host,
                                          size_t len) {
  if (!t)
    return;
  fio_keystr_s nm = {0};
  if (host && len && len < 256)
    nm =
        fio_keystr_init(FIO_STR_INFO2((char *)host, len), FIO_STRING_ALLOC_KEY);
  fio_keystr_destroy(&t->sni, FIO_STRING_FREE_KEY);
  t->sni = nm;
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
  /* a failed trust instruction still counts, so verification stays on */
  return tls ? ((uintptr_t)tls->trust.count + tls->trust_failed) : 0;
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
    if (a.tls->trust_failed) {
      FIO_LOG_ERROR("TLS trust configuration incomplete (failed trust_add)");
      return -1; /* fail closed: the context must not be built */
    }
    if (a.tls->trust_sys && a.each_trust(&a, NULL))
      return -1;
    FIO_IMAP_EACH(fio___io_tls_trust_map, &a.tls->trust, i) {
      if (a.each_trust(&a, fio_keystr_buf(&a.tls->trust.ary[i].nm).buf))
        return -1;
    }
  }
  return 0;
}

/* *****************************************************************************
TLS Server Certificate Index (SNI) - shared by every TLS backend

Backends load each certificate once (in `fio_io_tls_each` order, positions
0..n-1) and register its name here; at ClientHello they call
`fio___io_tls_sni_find` once to pick the position to present. Keeping the
matching rules here guarantees identical selection across backends:

* names match case-insensitively (exact match);
* a single-label wildcard (`*.example.com`) matches `www.example.com`, but
  not `example.com` or `a.b.example.com` (tested only after an exact miss);
* no SNI, an unknown name, or a malformed name (empty, > 255 bytes, control,
  space, `*` or non-ASCII bytes) selects the default certificate: the first
  one added with an empty / NULL server name, else the first one added.
***************************************************************************** */

typedef struct {
  fio_keystr_s nm;
  size_t pos;
} fio___io_tls_sni_s;

#define FIO___IO_SNI_HASH(o)   ((uint32_t)fio_keystr_hash(o->nm))
#define FIO___IO_SNI_CMP(a, b) fio_keystr_is_eq(a->nm, b->nm)
#define FIO___IO_SNI_VALID(o)  fio_keystr_buf(&o->nm).len
FIO_TYPEDEF_IMAP_ARRAY(fio___io_tls_sni_map,
                       fio___io_tls_sni_s,
                       uint32_t,
                       FIO___IO_SNI_HASH,
                       FIO___IO_SNI_CMP,
                       FIO___IO_SNI_VALID)
#undef FIO___IO_SNI_HASH
#undef FIO___IO_SNI_CMP
#undef FIO___IO_SNI_VALID

/** Immutable after the context is built; read concurrently at handshakes. */
typedef struct {
  fio___io_tls_sni_map_s map;
  size_t dflt;      /* default certificate position */
  uint8_t has_dflt; /* an empty-name certificate was registered */
} fio___io_tls_sni_index_s;

/** Validates and lowercases a server name into `dest` (256 bytes). */
FIO_SFUNC size_t fio___io_tls_sni_normalize(char *dest,
                                            const char *name,
                                            size_t len,
                                            int allow_wildcard) {
  if (!name || !len || len > 255)
    return 0;
  for (size_t i = 0; i < len; ++i) {
    const uint8_t c = (uint8_t)name[i];
    if (c <= 0x20 || c >= 0x7F || (c == '*' && !allow_wildcard))
      return 0;
    dest[i] = (char)(c | ((c >= 'A' && c <= 'Z') << 5));
  }
  return len;
}

/** Registers the certificate at `pos` (fail closed on -1). */
FIO_SFUNC int fio___io_tls_sni_add(fio___io_tls_sni_index_s *idx,
                                   const char *name,
                                   size_t pos) {
  char buf[256];
  size_t len = name ? FIO_STRLEN(name) : 0;
  if (!len) {
    if (!idx->has_dflt)
      idx->dflt = pos;
    idx->has_dflt = 1;
    return 0;
  }
  len = fio___io_tls_sni_normalize(buf, name, len, 1);
  if (!len) {
    FIO_LOG_WARNING("TLS: certificate name %s can't match any SNI", name);
    return 0;
  }
  fio___io_tls_sni_s o = {.nm = fio_keystr_tmp(buf, (uint32_t)len), .pos = pos};
  if (fio___io_tls_sni_map_get(&idx->map, o))
    return 0; /* first certificate registered for a name wins */
  o.nm = fio_keystr_init(FIO_STR_INFO2(buf, len), FIO_STRING_ALLOC_KEY);
  if (fio___io_tls_sni_map_set(&idx->map, o, 1))
    return 0;
  fio_keystr_destroy(&o.nm, FIO_STRING_FREE_KEY);
  FIO_LOG_ERROR("TLS: failed to index certificate for %s", name);
  return -1;
}

/** Returns the certificate position to present for the client's SNI. */
FIO_SFUNC size_t fio___io_tls_sni_find(fio___io_tls_sni_index_s *idx,
                                       const char *sni,
                                       size_t len) {
  char buf[257];
  if (!idx->map.count)
    return idx->dflt;
  len = fio___io_tls_sni_normalize(buf + 1, sni, len, 0);
  if (!len)
    return idx->dflt;
  fio___io_tls_sni_s o = {.nm = fio_keystr_tmp(buf + 1, (uint32_t)len)};
  fio___io_tls_sni_s *found = fio___io_tls_sni_map_get(&idx->map, o);
  if (found)
    return found->pos;
  /* single-label wildcard: "www.example.com" => "*.example.com" */
  char *dot = (char *)FIO_MEMCHR(buf + 1, '.', len);
  if (!dot || dot == buf + 1 || (size_t)(dot - buf) == len)
    return idx->dflt;
  dot[-1] = '*';
  o.nm = fio_keystr_tmp(dot - 1, (uint32_t)(len + 1 - (size_t)(dot - buf)) + 1);
  found = fio___io_tls_sni_map_get(&idx->map, o);
  return found ? found->pos : idx->dflt;
}

/** Frees the index. */
FIO_SFUNC void fio___io_tls_sni_destroy(fio___io_tls_sni_index_s *idx) {
  FIO_IMAP_EACH(fio___io_tls_sni_map, &idx->map, i) {
    fio_keystr_destroy(&idx->map.ary[i].nm, FIO_STRING_FREE_KEY);
  }
  fio___io_tls_sni_map_destroy(&idx->map);
  *idx = (fio___io_tls_sni_index_s){0};
}

/** If `NULL` returns current default, otherwise sets it. */
SFUNC fio_io_functions_s fio_io_tls_default_functions(fio_io_functions_s *f) {
  static fio_io_functions_s default_io_functions = {
      .build_context = fio___io_func_default_build_context,
      .free_context = fio___io_func_default_free_context,
      .dup_context = fio___io_func_default_dup_context,
      .start = fio_io_noop,
      .read = fio___io_func_default_read,
      .write = fio___io_func_default_write,
      .flush = fio___io_func_default_flush,
      .finish = fio___io_func_default_finish,
      .cleanup = fio___io_func_default_cleanup,
      .peer_info_next = fio___io_func_default_peer_info_next,
  };
  if (!f)
    return default_io_functions;
  if (!f->build_context)
    f->build_context = fio___io_func_default_build_context;
  if (!f->free_context)
    f->free_context = fio___io_func_default_free_context;
  if (!f->dup_context)
    f->dup_context = fio___io_func_default_dup_context;
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
  if (!f->peer_info_next)
    f->peer_info_next = fio___io_func_default_peer_info_next;
  default_io_functions = *f;
  return default_io_functions;
}

/* *****************************************************************************
IO Async Queues - Worker Threads for non-IO tasks
***************************************************************************** */

/**
 * Routes new tasks to the IO queue, joins the worker threads and performs
 * pending tasks: no async task is running once this returns.
 *
 * Never destroys the queue: a producer that read `q->q` before the switch may
 * still push to it (performed on the next start, or by the drain at exit).
 */
FIO_IFUNC void fio___io_async_stop(fio_io_async_s *q) {
  q->q = fio_io_queue();
  fio_queue_workers_join(&q->queue); /* drains the queue if it had workers */
  fio_queue_perform_all(&q->queue);
}

/** Spawns worker threads; existing workers (re-attach) are told to retire. */
FIO_SFUNC void fio___io_async_start(fio_io_async_s *q) {
  fio_queue_workers_stop(
      &q->queue); /* no-op unless re-attached while running */
  if (q->count) {
    if (q->count < 4096 &&
        !fio_queue_workers_add(&q->queue, (size_t)q->count)) {
      q->q = &q->queue;
      return;
    }
    FIO_LOG_ERROR("IO Async Queue couldn't spawn threads!");
  }
  q->q = fio_io_queue();
  fio_queue_perform_all(&q->queue);
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
  a.start_at = fio_io_last_tick();
  fio_timer_schedule FIO_NOOP(&q->timers, a);
}

/* *****************************************************************************
Managing data after a fork / at exit
***************************************************************************** */

/* Performs the IO queue and every async queue until all are empty: a task may
 * push more tasks to any queue (including a stale `fio_io_async_queue` pointer
 * to a stopped async queue). Async workers must be joined (tasks run here). */
FIO_SFUNC void fio___io_drain_all(void) {
  uint32_t pending;
  do {
    fio_queue_perform_all(&FIO___IO.queue);
    FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, a) {
      fio_queue_perform_all(&a->queue);
    }
    pending = fio_queue_count(&FIO___IO.queue);
    FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, a) {
      pending |= fio_queue_count(&a->queue);
    }
  } while (pending);
}

FIO_SFUNC void fio___io_after_fork(void *ignr_) {
  (void)ignr_;
  FIO___IO.pid = fio_thread_getpid();
  FIO___IO.io_thread = 0; /* forked child has a new reactor thread */
  fio___io_last_tick_update(NULL, NULL);
  FIO___IO.lock = FIO___LOCK_INIT; /* better leak the lock, avoid UB */
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

/* Forked child, before any reactor state is used: only the forking thread
 * survives, so locks held by any other parent thread (e.g., an application
 * thread calling `fio_io_defer`, `fio_io_env_set` or `fio_io_async`) are reset
 * (better leak the lock, avoid UB) and async queues return to the stopped
 * state (no consumers; routed to the IO queue). Pending inherited async tasks
 * are kept: they run on the child's reactor start (`fio___io_async_start`) or
 * by the drain at exit. Idempotent while single threaded. */
FIO_SFUNC void fio___io_reset_in_child(void) {
  FIO___IO.lock = FIO___LOCK_INIT;
  FIO___IO.queue.lock = FIO___LOCK_INIT;
  FIO___IO.timer.lock = FIO___LOCK_INIT;
  FIO___IO.env.lock = (fio_thread_mutex_t)FIO_THREAD_MUTEX_INIT;
  FIO___IO_FLAG_UNSET(&FIO___IO, FIO___IO_FLAG_ASYNC);
  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, a) {
    a->queue.lock = FIO___LOCK_INIT;
    a->queue.consumers = FIO_LIST_INIT(a->queue.consumers);
    a->timers.lock = FIO___LOCK_INIT;
    a->q = &FIO___IO.queue;
  }
  FIO_LIST_EACH(fio_io_protocol_s,
                reserved.protocols,
                &FIO___IO.protocols,
                pr) { /* inherited IOs are closed: `on_close` locks the env */
    FIO_LIST_EACH(fio_io_s, node, &pr->reserved.ios, io) {
      io->env.lock = (fio_thread_mutex_t)FIO_THREAD_MUTEX_INIT;
    }
  }
}

FIO_SFUNC void fio___io_in_child(void *ignr_) {
  fio___io_reset_in_child();
  fio___io_after_fork(ignr_);
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
  fio___io_last_tick_update(NULL, NULL);
  fio_queue_perform_all(&FIO___IO.queue);
  fio_timer_destroy(&FIO___IO.timer);
  /* final teardown: every pending task runs (none is dropped). Callbacks may
   * push more tasks to any queue, so each step drains until all are empty. */
  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, a) {
    fio___io_async_stop(a); /* joins workers; new pushes go to the IO queue */
  }
  fio___io_drain_all();
  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, a) {
    fio_timer_destroy(&a->timers); /* calls `on_finish` */
    a->timers = (fio_timer_queue_s)FIO_TIMER_QUEUE_INIT;
  }
  fio___io_drain_all();
  fio___io_env_safe_destroy(&FIO___IO.env); /* `on_close` may push tasks */
  fio___io_drain_all();
  /* every queue is empty (checked above, with no callback since): destroying
   * only releases retained ring memory, it can't discard a pending task. */
  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, a) {
    fio_queue_destroy(&a->queue);
  }
}

/* *****************************************************************************
Initializing IO Reactor State
***************************************************************************** */
FIO_CONSTRUCTOR(fio___io) {
  fio_queue_init(&FIO___IO.queue);
  FIO___IO.protocols = FIO_LIST_INIT(FIO___IO.protocols);
  fio___io_last_tick_update(NULL, NULL);
  FIO___IO.root_pid = FIO___IO.pid = fio_thread_getpid();
  FIO___IO.async = FIO_LIST_INIT(FIO___IO.async);
  FIO___IO.pids = FIO_LIST_INIT(FIO___IO.pids);
  fio___io_protocol_init(&FIO___IO_MOCK_PROTOCOL, 0);
  FIO___IO_MOCK_PROTOCOL.io_functions.cleanup = fio___io_mock_tls_cleanup;
  if (fio_poll_init(&FIO___IO.poll,
                    .on_data = fio___io_poll_on_data_schd,
                    .on_ready = fio___io_poll_on_ready_schd,
                    .on_close = fio___io_poll_on_close_schd)) {
    const int err = errno;
    FIO_LOG_FATAL("couldn't initialize IO poller.\n");
    exit(err ? err : 1);
  }
  fio___io_protocol_init_test(&FIO___IO_MOCK_PROTOCOL, 0);
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___io_in_child, NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___io_cleanup_at_exit, NULL);
}

/* *****************************************************************************
IO Types Finish
***************************************************************************** */
#endif /* FIO_IO */
