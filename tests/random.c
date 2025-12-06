/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_RAND
#include FIO_INCLUDE_FILE

FIO_DEFINE_RANDOM128_FN(FIO_SFUNC, fio___prng, 31, 0)
/* *****************************************************************************
Playhouse hashing (next risky version)
***************************************************************************** */

FIO_IFUNC void fio___risky2_round(fio_u256 *restrict v, uint8_t *bytes32) {
  const fio_u512 primes = {.u64 = {
                               FIO_U32_HASH_PRIME0,
                               FIO_U32_HASH_PRIME1,
                               FIO_U32_HASH_PRIME2,
                               FIO_U32_HASH_PRIME3,
                               FIO_U32_HASH_PRIME4,
                               FIO_U32_HASH_PRIME5,
                               FIO_U32_HASH_PRIME6,
                               FIO_U32_HASH_PRIME7,
                           }};
  fio_u512 in = {.u64 = {
                     1 + fio_buf2u32u(bytes32),
                     1 + fio_buf2u32u(bytes32 + 4),
                     1 + fio_buf2u32u(bytes32 + 8),
                     1 + fio_buf2u32u(bytes32 + 12),
                     1 + fio_buf2u32u(bytes32 + 16),
                     1 + fio_buf2u32u(bytes32 + 20),
                     1 + fio_buf2u32u(bytes32 + 24),
                     1 + fio_buf2u32u(bytes32 + 28),
                 }};
  FIO_FOR(i, 8) { in.u64[i] *= primes.u64[i]; }
  FIO_FOR(i, 8) { in.u32[(i << 1)] += in.u32[(i << 1) + 1]; }
  FIO_FOR(i, 8) { v->u32[i] += in.u32[(i << 1)]; }
}
/*  Computes a facil.io Stable Hash. */
FIO_SFUNC uint64_t fio_risky2_hash(const void *data_,
                                   size_t len,
                                   uint64_t seed) {
  uint64_t r;
  uint8_t *data = (uint8_t *)data_;
  fio_u256 v = {.u64 = {(FIO_U64_HASH_PRIME0 + seed) + (len),
                        (FIO_U64_HASH_PRIME1 - seed) + (len << 1),
                        (FIO_U64_HASH_PRIME2 + seed) + (len << 2),
                        (FIO_U64_HASH_PRIME3 ^ seed) + (len << 3)}};
  for (size_t i = 31; i < len; i += 32) {
    fio___risky2_round(&v, data);
    data += 32;
  }
  if ((len & 31)) { /* leftover + length input */
    fio_u256 buf = {0};
    fio_memcpy31x(buf.u8, data, len);
    fio___risky2_round(&v, buf.u8);
  }
  /* reducing */
  r = v.u64[0] ^ v.u64[1] ^ v.u64[2] ^ v.u64[3];
  r ^= v.u64[0] + v.u64[1] + v.u64[2] + v.u64[3];
  return r;
}

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, risky2_wrapper)(char *buf, size_t len) {
  return fio_risky2_hash(buf, len, 1);
}

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, risky_wrapper)(char *buf, size_t len) {
  return fio_risky_hash(buf, len, 1);
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, stable_wrapper)(char *buf, size_t len) {
  return fio_stable_hash(buf, len, 1);
}

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, risky_ptr_wrapper)(char *buf,
                                                          size_t len) {
  uint64_t h[4] = {0};
  while (len > 31) {
    h[0] += fio_risky_ptr((void *)fio_buf2u64u(buf));
    h[1] += fio_risky_ptr((void *)fio_buf2u64u(buf + 8));
    h[2] += fio_risky_ptr((void *)fio_buf2u64u(buf + 16));
    h[3] += fio_risky_ptr((void *)fio_buf2u64u(buf + 24));
    len -= 32;
    buf += 32;
  }
  if ((len & 31)) {
    uint64_t t[4] = {0};
    fio_memcpy31x(t, buf, len);
    h[0] += fio_risky_ptr((void *)t[0]);
    h[1] += fio_risky_ptr((void *)t[1]);
    h[2] += fio_risky_ptr((void *)t[2]);
    h[3] += fio_risky_ptr((void *)t[3]);
  }
  return h[0] + h[1] + h[2] + h[3];
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, risky_num_wrapper)(char *buf,
                                                          size_t len) {
  uint64_t h[4] = {0};
  while (len > 31) {
    h[0] += fio_risky_num(fio_buf2u64u(buf), 0);
    h[1] += fio_risky_num(fio_buf2u64u(buf + 8), 0);
    h[2] += fio_risky_num(fio_buf2u64u(buf + 16), 0);
    h[3] += fio_risky_num(fio_buf2u64u(buf + 24), 0);
    len -= 32;
    buf += 32;
  }
  if ((len & 31)) {
    uint64_t t[4] = {0};
    fio_memcpy31x(t, buf, len);
    h[0] += fio_risky_num(t[0], 0);
    h[1] += fio_risky_num(t[1], 0);
    h[2] += fio_risky_num(t[2], 0);
    h[3] += fio_risky_num(t[3], 0);
  }
  return h[0] + h[1] + h[2] + h[3];
}

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, xmask_wrapper)(char *buf, size_t len) {
  fio_xmask(buf, len, fio_rand64());
  return len;
}

