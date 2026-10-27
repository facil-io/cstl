/* Moving allocator for renderer security regression units.
 * Include before fio-stl; use only this TU. All tracked allocations must
 * balance.
 */
#ifndef H___RENDERER_TEST_ALLOC___H
#define H___RENDERER_TEST_ALLOC___H
#include <assert.h>
#include <stdio.h>
/* Assertions must remain active under the normal NDEBUG make preset. */
#undef assert
#define assert(check)                                                         \
  do {                                                                        \
    if (!(check)) {                                                            \
      fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #check);    \
      abort();                                                                 \
    }                                                                         \
  } while (0)
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
static size_t renderer_alloc_live;
static void *renderer_realloc(void *old,
                              size_t old_size,
                              size_t size,
                              size_t copy) {
  (void)old_size;
  void *p = malloc(size);
  if (!p)
    return NULL;
  if (old) {
    memcpy(p, old, copy < size ? copy : size);
    free(old);
  } else {
    ++renderer_alloc_live;
  }
  return p;
}
static void renderer_free(void *p) {
  if (p) {
    assert(renderer_alloc_live);
    --renderer_alloc_live;
    free(p);
  }
}
/* Core's inline buffer helpers precede memalt declarations in standalone
 * includes. Keep these allocator-isolated tests on the libc string helper. */
#define FIO_MEMORY_DISABLE           1
#define FIO_STRLEN                   strlen
#define FIO_MEM_REALLOC_(p, o, n, c) renderer_realloc((p), (o), (n), (c))
#define FIO_MEM_FREE_(p, n)          renderer_free((p))
#define FIO_MEM_REALLOC_IS_SAFE      0
#define FIO_MEM_ALIGNMENT_SIZE       16
#define FIO_MEM_REALLOC_IS_SAFE_     0
#define FIO_MEM_ALIGNMENT_SIZE_      16
#define FIO_MEM_REALLOC_ALIGNED_(p, o, n, c, a) ((p) = renderer_realloc((p), (o), (n), (c)))
#define FIO_MEM_FREE_ALIGNED_(p, n) renderer_free((p))
#endif
