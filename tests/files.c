/* *****************************************************************************
Test
***************************************************************************** */
#ifndef _WIN32
#define TEST_FD_HOOKS /* FIO___FD_PREAD, FSYNC and COPY_RANGE hooks */
#endif
#include "test-helpers.h"

/* fio_filename_tmp: temporary files are anonymous (no names left behind). */
static void test_filename_tmp(void) {
  fprintf(stderr, "* Testing fio_filename_tmp leaves no files behind\n");
  int fds[4];
#if FIO_OS_POSIX
  char dir[128];
  snprintf(dir, sizeof(dir), "tmp/test-files-tmp-%d", (int)getpid());
  FIO_ASSERT(!fio_filename_make_path(.path = dir), "couldn't create %s", dir);
  char *old = test_tmpdir_set(dir);
#endif
  for (size_t i = 0; i < 4; ++i) {
    fds[i] = fio_filename_tmp();
    FIO_ASSERT(fds[i] != -1, "fio_filename_tmp failed (%d)", errno);
    FIO_ASSERT(fio_fd_write(fds[i], "temp data", 9) == 9, "tmp write failed");
    char buf[16] = {0};
    FIO_ASSERT(fio_fd_read(fds[i], buf, 9, 0) == 9 &&
                   !memcmp(buf, "temp data", 9),
               "tmp read back failed");
  }
#if FIO_OS_POSIX
  /* the names are gone while the files are still open */
  long open_count = test_dir_count(dir);
#endif
  for (size_t i = 0; i < 4; ++i)
    close(fds[i]);
#if FIO_OS_POSIX
  long closed_count = test_dir_count(dir);
  test_tmpdir_restore(old);
  fio_filename_remove(.path = dir, .recursive = 1);
  FIO_ASSERT(!open_count,
             "%ld named temporary file(s) left (open)",
             open_count);
  FIO_ASSERT(!closed_count, "%ld temporary file(s) left behind", closed_count);
#endif
}

#if FIO_OS_POSIX
/* *****************************************************************************
fio_fd_read: EINTR retry, partial reads, short reads (FIO___FD_PREAD hook)
***************************************************************************** */

/* Opens (creates / truncates) `name` for reading and writing. */
static int test_files_open_rw(const char *name) {
  int fd = open(name, O_CREAT | O_RDWR | O_TRUNC, 0600);
  FIO_ASSERT(fd != -1, "couldn't open %s (%d)", name, errno);
  return fd;
}

/* Fills `buf` with a position dependent pattern. */
static void test_files_pattern(char *buf, size_t len, size_t seed) {
  for (size_t i = 0; i < len; ++i)
    buf[i] = (char)('A' + ((i * 7 + seed) % 53));
}

/* Returns 1 if the file `name` holds exactly `len` bytes of `expect`. */
static int test_files_content_is(const char *name,
                                 const char *expect,
                                 size_t len) {
  int fd = open(name, O_RDONLY);
  if (fd == -1)
    return 0;
  char tmp[4096];
  size_t total = 0;
  int r = 1;
  for (ssize_t got; r && (got = read(fd, tmp, sizeof(tmp))) > 0;) {
    r = (total + (size_t)got <= len) && !memcmp(tmp, expect + total, got);
    total += (size_t)got;
  }
  close(fd);
  return r && total == len;
}

static void test_fd_read_hooks(const char *dir) {
  fprintf(stderr, "* Testing fio_fd_read EINTR retry and short reads\n");
  char name[256];
  snprintf(name, sizeof(name), "%s/read.bin", dir);
  const size_t len = 20000;
  char *src = (char *)malloc(len), *dst = (char *)malloc(len);
  FIO_ASSERT_ALLOC(src && dst);
  test_files_pattern(src, len, 1);
  int fd = test_files_open_rw(name);
  FIO_ASSERT(fio_fd_write(fd, src, len) == (ssize_t)len, "read: write");
  /* EINTR (several, between partial reads) is retried transparently */
  test_fd_hooks_reset();
  test_fd_read.eintr = 3;
  test_fd_read.max = 1000;
  FIO_MEMSET(dst, 0, len);
  FIO_ASSERT(fio_fd_read(fd, dst, len, 0) == len && !memcmp(dst, src, len),
             "EINTR / partial reads should be retried until complete");
  FIO_ASSERT(test_fd_read.eintr_seen == 3 && test_fd_read.calls == 3 + 20,
             "EINTR retry: %zu EINTR, %zu calls",
             test_fd_read.eintr_seen,
             test_fd_read.calls);
  /* EOF before `len`: returns what was read (errno untouched) */
  test_fd_hooks_reset();
  test_fd_read.fail_at = 7000;
  errno = 0;
  FIO_ASSERT(fio_fd_read(fd, dst, len, 1000) == 6000 && !errno,
             "short read (EOF) should return the bytes read, errno unset");
  /* an error mid-way: returns what was read, errno from the failed call */
  test_fd_read.fail_errno = EIO;
  errno = 0;
  FIO_ASSERT(fio_fd_read(fd, dst, len, 0) == 7000 && errno == EIO,
             "short read (error) should return the bytes read, keep errno");
  test_fd_hooks_reset();
  close(fd);
  unlink(name);
  free(src);
  free(dst);
}

