/* *****************************************************************************
Test - Malloc Module
Covers 010 mem.h allocator behavior. The old ./tests-old/malloc.c file is a
performance harness, so correctness coverage here stays small and deterministic.

This file is compiled twice:
- as-is (custom fio allocator), and
- via tests/malloc_noalloc.c (FIO_MEMORY_DISABLE, system allocator backends).

NOTE: the underscore (template-local) macros are undefined after the final
include cycle, so direct fio_* aligned allocations are released with the
config-correct pairing (TEST_MEM_FREE_ALIGNED below); the global
FIO_MEM_REALLOC_ALIGNED / FIO_MEM_FREE_ALIGNED pair is tested as a
self-consistent unit.
***************************************************************************** */
#define FIO_CORE
#define FIO_THREADS
#include "fio-stl/include.h"

#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 0
#define FIO_MALLOC
#include FIO_INCLUDE_FILE

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* The free that pairs with fio_realloc_aligned in this build configuration. */
#if defined(FIO_MEMORY_DISABLE)
#define TEST_MEM_FREE_ALIGNED(p, sz) FIO_MEM_FREE_ALIGNED((p), (sz))
#else
#define TEST_MEM_FREE_ALIGNED(p, sz) fio_free((p))
#endif

FIO_SFUNC void fio___test_malloc_basic(void) {
  const size_t alignment = fio_malloc_alignment();
  FIO_ASSERT(alignment && !(alignment & (alignment - 1)),
             "fio_malloc_alignment should be a non-zero power of two");
  for (size_t len = 1; len <= 4096; len <<= 1) {
    uint8_t *p = (uint8_t *)fio_malloc(len);
    FIO_ASSERT(p, "fio_malloc failed for %zu bytes", len);
    FIO_ASSERT(((uintptr_t)p & (alignment - 1)) == 0,
               "fio_malloc returned misaligned pointer for %zu bytes",
               len);
    memset(p, 0xA5, len);
    for (size_t i = 0; i < len; ++i)
      FIO_ASSERT(p[i] == 0xA5, "allocated memory write/read failed");
    fio_free(p);
  }
}

FIO_SFUNC void fio___test_malloc_calloc_realloc(void) {
  uint8_t *p = (uint8_t *)fio_calloc(32, 4);
  FIO_ASSERT(p, "fio_calloc failed");
  for (size_t i = 0; i < 128; ++i)
    FIO_ASSERT(p[i] == 0, "fio_calloc should zero initialize memory");
  for (size_t i = 0; i < 128; ++i)
    p[i] = (uint8_t)i;

  uint8_t *grown = (uint8_t *)fio_realloc2(p, 256, 128);
  FIO_ASSERT(grown, "fio_realloc2 grow failed");
  for (size_t i = 0; i < 128; ++i)
    FIO_ASSERT(grown[i] == (uint8_t)i, "fio_realloc2 did not preserve data");

  uint8_t *shrunk = (uint8_t *)fio_realloc2(grown, 64, 64);
  FIO_ASSERT(shrunk, "fio_realloc2 shrink failed");
  for (size_t i = 0; i < 64; ++i)
    FIO_ASSERT(shrunk[i] == (uint8_t)i, "fio_realloc2 shrink lost data");
  fio_free(shrunk);
  fio_free(NULL);
}

/* *****************************************************************************
Aligned Allocation API (fio_* functions)
***************************************************************************** */

FIO_SFUNC void fio___test_malloc_aligned_basic(void) {
  static const size_t alignments[] = {16, 32, 64, 128, 256, 4096};
  static const size_t sizes[] = {1,
                                 63,
                                 64,
                                 100,
                                 4096,
                                 65536,
                                 (size_t)1 << 20,          /* big-block path */
                                 ((size_t)3 << 20) + 777}; /* page-backed path */
  for (size_t ai = 0; ai < (sizeof(alignments) / sizeof(*alignments)); ++ai) {
    const size_t a = alignments[ai];
    for (size_t si = 0; si < (sizeof(sizes) / sizeof(*sizes)); ++si) {
      const size_t sz = sizes[si];
      uint8_t *p = (uint8_t *)fio_malloc_aligned(sz, a);
      FIO_ASSERT(p,
                 "fio_malloc_aligned failed for %zu bytes (alignment %zu)",
                 sz,
                 a);
      FIO_ASSERT(!((uintptr_t)p & (a - 1)),
                 "fio_malloc_aligned misaligned: %p (alignment %zu, size %zu)",
                 (void *)p,
                 a,
                 sz);
      memset(p, 0xA5, sz);
      FIO_ASSERT(p[0] == 0xA5 && p[sz - 1] == 0xA5 && p[sz >> 1] == 0xA5,
                 "aligned allocation write/read failed (size %zu)",
                 sz);
      TEST_MEM_FREE_ALIGNED(p, sz);
    }
  }
}

