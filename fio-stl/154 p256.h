/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_P256               /* Development inclusion - ignore line */
#define FIO_SHA2               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                    Elliptic Curve Cryptography: ECDSA P-256 (secp256r1)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_P256) && !defined(H___FIO_P256___H)
#define H___FIO_P256___H

/* *****************************************************************************
P-256 (secp256r1) ECDSA Module

This module provides ECDSA signature verification for the P-256 curve,
primarily for TLS 1.3 certificate chain validation.

P-256 curve parameters (NIST FIPS 186-4):
- Prime p = 2^256 - 2^224 + 2^192 + 2^96 - 1
- Order n = 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
- Curve equation: y² = x³ - 3x + b (mod p)

**Note**: This implementation has not been audited. Use at your own risk.
***************************************************************************** */

/* *****************************************************************************
ECDSA P-256 Verification API
***************************************************************************** */

/**
 * Verifies an ECDSA P-256 signature.
 *
 * @param sig DER-encoded signature (SEQUENCE { r INTEGER, s INTEGER })
 * @param sig_len Length of signature in bytes
 * @param msg_hash SHA-256 hash of the message (32 bytes)
 * @param pubkey Uncompressed public key (65 bytes: 0x04 || x || y)
 * @param pubkey_len Length of public key (must be 65)
 * @return 0 on success (valid signature), -1 on failure (invalid signature)
 */
SFUNC int fio_ecdsa_p256_verify(const uint8_t *sig,
                                size_t sig_len,
                                const uint8_t *msg_hash,
                                const uint8_t *pubkey,
                                size_t pubkey_len);

/**
 * Signs a message hash using ECDSA P-256.
 *
 * @param sig Output: DER-encoded signature (max 72 bytes)
 * @param sig_len Output: actual signature length
 * @param sig_capacity Capacity of signature buffer (should be >= 72)
 * @param msg_hash SHA-256 hash of the message (32 bytes)
 * @param secret_key 32-byte secret key (scalar, big-endian)
 * @return 0 on success, -1 on failure
 */
SFUNC int fio_ecdsa_p256_sign(uint8_t *sig,
                              size_t *sig_len,
                              size_t sig_capacity,
                              const uint8_t msg_hash[32],
                              const uint8_t secret_key[32]);

/**
 * Verifies an ECDSA P-256 signature with raw r,s values.
 *
 * @param r The r component of the signature (32 bytes, big-endian)
 * @param s The s component of the signature (32 bytes, big-endian)
 * @param msg_hash SHA-256 hash of the message (32 bytes)
 * @param pubkey_x X coordinate of public key (32 bytes, big-endian)
 * @param pubkey_y Y coordinate of public key (32 bytes, big-endian)
 * @return 0 on success (valid signature), -1 on failure (invalid signature)
 */
SFUNC int fio_ecdsa_p256_verify_raw(const uint8_t r[32],
                                    const uint8_t s[32],
                                    const uint8_t msg_hash[32],
                                    const uint8_t pubkey_x[32],
                                    const uint8_t pubkey_y[32]);

/* *****************************************************************************
P-256 ECDHE (Elliptic Curve Diffie-Hellman) API
***************************************************************************** */

/**
 * Generates a P-256 keypair for ECDHE key exchange.
 *
 * @param secret_key Output: 32-byte secret key (scalar, big-endian)
 * @param public_key Output: 65-byte uncompressed public key (0x04 || x || y)
 * @return 0 on success, -1 on failure
 */
SFUNC int fio_p256_keypair(uint8_t secret_key[32], uint8_t public_key[65]);

/**
 * Computes P-256 ECDH shared secret.
 *
 * @param shared_secret Output: 32-byte shared secret (x-coordinate of result)
 * @param secret_key 32-byte secret key (scalar, big-endian)
 * @param their_public_key Their public key (uncompressed 65 bytes, or
 *                         compressed 33 bytes)
 * @param their_public_key_len Length of their public key (33 or 65)
 * @return 0 on success, -1 on failure (invalid key, point at infinity, etc.)
 */
SFUNC int fio_p256_shared_secret(uint8_t shared_secret[32],
                                 const uint8_t secret_key[32],
                                 const uint8_t *their_public_key,
                                 size_t their_public_key_len);

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
P-256 Field Arithmetic (mod p)

Prime p = 2^256 - 2^224 + 2^192 + 2^96 - 1
       = 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF

We use 4 x 64-bit limbs in little-endian order.
***************************************************************************** */

/* P-256 prime p in little-endian limbs */
static const uint64_t FIO___P256_P[4] = {
    0xFFFFFFFFFFFFFFFFULL, /* p[0] */
    0x00000000FFFFFFFFULL, /* p[1] */
    0x0000000000000000ULL, /* p[2] */
    0xFFFFFFFF00000001ULL, /* p[3] */
};

/* P-256 curve order n in little-endian limbs */
static const uint64_t FIO___P256_N[4] = {
    0xF3B9CAC2FC632551ULL, /* n[0] */
    0xBCE6FAADA7179E84ULL, /* n[1] */
    0xFFFFFFFFFFFFFFFFULL, /* n[2] */
    0xFFFFFFFF00000000ULL, /* n[3] */
};

/* P-256 curve parameter b in little-endian limbs */
static const uint64_t FIO___P256_B[4] = {
    0x3BCE3C3E27D2604BULL, /* b[0] */
    0x651D06B0CC53B0F6ULL, /* b[1] */
    0xB3EBBD55769886BCULL, /* b[2] */
    0x5AC635D8AA3A93E7ULL, /* b[3] */
};

/* P-256 base point G x-coordinate in little-endian limbs */
static const uint64_t FIO___P256_GX[4] = {
    0xF4A13945D898C296ULL, /* Gx[0] */
    0x77037D812DEB33A0ULL, /* Gx[1] */
    0xF8BCE6E563A440F2ULL, /* Gx[2] */
    0x6B17D1F2E12C4247ULL, /* Gx[3] */
};

/* P-256 base point G y-coordinate in little-endian limbs */
static const uint64_t FIO___P256_GY[4] = {
    0xCBB6406837BF51F5ULL, /* Gy[0] */
    0x2BCE33576B315ECEULL, /* Gy[1] */
    0x8EE7EB4A7C0F9E16ULL, /* Gy[2] */
    0x4FE342E2FE1A7F9BULL, /* Gy[3] */
};

/* Field element type: 4 x 64-bit limbs in little-endian */
typedef uint64_t fio___p256_fe_s[4];

/* Point in affine coordinates */
typedef struct {
  fio___p256_fe_s x;
  fio___p256_fe_s y;
} fio___p256_point_affine_s;

/* Point in Jacobian coordinates (x = X/Z², y = Y/Z³) */
typedef struct {
  fio___p256_fe_s x;
  fio___p256_fe_s y;
  fio___p256_fe_s z;
} fio___p256_point_jacobian_s;

/* *****************************************************************************
Field Element Operations (mod p)
***************************************************************************** */

/** Copy field element: dest = src */
FIO_IFUNC void fio___p256_fe_copy(fio___p256_fe_s dest,
                                  const fio___p256_fe_s src) {
  dest[0] = src[0];
  dest[1] = src[1];
  dest[2] = src[2];
  dest[3] = src[3];
}

/** Set field element to zero */
FIO_IFUNC void fio___p256_fe_zero(fio___p256_fe_s r) {
  r[0] = 0;
  r[1] = 0;
  r[2] = 0;
  r[3] = 0;
}

/** Set field element to one */
FIO_IFUNC void fio___p256_fe_one(fio___p256_fe_s r) {
  r[0] = 1;
  r[1] = 0;
  r[2] = 0;
  r[3] = 0;
}

/** Public-value check: returns 1 if field element is zero. */
FIO_IFUNC int fio___p256_fe_is_zero(const fio___p256_fe_s a) {
  return (a[0] | a[1] | a[2] | a[3]) == 0;
}

/** Secret-safe check: returns 1 if field element is zero. */
FIO_IFUNC int fio___p256_fe_ct_is_zero(const fio___p256_fe_s a) {
  uint64_t diff = a[0] | a[1] | a[2] | a[3];
  return (int)fio_ct_false(diff);
}

/** Public-value compare: returns 0 if equal. */
FIO_IFUNC int fio___p256_fe_eq(const fio___p256_fe_s a,
                               const fio___p256_fe_s b) {
  uint64_t diff = (a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2]) | (a[3] ^ b[3]);
  return diff == 0 ? 0 : 1;
}

/** Secret-safe compare: returns 1 if equal. */
FIO_IFUNC int fio___p256_fe_ct_is_eq(const fio___p256_fe_s a,
                                     const fio___p256_fe_s b) {
  uint64_t diff = (a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2]) | (a[3] ^ b[3]);
  return (int)fio_ct_false(diff);
}

/** Load 32-byte big-endian number into field element */
FIO_IFUNC void fio___p256_fe_from_bytes(fio___p256_fe_s r,
                                        const uint8_t in[32]) {
  r[3] = fio_buf2u64_be(in);
  r[2] = fio_buf2u64_be(in + 8);
  r[1] = fio_buf2u64_be(in + 16);
  r[0] = fio_buf2u64_be(in + 24);
}

