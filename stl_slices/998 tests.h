/* ************************************************************************* */
#ifndef H___FIO_CSTL_INCLUDE_ONCE___H /* Development inclusion - ignore line*/
#include "000 header.h"               /* Development inclusion - ignore line */
#endif                                /* Development inclusion - ignore line */
/* *****************************************************************************



                                Testing


Copyright and License: see header file (000 header.h) or top of file
***************************************************************************** */

#if !defined(FIO_FIO_TEST_CSTL_ONLY_ONCE) && (defined(FIO_TEST_CSTL))
#define FIO_FIO_TEST_CSTL_ONLY_ONCE 1

#ifdef FIO_EXTERN_TEST
void fio_test_dynamic_types(void);
#else
FIO_SFUNC void fio_test_dynamic_types(void);
#endif

#if !defined(FIO_EXTERN_TEST) || defined(FIO_EXTERN_COMPLETE)

/* Make sure logging and memory leak counters are set. */
#define FIO_LOG
#ifndef FIO_LEAK_COUNTER
#define FIO_LEAK_COUNTER 1
#endif
#ifndef FIO_FIOBJ
#define FIO_FIOBJ
#endif
#ifndef FIOBJ_MALLOC
#define FIOBJ_MALLOC /* define to test with custom allocator */
#endif
#define FIO_TIME
#include __FILE__

/* Add non-type options to minimize `#include` instructions */
#define FIO_ATOL
#define FIO_ATOMIC
#define FIO_BITMAP
#define FIO_BITWISE
#define FIO_CHACHA
#define FIO_CLI
#define FIO_GLOB_MATCH
#define FIO_MATH
#define FIO_POLL
#define FIO_QUEUE
#define FIO_RAND
#define FIO_RISKY_HASH
#define FIO_SHA1
#define FIO_SIGNAL
#define FIO_SOCK
#define FIO_STATE
#define FIO_STREAM
#define FIO_THREADS
#define FIO_TIME
#define FIO_URL
#define FIO_SERVER
#define FIO_SORT_NAME num
#define FIO_SORT_TYPE size_t
#define FIO_SORT_TEST 1
// #define FIO_LOCK2 /* a signal based blocking lock is WIP */
#include __FILE__

FIO_SFUNC uintptr_t fio___dynamic_types_test_tag(uintptr_t i) { return i | 1; }
FIO_SFUNC uintptr_t fio___dynamic_types_test_untag(uintptr_t i) {
  return i & (~((uintptr_t)1UL));
}

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
#include __FILE__

#define FIO_ARRAY_NAME                 ary2____test
#define FIO_ARRAY_TYPE                 uint8_t
#define FIO_ARRAY_TYPE_INVALID         0xFF
#define FIO_ARRAY_TYPE_COPY(dest, src) (dest) = (src)
#define FIO_ARRAY_TYPE_DESTROY(obj)    (obj = FIO_ARRAY_TYPE_INVALID)
#define FIO_ARRAY_TYPE_CMP(a, b)       (a) == (b)
#define FIO_PTR_TAG(p)                 fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p)               fio___dynamic_types_test_untag(((uintptr_t)p))
#include __FILE__

/* test all defaults */
#define FIO_ARRAY_NAME ary3____test
#include __FILE__

#define FIO_UMAP_NAME   uset___test_size_t
#define FIO_MEMORY_NAME uset___test_size_t_mem
#define FIO_MAP_KEY     size_t
#define FIO_MAP_TEST
#include __FILE__
#define FIO_UMAP_NAME   umap___test_size
#define FIO_MEMORY_NAME umap___test_size_mem
#define FIO_MAP_KEY     size_t
#define FIO_MAP_VALUE   size_t
#define FIO_MAP_TEST
#include __FILE__
#define FIO_OMAP_NAME   omap___test_size_t
#define FIO_MEMORY_NAME omap___test_size_t_mem
#define FIO_MAP_KEY     size_t
#define FIO_MAP_ORDERED 1
#define FIO_MAP_TEST
#include __FILE__
#define FIO_OMAP_NAME   omap___test_size_lru
#define FIO_MEMORY_NAME omap___test_size_lru_mem
#define FIO_MAP_KEY     size_t
#define FIO_MAP_VALUE   size_t
#define FIO_MAP_LRU     1
#define FIO_MAP_TEST
#include __FILE__

