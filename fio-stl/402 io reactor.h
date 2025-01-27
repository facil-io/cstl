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
    !defined(H___FIO_IO_REACTOR___H) &&                                        \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
#define H___FIO_IO_REACTOR___H

/* *****************************************************************************
The IO Reactor Cycle (the actual work)
***************************************************************************** */

static void fio___io_signal_stop(int sig, void *flg) {
  fio_io_stop();
  (void)sig, (void)flg;
}

static void fio___io_signal_restart(int sig, void *flg) {
  if (fio_io_is_master())
    fio_io_restart(FIO___IO.workers);
  else
    fio_io_stop();
  (void)sig, (void)flg;
}

FIO_SFUNC void fio___io_tick(int timeout) {
  static size_t performed_idle = 0;
  size_t idle_round = (fio_poll_review(&FIO___IO.poll, timeout) == 0);
  performed_idle &= idle_round;
  idle_round &= (timeout > 0);
  idle_round ^= performed_idle;
  if ((idle_round & !FIO___IO.stop)) {
    fio_state_callback_force(FIO_CALL_ON_IDLE);
    performed_idle = 1;
  }
  FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  fio_timer_push2queue(&FIO___IO.queue, &FIO___IO.timer, FIO___IO.tick);
  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, a) {
    fio_timer_push2queue(a->q, &a->timers, FIO___IO.tick);
  }
  for (size_t i = 0; i < 2048; ++i)
    if (fio_queue_perform(&FIO___IO.queue))
      break;
  fio___io_review_timeouts();
  fio_signal_review();
}

FIO_SFUNC void fio___io_run_async_as_sync(void *ignr_1, void *ignr_2) {
  (void)ignr_1, (void)ignr_2;
  unsigned repeat = 0;
  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, pos) {
    fio_queue_task_s t = fio_queue_pop(&pos->queue);
    if (!t.fn)
      continue;
    t.fn(t.udata1, t.udata2);
    repeat = 1;
  }
  if (repeat)
    fio_queue_push(&FIO___IO.queue, fio___io_run_async_as_sync);
}

FIO_SFUNC void fio___io_shutdown_task(void *shutdown_start_, void *a2) {
  intptr_t shutdown_start = (intptr_t)shutdown_start_;
  if (shutdown_start + FIO_IO_SHUTDOWN_TIMEOUT < FIO___IO.tick ||
      FIO_LIST_IS_EMPTY(&FIO___IO.protocols))
    return;
  fio___io_tick(fio_queue_count(&FIO___IO.queue) ? 0 : 100);
  fio_queue_push(&FIO___IO.queue, fio___io_run_async_as_sync);
  fio_queue_push(&FIO___IO.queue, fio___io_shutdown_task, shutdown_start_, a2);
}

FIO_SFUNC void fio___io_shutdown(void) {
  /* collect tick for shutdown start, to monitor for possible timeout */
  int64_t shutdown_start = FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  size_t connected = 0;
  /* first notify that shutdown is starting */
  fio_state_callback_force(FIO_CALL_ON_SHUTDOWN);
  /* preform on_shutdown callback for each connection and close */
  FIO_LIST_EACH(fio_io_protocol_s,
                reserved.protocols,
                &FIO___IO.protocols,
                pr) {
    FIO_LIST_EACH(fio_io_s, node, &pr->reserved.ios, io) {
      pr->on_shutdown(io); /* TODO / FIX: move callback to task? */
      if (!(io->flags & FIO___IO_FLAG_SUSPENDED))
        fio_io_close(io); /* TODO / FIX: skip close on return value? */
      ++connected;
    }
  }
  FIO_LOG_DEBUG2("(%d) IO Reactor shutting down with %zu connected clients",
                 fio_io_pid(),
                 connected);
  /* cycle while connections exist. */
  fio_queue_push(&FIO___IO.queue,
                 fio___io_shutdown_task,
                 (void *)(intptr_t)shutdown_start,
                 NULL);
  fio_queue_perform_all(&FIO___IO.queue);
  /* in case of timeout, force close remaining connections. */
  connected = 0;
  FIO_LIST_EACH(fio_io_protocol_s,
                reserved.protocols,
                &FIO___IO.protocols,
                pr) {
    FIO_LIST_EACH(fio_io_s, node, &pr->reserved.ios, io) {
      fio_io_close_now(io);
      ++connected;
    }
  }
  FIO_LOG_DEBUG2("(%d) IO Reactor shutdown timeout/done with %zu clients",
                 fio_io_pid(),
                 connected);
  /* perform remaining tasks. */
  fio_queue_perform_all(&FIO___IO.queue);
}

