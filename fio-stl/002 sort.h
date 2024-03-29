/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_SORT_NAME num      /* Development inclusion - ignore line */
#define FIO_SORT_TYPE size_t   /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          A Good Enough Sorting Helper



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#ifdef FIO_SORT_NAME

/* *****************************************************************************
Sort Settings
***************************************************************************** */

#ifndef FIO_SORT_TYPE
#error FIO_SORT_TYPE must contain a valid type name!
#endif

#ifndef FIO_SORT_THRESHOLD
/** The default threshold below which quicksort delegates to insert sort. */
#define FIO_SORT_THRESHOLD 96
#endif

#ifndef FIO_SORT_SWAP
/** Default swap operation assumes an array and swaps array members */
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
FIO_IFUNC void FIO_NAME(FIO_SORT_NAME, sort)(FIO_SORT_TYPE *array,
                                             size_t count);

/* Insert sort, for small arrays of `FIO_SORT_TYPE`. */
SFUNC void FIO_NAME(FIO_SORT_NAME, isort)(FIO_SORT_TYPE *array, size_t count);

/* Quick sort, for larger arrays of `FIO_SORT_TYPE`. */
SFUNC void FIO_NAME(FIO_SORT_NAME, qsort)(FIO_SORT_TYPE *array, size_t count);

/* *****************************************************************************
Sort Implementation - inlined static functions
see ideas from: https://youtu.be/FJJTYQYB1JQ
***************************************************************************** */

/* Sorts a `FIO_SORT_TYPE` array with `count` members (quicksort). */
FIO_IFUNC void FIO_NAME(FIO_SORT_NAME, sort)(FIO_SORT_TYPE *array,
                                             size_t count) {
  FIO_NAME(FIO_SORT_NAME, qsort)(array, count);
}

/* *****************************************************************************
Sort Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* Insert sort, for small arrays of `FIO_SORT_TYPE`. */
SFUNC void FIO_NAME(FIO_SORT_NAME, isort)(FIO_SORT_TYPE *array, size_t count) {
  /* a fast(ish) small sort on small arrays */
  if ((!count | !array))
    return;
  if (count < 3) { /* special case */
    if (FIO_SORT_IS_BIGGER((array[0]), (array[count == 2])))
      FIO_SORT_SWAP((array[0]), (array[1]));
    return;
  }
  /* place smallest item in position array[0] (guard element) */
  for (size_t pos = 1; pos < count; ++pos) {
    if (FIO_SORT_IS_BIGGER((array[0]), (array[pos]))) {
      FIO_SORT_SWAP((array[0]), (array[pos]));
    }
  }
  /* perform insert sort */
  for (size_t i = 2; i < count; ++i) {
    for (size_t a = i - 1; FIO_SORT_IS_BIGGER((array[a]), (array[a + 1]));
         --a) {
      FIO_SORT_SWAP((array[a]), (array[a + 1]));
    }
  }
}

/* Sorts a `FIO_SORT_TYPE` array with `count` members. */
SFUNC void FIO_NAME(FIO_SORT_NAME, qsort)(FIO_SORT_TYPE *array, size_t count) {
  /* With thanks to Douglas C. Schmidt, as I used his code for reference:
   * https://code.woboq.org/userspace/glibc/stdlib/qsort.c.html
   */
  if ((!count | !array))
    return;
  if (count < FIO_SORT_THRESHOLD) {
    FIO_NAME(FIO_SORT_NAME, isort)(array, count);
    return;
  }
  /* no recursion, setup a stack that can hold log2(count). */
  struct {
    FIO_SORT_TYPE *lo;
    FIO_SORT_TYPE *hi;
  } queue[CHAR_BIT * sizeof(count) + 1], *top = queue;
#define fio_sort___queue_push(l, h)                                            \
  top->lo = l;                                                                 \
  top->hi = h;                                                                 \
  ++top;
  /* push all the array as the first queued partition */
  fio_sort___queue_push(array, array + (count - 1));
  for (;;) {
    FIO_SORT_TYPE *lo;
    FIO_SORT_TYPE *hi;
    FIO_SORT_TYPE *mid;
    --top; /* pop stack */
    lo = top->lo;
    hi = top->hi;
    const size_t slice_len = (hi - lo) + 1;

    /* sort small ranges using insert sort */
    if (slice_len < FIO_SORT_THRESHOLD) {
      FIO_NAME(FIO_SORT_NAME, isort)(lo, slice_len);
      if (queue == top)
        return;
      continue;
    }

    /* select a median element (1 of 3, fist, middle, last). */
    /* this also promises ordering between these 3 elements. */
    mid = lo + ((slice_len) >> 1);
    if (FIO_SORT_IS_BIGGER((lo[0]), (hi[0])))
      FIO_SORT_SWAP((lo[0]), (hi[0]));
    if (FIO_SORT_IS_BIGGER((lo[0]), (mid[0])))
      FIO_SORT_SWAP((lo[0]), (mid[0]));
    else if (FIO_SORT_IS_BIGGER((mid[0]), (hi[0])))
      FIO_SORT_SWAP((hi[0]), (mid[0]));

    /* partition: swap elements and pointers so mid is a partition pivot */
    FIO_SORT_TYPE *left = lo + 1;
    FIO_SORT_TYPE *right = hi - 2;
    /* place mid in the lower partition and update pointer, as it's known */
    FIO_SORT_SWAP((right[1]), (mid[0]));
    mid = right + 1;
    for (;;) {
      /* while order is fine, move on. */
      while (FIO_SORT_IS_BIGGER((mid[0]), (left[0])))
        ++left;
      while (FIO_SORT_IS_BIGGER((right[0]), (mid[0])))
        --right;
      /* order issue encountered (relative to pivot / mid)... */
      if (left < right) {
        /* right now, left is bigger than mid *and* right is smaller... swap. */
        FIO_SORT_SWAP(left[0], right[0]);
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
    /* push partitions in order of size to the stack (clears smaller first) */
    if ((right - lo) > (hi - left)) {
      fio_sort___queue_push(lo, right);
      fio_sort___queue_push(left, hi);
    } else {
      fio_sort___queue_push(left, hi);
      fio_sort___queue_push(lo, right);
    }
  }
}
#undef fio_sort___queue_push

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_SORT_TYPE
#undef FIO_SORT_TEST
#undef FIO_SORT_SWAP
#undef FIO_SORT_IS_BIGGER
#undef FIO_SORT_NAME
#endif /* FIO_SORT_NAME */
