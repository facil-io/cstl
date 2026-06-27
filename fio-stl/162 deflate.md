# DEFLATE / Gzip

```c
#define FIO_DEFLATE
#include "fio-stl.h"
```

Raw DEFLATE compression/decompression (RFC 1951), gzip wrappers (RFC 1952), and a streaming API for WebSocket `permessage-deflate` (RFC 7692).

This module is built around a 64-bit bit buffer, packed Huffman tables, a 32KB sliding window, and word-at-a-time match copies. Translation: it tries hard not to be slow.

## Raw DEFLATE

### `fio_deflate_decompress`

```c
SFUNC size_t fio_deflate_decompress(void *out,
                                    size_t out_len,
                                    const void *in,
                                    size_t in_len);
```

Decompresses raw DEFLATE data with no zlib or gzip wrapper.

Return values:

| Condition | Return value |
| --- | --- |
| Success | Decompressed byte count, `<= out_len`. |
| Output too small | Required buffer size, `> out_len`. |
| Corrupt input | `0`. |
| Size query | Required size, or `0` if corrupt. |

Pass `out == NULL` or `out_len == 0` to run a decode pass that counts output bytes without writing them.

### `fio_deflate_decompress_bound`

```c
FIO_IFUNC size_t fio_deflate_decompress_bound(size_t in_len);
```

Returns a conservative decompression bound. For exact sizing, prefer the `fio_deflate_decompress(NULL, 0, ...)` query mode.

### `fio_deflate_compress`

```c
SFUNC size_t fio_deflate_compress(void *out,
                                  size_t out_len,
                                  const void *in,
                                  size_t in_len,
                                  int level);
```

Compresses raw DEFLATE data.

Compression levels:

- `0`: store only.
- `1–3`: fast.
- `4–6`: normal.
- `7–9`: best compression.

Returns compressed length on success, `0` on error.

### `fio_deflate_compress_bound`

```c
FIO_IFUNC size_t fio_deflate_compress_bound(size_t in_len);
```

Returns an upper bound for raw DEFLATE output size.

## Gzip Wrappers

### `fio_gzip_compress`

```c
SFUNC size_t fio_gzip_compress(void *out,
                               size_t out_len,
                               const void *in,
                               size_t in_len,
                               int level);
```

Compresses with a gzip wrapper for uses such as HTTP `Content-Encoding: gzip`. The output includes the gzip header, DEFLATE payload, CRC32, and original size trailer.

Returns total output length, or `0` on error.

### `fio_gzip_decompress`

```c
SFUNC size_t fio_gzip_decompress(void *out,
                                 size_t out_len,
                                 const void *in,
                                 size_t in_len);
```

Decompresses gzip data, including wrapper validation and CRC/size checks when data is actually decoded.

Return modes mirror `fio_deflate_decompress`: byte count, required size, or `0` on invalid input.

## Streaming API

### `fio_deflate_s`

```c
typedef struct fio_deflate_s fio_deflate_s;
```

Opaque streaming compression/decompression state. It keeps window state across calls for context takeover.

### `fio_deflate_new`

```c
SFUNC fio_deflate_s *fio_deflate_new(int level, int is_compress);
```

Creates a streaming state. `is_compress != 0` creates a compressor; `0` creates a decompressor. Returns `NULL` on allocation failure.

### `fio_deflate_free`

```c
SFUNC void fio_deflate_free(fio_deflate_s *s);
```

Frees a streaming state.

### `fio_deflate_destroy`

```c
SFUNC void fio_deflate_destroy(fio_deflate_s *s);
```

Resets a streaming context while keeping allocated memory.

### `fio_deflate_push`

```c
SFUNC size_t fio_deflate_push(fio_deflate_s *s,
                              void *out,
                              size_t out_len,
                              const void *in,
                              size_t in_len,
                              int flush);
```

Compresses or decompresses the next input chunk.

- `flush == 0`: normal streaming.
- `flush == 1`: sync flush, useful at WebSocket frame boundaries.

For decompression, a return value greater than `out_len` means “retry with this much output space”; buffered input is preserved for that retry.

## Example: Raw Roundtrip

```c
#define FIO_DEFLATE
#include "fio-stl.h"

int roundtrip(const void *src, size_t src_len) {
  size_t cap = fio_deflate_compress_bound(src_len);
  uint8_t *compressed = malloc(cap);
  if (!compressed)
    return -1;

  size_t compressed_len = fio_deflate_compress(compressed, cap, src, src_len, 6);
  if (!compressed_len) {
    free(compressed);
    return -1;
  }

  size_t plain_len = fio_deflate_decompress(NULL, 0, compressed, compressed_len);
  uint8_t *plain = malloc(plain_len);
  if (!plain) {
    free(compressed);
    return -1;
  }

  size_t actual = fio_deflate_decompress(plain, plain_len, compressed, compressed_len);

  free(plain);
  free(compressed);
  return actual == src_len ? 0 : -1;
}
```

------------------------------------------------------------
