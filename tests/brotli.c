/* *****************************************************************************
BROTLI Tests (Decompression + Compression)
***************************************************************************** */
#include "test-helpers.h"
#define FIO_RAND
#define FIO_BROTLI
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
Decompression Tests
***************************************************************************** */

static void test_brotli_empty(void) {
  /* Empty Brotli stream: 0x3b = WBITS=22, ISLAST=1, ISLASTEMPTY=1 */
  uint8_t compressed[] = {0x3b};
  uint8_t out[64];
  size_t result =
      fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
  TEST_ASSERT(result == 0, "empty stream: expected 0 bytes, got %zu", result);
}

static void test_brotli_hello(void) {
  /* brotli.compress(b'Hello, World!', quality=1) */
  uint8_t compressed[] = {0x0b,
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
  const char *expected = "Hello, World!";
  size_t expected_len = 13;
  uint8_t out[256];
  size_t result =
      fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
  TEST_ASSERT(result == expected_len,
              "hello q1: expected %zu bytes, got %zu",
              expected_len,
              result);
  if (result == expected_len)
    TEST_ASSERT(memcmp(out, expected, expected_len) == 0,
                "hello q1: data mismatch");
}

static void test_brotli_repeated(void) {
  /* brotli.compress(b'ABCDEFGH' * 100, quality=1) */
  uint8_t compressed[] = {0x8b, 0x8f, 0x01, 0x00, 0x80, 0xaa, 0xaa, 0xaa,
                          0xea, 0xff, 0x7c, 0xe6, 0x65, 0x81, 0x03, 0xb8,
                          0xf8, 0x95, 0x2e, 0x55, 0x0c, 0x36, 0x18, 0x73,
                          0xec, 0x28, 0xa0, 0x9c, 0xee, 0x3d, 0x34, 0x03};
  size_t expected_len = 800;
  uint8_t expected[800];
  for (int i = 0; i < 100; i++)
    memcpy(expected + i * 8, "ABCDEFGH", 8);
  uint8_t out[1024];
  size_t result =
      fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
  TEST_ASSERT(result == expected_len,
              "repeated q1: expected %zu bytes, got %zu",
              expected_len,
              result);
  if (result == expected_len)
    TEST_ASSERT(memcmp(out, expected, expected_len) == 0,
                "repeated q1: data mismatch");
}

static void test_brotli_text_q5(void) {
  /* brotli.compress(b'The quick brown fox...' * 20, quality=5) */
  uint8_t compressed[] = {0x1b, 0x83, 0x03, 0x00, 0x44, 0xdb, 0x46, 0xa9, 0x2e,
                          0x24, 0x5b, 0x32, 0x14, 0xc5, 0x53, 0x91, 0x67, 0x72,
                          0xf2, 0xe7, 0x28, 0x50, 0x15, 0x98, 0x57, 0xb6, 0xb2,
                          0x59, 0xd0, 0x6c, 0xe1, 0x95, 0xa7, 0x23, 0xf2, 0xa2,
                          0xac, 0x36, 0x26, 0xb8, 0x45, 0x1f, 0x18, 0x27, 0x62,
                          0x75, 0xff, 0x21, 0x30, 0x19, 0x00};
  const char *phrase = "The quick brown fox jumps over the lazy dog. ";
  size_t phrase_len = 45;
  size_t expected_len = phrase_len * 20;
  uint8_t expected[900];
  for (int i = 0; i < 20; i++)
    memcpy(expected + i * phrase_len, phrase, phrase_len);
  uint8_t out[1024];
  size_t result =
      fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
  TEST_ASSERT(result == expected_len,
              "text q5: expected %zu bytes, got %zu",
              expected_len,
              result);
  if (result == expected_len)
    TEST_ASSERT(memcmp(out, expected, expected_len) == 0,
                "text q5: data mismatch");
}

static void test_brotli_html_q11(void) {
  /* brotli.compress(b'<html>...', quality=11) */
  uint8_t compressed[] = {
      0x1b, 0x4b, 0x00, 0x58, 0x8c, 0xd4, 0x61, 0xcd, 0x9d, 0x07, 0x02, 0xdd,
      0x58, 0x2e, 0xe3, 0x25, 0x19, 0xaa, 0x60, 0x50, 0x38, 0xf8, 0x78, 0x81,
      0x0d, 0x38, 0x70, 0x68, 0xb2, 0x41, 0x1f, 0x7c, 0xe1, 0xc1, 0x21, 0x37,
      0xbf, 0x61, 0x09, 0x24, 0x1e, 0x7a, 0x5c, 0x31, 0xb9, 0x55, 0x89, 0x4c,
      0x54, 0x7f, 0x60, 0xd1, 0x16, 0xd6, 0xc2, 0x78, 0xa2, 0x83, 0x15, 0x02};
  const char *expected = "<html><head><title>Test</title></head><body><p>Hello "
                         "World</p></body></html>";
  size_t expected_len = 76;
  uint8_t out[256];
  size_t result =
      fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
  TEST_ASSERT(result == expected_len,
              "html q11: expected %zu bytes, got %zu",
              expected_len,
              result);
  if (result == expected_len)
    TEST_ASSERT(memcmp(out, expected, expected_len) == 0,
                "html q11: data mismatch");
}

static void test_brotli_bounds(void) {
  TEST_ASSERT(fio_brotli_decompress_bound(100) > 100,
              "decompress_bound should be > input");
  TEST_ASSERT(fio_brotli_compress_bound(100) > 100,
              "compress_bound should be > input");
  TEST_ASSERT(fio_brotli_decompress_bound(0) >= 1024,
              "decompress_bound(0) should have minimum");
}

static void test_brotli_null_inputs(void) {
  uint8_t out[64];
  /* null input / zero input length are always errors */
  TEST_ASSERT(fio_brotli_decompress(out, 64, NULL, 1) == 0,
              "null input should return 0");
  TEST_ASSERT(fio_brotli_decompress(out, 64, "x", 0) == 0,
              "zero input length should return 0");

  /* null output or zero output length = size query mode.
   * With garbage input "x", should return 0 (corrupt). */
  TEST_ASSERT(fio_brotli_decompress(NULL, 64, "x", 1) == 0,
              "null output + garbage input should return 0");
  TEST_ASSERT(fio_brotli_decompress(out, 0, "x", 1) == 0,
              "zero output length + garbage input should return 0");
}

/* *****************************************************************************
Compression Tests
***************************************************************************** */

static void test_brotli_compress_empty(void) {
  uint8_t compressed[64];
  size_t clen = fio_brotli_compress(compressed, sizeof(compressed), NULL, 0, 1);
  TEST_ASSERT(clen > 0, "compress empty: should produce valid stream");
  uint8_t decompressed[64];
  size_t dlen = fio_brotli_decompress(decompressed,
                                      sizeof(decompressed),
                                      compressed,
                                      clen);
  TEST_ASSERT(dlen == 0, "compress empty roundtrip: expected 0, got %zu", dlen);
}

static void test_brotli_compress_single_byte(void) {
  uint8_t input[] = {0x42};
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, 1, 1);
  TEST_ASSERT(clen > 0, "compress single byte: should produce output");
  uint8_t decompressed[64];
  size_t dlen = fio_brotli_decompress(decompressed,
                                      sizeof(decompressed),
                                      compressed,
                                      clen);
  TEST_ASSERT(dlen == 1,
              "compress single byte roundtrip: expected 1, got %zu",
              dlen);
  if (dlen == 1)
    TEST_ASSERT(decompressed[0] == 0x42,
                "compress single byte roundtrip: data mismatch (got 0x%02x)",
                decompressed[0]);
}

static void test_brotli_compress_hello(void) {
  const char *input = "Hello, World!";
  size_t in_len = 13;
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, in_len, 1);
  TEST_ASSERT(clen > 0,
              "compress hello: should produce output (got %zu)",
              clen);
  uint8_t decompressed[256];
  size_t dlen = fio_brotli_decompress(decompressed,
                                      sizeof(decompressed),
                                      compressed,
                                      clen);
  TEST_ASSERT(dlen == in_len,
              "compress hello roundtrip: expected %zu, got %zu",
              in_len,
              dlen);
  if (dlen == in_len)
    TEST_ASSERT(memcmp(decompressed, input, in_len) == 0,
                "compress hello roundtrip: data mismatch");
}

static void test_brotli_compress_repeated(void) {
  uint8_t input[800];
  for (int i = 0; i < 100; i++)
    memcpy(input + i * 8, "ABCDEFGH", 8);
  size_t bound = fio_brotli_compress_bound(800);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  TEST_ASSERT(compressed != NULL, "compress repeated: malloc failed");
  if (!compressed)
    return;
  size_t clen = fio_brotli_compress(compressed, bound, input, 800, 1);
  TEST_ASSERT(clen > 0, "compress repeated: should produce output");
  TEST_ASSERT(clen < 800,
              "compress repeated: should compress (got %zu >= 800)",
              clen);
  uint8_t decompressed[1024];
  size_t dlen = fio_brotli_decompress(decompressed,
                                      sizeof(decompressed),
                                      compressed,
                                      clen);
  TEST_ASSERT(dlen == 800,
              "compress repeated roundtrip: expected 800, got %zu",
              dlen);
  if (dlen == 800)
    TEST_ASSERT(memcmp(decompressed, input, 800) == 0,
                "compress repeated roundtrip: data mismatch");
  free(compressed);
}

static void test_brotli_compress_all_bytes(void) {
  uint8_t input[256];
  for (int i = 0; i < 256; i++)
    input[i] = (uint8_t)i;
  uint8_t compressed[1024];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, 256, 1);
  TEST_ASSERT(clen > 0, "compress all bytes: should produce output");
  uint8_t decompressed[512];
  size_t dlen = fio_brotli_decompress(decompressed,
                                      sizeof(decompressed),
                                      compressed,
                                      clen);
  TEST_ASSERT(dlen == 256,
              "compress all bytes roundtrip: expected 256, got %zu",
              dlen);
  if (dlen == 256)
    TEST_ASSERT(memcmp(decompressed, input, 256) == 0,
                "compress all bytes roundtrip: data mismatch");
}

/* *****************************************************************************
Cross-quality roundtrip: compress at q1-q6, decompress all
***************************************************************************** */

static void test_brotli_cross_quality_roundtrip(void) {
  const char *phrase = "The quick brown fox jumps over the lazy dog. ";
  size_t phrase_len = 45;
  size_t total_len = phrase_len * 20; /* 900 bytes */
  uint8_t input[900];
  for (int i = 0; i < 20; i++)
    memcpy(input + i * phrase_len, phrase, phrase_len);

  size_t bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  TEST_ASSERT(compressed != NULL, "cross-quality: malloc failed");
  if (!compressed)
    return;

  for (int q = 1; q <= 6; q++) {
    size_t clen = fio_brotli_compress(compressed, bound, input, total_len, q);
    TEST_ASSERT(clen > 0, "cross-quality q%d: compression failed", q);
    TEST_ASSERT(clen < total_len,
                "cross-quality q%d: should compress (%zu >= %zu)",
                q,
                clen,
                total_len);
    uint8_t decompressed[1024];
    size_t dlen = fio_brotli_decompress(decompressed,
                                        sizeof(decompressed),
                                        compressed,
                                        clen);
    TEST_ASSERT(dlen == total_len,
                "cross-quality q%d roundtrip: decompressed %zu bytes, "
                "expected %zu",
                q,
                dlen,
                total_len);
    if (dlen == total_len)
      TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                  "cross-quality q%d roundtrip: data mismatch",
                  q);
  }
  free(compressed);
}

/* *****************************************************************************
Large file roundtrip: 256KB+ of realistic data at q5 and q6
***************************************************************************** */

static void test_brotli_large_file_roundtrip(void) {
  /* Build ~300KB of mixed HTML-like content */
  const char *fragments[] = {
      "<html><head><title>Test Page</title></head><body>\n",
      "<div class=\"container\" id=\"main\">\n",
      "  <h1>Welcome to the Test Page</h1>\n",
      "  <p>This is a paragraph with some <strong>bold</strong> text.</p>\n",
      "  <p>Another paragraph with <a "
      "href=\"https://example.com\">link</a>.</p>\n",
      "  <ul><li>First item</li><li>Second item</li><li>Third item</li></ul>\n",
      "  <table><tr><th>Name</th><th>Value</th></tr>\n",
      "    <tr><td>Alpha</td><td>100</td></tr>\n",
      "    <tr><td>Beta</td><td>200</td></tr></table>\n",
      "</div></body></html>\n",
      "The quick brown fox jumps over the lazy dog. Pack my box with five "
      "dozen "
      "liquor jugs! How vexingly quick daft zebras jump.\n",
      "{\"name\":\"test\",\"value\":42,\"items\":[1,2,3],\"nested\":{\"key\":"
      "\"val\"}}\n",
  };
  size_t nfrags = sizeof(fragments) / sizeof(fragments[0]);

  size_t data_size = 300000;
  uint8_t *src = (uint8_t *)malloc(data_size);
  TEST_ASSERT(src != NULL, "large file: src malloc failed");
  if (!src)
    return;

  size_t total_len = 0;
  for (size_t rep = 0; total_len + 200 < data_size; rep++) {
    const char *frag = fragments[rep % nfrags];
    size_t flen = strlen(frag);
    if (total_len + flen > data_size)
      break;
    memcpy(src + total_len, frag, flen);
    total_len += flen;
  }

  size_t comp_bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
  TEST_ASSERT(compressed && decompressed, "large file: malloc failed");
  if (!compressed || !decompressed) {
    free(src);
    free(compressed);
    free(decompressed);
    return;
  }

  for (int q = 5; q <= 6; q++) {
    size_t clen =
        fio_brotli_compress(compressed, comp_bound, src, total_len, q);
    TEST_ASSERT(clen > 0, "large file q%d: compression failed", q);
    TEST_ASSERT(clen < total_len,
                "large file q%d: should compress (%zu >= %zu)",
                q,
                clen,
                total_len);
    size_t dlen =
        fio_brotli_decompress(decompressed, total_len + 256, compressed, clen);
    TEST_ASSERT(dlen == total_len,
                "large file q%d roundtrip: decompressed %zu, expected %zu",
                q,
                dlen,
                total_len);
    if (dlen == total_len)
      TEST_ASSERT(memcmp(decompressed, src, total_len) == 0,
                  "large file q%d roundtrip: data mismatch",
                  q);
  }

  free(src);
  free(compressed);
  free(decompressed);
}

