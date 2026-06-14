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

/* Dependency: binary string buffer for output accumulation. */
#ifndef H___FIO_STR___H
#define FIO_STR
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
#endif

/* Dependency: GFM parser event source. */
#ifndef H___FIO_GFM___H
#define FIO_GFM
#define FIO___RECURSIVE_INCLUDE 1
#include FIO_INCLUDE_FILE
#undef FIO___RECURSIVE_INCLUDE
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

/* ---------------------------------------------------------------------------
 * Internal renderer state
 * ------------------------------------------------------------------------- */

typedef struct {
  char *bstr;             /* output buffer (fio_bstr) */
  uint32_t table_row;     /* 0 = header row, 1+ = body rows */
  uint32_t table_cell_depth; /* >0 iff rendering a table cell */
  uint32_t tight_depth;   /* >0 iff we're inside tight list items */
  uint32_t li_depth;      /* renderer LIST_ITEM stack depth */
  uint32_t para_suppress; /* >0 iff paragraphs are suppressed (tight) */
  uint32_t block_container_depth; /* nesting depth of block containers within tight LI */
  uint8_t in_code_block;  /* 1 iff inside <pre><code> */
  uint8_t in_html_block;  /* 1 iff inside HTML block */
  uint8_t code_has_line;  /* 1 iff at least one code line was emitted */
  uint8_t tight_child_pending; /* 1 iff a prior tight-LI child was emitted */
  uint8_t li_tight[128];  /* original tight state for LIST_ITEM pops */
  uint32_t li_block_base[128]; /* block_container_depth at LIST_ITEM push */
  int err;
} fio___md2html_renderer_s;

/* ---------------------------------------------------------------------------
 * Output helpers
 * ------------------------------------------------------------------------- */

FIO_IFUNC void fio___md2html_append(fio___md2html_renderer_s *r,
                                    const char *s,
                                    size_t n) {
  if (r->err || !n)
    return;
  r->bstr = fio_bstr_write(r->bstr, s, n);
  if (!r->bstr)
    r->err = FIO_MD2HTML_ERR_ALLOC;
}

#define FIO___MD2HTML_LIT(r, lit)                                              \
  fio___md2html_append((r), (lit), sizeof(lit) - 1)

FIO_IFUNC void fio___md2html_append_u(fio___md2html_renderer_s *r, uint32_t n) {
  char tmp[16];
  int len = snprintf(tmp, sizeof(tmp), "%u", n);
  fio___md2html_append(r, tmp, (size_t)len);
}

/** Escape text for HTML output: & < > " */
FIO_IFUNC void fio___md2html_escape(fio___md2html_renderer_s *r,
                                    const char *s,
                                    size_t n) {
  const char *mark = s;
  const char *end = s + n;
  while (s < end) {
    const char *entity = NULL;
    size_t elen = 0;
    switch (*s) {
    case '&': entity = "&amp;";  elen = 5; break;
    case '<': entity = "&lt;";   elen = 4; break;
    case '>': entity = "&gt;";   elen = 4; break;
    case '"': entity = "&quot;"; elen = 6; break;
    }
    if (entity) {
      fio___md2html_append(r, mark, (size_t)(s - mark));
      fio___md2html_append(r, entity, elen);
      mark = s + 1;
    }
    ++s;
  }
  fio___md2html_append(r, mark, (size_t)(end - mark));
}

/* ---------------------------------------------------------------------------
 * Tag filter (GFM disallows certain raw HTML tags)
 * ------------------------------------------------------------------------- */

