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

#if FIO_OS_POSIX
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
typedef pid_t fio_thread_pid_t;
typedef pthread_t fio_thread_t;
typedef pthread_mutex_t fio_thread_mutex_t;
typedef pthread_cond_t fio_thread_cond_t;
/** Used this macro for static initialization. */
#define FIO_THREAD_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER

#elif FIO_OS_WIN
#include <synchapi.h>
typedef DWORD fio_thread_pid_t;
typedef HANDLE fio_thread_t;
typedef CRITICAL_SECTION fio_thread_mutex_t;
typedef CONDITION_VARIABLE fio_thread_cond_t;
/** Used this macro for static initialization. */
#define FIO_THREAD_MUTEX_INIT ((fio_thread_mutex_t){0})
#else
#error facil.io Simple Portable Threads require a POSIX system or Windows
#endif

#ifdef FIO_THREADS_BYO
#define FIO_IFUNC_T
#else
#define FIO_IFUNC_T FIO_IFUNC
#endif

#ifdef FIO_THREADS_FORK_BYO
#define FIO_IFUNC_F
#else
#define FIO_IFUNC_F FIO_IFUNC
#endif

#ifdef FIO_THREADS_MUTEX_BYO
#define FIO_IFUNC_M
#else
#define FIO_IFUNC_M FIO_IFUNC
#endif

#ifdef FIO_THREADS_COND_BYO
#define FIO_IFUNC_C
#else
#define FIO_IFUNC_C FIO_IFUNC
#endif

/* *****************************************************************************
API for forking processes
***************************************************************************** */

/** Should behave the same as the POSIX system call `fork`. */
FIO_IFUNC_F fio_thread_pid_t fio_thread_fork(void);

/** Should behave the same as the POSIX system call `getpid`. */
FIO_IFUNC_F fio_thread_pid_t fio_thread_getpid(void);

/** Should behave the same as the POSIX system call `kill`. */
FIO_IFUNC_F int fio_thread_kill(fio_thread_pid_t pid, int sig);

/** Should behave the same as the POSIX system call `waitpid`. */
FIO_IFUNC_F int fio_thread_waitpid(fio_thread_pid_t pid,
                                   int *stat_loc,
                                   int options);

/* *****************************************************************************
API for spawning threads
***************************************************************************** */

/** Starts a new thread, returns 0 on success and -1 on failure. */
FIO_IFUNC_T int fio_thread_create(fio_thread_t *t,
                                  void *(*fn)(void *),
                                  void *arg);

/** Waits for the thread to finish. */
FIO_IFUNC_T int fio_thread_join(fio_thread_t *t);

/** Detaches the thread, so thread resources are freed automatically. */
FIO_IFUNC_T int fio_thread_detach(fio_thread_t *t);

/** Ends the current running thread. */
FIO_IFUNC_T void fio_thread_exit(void);

/* Returns non-zero if both threads refer to the same thread. */
FIO_IFUNC_T int fio_thread_equal(fio_thread_t *a, fio_thread_t *b);

/** Returns the current thread. */
FIO_IFUNC_T fio_thread_t fio_thread_current(void);

/** Yields thread execution. */
FIO_IFUNC_T void fio_thread_yield(void);

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
API for conditional variables
***************************************************************************** */

/** Initializes a simple conditional variable. */
FIO_IFUNC_C int fio_thread_cond_init(fio_thread_cond_t *c);

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC_C int fio_thread_cond_wait(fio_thread_cond_t *c,
                                     fio_thread_mutex_t *m);

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC_C int fio_thread_cond_timedwait(fio_thread_cond_t *c,
                                          fio_thread_mutex_t *m,
                                          size_t milliseconds);

/** Signals a simple conditional variable. */
FIO_IFUNC_C int fio_thread_cond_signal(fio_thread_cond_t *c);

/** Destroys a simple conditional variable. */
FIO_IFUNC_C void fio_thread_cond_destroy(fio_thread_cond_t *c);

/* *****************************************************************************


POSIX Implementation - inlined static functions


***************************************************************************** */
#if FIO_OS_POSIX

