## Hash Tables and Maps

```c
/* Create a binary safe String type for Strings that aren't mutated often */
#define FIO_STR_SMALL str
#include "fio-stl.h"

/* Set the properties for the key-value Hash Map type called `dict_s` */
#define FIO_UMAP_NAME                dict
#define FIO_MAP_TYPE                 str_s
#define FIO_MAP_TYPE_COPY(dest, src) str_init_copy2(&(dest), &(src))
#define FIO_MAP_TYPE_DESTROY(k)      str_destroy(&k)
#define FIO_MAP_TYPE_CMP(a, b)       str_is_eq(&(a), &(b))
#define FIO_MAP_KEY                  FIO_MAP_TYPE
#define FIO_MAP_KEY_COPY             FIO_MAP_TYPE_COPY
#define FIO_MAP_KEY_DESTROY          FIO_MAP_TYPE_DESTROY
#define FIO_MAP_KEY_CMP              FIO_MAP_TYPE_CMP
#include "fio-stl.h"
/** set helper for consistent hash values */
FIO_IFUNC str_s *dict_set2(dict_s *m, str_s key, str_s obj) {
  return dict_set_ptr(m, str_hash(&key, (uint64_t)m), key, obj, NULL, 1);
}
/** get helper for consistent hash values */
FIO_IFUNC str_s *dict_get2(dict_s *m, str_s key) {
  return dict_get_ptr(m, str_hash(&key, (uint64_t)m), key);
}
```

Hash maps and sets are extremely useful and common mapping / dictionary primitives, also sometimes known as "dictionary".

Hash maps use both a `hash` and a `key` to identify a `value`. The `hash` value is calculated by feeding the key's data to a hash function (such as Risky Hash or SipHash).

A hash map without a `key` is known as a Set or a Bag. It uses only a `hash` (often calculated using `value`) to identify the `value` in the Set, sometimes requiring a `value` equality test as well. This approach often promises a collection of unique values (no duplicate values).

