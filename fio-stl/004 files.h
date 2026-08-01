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
#if !FIO_OS_WIN
#include <dirent.h>
#endif

/**
 * Stack buffer capacity used for path manipulation (`"~/"` expansion, folder
 * tree walking / creation). Path operations that exceed this fail with
 * `errno == ENAMETOOLONG`. Guarantees at least ~4KB (`PATH_MAX | 4094`).
 */
#ifndef FIO_FILENAME_PATH_CAPA
#if defined(PATH_MAX) && (PATH_MAX <= 16384)
#define FIO_FILENAME_PATH_CAPA (PATH_MAX | 4094)
#else
#define FIO_FILENAME_PATH_CAPA (4094)
#endif
#endif

/* *****************************************************************************
File Helper API
***************************************************************************** */

/**
 * Opens `filename`, returning the same as values as `open` on POSIX systems.
 *
 * If `path` starts with a `"~/"` than it will be relative to the user's home
 * folder (on Windows, testing for `"~\"`).
 *
 * Uses a fixed size stack buffer (zero allocations); over-long paths fail
 * with `errno == ENAMETOOLONG` (see `FIO_FILENAME_PATH_CAPA`).
 */
SFUNC int fio_filename_open(const char *filename, int flags);

/** Returns 1 if `path` does folds backwards (OS separator dependent). */
SFUNC int fio_filename_is_unsafe(const char *path);

/** Returns 1 if `path` does folds backwards (has "/../" or "//"). */
SFUNC int fio_filename_is_unsafe_url(const char *path);

/** Creates a temporary file, returning its file descriptor. */
SFUNC int fio_filename_tmp(void);

/** Arguments for `fio_filename_remove`. */
typedef struct {
  /** The path of the file / folder to remove. */
  const char *path;
  /** Set to allow the removal of an (empty) folder. */
  uint8_t folder;
  /** Set to remove a folder and all of its content (implies `folder`). */
  uint8_t recursive;
} fio_filename_remove_args_s;

/**
 * Removes the file / link or folder at `path`.
 *
 * * By default (no flags), removes a file / link (like `unlink`).
 * * With `folder` set, removes an empty folder (like `rmdir`).
 * * With `recursive` set (implies `folder`), removes a folder and all of its
 *   content (like `rm -r`). If `path` isn't a folder, the `recursive` flag
 *   is ignored and `path` is removed as a file / link.
 *
 * Links are removed, never followed into.
 *
 * Uses a fixed size stack buffer (zero allocations); over-long paths fail
 * with `errno == ENAMETOOLONG` (see `FIO_FILENAME_PATH_CAPA`).
 *
 * Returns 0 on success and -1 on error. Recursive removal stops on the first
 * error (some content may remain).
 */
SFUNC int fio_filename_remove(fio_filename_remove_args_s args);
#define fio_filename_remove(...)                                               \
  fio_filename_remove((fio_filename_remove_args_s){__VA_ARGS__})

/** Arguments for `fio_filename_make_path`. */
typedef struct {
  /** The folder path to create (nested folders allowed). */
  const char *path;
  /** The creation mode (POSIX only); zero (0) defaults to 0755. */
  uint32_t mode;
} fio_filename_make_path_args_s;

/**
 * Creates the folder at `path`, including any missing parent folders
 * (similar to `mkdir -p`).
 *
 * An existing folder is NOT an error. A non-folder component along the way IS
 * an error (`errno == ENOTDIR`).
 *
 * Uses a fixed size stack buffer (zero allocations); over-long paths fail
 * with `errno == ENAMETOOLONG` (see `FIO_FILENAME_PATH_CAPA`).
 *
 * Returns 0 on success and -1 on error.
 */
SFUNC int fio_filename_make_path(fio_filename_make_path_args_s args);
#define fio_filename_make_path(...)                                            \
  fio_filename_make_path((fio_filename_make_path_args_s){__VA_ARGS__})

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

/** Populates `stat_buf` with the file's metadata. Returns 0 on success. */
FIO_IFUNC int fio_filename_stat(const char *filename, struct stat *stat_buf);

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
/** Duplicates the file handle (int)*/
#define fio_file_dup(fd) _dup(fd)
#else
#define FIO_FOLDER_SEPARATOR '/'
/** Duplicates the file handle (int)*/
#define fio_file_dup(fd)     dup(fd)
#endif /* FIO_OS_WIN */

