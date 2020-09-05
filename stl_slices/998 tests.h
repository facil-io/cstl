/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************














                                Testing














***************************************************************************** */

#if !defined(FIO_FIO_TEST_CSTL_ONLY_ONCE) && (defined(FIO_TEST_CSTL))
#define FIO_FIO_TEST_CSTL_ONLY_ONCE 1

#ifdef FIO_EXTERN_TEST
void fio_test_dynamic_types(void);
#else
FIO_SFUNC void fio_test_dynamic_types(void);
#endif
#if !defined(FIO_EXTERN_TEST) || defined(FIO_EXTERN_COMPLETE)

/* Common testing values / Macros */
#define TEST_FUNC   static __attribute__((unused))
#define TEST_REPEAT 4096

/* Make sure logging and FIOBJ memory marking are set. */
#define FIO_LOG
#ifndef FIOBJ_MARK_MEMORY
#define FIOBJ_MARK_MEMORY 1
#endif
#ifndef FIO_FIOBJ
#define FIO_FIOBJ
#endif
#ifndef FIOBJ_MALLOC
#define FIOBJ_MALLOC /* define to test with custom allocator */
#endif
#include __FILE__

/* Add non-type options to minimize `#include` instructions */
#define FIO_ATOL
#define FIO_BITWISE
#define FIO_BITMAP
#define FIO_RAND
#define FIO_ATOMIC
#define FIO_RISKY_HASH
#define FIO_MALLOC /* define to tests types with custom allocator */
#include __FILE__

TEST_FUNC uintptr_t fio___dynamic_types_test_tag(uintptr_t i) { return i | 1; }
TEST_FUNC uintptr_t fio___dynamic_types_test_untag(uintptr_t i) {
  return i & (~((uintptr_t)1UL));
}

/* *****************************************************************************
URL parsing - Test
***************************************************************************** */

#define FIO_URL
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

TEST_FUNC void fio___dynamic_types_test___linked_list_test(void) {
  fprintf(stderr, "* Testing linked lists.\n");
  FIO_LIST_HEAD FIO_LIST_INIT(ls);
  for (int i = 0; i < TEST_REPEAT; ++i) {
    ls____test_s *node = ls____test_push(
        &ls, (ls____test_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*node), 0));
    node->data = i;
  }
  int tester = 0;
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    FIO_ASSERT(pos->data == tester++,
               "Linked list ordering error for push or each");
    FIO_ASSERT(ls____test_root(&pos->node) == pos,
               "Linked List root offset error");
  }
  FIO_ASSERT(tester == TEST_REPEAT,
             "linked list EACH didn't loop through all the list");
  while (ls____test_any(&ls)) {
    ls____test_s *node = ls____test_pop(&ls);
    node = (ls____test_s *)fio___dynamic_types_test_untag((uintptr_t)(node));
    FIO_ASSERT(node, "Linked list pop or any failed");
    FIO_ASSERT(node->data == --tester, "Linked list ordering error for pop");
    FIO_MEM_FREE(node, sizeof(*node));
  }
  tester = TEST_REPEAT;
  for (int i = 0; i < TEST_REPEAT; ++i) {
    ls____test_s *node = ls____test_unshift(
        &ls, (ls____test_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*node), 0));
    node->data = i;
  }
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    FIO_ASSERT(pos->data == --tester,
               "Linked list ordering error for unshift or each");
  }
  FIO_ASSERT(tester == 0,
             "linked list EACH didn't loop through all the list after unshift");
  tester = TEST_REPEAT;
  while (ls____test_any(&ls)) {
    ls____test_s *node = ls____test_shift(&ls);
    node = (ls____test_s *)fio___dynamic_types_test_untag((uintptr_t)(node));
    FIO_ASSERT(node, "Linked list pop or any failed");
    FIO_ASSERT(node->data == --tester, "Linked list ordering error for shift");
    FIO_MEM_FREE(node, sizeof(*node));
  }
  FIO_ASSERT(FIO_NAME_BL(ls____test, empty)(&ls),
             "Linked list empty should have been true");
  for (int i = 0; i < TEST_REPEAT; ++i) {
    ls____test_s *node = ls____test_push(
        &ls, (ls____test_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*node), 0));
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

/* *****************************************************************************
Dynamic Array - Test
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
#define FIO_ATOMIC
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

/* *****************************************************************************
Hash Map / Set - test
***************************************************************************** */