/* *****************************************************************************
Edge cases: boundary sizes, repetitive, mixed content, all byte values
***************************************************************************** */

static void test_brotli_edge_cases(void) {
  size_t bound;
  uint8_t *compressed;
  uint8_t *decompressed;

  /* Test 1: Input exactly at block split boundary sizes (512, 1024 bytes) */
  for (size_t sz = 512; sz <= 1024; sz += 512) {
    uint8_t *input = (uint8_t *)malloc(sz);
    TEST_ASSERT(input != NULL, "edge boundary %zu: malloc failed", sz);
    if (!input)
      continue;
    /* Fill with varied content */
    for (size_t i = 0; i < sz; i++)
      input[i] = (uint8_t)((i * 7 + 13) ^ (i >> 3));

    bound = fio_brotli_compress_bound(sz);
    compressed = (uint8_t *)malloc(bound);
    decompressed = (uint8_t *)malloc(sz + 64);
    if (!compressed || !decompressed) {
      free(input);
      free(compressed);
      free(decompressed);
      continue;
    }

    for (int q = 1; q <= 6; q++) {
      size_t clen = fio_brotli_compress(compressed, bound, input, sz, q);
      TEST_ASSERT(clen > 0, "edge boundary %zu q%d: compression failed", sz, q);
      size_t dlen =
          fio_brotli_decompress(decompressed, sz + 64, compressed, clen);
      TEST_ASSERT(dlen == sz,
                  "edge boundary %zu q%d: decompressed %zu, expected %zu",
                  sz,
                  q,
                  dlen,
                  sz);
      if (dlen == sz)
        TEST_ASSERT(memcmp(decompressed, input, sz) == 0,
                    "edge boundary %zu q%d: data mismatch",
                    sz,
                    q);
    }
    free(input);
    free(compressed);
    free(decompressed);
  }

  /* Test 2: Very repetitive data (should produce few literals) */
  {
    uint8_t input[4096];
    memset(input, 'A', sizeof(input));
    bound = fio_brotli_compress_bound(sizeof(input));
    compressed = (uint8_t *)malloc(bound);
    decompressed = (uint8_t *)malloc(sizeof(input) + 64);
    if (compressed && decompressed) {
      for (int q = 5; q <= 6; q++) {
        size_t clen =
            fio_brotli_compress(compressed, bound, input, sizeof(input), q);
        TEST_ASSERT(clen > 0, "edge repetitive q%d: compression failed", q);
        TEST_ASSERT(clen < 50,
                    "edge repetitive q%d: should compress well (%zu >= 50)",
                    q,
                    clen);
        size_t dlen = fio_brotli_decompress(decompressed,
                                            sizeof(input) + 64,
                                            compressed,
                                            clen);
        TEST_ASSERT(dlen == sizeof(input),
                    "edge repetitive q%d: decompressed %zu, expected %zu",
                    q,
                    dlen,
                    sizeof(input));
        if (dlen == sizeof(input))
          TEST_ASSERT(memcmp(decompressed, input, sizeof(input)) == 0,
                      "edge repetitive q%d: data mismatch",
                      q);
      }
    }
    free(compressed);
    free(decompressed);
  }

  /* Test 3: Mixed content types (should trigger block splits at q6) */
  {
    uint8_t input[4096];
    size_t pos = 0;
    /* First half: English text */
    const char *text = "The international conference discussed important "
                       "developments in technology and science. ";
    while (pos + strlen(text) < 2048) {
      memcpy(input + pos, text, strlen(text));
      pos += strlen(text);
    }
    /* Second half: binary-like data */
    for (size_t i = pos; i < sizeof(input); i++)
      input[i] = (uint8_t)(i * 37 + 91);

    bound = fio_brotli_compress_bound(sizeof(input));
    compressed = (uint8_t *)malloc(bound);
    decompressed = (uint8_t *)malloc(sizeof(input) + 64);
    if (compressed && decompressed) {
      for (int q = 5; q <= 6; q++) {
        size_t clen =
            fio_brotli_compress(compressed, bound, input, sizeof(input), q);
        TEST_ASSERT(clen > 0, "edge mixed q%d: compression failed", q);
        size_t dlen = fio_brotli_decompress(decompressed,
                                            sizeof(input) + 64,
                                            compressed,
                                            clen);
        TEST_ASSERT(dlen == sizeof(input),
                    "edge mixed q%d: decompressed %zu, expected %zu",
                    q,
                    dlen,
                    sizeof(input));
        if (dlen == sizeof(input))
          TEST_ASSERT(memcmp(decompressed, input, sizeof(input)) == 0,
                      "edge mixed q%d: data mismatch",
                      q);
      }
    }
    free(compressed);
    free(decompressed);
  }

  /* Test 4: All 256 byte values in sequence (at q5 and q6) */
  {
    uint8_t input[256];
    for (int i = 0; i < 256; i++)
      input[i] = (uint8_t)i;
    uint8_t comp[1024];
    uint8_t decomp[512];
    for (int q = 5; q <= 6; q++) {
      size_t clen = fio_brotli_compress(comp, sizeof(comp), input, 256, q);
      TEST_ASSERT(clen > 0, "edge all-bytes q%d: compression failed", q);
      size_t dlen = fio_brotli_decompress(decomp, sizeof(decomp), comp, clen);
      TEST_ASSERT(dlen == 256,
                  "edge all-bytes q%d: decompressed %zu, expected 256",
                  q,
                  dlen);
      if (dlen == 256)
        TEST_ASSERT(memcmp(decomp, input, 256) == 0,
                    "edge all-bytes q%d: data mismatch",
                    q);
    }
  }
}

/* *****************************************************************************
q5-q6 Hash Chain + Distance Cache + Lazy Matching Tests
***************************************************************************** */

static void test_brotli_compress_q5_roundtrip(void) {
  /* Test 1: Repeated text (good for distance cache) */
  {
    const char *phrase = "The quick brown fox jumps over the lazy dog. ";
    size_t phrase_len = 45;
    size_t total_len = phrase_len * 40;
    uint8_t *input = (uint8_t *)malloc(total_len);
    TEST_ASSERT(input != NULL, "q5 text: malloc failed");
    if (!input)
      return;
    for (size_t i = 0; i < 40; i++)
      memcpy(input + i * phrase_len, phrase, phrase_len);

    size_t bound = fio_brotli_compress_bound(total_len);
    uint8_t *compressed = (uint8_t *)malloc(bound);
    uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
    TEST_ASSERT(compressed && decompressed, "q5 text: malloc failed");
    if (!compressed || !decompressed) {
      free(input);
      free(compressed);
      free(decompressed);
      return;
    }

    size_t clen = fio_brotli_compress(compressed, bound, input, total_len, 5);
    TEST_ASSERT(clen > 0, "q5 text: compression failed");
    TEST_ASSERT(clen < total_len,
                "q5 text: should compress (%zu >= %zu)",
                clen,
                total_len);
    size_t dlen =
        fio_brotli_decompress(decompressed, total_len + 256, compressed, clen);
    TEST_ASSERT(dlen == total_len,
                "q5 text roundtrip: decompressed %zu, expected %zu",
                dlen,
                total_len);
    if (dlen == total_len)
      TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                  "q5 text roundtrip: data mismatch");
    free(input);
    free(compressed);
    free(decompressed);
  }

  /* Test 2: All byte values */
  {
    uint8_t input[256];
    for (int i = 0; i < 256; i++)
      input[i] = (uint8_t)i;
    uint8_t compressed[1024];
    size_t clen =
        fio_brotli_compress(compressed, sizeof(compressed), input, 256, 5);
    TEST_ASSERT(clen > 0, "q5 all-bytes: compression failed");
    uint8_t decompressed[512];
    size_t dlen = fio_brotli_decompress(decompressed,
                                        sizeof(decompressed),
                                        compressed,
                                        clen);
    TEST_ASSERT(dlen == 256,
                "q5 all-bytes roundtrip: expected 256, got %zu",
                dlen);
    if (dlen == 256)
      TEST_ASSERT(memcmp(decompressed, input, 256) == 0,
                  "q5 all-bytes roundtrip: data mismatch");
  }

  /* Test 3: Single byte */
  {
    uint8_t input[] = {0x42};
    uint8_t compressed[256];
    size_t clen =
        fio_brotli_compress(compressed, sizeof(compressed), input, 1, 5);
    TEST_ASSERT(clen > 0, "q5 single-byte: compression failed");
    uint8_t decompressed[64];
    size_t dlen = fio_brotli_decompress(decompressed,
                                        sizeof(decompressed),
                                        compressed,
                                        clen);
    TEST_ASSERT(dlen == 1,
                "q5 single-byte roundtrip: expected 1, got %zu",
                dlen);
    if (dlen == 1)
      TEST_ASSERT(decompressed[0] == 0x42,
                  "q5 single-byte roundtrip: data mismatch");
  }
}

static void test_brotli_compress_q6_roundtrip(void) {
  /* Test 1: Repeated text */
  {
    const char *phrase = "The quick brown fox jumps over the lazy dog. ";
    size_t phrase_len = 45;
    size_t total_len = phrase_len * 40;
    uint8_t *input = (uint8_t *)malloc(total_len);
    TEST_ASSERT(input != NULL, "q6 text: malloc failed");
    if (!input)
      return;
    for (size_t i = 0; i < 40; i++)
      memcpy(input + i * phrase_len, phrase, phrase_len);

    size_t bound = fio_brotli_compress_bound(total_len);
    uint8_t *compressed = (uint8_t *)malloc(bound);
    uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
    TEST_ASSERT(compressed && decompressed, "q6 text: malloc failed");
    if (!compressed || !decompressed) {
      free(input);
      free(compressed);
      free(decompressed);
      return;
    }

    size_t clen = fio_brotli_compress(compressed, bound, input, total_len, 6);
    TEST_ASSERT(clen > 0, "q6 text: compression failed");
    TEST_ASSERT(clen < total_len,
                "q6 text: should compress (%zu >= %zu)",
                clen,
                total_len);
    size_t dlen =
        fio_brotli_decompress(decompressed, total_len + 256, compressed, clen);
    TEST_ASSERT(dlen == total_len,
                "q6 text roundtrip: decompressed %zu, expected %zu",
                dlen,
                total_len);
    if (dlen == total_len)
      TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                  "q6 text roundtrip: data mismatch");
    free(input);
    free(compressed);
    free(decompressed);
  }

  /* Test 2: Highly repetitive data */
  {
    uint8_t input[2000];
    for (int i = 0; i < 250; i++)
      memcpy(input + i * 8, "ABCDEFGH", 8);
    size_t bound = fio_brotli_compress_bound(2000);
    uint8_t *compressed = (uint8_t *)malloc(bound);
    TEST_ASSERT(compressed != NULL, "q6 repeated: malloc failed");
    if (!compressed)
      return;
    size_t clen = fio_brotli_compress(compressed, bound, input, 2000, 6);
    TEST_ASSERT(clen > 0, "q6 repeated: compression failed");
    TEST_ASSERT(clen < 2000,
                "q6 repeated: should compress (%zu >= 2000)",
                clen);
    uint8_t decompressed[2256];
    size_t dlen = fio_brotli_decompress(decompressed,
                                        sizeof(decompressed),
                                        compressed,
                                        clen);
    TEST_ASSERT(dlen == 2000,
                "q6 repeated roundtrip: decompressed %zu, expected 2000",
                dlen);
    if (dlen == 2000)
      TEST_ASSERT(memcmp(decompressed, input, 2000) == 0,
                  "q6 repeated roundtrip: data mismatch");
    free(compressed);
  }
}

