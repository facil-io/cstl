/* *****************************************************************************
DEFLATE / INFLATE Correctness Tests
Covers ./fio-stl/162 deflate.h.
***************************************************************************** */
/* Allocation-counting seam (T004/T007 white-box memory budgets).
 * Defining FIO_MEM_REALLOC / FIO_MEM_FREE before the first fio include routes
 * every STL allocation in this translation unit through the counters below
 * (`001 header.h` skips its defaults when both macros are already defined). */
#include <stdlib.h>

static void *fio___test_mem_realloc(void *ptr,
                                    size_t old_size,
                                    size_t new_size,
                                    size_t copy_len);
static void fio___test_mem_free(void *ptr, size_t size);
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  fio___test_mem_realloc((ptr), (old_size), (new_size), (copy_len))
#define FIO_MEM_FREE(ptr, size) fio___test_mem_free((ptr), (size))
#define FIO_MEM_REALLOC_IS_SAFE 0

#include "test-helpers.h"

#define FIO_DEFLATE
#include FIO_INCLUDE_FILE

#include <string.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

static int g_pass = 0;
static int g_fail = 0;

#define TEST_ASSERT(cond, ...)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "  FAIL: " __VA_ARGS__);                                 \
      fprintf(stderr, "\n");                                                   \
      ++g_fail;                                                                \
    } else {                                                                   \
      ++g_pass;                                                                \
    }                                                                          \
  } while (0)

FIO_SFUNC void fio___deflate_fill(uint8_t *buf, size_t len, uint32_t seed) {
  for (size_t i = 0; i < len; ++i) {
    seed = seed * 1103515245U + 12345U;
    buf[i] = (uint8_t)((seed >> 16) & 0xFFU);
  }
}

/* *****************************************************************************
Allocation counting (white-box memory budgets)
***************************************************************************** */

static struct {
  size_t live_allocs; /* live allocation count */
  size_t live_bytes;  /* bytes currently allocated */
  size_t peak_bytes;  /* high-water mark of live_bytes */
  size_t max_single;  /* largest single allocation seen while armed */
  size_t total_bytes; /* cumulative bytes allocated while armed */
  int armed;          /* non-zero => count */
} g_mem;

static void fio___test_mem_reset(void) {
  g_mem.live_allocs = 0;
  g_mem.live_bytes = 0;
  g_mem.peak_bytes = 0;
  g_mem.max_single = 0;
  g_mem.total_bytes = 0;
}

static void *fio___test_mem_realloc(void *ptr,
                                    size_t old_size,
                                    size_t new_size,
                                    size_t copy_len) {
  (void)copy_len;
  if (g_mem.armed) {
    if (!ptr) {
      if (new_size) {
        ++g_mem.live_allocs;
        g_mem.live_bytes += new_size;
        g_mem.total_bytes += new_size;
        if (new_size > g_mem.max_single)
          g_mem.max_single = new_size;
      }
    } else if (!new_size) {
      /* new_size == 0 acts like free */
      if (g_mem.live_allocs)
        --g_mem.live_allocs;
      g_mem.live_bytes =
          (old_size >= g_mem.live_bytes) ? 0 : (g_mem.live_bytes - old_size);
    } else {
      g_mem.live_bytes = (new_size >= old_size)
                             ? (g_mem.live_bytes + (new_size - old_size))
                             : (g_mem.live_bytes - (old_size - new_size));
      if (new_size > g_mem.max_single)
        g_mem.max_single = new_size;
    }
    if (g_mem.live_bytes > g_mem.peak_bytes)
      g_mem.peak_bytes = g_mem.live_bytes;
  }
  return realloc(ptr, new_size);
}

static void fio___test_mem_free(void *ptr, size_t size) {
  if (g_mem.armed && ptr) {
    if (g_mem.live_allocs)
      --g_mem.live_allocs;
    g_mem.live_bytes =
        (size >= g_mem.live_bytes) ? 0 : (g_mem.live_bytes - size);
  }
  free(ptr);
}

/** xorshift64 fill — cryptographically-ish random, incompressible. */
FIO_SFUNC void fio___deflate_fill_random(uint8_t *buf, size_t len, uint64_t s) {
  for (size_t i = 0; i < len; ++i) {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    buf[i] = (uint8_t)s;
  }
}

FIO_SFUNC void fio___deflate_make_text(uint8_t *buf, size_t len) {
  static const char phrase[] = "the quick brown fox jumps over the lazy dog; "
                               "facil.io deflate correctness vector; ";
  const size_t phrase_len = sizeof(phrase) - 1;
  for (size_t i = 0; i < len;) {
    size_t n = phrase_len;
    if (n > len - i)
      n = len - i;
    FIO_MEMCPY(buf + i, phrase, n);
    i += n;
  }
  for (size_t i = 64; i + 256 <= len; i += 2048)
    FIO_MEMCPY(buf + i, buf, 256);
}

static void test_known_stored_block(void) {
  fprintf(stderr, "Testing raw inflate known stored block...\n");
  static const uint8_t compressed[] =
      {0x01, 0x05, 0x00, 0xFA, 0xFF, 'H', 'e', 'l', 'l', 'o'};
  uint8_t out[16] = {0};

  size_t len =
      fio_deflate_decompress(out, sizeof(out), compressed, sizeof(compressed));
  TEST_ASSERT(len == 5, "stored block: expected 5 bytes, got %zu", len);
  TEST_ASSERT(len == 5 && !FIO_MEMCMP(out, "Hello", 5),
              "stored block: output mismatch");

  size_t required =
      fio_deflate_decompress(NULL, 0, compressed, sizeof(compressed));
  TEST_ASSERT(required == 5,
              "stored block counting mode: expected 5, got %zu",
              required);

  uint8_t small[4];
  required = fio_deflate_decompress(small,
                                    sizeof(small),
                                    compressed,
                                    sizeof(compressed));
  TEST_ASSERT(required == 5,
              "stored block small buffer: expected required size 5, got %zu",
              required);
}

static void test_raw_roundtrip_levels(void) {
  fprintf(stderr, "Testing raw deflate roundtrips across levels...\n");
  enum { DATA_LEN = 8192 };
  uint8_t data[DATA_LEN];
  uint8_t out[DATA_LEN + 64];
  fio___deflate_make_text(data, sizeof(data));

  size_t bound = fio_deflate_compress_bound(sizeof(data));
  uint8_t *compressed = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, bound, 0);
  TEST_ASSERT(compressed != NULL, "roundtrip: compressed allocation failed");
  if (!compressed)
    return;

  for (int level = 0; level <= 9; ++level) {
    FIO_MEMSET(out, 0xA5, sizeof(out));
    size_t clen =
        fio_deflate_compress(compressed, bound, data, sizeof(data), level);
    TEST_ASSERT(clen > 0 && clen <= bound,
                "level %d: compress returned %zu (bound %zu)",
                level,
                clen,
                bound);
    if (!clen || clen > bound)
      continue;

    size_t required = fio_deflate_decompress(NULL, 0, compressed, clen);
    TEST_ASSERT(required == sizeof(data),
                "level %d: counting mode expected %zu, got %zu",
                level,
                sizeof(data),
                required);

    size_t dlen = fio_deflate_decompress(out, sizeof(out), compressed, clen);
    TEST_ASSERT(dlen == sizeof(data),
                "level %d: expected %zu bytes, got %zu",
                level,
                sizeof(data),
                dlen);
    TEST_ASSERT(dlen == sizeof(data) && !FIO_MEMCMP(out, data, sizeof(data)),
                "level %d: roundtrip mismatch",
                level);
  }

  FIO_MEM_FREE(compressed, bound);
}

static void test_edges_and_errors(void) {
  fprintf(stderr, "Testing bounds, empty input, and corrupt inputs...\n");

  TEST_ASSERT(fio_deflate_compress_bound(0) >= 5,
              "compress_bound(0) should fit an empty stored block");
  TEST_ASSERT(fio_deflate_compress_bound(65535) >= 65540,
              "compress_bound(65535) should fit stored block overhead");
  TEST_ASSERT(fio_deflate_decompress_bound(1) >= 4096,
              "decompress_bound(1) should honor documented minimum");

  uint8_t empty_deflated[64];
  size_t empty_len =
      fio_deflate_compress(empty_deflated, sizeof(empty_deflated), "", 0, 6);
  TEST_ASSERT(empty_len == 5,
              "empty raw deflate: expected 5-byte stored block, got %zu",
              empty_len);
  TEST_ASSERT(fio_deflate_decompress(empty_deflated,
                                     sizeof(empty_deflated),
                                     empty_deflated,
                                     empty_len) == 0,
              "empty raw inflate should return zero output bytes");

  static const uint8_t invalid_btype[] = {0x07}; /* BFINAL=1, BTYPE=3 */
  TEST_ASSERT(fio_deflate_decompress(empty_deflated,
                                     sizeof(empty_deflated),
                                     invalid_btype,
                                     sizeof(invalid_btype)) == 0,
              "reserved BTYPE must be rejected");

  static const uint8_t truncated_stored[] = {0x01, 0x05, 0x00, 0xFA};
  TEST_ASSERT(fio_deflate_decompress(empty_deflated,
                                     sizeof(empty_deflated),
                                     truncated_stored,
                                     sizeof(truncated_stored)) == 0,
              "truncated stored block must be rejected");

  TEST_ASSERT(fio_deflate_compress(NULL, 10, "x", 1, 6) == 0,
              "raw deflate with NULL output must fail");
  TEST_ASSERT(
      fio_deflate_decompress(empty_deflated, sizeof(empty_deflated), NULL, 0) ==
          0,
      "raw inflate with NULL input must fail");
  TEST_ASSERT(fio_deflate_push(NULL,
                               empty_deflated,
                               sizeof(empty_deflated),
                               "x",
                               1,
                               1) == 0,
              "stream push with NULL state must fail");
  fio_deflate_free(NULL);
}

