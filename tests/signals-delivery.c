/* Actual delivery regressions. POSIX delivery is isolated in a bounded child. */
#include "test-helpers.h"
#include <signal.h>
#if FIO_OS_POSIX
#include <sys/wait.h>
static int test_sigaction(int, const struct sigaction *, struct sigaction *);
#define sigaction(...) test_sigaction(__VA_ARGS__)
#endif
#define FIO_SIGNAL_MONITOR_MAX 2
#define FIO_SIGNAL
#include FIO_INCLUDE_FILE
#if FIO_OS_POSIX
#undef sigaction
static int fail_forget;
static int test_sigaction(int sig,
                          const struct sigaction *act,
                          struct sigaction *old) {
  if (fail_forget && act) { /* fail any installation during forget */
    errno = EIO;
    return -1;
  }
  return sigaction(sig, act, old);
}
#endif

static volatile sig_atomic_t calls;
static volatile sig_atomic_t propagated;
static void previous_handler(int sig) {
  (void)sig;
  ++propagated;
}
static void foreign_handler(int sig) { (void)sig; }
static void count_signal(int sig, void *udata) {
  (void)sig;
  (void)udata;
  ++calls;
}

static void delivery(void) {
  /* NULL means ignore, in both immediate and deferred modes. */
  FIO_ASSERT(!fio_signal_monitor(.sig = FIO_SIGNAL_USER1, .immediate = 1),
             "register NULL immediate");
  FIO_ASSERT(!raise(FIO_SIGNAL_USER1), "raise NULL immediate");
  FIO_ASSERT(!fio_signal_review(), "NULL immediate is not a callback");
  FIO_ASSERT(!fio_signal_monitor(.sig = FIO_SIGNAL_USER1), "update NULL");
  FIO_ASSERT(!raise(FIO_SIGNAL_USER1), "raise NULL deferred");
  FIO_ASSERT(!fio_signal_review(), "NULL deferred is not a callback");
  FIO_ASSERT(!fio_signal_forget(FIO_SIGNAL_USER1), "forget NULL monitor");

  for (size_t i = 0; i < 100; ++i) {
    calls = 0;
    FIO_ASSERT(!fio_signal_monitor(.sig = FIO_SIGNAL_USER1,
                                   .callback = count_signal,
                                   .immediate = 1),
               "register immediate / reuse slot");
    for (size_t j = 0; j < 3; ++j)
      FIO_ASSERT(!raise(FIO_SIGNAL_USER1), "repeat immediate");
    FIO_ASSERT(calls == 3, "immediate callback repeats");
    FIO_ASSERT(!fio_signal_review(), "immediate does not queue callback");
    FIO_ASSERT(!fio_signal_forget(FIO_SIGNAL_USER1), "forget repeated monitor");
  }
  calls = 0;
  FIO_ASSERT(
      !fio_signal_monitor(.sig = FIO_SIGNAL_USER1, .callback = count_signal),
      "register deferred");
  FIO_ASSERT(!raise(FIO_SIGNAL_USER1) && !raise(FIO_SIGNAL_USER1), "coalesce");
  FIO_ASSERT(fio_signal_review() == 1 && calls == 1, "deferred coalesces");
  FIO_ASSERT(!fio_signal_review(), "pending consumed once");
  FIO_ASSERT(!raise(FIO_SIGNAL_USER1), "queue before update");
  FIO_ASSERT(
      !fio_signal_monitor(.sig = FIO_SIGNAL_USER1, .callback = count_signal),
      "update pending");
  FIO_ASSERT(fio_signal_review() == 1 && calls == 2, "update retains pending");
  FIO_ASSERT(!raise(FIO_SIGNAL_USER1), "queue before forget");
  FIO_ASSERT(!fio_signal_forget(FIO_SIGNAL_USER1), "forget pending");
  FIO_ASSERT(!fio_signal_review(), "forget discards pending");

  FIO_ASSERT(signal(FIO_SIGNAL_USER1, previous_handler) != SIG_ERR,
             "install one-argument handler");
  propagated = 0;
  FIO_ASSERT(!fio_signal_monitor(.sig = FIO_SIGNAL_USER1, .propagate = 1),
             "register one-argument forwarding");
  for (size_t i = 0; i < 3; ++i)
    FIO_ASSERT(!raise(FIO_SIGNAL_USER1), "repeat one-argument forwarding");
  FIO_ASSERT(propagated == 3, "saved handler survives repeated delivery");
  FIO_ASSERT(!fio_signal_forget(FIO_SIGNAL_USER1), "forget forwarding");
  FIO_ASSERT(signal(FIO_SIGNAL_USER1, previous_handler) == previous_handler,
             "forget restores the previous handler");
  FIO_ASSERT(!fio_signal_monitor(.sig = FIO_SIGNAL_USER1),
             "register under previous handler");
  FIO_ASSERT(signal(FIO_SIGNAL_USER1, foreign_handler) != SIG_ERR,
             "foreign handler replaces monitor");
  FIO_ASSERT(!fio_signal_forget(FIO_SIGNAL_USER1), "forget replaced monitor");
  FIO_ASSERT(signal(FIO_SIGNAL_USER1, SIG_DFL) == foreign_handler,
             "forget leaves a newer foreign handler untouched");
  FIO_ASSERT(!fio_signal_monitor(.sig = SIGINT) &&
                 !fio_signal_monitor(.sig = SIGTERM),
             "fill capacity");
  FIO_ASSERT(fio_signal_monitor(.sig = SIGABRT) == -1,
             "only concurrent monitors consume capacity");
  FIO_ASSERT(!fio_signal_forget(SIGINT), "release capacity");
  FIO_ASSERT(!fio_signal_monitor(.sig = SIGABRT), "reuse released capacity");
  FIO_ASSERT(!fio_signal_forget(SIGTERM) && !fio_signal_forget(SIGABRT),
             "release capacity monitors");
}

