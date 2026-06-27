# Dynamic Arrays — Module 202

Type-safe dynamic arrays, generated from a name macro.

See also: [← 200 types-overview.md](./200 types-overview.md)

---

## Setup

```c
#define FIO_ARRAY_NAME ary         /* required — sets the type and function prefix */
#define FIO_ARRAY_TYPE int         /* optional — element type; default void * */
#include "fio-stl.h"
```

This generates `ary_s` and a full API prefixed `ary_*`.

Throughout this document `ARY` stands for whatever name you chose. `ARY_PTR` is shorthand for `FIO_ARRAY_PTR` (usually `ARY_s *`). All examples use `fio_ary` as the name.

---

## Configuration Macros

Set these **before** `#include`, after `FIO_ARRAY_NAME`.

### Element type

| Macro | Default | Effect |
|---|---|---|
| `FIO_ARRAY_TYPE` | `void *` | The element type stored in the array. |
| `FIO_ARRAY_TYPE_INVALID` | `NULL` / `{0}` | Sentinel value returned for out-of-bounds access. |
| `FIO_ARRAY_TYPE_INVALID_SIMPLE` | `1` if `{0}` | Set to `1` if the invalid element is all-zero bytes. |
| `FIO_ARRAY_TYPE_STR` | — | Shortcut: configures the array for `fio_keystr_s` string elements (sets type, copy, destroy, and compare automatically). |

### Element hooks

| Macro | Default | Effect |
|---|---|---|
| `FIO_ARRAY_TYPE_COPY(dest, src)` | `dest = src` | Called when copying an element into the array. |
| `FIO_ARRAY_TYPE_DESTROY(obj)` | _(nothing)_ | Called when an element is removed or the array is destroyed. |
| `FIO_ARRAY_TYPE_CMP(a, b)` | `a == b` | Called by `find`, `remove2`, etc. |
| `FIO_ARRAY_TYPE_CONCAT_COPY(dest, src)` | same as `COPY` | Used during `concat`; override if concat needs different copy semantics. |
| `FIO_ARRAY_DESTROY_AFTER_COPY` | `1` if both hooks are non-trivial | When set, `DESTROY` is called on an element after it has been copied to an `old` pointer. |

### Memory / growth

| Macro | Default | Effect |
|---|---|---|
| `FIO_ARRAY_PADDING` | `4` | Extra empty slots allocated beyond the requested capacity. |
| `FIO_ARRAY_EXPONENTIAL` | `0` | Set to `1` for exponential (doubling) growth. Linear growth with padding is the default. |
| `FIO_ARRAY_ENABLE_EMBEDDED` | `1` | Store small arrays directly inside the struct (no heap allocation). Values > 1 add extra struct fields to increase the embedded capacity. |

---

## Initialization

```c
ARY_s a = FIO_ARRAY_INIT;      /* stack — zero-initialize */

ARY_s *p = ARY_new();          /* heap — allocate + zero-initialize */
```

`FIO_ARRAY_INIT` expands to `{0}`, so a zero-initializing `memset` also works.

---

## Lifecycle

```c
ARY_s *ARY_new(void);          /* allocate + initialize on the heap */
void   ARY_free(ARY_PTR ary);  /* destroy elements + free the container */
void   ARY_destroy(ARY_PTR ary); /* destroy elements, reset to empty (stack-safe) */
```

Use `ARY_destroy` for stack-allocated arrays. Use `ARY_new` / `ARY_free` for heap.

```c
#define FIO_ARRAY_NAME fio_ary
#define FIO_ARRAY_TYPE int
#include "fio-stl.h"

void example(void) {
    fio_ary_s a = FIO_ARRAY_INIT;
    fio_ary_push(&a, 1);
    fio_ary_push(&a, 2);
    fio_ary_push(&a, 3);
    /* ... use ... */
    fio_ary_destroy(&a);
}
```

---

## State Inspection

```c
uint32_t       ARY_count(ARY_PTR ary);   /* number of elements */
uint32_t       ARY_capa (ARY_PTR ary);   /* current capacity   */
int            ARY_is_embedded(ARY_PTR ary); /* 1 = embedded, 0 = heap alloc, -1 = error */
FIO_ARRAY_TYPE *ARY2ptr(ARY_PTR ary);        /* raw C pointer to the first element */
```

`ARY_is_embedded` reports whether the array currently uses in-struct storage. `ARY2ptr` returns a pointer directly into the backing array. It is valid until the next mutating call. Useful for bulk reads or passing to functions that expect a C array.

---

## Memory Management

