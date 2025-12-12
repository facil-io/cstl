## iMap - Index Mapped Array (Hash Map - Array Combo)

```c
#define FIO_IMAP_CORE
#include "fio-stl.h"
```

The iMap module provides an indexed array data structure that combines the benefits of both arrays and hash maps. It maintains insertion order while providing fast O(1) lookups through an index map.

This is primarily **used internally** by other facil.io modules to minimize dependencies and avoid nested inclusions. For most use cases, it is recommended to use the `FIO_MAP_NAME` macro instead.

**Key Features:**
- Maintains insertion order for iteration
- Fast hash-based lookups
- Combines array storage with hash map indexing
- Suitable when both ordered iteration and random access are needed

**Note**: there is no automatic memory management when objects are removed or the iMap is destroyed. You must handle cleanup of stored objects manually.

-------------------------------------------------------------------------------

### Configuration Macros

#### `FIO_TYPEDEF_IMAP_REALLOC`

```c
#define FIO_TYPEDEF_IMAP_REALLOC FIO_MEM_REALLOC
```

Defines the reallocation function used by iMap. Defaults to `FIO_MEM_REALLOC`.

#### `FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE`

```c
#define FIO_TYPEDEF_IMAP_REALLOC_IS_SAFE FIO_MEM_REALLOC_IS_SAFE
```

Indicates whether the reallocation function zeros out new memory. Defaults to `FIO_MEM_REALLOC_IS_SAFE`.

#### `FIO_TYPEDEF_IMAP_FREE`

```c
#define FIO_TYPEDEF_IMAP_FREE FIO_MEM_FREE
```

Defines the deallocation function used by iMap. Defaults to `FIO_MEM_FREE`.

-------------------------------------------------------------------------------

### Helper Macros

#### `FIO_IMAP_ALWAYS_VALID`

```c
#define FIO_IMAP_ALWAYS_VALID(o) (1)
```

A helper macro for simple iMap array types where all objects are considered valid.

#### `FIO_IMAP_ALWAYS_CMP_TRUE`

```c
#define FIO_IMAP_ALWAYS_CMP_TRUE(a, b) (1)
```

A helper macro for simple iMap array types where all comparisons return true.

#### `FIO_IMAP_ALWAYS_CMP_FALSE`

```c
#define FIO_IMAP_ALWAYS_CMP_FALSE(a, b) (0)
```

A helper macro for simple iMap array types where all comparisons return false.

#### `FIO_IMAP_SIMPLE_CMP`

```c
#define FIO_IMAP_SIMPLE_CMP(a, b) ((a)[0] == (b)[0])
```

A helper macro for simple iMap array types that compares the first element of two objects.

#### `FIO_IMAP_EACH`

```c
#define FIO_IMAP_EACH(array_name, map_ptr, i)                                  \
  for (size_t i = 0; i < (map_ptr)->w; ++i)                                    \
    if (!FIO_NAME(array_name, is_valid)((map_ptr)->ary + i))                   \
      continue;                                                                \
    else
```

Iterates over all valid elements in the iMap array.

Example:

```c
FIO_IMAP_EACH(my_array, &my_map, i) {
  printf("Element at index %zu: %d\n", i, my_map.ary[i].value);
}
```

-------------------------------------------------------------------------------

### Type Definition Macro

#### `FIO_TYPEDEF_IMAP_ARRAY`

```c
#define FIO_TYPEDEF_IMAP_ARRAY(array_name,                                     \
                               array_type,                                     \
                               imap_type,                                      \
                               hash_fn,                                        \
                               cmp_fn,                                         \
                               is_valid_fn)
```

This macro defines the type and functions needed for an indexed array.

An indexed array is a simple ordered array whose objects are indexed using an almost-hash map, allowing for easy seeking while also enjoying the advantages provided by the array structure.

The index map uses one `imap_type` (e.g., `uint64_t`) to store both the index in the array and any leftover hash data. The first half of the bits are tested during random access, and the remaining bits are used during comparison.

**Reserved Values:**
- `0` - indicates a free slot
- `~0` (all bits set) - indicates a freed item (a slot that was previously used)