#define FIO_STR_NAME fio_big_str
#define FIO_STR_WRITE_TEST_FUNC
#include __FILE__

#define FIO_STR_SMALL fio_small_str
#define FIO_STR_WRITE_TEST_FUNC
#include __FILE__

#define FIO_MEMORY_NAME                   fio_mem_test_safe
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 1
#undef FIO_MEMORY_USE_THREAD_MUTEX
#define FIO_MEMORY_USE_THREAD_MUTEX 0
#define FIO_MEMORY_ARENA_COUNT      4
#include __FILE__

#define FIO_MEMORY_NAME                   fio_mem_test_unsafe
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 0
#undef FIO_MEMORY_USE_THREAD_MUTEX
#define FIO_MEMORY_USE_THREAD_MUTEX 0
#define FIO_MEMORY_ARENA_COUNT      4
#include __FILE__

#define FIO_FIOBJ
#include __FILE__

/* *****************************************************************************
Linked List - Test
***************************************************************************** */

typedef struct {
  int data;
  FIO_LIST_NODE node;
} ls____test_s;

#define FIO_LIST_NAME    ls____test
#define FIO_PTR_TAG(p)   fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p) fio___dynamic_types_test_untag(((uintptr_t)p))

#include __FILE__

FIO_SFUNC void fio___dynamic_types_test___linked_list_test(void) {
  fprintf(stderr, "* Testing linked lists.\n");
  FIO_LIST_HEAD ls = FIO_LIST_INIT(ls);
  for (int i = 0; i < FIO_TEST_REPEAT; ++i) {
    ls____test_s *node = ls____test_push(
        &ls,
        (ls____test_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*node), 0));
    node->data = i;
  }
  int tester = 0;
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    FIO_ASSERT(pos->data == tester++,
               "Linked list ordering error for push or each");
    FIO_ASSERT(ls____test_root(&pos->node) == pos,
               "Linked List root offset error");
  }
  FIO_ASSERT(tester == FIO_TEST_REPEAT,
             "linked list EACH didn't loop through all the list");
  while (ls____test_any(&ls)) {
    ls____test_s *node = ls____test_pop(&ls);
    node = (ls____test_s *)fio___dynamic_types_test_untag((uintptr_t)(node));
    FIO_ASSERT(node, "Linked list pop or any failed");
    FIO_ASSERT(node->data == --tester, "Linked list ordering error for pop");
    FIO_MEM_FREE(node, sizeof(*node));
  }
  tester = FIO_TEST_REPEAT;
  for (int i = 0; i < FIO_TEST_REPEAT; ++i) {
    ls____test_s *node = ls____test_unshift(
        &ls,
        (ls____test_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*node), 0));
    node->data = i;
  }
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    FIO_ASSERT(pos->data == --tester,
               "Linked list ordering error for unshift or each");
  }
  FIO_ASSERT(tester == 0,
             "linked list EACH didn't loop through all the list after unshift");
  tester = FIO_TEST_REPEAT;
  while (ls____test_any(&ls)) {
    ls____test_s *node = ls____test_shift(&ls);
    node = (ls____test_s *)fio___dynamic_types_test_untag((uintptr_t)(node));
    FIO_ASSERT(node, "Linked list pop or any failed");
    FIO_ASSERT(node->data == --tester, "Linked list ordering error for shift");
    FIO_MEM_FREE(node, sizeof(*node));
  }
  FIO_ASSERT(FIO_NAME_BL(ls____test, empty)(&ls),
             "Linked list empty should have been true");
  for (int i = 0; i < FIO_TEST_REPEAT; ++i) {
    ls____test_s *node = ls____test_push(
        &ls,
        (ls____test_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*node), 0));
    node->data = i;
  }
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    ls____test_remove(pos);
    pos = (ls____test_s *)fio___dynamic_types_test_untag((uintptr_t)(pos));
    FIO_MEM_FREE(pos, sizeof(*pos));
  }
  FIO_ASSERT(FIO_NAME_BL(ls____test, empty)(&ls),
             "Linked list empty should have been true");
}

