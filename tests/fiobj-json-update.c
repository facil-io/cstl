/* FIOBJ JSON hash update semantics. */
#include "renderer-test-alloc.h"
#define FIO_FIOBJ
#include "fio-stl/include.h"

int main(void) {
  FIOBJ map = fiobj_hash_new();
  assert(map);
  size_t parsed = fiobj_hash_update_json(map, FIO_STR_INFO1("{\"items\":[1]}"));
  assert(parsed == strlen("{\"items\":[1]}"));
  parsed = fiobj_hash_update_json(map, FIO_STR_INFO1("{\"items\":[2]}"));
  assert(parsed == strlen("{\"items\":[2]}"));
  FIOBJ items = fiobj_hash_get2(map, "items", 5);
  assert(fiobj_array_count(items) == (FIOBJ_JSON_APPEND ? 2U : 1U));
  assert(fiobj2i(fiobj_array_get(items, 0)) == (FIOBJ_JSON_APPEND ? 1 : 2));
  fiobj_free(map);
  assert(!renderer_alloc_live);
  return 0;
}
