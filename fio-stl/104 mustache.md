# Mustache-ish Template Engine

```c
#define FIO_MUSTACHE
#include "fio-stl.h"
```

A Mustache-ish template parser and renderer with variables, sections, inverted sections, partials, delimiter changes, comments, and optional YAML front matter callbacks. Logic-less-ish, because C callbacks still get a vote. Implemented in [`./104 mustache.h`](./104%20mustache.h).

Defining `FIO_MUSTACHE` pulls in string / `fio_bstr` helpers as needed.

### Configuration Macros

#### `FIO_MUSTACHE_MAX_DEPTH`

```c
#ifndef FIO_MUSTACHE_MAX_DEPTH
#define FIO_MUSTACHE_MAX_DEPTH 128
#endif
```

Maximum parser / builder nesting depth for sections and partials.

#### `FIO_MUSTACHE_PRESERVE_PADDING`

```c
#ifndef FIO_MUSTACHE_PRESERVE_PADDING
#define FIO_MUSTACHE_PRESERVE_PADDING 0
#endif
```

When enabled, preserves padding for stand-alone variables and partial templates.

#### `FIO_MUSTACHE_LAMBDA_SUPPORT`

```c
#ifndef FIO_MUSTACHE_LAMBDA_SUPPORT
#define FIO_MUSTACHE_LAMBDA_SUPPORT 0
#endif
```

When enabled, stores raw section text for lambda-style section handling through `is_lambda`.

#### `FIO_MUSTACHE_ISOLATE_PARTIALS`

```c
#ifndef FIO_MUSTACHE_ISOLATE_PARTIALS
#define FIO_MUSTACHE_ISOLATE_PARTIALS 1
#endif
```

When enabled, limits partial lookup scope to the context of the partial's section.

#### `FIO_MUSTACHE_SECURE_PATH`

```c
#ifndef FIO_MUSTACHE_SECURE_PATH
#define FIO_MUSTACHE_SECURE_PATH 1
#endif
```

When enabled, skips partial names that attempt `../` path traversal.

### Syntax Quick Map

| Syntax | Meaning |
| --- | --- |
| `{{name}}` | escaped variable |
| `{{{name}}}` | raw variable |
| `{{&name}}` | raw variable |
| `{{#items}}...{{/items}}` | truthy section / array iteration |
| `{{^items}}...{{/items}}` | inverted section |
| `{{>partial}}` | partial template include |
| `{{! comment}}` | comment |
| `{{=<% %>=}}` | delimiter change |
| `{{.}}` | current context |

Dot lookup such as `person.name` is supported by repeatedly calling `get_var` for path segments.

### Types

#### `fio_mustache_s`

```c
typedef struct fio_mustache_s fio_mustache_s;
```

Opaque parsed template object. Internally it is stored as a `fio_bstr` instruction stream.

**Ownership:** `fio_mustache_load` returns an owned template. Free it with `fio_mustache_free`. `fio_mustache_dup` creates a copy; it is not a shared reference counter in this header.

#### `fio_mustache_bargs_s`

```c
typedef struct fio_mustache_bargs_s fio_mustache_bargs_s;
```

Forward declaration for build arguments. The full struct is listed below.

#### `fio_mustache_load_args_s`

```c
typedef struct {
  fio_buf_info_s data;
  fio_buf_info_s filename;
  fio_buf_info_s (*load_file_data)(fio_buf_info_s filename, void *udata);
  void (*free_file_data)(fio_buf_info_s file_data, void *udata);
  void (*on_yaml_front_matter)(fio_buf_info_s yaml_front_matter, void *udata);
  void *udata;
} fio_mustache_load_args_s;
```

Load / parse settings.

**Members:**
- `data` - preloaded template bytes
- `filename` - file name, also used as base path for partials
- `load_file_data` - callback that loads file contents
- `free_file_data` - callback that frees loaded file contents
- `on_yaml_front_matter` - called when front matter is found
- `udata` - user data for load callbacks

If both `load_file_data` and `free_file_data` are missing, defaults use `fio_bstr_readfile` and `fio_bstr_free`. If one is custom, provide the matching cleanup. Either `filename` or `data` is required.

#### `fio_mustache_bargs_s`

```c
struct fio_mustache_bargs_s {
  void *(*write_text)(void *udata, fio_buf_info_s txt);
  void *(*write_text_escaped)(void *udata, fio_buf_info_s raw);
  void *(*get_var)(void *ctx, fio_buf_info_s name);
  size_t (*array_length)(void *ctx);
  void *(*get_var_index)(void *ctx, size_t index);
  fio_buf_info_s (*var2str)(void *var);
  int (*var_is_truthful)(void *ctx);
  void (*release_var)(void *ctx);
  int (*is_lambda)(void **udata,
                   void *ctx,
                   fio_buf_info_s raw_template_section);
  void *ctx;
  void *udata;
};
```

