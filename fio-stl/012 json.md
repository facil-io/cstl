# JSON Parser

```c
#define FIO_JSON
#include "fio-stl.h"
```

A non-strict, streaming, callback-based JSON parser. It does not build a tree for you; it calls your callbacks for each value and lets you decide what to construct. Implemented in [`./012 json.h`](./012%20json.h). Depends on `FIO_ATOL` and `FIO_STR` (string core, used to decode object keys); both are included automatically.

The parser tolerates trailing commas, comments (`//`, `/* */`, `#`), newlines inside strings, hex/octal/binary numbers, `NaN`, and `Infinity`. Numbers are parsed with `fio_aton`: floats are correctly rounded and locale independent, and `0b...f` spells a float's raw IEEE 754 bits.

### Configuration Macros

#### `FIO_JSON_MAX_DEPTH`

```c
#if !defined(FIO_JSON_MAX_DEPTH) || (FIO_JSON_MAX_DEPTH + 0) == 0
#undef FIO_JSON_MAX_DEPTH
#define FIO_JSON_MAX_DEPTH 128
#endif
```

Allows up to `FIO_JSON_MAX_DEPTH` open containers, counting the root as level 1 (default 128). Zero selects the default; scalar roots are allowed at any setting. Must be less than `65536`.

The limit sizes per-parse state (one pointer plus one byte per level); deeper input aborts with an error.

### Error Codes

```c
/** Malformed JSON and Generic Errors */
#define FIO_JSON_ERROR_GENERIC (-1)
/** Nesting level too deep error */
#define FIO_JSON_ERROR_NESTING (-2)
```

Errors returned by the parser itself. Any other non-zero value is a callback error, returned unchanged. Callbacks may return these values too; the parser doesn't rewrite them, so their origin is not distinguished.

### Types

#### `fio_json_obj_result_s`

```c
typedef struct {
  void *obj;
  int err;
} fio_json_obj_result_s;
```

Returned by the value callbacks, by `map_push` and by the `fio_json_resolve_on_*` helpers. `obj` is the produced object (for `map_push`, the replaced value). `err` is `0` on success or any non-zero value on failure; on failure `obj` is ignored.

#### `fio_json_push_result_s`

```c
typedef struct {
  void *obj;
  void *old;
  int err;
} fio_json_push_result_s;
```

Returned by `on_map` and `on_array`. `obj` is the opened container, `old` is a value the callback replaced and hands back for conflict resolution (or NULL), and `err` is `0` or any non-zero error. On failure `obj` and `old` are ignored.

#### `fio_json_parser_callbacks_s`

```c
typedef struct FIO_ALIGN(64) {
  fio_json_obj_result_s (*on_null)(void *udata);
  fio_json_obj_result_s (*on_true)(void *udata);
  fio_json_obj_result_s (*on_false)(void *udata);
  fio_json_obj_result_s (*on_number)(void *udata, int64_t i);
  fio_json_obj_result_s (*on_float)(void *udata, double f);
  fio_json_obj_result_s (*on_string)(void *udata, fio_buf_info_s str);
  fio_json_obj_result_s (*on_string_simple)(void *udata, fio_buf_info_s str);
  fio_json_push_result_s (*on_map)(void *udata, void *parent, fio_buf_info_s key);
  fio_json_push_result_s (*on_array)(void *udata, void *parent, fio_buf_info_s key);
  fio_json_obj_result_s (*map_push)(void *udata, void *map, fio_buf_info_s key,
                                    void *value);
  int (*array_push)(void *udata, void *array, void *value);
  int (*is_array)(void *udata, void *obj);
  int (*is_map)(void *udata, void *obj);
  void (*free_unused_object)(void *udata, void *obj);
} fio_json_parser_callbacks_s;
```

The parser's callback table. Each callback receives the per-call `udata`. The parser never writes to the table, so it can be `static const`; it copies the table and fills defaults in its private copy.

**Required callbacks:** `on_null`, `on_true`, `on_false`, `on_number`, `on_float`, at least one of `on_string` and `on_string_simple`, `on_map`, `on_array`, `map_push`, `array_push`, and `free_unused_object`. A missing required callback fails the parse before any input is read.

