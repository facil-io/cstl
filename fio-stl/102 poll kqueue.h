/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO_POLL_ENGINE_KQUEUE /* Dev */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_POLL               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* ************************************************************************* */
#if defined(FIO_POLL) && defined(FIO_POLL_ENGINE_KQUEUE) &&                    \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)) &&                  \
    !defined(H___FIO_POLL_EGN___H) && !defined(H___FIO_POLL___H) &&            \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_POLL_EGN___H
/* *****************************************************************************




                        POSIX Portable Polling with `kqueue`



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#include <limits.h>
#include <sys/event.h>
/* *****************************************************************************
Polling API
***************************************************************************** */

/** the `fio_poll_s` type should be considered opaque. */
struct fio_poll_s {
  fio_poll_settings_s settings;
  int fd;
};

FIO_SFUNC void fio___kqueue_after_fork(void *p_) {
  fio_poll_s *p = (fio_poll_s *)p_;
  /* kqueue descriptors are not inherited by fork. The old numeric slot may
   * already belong to a different resource in this child. */
  p->fd = -1;
  fio_state_callback_remove(FIO_CALL_IN_CHILD, fio___kqueue_after_fork, p);
  (void)fio_poll_init FIO_NOOP(p, p->settings);
}

/** Initializes the polling object. Returns 0 on success, -1 on failure. */
IFUNC int fio_poll_init FIO_NOOP(fio_poll_s *p, fio_poll_settings_s args) {
  if (!p) {
    errno = EINVAL;
    return -1;
  }
  *p = (fio_poll_s){.settings = args, .fd = -1};
  p->fd = kqueue();
  if (p->fd == -1) {
    const int err = errno;
#if FIO_POLL_EXIT_ON_SYSERR
    FIO_LOG_FATAL("couldn't open kqueue.\n");
    exit(err);
#else
    errno = err;
    return -1;
#endif
  }
  /* kqueue() has no portable atomic CLOEXEC variant. */
  if (fcntl(p->fd, F_SETFD, FD_CLOEXEC) == -1) {
    const int err = errno;
    close(p->fd);
    p->fd = -1;
#if FIO_POLL_EXIT_ON_SYSERR
    FIO_LOG_FATAL("couldn't set close-on-exec for kqueue.\n");
    exit(err);
#else
    errno = err;
    return -1;
#endif
  }
  FIO_POLL_VALIDATE(p->settings);
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___kqueue_after_fork, p);
  return 0;
}

/** Destroys the polling object, freeing its resources. */
IFUNC void fio_poll_destroy(fio_poll_s *p) {
  if (p->fd != -1)
    close(p->fd);
  p->fd = -1;
  fio_state_callback_remove(FIO_CALL_IN_CHILD, fio___kqueue_after_fork, p);
}

/* *****************************************************************************
Poll Monitoring Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/**
 * Adds a file descriptor to be monitored, adds events to be monitored or
 * updates the monitored file's `udata`.
 *
 * Possible flags are: `POLLIN` and `POLLOUT`. Other flags may be set but might
 * be ignored.
 *
 * Monitoring mode is always one-shot. If an event if fired, it is removed from
 * the monitoring state.
 *
 * Returns -1 on error.
 */
SFUNC int fio_poll_monitor(fio_poll_s *p,
                           fio_socket_i fd,
                           void *udata,
                           unsigned short flags) {
  int r = -1;
  struct kevent chevent[2];
  int i = 0;
  if ((flags & POLLIN)) {
    EV_SET(chevent,
           fd,
           EVFILT_READ,
           EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT,
           0,
           0,
           udata);
    ++i;
  }
  if ((flags & POLLOUT)) {
    EV_SET(chevent + i,
           fd,
           EVFILT_WRITE,
           EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT,
           0,
           0,
           udata);
    ++i;
  }
  if (!i) {
    errno = EINVAL;
    return -1;
  }
  do {
    errno = 0;
  } while ((r = kevent(p->fd, chevent, i, NULL, 0, NULL)) == -1 &&
           errno == EINTR);
  return r;
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
SFUNC int fio_poll_review(fio_poll_s *p, size_t timeout_) {
  if (p->fd < 0)
    return -1;
  struct kevent events[FIO_POLL_MAX_EVENTS] = {{0}};

  /* Bound public waits as well as the conversion on narrow time_t systems. */
  if (timeout_ > INT_MAX)
    timeout_ = INT_MAX;
  const struct timespec timeout = {
      .tv_sec = (time_t)(timeout_ / 1000),
      .tv_nsec = (suseconds_t)((timeout_ % 1000) * 1000000)};
  /* wait for events and handle them */
  int active_count =
      kevent(p->fd, NULL, 0, events, FIO_POLL_MAX_EVENTS, &timeout);

  if (active_count > 0) {
    for (unsigned i = 0; i < (unsigned)active_count; i++) {
      // test for event(s) type
      if (events[i].filter == EVFILT_WRITE) {
        /* an error/EOF write event is a closure, not readiness (poll
         * backend parity): a failed non-blocking connect reports
         * writability together with EV_ERROR. */
        if (events[i].flags & (EV_EOF | EV_ERROR))
          p->settings.on_close(events[i].udata);
        else
          p->settings.on_ready(events[i].udata);
      }
      if (events[i].filter == EVFILT_READ)
        p->settings.on_data(events[i].udata);
      if (events[i].filter == EVFILT_READ &&
          (events[i].flags & (EV_EOF | EV_ERROR)))
        p->settings.on_close(events[i].udata);
    }
  } else if (active_count < 0) {
    if (errno == EINTR)
      return 0;
    return -1;
  }
  return active_count;
}

/** Stops monitoring the specified file descriptor (if monitoring). */
SFUNC int fio_poll_forget(fio_poll_s *p, fio_socket_i fd) {
  if (p->fd == -1)
    return -1;
  struct kevent change;
  int error = 0;
  const int16_t filters[2] = {EVFILT_READ, EVFILT_WRITE};
  for (size_t i = 0; i < 2; ++i) {
    EV_SET(&change, fd, filters[i], EV_DELETE, 0, 0, NULL);
    int r;
    do {
      r = kevent(p->fd, &change, 1, NULL, 0, NULL);
    } while (r == -1 && errno == EINTR);
    if (r == -1 && errno != ENOENT && !error)
      error = errno;
  }
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_POLL_ENGINE_KQUEUE */
