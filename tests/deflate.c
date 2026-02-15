/* *****************************************************************************
DEFLATE / INFLATE Tests
***************************************************************************** */
#include "test-helpers.h"
#define FIO_DEFLATE
#include FIO_INCLUDE_FILE

#include <stdio.h>
#include <string.h>

static int g_pass = 0;
static int g_fail = 0;

#define TEST_ASSERT(cond, ...)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "  FAIL: " __VA_ARGS__);                                 \
      fprintf(stderr, "\n");                                                   \
      g_fail++;                                                                \
    } else {                                                                   \
      g_pass++;                                                                \
    }                                                                          \
  } while (0)

/* *****************************************************************************
Test: inflate known DEFLATE data
***************************************************************************** */

static void test_inflate_stored_block(void) {
  fprintf(stderr, "Testing inflate: stored block...\n");
  /* A stored block: BFINAL=1, BTYPE=00, LEN=5, NLEN=~5, "Hello" */
  uint8_t compressed[] = {
      0x01, /* BFINAL=1, BTYPE=00 (stored) */
      0x05,
      0x00, /* LEN = 5 */
      0xFA,
      0xFF, /* NLEN = ~5 */
      'H',
      'e',
      'l',
      'l',
      'o' /* data */
  };
  uint8_t out[64];
  size_t result =
      fio_deflate_decompress(out, sizeof(out), compressed, sizeof(compressed));
  TEST_ASSERT(result == 5, "stored block: expected 5 bytes, got %zu", result);
  TEST_ASSERT(result >= 5 && !memcmp(out, "Hello", 5),
              "stored block: data mismatch");
}

/* *****************************************************************************
Test: round-trip deflate then inflate
***************************************************************************** */

static void test_roundtrip_simple(void) {
  fprintf(stderr, "Testing round-trip: simple string...\n");
  const char *input = "Hello, World! This is a test of DEFLATE compression.";
  size_t in_len = strlen(input);

  /* Compress */
  size_t comp_bound = fio_deflate_compress_bound(in_len);
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  TEST_ASSERT(compressed != NULL, "malloc failed for compressed buffer");
  if (!compressed)
    return;

  size_t comp_len =
      fio_deflate_compress(compressed, comp_bound, input, in_len, 6);
  TEST_ASSERT(comp_len > 0, "deflate returned 0");

  /* Decompress */
  uint8_t *decompressed = (uint8_t *)malloc(in_len + 256);
  TEST_ASSERT(decompressed != NULL, "malloc failed for decompressed buffer");
  if (!decompressed) {
    free(compressed);
    return;
  }

  size_t decomp_len =
      fio_deflate_decompress(decompressed, in_len + 256, compressed, comp_len);
  TEST_ASSERT(decomp_len == in_len,
              "round-trip: expected %zu bytes, got %zu",
              in_len,
              decomp_len);
  TEST_ASSERT(decomp_len == in_len && !memcmp(decompressed, input, in_len),
              "round-trip: data mismatch");

  fprintf(stderr,
          "  Compressed %zu -> %zu bytes (%.1f%%)\n",
          in_len,
          comp_len,
          100.0 * (double)comp_len / (double)in_len);

  free(compressed);
  free(decompressed);
}

static void test_roundtrip_levels(void) {
  fprintf(stderr, "Testing round-trip: all compression levels...\n");
  /* Create test data with some repetition */
  const size_t data_len = 4096;
  uint8_t *data = (uint8_t *)malloc(data_len);
  TEST_ASSERT(data != NULL, "malloc failed");
  if (!data)
    return;

  for (size_t i = 0; i < data_len; ++i)
    data[i] = (uint8_t)((i * 7 + 13) & 0xFF);
  /* Add some repetitive patterns */
  for (size_t i = 0; i < 256; ++i) {
    data[1024 + i] = data[512 + i]; /* copy a block */
  }

  size_t comp_bound = fio_deflate_compress_bound(data_len);
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  uint8_t *decompressed = (uint8_t *)malloc(data_len + 256);

  if (!compressed || !decompressed) {
    free(data);
    free(compressed);
    free(decompressed);
    return;
  }

  for (int level = 0; level <= 9; ++level) {
    size_t comp_len =
        fio_deflate_compress(compressed, comp_bound, data, data_len, level);
    TEST_ASSERT(comp_len > 0, "level %d: deflate returned 0", level);
    if (!comp_len)
      continue;

    size_t decomp_len = fio_deflate_decompress(decompressed,
                                               data_len + 256,
                                               compressed,
                                               comp_len);
    TEST_ASSERT(decomp_len == data_len,
                "level %d: expected %zu, got %zu",
                level,
                data_len,
                decomp_len);
    TEST_ASSERT(decomp_len == data_len && !memcmp(decompressed, data, data_len),
                "level %d: data mismatch",
                level);

    fprintf(stderr,
            "  Level %d: %zu -> %zu bytes (%.1f%%)\n",
            level,
            data_len,
            comp_len,
            100.0 * (double)comp_len / (double)data_len);
  }

  free(data);
  free(compressed);
  free(decompressed);
}

/* *****************************************************************************
Test: round-trip with highly compressible data
***************************************************************************** */

