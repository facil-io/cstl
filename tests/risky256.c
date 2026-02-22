/* *****************************************************************************
Comprehensive Security/Quality Tests for fio_risky256 / fio_risky512

Tests the A3 (Zero-Copy ILP) hash variation.
Categories: sanity, avalanche, collision, differential, HMAC,
            length extension resistance, known answer tests, speed sanity,
            absorption boundaries.

Compile: make tests/risky256
***************************************************************************** */
#include "test-helpers.h"

#define FIO_RAND
#include FIO_INCLUDE_FILE

/* *****************************************************************************
A3 Implementation — provided by the library via FIO_RAND.
fio_risky256() and fio_risky512() are defined in fio-stl/002 random.h.
***************************************************************************** */

/* *****************************************************************************
Helpers
***************************************************************************** */

/** Compare two fio_u256 values, return 0 if equal. */
FIO_IFUNC int fio___u256_cmp(fio_u256 a, fio_u256 b) {
  return FIO_MEMCMP(&a, &b, 32);
}

/** Compare two fio_u512 values, return 0 if equal. */
FIO_IFUNC int fio___u512_cmp(fio_u512 a, fio_u512 b) {
  return FIO_MEMCMP(&a, &b, 64);
}

/** Count set bits in a byte array (Hamming weight). */
FIO_SFUNC size_t fio___popcount_bytes(const uint8_t *buf, size_t len) {
  size_t count = 0;
  for (size_t i = 0; i < len; ++i) {
    uint8_t b = buf[i];
    while (b) {
      count += (b & 1);
      b >>= 1;
    }
  }
  return count;
}

/** XOR two byte arrays into dst. */
FIO_SFUNC void fio___xor_bytes(uint8_t *dst,
                               const uint8_t *a,
                               const uint8_t *b,
                               size_t len) {
  for (size_t i = 0; i < len; ++i)
    dst[i] = a[i] ^ b[i];
}

/** Print a fio_u256 in hex. */
FIO_SFUNC void fio___print_u256(const char *label, fio_u256 h) {
  fprintf(stderr, "    %s: ", label);
  for (size_t i = 0; i < 32; ++i)
    fprintf(stderr, "%02x", h.u8[i]);
  fprintf(stderr, "\n");
}

/** Print a fio_u512 in hex. */
FIO_SFUNC void fio___print_u512(const char *label, fio_u512 h) {
  fprintf(stderr, "    %s: ", label);
  for (size_t i = 0; i < 64; ++i)
    fprintf(stderr, "%02x", h.u8[i]);
  fprintf(stderr, "\n");
}

/** qsort comparator for uint64_t. */
FIO_SFUNC int fio___cmp_u64(const void *a, const void *b) {
  uint64_t va = *(const uint64_t *)a;
  uint64_t vb = *(const uint64_t *)b;
  return (va > vb) - (va < vb);
}

/** qsort comparator for fio_u256 (full 32-byte comparison). */
FIO_SFUNC int fio___cmp_u256(const void *a, const void *b) {
  return FIO_MEMCMP(a, b, 32);
}

/* *****************************************************************************
1. Basic Sanity Tests
***************************************************************************** */

