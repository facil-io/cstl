/* GFM complexity hardening regression.
 * Build/run: make tests/gfm-complexity; asan/tests/gfm-complexity.
 * Correctness only; scaling is measured outside unit tests.
 */
#define FIO_MD2HTML
#include "fio-stl/include.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);           \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

static void check_following_link(size_t n, int balanced) {
  char *src = malloc(n * 2 + 40);
  CHECK(src);
  size_t at = 0;
  memset(src + at, '[', n);
  at += n;
  if (balanced) {
    src[at++] = 'x';
    memset(src + at, ']', n);
    at += n;
    memcpy(src + at, "(v)", 3);
    at += 3;
  }
  memcpy(src + at, " [later](w)\n", 12);
  at += 12;
  char *html = fio_md2html(NULL, FIO_BUF_INFO2(src, at));
  CHECK(html && strstr(html, "<a href=\"w\">later</a>"));
  fio_bstr_free(html);
  free(src);
}

static void check_image_depth(size_t n) {
  char *src = malloc(6 * n + 16);
  CHECK(src);
  size_t at = 0;
  for (size_t i = 0; i < n; ++i) {
    src[at++] = '!';
    src[at++] = '[';
  }
  src[at++] = 'x';
  for (size_t i = 0; i < n; ++i) {
    memcpy(src + at, "](u)", 4);
    at += 4;
  }
  memcpy(src + at, " [later](w)\n", 12);
  at += 12;
  char *html = fio_md2html(NULL, FIO_BUF_INFO2(src, at));
  CHECK(html && strstr(html, "<a href=\"w\">later</a>"));
  /* An over-deep opener is literal; its inner eligible opener may still
   * produce an image, as must the unrelated link following it. */
  CHECK(strstr(html, "<img src=\"u\" alt=") != NULL);
  if (n > FIO___GFM_MAX_INLINE_BRACKETS)
    CHECK(!memcmp(html, "<p>![", 5));
  fio_bstr_free(html);
  free(src);
}

/* Repeat `unit` n times between `pre` and `post`; render; require `want`. */
static void check_repeat(const char *pre,
                         const char *unit,
                         size_t n,
                         const char *post,
                         const char *want) {
  size_t pl = strlen(pre), ul = strlen(unit), sl = strlen(post);
  char *src = malloc(pl + ul * n + sl);
  CHECK(src);
  memcpy(src, pre, pl);
  for (size_t i = 0; i < n; ++i)
    memcpy(src + pl + ul * i, unit, ul);
  memcpy(src + pl + ul * n, post, sl);
  char *html = fio_md2html(NULL, FIO_BUF_INFO2(src, pl + ul * n + sl));
  if (!html || !strstr(html, want)) {
    fprintf(stderr, "FAIL: [%s][%s]x%zu[%s] lacks %s\n", pre, unit, n, post,
            want);
    exit(1);
  }
  fio_bstr_free(html);
  free(src);
}

static void check_html(const char *md, const char *want) {
  check_repeat(md, "", 0, "", want);
}

/* Malformed inline-link families must stay literal without hiding a later
 * independent link (these shapes used to rescan the paragraph suffix). */
static void check_malformed_families(void) {
  static const char *units[] = {
      "[x](", "[x](<", "[x](u \"", "[x](u '", "[x](u (", "[x][", "[x](()",
      "[x](<a [y](b (c [z](d \"e ",
  };
  static const size_t sizes[] = {1, 64, 4096};
  const char *later = "<a href=\"w\">later</a>";
  for (size_t u = 0; u < sizeof(units) / sizeof(units[0]); ++u)
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
      check_repeat("", units[u], sizes[i], " [later](w)\n", later);
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    check_repeat("", "[", sizes[i], " [later](w)\n", later);
    check_repeat("<", " ", sizes[i], "a [later](w)\n", later);
    /* Outer text with many unclosed inner destinations. */
    check_repeat("[", "[a](", sizes[i], "](u) [later](w)\n", later);
  }
}

static void check_valid_controls(void) {
  check_html("[a](b \"t\")\n", "<a href=\"b\" title=\"t\">a</a>");
  check_html("[a](b (t))\n", "<a href=\"b\" title=\"t\">a</a>");
  check_html("[a](b (t\\(x))\n", "<a href=\"b\" title=\"t(x\">a</a>");
  check_html("[a](<b c> 't')\n", "<a href=\"b%20c\" title=\"t\">a</a>");
  check_html("[a](<b\\<c>)\n", "<a href=\"b%3Cc\">a</a>");
  check_html("[a](b(c(d)e)f)\n", "<a href=\"b(c(d)e)f\">a</a>");
  check_html("[a](b\\(c)\n", "<a href=\"b(c\">a</a>");
  check_html("![i](s \"t\")\n", "<img src=\"s\" alt=\"i\" title=\"t\" />");
  check_html("[o [n](m)](s)\n", "<p>[o <a href=\"m\">n</a>](s)</p>");
  check_html("![i [n](m)](s)\n", "<img src=\"s\" alt=\"i n\" />");
  check_html("[t][a]\n\n[a]: /u\n", "<a href=\"/u\">t</a>");
  check_html("[a][]\n\n[a]: /u\n", "<a href=\"/u\">a</a>");
  check_html("[a]\n\n[a]: /u\n", "<a href=\"/u\">a</a>");
  /* Spec: no unescaped '<' in <dest>, no unescaped '(' in a (title). */
  check_html("[a](<b<c>)\n", "<p>[a](&lt;b<c>)</p>");
  check_html("[a](b (t(x)))\n", "<p>[a](b (t(x)))</p>");
  /* Destination parenthesis nesting is capped per opener. */
  {
    const size_t cap = FIO___GFM_MAX_INLINE_BRACKETS;
    char src[2 * FIO___GFM_MAX_INLINE_BRACKETS + 32];
    for (size_t extra = 0; extra < 2; ++extra) {
      size_t n = cap + extra, at = 0;
      memcpy(src, "[a](", 4);
      at = 4;
      memset(src + at, '(', n);
      at += n;
      src[at++] = 'b';
      memset(src + at, ')', n);
      at += n;
      memcpy(src + at, ") [later](w)\n", 13);
      at += 13;
      char *html = fio_md2html(NULL, FIO_BUF_INFO2(src, at));
      CHECK(html && strstr(html, "<a href=\"w\">later</a>"));
      CHECK(!memcmp(html, extra ? "<p>[a]" : "<p><a href=", extra ? 6 : 11));
      fio_bstr_free(html);
    }
  }
}

int main(void) {
  check_malformed_families();
  check_valid_controls();
  /* The cap belongs to one opener, not the rest of a paragraph. */
  check_following_link(63, 1);
  check_following_link(64, 1);
  check_following_link(1024, 1);
  check_following_link(1024, 0);
  check_image_depth(63);
  check_image_depth(65);
  check_image_depth(4096);
  puts("GFM per-element nesting regressions passed");
  return 0;
}
