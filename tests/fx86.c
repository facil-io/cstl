#ifndef FIO_FAKE_X86
#define FIO_FAKE_X86 1
#endif
#include "test-helpers.h"

static void fio___fx86_ref_slli_si128(uint8_t out[16],
                                      const uint8_t in[16],
                                      int imm8) {
  uint8_t const n = (uint8_t)imm8;
  FIO_MEMSET(out, 0, 16);
  if (!n)
    fio_memcpy16(out, in);
  else if (n < 16)
    FIO_MEMCPY(out + n, in, (size_t)(16 - n));
}

static void fio___fx86_ref_srli_si128(uint8_t out[16],
                                      const uint8_t in[16],
                                      int imm8) {
  uint8_t const n = (uint8_t)imm8;
  FIO_MEMSET(out, 0, 16);
  if (!n)
    fio_memcpy16(out, in);
  else if (n < 16)
    FIO_MEMCPY(out, in + n, (size_t)(16 - n));
}

static void fio___fx86_ref_alignr_epi8(uint8_t out[16],
                                       const uint8_t a[16],
                                       const uint8_t b[16],
                                       int imm8) {
  uint8_t const n = (uint8_t)imm8;
  uint8_t tmp[48];
  FIO_MEMSET(tmp, 0, 48);
  fio_memcpy16(tmp, b);
  fio_memcpy16(tmp + 16, a);
  if (n >= 32) {
    FIO_MEMSET(out, 0, 16);
    return;
  }
  fio_memcpy16(out, tmp + n);
}

static void fio___fx86_ref_alignr_epi8_256(uint8_t out[32],
                                           const uint8_t a[32],
                                           const uint8_t b[32],
                                           int imm8) {
  uint8_t const n = (uint8_t)imm8;
  uint8_t lo[48], hi[48];
  FIO_MEMSET(lo, 0, 48);
  FIO_MEMSET(hi, 0, 48);
  fio_memcpy16(lo, b);
  fio_memcpy16(lo + 16, a);
  fio_memcpy16(hi, b + 16);
  fio_memcpy16(hi + 16, a + 16);
  if (n >= 32) {
    FIO_MEMSET(out, 0, 32);
    return;
  }
  fio_memcpy16(out, lo + n);
  fio_memcpy16(out + 16, hi + n);
}

