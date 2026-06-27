# File Helpers

```c
#define FIO_FILES
#include "fio-stl.h"
```

POSIX-style file helpers: open, read, write, size, type, path safety, filename parsing, and temporary files. Nothing fancy — just the usual paperwork. Implemented in [`./004 files.h`](./004%20files.h).

### Configuration Macros

#### `FIO_FD_FIND_BLOCK`

```c
#ifndef FIO_FD_FIND_BLOCK
#define FIO_FD_FIND_BLOCK 4096
#endif
```

Stack buffer size used by `fio_fd_find_next` for each read cycle. Override before inclusion if you want larger or smaller scans.

### Types

#### `fio_filename_s`

```c
typedef struct {
  fio_buf_info_s folder;
  fio_buf_info_s basename;
  fio_buf_info_s ext;
} fio_filename_s;
```

Zero-copy result from `fio_filename_parse` / `fio_filename_parse2`. All members point into the original path string.

**Members:**
- `folder` - directory path including trailing separator, or `{NULL,0}` if none
- `basename` - file name without extension, or `{NULL,0}` if empty
- `ext` - extension without leading `.`, or `{NULL,0}` if none

### Macros

#### `FIO_FOLDER_SEPARATOR`

```c
#if FIO_OS_WIN
#define FIO_FOLDER_SEPARATOR '\\'
#else
#define FIO_FOLDER_SEPARATOR '/'
#endif
```

Native path separator. On Windows the parser also accepts `/`.

#### `fio_file_dup`

```c
#if FIO_OS_WIN
#define fio_file_dup(fd) _dup(fd)
#else
#define fio_file_dup(fd) dup(fd)
#endif
```

Duplicates a file descriptor.

#### `fio_filename_is_folder`

```c
#define fio_filename_is_folder(filename) \
  (fio_filename_type((filename)) == S_IFDIR)
```

Returns non-zero if `filename` is a directory, `0` otherwise (including errors).

#### `FIO_FD_FIND_EOF`

```c
#define FIO_FD_FIND_EOF ((size_t)-1)
```

Sentinel returned by `fio_fd_find_next` when the token is not found before EOF.

### Opening and Creating Files

#### `fio_filename_open`

```c
SFUNC int fio_filename_open(const char *filename, int flags);
```

Opens `filename` with POSIX `open` semantics. If `filename` starts with `"~/"` (or `"~\\"` on Windows), it is resolved relative to `$HOME`.

**Parameters:**
- `filename` - path to open
- `flags` - `O_RDONLY`, `O_RDWR | O_CREAT | O_TRUNC`, etc.

**Returns:** file descriptor on success, `-1` on error.

#### `fio_filename_tmp`

```c
SFUNC int fio_filename_tmp(void);
```

Creates a temporary file and returns its descriptor. Tries `TMPDIR`, `TMP`, `TEMP`, `P_tmpdir`, then the current directory. On Linux it attempts `O_TMPFILE` first; otherwise it builds a name prefixed with `facil_io_tmp_` and opens with `O_CREAT | O_EXCL`.

**Returns:** file descriptor on success, `-1` on error.

#### `fio_filename_overwrite`

```c
FIO_IFUNC int fio_filename_overwrite(const char *filename,
                                     const void *buf,
                                     size_t len);
```

Opens `filename` with `O_RDWR | O_CREAT | O_TRUNC`, writes `len` bytes from `buf`, and closes the file.

**Returns:** `0` on success, `-1` on error. On error the file state is undefined.

### Reading and Writing

#### `fio_fd_write`

```c
FIO_IFUNC ssize_t fio_fd_write(int fd, const void *buf, size_t len);
```

Writes `len` bytes to `fd`, fragmenting the syscall into `write` blocks of up to `1 << 17` bytes. Retries on `EINTR`.

**Returns:** number of bytes written, or `-1` on error. If `fd == -1`, `buf == NULL`, or `len == 0`, returns `-1`.

**Note:** for non-blocking descriptors, check `errno` for `EAGAIN` / `EWOULDBLOCK`.

#### `fio_fd_read`

```c
FIO_IFUNC size_t fio_fd_read(int fd, void *buf, size_t len, off_t start_at);
```

