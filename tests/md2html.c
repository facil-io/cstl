/* *****************************************************************************
Markdown to HTML bstr Renderer Tests
***************************************************************************** */
#define FIO_LOG
#define FIO_MD2HTML
#include "test-helpers.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  char *markdown;
  char *expected;
  size_t markdown_len;
  size_t expected_len;
  size_t line_start;
  size_t line_count;
} test_md2html_fixture_case_s;

typedef struct {
  test_md2html_fixture_case_s *items;
  size_t count;
} test_md2html_fixture_s;

static void test_md2html_expect(const char *name,
                                char *source,
                                const char *expected) {
  size_t expected_len = FIO_STRLEN(expected);
  char *html = fio_md2html(NULL, FIO_BUF_INFO1(source));
  FIO_ASSERT(html, "%s: expected rendered HTML", name);
  FIO_ASSERT(fio_bstr_len(html) == expected_len,
             "%s: expected length %zu, got %zu\n%s",
             name,
             expected_len,
             fio_bstr_len(html),
             html);
  FIO_ASSERT(FIO_MEMCMP(html, expected, expected_len) == 0,
             "%s: unexpected HTML\nexpected:\n%s\nactual:\n%s",
             name,
             expected,
             html);
  fio_bstr_free(html);
}

static char *test_md2html_file_read(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  char *buf = NULL;
  long file_len;
  FIO_ASSERT(f, "could not open %s", path);
  FIO_ASSERT(fseek(f, 0, SEEK_END) == 0, "could not seek %s", path);
  file_len = ftell(f);
  FIO_ASSERT(file_len >= 0, "could not tell %s", path);
  FIO_ASSERT(fseek(f, 0, SEEK_SET) == 0, "could not rewind %s", path);
  buf = (char *)malloc((size_t)file_len + 1);
  FIO_ASSERT(buf, "could not allocate fixture buffer");
  *len = fread(buf, 1, (size_t)file_len, f);
  FIO_ASSERT(*len == (size_t)file_len, "could not read %s", path);
  buf[*len] = 0;
  fclose(f);
  return buf;
}

static size_t test_md2html_line_number(char *base, char *p) {
  size_t line = 1;
  while (base < p) {
    if (*base == '\n')
      ++line;
    ++base;
  }
  return line;
}

static char *test_md2html_line_end(char *p, char *end) {
  while (p < end && *p != '\n' && *p != '\r')
    ++p;
  return p;
}

static char *test_md2html_line_next(char *p, char *end) {
  if (p < end && *p == '\r')
    ++p;
  if (p < end && *p == '\n')
    ++p;
  return p;
}

static int test_md2html_is_example_fence(char *line,
                                         char *line_end,
                                         size_t *fence_len) {
  char *p = line;
  while (p < line_end && *p == '`')
    ++p;
  if ((size_t)(p - line) < 3)
    return 0;
  if ((size_t)(line_end - p) != 8 || FIO_MEMCMP(p, " example", 8) != 0)
    return 0;
  *fence_len = (size_t)(p - line);
  return 1;
}

static int test_md2html_same_closing_fence(char *line,
                                           char *line_end,
                                           char *fence,
                                           size_t fence_len) {
  return (size_t)(line_end - line) == fence_len &&
         FIO_MEMCMP(line, fence, fence_len) == 0;
}

static char *test_md2html_gfm_copy(char *start, char *end, size_t *len) {
  char *buf = (char *)malloc((size_t)(end - start) + 1);
  char *w = buf;
  FIO_ASSERT(buf, "could not allocate GFM example copy");
  while (start < end) {
    if ((unsigned char)*start == 0xE2 && start + 2 < end &&
        (unsigned char)start[1] == 0x86 && (unsigned char)start[2] == 0x92) {
      *w++ = '\t';
      start += 3;
    } else {
      *w++ = *start++;
    }
  }
  *w = 0;
  *len = (size_t)(w - buf);
  return buf;
}

static void test_md2html_fixture_free(test_md2html_fixture_s *fixture) {
  if (!fixture || !fixture->items)
    return;
  for (size_t i = 0; i < fixture->count; ++i) {
    free(fixture->items[i].markdown);
    free(fixture->items[i].expected);
  }
  free(fixture->items);
  fixture->items = NULL;
  fixture->count = 0;
}

