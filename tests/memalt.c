/* *****************************************************************************
Test - Memalt Module
Covers 001 memalt.h fallback memory helpers and 010 mem.h edge assertions
split out of ./tests/core.c during Stage 4A ownership normalization.
***************************************************************************** */
#define FIO_MEMALT
#include "test-helpers.h"

/* *****************************************************************************
Functional Tests
***************************************************************************** */

FIO_SFUNC void fio___test_memcpy_overlap(void) {
  /* Forward overlap: dest < src < dest+len (should use buffered copy) */
  {
    char buf[256];
    fio_rand_bytes(buf, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf, 256);

    /* Copy from buf+10 to buf+0 (100 bytes) - forward overlap */
    fio_memcpy(buf, buf + 10, 100);
    FIO_ASSERT(!memcmp(buf, expected + 10, 100),
               "fio_memcpy forward overlap failed");
  }

  /* Backward overlap: src < dest < src+len (should use reversed copy) */
  {
    char buf[256];
    fio_rand_bytes(buf, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf, 256);

    /* Copy from buf+0 to buf+10 (100 bytes) - backward overlap */
    fio_memcpy(buf + 10, buf, 100);
    FIO_ASSERT(!memcmp(buf + 10, expected, 100),
               "fio_memcpy backward overlap failed");
  }

  /* No overlap: dest+len <= src */
  {
    char buf[512];
    fio_rand_bytes(buf + 256, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf + 256, 256);

    fio_memcpy(buf, buf + 256, 256);
    FIO_ASSERT(!memcmp(buf, expected, 256),
               "fio_memcpy non-overlapping (dest before src) failed");
  }

  /* No overlap: src+len <= dest */
  {
    char buf[512];
    fio_rand_bytes(buf, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf, 256);

    fio_memcpy(buf + 256, buf, 256);
    FIO_ASSERT(!memcmp(buf + 256, expected, 256),
               "fio_memcpy non-overlapping (src before dest) failed");
  }

  /* Adjacent memory: dest = src+len (no overlap) */
  {
    char buf[256];
    fio_rand_bytes(buf, 128);
    char expected[128];
    FIO_MEMCPY(expected, buf, 128);

    fio_memcpy(buf + 128, buf, 128);
    FIO_ASSERT(!memcmp(buf + 128, expected, 128),
               "fio_memcpy adjacent memory failed");
  }

  /* Same pointer: dest == src (should be no-op) */
  {
    char buf[64];
    fio_rand_bytes(buf, 64);
    char expected[64];
    FIO_MEMCPY(expected, buf, 64);

    fio_memcpy(buf, buf, 64);
    FIO_ASSERT(!memcmp(buf, expected, 64), "fio_memcpy same pointer failed");
  }

  /* Overlap at various offsets */
  for (size_t offset = 1; offset <= 64; offset *= 2) {
    char buf[256];
    fio_rand_bytes(buf, 256);
    char expected[256];
    FIO_MEMCPY(expected, buf, 256);

    /* Forward overlap */
    fio_memcpy(buf, buf + offset, 128);
    FIO_ASSERT(!memcmp(buf, expected + offset, 128),
               "fio_memcpy forward overlap at offset %zu failed",
               offset);

    /* Reset and test backward overlap */
    FIO_MEMCPY(buf, expected, 256);
    fio_memcpy(buf + offset, buf, 128);
    FIO_ASSERT(!memcmp(buf + offset, expected, 128),
               "fio_memcpy backward overlap at offset %zu failed",
               offset);
  }
}

FIO_SFUNC void fio___test_memcpy_null_handling(void) {
  char buf[64];
  fio_rand_bytes(buf, 64);
  char expected[64];
  FIO_MEMCPY(expected, buf, 64);

  /* Both NULL with len = 0 - should be safe no-op */
  void *result = fio_memcpy(NULL, NULL, 0);
  FIO_ASSERT(result == NULL, "fio_memcpy(NULL, NULL, 0) should return NULL");

  /* len = 0 with valid pointers - should be no-op */
  result = fio_memcpy(buf, buf + 32, 0);
  FIO_ASSERT(!memcmp(buf, expected, 64),
             "fio_memcpy with len=0 should not modify buffer");
}

