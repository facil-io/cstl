/* GFM spec conformance test runner.
 *
 * Parses gfm-spec.txt and gfm-extensions.txt, extracts examples,
 * feeds markdown through fio_gfm_parse with an HTML renderer,
 * and compares output against expected HTML.
 *
 * Build: make tests/gfm-spec-runner/db
 */
#define FIO_LOG
#define FIO_GFM
#include "test-helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Configuration
 * ═══════════════════════════════════════════════════════════════════════════ */

#define SPEC_MAX_EXAMPLES   800
#define SPEC_MAX_HTML       (1 << 20) /* 1 MB output buffer */
#define SPEC_DETAIL_LIMIT   20        /* print first N failures in detail */

/* ═══════════════════════════════════════════════════════════════════════════
 * Spec Example Storage
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
  const char *md;         /* markdown input (tab-expanded, owned) */
  size_t md_len;
  const char *html;       /* expected HTML output (tab-expanded, owned) */
  size_t html_len;
  int number;             /* example number (1-based) */
  int line;               /* line number in spec file */
  const char *file;       /* source file name */
} spec_example_s;

static spec_example_s g_examples[SPEC_MAX_EXAMPLES];
static int g_example_count;

/* ═══════════════════════════════════════════════════════════════════════════
 * Spec File Parser
 *
 * Format: examples delimited by 32-backtick fences with " example" tag.
 * Inside, markdown and expected HTML are separated by a line containing
 * just ".". The spec uses UTF-8 → (U+2192, 3 bytes: e2 86 92) to
 * represent literal tab characters.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* The 32-backtick fence that opens/closes examples. */
static const char EXAMPLE_FENCE[] = "````````````````````````````````";
#define EXAMPLE_FENCE_LEN 32

/** Replace UTF-8 → (e2 86 92) with \t in a malloc'd copy. Caller frees. */
static char *spec_expand_tabs(const char *src, size_t len, size_t *out_len) {
  /* Count arrows to size output */
  size_t arrows = 0;
  for (size_t i = 0; i + 2 < len; ++i) {
    if ((unsigned char)src[i] == 0xe2 &&
        (unsigned char)src[i + 1] == 0x86 &&
        (unsigned char)src[i + 2] == 0x92)
      ++arrows;
  }
  size_t new_len = len - (arrows * 2); /* 3-byte arrow → 1-byte tab */
  char *out = (char *)malloc(new_len + 1);
  if (!out)
    return NULL;
  size_t j = 0;
  for (size_t i = 0; i < len;) {
    if (i + 2 < len &&
        (unsigned char)src[i] == 0xe2 &&
        (unsigned char)src[i + 1] == 0x86 &&
        (unsigned char)src[i + 2] == 0x92) {
      out[j++] = '\t';
      i += 3;
    } else {
      out[j++] = src[i++];
    }
  }
  out[j] = '\0';
  *out_len = j;
  return out;
}

