/* Markdown-to-HTML renderer focused tests.
 * Build: make tests/md2html/db
 */
#define FIO_LOG
#define FIO_MD2HTML
#include "test-helpers.h"

#include <stdio.h>
#include <string.h>

/* ── test infrastructure ── */

static int g_pass = 0;
static int g_fail = 0;

#define TEST_ASSERT(cond, ...)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "  FAIL %s:%d: ", __FILE__, __LINE__);                   \
      fprintf(stderr, __VA_ARGS__);                                            \
      fprintf(stderr, "\n");                                                   \
      ++g_fail;                                                                \
    } else {                                                                   \
      ++g_pass;                                                                \
    }                                                                          \
  } while (0)

#define TEST_HTML_EQ(src, expected)                                            \
  do {                                                                         \
    char *html = fio_md2html(NULL, FIO_BUF_INFO1((char *)(src)));              \
    TEST_ASSERT(html != NULL, "fio_md2html returned NULL for: %s", #src);      \
    if (html) {                                                                \
      TEST_ASSERT(strcmp(html, (expected)) == 0,                               \
                  "%s\n  expected: %s\n  got:      %s",                        \
                  #src,                                                        \
                  (expected),                                                  \
                  html);                                                       \
      fio_bstr_free(html);                                                     \
    }                                                                          \
  } while (0)

/* ── tests ── */

static void test_empty(void) {
  fprintf(stderr, "test_empty\n");
  char *html = fio_md2html(NULL, FIO_BUF_INFO1((char *)""));
  TEST_ASSERT(html != NULL, "empty input returns non-NULL");
  TEST_ASSERT(fio_bstr_len(html) == 0, "empty input produces empty output");
  fio_bstr_free(html);
}

static void test_paragraph(void) {
  fprintf(stderr, "test_paragraph\n");
  TEST_HTML_EQ("hello\n", "<p>hello</p>");
  TEST_HTML_EQ("hello world\n", "<p>hello world</p>");
}

static void test_headings(void) {
  fprintf(stderr, "test_headings\n");
  TEST_HTML_EQ("# h1\n", "<h1>h1</h1>");
  TEST_HTML_EQ("## h2\n", "<h2>h2</h2>");
  TEST_HTML_EQ("###### h6\n", "<h6>h6</h6>");
}

static void test_thematic_break(void) {
  fprintf(stderr, "test_thematic_break\n");
  TEST_HTML_EQ("---\n", "<hr />");
  TEST_HTML_EQ("***\n", "<hr />");
}

static void test_blockquote(void) {
  fprintf(stderr, "test_blockquote\n");
  TEST_HTML_EQ("> quote\n", "<blockquote><p>quote</p></blockquote>");
}

static void test_list(void) {
  fprintf(stderr, "test_list\n");
  TEST_HTML_EQ("- a\n- b\n", "<ul><li>a</li><li>b</li></ul>");
  TEST_HTML_EQ("1. a\n2. b\n", "<ol><li>a</li><li>b</li></ol>");
}

static void test_task_list(void) {
  fprintf(stderr, "test_task_list\n");
  TEST_HTML_EQ("- [x] done\n",
               "<ul><li><input type=\"checkbox\" checked=\"\" "
               "disabled=\"\" /> done</li></ul>");
  TEST_HTML_EQ(
      "- [ ] todo\n",
      "<ul><li><input type=\"checkbox\" disabled=\"\" /> todo</li></ul>");
}

static void test_code_block(void) {
  fprintf(stderr, "test_code_block\n");
  TEST_HTML_EQ("```\ncode\n```\n", "<pre><code>code\n</code></pre>");
  TEST_HTML_EQ("```c\ncode\n```\n",
               "<pre><code class=\"language-c\">code\n</code></pre>");
}

static void test_inline_formatting(void) {
  fprintf(stderr, "test_inline_formatting\n");
  TEST_HTML_EQ("*em*\n", "<p><em>em</em></p>");
  TEST_HTML_EQ("**strong**\n", "<p><strong>strong</strong></p>");
  TEST_HTML_EQ("~~del~~\n", "<p><del>del</del></p>");
  TEST_HTML_EQ("`code`\n", "<p><code>code</code></p>");
}

static void test_link(void) {
  fprintf(stderr, "test_link\n");
  TEST_HTML_EQ("[text](http://example.com)\n",
               "<p><a href=\"http://example.com\">text</a></p>");
  TEST_HTML_EQ(
      "[text](http://example.com \"title\")\n",
      "<p><a href=\"http://example.com\" title=\"title\">text</a></p>");
}

static void test_image(void) {
  fprintf(stderr, "test_image\n");
  TEST_HTML_EQ("![alt](/img.png)\n",
               "<p><img src=\"/img.png\" alt=\"alt\" /></p>");
}

static void test_autolink(void) {
  fprintf(stderr, "test_autolink\n");
  TEST_HTML_EQ(
      "<user@example.com>\n",
      "<p><a href=\"mailto:user@example.com\">user@example.com</a></p>");
  TEST_HTML_EQ("<http://example.com>\n",
               "<p><a href=\"http://example.com\">http://example.com</a></p>");
}

static void test_html_escape(void) {
  fprintf(stderr, "test_html_escape\n");
  TEST_HTML_EQ("a < b & c\n", "<p>a &lt; b &amp; c</p>");
  TEST_HTML_EQ("\"quoted\"\n", "<p>&quot;quoted&quot;</p>");
}

static void test_table(void) {
  fprintf(stderr, "test_table\n");
  TEST_HTML_EQ("| a | b |\n|---|---|\n| c | d |\n",
               "<table><thead><tr><th>a</th><th>b</th>"
               "</tr></thead><tbody><tr><td>c</td>"
               "<td>d</td></tr></tbody></table>");
}

static void test_table_column_limit(void) {
  fprintf(stderr, "test_table_column_limit\n");
  const size_t counts[] = {
      FIO_GFM_MAX_TABLE_COLUMNS, FIO_GFM_MAX_TABLE_COLUMNS + 1, 4096};
  for (size_t t = 0; t < sizeof(counts) / sizeof(*counts); ++t) {
    const size_t columns = counts[t];
    const size_t len = columns * 4 + 2;
    char *src = malloc(len);
    TEST_ASSERT(src != NULL, "column-limit input allocation");
    if (!src)
      return;
    size_t n = 0;
    for (size_t i = 0; i < columns; ++i) {
      src[n++] = 'x';
      src[n++] = '|';
    }
    src[n++] = '\n';
    for (size_t i = 0; i < columns; ++i) {
      src[n++] = '-';
      src[n++] = '|';
    }
    src[n++] = '\n';
    char *html = fio_md2html(NULL, FIO_BUF_INFO2(src, len));
    TEST_ASSERT(html != NULL, "%zu columns render successfully", columns);
    if (html) {
      const int is_table = strstr(html, "<table>") != NULL;
      TEST_ASSERT(is_table == (columns <= FIO_GFM_MAX_TABLE_COLUMNS),
                  "%zu columns table detection = %d",
                  columns,
                  is_table);
      fio_bstr_free(html);
    }
    free(src);
  }
}

static void test_hard_break(void) {
  fprintf(stderr, "test_hard_break\n");
  TEST_HTML_EQ("foo  \nbar\n", "<p>foo<br />\nbar</p>");
}

static void test_append_to_existing(void) {
  fprintf(stderr, "test_append_to_existing\n");
  char *bstr = fio_bstr_write(NULL, "prefix:", 7);
  TEST_ASSERT(bstr != NULL, "seed bstr allocated");
  bstr = fio_md2html(bstr, FIO_BUF_INFO1((char *)"x\n"));
  TEST_ASSERT(bstr != NULL, "fio_md2html appended to existing bstr");
  if (bstr) {
    TEST_ASSERT(strcmp(bstr, "prefix:<p>x</p>") == 0,
                "appended output correct, got: %s",
                bstr);
    fio_bstr_free(bstr);
  }
}

static void test_entity_exact_windows(void) {
  fprintf(stderr, "test_entity_exact_windows\n");
  static const struct {
    const char *src;
    const char *expected;
  } cases[] = {{"&#1", "<p>&amp;#1</p>"},
               {"&#x", "<p>&amp;#x</p>"},
               {"&#x0", "<p>&amp;#x0</p>"},
               {"&#x41", "<p>&amp;#x41</p>"},
               {"&#1_", "<p>&amp;#1_</p>"},
               {"&#65;", "<p>A</p>"},
               {"&#x41;", "<p>A</p>"},
               {"&#60;", "<p>&lt;</p>"},
               {"&#x3c;", "<p>&lt;</p>"},
               {"&#6_0;", "<p>&amp;#6_0;</p>"},
               {"&#x0x3c;", "<p>&amp;#x0x3c;</p>"},
               {"&#55296;", "<p>\xEF\xBF\xBD</p>"}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    size_t len = strlen(cases[i].src);
    char *exact = malloc(len);
    FIO_ASSERT(exact, "exact entity input allocation failed");
    FIO_MEMCPY(exact, cases[i].src, len);
    char *html = fio_md2html(NULL, FIO_BUF_INFO2(exact, len));
    TEST_ASSERT(html != NULL, "exact entity conversion returned NULL");
    if (html) {
      TEST_ASSERT(!strcmp(html, cases[i].expected),
                  "exact entity %s: expected %s, got %s",
                  cases[i].src,
                  cases[i].expected,
                  html);
      fio_bstr_free(html);
    }
    free(exact);
  }
}

/* ── main ── */

int main(void) {
  fprintf(stderr, "=== Markdown-to-HTML Tests ===\n");
  test_empty();
  test_paragraph();
  test_headings();
  test_thematic_break();
  test_blockquote();
  test_list();
  test_task_list();
  test_code_block();
  test_inline_formatting();
  test_link();
  test_image();
  test_autolink();
  test_html_escape();
  test_table();
  test_table_column_limit();
  test_hard_break();
  test_append_to_existing();
  test_entity_exact_windows();
  fprintf(stderr, "\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