static void test_gzip_roundtrip_and_trailer(void) {
  fprintf(stderr, "Testing gzip wrapper roundtrips and trailer checks...\n");
  enum { DATA_LEN = 4096 };
  uint8_t data[DATA_LEN];
  uint8_t out[DATA_LEN + 64];
  fio___deflate_make_text(data, sizeof(data));

  size_t gz_bound = fio_deflate_compress_bound(sizeof(data)) + 18;
  uint8_t *gz = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, gz_bound, 0);
  TEST_ASSERT(gz != NULL, "gzip: allocation failed");
  if (!gz)
    return;

  size_t gz_len = fio_gzip_compress(gz, gz_bound, data, sizeof(data), 6);
  TEST_ASSERT(gz_len > 18 && gz_len <= gz_bound,
              "gzip: compress returned %zu (bound %zu)",
              gz_len,
              gz_bound);
  if (gz_len > 18 && gz_len <= gz_bound) {
    TEST_ASSERT(gz[0] == 0x1F && gz[1] == 0x8B && gz[2] == 0x08,
                "gzip: invalid header magic/method");

    size_t required = fio_gzip_decompress(NULL, 0, gz, gz_len);
    TEST_ASSERT(required == sizeof(data),
                "gzip: counting mode expected %zu, got %zu",
                sizeof(data),
                required);

    uint8_t small[32];
    required = fio_gzip_decompress(small, sizeof(small), gz, gz_len);
    TEST_ASSERT(required == sizeof(data),
                "gzip: small buffer expected required size %zu, got %zu",
                sizeof(data),
                required);

    size_t dlen = fio_gzip_decompress(out, sizeof(out), gz, gz_len);
    TEST_ASSERT(dlen == sizeof(data),
                "gzip: expected %zu bytes, got %zu",
                sizeof(data),
                dlen);
    TEST_ASSERT(dlen == sizeof(data) && !FIO_MEMCMP(out, data, sizeof(data)),
                "gzip: roundtrip mismatch");

    gz[gz_len - 8] ^= 0x01; /* corrupt CRC32 trailer */
    TEST_ASSERT(fio_gzip_decompress(out, sizeof(out), gz, gz_len) == 0,
                "gzip: corrupted CRC trailer must be rejected");
    gz[gz_len - 8] ^= 0x01;
    gz[gz_len - 1] ^= 0x01; /* corrupt ISIZE trailer */
    TEST_ASSERT(fio_gzip_decompress(out, sizeof(out), gz, gz_len) == 0,
                "gzip: corrupted ISIZE trailer must be rejected");
  }

  FIO_MEM_FREE(gz, gz_bound);
}

static void test_streaming_roundtrip(void) {
  fprintf(stderr, "Testing streaming WebSocket-style roundtrips...\n");
  static const char *messages[] = {
      "Hello WebSocket permessage-deflate!",
      "The quick brown fox jumps over the lazy dog. The quick brown fox jumps.",
      "Hello WebSocket permessage-deflate! repeated context data."};
  enum { MSG_COUNT = sizeof(messages) / sizeof(messages[0]) };
  uint8_t compressed[1024];
  uint8_t out[1024];

  fio_deflate_s *enc = fio_deflate_new(6, 1);
  fio_deflate_s *dec = fio_deflate_new(0, 0);
  TEST_ASSERT(enc != NULL, "stream: compressor allocation failed");
  TEST_ASSERT(dec != NULL, "stream: decompressor allocation failed");
  if (!enc || !dec) {
    fio_deflate_free(enc);
    fio_deflate_free(dec);
    return;
  }

  for (size_t i = 0; i < MSG_COUNT; ++i) {
    size_t msg_len = strlen(messages[i]);
    size_t clen = fio_deflate_push(enc,
                                   compressed,
                                   sizeof(compressed),
                                   messages[i],
                                   msg_len,
                                   1);
    TEST_ASSERT(clen > 4, "stream message %zu: compress returned %zu", i, clen);
    if (clen <= 4)
      continue;
    TEST_ASSERT(compressed[clen - 4] == 0x00 && compressed[clen - 3] == 0x00 &&
                    compressed[clen - 2] == 0xFF &&
                    compressed[clen - 1] == 0xFF,
                "stream message %zu: missing sync-flush trailer",
                i);

    size_t dlen =
        fio_deflate_push(dec, out, sizeof(out), compressed, clen - 4, 1);
    TEST_ASSERT(dlen == msg_len,
                "stream message %zu: expected %zu bytes, got %zu",
                i,
                msg_len,
                dlen);
    TEST_ASSERT(dlen == msg_len && !FIO_MEMCMP(out, messages[i], msg_len),
                "stream message %zu: data mismatch",
                i);
  }

  fio_deflate_destroy(enc);
  fio_deflate_destroy(dec);
  {
    static const char msg[] = "state reset after fio_deflate_destroy";
    size_t clen = fio_deflate_push(enc,
                                   compressed,
                                   sizeof(compressed),
                                   msg,
                                   sizeof(msg) - 1,
                                   1);
    size_t dlen = 0;
    if (clen > 4)
      dlen = fio_deflate_push(dec, out, sizeof(out), compressed, clen - 4, 1);
    TEST_ASSERT(clen > 4 && dlen == sizeof(msg) - 1 &&
                    !FIO_MEMCMP(out, msg, sizeof(msg) - 1),
                "stream reset after destroy should permit a fresh roundtrip");
  }

  fio_deflate_free(enc);
  fio_deflate_free(dec);
}

static void test_streaming_buffered_pushes(void) {
  fprintf(stderr, "Testing streaming buffered pushes before sync flush...\n");
  static const char *chunks[] = {
      "Hello, World! This is chunk one. ",
      "The quick brown fox jumps over the lazy dog. ",
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
      "Final chunk with some more data to compress efficiently."};
  enum { CHUNK_COUNT = sizeof(chunks) / sizeof(chunks[0]), REF_LEN = 174 };
  uint8_t compressed[512];
  uint8_t out[REF_LEN + 32];
  char reference[REF_LEN + 1] = {0};
  size_t offset = 0;

  for (size_t i = 0; i < CHUNK_COUNT; ++i) {
    size_t len = strlen(chunks[i]);
    FIO_MEMCPY(reference + offset, chunks[i], len);
    offset += len;
  }
  TEST_ASSERT(offset == REF_LEN,
              "buffered stream: expected reference length %u, got %zu",
              (unsigned)REF_LEN,
              offset);
  if (offset != REF_LEN)
    return;

  fio_deflate_s *enc = fio_deflate_new(6, 1);
  fio_deflate_s *dec = fio_deflate_new(0, 0);
  TEST_ASSERT(enc != NULL, "buffered stream: compressor allocation failed");
  TEST_ASSERT(dec != NULL, "buffered stream: decompressor allocation failed");
  if (!enc || !dec) {
    fio_deflate_free(enc);
    fio_deflate_free(dec);
    return;
  }

  for (size_t i = 0; i + 1 < CHUNK_COUNT; ++i) {
    size_t result = fio_deflate_push(enc,
                                     compressed,
                                     sizeof(compressed),
                                     chunks[i],
                                     strlen(chunks[i]),
                                     0);
    TEST_ASSERT(result == 0,
                "buffered stream chunk %zu: expected buffered result 0, got %zu",
                i,
                result);
  }

  size_t clen = fio_deflate_push(enc,
                                 compressed,
                                 sizeof(compressed),
                                 chunks[CHUNK_COUNT - 1],
                                 strlen(chunks[CHUNK_COUNT - 1]),
                                 1);
  TEST_ASSERT(clen > 4,
              "buffered stream final flush: expected compressed output, got %zu",
              clen);
  if (clen > 4) {
    size_t wire_len = clen - 4;
    size_t split = wire_len >> 1;
    size_t first = fio_deflate_push(dec,
                                    out,
                                    sizeof(out),
                                    compressed,
                                    split,
                                    0);
    TEST_ASSERT(first == 0,
                "buffered stream decode first half: expected buffered result 0, got %zu",
                first);

    size_t dlen = fio_deflate_push(dec,
                                   out,
                                   sizeof(out),
                                   compressed + split,
                                   wire_len - split,
                                   1);
    TEST_ASSERT(dlen == REF_LEN,
                "buffered stream final decode: expected %u bytes, got %zu",
                (unsigned)REF_LEN,
                dlen);
    TEST_ASSERT(dlen == REF_LEN && !FIO_MEMCMP(out, reference, REF_LEN),
                "buffered stream final decode: data mismatch");
  }

  fio_deflate_free(enc);
  fio_deflate_free(dec);
}