**Optional callbacks:** a missing string callback is replaced by the other one, which must then handle both forms. A missing `is_map` / `is_array` always returns false; both are needed to update an existing root and to resolve conflicts with existing containers.

**Strings and keys:** string *values* are passed undecoded: `on_string` receives bytes that contain escapes, and `on_string_simple` receives bytes without escapes. Decoding is up to the callback, so a value is allocated only once. Object keys don't call the string callbacks; the parser decodes them into a borrowed `key`, valid only during the callback. Copy the bytes to keep them. A non-NULL `key.buf` with zero length is the empty key `""`.

**Containers:** `on_map(udata, parent, key)` and `on_array(udata, parent, key)` open a container at `parent[key]`:
- `key.buf == NULL` appends to an array `parent`.
- `parent == NULL` creates an owned root.
- Otherwise the callback places the container at `parent[key]` and returns it borrowed.

The callback picks the strategy (for example a lookup, or a "set if missing"). Returning an existing container of the same type merges maps and appends to arrays. A value the callback replaces is either freed by the callback (overwrite), returned in `old` (the default rules below apply), or handled by any custom policy.

**Ownership:** value callbacks return owned objects. Ownership always transfers: `map_push` and `array_push` take `value` even when they fail, and the parser never frees a value after passing it to a callback. `map_push` frees a replaced value itself or returns it in `obj` ("the object to keep if possible") for conflict resolution. `free_unused_object` frees owned, unused values (never keys) and may receive NULL.

**NULL values:** a native NULL `obj` / `old` means "nothing". A value callback may return NULL as a value (for example, from `on_null`), and the parser stores it: `map_push` and `array_push` receive it. A key with no value (`{"a":}`) also stores NULL, while the empty slot after a trailing comma (`[1,]`) adds nothing. A replaced NULL never triggers conflict resolution. To resolve null conflicts by the rules, return a dedicated null object (FIOBJ uses `fiobj_null()`).

**Errors:** any non-zero `err` (or `array_push` return value) stops the parse and is returned unchanged in `fio_json_result_s.err`. Conflict helpers descend iteratively through at most `FIO_JSON_MAX_DEPTH` containers per call. Longer conflict chains return `FIO_JSON_ERROR_NESTING`, even when duplicate keys kept the input nesting shallow.

**Conflict rules:** when a callback hands a replaced value back (in `old`, or in the `obj` of `map_push`), the parser applies these rules (an upload or any other non-map, non-array object counts as a scalar):

| Incoming | Existing | Result |
|---|---|---|
| map | map | merge (when `on_map` reuses the existing map) |
| array | array | append (when `on_array` reuses the existing array) |
| map | scalar or array | new map with the existing value at the empty key `""` |
| array | scalar | new array with the existing value first |
| array | map | keep the map; put the incoming array at its `""` key, by the same rules |
| scalar | scalar | array of `[existing, incoming]` |
| scalar | array | append the incoming value |
| scalar | map | keep the map; put the incoming value at its `""` key, by the same rules |

#### `fio_json_result_s`

```c
typedef struct {
  void *ctx;
  size_t stop_pos;
  int err;
} fio_json_result_s;
```

The parse result.

**Members:**
- `ctx` - the root object, owned by the caller: the complete root on success, the partial root on failure, or the `root` passed to `fio_json_parse`.
- `stop_pos` - byte offset where parsing stopped (zero when callback validation fails).
- `err` - `0` on success, `FIO_JSON_ERROR_GENERIC`, `FIO_JSON_ERROR_NESTING`, or an unchanged callback error.

### API Functions

#### `fio_json_parse`

```c
SFUNC fio_json_result_s fio_json_parse(const fio_json_parser_callbacks_s *cb,
                                       void *udata,
                                       void *root,
                                       const char *json_string,
                                       const size_t len);
```

Parses one JSON value from `json_string` (up to `len` bytes) and stops once the value is complete or an error occurs.

**Parameters:**
- `cb` - the callback table (never written).
- `udata` - per-call user data passed to every callback.
- `root` - NULL, or an existing map / array to update in place.
- `json_string` - the input buffer.
- `len` - the input length in bytes.

