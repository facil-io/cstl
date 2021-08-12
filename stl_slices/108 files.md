## File Utility Helpers

By defining the macro `FIO_FILES` the following file helper functions are defined:

#### `fio_filename_open`

```c
int fio_filename_open(const char *filename, int flags);
```

Opens `filename`, returning the same as values as `open` on POSIX systems.

If `path` starts with a `"~/"` than it will be relative to the user's Home folder (on Windows, testing for `"~\"`).

#### `fio_filename_is_unsafe`

```c
int fio_filename_is_unsafe(const char *path);
```

Returns 1 if `path` does folds backwards (has "/../" or "//").

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

If `path` starts with a `"~/"` than it will be relative to the user's home folder (on Windows, testing for `"~\"`).

Returns -1 on error or 0 on success. On error, the state of the file is undefined (may be doesn't exit / nothing written / partially written).

#### `fio_fd_write`

```c
ssize_t fio_fd_write(int fd, const void *buf, size_t len);
```

Writes data to a file, returning the number of bytes written.

Returns -1 on error.

Since some systems have a limit on the number of bytes that can be written at a single time, this function fragments the system calls into smaller `write` blocks, allowing large data to be written.

If the file descriptor is non-blocking, test `errno` for `EAGAIN` / `EWOULDBLOCK`.

-------------------------------------------------------------------------------