/* a simple set of numbers */
#define FIO_MAP_NAME           set_____test
#define FIO_MAP_TYPE           size_t
#define FIO_MAP_TYPE_CMP(a, b) ((a) == (b))
#define FIO_PTR_TAG(p)         fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p)       fio___dynamic_types_test_untag(((uintptr_t)p))
#include __FILE__

/* a simple set of numbers */
#define FIO_MAP_NAME           set2_____test
#define FIO_MAP_TYPE           size_t
#define FIO_MAP_TYPE_CMP(a, b) 1
#define FIO_PTR_TAG(p)         fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p)       fio___dynamic_types_test_untag(((uintptr_t)p))
#include __FILE__

TEST_FUNC size_t map_____test_key_copy_counter = 0;
TEST_FUNC void map_____test_key_copy(char **dest, char *src) {
  *dest =
      (char *)FIO_MEM_REALLOC(NULL, 0, (strlen(src) + 1) * sizeof(*dest), 0);
  FIO_ASSERT(*dest, "no memory to allocate key in map_test")
  strcpy(*dest, src);
  ++map_____test_key_copy_counter;
}
TEST_FUNC void map_____test_key_destroy(char **dest) {
  FIO_MEM_FREE(*dest, strlen(*dest) + 1);
  *dest = NULL;
  --map_____test_key_copy_counter;
}

/* keys are strings, values are numbers */
#define FIO_MAP_KEY            char *
#define FIO_MAP_KEY_CMP(a, b)  (strcmp((a), (b)) == 0)
#define FIO_MAP_KEY_COPY(a, b) map_____test_key_copy(&(a), (b))
#define FIO_MAP_KEY_DESTROY(a) map_____test_key_destroy(&(a))
#define FIO_MAP_TYPE           size_t
#define FIO_MAP_NAME           map_____test
#include __FILE__

#define HASHOFi(i) i /* fio_risky_hash(&(i), sizeof((i)), 0) */
#define HASHOFs(s) fio_risky_hash(s, strlen((s)), 0)

TEST_FUNC int set_____test_each_task(size_t o, void *a_) {
  uintptr_t *i_p = (uintptr_t *)a_;
  FIO_ASSERT(o == ++(*i_p), "set_each started at a bad offset!");
  FIO_ASSERT(HASHOFi((o - 1)) == set_____test_each_get_key(),
             "set_each key error!");
  return 0;
}

