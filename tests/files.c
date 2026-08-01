/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

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
  /* Path traversal tests for fio_filename_is_unsafe / _url.
   *
   * Expected values depend on the separator set each function guards:
   * - On Windows, Win32 APIs accept BOTH '/' and '\\' as separators, so
   *   BOTH functions guard both flavors (fixed 2026-07-31: previously
   *   fio_filename_is_unsafe guarded '\\' only, leaving Unix-flavored
   *   traversal open on Windows - the same bug class as the public one).
   * - On POSIX systems '\\' is an ordinary filename character, so Windows
   *   flavored attempts are inert and must be reported as safe (0) by both
   *   functions.
   * - Both functions are therefore equivalent; fio_filename_is_unsafe_url
   *   remains the canonical guard for URL-decoded paths.
   */
#if FIO_OS_WIN
#define FIO___UNSAFE_EXP(posix, win) (win)
#else
#define FIO___UNSAFE_EXP(posix, win) (posix)
#endif
  struct {
    const char *str;
    int unsafe;     /* expected fio_filename_is_unsafe result     */
    int unsafe_url; /* expected fio_filename_is_unsafe_url result */
  } filename_unsafe_test[] = {
      // clang-format off
      /* safe names on every system */
      {.str = "index.html", .unsafe = 0, .unsafe_url = 0},
      {.str = "folder/file.txt", .unsafe = 0, .unsafe_url = 0},
      {.str = "folder\\file.txt", .unsafe = 0, .unsafe_url = 0},
      {.str = "C:\\folder\\file.txt", .unsafe = 0, .unsafe_url = 0},
      {.str = "/absolute/path/file", .unsafe = 0, .unsafe_url = 0},
      {.str = "/my_folder/.name", .unsafe = 0, .unsafe_url = 0},
      {.str = "file..name", .unsafe = 0, .unsafe_url = 0},
      {.str = "..file", .unsafe = 0, .unsafe_url = 0},
      {.str = "....", .unsafe = 0, .unsafe_url = 0},
      {.str = "/.../file", .unsafe = 0, .unsafe_url = 0},
      {.str = "a/./b", .unsafe = 0, .unsafe_url = 0},
      {.str = "folder/", .unsafe = 0, .unsafe_url = 0},
      {.str = "/", .unsafe = 0, .unsafe_url = 0},
      {.str = "\\", .unsafe = 0, .unsafe_url = 0},
      {.str = ".", .unsafe = 0, .unsafe_url = 0},
      {.str = "", .unsafe = 0, .unsafe_url = 0},
      /* caught on every system, by both functions */
      {.str = "..", .unsafe = 1, .unsafe_url = 1},
      /* Unix flavored attempts (caught by both functions on every system) */
      {.str = "../", .unsafe = 1, .unsafe_url = 1},
      {.str = "../file", .unsafe = 1, .unsafe_url = 1},
      {.str = "../../etc/passwd", .unsafe = 1, .unsafe_url = 1},
      {.str = "/..", .unsafe = 1, .unsafe_url = 1},
      {.str = "/../", .unsafe = 1, .unsafe_url = 1},
      {.str = "folder/../file", .unsafe = 1, .unsafe_url = 1},
      {.str = "folder/..", .unsafe = 1, .unsafe_url = 1},
      {.str = "a/b/../../c", .unsafe = 1, .unsafe_url = 1},
      {.str = "//", .unsafe = 1, .unsafe_url = 1},
      {.str = "//server/share", .unsafe = 1, .unsafe_url = 1},
      {.str = "folder//file", .unsafe = 1, .unsafe_url = 1},
      {.str = "../..\\file", .unsafe = 1, .unsafe_url = 1}, /* mixed flavors */
      /* Windows flavored attempts (inert on POSIX, both 1 on Windows) */
      {.str = "..\\", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      {.str = "..\\file", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      {.str = "..\\..\\windows\\system32", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      {.str = "\\..", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      {.str = "folder\\..\\file", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      {.str = "folder\\..", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      {.str = "\\\\", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      {.str = "\\\\server\\share", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      {.str = "folder\\\\file", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      /* mixed flavors - inert on POSIX ('\\' is not a separator there) */
      {.str = "..\\../file", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      /* mixed flavors - on Windows the '/' traversal is caught by both */
      {.str = "folder/..\\file", .unsafe = FIO___UNSAFE_EXP(0, 1), .unsafe_url = FIO___UNSAFE_EXP(0, 1)},
      {.str = NULL}, // clang-format on
  };
#undef FIO___UNSAFE_EXP
  FIO_ASSERT(fio_filename_is_unsafe(NULL) == 0 &&
                 fio_filename_is_unsafe_url(NULL) == 0,
             "NULL path should be reported as safe (nothing to do)");
  for (size_t i = 0; filename_unsafe_test[i].str; ++i) {
    const char *s = filename_unsafe_test[i].str;
    int unsafe = fio_filename_is_unsafe(s);
    int unsafe_url = fio_filename_is_unsafe_url(s);
    FIO_ASSERT(unsafe == filename_unsafe_test[i].unsafe,
               "fio_filename_is_unsafe(\"%s\") == %d (expected %d)",
               s,
               unsafe,
               filename_unsafe_test[i].unsafe);
    FIO_ASSERT(unsafe_url == filename_unsafe_test[i].unsafe_url,
               "fio_filename_is_unsafe_url(\"%s\") == %d (expected %d)",
               s,
               unsafe_url,
               filename_unsafe_test[i].unsafe_url);
  }
  return 0;
}
