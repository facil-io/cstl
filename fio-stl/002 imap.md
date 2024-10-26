## iMap - an Index Mapped Array (Hash Map - Array Combo)

The `FIO_TYPEDEF_IMAP_ARRAY` macro is one way to design a hash map and is **used internally** for some modules (to minimize dependencies or nested inclusions).

It is used when both insertion order and iteration over the complete data set is of high priority, or when it is important to hold the same data as both an Array and a Hash Map.

**Note**: there's no memory management when objects are removed or the iMap is destroyed.

**Note**: for most use cases it is much better to create a type with the `FIO_MAP_NAME` macro.

#### `FIO_TYPEDEF_IMAP_ARRAY`

```c
#define FIO_TYPEDEF_IMAP_ARRAY(array_name,                                     \
                               array_type,                                     \
                               imap_type,                                      \
                               hash_fn,                                        \
                               cmp_fn,                                         \
                               is_valid_fn)
```

This MACRO defines the type and functions needed for an indexed array.

An indexed array is simple ordered array who's objects are indexed using an almost-hash map, allowing for easy seeking while also enjoying the advantages provided by the array structure.

The index map uses one `imap_type` (i.e., `uint64_t`) to store both the index in array and any leftover hash data (the first half being tested during the random access and the leftover during comparison). The reserved value `0` indicates a free slot. The reserved value `~0` indicates a freed item (a free slot that was previously used).

This is mostly for internal use and documentation is poor (PR, anyone?).

The macro defines the following:

- `array_name_s`        the main array container (.ary is the array itself)

- `array_name_seeker_s` is a seeker type that finds objects.
- `array_name_seek`     finds an object or its future position.

- `array_name_reserve`  reserves a minimum imap storage capacity.
- `array_name_capa`     the imap's theoretical storage capacity.

- `array_name_set`      writes or overwrites data to the array.
- `array_name_get`      returns a pointer to the object within the array.
- `array_name_remove`   removes an object and resets its memory to zero.

- `array_name_rehash`   re-builds the imap (use after sorting).


Notes:

- `hash_fn(ptr)`, `cmp_fn(a_ptr,b_ptr)` and `is_valid_fn(ptr)` accepts **pointers**  and needs to de-reference them in order to compare their content.

-------------------------------------------------------------------------------
