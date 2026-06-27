# Packet Data Stream

```c
#define FIO_STREAM
#include "fio-stl.h"
```

A packet-based byte stream for buffering data from memory buffers and file descriptors. It is useful when writes are partial: add packets to the stream, read the next contiguous chunk into or from a scratch buffer, write what you can, then advance by the number of bytes written.

For the low-level helpers commonly used with streams, see [`004 files.md`](./004%20files.md), [`004 sock.md`](./004%20sock.md), and [`102 poll api.md`](./102%20poll%20api.md).

---

## Configuration Macros

#### `FIO_STREAM_COPY_PER_PACKET`

```c
#define FIO_STREAM_COPY_PER_PACKET 98304
```

Maximum copied payload per packet. Larger copied buffers are split into multiple packets so memory can be released progressively. Override before including the implementation.

#### `FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN`

```c
#define FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN 116 /* 8 in DEBUG */
```

Small buffers are copied even when `copy_buffer == 0`, improving locality and avoiding tiny external references.

---

## Types

#### `fio_stream_s`

```c
typedef struct {
  fio_stream_packet_s *next;
  fio_stream_packet_s **pos;
  size_t consumed;
  size_t length;
} fio_stream_s;
```

Stream object. Treat the fields as private. Allocate on the stack with `FIO_STREAM_INIT` or on the heap with `fio_stream_new`.

#### `fio_stream_packet_s`

```c
typedef struct fio_stream_packet_s fio_stream_packet_s;
```

Opaque packed data node. Packets are prepared separately, then transferred to a stream with `fio_stream_add`.

---

## Lifecycle

#### `FIO_STREAM_INIT`

```c
#define FIO_STREAM_INIT(s) { .next = NULL, .pos = &(s).next }
```

Initializer for an in-place stream.

```c
fio_stream_s stream = FIO_STREAM_INIT(stream);
```

#### `fio_stream_new`

```c
fio_stream_s *fio_stream_new(void);
```

Allocates and initializes a heap stream.

**Returns:** stream pointer, or `NULL` on allocation failure.

Not declared when `FIO_REF_CONSTRUCTOR_ONLY` is defined.

#### `fio_stream_free`

```c
int fio_stream_free(fio_stream_s *stream);
```

Destroys `stream`, frees the stream object itself, and returns `0`.

Not declared when `FIO_REF_CONSTRUCTOR_ONLY` is defined.

#### `fio_stream_destroy`

```c
void fio_stream_destroy(fio_stream_s *stream);
```

Frees all queued packets and re-initializes the stream object. Safe to call with `NULL`.

---

## Packing Data

#### `fio_stream_pack_data`

```c
fio_stream_packet_s *fio_stream_pack_data(void *buf,
                                          size_t len,
                                          size_t offset,
                                          uint8_t copy_buffer,
                                          void (*dealloc_func)(void *));
```

Packs `len` bytes starting at `((char *)buf + offset)`.

- If `copy_buffer` is non-zero, or `len < FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN`, bytes are copied into stream-owned packets.
- Otherwise the packet references `buf` and calls `dealloc_func(buf)` when the packet is freed, if `dealloc_func` is not `NULL`.
- Copied data larger than `FIO_STREAM_COPY_PER_PACKET` is split into multiple packets.

**Returns:** packet pointer, or `NULL` if `buf == NULL`, `len == 0`, the length is too large for the packet format, or allocation fails. If `dealloc_func` is provided, it is called on both success-after-copy and failure.

#### `fio_stream_pack_fd`

```c
fio_stream_packet_s *fio_stream_pack_fd(int fd,
                                        size_t len,
                                        size_t offset,
                                        uint8_t keep_open);
```

Packs bytes from an open file descriptor. Reads are performed later by `fio_stream_read` using `fio_fd_read`.

- `len == 0` auto-detects the remaining file size with `fio_fd_size(fd)`.
- `offset` is the starting file offset.
- If `keep_open == 0`, the descriptor is closed when the packet is freed, or on pack failure after ownership was accepted.
- If `keep_open != 0`, the caller remains responsible for closing `fd`.