#ifndef FIO_THREADS_FORK_BYO
/** Should behave the same as the POSIX system call `getpid`. */
FIO_IFUNC_F fio_thread_pid_t fio_thread_getpid(void) {
  return (fio_thread_pid_t)getpid();
}
/** Should behave the same as the POSIX system call `fork`. */
FIO_IFUNC_F fio_thread_pid_t fio_thread_fork(void) {
  return (fio_thread_pid_t)fork();
}

/** Should behave the same as the POSIX system call `kill`. */
FIO_IFUNC_F int fio_thread_kill(fio_thread_pid_t i, int s) {
  return kill((pid_t)i, s);
}

/** Should behave the same as the POSIX system call `waitpid`. */
FIO_IFUNC_F int fio_thread_waitpid(fio_thread_pid_t i, int *s, int o) {
  return waitpid((pid_t)i, s, o);
}
#endif

#ifndef FIO_THREADS_BYO
// clang-format off

/** Starts a new thread, returns 0 on success and -1 on failure. */
FIO_IFUNC int fio_thread_create(fio_thread_t *t, void *(*fn)(void *), void *arg) { return pthread_create(t, NULL, fn, arg); }

FIO_IFUNC int fio_thread_join(fio_thread_t *t) { return pthread_join(*t, NULL); }

/** Detaches the thread, so thread resources are freed automatically. */
FIO_IFUNC int fio_thread_detach(fio_thread_t *t) { return pthread_detach(*t); }

/** Ends the current running thread. */
FIO_IFUNC void fio_thread_exit(void) { pthread_exit(NULL); }

/* Returns non-zero if both threads refer to the same thread. */
FIO_IFUNC int fio_thread_equal(fio_thread_t *a, fio_thread_t *b) { return pthread_equal(*a, *b); }

/** Returns the current thread. */
FIO_IFUNC fio_thread_t fio_thread_current(void) { return pthread_self(); }

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
  // pthread_get_qos_class_np(pthread_t  _Nonnull __pthread, qos_class_t * _Nullable __qos_class, int * _Nullable __relative_priority)
  qos_class_t qos = QOS_CLASS_DEFAULT;
  int rel = 0;
  switch(pr) {
  case FIO_THREAD_PRIORITY_LOWEST:  qos = QOS_CLASS_BACKGROUND;       rel = -4; break;
  case FIO_THREAD_PRIORITY_LOW:     qos = QOS_CLASS_UTILITY;          rel = -2; break;
  case FIO_THREAD_PRIORITY_NORMAL:  qos = QOS_CLASS_DEFAULT;          rel = 0; break;
  case FIO_THREAD_PRIORITY_HIGH:    qos = QOS_CLASS_USER_INITIATED;   rel = 2; break;
  case FIO_THREAD_PRIORITY_HIGHEST: qos = QOS_CLASS_USER_INTERACTIVE; rel = 4; break;
  case FIO_THREAD_PRIORITY_ERROR:   qos = QOS_CLASS_DEFAULT;          rel = 0; break;
  }
  return pthread_set_qos_class_self_np(qos, rel);
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
  size_t steps = (size_t)(max - min) / 5;
  size_t priority_value = (schd.sched_priority - min);
  if(steps) priority_value /= steps;
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
  int policy;
  struct sched_param schd;
  if(pthread_getschedparam(pthread_self(), &policy, &schd))
    return -1;
  int min = sched_get_priority_min(policy);
  int max = sched_get_priority_max(policy);
  size_t steps = (size_t)(max - min) / 5;
  switch(priority_value) {
  case FIO_THREAD_PRIORITY_LOWEST:  schd.sched_priority = min + steps; break;
  case FIO_THREAD_PRIORITY_LOW:     schd.sched_priority = min + (steps << 1); break; 
  case FIO_THREAD_PRIORITY_NORMAL:  schd.sched_priority = max - (steps << 1); break; 
  case FIO_THREAD_PRIORITY_HIGH:    schd.sched_priority = max - steps; break; 
  case FIO_THREAD_PRIORITY_HIGHEST: schd.sched_priority = max; break; 
  case FIO_THREAD_PRIORITY_ERROR: return -1;
  }
  return pthread_setschedparam(pthread_self(), policy, &schd);
}

#endif /* MacOS vs. portable POSIX */

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

