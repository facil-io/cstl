/* *****************************************************************************






                            Start Test Code






***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_TESTS_START___H)
#define H___FIO_TESTS_START___H
/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************
C++ extern start
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
extern "C" {
#endif

FIO_SFUNC void fio_test_dynamic_types(void);

FIO_SFUNC uintptr_t fio___dynamic_types_test_tag(uintptr_t i) { return i | 1; }
FIO_SFUNC uintptr_t fio___dynamic_types_test_untag(uintptr_t i) {
  return i & (~((uintptr_t)1UL));
}

#define FIO_TEST_REPEAT (1ULL << 12U)

/* *****************************************************************************
Memory Allocator Tests
***************************************************************************** */
#define FIO___TEST_REINCLUDE

#define FIO_MEMORY_NAME                   fio_mem_test_safe
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 1
#undef FIO_MEMORY_USE_THREAD_MUTEX
#define FIO_MEMORY_USE_THREAD_MUTEX 0
#define FIO_MEMORY_ARENA_COUNT      4
#include FIO_INCLUDE_FILE

#define FIO_MEMORY_NAME                   fio_mem_test_unsafe
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 0
#undef FIO_MEMORY_USE_THREAD_MUTEX
#define FIO_MEMORY_USE_THREAD_MUTEX 0
#define FIO_MEMORY_ARENA_COUNT      4
#include FIO_INCLUDE_FILE

#undef FIO___TEST_REINCLUDE
/* *****************************************************************************
Dynamically Produced Test Types
***************************************************************************** */
#define FIO___TEST_REINCLUDE

static int ary____test_was_destroyed = 0;
#define FIO_ARRAY_NAME    ary____test
#define FIO_ARRAY_TYPE    int
#define FIO_REF_NAME      ary____test
#define FIO_REF_INIT(obj) obj = (ary____test_s)FIO_ARRAY_INIT
#define FIO_REF_DESTROY(obj)                                                   \
  do {                                                                         \
    ary____test_destroy(&obj);                                                 \
    ary____test_was_destroyed = 1;                                             \
  } while (0)
#define FIO_PTR_TAG(p)   fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p) fio___dynamic_types_test_untag(((uintptr_t)p))
#include FIO_INCLUDE_FILE

#define FIO_ARRAY_NAME                 ary2____test
#define FIO_ARRAY_TYPE                 uint8_t
#define FIO_ARRAY_TYPE_INVALID         0xFF
#define FIO_ARRAY_TYPE_COPY(dest, src) (dest) = (src)
#define FIO_ARRAY_TYPE_DESTROY(obj)    (obj = FIO_ARRAY_TYPE_INVALID)
#define FIO_ARRAY_TYPE_CMP(a, b)       (a) == (b)
#define FIO_PTR_TAG(p)                 fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p)               fio___dynamic_types_test_untag(((uintptr_t)p))
#include FIO_INCLUDE_FILE

/* test all defaults */
#define FIO_ARRAY_NAME ary3____test
#include FIO_INCLUDE_FILE

#define FIO_UMAP_NAME   uset___test_size_t
#define FIO_MEMORY_NAME uset___test_size_t_mem
#define FIO_MAP_KEY     size_t
#define FIO_MAP_TEST
#include FIO_INCLUDE_FILE
#define FIO_UMAP_NAME   umap___test_size
#define FIO_MEMORY_NAME umap___test_size_mem
#define FIO_MAP_KEY     size_t
#define FIO_MAP_VALUE   size_t
#define FIO_MAP_TEST
#include FIO_INCLUDE_FILE
#define FIO_OMAP_NAME   omap___test_size_t
#define FIO_MEMORY_NAME omap___test_size_t_mem
#define FIO_MAP_KEY     size_t
#define FIO_MAP_ORDERED 1
#define FIO_MAP_TEST
#include FIO_INCLUDE_FILE
#define FIO_OMAP_NAME   omap___test_size_lru
#define FIO_MEMORY_NAME omap___test_size_lru_mem
#define FIO_MAP_KEY     size_t
#define FIO_MAP_VALUE   size_t
#define FIO_MAP_LRU     (1UL << 24)
#define FIO_MAP_TEST
#include FIO_INCLUDE_FILE

#define FIO_STR_NAME fio_big_str
#define FIO_STR_WRITE_TEST_FUNC
#include FIO_INCLUDE_FILE

#define FIO_STR_SMALL fio_small_str
#define FIO_STR_WRITE_TEST_FUNC
#include FIO_INCLUDE_FILE

#undef FIO___TEST_REINCLUDE
/* *****************************************************************************
Environment printout
***************************************************************************** */
#ifndef FIO_PRINT_SIZE_OF
#define FIO_PRINT_SIZE_OF(T)                                                   \
  fprintf(stderr, "\t%-19s%zu Bytes\n", #T, sizeof(T))
#endif

FIO_SFUNC void FIO_NAME_TEST(stl, type_sizes)(void) {
  switch (sizeof(void *)) {
  case 2:
    fprintf(stderr, "* 16bit words size (unexpected, unknown effects).\n");
    break;
  case 4:
    fprintf(stderr, "* 32bit words size (some features might be slower).\n");
    break;
  case 8: fprintf(stderr, "* 64bit words size okay.\n"); break;
  case 16: fprintf(stderr, "* 128bit words size... wow!\n"); break;
  default:
    fprintf(stderr, "* Unknown words size %zubit!\n", sizeof(void *) << 3);
    break;
  }
  fprintf(stderr, "* Using the following type sizes:\n");
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
    fprintf(stderr, "\t%-17s%ld bytes.\n", "Page", page);
    if (page != (1UL << FIO_MEM_PAGE_SIZE_LOG))
      FIO_LOG_INFO("unexpected page size != 4096\n          "
                   "facil.io could be recompiled with:\n          "
                   "`CFLAGS=\"-DFIO_MEM_PAGE_SIZE_LOG=%.0lf\"`",
                   log2(page));
  }
#endif /* FIO_OS_POSIX */
}
/* *****************************************************************************

***************************************************************************** */
#endif /* H___FIO_TESTS_START___H */