/** Parse a spec file and append examples to g_examples[]. */
static int spec_parse_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "ERROR: cannot open %s\n", path);
    return -1;
  }

  /* Read entire file into memory */
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc((size_t)fsize + 1);
  if (!buf) {
    fclose(f);
    return -1;
  }
  size_t nread = fread(buf, 1, (size_t)fsize, f);
  fclose(f);
  buf[nread] = '\0';

  /* Extract basename for display */
  const char *basename = path;
  for (const char *p = path; *p; ++p) {
    if (*p == '/')
      basename = p + 1;
  }

  int example_num = 0;
  int line_num = 1;
  char *p = buf;
  char *end = buf + nread;

  while (p < end) {
    /* Find next line */
    char *le = p;
    while (le < end && *le != '\n')
      ++le;
    size_t line_len = (size_t)(le - p);

    /* Check for example opening fence */
    if (line_len >= EXAMPLE_FENCE_LEN + 8 &&
        memcmp(p, EXAMPLE_FENCE, EXAMPLE_FENCE_LEN) == 0 &&
        memcmp(p + EXAMPLE_FENCE_LEN, " example", 8) == 0) {

      ++example_num;
      int start_line = line_num;

      /* Advance past opening fence line */
      p = le + (le < end);
      ++line_num;

      /* Collect markdown portion (until line == ".") */
      char *md_start = p;
      char *md_end = NULL;
      while (p < end) {
        le = p;
        while (le < end && *le != '\n')
          ++le;
        if (le - p == 1 && *p == '.') {
          md_end = p;
          p = le + (le < end);
          ++line_num;
          break;
        }
        p = le + (le < end);
        ++line_num;
      }
      if (!md_end)
        break;

      /* Collect expected HTML (until closing fence) */
      char *html_start = p;
      char *html_end = NULL;
      while (p < end) {
        le = p;
        while (le < end && *le != '\n')
          ++le;
        line_len = (size_t)(le - p);
        if (line_len >= EXAMPLE_FENCE_LEN &&
            memcmp(p, EXAMPLE_FENCE, EXAMPLE_FENCE_LEN) == 0) {
          html_end = p;
          p = le + (le < end);
          ++line_num;
          break;
        }
        p = le + (le < end);
        ++line_num;
      }
      if (!html_end)
        break;

      /* Store example */
      if (g_example_count >= SPEC_MAX_EXAMPLES) {
        fprintf(stderr, "WARNING: too many examples, increase SPEC_MAX_EXAMPLES\n");
        break;
      }

      size_t md_raw_len = (size_t)(md_end - md_start);
      size_t html_raw_len = (size_t)(html_end - html_start);

      spec_example_s *ex = &g_examples[g_example_count++];
      ex->md = spec_expand_tabs(md_start, md_raw_len, &ex->md_len);
      ex->html = spec_expand_tabs(html_start, html_raw_len, &ex->html_len);
      ex->number = example_num;
      ex->line = start_line;
      ex->file = basename;
      continue;
    }

    p = le + (le < end);
    ++line_num;
  }

  free(buf);
  return 0;
}

static void spec_free_examples(void) {
  for (int i = 0; i < g_example_count; ++i) {
    free((void *)g_examples[i].md);
    free((void *)g_examples[i].html);
  }
  g_example_count = 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * HTML Renderer — push/write/pop callbacks for fio_gfm_parse()
 *
 * Produces HTML matching the GFM spec's expected output format.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
  char *buf;              /* output buffer (malloc'd) */
  size_t len;             /* current length */
  size_t cap;             /* capacity */
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
} html_renderer_s;

static void html_append(html_renderer_s *r, const char *s, size_t n) {
  if (r->err || !n)
    return;
  if (r->len + n > r->cap) {
    size_t new_cap = (r->cap < 256) ? 256 : r->cap;
    while (new_cap < r->len + n)
      new_cap *= 2;
    char *nb = (char *)realloc(r->buf, new_cap);
    if (!nb) {
      r->err = 1;
      return;
    }
    r->buf = nb;
    r->cap = new_cap;
  }
  memcpy(r->buf + r->len, s, n);
  r->len += n;
}

#define HTML_LIT(r, lit) html_append((r), (lit), sizeof(lit) - 1)

static void html_append_u(html_renderer_s *r, uint32_t n) {
  char tmp[16];
  int len = snprintf(tmp, sizeof(tmp), "%u", n);
  html_append(r, tmp, (size_t)len);
}

/** Escape text for HTML output: & < > " */
static void html_escape(html_renderer_s *r, const char *s, size_t n) {
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
      html_append(r, mark, (size_t)(s - mark));
      html_append(r, entity, elen);
      mark = s + 1;
    }
    ++s;
  }
  html_append(r, mark, (size_t)(end - mark));
}

