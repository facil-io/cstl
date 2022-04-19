## Quick Sort and Insert Sort

```c
#define FIO_SORT
#include "fio-stl.h"
```

If the `FIO_SORT` is defined (and named), the following functions will be defined.

This can be performed multiple times for multiple types.

### Sort Settings

The following macros define the behavior of the sorting algorithm.

#### `FIO_SORT`

```c
#define FIO_SORT num // will produce function names such as num_sort(...)
```

The prefix used for naming the sorting functions.

**Note**: a name is required.

#### `FIO_SORT_TYPE`

```c
// i.e.
#define FIO_SORT_TYPE size_t
```

The type of the array members to be sorted.

**Note**: this macro **MUST** be defined.

#### `FIO_SORT_IS_BIGGER`

```c
#define FIO_SORT_IS_BIGGER(a, b) ((a) > (b))
```

Equality test - **must** evaluate as 1 if a > b (zero if equal or smaller).

#### `FIO_SORT_SWAP`

```c
#define FIO_SORT_SWAP(a, b)                                                    \
  do {                                                                         \
    FIO_SORT_TYPE tmp__ = (a);                                                 \
    (a) = (b);                                                                 \
    (b) = tmp__;                                                               \
  } while (0)
```

Swaps array members. Usually there is no need to override the default macro.

#### `FIO_SORT_THRESHOLD`

```c
#define FIO_SORT_THRESHOLD 96
```

The threshold below which quick-sort delegates to insert sort. Usually there is no need to override the default macro.


### Sorting API

#### `FIO_SORT_sort`

```c
void FIO_SORT_sort(FIO_SORT_TYPE *array, size_t count);
```

Sorts the first `count` members of `array`.

Currently this wraps the [`FIO_SORT_qsort`](#fio_sort_qsort) function.

#### `FIO_SORT_qsort`

```c
void FIO_SORT_qsort(FIO_SORT_TYPE *array, size_t count);
```

Sorts the first `count` members of `array` using quick-sort.


#### `FIO_SORT_isort`

```c
void FIO_SORT_isort(FIO_SORT_TYPE *array, size_t count);
```

Sorts the first `count` members of `array` using insert-sort.

Use only with small arrays (unless you are a fan of inefficiency).

-------------------------------------------------------------------------------
