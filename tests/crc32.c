/* *****************************************************************************
CRC32 Compliance Tests and Performance Benchmarks
Tests the standard CRC32 polynomial (0xEDB88320 / gzip / ITU-T V.42).
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRC32
#include FIO_INCLUDE_FILE

#include <string.h>
#include <time.h>

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
      g_fail++;                                                                \
    } else {                                                                   \
      g_pass++;                                                                \
    }                                                                          \
  } while (0)

/* *****************************************************************************
Section 1 — Compliance Tests
Known-answer vectors for CRC32 (polynomial 0xEDB88320, gzip/ISO 3309).
***************************************************************************** */

static void test_crc32_known_vectors(void) {
  fprintf(stderr, "Testing CRC32 known-answer vectors...\n");

  /* Empty string → 0x00000000 */
  {
    uint32_t crc = fio_crc32("", 0, 0);
    TEST_ASSERT(crc == 0x00000000U,
                "empty string: expected 0x00000000, got 0x%08X",
                crc);
  }

  /* "123456789" → 0xCBF43926 */
  {
    uint32_t crc = fio_crc32("123456789", 9, 0);
    TEST_ASSERT(crc == 0xCBF43926U,
                "\"123456789\": expected 0xCBF43926, got 0x%08X",
                crc);
  }

  /* "The quick brown fox jumps over the lazy dog" → 0x414FA339 */
  {
    const char *s = "The quick brown fox jumps over the lazy dog";
    uint32_t crc = fio_crc32(s, strlen(s), 0);
    TEST_ASSERT(crc == 0x414FA339U,
                "fox string: expected 0x414FA339, got 0x%08X",
                crc);
  }
}

/* *****************************************************************************
Test: incremental (chained) computation
Split "123456789" at every byte boundary and verify the chained result
matches the single-pass result (0xCBF43926).
***************************************************************************** */

static void test_crc32_incremental(void) {
  fprintf(stderr, "Testing CRC32 incremental (chained) computation...\n");

  const char *data = "123456789";
  const size_t len = 9;
  const uint32_t expected = 0xCBF43926U;

  for (size_t split = 0; split <= len; ++split) {
    uint32_t crc = fio_crc32(data, split, 0);
    crc = fio_crc32(data + split, len - split, crc);
    TEST_ASSERT(crc == expected,
                "incremental split at %zu: expected 0x%08X, got 0x%08X",
                split,
                expected,
                crc);
  }
}

/* *****************************************************************************
Test: large buffer consistency
64KB of a repeating pattern — compute twice and compare.
***************************************************************************** */

static void test_crc32_large_buffer(void) {
  fprintf(stderr, "Testing CRC32 large buffer (64KB) consistency...\n");

  const size_t buf_len = 64 * 1024;
  uint8_t *buf = (uint8_t *)malloc(buf_len);
  TEST_ASSERT(buf != NULL, "large buffer: malloc failed");
  if (!buf)
    return;

  /* Fill with a repeating pattern */
  for (size_t i = 0; i < buf_len; ++i)
    buf[i] = (uint8_t)(i & 0xFF);

  uint32_t crc1 = fio_crc32(buf, buf_len, 0);
  uint32_t crc2 = fio_crc32(buf, buf_len, 0);
  TEST_ASSERT(crc1 == crc2,
              "large buffer: two passes differ: 0x%08X vs 0x%08X",
              crc1,
              crc2);
  TEST_ASSERT(crc1 != 0,
              "large buffer: CRC32 of non-trivial data should not be 0");

  fprintf(stderr, "  64KB repeating pattern CRC32: 0x%08X\n", crc1);

  free(buf);
}

/* *****************************************************************************
Test: all lengths 0–64
Compare byte-at-a-time reference (single-byte loop) against bulk call.
***************************************************************************** */

