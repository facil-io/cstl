/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_P384               /* Development inclusion - ignore line */
#define FIO_SHA2               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                    Elliptic Curve Cryptography: ECDSA P-384 (secp384r1)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_P384) && !defined(H___FIO_P384___H)
#define H___FIO_P384___H

/* *****************************************************************************
P-384 (secp384r1) ECDSA Module

This module provides ECDSA signature verification and signing for the P-384
curve, primarily for TLS 1.3 certificate chain validation (Let's Encrypt root
CAs) and P-384 certificate keys (CertificateVerify signing).

Secret scalars (private keys, nonces) only reach the constant-time Montgomery
ladder and the branch-free field / scalar arithmetic; verification keeps a
faster variable-time path (public data only).

P-384 curve parameters (NIST FIPS 186-4):
- Prime p = 2^384 - 2^128 - 2^96 + 2^32 - 1
- Order n = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF
            581A0DB248B0A77AECEC196ACCC52973
- Curve equation: y^2 = x^3 - 3x + b (mod p)

**Note**: This implementation has not been audited. Use at your own risk.
***************************************************************************** */

/* *****************************************************************************
ECDSA P-384 Verification / Signing API
***************************************************************************** */

/**
 * Verifies an ECDSA P-384 signature.
 *
 * @param sig DER-encoded signature (SEQUENCE { r INTEGER, s INTEGER })
 * @param sig_len Length of signature in bytes
 * @param msg_hash SHA-384 hash of the message (48 bytes)
 * @param pubkey SEC 1 public key: uncompressed (97 bytes: 0x04 || x || y)
 *               or compressed (49 bytes: 0x02/0x03 || x)
 * @param pubkey_len Length of public key (97 or 49)
 * @return 0 on success (valid signature), -1 on failure (invalid signature or
 *         an invalid / off-curve public key)
 */
SFUNC int fio_ecdsa_p384_verify(const uint8_t *sig,
                                size_t sig_len,
                                const uint8_t *msg_hash,
                                const uint8_t *pubkey,
                                size_t pubkey_len);

/**
 * Verifies an ECDSA P-384 signature with raw r,s values.
 *
 * @param r The r component of the signature (48 bytes, big-endian)
 * @param s The s component of the signature (48 bytes, big-endian)
 * @param msg_hash SHA-384 hash of the message (48 bytes)
 * @param pubkey_x X coordinate of public key (48 bytes, big-endian)
 * @param pubkey_y Y coordinate of public key (48 bytes, big-endian)
 * @return 0 on success (valid signature), -1 on failure (invalid signature)
 */
SFUNC int fio_ecdsa_p384_verify_raw(const uint8_t r[48],
                                    const uint8_t s[48],
                                    const uint8_t msg_hash[48],
                                    const uint8_t pubkey_x[48],
                                    const uint8_t pubkey_y[48]);

/**
 * Signs a message hash using ECDSA P-384 (random nonce).
 *
 * @param sig Output: DER-encoded signature (max 104 bytes)
 * @param sig_len Output: actual signature length
 * @param sig_capacity Capacity of signature buffer (must be >= 104)
 * @param msg_hash SHA-384 hash of the message (48 bytes)
 * @param secret_key 48-byte secret key (scalar, big-endian), in [1, n-1]
 * @return 0 on success, -1 on failure (NULL arguments, a small buffer or an
 *         out-of-range secret key)
 */
SFUNC int fio_ecdsa_p384_sign(uint8_t *sig,
                              size_t *sig_len,
                              size_t sig_capacity,
                              const uint8_t msg_hash[48],
                              const uint8_t secret_key[48]);

/**
 * Derives the P-384 public key for an existing secret key (`Q = d * G`).
 *
 * Uses the constant-time scalar multiplication ladder.
 *
 * @param public_key Output: 97-byte uncompressed public key (0x04 || x || y)
 * @param secret_key 48-byte secret key (scalar, big-endian), in [1, n-1]
 * @return 0 on success, -1 on NULL arguments or an out-of-range scalar
 *         (`public_key`, when not NULL, is zeroed on failure).
 */
SFUNC int fio_p384_public_key(uint8_t public_key[97],
                              const uint8_t secret_key[48]);

/**
 * Generates a random P-384 keypair.
 *
 * @param secret_key Output: 48-byte secret key (scalar, big-endian)
 * @param public_key Output: 97-byte uncompressed public key (0x04 || x || y)
 * @return 0 on success, -1 on failure
 */
SFUNC int fio_p384_keypair(uint8_t secret_key[48], uint8_t public_key[97]);

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
P-384 Field Arithmetic (mod p)

Prime p = 2^384 - 2^128 - 2^96 + 2^32 - 1
       = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE
         FFFFFFFF0000000000000000FFFFFFFF

We use 6 x 64-bit limbs in little-endian order.
***************************************************************************** */

/* P-384 prime p in little-endian limbs */
static const uint64_t FIO___P384_P[6] = {
    0x00000000FFFFFFFFULL, /* p[0] */
    0xFFFFFFFF00000000ULL, /* p[1] */
    0xFFFFFFFFFFFFFFFEULL, /* p[2] */
    0xFFFFFFFFFFFFFFFFULL, /* p[3] */
    0xFFFFFFFFFFFFFFFFULL, /* p[4] */
    0xFFFFFFFFFFFFFFFFULL, /* p[5] */
};

/* P-384 curve order n in little-endian limbs */
static const uint64_t FIO___P384_N[6] = {
    0xECEC196ACCC52973ULL, /* n[0] */
    0x581A0DB248B0A77AULL, /* n[1] */
    0xC7634D81F4372DDFULL, /* n[2] */
    0xFFFFFFFFFFFFFFFFULL, /* n[3] */
    0xFFFFFFFFFFFFFFFFULL, /* n[4] */
    0xFFFFFFFFFFFFFFFFULL, /* n[5] */
};

/* P-384 curve parameter b in little-endian limbs */
static const uint64_t FIO___P384_B[6] = {
    0x2A85C8EDD3EC2AEFULL, /* b[0] */
    0xC656398D8A2ED19DULL, /* b[1] */
    0x0314088F5013875AULL, /* b[2] */
    0x181D9C6EFE814112ULL, /* b[3] */
    0x988E056BE3F82D19ULL, /* b[4] */
    0xB3312FA7E23EE7E4ULL, /* b[5] */
};

/* P-384 base point G x-coordinate in little-endian limbs */
static const uint64_t FIO___P384_GX[6] = {
    0x3A545E3872760AB7ULL, /* Gx[0] */
    0x5502F25DBF55296CULL, /* Gx[1] */
    0x59F741E082542A38ULL, /* Gx[2] */
    0x6E1D3B628BA79B98ULL, /* Gx[3] */
    0x8EB1C71EF320AD74ULL, /* Gx[4] */
    0xAA87CA22BE8B0537ULL, /* Gx[5] */
};

/* P-384 base point G y-coordinate in little-endian limbs */
static const uint64_t FIO___P384_GY[6] = {
    0x7A431D7C90EA0E5FULL, /* Gy[0] */
    0x0A60B1CE1D7E819DULL, /* Gy[1] */
    0xE9DA3113B5F0B8C0ULL, /* Gy[2] */
    0xF8F41DBD289A147CULL, /* Gy[3] */
    0x5D9E98BF9292DC29ULL, /* Gy[4] */
    0x3617DE4A96262C6FULL, /* Gy[5] */
};

/* Field element type: 6 x 64-bit limbs in little-endian */
typedef uint64_t fio___p384_fe_s[6];

/* Point in affine coordinates */
typedef struct {
  fio___p384_fe_s x;
  fio___p384_fe_s y;
} fio___p384_point_affine_s;

/* Point in Jacobian coordinates (x = X/Z^2, y = Y/Z^3) */
typedef struct {
  fio___p384_fe_s x;
  fio___p384_fe_s y;
  fio___p384_fe_s z;
} fio___p384_point_jacobian_s;

/* *****************************************************************************
Field Element Operations (mod p)
***************************************************************************** */

/** Copy field element: dest = src */
FIO_IFUNC void fio___p384_fe_copy(fio___p384_fe_s dest,
                                  const fio___p384_fe_s src) {
  dest[0] = src[0];
  dest[1] = src[1];
  dest[2] = src[2];
  dest[3] = src[3];
  dest[4] = src[4];
  dest[5] = src[5];
}

/** Set field element to zero */
FIO_IFUNC void fio___p384_fe_zero(fio___p384_fe_s r) {
  r[0] = 0;
  r[1] = 0;
  r[2] = 0;
  r[3] = 0;
  r[4] = 0;
  r[5] = 0;
}

/** Set field element to one */
FIO_IFUNC void fio___p384_fe_one(fio___p384_fe_s r) {
  r[0] = 1;
  r[1] = 0;
  r[2] = 0;
  r[3] = 0;
  r[4] = 0;
  r[5] = 0;
}

/** Public-value check: returns 1 if field element is zero. */
FIO_IFUNC int fio___p384_fe_is_zero(const fio___p384_fe_s a) {
  return (a[0] | a[1] | a[2] | a[3] | a[4] | a[5]) == 0;
}

/** Secret-safe check: returns all-ones if field element is zero, else 0. */
FIO_IFUNC uint64_t fio___p384_fe_ct_is_zero_mask(const fio___p384_fe_s a) {
  uint64_t diff = a[0] | a[1] | a[2] | a[3] | a[4] | a[5];
  return (uint64_t)0 - (uint64_t)fio_ct_false(diff);
}

/** Public-value compare: returns 0 if equal. */
FIO_IFUNC int fio___p384_fe_eq(const fio___p384_fe_s a,
                               const fio___p384_fe_s b) {
  uint64_t diff = (a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2]) |
                  (a[3] ^ b[3]) | (a[4] ^ b[4]) | (a[5] ^ b[5]);
  return diff == 0 ? 0 : 1;
}

