## Markdown to HTML bstr Renderer

```c
#define FIO_MD2HTML
#include "fio-stl.h"
```

`FIO_MD2HTML` provides a small Markdown / GFM to HTML renderer built on the
`FIO_MARKDOWN` callback parser. The renderer accumulates output in `fio_bstr`.

Defining `FIO_MD2HTML` automatically enables its dependencies:

- `FIO_MARKDOWN`
- `FIO_STR` for `fio_bstr`

### API

```c
char *fio_md2html(char *bstr_target, fio_buf_info_s source);
```

Parses a complete Markdown buffer and appends rendered HTML to `bstr_target`, returning the resulting owned `fio_bstr`. Pass `NULL` to create a new buffer. Release the returned value with `fio_bstr_free`.

Returns `NULL` if parsing aborts or output allocation fails. Empty input renders
to a non-NULL empty `fio_bstr` when allocation succeeds.

### Rendering Notes

- Normal text, code, and attribute values are HTML-escaped.
- Markdown raw HTML blocks and inline HTML are preserved as raw HTML.
- Fenced code info strings render as `class="language-..."` using the first info
  word.
- GFM tables render with `<thead>` for the first row and `<tbody>` for remaining
  rows.
- GFM task-list items render disabled checkbox inputs.
- GFM footnotes render with GitHub-compatible `footnote-ref`, `footnote-backref`, `fn-...`, and `fnref-...` markup.
- Reference-link URLs are percent-encoded when the destination has no existing `%` escape.

### Example

```c
char md[] = "# Demo\n\nHello **world**.\n";
char *html = fio_md2html(NULL, FIO_BUF_INFO1(md));
if (html) {
  fwrite(html, 1, fio_bstr_len(html), stdout);
  fio_bstr_free(html);
}
```