static void test_crc32_all_short_lengths(void) {
  fprintf(stderr,
          "Testing CRC32 for all lengths 0–64 (bulk vs byte-loop)...\n");

  /* Fixed test data — 64 bytes */
  static const uint8_t data[64] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
      0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
      0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,
      0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
      0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  };

  for (size_t len = 0; len <= 64; ++len) {
    /* Reference: byte-at-a-time loop */
    uint32_t ref = 0;
    for (size_t i = 0; i < len; ++i)
      ref = fio_crc32(data + i, 1, ref);

    /* Bulk call */
    uint32_t bulk = fio_crc32(data, len, 0);

    TEST_ASSERT(ref == bulk,
                "length %zu: byte-loop=0x%08X, bulk=0x%08X",
                len,
                ref,
                bulk);
  }
}

/* *****************************************************************************
Section 2 — Performance Benchmark
Skipped entirely when DEBUG macro is defined.
***************************************************************************** */

#ifndef DEBUG

static int64_t crc32_time_us(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return ((int64_t)t.tv_sec * 1000000) + (int64_t)t.tv_nsec / 1000;
}

static void bench_crc32(void) {
  /* 64MB total data processed — use a 1MB buffer and iterate */
  const size_t buf_len = 1024 * 1024;              /* 1 MB */
  const size_t target_bytes = 64ULL * 1024 * 1024; /* 64 MB */

  uint8_t *buf = (uint8_t *)malloc(buf_len);
  if (!buf) {
    fprintf(stderr, "  ERROR: malloc failed for benchmark buffer\n");
    return;
  }

  /* Fill with a repeating pattern */
  for (size_t i = 0; i < buf_len; ++i)
    buf[i] = (uint8_t)(i & 0xFF);

  /* Warmup */
  volatile uint32_t sink = 0;
  for (int w = 0; w < 4; ++w)
    sink ^= fio_crc32(buf, buf_len, 0);

  /* Benchmark fio_crc32 */
  size_t iterations = target_bytes / buf_len;
  if (iterations < 4)
    iterations = 4;

  int64_t t0 = crc32_time_us();
  uint32_t crc = 0;
  for (size_t i = 0; i < iterations; ++i) {
    crc = fio_crc32(buf, buf_len, 0);
    FIO_COMPILER_GUARD;
  }
  int64_t t1 = crc32_time_us();
  (void)crc;

  double elapsed_sec = (double)(t1 - t0) / 1e6;
  double total_mb = (double)(iterations * buf_len) / (1024.0 * 1024.0);
  double fio_mbps = (elapsed_sec > 0.0) ? total_mb / elapsed_sec : 0.0;

  fprintf(stderr,
          "\nPerformance (CRC32, %.0f MB):\n"
          "  fio_crc32:  %.0f MB/s\n",
          total_mb,
          fio_mbps);

#ifdef HAVE_ZLIB
  /* Benchmark zlib crc32() on the same buffer */
  int64_t z0 = crc32_time_us();
  uLong zcrc = crc32(0L, Z_NULL, 0);
  for (size_t i = 0; i < iterations; ++i) {
    zcrc = crc32(0L, (const Bytef *)buf, (uInt)buf_len);
    FIO_COMPILER_GUARD;
  }
  int64_t z1 = crc32_time_us();
  (void)zcrc;

  double z_elapsed = (double)(z1 - z0) / 1e6;
  double zlib_mbps = (z_elapsed > 0.0) ? total_mb / z_elapsed : 0.0;
  double ratio = (zlib_mbps > 0.0) ? fio_mbps / zlib_mbps : 0.0;

  fprintf(stderr,
          "  zlib crc32: %.0f MB/s  (ratio: %.2fx)\n",
          zlib_mbps,
          ratio);
#endif /* HAVE_ZLIB */

  fprintf(stderr, "\n");
  free(buf);
}

#endif /* !DEBUG */

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr, "=== CRC32 Test Suite ===\n\n");

  /* Section 1: Compliance tests (always run) */
  test_crc32_known_vectors();
  test_crc32_incremental();
  test_crc32_large_buffer();
  test_crc32_all_short_lengths();

  fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);

#ifndef DEBUG
  /* Section 2: Performance benchmark (release only) */
  bench_crc32();
#endif /* !DEBUG */

  return g_fail ? 1 : 0;
}
