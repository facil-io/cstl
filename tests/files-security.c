/* Files report regressions. Interpose OS calls to reproduce bounded swaps and
 * cleanup failures deterministically, without racing another process. */
#define FIO_LOG
#include "fio-stl/include.h"
#if !FIO_OS_WIN
#include <dirent.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>

static int test_open(const char *, int, ...);
static int test_openat(int, const char *, int, ...);
static int test_unlink(const char *);
static int test_close(int);
static int test_fsync(int);
#define open   test_open
#define openat test_openat
#define unlink test_unlink
#define close  test_close
#define fsync  test_fsync
#endif
#define FIO_FILES
#include "fio-stl/include.h"
#if !FIO_OS_WIN
#undef open
#undef openat
#undef unlink
#undef close
#undef fsync

static struct {
  int tmp; /* 1: force named fallback; 2: emulate successful O_TMPFILE */
  int unlink_error;
  int close_error;
  int fsync_error;
  int swap;
  int closed;
  int tmp_calls;
  int named_calls;
  size_t path_len;
  char swap_path[512], swap_held[512], swap_target[512];
  char residue[512];
} hook;

static void test_swap(const char *path) {
  const char *base = strrchr(hook.swap_path, '/');
  if (!hook.swap ||
      (strcmp(path, hook.swap_path) && (!base || strcmp(path, base + 1))))
    return;
  int replacement = hook.swap;
  hook.swap = 0;
  FIO_ASSERT(!rename(hook.swap_path, hook.swap_held) &&
                 !(replacement == 2 ? mkdir(hook.swap_path, 0700)
                                     : symlink(hook.swap_target, hook.swap_path)),
             "swap setup (%d)",
             errno);
}
static int test_tmp_emulated(void) {
  int fd = open("emulated-anonymous", O_CREAT | O_EXCL | O_RDWR, 0600);
  FIO_ASSERT(fd != -1 && !unlink("emulated-anonymous"), "emulate O_TMPFILE");
  return fd;
}
static int test_open(const char *path, int flags, ...) {
  int mode = 0;
  if (flags & O_CREAT) {
    va_list a;
    va_start(a, flags);
    mode = va_arg(a, int);
    va_end(a);
  }
#ifdef O_TMPFILE
  if ((flags & O_TMPFILE) == O_TMPFILE && hook.tmp) {
    ++hook.tmp_calls;
    hook.path_len = strlen(path);
    if (hook.tmp == 2)
      return test_tmp_emulated();
    errno = EOPNOTSUPP;
    return -1;
  }
#endif
  if (hook.tmp && (flags & O_CREAT) && strstr(path, "facil_io_tmp_")) {
    ++hook.named_calls;
    hook.path_len = strlen(path);
    if (hook.tmp == 2)
      return test_tmp_emulated();
    snprintf(hook.residue, sizeof(hook.residue), "%s", path);
  }
  return open(path, flags, mode);
}
static int test_openat(int fd, const char *path, int flags, ...) {
  test_swap(path);
  return openat(fd, path, flags, 0600);
}
static int test_unlink(const char *path) {
  if (hook.tmp == 2)
    return 0;
  if (hook.unlink_error) {
    snprintf(hook.residue, sizeof(hook.residue), "%s", path);
    errno = hook.unlink_error;
    return -1;
  }
  return unlink(path);
}
static int test_close(int fd) {
  if (!hook.tmp && !hook.close_error)
    return close(fd);
  ++hook.closed;
  int r = close(fd);
  if (hook.close_error) {
    errno = hook.close_error;
    return -1;
  }
  return r;
}
static int test_fsync(int fd) {
  if (hook.fsync_error) {
    errno = hook.fsync_error;
    return -1;
  }
  return fsync(fd);
}

