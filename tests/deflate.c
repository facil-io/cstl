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

  size_t comp_len = fio_deflate_compress(compressed, comp_bound, input, in_len, 6);
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

  size_t comp_len = fio_deflate_compress(compressed, comp_bound, data, data_len, 6);
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

  size_t comp_len = fio_deflate_compress(compressed, comp_bound, data, data_len, 6);
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
  size_t comp_len = fio_deflate_compress(compressed, comp_bound, data, data_len, 0);
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
  TEST_ASSERT(comp_len > 0, "streaming: compress returned 0");

  /* Create decompressor */
  fio_deflate_s *decomp = fio_deflate_new(0, 0);
  TEST_ASSERT(decomp != NULL, "streaming: fio_deflate_new(decompress) failed");

  if (decomp && comp_len > 0) {
    uint8_t *decompressed = (uint8_t *)malloc(in_len + 256);
    if (decompressed) {
      size_t decomp_len = fio_deflate_push(decomp,
                                           decompressed,
                                           in_len + 256,
                                           compressed,
                                           comp_len,
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

  fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