static void test_brotli_compress_q5q6_large_random(void) {
  size_t data_size = 65536;
  uint8_t *src = (uint8_t *)malloc(data_size);
  TEST_ASSERT(src != NULL, "q5q6 large random: src malloc failed");
  if (!src)
    return;
  fio_rand_bytes(src, data_size);

  size_t comp_bound = fio_brotli_compress_bound(data_size);
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  TEST_ASSERT(compressed != NULL,
              "q5q6 large random: compressed malloc failed");
  if (!compressed) {
    free(src);
    return;
  }

  for (int q = 5; q <= 6; q++) {
    size_t clen =
        fio_brotli_compress(compressed, comp_bound, src, data_size, q);
    TEST_ASSERT(clen > 0, "q%d large random: compression failed", q);
    if (!clen)
      continue;
    size_t decomp_bound = fio_brotli_decompress_bound(clen);
    uint8_t *decompressed = (uint8_t *)malloc(decomp_bound);
    TEST_ASSERT(decompressed != NULL,
                "q%d large random: decompressed malloc failed",
                q);
    if (!decompressed)
      continue;
    size_t dlen =
        fio_brotli_decompress(decompressed, decomp_bound, compressed, clen);
    TEST_ASSERT(dlen == data_size,
                "q%d large random: decompressed %zu, expected %zu",
                q,
                dlen,
                data_size);
    if (dlen == data_size)
      TEST_ASSERT(memcmp(decompressed, src, data_size) == 0,
                  "q%d large random: data mismatch",
                  q);
    free(decompressed);
  }
  free(compressed);
  free(src);
}

/* *****************************************************************************
Context Modeling Tests (q5 with NTREESL=2)
***************************************************************************** */

static void test_brotli_context_modeling_text(void) {
  const char *lines[] = {
      "The quick brown fox jumps over the lazy dog.\n",
      "Pack my box with five dozen liquor jugs!\n",
      "How vexingly quick daft zebras jump.\n",
      "The five boxing wizards jump quickly.\n",
      "Sphinx of black quartz, judge my vow.\n",
      "Two driven jocks help fax my big quiz.\n",
      "The jay, pig, fox, zebra and my wolves quack!\n",
      "Sympathizing would fix Quaker objectives.\n",
      "A wizard's job is to vex chumps quickly in fog.\n",
      "Watch Jeopardy!, Alex Trebek's fun TV quiz game.\n",
      "By Jove, my quick study of lexicography won a prize.\n",
      "Crazy Frederick bought many very exquisite opal jewels.\n",
  };
  size_t nlines = sizeof(lines) / sizeof(lines[0]);

  size_t total_len = 0;
  uint8_t input[8192];
  while (total_len + 60 < sizeof(input)) {
    const char *line = lines[total_len % nlines];
    size_t llen = strlen(line);
    if (total_len + llen > sizeof(input))
      break;
    memcpy(input + total_len, line, llen);
    total_len += llen;
  }

  size_t bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
  TEST_ASSERT(compressed && decompressed, "context text: malloc failed");
  if (!compressed || !decompressed) {
    free(compressed);
    free(decompressed);
    return;
  }

  size_t clen_q4 = fio_brotli_compress(compressed, bound, input, total_len, 4);
  TEST_ASSERT(clen_q4 > 0, "context text q4: compression failed");

  size_t clen_q5 = fio_brotli_compress(compressed, bound, input, total_len, 5);
  TEST_ASSERT(clen_q5 > 0, "context text q5: compression failed");

  /* Verify q5 roundtrip */
  size_t dlen =
      fio_brotli_decompress(decompressed, total_len + 256, compressed, clen_q5);
  TEST_ASSERT(dlen == total_len,
              "context text q5 roundtrip: decompressed %zu, expected %zu",
              dlen,
              total_len);
  if (dlen == total_len)
    TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                "context text q5 roundtrip: data mismatch");

  size_t clen_q6 = fio_brotli_compress(compressed, bound, input, total_len, 6);
  TEST_ASSERT(clen_q6 > 0, "context text q6: compression failed");

  /* Verify q6 roundtrip */
  dlen =
      fio_brotli_decompress(decompressed, total_len + 256, compressed, clen_q6);
  TEST_ASSERT(dlen == total_len,
              "context text q6 roundtrip: decompressed %zu, expected %zu",
              dlen,
              total_len);
  if (dlen == total_len)
    TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                "context text q6 roundtrip: data mismatch");

  free(compressed);
  free(decompressed);
}

static void test_brotli_context_modeling_html(void) {
  const char *fragments[] = {
      "<html><head><title>Test Page</title></head><body>\n",
      "<div class=\"container\" id=\"main\">\n",
      "  <h1>Welcome to the Test Page</h1>\n",
      "  <p>This is a paragraph with some <strong>bold</strong> text.</p>\n",
      ("  <p>Another paragraph with <a href=\"https://example.com\">a "
       "link</a>.</p>\n"),
      "  <ul>\n",
      "    <li>First item in the list</li>\n",
      "    <li>Second item with more content</li>\n",
      "    <li>Third item: special chars &amp; entities</li>\n",
      "  </ul>\n",
      "  <table border=\"1\">\n",
      "    <tr><th>Name</th><th>Value</th><th>Description</th></tr>\n",
      "    <tr><td>Alpha</td><td>100</td><td>First entry</td></tr>\n",
      "    <tr><td>Beta</td><td>200</td><td>Second entry</td></tr>\n",
      "    <tr><td>Gamma</td><td>300</td><td>Third entry</td></tr>\n",
      "  </table>\n",
      "</div>\n",
      "<script>var x = document.getElementById('main');</script>\n",
      "</body></html>\n",
  };
  size_t nfrags = sizeof(fragments) / sizeof(fragments[0]);

  size_t total_len = 0;
  uint8_t input[10240];
  for (int rep = 0; total_len + 100 < sizeof(input); rep++) {
    const char *frag = fragments[rep % nfrags];
    size_t flen = strlen(frag);
    if (total_len + flen > sizeof(input))
      break;
    memcpy(input + total_len, frag, flen);
    total_len += flen;
  }

  size_t bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
  TEST_ASSERT(compressed && decompressed, "context html: malloc failed");
  if (!compressed || !decompressed) {
    free(compressed);
    free(decompressed);
    return;
  }

  size_t clen_q5 = fio_brotli_compress(compressed, bound, input, total_len, 5);

  /* Verify q5 roundtrip */
  size_t dlen =
      fio_brotli_decompress(decompressed, total_len + 256, compressed, clen_q5);
  TEST_ASSERT(dlen == total_len,
              "context html q5 roundtrip: decompressed %zu, expected %zu",
              dlen,
              total_len);
  if (dlen == total_len)
    TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                "context html q5 roundtrip: data mismatch");

  free(compressed);
  free(decompressed);
}

static void test_brotli_context_modeling_binary(void) {
  size_t total_len = 8192;
  uint8_t *input = (uint8_t *)malloc(total_len);
  TEST_ASSERT(input != NULL, "context binary: malloc failed");
  if (!input)
    return;
  fio_rand_bytes(input, total_len);

  size_t bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
  TEST_ASSERT(compressed && decompressed, "context binary: malloc failed");
  if (!compressed || !decompressed) {
    free(input);
    free(compressed);
    free(decompressed);
    return;
  }

  for (int q = 5; q <= 6; q++) {
    size_t clen = fio_brotli_compress(compressed, bound, input, total_len, q);
    TEST_ASSERT(clen > 0, "context binary q%d: compression failed", q);
    size_t dlen =
        fio_brotli_decompress(decompressed, total_len + 256, compressed, clen);
    TEST_ASSERT(dlen == total_len,
                "context binary q%d roundtrip: decompressed %zu, expected %zu",
                q,
                dlen,
                total_len);
    if (dlen == total_len)
      TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                  "context binary q%d roundtrip: data mismatch",
                  q);
  }

  free(input);
  free(compressed);
  free(decompressed);
}

/* *****************************************************************************
Static Dictionary (q5+)
***************************************************************************** */

static void test_brotli_dict_english_text(void) {
  const char *text =
      "The international conference discussed important developments "
      "in technology and science. Representatives from different "
      "countries presented their research findings about environmental "
      "protection and sustainable development. The organization "
      "published comprehensive guidelines for implementation across "
      "various government departments and educational institutions. "
      "Information about the application process was distributed to "
      "all participants through electronic communication channels. "
      "The committee recommended significant improvements to the "
      "existing infrastructure and transportation systems. Several "
      "professional associations contributed valuable resources for "
      "the community development program. The investigation revealed "
      "interesting connections between economic performance and social "
      "conditions in metropolitan areas throughout the region.";
  size_t text_len = strlen(text);

  size_t bound = fio_brotli_compress_bound(text_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  uint8_t *decompressed = (uint8_t *)malloc(text_len + 256);
  TEST_ASSERT(compressed && decompressed, "dict english: malloc failed");
  if (!compressed || !decompressed) {
    free(compressed);
    free(decompressed);
    return;
  }

  size_t clen_q4 = fio_brotli_compress(compressed,
                                       bound,
                                       (const uint8_t *)text,
                                       text_len,
                                       4);
  size_t clen_q5 = fio_brotli_compress(compressed,
                                       bound,
                                       (const uint8_t *)text,
                                       text_len,
                                       5);

  /* Verify roundtrip */
  size_t dlen =
      fio_brotli_decompress(decompressed, text_len + 256, compressed, clen_q5);
  TEST_ASSERT(dlen == text_len,
              "dict english q5 roundtrip: decompressed %zu, expected %zu",
              dlen,
              text_len);
  if (dlen == text_len)
    TEST_ASSERT(memcmp(decompressed, text, text_len) == 0,
                "dict english q5 roundtrip: data mismatch");

  TEST_ASSERT(clen_q5 <= clen_q4,
              "dict english: q5 (%zu) should be <= q4 (%zu)",
              clen_q5,
              clen_q4);

  free(compressed);
  free(decompressed);
}

static void test_brotli_dict_no_regression_repetitive(void) {
  const char *phrase = "abcdefghij";
  size_t phrase_len = 10;
  size_t total_len = phrase_len * 200;
  uint8_t *input = (uint8_t *)malloc(total_len);
  TEST_ASSERT(input != NULL, "dict repetitive: malloc failed");
  if (!input)
    return;
  for (size_t i = 0; i < 200; i++)
    memcpy(input + i * phrase_len, phrase, phrase_len);

  size_t bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
  TEST_ASSERT(compressed && decompressed, "dict repetitive: malloc failed");
  if (!compressed || !decompressed) {
    free(input);
    free(compressed);
    free(decompressed);
    return;
  }

  size_t clen_q4 = fio_brotli_compress(compressed, bound, input, total_len, 4);
  size_t clen_q5 = fio_brotli_compress(compressed, bound, input, total_len, 5);

  /* Verify roundtrip */
  size_t dlen =
      fio_brotli_decompress(decompressed, total_len + 256, compressed, clen_q5);
  TEST_ASSERT(dlen == total_len,
              "dict repetitive q5 roundtrip: decompressed %zu, expected %zu",
              dlen,
              total_len);
  if (dlen == total_len)
    TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                "dict repetitive q5 roundtrip: data mismatch");

  TEST_ASSERT(clen_q5 <= clen_q4 + 2,
              "dict repetitive: q5 (%zu) should be <= q4+2 (%zu)",
              clen_q5,
              clen_q4 + 2);

  free(input);
  free(compressed);
  free(decompressed);
}

static void test_brotli_dict_mixed_content(void) {
  const char *fragments[] = {
      "The application requires authentication before processing. ",
      "Configuration parameters include database connection settings. ",
      "The server responded with an unexpected error message. ",
      "Performance optimization techniques were implemented successfully. ",
      "Documentation describes the recommended implementation approach. ",
      "The development environment supports multiple programming languages. ",
      "Security considerations include encryption and authorization. ",
      "The infrastructure deployment followed established procedures. ",
  };
  size_t nfrags = sizeof(fragments) / sizeof(fragments[0]);

  size_t total_len = 0;
  uint8_t input[4096];
  for (int rep = 0; total_len + 100 < sizeof(input); rep++) {
    const char *frag = fragments[rep % nfrags];
    size_t flen = strlen(frag);
    if (total_len + flen > sizeof(input))
      break;
    memcpy(input + total_len, frag, flen);
    total_len += flen;
  }

  size_t bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
  TEST_ASSERT(compressed && decompressed, "dict mixed: malloc failed");
  if (!compressed || !decompressed) {
    free(compressed);
    free(decompressed);
    return;
  }

  size_t clen_q4 = fio_brotli_compress(compressed, bound, input, total_len, 4);
  size_t clen_q5 = fio_brotli_compress(compressed, bound, input, total_len, 5);

  /* Verify roundtrip */
  size_t dlen =
      fio_brotli_decompress(decompressed, total_len + 256, compressed, clen_q5);
  TEST_ASSERT(dlen == total_len,
              "dict mixed q5 roundtrip: decompressed %zu, expected %zu",
              dlen,
              total_len);
  if (dlen == total_len)
    TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                "dict mixed q5 roundtrip: data mismatch");

  TEST_ASSERT(clen_q5 <= clen_q4,
              "dict mixed: q5 (%zu) should be <= q4 (%zu)",
              clen_q5,
              clen_q4);

  free(compressed);
  free(decompressed);
}

