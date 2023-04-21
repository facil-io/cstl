## File Utility Helpers

```c
#define FIO_FILES
#include "fio-stl.h"
```

By defining the macro `FIO_FILES` the following file helper functions are defined:

#### `fio_filename_open`

```c
int fio_filename_open(const char *filename, int flags);
```

Opens `filename`, returning the same as values as `open` on POSIX systems.

If `path` starts with a `"~/"` than it will be relative to the user's Home folder (on Windows, testing also for `"~\"`).

#### `fio_filename_is_unsafe`

```c
int fio_filename_is_unsafe(const char *path);
```

Returns 1 if `path` possibly folds backwards (has "/../" or "//").

#### `fio_filename_tmp`

```c
int fio_filename_tmp(void);
```

Creates a temporary file, returning its file descriptor.

Returns -1 on error.

#### `fio_filename_overwrite`

```c
int fio_filename_overwrite(const char *filename, const void *buf, size_t len);
```

Overwrites `filename` with the data in the buffer.

If `path` starts with a `"~/"` than it will be relative to the user's home folder (on Windows, testing also for `"~\"`).

Returns -1 on error or 0 on success. On error, the state of the file is undefined (may be doesn't exit / nothing written / partially written).

#### `fio_fd_write`

```c
ssize_t fio_fd_write(int fd, const void *buf, size_t len);
```

Writes data to a file, returning the number of bytes written.

Returns -1 on error.

Since some systems have a limit on the number of bytes that can be written at a single time, this function fragments the system calls into smaller `write` blocks, allowing large data to be written.

If the file descriptor is non-blocking, test `errno` for `EAGAIN` / `EWOULDBLOCK`.

#### `fio_fd_read`

```c
size_t fio_fd_read(int fd, void *buf, size_t len, off_t start_at);
```


Reads up to `len` bytes from `fd` starting at `start_at` offset.

Returns the number of bytes read.

Since some systems have a limit on the number of bytes that can be read at a time, this function fragments the system calls into smaller `read` blocks, allowing larger data blocks to be read.

If the file descriptor is non-blocking, test `errno` for `EAGAIN` / `EWOULDBLOCK`.

#### `fio_filename_parse`

```c
fio_filename_s fio_filename_parse(const char *filename);
/** A result type for the filename parsing helper. */
typedef struct {
  fio_buf_info_s folder;   /* folder name */
  fio_buf_info_s basename; /* base file name */
  fio_buf_info_s ext;      /* extension (without '.') */
} fio_filename_s;
```

Parses a file name to folder, base name and extension (zero-copy).

#### `fio_filename_parse2`

```c
fio_filename_s fio_filename_parse2(const char *filename, size_t len);
```

Same as [`fio_filename_parse`](#fio_filename_parse), only limited to `len` characters - use in cases where the `filename` string might not end with a `NUL` character.

#### `FIO_FOLDER_SEPARATOR`

```c
#if FIO_OS_WIN
#define FIO_FOLDER_SEPARATOR '\\'
#else
#define FIO_FOLDER_SEPARATOR '/'
#endif
```

Selects the folder separation character according to the detected OS.

**Note**: on windows both separators will be tested for.

#### `fio_fd_find_next`

```c
size_t fio_fd_find_next(int fd, char token, size_t start_at);
/** End of file value for `fio_fd_find_next` */
#define FIO_FD_FIND_EOF ((size_t)-1)
/** Size on the stack used by `fio_fd_find_next` for each read cycle. */
#define FIO_FD_FIND_BLOCK 4096
```

Returns offset for the next `token` in `fd`, or -1 if reached  EOF.

This will use `FIO_FD_FIND_BLOCK` bytes on the stack to read the file in a loop.

**Pros**: limits memory use and (re)allocations, easier overflow protection.

**Cons**: may be slower, as data will most likely be copied again from the file.

-------------------------------------------------------------------------------
