## Custom JSON Parser

```c
#define FIO_JSON
#include "fio-stl.h"
```

The facil.io JSON parser is a non-strict parser, with support for trailing commas in collections, new-lines in strings, extended escape characters, comments, and common numeral formats (octal, hex and binary).

The facil.io JSON parser should be considered **unsafe** as overflow protection depends on the `NUL` character appearing at the end of the string passed to the parser.

**Note**: this module depends on the `FIO_ATOL` module which will be automatically included.

### Configuration Macros

#### `FIO_JSON_MAX_DEPTH`

```c
#ifndef FIO_JSON_MAX_DEPTH
/** Maximum allowed JSON nesting level. MUST be less than 65536. */
#define FIO_JSON_MAX_DEPTH 128
#endif
```

To ensure the program's stack isn't abused, the parser will limit JSON nesting levels to a customizable `FIO_JSON_MAX_DEPTH` number of nesting levels.

Values above 65536 might cause the stack to overflow and cause a failure.

#### `FIO_JSON_USE_FIO_ATON`

```c
#ifndef FIO_JSON_USE_FIO_ATON
#define FIO_JSON_USE_FIO_ATON 0
#endif
```

When set to `1`, the JSON parser will use `fio_aton` for number parsing instead of the default number parsing logic.

The default value is `0`.

### Types

#### `fio_json_parser_callbacks_s`

```c
typedef struct {
  /** NULL object was detected. Returns new object as `void *`. */
  void *(*on_null)(void *udata);
  /** TRUE object was detected. Returns new object as `void *`. */
  void *(*on_true)(void *udata);
  /** FALSE object was detected. Returns new object as `void *`. */
  void *(*on_false)(void *udata);
  /** Number was detected (long long). Returns new object as `void *`. */
  void *(*on_number)(void *udata, int64_t i);
  /** Float was detected (double). Returns new object as `void *`. */
  void *(*on_float)(void *udata, double f);
  /** (escaped) String was detected. Returns a new String as `void *`. */
  void *(*on_string)(void *udata, const void *start, size_t len);
  /** (unescaped) String was detected. Returns a new String as `void *`. */
  void *(*on_string_simple)(void *udata, const void *start, size_t len);
  /** Dictionary was detected. Returns ctx to hash map or NULL on error. */
  void *(*on_map)(void *udata, void *ctx, void *at);
  /** Array was detected. Returns ctx to array or NULL on error. */
  void *(*on_array)(void *udata, void *ctx, void *at);
  /** Map entry detected. Returns non-zero on error. Owns key and value. */
  int (*map_push)(void *udata, void *ctx, void *key, void *value);
  /** Array entry detected. Returns non-zero on error. Owns value. */
  int (*array_push)(void *udata, void *ctx, void *value);
  /** Called when an array object (`ctx`) appears done. */
  int (*array_finished)(void *udata, void *ctx);
  /** Called when a map object (`ctx`) appears done. */
  int (*map_finished)(void *udata, void *ctx);
  /** Called when context is expected to be an array (i.e., fio_json_parse_update). */
  int (*is_array)(void *udata, void *ctx);
  /** Called when context is expected to be a map (i.e., fio_json_parse_update). */
  int (*is_map)(void *udata, void *ctx);
  /** Called for unused objects (e.g., key on error). Must free the object. */
  void (*free_unused_object)(void *udata, void *ctx);
  /** The JSON parsing encountered an error. Owns ctx, should free or return. */
  void *(*on_error)(void *udata, void *ctx);
} fio_json_parser_callbacks_s;
```

The JSON parser requires certain callbacks to create objects or perform actions based on JSON data.

Every callback receives `udata` as its first argument — the per-call user data pointer passed to `fio_json_parse()`. This allows the same `fio_json_parser_callbacks_s` instance to be shared (even as a `static const`) across many concurrent callers without any per-call state stored in the struct.

**Ownership**: Callbacks that return `void *` objects transfer ownership to the parser. The parser will either pass these objects to `map_push` / `array_push` (transferring ownership to the container), or call `free_unused_object` if the object is not used (e.g., on error or NULL map key). The `on_error` callback receives ownership of any partial result.

**Required Callbacks:**

The following callbacks **MUST** be provided to the parser:

- `on_null` - `NULL` object was detected. Returns NULL object as `void *`.

