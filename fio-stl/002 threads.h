/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_THREADS            /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        Simple Portable Threads



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_THREADS) && !defined(H___FIO_THREADS___H)
#define H___FIO_THREADS___H

/* *****************************************************************************
Module Settings

At this point, define any MACROs and customizable settings available to the
developer.
***************************************************************************** */

#if FIO_OS_POSIX || defined(__CYGWIN__) /* POSIX Systems */
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifndef FIO_THREADS_BYO
typedef pthread_t fio_thread_t;
#endif

#ifndef FIO_THREADS_FORK_BYO
typedef pid_t fio_thread_pid_t;
#endif

#ifndef FIO_THREADS_MUTEX_BYO
typedef pthread_mutex_t fio_thread_mutex_t;
/** Used this macro for static initialization. */
#define FIO_THREAD_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#endif

#ifndef FIO_THREADS_COND_BYO
typedef pthread_cond_t fio_thread_cond_t;
#endif

#elif FIO_OS_WIN /* Windows Systems */
#include <synchapi.h>

#ifndef FIO_THREADS_BYO
/* Numeric thread ID. The implementation retains each created thread's real
 * HANDLE until join/detach, so fast exit and TID reuse cannot invalidate joins.
 * Current/equal remain allocation-free and compatible with scalar callers. */
typedef uintptr_t fio_thread_t;
#endif

#ifndef FIO_THREADS_FORK_BYO
typedef DWORD fio_thread_pid_t;
#endif

#ifndef FIO_THREADS_MUTEX_BYO
/* SRWLOCK is non-recursive (unlike CRITICAL_SECTION), zero-initialized, and
 * compatible with CONDITION_VARIABLE via SleepConditionVariableSRW. */
typedef SRWLOCK fio_thread_mutex_t;
/** Used this macro for static initialization. */
#define FIO_THREAD_MUTEX_INIT                                                  \
  { 0 } /* SRWLOCK is zero-initialized */
#endif

#ifndef FIO_THREADS_COND_BYO
typedef CONDITION_VARIABLE fio_thread_cond_t;
#endif

#else /* No Known System */
#if !defined(FIO_THREADS_BYO) || !defined(FIO_THREADS_FORK_BYO) ||             \
    !defined(FIO_THREADS_MUTEX_BYO) || !defined(FIO_THREADS_COND_BYO)
#error facil.io Simple Portable Threads require a POSIX system or Windows
#endif
#endif /* system detection */

/* *****************************************************************************
API for forking processes
***************************************************************************** */

/** POSIX fork; native Windows returns (fio_thread_pid_t)-1 with ENOSYS. */
FIO_IFUNC fio_thread_pid_t fio_thread_fork(void);

/** Should behave the same as the POSIX system call `getpid`. */
FIO_IFUNC fio_thread_pid_t fio_thread_getpid(void);

/** POSIX kill; native Windows supports positive PIDs only (no groups). */
FIO_IFUNC int fio_thread_kill(fio_thread_pid_t pid, int sig);

/** POSIX waitpid. Native Windows supports snapshot-visible children with
 * options 0 or WNOHANG only; no POSIX reaping or durable zombie status. */
FIO_IFUNC int fio_thread_waitpid(fio_thread_pid_t pid,
                                 int *stat_loc,
                                 int options);

/* *****************************************************************************
API for spawning threads
***************************************************************************** */

/** Starts fn(arg); returns 0 or -1 with errno on failure. On success t owns
 * one join/detach obligation. Keep arg alive until the callback stops using it.
 */
FIO_IFUNC int fio_thread_create(fio_thread_t *t,
                                void *(*fn)(void *),
                                void *arg);

/** Waits for a created, joinable thread. Zero means completion; nonzero is an
 * error, not completion. Do not concurrently join/detach the same thread. */
FIO_IFUNC int fio_thread_join(fio_thread_t *t);

/** Detaches the thread, so thread resources are freed automatically. */
FIO_IFUNC int fio_thread_detach(fio_thread_t *t);

/** Ends the current running thread. */
FIO_IFUNC void fio_thread_exit(void);

/* Returns non-zero if both threads refer to the same thread. */
FIO_IFUNC int fio_thread_equal(fio_thread_t *a, fio_thread_t *b);

/** Returns the current thread. */
FIO_IFUNC fio_thread_t fio_thread_current(void);

/**
 * Returns a process-local numeral ID for the current thread.
 *
 * The value is stable for the thread's lifetime and unique among live threads.
 * It may be reused after the thread exits.
 */
FIO_IFUNC uintptr_t fio_thread_nid(void);

/** Yields thread execution. */
FIO_IFUNC void fio_thread_yield(void);

