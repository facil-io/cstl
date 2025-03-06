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
 * Starts to monitor for the specified signal, setting an optional callback.
 */
SFUNC int fio_signal_monitor(fio_signal_monitor_args_s args);
#define fio_signal_monitor(...)                                                \
  fio_signal_monitor((fio_signal_monitor_args_s){__VA_ARGS__})

/** Reviews all signals, calling any relevant callbacks. */
SFUNC int fio_signal_review(void);

/** Stops monitoring the specified signal. */
SFUNC int fio_signal_forget(int sig);

/* *****************************************************************************




                          Signal Monitoring Implementation




***************************************************************************** */

/* *****************************************************************************
Signal Monitoring Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
POSIX implementation
***************************************************************************** */
#if defined(FIO_OS_POSIX)

static struct {
  fio_signal_monitor_args_s args;
  struct sigaction old;
  volatile uint16_t flag;
} fio___signal_watchers[FIO_SIGNAL_MONITOR_MAX];

FIO_SFUNC void fio___signal_catcher(int sig) {
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    if (!fio___signal_watchers[i].args.sig &&
        !fio___signal_watchers[i].args.udata)
      return; /* initialized list is finishe */
    if (fio___signal_watchers[i].args.sig != sig)
      continue;
    /* execute vs mark */
    if (fio___signal_watchers[i].args.immediate)
      fio___signal_watchers[i].args.callback(
          fio___signal_watchers[i].args.sig,
          fio___signal_watchers[i].args.udata);
    /* mark flag */
    fio___signal_watchers[i].flag = !fio___signal_watchers[i].args.immediate;

    /* pass-through if exists */
    if (fio___signal_watchers[i].args.propagate &&
        fio___signal_watchers[i].old.sa_handler != SIG_IGN &&
        fio___signal_watchers[i].old.sa_handler != SIG_DFL)
      fio___signal_watchers[i].old.sa_handler(sig);
    return;
  }
}

int fio_signal_monitor___(void); /* IDE Marker */
/**
 * Starts to monitor for the specified signal, setting an optional callback.
 */
SFUNC int fio_signal_monitor FIO_NOOP(fio_signal_monitor_args_s args) {
  if (!args.sig)
    return -1;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    /* updating an existing monitor */
    if (fio___signal_watchers[i].args.sig == args.sig) {
      fio___signal_watchers[i].args = args;
      return 0;
    }
    /* slot busy */
    if (fio___signal_watchers[i].args.sig ||
        fio___signal_watchers[i].args.callback)
      continue;
    /* place monitor in this slot */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    memset(fio___signal_watchers + i, 0, sizeof(fio___signal_watchers[i]));
    fio___signal_watchers[i].args = args;
    act.sa_handler = fio___signal_catcher;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(args.sig, &act, &fio___signal_watchers[i].old)) {
      FIO_LOG_ERROR("couldn't set signal handler: %s", strerror(errno));
      fio___signal_watchers[i].args = (fio_signal_monitor_args_s){
          .udata = (void *)1,
      };
      return -1;
    }
    return 0;
  }
  return -1;
}

/** Stops monitoring the specified signal. */
SFUNC int fio_signal_forget(int sig) {
  if (!sig)
    return -1;
  struct sigaction act = {0};
  act.sa_handler = SIG_DFL;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    if (!fio___signal_watchers[i].args.sig &&
        !fio___signal_watchers[i].args.udata)
      break; /* initialized list is finished */
    if (fio___signal_watchers[i].args.sig != sig)
      continue;
    fio___signal_watchers[i].args = (fio_signal_monitor_args_s){
        .udata = (void *)1,
    };
    struct sigaction old = fio___signal_watchers[i].old;
    old = act;
    if (sigaction(sig, &old, &act)) {
      FIO_LOG_ERROR("couldn't unset signal handler: %s", strerror(errno));
      return -1;
    }
    return 0;
  }
  sigaction(sig, &act, NULL);
  return -1;
}

/* *****************************************************************************
Windows Implementation
***************************************************************************** */
#elif FIO_OS_WIN

static struct {
  fio_signal_monitor_args_s args;
  void (*old)(int sig);
  volatile uint16_t flag;
} fio___signal_watchers[FIO_SIGNAL_MONITOR_MAX];

FIO_SFUNC void fio___signal_catcher(int sig) {
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    if (!fio___signal_watchers[i].args.sig &&
        !fio___signal_watchers[i].args.udata)
      return; /* initialized list is finished */
    if (fio___signal_watchers[i].args.sig != sig)
      continue;
    /* execute vs mark */
    if (fio___signal_watchers[i].args.immediate)
      fio___signal_watchers[i].args.callback(
          fio___signal_watchers[i].args.sig,
          fio___signal_watchers[i].args.udata);
    /* mark flag */
    fio___signal_watchers[i].flag = !fio___signal_watchers[i].args.immediate;
    /* pass-through if exists */
    if (fio___signal_watchers[i].args.propagate &&
        fio___signal_watchers[i].old &&
        (intptr_t)fio___signal_watchers[i].old != (intptr_t)SIG_IGN &&
        (intptr_t)fio___signal_watchers[i].old != (intptr_t)SIG_DFL) {
      fio___signal_watchers[i].old(sig);
      fio___signal_watchers[i].old = signal(sig, fio___signal_catcher);
    } else {
      fio___signal_watchers[i].old = signal(sig, fio___signal_catcher);
    }
    break;
  }
}

