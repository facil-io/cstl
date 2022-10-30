/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO_POLL_ENGINE FIO_POLL_ENGINE_EPOLL   /* Dev */
#define FIO___DEV___    /* Development inclusion - ignore line */
#define FIO_POLL        /* Development inclusion - ignore line */
#include "./include.h"  /* Development inclusion - ignore line */
#endif                  /* Development inclusion - ignore line */
/* ************************************************************************* */
#if (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)) &&                  \
    FIO_POLL_ENGINE == FIO_POLL_ENGINE_EPOLL &&                                \
    !defined(H___FIO_POLL_EGN___H) && defined(H___FIO_POLL___H) &&             \
    !defined(FIO_STL_KEEP__)
#define H___FIO_POLL_EGN___H
/* *****************************************************************************




                        POSIX Portable Polling with `epoll`



Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#include <sys/epoll.h>

/* *****************************************************************************
Polling API
***************************************************************************** */

/** the `fio_poll_s` type should be considered opaque. */
struct fio_poll_s {
  fio_poll_settings_s settings;
  struct pollfd fds[2];
  int fd[2];
};

FIO_SFUNC void fio___epoll_after_fork(void *p_) {
  fio_poll_s *p = (fio_poll_s *)p_;
  fio_poll_destroy(p);
  fio_poll_init FIO_NOOP(p, p->settings);
}

/** Initializes the polling object, allocating its resources. */
FIO_IFUNC void fio_poll_init FIO_NOOP(fio_poll_s *p, fio_poll_settings_s args) {
  *p = (fio_poll_s){
      .settings = args,
      .fds =
          {
              {.fd = epoll_create1(0), .events = (POLLIN | POLLOUT)},
              {.fd = epoll_create1(0), .events = (POLLIN | POLLOUT)},
          },
  };
  FIO_POLL_VALIDATE(p->settings);
  fio_state_callback_add(FIO_CALL_IN_CHILD, fio___epoll_after_fork, p);
}

/** Destroys the polling object, freeing its resources. */
FIO_IFUNC void fio_poll_destroy(fio_poll_s *p) {
  for (int i = 0; i < 2; ++i) {
    if (p->fds[i].fd != -1)
      close(p->fds[i].fd);
    p->fds[i].fd = -1;
  }
  fio_state_callback_remove(FIO_CALL_IN_CHILD, fio___epoll_after_fork, p);
}

/* *****************************************************************************
Poll Monitoring Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO_IFUNC int fio___epoll_add2(int fd,
                               void *udata,
                               uint32_t events,
                               int ep_fd) {
  int ret = 0;
  struct epoll_event chevent;
  do {
    errno = 0;
    chevent = (struct epoll_event){
        .events = events,
        .data.ptr = udata,
    };
    ret = epoll_ctl(ep_fd, EPOLL_CTL_MOD, fd, &chevent);
    if (ret == -1 && errno == ENOENT) {
      errno = 0;
      chevent = (struct epoll_event){
          .events = events,
          .data.ptr = udata,
      };
      ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, fd, &chevent);
    }
  } while (errno == EINTR);

  return ret;
}

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
  int r = 0;
  if ((flags & POLLOUT))
    r |= fio___epoll_add2(fd,
                          udata,
                          (EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT),
                          p->fds[0].fd);
  if ((flags & POLLIN))
    r |= fio___epoll_add2(fd,
                          udata,
                          (EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT),
                          p->fds[1].fd);
  return r;
}

/**
 * Stops monitoring the specified file descriptor, returning its udata (if any).
 */
SFUNC int fio_poll_forget(fio_poll_s *p, int fd) {
  int r = 0;
  struct epoll_event chevent = {.events = (EPOLLOUT | EPOLLIN)};
  r |= epoll_ctl(p->fds[0].fd, EPOLL_CTL_DEL, fd, &chevent);
  r |= epoll_ctl(p->fds[1].fd, EPOLL_CTL_DEL, fd, &chevent);
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
SFUNC int fio_poll_review(fio_poll_s *p, size_t timeout) {
  int total = 0;
  struct epoll_event events[FIO_POLL_MAX_EVENTS];
  /* wait for events and handle them */
  int internal_count = poll(p->fds, 2, timeout);
  if (internal_count <= 0)
    return total;
  int active_count = epoll_wait(p->fds[0].fd, events, FIO_POLL_MAX_EVENTS, 0);
  if (active_count > 0) {
    for (int i = 0; i < active_count; i++) {
      // errors are handled as disconnections (on_close) in the EPOLLIN queue
      // if no error, try an active event(s)
      if (events[i].events & EPOLLOUT)
        p->settings.on_ready(events[i].data.ptr);
    } // end for loop
    total += active_count;
  }
  active_count = epoll_wait(p->fds[1].fd, events, FIO_POLL_MAX_EVENTS, 0);
  if (active_count > 0) {
    for (int i = 0; i < active_count; i++) {
      // errors are handled as disconnections (on_close), but only once...
      if (events[i].events & (~(EPOLLIN | EPOLLOUT)))
        p->settings.on_close(events[i].data.ptr);
      // no error, then it's an active event(s)
      else if (events[i].events & EPOLLIN)
        p->settings.on_data(events[i].data.ptr);
    } // end for loop
    total += active_count;
  }
  return total;
}

/* *****************************************************************************
Poll Monitoring Testing?
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, poll)(void) {
  fprintf(stderr,
          "* skipped testing file descriptor polling (engine: epoll).\n");
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_POLL_ENGINE == FIO_POLL_ENGINE_EPOLL */