/** Load 48-byte big-endian number into field element */
FIO_IFUNC void fio___p384_fe_from_bytes(fio___p384_fe_s r,
                                        const uint8_t in[48]) {
  r[5] = fio_buf2u64_be(in);
  r[4] = fio_buf2u64_be(in + 8);
  r[3] = fio_buf2u64_be(in + 16);
  r[2] = fio_buf2u64_be(in + 24);
  r[1] = fio_buf2u64_be(in + 32);
  r[0] = fio_buf2u64_be(in + 40);
}

/** Public-value load: returns -1 (r undefined) if encoding is >= p. */
FIO_IFUNC int fio___p384_fe_from_bytes_canonical(fio___p384_fe_s r,
                                                 const uint8_t in[48]) {
  /* Only the final borrow of r - p matters (set iff r < p); the difference
   * words are discarded (sink keeps GCC's warn_unused_result quiet). */
  uint64_t borrow = 0, diff;
  fio___p384_fe_from_bytes(r, in);
  diff = fio_math_subc64(r[0], FIO___P384_P[0], 0, &borrow);
  diff = fio_math_subc64(r[1], FIO___P384_P[1], borrow, &borrow);
  diff = fio_math_subc64(r[2], FIO___P384_P[2], borrow, &borrow);
  diff = fio_math_subc64(r[3], FIO___P384_P[3], borrow, &borrow);
  diff = fio_math_subc64(r[4], FIO___P384_P[4], borrow, &borrow);
  diff = fio_math_subc64(r[5], FIO___P384_P[5], borrow, &borrow);
  (void)diff;
  return borrow ? 0 : -1; /* borrow means r < p */
}

/** Store field element to 48-byte big-endian output */
FIO_IFUNC void fio___p384_fe_to_bytes(uint8_t out[48],
                                      const fio___p384_fe_s a) {
  fio_u2buf64_be(out, a[5]);
  fio_u2buf64_be(out + 8, a[4]);
  fio_u2buf64_be(out + 16, a[3]);
  fio_u2buf64_be(out + 24, a[2]);
  fio_u2buf64_be(out + 32, a[1]);
  fio_u2buf64_be(out + 40, a[0]);
}

/** Field addition: r = a + b mod p */
FIO_IFUNC void fio___p384_fe_add(fio___p384_fe_s r,
                                 const fio___p384_fe_s a,
                                 const fio___p384_fe_s b) {
  uint64_t c = 0;
  uint64_t t[6];

  /* Add a + b */
  t[0] = fio_math_addc64(a[0], b[0], 0, &c);
  t[1] = fio_math_addc64(a[1], b[1], c, &c);
  t[2] = fio_math_addc64(a[2], b[2], c, &c);
  t[3] = fio_math_addc64(a[3], b[3], c, &c);
  t[4] = fio_math_addc64(a[4], b[4], c, &c);
  t[5] = fio_math_addc64(a[5], b[5], c, &c);

  /* Reduce mod p if needed: subtract p if result >= p */
  uint64_t borrow = 0;
  uint64_t s[6];
  s[0] = fio_math_subc64(t[0], FIO___P384_P[0], 0, &borrow);
  s[1] = fio_math_subc64(t[1], FIO___P384_P[1], borrow, &borrow);
  s[2] = fio_math_subc64(t[2], FIO___P384_P[2], borrow, &borrow);
  s[3] = fio_math_subc64(t[3], FIO___P384_P[3], borrow, &borrow);
  s[4] = fio_math_subc64(t[4], FIO___P384_P[4], borrow, &borrow);
  s[5] = fio_math_subc64(t[5], FIO___P384_P[5], borrow, &borrow);

  /* If carry from addition or no borrow from subtraction, use subtracted value
   */
  uint64_t mask = (uint64_t)0 - (c | (borrow ^ 1));
  r[0] = (s[0] & mask) | (t[0] & ~mask);
  r[1] = (s[1] & mask) | (t[1] & ~mask);
  r[2] = (s[2] & mask) | (t[2] & ~mask);
  r[3] = (s[3] & mask) | (t[3] & ~mask);
  r[4] = (s[4] & mask) | (t[4] & ~mask);
  r[5] = (s[5] & mask) | (t[5] & ~mask);
}

/** Field subtraction: r = a - b mod p */
FIO_IFUNC void fio___p384_fe_sub(fio___p384_fe_s r,
                                 const fio___p384_fe_s a,
                                 const fio___p384_fe_s b) {
  uint64_t borrow = 0;
  uint64_t t[6];

  /* Subtract a - b */
  t[0] = fio_math_subc64(a[0], b[0], 0, &borrow);
  t[1] = fio_math_subc64(a[1], b[1], borrow, &borrow);
  t[2] = fio_math_subc64(a[2], b[2], borrow, &borrow);
  t[3] = fio_math_subc64(a[3], b[3], borrow, &borrow);
  t[4] = fio_math_subc64(a[4], b[4], borrow, &borrow);
  t[5] = fio_math_subc64(a[5], b[5], borrow, &borrow);

  /* If borrow, add p back */
  uint64_t c = 0;
  uint64_t mask = (uint64_t)0 - borrow;
  uint64_t s[6];
  s[0] = fio_math_addc64(t[0], FIO___P384_P[0] & mask, 0, &c);
  s[1] = fio_math_addc64(t[1], FIO___P384_P[1] & mask, c, &c);
  s[2] = fio_math_addc64(t[2], FIO___P384_P[2] & mask, c, &c);
  s[3] = fio_math_addc64(t[3], FIO___P384_P[3] & mask, c, &c);
  s[4] = fio_math_addc64(t[4], FIO___P384_P[4] & mask, c, &c);
  s[5] = fio_math_addc64(t[5], FIO___P384_P[5] & mask, c, &c);

  r[0] = s[0];
  r[1] = s[1];
  r[2] = s[2];
  r[3] = s[3];
  r[4] = s[4];
  r[5] = s[5];
}

/** Field negation: r = -a mod p */
FIO_IFUNC void fio___p384_fe_neg(fio___p384_fe_s r, const fio___p384_fe_s a) {
  fio___p384_fe_s zero = {0};
  fio___p384_fe_sub(r, zero, a);
}

/**
 * P-384 reduction using the NIST fast reduction formula.
 *
 * p = 2^384 - 2^128 - 2^96 + 2^32 - 1
 *
 * For a 768-bit number T, we use the NIST formula with 32-bit words.
 * T = (c23,...,c0) where each ci is 32 bits.
 *
 * The result is: T mod p = s1 + 2*s2 + s3 + s4 + s5 + s6 + s7 - s8 - s9 - s10
 * where each si is a 384-bit value constructed from the 32-bit words.
 *
 * Reference: NIST FIPS 186-4, Section D.2.4
 */