Reads up to `len` bytes from `fd` starting at `start_at`. On POSIX systems with `pread`, the file offset is preserved; otherwise it seeks. Reads are fragmented to `1 << 27` byte chunks. Retries on `EINTR`.

**Parameters:**
- `fd` - file descriptor
- `buf` - destination buffer
- `len` - maximum bytes to read
- `start_at` - offset; negative values seek from end

**Returns:** bytes read, or `0` on error / EOF. Sets `errno = ENOENT` if `fd == -1`, `buf == NULL`, or `len == 0`.

### File Information

#### `fio_filename_size`

```c
FIO_IFUNC size_t fio_filename_size(const char *filename);
```

Returns file size in bytes, or `0` on error or empty file.

#### `fio_fd_size`

```c
FIO_IFUNC size_t fio_fd_size(int fd);
```

Returns file size for an open descriptor, or `0` on error or empty file.

#### `fio_filename_type`

```c
FIO_IFUNC size_t fio_filename_type(const char *filename);
```

Returns the file type bits (`st_mode & S_IFMT`), e.g. `S_IFREG`, `S_IFDIR`, or `0` on error.

#### `fio_fd_type`

```c
FIO_IFUNC size_t fio_fd_type(int fd);
```

Same as `fio_filename_type`, but for an open descriptor.

#### `fio_filename_stat`

```c
FIO_IFUNC int fio_filename_stat(const char *filename, struct stat *stat_buf);
```

Populates `stat_buf` via `stat`. Returns `0` on success, `-1` on error (including `filename == NULL` or `stat_buf == NULL`).

### Path Safety

#### `fio_filename_is_unsafe`

```c
SFUNC int fio_filename_is_unsafe(const char *path);
```

Returns `1` if `path` possibly folds backwards or contains double separators:
- leading `../`
- `/../` or trailing `/..`
- `//`

Uses the OS separator (`\` on Windows, `/` elsewhere). Returns `0` for `NULL` paths.

#### `fio_filename_is_unsafe_url`

```c
SFUNC int fio_filename_is_unsafe_url(const char *path);
```

Same check as `fio_filename_is_unsafe`, but always uses `/` as the separator. Use this for URL paths.

### Filename Parsing

#### `fio_filename_parse`

```c
SFUNC fio_filename_s fio_filename_parse(const char *filename);
```

Splits a NUL-terminated path into folder, basename, and extension without copying.

**Returns:** `fio_filename_s` with pointers into `filename`.

#### `fio_filename_parse2`

```c
SFUNC fio_filename_s fio_filename_parse2(const char *filename, size_t len);
```

Same as `fio_filename_parse`, but reads at most `len` bytes. Use when the input is not NUL-terminated.

### File Search

#### `fio_fd_find_next`

```c
SFUNC size_t fio_fd_find_next(int fd, char token, size_t start_at);
```

Scans `fd` for the next occurrence of `token` starting at `start_at`. Reads in `FIO_FD_FIND_BLOCK` byte chunks.

**Returns:** byte offset of the match, or `FIO_FD_FIND_EOF` if not found.

### Example

```c
#define FIO_FILES
#define FIO_LOG
#include "fio-stl.h"

int main(void) {
  fio_filename_s parts = fio_filename_parse("/var/log/app.log");
  printf("folder=%.*s base=%.*s ext=%.*s\n",
         (int)parts.folder.len, parts.folder.buf,
         (int)parts.basename.len, parts.basename.buf,
         (int)parts.ext.len, parts.ext.buf);

  const char *unsafe = "/var/www/../etc/passwd";
  printf("unsafe? %s\n", fio_filename_is_unsafe(unsafe) ? "yes" : "no");

  const char *msg = "hello, files";
  if (fio_filename_overwrite("/tmp/fio_test.txt", msg, strlen(msg)) == 0) {
    printf("wrote %zu bytes\n", strlen(msg));
  }

  size_t sz = fio_filename_size("/tmp/fio_test.txt");
  printf("size = %zu\n", sz);

  int tmp = fio_filename_tmp();
  if (tmp != -1) {
    fio_fd_write(tmp, "temp", 4);
    close(tmp);
  }
  return 0;
}
```

------------------------------------------------------------