FIO_SFUNC void fio___io_work_task(void *ignr_1, void *ignr_2) {
  if (FIO___IO.stop)
    goto no_run;
  fio___io_tick(fio_queue_count(&FIO___IO.queue) ? 0 : 500);
  fio_queue_push(&FIO___IO.queue, fio___io_work_task, ignr_1, ignr_2);
  return;
no_run:
  FIO___IO_FLAG_UNSET(&FIO___IO, FIO___IO_FLAG_CYCLING);
}

FIO_SFUNC void fio___io_work(int is_worker) {
  FIO___IO.is_worker = is_worker;
  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, q) {
    fio___io_async_start(q);
  }

  fio_queue_perform_all(&FIO___IO.queue);
  if (is_worker) {
    fio_state_callback_force(FIO_CALL_ON_START);
  }
  fio___io_wakeup_init();
  FIO___IO_FLAG_SET(&FIO___IO, FIO___IO_FLAG_CYCLING);
  fio_queue_push(&FIO___IO.queue, fio___io_work_task);
  fio_queue_perform_all(&FIO___IO.queue);
  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, q) {
    fio___io_async_stop(q);
  }
  fio___io_shutdown();

  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, q) {
    fio___io_async_stop(q);
  }

  fio_queue_perform_all(&FIO___IO.queue);
  fio_state_callback_force(FIO_CALL_ON_STOP);
  fio_queue_perform_all(&FIO___IO.queue);
  FIO___IO.workers = 0;
}

/* *****************************************************************************
Worker Forking
***************************************************************************** */
static void fio___io_spawn_workers_task(void *ignr_1, void *ignr_2);

static void fio___io_wait_for_worker(void *thr_) {
  fio_thread_t t = (fio_thread_t)thr_;
  fio_thread_join(&t);
}

/** Worker sentinel */
static void *fio___io_worker_sentinel(void *pid_data) {
#ifdef WEXITSTATUS
  fio___io_pid_s sentinal = {.pid = (fio_thread_pid_t)(uintptr_t)pid_data};
  int status = 0;
  (void)status;
  fio_thread_t thr = fio_thread_current();
  fio_state_callback_add(FIO_CALL_ON_STOP,
                         fio___io_wait_for_worker,
                         (void *)thr);

  FIO___LOCK_LOCK(FIO___IO.lock);
  if (!FIO___IO.pids.next)
    FIO___IO.pids = FIO_LIST_INIT(FIO___IO.pids);
  FIO_LIST_PUSH(&FIO___IO.pids, &sentinal.node);
  FIO___LOCK_UNLOCK(FIO___IO.lock);

  if (fio_thread_waitpid(sentinal.pid, &status, 0) != sentinal.pid &&
      !FIO___IO.stop)
    FIO_LOG_ERROR("waitpid failed, worker re-spawning might fail.");

  FIO___LOCK_LOCK(FIO___IO.lock);
  FIO_LIST_REMOVE(&sentinal.node);
  FIO___LOCK_UNLOCK(FIO___IO.lock);

  if (!WIFEXITED(status) || WEXITSTATUS(status)) {
    FIO_LOG_WARNING("(%d) abnormal worker exit detected", FIO___IO.pid);
    fio_state_callback_force(FIO_CALL_ON_CHILD_CRUSH);
  }
  if (!FIO___IO.stop && !sentinal.stop) {
    FIO_ASSERT_DEBUG(
        0,
        "DEBUG mode prevents worker re-spawning, now crashing parent.");
    fio_state_callback_remove(FIO_CALL_ON_STOP,
                              fio___io_wait_for_worker,
                              (void *)thr);
    fio_thread_detach(&thr);
    FIO_LOG_WARNING("(%d) worker exit detected, replacing worker %d",
                    FIO___IO.pid,
                    sentinal.pid);
    fio_atomic_add(&FIO___IO.to_spawn, (uint32_t)1);
    fio_queue_push_urgent(fio_io_queue(), fio___io_spawn_workers_task);
  }
#else /* Non POSIX? no `fork`? no fio_thread_waitpid? */
  FIO_ASSERT(
      0,
      "facil.io doesn't know how to spawn and wait on workers on this system.");
#endif
  return NULL;
}

