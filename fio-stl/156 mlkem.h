/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_MLKEM              /* Development inclusion - ignore line */
#define FIO_SHA3               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                              ML-KEM-768 (FIPS 203)
                        Post-Quantum Key Encapsulation Mechanism




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_MLKEM) && !defined(H___FIO_MLKEM___H)
#define H___FIO_MLKEM___H

/* *****************************************************************************
ML-KEM-768 API

ML-KEM (Module-Lattice-Based Key-Encapsulation Mechanism) provides
post-quantum secure key encapsulation with 192-bit security level.

Parameters (ML-KEM-768):
  n = 256, k = 3, q = 3329
  eta1 = 2, eta2 = 2
  d_u = 10, d_v = 4
  Public key:   1184 bytes
  Secret key:   2400 bytes
  Ciphertext:   1088 bytes
  Shared secret:  32 bytes

Note: This implementation has not been audited. Use at your own risk.
***************************************************************************** */

/** ML-KEM-768 constants */
#define FIO_MLKEM768_PUBLICKEYBYTES  1184
#define FIO_MLKEM768_SECRETKEYBYTES  2400
#define FIO_MLKEM768_CIPHERTEXTBYTES 1088
#define FIO_MLKEM768_SSBYTES         32
#define FIO_MLKEM768_SYMBYTES        32

/**
 * Generate ML-KEM-768 keypair.
 *
 * Generates a random keypair using the system CSPRNG.
 * Returns 0 on success, -1 on failure.
 */
SFUNC int fio_mlkem768_keypair(uint8_t pk[1184], uint8_t sk[2400]);

/**
 * Generate ML-KEM-768 keypair from deterministic seed.
 *
 * The coins buffer must be exactly 64 bytes (d || z).
 * Returns 0 on success, -1 on failure.
 */
SFUNC int fio_mlkem768_keypair_derand(uint8_t pk[1184],
                                      uint8_t sk[2400],
                                      const uint8_t coins[64]);

/**
 * Encapsulate: generate ciphertext and shared secret from public key.
 *
 * Uses system CSPRNG for randomness.
 * Returns 0 on success, -1 on failure.
 */
SFUNC int fio_mlkem768_encaps(uint8_t ct[1088],
                              uint8_t ss[32],
                              const uint8_t pk[1184]);

/**
 * Encapsulate with deterministic randomness.
 *
 * The coins buffer must be exactly 32 bytes.
 * Returns 0 on success, -1 on failure.
 */
SFUNC int fio_mlkem768_encaps_derand(uint8_t ct[1088],
                                     uint8_t ss[32],
                                     const uint8_t pk[1184],
                                     const uint8_t coins[32]);

/**
 * Decapsulate: recover shared secret from ciphertext and secret key.
 *
 * Uses implicit rejection: if the ciphertext is invalid, a pseudorandom
 * shared secret is returned (derived from the secret key and ciphertext)
 * rather than an error, preventing chosen-ciphertext attacks.
 *
 * Returns 0 on success (always succeeds for well-formed inputs).
 */
SFUNC int fio_mlkem768_decaps(uint8_t ss[32],
                              const uint8_t ct[1088],
                              const uint8_t sk[2400]);

/* *****************************************************************************
ML-KEM-768 Internal Constants
***************************************************************************** */

#define FIO___MLKEM_N    256
#define FIO___MLKEM_K    3
#define FIO___MLKEM_Q    3329
#define FIO___MLKEM_ETA1 2
#define FIO___MLKEM_ETA2 2
#define FIO___MLKEM_DU   10
#define FIO___MLKEM_DV   4

#define FIO___MLKEM_POLYBYTES              384
#define FIO___MLKEM_POLYVECBYTES           (FIO___MLKEM_K * FIO___MLKEM_POLYBYTES)
#define FIO___MLKEM_POLYCOMPRESSEDBYTES    128
#define FIO___MLKEM_POLYVECCOMPRESSEDBYTES (FIO___MLKEM_K * 320)

#define FIO___MLKEM_SYMBYTES              32
#define FIO___MLKEM_INDCPA_MSGBYTES       32
#define FIO___MLKEM_INDCPA_PUBLICKEYBYTES FIO_MLKEM768_PUBLICKEYBYTES
#define FIO___MLKEM_INDCPA_SECRETKEYBYTES FIO___MLKEM_POLYVECBYTES
#define FIO___MLKEM_INDCPA_BYTES          FIO_MLKEM768_CIPHERTEXTBYTES

/* Montgomery constant: 2^16 mod q */
#define FIO___MLKEM_MONT ((int16_t)-1044)
/* q^{-1} mod 2^16 */
#define FIO___MLKEM_QINV ((int16_t)-3327)

/* *****************************************************************************
ML-KEM-768 Internal Types
***************************************************************************** */

typedef struct {
  int16_t coeffs[FIO___MLKEM_N];
} fio___mlkem_poly;

typedef struct {
  fio___mlkem_poly vec[FIO___MLKEM_K];
} fio___mlkem_polyvec;

/* *****************************************************************************
Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
NTT Zetas Table
***************************************************************************** */

static const int16_t fio___mlkem_zetas[128] = {
    -1044, -758,  -359,  -1517, 1493,  1422,  287,   202,  -171,  622,   1577,
    182,   962,   -1202, -1474, 1468,  573,   -1325, 264,  383,   -829,  1458,
    -1602, -130,  -681,  1017,  732,   608,   -1542, 411,  -205,  -1571, 1223,
    652,   -552,  1015,  -1293, 1491,  -282,  -1544, 516,  -8,    -320,  -666,
    -1618, -1162, 126,   1469,  -853,  -90,   -271,  830,  107,   -1421, -247,
    -951,  -398,  961,   -1508, -725,  448,   -1065, 677,  -1275, -1103, 430,
    555,   843,   -1251, 871,   1550,  105,   422,   587,  177,   -235,  -291,
    -460,  1574,  1653,  -246,  778,   1159,  -147,  -777, 1483,  -602,  1119,
    -1590, 644,   -872,  349,   418,   329,   -156,  -75,  817,   1097,  603,
    610,   1322,  -1285, -1465, 384,   -1215, -136,  1218, -1335, -874,  220,
    -1187, -1659, -1185, -1530, -1278, 794,   -1510, -854, -870,  478,   -108,
    -308,  996,   991,   958,   -1460, 1522,  1628};