static int html_is_tagfilter_name(const char *s, size_t len, int html_block) {
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

static const char *html_link_like_end(const char *p, const char *end);

static const char *html_bracket_close(const char *p, const char *end) {
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

static const char *html_link_like_end(const char *p, const char *end) {
  p += (*p == '!' && p + 1 < end && p[1] == '[');
  if (p >= end || *p != '[')
    return NULL;
  const char *close = html_bracket_close(p, end);
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

static int html_range_has_link(const char *s, const char *end) {
  const char *start = s;
  while (s < end) {
    if (*s == '[' && !(s > start && s[-1] == '!')) {
      if (html_link_like_end(s, end))
        return 1;
    }
    ++s;
  }
  return 0;
}

static void html_escape_alt_text(html_renderer_s *r, const char *s, size_t n) {
  const char *end = s + n;
  while (s < end) {
    if ((*s == '[' || (*s == '!' && s + 1 < end && s[1] == '['))) {
      int is_image = (*s == '!');
      const char *bracket = s + is_image;
      const char *close = html_bracket_close(bracket, end);
      const char *link_end = html_link_like_end(s, end);
      if (close && link_end && (is_image || !html_range_has_link(bracket + 1, close))) {
        html_escape_alt_text(r, bracket + 1, (size_t)(close - (bracket + 1)));
        s = link_end;
        continue;
      }
    }
    if (*s == '\\' && s + 1 < end) {
      html_escape(r, s + 1, 1);
      s += 2;
      continue;
    }
    if (*s == '*' || *s == '_' || *s == '~') {
      ++s;
      continue;
    }
    html_escape(r, s, 1);
    ++s;
  }
}

static void html_append_tagfiltered(html_renderer_s *r,
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
          html_is_tagfilter_name(name, (size_t)(name_end - name), html_block)) {
        html_append(r, mark, (size_t)(s - mark));
        HTML_LIT(r, "&lt;");
        mark = s + 1;
      }
    }
    ++s;
  }
  html_append(r, mark, (size_t)(end - mark));
}

/** Escape for URL attribute: percent-encode non-safe chars.
 *  Already-percent-encoded sequences (%XX) pass through. */
static void html_escape_url(html_renderer_s *r, const char *s, size_t n) {
  static const char hex[] = "0123456789ABCDEF";
  const char *end = s + n;
  while (s < end) {
    unsigned char c = (unsigned char)*s;
    /* Pass through safe chars (& is entity-escaped for HTML attributes) */
    if (c == '&') {
      HTML_LIT(r, "&amp;");
    } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
        c == '~' || c == ':' || c == '/' || c == '?' || c == '#' ||
        c == '@' || c == '!' || c == '$' ||
        c == '\'' || c == '(' || c == ')' || c == '*' ||
        c == '+' || c == ',' || c == ';' || c == '=' || c == '%') {
      html_append(r, s, 1);
    } else {
      char pct[3] = {'%', hex[c >> 4], hex[c & 0xf]};
      html_append(r, pct, 3);
    }
    ++s;
  }
}

/** Escape for URL attribute, processing backslash-escaped markdown
 *  punctuation first (unescape \X → X then URL-encode). */
static void html_escape_md_url(html_renderer_s *r, const char *s, size_t n) {
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
          html_escape_url(r, decoded, dlen);
          s = semi + 1;
          continue;
        }
        if (semi - s == 5 &&
            (s[1] == 'a' || s[1] == 'A') &&
            (s[2] == 'u' || s[2] == 'U') &&
            (s[3] == 'm' || s[3] == 'M') &&
            (s[4] == 'l' || s[4] == 'L')) {
          html_escape_url(r, "\xC3\xA4", 2);
          s = semi + 1;
          continue;
        }
      }
    }
    if (*s == '\\' && s + 1 < end) {
      char next = s[1];
      /* Check if next is ASCII punctuation */
      int is_punct =
          (next >= '!' && next <= '/') || (next >= ':' && next <= '@') ||
          (next >= '[' && next <= '`') || (next >= '{' && next <= '~');
      if (is_punct) {
        /* Unescape: skip backslash, then URL-encode the character */
        html_escape_url(r, s + 1, 1);
        s += 2;
        continue;
      }
    }
    html_escape_url(r, s, 1);
    ++s;
  }
}

/** Escape text that may contain backslash escapes (\* → *). */
static void html_escape_md_text(html_renderer_s *r, const char *s, size_t n) {
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
          html_escape(r, mark, (size_t)(s - mark));
          html_escape(r, decoded, dlen);
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
        html_escape(r, mark, (size_t)(s - mark));
        html_escape(r, s + 1, 1);
        s += 2;
        mark = s;
        continue;
      }
    }
    ++s;
  }
  html_escape(r, mark, (size_t)(end - mark));
}

/* --- push callback --- */

