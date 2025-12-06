/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SORT_TYPE size_t
#include FIO_INCLUDE_FILE

FIO_SFUNC int FIO_NAME_TEST(stl, qsort_cmp)(size_t *a, size_t *b) {
  return (int)(a[0] - b[0]);
}

int main(void) {
  { /* test insert sort of short array */
    size_t mixed[] = {19, 23, 28, 21, 3,  10, 7, 2,  13, 4,  15,
                      29, 26, 16, 24, 22, 11, 5, 14, 31, 25, 8,
                      12, 18, 20, 17, 1,  27, 9, 0,  6,  30};
    size_t ordered[] = {19, 23, 28, 21, 3,  10, 7, 2,  13, 4,  15,
                        29, 26, 16, 24, 22, 11, 5, 14, 31, 25, 8,
                        12, 18, 20, 17, 1,  27, 9, 0,  6,  30};
    const size_t len =
        (sizeof(ordered) / sizeof(ordered[0])) > FIO_SORT_THRESHOLD
            ? FIO_SORT_THRESHOLD
            : (sizeof(ordered) / sizeof(ordered[0]));
    qsort(ordered,
          len,
          sizeof(ordered[0]),
          (int (*)(const void *, const void *))FIO_NAME_TEST(stl, qsort_cmp));
    size_t_vec_isort(mixed, len);
    FIO_ASSERT(!memcmp(mixed, ordered, sizeof(*ordered) * len),
               "short sort failed!");
    clock_t start, end;
    start = clock();
    for (size_t i = 0; i < (1UL << 16); ++i) {
      FIO_COMPILER_GUARD;
      size_t_vec_sort(mixed, len);
    }
    end = clock();
    fprintf(stderr,
            "\t* facil.io small sorted test cycles:          %zu\n",
            (size_t)(end - start));
    start = clock();
    for (size_t i = 0; i < (1UL << 16); ++i) {
      FIO_COMPILER_GUARD;
      qsort(mixed,
            len,
            sizeof(mixed[0]),
            (int (*)(const void *, const void *))FIO_NAME_TEST(stl, qsort_cmp));
    }
    end = clock();
    fprintf(stderr,
            "\t* clib     small sorted test cycles:          %zu\n",
            (size_t)(end - start));
  }
  { /* test quick sort of an array with (1ULL << 18) elements */
    const size_t len = (1ULL << 18);
    size_t *mem =
        (size_t *)FIO_MEM_REALLOC(NULL, 0, (sizeof(*mem) * (len << 1)), 0);
    for (size_t i = 0; i < len; ++i) {
      mem[i] = mem[len + i] = (size_t)rand();
    }
    size_t_vec_sort(mem, len);
    qsort(mem + len,
          len,
          sizeof(mem[0]),
          (int (*)(const void *, const void *))FIO_NAME_TEST(stl, qsort_cmp));
    if (memcmp(mem, mem + len, (sizeof(mem[0]) * len))) {
      size_t i = 0;
      while (mem[i] == mem[len + i] && i < len)
        ++i;
      FIO_ASSERT(0, "fio_sort != clib qsort first error at index %zu", i);
    }
    clock_t start, end, fio_clk = 0, lib_clk = 0;
    for (size_t count = 0; count < 8; ++count) {
      for (size_t i = 0; i < len; ++i) {
        mem[i] = mem[len + i] = (size_t)rand();
      }
      start = clock();
      size_t_vec_sort(mem, len);
      end = clock();
      fio_clk += end - start;
      start = clock();
      qsort(mem + len,
            len,
            sizeof(mem[0]),
            (int (*)(const void *, const void *))FIO_NAME_TEST(stl, qsort_cmp));
      end = clock();
      lib_clk += end - start;
      FIO_ASSERT(!memcmp(mem, mem + len, (sizeof(mem[0]) * len)),
                 "fio_sort != clib qsort (iteration %zu)",
                 count);
    }
    FIO_MEM_FREE(mem, (sizeof(*mem) * (len << 1)));

    fprintf(stderr,
            "\t* facil.io random quick sort test cycles:     %zu\n",
            (size_t)fio_clk);
    fprintf(stderr,
            "\t* clib     random quick sort test cycles:     %zu\n",
            (size_t)lib_clk);
  }
}
