/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_POLL                    /* Development inclusion - ignore line */
#define FIO_POLL_DEV                /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#include "104 sock.h"               /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
#ifdef FIO_POLL_DEV                 /* Development inclusion - ignore line */
#include "201 array.h"              /* Development inclusion - ignore line */
#include "210 map api.h"            /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */

/* *****************************************************************************




                        POSIX Portable Polling with `poll`




***************************************************************************** */
#if defined(FIO_POLL) && !defined(H___FIO_POLL___H) && !defined(FIO_STL_KEEP__)
#define H___FIO_POLL___H

#ifndef FIO_POLL_HAS_UDATA_COLLECTION
/** A unique `udata` per fd (true)? or a global `udata` (false)?*/
#define FIO_POLL_HAS_UDATA_COLLECTION 1
#endif

#ifndef FIO_POLL_POSSIBLE_FLAGS
/** The user flags IO events recognize */
#define FIO_POLL_POSSIBLE_FLAGS (POLLIN | POLLOUT | POLLPRI)
#endif

/* *****************************************************************************
Polling API
***************************************************************************** */

/** the `fio_poll_s` type should be considered opaque. */
typedef struct fio_poll_s fio_poll_s;

typedef struct {
  /** callback for when data is availabl in the incoming buffer. */
  void (*on_data)(int fd, void *udata);
  /** callback for when the outgoing buffer allows a call to `write`. */
  void (*on_ready)(int fd, void *udata);
  /** callback for closed connections and / or connections with errors. */
  void (*on_close)(int fd, void *udata);
} fio_poll_settings_s;

#if FIO_USE_THREAD_MUTEX_TMP
#define FIO_POLL_INIT(...)                                                     \
  { /* FIO_POLL_INIT(on_data_func, on_ready_func, on_close_func) */            \
    .settings = {__VA_ARGS__},                                                 \
    .lock = (fio_thread_mutex_t)FIO_THREAD_MUTEX_INIT                          \
  }
#else
#define FIO_POLL_INIT(...)                                                     \
  /* FIO_POLL_INIT(on_data_func, on_ready_func, on_close_func) */              \
  { .settings = {__VA_ARGS__}, .lock = FIO_LOCK_INIT }
#endif

#ifndef FIO_REF_CONSTRUCTOR_ONLY
/** Creates a new polling object / queue. */
FIO_IFUNC fio_poll_s *fio_poll_new(fio_poll_settings_s settings);
#define fio_poll_new(...) fio_poll_new((fio_poll_settings_s){__VA_ARGS__})

/** Frees the polling object and its resources. */
FIO_IFUNC int fio_poll_free(fio_poll_s *p);
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

/** Destroys the polling object, freeing its resources. */
FIO_IFUNC void fio_poll_destroy(fio_poll_s *p);

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
                           unsigned short flags);

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
SFUNC int fio_poll_review(fio_poll_s *p, int timeout);

/**
 * Stops monitoring the specified file descriptor, returning its udata (if any).
 */
SFUNC void *fio_poll_forget(fio_poll_s *p, int fd);

/** Closes all sockets, calling the `on_close`. */
SFUNC void fio_poll_close_all(fio_poll_s *p);

