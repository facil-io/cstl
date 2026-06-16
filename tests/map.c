/* *****************************************************************************
Test - map modules (`210 map.h`, `210 map2.h`)
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

#define FIO_MAP2_NAME  map2___test_size
#define FIO_MAP2_KEY   size_t
#define FIO_MAP2_VALUE size_t
#define FIO_MAP2_TEST
#include FIO_INCLUDE_FILE

#define FIO_MAP2_NAME    map2___test_ordered
#define FIO_MAP2_KEY     size_t
#define FIO_MAP2_VALUE   size_t
#define FIO_MAP2_ORDERED 1
#define FIO_MAP2_TEST
#include FIO_INCLUDE_FILE

int main(void) {
  FIO_NAME_TEST(stl, uset___test_size_t)();
  FIO_NAME_TEST(stl, umap___test_size)();
  FIO_NAME_TEST(stl, omap___test_size_t)();
  FIO_NAME_TEST(stl, omap___test_size_lru)();
  FIO_NAME_TEST(stl, map2___test_size)();
  FIO_NAME_TEST(stl, map2___test_ordered)();
  return 0;
}