static void test_roundtrip_repetitive(void) {
  fprintf(stderr, "Testing round-trip: highly repetitive data...\n");
  const size_t data_len = 8192;
  uint8_t *data = (uint8_t *)malloc(data_len);
  if (!data)
    return;

  /* Fill with repeating pattern "ABCABC..." */
  for (size_t i = 0; i < data_len; ++i)
    data[i] = (uint8_t)('A' + (i % 3));

  size_t comp_bound = fio_deflate_compress_bound(data_len);
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  uint8_t *decompressed = (uint8_t *)malloc(data_len + 256);

  if (!compressed || !decompressed) {
    free(data);
    free(compressed);
    free(decompressed);
    return;
  }

  size_t comp_len =
      fio_deflate_compress(compressed, comp_bound, data, data_len, 6);
  TEST_ASSERT(comp_len > 0, "repetitive: deflate returned 0");
  TEST_ASSERT(comp_len < data_len / 2,
              "repetitive: poor compression %zu -> %zu",
              data_len,
              comp_len);

  if (comp_len > 0) {
    size_t decomp_len = fio_deflate_decompress(decompressed,
                                               data_len + 256,
                                               compressed,
                                               comp_len);
    TEST_ASSERT(decomp_len == data_len,
                "repetitive: expected %zu, got %zu",
                data_len,
                decomp_len);
    TEST_ASSERT(decomp_len == data_len && !memcmp(decompressed, data, data_len),
                "repetitive: data mismatch");
    fprintf(stderr,
            "  Compressed %zu -> %zu bytes (%.1f%%)\n",
            data_len,
            comp_len,
            100.0 * (double)comp_len / (double)data_len);
  }

  free(data);
  free(compressed);
  free(decompressed);
}

/* *****************************************************************************
Test: gzip wrapper
***************************************************************************** */

static void test_gzip_roundtrip(void) {
  fprintf(stderr, "Testing gzip round-trip...\n");
  const char *input = "The quick brown fox jumps over the lazy dog. "
                      "Pack my box with five dozen liquor jugs.";
  size_t in_len = strlen(input);

  size_t gz_bound = fio_deflate_compress_bound(in_len) + 18;
  uint8_t *gz_data = (uint8_t *)malloc(gz_bound);
  uint8_t *decompressed = (uint8_t *)malloc(in_len + 256);

  if (!gz_data || !decompressed) {
    free(gz_data);
    free(decompressed);
    return;
  }

  size_t gz_len = fio_gzip_compress(gz_data, gz_bound, input, in_len, 6);
  TEST_ASSERT(gz_len > 0, "gzip returned 0");

  if (gz_len > 0) {
    /* Verify gzip header magic */
    TEST_ASSERT(gz_data[0] == 0x1F && gz_data[1] == 0x8B,
                "gzip: bad magic bytes");

    size_t decomp_len =
        fio_gzip_decompress(decompressed, in_len + 256, gz_data, gz_len);
    TEST_ASSERT(decomp_len == in_len,
                "gunzip: expected %zu, got %zu",
                in_len,
                decomp_len);
    TEST_ASSERT(decomp_len == in_len && !memcmp(decompressed, input, in_len),
                "gunzip: data mismatch");

    fprintf(stderr, "  Gzip: %zu -> %zu bytes\n", in_len, gz_len);
  }

  free(gz_data);
  free(decompressed);
}

/* *****************************************************************************
Test: edge cases
***************************************************************************** */

static void test_edge_empty_input(void) {
  fprintf(stderr, "Testing edge case: empty input...\n");

  /* Deflate empty */
  uint8_t comp[64];
  size_t comp_len = fio_deflate_compress(comp, sizeof(comp), NULL, 0, 6);
  TEST_ASSERT(comp_len == 5,
              "empty deflate: expected 5 (stored), got %zu",
              comp_len);

  /* Inflate the empty stored block */
  if (comp_len > 0) {
    uint8_t decomp[64];
    size_t decomp_len =
        fio_deflate_decompress(decomp, sizeof(decomp), comp, comp_len);
    TEST_ASSERT(decomp_len == 0,
                "empty inflate: expected 0, got %zu",
                decomp_len);
  }

  /* Gzip empty */
  uint8_t gz[64];
  size_t gz_len = fio_gzip_compress(gz, sizeof(gz), "", 0, 6);
  TEST_ASSERT(gz_len > 0, "empty gzip returned 0");

  if (gz_len > 0) {
    uint8_t decomp[64];
    size_t decomp_len = fio_gzip_decompress(decomp, sizeof(decomp), gz, gz_len);
    TEST_ASSERT(decomp_len == 0,
                "empty gunzip: expected 0, got %zu",
                decomp_len);
  }
}

static void test_edge_single_byte(void) {
  fprintf(stderr, "Testing edge case: single byte...\n");
  uint8_t input = 0x42;

  size_t comp_bound = fio_deflate_compress_bound(1);
  uint8_t comp[64];
  size_t comp_len = fio_deflate_compress(comp, sizeof(comp), &input, 1, 6);
  TEST_ASSERT(comp_len > 0, "single byte deflate returned 0");

  if (comp_len > 0) {
    uint8_t decomp[64];
    size_t decomp_len =
        fio_deflate_decompress(decomp, sizeof(decomp), comp, comp_len);
    TEST_ASSERT(decomp_len == 1,
                "single byte: expected 1, got %zu",
                decomp_len);
    TEST_ASSERT(decomp_len == 1 && decomp[0] == 0x42,
                "single byte: data mismatch");
  }
  (void)comp_bound;
}