/* *****************************************************************************
File Helper Inline Implementation
***************************************************************************** */

/** Tests if `c` is a folder separator (on Windows both `/` and `\`). */
FIO_IFUNC int fio___filename_is_sep(const char c) {
#if FIO_OS_WIN
  return (c == '\\' || c == '/');
#else
  return (c == '/');
#endif
}

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
  int saved_errno = errno; /* preserve the write's errno across close() */
  close(fd);
  if ((size_t)w != len) {
    errno = saved_errno;
    return -1;
  }
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
#if (defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE + 1) > 200809L)
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

FIO_IFUNC int fio_filename_stat(const char *filename, struct stat *stat_buf) {
  if (!filename || !stat_buf)
    return -1;
  return stat(filename, stat_buf);
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
  /* POSIX implementations. */
  if (filename == NULL)
    return -1;
  if (filename[0] != '~' ||
      (filename[1] != FIO_FOLDER_SEPARATOR && filename[1] != '/'))
    return open(filename, flags, (S_IWUSR | S_IRUSR));
  /* expand "~/" relative to the user's home folder */
  char *home = fio_sys_env("HOME");
  if (!home)
    return open(filename, flags, (S_IWUSR | S_IRUSR));
  FIO_STR_INFO_TMP_VAR(path, FIO_FILENAME_PATH_CAPA);
  const size_t filename_len = FIO_STRLEN(filename);
  size_t home_len = FIO_STRLEN(home);
  if (home_len &&
      (home[home_len - 1] == FIO_FOLDER_SEPARATOR || home[home_len - 1] == '/'))
    --home_len;
  if (home_len + filename_len - 1 >= path.capa) {
    /* too long */
    errno = ENAMETOOLONG;
    FIO_LOG_ERROR("couldn't open file, as filename is too long %.*s...",
                  (int)16,
                  (filename_len >= 16 ? filename : home));
    return -1;
  }
  FIO_MEMCPY(path.buf, home, home_len);
  FIO_MEMCPY(path.buf + home_len, filename + 1, filename_len);
  path.buf[home_len + filename_len - 1] = 0;
  return open(path.buf, flags, (S_IWUSR | S_IRUSR));
}

/** Returns 1 if `path` possibly folds backwards (has "/../", "/..", "//"). */
FIO_IFUNC int fio___filename_is_unsafe_sep(const char *path,
                                           const char sep1,
                                           const char sep2) {
  if (!path) /* no file is a safe file, nothing to do */
    return 0;
  /* Check for leading "../" which escapes the base directory */
  if (path[0] == '.' && path[1] == '.' &&
      (path[2] == sep1 || path[2] == sep2 || path[2] == '\0'))
    return 1;
  /* Scan through path looking for problematic patterns */
  while (*path) {
    if (path[0] == sep1 || path[0] == sep2) {
      /* Check for "//" (double separator, potential path confusion) */
      if (path[1] == sep1 || path[1] == sep2)
        return 1;
      /* Check for "/../" or "/.." at end (path traversal) */
      if (path[1] == '.' && path[2] == '.' &&
          (path[3] == sep1 || path[3] == sep2 || path[3] == '\0'))
        return 1;
    }
    ++path;
  }
  return 0;
}

/** Returns 1 if `path` possibly folds backwards (has "/../", "/..", "//").
 *
 * On Windows both `/` and `\` are guarded, since Win32 APIs accept either
 * separator - guarding only one would leave the other flavor open to path
 * traversal. On POSIX `\` is an ordinary filename character and is ignored.
 */
SFUNC int fio_filename_is_unsafe(const char *path) {
  return fio___filename_is_unsafe_sep(path, '/', FIO_FOLDER_SEPARATOR);
}

/** Returns 1 if `path` does folds backwards (has "/../" or "//"). */
SFUNC int fio_filename_is_unsafe_url(const char *path) {
  return fio___filename_is_unsafe_sep(path, '/', FIO_FOLDER_SEPARATOR);
}

