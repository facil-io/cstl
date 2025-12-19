/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_MEMALT
#include FIO_INCLUDE_FILE

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

int main(void) {
  uint64_t start, end;

  { /* test fio_memcpy small copy fast paths (1-64 bytes) */
    FIO_LOG_DDEBUG("testing fio_memcpy small copy fast paths (1-64 bytes)");
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
    FIO_LOG_DDEBUG("testing fio_memcpy with overlapping memory (memmove)");
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
