# FIOBJ — Soft Dynamic Types — Module 250

A unified soft-type system built on pointer tagging. One `FIOBJ` handle covers `null`, `true`, `false`, integers, floats, strings, arrays, and hash maps — plus user-defined extensions.

See also: [← 200 types-overview.md](./200 types-overview.md)

---

## Setup

```c
#define FIO_FIOBJ
#include "fio-stl.h"
```

No name macro is needed — `FIO_FIOBJ` activates the entire soft-type system in one step.

To expose symbols across multiple translation units add `FIO_EXTERN` before the include everywhere, and `FIO_EXTERN_COMPLETE` in exactly one `.c` file.

---

## How it Works

`FIOBJ` is a tagged pointer (`uintptr_t`). The bottom 3 bits encode the type class; the remaining bits hold the value or a heap pointer. Small integers and many floats require **no heap allocation** at all.

```
 bits[2:0]  type class
 bits[63:3] value or aligned pointer
```

All heap-allocated types are reference-counted. Primitives, small numbers, and many floats are immediate values — `fiobj_dup` / `fiobj_free` are no-ops for them.

The system uses an **ownership** model:
- Placing a value into an Array or Hash Map **transfers ownership** to the container.
- Freeing the container frees its elements (Arrays own members; Hash Maps own keys and values).
- Call `fiobj_dup` before inserting if you need to keep an independent reference.

---

## Configuration Macros

Set these before `#include` to override defaults.

### Nesting / recursion

| Macro | Default | Effect |
|---|---|---|
| `FIOBJ_MAX_NESTING` | `512` | Maximum depth for `fiobj_each2` and JSON serialization/parsing. Does **not** apply to `fiobj_free`. |
| `FIOBJ_JSON_APPEND` | `1` | When `1`, JSON parsing appends to existing containers; when `0`, it replaces them. |

### Type-name prefixes

All generated type names can be renamed by defining the following macros before the include. The defaults produce the API names used throughout this document.

| Macro | Default | Resulting prefix |
|---|---|---|
| `FIOBJ___NAME_NULL` | `null` | `fiobj_null` |
| `FIOBJ___NAME_NUMBER` | `num` | `fiobj_num_*` |
| `FIOBJ___NAME_FLOAT` | `float` | `fiobj_float_*` |
| `FIOBJ___NAME_STRING` | `str` | `fiobj_str_*` |
| `FIOBJ___NAME_ARRAY` | `array` | `fiobj_array_*` |
| `FIOBJ___NAME_HASH` | `hash` | `fiobj_hash_*` |

---

## Type Identification

### Type constants

| Constant | Value | Description |
|---|---|---|
| `FIOBJ_T_INVALID` | `0` | Invalid / uninitialized FIOBJ |
| `FIOBJ_T_PRIMITIVE` | `2` | Raw pointer-tag class for all primitives |
| `FIOBJ_T_NULL` | `2` | `null` primitive |
| `FIOBJ_T_TRUE` | `18` | `true` primitive |
| `FIOBJ_T_FALSE` | `34` | `false` primitive |
| `FIOBJ_T_NUMBER` | `0x01` | Integer (small integers are unboxed) |
| `FIOBJ_T_FLOAT` | `0x06` | Float / double (many values are unboxed) |
| `FIOBJ_T_STRING` | `0x03` | Dynamic string |
| `FIOBJ_T_ARRAY` | `0x04` | Dynamic array |
| `FIOBJ_T_HASH` | `0x05` | Ordered hash map |
| `FIOBJ_T_OTHER` | `0x07` | Extension / user-defined type |
| `FIOBJ_INVALID` | `0` | The invalid FIOBJ sentinel value |

### Type-check macros