/** Store field element to 32-byte big-endian output */
FIO_IFUNC void fio___p256_fe_to_bytes(uint8_t out[32],
                                      const fio___p256_fe_s a) {
  fio_u2buf64_be(out, a[3]);
  fio_u2buf64_be(out + 8, a[2]);
  fio_u2buf64_be(out + 16, a[1]);
  fio_u2buf64_be(out + 24, a[0]);
}

/** Field addition: r = a + b mod p */
FIO_IFUNC void fio___p256_fe_add(fio___p256_fe_s r,
                                 const fio___p256_fe_s a,
                                 const fio___p256_fe_s b) {
  uint64_t c = 0;
  uint64_t t[4];

  /* Add a + b */
  t[0] = fio_math_addc64(a[0], b[0], 0, &c);
  t[1] = fio_math_addc64(a[1], b[1], c, &c);
  t[2] = fio_math_addc64(a[2], b[2], c, &c);
  t[3] = fio_math_addc64(a[3], b[3], c, &c);

  /* Reduce mod p if needed: subtract p if result >= p */
  uint64_t borrow = 0;
  uint64_t s[4];
  s[0] = fio_math_subc64(t[0], FIO___P256_P[0], 0, &borrow);
  s[1] = fio_math_subc64(t[1], FIO___P256_P[1], borrow, &borrow);
  s[2] = fio_math_subc64(t[2], FIO___P256_P[2], borrow, &borrow);
  s[3] = fio_math_subc64(t[3], FIO___P256_P[3], borrow, &borrow);

  /* If carry from addition or no borrow from subtraction, use subtracted value
   */
  uint64_t mask = (uint64_t)0 - (c | (borrow ^ 1));
  r[0] = (s[0] & mask) | (t[0] & ~mask);
  r[1] = (s[1] & mask) | (t[1] & ~mask);
  r[2] = (s[2] & mask) | (t[2] & ~mask);
  r[3] = (s[3] & mask) | (t[3] & ~mask);
}

/** Field subtraction: r = a - b mod p */
FIO_IFUNC void fio___p256_fe_sub(fio___p256_fe_s r,
                                 const fio___p256_fe_s a,
                                 const fio___p256_fe_s b) {
  uint64_t borrow = 0;
  uint64_t t[4];

  /* Subtract a - b */
  t[0] = fio_math_subc64(a[0], b[0], 0, &borrow);
  t[1] = fio_math_subc64(a[1], b[1], borrow, &borrow);
  t[2] = fio_math_subc64(a[2], b[2], borrow, &borrow);
  t[3] = fio_math_subc64(a[3], b[3], borrow, &borrow);

  /* If borrow, add p back */
  uint64_t c = 0;
  uint64_t mask = (uint64_t)0 - borrow;
  uint64_t s[4];
  s[0] = fio_math_addc64(t[0], FIO___P256_P[0] & mask, 0, &c);
  s[1] = fio_math_addc64(t[1], FIO___P256_P[1] & mask, c, &c);
  s[2] = fio_math_addc64(t[2], FIO___P256_P[2] & mask, c, &c);
  s[3] = fio_math_addc64(t[3], FIO___P256_P[3] & mask, c, &c);

  r[0] = s[0];
  r[1] = s[1];
  r[2] = s[2];
  r[3] = s[3];
}

/** Field negation: r = -a mod p */
FIO_IFUNC void fio___p256_fe_neg(fio___p256_fe_s r, const fio___p256_fe_s a) {
  fio___p256_fe_s zero = {0};
  fio___p256_fe_sub(r, zero, a);
}

/**
 * P-256 reduction using the NIST fast reduction formula.
 *
 * p = 2^256 - 2^224 + 2^192 + 2^96 - 1
 *
 * For a 512-bit number T, we use the NIST formula with 32-bit words.
 * T = (c15,...,c0) where each ci is 32 bits.
 * Note: NIST notation is big-endian (c7 is MSB of low 256 bits)
 *
 * The result is: T mod p = s1 + 2*s2 + 2*s3 + s4 + s5 - s6 - s7 - s8 - s9
 * where each si is a 256-bit value constructed from the 32-bit words.
 *
 * Reference: NIST FIPS 186-4, Section D.2.3
 */
FIO_SFUNC void fio___p256_fe_reduce(fio___p256_fe_s r, const uint64_t t[8]) {
  /* Extract 32-bit words from 512-bit input (little-endian) */
  uint32_t c[16];
  for (int i = 0; i < 8; ++i) {
    c[2 * i] = (uint32_t)t[i];
    c[2 * i + 1] = (uint32_t)(t[i] >> 32);
  }

  /* Use 64-bit accumulators for each 32-bit position to handle carries.
   * acc[0] is LSB (c0 position), acc[7] is MSB (c7 position).
   * NIST notation: (c7, c6, c5, c4, c3, c2, c1, c0) means
   * c7 is at position 7 (MSB), c0 is at position 0 (LSB). */
  int64_t acc[8] = {0};

  /* s1 = (c7, c6, c5, c4, c3, c2, c1, c0) - the low 256 bits */
  acc[0] += c[0];
  acc[1] += c[1];
  acc[2] += c[2];
  acc[3] += c[3];
  acc[4] += c[4];
  acc[5] += c[5];
  acc[6] += c[6];
  acc[7] += c[7];

  /* s2 = (c15, c14, c13, c12, c11, 0, 0, 0) - add twice
   * Position mapping: c15->acc[7], c14->acc[6], c13->acc[5], c12->acc[4],
   * c11->acc[3] */
  acc[3] += 2 * (int64_t)c[11];
  acc[4] += 2 * (int64_t)c[12];
  acc[5] += 2 * (int64_t)c[13];
  acc[6] += 2 * (int64_t)c[14];
  acc[7] += 2 * (int64_t)c[15];

  /* s3 = (0, c15, c14, c13, c12, 0, 0, 0) - add twice
   * Position mapping: c15->acc[6], c14->acc[5], c13->acc[4], c12->acc[3] */
  acc[3] += 2 * (int64_t)c[12];
  acc[4] += 2 * (int64_t)c[13];
  acc[5] += 2 * (int64_t)c[14];
  acc[6] += 2 * (int64_t)c[15];

  /* s4 = (c15, c14, 0, 0, 0, c10, c9, c8)
   * Position mapping: c15->acc[7], c14->acc[6], c10->acc[2], c9->acc[1],
   * c8->acc[0] */
  acc[0] += c[8];
  acc[1] += c[9];
  acc[2] += c[10];
  acc[6] += c[14];
  acc[7] += c[15];

  /* s5 = (c8, c13, c15, c14, c13, c11, c10, c9)
   * Position mapping: c8->acc[7], c13->acc[6], c15->acc[5], c14->acc[4],
   * c13->acc[3], c11->acc[2], c10->acc[1], c9->acc[0] */
  acc[0] += c[9];
  acc[1] += c[10];
  acc[2] += c[11];
  acc[3] += c[13];
  acc[4] += c[14];
  acc[5] += c[15];
  acc[6] += c[13];
  acc[7] += c[8];

  /* s6 = (c10, c8, 0, 0, 0, c13, c12, c11) - subtract
   * Position mapping: c10->acc[7], c8->acc[6], c13->acc[2], c12->acc[1],
   * c11->acc[0] */
  acc[0] -= c[11];
  acc[1] -= c[12];
  acc[2] -= c[13];
  acc[6] -= c[8];
  acc[7] -= c[10];

  /* s7 = (c11, c9, 0, 0, c15, c14, c13, c12) - subtract
   * Position mapping: c11->acc[7], c9->acc[6], c15->acc[3], c14->acc[2],
   * c13->acc[1], c12->acc[0] */
  acc[0] -= c[12];
  acc[1] -= c[13];
  acc[2] -= c[14];
  acc[3] -= c[15];
  acc[6] -= c[9];
  acc[7] -= c[11];

  /* s8 = (c12, 0, c10, c9, c8, c15, c14, c13) - subtract
   * Position mapping: c12->acc[7], c10->acc[5], c9->acc[4], c8->acc[3],
   * c15->acc[2], c14->acc[1], c13->acc[0] */
  acc[0] -= c[13];
  acc[1] -= c[14];
  acc[2] -= c[15];
  acc[3] -= c[8];
  acc[4] -= c[9];
  acc[5] -= c[10];
  acc[7] -= c[12];

  /* s9 = (c13, 0, c11, c10, c9, 0, c15, c14) - subtract
   * Position mapping: c13->acc[7], c11->acc[5], c10->acc[4], c9->acc[3],
   * c15->acc[1], c14->acc[0] */
  acc[0] -= c[14];
  acc[1] -= c[15];
  acc[3] -= c[9];
  acc[4] -= c[10];
  acc[5] -= c[11];
  acc[7] -= c[13];

  /* Carry propagation with signed arithmetic.
   * For signed right shift, we need to handle negative values properly.
   * The carry is the arithmetic right shift by 32 bits. */
  for (int i = 0; i < 7; ++i) {
    /* Arithmetic right shift for signed carry */
    int64_t carry = acc[i] >> 32;
    /* Keep only the low 32 bits (may be negative representation) */
    acc[i] = acc[i] & 0xFFFFFFFFLL;
    acc[i + 1] += carry;
  }

  /* Handle overflow/underflow in the top limb.
   * acc[7] may be negative or > 2^32.
   * We reduce by adding/subtracting multiples of p.
   *
   * p = 2^256 - 2^224 + 2^192 + 2^96 - 1
   * In 32-bit words (little-endian): p = (0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
   * 0, 0, 0, 1, 0xFFFFFFFF) That is: p[0]=0xFFFFFFFF, p[1]=0xFFFFFFFF,
   * p[2]=0xFFFFFFFF, p[3]=0, p[4]=0, p[5]=0, p[6]=1, p[7]=0xFFFFFFFF
   *
   * When we add p: acc[0]+=0xFFFFFFFF, acc[1]+=0xFFFFFFFF, acc[2]+=0xFFFFFFFF,
   *                acc[6]+=1, acc[7]+=0xFFFFFFFF
   * When we subtract p: acc[0]-=0xFFFFFFFF, acc[1]-=0xFFFFFFFF,
   * acc[2]-=0xFFFFFFFF, acc[6]-=1, acc[7]-=0xFFFFFFFF
   */

  /* Reduce: perform exactly 4 conditional add/subtract passes of p.
   * After the initial carry propagation the NIST formula guarantees acc[7]
   * is in the range [-4, 4] * 0xFFFFFFFF, so 4 passes are always sufficient.
   * Each pass is branchless: we derive a signed direction from acc[7] and
   * apply the corresponding multiple of p without branching.
   *
   * p in 32-bit words (little-endian):
   *   acc[0..2] += 0xFFFFFFFF, acc[3..5] += 0, acc[6] += 1, acc[7] +=
   * 0xFFFFFFFF Subtracting p negates all of the above.
   */
  for (int pass = 0; pass < 4; ++pass) {
    /* neg_mask: all-ones (-1) if acc[7] < 0, else 0 */
    int64_t neg_mask = acc[7] >> 63;
    /* ovf_mask: all-ones (-1) if acc[7] >= 2^32, else 0.
     * (acc[7] - 2^32) is negative iff acc[7] < 2^32.
     * ~neg_mask ensures ovf_mask=0 when acc[7] < 0 (neg_mask handles that). */
    int64_t ovf_mask = ~neg_mask & ~((acc[7] - (int64_t)0x100000000LL) >> 63);

    /* adj: +0xFFFFFFFF if adding p, -0xFFFFFFFF if subtracting p, 0 otherwise
     */
    int64_t adj = (neg_mask & (int64_t)0xFFFFFFFFLL) |
                  (ovf_mask & -(int64_t)0xFFFFFFFFLL);
    /* adj6: +1 if adding p, -1 if subtracting p, 0 otherwise */
    int64_t adj6 = (neg_mask & 1LL) | (ovf_mask & -1LL);

    acc[0] += adj;
    acc[1] += adj;
    acc[2] += adj;
    acc[6] += adj6;
    acc[7] += adj;

    /* Re-propagate carries */
    for (int i = 0; i < 7; ++i) {
      int64_t carry = acc[i] >> 32;
      acc[i] = acc[i] & 0xFFFFFFFFLL;
      acc[i + 1] += carry;
    }
  }

  /* Convert back to 64-bit limbs */
  uint64_t res[4];
  res[0] = ((uint64_t)(uint32_t)acc[1] << 32) | (uint32_t)acc[0];
  res[1] = ((uint64_t)(uint32_t)acc[3] << 32) | (uint32_t)acc[2];
  res[2] = ((uint64_t)(uint32_t)acc[5] << 32) | (uint32_t)acc[4];
  res[3] = ((uint64_t)(uint32_t)acc[7] << 32) | (uint32_t)acc[6];

  /* Final reduction: subtract p at most twice (branchless).
   * After the pass above, res < 2p, so at most one subtraction is needed.
   * We do two passes for safety, each branchless. */
  for (int iter = 0; iter < 2; ++iter) {
    uint64_t borrow = 0;
    uint64_t sub[4];
    sub[0] = fio_math_subc64(res[0], FIO___P256_P[0], 0, &borrow);
    sub[1] = fio_math_subc64(res[1], FIO___P256_P[1], borrow, &borrow);
    sub[2] = fio_math_subc64(res[2], FIO___P256_P[2], borrow, &borrow);
    sub[3] = fio_math_subc64(res[3], FIO___P256_P[3], borrow, &borrow);

    /* If no borrow (result >= p), use subtracted value — branchless select */
    uint64_t keep = (uint64_t)0 - borrow; /* 0 if res>=p, ~0 if res<p */
    res[0] = (res[0] & keep) | (sub[0] & ~keep);
    res[1] = (res[1] & keep) | (sub[1] & ~keep);
    res[2] = (res[2] & keep) | (sub[2] & ~keep);
    res[3] = (res[3] & keep) | (sub[3] & ~keep);
  }

  r[0] = res[0];
  r[1] = res[1];
  r[2] = res[2];
  r[3] = res[3];
}

