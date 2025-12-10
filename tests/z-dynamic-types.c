/* *****************************************************************************
Test
***************************************************************************** */
#define FIO_TEST_ALL
#include "test-helpers.h"

/* *****************************************************************************
Memory Allocator Tests
***************************************************************************** */

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

/* *****************************************************************************
Dynamically Produced Test Types
***************************************************************************** */

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

/* *****************************************************************************
Locking - Speed Test
***************************************************************************** */
#define FIO___LOCK_TEST_TASK    (1LU << 25)
#define FIO___LOCK_TEST_THREADS 32U
#define FIO___LOCK_TEST_REPEAT  1

FIO_SFUNC void fio___lock_speedtest_task_inner(void *s) {
  size_t *r = (size_t *)s;
  static size_t i;
  for (i = 0; i < FIO___LOCK_TEST_TASK; ++i) {
    FIO_COMPILER_GUARD;
    ++r[0];
  }
}

static void *fio___lock_mytask_lock(void *s) {
  static fio_lock_i lock = FIO_LOCK_INIT;
  fio_lock(&lock);
  if (s)
    fio___lock_speedtest_task_inner(s);
  fio_unlock(&lock);
  return NULL;
}

#ifdef H___FIO_LOCK2___H
static void *fio___lock_mytask_lock2(void *s) {
  static fio_lock2_s lock = {FIO_LOCK_INIT};
  fio_lock2(&lock, 1);
  if (s)
    fio___lock_speedtest_task_inner(s);
  fio_unlock2(&lock, 1);
  return NULL;
}
#endif

static void *fio___lock_mytask_mutex(void *s) {
#if FIO_OS_WIN
  static fio_thread_mutex_t mutex;
#else
  static fio_thread_mutex_t mutex = FIO_THREAD_MUTEX_INIT;
#endif
  fio_thread_mutex_lock(&mutex);
  if (s)
    fio___lock_speedtest_task_inner(s);
  fio_thread_mutex_unlock(&mutex);
  return NULL;
}