#ifdef HAVE_ZLIB
FIO_SFUNC size_t fio___test_zlib_raw_compress(void *out,
                                              size_t out_len,
                                              const void *in,
                                              size_t in_len,
                                              int level) {
  z_stream zs;
  FIO_MEMSET(&zs, 0, sizeof(zs));
  if (deflateInit2(&zs, level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    return 0;
  zs.next_in = (Bytef *)in;
  zs.avail_in = (uInt)in_len;
  zs.next_out = (Bytef *)out;
  zs.avail_out = (uInt)out_len;
  int rc = deflate(&zs, Z_FINISH);
  size_t result = (rc == Z_STREAM_END) ? (size_t)zs.total_out : 0;
  deflateEnd(&zs);
  return result;
}

FIO_SFUNC size_t fio___test_zlib_raw_decompress(void *out,
                                                size_t out_len,
                                                const void *in,
                                                size_t in_len) {
  z_stream zs;
  FIO_MEMSET(&zs, 0, sizeof(zs));
  if (inflateInit2(&zs, -15) != Z_OK)
    return 0;
  zs.next_in = (Bytef *)in;
  zs.avail_in = (uInt)in_len;
  zs.next_out = (Bytef *)out;
  zs.avail_out = (uInt)out_len;
  int rc = inflate(&zs, Z_FINISH);
  size_t result = (rc == Z_STREAM_END) ? (size_t)zs.total_out : 0;
  inflateEnd(&zs);
  return result;
}

FIO_SFUNC size_t fio___test_zlib_gzip_compress(void *out,
                                               size_t out_len,
                                               const void *in,
                                               size_t in_len,
                                               int level) {
  z_stream zs;
  FIO_MEMSET(&zs, 0, sizeof(zs));
  if (deflateInit2(&zs, level, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) !=
      Z_OK)
    return 0;
  zs.next_in = (Bytef *)in;
  zs.avail_in = (uInt)in_len;
  zs.next_out = (Bytef *)out;
  zs.avail_out = (uInt)out_len;
  int rc = deflate(&zs, Z_FINISH);
  size_t result = (rc == Z_STREAM_END) ? (size_t)zs.total_out : 0;
  deflateEnd(&zs);
  return result;
}

FIO_SFUNC size_t fio___test_zlib_gzip_decompress(void *out,
                                                 size_t out_len,
                                                 const void *in,
                                                 size_t in_len) {
  z_stream zs;
  FIO_MEMSET(&zs, 0, sizeof(zs));
  if (inflateInit2(&zs, 15 + 16) != Z_OK)
    return 0;
  zs.next_in = (Bytef *)in;
  zs.avail_in = (uInt)in_len;
  zs.next_out = (Bytef *)out;
  zs.avail_out = (uInt)out_len;
  int rc = inflate(&zs, Z_FINISH);
  size_t result = (rc == Z_STREAM_END) ? (size_t)zs.total_out : 0;
  inflateEnd(&zs);
  return result;
}

FIO_SFUNC size_t fio___test_zlib_sync_inflate(void *out,
                                              size_t out_len,
                                              const void *wire,
                                              size_t wire_len) {
  static const uint8_t trailer[4] = {0x00, 0x00, 0xFF, 0xFF};
  uint8_t input[2048];
  TEST_ASSERT(wire_len + sizeof(trailer) <= sizeof(input),
              "zlib sync inflate helper: wire payload too large");
  if (wire_len + sizeof(trailer) > sizeof(input))
    return 0;
  FIO_MEMCPY(input, wire, wire_len);
  FIO_MEMCPY(input + wire_len, trailer, sizeof(trailer));

  z_stream zs;
  FIO_MEMSET(&zs, 0, sizeof(zs));
  if (inflateInit2(&zs, -15) != Z_OK)
    return 0;
  zs.next_in = input;
  zs.avail_in = (uInt)(wire_len + sizeof(trailer));
  zs.next_out = (Bytef *)out;
  zs.avail_out = (uInt)out_len;
  int rc = inflate(&zs, Z_SYNC_FLUSH);
  size_t result = (rc == Z_OK || rc == Z_STREAM_END) ? (size_t)zs.total_out : 0;
  inflateEnd(&zs);
  return result;
}

static void test_zlib_raw_interop(void) {
  fprintf(stderr, "Testing raw DEFLATE interoperability with zlib...\n");
  const size_t data_lens[] = {0, 1, 257, 4096, 16384};
  const int levels[] = {0, 1, 6, 9};
  uint8_t data[16384];
  uint8_t fio_comp[fio_deflate_compress_bound(sizeof(data))];
  uint8_t z_comp[fio_deflate_compress_bound(sizeof(data)) + 64];
  uint8_t out[sizeof(data) + 64];

  for (size_t li = 0; li < sizeof(data_lens) / sizeof(data_lens[0]); ++li) {
    size_t data_len = data_lens[li];
    fio___deflate_fill(data, data_len, 0xC001D00DU + (uint32_t)data_len);
    if (data_len > 1024)
      fio___deflate_make_text(data + (data_len / 2), data_len / 2);

    for (size_t lv = 0; lv < sizeof(levels) / sizeof(levels[0]); ++lv) {
      int level = levels[lv];
      size_t bound = fio_deflate_compress_bound(data_len);

      size_t fio_len = fio_deflate_compress(fio_comp,
                                            sizeof(fio_comp),
                                            data,
                                            data_len,
                                            level);
      TEST_ASSERT(fio_len > 0 && fio_len <= bound,
                  "zlib raw: fio compress len=%zu for data=%zu level=%d",
                  fio_len,
                  data_len,
                  level);
      if (fio_len && level <= 1) {
        /* Validate fio's stored/fast raw output against independent zlib.
         * Higher fio compression levels are covered by internal roundtrips;
         * zlib-generated dynamic/fixed streams below validate fio's inflater.
         */
        size_t z_out =
            fio___test_zlib_raw_decompress(out, sizeof(out), fio_comp, fio_len);
        TEST_ASSERT(z_out == data_len,
                    "zlib raw: zlib inflate fio data expected %zu, got %zu",
                    data_len,
                    z_out);
        TEST_ASSERT(z_out == data_len && !FIO_MEMCMP(out, data, data_len),
                    "zlib raw: zlib inflate fio data mismatch");
      }

      size_t z_len = fio___test_zlib_raw_compress(z_comp,
                                                  sizeof(z_comp),
                                                  data,
                                                  data_len,
                                                  level);
      TEST_ASSERT(z_len > 0,
                  "zlib raw: zlib compress failed for data=%zu level=%d",
                  data_len,
                  level);
      if (z_len) {
        size_t fio_out =
            fio_deflate_decompress(out, sizeof(out), z_comp, z_len);
        TEST_ASSERT(fio_out == data_len,
                    "zlib raw: fio inflate zlib data expected %zu, got %zu",
                    data_len,
                    fio_out);
        TEST_ASSERT(fio_out == data_len && !FIO_MEMCMP(out, data, data_len),
                    "zlib raw: fio inflate zlib data mismatch");
      }
    }
  }
}

static void test_zlib_gzip_interop(void) {
  fprintf(stderr, "Testing gzip interoperability with zlib...\n");
  enum { DATA_LEN = 4096 };
  uint8_t data[DATA_LEN];
  uint8_t fio_gz[fio_deflate_compress_bound(DATA_LEN) + 18];
  uint8_t z_gz[sizeof(fio_gz) + 64];
  uint8_t out[DATA_LEN + 64];
  fio___deflate_make_text(data, sizeof(data));

  size_t fio_len =
      fio_gzip_compress(fio_gz, sizeof(fio_gz), data, sizeof(data), 6);
  TEST_ASSERT(fio_len > 18, "zlib gzip: fio gzip returned %zu", fio_len);
  if (fio_len > 18) {
    size_t z_out =
        fio___test_zlib_gzip_decompress(out, sizeof(out), fio_gz, fio_len);
    TEST_ASSERT(z_out == sizeof(data),
                "zlib gzip: zlib gunzip fio data expected %zu, got %zu",
                sizeof(data),
                z_out);
    TEST_ASSERT(z_out == sizeof(data) && !FIO_MEMCMP(out, data, sizeof(data)),
                "zlib gzip: zlib gunzip fio data mismatch");
  }

  size_t z_len =
      fio___test_zlib_gzip_compress(z_gz, sizeof(z_gz), data, sizeof(data), 6);
  TEST_ASSERT(z_len > 18, "zlib gzip: zlib gzip returned %zu", z_len);
  if (z_len > 18) {
    size_t fio_out = fio_gzip_decompress(out, sizeof(out), z_gz, z_len);
    TEST_ASSERT(fio_out == sizeof(data),
                "zlib gzip: fio gunzip zlib data expected %zu, got %zu",
                sizeof(data),
                fio_out);
    TEST_ASSERT(fio_out == sizeof(data) && !FIO_MEMCMP(out, data, sizeof(data)),
                "zlib gzip: fio gunzip zlib data mismatch");
  }
}

static void test_zlib_streaming_interop(void) {
  fprintf(stderr,
          "Testing streaming sync-flush interoperability with zlib...\n");
  static const char msg[] =
      "WebSocket permessage-deflate payload with repeated repeated repeated "
      "words for context takeover coverage.";
  uint8_t compressed[512];
  uint8_t out[512];
  fio_deflate_s *enc = fio_deflate_new(6, 1);
  TEST_ASSERT(enc != NULL, "zlib stream: compressor allocation failed");
  if (!enc)
    return;

  size_t clen = fio_deflate_push(enc,
                                 compressed,
                                 sizeof(compressed),
                                 msg,
                                 sizeof(msg) - 1,
                                 1);
  TEST_ASSERT(clen > 4, "zlib stream: fio_deflate_push returned %zu", clen);
  if (clen > 4) {
    size_t z_out =
        fio___test_zlib_sync_inflate(out, sizeof(out), compressed, clen - 4);
    TEST_ASSERT(z_out == sizeof(msg) - 1,
                "zlib stream: expected %zu bytes, got %zu",
                sizeof(msg) - 1,
                z_out);
    TEST_ASSERT(z_out == sizeof(msg) - 1 && !FIO_MEMCMP(out, msg, z_out),
                "zlib stream: zlib sync inflate mismatch");
  }

  fio_deflate_free(enc);
}
#endif /* HAVE_ZLIB */

/* *****************************************************************************
T001 — Multi-block deflate streams must inflate fully

RFC 7692 peers (e.g. zlib at memLevel 8) emit MULTIPLE deflate blocks per
message. The streaming inflater must process the whole stream, not stop after
the first block (regression: forced BFINAL on the first block truncated
multi-block messages).
***************************************************************************** */

static void test_stream_multiblock_inflate(void) {
  fprintf(stderr, "Testing multi-block stream inflation...\n");
  /* A single message containing TWO deflate blocks. Stored blocks are
   * byte-aligned, so the hand-crafted concatenation is bit-exact (verified
   * against zlib as the reference inflater — see the T009 critic notes).
   * The inflater must not stop after the first block. */
  static const uint8_t two_block_final[] = {
      0x00, 0x05, 0x00, 0xFA, 0xFF, 'H', 'e', 'l', 'l', 'o', /* blk1 BFINAL=0 */
      0x01, 0x05, 0x00, 0xFA, 0xFF, 'W', 'o', 'r', 'l', 'd'  /* blk2 BFINAL=1 */
  };
  /* Same two blocks, both non-final, followed by a stripped RFC 7692
   * sync-flush (the pending empty-stored-block header byte without its
   * 00 00 FF FF trailer) — the shape the WS receive path produces. */
  static const uint8_t two_block_sync[] = {
      0x00, 0x05, 0x00, 0xFA, 0xFF, 'H', 'e', 'l', 'l', 'o', /* blk1 BFINAL=0 */
      0x00, 0x05, 0x00, 0xFA, 0xFF, 'W', 'o', 'r', 'l', 'd', /* blk2 BFINAL=0 */
      0x00 /* pending empty stored block header (trailer stripped) */};
  uint8_t out[32];
  fio_deflate_s *dec = fio_deflate_new(0, 0);
  TEST_ASSERT(dec != NULL, "multiblock: decompressor allocation failed");
  if (!dec)
    return;

  size_t r = fio_deflate_push(dec,
                              out,
                              sizeof(out),
                              two_block_final,
                              sizeof(two_block_final),
                              1);
  TEST_ASSERT(r == 10,
              "multiblock (final): expected 10 inflated bytes, got %zu "
              "(stream truncated after first block)",
              r);
  TEST_ASSERT(r == 10 && !FIO_MEMCMP(out, "HelloWorld", 10),
              "multiblock (final): payload mismatch");

  fio_deflate_destroy(dec);
  FIO_MEMSET(out, 0, sizeof(out));
  r = fio_deflate_push(dec,
                       out,
                       sizeof(out),
                       two_block_sync,
                       sizeof(two_block_sync),
                       1);
  TEST_ASSERT(r == 10,
              "multiblock (sync-stripped): expected 10 inflated bytes, got "
              "%zu (stream truncated after first block)",
              r);
  TEST_ASSERT(r == 10 && !FIO_MEMCMP(out, "HelloWorld", 10),
              "multiblock (sync-stripped): payload mismatch");

  fio_deflate_free(dec);
}

#ifdef HAVE_ZLIB
static void test_zlib_multiblock_inflate(void) {
  fprintf(stderr,
          "Testing multi-block inflation of a real zlib peer stream...\n");
  /* zlib (memLevel 8) starts a new deflate block every ~16K tokens, so a
   * 200KB sync-flushed message is a multi-block stream — exactly what
   * browser WebSocket peers send. */
  const size_t msg_len = 200 * 1024;
  uint8_t *msg = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, msg_len + 64, 0);
  TEST_ASSERT(msg != NULL, "zlib multiblock: allocation failed");
  if (!msg)
    return;
  /* Pre-fill deterministically: the snprintf records below only cover
   * 26 of every 32 bytes — without a pre-fill the gaps (and the tail)
   * hold uninitialized heap data, making the compressed stream (and any
   * failure it triggers) sporadic and non-reproducible. */
  fio___deflate_fill(msg, msg_len + 64, 0x5AFE2026U);
  for (size_t i = 0; i < msg_len; i += 32) {
    int n = snprintf((char *)msg + i,
                     msg_len - i,
                     "{\"key%05u\":\"value%05u\"},",
                     (unsigned)i,
                     (unsigned)(i * 7));
    if (n < 0)
      break;
  }

  z_stream zs;
  FIO_MEMSET(&zs, 0, sizeof(zs));
  TEST_ASSERT(deflateInit2(&zs, 1, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) ==
                  Z_OK,
              "zlib multiblock: deflateInit2 failed");
  size_t comp_cap = deflateBound(&zs, (uLong)msg_len) + 16;
  uint8_t *comp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, comp_cap, 0);
  TEST_ASSERT(comp != NULL, "zlib multiblock: allocation failed");
  if (!comp) {
    deflateEnd(&zs);
    FIO_MEM_FREE(msg, msg_len + 64);
    return;
  }
  zs.next_in = msg;
  zs.avail_in = (uInt)msg_len;
  zs.next_out = comp;
  zs.avail_out = (uInt)comp_cap;
  int zr = deflate(&zs, Z_SYNC_FLUSH);
  TEST_ASSERT(zr == Z_OK, "zlib multiblock: deflate failed (%d)", zr);
  size_t comp_len = comp_cap - zs.avail_out;
  deflateEnd(&zs);
  TEST_ASSERT(comp_len > 4 && comp[comp_len - 4] == 0x00 &&
                  comp[comp_len - 3] == 0x00 && comp[comp_len - 2] == 0xFF &&
                  comp[comp_len - 1] == 0xFF,
              "zlib multiblock: expected sync-flush trailer");

  /* Inflate as ONE WebSocket message (trailer stripped). */
  fio_deflate_s *dec = fio_deflate_new(0, 0);
  TEST_ASSERT(dec != NULL, "zlib multiblock: decompressor allocation failed");
  size_t out_cap = msg_len + 64 * 1024;
  uint8_t *out = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, out_cap, 0);
  size_t r = 0;
  if (dec && out)
    r = fio_deflate_push(dec, out, out_cap, comp, comp_len - 4, 1);
  TEST_ASSERT(r == msg_len,
              "zlib multiblock: expected %zu inflated bytes, got %zu "
              "(multi-block stream truncated)",
              msg_len,
              r);
  TEST_ASSERT(r == msg_len && !FIO_MEMCMP(out, msg, msg_len),
              "zlib multiblock: payload mismatch");

  fio_deflate_free(dec);
  if (out)
    FIO_MEM_FREE(out, out_cap);
  FIO_MEM_FREE(comp, comp_cap);
  FIO_MEM_FREE(msg, msg_len + 64);
}
#endif /* HAVE_ZLIB */