static test_md2html_fixture_s test_md2html_fixture_load(const char *path) {
  size_t file_len = 0;
  char *file = test_md2html_file_read(path, &file_len);
  char *p = file;
  char *end = file + file_len;
  test_md2html_fixture_s fixture = {0};
  while (p < end) {
    char *line = p;
    char *line_end = test_md2html_line_end(line, end);
    size_t fence_len = 0;
    if (test_md2html_is_example_fence(line, line_end, &fence_len)) {
      char *fence = line;
      size_t line_start = test_md2html_line_number(file, line);
      char *markdown_start;
      char *markdown_end;
      char *html_start;
      char *html_end;
      char *close_line;
      char *after_close;
      test_md2html_fixture_case_s *tmp;
      p = test_md2html_line_next(line_end, end);
      markdown_start = p;
      while (p < end) {
        line = p;
        line_end = test_md2html_line_end(line, end);
        if ((size_t)(line_end - line) == 1 && *line == '.')
          break;
        p = test_md2html_line_next(line_end, end);
      }
      FIO_ASSERT(p < end,
                 "%s:%zu: unterminated markdown example",
                 path,
                 line_start);
      markdown_end = p;
      p = test_md2html_line_next(test_md2html_line_end(p, end), end);
      html_start = p;
      close_line = end;
      while (p < end) {
        line = p;
        line_end = test_md2html_line_end(line, end);
        if (test_md2html_same_closing_fence(line, line_end, fence, fence_len)) {
          close_line = line;
          break;
        }
        p = test_md2html_line_next(line_end, end);
      }
      FIO_ASSERT(close_line < end,
                 "%s:%zu: unterminated expected HTML",
                 path,
                 line_start);
      html_end = close_line;
      after_close =
          test_md2html_line_next(test_md2html_line_end(close_line, end), end);
      tmp = (test_md2html_fixture_case_s *)realloc(fixture.items,
                                                   (fixture.count + 1) *
                                                       sizeof(*fixture.items));
      FIO_ASSERT(tmp, "could not grow fixture array");
      fixture.items = tmp;
      fixture.items[fixture.count].markdown =
          test_md2html_gfm_copy(markdown_start,
                                markdown_end,
                                &fixture.items[fixture.count].markdown_len);
      fixture.items[fixture.count].expected =
          test_md2html_gfm_copy(html_start,
                                html_end,
                                &fixture.items[fixture.count].expected_len);
      fixture.items[fixture.count].line_start = line_start;
      fixture.items[fixture.count].line_count =
          test_md2html_line_number(file, after_close) - line_start;
      ++fixture.count;
      p = after_close;
      continue;
    }
    p = test_md2html_line_next(line_end, end);
  }
  free(file);
  return fixture;
}

static int test_md2html_ascii_eq_lc(char a, char b) {
  if (a >= 'A' && a <= 'Z')
    a = (char)(a | 32);
  if (b >= 'A' && b <= 'Z')
    b = (char)(b | 32);
  return a == b;
}

static int test_md2html_tag_prefix(char *p, char *end, const char *tag) {
  while (*tag) {
    if (p == end || !test_md2html_ascii_eq_lc(*p, *tag))
      return 0;
    ++p;
    ++tag;
  }
  return p == end || *p == '>' || *p == '/' || *p == ' ' || *p == '\t' ||
         *p == '\n' || *p == '\r';
}