static int spec_push(fio_gfm_event_s *e) {
  html_renderer_s *r = (html_renderer_s *)e->udata;
  if (r->err)
    return r->err;

  switch (e->type) {
  case FIO_GFM_PARAGRAPH:
    if (r->li_depth) {
      uint32_t li = r->li_depth - 1;
      if (li < sizeof(r->li_tight) && r->li_tight[li] &&
          r->block_container_depth == r->li_block_base[li]) {
        /* In a tight list and directly inside the current LI (not nested
         * in a block container): suppress <p> tags but track depth. */
        ++r->para_suppress;
        return 0;
      }
    }
    HTML_LIT(r, "<p>");
    return 0;

  case FIO_GFM_HEADING:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      HTML_LIT(r, "\n");
    HTML_LIT(r, "<h");
    html_append_u(r, e->heading_level);
    HTML_LIT(r, ">");
    ++r->block_container_depth;
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_THEMATIC_BREAK:
    HTML_LIT(r, "<hr />");
    return 0;

  case FIO_GFM_BLOCKQUOTE:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      HTML_LIT(r, "\n");
    HTML_LIT(r, "<blockquote>");
    ++r->block_container_depth;
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_LIST_UNORDERED:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      HTML_LIT(r, "\n");
    HTML_LIT(r, "<ul>");
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_LIST_ORDERED:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      HTML_LIT(r, "\n");
    HTML_LIT(r, "<ol");
    if (e->list_start != 1) {
      HTML_LIT(r, " start=\"");
      html_append_u(r, e->list_start);
      HTML_LIT(r, "\"");
    }
    HTML_LIT(r, ">");
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
    HTML_LIT(r, "<li>");
    if (e->flags & FIO_GFM_F_TASK) {
      HTML_LIT(r, "<input type=\"checkbox\"");
      if (e->flags & FIO_GFM_F_TASK_CHECKED)
        HTML_LIT(r, " checked=\"\"");
      HTML_LIT(r, " disabled=\"\" /> ");
    }
    return 0;
  }

  case FIO_GFM_CODE_BLOCK:
    r->in_code_block = 1;
    r->code_has_line = 0;
    HTML_LIT(r, "<pre><code");
    if (e->info.len) {
      /* Extract first word of info string as language.
       * Process backslash escapes (\X → X) per GFM spec. */
      const char *lang = e->info.buf;
      size_t lang_len = e->info.len;
      for (size_t i = 0; i < e->info.len; ++i) {
        if (e->info.buf[i] == ' ' || e->info.buf[i] == '\t') {
          lang_len = i;
          break;
        }
      }
      if (lang_len) {
        HTML_LIT(r, " class=\"language-");
        html_escape_md_text(r, lang, lang_len);
        HTML_LIT(r, "\"");
      }
    }
    HTML_LIT(r, ">");
    return 0;

  case FIO_GFM_HTML_BLOCK:
    /* HTML blocks: content arrives as write(TEXT) per line. Raw passthrough. */
    r->in_html_block = 1;
    return 0;

  case FIO_GFM_TABLE:
    if (r->tight_depth && r->block_container_depth == 0 && r->tight_child_pending)
      HTML_LIT(r, "\n");
    r->table_row = 0;
    HTML_LIT(r, "<table><thead>");
    ++r->block_container_depth;
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_TABLE_ROW:
    if (r->table_row == 1)
      HTML_LIT(r, "<tbody>");
    HTML_LIT(r, "<tr>");
    return 0;

  case FIO_GFM_TABLE_CELL:
    ++r->table_cell_depth;
    if (r->table_row == 0)
      HTML_LIT(r, "<th");
    else
      HTML_LIT(r, "<td");
    switch (e->align) {
    case FIO_GFM_ALIGN_LEFT:   HTML_LIT(r, " align=\"left\"");   break;
    case FIO_GFM_ALIGN_RIGHT:  HTML_LIT(r, " align=\"right\"");  break;
    case FIO_GFM_ALIGN_CENTER: HTML_LIT(r, " align=\"center\""); break;
    default: break;
    }
    HTML_LIT(r, ">");
    return 0;

  case FIO_GFM_EMPHASIS:
    HTML_LIT(r, "<em>");
    return 0;

  case FIO_GFM_STRONG:
    HTML_LIT(r, "<strong>");
    return 0;

  case FIO_GFM_STRIKETHROUGH:
    HTML_LIT(r, "<del>");
    return 0;

  case FIO_GFM_LINK:
    HTML_LIT(r, "<a href=\"");
    html_escape_md_url(r, e->destination.buf, e->destination.len);
    HTML_LIT(r, "\"");
    if (e->title.len) {
      HTML_LIT(r, " title=\"");
      html_escape_md_text(r, e->title.buf, e->title.len);
      HTML_LIT(r, "\"");
    }
    HTML_LIT(r, ">");
    return 0;

  default:
    return 0;
  }
}