FIO_SFUNC void fio___test_memcpy_size_boundaries(void) {
  /* Test sizes: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 31, 32, 33,
   * 63, 64, 65, 127, 128, 129, 255, 256, 257 */
  size_t test_sizes[] = {0,  1,   2,   3,   4,   5,   6,   7,   8,
                         9,  15,  16,  17,  31,  32,  33,  63,  64,
                         65, 127, 128, 129, 255, 256, 257, 511, 512};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  char src[1024];
  char dest[1024];
  char guard = (char)0xAA;

  fio_rand_bytes(src, sizeof(src));

  for (size_t i = 0; i < num_sizes; ++i) {
    size_t len = test_sizes[i];
    if (len > sizeof(src))
      continue;

    /* Clear dest and set guard byte */
    FIO_MEMSET(dest, 0, sizeof(dest));
    if (len < sizeof(dest))
      dest[len] = guard;

    fio_memcpy(dest, src, len);

    FIO_ASSERT(!memcmp(dest, src, len), "fio_memcpy failed for size %zu", len);

    /* Check guard byte wasn't modified (overflow detection) */
    if (len < sizeof(dest)) {
      FIO_ASSERT(dest[len] == guard,
                 "fio_memcpy overflow detected at size %zu",
                 len);
    }
  }
}

FIO_SFUNC void fio___test_memcpy_alignment(void) {
  char buf[256 + 16] FIO_ALIGN(16);
  char src[256 + 16] FIO_ALIGN(16);
  fio_rand_bytes(src, sizeof(src));

  /* Test all combinations of dest and src alignment offsets 0-7 */
  for (size_t dest_off = 0; dest_off < 8; ++dest_off) {
    for (size_t src_off = 0; src_off < 8; ++src_off) {
      FIO_MEMSET(buf, 0, sizeof(buf));

      fio_memcpy(buf + dest_off, src + src_off, 64);

      FIO_ASSERT(!memcmp(buf + dest_off, src + src_off, 64),
                 "fio_memcpy alignment test failed: dest_off=%zu, src_off=%zu",
                 dest_off,
                 src_off);
    }
  }
}

FIO_SFUNC void fio___test_memset_size_boundaries(void) {
  size_t test_sizes[] = {0, 1, 7, 8, 15, 16, 31, 32, 33, 63, 64, 127, 128};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  char buf[256];
  char guard = (char)0xCC;

  for (size_t i = 0; i < num_sizes; ++i) {
    size_t len = test_sizes[i];

    /* Clear buffer and set guard */
    FIO_MEMSET(buf, 0xFF, sizeof(buf));
    if (len < sizeof(buf))
      buf[len] = guard;

    fio_memset(buf, 0x42, len);

    /* Verify all bytes are set correctly */
    for (size_t j = 0; j < len; ++j) {
      FIO_ASSERT((uint8_t)buf[j] == 0x42,
                 "fio_memset failed at byte %zu for size %zu",
                 j,
                 len);
    }

    /* Check guard byte */
    if (len < sizeof(buf)) {
      FIO_ASSERT(buf[len] == guard,
                 "fio_memset overflow detected at size %zu",
                 len);
    }
  }
}

