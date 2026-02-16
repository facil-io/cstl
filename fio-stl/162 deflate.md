## DEFLATE / Gzip Compression

```c
#define FIO_DEFLATE
#include "fio-stl.h"
```

Provides raw DEFLATE compression/decompression (RFC 1951) plus gzip wrappers (RFC 1952). Designed for WebSocket permessage-deflate (RFC 7692) and static file compression.

Key design features:

- 64-bit branchless bit buffer for high throughput decompression
- Two-level packed Huffman tables (11-bit litlen, 8-bit distance, ~12KB on stack)
- LZ77 with hash chain matching, lazy evaluation, and 8-byte word-at-a-time match extension
- Automatic fixed vs. dynamic Huffman block selection based on cost comparison
- Streaming API with context takeover for WebSocket permessage-deflate

Compression levels: 0 = store (no compression), 1 = fast greedy (sparse hash insertion), 2-3 = greedy (sparse insertion), 4-6 = lazy matching (full hash insertion), 7-9 = maximum compression (deep chain search).

**Note:** this module depends on `FIO_CRC32` which will be automatically included.

### Compression Bounds

#### `fio_deflate_compress_bound`

```c
size_t fio_deflate_compress_bound(size_t in_len);
```

Returns a conservative upper bound on the compressed output size for raw DEFLATE. The bound accounts for stored block headers (5 bytes per 65535 bytes of data) plus dynamic Huffman header overhead.

Use this to allocate an output buffer that is guaranteed to be large enough:

```c
size_t bound = fio_deflate_compress_bound(in_len);
void *out = malloc(bound);
size_t compressed = fio_deflate_compress(out, bound, data, in_len, 6);
```

#### `fio_deflate_decompress_bound`

```c
size_t fio_deflate_decompress_bound(size_t in_len);
```

Returns a conservative upper bound on the decompressed size (1032x expansion ratio, minimum 4096 bytes). The DEFLATE theoretical maximum is 1032:1 but practical data is much less.

**Note:** for precise sizing, call `fio_deflate_decompress` with `out = NULL` and `out_len = 0` to query the exact decompressed size without allocating an output buffer.

### Raw DEFLATE API

#### `fio_deflate_compress`

```c
size_t fio_deflate_compress(void *out,
                            size_t out_len,
                            const void *in,
                            size_t in_len,
                            int level);
```

Compresses data using raw DEFLATE (no zlib/gzip headers or trailers).

- `out` - output buffer for compressed data
- `out_len` - capacity of the output buffer in bytes
- `in` - input data to compress
- `in_len` - length of input data in bytes
- `level` - compression level: 0 = store, 1-3 = fast, 4-6 = normal, 7-9 = best

**Returns:** compressed length on success, 0 on error (output buffer too small or allocation failure).

**Note:** when `in` is NULL or `in_len` is 0, emits an empty stored block (5 bytes). The output buffer must have at least 5 bytes available.

#### `fio_deflate_decompress`

```c
size_t fio_deflate_decompress(void *out,
                              size_t out_len,
                              const void *in,
                              size_t in_len);
```

Decompresses raw DEFLATE data (no zlib/gzip headers).

- `out` - output buffer for decompressed data (may be NULL for size query)
- `out_len` - capacity of the output buffer in bytes (may be 0 for size query)
- `in` - compressed input data
- `in_len` - length of compressed data in bytes

**Returns:**

- On success: decompressed byte count (`<= out_len`).
- On buffer too small: the **required** buffer size (`> out_len`). Callers should reallocate and retry.
- On corrupt/invalid data: `0`.
- When `out == NULL` or `out_len == 0`: performs a full decode pass counting output bytes and returns the required size (`0` if data is corrupt). This allows callers to query the decompressed size without allocating.

**Note:** the size-return behavior on buffer overflow is a key design feature. When the return value exceeds `out_len`, the caller knows exactly how many bytes to allocate for a successful retry. A return of `0` always indicates corrupt or invalid data.

### Gzip API

#### `fio_gzip_compress`