/* *****************************************************************************
Reduction Functions
***************************************************************************** */

/**
 * Montgomery reduction: given a 32-bit integer a, compute
 * a * R^{-1} mod q where R = 2^16.
 */
FIO_IFUNC int16_t fio___mlkem_montgomery_reduce(int32_t a) {
  int16_t t;
  t = (int16_t)a * FIO___MLKEM_QINV;
  t = (int16_t)((a - (int32_t)t * FIO___MLKEM_Q) >> 16);
  return t;
}

/**
 * Barrett reduction: reduce a mod q to range [0, q).
 */
FIO_IFUNC int16_t fio___mlkem_barrett_reduce(int16_t a) {
  int16_t t;
  const int16_t v = ((1 << 26) + FIO___MLKEM_Q / 2) / FIO___MLKEM_Q;
  t = (int16_t)(((int32_t)v * a + (1 << 25)) >> 26);
  t = (int16_t)(t * FIO___MLKEM_Q);
  return (int16_t)(a - t);
}

/** Field multiplication via Montgomery: a*b*R^{-1} mod q */
FIO_IFUNC int16_t fio___mlkem_fqmul(int16_t a, int16_t b) {
  return fio___mlkem_montgomery_reduce((int32_t)a * b);
}

/* *****************************************************************************
NTT / Inverse NTT / Base Multiplication
***************************************************************************** */

/** Forward NTT in-place. Input in standard order, output in bit-reversed. */
FIO_SFUNC void fio___mlkem_ntt(int16_t r[256]) {
  unsigned int len, start, j, k;
  int16_t t, zeta;

  k = 1;
  for (len = 128; len >= 2; len >>= 1) {
    for (start = 0; start < 256; start = j + len) {
      zeta = fio___mlkem_zetas[k++];
      for (j = start; j < start + len; j++) {
        t = fio___mlkem_fqmul(zeta, r[j + len]);
        r[j + len] = (int16_t)(r[j] - t);
        r[j] = (int16_t)(r[j] + t);
      }
    }
  }
}

/** Inverse NTT in-place. Input in bit-reversed order, output in standard. */
FIO_SFUNC void fio___mlkem_invntt(int16_t r[256]) {
  unsigned int start, len, j, k;
  int16_t t, zeta;
  const int16_t f = 1441; /* mont^2 / 128 */

  k = 127;
  for (len = 2; len <= 128; len <<= 1) {
    for (start = 0; start < 256; start = j + len) {
      zeta = fio___mlkem_zetas[k--];
      for (j = start; j < start + len; j++) {
        t = r[j];
        r[j] = fio___mlkem_barrett_reduce((int16_t)(t + r[j + len]));
        r[j + len] = (int16_t)(r[j + len] - t);
        r[j + len] = fio___mlkem_fqmul(zeta, r[j + len]);
      }
    }
  }
  for (j = 0; j < 256; j++)
    r[j] = fio___mlkem_fqmul(r[j], f);
}

/**
 * Multiplication of polynomials in Zq[X]/(X^2-zeta).
 * Used for multiplication in NTT domain.
 */
FIO_SFUNC void fio___mlkem_basemul(int16_t r[2],
                                   const int16_t a[2],
                                   const int16_t b[2],
                                   int16_t zeta) {
  r[0] = fio___mlkem_fqmul(a[1], b[1]);
  r[0] = fio___mlkem_fqmul(r[0], zeta);
  r[0] = (int16_t)(r[0] + fio___mlkem_fqmul(a[0], b[0]));
  r[1] = fio___mlkem_fqmul(a[0], b[1]);
  r[1] = (int16_t)(r[1] + fio___mlkem_fqmul(a[1], b[0]));
}

/* *****************************************************************************
Polynomial Operations
***************************************************************************** */

/** Serialize polynomial to bytes (12-bit coefficients packed into bytes). */
FIO_SFUNC void fio___mlkem_poly_tobytes(uint8_t r[FIO___MLKEM_POLYBYTES],
                                        const fio___mlkem_poly *a) {
  unsigned int i;
  uint16_t t0, t1;
  for (i = 0; i < FIO___MLKEM_N / 2; i++) {
    /* Map to positive representative */
    t0 = (uint16_t)a->coeffs[2 * i];
    t0 = (uint16_t)(t0 + ((uint16_t)((int16_t)t0 >> 15) & FIO___MLKEM_Q));
    t1 = (uint16_t)a->coeffs[2 * i + 1];
    t1 = (uint16_t)(t1 + ((uint16_t)((int16_t)t1 >> 15) & FIO___MLKEM_Q));
    r[3 * i + 0] = (uint8_t)(t0 >> 0);
    r[3 * i + 1] = (uint8_t)((t0 >> 8) | (t1 << 4));
    r[3 * i + 2] = (uint8_t)(t1 >> 4);
  }
}

/** Deserialize polynomial from bytes. */
FIO_SFUNC void fio___mlkem_poly_frombytes(
    fio___mlkem_poly *r,
    const uint8_t a[FIO___MLKEM_POLYBYTES]) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_N / 2; i++) {
    r->coeffs[2 * i] = (int16_t)(((uint16_t)(a[3 * i + 0] >> 0) |
                                  ((uint16_t)(a[3 * i + 1]) << 8)) &
                                 0xFFF);
    r->coeffs[2 * i + 1] = (int16_t)(((uint16_t)(a[3 * i + 1] >> 4) |
                                      ((uint16_t)(a[3 * i + 2]) << 4)) &
                                     0xFFF);
  }
}

