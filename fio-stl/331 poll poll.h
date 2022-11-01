/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO_POLL_ENGINE FIO_POLL_ENGINE_POLL    /* Dev */
#define FIO___DEV___    /* Development inclusion - ignore line */
#define FIO_POLL        /* Development inclusion - ignore line */
#include "./include.h"  /* Development inclusion - ignore line */
#endif                  /* Development inclusion - ignore line */
/* ************************************************************************* */
#if (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)) &&                  \
    FIO_POLL_ENGINE == FIO_POLL_ENGINE_POLL &&                                 \
    !defined(H___FIO_POLL_EGN___H) && defined(H___FIO_POLL___H) &&             \
    !defined(FIO_STL_KEEP__)
#define H___FIO_POLL_EGN___H
/* *****************************************************************************



                        POSIX Portable Polling with `poll`


Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#ifdef POLLRDHUP
#define FIO_POLL_EX_FLAGS POLLRDHUP
#else
#define FIO_POLL_EX_FLAGS 0
#endif

typedef struct {
  void *udata;
  int fd;
  unsigned short flags;
} fio___poll_i_s;

#define FIO___POLL_IMAP_CMP(a, b) ((a)->fd == (b)->fd)
#define FIO___POLL_IMAP_VALID(o)  (1)
#define FIO___POLL_IMAP_HASH(o)   (fio_risky_ptr((void *)((uintptr_t)((o)->fd))))
FIO_TYPEDEF_IMAP_ARRAY(fio___poll_map,
                       fio___poll_i_s,
                       uint32_t,
                       FIO___POLL_IMAP_HASH,
                       FIO___POLL_IMAP_CMP,
                       FIO___POLL_IMAP_VALID)
#undef FIO___POLL_IMAP_CMP
#undef FIO___POLL_IMAP_VALID
#undef FIO___POLL_IMAP_HASH

struct fio_poll_s {
  fio_poll_settings_s settings;
  fio___poll_map_s map;
  FIO___LOCK_TYPE lock;
};

/* *****************************************************************************
Poll Monitoring Implementation - inline static functions
***************************************************************************** */

/** Initializes the polling object, allocating its resources. */
FIO_IFUNC void fio_poll_init FIO_NOOP(fio_poll_s *p, fio_poll_settings_s args) {
  if (p) {
    *p = (fio_poll_s){
        .settings = args,
        .map = {0},
        .lock = FIO___LOCK_INIT,
    };
    FIO_POLL_VALIDATE(p->settings);
  }
}

/** Destroys the polling object, freeing its resources. */
FIO_IFUNC void fio_poll_destroy(fio_poll_s *p) {
  if (!p)
    return;
  fio___poll_map_destroy(&p->map);
  FIO___LOCK_DESTROY(p->lock);
}