/* *****************************************************************************
T001b — Decode table sizing, capacity guard, and heap growth

The inflater builds Huffman decode tables into fixed stack buffers; when a
table ever needs more entries than the stack budget, storage must EXPAND
through the heap — never hard-fail a stream, never overflow the stack, and
never leak. This white-box test drives the machinery directly (the internal
macros are file-local, so root widths/caps are spelled out as literals).
***************************************************************************** */

static void test_decode_table_heap_growth(void) {
  fprintf(stderr, "Testing decode-table sizing and heap growth path...\n");
  enum {
    LITLEN_ROOT_BITS = 11,
    LITLEN_ROOT_SIZE = 1 << 11,
    LITLEN_STACK_CAP = (1 << 11) + 512 /* LITLEN_MAX in 162 deflate.h */
  };

  uint8_t ll[288];
  fio___deflate_fixed_litlen_lens(ll);

  /* Sizing query touches no memory: fixed tree fits the 11-bit root. */
  uint32_t need = fio___deflate_build_decode_table(NULL, 0, ll, 288, 11, 1);
  TEST_ASSERT(need == (uint32_t)LITLEN_ROOT_SIZE,
              "table sizing: fixed litlen needs %u entries, expected %u",
              need,
              (unsigned)LITLEN_ROOT_SIZE);

  /* A deep but valid (Kraft-complete) code: one symbol each at lengths
   * 1..10 plus 32 symbols at length 15 ((1 - 2^-10) + 32×2^-15 == 1).
   * Canonically the 32 deep codes cover exactly two root prefixes, each
   * needing a 16-entry subtable: 2048 + 2×16 = 2080 entries. */
  uint8_t deep[286];
  FIO_MEMSET(deep, 0, sizeof(deep));
  for (uint32_t i = 0; i < 10; ++i)
    deep[i] = (uint8_t)(i + 1);
  for (uint32_t i = 10; i < 42; ++i)
    deep[i] = 15;
  need = fio___deflate_build_decode_table(NULL, 0, deep, 286, 11, 1);
  TEST_ASSERT(need == (uint32_t)LITLEN_ROOT_SIZE + 32,
              "table sizing: deep code needs %u entries, expected %u",
              need,
              (unsigned)LITLEN_ROOT_SIZE + 32);

  /* Capacity guard: an undersized caller buffer fails cleanly (no write). */
  uint32_t root_only[LITLEN_ROOT_SIZE];
  FIO_MEMSET(root_only, 0xAA, sizeof(root_only));
  uint32_t r =
      fio___deflate_build_decode_table(root_only, LITLEN_ROOT_SIZE, deep, 286, 11, 1);
  TEST_ASSERT(r == 0, "table guard: undersized buffer returned %u", r);

  /* Reference build into a correctly sized buffer. */
  uint32_t reference[LITLEN_STACK_CAP];
  uint32_t ref = fio___deflate_build_decode_table(reference,
                                                  LITLEN_STACK_CAP,
                                                  deep,
                                                  286,
                                                  11,
                                                  1);
  TEST_ASSERT(ref == need, "table build: used %u entries, sizing said %u", ref, need);

  /* Heap growth: a tiny "stack" buffer forces expansion through the heap.
   * The heap-built table must be byte-identical to the reference build. */
  fio___test_mem_reset();
  g_mem.armed = 1;
  uint32_t tiny[8];
  uint32_t *heap = NULL;
  size_t heap_cap = 0;
  uint32_t *t = fio___deflate_decode_table_build(tiny,
                                                 8,
                                                 &heap,
                                                 &heap_cap,
                                                 deep,
                                                 286,
                                                 11,
                                                 1);
  TEST_ASSERT(t != NULL && t == heap && heap_cap >= need,
              "table growth: expected heap table with cap >= %u (cap %zu)",
              need,
              heap_cap);
  TEST_ASSERT(t && !FIO_MEMCMP(t, reference, (size_t)need * sizeof(uint32_t)),
              "table growth: heap-built table differs from stack build");

  /* The grown buffer is reused (not re-allocated) when it already fits. */
  size_t cap_after_growth = heap_cap;
  uint32_t *t2 = fio___deflate_decode_table_build(tiny,
                                                  8,
                                                  &heap,
                                                  &heap_cap,
                                                  deep,
                                                  286,
                                                  11,
                                                  1);
  TEST_ASSERT(t2 == heap && heap_cap == cap_after_growth,
              "table growth: heap buffer not reused across calls");

  /* Cleanup releases the grown buffer — no leak (allocation counter). */
  {
    fio___deflate_decode_tables_s tb;
    tb.litlen_heap = heap;
    tb.litlen_heap_cap = heap_cap;
    tb.dist_heap = NULL;
    tb.dist_heap_cap = 0;
    fio___deflate_decode_tables_cleanup(&tb);
  }
  g_mem.armed = 0;
  TEST_ASSERT(g_mem.live_allocs == 0,
              "table growth: %zu live allocations after cleanup (leak)",
              g_mem.live_allocs);
}