/* tests Risky Hash and Stable Hash... takes a while (speed tests as well) */
FIO_SFUNC void FIO_NAME_TEST(stl, risky)(void) {
  fprintf(stderr, "\t Testing Risky Hash and Risky Mask (sanity).\n");
  {
    char *str = (char *)"testing that risky hash is always the same hash";
    const size_t len = FIO_STRLEN(str);
    char buf[128];
    FIO_MEMCPY(buf, str, len);
    uint64_t org_hash = fio_risky_hash(buf, len, 0);
    FIO_ASSERT(!memcmp(buf, str, len), "hashing shouldn't touch data");
    for (size_t i = 0; i < 8; ++i) {
      char *tmp = buf + i;
      FIO_MEMCPY(tmp, str, len);
      uint64_t tmp_hash = fio_risky_hash(tmp, len, 0);
      FIO_ASSERT(tmp_hash == fio_risky_hash(tmp, len, 0),
                 "hash should be consistent!");
      FIO_ASSERT(tmp_hash == org_hash, "memory address shouldn't effect hash!");
    }
  }
#if !DEBUG
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_wrapper),
                         (char *)"fio_risky_hash",
                         7,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_wrapper),
                         (char *)"fio_risky_hash",
                         13,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_wrapper),
                         (char *)"fio_risky_hash (unaligned)",
                         6,
                         3,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_wrapper),
                         (char *)"fio_risky_hash (unaligned)",
                         5,
                         3,
                         2);
  fprintf(stderr, "\n");
  fio_test_hash_function(FIO_NAME_TEST(stl, stable_wrapper),
                         (char *)"fio_stable_hash (64 bit)",
                         7,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, stable_wrapper),
                         (char *)"fio_stable_hash (64 bit)",
                         13,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, stable_wrapper),
                         (char *)"fio_stable_hash (64 bit unaligned)",
                         6,
                         3,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, stable_wrapper),
                         (char *)"fio_stable_hash (64 bit unaligned)",
                         5,
                         3,
                         2);
  fprintf(stderr, "\n");
#if 0  /* speed test num and ptr hashing */
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_ptr_wrapper),
                         (char *)"fio_risky_ptr (emulated)",
                         7,
                         0,
                         2);
  fprintf(stderr, "\n");
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_num_wrapper),
                         (char *)"fio_risky_num (emulated)",
                         7,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_num_wrapper),
                         (char *)"fio_risky_num (emulated)",
                         13,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_num_wrapper),
                         (char *)"fio_risky_num (emulated)",
                         6,
                         3,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky_num_wrapper),
                         (char *)"fio_risky_num (emulated)",
                         5,
                         3,
                         2);
#endif /* speed test num and ptr hashing */

  /* xmask speed testing */
  fprintf(stderr, "\n");
  fio_test_hash_function(FIO_NAME_TEST(stl, xmask_wrapper),
                         (char *)"fio_xmask (XOR, NO counter)",
                         13,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, xmask_wrapper),
                         (char *)"fio_xmask (unaligned)",
                         13,
                         1,
                         2);