static void test_edge_large_input(void) {
  fprintf(stderr, "Testing edge case: large input (64KB)...\n");
  const size_t data_len = 65536;
  uint8_t *data = (uint8_t *)malloc(data_len);
  if (!data)
    return;

  /* Fill with pseudo-random but compressible data */
  uint32_t seed = 12345;
  for (size_t i = 0; i < data_len; ++i) {
    seed = seed * 1103515245 + 12345;
    data[i] = (uint8_t)((seed >> 16) & 0xFF);
  }
  /* Add some repetitive sections */
  for (size_t i = 0; i < 4096; ++i)
    data[32768 + i] = data[i];

  size_t comp_bound = fio_deflate_compress_bound(data_len);
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  uint8_t *decompressed = (uint8_t *)malloc(data_len + 256);

  if (!compressed || !decompressed) {
    free(data);
    free(compressed);
    free(decompressed);
    return;
  }

  size_t comp_len =
      fio_deflate_compress(compressed, comp_bound, data, data_len, 6);
  TEST_ASSERT(comp_len > 0, "large: deflate returned 0");

  if (comp_len > 0) {
    size_t decomp_len = fio_deflate_decompress(decompressed,
                                               data_len + 256,
                                               compressed,
                                               comp_len);
    TEST_ASSERT(decomp_len == data_len,
                "large: expected %zu, got %zu",
                data_len,
                decomp_len);
    TEST_ASSERT(decomp_len == data_len && !memcmp(decompressed, data, data_len),
                "large: data mismatch");
    fprintf(stderr,
            "  Large: %zu -> %zu bytes (%.1f%%)\n",
            data_len,
            comp_len,
            100.0 * (double)comp_len / (double)data_len);
  }

  free(data);
  free(compressed);
  free(decompressed);
}

static void test_edge_incompressible(void) {
  fprintf(stderr, "Testing edge case: incompressible data...\n");
  const size_t data_len = 1024;
  uint8_t *data = (uint8_t *)malloc(data_len);
  if (!data)
    return;

  /* Fill with random-looking data */
  for (size_t i = 0; i < data_len; ++i)
    data[i] = (uint8_t)(i * 251 + 17);

  size_t comp_bound = fio_deflate_compress_bound(data_len);
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  uint8_t *decompressed = (uint8_t *)malloc(data_len + 256);

  if (!compressed || !decompressed) {
    free(data);
    free(compressed);
    free(decompressed);
    return;
  }

  /* Level 0 (store) should work for incompressible data */
  size_t comp_len =
      fio_deflate_compress(compressed, comp_bound, data, data_len, 0);
  TEST_ASSERT(comp_len > 0, "incompressible: store deflate returned 0");

  if (comp_len > 0) {
    size_t decomp_len = fio_deflate_decompress(decompressed,
                                               data_len + 256,
                                               compressed,
                                               comp_len);
    TEST_ASSERT(decomp_len == data_len,
                "incompressible: expected %zu, got %zu",
                data_len,
                decomp_len);
    TEST_ASSERT(decomp_len == data_len && !memcmp(decompressed, data, data_len),
                "incompressible: data mismatch");
  }

  free(data);
  free(compressed);
  free(decompressed);
}

/* *****************************************************************************
Test: streaming API
***************************************************************************** */

static void test_streaming_compress_decompress(void) {
  fprintf(stderr, "Testing streaming API...\n");
  const char *input = "Hello World! This is a streaming test. "
                      "The quick brown fox jumps over the lazy dog. "
                      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
  size_t in_len = strlen(input);

  /* Create compressor */
  fio_deflate_s *comp = fio_deflate_new(6, 1);
  TEST_ASSERT(comp != NULL, "streaming: fio_deflate_new(compress) failed");
  if (!comp)
    return;

  /* Compress */
  size_t comp_bound = fio_deflate_compress_bound(in_len) + 16;
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  if (!compressed) {
    fio_deflate_free(comp);
    return;
  }

  size_t comp_len =
      fio_deflate_push(comp, compressed, comp_bound, input, in_len, 1);
  TEST_ASSERT(comp_len > 4,
              "streaming: compress returned %zu (need >4)",
              comp_len);

  /* Create decompressor */
  fio_deflate_s *decomp = fio_deflate_new(0, 0);
  TEST_ASSERT(decomp != NULL, "streaming: fio_deflate_new(decompress) failed");

  if (decomp && comp_len > 4) {
    uint8_t *decompressed = (uint8_t *)malloc(in_len + 256);
    if (decompressed) {
      /* Strip trailing sync marker (00 00 FF FF), decompressor re-appends */
      size_t decomp_len = fio_deflate_push(decomp,
                                           decompressed,
                                           in_len + 256,
                                           compressed,
                                           comp_len - 4,
                                           1);
      TEST_ASSERT(decomp_len == in_len,
                  "streaming: expected %zu, got %zu",
                  in_len,
                  decomp_len);
      TEST_ASSERT(decomp_len == in_len && !memcmp(decompressed, input, in_len),
                  "streaming: data mismatch");
      free(decompressed);
    }
  }

  free(compressed);
  fio_deflate_free(comp);
  fio_deflate_free(decomp);
}

/* *****************************************************************************
Test: bounds functions
***************************************************************************** */

static void test_bounds(void) {
  fprintf(stderr, "Testing bound functions...\n");

  TEST_ASSERT(fio_deflate_decompress_bound(0) >= 4096,
              "inflate_bound(0) should be >= 4096");
  TEST_ASSERT(fio_deflate_decompress_bound(100) >= 100,
              "inflate_bound(100) should be >= 100");
  TEST_ASSERT(fio_deflate_compress_bound(0) >= 16,
              "deflate_bound(0) should be >= 16");
  TEST_ASSERT(fio_deflate_compress_bound(65535) >= 65535,
              "deflate_bound(65535) should be >= 65535");
}

/* *****************************************************************************
Test: NULL/invalid inputs
***************************************************************************** */

static void test_null_inputs(void) {
  fprintf(stderr, "Testing NULL/invalid inputs...\n");

  TEST_ASSERT(fio_deflate_decompress(NULL, 100, "x", 1) == 0,
              "inflate with NULL out should return 0");
  uint8_t buf[64];
  TEST_ASSERT(fio_deflate_decompress(buf, sizeof(buf), NULL, 0) == 0,
              "inflate with NULL in should return 0");
  TEST_ASSERT(fio_deflate_compress(NULL, 100, "x", 1, 6) == 0,
              "deflate with NULL out should return 0");
  TEST_ASSERT(fio_gzip_compress(NULL, 100, "x", 1, 6) == 0,
              "gzip with NULL out should return 0");
  TEST_ASSERT(fio_gzip_decompress(buf, sizeof(buf), NULL, 0) == 0,
              "gunzip with NULL in should return 0");

  /* Invalid gzip data */
  uint8_t bad_gz[] = {0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00};
  TEST_ASSERT(fio_gzip_decompress(buf, sizeof(buf), bad_gz, sizeof(bad_gz)) ==
                  0,
              "gunzip with bad magic should return 0");

  /* Streaming API NULL checks */
  TEST_ASSERT(fio_deflate_push(NULL, buf, sizeof(buf), "x", 1, 0) == 0,
              "push with NULL state should return 0");
  fio_deflate_free(NULL); /* should not crash */
}

