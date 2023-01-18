#ifndef FIO_TEST_CSTL
#define FIO_TEST_CSTL
#endif
#ifndef FIO_LEAK_COUNTER
#define FIO_LEAK_COUNTER 1
#endif

#ifdef FIO_MEMALT
#ifndef FIO_MEMCPY
#define FIO_MEMCPY fio_memcpy
#endif
#ifndef FIO_MEMMOVE
#define FIO_MEMMOVE fio_memcpy
#endif
#ifndef FIO_MEMCMP
#define FIO_MEMCMP fio_memcmp
#endif
#ifndef FIO_MEMCHR
#define FIO_MEMCHR fio_memchr
#endif
#ifndef FIO_MEMSET
#define FIO_MEMSET fio_memset
#endif
#endif /* FIO_MEMALT */

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