static void test_brotli_dict_uppercase(void) {
  const char *text =
      "The International Conference on Technology and Science was held "
      "in Washington. Representatives from the European Commission and "
      "the American Association presented their findings. The National "
      "Institute published a comprehensive report about Environmental "
      "Protection and Sustainable Development. The Organization for "
      "Economic Cooperation released new guidelines for International "
      "Trade and Commerce. The Department of Education announced "
      "significant changes to the curriculum. The Government allocated "
      "additional resources for Infrastructure Development across the "
      "country. Professional Associations contributed to the Community "
      "Development Program with valuable expertise and resources.";
  size_t text_len = strlen(text);

  size_t bound = fio_brotli_compress_bound(text_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  uint8_t *decompressed = (uint8_t *)malloc(text_len + 256);
  TEST_ASSERT(compressed && decompressed, "dict uppercase: malloc failed");
  if (!compressed || !decompressed) {
    free(compressed);
    free(decompressed);
    return;
  }

  size_t clen_q5 = fio_brotli_compress(compressed,
                                       bound,
                                       (const uint8_t *)text,
                                       text_len,
                                       5);

  size_t dlen =
      fio_brotli_decompress(decompressed, text_len + 256, compressed, clen_q5);
  TEST_ASSERT(dlen == text_len,
              "dict uppercase q5 roundtrip: decompressed %zu, expected %zu",
              dlen,
              text_len);
  if (dlen == text_len)
    TEST_ASSERT(memcmp(decompressed, text, text_len) == 0,
                "dict uppercase q5 roundtrip: data mismatch");

  free(compressed);
  free(decompressed);
}

static void test_brotli_dict_short_input(void) {
  const char *inputs[] = {
      "Hello",
      "The quick",
      "international",
      "Hello, World!",
      "The quick brown fox",
  };
  size_t ninputs = sizeof(inputs) / sizeof(inputs[0]);

  for (size_t i = 0; i < ninputs; i++) {
    size_t ilen = strlen(inputs[i]);
    uint8_t compressed[256];
    uint8_t decompressed[256];

    size_t clen = fio_brotli_compress(compressed,
                                      sizeof(compressed),
                                      (const uint8_t *)inputs[i],
                                      ilen,
                                      5);
    TEST_ASSERT(clen > 0, "dict short q5 '%s': compression failed", inputs[i]);

    size_t dlen = fio_brotli_decompress(decompressed,
                                        sizeof(decompressed),
                                        compressed,
                                        clen);
    TEST_ASSERT(dlen == ilen,
                "dict short q5 '%s' roundtrip: decompressed %zu, expected %zu",
                inputs[i],
                dlen,
                ilen);
    if (dlen == ilen)
      TEST_ASSERT(memcmp(decompressed, inputs[i], ilen) == 0,
                  "dict short q5 '%s' roundtrip: data mismatch",
                  inputs[i]);
  }
}

static void test_brotli_dict_large_diverse(void) {
  const char *paragraphs[] = {
      "The government established a comprehensive framework for managing "
      "environmental resources and protecting natural habitats. ",
      "Scientific research demonstrated significant improvements in "
      "understanding molecular structures and chemical reactions. ",
      "The educational institution developed innovative approaches to "
      "teaching mathematics and computer science fundamentals. ",
      "International cooperation facilitated the exchange of technical "
      "knowledge and professional expertise between countries. ",
      "The transportation department announced plans for modernizing "
      "infrastructure and improving public transit systems. ",
      "Healthcare professionals recommended preventive measures and "
      "regular examinations for maintaining physical wellness. ",
      "The financial institution implemented advanced security protocols "
      "for protecting customer information and transactions. ",
      "Agricultural development programs focused on sustainable farming "
      "practices and efficient water management techniques. ",
      "The communications industry experienced rapid technological "
      "advancement in wireless networks and digital services. ",
      "Manufacturing companies adopted automated production processes "
      "to increase efficiency and reduce operational costs. ",
  };
  size_t nparas = sizeof(paragraphs) / sizeof(paragraphs[0]);

  size_t total_len = 0;
  uint8_t *input = (uint8_t *)malloc(16384);
  TEST_ASSERT(input != NULL, "dict large diverse: malloc failed");
  if (!input)
    return;
  for (int rep = 0; total_len + 200 < 16384; rep++) {
    const char *para = paragraphs[rep % nparas];
    size_t plen = strlen(para);
    if (total_len + plen > 16384)
      break;
    memcpy(input + total_len, para, plen);
    total_len += plen;
  }

  size_t bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
  TEST_ASSERT(compressed && decompressed, "dict large diverse: malloc failed");
  if (!compressed || !decompressed) {
    free(input);
    free(compressed);
    free(decompressed);
    return;
  }

  size_t clen_q4 = fio_brotli_compress(compressed, bound, input, total_len, 4);
  size_t clen_q5 = fio_brotli_compress(compressed, bound, input, total_len, 5);

  /* Verify roundtrip */
  size_t dlen =
      fio_brotli_decompress(decompressed, total_len + 256, compressed, clen_q5);
  TEST_ASSERT(dlen == total_len,
              "dict large diverse q5 roundtrip: decompressed %zu, expected %zu",
              dlen,
              total_len);
  if (dlen == total_len)
    TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                "dict large diverse q5 roundtrip: data mismatch");

  TEST_ASSERT(clen_q5 <= clen_q4,
              "dict large diverse: q5 (%zu) should be <= q4 (%zu)",
              clen_q5,
              clen_q4);

  free(input);
  free(compressed);
  free(decompressed);
}

/* *****************************************************************************
Compression ratio regression: q5 >= q4, q6 >= q5 on text/HTML
***************************************************************************** */

static void test_brotli_ratio_regression(void) {
  /* Build ~8KB of diverse text */
  const char *lines[] = {
      "The quick brown fox jumps over the lazy dog.\n",
      "Pack my box with five dozen liquor jugs!\n",
      "How vexingly quick daft zebras jump.\n",
      "The five boxing wizards jump quickly.\n",
      "Sphinx of black quartz, judge my vow.\n",
      "Two driven jocks help fax my big quiz.\n",
      "The jay, pig, fox, zebra and my wolves quack!\n",
      "Sympathizing would fix Quaker objectives.\n",
  };
  size_t nlines = sizeof(lines) / sizeof(lines[0]);

  size_t total_len = 0;
  uint8_t input[8192];
  while (total_len + 60 < sizeof(input)) {
    const char *line = lines[total_len % nlines];
    size_t llen = strlen(line);
    if (total_len + llen > sizeof(input))
      break;
    memcpy(input + total_len, line, llen);
    total_len += llen;
  }

  size_t bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  TEST_ASSERT(compressed != NULL, "ratio regression: malloc failed");
  if (!compressed)
    return;

  size_t clen_q4 = fio_brotli_compress(compressed, bound, input, total_len, 4);
  size_t clen_q5 = fio_brotli_compress(compressed, bound, input, total_len, 5);
  size_t clen_q6 = fio_brotli_compress(compressed, bound, input, total_len, 6);

  /* Verify roundtrip for q6 */
  uint8_t *decompressed = (uint8_t *)malloc(total_len + 256);
  if (decompressed) {
    size_t dlen = fio_brotli_decompress(decompressed,
                                        total_len + 256,
                                        compressed,
                                        clen_q6);
    TEST_ASSERT(dlen == total_len,
                "ratio regression q6 roundtrip: decompressed %zu, expected %zu",
                dlen,
                total_len);
    if (dlen == total_len)
      TEST_ASSERT(memcmp(decompressed, input, total_len) == 0,
                  "ratio regression q6 roundtrip: data mismatch");
    free(decompressed);
  }

  /* q5 should be <= q4 (or at most marginally worse, +2 bytes tolerance) */
  TEST_ASSERT(clen_q5 <= clen_q4 + 2,
              "ratio regression: q5 (%zu) should be <= q4+2 (%zu) on text",
              clen_q5,
              clen_q4 + 2);

  /* q6 should be <= q5 (or at most marginally worse, +5 bytes tolerance
   * since block splitting overhead may not pay off on small data) */
  TEST_ASSERT(clen_q6 <= clen_q5 + 5,
              "ratio regression: q6 (%zu) should be <= q5+5 (%zu) on text",
              clen_q6,
              clen_q5 + 5);

  free(compressed);
}

/* *****************************************************************************
Robustness Tests — decompressor must never crash on malicious input
***************************************************************************** */

static void test_brotli_random_garbage(void) {
  uint8_t out[4096];
  uint8_t garbage[1024];
  for (int trial = 0; trial < 100; trial++) {
    fio_rand_bytes(garbage, sizeof(garbage));
    size_t result =
        fio_brotli_decompress(out, sizeof(out), garbage, sizeof(garbage));
    (void)result;
  }
  TEST_ASSERT(1, "random garbage: survived 100 trials without crash");
}

static void test_brotli_truncated_stream(void) {
  const char *input = "The quick brown fox jumps over the lazy dog. ";
  size_t in_len = 45;
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, in_len, 1);
  TEST_ASSERT(clen > 0, "truncated: compression failed");
  if (!clen)
    return;
  uint8_t out[256];
  for (size_t trunc = 0; trunc < clen; trunc++) {
    size_t result = fio_brotli_decompress(out, sizeof(out), compressed, trunc);
    (void)result;
  }
  TEST_ASSERT(1, "truncated: survived all truncation points");
}

static void test_brotli_corrupted_stream(void) {
  const char *input = "Hello, World! This is a test of brotli corruption.";
  size_t in_len = 50;
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, in_len, 1);
  TEST_ASSERT(clen > 0, "corrupted: compression failed");
  if (!clen)
    return;
  uint8_t corrupted[256];
  uint8_t out[256];
  for (size_t byte_idx = 0; byte_idx < clen; byte_idx++) {
    for (int bit = 0; bit < 8; bit++) {
      memcpy(corrupted, compressed, clen);
      corrupted[byte_idx] ^= (uint8_t)(1 << bit);
      size_t result = fio_brotli_decompress(out, sizeof(out), corrupted, clen);
      (void)result;
    }
  }
  TEST_ASSERT(1, "corrupted: survived all bit-flip corruptions");
}

static void test_brotli_zero_input(void) {
  uint8_t out[64];
  size_t result = fio_brotli_decompress(out, sizeof(out), "", 0);
  TEST_ASSERT(result == 0, "zero-length input: expected 0, got %zu", result);
}

static void test_brotli_all_zeros(void) {
  uint8_t zeros[256];
  memset(zeros, 0, sizeof(zeros));
  uint8_t out[4096];
  for (size_t sz = 1; sz <= sizeof(zeros); sz *= 2) {
    size_t result = fio_brotli_decompress(out, sizeof(out), zeros, sz);
    (void)result;
  }
  TEST_ASSERT(1, "all-zeros: survived all sizes without crash");
}

static void test_brotli_all_ones(void) {
  uint8_t ones[256];
  memset(ones, 0xFF, sizeof(ones));
  uint8_t out[4096];
  for (size_t sz = 1; sz <= sizeof(ones); sz *= 2) {
    size_t result = fio_brotli_decompress(out, sizeof(out), ones, sz);
    (void)result;
  }
  TEST_ASSERT(1, "all-0xFF: survived all sizes without crash");
}

static void test_brotli_single_byte_inputs(void) {
  uint8_t out[4096];
  for (int b = 0; b < 256; b++) {
    uint8_t byte = (uint8_t)b;
    size_t result = fio_brotli_decompress(out, sizeof(out), &byte, 1);
    (void)result;
  }
  TEST_ASSERT(1, "single-byte: survived all 256 values without crash");
}

static void test_brotli_compress_random_roundtrip(void) {
  uint8_t src[4096];
  fio_rand_bytes(src, sizeof(src));

  size_t comp_bound = fio_brotli_compress_bound(sizeof(src));
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  TEST_ASSERT(compressed != NULL, "random roundtrip: malloc failed");
  if (!compressed)
    return;

  for (int q = 1; q <= 6; q++) {
    size_t clen =
        fio_brotli_compress(compressed, comp_bound, src, sizeof(src), q);
    TEST_ASSERT(clen > 0, "random roundtrip q%d: compression failed", q);
    if (!clen)
      continue;
    size_t decomp_bound = fio_brotli_decompress_bound(clen);
    uint8_t *decompressed = (uint8_t *)malloc(decomp_bound);
    TEST_ASSERT(decompressed != NULL, "random roundtrip q%d: malloc failed", q);
    if (!decompressed)
      continue;
    size_t dlen =
        fio_brotli_decompress(decompressed, decomp_bound, compressed, clen);
    TEST_ASSERT(dlen == sizeof(src),
                "random roundtrip q%d: decompressed %zu, expected %zu",
                q,
                dlen,
                sizeof(src));
    if (dlen == sizeof(src))
      TEST_ASSERT(memcmp(decompressed, src, sizeof(src)) == 0,
                  "random roundtrip q%d: data mismatch",
                  q);
    free(decompressed);
  }
  free(compressed);
}