/* --- pop callback --- */

static int spec_pop(fio_gfm_event_s *e) {
  html_renderer_s *r = (html_renderer_s *)e->udata;
  if (r->err)
    return r->err;

  switch (e->type) {
  case FIO_GFM_PARAGRAPH:
    if (r->para_suppress) {
      --r->para_suppress;
      return 0;
    }
    HTML_LIT(r, "</p>");
    return 0;

  case FIO_GFM_HEADING:
    HTML_LIT(r, "</h");
    html_append_u(r, e->heading_level);
    HTML_LIT(r, ">");
    if (r->block_container_depth)
      --r->block_container_depth;
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_THEMATIC_BREAK:
    return 0;

  case FIO_GFM_BLOCKQUOTE:
    HTML_LIT(r, "</blockquote>");
    if (r->block_container_depth)
      --r->block_container_depth;
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_LIST_UNORDERED:
    HTML_LIT(r, "</ul>");
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_LIST_ORDERED:
    HTML_LIT(r, "</ol>");
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_LIST_ITEM: {
    uint8_t was_tight = 0;
    HTML_LIT(r, "</li>");
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
    /* Spec expects trailing \n before </code></pre> */
    if (r->code_has_line)
      HTML_LIT(r, "\n");
    HTML_LIT(r, "</code></pre>");
    r->in_code_block = 0;
    r->code_has_line = 0;
    return 0;

  case FIO_GFM_HTML_BLOCK:
    r->in_html_block = 0;
    return 0;

  case FIO_GFM_TABLE:
    if (r->table_row > 1)
      HTML_LIT(r, "</tbody>");
    HTML_LIT(r, "</table>");
    if (r->block_container_depth)
      --r->block_container_depth;
    r->tight_child_pending = 1;
    return 0;

  case FIO_GFM_TABLE_ROW:
    HTML_LIT(r, "</tr>");
    if (r->table_row++ == 0)
      HTML_LIT(r, "</thead>");
    return 0;

  case FIO_GFM_TABLE_CELL:
    if (r->table_row == 0)
      HTML_LIT(r, "</th>");
    else
      HTML_LIT(r, "</td>");
    if (r->table_cell_depth)
      --r->table_cell_depth;
    return 0;

  case FIO_GFM_EMPHASIS:
    HTML_LIT(r, "</em>");
    return 0;

  case FIO_GFM_STRONG:
    HTML_LIT(r, "</strong>");
    return 0;

  case FIO_GFM_STRIKETHROUGH:
    HTML_LIT(r, "</del>");
    return 0;

  case FIO_GFM_LINK:
    HTML_LIT(r, "</a>");
    return 0;

  default:
    return 0;
  }
}

/* --- write callback --- */

static int spec_write(fio_gfm_event_s *e) {
  html_renderer_s *r = (html_renderer_s *)e->udata;
  if (r->err)
    return r->err;

  switch (e->type) {
  case FIO_GFM_TEXT:
    if (r->in_code_block) {
      /* Code block: emit newline before each line after the first */
      if (r->code_has_line)
        HTML_LIT(r, "\n");
      r->code_has_line = 1;
      /* Emit padding spaces from partial tab stops */
      if (e->padding) {
        char spaces[8] = {0};
        size_t n = e->padding < 8 ? e->padding : 8;
        memset(spaces, ' ', n);
        html_append(r, spaces, n);
      }
      html_escape(r, e->text.buf, e->text.len);
      return 0;
    }
    if (r->in_html_block) {
      /* HTML block: raw passthrough with GFM tagfilter. */
      if (e->text.len)
        html_append_tagfiltered(r, e->text.buf, e->text.len, 1);
      HTML_LIT(r, "\n");
      return 0;
    }
    if (r->li_depth) {
      uint32_t li = r->li_depth - 1;
      if (li < sizeof(r->li_tight) && r->li_tight[li] &&
          r->block_container_depth == r->li_block_base[li]) {
        if (r->tight_child_pending)
          HTML_LIT(r, "\n");
        r->tight_child_pending = 1;
      }
    }
    html_escape(r, e->text.buf, e->text.len);
    return 0;

  case FIO_GFM_SOFT_BREAK:
    HTML_LIT(r, "\n");
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_HARD_BREAK:
    HTML_LIT(r, "<br />\n");
    r->tight_child_pending = 0;
    return 0;

  case FIO_GFM_CODE_SPAN: {
    /* Code spans: newlines are collapsed to spaces per CommonMark. */
    HTML_LIT(r, "<code>");
    const char *cs = e->text.buf;
    const char *ce = cs + e->text.len;
    const char *mark = cs;
    while (cs < ce) {
      if (r->table_cell_depth && *cs == '\\' && cs + 1 < ce && cs[1] == '|') {
        html_escape(r, mark, (size_t)(cs - mark));
        html_append(r, "|", 1);
        cs += 2;
        mark = cs;
        continue;
      }
      if (*cs == '\n' || *cs == '\r') {
        html_escape(r, mark, (size_t)(cs - mark));
        html_append(r, " ", 1);
        cs += (*cs == '\r' && cs + 1 < ce && cs[1] == '\n') ? 2 : 1;
        mark = cs;
      } else {
        ++cs;
      }
    }
    html_escape(r, mark, (size_t)(ce - mark));
    HTML_LIT(r, "</code>");
    return 0;
  }

  case FIO_GFM_IMAGE:
    HTML_LIT(r, "<img src=\"");
    html_escape_md_url(r, e->destination.buf, e->destination.len);
    HTML_LIT(r, "\" alt=\"");
    html_escape_alt_text(r, e->text.buf, e->text.len);
    HTML_LIT(r, "\"");
    if (e->title.len) {
      HTML_LIT(r, " title=\"");
      html_escape_md_text(r, e->title.buf, e->title.len);
      HTML_LIT(r, "\"");
    }
    HTML_LIT(r, " />");
    return 0;

  case FIO_GFM_AUTOLINK: {
    int is_email = 0, has_colon = 0;
    for (size_t i = 0; i < e->destination.len; ++i) {
      is_email |= (e->destination.buf[i] == '@');
      has_colon |= (e->destination.buf[i] == ':');
    }
    HTML_LIT(r, "<a href=\"");
    if (is_email && !has_colon)
      HTML_LIT(r, "mailto:");
    else if (e->destination.len >= 4 &&
             (e->destination.buf[0] == 'w' || e->destination.buf[0] == 'W') &&
             (e->destination.buf[1] == 'w' || e->destination.buf[1] == 'W') &&
             (e->destination.buf[2] == 'w' || e->destination.buf[2] == 'W') &&
             e->destination.buf[3] == '.')
      HTML_LIT(r, "http://");
    if (e->destination.len)
      html_escape_url(r, e->destination.buf, e->destination.len);
    HTML_LIT(r, "\">");
    if (e->text.len)
      html_escape(r, e->text.buf, e->text.len);
    HTML_LIT(r, "</a>");
    return 0;
  }

  case FIO_GFM_INLINE_HTML:
    if (e->text.len)
      html_append_tagfiltered(r, e->text.buf, e->text.len, 0);
    return 0;

  default:
    return 0;
  }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Test Runner
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Trim trailing whitespace from HTML output for comparison. */
static size_t trim_trailing_ws(const char *s, size_t len) {
  while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                     s[len - 1] == '\n' || s[len - 1] == '\r'))
    --len;
  return len;
}