FIO_SFUNC void fio___dynamic_types_test___index_list_test(void) {
  fprintf(stderr, "* Testing indexed lists.\n");
  struct {
    size_t i;
    struct {
      uint16_t next;
      uint16_t prev;
    } node;
  } data[16];
  size_t count;
  const size_t len = 16;
  for (size_t i = 0; i < len; ++i) {
    data[i].i = i;
    if (!i)
      data[i].node.prev = data[i].node.next = i;
    else
      FIO_INDEXED_LIST_PUSH(data, node, 0, i);
  }
  count = 0;
  FIO_INDEXED_LIST_EACH(data, node, 0, i) {
    FIO_ASSERT(data[i].i == count,
               "indexed list order issue? %zu != %zu",
               data[i].i != i);
    ++count;
  }
  FIO_ASSERT(count == 16, "indexed list each failed? (%zu != %zu)", count, len);
  count = 0;
  while (data[0].node.next != 0 && count < 32) {
    ++count;
    uint16_t n = data[0].node.prev;
    FIO_INDEXED_LIST_REMOVE(data, node, n);
  }
  FIO_ASSERT(count == 15,
             "indexed list remove failed? (%zu != %zu)",
             count,
             len);
  for (size_t i = 0; i < len; ++i) {
    data[i].i = i;
    if (!i)
      data[i].node.prev = data[i].node.next = i;
    else {
      FIO_INDEXED_LIST_PUSH(data, node, 0, i);
      FIO_INDEXED_LIST_REMOVE(data, node, i);
      FIO_INDEXED_LIST_PUSH(data, node, 0, i);
    }
  }
  count = 0;
  FIO_INDEXED_LIST_EACH(data, node, 0, i) {
    FIO_ASSERT(data[i].i == count,
               "indexed list order issue (push-pop-push? %zu != %zu",
               data[i].i != count);
    ++count;
  }
}

/* *****************************************************************************
Environment printout
***************************************************************************** */