FIO_SFUNC void fio___test_memset_data_patterns(void) {
  char buf[128];

  /* data = 0 (all zeros) */
  {
    FIO_MEMSET(buf, 0xFF, sizeof(buf));
    fio_memset(buf, 0, 64);
    for (size_t i = 0; i < 64; ++i) {
      FIO_ASSERT(buf[i] == 0, "fio_memset with data=0 failed at byte %zu", i);
    }
  }

  /* data = 0xFF (single byte, should expand to 0xFFFFFFFFFFFFFFFF) */
  {
    FIO_MEMSET(buf, 0, sizeof(buf));
    fio_memset(buf, 0xFF, 64);
    for (size_t i = 0; i < 64; ++i) {
      FIO_ASSERT((uint8_t)buf[i] == 0xFF,
                 "fio_memset with data=0xFF failed at byte %zu",
                 i);
    }
  }

  /* data = 1 (single byte, should expand to 0x0101010101010101) */
  {
    FIO_MEMSET(buf, 0, sizeof(buf));
    fio_memset(buf, 1, 64);
    for (size_t i = 0; i < 64; ++i) {
      FIO_ASSERT((uint8_t)buf[i] == 0x01,
                 "fio_memset with data=1 failed at byte %zu",
                 i);
    }
  }

  /* data = 0x0100 (two-byte pattern, NOT expanded - only low byte used) */
  {
    FIO_MEMSET(buf, 0xFF, sizeof(buf));
    fio_memset(buf, 0x0100, 64);
    /* When data >= 0x100, fio_memset uses the full 64-bit pattern */
    /* Check that pattern is applied correctly */
    uint64_t expected = 0x0100;
    for (size_t i = 0; i + 8 <= 64; i += 8) {
      uint64_t val;
      fio_memcpy8(&val, buf + i);
      FIO_ASSERT(val == expected,
                 "fio_memset with data=0x0100 failed at offset %zu",
                 i);
    }
  }

  /* data = 0xDEADBEEFCAFEBABE (8-byte pattern) */
  {
    FIO_MEMSET(buf, 0, sizeof(buf));
    fio_memset(buf, 0xDEADBEEFCAFEBABEULL, 64);
    uint64_t expected = 0xDEADBEEFCAFEBABEULL;
    for (size_t i = 0; i + 8 <= 64; i += 8) {
      uint64_t val;
      fio_memcpy8(&val, buf + i);
      FIO_ASSERT(val == expected,
                 "fio_memset with 8-byte pattern failed at offset %zu",
                 i);
    }
  }
}

FIO_SFUNC void fio___test_memset_alignment(void) {
  char buf[128 + 16] FIO_ALIGN(16);

  /* Test dest unaligned by 1-7 bytes */
  for (size_t offset = 0; offset < 8; ++offset) {
    FIO_MEMSET(buf, 0, sizeof(buf));
    fio_memset(buf + offset, 0xAB, 64);

    for (size_t i = 0; i < 64; ++i) {
      FIO_ASSERT((uint8_t)buf[offset + i] == 0xAB,
                 "fio_memset alignment test failed: offset=%zu, byte=%zu",
                 offset,
                 i);
    }

    /* Verify bytes before offset weren't touched */
    for (size_t i = 0; i < offset; ++i) {
      FIO_ASSERT(buf[i] == 0,
                 "fio_memset wrote before dest at offset=%zu",
                 offset);
    }
  }
}

FIO_SFUNC void fio___test_memchr_token_position(void) {
  char buf[256];
  FIO_MEMSET(buf, 'X', sizeof(buf));

  /* Token at position 0 (first byte) */
  {
    buf[0] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf, "fio_memchr failed to find token at position 0");
    buf[0] = 'X';
  }

  /* Token at position 1 */
  {
    buf[1] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + 1,
               "fio_memchr failed to find token at position 1");
    buf[1] = 'X';
  }

  /* Token at position len-1 (last byte) */
  {
    buf[sizeof(buf) - 1] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + sizeof(buf) - 1,
               "fio_memchr failed to find token at last position");
    buf[sizeof(buf) - 1] = 'X';
  }

  /* Token not present (return NULL) */
  {
    void *result = fio_memchr(buf, 'Z', sizeof(buf));
    FIO_ASSERT(result == NULL,
               "fio_memchr should return NULL when token not found");
  }

  /* Multiple occurrences (find first) */
  {
    buf[10] = 'A';
    buf[50] = 'A';
    buf[100] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + 10, "fio_memchr should find first occurrence");
    buf[10] = buf[50] = buf[100] = 'X';
  }
}