int main(void) {
  fprintf(stderr, "* fx86 tests\n");

  { /* imm8 normalization: byte shifts */
    uint8_t in[16], got[16], expect[16];
    int tests[] = {-1, 0, 1, 7, 15, 16, 17, 255, 256};
    for (size_t i = 0; i < 16; ++i)
      in[i] = (uint8_t)(i + 1);
    for (size_t i = 0; i < (sizeof(tests) / sizeof(*tests)); ++i) {
      int imm8 = tests[i];
      __m128i v = fio_fx86_loadu_si128(in);
      fio_fx86_storeu_si128(got, fio_fx86_slli_si128(v, imm8));
      fio___fx86_ref_slli_si128(expect, in, imm8);
      FIO_ASSERT(!FIO_MEMCMP(got, expect, 16),
                 "fio_fx86_slli_si128 mismatch for imm8=%d",
                 imm8);

      fio_fx86_storeu_si128(got, fio_fx86_srli_si128(v, imm8));
      fio___fx86_ref_srli_si128(expect, in, imm8);
      FIO_ASSERT(!FIO_MEMCMP(got, expect, 16),
                 "fio_fx86_srli_si128 mismatch for imm8=%d",
                 imm8);
    }
  }

  { /* imm8 normalization: 32-bit shifts */
    uint32_t in[4] = {0x80000001U, 0x7FFFFFFFU, 0x00000001U, 0xFFFFFFFFU};
    uint32_t got[4], expect[4];
    int tests[] = {-1, 0, 1, 16, 31, 32, 255};
    __m128i v = fio_fx86_loadu_si128(in);
    for (size_t t = 0; t < (sizeof(tests) / sizeof(*tests)); ++t) {
      uint8_t const n = (uint8_t)tests[t];
      fio_fx86_storeu_si128(got, fio_fx86_slli_epi32(v, tests[t]));
      for (size_t i = 0; i < 4; ++i)
        expect[i] = (n >= 32) ? 0 : (in[i] << n);
      FIO_ASSERT(!FIO_MEMCMP(got, expect, 16),
                 "fio_fx86_slli_epi32 mismatch for imm8=%d",
                 tests[t]);

      fio_fx86_storeu_si128(got, fio_fx86_srli_epi32(v, tests[t]));
      for (size_t i = 0; i < 4; ++i)
        expect[i] = (n >= 32) ? 0 : (in[i] >> n);
      FIO_ASSERT(!FIO_MEMCMP(got, expect, 16),
                 "fio_fx86_srli_epi32 mismatch for imm8=%d",
                 tests[t]);
    }
  }

  { /* imm8 normalization: 64-bit shifts */
    uint64_t in[2] = {0x8000000000000001ULL, 0x7FFFFFFFFFFFFFFFULL};
    uint64_t got[2], expect[2];
    int tests[] = {-1, 0, 1, 32, 63, 64, 255};
    __m128i v = fio_fx86_loadu_si128(in);
    for (size_t t = 0; t < (sizeof(tests) / sizeof(*tests)); ++t) {
      uint8_t const n = (uint8_t)tests[t];
      fio_fx86_storeu_si128(got, fio_fx86_slli_epi64(v, tests[t]));
      for (size_t i = 0; i < 2; ++i)
        expect[i] = (n >= 64) ? 0 : (in[i] << n);
      FIO_ASSERT(!FIO_MEMCMP(got, expect, 16),
                 "fio_fx86_slli_epi64 mismatch for imm8=%d",
                 tests[t]);

      fio_fx86_storeu_si128(got, fio_fx86_srli_epi64(v, tests[t]));
      for (size_t i = 0; i < 2; ++i)
        expect[i] = (n >= 64) ? 0 : (in[i] >> n);
      FIO_ASSERT(!FIO_MEMCMP(got, expect, 16),
                 "fio_fx86_srli_epi64 mismatch for imm8=%d",
                 tests[t]);
    }
  }

  { /* alignr semantics + bounds for 128-bit */
    uint8_t a[16], b[16], got[16], expect[16];
    int tests[] = {-1, 0, 4, 8, 15, 16, 17, 31, 32, 255};
    for (size_t i = 0; i < 16; ++i) {
      a[i] = (uint8_t)(0xA0U + i);
      b[i] = (uint8_t)(0x10U + i);
    }
    __m128i va = fio_fx86_loadu_si128(a);
    __m128i vb = fio_fx86_loadu_si128(b);
    for (size_t t = 0; t < (sizeof(tests) / sizeof(*tests)); ++t) {
      fio_fx86_storeu_si128(got, fio_fx86_alignr_epi8(va, vb, tests[t]));
      fio___fx86_ref_alignr_epi8(expect, a, b, tests[t]);
      FIO_ASSERT(!FIO_MEMCMP(got, expect, 16),
                 "fio_fx86_alignr_epi8 mismatch for imm8=%d",
                 tests[t]);
    }
  }

  { /* alignr semantics + bounds for 256-bit lanes */
    uint8_t a[32], b[32], got[32], expect[32];
    int tests[] = {-1, 0, 8, 15, 16, 17, 31, 32, 255};
    for (size_t i = 0; i < 32; ++i) {
      a[i] = (uint8_t)(0x80U + i);
      b[i] = (uint8_t)(0x20U + i);
    }
    __m256i va = fio_fx86_256_loadu_si256(a);
    __m256i vb = fio_fx86_256_loadu_si256(b);
    for (size_t t = 0; t < (sizeof(tests) / sizeof(*tests)); ++t) {
      fio_fx86_256_storeu_si256(got,
                                fio_fx86_256_alignr_epi8(va, vb, tests[t]));
      fio___fx86_ref_alignr_epi8_256(expect, a, b, tests[t]);
      FIO_ASSERT(!FIO_MEMCMP(got, expect, 32),
                 "fio_fx86_256_alignr_epi8 mismatch for imm8=%d",
                 tests[t]);
    }
  }

  { /* movemask UB regression: bit 31 must be well-defined */
    uint8_t b32[32] = {0};
    uint32_t expect = 0;
    b32[0] = 0x80;
    b32[7] = 0xFF;
    b32[15] = 0x80;
    b32[16] = 0x80;
    b32[23] = 0x80;
    b32[31] = 0x80;
    for (size_t i = 0; i < 32; ++i)
      expect |= ((uint32_t)((b32[i] >> 7) & 1U) << i);
    {
      __m256i v = fio_fx86_256_loadu_si256(b32);
      uint32_t got = (uint32_t)fio_fx86_256_movemask_epi8(v);
      FIO_ASSERT(
          got == expect,
          "fio_fx86_256_movemask_epi8 mismatch (got=0x%08x expect=0x%08x)",
          got,
          expect);
    }

    FIO_MEMSET(b32, 0, 32);
    b32[31] = 0x80;
    {
      __m256i v = fio_fx86_256_loadu_si256(b32);
      uint32_t got = (uint32_t)fio_fx86_256_movemask_epi8(v);
      FIO_ASSERT(got == 0x80000000U,
                 "fio_fx86_256_movemask_epi8 high-bit mismatch (got=0x%08x)",
                 got);
    }
  }

  { /* imm8 normalization: AVX2 shifts */
    uint32_t in_u[8] =
        {0x80000001U, 0x70000000U, 3U, 0xFFFFFFFFU, 9U, 11U, 13U, 17U};
    int32_t in_i[8] = {INT32_MIN, -1, -2, -3, 1, 2, 3, 4};
    uint32_t got_u[8], expect_u[8];
    int32_t got_i[8], expect_i[8];
    int tests[] = {-1, 0, 1, 16, 31, 32, 255};

    __m256i vu = fio_fx86_256_loadu_si256(in_u);
    __m256i vi = fio_fx86_256_loadu_si256(in_i);
    for (size_t t = 0; t < (sizeof(tests) / sizeof(*tests)); ++t) {
      uint8_t const n = (uint8_t)tests[t];

      fio_fx86_256_storeu_si256(got_u, fio_fx86_256_slli_epi32(vu, tests[t]));
      for (size_t i = 0; i < 8; ++i)
        expect_u[i] = (n >= 32) ? 0 : (in_u[i] << n);
      FIO_ASSERT(!FIO_MEMCMP(got_u, expect_u, 32),
                 "fio_fx86_256_slli_epi32 mismatch for imm8=%d",
                 tests[t]);

      fio_fx86_256_storeu_si256(got_i, fio_fx86_256_srai_epi32(vi, tests[t]));
      for (size_t i = 0; i < 8; ++i)
        expect_i[i] = (n >= 32) ? (in_i[i] >> 31) : (in_i[i] >> n);
      FIO_ASSERT(!FIO_MEMCMP(got_i, expect_i, 32),
                 "fio_fx86_256_srai_epi32 mismatch for imm8=%d",
                 tests[t]);
    }
  }

  fprintf(stderr, "* fx86 tests passed\n");
  return 0;
}