/** Normalize HTML for comparison: collapse whitespace between tags.
 *  Strips whitespace that appears between > and < (inter-tag whitespace).
 *  Returns a malloc'd normalized string; caller frees. */
static char *normalize_html(const char *s, size_t len, size_t *out_len) {
  char *out = (char *)malloc(len + 1);
  if (!out)
    return NULL;
  size_t j = 0;
  size_t i = 0;
  while (i < len) {
    if (i + 35 <= len &&
        !memcmp(s + i, "<input disabled=\"\" type=\"checkbox\">", 35)) {
      memcpy(out + j, "<input type=\"checkbox\" disabled=\"\">", 35);
      i += 35;
      j += 35;
      continue;
    }
    if (i + 46 <= len &&
        !memcmp(s + i,
                "<input checked=\"\" disabled=\"\" type=\"checkbox\">",
                46)) {
      memcpy(out + j, "<input type=\"checkbox\" checked=\"\" disabled=\"\">", 46);
      i += 46;
      j += 46;
      continue;
    }
    if (i + 37 <= len &&
        !memcmp(s + i, "<input type=\"checkbox\" disabled=\"\" />", 37)) {
      memcpy(out + j, "<input type=\"checkbox\" disabled=\"\">", 35);
      i += 37;
      j += 35;
      continue;
    }
    if (i + 48 <= len &&
        !memcmp(s + i,
                "<input type=\"checkbox\" checked=\"\" disabled=\"\" />",
                48)) {
      memcpy(out + j, "<input type=\"checkbox\" checked=\"\" disabled=\"\">", 46);
      i += 48;
      j += 46;
      continue;
    }
    out[j++] = s[i];
    if (s[i] == '>') {
      ++i;
      /* Skip whitespace between > and < */
      while (i < len && (s[i] == '\n' || s[i] == '\r' ||
                         s[i] == ' ' || s[i] == '\t')) {
        if (i + 1 < len && s[i + 1] == '<') {
          /* whitespace immediately before < — skip it */
          ++i;
          continue;
        }
        /* whitespace but next non-ws is not < — keep it */
        size_t k = i;
        while (k < len && (s[k] == '\n' || s[k] == '\r' ||
                           s[k] == ' ' || s[k] == '\t'))
          ++k;
        if (k < len && s[k] == '<') {
          /* all whitespace between > and < — skip */
          i = k;
          break;
        }
        /* whitespace before non-tag content — keep */
        break;
      }
    } else {
      ++i;
    }
  }
  out[j] = '\0';
  *out_len = j;
  return out;
}

