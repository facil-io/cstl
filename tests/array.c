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
}

int main(void) {
  FIO_NAME_TEST(stl, ary____test)();
  FIO_NAME_TEST(stl, ary2____test)();
  FIO_NAME_TEST(stl, ary3____test)();
  test_array_direct_edges();
  (void)ary____test_was_destroyed;
  return 0;
}
