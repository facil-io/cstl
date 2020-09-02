## Dynamic Arrays

```c
#define FIO_ARRAY_NAME str_ary
#define FIO_ARRAY_TYPE char *
#define FIO_ARRAY_TYPE_CMP(a,b) (!strcmp((a),(b)))
#include "fio-stl.h"
```

Dynamic arrays are extremely common and useful data structures.

In essence, Arrays are blocks of memory that contain all their elements "in a row". They grow (or shrink) as more items are added (or removed).

Items are accessed using a numerical `index` indicating the element's position within the array.

Indexes are zero based (first element == 0).

### Dynamic Array Performance

Seeking time is an extremely fast O(1). Arrays are also very fast to iterate since they enjoy high memory locality.

Adding and editing items is also a very fast O(1), especially if enough memory was previously reserved. Otherwise, memory allocation and copying will slow performance.

However, arrays suffer from slow find operations. Find has a worst case scenario O(n) cost.

They also suffer from slow item removal (except, in our case, for `pop` / `unshift` operations), since middle-element removal requires memory copying when fixing the "hole" made in the array.

A common solution is to reserve a value for "empty" elements and `set` the element's value instead of `remove` the element.

**Note**: unlike some dynamic array implementations, this STL implementation doesn't grow exponentially. Using the `ARY_reserve` function is highly encouraged for performance.


### Dynamic Array Overview

To create a dynamic array type, define the type name using the `FIO_ARRAY_NAME` macro. i.e.:

```c
#define FIO_ARRAY_NAME int_ary
```

Next (usually), define the `FIO_ARRAY_TYPE` macro with the element type. The default element type is `void *`. For example:

```c
#define FIO_ARRAY_TYPE int
```

For complex types, define any (or all) of the following macros:

```c
// set to adjust element copying 
#define FIO_ARRAY_TYPE_COPY(dest, src)  
// set for element cleanup 
#define FIO_ARRAY_TYPE_DESTROY(obj)     
// set to adjust element comparison 
#define FIO_ARRAY_TYPE_CMP(a, b)        
// to be returned when `index` is out of bounds / holes 
#define FIO_ARRAY_TYPE_INVALID 0 
// set ONLY if the invalid element is all zero bytes 
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1     
// should the object be destroyed when copied to an `old` pointer?
#define FIO_ARRAY_DESTROY_AFTER_COPY 1 
// when array memory grows, how many extra "spaces" should be allocated?
#define FIO_ARRAY_PADDING 4 
// should the array growth be exponential? (ignores FIO_ARRAY_PADDING)
#define FIO_ARRAY_EXPONENTIAL 0 
// optimizes small arrays (mostly tuplets and single item arrays).
// note: values larger than 1 add a memory allocation cost to the array container
#define FIO_ARRAY_ENABLE_EMBEDDED 1
```

To create the type and helper functions, include the Simple Template Library header.

For example:

```c
typedef struct {
  int i;
  float f;
} foo_s;

#define FIO_ARRAY_NAME ary
#define FIO_ARRAY_TYPE foo_s
#define FIO_ARRAY_TYPE_CMP(a,b) (a.i == b.i && a.f == b.f)
#include "fio-stl.h"

void example(void) {
  ary_s a = FIO_ARRAY_INIT;
  foo_s *p = ary_push(&a, (foo_s){.i = 42});
  FIO_ARRAY_EACH(&a, pos) { // pos will be a pointer to the element
    fprintf(stderr, "* [%zu]: %p : %d\n", (size_t)(pos - ary_to_a(&a)), pos->i);
  }
  ary_destroy(&a);
}
```

### Dynamic Arrays - API

#### The Array Type (`ARY_s`)

```c
typedef struct {
  FIO_ARRAY_TYPE *ary;
  uint32_t capa;
  uint32_t start;
  uint32_t end;
} FIO_NAME(FIO_ARRAY_NAME, s); /* ARY_s in these docs */
```

The array type should be considered opaque. Use the helper functions to updated the array's state when possible, even though the array's data is easily understood and could be manually adjusted as needed.

#### `FIO_ARRAY_INIT`

````c
#define FIO_ARRAY_INIT  {0}
````

This macro initializes an uninitialized array object.

#### `ARY_destroy`

````c
void ARY_destroy(ARY_s * ary);
````

Destroys any objects stored in the array and frees the internal state.

#### `ARY_new`

````c
ARY_s * ARY_new(void);
````

Allocates a new array object on the heap and initializes it's memory.

#### `ARY_free`

````c
void ARY_free(ARY_s * ary);
````

Frees an array's internal data AND it's container!

#### `ARY_count`

````c
uint32_t ARY_count(ARY_s * ary);
````

Returns the number of elements in the Array.

#### `ARY_capa`

````c
uint32_t ARY_capa(ARY_s * ary);
````