/** Field multiplication: r = a * b mod p */
FIO_SFUNC void fio___p256_fe_mul(fio___p256_fe_s r,
                                 const fio___p256_fe_s a,
                                 const fio___p256_fe_s b) {
  uint64_t t[8] = {0};

  /* Schoolbook multiplication to get 512-bit product */
  for (int i = 0; i < 4; ++i) {
    uint64_t carry = 0;
    for (int j = 0; j < 4; ++j) {
      uint64_t hi;
      uint64_t lo = fio_math_mulc64(a[i], b[j], &hi);
      uint64_t c1 = 0, c2 = 0;
      t[i + j] = fio_math_addc64(t[i + j], lo, 0, &c1);
      t[i + j] = fio_math_addc64(t[i + j], carry, 0, &c2);
      carry = hi + c1 + c2;
    }
    t[i + 4] += carry;
  }

  /* Reduce mod p */
  fio___p256_fe_reduce(r, t);
}

/** Field squaring: r = a² mod p */
FIO_SFUNC void fio___p256_fe_sqr(fio___p256_fe_s r, const fio___p256_fe_s a) {
  fio___p256_fe_mul(r, a, a);
}

/**
 * Field inversion: r = a^(-1) mod p using Fermat's little theorem.
 * a^(-1) = a^(p-2) mod p
 *
 * p-2 = 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFD
 *
 * We use a simple square-and-multiply algorithm for correctness.
 */
FIO_SFUNC void fio___p256_fe_inv(fio___p256_fe_s r, const fio___p256_fe_s a) {
  /* p - 2 in little-endian 64-bit limbs */
  static const uint64_t pm2[4] = {
      0xFFFFFFFFFFFFFFFDULL, /* pm2[0] */
      0x00000000FFFFFFFFULL, /* pm2[1] */
      0x0000000000000000ULL, /* pm2[2] */
      0xFFFFFFFF00000001ULL, /* pm2[3] */
  };

  fio___p256_fe_s base, result, tmp;

  /* Initialize result = 1 */
  fio___p256_fe_one(result);

  /* Copy input to base */
  fio___p256_fe_copy(base, a);

  /* Square-and-multiply from LSB to MSB */
  for (int i = 0; i < 4; ++i) {
    uint64_t bits = pm2[i];
    for (int j = 0; j < 64; ++j) {
      if (bits & 1) {
        /* result = result * base */
        fio___p256_fe_mul(tmp, result, base);
        fio___p256_fe_copy(result, tmp);
      }
      /* base = base^2 */
      fio___p256_fe_sqr(tmp, base);
      fio___p256_fe_copy(base, tmp);
      bits >>= 1;
    }
  }

  fio___p256_fe_copy(r, result);
}

/* Note: Debug version of inversion removed - use fio___p256_fe_inv instead */

/* *****************************************************************************
Scalar Arithmetic (mod n)
***************************************************************************** */

/** Scalar element type: 4 x 64-bit limbs in little-endian */
typedef uint64_t fio___p256_scalar_s[4];

/** Load 32-byte big-endian number into scalar */
FIO_IFUNC void fio___p256_scalar_from_bytes(fio___p256_scalar_s r,
                                            const uint8_t in[32]) {
  r[3] = fio_buf2u64_be(in);
  r[2] = fio_buf2u64_be(in + 8);
  r[1] = fio_buf2u64_be(in + 16);
  r[0] = fio_buf2u64_be(in + 24);
}

/** Public-value check: returns 1 if scalar is zero. */
FIO_IFUNC int fio___p256_scalar_is_zero(const fio___p256_scalar_s a) {
  return (a[0] | a[1] | a[2] | a[3]) == 0;
}

/** Secret-safe check: returns 1 if scalar is zero. */
FIO_IFUNC int fio___p256_scalar_ct_is_zero(const fio___p256_scalar_s a) {
  uint64_t diff = a[0] | a[1] | a[2] | a[3];
  return (int)fio_ct_false(diff);
}

/** Public-value check: returns 1 if scalar >= n (curve order). */
FIO_IFUNC int fio___p256_scalar_gte_n(const fio___p256_scalar_s a) {
  /* Compare from most significant limb */
  if (a[3] > FIO___P256_N[3])
    return 1;
  if (a[3] < FIO___P256_N[3])
    return 0;
  if (a[2] > FIO___P256_N[2])
    return 1;
  if (a[2] < FIO___P256_N[2])
    return 0;
  if (a[1] > FIO___P256_N[1])
    return 1;
  if (a[1] < FIO___P256_N[1])
    return 0;
  if (a[0] >= FIO___P256_N[0])
    return 1;
  return 0;
}