#if FIO_OS_POSIX
static volatile sig_atomic_t old_calls, info_ok;
static void old_info(int sig, siginfo_t *info, void *context) {
  ++old_calls;
  sigset_t blocked;
  info_ok = (info && context && info->si_signo == sig &&
             info->si_pid == getpid() && !sigprocmask(SIG_SETMASK, NULL, &blocked) &&
             sigismember(&blocked, SIGUSR2) == 1);
}
static void propagation(void) {
  struct sigaction act = {0};
  act.sa_sigaction = old_info;
  act.sa_flags = SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, SIGUSR2);
  FIO_ASSERT(!sigaction(SIGUSR1, &act, NULL), "install SA_SIGINFO");
  FIO_ASSERT(!fio_signal_monitor(.sig = SIGUSR1,
                                 .callback = count_signal,
                                 .propagate = 1),
             "monitor propagation");
  for (size_t i = 0; i < 3; ++i) {
    FIO_ASSERT(!raise(SIGUSR1), "raise SA_SIGINFO");
    FIO_ASSERT(info_ok && old_calls == (sig_atomic_t)i + 1,
               "propagate original info/context repeatedly");
    FIO_ASSERT(fio_signal_review() == 1, "review propagated delivery");
  }
  fail_forget = 1;
  FIO_ASSERT(fio_signal_forget(SIGUSR1) == -1, "injected removal failure");
  fail_forget = 0;
  FIO_ASSERT(!raise(SIGUSR1), "delivery after failed forget");
  FIO_ASSERT(old_calls == 4 && info_ok && fio_signal_review() == 1,
             "failed forget preserves watcher and propagation");
  FIO_ASSERT(!fio_signal_forget(SIGUSR1), "remove after failed forget");
  struct sigaction restored = {0};
  FIO_ASSERT(!sigaction(SIGUSR1, NULL, &restored) &&
                 (restored.sa_flags & SA_SIGINFO) &&
                 restored.sa_sigaction == old_info &&
                 sigismember(&restored.sa_mask, SIGUSR2) == 1,
             "forget restores previous SA_SIGINFO action and mask");

  for (size_t i = 0; i < 100; ++i)
    FIO_ASSERT(fio_signal_monitor(.sig = SIGKILL) == -1,
               "failed installation must not exhaust slots");
  act.sa_flags = SA_SIGINFO | SA_RESETHAND;
  FIO_ASSERT(!sigaction(SIGUSR1, &act, NULL), "install one-shot action");
  struct sigaction unchanged = {0};
  FIO_ASSERT(!sigaction(SIGUSR1, NULL, &unchanged), "query one-shot action");
  if (unchanged.sa_flags & SA_RESETHAND) {
    FIO_ASSERT(fio_signal_monitor(.sig = SIGUSR1, .propagate = 1) == -1,
               "reject unsupported one-shot propagation");
    FIO_ASSERT(!sigaction(SIGUSR1, NULL, &unchanged) &&
                   unchanged.sa_sigaction == old_info &&
                   (unchanged.sa_flags & SA_RESETHAND),
               "rejection leaves previous OS action intact");
  } else {
    fprintf(stderr,
            "OS does not report SA_RESETHAND in sigaction queries; "
            "one-shot rejection test skipped\n");
  }
  act.sa_handler = SIG_DFL;
  act.sa_flags = 0;
  FIO_ASSERT(!sigaction(SIGUSR1, &act, NULL), "clear one-shot action");
}


#endif

int main(void) {
#if FIO_OS_POSIX
  pid_t child = fork();
  FIO_ASSERT(child != -1, "fork delivery tests");
  if (!child) {
    alarm(30);
    delivery();
    propagation();
    _exit(0);
  }
  int status = 0;
  FIO_ASSERT(waitpid(child, &status, 0) == child, "wait delivery tests");
  FIO_ASSERT(WIFEXITED(status) && !WEXITSTATUS(status),
             "signal delivery child failed: status=%d",
             status);
#else
  delivery();
#endif
  return 0;
}