/** Possible thread priority values. */
typedef enum {
  FIO_THREAD_PRIORITY_ERROR = -1,
  FIO_THREAD_PRIORITY_LOWEST = 0,
  FIO_THREAD_PRIORITY_LOW,
  FIO_THREAD_PRIORITY_NORMAL,
  FIO_THREAD_PRIORITY_HIGH,
  FIO_THREAD_PRIORITY_HIGHEST,
} fio_thread_priority_e;

/** Returns a thread's priority level. */
FIO_SFUNC fio_thread_priority_e fio_thread_priority(void);

/** Sets a thread's priority level. */
FIO_SFUNC int fio_thread_priority_set(fio_thread_priority_e);

/* *****************************************************************************
API for mutexes
***************************************************************************** */

/**
 * Initializes a simple Mutex.
 *
 * Or use the static initialization value: FIO_THREAD_MUTEX_INIT
 */
FIO_IFUNC int fio_thread_mutex_init(fio_thread_mutex_t *m);

/** Locks a simple Mutex, returning -1 on error. */
FIO_IFUNC int fio_thread_mutex_lock(fio_thread_mutex_t *m);

/** Attempts to lock a simple Mutex, returning zero on success. */
FIO_IFUNC int fio_thread_mutex_trylock(fio_thread_mutex_t *m);

/** Unlocks a simple Mutex, returning zero on success or -1 on error. */
FIO_IFUNC int fio_thread_mutex_unlock(fio_thread_mutex_t *m);

/** Destroys the simple Mutex (cleanup). */
FIO_IFUNC void fio_thread_mutex_destroy(fio_thread_mutex_t *m);

/* *****************************************************************************
API for conditional variables
***************************************************************************** */

/** Initializes a simple conditional variable. */
FIO_IFUNC int fio_thread_cond_init(fio_thread_cond_t *c);

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC int fio_thread_cond_wait(fio_thread_cond_t *c, fio_thread_mutex_t *m);

/** Waits with m locked. Returns 0 on wake, nonzero on timeout/error. POSIX
 * uses CLOCK_REALTIME and rejects unrepresentable deadlines; Windows clamps
 * finite durations to INFINITE-1 milliseconds rather than wrapping. */
FIO_IFUNC int fio_thread_cond_timedwait(fio_thread_cond_t *c,
                                        fio_thread_mutex_t *m,
                                        size_t milliseconds);

/** Signals a simple conditional variable. */
FIO_IFUNC int fio_thread_cond_signal(fio_thread_cond_t *c);

/** Signal broadcast for a conditional variable. */
FIO_IFUNC int fio_thread_cond_broadcast(fio_thread_cond_t *c);

/** Destroys a simple conditional variable. */
FIO_IFUNC void fio_thread_cond_destroy(fio_thread_cond_t *c);

/* *****************************************************************************


POSIX Implementation - inlined static functions


***************************************************************************** */
#if FIO_OS_POSIX || defined(__CYGWIN__)

#ifndef FIO_THREADS_FORK_BYO
/** Should behave the same as the POSIX system call `getpid`. */
FIO_IFUNC fio_thread_pid_t fio_thread_getpid(void) {
  return (fio_thread_pid_t)getpid();
}
/** Should behave the same as the POSIX system call `fork`. */
FIO_IFUNC fio_thread_pid_t fio_thread_fork(void) {
  return (fio_thread_pid_t)fork();
}

/** Should behave the same as the POSIX system call `kill`. */
FIO_IFUNC int fio_thread_kill(fio_thread_pid_t i, int s) {
  return kill((pid_t)i, s);
}

/** Should behave the same as the POSIX system call `waitpid`. */
FIO_IFUNC int fio_thread_waitpid(fio_thread_pid_t i, int *s, int o) {
  return waitpid((pid_t)i, s, o);
}
#endif /* FIO_THREADS_FORK_BYO */

#ifndef FIO_THREADS_BYO
// clang-format off

/** Starts a new thread, returns 0 on success and -1 on failure. */
FIO_IFUNC int fio_thread_create(fio_thread_t *t, void *(*fn)(void *), void *arg) {
  int r = pthread_create(t, NULL, fn, arg);
  if (r) errno = r;
  return r ? -1 : 0;
}

FIO_IFUNC int fio_thread_join(fio_thread_t *t) { return pthread_join(*t, NULL); }

/** Detaches the thread, so thread resources are freed automatically. */
FIO_IFUNC int fio_thread_detach(fio_thread_t *t) { return pthread_detach(*t); }

/** Ends the current running thread. */
FIO_IFUNC void fio_thread_exit(void) { pthread_exit(NULL); }

/* Returns non-zero if both threads refer to the same thread. */
FIO_IFUNC int fio_thread_equal(fio_thread_t *a, fio_thread_t *b) { return pthread_equal(*a, *b); }

/** Returns the current thread. */
FIO_IFUNC fio_thread_t fio_thread_current(void) { return pthread_self(); }