```c
/* Reserve capacity for at least `capa` additional elements.
   Negative capa reserves space at the beginning of the array.
   Returns the new capacity. */
uint32_t ARY_reserve(ARY_PTR ary, int64_t capa);

/* Attempt to shrink memory to fit the current element count. */
void ARY_compact(ARY_PTR ary);
```

Growth is linear by default (current count + padding). Exponential growth can be enabled with `FIO_ARRAY_EXPONENTIAL 1`. Always call `ARY_reserve` before large batch inserts to avoid repeated reallocation.

---

## Push / Pop (end of array)

```c
/* Append an element. Returns a pointer to the new element, or NULL on error. */
FIO_ARRAY_TYPE *ARY_push(ARY_PTR ary, FIO_ARRAY_TYPE data);

/* Remove the last element.
   If `old` is not NULL, the removed value is copied there.
   Returns 0 on success, -1 if the array is empty. */
int ARY_pop(ARY_PTR ary, FIO_ARRAY_TYPE *old);
```

```c
fio_ary_push(&a, 42);

int out;
if (!fio_ary_pop(&a, &out))
    printf("popped: %d\n", out);
```

---

## Unshift / Shift (beginning of array)

```c
/* Prepend an element. Returns a pointer to the new element, or NULL on error.
   May trigger memmove. */
FIO_ARRAY_TYPE *ARY_unshift(ARY_PTR ary, FIO_ARRAY_TYPE data);

/* Remove the first element.
   If `old` is not NULL, the removed value is copied there.
   Returns 0 on success, -1 if the array is empty. */
int ARY_shift(ARY_PTR ary, FIO_ARRAY_TYPE *old);
```

Both `push`/`pop` and heap-backed `unshift`/`shift` are O(1) amortized. The implementation maintains headroom at both ends of the heap buffer to keep prepend operations cheap; the tiny embedded path may move elements.

---

## Random Access

```c
/* Set the element at `index`. Grows the array if needed, filling gaps with
   FIO_ARRAY_TYPE_INVALID.
   Negative index counts from the end (-1 == last element).
   If `old` is not NULL, the previous value is copied there before being
   destroyed.
   Returns a pointer to the set element, or NULL on error. */
FIO_ARRAY_TYPE *ARY_set(ARY_PTR ary, int64_t index, FIO_ARRAY_TYPE data,
                        FIO_ARRAY_TYPE *old);

/* Return the element at `index` (no copy performed).
   Negative index counts from the end.
   Returns FIO_ARRAY_TYPE_INVALID if out of bounds. */
FIO_ARRAY_TYPE ARY_get(ARY_PTR ary, int64_t index);
```

```c
fio_ary_set(&a, 5, 99, NULL);    /* extend + set index 5 */
int v = fio_ary_get(&a, -1);     /* last element */
int prev;
fio_ary_set(&a, 0, 7, &prev);   /* replace index 0, capture old value */
```

---

## Concat

```c
/* Append all elements from `src` to `dest`.
   `src` is not modified.
   Returns `dest`, or NULL on allocation failure. */
ARY_PTR ARY_concat(ARY_PTR dest, ARY_PTR src);
```

Uses `FIO_ARRAY_TYPE_CONCAT_COPY` (defaults to `FIO_ARRAY_TYPE_COPY`) for each element, so managed types are properly duplicated.

---

## Search and Removal

```c
/* Find the first occurrence of `data` starting at `start_at`.
   Negative `start_at` seeks backwards (-1 == last element).
   Returns the index, or FIO_ARRAY_NOT_FOUND ((uint32_t)-1). */
uint32_t ARY_find(ARY_PTR ary, FIO_ARRAY_TYPE data, int64_t start_at);

/* Remove element at `index`. All subsequent elements shift left (memmove).
   If `old` is not NULL, the removed value is copied there.
   Returns 0 on success, -1 on error. O(n). */
int ARY_remove(ARY_PTR ary, int64_t index, FIO_ARRAY_TYPE *old);

/* Remove all occurrences of `data`. Elements are compacted in-place.
   Returns the count of removed items. O(n). */
uint32_t ARY_remove2(ARY_PTR ary, FIO_ARRAY_TYPE data);
```

```c
#define FIO_ARRAY_NOT_FOUND ((uint32_t)-1)

uint32_t idx = fio_ary_find(&a, 42, 0);
if (idx != FIO_ARRAY_NOT_FOUND)
    fio_ary_remove(&a, idx, NULL);
```

---

## Iteration

### Callback-based