static void fio___io_spawn_worker(void) {
  fio_thread_t t;
  fio_signal_review();

  if (FIO___IO.stop || !fio_io_is_master())
    return;

  /* do not allow master tasks to run in worker - pretend to stop. */
  FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  if (fio_atomic_or_fetch(&FIO___IO.stop, 2) != 2)
    return;
  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, q) {
    fio___io_async_stop(q);
  }
  fio_queue_perform_all(&FIO___IO.queue);
  /* perform forking procedure with the stop flag reset. */
  fio_atomic_and_fetch(&FIO___IO.stop, 1);
  fio_state_callback_force(FIO_CALL_BEFORE_FORK);
  FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  /* perform actual fork */
  fio_thread_pid_t pid = fio_thread_fork();
  FIO_ASSERT(pid != (fio_thread_pid_t)-1, "system call `fork` failed.");
  if (!pid)
    goto is_worker_process;
  /* finish up */
  fio_state_callback_force(FIO_CALL_AFTER_FORK);
  fio_state_callback_force(FIO_CALL_IN_MASTER);
  if (fio_thread_create(&t, fio___io_worker_sentinel, (void *)(uintptr_t)pid)) {
    FIO_LOG_FATAL(
        "sentinel thread creation failed, no worker will be spawned.");
    fio_io_stop();
  }
  return;

is_worker_process:
  FIO___IO.pid = fio_thread_getpid();
  /* close all inherited connections immediately? */
  FIO_LIST_EACH(fio_io_protocol_s,
                reserved.protocols,
                &FIO___IO.protocols,
                pr) {
    FIO_LIST_EACH(fio_io_s, node, &pr->reserved.ios, io) {
      fio_io_close_now(io);
    }
  }
  fio_queue_perform_all(&FIO___IO.queue);
  /* TODO: keep? */

  FIO___IO.is_worker = 1;
  FIO_LOG_INFO("(%d) worker starting up.", fio_io_pid());

  if (FIO___IO.stop)
    goto skip_work;
  fio_state_callback_force(FIO_CALL_AFTER_FORK);
  fio_queue_perform_all(&FIO___IO.queue);
  fio_state_callback_force(FIO_CALL_IN_CHILD);
  fio_queue_perform_all(&FIO___IO.queue);
  fio___io_work(1);
skip_work:
  FIO_LOG_INFO("(%d) worker exiting.", fio_io_pid());
  exit(0);
}

static void fio___io_spawn_workers_task(void *ignr_1, void *ignr_2) {
  static volatile unsigned is_running = 0;

  if (!fio_io_is_master() || !FIO___IO.to_spawn)
    return;
  /* don't run nested */
  if (fio_atomic_or(&is_running, 1))
    return;
  FIO_LOG_INFO("(%d) spawning %d workers.", fio_io_pid(), FIO___IO.to_spawn);
  do {
    fio___io_spawn_worker();
  } while (fio_atomic_sub_fetch(&FIO___IO.to_spawn, 1));

  if (!(FIO___IO_FLAG_SET(&FIO___IO, FIO___IO_FLAG_CYCLING) &
        FIO___IO_FLAG_CYCLING)) {
    fio___io_defer_no_wakeup(fio___io_work_task, NULL, NULL);
    FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, q) {
      fio___io_async_start(q);
    }
  }

  is_running = 0;
  (void)ignr_1, (void)ignr_2;
}

/* *****************************************************************************
Starting / Stopping the IO Reactor
***************************************************************************** */