#ifndef FIO_THREADS_COND_BYO
/** Initializes a simple conditional variable. */
FIO_IFUNC_C int fio_thread_cond_init(fio_thread_cond_t *c) {
  return pthread_cond_init(c, NULL);
}

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC_C int fio_thread_cond_wait(fio_thread_cond_t *c,
                                     fio_thread_mutex_t *m) {
  return pthread_cond_wait(c, m);
}

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC_C int fio_thread_cond_timedwait(fio_thread_cond_t *c,
                                          fio_thread_mutex_t *m,
                                          size_t milliseconds) {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  milliseconds += t.tv_nsec / 1000000;
  t.tv_sec += (long)(milliseconds / 1000);
  t.tv_nsec = (long)((milliseconds % 1000) * 1000000);
  return pthread_cond_timedwait(c, m, &t);
}

/** Signals a simple conditional variable. */
FIO_IFUNC_C int fio_thread_cond_signal(fio_thread_cond_t *c) {
  return pthread_cond_signal(c);
}

/** Destroys a simple conditional variable. */
FIO_IFUNC_C void fio_thread_cond_destroy(fio_thread_cond_t *c) {
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

FIO_IFUNC_F fio_thread_pid_t fio_thread_getpid(void) {
  return (fio_thread_pid_t)GetCurrentProcessId();
}

#if defined(fork) && defined(WEXITSTATUS) /* unix features pre-patched */
FIO_IFUNC_F fio_thread_pid_t fio_thread_fork(void) {
  return (fio_thread_pid_t)fork();
}
FIO_IFUNC_F int fio_thread_kill(fio_thread_pid_t i, int s) {
  return kill((pid_t)i, s);
}
FIO_IFUNC_F int fio_thread_waitpid(fio_thread_pid_t i, int *s, int o) {
  return waitpid((pid_t)i, s, o);
}

#else

FIO_IFUNC_F fio_thread_pid_t fio_thread_fork(void) {
  FIO_LOG_ERROR("`fork` not implemented, cannot spawn child processes.");
  return (fio_thread_pid_t)-1;
}

FIO_IFUNC_F int fio_thread_kill(fio_thread_pid_t pid, int sig) {
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

  if (pid == -1)
    pid = 0;

  if (!pid)
    handle = GetCurrentProcess();
  else
    handle =
        OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, pid);
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
#define WIFSIGNALED(status) (((signed char)(__WTERMSIG(status) + 1) >> 1) > 0)
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

static int fio___thread_waitpid_anychild(PROCESSENTRY32W *pe, DWORD pid) {
  return pe->th32ParentProcessID == GetCurrentProcessId();
}

static int fio___thread_waitpid_pid(PROCESSENTRY32W *pe, DWORD pid) {
  return pe->th32ProcessID == pid;
}

FIO_IFUNC_F int fio_thread_waitpid(fio_thread_pid_t pid, int *status, int opt) {
  /* adopted from:
   * https://github.com/win32ports/sys_wait_h/blob/master/sys/wait.h Copyright
   * Copyright (c) 2019 win32ports, MIT license
   */
  int saved_status = 0;
  HANDLE hProcess = INVALID_HANDLE_VALUE, hSnapshot = INVALID_HANDLE_VALUE;
  int (*are_these_the_druides_were_looking_for)(PROCESSENTRY32W *, DWORD);
  PROCESSENTRY32W pe;
  DWORD wait_status = 0, exit_code = 0;
  int nohang = WNOHANG == (WNOHANG & opt);
  opt &= ~(WUNTRACED | WNOWAIT | WCONTINUED | WNOHANG);
  if (opt) {
    errno = -EINVAL;
    return -1;
  }

  if (pid > 0 || pid == -1) {
    FIO_LOG_ERROR(
        "fio_thread_waitpid not implemented for pid < -1 || pid ==0.");
    return -1;
  }

  are_these_the_druides_were_looking_for = fio___thread_waitpid_pid;
  if (pid == -1) /* wait for any child */
    are_these_the_druides_were_looking_for = fio___thread_waitpid_anychild;

  hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (INVALID_HANDLE_VALUE == hSnapshot) {
    errno = ECHILD;
    return -1;
  }

  pe.dwSize = sizeof(pe);
  if (!Process32FirstW(hSnapshot, &pe)) {
    CloseHandle(hSnapshot);
    errno = ECHILD;
    return -1;
  }
  do {
    if (are_these_the_druides_were_looking_for(&pe, pid)) {
      hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                             0,
                             pe.th32ProcessID);
      if (INVALID_HANDLE_VALUE == hProcess) {
        CloseHandle(hSnapshot);
        errno = ECHILD;
        return -1;
      }
      break;
    }
  } while (Process32NextW(hSnapshot, &pe));
  if (INVALID_HANDLE_VALUE == hProcess) {
    CloseHandle(hSnapshot);
    errno = ECHILD;
    return -1;
  }

  wait_status = WaitForSingleObject(hProcess, nohang ? 0 : INFINITE);

  if (WAIT_OBJECT_0 == wait_status) {
    if (GetExitCodeProcess(hProcess, &exit_code))
      saved_status |= (exit_code & 0xFF) << 8;
  } else if (WAIT_TIMEOUT == wait_status && nohang) {
    return 0;
  } else {
    CloseHandle(hProcess);
    CloseHandle(hSnapshot);
    errno = ECHILD;
    return -1;
  }

  CloseHandle(hProcess);
  CloseHandle(hSnapshot);

  if (status)
    *status = saved_status;

  return pe.th32ParentProcessID;
}

