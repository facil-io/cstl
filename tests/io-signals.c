/* Run reactor signal delivery in bounded child processes on POSIX and Windows. */
#include "test-helpers.h"
#define FIO_IO
#include FIO_INCLUDE_FILE
#if FIO_OS_POSIX
#include <sys/wait.h>
#endif

static void test_deadline(void) {
#if FIO_OS_POSIX
  alarm(10);
#endif
}

static void exercise_stop(void) {
  test_deadline();
  FIO___IO.stop = 0;
  fio___io_signal_stopping = 0;
  FIO_ASSERT(!fio_signal_monitor(.sig = SIGTERM,
                                 .callback = fio___io_signal_stop,
                                 .immediate = 1),
             "register reactor stop");
  FIO_ASSERT(!raise(SIGTERM), "raise stop signal");
  FIO_ASSERT(!FIO___IO.stop && fio___io_signal_stopping,
             "shutdown work must wait for normal execution context");
  fio___io_signal_review();
  FIO_ASSERT(FIO___IO.stop, "normal-context review starts shutdown");
  fio___io_signal_review();
  FIO_ASSERT(FIO___IO.stop, "repeated review preserves shutdown");
  FIO_ASSERT(!fio_signal_forget(SIGTERM), "remove stop monitor");
}

static void exercise_force(void) {
  test_deadline();
  fio___io_signal_stopping = 0;
  FIO_ASSERT(!fio_signal_monitor(.sig = SIGTERM,
                                 .callback = fio___io_signal_stop,
                                 .immediate = 1),
             "register forced stop");
  FIO_ASSERT(!raise(SIGTERM), "first stop");
  raise(SIGTERM);
  _exit(1);
}

static int send_stop(void *a, void *b) {
  (void)a;
  (void)b;
  FIO_ASSERT(!raise(SIGTERM), "deliver live reactor stop");
  return -1;
}
static void exercise_lifecycle(void) {
  test_deadline();
  /* Repeated starts must reset stop state and retain shared SIGPIPE protection.
   */
  for (size_t i = 0; i < 3; ++i) {
    fio_io_run_every(.fn = send_stop, .every = 5, .repetitions = 1);
    fio_io_start(0);
    FIO_ASSERT(FIO___IO.stop, "signal stops live reactor");
#ifdef SIGPIPE
    FIO_ASSERT(!raise(SIGPIPE), "SIGPIPE stays suppressed after reactor stop");
#endif
  }
}

#if FIO_OS_POSIX
int main(void) {
  for (int forced = 0; forced != 3; ++forced) {
    pid_t child = fork();
    FIO_ASSERT(child != -1, "fork reactor signal child");
    if (!child) {
      if (forced == 2)
        exercise_lifecycle();
      else if (forced)
        exercise_force();
      else
        exercise_stop();
      _exit(0);
    }
    int status = 0;
    FIO_ASSERT(waitpid(child, &status, 0) == child, "wait reactor signals");
    FIO_ASSERT(WIFEXITED(status) &&
                   WEXITSTATUS(status) == (forced == 1 ? 128 + SIGTERM : 0),
               "reactor signal child status=%d forced=%d",
               status,
               forced);
  }
  return 0;
}
#elif FIO_OS_WIN
static void run_child(const char *executable,
                      const char *mode,
                      DWORD expected) {
  char command[32768];
  int length =
      snprintf(command, sizeof(command), "\"%s\" %s", executable, mode);
  FIO_ASSERT(length > 0 && (size_t)length < sizeof(command), "child command");
  STARTUPINFOA startup = {0};
  PROCESS_INFORMATION process = {0};
  startup.cb = sizeof(startup);
  FIO_ASSERT(CreateProcessA(executable,
                            command,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &startup,
                            &process),
             "create reactor signal child (%s): %lu",
             mode,
             GetLastError());
  DWORD wait_result = WaitForSingleObject(process.hProcess, 15000);
  if (wait_result != WAIT_OBJECT_0) {
    /* Kill only this exact child, never a process group or the coordinator. */
    TerminateProcess(process.hProcess, 1);
    WaitForSingleObject(process.hProcess, 5000);
  }
  DWORD code = 1;
  BOOL queried = GetExitCodeProcess(process.hProcess, &code);
  CloseHandle(process.hThread);
  CloseHandle(process.hProcess);
  FIO_ASSERT(wait_result == WAIT_OBJECT_0 && queried && code == expected,
             "reactor signal child %s: wait=%lu code=%lu expected=%lu",
             mode,
             wait_result,
             code,
             expected);
  fprintf(stderr, "* reactor signal child %s: OK\n", mode);
}

int main(int argc, char **argv) {
  if (argc == 2) {
    if (!strcmp(argv[1], "--stop"))
      exercise_stop();
    else if (!strcmp(argv[1], "--force"))
      exercise_force();
    else if (!strcmp(argv[1], "--lifecycle"))
      exercise_lifecycle();
    else
      return 1;
    return 0;
  }
  char executable[32768];
  DWORD length = GetModuleFileNameA(NULL, executable, sizeof(executable));
  FIO_ASSERT(length && length < sizeof(executable), "resolve test executable");
  run_child(executable, "--stop", 0);
  run_child(executable, "--force", 128 + SIGTERM);
  run_child(executable, "--lifecycle", 0);
  return 0;
}
#endif
