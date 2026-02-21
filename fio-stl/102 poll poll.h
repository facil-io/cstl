/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO_POLL_ENGINE_POLL   /* Dev */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_POLL               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* ************************************************************************* */
#if defined(FIO_POLL) && defined(FIO_POLL_ENGINE_POLL) &&                      \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)) &&                  \
    !defined(H___FIO_POLL_EGN___H) && !defined(H___FIO_POLL___H) &&            \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_POLL_EGN___H
/* *****************************************************************************



                        POSIX Portable Polling with `poll`


Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#ifdef POLLRDHUP
#define FIO_POLL_EX_FLAGS POLLRDHUP
#else
#define FIO_POLL_EX_FLAGS 0
#endif

typedef struct {
  void *udata;
  fio_socket_i fd;
  unsigned short flags;
} fio___poll_i_s;

#define FIO___POLL_IMAP_CMP(a, b) ((a)->fd == (b)->fd)
#define FIO___POLL_IMAP_HASH(o)   (fio_risky_ptr((void *)((uintptr_t)((o)->fd))))
FIO_TYPEDEF_IMAP_ARRAY(fio___poll_map,
                       fio___poll_i_s,
                       uint32_t,
                       FIO___POLL_IMAP_HASH,
                       FIO___POLL_IMAP_CMP,
                       FIO_IMAP_ALWAYS_VALID)
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

/* Fire callbacks for the events that occurred (revents from poll/WSAPoll). */
FIO_IFUNC void fio___poll_handle_events(fio_poll_s *p,
                                        void *udata,
                                        unsigned short revents) {
  if ((revents & POLLOUT))
    p->settings.on_ready(udata);
  if ((revents & (POLLIN | POLLPRI)))
    p->settings.on_data(udata);
  if ((revents & (POLLHUP | POLLERR | POLLNVAL | FIO_POLL_EX_FLAGS)))
    p->settings.on_close(udata);
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
                           fio_socket_i fd,
                           void *udata,
                           unsigned short flags) {
  int r = -1;
  if (!p || fd == FIO_SOCKET_INVALID)
    return r;
  r = 0;
  flags &= FIO_POLL_POSSIBLE_FLAGS;
  flags |= FIO_POLL_EX_FLAGS;
  FIO___LOCK_LOCK(p->lock);
  fio___poll_i_s *ptr = fio___poll_map_get(&p->map, (fio___poll_i_s){.fd = fd});
  if (ptr) {
    /* re-arm: OR in new flags, always update udata */
    ptr->flags |= flags;
    ptr->udata = udata;
  } else {
    fio___poll_map_set(
        &p->map,
        (fio___poll_i_s){.udata = udata, .fd = fd, .flags = flags},
        1);
  }
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

  int r = 0, i = 0;
  /* Allocate pfd[] and uary[] in one block.
   * Pad the pfd[] section up to void* alignment before placing uary[]. */
  const size_t pfd_bytes = (max * sizeof(struct pollfd) + sizeof(void *) - 1) &
                           ~(sizeof(void *) - 1);
  const size_t alloc_size = pfd_bytes + max * sizeof(void *);
  struct pollfd *pfd =
      (struct pollfd *)FIO_MEM_REALLOC_(NULL, 0, alloc_size, 0);
  void **uary = (void **)((char *)pfd + pfd_bytes);

  FIO_IMAP_EACH(fio___poll_map, (&cpy.map), pos) {
    if (!(cpy.map.ary[pos].flags & flag_mask))
      continue;
    pfd[r].fd = cpy.map.ary[pos].fd;
#if FIO_OS_WIN
    /* POLLPRI is not supported by WSAPoll and causes WSAEINVAL */
    pfd[r].events = (short)(cpy.map.ary[pos].flags & (POLLIN | POLLOUT));
#else
    pfd[r].events = (short)(cpy.map.ary[pos].flags & FIO_POLL_POSSIBLE_FLAGS);
#endif
    uary[r] = cpy.map.ary[pos].udata;
    ++r;
  }

  {
    /* clamp timeout: poll()/WSAPoll() take int; SIZE_MAX cast → negative → ∞ */
    int timeout_ms = (timeout > (size_t)INT_MAX) ? INT_MAX : (int)timeout;
#if FIO_OS_WIN
    events = WSAPoll(pfd, r, timeout_ms);
#else
    events = poll(pfd, r, timeout_ms);
#endif
  }

  if (events > 0) {
    /* handle events and strip consumed flags */
    for (i = 0; i < r && handled < events; ++i) {
      if (!pfd[i].revents)
        continue;
      ++handled;
      /* strip fired flags — one-shot: consumed events are not re-queued */
      pfd[i].events &= (short)~pfd[i].revents;
      /* if a close/error event fired, disarm all remaining flags for this fd */
      if (pfd[i].revents & (POLLHUP | POLLERR | POLLNVAL | FIO_POLL_EX_FLAGS))
        pfd[i].events = 0;
      fio___poll_handle_events(&cpy, uary[i], pfd[i].revents);
    }
  }

  /* merge surviving (un-fired) entries from cpy back into p->map.
   * On timeout (events <= 0), ALL cpy entries are surviving.
   * Entries re-armed or forgotten by another thread during the poll
   * are already reflected in p->map; OR surviving flags in on top. */
  FIO___LOCK_LOCK(p->lock);
  if (!p->map.count && events <= 0) {
    /* fast path: nothing changed while we were polling — swap back */
    p->map = cpy.map;
    FIO___LOCK_UNLOCK(p->lock);
    FIO_MEM_FREE(pfd, alloc_size);
    return 0;
  }
  /* merge cpy entries that still have flags (un-fired) back into p->map */
  FIO_IMAP_EACH(fio___poll_map, (&cpy.map), pos) {
    fio___poll_i_s *src = &cpy.map.ary[pos];
    /* find the surviving flags for this fd from the pfd array */
    unsigned short surviving = src->flags; /* default: all flags (timeout) */
    for (int j = 0; j < r; ++j) {
      if (pfd[j].fd == src->fd) {
        surviving = (unsigned short)pfd[j].events;
        break;
      }
    }
    if (!surviving)
      continue; /* all events fired for this fd — truly one-shot, drop it */
    fio___poll_i_s *existing =
        fio___poll_map_get(&p->map, (fio___poll_i_s){.fd = src->fd});
    if (existing) {
      /* user re-armed during poll: OR surviving flags in; keep new udata */
      existing->flags |= surviving;
    } else {
      fio___poll_map_set(&p->map,
                         (fio___poll_i_s){
                             .fd = src->fd,
                             .flags = surviving,
                             .udata = src->udata,
                         },
                         1);
    }
  }
  FIO___LOCK_UNLOCK(p->lock);
  fio___poll_map_destroy(&cpy.map);
  FIO_MEM_FREE(pfd, alloc_size);
  return events;
}

/** Stops monitoring the specified file descriptor, returning -1 on error. */
SFUNC int fio_poll_forget(fio_poll_s *p, fio_socket_i fd) {
  FIO___LOCK_LOCK(p->lock);
  int r = fio___poll_map_remove(&p->map, (fio___poll_i_s){.fd = fd});
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
Cleanup
***************************************************************************** */
#undef FIO_POLL_EX_FLAGS
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_POLL_ENGINE_POLL */

#if defined(FIO_POLL) && !defined(H___FIO_POLL___H) &&                         \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_POLL___H
#undef FIO_POLL
#endif /* FIO_POLL */
