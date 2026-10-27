/* *****************************************************************************
BROTLI Correctness Tests
Covers ./fio-stl/161 brotli-tables.h and ./fio-stl/162 brotli.h.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_BROTLI
#define FIO_THREADS
#include FIO_INCLUDE_FILE

#include <string.h>

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

FIO_SFUNC void fio___brotli_fill_pattern(uint8_t *buf,
                                         size_t len,
                                         uint32_t seed) {
  for (size_t i = 0; i < len; ++i) {
    seed = seed * 1103515245U + 12345U;
    buf[i] = (uint8_t)((seed >> 16) ^ (seed >> 24) ^ (uint32_t)i);
  }
}

FIO_SFUNC void fio___brotli_make_corpus(uint8_t *buf, size_t len) {
  static const char *segments[] = {
      "The international conference discussed important developments in "
      "technology, education, communication, and public infrastructure. ",
      "<article><h1>Brotli correctness coverage</h1><p>Static dictionary "
      "and context modeling should help with common English words and "
      "markup.</p></article>\n",
      "{\"module\":\"brotli\",\"status\":\"ok\",\"message\":"
      "\"reference vectors and deterministic roundtrips\"}\n",
      "The quick brown fox jumps over the lazy dog while facil.io validates "
      "compression and decompression behavior. ",
  };
  const size_t count = sizeof(segments) / sizeof(segments[0]);
  size_t pos = 0;
  for (size_t i = 0; pos < len; ++i) {
    const char *seg = segments[i % count];
    size_t seg_len = strlen(seg);
    if (seg_len > len - pos)
      seg_len = len - pos;
    FIO_MEMCPY(buf + pos, seg, seg_len);
    pos += seg_len;
  }
}

FIO_SFUNC const uint8_t *fio___brotli_find_ascii_word(uint32_t min_len,
                                                      uint32_t *out_len) {
  for (uint32_t wlen = min_len; wlen <= 24; ++wlen) {
    uint32_t nbits = fio___brotli_ndbits[wlen];
    if (!nbits)
      continue;
    uint32_t nwords = 1U << nbits;
    const uint8_t *base = fio___brotli_dict + fio___brotli_dict_offsets[wlen];
    for (uint32_t wid = 0; wid < nwords; ++wid) {
      const uint8_t *word = base + (wid * wlen);
      if (word[0] < 'a' || word[0] > 'z')
        continue;
      int ascii = 1;
      for (uint32_t i = 1; i < wlen; ++i) {
        if (word[i] < 'a' || word[i] > 'z') {
          ascii = 0;
          break;
        }
      }
      if (ascii) {
        *out_len = wlen;
        return word;
      }
    }
  }
  return NULL;
}

static void test_reference_decode_vectors(void) {
  fprintf(stderr, "Testing reference Brotli decode vectors...\n");

  {
    static const uint8_t empty_stream[] = {0x3b};
    uint8_t out[8] = {0};
    size_t len = fio_brotli_decompress(out,
                                       sizeof(out),
                                       empty_stream,
                                       sizeof(empty_stream));
    TEST_ASSERT(len == 0,
                "reference empty stream: expected 0 bytes, got %zu",
                len);
    TEST_ASSERT(
        fio_brotli_decompress(NULL, 0, empty_stream, sizeof(empty_stream)) == 0,
        "reference empty stream: counting mode expected 0");
  }

  {
    static const uint8_t compressed[] = {0x0b,
                                         0x06,
                                         0x80,
                                         0x48,
                                         0x65,
                                         0x6c,
                                         0x6c,
                                         0x6f,
                                         0x2c,
                                         0x20,
                                         0x57,
                                         0x6f,
                                         0x72,
                                         0x6c,
                                         0x64,
                                         0x21,
                                         0x03};
    static const char expected[] = "Hello, World!";
    uint8_t out[64] = {0};
    size_t len =
        fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
    TEST_ASSERT(len == sizeof(expected) - 1,
                "reference hello: expected %zu bytes, got %zu",
                sizeof(expected) - 1,
                len);
    TEST_ASSERT(len == sizeof(expected) - 1 &&
                    !FIO_MEMCMP(out, expected, sizeof(expected) - 1),
                "reference hello: output mismatch");
    TEST_ASSERT(
        fio_brotli_decompress(NULL, 0, compressed, sizeof(compressed)) ==
            sizeof(expected) - 1,
        "reference hello: counting mode mismatch");
  }

  {
    static const uint8_t compressed[] = {
        0x8b, 0x8f, 0x01, 0x00, 0x80, 0xaa, 0xaa, 0xaa, 0xea, 0xff, 0x7c,
        0xe6, 0x65, 0x81, 0x03, 0xb8, 0xf8, 0x95, 0x2e, 0x55, 0x0c, 0x36,
        0x18, 0x73, 0xec, 0x28, 0xa0, 0x9c, 0xee, 0x3d, 0x34, 0x03};
    uint8_t expected[800];
    uint8_t out[832] = {0};
    for (size_t i = 0; i < 100; ++i)
      FIO_MEMCPY(expected + (i * 8), "ABCDEFGH", 8);
    size_t len =
        fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
    TEST_ASSERT(len == sizeof(expected),
                "reference repeated: expected %zu bytes, got %zu",
                sizeof(expected),
                len);
    TEST_ASSERT(len == sizeof(expected) &&
                    !FIO_MEMCMP(out, expected, sizeof(expected)),
                "reference repeated: output mismatch");
  }

  {
    static const uint8_t compressed[] = {
        0x1b, 0x83, 0x03, 0x00, 0x44, 0xdb, 0x46, 0xa9, 0x2e, 0x24, 0x5b,
        0x32, 0x14, 0xc5, 0x53, 0x91, 0x67, 0x72, 0xf2, 0xe7, 0x28, 0x50,
        0x15, 0x98, 0x57, 0xb6, 0xb2, 0x59, 0xd0, 0x6c, 0xe1, 0x95, 0xa7,
        0x23, 0xf2, 0xa2, 0xac, 0x36, 0x26, 0xb8, 0x45, 0x1f, 0x18, 0x27,
        0x62, 0x75, 0xff, 0x21, 0x30, 0x19, 0x00};
    static const char phrase[] =
        "The quick brown fox jumps over the lazy dog. ";
    uint8_t expected[(sizeof(phrase) - 1) * 20];
    uint8_t out[1024] = {0};
    for (size_t i = 0; i < 20; ++i)
      FIO_MEMCPY(expected + (i * (sizeof(phrase) - 1)),
                 phrase,
                 sizeof(phrase) - 1);
    size_t len =
        fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
    TEST_ASSERT(len == sizeof(expected),
                "reference q5 text: expected %zu bytes, got %zu",
                sizeof(expected),
                len);
    TEST_ASSERT(len == sizeof(expected) &&
                    !FIO_MEMCMP(out, expected, sizeof(expected)),
                "reference q5 text: output mismatch");
  }

  {
    static const uint8_t compressed[] = {
        0x1b, 0x4b, 0x00, 0x58, 0x8c, 0xd4, 0x61, 0xcd, 0x9d, 0x07, 0x02, 0xdd,
        0x58, 0x2e, 0xe3, 0x25, 0x19, 0xaa, 0x60, 0x50, 0x38, 0xf8, 0x78, 0x81,
        0x0d, 0x38, 0x70, 0x68, 0xb2, 0x41, 0x1f, 0x7c, 0xe1, 0xc1, 0x21, 0x37,
        0xbf, 0x61, 0x09, 0x24, 0x1e, 0x7a, 0x5c, 0x31, 0xb9, 0x55, 0x89, 0x4c,
        0x54, 0x7f, 0x60, 0xd1, 0x16, 0xd6, 0xc2, 0x78, 0xa2, 0x83, 0x15, 0x02};
    static const char expected[] =
        "<html><head><title>Test</title></head><body><p>Hello "
        "World</p></body></html>";
    uint8_t out[256] = {0};
    size_t len =
        fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
    TEST_ASSERT(len == sizeof(expected) - 1,
                "reference q11 html: expected %zu bytes, got %zu",
                sizeof(expected) - 1,
                len);
    TEST_ASSERT(len == sizeof(expected) - 1 &&
                    !FIO_MEMCMP(out, expected, sizeof(expected) - 1),
                "reference q11 html: output mismatch");
    TEST_ASSERT(
        fio_brotli_decompress(NULL, 0, compressed, sizeof(compressed)) ==
            sizeof(expected) - 1,
        "reference q11 html: counting mode mismatch");
  }
  /* The vectors below were built by hand and checked with libbrotlidec. */

  {
    /* Two block types in each category: every NBLTYPES is followed by the
     * block type code, the block count code and the first block count of
     * its own category (RFC 7932 section 9.2). */
    static const uint8_t compressed[] = {
        0x02, 0x00, 0x20, 0x82, 0xc8, 0x00, 0x00, 0x00, 0x11,
        0x44, 0x06, 0x00, 0x00, 0x88, 0x20, 0x32, 0x00, 0x00,
        0x00, 0x00, 0x44, 0x58, 0x20, 0x10, 0x08, 0x04, 0x00};
    uint8_t out[16] = {0};
    size_t len =
        fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
    TEST_ASSERT(len == 1 && out[0] == 'a',
                "block types in every category: expected \"a\", got %zu bytes",
                len);
  }

  {
    /* All 256 literals have 8-bit codes. The code length code uses only
     * one symbol (8), which takes zero bits (RFC 7932 section 3.5). */
    static const uint8_t compressed[] = {0x62,
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x07,
                                         0x00,
                                         0x04,
                                         0x08,
                                         0x01,
                                         0x18,
                                         0x39,
                                         0xd9,
                                         0xbb,
                                         0x00};
    uint8_t out[16] = {0};
    size_t len =
        fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
    TEST_ASSERT(len == 4 && !FIO_MEMCMP(out, "brot", 4),
                "single code length code: expected \"brot\", got %zu bytes",
                len);
  }

  {
    /* WBITS=10 (1008-byte window): 1090 literals, then distance 1009. That
     * is within the output but beyond the window, so it refers to the static
     * dictionary (RFC 7932 section 4): word 0 of length 4, "time". */
    static const uint8_t compressed[] = {0xa1,
                                         0x28,
                                         0x22,
                                         0x00,
                                         0x00,
                                         0x11,
                                         0x16,
                                         0xe2,
                                         0xc5,
                                         0x07,
                                         0x00,
                                         0x3d};
    uint8_t expected[1094];
    uint8_t out[1200] = {0};
    FIO_MEMSET(expected, 'a', 1090);
    FIO_MEMCPY(expected + 1090, "time", 4);
    size_t len =
        fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
    TEST_ASSERT(len == sizeof(expected) &&
                    !FIO_MEMCMP(out, expected, sizeof(expected)),
                "dictionary beyond the window: expected %zu bytes ending in "
                "\"time\", got %zu bytes",
                sizeof(expected),
                len);
  }

  {
    /* google/brotli 1.2.0, quality 3: all byte values repeated 32 times.
     * The literal tree uses the single-symbol code-length representation. */
    static const uint8_t compressed[] = {
        0x1b, 0xff, 0x1f, 0x00, 0x60, 0x00, 0x0e, 0x00, 0x00, 0xb5, 0x78, 0x34,
        0x38, 0x22, 0xad, 0x05, 0x00, 0x00, 0x20, 0x10, 0x30, 0x08, 0x28, 0x18,
        0x38, 0x04, 0x24, 0x14, 0x34, 0x0c, 0x2c, 0x1c, 0x3c, 0x02, 0x22, 0x12,
        0x32, 0x0a, 0x2a, 0x1a, 0x3a, 0x06, 0x26, 0x16, 0x36, 0x0e, 0x2e, 0x1e,
        0x3e, 0x01, 0x21, 0x11, 0x31, 0x09, 0x29, 0x19, 0x39, 0x05, 0x25, 0x15,
        0x35, 0x0d, 0x2d, 0x1d, 0x3d, 0x03, 0x23, 0x13, 0x33, 0x0b, 0x2b, 0x1b,
        0x3b, 0x07, 0x27, 0x17, 0x37, 0x0f, 0x2f, 0x1f, 0xbf, 0x80, 0xa0, 0x90,
        0xb0, 0x88, 0xa8, 0x98, 0xb8, 0x84, 0xa4, 0x94, 0xb4, 0x8c, 0xac, 0x9c,
        0xbc, 0x82, 0xa2, 0x92, 0xb2, 0x8a, 0xaa, 0x9a, 0xba, 0x86, 0xa6, 0x96,
        0xb6, 0x8e, 0xae, 0x9e, 0xbe, 0x81, 0xa1, 0x91, 0xb1, 0x89, 0xa9, 0x99,
        0xb9, 0x85, 0xa5, 0x95, 0xb5, 0x8d, 0xad, 0x9d, 0xbd, 0x83, 0xa3, 0x93,
        0xb3, 0x8b, 0xab, 0x9b, 0xbb, 0x87, 0xa7, 0x97, 0xb7, 0x8f, 0xaf, 0x9f,
        0x7f, 0x40, 0x60, 0x50, 0x70, 0x48, 0x68, 0x58, 0x78, 0x44, 0x64, 0x54,
        0x74, 0x4c, 0x6c, 0x5c, 0x7c, 0x42, 0x62, 0x52, 0x72, 0x4a, 0x6a, 0x5a,
        0x7a, 0x46, 0x66, 0x56, 0x76, 0x4e, 0x6e, 0x5e, 0x7e, 0x41, 0x61, 0x51,
        0x71, 0x49, 0x69, 0x59, 0x79, 0x45, 0x65, 0x55, 0x75, 0x4d, 0x6d, 0x5d,
        0x7d, 0x43, 0x63, 0x53, 0x73, 0x4b, 0x6b, 0x5b, 0x7b, 0x47, 0x67, 0x57,
        0x77, 0x4f, 0x6f, 0x5f, 0xff, 0xc0, 0xe0, 0xd0, 0xf0, 0xc8, 0xe8, 0xd8,
        0xf8, 0xc4, 0xe4, 0xd4, 0xf4, 0xcc, 0xec, 0xdc, 0xfc, 0xc2, 0xe2, 0xd2,
        0xf2, 0xca, 0xea, 0xda, 0xfa, 0xc6, 0xe6, 0xd6, 0xf6, 0xce, 0xee, 0xde,
        0xfe, 0xc1, 0xe1, 0xd1, 0xf1, 0xc9, 0xe9, 0xd9, 0xf9, 0xc5, 0xe5, 0xd5,
        0xf5, 0xcd, 0xed, 0xdd, 0xfd, 0xc3, 0xe3, 0xd3, 0xf3, 0xcb, 0xeb, 0xdb,
        0xfb, 0xc7, 0xe7, 0xd7, 0xf7, 0xcf, 0xef, 0xdf, 0x3f, 0x00, 0x20, 0x10,
        0x30, 0x08, 0xe8, 0x00,
    };
    uint8_t out[8192] = {0};
    size_t len =
        fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
    TEST_ASSERT(len == sizeof(out),
                "reference q3 binary: expected %zu bytes, got %zu",
                sizeof(out), len);
    size_t matched = 0;
    while (matched < sizeof(out) && out[matched] == (uint8_t)matched)
      ++matched;
    TEST_ASSERT(len == sizeof(out) && matched == sizeof(out),
                "reference q3 binary: mismatch at byte %zu", matched);
  }

}