/* *****************************************************************************



                          Poll Monitoring Implementation



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

FIO_IFUNC uint64_t fio___poll_i_hash(fio___poll_i_s o) {
  return fio_risky_ptr((void *)(uintptr_t)(o.fd));
}
#define FIO_STL_KEEP__
#define FIO_RISKY_HASH
#define FIO_UMAP_NAME          fio___poll_map
#define FIO_MAP_TYPE           fio___poll_i_s
#define FIO_MAP_TYPE_CMP(a, b) ((a).fd == (b).fd)
#define FIO_MAP_TYPE_COPY(d, s)                                                \
  do {                                                                         \
    (d).udata = (s).udata;                                                     \
    (d).flags = ((d).flags * ((d).fd == (s).fd)) | (s).flags;                  \
    (d).fd = (s).fd;                                                           \
  } while (0)
#define FIO_MAP_HASH_FN(o) fio___poll_i_hash(o)
#include __FILE__

#ifdef FIO_STL_KEEP__
#undef FIO_STL_KEEP__
#endif

struct fio_poll_s {
  fio_poll_settings_s settings;
  fio___poll_map_s map;
  FIO___LOCK_TYPE lock;
};

FIO_IFUNC void fio___poll_map_set2(fio___poll_map_s *m, fio___poll_i_s o) {
  fio___poll_map_set(m, fio___poll_i_hash(o), o, NULL);
}

FIO_IFUNC fio___poll_i_s *fio___poll_map_get2(fio___poll_map_s *m, int fd) {
  fio___poll_i_s o = {.fd = fd};
  return fio___poll_map_get_ptr(m, fio___poll_i_hash(o), o);
}

FIO_IFUNC void fio___poll_map_remove2(fio___poll_map_s *m, int fd) {
  fio___poll_i_s *i = fio___poll_map_get2(m, fd);
  if (i) {
    i->flags = 0;
    i->udata = NULL;
    return;
  }
  fio___poll_map_set2(m, (fio___poll_i_s){.fd = fd});
}

/* *****************************************************************************
Poll Monitoring Implementation - inline static functions
***************************************************************************** */

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC fio_poll_s *fio_poll_new FIO_NOOP(fio_poll_settings_s settings) {
  fio_poll_s *p = (fio_poll_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*p), 0);
  if (p) {
    *p = (fio_poll_s){
        .settings = settings,
        .map = FIO_MAP_INIT,
        .lock = FIO___LOCK_INIT,
    };
  }
  return p;
}
/* Frees any internal data AND the object's container! */
FIO_IFUNC int fio_poll_free(fio_poll_s *p) {
  fio_poll_destroy(p);
  FIO_MEM_FREE_(p, sizeof(*p));
  return 0;
}
#endif /* FIO_REF_CONSTRUCTOR_ONLY */

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
#ifdef FIO_EXTERN_COMPLETE

#ifdef FIO_POLL_DEBUG
#define FIO_POLL_DEBUG_LOG FIO_LOG_DEBUG
#else
#define FIO_POLL_DEBUG_LOG(...)
#endif

/* mock event */
FIO_SFUNC void fio___poll_ev_mock(int fd, void *udata) {
  (void)fd;
  (void)udata;
}

/* validate settings */
FIO_SFUNC void fio___poll_validate(fio_poll_s *p) {
  if (!p->settings.on_data)
    p->settings.on_data = fio___poll_ev_mock;
  if (!p->settings.on_ready)
    p->settings.on_ready = fio___poll_ev_mock;
  if (!p->settings.on_close)
    p->settings.on_close = fio___poll_ev_mock;
}

FIO_IFUNC void fio___poll_validate_test(fio_poll_s *p) {
  if (!(((uintptr_t)p->settings.on_data) & ((uintptr_t)p->settings.on_ready) &
        ((uintptr_t)p->settings.on_close)))
    fio___poll_validate(p);
}