SFUNC int fio_signal_monitor___(void); /* IDE Marker */
/**
 * Starts to monitor for the specified signal, setting an optional callback.
 */
SFUNC int fio_signal_monitor FIO_NOOP(fio_signal_monitor_args_s args) {
  if (!args.sig)
    return -1;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    /* updating an existing monitor */
    if (fio___signal_watchers[i].args.sig == args.sig) {
      fio___signal_watchers[i].args = args;
      return 0;
    }
    /* slot busy */
    if (fio___signal_watchers[i].args.sig ||
        fio___signal_watchers[i].args.callback ||
        fio___signal_watchers[i].args.udata)
      continue;
    /* place monitor in this slot */
    fio___signal_watchers[i].args = args;
    fio___signal_watchers[i].old = signal(args.sig, fio___signal_catcher);
    if ((intptr_t)SIG_ERR == (intptr_t)fio___signal_watchers[i].old) {
      fio___signal_watchers[i].args = (fio_signal_monitor_args_s){
          .udata = (void *)1,
      };
      FIO_LOG_ERROR("couldn't set signal handler: %s", strerror(errno));
      return -1;
    }
    return 0;
  }
  return -1;
}

/** Stops monitoring the specified signal. */
SFUNC int fio_signal_forget(int sig) {
  if (!sig)
    return -1;
  size_t i = 0;
  for (; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    if (!fio___signal_watchers[i].args.sig &&
        !fio___signal_watchers[i].args.udata)
      return -1; /* initialized list is finished */
    if (fio___signal_watchers[i].args.sig != sig)
      continue;
    fio___signal_watchers[i].args = (fio_signal_monitor_args_s){
        .udata = (void *)1,
    };
    if (fio___signal_watchers[i].old &&
        fio___signal_watchers[i].old != SIG_DFL) {
      if ((intptr_t)signal(sig, fio___signal_watchers[i].old) ==
          (intptr_t)SIG_ERR)
        goto sig_error;
    } else {
      if ((intptr_t)signal(sig, SIG_DFL) == (intptr_t)SIG_ERR)
        goto sig_error;
    }
    fio___signal_watchers[i].old = SIG_DFL;
    return 0;
  }
  signal(sig, SIG_DFL);
  return -1;
sig_error:
  fio___signal_watchers[i].old = SIG_DFL;
  signal(sig, SIG_DFL);
  FIO_LOG_ERROR("couldn't unset signal handler: %s", strerror(errno));
  return -1;
}
#endif /* POSIX vs WINDOWS */

/* *****************************************************************************
Common OS implementation
***************************************************************************** */

/** Reviews all signals, calling any relevant callbacks. */
SFUNC int fio_signal_review(void) {
  int c = 0;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    if (!fio___signal_watchers[i].args.sig &&
        !fio___signal_watchers[i].args.udata)
      return c;
    if (fio___signal_watchers[i].flag) {
      fio___signal_watchers[i].flag = 0;
      ++c;
      if (fio___signal_watchers[i].args.callback)
        fio___signal_watchers[i].args.callback(
            fio___signal_watchers[i].args.sig,
            fio___signal_watchers[i].args.udata);
    }
  }
  return c;
}

/* *****************************************************************************
Signal Monitoring Testing?
***************************************************************************** */
#ifdef FIO_TEST_ALL
FIO_SFUNC void FIO_NAME_TEST(stl, signal)(void) {

#define FIO___SIGNAL_MEMBER(a)                                                 \
  { (int)a, #a }
  struct {
    int sig;
    const char *name;
  } t[] = {
    FIO___SIGNAL_MEMBER(SIGINT),
    FIO___SIGNAL_MEMBER(SIGILL),
    FIO___SIGNAL_MEMBER(SIGABRT),
    FIO___SIGNAL_MEMBER(SIGSEGV),
    FIO___SIGNAL_MEMBER(SIGTERM),
#if FIO_OS_POSIX
    FIO___SIGNAL_MEMBER(SIGQUIT),
    FIO___SIGNAL_MEMBER(SIGHUP),
    FIO___SIGNAL_MEMBER(SIGTRAP),
    FIO___SIGNAL_MEMBER(SIGBUS),
    FIO___SIGNAL_MEMBER(SIGFPE),
    FIO___SIGNAL_MEMBER(SIGUSR1),
    FIO___SIGNAL_MEMBER(SIGUSR2),
    FIO___SIGNAL_MEMBER(SIGPIPE),
    FIO___SIGNAL_MEMBER(SIGALRM),
    FIO___SIGNAL_MEMBER(SIGCHLD),
    FIO___SIGNAL_MEMBER(SIGCONT),
#endif
  };
#undef FIO___SIGNAL_MEMBER
  size_t e = 0;
  fprintf(stderr, "* testing signal monitoring (setup / cleanup only).\n");
  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    if (fio_signal_monitor(t[i].sig, NULL, NULL, 1)) {
      FIO_LOG_ERROR("couldn't set signal monitoring for %s (%d)",
                    t[i].name,
                    t[i].sig);
      e = 1;
    }
  }
  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    if (fio_signal_forget(t[i].sig)) {
      FIO_LOG_ERROR("couldn't stop signal monitoring for %s (%d)",
                    t[i].name,
                    t[i].sig);
      e = 1;
    }
  }
  FIO_ASSERT(!e, "signal monitoring error");
}

#endif /* FIO_TEST_ALL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_SIGNAL_MONITOR_MAX
#endif /* FIO_SIGNAL */
#undef FIO_SIGNAL
