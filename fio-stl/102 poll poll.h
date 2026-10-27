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
#if !FIO_OS_WIN && defined(POLLRDHUP)
#define FIO_POLL_EX_FLAGS POLLRDHUP
#else
#define FIO_POLL_EX_FLAGS 0
#endif
/* WSAPoll rejects POLLPRI, even when combined with a valid event. */
#if FIO_OS_WIN
#define FIO___POLL_REQUEST_FLAGS (FIO_POLL_POSSIBLE_FLAGS & ~POLLPRI)
#else
#define FIO___POLL_REQUEST_FLAGS FIO_POLL_POSSIBLE_FLAGS
#endif

/* Empty reviews have no pollfd to wait on. Keep each wait below 2^32 ns so
 * FIO_THREAD_WAIT remains safe on 32-bit platforms and for time_t / DWORD. */
FIO_IFUNC void fio___poll_wait_ms(size_t timeout) {
  while (timeout) {
    const size_t chunk = timeout > 4000 ? 4000 : timeout;
    FIO_THREAD_WAIT((uint64_t)chunk * UINT64_C(1000000));
    timeout -= chunk;
  }
}

typedef struct {
  void *udata;
  fio_socket_i fd;
  unsigned short flags;
  size_t generation;
} fio___poll_i_s;

#define FIO___POLL_IMAP_CMP(a, b) ((a)->fd == (b)->fd)
#define FIO___POLL_IMAP_HASH(o)   (fio_risky_ptr((void *)((uintptr_t)((o)->fd))))
/* Entries are always sockets: a live entry holds a valid socket descriptor.
 * Removed slots are zeroed by the imap (fd == 0), so validity must reject
 * both FIO_SOCKET_INVALID and zeroed slots (fd 0 is never monitored here) -
 * otherwise rehashes resurrect removed entries as fd 0 zombies, duplicated
 * fd 0 entries then fail every __fill_imap and the map expands without bound.
 */
#define FIO___POLL_IMAP_VALID(o) (FIO_SOCK_FD_ISVALID((o)->fd) && (o)->fd)
FIO_TYPEDEF_IMAP_ARRAY(fio___poll_map,
                       fio___poll_i_s,
                       uint32_t,
                       FIO___POLL_IMAP_HASH,
                       FIO___POLL_IMAP_CMP,
                       FIO___POLL_IMAP_VALID)
#undef FIO___POLL_IMAP_CMP
#undef FIO___POLL_IMAP_VALID
#undef FIO___POLL_IMAP_HASH

/* poll_review allocates this transient snapshot before consuming map flags. */
FIO_LEAK_COUNTER_DEF(fio___poll_review_buffer)

struct fio_poll_s {
  fio_poll_settings_s settings;
  fio___poll_map_s map;
  size_t next_generation;
  FIO___LOCK_TYPE lock;
};

/* *****************************************************************************
Poll Monitoring Implementation - inline static functions
***************************************************************************** */

/** Initializes the polling object. Returns 0 on success, -1 on error. */
IFUNC int fio_poll_init FIO_NOOP(fio_poll_s *p, fio_poll_settings_s args) {
  if (!p) {
    errno = EINVAL;
    return -1;
  }
  *p = (fio_poll_s){
      .settings = args,
      .map = {0},
      .lock = FIO___LOCK_INIT,
  };
  FIO_POLL_VALIDATE(p->settings);
  return 0;
}

