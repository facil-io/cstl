/* *****************************************************************************
Test - Malloc Module (FIO_MALLOC_OVERRIDE_SYSTEM)

Replaces the program's malloc family with the custom fio allocator. Covers
start-up (state callbacks allocate while the allocator initializes), the libc
contracts of the overridden functions, and teardown after exit cleanup.

Skipped on Windows (CRT symbols can't be replaced at link time) and when the
custom allocator is disabled (sanitizer builds).
***************************************************************************** */
#include "fio-stl/include.h"

#if !defined(FIO_MEMORY_DISABLE) && !defined(_WIN32)
#define FIO_MALLOC_OVERRIDE_SYSTEM 1
#define TEST_OVERRIDE              1 /* the macro is consumed by the include */
#endif
#define FIO_MALLOC
#include FIO_INCLUDE_FILE

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  /* basic family: allocation, zeroing, reallocation */
  char *s = (char *)malloc(100);
  FIO_ASSERT(s, "malloc failed");
  memset(s, 'x', 100);
  s = (char *)realloc(s, 4096);
  FIO_ASSERT(s && s[0] == 'x' && s[99] == 'x', "realloc lost data");
  free(s);
  uint8_t *z = (uint8_t *)calloc(64, 64);
  FIO_ASSERT(z, "calloc failed");
  for (size_t i = 0; i < 4096; ++i)
    FIO_ASSERT(!z[i], "calloc memory not zeroed at %zu", i);
  free(z);

#ifdef TEST_OVERRIDE
  /* posix_memalign: failure never reports success nor touches memptr */
  {
    void *const sentinel = (void *)(uintptr_t)0x1234;
    void *p = sentinel;
    errno = 0;
    int r = posix_memalign(&p, 64, ~(size_t)0);
    FIO_ASSERT(r == ENOMEM, "posix_memalign(SIZE_MAX) returned %d", r);
    FIO_ASSERT(p == sentinel && !errno, "posix_memalign failure side effects");
    r = posix_memalign(&p, 48, 64);
    FIO_ASSERT(r == EINVAL && p == sentinel, "non pow2 alignment => EINVAL");
    r = posix_memalign(&p, 256, 100);
    FIO_ASSERT(!r && p != sentinel && !((uintptr_t)p & 255),
               "posix_memalign(256) failed (%d)",
               r);
    free(p);
  }
#endif /* TEST_OVERRIDE */

  /* the allocator outlives (and survives) exit cleanup: stdio and libc keep
   * allocating / freeing after `FIO_CALL_AFTER_EXIT` */
  {
    void *keep_small = malloc(32);
    void *keep_big = malloc(fio_malloc_block_size()); /* big-block route */
    FIO_ASSERT(keep_small && keep_big, "allocation failed");
    fio_state_callback_force(FIO_CALL_AFTER_EXIT);
    free(keep_small);
    free(keep_big);
    void *again = malloc(64); /* re-initializes the allocator */
    FIO_ASSERT(again, "allocation after cleanup failed");
    free(again);
  }
  fprintf(stderr, "* malloc override: passed.\n");
  return 0;
}