/* *****************************************************************************
Test: gzip with larger data
***************************************************************************** */

static void test_gzip_large(void) {
  fprintf(stderr, "Testing gzip with larger data...\n");
  const size_t data_len = 16384;
  uint8_t *data = (uint8_t *)malloc(data_len);
  if (!data)
    return;

  /* Create text-like data */
  const char *words[] = {"the ",
                         "quick ",
                         "brown ",
                         "fox ",
                         "jumps ",
                         "over ",
                         "lazy ",
                         "dog ",
                         "and ",
                         "cat "};
  size_t pos = 0;
  size_t word_idx = 0;
  while (pos < data_len) {
    const char *w = words[word_idx % 10];
    size_t wlen = strlen(w);
    if (pos + wlen > data_len)
      wlen = data_len - pos;
    memcpy(data + pos, w, wlen);
    pos += wlen;
    word_idx++;
  }

  size_t gz_bound = fio_deflate_compress_bound(data_len) + 18;
  uint8_t *gz_data = (uint8_t *)malloc(gz_bound);
  uint8_t *decompressed = (uint8_t *)malloc(data_len + 256);

  if (!gz_data || !decompressed) {
    free(data);
    free(gz_data);
    free(decompressed);
    return;
  }

  size_t gz_len = fio_gzip_compress(gz_data, gz_bound, data, data_len, 6);
  TEST_ASSERT(gz_len > 0, "gzip large: returned 0");

  if (gz_len > 0) {
    size_t decomp_len =
        fio_gzip_decompress(decompressed, data_len + 256, gz_data, gz_len);
    TEST_ASSERT(decomp_len == data_len,
                "gzip large: expected %zu, got %zu",
                data_len,
                decomp_len);
    TEST_ASSERT(decomp_len == data_len && !memcmp(decompressed, data, data_len),
                "gzip large: data mismatch");
    fprintf(stderr,
            "  Gzip large: %zu -> %zu bytes (%.1f%%)\n",
            data_len,
            gz_len,
            100.0 * (double)gz_len / (double)data_len);
  }

  free(data);
  free(gz_data);
  free(decompressed);
}

/* *****************************************************************************
Test: fixed vs dynamic Huffman selection
***************************************************************************** */

static void test_fixed_huffman_selection(void) {
  fprintf(stderr, "Testing fixed vs dynamic Huffman selection...\n");

  /* Small input: fixed Huffman should be chosen (dynamic header overhead
   * exceeds any savings from custom codes). Verify by checking that the
   * compressed output uses BTYPE=01 (fixed) in the block header. */
  {
    const char *small = "Hello!";
    size_t in_len = strlen(small);
    uint8_t comp[128];
    size_t comp_len =
        fio_deflate_compress(comp, sizeof(comp), small, in_len, 6);
    TEST_ASSERT(comp_len > 0, "fixed_huff small: deflate returned 0");

    /* Check BTYPE: first byte bit 1-2 (after BFINAL at bit 0).
     * BTYPE=01 (fixed) means bits 1-2 = 01 → byte & 0x06 == 0x02 */
    if (comp_len > 0) {
      uint8_t btype = (comp[0] >> 1) & 0x03;
      TEST_ASSERT(btype == 1,
                  "fixed_huff small: expected BTYPE=01 (fixed), got %u",
                  btype);
    }

    /* Verify round-trip */
    if (comp_len > 0) {
      uint8_t decomp[128];
      size_t decomp_len =
          fio_deflate_decompress(decomp, sizeof(decomp), comp, comp_len);
      TEST_ASSERT(decomp_len == in_len,
                  "fixed_huff small: expected %zu, got %zu",
                  in_len,
                  decomp_len);
      TEST_ASSERT(decomp_len == in_len && !memcmp(decomp, small, in_len),
                  "fixed_huff small: data mismatch");
    }

    fprintf(stderr,
            "  Small (%zu bytes): compressed to %zu bytes\n",
            in_len,
            comp_len);
  }

  /* Medium input with varied symbols: dynamic Huffman should be chosen
   * (custom codes save more than the header overhead). */
  {
    const size_t data_len = 4096;
    uint8_t *data = (uint8_t *)malloc(data_len);
    if (!data)
      return;
    /* Text-like data with repetitive patterns */
    const char *phrase = "the quick brown fox jumps over the lazy dog ";
    size_t plen = strlen(phrase);
    for (size_t i = 0; i < data_len; i += plen) {
      size_t n = plen;
      if (i + n > data_len)
        n = data_len - i;
      memcpy(data + i, phrase, n);
    }

    size_t comp_bound = fio_deflate_compress_bound(data_len);
    uint8_t *comp = (uint8_t *)malloc(comp_bound);
    if (!comp) {
      free(data);
      return;
    }

    size_t comp_len = fio_deflate_compress(comp, comp_bound, data, data_len, 6);
    TEST_ASSERT(comp_len > 0, "fixed_huff medium: deflate returned 0");

    /* For 4KB text, dynamic should be chosen (BTYPE=10, bits 1-2 = 10) */
    if (comp_len > 0) {
      uint8_t btype = (comp[0] >> 1) & 0x03;
      TEST_ASSERT(btype == 2,
                  "fixed_huff medium: expected BTYPE=10 (dynamic), got %u",
                  btype);
    }

    /* Verify round-trip */
    if (comp_len > 0) {
      uint8_t *decomp = (uint8_t *)malloc(data_len + 256);
      if (decomp) {
        size_t decomp_len =
            fio_deflate_decompress(decomp, data_len + 256, comp, comp_len);
        TEST_ASSERT(decomp_len == data_len,
                    "fixed_huff medium: expected %zu, got %zu",
                    data_len,
                    decomp_len);
        TEST_ASSERT(decomp_len == data_len && !memcmp(decomp, data, data_len),
                    "fixed_huff medium: data mismatch");
        free(decomp);
      }
    }

    fprintf(stderr,
            "  Medium (%zu bytes): compressed to %zu bytes\n",
            data_len,
            comp_len);

    free(data);
    free(comp);
  }

  /* Gzip with small input: verify fixed Huffman works through gzip wrapper */
  {
    const char *small = "Short gzip test";
    size_t in_len = strlen(small);
    uint8_t gz[256];
    size_t gz_len = fio_gzip_compress(gz, sizeof(gz), small, in_len, 6);
    TEST_ASSERT(gz_len > 0, "fixed_huff gzip: returned 0");

    if (gz_len > 0) {
      uint8_t decomp[256];
      size_t decomp_len =
          fio_gzip_decompress(decomp, sizeof(decomp), gz, gz_len);
      TEST_ASSERT(decomp_len == in_len,
                  "fixed_huff gzip: expected %zu, got %zu",
                  in_len,
                  decomp_len);
      TEST_ASSERT(decomp_len == in_len && !memcmp(decomp, small, in_len),
                  "fixed_huff gzip: data mismatch");
      fprintf(stderr,
              "  Gzip small (%zu bytes): compressed to %zu bytes\n",
              in_len,
              gz_len);
    }
  }
}

