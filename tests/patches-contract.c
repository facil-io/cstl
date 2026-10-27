/* Deterministic Win32 API contract model; not a Windows runtime emulator.
 * Compile the actual patch header against documented success/failure results.
 */
#if defined(_WIN32)
#include <stdio.h>
int main(void) {
  puts("Win32 contract model: use patches-windows for native coverage");
  return 0;
}
#else
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define FIO_INCLUDE_FILE                      "unused"
#define FIO_OS_WIN                            1
#define FIO_OS_POSIX                          0
#define FIO_IFUNC                             static inline
#define FIO_SFUNC                             static inline
#define FIO_CONSTRUCTOR(name)                 static inline void name(void)
#define FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX 1
#define FIO_STATIC_ALLOC_DEF(name, type, size, slots)                          \
  static type *name(int ignored) {                                             \
    static type buf[size];                                                     \
    (void)ignored;                                                             \
    memset(buf, '!', sizeof(buf));                                             \
    return buf;                                                                \
  }
/* A Linux host may supply O_TMPFILE; model a Windows SDK without it. */
#ifdef O_TMPFILE
#undef O_TMPFILE
#endif
#define clock_gettime                      model_clock_gettime
#define pwrite                             model_pwrite
#define strcasecmp                         model_strcasecmp
#define u_long                             uint32_t

typedef uint32_t DWORD;
typedef intptr_t HANDLE;
typedef struct {
  int64_t QuadPart;
} LARGE_INTEGER;
typedef struct {
  uint32_t low, high;
} FILETIME;
struct _stati64 {
  int64_t st_size;
};
struct _stat {
  int32_t st_size;
};
typedef struct {
  uintptr_t Internal, InternalHigh;
  DWORD Offset, OffsetHigh;
  HANDLE hEvent;
} OVERLAPPED;
#define INVALID_HANDLE_VALUE               ((HANDLE)-1)
#define STD_OUTPUT_HANDLE                  1
#define STD_ERROR_HANDLE                   2
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 4
#define _O_BINARY                          0
#define ERROR_SUCCESS                      0
#define ERROR_ENVVAR_NOT_FOUND             203
#define ERROR_ACCESS_DENIED                5
#define ERROR_IO_PENDING                   997
#define ERROR_INVALID_HANDLE               6
#define ERROR_DISK_FULL                    112
#define ERROR_NOT_ENOUGH_MEMORY            8
#define FILE_TYPE_DISK                     1
#define FILE_TYPE_PIPE                     3
#define FILE_CURRENT                       1
#define FILE_BEGIN                         0
#define TRUE                               1
#define FALSE                              0

