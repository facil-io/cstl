## File Utility Helpers

```c
#define FIO_FILES
#include "fio-stl.h"
```

By defining the macro `FIO_FILES`, the following file helper functions, macros, and types are defined for common file operations using POSIX-style APIs.

### Types

#### `fio_filename_s`

```c
typedef struct {
  fio_buf_info_s folder;   /* folder name */
  fio_buf_info_s basename; /* base file name */
  fio_buf_info_s ext;      /* extension (without '.') */
} fio_filename_s;
```

A result type for the filename parsing helper functions.

**Members:**
- `folder` - The folder/directory path component (includes trailing separator)
- `basename` - The base file name (without extension)
- `ext` - The file extension (without the leading `.`)

**Note**: all members are `fio_buf_info_s` types pointing into the original string (zero-copy). Empty components will have `.buf` set to `NULL` and `.len` set to `0`.

### Macros

#### `FIO_FOLDER_SEPARATOR`

```c
#if FIO_OS_WIN
#define FIO_FOLDER_SEPARATOR '\\'
#else
#define FIO_FOLDER_SEPARATOR '/'
#endif
```

Selects the folder separation character according to the detected OS.

**Note**: on Windows, both separators (`\` and `/`) will be tested for when parsing paths.

#### `fio_file_dup`

```c
#if FIO_OS_WIN
#define fio_file_dup(fd) _dup(fd)
#else
#define fio_file_dup(fd) dup(fd)
#endif
```

Duplicates the file handle (int).

#### `fio_filename_is_folder`

```c
#define fio_filename_is_folder(filename) (fio_filename_type((filename)) == S_IFDIR)
```

Tests if `filename` references a folder/directory.

Returns non-zero if the path is a directory, `0` if it is not a directory, or `0` on error (when `fio_filename_type` returns `0`).

#### `FIO_FD_FIND_EOF`

```c
#define FIO_FD_FIND_EOF ((size_t)-1)
```

End of file value returned by `fio_fd_find_next` when the token is not found before EOF.

#### `FIO_FD_FIND_BLOCK`

```c
#ifndef FIO_FD_FIND_BLOCK
#define FIO_FD_FIND_BLOCK 4096
#endif
```

Size on the stack used by `fio_fd_find_next` for each read cycle. Can be overridden before including the header.

### File Opening and Creation

#### `fio_filename_open`

```c
int fio_filename_open(const char *filename, int flags);
```

Opens `filename`, returning the same values as `open` on POSIX systems.

If `filename` starts with `"~/"` then it will be relative to the user's home folder (on Windows, also tests for `"~\"`).

**Parameters:**
- `filename` - path to the file to open
- `flags` - file open flags (e.g., `O_RDONLY`, `O_RDWR | O_CREAT | O_TRUNC`)

**Returns:** a file descriptor on success, or `-1` on error.

#### `fio_filename_tmp`

```c
int fio_filename_tmp(void);
```

Creates a temporary file, returning its file descriptor.

The function attempts to use the system's temporary directory (checking `TMPDIR`, `TMP`, `TEMP` environment variables, and `P_tmpdir` if defined). If no temporary directory is found, the current directory is used.

**Returns:** a file descriptor on success, or `-1` on error.

#### `fio_filename_overwrite`

```c
int fio_filename_overwrite(const char *filename, const void *buf, size_t len);
```

Overwrites `filename` with the data in the buffer.

If `filename` starts with `"~/"` then it will be relative to the user's home folder (on Windows, also tests for `"~\"`).

**Parameters:**
- `filename` - path to the file to overwrite
- `buf` - pointer to the data to write
- `len` - number of bytes to write

**Returns:** `0` on success, or `-1` on error. On error, the state of the file is undefined (may not exist, nothing written, or partially written).

### File Reading and Writing

#### `fio_fd_write`

```c
ssize_t fio_fd_write(int fd, const void *buf, size_t len);
```

Writes data to a file handle, returning the number of bytes written.

Since some systems have a limit on the number of bytes that can be written at a single time, this function fragments the system calls into smaller `write` blocks, allowing large data to be written.

**Parameters:**
- `fd` - file descriptor to write to
- `buf` - pointer to the data to write
- `len` - number of bytes to write

**Returns:** the number of bytes written, or `-1` on error.

**Note**: if the file descriptor is non-blocking, test `errno` for `EAGAIN` / `EWOULDBLOCK`.

#### `fio_fd_read`

```c
size_t fio_fd_read(int fd, void *buf, size_t len, off_t start_at);
```

Reads up to `len` bytes from `fd` starting at `start_at` offset.

Since some systems have a limit on the number of bytes that can be read at a time, this function fragments the system calls into smaller `read` blocks, allowing larger data blocks to be read.

**Parameters:**
- `fd` - file descriptor to read from
- `buf` - buffer to read data into
- `len` - maximum number of bytes to read
- `start_at` - offset in the file to start reading from (negative values read from end)

**Returns:** the number of bytes read, or `0` if no bytes were read or on error.

**Note**: if the file descriptor is non-blocking, test `errno` for `EAGAIN` / `EWOULDBLOCK`.

**Note**: may (or may not) change the file's pointer (reading/writing position), depending on the OS and POSIX version.

### File Information

#### `fio_filename_size`

```c
size_t fio_filename_size(const char *filename);
```

Returns the file size for the given filename.

**Parameters:**
- `filename` - path to the file

**Returns:** the file size in bytes, or `0` on both error and empty file.

#### `fio_fd_size`

```c
size_t fio_fd_size(int fd);
```

Returns the file size for the given file descriptor.

**Parameters:**
- `fd` - file descriptor

**Returns:** the file size in bytes, or `0` on both error and empty file.

#### `fio_filename_type`

```c
size_t fio_filename_type(const char *filename);
```

Returns the file type for the given filename.

**Parameters:**
- `filename` - path to the file

**Returns:** the file type as a bitmask (e.g., `S_IFREG` for regular file, `S_IFDIR` for directory), or `0` on error.

See: https://www.man7.org/linux/man-pages/man7/inode.7.html for file type constants.

#### `fio_fd_type`

```c
size_t fio_fd_type(int fd);
```

Returns the file type for the given file descriptor.

**Parameters:**
- `fd` - file descriptor

**Returns:** the file type as a bitmask (e.g., `S_IFREG` for regular file, `S_IFDIR` for directory), or `0` on error.

See: https://www.man7.org/linux/man-pages/man7/inode.7.html for file type constants.

### Path Safety

#### `fio_filename_is_unsafe`

```c
int fio_filename_is_unsafe(const char *path);
```

Returns `1` if `path` possibly folds backwards (path traversal attack detection).

This function checks for patterns that could escape a base directory:
- Leading `../` sequences
- `/../` or `/..` at end (path traversal)
- `//` (double separator, potential path confusion)

The check is OS separator dependent (uses `\` on Windows, `/` on other systems).

**Parameters:**
- `path` - the path string to check

**Returns:** `1` if the path is potentially unsafe, `0` if safe (or if `path` is `NULL`).

#### `fio_filename_is_unsafe_url`

```c
int fio_filename_is_unsafe_url(const char *path);
```

Returns `1` if `path` possibly folds backwards using URL-style separators.

Same as `fio_filename_is_unsafe`, but always uses `/` as the separator regardless of OS. Use this for validating URL paths.

**Parameters:**
- `path` - the path string to check

**Returns:** `1` if the path is potentially unsafe, `0` if safe (or if `path` is `NULL`).

### Filename Parsing

#### `fio_filename_parse`

```c
fio_filename_s fio_filename_parse(const char *filename);
```

Parses a file name into folder, base name, and extension components (zero-copy).

**Parameters:**
- `filename` - NUL-terminated path string to parse

**Returns:** a `fio_filename_s` struct with pointers into the original string.

Example:

```c
fio_filename_s parts = fio_filename_parse("/path/to/file.txt");
// parts.folder   -> "/path/to/"
// parts.basename -> "file"
// parts.ext      -> "txt"
```

#### `fio_filename_parse2`

```c
fio_filename_s fio_filename_parse2(const char *filename, size_t len);
```

Same as `fio_filename_parse`, but limited to `len` characters.

Use this when the `filename` string might not end with a NUL character.

**Parameters:**
- `filename` - path string to parse
- `len` - number of characters to parse

**Returns:** a `fio_filename_s` struct with pointers into the original string.

### File Search

#### `fio_fd_find_next`

```c
size_t fio_fd_find_next(int fd, char token, size_t start_at);
```

Returns the offset for the next occurrence of `token` in `fd`, or `FIO_FD_FIND_EOF` if reached EOF.

This function uses `FIO_FD_FIND_BLOCK` bytes on the stack to read the file in a loop.

**Parameters:**
- `fd` - file descriptor to search in
- `token` - character to search for
- `start_at` - offset to start searching from

**Returns:** the offset of the next `token`, or `FIO_FD_FIND_EOF` (`(size_t)-1`) if not found.

**Pros**: limits memory use and (re)allocations, easier overflow protection.

**Cons**: may be slower, as data will most likely be copied again from the file.

### Example

```c
#define FIO_FILES
#include "fio-stl.h"

int main(void) {
  /* Parse a filename */
  fio_filename_s parts = fio_filename_parse("/home/user/documents/report.pdf");
  printf("Folder:   %.*s\n", (int)parts.folder.len, parts.folder.buf);
  printf("Basename: %.*s\n", (int)parts.basename.len, parts.basename.buf);
  printf("Extension: %.*s\n", (int)parts.ext.len, parts.ext.buf);

  /* Check path safety */
  const char *safe_path = "/var/www/index.html";
  const char *unsafe_path = "/var/www/../etc/passwd";
  printf("Safe path: %s -> %s\n", safe_path,
         fio_filename_is_unsafe(safe_path) ? "UNSAFE" : "safe");
  printf("Unsafe path: %s -> %s\n", unsafe_path,
         fio_filename_is_unsafe(unsafe_path) ? "UNSAFE" : "safe");

  /* Get file information */
  const char *filename = "test.txt";
  size_t size = fio_filename_size(filename);
  if (size) {
    printf("File size: %zu bytes\n", size);
  }

  /* Check if path is a directory */
  if (fio_filename_is_folder("/tmp")) {
    printf("/tmp is a directory\n");
  }

  /* Write and read a file */
  const char *data = "Hello, World!";
  if (fio_filename_overwrite("~/test.txt", data, strlen(data)) == 0) {
    printf("File written successfully\n");
  }

  /* Create a temporary file */
  int tmp_fd = fio_filename_tmp();
  if (tmp_fd != -1) {
    fio_fd_write(tmp_fd, "temporary data", 14);
    close(tmp_fd);
  }

  return 0;
}
```

-------------------------------------------------------------------------------