/** Creates a temporary file, returning its file descriptor. */
SFUNC int fio_filename_tmp(void) {
  // create a temporary file to contain the data.
  int fd;
  char name_template[512];
  size_t len = 0;
  const char sep = FIO_FOLDER_SEPARATOR;
  const char *tmp = NULL;
  const char *options[] = {"TMPDIR", "TMP", "TEMP", NULL};
  for (size_t i = 0; !tmp && options[i]; ++i) {
    tmp = fio_sys_env(options[i]);
  }
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

/* *****************************************************************************
Filename Removal Implementation
***************************************************************************** */

#if FIO_OS_WIN
/** Removes a file / link (no flags) or an empty folder (`folder` flag). */
FIO_SFUNC int fio___filename_remove_simple(const char *path, int folder) {
  if (folder)
    return RemoveDirectoryA(path) ? 0 : -1;
  return DeleteFileA(path) ? 0 : -1;
}
#else
/** Removes a file / link (no flags) or an empty folder (`folder` flag). */
FIO_SFUNC int fio___filename_remove_simple(const char *path, int folder) {
  if (folder)
    return rmdir(path);
  return unlink(path);
}
#endif

/**
 * Removes all content within the folder at `path` (not the folder itself).
 *
 * `path` must be mutable, `len` its current length and `capa` its capacity.
 */
FIO_SFUNC int fio___filename_remove_content(char *path,
                                            size_t len,
                                            size_t capa) {
  int r = -1;
#if FIO_OS_WIN
  if (len + 3 >= capa) {
    errno = ENAMETOOLONG;
    return -1;
  }
  path[len] = FIO_FOLDER_SEPARATOR;
  path[len + 1] = '*';
  path[len + 2] = 0;
  WIN32_FIND_DATAA fdata;
  HANDLE h = FindFirstFileA(path, &fdata);
  if (h == INVALID_HANDLE_VALUE)
    return -1;
  do {
    if (fdata.cFileName[0] == '.' &&
        (!fdata.cFileName[1] ||
         (fdata.cFileName[1] == '.' && !fdata.cFileName[2])))
      continue;
    const size_t name_len = FIO_STRLEN(fdata.cFileName);
    if (len + 1 + name_len + 1 >= capa) {
      errno = ENAMETOOLONG;
      goto done;
    }
    path[len] = FIO_FOLDER_SEPARATOR;
    FIO_MEMCPY(path + len + 1, fdata.cFileName, name_len + 1);
    if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        !(fdata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
      if (fio___filename_remove_content(path, len + 1 + name_len, capa))
        goto done;
      path[len + 1 + name_len] = 0;
      if (!RemoveDirectoryA(path))
        goto done;
    } else if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                   ? !RemoveDirectoryA(
                         path) /* links are removed, not followed */
                   : !DeleteFileA(path)) {
      goto done;
    }
  } while (FindNextFileA(h, &fdata));
  r = 0;
done:
  FindClose(h);
  path[len] = 0;
  return r;
#else
  DIR *d = opendir(path);
  if (!d)
    return -1;
  struct dirent *ent;
  while ((ent = readdir(d))) {
    if (ent->d_name[0] == '.' &&
        (!ent->d_name[1] || (ent->d_name[1] == '.' && !ent->d_name[2])))
      continue;
    const size_t name_len = FIO_STRLEN(ent->d_name);
    if (len + 1 + name_len + 1 >= capa) {
      errno = ENAMETOOLONG;
      goto done;
    }
    path[len] = FIO_FOLDER_SEPARATOR;
    FIO_MEMCPY(path + len + 1, ent->d_name, name_len + 1);
    struct stat stt;
    if (lstat(path, &stt))
      goto done;
    if (S_ISDIR(stt.st_mode)) {
      if (fio___filename_remove_content(path, len + 1 + name_len, capa))
        goto done;
      path[len + 1 + name_len] = 0;
      if (rmdir(path))
        goto done;
    } else if (unlink(path)) {
      goto done;
    }
  }
  r = 0;
done:
  closedir(d);
  path[len] = 0;
  return r;
#endif
}

