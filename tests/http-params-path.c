/* HTTP query/form field-name path splitting tests. */
#define FIO_LOG
#include "test-helpers.h"
#define FIO_HTTP
#include FIO_INCLUDE_FILE

typedef struct {
  const char *name;
  uint32_t count;
  uint8_t append;
  const char *segments[FIO_JSON_MAX_DEPTH];
} path_case_s;

int main(void) {
  static const path_case_s cases[] = {
      {"user[name]", 2, 0, {"user", "name"}},
      {"user.name", 2, 0, {"user", "name"}},
      {"user[address][country]", 3, 0, {"user", "address", "country"}},
      {"user.address.country", 3, 0, {"user", "address", "country"}},
      {"user[address].country", 3, 0, {"user", "address", "country"}},
      {"tag[]", 1, 1, {"tag"}},
      {"a[b][]", 2, 1, {"a", "b"}},
      {"a.b[]", 2, 1, {"a", "b"}},
      {"a[b.c]", 2, 0, {"a", "b.c"}},
      {"a..b", 1, 0, {"a..b"}},
      {"items[][id]", 1, 0, {"items[][id]"}},
      {"a.", 1, 0, {"a."}},
      {".a", 1, 0, {".a"}},
      {"", 1, 0, {""}},
      {"[]", 1, 0, {"[]"}},
      {"[a]", 1, 0, {"[a]"}},
      {"a[", 1, 0, {"a["}},
      {"a[b", 1, 0, {"a[b"}},
      {"a]", 1, 0, {"a]"}},
      {"a[b]c", 1, 0, {"a[b]c"}},
      {"a[]b", 1, 0, {"a[]b"}},
      {"a[[b]]", 1, 0, {"a[[b]]"}},
      {"a[][]", 1, 0, {"a[][]"}},
      {"a", 1, 0, {"a"}},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    const path_case_s *c = cases + i;
    fio_buf_info_s name = {.buf = (char *)c->name, .len = strlen(c->name)};
    fio___http_params_path_s path;
    FIO_ASSERT(!fio___http_params_path_parse(&path, name),
               "case %zu (%s): unexpected depth error", i, c->name);
    FIO_ASSERT(path.count == c->count && path.append == c->append,
               "case %zu (%s): count=%u append=%u",
               i, c->name, path.count, path.append);
    for (uint32_t j = 0; j < path.count; ++j) {
      size_t len = strlen(c->segments[j]);
      FIO_ASSERT(path.seg[j].buf >= name.buf &&
                     path.seg[j].buf <= name.buf + name.len &&
                     len <= (size_t)(name.buf + name.len - path.seg[j].buf),
                 "case %zu (%s): segment %u is outside input", i, c->name, j);
      FIO_ASSERT(path.seg[j].len == len &&
                     !memcmp(path.seg[j].buf, c->segments[j], len),
                 "case %zu (%s): segment %u mismatch", i, c->name, j);
    }
  }
  /* The root counts as level 1; [] adds one more level. */
  char many[2 * (FIO_JSON_MAX_DEPTH + 1)];
  for (size_t append = 0; append <= 1; ++append) {
    size_t accepted = FIO_JSON_MAX_DEPTH - append;
    for (size_t n = accepted; n <= accepted + 1; ++n) {
      for (size_t i = 0; i < n; ++i) {
        many[2 * i] = 'a';
        if (i + 1 < n)
          many[2 * i + 1] = '.';
      }
      size_t len = 2 * n - 1;
      if (append) {
        many[len++] = '[';
        many[len++] = ']';
      }
      many[len] = 0;
      fio_buf_info_s name = {.buf = many, .len = len};
      fio___http_params_path_s path;
      int err = fio___http_params_path_parse(&path, name);
      FIO_ASSERT((err != 0) == (n > accepted),
                 "%zu segments append=%zu: wrong depth result", n, append);
      if (!err) {
        FIO_ASSERT(path.count == n && path.append == append,
                   "%zu segments append=%zu: wrong count/append", n, append);
        for (uint32_t j = 0; j < path.count; ++j)
          FIO_ASSERT(path.seg[j].buf == many + 2 * j && path.seg[j].len == 1,
                     "%zu segments: slice %u mismatch", n, j);
      }
    }
  }
  fprintf(stderr, "HTTP params path splitter: PASS\n");
  return 0;
}
