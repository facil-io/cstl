/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_ATOMIC                  /* Development inclusion - ignore line */
#define FIO_POLL                    /* Development inclusion - ignore line */
#define FIO_POLL_DEV                /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
#ifdef FIO_POLL_DEV                 /* Development inclusion - ignore line */
#include "201 array.h"              /* Development inclusion - ignore line */
#include "202 hashmap.h"            /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */

/* *****************************************************************************










      A packet based data stream for storing / buffering endless data.










***************************************************************************** */
#if defined(FIO_POLL) && !defined(H___FIO_POLL___H) && !defined(FIO_STL_KEEP__)
#define H___FIO_POLL___H

#if !FIO_HAVE_UNIX_TOOLS
#warning "POSIX is required for the fio_poll API."
#endif
#include <poll.h>

#ifndef FIO_POLL_HAS_UDATA_COLLECTION
/* A unique `udata` per fd (true)? or a global `udata` (false)?*/
#define FIO_POLL_HAS_UDATA_COLLECTION 1
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

#define FIO_POLL_INIT(on_data_func, on_ready_func, on_close_func)              \
  {                                                                            \
    .settings =                                                                \
        {                                                                      \
            .on_data = on_data_func,                                           \
            .on_ready = on_ready_func,                                         \
            .on_close = on_close_func,                                         \
        },                                                                     \
    .lock = FIO_LOCK_INIT                                                      \
  }

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
 * thread will not poll that IO untill `fio_poll_review` is called again.
 */
SFUNC int fio_poll_review(fio_poll_s *p, int timeout);

/**
 * Stops monitoring the specified file descriptor, returning its udata (if any).
 */
SFUNC void *fio_poll_forget(fio_poll_s *p, int fd);

/* *****************************************************************************








                          Poll Monitoring Implementation








***************************************************************************** */

/* *****************************************************************************
Poll Monitoring Implementation - The polling type(s)
***************************************************************************** */
#define FIO_STL_KEEP__

#define FIO_RISKY_HASH
#define FIO_MAP_TYPE         uint32_t
#define FIO_MAP_TYPE_INVALID -1 /* allow monitoring of fd == 0*/
#define FIO_MAP_NAME         fio___poll_index
#include __FILE__
#define FIO_ARRAY_TYPE           struct pollfd
#define FIO_ARRAY_NAME           fio___poll_fds
#define FIO_ARRAY_TYPE_CMP(a, b) (a.fd == b.fd)
#include __FILE__
#if FIO_POLL_HAS_UDATA_COLLECTION
#define FIO_ARRAY_TYPE void *
#define FIO_ARRAY_NAME fio___poll_udata
#include __FILE__
#endif /* FIO_POLL_HAS_UDATA_COLLECTION */

#ifdef FIO_STL_KEEP__
#undef FIO_STL_KEEP__
#endif

struct fio_poll_s {
  fio_poll_settings_s settings;
  fio___poll_index_s index;
  fio___poll_fds_s fds;
#if FIO_POLL_HAS_UDATA_COLLECTION
  fio___poll_udata_s udata;
#else
  void *udata;
#endif /* FIO_POLL_HAS_UDATA_COLLECTION */
  fio_lock_i lock;
};

/* *****************************************************************************
When avoiding the `udata` array
***************************************************************************** */
#if !FIO_POLL_HAS_UDATA_COLLECTION
FIO_IFUNC void fio___poll_udata_destroy(void **pu) { *pu = NULL; }
FIO_IFUNC void **fio___poll_udata_push(void **pu, void *udata) {
  if (udata)
    *pu = udata;
  return pu;
}
FIO_IFUNC void **fio___poll_udata_set(void **pu,
                                      int32_t pos,
                                      void *udata,
                                      void **ignr) {
  if (udata)
    *pu = udata;
  return pu;
  (void)ignr;
  (void)pos;
}
FIO_IFUNC void **fio___poll_udata2ptr(void **pu) { return pu; }
FIO_IFUNC void *fio___poll_udata_get(void **pu, int32_t pos) {
  return *pu;
  (void)pos;
}
#endif /* FIO_POLL_HAS_UDATA_COLLECTION */
/* *****************************************************************************
Poll Monitoring Implementation - inlined static functions
***************************************************************************** */