FIO_SFUNC void fio___p384_fe_reduce(fio___p384_fe_s r, const uint64_t t[12]) {
  /* Extract 32-bit words from 768-bit input (little-endian) */
  uint32_t c[24];
  for (int i = 0; i < 12; ++i) {
    c[2 * i] = (uint32_t)t[i];
    c[2 * i + 1] = (uint32_t)(t[i] >> 32);
  }

  /* Use 64-bit accumulators for each 32-bit position to handle carries.
   * acc[0] is LSB (c0 position), acc[11] is MSB (c11 position).
   * NIST notation: (c11, c10, ..., c1, c0) means
   * c11 is at position 11 (MSB), c0 is at position 0 (LSB). */
  int64_t acc[12] = {0};

  /* s1 = (c11, c10, c9, c8, c7, c6, c5, c4, c3, c2, c1, c0) - the low 384 bits
   */
  acc[0] += c[0];
  acc[1] += c[1];
  acc[2] += c[2];
  acc[3] += c[3];
  acc[4] += c[4];
  acc[5] += c[5];
  acc[6] += c[6];
  acc[7] += c[7];
  acc[8] += c[8];
  acc[9] += c[9];
  acc[10] += c[10];
  acc[11] += c[11];

  /* s2 = (0, 0, 0, 0, 0, c23, c22, c21, 0, 0, 0, 0) - add twice */
  acc[4] += 2 * (int64_t)c[21];
  acc[5] += 2 * (int64_t)c[22];
  acc[6] += 2 * (int64_t)c[23];

  /* s3 = (c23, c22, c21, c20, c19, c18, c17, c16, c15, c14, c13, c12) */
  acc[0] += c[12];
  acc[1] += c[13];
  acc[2] += c[14];
  acc[3] += c[15];
  acc[4] += c[16];
  acc[5] += c[17];
  acc[6] += c[18];
  acc[7] += c[19];
  acc[8] += c[20];
  acc[9] += c[21];
  acc[10] += c[22];
  acc[11] += c[23];

  /* s4 = (c20, c19, c18, c17, c16, c15, c14, c13, c12, c23, c22, c21) */
  acc[0] += c[21];
  acc[1] += c[22];
  acc[2] += c[23];
  acc[3] += c[12];
  acc[4] += c[13];
  acc[5] += c[14];
  acc[6] += c[15];
  acc[7] += c[16];
  acc[8] += c[17];
  acc[9] += c[18];
  acc[10] += c[19];
  acc[11] += c[20];

  /* s5 = (c19, c18, c17, c16, c15, c14, c13, c12, c20, 0, c23, 0) */
  acc[1] += c[23];
  acc[3] += c[20];
  acc[4] += c[12];
  acc[5] += c[13];
  acc[6] += c[14];
  acc[7] += c[15];
  acc[8] += c[16];
  acc[9] += c[17];
  acc[10] += c[18];
  acc[11] += c[19];

  /* s6 = (0, 0, 0, 0, c23, c22, c21, c20, 0, 0, 0, 0) */
  acc[4] += c[20];
  acc[5] += c[21];
  acc[6] += c[22];
  acc[7] += c[23];

  /* s7 = (0, 0, 0, 0, 0, 0, c23, c22, c21, 0, 0, c20) */
  acc[0] += c[20];
  acc[3] += c[21];
  acc[4] += c[22];
  acc[5] += c[23];

  /* s8 = (c22, c21, c20, c19, c18, c17, c16, c15, c14, c13, c12, c23) -
   * subtract */
  acc[0] -= c[23];
  acc[1] -= c[12];
  acc[2] -= c[13];
  acc[3] -= c[14];
  acc[4] -= c[15];
  acc[5] -= c[16];
  acc[6] -= c[17];
  acc[7] -= c[18];
  acc[8] -= c[19];
  acc[9] -= c[20];
  acc[10] -= c[21];
  acc[11] -= c[22];

  /* s9 = (0, 0, 0, 0, 0, 0, 0, c23, c22, c21, c20, 0) - subtract */
  acc[1] -= c[20];
  acc[2] -= c[21];
  acc[3] -= c[22];
  acc[4] -= c[23];

  /* s10 = (0, 0, 0, 0, 0, 0, 0, c23, c23, 0, 0, 0) - subtract */
  acc[3] -= c[23];
  acc[4] -= c[23];

  /* Carry propagation with signed arithmetic. */
  for (int i = 0; i < 11; ++i) {
    int64_t carry = acc[i] >> 32;
    acc[i] = acc[i] & 0xFFFFFFFFLL;
    acc[i + 1] += carry;
  }

  /* Fold the signed top carry back in: 2^384 = 2^128 + 2^96 - 2^32 + 1 (mod
   * p), i.e. H * 2^384 adds H to words 0, 3, 4 and subtracts it from word 1.
   * Fixed work, no data-dependent branches (field values may be secret).
   *
   * Bound: the sum above is in (-3 * 2^384, 8 * 2^384), so after the carry
   * propagation |H| <= 8 and the first fold leaves a value in
   * (-2^133, 2^384 + 2^133). Its carry H' is then -1, 0 or 1, and the second
   * fold lands in [0, 2^384) (for H' = 1 the low part is < 2^133, for H' = -1
   * it is > 2^384 - 2^133). */
  for (int pass = 0; pass < 2; ++pass) {
    int64_t h = acc[11] >> 32;
    acc[11] &= 0xFFFFFFFFLL;
    acc[0] += h;
    acc[1] -= h;
    acc[3] += h;
    acc[4] += h;
    for (int i = 0; i < 11; ++i) {
      int64_t carry = acc[i] >> 32;
      acc[i] = acc[i] & 0xFFFFFFFFLL;
      acc[i + 1] += carry;
    }
  }

  /* Convert back to 64-bit limbs */
  uint64_t res[6];
  res[0] = ((uint64_t)(uint32_t)acc[1] << 32) | (uint32_t)acc[0];
  res[1] = ((uint64_t)(uint32_t)acc[3] << 32) | (uint32_t)acc[2];
  res[2] = ((uint64_t)(uint32_t)acc[5] << 32) | (uint32_t)acc[4];
  res[3] = ((uint64_t)(uint32_t)acc[7] << 32) | (uint32_t)acc[6];
  res[4] = ((uint64_t)(uint32_t)acc[9] << 32) | (uint32_t)acc[8];
  res[5] = ((uint64_t)(uint32_t)acc[11] << 32) | (uint32_t)acc[10];

  /* Final reduction: res < 2^384 < 2p, so one branchless subtraction. */
  {
    uint64_t borrow = 0;
    uint64_t sub[6];
    sub[0] = fio_math_subc64(res[0], FIO___P384_P[0], 0, &borrow);
    sub[1] = fio_math_subc64(res[1], FIO___P384_P[1], borrow, &borrow);
    sub[2] = fio_math_subc64(res[2], FIO___P384_P[2], borrow, &borrow);
    sub[3] = fio_math_subc64(res[3], FIO___P384_P[3], borrow, &borrow);
    sub[4] = fio_math_subc64(res[4], FIO___P384_P[4], borrow, &borrow);
    sub[5] = fio_math_subc64(res[5], FIO___P384_P[5], borrow, &borrow);
    const uint64_t keep = (uint64_t)0 - borrow; /* ~0 if res < p */
    for (int i = 0; i < 6; ++i)
      res[i] = fio_ct_mux64(keep, res[i], sub[i]);
  }

  r[0] = res[0];
  r[1] = res[1];
  r[2] = res[2];
  r[3] = res[3];
  r[4] = res[4];
  r[5] = res[5];
}

/** Field multiplication: r = a * b mod p */
FIO_SFUNC void fio___p384_fe_mul(fio___p384_fe_s r,
                                 const fio___p384_fe_s a,
                                 const fio___p384_fe_s b) {
  uint64_t t[12] = {0};

  /* Schoolbook multiplication to get 768-bit product */
  for (int i = 0; i < 6; ++i) {
    uint64_t carry = 0;
    for (int j = 0; j < 6; ++j) {
      uint64_t hi;
      uint64_t lo = fio_math_mulc64(a[i], b[j], &hi);
      uint64_t c1 = 0, c2 = 0;
      t[i + j] = fio_math_addc64(t[i + j], lo, 0, &c1);
      t[i + j] = fio_math_addc64(t[i + j], carry, 0, &c2);
      carry = hi + c1 + c2;
    }
    t[i + 6] += carry;
  }

  /* Reduce mod p */
  fio___p384_fe_reduce(r, t);
}

/** Field squaring: r = a^2 mod p */
FIO_SFUNC void fio___p384_fe_sqr(fio___p384_fe_s r, const fio___p384_fe_s a) {
  fio___p384_fe_mul(r, a, a);
}

/**
 * Field inversion: r = a^(-1) mod p using Fermat's little theorem.
 * a^(-1) = a^(p-2) mod p
 *
 * p-2 = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE
 *       FFFFFFFF0000000000000000FFFFFFFD
 *
 * We use a simple square-and-multiply algorithm for correctness.
 */
FIO_SFUNC void fio___p384_fe_inv(fio___p384_fe_s r, const fio___p384_fe_s a) {
  /* p - 2 in little-endian 64-bit limbs */
  static const uint64_t pm2[6] = {
      0x00000000FFFFFFFDULL, /* pm2[0] */
      0xFFFFFFFF00000000ULL, /* pm2[1] */
      0xFFFFFFFFFFFFFFFEULL, /* pm2[2] */
      0xFFFFFFFFFFFFFFFFULL, /* pm2[3] */
      0xFFFFFFFFFFFFFFFFULL, /* pm2[4] */
      0xFFFFFFFFFFFFFFFFULL, /* pm2[5] */
  };

  fio___p384_fe_s base, result, tmp;

  /* Initialize result = 1 */
  fio___p384_fe_one(result);

  /* Copy input to base */
  fio___p384_fe_copy(base, a);

  /* Square-and-multiply from LSB to MSB */
  for (int i = 0; i < 6; ++i) {
    uint64_t bits = pm2[i];
    for (int j = 0; j < 64; ++j) {
      if (bits & 1) {
        /* result = result * base */
        fio___p384_fe_mul(tmp, result, base);
        fio___p384_fe_copy(result, tmp);
      }
      /* base = base^2 */
      fio___p384_fe_sqr(tmp, base);
      fio___p384_fe_copy(base, tmp);
      bits >>= 1;
    }
  }

  fio___p384_fe_copy(r, result);
}

/* *****************************************************************************
Scalar Arithmetic (mod n)
***************************************************************************** */

/** Scalar element type: 6 x 64-bit limbs in little-endian */
typedef uint64_t fio___p384_scalar_s[6];

/** Load 48-byte big-endian number into scalar */
FIO_IFUNC void fio___p384_scalar_from_bytes(fio___p384_scalar_s r,
                                            const uint8_t in[48]) {
  r[5] = fio_buf2u64_be(in);
  r[4] = fio_buf2u64_be(in + 8);
  r[3] = fio_buf2u64_be(in + 16);
  r[2] = fio_buf2u64_be(in + 24);
  r[1] = fio_buf2u64_be(in + 32);
  r[0] = fio_buf2u64_be(in + 40);
}

/** Public-value check: returns 1 if scalar is zero. */
FIO_IFUNC int fio___p384_scalar_is_zero(const fio___p384_scalar_s a) {
  return (a[0] | a[1] | a[2] | a[3] | a[4] | a[5]) == 0;
}

/** Secret-safe check: returns 1 if scalar is zero. */
FIO_IFUNC int fio___p384_scalar_ct_is_zero(const fio___p384_scalar_s a) {
  uint64_t diff = a[0] | a[1] | a[2] | a[3] | a[4] | a[5];
  return (int)fio_ct_false(diff);
}

/** Public-value check: returns 1 if scalar >= n (curve order). */
FIO_IFUNC int fio___p384_scalar_gte_n(const fio___p384_scalar_s a) {
  /* Compare from most significant limb */
  if (a[5] > FIO___P384_N[5])
    return 1;
  if (a[5] < FIO___P384_N[5])
    return 0;
  if (a[4] > FIO___P384_N[4])
    return 1;
  if (a[4] < FIO___P384_N[4])
    return 0;
  if (a[3] > FIO___P384_N[3])
    return 1;
  if (a[3] < FIO___P384_N[3])
    return 0;
  if (a[2] > FIO___P384_N[2])
    return 1;
  if (a[2] < FIO___P384_N[2])
    return 0;
  if (a[1] > FIO___P384_N[1])
    return 1;
  if (a[1] < FIO___P384_N[1])
    return 0;
  if (a[0] >= FIO___P384_N[0])
    return 1;
  return 0;
}

