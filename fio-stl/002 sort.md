# Sorting

```c
#define FIO_SORT_NAME num
#define FIO_SORT_TYPE size_t
#include "fio-stl.h"
```

A macro-generated quicksort + insert-sort combo. Define `FIO_SORT_NAME` and `FIO_SORT_TYPE` before including the header to get a set of sort functions named after your type. Include the header multiple times to sort different types.

### Configuration Macros

#### `FIO_SORT_NAME`

```c
#define FIO_SORT_NAME num
```

Prefix for generated function names: `num_sort`, `num_qsort`, `num_isort`.

#### `FIO_SORT_TYPE`

```c
#define FIO_SORT_TYPE size_t
```

The element type. **Required.**

#### `FIO_SORT_IS_BIGGER`

```c
#ifndef FIO_SORT_IS_BIGGER
#define FIO_SORT_IS_BIGGER(a, b) ((a) > (b))
#endif
```

Must evaluate to `1` if `a > b`, `0` otherwise. Override for custom ordering.

#### `FIO_SORT_SWAP`

```c
#ifndef FIO_SORT_SWAP
#define FIO_SORT_SWAP(a, b)                                                    \
  do {                                                                         \
    FIO_SORT_TYPE tmp__ = (a);                                                 \
    (a) = (b);                                                                 \
    (b) = tmp__;                                                               \
  } while (0)
#endif
```

Swaps two elements. Override if your type needs a custom swap.

#### `FIO_SORT_THRESHOLD`

```c
#ifndef FIO_SORT_THRESHOLD
#define FIO_SORT_THRESHOLD 96
#endif
```

Partition size below which quicksort switches to insert-sort.

### API Functions

#### `FIO_SORT_NAME_sort`

```c
FIO_IFUNC void FIO_NAME(FIO_SORT_NAME, sort)(FIO_SORT_TYPE *array,
                                             size_t count);
```

Sorts `array[0..count-1]` in ascending order. Currently dispatches to quicksort.

#### `FIO_SORT_NAME_qsort`

```c
SFUNC void FIO_NAME(FIO_SORT_NAME, qsort)(FIO_SORT_TYPE *array, size_t count);
```

Non-recursive quicksort with median-of-three pivot selection. Falls back to insert-sort for small partitions.

#### `FIO_SORT_NAME_isort`

```c
SFUNC void FIO_NAME(FIO_SORT_NAME, isort)(FIO_SORT_TYPE *array, size_t count);
```

Insert-sort for small or nearly-sorted arrays. O(n²), so avoid large inputs.

### Example

```c
#define FIO_SORT_NAME int
#define FIO_SORT_TYPE int
#include "fio-stl.h"

int main(void) {
  int numbers[] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
  size_t count = sizeof(numbers) / sizeof(numbers[0]);

  int_sort(numbers, count);

  for (size_t i = 0; i < count; ++i)
    printf("%d ", numbers[i]);
  printf("\n");
  return 0;
}
```

------------------------------------------------------------
