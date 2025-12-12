## Data Stream Container

```c
#define FIO_STREAM
#include "fio-stl.h"
```

Data Stream objects solve the issues that could arise when `write` operations don't write all the data (due to OS buffering). 

Data Streams offer a way to store / concat different data sources (static strings, dynamic strings, files) as a single data stream. This allows the data to be easily written to an IO target (socket / pipe / file) using the `write` operation.

By defining the macro `FIO_STREAM`, the following macros and functions will be defined.

### Configuration Macros

#### `FIO_STREAM_COPY_PER_PACKET`

```c
#define FIO_STREAM_COPY_PER_PACKET 98304
```

When copying data to the stream, large memory sections will be divided into smaller allocations in order to free memory faster and minimize the direct use of `mmap`.

This macro should be set according to the specific allocator limits. By default, it is set to 96Kb (98304 bytes).

#### `FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN`

```c
#define FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN 116
```

If the data added is less than this number of bytes, copying is preferred over referencing for better memory locality. By default, it is set to 116 bytes (or 8 bytes in DEBUG mode).

### Types

#### `fio_stream_s`

```c
typedef struct {
  /* do not directly access! */
  fio_stream_packet_s *next;
  fio_stream_packet_s **pos;
  size_t consumed;
  size_t length;
} fio_stream_s;
```

The `fio_stream_s` type should be considered opaque and only accessed through the following API.

#### `fio_stream_packet_s`

The `fio_stream_packet_s` type should be considered opaque and only accessed through the following API.

This type is used to separate data packing from any updates made to the stream object, allowing data packing to be performed concurrently with stream reading / updating (which requires a lock in multi-threaded applications).

### Initialization and Destruction

#### `FIO_STREAM_INIT`

```c
#define FIO_STREAM_INIT(s)                                                     \
  { .next = NULL, .pos = &(s).next }
```

Object initialization macro.

#### `fio_stream_new`

```c
fio_stream_s *fio_stream_new(void);
```

Allocates a new object on the heap and initializes its memory.

**Returns:** a pointer to the newly allocated stream, or NULL on allocation failure.

#### `fio_stream_free`

```c
int fio_stream_free(fio_stream_s *stream);
```

Frees any internal data AND the object's container!

**Parameters:**
- `stream` - the stream object to free

**Returns:** 0.

#### `fio_stream_destroy`

```c
void fio_stream_destroy(fio_stream_s *stream);
```

Destroys the object, reinitializing its container.

**Parameters:**
- `stream` - the stream object to destroy

### Stream Information

#### `fio_stream_any`

```c
uint8_t fio_stream_any(fio_stream_s *stream);
```

Returns true if there's any data in the stream.

**Parameters:**
- `stream` - the stream object to check

**Returns:** non-zero if there's data in the stream, 0 otherwise.

**Note**: this isn't truly thread safe, but it often doesn't matter if it is.

#### `fio_stream_length`

```c
size_t fio_stream_length(fio_stream_s *stream);
```

Returns the number of bytes waiting in the stream.

**Parameters:**
- `stream` - the stream object to query

**Returns:** the number of bytes in the stream.

**Note**: this isn't truly thread safe, but it often doesn't matter if it is.

### Packing Data into the Stream

#### `fio_stream_pack_data`

```c
fio_stream_packet_s *fio_stream_pack_data(void *buf,
                                          size_t len,
                                          size_t offset,
                                          uint8_t copy_buffer,
                                          void (*dealloc_func)(void *));
```

Packs data into a `fio_stream_packet_s` container.

**Parameters:**
- `buf` - pointer to the data buffer
- `len` - length of the data in bytes
- `offset` - offset within the buffer to start from
- `copy_buffer` - if non-zero, the data will be copied; otherwise, the buffer is referenced
- `dealloc_func` - function to call to free the buffer when done (can be NULL)

**Returns:** a pointer to the packet, or NULL on error.

**Note**: can be performed concurrently with other stream operations. If `copy_buffer` is set or if `len` is less than `FIO_STREAM_ALWAYS_COPY_IF_LESS_THAN`, the data will be copied. Large data blocks may be split into multiple packets based on `FIO_STREAM_COPY_PER_PACKET`. If `dealloc_func` is provided, it will be called even on error.

#### `fio_stream_pack_fd`

```c
fio_stream_packet_s *fio_stream_pack_fd(int fd,
                                        size_t len,
                                        size_t offset,
                                        uint8_t keep_open);
```

Packs a file descriptor into a `fio_stream_packet_s` container.

**Parameters:**
- `fd` - the file descriptor to pack
- `len` - number of bytes to read from the file (0 to auto-detect from file size)
- `offset` - offset within the file to start reading from
- `keep_open` - if non-zero, the file descriptor will NOT be closed when the packet is freed

**Returns:** a pointer to the packet, or NULL on error.

**Note**: if `len` is 0, the file size will be queried and `len` will be set to `file_size - offset`. If `keep_open` is 0 and an error occurs, the file descriptor will be closed.

#### `fio_stream_add`

```c
void fio_stream_add(fio_stream_s *stream, fio_stream_packet_s *packet);
```

Adds a packet to the stream.

**Parameters:**
- `stream` - the stream to add the packet to
- `packet` - the packet to add

**Note**: this isn't thread safe. If `stream` or `packet` is NULL, the packet will be freed.

#### `fio_stream_pack_free`

```c
void fio_stream_pack_free(fio_stream_packet_s *packet);
```

Destroys the `fio_stream_packet_s` - call this ONLY if the packed data was never added to the stream using `fio_stream_add`.

**Parameters:**
- `packet` - the packet to free

### Reading / Consuming Data from the Stream

#### `fio_stream_read`

```c
void fio_stream_read(fio_stream_s *stream, char **buf, size_t *len);
```

Reads data from the stream (if any), leaving the data in the stream **without advancing the reading position** (see [`fio_stream_advance`](#fio_stream_advance)).

`buf` MUST point to a buffer with - at least - `len` bytes. This is required in case the packed data is fragmented or references a file and needs to be copied to an available buffer.

On error, or if the stream is empty, `buf` will be set to NULL and `len` will be set to zero.

Otherwise, `buf` may retain the same value or it may point directly to a memory address within the stream's buffer (the original value may be lost) and `len` will be updated to the largest possible value for valid data that can be read from `buf`.

**Parameters:**
- `stream` - the stream to read from
- `buf` - pointer to a buffer pointer (will be updated)
- `len` - pointer to the buffer length (will be updated)

**Note**: this isn't thread safe.

#### `fio_stream_advance`

```c
void fio_stream_advance(fio_stream_s *stream, size_t len);
```

Advances the Stream, so the first `len` bytes are marked as consumed.

**Parameters:**
- `stream` - the stream to advance
- `len` - number of bytes to mark as consumed

**Note**: this isn't thread safe.

-------------------------------------------------------------------------------