FIO_SFUNC void fio___test_malloc_aligned_realloc(void) {
  const size_t len = 100;
  uint8_t *p = (uint8_t *)fio_malloc_aligned(len, 64);
  FIO_ASSERT(p, "fio_malloc_aligned failed");
  for (size_t i = 0; i < len; ++i)
    p[i] = (uint8_t)(i * 7 + 1);

  /* grow + realign upwards (64 -> 4096): content must be preserved */
  p = (uint8_t *)fio_realloc_aligned(p, 300000, len, 4096);
  FIO_ASSERT(p, "realloc_aligned grow+realign failed");
  FIO_ASSERT(!((uintptr_t)p & 4095), "realloc_aligned did not realign: %p",
             (void *)p);
  for (size_t i = 0; i < len; ++i)
    FIO_ASSERT(p[i] == (uint8_t)(i * 7 + 1),
               "content lost across realignment (byte %zu)",
               i);

  /* shrink + lower requested alignment: effective alignment never regresses
   * (the pointer's own alignment is a floor), content is preserved */
  p = (uint8_t *)fio_realloc_aligned(p, 64, 64, 16);
  FIO_ASSERT(p, "realloc_aligned shrink failed");
  FIO_ASSERT(!((uintptr_t)p & 15), "alignment below requested: %p", (void *)p);
  FIO_ASSERT(!((uintptr_t)p & 4095), "pointer alignment regressed: %p",
             (void *)p);
  for (size_t i = 0; i < 64; ++i)
    FIO_ASSERT(p[i] == (uint8_t)(i * 7 + 1),
               "content lost across shrink (byte %zu)",
               i);
  TEST_MEM_FREE_ALIGNED(p, 64);

  /* page-backed (mmap) growth keeps its alignment */
  const size_t big = (size_t)1 << 21; /* beyond any arena/big-block limit */
  p = (uint8_t *)fio_malloc_aligned(big, 4096);
  FIO_ASSERT(p && !((uintptr_t)p & 4095), "page-backed aligned alloc failed");
  p[0] = 0x11;
  p[big - 1] = 0x22;
  p = (uint8_t *)fio_realloc_aligned(p, big << 1, big, 4096);
  FIO_ASSERT(p && !((uintptr_t)p & 4095), "page-backed aligned realloc failed");
  FIO_ASSERT(p[0] == 0x11 && p[big - 1] == 0x22,
             "page-backed content lost across realloc");
  TEST_MEM_FREE_ALIGNED(p, big << 1);

  /* zero-size allocations are freeable and realloc-able */
  p = (uint8_t *)fio_malloc_aligned(0, 0); /* default alignment */
  FIO_ASSERT(p, "zero-size aligned allocation should return a pointer");
  FIO_ASSERT(!((uintptr_t)p & (fio_malloc_alignment() - 1)),
             "zero-size pointer below default alignment");
  p = (uint8_t *)fio_realloc_aligned(p, 100, 0, 128);
  FIO_ASSERT(p && !((uintptr_t)p & 127), "realloc of zero-size failed");
  TEST_MEM_FREE_ALIGNED(p, 100);
}

FIO_SFUNC void fio___test_malloc_calloc_aligned(void) {
  uint8_t *p = (uint8_t *)fio_calloc_aligned(3, 37, 128);
  FIO_ASSERT(p && !((uintptr_t)p & 127),
             "fio_calloc_aligned failed/misaligned");
  for (size_t i = 0; i < 111; ++i)
    FIO_ASSERT(!p[i], "fio_calloc_aligned memory not zeroed (byte %zu)", i);
  TEST_MEM_FREE_ALIGNED(p, 111);

  /* size overflow must fail */
  p = (uint8_t *)fio_calloc_aligned((size_t)-1, 2, 64);
  FIO_ASSERT(!p, "fio_calloc_aligned overflow not caught");
}

FIO_SFUNC void fio___test_malloc_alloc_size(void) {
  FIO_ASSERT(!fio_alloc_size(0), "alloc_size(0) should be zero");
  FIO_ASSERT(FIO_MEM_ALLOC_SIZE(100) >= 100,
             "FIO_MEM_ALLOC_SIZE must never under-report");
#if defined(FIO_MEMORY_DISABLE)
  FIO_ASSERT(fio_alloc_size(100) == 100,
             "alloc_size must be an identity function with FIO_MEMORY_DISABLE");
#else
  const size_t al = fio_malloc_alignment();
  const size_t limit = fio_malloc_alloc_limit();
  FIO_ASSERT(fio_alloc_size(1) == al, "alloc_size(1) should round up (%zu)", al);
  FIO_ASSERT(fio_alloc_size(al) == al, "alloc_size rounding error");
  FIO_ASSERT(fio_alloc_size(al + 1) == (al << 1), "alloc_size rounding error");
  FIO_ASSERT(fio_alloc_size(limit) == limit, "alloc_size at limit error");
  {
    /* page-backed allocations: page-rounded, minus the header offset */
    const size_t page = (size_t)1 << FIO_MEM_PAGE_SIZE_LOG;
    const size_t req = limit + 1;
    const size_t expect = (((req + al + page - 1) & ~(page - 1)) - al);
    FIO_ASSERT(fio_alloc_size(req) == expect,
               "alloc_size(page-backed) error: %zu != %zu",
               fio_alloc_size(req),
               expect);
  }
  /* alloc_size must never under-report: we may write that many bytes */
  {
    const size_t req = 100;
    const size_t usable = fio_alloc_size(req);
    uint8_t *p = (uint8_t *)fio_malloc(req);
    FIO_ASSERT(p && usable >= req, "alloc_size under-reports");
    memset(p, 0x5A, usable); /* must be safe */
    fio_free(p);
  }
#endif
}