- `on_true` - `true` object was detected. Returns TRUE object as `void *`.

- `on_false` - `false` object was detected. Returns FALSE object as `void *`.

- `on_number` - Number was detected (`int64_t`). Returns new number object as `void *`.

- `on_float` - Float was detected (`double`). Returns new float object as `void *`.

- `on_string` - Escaped string was detected. `start` points to a JSON escaped String (remember to unescape). Returns a new String as `void *`.

- `on_map` - Dictionary was detected. `ctx` is the current context, `at` is the key (if any). Returns new `ctx` to hash map or `NULL` on error.

- `on_array` - Array was detected. `ctx` is the current context, `at` is the key (if any). Returns new `ctx` to array or `NULL` on error.

- `map_push` - Pushes a key-value pair to a dictionary. Returns non-zero on error.

- `array_push` - Pushes a value to an array. Returns non-zero on error.

- `free_unused_object` - Called for unused objects (e.g., a key with no matching value on error). Must free the object.

**Optional Callbacks:**

The following callbacks are optional and will use default no-op implementations if not provided:

- `on_string_simple` - Unescaped string was detected (no escape sequences). If not provided, falls back to `on_string`. If `on_string` is not provided but `on_string_simple` is, `on_string` will use `on_string_simple`.

- `array_finished` - Called when an array object (`ctx`) appears done. Returns non-zero on error. Default: no-op returning 0.

- `map_finished` - Called when a map object (`ctx`) appears done. Returns non-zero on error. Default: no-op returning 0.

- `is_array` - Called to check if context is an array (used by `fio_json_parse_update`). Returns non-zero if true. Default: no-op returning 0.

- `is_map` - Called to check if context is a map (used by `fio_json_parse_update`). Returns non-zero if true. Default: no-op returning 0.

- `on_error` - The JSON parsing encountered an error. Receives the current context and returns what to do with it. Default: returns ctx unchanged.

#### `fio_json_result_s`

```c
typedef struct {
  void *ctx;
  size_t stop_pos;
  int err;
} fio_json_result_s;
```

The JSON return type containing the parsing result.

**Members:**

- `ctx` - The top-most context/object in the JSON stream (the root object)
- `stop_pos` - The number of bytes consumed before parsing stopped
- `err` - Non-zero if the parsing stopped due to an error

### JSON Parser API

#### `fio_json_parse`

```c
fio_json_result_s fio_json_parse(fio_json_parser_callbacks_s *callbacks,
                                 void *udata,
                                 const char *json_string,
                                 const size_t len);
```

Parses a JSON string using the provided callbacks.

The facil.io JSON parser is a non-strict parser, with support for:

- Trailing commas in collections
- New-lines in strings
- Extended escape characters
- Comments (C-style `//` and `/* */`, and shell-style `#`)
- Octal, hex, and binary number formats
- `NaN` and `Infinity` float values

The parser decouples the parsing process from the resulting data-structure by calling the requested callbacks for JSON related events.

**Parameters:**

- `callbacks` - Pointer to the callback structure defining how to handle JSON elements
- `udata` - Per-call user data pointer passed as the first argument to every callback
- `json_string` - The JSON string to parse
- `len` - Length of the JSON string in bytes

**Returns:** A `fio_json_result_s` containing:

- `ctx` - The root object created by the callbacks
- `stop_pos` - Number of bytes consumed (position where parsing stopped)
- `err` - Non-zero if an error occurred

**Note**: `udata` is passed as a function argument rather than stored in the callbacks struct. This allows the same `fio_json_parser_callbacks_s` to be shared (even as a `static const`) across many concurrent callers — the struct stays hot in CPU cache while each call threads its own context through every callback.

**Note**: The parser automatically skips UTF-8 BOM (Byte Order Mark) if present at the beginning of the string.

#### `fio_json_parse_update`

```c
fio_json_result_s fio_json_parse_update(fio_json_parser_callbacks_s *callbacks,
                                        void *udata,
                                        void *ctx,
                                        const char *json_string,
                                        const size_t len);
```

Updates an existing object with data from a JSON string.

Use only when `ctx` is an existing object (array or map) and the JSON data is wrapped in an object of the same type.

This function is useful for merging JSON data into an existing data structure.

**Parameters:**

