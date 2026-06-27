# Index Mapped Array (iMap)

```c
#define FIO_IMAP_CORE
#include "fio-stl.h"
```

A macro-generated hash-map-backed array. Keeps insertion order in a dense array while the index map provides near-O(1) lookups. Mostly used internally by other STL modules; for public map needs see [`FIO_MAP_NAME`](./210%20map.md).

**Note:** iMap does not manage object lifetimes. Clean up stored resources before calling destroy.

### Configuration Macros

#### `FIO_TYPEDEF_IMAP_REALLOC`

```c
#ifndef FIO_TYPEDEF_IMAP_REALLOC
#define FIO_TYPEDEF_IMAP_REALLOC FIO_MEM_REALLOC
#endif
```

Allocator used for iMap growth. Defaults to `FIO_MEM_REALLOC`.

#### `FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE`

```c
#ifndef FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE FIO_MEM_REALLOC_IS_SAFE
#endif
```

Set if the realloc zeroes new memory; otherwise iMap zeroes it explicitly.

#### `FIO_TYPEDEF_IMAP_FREE`

```c
#ifndef FIO_TYPEDEF_IMAP_FREE
#define FIO_TYPEDEF_IMAP_FREE FIO_MEM_FREE
#endif
```

Deallocator used by `array_name_destroy`.

### Helper Macros

#### `FIO_IMAP_ALWAYS_VALID`

```c
#define FIO_IMAP_ALWAYS_VALID(o) (1)
```

Validity helper that treats every object as valid.

#### `FIO_IMAP_VALID_NON_ZERO`

```c
#define FIO_IMAP_VALID_NON_ZERO(o) (!!((o)[0]))
```

Validity helper that treats an object as valid if its first element is non-zero.

#### `FIO_IMAP_ALWAYS_CMP_TRUE` / `FIO_IMAP_ALWAYS_CMP_FALSE`

```c
#define FIO_IMAP_ALWAYS_CMP_TRUE(a, b) (1)
#define FIO_IMAP_ALWAYS_CMP_FALSE(a, b) (0)
```

Comparison helpers that always return true or false.

#### `FIO_IMAP_SIMPLE_CMP`

```c
#define FIO_IMAP_SIMPLE_CMP(a, b) ((a)[0] == (b)[0])
```

Compares two objects by their first element.

#### `FIO_IMAP_EACH`

```c
#define FIO_IMAP_EACH(array_name, map_ptr, i)                          \
  for (size_t i = 0; i < (map_ptr)->w; ++i)                             \
    if (!FIO_NAME(array_name, is_valid)((map_ptr)->ary + i))            \
      continue;                                                        \
    else
```

Iterates valid elements in insertion order. Use it as a loop header:

```c
FIO_IMAP_EACH(my_map, &map, i) {
  printf("%llu\n", (unsigned long long)map.ary[i].key);
}
```

### Type Definition Macro

#### `FIO_TYPEDEF_IMAP_ARRAY`

```c
#define FIO_TYPEDEF_IMAP_ARRAY(array_name,                            \
                               array_type,                            \
                               imap_type,                             \
                               hash_fn,                               \
                               cmp_fn,                                \
                               is_valid_fn)
```

Generates the iMap container type and functions prefixed with `array_name`. The callbacks take **pointers** to elements:

- `hash_fn(pobj)` returns an `imap_type` hash.
- `cmp_fn(a_ptr, b_ptr)` returns non-zero on match.
- `is_valid_fn(pobj)` returns non-zero if the element is valid.

Reserved index-map values: `0` means empty, `~0` means freed.

### Generated Types

#### `array_name_s`

```c
typedef struct {
  array_type *ary;
  imap_type count;
  imap_type w;
  uint32_t capa_bits;
} array_name_s;
```

- `ary` — dense array of elements.
- `count` — number of valid (non-removed) elements.
- `w` — next write position / logical size.
- `capa_bits` — log2 of capacity; actual capacity is `1 << capa_bits`.

#### `array_name_seeker_s`

```c
typedef struct {
  imap_type pos;
  imap_type ipos;
  imap_type set_val;
} array_name_seeker_s;
```

- `pos` — array index, or `w` if not found.
- `ipos` — index-map slot, or `~0` if no room.
- `set_val` — value to write into the index map on insert.