#endif /* already patched by some other implementation */
#endif /* FIO_THREADS_FORK_BYO */

#ifndef FIO_THREADS_BYO
/** Starts a new thread, returns 0 on success and -1 on failure. */
FIO_IFUNC int fio_thread_create(fio_thread_t *t,
                                void *(*fn)(void *),
                                void *arg) {
  *t = (HANDLE)_beginthreadex(NULL,
                              0,
                              (_beginthreadex_proc_type)(uintptr_t)fn,
                              arg,
                              0,
                              NULL);
  return (!!t) - 1;
}

FIO_IFUNC int fio_thread_join(fio_thread_t *t) {
  int r = 0;
  if (WaitForSingleObject(*t, INFINITE) == WAIT_FAILED) {
    errno = GetLastError();
    r = -1;
  } else
    CloseHandle(*t);
  return r;
}

// clang-format off
/** Detaches the thread, so thread resources are freed automatically. */
FIO_IFUNC int fio_thread_detach(fio_thread_t *t) { return CloseHandle(*t) - 1; }

/** Ends the current running thread. */
FIO_IFUNC void fio_thread_exit(void) { _endthread(); }

/* Returns non-zero if both threads refer to the same thread. */
FIO_IFUNC int fio_thread_equal(fio_thread_t *a, fio_thread_t *b) { return GetThreadId(*a) == GetThreadId(*b); }

/** Returns the current thread. */
FIO_IFUNC fio_thread_t fio_thread_current(void) { return GetCurrentThread(); }

/** Yields thread execution. */
FIO_IFUNC void fio_thread_yield(void) { Sleep(0); }

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
  case FIO_THREAD_PRIORITY_ERROR: return -1;
  case FIO_THREAD_PRIORITY_LOWEST:  priority = THREAD_PRIORITY_LOWEST; break;
  case FIO_THREAD_PRIORITY_LOW:     priority = THREAD_PRIORITY_BELOW_NORMAL; break;
  case FIO_THREAD_PRIORITY_NORMAL:  priority = THREAD_PRIORITY_NORMAL; break;
  case FIO_THREAD_PRIORITY_HIGH:    priority = THREAD_PRIORITY_ABOVE_NORMAL; break;
  case FIO_THREAD_PRIORITY_HIGHEST: priority = THREAD_PRIORITY_HIGHEST; break;
  }
  return 0 - !SetThreadPriority(GetCurrentThread(), priority);
}

#ifndef FIO_THREADS_MUTEX_BYO

SFUNC int fio___thread_mutex_lazy_init(fio_thread_mutex_t *m);

FIO_IFUNC int fio_thread_mutex_init(fio_thread_mutex_t *m) { InitializeCriticalSection(m); return 0; }

/** Destroys the simple Mutex (cleanup). */
FIO_IFUNC void fio_thread_mutex_destroy(fio_thread_mutex_t *m) { DeleteCriticalSection(m); memset(m,0,sizeof(*m)); }
// clang-format on
/** Unlocks a simple Mutex, returning zero on success or -1 on error. */
FIO_IFUNC int fio_thread_mutex_unlock(fio_thread_mutex_t *m) {
  if (!m)
    return -1;
  LeaveCriticalSection(m);
  return 0;
}