FIO_SFUNC void fio___test_memchr_token_values(void) {
  char buf[128];
  FIO_MEMSET(buf, 0x55, sizeof(buf));

  /* token = 0 (NUL byte) - critical for fio_has_zero_byte64 */
  {
    buf[32] = '\0';
    void *result = fio_memchr(buf, '\0', sizeof(buf));
    FIO_ASSERT(result == buf + 32, "fio_memchr failed to find NUL byte");
    buf[32] = 0x55;
  }

  /* token = 0xFF */
  {
    buf[48] = (char)0xFF;
    void *result = fio_memchr(buf, (char)0xFF, sizeof(buf));
    FIO_ASSERT(result == buf + 48, "fio_memchr failed to find 0xFF byte");
    buf[48] = 0x55;
  }

  /* token = 0x80 (high bit set) */
  {
    buf[64] = (char)0x80;
    void *result = fio_memchr(buf, (char)0x80, sizeof(buf));
    FIO_ASSERT(result == buf + 64, "fio_memchr failed to find 0x80 byte");
    buf[64] = 0x55;
  }

  /* token = 'A' (normal ASCII) */
  {
    buf[16] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + 16, "fio_memchr failed to find 'A'");
    buf[16] = 0x55;
  }
}

FIO_SFUNC void fio___test_memchr_size_boundaries(void) {
  char buf[256];
  FIO_MEMSET(buf, 'X', sizeof(buf));

  /* len = 0 (return NULL) */
  {
    buf[0] = 'A';
    void *result = fio_memchr(buf, 'A', 0);
    FIO_ASSERT(result == NULL, "fio_memchr with len=0 should return NULL");
    buf[0] = 'X';
  }

  /* Test various sizes with token at end */
  size_t test_sizes[] = {1, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t i = 0; i < num_sizes; ++i) {
    size_t len = test_sizes[i];
    buf[len - 1] = 'A';
    void *result = fio_memchr(buf, 'A', len);
    FIO_ASSERT(result == buf + len - 1, "fio_memchr failed for size %zu", len);
    buf[len - 1] = 'X';
  }
}

FIO_SFUNC void fio___test_memchr_simd_boundaries(void) {
  char buf[256] FIO_ALIGN(32);
  FIO_MEMSET(buf, 'X', sizeof(buf));

  /* Token at SIMD vector boundaries: 15, 16, 31, 32, 63, 64 */
  size_t boundary_positions[] = {15, 16, 31, 32, 63, 64, 127, 128};
  size_t num_positions =
      sizeof(boundary_positions) / sizeof(boundary_positions[0]);

  for (size_t i = 0; i < num_positions; ++i) {
    size_t pos = boundary_positions[i];
    if (pos >= sizeof(buf))
      continue;

    buf[pos] = 'A';
    void *result = fio_memchr(buf, 'A', sizeof(buf));
    FIO_ASSERT(result == buf + pos,
               "fio_memchr failed at SIMD boundary position %zu",
               pos);
    buf[pos] = 'X';
  }

  /* Test with buffer unaligned by 1-15 bytes */
  for (size_t offset = 1; offset < 16; ++offset) {
    char *unaligned = buf + offset;
    size_t len = sizeof(buf) - offset - 1;
    FIO_MEMSET(unaligned, 'Y', len);
    unaligned[32] = 'B';

    void *result = fio_memchr(unaligned, 'B', len);
    FIO_ASSERT(result == unaligned + 32,
               "fio_memchr failed with unaligned buffer offset=%zu",
               offset);
  }
}

FIO_SFUNC void fio___test_memcmp_equal_buffers(void) {
  char a[128], b[128];
  fio_rand_bytes(a, sizeof(a));
  FIO_MEMCPY(b, a, sizeof(b));

  /* Test various sizes */
  size_t test_sizes[] = {0, 1, 7, 8, 63, 64, 65, 127, 128};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t i = 0; i < num_sizes; ++i) {
    size_t len = test_sizes[i];
    int result = fio_memcmp(a, b, len);
    FIO_ASSERT(result == 0,
               "fio_memcmp should return 0 for equal buffers, size=%zu",
               len);
  }
}

