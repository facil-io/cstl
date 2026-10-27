/* Length-delimited, bytecode and callback ownership regression tests.
 * Build/run: make tests/mustache-security (also asan/ and ub/).
 */
#include "renderer-test-alloc.h"
#define FIO_MUSTACHE
#include "fio-stl/include.h"
#include <stdio.h>
#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      fprintf(stderr, "FAIL %d: %s\n", __LINE__, #c);                          \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
static void oversized(void) {
  size_t n = 65537;
  char *s = malloc(n + 12);
  CHECK(s);
  memcpy(s, "{{#", 3);
  memset(s + 3, 'a', n);
  memcpy(s + 3 + n, "}}x{{/a}}", 9);
  fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO2(s, n + 12));
  CHECK(!m);
  s[2] = '^';
  m = fio_mustache_load(.data = FIO_BUF_INFO2(s, n + 12));
  CHECK(!m);
  free(s);
  /* Reject an unrepresentable instruction without allocating gigabytes. */
  fio___mustache_parser_s p = {.root = fio_bstr_reserve(NULL, 1)};
  CHECK(p.root);
  fio___mustache_write2 FIO_NOOP(
      &p,
      (fio_string_write_s[]){FIO_STRING_WRITE_STR2("x", UINT32_MAX), {0}});
  CHECK(p.err && fio_bstr_len(p.root) == 0);
  fio_bstr_free(p.root);
}
static void exact_views(void) {
  const char *cases[] = {"@",
                         "@:\n",
                         "---\n",
                         "---\n---",
                         "---\n---\n",
                         "---\na\n---",
                         "---\na\n---\r\nX"};
  const char *expected[] = {"@", "", "", "", "", "", "X"};
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    size_t n = strlen(cases[i]);
    char *s = malloc(n);
    CHECK(s);
    memcpy(s, cases[i], n);
    fio_mustache_s *m = fio_mustache_load(.data = FIO_BUF_INFO2(s, n));
    if (m) {
      char *out = fio_mustache_build(m, .ctx = NULL);
      CHECK((!out && !*expected[i]) || (out && !strcmp(out, expected[i])));
      fio_bstr_free(out);
      fio_mustache_free(m);
    } else
      CHECK(!*expected[i]);
    free(s);
  }
}
static void filename_view(void) {
  char *fn = malloc(1);
  CHECK(fn);
  *fn = 'x';
  fio_mustache_s *m = fio_mustache_load(.filename = FIO_BUF_INFO2(fn, 1),
                                        .data = FIO_BUF_INFO1((char *)"x"));
  CHECK(m);
  fio_mustache_free(m);
  free(fn);
}
static size_t acquired, released;
typedef struct {
  int owned, truth;
  char text[8];
} value_s;
static void *get_var(void *ctx, fio_buf_info_s name) {
  CHECK(ctx);
  value_s *parent = ctx;
  CHECK(parent->text[0] == 'v');
  if (memchr(name.buf, '.', name.len))
    return NULL;
  value_s *v = malloc(sizeof(*v));
  CHECK(v);
  *v = (value_s){.owned = 1,
                 .truth = !(name.len == 1 && *name.buf == 'f'),
                 .text = "value"};
  ++acquired;
  return v;
}
static fio_buf_info_s string_value(void *p) {
  value_s *v = p;
  CHECK(v->text[0] == 'v');
  return FIO_BUF_INFO2(v->text, 5);
}
static int truth(void *p) { return p && ((value_s *)p)->truth; }
static size_t array_length(void *p) { return p ? 2 : 0; }
static void *array_item(void *p, size_t i) {
  (void)i;
  return get_var(p, FIO_BUF_INFO1((char *)"a"));
}
static void release(void *p) {
  if (!p)
    return;
  value_s *v = p;
  CHECK(v->owned);
  ++released;
  memset(v, 0, sizeof(*v));
  free(v);
}
static void ownership(int selected) {
  value_s root = {.text = "value", .truth = 1};
  const char *cases[] = {"{{a}}",
                         "{{.}}{{.}}",
                         "{{#f}}X{{/f}}",
                         "{{^f}}X{{/f}}",
                         "{{a.b.c}}",
                         "{{#a}}{{.}}{{/a}}",
                         "{{#.}}{{.}}{{/.}}"};
  const char *expected[] =
      {"value", "valuevalue", "", "X", "value", "value", "value"};
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    if (selected >= 0 && i != (size_t)selected)
      continue;
    acquired = released = 0;
    fio_mustache_s *m =
        fio_mustache_load(.data = FIO_BUF_INFO1((char *)cases[i]));
    CHECK(m);
    char *out = fio_mustache_build(m,
                                   .ctx = &root,
                                   .get_var = get_var,
                                   .var2str = string_value,
                                   .release_var = release,
                                   .var_is_truthful = truth);
    CHECK(acquired == released);
    CHECK(root.text[0] == 'v');
    CHECK((!out && !*expected[i]) || (out && !strcmp(out, expected[i])));
    fio_bstr_free(out);
    fio_mustache_free(m);
  }
  if (selected < 0) {
    acquired = released = 0;
    fio_mustache_s *m =
        fio_mustache_load(.data = FIO_BUF_INFO1((char *)"{{#a}}{{.}}{{/a}}"));
    CHECK(m);
    char *out = fio_mustache_build(m,
                                   .ctx = &root,
                                   .get_var = get_var,
                                   .var2str = string_value,
                                   .release_var = release,
                                   .var_is_truthful = truth,
                                   .array_length = array_length,
                                   .get_var_index = array_item);
    CHECK(out && !strcmp(out, "valuevalue") && acquired == 3 && released == 3);
    fio_bstr_free(out);
    fio_mustache_free(m);
  }
}
static size_t loads, frees;
static fio_buf_info_s empty_loader(fio_buf_info_s fn, void *u) {
  (void)fn;
  (void)u;
  ++loads;
  char *s = malloc(1);
  CHECK(s);
  return FIO_BUF_INFO2(s, 0);
}
static void empty_free(fio_buf_info_s d, void *u) {
  (void)u;
  if (d.buf) {
    ++frees;
    free(d.buf);
  }
}
static void loaders(void) {
  loads = frees = 0;
  fio_mustache_s *m =
      fio_mustache_load(.data = FIO_BUF_INFO1((char *)"x{{>empty}}"),
                        .load_file_data = empty_loader,
                        .free_file_data = empty_free);
  CHECK(m && loads && loads == frees);
  fio_mustache_free(m);
}
static void paths(void) {
  loads = frees = 0;
  fio_mustache_s *m = fio_mustache_load(
          .data = FIO_BUF_INFO1(
              (char *)"x{{>../secret}}{{>..\\secret}}{{>a\\..\\secret}}"),
          .load_file_data = empty_loader,
          .free_file_data = empty_free);
  CHECK(m && !loads);
  fio_mustache_free(m);
}
int main(int argc, char **argv) {
  if (argc == 1 || !strcmp(argv[1], "oversized"))
    oversized();
  if (argc == 1 || !strcmp(argv[1], "views"))
    exact_views();
  if (argc == 1 || !strcmp(argv[1], "filename"))
    filename_view();
  if (argc == 1 || !strcmp(argv[1], "ownership"))
    ownership(argc > 2 ? atoi(argv[2]) : -1);
  if (argc == 1 || !strcmp(argv[1], "paths"))
    paths();
  if (argc == 1 || !strcmp(argv[1], "loaders"))
    loaders();
  CHECK(!renderer_alloc_live);
  return 0;
}