Build / render callbacks.

**Members:**
- `write_text` - writes raw template text
- `write_text_escaped` - writes escaped variable text
- `get_var` - returns a value for `name` from `ctx`
- `array_length` - returns array length, or `0` for non-array values
- `get_var_index` - returns an item context by index
- `var2str` - returns a string view for a value
- `var_is_truthful` - returns non-zero for truthy values
- `release_var` - releases values returned by callbacks
- `is_lambda` - handles lambda sections when lambda support is enabled
- `ctx` - root render context
- `udata` - output / user pointer; the final value is returned by `fio_mustache_build`

If both writer callbacks are missing, defaults append to a `fio_bstr` in `udata`. The returned pointer should be freed with `fio_bstr_free`.

### Loading

#### `fio_mustache_load`

```c
SFUNC fio_mustache_s *fio_mustache_load(fio_mustache_load_args_s settings);
#define fio_mustache_load(...) \
  fio_mustache_load((fio_mustache_load_args_s){__VA_ARGS__})
```

Loads and parses a template from `settings.data` or `settings.filename`.

If `filename` is provided and `data` is empty, the loader callback is used. If both are provided, `data` is parsed and `filename` is kept for partial path resolution.

**Returns:** a parsed template object, or `NULL` on error.

#### `fio_mustache_free`

```c
SFUNC void fio_mustache_free(fio_mustache_s *m);
```

Frees a parsed template. Accepts `NULL`.

#### `fio_mustache_dup`

```c
SFUNC fio_mustache_s *fio_mustache_dup(fio_mustache_s *m);
```

Copies a parsed template using `fio_bstr_copy`.

**Returns:** the copied template, or `NULL` if `m == NULL`.

### Rendering

#### `fio_mustache_build`

```c
SFUNC void *fio_mustache_build(fio_mustache_s *m, fio_mustache_bargs_s);
#define fio_mustache_build(m, ...) \
  fio_mustache_build((m), ((fio_mustache_bargs_s){__VA_ARGS__}))
```

Renders `m` with the supplied context and callbacks.

**Returns:** the final `udata` value. If `m == NULL`, returns `args.udata` unchanged.

During rendering:
- escaped variables call `write_text_escaped`
- raw variables call `write_text`
- sections render once for truthy non-arrays, or once per array item
- inverted sections render when `var_is_truthful` returns zero
- values returned from lookup / array callbacks are released with `release_var`

### Partials and Files

Partials are loaded by name through `load_file_data`. The implementation tries each name with these suffixes:

1. `.mustache`
2. `.html`
3. no suffix

Relative partials are searched against the including template's path chain, then as provided. With `FIO_MUSTACHE_SECURE_PATH`, `../` partials are skipped.

### YAML Front Matter

If a template begins with `---` followed by a line break, the parser scans until a closing `---` line and calls:

```c
void (*on_yaml_front_matter)(fio_buf_info_s yaml_front_matter, void *udata);
```

The front matter bytes are not rendered. The engine does not parse YAML for you. It has enough hobbies.

### Thread-Safety

Parsed templates are immutable during `fio_mustache_build`, so separate builds can use the same template as long as your callbacks and contexts are safe. Loading and freeing are caller-owned operations.

### Example

```c
#define FIO_MUSTACHE
#include "fio-stl.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  const char *name;
} data_s;

static void *get_var(void *ctx, fio_buf_info_s name) {
  data_s *d = (data_s *)ctx;
  if (name.len == 4 && !memcmp(name.buf, "name", 4))
    return (void *)d->name;
  return NULL;
}

static fio_buf_info_s var2str(void *var) {
  char *s = (char *)var;
  return FIO_BUF_INFO2(s, strlen(s));
}

int main(void) {
  char template_text[] = "Hello, {{name}}!\n";
  data_s data = {.name = "World"};

  fio_mustache_s *m = fio_mustache_load(
      .data = FIO_BUF_INFO1(template_text));
  if (!m)
    return 1;

  char *out = fio_mustache_build(m,
      .ctx = &data,
      .get_var = get_var,
      .var2str = var2str);

  if (out) {
    fwrite(out, 1, fio_bstr_len(out), stdout);
    fio_bstr_free(out);
  }

  fio_mustache_free(m);
  return 0;
}
```

---