/** Secret-safe check: returns 1 if scalar >= n (curve order). */
FIO_IFUNC int fio___p256_scalar_ct_gte_n(const fio___p256_scalar_s a) {
  uint64_t borrow = 0;
  (void)fio_math_subc64(a[0], FIO___P256_N[0], 0, &borrow);
  (void)fio_math_subc64(a[1], FIO___P256_N[1], borrow, &borrow);
  (void)fio_math_subc64(a[2], FIO___P256_N[2], borrow, &borrow);
  (void)fio_math_subc64(a[3], FIO___P256_N[3], borrow, &borrow);
  return (int)(borrow ^ 1);
}

/** Secret-safe conditional scalar subtraction: a = cond ? a - n : a. */
FIO_IFUNC void fio___p256_scalar_sub_n_if(fio___p256_scalar_s a,
                                          uint64_t cond) {
  uint64_t borrow = 0;
  uint64_t s[4];
  uint64_t mask = (uint64_t)0 - (cond & 1);
  s[0] = fio_math_subc64(a[0], FIO___P256_N[0], 0, &borrow);
  s[1] = fio_math_subc64(a[1], FIO___P256_N[1], borrow, &borrow);
  s[2] = fio_math_subc64(a[2], FIO___P256_N[2], borrow, &borrow);
  s[3] = fio_math_subc64(a[3], FIO___P256_N[3], borrow, &borrow);
  a[0] = fio_ct_mux64(mask, s[0], a[0]);
  a[1] = fio_ct_mux64(mask, s[1], a[1]);
  a[2] = fio_ct_mux64(mask, s[2], a[2]);
  a[3] = fio_ct_mux64(mask, s[3], a[3]);
}

/** Scalar reduction mod n using fio_math_div */
FIO_SFUNC void fio___p256_scalar_reduce(fio___p256_scalar_s r,
                                        const uint64_t t[8]) {
  /* Use fio_math_div to compute t mod n directly.
   * We need to extend n to 8 limbs for the division. */
  uint64_t n_ext[8] = {FIO___P256_N[0],
                       FIO___P256_N[1],
                       FIO___P256_N[2],
                       FIO___P256_N[3],
                       0,
                       0,
                       0,
                       0};
  uint64_t remainder[8] = {0};

  /* Compute t mod n */
  fio_math_div(NULL, remainder, t, n_ext, 8);

  /* Copy the low 4 limbs to result */
  r[0] = remainder[0];
  r[1] = remainder[1];
  r[2] = remainder[2];
  r[3] = remainder[3];
}

/** Scalar multiplication: r = a * b mod n */
FIO_SFUNC void fio___p256_scalar_mul(fio___p256_scalar_s r,
                                     const fio___p256_scalar_s a,
                                     const fio___p256_scalar_s b) {
  uint64_t t[8] = {0};

  /* Schoolbook multiplication with full carry propagation. */
#if defined(__SIZEOF_INT128__)
  for (int i = 0; i < 4; ++i) {
    __uint128_t carry = 0;
    for (int j = 0; j < 4; ++j) {
      const int k = i + j;
      __uint128_t sum = (__uint128_t)a[i] * b[j] + t[k] + carry;
      t[k] = (uint64_t)sum;
      carry = sum >> 64;
    }
    for (int k = i + 4; carry && k < 8; ++k) {
      __uint128_t sum = (__uint128_t)t[k] + carry;
      t[k] = (uint64_t)sum;
      carry = sum >> 64;
    }
  }
#else
  for (int i = 0; i < 4; ++i) {
    uint64_t carry = 0;
    for (int j = 0; j < 4; ++j) {
      uint64_t hi;
      uint64_t lo = fio_math_mulc64(a[i], b[j], &hi);
      uint64_t c1 = 0, c2 = 0;
      t[i + j] = fio_math_addc64(t[i + j], lo, 0, &c1);
      t[i + j] = fio_math_addc64(t[i + j], carry, 0, &c2);
      carry = hi + c1 + c2;
    }
    for (int k = i + 4; carry && k < 8; ++k) {
      uint64_t c = 0;
      t[k] = fio_math_addc64(t[k], carry, 0, &c);
      carry = c;
    }
  }
#endif

  fio___p256_scalar_reduce(r, t);
}

/**
 * Scalar inversion: r = a^(-1) mod n using Fermat's little theorem.
 * a^(-1) = a^(n-2) mod n
 */
FIO_SFUNC void fio___p256_scalar_inv(fio___p256_scalar_s r,
                                     const fio___p256_scalar_s a) {
  /* n-2 = 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC63254F */
  fio___p256_scalar_s t, tmp;

  /* Start with a */
  fio___p256_scalar_s base;
  base[0] = a[0];
  base[1] = a[1];
  base[2] = a[2];
  base[3] = a[3];

  /* Initialize result to 1 */
  t[0] = 1;
  t[1] = 0;
  t[2] = 0;
  t[3] = 0;

  /* Binary exponentiation for n-2 */
  /* n-2 in binary has a specific pattern we can optimize */
  /* For now, use simple square-and-multiply */

  /* Process each bit of n-2 from LSB to MSB */
  static const uint64_t nm2[4] = {
      0xF3B9CAC2FC63254FULL, /* (n-2)[0] */
      0xBCE6FAADA7179E84ULL, /* (n-2)[1] */
      0xFFFFFFFFFFFFFFFFULL, /* (n-2)[2] */
      0xFFFFFFFF00000000ULL, /* (n-2)[3] */
  };

  for (int i = 0; i < 4; ++i) {
    uint64_t bits = nm2[i];
    for (int j = 0; j < 64; ++j) {
      if (bits & 1) {
        fio___p256_scalar_mul(tmp, t, base);
        t[0] = tmp[0];
        t[1] = tmp[1];
        t[2] = tmp[2];
        t[3] = tmp[3];
      }
      fio___p256_scalar_mul(tmp, base, base);
      base[0] = tmp[0];
      base[1] = tmp[1];
      base[2] = tmp[2];
      base[3] = tmp[3];
      bits >>= 1;
    }
  }

  r[0] = t[0];
  r[1] = t[1];
  r[2] = t[2];
  r[3] = t[3];
}

/* *****************************************************************************
Point Operations (Jacobian Coordinates)
***************************************************************************** */

/** Check if point is at infinity (Z = 0) */
FIO_IFUNC int fio___p256_point_is_infinity(
    const fio___p256_point_jacobian_s *p) {
  return fio___p256_fe_is_zero(p->z);
}

/** Set point to infinity */
FIO_IFUNC void fio___p256_point_set_infinity(fio___p256_point_jacobian_s *p) {
  fio___p256_fe_one(p->x);
  fio___p256_fe_one(p->y);
  fio___p256_fe_zero(p->z);
}

/** Convert affine point to Jacobian */
FIO_IFUNC void fio___p256_point_to_jacobian(
    fio___p256_point_jacobian_s *j,
    const fio___p256_point_affine_s *a) {
  fio___p256_fe_copy(j->x, a->x);
  fio___p256_fe_copy(j->y, a->y);
  fio___p256_fe_one(j->z);
}

/** Return all-ones if field element is zero, zero otherwise. */
FIO_IFUNC uint64_t fio___p256_fe_ct_is_zero_mask(const fio___p256_fe_s a) {
  return (uint64_t)0 - (uint64_t)fio___p256_fe_ct_is_zero(a);
}

FIO_IFUNC void fio___p256_point_cswap(fio___p256_point_jacobian_s *a,
                                      fio___p256_point_jacobian_s *b,
                                      uint64_t swap);

/** Convert Jacobian point to affine (x = X/Z², y = Y/Z³) */
FIO_SFUNC void fio___p256_point_to_affine(
    fio___p256_point_affine_s *a,
    const fio___p256_point_jacobian_s *j) {
  fio___p256_fe_s z_inv, z_inv2, z_inv3;

  /* Inverting Z=0 yields zero with this fixed-exponent inversion path, so
   * infinity maps to the same affine (0, 0) result with no branch. */
  fio___p256_fe_inv(z_inv, j->z);
  fio___p256_fe_sqr(z_inv2, z_inv);
  fio___p256_fe_mul(z_inv3, z_inv2, z_inv);

  fio___p256_fe_mul(a->x, j->x, z_inv2);
  fio___p256_fe_mul(a->y, j->y, z_inv3);

  fio_secure_zero(z_inv, sizeof(z_inv));
  fio_secure_zero(z_inv2, sizeof(z_inv2));
  fio_secure_zero(z_inv3, sizeof(z_inv3));
}

/**
 * Point doubling in Jacobian coordinates.
 * Uses the formula from "Guide to Elliptic Curve Cryptography" (Hankerson et
 * al.)
 *
 * For P-256 where a = -3:
 * λ = 3(X - Z²)(X + Z²)
 * X' = λ² - 2S where S = 4XY²
 * Y' = λ(S - X') - 8Y⁴
 * Z' = 2YZ
 */
