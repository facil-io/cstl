/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO_POLL_ENGINE_WEPOLL /* Dev */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_POLL               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* ************************************************************************* */
#if defined(FIO_POLL) && defined(FIO_POLL_ENGINE_WEPOLL) &&                    \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)) &&                  \
    !defined(H___FIO_POLL_EGN___H) && !defined(H___FIO_POLL___H) &&            \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_POLL_EGN___H
/* *****************************************************************************




                    Windows Native Polling with `wepoll`
                    (epoll API for Windows, by Bert Belder)



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */

/*
 * wepoll - epoll for Windows, BSD-2-Clause License
 * Copyright 2012-2020 Bert Belder <https://github.com/piscisaureus/wepoll>
 *
 * Embed minimal API declarations here so that no external wepoll.h is needed
 * at this point. Users must compile extras/wepoll.c alongside their project.
 * See extras/wepoll.h for download and build instructions.
 */

#include <winsock2.h>

/* Guard against redefinition of HANDLE (already defined by <windows.h>) */
#ifndef HANDLE
typedef void *HANDLE;
#endif

#ifndef WEPOLL_EXPORT
#define WEPOLL_EXPORT
#endif

/* epoll event flags — identical values to Linux epoll */
#ifndef EPOLLIN
#define EPOLLIN (1 << 0)
#endif
#ifndef EPOLLPRI
#define EPOLLPRI (1 << 1)
#endif
#ifndef EPOLLOUT
#define EPOLLOUT (1 << 2)
#endif
#ifndef EPOLLERR
#define EPOLLERR (1 << 3)
#endif
#ifndef EPOLLHUP
#define EPOLLHUP (1 << 4)
#endif
#ifndef EPOLLRDNORM
#define EPOLLRDNORM (1 << 6)
#endif
#ifndef EPOLLRDBAND
#define EPOLLRDBAND (1 << 7)
#endif
#ifndef EPOLLWRNORM
#define EPOLLWRNORM (1 << 8)
#endif
#ifndef EPOLLWRBAND
#define EPOLLWRBAND (1 << 9)
#endif
#ifndef EPOLLMSG
#define EPOLLMSG (1 << 10)
#endif
#ifndef EPOLLRDHUP
#define EPOLLRDHUP (1 << 13)
#endif
#ifndef EPOLLONESHOT
#define EPOLLONESHOT (1 << 30)
#endif

#ifndef EPOLL_CTL_ADD
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_MOD 2
#define EPOLL_CTL_DEL 3
#endif

typedef union epoll_data {
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
  SOCKET sock;
  HANDLE hnd;
} epoll_data_t;

struct epoll_event {
  uint32_t events;
  epoll_data_t data;
};

WEPOLL_EXPORT HANDLE epoll_create(int size);
WEPOLL_EXPORT HANDLE epoll_create1(int flags);
WEPOLL_EXPORT int epoll_close(HANDLE ephnd);
WEPOLL_EXPORT int epoll_ctl(HANDLE ephnd,
                            int op,
                            SOCKET sock,
                            struct epoll_event *ev);
WEPOLL_EXPORT int epoll_wait(HANDLE ephnd,
                             struct epoll_event *events,
                             int maxevents,
                             int timeout);

/* *****************************************************************************
Polling API
***************************************************************************** */

/** the `fio_poll_s` type should be considered opaque. */
struct fio_poll_s {
  fio_poll_settings_s settings;
  HANDLE ep; /* wepoll handle from epoll_create1(0) */
};

/** Initializes the polling object, allocating its resources. */
FIO_IFUNC void fio_poll_init FIO_NOOP(fio_poll_s *p, fio_poll_settings_s args) {
  *p = (fio_poll_s){
      .settings = args,
      .ep = epoll_create1(0),
  };
  if (p->ep == NULL) {
    FIO_LOG_FATAL("couldn't open wepoll handle.\n");
    exit(errno);
  }
  FIO_POLL_VALIDATE(p->settings);
  /* Windows has no fork(), so no FIO_CALL_IN_CHILD callback is needed. */
}