/* *****************************************************************************
T001c — Big payload roundtrip

Loads ./fio-stl.h and ./fio-stl.md (the repo's two largest text artifacts)
TWICE each, joined by 64KB of incompressible random data: a ~12MB payload
mixing highly compressible and incompressible regions, forcing many deflate
blocks and deep Huffman trees at scale. Roundtrips through the one-shot API,
the streaming (WebSocket-shaped) API, and — when zlib is available — a real
zlib peer stream, mirroring the original sporadic-failure shape.
***************************************************************************** */

static uint8_t *fio___test_read_file(const char *path, size_t *len_out) {
  *len_out = 0;
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  if (fseek(f, 0, SEEK_END) == -1) {
    fclose(f);
    return NULL;
  }
  long sz = ftell(f);
  if (sz <= 0 || fseek(f, 0, SEEK_SET) == -1) {
    fclose(f);
    return NULL;
  }
  uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, (size_t)sz, 0);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
    FIO_MEM_FREE(buf, (size_t)sz);
    fclose(f);
    return NULL;
  }
  fclose(f);
  *len_out = (size_t)sz;
  return buf;
}

static void test_big_payload_roundtrip(void) {
  fprintf(stderr,
          "Testing big payload roundtrip (fio-stl.h + fio-stl.md, \u00d72 + "
          "random joints)...\n");
  size_t hlen = 0, mlen = 0;
  uint8_t *h = fio___test_read_file("./fio-stl.h", &hlen);
  uint8_t *m = fio___test_read_file("./fio-stl.md", &mlen);
  TEST_ASSERT(h != NULL && m != NULL,
              "big payload: failed to load ./fio-stl.h / ./fio-stl.md "
              "(tests must run from the project root)");
  if (!h || !m) {
    if (h)
      FIO_MEM_FREE(h, hlen);
    if (m)
      FIO_MEM_FREE(m, mlen);
    return;
  }

  const size_t joint = 64 * 1024;
  const size_t plen = 2 * hlen + 2 * mlen + 3 * joint;
  uint8_t *payload = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, plen, 0);
  size_t bound = fio_deflate_compress_bound(plen);
  uint8_t *comp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, bound, 0);
  uint8_t *out = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, plen, 0);
  TEST_ASSERT(payload && comp && out, "big payload: allocation failed");
  if (!payload || !comp || !out)
    goto done_files;

  /* Assemble: [h][rnd][md][rnd][h][rnd][md] */
  {
    uint8_t *w = payload;
    const uint8_t *parts[4] = {h, m, h, m};
    const size_t lens[4] = {hlen, mlen, hlen, mlen};
    for (int i = 0; i < 4; ++i) {
      FIO_MEMCPY(w, parts[i], lens[i]);
      w += lens[i];
      if (i < 3) {
        fio___deflate_fill_random(w, joint, 0xB16B00B5ULL + (uint64_t)i);
        w += joint;
      }
    }
    TEST_ASSERT((size_t)(w - payload) == plen, "big payload: assembly size error");
  }

  fio___test_mem_reset();
  g_mem.armed = 1;

  /* (a) One-shot, level 9 (deepest trees). */
  {
    size_t clen = fio_deflate_compress(comp, bound, payload, plen, 9);
    TEST_ASSERT(clen > 0 && clen <= bound,
                "big payload (one-shot): compress returned %zu", clen);
    size_t dlen = fio_deflate_decompress(out, plen, comp, clen);
    TEST_ASSERT(dlen == plen && !FIO_MEMCMP(out, payload, plen),
                "big payload (one-shot): roundtrip mismatch (%zu of %zu)",
                dlen,
                plen);
  }

  /* (b) Streaming (WebSocket-shaped: sync-flushed, trailer stripped). */
  {
    fio_deflate_s *enc = fio_deflate_new(6, 1);
    fio_deflate_s *dec = fio_deflate_new(0, 0);
    TEST_ASSERT(enc && dec, "big payload (stream): context allocation failed");
    size_t clen = 0, dlen = 0;
    if (enc && dec) {
      clen = fio_deflate_push(enc, comp, bound, payload, plen, 1);
      if (clen > 4)
        dlen = fio_deflate_push(dec, out, plen, comp, clen - 4, 1);
    }
    TEST_ASSERT(clen > 4, "big payload (stream): compress returned %zu", clen);
    TEST_ASSERT(dlen == plen && !FIO_MEMCMP(out, payload, plen),
                "big payload (stream): roundtrip mismatch (%zu of %zu)",
                dlen,
                plen);
    fio_deflate_free(enc);
    fio_deflate_free(dec);
  }

#ifdef HAVE_ZLIB
  /* (c) Real zlib peer stream at level 9 — the original sporadic-failure
   * shape, at ~60× the scale. */
  {
    z_stream zs;
    FIO_MEMSET(&zs, 0, sizeof(zs));
    TEST_ASSERT(deflateInit2(&zs, 9, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) ==
                    Z_OK,
                "big payload (zlib): deflateInit2 failed");
    zs.next_in = payload;
    zs.avail_in = (uInt)plen;
    zs.next_out = comp;
    zs.avail_out = (uInt)bound;
    int zr = deflate(&zs, Z_SYNC_FLUSH);
    TEST_ASSERT(zr == Z_OK, "big payload (zlib): deflate failed (%d)", zr);
    size_t clen = bound - zs.avail_out;
    deflateEnd(&zs);
    TEST_ASSERT(clen > 4 && comp[clen - 4] == 0x00 && comp[clen - 3] == 0x00 &&
                    comp[clen - 2] == 0xFF && comp[clen - 1] == 0xFF,
                "big payload (zlib): expected sync-flush trailer");
    fio_deflate_s *dec = fio_deflate_new(0, 0);
    size_t dlen = 0;
    if (dec)
      dlen = fio_deflate_push(dec, out, plen, comp, clen > 4 ? clen - 4 : 0, 1);
    TEST_ASSERT(dlen == plen && !FIO_MEMCMP(out, payload, plen),
                "big payload (zlib peer): roundtrip mismatch (%zu of %zu)",
                dlen,
                plen);
    fio_deflate_free(dec);
  }