/**
 * Compress polynomial (d_v = 4 bits).
 * Uses multiply-shift to avoid division by q.
 */
FIO_SFUNC void fio___mlkem_poly_compress(
    uint8_t r[FIO___MLKEM_POLYCOMPRESSEDBYTES],
    const fio___mlkem_poly *a) {
  unsigned int i, j;
  uint8_t t[8];

  for (i = 0; i < FIO___MLKEM_N / 8; i++) {
    for (j = 0; j < 8; j++) {
      /* Map to positive representative */
      int16_t u = a->coeffs[8 * i + j];
      u = (int16_t)(u + ((u >> 15) & FIO___MLKEM_Q));
      /* Compress: round(2^4 / q * u) mod 2^4, via multiply-shift (no division)
       * 80635 ≈ ceil(2^28 / 3329), avoiding KyberSlash timing leak */
      uint64_t d0 = (uint64_t)u << 4;
      d0 += 1665;
      d0 *= 80635;
      d0 >>= 28;
      t[j] = (uint8_t)(d0 & 0xf);
    }
    r[4 * i + 0] = (uint8_t)(t[0] | (t[1] << 4));
    r[4 * i + 1] = (uint8_t)(t[2] | (t[3] << 4));
    r[4 * i + 2] = (uint8_t)(t[4] | (t[5] << 4));
    r[4 * i + 3] = (uint8_t)(t[6] | (t[7] << 4));
  }
}

/** Decompress polynomial (d_v = 4 bits). */
FIO_SFUNC void fio___mlkem_poly_decompress(
    fio___mlkem_poly *r,
    const uint8_t a[FIO___MLKEM_POLYCOMPRESSEDBYTES]) {
  unsigned int i;

  for (i = 0; i < FIO___MLKEM_N / 2; i++) {
    r->coeffs[2 * i + 0] =
        (int16_t)((((uint16_t)(a[i] & 15) * FIO___MLKEM_Q) + 8) >> 4);
    r->coeffs[2 * i + 1] =
        (int16_t)((((uint16_t)(a[i] >> 4) * FIO___MLKEM_Q) + 8) >> 4);
  }
}

/**
 * Convert message bytes to polynomial.
 * Each bit of the 32-byte message maps to a coefficient: 0 or q/2.
 * Uses constant-time cmov_int16.
 */
FIO_SFUNC void fio___mlkem_poly_frommsg(
    fio___mlkem_poly *r,
    const uint8_t msg[FIO___MLKEM_INDCPA_MSGBYTES]) {
  unsigned int i, j;
  int16_t mask;

  for (i = 0; i < FIO___MLKEM_N / 8; i++) {
    for (j = 0; j < 8; j++) {
      mask = (int16_t) - (int16_t)((msg[i] >> j) & 1);
      r->coeffs[8 * i + j] =
          (int16_t)(mask & (int16_t)((FIO___MLKEM_Q + 1) / 2));
    }
  }
}

/**
 * Convert polynomial to message bytes.
 * Uses multiply-shift to avoid division by q.
 */
FIO_SFUNC void fio___mlkem_poly_tomsg(uint8_t msg[FIO___MLKEM_INDCPA_MSGBYTES],
                                      const fio___mlkem_poly *a) {
  unsigned int i, j;
  uint32_t t;

  for (i = 0; i < FIO___MLKEM_N / 8; i++) {
    msg[i] = 0;
    for (j = 0; j < 8; j++) {
      int16_t u = a->coeffs[8 * i + j];
      u = (int16_t)(u + ((u >> 15) & FIO___MLKEM_Q));
      /* Compress to 1 bit: round(2/q * u) mod 2 */
      t = (uint32_t)u;
      t <<= 1;
      t += 1665;
      t *= 80635;
      t >>= 28;
      t &= 1;
      msg[i] = (uint8_t)(msg[i] | (t << j));
    }
  }
}

/** CBD eta=2 sampling from uniform bytes. */
FIO_SFUNC void fio___mlkem_cbd2(fio___mlkem_poly *r,
                                const uint8_t buf[2 * FIO___MLKEM_N / 4]) {
  unsigned int i, j;
  uint32_t t, d;
  int16_t a, b;

  for (i = 0; i < FIO___MLKEM_N / 8; i++) {
    t = fio_buf2u32_le(buf + 4 * i);
    d = t & 0x55555555u;
    d += (t >> 1) & 0x55555555u;
    for (j = 0; j < 8; j++) {
      a = (int16_t)((d >> (4 * j + 0)) & 0x3);
      b = (int16_t)((d >> (4 * j + 2)) & 0x3);
      r->coeffs[8 * i + j] = (int16_t)(a - b);
    }
  }
}

/** Sample noise polynomial using SHAKE256 PRF (eta1 = 2 for ML-KEM-768). */
FIO_SFUNC void fio___mlkem_poly_getnoise_eta1(
    fio___mlkem_poly *r,
    const uint8_t seed[FIO___MLKEM_SYMBYTES],
    uint8_t nonce) {
  uint8_t buf[FIO___MLKEM_ETA1 * FIO___MLKEM_N / 4]; /* 128 bytes */
  uint8_t extseed[FIO___MLKEM_SYMBYTES + 1];
  FIO_MEMCPY(extseed, seed, FIO___MLKEM_SYMBYTES);
  extseed[FIO___MLKEM_SYMBYTES] = nonce;
  fio_shake256(buf, sizeof(buf), extseed, sizeof(extseed));
  fio___mlkem_cbd2(r, buf);
}

/** Sample noise polynomial using SHAKE256 PRF (eta2 = 2 for ML-KEM-768). */
FIO_SFUNC void fio___mlkem_poly_getnoise_eta2(
    fio___mlkem_poly *r,
    const uint8_t seed[FIO___MLKEM_SYMBYTES],
    uint8_t nonce) {
  uint8_t buf[FIO___MLKEM_ETA2 * FIO___MLKEM_N / 4]; /* 128 bytes */
  uint8_t extseed[FIO___MLKEM_SYMBYTES + 1];
  FIO_MEMCPY(extseed, seed, FIO___MLKEM_SYMBYTES);
  extseed[FIO___MLKEM_SYMBYTES] = nonce;
  /* PRF(s, b) = SHAKE256(s || b, 64*eta) */
  fio_shake256(buf, sizeof(buf), extseed, sizeof(extseed));
  fio___mlkem_cbd2(r, buf);
}

