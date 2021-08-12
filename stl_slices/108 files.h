/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_FILES                   /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "005 riskyhash.h"          /* Development inclusion - ignore line */
#include "006 atol.h"               /* Development inclusion - ignore line */
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
#if FIO_OS_WIN
  const char sep = '\\';
#else
  const char sep = '/';
#endif

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
  fd = open(filename, flags);
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
    fd = open(name_template, O_CREAT | O_TMPFILE | O_EXCL | O_RDWR);
#else
    FIO_MEMCPY(name_template + len, "XXXXXXXXXXXX", 12);
    name_template[12 + len] = 0;
    fd = mkstemp(name_template);
#endif
  } while (fd == -1 && errno == EEXIST);
  return fd;
  (void)tmp;
}

/* *****************************************************************************
Module Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, filename)(void) { /* TODO: test module */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_FILES */
#undef FIO_FILES