Returns the current, temporary, array capacity (it's dynamic).

#### `ARY_reserve`

```c
uint32_t ARY_reserve(ARY_s * ary, int32_t capa);
```

Reserves a minimal capacity for the array.

If `capa` is negative, new memory will be allocated at the beginning of the array rather then it's end.

Returns the array's new capacity.

**Note**: the reserved capacity includes existing data / capacity. If the requested reserved capacity is equal (or less) then the existing capacity, nothing will be done.

#### `ARY_concat`

```c
ARY_s * ARY_concat(ARY_s * dest, ARY_s * src);
```

Adds all the items in the `src` Array to the end of the `dest` Array.

The `src` Array remain untouched.

Always returns the destination array (`dest`).

#### `ARY_set`

```c
FIO_ARRAY_TYPE * ARY_set(ARY_s * ary,
                       int32_t index,
                       FIO_ARRAY_TYPE data,
                       FIO_ARRAY_TYPE *old);
```

Sets `index` to the value in `data`.

If `index` is negative, it will be counted from the end of the Array (-1 == last element).

If `old` isn't NULL, the existing data will be copied to the location pointed to by `old` before the copy in the Array is destroyed.

Returns a pointer to the new object, or NULL on error.

#### `ARY_get`

```c
FIO_ARRAY_TYPE ARY_get(ARY_s * ary, int32_t index);
```

Returns the value located at `index` (no copying is performed).

If `index` is negative, it will be counted from the end of the Array (-1 == last element).

**Reminder**: indexes are zero based (first element == 0).

#### `ARY_find`

```c
int32_t ARY_find(ARY_s * ary, FIO_ARRAY_TYPE data, int32_t start_at);
```

Returns the index of the object or -1 if the object wasn't found.

If `start_at` is negative (i.e., -1), than seeking will be performed in reverse, where -1 == last index (-2 == second to last, etc').

#### `ARY_remove`
```c
int ARY_remove(ARY_s * ary, int32_t index, FIO_ARRAY_TYPE *old);
```

Removes an object from the array, MOVING all the other objects to prevent "holes" in the data.

If `old` is set, the data is copied to the location pointed to by `old` before the data in the array is destroyed.

Returns 0 on success and -1 on error.

This action is O(n) where n in the length of the array. It could get expensive.

#### `ARY_remove2`

```c
uint32_t ARY_remove2(ARY_S * ary, FIO_ARRAY_TYPE data);
```

Removes all occurrences of an object from the array (if any), MOVING all the existing objects to prevent "holes" in the data.

Returns the number of items removed.

This action is O(n) where n in the length of the array. It could get expensive.

#### `ARY_compact`
```c
void ARY_compact(ARY_s * ary);
```

Attempts to lower the array's memory consumption.

#### `ARY_to_a`

```c
FIO_ARRAY_TYPE * ARY_to_a(ARY_s * ary);
```

Returns a pointer to the C array containing the objects.

#### `ARY_push`

```c
FIO_ARRAY_TYPE * ARY_push(ARY_s * ary, FIO_ARRAY_TYPE data);
```

 Pushes an object to the end of the Array. Returns a pointer to the new object or NULL on error.

#### `ARY_pop`

```c
int ARY_pop(ARY_s * ary, FIO_ARRAY_TYPE *old);
```

Removes an object from the end of the Array.

If `old` is set, the data is copied to the location pointed to by `old` before the data in the array is destroyed.

Returns -1 on error (Array is empty) and 0 on success.

#### `ARY_unshift`

```c
FIO_ARRAY_TYPE *ARY_unshift(ARY_s * ary, FIO_ARRAY_TYPE data);
```

Unshifts an object to the beginning of the Array. Returns a pointer to the new object or NULL on error.

This could be expensive, causing `memmove`.

#### `ARY_shift`

```c
int ARY_shift(ARY_s * ary, FIO_ARRAY_TYPE *old);
```

Removes an object from the beginning of the Array.

If `old` is set, the data is copied to the location pointed to by `old` before the data in the array is destroyed.

Returns -1 on error (Array is empty) and 0 on success.

#### `ARY_each`

```c
uint32_t ARY_each(ARY_s * ary, int32_t start_at,
                               int (*task)(FIO_ARRAY_TYPE obj, void *arg),
                               void *arg);
```

Iteration using a callback for each entry in the array.

The callback task function must accept an the entry data as well as an opaque user pointer.

If the callback returns -1, the loop is broken. Any other value is ignored.

Returns the relative "stop" position (number of items processed + starting point).

#### `FIO_ARRAY_EACH`

```c
#define FIO_ARRAY_EACH(array, pos)                                               \
  if ((array)->ary)                                                            \
    for (__typeof__((array)->ary) start__tmp__ = (array)->ary,                 \
                                  pos = ((array)->ary + (array)->start);       \
         pos < (array)->ary + (array)->end;                                    \
         (pos = (array)->ary + (pos - start__tmp__) + 1),                      \
                                  (start__tmp__ = (array)->ary))
```

Iterates through the list using a `for` loop.

Access the object with the pointer `pos`. The `pos` variable can be named however you please.

It's possible to edit elements within the loop, but avoid editing the array itself (adding / removing elements). Although I hope it's possible, I wouldn't count on it and it could result in items being skipped or unending loops.

-------------------------------------------------------------------------------
