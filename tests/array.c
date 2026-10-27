/* *****************************************************************************
Test - array module (`201 array.h`)
***************************************************************************** */
#include "test-helpers.h"

static int ary____test_was_destroyed = 0;

#define FIO_ARRAY_NAME ary____test
#define FIO_ARRAY_TYPE int
#define FIO_ARRAY_TEST
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
#define FIO_ARRAY_TEST
#define FIO_PTR_TAG(p)   fio___dynamic_types_test_tag(((uintptr_t)p))
#define FIO_PTR_UNTAG(p) fio___dynamic_types_test_untag(((uintptr_t)p))
#include FIO_INCLUDE_FILE

#define FIO_ARRAY_NAME ary3____test
#define FIO_ARRAY_TEST
#include FIO_INCLUDE_FILE

FIO_SFUNC void test_array_direct_edges(void) {
  ary2____test_s a = FIO_ARRAY_INIT;
  uint8_t old = 0;

  ary2____test_push(&a, 1);
  ary2____test_push(&a, 2);
  ary2____test_push(&a, 3);
  FIO_ASSERT(ary2____test_count(&a) == 3, "array count after pushes");
  FIO_ASSERT(ary2____test_get(&a, 1) == 2, "array get after pushes");

  ary2____test_set(&a, 1, 42, &old);
  FIO_ASSERT(old == 2, "array set should return old value");
  FIO_ASSERT(ary2____test_get(&a, 1) == 42, "array set should update slot");

  old = 0;
  ary2____test_remove(&a, 1, &old);
  FIO_ASSERT(old == 42, "array remove should return removed value");
  FIO_ASSERT(ary2____test_count(&a) == 2, "array count after remove");
  FIO_ASSERT(ary2____test_get(&a, 1) == 3, "array remove should compact");

  ary2____test_destroy(&a);

  /* Inline expansion, shift and index bounds. */
  a = (ary2____test_s)FIO_ARRAY_INIT;
  for (uint8_t i = 0; i < 10; ++i)
    ary2____test_push(&a, i + 1);
  FIO_ASSERT(ary2____test_shift(&a, NULL) == 0, "inline shift");
  for (uint8_t i = 0; i < 9; ++i)
    FIO_ASSERT(ary2____test_get(&a, i) == i + 2, "inline shift order");
  FIO_ASSERT(ary2____test_get(&a, INT64_C(0x100000000)) == 0xFF,
             "large index rejected");
  FIO_ASSERT(ary2____test_remove(&a, INT64_C(0x100000000), NULL) == -1,
             "large removal rejected");
  FIO_ASSERT(!ary2____test_set(&a, UINT32_MAX, 7, NULL),
             "maximal index rejected");
  ary2____test_destroy(&a);

  a = (ary2____test_s)FIO_ARRAY_INIT;
  for (uint8_t i = 0; i < 4; ++i)
    ary2____test_push(&a, i + 1);
  FIO_ASSERT(ary2____test_set(&a, -5, 42, NULL), "inline prepend gap");
  FIO_ASSERT(ary2____test_count(&a) == 5 && ary2____test_get(&a, 0) == 42 &&
                 ary2____test_get(&a, 4) == 4,
             "inline prepend state");
  ary2____test_destroy(&a);

  /* Heap offset must not be lost when a sparse set stays within capacity. */
  ary2____test_reserve(&a, 32);
  ary2____test_push(&a, 13);
  ary2____test_shift(&a, NULL);
  ary2____test_push(&a, 17);
  FIO_ASSERT(ary2____test_set(&a, 4, 29, NULL), "heap sparse set");
  FIO_ASSERT(ary2____test_count(&a) == 5 && ary2____test_get(&a, 0) == 17 &&
                 ary2____test_get(&a, 4) == 29 &&
                 ary2____test_get(&a, 2) == 0xFF,
             "heap offset and gap");
  ary2____test_destroy(&a);

  a = (ary2____test_s)FIO_ARRAY_INIT;
  ary2____test_reserve(&a, 32);
  ary2____test_push(&a, 9);
  FIO_ASSERT(ary2____test_set(&a, -4, 8, NULL), "heap prepend gap");
  FIO_ASSERT(ary2____test_count(&a) == 4 && ary2____test_get(&a, 0) == 8 &&
                 ary2____test_get(&a, 1) == 0xFF &&
                 ary2____test_get(&a, 3) == 9,
             "heap prepend state");
  ary2____test_destroy(&a);

  ary____test_s ints = FIO_ARRAY_INIT;
  ary____test_s more = FIO_ARRAY_INIT;
  ary____test_push(&ints, 101);
  ary____test_push(&more, 202);
  ary____test_push(&more, 303);
  FIO_ASSERT(ary____test_concat(&ints, &more) == &ints,
             "non-byte concatenation");
  FIO_ASSERT(ary____test_count(&ints) == 3 && ary____test_get(&ints, 1) == 202 &&
                 ary____test_get(&ints, 2) == 303,
             "concatenated elements");
  ary____test_destroy(&ints);
  ary____test_destroy(&more);
}