/** Adds `workers` amount of workers to the root IO reactor process. */
SFUNC void fio_io_add_workers(int workers) {
  if (!workers || !fio_io_is_master())
    return;
  fio_atomic_add(&FIO___IO.to_spawn, (uint32_t)fio_io_workers(workers));
  fio_queue_push_urgent(&FIO___IO.queue, fio___io_spawn_workers_task);
}

/** Starts the IO reactor, using optional `workers` processes. Will BLOCK! */
SFUNC void fio_io_start(int workers) {
  FIO___IO.stop = 0;
  FIO___IO.workers = fio_io_workers(workers);
  workers = (int)FIO___IO.workers;
  FIO___IO.is_worker = !workers;
  fio_sock_maximize_limits(0);

  FIO_LIST_EACH(fio_io_async_s, node, &FIO___IO.async, q) {
    fio___io_async_start(q);
  }

  fio_state_callback_force(FIO_CALL_PRE_START);
  fio_queue_perform_all(&FIO___IO.queue);
  fio_signal_monitor(SIGINT, fio___io_signal_stop, NULL);
  fio_signal_monitor(SIGTERM, fio___io_signal_stop, NULL);
  if (FIO___IO.restart_signal)
    fio_signal_monitor(FIO___IO.restart_signal, fio___io_signal_restart, NULL);

#ifdef SIGPIPE
  fio_signal_monitor(SIGPIPE, NULL, NULL);
#endif
  FIO___IO.tick = FIO___IO_GET_TIME_MILLI();
  if (workers) {
    FIO_LOG_INFO("(%d) spawning %d workers.", fio_io_root_pid(), workers);
    for (int i = 0; i < workers; ++i) {
      fio___io_spawn_worker();
    }
  } else {
    FIO_LOG_DEBUG2("(%d) starting facil.io IO reactor in single process mode.",
                   fio_io_root_pid());
  }
  fio___io_work(!workers);
  fio_signal_forget(SIGINT);
  fio_signal_forget(SIGTERM);
  if (FIO___IO.restart_signal)
    fio_signal_forget(FIO___IO.restart_signal);
#ifdef SIGPIPE
  fio_signal_forget(SIGPIPE);
#endif
  fio_queue_perform_all(&FIO___IO.queue);
}

/** Returns the number or workers the IO reactor will actually run. */
SFUNC uint16_t fio_io_workers(int workers) {
  if (workers < 0) {
    long cores = -1;
#ifdef _SC_NPROCESSORS_ONLN
    cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif /* _SC_NPROCESSORS_ONLN */
    if (cores == -1L) {
      cores = 8;
      FIO_LOG_WARNING(
          "fio_io_start / fio_io_workers called with negative value for worker "
          "count, but auto-detect failed, assuming %d CPU cores",
          cores);
    }
    workers = (int)(cores / (0 - workers));
    workers += !workers;
  }
  return (uint16_t)workers;
}

/** Retiers all existing workers and restarts with the number of workers. */
SFUNC void fio___io_restart(void *workers_, void *ignr_) {
  int workers = (int)(uintptr_t)workers_;
  (void)ignr_;
  if (!fio_io_is_master())
    return;
  if (!FIO___IO.workers)
    goto no_workers;

  FIO___IO.workers = fio_io_workers(workers);
  workers = (int)FIO___IO.workers;
  if (workers) {
    FIO_LOG_INFO(
        "(%d) shutting down existing workers and (re)spawning %d workers.",
        fio_io_root_pid(),
        workers);
    fio_atomic_add(&FIO___IO.to_spawn, (uint32_t)workers);
    /* schedule workers to spawn - won't run until we return from function. */
    fio_queue_push_urgent(fio_io_queue(), fio___io_spawn_workers_task);
  } else {
    FIO_LOG_INFO(
        "(%d) shutting down existing workers and switching to single mode.",
        fio_io_root_pid());
  }
  /* signal existing children */
  FIO___LOCK_LOCK(FIO___IO.lock);
  FIO_LIST_EACH(fio___io_pid_s, node, &FIO___IO.pids, w) {
    w->stop = 1;
    fio_thread_kill(w->pid, SIGTERM);
  }
  FIO___LOCK_UNLOCK(FIO___IO.lock);
  /* switch to single mode? */
  if (!workers) {
    fio_state_callback_force(FIO_CALL_ON_START);
    FIO___IO.is_worker = 1;
  }
  return;
no_workers:
  FIO_LOG_ERROR("no workers to restart - IO worker restart is only available "
                "in cluster mode!");
  /* TODO: exec with all listeners intact...? */
  return;
}

