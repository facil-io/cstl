## Quick Sort and Insert Sort

```c
#define FIO_SORT_NAME num
#define FIO_SORT_TYPE size_t
#include "fio-stl.h"
```

If `FIO_SORT_NAME` is defined, the following sorting functions will be defined. The `FIO_SORT_TYPE` macro **must** also be defined to specify the type of array elements to sort.

This module can be included multiple times to create sorting functions for different types.

### Sort Settings

The following macros define the behavior of the sorting algorithm.

#### `FIO_SORT_NAME`

```c
#define FIO_SORT_NAME num // produces function names: num_sort, num_qsort, num_isort
```

The prefix used for naming the sorting functions.

#### `FIO_SORT_TYPE`

```c
#define FIO_SORT_TYPE size_t
```

The type of the array members to be sorted.

**Note**: this macro **must** be defined.

#### `FIO_SORT_IS_BIGGER`

```c
#define FIO_SORT_IS_BIGGER(a, b) ((a) > (b))
```

Comparison macro that **must** evaluate to `1` if `a > b`, and `0` if equal or smaller.

The default implementation uses the `>` operator, which works for numeric types.

#### `FIO_SORT_SWAP`

```c
#define FIO_SORT_SWAP(a, b)                                                    \
  do {                                                                         \
    FIO_SORT_TYPE tmp__ = (a);                                                 \
    (a) = (b);                                                                 \
    (b) = tmp__;                                                               \
  } while (0)
```

Swaps two array members. The default implementation uses a temporary variable.

Usually there is no need to override this macro.

#### `FIO_SORT_THRESHOLD`

```c
#define FIO_SORT_THRESHOLD 96
```

The threshold below which quick-sort delegates to insert sort. For small arrays, insert sort is faster due to lower overhead.

Usually there is no need to override this macro.

### Sorting API

The following functions are created using the `FIO_SORT_NAME` prefix. In the examples below, `FIO_SORT_NAME` is assumed to be `num`.

#### `num_sort`

```c
void num_sort(FIO_SORT_TYPE *array, size_t count);
```

Sorts the first `count` members of `array`.

This is the main sorting function and currently wraps `num_qsort` (quick-sort).

#### `num_qsort`

```c
void num_qsort(FIO_SORT_TYPE *array, size_t count);
```

Sorts the first `count` members of `array` using quick-sort.

The implementation is non-recursive, using an internal stack to avoid stack overflow on large arrays. For small partitions (below `FIO_SORT_THRESHOLD`), it delegates to insert sort for better performance.

The algorithm uses median-of-three pivot selection for improved performance on partially sorted data.

#### `num_isort`

```c
void num_isort(FIO_SORT_TYPE *array, size_t count);
```

Sorts the first `count` members of `array` using insert-sort.

Insert sort is efficient for small arrays but has O(n²) complexity. Use only with small arrays or when the data is nearly sorted.

### Sort Example

The following example creates an array of random strings and then sorts the array:

```c
#define FIO_STR_SMALL sstr
#define FIO_SORT_NAME      sstr
#define FIO_SORT_TYPE sstr_s
#define FIO_SORT_IS_BIGGER(a, b)                                               \
  fio_string_is_greater(sstr_info(&a), sstr_info(&b))
#define FIO_RAND
#include "fio-stl.h"

#define STRING_ARRAY_LENGTH 128
int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;
  sstr_s ary[STRING_ARRAY_LENGTH] = {{0}};
  /* fill array with random data and print state */
  for (size_t i = 0; i < STRING_ARRAY_LENGTH; ++i) {
    sstr_write_hex(ary + i, fio_rand64());
  }
  printf("Starting with array of strings as:\n");
  for (size_t i = 0; i < STRING_ARRAY_LENGTH; ++i) {
    printf("[%zu] %s\n", i, sstr2ptr(ary + i));
  }
  /* sort array and print state */
  sstr_qsort(ary, STRING_ARRAY_LENGTH);
  printf("\n\nOrdered array of strings is:\n");
  for (size_t i = 0; i < STRING_ARRAY_LENGTH; ++i) {
    printf("[%zu] %s\n", i, sstr2ptr(ary + i));
    sstr_destroy(ary + i); /* cleanup */
  }
  return 0;
}
```

A simpler example sorting integers:

```c
#define FIO_SORT_NAME int
#define FIO_SORT_TYPE int
#include "fio-stl.h"

int main(void) {
  int numbers[] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
  size_t count = sizeof(numbers) / sizeof(numbers[0]);
  
  int_sort(numbers, count);
  
  for (size_t i = 0; i < count; ++i) {
    printf("%d ", numbers[i]);
  }
  printf("\n"); /* Output: 0 1 2 3 4 5 6 7 8 9 */
  return 0;
}
```

-------------------------------------------------------------------------------
