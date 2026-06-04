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
/** Maximum nesting depth (container + inline). */
#define FIO_MARKDOWN_MAX_DEPTH 255
#endif

#ifndef FIO_MARKDOWN_MAX_TABLE_COLUMNS
/** Bytes for table alignment cache (2 bits/column = 4 columns/byte). */
#define FIO_MARKDOWN_MAX_TABLE_COLUMNS 64
#endif
FIO_ASSERT_STATIC(FIO_MARKDOWN_MAX_TABLE_COLUMNS > 0,
                  "FIO_MARKDOWN_MAX_TABLE_COLUMNS too small");

#ifndef FIO_MARKDOWN_REF_CACHE_SIZE
/** Maximum reference definitions cached during pre-scan. */
#define FIO_MARKDOWN_REF_CACHE_SIZE 128
#endif
FIO_ASSERT_STATIC(FIO_MARKDOWN_REF_CACHE_SIZE > 0,
                  "FIO_MARKDOWN_REF_CACHE_SIZE too small");

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
#define FIO_MD_F_TIGHT ((uint32_t)1U << 0)
/** Block flag: list item has a GFM task-list marker. */
#define FIO_MD_F_TASK ((uint32_t)1U << 1)
/** Block flag: GFM task-list marker is checked. Requires FIO_MD_F_TASK. */
#define FIO_MD_F_TASK_CHECKED ((uint32_t)1U << 2)

/** Markdown node type (block, inline section, or text). */
typedef enum {
  /* === Block sections === */
  FIO_MD_PARAGRAPH = 0,
  FIO_MD_HEADING,
  FIO_MD_THEMATIC_BREAK,
  FIO_MD_BLOCKQUOTE,
  FIO_MD_LIST_UNORDERED,
  FIO_MD_LIST_ORDERED,
  FIO_MD_LIST_ITEM,
  FIO_MD_CODE_INDENTED,
  FIO_MD_CODE_FENCED,
  FIO_MD_HTML_BLOCK,
  FIO_MD_TABLE,
  FIO_MD_TABLE_ROW,
  FIO_MD_TABLE_CELL,

  /* === Inline sections === */
  FIO_MD_EMPHASIS_STAR,
  FIO_MD_EMPHASIS_UNDERSCORE,
  FIO_MD_STRONG_STAR,
  FIO_MD_STRONG_UNDERSCORE,
  FIO_MD_STRIKETHROUGH,
  FIO_MD_LINK,

  /* === Text / leaf content === */
  FIO_MD_TEXT,
  FIO_MD_SOFT_BREAK,
  FIO_MD_HARD_BREAK,
  FIO_MD_CODE_SPAN,
  FIO_MD_IMAGE,
  FIO_MD_AUTOLINK,
  FIO_MD_INLINE_HTML,
  FIO_MD_ESCAPE,
  FIO_MD_ENTITY,
  FIO_MD_FOOTNOTE_REF,
} fio_md_type_e;

/** GFM table cell alignment. */
typedef enum {
  FIO_MD_ALIGN_NONE = 0,
  FIO_MD_ALIGN_LEFT,
  FIO_MD_ALIGN_RIGHT,
  FIO_MD_ALIGN_CENTER,
} fio_md_align_e;

/** Markdown event. Passed by pointer to all callbacks. */
typedef struct {
  void *udata;                /* live: parser copies back after callback */
  fio_buf_info_s source;      /* full source slice for this event */
  fio_buf_info_s text;        /* text / label / code content */
  fio_buf_info_s marker;      /* marker slice (`#`, fence, list marker) */
  fio_buf_info_s info;        /* fenced code info string */
  fio_buf_info_s destination; /* link/image/autolink URL */
  fio_buf_info_s title;       /* link/image title */
  fio_buf_info_s reference;   /* reference label for ref-style links/images */
  uint32_t list_start;        /* ordered list start number */
  uint16_t columns;           /* table column count */
  uint16_t column;            /* table cell column index */
  uint8_t type;               /* fio_md_type_e */
  uint8_t heading_level;      /* 1..6 for headings */
  uint8_t flags;              /* FIO_MD_F_* */
  uint8_t align;              /* FIO_MD_ALIGN_* for table cells */
  uint8_t padding;            /* virtual leading spaces from tab expansion */
  int err;                    /* error code (error callback only) */
  size_t consumed;            /* bytes consumed (error callback only) */
} fio_md_event_s;

/** Markdown parser callbacks. All callbacks are optional. */
typedef struct {
  int (*push)(fio_md_event_s *e);   /* section start */
  int (*text)(fio_md_event_s *e);   /* text / leaf content */
  int (*pop)(fio_md_event_s *e);    /* section end */
  void (*error)(fio_md_event_s *e); /* parse error */
} fio_md_callbacks_s;

/**
 * Parses a complete Markdown / GFM document and emits callback events.
 *
 * The parser is non-streaming: `source` must contain the complete document.
 * Events are zero-copy slices into `source` wherever Markdown structure
 * permits. The parser builds no AST, performs no rendering, and performs no
 * heap allocation.
 *
 * A non-zero callback return aborts parsing and is forwarded as `err` to the
 * optional `error` callback. Parser-generated errors are negative; `-1` is
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
  /* --- hot flags --- */
  uint8_t in_footnote;
  uint8_t tight_list_item_depth;
  uint8_t paragraph_suppressed_depth;
  uint8_t rendering_footnotes;
  uint8_t _pad[4];

  /* --- iteration state --- */
  const fio_md_callbacks_s *cb;
  void *udata;
  char *start;
  char *end;
  char *p;
  size_t consumed;
  int err;

  /* --- 256-byte nesting array --- */
  uint8_t nest[FIO_MARKDOWN_MAX_DEPTH + 1];

  /* --- reference cache --- */
  fio___md_ref_s refs[FIO_MARKDOWN_REF_CACHE_SIZE];
  fio___md_ref_s ref_overflow_slot;
  char *ref_scanned_to;
  uint16_t ref_count;
  uint8_t ref_overflow;

  /* --- table alignment bit-packed cache --- */
  uint8_t table_align[(FIO_MARKDOWN_MAX_TABLE_COLUMNS + 3) >> 2];
} fio___md_parser_s;

/* --- Nesting array macros --- */
#define FIO___MD_NEST_DEPTH(st)   ((st)->nest[0])
#define FIO___MD_NEST_TYPE(st, d) ((st)->nest[(d)])
#define FIO___MD_NEST_PUSH(st, t)                                              \
  do {                                                                         \
    if ((st)->nest[0] >= FIO_MARKDOWN_MAX_DEPTH) {                             \
      (st)->err = FIO_MD_ERR_DEPTH;                                            \
      break;                                                                   \
    }                                                                          \
    (st)->nest[++((st)->nest[0])] = (t);                                       \
  } while (0)
#define FIO___MD_NEST_POP(st) (--(st)->nest[0])
#define FIO___MD_NEST_TOP(st) ((st)->nest[(st)->nest[0]])

/* --- Table alignment bit-packed macros --- */
#define FIO___MD_TABLE_ALIGN_GET(st, col)                                      \
  (((st)->table_align[(col) >> 2] >> (((col)&3) << 1)) & 3)
#define FIO___MD_TABLE_ALIGN_SET(st, col, val)                                 \
  do {                                                                         \
    uint8_t _shift = ((col)&3) << 1;                                           \
    (st)->table_align[(col) >> 2] &= ~(3 << _shift);                           \
    (st)->table_align[(col) >> 2] |= ((val)&3) << _shift;                      \
  } while (0)

/* --- Event helpers --- */
FIO_IFUNC void fio___md_event_init(fio___md_parser_s *st, fio_md_event_s *e) {
  FIO_MEMSET(e, 0, sizeof(*e));
  e->udata = st->udata;
}

FIO_IFUNC int fio___md_push(fio___md_parser_s *st, fio_md_event_s *e) {
  int r;
  if (!st->cb || !st->cb->push)
    return 0;
  e->udata = st->udata;
  r = st->cb->push(e);
  st->udata = e->udata;
  return r;
}

FIO_IFUNC int fio___md_text(fio___md_parser_s *st, fio_md_event_s *e) {
  int r;
  if (!st->cb || !st->cb->text)
    return 0;
  e->udata = st->udata;
  r = st->cb->text(e);
  st->udata = e->udata;
  return r;
}

FIO_IFUNC int fio___md_pop(fio___md_parser_s *st, fio_md_event_s *e) {
  int r;
  if (!st->cb || !st->cb->pop)
    return 0;
  e->udata = st->udata;
  r = st->cb->pop(e);
  st->udata = e->udata;
  return r;
}

FIO_IFUNC void fio___md_error(fio___md_parser_s *st, fio_md_event_s *e) {
  if (!st->cb || !st->cb->error)
    return;
  e->udata = st->udata;
  st->cb->error(e);
  st->udata = e->udata;
}

