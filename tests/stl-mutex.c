#ifndef FIO_TEST_CSTL
#define FIO_TEST_CSTL
#endif

#ifndef FIO_USE_PTHREAD_MUTEX
#define FIO_USE_PTHREAD_MUTEX 1
#endif

#include "fio-stl.h"

int main(int argc, char const *argv[]) {
  fio_test_dynamic_types();
  (void)argc;
  (void)argv;
  return 0;
}