/** Apply forward NTT to polynomial. */
FIO_SFUNC void fio___mlkem_poly_ntt(fio___mlkem_poly *r) {
  fio___mlkem_ntt(r->coeffs);
}

/** Apply inverse NTT to polynomial. */
FIO_SFUNC void fio___mlkem_poly_invntt_tomont(fio___mlkem_poly *r) {
  fio___mlkem_invntt(r->coeffs);
}

/** Pointwise multiplication of polynomials in NTT domain. */
FIO_SFUNC void fio___mlkem_poly_basemul_montgomery(fio___mlkem_poly *r,
                                                   const fio___mlkem_poly *a,
                                                   const fio___mlkem_poly *b) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_N / 4; i++) {
    fio___mlkem_basemul(&r->coeffs[4 * i],
                        &a->coeffs[4 * i],
                        &b->coeffs[4 * i],
                        fio___mlkem_zetas[64 + i]);
    fio___mlkem_basemul(&r->coeffs[4 * i + 2],
                        &a->coeffs[4 * i + 2],
                        &b->coeffs[4 * i + 2],
                        (int16_t)-fio___mlkem_zetas[64 + i]);
  }
}

/** Convert polynomial to Montgomery domain. */
FIO_SFUNC void fio___mlkem_poly_tomont(fio___mlkem_poly *r) {
  unsigned int i;
  const int16_t f = (int16_t)((1ULL << 32) % FIO___MLKEM_Q);
  for (i = 0; i < FIO___MLKEM_N; i++)
    r->coeffs[i] = fio___mlkem_montgomery_reduce((int32_t)r->coeffs[i] * f);
}

/** Reduce all coefficients to canonical range. */
FIO_SFUNC void fio___mlkem_poly_reduce(fio___mlkem_poly *r) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_N; i++)
    r->coeffs[i] = fio___mlkem_barrett_reduce(r->coeffs[i]);
}

/** Add two polynomials. */
FIO_SFUNC void fio___mlkem_poly_add(fio___mlkem_poly *r,
                                    const fio___mlkem_poly *a,
                                    const fio___mlkem_poly *b) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_N; i++)
    r->coeffs[i] = (int16_t)(a->coeffs[i] + b->coeffs[i]);
}

/** Subtract two polynomials: r = a - b. */
FIO_SFUNC void fio___mlkem_poly_sub(fio___mlkem_poly *r,
                                    const fio___mlkem_poly *a,
                                    const fio___mlkem_poly *b) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_N; i++)
    r->coeffs[i] = (int16_t)(a->coeffs[i] - b->coeffs[i]);
}

/* *****************************************************************************
Polyvec Operations
***************************************************************************** */

/**
 * Compress polyvec (d_u = 10 bits per coefficient).
 * Uses multiply-shift to avoid division by q.
 */
FIO_SFUNC void fio___mlkem_polyvec_compress(
    uint8_t r[FIO___MLKEM_POLYVECCOMPRESSEDBYTES],
    const fio___mlkem_polyvec *a) {
  unsigned int i, j, k;
  uint16_t t[4];

  for (i = 0; i < FIO___MLKEM_K; i++) {
    for (j = 0; j < FIO___MLKEM_N / 4; j++) {
      for (k = 0; k < 4; k++) {
        int16_t u = a->vec[i].coeffs[4 * j + k];
        u = (int16_t)(u + ((u >> 15) & FIO___MLKEM_Q));
        /* Compress: round(2^10 / q * u) mod 2^10, via multiply-shift
         * 1290167 ≈ ceil(2^32 / 3329), avoiding KyberSlash timing leak */
        uint64_t d0 = (uint64_t)((uint16_t)u) << 10;
        d0 += 1665;
        d0 *= 1290167;
        d0 >>= 32;
        t[k] = (uint16_t)(d0 & 0x3ff);
      }
      r[0] = (uint8_t)(t[0] >> 0);
      r[1] = (uint8_t)((t[0] >> 8) | (t[1] << 2));
      r[2] = (uint8_t)((t[1] >> 6) | (t[2] << 4));
      r[3] = (uint8_t)((t[2] >> 4) | (t[3] << 6));
      r[4] = (uint8_t)(t[3] >> 2);
      r += 5;
    }
  }
}

/** Decompress polyvec (d_u = 10 bits per coefficient). */
FIO_SFUNC void fio___mlkem_polyvec_decompress(
    fio___mlkem_polyvec *r,
    const uint8_t a[FIO___MLKEM_POLYVECCOMPRESSEDBYTES]) {
  unsigned int i, j;
  uint16_t t[4];

  for (i = 0; i < FIO___MLKEM_K; i++) {
    for (j = 0; j < FIO___MLKEM_N / 4; j++) {
      t[0] = (uint16_t)(((uint16_t)(a[0]) >> 0) | ((uint16_t)(a[1]) << 8));
      t[1] = (uint16_t)(((uint16_t)(a[1]) >> 2) | ((uint16_t)(a[2]) << 6));
      t[2] = (uint16_t)(((uint16_t)(a[2]) >> 4) | ((uint16_t)(a[3]) << 4));
      t[3] = (uint16_t)(((uint16_t)(a[3]) >> 6) | ((uint16_t)(a[4]) << 2));
      a += 5;
      r->vec[i].coeffs[4 * j + 0] =
          (int16_t)(((uint32_t)(t[0] & 0x3FF) * FIO___MLKEM_Q + 512) >> 10);
      r->vec[i].coeffs[4 * j + 1] =
          (int16_t)(((uint32_t)(t[1] & 0x3FF) * FIO___MLKEM_Q + 512) >> 10);
      r->vec[i].coeffs[4 * j + 2] =
          (int16_t)(((uint32_t)(t[2] & 0x3FF) * FIO___MLKEM_Q + 512) >> 10);
      r->vec[i].coeffs[4 * j + 3] =
          (int16_t)(((uint32_t)(t[3] & 0x3FF) * FIO___MLKEM_Q + 512) >> 10);
    }
  }
}