static void test_dictionary_tables_and_transforms(void) {
  fprintf(stderr, "Testing Brotli dictionary tables and transforms...\n");

  uint32_t word_len = 0;
  const uint8_t *word = fio___brotli_find_ascii_word(8, &word_len);
  TEST_ASSERT(word != NULL,
              "dictionary scan: expected an ASCII lowercase word");
  if (!word)
    return;

  uint8_t transformed[64] = {0};

  int tlen = fio___brotli_transform_word(transformed, word, (int)word_len, 0);
  TEST_ASSERT(tlen == (int)word_len,
              "transform 0: expected %u bytes, got %d",
              word_len,
              tlen);
  TEST_ASSERT(tlen == (int)word_len && !FIO_MEMCMP(transformed, word, word_len),
              "transform 0: identity mismatch");

  tlen = fio___brotli_transform_word(transformed, word, (int)word_len, 9);
  TEST_ASSERT(tlen == (int)word_len,
              "transform 9: expected %u bytes, got %d",
              word_len,
              tlen);
  TEST_ASSERT(tlen == (int)word_len &&
                  transformed[0] == (uint8_t)(word[0] ^ 32) &&
                  !FIO_MEMCMP(transformed + 1, word + 1, word_len - 1),
              "transform 9: uppercase-first mismatch");

  tlen = fio___brotli_transform_word(transformed,
                                     word,
                                     (int)word_len,
                                     fio___brotli_cut_off_transforms[3]);
  TEST_ASSERT(tlen == (int)word_len - 3,
              "cutoff transform: expected %u bytes, got %d",
              word_len - 3,
              tlen);
  TEST_ASSERT(tlen == (int)word_len - 3 &&
                  !FIO_MEMCMP(transformed, word, word_len - 3),
              "cutoff transform: omit-last-3 mismatch");

  fio___brotli_dict_ht_build();
  TEST_ASSERT(fio___brotli_dict_ht_ready,
              "dictionary hash table should be marked ready after build");
  {
    uint32_t base = fio___brotli_dict_hash4(word) * 2;
    TEST_ASSERT(
        fio___brotli_dict_ht[base].len || fio___brotli_dict_ht[base + 1].len,
        "dictionary hash table should populate a bucket for the chosen word");
  }
}