#endif /* HAVE_ZLIB */

  g_mem.armed = 0;
  TEST_ASSERT(g_mem.live_allocs == 0,
              "big payload: %zu live allocations after test (leak)",
              g_mem.live_allocs);

done_files:
  if (payload)
    FIO_MEM_FREE(payload, plen);
  if (comp)
    FIO_MEM_FREE(comp, bound);
  if (out)
    FIO_MEM_FREE(out, plen);
  FIO_MEM_FREE(h, hlen);
  FIO_MEM_FREE(m, mlen);
}

/* *****************************************************************************
T002 — compress_bound must be sufficient at every level

For incompressible data the Huffman worst case (~9 bits/literal) exceeds the
stored-block estimate used by fio_deflate_compress_bound. A buffer of exactly
`bound` bytes MUST always yield a valid, exactly-roundtripping stream; an
undersized buffer MUST return 0 (never a silently truncated stream).
***************************************************************************** */

static void test_compress_bound_property(void) {
  fprintf(stderr,
          "Testing compress_bound sufficiency property (all levels)...\n");
  enum { DATA_LEN = 1048576 }; /* 1MB (pre-fix bound is undersized here) */
  uint8_t *data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, DATA_LEN, 0);
  size_t bound = fio_deflate_compress_bound(DATA_LEN);
  uint8_t *comp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, bound, 0);
  uint8_t *out = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, DATA_LEN, 0);
  TEST_ASSERT(data && comp && out, "bound property: allocation failed");
  if (!data || !comp || !out)
    goto done;
  fio___deflate_fill_random(data, DATA_LEN, 0x123456789ABCDEF0ULL);

  for (int level = 0; level <= 9; ++level) {
    FIO_MEMSET(comp, 0xAA, bound);
    size_t clen = fio_deflate_compress(comp, bound, data, DATA_LEN, level);
    TEST_ASSERT(clen > 0 && clen <= bound,
                "level %d: compress into bound-sized buffer returned %zu "
                "(bound %zu)",
                level,
                clen,
                bound);
    if (!clen || clen > bound)
      continue;
    size_t dlen = fio_deflate_decompress(out, DATA_LEN, comp, clen);
    TEST_ASSERT(dlen == DATA_LEN && !FIO_MEMCMP(out, data, DATA_LEN),
                "level %d: bound-sized buffer must yield an exact roundtrip "
                "(got %zu bytes — silent truncation/corruption)",
                level,
                dlen);
  }

  /* Small sizes (stored-block boundaries) must also roundtrip exactly. */
  {
    static const size_t lens[] = {1, 63, 4096, 65535, 65536, 131073};
    static const int levels[] = {0, 1, 6, 9};
    for (size_t li = 0; li < sizeof(lens) / sizeof(lens[0]); ++li) {
      size_t n = lens[li];
      size_t b = fio_deflate_compress_bound(n);
      for (size_t vi = 0; vi < sizeof(levels) / sizeof(levels[0]); ++vi) {
        int level = levels[vi];
        size_t clen = fio_deflate_compress(comp, b < bound ? b : bound,
                                           data, n, level);
        TEST_ASSERT(clen > 0 && clen <= b,
                    "len %zu level %d: compress returned %zu (bound %zu)",
                    n,
                    level,
                    clen,
                    b);
        if (!clen || clen > b)
          continue;
        size_t dlen = fio_deflate_decompress(out, n, comp, clen);
        TEST_ASSERT(dlen == n && !FIO_MEMCMP(out, data, n),
                    "len %zu level %d: roundtrip mismatch (got %zu)",
                    n,
                    level,
                    dlen);
      }
    }
  }

  /* Deliberately undersized output buffer: MUST return 0 (no silent
   * truncation, no bitwriter UB). */
  {
    size_t clen =
        fio_deflate_compress(comp, bound >> 2, data, DATA_LEN, 6);
    TEST_ASSERT(clen == 0,
                "undersized buffer: compress must return 0, got %zu "
                "(silently truncated stream)",
                clen);
  }

  /* Streaming push with undersized buffer: same contract. */
  {
    fio_deflate_s *enc = fio_deflate_new(6, 1);
    TEST_ASSERT(enc != NULL, "bound property: compressor allocation failed");
    if (enc) {
      uint8_t small[1024];
      size_t clen = fio_deflate_push(enc, small, sizeof(small),
                                     data, DATA_LEN, 1);
      TEST_ASSERT(clen == 0,
                  "undersized stream buffer: push must return 0, got %zu "
                  "(silently truncated stream)",
                  clen);
      fio_deflate_free(enc);
    }
  }

done:
  if (out)
    FIO_MEM_FREE(out, DATA_LEN);
  if (comp)
    FIO_MEM_FREE(comp, bound);
  if (data)
    FIO_MEM_FREE(data, DATA_LEN);
}

/* *****************************************************************************
T003 — Negative-gain inputs must fall back to stored blocks

Incompressible data must expand by at most the stored-block overhead
(+5 bytes per 64KB block), never by the Huffman worst case.
***************************************************************************** */

static void test_stored_fallback_cap(void) {
  fprintf(stderr, "Testing stored-block fallback expansion cap...\n");
  enum { DATA_LEN = 262144 }; /* 256KB */
  const size_t stored_cap =
      DATA_LEN + 5 * ((DATA_LEN + 65534) / 65535) + 64;
  uint8_t *data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, DATA_LEN, 0);
  size_t bound = fio_deflate_compress_bound(DATA_LEN);
  uint8_t *comp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, bound, 0);
  uint8_t *out = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, DATA_LEN, 0);
  TEST_ASSERT(data && comp && out, "stored cap: allocation failed");
  if (!data || !comp || !out)
    goto done;
  fio___deflate_fill_random(data, DATA_LEN, 0x0F1E2D3C4B5A6978ULL);

  for (int level = 1; level <= 9; ++level) {
    size_t clen = fio_deflate_compress(comp, bound, data, DATA_LEN, level);
    TEST_ASSERT(clen > 0 && clen <= stored_cap,
                "level %d: incompressible output %zu exceeds stored cap %zu "
                "(no stored-block fallback)",
                level,
                clen,
                stored_cap);
    if (!clen || clen > bound)
      continue;
    size_t dlen = fio_deflate_decompress(out, DATA_LEN, comp, clen);
    TEST_ASSERT(dlen == DATA_LEN && !FIO_MEMCMP(out, data, DATA_LEN),
                "level %d: stored-fallback roundtrip mismatch (got %zu)",
                level,
                dlen);
  }

done:
  if (out)
    FIO_MEM_FREE(out, DATA_LEN);
  if (comp)
    FIO_MEM_FREE(comp, bound);
  if (data)
    FIO_MEM_FREE(data, DATA_LEN);
}

/* *****************************************************************************
T004 — Streaming compression must use bounded (chunk-sized) scratch

Compressing a huge message must not allocate message-sized scratch on the
heap: the compressor chunks its input and big scratch lives in static
round-robin pools (invisible to this counter). A 1MB message compressed in a
single flush push must not produce any single allocation above the chunk
budget (or grow the live heap beyond it).
***************************************************************************** */

static void test_stream_bounded_scratch(void) {
  fprintf(stderr,
          "Testing streaming compression scratch bounds (1MB message)...\n");
  enum { MSG_LEN = 1u << 20, CHUNK_BUDGET = 256 * 1024 };
  uint8_t *msg = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, MSG_LEN, 0);
  size_t out_cap = fio_deflate_compress_bound(MSG_LEN) + 64;
  uint8_t *out = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, out_cap, 0);
  uint8_t *back = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, MSG_LEN + 64, 0);
  TEST_ASSERT(msg && out && back, "bounded scratch: allocation failed");
  if (!msg || !out || !back)
    goto done;
  fio___deflate_make_text(msg, MSG_LEN);

  /* Warm up lazy global state so only per-call scratch is measured. */
  {
    fio_deflate_s *warm = fio_deflate_new(1, 1);
    size_t wlen =
        fio_deflate_push(warm, out, out_cap, msg, 4096, 1);
    (void)wlen;
    fio_deflate_free(warm);
  }

  size_t clen = 0, dlen = 0;
  fio_deflate_s *enc = NULL, *dec = NULL;
  fio___test_mem_reset();
  g_mem.armed = 1;
  enc = fio_deflate_new(1, 1);
  dec = fio_deflate_new(0, 0);
  if (enc && dec) {
    clen = fio_deflate_push(enc, out, out_cap, msg, MSG_LEN, 1);
    if (clen > 4)
      dlen = fio_deflate_push(dec, back, MSG_LEN + 64, out, clen - 4, 1);
  }
  g_mem.armed = 0;

  TEST_ASSERT(enc && dec, "bounded scratch: context allocation failed");
  TEST_ASSERT(clen > 4,
              "bounded scratch: compress returned %zu", clen);
  TEST_ASSERT(dlen == MSG_LEN && !FIO_MEMCMP(back, msg, MSG_LEN),
              "bounded scratch: roundtrip mismatch (got %zu of %u)",
              dlen,
              (unsigned)MSG_LEN);
  TEST_ASSERT(g_mem.max_single <= CHUNK_BUDGET,
              "bounded scratch: single heap allocation of %zu bytes during "
              "1MB flush push (budget %u; scratch must be chunk-sized static "
              "pool slots, never message-sized)",
              g_mem.max_single,
              (unsigned)CHUNK_BUDGET);
  TEST_ASSERT(g_mem.peak_bytes <= CHUNK_BUDGET,
              "bounded scratch: live heap peaked at %zu bytes during 1MB "
              "flush push (budget %u)",
              g_mem.peak_bytes,
              (unsigned)CHUNK_BUDGET);

  fio_deflate_free(enc);
  fio_deflate_free(dec);

