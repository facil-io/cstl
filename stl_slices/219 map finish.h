/* *****************************************************************************
Map Testing
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_MAP_NAME map            /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#include "210 map api.h"            /* Development inclusion - ignore line */
#include "211 ordered map.h"        /* Development inclusion - ignore line */
#include "211 unordered map.h"      /* Development inclusion - ignore line */
#define FIO_MAP_TEST                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
#if defined(FIO_MAP_TEST) && defined(FIO_MAP_NAME)

FIO_SFUNC int FIO_NAME_TEST(stl, FIO_NAME(FIO_MAP_NAME, task))(FIO_MAP_OBJ o,
                                                               void *p) {
  *(size_t *)p -= (size_t)FIO_MAP_OBJ2TYPE(o);
  return 0;
}
FIO_SFUNC void FIO_NAME_TEST(stl, FIO_MAP_NAME)(void) {
  /*
   * test unrodered maps here
   */
  uint64_t total = 0;
#ifdef FIO_MAP_KEY
  fprintf(stderr,
          "* testing %s map (hash-map) " FIO_MACRO2STR(FIO_MAP_NAME) "\n",
          (FIO_MAP_ORDERED ? "ordered  " : "unordered"));
#define FIO_MAP_TEST_KEY FIO_MAP_KEY
#else
  fprintf(stderr,
          "* testing %s map (set) " FIO_MACRO2STR(FIO_MAP_NAME) "\n",
          (FIO_MAP_ORDERED ? "ordered  " : "unordered"));
#define FIO_MAP_TEST_KEY FIO_MAP_TYPE
#endif
  FIO_NAME(FIO_MAP_NAME, s) m = FIO_MAP_INIT;
  const size_t MEMBERS = (1 << 18);
  for (size_t i = 1; i < MEMBERS; ++i) {
    total += i;
    FIO_MAP_TYPE old = (FIO_MAP_TYPE)i;
#ifdef FIO_MAP_KEY
    FIO_ASSERT((FIO_MAP_TYPE)i == FIO_NAME(FIO_MAP_NAME, set)(&m,
                                                              (FIO_MAP_HASH)i,
                                                              (FIO_MAP_KEY)i,
                                                              (FIO_MAP_TYPE)i,
                                                              &old),
               "insertion failed at %zu",
               i);
#else
    FIO_ASSERT((FIO_MAP_TYPE)i ==
                   FIO_NAME(FIO_MAP_NAME,
                            set)(&m, (FIO_MAP_HASH)i, (FIO_MAP_TYPE)i, &old),
               "insertion failed at %zu",
               i);
#endif
    FIO_ASSERT(old == FIO_MAP_TYPE_INVALID,
               "old value should be set to the invalid value (%zu != %zu @%zu)",
               old,
               (size_t)FIO_MAP_TYPE_INVALID,
               i);
    FIO_ASSERT(
        FIO_NAME(FIO_MAP_NAME, get)(&m, (FIO_MAP_HASH)i, (FIO_MAP_TEST_KEY)i) ==
            (FIO_MAP_TYPE)i,
        "set-get roundtrip error for %zu",
        i);
  }
  size_t old_capa = FIO_NAME(FIO_MAP_NAME, capa)(&m);

  for (size_t i = 1; i < MEMBERS; ++i) {
    FIO_ASSERT(
        FIO_NAME(FIO_MAP_NAME, get)(&m, (FIO_MAP_HASH)i, (FIO_MAP_TEST_KEY)i) ==
            (FIO_MAP_TYPE)i,
        "get error for %zu",
        i);
  }
  for (size_t i = 1; i < MEMBERS; ++i) {
    FIO_MAP_TYPE old = (FIO_MAP_TYPE)i;
#ifdef FIO_MAP_KEY
    FIO_ASSERT((FIO_MAP_TYPE)i == FIO_NAME(FIO_MAP_NAME, set)(&m,
                                                              (FIO_MAP_HASH)i,
                                                              (FIO_MAP_KEY)i,
                                                              (FIO_MAP_TYPE)i,
                                                              &old),
               "overwrite failed at %zu",
               i);
#else
    FIO_ASSERT((FIO_MAP_TYPE)i ==
                   FIO_NAME(FIO_MAP_NAME,
                            set)(&m, (FIO_MAP_HASH)i, (FIO_MAP_TYPE)i, &old),
               "overwrite failed at %zu",
               i);
#endif
    FIO_ASSERT(
        !memcmp(&old, &i, sizeof(old) > sizeof(i) ? sizeof(i) : sizeof(old)),
        "old value should be set to the replaced value");
    FIO_ASSERT(
        FIO_NAME(FIO_MAP_NAME, get)(&m, (FIO_MAP_HASH)i, (FIO_MAP_TEST_KEY)i) ==
            (FIO_MAP_TYPE)i,
        "set-get overwrite roundtrip error for %zu",
        i);
  }
  for (size_t i = 1; i < MEMBERS; ++i) {
    FIO_ASSERT(
        FIO_NAME(FIO_MAP_NAME, get)(&m, (FIO_MAP_HASH)i, (FIO_MAP_TEST_KEY)i) ==
            (FIO_MAP_TYPE)i,
        "get (overwrite) error for %zu",
        i);
  }
  for (size_t i = 1; i < MEMBERS; ++i) {

    FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&m) == MEMBERS - 1,
               "unexpected member count");
    FIO_NAME(FIO_MAP_NAME, remove)
    (&m, (FIO_MAP_HASH)i, (FIO_MAP_TEST_KEY)i, NULL);
    FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, count)(&m) == MEMBERS - 2,
               "removing member didn't count removal");
