/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        Random Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_RAND_TEST___H)
#define H___FIO_RAND_TEST___H
#ifndef H___FIO_RAND___H
#define FIO_RAND
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

/* *****************************************************************************
Playhouse hashing (next risky version)
***************************************************************************** */

typedef union {
  uint64_t v[4] FIO_ALIGN(32);
#ifdef __SIZEOF_INT128__
  __uint128_t u128[2];
#endif
} fio___r2hash_s;

FIO_IFUNC fio___r2hash_s fio_risky2_hash___inner(const void *restrict data_,
                                                 size_t len,
                                                 uint64_t seed) {
  fio___r2hash_s v = {.v = {seed, seed, seed, seed}};
  fio___r2hash_s const prime = {.v = {FIO_U64_HASH_PRIME0,
                                      FIO_U64_HASH_PRIME1,
                                      FIO_U64_HASH_PRIME2,
                                      FIO_U64_HASH_PRIME3}};
  fio___r2hash_s w;
  const uint8_t *data = (const uint8_t *)data_;
  /* seed selection is constant time to avoid leaking seed data */
  seed += len;
  seed ^= fio_lrot64(seed, 47);
  seed ^= FIO_U64_HASH_PRIME4;

#define FIO___R2_ROUND(i) /* this version passes all, but fast enough? */      \
  w.v[i] = fio_ltole64(w.v[i]); /* make sure we're using little endien? */     \
  v.v[i] ^= w.v[i];                                                            \
  v.v[i] *= prime.v[i];                                                        \
  w.v[i] = fio_lrot64(w.v[i], 31);                                             \
  v.v[i] += w.v[i];                                                            \
  v.v[i] ^= seed;

  /* consumes 32 bytes (256 bits) blocks (no padding needed) */
  for (size_t pos = 31; pos < len; pos += 32) {
    for (size_t i = 0; i < 4; ++i) {
      fio_memcpy8(w.v + i, data + (i << 3));
      FIO___R2_ROUND(i);
    }
    seed += w.v[0] + w.v[1] + w.v[2] + w.v[3];
    data += 32;
  }
#if (FIO___R2_PERFORM_FULL_BLOCK + 1) && 1
  if (len & 31) { // pad with zeros
    uint64_t tmp_buf[4] = {0};
    fio_memcpy31x(tmp_buf, data, len);
    for (size_t i = 0; i < 4; ++i) {
      w.v[0] = tmp_buf[1];
      FIO___R2_ROUND(i);
    }
  }
#else
  switch (len & 24) { /* only performed if data exits in these positions */
  case 24: fio_memcpy8(w.v + 2, data + 16); FIO___R2_ROUND(2); /*fall through*/
  case 16: fio_memcpy8(w.v + 1, data + 8); FIO___R2_ROUND(1);  /*fall through*/
  case 8:
    fio_memcpy8(w.v + 0, data);
    FIO___R2_ROUND(0);
    data += len & 24;
  }
  if (len & 7) {
    uint64_t i = (len & 24) >> 3;
    w.v[i] = 0;
    fio_memcpy7x(w.v + i, data, len);
    FIO___R2_ROUND(i);
  }
#endif

  /* inner vector mini-avalanche */
  for (size_t i = 0; i < 4; ++i)
    v.v[i] *= prime.v[i];
  v.v[0] ^= fio_lrot64(v.v[0], 7);
  v.v[1] ^= fio_lrot64(v.v[1], 11);
  v.v[2] ^= fio_lrot64(v.v[2], 13);
  v.v[3] ^= fio_lrot64(v.v[3], 17);
  return v;
#undef FIO___R2_ROUND
}

/*  Computes a facil.io Stable Hash. */
FIO_SFUNC uint64_t fio_risky2_hash(const void *data_,
                                   size_t len,
                                   uint64_t seed) {
  uint64_t r;
  fio___r2hash_s v = fio_risky2_hash___inner(data_, len, seed);
  /* summing avalanche */
  r = v.v[0] + v.v[1] + v.v[2] + v.v[3];
  r ^= r >> 31;
  r *= FIO_U64_HASH_PRIME4;
  r ^= r >> 31;
  return r;
}

FIO_SFUNC void fio_risky2_hash128(void *restrict dest,
                                  const void *restrict data_,
                                  size_t len,
                                  uint64_t seed) {
  fio___r2hash_s v = fio_risky2_hash___inner(data_, len, seed);
  uint64_t r[2];
  r[0] = v.v[0] + v.v[1] + v.v[2] + v.v[3];
  r[1] = v.v[0] ^ v.v[1] ^ v.v[2] ^ v.v[3];
  r[0] ^= r[0] >> 31;
  r[1] ^= r[1] >> 31;
  r[0] *= FIO_U64_HASH_PRIME4;
  r[1] *= FIO_U64_HASH_PRIME0;
  r[0] ^= r[0] >> 31;
  r[1] ^= r[1] >> 31;
  fio_memcpy16(dest, r);
}

