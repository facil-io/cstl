## FIOBJ Soft Dynamic Types

```c
#define FIO_FIOBJ
#define FIOBJ_MALLOC /* an optional local memory allocator for FIOBJ types */
#include "fio-stl.h"
```

The facil.io library includes a dynamic type system that makes it a easy to handle mixed-type tasks, such as JSON object construction.

This soft type system included in the facil.io STL, it is based on the Core types mentioned above and it shares their API (Dynamic Strings, Dynamic Arrays, and Hash Maps).

The soft type system also offers an (optional) * [Local Memory allocator](#local-memory-allocation) for improved performance when defined with the `FIOBJ_MALLOC` macro defined.

The `FIOBJ` API offers type generic functions in addition to the type specific API. An objects underlying type is easily identified using `FIOBJ_TYPE(obj)` or `FIOBJ_TYPE_IS(obj, type)`.

The documentation regarding the `FIOBJ` soft-type system is divided as follows:  

* [`FIOBJ` General Considerations](#fiobj-general-considerations)

* [`FIOBJ` Types and Identification](#fiobj-types-and-identification)

* [`FIOBJ` Core Memory Management](#fiobj-core-memory-management)

* [`FIOBJ` Common Functions](#fiobj-common-functions)

* [Primitive Types](#fiobj-primitive-types)

* [Numbers (Integers)](#fiobj-integers)

* [Floats](#fiobj-floats)

* [Strings](#fiobj-strings)

* [Arrays](#fiobj-arrays)

* [Hash Maps](#fiobj-hash-maps)

* [JSON Helpers](#fiobj-json-helpers)

* [How to Extend the `FIOBJ` Type System](#how-to-extend-the-fiobj-type-system)

In the facil.io web application framework, there are extensions to the core `FIOBJ` primitives, including:

* [IO storage](fiobj_io)

* [Mustache](fiobj_mustache)

### `FIOBJ` General Considerations

1. To use the `FIOBJ` soft types, define the `FIO_FIOBJ` macro and then include the facil.io STL header.

2. To include declarations as globally available symbols (allowing the functions to be called from multiple C files), define `FIOBJ_EXTERN` _before_ including the STL header.

    This also requires that a _single_ C file (translation unit) define `FIOBJ_EXTERN_COMPLETE` _before_ including the header with the `FIOBJ_EXTERN` directive.

3. The `FIOBJ` types use pointer tagging and require that the memory allocator provide allocations on 8 byte memory alignment boundaries (they also assume each byte is 8 bits).

    If the system allocator doesn't provide (at least) 8 byte memory alignment, use the facil.io memory allocator provided (`fio_malloc`).

4. The `FIOBJ` soft type system uses an "**ownership**" memory model.

    This means that Arrays "**own**" their **members** and Hash Maps "**own**" their **values** (but **not** the keys).

    Freeing an Array will free all the objects within the Array. Freeing a Hash Map will free all the values within the Hash Map (but none of the keys).

    Ownership is only transferred if the object is removed from it's container.

    i.e., `fiobj_array_get` does **not** transfer ownership (it just allows temporary "access"). Whereas, `fiobj_array_remove` **does** revoke ownership - either freeing the object or moving the ownership to the pointer provided to hold the `old` value.

### `FIOBJ` Types and Identification

`FIOBJ` objects can contain any number of possible types, including user defined types.

These are the built-in types / classes that the Core `FIOBJ` system includes (before any extensions):

* `FIOBJ_T_INVALID`: indicates an **invalid** type class / type (a `FIOBJ_INVALID` value).

* `FIOBJ_T_PRIMITIVE`: indicates a **Primitive** class / type.

* `FIOBJ_T_NUMBER`: indicates a **Number** class / type.

* `FIOBJ_T_FLOAT`: indicates a **Float** class / type.

* `FIOBJ_T_STRING`: indicates a **String** class / type.

* `FIOBJ_T_ARRAY`: indicates an **Array** class / type.

* `FIOBJ_T_HASH`: indicates a **Hash Map** class / type.

* `FIOBJ_T_OTHER`: (internal) indicates an **Other** class / type. This is designed to indicate an extension / user defined type.

The `FIOBJ_T_PRIMITIVE` class / type resolves to one of the following types:

* `FIOBJ_T_NULL`: indicates a `fiobj_null()` object.

* `FIOBJ_T_TRUE`: indicates a `fiobj_true()` object.

* `FIOBJ_T_FALSE`: indicates a `fiobj_false()` object.

In the facil.io web application framework, there are extensions to the core `FIOBJ` primitives, including:

* [`FIOBJ_T_IO`](fiobj_io)

The following functions / MACROs help identify a `FIOBJ` object's underlying type.

#### `FIOBJ_TYPE(o)`

```c
#define FIOBJ_TYPE(o) fiobj_type(o)
```

#### `FIOBJ_TYPE_IS(o)`

```c
#define FIOBJ_TYPE_IS(o, type) (fiobj_type(o) == type)
```

#### `FIOBJ_TYPE_CLASS(o)`

```c
#define FIOBJ_TYPE_CLASS(o) ((fiobj_class_en)(((uintptr_t)o) & 7UL))
```

Returns the object's type class. This is limited to one of the core types. `FIOBJ_T_PRIMITIVE` and `FIOBJ_T_OTHER` may be returned (they aren't expended to their underlying type).

**Note**: some numbers (`FIOBJ_T_NUMBER` / `FIOBJ_T_FLOAT`) may return `FIOBJ_T_OTHER` when `FIOBJ_TYPE_CLASS` is used, but return their proper type when `FIOBJ_TYPE` is used. This is due to memory optimizations being unavailable for some numerical values.

#### `FIOBJ_IS_INVALID(o)`

```c
#define FIOBJ_IS_INVALID(o) (((uintptr_t)(o)&7UL) == 0)
```

Tests if the object is (probably) a valid FIOBJ

#### `FIOBJ_IS_NULL(o)`

```c
#define FIOBJ_IS_NULL(o) (FIOBJ_IS_INVALID(o) || ((o) == FIOBJ_T_NULL))
```

Tests if the object is either a `NULL` `FIOBJ` object or an invalid object.

#### `FIOBJ_PTR_UNTAG(o)`

```c
#define FIOBJ_PTR_UNTAG(o) ((uintptr_t)o & (~7ULL))
```

Removes the `FIOBJ` type tag from a `FIOBJ` objects, allowing access to the underlying pointer and possible type.

This is made available for authoring `FIOBJ` extensions and **shouldn't** be normally used.

#### `fiobj_type`

```c
size_t fiobj_type(FIOBJ o);
```

Returns an objects type. This isn't limited to known types.

Avoid calling this function directly. Use the MACRO instead.

### `FIOBJ` Core Memory Management

`FIOBJ` objects are **copied by reference** (not by value). Once their reference count is reduced to zero, their memory is freed.

This is extremely important to note, especially in multi-threaded environments. This implied that: **access to a dynamic `FIOBJ` object is _NOT_ thread-safe** and `FIOBJ` objects that may be written to (such as Arrays, Strings and Hash Maps) should **not** be shared across threads (unless properly protected).

The `FIOBJ` soft type system uses an "**ownership**" memory model. When placing a **value** in an Array or a Hash Map, the "ownership" is moved. Freeing the Array / Hash Map will free the object (unless `fiobj_dup` was called). Hash Maps "**own**" their _values_ (but **not** the _keys_).

#### `fiobj_dup`

```c
FIOBJ fiobj_dup(FIOBJ o);
```

Increases an object's reference count and returns it.

#### `fiobj_free`

```c
void fiobj_free(FIOBJ o);
```

Decreases an object's reference count or frees it.

**Note**:

This function is **recursive** and could cause a **stack explosion** error.

In addition, recursive object structures may produce unexpected results (for example, objects are always freed).

The `FIOBJ_MAX_NESTING` nesting limit doesn't apply to `fiobj_free`, making it possible to "expload" the stack if misused.

This places the responsibility on the user / developer, not to exceed the maximum nesting limit (or errors may occur).

When accepting external data, consider using the JSON parser, as it protects against this issue, offering a measure of safety against external data attacks.

### `FIOBJ` Common Functions

#### `fiobj_is_eq`

```c
unsigned char fiobj_is_eq(FIOBJ a, FIOBJ b);
```

Compares two objects.

Note: objects that contain other objects (i.e., Hash Maps) don't support this equality check just yet (feel free to contribute a PR for this).

#### `fiobj2cstr`

```c
fio_str_info_s fiobj2cstr(FIOBJ o);
```

Returns a temporary String representation for any FIOBJ object.

For number objects and floats this is thread safe for up to 256 threads.

For printing Arrays and Hash maps, using a JSON representation will provide more information.

#### `fiobj2i`

```c
intptr_t fiobj2i(FIOBJ o);
```

Returns an integer representation for any FIOBJ object.

#### `fiobj2f`

```c
double fiobj2f(FIOBJ o);
```

Returns a float (double) representation for any FIOBJ object.


#### `fiobj_each1`

```c
uint32_t fiobj_each1(FIOBJ o, int32_t start_at,
                     int (*task)(FIOBJ child, void *arg),
                     void *arg);
```

Performs a task for each element held by the FIOBJ object **directly** (but **not** itself).

If `task` returns -1, the `each` loop will break (stop).

Returns the "stop" position - the number of elements processed + `start_at`.


#### `fiobj_each2`

```c
uint32_t fiobj_each2(FIOBJ o,
                     int (*task)(FIOBJ obj, void *arg),
                     void *arg);
```

Performs a task for each element held by the FIOBJ object (directly or indirectly), **including** itself and any nested elements (a deep task).

The order of performance is by order of appearance, as if all nesting levels were flattened.

If `task` returns -1, the `each` loop will break (stop).

Returns the number of elements processed.

**Note**:

This function is **recursive** and could cause a **stack explosion** error.

The facil.io library attempts to protect against this error by limiting recursive access to `FIOBJ_MAX_NESTING`... however, this also assumes that a user / developer doesn't exceed the maximum nesting limit (or errors may occur).

#### `fiobj_json_find`

```c
FIOBJ fiobj_json_find(FIOBJ object, fio_str_info_s notation);
```

Uses JavaScript (JSON) style notation to find data in an object structure.

For example, `"[0].name"` will return the `"name"` property of the first object in an Array object.

Returns a temporary reference to the object or `FIOBJ_INVALID` on an error.

Use `fiobj_dup` to collect an actual reference to the returned object.

**Note**:

Using the search algorithm with long object names and/or deeper nesting levels might incur a performance penalty due to the fact that the algorithm tests for all possible object name permutations.

i.e., `"name1.name2.name3"` will first be tested as the whole string (`"name1.name2.name3"`), then `"name1.name2" + "name.3"` will be tested, then `"name1" + "name2.name.3"` will be tested for and `"name1" + "name2" + "name3"` will only be attempted last (allowing all permutations to be reviewed rather than assuming a `.` is always a delimiter).

#### `fiobj_json_find2`

```c
#define fiobj_json_find2(object, str, length)                                  \
  fiobj_json_find(object, (fio_str_info_s){.buf = str, .len = length})
```

A macro helper for [`fiobj_json_find`](#fiobj_json_find).

### `FIOBJ` Primitive Types

The `true`, `false` and `null` primitive type functions (in addition to the common functions) are only their simple static constructor / accessor functions.

The primitive types are immutable.

#### `fiobj_true`

```c
FIOBJ fiobj_true(void);
```

Returns the `true` primitive.

#### `fiobj_false`

```c
FIOBJ fiobj_false(void);
```

Returns the `false` primitive.

#### `fiobj_null`

```c
FIOBJ fiobj_null(void);
```

Returns the `nil` / `null` primitive.


### `FIOBJ` Integers

#### `fiobj_num_new`

```c
FIOBJ fiobj_num_new(intptr_t i);
```

Creates a new Number object.

#### `fiobj_num2i`

```c
intptr_t fiobj_num2i(FIOBJ i);
```

Reads the number from a `FIOBJ` Number.

#### `fiobj_num2f`

```c
double fiobj_num2f(FIOBJ i);
```

Reads the number from a `FIOBJ` Number, fitting it in a double.

#### `fiobj_num2cstr`

```c
fio_str_info_s fiobj_num2cstr(FIOBJ i);
```

Returns a String representation of the number (in base 10).

#### `fiobj_num_free`

```c
void fiobj_num_free(FIOBJ i);
```

Frees a `FIOBJ` number (a type specific `fiobj_free` alternative - use only when the type was validated).


### `FIOBJ` Floats

#### `fiobj_float_new`

```c
FIOBJ fiobj_float_new(double i);
```

Creates a new Float (double) object.

#### `fiobj_float2i`

```c
intptr_t fiobj_float2i(FIOBJ i);
```

Reads the number from a `FIOBJ` Float rounding it to an integer.

#### `fiobj_float2f`

```c
double fiobj_float2f(FIOBJ i);
```

Reads the value from a `FIOBJ` Float, as a double.

#### `fiobj_float2cstr`

```c
fio_str_info_s fiobj_float2cstr(FIOBJ i);
```

Returns a String representation of the float.

#### `fiobj_float_free`

```c
void fiobj_float_free(FIOBJ i);
```

Frees a `FIOBJ` Float (a type specific `fiobj_free` alternative - use only when the type was validated).


### `FIOBJ` Strings

`FIOBJ` Strings are based on the core `STR_x` functions. This means that all these core type functions are available also for this type, using the `fiobj_str` prefix (i.e., [`STR_new`](#str_new) becomes [`fiobj_str_new`](#str_new), [`STR_write`](#str_write) becomes [`fiobj_str_write`](#str_write), etc').

In addition, the following `fiobj_str` functions and MACROs are defined:

#### `fiobj_str_new_cstr`

```c
FIOBJ fiobj_str_new_cstr(const char *ptr, size_t len);
```

Creates a new `FIOBJ` string object, copying the data to the new string.


#### `fiobj_str_new_buf`

```c
FIOBJ fiobj_str_new_buf(size_t capa);
```

Creates a new `FIOBJ` string object with (at least) the requested capacity.


#### `fiobj_str_new_copy`

```c
FIOBJ fiobj_str_new_copy(FIOBJ original);
```

Creates a new `FIOBJ` string object, copying the origin ([`fiobj2cstr`](#fiobj2cstr)).


#### `fiobj_str2cstr`

```c
fio_str_info_s fiobj_str2cstr(FIOBJ s);
```

Returns information about the string. Same as [`fiobj_str_info()`](#str_info).

#### `FIOBJ_STR_TEMP_DESTROY(name)`

```c
#define FIOBJ_STR_TEMP_DESTROY(str_name)  \
  FIO_NAME(fiobj_str, destroy)(str_name);
```

Resets a temporary `FIOBJ` String, freeing and any resources allocated.

See the following `FIOBJ_STR_TEMP_XXX` macros for creating temporary FIOBJ strings on the Stack.

#### `FIOBJ_STR_TEMP_VAR(name)`

```c
#define FIOBJ_STR_TEMP_VAR(str_name)                                   \
  struct {                                                             \
    uint64_t i1;                                                       \
    uint64_t i2;                                                       \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;               \
  } FIO_NAME(str_name, __auto_mem_tmp) = {                             \
      0x7f7f7f7f7f7f7f7fULL, 0x7f7f7f7f7f7f7f7fULL, FIO_STR_INIT};     \
  FIOBJ str_name =                                                     \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) | \
              FIOBJ_T_STRING);
```

Creates a temporary `FIOBJ` String object on the stack.

String data might be allocated dynamically, requiring the use of `FIOBJ_STR_TEMP_DESTROY`.

#### `FIOBJ_STR_TEMP_VAR_STATIC(str_name, buf, len)`

```c
#define FIOBJ_STR_TEMP_VAR_STATIC(str_name, buf_, len_)                        \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name,                                                         \
             __auto_mem_tmp) = {0x7f7f7f7f7f7f7f7fULL,                         \
                                0x7f7f7f7f7f7f7f7fULL,                         \
                                FIO_STR_INIT_STATIC2((buf_), (len_))};         \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);
```

Creates a temporary FIOBJ String object on the stack, initialized with a static string.

Editing the String data **will** cause dynamic memory allocation, use `FIOBJ_STR_TEMP_DESTROY` once done.

This variation will cause memory allocation immediately upon editing the String. The buffer _MAY_ be read only.

#### `FIOBJ_STR_TEMP_VAR_EXISTING(str_name, buf, len, capa)`

```c
#define FIOBJ_STR_TEMP_VAR_EXISTING(str_name, buf_, len_, capa_)               \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name, __auto_mem_tmp) = {                                     \
      0x7f7f7f7f7f7f7f7fULL,                                                   \
      0x7f7f7f7f7f7f7f7fULL,                                                   \
      FIO_STR_INIT_EXISTING((buf_), (len_), (capa_))};                         \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);
```

Creates a temporary FIOBJ String object on the stack for a read/write buffer with the specified capacity.

Editing the String data might cause dynamic memory allocation, use `FIOBJ_STR_TEMP_DESTROY` once done.

Remember to manage the buffer's memory once it was de-linked from the temporary string (as the FIOBJ object does **not** take ownership of the memory).

#### `FIOBJ` Strings - Core Type Functions

In addition, all the functions documented above as `STR_x`, are defined as `fiobj_str_x`:

* [`fiobj_str_new`](#str_new) - creates a new empty string.

* [`fiobj_str_free`](#str_free) - frees a FIOBJ known to be a String object.

* [`fiobj_str_destroy`](#str_destroy) - destroys / clears a String, returning it to an empty state. 

* [`fiobj_str_detach`](#str_detach) - destroys / clears a String, returning a `char *` C-String.

* [`fiobj_str_info`](#str_info) - returns information about the string.

* [`fiobj_str_len`](#str_len) - returns the string's length.

* [`fiobj_str2ptr`](#str2ptr) - returns a pointer to the string's buffer.

* [`fiobj_str_capa`](#str_capa) - returns the string's capacity.

* [`fiobj_str_freeze`](#str_freeze) - freezes a string (a soft flag, enforced only by functions).

* [`fiobj_str_is_frozen`](#str_is_frozen) - returns true if the string is frozen.

* [`fiobj_str_is_eq`](#str_is_eq) - returns true if the strings are equal.

* [`fiobj_str_hash`](#str_hash) - returns a string's Risky Hash.

* [`fiobj_str_resize`](#str_resize) - resizes a string (keeping the current buffer).

* [`fiobj_str_compact`](#str_compact) - attempts to minimize memory usage.

* [`fiobj_str_reserve`](#str_reserve) - reserves memory for future `write` operations.

* [`fiobj_str_utf8_valid`](#str_utf8_valid) - tests in a string is UTF8 valid.

* [`fiobj_str_utf8_len`](#str_utf8_len) - returns a string's length in UTF8 characters.

* [`fiobj_str_utf8_select`](#str_utf8_select) - selects a section of the string using UTF8 offsets.

* [`fiobj_str_write`](#str_write) - writes data to the string.

* [`fiobj_str_write_i`](#str_write_i) - writes a base 10 number to the string.

* [`fiobj_str_write_hex`](#str_write_hex) - writes a base 16 (hex) number to the string.

* [`fiobj_str_concat`](#str_concat-str_join) - writes an existing string to the string.

* [`fiobj_str_replace`](#str_replace) - replaces a section of the string.

* [`fiobj_str_vprintf`](#str_vprintf) - writes formatted data to the string.

* [`fiobj_str_printf`](#str_printf) - writes formatted data to the string.

* [`fiobj_str_readfd`](#str_readfd) - writes data from an open file to the string.

* [`fiobj_str_readfile`](#str_readfile) - writes data from an unopened file to the string.

* [`fiobj_str_write_b64enc`](#str_write_b64enc) - encodes and writes data to the string using base 64.

* [`fiobj_str_write_b64dec`](#str_write_b64dec) - decodes and writes data to the string using base 64.

* [`fiobj_str_write_escape`](#str_write_escape) - writes JSON style escaped data to the string.

* [`fiobj_str_write_unescape`](#str_write_unescape) - writes decoded JSON escaped data to the string.


### `FIOBJ` Arrays

`FIOBJ` Arrays are based on the core `ARY_x` functions. This means that all these core type functions are available also for this type, using the `fiobj_array` prefix (i.e., [`ARY_new`](#ary_new) becomes [`fiobj_array_new`](#ary_new), [`ARY_push`](#ary_push) becomes [`fiobj_array_push`](#ary_push), etc').

These functions include:

* [`fiobj_array_new`](#ary_new)

* [`fiobj_array_free`](#ary_free)

* [`fiobj_array_destroy`](#ary_destroy)

* [`fiobj_array_count`](#ary_count)

* [`fiobj_array_capa`](#ary_capa)

* [`fiobj_array_reserve`](#ary_reserve)

* [`fiobj_array_concat`](#ary_concat)

* [`fiobj_array_set`](#ary_set)

* [`fiobj_array_get`](#ary_get)

* [`fiobj_array_find`](#ary_find)

* [`fiobj_array_remove`](#ary_remove)

* [`fiobj_array_remove2`](#ary_remove2)

* [`fiobj_array_compact`](#ary_compact)

* [`fiobj_array_to_a`](#ary_to_a)

* [`fiobj_array_push`](#ary_push)

* [`fiobj_array_pop`](#ary_pop)

* [`fiobj_array_unshift`](#ary_unshift)

* [`fiobj_array_shift`](#ary_shift)

* [`fiobj_array_each`](#ary_each)

### `FIOBJ` Ordered Hash Maps

`FIOBJ` Ordered Hash Maps are based on the core `MAP_x` functions. This means that all these core type functions are available also for this type, using the `fiobj_hash` prefix (i.e., [`MAP_new`](#map_new) becomes [`fiobj_hash_new`](#map_new), [`MAP_set`](#map_set) becomes [`fiobj_hash_set`](#map_set), etc').

In addition, the following `fiobj_hash` functions and MACROs are defined:

#### `fiobj2hash`

```c
uint64_t fiobj2hash(FIOBJ target_hash, FIOBJ value);
```

Calculates an object's hash value for a specific hash map object.

#### `fiobj_hash_set2`

```c
FIOBJ fiobj_hash_set2(FIOBJ hash, FIOBJ key, FIOBJ value);
```

Inserts a value to a hash map, with a default hash value calculation.

#### `fiobj_hash_set_if_missing2`

```c
FIOBJ fiobj_hash_set_if_missing2(FIOBJ hash, FIOBJ key, FIOBJ value);
```

Inserts a value to a hash map, with a default hash value calculation.

If the key already exists in the Hash Map, the value will be freed instead.

#### `fiobj_hash_get2`

```c
FIOBJ fiobj_hash_get2(FIOBJ hash, FIOBJ key);
```

Finds a value in a hash map, with a default hash value calculation.

#### `fiobj_hash_remove2`

```c
int fiobj_hash_remove2(FIOBJ hash, FIOBJ key, FIOBJ *old);
```

Removes a value from a hash map, with a default hash value calculation.

#### `fiobj_hash_set3`

```c
FIOBJ fiobj_hash_set3(FIOBJ hash, const char *key, size_t len, FIOBJ value);
```

Sets a value in a hash map, allocating the key String and automatically calculating the hash value.

#### `fiobj_hash_get3`

```c
FIOBJ fiobj_hash_get3(FIOBJ hash, const char *buf, size_t len);
```

Finds a String value in a hash map, using a temporary String as the key and automatically calculating the hash value.

#### `fiobj_hash_remove3`

```c
int fiobj_hash_remove3(FIOBJ hash, const char *buf, size_t len, FIOBJ *old);
```

Removes a String value in a hash map, using a temporary String as the key and automatically calculating the hash value.

#### `FIOBJ` Hash Map - Core Type Functions

In addition, all the functions documented above as `MAP_x`, are defined as `fiobj_hash_x`:

* [`fiobj_hash_new`](#map_new)

* [`fiobj_hash_free`](#map_free)

* [`fiobj_hash_destroy`](#map_destroy)

* [`fiobj_hash_get`](#map_get-hash-map)

* [`fiobj_hash_get_ptr`](#map_get_ptr)

* [`fiobj_hash_set`](#map_set)

* [`fiobj_hash_set_ptr`](#map_set_ptr)

* [`fiobj_hash_remove`](#map_remove)

* [`fiobj_hash_evict`](#map_evict)

* [`fiobj_hash_count`](#map_count)

* [`fiobj_hash_capa`](#map_capa)

* [`fiobj_hash_reserve`](#map_reserve)

* [`fiobj_hash_compact`](#map_compact)

* [`fiobj_hash_rehash`](#map_rehash)

* [`fiobj_hash_each`](#map_each)

* [`fiobj_hash_each_get_key`](#map_each_get_key)

### `FIOBJ` JSON Helpers

Parsing, editing and outputting JSON in C can be easily accomplished using `FIOBJ` types.

`facil.io` offers the added benefit of complete parsing from JSON to object. This allows the result to be manipulated, updated, sliced or merged with ease. This is in contrast to some parsers that offer a mid-way structures or lazy (delayed) parsing for types such as `true`, `false` and Numbers.

`facil.io` also offers the added benefit of complete formatting from a framework wide object type (`FIOBJ`) to JSON, allowing the same soft type system to be used throughout the project (rather than having a JSON dedicated type system).

This is in addition to `facil.io` support to some JSON extensions such as comments, both C style (both `//` and `/* ... */` and bash style (`#`).

However, there are [faster alternatives as well as slower alternatives out there](json_performance.html) (i.e., the [Qajson4c library](https://github.com/DeHecht/qajson4c) is a wonderful alternative for embedded systems).

#### `fiobj2json`

```c
FIOBJ fiobj2json(FIOBJ dest, FIOBJ o, uint8_t beautify);
```

Returns a JSON valid FIOBJ String, representing the object.

If `dest` is an existing String, the formatted JSON data will be appended to the existing string.

```c
FIOBJ result = fiobj_json_parse2("{\"name\":\"John\",\"surname\":\"Smith\",\"ID\":1}",40, NULL);
FIO_ASSERT( fiobj2cstr(fiobj_hash_get3(result, "name", 4)).len == 4 &&
            !memcmp(fiobj2cstr(fiobj_hash_get3(result, "name", 4)).buf, "John", 4), "result error");

FIOBJ_STR_TEMP_VAR(json_str); /* places string on the stack */
fiobj2json(json_str, result, 1);
FIO_LOG_INFO("updated JSON data to look nicer:\n%s", fiobj2cstr(json_str).buf);
fiobj_free(result);
FIOBJ_STR_TEMP_DESTROY(json_str);
```

#### `fiobj_hash_update_json`

```c
size_t fiobj_hash_update_json(FIOBJ hash, fio_str_info_s str);

size_t fiobj_hash_update_json2(FIOBJ hash, char *ptr, size_t len);
```

Updates a Hash using JSON data.

Parsing errors and non-dictionary object JSON data are silently ignored, attempting to update the Hash as much as possible before any errors encountered.

Conflicting Hash data is overwritten (preferring the new over the old).

Returns the number of bytes consumed. On Error, 0 is returned and no data is consumed.

The `fiobj_hash_update_json2` function is a helper function, it calls `fiobj_hash_update_json` with the provided string information.

#### `fiobj_json_parse`

```c
FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed);

#define fiobj_json_parse2(data_, len_, consumed)                      \
  fiobj_json_parse((fio_str_info_s){.buf = data_, .len = len_}, consumed)
```

Parses a C string for JSON data. If `consumed` is not NULL, the `size_t` variable will contain the number of bytes consumed before the parser stopped (due to either error or end of a valid JSON data segment).

Returns a FIOBJ object matching the JSON valid C string `str`.

If the parsing failed (no complete valid JSON data) `FIOBJ_INVALID` is returned.

`fiobj_json_parse2` is a helper macro, it calls `fiobj_json_parse` with the provided string information.

### How to Extend the `FIOBJ` Type System

The `FIOBJ` source code includes two extensions for the `Float` and `Number` types.

In many cases, numbers and floats can be used without memory allocations. However, when memory allocation is required to store the data, the `FIOBJ_T_NUMBER` and `FIOBJ_T_FLOAT` types are extended using the same techniques described here.

#### `FIOBJ` Extension Requirements

To extend the `FIOBJ` soft type system, there are a number of requirements:

1. A **unique** type ID must be computed.

    Type IDs are `size_t` bits in length. Values under 100 are reserved. Values under 40 are illegal (might break implementation).

2. A static virtual function table object (`FIOBJ_class_vtable_s`) must be fully populated (`NULL` values may break cause a segmentation fault).

3. The unique type construct / destructor must be wrapped using the facil.io reference counting wrapper (using `FIO_REF_NAME`).

    The `FIO_REF_METADATA` should be set to a `FIOBJ_class_vtable_s` pointer and initialized for every object.

4. The unique type wrapper must use pointer tagging as described bellow (`FIO_PTR_TAG`).

5. A public API should be presented.

#### `FIOBJ` Pointer Tagging

The `FIOBJ` types is often identified by th a bit "tag" added to the pointer. All extension types **must** be tagged as `FIOBJ_T_OTHER`.

The facil.io memory allocator (`fio_malloc`), as well as most system allocators, promise a 64 bit allocation alignment. The `FIOBJ` types leverage this behavior by utilizing the least significant 3 bits that are always zero. However, this implementation might change in the future, so it's better to use the macros `FIOBJ_PTR_TAG` and `FIOBJ_PTR_UNTAG`.

The following macros should be defined for tagging an extension `FIOBJ` type, allowing the `FIO_REF_NAME` constructor / destructor to manage pointer tagging, reference counting and access to the `FIOBJ` virtual table (see later on).

```c
#define FIO_PTR_TAG(p)   FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
```

#### `FIOBJ` Virtual Function Tables

`FIOBJ` extensions use a virtual function table that is shared by all the objects of that type/class.

Basically, the virtual function table is a `struct` with the **Type ID** and function pointers.

**Type ID** values under 100 are reserved for facil.io and might cause conflicts with the existing type values if used (i.e., `FIOBJ_T_FALSE == 34`).

All function pointers must be populated (where `each1` is only called if `count` returns a non-zero value).

This is the structure of the virtual table:

```c
/** FIOBJ types can be extended using virtual function tables. */
typedef struct {
  /** A unique number to identify object type. */
  size_t type_id;
  /** Test for equality between two objects with the same `type_id` */
  unsigned char (*is_eq)(FIOBJ a, FIOBJ b);
  /** Converts an object to a String */
  fio_str_info_s (*to_s)(FIOBJ o);
  /** Converts an object to an integer */
  intptr_t (*to_i)(FIOBJ o);
  /** Converts an object to a double */
  double (*to_f)(FIOBJ o);
  /** Returns the number of exposed elements held by the object, if any. */
  uint32_t (*count)(FIOBJ o);
  /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
  uint32_t (*each1)(FIOBJ o, int32_t start_at,
                    int (*task)(FIOBJ child, void *arg), void *arg);
  /**
   * Decreases the reference count and/or frees the object, calling `free2` for
   * any nested objects.
   *
   * Returns 0 if the object is still alive or 1 if the object was freed. The
   * return value is currently ignored, but this might change in the future.
   */
  int (*free2)(FIOBJ o);
} FIOBJ_class_vtable_s;
```

#### `FIOBJ` Extension Example

For our example, let us implement a static string extension type. We will use the Type ID 100 because values under 100 are reserved.

Let's call our example header file `fiobj_static.h`, so we can find it later.

The API for this type and the header might look something like this:

```c
#ifndef FIO_STAT_STRING_HEADER_H
/* *****************************************************************************
FIOBJ Static String Extension Header Example
***************************************************************************** */
#define FIO_STAT_STRING_HEADER_H
/* *****************************************************************************
Perliminaries - include the FIOBJ extension, but not it's implementation
***************************************************************************** */
#define FIO_EXTERN
#define FIOBJ_EXTERN
#define FIO_FIOBJ
#include "fio-stl.h"

/* *****************************************************************************
Defining the Type ID and the API
***************************************************************************** */

/** The Static String Type ID */
#define FIOBJ_T_STATIC_STRING 100UL

/** Returns a new static string object. The string is considered immutable. */
FIOBJ fiobj_static_new(const char *str, size_t len);

/** Returns a pointer to the static string. */
const char *fiobj_static2ptr(FIOBJ s);

/** Returns the static strings length. */
size_t fiobj_static_len(FIOBJ s);

#endif
```

**Note**: The header assumes that _somewhere_ there's a C implementation file that includes the `FIOBJ` implementation. That C file defines the `FIOBJ_EXTERN_COMPLETE` macro **before** including the `fio-stl.h` file (as well as defining `FIO_FIOBJ` and `FIOBJ_EXTERN`).

The implementation may look like this.

```c
/* *****************************************************************************
FIOBJ Static String Extension Implementation Example
***************************************************************************** */
#include <fiobj_static.h> // include the header file here, whatever it's called

/* *****************************************************************************
The Virtual Function Table (definitions and table)
***************************************************************************** */

/** Test for equality between two objects with the same `type_id` */
static unsigned char static_string_is_eq(FIOBJ a, FIOBJ b);
/** Converts an object to a String */
static fio_str_info_s static_string_to_s(FIOBJ o);
/** Converts an object to an integer */
static intptr_t static_string_to_i(FIOBJ o);
/** Converts an object to a double */
static double static_string_to_f(FIOBJ o);
/** Returns the number of exposed elements held by the object, if any. */
static uint32_t static_string_count(FIOBJ o);
/** Iterates the exposed elements held by the object. See `fiobj_each1`. */
static uint32_t static_string_each1(FIOBJ o, int32_t start_at,
                                    int (*task)(FIOBJ, void *), void *arg);
/**
 * Decreases the reference count and/or frees the object, calling `free2` for
 * any nested objects (which we don't have for this type).
 *
 * Returns 0 if the object is still alive or 1 if the object was freed. The
 * return value is currently ignored, but this might change in the future.
 */
static int static_string_free2(FIOBJ o);

/** The virtual function table object. */
static const FIOBJ_class_vtable_s FIOBJ___STATIC_STRING_VTABLE = {
    .type_id = FIOBJ_T_STATIC_STRING,
    .is_eq = static_string_is_eq,
    .to_s = static_string_to_s,
    .to_i = static_string_to_i,
    .to_f = static_string_to_f,
    .count = static_string_count,
    .each1 = static_string_each1,
    .free2 = static_string_free2,
};

/* *****************************************************************************
The Static String Type (internal implementation)
***************************************************************************** */

/* leverage the small-string type to hold static string data */
#define FIO_STR_SMALL fiobj_static_string
/* add required pointer tagging */
#define FIO_PTR_TAG(p)   FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
/* add required reference counter / wrapper type */
#define FIO_REF_NAME fiobj_static_string
#define FIO_REF_CONSTRUCTOR_ONLY
/* initialization - for demonstration purposes, we don't use it here. */
#define FIO_REF_INIT(o)                                                        \
  do {                                                                         \
    o = (fiobj_static_string_s){0};                                            \
    FIOBJ_MARK_MEMORY_ALLOC(); /* mark memory allocation for debugging */      \
  } while (0)
/* cleanup - destroy the object data when the reference count reaches zero. */
#define FIO_REF_DESTROY(o)                                                     \
  do {                                                                         \
    fiobj_static_string_destroy((FIOBJ)&o);                                    \
    FIOBJ_MARK_MEMORY_FREE(); /* mark memory deallocation for debugging */     \
  } while (0)
/* metadata (vtable) definition and initialization. */
#define FIO_REF_METADATA const FIOBJ_class_vtable_s *
/* metadata initialization - required to initialize the vtable. */
#define FIO_REF_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___STATIC_STRING_VTABLE;                                         \
  } while (0)
#include <fio-stl.h>

/* *****************************************************************************
The Public API
***************************************************************************** */

/** Returns a new static string object. The string is considered immutable. */
FIOBJ fiobj_static_new(const char *str, size_t len) {
  FIOBJ o = fiobj_static_string_new();
  FIO_ASSERT_ALLOC(FIOBJ_PTR_UNTAG(o));
  fiobj_static_string_init_const(o, str, len);
  return o;
}

/** Returns a pointer to the static string. */
const char *fiobj_static2ptr(FIOBJ o) { return fiobj_static_string2ptr(o); }

/** Returns the static strings length. */
size_t fiobj_static_len(FIOBJ o) { return fiobj_static_string_len(o); }

/* *****************************************************************************
Virtual Function Table Implementation
***************************************************************************** */

/** Test for equality between two objects with the same `type_id` */
static unsigned char static_string_is_eq(FIOBJ a, FIOBJ b) {
  fio_str_info_s ai, bi;
  ai = fiobj_static_string_info(a);
  bi = fiobj_static_string_info(b);
  return (ai.len == bi.len && !memcmp(ai.buf, bi.buf, ai.len));
}
/** Converts an object to a String */
static fio_str_info_s static_string_to_s(FIOBJ o) {
  return fiobj_static_string_info(o);
}
/** Converts an object to an integer */
static intptr_t static_string_to_i(FIOBJ o) {
  fio_str_info_s s = fiobj_static_string_info(o);
  if (s.len)
    return fio_atol(&s.buf);
  return 0;
}
/** Converts an object to a double */
static double static_string_to_f(FIOBJ o) {
  fio_str_info_s s = fiobj_static_string_info(o);
  if (s.len)
    return fio_atof(&s.buf);
  return 0.0L;
}
/** Returns the number of exposed elements held by the object, if any. */
static uint32_t static_string_count(FIOBJ o) {
  return 0;
  (void)o;
}
/** Iterates the exposed elements held by the object. See `fiobj_each1`. */
static uint32_t static_string_each1(FIOBJ o, int32_t start_at,
                                    int (*task)(FIOBJ, void *), void *arg) {
  return 0;
  (void)o; (void)start_at; (void)task; (void)arg;
}
/** Decreases the reference count and/or frees the object. */
static int static_string_free2(FIOBJ o) { return fiobj_static_string_free(o); }
```

Example usage:

```c
#define FIOBJ_EXTERN_COMPLETE // we will place the FIOBJ implementation here.
#include "fiobj_static.h"     // include FIOBJ extension type
int main(void) {
  FIOBJ o = fiobj_static_new("my static string", 16);
  /* example test of virtual table redirection */
  FIO_ASSERT(fiobj2cstr(o).buf == fiobj_static2ptr(o) &&
                 fiobj2cstr(o).len == fiobj_static_len(o),
             "vtable redirection error.");
  fprintf(stderr, "allocated: %s\n", fiobj_static2ptr(o));
  fprintf(stderr, "it's %zu byte long\n", fiobj_static_len(o));
  fprintf(stderr, "object type: %zu\n", FIOBJ_TYPE(o));
  fiobj_free(o);
  FIOBJ_MARK_MEMORY_PRINT(); /* only in DEBUG mode */
}
```

-------------------------------------------------------------------------------

