/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SORT_TYPE size_t
#include FIO_INCLUDE_FILE

/* McIlroy's "A Killer Adversary for Quicksort" - forces worst case pivots. */
static size_t *anti_val, anti_gas, anti_solid, anti_candidate, anti_cmps;
FIO_SFUNC int anti_is_bigger(size_t x, size_t y) {
  ++anti_cmps;
  if (anti_val[x] == anti_gas && anti_val[y] == anti_gas)
    anti_val[(x == anti_candidate) ? x : y] = anti_solid++;
  if (anti_val[x] == anti_gas)
    anti_candidate = x;
  else if (anti_val[y] == anti_gas)
    anti_candidate = y;
  return anti_val[x] > anti_val[y];
}
#define FIO_SORT_NAME            anti
#define FIO_SORT_TYPE            size_t
#define FIO_SORT_IS_BIGGER(a, b) anti_is_bigger((a), (b))
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
  }
  { /* test quick sort against qsort reference */
    const size_t len = (1ULL << 12);
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
    for (size_t count = 0; count < 3; ++count) {
      for (size_t i = 0; i < len; ++i) {
        mem[i] = mem[len + i] = (size_t)rand();
      }
      size_t_vec_sort(mem, len);
      qsort(mem + len,
            len,
            sizeof(mem[0]),
            (int (*)(const void *, const void *))FIO_NAME_TEST(stl, qsort_cmp));
      FIO_ASSERT(!memcmp(mem, mem + len, (sizeof(mem[0]) * len)),
                 "fio_sort != clib qsort (iteration %zu)",
                 count);
    }
    FIO_MEM_FREE(mem, (sizeof(*mem) * (len << 1)));
  }
  { /* adversarial input must not force quadratic behavior (introsort) */
    const size_t len = (1ULL << 14);
    size_t *mem =
        (size_t *)FIO_MEM_REALLOC(NULL, 0, (sizeof(*mem) * (len << 1)), 0);
    anti_val = mem + len;
    anti_gas = len - 1;
    anti_solid = anti_candidate = anti_cmps = 0;
    for (size_t i = 0; i < len; ++i) {
      mem[i] = i;
      anti_val[i] = anti_gas;
    }
    anti_sort(mem, len);
    for (size_t i = 1; i < len; ++i)
      FIO_ASSERT(anti_val[mem[i - 1]] <= anti_val[mem[i]],
                 "adversarial sort out of order at %zu",
                 i);
    /* n log2(n) = 229,376; quadratic behavior requires tens of millions */
    FIO_ASSERT(anti_cmps < (len * 14 * 8),
               "adversarial sort quadratic: %zu comparisons for %zu items",
               anti_cmps,
               len);
    FIO_MEM_FREE(mem, (sizeof(*mem) * (len << 1)));
  }
  { /* reversed, sorted and all-equal inputs past the threshold */
    const size_t len = 4099;
    size_t *mem = (size_t *)FIO_MEM_REALLOC(NULL, 0, (sizeof(*mem) * len), 0);
    for (size_t round = 0; round < 3; ++round) {
      for (size_t i = 0; i < len; ++i)
        mem[i] = (round == 0) ? (len - i) : (round == 1) ? i : 7;
      size_t_vec_sort(mem, len);
      for (size_t i = 1; i < len; ++i)
        FIO_ASSERT(mem[i - 1] <= mem[i], "sort round %zu failed", round);
    }
    FIO_MEM_FREE(mem, (sizeof(*mem) * len));
  }
}