FIO_SFUNC void fio___p256_point_double(fio___p256_point_jacobian_s *r,
                                       const fio___p256_point_jacobian_s *p) {
  fio___p256_fe_s t1, t2, t3, t4, t5, yz;
  fio___p256_point_jacobian_s generic, infinity, candidate;

  /* Save Y*Z early to handle aliasing (r == p) */
  fio___p256_fe_mul(yz, p->y, p->z);

  /* t1 = Z² */
  fio___p256_fe_sqr(t1, p->z);

  /* t2 = X - Z² */
  fio___p256_fe_sub(t2, p->x, t1);

  /* t3 = X + Z² */
  fio___p256_fe_add(t3, p->x, t1);

  /* t2 = (X - Z²)(X + Z²) = X² - Z⁴ */
  fio___p256_fe_mul(t2, t2, t3);

  /* t2 = 3(X² - Z⁴) = λ (since a = -3, this equals 3X² + aZ⁴) */
  fio___p256_fe_add(t3, t2, t2);
  fio___p256_fe_add(t2, t3, t2);

  /* t4 = Y² */
  fio___p256_fe_sqr(t4, p->y);

  /* t5 = XY² */
  fio___p256_fe_mul(t5, p->x, t4);

  /* t5 = 4XY² = S */
  fio___p256_fe_add(t5, t5, t5);
  fio___p256_fe_add(t5, t5, t5);

  /* t3 = λ² */
  fio___p256_fe_sqr(t3, t2);

  /* X' = λ² - 2S */
  fio___p256_fe_sub(generic.x, t3, t5);
  fio___p256_fe_sub(generic.x, generic.x, t5);

  /* t4 = Y⁴ */
  fio___p256_fe_sqr(t4, t4);

  /* t4 = 8Y⁴ */
  fio___p256_fe_add(t4, t4, t4);
  fio___p256_fe_add(t4, t4, t4);
  fio___p256_fe_add(t4, t4, t4);

  /* t5 = S - X' */
  fio___p256_fe_sub(t5, t5, generic.x);

  /* Y' = λ(S - X') - 8Y⁴ */
  fio___p256_fe_mul(generic.y, t2, t5);
  fio___p256_fe_sub(generic.y, generic.y, t4);

  /* Z' = 2YZ (using pre-computed value to handle aliasing) */
  fio___p256_fe_add(generic.z, yz, yz);

  /* Select infinity for infinity input without branching on p->z. */
  fio___p256_point_set_infinity(&infinity);
  *r = generic;
  candidate = infinity;
  fio___p256_point_cswap(r, &candidate, fio___p256_fe_ct_is_zero_mask(p->z) & 1);

  fio_secure_zero(t1, sizeof(t1));
  fio_secure_zero(t2, sizeof(t2));
  fio_secure_zero(t3, sizeof(t3));
  fio_secure_zero(t4, sizeof(t4));
  fio_secure_zero(t5, sizeof(t5));
  fio_secure_zero(yz, sizeof(yz));
  fio_secure_zero(&generic, sizeof(generic));
  fio_secure_zero(&infinity, sizeof(infinity));
  fio_secure_zero(&candidate, sizeof(candidate));
}

/**
 * Point addition in Jacobian coordinates.
 * r = p + q where p is Jacobian and q is affine.
 *
 * Mixed addition is faster than full Jacobian addition.
 */
FIO_SFUNC void fio___p256_point_add_mixed(fio___p256_point_jacobian_s *r,
                                          const fio___p256_point_jacobian_s *p,
                                          const fio___p256_point_affine_s *q) {
  fio___p256_fe_s t1, t2, t3, t4, t5, t6;
  fio___p256_point_jacobian_s generic, doubled, infinity, q_jac, selected;
  fio___p256_point_jacobian_s candidate;

  /* t1 = Z₁² */
  fio___p256_fe_sqr(t1, p->z);

  /* t2 = Z₁³ */
  fio___p256_fe_mul(t2, t1, p->z);

  /* t3 = X₂Z₁² */
  fio___p256_fe_mul(t3, q->x, t1);

  /* t4 = Y₂Z₁³ */
  fio___p256_fe_mul(t4, q->y, t2);

  /* t3 = X₂Z₁² - X₁ = H */
  fio___p256_fe_sub(t3, t3, p->x);

  /* t4 = Y₂Z₁³ - Y₁ = R */
  fio___p256_fe_sub(t4, t4, p->y);

  /* t5 = H² */
  fio___p256_fe_sqr(t5, t3);

  /* t6 = H³ */
  fio___p256_fe_mul(t6, t5, t3);

  /* t1 = X₁H² */
  fio___p256_fe_mul(t1, p->x, t5);

  /* t2 = Y₁H³ */
  fio___p256_fe_mul(t2, p->y, t6);

  /* Z₃ = Z₁H */
  fio___p256_fe_mul(generic.z, p->z, t3);

  /* X₃ = R² - H³ - 2X₁H² */
  fio___p256_fe_sqr(generic.x, t4);
  fio___p256_fe_sub(generic.x, generic.x, t6);
  fio___p256_fe_sub(generic.x, generic.x, t1);
  fio___p256_fe_sub(generic.x, generic.x, t1);

  /* t1 = X₁H² - X₃ */
  fio___p256_fe_sub(t1, t1, generic.x);

  /* Y₃ = R(X₁H² - X₃) - Y₁H³ */
  fio___p256_fe_mul(generic.y, t4, t1);
  fio___p256_fe_sub(generic.y, generic.y, t2);

  /* Compute all exceptional-case results unconditionally. */
  fio___p256_point_double(&doubled, p);
  fio___p256_point_set_infinity(&infinity);
  fio___p256_point_to_jacobian(&q_jac, q);

  /* Select the correct result without branching on point coordinates:
   * - p infinity: q
   * - H == 0 and R == 0: 2p
   * - H == 0 and R != 0: infinity
   * - otherwise: generic mixed-add result */
  uint64_t p_inf_mask = fio___p256_fe_ct_is_zero_mask(p->z);
  uint64_t h_zero_mask = fio___p256_fe_ct_is_zero_mask(t3);
  uint64_t r_zero_mask = fio___p256_fe_ct_is_zero_mask(t4);
  uint64_t not_p_inf_mask = ~p_inf_mask;
  uint64_t equal_mask = not_p_inf_mask & h_zero_mask & r_zero_mask;
  uint64_t opposite_mask = not_p_inf_mask & h_zero_mask & ~r_zero_mask;

  selected = generic;
  candidate = infinity;
  fio___p256_point_cswap(&selected, &candidate, opposite_mask & 1);
  candidate = doubled;
  fio___p256_point_cswap(&selected, &candidate, equal_mask & 1);
  candidate = q_jac;
  fio___p256_point_cswap(&selected, &candidate, p_inf_mask & 1);
  *r = selected;

  fio_secure_zero(t1, sizeof(t1));
  fio_secure_zero(t2, sizeof(t2));
  fio_secure_zero(t3, sizeof(t3));
  fio_secure_zero(t4, sizeof(t4));
  fio_secure_zero(t5, sizeof(t5));
  fio_secure_zero(t6, sizeof(t6));
  fio_secure_zero(&generic, sizeof(generic));
  fio_secure_zero(&doubled, sizeof(doubled));
  fio_secure_zero(&infinity, sizeof(infinity));
  fio_secure_zero(&q_jac, sizeof(q_jac));
  fio_secure_zero(&selected, sizeof(selected));
  fio_secure_zero(&candidate, sizeof(candidate));
}

/* (fio___p256_point_add_jacobian removed — ladder uses mixed add directly) */

/**
 * Constant-time conditional swap of two Jacobian points.
 * swap must be exactly 0 or 1.
 */
FIO_IFUNC void fio___p256_point_cswap(fio___p256_point_jacobian_s *a,
                                      fio___p256_point_jacobian_s *b,
                                      uint64_t swap) {
  uint64_t mask = (uint64_t)0 - swap; /* 0x000...0 or 0xFFF...F */
  uint64_t tmp;
#define FIO___P256_CSWAP_LIMB(field, i)                                        \
  tmp = mask & (a->field[i] ^ b->field[i]);                                    \
  a->field[i] ^= tmp;                                                          \
  b->field[i] ^= tmp
  FIO___P256_CSWAP_LIMB(x, 0);
  FIO___P256_CSWAP_LIMB(x, 1);
  FIO___P256_CSWAP_LIMB(x, 2);
  FIO___P256_CSWAP_LIMB(x, 3);
  FIO___P256_CSWAP_LIMB(y, 0);
  FIO___P256_CSWAP_LIMB(y, 1);
  FIO___P256_CSWAP_LIMB(y, 2);
  FIO___P256_CSWAP_LIMB(y, 3);
  FIO___P256_CSWAP_LIMB(z, 0);
  FIO___P256_CSWAP_LIMB(z, 1);
  FIO___P256_CSWAP_LIMB(z, 2);
  FIO___P256_CSWAP_LIMB(z, 3);
#undef FIO___P256_CSWAP_LIMB
}

/**
 * Scalar multiplication: r = k * P  (constant-time Montgomery ladder)
 *
 * Uses the Montgomery ladder (double-and-add-always) for constant-time
 * execution. Every bit position performs exactly the same sequence of field
 * operations regardless of the scalar value, preventing timing side-channels.
 *
 * The ladder starts from R[0] = infinity and R[1] = P, then processes exactly
 * the 256 scalar bits from most-significant to least-significant. Do not use
 * the old k + 2n shortcut: for scalars near n it can overflow the assumed
 * 257-bit range and drop a high carry.
 *
 * Algorithm (256 bits):
 *   R[0] = infinity, R[1] = P
 *   for bit i from 255 down to 0:
 *     b = bit i of k
 *     cswap(R[0], R[1], b)
 *     D = 2 * R[0]
 *     P_sel = (b == 0) ? P : -P   (selected with masks, no branch)
 *     R[1] = D + P_sel            (equivalent to old R[0] + old R[1])
 *     R[0] = D
 *     cswap(R[0], R[1], b)
 *   result = R[0]
 *
 * After the first cswap, the invariant R[1] = R[0] ± P allows computing
 * R[0] + R[1] as 2*R[0] ± P using one double and one mixed add. The sign is
 * determined by the current bit b through branchless y-coordinate selection.
 * This avoids field inversion entirely: 1 double + 1 mixed add per bit.
 */
