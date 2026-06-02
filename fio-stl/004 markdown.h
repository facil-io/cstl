/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MARKDOWN           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                             Markdown / GFM Parser


Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_MARKDOWN) && !defined(H___FIO_MARKDOWN___H)
#define H___FIO_MARKDOWN___H

#ifndef FIO_MARKDOWN_MAX_DEPTH
/** Maximum inline recursion / nested container depth. */
#define FIO_MARKDOWN_MAX_DEPTH 64
#endif

#ifndef FIO_MARKDOWN_MAX_TABLE_COLUMNS
/** Maximum GFM table column alignments cached on the stack. */
#define FIO_MARKDOWN_MAX_TABLE_COLUMNS 64
#endif
FIO_ASSERT_STATIC(FIO_MARKDOWN_MAX_TABLE_COLUMNS > 0,
                  "FIO_MARKDOWN_MAX_TABLE_COLUMNS too small");
FIO_ASSERT_STATIC(FIO_MARKDOWN_MAX_TABLE_COLUMNS < 65536,
                  "FIO_MARKDOWN_MAX_TABLE_COLUMNS too large");

#ifndef FIO_MARKDOWN_MAX_REFERENCES
/** Maximum reference definitions indexed during the initial zero-heap scan. */
#define FIO_MARKDOWN_MAX_REFERENCES 128
#endif
FIO_ASSERT_STATIC(FIO_MARKDOWN_MAX_REFERENCES > 0,
                  "FIO_MARKDOWN_MAX_REFERENCES too small");
FIO_ASSERT_STATIC(FIO_MARKDOWN_MAX_REFERENCES < 65536,
                  "FIO_MARKDOWN_MAX_REFERENCES too large");

#define FIO___MD_TAB_WIDTH         4U
#define FIO___MD_MAX_MARKER_INDENT 3U
#define FIO___MD_MIN_FENCE_LEN     3
#define FIO___MD_MAX_ATX_HEADING   6
#define FIO___MD_MAX_ENTITY_LEN    32

/** Markdown parser internal error code: generic parser error. */
#define FIO_MD_ERR_GENERIC -1
/** Markdown parser internal error code: nesting exceeded configured limits. */
#define FIO_MD_ERR_DEPTH -2
/** Markdown parser internal error code: invalid parser input. */
#define FIO_MD_ERR_INPUT -3

/** Block flag: list is tight. Valid on list leave events. */
#define FIO_MD_BLOCK_F_TIGHT ((uint32_t)1U << 0)
/** Block flag: list item has a GFM task-list marker. */
#define FIO_MD_BLOCK_F_TASK ((uint32_t)1U << 1)
/** Block flag: GFM task-list marker is checked. Requires FIO_MD_BLOCK_F_TASK.
 */
#define FIO_MD_BLOCK_F_TASK_CHECKED ((uint32_t)1U << 2)

/** Markdown block event types. */
typedef enum {
  FIO_MD_BLOCK_DOCUMENT = 0,
  FIO_MD_BLOCK_PARAGRAPH,
  FIO_MD_BLOCK_HEADING,
  FIO_MD_BLOCK_THEMATIC_BREAK,
  FIO_MD_BLOCK_BLOCK_QUOTE,
  FIO_MD_BLOCK_LIST_UNORDERED,
  FIO_MD_BLOCK_LIST_ORDERED,
  FIO_MD_BLOCK_LIST_ITEM,
  FIO_MD_BLOCK_CODE_INDENTED,
  FIO_MD_BLOCK_CODE_FENCED,
  FIO_MD_BLOCK_HTML,
  FIO_MD_BLOCK_TABLE,
  FIO_MD_BLOCK_TABLE_ROW,
  FIO_MD_BLOCK_TABLE_CELL,
} fio_md_block_type_e;

/** Markdown inline event types. */
typedef enum {
  FIO_MD_INLINE_TEXT = 0,
  FIO_MD_INLINE_SOFT_BREAK,
  FIO_MD_INLINE_HARD_BREAK,
  FIO_MD_INLINE_CODE_SPAN,
  FIO_MD_INLINE_EMPHASIS,
  FIO_MD_INLINE_STRONG,
  FIO_MD_INLINE_STRIKETHROUGH,
  FIO_MD_INLINE_LINK,
  FIO_MD_INLINE_IMAGE,
  FIO_MD_INLINE_AUTOLINK,
  FIO_MD_INLINE_HTML,
  FIO_MD_INLINE_ESCAPE,
  FIO_MD_INLINE_ENTITY,
  FIO_MD_INLINE_FOOTNOTE_REF,
} fio_md_inline_type_e;

/** Container event phase for inline events that wrap child inline events. */
typedef enum {
  FIO_MD_EVENT_LEAF = 0,
  FIO_MD_EVENT_ENTER = 1,
  FIO_MD_EVENT_LEAVE = 2,
} fio_md_event_e;

/** GFM table cell alignment. */
typedef enum {
  FIO_MD_ALIGN_NONE = 0,
  FIO_MD_ALIGN_LEFT,
  FIO_MD_ALIGN_RIGHT,
  FIO_MD_ALIGN_CENTER,
} fio_md_align_e;

/** A zero-copy Markdown block event. */
typedef struct {
  /** Full source slice for the block, including Markdown markers. */
  fio_buf_info_s source;
  /** Block content slice when contiguous and useful (code/html/fenced body). */
  fio_buf_info_s content;
  /** Marker slice (`#`, fence, list marker, table delimiter, etc.). */
  fio_buf_info_s marker;
  /** Fenced-code info string, when any. */
  fio_buf_info_s info;
  /** Ordered-list start number, if any. */
  uint32_t list_start;
  /** GFM table column count. */
  uint16_t columns;
  /** GFM table cell column index. */
  uint16_t column;
  /** Block type (`FIO_MD_BLOCK_*`). */
  uint8_t type;
  /** Heading level (1..6), valid only for headings. */
  uint8_t heading_level;
  /** `FIO_MD_BLOCK_F_*` bit flags. */
  uint8_t flags;
  /** GFM table cell alignment (`FIO_MD_ALIGN_*`). */
  uint8_t align;
  /** Virtual leading spaces produced by tab expansion while stripping
   * containers. */
  uint8_t padding;
} fio_md_block_s;

/** A zero-copy Markdown inline event. */
typedef struct {
  /** Full source slice for this inline construct. */
  fio_buf_info_s source;
  /** Text / label / code content slice. */
  fio_buf_info_s text;
  /** Link/image/autolink destination slice. */
  fio_buf_info_s destination;
  /** Link/image optional title slice. */
  fio_buf_info_s title;
  /** Reference label slice for reference-style links. */
  fio_buf_info_s reference;
  /** Inline type (`FIO_MD_INLINE_*`). */
  uint8_t type;
  /** Leaf / enter / leave event phase (`FIO_MD_EVENT_*`). */
  uint8_t event;
} fio_md_inline_s;

/** Markdown parser callbacks. All callbacks are optional. */
typedef struct {
  /** Called before parsing starts. Non-zero return aborts parsing. */
  int (*on_document_start)(void *udata, fio_buf_info_s source);
  /** Called after parsing completes, unless parsing aborted. */
  int (*on_document_end)(void *udata, fio_buf_info_s source);
  /** Called when a block opens. Non-zero return aborts parsing. */
  int (*on_block_enter)(void *udata, const fio_md_block_s *block);
  /** Called when a block closes. Non-zero return aborts parsing. */
  int (*on_block_leave)(void *udata, const fio_md_block_s *block);
  /** Called for inline events. Non-zero return aborts parsing. */
  int (*on_inline)(void *udata, const fio_md_inline_s *inline_event);
  /** Called when parsing aborts. Negative `err` values are parser errors. */
  void (*on_error)(void *udata, int err, size_t consumed);
} fio_md_callbacks_s;

/**
 * Parses a full Markdown / GFM source buffer and emits callback events.
 *
 * The parser is non-streaming: `source` must contain the complete document.
 * Events are zero-copy slices into `source` wherever Markdown structure
 * permits. The parser builds no AST, performs no rendering, and performs no
 * heap allocation.
 *
 * A non-zero callback return aborts parsing and is forwarded as `err` to the
 * optional `on_error` callback. Parser-generated errors are negative; `-1` is
 * the generic parser error code. The return value is the number of source bytes
 * consumed before stopping.
 */
SFUNC size_t fio_md_parse(const fio_md_callbacks_s *callbacks,
                          void *udata,
                          fio_buf_info_s source);

