/* HTTP/FIOBJ bridge: JSON integer range stays exact. */
#define FIO_FIOBJ
#define FIO_HTTP
#include "test-helpers.h"

static void test_number(int64_t num) {
  fio_json_obj_result_s results[] = {
      FIO___HTTP_FIOBJ_CALLBACKS.on_number(NULL, num),
      fiobj___json_on_number(NULL, num),
  };
  for (size_t i = 0; i < sizeof(results) / sizeof(*results); ++i) {
    fio_json_obj_result_s r = results[i];
    FIOBJ o = (FIOBJ)r.obj;
    FIO_ASSERT(!r.err && o, "on_number(%lld) failed", (long long)num);
    if (num >= (int64_t)INTPTR_MIN && num <= (int64_t)INTPTR_MAX) {
      FIO_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_NUMBER && fiobj2i(o) == num,
                 "on_number(%lld) must be an exact Number",
                 (long long)num);
    } else {
      FIO_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_FLOAT && fiobj2f(o) == (double)num,
                 "on_number(%lld) must not narrow (Float expected)",
                 (long long)num);
    }
    fiobj_free(o);
  }
}

static void test_numbers(void) {
  test_number(0);
  test_number(-1);
  test_number((int64_t)INTPTR_MIN);
  test_number((int64_t)INTPTR_MAX);
  test_number(INT64_MIN);
  test_number(INT64_MAX);
#if INTPTR_MAX < INT64_MAX
  test_number((int64_t)INTPTR_MIN - 1);
  test_number((int64_t)INTPTR_MAX + 1);
  test_number(4294967296LL);
#endif
  fprintf(stderr,
          "\t* integer range (%zu-bit intptr_t): no narrowing\n",
          sizeof(intptr_t) * 8);
}

int main(void) {
  fprintf(stderr, "Testing HTTP params => FIOBJ bridge security:\n");
  test_numbers();
  return 0;
}
