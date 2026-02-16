## Brotli Compression

```c
#define FIO_BROTLI
#include "fio-stl.h"
```

Provides Brotli compression and decompression per [RFC 7932](https://tools.ietf.org/html/rfc7932).

The compressor supports quality levels 1-6:

- **q1-q2**: greedy matching with a direct-mapped hash table. Fast, low compression.
- **q3-q4**: lazy matching with a direct-mapped hash table. Better compression at moderate cost.
- **q5**: hash chain (depth-16 ring buffer per bucket) + distance cache + score-based lazy matching + context modeling (2 literal Huffman trees, UTF-8/LSB6 mode detection) + static dictionary search (122KB dictionary, identity + uppercase_first transforms).
- **q6**: everything in q5 plus greedy literal block splitting (up to 32 block types, each with its own Huffman tree).

Data smaller than 8KB always uses the q4 path regardless of the requested quality level, because the hash chain's multi-entry buckets provide no benefit at low load factors and the context modeling / block splitting overhead exceeds any entropy savings.

The compressor emits a single meta-block with NPOSTFIX=0 and NDIRECT=0. Maximum input size per call is 16MB (clamped internally).

The decompressor is fully RFC 7932 compliant: two-level packed Huffman tables, context-dependent literal decoding (4 context modes), 122,784-byte static dictionary with 121 transforms, and a 64-bit branchless bit reader.

**Note**: incompressibility detection is built into q5+. After probing ~1024 positions, if the match rate is below 5%, the compressor falls back to a depth-1 fast mode that avoids expensive chain walks, distance cache lookups, and dictionary searches.

### Brotli API

#### `fio_brotli_compress`

```c
size_t fio_brotli_compress(void *out,
                           size_t out_len,
                           const void *in,
                           size_t in_len,
                           int quality);
```

Compresses `in_len` bytes from `in` into the buffer `out` using Brotli (RFC 7932).

The caller must provide an output buffer sized via `fio_brotli_compress_bound()`.

Empty input (`in == NULL` or `in_len == 0`) produces a valid 1-byte empty Brotli stream (`0x06`).

* `out` - destination buffer for compressed data.
* `out_len` - capacity of the destination buffer in bytes.
* `in` - source data to compress.
* `in_len` - length of source data in bytes (max 16MB; larger values are clamped).
* `quality` - compression quality level, 1-6. Values outside this range are clamped.

**Returns:** the number of compressed bytes written on success, or `0` on error (e.g., output buffer too small or `out` is NULL).

#### `fio_brotli_compress_bound`

```c
size_t fio_brotli_compress_bound(size_t in_len);
```

Returns a conservative upper bound on the compressed output size for `in_len` bytes of input.

The bound is computed as `in_len + (in_len >> 2) + 1024`, which accounts for Brotli framing overhead and the worst case where data is incompressible.

Use this to allocate the output buffer before calling `fio_brotli_compress()`.

#### `fio_brotli_decompress`

```c
size_t fio_brotli_decompress(void *out,
                             size_t out_len,
                             const void *in,
                             size_t in_len);
```

Decompresses Brotli-compressed data (RFC 7932).

The function has three operating modes depending on the output buffer:

1. **Normal decompression** (`out != NULL` and `out_len > 0`): decompresses into the provided buffer.
2. **Size query** (`out == NULL` or `out_len == 0`): scans meta-block headers and returns the total decompressed size without allocating or writing any output. Returns `0` if the data is corrupt.
3. **Buffer too small**: if the output buffer is insufficient during decompression, returns the **required** buffer size (a value `> out_len`). The caller can then reallocate and retry.

* `out` - destination buffer for decompressed data, or NULL for size query.
* `out_len` - capacity of the destination buffer in bytes, or 0 for size query.
* `in` - Brotli-compressed input data.
* `in_len` - length of compressed input in bytes.

**Returns:**

| Condition | Return value |
|-----------|-------------|
| Success | Decompressed byte count (`<= out_len`) |
| Buffer too small | Required buffer size (`> out_len`) |
| Corrupt / invalid data | `0` |
| Size query (`out == NULL` or `out_len == 0`) | Required size, or `0` if corrupt |

**Note**: the size query mode sums MLEN values from meta-block headers. It can only determine the total size when all remaining meta-blocks are either uncompressed or the last meta-block. If a non-last compressed meta-block is encountered during scanning, the function returns `0` (size cannot be determined without full decompression).

#### `fio_brotli_decompress_bound`

```c
size_t fio_brotli_decompress_bound(size_t in_len);
```

Returns a conservative upper bound on the decompressed size for `in_len` bytes of Brotli-compressed input.

Brotli's theoretical maximum expansion ratio is very high; this function uses a practical bound of `in_len * 1032 + 1024`, capped at 4GB for inputs larger than 1GB.

**Note**: prefer using the size query mode of `fio_brotli_decompress()` (pass `out = NULL`) to determine the actual decompressed size rather than relying on this worst-case bound.

### Example

```c
#define FIO_BROTLI
#include "fio-stl.h"

void brotli_roundtrip_example(void) {
  const char *input = "Hello, Brotli! This is a compression test.";
  size_t in_len = strlen(input);

  /* Compress */
  size_t comp_bound = fio_brotli_compress_bound(in_len);
  void *compressed = malloc(comp_bound);
  size_t comp_len =
      fio_brotli_compress(compressed, comp_bound, input, in_len, 5);
  if (!comp_len) {
    fprintf(stderr, "Compression failed\n");
    free(compressed);
    return;
  }

  /* Query decompressed size */
  size_t decomp_size =
      fio_brotli_decompress(NULL, 0, compressed, comp_len);

  /* Decompress */
  void *decompressed = malloc(decomp_size);
  size_t decomp_len =
      fio_brotli_decompress(decompressed, decomp_size, compressed, comp_len);

  printf("Original: %zu bytes, Compressed: %zu bytes, Decompressed: %zu bytes\n",
         in_len, comp_len, decomp_len);

  free(compressed);
  free(decompressed);
}
```

------------------------------------------------------------