FIO_SFUNC void fio___p256_point_mul(fio___p256_point_jacobian_s *r,
                                    const fio___p256_scalar_s k,
                                    const fio___p256_point_affine_s *p) {
  /* Constant-time Montgomery ladder over the 256 scalar bits of k.
   *
   * The previous k' = k + 2n shortcut assumed bit 256 of k' was always the
   * leading 1, but k + 2n can exceed 2^257 for scalars near n.  That caused
   * the ladder to drop a high carry and produced wrong results for many
   * random scalars (small scalars like 1 and 2 happened to work).
   *
   * Instead we start from the true scalar k with the standard initialization:
   *   R[0] = infinity, R[1] = P
   * and process bits 255 down to 0.  point_add_mixed handles the infinity
   * case, so the first iterations where k's top bits are zero are safe.
   *
   * Invariant after the first cswap (and before the second):
   *   b=0: R[0] = a*P, R[1] = (a+1)*P  → R[1] = R[0] + P
   *   b=1: R[0] = (a+1)*P, R[1] = a*P  → R[1] = R[0] - P
   *
   * We compute D = 2*R[0] and then R[1] = D + P_sel where P_sel is P for
   * b=0 and -P for b=1, selected branchlessly.
   */

  /* Precompute -P (same x, negated y) for the b=1 case */
  fio___p256_point_affine_s P_neg;
  fio___p256_fe_copy(P_neg.x, p->x);
  fio___p256_fe_neg(P_neg.y, p->y);

  fio___p256_point_jacobian_s R0, R1, D;
  fio___p256_point_affine_s P_sel;

  /* Initialize ladder: R[0] = infinity, R[1] = P */
  fio___p256_point_set_infinity(&R0);
  fio___p256_point_to_jacobian(&R1, p);

  for (int i = 255; i >= 0; --i) {
    uint64_t b = (k[i / 64] >> (i % 64)) & 1ULL;

    /* First cswap: if b=1, swap so R[0] is the "larger" register */
    fio___p256_point_cswap(&R0, &R1, b);

    /* D = 2*R[0] */
    fio___p256_point_double(&D, &R0);

    /* Constant-time select P or -P based on b:
     * mask = 0 if b=0 (use P), ~0 if b=1 (use -P).
     * x-coordinate is the same for P and -P. */
    uint64_t bmask = (uint64_t)0 - b;
    fio___p256_fe_copy(P_sel.x, p->x);
    P_sel.y[0] = (p->y[0] & ~bmask) | (P_neg.y[0] & bmask);
    P_sel.y[1] = (p->y[1] & ~bmask) | (P_neg.y[1] & bmask);
    P_sel.y[2] = (p->y[2] & ~bmask) | (P_neg.y[2] & bmask);
    P_sel.y[3] = (p->y[3] & ~bmask) | (P_neg.y[3] & bmask);

    /* R[1] = D + P_sel = 2*R[0] + (b==0 ? P : -P) = R[0] + R[1] */
    fio___p256_point_add_mixed(&R1, &D, &P_sel);

    /* R[0] = D = 2*R[0] */
    R0 = D;

    /* Second cswap: restore order */
    fio___p256_point_cswap(&R0, &R1, b);
  }

  *r = R0;

  fio_secure_zero(&P_neg, sizeof(P_neg));
  fio_secure_zero(&P_sel, sizeof(P_sel));
  fio_secure_zero(&R0, sizeof(R0));
  fio_secure_zero(&R1, sizeof(R1));
  fio_secure_zero(&D, sizeof(D));
}

/**
 * Double scalar multiplication: r = u1*G + u2*Q
 * Uses Shamir's trick for efficiency.
 */
FIO_SFUNC void fio___p256_point_mul2(fio___p256_point_jacobian_s *r,
                                     const fio___p256_scalar_s u1,
                                     const fio___p256_scalar_s u2,
                                     const fio___p256_point_affine_s *q) {
  /* Base point G */
  fio___p256_point_affine_s g;
  fio___p256_fe_copy(g.x, FIO___P256_GX);
  fio___p256_fe_copy(g.y, FIO___P256_GY);

  /* Precompute G + Q */
  fio___p256_point_jacobian_s gpq;
  fio___p256_point_to_jacobian(&gpq, &g);
  fio___p256_point_add_mixed(&gpq, &gpq, q);
  fio___p256_point_affine_s gpq_affine;
  fio___p256_point_to_affine(&gpq_affine, &gpq);

  fio___p256_point_set_infinity(r);

  /* Find highest set bit in either scalar */
  int start_bit = 255;
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
    fio___p256_point_double(r, r);

    int limb = i / 64;
    int bit = i % 64;
    int b1 = (u1[limb] >> bit) & 1;
    int b2 = (u2[limb] >> bit) & 1;

    if (b1 && b2) {
      fio___p256_point_add_mixed(r, r, &gpq_affine);
    } else if (b1) {
      fio___p256_point_add_mixed(r, r, &g);
    } else if (b2) {
      fio___p256_point_add_mixed(r, r, q);
    }
  }
}

/* *****************************************************************************
DER Signature Parsing
***************************************************************************** */

/**
 * Parse DER-encoded ECDSA signature.
 * Format: SEQUENCE { r INTEGER, s INTEGER }
 *
 * Returns 0 on success, -1 on error.
 */
FIO_SFUNC int fio___p256_parse_der_signature(uint8_t r[32],
                                             uint8_t s[32],
                                             const uint8_t *sig,
                                             size_t sig_len) {
  if (!sig || sig_len < 8)
    return -1;

  const uint8_t *p = sig;
  const uint8_t *end = sig + sig_len;

  /* SEQUENCE tag */
  if (*p++ != 0x30)
    return -1;

  /* SEQUENCE length */
  size_t seq_len;
  if (*p & 0x80) {
    /* Long form length */
    int len_bytes = *p++ & 0x7F;
    if (len_bytes > 2 || p + len_bytes > end)
      return -1;
    seq_len = 0;
    for (int i = 0; i < len_bytes; ++i)
      seq_len = (seq_len << 8) | *p++;
  } else {
    seq_len = *p++;
  }

  if (p + seq_len > end)
    return -1;

  /* Parse r INTEGER */
  if (*p++ != 0x02)
    return -1;

  size_t r_len = *p++;
  if (p + r_len > end || r_len > 33)
    return -1;

  /* Skip leading zero if present (positive integer encoding) */
  const uint8_t *r_data = p;
  if (r_len > 0 && *r_data == 0x00) {
    r_data++;
    r_len--;
  }
  p += (r_len + (r_data != p ? 1 : 0));

  /* Copy r, right-aligned in 32 bytes */
  FIO_MEMSET(r, 0, 32);
  if (r_len > 32)
    return -1;
  FIO_MEMCPY(r + (32 - r_len), r_data, r_len);

  /* Parse s INTEGER */
  if (*p++ != 0x02)
    return -1;

  size_t s_len = *p++;
  if (p + s_len > end || s_len > 33)
    return -1;

  /* Skip leading zero if present */
  const uint8_t *s_data = p;
  if (s_len > 0 && *s_data == 0x00) {
    s_data++;
    s_len--;
  }

  /* Copy s, right-aligned in 32 bytes */
  FIO_MEMSET(s, 0, 32);
  if (s_len > 32)
    return -1;
  FIO_MEMCPY(s + (32 - s_len), s_data, s_len);

  return 0;
}

/* *****************************************************************************
ECDSA Verification
***************************************************************************** */

