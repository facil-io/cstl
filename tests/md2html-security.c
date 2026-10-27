/* Renderer ownership, moving allocation and adversarial alt regressions.
 * Build/run: make tests/md2html-security (also asan/ and ub/).
 */
#include "renderer-test-alloc.h"
#define FIO_MD2HTML
#include "fio-stl/include.h"
#include <stdio.h>
#include <time.h>
#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      fprintf(stderr, "FAIL %d: %s\n", __LINE__, #c);                          \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
static void late_failure(void) {
  char input[2048];
  memset(input, 'a', 1024);
  input[1024] = '\n';
  input[1025] = '\n';
  memset(input + 1026, '>', 300);
  memcpy(input + 1326, " x\n", 3);
  char *target = fio_bstr_write(NULL, "prefix", 6);
  CHECK(target);
  char *result = fio_md2html(target, FIO_BUF_INFO2(input, 1329));
  CHECK(!result);
  CHECK(fio_bstr_len(target) == 6 && !memcmp(target, "prefix", 7));
  fio_bstr_free(target);
}
static void alt_labels(void) {
  static const struct {
    const char *in, *out;
  } controls[] = {{"![x](u)", "x"},
                  {"[x](u)", "x"},
                  {"[x][ref]", "x"},
                  {"[![x](u)](v)", "x"},
                  {"[[x](u)](v)", "[x](v)"},
                  {"\\![x](u)", "!x"},
                  {"a [b] *c* \\_d", "a [b] c _d"},
                  {"[x](u [y](v))", "x"},
                  {"[x][[ref]]", "x]"}};
  for (size_t i = 0; i < sizeof(controls) / sizeof(*controls); ++i) {
    fio___md2html_renderer_s r = {0};
    fio___md2html_escape_alt_text(&r, controls[i].in, strlen(controls[i].in));
    CHECK(!r.err && r.bstr && !strcmp(r.bstr, controls[i].out));
    fio_bstr_free(r.bstr);
  }
  /* Parser bracket nesting is capped; keep accepted images within the cap. */
  const size_t depths[] = {16, FIO___GFM_MAX_INLINE_BRACKETS};
  for (size_t i = 0; i < sizeof(depths) / sizeof(*depths); ++i) {
    size_t n = depths[i];
    char *md = malloc(n * 6 + 1);
    CHECK(md);
    for (size_t j = 0; j < n; ++j)
      memcpy(md + j * 2, "![", 2);
    md[n * 2] = 'x';
    for (size_t j = 0; j < n; ++j)
      memcpy(md + n * 2 + 1 + j * 4, "](u)", 4);
    char *out = fio_md2html(NULL, FIO_BUF_INFO2(md, n * 6 + 1));
    CHECK(out && !strcmp(out, "<p><img src=\"u\" alt=\"x\" /></p>"));
    fio_bstr_free(out);
    free(md);
  }
  for (size_t n = 16; n < FIO___GFM_MAX_INLINE_BRACKETS; n *= 2) {
    char *md = malloc(n * 2 + 7);
    CHECK(md);
    memcpy(md, "![", 2);
    memset(md + 2, '[', n);
    md[n + 2] = 'x';
    memset(md + n + 3, ']', n);
    memcpy(md + n * 2 + 3, "](u)", 4);
    clock_t start = clock();
    for (size_t repeat = 0; repeat < 8; ++repeat) {
      char *out = fio_md2html(NULL, FIO_BUF_INFO2(md, n * 2 + 7));
      CHECK(out && fio_bstr_len(out) == n * 2 + 30);
      fio_bstr_free(out);
    }
    fprintf(stderr,
            "alt brackets %zu: %.6fs\n",
            n,
            (double)(clock() - start) / CLOCKS_PER_SEC);
    free(md);
  }
}
int main(int argc, char **argv) {
  if (argc == 1 || !strcmp(argv[1], "late"))
    late_failure();
  if (argc == 1 || !strcmp(argv[1], "alt"))
    alt_labels();
  CHECK(!renderer_alloc_live);
  return 0;
}