FIO_SFUNC void fio___test_memcmp_difference_position(void) {
  char a[128], b[128];

  /* First byte differs (a > b) */
  {
    FIO_MEMSET(a, 0x50, sizeof(a));
    FIO_MEMSET(b, 0x40, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result > 0,
               "fio_memcmp: first byte a > b should return positive");
  }

  /* First byte differs (a < b) */
  {
    FIO_MEMSET(a, 0x40, sizeof(a));
    FIO_MEMSET(b, 0x50, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0,
               "fio_memcmp: first byte a < b should return negative");
  }

  /* Last byte differs (a > b) */
  {
    FIO_MEMSET(a, 0x50, sizeof(a));
    FIO_MEMSET(b, 0x50, sizeof(b));
    a[sizeof(a) - 1] = 0x60;
    b[sizeof(b) - 1] = 0x40;
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result > 0,
               "fio_memcmp: last byte a > b should return positive");
  }

  /* Last byte differs (a < b) */
  {
    FIO_MEMSET(a, 0x50, sizeof(a));
    FIO_MEMSET(b, 0x50, sizeof(b));
    a[sizeof(a) - 1] = 0x40;
    b[sizeof(b) - 1] = 0x60;
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0,
               "fio_memcmp: last byte a < b should return negative");
  }

  /* Difference at boundary positions: 7, 8, 63, 64 */
  size_t boundary_positions[] = {7, 8, 63, 64};
  size_t num_positions =
      sizeof(boundary_positions) / sizeof(boundary_positions[0]);

  for (size_t i = 0; i < num_positions; ++i) {
    size_t pos = boundary_positions[i];
    if (pos >= sizeof(a))
      continue;

    FIO_MEMSET(a, 0x50, sizeof(a));
    FIO_MEMSET(b, 0x50, sizeof(b));
    a[pos] = 0x60;
    b[pos] = 0x40;

    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result > 0,
               "fio_memcmp: difference at position %zu (a > b) failed",
               pos);

    a[pos] = 0x40;
    b[pos] = 0x60;
    result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0,
               "fio_memcmp: difference at position %zu (a < b) failed",
               pos);
  }
}

FIO_SFUNC void fio___test_memcmp_same_pointer(void) {
  char buf[64];
  fio_rand_bytes(buf, sizeof(buf));

  int result = fio_memcmp(buf, buf, sizeof(buf));
  FIO_ASSERT(result == 0, "fio_memcmp(a, a, len) should return 0");
}

FIO_SFUNC void fio___test_memcmp_byte_value_edge_cases(void) {
  char a[8], b[8];

  /* 0x7F vs 0x80 (sign bit boundary - must use unsigned comparison) */
  {
    FIO_MEMSET(a, 0x7F, sizeof(a));
    FIO_MEMSET(b, 0x80, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0,
               "fio_memcmp: 0x7F < 0x80 (unsigned comparison required)");

    result = fio_memcmp(b, a, sizeof(a));
    FIO_ASSERT(result > 0,
               "fio_memcmp: 0x80 > 0x7F (unsigned comparison required)");
  }

  /* 0x00 vs 0x01 */
  {
    FIO_MEMSET(a, 0x00, sizeof(a));
    FIO_MEMSET(b, 0x01, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0, "fio_memcmp: 0x00 < 0x01 failed");
  }

  /* 0xFE vs 0xFF */
  {
    FIO_MEMSET(a, 0xFE, sizeof(a));
    FIO_MEMSET(b, 0xFF, sizeof(b));
    int result = fio_memcmp(a, b, sizeof(a));
    FIO_ASSERT(result < 0, "fio_memcmp: 0xFE < 0xFF failed");
  }
}

FIO_SFUNC void fio___test_strlen_lengths(void) {
  /* Empty string "" (return 0) */
  FIO_ASSERT(fio_strlen("") == 0, "fio_strlen(\"\") should return 0");

  /* Single char "a" (return 1) */
  FIO_ASSERT(fio_strlen("a") == 1, "fio_strlen(\"a\") should return 1");

  /* Test with string literals of known lengths */
  FIO_ASSERT(fio_strlen("1234567") == 7, "fio_strlen 7-char string failed");
  FIO_ASSERT(fio_strlen("12345678") == 8, "fio_strlen 8-char string failed");
  FIO_ASSERT(fio_strlen("123456789012345") == 15,
             "fio_strlen 15-char string failed");
  FIO_ASSERT(fio_strlen("1234567890123456") == 16,
             "fio_strlen 16-char string failed");

  /* Test longer strings with dynamically created buffers */
  char buf[512] FIO_ALIGN(64);
  size_t test_lengths[] = {31, 32, 63, 64, 127, 128, 255, 256};
  size_t num_lengths = sizeof(test_lengths) / sizeof(test_lengths[0]);

  for (size_t i = 0; i < num_lengths; ++i) {
    size_t len = test_lengths[i];
    if (len >= sizeof(buf) - 1)
      continue;

    /* Reset buffer and set NUL at desired position */
    FIO_MEMSET(buf, 'A', sizeof(buf));
    buf[len] = '\0';

    size_t result = fio_strlen(buf);
    FIO_ASSERT(result == len,
               "fio_strlen failed for length %zu (got %zu)",
               len,
               result);
  }
}