static DWORD last_error;
static DWORD GetLastError(void) { return last_error; }
static void SetLastError(DWORD err) { last_error = err; }
static HANDLE GetStdHandle(DWORD ignored) {
  (void)ignored;
  return 0;
}
static int GetConsoleMode(HANDLE h, DWORD *mode) {
  (void)h;
  (void)mode;
  return 0;
}
static int SetConsoleMode(HANDLE h, DWORD mode) {
  (void)h;
  (void)mode;
  return 1;
}
static int _set_fmode(int mode) { return mode; }
static int _stricmp(const char *a, const char *b) { return strcmp(a, b); }
static int _write(int fd, const void *buf, unsigned len) {
  (void)fd;
  (void)buf;
  return (int)len;
}
static int _read(int fd, void *buf, unsigned len) {
  (void)fd;
  (void)buf;
  (void)len;
  return 0;
}
static void GetSystemTimePreciseAsFileTime(FILETIME *t) {
  uint64_t n = UINT64_C(11644473600) * 10000000;
  memcpy(t, &n, sizeof(n));
}
static int QueryPerformanceCounter(LARGE_INTEGER *n) {
  n->QuadPart = 123456789;
  return 1;
}
static int QueryPerformanceFrequency(LARGE_INTEGER *n) {
  n->QuadPart = 10000000;
  return 1;
}
static const char *env_value;
static int env_present, env_probe_error, env_sets;
static DWORD GetEnvironmentVariableA(const char *name, char *buf, DWORD cap) {
  (void)name;
  if (env_probe_error) {
    SetLastError(ERROR_ACCESS_DENIED);
    return 0;
  }
  if (!env_present) {
    SetLastError(ERROR_ENVVAR_NOT_FOUND);
    return 0;
  }
  size_t len = strlen(env_value);
  if (!cap && !len)
    return 0; /* empty success; last error can be stale */
  if (cap <= len)
    return (DWORD)(len + 1);
  memcpy(buf, env_value, len + 1);
  /* Empty success need not replace a stale last-error value. */
  return (DWORD)len;
}
static int SetEnvironmentVariableA(const char *name, const char *value) {
  (void)name;
  (void)value;
  ++env_sets;
  return 1;
}
static intptr_t _get_osfhandle(int fd) { return fd < 0 ? -1 : fd + 1; }
static int64_t cursor = 17;
static uint64_t write_offset;
static DWORD write_count;
static int writes, completed, event_closed, restore_fail;
static int write_success, write_failure, completion_failure;
static int file_type = FILE_TYPE_DISK;
static DWORD GetFileType(HANDLE h) {
  (void)h;
  return (DWORD)file_type;
}
static int SetFilePointerEx(HANDLE h,
                            LARGE_INTEGER at,
                            LARGE_INTEGER *out,
                            DWORD method) {
  (void)h;
  if (method == FILE_BEGIN && restore_fail) {
    SetLastError(ERROR_ACCESS_DENIED);
    return 0;
  }
  if (method == FILE_BEGIN)
    cursor = at.QuadPart;
  if (out)
    out->QuadPart = cursor;
  return 1;
}
static HANDLE CreateEventA(void *attrs,
                           int manual,
                           int initial,
                           const char *name) {
  (void)attrs;
  (void)manual;
  (void)initial;
  (void)name;
  return 8;
}
static int CloseHandle(HANDLE h) {
  (void)h;
  event_closed = 1;
  return 1;
}
static int WriteFile(HANDLE h,
                     const void *buf,
                     DWORD n,
                     DWORD *written,
                     OVERLAPPED *ov) {
  (void)h;
  (void)buf;
  ++writes;
  if (write_failure) {
    SetLastError(ERROR_ACCESS_DENIED);
    return 0;
  }
  write_offset = ((uint64_t)ov->OffsetHigh << 32) | ov->Offset;
  write_count = n;
  cursor = (int64_t)(write_offset + n); /* synchronous handle contract */
  if (written)
    *written = 0;
  SetLastError(
      ERROR_IO_PENDING); /* exercise pending completion deterministically */
  return write_success;
}
static int GetOverlappedResult(HANDLE h,
                               OVERLAPPED *ov,
                               DWORD *written,
                               int wait) {
  (void)h;
  (void)ov;
  completed = wait;
  *written = write_count;
  return !completion_failure;
}
#ifndef PATCHES_CONTRACT_HEADER
#define PATCHES_CONTRACT_HEADER "fio-stl/001 patches.h"
#endif
#include PATCHES_CONTRACT_HEADER

static void *clock_thread(void *arg) {
  (void)arg;
  for (size_t i = 0; i < 10000; ++i) {
    struct timespec ts;
    if (model_clock_gettime(CLOCK_MONOTONIC, &ts) || ts.tv_sec != 12 ||
        ts.tv_nsec != 345678900)
      abort();
  }
  return NULL;
}
static int failures;
#define CHECK(c, msg)                                                          \
  do {                                                                         \
    if (!(c)) {                                                                \
      ++failures;                                                              \
      fprintf(stderr, "%s\n", msg);                                            \
    }                                                                          \
  } while (0)