/** Secret-safe check: returns 1 if scalar >= n (curve order). */
FIO_IFUNC int fio___p384_scalar_ct_gte_n(const fio___p384_scalar_s a) {
  uint64_t borrow = 0, ignr;
  ignr = fio_math_subc64(a[0], FIO___P384_N[0], 0, &borrow);
  ignr = fio_math_subc64(a[1], FIO___P384_N[1], borrow, &borrow);
  ignr = fio_math_subc64(a[2], FIO___P384_N[2], borrow, &borrow);
  ignr = fio_math_subc64(a[3], FIO___P384_N[3], borrow, &borrow);
  ignr = fio_math_subc64(a[4], FIO___P384_N[4], borrow, &borrow);
  ignr = fio_math_subc64(a[5], FIO___P384_N[5], borrow, &borrow);
  (void)ignr;
  return (int)(borrow ^ 1);
}

/** Secret-safe conditional scalar subtraction: a = cond ? a - n : a. */
FIO_IFUNC void fio___p384_scalar_sub_n_if(fio___p384_scalar_s a,
                                          uint64_t cond) {
  uint64_t borrow = 0;
  uint64_t s[6];
  const uint64_t mask = (uint64_t)0 - (cond & 1);
  s[0] = fio_math_subc64(a[0], FIO___P384_N[0], 0, &borrow);
  s[1] = fio_math_subc64(a[1], FIO___P384_N[1], borrow, &borrow);
  s[2] = fio_math_subc64(a[2], FIO___P384_N[2], borrow, &borrow);
  s[3] = fio_math_subc64(a[3], FIO___P384_N[3], borrow, &borrow);
  s[4] = fio_math_subc64(a[4], FIO___P384_N[4], borrow, &borrow);
  s[5] = fio_math_subc64(a[5], FIO___P384_N[5], borrow, &borrow);
  for (int i = 0; i < 6; ++i)
    a[i] = fio_ct_mux64(mask, s[i], a[i]);
}

/** Barrett constant: mu = floor(2^768 / n), little-endian 64-bit limbs. */
static const uint64_t FIO___P384_N_MU[7] = {
    0x1313E695333AD68DULL,
    0xA7E5F24DB74F5885ULL,
    0x389CB27E0BC8D220ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000001ULL,
};

/**
 * Secret-safe scalar reduction: r = t mod n, for any t < 2^768.
 *
 * Barrett reduction (HAC 14.42, b = 2^64, k = 6) with fixed work and no
 * secret-dependent branches or memory access (replaces the variable-time
 * long division previously used here).
 */
FIO_SFUNC void fio___p384_scalar_reduce(fio___p384_scalar_s r,
                                        const uint64_t t[12]) {
  static const uint64_t n7[7] = {FIO___P384_N[0],
                                 FIO___P384_N[1],
                                 FIO___P384_N[2],
                                 FIO___P384_N[3],
                                 FIO___P384_N[4],
                                 FIO___P384_N[5],
                                 0};
  uint64_t q[14], rr[7], s[7] = {0};
  /* q3 = floor(floor(t / 2^320) * mu / 2^448) -> q[7..13] */
  fio_math_mul(q, t + 5, FIO___P384_N_MU, 7);
  /* s = (q3 * n) mod 2^448 (truncated schoolbook) */
  for (size_t i = 0; i < 7; ++i) {
    uint64_t carry = 0;
    for (size_t j = 0; i + j < 7; ++j) {
      uint64_t hi, c1 = 0, c2 = 0;
      const uint64_t lo = fio_math_mulc64(q[7 + i], n7[j], &hi);
      s[i + j] = fio_math_addc64(s[i + j], lo, 0, &c1);
      s[i + j] = fio_math_addc64(s[i + j], carry, 0, &c2);
      carry = hi + c1 + c2;
    }
  }
  /* rr = (t mod 2^448) - s (mod 2^448); 0 <= rr < 3n */
  const uint64_t ignored_borrow = fio_math_sub(rr, t, s, 7);
  (void)ignored_borrow;
  /* at most two conditional subtractions of n */
  for (size_t round = 0; round < 2; ++round) {
    const uint64_t borrow = fio_math_sub(s, rr, n7, 7);
    const uint64_t mask = borrow - 1; /* all ones when rr >= n */
    for (size_t i = 0; i < 7; ++i)
      rr[i] = fio_ct_mux64(mask, s[i], rr[i]);
  }
  r[0] = rr[0];
  r[1] = rr[1];
  r[2] = rr[2];
  r[3] = rr[3];
  r[4] = rr[4];
  r[5] = rr[5];
}

/** Scalar multiplication: r = a * b mod n (secret-safe, fixed work). */
FIO_SFUNC void fio___p384_scalar_mul(fio___p384_scalar_s r,
                                     const fio___p384_scalar_s a,
                                     const fio___p384_scalar_s b) {
  uint64_t t[12];
  fio_math_mul(t, a, b, 6);
  fio___p384_scalar_reduce(r, t);
}

/**
 * Scalar inversion: r = a^(-1) mod n using Fermat's little theorem.
 * a^(-1) = a^(n-2) mod n
 */
FIO_SFUNC void fio___p384_scalar_inv(fio___p384_scalar_s r,
                                     const fio___p384_scalar_s a) {
  /* n-2 = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF
   *       581A0DB248B0A77AECEC196ACCC52971 */
  fio___p384_scalar_s t, tmp;

  /* Start with a */
  fio___p384_scalar_s base;
  base[0] = a[0];
  base[1] = a[1];
  base[2] = a[2];
  base[3] = a[3];
  base[4] = a[4];
  base[5] = a[5];

  /* Initialize result to 1 */
  t[0] = 1;
  t[1] = 0;
  t[2] = 0;
  t[3] = 0;
  t[4] = 0;
  t[5] = 0;

  /* Process each bit of n-2 from LSB to MSB */
  static const uint64_t nm2[6] = {
      0xECEC196ACCC52971ULL, /* (n-2)[0] */
      0x581A0DB248B0A77AULL, /* (n-2)[1] */
      0xC7634D81F4372DDFULL, /* (n-2)[2] */
      0xFFFFFFFFFFFFFFFFULL, /* (n-2)[3] */
      0xFFFFFFFFFFFFFFFFULL, /* (n-2)[4] */
      0xFFFFFFFFFFFFFFFFULL, /* (n-2)[5] */
  };

  for (int i = 0; i < 6; ++i) {
    uint64_t bits = nm2[i];
    for (int j = 0; j < 64; ++j) {
      if (bits & 1) {
        fio___p384_scalar_mul(tmp, t, base);
        t[0] = tmp[0];
        t[1] = tmp[1];
        t[2] = tmp[2];
        t[3] = tmp[3];
        t[4] = tmp[4];
        t[5] = tmp[5];
      }
      fio___p384_scalar_mul(tmp, base, base);
      base[0] = tmp[0];
      base[1] = tmp[1];
      base[2] = tmp[2];
      base[3] = tmp[3];
      base[4] = tmp[4];
      base[5] = tmp[5];
      bits >>= 1;
    }
  }

  r[0] = t[0];
  r[1] = t[1];
  r[2] = t[2];
  r[3] = t[3];
  r[4] = t[4];
  r[5] = t[5];
}

/* *****************************************************************************
Point Operations (Jacobian Coordinates)
***************************************************************************** */

/** Check if point is at infinity (Z = 0) */
FIO_IFUNC int fio___p384_point_is_infinity(
    const fio___p384_point_jacobian_s *p) {
  return fio___p384_fe_is_zero(p->z);
}

/** Set point to infinity */
FIO_IFUNC void fio___p384_point_set_infinity(fio___p384_point_jacobian_s *p) {
  fio___p384_fe_one(p->x);
  fio___p384_fe_one(p->y);
  fio___p384_fe_zero(p->z);
}

/** Convert affine point to Jacobian */
FIO_IFUNC void fio___p384_point_to_jacobian(
    fio___p384_point_jacobian_s *j,
    const fio___p384_point_affine_s *a) {
  fio___p384_fe_copy(j->x, a->x);
  fio___p384_fe_copy(j->y, a->y);
  fio___p384_fe_one(j->z);
}

/** Convert Jacobian point to affine (x = X/Z^2, y = Y/Z^3)
 * Infinity has no affine form: it yields (0,0), which is NOT a curve point.
 * Callers must check fio___p384_point_is_infinity() first. */
FIO_SFUNC void fio___p384_point_to_affine(
    fio___p384_point_affine_s *a,
    const fio___p384_point_jacobian_s *j) {
  fio___p384_fe_s z_inv, z_inv2, z_inv3;

  /* Inverting Z = 0 yields zero with this fixed-exponent inversion, so
   * infinity maps to (0, 0) with no branch on (possibly secret) Z. */
  fio___p384_fe_inv(z_inv, j->z);
  fio___p384_fe_sqr(z_inv2, z_inv);
  fio___p384_fe_mul(z_inv3, z_inv2, z_inv);

  fio___p384_fe_mul(a->x, j->x, z_inv2);
  fio___p384_fe_mul(a->y, j->y, z_inv3);

  fio_secure_zero(z_inv, sizeof(z_inv));
  fio_secure_zero(z_inv2, sizeof(z_inv2));
  fio_secure_zero(z_inv3, sizeof(z_inv3));
}

/**
 * Point doubling in Jacobian coordinates (variable time: public points only;
 * secret scalars use fio___p384_point_double_ct).
 * Uses the formula from "Guide to Elliptic Curve Cryptography" (Hankerson et
 * al.)
 *
 * For P-384 where a = -3:
 * lambda = 3(X - Z^2)(X + Z^2)
 * X' = lambda^2 - 2S where S = 4XY^2
 * Y' = lambda(S - X') - 8Y^4
 * Z' = 2YZ
 */
