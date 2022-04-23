/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_FILES                   /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "006 atol.h"               /* Development inclusion - ignore line */
#include "010 riskyhash.h"          /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                   Common File Operations (POSIX style)




***************************************************************************** */
#if defined(FIO_FILES) && !defined(H___FIO_FILES___H)
#define H___FIO_FILES___H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

/* *****************************************************************************
File Helper API
***************************************************************************** */

/**
 * Opens `filename`, returning the same as values as `open` on POSIX systems.
 *
 * If `path` starts with a `"~/"` than it will be relative to the user's home
 * folder (on Windows, testing for `"~\"`).
 */
SFUNC int fio_filename_open(const char *filename, int flags);

/** Returns 1 if `path` does folds backwards (has "/../" or "//"). */
SFUNC int fio_filename_is_unsafe(const char *path);

/** Creates a temporary file, returning its file descriptor. */
SFUNC int fio_filename_tmp(void);

/**
 * Overwrites `filename` with the data in the buffer.
 *
 * If `path` starts with a `"~/"` than it will be relative to the user's home
 * folder (on Windows, testing for `"~\"`).
 *
 * Returns -1 on error or 0 on success. On error, the state of the file is
 * undefined (may be doesn't exit / nothing written / partially written).
 */
FIO_IFUNC int fio_filename_overwrite(const char *filename,
                                     const void *buf,
                                     size_t len);

/** Returns the file size (or 0 on both error / empty file). */
FIO_IFUNC size_t fio_filename_size(const char *filename);

/** Returns the file size (or 0 on both error / empty file). */
FIO_IFUNC size_t fio_fd_size(int fd);

/**
 * Returns the file type (or 0 on both error).
 *
 * See: https://www.man7.org/linux/man-pages/man7/inode.7.html
 */
FIO_IFUNC mode_t fio_filename_type(const char *filename);

/**
 * Returns the file type (or 0 on both error).
 *
 * See: https://www.man7.org/linux/man-pages/man7/inode.7.html
 */
FIO_IFUNC mode_t fio_fd_type(int fd);

/** Tests if `filename` references a folder. Returns -1 on error. */
#define fio_filename_is_folder(filename)                                       \
  (fio_filename_type((filename)) == S_IFDIR)

/**
 * Writes data to a file, returning the number of bytes written.
 *
 * Returns -1 on error.
 *
 * Since some systems have a limit on the number of bytes that can be written at
 * a single time, this function fragments the system calls into smaller `write`
 * blocks, allowing large data to be written.
 *
 * If the file descriptor is non-blocking, test errno for EAGAIN / EWOULDBLOCK.
 */
FIO_IFUNC ssize_t fio_fd_write(int fd, const void *buf, size_t len);

/** A result type for the filename parsing helper. */
typedef struct {
  fio_buf_info_s folder;
  fio_buf_info_s basename;
  fio_buf_info_s ext;
} fio_filename_s;

/** Parses a file name to folder, base name and extension (zero-copy). */
SFUNC fio_filename_s fio_filename_parse(const char *filename);

#if FIO_OS_WIN
#define FIO_FOLDER_SEPARATOR '\\'
#else
#define FIO_FOLDER_SEPARATOR '/'
#endif

/* *****************************************************************************
File Helper Inline Implementation
***************************************************************************** */

/**
 * Writes data to a file, returning the number of bytes written.
 *
 * Returns -1 on error.
 *
 * Since some systems have a limit on the number of bytes that can be written at
 * a single time, this function fragments the system calls into smaller `write`
 * blocks, allowing large data to be written.
 *
 * If the file descriptor is non-blocking, test errno for EAGAIN / EWOULDBLOCK.
 */
FIO_IFUNC ssize_t fio_fd_write(int fd, const void *buf_, size_t len) {
  ssize_t total = 0;
  const char *buf = (const char *)buf_;
  const size_t write_limit = (1ULL << 17);
  while (len > write_limit) {
    ssize_t w = write(fd, buf, write_limit);
    if (w > 0) {
      len -= w;
      buf += w;
      total += w;
      continue;
    }
    /* if (w == -1 && errno == EINTR) continue; */
    if (total == 0)
      return -1;
    return total;
  }
  while (len) {
    ssize_t w = write(fd, buf, len);
    if (w > 0) {
      len -= w;
      buf += w;
      continue;
    }
    if (total == 0)
      return -1;
    return total;
  }
  return total;
}

