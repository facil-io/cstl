/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_FILES              /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                   Common File Operations (POSIX style)



Copyright and License: see header file (000 copyright.h) or top of file
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

/** Returns 1 if `path` does folds backwards (OS separator dependent). */
SFUNC int fio_filename_is_unsafe(const char *path);

/** Returns 1 if `path` does folds backwards (has "/../" or "//"). */
SFUNC int fio_filename_is_unsafe_url(const char *path);

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
FIO_IFUNC size_t fio_filename_type(const char *filename);

/**
 * Returns the file type (or 0 on both error).
 *
 * See: https://www.man7.org/linux/man-pages/man7/inode.7.html
 */
FIO_IFUNC size_t fio_fd_type(int fd);

/** Tests if `filename` references a folder. Returns -1 on error. */
#define fio_filename_is_folder(filename)                                       \
  (fio_filename_type((filename)) == S_IFDIR)

/**
 * Writes data to a file handle, returning the number of bytes written.
 *
 * Returns -1 on error.
 *
 * Since some systems have a limit on the number of bytes that can be written at
 * a time, this function fragments the system calls into smaller `write` blocks,
 * allowing large data to be written.
 *
 * If the file descriptor is non-blocking, test errno for EAGAIN / EWOULDBLOCK.
 */
FIO_IFUNC ssize_t fio_fd_write(int fd, const void *buf, size_t len);

/**
 * Reads up to `len` bytes from `fd`, returning the number of bytes read.
 *
 * Returns 0 if no bytes were read or on error.
 *
 * Since some systems have a limit on the number of bytes that can be read at
 * a time, this function fragments the system calls into smaller `read` blocks,
 * allowing large data to be read.
 *
 * If the file descriptor is non-blocking, test errno for EAGAIN / EWOULDBLOCK.
 */
FIO_IFUNC size_t fio_fd_read(int fd, void *buf, size_t len, off_t start_at);

/** A result type for the filename parsing helper. */
typedef struct {
  fio_buf_info_s folder;
  fio_buf_info_s basename;
  fio_buf_info_s ext;
} fio_filename_s;

/** Parses a file name to folder, base name and extension (zero-copy). */
SFUNC fio_filename_s fio_filename_parse(const char *filename);

/** Parses a file name to folder, base name and extension (zero-copy). */
SFUNC fio_filename_s fio_filename_parse2(const char *filename, size_t len);
/**
 * Returns offset for the next `token` in `fd`, or -1 if reached  EOF.
 *
 * This will use `FIO_FD_FIND_BLOCK` bytes on the stack to read the file in a
 * loop.
 *
 * Pros: limits memory use and (re)allocations, easier overflow protection.
 *
 * Cons: may be slower, as data will most likely be copied again from the file.
 */