/** Locks a simple Mutex, returning -1 on error. */
FIO_IFUNC int fio_thread_mutex_lock(fio_thread_mutex_t *m) {
  const fio_thread_mutex_t zero = {0};
  if (!FIO_MEMCMP(m, &zero, sizeof(zero)) && fio___thread_mutex_lazy_init(m))
    return -1;
  EnterCriticalSection(m);
  return 0;
}

/** Attempts to lock a simple Mutex, returning zero on success. */
FIO_IFUNC int fio_thread_mutex_trylock(fio_thread_mutex_t *m) {
  const fio_thread_mutex_t zero = {0};
  if (!FIO_MEMCMP(m, &zero, sizeof(zero)) && fio___thread_mutex_lazy_init(m))
    return -1;
  return TryEnterCriticalSection(m) - 1;
}
#endif /* FIO_THREADS_MUTEX_BYO */

#ifndef FIO_THREADS_COND_BYO
/** Initializes a simple conditional variable. */
FIO_IFUNC_C int fio_thread_cond_init(fio_thread_cond_t *c) {
  InitializeConditionVariable(c);
  return 0;
}

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC_C int fio_thread_cond_wait(fio_thread_cond_t *c,
                                     fio_thread_mutex_t *m) {
  return 0 - !SleepConditionVariableCS(c, m, INFINITE);
}

/** Waits on a conditional variable (MUST be previously locked). */
FIO_IFUNC_C int fio_thread_cond_timedwait(fio_thread_cond_t *c,
                                          fio_thread_mutex_t *m,
                                          size_t milliseconds) {
  return 0 - !SleepConditionVariableCS(c, m, milliseconds);
}

/** Signals a simple conditional variable. */
FIO_IFUNC_C int fio_thread_cond_signal(fio_thread_cond_t *c) {
  WakeConditionVariable(c);
  return 0;
}

/** Destroys a simple conditional variable. */
FIO_IFUNC_C void fio_thread_cond_destroy(fio_thread_cond_t *c) { (void)(c); }
#endif /* FIO_THREADS_COND_BYO */

#endif /* FIO_OS_WIN */

/* *****************************************************************************


Multi-Threaded `memcpy` (naive and slow)


***************************************************************************** */

#ifndef FIO_MEMCPY_THREADS
#define FIO_MEMCPY_THREADS 8
#endif
#undef FIO_MEMCPY_THREADS___MINCPY
#define FIO_MEMCPY_THREADS___MINCPY (1ULL << 23)
typedef struct {
  const char *restrict dest;
  void *restrict src;
  size_t bytes;
} fio___thread_memcpy_s;

FIO_SFUNC void *fio___thread_memcpy_task(void *v_) {
  fio___thread_memcpy_s *v = (fio___thread_memcpy_s *)v_;
  FIO_MEMCPY((void *)(v->dest), (void *)(v->src), v->bytes);
  return NULL;
}

/** Multi-threaded memcpy using up to FIO_MEMCPY_THREADS threads */
FIO_SFUNC size_t fio_thread_memcpy(const void *restrict dest,
                                   void *restrict src,
                                   size_t bytes) {
  size_t i = 0, r;
  const char *restrict d = (const char *restrict)dest;
  char *restrict s = (char *restrict)src;
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
  FIO_MEMCPY((void *)d, (void *)s, bytes); /* memcpy reminder */
  while (i) {
    --i;
    fio_thread_join(threads + i);
  }
  return r;
}

/* *****************************************************************************
Module Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)
#if FIO_OS_WIN
#ifndef FIO_THREADS_MUTEX_BYO
/** Initializes a simple Mutex */
SFUNC int fio___thread_mutex_lazy_init(fio_thread_mutex_t *m) {
  int r = 0;
  static fio_lock_i lock = FIO_LOCK_INIT;
  /* lazy initialization */
  fio_thread_mutex_t zero = {0};
  fio_lock(&lock);
  if (!FIO_MEMCMP(m,
                  &zero,
                  sizeof(zero))) { /* retest, as this may have changed... */
    r = fio_thread_mutex_init(m);
  }
  fio_unlock(&lock);
  return r;
}
#endif /* FIO_THREADS_MUTEX_BYO */
#endif /* FIO_OS_WIN */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_THREADS */
#undef FIO_THREADS
