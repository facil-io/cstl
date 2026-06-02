/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MD2HTML            /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                       Markdown to HTML bstr Renderer


Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_MD2HTML) && !defined(H___FIO_MD2HTML___H)
#define H___FIO_MD2HTML___H

#ifndef FIO_MD2HTML_ERR_ALLOC
/** Markdown-to-HTML renderer callback error: output allocation failed. */
#define FIO_MD2HTML_ERR_ALLOC 1
#endif

/**
 * Renders a complete Markdown / GFM document into an owned `fio_bstr`.
 *
 * The returned pointer should be released with `fio_bstr_free`. Pass NULL as
 * `bstr_target` to allocate a new buffer, or pass an existing `fio_bstr` to
 * append to it. On parser or allocation failure, NULL is returned; newly
 * allocated output is freed.
 *
 * The renderer preserves raw Markdown HTML blocks / inline HTML, and escapes
 * normal text, code, and attribute values.
 */
SFUNC char *fio_md2html(char *bstr_target, fio_buf_info_s source);

/* *****************************************************************************
Markdown to HTML Renderer - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

typedef struct {
  fio_buf_info_s label;
  fio_buf_info_s first;
  fio_buf_info_s rest;
  uint16_t index;
  uint16_t ref_count;
} fio___md2html_footnote_s;

typedef struct {
  fio___md2html_footnote_s *note;
  uint16_t ordinal;
} fio___md2html_footnote_use_s;

typedef struct {
  char *html;
  size_t consumed;
  fio___md2html_footnote_s footnotes[FIO_MARKDOWN_MAX_REFERENCES];
  fio___md2html_footnote_use_s footnote_uses[FIO_MARKDOWN_MAX_REFERENCES];
  uint32_t table_row;
  uint32_t tight_list_item_depth;
  uint32_t paragraph_suppressed_depth;
  uint16_t footnote_count;
  uint16_t footnote_use_count;
  uint8_t rendering_footnotes;
  int err;
} fio___md2html_s;

#define FIO___MD2HTML_APPEND_LITERAL(renderer, literal)                        \
  fio___md2html_append((renderer), (literal), sizeof(literal) - 1)

FIO_IFUNC int fio___md2html_append(fio___md2html_s *renderer,
                                   const void *buf,
                                   size_t len) {
  size_t before;
  if (renderer->err)
    return renderer->err;
  if (!len)
    return 0;
  before = fio_bstr_len(renderer->html);
  renderer->html = fio_bstr_write(renderer->html, buf, len);
  if (FIO_UNLIKELY(fio_bstr_len(renderer->html) != before + len)) {
    renderer->err = FIO_MD2HTML_ERR_ALLOC;
    return renderer->err;
  }
  return 0;
}

FIO_IFUNC int fio___md2html_append_u(fio___md2html_s *renderer, uint64_t n) {
  char buf[128];
  size_t len = fio_ltoa(buf, (int64_t)n, 10);
  return fio___md2html_append(renderer, buf, len);
}

FIO_IFUNC fio_buf_info_s fio___md2html_trim(fio_buf_info_s s) {
  char *start = s.buf;
  char *end;
  if (!s.buf || !s.len)
    return FIO_BUF_INFO0;
  end = s.buf + s.len;
  while (start < end &&
         (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r'))
    ++start;
  while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' ||
                         end[-1] == '\r'))
    --end;
  return FIO_BUF_INFO2(start, (size_t)(end - start));
}

FIO_IFUNC fio_buf_info_s fio___md2html_info_language(fio_buf_info_s info) {
  char *start;
  char *end;
  info = fio___md2html_trim(info);
  if (!info.buf || !info.len)
    return FIO_BUF_INFO0;
  start = info.buf;
  end = info.buf + info.len;
  while (start < end && *start != ' ' && *start != '\t' && *start != '\n' &&
         *start != '\r')
    ++start;
  return FIO_BUF_INFO2(info.buf, (size_t)(start - info.buf));
}

FIO_IFUNC int fio___md2html_escape(fio___md2html_s *renderer,
                                   fio_buf_info_s src,
                                   uint8_t is_attr) {
  char *pos = src.buf;
  char *mark = src.buf;
  char *end;
  if (!src.buf || !src.len)
    return 0;
  end = src.buf + src.len;
  while (pos < end) {
    const char *entity = NULL;
    size_t entity_len = 0;
    switch (*pos) {
    case '&':
      entity = "&amp;";
      entity_len = 5;
      break;
    case '<':
      entity = "&lt;";
      entity_len = 4;
      break;
    case '>':
      entity = "&gt;";
      entity_len = 4;
      break;
    case '"':
      entity = "&quot;";
      entity_len = 6;
      break;
    case '\'':
      if (is_attr) {
        entity = "&#39;";
        entity_len = 5;
      }
      break;
    }
    if (entity) {
      if (fio___md2html_append(renderer, mark, (size_t)(pos - mark)) ||
          fio___md2html_append(renderer, entity, entity_len))
        return renderer->err;
      mark = pos + 1;
    }
    ++pos;
  }
  return fio___md2html_append(renderer, mark, (size_t)(end - mark));
}

FIO_IFUNC int fio___md2html_escape_attr(fio___md2html_s *renderer,
                                        fio_buf_info_s src) {
  return fio___md2html_escape(renderer, src, 1);
}

FIO_IFUNC int fio___md2html_escape_text(fio___md2html_s *renderer,
                                        fio_buf_info_s src) {
  return fio___md2html_escape(renderer, src, 0);
}

FIO_IFUNC int fio___md2html_escape_md_attr(fio___md2html_s *renderer,
                                           fio_buf_info_s src) {
  char *p = src.buf;
  char *mark = src.buf;
  char *end = src.buf + src.len;
  while (p < end) {
    if (*p == '\\' && p + 1 < end && fio___md_punct(p[1])) {
      if (fio___md2html_escape_attr(renderer,
                                    FIO_BUF_INFO2(mark, (size_t)(p - mark))) ||
          fio___md2html_escape_attr(renderer, FIO_BUF_INFO2(p + 1, 1)))
        return renderer->err;
      p += 2;
      mark = p;
      continue;
    }
    ++p;
  }
  return fio___md2html_escape_attr(renderer,
                                   FIO_BUF_INFO2(mark, (size_t)(end - mark)));
}

FIO_IFUNC int fio___md2html_has_percent(fio_buf_info_s src) {
  return src.len && FIO_MEMCHR(src.buf, '%', src.len) != NULL;
}

FIO_IFUNC int fio___md2html_url_keep(unsigned char c, uint8_t keep_reserved) {
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
      (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
    return 1;
  if (keep_reserved) {
    switch (c) {
    case ':':
    case '/':
    case '?':
    case '#':
    case '[':
    case ']':
    case '@':
    case '!':
    case '$':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case ';':
    case '=': return 1;
    }
  } else if (c == '/' || c == '(' || c == ')') {
    return 1;
  }
  return 0;
}

FIO_IFUNC int fio___md2html_append_pct(fio___md2html_s *renderer,
                                       unsigned char c) {
  static const char hex[] = "0123456789ABCDEF";
  char buf[3] = {'%', hex[c >> 4], hex[c & 15]};
  return fio___md2html_append(renderer, buf, 3);
}

FIO_IFUNC int fio___md2html_escape_url(fio___md2html_s *renderer,
                                       fio_buf_info_s src,
                                       uint8_t keep_reserved) {
  char *p = src.buf;
  char *mark = src.buf;
  char *end = src.buf + src.len;
  while (p < end) {
    unsigned char c = (unsigned char)*p;
    if (!fio___md2html_url_keep(c, keep_reserved)) {
      if (fio___md2html_escape_attr(renderer,
                                    FIO_BUF_INFO2(mark, (size_t)(p - mark))) ||
          fio___md2html_append_pct(renderer, c))
        return renderer->err;
      mark = p + 1;
    }
    ++p;
  }
  return fio___md2html_escape_attr(renderer,
                                   FIO_BUF_INFO2(mark, (size_t)(end - mark)));
}

FIO_IFUNC int fio___md2html_escape_md_url(fio___md2html_s *renderer,
                                          fio_buf_info_s src,
                                          uint8_t keep_reserved) {
  char *p = src.buf;
  char *mark = src.buf;
  char *end = src.buf + src.len;
  while (p < end) {
    unsigned char c = (unsigned char)*p;
    if (*p == '\\' && p + 1 < end && fio___md_punct(p[1])) {
      c = (unsigned char)p[1];
      if (fio___md2html_escape_attr(renderer,
                                    FIO_BUF_INFO2(mark, (size_t)(p - mark))))
        return renderer->err;
      if (fio___md2html_url_keep(c, keep_reserved)) {
        if (fio___md2html_escape_attr(renderer, FIO_BUF_INFO2(p + 1, 1)))
          return renderer->err;
      } else if (fio___md2html_append_pct(renderer, c)) {
        return renderer->err;
      }
      p += 2;
      mark = p;
      continue;
    }
    if (!fio___md2html_url_keep(c, keep_reserved)) {
      if (fio___md2html_escape_attr(renderer,
                                    FIO_BUF_INFO2(mark, (size_t)(p - mark))) ||
          fio___md2html_append_pct(renderer, c))
        return renderer->err;
      mark = p + 1;
    }
    ++p;
  }
  return fio___md2html_escape_attr(renderer,
                                   FIO_BUF_INFO2(mark, (size_t)(end - mark)));
}

FIO_IFUNC int fio___md2html_escape_reference_url(fio___md2html_s *renderer,
                                                 fio_buf_info_s src,
                                                 fio_buf_info_s reference) {
  if (!reference.len || fio___md2html_has_percent(src))
    return fio___md2html_escape_attr(renderer, src);
  return fio___md2html_escape_md_url(renderer, src, 1);
}

FIO_IFUNC int fio___md2html_escape_footnote_id(fio___md2html_s *renderer,
                                               fio_buf_info_s src) {
  return fio___md2html_escape_url(renderer, src, 0);
}

FIO_IFUNC int fio___md2html_is_code_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

FIO_IFUNC int fio___md2html_escape_code_span(fio___md2html_s *renderer,
                                             fio_buf_info_s src) {
  char *p = src.buf;
  char *end;
  if (!src.buf || !src.len)
    return 0;
  end = src.buf + src.len;
  if (src.len > 1 && fio___md2html_is_code_space(*p) &&
      fio___md2html_is_code_space(end[-1])) {
    char *q = p + 1;
    while (q + 1 < end && fio___md2html_is_code_space(*q))
      ++q;
    if (q + 1 < end) {
      ++p;
      --end;
    }
  }
  while (p < end) {
    char *mark = p;
    while (p < end && *p != '\n' && *p != '\r')
      ++p;
    if (fio___md2html_escape_text(renderer,
                                  FIO_BUF_INFO2(mark, (size_t)(p - mark))))
      return renderer->err;
    if (p < end) {
      if (*p == '\r')
        ++p;
      if (p < end && *p == '\n')
        ++p;
      if (fio___md2html_escape_text(renderer, FIO_BUF_INFO2(" ", 1)))
        return renderer->err;
    }
  }
  return 0;
}

FIO_IFUNC int fio___md2html_escape_fenced_code(fio___md2html_s *renderer,
                                               fio_buf_info_s src,
                                               size_t strip_spaces) {
  char *pos = src.buf;
  char *end;
  if (!src.buf || !src.len)
    return 0;
  end = src.buf + src.len;
  while (pos < end) {
    char *line_end = pos;
    char *next;
    char *code = pos;
    size_t spaces = 0;
    while (line_end < end && *line_end != '\n' && *line_end != '\r')
      ++line_end;
    while (code < line_end && spaces < strip_spaces && *code == ' ') {
      ++code;
      ++spaces;
    }
    if (fio___md2html_escape_text(
            renderer,
            FIO_BUF_INFO2(code, (size_t)(line_end - code))))
      return renderer->err;
    next = line_end;
    if (next < end && *next == '\r')
      ++next;
    if (next < end && *next == '\n')
      ++next;
    if (fio___md2html_append(renderer, line_end, (size_t)(next - line_end)))
      return renderer->err;
    pos = next;
  }
  return 0;
}

FIO_IFUNC int fio___md2html_escape_bq_fenced_code(fio___md2html_s *renderer,
                                                  fio_buf_info_s src) {
  char *pos = src.buf;
  char *end;
  if (!src.buf || !src.len)
    return 0;
  end = src.buf + src.len;
  while (pos < end) {
    char *line_end = pos;
    char *next;
    char *code = pos;
    uint8_t spaces = 0;
    while (line_end < end && *line_end != '\n' && *line_end != '\r')
      ++line_end;
    while (code < line_end && spaces < FIO___MD_MAX_MARKER_INDENT &&
           *code == ' ') {
      ++code;
      ++spaces;
    }
    if (code < line_end && *code == '>') {
      ++code;
      if (code < line_end && *code == ' ')
        ++code;
    }
    if (fio___md2html_escape_text(
            renderer,
            FIO_BUF_INFO2(code, (size_t)(line_end - code))))
      return renderer->err;
    next = line_end;
    if (next < end && *next == '\r')
      ++next;
    if (next < end && *next == '\n')
      ++next;
    if (fio___md2html_append(renderer, line_end, (size_t)(next - line_end)))
      return renderer->err;
    pos = next;
  }
  return 0;
}

FIO_IFUNC int fio___md2html_escape_indented_code(fio___md2html_s *renderer,
                                                 fio_buf_info_s src,
                                                 uint8_t padding) {
  static const char pad[64] =
      "                                                               ";
  char *pos = src.buf;
  char *end;
  if (!src.buf || !src.len)
    return 0;
  end = src.buf + src.len;
  while (pos < end) {
    char *line_end = pos;
    char *next;
    char *code = pos;
    uint8_t spaces = padding;
    while (line_end < end && *line_end != '\n' && *line_end != '\r')
      ++line_end;
    while (code < line_end && spaces < 4) {
      if (*code == ' ') {
        ++code;
        ++spaces;
      } else if (*code == '\t') {
        spaces +=
            (uint8_t)((padding && spaces == padding) ? 4U
                                                     : (4U - (spaces & 3U)));
        ++code;
      } else {
        break;
      }
    }
    if (spaces > 4 && code < line_end &&
        fio___md2html_append(renderer, pad, spaces - 4))
      return renderer->err;
    if (fio___md2html_escape_text(
            renderer,
            FIO_BUF_INFO2(code, (size_t)(line_end - code))))
      return renderer->err;
    next = line_end;
    if (next < end && *next == '\r')
      ++next;
    if (next < end && *next == '\n')
      ++next;
    if (fio___md2html_append(renderer, line_end, (size_t)(next - line_end)))
      return renderer->err;
    pos = next;
  }
  return 0;
}

FIO_IFUNC int fio___md2html_append_title(fio___md2html_s *renderer,
                                         fio_buf_info_s title) {
  if (!title.len)
    return 0;
  return FIO___MD2HTML_APPEND_LITERAL(renderer, "\" title=\"") ||
         fio___md2html_escape_md_attr(renderer, title);
}

FIO_IFUNC int fio___md2html_align_attr(fio___md2html_s *renderer,
                                       uint8_t align) {
  switch (align) {
  case FIO_MD_ALIGN_LEFT:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, " align=\"left\"");
  case FIO_MD_ALIGN_RIGHT:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, " align=\"right\"");
  case FIO_MD_ALIGN_CENTER:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, " align=\"center\"");
  }
  return 0;
}

FIO_IFUNC fio___md2html_footnote_s *fio___md2html_footnote_find(
    fio___md2html_s *renderer,
    fio_buf_info_s label) {
  for (uint16_t i = 0; i < renderer->footnote_count; ++i) {
    if (fio___md_slice_eq_lc(label, renderer->footnotes[i].label))
      return renderer->footnotes + i;
  }
  return NULL;
}

FIO_SFUNC void fio___md2html_scan_footnotes(fio___md2html_s *renderer,
                                            fio_buf_info_s source) {
  char *p = source.buf;
  char *end = source.buf ? source.buf + source.len : NULL;
  while (p < end) {
    char *le = fio___md_line_end(p, end);
    char *content = NULL;
    fio_buf_info_s label = FIO_BUF_INFO0;
    if (fio___md_footnote_line(p, le, &label, &content) &&
        renderer->footnote_count < FIO_MARKDOWN_MAX_REFERENCES &&
        !fio___md2html_footnote_find(renderer, label)) {
      char *next = fio___md_line_next(le, end);
      char *def_end = fio___md_footnote_end(p, end);
      fio___md2html_footnote_s *note =
          renderer->footnotes + renderer->footnote_count++;
      note->label = label;
      note->first = fio___md_buf(content, le);
      note->rest = fio___md_buf(next, def_end);
      p = def_end;
      continue;
    }
    p = fio___md_line_next(le, end);
  }
}

FIO_IFUNC int fio___md2html_append_footnote_id(fio___md2html_s *renderer,
                                               const char *prefix,
                                               fio_buf_info_s label) {
  return fio___md2html_append(renderer, prefix, FIO_STRLEN(prefix)) ||
         fio___md2html_escape_footnote_id(renderer, label);
}

FIO_SFUNC fio___md2html_footnote_s *fio___md2html_use_footnote(
    fio___md2html_s *renderer,
    fio_buf_info_s label,
    uint16_t *ordinal) {
  fio___md2html_footnote_s *note = fio___md2html_footnote_find(renderer, label);
  if (!note || renderer->footnote_use_count >= FIO_MARKDOWN_MAX_REFERENCES)
    return NULL;
  if (!note->index)
    note->index = (uint16_t)(renderer->footnote_use_count + 1);
  *ordinal = (uint16_t)(++note->ref_count);
  renderer->footnote_uses[renderer->footnote_use_count].note = note;
  renderer->footnote_uses[renderer->footnote_use_count].ordinal = *ordinal;
  ++renderer->footnote_use_count;
  return note;
}

FIO_IFUNC int fio___md2html_append_footnote_ref_id(
    fio___md2html_s *renderer,
    fio___md2html_footnote_s *note,
    uint16_t ordinal) {
  if (fio___md2html_append_footnote_id(renderer, "fnref-", note->label))
    return renderer->err;
  if (ordinal > 1)
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "-") ||
           fio___md2html_append_u(renderer, ordinal);
  return 0;
}

FIO_SFUNC int fio___md2html_append_footnote_ref(fio___md2html_s *renderer,
                                                fio_buf_info_s label) {
  uint16_t ordinal = 0;
  fio___md2html_footnote_s *note =
      fio___md2html_use_footnote(renderer, label, &ordinal);
  if (!note)
    return fio___md2html_escape_text(
        renderer,
        FIO_BUF_INFO2(label.buf - 2, label.len + 3));
  return FIO___MD2HTML_APPEND_LITERAL(
             renderer,
             "<sup class=\"footnote-ref\"><a href=\"#") ||
         fio___md2html_append_footnote_id(renderer, "fn-", note->label) ||
         FIO___MD2HTML_APPEND_LITERAL(renderer, "\" id=\"") ||
         fio___md2html_append_footnote_ref_id(renderer, note, ordinal) ||
         FIO___MD2HTML_APPEND_LITERAL(renderer, "\" data-footnote-ref>") ||
         fio___md2html_append_u(renderer, note->index) ||
         FIO___MD2HTML_APPEND_LITERAL(renderer, "</a></sup>");
}

FIO_IFUNC int fio___md2html_normalized_footnote_append(char **dst,
                                                       const void *buf,
                                                       size_t len) {
  size_t before = fio_bstr_len(*dst);
  *dst = fio_bstr_write(*dst, buf, len);
  return fio_bstr_len(*dst) == before + len ? 0 : -1;
}

FIO_SFUNC char *fio___md2html_normalize_footnote(
    fio___md2html_footnote_s *note) {
  static const char pad[64] =
      "                                                               ";
  char *out = fio_bstr_reserve(NULL, note->first.len + note->rest.len + 8);
  char *p = note->rest.buf;
  char *end = note->rest.buf ? note->rest.buf + note->rest.len : NULL;
  if (!out)
    return NULL;
  if (note->first.len) {
    if (fio___md2html_normalized_footnote_append(&out,
                                                 note->first.buf,
                                                 note->first.len) ||
        fio___md2html_normalized_footnote_append(&out, "\n", 1))
      goto error;
  }
  while (p < end) {
    char *le = fio___md_line_end(p, end);
    char *next = fio___md_line_next(le, end);
    if (fio___md_is_blank(p, le)) {
      if (fio___md2html_normalized_footnote_append(&out, "\n", 1))
        goto error;
    } else {
      uint8_t padding = 0;
      char *content =
          fio___md_skip_indent2(p, le, FIO___MD_TAB_WIDTH, &padding);
      if (!content)
        content = p;
      if (padding &&
          fio___md2html_normalized_footnote_append(&out, pad, padding))
        goto error;
      if (fio___md2html_normalized_footnote_append(&out,
                                                   content,
                                                   (size_t)(le - content)) ||
          fio___md2html_normalized_footnote_append(&out, "\n", 1))
        goto error;
    }
    p = next;
  }
  return out;
error:
  fio_bstr_free(out);
  return NULL;
}

FIO_SFUNC int fio___md2html_append_backrefs(fio___md2html_s *renderer,
                                            fio___md2html_footnote_s *note) {
  uint16_t printed = 0;
  for (uint16_t i = 0; i < renderer->footnote_use_count; ++i) {
    fio___md2html_footnote_use_s *use = renderer->footnote_uses + i;
    if (use->note != note)
      continue;
    ++printed;
    if (printed > 1 && FIO___MD2HTML_APPEND_LITERAL(renderer, " "))
      return renderer->err;
    if (FIO___MD2HTML_APPEND_LITERAL(renderer, "<a href=\"#") ||
        fio___md2html_append_footnote_ref_id(renderer, note, use->ordinal) ||
        FIO___MD2HTML_APPEND_LITERAL(
            renderer,
            "\" class=\"footnote-backref\" data-footnote-backref "
            "data-footnote-backref-idx=\"") ||
        fio___md2html_append_u(renderer, note->index))
      return renderer->err;
    if (use->ordinal > 1 && (FIO___MD2HTML_APPEND_LITERAL(renderer, "-") ||
                             fio___md2html_append_u(renderer, use->ordinal)))
      return renderer->err;
    if (FIO___MD2HTML_APPEND_LITERAL(renderer,
                                     "\" aria-label=\"Back to reference ") ||
        fio___md2html_append_u(renderer, note->index))
      return renderer->err;
    if (use->ordinal > 1 && (FIO___MD2HTML_APPEND_LITERAL(renderer, "-") ||
                             fio___md2html_append_u(renderer, use->ordinal)))
      return renderer->err;
    if (FIO___MD2HTML_APPEND_LITERAL(renderer, "\">↩") ||
        (use->ordinal > 1
             ? (FIO___MD2HTML_APPEND_LITERAL(renderer,
                                             "<sup class=\"footnote-ref\">") ||
                fio___md2html_append_u(renderer, use->ordinal) ||
                FIO___MD2HTML_APPEND_LITERAL(renderer, "</sup>"))
             : 0) ||
        FIO___MD2HTML_APPEND_LITERAL(renderer, "</a>"))
      return renderer->err;
  }
  return 0;
}

FIO_SFUNC int fio___md2html_block_enter(void *udata,
                                        const fio_md_block_s *block) {
  fio___md2html_s *renderer = (fio___md2html_s *)udata;
  fio_buf_info_s language;
  if (renderer->err)
    return renderer->err;
  switch (block->type) {
  case FIO_MD_BLOCK_DOCUMENT: return 0;
  case FIO_MD_BLOCK_PARAGRAPH:
    if (renderer->tight_list_item_depth) {
      ++renderer->paragraph_suppressed_depth;
      return 0;
    }
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<p>");
  case FIO_MD_BLOCK_HEADING:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<h") ||
           fio___md2html_append_u(renderer, block->heading_level) ||
           FIO___MD2HTML_APPEND_LITERAL(renderer, ">");
  case FIO_MD_BLOCK_THEMATIC_BREAK:
    if (renderer->tight_list_item_depth &&
        FIO___MD2HTML_APPEND_LITERAL(renderer, "\n"))
      return renderer->err;
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<hr />\n");
  case FIO_MD_BLOCK_BLOCK_QUOTE:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<blockquote>\n");
  case FIO_MD_BLOCK_LIST_UNORDERED:
    if (renderer->tight_list_item_depth &&
        FIO___MD2HTML_APPEND_LITERAL(renderer, "\n"))
      return renderer->err;
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<ul>\n");
  case FIO_MD_BLOCK_LIST_ORDERED:
    if (renderer->tight_list_item_depth &&
        FIO___MD2HTML_APPEND_LITERAL(renderer, "\n"))
      return renderer->err;
    if (FIO___MD2HTML_APPEND_LITERAL(renderer, "<ol"))
      return renderer->err;
    if (block->list_start && block->list_start != 1) {
      if (FIO___MD2HTML_APPEND_LITERAL(renderer, " start=\"") ||
          fio___md2html_append_u(renderer, block->list_start) ||
          FIO___MD2HTML_APPEND_LITERAL(renderer, "\""))
        return renderer->err;
    }
    return FIO___MD2HTML_APPEND_LITERAL(renderer, ">\n");
  case FIO_MD_BLOCK_LIST_ITEM:
    if (block->flags & FIO_MD_BLOCK_F_TIGHT)
      ++renderer->tight_list_item_depth;
    if (FIO___MD2HTML_APPEND_LITERAL(renderer, "<li>"))
      return renderer->err;
    if (!(block->flags & FIO_MD_BLOCK_F_TIGHT) &&
        FIO___MD2HTML_APPEND_LITERAL(renderer, "\n"))
      return renderer->err;
    if (block->flags & FIO_MD_BLOCK_F_TASK) {
      if (FIO___MD2HTML_APPEND_LITERAL(renderer,
                                       "<input type=\"checkbox\" disabled") ||
          ((block->flags & FIO_MD_BLOCK_F_TASK_CHECKED)
               ? FIO___MD2HTML_APPEND_LITERAL(renderer, " checked")
               : 0) ||
          FIO___MD2HTML_APPEND_LITERAL(renderer, "> "))
        return renderer->err;
    }
    return 0;
  case FIO_MD_BLOCK_CODE_INDENTED:
  case FIO_MD_BLOCK_CODE_FENCED:
    if (FIO___MD2HTML_APPEND_LITERAL(renderer, "<pre><code"))
      return renderer->err;
    language = fio___md2html_info_language(block->info);
    if (language.len &&
        (FIO___MD2HTML_APPEND_LITERAL(renderer, " class=\"language-") ||
         fio___md2html_escape_attr(renderer, language) ||
         FIO___MD2HTML_APPEND_LITERAL(renderer, "\"")))
      return renderer->err;
    if (FIO___MD2HTML_APPEND_LITERAL(renderer, ">"))
      return renderer->err;
    if (block->type == FIO_MD_BLOCK_CODE_INDENTED) {
      if (fio___md2html_escape_indented_code(renderer,
                                             block->content,
                                             block->padding))
        return renderer->err;
    } else if (block->padding) {
      if (fio___md2html_escape_bq_fenced_code(renderer, block->content))
        return renderer->err;
    } else if (fio___md2html_escape_fenced_code(
                   renderer,
                   block->content,
                   (size_t)(block->marker.buf - block->source.buf))) {
      return renderer->err;
    }
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "</code></pre>\n");
  case FIO_MD_BLOCK_HTML:
    return fio___md2html_append(renderer,
                                block->content.buf,
                                block->content.len);
  case FIO_MD_BLOCK_TABLE:
    renderer->table_row = 0;
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<table>\n<thead>\n");
  case FIO_MD_BLOCK_TABLE_ROW:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<tr>");
  case FIO_MD_BLOCK_TABLE_CELL:
    if (renderer->table_row == 0) {
      if (FIO___MD2HTML_APPEND_LITERAL(renderer, "<th"))
        return renderer->err;
    } else if (FIO___MD2HTML_APPEND_LITERAL(renderer, "<td")) {
      return renderer->err;
    }
    return fio___md2html_align_attr(renderer, block->align) ||
           FIO___MD2HTML_APPEND_LITERAL(renderer, ">");
  }
  return 0;
}

FIO_SFUNC int fio___md2html_block_leave(void *udata,
                                        const fio_md_block_s *block) {
  fio___md2html_s *renderer = (fio___md2html_s *)udata;
  if (renderer->err)
    return renderer->err;
  switch (block->type) {
  case FIO_MD_BLOCK_DOCUMENT: return 0;
  case FIO_MD_BLOCK_PARAGRAPH:
    if (renderer->paragraph_suppressed_depth) {
      --renderer->paragraph_suppressed_depth;
      return 0;
    }
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "</p>\n");
  case FIO_MD_BLOCK_HEADING:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "</h") ||
           fio___md2html_append_u(renderer, block->heading_level) ||
           FIO___MD2HTML_APPEND_LITERAL(renderer, ">\n");
  case FIO_MD_BLOCK_BLOCK_QUOTE:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "</blockquote>\n");
  case FIO_MD_BLOCK_LIST_UNORDERED:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "</ul>\n");
  case FIO_MD_BLOCK_LIST_ORDERED:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "</ol>\n");
  case FIO_MD_BLOCK_LIST_ITEM:
    if (FIO___MD2HTML_APPEND_LITERAL(renderer, "</li>\n"))
      return renderer->err;
    if ((block->flags & FIO_MD_BLOCK_F_TIGHT) &&
        renderer->tight_list_item_depth)
      --renderer->tight_list_item_depth;
    return 0;
  case FIO_MD_BLOCK_TABLE:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "</tbody>\n</table>\n");
  case FIO_MD_BLOCK_TABLE_ROW:
    if (renderer->table_row++ == 0)
      return FIO___MD2HTML_APPEND_LITERAL(renderer,
                                          "</tr>\n</thead>\n<tbody>\n");
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "</tr>\n");
  case FIO_MD_BLOCK_TABLE_CELL:
    return (renderer->table_row == 0)
               ? FIO___MD2HTML_APPEND_LITERAL(renderer, "</th>")
               : FIO___MD2HTML_APPEND_LITERAL(renderer, "</td>");
  case FIO_MD_BLOCK_THEMATIC_BREAK:
  case FIO_MD_BLOCK_CODE_INDENTED:
  case FIO_MD_BLOCK_CODE_FENCED:
  case FIO_MD_BLOCK_HTML: return 0;
  }
  return 0;
}

FIO_SFUNC int fio___md2html_inline(void *udata,
                                   const fio_md_inline_s *inline_event) {
  fio___md2html_s *renderer = (fio___md2html_s *)udata;
  if (renderer->err)
    return renderer->err;
  switch (inline_event->type) {
  case FIO_MD_INLINE_TEXT:
    return fio___md2html_escape_text(renderer, inline_event->text);
  case FIO_MD_INLINE_SOFT_BREAK:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "\n");
  case FIO_MD_INLINE_HARD_BREAK:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<br />\n");
  case FIO_MD_INLINE_CODE_SPAN:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<code>") ||
           fio___md2html_escape_code_span(renderer, inline_event->text) ||
           FIO___MD2HTML_APPEND_LITERAL(renderer, "</code>");
  case FIO_MD_INLINE_EMPHASIS:
    if (inline_event->event == FIO_MD_EVENT_ENTER)
      return FIO___MD2HTML_APPEND_LITERAL(renderer, "<em>");
    if (inline_event->event == FIO_MD_EVENT_LEAVE)
      return FIO___MD2HTML_APPEND_LITERAL(renderer, "</em>");
    return 0;
  case FIO_MD_INLINE_STRONG:
    if (inline_event->event == FIO_MD_EVENT_ENTER)
      return FIO___MD2HTML_APPEND_LITERAL(renderer, "<strong>");
    if (inline_event->event == FIO_MD_EVENT_LEAVE)
      return FIO___MD2HTML_APPEND_LITERAL(renderer, "</strong>");
    return 0;
  case FIO_MD_INLINE_STRIKETHROUGH:
    if (inline_event->event == FIO_MD_EVENT_ENTER)
      return FIO___MD2HTML_APPEND_LITERAL(renderer, "<del>");
    if (inline_event->event == FIO_MD_EVENT_LEAVE)
      return FIO___MD2HTML_APPEND_LITERAL(renderer, "</del>");
    return 0;
  case FIO_MD_INLINE_LINK:
    if (inline_event->event == FIO_MD_EVENT_ENTER)
      return FIO___MD2HTML_APPEND_LITERAL(renderer, "<a href=\"") ||
             fio___md2html_escape_reference_url(renderer,
                                                inline_event->destination,
                                                inline_event->reference) ||
             fio___md2html_append_title(renderer, inline_event->title) ||
             FIO___MD2HTML_APPEND_LITERAL(renderer, "\">");
    if (inline_event->event == FIO_MD_EVENT_LEAVE)
      return FIO___MD2HTML_APPEND_LITERAL(renderer, "</a>");
    return 0;
  case FIO_MD_INLINE_IMAGE:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<img src=\"") ||
           fio___md2html_escape_attr(renderer, inline_event->destination) ||
           FIO___MD2HTML_APPEND_LITERAL(renderer, "\" alt=\"") ||
           fio___md2html_escape_attr(renderer, inline_event->text) ||
           fio___md2html_append_title(renderer, inline_event->title) ||
           FIO___MD2HTML_APPEND_LITERAL(renderer, "\">");
  case FIO_MD_INLINE_AUTOLINK:
    return FIO___MD2HTML_APPEND_LITERAL(renderer, "<a href=\"") ||
           fio___md2html_escape_attr(renderer, inline_event->destination) ||
           FIO___MD2HTML_APPEND_LITERAL(renderer, "\">") ||
           fio___md2html_escape_text(renderer, inline_event->destination) ||
           FIO___MD2HTML_APPEND_LITERAL(renderer, "</a>");
  case FIO_MD_INLINE_HTML:
    return fio___md2html_append(renderer,
                                inline_event->source.buf,
                                inline_event->source.len);
  case FIO_MD_INLINE_ESCAPE:
    return fio___md2html_escape_text(renderer, inline_event->text);
  case FIO_MD_INLINE_ENTITY:
    return fio___md2html_append(renderer,
                                inline_event->source.buf,
                                inline_event->source.len);
  case FIO_MD_INLINE_FOOTNOTE_REF:
    return fio___md2html_append_footnote_ref(renderer, inline_event->reference);
  }
  return 0;
}

FIO_SFUNC void fio___md2html_error(void *udata, int err, size_t consumed) {
  fio___md2html_s *renderer = (fio___md2html_s *)udata;
  if (!renderer->err)
    renderer->err = err;
  renderer->consumed = consumed;
}

FIO_IFUNC int fio___md2html_ends_with(char *s,
                                      size_t len,
                                      const char *suffix,
                                      size_t suffix_len) {
  return len >= suffix_len &&
         FIO_MEMCMP(s + len - suffix_len, suffix, suffix_len) == 0;
}

FIO_SFUNC int fio___md2html_render_footnotes(fio___md2html_s *renderer) {
  if (!renderer->footnote_use_count || renderer->rendering_footnotes)
    return 0;
  renderer->rendering_footnotes = 1;
  if (FIO___MD2HTML_APPEND_LITERAL(renderer,
                                   "<section class=\"footnotes\" "
                                   "data-footnotes>\n<ol>\n"))
    return renderer->err;
  for (uint16_t idx = 1; idx <= renderer->footnote_use_count; ++idx) {
    fio___md2html_footnote_s *note = NULL;
    char *normalized = NULL;
    char *body = NULL;
    size_t body_len = 0;
    for (uint16_t i = 0; i < renderer->footnote_count; ++i) {
      if (renderer->footnotes[i].index == idx) {
        note = renderer->footnotes + i;
        break;
      }
    }
    if (!note)
      continue;
    if (FIO___MD2HTML_APPEND_LITERAL(renderer, "<li id=\"") ||
        fio___md2html_append_footnote_id(renderer, "fn-", note->label) ||
        FIO___MD2HTML_APPEND_LITERAL(renderer, "\">\n"))
      return renderer->err;
    normalized = fio___md2html_normalize_footnote(note);
    if (!normalized) {
      renderer->err = FIO_MD2HTML_ERR_ALLOC;
      return renderer->err;
    }
    body =
        fio_md2html(NULL, FIO_BUF_INFO2(normalized, fio_bstr_len(normalized)));
    fio_bstr_free(normalized);
    if (!body) {
      renderer->err = FIO_MD2HTML_ERR_ALLOC;
      return renderer->err;
    }
    body_len = fio_bstr_len(body);
    if (fio___md2html_ends_with(body, body_len, "</p>\n", 5)) {
      if (fio___md2html_append(renderer, body, body_len - 5) ||
          FIO___MD2HTML_APPEND_LITERAL(renderer, " ") ||
          fio___md2html_append_backrefs(renderer, note) ||
          FIO___MD2HTML_APPEND_LITERAL(renderer, "</p>\n")) {
        fio_bstr_free(body);
        return renderer->err;
      }
    } else {
      if (fio___md2html_append(renderer, body, body_len) ||
          fio___md2html_append_backrefs(renderer, note) ||
          FIO___MD2HTML_APPEND_LITERAL(renderer, "\n")) {
        fio_bstr_free(body);
        return renderer->err;
      }
    }
    fio_bstr_free(body);
    if (FIO___MD2HTML_APPEND_LITERAL(renderer, "</li>\n"))
      return renderer->err;
  }
  return FIO___MD2HTML_APPEND_LITERAL(renderer, "</ol>\n</section>\n");
}

SFUNC char *fio_md2html(char *bstr_target, fio_buf_info_s source) {
  fio___md2html_s renderer = {0};
  fio_md_callbacks_s callbacks = {
      .on_block_enter = fio___md2html_block_enter,
      .on_block_leave = fio___md2html_block_leave,
      .on_inline = fio___md2html_inline,
      .on_error = fio___md2html_error,
  };
  size_t consumed;
  renderer.html = fio_bstr_reserve(bstr_target, source.len ? source.len : 1);
  if (FIO_UNLIKELY(!renderer.html))
    return NULL;
  fio___md2html_scan_footnotes(&renderer, source);
  consumed = fio_md_parse(&callbacks, &renderer, source);
  if (!renderer.err && consumed == source.len)
    fio___md2html_render_footnotes(&renderer);
  if (FIO_UNLIKELY(renderer.err || consumed != source.len)) {
    if (!bstr_target)
      fio_bstr_free(renderer.html);
    return NULL;
  }
  return renderer.html;
}

#undef FIO___MD2HTML_APPEND_LITERAL

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_MD2HTML */