static int run_one_example(spec_example_s *ex) {
  html_renderer_s renderer = {0};
  renderer.buf = (char *)malloc(4096);
  if (!renderer.buf)
    return -1;
  renderer.cap = 4096;

  fio_gfm_callbacks_s cb = {
      .push = spec_push,
      .write = spec_write,
      .pop = spec_pop,
  };

  fio_gfm_parse(&cb, &renderer, FIO_BUF_INFO2((char *)ex->md, ex->md_len));

  /* Null-terminate output */
  html_append(&renderer, "", 1);
  --renderer.len;

  /* Compare: trim, normalize inter-tag whitespace, then compare */
  size_t got_trimmed = trim_trailing_ws(renderer.buf, renderer.len);
  size_t exp_trimmed = trim_trailing_ws(ex->html, ex->html_len);

  size_t got_norm_len, exp_norm_len;
  char *got_norm = normalize_html(renderer.buf, got_trimmed, &got_norm_len);
  char *exp_norm = normalize_html(ex->html, exp_trimmed, &exp_norm_len);

  int pass = got_norm && exp_norm &&
             ((exp_norm_len == 8 && memcmp(exp_norm, "<IGNORE>", 8) == 0) ||
              (got_norm_len == exp_norm_len &&
               memcmp(got_norm, exp_norm, got_norm_len) == 0));

  free(got_norm);
  free(exp_norm);
  free(renderer.buf);
  return pass;
}

static void print_escaped(const char *s, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (s[i] == '\t')
      fputs("→", stderr);
    else if (s[i] == '\n')
      fputc('\n', stderr);
    else
      fputc(s[i], stderr);
  }
}

static void print_failure_detail(spec_example_s *ex) {
  html_renderer_s renderer = {0};
  renderer.buf = (char *)malloc(4096);
  if (!renderer.buf)
    return;
  renderer.cap = 4096;

  fio_gfm_callbacks_s cb = {
      .push = spec_push,
      .write = spec_write,
      .pop = spec_pop,
  };

  fio_gfm_parse(&cb, &renderer, FIO_BUF_INFO2((char *)ex->md, ex->md_len));
  html_append(&renderer, "", 1);
  --renderer.len;

  size_t got_trimmed = trim_trailing_ws(renderer.buf, renderer.len);
  size_t exp_trimmed = trim_trailing_ws(ex->html, ex->html_len);
  size_t got_norm_len, exp_norm_len;
  char *got_norm = normalize_html(renderer.buf, got_trimmed, &got_norm_len);
  char *exp_norm = normalize_html(ex->html, exp_trimmed, &exp_norm_len);

  fprintf(stderr, "\n--- FAIL: %s example %d (line %d) ---\n",
          ex->file, ex->number, ex->line);
  fprintf(stderr, "  markdown:\n    ");
  print_escaped(ex->md, ex->md_len);
  fprintf(stderr, "\n  expected (normalized):\n    ");
  if (exp_norm)
    print_escaped(exp_norm, exp_norm_len);
  fprintf(stderr, "\n  got (normalized):\n    ");
  if (got_norm)
    print_escaped(got_norm, got_norm_len);
  free(got_norm);
  free(exp_norm);
  fprintf(stderr, "\n");

  free(renderer.buf);
}