/* *****************************************************************************
fio_filename_overwrite (atomic) / fio_fd_save
***************************************************************************** */

/** Sets `HOME` to `path`, returning the old value (free with restore). */
static char *test_home_set(const char *path) {
  const char *old = getenv("HOME");
  char *r = old ? strdup(old) : NULL;
  FIO_ASSERT(!setenv("HOME", path, 1), "setenv(HOME) failed");
  return r;
}
static void test_home_restore(char *old) {
  if (old) {
    setenv("HOME", old, 1);
    free(old);
  } else {
    unsetenv("HOME");
  }
}

static void test_filename_overwrite(const char *dir) {
  fprintf(stderr, "* Testing fio_filename_overwrite (atomic, buffer source)\n");
  char name[256], home_name[256];
  snprintf(name, sizeof(name), "%s/saved.txt", dir);
  const mode_t um = umask(022);
  umask(um);
  const long base = test_dir_count(dir);
  /* new file */
  test_fd_hooks_reset();
  FIO_ASSERT(!fio_filename_overwrite(name, "hello world", 11), "overwrite: new file");
  FIO_ASSERT(test_files_content_is(name, "hello world", 11), "overwrite: content");
  FIO_ASSERT(test_fd_fsync.calls == 1, "overwrite: fsync called once");
  struct stat st;
  FIO_ASSERT(!stat(name, &st) && (st.st_mode & 0777) == (0666 & ~um),
             "overwrite: file mode %o (umask %o)",
             (unsigned)(st.st_mode & 0777),
             (unsigned)um);
  FIO_ASSERT(test_dir_count(dir) == base + 1, "overwrite: sibling file left");
  /* overwrite (longer, then shorter, then empty) */
  int old_fd = open(name, O_RDONLY); /* the replaced file stays intact */
  FIO_ASSERT(old_fd != -1, "overwrite: open old file");
  FIO_ASSERT(!fio_filename_overwrite(name, "a longer replacement", 20) &&
                 test_files_content_is(name, "a longer replacement", 20),
             "overwrite: overwrite (longer)");
  {
    char old_buf[16] = {0};
    FIO_ASSERT(read(old_fd, old_buf, 15) == 11 &&
                   !memcmp(old_buf, "hello world", 11),
               "overwrite: replaced (renamed over), not truncated in place");
    close(old_fd);
  }
  FIO_ASSERT(!fio_filename_overwrite(name, "short", 5) &&
                 test_files_content_is(name, "short", 5),
             "overwrite: overwrite (shorter)");
  FIO_ASSERT(!fio_filename_overwrite(name, NULL, 0) && !fio_filename_size(name) &&
                 fio_filename_is_folder(dir),
             "overwrite: empty buffer -> empty file");
  FIO_ASSERT(test_dir_count(dir) == base + 1, "overwrite: sibling file left (2)");
  /* "~/" is relative to HOME (with or without a trailing separator) */
  char home_slash[256];
  snprintf(home_slash, sizeof(home_slash), "%s/", dir);
  for (size_t i = 0; i < 2; ++i) {
    char *old = test_home_set(i ? home_slash : dir);
    int r = fio_filename_overwrite("~/home-saved.txt", "at home", 7);
    test_home_restore(old);
    snprintf(home_name, sizeof(home_name), "%s/home-saved.txt", dir);
    FIO_ASSERT(!r && test_files_content_is(home_name, "at home", 7),
               "overwrite: ~/ expansion (HOME %s)",
               (i ? home_slash : dir));
    unlink(home_name);
  }
  /* failure: missing folder (nothing created, errno kept) */
  char bad[288];
  snprintf(bad, sizeof(bad), "%s/no-such-folder/x.txt", dir);
  errno = 0;
  FIO_ASSERT(fio_filename_overwrite(bad, "x", 1) == -1 && errno == ENOENT,
             "overwrite: missing folder should fail with ENOENT (%d)",
             errno);
  /* failure: fsync fails -> target untouched, sibling removed, errno kept */
  FIO_ASSERT(!fio_filename_overwrite(name, "original", 8), "overwrite: original");
  test_fd_fsync.fail_errno = EIO;
  errno = 0;
  FIO_ASSERT(fio_filename_overwrite(name, "replacement", 11) == -1 && errno == EIO,
             "overwrite: fsync failure should fail with EIO (%d)",
             errno);
  test_fd_hooks_reset();
  FIO_ASSERT(test_files_content_is(name, "original", 8),
             "overwrite: target untouched after fsync failure");
  /* failure: rename fails (target is a non-empty folder) */
  char folder[288], inner[320];
  snprintf(folder, sizeof(folder), "%s/a-folder", dir);
  snprintf(inner, sizeof(inner), "%s/inner", folder);
  FIO_ASSERT(!fio_filename_make_path(.path = folder) &&
                 !fio_filename_overwrite(inner, "in", 2),
             "overwrite: folder setup");
  errno = 0;
  FIO_ASSERT(fio_filename_overwrite(folder, "x", 1) == -1 && errno &&
                 fio_filename_is_folder(folder) &&
                 test_files_content_is(inner, "in", 2),
             "overwrite: rename failure should fail, folder untouched (%d)",
             errno);
  fio_filename_remove(.path = folder, .recursive = 1);
  FIO_ASSERT(test_dir_count(dir) == base + 1, "overwrite: sibling left (errors)");
  /* invalid arguments */
  errno = 0;
  FIO_ASSERT(fio_filename_overwrite(NULL, "x", 1) == -1 && errno == EINVAL &&
                 fio_filename_overwrite("", "x", 1) == -1 &&
                 fio_filename_overwrite(name, NULL, 1) == -1,
             "overwrite: invalid arguments");
  unlink(name);
}