**Parameters:**
- `array_name` - the prefix for all generated type and function names
- `array_type` - the type of elements stored in the array
- `imap_type` - the unsigned integer type for the index map (e.g., `uint32_t`, `uint64_t`)
- `hash_fn(ptr)` - a function/macro that computes a hash from a pointer to an element
- `cmp_fn(a_ptr, b_ptr)` - a function/macro that compares two elements by pointer
- `is_valid_fn(ptr)` - a function/macro that returns non-zero if the element is valid

**Note**: `hash_fn`, `cmp_fn`, and `is_valid_fn` all accept **pointers** to elements and must dereference them to access the content.

-------------------------------------------------------------------------------

### Generated Types

#### `array_name_s`

```c
typedef struct {
  array_type *ary;    /* Pointer to the array of elements */
  imap_type count;    /* Number of valid elements in the array */
  imap_type w;        /* Write position (next available index) */
  uint32_t capa_bits; /* Log2 of the capacity */
} array_name_s;
```

The main container type for the indexed array.

**Members:**
- `ary` - pointer to the array of stored elements
- `count` - the number of valid (non-removed) elements
- `w` - the write position, indicating the next available slot
- `capa_bits` - the capacity expressed as a power of 2 (actual capacity is `1 << capa_bits`)

#### `array_name_seeker_s`

```c
typedef struct {
  imap_type pos;     /* Position in the array */
  imap_type ipos;    /* Position in the index map */
  imap_type set_val; /* Value to set in the index map */
} array_name_seeker_s;
```

A seeker type returned by `array_name_seek` that contains position information.

**Members:**
- `pos` - the position of the element in the array (or `w` if not found)
- `ipos` - the position in the index map
- `set_val` - the value to write to the index map when inserting

-------------------------------------------------------------------------------

### Generated Functions

#### `array_name_is_valid`

```c
int array_name_is_valid(array_type *pobj);
```

Returns non-zero if the object pointed to by `pobj` is valid.

This is a wrapper around the `is_valid_fn` provided to `FIO_TYPEDEF_IMAP_ARRAY`.

#### `array_name_capa`

```c
size_t array_name_capa(array_name_s *a);
```

Returns the theoretical storage capacity for the indexed array.

The capacity is calculated as `1 << a->capa_bits`.

#### `array_name_imap`

```c
imap_type *array_name_imap(array_name_s *a);
```

Returns a pointer to the index map.

The index map is stored immediately after the array data in memory.

#### `array_name_destroy`

```c
void array_name_destroy(array_name_s *a);
```

Deallocates all dynamic memory associated with the indexed array.

**Note**: this does not call destructors on stored elements. You must clean up element resources before calling this function.

#### `array_name_seek`

```c
array_name_seeker_s array_name_seek(array_name_s *a, array_type *pobj);
```

Finds an object in the array or determines its future position.

**Parameters:**
- `a` - pointer to the indexed array
- `pobj` - pointer to the object to search for

**Returns:** a seeker struct containing:
- `pos` - the array index of the found element, or `a->w` if not found
- `ipos` - the index map position, or `~0` if no suitable slot was found
- `set_val` - the value to write to the index map when inserting

#### `array_name_reserve`

```c
int array_name_reserve(array_name_s *a, imap_type min);
```

Reserves a minimum storage capacity for the indexed array.

**Parameters:**
- `a` - pointer to the indexed array
- `min` - the minimum capacity to reserve

**Returns:** `0` on success, `-1` on failure.

#### `array_name_rehash`

```c
int array_name_rehash(array_name_s *a);
```

Rebuilds the index map from the current array contents.

Use this function after sorting the array or after any operation that changes element positions without updating the index map.

**Returns:** `0` on success, `-1` on failure.

#### `array_name_set`

```c
array_type *array_name_set(array_name_s *a, array_type obj, int overwrite);
```

Writes or overwrites data in the array.

**Parameters:**
- `a` - pointer to the indexed array
- `obj` - the object to insert
- `overwrite` - if non-zero, overwrites existing data; otherwise returns existing element

**Returns:** a pointer to the element in the array, or `NULL` on failure.

#### `array_name_get`

```c
array_type *array_name_get(array_name_s *a, array_type obj);
```

