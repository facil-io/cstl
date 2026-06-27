# Reference Counting Wrapper — Module 249

Heap-allocate any type with an atomic reference count attached, generated from a name macro.

See also: [← 200 types-overview.md](./200 types-overview.md)

---

## Setup

```c
#define FIO_REF_NAME  my_obj           /* required */
#define FIO_REF_TYPE  my_obj_s         /* optional — defaults to FIO_REF_NAME_s */
#define FIO_REF_CONSTRUCTOR_ONLY       /* optional — see below */
#include "fio-stl.h"
```

This generates a thin wrapper struct that sits **before** the `FIO_REF_TYPE` object in memory. The caller only ever sees a `FIO_REF_TYPE *` (or the tagged pointer type if `FIO_PTR_TAG_TYPE` is set). The reference count, optional metadata, and optional flex-array length are all hidden in the prefix.

Throughout this document `REF` stands for whatever name you chose (e.g. `my_obj`).

> **Requires:** the `FIO_ATOMIC` helpers must be available (included automatically when using `fio-stl.h` as a single header).

---

## Configuration Macros

Set these **before** `#include`, after `FIO_REF_NAME`.

### Wrapped type

| Macro | Default | Effect |
|---|---|---|
| `FIO_REF_TYPE` | `FIO_REF_NAME_s` | The C type to wrap and reference-count. |

### Constructor / destructor naming

| Macro | Effect |
|---|---|
| _(not defined)_ | Generates `REF_new2`, `REF_dup2`, `REF_free2` — the `2` suffix leaves room for a separately-defined `REF_new` / `REF_free`. |
| `FIO_REF_CONSTRUCTOR_ONLY` | Generates `REF_new`, `REF_dup`, `REF_free` — use this when the ref-counted constructor **is** the primary constructor. |

### Object lifecycle hooks

| Macro | Default | Effect |
|---|---|---|
| `FIO_REF_INIT(obj)` | zero-fill if allocator doesn't guarantee it | Called on the newly allocated `FIO_REF_TYPE` object after allocation. `obj` is the dereferenced object (not a pointer). |
| `FIO_REF_DESTROY(obj)` | _(nothing)_ | Called on the `FIO_REF_TYPE` object just before memory is freed (when the last reference drops). |

### Metadata

| Macro | Default | Effect |
|---|---|---|
| `FIO_REF_METADATA` | _(not defined)_ | A type to embed as hidden metadata alongside the ref-counted object. Access it via `REF_metadata()`. |
| `FIO_REF_METADATA_INIT(meta)` | zero-fill if needed | Called on the metadata field after allocation. |
| `FIO_REF_METADATA_DESTROY(meta)` | _(nothing)_ | Called on the metadata field just before memory is freed. |

### Flexible array

| Macro | Default | Effect |
|---|---|---|
| `FIO_REF_FLEX_TYPE` | _(not defined)_ | When defined, the constructor allocates extra memory for a `FIO_REF_FLEX_TYPE[]` array immediately after the main struct. The constructor accepts a `size_t members` argument and stores the count internally. The `members` value is also available inside `FIO_REF_INIT`. |

> **Note:** `FIO_REF_FLEX_TYPE` shrinks the reference counter to 32 bits (instead of the native word size). Do not combine this with a custom `FIO_MEM_FREE` that depends on the byte-count argument, because the flex free call reports a size that omits `sizeof(FIO_REF_TYPE)`.

### Pointer tagging

If `FIO_PTR_TAG_TYPE` is defined (globally, before including), all generated functions accept and return `FIO_PTR_TAG_TYPE` instead of `FIO_REF_TYPE *`. Tags are applied on allocation (`FIO_PTR_TAG`) and stripped internally (`FIO_PTR_UNTAG`).

---

## Generated API

### Constructor

```c
/* standard */
FIO_REF_TYPE *REF_new(void);                  /* FIO_REF_CONSTRUCTOR_ONLY */
FIO_REF_TYPE *REF_new2(void);                 /* default */

/* with FIO_REF_FLEX_TYPE */
FIO_REF_TYPE *REF_new(size_t members);
FIO_REF_TYPE *REF_new2(size_t members);
```

Allocates and zero-initializes the wrapper header, calls `FIO_REF_METADATA_INIT` (if any), then calls `FIO_REF_INIT` on the wrapped object. Returns `NULL` on allocation failure. Initial reference count is **1**.

### Increment reference count

```c
FIO_REF_TYPE *REF_dup(const FIO_REF_TYPE *obj);    /* FIO_REF_CONSTRUCTOR_ONLY */
FIO_REF_TYPE *REF_dup2(const FIO_REF_TYPE *obj);   /* default */
```

Atomically increments the reference count. Returns the same pointer, or `NULL` if the input is `NULL`. Thread-safe.

### Decrement reference count / free