/** Serialize polyvec to bytes. */
FIO_SFUNC void fio___mlkem_polyvec_tobytes(uint8_t r[FIO___MLKEM_POLYVECBYTES],
                                           const fio___mlkem_polyvec *a) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_tobytes(r + i * FIO___MLKEM_POLYBYTES, &a->vec[i]);
}

/** Deserialize polyvec from bytes. */
FIO_SFUNC void fio___mlkem_polyvec_frombytes(
    fio___mlkem_polyvec *r,
    const uint8_t a[FIO___MLKEM_POLYVECBYTES]) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_frombytes(&r->vec[i], a + i * FIO___MLKEM_POLYBYTES);
}

/** Apply NTT to all polynomials in polyvec. */
FIO_SFUNC void fio___mlkem_polyvec_ntt(fio___mlkem_polyvec *r) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_ntt(&r->vec[i]);
}

/** Apply inverse NTT to all polynomials in polyvec. */
FIO_SFUNC void fio___mlkem_polyvec_invntt_tomont(fio___mlkem_polyvec *r) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_invntt_tomont(&r->vec[i]);
}

/** Pointwise multiply-accumulate: r = sum(a[i] * b[i]). */
FIO_SFUNC void fio___mlkem_polyvec_basemul_acc_montgomery(
    fio___mlkem_poly *r,
    const fio___mlkem_polyvec *a,
    const fio___mlkem_polyvec *b) {
  unsigned int i;
  fio___mlkem_poly t;

  fio___mlkem_poly_basemul_montgomery(r, &a->vec[0], &b->vec[0]);
  for (i = 1; i < FIO___MLKEM_K; i++) {
    fio___mlkem_poly_basemul_montgomery(&t, &a->vec[i], &b->vec[i]);
    fio___mlkem_poly_add(r, r, &t);
  }
  fio___mlkem_poly_reduce(r);
}

/** Reduce all coefficients in polyvec. */
FIO_SFUNC void fio___mlkem_polyvec_reduce(fio___mlkem_polyvec *r) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_reduce(&r->vec[i]);
}

/** Add two polyvecs. */
FIO_SFUNC void fio___mlkem_polyvec_add(fio___mlkem_polyvec *r,
                                       const fio___mlkem_polyvec *a,
                                       const fio___mlkem_polyvec *b) {
  unsigned int i;
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}

/* *****************************************************************************
Matrix Generation (Rejection Sampling via SHAKE128 XOF)
***************************************************************************** */

/**
 * Parse uniform random bytes from SHAKE128 stream into polynomial coefficients.
 * Rejection sampling: accept 12-bit values < q.
 */
FIO_SFUNC unsigned int fio___mlkem_rej_uniform(int16_t *r,
                                               unsigned int len,
                                               const uint8_t *buf,
                                               unsigned int buflen) {
  unsigned int ctr, pos;
  uint16_t val0, val1, tmp;

  ctr = pos = 0;
  while (ctr < len && pos + 3 <= buflen) {
    val0 = (uint16_t)(((uint16_t)(buf[pos]) | ((uint16_t)(buf[pos + 1]) << 8)) &
                      0xFFF);
    val1 = (uint16_t)(((uint16_t)(buf[pos + 1] >> 4) |
                       ((uint16_t)(buf[pos + 2]) << 4)) &
                      0xFFF);
    pos += 3;

    tmp = r[ctr];
    r[ctr] = (int16_t)val0;
    ctr += (int)(val0 < FIO___MLKEM_Q);
    r[ctr] = (int16_t)val1;
    ctr += (int)((unsigned)(ctr < len) & (unsigned)(val1 < FIO___MLKEM_Q));
    r[ctr] = tmp;
  }
  return ctr;
}

#define FIO___MLKEM_GEN_MATRIX_NBLOCKS                                         \
  ((12 * FIO___MLKEM_N / 8 * (1 << 12) / FIO___MLKEM_Q + 168) / 168)

/**
 * Generate matrix A (or A^T) from seed rho using SHAKE128.
 * If transposed != 0, generate A^T instead.
 */
FIO_IFUNC void fio___mlkem_gen_matrix(fio___mlkem_polyvec *a,
                                      const uint8_t seed[FIO___MLKEM_SYMBYTES],
                                      int transposed) {
  unsigned int ctr, i, j;
  unsigned int buflen;
  uint8_t buf[FIO___MLKEM_GEN_MATRIX_NBLOCKS * 168 + 2];
  fio_sha3_s state;

  for (i = 0; i < FIO___MLKEM_K; i++) {
    for (j = 0; j < FIO___MLKEM_K; j++) {
      uint8_t extseed[FIO___MLKEM_SYMBYTES + 2];
      FIO_MEMCPY(extseed, seed, FIO___MLKEM_SYMBYTES);
      if (transposed) {
        extseed[FIO___MLKEM_SYMBYTES] = (uint8_t)i;
        extseed[FIO___MLKEM_SYMBYTES + 1] = (uint8_t)j;
      } else {
        extseed[FIO___MLKEM_SYMBYTES] = (uint8_t)j;
        extseed[FIO___MLKEM_SYMBYTES + 1] = (uint8_t)i;
      }

      state = fio_shake128_init();
      fio_sha3_consume(&state, extseed, sizeof(extseed));

      buflen = FIO___MLKEM_GEN_MATRIX_NBLOCKS * 168;
      fio_shake_squeeze(&state, buf, buflen);

      ctr = fio___mlkem_rej_uniform(a[i].vec[j].coeffs,
                                    FIO___MLKEM_N,
                                    buf,
                                    buflen);

      while (ctr < FIO___MLKEM_N) {
        fio_shake_squeeze(&state, buf, 168);
        ctr += fio___mlkem_rej_uniform(a[i].vec[j].coeffs + ctr,
                                       FIO___MLKEM_N - ctr,
                                       buf,
                                       168);
      }
    }
  }
}