FIO_IFUNC int fio___md2html_is_tagfilter_name(const char *s,
                                              size_t len,
                                              int html_block) {
  static const char *const names[] = {"iframe", "noembed", "noframes",
                                      "plaintext", "style", "textarea",
                                      "title", "xmp"};
  if (html_block) {
    return (len == 3 && (s[0] == 'x' || s[0] == 'X') &&
            (s[1] == 'm' || s[1] == 'M') && (s[2] == 'p' || s[2] == 'P')) ||
           (len == 8 && (s[0] == 't' || s[0] == 'T') &&
            (s[1] == 'e' || s[1] == 'E') && (s[2] == 'x' || s[2] == 'X') &&
            (s[3] == 't' || s[3] == 'T') && (s[4] == 'a' || s[4] == 'A') &&
            (s[5] == 'r' || s[5] == 'R') && (s[6] == 'e' || s[6] == 'E') &&
            (s[7] == 'a' || s[7] == 'A'));
  }
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
    const char *n = names[i];
    size_t j = 0;
    while (j < len && n[j]) {
      char c = s[j];
      if (c >= 'A' && c <= 'Z')
        c = (char)(c + ('a' - 'A'));
      if (c != n[j])
        break;
      ++j;
    }
    if (j == len && !n[j])
      return 1;
  }
  return 0;
}

FIO_IFUNC const char *fio___md2html_bracket_close(const char *p,
                                                  const char *end) {
  const char *s = p + 1;
  uint16_t depth = 0;
  while (s < end) {
    if (*s == '\\' && s + 1 < end) {
      s += 2;
      continue;
    }
    if (*s == '[') {
      ++depth;
    } else if (*s == ']') {
      if (!depth)
        return s;
      --depth;
    }
    ++s;
  }
  return NULL;
}

FIO_IFUNC const char *fio___md2html_link_like_end(const char *p,
                                                  const char *end) {
  p += (*p == '!' && p + 1 < end && p[1] == '[');
  if (p >= end || *p != '[')
    return NULL;
  const char *close = fio___md2html_bracket_close(p, end);
  if (!close)
    return NULL;
  const char *after = close + 1;
  if (after < end && *after == '(') {
    const char *q = after + 1;
    int pd = 1;
    while (q < end && pd) {
      if (*q == '\\' && q + 1 < end) {
        q += 2;
        continue;
      }
      pd += (*q == '(') - (*q == ')');
      ++q;
    }
    return pd ? NULL : q;
  }
  if (after < end && *after == '[') {
    const char *q = after + 1;
    while (q < end && *q != ']')
      q += (*q == '\\' && q + 1 < end) ? 2 : 1;
    return (q < end) ? q + 1 : NULL;
  }
  return NULL;
}

FIO_IFUNC int fio___md2html_range_has_link(const char *s, const char *end) {
  const char *start = s;
  while (s < end) {
    if (*s == '[' && !(s > start && s[-1] == '!')) {
      if (fio___md2html_link_like_end(s, end))
        return 1;
    }
    ++s;
  }
  return 0;
}

/** Escape image alt text: strip nested link/image markup and emphasis markers. */
FIO_IFUNC void fio___md2html_escape_alt_text(fio___md2html_renderer_s *r,
                                             const char *s,
                                             size_t n) {
  const char *end = s + n;
  while (s < end) {
    if ((*s == '[' || (*s == '!' && s + 1 < end && s[1] == '['))) {
      int is_image = (*s == '!');
      const char *bracket = s + is_image;
      const char *close = fio___md2html_bracket_close(bracket, end);
      const char *link_end = fio___md2html_link_like_end(s, end);
      if (close && link_end &&
          (is_image || !fio___md2html_range_has_link(bracket + 1, close))) {
        fio___md2html_escape_alt_text(r, bracket + 1,
                                      (size_t)(close - (bracket + 1)));
        s = link_end;
        continue;
      }
    }
    if (*s == '\\' && s + 1 < end) {
      fio___md2html_escape(r, s + 1, 1);
      s += 2;
      continue;
    }
    if (*s == '*' || *s == '_' || *s == '~') {
      ++s;
      continue;
    }
    fio___md2html_escape(r, s, 1);
    ++s;
  }
}

