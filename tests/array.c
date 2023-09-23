#define FIO_LOG
#define FIO_TIME
#include "fio-stl.h"

#define FIO_FIOBJ
#include "fio-stl.h"

#define FIO_ARRAY_NAME ary1
#include "fio-stl.h"

#define FIO_ARRAY_NAME            ary2
#define FIO_ARRAY_ENABLE_EMBEDDED 0
#include "fio-stl.h"

#define FIO_ARRAY_NAME            ary3
#define FIO_ARRAY_EXPONENTIAL     1
#define FIO_ARRAY_ENABLE_EMBEDDED 0
#include "fio-stl.h"

#define FIO_ARRAY_NAME            ary4
#define FIO_ARRAY_ENABLE_EMBEDDED 0
#include "fio-stl.h"

static void *stub_i2v(size_t i) { return (void *)i; }

int main(int argc, char const *argv[]) {
  struct {
    char *name;
    void *(*push)(void *, void *);
    int (*pop)(void *, void **);
    void (*unshift)(void *, void *);
    int (*shift)(void *, void **);
    void (*free)(void *);
    void *(*new)(void);
    void *(*i2v)(size_t);
  } t[] = {{
               .name = "FIOBJ - tagging, steady growth, embedded",
               .push = (void *(*)(void *, void *))fiobj_array_push,
               .pop = (int (*)(void *, void **))fiobj_array_pop,
               .unshift = (void (*)(void *, void *))fiobj_array_unshift,
               .shift = (int (*)(void *, void **))fiobj_array_shift,
               .free = (void (*)(void *))fiobj_free,
               .new = (void *(*)(void))fiobj_array_new,
               .i2v = (void *(*)(size_t))fiobj_num_new,
           },
           {
               .name = "Dynamic - no tagging, steady growth, embedded",
               .push = (void *(*)(void *, void *))ary1_push,
               .pop = (int (*)(void *, void **))ary1_pop,
               .unshift = (void (*)(void *, void *))ary1_unshift,
               .shift = (int (*)(void *, void **))ary1_shift,
               .free = (void (*)(void *))ary1_free,
               .new = (void *(*)(void))ary1_new,
               .i2v = stub_i2v,
           },
           {
               .name = "Dynamic - no tagging, steady growth, no embedded",
               .push = (void *(*)(void *, void *))ary2_push,
               .pop = (int (*)(void *, void **))ary2_pop,
               .unshift = (void (*)(void *, void *))ary2_unshift,
               .shift = (int (*)(void *, void **))ary2_shift,
               .free = (void (*)(void *))ary2_free,
               .new = (void *(*)(void))ary2_new,
               .i2v = stub_i2v,
           },
           {
               .name = "Dynamic - no tagging, exponential growth, no embedded",
               .push = (void *(*)(void *, void *))ary3_push,
               .pop = (int (*)(void *, void **))ary3_pop,
               .unshift = (void (*)(void *, void *))ary3_unshift,
               .shift = (int (*)(void *, void **))ary3_shift,
               .free = (void (*)(void *))ary3_free,
               .new = (void *(*)(void))ary3_new,
               .i2v = stub_i2v,
           },
           {
               .name = NULL,
           }};

  fprintf(stderr,
          "This is a performance test to test the cost of pointer tagging and "
          "the embedded array optimization on performance\n");

  for (size_t items = 1; items < 64000; items <<= 1) {
    fprintf(stderr, "Running push/pop test for %zu items\n", items);
    for (int i = 0; t[i].name; ++i) {
      uint64_t start;
      uint64_t end;
      fprintf(stderr, "   * testing %s:\n", t[i].name);

      void *a = t[i].new();

      start = fio_time_micro();
      for (size_t number = 0; number < items; ++number) {
        t[i].push(a, t[i].i2v(number));
      }
      end = fio_time_micro();
      fprintf(stderr, "\t - push:     %lld us\n", end - start);

      start = fio_time_micro();
      while (!t[i].pop(a, NULL))
        ;
      end = fio_time_micro();
      fprintf(stderr, "\t - pop:      %lld us\n", end - start);

      t[i].free(a);
      a = t[i].new();

      start = fio_time_micro();
      for (size_t number = 0; number < items; ++number) {
        t[i].unshift(a, t[i].i2v(number));
      }
      end = fio_time_micro();
      fprintf(stderr, "\t - unshift:  %lld us\n", end - start);

      start = fio_time_micro();
      while (!t[i].shift(a, NULL))
        ;
      end = fio_time_micro();
      fprintf(stderr, "\t - shift:     %lld us\n", end - start);

      t[i].free(a);
    }
  }

  return 0;
  (void)argc;
  (void)argv;
}
