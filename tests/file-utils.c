/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

FIO_SFUNC void FIO_NAME_TEST(stl, math_speed)(void) {
  uint64_t n = 0, d = 1;
  uint64_t start[2], end[2];
  start[0] = fio_time_nano();
  for (size_t i = 0; i < 64; ++i) {
    n = (n << 7) ^ 0xAA;
    uint64_t q = 0, r = 0;
    FIO_COMPILER_GUARD;
    for (size_t j = 0; j < 64; ++j) {
      d = (d << 3) ^ 0xAA;
      FIO_COMPILER_GUARD;
      fio_math_div(&q, &r, &n, &d, 1);
      FIO_COMPILER_GUARD;
    }
    (void)q;
  }
  end[0] = fio_time_nano();
  n = 0, d = 1;
  start[1] = fio_time_nano();
  for (size_t i = 0; i < 64; ++i) {
    n = (n << 7) ^ 0xAA;
    uint64_t q = 0;
    FIO_COMPILER_GUARD;
    for (size_t j = 0; j < 64; ++j) {
      d = (d << 3) ^ 0xAA;
      FIO_COMPILER_GUARD;
      q = n / d;
      FIO_COMPILER_GUARD;
    }
    (void)q;
  }
  end[1] = fio_time_nano();
  FIO_LOG_INFO("\t fio_math_div test took %zu us (vs. %zu us) for a single "
               "64 bit word.",
               (size_t)(end[0] - start[0]),
               (size_t)(end[1] - start[1]));
}

int main(void) {
  struct {
    const char *str;
    fio_filename_s result;
  } filename_test[] = {
      // clang-format off
      {.str = "/", .result = {.folder = FIO_BUF_INFO2((char*)0, 1), .basename = FIO_BUF_INFO2(NULL, 0), .ext = FIO_BUF_INFO2(NULL, 0)}},
      {.str = "/.", .result = {.folder = FIO_BUF_INFO2((char*)0, 1), .basename = FIO_BUF_INFO2((char*)1, 1), .ext = FIO_BUF_INFO2(NULL, 0)}},
      {.str = "/..", .result = {.folder = FIO_BUF_INFO2((char*)0, 1), .basename = FIO_BUF_INFO2((char*)1, 2), .ext = FIO_BUF_INFO2(NULL, 0)}},
      {.str = "name", .result = {.folder = FIO_BUF_INFO2(NULL, 0), .basename = FIO_BUF_INFO2(0, 4), .ext = FIO_BUF_INFO2(NULL, 0)}},
      {.str = "name.ext", .result = {.folder = FIO_BUF_INFO2(NULL, 0), .basename = FIO_BUF_INFO2((char*)0, 4), .ext = FIO_BUF_INFO2((char*)5, 3)}},
      {.str = ".name", .result = {.folder = FIO_BUF_INFO2(NULL, 0), .basename = FIO_BUF_INFO2((char*)0, 5), .ext = FIO_BUF_INFO2(NULL, 0)}},
      {.str = "/.name", .result = {.folder = FIO_BUF_INFO2((char*)0, 1), .basename = FIO_BUF_INFO2((char*)1, 5), .ext = FIO_BUF_INFO2(NULL, 0)}},
      {.str = "/my_folder/.name", .result = {.folder = FIO_BUF_INFO2((char*)0, 11), .basename = FIO_BUF_INFO2((char*)11, 5), .ext = FIO_BUF_INFO2(NULL, 0)}},
      {.str = "/my_folder/name.ext", .result = {.folder = FIO_BUF_INFO2((char*)0, 11), .basename = FIO_BUF_INFO2((char*)11, 4), .ext = FIO_BUF_INFO2((char*)16, 3)}},
      {.str = NULL}, // clang-format on
  };
  for (size_t i = 0; filename_test[i].str; ++i) {
    fio_filename_s r = fio_filename_parse(filename_test[i].str);
    FIO_ASSERT(
        r.folder.len == filename_test[i].result.folder.len &&
            r.basename.len == filename_test[i].result.basename.len &&
            r.ext.len == filename_test[i].result.ext.len &&
            ((!r.folder.buf && !filename_test[i].result.folder.len) ||
             r.folder.buf == (filename_test[i].str +
                              (size_t)filename_test[i].result.folder.buf)) &&
            ((!r.basename.buf && !filename_test[i].result.basename.len) ||
             r.basename.buf ==
                 (filename_test[i].str +
                  (size_t)filename_test[i].result.basename.buf)) &&
            ((!r.ext.buf && !filename_test[i].result.ext.len) ||
             r.ext.buf == (filename_test[i].str +
                           (size_t)filename_test[i].result.ext.buf)),
        "fio_filename_parse error for %s"
        "\n\t folder:    (%zu) %.*s (%p)"
        "\n\t basename:  (%zu) %.*s (%p)"
        "\n\t extension: (%zu) %.*s (%p)",
        filename_test[i].str,
        r.folder.len,
        (int)r.folder.len,
        (r.folder.buf ? r.folder.buf : "null"),
        r.folder.buf,
        r.basename.len,
        (int)r.basename.len,
        (r.basename.buf ? r.basename.buf : "null"),
        r.basename.buf,
        r.ext.len,
        (int)r.ext.len,
        (r.ext.buf ? r.ext.buf : "null"),
        r.ext.buf);
  }
  FIO_NAME_TEST(stl, math_speed)();
}