#if 0  /* speed test playground */
  /* playground speed testing */
  fprintf(stderr, "\n");
  fio_test_hash_function(FIO_NAME_TEST(stl, risky2_wrapper),
                         (char *)"rXtest (64 bit)",
                         7,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky2_wrapper),
                         (char *)"rXtest (64 bit)",
                         13,
                         0,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky2_wrapper),
                         (char *)"rXtest (64 bit unaligned)",
                         6,
                         3,
                         2);
  fio_test_hash_function(FIO_NAME_TEST(stl, risky2_wrapper),
                         (char *)"rXtest (64 bit unaligned)",
                         5,
                         3,
                         2);
#endif /* speed test playground */
  fprintf(stderr, "\n");
#endif /* DEBUG */
}

FIO_SFUNC void FIO_NAME_TEST(stl, random_buffer)(uint64_t *stream,
                                                 size_t len,
                                                 const char *name,
                                                 size_t clk) {
  size_t totals[2] = {0};
  size_t freq[256] = {0};
  const size_t total_bits = (len * sizeof(*stream) * 8);
  uint64_t hemming = 0;
  /* collect data */
  for (size_t i = 1; i < len; i += 2) {
    hemming += fio_hemming_dist(stream[i], stream[i - 1]);
    for (size_t byte = 0; byte < (sizeof(*stream) << 1); ++byte) {
      uint8_t val = ((uint8_t *)(stream + (i - 1)))[byte];
      ++freq[val];
      for (size_t bit = 0; bit < 8; ++bit) {
        ++totals[(val >> bit) & 1];
      }
    }
  }
  hemming /= len;
  fprintf(stderr, "\n");
#if DEBUG
  fprintf(stderr,
          "\t- \x1B[1m%s\x1B[0m (%zu CPU cycles NOT OPTIMIZED):\n",
          name,
          clk);
#else
  fprintf(stderr, "\t- \x1B[1m%s\x1B[0m (%zu CPU cycles):\n", name, clk);
#endif
  fprintf(stderr,
          "\t  zeros / ones (frequency bias)\t%.05f%% (should be near zero)\n",
          ((((float)100.0 * totals[0]) / totals[1]) > (float)100.0
               ? ((((float)100.0 * totals[0]) / totals[1]) - 100)
               : ((float)100 - (((float)100.0 * totals[0]) / totals[1]))));
  if (!(totals[0] < totals[1] + (total_bits / 20) &&
        totals[1] < totals[0] + (total_bits / 20)))
    FIO_LOG_ERROR("randomness isn't random?");
  fprintf(stderr,
          "\t  avarage hemming distance\t%zu (should be: 14-18)\n",
          (size_t)hemming);
  /* expect avarage hemming distance of 25% == 16 bits */
  if (!(hemming >= 14 && hemming <= 18))
    FIO_LOG_ERROR("randomness isn't random (hemming distance failed)?");
  /* test chi-square ... I think */
  if (len * sizeof(*stream) > 2560) {
    double n_r = (double)1.0 * ((len * sizeof(*stream)) / 256);
    double chi_square = 0;
    for (unsigned int i = 0; i < 256; ++i) {
      double f = freq[i] - n_r;
      chi_square += (f * f);
    }
    chi_square /= n_r;
    double chi_square_r_abs =
        (chi_square - 256 >= 0) ? chi_square - 256 : (256 - chi_square);
    fprintf(
        stderr,
        "\t  chi-sq. variation\t\t%.02lf - %s (expect <= %0.2lf)\n",
        chi_square_r_abs,
        ((chi_square_r_abs <= 2 * (sqrt(n_r)))
             ? "good"
             : ((chi_square_r_abs <= 3 * (sqrt(n_r))) ? "not amazing"
                                                      : "\x1B[1mBAD\x1B[0m")),
        2 * (sqrt(n_r)));
  }
}

