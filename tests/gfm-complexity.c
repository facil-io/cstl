/* GFM complexity hardening regression.
 * Build/run: make tests/gfm-complexity; asan/tests/gfm-complexity.
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

int main(void) {
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