static char *test_md2html_tag_name(char *p, char *end, int *is_close) {
  *is_close = 0;
  if (p < end && *p == '/') {
    *is_close = 1;
    ++p;
  }
  if (p == end || !((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')))
    return NULL;
  return p;
}

static int test_md2html_tag_name_is(char *p, char *end, const char *tag) {
  int is_close;
  p = test_md2html_tag_name(p, end, &is_close);
  (void)is_close;
  if (!p)
    return 0;
  return test_md2html_tag_prefix(p, end, tag);
}

static int test_md2html_tag_is_phrasing(char *p, char *end) {
  static const char *phrasing[] = {
      "a",      "abbr",  "b",    "bdi",  "bdo", "br",   "cite",  "code",
      "data",   "del",   "dfn",  "em",   "i",   "img",  "input", "ins",
      "kbd",    "label", "mark", "q",    "s",   "samp", "small", "span",
      "strong", "sub",   "sup",  "time", "u",   "var",  "wbr",
  };
  int is_close;
  p = test_md2html_tag_name(p, end, &is_close);
  (void)is_close;
  if (!p)
    return 0;
  for (size_t i = 0; i < sizeof(phrasing) / sizeof(phrasing[0]); ++i) {
    if (test_md2html_tag_prefix(p, end, phrasing[i]))
      return 1;
  }
  return 0;
}

static char *test_md2html_skip_space(char *p, char *end) {
  while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
    ++p;
  return p;
}

static int test_md2html_attr_name_is(char *p, char *end, const char *name) {
  while (*name) {
    if (p == end || !test_md2html_ascii_eq_lc(*p, *name))
      return 0;
    ++p;
    ++name;
  }
  return p == end || *p == '=' || *p == ' ' || *p == '\t' || *p == '\n' ||
         *p == '\r' || *p == '>' || *p == '/';
}

static int test_md2html_bool_attr(char *p, char *end) {
  static const char *names[] = {
      "allowfullscreen", "async",   "autofocus", "autoplay", "checked",
      "controls",        "default", "defer",     "disabled", "formnovalidate",
      "hidden",          "ismap",   "loop",      "multiple", "muted",
      "novalidate",      "open",    "readonly",  "required", "reversed",
      "selected"};
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
    if (test_md2html_attr_name_is(p, end, names[i]))
      return 1;
  }
  return 0;
}

static char *test_md2html_copy_normalized_tag(char *w, char *p, char *end) {
  char *q = p;
  while (q < end) {
    char *attr;
    char *attr_end;
    if (*q != ' ' && *q != '\t' && *q != '\n' && *q != '\r') {
      *w++ = *q++;
      continue;
    }
    attr = test_md2html_skip_space(q, end);
    attr_end = attr;
    while (attr_end < end && attr_end[0] != '=' && attr_end[0] != ' ' &&
           attr_end[0] != '\t' && attr_end[0] != '\n' && attr_end[0] != '\r' &&
           attr_end[0] != '>' && attr_end[0] != '/')
      ++attr_end;
    if (test_md2html_bool_attr(attr, attr_end)) {
      char *after = test_md2html_skip_space(attr_end, end);
      if (after < end && *after == '=') {
        after = test_md2html_skip_space(after + 1, end);
        if (after + 1 < end && after[0] == '"' && after[1] == '"') {
          *w++ = ' ';
          FIO_MEMCPY(w, attr, (size_t)(attr_end - attr));
          w += (size_t)(attr_end - attr);
          q = after + 2;
          continue;
        }
      }
    }
    *w++ = *q++;
  }
  return w;
}

static char *test_md2html_normalize_html(char *src,
                                         size_t len,
                                         size_t *out_len) {
  char *out = (char *)malloc(len + 1);
  char *w = out;
  char *p = src;
  char *end = src + len;
  int preserve_space_depth = 0;
  int previous_tag_phrasing = 0;
  FIO_ASSERT(out, "could not allocate normalized HTML buffer");
  while (p < end) {
    if (!preserve_space_depth &&
        (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') && w > out &&
        w[-1] == '>') {
      char *q = test_md2html_skip_space(p, end);
      if (q < end && *q == '<') {
        char *tag_end = q + 1;
        while (tag_end < end && *tag_end != '>')
          ++tag_end;
        if (tag_end < end && (!previous_tag_phrasing ||
                              !test_md2html_tag_is_phrasing(q + 1, tag_end))) {
          p = q;
          continue;
        }
      }
    }
    if (*p == '<' && p + 1 < end) {
      char *q = p + 1;
      char *tag_start = q;
      int is_close = 0;
      while (q < end && *q != '>')
        ++q;
      if (q < end) {
        (void)test_md2html_tag_name(tag_start, q, &is_close);
        if (test_md2html_tag_name_is(tag_start, q, "pre")) {
          if (is_close) {
            if (preserve_space_depth)
              --preserve_space_depth;
          } else {
            ++preserve_space_depth;
          }
        }
        previous_tag_phrasing = test_md2html_tag_is_phrasing(tag_start, q);
      }
      if (q < end && test_md2html_tag_prefix(tag_start, q, "br")) {
        FIO_MEMCPY(w, "<br>", 4);
        w += 4;
        p = q + 1;
        continue;
      }
      if (q < end && test_md2html_tag_prefix(tag_start, q, "hr")) {
        FIO_MEMCPY(w, "<hr>", 4);
        w += 4;
        p = q + 1;
        continue;
      }
      if (q < end && (test_md2html_tag_prefix(tag_start, q, "img") ||
                      test_md2html_tag_prefix(tag_start, q, "input"))) {
        char *trim = q;
        while (trim > p && (trim[-1] == ' ' || trim[-1] == '\t' ||
                            trim[-1] == '\n' || trim[-1] == '\r'))
          --trim;
        if (trim > p && trim[-1] == '/') {
          --trim;
          while (trim > p && (trim[-1] == ' ' || trim[-1] == '\t' ||
                              trim[-1] == '\n' || trim[-1] == '\r'))
            --trim;
        }
        w = test_md2html_copy_normalized_tag(w, p, trim);
        *w++ = '>';
        p = q + 1;
        continue;
      }
    }
    *w++ = *p++;
  }
  *w = 0;
  *out_len = (size_t)(w - out);
  return out;
}

static int test_md2html_case_compare(const char *path,
                                     const test_md2html_fixture_case_s *example,
                                     int print_error) {
  size_t expected_norm_len = 0;
  size_t actual_norm_len = 0;
  char *actual =
      fio_md2html(NULL,
                  FIO_BUF_INFO2(example->markdown, example->markdown_len));
  char *expected_norm = test_md2html_normalize_html(example->expected,
                                                    example->expected_len,
                                                    &expected_norm_len);
  char *actual_norm = actual ? test_md2html_normalize_html(actual,
                                                           fio_bstr_len(actual),
                                                           &actual_norm_len)
                             : NULL;
  int ok = actual && actual_norm_len == expected_norm_len &&
           FIO_MEMCMP(actual_norm, expected_norm, expected_norm_len) == 0;
  if (!ok && print_error) {
    FIO_LOG_DEBUG2("Mismatch on %s:%zu-%zu, got:\n%s",
                   path,
                   example->line_start,
                   example->line_start + example->line_count - 1,
                   actual_norm ? actual_norm : "<render failed>");
  }
  if (actual_norm)
    free(actual_norm);
  free(expected_norm);
  if (actual)
    fio_bstr_free(actual);
  return ok;
}

static void test_md2html_fixture_line(const char *path, size_t line) {
  test_md2html_fixture_s fixture = test_md2html_fixture_load(path);
  for (size_t i = 0; i < fixture.count; ++i) {
    if (fixture.items[i].line_start == line) {
      FIO_ASSERT(test_md2html_case_compare(path, &fixture.items[i], 1),
                 "GFM fixture example failed: %s:%zu-%zu",
                 path,
                 fixture.items[i].line_start,
                 fixture.items[i].line_start + fixture.items[i].line_count - 1);
      test_md2html_fixture_free(&fixture);
      return;
    }
  }
  test_md2html_fixture_free(&fixture);
  FIO_ASSERT(0, "GFM fixture example %s:%zu not found", path, line);
}

#ifdef DEBUG
static size_t test_md2html_fixture_audit(const char *path) {
  test_md2html_fixture_s fixture = test_md2html_fixture_load(path);
  size_t mismatches = 0;
  fprintf(stderr, "GFM fixture audit: scanning %s\n", path);
  for (size_t i = 0; i < fixture.count; ++i) {
    if (!test_md2html_case_compare(path, &fixture.items[i], 1))
      ++mismatches;
  }
  fprintf(stderr,
          "GFM fixture audit summary for %s: %zu/%zu examples mismatch.\n",
          path,
          mismatches,
          fixture.count);
  test_md2html_fixture_free(&fixture);
  return mismatches;
}
#endif

static void test_md2html_fixture_normalization(void) {
  char readable[] =
      "<table>\n<thead>\n<tr>\n<th>abc</th>\n</tr>\n</thead>\n</table>\n";
  char compact[] = "<table><thead><tr><th>abc</th></tr></thead></table>\n";
  char inline_readable[] = "<p><em>a</em> <strong>b</strong></p>\n";
  char inline_compact[] = "<p><em>a</em><strong>b</strong></p>\n";
  char bool_long[] = "<input type=\"checkbox\" disabled=\"\" checked=\"\">\n";
  char bool_short[] = "<input type=\"checkbox\" disabled checked>\n";
  size_t readable_len = 0;
  size_t compact_len = 0;
  size_t inline_readable_len = 0;
  size_t inline_compact_len = 0;
  size_t bool_long_len = 0;
  size_t bool_short_len = 0;
  char *readable_norm = test_md2html_normalize_html(readable,
                                                    FIO_STRLEN(readable),
                                                    &readable_len);
  char *compact_norm =
      test_md2html_normalize_html(compact, FIO_STRLEN(compact), &compact_len);
  char *inline_readable_norm =
      test_md2html_normalize_html(inline_readable,
                                  FIO_STRLEN(inline_readable),
                                  &inline_readable_len);
  char *inline_compact_norm =
      test_md2html_normalize_html(inline_compact,
                                  FIO_STRLEN(inline_compact),
                                  &inline_compact_len);
  char *bool_long_norm = test_md2html_normalize_html(bool_long,
                                                     FIO_STRLEN(bool_long),
                                                     &bool_long_len);
  char *bool_short_norm = test_md2html_normalize_html(bool_short,
                                                      FIO_STRLEN(bool_short),
                                                      &bool_short_len);
  FIO_ASSERT(readable_len == compact_len &&
                 FIO_MEMCMP(readable_norm, compact_norm, readable_len) == 0,
             "inter-element fixture whitespace should be optional");
  FIO_ASSERT(inline_readable_len != inline_compact_len ||
                 FIO_MEMCMP(inline_readable_norm,
                            inline_compact_norm,
                            inline_readable_len) != 0,
             "spaces between phrasing elements should remain significant");
  FIO_ASSERT(bool_long_len == bool_short_len &&
                 FIO_MEMCMP(bool_long_norm, bool_short_norm, bool_long_len) ==
                     0,
             "boolean attributes should allow empty-string shorthand");
  free(readable_norm);
  free(compact_norm);
  free(inline_readable_norm);
  free(inline_compact_norm);
  free(bool_long_norm);
  free(bool_short_norm);
}

static void test_md2html_gfm_fixture_examples(void) {
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 395);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 408);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 431);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 440);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 461);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 551);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 580);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 931);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 941);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 1033);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 1199);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 1583);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 1763);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2100);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2151);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2191);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2202);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2304);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2328);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2403);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2424);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2479);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2517);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2542);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2560);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2588);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2678);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2829);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2838);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2849);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2871);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2890);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2929);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2940);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 2953);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-spec.txt", 3685);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-extensions.txt", 646);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-extensions.txt", 704);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-extensions.txt", 764);
  test_md2html_fixture_line("./tests/gfm-spec/gfm-extensions.txt", 784);
}