/** Returns a process-local numeral ID for the current thread. */
FIO_IFUNC uintptr_t fio_thread_nid(void) {
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) ||         \
    defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) ||  \
    defined(__sun) || defined(_AIX)
  return (uintptr_t)pthread_self();
#else
  /* errno is thread-local on POSIX systems and avoids pthread_t assumptions. */
  return (uintptr_t)&errno;
#endif
}

/** Yields thread execution. */
FIO_IFUNC void fio_thread_yield(void) { sched_yield(); }
#endif /* FIO_THREADS_BYO */


#if defined(__APPLE__) &&  __has_include("sys/qos.h") /* MacOS with QoS */
#include "sys/qos.h"

/** Returns a thread's priority level. */
FIO_SFUNC fio_thread_priority_e fio_thread_priority(void) {
  qos_class_t qos;
  int rel;
  if(pthread_get_qos_class_np(pthread_self(), &qos, &rel)) return FIO_THREAD_PRIORITY_ERROR;
  switch(qos) {
  case QOS_CLASS_BACKGROUND:       return FIO_THREAD_PRIORITY_LOWEST;
  case QOS_CLASS_UTILITY:          return FIO_THREAD_PRIORITY_LOW;
  case QOS_CLASS_DEFAULT:          return FIO_THREAD_PRIORITY_NORMAL;
  case QOS_CLASS_UNSPECIFIED:      return FIO_THREAD_PRIORITY_NORMAL;
  case QOS_CLASS_USER_INITIATED:   return FIO_THREAD_PRIORITY_HIGH;
  case QOS_CLASS_USER_INTERACTIVE: return FIO_THREAD_PRIORITY_HIGHEST;
  }
  return FIO_THREAD_PRIORITY_ERROR;
}

/** Sets a thread's priority level. */
FIO_SFUNC int fio_thread_priority_set(fio_thread_priority_e pr) {
  qos_class_t qos = QOS_CLASS_DEFAULT;
  int rel = 0;
  switch(pr) {
  case FIO_THREAD_PRIORITY_LOWEST:  qos = QOS_CLASS_BACKGROUND;       rel = -4; break;
  case FIO_THREAD_PRIORITY_LOW:     qos = QOS_CLASS_UTILITY;          rel = -2; break;
  case FIO_THREAD_PRIORITY_NORMAL:  qos = QOS_CLASS_DEFAULT;          rel = 0; break;
  case FIO_THREAD_PRIORITY_HIGH:    qos = QOS_CLASS_USER_INITIATED;   rel = 2; break;
  case FIO_THREAD_PRIORITY_HIGHEST: qos = QOS_CLASS_USER_INTERACTIVE; rel = 4; break;
  default: errno = EINVAL; return -1;
  }
  int r = pthread_set_qos_class_self_np(qos, rel);
  if (r) errno = r;
  return r ? -1 : 0;
}

#else /* portable POSIX */

/** Returns a thread's priority level. */
FIO_SFUNC fio_thread_priority_e fio_thread_priority(void) {
  int policy;
  struct sched_param schd;
  if(pthread_getschedparam(pthread_self(), &policy, &schd))
    return FIO_THREAD_PRIORITY_ERROR;
  int min = sched_get_priority_min(policy);
  int max = sched_get_priority_max(policy);
  if (min == -1 || max == -1 || max < min)
    return FIO_THREAD_PRIORITY_ERROR;
  if (min == max)
    return FIO_THREAD_PRIORITY_NORMAL;
  size_t steps = (size_t)(max - min) / 5;
  size_t priority_value = (schd.sched_priority - min);
  if(steps) priority_value /= steps; else priority_value = 5;
  switch(priority_value) {
  case 0: return FIO_THREAD_PRIORITY_LOWEST;
  case 1: return FIO_THREAD_PRIORITY_LOW;  
  case 2: return FIO_THREAD_PRIORITY_NORMAL;  
  case 3: return FIO_THREAD_PRIORITY_HIGH;  
  case 4: return FIO_THREAD_PRIORITY_HIGHEST;  
  }
  return FIO_THREAD_PRIORITY_ERROR;
}