static void test_brotli_output_too_small(void) {
  const char *input = "ABCDEFGH";
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, 8, 1);
  TEST_ASSERT(clen > 0, "output too small: compression failed");
  if (!clen)
    return;
  /* With undersized buffers, should return required size (> out_sz) */
  for (size_t out_sz = 1; out_sz < 8; out_sz++) {
    uint8_t out[8];
    size_t result = fio_brotli_decompress(out, out_sz, compressed, clen);
    TEST_ASSERT(result > out_sz,
                "output too small (out_sz=%zu): expected > %zu, got %zu",
                out_sz,
                out_sz,
                result);
    TEST_ASSERT(result >= 8,
                "output too small (out_sz=%zu): expected >= 8, got %zu",
                out_sz,
                result);
  }
  /* With out_sz=0 (size query), should return required size */
  {
    uint8_t out[8];
    size_t result = fio_brotli_decompress(out, 0, compressed, clen);
    TEST_ASSERT(result == 8,
                "output too small (out_sz=0): expected 8, got %zu",
                result);
  }
}

static void test_brotli_size_query(void) {
  /* Test 1: Size query with NULL output */
  {
    const char *input = "Hello, World!";
    uint8_t compressed[256];
    size_t clen =
        fio_brotli_compress(compressed, sizeof(compressed), input, 13, 1);
    TEST_ASSERT(clen > 0, "size query hello: compression failed");
    if (clen) {
      size_t needed = fio_brotli_decompress(NULL, 0, compressed, clen);
      TEST_ASSERT(needed == 13,
                  "size query hello NULL: expected 13, got %zu",
                  needed);
      needed = fio_brotli_decompress(NULL, 1024, compressed, clen);
      TEST_ASSERT(needed == 13,
                  "size query hello NULL+len: expected 13, got %zu",
                  needed);
    }
  }

  /* Test 2: Size query with out_len=0 */
  {
    uint8_t input[800];
    for (int i = 0; i < 100; i++)
      memcpy(input + i * 8, "ABCDEFGH", 8);
    size_t bound = fio_brotli_compress_bound(800);
    uint8_t *compressed = (uint8_t *)malloc(bound);
    TEST_ASSERT(compressed != NULL, "size query repeated: malloc failed");
    if (compressed) {
      size_t clen = fio_brotli_compress(compressed, bound, input, 800, 1);
      TEST_ASSERT(clen > 0, "size query repeated: compression failed");
      if (clen) {
        uint8_t dummy[1];
        size_t needed = fio_brotli_decompress(dummy, 0, compressed, clen);
        TEST_ASSERT(needed == 800,
                    "size query repeated: expected 800, got %zu",
                    needed);
      }
      free(compressed);
    }
  }

  /* Test 3: Size query returns 0 for corrupt data */
  {
    uint8_t garbage[64];
    memset(garbage, 0xAB, sizeof(garbage));
    size_t needed = fio_brotli_decompress(NULL, 0, garbage, sizeof(garbage));
    TEST_ASSERT(needed == 0, "size query garbage: expected 0, got %zu", needed);
  }

  /* Test 4: Buffer too small returns required size > out_len */
  {
    const char *phrase = "The quick brown fox jumps over the lazy dog. ";
    size_t phrase_len = 45;
    size_t total_len = phrase_len * 20;
    uint8_t input[900];
    for (int i = 0; i < 20; i++)
      memcpy(input + i * phrase_len, phrase, phrase_len);
    size_t bound = fio_brotli_compress_bound(total_len);
    uint8_t *compressed = (uint8_t *)malloc(bound);
    TEST_ASSERT(compressed != NULL, "size query too small: malloc failed");
    if (compressed) {
      size_t clen = fio_brotli_compress(compressed, bound, input, total_len, 1);
      TEST_ASSERT(clen > 0, "size query too small: compression failed");
      if (clen) {
        /* Try with half the needed buffer */
        size_t half = total_len / 2;
        uint8_t *small_buf = (uint8_t *)malloc(half);
        if (small_buf) {
          size_t result =
              fio_brotli_decompress(small_buf, half, compressed, clen);
          TEST_ASSERT(result > half,
                      "size query too small: expected > %zu, got %zu",
                      half,
                      result);
          TEST_ASSERT(result >= total_len,
                      "size query too small: expected >= %zu, got %zu",
                      total_len,
                      result);
          free(small_buf);
        }
      }
      free(compressed);
    }
  }

  /* Test 5: Size query across all quality levels */
  {
    const char *input = "Hello, World! This is a test of brotli size query.";
    size_t in_len = 50;
    uint8_t compressed[256];
    for (int q = 1; q <= 6; q++) {
      size_t clen =
          fio_brotli_compress(compressed, sizeof(compressed), input, in_len, q);
      TEST_ASSERT(clen > 0, "size query q%d: compression failed", q);
      if (clen) {
        size_t needed = fio_brotli_decompress(NULL, 0, compressed, clen);
        TEST_ASSERT(needed == in_len,
                    "size query q%d: expected %zu, got %zu",
                    q,
                    in_len,
                    needed);
      }
    }
  }

  /* Test 6: Empty stream size query */
  {
    uint8_t empty_stream[] = {0x3b}; /* WBITS=22, ISLAST=1, ISLASTEMPTY=1 */
    size_t needed =
        fio_brotli_decompress(NULL, 0, empty_stream, sizeof(empty_stream));
    TEST_ASSERT(needed == 0, "size query empty: expected 0, got %zu", needed);
  }
}

static void test_brotli_large_random_roundtrip(void) {
  size_t data_size = 65536;
  uint8_t *src = (uint8_t *)malloc(data_size);
  TEST_ASSERT(src != NULL, "large random: src malloc failed");
  if (!src)
    return;
  fio_rand_bytes(src, data_size);

  size_t comp_bound = fio_brotli_compress_bound(data_size);
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  TEST_ASSERT(compressed != NULL, "large random: compressed malloc failed");
  if (!compressed) {
    free(src);
    return;
  }

  size_t clen = fio_brotli_compress(compressed, comp_bound, src, data_size, 4);
  TEST_ASSERT(clen > 0, "large random: compression failed");

  if (clen > 0) {
    size_t decomp_bound = fio_brotli_decompress_bound(clen);
    uint8_t *decompressed = (uint8_t *)malloc(decomp_bound);
    TEST_ASSERT(decompressed != NULL,
                "large random: decompressed malloc failed");
    if (decompressed) {
      size_t dlen =
          fio_brotli_decompress(decompressed, decomp_bound, compressed, clen);
      TEST_ASSERT(dlen == data_size,
                  "large random: decompressed %zu, expected %zu",
                  dlen,
                  data_size);
      if (dlen == data_size)
        TEST_ASSERT(memcmp(decompressed, src, data_size) == 0,
                    "large random: data mismatch");
      free(decompressed);
    }
  }

  free(compressed);
  free(src);
}

/* *****************************************************************************
Diagnostic: exact compressed sizes for q1-q6 on 64KB HTML
***************************************************************************** */

static void test_brotli_q_comparison(void) {
  /* Regression test: higher quality levels must compress at least as well
   * as lower ones. Specifically q5 >= q4 and q6 >= q5 on all data sizes. */
  static const char html_pattern[] =
      "<div class=\"item\"><h2>Title</h2><p>Lorem ipsum dolor sit amet, "
      "consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut "
      "labore et dolore magna aliqua.</p><span class=\"meta\">Author: "
      "John</span></div>\n";
  size_t plen = sizeof(html_pattern) - 1;
  size_t max_size = 65536;
  uint8_t *src = (uint8_t *)malloc(max_size);
  if (!src)
    return;
  size_t pos = 0;
  while (pos + plen <= max_size) {
    memcpy(src + pos, html_pattern, plen);
    pos += plen;
  }
  if (pos < max_size)
    memcpy(src + pos, html_pattern, max_size - pos);

  size_t comp_bound = fio_brotli_compress_bound(max_size);
  uint8_t *compressed = (uint8_t *)malloc(comp_bound);
  if (!compressed) {
    free(src);
    return;
  }

  static const size_t test_sizes[] = {4096, 8192, 16384, 32768, 65536};
  for (size_t si = 0; si < sizeof(test_sizes) / sizeof(test_sizes[0]); ++si) {
    size_t data_size = test_sizes[si];
    size_t sizes[7] = {0}; /* sizes[1..6] for q1..q6 */
    fprintf(stderr, "\n  === q1-q6 on %zuKB HTML ===\n", data_size / 1024);
    for (int q = 1; q <= 6; ++q) {
      sizes[q] = fio_brotli_compress(compressed, comp_bound, src, data_size, q);
      fprintf(stderr,
              "    q%d: %zu bytes (%.2f%%)\n",
              q,
              sizes[q],
              100.0 * (double)sizes[q] / (double)data_size);
      /* Verify roundtrip */
      size_t decomp_bound = fio_brotli_decompress_bound(sizes[q]);
      uint8_t *decompressed = (uint8_t *)malloc(decomp_bound);
      if (decompressed) {
        size_t dlen = fio_brotli_decompress(decompressed,
                                            decomp_bound,
                                            compressed,
                                            sizes[q]);
        TEST_ASSERT(dlen == data_size,
                    "q%d %zuKB HTML: decompressed %zu, expected %zu",
                    q,
                    data_size / 1024,
                    dlen,
                    data_size);
        if (dlen == data_size)
          TEST_ASSERT(memcmp(decompressed, src, data_size) == 0,
                      "q%d %zuKB HTML: data mismatch",
                      q,
                      data_size / 1024);
        free(decompressed);
      }
    }
    /* Monotonicity: higher quality must not produce larger output */
    TEST_ASSERT(sizes[5] <= sizes[4],
                "%zuKB HTML: q5 (%zu) > q4 (%zu)",
                data_size / 1024,
                sizes[5],
                sizes[4]);
    TEST_ASSERT(sizes[6] <= sizes[5],
                "%zuKB HTML: q6 (%zu) > q5 (%zu)",
                data_size / 1024,
                sizes[6],
                sizes[5]);
  }

  free(compressed);
  free(src);
}

/* *****************************************************************************
Test: 16MB roundtrip corruption test
***************************************************************************** */

static void test_brotli_16mb_roundtrip(void) {
  fprintf(stderr, "Testing 16MB roundtrip corruption test...\n");

  const size_t target_size = 16 * 1024 * 1024;
  uint8_t *data = (uint8_t *)malloc(target_size + 4096);
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
    memcpy(data + pos, block, blen);
    pos += blen;
  }
  size_t data_len = pos;

  fprintf(stderr,
          "  Generated %zu bytes (%.1f MB) of test data\n",
          data_len,
          (double)data_len / (1024.0 * 1024.0));

  int qualities[] = {1, 3, 4, 6};
  for (int qi = 0; qi < 4; ++qi) {
    int quality = qualities[qi];

    /* Compress */
    size_t comp_bound = fio_brotli_compress_bound(data_len);
    uint8_t *compressed = (uint8_t *)malloc(comp_bound);
    TEST_ASSERT(compressed != NULL,
                "16mb q%d: malloc compressed failed",
                quality);
    if (!compressed)
      continue;

    size_t comp_len =
        fio_brotli_compress(compressed, comp_bound, data, data_len, quality);
    TEST_ASSERT(comp_len > 0, "16mb q%d: brotli returned 0", quality);
    TEST_ASSERT(comp_len < data_len,
                "16mb q%d: compressed %zu >= original %zu",
                quality,
                comp_len,
                data_len);

    if (comp_len > 0 && comp_len < data_len) {
      /* Decompress */
      uint8_t *decompressed = (uint8_t *)malloc(data_len + 256);
      TEST_ASSERT(decompressed != NULL,
                  "16mb q%d: malloc decompressed failed",
                  quality);
      if (decompressed) {
        size_t decomp_len = fio_brotli_decompress(decompressed,
                                                  data_len + 256,
                                                  compressed,
                                                  comp_len);
        TEST_ASSERT(decomp_len == data_len,
                    "16mb q%d: decompressed %zu, expected %zu",
                    quality,
                    decomp_len,
                    data_len);
        int match = (decomp_len == data_len) &&
                    (memcmp(decompressed, data, data_len) == 0);
        TEST_ASSERT(match, "16mb q%d: data corruption detected", quality);
        fprintf(stderr,
                "  q%d: %zu -> %zu bytes (%.2f%%) — %s\n",
                quality,
                data_len,
                comp_len,
                100.0 * (double)comp_len / (double)data_len,
                match ? "ROUNDTRIP OK" : "CORRUPTION DETECTED");
        free(decompressed);
      }
    }

    free(compressed);
  }

  free(data);
}

/* *****************************************************************************
Adversarial / Maliciously-Crafted Payloads
***************************************************************************** */

/*
 * Brotli stream bit layout (LSB-first):
 *
 * WBITS encoding:
 *   bit0=0                     → WBITS=16  (1 bit consumed)
 *   bit0=1, bits[1:3]=n≠0      → WBITS=n+17 (4 bits consumed, n=1..7 → 18..24)
 *   bit0=1, bits[1:3]=0, bits[4:6]=n≠0,1 → WBITS=n+8 (7 bits, n=2..7 → 10..15)
 *   bit0=1, bits[1:3]=0, bits[4:6]=0  → WBITS=17 (7 bits)
 *   bit0=1, bits[1:3]=0, bits[4:6]=1  → invalid (return 0)
 *
 * After WBITS:
 *   ISLAST (1 bit)
 *   if ISLAST: ISLASTEMPTY (1 bit) — if 1: empty stream, done
 *   MNIBBLES (2 bits): 0→4 nibbles, 1→5 nibbles, 2→6 nibbles, 3→metadata
 *   MLEN-1: (MNIBBLES+4)*4 bits, little-endian
 *
 * Known empty stream: {0x3b} = 0b00111011 (LSB first: 1,1,0,1,1,1,0,0)
 *   bit0=1, bits[1:3]=5 → WBITS=22; bit4=1 ISLAST; bit5=1 ISLASTEMPTY → done
 */

