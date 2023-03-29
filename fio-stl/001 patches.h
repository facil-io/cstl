#ifndef H___FIO_OS_PATCHES___H
#define H___FIO_OS_PATCHES___H
/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* ************************************************************************* */

/* *****************************************************************************


Patch for OSX version < 10.12 from https://stackoverflow.com/a/9781275/4025095

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if (defined(__MACH__) && !defined(CLOCK_REALTIME))
#warning fio_time functions defined using gettimeofday patch.
#include <sys/time.h>
#define CLOCK_REALTIME 0
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif
#define clock_gettime fio_clock_gettime
// clock_gettime is not implemented on older versions of OS X (< 10.12).
// If implemented, CLOCK_MONOTONIC will have already been defined.
FIO_IFUNC int fio_clock_gettime(int clk_id, struct timespec *t) {
  struct timeval now;
  int rv = gettimeofday(&now, NULL);
  if (rv)
    return rv;
  t->tv_sec = now.tv_sec;
  t->tv_nsec = now.tv_usec * 1000;
  return 0;
  (void)clk_id;
}

#endif
/* *****************************************************************************




Patches for Windows




***************************************************************************** */
#if FIO_OS_WIN
#if _MSC_VER
#pragma message("Warning: some functionality is enabled by patchwork.")
#else
#warning some functionality is enabled by patchwork.
#endif
#define _CRT_NONSTDC_NO_WARNINGS

#include <fcntl.h>
#include <io.h>
#include <processthreadsapi.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <sysinfoapi.h>
#include <time.h>
#include <winsock2.h> /* struct timeval is here... why? Microsoft. */

/* *****************************************************************************
Windows initialization
***************************************************************************** */

/* Enable console colors */
FIO_CONSTRUCTOR(fio___windows_startup_housekeeping) {
  HANDLE c = GetStdHandle(STD_OUTPUT_HANDLE);
  if (c) {
    DWORD mode = 0;
    if (GetConsoleMode(c, &mode)) {
      mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      SetConsoleMode(c, mode);
    }
  }
  c = GetStdHandle(STD_ERROR_HANDLE);
  if (c) {
    DWORD mode = 0;
    if (GetConsoleMode(c, &mode)) {
      mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      SetConsoleMode(c, mode);
    }
  }
}

/* *****************************************************************************
Inlined patched and MACRO statements
***************************************************************************** */

FIO_IFUNC struct tm *gmtime_r(const time_t *timep, struct tm *result) {
  struct tm *t = gmtime(timep);
  if (t && result)
    *result = *t;
  return result;
}

FIO_IFUNC int strcasecmp(const char *s1, const char *s2) {
  return _stricmp(s1, s2);
}

FIO_IFUNC int write(int fd, const void *b, unsigned int l) {
  return _write(fd, b, l);
}

FIO_IFUNC int read(int const fd, void *const b, unsigned const l) {
  return _read(fd, b, l);
}

/** patch for clock_gettime */
FIO_SFUNC int clock_gettime(const uint32_t clk_type, struct timespec *tv);
/** patch for pread */
FIO_SFUNC ssize_t pread(int fd, void *buf, size_t count, off_t offset);
/** patch for pwrite */
FIO_SFUNC ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
FIO_SFUNC int kill(int pid, int signum);

#ifndef O_APPEND
#define O_APPEND      _O_APPEND
#define O_BINARY      _O_BINARY
#define O_CREAT       _O_CREAT
#define O_CREAT       _O_CREAT
#define O_SHORT_LIVED _O_SHORT_LIVED
#define O_CREAT       _O_CREAT
#define O_TEMPORARY   _O_TEMPORARY
#define O_CREAT       _O_CREAT
#define O_EXCL        _O_EXCL
#define O_NOINHERIT   _O_NOINHERIT
#define O_RANDOM      _O_RANDOM
#define O_RDONLY      _O_RDONLY
#define O_RDWR        _O_RDWR
#define O_SEQUENTIAL  _O_SEQUENTIAL
#define O_TEXT        _O_TEXT
#define O_TRUNC       _O_TRUNC
#define O_WRONLY      _O_WRONLY
#define O_U16TEXT     _O_U16TEXT
#define O_U8TEXT      _O_U8TEXT
#define O_WTEXT       _O_WTEXT
#endif /* O_APPEND */

#ifndef S_IWUSR
#define S_IREAD  _S_IREAD
#define S_IWRITE _S_IWRITE
#define S_IRUSR  _S_IREAD
#define S_IWUSR  _S_IWRITE
#define S_IRWXO  (_S_IREAD | _S_IWRITE)
#define S_IRWXG  (_S_IREAD | _S_IWRITE)
#define S_IRWXU  (_S_IREAD | _S_IWRITE)
#endif /* S_IWUSR */