/* *****************************************************************************
Test: multi-push streaming compress roundtrip
***************************************************************************** */

static void test_streaming_multi_push(void) {
  fprintf(stderr, "Testing streaming: multi-push compress roundtrip...\n");

  const char *chunks[] = {
      "Hello, World! This is chunk one. ",
      "The quick brown fox jumps over the lazy dog. ",
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
      "Final chunk with some more data to compress efficiently.",
  };
  const int num_chunks = 4;

  /* Compute total input size */
  size_t total_in = 0;
  for (int i = 0; i < num_chunks; ++i)
    total_in += strlen(chunks[i]);

  /* Build reference (all chunks concatenated) */
  uint8_t *reference = (uint8_t *)malloc(total_in);
  TEST_ASSERT(reference != NULL, "multi-push: malloc reference failed");
  if (!reference)
    return;
  {
    size_t off = 0;
    for (int i = 0; i < num_chunks; ++i) {
      size_t clen = strlen(chunks[i]);
      memcpy(reference + off, chunks[i], clen);
      off += clen;
    }
  }

  /* Compress: push all chunks without flush, then flush */
  fio_deflate_s *comp = fio_deflate_new(6, 1);
  TEST_ASSERT(comp != NULL, "multi-push: fio_deflate_new(compress) failed");
  if (!comp) {
    free(reference);
    return;
  }

  size_t comp_bound = fio_deflate_compress_bound(total_in) + 64;
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  TEST_ASSERT(compressed != NULL, "multi-push: malloc compressed failed");
  if (!compressed) {
    fio_deflate_free(comp);
    free(reference);
    return;
  }

  /* Push chunks without flush (should buffer, return 0) */
  for (int i = 0; i < num_chunks - 1; ++i) {
    size_t r = fio_deflate_push(comp,
                                compressed,
                                comp_bound,
                                chunks[i],
                                strlen(chunks[i]),
                                0);
    TEST_ASSERT(r == 0,
                "multi-push: push chunk %d without flush should return 0, "
                "got %zu",
                i,
                r);
  }

  /* Push last chunk with flush=1 */
  size_t comp_len = fio_deflate_push(comp,
                                     compressed,
                                     comp_bound,
                                     chunks[num_chunks - 1],
                                     strlen(chunks[num_chunks - 1]),
                                     1);
  TEST_ASSERT(comp_len > 0, "multi-push: flush compress returned 0");

  /* Decompress: strip trailing 4 bytes (sync marker tail), push with flush */
  fio_deflate_s *decomp = fio_deflate_new(0, 0);
  TEST_ASSERT(decomp != NULL, "multi-push: fio_deflate_new(decompress) failed");

  if (decomp && comp_len > 4) {
    uint8_t *decompressed = (uint8_t *)malloc(total_in + 256);
    TEST_ASSERT(decompressed != NULL, "multi-push: malloc decompressed failed");
    if (decompressed) {
      /* Strip trailing 00 00 FF FF from compressed output */
      size_t stripped_len = comp_len - 4;
      size_t decomp_len = fio_deflate_push(decomp,
                                           decompressed,
                                           total_in + 256,
                                           compressed,
                                           stripped_len,
                                           1);
      TEST_ASSERT(decomp_len == total_in,
                  "multi-push: expected %zu, got %zu",
                  total_in,
                  decomp_len);
      TEST_ASSERT(decomp_len == total_in &&
                      !memcmp(decompressed, reference, total_in),
                  "multi-push: data mismatch");
      fprintf(stderr,
              "  Multi-push: %zu -> %zu bytes (%.1f%%)\n",
              total_in,
              comp_len,
              100.0 * (double)comp_len / (double)total_in);
      free(decompressed);
    }
  }

  free(compressed);
  free(reference);
  fio_deflate_free(comp);
  fio_deflate_free(decomp);
}