static void test_md2html_inline_and_blocks(void) {
  char source[] = "# Hi & \"you\"\n"
                  "\n"
                  "This is **strong** and *em* with "
                  "[link](https://example.com?q=1&x=\"y\" \"title & more\").\n"
                  "\n"
                  "- [x] task\n"
                  "\n"
                  "```c\n"
                  "if (a < b) return &x;\n"
                  "```\n";
  const char expected[] =
      "<h1>Hi &amp; &quot;you&quot;</h1>\n"
      "<p>This is <strong>strong</strong> and <em>em</em> with "
      "<a href=\"https://example.com?q=1&amp;x=&quot;y&quot;\" "
      "title=\"title &amp; more\">link</a>.</p>\n"
      "<ul>\n"
      "<li><input type=\"checkbox\" disabled checked> task</li>\n"
      "</ul>\n"
      "<pre><code class=\"language-c\">"
      "if (a &lt; b) return &amp;x;\n"
      "</code></pre>\n";
  test_md2html_expect("inline and block rendering", source, expected);
}

static void test_md2html_table_and_raw_html(void) {
  char source[] = "| a | b |\n"
                  "| :- | -: |\n"
                  "| 1 | 2 |\n"
                  "\n"
                  "<span>raw</span> &amp;\n";
  const char expected[] = "<table>\n"
                          "<thead>\n"
                          "<tr><th align=\"left\">a</th>"
                          "<th align=\"right\">b</th></tr>\n"
                          "</thead>\n"
                          "<tbody>\n"
                          "<tr><td align=\"left\">1</td>"
                          "<td align=\"right\">2</td></tr>\n"
                          "</tbody>\n"
                          "</table>\n"
                          "<p><span>raw</span> &amp;</p>\n";
  test_md2html_expect("table and raw html rendering", source, expected);
}