FIO_SFUNC void fio___p384_point_double(fio___p384_point_jacobian_s *r,
                                       const fio___p384_point_jacobian_s *p) {
  if (fio___p384_point_is_infinity(p)) {
    fio___p384_point_set_infinity(r);
    return;
  }

  fio___p384_fe_s t1, t2, t3, t4, t5, yz;

  /* Save Y*Z early to handle aliasing (r == p) */
  fio___p384_fe_mul(yz, p->y, p->z);

  /* t1 = Z^2 */
  fio___p384_fe_sqr(t1, p->z);

  /* t2 = X - Z^2 */
  fio___p384_fe_sub(t2, p->x, t1);

  /* t3 = X + Z^2 */
  fio___p384_fe_add(t3, p->x, t1);

  /* t2 = (X - Z^2)(X + Z^2) = X^2 - Z^4 */
  fio___p384_fe_mul(t2, t2, t3);

  /* t2 = 3(X^2 - Z^4) = lambda (since a = -3, this equals 3X^2 + aZ^4) */
  fio___p384_fe_add(t3, t2, t2);
  fio___p384_fe_add(t2, t3, t2);

  /* t4 = Y^2 */
  fio___p384_fe_sqr(t4, p->y);

  /* t5 = XY^2 */
  fio___p384_fe_mul(t5, p->x, t4);

  /* t5 = 4XY^2 = S */
  fio___p384_fe_add(t5, t5, t5);
  fio___p384_fe_add(t5, t5, t5);

  /* t3 = lambda^2 */
  fio___p384_fe_sqr(t3, t2);

  /* X' = lambda^2 - 2S */
  fio___p384_fe_sub(r->x, t3, t5);
  fio___p384_fe_sub(r->x, r->x, t5);

  /* t4 = Y^4 */
  fio___p384_fe_sqr(t4, t4);

  /* t4 = 8Y^4 */
  fio___p384_fe_add(t4, t4, t4);
  fio___p384_fe_add(t4, t4, t4);
  fio___p384_fe_add(t4, t4, t4);

  /* t5 = S - X' */
  fio___p384_fe_sub(t5, t5, r->x);

  /* Y' = lambda(S - X') - 8Y^4 */
  fio___p384_fe_mul(r->y, t2, t5);
  fio___p384_fe_sub(r->y, r->y, t4);

  /* Z' = 2YZ (using pre-computed value to handle aliasing) */
  fio___p384_fe_add(r->z, yz, yz);
}

/**
 * Point addition in Jacobian coordinates.
 * r = p + q where p is Jacobian and q is affine.
 *
 * Mixed addition is faster than full Jacobian addition.
 *
 * Variable time (branches on the operands): public points only; secret
 * scalars use fio___p384_point_add_mixed_ct.
 */
FIO_SFUNC void fio___p384_point_add_mixed(fio___p384_point_jacobian_s *r,
                                          const fio___p384_point_jacobian_s *p,
                                          const fio___p384_point_affine_s *q) {
  if (fio___p384_point_is_infinity(p)) {
    fio___p384_point_to_jacobian(r, q);
    return;
  }

  fio___p384_fe_s t1, t2, t3, t4, t5, t6;

  /* t1 = Z1^2 */
  fio___p384_fe_sqr(t1, p->z);

  /* t2 = Z1^3 */
  fio___p384_fe_mul(t2, t1, p->z);

  /* t3 = X2*Z1^2 */
  fio___p384_fe_mul(t3, q->x, t1);

  /* t4 = Y2*Z1^3 */
  fio___p384_fe_mul(t4, q->y, t2);

  /* t3 = X2*Z1^2 - X1 = H */
  fio___p384_fe_sub(t3, t3, p->x);

  /* t4 = Y2*Z1^3 - Y1 = R */
  fio___p384_fe_sub(t4, t4, p->y);

  /* Check if points are equal or opposite */
  if (fio___p384_fe_is_zero(t3)) {
    if (fio___p384_fe_is_zero(t4)) {
      /* Points are equal - double */
      fio___p384_point_double(r, p);
      return;
    } else {
      /* Points are opposite - result is infinity */
      fio___p384_point_set_infinity(r);
      return;
    }
  }

  /* t5 = H^2 */
  fio___p384_fe_sqr(t5, t3);

  /* t6 = H^3 */
  fio___p384_fe_mul(t6, t5, t3);

  /* t1 = X1*H^2 */
  fio___p384_fe_mul(t1, p->x, t5);

  /* t2 = Y1*H^3 (compute early to handle r == p aliasing) */
  fio___p384_fe_mul(t2, p->y, t6);

  /* Z3 = Z1*H (compute early to handle r == p aliasing) */
  fio___p384_fe_mul(r->z, p->z, t3);

  /* X3 = R^2 - H^3 - 2*X1*H^2 */
  fio___p384_fe_sqr(r->x, t4);
  fio___p384_fe_sub(r->x, r->x, t6);
  fio___p384_fe_sub(r->x, r->x, t1);
  fio___p384_fe_sub(r->x, r->x, t1);

  /* t1 = X1*H^2 - X3 */
  fio___p384_fe_sub(t1, t1, r->x);

  /* Y3 = R(X1*H^2 - X3) - Y1*H^3 */
  fio___p384_fe_mul(r->y, t4, t1);
  fio___p384_fe_sub(r->y, r->y, t2);
}

/**
 * Constant-time conditional swap of two Jacobian points.
 * swap must be exactly 0 or 1.
 */
FIO_IFUNC void fio___p384_point_cswap(fio___p384_point_jacobian_s *a,
                                      fio___p384_point_jacobian_s *b,
                                      uint64_t swap) {
  const uint64_t mask = (uint64_t)0 - swap; /* 0x000...0 or 0xFFF...F */
  for (int i = 0; i < 6; ++i) {
    uint64_t tmp = mask & (a->x[i] ^ b->x[i]);
    a->x[i] ^= tmp;
    b->x[i] ^= tmp;
    tmp = mask & (a->y[i] ^ b->y[i]);
    a->y[i] ^= tmp;
    b->y[i] ^= tmp;
    tmp = mask & (a->z[i] ^ b->z[i]);
    a->z[i] ^= tmp;
    b->z[i] ^= tmp;
  }
}

/**
 * Constant-time point doubling (same formula as fio___p384_point_double).
 *
 * The infinity case (Z = 0) is selected with masks, never branched on.
 */
FIO_SFUNC void fio___p384_point_double_ct(
    fio___p384_point_jacobian_s *r,
    const fio___p384_point_jacobian_s *p) {
  fio___p384_fe_s t1, t2, t3, t4, t5, yz;
  fio___p384_point_jacobian_s generic, candidate;

  fio___p384_fe_mul(yz, p->y, p->z); /* YZ (aliasing safe) */
  fio___p384_fe_sqr(t1, p->z);       /* Z^2 */
  fio___p384_fe_sub(t2, p->x, t1);   /* X - Z^2 */
  fio___p384_fe_add(t3, p->x, t1);   /* X + Z^2 */
  fio___p384_fe_mul(t2, t2, t3);     /* X^2 - Z^4 */
  fio___p384_fe_add(t3, t2, t2);     /* lambda = 3(...) */
  fio___p384_fe_add(t2, t3, t2);
  fio___p384_fe_sqr(t4, p->y);     /* Y^2 */
  fio___p384_fe_mul(t5, p->x, t4); /* XY^2 */
  fio___p384_fe_add(t5, t5, t5);   /* S = 4XY^2 */
  fio___p384_fe_add(t5, t5, t5);
  fio___p384_fe_sqr(t3, t2);            /* lambda^2 */
  fio___p384_fe_sub(generic.x, t3, t5); /* X' = lambda^2 - 2S */
  fio___p384_fe_sub(generic.x, generic.x, t5);
  fio___p384_fe_sqr(t4, t4);     /* Y^4 */
  fio___p384_fe_add(t4, t4, t4); /* 8Y^4 */
  fio___p384_fe_add(t4, t4, t4);
  fio___p384_fe_add(t4, t4, t4);
  fio___p384_fe_sub(t5, t5, generic.x); /* S - X' */
  fio___p384_fe_mul(generic.y, t2, t5); /* Y' */
  fio___p384_fe_sub(generic.y, generic.y, t4);
  fio___p384_fe_add(generic.z, yz, yz); /* Z' = 2YZ */

  /* Select infinity for an infinity input without branching on p->z. */
  const uint64_t inf = fio___p384_fe_ct_is_zero_mask(p->z) & 1;
  fio___p384_point_set_infinity(&candidate);
  *r = generic;
  fio___p384_point_cswap(r, &candidate, inf);

  fio_secure_zero(t1, sizeof(t1));
  fio_secure_zero(t2, sizeof(t2));
  fio_secure_zero(t3, sizeof(t3));
  fio_secure_zero(t4, sizeof(t4));
  fio_secure_zero(t5, sizeof(t5));
  fio_secure_zero(yz, sizeof(yz));
  fio_secure_zero(&generic, sizeof(generic));
  fio_secure_zero(&candidate, sizeof(candidate));
}

/**
 * Constant-time mixed addition r = p + q (p Jacobian, q affine, q != O).
 *
 * Computes the generic sum, the doubling and the infinity / q results
 * unconditionally and selects the right one with masks:
 * - p infinity: q
 * - H == 0 and R == 0 (p == q): 2p
 * - H == 0 and R != 0 (p == -q): infinity
 * - otherwise: the generic mixed-add result
 */