/* *****************************************************************************
Constant-Time Utilities
***************************************************************************** */

/**
 * Constant-time comparison of two buffers.
 * Returns 0 if equal, 1 if different.
 */
FIO_SFUNC uint8_t fio___mlkem_verify(const uint8_t *a,
                                     const uint8_t *b,
                                     size_t len) {
  size_t i;
  uint8_t r = 0;
  for (i = 0; i < len; i++)
    r |= a[i] ^ b[i];
  /* Collapse to 0 or 1 */
  r = (uint8_t)((-((int64_t)r)) >> 63);
  return r;
}

/**
 * Constant-time conditional move.
 * If b != 0, copy src to dst. Otherwise do nothing.
 * Uses asm barrier to prevent compiler from introducing branches.
 */
FIO_SFUNC void fio___mlkem_cmov(uint8_t *restrict dst,
                                const uint8_t *restrict src,
                                size_t len,
                                uint8_t b) {
  size_t i;
  b = (uint8_t)(-b); /* 0x00 or 0xFF */
#if defined(__GNUC__) || defined(__clang__)
  __asm__ __volatile__("" : "+r"(b) : : "memory");
#endif
  for (i = 0; i < len; i++)
    dst[i] ^= b & (dst[i] ^ src[i]);
}

/**
 * Constant-time conditional move for int16_t arrays.
 * If b != 0, copy v to r. Otherwise do nothing.
 */
FIO_SFUNC void fio___mlkem_cmov_int16(int16_t *r, int16_t v, uint16_t b) {
  b = (uint16_t)(-b); /* 0x0000 or 0xFFFF */
#if defined(__GNUC__) || defined(__clang__)
  __asm__ __volatile__("" : "+r"(b) : : "memory");
#endif
  *r ^= (int16_t)(b & ((*r) ^ v));
}

/* *****************************************************************************
IND-CPA PKE (Internal)
***************************************************************************** */

/**
 * IND-CPA key generation (deterministic).
 * Input: coins = 32 bytes (d from FIPS 203).
 * Output: pk (1184 bytes), sk (1152 bytes = polyvec in NTT domain).
 */
FIO_SFUNC void fio___mlkem_indcpa_keypair_derand(
    uint8_t pk[FIO___MLKEM_INDCPA_PUBLICKEYBYTES],
    uint8_t sk[FIO___MLKEM_INDCPA_SECRETKEYBYTES],
    const uint8_t coins[FIO___MLKEM_SYMBYTES]) {
  unsigned int i;
  uint8_t buf[2 * FIO___MLKEM_SYMBYTES];
  const uint8_t *publicseed = buf;
  const uint8_t *noiseseed = buf + FIO___MLKEM_SYMBYTES;
  fio___mlkem_polyvec a[FIO___MLKEM_K], e, pkpv, skpv;
  uint8_t nonce = 0;

  /* G(d || k) = (rho, sigma) — FIPS 203 requires appending k as a byte */
  {
    uint8_t gbuf[FIO___MLKEM_SYMBYTES + 1];
    FIO_MEMCPY(gbuf, coins, FIO___MLKEM_SYMBYTES);
    gbuf[FIO___MLKEM_SYMBYTES] = FIO___MLKEM_K;
    fio_sha3_512(buf, gbuf, sizeof(gbuf));
  }

  fio___mlkem_gen_matrix(a, publicseed, 0);

  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_getnoise_eta1(&skpv.vec[i], noiseseed, nonce++);
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_getnoise_eta1(&e.vec[i], noiseseed, nonce++);

  fio___mlkem_polyvec_ntt(&skpv);
  fio___mlkem_polyvec_ntt(&e);

  /* Compute t = A*s + e */
  for (i = 0; i < FIO___MLKEM_K; i++) {
    fio___mlkem_polyvec_basemul_acc_montgomery(&pkpv.vec[i], &a[i], &skpv);
    fio___mlkem_poly_tomont(&pkpv.vec[i]);
  }

  fio___mlkem_polyvec_add(&pkpv, &pkpv, &e);
  fio___mlkem_polyvec_reduce(&pkpv);

  /* Pack secret key (NTT domain) — reduce first for 12-bit packing */
  fio___mlkem_polyvec_reduce(&skpv);
  fio___mlkem_polyvec_tobytes(sk, &skpv);
  /* Pack public key: t || rho */
  fio___mlkem_polyvec_tobytes(pk, &pkpv);
  FIO_MEMCPY(pk + FIO___MLKEM_POLYVECBYTES, publicseed, FIO___MLKEM_SYMBYTES);

  /* Zero sensitive stack data */
  FIO_MEMSET(buf, 0, sizeof(buf));
  FIO_MEMSET(&skpv, 0, sizeof(skpv));
  FIO_MEMSET(&e, 0, sizeof(e));
}

/**
 * IND-CPA encryption.
 * Input: msg (32 bytes), pk (1184 bytes), coins (32 bytes of randomness).
 * Output: ct (1088 bytes).
 */
