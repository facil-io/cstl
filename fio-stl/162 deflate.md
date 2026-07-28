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

Opaque streaming compression/decompression state (~32 bytes without
takeover). The streaming API supports two modes:

- **No-takeover (default, `fio_deflate_new`):** each flushed message is an
  independent deflate stream. Compressor scratch (hash + token buffers)
  comes from a contention-safe static slot pool (`FIO_STATIC_SAFE_ALLOC_DEF`)
  checked out per call, so persistent per-context state is ~0. This is the
  only mode the WebSocket layer negotiates (both `*_no_context_takeover`
  flags are always forced).
- **Context takeover (`fio_deflate_new_takeover`):** matches may reference
  the last 32KB of previous messages. The window (+ compressor hash) is
  allocated inside the context's own block — the documented per-context cost
  (~160KB compressor / ~32KB decompressor).

**Thread safety:** contexts are stateful and unsynchronized — use one
context per connection and serialize all `fio_deflate_push` calls per
context (one writer at a time). The static scratch pool is internally
synchronized; when every slot is momentarily busy, compression fails
gracefully (returns `0`).

**Overflow contract:** when the output buffer is too small, `fio_deflate_push`
(compression) and `fio_deflate_compress` return `0` — output is NEVER
silently truncated. Callers should treat `0` as "send uncompressed" or retry
with a correctly-sized buffer (`fio_deflate_compress_bound`, which is now
guaranteed sufficient at every level: the compressor falls back to stored
blocks whenever Huffman coding would expand, capping negative-gain output at
input + 5 bytes per 64KB block + a small header).

### `fio_deflate_new`

```c
SFUNC fio_deflate_s *fio_deflate_new(int level, int is_compress);
```

Creates a no-takeover streaming state. `is_compress != 0` creates a compressor; `0` creates a decompressor. Returns `NULL` on allocation failure. `level` is recorded for API compatibility; the streaming compressor always uses the fast greedy matcher (the one-shot `fio_deflate_compress` keeps levels 0-9).

### `fio_deflate_new_takeover`

```c
SFUNC fio_deflate_s *fio_deflate_new_takeover(int level, int is_compress);
```

Creates a streaming state with context takeover (cross-message history over the last 32KB). Allocates the window (+ compressor hash) inside the context's own block (~160KB compressor / ~32KB decompressor). `fio_deflate_destroy` resets the history (keeping the allocation).

### `fio_deflate_free`

```c
SFUNC void fio_deflate_free(fio_deflate_s *s);
```

Frees a streaming state.

### `fio_deflate_destroy`

```c
SFUNC void fio_deflate_destroy(fio_deflate_s *s);
```

Resets a streaming context. The input buffer is freed when it grew past 64KB, keeping persistent per-connection state bounded (no-takeover design).

### `fio_deflate_window_bits_set`

```c
SFUNC void fio_deflate_window_bits_set(fio_deflate_s *s, int bits);
```

Clamps compressor match distances to 2^`bits` (8..15, default 15). Used to honor `server_max_window_bits` from RFC 7692 negotiation. Decompression ignores it (any in-message distance up to 32KB is accepted).

### `fio_deflate_push`

```c
SFUNC size_t fio_deflate_push(fio_deflate_s *s,
                              void *out,
                              size_t out_len,
                              const void *in,
                              size_t in_len,
                              int flush);
```

Compresses or decompresses the next input chunk. Compression chunks input
into 32KB blocks (bounded scratch, no message-sized copies) and emits the
sync-flush trailer only on the message-final chunk.

- `flush == 0`: normal streaming (buffered up to 32KB, then auto-compressed).
- `flush == 1`: sync flush, useful at WebSocket frame boundaries.

For decompression, a return value greater than `out_len` means “retry with this much output space”; buffered input is preserved for that retry. Multi-block peer streams (e.g. zlib at any memLevel) inflate fully; the 9 completion bytes are appended internally.

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