/* NBLTYPESx is a VarLenUint8 plus one (RFC 7932 section 9.2), the same
 * encoding the compressor writes. */
static void test_block_type_counts(void) {
  fprintf(stderr, "Testing Brotli block type count decoding...\n");
  uint32_t first_bad = 0, got = 0;
  for (uint32_t count = 1; count <= 256; ++count) {
    uint8_t buf[16] = {0};
    fio___brotli_bw_s w;
    fio___brotli_bw_init(&w, buf, sizeof(buf));
    fio___brotli_write_varlen_uint8(&w, count - 1);
    fio___brotli_bw_put(&w, 0x5A, 8); /* marker: the next field */
    size_t len = fio___brotli_bw_finish(&w, buf);

    fio___brotli_bits_s bits;
    fio___brotli_bits_init(&bits, buf, len);
    got = fio___brotli_read_nbltypes(&bits);
    if (got != count || fio___brotli_bits_get(&bits, 8) != 0x5A) {
      first_bad = count;
      break;
    }
  }
  TEST_ASSERT(!first_bad,
              "block type count %u decoded as %u (or misaligned the stream)",
              (unsigned)first_bad,
              (unsigned)got);
}


static void test_api_edges_and_size_queries(void) {
  fprintf(stderr, "Testing Brotli API edge cases and size queries...\n");

  TEST_ASSERT(fio_brotli_decompress_bound(0) >= 1024,
              "decompress_bound(0) should honor documented minimum");
  TEST_ASSERT(fio_brotli_decompress_bound(128) > 128,
              "decompress_bound should exceed compressed input");
  TEST_ASSERT(fio_brotli_compress_bound(128) > 128,
              "compress_bound should exceed source input");

  {
    uint8_t out[16] = {0};
    TEST_ASSERT(fio_brotli_decompress(out, sizeof(out), NULL, 1) == 0,
                "NULL input must fail");
    TEST_ASSERT(fio_brotli_decompress(out, sizeof(out), "x", 0) == 0,
                "zero-length input must fail");
    TEST_ASSERT(fio_brotli_decompress(NULL, 0, "x", 1) == 0,
                "counting mode with corrupt input must fail");
    TEST_ASSERT(fio_brotli_compress(NULL, sizeof(out), "x", 1, 1) == 0,
                "NULL output must fail");
  }

  {
    uint8_t compressed[8] = {0};
    uint8_t out[8] = {0};
    size_t clen =
        fio_brotli_compress(compressed, sizeof(compressed), NULL, 0, 4);
    TEST_ASSERT(clen == 1,
                "empty input: expected 1-byte stream, got %zu",
                clen);
    TEST_ASSERT(clen == 1 && compressed[0] == 0x06,
                "empty input: expected canonical 0x06 empty stream");
    TEST_ASSERT(fio_brotli_decompress(out, sizeof(out), compressed, clen) == 0,
                "empty stream: decompressed length should be zero");
    TEST_ASSERT(fio_brotli_decompress(NULL, 0, compressed, clen) == 0,
                "empty stream: counting mode should be zero");
  }

  {
    static const char input[] = "ABCDEFGH";
    uint8_t compressed[24 + 1024];
    uint8_t out[16] = {0};
    size_t clen = fio_brotli_compress(compressed,
                                      sizeof(compressed),
                                      input,
                                      sizeof(input) - 1,
                                      1);
    TEST_ASSERT(clen > 0, "small-buffer test: compression failed");
    if (clen) {
      TEST_ASSERT(fio_brotli_decompress(NULL, 0, compressed, clen) ==
                      sizeof(input) - 1,
                  "small-buffer test: counting mode mismatch");
      TEST_ASSERT(fio_brotli_decompress(NULL, 64, compressed, clen) ==
                      sizeof(input) - 1,
                  "small-buffer test: NULL output mismatch");
      for (size_t out_len = 1; out_len < sizeof(input) - 1; ++out_len) {
        size_t needed = fio_brotli_decompress(out, out_len, compressed, clen);
        TEST_ASSERT(needed == sizeof(input) - 1,
                    "small-buffer test: out_len=%zu expected %zu, got %zu",
                    out_len,
                    sizeof(input) - 1,
                    needed);
      }
    }
  }
}