static void test_fd_save(const char *dir) {
  fprintf(stderr, "* Testing fio_fd_save (fd source)\n");
  char src_name[256], name[256];
  snprintf(src_name, sizeof(src_name), "%s/source.bin", dir);
  snprintf(name, sizeof(name), "%s/copy.bin", dir);
  const size_t len = 100000 + 17;
  char *src = (char *)malloc(len);
  FIO_ASSERT_ALLOC(src);
  test_files_pattern(src, len, 3);
  int fd = test_files_open_rw(src_name);
  FIO_ASSERT(fio_fd_write(fd, src, len) == (ssize_t)len, "fd_save: source");
  const long base = test_dir_count(dir);
  const size_t part = len - 7; /* [0, part) of a longer file */

  /* default: the kernel copy on Linux (ENOSYS -> block copy elsewhere) */
  test_fd_hooks_reset();
  FIO_ASSERT(!fio_fd_save(fd, name, part) &&
                 test_files_content_is(name, src, part),
             "fd_save: default path");
  FIO_ASSERT(test_fd_copy.calls >= 1 && test_fd_fsync.calls == 1,
             "fd_save: kernel copy attempted, fsync called");
#if defined(__linux__)
  fprintf(stderr,
          "  - Linux copy_file_range copied %zu / %zu bytes (%zu calls)\n",
          test_fd_copy.bytes,
          part,
          test_fd_copy.calls);
#else
  FIO_ASSERT(!test_fd_copy.bytes, "fd_save: no kernel copy (fallback)");
#endif
  /* the kernel copy loop (emulated, partial calls) */
  test_fd_hooks_reset();
  test_fd_copy.emulate = 1;
  test_fd_copy.max = 3000;
  FIO_ASSERT(!fio_fd_save(fd, name, part) &&
                 test_files_content_is(name, src, part),
             "fd_save: kernel copy loop");
  FIO_ASSERT(test_fd_copy.bytes == part && !test_fd_read.calls,
             "fd_save: copied by the kernel path only (%zu bytes, %zu reads)",
             test_fd_copy.bytes,
             test_fd_read.calls);
  /* fallback continues from where the kernel copy stopped (EXDEV, EOF) */
  const int stop_errno[] = {EXDEV, ENOSYS, EINVAL, 0};
  for (size_t i = 0; i < sizeof(stop_errno) / sizeof(stop_errno[0]); ++i) {
    test_fd_hooks_reset();
    test_fd_copy.emulate = 1;
    test_fd_copy.limit = 12345;
    test_fd_copy.fail_errno = stop_errno[i];
    test_fd_read.max = 5000; /* and partial block reads */
    FIO_ASSERT(!fio_fd_save(fd, name, part) &&
                   test_files_content_is(name, src, part),
               "fd_save: fallback after kernel copy stop (errno %d)",
               stop_errno[i]);
    FIO_ASSERT(test_fd_copy.bytes == 12345 && test_fd_read.calls,
               "fd_save: fallback continued after %zu kernel bytes",
               test_fd_copy.bytes);
  }
  /* fallback from the start (EINTR during block reads is retried) */
  test_fd_hooks_reset();
  test_fd_copy.limit = 0;
  test_fd_copy.fail_errno = EOPNOTSUPP;
  test_fd_read.eintr = 2;
  FIO_ASSERT(!fio_fd_save(fd, name, part) &&
                 test_files_content_is(name, src, part) &&
                 test_fd_read.eintr_seen == 2,
             "fd_save: block copy with EINTR");
  /* empty source range */
  test_fd_hooks_reset();
  FIO_ASSERT(!fio_fd_save(fd, name, 0) && !fio_filename_size(name),
             "fd_save: empty range");
  FIO_ASSERT(test_dir_count(dir) == base + 1, "fd_save: sibling left");
  /* read failure (block copy) -> target untouched, errno kept */
  FIO_ASSERT(!fio_filename_overwrite(name, "original", 8), "fd_save: original");
  test_fd_hooks_reset();
  test_fd_copy.limit = 0;
  test_fd_copy.fail_errno = EXDEV;
  test_fd_read.fail_at = 50000;
  test_fd_read.fail_errno = EIO;
  errno = 0;
  FIO_ASSERT(fio_fd_save(fd, name, part) == -1 && errno == EIO,
             "fd_save: read error should fail with EIO (%d)",
             errno);
  /* short source (EOF before `len`) -> EIO */
  test_fd_read.fail_errno = 0;
  errno = 0;
  FIO_ASSERT(fio_fd_save(fd, name, part) == -1 && errno == EIO,
             "fd_save: short source should fail with EIO (%d)",
             errno);
  test_fd_hooks_reset();
  errno = 0;
  FIO_ASSERT(fio_fd_save(fd, name, len + 10) == -1 && errno == EIO,
             "fd_save: len past EOF should fail with EIO (%d)",
             errno);
  FIO_ASSERT(test_files_content_is(name, "original", 8),
             "fd_save: target untouched after failures");
  FIO_ASSERT(test_dir_count(dir) == base + 1, "fd_save: sibling left (errors)");
  /* invalid fd */
  errno = 0;
  FIO_ASSERT(fio_fd_save(-1, name, 1) == -1 && errno == EBADF,
             "fd_save: invalid fd");
  /* "~/" expansion */
  char *old = test_home_set(dir);
  int r = fio_fd_save(fd, "~/copy.bin", part);
  test_home_restore(old);
  FIO_ASSERT(!r && test_files_content_is(name, src, part),
             "fd_save: ~/ expansion");
  close(fd);
  unlink(src_name);
  unlink(name);
  free(src);
}

static void test_filename_save(void) {
  char dir[128];
  snprintf(dir, sizeof(dir), "tmp/test-files-save-%d", (int)getpid());
  fio_filename_remove(.path = dir, .recursive = 1);
  FIO_ASSERT(!fio_filename_make_path(.path = dir), "couldn't create %s", dir);
  test_fd_read_hooks(dir);
  test_filename_overwrite(dir);
  test_fd_save(dir);
  test_fd_hooks_reset();
  long left = test_dir_count(dir);
  fio_filename_remove(.path = dir, .recursive = 1);
  FIO_ASSERT(!left, "%ld file(s) left behind by save tests", left);
}
#endif /* FIO_OS_POSIX */

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
  test_filename_tmp();
#if FIO_OS_POSIX
  test_filename_save();
#endif
  return 0;
}