/** Sets a thread's priority level. */
FIO_SFUNC int fio_thread_priority_set(fio_thread_priority_e priority_value) {
  if (priority_value < FIO_THREAD_PRIORITY_LOWEST ||
      priority_value > FIO_THREAD_PRIORITY_HIGHEST) {
    errno = EINVAL;
    return -1;
  }
  int policy;
  struct sched_param schd;
  int r = pthread_getschedparam(pthread_self(), &policy, &schd);
  if (r) { errno = r; return -1; }
  int min = sched_get_priority_min(policy);
  int max = sched_get_priority_max(policy);
  if (min == -1 || max == -1 || max < min)
    return -1;
  size_t steps = (size_t)(max - min) / 5;
  switch(priority_value) {
  case FIO_THREAD_PRIORITY_LOWEST:  schd.sched_priority = min + steps; break;
  case FIO_THREAD_PRIORITY_LOW:     schd.sched_priority = min + (steps << 1); break; 
  case FIO_THREAD_PRIORITY_NORMAL:  schd.sched_priority = max - (steps << 1); break; 
  case FIO_THREAD_PRIORITY_HIGH:    schd.sched_priority = max - steps; break; 
  case FIO_THREAD_PRIORITY_HIGHEST: schd.sched_priority = max; break; 
  default: errno = EINVAL; return -1;
  }
  r = pthread_setschedparam(pthread_self(), policy, &schd);
  if (r) errno = r;
  return r ? -1 : 0;
}

#endif /* MacOS vs. portable POSIX */

#ifndef FIO_THREADS_MUTEX_BYO

/** Initializes a simple Mutex. */
FIO_IFUNC int fio_thread_mutex_init(fio_thread_mutex_t *m) { return pthread_mutex_init(m, NULL); }

/** Locks a simple Mutex, returning -1 on error. */
FIO_IFUNC int fio_thread_mutex_lock(fio_thread_mutex_t *m) {
  int r = pthread_mutex_lock(m);
  if (r) errno = r;
  return r ? -1 : 0;
}

/** Attempts to lock a simple Mutex, returning zero on success. */
FIO_IFUNC int fio_thread_mutex_trylock(fio_thread_mutex_t *m) { return pthread_mutex_trylock(m); }

/** Unlocks a simple Mutex, returning zero on success or -1 on error. */
FIO_IFUNC int fio_thread_mutex_unlock(fio_thread_mutex_t *m) {
  int r = pthread_mutex_unlock(m);
  if (r) errno = r;
  return r ? -1 : 0;
}

/** Destroys the simple Mutex (cleanup). */
FIO_IFUNC void fio_thread_mutex_destroy(fio_thread_mutex_t *m) { pthread_mutex_destroy(m); *m = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER; }

#endif /* FIO_THREADS_MUTEX_BYO */
// clang-format on

#ifndef FIO_THREADS_COND_BYO
/** Initializes a simple conditional variable. */
FIO_IFUNC int fio_thread_cond_init(fio_thread_cond_t *c) {
  return pthread_cond_init(c, NULL);
}

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC int fio_thread_cond_wait(fio_thread_cond_t *c,
                                   fio_thread_mutex_t *m) {
  return pthread_cond_wait(c, m);
}

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC int fio_thread_cond_timedwait(fio_thread_cond_t *c,
                                        fio_thread_mutex_t *m,
                                        size_t milliseconds) {
  struct timespec t;
  if (clock_gettime(CLOCK_REALTIME, &t))
    return errno;
  /* Split before adding: neither SIZE_MAX nor a fractional carry may wrap. */
  uintmax_t seconds = (uintmax_t)(milliseconds / 1000);
  long nanoseconds = t.tv_nsec + (long)(milliseconds % 1000) * 1000000L;
  seconds += (nanoseconds >= 1000000000L);
  nanoseconds %= 1000000000L;
  const uintmax_t time_max =
      ((time_t)-1 > (time_t)0)
          ? (uintmax_t)(time_t)-1
          : (((uintmax_t)1 << (sizeof(time_t) * CHAR_BIT - 1)) - 1);
  if (t.tv_sec < 0 || seconds > time_max - (uintmax_t)t.tv_sec)
    return EOVERFLOW;
  t.tv_sec += (time_t)seconds;
  t.tv_nsec = nanoseconds;
  return pthread_cond_timedwait(c, m, &t);
}

/** Signals a simple conditional variable. */
FIO_IFUNC int fio_thread_cond_signal(fio_thread_cond_t *c) {
  return pthread_cond_signal(c);
}

/** Signal broadcast for a conditional variable. */
FIO_IFUNC int fio_thread_cond_broadcast(fio_thread_cond_t *c) {
  return pthread_cond_broadcast(c);
}
/** Destroys a simple conditional variable. */
FIO_IFUNC void fio_thread_cond_destroy(fio_thread_cond_t *c) {
  pthread_cond_destroy(c);
}
#endif /* FIO_THREADS_COND_BYO */

/* *****************************************************************************


Windows Implementation - inlined static functions


***************************************************************************** */
#elif FIO_OS_WIN
#include <process.h>
#include <processthreadsapi.h>
#include <tlhelp32.h>

#ifndef FIO_THREADS_FORK_BYO

FIO_IFUNC fio_thread_pid_t fio_thread_getpid(void) {
  return (fio_thread_pid_t)GetCurrentProcessId();
}

