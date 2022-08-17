/* ************************************************************************* */
#ifndef H___FIO_CSTL_INCLUDE_ONCE___H /* Development inclusion - ignore line*/
#define FIO_POLL_ENGINE FIO_POLL_ENGINE_KQUEUE /* Development inclusion */
#include "330 poll api.h"                      /* Development inclusion */
#endif                                         /* Development inclusion */
#if FIO_POLL_ENGINE == FIO_POLL_ENGINE_KQUEUE
/* *****************************************************************************




                        POSIX Portable Polling with `kqueue`



Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
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
  fio_poll_destroy(p);
  fio_poll_init FIO_NOOP(p, p->settings);
}

/** Initializes the polling object, allocating its resources. */
FIO_IFUNC void fio_poll_init FIO_NOOP(fio_poll_s *p, fio_poll_settings_s args) {
  *p = (fio_poll_s){
      .settings = args,
      .fd = kqueue(),
  };
  if (p->fd == -1) {
    FIO_LOG_FATAL("couldn't open kqueue.\n");
    exit(errno);
  }
  FIO_POLL_VALIDATE(p->settings);
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___kqueue_after_fork, p);
}

/** Destroys the polling object, freeing its resources. */
FIO_IFUNC void fio_poll_destroy(fio_poll_s *p) {
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
                           int fd,
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

  const struct timespec timeout = {.tv_sec = (timeout_ / 1024),
                                   .tv_nsec =
                                       ((timeout_ & (1023UL)) * 1000000)};
  /* wait for events and handle them */
  int active_count =
      kevent(p->fd, NULL, 0, events, FIO_POLL_MAX_EVENTS, &timeout);

  if (active_count > 0) {
    for (int i = 0; i < active_count; i++) {
      // test for event(s) type
      if (events[i].filter == EVFILT_WRITE) {
        p->settings.on_ready(events[i].udata);
      } else if (events[i].filter == EVFILT_READ) {
        p->settings.on_data(events[i].udata);
      }
      if (events[i].flags & (EV_EOF | EV_ERROR)) {
        p->settings.on_close(events[i].udata);
      }
    }
  } else if (active_count < 0) {
    if (errno == EINTR)
      return 0;
    return -1;
  }
  return active_count;
}

/** Stops monitoring the specified file descriptor (if monitoring). */
SFUNC int fio_poll_forget(fio_poll_s *p, int fd) {
  int r = 0;
  if (p->fd == -1)
    return (r = -1);
  struct kevent chevent[2];
  EV_SET(chevent, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
  EV_SET(chevent + 1, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
  do {
    errno = 0;
    r = kevent(p->fd, chevent, 2, NULL, 0, NULL);
  } while (errno == EINTR);
  return r;
}

/* *****************************************************************************
Poll Monitoring Testing?
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, poll)(void) {
  fprintf(stderr,
          "* skipped testing file descriptor polling (engine: kqueue).\n");
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_POLL_ENGINE == FIO_POLL_ENGINE_KQUEUE */
