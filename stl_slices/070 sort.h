/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_SORT num                /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                          A Good Enough Sorting Helper




***************************************************************************** */
#ifdef FIO_SORT

/* *****************************************************************************
Sort Settings
***************************************************************************** */

#ifndef FIO_SORT_THRESHOLD
#define FIO_SORT_THRESHOLD 96
#endif

#ifndef FIO_SORT_TYPE
#define FIO_SORT_TYPE      size_t
#define FIO_SORT_TYPE_AUTO 1
#endif

#ifndef FIO_SORT_SWAP
#define FIO_SORT_SWAP(a, b)                                                    \
  do {                                                                         \
    FIO_SORT_TYPE tmp__ = (a);                                                 \
    (a) = (b);                                                                 \
    (b) = tmp__;                                                               \
  } while (0)
#endif

#ifndef FIO_SORT_IS_BIGGER
/** MUST evaluate as 1 if a > b (zero if equal or smaller). */
#define FIO_SORT_IS_BIGGER(a, b) ((a) > (b))
#endif

/* *****************************************************************************
Sort API
***************************************************************************** */

/* Sorts a `FIO_SORT_TYPE` array with `count` members (quicksort). */
FIO_IFUNC void FIO_NAME(FIO_SORT, sort)(FIO_SORT_TYPE *array, size_t count);

/* Insert sort, for small arrays of `FIO_SORT_TYPE`. */
SFUNC void FIO_NAME(FIO_SORT, isort)(FIO_SORT_TYPE *array, size_t count);

/* Quick sort, for larger arrays of `FIO_SORT_TYPE`. */
SFUNC void FIO_NAME(FIO_SORT, qsort)(FIO_SORT_TYPE *array, size_t count);

/* *****************************************************************************
Sort Implementation - inlined static functions
see ideas from: https://youtu.be/FJJTYQYB1JQ
***************************************************************************** */

/* Sorts a `FIO_SORT_TYPE` array with `count` members (quicksort). */
FIO_IFUNC void FIO_NAME(FIO_SORT, sort)(FIO_SORT_TYPE *array, size_t count) {
  FIO_NAME(FIO_SORT, qsort)(array, count);
}

/* *****************************************************************************
Sort Implementation - possibly externed functions.
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/* Insert sort, for small arrays of `FIO_SORT_TYPE`. */
SFUNC void FIO_NAME(FIO_SORT, isort)(FIO_SORT_TYPE *array, size_t count) {
  /* TODO: a fast(ish) small sort on small arrays */
  if ((!count | !array))
    return;
  if (count < 3) { /* special case */
    if (FIO_SORT_IS_BIGGER(array[0], array[count == 2]))
      FIO_SORT_SWAP(array[0], array[1]);
    return;
  }
  /* place smallest item in position array[0] (guard element) */
  for (size_t pos = 1; pos < count; ++pos) {
    if (FIO_SORT_IS_BIGGER(array[0], array[pos])) {
      FIO_SORT_SWAP(array[0], array[pos]);
    }
  }
  /* perform insert sort */
  for (size_t i = 2; i < count; ++i) {
    for (size_t a = i - 1; FIO_SORT_IS_BIGGER(array[a], array[a + 1]); --a) {
      FIO_SORT_SWAP(array[a], array[a + 1]);
    }
  }
}

/* Sorts a `FIO_SORT_TYPE` array with `count` members. */
SFUNC void FIO_NAME(FIO_SORT, qsort)(FIO_SORT_TYPE *array, size_t count) {
  if ((!count | !array))
    return;
  if (count < FIO_SORT_THRESHOLD) {
    FIO_NAME(FIO_SORT, isort)(array, count);
    return;
  }
  /* no recursion, setup a stack that can hold log2(count). */
  struct {
    FIO_SORT_TYPE *lo;
    FIO_SORT_TYPE *hi;
  } stack[CHAR_BIT * sizeof(count) + 1], *top = stack;
#define stack_push(l, h)                                                       \
  top->lo = l;                                                                 \
  top->hi = h;                                                                 \
  ++top;
#define stack_pop(l, h)                                                        \
  --top;                                                                       \
  l = top->lo;                                                                 \
  h = top->hi;
  /* push all array to the stack */
  stack_push(array, array + (count - 1));
  for (;;) {
    FIO_SORT_TYPE *lo;
    FIO_SORT_TYPE *hi;
    FIO_SORT_TYPE *mid;
    stack_pop(lo, hi);
    const size_t slice_len = (hi - lo) + 1;

    /* sort small ranges using insert sort */
    if (slice_len < FIO_SORT_THRESHOLD) {
      FIO_NAME(FIO_SORT, isort)(lo, slice_len);
      if (stack == top)
        return;
      continue;
    }

    /* select a median element (1 of 3, fist, middle, last). */
    /* this also promises ordering between these 3 elements. */
    mid = lo + ((slice_len) >> 1);
    if (FIO_SORT_IS_BIGGER(lo[0], hi[0]))
      FIO_SORT_SWAP(lo[0], hi[0]);
    if (FIO_SORT_IS_BIGGER(lo[0], mid[0]))
      FIO_SORT_SWAP(lo[0], mid[0]);
    else if (FIO_SORT_IS_BIGGER(mid[0], hi[0]))
      FIO_SORT_SWAP(hi[0], mid[0]);

    /* partition: swap elements and pointers so mid is a partition pivot */
    FIO_SORT_TYPE *left = lo + 1;
    FIO_SORT_TYPE *right = hi - 1;
    for (;;) {
      /* while order is fine, move on. */
      while (FIO_SORT_IS_BIGGER(mid[0], left[0]))
        ++left;
      while (FIO_SORT_IS_BIGGER(right[0], mid[0]))
        --right;
      /* order issue encountered... */
      if (left < right) {
        /* right now, left is bigger than mid *and* right is smaller... swap. */
        FIO_SORT_SWAP(left[0], right[0]);
        /* test if we actually swapped mid itself, if so, pointer follows. */
        if (mid == left)
          mid = right;
        else if (mid == right)
          mid = left;
        ++left;
        --right;
        continue;
      }
      /* we passed the middle point and so, we can finish partitioning */
      if (left > right)
        break;
      /* left == right (odd numbered array) */
      ++left;
      --right;
      break;
    }
    /* push partitions in order of size to the stack (clear smaller first) */
    if ((right - lo) > (hi - left)) {
      stack_push(lo, right);
      stack_push(left, hi);
    } else {
      stack_push(left, hi);
      stack_push(lo, right);
    }
  }
}

