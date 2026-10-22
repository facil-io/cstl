## Data Store (`fio_data_s`)

```c
#define FIO_DATA
#include FIO_INCLUDE_FILE
```

> **Requires:** `FIO_STR` and `FIO_FILES`. When using `include.h`, these dependencies are resolved automatically. The HTTP and FIOBJ modules include this module.

A seekable data store that keeps data in RAM until it grows past a limit, then moves it to a temporary file. This is the storage behind HTTP request and response bodies, and it's useful anywhere large payloads (such as file uploads) shouldn't stay in memory.

### Settings

```c
#define FIO_DATA_RAM_LIMIT  (1UL << 17) /* default RAM limit, in bytes */
#define FIO_DATA_READ_BLOCK (1UL << 14) /* block size without mmap (spilled) */
/* #define FIO_DATA_NO_MMAP */          /* never memory map temporary files */
```

The effective RAM limit is clamped below the `fio_bstr` size limit (about 2GiB), so RAM data never hits that limit. Spilled data has no such limit.

`FIO_DATA_NO_MMAP` (define before including the module) disables memory mapping, forcing the heap read buffer described below. It's always the case on systems without `sys/mman.h` (i.e., Windows).

### Type and lifetime

```c
typedef struct {
  char *buf;    /* RAM data (fio_bstr); once spilled, the mapping / read buffer */
  size_t len;   /* total length (bytes actually stored) */
  size_t pos;   /* reading position */
  size_t limit; /* RAM limit (0 = FIO_DATA_RAM_LIMIT) */
  size_t capa;  /* once spilled: bytes mapped, or read buffer capacity */
  int fd;       /* temporary file, or -1 while in RAM */
  int err;      /* sticky error flag (see fio_data_is_error) */
  int map;      /* once spilled: mapping state */
} fio_data_s;

#define FIO_DATA_INIT /* all zero, except .fd = -1 */

void fio_data_init(fio_data_s *d, size_t ram_limit);
void fio_data_destroy(fio_data_s *d);
```

Initialize with `FIO_DATA_INIT` or `fio_data_init`, and release with `fio_data_destroy`. Destroying unmaps and closes the temporary file (if any), clears the error state and resets the store to empty, so it can be reused. It keeps the RAM limit. Treat the fields as read-only and use the functions below.

Temporary files are anonymous (see `fio_filename_tmp`): they have no name in the file system and disappear when closed, so nothing is left behind, even if the process crashes.

### Writing

```c
int  fio_data_write(fio_data_s *d, const void *data, size_t len);
void fio_data_expect(fio_data_s *d, size_t len);
int  fio_data_is_error(fio_data_s *d);
```

`fio_data_write` appends data. Once the total would exceed the RAM limit, the data moves to a temporary file (and the RAM buffer is released); later writes go to the file. It returns `0` on success or `-1` on error.

**Write errors are sticky.** Writing to a file (moving the RAM data, or appending) is retried for as long as it makes progress. It fails after two consecutive attempts that write nothing (creating the temporary file is also tried twice). The store then enters an error state:

- `fio_data_len` is the number of bytes actually stored. They are a readable prefix of the data written (on a failed move, the RAM data stays intact).
- The failing write and all later writes return `-1` without writing.
- RAM never grows past the RAM limit.
- `fio_data_is_error` returns `1`, until `fio_data_destroy` clears the state.

Memory allocation failures while writing are fatal (`FIO_ASSERT_ALLOC`); read buffer allocation failures aren't (see below).

`fio_data_expect` prepares for `len` more bytes. It reserves RAM, or moves to a temporary file early if the expected size crosses the limit. It's only a hint: if no temporary file can be created, the data stays in RAM and no error state is set (a later write past the limit will fail).

### Reading

```c
size_t         fio_data_len(fio_data_s *d);
size_t         fio_data_pos(fio_data_s *d);
int            fio_data_fd(fio_data_s *d);
char *         fio_data_bstr(fio_data_s *d);
size_t         fio_data_seek(fio_data_s *d, ssize_t pos);
fio_str_info_s fio_data_read(fio_data_s *d, size_t len);
fio_str_info_s fio_data_read_until(fio_data_s *d, char token, size_t limit);
```