TEST_FUNC void fio___dynamic_types_test___map_test(void) {
  {
    set_____test_s m = FIO_MAP_INIT;
    fprintf(stderr, "* Testing dynamic hash / set maps.\n");

    fprintf(stderr, "* Testing set (hash map where value == key).\n");
    FIO_ASSERT(set_____test_count(&m) == 0,
               "freshly initialized map should have no objects");
    FIO_ASSERT(set_____test_capa(&m) == 0,
               "freshly initialized map should have no capacity");
    FIO_ASSERT(set_____test_reserve(&m, (TEST_REPEAT >> 1)) >=
                   (TEST_REPEAT >> 1),
               "reserve should increase capacity.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set_____test_set_if_missing(&m, HASHOFi(i), i + 1);
    }
    {
      uintptr_t pos_test = (TEST_REPEAT >> 1);
      size_t count =
          set_____test_each(&m, pos_test, set_____test_each_task, &pos_test);
      FIO_ASSERT(count == set_____test_count(&m),
                 "set_each tast returned the wrong counter.");
      FIO_ASSERT(count == pos_test, "set_each position testing error");
    }

    FIO_ASSERT(set_____test_count(&m) == TEST_REPEAT,
               "After inserting %zu items to set, got %zu items",
               (size_t)TEST_REPEAT,
               (size_t)set_____test_count(&m));
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set_____test_get(&m, HASHOFi(i), i + 1) == i + 1,
                 "item retrival error in set.");
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set_____test_get(&m, HASHOFi(i), i + 2) == 0,
                 "item retrival error in set - object comparisson error?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set_____test_set_if_missing(&m, HASHOFi(i), i + 1);
    }
    {
      size_t i = 0;
      FIO_MAP_EACH(set_____test, &m, pos) {
        FIO_ASSERT(pos->obj == pos->hash + 1 || !(~pos->hash),
                   "FIO_MAP_EACH loop out of order?")
        ++i;
      }
      FIO_ASSERT(i == set_____test_count(&m), "FIO_MAP_EACH loop incomplete?")
    }
    FIO_ASSERT(set_____test_count(&m) == TEST_REPEAT,
               "Inserting existing object should keep existing object.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set_____test_get(&m, HASHOFi(i), i + 1) == i + 1,
                 "item retrival error in set - insert failed to update?");
      FIO_ASSERT(set_____test_get_ptr(&m, HASHOFi(i), i + 1) &&
                     set_____test_get_ptr(&m, HASHOFi(i), i + 1)[0] == i + 1,
                 "pointer retrival error in set.");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set_____test_set(&m, HASHOFi(i), i + 2, &old);
      FIO_ASSERT(old == 0,
                 "old pointer not initialized with old (or missing) data");
    }

    FIO_ASSERT(set_____test_count(&m) == (TEST_REPEAT * 2),
               "full hash collision shoudn't break map until attack limit.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set_____test_get(&m, HASHOFi(i), i + 2) == i + 2,
                 "item retrival error in set - overwrite failed to update?");
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set_____test_get(&m, HASHOFi(i), i + 1) == i + 1,
                 "item retrival error in set - collision resolution error?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set_____test_remove(&m, HASHOFi(i), i + 1, &old);
      FIO_ASSERT(old == i + 1,
                 "removed item not initialized with old (or missing) data");
    }
    FIO_ASSERT(set_____test_count(&m) == TEST_REPEAT,
               "removal should update object count.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set_____test_get(&m, HASHOFi(i), i + 1) == 0,
                 "removed items should be unavailable");
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set_____test_get(&m, HASHOFi(i), i + 2) == i + 2,
                 "previous items should be accessible after removal");
    }
    set_____test_destroy(&m);
  }
  {
    set2_____test_s m = FIO_MAP_INIT;
    fprintf(stderr, "* Testing set map without value comparison.\n");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set2_____test_set_if_missing(&m, HASHOFi(i), i + 1);
    }

    FIO_ASSERT(set2_____test_count(&m) == TEST_REPEAT,
               "After inserting %zu items to set, got %zu items",
               (size_t)TEST_REPEAT,
               (size_t)set2_____test_count(&m));
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == i + 1,
                 "item retrival error in set.");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set2_____test_set_if_missing(&m, HASHOFi(i), i + 2);
    }
    FIO_ASSERT(set2_____test_count(&m) == TEST_REPEAT,
               "Inserting existing object should keep existing object.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == i + 1,
                 "item retrival error in set - insert failed to update?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set2_____test_set(&m, HASHOFi(i), i + 2, &old);
      FIO_ASSERT(old == i + 1,
                 "old pointer not initialized with old (or missing) data");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == i + 2,
                 "item retrival error in set - overwrite failed to update?");
    }
    {
      /* test partial removal */
      for (size_t i = 1; i < TEST_REPEAT; i += 2) {
        size_t old = 5;
        set2_____test_remove(&m, HASHOFi(i), 0, &old);
        FIO_ASSERT(old == i + 2,
                   "removed item not initialized with old (or missing) data "
                   "(%zu != %zu)",
                   old,
                   i + 2);
      }
      for (size_t i = 1; i < TEST_REPEAT; i += 2) {
        FIO_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == 0,
                   "previous items should NOT be accessible after removal");
        set2_____test_set_if_missing(&m, HASHOFi(i), i + 2);
      }
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set2_____test_remove(&m, HASHOFi(i), 0, &old);
      FIO_ASSERT(old == i + 2,
                 "removed item not initialized with old (or missing) data "
                 "(%zu != %zu)",
                 old,
                 i + 2);
    }
    FIO_ASSERT(set2_____test_count(&m) == 0,
               "removal should update object count.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      FIO_ASSERT(set2_____test_get(&m, HASHOFi(i), 0) == 0,
                 "previous items should NOT be accessible after removal");
    }
    set2_____test_destroy(&m);
  }

  {
    map_____test_s *m = map_____test_new();
    fprintf(stderr, "* Testing hash map.\n");
    FIO_ASSERT(map_____test_count(m) == 0,
               "freshly initialized map should have no objects");
    FIO_ASSERT(map_____test_capa(m) == 0,
               "freshly initialized map should have no capacity");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      char buffer[64];
      int l = snprintf(buffer, 63, "%zu", i);
      buffer[l] = 0;
      map_____test_set(m, HASHOFs(buffer), buffer, i + 1, NULL);
    }
    FIO_ASSERT(map_____test_key_copy_counter == TEST_REPEAT,
               "key copying error - was the key copied?");
    FIO_ASSERT(map_____test_count(m) == TEST_REPEAT,
               "After inserting %zu items to map, got %zu items",
               (size_t)TEST_REPEAT,
               (size_t)map_____test_count(m));
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      char buffer[64];
      int l = snprintf(buffer + 1, 61, "%zu", i);
      buffer[l + 1] = 0;
      FIO_ASSERT(map_____test_get(m, HASHOFs(buffer + 1), buffer + 1) == i + 1,
                 "item retrival error in map.");
      FIO_ASSERT(map_____test_get_ptr(m, HASHOFs(buffer + 1), buffer + 1) &&
                     map_____test_get_ptr(
                         m, HASHOFs(buffer + 1), buffer + 1)[0] == i + 1,
                 "pointer retrival error in map.");
    }
    map_____test_free(m);
    FIO_ASSERT(map_____test_key_copy_counter == 0,
               "key destruction error - was the key freed?");
  }
  {
    set_____test_s s = FIO_MAP_INIT;
    map_____test_s m = FIO_MAP_INIT;
    fprintf(stderr, "* Testing attack resistance (SHOULD print warnings).\n");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      char buf[64];
      fio_ltoa(buf, i, 16);
      set_____test_set(&s, 1, i + 1, NULL);
      map_____test_set(&m, 1, buf, i + 1, NULL);
    }
    FIO_ASSERT(set_____test_count(&s) != TEST_REPEAT,
               "full collision protection failed (set)?");
    FIO_ASSERT(map_____test_count(&m) != TEST_REPEAT,
               "full collision protection failed (map)?");
    FIO_ASSERT(set_____test_count(&s) != 1,
               "full collision test failed to push elements (set)?");
    FIO_ASSERT(map_____test_count(&m) != 1,
               "full collision test failed to push elements (map)?");
    set_____test_destroy(&s);
    map_____test_destroy(&m);
  }
}