static void test_quality_roundtrips(void) {
  fprintf(stderr, "Testing Brotli roundtrips across quality levels...\n");

  enum { TEXT_LEN = 16384, BINARY_LEN = 12288 };
  uint8_t text[TEXT_LEN];
  uint8_t binary[BINARY_LEN];
  uint8_t out[TEXT_LEN + 64];
  fio___brotli_make_corpus(text, sizeof(text));
  fio___brotli_fill_pattern(binary, sizeof(binary), 0xC0FFEEU);

  {
    uint8_t compressed[(TEXT_LEN * 3) + 1024];
    for (int quality = 1; quality <= 6; ++quality) {
      size_t clen = fio_brotli_compress(compressed,
                                        sizeof(compressed),
                                        text,
                                        sizeof(text),
                                        quality);
      TEST_ASSERT(clen > 0 && clen <= sizeof(compressed),
                  "text q%d: compress returned %zu",
                  quality,
                  clen);
      if (!clen || clen > sizeof(compressed))
        continue;

      TEST_ASSERT(fio_brotli_decompress(NULL, 0, compressed, clen) ==
                      sizeof(text),
                  "text q%d: counting mode mismatch",
                  quality);
      TEST_ASSERT(fio_brotli_decompress(out, TEXT_LEN / 3, compressed, clen) ==
                      sizeof(text),
                  "text q%d: small-buffer required size mismatch",
                  quality);

      size_t dlen = fio_brotli_decompress(out, sizeof(out), compressed, clen);
      TEST_ASSERT(dlen == sizeof(text),
                  "text q%d: expected %zu bytes, got %zu",
                  quality,
                  sizeof(text),
                  dlen);
      TEST_ASSERT(dlen == sizeof(text) && !FIO_MEMCMP(out, text, sizeof(text)),
                  "text q%d: roundtrip mismatch",
                  quality);
    }
  }

  {
    uint8_t compressed[(BINARY_LEN * 3) + 1024];
    for (int quality = 1; quality <= 6; ++quality) {
      size_t clen = fio_brotli_compress(compressed,
                                        sizeof(compressed),
                                        binary,
                                        sizeof(binary),
                                        quality);
      TEST_ASSERT(clen > 0 && clen <= sizeof(compressed),
                  "binary q%d: compress returned %zu",
                  quality,
                  clen);
      if (!clen || clen > sizeof(compressed))
        continue;
      TEST_ASSERT(fio_brotli_decompress(NULL, 0, compressed, clen) ==
                      sizeof(binary),
                  "binary q%d: counting mode mismatch",
                  quality);
      size_t dlen = fio_brotli_decompress(out, sizeof(out), compressed, clen);
      TEST_ASSERT(dlen == sizeof(binary),
                  "binary q%d: expected %zu bytes, got %zu",
                  quality,
                  sizeof(binary),
                  dlen);
      TEST_ASSERT(dlen == sizeof(binary) &&
                      !FIO_MEMCMP(out, binary, sizeof(binary)),
                  "binary q%d: roundtrip mismatch",
                  quality);
    }
  }
}