int main(void) {
  const size_t test_len = (1UL << 21);
  uint64_t *rs =
      (uint64_t *)FIO_MEM_REALLOC(NULL, 0, sizeof(*rs) * test_len, 0);
  fprintf(
      stderr,
      "\t * Testing randomness "
      "- bit frequency / hemming distance / chi-square (%zu random bytes).\n",
      (size_t)(sizeof(*rs) * test_len));
  clock_t start, end;
  FIO_ASSERT_ALLOC(rs);

  rand(); /* warmup */
  if (sizeof(int) < sizeof(uint64_t)) {
    start = clock();
    for (size_t i = 0; i < test_len; ++i) {
      rs[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
    end = clock();
  } else {
    start = clock();
    for (size_t i = 0; i < test_len; ++i) {
      rs[i] = (uint64_t)rand();
    }
    end = clock();
  }
  FIO_NAME_TEST(stl, random_buffer)
  (rs, test_len, "rand (system - naive, ignoring missing bits)", end - start);

  FIO_MEMSET(rs, 0, sizeof(*rs) * test_len);
  {
    if (RAND_MAX == ~(uint64_t)0ULL) {
      /* RAND_MAX fills all bits */
      start = clock();
      for (size_t i = 0; i < test_len; ++i) {
        rs[i] = (uint64_t)rand();
      }
      end = clock();
    } else if (RAND_MAX >= (~(uint32_t)0UL)) {
      /* RAND_MAX fill at least 32 bits per call */
      uint32_t *rs_adjusted = (uint32_t *)rs;

      start = clock();
      for (size_t i = 0; i < (test_len << 1); ++i) {
        rs_adjusted[i] = (uint32_t)rand();
      }
      end = clock();
    } else if (RAND_MAX >= (~(uint16_t)0U)) {
      /* RAND_MAX fill at least 16 bits per call */
      uint16_t *rs_adjusted = (uint16_t *)rs;

      start = clock();
      for (size_t i = 0; i < (test_len << 2); ++i) {
        rs_adjusted[i] = (uint16_t)rand();
      }
      end = clock();
    } else {
      /* assume RAND_MAX fill at least 8 bits per call */
      uint8_t *rs_adjusted = (uint8_t *)rs;

      start = clock();
      for (size_t i = 0; i < (test_len << 2); ++i) {
        rs_adjusted[i] = (uint8_t)rand();
      }
      end = clock();
    }
    /* test RAND_MAX value */
    uint8_t rand_bits = 63;
    while (rand_bits) {
      if (RAND_MAX <= (~(0ULL)) >> rand_bits) break;
      --rand_bits;
    }
    rand_bits = 64 - rand_bits;

    char buffer[128] = {0};
    snprintf(buffer,
             128 - 14,
             "rand (system - fixed, testing %d random bits)",
             (int)rand_bits);
    FIO_NAME_TEST(stl, random_buffer)(rs, test_len, buffer, end - start);
  }

  FIO_MEMSET(rs, 0, sizeof(*rs) * test_len);
  fio_rand64(); /* warmup */
  start = clock();
  for (size_t i = 0; i < test_len; ++i) {
    rs[i] = fio_rand64();
  }
  end = clock();
  FIO_NAME_TEST(stl, random_buffer)(rs, test_len, "fio_rand64", end - start);
  FIO_MEMSET(rs, 0, sizeof(*rs) * test_len);
  start = clock();
  fio_rand_bytes(rs, test_len * sizeof(*rs));
  end = clock();
  FIO_NAME_TEST(stl, random_buffer)
  (rs, test_len, "fio_rand_bytes", end - start);

  FIO_MEMSET(rs, 0, sizeof(*rs) * test_len);
  fio___prng64(); /* warmup */
  start = clock();
  for (size_t i = 0; i < test_len; ++i) {
    rs[i] = fio___prng64();
  }
  end = clock();
  FIO_NAME_TEST(stl, random_buffer)
  (rs, test_len, "PNGR128/64bits", end - start);
  FIO_MEMSET(rs, 0, sizeof(*rs) * test_len);
  start = clock();
  fio___prng_bytes(rs, test_len * sizeof(*rs));
  end = clock();
  FIO_NAME_TEST(stl, random_buffer)
  (rs, test_len, "PNGR128_bytes", end - start);

  FIO_MEM_FREE(rs, sizeof(*rs) * test_len);
  fprintf(stderr, "\n");
  {
    FIO_STR_INFO_TMP_VAR(data, 1124);
    data.len = 1024;
    for (size_t i = 0; i < data.len; ++i) data.buf[i] = (char)(i & 255);
    uint64_t h = fio_stable_hash(data.buf, 1024, 0);
    FIO_LOG_DDEBUG2("Stable Hash Value: %p", (void *)h);
    FIO_ASSERT(h == (uint64_t)0x5DC4DAD435547F67ULL,
               "Stable Hash Value Error!");
  }
#if DEBUG
  fprintf(stderr,
          "\t- to compare CPU cycles, test randomness with optimization.\n\n");
#endif /* DEBUG */
}
