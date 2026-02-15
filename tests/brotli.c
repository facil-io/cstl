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

  fprintf(stderr, "=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