```c
size_t fio_gzip_compress(void *out,
                         size_t out_len,
                         const void *in,
                         size_t in_len,
                         int level);
```

Compresses data with a gzip wrapper (suitable for HTTP `Content-Encoding: gzip`).

Writes a 10-byte gzip header, raw DEFLATE compressed data, and an 8-byte trailer containing the CRC32 checksum and original data size (ISIZE).

- `out` - output buffer (must have room for at least 18 bytes: 10 header + 0 data + 8 trailer)
- `out_len` - capacity of the output buffer in bytes
- `in` - input data to compress
- `in_len` - length of input data in bytes
- `level` - compression level: 0 = store, 1-3 = fast, 4-6 = normal, 7-9 = best

**Returns:** total output length on success (header + compressed data + trailer), 0 on error.

#### `fio_gzip_decompress`

```c
size_t fio_gzip_decompress(void *out,
                           size_t out_len,
                           const void *in,
                           size_t in_len);
```

Decompresses gzip data. Validates the gzip header, decompresses the DEFLATE payload, and verifies the CRC32 checksum and ISIZE trailer.

- `out` - output buffer for decompressed data (may be NULL for size query)
- `out_len` - capacity of the output buffer in bytes (may be 0 for size query)
- `in` - gzip compressed input data
- `in_len` - length of compressed data in bytes

**Returns:**

- On success: decompressed byte count (`<= out_len`).
- On buffer too small: the **required** buffer size (`> out_len`). Only ISIZE is verified (CRC32 requires actual data).
- On corrupt/invalid data or checksum mismatch: `0`.
- When `out == NULL` or `out_len == 0`: returns required size (`0` if corrupt).

**Note:** supports gzip files with optional FEXTRA, FNAME, FCOMMENT, and FHCRC header fields. The minimum valid gzip input is 18 bytes (10 header + 0 data + 8 trailer).

### Streaming API

The streaming API provides incremental compression and decompression with context takeover, designed for WebSocket permessage-deflate (RFC 7692).

**Context takeover** means the compressor and decompressor maintain a 32KB sliding window across multiple `fio_deflate_push` calls. This allows LZ77 back-references to reach into data from previous messages, significantly improving compression ratio for repetitive WebSocket traffic.

**Protocol details:**

- **Compression:** each flush emits a non-final DEFLATE block (`BFINAL=0`) followed by a sync flush marker (`0x00 0x00 0xFF 0xFF`). For WebSocket permessage-deflate, strip the last 4 bytes (the sync marker) before sending the frame.
- **Decompression:** on flush, the sync flush marker (`0x00 0x00 0xFF 0xFF`) is automatically re-appended to the buffered compressed data before decompression. For WebSocket permessage-deflate, pass the received frame data directly (without the sync marker) and call with `flush = 1`.

#### `fio_deflate_s`

```c
typedef struct fio_deflate_s fio_deflate_s;
```

Opaque streaming deflate/inflate state. Maintains the sliding window, hash chain (compressor), and input buffer across multiple `fio_deflate_push` calls.

#### `fio_deflate_new`

```c
fio_deflate_s *fio_deflate_new(int level, int is_compress);
```

Creates a new streaming deflate/inflate state.

- `level` - compression level 1-9 (clamped to this range; ignored for decompression)
- `is_compress` - non-zero for compression, 0 for decompression

**Returns:** a new `fio_deflate_s` pointer on success, NULL on allocation failure.

**Note:** for compression, this allocates ~384KB for the hash chain tables (head + generation + prev arrays). For decompression, only the base struct (~32KB for the sliding window) is allocated.

#### `fio_deflate_free`

```c
void fio_deflate_free(fio_deflate_s *s);
```

Frees a streaming deflate/inflate state and all associated memory (hash tables, input buffer).

- `s` - streaming state to free (may be NULL, in which case this is a no-op)

#### `fio_deflate_destroy`

```c
void fio_deflate_destroy(fio_deflate_s *s);
```

Resets a streaming deflate/inflate context. Clears the sliding window, resets the hash chain generation counter (compressor), and resets the input buffer length. Keeps all allocated memory for reuse.