Finds an object in the array using the index map.

**Parameters:**
- `a` - pointer to the indexed array
- `obj` - an object with the key/hash to search for

**Returns:** a pointer to the found element, or `NULL` if not found.

#### `array_name_remove`

```c
int array_name_remove(array_name_s *a, array_type obj);
```

Removes an object from the array and zeros out its memory.

**Parameters:**
- `a` - pointer to the indexed array
- `obj` - an object with the key/hash to remove

**Returns:** `0` on success, `-1` if the object was not found.

**Note**: the element's memory is zeroed, but no destructor is called. Handle resource cleanup before calling this function.

-------------------------------------------------------------------------------

### Internal Seeker Types

The iMap module also provides internal seeker types and functions for different integer sizes. These are used internally by other facil.io modules.

#### `fio___imapN_seeker_s`

```c
typedef struct {
  uintN_t pos;     /* Position in the array */
  uintN_t ipos;    /* Position in the index map */
  uintN_t set_val; /* Value to set in the index map */
  bool is_valid;   /* True if a valid element was found */
} fio___imapN_seeker_s;
```

Where `N` is one of `8`, `16`, `32`, or `64`.

#### `fio___imapN_seek`

```c
fio___imapN_seeker_s fio___imapN_seek(
    void *ary,
    uintN_t *imap,
    const uintN_t capa_bits,
    void *pobj,
    uintN_t hash,
    bool cmp_fn(void *ary, void *obj, uintN_t indx),
    const size_t max_attempts);
```

A lower-level seek function that searches for an element in an index map.

**Parameters:**
- `ary` - pointer to the array
- `imap` - pointer to the index map
- `capa_bits` - log2 of the capacity
- `pobj` - pointer to the object to search for
- `hash` - the hash value of the object
- `cmp_fn` - comparison function
- `max_attempts` - maximum number of probing attempts

#### `fio___imapN_set`

```c
void fio___imapN_set(uintN_t *imap, uintN_t ipos, uintN_t set_val);
```

Sets a value in the index map at the specified position.

-------------------------------------------------------------------------------

### Example

```c
#define FIO_IMAP_CORE
#include "fio-stl.h"

/* Define a simple key-value pair type */
typedef struct {
  uint64_t key;
  int value;
} my_kv_s;

/* Hash function - takes a pointer, returns hash of the key */
static uint64_t my_kv_hash(my_kv_s *p) {
  return fio_risky_num(p->key, 0);
}

/* Comparison function - takes two pointers, returns true if keys match */
static int my_kv_cmp(my_kv_s *a, my_kv_s *b) {
  return a->key == b->key;
}

/* Validity function - takes a pointer, returns true if valid */
static int my_kv_valid(my_kv_s *p) {
  return p->key != 0;
}

/* Define the indexed array type */
FIO_TYPEDEF_IMAP_ARRAY(my_kv,        /* array_name */
                       my_kv_s,      /* array_type */
                       uint32_t,     /* imap_type */
                       my_kv_hash,   /* hash_fn */
                       my_kv_cmp,    /* cmp_fn */
                       my_kv_valid)  /* is_valid_fn */

int main(void) {
  my_kv_s map = {0};
  
  /* Insert some key-value pairs */
  my_kv_set(&map, (my_kv_s){.key = 1, .value = 100}, 1);
  my_kv_set(&map, (my_kv_s){.key = 2, .value = 200}, 1);
  my_kv_set(&map, (my_kv_s){.key = 3, .value = 300}, 1);
  
  /* Look up a value */
  my_kv_s *found = my_kv_get(&map, (my_kv_s){.key = 2});
  if (found) {
    printf("Found key 2 with value: %d\n", found->value);
  }
  
  /* Iterate over all elements (in insertion order) */
  FIO_IMAP_EACH(my_kv, &map, i) {
    printf("Key: %llu, Value: %d\n", 
           (unsigned long long)map.ary[i].key, 
           map.ary[i].value);
  }
  
  /* Remove an element */
  my_kv_remove(&map, (my_kv_s){.key = 2});
  
  /* Clean up */
  my_kv_destroy(&map);
  
  return 0;
}
```

-------------------------------------------------------------------------------
