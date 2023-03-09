#ifndef FIO_TEST_ALL
#define FIO_TEST_ALL
#endif

#ifndef FIO_USE_THREAD_MUTEX
#define FIO_USE_THREAD_MUTEX 1
#endif

#include "fio-stl.h"

int main(int argc, char const *argv[]) {
  fio_test_dynamic_types();
  (void)argc;
  (void)argv;
  return 0;
}