/** Destroys the polling object, freeing its resources. */
IFUNC void fio_poll_destroy(fio_poll_s *p) {
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
  if ((revents & (POLLIN | POLLPRI)))
    p->settings.on_data(udata);
  if ((revents & (POLLHUP | POLLERR | POLLNVAL | FIO_POLL_EX_FLAGS)))
    p->settings.on_close(udata);
  else if ((revents & POLLOUT))
    p->settings.on_ready(udata);
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
  if (!p || !FIO_SOCK_FD_ISVALID(fd) || !fd)
    return r;
  r = 0;
#if FIO_OS_WIN
  /* Reject priority-only requests rather than poisoning the whole WSAPoll
   * batch; mixed requests retain their supported IN/OUT flags. */
  if ((flags & FIO_POLL_POSSIBLE_FLAGS) &&
      !(flags & FIO___POLL_REQUEST_FLAGS)) {
    errno = EINVAL;
    return -1;
  }
#endif
  flags &= FIO___POLL_REQUEST_FLAGS;
  flags |= FIO_POLL_EX_FLAGS;
  FIO___LOCK_LOCK(p->lock);
  fio___poll_i_s *ptr = fio___poll_map_get(&p->map, (fio___poll_i_s){.fd = fd});
  if (ptr) {
    /* re-arm: OR in new flags, always update udata */
    ptr->flags |= flags;
    ptr->udata = udata;
  } else {
    size_t generation = ++p->next_generation;
    if (!generation)
      generation = ++p->next_generation;
    if (!fio___poll_map_set(&p->map,
                            (fio___poll_i_s){.udata = udata,
                                             .fd = fd,
                                             .flags = flags,
                                             .generation = generation},
                            1))
      r = -1;
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
  int handled = 0;
  if (!p)
    return -1;

  /* Allocate before snapshotting, so failure leaves monitors intact. */
  FIO___LOCK_LOCK(p->lock);
  const size_t max = p->map.count;
  if (!max) {
    FIO___LOCK_UNLOCK(p->lock);
    fio___poll_wait_ms(timeout);
    return 0;
  }

  int r = 0, i = 0;
  struct pollfd *pfd = NULL;
  void **uary = NULL;
  size_t pfd_bytes = 0;
  size_t alloc_size = 0;
  size_t *generations = NULL;
  /* poll()/WSAPoll() receive an int-sized count here. Guard both that
   * narrowing and the one-block allocation arithmetic. */
  if (max > (size_t)INT_MAX ||
      max > ((SIZE_MAX - (sizeof(void *) - 1)) / sizeof(*pfd))) {
    FIO___LOCK_UNLOCK(p->lock);
    errno = ENOMEM;
    return -1;
  }
  pfd_bytes = (max * sizeof(*pfd) + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
  if (max > ((SIZE_MAX - pfd_bytes) / (sizeof(*uary) + sizeof(*generations)))) {
    FIO___LOCK_UNLOCK(p->lock);
    errno = ENOMEM;
    return -1;
  }
  alloc_size = pfd_bytes + max * (sizeof(*uary) + sizeof(*generations));
  pfd = (struct pollfd *)FIO_MEM_REALLOC_(NULL, 0, alloc_size, 0);
  if (!pfd) {
    FIO___LOCK_UNLOCK(p->lock);
    errno = ENOMEM;
    return -1;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___poll_review_buffer);

  /* Snapshot events while retaining the map allocation. This lets concurrent
   * monitor calls update the live map without a fallible merge afterwards. */
  fio_poll_s cpy = {.settings = p->settings};
  uary = (void **)((char *)pfd + pfd_bytes);
  generations = (size_t *)(uary + max);
  const unsigned short flag_mask = FIO___POLL_REQUEST_FLAGS | FIO_POLL_EX_FLAGS;
  FIO_IMAP_EACH(fio___poll_map, (&p->map), pos) {
    fio___poll_i_s *entry = p->map.ary + pos;
    if (!(entry->flags & flag_mask))
      continue;
    pfd[r].fd = entry->fd;
    pfd[r].events = (short)(entry->flags & flag_mask);
    uary[r] = entry->udata;
    generations[r] = entry->generation;
    entry->flags = 0;
    ++r;
  }
  FIO___LOCK_UNLOCK(p->lock);

  /* A consumed one-shot entry remains in the map so it can be re-armed, but
   * leaves an empty snapshot. WSAPoll rejects a zero descriptor count. */
  if (!r) {
    FIO_LEAK_COUNTER_ON_FREE(fio___poll_review_buffer);
    FIO_MEM_FREE_(pfd, alloc_size);
    fio___poll_wait_ms(timeout);
    return 0;
  }

  {
    /* clamp timeout: poll()/WSAPoll() take int; SIZE_MAX cast → negative → ∞ */
    int timeout_ms = (timeout > (size_t)INT_MAX) ? INT_MAX : (int)timeout;
#if FIO_OS_WIN
    events = WSAPoll(pfd, r, timeout_ms);
    if (events == -1)
      fio___sock_wsa_set_errno_last();
#else
    events = poll(pfd, r, timeout_ms);
#endif
  }
  if (events == -1 && errno == EINTR)
    events = 0;

  if (events > 0) {
    /* handle events and strip consumed flags */
    for (i = 0; i < r && handled < events; ++i) {
      if (!pfd[i].revents)
        continue;
      ++handled;
      /* strip fired flags — one-shot: consumed events are not re-queued.
       * Strip whole event groups: WSAPoll reports sub-band bits (e.g.
       * POLLRDNORM) while POLLIN/POLLOUT are supersets, so a raw bitwise
       * strip can leave band bits (e.g. POLLRDBAND) armed on Windows. */
      if (pfd[i].revents & POLLIN)
        pfd[i].events &= (short)~POLLIN;
      if (pfd[i].revents & POLLOUT)
        pfd[i].events &= (short)~POLLOUT;
      pfd[i].events &= (short)~pfd[i].revents;
      /* if a close/error event fired, disarm all remaining flags for this fd */
      if (pfd[i].revents & (POLLHUP | POLLERR | POLLNVAL | FIO_POLL_EX_FLAGS))
        pfd[i].events = 0;
      fio___poll_handle_events(&cpy, uary[i], pfd[i].revents);
    }
  }

  /* Restore flags that did not fire. A concurrent forget removes the retained
   * entry, preventing the stale fd from being re-added after this review. */
  FIO___LOCK_LOCK(p->lock);
  for (int j = 0; j < r; ++j) {
    if (!pfd[j].events)
      continue;
    fio___poll_i_s *entry =
        fio___poll_map_get(&p->map, (fio___poll_i_s){.fd = pfd[j].fd});
    if (entry && entry->generation == generations[j])
      entry->flags |= (unsigned short)pfd[j].events;
  }
  FIO___LOCK_UNLOCK(p->lock);
  FIO_LEAK_COUNTER_ON_FREE(fio___poll_review_buffer);
  FIO_MEM_FREE_(pfd, alloc_size);
  return events;
}

/** Stops monitoring the specified file descriptor, returning -1 on error. */
SFUNC int fio_poll_forget(fio_poll_s *p, fio_socket_i fd) {
  if (!p || !FIO_SOCK_FD_ISVALID(fd) || !fd)
    return -1;
  FIO___LOCK_LOCK(p->lock);
  int r = fio___poll_map_remove(&p->map, (fio___poll_i_s){.fd = fd});
  FIO___LOCK_UNLOCK(p->lock);
  return r;
}

/** Closes all sockets, calling the `on_close`. */
SFUNC void fio_poll_close_all(fio_poll_s *p) {
  if (!p)
    return;
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
#undef FIO___POLL_REQUEST_FLAGS
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_POLL_ENGINE_POLL */

#if defined(FIO_POLL) && !defined(H___FIO_POLL___H) &&                         \
    !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_POLL___H
#undef FIO_POLL
#endif /* FIO_POLL */
