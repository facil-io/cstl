/* *****************************************************************************
BROTLI Correctness Tests
Covers ./fio-stl/161 brotli-tables.h and ./fio-stl/162 brotli.h.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_BROTLI
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
  size_t lens[7] = {0};
  fio___brotli_make_corpus(text, sizeof(text));
  fio___brotli_fill_pattern(binary, sizeof(binary), 0xC0FFEEU);

  {
    uint8_t compressed[fio_brotli_compress_bound(TEXT_LEN)];
    for (int quality = 1; quality <= 6; ++quality) {
      size_t clen = fio_brotli_compress(compressed,
                                        sizeof(compressed),
                                        text,
                                        sizeof(text),
                                        quality);
      lens[quality] = clen;
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

int main(void) {
  fprintf(stderr, "=== Brotli Correctness Test Suite ===\n\n");

  test_reference_decode_vectors();
  test_dictionary_tables_and_transforms();
  test_api_edges_and_size_queries();
  test_quality_roundtrips();

  fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