FIO_SFUNC void FIO_NAME_TEST(stl, lock_speed)(void) {
  uint64_t start, end;
  fio_thread_t threads[FIO___LOCK_TEST_THREADS];

  struct {
    size_t type_size;
    const char *type_name;
    const char *name;
    void *(*task)(void *);
  } test_funcs[] = {
      {
          .type_size = sizeof(fio_lock_i),
          .type_name = "fio_lock_i",
          .name = "fio_lock      (spinlock)",
          .task = fio___lock_mytask_lock,
      },
      {
          .type_size = sizeof(fio_thread_mutex_t),
          .type_name = "fio_thread_mutex_t",
          .name = "OS threads (pthread_mutex / Windows handle)",
          .task = fio___lock_mytask_mutex,
      },
      {
          .name = NULL,
          .task = NULL,
      },
  };
  fprintf(stderr, "\t* Speed testing The following types:\n");
  for (size_t fn = 0; test_funcs[fn].name; ++fn) {
    fprintf(stderr,
            "\t%s\t(%zu bytes)\n",
            test_funcs[fn].type_name,
            test_funcs[fn].type_size);
  }

  start = fio_time_micro();
  for (size_t i = 0; i < FIO___LOCK_TEST_TASK; ++i) {
    FIO_COMPILER_GUARD;
  }
  end = fio_time_micro();
  fprintf(
      stderr,
      "\n\t* Speed testing locking schemes - no contention, short work (%zu "
      "mms): (%zu iterations)\n",
      (size_t)(end - start),
      (size_t)FIO___LOCK_TEST_TASK);

  for (size_t test_repeat = 0; test_repeat < FIO___LOCK_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK_TEST_REPEAT > 1)
      fprintf(stderr,
              "%s (%zu)\n",
              (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      test_funcs[fn].task(NULL); /* warmup */
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK_TEST_TASK; ++i) {
        FIO_COMPILER_GUARD;
        test_funcs[fn].task(NULL);
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\t%s: %zu mms\n",
              test_funcs[fn].name,
              (size_t)(end - start));
    }
  }

  fprintf(stderr,
          "\n\t* Speed testing locking schemes - no contention, long work ");
  start = fio_time_micro();
  for (size_t i = 0; i < FIO___LOCK_TEST_THREADS; ++i) {
    size_t result = 0;
    FIO_COMPILER_GUARD;
    fio___lock_speedtest_task_inner(&result);
  }
  end = fio_time_micro();
  fprintf(stderr, " %zu mms ", (size_t)(end - start));
  clock_t long_work = end - start;
  fprintf(stderr, "(%zu mms):\n", (size_t)long_work);
  for (size_t test_repeat = 0; test_repeat < FIO___LOCK_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK_TEST_REPEAT > 1)
      fprintf(stderr,
              "\t%s (%zu)\n",
              (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK_TEST_THREADS; ++i) {
        FIO_COMPILER_GUARD;
        test_funcs[fn].task(&result);
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\t%s: %zu mms (%zu mms)\n",
              test_funcs[fn].name,
              (size_t)(end - start),
              (size_t)(end - (start + long_work)));
      FIO_ASSERT(result == (FIO___LOCK_TEST_TASK * FIO___LOCK_TEST_THREADS),
                 "%s final result error.",
                 test_funcs[fn].name);
    }
  }

  fprintf(stderr,
          "\n\t* Speed testing locking schemes - %zu threads, long work (%zu "
          "mms):\n",
          (size_t)FIO___LOCK_TEST_THREADS,
          (size_t)long_work);
  for (size_t test_repeat = 0; test_repeat < FIO___LOCK_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK_TEST_REPEAT > 1)
      fprintf(stderr,
              "%s (%zu)\n",
              (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK_TEST_THREADS; ++i) {
        fio_thread_create(threads + i, test_funcs[fn].task, &result);
      }
      for (size_t i = 0; i < FIO___LOCK_TEST_THREADS; ++i) {
        fio_thread_join(threads + i);
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\t%s: %zu mms (%zu mms)\n",
              test_funcs[fn].name,
              (size_t)(end - start),
              (size_t)(end - (start + long_work)));
      FIO_ASSERT(result == (FIO___LOCK_TEST_TASK * FIO___LOCK_TEST_THREADS),
                 "%s final result error.",
                 test_funcs[fn].name);
    }
  }
}

/* *****************************************************************************
Testing function
***************************************************************************** */

int main(void) {
  char *filename = (char *)FIO_INCLUDE_FILE;
  while (filename[0] == '.' && filename[1] == '/')
    filename += 2;
  FIO_NAME_TEST(stl, ary____test)();
  FIO_NAME_TEST(stl, ary2____test)();
  FIO_NAME_TEST(stl, ary3____test)();
  FIO_NAME_TEST(stl, uset___test_size_t)();
  FIO_NAME_TEST(stl, umap___test_size)();
  FIO_NAME_TEST(stl, omap___test_size_t)();
  FIO_NAME_TEST(stl, omap___test_size_lru)();
  FIO_NAME_TEST(stl, fio_big_str)();
  FIO_NAME_TEST(stl, fio_small_str)();

  /* test memory allocator that initializes memory to zero */
  FIO_NAME_TEST(FIO_NAME(stl, fio_mem_test_safe), mem)();
  /* test memory allocator that allows junk data in allocations */
  FIO_NAME_TEST(FIO_NAME(stl, fio_mem_test_unsafe), mem)();
#if !DEBUG
  FIO_NAME_TEST(stl, lock_speed)();
#endif
  fprintf(stderr,
          "\n\tThe facil.io library (" FIO_VERSION_STRING
          ") was originally coded by \x1B[1mBoaz "
          "Segev\x1B[0m.\n"
          "\t\x1B[1mValue deserves to be valued.\x1B[0m\n"
          "\t(please consider code contributions / donations)\n\n");
}