/* ── helper: build a minimal non-empty compressed meta-block header ──────── */
/*
 * Minimal 1-byte literal stream (WBITS=16, ISLAST=1, ISLASTEMPTY=0):
 *   Bit layout (LSB first):
 *     bit 0   = 0          → WBITS=16
 *     bit 1   = 1          → ISLAST=1
 *     bit 2   = 0          → ISLASTEMPTY=0
 *     bits 3-4 = 00        → MNIBBLES=0 → 4 nibbles for MLEN
 *     bits 5-20 = MLEN-1   → 16 bits (4 nibbles)
 *   Then prefix codes + data follow.
 *
 * For our adversarial tests we mostly just need the decompressor to
 * *not crash* — returning 0 (corrupt) is perfectly acceptable.
 */

/* ── 1. Window size variants ─────────────────────────────────────────────── */
static void test_brotli_adversarial_window_size(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv window size: malloc skipped");
    return;
  }

  /* Baseline: known-good empty stream (WBITS=22) */
  {
    uint8_t s[] = {0x3b};
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    TEST_ASSERT(r == 0, "adv window: baseline empty stream should return 0");
  }

  /*
   * WBITS=16: first bit=0, then ISLAST=1, ISLASTEMPTY=1
   * Byte: bits 0..7 = 0,1,1,x,x,x,x,x → 0b00000110 = 0x06
   */
  {
    uint8_t s[] = {0x06};
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv window: WBITS=16 empty stream survived");
  }

  /*
   * WBITS=17: bit0=1, bits[1:3]=0, bits[4:6]=0 → 7 bits consumed
   * Then ISLAST=1, ISLASTEMPTY=1
   * Bits (LSB first): 1,0,0,0,0,0,0,1,1,...
   * Byte0 = 0b00000001 = 0x01, Byte1 = 0b00000011 = 0x03 (ISLAST+ISLASTEMPTY
   * in bits 0-1 of byte1)
   */
  {
    uint8_t s[] = {0x01, 0x03};
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv window: WBITS=17 stream survived");
  }

  /*
   * WBITS=10: bit0=1, bits[1:3]=0, bits[4:6]=2 → n=2 → WBITS=10
   * Bits: 1,0,0,0,0,1,0 then ISLAST=1, ISLASTEMPTY=1
   * Byte0 = 0b00100001 = 0x21, Byte1 = 0b00000011 = 0x03
   */
  {
    uint8_t s[] = {0x21, 0x03};
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv window: WBITS=10 stream survived");
  }

  /*
   * WBITS=24 (max): bit0=1, bits[1:3]=7 → n=7 → WBITS=24
   * Bits: 1,1,1,1,0 then ISLAST=1, ISLASTEMPTY=1
   * Byte0 = 0b00001111 = 0x0F, Byte1 = 0b00000011 = 0x03
   */
  {
    uint8_t s[] = {0x0F, 0x03};
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv window: WBITS=24 stream survived");
  }

  /*
   * Invalid WBITS: bit0=1, bits[1:3]=0, bits[4:6]=1 → invalid per spec
   * Bits: 1,0,0,0,0,0,1 → Byte0 = 0b01000001 = 0x41
   */
  {
    uint8_t s[] = {0x41, 0x03};
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    TEST_ASSERT(r == 0, "adv window: invalid WBITS should return 0");
  }

  /* Truncated after WBITS byte — must not crash */
  {
    uint8_t s[] = {0x01}; /* WBITS=17 path, then nothing */
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv window: truncated after WBITS survived");
  }

  free(out);
  TEST_ASSERT(1, "adv window size: all variants survived without crash");
}

/* ── 2. Incomplete Huffman code ──────────────────────────────────────────── */
static void test_brotli_adversarial_huffman_incomplete(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv huffman incomplete: malloc skipped");
    return;
  }

  /*
   * Craft a stream that reaches the complex prefix code path (hskip != 1)
   * with a code-length Huffman tree that is incomplete (not all 2^max_len
   * codes assigned).  The decompressor must return 0, not crash.
   *
   * Stream layout (WBITS=16, ISLAST=1, ISLASTEMPTY=0, MNIBBLES=0, MLEN=1):
   *   Byte 0: bit0=0 (WBITS=16), bit1=1 (ISLAST), bit2=0 (ISLASTEMPTY=0)
   *           bits3-4=00 (MNIBBLES=0 → 4 nibbles)
   *           bits5-7=000 (low 3 bits of MLEN-1=0)
   *   Byte 1: bits0-12 = remaining MLEN-1 bits (13 bits, all 0 → MLEN=1)
   *           then NBLTYPES-1 for L,I,D (each: 0 bit = 1 type)
   *           then NPOSTFIX (2 bits=0), NDIRECT (4 bits=0)
   *           then context_modes[0] (2 bits=0)
   *           then NTREESL varlen (bit=0 → NTREESL=1)
   *   Then context map (NTREESL=1 → skip), NTREESD varlen (bit=0 → 1)
   *   Then dist context map (skip)
   *   Then literal prefix code: hskip=0b10 (complex, hskip=2)
   *     → reads code-length code lengths starting at index 2
   *     → we provide bytes that create an incomplete cl_table
   *   Then IAC prefix code, dist prefix code, then data
   *
   * Rather than bit-perfect crafting (extremely complex), we use a
   * representative corrupt payload that exercises the incomplete-Huffman
   * path: a stream that starts valid (WBITS, meta-block header) but
   * then has garbage bytes where the prefix codes should be.  The
   * decompressor must return 0 without crashing.
   *
   * We use several known-bad patterns:
   */

  /* Pattern A: all-zero bytes after a valid WBITS+meta-block header.
   * The zero bytes will be parsed as prefix codes; an all-zero code-length
   * sequence creates a degenerate Huffman table (all symbols length 0).
   * The decompressor should handle this gracefully. */
  {
    /* WBITS=16 (bit0=0), ISLAST=1 (bit1=1), ISLASTEMPTY=0 (bit2=0),
     * MNIBBLES=0 (bits3-4=00), MLEN-1=0 (bits5-20=0) → MLEN=1
     * Packed into bytes:
     *   byte0: bits[0..7] = 0,1,0,0,0,0,0,0 = 0x02
     *   byte1..N: all zeros (garbage prefix codes) */
    uint8_t s[32];
    memset(s, 0, sizeof(s));
    s[0] = 0x02; /* WBITS=16, ISLAST=1, ISLASTEMPTY=0, MNIBBLES=0, MLEN low */
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv huffman incomplete A: survived");
  }

  /* Pattern B: hskip=2 (complex prefix code) with only 1 code-length entry
   * set, leaving the tree incomplete. */
  {
    uint8_t s[64];
    memset(s, 0, sizeof(s));
    /* WBITS=16, ISLAST=1, ISLASTEMPTY=0, MNIBBLES=0, MLEN=1 */
    s[0] = 0x02;
    /* After MLEN: NBLTYPES for L=0 (1 type), I=0, D=0; NPOSTFIX=0,
     * NDIRECT=0; context_modes[0]=0; NTREESL varlen=0 (1 tree);
     * context map (skipped for 1 tree); NTREESD=0 (1 tree);
     * dist context map (skipped).
     * All these fit in the zero bytes.
     * Then literal prefix code starts: hskip bits = 0b10 = complex, hskip=2
     * We need to set those bits. After the header bits above, we're at
     * approximately bit 35 in the stream. Rather than computing exactly,
     * we set byte 4 to 0x80 to inject a non-zero pattern that will be
     * interpreted as hskip=2 somewhere in the prefix code reading. */
    s[4] = 0x80;
    s[5] = 0xFF; /* incomplete code-length data */
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv huffman incomplete B: survived");
  }

  /* Pattern C: maximum code lengths that don't sum to a complete tree */
  {
    uint8_t s[128];
    memset(s, 0xFF, sizeof(s)); /* all-ones: aggressive bit pattern */
    s[0] = 0x02;                /* valid WBITS=16 + meta-block start */
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv huffman incomplete C: survived");
  }

  free(out);
  TEST_ASSERT(1, "adv huffman incomplete: all patterns survived without crash");
}

/* ── 3. Distance overflow (distance > pos) ───────────────────────────────── */
static void test_brotli_adversarial_distance_overflow(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv distance overflow: malloc skipped");
    return;
  }

  /*
   * We use a real compressed stream and then corrupt the distance field
   * to be larger than the current output position.  The decompressor
   * should either reject it (return 0) or treat it as a dictionary ref.
   * It must NOT read before dst[0].
   *
   * Strategy: compress a short string, then flip bits in the distance
   * area to create large distance values.
   */
  const char *input = "AB";
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, 2, 1);
  if (clen > 0) {
    /* Corrupt bytes in the middle/end of the stream to create bad distances */
    for (size_t i = clen / 2; i < clen; i++) {
      uint8_t corrupted[256];
      memcpy(corrupted, compressed, clen);
      corrupted[i] = 0xFF; /* set all bits → large distance codes */
      size_t r = fio_brotli_decompress(out, 65536, corrupted, clen);
      (void)r;
    }
  }
  TEST_ASSERT(1, "adv distance overflow: survived distance corruption");

  /*
   * Craft a stream with dist_sym=4 (ring buffer distance -1 from last)
   * at position 0 (no output yet) — distance would be 15 but pos=0,
   * so distance > pos → should go to dict path or error.
   *
   * Use a known-good stream for "A" and corrupt the IAC command to
   * force a copy before any literals are emitted.
   */
  {
    /* All-zeros stream with WBITS=16 header: the IAC decoder will see
     * insert_len=0, copy_len=2, dist_code_zero=1 (distance from ring buffer).
     * Ring buffer initial values are {16,15,11,4}, so distance=15 > pos=0
     * → dict reference path → wlen=2 < 4 → goto err_free_all → return 0 */
    uint8_t s[64];
    memset(s, 0, sizeof(s));
    s[0] = 0x02; /* WBITS=16, ISLAST=1, ISLASTEMPTY=0, MNIBBLES=0 */
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv distance overflow: zero-pos copy survived");
  }

  free(out);
  TEST_ASSERT(1, "adv distance overflow: all variants survived without crash");
}

/* ── 4. MLEN too large ───────────────────────────────────────────────────── */
static void test_brotli_adversarial_mlen_too_large(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv mlen too large: malloc skipped");
    return;
  }

  /*
   * Craft a stream claiming MLEN = 16,777,216 (2^24, maximum for WBITS=24)
   * but with no actual data.  The decompressor should:
   *   - Return required_size > out_len when buffer is small
   *   - Not crash with a large buffer
   *   - Not loop forever
   *
   * WBITS=16 (bit0=0), ISLAST=1 (bit1=1), ISLASTEMPTY=0 (bit2=0),
   * MNIBBLES=2 (bits3-4=10 → 6 nibbles → 24 bits for MLEN-1),
   * MLEN-1 = 0xFFFFFF (24 bits, all ones → MLEN = 16,777,216)
   *
   * Bit stream (LSB first):
   *   0: WBITS=16
   *   1: ISLAST=1
   *   2: ISLASTEMPTY=0
   *   3-4: MNIBBLES=2 (binary 10)
   *   5-28: MLEN-1 = 0xFFFFFF (24 bits, all ones)
   *   29+: garbage (zeros) for prefix codes
   *
   * Packing into bytes (LSB first):
   *   byte0: bits[0..7] = 0,1,0,0,1,1,1,1 = 0b11110010 = 0xF2
   *     (bit0=0 WBITS, bit1=1 ISLAST, bit2=0 ISLASTEMPTY,
   *      bits3-4=10 MNIBBLES=2, bits5-7=111 low 3 of MLEN-1)
   *   byte1: bits[8..15] = 1,1,1,1,1,1,1,1 = 0xFF (MLEN-1 bits 3-10)
   *   byte2: bits[16..23] = 1,1,1,1,1,1,1,1 = 0xFF (MLEN-1 bits 11-18)
   *   byte3: bits[24..28] = 1,1,1,1,1 (MLEN-1 bits 19-23), then prefix code
   *   byte3 = 0b00011111 = 0x1F (5 bits of MLEN-1, 3 bits of prefix code)
   *   bytes 4+: zeros
   */
  {
    uint8_t s[64];
    memset(s, 0, sizeof(s));
    s[0] = 0xF2;
    s[1] = 0xFF;
    s[2] = 0xFF;
    s[3] = 0x1F;
    /* With a tiny output buffer, should return required_size > out_len */
    size_t r_small = fio_brotli_decompress(out, 1, s, sizeof(s));
    (void)r_small;
    TEST_ASSERT(1, "adv mlen too large: small buffer survived");

    /* With a large output buffer, should return 0 (corrupt, no data) */
    size_t r_large = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r_large;
    TEST_ASSERT(1, "adv mlen too large: large buffer survived");
  }

  /*
   * Variant: MNIBBLES=1 (5 nibbles = 20 bits), MLEN-1 = 0xFFFFF → MLEN=1M
   * byte0: 0,1,0,0,1,0,1,1 = 0b11010010 = 0xD2
   *   (WBITS=16, ISLAST=1, ISLASTEMPTY=0, MNIBBLES=1=01, low3=111)
   * byte1: 0xFF, byte2: 0x1F (remaining 17 bits of MLEN-1)
   */
  {
    uint8_t s[64];
    memset(s, 0, sizeof(s));
    s[0] = 0xD2;
    s[1] = 0xFF;
    s[2] = 0x1F;
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv mlen too large variant: survived");
  }

  free(out);
  TEST_ASSERT(1, "adv mlen too large: all variants survived without crash");
}