```c
void REF_free(FIO_REF_TYPE *obj);             /* FIO_REF_CONSTRUCTOR_ONLY */
void REF_free2(FIO_REF_TYPE *obj);            /* default */
```

Atomically decrements the reference count. When the count reaches zero, calls `FIO_REF_DESTROY(object)`, then `FIO_REF_METADATA_DESTROY(metadata)` (if any), then frees the backing memory. Thread-safe. No-op on `NULL`.

### Metadata access

```c
FIO_REF_METADATA *REF_metadata(FIO_REF_TYPE *obj);
```

Returns a pointer to the hidden metadata field. Only generated when `FIO_REF_METADATA` is defined.

### Flex array length

```c
uint32_t REF_metadata_flex_len(FIO_REF_TYPE *obj);
```

Returns the number of `FIO_REF_FLEX_TYPE` members allocated alongside the object. Only generated when `FIO_REF_FLEX_TYPE` is defined. Returns `0` for `NULL`.

### Debugging helper

```c
size_t REF_references(FIO_REF_TYPE *obj);
```

Returns the current reference count. **Do not use for program logic** — the value is inherently unstable in a concurrent context. Useful for assertions and leak-hunting.

---

## Examples

### Basic reference-counted object

```c
typedef struct { int x; int y; } point_s;

#define FIO_REF_NAME             point
#define FIO_REF_TYPE             point_s
#define FIO_REF_CONSTRUCTOR_ONLY          /* point_new / point_dup / point_free */
#define FIO_REF_INIT(obj)        (obj).x = 0; (obj).y = 0
#include "fio-stl.h"

void example(void) {
    point_s *p = point_new();     /* ref count = 1 */
    p->x = 10; p->y = 20;

    point_s *alias = point_dup(p); /* ref count = 2 */

    point_free(alias);            /* ref count = 1 — not freed yet */
    point_free(p);                /* ref count = 0 — freed */
}
```

### Using the `2`-suffix when a primary constructor already exists

```c
/* Suppose FIO_STR_NAME already generated str_new / str_free. */
#define FIO_REF_NAME  str
#define FIO_REF_TYPE  str_s
/* No FIO_REF_CONSTRUCTOR_ONLY — generates str_new2 / str_dup2 / str_free2 */
#define FIO_REF_DESTROY(obj) str_destroy(&(obj))
#include "fio-stl.h"

str_s *s = str_new2();           /* ref-counted allocation */
str_write(s, "hello", 5);        /* use the string API directly */
str_s *s2 = str_dup2(s);
str_free2(s2);
str_free2(s);                    /* calls str_destroy, then frees memory */
```

### Hidden metadata

```c
typedef struct { char name[64]; } widget_s;
typedef struct { uint64_t created_at; } widget_meta_s;

#define FIO_REF_NAME              widget
#define FIO_REF_TYPE              widget_s
#define FIO_REF_CONSTRUCTOR_ONLY
#define FIO_REF_METADATA          widget_meta_s
#define FIO_REF_METADATA_INIT(m)  (m).created_at = fio_time_real().tv_sec
#include "fio-stl.h"

void example(void) {
    widget_s *w = widget_new();
    widget_meta_s *m = widget_metadata(w);
    printf("created at: %llu\n", (unsigned long long)m->created_at);
    widget_free(w);
}
```

### Flexible array suffix

```c
typedef struct { size_t len; double data[]; } vec_s;

#define FIO_REF_NAME              vec
#define FIO_REF_TYPE              vec_s
#define FIO_REF_CONSTRUCTOR_ONLY
#define FIO_REF_FLEX_TYPE         double
#define FIO_REF_INIT(obj)         (obj).len = members  /* 'members' is in scope */
#include "fio-stl.h"

void example(void) {
    vec_s *v = vec_new(8);       /* allocates vec_s + 8 doubles */
    uint32_t n = vec_metadata_flex_len(v); /* returns 8 */
    for (uint32_t i = 0; i < n; i++) v->data[i] = (double)i;
    vec_free(v);
}
```

---

## Memory Layout

```
 [ _wrapper_s header ] [ FIO_REF_TYPE object ] [ FIO_REF_FLEX_TYPE[] ] (optional)
       ^                        ^
   hidden prefix         pointer returned to caller
```

The `_wrapper_s` header holds the reference count (and optional `flx_size` / `metadata` fields). Callers never see it — they only receive a pointer to the `FIO_REF_TYPE` that immediately follows.

---

## Notes

- All reference-count operations use **atomic** instructions and are thread-safe.
- The module must be placed after any other type modules it wraps (e.g., after `FIO_STR_NAME` or `FIO_ARRAY_NAME` inclusions) when used together in one translation unit.
- Each `FIO_REF_NAME` definition is independent: you can wrap multiple types in the same file.
- `REF_references()` is a debugging aid; its return value is transient and must not drive control flow.