#define FIO_PRINT_SIZE_OF(T)                                                   \
  fprintf(stderr, "\t%-19s%zu Bytes\n", #T, sizeof(T))

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
#undef FIO_PRINT_SIZE_OF

/* *****************************************************************************
Locking - Speed Test
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, math_speed)(void) {
  uint64_t n = 0, d = 1;
  uint64_t start[2], end[2];
  start[0] = fio_time_nano();
  for (size_t i = 0; i < 64; ++i) {
    n = (n << 7) ^ 0xAA;
    uint64_t q = 0, r = 0;
    FIO_COMPILER_GUARD;
    for (size_t j = 0; j < 64; ++j) {
      d = (d << 3) ^ 0xAA;
      FIO_COMPILER_GUARD;
      fio_math_div(&q, &r, &n, &d, 1);
      FIO_COMPILER_GUARD;
    }
    (void)q;
  }
  end[0] = fio_time_nano();
  n = 0, d = 1;
  start[1] = fio_time_nano();
  for (size_t i = 0; i < 64; ++i) {
    n = (n << 7) ^ 0xAA;
    uint64_t q = 0;
    FIO_COMPILER_GUARD;
    for (size_t j = 0; j < 64; ++j) {
      d = (d << 3) ^ 0xAA;
      FIO_COMPILER_GUARD;
      q = n / d;
      FIO_COMPILER_GUARD;
    }
    (void)q;
  }
  end[1] = fio_time_nano();
  FIO_LOG_INFO("\t fio_math_div test took %zu us (vs. %zu us) for a single "
               "64 bit word.",
               (size_t)(end[0] - start[0]),
               (size_t)(end[1] - start[1]));
}

/* *****************************************************************************
Locking - Speed Test
***************************************************************************** */
#define FIO___LOCK2_TEST_TASK    (1LU << 25)
#define FIO___LOCK2_TEST_THREADS 32U
#define FIO___LOCK2_TEST_REPEAT  1

FIO_SFUNC void fio___lock_speedtest_task_inner(void *s) {
  size_t *r = (size_t *)s;
  static size_t i;
  for (i = 0; i < FIO___LOCK2_TEST_TASK; ++i) {
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
  static fio_thread_mutex_t mutex = FIO_THREAD_MUTEX_INIT;
  fio_thread_mutex_lock(&mutex);
  if (s)
    fio___lock_speedtest_task_inner(s);
  fio_thread_mutex_unlock(&mutex);
  return NULL;
}

FIO_SFUNC void FIO_NAME_TEST(stl, lock_speed)(void) {
  uint64_t start, end;
  fio_thread_t threads[FIO___LOCK2_TEST_THREADS];

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
#ifdef H___FIO_LOCK2___H
      {
          .type_size = sizeof(fio_lock2_s),
          .type_name = "fio_lock2_s",
          .name = "fio_lock2 (pause/resume)",
          .task = fio___lock_mytask_lock2,
      },
#endif
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
  fprintf(stderr, "* Speed testing The following types:\n");
  for (size_t fn = 0; test_funcs[fn].name; ++fn) {
    fprintf(stderr,
            "\t%s\t(%zu bytes)\n",
            test_funcs[fn].type_name,
            test_funcs[fn].type_size);
  }
#ifndef H___FIO_LOCK2___H
  FIO_LOG_WARNING("Won't test `fio_lock2` functions (needs `FIO_LOCK2`).");
#endif

  start = fio_time_micro();
  for (size_t i = 0; i < FIO___LOCK2_TEST_TASK; ++i) {
    FIO_COMPILER_GUARD;
  }
  end = fio_time_micro();
  fprintf(stderr,
          "\n* Speed testing locking schemes - no contention, short work (%zu "
          "mms):\n"
          "\t\t(%zu itterations)\n",
          (size_t)(end - start),
          (size_t)FIO___LOCK2_TEST_TASK);

  for (int test_repeat = 0; test_repeat < FIO___LOCK2_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK2_TEST_REPEAT > 1)
      fprintf(stderr,
              "%s (%d)\n",
              (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      test_funcs[fn].task(NULL); /* warmup */
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK2_TEST_TASK; ++i) {
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
          "\n* Speed testing locking schemes - no contention, long work ");
  start = fio_time_micro();
  for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
    size_t result = 0;
    FIO_COMPILER_GUARD;
    fio___lock_speedtest_task_inner(&result);
  }
  end = fio_time_micro();
  fprintf(stderr, " %zu mms\n", (size_t)(end - start));
  clock_t long_work = end - start;
  fprintf(stderr, "(%zu mms):\n", (size_t)long_work);
  for (int test_repeat = 0; test_repeat < FIO___LOCK2_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK2_TEST_REPEAT > 1)
      fprintf(stderr,
              "%s (%d)\n",
              (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
        FIO_COMPILER_GUARD;
        test_funcs[fn].task(&result);
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\t%s: %zu mms (%zu mms)\n",
              test_funcs[fn].name,
              (size_t)(end - start),
              (size_t)(end - (start + long_work)));
      FIO_ASSERT(result == (FIO___LOCK2_TEST_TASK * FIO___LOCK2_TEST_THREADS),
                 "%s final result error.",
                 test_funcs[fn].name);
    }
  }

  fprintf(stderr,
          "\n* Speed testing locking schemes - %zu threads, long work (%zu "
          "mms):\n",
          (size_t)FIO___LOCK2_TEST_THREADS,
          (size_t)long_work);
  for (int test_repeat = 0; test_repeat < FIO___LOCK2_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK2_TEST_REPEAT > 1)
      fprintf(stderr,
              "%s (%d)\n",
              (test_repeat ? "Round" : "Warmup"),
              test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
        fio_thread_create(threads + i, test_funcs[fn].task, &result);
      }
      for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
        fio_thread_join(threads + i);
      }
      end = fio_time_micro();
      fprintf(stderr,
              "\t%s: %zu mms (%zu mms)\n",
              test_funcs[fn].name,
              (size_t)(end - start),
              (size_t)(end - (start + long_work)));
      FIO_ASSERT(result == (FIO___LOCK2_TEST_TASK * FIO___LOCK2_TEST_THREADS),
                 "%s final result error.",
                 test_funcs[fn].name);
    }
  }
}

/* *****************************************************************************
Testing function
***************************************************************************** */

FIO_SFUNC void fio____test_dynamic_types__stack_poisoner(void) {
#define FIO___STACK_POISON_LENGTH (1ULL << 16)
  uint8_t buf[FIO___STACK_POISON_LENGTH];
  FIO_COMPILER_GUARD;
  FIO_MEMSET(buf, (int)(~0U), FIO___STACK_POISON_LENGTH);
  FIO_COMPILER_GUARD;
  fio_trylock(buf);
#undef FIO___STACK_POISON_LENGTH
}