/* do we have a constructor? */
#ifndef FIO_REF_CONSTRUCTOR_ONLY
/* Allocates a new object on the heap and initializes it's memory. */
FIO_IFUNC fio_poll_s *fio_poll_new FIO_NOOP(fio_poll_settings_s settings) {
  fio_poll_s *p = (fio_poll_s *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*p), 0);
  if (p) {
    *p = (fio_poll_s) {
      .settings = settings, .lock = FIO_LOCK_INIT, .index = FIO_MAP_INIT,
      .fds = FIO_ARRAY_INIT,
#if FIO_POLL_HAS_UDATA_COLLECTION
      .udata = FIO_ARRAY_INIT,
#endif
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
  if (p) {
    fio___poll_index_destroy(&p->index);
    fio___poll_fds_destroy(&p->fds);
    fio___poll_udata_destroy(&p->udata);
    p->lock = FIO_LOCK_INIT;
  }
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

FIO_IFUNC int fio___poll_monitor(fio_poll_s *p,
                                 int fd,
                                 void *udata,
                                 unsigned short flags) {
  if (fd == -1)
    return -1;
  int32_t pos = fio___poll_index_get(&p->index, fd, 0);
  struct pollfd *i = fio___poll_fds2ptr(&p->fds);
  if (i && pos != -1 && i[pos].fd == fd)
    goto edit_existing;
  if (i && pos != -1 && i[pos].fd == -1)
    goto renew_monitoring;
  /* insert new entry */
  i = fio___poll_fds_push(&p->fds,
                          (struct pollfd){.fd = fd, .events = (short)flags});
  if (!i) {
    FIO_LOG_ERROR("fio___poll_monitor failed to push fd %d", fd);
    return -1;
  }
  pos = (uint32_t)(i - fio___poll_fds2ptr(&p->fds));
  if (!fio___poll_udata_push(&p->udata, udata)) {
    FIO_LOG_ERROR("fio___poll_monitor failed to push udata for fd %d", fd);
    fio___poll_fds_pop(&p->fds, NULL);
    return -1;
  }
  fio___poll_index_set(&p->index, fd, pos, NULL);
  return 0;

edit_existing:
  /* pos is correct, we are updating a value */
  i[pos].events |= flags;
  if (udata)
    fio___poll_udata_set(&p->udata, (int32_t)pos, udata, NULL);
  return 0;

renew_monitoring:
  i[pos].fd = fd;
  i[pos].events = flags;
  i[pos].revents = 0;
  fio___poll_udata_set(&p->udata, (int32_t)pos, udata, NULL);
  return 0;
}

/**
 * Adds a file descriptor to be monitored, adds events to be monitored or
 * updates the monitored file's `udata`.
 *
 * Possible flags are: `POLLIN` and `POLLOUT`. Other flags may be set but might
 * be ignored.
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
  flags &= POLLIN | POLLOUT | POLLPRI;
  fio_lock(&p->lock);
  r = fio___poll_monitor(p, fd, udata, flags);
  fio_unlock(&p->lock);
  return r;
}

/**
 * Reviews if any of the monitored file descriptors has any events.
 *
 * Returns the number of events called.
 *
 * Polling is thread safe, but has different effects on different threads.
 *
 * Adding a new file descriptor from one thread while polling in a different
 * thread will not poll that IO untill `fio_poll_review` is called again.
 */
SFUNC int fio_poll_review(fio_poll_s *p, int timeout) {
  int r = -1;
  int to_copy = 0;
  fio_poll_s cpy;
  if (!p)
    return r;

  /* move all data to a copy (thread safety) */
  fio_lock(&p->lock);
  cpy = *p;
  p->index = (fio___poll_index_s)FIO_MAP_INIT;
  p->fds = (fio___poll_fds_s)FIO_ARRAY_INIT;
#if FIO_POLL_HAS_UDATA_COLLECTION
  p->udata = (fio___poll_udata_s)FIO_ARRAY_INIT;
#endif /* FIO_POLL_HAS_UDATA_COLLECTION */

  fio_unlock(&p->lock);

  /* move if conditions out of the loop */
  if (!cpy.settings.on_data)
    cpy.settings.on_data = fio___poll_ev_mock;
  if (!cpy.settings.on_ready)
    cpy.settings.on_ready = fio___poll_ev_mock;
  if (!cpy.settings.on_close)
    cpy.settings.on_close = fio___poll_ev_mock;

  /* poll the array */
  struct pollfd *const fds_ary = fio___poll_fds2ptr(&cpy.fds);
  int const len = (int)fio___poll_fds_count(&cpy.fds);
  void **const ud_ary = fio___poll_udata2ptr(&cpy.udata);
  FIO_POLL_DEBUG_LOG("fio_poll_review reviewing %zu file descriptors.", len);
#if FIO_POLL_HAS_UDATA_COLLECTION
#define FIO___POLL_UDATA_GET(index) ud_ary[(index)]
#else
#define FIO___POLL_UDATA_GET(index) ud_ary[0]
#endif
  r = poll(fds_ary, len, timeout);

  /* process events */
  if (r > 0) {
    int i = 0;
    int c = 0;
    do {
      if ((fds_ary[i].revents & (POLLIN | POLLPRI))) {
        cpy.settings.on_data(fds_ary[i].fd, FIO___POLL_UDATA_GET(i));
        FIO_POLL_DEBUG_LOG("fio_poll_review calling `on_data` for %d.",
                           fds_ary[i].fd);
      }
      if ((fds_ary[i].revents & POLLOUT)) {
        cpy.settings.on_ready(fds_ary[i].fd, FIO___POLL_UDATA_GET(i));
        FIO_POLL_DEBUG_LOG("fio_poll_review calling `on_ready` for %d.",
                           fds_ary[i].fd);
      }
      if ((fds_ary[i].revents & (POLLHUP | POLLERR | POLLNVAL))) {
        cpy.settings.on_close(fds_ary[i].fd, FIO___POLL_UDATA_GET(i));
        fds_ary[i].events = 0; /* never retain events after closure / error */
        FIO_POLL_DEBUG_LOG("fio_poll_review calling `on_close` for %d.",
                           fds_ary[i].fd);
        /* if it was re-inserted to the queue, remove it */
        fio_poll_forget(p, fds_ary[i].fd);
      }
      fds_ary[i].events &= ~fds_ary[i].revents;
      if (fds_ary[i].events) {
        /* unfired events await */
        fds_ary[to_copy].fd = fds_ary[i].fd;
        fds_ary[to_copy].events = fds_ary[i].events;
        fds_ary[to_copy].revents = 0;
        FIO___POLL_UDATA_GET(to_copy) = FIO___POLL_UDATA_GET(i);
        ++to_copy;
        FIO_POLL_DEBUG_LOG("fio_poll_review %d still has pending events",
                           fds_ary[i].fd);
      } else {
        FIO_POLL_DEBUG_LOG("fio_poll_review no more events for %d",
                           fds_ary[i].fd);
      }
      /* any more events? */
      c += !!fds_ary[i].revents;
      ++i;
      if (i < len && c < r)
        continue;
      if (to_copy != i) {
        while (i < len) {
          FIO_POLL_DEBUG_LOG("fio_poll_review %d no-events-left mark copy",
                             fds_ary[i].fd);
          fds_ary[to_copy].fd = fds_ary[i].fd;
          fds_ary[to_copy].events = fds_ary[i].events;
          FIO_ASSERT(!fds_ary[i].revents,
                     "Event unhandlerd for %d",
                     fds_ary[i].fd);
          fds_ary[to_copy].revents = 0;
          FIO___POLL_UDATA_GET(to_copy) = FIO___POLL_UDATA_GET(i);
          ++to_copy;
          ++i;
        }
      } else {
        if (to_copy != len) {
          FIO_POLL_DEBUG_LOG("fio_poll_review no events left, quick mark");
        }
        to_copy = len;
      }
      break;
    } while (1);
  } else
    to_copy = len;

  /* insert all unfired events back to the (thread safe) queue */
  fio_lock(&p->lock);
  if (to_copy == len && !fio___poll_index_count(&p->index)) {
    /* it's possible to move the data set as is */
    FIO_POLL_DEBUG_LOG(
        "fio_poll_review overwriting %zu items for pending events",
        to_copy);
    *p = cpy;
    cpy = (fio_poll_s)FIO_POLL_INIT(NULL, NULL, NULL);
  } else {
    FIO_POLL_DEBUG_LOG("fio_poll_review copying %zu items with pending events",
                       to_copy);
    for (int i = 0; i < to_copy; ++i) {
      fio___poll_monitor(p,
                         fds_ary[i].fd,
                         FIO___POLL_UDATA_GET(i),
                         fds_ary[i].events);
    }
  }
  fio_unlock(&p->lock);

  /* cleanup memory */
  fio___poll_index_destroy(&cpy.index);
  fio___poll_fds_destroy(&cpy.fds);
  fio___poll_udata_destroy(&cpy.udata);
  return r;
#undef FIO___POLL_UDATA_GET
}

/**
 * Stops monitoring the specified file descriptor, returning its udata (if any).
 */
SFUNC void *fio_poll_forget(fio_poll_s *p, int fd) {
  void *old = NULL;
  FIO_POLL_DEBUG_LOG("fio_poll_forget called for %d", fd);
  if (!p || fd == -1 || !fio___poll_fds_count(&p->fds))
    return old;
  fio_lock(&p->lock);
  uint32_t pos = fio___poll_index_get(&p->index, fd, 0);
  if (fio___poll_fds_get(&p->fds, pos).fd == fd) {
    /* pos is correct (index 0 could have been a false positive) */
    fio___poll_index_remove(&p->index, fd, 0, NULL);
    fio___poll_fds2ptr(&p->fds)[(int32_t)pos].fd = -1;
    old = fio___poll_udata_get(&p->udata, (int32_t)pos);
  }
  fio_unlock(&p->lock);
  return old;
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
    }
  }
  for (int i = 128; i--;) {
    size_t pos = fio___poll_index_get(&p.index, i, 0);
    if ((i & 3) == 3) {
      FIO_ASSERT(fio___poll_fds_get(&p.fds, pos).fd != i, "fd wasn't removed?");
      FIO_ASSERT((int)(uintptr_t)fio___poll_udata_get(&p.udata, pos) != i,
                 "udata value wasn't removed?");
      continue;
    }
    FIO_ASSERT(fio___poll_fds_get(&p.fds, pos).fd == i,
               "index value [%zu] doesn't match fd (%d != %d)",
               pos,
               fio___poll_fds_get(&p.fds, pos).fd,
               i);
    FIO_ASSERT(fio___poll_fds_get(&p.fds, pos).events == events[(i & 3)],
               "events value isn't setup correctly");
    FIO_ASSERT((int)(uintptr_t)fio___poll_udata_get(&p.udata, pos) == i,
               "udata value isn't setup correctly");
  }
  fio_poll_destroy(&p);
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_POLL_HAS_UDATA_COLLECTION
#undef FIO_POLL
#endif /* FIO_POLL */
