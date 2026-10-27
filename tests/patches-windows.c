/* Native Windows regressions; shared temporary-path boundary runs on POSIX. */
#include "test-helpers.h"
#if FIO_OS_WIN
#include <winioctl.h>
#endif

static char *saved_env(const char *name) {
  const char *value = fio_sys_env(name);
  if (!value)
    return NULL;
  size_t len = strlen(value) + 1;
  char *copy = (char *)malloc(len);
  FIO_ASSERT_ALLOC(copy);
  memcpy(copy, value, len);
  return copy;
}
static void restore_env(const char *name, char *value) {
#if FIO_OS_WIN
  FIO_ASSERT(SetEnvironmentVariableA(name, value), "restore environment");
#else
  FIO_ASSERT(!(value ? setenv(name, value, 1) : unsetenv(name)),
             "restore environment");
#endif
  free(value);
}
static void test_tmp_long_path(void) {
  char *old = saved_env("TMPDIR");
  char path[601];
  memset(path, 'x', sizeof(path) - 1);
  path[sizeof(path) - 1] = 0;
  FIO_ASSERT(!fio_sys_env_set("TMPDIR", path, 1), "set long temporary path");
  int fd = fio_filename_tmp();
  restore_env("TMPDIR", old);
  FIO_ASSERT(fd != -1, "long temporary path must fall back to current folder");
  close(fd);
}

#if FIO_OS_WIN
static void test_env(void) {
  char name[80];
  snprintf(name,
           sizeof(name),
           "FIO_PATCHES_ENV_%lu",
           (unsigned long)GetCurrentProcessId());
  char *old = saved_env(name);
  char value[4097];
  memset(value, 'v', sizeof(value) - 1);
  value[sizeof(value) - 1] = 0;
  const size_t lengths[] = {0, 1, 4095, 4096};
  for (size_t i = 0; i < sizeof(lengths) / sizeof(*lengths); ++i) {
    value[lengths[i]] = 0;
    FIO_ASSERT(!fio_sys_env_set(name, value, 1), "set environment boundary");
    SetLastError(ERROR_ENVVAR_NOT_FOUND);
    char *got = fio_sys_env(name);
    FIO_ASSERT(lengths[i] >= 4096 ? !got
                                  : (got && strlen(got) == lengths[i] &&
                                     !memcmp(got, value, lengths[i])),
               "environment boundary %zu",
               lengths[i]);
    FIO_ASSERT(!fio_sys_env_set(name, "replacement", 0), "no-overwrite result");
    if (lengths[i] < 4096) {
      got = fio_sys_env(name);
      FIO_ASSERT(got && strlen(got) == lengths[i], "existing value unchanged");
    }
    value[lengths[i]] = 'v';
  }
  FIO_ASSERT(SetEnvironmentVariableA(name, NULL), "unset environment");
  FIO_ASSERT(!fio_sys_env(name) && !fio_sys_env_set(name, "default", 0),
             "absent environment default");
  restore_env(name, old);
}