/* *****************************************************************************
Test: context takeover (cross-message back-references)
***************************************************************************** */

static void test_streaming_context_takeover(void) {
  fprintf(stderr, "Testing streaming: context takeover...\n");

  /* Message A: establish context with repetitive data */
  const char *msg_a = "the quick brown fox jumps over the lazy dog. "
                      "the quick brown fox jumps over the lazy dog. "
                      "the quick brown fox jumps over the lazy dog. "
                      "the quick brown fox jumps over the lazy dog. ";
  size_t msg_a_len = strlen(msg_a);

  /* Message B: same content — should compress much better with context */
  const char *msg_b = "the quick brown fox jumps over the lazy dog. "
                      "the quick brown fox jumps over the lazy dog. ";
  size_t msg_b_len = strlen(msg_b);

  size_t comp_bound = fio_deflate_compress_bound(msg_a_len) + 64;
  uint8_t *comp_a = (uint8_t *)malloc(comp_bound);
  uint8_t *comp_b = (uint8_t *)malloc(comp_bound);
  uint8_t *comp_b_nocontext = (uint8_t *)malloc(comp_bound);
  TEST_ASSERT(comp_a && comp_b && comp_b_nocontext,
              "context_takeover: malloc failed");
  if (!comp_a || !comp_b || !comp_b_nocontext) {
    free(comp_a);
    free(comp_b);
    free(comp_b_nocontext);
    return;
  }

  /* Compress msg_a with context, then msg_b with same context */
  fio_deflate_s *comp = fio_deflate_new(6, 1);
  TEST_ASSERT(comp != NULL, "context_takeover: fio_deflate_new failed");
  if (!comp) {
    free(comp_a);
    free(comp_b);
    free(comp_b_nocontext);
    return;
  }

  size_t comp_a_len =
      fio_deflate_push(comp, comp_a, comp_bound, msg_a, msg_a_len, 1);
  TEST_ASSERT(comp_a_len > 0, "context_takeover: compress msg_a returned 0");

  size_t comp_b_len =
      fio_deflate_push(comp, comp_b, comp_bound, msg_b, msg_b_len, 1);
  TEST_ASSERT(comp_b_len > 0, "context_takeover: compress msg_b returned 0");

  /* Compress msg_b WITHOUT context (fresh compressor) */
  fio_deflate_s *comp_fresh = fio_deflate_new(6, 1);
  size_t comp_b_nocontext_len = 0;
  if (comp_fresh) {
    comp_b_nocontext_len = fio_deflate_push(comp_fresh,
                                            comp_b_nocontext,
                                            comp_bound,
                                            msg_b,
                                            msg_b_len,
                                            1);
    TEST_ASSERT(comp_b_nocontext_len > 0,
                "context_takeover: compress msg_b (no context) returned 0");
    fio_deflate_free(comp_fresh);
  }

  /* Context takeover should produce smaller output for msg_b */
  if (comp_b_len > 0 && comp_b_nocontext_len > 0) {
    TEST_ASSERT(comp_b_len <= comp_b_nocontext_len,
                "context_takeover: msg_b with context (%zu) should be <= "
                "without context (%zu)",
                comp_b_len,
                comp_b_nocontext_len);
    fprintf(stderr,
            "  msg_b with context: %zu bytes, without: %zu bytes\n",
            comp_b_len,
            comp_b_nocontext_len);
  }

  /* Decompress both messages and verify */
  fio_deflate_s *decomp = fio_deflate_new(0, 0);
  TEST_ASSERT(decomp != NULL,
              "context_takeover: fio_deflate_new(decomp) failed");

  if (decomp && comp_a_len > 4 && comp_b_len > 4) {
    uint8_t *out_a = (uint8_t *)malloc(msg_a_len + 256);
    uint8_t *out_b = (uint8_t *)malloc(msg_b_len + 256);
    if (out_a && out_b) {
      size_t dec_a_len = fio_deflate_push(decomp,
                                          out_a,
                                          msg_a_len + 256,
                                          comp_a,
                                          comp_a_len - 4,
                                          1);
      TEST_ASSERT(dec_a_len == msg_a_len,
                  "context_takeover: decompress msg_a expected %zu, got %zu",
                  msg_a_len,
                  dec_a_len);
      TEST_ASSERT(dec_a_len == msg_a_len && !memcmp(out_a, msg_a, msg_a_len),
                  "context_takeover: msg_a data mismatch");

      size_t dec_b_len = fio_deflate_push(decomp,
                                          out_b,
                                          msg_b_len + 256,
                                          comp_b,
                                          comp_b_len - 4,
                                          1);
      TEST_ASSERT(dec_b_len == msg_b_len,
                  "context_takeover: decompress msg_b expected %zu, got %zu",
                  msg_b_len,
                  dec_b_len);
      TEST_ASSERT(dec_b_len == msg_b_len && !memcmp(out_b, msg_b, msg_b_len),
                  "context_takeover: msg_b data mismatch");
    }
    free(out_a);
    free(out_b);
  }

  free(comp_a);
  free(comp_b);
  free(comp_b_nocontext);
  fio_deflate_free(comp);
  fio_deflate_free(decomp);
}

/* *****************************************************************************
Test: WebSocket-style (strip trailing 4 bytes, re-append on decompress)
***************************************************************************** */