/* handle events, return a mask for possible remaining flags. */
FIO_IFUNC unsigned short fio___poll_handle_events(fio_poll_s *p,
                                                  int fd,
                                                  void *udata,
                                                  unsigned short flags) {
  if ((flags & POLLOUT))
    p->settings.on_ready(fd, udata);
  if ((flags & (POLLIN | POLLPRI)))
    p->settings.on_data(fd, udata);
  if ((flags & (POLLHUP | POLLERR | POLLNVAL | FIO_POLL_EX_FLAGS))) {
    p->settings.on_close(fd, udata);
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
  fio___poll_map_set2(&p->map, i);
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
SFUNC int fio_poll_review(fio_poll_s *p, int timeout) {
  int events = -1;
  int handled = -1;
  if (!p || !fio___poll_map_count(&p->map)) {
    if (timeout) {
      FIO_THREAD_WAIT((timeout * 1000000));
    }
    return 0;
  }
  fio___poll_validate_test(p);

  /* handle events in a copy, allowing events / threads to mutate it */
  FIO___LOCK_LOCK(p->lock);
  fio_poll_s cpy = *p;
  p->map = (fio___poll_map_s)FIO_MAP_INIT;
  FIO___LOCK_UNLOCK(p->lock);

  const size_t max = fio___poll_map_count(&cpy.map);
  const unsigned short flag_mask = FIO_POLL_POSSIBLE_FLAGS | FIO_POLL_EX_FLAGS;

  int w = 0, r = 0, i = 0;
  struct pollfd *pfd = (struct pollfd *)FIO_MEM_REALLOC_(
      NULL,
      0,
      ((max * sizeof(void *)) + (max * sizeof(struct pollfd))),
      0);
  void **uary = (void **)(pfd + max);

  FIO_MAP_EACH(fio___poll_map, (&cpy.map), pos) {
    if (!(pos->obj.flags & flag_mask))
      continue;
    pfd[r] =
        (struct pollfd){.fd = pos->obj.fd, .events = (short)pos->obj.flags};
    uary[r] = pos->obj.udata;
    ++r;
  }

#if FIO_OS_WIN
  events = WSAPoll(pfd, r, timeout);
#else
  events = poll(pfd, r, timeout);
#endif

  if (events > 0) {
    /* handle events and remove consumed entries */
    for (i = 0; i < r && handled < events; ++i) {
      if (pfd[i].revents) {
        ++handled;
        pfd[i].events &=
            fio___poll_handle_events(&cpy, pfd[i].fd, uary[i], pfd[i].revents);
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
  if (!fio___poll_map_count(&p->map) && events <= 0) {
    p->map = cpy.map;
    i = 1;
    goto finish;
  }
  if (w) {
    fio___poll_map_reserve(&p->map, w);
    for (i = 0; i < w; ++i) {
      fio___poll_i_s *existing = fio___poll_map_get2(&p->map, pfd[i].fd);
      if (existing && existing->fd == pfd[i].fd) {
        existing->flags |= (!!existing->flags) * (pfd[i].events);
        continue;
      }
      fio___poll_map_set2(&p->map,
                          (fio___poll_i_s){
                              .fd = pfd[i].fd,
                              .flags = (unsigned short)pfd[i].events,
                              .udata = uary[i],
                          });
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

/**
 * Stops monitoring the specified file descriptor, returning its udata (if any).
 */
SFUNC void *fio_poll_forget(fio_poll_s *p, int fd) {
  fio___poll_i_s *i = NULL;
  void *udata = NULL;
  FIO___LOCK_LOCK(p->lock);
  i = fio___poll_map_get2(&p->map, fd);
  if (i) {
    udata = i->udata;
    i->flags = 0;
    i->udata = NULL;
  }
  FIO___LOCK_UNLOCK(p->lock);
  return udata;
}

/** Closes all sockets, calling the `on_close`. */
SFUNC void fio_poll_close_all(fio_poll_s *p) {
  FIO___LOCK_LOCK(p->lock);
  fio_poll_s cpy = *p;
  p->map = (fio___poll_map_s)FIO_MAP_INIT;
  FIO___LOCK_UNLOCK(p->lock);
  const unsigned short flag_mask = FIO_POLL_POSSIBLE_FLAGS | FIO_POLL_EX_FLAGS;
  FIO_MAP_EACH(fio___poll_map, (&cpy.map), pos) {
    if ((pos->obj.flags & flag_mask)) {
      cpy.settings.on_close(pos->obj.fd, pos->obj.udata);
      fio_sock_close(pos->obj.fd);
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
      "* testing file descriptor monitoring (poll setup / cleanup only).\n");
  fio_poll_s p = FIO_POLL_INIT(NULL, NULL, NULL);
  short events[4] = {POLLOUT, POLLIN, POLLOUT | POLLIN, POLLOUT | POLLIN};
  for (int i = 128; i--;) {
    FIO_ASSERT(!fio_poll_monitor(&p, i, (void *)(uintptr_t)i, events[(i & 3)]),
               "fio_poll_monitor failed for fd %d",
               i);
  }
  for (int i = 128; i--;) {
    if ((i & 3) == 3) {
      FIO_ASSERT(fio_poll_forget(&p, i) == (void *)(uintptr_t)i,
                 "fio_poll_forget didn't return correct udata at %d",
                 i);
      FIO_ASSERT(fio_poll_forget(&p, i) == NULL,
                 "fio_poll_forget didn't forget udata at %d",
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
#endif /* FIO_POLL */
