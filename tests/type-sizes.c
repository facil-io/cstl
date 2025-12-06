/* *****************************************************************************
C++ extern start
***************************************************************************** */
#include "test-helpers.h"

#define FIO_THREADS
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Environment printout
***************************************************************************** */
int main(void) {
  switch (sizeof(void *)) {
  case 2:
    fprintf(stderr, "\t 16bit words size (unexpected, unknown effects).\n");
    break;
  case 4:
    fprintf(stderr, "\t 32bit words size (some features might be slower).\n");
    break;
  case 8: fprintf(stderr, "\t 64bit words size okay.\n"); break;
  case 16: fprintf(stderr, "\t 128bit words size... wow!\n"); break;
  default:
    fprintf(stderr, "\t Unknown words size %zubit!\n", sizeof(void *) << 3);
    break;
  }
  fprintf(stderr, "\t Using the following type sizes:\n");
  FIO_PRINT_SIZE_OF(char);
  FIO_PRINT_SIZE_OF(short);
  FIO_PRINT_SIZE_OF(int);
  FIO_PRINT_SIZE_OF(float);
  FIO_PRINT_SIZE_OF(long);
  FIO_PRINT_SIZE_OF(double);
  FIO_PRINT_SIZE_OF(size_t);
  FIO_PRINT_SIZE_OF(void *);
  FIO_PRINT_SIZE_OF(uintmax_t);
  FIO_PRINT_SIZE_OF(long double);
#ifdef __SIZEOF_INT128__
  FIO_PRINT_SIZE_OF(__uint128_t);
#endif
  FIO_PRINT_SIZE_OF(fio_thread_t);
  FIO_PRINT_SIZE_OF(fio_thread_mutex_t);
#if FIO_OS_POSIX || defined(_SC_PAGESIZE)
  long page = sysconf(_SC_PAGESIZE);
  if (page > 0) {
    fprintf(stderr, "\t\t%-17s%ld bytes.\n", "Page", page);
    if (page != (1UL << FIO_MEM_PAGE_SIZE_LOG))
      FIO_LOG_INFO("unexpected page size != 4096\n          "
                   "facil.io could be recompiled with:\n          "
                   "`CFLAGS=\"-DFIO_MEM_PAGE_SIZE_LOG=%.0lf\"`",
                   log2(page));
  }
#endif /* FIO_OS_POSIX */
}
