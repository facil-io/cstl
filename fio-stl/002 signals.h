/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_SIGNAL             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                              Signal Monitoring



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_SIGNAL) && !defined(H___FIO_SIGNAL___H)
#define H___FIO_SIGNAL___H

#ifndef FIO_SIGNAL_MONITOR_MAX
/* The maximum number of signals the implementation will be able to monitor */
#define FIO_SIGNAL_MONITOR_MAX 24
#endif

#if !(FIO_OS_POSIX) && !(FIO_OS_WIN) /* use FIO_HAVE_UNIX_TOOLS instead? */
#error Either POSIX or Windows are required for the fio_signal API.
#endif

#include <signal.h>
/* ****************************************************************************
Portable signal aliases (abstract OS differences)
***************************************************************************** */

#if FIO_OS_POSIX
#define FIO_SIGNAL_USER1             SIGUSR1
#define FIO_SIGNAL_USER2             SIGUSR2
#define FIO_SIGNAL_USER_UNREGISTERED SIGUSR2
#elif FIO_OS_WIN
/* Windows has no SIGUSR1/SIGUSR2; map to safe, distinct signals. */
#define FIO_SIGNAL_USER1             SIGBREAK
#define FIO_SIGNAL_USER2             SIGABRT
/*
 * Use SIGBREAK for unregistered-signal tests to avoid clobbering SIGINT/SIGTERM
 * handlers that a test harness might rely on.
 */
#define FIO_SIGNAL_USER_UNREGISTERED SIGBREAK
#endif
/* *****************************************************************************
Signal Monitoring API
***************************************************************************** */

typedef struct {
  /** The signal number to listen for. */
  int sig;
  /** The callback to run - leave NULL to ignore signal. */
  void (*callback)(int sig, void *udata);
  /** Opaque user data. */
  void *udata;
  /** Should the signal propagate to existing handler(s)? */
  bool propagate;
  /** Call (safe) callback immediately? or wait for `fio_signal_review`? */
  bool immediate;
} fio_signal_monitor_args_s;
/**
 * Monitors a signal; NULL callback ignores delivery. Returns 0 or -1 on error.
 * Immediate callbacks / propagated handlers must be async-signal-safe and
 * must not call this API. Register, update and forget only outside signal
 * delivery; run review on a single thread.
 */
SFUNC int fio_signal_monitor(fio_signal_monitor_args_s args);
#define fio_signal_monitor(...)                                                \
  fio_signal_monitor((fio_signal_monitor_args_s){__VA_ARGS__})

/** Reviews all signals, calling any relevant callbacks. */
SFUNC int fio_signal_review(void);

/**
 * Stops monitoring the specified signal, restoring the handler that was
 * installed before `fio_signal_monitor` (if ours is still installed).
 */
SFUNC int fio_signal_forget(int sig);

/* *****************************************************************************




                          Signal Monitoring Implementation




***************************************************************************** */

/* *****************************************************************************
Signal Monitoring Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* Registration/removal are single-threaded phase-boundary operations; only
 * pending delivery is shared with signal handlers. */
typedef struct {
  fio_signal_monitor_args_s args;
#if FIO_OS_POSIX
  struct sigaction old;
#else
  void (*old)(int);
#endif
  unsigned pending;
} fio___signal_watch_s;
static fio___signal_watch_s fio___signal_watchers[FIO_SIGNAL_MONITOR_MAX];

#if FIO_OS_POSIX
FIO_SFUNC void fio___signal_catcher(int sig, siginfo_t *info, void *context) {
#else
FIO_SFUNC void fio___signal_catcher(int sig) {
#endif
  int saved_errno = errno;
#if FIO_OS_WIN
  /* The CRT may reset handlers on delivery. Keep the saved old handler. */
  if (sig != SIGFPE && signal(sig, fio___signal_catcher) == SIG_ERR) {
    errno = saved_errno;
    return;
  }
#endif
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    fio___signal_watch_s *w = fio___signal_watchers + i;
    if (w->args.sig != sig)
      continue;
    if (w->args.callback) {
      if (w->args.immediate)
        w->args.callback(sig, w->args.udata);
      else
        fio_atomic_or(&w->pending, 1U);
    }
    if (w->args.propagate) {
#if FIO_OS_POSIX
      if (w->old.sa_handler != SIG_DFL && w->old.sa_handler != SIG_IGN) {
        if (w->old.sa_flags & SA_SIGINFO)
          w->old.sa_sigaction(sig, info, context);
        else
          w->old.sa_handler(sig);
      }
#else
      if (w->old && w->old != SIG_DFL && w->old != SIG_IGN && w->old != SIG_ERR)
        w->old(sig);
#endif
    }
    break;
  }
  errno = saved_errno;
}