```c
/** Iteration info passed to the callback. */
typedef struct ARY_each_s {
    ARY_PTR const parent;   /* the array being iterated */
    uint64_t      index;    /* current element index */
    int (*task)(struct ARY_each_s *info); /* the callback; may be updated mid-cycle */
    void         *udata;    /* opaque user data */
    FIO_ARRAY_TYPE value;   /* the current element's value */
    uint64_t      padding;  /* reserved */
} ARY_each_s;

/* Call `task` for each element starting at `start_at`.
   Returning -1 from `task` stops the loop.
   Returns the stop position (elements processed + start). */
uint32_t ARY_each(ARY_PTR ary,
                  int (*task)(ARY_each_s *info),
                  void *udata,
                  int64_t start_at);
```

### Pointer iteration

```c
/* Returns a pointer to the next element.
   Pass NULL for `pos` to get the first element.
   `first` tracks the base pointer across reallocations.
   Returns NULL when the end is reached. */
FIO_ARRAY_TYPE *ARY_each_next(ARY_PTR ary,
                               FIO_ARRAY_TYPE **first,
                               FIO_ARRAY_TYPE *pos);
```

### Loop macro

```c
/* Iterates with a typed `pos` pointer.
   `array_name` must be the literal macro name used when generating the type.
   Avoid structural mutations inside the loop (push/pop/remove may invalidate pos). */
FIO_ARRAY_EACH(array_name, array_ptr, pos) { /* pos is a FIO_ARRAY_TYPE * */ }
```

```c
#define FIO_ARRAY_NAME fio_ary
#define FIO_ARRAY_TYPE int
#include "fio-stl.h"

void print_all(fio_ary_s *a) {
    FIO_ARRAY_EACH(fio_ary, a, pos) {
        printf("%d\n", *pos);
    }
}
```

---

## Embedded Arrays

When `FIO_ARRAY_ENABLE_EMBEDDED` is set (the default), small arrays are stored directly inside the `ARY_s` struct, skipping heap allocation. For `void *` arrays on a 64-bit system, the struct has room for 2 pointers embedded.

Setting `FIO_ARRAY_ENABLE_EMBEDDED` to a value greater than 1 adds extra fields to the struct, increasing embedded capacity at the cost of a larger struct.

`ARY_is_embedded(ary)` returns `1` when the array is currently in embedded mode. `ARY_compact` will re-embed an array if its count fits within the embedded capacity.

---

## Practical Example — String Pointer Array

```c
#define FIO_ARRAY_NAME str_list
#define FIO_ARRAY_TYPE const char *
#include "fio-stl.h"

void example(void) {
    str_list_s list = FIO_ARRAY_INIT;

    str_list_push(&list, "apple");
    str_list_push(&list, "banana");
    str_list_push(&list, "cherry");

    FIO_ARRAY_EACH(str_list, &list, pos) {
        printf("%s\n", *pos);
    }

    str_list_destroy(&list);
}
```

---

## Practical Example — Struct Array with Custom Hooks

```c
typedef struct { int id; float val; } point_s;

#define FIO_ARRAY_NAME          pt_ary
#define FIO_ARRAY_TYPE          point_s
#define FIO_ARRAY_TYPE_CMP(a,b) ((a).id == (b).id)
#include "fio-stl.h"

void example(void) {
    pt_ary_s a = FIO_ARRAY_INIT;

    pt_ary_push(&a, (point_s){.id = 1, .val = 1.5f});
    pt_ary_push(&a, (point_s){.id = 2, .val = 2.5f});

    point_s key = {.id = 1};
    uint32_t idx = pt_ary_find(&a, key, 0);
    if (idx != FIO_ARRAY_NOT_FOUND)
        printf("found at %u\n", idx);

    pt_ary_destroy(&a);
}
```

---

## Performance Notes

- **Access / set**: O(1).
- **Iteration**: excellent cache locality.
- **push / pop**: O(1) amortized.
- **unshift / shift**: O(1) amortized for heap-backed arrays; the tiny embedded path may move elements.
- **find**: O(n) worst case.
- **remove / remove2**: O(n) — involve `memmove`.
- Growth is **linear** by default (count + `FIO_ARRAY_PADDING`). Use `FIO_ARRAY_EXPONENTIAL 1` or call `ARY_reserve` up-front to avoid repeated reallocations on large batch inserts.
- Capacity is limited to **32 bits** (max ~4 billion elements).

---

See [→ 200 types-overview.md](./200 types-overview.md) for the shared define-include-use pattern and how to combine arrays with strings, maps, and reference counters.