static void test_md2html_gfm_regressions(void) {
  char hr_source[] = "***\n";
  char list_source[] = "- foo\n- bar\n";
  char paragraph_source[] = "Foo\n    ***\n";
  char empty_em_source[] = "--\n**\n__\n";
  char fenced_indent_source[] = " ```\n aaa\naaa\n```\n";
  char blockquote_lazy_setext_source[] = "> foo\nbar\n===\n";
  char blockquote_list_source[] = "> - foo\n> - bar\n";
  char reference_url_encode_source[] = "[x][r]\n\n[r]: <a b>\n";
  char reference_url_preserve_pct_source[] = "[x][r]\n\n[r]: <a%20b>\n";
  test_md2html_expect("GFM thematic break", hr_source, "<hr />\n");
  test_md2html_expect("GFM tight list",
                      list_source,
                      "<ul>\n<li>foo</li>\n<li>bar</li>\n</ul>\n");
  test_md2html_expect("GFM indented paragraph continuation",
                      paragraph_source,
                      "<p>Foo\n***</p>\n");
  test_md2html_expect("GFM empty emphasis delimiters",
                      empty_em_source,
                      "<p>--\n**\n__</p>\n");
  test_md2html_expect("GFM fenced code strips opening indent",
                      fenced_indent_source,
                      "<pre><code>aaa\naaa\n</code></pre>\n");
  test_md2html_expect("GFM blockquote lazy setext continuation",
                      blockquote_lazy_setext_source,
                      "<blockquote>\n<p>foo\nbar\n===</p>\n</blockquote>\n");
  test_md2html_expect(
      "GFM blockquote list",
      blockquote_list_source,
      "<blockquote>\n<ul>\n<li>foo</li>\n<li>bar</li>\n</ul>\n</blockquote>\n");
  test_md2html_expect("reference URL percent encoding",
                      reference_url_encode_source,
                      "<p><a href=\"a%20b\">x</a></p>\n");
  test_md2html_expect("reference URL existing percent preserved",
                      reference_url_preserve_pct_source,
                      "<p><a href=\"a%20b\">x</a></p>\n");
}