#if defined(__CYGWIN__) || (defined(fork) && defined(WEXITSTATUS))
FIO_IFUNC fio_thread_pid_t fio_thread_fork(void) {
  return (fio_thread_pid_t)fork();
}
FIO_IFUNC int fio_thread_kill(fio_thread_pid_t i, int s) {
  return kill((pid_t)i, s);
}
FIO_IFUNC int fio_thread_waitpid(fio_thread_pid_t i, int *s, int o) {
  return waitpid((pid_t)i, s, o);
}

#else /* defined(fork) && defined(WEXITSTATUS) */

FIO_IFUNC fio_thread_pid_t fio_thread_fork(void) {
  FIO_LOG_FATAL(
      "`fork` unsupported on this platform - cannot spawn worker processes.");
  errno = ENOSYS;
  return (fio_thread_pid_t)-1;
}

FIO_IFUNC int fio_thread_kill(fio_thread_pid_t pid, int sig) {
  /* Credit to Jan Biedermann (GitHub: @janbiedermann) */
  HANDLE handle;
  DWORD status;
  if (sig < 0 || sig >= NSIG) {
    errno = EINVAL;
    return -1;
  }
#ifdef SIGCONT
  if (sig == SIGCONT) {
    errno = ENOSYS;
    return -1;
  }
#endif

  /* POSIX group/broadcast forms must never silently target the caller. */
  if (!pid || pid == (fio_thread_pid_t)-1) {
    errno = ENOSYS;
    return -1;
  }
  handle =
      OpenProcess((sig ? PROCESS_TERMINATE : 0) | PROCESS_QUERY_INFORMATION,
                  FALSE,
                  pid);
  if (!handle)
    goto something_went_wrong;

  switch (sig) {
#ifdef SIGKILL
  case SIGKILL:
#endif
  case SIGTERM:
  case SIGINT: /* terminate */
    if (!TerminateProcess(handle, 1))
      goto something_went_wrong;
    break;
  case 0: /* check status */
    if (!GetExitCodeProcess(handle, &status))
      goto something_went_wrong;
    if (status != STILL_ACTIVE) {
      errno = ESRCH;
      goto cleanup_after_error;
    }
    break;
  default: /* not supported? */ errno = ENOSYS; goto cleanup_after_error;
  }

  if (pid) {
    CloseHandle(handle);
  }
  return 0;

something_went_wrong:

  switch (GetLastError()) {
  case ERROR_INVALID_PARAMETER: errno = ESRCH; break;
  case ERROR_ACCESS_DENIED:
    errno = EPERM;
    if (handle && GetExitCodeProcess(handle, &status) && status != STILL_ACTIVE)
      errno = ESRCH;
    break;
  default: errno = GetLastError();
  }
cleanup_after_error:
  if (handle && pid)
    CloseHandle(handle);
  return -1;
}

#ifndef WNOHANG
#define WNOHANG 1
#endif /* WNOHANG */

#ifndef WUNTRACED
#define WUNTRACED 2
#endif /* WUNTRACED */

#ifndef WCONTINUED
#define WCONTINUED 8
#endif /* WCONTINUED */

#ifndef WNOWAIT
#define WNOWAIT 0x01000000
#endif /* WNOWAIT */

#ifndef WEXITSTATUS
#define WEXITSTATUS(status) (((status)&0xFF00) >> 8)
#endif /* WEXITSTATUS */

#ifndef WIFEXITED
#define WIFEXITED(status) (WTERMSIG(status) == 0)
#endif /* WIFEXITED */

#ifndef WIFSIGNALED
#define WIFSIGNALED(status) (((signed char)(WTERMSIG(status) + 1) >> 1) > 0)
#endif /* WIFSIGNALED */

#ifndef WTERMSIG
#define WTERMSIG(status) ((status)&0x7F)
#endif /* WTERMSIG */

#ifndef WIFSTOPPED
#define WIFSTOPPED(status) (((status)&0xFF) == 0x7F)
#endif /* WIFSTOPPED */

#ifndef WSTOPSIG
#define WSTOPSIG(status) WEXITSTATUS(status)
#endif /* WSTOPSIG */