SFUNC size_t fio_fd_find_next(int fd, char token, size_t start_at);
/** End of file value for `fio_fd_find_next` */
#define FIO_FD_FIND_EOF ((size_t)-1)
#ifndef FIO_FD_FIND_BLOCK
/** Size on the stack used by `fio_fd_find_next` for each read cycle. */
#define FIO_FD_FIND_BLOCK 4096
#endif

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
  if (fd == -1 || !buf_ || !len)
    return -1;
  ssize_t total = 0;
  const char *buf = (const char *)buf_;
  const size_t write_limit = (1ULL << 17);
  while (len > (write_limit - 1)) {
    ssize_t w = write(fd, buf, write_limit);
    if (w > 0) {
      len -= w;
      buf += w;
      total += w;
      continue;
    }
    if (w == -1 && errno == EINTR)
      continue;
    if (total == 0)
      return -1;
    return total;
  }
  while (len) {
    ssize_t w = write(fd, buf, len);
    if (w > 0) {
      len -= w;
      buf += w;
      total += w;
      continue;
    }
    if (w == -1 && errno == EINTR)
      continue;
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

/**
 * Reads up to `len` bytes from `fd`, returning the number of bytes read.
 *
 * Since some systems have a limit on the number of bytes that can be read at
 * a time, this function fragments the system calls into smaller `read` blocks,
 * allowing large data to be read.
 *
 * If the file descriptor is non-blocking, test errno for EAGAIN / EWOULDBLOCK.
 */
FIO_IFUNC size_t fio_fd_read(int fd, void *buf, size_t len, off_t start_at) {
  size_t r = 0;
  if (fd == -1 || !len || !buf) {
    errno = ENOENT;
    return r;
  }
  char *d = (char *)buf;
  for (;;) {
    const size_t to_read = /* use read sizes of up to 27 bits */
        (len & (((size_t)1 << 27) - 1)) | ((!!(len >> 27)) << 27);
    ssize_t act;
#if (_POSIX_C_SOURCE + 1) > 200809L
    if ((act = pread(fd, d + r, to_read, start_at)) > 0) {
      r += act;
      len -= act;
      start_at += act;
      if (!len)
        return r;
      continue;
    }
#else
    if ((off_t)lseek(fd,
                     (start_at + (start_at < 0)),
                     ((start_at < 0) ? SEEK_END : SEEK_SET)) == (off_t)-1) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if ((act = read(fd, d + r, to_read)) > 0) {
      r += act;
      len -= act;
      start_at += act;
      if (!len)
        return r;
      continue;
    }
#endif
    if (act == -1 && errno == EINTR)
      continue;
    return r;
  }
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
}

FIO_IFUNC size_t fio_filename_type(const char *filename) {
  size_t r = 0;
  struct stat stt;
  if (stat(filename, &stt))
    return r;
  return (r = (size_t)((stt.st_mode & S_IFMT)));
}

FIO_IFUNC size_t fio_fd_type(int fd) {
  size_t r = 0;
  struct stat stt;
  if (fd == -1)
    return r;
  if (fstat(fd, &stt))
    return r;
  return (r = (size_t)((stt.st_mode & S_IFMT)));
}

/* *****************************************************************************
File Helper Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

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

  if (filename[0] == '~' &&
      (filename[1] == FIO_FOLDER_SEPARATOR || filename[1] == '/')) {
    char *home = getenv("HOME");
    if (home) {
      size_t filename_len = FIO_STRLEN(filename);
      size_t home_len = FIO_STRLEN(home);
      if ((home_len + filename_len) >= (1 << 16)) {
        /* too long */
        FIO_LOG_ERROR("couldn't open file, as filename is too long %.*s...",
                      (int)16,
                      (filename_len >= 16 ? filename : home));
        return fd;
      }
      if (home[home_len - 1] == FIO_FOLDER_SEPARATOR ||
          home[home_len - 1] == '/')
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