```c
size_t         FIOBJ_TYPE(FIOBJ o);              /* full type ID, including extensions */
int            FIOBJ_TYPE_IS(FIOBJ o, type);     /* 1 if type matches */
fiobj_class_en FIOBJ_TYPE_CLASS(FIOBJ o);        /* raw 3-bit class (no extension lookup) */
int            FIOBJ_IS_INVALID(FIOBJ o);        /* true if o == FIOBJ_INVALID (0) */
int            FIOBJ_IS_NULL(FIOBJ o);           /* true if INVALID or null primitive */
```

`FIOBJ_TYPE` is the safe, recommended way to check type. `FIOBJ_TYPE_CLASS` is faster but may return `FIOBJ_T_OTHER` for heap-boxed numbers and floats. It also returns `FIOBJ_T_PRIMITIVE` (`2`) for `null`, `true`, and `false` alike — use `FIOBJ_TYPE` to distinguish primitives.

---

## Lifecycle — Reference Counting

```c
FIOBJ fiobj_dup(FIOBJ o);    /* increment ref count; returns o */
void  fiobj_free(FIOBJ o);   /* decrement ref count; frees when zero */
```

Primitives, unboxed integers, and unboxed floats are immune — `fiobj_dup` / `fiobj_free` are no-ops for them. Always call `fiobj_free` on every FIOBJ you own.

**Warning:** `fiobj_free` is recursive. Deeply nested structures can overflow the stack. `FIOBJ_MAX_NESTING` does **not** protect it. Use the JSON parser (which does protect against nesting attacks) when handling untrusted input.

```c
/* build a small object graph */
FIOBJ arr = fiobj_array_new();
fiobj_array_push(arr, fiobj_num_new(42));      /* array owns the number */
fiobj_array_push(arr, fiobj_str_new_cstr("hi", 2)); /* array owns the string */

FIOBJ ref = fiobj_dup(arr);   /* arr and ref both point to same array */
fiobj_free(ref);              /* decrement; array still alive */
fiobj_free(arr);              /* decrement to zero → frees array + contents */
```

---

## Generic Accessors

These work on **any** FIOBJ type.

```c
fio_str_info_s fiobj2cstr(FIOBJ o);      /* string representation (temporary) */
intptr_t       fiobj2i(FIOBJ o);         /* integer representation */
double         fiobj2f(FIOBJ o);         /* float representation */
uint64_t       fiobj2hash(FIOBJ o);      /* hash value (for use as map key) */
unsigned char  fiobj_is_eq(FIOBJ a, FIOBJ b); /* deep equality */
```

Conversion behaviour by type:

| Object type | `fiobj2i` | `fiobj2f` | `fiobj2cstr` |
|---|---|---|---|
| `null` / invalid | `0` | `0.0` | `"null"` / `""` |
| `true` | `1` | `1.0` | `"true"` |
| `false` | `0` | `0.0` | `"false"` |
| Number | the integer | cast to double | base-10 string |
| Float | floor to integer | the double | decimal string |
| String | `fio_atol` of content | `fio_atof` of content | raw content |
| Array | element count | element count as double | `"[...]"` |
| Hash | key-value pair count | count as double | `"{...}"` |

`fiobj_is_eq` performs deep structural comparison for Arrays and Hash Maps.

`fiobj2cstr` returns a temporary view. For numbers/floats the buffer is thread-local (safe for up to 128 concurrent threads). For heap strings it points directly into the string's buffer.

---

## Primitives — null, true, false

The three primitives are immediate constants. They never allocate memory and never need `fiobj_free`.

```c
FIOBJ fiobj_null(void);    /* returns the null primitive */
FIOBJ fiobj_true(void);    /* returns the true primitive */
FIOBJ fiobj_false(void);   /* returns the false primitive */
```

```c
FIOBJ v = fiobj_null();
if (FIOBJ_IS_NULL(v))        /* also true for FIOBJ_INVALID (0) */
    printf("got null\n");

if (FIOBJ_TYPE_IS(v, FIOBJ_T_NULL))   /* strictly the null primitive */
    printf("strict null\n");
```