FIO_IFUNC int fio_thread_waitpid(fio_thread_pid_t pid, int *status, int opt) {
  /* Windows has no POSIX zombie/reap facility. Only children present in the
   * snapshot can be discovered; callers needing durable exit status should
   * retain their CreateProcess handle instead. */
  if (opt & ~WNOHANG) {
    errno = EINVAL;
    return -1;
  }
  if (!pid) {
    errno = ENOSYS;
    return -1;
  }
  if (pid != (fio_thread_pid_t)-1 && pid > INT_MAX) {
    errno = EOVERFLOW; /* public return type is int */
    return -1;
  }
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    errno = EIO;
    return -1;
  }
  PROCESSENTRY32W pe;
  pe.dwSize = sizeof(pe);
  int result = -1, error = ECHILD;
  int live_child = 0;
  if (!Process32FirstW(snapshot, &pe))
    goto cleanup;
  do {
    if (pe.th32ParentProcessID != GetCurrentProcessId() ||
        (pid != (fio_thread_pid_t)-1 && pe.th32ProcessID != pid))
      continue;
    if (pe.th32ProcessID > INT_MAX) {
      error = EOVERFLOW;
      goto cleanup;
    }
    HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                                 FALSE,
                                 pe.th32ProcessID);
    if (!process) {
      if (GetLastError() == ERROR_INVALID_PARAMETER)
        continue; /* exited between snapshot and open */
      error = (GetLastError() == ERROR_ACCESS_DENIED) ? EPERM : EIO;
      goto cleanup;
    }
    DWORD wait = WaitForSingleObject(process, (opt & WNOHANG) ? 0 : INFINITE);
    DWORD exit_code = 0;
    if (wait == WAIT_OBJECT_0) {
      if (!GetExitCodeProcess(process, &exit_code)) {
        error = EIO;
      } else {
        result = (int)pe.th32ProcessID;
        if (status)
          *status = (int)((exit_code & 0xFF) << 8);
      }
      CloseHandle(process);
      goto cleanup;
    }
    CloseHandle(process);
    if (wait != WAIT_TIMEOUT || !(opt & WNOHANG)) {
      error = EIO;
      goto cleanup;
    }
    live_child = 1; /* scan other children before returning a no-change poll */
  } while (Process32NextW(snapshot, &pe));
  if (live_child)
    result = 0;
cleanup:
  CloseHandle(snapshot);
  if (result < 0)
    errno = error;
  return result;
}

#endif /* already patched by some other implementation */
#endif /* FIO_THREADS_FORK_BYO */

#ifndef FIO_THREADS_BYO
typedef struct fio___thread_start_s {
  struct fio___thread_start_s *next;
  HANDLE handle;
  void *(*fn)(void *);
  void *arg;
  DWORD tid;
  volatile LONG refs; /* registry owner + startup worker */
} fio___thread_start_s;
typedef struct {
  SRWLOCK lock;
  fio___thread_start_s *head;
} fio___thread_registry_s;

/* Share joinable ownership across translation units, as with state callbacks.
 */
FIO_WEAK_VAR fio___thread_registry_s fio___thread_registry = {0};

FIO_IFUNC void fio___thread_start_release(fio___thread_start_s *s) {
  if (!InterlockedDecrement(&s->refs))
    HeapFree(GetProcessHeap(), 0, s);
}

/* Match _beginthreadex's ABI, including __stdcall on 32-bit Windows. */
FIO_SFUNC unsigned __stdcall fio___thread_start(void *arg) {
  fio___thread_start_s *s = (fio___thread_start_s *)arg;
  fio___thread_registry_s *registry = &fio___thread_registry;
  /* The creator publishes the handle, TID and output before user code runs. */
  AcquireSRWLockShared(&registry->lock);
  void *(*fn)(void *) = s->fn;
  void *fn_arg = s->arg;
  ReleaseSRWLockShared(&registry->lock);
  fio___thread_start_release(s);
  (void)fn(fn_arg);
  return 0;
}

/** Starts a new thread, returns 0 on success and -1 on failure. */
FIO_IFUNC int fio_thread_create(fio_thread_t *t,
                                void *(*fn)(void *),
                                void *arg) {
  fio___thread_start_s *s = (fio___thread_start_s *)HeapAlloc(GetProcessHeap(),
                                                              HEAP_ZERO_MEMORY,
                                                              sizeof(*s));
  if (!s) {
    errno = ENOMEM;
    return -1;
  }
  s->fn = fn;
  s->arg = arg;
  s->refs = 2;
  unsigned tid = 0;
  fio___thread_registry_s *registry = &fio___thread_registry;
  AcquireSRWLockExclusive(&registry->lock);
  s->handle = (HANDLE)_beginthreadex(NULL, 0, fio___thread_start, s, 0, &tid);
  if (!s->handle) {
    ReleaseSRWLockExclusive(&registry->lock);
    HeapFree(GetProcessHeap(), 0, s);
    return -1;
  }
  s->tid = tid;
  s->next = registry->head;
  registry->head = s;
  *t = (fio_thread_t)tid;
  ReleaseSRWLockExclusive(&registry->lock);
  return 0;
}

FIO_IFUNC int fio_thread_join(fio_thread_t *t) {
  if (*t == fio_thread_current()) {
    errno = EDEADLK;
    return -1;
  }
  fio___thread_registry_s *registry = &fio___thread_registry;
  AcquireSRWLockShared(&registry->lock);
  fio___thread_start_s *s = registry->head;
  while (s && s->tid != (DWORD)*t)
    s = s->next;
  ReleaseSRWLockShared(&registry->lock);
  if (!s) {
    errno = EINVAL;
    return -1;
  }
  /* A caller must not join/detach the same thread concurrently. */
  if (WaitForSingleObject(s->handle, INFINITE) != WAIT_OBJECT_0) {
    errno = EIO;
    return -1; /* retain ownership so the caller can retry */
  }
  return fio_thread_detach(t);
}