FIO_SFUNC int fio___md_abort(fio___md_parser_s *st,
                             int err,
                             char *pos,
                             fio_md_event_s *e) {
  if (!err)
    err = FIO_MD_ERR_GENERIC;
  st->err = err;
  if (!st->start) {
    st->consumed = 0;
  } else {
    if (pos < st->start)
      pos = st->start;
    if (pos > st->end)
      pos = st->end;
    st->consumed = (size_t)(pos - st->start);
  }
  if (st->cb && st->cb->error) {
    fio___md_event_init(st, e);
    e->err = err;
    e->consumed = st->consumed;
    fio___md_error(st, e);
  }
  return -1;
}

/* --- Buffer helpers --- */
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

/* --- Label / reference helpers --- */
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
  char ca, cb;
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
  char q, close, *title_start;
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

/* Find the end of a reference definition label (may span multiple lines).
   Returns pointer to the closing ']' or NULL if not found before end.
   Sets *label_end to the position after ']' and *label_line_end to the
   line end where ']' was found. */
FIO_IFUNC char *fio___md_ref_label_end(char *p,
                                       char *end,
                                       char **label_line_end) {
  while (p < end) {
    if (*p == '\\' && p + 1 < end) {
      p += 2;
      continue;
    }
    if (*p == ']') {
      if (label_line_end)
        *label_line_end = fio___md_line_end(p, end);
      return p;
    }
    if (*p == '\n' || *p == '\r') {
      /* label may span lines, skip line ending */
      if (*p == '\r')
        ++p;
      if (p < end && *p == '\n')
        ++p;
      continue;
    }
    ++p;
  }
  return NULL;
}