/** Emit raw HTML with GFM tagfilter applied. */
FIO_IFUNC void fio___md2html_append_tagfiltered(fio___md2html_renderer_s *r,
                                                const char *s,
                                                size_t n,
                                                int html_block) {
  const char *mark = s;
  const char *end = s + n;
  while (s < end) {
    if (*s == '<') {
      const char *name = s + 1;
      name += (name < end && *name == '/');
      const char *name_end = name;
      while (name_end < end &&
             ((*name_end >= 'a' && *name_end <= 'z') ||
              (*name_end >= 'A' && *name_end <= 'Z') ||
              (*name_end >= '0' && *name_end <= '9') || *name_end == '-'))
        ++name_end;
      if (name_end > name &&
          fio___md2html_is_tagfilter_name(name,
                                          (size_t)(name_end - name),
                                          html_block)) {
        fio___md2html_append(r, mark, (size_t)(s - mark));
        FIO___MD2HTML_LIT(r, "&lt;");
        mark = s + 1;
      }
    }
    ++s;
  }
  fio___md2html_append(r, mark, (size_t)(end - mark));
}

/* ---------------------------------------------------------------------------
 * URL / attribute escaping
 * ------------------------------------------------------------------------- */

/** Escape for URL attribute: percent-encode non-safe chars.
 *  Already-percent-encoded sequences (%XX) pass through. */
FIO_IFUNC void fio___md2html_escape_url(fio___md2html_renderer_s *r,
                                        const char *s,
                                        size_t n) {
  static const char hex[] = "0123456789ABCDEF";
  const char *end = s + n;
  while (s < end) {
    unsigned char c = (unsigned char)*s;
    if (c == '&') {
      FIO___MD2HTML_LIT(r, "&amp;");
    } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
               c == '~' || c == ':' || c == '/' || c == '?' || c == '#' ||
               c == '@' || c == '!' || c == '$' ||
               c == '\'' || c == '(' || c == ')' || c == '*' ||
               c == '+' || c == ',' || c == ';' || c == '=' || c == '%') {
      fio___md2html_append(r, s, 1);
    } else {
      char pct[3] = {'%', hex[c >> 4], hex[c & 0xf]};
      fio___md2html_append(r, pct, 3);
    }
    ++s;
  }
}

/** Escape for URL attribute, processing backslash-escaped markdown
 *  punctuation first (unescape \X -> X then URL-encode). */
FIO_IFUNC void fio___md2html_escape_md_url(fio___md2html_renderer_s *r,
                                           const char *s,
                                           size_t n) {
  const char *end = s + n;
  while (s < end) {
    if (*s == '&') {
      const char *semi = s + 1;
      while (semi < end && *semi != ';' && (semi - s) < 32)
        ++semi;
      if (semi < end && *semi == ';') {
        char decoded[8];
        size_t dlen = fio_entity(decoded, s, (size_t)(semi - s + 1));
        if (dlen) {
          fio___md2html_escape_url(r, decoded, dlen);
          s = semi + 1;
          continue;
        }
        if (semi - s == 5 &&
            (s[1] == 'a' || s[1] == 'A') &&
            (s[2] == 'u' || s[2] == 'U') &&
            (s[3] == 'm' || s[3] == 'M') &&
            (s[4] == 'l' || s[4] == 'L')) {
          fio___md2html_escape_url(r, "\xC3\xA4", 2);
          s = semi + 1;
          continue;
        }
      }
    }
    if (*s == '\\' && s + 1 < end) {
      char next = s[1];
      int is_punct =
          (next >= '!' && next <= '/') || (next >= ':' && next <= '@') ||
          (next >= '[' && next <= '`') || (next >= '{' && next <= '~');
      if (is_punct) {
        fio___md2html_escape_url(r, s + 1, 1);
        s += 2;
        continue;
      }
    }
    fio___md2html_escape_url(r, s, 1);
    ++s;
  }
}