/** Releases the joinable handle; the startup worker owns its own reference. */
FIO_IFUNC int fio_thread_detach(fio_thread_t *t) {
  fio___thread_registry_s *registry = &fio___thread_registry;
  AcquireSRWLockExclusive(&registry->lock);
  fio___thread_start_s **pos = &registry->head;
  while (*pos && (*pos)->tid != (DWORD)*t)
    pos = &(*pos)->next;
  fio___thread_start_s *s = *pos;
  if (!s) {
    ReleaseSRWLockExclusive(&registry->lock);
    errno = EINVAL;
    return -1;
  }
  if (!CloseHandle(s->handle)) {
    ReleaseSRWLockExclusive(&registry->lock);
    errno = EIO;
    return -1;
  }
  *pos = s->next;
  ReleaseSRWLockExclusive(&registry->lock);
  *t = 0;
  fio___thread_start_release(s);
  return 0;
}

// clang-format off
/** Ends the current running thread. */
FIO_IFUNC void fio_thread_exit(void) { _endthreadex(0); }

/* Returns non-zero if both threads refer to the same thread.
 * Both values are TIDs — direct integer compare is correct and sufficient. */
FIO_IFUNC int fio_thread_equal(fio_thread_t *a, fio_thread_t *b) { return *a == *b; }

/** Returns the calling thread's numeric Thread ID (TID, not a HANDLE).
 *
 * No kernel object created, no CloseHandle required, no leak. */
FIO_IFUNC fio_thread_t fio_thread_current(void) {
  return (fio_thread_t)GetCurrentThreadId();
}

/** Returns a process-local numeral ID for the current thread. */
FIO_IFUNC uintptr_t fio_thread_nid(void) {
  return (uintptr_t)GetCurrentThreadId();
}

/** Yields thread execution. */
FIO_IFUNC void fio_thread_yield(void) { SleepEx(0, FALSE); }

#endif /* FIO_THREADS_BYO */

/** Returns a thread's priority level. */
FIO_SFUNC fio_thread_priority_e fio_thread_priority(void) {
  switch(GetThreadPriority(GetCurrentThread())) {
  case THREAD_PRIORITY_LOWEST:       return FIO_THREAD_PRIORITY_LOWEST;
  case THREAD_PRIORITY_BELOW_NORMAL: return FIO_THREAD_PRIORITY_LOW;
  case THREAD_PRIORITY_NORMAL:       return FIO_THREAD_PRIORITY_NORMAL;
  case THREAD_PRIORITY_ABOVE_NORMAL: return FIO_THREAD_PRIORITY_HIGH;
  case THREAD_PRIORITY_HIGHEST:      return FIO_THREAD_PRIORITY_HIGHEST;
  default:                           return FIO_THREAD_PRIORITY_ERROR;
  }
}

/** Sets a thread's priority level. */
FIO_SFUNC int fio_thread_priority_set(fio_thread_priority_e pr) {
  int    priority;
  switch(pr){
  default: errno = EINVAL; return -1;
  case FIO_THREAD_PRIORITY_LOWEST:  priority = THREAD_PRIORITY_LOWEST; break;
  case FIO_THREAD_PRIORITY_LOW:     priority = THREAD_PRIORITY_BELOW_NORMAL; break;
  case FIO_THREAD_PRIORITY_NORMAL:  priority = THREAD_PRIORITY_NORMAL; break;
  case FIO_THREAD_PRIORITY_HIGH:    priority = THREAD_PRIORITY_ABOVE_NORMAL; break;
  case FIO_THREAD_PRIORITY_HIGHEST: priority = THREAD_PRIORITY_HIGHEST; break;
  }
  return 0 - !SetThreadPriority(GetCurrentThread(), priority);
}

#ifndef FIO_THREADS_MUTEX_BYO

// clang-format off
/** Initializes a simple Mutex. SRWLOCK is zero-initialized; this is a no-op. */
FIO_IFUNC int fio_thread_mutex_init(fio_thread_mutex_t *m) { InitializeSRWLock(m); return 0; }

/** Destroys the simple Mutex (cleanup). SRWLOCK has no destroy function. */
FIO_IFUNC void fio_thread_mutex_destroy(fio_thread_mutex_t *m) { FIO_MEMSET(m, 0, sizeof(*m)); }
// clang-format on

/** Unlocks a simple Mutex, returning zero on success or -1 on error. */
FIO_IFUNC int fio_thread_mutex_unlock(fio_thread_mutex_t *m) {
  if (!m)
    return -1;
  ReleaseSRWLockExclusive(m);
  return 0;
}