---

## Numbers — `fiobj_num_*`

Small integers are stored in the tagged pointer itself — no allocation. Large values that don't fit are heap-boxed transparently.

```c
FIOBJ         fiobj_num_new(intptr_t i);     /* create */
intptr_t      fiobj_num2i(FIOBJ i);          /* read as integer */
double        fiobj_num2f(FIOBJ i);          /* read as double */
fio_str_info_s fiobj_num2cstr(FIOBJ i);      /* base-10 string (temp) */
void          fiobj_num_free(FIOBJ i);       /* type-specific free */
```

`fiobj_num_free` is a fast alternative to `fiobj_free` when the type has already been validated.

---

## Floats — `fiobj_float_*`

Many double values fit in the tagged pointer (those whose lower 3 bits of the IEEE 754 representation are zero). Others are heap-boxed.

```c
FIOBJ          fiobj_float_new(double i);    /* create */
intptr_t       fiobj_float2i(FIOBJ i);       /* floor to integer */
double         fiobj_float2f(FIOBJ i);       /* read as double */
fio_str_info_s fiobj_float2cstr(FIOBJ i);    /* decimal string (temp) */
void           fiobj_float_free(FIOBJ i);    /* type-specific free */
```

---

## Strings — `fiobj_str_*`

FIOBJ strings wrap the [Dynamic String module (201)](./201 string.md). The full `fiobj_str_*` API mirrors the `STR_*` API — write, concat, printf, base64, JSON-escape, UTF-8 helpers, freeze, etc.

### Constructors

```c
FIOBJ fiobj_str_new(void);                          /* empty string */
FIOBJ fiobj_str_new_cstr(const char *ptr, size_t len); /* copy from C string */
FIOBJ fiobj_str_new_buf(size_t capa);               /* empty with reserved capacity */
FIOBJ fiobj_str_new_copy(FIOBJ original);           /* copy from any FIOBJ (via fiobj2cstr) */
```

### Common operations

```c
fio_str_info_s fiobj_str2cstr(FIOBJ s);             /* same as fiobj_str_info() */
void           fiobj_str_write(FIOBJ s, const char *buf, size_t len);
void           fiobj_str_printf(FIOBJ s, const char *fmt, ...);
void           fiobj_str_concat(FIOBJ dest, FIOBJ src); /* append src string */
void           fiobj_str_freeze(FIOBJ s);            /* mark immutable */
void           fiobj_str_free(FIOBJ s);              /* type-specific free */
```

### Stack-allocated temporary strings

```c
/* Empty temporary string on the stack */
FIOBJ_STR_TEMP_VAR(name);

/* Temporary string wrapping a static buffer (read-only view) */
FIOBJ_STR_TEMP_VAR_STATIC(name, buf, len);

/* Temporary string wrapping an existing read/write buffer */
FIOBJ_STR_TEMP_VAR_EXISTING(name, buf, len, capa);

/* Always call when done — releases any dynamic allocation */
FIOBJ_STR_TEMP_DESTROY(name);
```

Temporary strings live on the stack and are valid FIOBJ values. They must **not** be passed to containers or freed with `fiobj_free`.

```c
/* example: use a stack string as a hash key without heap allocation */
FIOBJ_STR_TEMP_VAR_STATIC(key, "name", 4);
FIOBJ val = fiobj_hash_get(hash, key);
FIOBJ_STR_TEMP_DESTROY(key);
```

---

## Arrays — `fiobj_array_*`

FIOBJ arrays wrap the [Dynamic Array module (202)](./202 array.md). The element type is `FIOBJ`; elements are owned by the array.

### Common operations