FIO_SFUNC void fio___test_strlen_null_pointer(void) {
  size_t result = fio_strlen(NULL);
  FIO_ASSERT(result == 0, "fio_strlen(NULL) should return 0");
}

FIO_SFUNC void fio___test_strlen_alignment(void) {
  char buf[128 + 16] FIO_ALIGN(16);
  FIO_MEMSET(buf, 'B', sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  /* Test string starting at various alignment offsets */
  for (size_t offset = 0; offset < 8; ++offset) {
    char *str = buf + offset;
    size_t expected_len = sizeof(buf) - 1 - offset;

    size_t result = fio_strlen(str);
    FIO_ASSERT(result == expected_len,
               "fio_strlen alignment test failed: offset=%zu, expected=%zu, "
               "got=%zu",
               offset,
               expected_len,
               result);
  }
}

FIO_SFUNC void fio___test_strlen_simd_boundaries(void) {
  char buf[256] FIO_ALIGN(32);

  /* NUL at SIMD vector boundaries: 15, 16, 31, 32, 63, 64 */
  size_t boundary_positions[] = {15, 16, 31, 32, 63, 64, 127, 128};
  size_t num_positions =
      sizeof(boundary_positions) / sizeof(boundary_positions[0]);

  for (size_t i = 0; i < num_positions; ++i) {
    size_t pos = boundary_positions[i];
    if (pos >= sizeof(buf))
      continue;

    FIO_MEMSET(buf, 'C', sizeof(buf));
    buf[pos] = '\0';

    size_t result = fio_strlen(buf);
    FIO_ASSERT(result == pos,
               "fio_strlen failed at SIMD boundary position %zu (got %zu)",
               pos,
               result);
  }
}

FIO_IFUNC void fio___memset_test_aligned(void *restrict dest_,
                                         uint64_t data,
                                         size_t bytes,
                                         const char *msg) {
  uint8_t *r = (uint8_t *)dest_;
  uint8_t *e_group = r + (bytes & (~(size_t)63ULL));
  uint64_t d[8] = {data, data, data, data, data, data, data, data};
  while (r < e_group) {
    fio_memcpy64(d, r);
    FIO_ASSERT(d[0] == data && d[1] == data && d[2] == data && d[3] == data &&
                   d[4] == data && d[5] == data && d[6] == data && d[7] == data,
               "%s memory data was overwritten",
               msg);
    r += 64;
  }
  fio_memcpy63x(d, r, bytes);
  FIO_ASSERT(d[0] == data && d[1] == data && d[2] == data && d[3] == data &&
                 d[4] == data && d[5] == data && d[6] == data && d[7] == data,
             "%s memory data was overwritten",
             msg);
  (void)msg; /* in case FIO_ASSERT is disabled */
}

FIO_SFUNC void fio___test_memalt_archived(void) {
  uint64_t start, end;

  { /* test fio_memcpy small copy fast paths (1-64 bytes) */
    uint8_t src[128];
    uint8_t dst[128];
    /* Initialize source with known pattern */
    for (size_t i = 0; i < sizeof(src); ++i)
      src[i] = (uint8_t)(i + 1);
    /* Test all sizes from 1 to 64 bytes */
    for (size_t len = 1; len <= 64; ++len) {
      /* Clear destination */
      memset(dst, 0xFF, sizeof(dst));
      /* Copy using fio_memcpy */
      fio_memcpy(dst, src, len);
      /* Verify copied data matches */
      FIO_ASSERT(!memcmp(dst, src, len),
                 "fio_memcpy small copy failed for %zu bytes",
                 len);
      /* Verify no overflow (byte after copied region should be untouched) */
      FIO_ASSERT(dst[len] == 0xFF,
                 "fio_memcpy small copy overflow for %zu bytes",
                 len);
    }
  }
  { /* test fio_memcpy possible overflow. */
    uint64_t buf1[64];
    uint8_t *buf = (uint8_t *)buf1;
    fio_memset(buf1, ~(uint64_t)0, sizeof(*buf1) * 64);
    char *data =
        (char *)"This should be an uneven amount of characters, say 53";
    fio_memcpy(buf, data, FIO_STRLEN(data));
    FIO_ASSERT(!memcmp(buf, data, FIO_STRLEN(data)) &&
                   buf[FIO_STRLEN(data)] == 0xFF,
               "fio_memcpy should not overflow or underflow on uneven "
               "amounts of bytes.");
  }
  { /* test fio_memcpy as memmove */
    char *msg = (char *)"fio_memcpy should work also as memmove, "
                        "so undefined behavior should not occur. "
                        "Should be true for larger offsets too. At least over "
                        "128 Bytes.";
    size_t len = FIO_STRLEN(msg);
    char buf[512];
    for (size_t offset = 1; offset < len; ++offset) {
      memset(buf, 0, sizeof(buf));
      memmove(buf, msg, len);
      fio_memcpy(buf + offset, buf, len);
      FIO_ASSERT(!memcmp(buf + offset, msg, len),
                 "fio_memcpy failed on overlapping data (offset +%d, len %zu)",
                 offset,
                 len);
      memset(buf, 0, sizeof(buf));
      memmove(buf + offset, msg, len);
      fio_memcpy(buf, buf + offset, len);
      FIO_ASSERT(!memcmp(buf, msg, len),
                 "fio_memcpy failed on overlapping data (offset -%d, len %zu)",
                 offset,
                 len);
    }
  }
  { /* test fio_memcmp */
    for (size_t i = 0; i < 4096; ++i) {
      uint64_t a = fio_rand64(), b = fio_rand64();
      int s = memcmp(&a, &b, sizeof(a));
      int f = fio_memcmp(&a, &b, sizeof(a));
      FIO_ASSERT((s < 0 && f < 0) || (s > 0 && f > 0) || (!s && !f),
                 "fio_memcmp != memcmp (result meaning, not value).");
      FIO_ASSERT(fio_ct_is_eq(&a, &b, sizeof(a)) == (!s),
                 "fio_ct_is_eq differs from memcmp result");
    }
  }
  { /* test fio_memchr and fio_strlen */
    char membuf[4096];
    memset(membuf, 0xff, 4096);
    membuf[4095] = 0;
    for (size_t i = 0; i < 4095; ++i) {
      membuf[i] = 0;
      char *result = (char *)fio_memchr(membuf, 0, 4096);
      size_t len = fio_strlen(membuf);
      membuf[i] = (char)((i & 0xFFU) | 1U);
      FIO_ASSERT(result == membuf + i, "fio_memchr failed.");
      FIO_ASSERT(len == i, "fio_strlen failed (%zu != %zu).", len, i);
    }
  }
  /* Performance tests moved to tests/performance-memalt.c */
  (void)start;
  (void)end;
}

/* *****************************************************************************
Main Test Entry Point
***************************************************************************** */

int main(void) {
  fio___test_memalt_archived();
  fio___test_memcpy_overlap();
  fio___test_memcpy_null_handling();
  fio___test_memcpy_size_boundaries();
  fio___test_memcpy_alignment();
  fio___test_memset_size_boundaries();
  fio___test_memset_data_patterns();
  fio___test_memset_alignment();
  fio___test_memchr_token_position();
  fio___test_memchr_token_values();
  fio___test_memchr_size_boundaries();
  fio___test_memchr_simd_boundaries();
  fio___test_memcmp_equal_buffers();
  fio___test_memcmp_difference_position();
  fio___test_memcmp_same_pointer();
  fio___test_memcmp_byte_value_edge_cases();
  fio___test_strlen_lengths();
  fio___test_strlen_null_pointer();
  fio___test_strlen_alignment();
  fio___test_strlen_simd_boundaries();
  return 0;
}
