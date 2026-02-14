/* *****************************************************************************
BROTLI Tests (Decompression + Compression)
***************************************************************************** */
#include "test-helpers.h"
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
Test: empty Brotli stream
***************************************************************************** */

static void test_brotli_empty(void) {
  fprintf(stderr, "Testing brotli: empty stream...\n");
  /* Empty Brotli stream: single byte 0x3B
   * WBITS=0 (bit0=0 -> WBITS=16), ISLAST=1, ISLASTEMPTY=1
   * Binary: 0 0 1 1 1 0 1 1 = 0x3B? Let me verify:
   * bit0=0 -> WBITS=16
   * bit1=1 -> ISLAST=1
   * bit2=1 -> ISLASTEMPTY=1
   * Remaining bits: padding
   * So byte = 0b00000110 = 0x06? No...
   * Actually: bits are packed LSB first.
   * bit0 (WBITS first bit) = 0
   * bit1 (ISLAST) = 1
   * bit2 (ISLASTEMPTY) = 1
   * bits 3-7 = 0 (padding)
   * byte = 0b00000110 = 0x06
   *
   * But Python brotli.compress(b'') gives 0x3b.
   * 0x3b = 0b00111011
   * bit0 = 1 -> WBITS needs more bits
   * bits 1-3 = 101 = 5 -> WBITS = 5 + 17 = 22
   * bit4 = 1 -> ISLAST = 1
   * bit5 = 1 -> ISLASTEMPTY = 1
   * bits 6-7 = 00 padding
   * That makes sense! The Python encoder chose WBITS=22.
   */
  uint8_t compressed[] = {0x3b};
  uint8_t out[64];
  size_t result =
      fio_brotli_decompress(out, sizeof(out), compressed, sizeof(compressed));
  TEST_ASSERT(result == 0, "empty stream: expected 0 bytes, got %zu", result);
}

/* *****************************************************************************
Test: simple uncompressed-like stream (quality 0/1)
***************************************************************************** */

static void test_brotli_hello(void) {
  fprintf(stderr, "Testing brotli: 'Hello, World!' q1...\n");
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
  if (result == expected_len) {
    TEST_ASSERT(memcmp(out, expected, expected_len) == 0,
                "hello q1: data mismatch");
  }
}

/* *****************************************************************************
Test: repeated data with LZ77 backreferences
***************************************************************************** */

static void test_brotli_repeated(void) {
  fprintf(stderr, "Testing brotli: repeated data q1...\n");
  /* brotli.compress(b'ABCDEFGH' * 100, quality=1) */
  uint8_t compressed[] = {0x8b, 0x8f, 0x01, 0x00, 0x80, 0xaa, 0xaa, 0xaa,
                          0xea, 0xff, 0x7c, 0xe6, 0x65, 0x81, 0x03, 0xb8,
                          0xf8, 0x95, 0x2e, 0x55, 0x0c, 0x36, 0x18, 0x73,
                          0xec, 0x28, 0xa0, 0x9c, 0xee, 0x3d, 0x34, 0x03};
  size_t expected_len = 800; /* 8 * 100 */

  /* Build expected output */
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
  if (result == expected_len) {
    TEST_ASSERT(memcmp(out, expected, expected_len) == 0,
                "html q11: data mismatch");
  }
}

/* *****************************************************************************
Test: quality 5 compressed text
***************************************************************************** */

static void test_brotli_text_q5(void) {
  fprintf(stderr, "Testing brotli: text q5...\n");
  /* brotli.compress(b'The quick brown fox jumps over the lazy dog. ' * 20,
   * quality=5) */
  uint8_t compressed[] = {0x1b, 0x83, 0x03, 0x00, 0x44, 0xdb, 0x46, 0xa9, 0x2e,
                          0x24, 0x5b, 0x32, 0x14, 0xc5, 0x53, 0x91, 0x67, 0x72,
                          0xf2, 0xe7, 0x28, 0x50, 0x15, 0x98, 0x57, 0xb6, 0xb2,
                          0x59, 0xd0, 0x6c, 0xe1, 0x95, 0xa7, 0x23, 0xf2, 0xa2,
                          0xac, 0x36, 0x26, 0xb8, 0x45, 0x1f, 0x18, 0x27, 0x62,
                          0x75, 0xff, 0x21, 0x30, 0x19, 0x00};
  const char *phrase = "The quick brown fox jumps over the lazy dog. ";
  size_t phrase_len = 45;
  size_t expected_len = phrase_len * 20; /* 900 */

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
  if (result == expected_len) {
    TEST_ASSERT(memcmp(out, expected, expected_len) == 0,
                "text q5: data mismatch");
  }
}