```c
FIOBJ  fiobj_array_new(void);
void   fiobj_array_free(FIOBJ a);
FIOBJ *fiobj_array_push(FIOBJ a, FIOBJ value);  /* append; transfers ownership; NULL on error */
int    fiobj_array_pop(FIOBJ a, FIOBJ *old);    /* remove last; stores in *old or frees if NULL */
FIOBJ  fiobj_array_get(FIOBJ a, int64_t index); /* access; NO ownership transfer */
FIOBJ *fiobj_array_set(FIOBJ a, int64_t index, FIOBJ value, FIOBJ *old); /* old in *old or freed */
int    fiobj_array_remove(FIOBJ a, int64_t index, FIOBJ *old); /* remove; optional ownership transfer */
uint32_t fiobj_array_count(FIOBJ a);
void   fiobj_array_concat(FIOBJ dest, FIOBJ src); /* appends copies of src elements */
```

Negative indices count from the end (`-1` = last element).

```c
FIOBJ a = fiobj_array_new();
fiobj_array_push(a, fiobj_num_new(1));
fiobj_array_push(a, fiobj_num_new(2));
fiobj_array_push(a, fiobj_str_new_cstr("three", 5));

printf("count: %u\n", fiobj_array_count(a));    /* 3 */
printf("first: %ld\n", fiobj2i(fiobj_array_get(a, 0))); /* 1 */

fiobj_free(a);   /* frees array and all three elements */
```

---

## Hash Maps — `fiobj_hash_*`

FIOBJ hash maps are **ordered** maps keyed and valued by FIOBJ. They wrap the [Hash Map module (210)](./210 map.md). The map owns stored keys and values. The low-level `fiobj_hash_set` duplicates FIOBJ keys, so the caller still owns its original key reference; C-string helpers copy the key string internally.

### Core operations

```c
FIOBJ fiobj_hash_new(void);
void  fiobj_hash_free(FIOBJ h);
uint32_t fiobj_hash_count(FIOBJ h);

/* FIOBJ-key variants (low level) */
FIOBJ fiobj_hash_set(FIOBJ h, FIOBJ key, FIOBJ value, FIOBJ *old);
FIOBJ fiobj_hash_get(FIOBJ h, FIOBJ key);
int   fiobj_hash_remove(FIOBJ h, FIOBJ key, FIOBJ *old);
```

### C-string key helpers (recommended)

```c
/* set: copies key string, transfers value ownership */
FIOBJ fiobj_hash_set2(FIOBJ h, const char *key, size_t len, FIOBJ value);

/* get: no allocation, no ownership transfer */
FIOBJ fiobj_hash_get2(FIOBJ h, const char *key, size_t len);

/* remove: returns 0 on success; puts old value in *old (or frees it if NULL) */
int   fiobj_hash_remove2(FIOBJ h, const char *key, size_t len, FIOBJ *old);
```

### Merge

```c
/* deep merge: arrays concatenate, nested hashes recurse, scalars overwrite;
   null/invalid values in src remove the matching key from dest. */
void fiobj_hash_update(FIOBJ dest, FIOBJ src);
```

```c
FIOBJ h = fiobj_hash_new();
fiobj_hash_set2(h, "name", 4, fiobj_str_new_cstr("Alice", 5));
fiobj_hash_set2(h, "age",  3, fiobj_num_new(30));

FIOBJ name = fiobj_hash_get2(h, "name", 4);
printf("%s\n", fiobj2cstr(name).buf);   /* Alice */

fiobj_free(h);   /* frees map, values, and key strings */
```

---

## Iteration

### Shallow — `fiobj_each1`

Iterates direct children of an Array or Hash Map.

```c
typedef struct fiobj_each_s {
  FIOBJ const parent;   /* the container being iterated */
  uint64_t    index;    /* current element index */
  int (*task)(struct fiobj_each_s *info);  /* callback (may be swapped mid-loop) */
  void       *udata;    /* caller-supplied data */
  FIOBJ       value;    /* current element value */
  FIOBJ       key;      /* current key (Hash Maps only) */
} fiobj_each_s;

uint32_t fiobj_each1(FIOBJ o,
                     int (*task)(fiobj_each_s *info),
                     void *udata,
                     int32_t start_at);
```