/** Locks a simple Mutex, returning -1 on error. */
FIO_IFUNC int fio_thread_mutex_lock(fio_thread_mutex_t *m) {
  if (!m)
    return -1;
  AcquireSRWLockExclusive(m);
  return 0;
}

/** Attempts to lock a simple Mutex, returning zero on success.
 *
 * NOTE: SRWLOCK is non-recursive — TryAcquireSRWLockExclusive returns FALSE
 * when the lock is already held by the calling thread, matching POSIX behavior
 * for non-recursive mutexes. */
FIO_IFUNC int fio_thread_mutex_trylock(fio_thread_mutex_t *m) {
  if (!m)
    return -1;
  return TryAcquireSRWLockExclusive(m) ? 0 : -1;
}
#endif /* FIO_THREADS_MUTEX_BYO */

#ifndef FIO_THREADS_COND_BYO
/** Initializes a simple conditional variable. */
FIO_IFUNC int fio_thread_cond_init(fio_thread_cond_t *c) {
  InitializeConditionVariable(c);
  return 0;
}

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC int fio_thread_cond_wait(fio_thread_cond_t *c,
                                   fio_thread_mutex_t *m) {
  return 0 - !SleepConditionVariableSRW(c, m, INFINITE, 0);
}

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC int fio_thread_cond_timedwait(fio_thread_cond_t *c,
                                        fio_thread_mutex_t *m,
                                        size_t milliseconds) {
  /* INFINITE is a sentinel, not a finite duration. Saturate without wrapping.
   */
  DWORD timeout =
      milliseconds >= (size_t)INFINITE ? INFINITE - 1 : (DWORD)milliseconds;
  return 0 - !SleepConditionVariableSRW(c, m, timeout, 0);
}

/** Signals a simple conditional variable. */
FIO_IFUNC int fio_thread_cond_signal(fio_thread_cond_t *c) {
  WakeConditionVariable(c);
  return 0;
}

FIO_IFUNC int fio_thread_cond_broadcast(fio_thread_cond_t *c) {
  WakeAllConditionVariable(c);
  return 0;
}

/** Destroys a simple conditional variable. */
FIO_IFUNC void fio_thread_cond_destroy(fio_thread_cond_t *c) { (void)c; }
#endif /* FIO_THREADS_COND_BYO */

#endif /* FIO_OS_WIN */

/* *****************************************************************************


Multi-Threaded `memcpy` (naive and slow)


***************************************************************************** */

#ifndef FIO_MEMCPY_THREADS
#define FIO_MEMCPY_THREADS 8
#endif
#ifndef FIO_MEMCPY_THREADS___MINCPY
#define FIO_MEMCPY_THREADS___MINCPY (1ULL << 23)
#endif
typedef struct {
  char *restrict dest;
  const char *restrict src;
  size_t bytes;
} fio___thread_memcpy_s;

FIO_SFUNC void *fio___thread_memcpy_task(void *v_) {
  fio___thread_memcpy_s *v = (fio___thread_memcpy_s *)v_;
  FIO_MEMCPY(v->dest, (const void *)(v->src), v->bytes);
  return NULL;
}

/** Multi-threaded memcpy using up to FIO_MEMCPY_THREADS threads */
FIO_SFUNC size_t fio_thread_memcpy(void *restrict dest,
                                   const void *restrict src,
                                   size_t bytes) {
  size_t i = 0, r;
  char *restrict d = (char *restrict)dest;
  const char *restrict s = (const char *restrict)src;
  fio_thread_t threads[FIO_MEMCPY_THREADS - 1];
  fio___thread_memcpy_s info[FIO_MEMCPY_THREADS - 1];
  size_t bytes_per_thread = bytes / FIO_MEMCPY_THREADS;
  if (bytes < FIO_MEMCPY_THREADS___MINCPY)
    goto finished_creating_thread;

  for (; i < (FIO_MEMCPY_THREADS - 1); ++i) {
    info[i] = (fio___thread_memcpy_s){d, s, bytes_per_thread};
    if (fio_thread_create(threads + i, fio___thread_memcpy_task, info + i))
      goto finished_creating_thread;
    d += bytes_per_thread;
    s += bytes_per_thread;
    bytes -= bytes_per_thread;
  }
finished_creating_thread:
  r = i + 1;
  FIO_MEMCPY(d, (const void *)s, bytes); /* memcpy reminder */
  while (i) {
    --i;
    /* There is no safe return while a worker may still own info/buffers. */
    FIO_ASSERT(!fio_thread_join(threads + i),
               "fio_thread_memcpy worker join failed");
  }
  return r;
}

/* *****************************************************************************
Module Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_THREADS */
#undef FIO_THREADS
