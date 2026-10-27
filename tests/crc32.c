/* *****************************************************************************
CRC32 Compliance Tests
Tests the standard CRC32 polynomial (0xEDB88320 / gzip / ITU-T V.42).
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRC32
#include FIO_INCLUDE_FILE

#include <string.h>
#if !defined(_WIN32)
#include <sys/mman.h>
#include <unistd.h>
#endif

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
Independent differential and exact-boundary regression tests.
Exercise SIMD entry, parallel folds, 16-byte drains and tails with seeded and
incremental CRCs. The bitwise oracle shares no production tables or intrinsics.
***************************************************************************** */

static uint32_t test_crc32_reference(const uint8_t *p,
                                     size_t len,
                                     uint32_t initial_crc) {
  uint32_t crc = ~initial_crc;
  while (len--) {
    crc ^= *p++;
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
  }
  return ~crc;
}

static const size_t test_crc32_long_lengths[] = {
    260,  261,  263,  264,  265,  319,  320,  321,  511,  512,  513,  1023,
    1024, 1025, 2047, 2048, 2049, 4095, 4096, 4097, 8191, 8192, 8193,
};

static void test_crc32_case(uint8_t *p, size_t len, size_t offset) {
  static const uint32_t seeds[] = {0, 0xA5F03C69U, 0xFFFFFFFFU};
  static const size_t splits[] = {
      0,
      1,
      4,
      8,
      15,
      16,
      17,
      31,
      32,
      63,
      64,
      65,
      79,
      80,
      127,
      128,
      129,
  };
  uint32_t rng = 0x12345678U;
  for (size_t i = 0; i < len; ++i) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    p[i] = (uint8_t)rng;
  }
  for (size_t s = 0; s < sizeof(seeds) / sizeof(*seeds); ++s) {
    uint32_t expected = test_crc32_reference(p, len, seeds[s]);
    TEST_ASSERT(fio_crc32(p, len, seeds[s]) == expected,
                "oracle: offset=%zu len=%zu seed=0x%08X",
                offset,
                len,
                seeds[s]);
    for (size_t i = 0; i < sizeof(splits) / sizeof(*splits) + 2; ++i) {
      size_t split =
          (i < sizeof(splits) / sizeof(*splits))
              ? splits[i]
              : ((i == sizeof(splits) / sizeof(*splits)) ? len / 2 : len);
      if (split > len)
        continue;
      uint32_t crc = fio_crc32(p, split, seeds[s]);
      crc = fio_crc32(p + split, len - split, crc);
      TEST_ASSERT(
          crc == expected,
          "incremental oracle: offset=%zu len=%zu split=%zu seed=0x%08X",
          offset,
          len,
          split,
          seeds[s]);
    }
  }
}

static void test_crc32_differential(void) {
  fprintf(stderr,
          "Testing CRC32 independent oracle, offsets and exact extents...\n");
  for (size_t offset = 0; offset < 16; ++offset) {
    for (size_t i = 0; i < 260 + sizeof(test_crc32_long_lengths) /
                                     sizeof(*test_crc32_long_lengths);
         ++i) {
      size_t len = i < 260 ? i : test_crc32_long_lengths[i - 260];
      /* No readable suffix: ASan detects even a one-byte overread. */
      size_t extent = offset + len;
      uint8_t *buf = (uint8_t *)malloc(extent ? extent : 1);
      TEST_ASSERT(buf != NULL, "differential: malloc failed");
      if (!buf)
        return;
      test_crc32_case(buf + offset, len, offset);
      free(buf);
    }
  }
}

static void test_crc32_guard_page(void) {
#if !defined(_WIN32) && defined(MAP_ANONYMOUS)
  fprintf(stderr, "Testing CRC32 at an unreadable page boundary...\n");
  long page_query = sysconf(_SC_PAGESIZE);
  TEST_ASSERT(page_query > 0, "guard: sysconf failed");
  if (page_query <= 0)
    return;
  size_t page = (size_t)page_query;
  size_t data_size = ((8193 + page - 1) / page) * page;
  size_t mapping_size = data_size + page;
  uint8_t *mapping = (uint8_t *)mmap(NULL,
                                     mapping_size,
                                     PROT_READ | PROT_WRITE,
                                     MAP_PRIVATE | MAP_ANONYMOUS,
                                     -1,
                                     0);
  TEST_ASSERT(mapping != MAP_FAILED, "guard: mmap failed");
  if (mapping == MAP_FAILED)
    return;
  uint8_t *guard = mapping + data_size;
  int rc = mprotect(guard, page, PROT_NONE);
  TEST_ASSERT(!rc, "guard: mprotect failed");
  if (!rc) {
    for (size_t len = 0; len < 260; ++len)
      test_crc32_case(guard - len, len, len % 16);
    for (size_t i = 0;
         i < sizeof(test_crc32_long_lengths) / sizeof(*test_crc32_long_lengths);
         ++i) {
      size_t len = test_crc32_long_lengths[i];
      test_crc32_case(guard - len, len, len % 16);
    }
  }
  TEST_ASSERT(!munmap(mapping, mapping_size), "guard: munmap failed");
#endif
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr, "=== CRC32 Test Suite ===\n\n");
#if defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
#if defined(FIO___HAS_ARM_INTRIN) || defined(__ARM_FEATURE_PMULL) ||           \
    defined(__ARM_FEATURE_CRYPTO)
  fprintf(stderr, "CRC32 path: ARM PMULL + scalar\n");
#else
  fprintf(stderr, "CRC32 path: ARM scalar\n");
#endif
#elif defined(FIO___HAS_X86_INTRIN) && defined(__SSE4_2__) &&                  \
    defined(__PCLMUL__)
  fprintf(stderr,
          "CRC32 path: x86 PCLMUL%s\n",
#if defined(FIO_FAKE_X86_SHADOW)
          " (fx86 shadow)"
#else
          ""
#endif
  );
#else
  fprintf(stderr, "CRC32 path: software\n");
#endif

  /* Section 1: Compliance tests (always run) */
  test_crc32_known_vectors();
  test_crc32_incremental();
  test_crc32_large_buffer();
  test_crc32_all_short_lengths();
  test_crc32_differential();
  test_crc32_guard_page();

  fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);

  return g_fail ? 1 : 0;
}