/* *****************************************************************************
T005 — Inputs above 16MB must compress fully (chunked meta-blocks)

fio_brotli_compress must support inputs larger than the 16MB single
meta-block limit by emitting chunked meta-blocks (ISLAST on the final chunk
only). Regression: the pre-fix implementation silently truncated the input
at 16MB and returned a valid stream of the prefix.
***************************************************************************** */

static void test_large_input_roundtrips(void) {
  fprintf(stderr,
          "Testing >16MB Brotli roundtrips (chunked meta-blocks)...\n");
  static const size_t sizes[] = {
      (20u << 20) + 17, /* crosses the 16MB meta-block limit (+ tail) */
      (16u << 20),      /* exactly at the old limit (regression guard) */
      (16u << 20) + 1,  /* one byte past the old limit */
  };
  static const int qualities[] = {1, 6};

  for (size_t si = 0; si < sizeof(sizes) / sizeof(sizes[0]); ++si) {
    size_t n = sizes[si];
    uint8_t *src = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, n, 0);
    size_t bound = fio_brotli_compress_bound(n);
    uint8_t *comp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, bound, 0);
    uint8_t *out = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, n + 1024, 0);
    TEST_ASSERT(src && comp && out,
                "large input %zu: allocation failed",
                n);
    if (!src || !comp || !out)
      goto next;
    fio___brotli_make_corpus(src, n);

    for (size_t qi = 0; qi < sizeof(qualities) / sizeof(qualities[0]);
         ++qi) {
      int q = qualities[qi];
      size_t clen = fio_brotli_compress(comp, bound, src, n, q);
      TEST_ASSERT(clen > 0 && clen <= bound,
                  "large input %zu q%d: compress returned %zu (bound %zu)",
                  n,
                  q,
                  clen,
                  bound);
      if (!clen || clen > bound)
        continue;

      TEST_ASSERT(fio_brotli_decompress(NULL, 0, comp, clen) == n,
                  "large input %zu q%d: counting mode expected %zu",
                  n,
                  q,
                  n);

      size_t dlen = fio_brotli_decompress(out, n + 1024, comp, clen);
      TEST_ASSERT(dlen == n,
                  "large input %zu q%d: expected full roundtrip %zu bytes, "
                  "got %zu (input silently truncated at 16MB)",
                  n,
                  q,
                  n,
                  dlen);
      TEST_ASSERT(dlen == n && !FIO_MEMCMP(out, src, n),
                  "large input %zu q%d: payload mismatch",
                  n,
                  q);
    }

  next:
    if (out)
      FIO_MEM_FREE(out, n + 1024);
    if (comp)
      FIO_MEM_FREE(comp, bound);
    if (src)
      FIO_MEM_FREE(src, n);
  }
}

