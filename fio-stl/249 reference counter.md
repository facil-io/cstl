## Reference Counting and Type Wrapping

```c
#define FIO_REF_NAME my_type
#define FIO_REF_TYPE my_type_s
#define FIO_REF_CONSTRUCTOR_ONLY
#include "fio-stl.h"
```

If the `FIO_REF_NAME` macro is defined, then reference counting helpers can be defined for any named type.

**Note**: requires the atomic operations to be defined (`FIO_ATOMIC`).

**Note**: if `FIO_PTR_TAG_TYPE` is defined, the reference counting functions will use the tagged pointer type for parameters and return values instead of `FIO_REF_TYPE *`.

### Reference Counting Type Macros

The following setup Macros are supported when setting up the reference counting type helpers:

#### `FIO_REF_TYPE`

```c
#define FIO_REF_TYPE FIO_NAME(FIO_REF_NAME, s)
```

The type to be wrapped and reference counted by the `FIO_REF_NAME` wrapper API.

By default, `FIO_REF_TYPE` will equal `FIO_REF_NAME_s`, using the naming convention in this library.

#### `FIO_REF_INIT`

```c
#define FIO_REF_INIT(obj)                                                      \
  do {                                                                         \
    if (!FIO_MEM_REALLOC_IS_SAFE_)                                             \
      (obj) = (FIO_REF_TYPE){0};                                               \
  } while (0)
```

Sets up the default object initializer.

By default initializes the object's memory to zero, but only if the memory allocator doesn't guarantee zeroed memory (`FIO_MEM_REALLOC_IS_SAFE_`).

If `FIO_REF_FLEX_TYPE` is defined, the variable `members` may be used during initialization. It's value is the same as the value passed on to the `REF_new` function.

**Note**:  `FIO_REF_FLEX_TYPE` should **not** be used when `FIO_MEM_FREE` macro only frees the number of bytes specified (rather than freeing the whole pointer, as `free` might do). The reference counter type does not store the data passed to the flex-`REF_new` function and frees the same number of bytes as a flex length of `0`.

#### `FIO_REF_DESTROY`

```c
#define FIO_REF_DESTROY(obj)
```

Sets up the default object cleanup. By default does nothing.

#### `FIO_REF_CONSTRUCTOR_ONLY`

By default, the reference counter generator will generate the `new2`, `free2` and `dup2` functions.

However, f the `FIO_REF_CONSTRUCTOR_ONLY` macro is defined, the reference counter will name these functions as `new`, `free` and `dup` instead, making them the type's only and primary constructor / destructor.

#### `FIO_REF_FLEX_TYPE`

If the `FIO_REF_FLEX_TYPE` macro is defined, the constructor will allocate a enough memory for both the type and a `FIO_REF_FLEX_TYPE` array consisting of the specified amount of members (as passed to the constructor's `member` argument).

This allows reference objects structures to include a flexible array of type `FIO_REF_FLEX_TYPE` at the end of the `struct`.

The `members` variable passed to the constructor will also be available to the `FIO_REF_INIT` macro.

**Note**: using `FIO_REF_FLEX_TYPE` limits the reference counter to 32 bits (rather then the native word size which **may** be 64 bits).

#### `FIO_REF_METADATA`

If defined, should be type that will be available as "meta data".

A pointer to this type sill be available using the `REF_metadata` function and will allow "hidden" data to be accessible even though it isn't part of the observable object.

#### `FIO_REF_METADATA_INIT`

```c
#define FIO_REF_METADATA_INIT(meta)                                            \
  do {                                                                         \
    if (!FIO_MEM_REALLOC_IS_SAFE_)                                             \
      (meta) = (FIO_REF_METADATA){0};                                          \
  } while (0)
```

Sets up object's meta-data initialization (if any). By default initializes the meta-data object's memory to zero, but only if the memory allocator doesn't guarantee zeroed memory (`FIO_MEM_REALLOC_IS_SAFE_`).

#### `FIO_REF_METADATA_DESTROY`

```c
#define FIO_REF_METADATA_DESTROY(meta)
```

### Reference Counting Generated Functions

Reference counting adds the following functions:

#### `REF_new` / `REF_new2`

```c
FIO_REF_TYPE * REF_new2(void)
// or, if FIO_REF_FLEX_TYPE is defined:
FIO_REF_TYPE * REF_new2(size_t members)


// or, if FIO_REF_CONSTRUCTOR_ONLY is defined
FIO_REF_TYPE * REF_new(void) 
FIO_REF_TYPE * REF_new(size_t members) // for FIO_REF_FLEX_TYPE

```

Allocates a new reference counted object, initializing it using the `FIO_REF_INIT(object)` macro.

If `FIO_REF_METADATA` is defined, than the metadata is initialized using the `FIO_REF_METADATA_INIT(metadata)` macro.

#### `REF_dup` / `REF_dup2`

```c
FIO_REF_TYPE * REF_dup2(FIO_REF_TYPE * wrapped)
// or, if FIO_REF_CONSTRUCTOR_ONLY is defined
FIO_REF_TYPE * REF_dup(FIO_REF_TYPE * wrapped)
```

Increases an object's reference count (an atomic operation, thread-safe).

**Parameters:**
- `wrapped` - pointer to the reference counted object

**Returns:** the same pointer passed in, or `NULL` if the input was `NULL`.

#### `REF_free` / `REF_free2`

```c
void REF_free2(FIO_REF_TYPE * object)
// or, if FIO_REF_CONSTRUCTOR_ONLY is defined
void REF_free(FIO_REF_TYPE * object)
```

Frees an object or decreases it's reference count (an atomic operation, thread-safe).

Before the object is freed, the `FIO_REF_DESTROY(object)` macro will be called.

If `FIO_REF_METADATA` is defined, than the metadata is also destroyed using the `FIO_REF_METADATA_DESTROY(metadata)` macro.

#### `REF_metadata`

```c
FIO_REF_METADATA * REF_metadata(FIO_REF_TYPE * wrapped)
```

If `FIO_REF_METADATA` is defined, then the metadata is accessible using this inlined function.

**Parameters:**
- `wrapped` - pointer to the reference counted object

**Returns:** a pointer to the object's metadata.

#### `REF_metadata_flex_len`

```c
uint32_t REF_metadata_flex_len(FIO_REF_TYPE * wrapped)
```

If `FIO_REF_FLEX_TYPE` is defined, this function returns the number of flex array members that were allocated with the object.

**Parameters:**
- `wrapped` - pointer to the reference counted object

**Returns:** the number of flex array members allocated, or `0` if `wrapped` is `NULL`.

#### `REF_references`

```c
size_t REF_references(FIO_REF_TYPE * wrapped)
```

A debugging helper that returns the current reference count for an object.

**Parameters:**
- `wrapped` - pointer to the reference counted object

**Returns:** the current reference count, or `0` if `wrapped` is `NULL`.

**Note**: the returned value is unstable and should not be used for program logic. It is intended for debugging purposes only.

-------------------------------------------------------------------------------