Return `-1` from the callback to stop early. Returns the stop position (`elements processed + start_at`).

```c
int print_item(fiobj_each_s *e) {
    printf("[%llu] %s\n", (unsigned long long)e->index,
           fiobj2cstr(e->value).buf);
    return 0;
}

fiobj_each1(arr, print_item, NULL, 0);
```

### Deep — `fiobj_each2`

Recursively walks the object and all nested containers, as if flattened.

```c
uint32_t fiobj_each2(FIOBJ o,
                     int (*task)(fiobj_each_s *info),
                     void *udata);
```

Respects `FIOBJ_MAX_NESTING`. The root object itself is also passed to the callback (`index == 0`, `parent == FIOBJ_INVALID`).

---

## JSON

### Serialize to JSON

```c
FIOBJ fiobj2json(FIOBJ dest, FIOBJ o, uint8_t beautify);
```

Returns a `FIOBJ` string containing the JSON representation of `o`. If `dest` is an existing FIOBJ string, the JSON is **appended** to it; otherwise a new string is created. Pass `beautify != 0` for indented output.

```c
FIOBJ obj = fiobj_json_parse2("{\"x\":1,\"y\":[2,3]}", 18, NULL);
FIOBJ json = fiobj2json(FIOBJ_INVALID, obj, 1);   /* beautified */
printf("%s\n", fiobj2cstr(json).buf);
fiobj_free(json);
fiobj_free(obj);
```

### Parse JSON

```c
/* parse a full JSON value */
FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed);

/* convenience macro */
#define fiobj_json_parse2(data, len, consumed) \
  fiobj_json_parse(FIO_STR_INFO2(data, len), consumed)
```

Returns `FIOBJ_INVALID` on parse error. If `consumed` is non-NULL it is set to the number of bytes read.

### Merge JSON into a Hash Map

```c
size_t fiobj_hash_update_json(FIOBJ hash, fio_str_info_s str);
size_t fiobj_hash_update_json2(FIOBJ hash, char *ptr, size_t len);
```

Parses JSON and merges the resulting key-value pairs into an existing hash map. Silently skips non-object JSON data. Returns bytes consumed (0 on error).

### JavaScript-style path lookup

```c
FIOBJ fiobj_json_find(FIOBJ object, fio_str_info_s notation);
#define fiobj_json_find2(object, str, length) \
  fiobj_json_find(object, FIO_STR_INFO2(str, length))
```

Finds a nested value using dot/bracket notation. For example `"[0].name"` returns the `name` field of the first array element. Returns a **temporary reference** — call `fiobj_dup` if you need to keep it.

```c
FIOBJ data = fiobj_json_parse2("[{\"name\":\"Bob\"}]", 16, NULL);
FIOBJ name = fiobj_json_find2(data, "[0].name", 8);
printf("%s\n", fiobj2cstr(name).buf);   /* Bob */
fiobj_free(data);
/* name was a temporary reference into data; do not free separately */
```

---

## Mustache Templates

Renders a compiled Mustache template using FIOBJ data (typically a Hash Map) as context.

```c
/* create a new FIOBJ string with the rendered output */
FIOBJ fiobj_mustache_build(fio_mustache_s *m, FIOBJ ctx);

/* append rendered output to an existing FIOBJ string */
FIOBJ fiobj_mustache_build2(fio_mustache_s *m, FIOBJ dest, FIOBJ ctx);
```

Both may return `FIOBJ_INVALID` if nothing was written and `dest` was empty/invalid. See the Mustache module for how to compile a template (`fio_mustache_s *`).

---

## Extending FIOBJ

Custom types integrate via a virtual function table tagged as `FIOBJ_T_OTHER`.

### Requirements