#ifdef FIO_MAP_KEY
    FIO_ASSERT((FIO_MAP_TYPE)i == FIO_NAME(FIO_MAP_NAME, set)(&m,
                                                              (FIO_MAP_HASH)i,
                                                              (FIO_MAP_KEY)i,
                                                              (FIO_MAP_TYPE)i,
                                                              NULL),
               "re-insertion failed at %zu",
               i);
#else
    FIO_ASSERT((FIO_MAP_TYPE)i ==
                   FIO_NAME(FIO_MAP_NAME,
                            set)(&m, (FIO_MAP_HASH)i, (FIO_MAP_TYPE)i, NULL),
               "re-insertion failed at %zu",
               i);
#endif

    FIO_ASSERT(FIO_NAME(FIO_MAP_NAME, get)(&m,
                                           (FIO_MAP_HASH)i,
                                           (FIO_MAP_TYPE)i) == (FIO_MAP_TYPE)i,
               "remove-set-get roundtrip error for %zu",
               i);
  }
  for (size_t i = 1; i < MEMBERS; ++i) {
    FIO_ASSERT(
        FIO_NAME(FIO_MAP_NAME, get)(&m, (FIO_MAP_HASH)i, (FIO_MAP_TEST_KEY)i) ==
            (FIO_MAP_TYPE)i,
        "get (remove/re-insert) error for %zu",
        i);
  }
  if (FIO_NAME(FIO_MAP_NAME, capa)(&m) != old_capa) {
    FIO_LOG_WARNING("capacity shouldn't change when re-inserting the same "
                    "number of items.");
  }
  {
    size_t count = 0;
    size_t tmp = total;
    FIO_MAP_EACH(FIO_MAP_NAME, &m, i) {
      ++count;
      tmp -= (size_t)(FIO_MAP_OBJ2TYPE(i->obj));
    }
    FIO_ASSERT(count + 1 == MEMBERS,
               "FIO_MAP_EACH macro error, repetitions %zu != %zu",
               count,
               MEMBERS - 1);
    FIO_ASSERT(
        !tmp,
        "FIO_MAP_EACH macro error total value %zu != 0 (%zu repetitions)",
        tmp,
        count);
    tmp = total;
    count = FIO_NAME(FIO_MAP_NAME,
                     each)(&m,
                           0,
                           FIO_NAME_TEST(stl, FIO_NAME(FIO_MAP_NAME, task)),
                           (void *)&tmp);
    FIO_ASSERT(count + 1 == MEMBERS,
               "each task error, repetitions %zu != %zu",
               count,
               MEMBERS - 1);
    FIO_ASSERT(!tmp,
               "each task error, total value %zu != 0 (%zu repetitions)",
               tmp,
               count);
  }
  FIO_NAME(FIO_MAP_NAME, destroy)(&m);
}
#undef FIO_MAP_TEST_KEY
#endif /* FIO_MAP_TEST */

/* *****************************************************************************
Map - cleanup
***************************************************************************** */
#undef FIO_MAP_PTR

#undef FIO_MAP_NAME
#undef FIO_UMAP_NAME
#undef FIO_OMAP_NAME

#undef FIO_MAP_DESTROY_AFTER_COPY