static void test_parse(void) {
  size_t n = (size_t)sysconf(_SC_PAGESIZE);
  char *p =
      mmap(NULL, n * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  FIO_ASSERT(p != MAP_FAILED && !mprotect(p + n, n, PROT_NONE), "mmap");
  fio_filename_s (*volatile parse)(const char *, size_t) = fio_filename_parse2;
  fio_filename_s r = parse(p + n, 0);
  FIO_ASSERT(!r.folder.buf && !r.basename.buf && !r.ext.buf && !r.folder.len &&
                 !r.basename.len && !r.ext.len,
             "zero-length parse must not dereference its pointer");
  char *s = p + n - 7;
  memcpy(s, "a/b.ext", 7);
  r = parse(s, 7);
  FIO_ASSERT(r.folder.buf == s && r.folder.len == 2 &&
                 r.basename.buf == s + 2 && r.basename.len == 1 &&
                 r.ext.buf == s + 4 && r.ext.len == 3,
             "exact non-NUL extent");
  s[3] = 0;
  r = parse(s, 7);
  FIO_ASSERT(r.basename.len == 1 && !r.ext.len, "embedded NUL");
  FIO_ASSERT(!munmap(p, n * 2), "munmap");
}

static void test_sentinel(const char *path) {
  int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0600);
  FIO_ASSERT(fd != -1 && write(fd, "safe", 4) == 4 && !close(fd), "sentinel");
}
static void test_remove(int swap_only) {
  FIO_ASSERT(!mkdir("outside", 0700), "mkdir outside");
  test_sentinel("outside/sentinel");
  const char *paths[] = {"link", "link/", "link///"};
  for (size_t i = 0; !swap_only && i < 3; ++i) {
    for (size_t folder = 0; folder < 2; ++folder) {
      FIO_ASSERT(!symlink("outside", "link"), "symlink");
      FIO_ASSERT(!fio_filename_remove(.path = paths[i],
                                      .recursive = !folder,
                                      .folder = folder),
                 "remove symlink");
      struct stat st;
      FIO_ASSERT(!stat("outside/sentinel", &st) && lstat("link", &st) == -1 &&
                     errno == ENOENT,
                 "symlink removal must preserve target");
    }
  }
  /* Swap the root, then a child, exactly at the directory-open boundary. */
  for (int change = 0; change < 4; ++change) {
    int child = change & 1;
    FIO_ASSERT(!mkdir("tree", 0700), "mkdir tree");
    if (child)
      FIO_ASSERT(!mkdir("tree/child", 0700), "mkdir child");
    memset(&hook, 0, sizeof(hook));
    hook.swap = 1 + (change >> 1);
    snprintf(hook.swap_path,
             sizeof(hook.swap_path),
             "%s",
             child ? "child" : "tree");
    snprintf(hook.swap_held,
             sizeof(hook.swap_held),
             "%s",
             child ? "tree/held" : "held");
    snprintf(hook.swap_target,
             sizeof(hook.swap_target),
             "%s",
             child ? "../outside" : "outside");
    if (child)
      snprintf(hook.swap_path, sizeof(hook.swap_path), "tree/child");
    int r = fio_filename_remove(.path = "tree", .recursive = 1);
    struct stat st;
    FIO_ASSERT(!hook.swap && r == -1 && !stat("outside/sentinel", &st),
               "directory swap must fail without traversing the symlink");
    memset(&hook, 0, sizeof(hook));
    if (child)
      FIO_ASSERT(!rmdir("tree/held"), "cleanup renamed child");
    FIO_ASSERT(!fio_filename_remove(.path = "tree", .recursive = 1),
               "cleanup tree");
    if (!child)
      FIO_ASSERT(!rmdir("held"), "cleanup held");
  }
  FIO_ASSERT(!unlink("outside/sentinel") && !rmdir("outside"),
             "cleanup outside");
  FIO_ASSERT(!fio_filename_make_path(.path = "normal/a/b") &&
                 !fio_filename_overwrite("normal/a/b/file", "data", 4) &&
                 !fio_filename_remove(.path = "normal///", .recursive = 1),
             "normal recursive removal");
}