/* Build a single-symbol compressed block, including deliberately invalid
 * command/MLEN combinations that must be rejected before writing output. */
static size_t test_malformed_stream(uint8_t *buf,
                                    uint32_t mlen,
                                    uint32_t iac,
                                    uint32_t distance) {
  fio___brotli_bw_s w;
  fio___brotli_bw_init(&w, buf, 128);
  fio___brotli_bw_put(&w, 0, 1); /* WBITS=16 */
  fio___brotli_bw_put(&w, 1, 1); /* ISLAST */
  fio___brotli_bw_put(&w, 0, 1); /* !ISLASTEMPTY */
  fio___brotli_bw_put(&w, 0, 2); /* four MLEN nibbles */
  fio___brotli_bw_put(&w, mlen - 1, 16);
  /* One block type in each category, NPOSTFIX=NDIRECT=0, context mode=0,
   * one literal tree and one distance tree. */
  fio___brotli_bw_put(&w, 0, 3 + 2 + 4 + 2 + 1 + 1);
  fio___brotli_bw_put(&w, 1, 2);
  fio___brotli_bw_put(&w, 0, 2);
  fio___brotli_bw_put(&w, 'a', 8);
  fio___brotli_bw_put(&w, 1, 2);
  fio___brotli_bw_put(&w, 0, 2);
  fio___brotli_bw_put(&w, iac, 10);
  fio___brotli_bw_put(&w, 1, 2);
  fio___brotli_bw_put(&w, 0, 2);
  fio___brotli_bw_put(&w, distance, 6);
  fio___brotli_bw_put(&w, 0, 32);
  return fio___brotli_bw_finish(&w, buf);
}