Some map implementations support a FIFO limited storage, which could be used for naive limited-space caching (though caching solutions may require a more complex data-storage that's slower).

### Ordered Maps, Unordered Maps, Indexing and Performance

The facil.io library offers both ordered and unordered maps. Unordered maps are often faster and use less memory. If iteration is performed, ordered maps might be better.

Indexing the map allows LRU (least recently used) eviction, but comes at a performance cost in both memory (due to the extra data per object) and speed (due to out of order memory access and increased cache misses).

Ordered maps are constructed using an ordered Array + an index map that uses 4 or 8 bytes per array index.

Unordered maps are constructed using an unordered Array + an index map that uses 1 byte per array index.

Indexing is performed using a linked list that uses 4 or 8 byte index values instead of pointers.

In addition, each value stores a copy of the hash data, so hash data doesn't need to be recomputed.

The map implementations have protection features against too many full collisions or non-random hashes. When the map detects a possible "attack", it will start overwriting existing data instead of trying to resolve collisions. This can be adjusted using the `FIO_MAP_MAX_FULL_COLLISIONS` macro.

### Map Overview 

To create a map, define `FIO_MAP_NAME` or `FIO_UMAP_NAME` (unordered).

To create a hash map (rather then a set), also define `FIO_MAP_KEY` (containing the key's type).

To create an unordered map either use `FIO_UMAP_NAME` or define `FIO_MAP_ORDERED`.

Other helpful macros to define might include:

- `FIO_MAP_TYPE`, which defaults to `void *`
- `FIO_MAP_TYPE_INVALID`, which defaults to `((FIO_MAP_TYPE){0})`
- `FIO_MAP_TYPE_COPY(dest, src)`, which defaults to `(dest) = (src)`
- `FIO_MAP_TYPE_DESTROY(obj)`
- `FIO_MAP_TYPE_CMP(a, b)`, which defaults to `1`
- `FIO_MAP_KEY`
- `FIO_MAP_KEY_INVALID`
- `FIO_MAP_KEY_COPY(dest, src)`
- `FIO_MAP_KEY_DESTROY(obj)`
- `FIO_MAP_KEY_CMP(a, b)`
- `FIO_MAP_MAX_FULL_COLLISIONS`, which defaults to `22`


- `FIO_MAP_DESTROY_AFTER_COPY`, uses "smart" defaults to decide if to destroy an object after it was copied (when using `set` / `remove` / `pop` with a pointer to contain `old` object).
- `FIO_MAP_TYPE_DISCARD(obj)`, handles discarded element data (i.e., insert without overwrite in a Set).
- `FIO_MAP_KEY_DISCARD(obj)`, handles discarded element data (i.e., when overwriting an existing value in a hash map).
- `FIO_MAP_MAX_ELEMENTS`, the maximum number of elements allowed before removing old data (FIFO).
- `FIO_MAP_EVICT_LRU`, if set to true (1), the `evict` method and the `FIO_MAP_MAX_ELEMENTS` macro will evict members based on the Least Recently Used object.
- `FIO_MAP_MAX_SEEK` , the maximum number of bins to rotate when (partial/full) collisions occur. Limited to a maximum of 255 and should be higher than `FIO_MAP_MAX_FULL_COLLISIONS/4`, by default `17`.

- `FIO_MAP_HASH`, defaults to `uint64_t`, may be set to `uint32_t` if hash data is 32 bit wide.
- `FIO_MAP_HASH_FN`, replace the cached `hash` for unordered maps with a re-hash calculation. This is good if the caching is dirt cheap but can only be used with unordered maps since the ordered maps double the cached hash with a "hole" marker.
- `FIO_MAP_BIG`, if defined, the maximum theoretical capacity increases to `(1 << 64) -1`.
To limit the number of elements in a map (FIFO, ignoring last access time), allowing it to behave similarly to a simple caching primitive, define: `FIO_MAP_MAX_ELEMENTS`.

If `FIO_MAP_MAX_ELEMENTS` is `0`, then the theoretical maximum number of elements should be: `(1 << 32) - 1`. In practice, the safe limit should be calculated as `1 << 31` or `1 << 30`. The same is true for `FIO_MAP_BIG`, only relative to 64 bits.

Example:

```c
/* TODO */
/* We'll use small immutable binary strings as keys */
#define FIO_STR_SMALL str
#include "fio-stl.h"

#define FIO_MAP_NAME number_map /* results in the type: number_map_s */
#define FIO_MAP_TYPE size_t
#define FIO_MAP_KEY str_s
#define FIO_MAP_KEY_DESTROY(k) key_destroy(&k)
#define FIO_MAP_KEY_DISCARD(k) key_destroy(&k)
#include "fio-stl.h"
```

### Hash Map / Set - API (initialization)

#### `MAP_new`

```c
FIO_MAP_PTR MAP_new(void);
```

Allocates a new map on the heap.

#### `MAP_free`

```c
void MAP_free(MAP_PTR m);
```

Frees a map that was allocated on the heap.

#### `FIO_MAP_INIT`

```c
#define FIO_MAP_INIT { .map = NULL }
```

This macro initializes a map object - often used for maps placed on the stack.

#### `MAP_destroy`

```c
void MAP_destroy(MAP_PTR m);
```

Destroys the map's internal data and re-initializes it.


### Hash Map - API (hash map only)

#### `MAP_get` (hash map)

```c
FIO_MAP_TYPE MAP_get(FIO_MAP_PTR m,
                     FIO_MAP_HASH hash,
                     FIO_MAP_KEY key);
```
Returns the object in the hash map (if any) or FIO_MAP_TYPE_INVALID.

#### `MAP_get_ptr` (hash map)

```c
FIO_MAP_TYPE *MAP_get_ptr(FIO_MAP_PTR m,
                          FIO_MAP_HASH hash,
                          FIO_MAP_KEY key);
```

Returns a pointer to the object in the hash map (if any) or NULL.

#### `MAP_set` (hash map)

```c
FIO_MAP_TYPE MAP_set(FIO_MAP_PTR m,
                     FIO_MAP_HASH hash,
                     FIO_MAP_KEY key,
                     FIO_MAP_TYPE obj,
                     FIO_MAP_TYPE *old);
```


Inserts an object to the hash map, returning the new object.

If `old` is given, existing data will be copied to that location.

#### `MAP_set_ptr` (hash map)

```c
FIO_MAP_TYPE *MAP_set(FIO_MAP_PTR m,
                      FIO_MAP_HASH hash,
                      FIO_MAP_KEY key,
                      FIO_MAP_TYPE obj,
                      FIO_MAP_TYPE *old,
                      uint8_t overwrite);
```


Inserts an object to the hash map, returning the new object.

If `old` is given, existing data will be copied to that location unless `overwrite` is false (in which case, old data isn't overwritten).

#### `MAP_remove` (hash map)

```c
int MAP_remove(FIO_MAP_PTR m,
               FIO_MAP_HASH hash,
               FIO_MAP_KEY key,
               FIO_MAP_TYPE *old);
```

Removes an object from the hash map.

If `old` is given, existing data will be copied to that location.

Returns 0 on success or -1 if the object couldn't be found.

### Set - API (set only)

#### `MAP_get` (set)

```c
FIO_MAP_TYPE MAP_get(FIO_MAP_PTR m,
                     FIO_MAP_HASH hash,
                     FIO_MAP_TYPE obj);
```

Returns the object in the hash map (if any) or `FIO_MAP_TYPE_INVALID`.

#### `MAP_get_ptr` (set)

```c
FIO_MAP_TYPE *MAP_get_ptr(FIO_MAP_PTR m,
                          FIO_MAP_HASH hash,
                          FIO_MAP_TYPE obj);
```

Returns a pointer to the object in the hash map (if any) or NULL.

#### `set_if_missing` (set)

```c
FIO_MAP_TYPE set_if_missing(FIO_MAP_PTR m,
                            FIO_MAP_HASH hash,
                            FIO_MAP_TYPE obj);
```

Inserts an object to the hash map, returning the existing or new object.

If `old` is given, existing data will be copied to that location.

#### `MAP_set` (set)

```c
FIO_MAP_TYPE MAP_set(FIO_MAP_PTR m,
                     FIO_MAP_HASH hash,
                     FIO_MAP_TYPE obj,
                     FIO_MAP_TYPE *old);
```

Inserts an object to the hash map, returning the new object.

If `old` is given, existing data will be copied to that location.

#### `MAP_set_ptr` (set)

```c
FIO_MAP_TYPE *MAP_set(FIO_MAP_PTR m,
                      FIO_MAP_HASH hash,
                      FIO_MAP_TYPE obj,
                      FIO_MAP_TYPE *old);
```

Inserts an object to the hash map, returning the new object.

If `old` is given, existing data will be copied to that location unless `overwrite` is false (in which case, old data isn't overwritten).

#### `MAP_remove` (set)

```c
int MAP_remove(FIO_MAP_PTR m, FIO_MAP_HASH hash,
               FIO_MAP_TYPE obj, FIO_MAP_TYPE *old);
```

Removes an object from the hash map.

If `old` is given, existing data will be copied to that location.

Returns 0 on success or -1 if the object couldn't be found.

#### `MAP_clear`

```c
void MAP_clear(MAP_PTR m);
```

Removes all elements from the Map without freeing the memory used.

Similar to calling:

```c
size_t capa_was = MAP_capa(m);
MAP_destroy(m);
MAP_reserve(m, capa_was);
```

#### `MAP_evict` (set)

```c
int MAP_evict(FIO_MAP_PTR m, size_t number_of_elements);
```

Evicts (removed) `number_of_elements` from the Map.

Eviction is FIFO based (First In First Out) unless FIO_MAP_EVICT_LRU is defined, in which case the Least Recently Used element will be evicted.

Returns 0 on success or -1 on error (i.e., element number bigger than existing element count).

### Hash Map / Set - API (common)

#### `MAP_count`

```c
uintptr_t MAP_count(FIO_MAP_PTR m);
```

Returns the number of objects in the map.

#### `MAP_capa`

```c
uintptr_t MAP_capa(FIO_MAP_PTR m);
```

Returns the current map's theoretical capacity.

#### `MAP_reserve`

```c
uintptr_t MAP_reserve(FIO_MAP_PTR m, uint32_t capa);
```

Reserves a minimal capacity for the hash map.

#### `MAP_compact`

```c
void MAP_compact(FIO_MAP_PTR m);
```

Attempts to lower the map's memory consumption.

#### `MAP_rehash`

```c
int MAP_rehash(FIO_MAP_PTR m);
```

Rehashes the Hash Map / Set. Usually this is performed automatically, no need to call the function.

#### `MAP_each_next`

```c
MAP_each_s * MAP_each_next(FIO_MAP_PTR m, MAP_each_s ** first, MAP_each_s * pos);
```

Returns a pointer to the (next) object's information in the map.

To access the object information, use:

```c
MAP_each_s * pos = MAP_each_next(map, NULL);
```

- `i->hash` to access the hash value.

- `i->obj` to access the object's data.

   For Hash Maps, use `i->obj.key` and `i->obj.value`.

Returns the first object if `pos == NULL` and there are objects in the map.

Returns the next object if `pos` is valid.

Returns NULL if `pos` was the last object or no object exist.

**Note**:

If `pos` is invalid or `NULL`, a pointer to the first object will be returned.

The value of `first` is required and used to revalidate `pos` in cases where object insertion or memory changes occurred while iterating.

The value of `first` is set automatically by the function. Manually changing this value may result in unexpected behavior such as the loop restarting, terminating early, skipping some objects, reiterating some objects or exploding the screen.

#### `MAP_each`

```c
uint32_t MAP_each(FIO_MAP_PTR m,
                  int32_t start_at,
                  int (*task)(FIO_MAP_TYPE obj, void *arg),
                  void *arg);
```

Iteration using a callback for each element in the map.

The callback task function must accept an element variable as well as an opaque user pointer.

If the callback returns -1, the loop is broken. Any other value is ignored.

Returns the relative "stop" position, i.e., the number of items processed + the starting point.

#### `MAP_each_get_key`

```c
FIO_MAP_KEY MAP_each_get_key(void);
```

Returns the current `key` within an `each` task.

Only available within an `each` loop.

_Note: For sets, returns the hash value, for hash maps, returns the key value._

#### `FIO_MAP_EACH`

```c
#define FIO_MAP_EACH(map_type, map_p, pos)                                    \
  for (FIO_NAME(map_type, each_s) *pos =                                       \
           FIO_NAME(map_type, each_next)(map_p, NULL);                         \
       pos;                                                                    \
       pos = FIO_NAME(map_type, each_next)(map_p, pos))
```

A macro for a `for` loop that iterates over all the Map's objects (in order).

Use this macro for small Hash Maps / Sets.

- `map_type` is the Map's type name/function prefix, same as FIO_MAP_NAME.

- `map_p` is a pointer to the Hash Map / Set variable.

- `pos` is a temporary variable name to be created for iteration. This
   variable may SHADOW external variables, be aware.

To access the object information, use:

- `pos->hash` to access the hash value.

- `pos->obj` to access the object's data.

   For Hash Maps, use `pos->obj.key` and `pos->obj.value`.

-------------------------------------------------------------------------------