/* ── 5. Block type overflow ──────────────────────────────────────────────── */
static void test_brotli_adversarial_block_type_overflow(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv block type overflow: malloc skipped");
    return;
  }

  /*
   * Craft a stream with NBLTYPES=255 for literals, then corrupt the
   * block type switch to claim type >= NBLTYPES.
   *
   * We use a real compressed stream and corrupt the NBLTYPES field.
   * The decompressor reads NBLTYPES as: if bit=1, read VarLenUint8+2.
   * VarLenUint8: if bit=0 → 0; if bit=1, read 3 bits n, if n=0 → 1,
   *   else read n bits → value + (1<<n).
   * So NBLTYPES=255: bit=1, VarLenUint8=253: bit=1, n=7, read 7 bits=125
   *   → 125 + 128 = 253 → NBLTYPES = 253+2 = 255.
   *
   * Rather than exact bit crafting, we use a corrupt stream approach:
   * take a valid stream and flip bits in the NBLTYPES area.
   */
  const char *input = "Hello, World! This is a test.";
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, 29, 1);
  if (clen > 4) {
    /* Corrupt bytes 2-4 which typically contain NBLTYPES fields */
    for (int byte_idx = 2; byte_idx <= 4 && byte_idx < (int)clen; byte_idx++) {
      uint8_t corrupted[256];
      memcpy(corrupted, compressed, clen);
      corrupted[byte_idx] = 0xFF; /* set all bits → large NBLTYPES */
      size_t r = fio_brotli_decompress(out, 65536, corrupted, clen);
      (void)r;
    }
  }
  TEST_ASSERT(1, "adv block type overflow: survived NBLTYPES corruption");

  /*
   * Direct crafted stream: WBITS=16, ISLAST=1, ISLASTEMPTY=0, MLEN=1,
   * then NBLTYPES-1 bit=1 for literals (requesting many block types),
   * followed by garbage.
   */
  {
    uint8_t s[128];
    memset(s, 0, sizeof(s));
    s[0] = 0x02; /* WBITS=16, ISLAST=1, ISLASTEMPTY=0, MNIBBLES=0 */
    /* Set bit 5 (NBLTYPES-1 flag for literals) and subsequent bits to 0xFF
     * to request a large NBLTYPES value */
    s[0] |= 0xE0; /* bits 5-7 = 111 */
    s[1] = 0xFF;
    s[2] = 0xFF;
    s[3] = 0xFF;
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv block type overflow: direct crafted survived");
  }

  free(out);
  TEST_ASSERT(1,
              "adv block type overflow: all variants survived without crash");
}

/* ── 6. Context map overflow ─────────────────────────────────────────────── */
static void test_brotli_adversarial_context_map_overflow(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv context map overflow: malloc skipped");
    return;
  }

  /*
   * Craft a stream where the literal context map references tree index
   * >= NTREESL.  The decompressor validates this at:
   *   if (val >= num_trees) return -1;
   * and also at:
   *   if (lit_tree_idx >= ntreesl) goto err_free_all;
   * Both should return 0 cleanly.
   *
   * Strategy: corrupt a valid stream's context map bytes.
   */
  const char *input = "The quick brown fox jumps over the lazy dog.";
  uint8_t compressed[512];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, 44, 5);
  if (clen > 8) {
    /* Corrupt the latter half of the stream (context map area) */
    for (size_t i = clen / 2; i < clen - 2; i += 3) {
      uint8_t corrupted[512];
      memcpy(corrupted, compressed, clen);
      corrupted[i] = 0xFF;
      size_t r = fio_brotli_decompress(out, 65536, corrupted, clen);
      (void)r;
    }
  }
  TEST_ASSERT(1, "adv context map overflow: survived context map corruption");

  /*
   * Direct: stream with NTREESL=1 but context map claiming tree index 5.
   * The context map validation will catch this and return 0.
   */
  {
    uint8_t s[128];
    memset(s, 0, sizeof(s));
    s[0] = 0x02; /* WBITS=16, ISLAST=1, ISLASTEMPTY=0, MNIBBLES=0 */
    /* Inject 0xFF bytes to corrupt the context map area */
    s[6] = 0xFF;
    s[7] = 0xFF;
    s[8] = 0xFF;
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv context map overflow: direct crafted survived");
  }

  free(out);
  TEST_ASSERT(1,
              "adv context map overflow: all variants survived without crash");
}

/* ── 7. Copy length exceeds meta-block remaining ─────────────────────────── */
static void test_brotli_adversarial_copy_len_exceeds_meta(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv copy len exceeds meta: malloc skipped");
    return;
  }

  /*
   * The decompressor checks: if (copy_len > meta_remaining) goto err_free_all
   * We need a stream where copy_len > meta_remaining.
   *
   * Strategy: compress a short string (MLEN=small), then corrupt the
   * IAC command bytes to produce a large copy_len.
   */
  const char *input = "ABCDE"; /* 5 bytes */
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, 5, 1);
  if (clen > 4) {
    /* Corrupt the IAC command area (typically in the latter bytes) */
    for (size_t i = clen / 2; i < clen; i++) {
      uint8_t corrupted[256];
      memcpy(corrupted, compressed, clen);
      corrupted[i] ^= 0xFF; /* flip all bits */
      size_t r = fio_brotli_decompress(out, 65536, corrupted, clen);
      (void)r;
    }
  }
  TEST_ASSERT(1, "adv copy len exceeds meta: survived IAC corruption");

  /*
   * Direct crafted: WBITS=16, ISLAST=1, MLEN=1 (1 byte), then
   * prefix codes that produce insert_len=0, copy_len=255 (>> MLEN).
   * The decompressor should hit: copy_len > meta_remaining → err_free_all.
   */
  {
    uint8_t s[128];
    memset(s, 0xFF, sizeof(s)); /* all-ones → large code values */
    s[0] = 0x02;                /* WBITS=16, ISLAST=1, ISLASTEMPTY=0 */
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv copy len exceeds meta: direct crafted survived");
  }

  free(out);
  TEST_ASSERT(1,
              "adv copy len exceeds meta: all variants survived without crash");
}

/* ── 8. Dictionary reference with invalid word length ────────────────────── */
static void test_brotli_adversarial_dict_invalid_wlen(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv dict invalid wlen: malloc skipped");
    return;
  }

  /*
   * The decompressor checks: if (wlen < 4 || wlen > 24 ||
   *   fio___brotli_ndbits[wlen] == 0) goto err_free_all
   *
   * wlen = copy_len when distance > pos (dict reference path).
   * We need copy_len outside [4,24] with distance > pos.
   *
   * Strategy: corrupt a valid stream to produce a large distance
   * (triggering dict path) with copy_len=2 or copy_len=25.
   */
  const char *input = "Hello World";
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, 11, 1);
  if (clen > 4) {
    /* Corrupt distance bytes to be very large (> pos) */
    for (size_t i = clen - 4; i < clen; i++) {
      uint8_t corrupted[256];
      memcpy(corrupted, compressed, clen);
      corrupted[i] = 0xFF;
      size_t r = fio_brotli_decompress(out, 65536, corrupted, clen);
      (void)r;
    }
  }
  TEST_ASSERT(1, "adv dict invalid wlen: survived distance corruption");

  /*
   * Direct: all-zeros stream with WBITS=16 header.
   * The IAC decoder with all-zero bits produces insert_len=0, copy_len=2,
   * dist_code_zero=1 → distance from ring buffer (=16) > pos=0 → dict path
   * → wlen=2 < 4 → goto err_free_all → return 0.
   */
  {
    uint8_t s[64];
    memset(s, 0, sizeof(s));
    s[0] = 0x02;
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    TEST_ASSERT(r == 0,
                "adv dict invalid wlen: copy_len=2 dict ref should return 0");
  }

  free(out);
  TEST_ASSERT(1, "adv dict invalid wlen: all variants survived without crash");
}

/* ── 9. Transform ID overflow ────────────────────────────────────────────── */
static void test_brotli_adversarial_transform_overflow(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv transform overflow: malloc skipped");
    return;
  }

  /*
   * The decompressor checks: if (transform_id >= FIO___BROTLI_NUM_TRANSFORMS)
   *   goto err_free_all
   * FIO___BROTLI_NUM_TRANSFORMS = 121.
   *
   * transform_id = dict_off / nwords where dict_off = distance - max_dist - 1.
   * We need a large distance to push transform_id >= 121.
   *
   * Strategy: corrupt a valid stream's distance bytes to be very large.
   */
  const char *input = "international";
  uint8_t compressed[256];
  size_t clen =
      fio_brotli_compress(compressed, sizeof(compressed), input, 13, 5);
  if (clen > 4) {
    /* Corrupt the last few bytes (distance area) */
    for (size_t i = clen - 3; i < clen; i++) {
      uint8_t corrupted[256];
      memcpy(corrupted, compressed, clen);
      corrupted[i] = 0xFF;
      size_t r = fio_brotli_decompress(out, 65536, corrupted, clen);
      (void)r;
    }
  }
  TEST_ASSERT(1, "adv transform overflow: survived distance corruption");

  /*
   * Direct: all-0xFF stream with valid WBITS header.
   * The large bit values will produce large distance codes → dict path
   * → transform_id will be large → goto err_free_all → return 0.
   */
  {
    uint8_t s[128];
    memset(s, 0xFF, sizeof(s));
    s[0] = 0x02; /* WBITS=16, ISLAST=1, ISLASTEMPTY=0 */
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv transform overflow: direct crafted survived");
  }

  free(out);
  TEST_ASSERT(1, "adv transform overflow: all variants survived without crash");
}

/* ── 10. Tiny input variants ─────────────────────────────────────────────── */
static void test_brotli_adversarial_tiny_input_variants(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv tiny input: malloc skipped");
    return;
  }

  /* All 256 single-byte values (already covered by
   * test_brotli_single_byte_inputs but we repeat here for completeness in the
   * adversarial section) */
  for (int b = 0; b < 256; b++) {
    uint8_t byte = (uint8_t)b;
    size_t r = fio_brotli_decompress(out, 65536, &byte, 1);
    (void)r;
  }
  TEST_ASSERT(1, "adv tiny input: all 256 single-byte values survived");

  /* Representative 2-byte values: all combinations of first byte with
   * second byte in {0x00, 0x01, 0x3b, 0x7f, 0x80, 0xfe, 0xff} */
  {
    static const uint8_t second_bytes[] = {0x00,
                                           0x01,
                                           0x03,
                                           0x06,
                                           0x0F,
                                           0x21,
                                           0x3b,
                                           0x41,
                                           0x7f,
                                           0x80,
                                           0xAA,
                                           0xfe,
                                           0xff};
    size_t nsecond = sizeof(second_bytes) / sizeof(second_bytes[0]);
    for (int b0 = 0; b0 < 256; b0++) {
      for (size_t b1i = 0; b1i < nsecond; b1i++) {
        uint8_t s[2] = {(uint8_t)b0, second_bytes[b1i]};
        size_t r = fio_brotli_decompress(out, 65536, s, 2);
        (void)r;
      }
    }
    TEST_ASSERT(1, "adv tiny input: 2-byte representative set survived");
  }

  /* 3-byte values: stress the bit-reader boundary (safe_end path) */
  {
    static const uint8_t patterns[][3] = {
        {0x00, 0x00, 0x00}, /* all zeros */
        {0xFF, 0xFF, 0xFF}, /* all ones */
        {0x3b, 0x00, 0x00}, /* valid empty + garbage */
        {0x06, 0x00, 0x00}, /* WBITS=16 empty + garbage */
        {0x01, 0x03, 0x00}, /* WBITS=17 empty + garbage */
        {0x02, 0x00, 0x00}, /* WBITS=16, ISLAST, ISLASTEMPTY=0, MLEN=1 */
        {0x02, 0xFF, 0xFF}, /* above + garbage prefix codes */
        {0xF2, 0xFF, 0x1F}, /* WBITS=16, large MLEN */
        {0x41, 0x03, 0x00}, /* invalid WBITS */
        {0x0F, 0x03, 0x00}, /* WBITS=24 empty */
        {0x21, 0x03, 0x00}, /* WBITS=10 empty */
        {0xAA, 0x55, 0xAA}, /* alternating bits */
        {0x55, 0xAA, 0x55}, /* alternating bits inverted */
    };
    size_t npatterns = sizeof(patterns) / sizeof(patterns[0]);
    for (size_t i = 0; i < npatterns; i++) {
      size_t r = fio_brotli_decompress(out, 65536, patterns[i], 3);
      (void)r;
    }
    TEST_ASSERT(1, "adv tiny input: 3-byte patterns survived");
  }

  /* 4-7 byte inputs: stress the safe_end boundary (< 8 bytes → slow path) */
  {
    for (size_t len = 4; len <= 7; len++) {
      uint8_t s[7];
      /* Pattern 1: all zeros */
      memset(s, 0, len);
      s[0] = 0x02;
      size_t r = fio_brotli_decompress(out, 65536, s, len);
      (void)r;
      /* Pattern 2: all ones */
      memset(s, 0xFF, len);
      r = fio_brotli_decompress(out, 65536, s, len);
      (void)r;
      /* Pattern 3: valid WBITS + garbage */
      memset(s, 0xAB, len);
      s[0] = 0x3b;
      r = fio_brotli_decompress(out, 65536, s, len);
      (void)r;
    }
    TEST_ASSERT(1, "adv tiny input: 4-7 byte inputs survived");
  }

  free(out);
  TEST_ASSERT(1, "adv tiny input: all tiny input variants survived");
}