/** Escape text that may contain backslash escapes (\* -> *). */
FIO_IFUNC void fio___md2html_escape_md_text(fio___md2html_renderer_s *r,
                                            const char *s,
                                            size_t n) {
  const char *mark = s;
  const char *end = s + n;
  while (s < end) {
    if (*s == '&') {
      const char *semi = s + 1;
      while (semi < end && *semi != ';' && (semi - s) < 32)
        ++semi;
      if (semi < end && *semi == ';') {
        char decoded[8];
        size_t dlen = fio_entity(decoded, s, (size_t)(semi - s + 1));
        if (dlen) {
          fio___md2html_append(r, mark, (size_t)(s - mark));
          fio___md2html_escape(r, decoded, dlen);
          s = semi + 1;
          mark = s;
          continue;
        }
      }
    }
    if (*s == '\\' && s + 1 < end) {
      char next = s[1];
      int is_punct =
          (next >= '!' && next <= '/') || (next >= ':' && next <= '@') ||
          (next >= '[' && next <= '`') || (next >= '{' && next <= '~');
      if (is_punct) {
        fio___md2html_append(r, mark, (size_t)(s - mark));
        fio___md2html_escape(r, s + 1, 1);
        s += 2;
        mark = s;
        continue;
      }
    }
    ++s;
  }
  fio___md2html_append(r, mark, (size_t)(end - mark));
}

/* ---------------------------------------------------------------------------
 * GFM event callbacks
 * ------------------------------------------------------------------------- */

