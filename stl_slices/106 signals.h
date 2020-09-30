/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_ATOMIC                  /* Development inclusion - ignore line */
#define FIO_SIGNAL                  /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************










      A packet based data stream for storing / buffering endless data.










***************************************************************************** */
#if defined(FIO_SIGNAL) && !defined(H___FIO_SIGNAL___H)
#define H___FIO_SIGNAL___H

#ifndef FIO_SIGNAL_MONITOR_MAX
/* The maximum number of signals the implementation will be able to monitor */
#define FIO_SIGNAL_MONITOR_MAX 24
#endif

#if !FIO_HAVE_UNIX_TOOLS
#warning "POSIX is required for the fio_signal API."
#endif
#include <signal.h>
/* *****************************************************************************
Signal Monitoring API
***************************************************************************** */

/**
 * Starts to monitor for the specified signal, setting an optional callback.
 */
SFUNC int fio_signal_monitor(int sig,
                             void (*callback)(int sig, void *),
                             void *udata);

/** Reviews all signals, calling any relevant callbacks. */
SFUNC int fio_signal_review(void);

/** Stops monitoring the specified signal. */
SFUNC int fio_signal_forget(int sig);

/* *****************************************************************************








                          Signal Monitoring Implementation








***************************************************************************** */

/* *****************************************************************************
Signal Monitoring Implementation - inlined static functions
***************************************************************************** */

/* *****************************************************************************
Signal Monitoring Implementation - possibly externed functions.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

struct {
  int32_t sig;
  volatile int32_t flag;
  void (*callback)(int sig, void *);
  void *udata;
  struct sigaction old;
} fio___signal_watchers[FIO_SIGNAL_MONITOR_MAX];

FIO_SFUNC void fio___signal_catcher(int sig) {
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    if (!fio___signal_watchers[i].sig && !fio___signal_watchers[i].udata)
      return; /* initialized list is finishe */
    if (fio___signal_watchers[i].sig != sig)
      continue;
    /* mark flag */
    fio_atomic_exchange(&fio___signal_watchers[i].flag, 1);
    /* pass-through if exists */
    if (fio___signal_watchers[i].old.sa_handler != SIG_IGN &&
        fio___signal_watchers[i].old.sa_handler != SIG_DFL)
      fio___signal_watchers[i].old.sa_handler(sig);
    return;
  }
}

/**
 * Starts to monitor for the specified signal, setting an optional callback.
 */
SFUNC int fio_signal_monitor(int sig,
                             void (*callback)(int sig, void *),
                             void *udata) {
  if (!sig)
    return -1;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    /* updating an existing monitor */
    if (fio___signal_watchers[i].sig == sig) {
      fio___signal_watchers[i].callback = callback;
      fio___signal_watchers[i].udata = udata;
      return 0;
    }
    /* slot busy */
    if (fio___signal_watchers[i].sig || fio___signal_watchers[i].callback)
      continue;
    /* place monitor in this slot */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    memset(fio___signal_watchers + i, 0, sizeof(fio___signal_watchers[i]));
    fio___signal_watchers[i].sig = sig;
    fio___signal_watchers[i].callback = callback;
    fio___signal_watchers[i].udata = udata;
    act.sa_handler = fio___signal_catcher;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(sig, &act, &fio___signal_watchers[i].old)) {
      FIO_LOG_ERROR("couldn't set signal handler: %s", strerror(errno));
      fio___signal_watchers[i].callback = NULL;
      fio___signal_watchers[i].udata = (void *)1;
      fio___signal_watchers[i].sig = 0;
      return -1;
    }
    return 0;
  }
  return -1;
}

/** Reviews all signals, calling any relevant callbacks. */
SFUNC int fio_signal_review(void) {
  int c = 0;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    if (!fio___signal_watchers[i].sig && !fio___signal_watchers[i].udata)
      return c;
    if (fio_atomic_exchange(&fio___signal_watchers[i].flag, 0)) {
      ++c;
      if (fio___signal_watchers[i].callback)
        fio___signal_watchers[i].callback(fio___signal_watchers[i].sig,
                                          fio___signal_watchers[i].udata);
    }
  }
  return c;
}

/** Stops monitoring the specified signal. */
SFUNC int fio_signal_forget(int sig) {
  if (!sig)
    return -1;
  for (size_t i = 0; i < FIO_SIGNAL_MONITOR_MAX; ++i) {
    if (!fio___signal_watchers[i].sig && !fio___signal_watchers[i].udata)
      return -1; /* initialized list is finishe */
    if (fio___signal_watchers[i].sig != sig)
      continue;
    fio___signal_watchers[i].callback = NULL;
    fio___signal_watchers[i].udata = (void *)1;
    fio___signal_watchers[i].sig = 0;
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    if (sigaction(sig, &fio___signal_watchers[i].old, &act)) {
      FIO_LOG_ERROR("couldn't unset signal handler: %s", strerror(errno));
      return -1;
    }
    return 0;
  }
  return -1;
}

/* *****************************************************************************
Signal Monitoring Testing?
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, signal)(void) {

#define FIO___SIGNAL_MEMBER(a)                                                 \
  { (int)a, #a }
  struct {
    int sig;
    const char *name;
  } t[] = {
      FIO___SIGNAL_MEMBER(SIGHUP),
      FIO___SIGNAL_MEMBER(SIGINT),
      FIO___SIGNAL_MEMBER(SIGQUIT),
      FIO___SIGNAL_MEMBER(SIGILL),
      FIO___SIGNAL_MEMBER(SIGTRAP),
      FIO___SIGNAL_MEMBER(SIGABRT),
      FIO___SIGNAL_MEMBER(SIGBUS),
      FIO___SIGNAL_MEMBER(SIGFPE),
      FIO___SIGNAL_MEMBER(SIGUSR1),
      FIO___SIGNAL_MEMBER(SIGSEGV),
      FIO___SIGNAL_MEMBER(SIGUSR2),
      FIO___SIGNAL_MEMBER(SIGPIPE),
      FIO___SIGNAL_MEMBER(SIGALRM),
      FIO___SIGNAL_MEMBER(SIGTERM),
      FIO___SIGNAL_MEMBER(SIGCHLD),
      FIO___SIGNAL_MEMBER(SIGCONT),
  };
#undef FIO___SIGNAL_MEMBER
  size_t e = 0;
  fprintf(stderr, "* testing signal monitoring (setup / cleanup only).\n");
  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    if (fio_signal_monitor(t[i].sig, NULL, NULL)) {
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

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_SIGNAL_MONITOR_MAX
#endif /* FIO_SIGNAL */
#undef FIO_SIGNAL