FIO_SFUNC void fio___mlkem_indcpa_enc(
    uint8_t ct[FIO___MLKEM_INDCPA_BYTES],
    const uint8_t msg[FIO___MLKEM_INDCPA_MSGBYTES],
    const uint8_t pk[FIO___MLKEM_INDCPA_PUBLICKEYBYTES],
    const uint8_t coins[FIO___MLKEM_SYMBYTES]) {
  unsigned int i;
  fio___mlkem_polyvec sp, pkpv, ep, at[FIO___MLKEM_K], b;
  fio___mlkem_poly v, k, epp;
  uint8_t seed[FIO___MLKEM_SYMBYTES];
  uint8_t nonce = 0;

  /* Unpack public key */
  fio___mlkem_polyvec_frombytes(&pkpv, pk);
  FIO_MEMCPY(seed, pk + FIO___MLKEM_POLYVECBYTES, FIO___MLKEM_SYMBYTES);

  /* Generate A^T */
  fio___mlkem_gen_matrix(at, seed, 1);

  /* Sample r (eta1), e1 (eta2), e2 (eta2) */
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_getnoise_eta1(&sp.vec[i], coins, nonce++);
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_poly_getnoise_eta2(&ep.vec[i], coins, nonce++);
  fio___mlkem_poly_getnoise_eta2(&epp, coins, nonce++);

  fio___mlkem_polyvec_ntt(&sp);

  /* Compute u = A^T * r + e1 */
  for (i = 0; i < FIO___MLKEM_K; i++)
    fio___mlkem_polyvec_basemul_acc_montgomery(&b.vec[i], &at[i], &sp);

  fio___mlkem_polyvec_invntt_tomont(&b);
  fio___mlkem_polyvec_add(&b, &b, &ep);
  fio___mlkem_polyvec_reduce(&b);

  /* Compute v = t^T * r + e2 + Decompress(Decode(m)) */
  fio___mlkem_polyvec_basemul_acc_montgomery(&v, &pkpv, &sp);
  fio___mlkem_poly_invntt_tomont(&v);

  fio___mlkem_poly_frommsg(&k, msg);
  fio___mlkem_poly_add(&v, &v, &epp);
  fio___mlkem_poly_add(&v, &v, &k);
  fio___mlkem_poly_reduce(&v);

  /* Pack ciphertext: Compress(u) || Compress(v) */
  fio___mlkem_polyvec_compress(ct, &b);
  fio___mlkem_poly_compress(ct + FIO___MLKEM_POLYVECCOMPRESSEDBYTES, &v);

  /* Zero sensitive stack data */
  FIO_MEMSET(&sp, 0, sizeof(sp));
  FIO_MEMSET(&k, 0, sizeof(k));
}

/**
 * IND-CPA decryption.
 * Input: ct (1088 bytes), sk (1152 bytes).
 * Output: msg (32 bytes).
 */
FIO_SFUNC void fio___mlkem_indcpa_dec(
    uint8_t msg[FIO___MLKEM_INDCPA_MSGBYTES],
    const uint8_t ct[FIO___MLKEM_INDCPA_BYTES],
    const uint8_t sk[FIO___MLKEM_INDCPA_SECRETKEYBYTES]) {
  fio___mlkem_polyvec b, skpv;
  fio___mlkem_poly v, mp;

  /* Unpack ciphertext */
  fio___mlkem_polyvec_decompress(&b, ct);
  fio___mlkem_poly_decompress(&v, ct + FIO___MLKEM_POLYVECCOMPRESSEDBYTES);

  /* Unpack secret key */
  fio___mlkem_polyvec_frombytes(&skpv, sk);

  fio___mlkem_polyvec_ntt(&b);

  /* Compute m = v - s^T * u */
  fio___mlkem_polyvec_basemul_acc_montgomery(&mp, &skpv, &b);
  fio___mlkem_poly_invntt_tomont(&mp);

  fio___mlkem_poly_sub(&mp, &v, &mp);
  fio___mlkem_poly_reduce(&mp);

  fio___mlkem_poly_tomsg(msg, &mp);
}

/* *****************************************************************************
ML-KEM-768 KEM (FO Transform — FIPS 203)
***************************************************************************** */

/**
 * ML-KEM-768 keypair generation (deterministic).
 *
 * coins: 64 bytes = d (32 bytes) || z (32 bytes)
 * where d is the seed for IND-CPA keygen and z is the implicit rejection seed.
 */
SFUNC int fio_mlkem768_keypair_derand(uint8_t pk[1184],
                                      uint8_t sk[2400],
                                      const uint8_t coins[64]) {
  if (!pk || !sk || !coins)
    return -1;

  /* IND-CPA keypair from d */
  fio___mlkem_indcpa_keypair_derand(pk, sk, coins);

  /* sk = sk_cpa || pk || H(pk) || z */
  /* sk_cpa is already at sk[0..1151] */
  FIO_MEMCPY(sk + FIO___MLKEM_INDCPA_SECRETKEYBYTES,
             pk,
             FIO___MLKEM_INDCPA_PUBLICKEYBYTES);
  /* H(pk) = SHA3-256(pk) */
  fio_sha3_256(sk + FIO___MLKEM_INDCPA_SECRETKEYBYTES +
                   FIO___MLKEM_INDCPA_PUBLICKEYBYTES,
               pk,
               FIO___MLKEM_INDCPA_PUBLICKEYBYTES);
  /* z (implicit rejection seed) */
  FIO_MEMCPY(sk + FIO___MLKEM_INDCPA_SECRETKEYBYTES +
                 FIO___MLKEM_INDCPA_PUBLICKEYBYTES + FIO___MLKEM_SYMBYTES,
             coins + FIO___MLKEM_SYMBYTES,
             FIO___MLKEM_SYMBYTES);

  return 0;
}

/** ML-KEM-768 keypair generation (random). */
SFUNC int fio_mlkem768_keypair(uint8_t pk[1184], uint8_t sk[2400]) {
  uint8_t coins[64];
  if (!pk || !sk)
    return -1;
  fio_rand_bytes(coins, 64);
  int r = fio_mlkem768_keypair_derand(pk, sk, coins);
  FIO_MEMSET(coins, 0, sizeof(coins));
  return r;
}

/**
 * ML-KEM-768 encapsulation (deterministic).
 *
 * coins: 32 bytes of randomness (m in FIPS 203).
 */