SFUNC int fio_ecdsa_p256_verify_raw(const uint8_t r_bytes[32],
                                    const uint8_t s_bytes[32],
                                    const uint8_t msg_hash[32],
                                    const uint8_t pubkey_x[32],
                                    const uint8_t pubkey_y[32]) {
  fio___p256_scalar_s r, s, e;
  fio___p256_point_affine_s q;

  /* Load signature components */
  fio___p256_scalar_from_bytes(r, r_bytes);
  fio___p256_scalar_from_bytes(s, s_bytes);

  /* Verify r, s are in [1, n-1] */
  if (fio___p256_scalar_is_zero(r) || fio___p256_scalar_gte_n(r))
    return -1;
  if (fio___p256_scalar_is_zero(s) || fio___p256_scalar_gte_n(s))
    return -1;

  /* Load message hash as scalar e */
  fio___p256_scalar_from_bytes(e, msg_hash);

  /* Reduce e mod n if needed */
  if (fio___p256_scalar_gte_n(e)) {
    uint64_t borrow = 0;
    e[0] = fio_math_subc64(e[0], FIO___P256_N[0], 0, &borrow);
    e[1] = fio_math_subc64(e[1], FIO___P256_N[1], borrow, &borrow);
    e[2] = fio_math_subc64(e[2], FIO___P256_N[2], borrow, &borrow);
    e[3] = fio_math_subc64(e[3], FIO___P256_N[3], borrow, &borrow);
  }

  /* Load public key point Q */
  fio___p256_fe_from_bytes(q.x, pubkey_x);
  fio___p256_fe_from_bytes(q.y, pubkey_y);

  /* Verify Q is on the curve: y² = x³ - 3x + b (mod p) */
  {
    fio___p256_fe_s y2, x3, t;

    /* y² */
    fio___p256_fe_sqr(y2, q.y);

    /* x³ */
    fio___p256_fe_sqr(t, q.x);
    fio___p256_fe_mul(x3, t, q.x);

    /* x³ - 3x */
    fio___p256_fe_sub(t, x3, q.x);
    fio___p256_fe_sub(t, t, q.x);
    fio___p256_fe_sub(t, t, q.x);

    /* x³ - 3x + b */
    fio___p256_fe_add(t, t, FIO___P256_B);

    /* Check y² == x³ - 3x + b */
    if (fio___p256_fe_eq(y2, t) != 0)
      return -1;
  }

  /* Compute w = s^(-1) mod n */
  fio___p256_scalar_s w;
  fio___p256_scalar_inv(w, s);

  /* Compute u1 = e * w mod n */
  fio___p256_scalar_s u1;
  fio___p256_scalar_mul(u1, e, w);

  /* Compute u2 = r * w mod n */
  fio___p256_scalar_s u2;
  fio___p256_scalar_mul(u2, r, w);

  /* Compute R = u1*G + u2*Q */
  fio___p256_point_jacobian_s R_jac;
  fio___p256_point_mul2(&R_jac, u1, u2, &q);

  /* If R is infinity, reject */
  if (fio___p256_point_is_infinity(&R_jac))
    return -1;

  /* Convert R to affine */
  fio___p256_point_affine_s R_aff;
  fio___p256_point_to_affine(&R_aff, &R_jac);

  /* Get R.x as bytes */
  uint8_t rx_bytes[32];
  fio___p256_fe_to_bytes(rx_bytes, R_aff.x);

  /* Load R.x as scalar and reduce mod n */
  fio___p256_scalar_s rx;
  fio___p256_scalar_from_bytes(rx, rx_bytes);
  while (fio___p256_scalar_gte_n(rx)) {
    uint64_t borrow = 0;
    rx[0] = fio_math_subc64(rx[0], FIO___P256_N[0], 0, &borrow);
    rx[1] = fio_math_subc64(rx[1], FIO___P256_N[1], borrow, &borrow);
    rx[2] = fio_math_subc64(rx[2], FIO___P256_N[2], borrow, &borrow);
    rx[3] = fio_math_subc64(rx[3], FIO___P256_N[3], borrow, &borrow);
  }

  /* Verify r == R.x mod n */
  if (r[0] != rx[0] || r[1] != rx[1] || r[2] != rx[2] || r[3] != rx[3])
    return -1;

  return 0;
}

SFUNC int fio_ecdsa_p256_verify(const uint8_t *sig,
                                size_t sig_len,
                                const uint8_t *msg_hash,
                                const uint8_t *pubkey,
                                size_t pubkey_len) {
  if (!sig || !msg_hash || !pubkey)
    return -1;

  /* Uncompressed public key format: 0x04 || x (32 bytes) || y (32 bytes) */
  if (pubkey_len != 65 || pubkey[0] != 0x04)
    return -1;

  /* Parse DER signature */
  uint8_t r[32], s[32];
  if (fio___p256_parse_der_signature(r, s, sig, sig_len) != 0)
    return -1;

  /* Verify */
  return fio_ecdsa_p256_verify_raw(r, s, msg_hash, pubkey + 1, pubkey + 33);
}

/* *****************************************************************************
ECDSA P-256 Signing Implementation
***************************************************************************** */

/**
 * Encode a 32-byte integer as DER INTEGER (handles leading zero for positive).
 * Returns number of bytes written.
 */
FIO_SFUNC size_t fio___p256_encode_der_integer(uint8_t *out,
                                               const uint8_t val[32]) {
  /* Skip leading zeros */
  size_t start = 0;
  while (start < 31 && val[start] == 0)
    ++start;

  /* Need leading zero if high bit is set (to keep positive) */
  int need_zero = (val[start] & 0x80) != 0;
  size_t len = 32 - start + (need_zero ? 1 : 0);

  out[0] = 0x02; /* INTEGER tag */
  out[1] = (uint8_t)len;
  size_t offset = 2;
  if (need_zero)
    out[offset++] = 0x00;
  FIO_MEMCPY(out + offset, val + start, 32 - start);

  return 2 + len;
}

