/* *****************************************************************************
Signal Monitoring Tests

NOTE: Signal handling is inherently difficult to unit test safely because:
1. Sending actual signals (SIGINT, SIGTERM, etc.) could terminate the test
process
2. Signal behavior varies between platforms (POSIX vs Windows)
3. Signal handlers run in an async context with limited safe operations

These tests focus on what CAN be safely tested:
- Signal handler registration/deregistration API
- Edge cases and error handling
- The fio_signal_review() function with no pending signals

What CANNOT be safely unit tested (requires integration/manual testing):
- Actual signal delivery and callback invocation
- Signal propagation to previous handlers
- The `immediate` vs deferred callback behavior
- Race conditions in signal handling
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SIGNAL
#include FIO_INCLUDE_FILE

/* Test callback - should never be called in unit tests */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_callback)(int sig, void *udata) {
  (void)sig;
  size_t *counter = (size_t *)udata;
  if (counter)
    ++(*counter);
}

/* *****************************************************************************
Test: Basic signal monitor registration and cleanup
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_basic)(void) {
  /* Test registering a signal monitor */
  int result =
      fio_signal_monitor(.sig = FIO_SIGNAL_USER1,
                         .callback = FIO_NAME_TEST(stl, signal_callback),
                         .udata = NULL,
                         .propagate = false,
                         .immediate = false);
  FIO_ASSERT(result == 0,
             "fio_signal_monitor should succeed for FIO_SIGNAL_USER1");

  /* Test updating an existing monitor (same signal) */
  size_t counter = 0;
  result = fio_signal_monitor(.sig = FIO_SIGNAL_USER1,
                              .callback = FIO_NAME_TEST(stl, signal_callback),
                              .udata = &counter,
                              .propagate = true,
                              .immediate = true);
  FIO_ASSERT(
      result == 0,
      "fio_signal_monitor should succeed when updating existing monitor");

  /* Test cleanup */
  result = fio_signal_forget(FIO_SIGNAL_USER1);
  FIO_ASSERT(result == 0,
             "fio_signal_forget should succeed for FIO_SIGNAL_USER1");
}

/* *****************************************************************************
Test: Edge cases and error handling
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_edge_cases)(void) {
  /* Test: sig=0 should fail */
  int result =
      fio_signal_monitor(.sig = 0,
                         .callback = FIO_NAME_TEST(stl, signal_callback),
                         .udata = NULL);
  FIO_ASSERT(result == -1, "fio_signal_monitor should fail for sig=0");

  /* Test: forget sig=0 should fail */
  result = fio_signal_forget(0);
  FIO_ASSERT(result == -1, "fio_signal_forget should fail for sig=0");

  /* Test: forget unregistered signal should fail */
  result = fio_signal_forget(FIO_SIGNAL_USER_UNREGISTERED);
  FIO_ASSERT(result == -1,
             "fio_signal_forget should fail for unregistered signal");

  /* Test: NULL callback (ignore signal) should work */
  result = fio_signal_monitor(.sig = FIO_SIGNAL_USER1,
                              .callback = NULL,
                              .udata = NULL);
  FIO_ASSERT(result == 0,
             "fio_signal_monitor should succeed with NULL callback (ignore)");
  fio_signal_forget(FIO_SIGNAL_USER1);
}

/* *****************************************************************************
Test: fio_signal_review with no pending signals
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_review)(void) {
  /* Register a monitor */
  size_t counter = 0;
  int result =
      fio_signal_monitor(.sig = FIO_SIGNAL_USER1,
                         .callback = FIO_NAME_TEST(stl, signal_callback),
                         .udata = &counter,
                         .propagate = false,
                         .immediate = false);
  FIO_ASSERT(result == 0, "fio_signal_monitor should succeed");

  /* Review with no signals pending - should return 0 and not call callback */
  int reviewed = fio_signal_review();
  FIO_ASSERT(reviewed == 0,
             "fio_signal_review should return 0 with no pending signals");
  FIO_ASSERT(counter == 0,
             "callback should not be called when no signals pending");

  /* Cleanup */
  fio_signal_forget(FIO_SIGNAL_USER1);
}