/* *****************************************************************************
Global macros (FIO_MEM_REALLOC_ALIGNED / FIO_MEM_FREE_ALIGNED pair)
***************************************************************************** */

FIO_SFUNC void fio___test_malloc_aligned_macros(void) {
  /* the global macros are a self-consistent pair in every inclusion pattern */
  void *p = NULL;
  FIO_MEM_REALLOC_ALIGNED(p, 0, 256, 0, 128);
  FIO_ASSERT(p, "FIO_MEM_REALLOC_ALIGNED failed");
  FIO_ASSERT(!((uintptr_t)p & 127), "FIO_MEM_REALLOC_ALIGNED misaligned: %p",
             (void *)p);
  memset(p, 0xC3, 256);
  FIO_MEM_REALLOC_ALIGNED(p, 256, 1024, 256, 4096);
  FIO_ASSERT(p && !((uintptr_t)p & 4095),
             "FIO_MEM_REALLOC_ALIGNED grow+realign failed: %p",
             (void *)p);
  FIO_ASSERT(((uint8_t *)p)[0] == 0xC3 && ((uint8_t *)p)[255] == 0xC3,
             "FIO_MEM_REALLOC_ALIGNED lost content");
  FIO_MEM_FREE_ALIGNED(p, 1024);
  p = NULL;
  FIO_MEM_REALLOC_ALIGNED(p, 0, 64, 0, 0); /* alignment 0 == default */
  FIO_ASSERT(p &&
                 !((uintptr_t)p & (FIO_MEM_ALIGNMENT_SIZE - 1)),
             "default alignment failed");
  FIO_MEM_FREE_ALIGNED(p, 64);
}

/* *****************************************************************************
Over-aligned reference-counted type (249 reference counter.h)
***************************************************************************** */

typedef struct {
  uint64_t u64[4];
} FIO_ALIGN(128) fio___test_aligned128_s;

#define FIO_REF_NAME fio___test_refaligned
#define FIO_REF_TYPE fio___test_aligned128_s
#include FIO_INCLUDE_FILE

FIO_SFUNC void fio___test_malloc_refcounted_aligned(void) {
  fio___test_aligned128_s *o = fio___test_refaligned_new2();
  FIO_ASSERT(o, "ref-counted over-aligned allocation failed");
  FIO_ASSERT(!((uintptr_t)o & (_Alignof(fio___test_aligned128_s) - 1)),
             "ref-counted object misaligned: %p (needs %zu)",
             (void *)o,
             (size_t)_Alignof(fio___test_aligned128_s));
  FIO_ASSERT(fio___test_refaligned_references(o) == 1, "refcount should be 1");
  fio___test_aligned128_s *o2 = fio___test_refaligned_dup2(o);
  FIO_ASSERT(o2 == o, "dup should return the same pointer");
  FIO_ASSERT(fio___test_refaligned_references(o) == 2, "refcount should be 2");
  fio___test_refaligned_free2(o);
  FIO_ASSERT(fio___test_refaligned_references(o2) == 1,
             "refcount should drop to 1");
  fio___test_refaligned_free2(o2);
}

/* *****************************************************************************
Invalid alignment requests
***************************************************************************** */

FIO_SFUNC void fio___test_malloc_aligned_invalid(void) {
#if !defined(DEBUG) || !DEBUG
  /* above the supported maximum: fails with EINVAL (NULL result) */
  const size_t too_much = fio_malloc_sys_alloc_size() << 1;
  errno = 0;
  void *bad = fio_realloc_aligned(NULL, 64, 0, too_much);
  FIO_ASSERT(!bad, "out-of-range alignment should fail");
  FIO_ASSERT(errno == EINVAL, "errno should be EINVAL (got %d)", (int)errno);
  /* non power-of-2 requests are rounded DOWN, not rejected */
  uint8_t *p = (uint8_t *)fio_malloc_aligned(64, 48); /* rounds down to 32 */
  FIO_ASSERT(p && !((uintptr_t)p & 31), "pow2-floor alignment failed");
  TEST_MEM_FREE_ALIGNED(p, 64);
#endif
}

int main(void) {
  fio___test_malloc_basic();
  fio___test_malloc_calloc_realloc();
  fio___test_malloc_aligned_basic();
  fio___test_malloc_aligned_realloc();
  fio___test_malloc_calloc_aligned();
  fio___test_malloc_alloc_size();
  fio___test_malloc_aligned_macros();
  fio___test_malloc_refcounted_aligned();
  fio___test_malloc_aligned_invalid();
  return 0;
}