FIO_SFUNC int fio___test_risky256_sanity(void) {
  int failures = 0;
  fprintf(stderr, "  * [1] Basic Sanity Tests\n");

  /* 1a. Empty string hash is non-zero and deterministic */
  {
    fio_u256 h1 = fio_risky256("", 0);
    fio_u256 h2 = fio_risky256("", 0);
    fio_u256 zero = {{0}};
    int is_zero = !fio___u256_cmp(h1, zero);
    int is_deterministic = !fio___u256_cmp(h1, h2);
    if (is_zero) {
      fprintf(stderr, "    hash(\"\",0) == 0: * FAIL *\n");
      ++failures;
    }
    if (!is_deterministic) {
      fprintf(stderr, "    hash(\"\",0) not deterministic: * FAIL *\n");
      ++failures;
    }
    if (!is_zero && is_deterministic)
      fprintf(stderr, "    1a. Empty string non-zero & deterministic: PASS\n");
  }

  /* 1b. hash("abc") is deterministic */
  {
    fio_u256 h1 = fio_risky256("abc", 3);
    fio_u256 h2 = fio_risky256("abc", 3);
    if (fio___u256_cmp(h1, h2)) {
      fprintf(stderr, "    1b. hash(\"abc\") deterministic: * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr, "    1b. hash(\"abc\") deterministic: PASS\n");
    }
  }

  /* 1c. Different inputs produce different hashes */
  {
    int collisions = 0;
    for (int i = 0; i < 100; ++i) {
      uint64_t a = (uint64_t)i;
      uint64_t b = (uint64_t)(i + 100);
      fio_u256 ha = fio_risky256(&a, sizeof(a));
      fio_u256 hb = fio_risky256(&b, sizeof(b));
      if (!fio___u256_cmp(ha, hb))
        ++collisions;
    }
    if (collisions) {
      fprintf(stderr,
              "    1c. Different inputs (%d/100 collisions): * FAIL *\n",
              collisions);
      ++failures;
    } else {
      fprintf(stderr, "    1c. Different inputs (0/100 collisions): PASS\n");
    }
  }

  /* 1d. Different HMAC keys produce different outputs */
  {
    const char *msg = "test message";
    int collisions = 0;
    for (uint64_t s = 0; s < 100; ++s) {
      uint64_t k1 = s;
      uint64_t k2 = s + 100;
      fio_u256 ha = fio_risky256_hmac(&k1, sizeof(k1), msg, 12);
      fio_u256 hb = fio_risky256_hmac(&k2, sizeof(k2), msg, 12);
      if (!fio___u256_cmp(ha, hb))
        ++collisions;
    }
    if (collisions) {
      fprintf(stderr,
              "    1d. Different HMAC keys (%d/100 collisions): * FAIL *\n",
              collisions);
      ++failures;
    } else {
      fprintf(stderr, "    1d. Different HMAC keys (0/100 collisions): PASS\n");
    }
  }

  /* 1e. 512-bit output extends 256-bit (first half matches, second half is new)
     The 256-bit hash is the first squeeze; 512-bit adds a second squeeze.
     This is the same pattern as SHAKE: truncation is safe. */
  {
    fio_u256 h256 = fio_risky256("test", 4);
    fio_u512 h512 = fio_risky512("test", 4);
    /* First 256 bits of 512 should equal the 256-bit hash (same squeeze) */
    int first_half_match = !FIO_MEMCMP(&h256, &h512, 32);
    /* Second 256 bits should be different from the first 256 bits */
    int halves_differ = !!FIO_MEMCMP(&h512.u64[0], &h512.u64[4], 32);
    if (!first_half_match) {
      fprintf(stderr, "    1e. 512-bit first half == 256-bit: * FAIL *\n");
      ++failures;
    } else if (!halves_differ) {
      fprintf(stderr, "    1e. 512-bit halves are identical: * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr, "    1e. 512-bit extends 256-bit correctly: PASS\n");
    }
  }

  /* 1f. Alignment independence */
  {
    uint8_t buf[128];
    fio_rand_bytes(buf, 128);
    int align_fail = 0;
    for (int off = 0; off < 8; ++off) {
      /* Copy 64 bytes to aligned and unaligned positions */
      uint8_t aligned[64] FIO_ALIGN(16);
      FIO_MEMCPY(aligned, buf, 64);
      fio_u256 ha = fio_risky256(aligned, 64);
      fio_u256 hb = fio_risky256(buf + off, 64);
      /* These should only match if the data is the same */
      if (off == 0 && fio___u256_cmp(ha, hb)) {
        align_fail = 1; /* offset 0 with same data should match */
      }
      /* For off > 0, data differs so hashes should differ — not an alignment
         test. Instead, copy same data to unaligned position: */
      if (off > 0) {
        uint8_t unaligned_buf[72];
        FIO_MEMCPY(unaligned_buf + off, aligned, 64);
        fio_u256 hu = fio_risky256(unaligned_buf + off, 64);
        if (fio___u256_cmp(ha, hu))
          align_fail = 1;
      }
    }
    if (align_fail) {
      fprintf(stderr, "    1f. Alignment independence: * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr, "    1f. Alignment independence (offsets 0-7): PASS\n");
    }
  }

  /* 1g. Length parameter respected (extra bytes beyond len don't matter) */
  {
    uint8_t buf1[8] = {'a', 'b', 'c', 0, 0, 0, 0, 0};
    uint8_t buf2[8] = {'a', 'b', 'c', 'X', 'Y', 'Z', '!', '?'};
    fio_u256 h1 = fio_risky256(buf1, 3);
    fio_u256 h2 = fio_risky256(buf2, 3);
    if (fio___u256_cmp(h1, h2)) {
      fprintf(stderr, "    1g. Length independence: * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr, "    1g. Length independence: PASS\n");
    }
  }

  return failures;
}

/* *****************************************************************************
2. Avalanche Test (Strict)
***************************************************************************** */

FIO_SFUNC int fio___test_risky256_avalanche(void) {
  int failures = 0;
#if DEBUG
  const int N = 5000; /* reduced for unoptimized builds */
  fprintf(stderr,
          "  * [2] Avalanche Test (strict, 5K inputs x 512 bits)"
          " [DEBUG reduced]\n");
#else
  const int N = 50000;
  fprintf(stderr, "  * [2] Avalanche Test (strict, 50K inputs x 512 bits)\n");
#endif
  const int INPUT_BITS = 512; /* 64 bytes */
  const int OUTPUT_BITS = 256;
  const int OUTPUT_BYTES = OUTPUT_BITS / 8;

  /* Per-output-bit flip counters */
  uint64_t *bit_flips =
      (uint64_t *)FIO_MEM_REALLOC(NULL, 0, sizeof(uint64_t) * OUTPUT_BITS, 0);
  FIO_ASSERT_ALLOC(bit_flips);
  FIO_MEMSET(bit_flips, 0, sizeof(uint64_t) * OUTPUT_BITS);

  uint64_t total_flips = 0;
  uint64_t total_trials = 0;

  for (int n = 0; n < N; ++n) {
    uint8_t input[64];
    fio_rand_bytes(input, 64);
    fio_u256 base = fio_risky256(input, 64);

    for (int bit = 0; bit < INPUT_BITS; ++bit) {
      /* Flip one input bit */
      uint8_t modified[64];
      FIO_MEMCPY(modified, input, 64);
      modified[bit >> 3] ^= (uint8_t)(1U << (bit & 7));

      fio_u256 flipped = fio_risky256(modified, 64);

      /* Count which output bits changed */
      uint8_t diff[32];
      fio___xor_bytes(diff, base.u8, flipped.u8, OUTPUT_BYTES);
      size_t changed = fio___popcount_bytes(diff, OUTPUT_BYTES);
      total_flips += changed;
      ++total_trials;

      /* Track per-bit flips */
      for (int ob = 0; ob < OUTPUT_BITS; ++ob) {
        if (diff[ob >> 3] & (1U << (ob & 7)))
          ++bit_flips[ob];
      }
    }
  }

  double avg_ratio = (double)total_flips / (double)(total_trials * OUTPUT_BITS);
  double avg_pct = avg_ratio * 100.0;

  /* Strict criterion: average must be in [49.5%, 50.5%] */
  int avg_pass = (avg_pct >= 49.5 && avg_pct <= 50.5);

  /* Per-bit criterion: each output bit must flip in [45%, 55%] */
  int per_bit_failures = 0;
  double worst_bit_pct = 50.0;
  int worst_bit_idx = 0;
  uint64_t trials_per_bit = (uint64_t)N * INPUT_BITS;
  for (int ob = 0; ob < OUTPUT_BITS; ++ob) {
    double pct = (double)bit_flips[ob] / (double)trials_per_bit * 100.0;
    if (pct < 45.0 || pct > 55.0)
      ++per_bit_failures;
    double dev = (pct > 50.0) ? (pct - 50.0) : (50.0 - pct);
    double worst_dev = (worst_bit_pct > 50.0) ? (worst_bit_pct - 50.0)
                                              : (50.0 - worst_bit_pct);
    if (dev > worst_dev) {
      worst_bit_pct = pct;
      worst_bit_idx = ob;
    }
  }

  fprintf(stderr,
          "    Average flip: %.4f%% (target: 49.5-50.5%%): %s\n",
          avg_pct,
          avg_pass ? "PASS" : "* FAIL *");
  fprintf(stderr,
          "    Per-bit range [45-55%%]: %d/%d failures, worst bit %d = %.4f%%: "
          "%s\n",
          per_bit_failures,
          OUTPUT_BITS,
          worst_bit_idx,
          worst_bit_pct,
          per_bit_failures == 0 ? "PASS" : "* FAIL *");

  if (!avg_pass)
    ++failures;
  if (per_bit_failures > 0)
    ++failures;

  FIO_MEM_FREE(bit_flips, sizeof(uint64_t) * OUTPUT_BITS);
  return failures;
}

/* *****************************************************************************
3. Collision Resistance (Birthday Bound Probe)
***************************************************************************** */

FIO_SFUNC int fio___test_risky256_collisions(void) {
  int failures = 0;
#if DEBUG
  const int N = 1 << 16; /* 64K hashes — reduced for unoptimized builds */
#else
  const int N = 1 << 20; /* 1M hashes */
#endif
  fprintf(stderr,
          "  * [3] Collision Resistance (%d random hashes)%s\n",
          N,
          (N < (1 << 20)) ? " [DEBUG reduced]" : "");

  /* Allocate hash storage */
  fio_u256 *hashes =
      (fio_u256 *)FIO_MEM_REALLOC(NULL, 0, sizeof(fio_u256) * N, 0);
  FIO_ASSERT_ALLOC(hashes);

  /* Generate random hashes */
  for (int i = 0; i < N; ++i) {
    uint8_t input[32];
    fio_rand_bytes(input, 32);
    hashes[i] = fio_risky256(input, 32);
  }

  /* Check 64-bit collisions (first 8 bytes) using a simple sort approach.
     Birthday bound for 64 bits is ~2^32, so 1M hashes should have ~0. */
  int coll_64 = 0;
  int coll_128 = 0;
  int coll_256 = 0;

  /* Sort by first 64 bits, then scan for duplicates */
  uint64_t *keys64 =
      (uint64_t *)FIO_MEM_REALLOC(NULL, 0, sizeof(uint64_t) * N, 0);
  FIO_ASSERT_ALLOC(keys64);
  for (int i = 0; i < N; ++i)
    keys64[i] = hashes[i].u64[0];

  {
    qsort(keys64, N, sizeof(uint64_t), fio___cmp_u64);
    for (int i = 1; i < N; ++i) {
      if (keys64[i] == keys64[i - 1])
        ++coll_64;
    }
  }

  /* For 128-bit and 256-bit collision checks, sort full hashes */
  {
    qsort(hashes, N, sizeof(fio_u256), fio___cmp_u256);
    for (int i = 1; i < N; ++i) {
      if (!FIO_MEMCMP(&hashes[i], &hashes[i - 1], 32)) {
        ++coll_256;
        ++coll_128; /* 256-bit collision implies 128-bit collision */
      } else if (!FIO_MEMCMP(&hashes[i], &hashes[i - 1], 16)) {
        ++coll_128;
      }
    }
  }

  fprintf(stderr,
          "    64-bit collisions: %d (expected ~0): %s\n",
          coll_64,
          coll_64 <= 1 ? "PASS" : "* FAIL *");
  fprintf(stderr,
          "    128-bit collisions: %d (expected 0): %s\n",
          coll_128,
          coll_128 == 0 ? "PASS" : "* FAIL *");
  fprintf(stderr,
          "    256-bit collisions: %d (expected 0): %s\n",
          coll_256,
          coll_256 == 0 ? "PASS" : "* FAIL *");

  if (coll_64 > 1)
    ++failures; /* Allow 1 due to birthday paradox at 64-bit */
  if (coll_128 > 0)
    ++failures;
  if (coll_256 > 0)
    ++failures;

  FIO_MEM_FREE(keys64, sizeof(uint64_t) * N);
  FIO_MEM_FREE(hashes, sizeof(fio_u256) * N);
  return failures;
}

/* *****************************************************************************
4. Differential Test
***************************************************************************** */

FIO_SFUNC int fio___test_risky256_differential(void) {
  int failures = 0;
#if DEBUG
  const int N = 1000; /* reduced for unoptimized builds */
  fprintf(stderr,
          "  * [4] Differential Test (1K inputs, structured deltas)"
          " [DEBUG reduced]\n");
#else
  const int N = 10000;
  fprintf(stderr,
          "  * [4] Differential Test (10K inputs, structured deltas)\n");
#endif
  const int INPUT_LEN = 64;
  int min_hamming = 256; /* Track minimum Hamming distance in output */

  for (int n = 0; n < N; ++n) {
    uint8_t input[64];
    fio_rand_bytes(input, 64);
    fio_u256 base = fio_risky256(input, INPUT_LEN);

    /* 4a. Single-byte differences at each position */
    for (int pos = 0; pos < INPUT_LEN; ++pos) {
      uint8_t modified[64];
      FIO_MEMCPY(modified, input, 64);
      modified[pos] ^= 0x01; /* flip lowest bit of one byte */
      fio_u256 h = fio_risky256(modified, INPUT_LEN);
      uint8_t diff[32];
      fio___xor_bytes(diff, base.u8, h.u8, 32);
      int hw = (int)fio___popcount_bytes(diff, 32);
      if (hw < min_hamming)
        min_hamming = hw;
    }

    /* 4b. Structured deltas (only on first 100 inputs to save time) */
    if (n < 100) {
      /* All-zeros vs all-ones */
      {
        uint8_t zeros[64], ones[64];
        FIO_MEMSET(zeros, 0, 64);
        FIO_MEMSET(ones, 0xFF, 64);
        fio_u256 hz = fio_risky256(zeros, 64);
        fio_u256 ho = fio_risky256(ones, 64);
        uint8_t diff[32];
        fio___xor_bytes(diff, hz.u8, ho.u8, 32);
        int hw = (int)fio___popcount_bytes(diff, 32);
        if (hw < min_hamming)
          min_hamming = hw;
      }
      /* 0xAA..AA vs 0x55..55 */
      {
        uint8_t aa[64], x55[64];
        FIO_MEMSET(aa, 0xAA, 64);
        FIO_MEMSET(x55, 0x55, 64);
        fio_u256 ha = fio_risky256(aa, 64);
        fio_u256 h5 = fio_risky256(x55, 64);
        uint8_t diff[32];
        fio___xor_bytes(diff, ha.u8, h5.u8, 32);
        int hw = (int)fio___popcount_bytes(diff, 32);
        if (hw < min_hamming)
          min_hamming = hw;
      }
    }
  }

  /* For a good 256-bit hash, minimum Hamming distance should be well above 64
     (25% of 256 bits). A weak hash might produce low-weight differentials. */
  int pass = (min_hamming >= 64);
  fprintf(stderr,
          "    Minimum Hamming distance: %d/256 (threshold >= 64): %s\n",
          min_hamming,
          pass ? "PASS" : "* FAIL *");
  if (!pass)
    ++failures;

  return failures;
}

/* *****************************************************************************
5. HMAC Tests
***************************************************************************** */

FIO_SFUNC int fio___test_risky256_hmac(void) {
  int failures = 0;
  const int N = 1000;
  fprintf(stderr, "  * [5] HMAC Tests (correctness, key/msg sensitivity)\n");

  /* 5a. HMAC is deterministic */
  {
    const char *key = "secret key";
    const char *msg = "hello world";
    fio_u256 h1 = fio_risky256_hmac(key, 10, msg, 11);
    fio_u256 h2 = fio_risky256_hmac(key, 10, msg, 11);
    if (fio___u256_cmp(h1, h2)) {
      fprintf(stderr, "    5a. HMAC deterministic: * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr, "    5a. HMAC deterministic: PASS\n");
    }
  }

  /* 5b. Different keys produce different HMACs */
  {
    const char *msg = "test message";
    int collisions = 0;
    for (int i = 0; i < N; ++i) {
      uint64_t k1 = (uint64_t)i;
      uint64_t k2 = (uint64_t)(i + N);
      fio_u256 h1 = fio_risky256_hmac(&k1, sizeof(k1), msg, 12);
      fio_u256 h2 = fio_risky256_hmac(&k2, sizeof(k2), msg, 12);
      if (!fio___u256_cmp(h1, h2))
        ++collisions;
    }
    if (collisions) {
      fprintf(stderr,
              "    5b. Key sensitivity (%d/%d collisions): * FAIL *\n",
              collisions,
              N);
      ++failures;
    } else {
      fprintf(stderr, "    5b. Key sensitivity (0/%d collisions): PASS\n", N);
    }
  }

  /* 5c. Different messages produce different HMACs */
  {
    const char *key = "fixed key";
    int collisions = 0;
    for (int i = 0; i < N; ++i) {
      uint64_t m1 = (uint64_t)i;
      uint64_t m2 = (uint64_t)(i + N);
      fio_u256 h1 = fio_risky256_hmac(key, 9, &m1, sizeof(m1));
      fio_u256 h2 = fio_risky256_hmac(key, 9, &m2, sizeof(m2));
      if (!fio___u256_cmp(h1, h2))
        ++collisions;
    }
    if (collisions) {
      fprintf(stderr,
              "    5c. Message sensitivity (%d/%d collisions): * FAIL *\n",
              collisions,
              N);
      ++failures;
    } else {
      fprintf(stderr,
              "    5c. Message sensitivity (0/%d collisions): PASS\n",
              N);
    }
  }

  /* 5d. HMAC with key > block_size (64 bytes) — key gets hashed */
  {
    uint8_t long_key[128];
    fio_rand_bytes(long_key, 128);
    const char *msg = "message";
    fio_u256 h1 = fio_risky256_hmac(long_key, 128, msg, 7);
    fio_u256 h2 = fio_risky256_hmac(long_key, 128, msg, 7);
    fio_u256 zero = {{0}};
    int is_zero = !fio___u256_cmp(h1, zero);
    int is_det = !fio___u256_cmp(h1, h2);
    if (is_zero || !is_det) {
      fprintf(stderr,
              "    5d. HMAC long key (>64B): %s * FAIL *\n",
              is_zero ? "zero output" : "not deterministic");
      ++failures;
    } else {
      fprintf(stderr, "    5d. HMAC long key (>64B): PASS\n");
    }
  }

  /* 5e. HMAC with empty key */
  {
    const char *msg = "test";
    fio_u256 h1 = fio_risky256_hmac(NULL, 0, msg, 4);
    fio_u256 h2 = fio_risky256_hmac(NULL, 0, msg, 4);
    fio_u256 zero = {{0}};
    if (!fio___u256_cmp(h1, zero)) {
      fprintf(stderr, "    5e. HMAC empty key: zero output * FAIL *\n");
      ++failures;
    } else if (fio___u256_cmp(h1, h2)) {
      fprintf(stderr, "    5e. HMAC empty key: not deterministic * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr, "    5e. HMAC empty key: PASS\n");
    }
  }

  /* 5f. HMAC with empty message */
  {
    const char *key = "key";
    fio_u256 h1 = fio_risky256_hmac(key, 3, NULL, 0);
    fio_u256 h2 = fio_risky256_hmac(key, 3, NULL, 0);
    fio_u256 zero = {{0}};
    if (!fio___u256_cmp(h1, zero)) {
      fprintf(stderr, "    5f. HMAC empty message: zero output * FAIL *\n");
      ++failures;
    } else if (fio___u256_cmp(h1, h2)) {
      fprintf(stderr,
              "    5f. HMAC empty message: not deterministic * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr, "    5f. HMAC empty message: PASS\n");
    }
  }

  /* 5g. HMAC-256 != HMAC-512 first 256 bits (different squeeze functions) */
  {
    const char *key = "test key";
    const char *msg = "test message";
    fio_u256 h256 = fio_risky256_hmac(key, 8, msg, 12);
    fio_u512 h512 = fio_risky512_hmac(key, 8, msg, 12);
    /* The outer hash inputs differ (inner hash sizes differ: 32 vs 64 bytes),
       so the results should differ. */
    int differ = !!FIO_MEMCMP(h256.u8, h512.u8, 32);
    if (!differ) {
      fprintf(stderr,
              "    5g. HMAC-256 vs HMAC-512 first 256 bits differ: * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr,
              "    5g. HMAC-256 vs HMAC-512 first 256 bits differ: PASS\n");
    }
  }

  /* 5h. HMAC-512 deterministic and non-zero */
  {
    const char *key = "key512";
    const char *msg = "message512";
    fio_u512 h1 = fio_risky512_hmac(key, 6, msg, 10);
    fio_u512 h2 = fio_risky512_hmac(key, 6, msg, 10);
    fio_u512 zero = {{0}};
    if (!fio___u512_cmp(h1, zero)) {
      fprintf(stderr, "    5h. HMAC-512 basic: zero output * FAIL *\n");
      ++failures;
    } else if (fio___u512_cmp(h1, h2)) {
      fprintf(stderr, "    5h. HMAC-512 basic: not deterministic * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr, "    5h. HMAC-512 basic: PASS\n");
    }
  }

  /* 5i. HMAC bit distribution across keys (each bit ~50% set) */
  {
    const char *msg = "fixed message for bit distribution test";
    uint64_t bit_counts[256] = {0};
    for (int i = 0; i < N; ++i) {
      uint64_t key_val = (uint64_t)i;
      fio_u256 h = fio_risky256_hmac(&key_val, sizeof(key_val), msg, 39);
      for (int b = 0; b < 256; ++b) {
        if (h.u8[b >> 3] & (1U << (b & 7)))
          ++bit_counts[b];
      }
    }
    double worst_bias = 0.0;
    int worst_bit = 0;
    for (int b = 0; b < 256; ++b) {
      double pct = (double)bit_counts[b] / (double)N * 100.0;
      double bias = (pct > 50.0) ? (pct - 50.0) : (50.0 - pct);
      if (bias > worst_bias) {
        worst_bias = bias;
        worst_bit = b;
      }
    }
    /* With 1K samples and 256 bits, expect worst bias < 8% (~5-sigma) */
    int pass = (worst_bias < 8.0);
    fprintf(stderr,
            "    5i. HMAC bit distribution: worst bias %.2f%% at bit %d "
            "(threshold < 8%%): %s\n",
            worst_bias,
            worst_bit,
            pass ? "PASS" : "* FAIL *");
    if (!pass)
      ++failures;
  }

  return failures;
}

/* *****************************************************************************
6. Length Extension Resistance Probe
***************************************************************************** */

FIO_SFUNC int fio___test_risky256_length_extension(void) {
  int failures = 0;
  fprintf(stderr, "  * [6] Length Extension Resistance Probe\n");

  /* For input M, compute H = hash(M).
     For input M||padding||M', verify hash(M||padding||M') != H.
     Also verify it's not a simple XOR/add of H with hash(M'). */
  uint8_t m[32];
  fio_rand_bytes(m, 32);
  fio_u256 h_m = fio_risky256(m, 32);

  /* Construct M || 0x80 || zeros || M' */
  uint8_t extended[128];
  FIO_MEMCPY(extended, m, 32);
  FIO_MEMSET(extended + 32, 0, 32); /* padding */
  extended[32] = 0x80;              /* typical padding byte */
  uint8_t m_prime[32];
  fio_rand_bytes(m_prime, 32);
  FIO_MEMCPY(extended + 64, m_prime, 32);

  /* Lengths to test: just M, and M||pad||M' */
  fio_u256 h_ext = fio_risky256(extended, 96);
  fio_u256 h_mp = fio_risky256(m_prime, 32);

  /* h_ext should not equal h_m */
  int ext_eq_m = !fio___u256_cmp(h_ext, h_m);

  /* h_ext should not equal h_m XOR h_mp */
  fio_u256 h_xor;
  for (size_t i = 0; i < 4; ++i)
    h_xor.u64[i] = h_m.u64[i] ^ h_mp.u64[i];
  int ext_eq_xor = !fio___u256_cmp(h_ext, h_xor);

  /* h_ext should not equal h_m + h_mp (wrapping add) */
  fio_u256 h_add;
  for (size_t i = 0; i < 4; ++i)
    h_add.u64[i] = h_m.u64[i] + h_mp.u64[i];
  int ext_eq_add = !fio___u256_cmp(h_ext, h_add);

  int pass = !ext_eq_m && !ext_eq_xor && !ext_eq_add;
  fprintf(stderr,
          "    hash(M||pad||M') != hash(M): %s\n",
          !ext_eq_m ? "PASS" : "* FAIL *");
  fprintf(stderr,
          "    hash(M||pad||M') != hash(M) XOR hash(M'): %s\n",
          !ext_eq_xor ? "PASS" : "* FAIL *");
  fprintf(stderr,
          "    hash(M||pad||M') != hash(M) + hash(M'): %s\n",
          !ext_eq_add ? "PASS" : "* FAIL *");

  if (!pass)
    ++failures;
  return failures;
}

/* *****************************************************************************
7. Known Answer Tests (KATs)
***************************************************************************** */

FIO_SFUNC int fio___test_risky256_kats(void) {
  int failures = 0;
  fprintf(stderr, "  * [7] Known Answer Tests (KATs)\n");
  fprintf(stderr,
          "    Computing reference values (freeze after integration):\n");

  /* 7a. hash("", 0) */
  {
    fio_u256 h256 = fio_risky256("", 0);
    fio_u512 h512 = fio_risky512("", 0);
    fio___print_u256("risky256(\"\", 0)", h256);
    fio___print_u512("risky512(\"\", 0)", h512);
  }

  /* 7b. hash("abc", 3) */
  {
    fio_u256 h256 = fio_risky256("abc", 3);
    fio_u512 h512 = fio_risky512("abc", 3);
    fio___print_u256("risky256(\"abc\", 3)", h256);
    fio___print_u512("risky512(\"abc\", 3)", h512);
  }

  /* 7c. 1024 bytes of sequential 0..255 pattern */
  {
    uint8_t pattern[1024];
    for (size_t i = 0; i < 1024; ++i)
      pattern[i] = (uint8_t)(i & 255);
    fio_u256 h256 = fio_risky256(pattern, 1024);
    fio_u512 h512 = fio_risky512(pattern, 1024);
    fio___print_u256("risky256(seq_0_255, 1024)", h256);
    fio___print_u512("risky512(seq_0_255, 1024)", h512);
  }

  /* 7d. 64KB of 0xAB pattern */
  {
    size_t sz = 65536;
    uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, sz, 0);
    FIO_ASSERT_ALLOC(buf);
    FIO_MEMSET(buf, 0xAB, sz);
    fio_u256 h256 = fio_risky256(buf, sz);
    fio_u512 h512 = fio_risky512(buf, sz);
    fio___print_u256("risky256(0xAB*64K, 64K)", h256);
    fio___print_u512("risky512(0xAB*64K, 64K)", h512);
    FIO_MEM_FREE(buf, sz);
  }

  /* 7e. HMAC KATs */
  {
    const char *key = "HMAC test key";
    const char *msg = "HMAC test message";
    fio_u256 hmac256 = fio_risky256_hmac(key, 13, msg, 17);
    fio_u512 hmac512 = fio_risky512_hmac(key, 13, msg, 17);
    fio___print_u256("risky256_hmac(\"HMAC test key\", \"HMAC test message\")",
                     hmac256);
    fio___print_u512("risky512_hmac(\"HMAC test key\", \"HMAC test message\")",
                     hmac512);
  }

  /* 7f. Verify 256-bit and 512-bit KATs are self-consistent (deterministic) */
  {
    fio_u256 a = fio_risky256("abc", 3);
    fio_u256 b = fio_risky256("abc", 3);
    fio_u512 c = fio_risky512("abc", 3);
    fio_u512 d = fio_risky512("abc", 3);
    if (fio___u256_cmp(a, b) || fio___u512_cmp(c, d)) {
      fprintf(stderr, "    KAT determinism check: * FAIL *\n");
      ++failures;
    } else {
      fprintf(stderr, "    KAT determinism check: PASS\n");
    }
  }

  return failures;
}

/* *****************************************************************************
8. Speed Sanity
***************************************************************************** */

FIO_SFUNC int fio___test_risky256_speed(void) {
  int failures = 0;
  /* Speed tests are informational on CI - not pass/fail */
  fprintf(stderr, "  * [8] Speed Sanity (1MB, informational)\n");

  size_t sz = 1024 * 1024;
  uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, sz, 0);
  FIO_ASSERT_ALLOC(buf);
  FIO_MEMSET(buf, 0xAB, sz);

  /* Warmup */
  fio_u256 h = fio_risky256(buf, sz);
  FIO_COMPILER_GUARD;

  /* Time 256-bit */
  const int ITERS = 100;
  clock_t start = clock();
  for (int i = 0; i < ITERS; ++i) {
    h = fio_risky256(buf, sz);
    buf[0] ^= h.u8[0]; /* prevent dead code elimination */
    FIO_COMPILER_GUARD;
  }
  clock_t end = clock();
  (void)h;

  double elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC;
  double mb_per_sec = ((double)sz * ITERS / (1024.0 * 1024.0)) / elapsed_sec;
  double gb_per_sec = mb_per_sec / 1024.0;

  /* Speed tests are informational on CI - not pass/fail */
  int pass = (gb_per_sec >= 1.0);
  fprintf(stderr,
          "    256-bit: %.0f MB/s (%.2f GB/s): %s\n",
          mb_per_sec,
          gb_per_sec,
          pass ? "PASS" : "INFO (below 1 GB/s threshold)");

  /* Time 512-bit */
  fio_u512 h512 = fio_risky512(buf, sz);
  FIO_COMPILER_GUARD;
  start = clock();
  for (int i = 0; i < ITERS; ++i) {
    h512 = fio_risky512(buf, sz);
    buf[0] ^= h512.u8[0]; /* prevent dead code elimination */
    FIO_COMPILER_GUARD;
  }
  end = clock();
  (void)h512;

  elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC;
  mb_per_sec = ((double)sz * ITERS / (1024.0 * 1024.0)) / elapsed_sec;
  gb_per_sec = mb_per_sec / 1024.0;

  /* Speed tests are informational on CI - not pass/fail */
  int pass512 = (gb_per_sec >= 1.0);
  fprintf(stderr,
          "    512-bit: %.0f MB/s (%.2f GB/s): %s\n",
          mb_per_sec,
          gb_per_sec,
          pass512 ? "PASS" : "INFO (below 1 GB/s threshold)");

  /* Do NOT increment failures for speed tests — performance is informational */
  (void)pass;
  (void)pass512;

  FIO_MEM_FREE(buf, sz);
  return failures;
}

/* *****************************************************************************
9. Absorption Boundary Tests
***************************************************************************** */

FIO_SFUNC int fio___test_risky256_boundaries(void) {
  int failures = 0;
  static const size_t lens[] =
      {0, 1, 63, 64, 65, 127, 128, 129, 191, 192, 193, 255, 256, 257};
  const size_t N = sizeof(lens) / sizeof(lens[0]);
  fprintf(stderr, "  * [9] Absorption Boundary Tests (%zu lengths)\n", N);

  /* sequential fill buffer — large enough for max test length */
  uint8_t buf[512];
  for (size_t i = 0; i < sizeof(buf); ++i)
    buf[i] = (uint8_t)(i & 255);

  /* store hashes for adjacent-length comparison */
  fio_u256 prev_h = {{0}};
  int prev_valid = 0;

  for (size_t t = 0; t < N; ++t) {
    size_t len = lens[t];
    fio_u256 h0 = fio_risky256(buf, len);
    fio_u512 h512 = fio_risky512(buf, len);

    /* 9a. Non-zero (skip len==0 which still produces non-zero from init) */
    {
      fio_u256 zero = {{0}};
      if (!fio___u256_cmp(h0, zero)) {
        fprintf(stderr, "    len=%zu: hash is zero: * FAIL *\n", len);
        ++failures;
      }
    }

    /* 9b. Different from previous length */
    if (prev_valid && !fio___u256_cmp(h0, prev_h)) {
      fprintf(stderr, "    len=%zu == len=%zu: * FAIL *\n", len, lens[t - 1]);
      ++failures;
    }

    /* 9c. 512-bit first half matches 256-bit */
    if (FIO_MEMCMP(&h0, &h512, 32)) {
      fprintf(stderr,
              "    len=%zu: risky512 first 256 bits != risky256: * FAIL *\n",
              len);
      ++failures;
    }

    prev_h = h0;
    prev_valid = 1;
  }

  if (!failures)
    fprintf(stderr,
            "    All %zu boundary lengths: non-zero, "
            "length-distinct, 512==256 prefix: PASS\n",
            N);
  return failures;
}

/* *****************************************************************************
Main Test Entry Point
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, risky256)(void) {
  int total_failures = 0;

  fprintf(stderr,
          "\n========================================================"
          "========================\n");
  fprintf(stderr,
          "  fio_risky256 / fio_risky512 — Comprehensive Quality Tests\n");
  fprintf(stderr,
          "========================================================"
          "========================\n\n");

  total_failures += fio___test_risky256_sanity();
  fprintf(stderr, "\n");
  total_failures += fio___test_risky256_avalanche();
  fprintf(stderr, "\n");
  total_failures += fio___test_risky256_collisions();
  fprintf(stderr, "\n");
  total_failures += fio___test_risky256_differential();
  fprintf(stderr, "\n");
  total_failures += fio___test_risky256_hmac();
  fprintf(stderr, "\n");
  total_failures += fio___test_risky256_length_extension();
  fprintf(stderr, "\n");
  total_failures += fio___test_risky256_kats();
  fprintf(stderr, "\n");
#if !DEBUG
  total_failures += fio___test_risky256_speed();
  fprintf(stderr, "\n");
#else
  fprintf(stderr, "  * [8] Speed Sanity: SKIPPED (DEBUG build)\n\n");
#endif
  total_failures += fio___test_risky256_boundaries();
  fprintf(stderr, "\n");

  fprintf(stderr,
          "========================================================"
          "========================\n");
  if (total_failures == 0) {
    fprintf(stderr, "  ALL TESTS PASSED\n");
  } else {
    fprintf(stderr, "  %d TEST(S) FAILED\n", total_failures);
  }
  fprintf(stderr,
          "========================================================"
          "========================\n\n");

  FIO_ASSERT(!total_failures, "fio_risky256 quality tests failed!");
}

int main(void) {
  fio_rand_reseed();
  FIO_NAME_TEST(stl, risky256)();
  return 0;
}