#if FIO_OS_POSIX
FIO_SFUNC int fio___signal_install(fio___signal_watch_s *w) {
  struct sigaction act = {0};
  act.sa_sigaction = fio___signal_catcher;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP;
  if (w->args.propagate) {
    act.sa_mask = w->old.sa_mask;
    act.sa_flags = SA_SIGINFO |
                   (w->old.sa_flags & (SA_RESTART | SA_NOCLDSTOP |
                                       SA_NOCLDWAIT | SA_ONSTACK | SA_NODEFER));
  }
  return sigaction(w->args.sig, &act, NULL);
}
#endif

int fio_signal_monitor___(void); /* IDE Marker */
SFUNC int fio_signal_monitor FIO_NOOP(fio_signal_monitor_args_s args) {
  if (args.sig <= 0)
    return -1;
  fio___signal_watch_s *free_slot = NULL;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    fio___signal_watch_s *w = fio___signal_watchers + i;
    if (w->args.sig == args.sig) {
#if FIO_OS_POSIX
      if (args.propagate && (w->old.sa_flags & SA_RESETHAND))
        return -1;
      if (args.propagate != w->args.propagate) {
        fio___signal_watch_s changed = *w;
        changed.args = args;
        if (fio___signal_install(&changed))
          return -1;
      }
#endif
      w->args = args;
      return 0;
    }
    if (!w->args.sig && !free_slot)
      free_slot = w;
  }
  if (!free_slot)
    return -1;
#if FIO_OS_POSIX
  if (sigaction(args.sig, NULL, &free_slot->old))
    return -1;
  if (args.propagate && (free_slot->old.sa_flags & SA_RESETHAND))
    return -1;
  free_slot->args = args;
  if (fio___signal_install(free_slot)) {
    free_slot->args = (fio_signal_monitor_args_s){0};
    return -1;
  }
#else
  /* The CRT only returns the old handler when installing a new one. */
  free_slot->args = args;
  free_slot->old = signal(args.sig, fio___signal_catcher);
  if (free_slot->old == SIG_ERR) {
    free_slot->args = (fio_signal_monitor_args_s){0};
    return -1;
  }
#endif
  return 0;
}

SFUNC int fio_signal_forget(int sig) {
  if (sig <= 0)
    return -1;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    fio___signal_watch_s *w = fio___signal_watchers + i;
    if (w->args.sig != sig)
      continue;
    /* Restore the handler saved by monitor, but only while ours is still
     * installed; a handler installed after ours is left untouched. */
#if FIO_OS_POSIX
    struct sigaction cur;
    if (sigaction(sig, NULL, &cur))
      return -1;
    if ((cur.sa_flags & SA_SIGINFO) &&
        cur.sa_sigaction == fio___signal_catcher &&
        sigaction(sig, &w->old, NULL))
      return -1;
#else
    /* The CRT can only query by installing; put back a foreign handler. */
    void (*cur)(int) = signal(sig, w->old);
    if (cur == SIG_ERR)
      return -1;
    if (cur != fio___signal_catcher && signal(sig, cur) == SIG_ERR)
      return -1;
#endif
    w->args = (fio_signal_monitor_args_s){0};
    fio_atomic_and(&w->pending, 0U);
    return 0;
  }
  return -1;
}

SFUNC int fio_signal_review(void) {
  int count = 0;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    fio___signal_watch_s *w = fio___signal_watchers + i;
    if (fio_atomic_and(&w->pending, 0U) && w->args.callback) {
      ++count;
      w->args.callback(w->args.sig, w->args.udata);
    }
  }
  return count;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_SIGNAL_MONITOR_MAX
#endif /* FIO_SIGNAL */
#undef FIO_SIGNAL
