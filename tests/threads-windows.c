/* Windows ABI, handle lifetime, process polling and timeout regressions. */
#if defined(_WIN32)
#include <errno.h>
#include <stdint.h>
#include <synchapi.h>
#include <tlhelp32.h>
#include <windows.h>
#include <winsock2.h>
/* Wine 10 reports a successful handle count of zero. Count the wrapper's
 * acquisitions/closes too, so the leak regression is meaningful under Wine. */
static size_t process_acquires, process_closes;
static size_t thread_allocations, thread_frees, thread_handles;
static int fail_create, fail_wait, fail_open, fail_exit_code;
static int count_process_handles;
static LPVOID test_allocate(HANDLE heap, DWORD flags, SIZE_T bytes) {
  LPVOID p = HeapAlloc(heap, flags, bytes);
  thread_allocations += p != NULL;
  return p;
}
static BOOL test_free(HANDLE heap, DWORD flags, LPVOID p) {
  BOOL r = HeapFree(heap, flags, p);
  thread_frees += r && p != NULL;
  return r;
}
static DWORD test_wait(HANDLE h, DWORD milliseconds) {
  if (fail_wait) {
    SetLastError(ERROR_INVALID_HANDLE);
    return WAIT_FAILED;
  }
  if (fail_exit_code)
    return WAIT_OBJECT_0; /* reach status-query failure while the child is live
                           */
  return WaitForSingleObject(h, milliseconds);
}
#include <process.h>
static uintptr_t test_begin(void *security,
                            unsigned stack_size,
                            unsigned(__stdcall *fn)(void *),
                            void *arg,
                            unsigned flags,
                            unsigned *tid) {
  if (fail_create) {
    errno = EAGAIN;
    return 0;
  }
  uintptr_t h = _beginthreadex(security, stack_size, fn, arg, flags, tid);
  thread_handles += h != 0;
  return h;
}
#define HeapAlloc           test_allocate
#define HeapFree            test_free
#define WaitForSingleObject test_wait
#define _beginthreadex      test_begin
static HANDLE test_open_process(DWORD access, BOOL inherit, DWORD pid) {
  if (fail_open) {
    SetLastError(ERROR_ACCESS_DENIED);
    return NULL;
  }
  HANDLE h = OpenProcess(access, inherit, pid);
  process_acquires += count_process_handles && h != NULL;
  return h;
}
static HANDLE test_snapshot(DWORD flags, DWORD pid) {
  HANDLE h = CreateToolhelp32Snapshot(flags, pid);
  process_acquires += count_process_handles && h != INVALID_HANDLE_VALUE;
  return h;
}
static BOOL test_close(HANDLE h) {
  BOOL r = CloseHandle(h);
  process_closes += count_process_handles && r;
  if (!count_process_handles && r && thread_handles)
    --thread_handles;
  return r;
}
static BOOL test_exit_code(HANDLE h, LPDWORD code) {
  if (fail_exit_code) {
    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  return GetExitCodeProcess(h, code);
}
#define GetExitCodeProcess       test_exit_code
#define OpenProcess              test_open_process
#define CreateToolhelp32Snapshot test_snapshot
#define CloseHandle              test_close
static DWORD observed_timeout;
static BOOL test_timedwait(PCONDITION_VARIABLE c,
                           PSRWLOCK m,
                           DWORD ms,
                           ULONG flags) {
  (void)c;
  (void)m;
  (void)flags;
  observed_timeout = ms;
  SetLastError(ERROR_TIMEOUT);
  return FALSE;
}
#define SleepConditionVariableSRW test_timedwait
#endif
#define FIO_CORE
#define FIO_THREADS
#include "fio-stl/include.h"

#if FIO_OS_WIN
static void *test_return(void *p) {
  InterlockedIncrement((volatile LONG *)p);
  return p;
}
static void *test_detach(void *p) {
  FIO_ASSERT(!fio_thread_detach(&(fio_thread_t){fio_thread_current()}),
             "detach self after safe startup publication");
  SetEvent((HANDLE)p);
  return NULL;
}
static void *test_exit(void *p) {
  InterlockedIncrement((volatile LONG *)p);
  fio_thread_exit();
  abort();
  return NULL;
}
static void test_lifecycle(void) {
  DWORD before, after;
  volatile LONG calls = 0;
  FIO_ASSERT(GetProcessHandleCount(GetCurrentProcess(), &before),
             "handle count");
  for (size_t i = 0; i < 128; ++i) {
    fio_thread_t t;
    FIO_ASSERT(!fio_thread_create(&t,
                                  (i & 1) ? test_exit : test_return,
                                  (void *)&calls),
               "create");
    /* Join after the OS thread has already exited, not only while it is live.
     */
    Sleep(1);
    FIO_ASSERT(!fio_thread_join(&t), "join exited thread");
  }
  FIO_ASSERT(calls == 128, "callback completion");
  FIO_ASSERT(!thread_handles && thread_allocations == thread_frees,
             "thread startup/handle leak");
  FIO_ASSERT(GetProcessHandleCount(GetCurrentProcess(), &after) &&
                 after == before,
             "thread handles leaked: %lu -> %lu",
             before,
             after);

  fio_thread_t t = 0;
  fail_create = 1;
  FIO_ASSERT(fio_thread_create(&t, test_return, (void *)&calls) == -1 && !t,
             "OS create failure");
  fail_create = 0;
  FIO_ASSERT(thread_allocations == thread_frees,
             "failed create leaked startup");
  FIO_ASSERT(!fio_thread_create(&t, test_return, (void *)&calls), "create");
  fail_wait = 1;
  FIO_ASSERT(fio_thread_join(&t) == -1 && t,
             "join error must retain ownership");
  fail_wait = 0;
  FIO_ASSERT(!fio_thread_join(&t), "join retry");
  FIO_ASSERT(!thread_handles && thread_allocations == thread_frees,
             "join retry leaked");
  HANDLE done = CreateEventW(NULL, TRUE, FALSE, NULL);
  FIO_ASSERT(done != NULL, "detach completion event");
  for (size_t i = 0; i < 32; ++i) {
    ResetEvent(done);
    FIO_ASSERT(!fio_thread_create(&t, test_detach, done),
               "create self-detacher");
    FIO_ASSERT(WaitForSingleObject(done, 5000) == WAIT_OBJECT_0,
               "self-detached worker completion");
    FIO_ASSERT(!thread_handles && thread_allocations == thread_frees,
               "detach leaked startup or handle");
  }
  CloseHandle(done);
}
static PROCESS_INFORMATION test_child(HANDLE gate) {
  wchar_t path[MAX_PATH];
  wchar_t command[MAX_PATH + 80];
  STARTUPINFOW startup = {0};
  PROCESS_INFORMATION child = {0};
  startup.cb = sizeof(startup);
  FIO_ASSERT(GetModuleFileNameW(NULL, path, MAX_PATH), "executable path");
  swprintf(command,
           sizeof(command) / sizeof(*command),
           L"\"%ls\" --child %llu",
           path,
           (unsigned long long)(uintptr_t)gate);
  FIO_ASSERT(CreateProcessW(path,
                            command,
                            NULL,
                            NULL,
                            TRUE,
                            0,
                            NULL,
                            NULL,
                            &startup,
                            &child),
             "spawn child");
  CloseHandle(child.hThread);
  return child;
}
static void test_process(void) {
  SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
  HANDLE gate = CreateEventW(&sa, TRUE, FALSE, NULL);
  FIO_ASSERT(gate != NULL, "child gate");
  PROCESS_INFORMATION child = test_child(gate);
  DWORD before, after;
  int status = 0x12345678;
  FIO_ASSERT(GetProcessHandleCount(GetCurrentProcess(), &before),
             "handle count");
  count_process_handles = 1;
  for (size_t i = 0; i < 64; ++i) {
    FIO_ASSERT(!fio_thread_waitpid((fio_thread_pid_t)-1, &status, WNOHANG),
               "any live child");
    FIO_ASSERT(status == 0x12345678, "poll must leave status unchanged");
  }
  FIO_ASSERT(GetProcessHandleCount(GetCurrentProcess(), &after),
             "handle count");
  fprintf(stderr, "Process poll handle count: %lu -> %lu\n", before, after);
  FIO_ASSERT(after == before && process_acquires >= 128 &&
                 process_acquires == process_closes,
             "process polls leaked handles: %zu acquired, %zu closed",
             process_acquires,
             process_closes);
  for (size_t i = 0; i < 64; ++i)
    FIO_ASSERT(!fio_thread_waitpid((fio_thread_pid_t)child.dwProcessId,
                                   &status,
                                   WNOHANG),
               "specific live child");
  FIO_ASSERT(GetProcessHandleCount(GetCurrentProcess(), &after) &&
                 after == before && process_acquires == process_closes,
             "specific process polls leaked handles");
  fprintf(stderr,
          "Process poll acquisitions/closes: %zu/%zu\n",
          process_acquires,
          process_closes);
  count_process_handles = 0;
  FIO_ASSERT(fio_thread_waitpid(child.dwProcessId, &status, 0x40000000) == -1 &&
                 errno == EINVAL,
             "invalid wait options");
  FIO_ASSERT(fio_thread_kill(0, 0) == -1 && errno == ENOSYS,
             "unsupported group kill must not target self");
  fail_open = 1;
  FIO_ASSERT(fio_thread_waitpid(child.dwProcessId, &status, WNOHANG) == -1 &&
                 errno == EPERM && status == 0x12345678,
             "OpenProcess NULL must be reported without touching status");
  fail_open = 0;
  fail_exit_code = 1;
  FIO_ASSERT(fio_thread_waitpid(child.dwProcessId, &status, 0) == -1 &&
                 status == 0x12345678,
             "failed status query must not fabricate clean exit");
  fail_exit_code = 0;
  SetEvent(gate);
  FIO_ASSERT(fio_thread_waitpid(child.dwProcessId, &status, 0) ==
                 (int)child.dwProcessId,
             "wait returns child, not parent");
  FIO_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 37 &&
                 !WIFSIGNALED(status),
             "exit status macros");
  FIO_ASSERT(WIFSIGNALED(9) && WTERMSIG(9) == 9 && WIFSTOPPED(0x7F) &&
                 WSTOPSIG(0x137F) == 0x13,
             "fallback status macros");
  CloseHandle(child.hProcess);
  CloseHandle(gate);
}
#endif
int main(int argc, char **argv) {
#if FIO_OS_WIN
  if (argc == 3 && !strcmp(argv[1], "--child")) {
    HANDLE gate = (HANDLE)(uintptr_t)strtoull(argv[2], NULL, 10);
    if (WaitForSingleObject(gate, 15000) != WAIT_OBJECT_0)
      return 99;
    return 37;
  }
  fprintf(stderr, "Windows thread lifecycle\n");
  test_lifecycle();
  fprintf(stderr, "Windows child process polling\n");
  test_process();
  fprintf(stderr, "Windows finite timeout bounds\n");
  fio_thread_cond_t c = {0};
  fio_thread_mutex_t m = FIO_THREAD_MUTEX_INIT;
  FIO_ASSERT(fio_thread_cond_timedwait(&c, &m, SIZE_MAX) != 0 &&
                 observed_timeout != INFINITE,
             "finite wait became infinite");
  FIO_ASSERT(fio_thread_cond_timedwait(&c, &m, (size_t)INFINITE) != 0 &&
                 observed_timeout != INFINITE,
             "DWORD_MAX wait became infinite");
#else
  (void)argc;
  (void)argv;
#endif
  return 0;
}
