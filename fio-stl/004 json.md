# JSON Parser

```c
#define FIO_JSON
#include "fio-stl.h"
```

A non-strict, streaming, callback-based JSON parser. It does not build a tree for you; it calls your callbacks for each value and lets you decide what to construct. Implemented in [`./004 json.h`](./004%20json.h). Depends on `FIO_ATOL`, which is included automatically.

The parser tolerates trailing commas, comments (`//`, `/* */`, `#`), newlines inside strings, hex/octal/binary numbers, `NaN`, and `Infinity`.

### Configuration Macros

#### `FIO_JSON_MAX_DEPTH`

```c
#ifndef FIO_JSON_MAX_DEPTH
#define FIO_JSON_MAX_DEPTH 128
#endif
```

Maximum JSON nesting depth. Must be less than `65536`. Deeper input aborts with an error.

#### `FIO_JSON_USE_FIO_ATON`

```c
#ifndef FIO_JSON_USE_FIO_ATON
#define FIO_JSON_USE_FIO_ATON 0
#endif
```

Set to `1` to use `fio_aton` for number parsing instead of the built-in logic.

### Types

#### `fio_json_parser_callbacks_s`

```c
typedef struct {
  void *(*on_null)(void *udata);
  void *(*on_true)(void *udata);
  void *(*on_false)(void *udata);
  void *(*on_number)(void *udata, int64_t i);
  void *(*on_float)(void *udata, double f);
  void *(*on_string)(void *udata, const void *start, size_t len);
  void *(*on_string_simple)(void *udata, const void *start, size_t len);
  void *(*on_map)(void *udata, void *ctx, void *at);
  void *(*on_array)(void *udata, void *ctx, void *at);
  int (*map_push)(void *udata, void *ctx, void *key, void *value);
  int (*array_push)(void *udata, void *ctx, void *value);
  int (*array_finished)(void *udata, void *ctx);
  int (*map_finished)(void *udata, void *ctx);
  int (*is_array)(void *udata, void *ctx);
  int (*is_map)(void *udata, void *ctx);
  void (*free_unused_object)(void *udata, void *ctx);
  void *(*on_error)(void *udata, void *ctx);
} fio_json_parser_callbacks_s;
```

Callback table for the parser. All callbacks receive `udata` as their first argument, so the same `static const` table can be shared across concurrent calls.

**Required callbacks:** `on_null`, `on_true`, `on_false`, `on_number`, `on_float`, `on_string` (or `on_string_simple`), `on_map`, `on_array`, `map_push`, `array_push`, `free_unused_object`.

**Optional callbacks:** `on_string_simple`, `array_finished`, `map_finished`, `is_array`, `is_map`, `on_error`. Missing optional callbacks default to no-ops. If only one of `on_string` / `on_string_simple` is provided, the other uses it as a fallback.

**Ownership:** callbacks that return `void *` transfer ownership to the parser. The parser passes objects to `map_push` / `array_push` or calls `free_unused_object` for discarded objects. `on_error` receives ownership of any partial result.

#### `fio_json_result_s`

```c
typedef struct {
  void *ctx;
  size_t stop_pos;
  int err;
} fio_json_result_s;
```

Parse result.

**Members:**
- `ctx` - root object returned by callbacks, or `NULL`
- `stop_pos` - number of bytes consumed from the input
- `err` - non-zero if parsing stopped because of an error

### API Functions

#### `fio_json_parse`

```c
SFUNC fio_json_result_s fio_json_parse(fio_json_parser_callbacks_s *settings,
                                       void *udata,
                                       const char *json_string,
                                       const size_t len);
```

Parses `json_string` up to `len` bytes. Stops as soon as a complete top-level value is consumed or an error occurs.

**Parameters:**
- `settings` - callback table
- `udata` - per-call user data threaded through every callback
- `json_string` - input buffer
- `len` - input length in bytes

**Returns:** `fio_json_result_s` with the root context, bytes consumed, and error flag.

**Note:** a UTF-8 BOM at the start of the buffer is skipped automatically.

#### `fio_json_parse_update`

```c
SFUNC fio_json_result_s fio_json_parse_update(fio_json_parser_callbacks_s *s,
                                              void *udata,
                                              void *ctx,
                                              const char *start,
                                              const size_t len);
```

Merges JSON data into an existing object (`ctx`). The input must be wrapped in an aggregate of the same type as `ctx`. Requires `is_array` and `is_map` callbacks.

**Parameters:**
- `s` - callback table
- `udata` - per-call user data
- `ctx` - existing object to update
- `start` - input buffer
- `len` - input length

**Returns:** parse result with the (possibly updated) context.

### Example

```c
#define FIO_JSON
#define FIO_LOG
#include "fio-stl.h"

typedef enum { T_NULL, T_TRUE, T_FALSE, T_NUM, T_STR, T_ARR, T_MAP } type_e;

typedef struct obj_s {
  type_e type;
  union {
    int64_t i;
    struct { char *buf; size_t len; } s;
    struct { struct obj_s **items; size_t len; } a;
    struct { struct obj_s **keys; struct obj_s **vals; size_t len; } m;
  };
} obj_s;

static obj_s *mk(type_e t) {
  obj_s *o = calloc(1, sizeof(*o));
  o->type = t;
  return o;
}

static void *on_null(void *u) { (void)u; return mk(T_NULL); }
static void *on_true(void *u) { (void)u; return mk(T_TRUE); }
static void *on_false(void *u) { (void)u; return mk(T_FALSE); }
static void *on_number(void *u, int64_t i) { (void)u; obj_s *o = mk(T_NUM); o->i = i; return o; }
static void *on_string(void *u, const void *s, size_t l) {
  (void)u; obj_s *o = mk(T_STR); o->s.buf = malloc(l+1); memcpy(o->s.buf, s, l); o->s.buf[l] = 0; o->s.len = l; return o;
}
static void *on_array(void *u, void *c, void *a) { (void)u; (void)c; (void)a; return mk(T_ARR); }
static void *on_map(void *u, void *c, void *a) { (void)u; (void)c; (void)a; return mk(T_MAP); }
static int array_push(void *u, void *c, void *v) {
  (void)u; obj_s *a = c; a->a.items = realloc(a->a.items, (a->a.len+1)*sizeof(*a->a.items)); a->a.items[a->a.len++] = v; return 0;
}
static int map_push(void *u, void *c, void *k, void *v) {
  (void)u; obj_s *m = c;
  m->m.keys = realloc(m->m.keys, (m->m.len+1)*sizeof(*m->m.keys));
  m->m.vals = realloc(m->m.vals, (m->m.len+1)*sizeof(*m->m.vals));
  m->m.keys[m->m.len] = k; m->m.vals[m->m.len] = v; m->m.len++; return 0;
}
static void free_obj(void *u, void *c) { (void)u; free(c); }

int main(void) {
  static const fio_json_parser_callbacks_s cb = {
    .on_null = on_null, .on_true = on_true, .on_false = on_false,
    .on_number = on_number, .on_string = on_string, .on_string_simple = on_string,
    .on_array = on_array, .on_map = on_map,
    .array_push = array_push, .map_push = map_push,
    .free_unused_object = free_obj,
  };
  const char *json = "{ \"n\": 42, \"s\": \"hi\", \"a\": [1, 2] }";
  fio_json_result_s r = fio_json_parse(&cb, NULL, json, strlen(json));
  if (r.err) {
    FIO_LOG_ERROR("parse error at %zu", r.stop_pos);
    return 1;
  }
  printf("parsed %zu bytes, root ctx %p\n", r.stop_pos, r.ctx);
  return 0;
}
```

------------------------------------------------------------
