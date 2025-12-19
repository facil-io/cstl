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
  FIO_LOG_DDEBUG("Testing signal monitor registration and cleanup.");

  /* Test registering a signal monitor */
  int result =
      fio_signal_monitor(.sig = SIGUSR1,
                         .callback = FIO_NAME_TEST(stl, signal_callback),
                         .udata = NULL,
                         .propagate = false,
                         .immediate = false);
  FIO_ASSERT(result == 0, "fio_signal_monitor should succeed for SIGUSR1");

  /* Test updating an existing monitor (same signal) */
  size_t counter = 0;
  result = fio_signal_monitor(.sig = SIGUSR1,
                              .callback = FIO_NAME_TEST(stl, signal_callback),
                              .udata = &counter,
                              .propagate = true,
                              .immediate = true);
  FIO_ASSERT(
      result == 0,
      "fio_signal_monitor should succeed when updating existing monitor");

  /* Test cleanup */
  result = fio_signal_forget(SIGUSR1);
  FIO_ASSERT(result == 0, "fio_signal_forget should succeed for SIGUSR1");

  FIO_LOG_DDEBUG("Basic registration/cleanup: PASSED");
}

/* *****************************************************************************
Test: Edge cases and error handling
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_edge_cases)(void) {
  FIO_LOG_DDEBUG("Testing signal monitor edge cases.");

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
  result = fio_signal_forget(SIGUSR2);
  FIO_ASSERT(result == -1,
             "fio_signal_forget should fail for unregistered signal");

  /* Test: NULL callback (ignore signal) should work */
  result = fio_signal_monitor(.sig = SIGUSR1, .callback = NULL, .udata = NULL);
  FIO_ASSERT(result == 0,
             "fio_signal_monitor should succeed with NULL callback (ignore)");
  fio_signal_forget(SIGUSR1);

  FIO_LOG_DDEBUG("Edge cases: PASSED");
}

/* *****************************************************************************
Test: fio_signal_review with no pending signals
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_review)(void) {
  FIO_LOG_DDEBUG("Testing fio_signal_review with no pending signals.");

  /* Register a monitor */
  size_t counter = 0;
  int result =
      fio_signal_monitor(.sig = SIGUSR1,
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
  fio_signal_forget(SIGUSR1);

  FIO_LOG_DDEBUG("Signal review (no pending): PASSED");
}

/* *****************************************************************************
Test: Multiple signal monitors
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_multiple)(void) {
  FIO_LOG_DDEBUG("Testing multiple signal monitors.");

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

  FIO_LOG_DDEBUG("Multiple monitors: PASSED");
}

#if FIO_OS_POSIX
/* *****************************************************************************
Test: POSIX-specific signals
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, signal_posix)(void) {
  FIO_LOG_DDEBUG("Testing POSIX-specific signal monitors.");

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

  FIO_LOG_DDEBUG("POSIX signals: PASSED");
}
#endif /* FIO_OS_POSIX */

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  FIO_LOG_DDEBUG("Testing Signal Monitoring Module");
  FIO_LOG_DDEBUG("================================");
  FIO_LOG_DDEBUG("NOTE: These tests verify API behavior without sending actual "
                 "signals.");
  FIO_LOG_DDEBUG("Full signal handling requires integration/manual testing.");

  FIO_NAME_TEST(stl, signal_basic)();
  FIO_NAME_TEST(stl, signal_edge_cases)();
  FIO_NAME_TEST(stl, signal_review)();
  FIO_NAME_TEST(stl, signal_multiple)();
#if FIO_OS_POSIX
  FIO_NAME_TEST(stl, signal_posix)();
#endif

  FIO_LOG_DDEBUG("================================");
  FIO_LOG_DDEBUG("Signal monitoring tests complete.");
  FIO_LOG_DDEBUG("Limitations (not tested - require integration testing):");
  FIO_LOG_DDEBUG("  - Actual signal delivery and callback invocation");
  FIO_LOG_DDEBUG("  - Signal propagation to previous handlers");
  FIO_LOG_DDEBUG("  - Immediate vs deferred callback behavior");
  FIO_LOG_DDEBUG("  - Race conditions in signal handling");

  return 0;
}