#ifndef O_TMPFILE
#define O_TMPFILE O_TEMPORARY
#endif

#if defined(CLOCK_REALTIME) && defined(CLOCK_MONOTONIC) &&                     \
    CLOCK_REALTIME == CLOCK_MONOTONIC
#undef CLOCK_MONOTONIC
#undef CLOCK_REALTIME
#endif

#ifndef CLOCK_REALTIME
#ifdef CLOCK_MONOTONIC
#define CLOCK_REALTIME (CLOCK_MONOTONIC + 1)
#else
#define CLOCK_REALTIME 0
#endif
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

#if !defined(fstat)
#define fstat _fstat
#endif /* fstat */
#if !defined(stat)
#define stat _stat
#endif /* stat */
#if !defined(unlink)
#define unlink _unlink
#endif /* unlink */
#ifndef getpid
#define getpid _getpid
#endif /* getpid */
#ifndef pid_t
#define pid_t int
#endif /* pid_t */

#if !FIO_HAVE_UNIX_TOOLS
/* patch clock_gettime */
#define clock_gettime fio_clock_gettime
#define pipe(fds)     _pipe(fds, 65536, _O_BINARY)
#endif

/* *****************************************************************************
Patched function Implementation
***************************************************************************** */

/* based on:
 * https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
 */
/** patch for clock_gettime */
FIO_SFUNC int clock_gettime(const uint32_t clk_type, struct timespec *tv) {
  if (!tv)
    return -1;
  static union {
    uint64_t u;
    LARGE_INTEGER li;
  } freq = {.u = 0};
  static double tick2n = 0;
  union {
    uint64_t u;
    FILETIME ft;
    LARGE_INTEGER li;
  } tu;

  switch (clk_type) {
  case CLOCK_REALTIME:
  realtime_clock:
    GetSystemTimePreciseAsFileTime(&tu.ft);
    tv->tv_sec = tu.u / 10000000;
    tv->tv_nsec = tu.u - (tv->tv_sec * 10000000);
    return 0;

#ifdef CLOCK_PROCESS_CPUTIME_ID
  case CLOCK_PROCESS_CPUTIME_ID:
#endif
#ifdef CLOCK_THREAD_CPUTIME_ID
  case CLOCK_THREAD_CPUTIME_ID:
#endif
  case CLOCK_MONOTONIC:
    if (!QueryPerformanceCounter(&tu.li))
      goto realtime_clock;
    if (!freq.u)
      QueryPerformanceFrequency(&freq.li);
    if (!freq.u) {
      tick2n = 0;
      freq.u = 1;
    } else {
      tick2n = (double)1000000000 / freq.u;
    }
    tv->tv_sec = tu.u / freq.u;
    tv->tv_nsec =
        (uint64_t)(0ULL + ((double)(tu.u - (tv->tv_sec * freq.u)) * tick2n));
    return 0;
  }
  return -1;
}

/** patch for pread */
FIO_SFUNC ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
  /* Credit to Jan Biedermann (GitHub: @janbiedermann) */
  ssize_t bytes_read = 0;
  HANDLE handle = (HANDLE)_get_osfhandle(fd);
  if (handle == INVALID_HANDLE_VALUE)
    goto bad_file;
  OVERLAPPED overlapped = {0};
  if (offset > 0)
    overlapped.Offset = offset;
  if (ReadFile(handle, buf, count, (u_long *)&bytes_read, &overlapped))
    return bytes_read;
  if (GetLastError() == ERROR_HANDLE_EOF)
    return bytes_read;
  errno = EIO;
  return -1;
bad_file:
  errno = EBADF;
  return -1;
}

/** patch for pwrite */
FIO_SFUNC ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
  /* Credit to Jan Biedermann (GitHub: @janbiedermann) */
  ssize_t bytes_written = 0;
  HANDLE handle = (HANDLE)_get_osfhandle(fd);
  if (handle == INVALID_HANDLE_VALUE)
    goto bad_file;
  OVERLAPPED overlapped = {0};
  if (offset > 0)
    overlapped.Offset = offset;
  if (WriteFile(handle, buf, count, (u_long *)&bytes_written, &overlapped))
    return bytes_written;
  errno = EIO;
  return -1;
bad_file:
  errno = EBADF;
  return -1;
}

/** patch for kill */
FIO_SFUNC int kill(int pid, int sig) {
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
/* *****************************************************************************



Patches for POSIX



***************************************************************************** */
#elif FIO_OS_POSIX /* POSIX patches */
#endif

/* *****************************************************************************
Done with Patches
***************************************************************************** */
#endif /* H___FIO_OS_PATCHES___H */