/* *****************************************************************************
Test: quality 11 with static dictionary (HTML content)
***************************************************************************** */

static void test_brotli_html_q11(void) {
  fprintf(stderr, "Testing brotli: HTML q11 (static dict)...\n");
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
  if (result == expected_len) {
    if (memcmp(out, expected, expected_len) != 0) {
      fprintf(stderr, "  Expected: '%.*s'\n", (int)expected_len, expected);
      fprintf(stderr, "  Got:      '%.*s'\n", (int)result, (char *)out);
      for (size_t i = 0; i < expected_len; i++) {
        if (out[i] != (uint8_t)expected[i]) {
          fprintf(stderr,
                  "  First diff at byte %zu: expected 0x%02x '%c', got 0x%02x "
                  "'%c'\n",
                  i,
                  (uint8_t)expected[i],
                  expected[i],
                  out[i],
                  out[i]);
          break;
        }
      }
      TEST_ASSERT(0, "html q11: data mismatch");
    }
  }
}

/* *****************************************************************************
Test: decompress_bound
***************************************************************************** */

static void test_brotli_bounds(void) {
  fprintf(stderr, "Testing brotli: bound functions...\n");
  TEST_ASSERT(fio_brotli_decompress_bound(100) > 100,
              "decompress_bound should be > input");
  TEST_ASSERT(fio_brotli_compress_bound(100) > 100,
              "compress_bound should be > input");
  TEST_ASSERT(fio_brotli_decompress_bound(0) >= 1024,
              "decompress_bound(0) should have minimum");
}

/* *****************************************************************************
Test: NULL/zero inputs
***************************************************************************** */

static void test_brotli_null_inputs(void) {
  fprintf(stderr, "Testing brotli: null/zero inputs...\n");
  uint8_t out[64];
  TEST_ASSERT(fio_brotli_decompress(NULL, 64, "x", 1) == 0,
              "null output should return 0");
  TEST_ASSERT(fio_brotli_decompress(out, 64, NULL, 1) == 0,
              "null input should return 0");
  TEST_ASSERT(fio_brotli_decompress(out, 64, "x", 0) == 0,
              "zero input length should return 0");
  TEST_ASSERT(fio_brotli_decompress(out, 0, "x", 1) == 0,
              "zero output length should return 0");
}

/* *****************************************************************************
Compression Tests
***************************************************************************** */

static void test_brotli_compress_empty(void) {
  fprintf(stderr, "Testing brotli compress: empty input...\n");
  uint8_t compressed[64];
  size_t clen = fio_brotli_compress(compressed, sizeof(compressed), NULL, 0, 1);
  TEST_ASSERT(clen > 0, "compress empty: should produce valid stream");

  /* Decompress and verify */
  uint8_t decompressed[64];
  size_t dlen = fio_brotli_decompress(decompressed,
                                      sizeof(decompressed),
                                      compressed,
                                      clen);
  TEST_ASSERT(dlen == 0, "compress empty roundtrip: expected 0, got %zu", dlen);
}

static void test_brotli_compress_single_byte(void) {
  fprintf(stderr, "Testing brotli compress: single byte...\n");
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
  if (dlen == 1) {
    TEST_ASSERT(decompressed[0] == 0x42,
                "compress single byte roundtrip: data mismatch (got 0x%02x)",
                decompressed[0]);
  }
}