int main(void) {
  char large[4097];
  memset(large, 'v', sizeof(large) - 1);
  large[sizeof(large) - 1] = 0;
  env_present = 1;
  env_value = large;
  CHECK(fio_sys_env("X") == NULL,
        "F01: oversized value must not expose undefined buffer");
  large[4095] = 0;
  char *value = fio_sys_env("X");
  CHECK(value && strlen(value) == 4095, "F01: last fitting value");
  env_value = "";
  SetLastError(ERROR_ENVVAR_NOT_FOUND);
  value = fio_sys_env("X");
  CHECK(value && !*value, "F01: empty value exists");
  env_present = 0;
  CHECK(!fio_sys_env("X"), "F01: missing value");
  env_probe_error = 1;
  CHECK(fio_sys_env_set("X", "v", 0) == -1,
        "F07: probe failure must not report success");
  env_probe_error = 0;
  env_present = 1;
  env_value = "";
  SetLastError(ERROR_ENVVAR_NOT_FOUND);
  env_sets = 0;
  CHECK(!fio_sys_env_set("X", "v", 0) && !env_sets,
        "F07: empty value must not be overwritten");
  env_present = 0;
  CHECK(!fio_sys_env_set("X", "v", 0) && env_sets == 1,
        "F07: absent value is set");
  writes = 0;
  CHECK(model_pwrite(1, "x", 1, -1) == -1 && errno == EINVAL && !writes,
        "F03: negative offset must not write at zero");
  cursor = 17;
  completed = event_closed = 0;
  CHECK(model_pwrite(1, "x", 1, (off_t)(UINT64_C(1) << 32) + 7) == 1,
        "F02: pending operation must complete before return");
  CHECK(completed && event_closed, "F02: completion and event cleanup");
  CHECK(write_offset == (UINT64_C(1) << 32) + 7, "F03: high offset bits");
  CHECK(cursor == 17, "F03: preserve cursor");
  CHECK(model_pwrite(1, "x", SIZE_MAX, 0) > 0 && write_count <= INT32_MAX,
        "F03: oversized count must not narrow or overflow signed return");
  write_success = 1;
  cursor = 17;
  CHECK(model_pwrite(1, "x", 1, 3) == 1 && cursor == 17,
        "F02/F03: immediate success and cursor restoration");
  write_failure = 1;
  event_closed = 0;
  CHECK(model_pwrite(1, "x", 1, 3) == -1 && event_closed && cursor == 17,
        "F02: failed write releases event");
  write_failure = 0;
  completion_failure = 1;
  event_closed = 0;
  CHECK(model_pwrite(1, "x", 1, 3) == -1 && event_closed && cursor == 17,
        "F02: failed completion releases event and restores cursor");
  completion_failure = 0;
  restore_fail = 1;
  CHECK(model_pwrite(1, "x", 1, 3) == -1 && event_closed,
        "F03: cursor restore error is reported");
  restore_fail = 0;
  file_type = FILE_TYPE_PIPE;
  writes = 0;
  CHECK(model_pwrite(1, "x", 1, 3) == -1 && errno == ESPIPE && !writes,
        "F03: non-seekable descriptor rejected");
  file_type = FILE_TYPE_DISK;
  CHECK(model_pwrite(-1, "x", 1, 0) == -1 && errno == EBADF,
        "F03: invalid descriptor");
  CHECK(sizeof(((struct stat *)0)->st_size) == 8,
        "F06: Windows stat needs 64-bit file sizes");
  struct timespec ts;
  CHECK(!model_clock_gettime(CLOCK_MONOTONIC, &ts) && ts.tv_sec == 12 &&
            ts.tv_nsec == 345678900,
        "F05: QPC conversion");
#ifdef O_TMPFILE
  CHECK(0, "F04: Windows must not invent O_TMPFILE");
#endif
  pthread_t threads[4];
  for (size_t i = 0; i < 4; ++i)
    if (pthread_create(threads + i, NULL, clock_thread, NULL))
      abort();
  for (size_t i = 0; i < 4; ++i)
    if (pthread_join(threads[i], NULL))
      abort();
  fprintf(stderr, "Win32 contract model: %d failure(s)\n", failures);
  return !!failures;
}
#endif