FIO_SFUNC void fio___p384_point_add_mixed_ct(
    fio___p384_point_jacobian_s *r,
    const fio___p384_point_jacobian_s *p,
    const fio___p384_point_affine_s *q) {
  fio___p384_fe_s t1, t2, t3, t4, t5, t6;
  fio___p384_point_jacobian_s generic, selected, candidate;

  fio___p384_fe_sqr(t1, p->z);            /* Z1^2 */
  fio___p384_fe_mul(t2, t1, p->z);        /* Z1^3 */
  fio___p384_fe_mul(t3, q->x, t1);        /* X2 Z1^2 */
  fio___p384_fe_mul(t4, q->y, t2);        /* Y2 Z1^3 */
  fio___p384_fe_sub(t3, t3, p->x);        /* H */
  fio___p384_fe_sub(t4, t4, p->y);        /* R */
  fio___p384_fe_sqr(t5, t3);              /* H^2 */
  fio___p384_fe_mul(t6, t5, t3);          /* H^3 */
  fio___p384_fe_mul(t1, p->x, t5);        /* X1 H^2 */
  fio___p384_fe_mul(t2, p->y, t6);        /* Y1 H^3 */
  fio___p384_fe_mul(generic.z, p->z, t3); /* Z3 = Z1 H */
  fio___p384_fe_sqr(generic.x, t4);       /* X3 = R^2 - H^3 - 2X1H^2 */
  fio___p384_fe_sub(generic.x, generic.x, t6);
  fio___p384_fe_sub(generic.x, generic.x, t1);
  fio___p384_fe_sub(generic.x, generic.x, t1);
  fio___p384_fe_sub(t1, t1, generic.x); /* X1 H^2 - X3 */
  fio___p384_fe_mul(generic.y, t4, t1); /* Y3 */
  fio___p384_fe_sub(generic.y, generic.y, t2);

  const uint64_t p_inf = fio___p384_fe_ct_is_zero_mask(p->z);
  const uint64_t h_zero = fio___p384_fe_ct_is_zero_mask(t3);
  const uint64_t r_zero = fio___p384_fe_ct_is_zero_mask(t4);
  const uint64_t equal = ~p_inf & h_zero & r_zero;
  const uint64_t opposite = ~p_inf & h_zero & ~r_zero;

  selected = generic;
  fio___p384_point_set_infinity(&candidate);
  fio___p384_point_cswap(&selected, &candidate, opposite & 1);
  fio___p384_point_double_ct(&candidate, p);
  fio___p384_point_cswap(&selected, &candidate, equal & 1);
  fio___p384_point_to_jacobian(&candidate, q);
  fio___p384_point_cswap(&selected, &candidate, p_inf & 1);
  *r = selected;

  fio_secure_zero(t1, sizeof(t1));
  fio_secure_zero(t2, sizeof(t2));
  fio_secure_zero(t3, sizeof(t3));
  fio_secure_zero(t4, sizeof(t4));
  fio_secure_zero(t5, sizeof(t5));
  fio_secure_zero(t6, sizeof(t6));
  fio_secure_zero(&generic, sizeof(generic));
  fio_secure_zero(&selected, sizeof(selected));
  fio_secure_zero(&candidate, sizeof(candidate));
}

/**
 * Scalar multiplication: r = k * P (constant-time Montgomery ladder).
 *
 * Use this for secret scalars (private keys, nonces). Ported from P-256:
 * R[0] = infinity, R[1] = P, then for every one of the 384 scalar bits (MSB
 * first, leading zeros included):
 *
 *   b = bit i of k
 *   cswap(R[0], R[1], b)
 *   D = 2 * R[0]
 *   R[1] = D + (b ? -P : P)     (equals old R[0] + old R[1])
 *   R[0] = D
 *   cswap(R[0], R[1], b)
 *
 * After the first cswap R[1] = R[0] + P (b = 0) or R[0] - P (b = 1), so the
 * ladder sum R[0] + R[1] is 2 * R[0] +/- P: one constant-time double and one
 * constant-time mixed add per bit, with the sign of P selected by masks. No
 * branch or memory index depends on k; the exceptional cases (infinity,
 * doubling) are handled by mask selection inside the point operations.
 */
FIO_SFUNC void fio___p384_point_mul(fio___p384_point_jacobian_s *r,
                                    const fio___p384_scalar_s k,
                                    const fio___p384_point_affine_s *p) {
  fio___p384_point_affine_s p_sel;
  fio___p384_fe_s p_neg_y;
  fio___p384_point_jacobian_s r0, r1, d;

  fio___p384_fe_neg(p_neg_y, p->y);
  fio___p384_fe_copy(p_sel.x, p->x);
  fio___p384_point_set_infinity(&r0);
  fio___p384_point_to_jacobian(&r1, p);

  for (int i = 383; i >= 0; --i) {
    const uint64_t b = (k[i >> 6] >> (i & 63)) & 1ULL;
    const uint64_t bmask = (uint64_t)0 - b;
    fio___p384_point_cswap(&r0, &r1, b);
    fio___p384_point_double_ct(&d, &r0);
    for (int j = 0; j < 6; ++j)
      p_sel.y[j] = fio_ct_mux64(bmask, p_neg_y[j], p->y[j]);
    fio___p384_point_add_mixed_ct(&r1, &d, &p_sel);
    r0 = d;
    fio___p384_point_cswap(&r0, &r1, b);
  }

  *r = r0;

  fio_secure_zero(&p_sel, sizeof(p_sel));
  fio_secure_zero(p_neg_y, sizeof(p_neg_y));
  fio_secure_zero(&r0, sizeof(r0));
  fio_secure_zero(&r1, sizeof(r1));
  fio_secure_zero(&d, sizeof(d));
}

/**
 * Scalar multiplication: r = k * P (variable-time double-and-add).
 *
 * NOT constant time: public scalars / points only (tests and diagnostics);
 * secret scalars must use fio___p384_point_mul.
 */
FIO_SFUNC void fio___p384_point_mul_vartime(
    fio___p384_point_jacobian_s *r,
    const fio___p384_scalar_s k,
    const fio___p384_point_affine_s *p) {
  fio___p384_point_set_infinity(r);

  /* Find the highest set bit */
  int start_bit = 383;
  while (start_bit >= 0) {
    int limb = start_bit / 64;
    int bit = start_bit % 64;
    if (k[limb] & (1ULL << bit))
      break;
    --start_bit;
  }

  if (start_bit < 0)
    return; /* k = 0 */

  /* Double-and-add from MSB to LSB */
  for (int i = start_bit; i >= 0; --i) {
    fio___p384_point_double(r, r);

    int limb = i / 64;
    int bit = i % 64;
    if (k[limb] & (1ULL << bit)) {
      fio___p384_point_add_mixed(r, r, p);
    }
  }
}

/**
 * Double scalar multiplication: r = u1*G + u2*Q
 * Uses Shamir's trick for efficiency.
 */
FIO_SFUNC void fio___p384_point_mul2(fio___p384_point_jacobian_s *r,
                                     const fio___p384_scalar_s u1,
                                     const fio___p384_scalar_s u2,
                                     const fio___p384_point_affine_s *q) {
  /* Base point G */
  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  /* Precompute G + Q */
  fio___p384_point_jacobian_s gpq;
  fio___p384_point_to_jacobian(&gpq, &g);
  fio___p384_point_add_mixed(&gpq, &gpq, q);
  /* G + Q is infinity iff Q == -G; adding infinity is then a no-op (never
   * use the (0,0) affine placeholder as a point). */
  const int gpq_inf = fio___p384_point_is_infinity(&gpq);
  fio___p384_point_affine_s gpq_affine;
  fio___p384_point_to_affine(&gpq_affine, &gpq);

  fio___p384_point_set_infinity(r);

  /* Find highest set bit in either scalar */
  int start_bit = 383;
  while (start_bit >= 0) {
    int limb = start_bit / 64;
    int bit = start_bit % 64;
    if ((u1[limb] & (1ULL << bit)) || (u2[limb] & (1ULL << bit)))
      break;
    --start_bit;
  }

  if (start_bit < 0)
    return;

  /* Shamir's trick: process both scalars simultaneously */
  for (int i = start_bit; i >= 0; --i) {
    fio___p384_point_double(r, r);

    int limb = i / 64;
    int bit = i % 64;
    int b1 = (u1[limb] >> bit) & 1;
    int b2 = (u2[limb] >> bit) & 1;

    if (b1 && b2) {
      if (!gpq_inf)
        fio___p384_point_add_mixed(r, r, &gpq_affine);
    } else if (b1) {
      fio___p384_point_add_mixed(r, r, &g);
    } else if (b2) {
      fio___p384_point_add_mixed(r, r, q);
    }
  }
}

/* *****************************************************************************
DER Signature Parsing
***************************************************************************** */

/**
 * Parse one strict DER INTEGER (short-form length, positive, minimal, at most
 * 48 value bytes), right-aligned into `out`. Advances `*pp`.
 *
 * Returns 0 on success, -1 on error.
 */
FIO_SFUNC int fio___p384_parse_der_integer(uint8_t out[48],
                                           const uint8_t **pp,
                                           const uint8_t *end) {
  const uint8_t *p = *pp;
  if ((size_t)(end - p) < 2 || p[0] != 0x02)
    return -1;
  size_t len = p[1];
  p += 2;
  if (!len || len > 49 || len > (size_t)(end - p))
    return -1;
  if (p[0] & 0x80) /* negative */
    return -1;
  if (p[0] == 0x00 && len > 1) {
    if (!(p[1] & 0x80)) /* non-minimal leading zero */
      return -1;
    ++p;
    --len;
  }
  if (len > 48)
    return -1;
  FIO_MEMSET(out, 0, 48);
  FIO_MEMCPY(out + (48 - len), p, len);
  *pp = p + len;
  return 0;
}

/**
 * Parse strict DER-encoded ECDSA signature for P-384.
 * Format: SEQUENCE { r INTEGER, s INTEGER }, with no trailing data.
 *
 * Intentionally strict (unlike the lenient `der.h`, which this does not use):
 * every mainstream signer emits canonical DER and mainstream verifiers reject
 * anything else, so leniency gains no interoperability; it would only let one
 * (r, s) have many encodings (CVE-2014-8275 style malleability, e.g. of
 * certificate fingerprints). The API reports no consumed length, so trailing
 * data is an error. Negative INTEGERs are different values, not encodings.
 *
 * Returns 0 on success, -1 on error.
 */