/**
 * Overwrites `filename` with the data in the buffer.
 *
 * If `path` starts with a `"~/"` than it will be relative to the user's home
 * folder (on Windows, testing for `"~\"`).
 */
FIO_IFUNC int fio_filename_overwrite(const char *filename,
                                     const void *buf,
                                     size_t len) {
  int fd = fio_filename_open(filename, O_RDWR | O_CREAT | O_TRUNC);
  if (fd == -1)
    return -1;
  ssize_t w = fio_fd_write(fd, buf, len);
  close(fd);
  if ((size_t)w != len)
    return -1;
  return 0;
}
/* *****************************************************************************
File Stat In-lined Helpers
***************************************************************************** */

FIO_IFUNC size_t fio_filename_size(const char *filename) {
  size_t r = 0;
  struct stat stt;
  if (stat(filename, &stt))
    return r;
  return (r = stt.st_size);
}

FIO_IFUNC size_t fio_fd_size(int fd) {
  size_t r = 0;
  struct stat stt;
  if (fd == -1)
    return r;
  if (fstat(fd, &stt))
    return r;
  return (r = stt.st_size);
  // S_ISDIR(stat.st_mode)
}

FIO_IFUNC mode_t fio_filename_type(const char *filename) {
  size_t r = 0;
  struct stat stt;
  if (stat(filename, &stt))
    return r;
  return ((stt.st_mode & S_IFMT));
}

FIO_IFUNC mode_t fio_fd_type(int fd) {
  size_t r = 0;
  struct stat stt;
  if (fd == -1)
    return r;
  if (fstat(fd, &stt))
    return r;
  return ((stt.st_mode & S_IFMT));
}

/* *****************************************************************************
File Helper Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/**
 * Opens `filename`, returning the same as values as `open` on POSIX systems.
 *
 * If `path` starts with a `"~/"` than it will be relative to the user's home
 * folder (on Windows, testing for `"~\"`).
 */
SFUNC int fio_filename_open(const char *filename, int flags) {
  int fd = -1;
  /* POSIX implementations. */
  if (filename == NULL)
    return fd;
  char *path = NULL;
  size_t path_len = 0;
  const char sep = FIO_FOLDER_SEPARATOR;

  if (filename[0] == '~' && filename[1] == sep) {
    char *home = getenv("HOME");
    if (home) {
      size_t filename_len = strlen(filename);
      size_t home_len = strlen(home);
      if ((home_len + filename_len) >= (1 << 16)) {
        /* too long */
        FIO_LOG_ERROR("couldn't open file, as filename is too long %.*s...",
                      (int)16,
                      (filename_len >= 16 ? filename : home));
        return fd;
      }
      if (home[home_len - 1] == sep)
        --home_len;
      path_len = home_len + filename_len - 1;
      path =
          (char *)FIO_MEM_REALLOC_(NULL, 0, sizeof(*path) * (path_len + 1), 0);
      if (!path)
        return fd;
      FIO_MEMCPY(path, home, home_len);
      FIO_MEMCPY(path + home_len, filename + 1, filename_len);
      path[path_len] = 0;
      filename = path;
    }
  }
  fd = open(filename, flags, (S_IWUSR | S_IRUSR));
  if (path) {
    FIO_MEM_FREE_(path, path_len + 1);
  }
  return fd;
}

/** Returns 1 if `path` does folds backwards (has "/../" or "//"). */
SFUNC int fio_filename_is_unsafe(const char *path) {
#if FIO_OS_WIN
  const char sep = '\\';
#else
  const char sep = '/';
#endif
  for (;;) {
    if (!path)
      return 0;
    if (path[0] == sep && path[1] == sep)
      return 1;
    if (path[0] == sep && path[1] == '.' && path[2] == '.' && path[3] == sep)
      return 1;
    ++path;
    path = strchr(path, sep);
  }
}