SFUNC void fio_io_restart(int workers) {
  fio_queue_push(&FIO___IO.queue, fio___io_restart, (void *)(uintptr_t)workers);
}

/* *****************************************************************************
Listening to Incoming Connections
***************************************************************************** */

typedef struct {
  fio_io_protocol_s *protocol;
  void *udata;
  void *tls_ctx;
  fio_io_async_s *queue_for_accept;
  fio_queue_s *queue;
  fio_io_s *io;
  void (*on_start)(fio_io_protocol_s *protocol, void *udata);
  void (*on_stop)(fio_io_protocol_s *protocol, void *udata);
  int owner;
  int fd;
  size_t ref_count;
  size_t url_len;
  uint8_t hide_from_log;
  char url[];
} fio___io_listen_s;

FIO_LEAK_COUNTER_DEF(fio_io_listen)

static fio___io_listen_s *fio___io_listen_dup(fio___io_listen_s *l) {
  fio_atomic_add(&l->ref_count, 1);
  return l;
}

static void fio___io_listen_free(void *l_) {
  fio___io_listen_s *l = (fio___io_listen_s *)l_;
  if (l->io)
    fio_io_close(l->io);
  if (fio_atomic_sub(&l->ref_count, 1))
    return;

  fio_state_callback_remove(FIO_CALL_AT_EXIT, fio___io_listen_free, (void *)l);
  fio_state_callback_remove(FIO_CALL_ON_START, fio___io_listen_free, (void *)l);
  fio_state_callback_remove(FIO_CALL_PRE_START,
                            fio___io_listen_free,
                            (void *)l);
  fio___io_func_free_context_caller(l->protocol->io_functions.free_context,
                                    l->tls_ctx);
  fio_sock_close(l->fd);

#ifdef AF_UNIX
  /* delete the unix socket file, if any. */
  fio_url_s u = fio_url_parse(l->url, FIO_STRLEN(l->url));
  if (FIO___IO.pid == l->owner && !u.host.buf && !u.port.buf && u.path.buf) {
    unlink(u.path.buf);
  }
#endif

  if (l->on_stop)
    l->on_stop(l->protocol, l->udata);

  if (l->hide_from_log)
    FIO_LOG_DEBUG2("(%d) stopped listening @ %.*s",
                   getpid(),
                   (int)l->url_len,
                   l->url);
  else
    FIO_LOG_INFO("(%d) stopped listening @ %.*s",
                 getpid(),
                 (int)l->url_len,
                 l->url);
  fio_queue_perform_all(&FIO___IO.queue);
  FIO_LEAK_COUNTER_ON_FREE(fio_io_listen);
  FIO_MEM_FREE_(l, sizeof(*l) + l->url_len + 1);
}

SFUNC void fio_io_listen_stop(void *listener) {
  if (listener)
    fio___io_listen_free(listener);
}

/** Returns the URL on which the listener is listening. */
SFUNC fio_buf_info_s fio_io_listener_url(void *listener) {
  fio___io_listen_s *l = (fio___io_listen_s *)listener;
  return FIO_BUF_INFO2(l->url, l->url_len);
}

/** Returns true if the listener protocol has an attached TLS context. */
SFUNC int fio_io_listener_is_tls(void *listener) {
  fio___io_listen_s *l = (fio___io_listen_s *)listener;
  return !!l->tls_ctx;
}

/** Returns the listener's associated protocol. */
SFUNC fio_io_protocol_s *fio_io_listener_protocol(void *listener) {
  fio___io_listen_s *l = (fio___io_listen_s *)listener;
  return l->protocol;
}

