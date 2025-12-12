## Mustache Template Engine

```c
#define FIO_MUSTACHE
#include "fio-stl.h"
```

The facil.io library includes a Mustache-ish template engine that can be used to render dynamic content from templates.

Mustache is a logic-less template syntax that can be used for HTML, config files, source code, or any text-based format. The templates consist of tags surrounded by mustache-style delimiters (double curly braces `{{` and `}}`).

This implementation supports most standard Mustache features including variables, sections, inverted sections, partials, comments, and delimiter changes. It also includes support for YAML front matter in template files.

**Note**: this module depends on `FIO_STR` and related modules which will be automatically included.

### Configuration Macros

#### `FIO_MUSTACHE_MAX_DEPTH`

```c
#ifndef FIO_MUSTACHE_MAX_DEPTH
#define FIO_MUSTACHE_MAX_DEPTH 128
#endif
```

The maximum depth of a template's context (nesting level for sections and partials).

This prevents stack overflow from deeply nested templates or recursive partial includes.

#### `FIO_MUSTACHE_PRESERVE_PADDING`

```c
#ifndef FIO_MUSTACHE_PRESERVE_PADDING
#define FIO_MUSTACHE_PRESERVE_PADDING 0
#endif
```

When enabled (set to `1`), preserves padding (leading whitespace) for stand-alone variables and partial templates.

This is useful for generating properly indented output, especially when including partial templates that should maintain the indentation level of their inclusion point.

#### `FIO_MUSTACHE_LAMBDA_SUPPORT`

```c
#ifndef FIO_MUSTACHE_LAMBDA_SUPPORT
#define FIO_MUSTACHE_LAMBDA_SUPPORT 0
#endif
```

When enabled (set to `1`), supports raw text for lambda-style template processing.

Lambda support allows sections to receive the raw, unprocessed template text, enabling dynamic template manipulation at runtime.

#### `FIO_MUSTACHE_ISOLATE_PARTIALS`

```c
#ifndef FIO_MUSTACHE_ISOLATE_PARTIALS
#define FIO_MUSTACHE_ISOLATE_PARTIALS 1
#endif
```

When enabled (set to `1`, the default), limits the scope of partial templates to the context of their section.

This prevents partials from accessing variables outside their immediate context, providing better encapsulation.

-------------------------------------------------------------------------------

### Mustache Syntax

The following Mustache syntax elements are supported:

| Syntax | Description |
|--------|-------------|
| `{{variable}}` | HTML-escaped variable output |
| `{{{variable}}}` | Raw (unescaped) variable output |
| `{{&variable}}` | Raw (unescaped) variable output (alternative syntax) |
| `{{#section}}...{{/section}}` | Section block (renders if truthy or iterates over arrays) |
| `{{^section}}...{{/section}}` | Inverted section (renders if falsy or empty) |
| `{{>partial}}` | Include a partial template |
| `{{!comment}}` | Comment (ignored in output) |
| `{{=<% %>=}}` | Change delimiters (example changes to `<%` and `%>`) |

#### Variable Tags

Variables are the most basic tag type. A `{{name}}` tag renders the value of the `name` key in the current context, with HTML escaping applied.

To render unescaped HTML, use triple mustaches `{{{name}}}` or the ampersand syntax `{{&name}}`.

Dot notation is supported for accessing nested values: `{{person.name}}` will look for a `name` key within the `person` object.

The special variable `{{.}}` refers to the current context itself, useful when iterating over arrays of simple values.

#### Section Tags

Sections render blocks of text zero or more times, depending on the value of the key:

- **Falsy values** (false, null, empty arrays): The section is not rendered.
- **Truthy non-array values**: The section is rendered once with the value as the new context.
- **Arrays**: The section is rendered once for each item, with the item as the context.

```mustache
{{#items}}
  <li>{{name}}</li>
{{/items}}
```

#### Inverted Sections

Inverted sections render only when the value is falsy or an empty array:

```mustache
{{^items}}
  <p>No items found.</p>
{{/items}}
```

#### Partials

Partials allow you to include other template files:

```mustache
{{>header}}
<main>Content here</main>
{{>footer}}
```

**Note**: Partial templates are loaded relative to the including template's directory. The engine will search for files with `.mustache`, `.html`, or no extension.

#### Comments

Comments are ignored and produce no output:

```mustache
{{! This is a comment }}
```

#### Delimiter Changes

You can change the tag delimiters if the default `{{` and `}}` conflict with your content:

```mustache
{{=<% %>=}}
<%variable%>
<%={{ }}=%>
```

-------------------------------------------------------------------------------

### Types

#### `fio_mustache_s`