When `root` is NULL, the parser creates the root through `on_map` / `on_array` with `parent == NULL`, or returns a scalar root.

When `root` is set, the JSON root must be a container of the same type, checked with `is_map` / `is_array`. On a mismatch (including a scalar JSON root, which is freed with `free_unused_object`) nothing changes and `err` is `FIO_JSON_ERROR_GENERIC`. Values in the JSON then go into `root` without calling `on_map` / `on_array` for it.

**Returns:** a `fio_json_result_s`. The caller owns `result.ctx` on success and on failure (see above). When a required callback is missing, `err` is `FIO_JSON_ERROR_GENERIC`, `ctx` is `root` and `stop_pos` is zero.

**Note:** a UTF-8 BOM at the start of the buffer is skipped.

**Guard-byte contract:** number and quote-less key scanning may read the byte at `json_string[len]` while deciding a token ended (see the `fio_atol*` guard-byte contract). Callers MUST keep that byte readable and non-numeric: pass a NUL-terminated string (any `fio_bstr` string qualifies) or append a non-numeric guard byte. Exact-size buffers with no readable byte past `len` violate the contract and will trigger AddressSanitizer.

#### `fio_json_resolve_on_map`

```c
SFUNC fio_json_obj_result_s fio_json_resolve_on_map(
    const fio_json_parser_callbacks_s *cb,
    void *udata,
    void *parent,
    fio_buf_info_s key);
```

Opens a map through `cb->on_map` and resolves a returned `old` value with the default conflict rules. The parser uses it; other parsers (such as the HTTP form parsers) can call it to apply the same rules.

**Parameters:** `cb` - the callback table; `udata` - callback user data; `parent` - the borrowed parent, or NULL for a root; `key` - the borrowed key, or a NULL buffer to append to a parent array.

**Requires:** `on_map`, `on_array`, `map_push`, `array_push`, `free_unused_object`. A NULL `is_map` / `is_array` counts as false.

**Returns:** the map in `obj` (borrowed from `parent`, or owned when `parent` is NULL), or a non-zero `err`: the unchanged callback error, or `FIO_JSON_ERROR_GENERIC` when `on_map` returns no map.

#### `fio_json_resolve_on_array`

```c
SFUNC fio_json_obj_result_s fio_json_resolve_on_array(
    const fio_json_parser_callbacks_s *cb,
    void *udata,
    void *parent,
    fio_buf_info_s key);
```

Opens an array through `cb->on_array`, resolving a returned `old` value like `fio_json_resolve_on_map`. Same parameters, requirements and return values.

#### `fio_json_resolve_map_push`

```c
SFUNC int fio_json_resolve_map_push(const fio_json_parser_callbacks_s *cb,
                                    void *udata,
                                    void *map,
                                    fio_buf_info_s key,
                                    void *value);
```

Stores `value` at `map[key]` through `cb->map_push` and resolves a returned replaced value with the default conflict rules.

**Parameters:** `cb` - the callback table; `udata` - callback user data; `map` - the borrowed map; `key` - the borrowed key; `value` - an owned value, taken even on failure.

**Requires:** `map_push`, `on_array`, `array_push`, `free_unused_object`. A NULL `is_map` / `is_array` counts as false.

**Returns:** `0` on success, the unchanged callback error, or `FIO_JSON_ERROR_GENERIC` when `on_array` returns no array.

#### `fio_json_callbacks_validate`

```c
SFUNC int fio_json_callbacks_validate(const fio_json_parser_callbacks_s *cb);
```

Checks that every required callback is set (see **Required callbacks** above). It never writes to the table. `fio_json_parse` calls it itself; other parsers call it before using the `fio_json_resolve_*` helpers.

**Returns:** `0` if the table is valid, or `-1` if a required callback is missing (or `cb` is NULL).

### Example

The callbacks below hand replaced values back, so duplicate scalar keys become arrays. They copy keys before returning, and push callbacks free the value when they fail. The second `fio_json_parse` call updates the existing root, appending to its `items` array. The example uses `assert` to keep allocation handling short; production callbacks should report allocation failures with a non-zero `err`.