FIO_IFUNC int fio___md2html_push(fio_gfm_event_s *e) {
  fio___md2html_renderer_s *r = (fio___md2html_renderer_s *)e->udata;
  if (r->err)
    return r->err;

  switch (e->type) {
  case FIO_GFM_PARAGRAPH:
    if (r->li_depth) {
      uint32_t li = r->li_depth - 1;
      if (li < sizeof(r->li_tight) && r->li_tight[li] &&
          r->block_container_depth == r->li_block_base[li]) {
        ++r->para_suppress;
        return 0;
      }
    }
    FIO___MD2HTML_LIT(r, "<p>");
    return 0;

  case FIO_GFM_HEADING:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      FIO___MD2HTML_LIT(r, "\n");
    FIO___MD2HTML_LIT(r, "<h");
    fio___md2html_append_u(r, e->heading_level);
    FIO___MD2HTML_LIT(r, ">");
    ++r->block_container_depth;
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_THEMATIC_BREAK:
    FIO___MD2HTML_LIT(r, "<hr />");
    return 0;

  case FIO_GFM_BLOCKQUOTE:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      FIO___MD2HTML_LIT(r, "\n");
    FIO___MD2HTML_LIT(r, "<blockquote>");
    ++r->block_container_depth;
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_LIST_UNORDERED:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      FIO___MD2HTML_LIT(r, "\n");
    FIO___MD2HTML_LIT(r, "<ul>");
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_LIST_ORDERED:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      FIO___MD2HTML_LIT(r, "\n");
    FIO___MD2HTML_LIT(r, "<ol");
    if (e->list_start != 1) {
      FIO___MD2HTML_LIT(r, " start=\"");
      fio___md2html_append_u(r, e->list_start);
      FIO___MD2HTML_LIT(r, "\"");
    }
    FIO___MD2HTML_LIT(r, ">");
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_LIST_ITEM: {
    uint8_t was_tight = !!(e->flags & FIO_GFM_F_TIGHT);
    if (r->li_depth < sizeof(r->li_tight)) {
      r->li_tight[r->li_depth] = was_tight;
      r->li_block_base[r->li_depth] = r->block_container_depth;
    }
    ++r->li_depth;
    if (was_tight)
      ++r->tight_depth;
    r->tight_child_pending = 0;
    FIO___MD2HTML_LIT(r, "<li>");
    if (e->flags & FIO_GFM_F_TASK) {
      FIO___MD2HTML_LIT(r, "<input type=\"checkbox\"");
      if (e->flags & FIO_GFM_F_TASK_CHECKED)
        FIO___MD2HTML_LIT(r, " checked=\"\"");
      FIO___MD2HTML_LIT(r, " disabled=\"\" /> ");
    }
    return 0;
  }

  case FIO_GFM_CODE_BLOCK:
    r->in_code_block = 1;
    r->code_has_line = 0;
    FIO___MD2HTML_LIT(r, "<pre><code");
    if (e->info.len) {
      const char *lang = e->info.buf;
      size_t lang_len = e->info.len;
      for (size_t i = 0; i < e->info.len; ++i) {
        if (e->info.buf[i] == ' ' || e->info.buf[i] == '\t') {
          lang_len = i;
          break;
        }
      }
      if (lang_len) {
        FIO___MD2HTML_LIT(r, " class=\"language-");
        fio___md2html_escape_md_text(r, lang, lang_len);
        FIO___MD2HTML_LIT(r, "\"");
      }
    }
    FIO___MD2HTML_LIT(r, ">");
    return 0;

  case FIO_GFM_HTML_BLOCK:
    r->in_html_block = 1;
    return 0;

  case FIO_GFM_TABLE:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      FIO___MD2HTML_LIT(r, "\n");
    r->table_row = 0;
    FIO___MD2HTML_LIT(r, "<table><thead>");
    ++r->block_container_depth;
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_TABLE_ROW:
    if (r->table_row == 1)
      FIO___MD2HTML_LIT(r, "<tbody>");
    FIO___MD2HTML_LIT(r, "<tr>");
    return 0;

  case FIO_GFM_TABLE_CELL:
    ++r->table_cell_depth;
    if (r->table_row == 0)
      FIO___MD2HTML_LIT(r, "<th");
    else
      FIO___MD2HTML_LIT(r, "<td");
    switch (e->align) {
    case FIO_GFM_ALIGN_LEFT:   FIO___MD2HTML_LIT(r, " align=\"left\"");   break;
    case FIO_GFM_ALIGN_RIGHT:  FIO___MD2HTML_LIT(r, " align=\"right\"");  break;
    case FIO_GFM_ALIGN_CENTER: FIO___MD2HTML_LIT(r, " align=\"center\""); break;
    default: break;
    }
    FIO___MD2HTML_LIT(r, ">");
    return 0;

  case FIO_GFM_EMPHASIS:
    FIO___MD2HTML_LIT(r, "<em>");
    return 0;

  case FIO_GFM_STRONG:
    FIO___MD2HTML_LIT(r, "<strong>");
    return 0;

  case FIO_GFM_STRIKETHROUGH:
    FIO___MD2HTML_LIT(r, "<del>");
    return 0;

  case FIO_GFM_LINK:
    FIO___MD2HTML_LIT(r, "<a href=\"");
    fio___md2html_escape_md_url(r, e->destination.buf, e->destination.len);
    FIO___MD2HTML_LIT(r, "\"");
    if (e->title.len) {
      FIO___MD2HTML_LIT(r, " title=\"");
      fio___md2html_escape_md_text(r, e->title.buf, e->title.len);
      FIO___MD2HTML_LIT(r, "\"");
    }
    FIO___MD2HTML_LIT(r, ">");
    return 0;

  default:
    return 0;
  }
}

