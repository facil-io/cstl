
#define FIO_MALLOC_TEST_NOTICE                                                 \
  "Testing FIO_MEMORY_NAME with the default, small allocator, settings."

/* use the fast-setup global allocator shortcut for FIO_MEMORY_NAME */
#define FIO_MEMORY_NAME fio
#define FIO_LOG
#define FIO_TIME
#define FIO_ATOMIC
#include <fio-stl.h>
#define H___FIO_MALLOC___H
#include "malloc.c"