FIO_SFUNC int fio___md_ref_def(char *ls,
                               char *end,
                               fio_buf_info_s *label,
                               fio_buf_info_s *dst,
                               fio_buf_info_s *title,
                               char **after) {
  char *le = fio___md_line_end(ls, end);
  char *p = fio___md_ltrim(ls, le);
  char *label_start, *label_end, *label_le;
  char *dst_start, *dst_end, *dst_line_end;
  char *ref_after;
  uint8_t angled_dst = 0, has_space_after_dst = 0;
  if ((uint32_t)(p - ls) > FIO___MD_MAX_MARKER_INDENT || p == le || *p != '[')
    return 0;
  label_start = ++p;
  label_end = fio___md_ref_label_end(p, end, &label_le);
  if (!label_end || label_end == label_start || label_end + 1 >= end ||
      label_end[1] != ':')
    return 0;
  if (*label_start == '^')
    return 0;
  /* After ]: must have destination on same line or next */
  p = fio___md_ltrim(label_end + 2, label_le);
  le = label_le;
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
        fio_buf_info_s try_title = FIO_BUF_INFO0;
        char *try_after = ref_after;
        if (fio___md_ref_title(title_line,
                               fio___md_rtrim(title_line, tle),
                               end,
                               &try_title,
                               &try_after)) {
          *title = try_title;
          ref_after = try_after;
        }
        /* If title parsing fails, the ref def is still valid with no title.
           The next line is not consumed. */
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
  char *label_start, *label_end;
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

FIO_IFUNC void fio___md_ref_cache_add(fio___md_parser_s *st,
                                       fio_buf_info_s label,
                                       fio_buf_info_s dst,
                                       fio_buf_info_s title) {
  uint16_t i = 0;
  label.buf = fio___md_ltrim(label.buf, label.buf + label.len);
  label.len =
      (size_t)(fio___md_rtrim(label.buf, label.buf + label.len) - label.buf);
  while (i < st->ref_count &&
         !fio___md_slice_eq_lc(label, st->refs[i].label))
    ++i;
  if (i == st->ref_count) {
    if (st->ref_count < FIO_MARKDOWN_REF_CACHE_SIZE) {
      st->refs[st->ref_count].label = label;
      st->refs[st->ref_count].destination = dst;
      st->refs[st->ref_count].title = title;
      ++st->ref_count;
    } else {
      st->ref_overflow = 1;
    }
  }
}

/* Forward declarations for block helpers used in ref_index */
FIO_SFUNC int fio___md_atx(char *ls,
                           char *le,
                           uint8_t *level,
                           char **content_start,
                           char **content_end,
                           fio_buf_info_s *marker);
FIO_SFUNC int fio___md_setext(char *ls, char *le, uint8_t *level);
FIO_SFUNC int fio___md_thematic(char *ls, char *le);
FIO_SFUNC int fio___md_fence(char *ls,
                             char *le,
                             fio_buf_info_s *marker,
                             fio_buf_info_s *info);
FIO_SFUNC int fio___md_fence_close(char *ls, char *le, fio_buf_info_s marker);
FIO_SFUNC int fio___md_html_block_start(char *ls, char *le);
FIO_SFUNC int fio___md_html_long_block(char *ls, char *le);
FIO_SFUNC int fio___md_html_long_close(char *ls, char *le);
FIO_SFUNC int fio___md_list_marker(char *ls,
                                   char *le,
                                   int *ordered,
                                   uint64_t *start,
                                   char **content,
                                   fio_buf_info_s *marker);

FIO_SFUNC int fio___md_line_starts_block(char *ls, char *le) {
  uint8_t level = 0;
  char *cs = NULL, *ce = NULL;
  fio_buf_info_s m = FIO_BUF_INFO0, info = FIO_BUF_INFO0;
  int ordered = 0;
  uint64_t sn = 0;
  char *content = NULL;
  char *trim = fio___md_ltrim(ls, le);
  return fio___md_atx(ls, le, &level, &cs, &ce, &m) ||
         fio___md_setext(ls, le, &level) ||
         fio___md_thematic(ls, le) ||
         fio___md_fence(ls, le, &m, &info) ||
         fio___md_html_block_start(ls, le) ||
         fio___md_list_marker(ls, le, &ordered, &sn, &content, &m) ||
         (trim < le && *trim == '>' &&
          (uint32_t)(trim - ls) <= FIO___MD_MAX_MARKER_INDENT);
}

/* Strip blockquote markers from a line, returning the content after them.
   Also returns the blockquote depth. */
FIO_IFUNC char *fio___md_strip_bq(char *p, char *le, int *bq_depth) {
  char *content = p;
  *bq_depth = 0;
  while (1) {
    char *trim = fio___md_ltrim(content, le);
    if (trim < le && *trim == '>') {
      content = trim + 1;
      if (content < le && *content == ' ')
        ++content;
      else if (content < le && *content == '\t')
        ++content;
      ++*bq_depth;
    } else {
      break;
    }
  }
  return content;
}

/* Lazy reference scanner: scans a range for ref definitions and caches them.
   Returns pointer to where scanning stopped (end or overflow point). */
FIO_SFUNC char *fio___md_ref_scan(fio___md_parser_s *st,
                                  char *p,
                                  char *scan_end) {
  int may_start_ref_def = 1;

  while (p < scan_end) {
    char *le = fio___md_line_end(p, scan_end);
    char *next = fio___md_line_next(le, scan_end);
    char *after = NULL;
    fio_buf_info_s label = FIO_BUF_INFO0;
    fio_buf_info_s dst = FIO_BUF_INFO0;
    fio_buf_info_s title = FIO_BUF_INFO0;
    fio_buf_info_s fence_marker = FIO_BUF_INFO0;
    fio_buf_info_s fence_info = FIO_BUF_INFO0;
    int bq_depth = 0;
    char *content = fio___md_strip_bq(p, le, &bq_depth);

    if (fio___md_is_blank(p, le)) {
      may_start_ref_def = 1;
      p = next;
      continue;
    }

    /* Skip fenced code blocks */
    if (fio___md_fence(p, le, &fence_marker, &fence_info)) {
      char *q = next;
      while (q < scan_end) {
        char *qe = fio___md_line_end(q, scan_end);
        if (fio___md_fence_close(q, qe, fence_marker)) {
          p = fio___md_line_next(qe, scan_end);
          break;
        }
        q = fio___md_line_next(qe, scan_end);
      }
      if (q >= scan_end)
        break;
      may_start_ref_def = 1;
      continue;
    }
    /* Skip indented code blocks */
    if (fio___md_indent(p, le) >= FIO___MD_TAB_WIDTH) {
      while (next < scan_end) {
        char *ne = fio___md_line_end(next, scan_end);
        if (!fio___md_is_blank(next, ne) &&
            fio___md_indent(next, ne) < FIO___MD_TAB_WIDTH)
          break;
        next = fio___md_line_next(ne, scan_end);
      }
      p = next;
      may_start_ref_def = 1;
      continue;
    }
    /* Skip HTML blocks */
    if (fio___md_html_block_start(p, le)) {
      if (fio___md_html_long_block(p, le)) {
        if (!fio___md_html_long_close(p, le)) {
          while (next < scan_end) {
            char *line = next;
            char *ne = fio___md_line_end(line, scan_end);
            next = fio___md_line_next(ne, scan_end);
            if (fio___md_html_long_close(line, ne))
              break;
          }
        }
      } else {
        while (next < scan_end) {
          char *ne = fio___md_line_end(next, scan_end);
          if (fio___md_is_blank(next, ne))
            break;
          next = fio___md_line_next(ne, scan_end);
        }
      }
      p = next;
      may_start_ref_def = 1;
      continue;
    }

    /* Check for ref def on stripped content */
    if (may_start_ref_def &&
        fio___md_ref_def(content, scan_end, &label, &dst, &title, &after)) {
      fio___md_ref_cache_add(st, label, dst, title);
      if (st->ref_overflow) {
        return after ? after : next;
      }
      may_start_ref_def = 1;
      p = after ? after : next;
      continue;
    }

    /* Update paragraph state for next line */
    may_start_ref_def = fio___md_line_starts_block(content, le);
    p = next;
  }
  return scan_end;
}

FIO_SFUNC fio___md_ref_s *fio___md_ref_find(fio___md_parser_s *st,
                                            fio_buf_info_s label) {
  uint16_t i;
  label.buf = fio___md_ltrim(label.buf, label.buf + label.len);
  label.len =
      (size_t)(fio___md_rtrim(label.buf, label.buf + label.len) - label.buf);
  for (i = 0; i < st->ref_count; ++i) {
    if (fio___md_slice_eq_lc(label, st->refs[i].label))
      return &st->refs[i];
  }

  /* First miss: perform lazy full-document scan */
  if (!st->ref_scanned_to) {
    st->ref_scanned_to = fio___md_ref_scan(st, st->start, st->end);
    for (i = 0; i < st->ref_count; ++i) {
      if (fio___md_slice_eq_lc(label, st->refs[i].label))
        return &st->refs[i];
    }
  }

  /* Overflow: scan from last position, looking for this specific label */
  if (st->ref_overflow && st->ref_scanned_to < st->end) {
    char *p = st->ref_scanned_to;
    while (p < st->end) {
      char *le = fio___md_line_end(p, st->end);
      char *next = fio___md_line_next(le, st->end);
      char *after = NULL;
      char *content = p;
      fio_buf_info_s ref_label = FIO_BUF_INFO0;
      fio_buf_info_s dst = FIO_BUF_INFO0;
      fio_buf_info_s title = FIO_BUF_INFO0;
      int bq_depth = 0;

      if (fio___md_is_blank(p, le)) {
        p = next;
        continue;
      }

      content = fio___md_strip_bq(p, le, &bq_depth);

      if (fio___md_ref_def(content, st->end, &ref_label, &dst, &title,
                           &after)) {
        if (fio___md_slice_eq_lc(label, ref_label)) {
          st->ref_overflow_slot.label = ref_label;
          st->ref_overflow_slot.destination = dst;
          st->ref_overflow_slot.title = title;
          return &st->ref_overflow_slot;
        }
        p = after ? after : next;
        continue;
      }
      p = next;
    }
    st->ref_scanned_to = st->end;
  }
  return NULL;
}

FIO_SFUNC void fio___md_ref_index(fio___md_parser_s *st) {
  char *p = st->start;
  int may_start_ref_def = 1;   /* top level: can a ref def start here? */
  int bq_may_start_ref_def = 0; /* inside blockquote: can a ref def start? */
  int prev_bq_depth = 0;

  while (p < st->end) {
    char *le = fio___md_line_end(p, st->end);
    char *next = fio___md_line_next(le, st->end);
    char *after = NULL;
    fio_buf_info_s label = FIO_BUF_INFO0;
    fio_buf_info_s dst = FIO_BUF_INFO0;
    fio_buf_info_s title = FIO_BUF_INFO0;
    fio_buf_info_s fence_marker = FIO_BUF_INFO0;
    fio_buf_info_s fence_info = FIO_BUF_INFO0;
    int bq_depth = 0;
    char *content = fio___md_strip_bq(p, le, &bq_depth);

    if (fio___md_is_blank(p, le)) {
      may_start_ref_def = 1;
      bq_may_start_ref_def = 1;
      prev_bq_depth = 0;
      p = next;
      continue;
    }

    /* Reset bq paragraph state when bq depth changes */
    if (bq_depth != prev_bq_depth) {
      bq_may_start_ref_def = 1;
      prev_bq_depth = bq_depth;
    }

    /* Skip fenced code blocks during pre-scan */
    if (fio___md_fence(p, le, &fence_marker, &fence_info)) {
      char *q = next;
      while (q < st->end) {
        char *qe = fio___md_line_end(q, st->end);
        if (fio___md_fence_close(q, qe, fence_marker)) {
          p = fio___md_line_next(qe, st->end);
          break;
        }
        q = fio___md_line_next(qe, st->end);
      }
      if (q >= st->end)
        break;
      may_start_ref_def = 1;
      bq_may_start_ref_def = 1;
      continue;
    }
    /* Skip indented code blocks during pre-scan */
    if (fio___md_indent(p, le) >= FIO___MD_TAB_WIDTH) {
      while (next < st->end) {
        char *ne = fio___md_line_end(next, st->end);
        if (!fio___md_is_blank(next, ne) &&
            fio___md_indent(next, ne) < FIO___MD_TAB_WIDTH)
          break;
        next = fio___md_line_next(ne, st->end);
      }
      p = next;
      may_start_ref_def = 1;
      bq_may_start_ref_def = 1;
      continue;
    }
    /* Skip HTML blocks during pre-scan */
    if (fio___md_html_block_start(p, le)) {
      if (fio___md_html_long_block(p, le)) {
        if (!fio___md_html_long_close(p, le)) {
          while (next < st->end) {
            char *line = next;
            char *ne = fio___md_line_end(line, st->end);
            next = fio___md_line_next(ne, st->end);
            if (fio___md_html_long_close(line, ne))
              break;
          }
        }
      } else {
        while (next < st->end) {
          char *ne = fio___md_line_end(next, st->end);
          if (fio___md_is_blank(next, ne))
            break;
          next = fio___md_line_next(ne, st->end);
        }
      }
      p = next;
      may_start_ref_def = 1;
      bq_may_start_ref_def = 1;
      continue;
    }

    /* Check for list marker at top level (not inside blockquote) */
    if (bq_depth == 0) {
      int ordered = 0;
      uint64_t start_num = 0;
      char *list_content = NULL;
      fio_buf_info_s list_marker = FIO_BUF_INFO0;
      if (fio___md_list_marker(p, le, &ordered, &start_num, &list_content,
                               &list_marker)) {
        if (list_content < le) {
          if (fio___md_ref_def(list_content, st->end, &label, &dst, &title,
                               &after)) {
            fio___md_ref_cache_add(st, label, dst, title);
            may_start_ref_def = 1;
            p = after ? after : next;
            continue;
          }
        }
      }
    }

    /* Check for ref def on stripped content */
    {
      int can_start =
          (bq_depth > 0) ? bq_may_start_ref_def : may_start_ref_def;
      if (can_start &&
          fio___md_ref_def(content, st->end, &label, &dst, &title, &after)) {
        fio___md_ref_cache_add(st, label, dst, title);
        if (bq_depth > 0)
          bq_may_start_ref_def = 1;
        else
          may_start_ref_def = 1;
        p = after ? after : next;
        continue;
      }
    }

    /* Update paragraph state for next line */
    {
      int is_block = fio___md_line_starts_block(content, le);
      if (bq_depth > 0)
        bq_may_start_ref_def = is_block;
      else
        may_start_ref_def = is_block;
    }
    p = next;
  }
}

/* --- Block detection helpers --- */
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
  char *p = ls;
  size_t n = 0;
  if (fio___md_indent(ls, le) > FIO___MD_MAX_MARKER_INDENT || !marker.len)
    return 0;
  while (p < le && (*p == ' ' || *p == '\t'))
    ++p;
  while (p < le && *p == marker.buf[0]) {
    ++n;
    ++p;
  }
  if (n < marker.len)
    return 0;
  while (p < le && (*p == ' ' || *p == '\t'))
    ++p;
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
  if (num > 999999999)
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
    return FIO_MD_F_TASK | ((p[1] != ' ') ? FIO_MD_F_TASK_CHECKED : 0);
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
                                   fio___md_parser_s *st,
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
  while (p < le && col < ((FIO_MARKDOWN_MAX_TABLE_COLUMNS + 3) >> 2) * 4) {
    char *cell = p;
    char *ce;
    char *t;
    uint8_t align = FIO_MD_ALIGN_NONE;
    p = fio___md_table_next_pipe(p, le);
    ce = fio___md_rtrim(cell, p);
    t = fio___md_ltrim(cell, ce);
    if (t == ce)
      return 0;
    if (*t == ':') {
      align = FIO_MD_ALIGN_LEFT;
      ++t;
    }
    if (t == ce || *t != '-')
      return 0;
    while (t < ce && *t == '-')
      ++t;
    if (t < ce && *t == ':') {
      align = (align == FIO_MD_ALIGN_LEFT) ? FIO_MD_ALIGN_CENTER
                                           : FIO_MD_ALIGN_RIGHT;
      ++t;
    }
    t = fio___md_ltrim(t, ce);
    if (t != ce)
      return 0;
    FIO___MD_TABLE_ALIGN_SET(st, col, align);
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

/* *****************************************************************************
Inline Parser
***************************************************************************** */

/* Forward declarations */
FIO_SFUNC int fio___md_parse_inline_span(fio___md_parser_s *st,
                                         char *p,
                                         char *end);

/* Check if a * or _ run at run_end is left-flanking (run_end points to char
 * after run) */
FIO_IFUNC int fio___md_left_flanking(char *run_start,
                                     char *run_end,
                                     char *buf_start,
                                     char *buf_end) {
  char after = (run_end < buf_end) ? *run_end : ' ';
  char before = (run_start > buf_start) ? run_start[-1] : ' ';
  if (after == ' ' || after == '\t' || after == '\n' || after == '\r')
    return 0;
  if (!fio___md_punct(after))
    return 1;
  return fio___md_punct(before) || before == ' ' || before == '\t' ||
         before == '\n' || before == '\r';
}

/* Check if a * or _ run at run_start is right-flanking */
FIO_IFUNC int fio___md_right_flanking(char *run_start,
                                      char *run_end,
                                      char *buf_start,
                                      char *buf_end) {
  char before = (run_start > buf_start) ? run_start[-1] : ' ';
  char after = (run_end < buf_end) ? *run_end : ' ';
  if (before == ' ' || before == '\t' || before == '\n' || before == '\r')
    return 0;
  if (!fio___md_punct(before))
    return 1;
  return fio___md_punct(after) || after == ' ' || after == '\t' ||
         after == '\n' || after == '\r';
}

/* Check if an outer container of the given type is open in the nesting stack */
FIO_IFUNC int fio___md_has_open_container(fio___md_parser_s *st, uint8_t type) {
  uint8_t d = st->nest[0];
  while (d > 0) {
    if (st->nest[d] == type)
      return 1;
    --d;
  }
  return 0;
}

/* Emit a text event */
FIO_SFUNC int fio___md_inline_text(fio___md_parser_s *st,
                                   char *start,
                                   char *end) {
  fio_md_event_s e;
  if (start == end)
    return 0;
  fio___md_event_init(st, &e);
  e.type = FIO_MD_TEXT;
  e.source = fio___md_buf(start, end);
  e.text = e.source;
  return fio___md_text(st, &e);
}

/* Try to find an emphasis closer. If an outer container's closer is found
   first, return NULL (abort). */
FIO_SFUNC char *fio___md_find_emphasis_closer(fio___md_parser_s *st,
                                              char *p,
                                              char *end,
                                              char marker,
                                              size_t run_len) {
  char *q = p;
  while (q < end) {
    if (*q == '\\' && q + 1 < end) {
      q += 2;
      continue;
    }
    if (*q == marker) {
      char *run_start = q;
      size_t n = 1;
      while (q + n < end && q[n] == marker)
        ++n;
      /* If this run could close an outer container, abort */
      if (marker == '*' && n >= 2 &&
          fio___md_has_open_container(st, FIO_MD_STRONG_STAR))
        return NULL;
      if (marker == '_' && n >= 2 &&
          fio___md_has_open_container(st, FIO_MD_STRONG_UNDERSCORE))
        return NULL;
      if (n == run_len && fio___md_right_flanking(run_start,
                                                  run_start + n,
                                                  st->start,
                                                  st->end)) {
        /* For _, also check intraword restriction */
        if (marker == '_') {
          int lf = fio___md_left_flanking(run_start,
                                          run_start + n,
                                          st->start,
                                          st->end);
          if (lf)
            return NULL; /* intraword _ cannot close */
        }
        return run_start;
      }
      q += n;
      continue;
    }
    ++q;
  }
  return NULL;
}

/* Try to resolve a link or image */
FIO_SFUNC int fio___md_try_link(fio___md_parser_s *st,
                                char *p,
                                char *end,
                                char **after) {
  char *label_start = p + (*p == '!' ? 2 : 1);
  char *label_end = label_start;
  int is_image = (*p == '!');
  fio_md_event_s e;

  if ((is_image && p + 1 == end) || (is_image && p[1] != '['))
    return 0;
  label_end = fio___md_label_end(label_end, end);
  if (label_end == end)
    return 0;

  /* Inline link: ](url "title") */
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

    fio___md_event_init(st, &e);
    e.source = fio___md_buf(p, q + 1);
    e.text = fio___md_buf(label_start, label_end);
    e.destination = fio___md_buf(dst_start, dst_end);
    if (title_start && title_end)
      e.title = fio___md_buf(title_start, title_end);

    if (is_image) {
      e.type = FIO_MD_IMAGE;
      if (fio___md_text(st, &e))
        return -1;
    } else {
      e.type = FIO_MD_LINK;
      if (fio___md_push(st, &e))
        return -1;
      FIO___MD_NEST_PUSH(st, FIO_MD_LINK);
      if (fio___md_parse_inline_span(st, label_start, label_end))
        return -1;
      FIO___MD_NEST_POP(st);
      if (fio___md_pop(st, &e))
        return -1;
    }
    *after = q + 1;
    return 1;
  }

  /* Reference-style links */
  if (!is_image) {
    fio___md_ref_s *ref = NULL;
    fio_buf_info_s ref_label = FIO_BUF_INFO0;

    if (label_end + 1 < end && label_end[1] == '[') {
      char *ref_start = label_end + 2;
      char *ref_end = fio___md_label_end(ref_start, end);
      if (ref_end == end)
        return 0;
      ref_label = (ref_start == ref_end) ? fio___md_buf(label_start, label_end)
                                         : fio___md_buf(ref_start, ref_end);
      ref = fio___md_ref_find(st, ref_label);
      if (ref) {
        fio___md_event_init(st, &e);
        e.type = FIO_MD_LINK;
        e.source = fio___md_buf(p, ref_end + 1);
        e.text = fio___md_buf(label_start, label_end);
        e.destination = ref->destination;
        e.title = ref->title;
        e.reference = ref_label;
        if (fio___md_push(st, &e))
          return -1;
        FIO___MD_NEST_PUSH(st, FIO_MD_LINK);
        if (fio___md_parse_inline_span(st, label_start, label_end))
          return -1;
        FIO___MD_NEST_POP(st);
        if (fio___md_pop(st, &e))
          return -1;
        *after = ref_end + 1;
        return 1;
      }
    } else {
      ref = fio___md_ref_find(st, fio___md_buf(label_start, label_end));
      if (ref) {
        fio___md_event_init(st, &e);
        e.type = FIO_MD_LINK;
        e.source = fio___md_buf(p, label_end + 1);
        e.text = fio___md_buf(label_start, label_end);
        e.destination = ref->destination;
        e.title = ref->title;
        e.reference = e.text;
        if (fio___md_push(st, &e))
          return -1;
        FIO___MD_NEST_PUSH(st, FIO_MD_LINK);
        if (fio___md_parse_inline_span(st, label_start, label_end))
          return -1;
        FIO___MD_NEST_POP(st);
        if (fio___md_pop(st, &e))
          return -1;
        *after = label_end + 1;
        return 1;
      }
    }
  }

  return 0;
}

/* Try autolink or raw HTML */
FIO_SFUNC int fio___md_try_autolink_or_html(fio___md_parser_s *st,
                                            char *p,
                                            char *end,
                                            char **after) {
  char *q = p + 1;
  fio_md_event_s e;
  if (p == end || *p != '<')
    return 0;

  /* Autolink: <http://...>, <https://...>, <email@domain> */
  while (q < end && *q != '>' && *q != ' ' && *q != '\t')
    ++q;
  if (q < end && *q == '>' &&
      (fio___md_prefix_lc(p + 1, q, "http://") ||
       fio___md_prefix_lc(p + 1, q, "https://") ||
       (FIO_MEMCHR(p + 1, '@', (size_t)(q - p - 1)) != NULL))) {
    fio___md_event_init(st, &e);
    e.type = FIO_MD_AUTOLINK;
    e.source = fio___md_buf(p, q + 1);
    e.destination = fio___md_buf(p + 1, q);
    *after = q + 1;
    return fio___md_text(st, &e) ? -1 : 1;
  }

  /* Raw HTML */
  q = p + 1;
  while (q < end && *q != '>')
    ++q;
  if (q < end && *q == '>') {
    fio___md_event_init(st, &e);
    e.type = FIO_MD_INLINE_HTML;
    e.source = fio___md_buf(p, q + 1);
    e.text = e.source;
    *after = q + 1;
    return fio___md_text(st, &e) ? -1 : 1;
  }
  return 0;
}

/* Main inline parser */
FIO_SFUNC int fio___md_parse_inline_span(fio___md_parser_s *st,
                                         char *p,
                                         char *end) {
  char *text = p;
  while (p < end) {
    char *after = p;

    /* Newline handling */
    if (*p == '\n' || *p == '\r') {
      int hard = 0;
      char *text_end = p;
      char *next = p;
      if (text_end > text && text_end[-1] == '\\') {
        hard = 1;
        --text_end;
      } else {
        char *trimmed = fio___md_rtrim(text, text_end);
        if (text_end - trimmed >= 2) {
          hard = 1;
          text_end = trimmed;
        } else {
          text_end = trimmed;
        }
      }
      if (fio___md_inline_text(st, text, text_end))
        return -1;
      if (*next == '\r')
        ++next;
      if (next < end && *next == '\n')
        ++next;
      {
        fio_md_event_s e;
        fio___md_event_init(st, &e);
        e.type = hard ? FIO_MD_HARD_BREAK : FIO_MD_SOFT_BREAK;
        e.source = fio___md_buf(text_end, next);
        if (fio___md_text(st, &e))
          return -1;
      }
      p = fio___md_ltrim(next, end);
      text = p;
      continue;
    }

    /* Code span */
    if (*p == '`') {
      size_t n = 1;
      char *q;
      fio_md_event_s e;
      while (p + n < end && p[n] == '`')
        ++n;
      q = p + n;
      while (q < end) {
        if (*q == '`') {
          char *r = q + 1;
          size_t m = 1;
          while (r < end && *r == '`')
            ++m, ++r;
          if (m == n) {
            if (fio___md_inline_text(st, text, p))
              return -1;
            fio___md_event_init(st, &e);
            e.type = FIO_MD_CODE_SPAN;
            e.source = fio___md_buf(p, r);
            e.text = fio___md_buf(p + n, q);
            if (fio___md_text(st, &e))
              return -1;
            p = r;
            text = p;
            break;
          }
          q = r;
          continue;
        }
        ++q;
      }
      if (q >= end) {
        p += n;
        continue;
      }
      continue;
    }

    /* Emphasis / Strong */
    if ((*p == '*' || *p == '_') && p + 1 < end && p[1] == *p) {
      char marker = *p;
      char *q = p + 2;
      int can_open = fio___md_left_flanking(p, p + 2, st->start, st->end);
      int can_close = fio___md_right_flanking(p, p + 2, st->start, st->end);
      if (marker == '_' && can_open && can_close)
        can_open = can_close = 0; /* intraword _ */
      if (!can_open) {
        p += 2;
        continue;
      }
      q = fio___md_find_emphasis_closer(st, q, end, marker, 2);
      if (q && q > p + 2 && fio___md_span_has_nonspace(p + 2, q)) {
        if (fio___md_inline_text(st, text, p))
          return -1;
        {
          fio_md_event_s e;
          uint8_t strong_type =
              (marker == '*') ? FIO_MD_STRONG_STAR : FIO_MD_STRONG_UNDERSCORE;
          fio___md_event_init(st, &e);
          e.type = strong_type;
          e.source = fio___md_buf(p, q + 2);
          if (fio___md_push(st, &e))
            return -1;
          FIO___MD_NEST_PUSH(st, strong_type);
          if (fio___md_parse_inline_span(st, p + 2, q))
            return -1;
          FIO___MD_NEST_POP(st);
          if (fio___md_pop(st, &e))
            return -1;
        }
        p = q + 2;
        text = p;
        continue;
      }
      p += 2;
      continue;
    }

    if (*p == '*' || *p == '_') {
      char marker = *p;
      char *q = p + 1;
      int can_open = fio___md_left_flanking(p, p + 1, st->start, st->end);
      int can_close = fio___md_right_flanking(p, p + 1, st->start, st->end);
      if (marker == '_' && can_open && can_close)
        can_open = can_close = 0;
      if (!can_open) {
        ++p;
        continue;
      }
      q = fio___md_find_emphasis_closer(st, q, end, marker, 1);
      if (q && q > p + 1 && fio___md_span_has_nonspace(p + 1, q)) {
        if (fio___md_inline_text(st, text, p))
          return -1;
        {
          fio_md_event_s e;
          uint8_t em_type = (marker == '*') ? FIO_MD_EMPHASIS_STAR
                                            : FIO_MD_EMPHASIS_UNDERSCORE;
          fio___md_event_init(st, &e);
          e.type = em_type;
          e.source = fio___md_buf(p, q + 1);
          if (fio___md_push(st, &e))
            return -1;
          FIO___MD_NEST_PUSH(st, em_type);
          if (fio___md_parse_inline_span(st, p + 1, q))
            return -1;
          FIO___MD_NEST_POP(st);
          if (fio___md_pop(st, &e))
            return -1;
        }
        p = q + 1;
        text = p;
        continue;
      }
      ++p;
      continue;
    }

    /* Strikethrough */
    if (*p == '~' && p + 1 < end && p[1] == '~') {
      char *q = p + 2;
      while (q < end) {
        if (*q == '\\' && q + 1 < end) {
          q += 2;
          continue;
        }
        if (q + 1 < end && q[0] == '~' && q[1] == '~')
          break;
        ++q;
      }
      if (q + 1 < end && fio___md_span_has_nonspace(p + 2, q)) {
        if (fio___md_inline_text(st, text, p))
          return -1;
        {
          fio_md_event_s e;
          fio___md_event_init(st, &e);
          e.type = FIO_MD_STRIKETHROUGH;
          e.source = fio___md_buf(p, q + 2);
          if (fio___md_push(st, &e))
            return -1;
          FIO___MD_NEST_PUSH(st, FIO_MD_STRIKETHROUGH);
          if (fio___md_parse_inline_span(st, p + 2, q))
            return -1;
          FIO___MD_NEST_POP(st);
          if (fio___md_pop(st, &e))
            return -1;
        }
        p = q + 2;
        text = p;
        continue;
      }
      p += 2;
      continue;
    }

    /* Link or image */
    if (*p == '!' || *p == '[') {
      int r;
      if (*p == '!' && (p + 1 == end || p[1] != '[')) {
        ++p;
        continue;
      }
      if (text < p) {
        if (fio___md_inline_text(st, text, p))
          return -1;
        text = p;
      }
      r = fio___md_try_link(st, p, end, &after);
      if (r < 0)
        return -1;
      if (r) {
        p = after;
        text = p;
        continue;
      }
    }

    /* Autolink or raw HTML */
    if (*p == '<') {
      int r;
      if (text < p) {
        if (fio___md_inline_text(st, text, p))
          return -1;
        text = p;
      }
      r = fio___md_try_autolink_or_html(st, p, end, &after);
      if (r < 0)
        return -1;
      if (r) {
        p = after;
        text = p;
        continue;
      }
    }

    /* Escape */
    if (*p == '\\' && p + 1 < end && fio___md_punct(p[1])) {
      fio_md_event_s e;
      if (fio___md_inline_text(st, text, p))
        return -1;
      fio___md_event_init(st, &e);
      e.type = FIO_MD_ESCAPE;
      e.source = fio___md_buf(p, p + 2);
      e.text = fio___md_buf(p + 1, p + 2);
      if (fio___md_text(st, &e))
        return -1;
      p += 2;
      text = p;
      continue;
    }

    /* Footnote reference */
    if (*p == '[' && p + 2 < end && p[1] == '^') {
      char *q = fio___md_label_end(p + 2, end);
      if (q < end && q > p + 2) {
        fio_md_event_s e;
        if (fio___md_inline_text(st, text, p))
          return -1;
        fio___md_event_init(st, &e);
        e.type = FIO_MD_FOOTNOTE_REF;
        e.source = fio___md_buf(p, q + 1);
        e.reference = fio___md_buf(p + 2, q);
        if (fio___md_text(st, &e))
          return -1;
        p = q + 1;
        text = p;
        continue;
      }
    }

    /* Entity */
    if (*p == '&') {
      char *q = p + 1;
      while (q < end && q - p < FIO___MD_MAX_ENTITY_LEN &&
             ((*q >= 'a' && *q <= 'z') || (*q >= 'A' && *q <= 'Z') ||
              (*q >= '0' && *q <= '9') || *q == '#'))
        ++q;
      if (q < end && *q == ';') {
        fio_md_event_s e;
        if (fio___md_inline_text(st, text, p))
          return -1;
        fio___md_event_init(st, &e);
        e.type = FIO_MD_ENTITY;
        e.source = fio___md_buf(p, q + 1);
        e.text = e.source;
        if (fio___md_text(st, &e))
          return -1;
        p = q + 1;
        text = p;
        continue;
      }
    }

    ++p;
  }
  return fio___md_inline_text(st, text, end);
}

/* Parse inline content from a buffer, trimming edges */
FIO_SFUNC int fio___md_inline_lines(fio___md_parser_s *st, char *p, char *end) {
  p = fio___md_ltrim(p, end);
  end = fio___md_rtrim(p, end);
  return fio___md_parse_inline_span(st, p, end);
}

/* *****************************************************************************
Block Parser
***************************************************************************** */

FIO_SFUNC int fio___md_parse_table_row(fio___md_parser_s *st,
                                       char *ls,
                                       char *le,
                                       uint16_t columns);
FIO_SFUNC int fio___md_parse_blocks(fio___md_parser_s *st, char *p, char *end);
FIO_SFUNC int fio___md_parse_blocks_ex(fio___md_parser_s *st,
                                       char *p,
                                       char *end,
                                       uint8_t initial_padding);

FIO_SFUNC int fio___md_emit_html_block(fio___md_parser_s *st,
                                       char *p,
                                       char *end) {
  fio_md_event_s e;
  fio___md_event_init(st, &e);
  e.type = FIO_MD_HTML_BLOCK;
  e.source = fio___md_buf(p, end);
  e.text = e.source;
  return (fio___md_push(st, &e) || fio___md_pop(st, &e)) ? -1 : 0;
}

FIO_SFUNC int fio___md_parse_paragraph(fio___md_parser_s *st,
                                       char *p,
                                       char *end) {
  fio_md_event_s e;
  fio___md_event_init(st, &e);
  e.type = FIO_MD_PARAGRAPH;
  e.source = fio___md_buf(p, end);
  if (fio___md_push(st, &e))
    return -1;
  if (fio___md_inline_lines(st, p, end))
    return -1;
  return fio___md_pop(st, &e);
}

/* Find the end of a block starting at line `p`. Returns pointer to first char
   after the block. Used for list item continuations to pass full blocks to
   fio___md_parse_blocks_ex. */
FIO_SFUNC char *fio___md_find_block_end(char *p,
                                        char *end,
                                        uint32_t content_indent) {
  char *le = fio___md_line_end(p, end);
  char *next = fio___md_line_next(le, end);
  fio_buf_info_s fence_marker = FIO_BUF_INFO0;
  fio_buf_info_s fence_info = FIO_BUF_INFO0;
  char *trim = fio___md_ltrim(p, le);
  uint8_t level = 0;
  char *cs = NULL, *ce = NULL;
  fio_buf_info_s dummy_m = FIO_BUF_INFO0, dummy_i = FIO_BUF_INFO0;

  /* Fenced code block */
  if (fio___md_fence(trim, le, &fence_marker, &fence_info)) {
    char *q = next;
    while (q < end) {
      char *qe = fio___md_line_end(q, end);
      if (fio___md_fence_close(q, qe, fence_marker))
        return fio___md_line_next(qe, end);
      q = fio___md_line_next(qe, end);
    }
    return end;
  }

  /* Indented code block */
  if ((uint32_t)(trim - p) + fio___md_indent(p, le) >= FIO___MD_TAB_WIDTH) {
    char *q = next;
    while (q < end) {
      char *qe = fio___md_line_end(q, end);
      if (!fio___md_is_blank(q, qe) &&
          fio___md_indent(q, qe) < FIO___MD_TAB_WIDTH)
        break;
      q = fio___md_line_next(qe, end);
    }
    return q;
  }

  /* HTML block */
  if (fio___md_html_block_start(trim, le)) {
    if (fio___md_html_long_block(trim, le)) {
      if (!fio___md_html_long_close(trim, le)) {
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
    return next;
  }

  /* ATX heading - single line */
  if (fio___md_atx(trim, le, &level, &cs, &ce, &dummy_m))
    return next;

  /* Thematic break - single line */
  if (fio___md_thematic(trim, le))
    return next;

  /* Blockquote */
  if (trim < le && *trim == '>' &&
      (uint32_t)(trim - p) <= FIO___MD_MAX_MARKER_INDENT)
    return next;

  /* List marker at same or lower indent - not our block */
  {
    int ordered = 0;
    uint64_t start_num = 0;
    char *unused = NULL;
    if (fio___md_list_marker(trim,
                             le,
                             &ordered,
                             &start_num,
                             &unused,
                             &dummy_m) &&
        fio___md_indent(p, le) <= content_indent)
      return p;
  }

  /* Paragraph: scan until blank line or block start */
  {
    char *q = next;
    while (q < end) {
      char *qe = fio___md_line_end(q, end);
      char *qn = fio___md_line_next(qe, end);
      char *qt = fio___md_ltrim(q, qe);
      if (fio___md_is_blank(q, qe))
        break;
      if (fio___md_atx(qt, qe, &level, &cs, &ce, &dummy_m) ||
          fio___md_thematic(qt, qe) ||
          fio___md_fence(qt, qe, &dummy_m, &dummy_i) ||
          fio___md_html_block_start(qt, qe) ||
          fio___md_indent(q, qe) < content_indent)
        break;
      q = qn;
    }
    return q;
  }
}

FIO_SFUNC int fio___md_parse_list(fio___md_parser_s *st, char *p, char *end) {
  char *list_start = p;
  char *cur = p;
  int ordered = 0;
  int list_ordered = 0;
  uint64_t start_num = 0;
  char *content;
  fio_buf_info_s marker = FIO_BUF_INFO0;
  fio_md_event_s list_e;
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

  fio___md_event_init(st, &list_e);
  list_e.type = list_ordered ? FIO_MD_LIST_ORDERED : FIO_MD_LIST_UNORDERED;
  list_e.list_start = start_num;
  list_e.marker = marker;
  list_e.flags = FIO_MD_F_TIGHT;

  /* Determine tight/loose */
  {
    uint32_t last_content_indent = 0;
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
          list_e.flags &= ~FIO_MD_F_TIGHT;
          scan = sn;
          continue;
        }
        if (fio___md_indent(look, fio___md_line_end(look, end)) >=
            last_content_indent) {
          list_e.flags &= ~FIO_MD_F_TIGHT;
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
      {
        uint32_t marker_indent = fio___md_indent(scan, scan_marker.buf);
        if (marker_indent == base_indent) {
          last_content_indent = fio___md_column_to(scan, scan_content);
        }
      }
      scan = sn;
    }
  }

  if (fio___md_push(st, &list_e))
    return -1;
  FIO___MD_NEST_PUSH(st, list_e.type);

  while (cur < end) {
    char *le = fio___md_line_end(cur, end);
    char *next = fio___md_line_next(le, end);
    char *item_content;
    fio_md_event_s item_e;
    uint32_t indent;
    uint32_t content_indent;

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

    fio___md_event_init(st, &item_e);
    item_e.type = FIO_MD_LIST_ITEM;
    item_e.source = fio___md_buf(cur, le);
    item_e.marker = marker;
    item_e.list_start = start_num;
    {
      char *after_marker = marker.buf + marker.len;
      uint8_t item_padding = 0;
      content_indent = fio___md_column_to(cur, item_content);
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
      item_e.flags = fio___md_task_flags(&item_content, le) |
                     (list_e.flags & FIO_MD_F_TIGHT);
      if (item_content < le &&
          ((uint32_t)item_padding + fio___md_indent(item_content, le) >=
               FIO___MD_TAB_WIDTH ||
           fio___md_block_start_line(item_content, le)))
        item_e.flags &= (uint8_t)~FIO_MD_F_TIGHT;
      item_e.padding = item_padding;
    }

    if (fio___md_push(st, &item_e))
      return -1;
    FIO___MD_NEST_PUSH(st, FIO_MD_LIST_ITEM);

    if (item_content < le) {
      if (fio___md_thematic(item_content, le)) {
        fio_md_event_s hr;
        fio___md_event_init(st, &hr);
        hr.type = FIO_MD_THEMATIC_BREAK;
        hr.source = fio___md_buf(item_content, le);
        hr.marker = fio___md_buf(fio___md_ltrim(item_content, le),
                                 fio___md_rtrim(item_content, le));
        if (fio___md_push(st, &hr) || fio___md_pop(st, &hr))
          return -1;
      } else if ((uint32_t)item_e.padding + fio___md_indent(item_content, le) >=
                     FIO___MD_TAB_WIDTH ||
                 fio___md_block_start_line(item_content, le)) {
        if (fio___md_parse_blocks_ex(st, item_content, next, item_e.padding))
          return -1;
      } else {
        fio_md_event_s para;
        fio___md_event_init(st, &para);
        para.type = FIO_MD_PARAGRAPH;
        para.source = fio___md_buf(item_content, le);
        if (fio___md_push(st, &para))
          return -1;
        if (fio___md_parse_inline_span(st,
                                       item_content,
                                       fio___md_rtrim(item_content, le)))
          return -1;
        if (fio___md_pop(st, &para))
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
        if (fio___md_parse_list(st, cur, end) < 0)
          return -1;
        cur = st->start + st->consumed;
        continue;
      }
      if (fio___md_indent(cur, ne) >= content_indent) {
        uint8_t child_padding = 0;
        char *child =
            fio___md_skip_indent2(cur, ne, content_indent, &child_padding);
        if (!child)
          break;
        char *child_end = fio___md_find_block_end(cur, end, content_indent);
        /* child_end is relative to cur; we need to strip indent for blocks_ex
         */
        /* For blocks that span multiple lines, child_end might be past the
           current line. fio___md_parse_blocks_ex needs to see the stripped
           content. We pass the raw lines and let blocks_ex handle padding. */
        if (fio___md_parse_blocks_ex(st, child, child_end, child_padding))
          return -1;
        cur = child_end;
        continue;
      }
      break;
    }

    FIO___MD_NEST_POP(st);
    if (fio___md_pop(st, &item_e))
      return -1;
  }

  FIO___MD_NEST_POP(st);
  list_e.source = fio___md_buf(list_start, cur);
  if (fio___md_pop(st, &list_e))
    return -1;
  st->consumed = (size_t)(cur - st->start);
  return 1;
}

FIO_SFUNC int fio___md_parse_blockquote(fio___md_parser_s *st,
                                        char *p,
                                        char *end,
                                        char **after) {
  char *bq_start = p;
  fio_md_event_s bq_e;
  fio_md_event_s para_e;
  char *para_start = NULL;
  char *para_end = NULL;
  char *break_start = NULL;
  char *break_end = NULL;
  int para_open = 0;
  int hard = 0;

  fio___md_event_init(st, &bq_e);
  bq_e.type = FIO_MD_BLOCKQUOTE;
  if (fio___md_push(st, &bq_e))
    return -1;
  FIO___MD_NEST_PUSH(st, FIO_MD_BLOCKQUOTE);

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
      if (!para_open || fio___md_is_blank(p, qe)) {
        break;
      }
      if (fio___md_block_start_line(p, qe)) {
        break;
      }
      qcontent = p;
      qpadding = 0;
    }

    if (fio___md_is_blank(qcontent, qe)) {
      if (para_open) {
        para_e.source = fio___md_buf(para_start, para_end);
        if (fio___md_pop(st, &para_e))
          return -1;
        para_open = 0;
      }
      p = next;
      continue;
    }

    /* Try list inside blockquote */
    if (!para_open) {
      char *qcontent2;
      char *qe2;
      char *next2;
      uint8_t qpadding2 = 0;
      int ordered = 0;
      uint64_t start_num = 0;
      char *item_content = NULL;
      fio_buf_info_s marker = FIO_BUF_INFO0;
      if (fio___md_blockquote_line(p,
                                   end,
                                   &qcontent2,
                                   &qpadding2,
                                   &qe2,
                                   &next2) &&
          fio___md_list_marker(qcontent2,
                               qe2,
                               &ordered,
                               &start_num,
                               &item_content,
                               &marker)) {
        char *list_start = p;
        fio_md_event_s list_e;
        int list_ordered = ordered;
        fio___md_event_init(st, &list_e);
        list_e.type =
            list_ordered ? FIO_MD_LIST_ORDERED : FIO_MD_LIST_UNORDERED;
        list_e.list_start = start_num;
        list_e.marker = marker;
        list_e.flags = FIO_MD_F_TIGHT;
        if (fio___md_push(st, &list_e))
          return -1;
        FIO___MD_NEST_PUSH(st, list_e.type);
        while (p < end) {
          fio_md_event_s item_e;
          fio_md_event_s para;
          uint32_t task_flags;
          if (!fio___md_blockquote_line(p,
                                        end,
                                        &qcontent2,
                                        &qpadding2,
                                        &qe2,
                                        &next2))
            break;
          if (fio___md_is_blank(qcontent2, qe2))
            break;
          if (!fio___md_list_marker(qcontent2,
                                    qe2,
                                    &ordered,
                                    &start_num,
                                    &item_content,
                                    &marker) ||
              ordered != list_ordered)
            break;
          task_flags = fio___md_task_flags(&item_content, qe2);
          fio___md_event_init(st, &item_e);
          item_e.type = FIO_MD_LIST_ITEM;
          item_e.source = fio___md_buf(qcontent2, qe2);
          item_e.marker = marker;
          item_e.list_start = start_num;
          item_e.flags = task_flags | FIO_MD_F_TIGHT;
          if (fio___md_push(st, &item_e))
            return -1;
          FIO___MD_NEST_PUSH(st, FIO_MD_LIST_ITEM);
          if (item_content < qe2) {
            fio___md_event_init(st, &para);
            para.type = FIO_MD_PARAGRAPH;
            para.source = fio___md_buf(item_content, qe2);
            if (fio___md_push(st, &para) ||
                fio___md_parse_inline_span(st,
                                           item_content,
                                           fio___md_rtrim(item_content, qe2)) ||
                fio___md_pop(st, &para))
              return -1;
          }
          FIO___MD_NEST_POP(st);
          if (fio___md_pop(st, &item_e))
            return -1;
          p = next2;
        }
        FIO___MD_NEST_POP(st);
        list_e.source = fio___md_buf(list_start, p);
        if (fio___md_pop(st, &list_e))
          return -1;
        continue;
      }
    }

    /* Fenced code inside blockquote */
    if (!para_open) {
      fio_buf_info_s fence_marker = FIO_BUF_INFO0;
      fio_buf_info_s fence_info = FIO_BUF_INFO0;
      if (fio___md_fence(qcontent, qe, &fence_marker, &fence_info)) {
        char *code_start = p;
        char *body_start = next;
        char *close_line = next;
        char *after_code = next;
        fio_md_event_s code_e;
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
        fio___md_event_init(st, &code_e);
        code_e.type = FIO_MD_CODE_FENCED;
        code_e.source = fio___md_buf(code_start, after_code);
        code_e.text = fio___md_buf(body_start, close_line);
        code_e.marker = fence_marker;
        code_e.info = fence_info;
        code_e.padding = 1;
        if (fio___md_push(st, &code_e) || fio___md_pop(st, &code_e))
          return -1;
        p = after_code;
        continue;
      }
    }

    /* HTML block inside blockquote */
    if (!para_open && fio___md_html_block_start(qcontent, qe)) {
      const int html_long = fio___md_html_long_block(qcontent, qe);
      for (;;) {
        const int html_done =
            html_long ? fio___md_html_long_close(qcontent, qe) : (next >= end);
        if (fio___md_emit_html_block(st, qcontent, next))
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

    /* Nested block inside blockquote */
    if (!para_open && fio___md_block_start_line(qcontent, qe)) {
      if (fio___md_parse_blocks_ex(st, qcontent, next, qpadding))
        return -1;
      p = next;
      continue;
    }

    /* Reference definition inside blockquote */
    if (!para_open) {
      char *ref_after = NULL;
      fio_buf_info_s ref_label = FIO_BUF_INFO0;
      fio_buf_info_s ref_dst = FIO_BUF_INFO0;
      fio_buf_info_s ref_title = FIO_BUF_INFO0;
      if (fio___md_ref_def(qcontent, end, &ref_label, &ref_dst, &ref_title,
                           &ref_after)) {
        fio___md_ref_cache_add(st, ref_label, ref_dst, ref_title);
        p = ref_after;
        continue;
      }
    }

    /* Paragraph continuation */
    ts = fio___md_ltrim(qcontent, qe);
    te = fio___md_rtrim(ts, qe);
    if (!para_open) {
      fio___md_event_init(st, &para_e);
      para_e.type = FIO_MD_PARAGRAPH;
      para_e.source = fio___md_buf(qcontent, qe);
      if (fio___md_push(st, &para_e))
        return -1;
      para_open = 1;
      para_start = qcontent;
    } else if (hard) {
      fio_md_event_s br;
      fio___md_event_init(st, &br);
      br.type = FIO_MD_HARD_BREAK;
      br.source = fio___md_buf(break_start, break_end);
      if (fio___md_text(st, &br))
        return -1;
    } else {
      fio_md_event_s sb;
      fio___md_event_init(st, &sb);
      sb.type = FIO_MD_SOFT_BREAK;
      sb.source = fio___md_buf(break_start, break_end);
      if (fio___md_text(st, &sb))
        return -1;
    }
    if (fio___md_parse_inline_span(st, ts, te))
      return -1;
    hard = 0;
    if (qe > qcontent && qe[-1] == '\\')
      hard = 1;
    {
      char *trimmed = fio___md_rtrim(qcontent, qe);
      if (qe - trimmed >= 2)
        hard = 1;
      if (hard) {
        para_end = trimmed;
        break_start = trimmed;
      } else {
        para_end = qe;
        break_start = qe;
      }
    }
    break_end = next;
    p = next;
  }

  if (para_open) {
    para_e.source = fio___md_buf(para_start, para_end);
    if (fio___md_pop(st, &para_e))
      return -1;
  }

  FIO___MD_NEST_POP(st);
  bq_e.source = fio___md_buf(bq_start, p);
  if (fio___md_pop(st, &bq_e))
    return -1;
  *after = p;
  return 1;
}

FIO_SFUNC int fio___md_parse_blocks_ex(fio___md_parser_s *st,
                                       char *p,
                                       char *end,
                                       uint8_t initial_padding) {
  while (p < end && !st->err) {
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

    /* Skip reference definitions and footnotes */
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
        fio___md_ref_cache_add(st, ref_label, ref_dst, ref_title);
        p = ref_after;
        continue;
      }
    }

    /* Blockquote */
    if (trim < le && *trim == '>' &&
        (uint32_t)(trim - p) <= FIO___MD_MAX_MARKER_INDENT) {
      int rr = fio___md_parse_blockquote(st, p, end, &p);
      if (rr < 0)
        return -1;
      continue;
    }

    /* ATX heading */
    if (fio___md_atx(p, le, &level, &content_start, &content_end, &marker)) {
      fio_md_event_s e;
      fio___md_event_init(st, &e);
      e.type = FIO_MD_HEADING;
      e.source = fio___md_buf(p, le);
      e.text = fio___md_buf(content_start, content_end);
      e.marker = marker;
      e.heading_level = level;
      if (fio___md_push(st, &e) ||
          fio___md_parse_inline_span(st, content_start, content_end) ||
          fio___md_pop(st, &e))
        return -1;
      p = next;
      continue;
    }

    /* Thematic break */
    if (fio___md_thematic(p, le)) {
      fio_md_event_s e;
      fio___md_event_init(st, &e);
      e.type = FIO_MD_THEMATIC_BREAK;
      e.source = fio___md_buf(p, le);
      e.marker = fio___md_buf(trim, fio___md_rtrim(trim, le));
      if (fio___md_push(st, &e) || fio___md_pop(st, &e))
        return -1;
      p = next;
      continue;
    }

    /* Fenced code block */
    if (fio___md_fence(p, le, &marker, &info)) {
      char *body_start = next;
      char *close_line = end;
      char *q = next;
      fio_md_event_s e;
      while (q < end) {
        char *qe = fio___md_line_end(q, end);
        if (fio___md_fence_close(q, qe, marker)) {
          close_line = q;
          q = fio___md_line_next(qe, end);
          break;
        }
        q = fio___md_line_next(qe, end);
      }
      fio___md_event_init(st, &e);
      e.type = FIO_MD_CODE_FENCED;
      e.source = fio___md_buf(p, q);
      e.text = fio___md_buf(body_start, close_line);
      e.marker = marker;
      e.info = info;
      if (fio___md_push(st, &e) || fio___md_pop(st, &e))
        return -1;
      p = q;
      continue;
    }

    /* Indented code block */
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
        fio_md_event_s e;
        fio___md_event_init(st, &e);
        e.type = FIO_MD_CODE_INDENTED;
        e.source = fio___md_buf(code_start, next);
        e.text = fio___md_buf(code_start, code_content_end);
        e.padding = line_padding;
        if (fio___md_push(st, &e) || fio___md_pop(st, &e))
          return -1;
      }
      p = next;
      continue;
    }

    /* HTML block */
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
        fio_md_event_s e;
        fio___md_event_init(st, &e);
        e.type = FIO_MD_HTML_BLOCK;
        e.source = fio___md_buf(html_start, next);
        e.text = e.source;
        if (fio___md_push(st, &e) || fio___md_pop(st, &e))
          return -1;
      }
      p = next;
      continue;
    }

    /* List */
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
        int rr = fio___md_parse_list(st, p, end);
        if (rr < 0)
          return -1;
        p = st->start + st->consumed;
        continue;
      }
    }

    /* Table */
    {
      char *nle = (next < end) ? fio___md_line_end(next, end) : next;
      uint16_t columns = 0;
      if (next < end && fio___md_count_table_cells(p, le) > 1 &&
          fio___md_table_delim(next, nle, st, &columns)) {
        char *table_start = p;
        char *row = fio___md_line_next(nle, end);
        fio_md_event_s e;
        fio___md_event_init(st, &e);
        e.type = FIO_MD_TABLE;
        e.columns = columns;
        while (row < end) {
          char *re = fio___md_line_end(row, end);
          if (fio___md_is_blank(row, re) ||
              fio___md_count_table_cells(row, re) < 2)
            break;
          row = fio___md_line_next(re, end);
        }
        e.source = fio___md_buf(table_start, row);
        if (fio___md_push(st, &e))
          return -1;
        FIO___MD_NEST_PUSH(st, FIO_MD_TABLE);
        if (fio___md_parse_table_row(st, p, le, columns))
          return -1;
        p = fio___md_line_next(nle, end);
        while (p < row) {
          char *re = fio___md_line_end(p, end);
          if (fio___md_parse_table_row(st, p, re, columns))
            return -1;
          p = fio___md_line_next(re, end);
        }
        FIO___MD_NEST_POP(st);
        if (fio___md_pop(st, &e))
          return -1;
        continue;
      }
    }

    /* Paragraph / Setext heading */
    {
      char *para_start = p;
      char *para_end = le;
      char *q = next;
      uint8_t setext_level = 0;
      int ordered_dummy = 0;
      uint64_t start_dummy = 0;
      char *content_dummy = NULL;
      while (q < end) {
        char *qe = fio___md_line_end(q, end);
        char *qt = fio___md_ltrim(q, qe);
        fio_buf_info_s dummy_a = FIO_BUF_INFO0;
        fio_buf_info_s dummy_b = FIO_BUF_INFO0;
        if (fio___md_setext(q, qe, &setext_level)) {
          fio_md_event_s e;
          fio___md_event_init(st, &e);
          e.type = FIO_MD_HEADING;
          e.source = fio___md_buf(para_start, qe);
          e.text = fio___md_buf(fio___md_ltrim(para_start, le),
                                fio___md_rtrim(para_start, para_end));
          e.marker = fio___md_buf(fio___md_ltrim(q, qe), fio___md_rtrim(q, qe));
          e.heading_level = setext_level;
          if (fio___md_push(st, &e) ||
              fio___md_inline_lines(st, e.text.buf, e.text.buf + e.text.len) ||
              fio___md_pop(st, &e))
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
             (uint32_t)(qt - q) <= FIO___MD_MAX_MARKER_INDENT) ||
            (fio___md_list_marker(q, qe, &ordered_dummy, &start_dummy,
                                  &content_dummy, &marker) &&
             (!ordered_dummy || start_dummy == 1)))
          break;
        para_end = qe;
        q = fio___md_line_next(qe, end);
      }
      if (fio___md_parse_paragraph(st, para_start, para_end))
        return -1;
      p = fio___md_line_next(para_end, end);
    }
  fio___md_blocks_next:;
  }
  st->consumed = (size_t)(p - st->start);
  return 0;
}

