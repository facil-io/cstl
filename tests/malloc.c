/* *****************************************************************************
Test - Malloc Module
Covers 010 mem.h allocator behavior. The old ./tests-old/malloc.c file is a
performance harness, so correctness coverage here stays small and deterministic.
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

int main(void) {
  fio___test_malloc_basic();
  fio___test_malloc_calloc_realloc();
  return 0;
}
