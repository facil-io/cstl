/* *****************************************************************************
Test - Random Module
Covers deterministic API edge cases from ./tests-old/random.c without the old
statistical speed harness.
***************************************************************************** */
#define FIO_RAND
#include "test-helpers.h"

static void fio___test_random_edge_cases(void) {
  /* Test: fio_rand64 produces non-zero values */
  {
    uint64_t r = fio_rand64();
    /* It's statistically unlikely but possible to get 0, so just log it */
    if (r == 0) {
      FIO_LOG_WARNING("fio_rand64 returned 0 (unlikely but possible)");
    }
  }

  /* Test: fio_rand64 produces different values on consecutive calls */
  {
    uint64_t r1 = fio_rand64();
    uint64_t r2 = fio_rand64();
    uint64_t r3 = fio_rand64();
    /* All three being equal is astronomically unlikely */
    FIO_ASSERT(!(r1 == r2 && r2 == r3),
               "fio_rand64 should produce different values");
  }

  /* Test: fio_rand_bytes with 0 bytes (should be no-op) */
  {
    uint8_t buf[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    uint8_t expected[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    fio_rand_bytes(buf, 0);
    FIO_ASSERT(FIO_MEMCMP(buf, expected, 8) == 0,
               "fio_rand_bytes(buf, 0) should not modify buffer");
  }

  /* Test: fio_rand_bytes with 1 byte */
  {
    uint8_t buf[1] = {0};
    fio_rand_bytes(buf, 1);
    /* Can't really assert much here, just that it doesn't crash */
  }

  /* Test: fio_rand_bytes with small buffer (7 bytes - not aligned) */
  {
    uint8_t buf[7] = {0};
    uint8_t zero[7] = {0};
    fio_rand_bytes(buf, 7);
    /* Statistically very unlikely to be all zeros */
    FIO_ASSERT(FIO_MEMCMP(buf, zero, 7) != 0,
               "fio_rand_bytes(7) should produce non-zero output");
  }

  /* Test: fio_rand_bytes with exactly 8 bytes (one uint64_t) */
  {
    uint8_t buf[8] = {0};
    uint8_t zero[8] = {0};
    fio_rand_bytes(buf, 8);
    FIO_ASSERT(FIO_MEMCMP(buf, zero, 8) != 0,
               "fio_rand_bytes(8) should produce non-zero output");
  }

  /* Test: fio_rand_bytes with 9 bytes (crosses boundary) */
  {
    uint8_t buf[9] = {0};
    uint8_t zero[9] = {0};
    fio_rand_bytes(buf, 9);
    FIO_ASSERT(FIO_MEMCMP(buf, zero, 9) != 0,
               "fio_rand_bytes(9) should produce non-zero output");
  }

  /* Test: fio_rand_bytes with large buffer (1MB) */
  {
    size_t large_len = 1024 * 1024;
    uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, large_len, 0);
    FIO_ASSERT(buf != NULL, "Failed to allocate 1MB buffer");
    FIO_MEMSET(buf, 0, large_len);

    fio_rand_bytes(buf, large_len);

    /* Check that it's not all zeros (sample first 1KB) */
    uint8_t zero[1024] = {0};
    FIO_ASSERT(FIO_MEMCMP(buf, zero, 1024) != 0,
               "fio_rand_bytes(1MB) should produce non-zero output");

    /* Check that different parts of the buffer are different */
    FIO_ASSERT(FIO_MEMCMP(buf, buf + large_len / 2, 64) != 0,
               "Different parts of large random buffer should differ");

    FIO_MEM_FREE(buf, large_len);
  }

  /* Test: fio_rand_bytes produces different output on consecutive calls */
  {
    uint8_t buf1[32], buf2[32];
    fio_rand_bytes(buf1, 32);
    fio_rand_bytes(buf2, 32);
    FIO_ASSERT(
        FIO_MEMCMP(buf1, buf2, 32) != 0,
        "Consecutive fio_rand_bytes calls should produce different output");
  }

  /* Test: NULL buffer with 0 length (should not crash) */
  {
    fio_rand_bytes(NULL, 0);
    /* If we get here, it didn't crash */
  }
}

static void fio___test_random_secure(void) {
  /* Test: the system CSPRNG backend works on every platform */
  {
    static uint8_t buf[4096];
    static const size_t lens[] = {1, 32, 4096};
    for (size_t i = 0; i < sizeof(lens) / sizeof(lens[0]); ++i) {
      FIO_ASSERT(fio_rand_bytes_secure(buf, lens[i]) == 0,
                 "fio_rand_bytes_secure(%zu) should succeed",
                 lens[i]);
    }
    uint8_t zero[64] = {0};
    FIO_ASSERT(FIO_MEMCMP(buf, zero, 64) != 0,
               "fio_rand_bytes_secure(4096) should produce non-zero output");
  }

  /* Test: consecutive 32 byte secrets differ */
  {
    uint8_t buf1[32] = {0}, buf2[32] = {0};
    FIO_ASSERT(fio_rand_bytes_secure(buf1, 32) == 0 &&
                   fio_rand_bytes_secure(buf2, 32) == 0,
               "fio_rand_bytes_secure(32) should succeed");
    FIO_ASSERT(FIO_MEMCMP(buf1, buf2, 32) != 0,
               "Consecutive fio_rand_bytes_secure calls should differ");
  }

  /* Test: argument edge cases */
  {
    uint8_t buf[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    FIO_ASSERT(fio_rand_bytes_secure(buf, 0) == 0 && buf[0] == 0xAA &&
                   buf[3] == 0xDD,
               "fio_rand_bytes_secure(buf, 0) should be a successful no-op");
    FIO_ASSERT(fio_rand_bytes_secure(NULL, 0) == 0,
               "fio_rand_bytes_secure(NULL, 0) should succeed");
    FIO_ASSERT(fio_rand_bytes_secure(NULL, 8) == -1,
               "fio_rand_bytes_secure(NULL, 8) should fail");
  }
}

/* Regression: the two 64 bit halves of a 128 bit result (and consecutive
 * fio_rand64 values) must be uncorrelated. A mux/maj output combiner made
 * halves agree on 75% of bits (mean Hamming distance 24, ideal 32). With
 * 2^16 samples the mean's standard deviation is ~0.016 bits, so the 0.5
 * tolerance is ~30 sigma: no flakes, but any real correlation fails. */
FIO_DEFINE_RANDOM128_FN(static, fio___test_rng_det, 0, 0)
FIO_DEFINE_RANDOM128_FN(static, fio___test_rng_live, 11, 0)

static double fio___test_random_halves_distance(fio_u128 (*fn)(void)) {
  uint64_t total = 0;
  const size_t samples = (size_t)1 << 16;
  for (size_t i = 0; i < samples; ++i) {
    fio_u128 r = fn();
    total += (uint64_t)fio_popcount(r.u64[0] ^ r.u64[1]);
  }
  return (double)total / (double)samples;
}

static void fio___test_random_uncorrelated(void) {
  struct {
    fio_u128 (*fn)(void);
    const char *name;
  } srcs[] = {
      {fio_rand128, "fio_rand128"},
      {fio___test_rng_det128, "RANDOM128_FN (reseed_log 0)"},
      {fio___test_rng_live128, "RANDOM128_FN (reseed_log 11)"},
  };
  for (size_t i = 0; i < sizeof(srcs) / sizeof(srcs[0]); ++i) {
    double d = fio___test_random_halves_distance(srcs[i].fn);
    FIO_ASSERT(d > 31.5 && d < 32.5,
               "%s: 128 bit halves correlated (mean Hamming distance %.2f, "
               "expected 32)",
               srcs[i].name,
               d);
  }
  uint64_t total = 0;
  const size_t samples = (size_t)1 << 16;
  for (size_t i = 0; i < samples; ++i) {
    uint64_t a = fio_rand64();
    total += (uint64_t)fio_popcount(a ^ fio_rand64());
  }
  double d = (double)total / (double)samples;
  FIO_ASSERT(d > 31.5 && d < 32.5,
             "fio_rand64: consecutive values correlated (mean Hamming "
             "distance %.2f, expected 32)",
             d);
}

int main(void) {
  fio___test_random_edge_cases();
  fio___test_random_secure();
  fio___test_random_uncorrelated();
  return 0;
}
