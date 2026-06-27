# Markdown to HTML bstr Renderer

```c
#define FIO_MD2HTML
#include "fio-stl.h"
```

A complete-document Markdown / GFM renderer that turns parser events into escaped HTML stored in an owned `fio_bstr`. Markdown goes in, tags come out. Implemented in [`./103 md2html.h`](./103%20md2html.h).

Defining `FIO_MD2HTML` automatically includes:
- `FIO_STR` for `fio_bstr`
- `FIO_GFM` for Markdown / GFM parsing

### Error Codes

#### `FIO_MD2HTML_ERR_ALLOC`

```c
#ifndef FIO_MD2HTML_ERR_ALLOC
#define FIO_MD2HTML_ERR_ALLOC 1
#endif
```

Internal renderer error used when output allocation fails. Override before inclusion only if you need a different non-zero value.

### Rendering

#### `fio_md2html`

```c
SFUNC char *fio_md2html(char *bstr_target, fio_buf_info_s source);
```

Renders a complete Markdown / GFM document into a `fio_bstr`.

**Parameters:**
- `bstr_target` - existing `fio_bstr` to append to, or `NULL` to allocate a new one
- `source` - Markdown bytes to parse

**Returns:** the output `fio_bstr`, or `NULL` on parser / allocation failure.

**Ownership:** the returned buffer is owned by the caller and should be released with `fio_bstr_free`. If `bstr_target == NULL` and rendering fails, the newly allocated output is freed. If `bstr_target` was provided, ownership stays with the caller.

### Rendering Behavior

From the implementation:
- normal text, code, and attribute values are escaped
- raw Markdown HTML blocks / inline HTML are preserved, with GFM tagfilter applied
- blocked tag names include `iframe`, `noembed`, `noframes`, `plaintext`, `style`, `textarea`, `title`, and `xmp`; HTML blocks only filter `textarea` and `xmp`
- fenced code info strings use the first word as `class="language-..."`
- GFM tables render `<table><thead>...` and open `<tbody>` for body rows
- table cell alignment uses `align="left"`, `align="right"`, or `align="center"`
- GFM task list items render disabled checkbox inputs
- autolinks add `mailto:` for bare email addresses and `http://` for `www.` links
- image alt text is escaped and strips nested link / emphasis markup

### Thread-Safety

The renderer uses caller-provided input and a local renderer state. It is thread-safe as long as separate calls do not mutate the same `bstr_target` at the same time.

### Example

```c
#define FIO_MD2HTML
#include "fio-stl.h"
#include <stdio.h>

int main(void) {
  char md[] = "# Demo\n\nHello **world**.\n";
  char *html = fio_md2html(NULL, FIO_BUF_INFO1(md));
  if (!html)
    return 1;

  fwrite(html, 1, fio_bstr_len(html), stdout);
  fio_bstr_free(html);
  return 0;
}
```

---
