## Data Stream

Data Stream objects solve the issues that could arise when `write` operations don't write all the data (due to OS buffering). 

Data Streams offer a way to store / concat different data sources (static strings, dynamic strings, files) as a single data stream. This allows the data to be easily written to an IO target (socket / pipe / file) using the `write` operation.

By defining the macro `FIO_STREAM`, the following macros and functions will be defined.

#### `fio_stream_s`

```c
typedef struct {
  /* do not directly acecss! */
  fio_stream_packet_s *next;
  fio_stream_packet_s **pos;
  uint32_t consumed;
  uint32_t packets;
} fio_stream_s;
```

The `fio_stream_s` type should be considered opaque and only accessed through the following API.

#### `fio_stream_packet_s`

The `fio_stream_packet_s` type should be considered opaque and only accessed through the following API.

This type is used to separate data packing with any updates made to the stream object, allowing data packing to be performed concurrently with stream reading / updating (which requires a lock in multi-threaded applications).


#### `FIO_STREAM_INIT(stream)`

```c
#define FIO_STREAM_INIT(s)                                                     \
  { .next = NULL, .pos = &(s).next }
#endif
```

Object initialization macro.

#### `fio_stream_new`

```c
fio_stream_s *fio_stream_new(void);
```

Allocates a new object on the heap and initializes it's memory.

#### `fio_stream_free`

```c
int fio_stream_free(fio_stream_s *stream);
```

Frees any internal data AND the object's container!

#### `fio_stream_destroy`

```c
void fio_stream_destroy(fio_stream_s *stream);
```

Destroys the object, reinitializing its container.

#### `fio_stream_any`

```c
uint8_t fio_stream_any(fio_stream_s *stream);
````

Returns true if there's any data in the stream.

**Note**: this isn't thread safe, but it often doesn't matter if it is.

#### `fio_stream_packets`

```c
uint32_t fio_stream_packets(fio_stream_s *stream);
````

Returns the number of packets waiting in the stream.

**Note**: this isn't thread safe, but it often doesn't matter if it is.

### Packing data into the stream

#### `fio_stream_pack_data`

```c
fio_stream_packet_s *fio_stream_pack_data(void *buf,
                                          size_t len,
                                          size_t offset,
                                          uint8_t copy_buffer,
                                          void (*dealloc_func)(void *));
```

Packs data into a `fio_stream_packet_s` container.

Can be performed concurrently with other operations.

#### `fio_stream_pack_fd`

```c
fio_stream_packet_s * fio_stream_pack_fd(int fd, size_t len, size_t offset, uint8_t keep_open);
```

Packs a file descriptor into a `fio_stream_packet_s` container. 

#### `fio_stream_add`

```c
void fio_stream_add(fio_stream_s *stream, fio_stream_packet_s *packet);
```

Adds a packet to the stream.

**Note**: this isn't thread safe.

#### `fio_stream_pack_free`

```c
void fio_stream_pack_free(fio_stream_packet_s *packet);
```

Destroys the `fio_stream_packet_s` - call this ONLY if the packed data was never added to the stream using `fio_stream_add`. 


### Reading / Consuming data from the Stream


#### `fio_stream_read`

```c
void fio_stream_read(fio_stream_s *stream, char **buf, size_t *len);
```

Reads data from the stream (if any), leaving it in the stream.

`buf` MUST point to a buffer with - at least - `len` bytes. This is required in case the packed data is fragmented or references a file and needs to be copied to an available buffer.

On error, or if the stream is empty, `buf` will be set to NULL and `len` will be set to zero.

Otherwise, `buf` may retain the same value or it may point directly to a memory address wiithin the stream's buffer (the original value may be lost) and `len` will be updated to the largest possible value for valid data that can be read from `buf`.

**Note**: this isn't thread safe.

#### `fio_stream_advance`

```c
void fio_stream_advance(fio_stream_s *stream, size_t len);
```

Advances the Stream, so the first `len` bytes are marked as consumed.

**Note**: this isn't thread safe.

-------------------------------------------------------------------------------

