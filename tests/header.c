/* Common header: system aligned allocation and custom pointer tagging.
 * header_generic.c forces only the portable backend, not a native generic OS.
 */
#define FIO_CORE
#define FIO_MEMORY_DISABLE 1
#define FIO_INCLUDE_FILE   "fio-stl/include.h"
#include "fio-stl/000 core.h"

/* The core alone deliberately has no logger; keep assertion failures visible.
 */
#undef FIO_LOG2STDERR
#define FIO_LOG2STDERR(...) fprintf(stderr, __VA_ARGS__)
#undef FIO_LOG_LEVEL_GET
#define FIO_LOG_LEVEL_GET() FIO_LOG_LEVEL_FATAL

#ifdef TEST_HEADER_GENERIC
#undef FIO_OS_POSIX
#define FIO_OS_POSIX 0
#undef FIO_OS_WIN
#define FIO_OS_WIN 0

#endif

#define FIO_PTR_TAG(p)   ((uintptr_t)(p) | (uintptr_t)1)
#define FIO_PTR_UNTAG(p) ((uintptr_t)(p) & ~(uintptr_t)1)
#define FIO_PTR_TAG_VALIDATE(p)                                                \
  (((uintptr_t)(p) & (uintptr_t)1) && FIO_PTR_UNTAG(p))
#include "fio-stl/001 header.h"

FIO_ASSERT_STATIC(FIO_MEM_ALIGNMENT_SIZE >= sizeof(void *),
                  "system alignment must accommodate the metadata pointer");
FIO_ASSERT_STATIC(FIO_MEM_ALIGNMENT_SIZE >= _Alignof(long double),
                  "system alignment must accommodate long double");
FIO_ASSERT_STATIC(!(FIO_MEM_ALIGNMENT_SIZE & (FIO_MEM_ALIGNMENT_SIZE - 1)),
                  "system alignment must be a power of two");
FIO_ASSERT_STATIC(FIO_MEM_ALIGNMENT_SIZE_ == FIO_MEM_ALIGNMENT_SIZE,
                  "temporary system allocations must use the same default");

static void test_aligned_allocations(void) {
  const size_t requests[] = {0, 1, 3, 12, 16, 48, 128, 4096};
  for (size_t i = 0; i < sizeof(requests) / sizeof(*requests); ++i) {
    size_t alignment = (size_t)FIO_MEM_ALIGNMENT_SIZE;
    if (requests[i]) {
      size_t floor = (size_t)1 << fio_msb_index_unsafe(requests[i]);
      if (floor > alignment)
        alignment = floor;
    }
    uint8_t *p = NULL;
    FIO_MEM_REALLOC_ALIGNED(p, 0, 33, 0, requests[i]);
    FIO_ASSERT(p && !((uintptr_t)p & (alignment - 1)),
               "allocation/default alignment failed for %zu",
               requests[i]);
    for (size_t j = 0; j < 33; ++j)
      p[j] = (uint8_t)(j + 1);
    FIO_MEM_REALLOC_ALIGNED(p, 33, 257, 33, 4096);
    FIO_ASSERT(p && !((uintptr_t)p & 4095), "grow/realign failed");
    for (size_t j = 0; j < 33; ++j)
      FIO_ASSERT(p[j] == (uint8_t)(j + 1), "growth lost data");
    /* A valid source copy length may exceed the new destination size. */
    FIO_MEM_REALLOC_ALIGNED(p, 257, 9, 33, 0);
    FIO_ASSERT(p && !((uintptr_t)p & 4095), "shrink lost alignment");
    for (size_t j = 0; j < 9; ++j)
      FIO_ASSERT(p[j] == (uint8_t)(j + 1), "shrink lost data");
    FIO_MEM_REALLOC_ALIGNED(p, 9, 0, 0, 0);
    FIO_ASSERT(!p, "zero-size realloc must free an existing block");
  }
  void *p = NULL;
  FIO_MEM_REALLOC_ALIGNED(p, 0, 0, 0, 0);
  FIO_ASSERT(p, "zero-size initial allocation must be freeable");
  FIO_MEM_FREE_ALIGNED(p, 0);
  p = NULL;
  FIO_MEM_REALLOC_ALIGNED_(p, 0, 64, 0, 0);
  FIO_ASSERT(p && !((uintptr_t)p & (FIO_MEM_ALIGNMENT_SIZE_ - 1)),
             "temporary system allocation default failed");
  FIO_MEM_FREE_ALIGNED_(p, 64);
}

static void test_pointer_tagging(void) {
  FIO_ALIGN(16) static uint8_t object[16];
  void *tagged = (void *)FIO_PTR_TAG(object);
  FIO_ASSERT(FIO_PTR_TAG_VALIDATE(tagged), "valid pointer tag rejected");
  FIO_ASSERT(!FIO_PTR_TAG_VALIDATE(NULL), "NULL must fail validation");
  FIO_ASSERT(!FIO_PTR_TAG_VALIDATE((void *)(uintptr_t)1),
             "tagged NULL must fail validation");
  FIO_ASSERT(!FIO_PTR_TAG_VALIDATE(object), "untagged pointer accepted");
  uint8_t *p = FIO_PTR_TAG_GET_UNTAGGED(uint8_t, tagged);
  FIO_ASSERT(p == object, "untagging must preserve all original pointer bits");
  p[0] = 0xA5;
  FIO_ASSERT(object[0] == 0xA5, "untagged pointer must be usable");
}

int main(void) {
  test_aligned_allocations();
  test_pointer_tagging();
  return 0;
}