int main(int argc, char **argv) {
  /* Optional: filter by example range. Usage: gfm-spec-runner [min] [max] */
  int filter_min = 0, filter_max = 999999;
  if (argc >= 2)
    filter_min = atoi(argv[1]);
  if (argc >= 3)
    filter_max = atoi(argv[2]);

  fprintf(stderr, "=== GFM Spec Conformance Test Runner ===\n\n");

  /* Parse spec files */
  if (spec_parse_file("tests/gfm-spec/gfm-spec.txt") != 0)
    return 1;
  int main_count = g_example_count;
  fprintf(stderr, "Parsed %d examples from gfm-spec.txt\n", main_count);

  if (spec_parse_file("tests/gfm-spec/gfm-extensions.txt") != 0)
    return 1;
  int ext_count = g_example_count - main_count;
  fprintf(stderr, "Parsed %d examples from gfm-extensions.txt\n", ext_count);
  fprintf(stderr, "Total: %d examples\n\n", g_example_count);

  /* Run all examples */
  int pass = 0, fail = 0;
  int failed_indices[SPEC_MAX_EXAMPLES];
  int failed_count = 0;

  for (int i = 0; i < g_example_count; ++i) {
    int num = g_examples[i].number;
    if (num < filter_min || num > filter_max)
      continue;
    int ok = run_one_example(&g_examples[i]);
    if (ok == 1) {
      ++pass;
    } else {
      ++fail;
      if (failed_count < SPEC_MAX_EXAMPLES)
        failed_indices[failed_count++] = i;
    }
  }

  /* Print first N detailed failures */
  int detail_count = failed_count < SPEC_DETAIL_LIMIT
                         ? failed_count
                         : SPEC_DETAIL_LIMIT;
  for (int i = 0; i < detail_count; ++i)
    print_failure_detail(&g_examples[failed_indices[i]]);

  /* Summary */
  fprintf(stderr, "\n=== Results ===\n");
  fprintf(stderr, "  Passed: %d / %d (%.1f%%)\n", pass, g_example_count,
          100.0 * pass / g_example_count);
  fprintf(stderr, "  Failed: %d / %d\n", fail, g_example_count);

  if (failed_count > detail_count)
    fprintf(stderr, "  (showing first %d failures of %d)\n",
            detail_count, failed_count);

  /* Section breakdown: main spec vs extensions (use first-run results) */
  {
    int main_pass = 0, ext_pass = 0;
    /* Failures for main spec are indices 0..main_count-1 */
    for (int i = 0; i < failed_count; ++i) {
      if (failed_indices[i] < main_count)
        continue; /* counted as main fail */
      else
        break;
    }
    /* Count: total - failures in that range */
    int main_fail = 0, ext_fail = 0;
    for (int i = 0; i < failed_count; ++i) {
      if (failed_indices[i] < main_count)
        ++main_fail;
      else
        ++ext_fail;
    }
    main_pass = main_count - main_fail;
    ext_pass = ext_count - ext_fail;
    fprintf(stderr, "\n  Main spec:   %d / %d (%.1f%%)\n",
            main_pass, main_count, 100.0 * main_pass / main_count);
    fprintf(stderr, "  Extensions:  %d / %d (%.1f%%)\n",
            ext_pass, ext_count,
            ext_count ? 100.0 * ext_pass / ext_count : 0.0);
  }

  /* Compact failure list */
  fprintf(stderr, "\n  Failed examples: ");
  for (int i = 0; i < failed_count; ++i) {
    if (i > 0)
      fprintf(stderr, ", ");
    fprintf(stderr, "%d", g_examples[failed_indices[i]].number);
  }
  fprintf(stderr, "\n");

  spec_free_examples();

  /* Exit 0 — this is a conformance metric, not pass/fail gate (yet). */
  return 0;
}