/** Returns the listener's associated `udata`. */
SFUNC void *fio_io_listener_udata(void *listener) {
  fio___io_listen_s *l = (fio___io_listen_s *)listener;
  return l->udata;
}

/** Sets the listener's associated `udata`, returning the old value. */
SFUNC void *fio_io_listener_udata_set(void *listener, void *new_udata) {
  void *old;
  fio___io_listen_s *l = (fio___io_listen_s *)listener;
  old = l->udata;
  l->udata = new_udata;
  return old;
}

static void fio___io_listen_on_data_task(void *io_, void *ignr_) {
  (void)ignr_;
  fio_io_s *io = (fio_io_s *)io_;
  int fd;
  fio___io_listen_s *l = (fio___io_listen_s *)fio_io_udata(io);
  fio_io_unsuspend(io);
  while (FIO_SOCK_FD_ISVALID(fd = fio_sock_accept(fio_io_fd(io), NULL, NULL))) {
    FIO_LOG_DDEBUG2("(%d) accepted new connection with fd %d",
                    fio_io_pid(),
                    fd);
    fio_io_attach_fd(fd, l->protocol, l->udata, l->tls_ctx);
  }
  fio___io_free2(io);
}
static void fio___io_listen_on_data_task_reschd(void *io_, void *ignr_) {
  fio_io_defer(fio___io_listen_on_data_task, io_, ignr_);
}
static void fio___io_listen_on_attach(fio_io_s *io) {
  fio___io_listen_s *l = (fio___io_listen_s *)(io->udata);
  l->queue = (l->queue_for_accept && l->queue_for_accept->q != &FIO___IO.queue)
                 ? l->queue_for_accept->q
                 : NULL;
  if (l->on_start)
    l->on_start(l->protocol, l->udata);
  if (l->hide_from_log)
    FIO_LOG_DEBUG2("(%d) started listening @ %s%s",
                   fio_io_pid(),
                   l->url,
                   l->tls_ctx ? " (TLS)" : "");
  else
    FIO_LOG_INFO("(%d) started listening @ %s%s",
                 fio_io_pid(),
                 l->url,
                 l->tls_ctx ? " (TLS)" : "");
}
static void fio___io_listen_on_shutdown(fio_io_s *io) {
  fio___io_listen_s *l = (fio___io_listen_s *)(io->udata);
  l->queue = fio_io_queue();
}
static void fio___io_listen_on_data(fio_io_s *io) {
  fio___io_listen_s *l = (fio___io_listen_s *)(io->udata);
  if (l->queue) {
    fio_io_suspend(io);
    fio_queue_push(l->queue,
                   fio___io_listen_on_data_task_reschd,
                   fio___io_dup2(io));
    return;
  }
  fio___io_listen_on_data_task(fio___io_dup2(io), NULL);
}
static void fio___io_listen_on_close(void *buffer, void *l) {
  ((fio___io_listen_s *)l)->io = NULL;
  fio___io_listen_free(l);
  (void)buffer;
}

static fio_io_protocol_s FIO___IO_LISTEN_PROTOCOL = {
    .on_attach = fio___io_listen_on_attach,
    .on_data = fio___io_listen_on_data,
    .on_close = fio___io_listen_on_close,
    .on_timeout = fio_io_touch,
    .on_shutdown = fio___io_listen_on_shutdown,
};

FIO_SFUNC void fio___io_listen_attach_task_deferred(void *l_, void *ignr_) {
  fio___io_listen_s *l = (fio___io_listen_s *)l_;
  l = fio___io_listen_dup(l);
  int fd = fio_sock_dup(l->fd);
  FIO_ASSERT(fd != -1, "listening socket failed to `dup`");
  FIO_LOG_DEBUG2("(%d) Called dup(%d) to attach %d as a listening socket.",
                 (int)fio_io_pid(),
                 l->fd,
                 fd);
  l->io = fio_io_attach_fd(fd, &FIO___IO_LISTEN_PROTOCOL, l, NULL);
  (void)ignr_;
}

FIO_SFUNC void fio___io_listen_attach_task(void *l_) {
  /* make sure to run in server thread */
  fio_io_defer(fio___io_listen_attach_task_deferred, l_, NULL);
}