static void test_tmp(void) {
  /* Oversized environment values fall back to '.'; 463 fits the template. */
  const size_t lens[] = {463, 464, 511, 512};
  char env[513];
  for (int outcome = 1; outcome <= 2; ++outcome) {
    for (size_t i = 0; i < 4; ++i) {
      memset(&hook, 0, sizeof(hook));
      hook.tmp = outcome;
      memset(env, 'x', lens[i]);
      env[lens[i]] = 0;
      FIO_ASSERT(!setenv("TMPDIR", env, 1), "setenv");
      int fd = fio_filename_tmp();
      if (lens[i] == 463 && outcome == 1) {
        FIO_ASSERT(fd == -1, "unavailable long directory");
      } else {
        FIO_ASSERT(fd != -1,
                   "overlong TMPDIR fallback (%zu, %d)",
                   lens[i],
                   outcome);
        FIO_ASSERT(!close(fd), "close tmp");
      }
      FIO_ASSERT(hook.path_len < 512, "bounded temporary name");
#ifdef O_TMPFILE
      FIO_ASSERT(hook.tmp_calls == 1 && hook.named_calls == (outcome == 1),
                 "both O_TMPFILE outcomes");
#endif
    }
  }
  unsetenv("TMPDIR");
  memset(&hook, 0, sizeof(hook));
  hook.tmp = 1;
  hook.unlink_error = EACCES;
  hook.close_error = EINTR; /* close performed once, errno must stay EACCES */
  FIO_ASSERT(!setenv("TMPDIR", ".", 1), "setenv");
  int fd = fio_filename_tmp();
  int e = errno;
  if (fd != -1)
    close(fd); /* don't leak on a pre-fix run */
  FIO_ASSERT(fd == -1 && e == EACCES && hook.closed == 1,
             "unlink failure must fail creation, preserve errno, close once");
  FIO_ASSERT(!unlink(hook.residue), "remove injected residue");
  memset(&hook, 0, sizeof(hook));
}
static void test_save_cleanup(void) {
  FIO_ASSERT(!fio_filename_overwrite("save", "old", 3), "save setup");
  hook.unlink_error = EACCES;
  hook.fsync_error = EIO;
  FIO_ASSERT(fio_filename_overwrite("save", "new", 3) == -1 && errno == EIO,
             "keep primary save error even when cleanup fails");
  char buf[3];
  int fd = open("save", O_RDONLY);
  FIO_ASSERT(fd != -1 && read(fd, buf, 3) == 3 && !memcmp(buf, "old", 3) &&
                 !close(fd),
             "save target must be untouched");
  FIO_ASSERT(!unlink(hook.residue), "cleanup injected residue");
  memset(&hook, 0, sizeof(hook));
  hook.close_error = EINTR;
  FIO_ASSERT(fio_filename_overwrite("save", "new", 3) == -1 && errno == EINTR &&
                 hook.closed == 1,
             "close error: fail, close once");
  memset(&hook, 0, sizeof(hook));
  fd = open("save", O_RDONLY);
  FIO_ASSERT(fd != -1 && read(fd, buf, 3) == 3 && !memcmp(buf, "old", 3) &&
                 !close(fd),
             "close failure preserves target");
  /* Replacement metadata is intentionally creation-time metadata. */
  mode_t old = umask(022);
  FIO_ASSERT(!chmod("save", 0600) && !fio_filename_overwrite("save", "new", 3),
             "metadata replace");
  umask(old);
  struct stat st;
  FIO_ASSERT(!stat("save", &st) && (st.st_mode & 0777) == 0644,
             "documented new-file permissions");
  FIO_ASSERT(!unlink("save"), "cleanup save");
}
#endif

int main(int argc, char **argv) {
#if !FIO_OS_WIN
  alarm(20);
  FIO_ASSERT(!mkdir("ai-tmp", 0700) || errno == EEXIST, "mkdir ai-tmp");
  char tree[] = "ai-tmp/files-security-XXXXXX";
  FIO_ASSERT(mkdtemp(tree), "mkdtemp");
  int cwd = open(".", O_RDONLY);
  FIO_ASSERT(cwd != -1 && !chdir(tree), "chdir sandbox");
  const char *old = getenv("TMPDIR");
  char *saved = old ? strdup(old) : NULL;
  FIO_ASSERT(!old || saved, "strdup");
  if (argc < 2 || !strcmp(argv[1], "parse"))
    test_parse();
  if (argc < 2 || !strcmp(argv[1], "remove") || !strcmp(argv[1], "swap"))
    test_remove(argc > 1 && !strcmp(argv[1], "swap"));
  if (argc < 2 || !strcmp(argv[1], "tmp"))
    test_tmp();
  if (argc < 2 || !strcmp(argv[1], "cleanup"))
    test_save_cleanup();
  if (saved) {
    setenv("TMPDIR", saved, 1);
    free(saved);
  } else
    unsetenv("TMPDIR");
  FIO_ASSERT(!fchdir(cwd) && !close(cwd) && !rmdir(tree), "cleanup sandbox");
#else
  (void)argc;
  (void)argv;
  char tree[160], nested[192], file[224], trailing[192];
  snprintf(tree, sizeof(tree), "files-security-%lu", (unsigned long)GetCurrentProcessId());
  snprintf(nested, sizeof(nested), "%s/nested/child", tree);
  snprintf(file, sizeof(file), "%s/data", nested);
  snprintf(trailing, sizeof(trailing), "%s///", tree);
  FIO_ASSERT(!fio_filename_make_path(.path = nested) &&
                 !fio_filename_overwrite(file, "safe", 4) &&
                 !fio_filename_remove(.path = trailing, .recursive = 1),
             "Windows directory-handle traversal");
  FIO_ASSERT(GetFileAttributesA(tree) == INVALID_FILE_ATTRIBUTES,
             "Windows removal must remove whole tree");
#endif
  return 0;
}