FIO_IFUNC int fio___md2html_pop(fio_gfm_event_s *e) {
  fio___md2html_renderer_s *r = (fio___md2html_renderer_s *)e->udata;
  if (r->err)
    return r->err;

  switch (e->type) {
  case FIO_GFM_PARAGRAPH:
    if (r->para_suppress) {
      --r->para_suppress;
      return 0;
    }
    FIO___MD2HTML_LIT(r, "</p>");
    return 0;

  case FIO_GFM_HEADING:
    FIO___MD2HTML_LIT(r, "</h");
    fio___md2html_append_u(r, e->heading_level);
    FIO___MD2HTML_LIT(r, ">");
    if (r->block_container_depth)
      --r->block_container_depth;
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_THEMATIC_BREAK:
    return 0;

  case FIO_GFM_BLOCKQUOTE:
    FIO___MD2HTML_LIT(r, "</blockquote>");
    if (r->block_container_depth)
      --r->block_container_depth;
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_LIST_UNORDERED:
    FIO___MD2HTML_LIT(r, "</ul>");
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_LIST_ORDERED:
    FIO___MD2HTML_LIT(r, "</ol>");
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_LIST_ITEM: {
    uint8_t was_tight = 0;
    FIO___MD2HTML_LIT(r, "</li>");
    if (r->li_depth) {
      --r->li_depth;
      if (r->li_depth < sizeof(r->li_tight))
        was_tight = r->li_tight[r->li_depth];
    }
    if (was_tight && r->tight_depth)
      --r->tight_depth;
    r->tight_child_pending = 0;
    return 0;
  }

  case FIO_GFM_CODE_BLOCK:
    if (r->code_has_line)
      FIO___MD2HTML_LIT(r, "\n");
    FIO___MD2HTML_LIT(r, "</code></pre>");
    r->in_code_block = 0;
    r->code_has_line = 0;
    return 0;

  case FIO_GFM_HTML_BLOCK:
    r->in_html_block = 0;
    return 0;

  case FIO_GFM_TABLE:
    if (r->table_row > 1)
      FIO___MD2HTML_LIT(r, "</tbody>");
    FIO___MD2HTML_LIT(r, "</table>");
    if (r->block_container_depth)
      --r->block_container_depth;
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_TABLE_ROW:
    FIO___MD2HTML_LIT(r, "</tr>");
    if (r->table_row++ == 0)
      FIO___MD2HTML_LIT(r, "</thead>");
    return 0;

  case FIO_GFM_TABLE_CELL:
    if (r->table_row == 0)
      FIO___MD2HTML_LIT(r, "</th>");
    else
      FIO___MD2HTML_LIT(r, "</td>");
    if (r->table_cell_depth)
      --r->table_cell_depth;
    return 0;

  case FIO_GFM_EMPHASIS:
    FIO___MD2HTML_LIT(r, "</em>");
    return 0;

  case FIO_GFM_STRONG:
    FIO___MD2HTML_LIT(r, "</strong>");
    return 0;

  case FIO_GFM_STRIKETHROUGH:
    FIO___MD2HTML_LIT(r, "</del>");
    return 0;

  case FIO_GFM_LINK:
    FIO___MD2HTML_LIT(r, "</a>");
    return 0;

  default:
    return 0;
  }
}