- **Reads** start at the reading position and advance it by the number of bytes returned. `fio_data_read` returns up to `len` bytes (`(size_t)-1` reads all the remaining data). `fio_data_read_until` stops after `token` (inclusive), after `limit` bytes (ignored if zero), or at EOF. Both return an empty result at EOF.
- **Spilled data is memory mapped.** On the first read after the data moved to a temporary file, the whole file is mapped (read-only), and reads return pointers into the mapping, just like RAM reads return pointers into the RAM buffer. Nothing is copied, so reading everything at once is as cheap as reading in chunks.
- **Without mapping** (`FIO_DATA_NO_MMAP`, Windows, or when `mmap` fails, i.e., a file too big for the address space), reads copy the data into a heap buffer that grows to fit the request (no size cap). `fio_data_read_until` reads the file in `FIO_DATA_READ_BLOCK` blocks and stops at the token, so it never reads past `limit`. Read in bounded chunks to keep memory bounded.
- **Read failures fail the read, loudly.** If the heap buffer can't be allocated, or the file returns fewer bytes than requested (a short read: an I/O error, or a file truncated by someone else), an error is logged and the read returns an empty result (`buf == NULL`, `len == 0`) without moving the reading position, keeping `errno` (`EIO` for a short read that didn't report an error). Interrupted reads (`EINTR`) are retried by `fio_fd_read`. It never returns a truncated read and never aborts (the size may be controlled by a remote sender). Tell this apart from EOF by `fio_data_pos(d) < fio_data_len(d)`. The stored data is intact and the store is NOT put in an error state, so a later or smaller read may succeed. Whole-content users (FIOBJ `fiobj2cstr`, equality, HTTP body parsers) see such a failure as an empty / unequal / failed read, never as truncated data.
- **Read buffers** are borrowed: any later `fio_data_*` call on the same store MAY invalidate them (don't rely on any call keeping them valid). In particular, a write to a spilled store unmaps the file (it's mapped again, at its new size, by the next read), and `fio_data_destroy` releases everything.
- **`fio_data_seek`** sets the position and returns it. Negative values count back from the end: `-n` is `len - n`, so `-1` is the last byte (as with `lseek(fd, -1, SEEK_END)`), and values before the start clamp to `0`. Any position at or past the end is EOF: seek to `fio_data_len(d)` (or `SSIZE_MAX`, which is simply a large value) to reach the end. Writes never move the reading position.
- **`fio_data_fd`** returns the temporary file descriptor (for example, for `sendfile`), or `-1` while the data is in RAM.
- **`fio_data_bstr`** is the inverse of `fio_data_fd`: it returns the RAM data, a `fio_bstr` holding `fio_data_len(d)` bytes, or `NULL` once the data is in a temporary file (`fio_data_fd(d) != -1`) or before anything was stored. It's borrowed like a read buffer (any later `fio_data_*` call MAY invalidate it). To keep it, take a reference with `fio_bstr_copy` (no data copy) and release it with `fio_bstr_free`; the reference never changes, since later writes copy the buffer before changing it (copy on write). The HTTP/1.1 client sends RAM bodies this way (zero-copy).

### Saving

```c
int fio_data_save(fio_data_s *d, const char *filename);
```

Saves all the data to `filename`, replacing an existing file atomically. Readers see either the old file or the complete new one. RAM data is saved with `fio_filename_overwrite` (from the RAM buffer); spilled data with `fio_fd_save` (from the temporary file: a kernel side copy on Linux, positional block copies elsewhere), so saving never maps or reads the file into memory. See the [File Helpers](./004 files.md): the data is written to a sibling name (`<filename>.fio-tmp-<random>`) in the target's folder, `fsync`ed and renamed over `filename`. A single `fsync` (of the new file, before the rename) guarantees the old or the complete new content after a crash, never a partial file; the folder isn't `fsync`ed, so a crash right after the save may still leave the previous version in place. New files get mode `0666`, minus the `umask`. A leading `"~/"` is the home folder, as with `fio_filename_open`.

Returns `0` on success or `-1` on error (with `errno` set). On error, an existing `filename` is untouched and no sibling file is left behind. A short read of the temporary file fails the save (never saves truncated data). A store in an error state is refused (`-1`, `errno == EIO`), since its data is incomplete.

Saving doesn't change the store: the data, reading position and buffers returned earlier stay the same, and the store stays usable.

### Example

```c
fio_data_s d = FIO_DATA_INIT;
fio_data_write(&d, "hello ", 6);
fio_data_write(&d, "world", 5);
fio_data_seek(&d, 0);
for (fio_str_info_s s; (s = fio_data_read(&d, 4096)).len;)
  fwrite(s.buf, 1, s.len, stdout);
if (fio_data_save(&d, "hello.txt"))
  perror("couldn't save");
fio_data_destroy(&d);
```