```c
typedef struct fio_mustache_s fio_mustache_s;
```

An opaque type representing a parsed Mustache template.

Templates are reference counted and can be safely shared between threads (for reading). Use `fio_mustache_dup` to increase the reference count and `fio_mustache_free` to decrease it.

#### `fio_mustache_load_args_s`

```c
typedef struct {
  /** The file's content (if pre-loaded) */
  fio_buf_info_s data;
  /** The file's name (even if preloaded, used for partials load paths) */
  fio_buf_info_s filename;
  /** Loads the file's content, returning a `fio_buf_info_s` structure. */
  fio_buf_info_s (*load_file_data)(fio_buf_info_s filename, void *udata);
  /** Frees the file's content from its `fio_buf_info_s` structure. */
  void (*free_file_data)(fio_buf_info_s file_data, void *udata);
  /** Called when YAML front matter data was found. */
  void (*on_yaml_front_matter)(fio_buf_info_s yaml_front_matter, void *udata);
  /** Opaque user data. */
  void *udata;
} fio_mustache_load_args_s;
```

Arguments structure for loading and parsing a Mustache template.

**Members:**

- `data` - The template content as a buffer. If provided, the template is parsed from this data instead of loading from a file.
- `filename` - The template file path. Used for loading the template (if `data` is not provided) and for resolving relative paths when loading partial templates.
- `load_file_data` - Custom callback for loading file content. If not provided, a default implementation using `fio_bstr_readfile` is used.
- `free_file_data` - Custom callback for freeing loaded file content. Must be provided if `load_file_data` is provided.
- `on_yaml_front_matter` - Callback invoked when YAML front matter is detected at the beginning of a template. The front matter content (including delimiters) is passed to this callback.
- `udata` - Opaque user data passed to all callbacks.

#### `fio_mustache_bargs_s`

```c
typedef struct fio_mustache_bargs_s {
  /* callback should write `txt` to output and return updated `udata.` */
  void *(*write_text)(void *udata, fio_buf_info_s txt);
  /* same as `write_text`, but should also HTML escape (sanitize) data. */
  void *(*write_text_escaped)(void *udata, fio_buf_info_s raw);
  /* callback should return a new context pointer with the value of `name`. */
  void *(*get_var)(void *ctx, fio_buf_info_s name);
  /* if context is an Array, should return its length. */
  size_t (*array_length)(void *ctx);
  /* if context is an Array, should return a context pointer @ index. */
  void *(*get_var_index)(void *ctx, size_t index);
  /* should return the String value of context `var` as a `fio_buf_info_s`. */
  fio_buf_info_s (*var2str)(void *var);
  /* should return non-zero if the context pointer refers to a valid value. */
  int (*var_is_truthful)(void *ctx);
  /* callback signals that the `ctx` context pointer is no longer in use. */
  void (*release_var)(void *ctx);
  /* returns non-zero if `ctx` is a lambda and handles section manually. */
  int (*is_lambda)(void **udata, void *ctx, fio_buf_info_s raw_template_section);
  /* the root context for finding named values. */
  void *ctx;
  /* opaque user data (settable as well as readable), the final return value. */
  void *udata;
} fio_mustache_bargs_s;
```

Arguments structure for building (rendering) a Mustache template.

**Members:**

- `write_text` - Callback to write raw text to the output. Should return the updated `udata` value.
- `write_text_escaped` - Callback to write HTML-escaped text to the output. Used for `{{variable}}` tags. Should return the updated `udata` value.
- `get_var` - Callback to retrieve a variable from the context by name. Returns a new context pointer representing the value, or `NULL` if not found.
- `array_length` - Callback to get the length of an array context. Returns `0` for non-array contexts.
- `get_var_index` - Callback to get an element from an array context by index.
- `var2str` - Callback to convert a context/variable to its string representation.
- `var_is_truthful` - Callback to determine if a context represents a truthy value. Returns non-zero for truthy values.
- `release_var` - Callback to release a context pointer when it's no longer needed. Used for memory management.
- `is_lambda` - Callback for lambda support. If the context is a lambda, this callback should handle the section rendering and return non-zero. Only called if `FIO_MUSTACHE_LAMBDA_SUPPORT` is enabled.
- `ctx` - The root context object containing the data for template rendering.
- `udata` - Opaque user data. This value is passed to callbacks and returned as the final result of `fio_mustache_build`.

**Note**: If `write_text` and `write_text_escaped` are both `NULL`, default implementations are used that build a `fio_bstr` string, which is returned via `udata`.

-------------------------------------------------------------------------------

### Loading and Parsing API

#### `fio_mustache_load`