void fio_test_dynamic_types(void) {
  char *filename = (char *)FIO__FILE__;
  while (filename[0] == '.' && filename[1] == '/')
    filename += 2;
  fio____test_dynamic_types__stack_poisoner();
  fprintf(stderr, "===============\n");
  fprintf(stderr, "Testing Dynamic Types (%s)\n", filename);
  fprintf(
      stderr,
      "facil.io core: version \x1B[1m" FIO_VERSION_STRING "\x1B[0m\n"
      "The facil.io library was originally coded by \x1B[1mBoaz Segev\x1B[0m.\n"
      "Please give credit where credit is due.\n"
      "\x1B[1mYour support is only fair\x1B[0m - give value for value.\n"
      "(code contributions / donations)\n\n");
  fprintf(stderr, "===============\n");
  FIO_LOG_DEBUG("example FIO_LOG_DEBUG message.");
  FIO_LOG_DEBUG2("example FIO_LOG_DEBUG2 message.");
  FIO_LOG_INFO("example FIO_LOG_INFO message.");
  FIO_LOG_WARNING("example FIO_LOG_WARNING message.");
  FIO_LOG_SECURITY("example FIO_LOG_SECURITY message.");
  FIO_LOG_ERROR("example FIO_LOG_ERROR message.");
  FIO_LOG_FATAL("example FIO_LOG_FATAL message.");
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, type_sizes)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, random)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, atomics)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, bitwise)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, atol)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, math)();
  FIO_NAME_TEST(stl, math_speed)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, FIO_NAME(sort, num))();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, url)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, glob_matching)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, imap_core)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, state)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, string_core_helpers)();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___linked_list_test();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___index_list_test();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, ary____test)();
  FIO_NAME_TEST(stl, ary2____test)();
  FIO_NAME_TEST(stl, ary3____test)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, uset___test_size_t)();
  FIO_NAME_TEST(stl, umap___test_size)();
  FIO_NAME_TEST(stl, omap___test_size_t)();
  FIO_NAME_TEST(stl, omap___test_size_lru)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, fio_big_str)();
  FIO_NAME_TEST(stl, fio_small_str)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, time)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, queue)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, cli)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, cli)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, stream)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, poll)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, filename)();
  fprintf(stderr, "===============\n");
#ifndef FIO_MEMORY_DISABLE
  FIO_NAME_TEST(stl, mem_helper_speeds)();
  fprintf(stderr, "===============\n");
#endif
  /* test memory allocator that initializes memory to zero */
  FIO_NAME_TEST(FIO_NAME(stl, fio_mem_test_safe), mem)();
  fprintf(stderr, "===============\n");
  /* test memory allocator that allows junk data in allocations */
  FIO_NAME_TEST(FIO_NAME(stl, fio_mem_test_unsafe), mem)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, sock)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, fiobj)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, server)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, risky)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, sha1)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, chacha)();
#if !DEBUG
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, lock_speed)();
#endif
  fprintf(stderr, "===============\n");
  {
    char timebuf[64];
    fio_time2rfc7231(timebuf, fio_time_real().tv_sec);
    fprintf(stderr,
            "On %s\n"
            "Testing \x1B[1mPASSED\x1B[0m "
            "for facil.io core version: "
            "\x1B[1m" FIO_VERSION_STRING "\x1B[0m"
            "\n",
            timebuf);
  }
  fprintf(stderr,
          "\nThe facil.io library was originally coded by \x1B[1mBoaz "
          "Segev\x1B[0m.\n"
          "Please give credit where credit is due.\n"
          "\x1B[1mYour support is only fair\x1B[0m - give value for value.\n"
          "(code contributions / donations)\n\n");
#if !defined(FIO_NO_COOKIE)
  fio___();
#endif
}

/* *****************************************************************************
Testing cleanup
***************************************************************************** */
#undef FIO_TEST_CSTL
#undef FIO_TEST_REPEAT

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_TEST_CSTL */

/* do not build test if fio-stl.h was already included once */
#ifndef FIO_FIO_TEST_CSTL_ONLY_ONCE
#define FIO_FIO_TEST_CSTL_ONLY_ONCE 1
#endif