static void test_security_regressions(void) {
  fprintf(stderr, "Testing malformed streams and encoder capacities...\n");
  uint8_t stream[128] = {0}, output[512];
  /* IAC 131: insert 0, copy 5. Distance 44: dictionary transform 63,
   * OMIT_LAST_5; the empty result must eventually hit sticky exhaustion. */
  size_t n = test_malformed_stream(stream, 1, 131, 44);
  TEST_ASSERT(fio_brotli_decompress(output, sizeof(output), stream, n) == 0,
              "empty dictionary commands must terminate on exhaustion");
  n = test_malformed_stream(stream, 1, 131, 16);
  TEST_ASSERT(fio_brotli_decompress(output, sizeof(output), stream, n) == 0,
              "dictionary output must not exceed MLEN");
  TEST_ASSERT(fio_brotli_decompress(output, 1, stream, n) == 0,
              "overlong dictionary output must not report partial success");
  n = test_malformed_stream(stream, 1, 16, 16); /* insert 2 */
  FIO_MEMSET(output, 0xA5, sizeof(output));
  TEST_ASSERT(fio_brotli_decompress(output, sizeof(output), stream, n) == 0 &&
                  output[0] == 0xA5,
              "overlong inserts must be rejected before writing");

  fio___brotli_bits_s bits;
  fio___brotli_bits_init(&bits, stream, 1);
  (void)fio___brotli_bits_get(&bits, 16);
  TEST_ASSERT(bits.error, "exhaustion must set a sticky error");
  uint32_t table[256];
  for (size_t i = 0; i < 256; ++i)
    table[i] = ((uint32_t)'a' << 8); /* valid zero-bit code */
  TEST_ASSERT(fio___brotli_huff_decode(&bits, table, 8) == UINT32_MAX,
              "zero-bit codes must not hide earlier exhaustion");

  static const char input[] = "hello hello hello and the international world";
  uint8_t encoded[256], guarded[258], decoded[sizeof(input)];
  for (int q = 1; q <= 6; ++q) {
    size_t full = fio_brotli_compress(encoded, sizeof(encoded), input,
                                      sizeof(input), q);
    TEST_ASSERT(full > 1, "capacity sweep q%d setup", q);
    for (size_t cap = 0; cap <= full + 1; ++cap) {
      FIO_MEMSET(guarded, 0xA5, sizeof(guarded));
      size_t got = fio_brotli_compress(guarded + 1, cap, input,
                                       sizeof(input), q);
      TEST_ASSERT(guarded[0] == 0xA5 && guarded[cap + 1] == 0xA5,
                  "q%d capacity %zu canaries", q, cap);
      TEST_ASSERT(cap < full ? got == 0 : got == full,
                  "q%d capacity %zu must fail or emit complete stream (%zu)",
                  q, cap, got);
      if (got)
        TEST_ASSERT(fio_brotli_decompress(decoded, sizeof(decoded),
                                            guarded + 1, got) == sizeof(input) &&
                        !FIO_MEMCMP(input, decoded, sizeof(input)),
                    "q%d capacity %zu roundtrip", q, cap);
    }
  }
}

