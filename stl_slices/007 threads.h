/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_THREADS                 /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                        Simple Portable Threads




***************************************************************************** */
#if defined(FIO_THREADS) && !defined(H___FIO_THREADS___H)
#define H___FIO_THREADS___H

/* *****************************************************************************
Module Settings

At this point, define any MACROs and customaizable settings avsailable to the
developer.
***************************************************************************** */

#if FIO_OS_POSIX
#include <pthread.h>
#include <sched.h>
typedef pthread_t fio_thread_t;
typedef pthread_mutex_t fio_thread_mutex_t;
/** Used this macro for static initialization. */
#define FIO_THREAD_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER

#elif FIO_OS_WIN
#include <synchapi.h>
typedef HANDLE fio_thread_t;
typedef HANDLE fio_thread_mutex_t;
/** Used this macro for static initialization. */
#define FIO_THREAD_MUTEX_INIT ((fio_thread_mutex_t)0)
#else
#error facil.io Simple Portable Threads require a POSIX system or Windows
#endif

#ifdef FIO_THREADS_BYO
#define FIO_IFUNC_T
#else
#define FIO_IFUNC_T FIO_IFUNC
#endif

#ifdef FIO_THREADS_MUTEX_BYO
#define FIO_IFUNC_M
#else
#define FIO_IFUNC_M FIO_IFUNC
#endif

/* *****************************************************************************
Module API
***************************************************************************** */

/** Starts a new thread, returns 0 on success and -1 on failure. */
FIO_IFUNC_T int fio_thread_create(fio_thread_t *t,
                                  void *(*fn)(void *),
                                  void *arg);

/** Waits for the thread to finish. */
FIO_IFUNC_T int fio_thread_join(fio_thread_t t);

/** Detaches the thread, so thread resources are freed automatically. */
FIO_IFUNC_T int fio_thread_detach(fio_thread_t t);

/** Ends the current running thread. */
FIO_IFUNC_T void fio_thread_exit(void);

/* Returns non-zero if both threads refer to the same thread. */
FIO_IFUNC_T int fio_thread_equal(fio_thread_t a, fio_thread_t b);

/** Returns the current thread. */
FIO_IFUNC_T fio_thread_t fio_thread_current(void);

/** Yields thread execution. */
FIO_IFUNC_T void fio_thread_yield(void);

/**
 * Initializes a simple Mutex.
 *
 * Or use the static initialization value: FIO_THREAD_MUTEX_INIT
 */
FIO_IFUNC_M int fio_thread_mutex_init(fio_thread_mutex_t *m);

/** Locks a simple Mutex, returning -1 on error. */
FIO_IFUNC_M int fio_thread_mutex_lock(fio_thread_mutex_t *m);

/** Attempts to lock a simple Mutex, returning zero on success. */
FIO_IFUNC_M int fio_thread_mutex_trylock(fio_thread_mutex_t *m);

/** Unlocks a simple Mutex, returning zero on success or -1 on error. */
FIO_IFUNC_M int fio_thread_mutex_unlock(fio_thread_mutex_t *m);

/** Destroys the simple Mutex (cleanup). */
FIO_IFUNC_M void fio_thread_mutex_destroy(fio_thread_mutex_t *m);

/* *****************************************************************************
POSIX Implementation - inlined static functions
***************************************************************************** */
#if FIO_OS_POSIX
#ifndef FIO_THREADS_BYO
// clang-format off
/** Starts a new thread, returns 0 on success and -1 on failure. */
FIO_IFUNC int fio_thread_create(fio_thread_t *t, void *(*fn)(void *), void *arg) { return pthread_create(t, NULL, fn, arg); }

FIO_IFUNC int fio_thread_join(fio_thread_t t) { return pthread_join(t, NULL); }

/** Detaches the thread, so thread resources are freed automatically. */
FIO_IFUNC int fio_thread_detach(fio_thread_t t) { return pthread_detach(t); }

/** Ends the current running thread. */
FIO_IFUNC void fio_thread_exit(void) { pthread_exit(NULL); }

/* Returns non-zero if both threads refer to the same thread. */
FIO_IFUNC int fio_thread_equal(fio_thread_t a, fio_thread_t b) { return pthread_equal(a, b); }

/** Returns the current thread. */
FIO_IFUNC fio_thread_t fio_thread_current(void) { return pthread_self(); }

/** Yields thread execution. */
FIO_IFUNC void fio_thread_yield(void) { sched_yield(); }

#endif /* FIO_THREADS_BYO */
#ifndef FIO_THREADS_MUTEX_BYO

/** Initializes a simple Mutex. */
FIO_IFUNC int fio_thread_mutex_init(fio_thread_mutex_t *m) { return pthread_mutex_init(m, NULL); }

/** Locks a simple Mutex, returning -1 on error. */
FIO_IFUNC int fio_thread_mutex_lock(fio_thread_mutex_t *m) { return pthread_mutex_lock(m); }