Use this to start a new compression/decompression session without reallocating:

```c
fio_deflate_s *ctx = fio_deflate_new(6, 1);
/* ... compress messages ... */
fio_deflate_destroy(ctx); /* reset for new session */
/* ... compress more messages ... */
fio_deflate_free(ctx);    /* done, release memory */
```

- `s` - streaming state to reset (may be NULL, in which case this is a no-op)

#### `fio_deflate_push`

```c
size_t fio_deflate_push(fio_deflate_s *s,
                        void *out,
                        size_t out_len,
                        const void *in,
                        size_t in_len,
                        int flush);
```

Streaming compress/decompress. Accumulates input data and produces output on flush or when the internal buffer is full (compressor only, at 32KB).

- `s` - streaming state created by `fio_deflate_new`
- `out` - output buffer for compressed/decompressed data
- `out_len` - capacity of the output buffer in bytes
- `in` - input data (may be NULL if only flushing)
- `in_len` - length of input data in bytes (may be 0 if only flushing)
- `flush` - 0 for normal buffering, 1 for sync flush (emit output now)

**Returns:**

- **Compression:** compressed byte count on success, 0 if data was buffered (no flush) or on error.
- **Decompression:**
  - Decompressed byte count on success (`<= out_len`).
  - Required buffer size when buffer too small (`> out_len`). The internal buffer is **preserved** for retry with a larger output buffer.
  - `0` on corrupt/invalid data, allocation failure, or if data was buffered (no flush).

**Note:** for WebSocket permessage-deflate, always call with `flush = 1` at message boundaries. The compressor emits a sync flush marker; the decompressor automatically re-appends the `0x00 0x00 0xFF 0xFF` sync marker before decompressing.

### Examples

#### One-shot compression and decompression

```c
#define FIO_DEFLATE
#include "fio-stl.h"

void example_oneshot(void) {
  const char *data = "Hello, DEFLATE compression!";
  size_t data_len = strlen(data);

  /* Compress */
  size_t bound = fio_deflate_compress_bound(data_len);
  void *compressed = malloc(bound);
  size_t comp_len = fio_deflate_compress(compressed, bound, data, data_len, 6);

  /* Query decompressed size */
  size_t needed = fio_deflate_decompress(NULL, 0, compressed, comp_len);

  /* Decompress */
  void *output = malloc(needed);
  size_t dec_len = fio_deflate_decompress(output, needed, compressed, comp_len);

  free(output);
  free(compressed);
}
```

#### Gzip for HTTP responses

```c
void example_gzip(const void *body, size_t body_len) {
  size_t bound = fio_deflate_compress_bound(body_len) + 18;
  void *gzipped = malloc(bound);
  size_t gz_len = fio_gzip_compress(gzipped, bound, body, body_len, 6);
  if (gz_len) {
    /* Send gzipped response with Content-Encoding: gzip */
  }
  free(gzipped);
}
```

#### Streaming WebSocket permessage-deflate

```c
void example_websocket_streaming(void) {
  /* Create compressor and decompressor with context takeover */
  fio_deflate_s *comp = fio_deflate_new(6, 1);  /* compress, level 6 */
  fio_deflate_s *decomp = fio_deflate_new(0, 0); /* decompress */

  char out[4096];

  /* Compress a message (flush=1 at message boundary) */
  const char *msg = "Hello WebSocket!";
  size_t comp_len = fio_deflate_push(comp, out, sizeof(out),
                                     msg, strlen(msg), 1);
  /* For permessage-deflate: strip last 4 bytes (sync marker) before sending */
  size_t send_len = comp_len - 4;

  /* Decompress (receiver re-appends sync marker internally on flush) */
  char decoded[4096];
  size_t dec_len = fio_deflate_push(decomp, decoded, sizeof(decoded),
                                    out, send_len, 1);

  /* Reset for new session (keeps memory allocated) */
  fio_deflate_destroy(comp);
  fio_deflate_destroy(decomp);

  /* Free when done */
  fio_deflate_free(comp);
  fio_deflate_free(decomp);
}
```

------------------------------------------------------------