#undef HASHOFi
#undef HASHOFs

/* *****************************************************************************
Dynamic Strings - test
***************************************************************************** */

#define FIO_STR_NAME fio_big_str
#define FIO_STR_WRITE_TEST_FUNC
#include __FILE__

#define FIO_STR_SMALL fio_small_str
#define FIO_STR_WRITE_TEST_FUNC
#include __FILE__

/**
 * Tests the fio_str functionality.
 */
TEST_FUNC void fio___dynamic_types_test___str(void) {
  FIO_NAME_TEST(stl, fio_big_str)();
  FIO_NAME_TEST(stl, fio_small_str)();
}

/* *****************************************************************************
Time - test
***************************************************************************** */
#define FIO_TIME
#include __FILE__
/* *****************************************************************************
Queue - test
***************************************************************************** */
#define FIO_QUEUE
#include __FILE__
/* *****************************************************************************
CLI - test
***************************************************************************** */
#define FIO_CLI
#include __FILE__
/* *****************************************************************************
Memory Allocation - test
***************************************************************************** */
#define FIO_MEMORY_NAME                   fio_mem_test_safe
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 1
#define FIO_MEMORY_USE_PTHREAD_MUTEX      0
#define FIO_MEMORY_ARENA_COUNT            2
#include __FILE__
#define FIO_MEMORY_NAME                   fio_mem_test_unsafe
#define FIO_MEMORY_INITIALIZE_ALLOCATIONS 0
#define FIO_MEMORY_USE_PTHREAD_MUTEX      0
#define FIO_MEMORY_ARENA_COUNT            2
#include __FILE__
/* *****************************************************************************
Socket helper and Stream testing
***************************************************************************** */
#define FIO_SOCK
#define FIO_STREAM
#include __FILE__

/* *****************************************************************************
FIOBJ and JSON testing
***************************************************************************** */
#define FIO_FIOBJ
#include __FILE__

/* *****************************************************************************
Environment printout
***************************************************************************** */

#define FIO_PRINT_SIZE_OF(T) fprintf(stderr, "\t" #T "\t%zu Bytes\n", sizeof(T))