1. Choose a **unique** type ID ≥ 100. Values below 100 are reserved; values below 40 are illegal.
2. Populate a `FIOBJ_class_vtable_s` — all function pointers must be non-NULL.
3. Wrap the type with `FIO_REF_NAME` (see [249 reference counter.md](./249 reference counter.md)), setting `FIO_REF_METADATA` to `const FIOBJ_class_vtable_s *` and initialising it in `FIO_REF_METADATA_INIT`.
4. Apply pointer tagging so all pointers carry `FIOBJ_T_OTHER` in the low 3 bits.

```c
#define FIO_PTR_TAG(p)          FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p)        FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE        FIOBJ
```

### Virtual function table

```c
typedef struct {
  size_t type_id;                          /* unique ID ≥ 100 */
  unsigned char (*is_eq)(FIOBJ a, FIOBJ b);
  fio_str_info_s (*to_s)(FIOBJ o);
  intptr_t       (*to_i)(FIOBJ o);
  double         (*to_f)(FIOBJ o);
  uint32_t       (*count)(FIOBJ o);        /* number of child elements */
  uint32_t       (*each1)(FIOBJ o, int (*task)(fiobj_each_s *), void *udata, int32_t start_at);
  void           (*free2)(FIOBJ o);        /* free when ref count reaches zero */
} FIOBJ_class_vtable_s;
```

All slots must be filled. If the type has no children, `count` should return `0` and `each1` should be a no-op that returns `0`.

### Minimal example

```c
#define FIOBJ_T_POINT 100UL

typedef struct { double x, y; } point_data_s;

static unsigned char point_eq(FIOBJ a, FIOBJ b) { /* ... */ return 0; }
static fio_str_info_s point_to_s(FIOBJ o)        { /* ... */ return (fio_str_info_s){0}; }
static intptr_t point_to_i(FIOBJ o)              { return 0; }
static double   point_to_f(FIOBJ o)              { return 0.0; }
static uint32_t point_count(FIOBJ o)             { return 0; (void)o; }
static uint32_t point_each1(FIOBJ o, int (*t)(fiobj_each_s *), void *u, int32_t s)
                                                 { return 0; (void)o;(void)t;(void)u;(void)s; }
static void     point_free2(FIOBJ o);            /* forward declaration */

static const FIOBJ_class_vtable_s FIOBJ___POINT_VTBL = {
    .type_id = FIOBJ_T_POINT,
    .is_eq   = point_eq,   .to_s  = point_to_s,
    .to_i    = point_to_i, .to_f  = point_to_f,
    .count   = point_count, .each1 = point_each1,
    .free2   = point_free2,
};

#define FIO_REF_NAME             fiobj_point
#define FIO_REF_TYPE             point_data_s
#define FIO_REF_CONSTRUCTOR_ONLY
#define FIO_REF_METADATA         const FIOBJ_class_vtable_s *
#define FIO_REF_METADATA_INIT(m) (m = &FIOBJ___POINT_VTBL)
#define FIO_PTR_TAG(p)           FIOBJ_PTR_TAG(p, FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p)         FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE         FIOBJ
#include "fio-stl.h"

static void point_free2(FIOBJ o) { fiobj_point_free(o); }

FIOBJ fiobj_point_new_xy(double x, double y) {
    FIOBJ o = fiobj_point_new();
    point_data_s *p = (point_data_s *)FIOBJ_PTR_UNTAG(o);
    p->x = x; p->y = y;
    return o;
}
```

---

## Debugging / Leak Detection

When compiled with `TEST`, `DEBUG`, or `FIO_LEAK_COUNTER` defined, two global counters track allocations:

```c
size_t FIOBJ_MARK_MEMORY_ALLOC_COUNTER;
size_t FIOBJ_MARK_MEMORY_FREE_COUNTER;
```

Call `FIOBJ_MARK_MEMORY_PRINT()` after all objects should be freed to log any leaks.