#undef FIO_MAP_HASH
#undef FIO_MAP_HASH_FIX
#undef FIO_MAP_HASH_FIXED
#undef FIO_MAP_HASH_INVALID
#undef FIO_MAP_HASH_IS_INVALID
#undef FIO_MAP_HASH_FN
#undef FIO_MAP_HASH_GET_HASH
#undef FIO_MAP_HASH_CACHED

#undef FIO_MAP_INDEX_CALC
#undef FIO_MAP_INDEX_INVALID
#undef FIO_MAP_INDEX_UNUSED
#undef FIO_MAP_INDEX_USED_BIT

#undef FIO_MAP_ORDERED

#undef FIO_MAP_KEY
#undef FIO_MAP_KEY_CMP
#undef FIO_MAP_KEY_COPY
#undef FIO_MAP_KEY_DESTROY
#undef FIO_MAP_KEY_DESTROY_SIMPLE
#undef FIO_MAP_KEY_DISCARD
#undef FIO_MAP_KEY_INVALID

#undef FIO_MAP_MAX_ELEMENTS
#undef FIO_MAP_MAX_FULL_COLLISIONS
#undef FIO_MAP_MAX_SEEK
#undef FIO_MAP_EVICT_LRU
#undef FIO_MAP_SHOULD_OVERWRITE

#undef FIO_MAP_OBJ
#undef FIO_MAP_OBJ2KEY
#undef FIO_MAP_OBJ2TYPE
#undef FIO_MAP_OBJ_CMP
#undef FIO_MAP_OBJ_COPY
#undef FIO_MAP_OBJ_DESTROY
#undef FIO_MAP_OBJ_DESTROY_AFTER
#undef FIO_MAP_OBJ_DISCARD
#undef FIO_MAP_OBJ_INVALID
#undef FIO_MAP_OBJ_KEY
#undef FIO_MAP_OBJ_KEY_CMP

#undef FIO_MAP_S
#undef FIO_MAP_SEEK_AS_ARRAY_LOG_LIMIT
#undef FIO_MAP_SIZE_TYPE
#undef FIO_MAP_TYPE
#undef FIO_MAP_TYPE_CMP
#undef FIO_MAP_TYPE_COPY
#undef FIO_MAP_TYPE_COPY_SIMPLE
#undef FIO_MAP_TYPE_DESTROY
#undef FIO_MAP_TYPE_DESTROY_SIMPLE
#undef FIO_MAP_TYPE_DISCARD
#undef FIO_MAP_TYPE_INVALID
#undef FIO_MAP_BIG
#undef FIO_MAP_HASH
#undef FIO_MAP_INDEX_USED_BIT
#undef FIO_MAP_TYPE
#undef FIO_MAP_TYPE_INVALID
#undef FIO_MAP_TYPE_COPY
#undef FIO_MAP_TYPE_COPY_SIMPLE
#undef FIO_MAP_TYPE_DESTROY
#undef FIO_MAP_TYPE_DESTROY_SIMPLE
#undef FIO_MAP_TYPE_DISCARD
#undef FIO_MAP_TYPE_CMP
#undef FIO_MAP_DESTROY_AFTER_COPY
#undef FIO_MAP_KEY
#undef FIO_MAP_KEY_INVALID
#undef FIO_MAP_KEY_COPY
#undef FIO_MAP_KEY_DESTROY
#undef FIO_MAP_KEY_DESTROY_SIMPLE
#undef FIO_MAP_KEY_DISCARD
#undef FIO_MAP_KEY_CMP
#undef FIO_MAP_OBJ
#undef FIO_MAP_OBJ_KEY
#undef FIO_MAP_OBJ_INVALID
#undef FIO_MAP_OBJ_COPY
#undef FIO_MAP_OBJ_DESTROY
#undef FIO_MAP_OBJ_CMP
#undef FIO_MAP_OBJ_KEY_CMP
#undef FIO_MAP_OBJ2KEY
#undef FIO_MAP_OBJ2TYPE
#undef FIO_MAP_OBJ_DISCARD
#undef FIO_MAP_DESTROY_AFTER_COPY
#undef FIO_MAP_OBJ_DESTROY_AFTER
#undef FIO_MAP_MAX_SEEK
#undef FIO_MAP_MAX_FULL_COLLISIONS
#undef FIO_MAP_CUCKOO_STEPS
#undef FIO_MAP_EVICT_LRU
#undef FIO_MAP_CAPA
#undef FIO_MAP_MEMORY_SIZE

#undef FIO_MAP___IMAP_FREE
#undef FIO_MAP___IMAP_DELETED
#undef FIO_MAP_TEST
