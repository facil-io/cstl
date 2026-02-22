/* *****************************************************************************
1MB Avalanche / Bit-Correlation Tests for All Hash Functions

Tests that flipping a single bit at the 750KB mark in a 1MB input causes
at least 25% of the output bits to change (Hamming distance >= total_bits/4).

A good hash should flip ~50% of output bits; < 25% indicates a serious
avalanche failure (hash not consuming all input, or constant output).

Compile: make tests/hash-avalanche
Run:     tests/hash-avalanche
***************************************************************************** */
#include "test-helpers.h"

/* Enable all hash modules */
#define FIO_LOG
#define FIO_RAND
#define FIO_BLAKE2
#define FIO_SHA3
#define FIO_CRYPT
#define FIO_CRC32
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Helpers
***************************************************************************** */

/** Count differing bits between two byte arrays (Hamming distance). */
FIO_SFUNC int fio___avalanche_hamming(const uint8_t *a,
                                      const uint8_t *b,
                                      size_t len) {
  int diff = 0;
  for (size_t i = 0; i < len; ++i)
    diff += fio_popcount(a[i] ^ b[i]);
  return diff;
}

/** Print avalanche result and assert >= 25% threshold. */
FIO_SFUNC void fio___avalanche_report(const char *name,
                                      int diff_bits,
                                      int total_bits) {
  int min_expected = total_bits / 4; /* 25% threshold */
  double pct = (double)diff_bits * 100.0 / (double)total_bits;
  FIO_LOG_INFO("%-16s avalanche: %3d/%3d bits differ (%5.1f%%) %s",
               name,
               diff_bits,
               total_bits,
               pct,
               (diff_bits >= min_expected) ? "PASS" : "FAIL");
  FIO_ASSERT(diff_bits >= min_expected,
             "%s avalanche FAIL: only %d/%d bits differ (%.1f%% < 25%%)",
             name,
             diff_bits,
             total_bits,
             pct);
}

/* *****************************************************************************
1MB Avalanche Test
***************************************************************************** */