SFUNC int fio_mlkem768_encaps_derand(uint8_t ct[1088],
                                     uint8_t ss[32],
                                     const uint8_t pk[1184],
                                     const uint8_t coins[32]) {
  uint8_t buf[2 * FIO___MLKEM_SYMBYTES];
  uint8_t kr[2 * FIO___MLKEM_SYMBYTES]; /* (K, r) */

  if (!ct || !ss || !pk || !coins)
    return -1;

  /* buf = m || H(pk) */
  FIO_MEMCPY(buf, coins, FIO___MLKEM_SYMBYTES);
  fio_sha3_256(buf + FIO___MLKEM_SYMBYTES,
               pk,
               FIO___MLKEM_INDCPA_PUBLICKEYBYTES);

  /* (K, r) = G(m || H(pk)) */
  fio_sha3_512(kr, buf, sizeof(buf));

  /* c = Enc(pk, m; r) */
  fio___mlkem_indcpa_enc(ct, buf, pk, kr + FIO___MLKEM_SYMBYTES);

  /* K = KDF(K || H(c)) — but FIPS 203 just uses K directly */
  FIO_MEMCPY(ss, kr, FIO___MLKEM_SYMBYTES);

  /* Zero sensitive stack data */
  FIO_MEMSET(buf, 0, sizeof(buf));
  FIO_MEMSET(kr, 0, sizeof(kr));

  return 0;
}

/** ML-KEM-768 encapsulation (random). */
SFUNC int fio_mlkem768_encaps(uint8_t ct[1088],
                              uint8_t ss[32],
                              const uint8_t pk[1184]) {
  uint8_t coins[FIO___MLKEM_SYMBYTES];
  if (!ct || !ss || !pk)
    return -1;
  fio_rand_bytes(coins, FIO___MLKEM_SYMBYTES);
  int r = fio_mlkem768_encaps_derand(ct, ss, pk, coins);
  FIO_MEMSET(coins, 0, sizeof(coins));
  return r;
}

/**
 * ML-KEM-768 decapsulation.
 *
 * Uses implicit rejection: if ciphertext verification fails, returns a
 * pseudorandom shared secret derived from z and the ciphertext, preventing
 * chosen-ciphertext attacks.
 */
SFUNC int fio_mlkem768_decaps(uint8_t ss[32],
                              const uint8_t ct[1088],
                              const uint8_t sk[2400]) {
  uint8_t buf[2 * FIO___MLKEM_SYMBYTES];
  uint8_t kr[2 * FIO___MLKEM_SYMBYTES];
  uint8_t cmp[FIO___MLKEM_INDCPA_BYTES];
  const uint8_t *pk;
  const uint8_t *hpk;
  const uint8_t *z;
  uint8_t fail;

  if (!ss || !ct || !sk)
    return -1;

  pk = sk + FIO___MLKEM_INDCPA_SECRETKEYBYTES;
  hpk = pk + FIO___MLKEM_INDCPA_PUBLICKEYBYTES;
  z = hpk + FIO___MLKEM_SYMBYTES;

  /* m' = Dec(sk_cpa, ct) */
  fio___mlkem_indcpa_dec(buf, ct, sk);

  /* buf = m' || H(pk) */
  FIO_MEMCPY(buf + FIO___MLKEM_SYMBYTES, hpk, FIO___MLKEM_SYMBYTES);

  /* (K', r') = G(m' || H(pk)) */
  fio_sha3_512(kr, buf, sizeof(buf));

  /* c' = Enc(pk, m'; r') */
  fio___mlkem_indcpa_enc(cmp, buf, pk, kr + FIO___MLKEM_SYMBYTES);

  /* Constant-time comparison: ct == c' ? */
  fail = fio___mlkem_verify(ct, cmp, FIO___MLKEM_INDCPA_BYTES);

  /* Implicit rejection: if fail, use K_bar = J(z || ct) instead */
  /* Overwrite K with K_bar = SHAKE256(z || ct) if fail */
  {
    uint8_t k_bar[FIO___MLKEM_SYMBYTES];
    fio_sha3_s state = fio_shake256_init();
    fio_sha3_consume(&state, z, FIO___MLKEM_SYMBYTES);
    fio_sha3_consume(&state, ct, FIO___MLKEM_INDCPA_BYTES);
    fio_shake_squeeze(&state, k_bar, FIO___MLKEM_SYMBYTES);

    /* Constant-time select: ss = fail ? k_bar : kr */
    FIO_MEMCPY(ss, kr, FIO___MLKEM_SYMBYTES);
    fio___mlkem_cmov(ss, k_bar, FIO___MLKEM_SYMBYTES, fail);
    FIO_MEMSET(k_bar, 0, sizeof(k_bar));
  }

  /* Zero sensitive stack data */
  FIO_MEMSET(buf, 0, sizeof(buf));
  FIO_MEMSET(kr, 0, sizeof(kr));
  FIO_MEMSET(cmp, 0, sizeof(cmp));

  return 0;
}

/* *****************************************************************************
Cleanup - Internal Macros
***************************************************************************** */

/* *****************************************************************************
Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MLKEM
#endif /* FIO_MLKEM */

/* Cleanup internal macros after guard closes */
#undef FIO___MLKEM_N
#undef FIO___MLKEM_K
#undef FIO___MLKEM_Q
#undef FIO___MLKEM_ETA1
#undef FIO___MLKEM_ETA2
#undef FIO___MLKEM_DU
#undef FIO___MLKEM_DV
#undef FIO___MLKEM_SYMBYTES
#undef FIO___MLKEM_POLYBYTES
#undef FIO___MLKEM_POLYVECBYTES
#undef FIO___MLKEM_POLYCOMPRESSEDBYTES
#undef FIO___MLKEM_POLYVECCOMPRESSEDBYTES
#undef FIO___MLKEM_INDCPA_MSGBYTES
#undef FIO___MLKEM_INDCPA_PUBLICKEYBYTES
#undef FIO___MLKEM_INDCPA_SECRETKEYBYTES
#undef FIO___MLKEM_INDCPA_BYTES
#undef FIO___MLKEM_MONT
#undef FIO___MLKEM_QINV
#undef FIO___MLKEM_GEN_MATRIX_NBLOCKS