static unsigned test_dict_start;
static unsigned test_dict_failures;
static void *test_dict_first_use_thread(void *arg) {
  (void)arg;
  unsigned ready;
  do {
    fio_atomic_load(ready, &test_dict_start);
    if (!ready)
      FIO_THREAD_RESCHEDULE();
  } while (!ready);
  uint8_t src[16384], encoded[22000], decoded[16384];
  fio___brotli_make_corpus(src, sizeof(src));
  for (int q = 5; q <= 6; ++q) {
    size_t n = fio_brotli_compress(encoded, sizeof(encoded), src, sizeof(src), q);
    if (!n || fio_brotli_decompress(decoded, sizeof(decoded), encoded, n) !=
                  sizeof(src) || FIO_MEMCMP(decoded, src, sizeof(src)))
      fio_atomic_add(&test_dict_failures, 1);
  }
  return NULL;
}

static void test_concurrent_dictionary_first_use(void) {
  fio_thread_t threads[8];
  for (size_t i = 0; i < 8; ++i)
    FIO_ASSERT(!fio_thread_create(threads + i, test_dict_first_use_thread, NULL),
                "dictionary thread creation failed");
  fio_atomic_or(&test_dict_start, 1);
  for (size_t i = 0; i < 8; ++i)
    fio_thread_join(threads + i);
  TEST_ASSERT(test_dict_failures == 0, "concurrent dictionary first-use");
}

int main(void) {
  fprintf(stderr, "=== Brotli Correctness Test Suite ===\n\n");

  test_concurrent_dictionary_first_use();
  test_security_regressions();
  test_reference_decode_vectors();
  test_dictionary_tables_and_transforms();
  test_block_type_counts();
  test_api_edges_and_size_queries();
  test_quality_roundtrips();
  test_large_input_roundtrips();

  fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