int fio_io_listen___(void); /* IDE marker */
/**
 * Sets up a network service on a listening socket.
 *
 * Returns 0 on success or -1 on error.
 *
 * See the `fio_listen` Macro for details.
 */
SFUNC void *fio_io_listen FIO_NOOP(struct fio_io_listen_args args) {
  fio___io_listen_s *l = NULL;
  void *built_tls = NULL;
  int should_free_tls = !args.tls;
  FIO_STR_INFO_TMP_VAR(url_alt, 2048);
  if (!args.protocol) {
    FIO_LOG_ERROR("fio_io_listen requires a protocol to be assigned.");
    return l;
  }
  if (args.on_root && !fio_io_is_master()) {
    FIO_LOG_ERROR("fio_io_listen called with `on_root` by a non-root worker.");
    return l;
  }
  if (!args.url) {
    args.url = getenv("ADDRESS");
    if (!args.url)
      args.url = "0.0.0.0";
  }
  url_alt.len = strlen(args.url);
  if (url_alt.len > 2024) {
    FIO_LOG_ERROR("binding address / url too long.");
    args.url = NULL;
  }
  fio_url_s url = fio_url_parse(args.url, url_alt.len);
  if (url.scheme.buf &&
      (url.scheme.len > 2 && url.scheme.len < 5 &&
       (url.scheme.buf[0] | (char)0x20) == 't' &&
       (url.scheme.buf[1] | (char)0x20) == 'c') &&
      (url.scheme.buf[2] | (char)0x20) == 'p')
    url.scheme = FIO_BUF_INFO0;
  if (!url.port.buf && !url.scheme.buf) {
    static size_t port_counter = 3000;
    size_t port = fio_atomic_add(&port_counter, 1);
    if (port == 3000 && getenv("PORT")) {
      char *port_env = getenv("PORT");
      port = fio_atol10(&port_env);
      if (!port | (port > 65535ULL))
        port = 3000;
    }
    url_alt.len = 0;
    fio_string_write2(&url_alt,
                      NULL,
                      FIO_STRING_WRITE_STR2(url.scheme.buf, url.scheme.len),
                      (url.scheme.len ? FIO_STRING_WRITE_STR2("://", 3)
                                      : FIO_STRING_WRITE_STR2(NULL, 0)),
                      FIO_STRING_WRITE_STR2(url.host.buf, url.host.len),
                      FIO_STRING_WRITE_STR2(":", 1),
                      FIO_STRING_WRITE_NUM(port));
    args.url = url_alt.buf;
    url = fio_url_parse(args.url, url_alt.len);
  }

  args.tls = fio_io_tls_from_url(args.tls, url);
  fio___io_init_protocol_test(args.protocol, !!args.tls);
  built_tls = args.protocol->io_functions.build_context(args.tls, 0);
  fio_buf_info_s url_buf = FIO_BUF_INFO2((char *)args.url, url_alt.len);
  /* remove query details from URL */
  if (url.query.len)
    url_buf.len = url.query.buf - (url_buf.buf + 1);
  else if (url.target.len)
    url_buf.len = url.target.buf - (url_buf.buf + 1);
  l = (fio___io_listen_s *)
      FIO_MEM_REALLOC_(NULL, 0, sizeof(*l) + url_buf.len + 1, 0);
  FIO_ASSERT_ALLOC(l);
  FIO_LEAK_COUNTER_ON_ALLOC(fio_io_listen);
  *l = (fio___io_listen_s){
      .protocol = args.protocol,
      .udata = args.udata,
      .tls_ctx = built_tls,
      .queue_for_accept = args.queue_for_accept,
      .on_start = args.on_start,
      .on_stop = args.on_stop,
      .owner = FIO___IO.pid,
      .url_len = url_buf.len,
      .hide_from_log = args.hide_from_log,
  };
  FIO_MEMCPY(l->url, url_buf.buf, url_buf.len);
  l->url[l->url_len] = 0;
  if (should_free_tls)
    fio_io_tls_free(args.tls);

  l->fd = fio_sock_open2(l->url, FIO_SOCK_SERVER | FIO_SOCK_TCP);
  if (l->fd == -1) {
    fio___io_listen_free(l);
    return (l = NULL);
  }
  if (fio_io_is_running()) {
    fio_io_defer(fio___io_listen_attach_task_deferred, l, NULL);
  } else {
    fio_state_callback_add(
        (args.on_root ? FIO_CALL_PRE_START : FIO_CALL_ON_START),
        fio___io_listen_attach_task,
        (void *)l);
  }
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___io_listen_free, l);
  return l;
}