TEST_FUNC void FIO_NAME_TEST(stl, type_sizes)(void) {
  switch (sizeof(void *)) {
  case 2:
    fprintf(stderr, "* 16bit words size (unexpected, unknown effects).\n");
    break;
  case 4:
    fprintf(stderr, "* 32bit words size (some features might be slower).\n");
    break;
  case 8:
    fprintf(stderr, "* 64bit words size okay.\n");
    break;
  case 16:
    fprintf(stderr, "* 128bit words size... wow!\n");
    break;
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
  long page = sysconf(_SC_PAGESIZE);
  if (page > 0) {
    fprintf(stderr, "\tPage\t%ld bytes.\n", page);
    FIO_ASSERT((page >> FIO_MEM_PAGE_SIZE_LOG) <= 1,
               "page size mismatch!\n          "
               "facil.io should be recompiled with "
               "`CFLAGS=-DFIO_MEM_PAGE_SIZE_LOG = %.0lf",
               log2(page));
  }
}
#undef FIO_PRINT_SIZE_OF

/* *****************************************************************************
Locking - Speed Test
***************************************************************************** */
#define FIO___LOCK2_TEST_TASK    (1LU << 25)
#define FIO___LOCK2_TEST_THREADS 32U
#define FIO___LOCK2_TEST_REPEAT  1

#ifndef H___FIO_LOCK2___H
#include <pthread.h>
#endif

FIO_SFUNC void fio___lock_speedtest_task_inner(void *s) {
  size_t *r = (size_t *)s;
  static size_t i;
  for (i = 0; i < FIO___LOCK2_TEST_TASK; ++i) {
    __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&mutex);
  if (s)
    fio___lock_speedtest_task_inner(s);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

FIO_SFUNC void FIO_NAME_TEST(stl, lock_speed)(void) {
  uint64_t start, end;
  pthread_t threads[FIO___LOCK2_TEST_THREADS];

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
          .type_size = sizeof(pthread_mutex_t),
          .type_name = "pthread_mutex_t",
          .name = "pthreads (pthread_mutex)",
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
    __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
      fprintf(
          stderr, "%s (%d)\n", (test_repeat ? "Round" : "Warmup"), test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      test_funcs[fn].task(NULL); /* warmup */
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK2_TEST_TASK; ++i) {
        __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
    __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
    fio___lock_speedtest_task_inner(&result);
  }
  end = fio_time_micro();
  fprintf(stderr, " %zu mms\n", (size_t)(end - start));
  clock_t long_work = end - start;
  fprintf(stderr, "(%zu mms):\n", long_work);
  for (int test_repeat = 0; test_repeat < FIO___LOCK2_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK2_TEST_REPEAT > 1)
      fprintf(
          stderr, "%s (%d)\n", (test_repeat ? "Round" : "Warmup"), test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
        __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
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
          long_work);
  for (int test_repeat = 0; test_repeat < FIO___LOCK2_TEST_REPEAT;
       ++test_repeat) {
    if (FIO___LOCK2_TEST_REPEAT > 1)
      fprintf(
          stderr, "%s (%d)\n", (test_repeat ? "Round" : "Warmup"), test_repeat);
    for (size_t fn = 0; test_funcs[fn].name; ++fn) {
      size_t result = 0;
      test_funcs[fn].task((void *)&result); /* warmup */
      result = 0;
      start = fio_time_micro();
      for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
        pthread_create(threads + i, NULL, test_funcs[fn].task, &result);
      }
      for (size_t i = 0; i < FIO___LOCK2_TEST_THREADS; ++i) {
        pthread_join(threads[i], NULL);
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
Testing functiun
***************************************************************************** */

TEST_FUNC void fio____test_dynamic_types__stack_poisoner(void) {
  const size_t len = 1UL << 16;
  uint8_t buf[1UL << 16];
  __asm__ __volatile__("" ::: "memory");
  memset(buf, (int)(~0U), len);
  __asm__ __volatile__("" ::: "memory");
  fio_trylock(buf);
}

TEST_FUNC void fio_test_dynamic_types(void) {
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
  FIO_NAME_TEST(stl, url)();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___linked_list_test();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, ary____test)();
  FIO_NAME_TEST(stl, ary2____test)();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___map_test();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___str();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, time)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, queue)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, cli)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, fio_stream)();
  fprintf(stderr, "===============\n");
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
  FIO_NAME_TEST(stl, risky)();
  fprintf(stderr, "===============\n");
  FIO_NAME_TEST(stl, lock_speed)();
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
#undef TEST_REPEAT
#undef TEST_FUNC
#undef FIO_ASSERT

#endif /* FIO_EXTERN_COMPLETE */
#endif