**Returns:** packet pointer, or `NULL` on invalid input, size detection failure / oversized auto-detected size when `len == 0`, or allocation failure.

#### `fio_stream_add`

```c
void fio_stream_add(fio_stream_s *stream, fio_stream_packet_s *packet);
```

Appends `packet` to `stream` and transfers packet ownership to the stream. If `stream` or `packet` is `NULL`, the packet is freed.

This is not thread-safe.

#### `fio_stream_pack_free`

```c
void fio_stream_pack_free(fio_stream_packet_s *packet);
```

Frees a packet that was not added to a stream. Do not call this after `fio_stream_add` succeeds.

---

## Reading and Consuming

#### `fio_stream_read`

```c
void fio_stream_read(fio_stream_s *stream, char **buf, size_t *len);
```

Reads the next available bytes without consuming them.

Before the call, `*buf` must point to a scratch buffer with at least `*len` bytes. This buffer is required when data spans multiple packets or comes from a file descriptor.

After the call:

- On empty stream or error, `*buf == NULL` and `*len == 0`.
- Otherwise `*buf` either still points to the supplied scratch buffer or points directly into stream-owned memory.
- `*len` is updated to the number of readable bytes available at `*buf`.

Reset both `*buf` and `*len` before each read if reusing the same scratch buffer. This is not thread-safe.

#### `fio_stream_advance`

```c
void fio_stream_advance(fio_stream_s *stream, size_t len);
```

Consumes `len` bytes from the stream, freeing fully consumed packets. Usually pass the number of bytes actually written to the destination.

This is not thread-safe.

#### `fio_stream_any`

```c
uint8_t fio_stream_any(fio_stream_s *stream);
```

Returns non-zero when `stream` has pending packets. Returns `0` for `NULL` or empty streams. This is not truly thread-safe.

#### `fio_stream_length`

```c
size_t fio_stream_length(fio_stream_s *stream);
```

Returns the number of bytes waiting in the stream. Call with a valid stream object. This is not truly thread-safe.

---

## Ownership and Thread-Safety

Packing creates a packet owned by the caller. `fio_stream_add` transfers that ownership to the stream. `fio_stream_destroy`, `fio_stream_free`, and `fio_stream_advance` free packets as needed.

For referenced memory packets, the referenced buffer must remain valid until the packet is freed. Pass `copy_buffer != 0` when that lifetime is not guaranteed. For file descriptor packets, keep the descriptor readable until the packet is consumed or destroyed; `keep_open` controls who closes it.

Packing packets can be done before taking the stream lock, but mutating or consuming a single `fio_stream_s` (`add`, `read`, `advance`, and length checks) should be externally synchronized when shared across threads.

---

## Example

```c
#define FIO_STREAM
#include "fio-stl.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  fio_stream_s stream = FIO_STREAM_INIT(stream);
  char scratch[4096];

  char msg[] = "hello, stream\n";
  fio_stream_packet_s *p = fio_stream_pack_data(msg, strlen(msg), 0, 1, NULL);
  fio_stream_add(&stream, p);

  while (fio_stream_any(&stream)) {
    char *buf = scratch;
    size_t len = sizeof(scratch);

    fio_stream_read(&stream, &buf, &len);
    if (!len)
      break;

    size_t written = fwrite(buf, 1, len, stdout);
    fio_stream_advance(&stream, written);

    if (written != len)
      break;
  }

  fio_stream_destroy(&stream);
  return 0;
}
```

### File Descriptor Packet

```c
#define FIO_STREAM
#define FIO_FILES
#include "fio-stl.h"

fio_stream_s stream = FIO_STREAM_INIT(stream);
int fd = fio_filename_open("./asset.bin", O_RDONLY);
fio_stream_packet_s *p = fio_stream_pack_fd(fd, 0, 0, 0); /* stream closes fd */
fio_stream_add(&stream, p);

/* ... read / write packets ... */
fio_stream_destroy(&stream);
```

------------------------------------------------------------