void fio_filename_remove___(void); /* IDE Marker */
/** Removes the file / link or folder at `path` (see flags for details). */
SFUNC int fio_filename_remove FIO_NOOP(fio_filename_remove_args_s args) {
  if (!args.path || !args.path[0]) {
    errno = ENOENT;
    return -1;
  }
  if (!args.recursive)
    return fio___filename_remove_simple(args.path, (int)args.folder);
  /* recursive removal (`rm -r` like) */
  FIO_STR_INFO_TMP_VAR(buf, FIO_FILENAME_PATH_CAPA);
  size_t len = FIO_STRLEN(args.path);
#if FIO_OS_WIN
  DWORD attr = GetFileAttributesA(args.path);
  if (attr == INVALID_FILE_ATTRIBUTES)
    return -1;
  if (!(attr & FILE_ATTRIBUTE_DIRECTORY) ||
      (attr & FILE_ATTRIBUTE_REPARSE_POINT))
    /* files and links (reparse points) are removed, never followed into */
    return fio___filename_remove_simple(
        args.path,
        (int)(!!(attr & FILE_ATTRIBUTE_DIRECTORY)));
#else
  struct stat stt;
  if (lstat(args.path, &stt))
    return -1;
  if (!S_ISDIR(stt.st_mode))
    return fio___filename_remove_simple(args.path, 0);
#endif
  /* trim trailing separators, so sub-paths use exactly one separator */
  while (len > 1 && fio___filename_is_sep(args.path[len - 1]))
    --len;
  if (len + 3 >= buf.capa) {
    errno = ENAMETOOLONG;
    return -1;
  }
  FIO_MEMCPY(buf.buf, args.path, len);
  buf.buf[len] = 0;
  if (fio___filename_remove_content(buf.buf, len, buf.capa))
    return -1;
  return fio___filename_remove_simple(args.path, 1);
}

void fio_filename_make_path___(void); /* IDE Marker */
/** Creates a single folder; 0 = created, 1 = already exists, -1 = error. */
FIO_SFUNC int fio___filename_mkdir(const char *path, uint32_t mode) {
#if FIO_OS_WIN
  (void)mode;
  if (CreateDirectoryA(path, NULL))
    return 0;
  return (GetLastError() == ERROR_ALREADY_EXISTS) ? 1 : -1;
#else
  if (!mkdir(path, (mode_t)mode))
    return 0;
  return (errno == EEXIST) ? 1 : -1;
#endif
}

/** Creates the folder at `path`, including missing parents (`mkdir -p`). */
SFUNC int fio_filename_make_path FIO_NOOP(fio_filename_make_path_args_s args) {
  if (!args.path || !args.path[0]) {
    errno = ENOENT;
    return -1;
  }
  size_t len = FIO_STRLEN(args.path);
  while (len > 1 && fio___filename_is_sep(args.path[len - 1]))
    --len;
  FIO_STR_INFO_TMP_VAR(buf, FIO_FILENAME_PATH_CAPA);
  if (len >= buf.capa) {
    errno = ENAMETOOLONG;
    return -1;
  }
  FIO_MEMCPY(buf.buf, args.path, len);
  buf.buf[len] = 0;
  if (!args.mode)
    args.mode = 0755;
  /* skip prefixes that can't be created (root, drive, UNC server/share) */
  size_t start = 0;
#if FIO_OS_WIN
  if (len >= 2 && buf.buf[1] == ':') {
    start = 2; /* drive letter ("C:") */
  } else if (len >= 2 && fio___filename_is_sep(buf.buf[0]) &&
             fio___filename_is_sep(buf.buf[1])) {
    /* UNC path: skip "\\server\share" (and any separator run after each) */
    size_t i = 2;
    for (size_t names = 0; i < len && names < 2; ++i) {
      if (fio___filename_is_sep(buf.buf[i])) {
        while (i + 1 < len && fio___filename_is_sep(buf.buf[i + 1]))
          ++i;
        ++names;
      }
    }
    start = i;
  }
#else
  while (start < len && fio___filename_is_sep(buf.buf[start]))
    ++start; /* skip root separator(s) */
#endif
  if (start >= len)
    start = len - 1; /* nothing beyond the prefix: create / verify whole */
  for (size_t i = start + 1; i <= len; ++i) {
    if (i < len && !fio___filename_is_sep(buf.buf[i]))
      continue;
    if (fio___filename_is_sep(buf.buf[i - 1]))
      continue; /* skip empty component (repeated separators) */
    /* reached a folder boundary: create buf.buf[0..i) */
    const char sep = buf.buf[i];
    buf.buf[i] = 0;
    int r = fio___filename_mkdir(buf.buf, args.mode);
    if (r == -1 || (r == 1 && !fio_filename_is_folder(buf.buf))) {
      if (r == 1)
        errno = ENOTDIR;
      return -1;
    }
    buf.buf[i] = sep;
  }
  return 0;
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
#if FIO_OS_WIN
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
#if FIO_OS_WIN
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