/* *****************************************************************************
Connecting as a Client
***************************************************************************** */

typedef struct {
  fio_io_protocol_s protocol;
  fio_io_protocol_s *upr;
  void (*on_failed)(fio_io_protocol_s *protocol, void *udata);
  void *udata;
  void *tls_ctx;
  size_t url_len;
  char url[];
} fio___io_connecting_s;

FIO_SFUNC void fio___connecting_cleanup(fio___io_connecting_s *c) {
  fio___io_func_free_context_caller(c->protocol.io_functions.free_context,
                                    c->tls_ctx);
  FIO_MEM_FREE_(c, sizeof(*c) + c->url_len + 1);
}

FIO_SFUNC void fio___connecting_on_close(void *buffer, void *udata) {
  fio___io_connecting_s *c = (fio___io_connecting_s *)udata;
  if (c->on_failed)
    c->on_failed(c->upr, c->udata);
  fio___connecting_cleanup(c);
  (void)buffer;
}

FIO_SFUNC void fio___connecting_on_ready(fio_io_s *io) {
  if (!fio_io_is_open(io))
    return;
  fio___io_connecting_s *c = (fio___io_connecting_s *)fio_io_udata(io);
  FIO_LOG_DEBUG2("(%d) established client connection to %s",
                 fio_io_pid(),
                 c->url);
  fio_io_udata_set(io, c->udata);
  fio_io_protocol_set(io, c->upr);
  c->on_failed = NULL;
  fio___io_defer_no_wakeup(fio___connecting_on_close, NULL, (void *)c);
}

void fio_io_connect___(void); /* IDE Marker */
SFUNC fio_io_s *fio_io_connect FIO_NOOP(fio_io_connect_args_s args) {
  int should_free_tls = !args.tls;
  if (!args.protocol)
    return NULL;
  if (!args.url) {
    if (args.on_failed)
      args.on_failed(args.protocol, args.udata);
    return NULL;
  }
  if (!args.timeout)
    args.timeout = 30000;

  size_t url_len = strlen(args.url);
  fio_url_s url = fio_url_parse(args.url, url_len);
  args.tls = fio_io_tls_from_url(args.tls, url);
  fio___io_init_protocol(args.protocol, !!args.tls);
  if (url.query.len)
    url_len = url.query.buf - (args.url + 1);
  else if (url.target.len)
    url_len = url.target.buf - (args.url + 1);
  fio___io_connecting_s *c = (fio___io_connecting_s *)
      FIO_MEM_REALLOC_(NULL, 0, sizeof(*c) + url_len + 1, 0);
  FIO_ASSERT_ALLOC(c);
  *c = (fio___io_connecting_s){
      .protocol =
          {
              .on_ready = fio___connecting_on_ready,
              .on_close = fio___connecting_on_close,
              .io_functions = args.protocol->io_functions,
              .timeout = args.timeout,
              .buffer_size = args.protocol->buffer_size,
          },
      .upr = args.protocol,
      .on_failed = args.on_failed,
      .udata = args.udata,
      .tls_ctx = args.protocol->io_functions.build_context(args.tls, 1),
  };
  FIO_MEMCPY(c->url, args.url, url_len);
  c->url[url_len] = 0;
  fio_io_s *io = fio_io_attach_fd(
      fio_sock_open2(c->url, FIO_SOCK_CLIENT | FIO_SOCK_NONBLOCK),
      &c->protocol,
      c,
      c->tls_ctx);
  if (should_free_tls)
    fio_io_tls_free(args.tls);
  return io;
}
/* *****************************************************************************
IO Reactor Finish
***************************************************************************** */
#endif /* FIO_IO */
