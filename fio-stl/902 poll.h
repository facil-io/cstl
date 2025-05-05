/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_POLL Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_POLL_TEST___H)
#define H___FIO_POLL_TEST___H
#ifndef H___FIO_POLL___H
#define FIO_POLL
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

#if FIO_POLL_ENGINE == FIO_POLL_ENGINE_EPOLL
FIO_SFUNC void FIO_NAME_TEST(stl, poll)(void) {
  fprintf(stderr,
          "* SKIPPED testing file descriptor polling (engine: epoll).\n");
}

#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_KQUEUE
FIO_SFUNC void FIO_NAME_TEST(stl, poll)(void) {
  fprintf(stderr,
          "* SKIPPED testing file descriptor polling (engine: kqueue).\n");
}

#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_POLL
FIO_SFUNC void FIO_NAME_TEST(stl, poll)(void) {
  fprintf(
      stderr,
      "* Testing file descriptor monitoring (poll setup / cleanup only).\n");
  fio_poll_s p;
  fio_poll_init(&p, NULL);
  short events[4] = {POLLOUT, POLLIN, POLLOUT | POLLIN, POLLOUT | POLLIN};
  for (size_t i = 128; i--;) {
    FIO_ASSERT(!fio_poll_monitor(&p, i, (void *)(uintptr_t)i, events[(i & 3)]),
               "fio_poll_monitor failed for fd %d",
               i);
  }
  for (size_t i = 128; i--;) {
    if ((i & 3) == 3) {
      FIO_ASSERT(!fio_poll_forget(&p, i), "fio_poll_forget failed at %d", i);
      FIO_ASSERT(fio_poll_forget(&p, i),
                 "fio_poll_forget didn't forget previous %d",
                 i);
    }
  }
  fio_poll_destroy(&p);
}

#endif
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