/** Creates a temporary file, returning its file descriptor. */
SFUNC int fio_filename_tmp(void) {
  // create a temporary file to contain the data.
  int fd;
  char name_template[512];
  size_t len = 0;
#if FIO_OS_WIN
  const char sep = '\\';
  const char *tmp = NULL;
#else
  const char sep = '/';
  const char *tmp = NULL;
#endif

  if (!tmp)
    tmp = getenv("TMPDIR");
  if (!tmp)
    tmp = getenv("TMP");
  if (!tmp)
    tmp = getenv("TEMP");
#if defined(P_tmpdir)
  if (!tmp && sizeof(P_tmpdir) <= 464 && sizeof(P_tmpdir) > 0) {
    tmp = P_tmpdir;
  }
#endif
  if (tmp && (len = strlen(tmp))) {
    FIO_MEMCPY(name_template, tmp, len);
    if (tmp[len - 1] != sep) {
      name_template[len++] = sep;
    }
  } else {
    /* use current folder */
    name_template[len++] = '.';
    name_template[len++] = sep;
  }

  FIO_MEMCPY(name_template + len, "facil_io_tmpfile_", 17);
  len += 17;
  do {
#ifdef O_TMPFILE
    uint64_t r = fio_rand64();
    size_t delta = fio_ltoa(name_template + len, r, 32);
    name_template[delta + len] = 0;
    fd = open(name_template,
              O_CREAT | O_TMPFILE | O_EXCL | O_RDWR,
              (S_IWUSR | S_IRUSR));
#else
    FIO_MEMCPY(name_template + len, "XXXXXXXXXXXX", 12);
    name_template[12 + len] = 0;
    fd = mkstemp(name_template);
#endif
  } while (fd == -1 && errno == EEXIST);
  return fd;
  (void)tmp;
}

/** Parses a file name to folder, base name and extension (zero-copy). */
SFUNC fio_filename_s fio_filename_parse(const char *filename) {
  fio_filename_s r = {{0}};
  if (!filename || !filename[0])
    return r;
  const char *pos = filename;
  for (;;) {
    switch (*pos) {
    case 0:
      if (r.folder.buf) {
        r.basename.buf = r.folder.buf + r.folder.len;
        r.basename.len = (size_t)(pos - (r.folder.buf + r.folder.len));
      } else {
        r.basename.buf = (char *)filename;
        r.basename.len = (size_t)(pos - filename);
      }
      if (pos == r.folder.buf + r.folder.len) {
        r.basename.buf = 0;
        r.basename.len = 0;
        r.ext.buf = 0;
        r.ext.len = 0;
        return r;
      }
      if (r.ext.buf) {
        r.ext.len = pos - r.ext.buf;
        if (FIO_UNLIKELY(filename + r.folder.len == r.ext.buf)) {
          r.basename.buf = r.ext.buf;
          r.basename.len = r.ext.len;
          r.ext.buf = 0;
          r.ext.len = 0;
        } else if (r.ext.len > 1) {
          r.basename.len -= r.ext.len;
          ++r.ext.buf; /* skip the '.' */
          --r.ext.len;
        } else {
          r.ext.buf = 0;
          r.ext.len = 0;
        }
      }
      return r;
    case FIO_FOLDER_SEPARATOR:
      r.folder.buf = (char *)filename;
      r.folder.len = (size_t)(pos - filename) + 1;
      r.ext.buf = NULL;
      break;
    case '.': r.ext.buf = (char *)pos; break;
    }
    ++pos;
  }
}

/* *****************************************************************************
Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, filename)(void) { /* TODO: test module */
  fprintf(stderr, "* Testing file utilities (partial).\n");
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
        "\n\t folder:    (%zu) %.*s"
        "\n\t basename:  (%zu) %.*s"
        "\n\t extension: (%zu) %.*s",
        filename_test[i].str,
        r.folder.len,
        (int)r.folder.len,
        (r.folder.buf ? r.folder.buf : "null"),
        r.basename.len,
        (int)r.basename.len,
        (r.basename.buf ? r.basename.buf : "null"),
        r.ext.len,
        (int)r.ext.len,
        (r.ext.buf ? r.ext.buf : "null"));
  }
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_FILES */
#undef FIO_FILES