#undef FIO___R2_HASH_MUL_PRIME
#undef FIO___R2_HASH_ROUND_FULL

/* *****************************************************************************
Hashing speed test
***************************************************************************** */
#include <math.h>

typedef uintptr_t (*fio__hashing_func_fn)(char *, size_t);

FIO_SFUNC void fio_test_hash_function(fio__hashing_func_fn h,
                                      char *name,
                                      uint8_t size_log,
                                      uint8_t mem_alignment_offset,
                                      uint8_t fast) {
  /* test based on code from BearSSL with credit to Thomas Pornin */
  if (size_log >= 21 || ((sizeof(uint64_t) - 1) >> size_log)) {
    FIO_LOG_ERROR("fio_test_hash_function called with a log size too big.");
    return;
  }
  mem_alignment_offset &= 7;
  size_t const buffer_len = (1ULL << size_log) - mem_alignment_offset;
  uint64_t cycles_start_at = (1ULL << (14 + (fast * 3)));
  if (size_log < 13)
    cycles_start_at <<= (13 - size_log);
  else if (size_log > 13)
    cycles_start_at >>= (size_log - 13);

#ifdef DEBUG
  fprintf(stderr,
          "* Testing %s speed with %zu byte blocks"
          "(DEBUG mode detected - speed may be affected).\n",
          name,
          buffer_len);
#else
  fprintf(stderr,
          "* Testing %s speed with %zu byte blocks.\n",
          name,
          buffer_len);
#endif

  uint8_t *buffer_mem = (uint8_t *)
      FIO_MEM_REALLOC(NULL, 0, (buffer_len + mem_alignment_offset) + 64, 0);
  uint8_t *buffer = buffer_mem + mem_alignment_offset;

  FIO_MEMSET(buffer, 'T', buffer_len);
  /* warmup */
  uint64_t hash = 0;
  for (size_t i = 0; i < 4; i++) {
    hash += h((char *)buffer, buffer_len);
    fio_memcpy8(buffer, &hash);
  }
  /* loop until test runs for more than 2 seconds */
  for (uint64_t cycles = cycles_start_at;;) {
    clock_t start, end;
    start = clock();
    for (size_t i = cycles; i > 0; i--) {
      hash += h((char *)buffer, buffer_len);
      FIO_COMPILER_GUARD;
    }
    end = clock();
    fio_memcpy8(buffer, &hash);
    if ((end - start) > CLOCKS_PER_SEC || cycles >= ((uint64_t)1 << 62)) {
      fprintf(stderr,
              "\t%-40s %8.2f MB/s\n",
              name,
              (double)(buffer_len * cycles) /
                  (((end - start) * (1000000.0 / CLOCKS_PER_SEC))));
      break;
    }
    cycles <<= 1;
  }
  FIO_MEM_FREE(buffer_mem, (buffer_len + mem_alignment_offset) + 64);
}

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, risky_wrapper)(char *buf, size_t len) {
  return fio_risky_hash(buf, len, 1);
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, stable_wrapper)(char *buf, size_t len) {
  return fio_stable_hash(buf, len, 1);
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, risky2_wrapper)(char *buf, size_t len) {
  return fio_risky2_hash(buf, len, 1);
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
  fprintf(stderr, "* Testing Risky Hash and Risky Mask (sanity).\n");
  {
    char *str = (char *)"testing that risky hash is always the same hash";
    const size_t len = FIO_STRLEN(str);
    char buf[128];
    FIO_MEMCPY(buf, str, len);
    uint64_t org_hash = fio_risky_hash(buf, len, 0);
    FIO_ASSERT(!memcmp(buf, str, len), "hashing shouldn't touch data");
    for (int i = 0; i < 8; ++i) {
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
      for (int bit = 0; bit < 8; ++bit) {
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
          "\t  zeros / ones (bit frequency)\t%.05f\n",
          ((float)1.0 * totals[0]) / totals[1]);
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

FIO_SFUNC void FIO_NAME_TEST(stl, random)(void) {
  fprintf(stderr,
          "* Testing randomness "
          "- bit frequency / hemming distance / chi-square.\n");
  const size_t test_len = (1UL << 21);
  uint64_t *rs =
      (uint64_t *)FIO_MEM_REALLOC(NULL, 0, sizeof(*rs) * test_len, 0);
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
      if (RAND_MAX <= (~(0ULL)) >> rand_bits)
        break;
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

  fio_rand_feed2seed(rs, sizeof(*rs) * test_len);
  FIO_MEM_FREE(rs, sizeof(*rs) * test_len);
  fprintf(stderr, "\n");
#if DEBUG
  fprintf(stderr,
          "\t- to compare CPU cycles, test randomness with optimization.\n\n");
#endif /* DEBUG */
}
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