/* *****************************************************************************
Poll Monitoring Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* handle events, return a mask for possible remaining flags. */
FIO_IFUNC unsigned short fio___poll_handle_events(fio_poll_s *p,
                                                  void *udata,
                                                  unsigned short flags) {
  if ((flags & POLLOUT))
    p->settings.on_ready(udata);
  if ((flags & (POLLIN | POLLPRI)))
    p->settings.on_data(udata);
  if ((flags & (POLLHUP | POLLERR | POLLNVAL | FIO_POLL_EX_FLAGS))) {
    p->settings.on_close(udata);
    return 0;
  }
  return ~flags;
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
  int r = -1;
  if (!p || fd == -1)
    return r;
  r = 0;
  flags &= FIO_POLL_POSSIBLE_FLAGS;
  flags |= FIO_POLL_EX_FLAGS;
  fio___poll_i_s i = {.udata = udata, .fd = fd, .flags = flags};
  FIO___LOCK_LOCK(p->lock);
  fio___poll_i_s *ptr = fio___poll_map_set(&p->map, i, 0);
  ptr->flags |= flags;
  FIO___LOCK_UNLOCK(p->lock);
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
  int events = -1;
  int handled = -1;
  if (!p || !(p->map.count)) {
    if (timeout) {
      FIO_THREAD_WAIT((timeout * 1000000));
    }
    return 0;
  }
  /* handle events in a copy, allowing events / threads to mutate it */
  FIO___LOCK_LOCK(p->lock);
  fio_poll_s cpy = *p;
  p->map = (fio___poll_map_s){0};
  FIO___LOCK_UNLOCK(p->lock);

  const size_t max = cpy.map.count;
  const unsigned short flag_mask = FIO_POLL_POSSIBLE_FLAGS | FIO_POLL_EX_FLAGS;

  int w = 0, r = 0, i = 0;
  struct pollfd *pfd = (struct pollfd *)FIO_MEM_REALLOC_(
      NULL,
      0,
      ((max * sizeof(void *)) + (max * sizeof(struct pollfd))),
      0);
  void **uary = (void **)(pfd + max);

  FIO_IMAP_EACH(fio___poll_map, (&cpy.map), pos) {
    if (!(cpy.map.ary[pos].flags & flag_mask))
      continue;
    pfd[r] = (struct pollfd){.fd = cpy.map.ary[pos].fd,
                             .events = (short)cpy.map.ary[pos].flags};
    uary[r] = cpy.map.ary[pos].udata;
    ++r;
  }

#if FIO_OS_WIN
  events = WSAPoll(pfd, r, (int)timeout);
#else
  events = poll(pfd, r, (int)timeout);
#endif

  if (events > 0) {
    /* handle events and remove consumed entries */
    for (i = 0; i < r && handled < events; ++i) {
      if (pfd[i].revents) {
        ++handled;
        pfd[i].events &=
            fio___poll_handle_events(&cpy, uary[i], pfd[i].revents);
      }
      if ((pfd[i].events & (~(FIO_POLL_EX_FLAGS)))) {
        if (i != w) {
          pfd[w] = pfd[i];
          uary[w] = uary[i];
        }
        ++w;
      }
    }
    if (i < r && i != w) {
      memmove(pfd + w, pfd + i, ((r - i) * sizeof(*pfd)));
      memmove(uary + w, uary + i, ((r - i) * sizeof(*uary)));
    }
  }
  w += r - i;
  i = 0;

  FIO___LOCK_LOCK(p->lock);
  if (!p->map.count && events <= 0) {
    p->map = cpy.map;
    i = 1;
    goto finish;
  }
  if (w) {
    fio___poll_map_reserve(&p->map, w + p->map.count);
    for (i = 0; i < w; ++i) {
      fio___poll_i_s *existing =
          fio___poll_map_get(&p->map, (fio___poll_i_s){.fd = pfd[i].fd});
      if (existing) {
        existing->flags |= existing->flags ? pfd[i].events : 0;
        continue;
      }
      fio___poll_map_set(&p->map,
                         (fio___poll_i_s){
                             .fd = pfd[i].fd,
                             .flags = (unsigned short)pfd[i].events,
                             .udata = uary[i],
                         },
                         1);
    }
  }
  i = 0;

finish:
  FIO___LOCK_UNLOCK(p->lock);
  FIO_MEM_FREE(pfd, ((max * sizeof(void *)) + (max * sizeof(struct pollfd))));
  if (!i)
    fio___poll_map_destroy(&cpy.map);
  return events;
}

/** Stops monitoring the specified file descriptor, returning -1 on error. */
SFUNC int fio_poll_forget(fio_poll_s *p, int fd) {
  int r = 0;
  fio___poll_i_s i = {.fd = fd};
  FIO___LOCK_LOCK(p->lock);
  fio___poll_i_s *ptr = fio___poll_map_set(&p->map, i, 0);
  ptr->flags = 0;
  FIO___LOCK_UNLOCK(p->lock);
  return r;
}

/** Closes all sockets, calling the `on_close`. */
SFUNC void fio_poll_close_all(fio_poll_s *p) {
  FIO___LOCK_LOCK(p->lock);
  fio_poll_s cpy = *p;
  p->map = (fio___poll_map_s){0};
  FIO___LOCK_UNLOCK(p->lock);
  const unsigned short flag_mask = FIO_POLL_POSSIBLE_FLAGS | FIO_POLL_EX_FLAGS;
  FIO_IMAP_EACH(fio___poll_map, (&cpy.map), pos) {
    if ((cpy.map.ary[pos].flags & flag_mask)) {
      cpy.settings.on_close(cpy.map.ary[pos].udata);
      fio_sock_close(cpy.map.ary[pos].fd);
    }
  }
  fio___poll_map_destroy(&cpy.map);
}
/* *****************************************************************************
Poll Monitoring Testing?
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, poll)(void) {
  fprintf(
      stderr,
      "* Testing file descriptor monitoring (poll setup / cleanup only).\n");
  fio_poll_s p;
  fio_poll_init(&p, NULL);
  short events[4] = {POLLOUT, POLLIN, POLLOUT | POLLIN, POLLOUT | POLLIN};
  for (int i = 128; i--;) {
    FIO_ASSERT(!fio_poll_monitor(&p, i, (void *)(uintptr_t)i, events[(i & 3)]),
               "fio_poll_monitor failed for fd %d",
               i);
  }
  for (int i = 128; i--;) {
    if ((i & 3) == 3) {
      FIO_ASSERT(!fio_poll_forget(&p, i), "fio_poll_forget failed at %d", i);
      FIO_ASSERT(fio_poll_forget(&p, i),
                 "fio_poll_forget didn't forget previous %d",
                 i);
    }
  }
  fio_poll_destroy(&p);
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Cleanup
***************************************************************************** */
#undef FIO_POLL_EX_FLAGS
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_POLL_ENGINE == FIO_POLL_ENGINE_POLL */