```c
#define FIO_JSON
#include "fio-stl.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { V_NULL, V_BOOL, V_NUM, V_FLOAT, V_STR, V_MAP, V_ARRAY } kind_e;
typedef struct value_s value_s;
struct value_s {
  kind_e kind;
  union {
    int64_t number;
    double floating;
    int boolean;
    struct { char *buf; size_t len; } str;
    struct { value_s **items; size_t len; } array;
    struct { char **keys; size_t *lengths; value_s **values; size_t len; } map;
  } as;
};
#define OK(v) ((fio_json_obj_result_s){.obj = (v)})
#define FAIL(e) ((fio_json_obj_result_s){.err = (e)})
static value_s *make(kind_e kind) {
  value_s *v = calloc(1, sizeof(*v));
  assert(v);
  v->kind = kind;
  return v;
}
static void release(value_s *v) {
  if (!v) return;
  if (v->kind == V_STR) free(v->as.str.buf);
  if (v->kind == V_ARRAY) {
    for (size_t i = 0; i < v->as.array.len; ++i) release(v->as.array.items[i]);
    free(v->as.array.items);
  }
  if (v->kind == V_MAP) {
    for (size_t i = 0; i < v->as.map.len; ++i) {
      free(v->as.map.keys[i]);
      release(v->as.map.values[i]);
    }
    free(v->as.map.keys);
    free(v->as.map.lengths);
    free(v->as.map.values);
  }
  free(v);
}
static size_t find(value_s *map, fio_buf_info_s key) {
  for (size_t i = 0; i < map->as.map.len; ++i)
    if (map->as.map.lengths[i] == key.len &&
        !memcmp(map->as.map.keys[i], key.buf, key.len)) return i;
  return map->as.map.len;
}
/* Takes `item`, even on failure. */
static int array_push(void *u, void *ctx, void *item) {
  (void)u;
  value_s *v = ctx;
  size_t n = v->as.array.len;
  value_s **p = realloc(v->as.array.items, (n + 1) * sizeof(*p));
  if (!p) { release(item); return -1; }
  v->as.array.items = p;
  v->as.array.items[n] = item;
  v->as.array.len = n + 1;
  return 0;
}
/* Takes `item`, even on failure; hands a replaced value back in `obj`. */
static fio_json_obj_result_s map_push(void *u, void *ctx,
                                      fio_buf_info_s key, void *item) {
  (void)u;
  value_s *v = ctx;
  size_t n = find(v, key);
  if (n != v->as.map.len) {
    value_s *old = v->as.map.values[n];
    v->as.map.values[n] = item;
    return OK(old); /* the parser resolves the conflict */
  }
  char *copy = malloc(key.len + 1); /* keys are borrowed: copy them */
  if (!copy) { release(item); return FAIL(-1); }
  if (key.len) memcpy(copy, key.buf, key.len);
  copy[key.len] = 0;
  char **keys = realloc(v->as.map.keys, (n + 1) * sizeof(*keys));
  assert(keys);
  v->as.map.keys = keys;
  size_t *lengths = realloc(v->as.map.lengths, (n + 1) * sizeof(*lengths));
  assert(lengths);
  v->as.map.lengths = lengths;
  value_s **values = realloc(v->as.map.values, (n + 1) * sizeof(*values));
  assert(values);
  v->as.map.values = values;
  v->as.map.keys[n] = copy;
  v->as.map.lengths[n] = key.len;
  v->as.map.values[n] = item;
  v->as.map.len = n + 1;
  return OK(NULL);
}
static fio_json_push_result_s open_container(void *udata, void *ctx,
                                             fio_buf_info_s key, kind_e kind) {
  fio_json_push_result_s r = {0};
  value_s *parent = ctx;
  if (parent && key.buf) {
    size_t i = find(parent, key);
    if (i != parent->as.map.len && parent->as.map.values[i]->kind == kind) {
      r.obj = parent->as.map.values[i]; /* reuse: merge / append */
      return r;
    }
  }
  value_s *v = make(kind);
  if (!parent) { /* an owned root */
    r.obj = v;
  } else if (!key.buf) { /* append to a parent array */
    r.err = array_push(udata, parent, v);
    r.obj = v;
  } else { /* a replaced value is handed back in `old` */
    fio_json_obj_result_s p = map_push(udata, parent, key, v);
    r.err = p.err;
    r.obj = v;
    r.old = p.obj;
  }
  return r;
}
static fio_json_push_result_s on_map(void *u, void *ctx, fio_buf_info_s key) {
  return open_container(u, ctx, key, V_MAP);
}
static fio_json_push_result_s on_array(void *u, void *ctx, fio_buf_info_s key) {
  return open_container(u, ctx, key, V_ARRAY);
}
static fio_json_obj_result_s on_null(void *u) { (void)u; return OK(make(V_NULL)); }
static fio_json_obj_result_s on_bool(int b) {
  value_s *v = make(V_BOOL);
  v->as.boolean = b;
  return OK(v);
}
static fio_json_obj_result_s on_true(void *u) { (void)u; return on_bool(1); }
static fio_json_obj_result_s on_false(void *u) { (void)u; return on_bool(0); }
static fio_json_obj_result_s on_number(void *u, int64_t n) {
  (void)u; value_s *v = make(V_NUM); v->as.number = n; return OK(v);
}
static fio_json_obj_result_s on_float(void *u, double n) {
  (void)u; value_s *v = make(V_FLOAT); v->as.floating = n; return OK(v);
}
/* Also serves as `on_string_simple`: unescaping is a copy without escapes. */
static fio_json_obj_result_s on_string(void *u, fio_buf_info_s s) {
  (void)u;
  value_s *v = make(V_STR);
  fio_str_info_s out = FIO_STR_INFO3(malloc(s.len + 1), 0, s.len + 1);
  assert(out.buf);
  fio_string_write_unescape(&out, NULL, s.buf, s.len);
  v->as.str.buf = out.buf;
  v->as.str.len = out.len;
  return OK(v);
}
static int is_map(void *u, void *ctx) { (void)u; return ((value_s *)ctx)->kind == V_MAP; }
static int is_array(void *u, void *ctx) { (void)u; return ((value_s *)ctx)->kind == V_ARRAY; }
static void free_unused(void *u, void *ctx) { (void)u; release(ctx); }

static const fio_json_parser_callbacks_s CALLBACKS = {
    .on_null = on_null, .on_true = on_true, .on_false = on_false,
    .on_number = on_number, .on_float = on_float, .on_string = on_string,
    .on_map = on_map, .on_array = on_array, .map_push = map_push,
    .array_push = array_push, .is_map = is_map, .is_array = is_array,
    .free_unused_object = free_unused,
};

int main(void) {
  const char *json = "{\"a\\u0062\":1,\"ab\":2,\"items\":[3]}";
  fio_json_result_s r = fio_json_parse(&CALLBACKS, NULL, NULL, json, strlen(json));
  if (r.err) {
    fprintf(stderr, "parse failed (%d) at %zu\n", r.err, r.stop_pos);
    release(r.ctx); /* the caller owns the partial root */
    return 1;
  }
  value_s *root = r.ctx;
  int ok = 1;
  value_s *pair = root->as.map.values[find(root, FIO_BUF_INFO1("ab"))];
  ok &= pair->kind == V_ARRAY && pair->as.array.len == 2; /* [1, 2] */
  /* update the existing root in place */
  const char *update = "{\"items\":[4]}";
  r = fio_json_parse(&CALLBACKS, NULL, root, update, strlen(update));
  ok &= !r.err && r.ctx == root;
  value_s *list = root->as.map.values[find(root, FIO_BUF_INFO1("items"))];
  ok &= list->kind == V_ARRAY && list->as.array.len == 2; /* [3, 4] */
  /* the JSON root must match the existing root's type: nothing changes */
  r = fio_json_parse(&CALLBACKS, NULL, root, "[5]", 3);
  ok &= r.err == FIO_JSON_ERROR_GENERIC && r.ctx == root;
  release(root);
  return !ok;
}
#ifndef FIO_EXTERN_COMPLETE
#define FIO_EXTERN_COMPLETE
#include "fio-stl.h"
#endif
```

------------------------------------------------------------