FIO_SFUNC int fio___p384_parse_der_signature(uint8_t r[48],
                                             uint8_t s[48],
                                             const uint8_t *sig,
                                             size_t sig_len) {
  /* max content is 2 * (2 + 49) = 102 bytes, so DER requires short form */
  if (!sig || sig_len < 8 || sig_len > 104)
    return -1;
  if (sig[0] != 0x30 || (size_t)sig[1] != sig_len - 2)
    return -1;
  const uint8_t *p = sig + 2;
  const uint8_t *end = sig + sig_len;
  if (fio___p384_parse_der_integer(r, &p, end) ||
      fio___p384_parse_der_integer(s, &p, end) || p != end)
    return -1;
  return 0;
}

/* *****************************************************************************
ECDSA Verification
***************************************************************************** */

SFUNC int fio_ecdsa_p384_verify_raw(const uint8_t r_bytes[48],
                                    const uint8_t s_bytes[48],
                                    const uint8_t msg_hash[48],
                                    const uint8_t pubkey_x[48],
                                    const uint8_t pubkey_y[48]) {
  fio___p384_scalar_s r, s, e;
  fio___p384_point_affine_s q;

  /* Load signature components */
  fio___p384_scalar_from_bytes(r, r_bytes);
  fio___p384_scalar_from_bytes(s, s_bytes);

  /* Verify r, s are in [1, n-1] */
  if (fio___p384_scalar_is_zero(r) || fio___p384_scalar_gte_n(r))
    return -1;
  if (fio___p384_scalar_is_zero(s) || fio___p384_scalar_gte_n(s))
    return -1;

  /* Load message hash as scalar e */
  fio___p384_scalar_from_bytes(e, msg_hash);

  /* Reduce e mod n if needed */
  if (fio___p384_scalar_gte_n(e)) {
    uint64_t borrow = 0;
    e[0] = fio_math_subc64(e[0], FIO___P384_N[0], 0, &borrow);
    e[1] = fio_math_subc64(e[1], FIO___P384_N[1], borrow, &borrow);
    e[2] = fio_math_subc64(e[2], FIO___P384_N[2], borrow, &borrow);
    e[3] = fio_math_subc64(e[3], FIO___P384_N[3], borrow, &borrow);
    e[4] = fio_math_subc64(e[4], FIO___P384_N[4], borrow, &borrow);
    e[5] = fio_math_subc64(e[5], FIO___P384_N[5], borrow, &borrow);
  }

  /* Load public key point Q (coordinates must be canonical: < p) */
  if (fio___p384_fe_from_bytes_canonical(q.x, pubkey_x) ||
      fio___p384_fe_from_bytes_canonical(q.y, pubkey_y))
    return -1;

  /* Verify Q is on the curve: y^2 = x^3 - 3x + b (mod p) */
  {
    fio___p384_fe_s y2, x3, t;

    /* y^2 */
    fio___p384_fe_sqr(y2, q.y);

    /* x^3 */
    fio___p384_fe_sqr(t, q.x);
    fio___p384_fe_mul(x3, t, q.x);

    /* x^3 - 3x */
    fio___p384_fe_sub(t, x3, q.x);
    fio___p384_fe_sub(t, t, q.x);
    fio___p384_fe_sub(t, t, q.x);

    /* x^3 - 3x + b */
    fio___p384_fe_add(t, t, FIO___P384_B);

    /* Check y^2 == x^3 - 3x + b */
    if (fio___p384_fe_eq(y2, t) != 0)
      return -1;
  }

  /* Compute w = s^(-1) mod n */
  fio___p384_scalar_s w;
  fio___p384_scalar_inv(w, s);

  /* Compute u1 = e * w mod n */
  fio___p384_scalar_s u1;
  fio___p384_scalar_mul(u1, e, w);

  /* Compute u2 = r * w mod n */
  fio___p384_scalar_s u2;
  fio___p384_scalar_mul(u2, r, w);

  /* Compute R = u1*G + u2*Q */
  fio___p384_point_jacobian_s R_jac;
  fio___p384_point_mul2(&R_jac, u1, u2, &q);

  /* If R is infinity, reject */
  if (fio___p384_point_is_infinity(&R_jac))
    return -1;

  /* Convert R to affine */
  fio___p384_point_affine_s R_aff;
  fio___p384_point_to_affine(&R_aff, &R_jac);

  /* Get R.x as bytes */
  uint8_t rx_bytes[48];
  fio___p384_fe_to_bytes(rx_bytes, R_aff.x);

  /* Load R.x as scalar and reduce mod n */
  fio___p384_scalar_s rx;
  fio___p384_scalar_from_bytes(rx, rx_bytes);
  while (fio___p384_scalar_gte_n(rx)) {
    uint64_t borrow = 0;
    rx[0] = fio_math_subc64(rx[0], FIO___P384_N[0], 0, &borrow);
    rx[1] = fio_math_subc64(rx[1], FIO___P384_N[1], borrow, &borrow);
    rx[2] = fio_math_subc64(rx[2], FIO___P384_N[2], borrow, &borrow);
    rx[3] = fio_math_subc64(rx[3], FIO___P384_N[3], borrow, &borrow);
    rx[4] = fio_math_subc64(rx[4], FIO___P384_N[4], borrow, &borrow);
    rx[5] = fio_math_subc64(rx[5], FIO___P384_N[5], borrow, &borrow);
  }

  /* Verify r == R.x mod n */
  if (r[0] != rx[0] || r[1] != rx[1] || r[2] != rx[2] || r[3] != rx[3] ||
      r[4] != rx[4] || r[5] != rx[5])
    return -1;

  return 0;
}

/**
 * Decompress a compressed P-384 point (49 bytes: 0x02/0x03 || x), SEC 1
 * section 2.3.4. Computes y from x using y^2 = x^3 - 3x + b and selects the
 * root whose parity matches the prefix.
 *
 * Rejects any other prefix (incl. the identity `0x00` and the hybrid forms
 * `0x06` / `0x07`, which RFC 5480 forbids), a non-canonical x (x >= p) and an
 * x with no curve point (y^2 is not a square). The result is on the curve.
 * (No point has y = 0: the group order is odd, so both prefixes are valid.)
 *
 * Public values only (not constant time).
 *
 * @return 0 on success, -1 on failure
 */
FIO_SFUNC int fio___p384_point_decompress(fio___p384_point_affine_s *p,
                                          const uint8_t compressed[49]) {
  /* (p+1)/4 in little-endian limbs: p = 3 (mod 4), so sqrt(a) = a^((p+1)/4) */
  static const uint64_t exp[6] = {
      0x0000000040000000ULL, /* exp[0] */
      0xBFFFFFFFC0000000ULL, /* exp[1] */
      0xFFFFFFFFFFFFFFFFULL, /* exp[2] */
      0xFFFFFFFFFFFFFFFFULL, /* exp[3] */
      0xFFFFFFFFFFFFFFFFULL, /* exp[4] */
      0x3FFFFFFFFFFFFFFFULL, /* exp[5] */
  };
  const uint8_t prefix = compressed[0];
  if (prefix != 0x02 && prefix != 0x03)
    return -1;

  /* Load x coordinate (must be canonical: x < p) */
  if (fio___p384_fe_from_bytes_canonical(p->x, compressed + 1))
    return -1;

  /* y^2 = x^3 - 3x + b */
  fio___p384_fe_s y2, t, y, base;
  fio___p384_fe_sqr(t, p->x);
  fio___p384_fe_mul(t, t, p->x);
  fio___p384_fe_sub(t, t, p->x);
  fio___p384_fe_sub(t, t, p->x);
  fio___p384_fe_sub(t, t, p->x);
  fio___p384_fe_add(y2, t, FIO___P384_B);

  /* y = y2^((p+1)/4): square-and-multiply, LSB to MSB */
  fio___p384_fe_one(y);
  fio___p384_fe_copy(base, y2);
  for (int i = 0; i < 6; ++i) {
    uint64_t bits = exp[i];
    for (int j = 0; j < 64; ++j) {
      if (bits & 1)
        fio___p384_fe_mul(y, y, base);
      fio___p384_fe_sqr(base, base);
      bits >>= 1;
    }
  }

  /* y^2 must equal y2, otherwise x is not on the curve */
  fio___p384_fe_sqr(t, y);
  if (fio___p384_fe_eq(t, y2) != 0)
    return -1;

  /* Select the root with the parity requested by the prefix */
  if ((y[0] & 1) != (uint64_t)(prefix & 1))
    fio___p384_fe_neg(p->y, y);
  else
    fio___p384_fe_copy(p->y, y);
  return 0;
}

SFUNC int fio_ecdsa_p384_verify(const uint8_t *sig,
                                size_t sig_len,
                                const uint8_t *msg_hash,
                                const uint8_t *pubkey,
                                size_t pubkey_len) {
  if (!sig || !msg_hash || !pubkey)
    return -1;

  /* SEC 1 public key: 0x04 || x || y (97 bytes) or 0x02/0x03 || x (49 bytes).
   * verify_raw checks the coordinates are canonical and on the curve. */
  const uint8_t *qx, *qy;
  uint8_t xy[96];
  if (pubkey_len == 97 && pubkey[0] == 0x04) {
    qx = pubkey + 1;
    qy = pubkey + 49;
  } else if (pubkey_len == 49) {
    fio___p384_point_affine_s q;
    if (fio___p384_point_decompress(&q, pubkey))
      return -1;
    fio___p384_fe_to_bytes(xy, q.x);
    fio___p384_fe_to_bytes(xy + 48, q.y);
    qx = xy;
    qy = xy + 48;
  } else {
    return -1;
  }

  /* Parse DER signature */
  uint8_t r[48], s[48];
  if (fio___p384_parse_der_signature(r, s, sig, sig_len) != 0)
    return -1;

  /* Verify */
  return fio_ecdsa_p384_verify_raw(r, s, msg_hash, qx, qy);
}

