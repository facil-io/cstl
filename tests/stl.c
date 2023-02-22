#ifndef FIO_TEST_CSTL
#define FIO_TEST_CSTL
#endif
#ifndef FIO_LEAK_COUNTER
#define FIO_LEAK_COUNTER 1
#endif

#ifdef FIO_UNIFIED
#include "fio-stl.h"
#else
#include "fio-stl/include.h"
#endif

int main(int argc, char const *argv[]) {
  fio_test_dynamic_types();
  (void)argc;
  (void)argv;
  return 0;
}
