/* ************************************************************************* */
#if !defined(H___FIO_CSTL_COMBINED___H) &&                                     \
    !defined(FIO___CSTL_NON_COMBINED_INCLUSION) /* Dev test - ignore line */
#define FIO___DEV___   /* Development inclusion - ignore line */
#define FIO_POLL       /* Development inclusion - ignore line */
#include "./include.h" /* Development inclusion - ignore line */
#endif                 /* Development inclusion - ignore line */
/* *****************************************************************************




                            POSIX Portable Polling



Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */
#if defined(FIO_POLL) && !defined(H___FIO_POLL___H) && !defined(FIO_STL_KEEP__)
#define H___FIO_POLL___H

#ifndef FIO_POLL_POSSIBLE_FLAGS
/** The user flags IO events recognize */
#define FIO_POLL_POSSIBLE_FLAGS (POLLIN | POLLOUT | POLLPRI)
#endif

#ifndef FIO_POLL_MAX_EVENTS
/** relevant only for epoll and kqueue - maximum number of events per review */
#define FIO_POLL_MAX_EVENTS FIO_QUEUE_TASKS_PER_ALLOC
#endif

/* *****************************************************************************
Possible polling engine (system call) selection
***************************************************************************** */

#ifndef FIO_POLL_ENGINE_POLL
/** define `FIO_POLL_ENGINE` as `FIO_POLL_ENGINE_POLL` to use `poll` */
#define FIO_POLL_ENGINE_POLL 1
#endif
#ifndef FIO_POLL_ENGINE_EPOLL
/** define `FIO_POLL_ENGINE` as `FIO_POLL_ENGINE_EPOLL` to use `epoll` */
#define FIO_POLL_ENGINE_EPOLL 2
#endif
#ifndef FIO_POLL_ENGINE_KQUEUE
/** define `FIO_POLL_ENGINE` as `FIO_POLL_ENGINE_KQUEUE` to use `kqueue` */
#define FIO_POLL_ENGINE_KQUEUE 3
#endif

/* if `FIO_POLL_ENGINE` wasn't define, detect automatically. */
#if !defined(FIO_POLL_ENGINE)
#if defined(HAVE_EPOLL) || __has_include("sys/epoll.h")
#define FIO_POLL_ENGINE FIO_POLL_ENGINE_EPOLL
#elif (defined(HAVE_KQUEUE) || __has_include("sys/event.h"))
#define FIO_POLL_ENGINE FIO_POLL_ENGINE_KQUEUE
#else
#define FIO_POLL_ENGINE FIO_POLL_ENGINE_POLL
#endif
#endif /* FIO_POLL_ENGINE */

#if FIO_POLL_ENGINE == FIO_POLL_ENGINE_POLL
#ifndef FIO_POLL_ENGINE_STR
#define FIO_POLL_ENGINE_STR "poll"
#endif
#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_EPOLL
#ifndef FIO_POLL_ENGINE_STR
#define FIO_POLL_ENGINE_STR "epoll"
#endif
#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_KQUEUE
#ifndef FIO_POLL_ENGINE_STR
#define FIO_POLL_ENGINE_STR "kqueue"
#endif
#endif
/* *****************************************************************************
Polling API
***************************************************************************** */

/** the `fio_poll_s` type should be considered opaque. */
typedef struct fio_poll_s fio_poll_s;

typedef struct {
  /** callback for when data is availabl in the incoming buffer. */
  void (*on_data)(void *udata);
  /** callback for when the outgoing buffer allows a call to `write`. */
  void (*on_ready)(void *udata);
  /** callback for closed connections and / or connections with errors. */
  void (*on_close)(void *udata);
} fio_poll_settings_s;

/** Initializes the polling object, allocating its resources. */
FIO_IFUNC void fio_poll_init(fio_poll_s *p, fio_poll_settings_s);
/** Initializes the polling object, allocating its resources. */
#define fio_poll_init(p, ...)                                                  \
  fio_poll_init((p), (fio_poll_settings_s){__VA_ARGS__})

/** Destroys the polling object, freeing its resources. */
FIO_IFUNC void fio_poll_destroy(fio_poll_s *p);

/** returns the system call used for polling as a constant string. */
FIO_IFUNC const char *fio_poll_engine(void);

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
SFUNC int fio_poll_review(fio_poll_s *p, size_t timeout);

/** Stops monitoring the specified file descriptor (if monitoring). */
SFUNC int fio_poll_forget(fio_poll_s *p, int fd);

/* *****************************************************************************
Implementation Helpers
***************************************************************************** */

/** returns the system call used for polling as a constant string. */
FIO_IFUNC const char *fio_poll_engine(void) { return FIO_POLL_ENGINE_STR; }

/* validate settings */
#define FIO_POLL_VALIDATE(settings_dest)                                       \
  if (!(settings_dest).on_data)                                                \
    (settings_dest).on_data = fio___poll_ev_mock;                              \
  if (!(settings_dest).on_ready)                                               \
    (settings_dest).on_ready = fio___poll_ev_mock;                             \
  if (!(settings_dest).on_close)                                               \
    (settings_dest).on_close = fio___poll_ev_mock;

SFUNC void fio___poll_ev_mock(void *udata);

#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
/* mock event */
SFUNC void fio___poll_ev_mock(void *udata) { (void)udata; }
#endif /* defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN) */
/* *****************************************************************************
Cleanup
***************************************************************************** */
#undef FIO_POLL
#endif /* FIO_POLL */