- `callbacks` - Pointer to the callback structure (must include `is_array` and `is_map` callbacks)
- `udata` - Per-call user data pointer passed as the first argument to every callback
- `ctx` - The existing object to update
- `json_string` - The JSON string containing update data
- `len` - Length of the JSON string in bytes

**Returns:** A `fio_json_result_s` with the updated context.

**Note**: The `is_array` and `is_map` callbacks **MUST** be provided when using this function, as they are used to verify that the existing context matches the JSON structure.

### JSON Parsing Example

The biggest question about parsing JSON is - where do we store the resulting data?

Different parsers solve this question in different ways.

The `FIOBJ` soft-typed object system offers a very effective solution for data manipulation, as it creates a separate object for every JSON element.

However, many parsers store the result in an internal data structure that can't be separated into different elements. These parsers appear faster while actually deferring a lot of the heavy lifting to a later stage.

Here is a short example that parses JSON data and prints it back as minified JSON:

```c
#define FIO_JSON
#define FIO_STR_NAME fio_str
#define FIO_LOG
#include "fio-stl.h"

/* Forward declarations for our object types */
typedef struct my_json_obj_s my_json_obj_s;

/* Simple tagged union for JSON values */
typedef enum {
  MY_JSON_NULL,
  MY_JSON_TRUE,
  MY_JSON_FALSE,
  MY_JSON_NUMBER,
  MY_JSON_FLOAT,
  MY_JSON_STRING,
  MY_JSON_ARRAY,
  MY_JSON_MAP,
} my_json_type_e;

struct my_json_obj_s {
  my_json_type_e type;
  union {
    int64_t i;
    double f;
    fio_str_s str;
    struct {
      my_json_obj_s **items;
      size_t len;
      size_t capa;
    } array;
    struct {
      my_json_obj_s **keys;
      my_json_obj_s **values;
      size_t len;
      size_t capa;
    } map;
  };
};

/* Callback implementations */
static void *my_on_null(void *udata) {
  my_json_obj_s *o = calloc(1, sizeof(*o));
  o->type = MY_JSON_NULL;
  (void)udata;
  return o;
}

static void *my_on_true(void *udata) {
  my_json_obj_s *o = calloc(1, sizeof(*o));
  o->type = MY_JSON_TRUE;
  (void)udata;
  return o;
}

static void *my_on_false(void *udata) {
  my_json_obj_s *o = calloc(1, sizeof(*o));
  o->type = MY_JSON_FALSE;
  (void)udata;
  return o;
}

static void *my_on_number(void *udata, int64_t i) {
  my_json_obj_s *o = calloc(1, sizeof(*o));
  o->type = MY_JSON_NUMBER;
  o->i = i;
  (void)udata;
  return o;
}

static void *my_on_float(void *udata, double f) {
  my_json_obj_s *o = calloc(1, sizeof(*o));
  o->type = MY_JSON_FLOAT;
  o->f = f;
  (void)udata;
  return o;
}

static void *my_on_string(void *udata, const void *start, size_t len) {
  my_json_obj_s *o = calloc(1, sizeof(*o));
  o->type = MY_JSON_STRING;
  /* Note: in production, you'd want to unescape the string */
  fio_str_write(&o->str, start, len);
  (void)udata;
  return o;
}

static void *my_on_array(void *udata, void *ctx, void *at) {
  my_json_obj_s *o = calloc(1, sizeof(*o));
  o->type = MY_JSON_ARRAY;
  o->array.capa = 8;
  o->array.items = calloc(o->array.capa, sizeof(void *));
  (void)udata;
  (void)ctx;
  (void)at;
  return o;
}

static void *my_on_map(void *udata, void *ctx, void *at) {
  my_json_obj_s *o = calloc(1, sizeof(*o));
  o->type = MY_JSON_MAP;
  o->map.capa = 8;
  o->map.keys = calloc(o->map.capa, sizeof(void *));
  o->map.values = calloc(o->map.capa, sizeof(void *));
  (void)udata;
  (void)ctx;
  (void)at;
  return o;
}

static int my_array_push(void *udata, void *ctx, void *value) {
  my_json_obj_s *arr = ctx;
  if (arr->array.len >= arr->array.capa) {
    arr->array.capa *= 2;
    arr->array.items = realloc(arr->array.items, arr->array.capa * sizeof(void *));
  }
  arr->array.items[arr->array.len++] = value;
  (void)udata;
  return 0;
}

static int my_map_push(void *udata, void *ctx, void *key, void *value) {
  my_json_obj_s *map = ctx;
  if (map->map.len >= map->map.capa) {
    map->map.capa *= 2;
    map->map.keys = realloc(map->map.keys, map->map.capa * sizeof(void *));
    map->map.values = realloc(map->map.values, map->map.capa * sizeof(void *));
  }
  map->map.keys[map->map.len] = key;
  map->map.values[map->map.len] = value;
  map->map.len++;
  (void)udata;
  return 0;
}

static void my_free_obj_impl(my_json_obj_s *o) {
  if (!o)
    return;
  switch (o->type) {
  case MY_JSON_STRING:
    fio_str_destroy(&o->str);
    break;
  case MY_JSON_ARRAY:
    for (size_t i = 0; i < o->array.len; i++)
      my_free_obj_impl(o->array.items[i]);
    free(o->array.items);
    break;
  case MY_JSON_MAP:
    for (size_t i = 0; i < o->map.len; i++) {
      my_free_obj_impl(o->map.keys[i]);
      my_free_obj_impl(o->map.values[i]);
    }
    free(o->map.keys);
    free(o->map.values);
    break;
  default:
    break;
  }
  free(o);
}

/* free_unused_object callback: called by the parser for unused objects */
static void my_free_obj(void *udata, void *ctx) {
  my_free_obj_impl((my_json_obj_s *)ctx);
  (void)udata;
}

/* Print JSON object as minified string */
static void my_print_json(my_json_obj_s *o) {
  if (!o)
    return;
  switch (o->type) {
  case MY_JSON_NULL:
    printf("null");
    break;
  case MY_JSON_TRUE:
    printf("true");
    break;
  case MY_JSON_FALSE:
    printf("false");
    break;
  case MY_JSON_NUMBER:
    printf("%lld", (long long)o->i);
    break;
  case MY_JSON_FLOAT:
    printf("%g", o->f);
    break;
  case MY_JSON_STRING:
    printf("\"%s\"", fio_str2ptr(&o->str));
    break;
  case MY_JSON_ARRAY:
    printf("[");
    for (size_t i = 0; i < o->array.len; i++) {
      if (i > 0)
        printf(",");
      my_print_json(o->array.items[i]);
    }
    printf("]");
    break;
  case MY_JSON_MAP:
    printf("{");
    for (size_t i = 0; i < o->map.len; i++) {
      if (i > 0)
        printf(",");
      my_print_json(o->map.keys[i]);
      printf(":");
      my_print_json(o->map.values[i]);
    }
    printf("}");
    break;
  }
}

int main(void) {
  const char *json = "{ \"name\": \"John\", \"age\": 30, \"active\": true, "
                     "\"scores\": [100, 95, 87] }";

  fio_json_parser_callbacks_s callbacks = {
      .on_null = my_on_null,
      .on_true = my_on_true,
      .on_false = my_on_false,
      .on_number = my_on_number,
      .on_float = my_on_float,
      .on_string = my_on_string,
      .on_string_simple = my_on_string,
      .on_array = my_on_array,
      .on_map = my_on_map,
      .array_push = my_array_push,
      .map_push = my_map_push,
      .free_unused_object = my_free_obj,
  };

  fio_json_result_s result = fio_json_parse(&callbacks, NULL, json, strlen(json));

  if (result.err) {
    FIO_LOG_ERROR("JSON parsing failed at position %zu", result.stop_pos);
    return 1;
  }

  printf("Minified: ");
  my_print_json(result.ctx);
  printf("\n");

  my_free_obj_impl(result.ctx);
  return 0;
}
```

### Supported JSON Extensions

The facil.io JSON parser supports several extensions beyond strict JSON:

**Comments:**
- C-style single-line comments: `// comment`
- C-style multi-line comments: `/* comment */`
- Shell/Ruby-style comments: `# comment`

**Numbers:**
- Hexadecimal: `0x1A2B`
- Binary: `0b1010`
- Octal: `0755`
- Infinity: `Infinity`, `inf`, `-Infinity`
- NaN: `NaN`, `nan`
- Leading plus sign: `+42`

**Strings:**
- New-lines within strings (non-strict)

**Collections:**
- Trailing commas: `[1, 2, 3,]` and `{"a": 1,}`

-------------------------------------------------------------------------------