static void test_streaming_websocket_style(void) {
  fprintf(stderr, "Testing streaming: WebSocket permessage-deflate style...\n");

  const char *messages[] = {
      "Hello WebSocket!",
      "This is message two with some repeated content.",
      "Hello WebSocket! Again with the same greeting.",
  };
  const int num_msgs = 3;

  fio_deflate_s *comp = fio_deflate_new(6, 1);
  fio_deflate_s *decomp = fio_deflate_new(0, 0);
  TEST_ASSERT(comp != NULL, "ws_style: fio_deflate_new(compress) failed");
  TEST_ASSERT(decomp != NULL, "ws_style: fio_deflate_new(decompress) failed");
  if (!comp || !decomp) {
    fio_deflate_free(comp);
    fio_deflate_free(decomp);
    return;
  }

  size_t comp_bound = 4096;
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  uint8_t *decompressed = (uint8_t *)malloc(comp_bound);
  TEST_ASSERT(compressed && decompressed, "ws_style: malloc failed");
  if (!compressed || !decompressed) {
    free(compressed);
    free(decompressed);
    fio_deflate_free(comp);
    fio_deflate_free(decomp);
    return;
  }

  for (int i = 0; i < num_msgs; ++i) {
    size_t msg_len = strlen(messages[i]);

    /* Compress with flush */
    size_t comp_len =
        fio_deflate_push(comp, compressed, comp_bound, messages[i], msg_len, 1);
    TEST_ASSERT(comp_len > 4,
                "ws_style msg %d: compress returned %zu (need >4)",
                i,
                comp_len);
    if (comp_len <= 4)
      continue;

    /* Verify trailing sync marker: 00 00 FF FF */
    TEST_ASSERT(compressed[comp_len - 4] == 0x00 &&
                    compressed[comp_len - 3] == 0x00 &&
                    compressed[comp_len - 2] == 0xFF &&
                    compressed[comp_len - 1] == 0xFF,
                "ws_style msg %d: missing trailing sync marker",
                i);

    /* WebSocket sender strips trailing 4 bytes */
    size_t wire_len = comp_len - 4;

    /* WebSocket receiver passes stripped data to decompressor with flush=1
     * (decompressor re-appends the 4-byte sync marker internally) */
    size_t decomp_len = fio_deflate_push(decomp,
                                         decompressed,
                                         comp_bound,
                                         compressed,
                                         wire_len,
                                         1);
    TEST_ASSERT(decomp_len == msg_len,
                "ws_style msg %d: expected %zu, got %zu",
                i,
                msg_len,
                decomp_len);
    TEST_ASSERT(decomp_len == msg_len &&
                    !memcmp(decompressed, messages[i], msg_len),
                "ws_style msg %d: data mismatch",
                i);

    fprintf(stderr,
            "  msg[%d]: %zu -> %zu wire bytes (%.1f%%)\n",
            i,
            msg_len,
            wire_len,
            100.0 * (double)wire_len / (double)msg_len);
  }

  free(compressed);
  free(decompressed);
  fio_deflate_free(comp);
  fio_deflate_free(decomp);
}

/* *****************************************************************************
Test: large streaming (256KB in 4KB chunks)
***************************************************************************** */

static void test_streaming_large(void) {
  fprintf(stderr, "Testing streaming: large data (256KB in 4KB chunks)...\n");

  const size_t total_len = 256 * 1024;
  const size_t chunk_size = 4096;
  uint8_t *data = (uint8_t *)malloc(total_len);
  TEST_ASSERT(data != NULL, "large_stream: malloc data failed");
  if (!data)
    return;

  /* Fill with text-like repetitive data */
  const char *phrase = "the quick brown fox jumps over the lazy dog ";
  size_t plen = strlen(phrase);
  for (size_t i = 0; i < total_len; i += plen) {
    size_t n = plen;
    if (i + n > total_len)
      n = total_len - i;
    memcpy(data + i, phrase, n);
  }

  fio_deflate_s *comp = fio_deflate_new(6, 1);
  fio_deflate_s *decomp = fio_deflate_new(0, 0);
  TEST_ASSERT(comp != NULL, "large_stream: fio_deflate_new(compress) failed");
  TEST_ASSERT(decomp != NULL,
              "large_stream: fio_deflate_new(decompress) failed");
  if (!comp || !decomp) {
    free(data);
    fio_deflate_free(comp);
    fio_deflate_free(decomp);
    return;
  }

  /* Compress and decompress in chunks, accumulate decompressed output */
  size_t comp_bound = fio_deflate_compress_bound(chunk_size) + 64;
  uint8_t *comp_buf = (uint8_t *)malloc(comp_bound);
  uint8_t *decomp_buf = (uint8_t *)malloc(chunk_size + 4096);
  uint8_t *result = (uint8_t *)malloc(total_len + 4096);
  TEST_ASSERT(comp_buf && decomp_buf && result,
              "large_stream: malloc buffers failed");
  if (!comp_buf || !decomp_buf || !result) {
    free(data);
    free(comp_buf);
    free(decomp_buf);
    free(result);
    fio_deflate_free(comp);
    fio_deflate_free(decomp);
    return;
  }

  size_t total_decomp = 0;
  size_t total_comp = 0;
  int ok = 1;

  for (size_t off = 0; off < total_len; off += chunk_size) {
    size_t clen = chunk_size;
    if (off + clen > total_len)
      clen = total_len - off;

    /* Compress chunk with flush */
    size_t comp_len =
        fio_deflate_push(comp, comp_buf, comp_bound, data + off, clen, 1);
    if (!comp_len) {
      TEST_ASSERT(0,
                  "large_stream: compress chunk at offset %zu returned 0",
                  off);
      ok = 0;
      break;
    }
    total_comp += comp_len;

    /* Strip trailing sync marker, decompress with flush */
    if (comp_len <= 4) {
      TEST_ASSERT(
          0,
          "large_stream: compressed chunk too small (%zu) at offset %zu",
          comp_len,
          off);
      ok = 0;
      break;
    }
    size_t wire_len = comp_len - 4;
    size_t decomp_len = fio_deflate_push(decomp,
                                         decomp_buf,
                                         chunk_size + 4096,
                                         comp_buf,
                                         wire_len,
                                         1);
    if (decomp_len != clen) {
      TEST_ASSERT(0,
                  "large_stream: chunk at offset %zu: expected %zu, got %zu",
                  off,
                  clen,
                  decomp_len);
      ok = 0;
      break;
    }

    memcpy(result + total_decomp, decomp_buf, decomp_len);
    total_decomp += decomp_len;
  }

  if (ok) {
    TEST_ASSERT(total_decomp == total_len,
                "large_stream: total expected %zu, got %zu",
                total_len,
                total_decomp);
    TEST_ASSERT(total_decomp == total_len && !memcmp(result, data, total_len),
                "large_stream: data mismatch");
    fprintf(stderr,
            "  Large stream: %zu -> %zu total compressed bytes (%.1f%%)\n",
            total_len,
            total_comp,
            100.0 * (double)total_comp / (double)total_len);
  }

  free(data);
  free(comp_buf);
  free(decomp_buf);
  free(result);
  fio_deflate_free(comp);
  fio_deflate_free(decomp);
}