/* *****************************************************************************
ECDSA P-384 Signing Implementation
***************************************************************************** */

/**
 * Encode a 48-byte integer as DER INTEGER (handles leading zero for positive).
 * Returns number of bytes written (at most 51). Public output values only.
 */
FIO_SFUNC size_t fio___p384_encode_der_integer(uint8_t *out,
                                               const uint8_t val[48]) {
  /* Skip leading zeros */
  size_t start = 0;
  while (start < 47 && val[start] == 0)
    ++start;

  /* Need leading zero if high bit is set (to keep positive) */
  int need_zero = (val[start] & 0x80) != 0;
  size_t len = 48 - start + (need_zero ? 1 : 0);

  out[0] = 0x02; /* INTEGER tag */
  out[1] = (uint8_t)len;
  size_t offset = 2;
  if (need_zero)
    out[offset++] = 0x00;
  FIO_MEMCPY(out + offset, val + start, 48 - start);

  return 2 + len;
}

/** Store a scalar as 48 big-endian bytes. */
FIO_IFUNC void fio___p384_scalar_to_bytes(uint8_t out[48],
                                          const fio___p384_scalar_s a) {
  fio___p384_fe_to_bytes(out, a);
}

SFUNC int fio_ecdsa_p384_sign(uint8_t *sig,
                              size_t *sig_len,
                              size_t sig_capacity,
                              const uint8_t msg_hash[48],
                              const uint8_t secret_key[48]) {
  if (!sig || !sig_len || !msg_hash || !secret_key || sig_capacity < 104)
    return -1;

  int rc = -1;
  fio___p384_scalar_s d = {0}, e = {0}, k = {0}, k_inv = {0};
  fio___p384_scalar_s r_scalar = {0}, s_scalar = {0}, tmp = {0};
  fio___p384_point_affine_s g = {0}, R_aff = {0};
  fio___p384_point_jacobian_s R_jac = {0};
  uint8_t k_bytes[48] = {0};
  uint8_t r_bytes[48] = {0}, s_bytes[48] = {0};
  uint8_t r_der[51] = {0}, s_der[51] = {0};

#define FIO___P384_SIGN_CLEAR_ATTEMPT()                                        \
  do {                                                                         \
    fio_secure_zero(k, sizeof(k));                                             \
    fio_secure_zero(k_inv, sizeof(k_inv));                                     \
    fio_secure_zero(r_scalar, sizeof(r_scalar));                               \
    fio_secure_zero(s_scalar, sizeof(s_scalar));                               \
    fio_secure_zero(tmp, sizeof(tmp));                                         \
    fio_secure_zero(k_bytes, sizeof(k_bytes));                                 \
    fio_secure_zero(r_bytes, sizeof(r_bytes));                                 \
    fio_secure_zero(s_bytes, sizeof(s_bytes));                                 \
    fio_secure_zero(r_der, sizeof(r_der));                                     \
    fio_secure_zero(s_der, sizeof(s_der));                                     \
    fio_secure_zero(&R_jac, sizeof(R_jac));                                    \
    fio_secure_zero(&R_aff, sizeof(R_aff));                                    \
  } while (0)

  /* Load secret key as scalar d */
  fio___p384_scalar_from_bytes(d, secret_key);

  /* Validate d: 0 < d < n */
  if (fio___p384_scalar_ct_is_zero(d) | fio___p384_scalar_ct_gte_n(d))
    goto cleanup;

  /* Load message hash as scalar e (public); reduce mod n if needed */
  fio___p384_scalar_from_bytes(e, msg_hash);
  fio___p384_scalar_sub_n_if(e, (uint64_t)fio___p384_scalar_gte_n(e));

  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  /* Generate random k and compute signature. */
  for (int attempts = 0; attempts < 100; ++attempts) {
    /* Generate random nonce k in the valid scalar range 0 < k < n.
     * Rejections consume an attempt, bounding total draws (stuck RNG). */
    if (fio_rand_bytes_secure(k_bytes, 48))
      fio_rand_bytes(k_bytes, 48);
    fio___p384_scalar_from_bytes(k, k_bytes);
    if (fio___p384_scalar_ct_is_zero(k) | fio___p384_scalar_ct_gte_n(k)) {
      FIO___P384_SIGN_CLEAR_ATTEMPT();
      continue;
    }

    /* Compute R = k * G (constant-time ladder) */
    fio___p384_point_mul(&R_jac, k, &g);

    if (fio___p384_point_is_infinity(&R_jac)) {
      FIO___P384_SIGN_CLEAR_ATTEMPT();
      continue;
    }

    /* Convert R to affine */
    fio___p384_point_to_affine(&R_aff, &R_jac);

    /* r = R.x mod n */
    fio___p384_fe_to_bytes(r_bytes, R_aff.x);
    fio___p384_scalar_from_bytes(r_scalar, r_bytes);

    /* Reduce r mod n if needed. R.x < p < 2n, so one subtract is enough. */
    fio___p384_scalar_sub_n_if(r_scalar,
                               (uint64_t)fio___p384_scalar_ct_gte_n(r_scalar));

    if (fio___p384_scalar_ct_is_zero(r_scalar)) {
      FIO___P384_SIGN_CLEAR_ATTEMPT();
      continue;
    }

    /* Compute s = k^(-1) * (e + r*d) mod n */
    fio___p384_scalar_mul(tmp, r_scalar, d);

    uint64_t carry = 0;
    tmp[0] = fio_math_addc64(tmp[0], e[0], 0, &carry);
    tmp[1] = fio_math_addc64(tmp[1], e[1], carry, &carry);
    tmp[2] = fio_math_addc64(tmp[2], e[2], carry, &carry);
    tmp[3] = fio_math_addc64(tmp[3], e[3], carry, &carry);
    tmp[4] = fio_math_addc64(tmp[4], e[4], carry, &carry);
    tmp[5] = fio_math_addc64(tmp[5], e[5], carry, &carry);

    /* e and r*d are each < n, so one subtract is enough. */
    fio___p384_scalar_sub_n_if(tmp,
                               carry |
                                   (uint64_t)fio___p384_scalar_ct_gte_n(tmp));

    fio___p384_scalar_inv(k_inv, k);
    fio___p384_scalar_mul(s_scalar, k_inv, tmp);

    if (fio___p384_scalar_ct_is_zero(s_scalar)) {
      FIO___P384_SIGN_CLEAR_ATTEMPT();
      continue;
    }

    fio___p384_scalar_to_bytes(r_bytes, r_scalar);
    fio___p384_scalar_to_bytes(s_bytes, s_scalar);

    size_t r_der_len = fio___p384_encode_der_integer(r_der, r_bytes);
    size_t s_der_len = fio___p384_encode_der_integer(s_der, s_bytes);
    size_t seq_content_len = r_der_len + s_der_len;
    size_t total_len = 2 + seq_content_len;

    if (total_len > sig_capacity)
      goto cleanup;

    sig[0] = 0x30;
    sig[1] = (uint8_t)seq_content_len; /* <= 102: short form */
    FIO_MEMCPY(sig + 2, r_der, r_der_len);
    FIO_MEMCPY(sig + 2 + r_der_len, s_der, s_der_len);
    *sig_len = total_len;
    rc = 0;
    goto cleanup;
  }

cleanup:
  fio_secure_zero(d, sizeof(d));
  fio_secure_zero(e, sizeof(e));
  fio_secure_zero(&g, sizeof(g));
  FIO___P384_SIGN_CLEAR_ATTEMPT();
#undef FIO___P384_SIGN_CLEAR_ATTEMPT
  return rc;
}

/* *****************************************************************************
P-384 Key Derivation / Generation
***************************************************************************** */

SFUNC int fio_p384_public_key(uint8_t public_key[97],
                              const uint8_t secret_key[48]) {
  if (!public_key)
    return -1;
  if (!secret_key) {
    FIO_MEMSET(public_key, 0, 97);
    return -1;
  }

  int rc = -1;
  fio___p384_scalar_s k = {0};
  fio___p384_point_affine_s g = {0}, pub_aff = {0};
  fio___p384_point_jacobian_s pub_jac = {0};

  /* Load and validate the secret scalar: 0 < k < n */
  fio___p384_scalar_from_bytes(k, secret_key);
  if (fio___p384_scalar_ct_is_zero(k) | fio___p384_scalar_ct_gte_n(k)) {
    FIO_MEMSET(public_key, 0, 97);
    goto cleanup;
  }

  /* Compute public key = k * G (constant-time ladder) */
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);
  fio___p384_point_mul(&pub_jac, k, &g);
  fio___p384_point_to_affine(&pub_aff, &pub_jac);

  /* Serialize: 0x04 || x || y */
  public_key[0] = 0x04;
  fio___p384_fe_to_bytes(public_key + 1, pub_aff.x);
  fio___p384_fe_to_bytes(public_key + 49, pub_aff.y);
  rc = 0;

cleanup:
  fio_secure_zero(k, sizeof(k));
  fio_secure_zero(&pub_jac, sizeof(pub_jac));
  fio_secure_zero(&pub_aff, sizeof(pub_aff));
  return rc;
}

SFUNC int fio_p384_keypair(uint8_t secret_key[48], uint8_t public_key[97]) {
  if (!secret_key || !public_key)
    return -1;

  /* Draw random scalars until 0 < k < n (bounded number of draws). */
  for (int attempts = 0; attempts < 100; ++attempts) {
    if (fio_rand_bytes_secure(secret_key, 48))
      fio_rand_bytes(secret_key, 48);
    if (!fio_p384_public_key(public_key, secret_key))
      return 0;
  }
  fio_secure_zero(secret_key, 48);
  return -1;
}

/* *****************************************************************************
Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_P384
#endif /* FIO_P384 */