static void test_brotli_compress_hello(void) {
  fprintf(stderr, "Testing brotli compress: 'Hello, World!' q1...\n");
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
  if (dlen == in_len) {
    TEST_ASSERT(memcmp(decompressed, input, in_len) == 0,
                "compress hello roundtrip: data mismatch");
  }
}

static void test_brotli_compress_repeated(void) {
  fprintf(stderr, "Testing brotli compress: repeated data q1...\n");
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

  fprintf(stderr,
          "  Compressed 800 -> %zu bytes (%.1f%%)\n",
          clen,
          (double)clen * 100.0 / 800.0);

  uint8_t decompressed[1024];
  size_t dlen = fio_brotli_decompress(decompressed,
                                      sizeof(decompressed),
                                      compressed,
                                      clen);
  TEST_ASSERT(dlen == 800,
              "compress repeated roundtrip: expected 800, got %zu",
              dlen);
  if (dlen == 800) {
    TEST_ASSERT(memcmp(decompressed, input, 800) == 0,
                "compress repeated roundtrip: data mismatch");
  }

  free(compressed);
}

static void test_brotli_compress_text(void) {
  fprintf(stderr, "Testing brotli compress: text at all quality levels...\n");
  const char *phrase = "The quick brown fox jumps over the lazy dog. ";
  size_t phrase_len = 45;
  size_t total_len = phrase_len * 20; /* 900 bytes */
  uint8_t input[900];
  for (int i = 0; i < 20; i++)
    memcpy(input + i * phrase_len, phrase, phrase_len);

  size_t bound = fio_brotli_compress_bound(total_len);
  uint8_t *compressed = (uint8_t *)malloc(bound);
  TEST_ASSERT(compressed != NULL, "compress text: malloc failed");
  if (!compressed)
    return;

  for (int q = 1; q <= 4; q++) {
    size_t clen = fio_brotli_compress(compressed, bound, input, total_len, q);
    TEST_ASSERT(clen > 0, "compress text q%d: should produce output", q);
    TEST_ASSERT(clen < total_len,
                "compress text q%d: should compress (%zu >= %zu)",
                q,
                clen,
                total_len);

    fprintf(stderr,
            "  q%d: %zu -> %zu bytes (%.1f%%)\n",
            q,
            total_len,
            clen,
            (double)clen * 100.0 / (double)total_len);

    uint8_t decompressed[1024];
    size_t dlen = fio_brotli_decompress(decompressed,
                                        sizeof(decompressed),
                                        compressed,
                                        clen);
    TEST_ASSERT(dlen == total_len,
                "compress text q%d roundtrip: expected %zu, got %zu",
                q,
                total_len,
                dlen);
    if (dlen == total_len) {
      if (memcmp(decompressed, input, total_len) != 0) {
        /* Find first mismatch */
        for (size_t i = 0; i < total_len; i++) {
          if (decompressed[i] != input[i]) {
            fprintf(stderr,
                    "  q%d: first diff at byte %zu: expected 0x%02x, "
                    "got 0x%02x\n",
                    q,
                    i,
                    input[i],
                    decompressed[i]);
            break;
          }
        }
        TEST_ASSERT(0, "compress text q%d roundtrip: data mismatch", q);
      }
    }
  }

  free(compressed);
}

static void test_brotli_compress_all_bytes(void) {
  fprintf(stderr, "Testing brotli compress: all byte values...\n");
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
  if (dlen == 256) {
    TEST_ASSERT(memcmp(decompressed, input, 256) == 0,
                "compress all bytes roundtrip: data mismatch");
  }
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr, "=== Brotli Tests ===\n\n");

  fprintf(stderr, "--- Decompression ---\n");
  test_brotli_null_inputs();
  test_brotli_bounds();
  test_brotli_empty();
  test_brotli_hello();
  test_brotli_repeated();
  test_brotli_text_q5();
  test_brotli_html_q11();

  fprintf(stderr, "\n--- Compression ---\n");
  test_brotli_compress_empty();
  test_brotli_compress_single_byte();
  test_brotli_compress_hello();
  test_brotli_compress_repeated();
  test_brotli_compress_text();
  test_brotli_compress_all_bytes();

  fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