static void test_tmp_sentinel(void) {
  char dir[160], path[192];
  snprintf(dir,
           sizeof(dir),
           "tmp/patches-%lu",
           (unsigned long)GetCurrentProcessId());
  FIO_ASSERT(!fio_filename_make_path(.path = dir), "make tmp test folder");
  snprintf(path, sizeof(path), "%s/sentinel", dir);
  int fd = open(path, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
  FIO_ASSERT(fd != -1 && write(fd, "sentinel", 8) == 8, "create sentinel");
  close(fd);
  char *old = saved_env("TMPDIR");
  FIO_ASSERT(!fio_sys_env_set("TMPDIR", path, 1), "sentinel path as tmpdir");
  fd = fio_filename_tmp();
  FIO_ASSERT(fd == -1, "regular file is not a temporary directory");
  fd = open(path, O_RDONLY);
  char buf[9] = {0};
  FIO_ASSERT(fd != -1 && read(fd, buf, 8) == 8 && !memcmp(buf, "sentinel", 8),
             "temporary file creation must not delete or change sentinel");
  close(fd);
  unlink(path);
  FIO_ASSERT(!fio_sys_env_set("TMPDIR", dir, 1), "temporary directory");
  fd = fio_filename_tmp();
  restore_env("TMPDIR", old);
  FIO_ASSERT(fd != -1, "temporary file in actual directory");
  close(fd);
  /* Delete-on-close must leave the directory empty. */
  FIO_ASSERT(RemoveDirectoryA(dir),
             "temporary file name must disappear on close");
}

static DWORD WINAPI clock_worker(void *arg) {
  (void)arg;
  struct timespec prev = {0}, now;
  for (size_t i = 0; i < 10000; ++i) {
    FIO_ASSERT(!clock_gettime(CLOCK_MONOTONIC, &now) && now.tv_nsec >= 0 &&
                   now.tv_nsec < 1000000000L,
               "valid monotonic clock");
    FIO_ASSERT(now.tv_sec > prev.tv_sec ||
                   (now.tv_sec == prev.tv_sec && now.tv_nsec >= prev.tv_nsec),
               "nondecreasing clock");
    prev = now;
  }
  return 0;
}
static void test_clock(void) {
  HANDLE threads[4];
  for (size_t i = 0; i < 4; ++i) {
    threads[i] = CreateThread(NULL, 0, clock_worker, NULL, 0, NULL);
    FIO_ASSERT(threads[i], "create clock thread");
  }
  for (size_t i = 0; i < 4; ++i) {
    FIO_ASSERT(WaitForSingleObject(threads[i], INFINITE) == WAIT_OBJECT_0,
               "join clock thread");
    CloseHandle(threads[i]);
  }
}

static void test_pwrite(void) {
  int fd = fio_filename_tmp();
  FIO_ASSERT(fd != -1 && write(fd, "abcdef", 6) == 6, "pwrite fixture");
  FIO_ASSERT(lseek(fd, 2, SEEK_SET) == 2, "set cursor");
  FIO_ASSERT(pwrite(fd, "XY", 2, 3) == 2 && lseek(fd, 0, SEEK_CUR) == 2,
             "pwrite preserves synchronous cursor");
  char buf[7] = {0};
  FIO_ASSERT(lseek(fd, 0, SEEK_SET) == 0 && read(fd, buf, 6) == 6 &&
                 !memcmp(buf, "abcXYf", 6),
             "pwrite positional content");
  FIO_ASSERT(pwrite(fd, "z", 1, -1) == -1 && errno == EINVAL,
             "negative pwrite offset");
  close(fd);

  /* Genuine overlapped file, associated with IOCP: no stack pointer may be
   * left in the completion queue when the synchronous shim returns. */
  char path[160];
  snprintf(path,
           sizeof(path),
           "tmp/patches-overlapped-%lu",
           (unsigned long)GetCurrentProcessId());
  HANDLE file = CreateFileA(path,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE,
                            NULL);
  FIO_ASSERT(file != INVALID_HANDLE_VALUE, "overlapped fixture");
  HANDLE port = CreateIoCompletionPort(file, NULL, 1, 0);
  FIO_ASSERT(port, "IOCP fixture");
  fd = _open_osfhandle((intptr_t)file, _O_RDWR | _O_BINARY);
  FIO_ASSERT(fd != -1, "CRT overlapped descriptor");
  FIO_ASSERT(pwrite(fd, "async", 5, 7) == 5, "overlapped completion");
  DWORD got = 0;
  ULONG_PTR key = 0;
  OVERLAPPED *ov = NULL;
  FIO_ASSERT(!GetQueuedCompletionStatus(port, &got, &key, &ov, 0) && !ov &&
                 GetLastError() == WAIT_TIMEOUT,
             "no stale stack completion packet");
  HANDLE event = CreateEventA(NULL, TRUE, FALSE, NULL);
  FIO_ASSERT(event, "read event");
  OVERLAPPED ro = {0};
  ro.Offset = 7;
  ro.hEvent = (HANDLE)((uintptr_t)event | 1);
  memset(buf, 0, sizeof(buf));
  int ok = ReadFile(file, buf, 5, NULL, &ro);
  FIO_ASSERT(ok || GetLastError() == ERROR_IO_PENDING, "overlapped read");
  FIO_ASSERT(GetOverlappedResult(file, &ro, &got, TRUE) && got == 5 &&
                 !memcmp(buf, "async", 5),
             "overlapped write finished");
  CloseHandle(event);
  close(fd); /* owns file */
  CloseHandle(port);
}

static void test_large_stat(void) {
  char path[160];
  snprintf(path,
           sizeof(path),
           "tmp/patches-large-%lu",
           (unsigned long)GetCurrentProcessId());
  int fd = open(path, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
  FIO_ASSERT(fd != -1, "large stat fixture");
  HANDLE file = (HANDLE)_get_osfhandle(fd);
  DWORD ignored = 0;
  FIO_ASSERT(
      DeviceIoControl(file, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &ignored, NULL),
      "mark sparse file (NTFS required)");
  const int64_t sizes[] = {INT32_MAX,
                           INT64_C(1) << 31,
                           (INT64_C(1) << 32) + 17};
  for (size_t i = 0; i < sizeof(sizes) / sizeof(*sizes); ++i) {
    LARGE_INTEGER size;
    size.QuadPart = sizes[i];
    FIO_ASSERT(SetFilePointerEx(file, size, NULL, FILE_BEGIN) &&
                   SetEndOfFile(file),
               "set sparse size");
    struct stat st;
    FIO_ASSERT(sizeof(st.st_size) >= 8 && !fstat(fd, &st) &&
                   st.st_size == size.QuadPart,
               "fstat 64-bit size");
    FIO_ASSERT(!stat(path, &st) && st.st_size == size.QuadPart,
               "stat matching 64-bit layout");
    if (SIZE_MAX > UINT32_MAX) {
      FIO_ASSERT(fio_fd_size(fd) == (uint64_t)size.QuadPart &&
                     fio_filename_size(path) == (uint64_t)size.QuadPart,
                 "public file size");
    }
  }
  close(fd);
  unlink(path);
}
#endif

int main(void) {
  test_tmp_long_path();
#if FIO_OS_WIN
  test_env();
  test_tmp_sentinel();
  test_clock();
  test_pwrite();
  test_large_stat();
#else
  fprintf(stderr, "Native Windows cases skipped on POSIX\n");
#endif
  return 0;
}
