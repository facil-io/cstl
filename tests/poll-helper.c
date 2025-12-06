/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_POLL
#include FIO_INCLUDE_FILE

#if FIO_POLL_ENGINE == FIO_POLL_ENGINE_EPOLL
int main(void) {
  fprintf(stderr,
          "\t* SKIPPED testing file descriptor polling (engine: epoll).\n");
}

#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_KQUEUE
int main(void) {
  fprintf(stderr,
          "\t* SKIPPED testing file descriptor polling (engine: kqueue).\n");
}

#elif FIO_POLL_ENGINE == FIO_POLL_ENGINE_POLL
int main(void) {
  fprintf(
      stderr,
      "\t* Testing file descriptor monitoring (poll setup / cleanup only).\n");
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