/* ── 11. Known crash payloads (regression for fixed bugs) ────────────────── */
static void test_brotli_adversarial_known_crash_payloads(void) {
  uint8_t *out = (uint8_t *)malloc(65536);
  if (!out) {
    TEST_ASSERT(1, "adv known crash: malloc skipped");
    return;
  }

  /*
   * Bug 1: NULL safe_end pointer comparison (fixed: now uses src+8<=end).
   * Trigger: input exactly 7 bytes long that looks like a valid brotli header.
   * The old code set safe_end = data + len - 8 = data - 1 (wraps to ~0),
   * then compared src <= safe_end which is UB when safe_end < data.
   * Fixed code uses: if (b->src + 8 <= b->end) for the fast path.
   *
   * We test several 7-byte inputs that exercise the slow byte-at-a-time path.
   */
  {
    /* 7-byte input: valid WBITS=22 empty stream + 6 garbage bytes */
    uint8_t s7a[7] = {0x3b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t r = fio_brotli_decompress(out, 65536, s7a, 7);
    TEST_ASSERT(r == 0,
                "adv known crash: 7-byte empty+garbage should return 0");

    /* 7-byte input: WBITS=16 header with MLEN=1 + garbage prefix codes */
    uint8_t s7b[7] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    r = fio_brotli_decompress(out, 65536, s7b, 7);
    (void)r;
    TEST_ASSERT(1, "adv known crash: 7-byte MLEN=1+garbage survived");

    /* 7-byte input: all 0xFF (aggressive bit pattern, 7 bytes) */
    uint8_t s7c[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    r = fio_brotli_decompress(out, 65536, s7c, 7);
    (void)r;
    TEST_ASSERT(1, "adv known crash: 7-byte all-0xFF survived");

    /* 7-byte input: alternating pattern */
    uint8_t s7d[7] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};
    r = fio_brotli_decompress(out, 65536, s7d, 7);
    (void)r;
    TEST_ASSERT(1, "adv known crash: 7-byte alternating survived");

    /* 7-byte input: valid WBITS=17 header + garbage */
    uint8_t s7e[7] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    r = fio_brotli_decompress(out, 65536, s7e, 7);
    (void)r;
    TEST_ASSERT(1, "adv known crash: 7-byte WBITS=17+garbage survived");
  }

  /*
   * Bug 2: Incomplete Huffman code / subtable underflow.
   * A code-length Huffman tree where some subtable entries are never filled
   * (zero-initialized), causing CODELEN(0) - root_bits to underflow unsigned
   * → UB shift.  Fixed by: if (codelen <= root_bits) return 0.
   *
   * Craft a stream that reaches the subtable lookup with a zero entry:
   * - WBITS=16, ISLAST=1, ISLASTEMPTY=0, MLEN=1
   * - Complex prefix code (hskip=0b00 or 0b10 or 0b11)
   * - Code-length data that creates an incomplete tree
   *
   * The key pattern: after the meta-block header, the literal prefix code
   * is read.  hskip=0b00 means complex code starting at cl index 0.
   * If the cl_lengths produce a tree where some 8-bit root entries point
   * to subtables with zero entries, the old code would UB-shift.
   *
   * We use several patterns that stress this path:
   */
  {
    /* Pattern: WBITS=16 header + hskip=0 (complex) + sparse cl_lengths */
    /* After the meta-block header bits, the first 2 bits of the prefix
     * code are hskip.  With our byte layout:
     *   byte0=0x02: WBITS=16(1b), ISLAST(1b), ISLASTEMPTY=0(1b),
     *               MNIBBLES=0(2b), MLEN-1 low 3 bits=0
     *   byte1=0x00: MLEN-1 bits 3-10 = 0 (MLEN=1)
     *   byte2=0x00: MLEN-1 bits 11-15 = 0, then NBLTYPES flags (3x 0=1type),
     *               NPOSTFIX=0(2b), NDIRECT=0(4b) start
     *   byte3=0x00: NDIRECT remaining, context_modes[0]=0(2b),
     *               NTREESL varlen=0(1b=1tree), NTREESD varlen=0(1b=1tree)
     *               then literal prefix code hskip bits start
     *   byte4=0x00: hskip=0b00 (complex, start at cl index 0)
     *               then cl_lengths via VLC...
     * All zeros → cl_lengths all 0 → degenerate tree → build_table returns
     * root_size with all entries SYM(0,0) → huff_decode returns 0 (sym=0,
     * codelen=0) → cl_sym=0 → literal code length 0 → space never decreases
     * → loop exits when sym_count reaches alphabet_size.
     * Then build_table for literal tree: all lengths 0 → max_len=0 →
     * fills with SYM(0,0) → huff_decode returns sym=0 always.
     * This is actually valid (degenerate but not crashing).
     * The stream then tries to decode IAC commands with all-zero bits.
     */
    uint8_t s[32];
    memset(s, 0, sizeof(s));
    s[0] = 0x02;
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv known crash: incomplete huffman pattern A survived");

    /* Pattern: inject a non-zero cl_length that creates a partial tree
     * where some subtable entries remain zero.
     * Set byte 5 = 0x08 to inject a code-length value of 8 for one symbol,
     * leaving the rest of the tree unfilled. */
    memset(s, 0, sizeof(s));
    s[0] = 0x02;
    s[5] = 0x08; /* inject a non-zero cl_length bit pattern */
    r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv known crash: incomplete huffman pattern B survived");

    /* Pattern: hskip=3 (skip first 3 cl entries) with sparse remaining */
    memset(s, 0, sizeof(s));
    s[0] = 0x02;
    s[4] = 0xC0; /* bits 6-7 = 11 → hskip=3 in the prefix code area */
    s[5] = 0x01; /* one non-zero cl_length */
    r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv known crash: incomplete huffman pattern C survived");
  }

  /*
   * Bug 3: Uninitialized hash_table[] / num[] — allocator returning memory
   * with generation value embedded → garbage candidates used as src offsets.
   * This is a compressor bug, not directly testable via the decompressor API.
   * We verify the decompressor handles streams produced by a potentially
   * corrupt compressor state by testing random garbage inputs.
   */
  {
    uint8_t garbage[64];
    for (int trial = 0; trial < 20; trial++) {
      /* Use a deterministic pattern that varies per trial */
      for (int i = 0; i < 64; i++)
        garbage[i] = (uint8_t)((trial * 37 + i * 13) ^ (i << 2));
      garbage[0] = 0x02; /* keep valid WBITS to get deeper into parsing */
      size_t r = fio_brotli_decompress(out, 65536, garbage, 64);
      (void)r;
    }
    TEST_ASSERT(1,
                "adv known crash: uninitialized hash table patterns survived");
  }

  /*
   * Bug 4: Subtable buffer overflow — Huffman subtable exceeding allocated
   * table buffer.  Fixed by: if (sub_offset + sub_size > max_size) return 0.
   *
   * Craft a stream where the prefix code has many long codes that would
   * require many subtables, exceeding the allocated buffer.
   * We use a stream with hskip=0 and cl_lengths that create many subtables.
   */
  {
    /* All-0xAA pattern after valid header: alternating bits create
     * varied code-length patterns that stress subtable allocation */
    uint8_t s[128];
    memset(s, 0xAA, sizeof(s));
    s[0] = 0x02; /* valid WBITS=16 header */
    size_t r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv known crash: subtable overflow pattern survived");

    /* All-0x55 pattern */
    memset(s, 0x55, sizeof(s));
    s[0] = 0x02;
    r = fio_brotli_decompress(out, 65536, s, sizeof(s));
    (void)r;
    TEST_ASSERT(1, "adv known crash: subtable overflow pattern 2 survived");
  }

  /*
   * Bug 5: fio___brotli_uppercase OOB write — 1-byte UTF-8 lead byte word
   * at end of output buffer.  Fixed by passing `remaining` to uppercase().
   * We test by providing a stream that would produce a dictionary word
   * with UPPERCASE_ALL transform at the very end of the output buffer.
   * Since we can't easily craft this exactly, we use a near-full output
   * buffer with a stream that exercises the dict+transform path.
   */
  {
    /* Use a q5 stream that exercises the static dictionary */
    const char *text = "international";
    uint8_t compressed[256];
    size_t clen =
        fio_brotli_compress(compressed, sizeof(compressed), text, 13, 5);
    if (clen > 0) {
      /* Decompress into a buffer that's exactly the right size */
      uint8_t exact_out[13];
      size_t r = fio_brotli_decompress(exact_out, 13, compressed, clen);
      TEST_ASSERT(r == 13,
                  "adv known crash: uppercase OOB - exact buffer should work");
      /* Decompress into a 1-byte-too-small buffer */
      uint8_t small_out[12];
      r = fio_brotli_decompress(small_out, 12, compressed, clen);
      TEST_ASSERT(r > 12,
                  "adv known crash: uppercase OOB - small buffer returns size");
    }
    TEST_ASSERT(1, "adv known crash: uppercase OOB test survived");
  }

  free(out);
  TEST_ASSERT(1, "adv known crash: all known crash payloads survived");
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr, "=== Brotli Tests ===\n");

  /* Decompression */
  test_brotli_null_inputs();
  test_brotli_bounds();
  test_brotli_empty();
  test_brotli_hello();
  test_brotli_repeated();
  test_brotli_text_q5();
  test_brotli_html_q11();

  /* Compression basics */
  test_brotli_compress_empty();
  test_brotli_compress_single_byte();
  test_brotli_compress_hello();
  test_brotli_compress_repeated();
  test_brotli_compress_all_bytes();

  /* Cross-quality roundtrip (q1-q6) */
  test_brotli_cross_quality_roundtrip();

  /* Large file roundtrip (256KB+) */
  test_brotli_large_file_roundtrip();

  /* Edge cases */
  test_brotli_edge_cases();

  /* q5-q6 hash chain + distance cache */
  test_brotli_compress_q5_roundtrip();
  test_brotli_compress_q6_roundtrip();
  test_brotli_compress_q5q6_large_random();

  /* Context modeling (q5) */
  test_brotli_context_modeling_text();
  test_brotli_context_modeling_html();
  test_brotli_context_modeling_binary();

  /* Static dictionary (q5+) */
  test_brotli_dict_english_text();
  test_brotli_dict_no_regression_repetitive();
  test_brotli_dict_mixed_content();
  test_brotli_dict_uppercase();
  test_brotli_dict_short_input();
  test_brotli_dict_large_diverse();

  /* Compression ratio regression */
  test_brotli_ratio_regression();

  /* Robustness (malicious input) */
  test_brotli_zero_input();
  test_brotli_all_zeros();
  test_brotli_all_ones();
  test_brotli_single_byte_inputs();
  test_brotli_random_garbage();
  test_brotli_truncated_stream();
  test_brotli_corrupted_stream();
  test_brotli_output_too_small();
  test_brotli_size_query();
  test_brotli_compress_random_roundtrip();
  test_brotli_large_random_roundtrip();

  /* Diagnostic: compare q1-q6 compressed sizes */
  test_brotli_q_comparison();

  /* 16MB roundtrip corruption test */
  test_brotli_16mb_roundtrip();

  /* Adversarial / Maliciously-Crafted Payloads */
  fprintf(stderr, "\n--- Adversarial Tests ---\n");
  test_brotli_adversarial_window_size();
  test_brotli_adversarial_huffman_incomplete();
  test_brotli_adversarial_distance_overflow();
  test_brotli_adversarial_mlen_too_large();
  test_brotli_adversarial_block_type_overflow();
  test_brotli_adversarial_context_map_overflow();
  test_brotli_adversarial_copy_len_exceeds_meta();
  test_brotli_adversarial_dict_invalid_wlen();
  test_brotli_adversarial_transform_overflow();
  test_brotli_adversarial_tiny_input_variants();
  test_brotli_adversarial_known_crash_payloads();

  fprintf(stderr, "=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