/** Destroys the polling object, freeing its resources. */
FIO_IFUNC void fio_poll_destroy(fio_poll_s *p) {
  if (p->ep)
    epoll_close(p->ep);
  p->ep = NULL;
}

/* *****************************************************************************
Poll Monitoring Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/**
 * Internal helper: register or update a socket in the wepoll instance.
 *
 * Tries EPOLL_CTL_MOD first; falls back to EPOLL_CTL_ADD when the socket is
 * not yet registered (wepoll maps that condition to errno == ENOENT).
 *
 * Returns 0 on success, -1 on error.
 */
FIO_IFUNC int fio___wepoll_add(HANDLE ep,
                               fio_socket_i fd,
                               void *udata,
                               uint32_t events) {
  int ret;
  struct epoll_event chevent;
  do {
    errno = 0;
    chevent = (struct epoll_event){
        .events = events,
        .data.ptr = udata,
    };
    ret = epoll_ctl(ep, EPOLL_CTL_MOD, (SOCKET)fd, &chevent);
    if (ret == -1 && errno == ENOENT) {
      errno = 0;
      chevent = (struct epoll_event){
          .events = events,
          .data.ptr = udata,
      };
      ret = epoll_ctl(ep, EPOLL_CTL_ADD, (SOCKET)fd, &chevent);
    }
  } while (errno == EINTR);
  return ret;
}

/**
 * Adds a file descriptor to be monitored, adds events to be monitored or
 * updates the monitored file's `udata`.
 *
 * Possible flags are: `POLLIN` and `POLLOUT`. `POLLPRI` is ignored on Windows.
 *
 * Monitoring mode is always one-shot. If an event is fired, it is removed from
 * the monitoring state.
 *
 * Returns -1 on error.
 */
SFUNC int fio_poll_monitor(fio_poll_s *p,
                           fio_socket_i fd,
                           void *udata,
                           unsigned short flags) {
  int r = 0;
  if ((flags & POLLOUT))
    r |= fio___wepoll_add(p->ep,
                          fd,
                          udata,
                          (EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT));
  if ((flags & POLLIN))
    r |= fio___wepoll_add(p->ep,
                          fd,
                          udata,
                          (EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT));
  return r;
}

/**
 * Stops monitoring the specified file descriptor (if monitoring).
 *
 * Returns 0 on success, -1 on error.
 */
SFUNC int fio_poll_forget(fio_poll_s *p, fio_socket_i fd) {
  if (!p->ep)
    return -1;
  return epoll_ctl(p->ep, EPOLL_CTL_DEL, (SOCKET)fd, NULL);
}

/**
 * Reviews if any of the monitored file descriptors has any events.
 *
 * `timeout` is in milliseconds.
 *
 * Returns the number of events called.
 *
 * Polling is thread safe, but has different effects on different threads.
 *
 * Adding a new file descriptor from one thread while polling in a different
 * thread will not poll that IO until `fio_poll_review` is called again.
 */
SFUNC int fio_poll_review(fio_poll_s *p, size_t timeout) {
  if (!p->ep)
    return -1;
  struct epoll_event events[FIO_POLL_MAX_EVENTS];

  /* epoll_wait timeout is in milliseconds; cast is safe (max ~49 days). */
  int active_count =
      epoll_wait(p->ep, events, FIO_POLL_MAX_EVENTS, (int)timeout);

  if (active_count > 0) {
    for (unsigned i = 0; i < (unsigned)active_count; i++) {
      if (events[i].events & EPOLLOUT)
        p->settings.on_ready(events[i].data.ptr);
      if (events[i].events & EPOLLIN)
        p->settings.on_data(events[i].data.ptr);
      if (events[i].events & ~(EPOLLIN | EPOLLOUT))
        p->settings.on_close(events[i].data.ptr);
    }
  } else if (active_count < 0) {
    if (errno == EINTR)
      return 0;
    return -1;
  }
  return active_count;
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_POLL_ENGINE_WEPOLL */
