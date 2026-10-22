## Data Store (`fio_data_s`)

```c
#define FIO_DATA
#include FIO_INCLUDE_FILE
```

> **Requires:** `FIO_STR` and `FIO_FILES`. When using `include.h`, these dependencies are resolved automatically. The HTTP and FIOBJ modules include this module.

A seekable data store that keeps data in RAM until it grows past a limit, then moves it to a temporary file. This is the storage behind HTTP request and response bodies, and it's useful anywhere large payloads (such as file uploads) shouldn't stay in memory.

### Settings

```c
#define FIO_DATA_RAM_LIMIT (1UL << 17) /* default RAM limit, in bytes */
```

### Type and lifetime

```c
typedef struct {
  char *buf;    /* RAM data (fio_bstr), or the last read once spilled */
  size_t len;   /* total length */
  size_t pos;   /* reading position */
  size_t limit; /* RAM limit (0 = FIO_DATA_RAM_LIMIT) */
  int fd;       /* temporary file, or -1 while in RAM */
} fio_data_s;

#define FIO_DATA_INIT { .buf = NULL, .len = 0, .pos = 0, .limit = 0, .fd = -1 }

void fio_data_init(fio_data_s *d, size_t ram_limit);
void fio_data_destroy(fio_data_s *d);
```

Initialize with `FIO_DATA_INIT` or `fio_data_init`, and release with `fio_data_destroy`. Destroying closes the temporary file (if any) and resets the store to empty, so it can be reused. It keeps the RAM limit. Treat the fields as read-only and use the functions below.

### Writing

```c
int  fio_data_write(fio_data_s *d, const void *data, size_t len);
void fio_data_expect(fio_data_s *d, size_t len);
```

`fio_data_write` appends data. Once the total would exceed the RAM limit, the data moves to a temporary file and later writes go there. It returns `0` on success or `-1` if the data couldn't be fully written. If no temporary file can be created, the data stays in RAM.

`fio_data_expect` prepares for `len` more bytes. It reserves RAM, or moves to a temporary file early if the expected size crosses the limit.

### Reading

```c
size_t         fio_data_len(fio_data_s *d);
size_t         fio_data_pos(fio_data_s *d);
int            fio_data_fd(fio_data_s *d);
size_t         fio_data_seek(fio_data_s *d, ssize_t pos);
fio_str_info_s fio_data_read(fio_data_s *d, size_t len);
fio_str_info_s fio_data_read_until(fio_data_s *d, char token, size_t limit);
```

- **Reads** start at the reading position and advance it. `fio_data_read` returns up to `len` bytes. `fio_data_read_until` stops after `token`, after `limit` bytes (ignored if zero), or at EOF. Both return an empty result at EOF.
- **Read buffers** are borrowed. They stay valid until the next read, write, or destroy. While in RAM, reads point into the store's own buffer. Once spilled, reads go through `buf`, so read in bounded chunks to keep memory bounded.
- **`fio_data_seek`** sets the position and returns it. Negative values count from the end, `SSIZE_MAX` keeps the current position, and the result is clamped to the data length.
- **`fio_data_fd`** returns the temporary file descriptor (for example, for `sendfile`), or `-1` while the data is in RAM.

### Example

```c
fio_data_s d = FIO_DATA_INIT;
fio_data_write(&d, "hello ", 6);
fio_data_write(&d, "world", 5);
fio_data_seek(&d, 0);
for (fio_str_info_s s; (s = fio_data_read(&d, 4096)).len;)
  fwrite(s.buf, 1, s.len, stdout);
fio_data_destroy(&d);
```