/** Returns 1 if `path` does folds backwards (has "/../" or "//"). */
SFUNC int fio_filename_is_unsafe_url(const char *path) {
  const char sep = '/';
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
  const char sep = FIO_FOLDER_SEPARATOR;
  const char *tmp = NULL;

  if (!tmp)
    tmp = getenv("TMPDIR");
  if (!tmp)
    tmp = getenv("TMP");
  if (!tmp)
    tmp = getenv("TEMP");
#if defined(P_tmpdir)
  if (!tmp && sizeof(P_tmpdir) < 464 && sizeof(P_tmpdir) > 0) {
    tmp = P_tmpdir;
  }
#endif
  if (tmp && (len = FIO_STRLEN(tmp)) && len < 464) {
    FIO_MEMCPY(name_template, tmp, len);
    len -= (tmp[len - 1] == sep || tmp[len - 1] == '/');
  } else {
    /* use current folder */
    name_template[len++] = '.';
  }
#ifdef O_TMPFILE
  name_template[len] = 0;
  fd = open(name_template, O_TMPFILE | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd != -1)
    return fd;
#endif
  name_template[len++] = sep;
  FIO_MEMCPY(name_template + len, "facil_io_tmp_", 13);
  len += 13;
  len += fio_ltoa(name_template + len, fio_rand64(), 32);
  do {
    fio_ltoa(name_template + len, fio_rand64(), 32);
    fd = open(name_template, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
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
  r.basename.buf = (char *)filename;
  for (;;) {
    switch (*pos) {
    case 0:
      if (r.ext.buf) {
        r.ext.len = pos - r.ext.buf;
        if (!r.basename.len) {
          r.basename = FIO_BUF_INFO2(--r.ext.buf, ++r.ext.len);
          r.ext.buf = NULL;
          r.ext.len = 0;
        }
      } else {
        r.basename.len = (size_t)(pos - r.basename.buf);
      }
      if (!r.folder.len)
        r.folder.buf = NULL;
      if (!r.basename.len)
        r.basename.buf = NULL;
      if (!r.ext.len)
        r.ext.buf = NULL;
      return r;
#ifdef FIO_OS_WIN
    case '/': /* pass through (on windows test both variants) */
#endif
    case FIO_FOLDER_SEPARATOR:
      r.folder.buf = (char *)filename;
      r.folder.len = (size_t)(pos - filename) + 1;
      r.basename.buf = (char *)pos + 1;
      r.ext.buf = NULL;
      r.basename.len = 0;
      break;
    case '.':
      if (!r.ext.buf) {
        r.ext.buf = (char *)pos + 1;
        r.basename.len = (char *)pos - r.basename.buf;
      }
      break;
    }
    ++pos;
  }
}

/** Parses a file name to folder, base name and extension (zero-copy). */
SFUNC fio_filename_s fio_filename_parse2(const char *filename, size_t len) {
  fio_filename_s r = {{0}};
  if (!filename || !filename[0])
    return r;
  const char *pos = filename;
  const char *end = filename + len;
  r.basename.buf = (char *)filename;
  for (;;) {
    if (pos == end)
      goto done;
    switch (*pos) {
    case 0:
    done:
      if (r.ext.buf) {
        r.ext.len = pos - r.ext.buf;
        if (!r.basename.len) {
          r.basename = FIO_BUF_INFO2(--r.ext.buf, ++r.ext.len);
          r.ext.buf = NULL;
          r.ext.len = 0;
        }
      } else {
        r.basename.len = (size_t)(pos - r.basename.buf);
      }
      if (!r.folder.len)
        r.folder.buf = NULL;
      if (!r.basename.len)
        r.basename.buf = NULL;
      if (!r.ext.len)
        r.ext.buf = NULL;
      return r;
#ifdef FIO_OS_WIN
    case '/': /* pass through (on windows test both variants) */
#endif
    case FIO_FOLDER_SEPARATOR:
      r.folder.buf = (char *)filename;
      r.folder.len = (size_t)(pos - filename) + 1;
      r.basename.buf = (char *)pos + 1;
      r.ext.buf = NULL;
      r.basename.len = 0;
      break;
    case '.':
      if (!r.ext.buf) {
        r.ext.buf = (char *)pos + 1;
        r.basename.len = (char *)pos - r.basename.buf;
      }
      break;
    }
    ++pos;
  }
}
/** Returns index for next `token` in `fd`, or -1 at EOF. */
SFUNC size_t fio_fd_find_next(int fd, char token, size_t start_at) {
  size_t r = FIO_FD_FIND_EOF;
  if (fd == -1 || start_at == FIO_FD_FIND_EOF)
    return r;
  char buf[FIO_FD_FIND_BLOCK];
  for (;;) {
    size_t l = fio_fd_read(fd, buf, (size_t)FIO_FD_FIND_BLOCK, (off_t)start_at);
    if (!l)
      return r;
    char *pos = (char *)FIO_MEMCHR(buf, token, l);
    if (!pos) {
      start_at += l;
      continue;
    }
    r = start_at + (size_t)(pos - buf);
    return r;
  }
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_FILES */
#undef FIO_FILES