/* *****************************************************************************
Test: Multiple signal monitors
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_multiple)(void) {
  /* Common signals available on both POSIX and Windows */
  int signals[] = {SIGINT, SIGILL, SIGABRT, SIGSEGV, SIGTERM};
  size_t num_signals = sizeof(signals) / sizeof(signals[0]);

  /* Register multiple monitors */
  for (size_t i = 0; i < num_signals; ++i) {
    int result =
        fio_signal_monitor(.sig = signals[i],
                           .callback = FIO_NAME_TEST(stl, signal_callback),
                           .udata = NULL);
    FIO_ASSERT(result == 0,
               "fio_signal_monitor should succeed for signal %d",
               signals[i]);
  }

  /* Cleanup all */
  for (size_t i = 0; i < num_signals; ++i) {
    int result = fio_signal_forget(signals[i]);
    FIO_ASSERT(result == 0,
               "fio_signal_forget should succeed for signal %d",
               signals[i]);
  }
}

#if FIO_OS_POSIX
/* *****************************************************************************
Test: POSIX-specific signals
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_posix)(void) {
  /* POSIX-only signals */
  int signals[] = {SIGQUIT,
                   SIGHUP,
                   SIGTRAP,
                   SIGBUS,
                   SIGFPE,
                   SIGUSR1,
                   SIGUSR2,
                   SIGPIPE,
                   SIGALRM};
  size_t num_signals = sizeof(signals) / sizeof(signals[0]);

  /* Register and cleanup */
  for (size_t i = 0; i < num_signals; ++i) {
    int result =
        fio_signal_monitor(.sig = signals[i],
                           .callback = FIO_NAME_TEST(stl, signal_callback),
                           .udata = NULL);
    FIO_ASSERT(result == 0,
               "fio_signal_monitor should succeed for POSIX signal %d",
               signals[i]);
  }

  for (size_t i = 0; i < num_signals; ++i) {
    int result = fio_signal_forget(signals[i]);
    FIO_ASSERT(result == 0,
               "fio_signal_forget should succeed for POSIX signal %d",
               signals[i]);
  }
}
#endif /* FIO_OS_POSIX */

/* *****************************************************************************
Old testing code
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, signal)(void) {

#define FIO___SIGNAL_MEMBER(a)                                                 \
  { (int)a, #a }
  struct {
    int sig;
    const char *name;
  } t[] = {
    FIO___SIGNAL_MEMBER(SIGINT),
    FIO___SIGNAL_MEMBER(SIGILL),
    FIO___SIGNAL_MEMBER(SIGABRT),
    FIO___SIGNAL_MEMBER(SIGSEGV),
    FIO___SIGNAL_MEMBER(SIGTERM),
#if FIO_OS_POSIX
    FIO___SIGNAL_MEMBER(SIGQUIT),
    FIO___SIGNAL_MEMBER(SIGHUP),
    FIO___SIGNAL_MEMBER(SIGTRAP),
    FIO___SIGNAL_MEMBER(SIGBUS),
    FIO___SIGNAL_MEMBER(SIGFPE),
    FIO___SIGNAL_MEMBER(SIGUSR1),
    FIO___SIGNAL_MEMBER(SIGUSR2),
    FIO___SIGNAL_MEMBER(SIGPIPE),
    FIO___SIGNAL_MEMBER(SIGALRM),
    FIO___SIGNAL_MEMBER(SIGCHLD),
    FIO___SIGNAL_MEMBER(SIGCONT),
#endif
  };
#undef FIO___SIGNAL_MEMBER
  size_t e = 0;
  fprintf(stderr, "* testing signal monitoring (setup / cleanup only).\n");
  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    if (fio_signal_monitor(t[i].sig, NULL, NULL, 1)) {
      FIO_LOG_ERROR("couldn't set signal monitoring for %s (%d)",
                    t[i].name,
                    t[i].sig);
      e = 1;
    }
  }
  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    if (fio_signal_forget(t[i].sig)) {
      FIO_LOG_ERROR("couldn't stop signal monitoring for %s (%d)",
                    t[i].name,
                    t[i].sig);
      e = 1;
    }
  }
  FIO_ASSERT(!e, "signal monitoring error");
}
/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  FIO_NAME_TEST(stl, signal_basic)();
  FIO_NAME_TEST(stl, signal_edge_cases)();
  FIO_NAME_TEST(stl, signal_review)();
  FIO_NAME_TEST(stl, signal_multiple)();
#if FIO_OS_POSIX
  FIO_NAME_TEST(stl, signal_posix)();
#endif
  FIO_NAME_TEST(stl, signal)();
  return 0;
}
