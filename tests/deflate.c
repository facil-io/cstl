/* *****************************************************************************
DEFLATE / INFLATE Correctness Tests
Covers ./fio-stl/162 deflate.h.
***************************************************************************** */
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

int main(void) {
  fprintf(stderr, "=== DEFLATE/INFLATE Correctness Test Suite ===\n\n");

  test_known_stored_block();
  test_raw_roundtrip_levels();
  test_edges_and_errors();
  test_gzip_roundtrip_and_trailer();
  test_streaming_roundtrip();
  test_streaming_buffered_pushes();
#ifdef HAVE_ZLIB
  test_zlib_raw_interop();
  test_zlib_gzip_interop();
  test_zlib_streaming_interop();
#else
  fprintf(stderr, "zlib unavailable: skipping independent interop checks.\n");
#endif

  fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
