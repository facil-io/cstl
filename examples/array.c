/* *****************************************************************************
Easily construct dynamic Array types for any type, including structs and unions.
***************************************************************************** */

typedef struct {
  int i;
  float f;
} foo_s;

#define FIO_ARRAY_NAME           foo_ary
#define FIO_ARRAY_TYPE           foo_s
#define FIO_ARRAY_TYPE_CMP(a, b) (a.i == b.i && a.f == b.f)
#include "fio-stl/include.h" /* or "fio-stl.h" */

int main(void) {
  foo_ary_s a = FIO_ARRAY_INIT;
  foo_ary_push(&a, (foo_s){.i = 42});
  foo_ary_push(&a, (foo_s){.i = -42});
  FIO_ARRAY_EACH(foo_ary, &a, pos) { // pos will be a pointer to the element
    printf("* [%zu]: %p : %d\n",
           (size_t)(pos - foo_ary2ptr(&a)),
           (void *)pos,
           pos->i);
  }
  foo_ary_destroy(&a);
}