FIO_SFUNC void fio___test_hash_avalanche(void) {
  const size_t DATA_LEN = 1048576; /* 1MB */
  const size_t FLIP_BYTE = 768000; /* 750KB mark */

  FIO_LOG_INFO("=== 1MB Avalanche Test (flip bit at 750KB mark) ===");

  /* Allocate 1MB buffer using project macros */
  uint8_t *data = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, DATA_LEN, 0);
  FIO_ASSERT(data, "1MB allocation failed");

  /* Fill with deterministic pseudo-random data */
  for (size_t i = 0; i < DATA_LEN; i++)
    data[i] = (uint8_t)(i * 2654435761UL ^ (i >> 7));

  /* -------------------------------------------------------------------------
   * SHA-1 (20 bytes = 160 bits)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 20;
    uint8_t out1[20], out2[20];

    fio_sha1_s h1 = fio_sha1(data, DATA_LEN);
    FIO_MEMCPY(out1, h1.digest, DIGEST);

    data[FLIP_BYTE] ^= 1;
    fio_sha1_s h2 = fio_sha1(data, DATA_LEN);
    FIO_MEMCPY(out2, h2.digest, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("SHA-1",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * SHA-256 (32 bytes = 256 bits)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 32;
    uint8_t out1[32], out2[32];

    fio_u256 r1 = fio_sha256(data, DATA_LEN);
    FIO_MEMCPY(out1, r1.u8, DIGEST);

    data[FLIP_BYTE] ^= 1;
    fio_u256 r2 = fio_sha256(data, DATA_LEN);
    FIO_MEMCPY(out2, r2.u8, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("SHA-256",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * SHA-512 (64 bytes = 512 bits)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 64;
    uint8_t out1[64], out2[64];

    fio_u512 r1 = fio_sha512(data, DATA_LEN);
    FIO_MEMCPY(out1, r1.u8, DIGEST);

    data[FLIP_BYTE] ^= 1;
    fio_u512 r2 = fio_sha512(data, DATA_LEN);
    FIO_MEMCPY(out2, r2.u8, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("SHA-512",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * SHA3-256 (32 bytes = 256 bits)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 32;
    uint8_t out1[32], out2[32];

    fio_sha3_256(out1, data, DATA_LEN);

    data[FLIP_BYTE] ^= 1;
    fio_sha3_256(out2, data, DATA_LEN);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("SHA3-256",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * SHA3-512 (64 bytes = 512 bits)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 64;
    uint8_t out1[64], out2[64];

    fio_sha3_512(out1, data, DATA_LEN);

    data[FLIP_BYTE] ^= 1;
    fio_sha3_512(out2, data, DATA_LEN);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("SHA3-512",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * BLAKE2b (64 bytes = 512 bits)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 64;
    uint8_t out1[64], out2[64];

    fio_blake2b_s ctx1 = fio_blake2b_init(DIGEST, NULL, 0);
    fio_blake2b_consume(&ctx1, data, DATA_LEN);
    fio_u512 r1 = fio_blake2b_finalize(&ctx1);
    FIO_MEMCPY(out1, r1.u8, DIGEST);

    data[FLIP_BYTE] ^= 1;
    fio_blake2b_s ctx2 = fio_blake2b_init(DIGEST, NULL, 0);
    fio_blake2b_consume(&ctx2, data, DATA_LEN);
    fio_u512 r2 = fio_blake2b_finalize(&ctx2);
    FIO_MEMCPY(out2, r2.u8, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("BLAKE2b",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * BLAKE2s (32 bytes = 256 bits)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 32;
    uint8_t out1[32], out2[32];

    fio_blake2s_s ctx1 = fio_blake2s_init(DIGEST, NULL, 0);
    fio_blake2s_consume(&ctx1, data, DATA_LEN);
    fio_u256 r1 = fio_blake2s_finalize(&ctx1);
    FIO_MEMCPY(out1, r1.u8, DIGEST);

    data[FLIP_BYTE] ^= 1;
    fio_blake2s_s ctx2 = fio_blake2s_init(DIGEST, NULL, 0);
    fio_blake2s_consume(&ctx2, data, DATA_LEN);
    fio_u256 r2 = fio_blake2s_finalize(&ctx2);
    FIO_MEMCPY(out2, r2.u8, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("BLAKE2s",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * RiskyHash-256 (32 bytes = 256 bits)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 32;
    uint8_t out1[32], out2[32];

    fio_u256 r1 = fio_risky256(data, DATA_LEN);
    FIO_MEMCPY(out1, r1.u8, DIGEST);

    data[FLIP_BYTE] ^= 1;
    fio_u256 r2 = fio_risky256(data, DATA_LEN);
    FIO_MEMCPY(out2, r2.u8, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("RiskyHash-256",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * RiskyHash-512 (64 bytes = 512 bits)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 64;
    uint8_t out1[64], out2[64];

    fio_u512 r1 = fio_risky512(data, DATA_LEN);
    FIO_MEMCPY(out1, r1.u8, DIGEST);

    data[FLIP_BYTE] ^= 1;
    fio_u512 r2 = fio_risky512(data, DATA_LEN);
    FIO_MEMCPY(out2, r2.u8, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("RiskyHash-512",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * fio_risky_hash (64-bit = 8 bytes)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 8;
    uint8_t out1[8], out2[8];

    uint64_t h1 = fio_risky_hash(data, DATA_LEN, 0);
    FIO_MEMCPY(out1, &h1, DIGEST);

    data[FLIP_BYTE] ^= 1;
    uint64_t h2 = fio_risky_hash(data, DATA_LEN, 0);
    FIO_MEMCPY(out2, &h2, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("fio_risky_hash",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * fio_stable_hash (64-bit = 8 bytes)
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 8;
    uint8_t out1[8], out2[8];

    uint64_t h1 = fio_stable_hash(data, DATA_LEN, 0);
    FIO_MEMCPY(out1, &h1, DIGEST);

    data[FLIP_BYTE] ^= 1;
    uint64_t h2 = fio_stable_hash(data, DATA_LEN, 0);
    FIO_MEMCPY(out2, &h2, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("fio_stable_hash",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  /* -------------------------------------------------------------------------
   * CRC32 (4 bytes = 32 bits)
   * Note: CRC32 is a checksum, not a cryptographic hash. Its avalanche
   * properties are weaker — we still test it but the 25% bar is lenient.
   * ---------------------------------------------------------------------- */
  {
    const int DIGEST = 4;
    uint8_t out1[4], out2[4];

    uint32_t c1 = fio_crc32(data, DATA_LEN, 0);
    FIO_MEMCPY(out1, &c1, DIGEST);

    data[FLIP_BYTE] ^= 1;
    uint32_t c2 = fio_crc32(data, DATA_LEN, 0);
    FIO_MEMCPY(out2, &c2, DIGEST);
    data[FLIP_BYTE] ^= 1; /* restore */

    fio___avalanche_report("CRC32",
                           fio___avalanche_hamming(out1, out2, DIGEST),
                           DIGEST * 8);
  }

  FIO_MEM_FREE(data, DATA_LEN);
  FIO_LOG_INFO("=== All avalanche tests passed ===");
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fio___test_hash_avalanche();
  return 0;
}