/* *****************************************************************************
Test: 16MB roundtrip corruption test
***************************************************************************** */

static void test_deflate_16mb_roundtrip(void) {
  fprintf(stderr, "Testing 16MB roundtrip corruption test...\n");

  const size_t target_size = 16 * 1024 * 1024;
  uint8_t *data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, target_size + 4096, 0);
  TEST_ASSERT(data != NULL, "16mb: malloc data failed");
  if (!data)
    return;

  /* Generate 16MB of repetitive HTML/JSON data with varying content */
  size_t pos = 0;
  for (size_t i = 0; pos < target_size; ++i) {
    char block[512];
    int n;
    if (i & 1) {
      /* JSON block */
      n = snprintf(
          block,
          sizeof(block),
          "{\"id\":%zu,\"name\":\"Item %zu\","
          "\"description\":\"A longer description for item number %zu\","
          "\"price\":%zu.%02zu,"
          "\"tags\":[\"tag-a\",\"tag-b\",\"tag-c\"],"
          "\"active\":true},\n",
          i,
          i,
          i,
          (i * 17 + 3) % 1000,
          (i * 31) % 100);
    } else {
      /* HTML block */
      n = snprintf(block,
                   sizeof(block),
                   "<div class=\"item-%zu\"><h2>Title %zu</h2>"
                   "<p>Description text for item %zu with some "
                   "varying content.</p>"
                   "<span class=\"price\">$%zu.%02zu</span></div>\n",
                   i,
                   i,
                   i,
                   (i * 17 + 3) % 1000,
                   (i * 31) % 100);
    }
    if (n <= 0)
      break;
    size_t blen = (size_t)n;
    if (pos + blen > target_size)
      blen = target_size - pos;
    FIO_MEMCPY(data + pos, block, blen);
    pos += blen;
  }
  size_t data_len = pos;

  fprintf(stderr,
          "  Generated %zu bytes (%.1f MB) of test data\n",
          data_len,
          (double)data_len / (1024.0 * 1024.0));

  int levels[] = {1, 6, 9};
  for (int li = 0; li < 3; ++li) {
    int level = levels[li];

    /* Compress */
    size_t comp_bound = fio_deflate_compress_bound(data_len);
    uint8_t *compressed = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, comp_bound, 0);
    TEST_ASSERT(compressed != NULL,
                "16mb level %d: malloc compressed failed",
                level);
    if (!compressed)
      continue;

    size_t comp_len =
        fio_deflate_compress(compressed, comp_bound, data, data_len, level);
    TEST_ASSERT(comp_len > 0, "16mb level %d: deflate returned 0", level);
    TEST_ASSERT(comp_len < data_len,
                "16mb level %d: compressed %zu >= original %zu",
                level,
                comp_len,
                data_len);

    if (comp_len > 0 && comp_len < data_len) {
      /* Decompress — use generous buffer (2x original) */
      size_t decomp_buf_size = data_len * 2;
      uint8_t *decompressed =
          (uint8_t *)FIO_MEM_REALLOC(NULL, 0, decomp_buf_size, 0);
      TEST_ASSERT(decompressed != NULL,
                  "16mb level %d: malloc decompressed failed",
                  level);
      if (decompressed) {
        size_t decomp_len = fio_deflate_decompress(decompressed,
                                                   decomp_buf_size,
                                                   compressed,
                                                   comp_len);
        TEST_ASSERT(decomp_len == data_len,
                    "16mb level %d: decompressed %zu, expected %zu",
                    level,
                    decomp_len,
                    data_len);
        int match = (decomp_len == data_len) &&
                    (FIO_MEMCMP(decompressed, data, data_len) == 0);
        TEST_ASSERT(match, "16mb level %d: data corruption detected", level);
        fprintf(stderr,
                "  Level %d: %zu -> %zu bytes (%.2f%%) — %s\n",
                level,
                data_len,
                comp_len,
                100.0 * (double)comp_len / (double)data_len,
                match ? "ROUNDTRIP OK" : "CORRUPTION DETECTED");
        FIO_MEM_FREE(decompressed, decomp_buf_size);
      }
    }

    FIO_MEM_FREE(compressed, comp_bound);
  }

  FIO_MEM_FREE(data, target_size + 4096);
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr, "=== DEFLATE/INFLATE Test Suite ===\n\n");

  test_inflate_stored_block();
  test_roundtrip_simple();
  test_roundtrip_levels();
  test_roundtrip_repetitive();
  test_gzip_roundtrip();
  test_gzip_large();
  test_edge_empty_input();
  test_edge_single_byte();
  test_edge_large_input();
  test_edge_incompressible();
  test_streaming_compress_decompress();
  test_bounds();
  test_null_inputs();
  test_fixed_huffman_selection();
  test_streaming_multi_push();
  test_streaming_context_takeover();
  test_streaming_websocket_style();
  test_streaming_large();
  test_deflate_16mb_roundtrip();

  fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