/* *****************************************************************************
Markdown Parser - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

typedef struct {
  fio_buf_info_s label;
  fio_buf_info_s destination;
  fio_buf_info_s title;
} fio___md_ref_s;

typedef struct {
  const fio_md_callbacks_s *cb;
  void *udata;
  char *start;
  char *end;
  size_t consumed;
  uint32_t depth;
  int err;
  fio___md_ref_s ref_cache;
  fio___md_ref_s refs[FIO_MARKDOWN_MAX_REFERENCES];
  uint32_t ref_cache_valid;
  uint16_t ref_count;
  uint8_t ref_overflow;
} fio___md_s;

FIO_IFUNC fio_buf_info_s fio___md_buf(char *start, char *end) {
  fio_buf_info_s r = FIO_BUF_INFO0;
  if (end > start) {
    r.buf = start;
    r.len = (size_t)(end - start);
  }
  return r;
}

FIO_IFUNC char *fio___md_line_end(char *p, char *end) {
  while (p < end && *p != '\n' && *p != '\r')
    ++p;
  return p;
}

FIO_IFUNC char *fio___md_line_next(char *line_end, char *end) {
  if (line_end < end && *line_end == '\r')
    ++line_end;
  if (line_end < end && *line_end == '\n')
    ++line_end;
  return line_end;
}

FIO_IFUNC char *fio___md_ltrim(char *p, char *end) {
  while (p < end && (*p == ' ' || *p == '\t'))
    ++p;
  return p;
}

FIO_IFUNC char *fio___md_rtrim(char *p, char *end) {
  while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
    --end;
  return end;
}

FIO_IFUNC uint32_t fio___md_indent(char *p, char *end) {
  uint32_t r = 0;
  while (p < end) {
    if (*p == ' ') {
      ++r;
      ++p;
    } else if (*p == '\t') {
      r += FIO___MD_TAB_WIDTH - (r & (FIO___MD_TAB_WIDTH - 1U));
      ++p;
    } else {
      break;
    }
  }
  return r;
}

FIO_IFUNC char *fio___md_skip_indent2(char *p,
                                      char *end,
                                      uint32_t columns,
                                      uint8_t *padding) {
  uint32_t r = 0;
  if (padding)
    *padding = 0;
  while (p < end && r < columns) {
    if (*p == ' ') {
      ++r;
      ++p;
    } else if (*p == '\t') {
      r += FIO___MD_TAB_WIDTH - (r & (FIO___MD_TAB_WIDTH - 1U));
      ++p;
    } else {
      break;
    }
  }
  if (r < columns)
    return NULL;
  if (padding && r > columns)
    *padding = (uint8_t)(r - columns);
  return p;
}

FIO_IFUNC char *fio___md_skip_indent(char *p, char *end, uint32_t columns) {
  return fio___md_skip_indent2(p, end, columns, NULL);
}

FIO_IFUNC uint32_t fio___md_column_to(char *p, char *target) {
  uint32_t r = 0;
  while (p < target) {
    if (*p == '\t')
      r += FIO___MD_TAB_WIDTH - (r & (FIO___MD_TAB_WIDTH - 1U));
    else
      ++r;
    ++p;
  }
  return r;
}

FIO_IFUNC char fio___md_ascii_lower(char c) {
  return (char)((c >= 'A' && c <= 'Z') ? (c | 32) : c);
}

FIO_IFUNC int fio___md_is_blank(char *p, char *end) {
  p = fio___md_ltrim(p, end);
  return p == end;
}

FIO_IFUNC int fio___md_span_has_nonspace(char *p, char *end) {
  while (p < end) {
    if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
      return 1;
    ++p;
  }
  return 0;
}

FIO_IFUNC int fio___md_punct(char c) {
  switch (c) {
  case '!':
  case '"':
  case '#':
  case '$':
  case '%':
  case '&':
  case '\'':
  case '(':
  case ')':
  case '*':
  case '+':
  case ',':
  case '-':
  case '.':
  case '/':
  case ':':
  case ';':
  case '<':
  case '=':
  case '>':
  case '?':
  case '@':
  case '[':
  case '\\':
  case ']':
  case '^':
  case '_':
  case '`':
  case '{':
  case '|':
  case '}':
  case '~': return 1;
  default: return 0;
  }
}

FIO_SFUNC int fio___md_abort(fio___md_s *s, int err, char *pos) {
  if (!err)
    err = FIO_MD_ERR_GENERIC;
  s->err = err;
  if (!s->start) {
    s->consumed = 0;
  } else {
    if (pos < s->start)
      pos = s->start;
    if (pos > s->end)
      pos = s->end;
    s->consumed = (size_t)(pos - s->start);
  }
  if (s->cb && s->cb->on_error)
    s->cb->on_error(s->udata, err, s->consumed);
  return -1;
}

FIO_IFUNC int fio___md_doc_start(fio___md_s *s, fio_buf_info_s source) {
  int r;
  if (!s->cb || !s->cb->on_document_start)
    return 0;
  r = s->cb->on_document_start(s->udata, source);
  return r ? fio___md_abort(s, r, source.buf) : 0;
}

FIO_IFUNC int fio___md_doc_end(fio___md_s *s, fio_buf_info_s source) {
  int r;
  if (!s->cb || !s->cb->on_document_end)
    return 0;
  r = s->cb->on_document_end(s->udata, source);
  return r ? fio___md_abort(s, r, s->end) : 0;
}

FIO_IFUNC int fio___md_block_enter(fio___md_s *s, const fio_md_block_s *b) {
  int r;
  if (!s->cb || !s->cb->on_block_enter)
    return 0;
  r = s->cb->on_block_enter(s->udata, b);
  return r ? fio___md_abort(s, r, b->source.buf) : 0;
}

FIO_IFUNC int fio___md_block_leave(fio___md_s *s, const fio_md_block_s *b) {
  int r;
  if (!s->cb || !s->cb->on_block_leave)
    return 0;
  r = s->cb->on_block_leave(s->udata, b);
  return r ? fio___md_abort(s, r, b->source.buf) : 0;
}

FIO_IFUNC int fio___md_inline(fio___md_s *s, const fio_md_inline_s *i) {
  int r;
  if (!s->cb || !s->cb->on_inline)
    return 0;
  r = s->cb->on_inline(s->udata, i);
  return r ? fio___md_abort(s, r, i->source.buf) : 0;
}

FIO_IFUNC char *fio___md_label_end(char *p, char *end) {
  while (p < end) {
    if (*p == '\\' && p + 1 < end) {
      p += 2;
      continue;
    }
    if (*p == ']')
      return p;
    ++p;
  }
  return end;
}

FIO_IFUNC int fio___md_label_next_char(fio_buf_info_s *s, char *c) {
  if (!s->len)
    return 0;
  if (s->buf[0] == '\\' && s->len > 1 && fio___md_punct(s->buf[1])) {
    *c = s->buf[1];
    s->buf += 2;
    s->len -= 2;
    return 1;
  }
  *c = s->buf[0];
  ++s->buf;
  --s->len;
  return 1;
}

FIO_IFUNC int fio___md_slice_eq_lc(fio_buf_info_s a, fio_buf_info_s b) {
  char ca;
  char cb;
  while (fio___md_label_next_char(&a, &ca)) {
    if (!fio___md_label_next_char(&b, &cb) ||
        fio___md_ascii_lower(ca) != fio___md_ascii_lower(cb))
      return 0;
  }
  return !fio___md_label_next_char(&b, &cb);
}

FIO_IFUNC int fio___md_prefix_lc(char *p, char *end, const char *word) {
  while (*word) {
    if (p == end || fio___md_ascii_lower(*p) != *word)
      return 0;
    ++p;
    ++word;
  }
  return 1;
}

FIO_SFUNC int fio___md_ref_title(char *p,
                                 char *le,
                                 char *end,
                                 fio_buf_info_s *title,
                                 char **after) {
  char q;
  char close;
  char *title_start;
  p = fio___md_ltrim(p, le);
  if (p == le)
    return 0;
  q = *p;
  close = q;
  if (q == '(')
    close = ')';
  if (q != '"' && q != '\'' && q != '(')
    return 0;
  title_start = ++p;
  for (;;) {
    while (p < le) {
      if (*p == '\\' && p + 1 < le && fio___md_punct(p[1])) {
        p += 2;
        continue;
      }
      if (*p == close)
        break;
      ++p;
    }
    if (p < le) {
      char *rest = fio___md_ltrim(p + 1, le);
      if (rest != le)
        return 0;
      *title = fio___md_buf(title_start, p);
      if (after)
        *after = fio___md_line_next(le, end);
      return 1;
    }
    p = fio___md_line_next(le, end);
    if (p >= end)
      return 0;
    le = fio___md_line_end(p, end);
    if (fio___md_is_blank(p, le))
      return 0;
  }
}

FIO_SFUNC int fio___md_ref_def(char *ls,
                               char *end,
                               fio_buf_info_s *label,
                               fio_buf_info_s *dst,
                               fio_buf_info_s *title,
                               char **after) {
  char *le = fio___md_line_end(ls, end);
  char *p = fio___md_ltrim(ls, le);
  char *label_start;
  char *label_end;
  char *dst_start;
  char *dst_end;
  char *dst_line_end;
  char *ref_after;
  uint8_t angled_dst = 0;
  uint8_t has_space_after_dst = 0;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p == le || *p != '[')
    return 0;
  label_start = ++p;
  p = fio___md_label_end(p, le);
  if (p == le || p == label_start || p + 1 == le || p[1] != ':')
    return 0;
  label_end = p;
  if (*label_start == '^')
    return 0;
  p = fio___md_ltrim(p + 2, le);
  if (p == le) {
    p = fio___md_line_next(le, end);
    if (p >= end)
      return 0;
    le = fio___md_line_end(p, end);
    p = fio___md_ltrim(p, le);
    if (p == le)
      return 0;
  }
  dst_line_end = le;
  le = fio___md_rtrim(p, le);
  if (*p == '<') {
    angled_dst = 1;
    dst_start = ++p;
    while (p < le && *p != '>')
      ++p;
    if (p == le)
      return 0;
    dst_end = p++;
  } else {
    dst_start = p;
    while (p < le && *p != ' ' && *p != '\t')
      ++p;
    dst_end = p;
  }
  if (!angled_dst && dst_start == dst_end)
    return 0;
  *label = fio___md_buf(label_start, label_end);
  *dst = fio___md_buf(dst_start, dst_end);
  *title = FIO_BUF_INFO0;
  ref_after = fio___md_line_next(dst_line_end, end);
  if (p < le && (*p == ' ' || *p == '\t'))
    has_space_after_dst = 1;
  p = fio___md_ltrim(p, le);
  if (p < le) {
    if (!has_space_after_dst || (*p != '"' && *p != '\'' && *p != '(') ||
        !fio___md_ref_title(p, le, end, title, &ref_after))
      return 0;
  } else {
    char *title_line = ref_after;
    if (title_line < end) {
      char *tle = fio___md_line_end(title_line, end);
      char *tt = fio___md_ltrim(title_line, tle);
      if (tt < tle && (*tt == '"' || *tt == '\'' || *tt == '(')) {
        if (!fio___md_ref_title(title_line,
                                fio___md_rtrim(title_line, tle),
                                end,
                                title,
                                &ref_after))
          return 0;
      }
    }
  }
  if (after)
    *after = ref_after;
  return 1;
}

FIO_SFUNC int fio___md_footnote_line(char *ls,
                                     char *le,
                                     fio_buf_info_s *label,
                                     char **content) {
  char *p = fio___md_ltrim(ls, le);
  char *label_start;
  char *label_end;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p + 3 >= le ||
      p[0] != '[' || p[1] != '^')
    return 0;
  label_start = p + 2;
  p = fio___md_label_end(label_start, le);
  if (p == le || p == label_start || p + 1 == le || p[1] != ':')
    return 0;
  label_end = p;
  if (label)
    *label = fio___md_buf(label_start, label_end);
  if (content)
    *content = fio___md_ltrim(p + 2, le);
  return 1;
}

FIO_SFUNC char *fio___md_footnote_end(char *p, char *end) {
  char *le = fio___md_line_end(p, end);
  char *scan = fio___md_line_next(le, end);
  while (scan < end) {
    char *se = fio___md_line_end(scan, end);
    char *next = fio___md_line_next(se, end);
    if (fio___md_is_blank(scan, se) ||
        fio___md_indent(scan, se) >= FIO___MD_TAB_WIDTH) {
      scan = next;
      continue;
    }
    break;
  }
  return scan;
}

FIO_SFUNC fio___md_ref_s *fio___md_ref_find(fio___md_s *s,
                                            fio_buf_info_s label) {
  label.buf = fio___md_ltrim(label.buf, label.buf + label.len);
  label.len =
      (size_t)(fio___md_rtrim(label.buf, label.buf + label.len) - label.buf);
  if (s->ref_cache_valid && fio___md_slice_eq_lc(label, s->ref_cache.label))
    return &s->ref_cache;
  for (uint16_t i = 0; i < s->ref_count; ++i) {
    if (fio___md_slice_eq_lc(label, s->refs[i].label)) {
      s->ref_cache = s->refs[i];
      s->ref_cache_valid = 1;
      return &s->ref_cache;
    }
  }
  if (s->ref_overflow) {
    char *p = s->start;
    while (p < s->end) {
      char *after = NULL;
      fio_buf_info_s ref_label = FIO_BUF_INFO0;
      fio_buf_info_s dst = FIO_BUF_INFO0;
      fio_buf_info_s title = FIO_BUF_INFO0;
      if (fio___md_ref_def(p, s->end, &ref_label, &dst, &title, &after) &&
          fio___md_slice_eq_lc(label, ref_label)) {
        s->ref_cache.label = ref_label;
        s->ref_cache.destination = dst;
        s->ref_cache.title = title;
        s->ref_cache_valid = 1;
        return &s->ref_cache;
      }
      p = after ? after
                : fio___md_line_next(fio___md_line_end(p, s->end), s->end);
    }
  }
  return NULL;
}

FIO_SFUNC void fio___md_ref_index(fio___md_s *s) {
  char *p = s->start;
  while (p < s->end) {
    char *after = NULL;
    fio_buf_info_s label = FIO_BUF_INFO0;
    fio_buf_info_s dst = FIO_BUF_INFO0;
    fio_buf_info_s title = FIO_BUF_INFO0;
    if (fio___md_ref_def(p, s->end, &label, &dst, &title, &after)) {
      uint16_t i = 0;
      while (i < s->ref_count && !fio___md_slice_eq_lc(label, s->refs[i].label))
        ++i;
      if (i == s->ref_count) {
        if (s->ref_count < FIO_MARKDOWN_MAX_REFERENCES) {
          s->refs[s->ref_count].label = label;
          s->refs[s->ref_count].destination = dst;
          s->refs[s->ref_count].title = title;
          ++s->ref_count;
        } else {
          s->ref_overflow = 1;
        }
      }
    }
    p = after ? after
              : fio___md_line_next(fio___md_line_end(p, s->end), s->end);
  }
}

FIO_SFUNC int fio___md_parse_inline_span(fio___md_s *s, char *p, char *end);
FIO_SFUNC int fio___md_inline_break(fio___md_s *s,
                                    int hard,
                                    char *break_start,
                                    char *break_end);

FIO_SFUNC char *fio___md_find_closing(char *p,
                                      char *end,
                                      char marker,
                                      size_t count) {
  while (p < end) {
    size_t n = 0;
    if (*p == '\\') {
      p += (p + 1 < end) ? 2 : 1;
      continue;
    }
    while (p + n < end && p[n] == marker)
      ++n;
    if (n >= count)
      return p;
    ++p;
  }
  return NULL;
}

FIO_SFUNC int fio___md_inline_text(fio___md_s *s, char *p, char *end) {
  fio_md_inline_s i = {0};
  if (p == end)
    return 0;
  i.type = FIO_MD_INLINE_TEXT;
  i.event = FIO_MD_EVENT_LEAF;
  i.source = fio___md_buf(p, end);
  i.text = i.source;
  return fio___md_inline(s, &i);
}

FIO_SFUNC int fio___md_inline_container(fio___md_s *s,
                                        fio_md_inline_type_e type,
                                        char *open,
                                        char *inner,
                                        char *inner_end,
                                        char *close_end) {
  fio_md_inline_s i = {0};
  if (s->depth >= FIO_MARKDOWN_MAX_DEPTH)
    return fio___md_abort(s, FIO_MD_ERR_DEPTH, open);
  i.type = type;
  i.event = FIO_MD_EVENT_ENTER;
  i.source = fio___md_buf(open, close_end);
  i.text = fio___md_buf(inner, inner_end);
  if (fio___md_inline(s, &i))
    return -1;
  ++s->depth;
  if (fio___md_parse_inline_span(s, inner, inner_end))
    return -1;
  --s->depth;
  i.event = FIO_MD_EVENT_LEAVE;
  return fio___md_inline(s, &i);
}

FIO_SFUNC int fio___md_try_link(fio___md_s *s,
                                char *p,
                                char *end,
                                char **after) {
  char *label_start = p + (*p == '!' ? 2 : 1);
  char *label_end = label_start;
  int is_image = (*p == '!');
  fio_md_inline_s i = {0};
  if ((is_image && p + 1 == end) || (is_image && p[1] != '['))
    return 0;
  label_end = fio___md_label_end(label_end, end);
  if (label_end == end)
    return 0;
  if (label_end + 1 < end && label_end[1] == '(') {
    char *dst_start = fio___md_ltrim(label_end + 2, end);
    char *dst_end = dst_start;
    char *title_start = NULL;
    char *title_end = NULL;
    char *q;
    while (dst_end < end && *dst_end != ')' && *dst_end != ' ' &&
           *dst_end != '\t')
      ++dst_end;
    q = fio___md_ltrim(dst_end, end);
    if (q < end && (*q == '"' || *q == '\'')) {
      char quote = *q++;
      title_start = q;
      while (q < end && *q != quote)
        ++q;
      if (q < end) {
        title_end = q++;
        q = fio___md_ltrim(q, end);
      }
    }
    while (q < end && *q != ')')
      ++q;
    if (q == end)
      return 0;
    i.type = is_image ? FIO_MD_INLINE_IMAGE : FIO_MD_INLINE_LINK;
    i.event = is_image ? FIO_MD_EVENT_LEAF : FIO_MD_EVENT_ENTER;
    i.source = fio___md_buf(p, q + 1);
    i.text = fio___md_buf(label_start, label_end);
    i.destination = fio___md_buf(dst_start, dst_end);
    if (title_start && title_end)
      i.title = fio___md_buf(title_start, title_end);
    if (fio___md_inline(s, &i))
      return -1;
    if (!is_image) {
      if (s->depth >= FIO_MARKDOWN_MAX_DEPTH)
        return fio___md_abort(s, FIO_MD_ERR_DEPTH, p);
      ++s->depth;
      if (fio___md_parse_inline_span(s, label_start, label_end))
        return -1;
      --s->depth;
      i.event = FIO_MD_EVENT_LEAVE;
      if (fio___md_inline(s, &i))
        return -1;
    }
    *after = q + 1;
    return 1;
  }
  if (!is_image && label_end + 1 < end && label_end[1] == '[') {
    char *ref_start = label_end + 2;
    char *ref_end = ref_start;
    fio_buf_info_s ref_label;
    fio___md_ref_s *ref;
    ref_end = fio___md_label_end(ref_end, end);
    if (ref_end == end)
      return 0;
    ref_label = (ref_start == ref_end) ? fio___md_buf(label_start, label_end)
                                       : fio___md_buf(ref_start, ref_end);
    ref = fio___md_ref_find(s, ref_label);
    if (!ref)
      return 0;
    i.type = FIO_MD_INLINE_LINK;
    i.event = FIO_MD_EVENT_ENTER;
    i.source = fio___md_buf(p, ref_end + 1);
    i.text = fio___md_buf(label_start, label_end);
    i.destination = ref->destination;
    i.title = ref->title;
    i.reference = ref_label;
    if (fio___md_inline(s, &i))
      return -1;
    ++s->depth;
    if (fio___md_parse_inline_span(s, label_start, label_end))
      return -1;
    --s->depth;
    i.event = FIO_MD_EVENT_LEAVE;
    if (fio___md_inline(s, &i))
      return -1;
    *after = ref_end + 1;
    return 1;
  }
  if (!is_image) {
    fio___md_ref_s *ref =
        fio___md_ref_find(s, fio___md_buf(label_start, label_end));
    if (!ref)
      return 0;
    i.type = FIO_MD_INLINE_LINK;
    i.event = FIO_MD_EVENT_ENTER;
    i.source = fio___md_buf(p, label_end + 1);
    i.text = fio___md_buf(label_start, label_end);
    i.destination = ref->destination;
    i.title = ref->title;
    i.reference = i.text;
    if (fio___md_inline(s, &i))
      return -1;
    ++s->depth;
    if (fio___md_parse_inline_span(s, label_start, label_end))
      return -1;
    --s->depth;
    i.event = FIO_MD_EVENT_LEAVE;
    if (fio___md_inline(s, &i))
      return -1;
    *after = label_end + 1;
    return 1;
  }
  return 0;
}

FIO_SFUNC int fio___md_try_autolink_or_html(fio___md_s *s,
                                            char *p,
                                            char *end,
                                            char **after) {
  char *q = p + 1;
  fio_md_inline_s i = {0};
  if (p == end || *p != '<')
    return 0;
  while (q < end && *q != '>' && *q != ' ' && *q != '\t')
    ++q;
  if (q < end && *q == '>' &&
      (fio___md_prefix_lc(p + 1, q, "http://") ||
       fio___md_prefix_lc(p + 1, q, "https://") ||
       (FIO_MEMCHR(p + 1, '@', (size_t)(q - p - 1)) != NULL))) {
    i.type = FIO_MD_INLINE_AUTOLINK;
    i.event = FIO_MD_EVENT_LEAF;
    i.source = fio___md_buf(p, q + 1);
    i.destination = fio___md_buf(p + 1, q);
    *after = q + 1;
    return fio___md_inline(s, &i) ? -1 : 1;
  }
  q = p + 1;
  while (q < end && *q != '>')
    ++q;
  if (q < end && *q == '>') {
    i.type = FIO_MD_INLINE_HTML;
    i.event = FIO_MD_EVENT_LEAF;
    i.source = fio___md_buf(p, q + 1);
    i.text = i.source;
    *after = q + 1;
    return fio___md_inline(s, &i) ? -1 : 1;
  }
  return 0;
}

FIO_SFUNC int fio___md_parse_inline_span(fio___md_s *s, char *p, char *end) {
  char *text = p;
  while (p < end) {
    char *after = p;
    if (*p == '\n' || *p == '\r') {
      int hard = 0;
      char *text_end = p;
      char *next = p;
      if (text_end > text && text_end[-1] == '\\') {
        hard = 1;
        --text_end;
      } else if (text_end > text + 1 && text_end[-1] == ' ' &&
                 text_end[-2] == ' ') {
        hard = 1;
        text_end -= 2;
      } else {
        text_end = fio___md_rtrim(text, text_end);
      }
      if (fio___md_inline_text(s, text, text_end))
        return -1;
      if (*next == '\r')
        ++next;
      if (next < end && *next == '\n')
        ++next;
      if (fio___md_inline_break(s, hard, text_end, next))
        return -1;
      p = fio___md_ltrim(next, end);
      text = p;
      continue;
    }
    if (*p == '`') {
      size_t n = 1;
      char *q;
      fio_md_inline_s i = {0};
      while (p + n < end && p[n] == '`')
        ++n;
      q = fio___md_find_closing(p + n, end, '`', n);
      if (q) {
        if (fio___md_inline_text(s, text, p))
          return -1;
        i.type = FIO_MD_INLINE_CODE_SPAN;
        i.event = FIO_MD_EVENT_LEAF;
        i.source = fio___md_buf(p, q + n);
        i.text = fio___md_buf(p + n, q);
        if (fio___md_inline(s, &i))
          return -1;
        p = q + n;
        text = p;
        continue;
      }
    }
    if ((*p == '*' || *p == '_') && p + 1 < end && p[1] == *p) {
      char *q = fio___md_find_closing(p + 2, end, *p, 2);
      if (q && q > p + 2 && fio___md_span_has_nonspace(p + 2, q)) {
        if (fio___md_inline_text(s, text, p))
          return -1;
        if (fio___md_inline_container(s,
                                      FIO_MD_INLINE_STRONG,
                                      p,
                                      p + 2,
                                      q,
                                      q + 2))
          return -1;
        p = q + 2;
        text = p;
        continue;
      }
    }
    if (*p == '*' || *p == '_') {
      char *q = fio___md_find_closing(p + 1, end, *p, 1);
      if (q && q > p + 1 && fio___md_span_has_nonspace(p + 1, q)) {
        if (fio___md_inline_text(s, text, p))
          return -1;
        if (fio___md_inline_container(s,
                                      FIO_MD_INLINE_EMPHASIS,
                                      p,
                                      p + 1,
                                      q,
                                      q + 1))
          return -1;
        p = q + 1;
        text = p;
        continue;
      }
    }
    if (*p == '~' && p + 1 < end && p[1] == '~') {
      char *q = fio___md_find_closing(p + 2, end, '~', 2);
      if (q && fio___md_span_has_nonspace(p + 2, q)) {
        if (fio___md_inline_text(s, text, p))
          return -1;
        if (fio___md_inline_container(s,
                                      FIO_MD_INLINE_STRIKETHROUGH,
                                      p,
                                      p + 2,
                                      q,
                                      q + 2))
          return -1;
        p = q + 2;
        text = p;
        continue;
      }
    }
    if (*p == '!' || *p == '[') {
      int r;
      if (*p == '!' && (p + 1 == end || p[1] != '[')) {
        ++p;
        continue;
      }
      if (text < p) {
        if (fio___md_inline_text(s, text, p))
          return -1;
        text = p;
        continue;
      }
      r = fio___md_try_link(s, p, end, &after);
      if (r < 0)
        return -1;
      if (r) {
        if (fio___md_inline_text(s, text, p))
          return -1;
        p = after;
        text = p;
        continue;
      }
    }
    if (*p == '<') {
      int r;
      if (text < p) {
        if (fio___md_inline_text(s, text, p))
          return -1;
        text = p;
        continue;
      }
      r = fio___md_try_autolink_or_html(s, p, end, &after);
      if (r < 0)
        return -1;
      if (r) {
        if (fio___md_inline_text(s, text, p))
          return -1;
        p = after;
        text = p;
        continue;
      }
    }
    if (*p == '\\' && p + 1 < end && fio___md_punct(p[1])) {
      fio_md_inline_s i = {0};
      if (fio___md_inline_text(s, text, p))
        return -1;
      i.type = FIO_MD_INLINE_ESCAPE;
      i.event = FIO_MD_EVENT_LEAF;
      i.source = fio___md_buf(p, p + 2);
      i.text = fio___md_buf(p + 1, p + 2);
      if (fio___md_inline(s, &i))
        return -1;
      p += 2;
      text = p;
      continue;
    }
    if (*p == '[' && p + 2 < end && p[1] == '^') {
      char *q = fio___md_label_end(p + 2, end);
      if (q < end && q > p + 2) {
        fio_md_inline_s i = {0};
        if (fio___md_inline_text(s, text, p))
          return -1;
        i.type = FIO_MD_INLINE_FOOTNOTE_REF;
        i.event = FIO_MD_EVENT_LEAF;
        i.source = fio___md_buf(p, q + 1);
        i.reference = fio___md_buf(p + 2, q);
        if (fio___md_inline(s, &i))
          return -1;
        p = q + 1;
        text = p;
        continue;
      }
    }
    if (*p == '&') {
      char *q = p + 1;
      while (q < end && q - p < FIO___MD_MAX_ENTITY_LEN &&
             ((*q >= 'a' && *q <= 'z') || (*q >= 'A' && *q <= 'Z') ||
              (*q >= '0' && *q <= '9') || *q == '#'))
        ++q;
      if (q < end && *q == ';') {
        fio_md_inline_s i = {0};
        if (fio___md_inline_text(s, text, p))
          return -1;
        i.type = FIO_MD_INLINE_ENTITY;
        i.event = FIO_MD_EVENT_LEAF;
        i.source = fio___md_buf(p, q + 1);
        i.text = i.source;
        if (fio___md_inline(s, &i))
          return -1;
        p = q + 1;
        text = p;
        continue;
      }
    }
    ++p;
  }
  return fio___md_inline_text(s, text, end);
}

FIO_SFUNC int fio___md_inline_break(fio___md_s *s,
                                    int hard,
                                    char *break_start,
                                    char *break_end) {
  fio_md_inline_s br = {0};
  br.type = hard ? FIO_MD_INLINE_HARD_BREAK : FIO_MD_INLINE_SOFT_BREAK;
  br.event = FIO_MD_EVENT_LEAF;
  br.source = fio___md_buf(break_start, break_end);
  return fio___md_inline(s, &br);
}

FIO_SFUNC int fio___md_inline_lines(fio___md_s *s, char *p, char *end) {
  p = fio___md_ltrim(p, end);
  end = fio___md_rtrim(p, end);
  return fio___md_parse_inline_span(s, p, end);
}

FIO_SFUNC int fio___md_atx(char *ls,
                           char *le,
                           uint8_t *level,
                           char **content_start,
                           char **content_end,
                           fio_buf_info_s *marker) {
  char *p = fio___md_ltrim(ls, le);
  char *q;
  *level = 0;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p == le || *p != '#')
    return 0;
  q = p;
  while (q < le && *q == '#')
    ++q;
  if (q - p > FIO___MD_MAX_ATX_HEADING || (q < le && *q != ' ' && *q != '\t'))
    return 0;
  *level = (uint8_t)(q - p);
  *marker = fio___md_buf(p, q);
  q = fio___md_ltrim(q, le);
  le = fio___md_rtrim(q, le);
  if (le > q && le[-1] == '#') {
    char *hash_start = le;
    while (hash_start > q && hash_start[-1] == '#')
      --hash_start;
    if (hash_start == q)
      le = q;
    else if (hash_start > q &&
             (hash_start[-1] == ' ' || hash_start[-1] == '\t'))
      le = fio___md_rtrim(q, hash_start - 1);
  }
  *content_start = q;
  *content_end = le;
  return 1;
}

FIO_SFUNC int fio___md_setext(char *ls, char *le, uint8_t *level) {
  char *p = fio___md_ltrim(ls, le);
  char c;
  size_t n = 0;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p == le)
    return 0;
  c = *p;
  if (c != '=' && c != '-')
    return 0;
  le = fio___md_rtrim(p, le);
  while (p < le) {
    if (*p != c)
      return 0;
    ++n;
    ++p;
  }
  if (!n)
    return 0;
  *level = (uint8_t)((c == '=') ? 1U : 2U);
  return 1;
}

FIO_SFUNC int fio___md_thematic(char *ls, char *le) {
  char *p = fio___md_ltrim(ls, le);
  char c;
  size_t n = 0;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p == le)
    return 0;
  c = *p;
  if (c != '*' && c != '-' && c != '_')
    return 0;
  while (p < le) {
    if (*p == c)
      ++n;
    else if (*p != ' ' && *p != '\t')
      return 0;
    ++p;
  }
  return n >= 3;
}

FIO_SFUNC int fio___md_fence(char *ls,
                             char *le,
                             fio_buf_info_s *marker,
                             fio_buf_info_s *info) {
  char *p = fio___md_ltrim(ls, le);
  char *q;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p == le ||
      (*p != '`' && *p != '~'))
    return 0;
  q = p;
  while (q < le && *q == *p)
    ++q;
  if (q - p < FIO___MD_MIN_FENCE_LEN)
    return 0;
  if (*p == '`' && FIO_MEMCHR(q, '`', (size_t)(le - q)))
    return 0;
  *marker = fio___md_buf(p, q);
  *info = fio___md_buf(fio___md_ltrim(q, le), fio___md_rtrim(q, le));
  return 1;
}

FIO_SFUNC int fio___md_fence_close(char *ls, char *le, fio_buf_info_s marker) {
  char *p = fio___md_ltrim(ls, le);
  size_t n = 0;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || !marker.len)
    return 0;
  while (p < le && *p == marker.buf[0]) {
    ++n;
    ++p;
  }
  if (n < marker.len)
    return 0;
  p = fio___md_ltrim(p, le);
  return p == le;
}

FIO_SFUNC int fio___md_list_marker(char *ls,
                                   char *le,
                                   int *ordered,
                                   uint64_t *start,
                                   char **content,
                                   fio_buf_info_s *marker) {
  char *p = fio___md_ltrim(ls, le);
  char *m = p;
  uint64_t num = 0;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p == le)
    return 0;
  if (*p == '-' || *p == '+' || *p == '*') {
    if (p + 1 < le && p[1] != ' ' && p[1] != '\t')
      return 0;
    *ordered = 0;
    *start = 0;
    *marker = fio___md_buf(p, p + 1);
    *content = fio___md_ltrim(p + 1, le);
    return 1;
  }
  if (*p < '0' || *p > '9')
    return 0;
  while (p < le && *p >= '0' && *p <= '9') {
    num = (num * 10) + (uint64_t)(*p - '0');
    ++p;
  }
  if (p == m || p == le || (*p != '.' && *p != ')'))
    return 0;
  ++p;
  if (p < le && *p != ' ' && *p != '\t')
    return 0;
  *ordered = 1;
  *start = num;
  *marker = fio___md_buf(m, p);
  *content = fio___md_ltrim(p, le);
  return 1;
}

FIO_SFUNC uint32_t fio___md_task_flags(char **content, char *end) {
  char *p = *content;
  if (p + 3 <= end && p[0] == '[' && p[2] == ']' &&
      (p[1] == ' ' || p[1] == 'x' || p[1] == 'X') &&
      (p + 3 == end || p[3] == ' ' || p[3] == '\t')) {
    *content = fio___md_ltrim(p + 3, end);
    return FIO_MD_BLOCK_F_TASK |
           ((p[1] != ' ') ? FIO_MD_BLOCK_F_TASK_CHECKED : 0);
  }
  return 0;
}

FIO_SFUNC char *fio___md_table_next_pipe(char *p, char *end) {
  size_t code_ticks = 0;
  while (p < end) {
    if (*p == '\\') {
      p += (p + 1 < end) ? 2 : 1;
      continue;
    }
    if (*p == '`') {
      size_t n = 1;
      while (p + n < end && p[n] == '`')
        ++n;
      if (!code_ticks)
        code_ticks = n;
      else if (code_ticks == n)
        code_ticks = 0;
      p += n;
      continue;
    }
    if (!code_ticks && *p == '|')
      return p;
    ++p;
  }
  return end;
}

FIO_SFUNC int fio___md_table_delim(char *ls,
                                   char *le,
                                   uint8_t *aligns,
                                   uint16_t *columns) {
  char *p = fio___md_ltrim(ls, le);
  uint16_t col = 0;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT)
    return 0;
  le = fio___md_rtrim(p, le);
  if (p < le && *p == '|')
    ++p;
  if (p < le && le[-1] == '|')
    --le;
  while (p < le && col < FIO_MARKDOWN_MAX_TABLE_COLUMNS) {
    char *cell = p;
    char *ce;
    char *t;
    p = fio___md_table_next_pipe(p, le);
    ce = fio___md_rtrim(cell, p);
    t = fio___md_ltrim(cell, ce);
    if (t == ce)
      return 0;
    aligns[col] = FIO_MD_ALIGN_NONE;
    if (*t == ':') {
      aligns[col] = FIO_MD_ALIGN_LEFT;
      ++t;
    }
    if (t == ce || *t != '-')
      return 0;
    while (t < ce && *t == '-')
      ++t;
    if (t < ce && *t == ':') {
      aligns[col] = (aligns[col] == FIO_MD_ALIGN_LEFT) ? FIO_MD_ALIGN_CENTER
                                                       : FIO_MD_ALIGN_RIGHT;
      ++t;
    }
    t = fio___md_ltrim(t, ce);
    if (t != ce)
      return 0;
    ++col;
    if (p < le && *p == '|')
      ++p;
  }
  *columns = col;
  return col > 0 && p >= le;
}

FIO_SFUNC uint16_t fio___md_count_table_cells(char *ls, char *le) {
  char *p = fio___md_ltrim(ls, le);
  uint16_t n = 0;
  le = fio___md_rtrim(p, le);
  if (p < le && *p == '|')
    ++p;
  if (p < le && le[-1] == '|')
    --le;
  while (p < le) {
    p = fio___md_table_next_pipe(p, le);
    ++n;
    if (p < le && *p == '|')
      ++p;
  }
  return n;
}

FIO_SFUNC int fio___md_html_block_tag(char *p, char *end) {
  static const char tags[] =
      "|address|article|aside|base|basefont|blockquote|body|caption|center|"
      "col|colgroup|dd|details|dialog|dir|div|dl|dt|fieldset|figcaption|"
      "figure|footer|form|frame|frameset|h1|h2|h3|h4|h5|h6|head|header|"
      "hr|html|iframe|legend|li|link|main|menu|menuitem|nav|noframes|ol|"
      "optgroup|option|p|param|section|summary|table|tbody|td|tfoot|th|"
      "thead|title|tr|track|ul|";
  char tag[16];
  size_t len = 0;
  char needle[20];
  if (p < end && *p == '/')
    ++p;
  while (p < end && len + 1 < sizeof(tag) &&
         ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
          (*p >= '0' && *p <= '9'))) {
    tag[len++] = fio___md_ascii_lower(*p);
    ++p;
  }
  if (!len || len + 2 > sizeof(needle))
    return 0;
  needle[0] = '|';
  FIO_MEMCPY(needle + 1, tag, len);
  needle[len + 1] = '|';
  for (size_t i = 0; i + len + 2 <= sizeof(tags) - 1; ++i) {
    if (tags[i] == '|' && FIO_MEMCMP(tags + i, needle, len + 2) == 0)
      return 1;
  }
  return 0;
}

FIO_SFUNC int fio___md_html_long_block(char *ls, char *le) {
  char *p = fio___md_ltrim(ls, le);
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p + 2 >= le ||
      *p != '<')
    return 0;
  if (p[1] == '?')
    return 1;
  if (p + 3 < le && p[1] == '!' && p[2] == '-' && p[3] == '-')
    return 1;
  if (p + 8 < le && p[1] == '!' && p[2] == '[' &&
      fio___md_prefix_lc(p + 3, le, "cdata["))
    return 1;
  return fio___md_prefix_lc(p + 1, le, "script") ||
         fio___md_prefix_lc(p + 1, le, "pre") ||
         fio___md_prefix_lc(p + 1, le, "style");
}

FIO_SFUNC int fio___md_html_long_close(char *ls, char *le) {
  char *p = ls;
  while (p < le) {
    if (p + 2 < le && p[0] == '-' && p[1] == '-' && p[2] == '>')
      return 1;
    if (p + 1 < le && p[0] == '?' && p[1] == '>')
      return 1;
    if (p + 2 < le && p[0] == ']' && p[1] == ']' && p[2] == '>')
      return 1;
    if (*p == '<' && p + 2 < le && p[1] == '/' &&
        (fio___md_prefix_lc(p + 2, le, "script") ||
         fio___md_prefix_lc(p + 2, le, "pre") ||
         fio___md_prefix_lc(p + 2, le, "style")))
      return 1;
    ++p;
  }
  return 0;
}

FIO_SFUNC int fio___md_html_block_start(char *ls, char *le) {
  char *p = fio___md_ltrim(ls, le);
  char *q;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p == le || *p != '<')
    return 0;
  if (p + 1 == le)
    return 0;
  if (p[1] == '!' || p[1] == '?')
    return 1;
  if (fio___md_html_long_block(p, le))
    return 1;
  if (fio___md_html_block_tag(p + 1, le))
    return 1;
  q = p + 1;
  while (q < le && *q != '>')
    ++q;
  if (q == le)
    return 0;
  q = fio___md_ltrim(q + 1, le);
  return q == le;
}

FIO_SFUNC int fio___md_parse_paragraph(fio___md_s *s, char *p, char *end) {
  fio_md_block_s b = {0};
  b.type = FIO_MD_BLOCK_PARAGRAPH;
  b.source = fio___md_buf(p, end);
  if (fio___md_block_enter(s, &b))
    return -1;
  if (fio___md_inline_lines(s, p, end))
    return -1;
  return fio___md_block_leave(s, &b);
}

FIO_SFUNC int fio___md_emit_html_block(fio___md_s *s, char *p, char *end) {
  fio_md_block_s b = {0};
  b.type = FIO_MD_BLOCK_HTML;
  b.source = fio___md_buf(p, end);
  b.content = b.source;
  return (fio___md_block_enter(s, &b) || fio___md_block_leave(s, &b)) ? -1 : 0;
}

FIO_SFUNC int fio___md_parse_table_row(fio___md_s *s,
                                       char *ls,
                                       char *le,
                                       uint8_t *aligns,
                                       uint16_t columns) {
  char *p = ls;
  uint16_t col = 0;
  fio_md_block_s row = {0};
  row.type = FIO_MD_BLOCK_TABLE_ROW;
  row.source = fio___md_buf(ls, le);
  row.columns = columns;
  if (fio___md_block_enter(s, &row))
    return -1;
  le = fio___md_rtrim(p, le);
  if (p < le && *p == '|')
    ++p;
  if (p < le && le[-1] == '|')
    --le;
  while (p <= le && col < columns) {
    char *cell_start = p;
    char *cell_end;
    fio_md_block_s cell = {0};
    p = fio___md_table_next_pipe(p, le);
    cell_end = p;
    cell.type = FIO_MD_BLOCK_TABLE_CELL;
    cell.source = fio___md_buf(cell_start, cell_end);
    cell.content = fio___md_buf(fio___md_ltrim(cell_start, cell_end),
                                fio___md_rtrim(cell_start, cell_end));
    cell.column = col;
    cell.columns = columns;
    cell.align = aligns[col];
    if (fio___md_block_enter(s, &cell))
      return -1;
    if (fio___md_parse_inline_span(s,
                                   cell.content.buf,
                                   cell.content.buf + cell.content.len))
      return -1;
    if (fio___md_block_leave(s, &cell))
      return -1;
    ++col;
    if (p < le && *p == '|')
      ++p;
    else
      break;
  }
  return fio___md_block_leave(s, &row);
}

FIO_SFUNC int fio___md_parse_blocks(fio___md_s *s, char *p, char *end);
FIO_SFUNC int fio___md_parse_blocks_ex(fio___md_s *s,
                                       char *p,
                                       char *end,
                                       uint8_t initial_padding);

FIO_SFUNC int fio___md_blockquote_line(char *p,
                                       char *end,
                                       char **content,
                                       uint8_t *padding,
                                       char **line_end,
                                       char **next_line) {
  char *le = fio___md_line_end(p, end);
  char *qt = fio___md_ltrim(p, le);
  if (qt == le || *qt != '>' || (uint32_t)(qt - p) > FIO___MD_MAX_MARKER_INDENT)
    return 0;
  *content = qt + 1;
  *padding = 0;
  if (*content < le && **content == ' ') {
    ++*content;
  } else if (*content < le && **content == '\t') {
    ++*content;
    *padding = 2;
  }
  *line_end = le;
  *next_line = fio___md_line_next(le, end);
  return 1;
}

FIO_SFUNC int fio___md_block_start_line(char *p, char *le) {
  uint8_t level = 0;
  int ordered = 0;
  uint64_t start_num = 0;
  char *content = NULL;
  fio_buf_info_s marker = FIO_BUF_INFO0;
  fio_buf_info_s info = FIO_BUF_INFO0;
  char *content_start = NULL;
  char *content_end = NULL;
  char *trim = fio___md_ltrim(p, le);
  return fio___md_atx(p, le, &level, &content_start, &content_end, &marker) ||
         fio___md_thematic(p, le) ||
         fio___md_indent(p, le) >= FIO___MD_TAB_WIDTH ||
         fio___md_fence(p, le, &marker, &info) ||
         fio___md_html_block_start(p, le) ||
         fio___md_list_marker(p, le, &ordered, &start_num, &content, &marker) ||
         (trim < le && *trim == '>' &&
          (uint32_t)(trim - p) <= FIO___MD_MAX_MARKER_INDENT);
}

FIO_SFUNC int fio___md_parse_list(fio___md_s *s, char *p, char *end) {
  char *list_start = p;
  char *cur = p;
  int ordered = 0;
  int list_ordered = 0;
  uint64_t start_num = 0;
  char *content;
  fio_buf_info_s marker = FIO_BUF_INFO0;
  fio_md_block_s list = {0};
  uint32_t base_indent;
  if (!fio___md_list_marker(p,
                            fio___md_line_end(p, end),
                            &ordered,
                            &start_num,
                            &content,
                            &marker))
    return 0;
  list_ordered = ordered;
  base_indent = fio___md_indent(p, marker.buf);
  list.type =
      list_ordered ? FIO_MD_BLOCK_LIST_ORDERED : FIO_MD_BLOCK_LIST_UNORDERED;
  list.list_start = start_num;
  list.marker = marker;
  list.flags = FIO_MD_BLOCK_F_TIGHT;
  for (char *scan = cur; scan < end;) {
    char *se = fio___md_line_end(scan, end);
    char *sn = fio___md_line_next(se, end);
    char *scan_content = NULL;
    fio_buf_info_s scan_marker = FIO_BUF_INFO0;
    if (fio___md_is_blank(scan, se)) {
      char *look = sn;
      int look_ordered = 0;
      uint64_t look_start = 0;
      char *look_content = NULL;
      fio_buf_info_s look_marker = FIO_BUF_INFO0;
      while (look < end) {
        char *look_end = fio___md_line_end(look, end);
        if (!fio___md_is_blank(look, look_end))
          break;
        look = fio___md_line_next(look_end, end);
      }
      if (look >= end)
        break;
      if (fio___md_list_marker(look,
                               fio___md_line_end(look, end),
                               &look_ordered,
                               &look_start,
                               &look_content,
                               &look_marker) &&
          fio___md_indent(look, look_marker.buf) >= base_indent) {
        list.flags &= ~FIO_MD_BLOCK_F_TIGHT;
        scan = sn;
        continue;
      }
      if (fio___md_indent(look, fio___md_line_end(look, end)) >=
          base_indent + 2) {
        list.flags &= ~FIO_MD_BLOCK_F_TIGHT;
        scan = sn;
        continue;
      }
      break;
    }
    if (!fio___md_list_marker(scan,
                              se,
                              &ordered,
                              &start_num,
                              &scan_content,
                              &scan_marker) ||
        fio___md_indent(scan, scan_marker.buf) < base_indent)
      break;
    scan = sn;
  }
  if (fio___md_block_enter(s, &list))
    return -1;
  while (cur < end) {
    char *le = fio___md_line_end(cur, end);
    char *next = fio___md_line_next(le, end);
    char *item_content;
    fio_md_block_s item = {0};
    uint32_t indent;
    if (fio___md_is_blank(cur, le)) {
      cur = next;
      continue;
    }
    if (cur != list_start && fio___md_thematic(cur, le))
      break;
    if (!fio___md_list_marker(cur,
                              le,
                              &ordered,
                              &start_num,
                              &item_content,
                              &marker) ||
        ordered != list_ordered)
      break;
    indent = fio___md_indent(cur, marker.buf);
    if (indent < base_indent)
      break;
    if (indent > base_indent)
      break;
    item.type = FIO_MD_BLOCK_LIST_ITEM;
    item.source = fio___md_buf(cur, le);
    item.marker = marker;
    item.list_start = start_num;
    uint8_t item_padding = 0;
    uint32_t content_indent = fio___md_column_to(cur, item_content);
    {
      char *after_marker = marker.buf + marker.len;
      if (after_marker < le && *after_marker == '\t') {
        uint32_t marker_end_col = fio___md_column_to(cur, after_marker);
        uint32_t content_col = marker_end_col + 1;
        uint32_t tab_end_col =
            marker_end_col +
            (FIO___MD_TAB_WIDTH - (marker_end_col & (FIO___MD_TAB_WIDTH - 1U)));
        item_content = after_marker + 1;
        item_padding = (uint8_t)(tab_end_col - content_col);
        content_indent = content_col;
      }
    }
    item.flags = fio___md_task_flags(&item_content, le) |
                 (list.flags & FIO_MD_BLOCK_F_TIGHT);
    if (item_content < le &&
        ((uint32_t)item_padding + fio___md_indent(item_content, le) >=
             FIO___MD_TAB_WIDTH ||
         fio___md_block_start_line(item_content, le)))
      item.flags &= (uint8_t)~FIO_MD_BLOCK_F_TIGHT;
    if (fio___md_block_enter(s, &item))
      return -1;
    if (item_content < le) {
      if (fio___md_thematic(item_content, le)) {
        fio_md_block_s hr = {0};
        hr.type = FIO_MD_BLOCK_THEMATIC_BREAK;
        hr.source = fio___md_buf(item_content, le);
        hr.marker = fio___md_buf(fio___md_ltrim(item_content, le),
                                 fio___md_rtrim(item_content, le));
        if (fio___md_block_enter(s, &hr) || fio___md_block_leave(s, &hr))
          return -1;
      } else if ((uint32_t)item_padding + fio___md_indent(item_content, le) >=
                     FIO___MD_TAB_WIDTH ||
                 fio___md_block_start_line(item_content, le)) {
        if (fio___md_parse_blocks_ex(s, item_content, next, item_padding))
          return -1;
      } else {
        fio_md_block_s para = {0};
        para.type = FIO_MD_BLOCK_PARAGRAPH;
        para.source = fio___md_buf(item_content, le);
        if (fio___md_block_enter(s, &para))
          return -1;
        if (fio___md_parse_inline_span(s,
                                       item_content,
                                       fio___md_rtrim(item_content, le)))
          return -1;
        if (fio___md_block_leave(s, &para))
          return -1;
      }
    }
    cur = next;
    while (cur < end) {
      char *ne = fio___md_line_end(cur, end);
      char *unused_content = NULL;
      fio_buf_info_s nested_marker = FIO_BUF_INFO0;
      uint64_t nested_start = 0;
      int nested_ordered = 0;
      uint32_t nested_indent;
      if (fio___md_is_blank(cur, ne)) {
        cur = fio___md_line_next(ne, end);
        continue;
      }
      if (fio___md_list_marker(cur,
                               ne,
                               &nested_ordered,
                               &nested_start,
                               &unused_content,
                               &nested_marker)) {
        nested_indent = fio___md_indent(cur, nested_marker.buf);
        if (nested_indent <= base_indent)
          break;
        if (fio___md_parse_list(s, cur, end) < 0)
          return -1;
        cur = s->start + s->consumed;
        continue;
      }
      if (fio___md_indent(cur, ne) >= content_indent) {
        uint8_t child_padding = 0;
        char *child =
            fio___md_skip_indent2(cur, ne, content_indent, &child_padding);
        if (!child)
          break;
        char *child_end = fio___md_line_next(ne, end);
        if (fio___md_parse_blocks_ex(s, child, child_end, child_padding))
          return -1;
        cur = child_end;
        continue;
      }
      break;
    }
    if (fio___md_block_leave(s, &item))
      return -1;
  }
  list.source = fio___md_buf(list_start, cur);
  if (fio___md_block_leave(s, &list))
    return -1;
  s->consumed = (size_t)(cur - s->start);
  return 1;
}

FIO_SFUNC int fio___md_blockquote_para_close(fio___md_s *s,
                                             fio_md_block_s *para,
                                             char *para_start,
                                             char *para_end) {
  para->source = fio___md_buf(para_start, para_end);
  return fio___md_block_leave(s, para);
}

FIO_SFUNC int fio___md_parse_blockquote_list(fio___md_s *s,
                                             char *p,
                                             char *end,
                                             char **after) {
  char *list_start = p;
  char *qcontent;
  char *qe;
  char *next;
  uint8_t qpadding = 0;
  int ordered = 0;
  int list_ordered = 0;
  uint64_t start_num = 0;
  char *item_content = NULL;
  fio_buf_info_s marker = FIO_BUF_INFO0;
  fio_md_block_s list = {0};
  if (!fio___md_blockquote_line(p, end, &qcontent, &qpadding, &qe, &next) ||
      !fio___md_list_marker(qcontent,
                            qe,
                            &ordered,
                            &start_num,
                            &item_content,
                            &marker))
    return 0;
  (void)qpadding;
  list_ordered = ordered;
  list.type =
      list_ordered ? FIO_MD_BLOCK_LIST_ORDERED : FIO_MD_BLOCK_LIST_UNORDERED;
  list.list_start = start_num;
  list.marker = marker;
  list.flags = FIO_MD_BLOCK_F_TIGHT;
  if (fio___md_block_enter(s, &list))
    return -1;
  while (p < end) {
    fio_md_block_s item = {0};
    fio_md_block_s para = {0};
    uint32_t task_flags;
    if (!fio___md_blockquote_line(p, end, &qcontent, &qpadding, &qe, &next))
      break;
    if (fio___md_is_blank(qcontent, qe))
      break;
    if (!fio___md_list_marker(qcontent,
                              qe,
                              &ordered,
                              &start_num,
                              &item_content,
                              &marker) ||
        ordered != list_ordered)
      break;
    task_flags = fio___md_task_flags(&item_content, qe);
    item.type = FIO_MD_BLOCK_LIST_ITEM;
    item.source = fio___md_buf(qcontent, qe);
    item.marker = marker;
    item.list_start = start_num;
    item.flags = task_flags | FIO_MD_BLOCK_F_TIGHT;
    if (fio___md_block_enter(s, &item))
      return -1;
    if (item_content < qe) {
      para.type = FIO_MD_BLOCK_PARAGRAPH;
      para.source = fio___md_buf(item_content, qe);
      if (fio___md_block_enter(s, &para) ||
          fio___md_parse_inline_span(s,
                                     item_content,
                                     fio___md_rtrim(item_content, qe)) ||
          fio___md_block_leave(s, &para))
        return -1;
    }
    if (fio___md_block_leave(s, &item))
      return -1;
    p = next;
  }
  list.source = fio___md_buf(list_start, p);
  if (fio___md_block_leave(s, &list))
    return -1;
  *after = p;
  return 1;
}

FIO_SFUNC int fio___md_parse_blockquote(fio___md_s *s,
                                        char *p,
                                        char *end,
                                        char **after) {
  char *bq_start = p;
  fio_md_block_s b = {0};
  fio_md_block_s para = {0};
  char *para_start = NULL;
  char *para_end = NULL;
  char *break_start = NULL;
  char *break_end = NULL;
  int para_open = 0;
  int hard = 0;
  b.type = FIO_MD_BLOCK_BLOCK_QUOTE;
  if (fio___md_block_enter(s, &b))
    return -1;
  while (p < end) {
    char *qcontent;
    char *qe;
    char *next;
    char *ts;
    char *te;
    uint8_t qpadding = 0;
    if (!fio___md_blockquote_line(p, end, &qcontent, &qpadding, &qe, &next)) {
      qe = fio___md_line_end(p, end);
      next = fio___md_line_next(qe, end);
      if (!para_open || fio___md_is_blank(p, qe) ||
          fio___md_block_start_line(p, qe))
        break;
      qcontent = p;
      qpadding = 0;
    }
    if (fio___md_is_blank(qcontent, qe)) {
      if (para_open &&
          fio___md_blockquote_para_close(s, &para, para_start, para_end))
        return -1;
      para_open = 0;
      p = next;
      continue;
    }
    if (!para_open) {
      int list_result = fio___md_parse_blockquote_list(s, p, end, &p);
      if (list_result < 0)
        return -1;
      if (list_result)
        continue;
    }
    {
      fio_buf_info_s fence_marker = FIO_BUF_INFO0;
      fio_buf_info_s fence_info = FIO_BUF_INFO0;
      if (!para_open &&
          fio___md_fence(qcontent, qe, &fence_marker, &fence_info)) {
        char *code_start = p;
        char *body_start = next;
        char *close_line = next;
        char *after_code = next;
        fio_md_block_s code = {0};
        while (after_code < end) {
          char *line_content;
          char *line_end;
          char *line_next;
          uint8_t line_padding = 0;
          if (!fio___md_blockquote_line(after_code,
                                        end,
                                        &line_content,
                                        &line_padding,
                                        &line_end,
                                        &line_next))
            break;
          if (fio___md_fence_close(line_content, line_end, fence_marker)) {
            close_line = after_code;
            after_code = line_next;
            break;
          }
          close_line = line_next;
          after_code = line_next;
        }
        code.type = FIO_MD_BLOCK_CODE_FENCED;
        code.source = fio___md_buf(code_start, after_code);
        code.content = fio___md_buf(body_start, close_line);
        code.marker = fence_marker;
        code.info = fence_info;
        code.padding = 1;
        if (fio___md_block_enter(s, &code) || fio___md_block_leave(s, &code))
          return -1;
        p = after_code;
        continue;
      }
    }
    if (!para_open && fio___md_html_block_start(qcontent, qe)) {
      const int html_long = fio___md_html_long_block(qcontent, qe);
      for (;;) {
        const int html_done =
            html_long ? fio___md_html_long_close(qcontent, qe) : (next >= end);
        if (fio___md_emit_html_block(s, qcontent, next))
          return -1;
        p = next;
        if (html_done || p >= end)
          break;
        if (!fio___md_blockquote_line(p, end, &qcontent, &qpadding, &qe, &next))
          break;
        if (!html_long && fio___md_is_blank(qcontent, qe))
          break;
      }
      continue;
    }
    if (!para_open && fio___md_block_start_line(qcontent, qe)) {
      if (fio___md_parse_blocks_ex(s, qcontent, next, qpadding))
        return -1;
      p = next;
      continue;
    }
    ts = fio___md_ltrim(qcontent, qe);
    te = fio___md_rtrim(ts, qe);
    if (!para_open) {
      FIO_MEMSET(&para, 0, sizeof(para));
      para.type = FIO_MD_BLOCK_PARAGRAPH;
      para.source = fio___md_buf(qcontent, qe);
      if (fio___md_block_enter(s, &para))
        return -1;
      para_open = 1;
      para_start = qcontent;
    } else if (fio___md_inline_break(s, hard, break_start, break_end)) {
      return -1;
    }
    if (fio___md_parse_inline_span(s, ts, te))
      return -1;
    hard = 0;
    if (qe > qcontent && qe[-1] == '\\')
      hard = 1;
    if (qe > qcontent + 1 && qe[-1] == ' ' && qe[-2] == ' ')
      hard = 1;
    para_end = qe;
    break_start = qe;
    break_end = next;
    p = next;
  }
  if (para_open &&
      fio___md_blockquote_para_close(s, &para, para_start, para_end))
    return -1;
  b.source = fio___md_buf(bq_start, p);
  if (fio___md_block_leave(s, &b))
    return -1;
  *after = p;
  return 1;
}

FIO_SFUNC int fio___md_parse_blocks_ex(fio___md_s *s,
                                       char *p,
                                       char *end,
                                       uint8_t initial_padding) {
  while (p < end && !s->err) {
    uint8_t line_padding = initial_padding;
    initial_padding = 0;
    char *le = fio___md_line_end(p, end);
    char *next = fio___md_line_next(le, end);
    char *trim = fio___md_ltrim(p, le);
    uint8_t level = 0;
    char *content_start = NULL;
    char *content_end = NULL;
    fio_buf_info_s marker = FIO_BUF_INFO0;
    fio_buf_info_s info = FIO_BUF_INFO0;
    if (fio___md_is_blank(p, le)) {
      p = next;
      continue;
    }
    {
      fio_buf_info_s ref_label = FIO_BUF_INFO0;
      fio_buf_info_s ref_dst = FIO_BUF_INFO0;
      fio_buf_info_s ref_title = FIO_BUF_INFO0;
      if (fio___md_footnote_line(p, le, NULL, NULL)) {
        p = fio___md_footnote_end(p, end);
        continue;
      }
      char *ref_after = NULL;
      if (fio___md_ref_def(p,
                           end,
                           &ref_label,
                           &ref_dst,
                           &ref_title,
                           &ref_after)) {
        p = ref_after;
        continue;
      }
    }
    if (trim < le && *trim == '>' &&
        (uint32_t)(trim - p) <= FIO___MD_MAX_MARKER_INDENT) {
      int rr = fio___md_parse_blockquote(s, p, end, &p);
      if (rr < 0)
        return -1;
      continue;
    }
    if (fio___md_atx(p, le, &level, &content_start, &content_end, &marker)) {
      fio_md_block_s b = {0};
      b.type = FIO_MD_BLOCK_HEADING;
      b.source = fio___md_buf(p, le);
      b.content = fio___md_buf(content_start, content_end);
      b.marker = marker;
      b.heading_level = level;
      if (fio___md_block_enter(s, &b) ||
          fio___md_parse_inline_span(s, content_start, content_end) ||
          fio___md_block_leave(s, &b))
        return -1;
      p = next;
      continue;
    }
    if (fio___md_thematic(p, le)) {
      fio_md_block_s b = {0};
      b.type = FIO_MD_BLOCK_THEMATIC_BREAK;
      b.source = fio___md_buf(p, le);
      b.marker = fio___md_buf(trim, fio___md_rtrim(trim, le));
      if (fio___md_block_enter(s, &b) || fio___md_block_leave(s, &b))
        return -1;
      p = next;
      continue;
    }
    if (fio___md_fence(p, le, &marker, &info)) {
      char *body_start = next;
      char *close_line = end;
      char *q = next;
      fio_md_block_s b = {0};
      while (q < end) {
        char *qe = fio___md_line_end(q, end);
        if (fio___md_fence_close(q, qe, marker)) {
          close_line = q;
          q = fio___md_line_next(qe, end);
          break;
        }
        q = fio___md_line_next(qe, end);
      }
      b.type = FIO_MD_BLOCK_CODE_FENCED;
      b.source = fio___md_buf(p, q);
      b.content = fio___md_buf(body_start, close_line);
      b.marker = marker;
      b.info = info;
      if (fio___md_block_enter(s, &b) || fio___md_block_leave(s, &b))
        return -1;
      p = q;
      continue;
    }
    if ((uint32_t)line_padding + fio___md_indent(p, le) >= FIO___MD_TAB_WIDTH) {
      char *code_start = p;
      char *code_content_end = next;
      char *scan = p;
      while (next < end) {
        char *ne = fio___md_line_end(next, end);
        if (!fio___md_is_blank(next, ne) &&
            fio___md_indent(next, ne) < FIO___MD_TAB_WIDTH)
          break;
        next = fio___md_line_next(ne, end);
      }
      while (scan < next) {
        char *se = fio___md_line_end(scan, end);
        char *sn = fio___md_line_next(se, end);
        char *code = scan;
        uint32_t spaces = 0;
        while (code < se && spaces < FIO___MD_TAB_WIDTH) {
          if (*code == ' ') {
            ++code;
            ++spaces;
          } else if (*code == '\t') {
            ++code;
            break;
          } else {
            break;
          }
        }
        if (!fio___md_is_blank(code, se))
          code_content_end = sn;
        scan = sn;
      }
      {
        fio_md_block_s b = {0};
        b.type = FIO_MD_BLOCK_CODE_INDENTED;
        b.source = fio___md_buf(code_start, next);
        b.content = fio___md_buf(code_start, code_content_end);
        b.padding = line_padding;
        if (fio___md_block_enter(s, &b) || fio___md_block_leave(s, &b))
          return -1;
      }
      p = next;
      continue;
    }
    if (fio___md_html_block_start(p, le)) {
      char *html_start = p;
      if (fio___md_html_long_block(p, le)) {
        if (!fio___md_html_long_close(p, le)) {
          while (next < end) {
            char *line = next;
            char *ne = fio___md_line_end(line, end);
            next = fio___md_line_next(ne, end);
            if (fio___md_html_long_close(line, ne))
              break;
          }
        }
      } else {
        while (next < end) {
          char *ne = fio___md_line_end(next, end);
          if (fio___md_is_blank(next, ne))
            break;
          next = fio___md_line_next(ne, end);
        }
      }
      {
        fio_md_block_s b = {0};
        b.type = FIO_MD_BLOCK_HTML;
        b.source = fio___md_buf(html_start, next);
        b.content = b.source;
        if (fio___md_block_enter(s, &b) || fio___md_block_leave(s, &b))
          return -1;
      }
      p = next;
      continue;
    }
    {
      int ordered = 0;
      uint64_t start_num = 0;
      char *unused_content = NULL;
      int lr = fio___md_list_marker(p,
                                    le,
                                    &ordered,
                                    &start_num,
                                    &unused_content,
                                    &marker);
      if (lr) {
        int rr = fio___md_parse_list(s, p, end);
        if (rr < 0)
          return -1;
        p = s->start + s->consumed;
        continue;
      }
    }
    {
      char *nle = (next < end) ? fio___md_line_end(next, end) : next;
      uint8_t aligns[FIO_MARKDOWN_MAX_TABLE_COLUMNS];
      uint16_t columns = 0;
      if (next < end && fio___md_count_table_cells(p, le) > 1 &&
          fio___md_table_delim(next, nle, aligns, &columns)) {
        char *table_start = p;
        char *row = fio___md_line_next(nle, end);
        fio_md_block_s b = {0};
        b.type = FIO_MD_BLOCK_TABLE;
        b.columns = columns;
        while (row < end) {
          char *re = fio___md_line_end(row, end);
          if (fio___md_is_blank(row, re) ||
              fio___md_count_table_cells(row, re) < 2)
            break;
          row = fio___md_line_next(re, end);
        }
        b.source = fio___md_buf(table_start, row);
        if (fio___md_block_enter(s, &b))
          return -1;
        if (fio___md_parse_table_row(s, p, le, aligns, columns))
          return -1;
        p = fio___md_line_next(nle, end);
        while (p < row) {
          char *re = fio___md_line_end(p, end);
          if (fio___md_parse_table_row(s, p, re, aligns, columns))
            return -1;
          p = fio___md_line_next(re, end);
        }
        if (fio___md_block_leave(s, &b))
          return -1;
        continue;
      }
    }
    {
      char *para_start = p;
      char *para_end = le;
      char *q = next;
      uint8_t setext_level = 0;
      while (q < end) {
        char *qe = fio___md_line_end(q, end);
        char *qt = fio___md_ltrim(q, qe);
        fio_buf_info_s dummy_a = FIO_BUF_INFO0;
        fio_buf_info_s dummy_b = FIO_BUF_INFO0;
        if (fio___md_setext(q, qe, &setext_level)) {
          fio_md_block_s b = {0};
          b.type = FIO_MD_BLOCK_HEADING;
          b.source = fio___md_buf(para_start, qe);
          b.content = fio___md_buf(fio___md_ltrim(para_start, le),
                                   fio___md_rtrim(para_start, para_end));
          b.marker = fio___md_buf(fio___md_ltrim(q, qe), fio___md_rtrim(q, qe));
          b.heading_level = setext_level;
          if (fio___md_block_enter(s, &b) ||
              fio___md_inline_lines(s,
                                    b.content.buf,
                                    b.content.buf + b.content.len) ||
              fio___md_block_leave(s, &b))
            return -1;
          p = fio___md_line_next(qe, end);
          goto fio___md_blocks_next;
        }
        if (fio___md_is_blank(q, qe) ||
            fio___md_atx(q,
                         qe,
                         &level,
                         &content_start,
                         &content_end,
                         &marker) ||
            fio___md_thematic(q, qe) ||
            fio___md_fence(q, qe, &dummy_a, &dummy_b) ||
            fio___md_html_long_block(q, qe) ||
            (qt + 1 < qe && *qt == '<' &&
             (qt[1] == '!' || qt[1] == '?' ||
              fio___md_html_block_tag(qt + 1, qe))) ||
            (qt < qe && *qt == '>' &&
             (uint32_t)(qt - q) <= FIO___MD_MAX_MARKER_INDENT))
          break;
        para_end = qe;
        q = fio___md_line_next(qe, end);
      }
      if (fio___md_parse_paragraph(s, para_start, para_end))
        return -1;
      p = fio___md_line_next(para_end, end);
    }
  fio___md_blocks_next:;
  }
  s->consumed = (size_t)(p - s->start);
  return 0;
}

FIO_SFUNC int fio___md_parse_blocks(fio___md_s *s, char *p, char *end) {
  return fio___md_parse_blocks_ex(s, p, end, 0);
}

SFUNC size_t fio_md_parse(const fio_md_callbacks_s *callbacks,
                          void *udata,
                          fio_buf_info_s source) {
  fio___md_s s;
  fio_md_block_s doc = {0};
  FIO_MEMSET(&s, 0, sizeof(s));
  s.cb = callbacks;
  s.udata = udata;
  s.start = source.buf;
  s.end = source.buf ? source.buf + source.len : NULL;
  doc.type = FIO_MD_BLOCK_DOCUMENT;
  doc.source = source;
  if (!source.buf && source.len) {
    fio___md_abort(&s, FIO_MD_ERR_INPUT, source.buf);
    return s.consumed;
  }
  if (source.buf)
    fio___md_ref_index(&s);
  if (fio___md_doc_start(&s, source))
    return s.consumed;
  if (fio___md_block_enter(&s, &doc))
    return s.consumed;
  if (source.buf &&
      fio___md_parse_blocks(&s, source.buf, source.buf + source.len))
    return s.consumed;
  s.consumed = source.len;
  if (fio___md_block_leave(&s, &doc))
    return s.consumed;
  if (fio___md_doc_end(&s, source))
    return s.consumed;
  return source.len;
}

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_MARKDOWN */