/** Attempts to lock a simple Mutex, returning zero on success. */
FIO_IFUNC int fio_thread_mutex_trylock(fio_thread_mutex_t *m) { return pthread_mutex_trylock(m); }

/** Unlocks a simple Mutex, returning zero on success or -1 on error. */
FIO_IFUNC int fio_thread_mutex_unlock(fio_thread_mutex_t *m) { return pthread_mutex_unlock(m); }

/** Destroys the simple Mutex (cleanup). */
FIO_IFUNC void fio_thread_mutex_destroy(fio_thread_mutex_t *m) { pthread_mutex_destroy(m); *m = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER; }

#endif /* FIO_THREADS_MUTEX_BYO */
// clang-format on
/* *****************************************************************************
Windows Implementation - inlined static functions
***************************************************************************** */
#elif FIO_OS_WIN
#include <process.h>
#ifndef FIO_THREADS_BYO

// clang-format off
/** Starts a new thread, returns 0 on success and -1 on failure. */
FIO_IFUNC int fio_thread_create(fio_thread_t *t, void *(*fn)(void *), void *arg) { *t = (HANDLE)_beginthreadex(NULL, 0, (unsigned int (*)(void *))(uintptr_t)fn, arg, 0, NULL); return (!!t) - 1; }

FIO_IFUNC int fio_thread_join(fio_thread_t t) {
  int r = 0;
  if (WaitForSingleObject(t, INFINITE) == WAIT_FAILED) {
    errno = GetLastError();
    r = -1;
  } else
    CloseHandle(t);
  return r;
}

/** Detaches the thread, so thread resources are freed automatically. */
FIO_IFUNC int fio_thread_detach(fio_thread_t t) { return CloseHandle(t) - 1; }

/** Ends the current running thread. */
FIO_IFUNC void fio_thread_exit(void) { _endthread(); }

/* Returns non-zero if both threads refer to the same thread. */
FIO_IFUNC int fio_thread_equal(fio_thread_t a, fio_thread_t b) { return GetThreadId(a) == GetThreadId(b); }

/** Returns the current thread. */
FIO_IFUNC fio_thread_t fio_thread_current(void) { return GetCurrentThread(); }

/** Yields thread execution. */
FIO_IFUNC void fio_thread_yield(void) { Sleep(0); }

#endif /* FIO_THREADS_BYO */
#ifndef FIO_THREADS_MUTEX_BYO

SFUNC int fio___thread_mutex_lazy_init(fio_thread_mutex_t *m);

FIO_IFUNC int fio_thread_mutex_init(fio_thread_mutex_t *m) { return ((*m = CreateMutexW(NULL, FALSE, NULL)) != NULL) - 1; }

/** Unlocks a simple Mutex, returning zero on success or -1 on error. */
FIO_IFUNC int fio_thread_mutex_unlock(fio_thread_mutex_t *m) { return ((m && *m) ? ReleaseMutex(*m) : 0) -1; }

/** Destroys the simple Mutex (cleanup). */
FIO_IFUNC void fio_thread_mutex_destroy(fio_thread_mutex_t *m) { CloseHandle(*m); *m = FIO_THREAD_MUTEX_INIT; }

// clang-format on

/** Locks a simple Mutex, returning -1 on error. */
FIO_IFUNC int fio_thread_mutex_lock(fio_thread_mutex_t *m) {
  if (!*m && fio___thread_mutex_lazy_init(m))
    return -1;
  return (WaitForSingleObject((*m), INFINITE) == WAIT_OBJECT_0) - 1;
}

/** Attempts to lock a simple Mutex, returning zero on success. */
FIO_IFUNC int fio_thread_mutex_trylock(fio_thread_mutex_t *m) {
  if (!*m && fio___thread_mutex_lazy_init(m))
    return -1;
  return (WaitForSingleObject((*m), 0) == WAIT_OBJECT_0) - 1;
}
#endif
#endif /* FIO_THREADS_MUTEX_BYO */

/* *****************************************************************************
Module Implementation - possibly externed functions.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE
#if FIO_OS_WIN
#ifndef FIO_THREADS_MUTEX_BYO
/** Initializes a simple Mutex */
SFUNC int fio___thread_mutex_lazy_init(fio_thread_mutex_t *m) {
  static fio_lock_i lock = FIO_LOCK_INIT;
  /* lazy initialization */
  fio_lock(&lock);
  if (!*m) { /* retest, as this may chave changed... */
    *m = CreateMutexW(NULL, FALSE, NULL);
  }
  fio_unlock(&lock);
  return (!!m) - 1;
}
#endif /* FIO_THREADS_MUTEX_BYO */
#endif /* FIO_OS_WIN */
/* *****************************************************************************
Module Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, threads)(void) {
  /*
   * TODO? test module here
   */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MODULE_PTR
#endif /* FIO_THREADS */
#undef FIO_THREADS
