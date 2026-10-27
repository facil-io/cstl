/* *****************************************************************************
Test - map module (`210 map.h`)
***************************************************************************** */
#include "test-helpers.h"

#define FIO_UMAP_NAME uset___test_size_t
#define FIO_MAP_KEY   size_t
#define FIO_MAP_TEST
#include FIO_INCLUDE_FILE

#define FIO_UMAP_NAME umap___test_size
#define FIO_MAP_KEY   size_t
#define FIO_MAP_VALUE size_t
#define FIO_MAP_TEST
#include FIO_INCLUDE_FILE

#define FIO_OMAP_NAME   omap___test_size_t
#define FIO_MAP_KEY     size_t
#define FIO_MAP_ORDERED 1
#define FIO_MAP_TEST
#include FIO_INCLUDE_FILE

#define FIO_OMAP_NAME omap___test_size_lru
#define FIO_MAP_KEY   size_t
#define FIO_MAP_VALUE size_t
#define FIO_MAP_LRU   (1UL << 24)
#define FIO_MAP_TEST
#include FIO_INCLUDE_FILE

static int map_clear_during_each(umap___test_size_each_s *e) {
  umap___test_size_clear(e->parent);
  return 0;
}

static int map_remove_during_each(umap___test_size_each_s *e) {
  umap___test_size_remove(e->parent, e->key, e->key, NULL);
  return 0;
}

int main(void) {
  { /* Valid empty eviction must not access a NULL allocation. */
    umap___test_size_s m = FIO_MAP_INIT;
    umap___test_size_evict(&m, 1);
    umap___test_size_reserve(&m, 128);
    for (size_t i = 0; i < 16; ++i)
      umap___test_size_set(&m, 7, i + 1, i + 1, NULL);
    /* A distinct seventeenth full collision cannot retrieve/delete a key. */
    FIO_ASSERT(!umap___test_size_get_ptr(&m, 7, 17),
               "collision lookup returned a different key");
    FIO_ASSERT(umap___test_size_remove(&m, 7, 17, NULL) == -1,
               "collision removal deleted a different key");
    FIO_ASSERT(umap___test_size_count(&m) == 16,
               "collision removal changed the count");
    umap___test_size_clear(&m);
    umap___test_size_evict(&m, 1);
    umap___test_size_destroy(&m);
  }
  { /* Callback mutation must stop traversal, including same-allocation clear. */
    umap___test_size_s m = FIO_MAP_INIT;
    umap___test_size_reserve(&m, 16);
    for (size_t i = 0; i < 8; ++i)
      umap___test_size_set(&m, i + 1, i + 1, i + 1, NULL);
    umap___test_size_each(&m, map_remove_during_each, NULL, 0);
    FIO_ASSERT(umap___test_size_count(&m) == 7,
               "callback removal did not stop iteration");
    umap___test_size_each(&m, map_clear_during_each, NULL, 0);
    FIO_ASSERT(!umap___test_size_count(&m), "callback clear failed");
    umap___test_size_destroy(&m);
  }
  FIO_NAME_TEST(stl, uset___test_size_t)();
  FIO_NAME_TEST(stl, umap___test_size)();
  FIO_NAME_TEST(stl, omap___test_size_t)();
  FIO_NAME_TEST(stl, omap___test_size_lru)();
  return 0;
}