done:
  if (back)
    FIO_MEM_FREE(back, MSG_LEN + 64);
  if (out)
    FIO_MEM_FREE(out, out_cap);
  if (msg)
    FIO_MEM_FREE(msg, MSG_LEN);
}

/* *****************************************************************************
T007 — Persistent per-connection compression state ≈ 0

The no-takeover design forbids persistent compression state per WebSocket
connection: no inline 32KB window in the context struct, no eager hash-table
allocation at context creation. Budget: 128KB MAX per connection (both
contexts), ≤64KB target, ~0 stretch goal.
***************************************************************************** */

static void test_ws_memory_budget(void) {
  fprintf(stderr,
          "Testing persistent per-connection compression state budget...\n");

  /* (1) The context struct must not embed a 32KB sliding window. */
  TEST_ASSERT(sizeof(fio_deflate_s) <= 1024,
              "fio_deflate_s embeds %zu bytes — the 32KB window must move "
              "out of the struct into per-call scratch",
              sizeof(fio_deflate_s));

  /* (2) Context creation must not eagerly allocate hash/window state. */
  {
    fio_deflate_s *warm = fio_deflate_new(1, 1);
    fio_deflate_free(warm);
  }
  fio_deflate_s *enc = NULL, *dec = NULL;
  fio___test_mem_reset();
  g_mem.armed = 1;
  enc = fio_deflate_new(1, 1); /* server writes */
  dec = fio_deflate_new(0, 0); /* peer data */
  g_mem.armed = 0;
  TEST_ASSERT(enc != NULL && dec != NULL,
              "memory budget: context allocation failed");

  /* Stretch target: a small struct + bounded input buffer only. */
  TEST_ASSERT(g_mem.peak_bytes <= 16 * 1024,
              "persistent compression state is %zu bytes at context "
              "creation (target ≈ 0; eager hash tables / windows must move "
              "to per-call static pools)",
              g_mem.peak_bytes);
  /* Hard outer bound mandated for each WebSocket connection. */
  TEST_ASSERT(g_mem.peak_bytes <= 128 * 1024,
              "persistent compression state %zu bytes exceeds the 128KB "
              "per-connection outer bound",
              g_mem.peak_bytes);

  fio_deflate_free(enc);
  fio_deflate_free(dec);
}

/* *****************************************************************************
Takeover mode (generic streaming): cross-message history
***************************************************************************** */

static void test_stream_takeover_mode(void) {
  fprintf(stderr,
          "Testing context-takeover streaming (cross-message history)...\n");
  /* Three messages: m2 is nearly identical to m1 (cross-message match
   * magnet), m3 is fresh content. */
  enum { M1_LEN = 2048 };
  uint8_t m1[M1_LEN], m2[M1_LEN + 32];
  static const char m3[] = "completely different payload, no shared history";
  fio___deflate_make_text(m1, sizeof(m1));
  FIO_MEMCPY(m2, m1, M1_LEN);
  FIO_MEMCPY(m2 + M1_LEN, "!!", 2);
  size_t m2_len = M1_LEN + 2;

  uint8_t comp[M1_LEN * 2 + 256];
  uint8_t out[M1_LEN * 2 + 256];

  fio_deflate_s *enc = fio_deflate_new_takeover(6, 1);
  fio_deflate_s *dec = fio_deflate_new_takeover(0, 0);
  TEST_ASSERT(enc != NULL && dec != NULL,
              "takeover: context allocation failed");
  if (!enc || !dec) {
    fio_deflate_free(enc);
    fio_deflate_free(dec);
    return;
  }

  /* Roundtrip m1, then m2 (which should lean on m1's history). */
  size_t c1 = fio_deflate_push(enc, comp, sizeof(comp), m1, M1_LEN, 1);
  TEST_ASSERT(c1 > 4, "takeover: m1 compress returned %zu", c1);
  size_t d1 = c1 > 4 ? fio_deflate_push(dec, out, sizeof(out), comp, c1 - 4, 1)
                     : 0;
  TEST_ASSERT(d1 == M1_LEN && !FIO_MEMCMP(out, m1, M1_LEN),
              "takeover: m1 roundtrip mismatch (%zu of %u)",
              d1,
              (unsigned)M1_LEN);

  size_t c2 = fio_deflate_push(enc, comp, sizeof(comp), m2, m2_len, 1);
  TEST_ASSERT(c2 > 4, "takeover: m2 compress returned %zu", c2);
  size_t d2 = c2 > 4 ? fio_deflate_push(dec, out, sizeof(out), comp, c2 - 4, 1)
                     : 0;
  TEST_ASSERT(d2 == m2_len && !FIO_MEMCMP(out, m2, m2_len),
              "takeover: m2 roundtrip mismatch (%zu of %zu)",
              d2,
              m2_len);

  /* The cross-message advantage must be decisive: m2 ≈ m1+2B, so with
   * takeover m2 compresses to a small fraction of m1's stream. */
  TEST_ASSERT(c2 < (c1 / 2),
              "takeover: m2 (%zu) should be << m1 (%zu) via cross-message "
              "back-references",
              c2,
              c1);

  /* m3 (fresh content) still roundtrips on the same contexts. */
  size_t c3 = fio_deflate_push(enc, comp, sizeof(comp), m3, sizeof(m3) - 1, 1);
  size_t d3 = c3 > 4 ? fio_deflate_push(dec, out, sizeof(out), comp, c3 - 4, 1)
                     : 0;
  TEST_ASSERT(d3 == sizeof(m3) - 1 && !FIO_MEMCMP(out, m3, sizeof(m3) - 1),
              "takeover: m3 roundtrip mismatch");

  /* destroy resets cross-message history: m2 again now stands alone. */
  fio_deflate_destroy(enc);
  fio_deflate_destroy(dec);
  size_t c4 = fio_deflate_push(enc, comp, sizeof(comp), m2, m2_len, 1);
  size_t d4 = c4 > 4 ? fio_deflate_push(dec, out, sizeof(out), comp, c4 - 4, 1)
                     : 0;
  TEST_ASSERT(d4 == m2_len && !FIO_MEMCMP(out, m2, m2_len),
              "takeover: post-destroy roundtrip mismatch");
  TEST_ASSERT(c4 > c2,
              "takeover: post-destroy m2 (%zu) must lose cross-message "
              "history (was %zu)",
              c4,
              c2);

  fio_deflate_free(enc);
  fio_deflate_free(dec);
}

#ifdef HAVE_ZLIB
static void test_zlib_takeover_interop(void) {
  fprintf(stderr,
          "Testing takeover interop against a persistent zlib inflater...\n");
  /* A persistent raw-inflate z_stream (like a real RFC 7692 peer with
   * context takeover) must resolve our cross-message back-references. */
  enum { M1_LEN = 4096 };
  uint8_t m1[M1_LEN], m2[M1_LEN + 8];
  fio___deflate_make_text(m1, sizeof(m1));
  FIO_MEMCPY(m2, m1, M1_LEN);
  FIO_MEMCPY(m2 + M1_LEN, "the end", 7);
  size_t m2_len = M1_LEN + 7;
  uint8_t comp[M1_LEN * 2 + 256];
  uint8_t out[M1_LEN * 2 + 256];

  fio_deflate_s *enc = fio_deflate_new_takeover(6, 1);
  TEST_ASSERT(enc != NULL, "takeover interop: compressor allocation failed");
  if (!enc)
    return;
  size_t c1 = fio_deflate_push(enc, comp, sizeof(comp), m1, M1_LEN, 1);
  size_t c2 = fio_deflate_push(enc, comp + c1, sizeof(comp) - c1, m2, m2_len, 1);
  TEST_ASSERT(c1 > 4 && c2 > 4,
              "takeover interop: compress failed (%zu, %zu)",
              c1,
              c2);

  /* The receiver's view per RFC 7692: each message's wire payload gets the
   * 4-byte sync trailer re-appended (our pushes already include it) and is
   * inflated on ONE persistent inflater whose window carries m1's history. */
  z_stream zs;
  FIO_MEMSET(&zs, 0, sizeof(zs));
  TEST_ASSERT(inflateInit2(&zs, -15) == Z_OK,
              "takeover interop: inflateInit2 failed");

  zs.next_in = comp;
  zs.avail_in = (uInt)c1;
  zs.next_out = out;
  zs.avail_out = (uInt)sizeof(out);
  int rc = inflate(&zs, Z_SYNC_FLUSH);
  size_t total = (rc == Z_OK || rc == Z_STREAM_END) ? (size_t)zs.total_out : 0;
  TEST_ASSERT(total == M1_LEN && !FIO_MEMCMP(out, m1, M1_LEN),
              "takeover interop: zlib m1 inflated %zu, expected %u",
              total,
              (unsigned)M1_LEN);

  zs.next_in = comp + c1;
  zs.avail_in = (uInt)c2;
  int rc2 = inflate(&zs, Z_SYNC_FLUSH);
  size_t total2 =
      (rc2 == Z_OK || rc2 == Z_STREAM_END) ? (size_t)zs.total_out : 0;
  inflateEnd(&zs);
  size_t expect = M1_LEN + m2_len;
  TEST_ASSERT(rc == Z_OK && rc2 == Z_OK && total2 == expect,
              "takeover interop: zlib m2 total %zu, expected %zu (rc %d/%d)",
              total2,
              expect,
              rc,
              rc2);
  TEST_ASSERT(total2 == expect && !FIO_MEMCMP(out, m1, M1_LEN) &&
                  !FIO_MEMCMP(out + M1_LEN, m2, m2_len),
              "takeover interop: payload mismatch");
  fio_deflate_free(enc);
}
#endif /* HAVE_ZLIB */