FIO_SFUNC int fio___md_parse_blocks(fio___md_parser_s *st, char *p, char *end) {
  return fio___md_parse_blocks_ex(st, p, end, 0);
}

FIO_SFUNC int fio___md_parse_table_row(fio___md_parser_s *st,
                                       char *ls,
                                       char *le,
                                       uint16_t columns) {
  char *p = ls;
  uint16_t col = 0;
  fio_md_event_s row_e;
  fio___md_event_init(st, &row_e);
  row_e.type = FIO_MD_TABLE_ROW;
  row_e.source = fio___md_buf(ls, le);
  row_e.columns = columns;
  if (fio___md_push(st, &row_e))
    return -1;
  FIO___MD_NEST_PUSH(st, FIO_MD_TABLE_ROW);

  le = fio___md_rtrim(p, le);
  if (p < le && *p == '|')
    ++p;
  if (p < le && le[-1] == '|')
    --le;

  while (p <= le && col < columns) {
    char *cell_start = p;
    char *cell_end;
    fio_md_event_s cell_e;
    p = fio___md_table_next_pipe(p, le);
    cell_end = p;

    fio___md_event_init(st, &cell_e);
    cell_e.type = FIO_MD_TABLE_CELL;
    cell_e.source = fio___md_buf(cell_start, cell_end);
    cell_e.text = fio___md_buf(fio___md_ltrim(cell_start, cell_end),
                               fio___md_rtrim(cell_start, cell_end));
    cell_e.column = col;
    cell_e.columns = columns;
    cell_e.align = FIO___MD_TABLE_ALIGN_GET(st, col);
    if (fio___md_push(st, &cell_e))
      return -1;
    if (fio___md_parse_inline_span(st,
                                   cell_e.text.buf,
                                   cell_e.text.buf + cell_e.text.len))
      return -1;
    if (fio___md_pop(st, &cell_e))
      return -1;
    ++col;
    if (p < le && *p == '|')
      ++p;
    else
      break;
  }

  FIO___MD_NEST_POP(st);
  return fio___md_pop(st, &row_e);
}

/* *****************************************************************************
Public API
***************************************************************************** */

SFUNC size_t fio_md_parse(const fio_md_callbacks_s *callbacks,
                          void *udata,
                          fio_buf_info_s source) {
  fio___md_parser_s st;
  FIO_MEMSET(&st, 0, sizeof(st));
  st.cb = callbacks;
  st.udata = udata;
  st.start = source.buf;
  st.end = source.buf ? source.buf + source.len : NULL;

  if (!source.buf && source.len) {
    fio_md_event_s e;
    fio___md_event_init(&st, &e);
    e.err = FIO_MD_ERR_INPUT;
    e.consumed = 0;
    fio___md_error(&st, &e);
    return 0;
  }

  if (source.buf &&
      fio___md_parse_blocks(&st, source.buf, source.buf + source.len))
    return st.consumed;

  st.consumed = source.len;
  return source.len;
}

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_MARKDOWN */