/* *****************************************************************************
Testing
***************************************************************************** */
#if defined(FIO_TEST_CSTL) && defined(FIO_SORT_TYPE_AUTO) && FIO_SORT_TYPE_AUTO

int FIO_NAME(fio_qsort___cmp, FIO_SORT)(FIO_SORT_TYPE *a, FIO_SORT_TYPE *b) {
  return (int)(a[0] - b[0]);
}

FIO_SFUNC void FIO_NAME_TEST(stl, FIO_NAME(sort, FIO_SORT))(void) {
  fprintf(stderr, "* Testing facil.io array sort helper\n");
  {
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
          (int (*)(const void *, const void *))FIO_NAME(fio_qsort___cmp,
                                                        FIO_SORT));
    FIO_NAME(FIO_SORT, isort)(mixed, len);
    FIO_ASSERT(!memcmp(mixed, ordered, sizeof(*ordered) * len),
               "short sort failed!");
    clock_t start, end;
    start = clock();
    for (size_t i = 0; i < (1UL << 16); ++i) {
      FIO_COMPILER_GUARD;
      FIO_NAME(FIO_SORT, sort)(mixed, len);
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
            (int (*)(const void *, const void *))FIO_NAME(fio_qsort___cmp,
                                                          FIO_SORT));
    }
    end = clock();
    fprintf(stderr,
            "\t* qsort    small sorted test cycles:          %zu\n",
            (size_t)(end - start));
  }
  { /*
     * TODO: test long array sort!
     */
    const size_t len = (1ULL << 18);
    size_t *mem =
        (size_t *)FIO_MEM_REALLOC(NULL, 0, (sizeof(*mem) * (len << 1)), 0);
    for (size_t i = 0; i < len; ++i) {
      mem[i] = mem[len + i] = (size_t)rand();
    }
    FIO_NAME(FIO_SORT, sort)(mem, len);
    qsort(mem + len,
          len,
          sizeof(mem[0]),
          (int (*)(const void *, const void *))FIO_NAME(fio_qsort___cmp,
                                                        FIO_SORT));
    if (memcmp(mem, mem + len, (sizeof(mem[0]) * len))) {
      size_t i = 0;
      while (mem[i] == mem[len + i] && i < len)
        ++i;
      FIO_ASSERT(0, "fio_sort != clib qsort first error at index %zu", i);
    }
    clock_t start, end, fio_clk = 0, lib_clk = 0;
    for (int count = 0; count < 8; ++count) {
      for (size_t i = 0; i < len; ++i) {
        mem[i] = mem[len + i] = (size_t)rand();
      }
      start = clock();
      FIO_NAME(FIO_SORT, sort)(mem, len);
      end = clock();
      fio_clk += end - start;
      start = clock();
      qsort(mem + len,
            len,
            sizeof(mem[0]),
            (int (*)(const void *, const void *))FIO_NAME(fio_qsort___cmp,
                                                          FIO_SORT));
      end = clock();
      lib_clk += end - start;
      FIO_ASSERT(!memcmp(mem, mem + len, (sizeof(mem[0]) * len)),
                 "fio_sort != clib qsort (iteration %zu)",
                 count);
    }
    FIO_MEM_FREE(mem, (sizeof(*mem) * (len << 1)));

    fprintf(stderr,
            "\t* facil.io random quick sort test cycles:   %zu\n",
            (size_t)fio_clk);
    fprintf(stderr,
            "\t* clib     random quick sort test cycles:   %zu\n",
            (size_t)lib_clk);
  }
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_SORT_THRESHOLD
#undef FIO_SORT_TYPE
#undef FIO_SORT_TYPE_AUTO
#undef FIO_SORT_SWAP
#undef FIO_SORT
#endif /* FIO_SORT */
