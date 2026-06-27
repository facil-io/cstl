# Brotli

```c
#define FIO_BROTLI
#include "fio-stl.h"
```

Brotli support for RFC 7932 data. The decoder handles the real format: Huffman tables, context modes, static dictionary, transforms, and the usual compressed-data acrobatics. The encoder provides practical quality levels 1–6.

## API

### `fio_brotli_decompress`

```c
SFUNC size_t fio_brotli_decompress(void *out,
                                   size_t out_len,
                                   const void *in,
                                   size_t in_len);
```

Decompresses Brotli data.

Return values:

| Condition | Return value |
| --- | --- |
| Success | Decompressed byte count, `<= out_len`. |
| Output too small | Required buffer size, `> out_len`. |
| Corrupt input | `0`. |
| Size query | Required size, or `0` if corrupt / not knowable by scan. |

Pass `out == NULL` or `out_len == 0` to query the decompressed size. The query path scans meta-block lengths without allocating.

### `fio_brotli_decompress_bound`

```c
FIO_IFUNC size_t fio_brotli_decompress_bound(size_t in_len);
```

Returns a conservative decompression bound. The practical formula is `in_len * 1032 + 1024`, capped at 4GB for very large inputs. Prefer the size-query mode of `fio_brotli_decompress` when possible.

### `fio_brotli_compress`

```c
SFUNC size_t fio_brotli_compress(void *out,
                                 size_t out_len,
                                 const void *in,
                                 size_t in_len,
                                 int quality);
```

Compresses data using Brotli. `quality` is clamped to 1–6:

- `1–2`: fast greedy matching.
- `3–4`: lazy matching.
- `5–6`: hash chain, context modeling, and static dictionary use.

Use an output buffer sized with `fio_brotli_compress_bound`. Returns compressed length on success, `0` on error.

### `fio_brotli_compress_bound`

```c
FIO_IFUNC size_t fio_brotli_compress_bound(size_t in_len);
```

Returns a generous compression bound: `in_len + (in_len >> 2) + 1024`.

## Example

```c
#define FIO_BROTLI
#include "fio-stl.h"

int roundtrip(const void *src, size_t src_len) {
  size_t cap = fio_brotli_compress_bound(src_len);
  uint8_t *compressed = malloc(cap);
  if (!compressed)
    return -1;

  size_t compressed_len = fio_brotli_compress(compressed, cap, src, src_len, 5);
  if (!compressed_len) {
    free(compressed);
    return -1;
  }

  size_t plain_len = fio_brotli_decompress(NULL, 0, compressed, compressed_len);
  uint8_t *plain = malloc(plain_len);
  if (!plain) {
    free(compressed);
    return -1;
  }

  size_t actual = fio_brotli_decompress(plain, plain_len, compressed, compressed_len);

  free(plain);
  free(compressed);
  return actual == src_len ? 0 : -1;
}
```

## Implementation Notes

The decompressor uses a 64-bit LSB-first bit reader, two-level packed Huffman tables with an 8-bit first level, the RFC 7932 static dictionary, 121 transforms, and context-dependent literal decoding.

The compressor emits Brotli streams with bounded memory use and quality-dependent matching. For tiny input, simpler paths may win because being clever has overhead too.

------------------------------------------------------------