SFUNC int fio_ecdsa_p256_sign(uint8_t *sig,
                              size_t *sig_len,
                              size_t sig_capacity,
                              const uint8_t msg_hash[32],
                              const uint8_t secret_key[32]) {
  if (!sig || !sig_len || !msg_hash || !secret_key || sig_capacity < 72)
    return -1;

  int rc = -1;
  fio___p256_scalar_s d = {0}, e = {0}, k = {0}, k_inv = {0};
  fio___p256_scalar_s r_scalar = {0}, s_scalar = {0}, tmp = {0};
  fio___p256_point_affine_s g = {0}, R_aff = {0};
  fio___p256_point_jacobian_s R_jac = {0};
  uint8_t k_bytes[32] = {0};
  uint8_t r_bytes[32] = {0}, s_bytes[32] = {0};
  uint8_t r_der[35] = {0}, s_der[35] = {0};

#define FIO___P256_SIGN_CLEAR_ATTEMPT()                                        \
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
  fio___p256_scalar_from_bytes(d, secret_key);

  /* Validate d: 0 < d < n */
  if (fio___p256_scalar_ct_is_zero(d) | fio___p256_scalar_ct_gte_n(d))
    goto cleanup;

  /* Load message hash as scalar e */
  fio___p256_scalar_from_bytes(e, msg_hash);

  /* Reduce e mod n if needed */
  if (fio___p256_scalar_gte_n(e)) {
    uint64_t borrow = 0;
    e[0] = fio_math_subc64(e[0], FIO___P256_N[0], 0, &borrow);
    e[1] = fio_math_subc64(e[1], FIO___P256_N[1], borrow, &borrow);
    e[2] = fio_math_subc64(e[2], FIO___P256_N[2], borrow, &borrow);
    e[3] = fio_math_subc64(e[3], FIO___P256_N[3], borrow, &borrow);
  }

  fio___p256_fe_copy(g.x, FIO___P256_GX);
  fio___p256_fe_copy(g.y, FIO___P256_GY);

  /* Generate random k and compute signature. */
  for (int attempts = 0; attempts < 100; ++attempts) {
    /* Generate random nonce k in the valid scalar range 0 < k < n. */
    do {
      fio_rand_bytes(k_bytes, 32);
      fio___p256_scalar_from_bytes(k, k_bytes);
    } while (fio___p256_scalar_ct_is_zero(k) |
             fio___p256_scalar_ct_gte_n(k));

    /* Compute R = k * G */
    fio___p256_point_mul(&R_jac, k, &g);

    if (fio___p256_point_is_infinity(&R_jac)) {
      FIO___P256_SIGN_CLEAR_ATTEMPT();
      continue;
    }

    /* Convert R to affine */
    fio___p256_point_to_affine(&R_aff, &R_jac);

    /* r = R.x mod n */
    fio___p256_fe_to_bytes(r_bytes, R_aff.x);
    fio___p256_scalar_from_bytes(r_scalar, r_bytes);

    /* Reduce r mod n if needed. R.x < p < 2n, so one subtract is enough. */
    fio___p256_scalar_sub_n_if(r_scalar,
                               (uint64_t)fio___p256_scalar_ct_gte_n(r_scalar));

    if (fio___p256_scalar_ct_is_zero(r_scalar)) {
      FIO___P256_SIGN_CLEAR_ATTEMPT();
      continue;
    }

    /* Compute s = k^(-1) * (e + r*d) mod n */
    fio___p256_scalar_mul(tmp, r_scalar, d);

    uint64_t carry = 0;
    tmp[0] = fio_math_addc64(tmp[0], e[0], 0, &carry);
    tmp[1] = fio_math_addc64(tmp[1], e[1], carry, &carry);
    tmp[2] = fio_math_addc64(tmp[2], e[2], carry, &carry);
    tmp[3] = fio_math_addc64(tmp[3], e[3], carry, &carry);

    /* e and r*d are each < n, so one subtract is enough. */
    fio___p256_scalar_sub_n_if(tmp,
                               carry |
                                   (uint64_t)fio___p256_scalar_ct_gte_n(tmp));

    fio___p256_scalar_inv(k_inv, k);
    fio___p256_scalar_mul(s_scalar, k_inv, tmp);

    if (fio___p256_scalar_ct_is_zero(s_scalar)) {
      FIO___P256_SIGN_CLEAR_ATTEMPT();
      continue;
    }

    fio_u2buf64_be(r_bytes, r_scalar[3]);
    fio_u2buf64_be(r_bytes + 8, r_scalar[2]);
    fio_u2buf64_be(r_bytes + 16, r_scalar[1]);
    fio_u2buf64_be(r_bytes + 24, r_scalar[0]);

    fio_u2buf64_be(s_bytes, s_scalar[3]);
    fio_u2buf64_be(s_bytes + 8, s_scalar[2]);
    fio_u2buf64_be(s_bytes + 16, s_scalar[1]);
    fio_u2buf64_be(s_bytes + 24, s_scalar[0]);

    size_t r_der_len = fio___p256_encode_der_integer(r_der, r_bytes);
    size_t s_der_len = fio___p256_encode_der_integer(s_der, s_bytes);
    size_t seq_content_len = r_der_len + s_der_len;
    size_t total_len = 2 + seq_content_len;

    if (total_len > sig_capacity)
      goto cleanup;

    sig[0] = 0x30;
    sig[1] = (uint8_t)seq_content_len;
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
  FIO___P256_SIGN_CLEAR_ATTEMPT();
#undef FIO___P256_SIGN_CLEAR_ATTEMPT
  return rc;
}

/* *****************************************************************************
P-256 ECDHE Implementation
***************************************************************************** */

/**
 * Validate that a point is on the P-256 curve.
 * Checks: y² = x³ - 3x + b (mod p)
 *
 * @return 0 if valid, -1 if invalid
 */
FIO_SFUNC int fio___p256_point_validate(const fio___p256_point_affine_s *p) {
  fio___p256_fe_s y2, x3, t;

  /* y² */
  fio___p256_fe_sqr(y2, p->y);

  /* x³ */
  fio___p256_fe_sqr(t, p->x);
  fio___p256_fe_mul(x3, t, p->x);

  /* x³ - 3x */
  fio___p256_fe_sub(t, x3, p->x);
  fio___p256_fe_sub(t, t, p->x);
  fio___p256_fe_sub(t, t, p->x);

  /* x³ - 3x + b */
  fio___p256_fe_add(t, t, FIO___P256_B);

  /* Check y² == x³ - 3x + b */
  return (fio___p256_fe_eq(y2, t) == 0) ? 0 : -1;
}

/**
 * Decompress a compressed P-256 point (33 bytes: 0x02/0x03 || x).
 * Computes y from x using y² = x³ - 3x + b, selects sign based on prefix.
 *
 * @return 0 on success, -1 on failure
 */
FIO_SFUNC int fio___p256_point_decompress(fio___p256_point_affine_s *p,
                                          const uint8_t compressed[33]) {
  uint8_t prefix = compressed[0];
  if (prefix != 0x02 && prefix != 0x03)
    return -1;

  /* Load x coordinate */
  fio___p256_fe_from_bytes(p->x, compressed + 1);

  /* Compute y² = x³ - 3x + b */
  fio___p256_fe_s y2, x3, t;
  fio___p256_fe_sqr(t, p->x);
  fio___p256_fe_mul(x3, t, p->x);
  fio___p256_fe_sub(t, x3, p->x);
  fio___p256_fe_sub(t, t, p->x);
  fio___p256_fe_sub(t, t, p->x);
  fio___p256_fe_add(y2, t, FIO___P256_B);

  /* Compute y = sqrt(y²) mod p using Tonelli-Shanks.
   * For P-256: p ≡ 3 (mod 4), so y = y²^((p+1)/4) mod p */
  /* (p+1)/4 =
   * 0x3FFFFFFFC0000000400000000000000000000000400000000000000000000000 */
  fio___p256_fe_s y;

  /* Use Fermat's method: y = y2^((p+1)/4) mod p
   * We use a simple square-and-multiply with the exponent (p+1)/4 */
  static const uint64_t exp[4] = {
      0x0000000000000000ULL, /* exp[0] */
      0x0000000040000000ULL, /* exp[1] */
      0x4000000000000000ULL, /* exp[2] */
      0x3FFFFFFFC0000000ULL, /* exp[3] */
  };

  /* Start with result = 1 */
  fio___p256_fe_s result;
  fio___p256_fe_one(result);

  /* base = y2 */
  fio___p256_fe_s base;
  fio___p256_fe_copy(base, y2);

  /* Square-and-multiply from LSB to MSB */
  for (int i = 0; i < 4; ++i) {
    uint64_t bits = exp[i];
    for (int j = 0; j < 64; ++j) {
      if (bits & 1) {
        fio___p256_fe_mul(result, result, base);
      }
      fio___p256_fe_sqr(base, base);
      bits >>= 1;
    }
  }

  fio___p256_fe_copy(y, result);

  /* Verify: y² should equal y2 */
  fio___p256_fe_s check;
  fio___p256_fe_sqr(check, y);
  if (fio___p256_fe_eq(check, y2) != 0)
    return -1; /* Not a quadratic residue - invalid point */

  /* Select correct sign: if (y mod 2) != (prefix - 2), negate y */
  uint8_t y_bytes[32];
  fio___p256_fe_to_bytes(y_bytes, y);
  int y_parity = y_bytes[31] & 1;
  int expected_parity = prefix - 0x02;

  if (y_parity != expected_parity) {
    fio___p256_fe_neg(p->y, y);
  } else {
    fio___p256_fe_copy(p->y, y);
  }

  return 0;
}

SFUNC int fio_p256_keypair(uint8_t secret_key[32], uint8_t public_key[65]) {
  if (!secret_key || !public_key)
    return -1;

  int rc = -1;
  fio___p256_scalar_s k = {0};
  fio___p256_point_affine_s g = {0}, pub_aff = {0};
  fio___p256_point_jacobian_s pub_jac = {0};

  /* Generate random scalar and ensure 0 < k < n */
  for (int attempts = 0; attempts < 100; ++attempts) {
    do {
      fio_rand_bytes(secret_key, 32);
    } while (!fio_buf2u64u(secret_key) || !fio_buf2u64u(secret_key + 8) ||
             !fio_buf2u64u(secret_key + 16) || !fio_buf2u64u(secret_key + 24));
    fio___p256_scalar_from_bytes(k, secret_key);

    /* Check k is not zero and k < n */
    if (!fio___p256_scalar_ct_is_zero(k) && !fio___p256_scalar_ct_gte_n(k)) {
      rc = 0;
      break;
    }
  }

  if (rc != 0)
    goto cleanup;

  /* Compute public key = k * G */
  fio___p256_fe_copy(g.x, FIO___P256_GX);
  fio___p256_fe_copy(g.y, FIO___P256_GY);
  fio___p256_point_mul(&pub_jac, k, &g);

  /* Convert to affine */
  fio___p256_point_to_affine(&pub_aff, &pub_jac);

  /* Serialize: 0x04 || x || y */
  public_key[0] = 0x04;
  fio___p256_fe_to_bytes(public_key + 1, pub_aff.x);
  fio___p256_fe_to_bytes(public_key + 33, pub_aff.y);

cleanup:
  if (rc != 0)
    fio_secure_zero(secret_key, 32);
  fio_secure_zero(k, sizeof(k));
  fio_secure_zero(&g, sizeof(g));
  fio_secure_zero(&pub_jac, sizeof(pub_jac));
  fio_secure_zero(&pub_aff, sizeof(pub_aff));
  return rc;
}

SFUNC int fio_p256_shared_secret(uint8_t shared_secret[32],
                                 const uint8_t secret_key[32],
                                 const uint8_t *their_public_key,
                                 size_t their_public_key_len) {
  if (!shared_secret || !secret_key || !their_public_key)
    return -1;

  int rc = -1;
  fio___p256_point_affine_s their_point = {0}, result_aff = {0};
  fio___p256_point_jacobian_s result_jac = {0};
  fio___p256_scalar_s k = {0};

  /* Parse their public key */
  if (their_public_key_len == 65) {
    /* Uncompressed: 0x04 || x || y */
    if (their_public_key[0] != 0x04)
      goto cleanup;
    fio___p256_fe_from_bytes(their_point.x, their_public_key + 1);
    fio___p256_fe_from_bytes(their_point.y, their_public_key + 33);
  } else if (their_public_key_len == 33) {
    /* Compressed: 0x02/0x03 || x */
    if (fio___p256_point_decompress(&their_point, their_public_key) != 0)
      goto cleanup;
  } else {
    goto cleanup;
  }

  /* Validate point is on curve */
  if (fio___p256_point_validate(&their_point) != 0)
    goto cleanup;

  /* Load and validate our secret scalar: 0 < k < n */
  fio___p256_scalar_from_bytes(k, secret_key);
  if (fio___p256_scalar_ct_is_zero(k) | fio___p256_scalar_ct_gte_n(k))
    goto cleanup;

  /* Compute shared = k * their_point */
  fio___p256_point_mul(&result_jac, k, &their_point);

  /* Check for point at infinity (shouldn't happen with valid inputs) */
  if (fio___p256_point_is_infinity(&result_jac))
    goto cleanup;

  /* Convert to affine and extract x-coordinate */
  fio___p256_point_to_affine(&result_aff, &result_jac);
  fio___p256_fe_to_bytes(shared_secret, result_aff.x);

  /* Check for all-zero output (low-order point attack) */
  uint8_t zero_check = 0;
  for (int i = 0; i < 32; ++i)
    zero_check |= shared_secret[i];
  rc = zero_check ? 0 : -1;

cleanup:
  if (rc != 0)
    fio_secure_zero(shared_secret, 32);
  fio_secure_zero(k, sizeof(k));
  fio_secure_zero(&their_point, sizeof(their_point));
  fio_secure_zero(&result_jac, sizeof(result_jac));
  fio_secure_zero(&result_aff, sizeof(result_aff));
  return rc;
}

/* *****************************************************************************
Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_P256
#endif /* FIO_P256 */