FIO_IFUNC int fio___md2html_write(fio_gfm_event_s *e) {
  fio___md2html_renderer_s *r = (fio___md2html_renderer_s *)e->udata;
  if (r->err)
    return r->err;

  switch (e->type) {
  case FIO_GFM_TEXT:
    if (r->in_code_block) {
      if (r->code_has_line)
        FIO___MD2HTML_LIT(r, "\n");
      r->code_has_line = 1;
      if (e->padding) {
        char spaces[8] = {0};
        size_t n = e->padding < 8 ? e->padding : 8;
        memset(spaces, ' ', n);
        fio___md2html_append(r, spaces, n);
      }
      fio___md2html_escape(r, e->text.buf, e->text.len);
      return 0;
    }
    if (r->in_html_block) {
      if (e->text.len)
        fio___md2html_append_tagfiltered(r, e->text.buf, e->text.len, 1);
      FIO___MD2HTML_LIT(r, "\n");
      return 0;
    }
    if (r->li_depth) {
      uint32_t li = r->li_depth - 1;
      if (li < sizeof(r->li_tight) && r->li_tight[li] &&
          r->block_container_depth == r->li_block_base[li]) {
        if (r->tight_child_pending)
          FIO___MD2HTML_LIT(r, "\n");
        r->tight_child_pending = 1;
      }
    }
    fio___md2html_escape(r, e->text.buf, e->text.len);
    return 0;

  case FIO_GFM_SOFT_BREAK:
    FIO___MD2HTML_LIT(r, "\n");
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_HARD_BREAK:
    FIO___MD2HTML_LIT(r, "<br />\n");
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_CODE_SPAN: {
    FIO___MD2HTML_LIT(r, "<code>");
    const char *cs = e->text.buf;
    const char *ce = cs + e->text.len;
    const char *mark = cs;
    while (cs < ce) {
      if (r->table_cell_depth && *cs == '\\' && cs + 1 < ce && cs[1] == '|') {
        fio___md2html_escape(r, mark, (size_t)(cs - mark));
        fio___md2html_append(r, "|", 1);
        cs += 2;
        mark = cs;
        continue;
      }
      if (*cs == '\n' || *cs == '\r') {
        fio___md2html_escape(r, mark, (size_t)(cs - mark));
        fio___md2html_append(r, " ", 1);
        cs += (*cs == '\r' && cs + 1 < ce && cs[1] == '\n') ? 2 : 1;
        mark = cs;
      } else {
        ++cs;
      }
    }
    fio___md2html_escape(r, mark, (size_t)(ce - mark));
    FIO___MD2HTML_LIT(r, "</code>");
    return 0;
  }

  case FIO_GFM_IMAGE:
    FIO___MD2HTML_LIT(r, "<img src=\"");
    fio___md2html_escape_md_url(r, e->destination.buf, e->destination.len);
    FIO___MD2HTML_LIT(r, "\" alt=\"");
    fio___md2html_escape_alt_text(r, e->text.buf, e->text.len);
    FIO___MD2HTML_LIT(r, "\"");
    if (e->title.len) {
      FIO___MD2HTML_LIT(r, " title=\"");
      fio___md2html_escape_md_text(r, e->title.buf, e->title.len);
      FIO___MD2HTML_LIT(r, "\"");
    }
    FIO___MD2HTML_LIT(r, " />");
    return 0;

  case FIO_GFM_AUTOLINK: {
    int is_email = 0, has_colon = 0;
    for (size_t i = 0; i < e->destination.len; ++i) {
      is_email |= (e->destination.buf[i] == '@');
      has_colon |= (e->destination.buf[i] == ':');
    }
    FIO___MD2HTML_LIT(r, "<a href=\"");
    if (is_email && !has_colon)
      FIO___MD2HTML_LIT(r, "mailto:");
    else if (e->destination.len >= 4 &&
             (e->destination.buf[0] == 'w' || e->destination.buf[0] == 'W') &&
             (e->destination.buf[1] == 'w' || e->destination.buf[1] == 'W') &&
             (e->destination.buf[2] == 'w' || e->destination.buf[2] == 'W') &&
             e->destination.buf[3] == '.')
      FIO___MD2HTML_LIT(r, "http://");
    if (e->destination.len)
      fio___md2html_escape_url(r, e->destination.buf, e->destination.len);
    FIO___MD2HTML_LIT(r, "\">");
    if (e->text.len)
      fio___md2html_escape(r, e->text.buf, e->text.len);
    FIO___MD2HTML_LIT(r, "</a>");
    return 0;
  }

  case FIO_GFM_INLINE_HTML:
    if (e->text.len)
      fio___md2html_append_tagfiltered(r, e->text.buf, e->text.len, 0);
    return 0;

  default:
    return 0;
  }
}

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

SFUNC char *fio_md2html(char *bstr_target, fio_buf_info_s source) {
  fio___md2html_renderer_s renderer = {0};
  renderer.bstr = bstr_target;
  if (!renderer.bstr) {
    /* allocate storage for an empty bstr before parsing begins */
    renderer.bstr = fio_bstr_reserve(NULL, 1);
    if (!renderer.bstr)
      return NULL;
  }

  fio_gfm_callbacks_s cb = {
      .push = fio___md2html_push,
      .write = fio___md2html_write,
      .pop = fio___md2html_pop,
  };

  size_t consumed = fio_gfm_parse(&cb, &renderer, source);
  if (consumed != source.len || renderer.err) {
    if (!bstr_target)
      fio_bstr_free(renderer.bstr);
    return NULL;
  }
  return renderer.bstr;
}

#endif /* FIO_EXTERN_COMPLETE */
#endif /* H___FIO_MD2HTML___H */