### Generated Functions

#### `array_name_is_valid`

```c
FIO_IFUNC int array_name_is_valid(array_type *pobj);
```

Wraps the `is_valid_fn` supplied to the macro.

#### `array_name_capa`

```c
FIO_IFUNC size_t array_name_capa(array_name_s *a);
```

Returns `1 << a->capa_bits`, or `0` if not allocated.

#### `array_name_imap`

```c
FIO_IFUNC imap_type *array_name_imap(array_name_s *a);
```

Returns a pointer to the index map stored immediately after the array.

#### `array_name_destroy`

```c
FIO_IFUNC void array_name_destroy(array_name_s *a);
```

Frees the array + index map and zeros the container. Does not run destructors on elements.

#### `array_name_seek`

```c
FIO_SFUNC array_name_seeker_s array_name_seek(array_name_s *a,
                                              array_type *pobj);
```

Finds an element or the place to insert it.

#### `array_name_reserve`

```c
FIO_IFUNC int array_name_reserve(array_name_s *a, imap_type min);
```

Grows the backing storage to at least `min` slots. Returns `0` on success, `-1` on failure.

#### `array_name_rehash`

```c
FIO_IFUNC int array_name_rehash(array_name_s *a);
```

Rebuilds the index map from the current array contents. Call after sorting or other position-changing operations. Returns `0` on success, `-1` on failure.

#### `array_name_set`

```c
FIO_IFUNC array_type *array_name_set(array_name_s *a,
                                     array_type obj,
                                     int overwrite);
```

Inserts or updates an element. If `overwrite` is zero and the key already exists, returns the existing element. Returns a pointer to the stored element or `NULL` on failure.

#### `array_name_get`

```c
FIO_IFUNC array_type *array_name_get(array_name_s *a, array_type obj);
```

Looks up an element. Returns a pointer to the match or `NULL`.

#### `array_name_remove`

```c
FIO_IFUNC int array_name_remove(array_name_s *a, array_type obj);
```

Zeros the matching element and marks its index-map slot as freed. Returns `0` on success, `-1` if not found.

### Internal Seeker Types

#### `fio___imapN_seeker_s`

```c
typedef struct {
  uintN_t pos;
  uintN_t ipos;
  uintN_t set_val;
  bool is_valid;
} fio___imapN_seeker_s;
```

Where `N` is `8`, `16`, `32`, or `64`. Low-level seeker used by other STL internals.

#### `fio___imapN_seek`

```c
FIO_SFUNC fio___imapN_seeker_s fio___imapN_seek(
    void *ary,
    uintN_t *imap,
    const uintN_t capa_bits,
    void *pobj,
    uintN_t hash,
    bool cmp_fn(void *arry, void *obj, uintN_t indx),
    const size_t max_attempts);
```

Low-level seek through an index map. Generally used internally.

#### `fio___imapN_set`

```c
FIO_IFUNC void fio___imapN_set(uintN_t *imap,
                               uintN_t ipos,
                               uintN_t set_val);
```

Writes `set_val` into `imap[ipos]`.

### Example

```c
#define FIO_IMAP_CORE
#include "fio-stl.h"

typedef struct { uint64_t key; int value; } kv_s;

static uint64_t kv_hash(kv_s *p)   { return fio_risky_num(p->key, 0); }
static int      kv_cmp(kv_s *a, kv_s *b) { return a->key == b->key; }
static int      kv_valid(kv_s *p)  { return p->key != 0; }

FIO_TYPEDEF_IMAP_ARRAY(kv, kv_s, uint32_t, kv_hash, kv_cmp, kv_valid)

int main(void) {
  kv_s map = {0};
  kv_set(&map, (kv_s){.key = 1, .value = 100}, 1);
  kv_set(&map, (kv_s){.key = 2, .value = 200}, 1);

  kv_s *found = kv_get(&map, (kv_s){.key = 2});
  if (found) printf("value: %d\n", found->value);

  FIO_IMAP_EACH(kv, &map, i) {
    printf("%llu -> %d\n",
           (unsigned long long)map.ary[i].key, map.ary[i].value);
  }

  kv_remove(&map, (kv_s){.key = 2});
  kv_destroy(&map);
  return 0;
}
```

------------------------------------------------------------