static void test_md2html_existing_target(void) {
  char source[] = "*x*\n";
  char *html = fio_bstr_write(NULL, "prefix", 6);
  html = fio_md2html(html, FIO_BUF_INFO1(source));
  FIO_ASSERT(html, "Existing target should render");
  FIO_ASSERT(fio_bstr_len(html) == FIO_STRLEN("prefix<p><em>x</em></p>\n") &&
                 FIO_MEMCMP(html,
                            "prefix<p><em>x</em></p>\n",
                            FIO_STRLEN("prefix<p><em>x</em></p>\n")) == 0,
             "Existing target should be appended to, got: %s",
             html);
  fio_bstr_free(html);
}

static void test_md2html_empty(void) {
  char source[] = "";
  char *html = fio_md2html(NULL, FIO_BUF_INFO1(source));
  FIO_ASSERT(html, "Empty Markdown should render an empty bstr");
  FIO_ASSERT(fio_bstr_len(html) == 0, "Empty Markdown should render empty");
  fio_bstr_free(html);
}

int main(void) {
  test_md2html_fixture_normalization();
  test_md2html_inline_and_blocks();
  test_md2html_table_and_raw_html();
  test_md2html_gfm_regressions();
  test_md2html_gfm_fixture_examples();
  test_md2html_existing_target();
#ifdef DEBUG
  {
    size_t gfm_mismatches =
        test_md2html_fixture_audit("./tests/gfm-spec/gfm-spec.txt") +
        test_md2html_fixture_audit("./tests/gfm-spec/gfm-extensions.txt");
    FIO_ASSERT(gfm_mismatches == 0,
               "GFM fixture audit failed: %zu total examples mismatch",
               gfm_mismatches);
  }
#endif
  test_md2html_empty();
  fprintf(stderr, "Markdown to HTML renderer tests passed.\n");
  return 0;
}
