/* A failed join must not return while a copy worker owns stack/buffers. */
#if !defined(_WIN32)
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
static int test_join(pthread_t thread, void **value) {
  (void)thread;
  (void)value;
  return EINVAL;
}
#define pthread_join test_join
/* Keep the expected fatal assertion inside this disposable child. */
#define FIO_KILL_SELF() ((void)0)
#define FIO_CORE
#define FIO_THREADS
#include "fio-stl/include.h"

int main(void) {
  pid_t child = fork();
  FIO_ASSERT(child >= 0, "fork join-failure test");
  if (!child) {
    static unsigned char src[1U << 23];
    static unsigned char dst[1U << 23];
    if (!freopen("/dev/null", "w", stderr)) /* best effort: silence child */
      (void)0;
    fio_thread_memcpy(dst, src, sizeof(src));
    _exit(0); /* returning is the defect */
  }
  int status;
  FIO_ASSERT(waitpid(child, &status, 0) == child, "wait for join-failure test");
  FIO_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) != 0,
             "copy must fail closed, never return after failed worker join");
  return 0;
}
#else
int main(void) { return 0; }
#endif