static void test_array_iterator(void) {
  ary____test_s a = FIO_ARRAY_INIT;
  ary____test_iterator_s it =
      ary____test_each_next(&a, (ary____test_iterator_s){0});
  FIO_ASSERT(!it.p && it.i == 0, "empty iterator");
  FIO_ASSERT(ary____test_push(&a, 1), "iteration setup");
  FIO_ASSERT(ary____test_push(&a, 2), "iteration setup");
  it = ary____test_each_next(&a, it);
  FIO_ASSERT(it.p && it.i == 0 && *it.p == 1,
             "NULL p restarts at index zero");
  FIO_ASSERT(ary____test_is_embedded(&a), "starts embedded");
  FIO_ASSERT(ary____test_reserve(&a, 64) >= 64,
             "force embedded-to-heap relocation");
  FIO_ASSERT(!ary____test_is_embedded(&a), "reserve relocated storage");
  it = ary____test_each_next(&a, it);
  FIO_ASSERT(it.p && it.i == 1 && *it.p == 2,
             "iteration continues after relocation");
  FIO_ASSERT(ary____test_push(&a, 3), "append during iteration");
  it = ary____test_each_next(&a, it);
  FIO_ASSERT(it.p && it.i == 2 && *it.p == 3, "append is visited");
  it = ary____test_each_next(&a, it);
  FIO_ASSERT(!it.p && it.i == 3, "iteration ends");
  it = ary____test_each_next(&a, it);
  FIO_ASSERT(!it.p && it.i == 3, "end remains invalid");
  it = ary____test_each_next(&a, (ary____test_iterator_s){0});
  FIO_ASSERT(it.p && it.i == 0 && *it.p == 1,
             "zero-initialized iterator starts iteration");
  int marker = 0;
  it.i = ary____test_count(&a);
  it.p = &marker; /* only non-NULL matters; it is never dereferenced */
  it = ary____test_each_next(&a, it);
  FIO_ASSERT(!it.p && it.i == ary____test_count(&a) + 1,
             "out-of-range index retains its position");
  size_t seen = 0;
  FIO_ARRAY_EACH(ary____test, &a, pos) {
    FIO_ASSERT(pos.i == seen && *pos.p == (int)(seen + 1),
               "macro iterator index and value");
    ++seen;
    if (pos.i == 0)
      continue;
    if (pos.i == 1)
      break;
  }
  FIO_ASSERT(seen == 2, "macro break and continue");
  seen = 0;
  FIO_ARRAY_EACH(ary____test, &a, pos) {
    FIO_ASSERT(pos.i == seen && *pos.p == (int)(seen + 1),
               "macro resumes from the index after relocation");
    if (pos.i == 0) {
      ary____test_compact(&a);
      FIO_ASSERT(ary____test_is_embedded(&a), "compact relocates to embedded");
    } else if (pos.i == 1) {
      FIO_ASSERT(ary____test_reserve(&a, 64) >= 64,
                 "reserve relocates to heap");
    }
    ++seen;
  }
  FIO_ASSERT(seen == 3, "macro completes across both relocations");
  ary____test_destroy(&a);

  ary2____test_s bytes = FIO_ARRAY_INIT;
  FIO_ASSERT(ary2____test_push(&bytes, 11), "byte iteration setup");
  FIO_ASSERT(ary2____test_push(&bytes, 12), "byte iteration setup");
  ary2____test_iterator_s byte_it =
      ary2____test_each_next(&bytes, (ary2____test_iterator_s){0});
  FIO_ASSERT(byte_it.p && *byte_it.p == 11, "byte iterator starts");
  ary2____test_reserve(&bytes, 32);
  ary2____test_compact(&bytes); /* returns to embedded storage */
  byte_it = ary2____test_each_next(&bytes, byte_it);
  FIO_ASSERT(byte_it.i == 1 && byte_it.p && *byte_it.p == 12,
             "iterator survives compact and re-embedding");
  ary2____test_destroy(&bytes);
}

int main(void) {
  test_array_iterator();
  FIO_NAME_TEST(stl, ary____test)();
  FIO_NAME_TEST(stl, ary2____test)();
  FIO_NAME_TEST(stl, ary3____test)();
  test_array_direct_edges();
  (void)ary____test_was_destroyed;
  return 0;
}
