/* *****************************************************************************
Map Testing
***************************************************************************** */
#if defined(FIO_MAP_TEST) && defined(FIO_MAP_NAME)

FIO_SFUNC void FIO_NAME_TEST(stl, FIO_MAP_NAME)(void) {
/*
 * test unrodered maps here
 */
#ifdef FIO_MAP_KEY
  fprintf(
      stderr,
      "* testing unordered map (hash-map)" FIO_MACRO2STR(FIO_MAP_NAME) "\n");
#define FIO_MAP_TEST_KEY FIO_MAP_KEY
#else
  fprintf(stderr,
          "* testing unordered map (set)" FIO_MACRO2STR(FIO_MAP_NAME) "\n");
#define FIO_MAP_TEST_KEY FIO_MAP_TYPE
#endif
  FIO_NAME(FIO_MAP_NAME, s) m = FIO_MAP_INIT;
  const size_t MEMBERS = (1 << 18);
  for (size_t i = 1; i < MEMBERS; ++i) {
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
               "old value should be set to the invalid value");
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
               "re-insertion failed at %zu",
               i);
#else
    FIO_ASSERT((FIO_MAP_TYPE)i ==
                   FIO_NAME(FIO_MAP_NAME,
                            set)(&m, (FIO_MAP_HASH)i, (FIO_MAP_TYPE)i, &old),
               "re-insertion failed at %zu",
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
        "get error for %zu",
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
        "get error for %zu",
        i);
  }
  if (FIO_NAME(FIO_MAP_NAME, capa)(&m) != old_capa) {
    FIO_LOG_WARNING("capacity shouldn't change when re-inserting the same "
                    "number of items.");
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
#undef FIO_MAP___NAME

#undef FIO_MAP_DESTROY_AFTER_COPY

#undef FIO_MAP_HASH
#undef FIO_MAP_HASH_FIX
#undef FIO_MAP_HASH_FIXED
#undef FIO_MAP_HASH_INVALID
#undef FIO_MAP_HASH_IS_INVALID

#undef FIO_MAP_INDEX_CALC
#undef FIO_MAP_INDEX_INVALID
#undef FIO_MAP_INDEX_UNUSED
#undef FIO_MAP_INDEX_USED_BIT

#undef FIO_MAP_UNORDERED

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
