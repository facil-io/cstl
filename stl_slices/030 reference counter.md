## Reference Counting / Type Wrapping

```c
#define FIO_STR_SMALL fio_str
#define FIO_REF_NAME fio_str
#define FIO_REF_CONSTRUCTOR_ONLY
#include "fio-stl.h"
```

If the `FIO_REF_NAME` macro is defined, then reference counting helpers can be
defined for any named type.

By default, `FIO_REF_TYPE` will equal `FIO_REF_NAME_s`, using the naming
convention in this library.

In addition, the `FIO_REF_METADATA` macro can be defined with any type, allowing
metadata to be attached and accessed using the helper function
`FIO_REF_metadata(object)`.

If the `FIO_REF_CONSTRUCTOR_ONLY` macro is defined, the reference counter constructor (`TYPE_new`) will be the only constructor function.  When set, the reference counting functions will use `X_new` and `X_free`. Otherwise (assuming `X_new` and `X_free` are already defined), the reference counter will define `X_new2` and `X_free2` instead.

Note: requires the atomic operations to be defined (`FIO_ATOMIC`).

Reference counting adds the following functions:

#### `REF_new` / `REF_new2`

```c
FIO_REF_TYPE * REF_new2(void)
// or, if FIO_REF_CONSTRUCTOR_ONLY is defined
FIO_REF_TYPE * REF_new(void)
```

Allocates a new reference counted object, initializing it using the
`FIO_REF_INIT(object)` macro.

If `FIO_REF_METADATA` is defined, than the metadata is initialized using the
`FIO_REF_METADATA_INIT(metadata)` macro.

#### `REF_up_ref`

```c
FIO_REF_TYPE * REF_up_ref(FIO_REF_TYPE * object)
```

Increases an object's reference count (an atomic operation, thread-safe).

#### `REF_free` / `REF_free2`

```c
void REF_free2(FIO_REF_TYPE * object)
// or, if FIO_REF_CONSTRUCTOR_ONLY is defined
void REF_free(FIO_REF_TYPE * object)
```

Frees an object or decreases it's reference count (an atomic operation,
thread-safe).

Before the object is freed, the `FIO_REF_DESTROY(object)` macro will be called.

If `FIO_REF_METADATA` is defined, than the metadata is also destroyed using the
`FIO_REF_METADATA_DESTROY(metadata)` macro.


#### `REF_metadata`

```c
FIO_REF_METADATA * REF_metadata(FIO_REF_TYPE * object)
```

If `FIO_REF_METADATA` is defined, than the metadata is accessible using this
inlined function.

-------------------------------------------------------------------------------
