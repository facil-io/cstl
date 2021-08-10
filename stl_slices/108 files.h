/* *****************************************************************************
Copyright: Boaz Segev, 2019-2021
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#define FIO_FILES module            /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "100 mem.h"                /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************




                   Common File Operations (POSIX style)




***************************************************************************** */
#if defined(FIO_FILES) && !defined(H___FIO_FILES___H)
#define H___FIO_FILES___H

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

/* *****************************************************************************
File Helper Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

#if FIO_OS_POSIX
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
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