```c
fio_mustache_s *fio_mustache_load(fio_mustache_load_args_s args);
/* Named arguments using macro. */
#define fio_mustache_load(...) fio_mustache_load((fio_mustache_load_args_s){__VA_ARGS__})
```

Loads and parses a Mustache template, returning a template object.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
fio_mustache_s *template = fio_mustache_load(.filename = FIO_BUF_INFO1("template.mustache"));
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `data` | `fio_buf_info_s` | Template content (if pre-loaded) |
| `filename` | `fio_buf_info_s` | Template file path |
| `load_file_data` | `fio_buf_info_s (*)(fio_buf_info_s, void *)` | Custom file loader callback |
| `free_file_data` | `void (*)(fio_buf_info_s, void *)` | Custom file data cleanup callback |
| `on_yaml_front_matter` | `void (*)(fio_buf_info_s, void *)` | YAML front matter callback |
| `udata` | `void *` | User data passed to callbacks |

**Returns:** a pointer to the parsed template object, or `NULL` on error.

**Note**: Either `data` or `filename` must be provided. If both are provided, `data` is used as the template content and `filename` is used only for resolving partial template paths.

#### `fio_mustache_free`

```c
void fio_mustache_free(fio_mustache_s *m);
```

Frees the Mustache template object (or reduces its reference count).

If the reference count reaches zero, the template and all its resources are freed.

#### `fio_mustache_dup`

```c
fio_mustache_s *fio_mustache_dup(fio_mustache_s *m);
```

Increases the Mustache template's reference count and returns the same pointer.

Use this when you need to share a template between multiple owners.

-------------------------------------------------------------------------------

### Building / Rendering API

#### `fio_mustache_build`

```c
void *fio_mustache_build(fio_mustache_s *m, fio_mustache_bargs_s args);
/* Named arguments using macro. */
#define fio_mustache_build(m, ...) fio_mustache_build((m), ((fio_mustache_bargs_s){__VA_ARGS__}))
```

Builds (renders) the template with the provided context, returning the final value of `udata`.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
char *result = fio_mustache_build(template,
                                  .ctx = my_data_context,
                                  .udata = NULL);
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `write_text` | `void *(*)(void *, fio_buf_info_s)` | Raw text output callback |
| `write_text_escaped` | `void *(*)(void *, fio_buf_info_s)` | HTML-escaped output callback |
| `get_var` | `void *(*)(void *, fio_buf_info_s)` | Variable lookup callback |
| `array_length` | `size_t (*)(void *)` | Array length callback |
| `get_var_index` | `void *(*)(void *, size_t)` | Array element access callback |
| `var2str` | `fio_buf_info_s (*)(void *)` | Variable to string conversion callback |
| `var_is_truthful` | `int (*)(void *)` | Truthiness check callback |
| `release_var` | `void (*)(void *)` | Variable release callback |
| `is_lambda` | `int (*)(void **, void *, fio_buf_info_s)` | Lambda support callback |
| `ctx` | `void *` | Root data context |
| `udata` | `void *` | User data (becomes return value) |

**Returns:** the final value of `udata` (or `NULL` if the template is `NULL`).

**Note**: If no writer callbacks are provided, the function uses default implementations that build a `fio_bstr` string. In this case, the returned `udata` is the resulting string which must be freed with `fio_bstr_free`.

-------------------------------------------------------------------------------

### YAML Front Matter

The Mustache engine supports YAML front matter at the beginning of template files. Front matter is delimited by `---` on its own line:

```yaml
---
title: My Page
author: John Doe
---
<html>
<head><title>{{title}}</title></head>
...
```

When front matter is detected, the `on_yaml_front_matter` callback is invoked with the complete front matter content (including the `---` delimiters). This allows you to parse metadata from templates for use in your application.

**Note**: The front matter is stripped from the template content before parsing. The Mustache engine does not parse the YAML itself - you must provide your own YAML parser in the callback if needed.

-------------------------------------------------------------------------------

### Partial Template Loading

When loading partial templates (via `{{>partial}}`), the engine searches for files in the following order:

1. Relative to the including template's directory
2. Relative to parent template directories (recursively)
3. In the current working directory

For each location, the engine tries the following file extensions:

1. `.mustache`
2. `.html`
3. No extension (exact filename)

**Example**: If `templates/page.mustache` includes `{{>header}}`, the engine will search for:
- `templates/header.mustache`
- `templates/header.html`
- `templates/header`
- `header.mustache`
- `header.html`
- `header`

-------------------------------------------------------------------------------

### Example - Basic Usage with FIOBJ

The following example demonstrates using the Mustache engine with the FIOBJ type system:

```c
#define FIO_FIOBJ
#define FIO_MUSTACHE
#include "fio-stl.h"

/* FIOBJ integration callbacks */
static void *fiobj_mustache_get_var(void *ctx, fio_buf_info_s name) {
  if (!ctx || !FIOBJ_TYPE_IS((FIOBJ)ctx, FIOBJ_T_HASH))
    return NULL;
  FIOBJ key = fiobj_str_new_cstr(name.buf, name.len);
  FIOBJ val = fiobj_hash_get((FIOBJ)ctx, key);
  fiobj_free(key);
  return (void *)fiobj_dup(val);
}

static size_t fiobj_mustache_array_length(void *ctx) {
  if (!ctx || !FIOBJ_TYPE_IS((FIOBJ)ctx, FIOBJ_T_ARRAY))
    return 0;
  return fiobj_array_count((FIOBJ)ctx);
}

static void *fiobj_mustache_get_var_index(void *ctx, size_t index) {
  if (!ctx || !FIOBJ_TYPE_IS((FIOBJ)ctx, FIOBJ_T_ARRAY))
    return NULL;
  return (void *)fiobj_dup(fiobj_array_get((FIOBJ)ctx, index));
}

static fio_buf_info_s fiobj_mustache_var2str(void *var) {
  return FIO_STR2BUF_INFO(fiobj2cstr((FIOBJ)var));
}

static int fiobj_mustache_var_is_truthful(void *ctx) {
  if (!ctx)
    return 0;
  FIOBJ o = (FIOBJ)ctx;
  switch (FIOBJ_TYPE(o)) {
  case FIOBJ_T_NULL:
  case FIOBJ_T_FALSE:
    return 0;
  case FIOBJ_T_ARRAY:
    return fiobj_array_count(o) > 0;
  default:
    return 1;
  }
}

static void fiobj_mustache_release_var(void *ctx) {
  fiobj_free((FIOBJ)ctx);
}

void example(void) {
  /* Load template */
  fio_mustache_s *m = fio_mustache_load(
      .filename = FIO_BUF_INFO1("template.mustache"));
  if (!m) {
    fprintf(stderr, "Failed to load template\n");
    return;
  }

  /* Create data context */
  FIOBJ data = fiobj_hash_new();
  fiobj_hash_set2(data, "name", 4, fiobj_str_new_cstr("World", 5));
  
  FIOBJ items = fiobj_array_new();
  fiobj_array_push(items, fiobj_str_new_cstr("Apple", 5));
  fiobj_array_push(items, fiobj_str_new_cstr("Banana", 6));
  fiobj_array_push(items, fiobj_str_new_cstr("Cherry", 6));
  fiobj_hash_set2(data, "items", 5, items);

  /* Render template */
  char *result = fio_mustache_build(m,
      .ctx = (void *)data,
      .get_var = fiobj_mustache_get_var,
      .array_length = fiobj_mustache_array_length,
      .get_var_index = fiobj_mustache_get_var_index,
      .var2str = fiobj_mustache_var2str,
      .var_is_truthful = fiobj_mustache_var_is_truthful,
      .release_var = fiobj_mustache_release_var);

  /* Output result */
  printf("%s\n", result);

  /* Cleanup */
  fio_bstr_free(result);
  fiobj_free(data);
  fio_mustache_free(m);
}
```

With a template file `template.mustache`:

```mustache
Hello, {{name}}!

Items:
{{#items}}
  - {{.}}
{{/items}}
{{^items}}
  No items available.
{{/items}}
```

Output:

```
Hello, World!

Items:
  - Apple
  - Banana
  - Cherry
```

-------------------------------------------------------------------------------

### Example - Simple String Rendering

For simple use cases where you just want to render a template to a string without a complex data context:

```c
#define FIO_MUSTACHE
#include "fio-stl.h"

void simple_example(void) {
  /* Load template from string */
  fio_mustache_s *m = fio_mustache_load(
      .data = FIO_BUF_INFO1("Hello, {{name}}!"));
  
  if (!m) {
    fprintf(stderr, "Failed to parse template\n");
    return;
  }

  /* Render with default (empty) context - variables won't be replaced */
  char *result = fio_mustache_build(m, .udata = NULL);
  
  printf("%s\n", result);  /* Output: Hello, ! */
  
  fio_bstr_free(result);
  fio_mustache_free(m);
}
```

**Note**: Without providing context callbacks, variables will render as empty strings. For meaningful output, you need to provide at least `ctx`, `get_var`, `var2str`, and `var_is_truthful` callbacks.

-------------------------------------------------------------------------------