/** Skewed-distribution text fill: random picks from a small vocabulary make
 * token frequencies skewed enough to push Huffman trees past the 15-bit
 * DEFLATE limit at scale (regression trigger for the corrupt-stream bug). */
FIO_SFUNC void fio___deflate_fill_skewed_text(uint8_t *buf,
                                              size_t len,
                                              uint32_t seed) {
  static const char words[] =
      "the quick brown fox jumps over the lazy dog and runs through "
      "compression tests headers content-type text/plain accept-encoding "
      "gzip deflate brotli vary cache-control response body ";
  const size_t wl = sizeof(words) - 1;
  for (size_t i = 0; i < len; ++i) {
    seed = seed * 1103515245U + 12345U;
    buf[i] = (uint8_t)words[(seed >> 16) % wl];
  }
}

/** Regression: on-the-fly gzip used to emit corrupt deflate streams for
 * large compressible inputs (skewed frequencies drove the Huffman length
 * limiter into an over/under-subscribed code set, which zlib rejects with
 * "invalid distances set" and which spun fio's own inflater forever).
 * Every level must produce streams that both fio and zlib can inflate. */
static void test_large_skewed_roundtrips(void) {
  fprintf(stderr,
          "Testing large skewed-input roundtrips (deep Huffman trees)...\n");
  const size_t sizes[] = {400 * 1024, 1024 * 1024};
  const size_t max_size = 1024 * 1024;
  uint8_t *data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, max_size, 0);
  uint8_t *out = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, max_size + 64, 0);
  size_t bound = fio_deflate_compress_bound(max_size) + 18;
  uint8_t *gz = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, bound, 0);
  TEST_ASSERT(data && out && gz, "large skewed: allocation failed");
  if (!data || !out || !gz)
    goto done;
  for (size_t si = 0; si < sizeof(sizes) / sizeof(sizes[0]); ++si) {
    size_t n = sizes[si];
    for (int mixed = 0; mixed < 2; ++mixed) {
      fio___deflate_fill_skewed_text(data, n, 42U + (uint32_t)n);
      if (mixed) /* half text, half incompressible */
        fio___deflate_fill_random(data + (n / 2), n - (n / 2), 0xF00DU + n);
      for (int level = 0; level <= 9; ++level) {
        size_t gz_len = fio_gzip_compress(gz, bound, data, n, level);
        TEST_ASSERT(gz_len > 18 && gz_len <= bound,
                    "large skewed: n=%zu mixed=%d level=%d gzip len %zu",
                    n,
                    mixed,
                    level,
                    gz_len);
        if (gz_len <= 18 || gz_len > bound)
          continue;
        size_t dlen = fio_gzip_decompress(out, max_size + 64, gz, gz_len);
        TEST_ASSERT(dlen == n && !FIO_MEMCMP(out, data, n),
                    "large skewed: fio roundtrip n=%zu mixed=%d level=%d "
                    "got %zu",
                    n,
                    mixed,
                    level,
                    dlen);
#ifdef HAVE_ZLIB
        /* The reported failure mode: third-party decoders must accept the
         * stream ("on-the-fly gzip emitted corrupt deflate streams"). */
        FIO_MEMSET(out, 0xA5, n);
        size_t zlen = fio___test_zlib_gzip_decompress(out, n, gz, gz_len);
        TEST_ASSERT(zlen == n && !FIO_MEMCMP(out, data, n),
                    "large skewed: ZLIB rejected fio gzip n=%zu mixed=%d "
                    "level=%d (gz_len %zu, inflated %zu)",
                    n,
                    mixed,
                    level,
                    gz_len,
                    zlen);
#endif
      }
    }
  }
done:
  if (data)
    FIO_MEM_FREE(data, max_size);
  if (out)
    FIO_MEM_FREE(out, max_size + 64);
  if (gz)
    FIO_MEM_FREE(gz, bound);
}

/** Regression: decoder table validation. Hand-crafted dynamic-block streams
 * (bit-exact fixtures) covering the incomplete/over-subscribed matrix.
 * Pre-fix, fio accepted incomplete tables and could spin forever decoding
 * the zero-filled table holes (0-bit code lengths = no bit consumption). */
static void test_invalid_huffman_tables(void) {
  fprintf(stderr, "Testing invalid / edge-case Huffman table rejection...\n");
  uint8_t out[64];

  /* A: incomplete distance table (lens 2+3, Kraft 3/8, max>1) — reject. */
  static const uint8_t s_incomplete_dist[] = {
      0x05, 0xC1, 0x81, 0x09, 0x00, 0x00, 0x00,
      0x83, 0x20, 0xFF, 0xBF, 0xDA, 0x26};
  TEST_ASSERT(fio_deflate_decompress(out,
                                     sizeof(out),
                                     s_incomplete_dist,
                                     sizeof(s_incomplete_dist)) == 0,
              "incomplete distance table (max>1) must be rejected");

  /* B: all-zero distance table = "no distance codes used" (RFC 1951
   * §3.2.7); an all-literals block. Must decode to "x" (zlib agrees). */
  static const uint8_t s_no_dist_codes[] = {
      0x05, 0xC0, 0x81, 0x09, 0x00, 0x00, 0x00,
      0x83, 0xA0, 0xB7, 0x3D, 0x5F, 0x04};
  size_t r = fio_deflate_decompress(out,
                                    sizeof(out),
                                    s_no_dist_codes,
                                    sizeof(s_no_dist_codes));
  TEST_ASSERT(r == 1 && out[0] == 'x',
              "all-zero distance table must decode as all-literals "
              "(expected 'x', got len %zu)",
              r);

  /* C: incomplete literal/length table (lens 2+3, max>1) — reject. */
  static const uint8_t s_incomplete_litlen[] = {
      0x05, 0xC1, 0x81, 0x09, 0x00, 0x00, 0x00,
      0x83, 0xA0, 0xFE, 0xBF, 0x7A, 0x0A};
  TEST_ASSERT(fio_deflate_decompress(out,
                                     sizeof(out),
                                     s_incomplete_litlen,
                                     sizeof(s_incomplete_litlen)) == 0,
              "incomplete litlen table (max>1) must be rejected");

  /* D: over-subscribed distance table (lens 1+1+1) — reject. */
  static const uint8_t s_oversub_dist[] = {
      0x05, 0xC2, 0x81, 0x09, 0x00, 0x00, 0x00,
      0x83, 0x20, 0xFF, 0xBF, 0x5A, 0x55};
  TEST_ASSERT(fio_deflate_decompress(out,
                                     sizeof(out),
                                     s_oversub_dist,
                                     sizeof(s_oversub_dist)) == 0,
              "over-subscribed distance table must be rejected");

  /* E: legal lone 1-bit distance code, but the stream references the
   * unused bit pattern (a table hole). Must be rejected, not decoded as
   * a 0-bit-length literal-0 spin (the pre-fix infinite loop). */
  static const uint8_t s_dist_hole[] = {
      0x0D, 0xC0, 0x81, 0x09, 0x00, 0x00, 0x00, 0x83,
      0xA0, 0xDB, 0xFA, 0xFF, 0xA9, 0x34, 0x07};
  TEST_ASSERT(fio_deflate_decompress(out,
                                     sizeof(out),
                                     s_dist_hole,
                                     sizeof(s_dist_hole)) == 0,
              "reference to a distance-table hole must be rejected");
  /* size-query (counting) mode must reject too — this was the hang path. */
  TEST_ASSERT(fio_deflate_decompress(NULL,
                                     0,
                                     s_dist_hole,
                                     sizeof(s_dist_hole)) == 0,
              "distance-table hole in counting mode must be rejected");
}

int main(void) {
  fprintf(stderr, "=== DEFLATE/INFLATE Correctness Test Suite ===\n\n");

  test_known_stored_block();
  test_raw_roundtrip_levels();
  test_edges_and_errors();
  test_gzip_roundtrip_and_trailer();
  test_streaming_roundtrip();
  test_streaming_buffered_pushes();
  test_stream_multiblock_inflate();
  test_compress_bound_property();
  test_stored_fallback_cap();
  test_stream_bounded_scratch();
  test_ws_memory_budget();
  test_stream_takeover_mode();
  test_large_skewed_roundtrips();
  test_invalid_huffman_tables();
  test_decode_table_heap_growth();
#ifdef HAVE_ZLIB
  test_zlib_raw_interop();
  test_zlib_gzip_interop();
  test_zlib_streaming_interop();
  test_zlib_multiblock_inflate();
  test_zlib_takeover_interop();
#else
  fprintf(stderr, "zlib unavailable: skipping independent interop checks.\n");
#endif
  test_big_payload_roundtrip();

  fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
