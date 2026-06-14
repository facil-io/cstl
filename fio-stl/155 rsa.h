/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_RSA                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                    RSA Signature Verification and Signing for TLS 1.3
                         (PKCS#1 v1.5 and RSA-PSS)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_RSA) && !defined(H___FIO_RSA___H)
#define H___FIO_RSA___H

/* *****************************************************************************
RSA Signature Module

This module provides RSA signature verification and signing for TLS 1.3.
It supports:

- PKCS#1 v1.5 signatures (sha256WithRSAEncryption, etc.)
- RSA-PSS signatures (required for TLS 1.3 CertificateVerify)
- Key sizes: 2048, 3072, 4096 bits
- Signing with RSA-PSS (for TLS 1.3 server CertificateVerify)

**Note**: This implementation has not been audited. Use at your own risk.
***************************************************************************** */

/* *****************************************************************************
RSA Constants
***************************************************************************** */

/** Maximum RSA key size in bits */
#define FIO_RSA_MAX_BITS 4096

/** Maximum RSA key size in bytes */
#define FIO_RSA_MAX_BYTES (FIO_RSA_MAX_BITS / 8)

/** Maximum RSA key size in 64-bit words */
#define FIO_RSA_MAX_WORDS (FIO_RSA_MAX_BYTES / 8)

/** Hash algorithm identifiers for RSA verification */
typedef enum {
  FIO_RSA_HASH_SHA256 = 0, /**< SHA-256 (32 bytes) */
  FIO_RSA_HASH_SHA384 = 1, /**< SHA-384 (48 bytes) */
  FIO_RSA_HASH_SHA512 = 2, /**< SHA-512 (64 bytes) */
} fio_rsa_hash_e;

/* *****************************************************************************
RSA Public Key Structure
***************************************************************************** */

/**
 * RSA public key for signature verification.
 *
 * The modulus (n) and exponent (e) are stored as big-endian byte arrays.
 * This matches the DER encoding used in X.509 certificates.
 */
typedef struct {
  const uint8_t *n; /**< Modulus (big-endian) */
  size_t n_len;     /**< Modulus length in bytes */
  const uint8_t *e; /**< Public exponent (big-endian) */
  size_t e_len;     /**< Exponent length in bytes */
} fio_rsa_pubkey_s;

/**
 * RSA private key for signature generation.
 *
 * The modulus (n) and private exponent (d) are stored as big-endian byte
 * arrays. This matches the DER encoding used in PKCS#8 private keys.
 *
 * Optional CRT parameters (p, q, dP, dQ, qInv) and the public exponent (e)
 * may be provided. When CRT parameters are available, signing uses CRT with
 * message blinding for better side-channel resistance. When only n and d are
 * available, signing falls back to a non-CRT (still constant-time) path.
 *
 * All optional fields are indicated by a non-zero length. Missing CRT
 * parameters may be derived from p, q, and d when p and q are present.
 */
typedef struct {
  const uint8_t *n; /**< Modulus (big-endian) */
  size_t n_len;     /**< Modulus length in bytes (256, 384, or 512) */
  const uint8_t *d; /**< Private exponent (big-endian) */
  size_t d_len;     /**< Private exponent length in bytes */
  const uint8_t *e; /**< Public exponent (big-endian), optional, for blinding */
  size_t e_len;
  const uint8_t *p; /**< Prime p (big-endian), optional, for CRT */
  size_t p_len;
  const uint8_t *q; /**< Prime q (big-endian), optional, for CRT */
  size_t q_len;
  const uint8_t *dP; /**< d mod (p-1) (big-endian), optional, for CRT */
  size_t dP_len;
  const uint8_t *dQ; /**< d mod (q-1) (big-endian), optional, for CRT */
  size_t dQ_len;
  const uint8_t *qInv; /**< q^-1 mod p (big-endian), optional, for CRT */
  size_t qInv_len;
} fio_rsa_privkey_s;

/* *****************************************************************************
RSA Signature Verification API
***************************************************************************** */

/**
 * Verify an RSA PKCS#1 v1.5 signature.
 *
 * This verifies signatures with DigestInfo encoding as used in:
 * - sha256WithRSAEncryption (OID 1.2.840.113549.1.1.11)
 * - sha384WithRSAEncryption (OID 1.2.840.113549.1.1.12)
 * - sha512WithRSAEncryption (OID 1.2.840.113549.1.1.13)
 *
 * @param sig        Signature bytes (same length as modulus)
 * @param sig_len    Signature length in bytes
 * @param msg_hash   Pre-computed hash of the message
 * @param hash_len   Hash length (32, 48, or 64 bytes)
 * @param hash_alg   Hash algorithm used (FIO_RSA_HASH_SHA256, etc.)
 * @param key        RSA public key
 * @return 0 on success (valid signature), -1 on failure
 */
SFUNC int fio_rsa_verify_pkcs1(const uint8_t *sig,
                               size_t sig_len,
                               const uint8_t *msg_hash,
                               size_t hash_len,
                               fio_rsa_hash_e hash_alg,
                               const fio_rsa_pubkey_s *key);

/**
 * Verify an RSA-PSS signature (required for TLS 1.3).
 *
 * RSA-PSS uses probabilistic padding and is the mandatory signature scheme
 * for TLS 1.3 CertificateVerify messages with RSA keys.
 *
 * This implementation uses:
 * - MGF1 with the same hash function
 * - Salt length = hash length (as required by TLS 1.3)
 * - Trailer field = 0xBC
 *
 * @param sig        Signature bytes (same length as modulus)
 * @param sig_len    Signature length in bytes
 * @param msg_hash   Pre-computed hash of the message
 * @param hash_len   Hash length (32, 48, or 64 bytes)
 * @param hash_alg   Hash algorithm used
 * @param key        RSA public key
 * @return 0 on success (valid signature), -1 on failure
 */
SFUNC int fio_rsa_verify_pss(const uint8_t *sig,
                             size_t sig_len,
                             const uint8_t *msg_hash,
                             size_t hash_len,
                             fio_rsa_hash_e hash_alg,
                             const fio_rsa_pubkey_s *key);

/* *****************************************************************************
RSA Signature Generation API (for TLS 1.3 Server CertificateVerify)
***************************************************************************** */

/**
 * Generate an RSA-PSS signature (RSASSA-PSS-SIGN per RFC 8017 Section 8.1.1).
 *
 * This is required for TLS 1.3 server CertificateVerify messages when using
 * RSA certificates. PKCS#1 v1.5 signatures are NOT allowed for
 * CertificateVerify in TLS 1.3.
 *
 * This implementation uses:
 * - MGF1 with the same hash function
 * - Salt length = hash length (as required by TLS 1.3)
 * - Trailer field = 0xBC
 *
 * @param signature  Output buffer for signature (must be key->n_len bytes)
 * @param sig_len    Output: actual signature length (equals key->n_len)
 * @param msg_hash   Pre-computed hash of the message to sign
 * @param hash_len   Hash length (32, 48, or 64 bytes)
 * @param hash_alg   Hash algorithm (FIO_RSA_HASH_SHA256, etc.)
 * @param key        RSA private key
 * @return 0 on success, -1 on error
 */
SFUNC int fio_rsa_sign_pss(uint8_t *signature,
                           size_t *sig_len,
                           const uint8_t *msg_hash,
                           size_t hash_len,
                           fio_rsa_hash_e hash_alg,
                           const fio_rsa_privkey_s *key);

/* *****************************************************************************
Implementation - Possibly Externed Functions
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
PKCS#1 v1.5 DigestInfo ASN.1 Prefixes

These encode: SEQUENCE { AlgorithmIdentifier, OCTET STRING hash }

RFC 8017 Section 9.2 defines these prefixes.
***************************************************************************** */

/** DigestInfo prefix for SHA-256 (19 bytes) */
static const uint8_t fio___rsa_digestinfo_sha256[] = {0x30,
                                                      0x31,
                                                      0x30,
                                                      0x0D,
                                                      0x06,
                                                      0x09,
                                                      0x60,
                                                      0x86,
                                                      0x48,
                                                      0x01,
                                                      0x65,
                                                      0x03,
                                                      0x04,
                                                      0x02,
                                                      0x01,
                                                      0x05,
                                                      0x00,
                                                      0x04,
                                                      0x20};

/** DigestInfo prefix for SHA-384 (19 bytes) */
static const uint8_t fio___rsa_digestinfo_sha384[] = {0x30,
                                                      0x41,
                                                      0x30,
                                                      0x0D,
                                                      0x06,
                                                      0x09,
                                                      0x60,
                                                      0x86,
                                                      0x48,
                                                      0x01,
                                                      0x65,
                                                      0x03,
                                                      0x04,
                                                      0x02,
                                                      0x02,
                                                      0x05,
                                                      0x00,
                                                      0x04,
                                                      0x30};

/** DigestInfo prefix for SHA-512 (19 bytes) */
static const uint8_t fio___rsa_digestinfo_sha512[] = {0x30,
                                                      0x51,
                                                      0x30,
                                                      0x0D,
                                                      0x06,
                                                      0x09,
                                                      0x60,
                                                      0x86,
                                                      0x48,
                                                      0x01,
                                                      0x65,
                                                      0x03,
                                                      0x04,
                                                      0x02,
                                                      0x03,
                                                      0x05,
                                                      0x00,
                                                      0x04,
                                                      0x40};

#define FIO___RSA_DIGESTINFO_PREFIX_LEN 19

/* *****************************************************************************
Implementation - Big Integer Helpers

Internal representation: Little-endian uint64_t array (word[0] = LSW)
Input/Output: Big-endian byte arrays (as used in X.509/TLS)
***************************************************************************** */

/** Convert big-endian bytes to little-endian uint64_t array */
FIO_SFUNC void fio___rsa_bytes_to_words(uint64_t *words,
                                        size_t word_count,
                                        const uint8_t *bytes,
                                        size_t byte_len) {
  FIO_MEMSET(words, 0, word_count * sizeof(uint64_t));

  /* Start from the end of bytes (least significant) */
  size_t word_idx = 0;
  size_t byte_idx = byte_len;

  while (byte_idx > 0 && word_idx < word_count) {
    uint64_t word = 0;
    size_t shift = 0;

    /* Collect up to 8 bytes into one word */
    while (byte_idx > 0 && shift < 64) {
      --byte_idx;
      word |= ((uint64_t)bytes[byte_idx]) << shift;
      shift += 8;
    }
    words[word_idx++] = word;
  }
}

/** Convert little-endian uint64_t array to big-endian bytes */
FIO_SFUNC void fio___rsa_words_to_bytes(uint8_t *bytes,
                                        size_t byte_len,
                                        const uint64_t *words,
                                        size_t word_count) {
  FIO_MEMSET(bytes, 0, byte_len);

  /* Start from the end of bytes (least significant) */
  size_t byte_idx = byte_len;
  size_t word_idx = 0;

  while (byte_idx > 0 && word_idx < word_count) {
    uint64_t word = words[word_idx++];

    for (int i = 0; i < 8 && byte_idx > 0; ++i) {
      --byte_idx;
      bytes[byte_idx] = (uint8_t)(word & 0xFF);
      word >>= 8;
    }
  }
}

/** Compare two big integers. Returns: <0 if a<b, 0 if a==b, >0 if a>b */
FIO_SFUNC int fio___rsa_cmp(const uint64_t *a,
                            const uint64_t *b,
                            size_t word_count) {
  for (size_t i = word_count; i > 0;) {
    --i;
    if (a[i] < b[i])
      return -1;
    if (a[i] > b[i])
      return 1;
  }
  return 0;
}

/** Constant-time conditional swap of two word arrays. */
FIO_SFUNC void fio___rsa_cswap_words(uint64_t *a,
                                     uint64_t *b,
                                     uint64_t swap,
                                     size_t word_count) {
  const uint64_t mask = (uint64_t)0 - (swap & 1);
  for (size_t i = 0; i < word_count; ++i) {
    const uint64_t t = mask & (a[i] ^ b[i]);
    a[i] ^= t;
    b[i] ^= t;
  }
}

/** Modular inverse of an odd 64-bit value modulo 2^64 (Newton-Raphson). */
FIO_SFUNC uint64_t fio___rsa_inv_mod_2pow64(uint64_t a) {
  /* a must be odd. */
  uint64_t x = 1;
  for (int i = 0; i < 8; ++i) {
    x = x * (2 - a * x);
  }
  return x;
}

/* Forward declarations for helpers defined below. */
FIO_SFUNC void fio___rsa_mont_mul(uint64_t *result,
                                  const uint64_t *a,
                                  const uint64_t *b,
                                  const uint64_t *n,
                                  const uint64_t *n_prime,
                                  size_t word_count);
FIO_SFUNC void fio___rsa_mont_setup(const uint64_t *n,
                                    size_t word_count,
                                    uint64_t *n_prime,
                                    uint64_t *r2modn);
FIO_SFUNC void fio___rsa_mul_mod(uint64_t *result,
                                 const uint64_t *a,
                                 const uint64_t *b,
                                 const uint64_t *mod,
                                 size_t mod_word_count);

/** Constant-time less-than: returns 1 if a < b, else 0. */

FIO_SFUNC uint64_t fio___rsa_ct_lt(const uint64_t *a,
                                   const uint64_t *b,
                                   size_t word_count) {
  uint64_t lt = 0;
  uint64_t eq = (uint64_t)0 - 1; /* ~0 */
  for (size_t i = word_count; i > 0;) {
    --i;
    uint64_t a_i = a[i];
    uint64_t b_i = b[i];
    uint64_t diff = a_i ^ b_i;
    uint64_t borrow = 0;
    (void)fio_math_subc64(a_i, b_i, 0, &borrow); /* borrow=1 iff a_i < b_i */
    lt |= (eq & borrow);
    eq &= (uint64_t)0 - (diff == 0);
  }
  return lt & 1;
}

/**
 * Constant-time reduction of a value that is smaller than mod^2.
 *
 * On input, m_full has full_word_count words and is known to satisfy
 * m_full < mod^2 (e.g. the RSA message representative m and mod = p or q).
 * The result is written to out (mod_word_count words) and satisfies
 * out = m_full mod mod.
 */
FIO_SFUNC void fio___rsa_reduce_mod(uint64_t *out,
                                    const uint64_t *m_full,
                                    const uint64_t *mod,
                                    size_t mod_word_count,
                                    size_t full_word_count) {
  uint64_t n_prime[FIO_RSA_MAX_WORDS];
  uint64_t r2modn[FIO_RSA_MAX_WORDS];
  fio___rsa_mont_setup(mod, mod_word_count, n_prime, r2modn);

  /* m_full = m_low + m_high * R where R = 2^(64*mod_word_count).
   * m_high may be shorter than mod_word_count words. */
  uint64_t m_high[FIO_RSA_MAX_WORDS];
  FIO_MEMSET(m_high, 0, sizeof(m_high));
  if (full_word_count > mod_word_count) {
    size_t high_words = full_word_count - mod_word_count;
    if (high_words > mod_word_count)
      high_words = mod_word_count;
    FIO_MEMCPY(m_high, m_full + mod_word_count,
               high_words * sizeof(uint64_t));
  }

  /* Compute R mod mod = 2^(64*mod_word_count) mod mod.  mod is public, so
   * the variable-time division is acceptable. */
  uint64_t R_ext[FIO_RSA_MAX_WORDS * 2];
  uint64_t mod_ext[FIO_RSA_MAX_WORDS * 2];
  uint64_t R_mod_mod[FIO_RSA_MAX_WORDS];
  FIO_MEMSET(R_ext, 0, sizeof(R_ext));
  FIO_MEMSET(mod_ext, 0, sizeof(mod_ext));
  FIO_MEMSET(R_mod_mod, 0, sizeof(R_mod_mod));
  R_ext[mod_word_count] = 1;
  FIO_MEMCPY(mod_ext, mod, mod_word_count * sizeof(uint64_t));
  fio_math_div(NULL, R_mod_mod, R_ext, mod_ext, mod_word_count * 2);

  /* out = m_high * (R mod mod) + m_low  (mod mod).
   * This is correct because m = m_low + m_high * R. */
  fio___rsa_mul_mod(out, m_high, R_mod_mod, mod, mod_word_count);
  (void)fio_math_add(out, out, m_full, mod_word_count);

  /* For RSA primes R < 2*mod, so the sum is < 3*mod.  Two conditional
   * subtractions are sufficient. */
  for (int pass = 0; pass < 2; ++pass) {
    uint64_t sub[FIO_RSA_MAX_WORDS];
    uint64_t borrow = fio_math_sub(sub, out, mod, mod_word_count);
    uint64_t use_sub = (uint64_t)0 - (borrow ^ 1);
    uint64_t use_out = (uint64_t)0 - borrow;
    for (size_t i = 0; i < mod_word_count; ++i) {
      out[i] = (sub[i] & use_sub) | (out[i] & use_out);
    }
  }

  fio_secure_zero(R_ext, sizeof(R_ext));
  fio_secure_zero(mod_ext, sizeof(mod_ext));
  fio_secure_zero(R_mod_mod, sizeof(R_mod_mod));
  fio_secure_zero(m_high, sizeof(m_high));
}

/**
 * Generate a random odd value in [1, mod-1] where mod is odd.
 *
 * The output is mod_word_count little-endian words.  The value is reduced
 * modulo mod in constant time and then forced odd, so it is always in the
 * required range and never zero.
 */
FIO_SFUNC void fio___rsa_random_mod(uint64_t *out,
                                    const uint64_t *mod,
                                    size_t mod_word_count) {
  uint8_t bytes[FIO_RSA_MAX_BYTES];
  size_t byte_len = mod_word_count * sizeof(uint64_t);

  if (fio_rand_bytes_secure(bytes, byte_len) != 0)
    fio_rand_bytes(bytes, byte_len);

  fio___rsa_bytes_to_words(out, mod_word_count, bytes, byte_len);

  /* Reduce out modulo mod in constant time: out = out - mod if out >= mod. */
  uint64_t sub[FIO_RSA_MAX_WORDS];
  uint64_t borrow = fio_math_sub(sub, out, mod, mod_word_count);
  uint64_t use_sub = (uint64_t)0 - (borrow ^ 1);
  uint64_t use_out = (uint64_t)0 - borrow;
  for (size_t i = 0; i < mod_word_count; ++i)
    out[i] = (sub[i] & use_sub) | (out[i] & use_out);

  /* Force odd and non-zero.  If mod-1 was even, the OR can produce mod;
   * subtract mod branchlessly in that case and map the zero result to one. */
  out[0] |= 1;
  borrow = fio_math_sub(sub, out, mod, mod_word_count);
  use_sub = (uint64_t)0 - (borrow ^ 1);
  use_out = (uint64_t)0 - borrow;
  uint64_t diff = 0;
  for (size_t i = 0; i < mod_word_count; ++i) {
    out[i] = (sub[i] & use_sub) | (out[i] & use_out);
    diff |= out[i];
  }
  out[0] |= (uint64_t)fio_ct_false(diff);

  fio_secure_zero(bytes, sizeof(bytes));
  fio_secure_zero(sub, sizeof(sub));
}

/**
 * Modular multiplication in normal form using Montgomery reduction.
 *
 * result = a * b mod mod.  a and b are in normal form.  The computation is
 * constant-time with respect to the values of a and b.
 */
FIO_SFUNC void fio___rsa_mul_mod(uint64_t *result,
                                 const uint64_t *a,
                                 const uint64_t *b,
                                 const uint64_t *mod,
                                 size_t mod_word_count) {
  uint64_t n_prime[FIO_RSA_MAX_WORDS];
  uint64_t r2modn[FIO_RSA_MAX_WORDS];
  fio___rsa_mont_setup(mod, mod_word_count, n_prime, r2modn);

  uint64_t a_mont[FIO_RSA_MAX_WORDS];
  uint64_t b_mont[FIO_RSA_MAX_WORDS];
  uint64_t one[FIO_RSA_MAX_WORDS];
  FIO_MEMSET(one, 0, sizeof(one));
  one[0] = 1;

  fio___rsa_mont_mul(a_mont, a, r2modn, mod, n_prime, mod_word_count);
  fio___rsa_mont_mul(b_mont, b, r2modn, mod, n_prime, mod_word_count);

  uint64_t ab_mont[FIO_RSA_MAX_WORDS];
  fio___rsa_mont_mul(ab_mont, a_mont, b_mont, mod, n_prime, mod_word_count);
  fio___rsa_mont_mul(result, ab_mont, one, mod, n_prime, mod_word_count);

  fio_secure_zero(n_prime, sizeof(n_prime));
  fio_secure_zero(r2modn, sizeof(r2modn));
  fio_secure_zero(a_mont, sizeof(a_mont));
  fio_secure_zero(b_mont, sizeof(b_mont));
  fio_secure_zero(ab_mont, sizeof(ab_mont));
}

/**
 * SOS Montgomery multiplication: result = a * b * R^-1 mod n.
 *
 * Requires n to be odd and n_prime to satisfy n * n_prime == -1 (mod R),
 * where R = 2^(64 * word_count).
 */
FIO_SFUNC void fio___rsa_mont_mul(uint64_t *result,
                                   const uint64_t *a,
                                   const uint64_t *b,
                                   const uint64_t *n,
                                   const uint64_t *n_prime,
                                   size_t word_count) {
  uint64_t t[FIO_RSA_MAX_WORDS * 2];
  uint64_t m_full[FIO_RSA_MAX_WORDS * 2];
  uint64_t mn[FIO_RSA_MAX_WORDS * 2];
  uint64_t sum[FIO_RSA_MAX_WORDS * 2];
  uint64_t sub[FIO_RSA_MAX_WORDS];

  /* T = a * b. Use the branchless long-multiplication helper because the
   * operands here are secret (ladder state / base). */
  fio___math_mul_long(t, a, b, word_count);

  /* m = (T_low * n') mod R (low word_count words of the product) */
  fio___math_mul_long(m_full, t, n_prime, word_count);

  /* m * n */
  fio___math_mul_long(mn, m_full, n, word_count);

  /* sum = T + m*n */
  const uint64_t high_carry = fio_math_add(sum, t, mn, word_count * 2);

  /* U = sum / R.  The low half is zero because sum is divisible by R. */
  const uint64_t borrow = fio_math_sub(sub, sum + word_count, n, word_count);
  const uint64_t use_sub = (borrow ^ 1) | high_carry;
  const uint64_t keep = (uint64_t)0 - (use_sub == 0);

  for (size_t i = 0; i < word_count; ++i) {
    result[i] = (sum[word_count + i] & keep) | (sub[i] & ~keep);
  }
}

/**
 * Precompute Montgomery constants for an odd modulus n.
 *
 * On output:
 *   n_prime satisfies n * n_prime == -1 (mod R), where R = 2^(64*w).
 *   r2modn is R^2 mod n.
 */
FIO_SFUNC void fio___rsa_mont_setup(const uint64_t *n,
                                     size_t word_count,
                                     uint64_t *n_prime,
                                     uint64_t *r2modn) {
  uint64_t n_ext[FIO_RSA_MAX_WORDS * 2];
  uint64_t tmp[FIO_RSA_MAX_WORDS * 2];
  uint64_t prod[FIO_RSA_MAX_WORDS * 2];

  /* n' = -n^{-1} mod R, lifted from the word inverse by Newton iteration. */
  const uint64_t n0_inv = fio___rsa_inv_mod_2pow64(n[0]);
  FIO_MEMSET(n_prime, 0, word_count * sizeof(uint64_t));
  n_prime[0] = (uint64_t)0 - n0_inv;

  for (int iter = 0; iter < 16; ++iter) {
    /* prod = n * n_prime + 2  (Newton step for n' satisfying n*n' == -1 mod R) */
    fio_math_mul(prod, n, n_prime, word_count);
    uint64_t c = 0;
    prod[0] = fio_math_addc64(prod[0], 2, 0, &c);
    for (size_t j = 1; j < word_count * 2; ++j) {
      prod[j] = fio_math_addc64(prod[j], 0, c, &c);
    }
    /* n_prime = n_prime * prod mod R (low words only) */
    fio_math_mul(tmp, n_prime, prod, word_count);
    FIO_MEMCPY(n_prime, tmp, word_count * sizeof(uint64_t));
  }

  /* R mod n, where R = 2^(64*word_count) */
  FIO_MEMSET(n_ext, 0, word_count * 2 * sizeof(uint64_t));
  FIO_MEMCPY(n_ext, n, word_count * sizeof(uint64_t));
  FIO_MEMSET(tmp, 0, word_count * 2 * sizeof(uint64_t));
  tmp[word_count] = 1ULL;

  uint64_t rmodn_ext[FIO_RSA_MAX_WORDS * 2];
  fio_math_div(NULL, rmodn_ext, tmp, n_ext, word_count * 2);

  /* R^2 mod n = (R mod n)^2 mod n */
  fio_math_mul(tmp, rmodn_ext, rmodn_ext, word_count);
  uint64_t r2_ext[FIO_RSA_MAX_WORDS * 2];
  fio_math_div(NULL, r2_ext, tmp, n_ext, word_count * 2);
  FIO_MEMCPY(r2modn, r2_ext, word_count * sizeof(uint64_t));
}

/* *****************************************************************************
Implementation - Modular Exponentiation

Compute: result = base^exp mod n

Uses a constant-time Montgomery ladder with SOS Montgomery multiplication.
The same sequence of operations is executed for every exponent bit.
***************************************************************************** */

/**
 * Constant-time modular exponentiation: result = base^exp mod n
 *
 * Uses a Montgomery ladder with Separated-Operand-Scanning (SOS) Montgomery
 * multiplication.  The loop processes every bit position of the exponent,
 * performing the same sequence of operations regardless of the exponent bit.
 * No branch is taken on secret data.
 */
FIO_SFUNC void fio___rsa_modexp(uint64_t *result,
                                const uint64_t *base,
                                const uint64_t *exp,
                                const uint64_t *n,
                                size_t word_count) {
  if (!word_count)
    return;

  FIO_ASSERT(word_count <= FIO_RSA_MAX_WORDS,
             "RSA key size exceeds maximum supported");

  uint64_t n_prime[FIO_RSA_MAX_WORDS];
  uint64_t r2modn[FIO_RSA_MAX_WORDS];
  fio___rsa_mont_setup(n, word_count, n_prime, r2modn);

  uint64_t one[FIO_RSA_MAX_WORDS];
  uint64_t base_mont[FIO_RSA_MAX_WORDS];
  uint64_t r0[FIO_RSA_MAX_WORDS];
  uint64_t r1[FIO_RSA_MAX_WORDS];
  uint64_t tmp[FIO_RSA_MAX_WORDS];

  FIO_MEMSET(one, 0, word_count * sizeof(uint64_t));
  one[0] = 1;

  /* Convert base and 1 into Montgomery representation. */
  fio___rsa_mont_mul(base_mont, base, r2modn, n, n_prime, word_count);
  fio___rsa_mont_mul(r0, one, r2modn, n, n_prime, word_count);
  FIO_MEMCPY(r1, base_mont, word_count * sizeof(uint64_t));

  /* Montgomery ladder: process all exponent bits from MSB to LSB. */
  const int total_bits = (int)(word_count * 64);
  for (int i = total_bits - 1; i >= 0; --i) {
    const uint64_t b = (exp[(unsigned)i >> 6] >> (i & 63)) & 1ULL;

    fio___rsa_cswap_words(r0, r1, b, word_count);
    fio___rsa_mont_mul(tmp, r0, r1, n, n_prime, word_count);
    FIO_MEMCPY(r1, tmp, word_count * sizeof(uint64_t));
    fio___rsa_mont_mul(tmp, r0, r0, n, n_prime, word_count);
    FIO_MEMCPY(r0, tmp, word_count * sizeof(uint64_t));
    fio___rsa_cswap_words(r0, r1, b, word_count);
  }

  /* Convert result out of Montgomery representation. */
  fio___rsa_mont_mul(result, r0, one, n, n_prime, word_count);

  fio_secure_zero(n_prime, sizeof(n_prime));
  fio_secure_zero(r2modn, sizeof(r2modn));
  fio_secure_zero(base_mont, sizeof(base_mont));
  fio_secure_zero(r0, sizeof(r0));
  fio_secure_zero(r1, sizeof(r1));
  fio_secure_zero(tmp, sizeof(tmp));
}

/**
 * Optimized modular exponentiation for e = 65537 (0x10001).
 *
 * This is the most common RSA public exponent. It has only 2 bits set,
 * so we can do: result = base^65537 = base^(2^16 + 1) = base^(2^16) * base
 *
 * This requires 16 squarings + 1 multiplication.
 */
FIO_SFUNC int fio___rsa_modexp_65537(uint64_t *result,
                                     const uint64_t *base,
                                     const uint64_t *n,
                                     size_t word_count) {
/* Double-size buffer for multiplication results */
#if !defined(_MSC_VER) && (!defined(__cplusplus) || __cplusplus > 201402L)
  uint64_t tmp[word_count * 2];
  uint64_t sqr[word_count];
  uint64_t rem[word_count * 2];
  uint64_t n_ext[word_count * 2];
#else
  uint64_t tmp[FIO_RSA_MAX_WORDS * 2];
  uint64_t sqr[FIO_RSA_MAX_WORDS];
  uint64_t rem[FIO_RSA_MAX_WORDS * 2];
  uint64_t n_ext[FIO_RSA_MAX_WORDS * 2];
  FIO_ASSERT(word_count <= FIO_RSA_MAX_WORDS,
             "RSA key size exceeds maximum supported");
#endif

  /* Zero-extend n to double size for use with fio_math_div */
  FIO_MEMCPY(n_ext, n, word_count * sizeof(uint64_t));
  FIO_MEMSET(n_ext + word_count, 0, word_count * sizeof(uint64_t));

  /* sqr = base (mod n is already ensured by caller if base < n) */
  FIO_MEMCPY(sqr, base, word_count * sizeof(uint64_t));

  /* Square 16 times to get base^(2^16) */
  for (int i = 0; i < 16; ++i) {
    fio_math_mul(tmp, sqr, sqr, word_count);
    /* Reduce mod n */
    FIO_MEMSET(rem, 0, word_count * 2 * sizeof(uint64_t));
    fio_math_div(NULL, rem, tmp, n_ext, word_count * 2);
    FIO_MEMCPY(sqr, rem, word_count * sizeof(uint64_t));
  }

  /* result = sqr * base (mod n) = base^(2^16) * base = base^65537 */
  fio_math_mul(tmp, sqr, base, word_count);
  FIO_MEMSET(rem, 0, word_count * 2 * sizeof(uint64_t));
  fio_math_div(NULL, rem, tmp, n_ext, word_count * 2);
  FIO_MEMCPY(result, rem, word_count * sizeof(uint64_t));

  return 0;
}

/**
 * RSA public key operation: result = sig^e mod n
 *
 * Verifies the signature by computing the modular exponentiation and
 * returns the result (which should match the padded hash).
 */
FIO_SFUNC int fio___rsa_public_op(uint8_t *result,
                                  const uint8_t *sig,
                                  size_t sig_len,
                                  const fio_rsa_pubkey_s *key) {
  if (!result || !sig || !key || !key->n || !key->e)
    return -1;

  /* Validate key size */
  if (key->n_len > FIO_RSA_MAX_BYTES || key->n_len < 256)
    return -1; /* Only support 2048-4096 bit keys */

  if (sig_len != key->n_len)
    return -1; /* Signature must be same length as modulus */

  size_t word_count = (key->n_len + 7) / 8;

#if !defined(_MSC_VER) && (!defined(__cplusplus) || __cplusplus > 201402L)
  uint64_t n_words[word_count];
  uint64_t sig_words[word_count];
  uint64_t result_words[word_count];
#else
  uint64_t n_words[FIO_RSA_MAX_WORDS];
  uint64_t sig_words[FIO_RSA_MAX_WORDS];
  uint64_t result_words[FIO_RSA_MAX_WORDS];
#endif

  /* Convert to internal representation */
  fio___rsa_bytes_to_words(n_words, word_count, key->n, key->n_len);
  fio___rsa_bytes_to_words(sig_words, word_count, sig, sig_len);

  /* Verify signature < modulus */
  if (fio___rsa_cmp(sig_words, n_words, word_count) >= 0)
    return -1;

  /* Check if exponent is 65537 (common case) */
  uint64_t e_val = 0;
  if (key->e_len <= 8) {
    for (size_t i = 0; i < key->e_len; ++i)
      e_val = (e_val << 8) | key->e[i];
  }

  if (e_val == 65537) {
    /* Use optimized path for e = 65537 */
    if (fio___rsa_modexp_65537(result_words, sig_words, n_words, word_count) !=
        0)
      return -1;
  } else {
    /* General case - use square-and-multiply */
#if !defined(_MSC_VER) && (!defined(__cplusplus) || __cplusplus > 201402L)
    uint64_t e_words[word_count];
#else
    uint64_t e_words[FIO_RSA_MAX_WORDS];
#endif
    FIO_MEMSET(e_words, 0, word_count * sizeof(uint64_t));
    fio___rsa_bytes_to_words(e_words, word_count, key->e, key->e_len);
    fio___rsa_modexp(result_words, sig_words, e_words, n_words, word_count);
  }

  /* Convert back to bytes */
  fio___rsa_words_to_bytes(result, key->n_len, result_words, word_count);

  return 0;
}

/* *****************************************************************************
Implementation - PKCS#1 v1.5 Signature Verification
***************************************************************************** */

/**
 * Constant-time memory comparison.
 * Returns 0 if equal, non-zero otherwise.
 */
FIO_SFUNC int fio___rsa_memcmp_ct(const uint8_t *a,
                                  const uint8_t *b,
                                  size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; ++i)
    diff |= a[i] ^ b[i];
  return diff;
}

SFUNC int fio_rsa_verify_pkcs1(const uint8_t *sig,
                               size_t sig_len,
                               const uint8_t *msg_hash,
                               size_t hash_len,
                               fio_rsa_hash_e hash_alg,
                               const fio_rsa_pubkey_s *key) {
  if (!sig || !msg_hash || !key)
    return -1;

  /* Validate hash algorithm and length */
  const uint8_t *digestinfo_prefix;
  size_t expected_hash_len;

  switch (hash_alg) {
  case FIO_RSA_HASH_SHA256:
    digestinfo_prefix = fio___rsa_digestinfo_sha256;
    expected_hash_len = 32;
    break;
  case FIO_RSA_HASH_SHA384:
    digestinfo_prefix = fio___rsa_digestinfo_sha384;
    expected_hash_len = 48;
    break;
  case FIO_RSA_HASH_SHA512:
    digestinfo_prefix = fio___rsa_digestinfo_sha512;
    expected_hash_len = 64;
    break;
  default: return -1;
  }

  if (hash_len != expected_hash_len)
    return -1;

  /* Compute sig^e mod n */
  uint8_t decrypted[FIO_RSA_MAX_BYTES];
  if (fio___rsa_public_op(decrypted, sig, sig_len, key) != 0)
    return -1;

  /*
   * Verify PKCS#1 v1.5 padding (RFC 8017 Section 8.2.2):
   *
   * EM = 0x00 || 0x01 || PS || 0x00 || DigestInfo
   *
   * where PS is padding bytes (all 0xFF), minimum 8 bytes
   * DigestInfo = prefix || hash
   */

  size_t k = key->n_len; /* Encoded message length */
  size_t digestinfo_len = FIO___RSA_DIGESTINFO_PREFIX_LEN + hash_len;
  size_t ps_len = k - 3 - digestinfo_len;

  if (k < 11 + digestinfo_len)
    return -1; /* Message too short */

  /* Verify: 0x00 || 0x01 */
  int result = 0;
  result |= decrypted[0];        /* Must be 0x00 */
  result |= decrypted[1] ^ 0x01; /* Must be 0x01 */

  /* Verify PS (all 0xFF bytes) */
  for (size_t i = 2; i < 2 + ps_len; ++i)
    result |= decrypted[i] ^ 0xFF;

  /* Verify separator 0x00 */
  result |= decrypted[2 + ps_len];

  /* Verify DigestInfo prefix */
  result |= fio___rsa_memcmp_ct(decrypted + 3 + ps_len,
                                digestinfo_prefix,
                                FIO___RSA_DIGESTINFO_PREFIX_LEN);

  /* Verify hash */
  result |= fio___rsa_memcmp_ct(decrypted + 3 + ps_len +
                                    FIO___RSA_DIGESTINFO_PREFIX_LEN,
                                msg_hash,
                                hash_len);

  return result ? -1 : 0;
}

/* *****************************************************************************
Implementation - RSA-PSS Signature Verification
***************************************************************************** */

/**
 * MGF1 (Mask Generation Function) per RFC 8017 Appendix B.2.1.
 *
 * mask = Hash(seed || counter) for counter = 0, 1, 2, ...
 */
FIO_SFUNC void fio___rsa_mgf1(uint8_t *mask,
                              size_t mask_len,
                              const uint8_t *seed,
                              size_t seed_len,
                              fio_rsa_hash_e hash_alg) {
  size_t hash_len;
  switch (hash_alg) {
  case FIO_RSA_HASH_SHA256: hash_len = 32; break;
  case FIO_RSA_HASH_SHA384: hash_len = 48; break;
  case FIO_RSA_HASH_SHA512: hash_len = 64; break;
  default: return;
  }

  uint8_t counter_buf[4];
  uint8_t hash_input[FIO_RSA_MAX_BYTES + 4]; /* seed || counter */
  size_t offset = 0;
  uint32_t counter = 0;

  FIO_MEMCPY(hash_input, seed, seed_len);

  while (offset < mask_len) {
    /* Append counter (big-endian) */
    counter_buf[0] = (uint8_t)(counter >> 24);
    counter_buf[1] = (uint8_t)(counter >> 16);
    counter_buf[2] = (uint8_t)(counter >> 8);
    counter_buf[3] = (uint8_t)(counter);
    FIO_MEMCPY(hash_input + seed_len, counter_buf, 4);

    /* Hash and copy to output */
    size_t to_copy = mask_len - offset;
    if (to_copy > hash_len)
      to_copy = hash_len;

    switch (hash_alg) {
    case FIO_RSA_HASH_SHA256: {
      fio_u256 h = fio_sha256(hash_input, seed_len + 4);
      FIO_MEMCPY(mask + offset, h.u8, to_copy);
      break;
    }
    case FIO_RSA_HASH_SHA384: {
      fio_u512 h = fio_sha512(hash_input, seed_len + 4);
      /* SHA-384 is truncated SHA-512 */
      FIO_MEMCPY(mask + offset, h.u8, to_copy);
      break;
    }
    case FIO_RSA_HASH_SHA512: {
      fio_u512 h = fio_sha512(hash_input, seed_len + 4);
      FIO_MEMCPY(mask + offset, h.u8, to_copy);
      break;
    }
    }

    offset += to_copy;
    ++counter;
  }
}

SFUNC int fio_rsa_verify_pss(const uint8_t *sig,
                             size_t sig_len,
                             const uint8_t *msg_hash,
                             size_t hash_len,
                             fio_rsa_hash_e hash_alg,
                             const fio_rsa_pubkey_s *key) {
  if (!sig || !msg_hash || !key)
    return -1;

  /* Validate hash algorithm and length */
  size_t expected_hash_len;
  switch (hash_alg) {
  case FIO_RSA_HASH_SHA256: expected_hash_len = 32; break;
  case FIO_RSA_HASH_SHA384: expected_hash_len = 48; break;
  case FIO_RSA_HASH_SHA512: expected_hash_len = 64; break;
  default: return -1;
  }

  if (hash_len != expected_hash_len)
    return -1;

  /* For TLS 1.3, salt length = hash length */
  size_t salt_len = hash_len;

  /* Compute sig^e mod n */
  uint8_t em[FIO_RSA_MAX_BYTES];
  if (fio___rsa_public_op(em, sig, sig_len, key) != 0)
    return -1;

  /*
   * RSA-PSS Verification (RFC 8017 Section 9.1.2):
   *
   * EM = maskedDB || H || 0xBC
   *
   * where:
   *   - maskedDB = DB XOR dbMask
   *   - H = Hash(M')
   *   - M' = 0x00 00 00 00 00 00 00 00 || mHash || salt
   *   - DB = PS || 0x01 || salt
   *   - PS = zero bytes
   */

  size_t em_len = key->n_len;
  size_t db_len = em_len - hash_len - 1;

  /* Compute top-mask from the actual modulus bit length (RFC 8017 9.1.2). */
  size_t word_count = (key->n_len + 7) / 8;
  uint64_t n_words[FIO_RSA_MAX_WORDS];
  fio___rsa_bytes_to_words(n_words, word_count, key->n, key->n_len);
  size_t em_bits = fio_math_msb_index(n_words, word_count);
  if (em_bits == (size_t)-1)
    em_bits = 0;
  size_t top_mask = 0xFF >> (8 * em_len - em_bits);

  /* Accumulate all PSS validation checks in constant time.
   * No return statement appears after the public-key operation. */
  int result = 0;

  /* Verify trailing byte is 0xBC */
  result |= em[em_len - 1] ^ 0xBC;

  /* The top bits of EM should be 0 (based on modulus bit length) */
  result |= em[0] & ~top_mask;

  /* Extract H (hash) from EM */
  const uint8_t *masked_db = em;
  const uint8_t *h = em + db_len;

  /* Compute dbMask = MGF1(H, db_len) */
  uint8_t db_mask[FIO_RSA_MAX_BYTES];
  fio___rsa_mgf1(db_mask, db_len, h, hash_len, hash_alg);

  /* Compute DB = maskedDB XOR dbMask */
  uint8_t db[FIO_RSA_MAX_BYTES];
  db[0] = 0;
  for (size_t i = 0; i < db_len; ++i)
    db[i] = masked_db[i] ^ db_mask[i];

  /* Clear top bits of DB */
  db[0] &= top_mask;

  /* Verify DB = PS || 0x01 || salt */
  /* PS should be all zeros, length = db_len - salt_len - 1 */
  size_t ps_len = db_len - salt_len - 1;

  /* Verify PS (all zeros) */
  for (size_t i = 0; i < ps_len; ++i)
    result |= db[i];

  /* Verify separator 0x01 */
  result |= db[ps_len] ^ 0x01;

  /* Extract salt */
  const uint8_t *salt = db + ps_len + 1;

  /* Compute M' = 0x0000000000000000 || mHash || salt */
  uint8_t m_prime[8 + 64 + 64]; /* max: 8 + hash_len + salt_len */
  FIO_MEMSET(m_prime, 0, 8);
  FIO_MEMCPY(m_prime + 8, msg_hash, hash_len);
  FIO_MEMCPY(m_prime + 8 + hash_len, salt, salt_len);

  /* Compute H' = Hash(M') */
  uint8_t h_prime[64];
  switch (hash_alg) {
  case FIO_RSA_HASH_SHA256: {
    fio_u256 hp = fio_sha256(m_prime, 8 + hash_len + salt_len);
    fio_memcpy32(h_prime, hp.u8);
    break;
  }
  case FIO_RSA_HASH_SHA384: {
    fio_u512 hp = fio_sha512(m_prime, 8 + hash_len + salt_len);
    FIO_MEMCPY(h_prime, hp.u8, 48);
    break;
  }
  case FIO_RSA_HASH_SHA512: {
    fio_u512 hp = fio_sha512(m_prime, 8 + hash_len + salt_len);
    fio_memcpy64(h_prime, hp.u8);
    break;
  }
  }

  /* Verify H == H' */
  result |= fio___rsa_memcmp_ct(h, h_prime, hash_len);

  /* Single exit point; branchless on the accumulated validation result. */
  return -1 + (!result);
}

/* *****************************************************************************
Implementation - RSA-PSS Signature Generation (RFC 8017 Section 8.1)
***************************************************************************** */

/**
 * Non-CRT private key operation.
 *
 * Falls back to the plain constant-time Montgomery ladder.  A non-CRT
 * blinding path would require a constant-time modular inverse modulo the
 * composite n, which is not implemented here; production keys should supply
 * CRT parameters so the CRT+blinding path is used instead.
 */
FIO_SFUNC int fio___rsa_private_op_non_crt(uint64_t *result_words,
                                           const uint64_t *m_words,
                                           const uint64_t *d_words,
                                           const uint64_t *n_words,
                                           size_t word_count) {
  fio___rsa_modexp(result_words, m_words, d_words, n_words, word_count);
  return 0;
}

/**
 * CRT private key operation with RSA blinding.
 *
 * Requires p, q, dP, dQ, qInv and the public exponent e.  Blinding is done
 * independently modulo p and q, then the two halves are recombined with the
 * CRT formula.  All scalar operations are constant-time.
 */
FIO_SFUNC int fio___rsa_private_op_crt(uint64_t *result_words,
                                       const uint64_t *m_words,
                                       const fio_rsa_privkey_s *key,
                                       size_t word_count) {
  size_t p_word_count = (key->p_len + 7) / 8;
  size_t q_word_count = (key->q_len + 7) / 8;

  if (!p_word_count || p_word_count > FIO_RSA_MAX_WORDS / 2 ||
      !q_word_count || q_word_count > FIO_RSA_MAX_WORDS / 2)
    return -1;

  uint64_t n_words[FIO_RSA_MAX_WORDS];
  fio___rsa_bytes_to_words(n_words, word_count, key->n, key->n_len);

  uint64_t p[FIO_RSA_MAX_WORDS];
  uint64_t q[FIO_RSA_MAX_WORDS];
  uint64_t dP[FIO_RSA_MAX_WORDS];
  uint64_t dQ[FIO_RSA_MAX_WORDS];
  uint64_t qInv[FIO_RSA_MAX_WORDS];
  uint64_t e_p[FIO_RSA_MAX_WORDS];
  uint64_t e_q[FIO_RSA_MAX_WORDS];

  FIO_MEMSET(p, 0, sizeof(p));
  FIO_MEMSET(q, 0, sizeof(q));
  FIO_MEMSET(dP, 0, sizeof(dP));
  FIO_MEMSET(dQ, 0, sizeof(dQ));
  FIO_MEMSET(qInv, 0, sizeof(qInv));
  FIO_MEMSET(e_p, 0, sizeof(e_p));
  FIO_MEMSET(e_q, 0, sizeof(e_q));

  fio___rsa_bytes_to_words(p, p_word_count, key->p, key->p_len);
  fio___rsa_bytes_to_words(q, q_word_count, key->q, key->q_len);
  fio___rsa_bytes_to_words(dP, p_word_count, key->dP, key->dP_len);
  fio___rsa_bytes_to_words(dQ, q_word_count, key->dQ, key->dQ_len);
  fio___rsa_bytes_to_words(qInv, p_word_count, key->qInv, key->qInv_len);
  fio___rsa_bytes_to_words(e_p, p_word_count, key->e, key->e_len);
  fio___rsa_bytes_to_words(e_q, q_word_count, key->e, key->e_len);

  /* m_p = m mod p, m_q = m mod q */
  uint64_t m_p[FIO_RSA_MAX_WORDS];
  uint64_t m_q[FIO_RSA_MAX_WORDS];
  fio___rsa_reduce_mod(m_p, m_words, p, p_word_count, word_count);
  fio___rsa_reduce_mod(m_q, m_words, q, q_word_count, word_count);

  /* Random blinding factors r_p, r_q */
  uint64_t r_p[FIO_RSA_MAX_WORDS];
  uint64_t r_q[FIO_RSA_MAX_WORDS];
  fio___rsa_random_mod(r_p, p, p_word_count);
  fio___rsa_random_mod(r_q, q, q_word_count);

  /* m'_p = r_p^e * m_p mod p, m'_q = r_q^e * m_q mod q */
  uint64_t r_e_p[FIO_RSA_MAX_WORDS];
  uint64_t r_e_q[FIO_RSA_MAX_WORDS];
  uint64_t m_blind_p[FIO_RSA_MAX_WORDS];
  uint64_t m_blind_q[FIO_RSA_MAX_WORDS];
  fio___rsa_modexp(r_e_p, r_p, e_p, p, p_word_count);
  fio___rsa_modexp(r_e_q, r_q, e_q, q, q_word_count);
  fio___rsa_mul_mod(m_blind_p, r_e_p, m_p, p, p_word_count);
  fio___rsa_mul_mod(m_blind_q, r_e_q, m_q, q, q_word_count);

  /* s'_p = m'_p^dP mod p, s'_q = m'_q^dQ mod q */
  uint64_t s_blind_p[FIO_RSA_MAX_WORDS];
  uint64_t s_blind_q[FIO_RSA_MAX_WORDS];
  fio___rsa_modexp(s_blind_p, m_blind_p, dP, p, p_word_count);
  fio___rsa_modexp(s_blind_q, m_blind_q, dQ, q, q_word_count);

  /* Unblind using Fermat's little theorem: r^-1 = r^(mod-2) mod mod. */
  uint64_t one[FIO_RSA_MAX_WORDS];
  FIO_MEMSET(one, 0, sizeof(one));
  one[0] = 1;

  uint64_t p_minus_2[FIO_RSA_MAX_WORDS];
  uint64_t q_minus_2[FIO_RSA_MAX_WORDS];
  FIO_MEMSET(p_minus_2, 0, sizeof(p_minus_2));
  FIO_MEMSET(q_minus_2, 0, sizeof(q_minus_2));
  FIO_MEMCPY(p_minus_2, p, p_word_count * sizeof(uint64_t));
  FIO_MEMCPY(q_minus_2, q, q_word_count * sizeof(uint64_t));
  (void)fio_math_sub(p_minus_2, p_minus_2, one, p_word_count); /* p-1 */
  (void)fio_math_sub(p_minus_2, p_minus_2, one, p_word_count); /* p-2 */
  (void)fio_math_sub(q_minus_2, q_minus_2, one, q_word_count); /* q-1 */
  (void)fio_math_sub(q_minus_2, q_minus_2, one, q_word_count); /* q-2 */

  uint64_t r_inv_p[FIO_RSA_MAX_WORDS];
  uint64_t r_inv_q[FIO_RSA_MAX_WORDS];
  uint64_t s_p[FIO_RSA_MAX_WORDS];
  uint64_t s_q[FIO_RSA_MAX_WORDS];
  fio___rsa_modexp(r_inv_p, r_p, p_minus_2, p, p_word_count);
  fio___rsa_modexp(r_inv_q, r_q, q_minus_2, q, q_word_count);
  fio___rsa_mul_mod(s_p, s_blind_p, r_inv_p, p, p_word_count);
  fio___rsa_mul_mod(s_q, s_blind_q, r_inv_q, q, q_word_count);

  /* CRT recombination: h = qInv * (s_p - s_q) mod p,
   *                    s = s_q + q * h mod n */
  uint64_t diff[FIO_RSA_MAX_WORDS];
  uint64_t h[FIO_RSA_MAX_WORDS];
  uint64_t p_mask[FIO_RSA_MAX_WORDS];
  uint64_t borrow = fio_math_sub(diff, s_p, s_q, p_word_count);
  uint64_t use_add = (uint64_t)0 - borrow;
  for (size_t i = 0; i < p_word_count; ++i)
    p_mask[i] = p[i] & use_add;
  (void)fio_math_add(diff, diff, p_mask, p_word_count);
  fio___rsa_mul_mod(h, qInv, diff, p, p_word_count);

  /* q * h mod n (zero-extend q and h to word_count) */
  uint64_t q_ext[FIO_RSA_MAX_WORDS];
  uint64_t h_ext[FIO_RSA_MAX_WORDS];
  uint64_t s_q_ext[FIO_RSA_MAX_WORDS];
  FIO_MEMSET(q_ext, 0, word_count * sizeof(uint64_t));
  FIO_MEMSET(h_ext, 0, word_count * sizeof(uint64_t));
  FIO_MEMSET(s_q_ext, 0, word_count * sizeof(uint64_t));
  FIO_MEMCPY(q_ext, q, q_word_count * sizeof(uint64_t));
  FIO_MEMCPY(h_ext, h, p_word_count * sizeof(uint64_t));
  FIO_MEMCPY(s_q_ext, s_q, q_word_count * sizeof(uint64_t));

  uint64_t qh[FIO_RSA_MAX_WORDS];
  fio___rsa_mul_mod(qh, q_ext, h_ext, n_words, word_count);
  (void)fio_math_add(result_words, qh, s_q_ext, word_count);

  /* result may be in [0, 2*n); reduce once. */
  uint64_t sub_n[FIO_RSA_MAX_WORDS];
  uint64_t borrow_n = fio_math_sub(sub_n, result_words, n_words, word_count);
  uint64_t use_sub_n = (uint64_t)0 - (borrow_n ^ 1);
  uint64_t use_res = (uint64_t)0 - borrow_n;
  for (size_t i = 0; i < word_count; ++i)
    result_words[i] = (sub_n[i] & use_sub_n) | (result_words[i] & use_res);

  fio_secure_zero(p, sizeof(p));
  fio_secure_zero(q, sizeof(q));
  fio_secure_zero(dP, sizeof(dP));
  fio_secure_zero(dQ, sizeof(dQ));
  fio_secure_zero(qInv, sizeof(qInv));
  fio_secure_zero(e_p, sizeof(e_p));
  fio_secure_zero(e_q, sizeof(e_q));
  fio_secure_zero(m_p, sizeof(m_p));
  fio_secure_zero(m_q, sizeof(m_q));
  fio_secure_zero(r_p, sizeof(r_p));
  fio_secure_zero(r_q, sizeof(r_q));
  fio_secure_zero(r_e_p, sizeof(r_e_p));
  fio_secure_zero(r_e_q, sizeof(r_e_q));
  fio_secure_zero(m_blind_p, sizeof(m_blind_p));
  fio_secure_zero(m_blind_q, sizeof(m_blind_q));
  fio_secure_zero(s_blind_p, sizeof(s_blind_p));
  fio_secure_zero(s_blind_q, sizeof(s_blind_q));
  fio_secure_zero(r_inv_p, sizeof(r_inv_p));
  fio_secure_zero(r_inv_q, sizeof(r_inv_q));
  fio_secure_zero(s_p, sizeof(s_p));
  fio_secure_zero(s_q, sizeof(s_q));
  fio_secure_zero(p_minus_2, sizeof(p_minus_2));
  fio_secure_zero(q_minus_2, sizeof(q_minus_2));
  fio_secure_zero(diff, sizeof(diff));
  fio_secure_zero(h, sizeof(h));
  fio_secure_zero(p_mask, sizeof(p_mask));
  fio_secure_zero(qh, sizeof(qh));
  fio_secure_zero(n_words, sizeof(n_words));

  return 0;
}

/**
 * RSA private key operation: result = m^d mod n (RSASP1 per RFC 8017)
 *
 * Uses CRT with RSA blinding when CRT parameters and the public exponent are
 * available.  Falls back to the plain constant-time Montgomery ladder when
 * CRT parameters are unavailable.
 */
FIO_SFUNC int fio___rsa_private_op(uint8_t *result,
                                   const uint8_t *message,
                                   size_t msg_len,
                                   const fio_rsa_privkey_s *key) {
  if (!result || !message || !key || !key->n || !key->d)
    return -1;

  /* Validate key size */
  if (key->n_len > FIO_RSA_MAX_BYTES || key->n_len < 256)
    return -1; /* Only support 2048-4096 bit keys */

  if (msg_len != key->n_len)
    return -1; /* Message must be same length as modulus */

  size_t word_count = (key->n_len + 7) / 8;

  uint64_t n_words[FIO_RSA_MAX_WORDS];
  uint64_t d_words[FIO_RSA_MAX_WORDS];
  uint64_t msg_words[FIO_RSA_MAX_WORDS];
  uint64_t result_words[FIO_RSA_MAX_WORDS];

  FIO_MEMSET(n_words, 0, sizeof(n_words));
  FIO_MEMSET(d_words, 0, sizeof(d_words));
  FIO_MEMSET(msg_words, 0, sizeof(msg_words));
  FIO_MEMSET(result_words, 0, sizeof(result_words));

  /* Convert to internal representation */
  fio___rsa_bytes_to_words(n_words, word_count, key->n, key->n_len);
  fio___rsa_bytes_to_words(d_words, word_count, key->d, key->d_len);
  fio___rsa_bytes_to_words(msg_words, word_count, message, msg_len);

  int have_crt = (key->p && key->p_len && key->q && key->q_len &&
                  key->dP && key->dP_len && key->dQ && key->dQ_len &&
                  key->qInv && key->qInv_len && key->e && key->e_len);
  int rc = -1;

  /* Verify message < modulus (constant-time). */
  if (fio___rsa_ct_lt(msg_words, n_words, word_count) == 0)
    goto cleanup;

  if (have_crt) {
    rc = fio___rsa_private_op_crt(result_words, msg_words, key, word_count);
  } else {
    /* No CRT parameters: fall back to the plain constant-time Montgomery
     * ladder.  Production signing keys should supply CRT parameters so the
     * CRT+blinding path above is used. */
    rc = fio___rsa_private_op_non_crt(
        result_words, msg_words, d_words, n_words, word_count);
  }

  if (rc == 0)
    fio___rsa_words_to_bytes(result, key->n_len, result_words, word_count);

cleanup:
  /* Clear sensitive data */
  fio_secure_zero(n_words, sizeof(n_words));
  fio_secure_zero(d_words, sizeof(d_words));
  fio_secure_zero(msg_words, sizeof(msg_words));
  fio_secure_zero(result_words, sizeof(result_words));

  return rc;
}

/**
 * EMSA-PSS-ENCODE per RFC 8017 Section 9.1.1
 *
 * Encodes a message hash into the PSS format for signing.
 *
 * @param em        Output buffer for encoded message (em_len bytes)
 * @param em_len    Encoded message length (same as modulus length)
 * @param msg_hash  Pre-computed hash of the message
 * @param hash_len  Hash length (32, 48, or 64 bytes)
 * @param hash_alg  Hash algorithm used
 * @return 0 on success, -1 on error
 */
FIO_SFUNC int fio___rsa_emsa_pss_encode(uint8_t *em,
                                        size_t em_len,
                                        const uint8_t *msg_hash,
                                        size_t hash_len,
                                        fio_rsa_hash_e hash_alg,
                                        size_t top_mask) {
  if (!em || !msg_hash)
    return -1;

  /* For TLS 1.3, salt length = hash length */
  size_t salt_len = hash_len;

  /* Validate sizes:
   * em_len >= hash_len + salt_len + 2
   * For 2048-bit key (256 bytes) with SHA-256 (32 bytes):
   *   256 >= 32 + 32 + 2 = 66 ✓ */
  if (em_len < hash_len + salt_len + 2)
    return -1;

  /* Calculate DB length: em_len - hash_len - 1 */
  size_t db_len = em_len - hash_len - 1;

  /* Generate random salt using secure random */
  uint8_t salt[64]; /* Max salt length = max hash length */
  if (fio_rand_bytes_secure(salt, salt_len) != 0) {
    /* Fallback to pseudo-random if secure random fails */
    fio_rand_bytes(salt, salt_len);
  }

  /* Step 3: M' = (0x00 * 8) || mHash || salt */
  uint8_t m_prime[8 + 64 + 64]; /* 8 zeros + hash + salt */
  FIO_MEMSET(m_prime, 0, 8);
  FIO_MEMCPY(m_prime + 8, msg_hash, hash_len);
  FIO_MEMCPY(m_prime + 8 + hash_len, salt, salt_len);

  /* Step 4: H = Hash(M') */
  uint8_t h[64];
  switch (hash_alg) {
  case FIO_RSA_HASH_SHA256: {
    fio_u256 hp = fio_sha256(m_prime, 8 + hash_len + salt_len);
    fio_memcpy32(h, hp.u8);
    break;
  }
  case FIO_RSA_HASH_SHA384: {
    fio_sha512_s sha = fio_sha512_init();
    fio_sha512_consume(&sha, m_prime, 8 + hash_len + salt_len);
    fio_u512 hp = fio_sha512_finalize(&sha);
    FIO_MEMCPY(h, hp.u8, 48);
    break;
  }
  case FIO_RSA_HASH_SHA512: {
    fio_u512 hp = fio_sha512(m_prime, 8 + hash_len + salt_len);
    fio_memcpy64(h, hp.u8);
    break;
  }
  default: return -1;
  }

  /* Step 5: DB = PS || 0x01 || salt
   * PS = zeros of length (db_len - salt_len - 1) */
  uint8_t db[FIO_RSA_MAX_BYTES];
  size_t ps_len = db_len - salt_len - 1;
  FIO_MEMSET(db, 0, ps_len); /* PS = zeros */
  db[ps_len] = 0x01;         /* Separator */
  FIO_MEMCPY(db + ps_len + 1, salt, salt_len);

  /* Step 6: dbMask = MGF1(H, db_len) */
  uint8_t db_mask[FIO_RSA_MAX_BYTES];
  fio___rsa_mgf1(db_mask, db_len, h, hash_len, hash_alg);

  /* Step 7: maskedDB = DB XOR dbMask */
  for (size_t i = 0; i < db_len; ++i)
    em[i] = db[i] ^ db_mask[i];

  /* Step 8: Set leftmost bits of maskedDB to zero
   * based on the actual modulus bit length. */
  em[0] &= top_mask;

  /* Step 9: EM = maskedDB || H || 0xBC */
  FIO_MEMCPY(em + db_len, h, hash_len);
  em[em_len - 1] = 0xBC;

  /* Clear sensitive data */
  fio_secure_zero(salt, sizeof(salt));
  fio_secure_zero(m_prime, sizeof(m_prime));
  fio_secure_zero(h, sizeof(h));
  fio_secure_zero(db, sizeof(db));
  fio_secure_zero(db_mask, sizeof(db_mask));

  return 0;
}

/**
 * RSA-PSS signature generation (RSASSA-PSS-SIGN per RFC 8017 Section 8.1.1)
 */
SFUNC int fio_rsa_sign_pss(uint8_t *signature,
                           size_t *sig_len,
                           const uint8_t *msg_hash,
                           size_t hash_len,
                           fio_rsa_hash_e hash_alg,
                           const fio_rsa_privkey_s *key) {
  if (!signature || !sig_len || !msg_hash || !key)
    return -1;

  /* Validate hash algorithm and length */
  size_t expected_hash_len;
  switch (hash_alg) {
  case FIO_RSA_HASH_SHA256: expected_hash_len = 32; break;
  case FIO_RSA_HASH_SHA384: expected_hash_len = 48; break;
  case FIO_RSA_HASH_SHA512: expected_hash_len = 64; break;
  default: return -1;
  }

  if (hash_len != expected_hash_len)
    return -1;

  /* Validate key size */
  if (key->n_len > FIO_RSA_MAX_BYTES || key->n_len < 256)
    return -1;

  /* Compute top-mask from the actual modulus bit length. */
  size_t word_count = (key->n_len + 7) / 8;
  uint64_t n_words[FIO_RSA_MAX_WORDS];
  fio___rsa_bytes_to_words(n_words, word_count, key->n, key->n_len);
  size_t em_bits = fio_math_msb_index(n_words, word_count);
  if (em_bits == (size_t)-1)
    em_bits = 0;
  size_t top_mask = 0xFF >> (8 * key->n_len - em_bits);

  /* Step 1: EMSA-PSS-ENCODE */
  uint8_t em[FIO_RSA_MAX_BYTES];
  if (fio___rsa_emsa_pss_encode(
          em, key->n_len, msg_hash, hash_len, hash_alg, top_mask) != 0) {
    return -1;
  }

  /* Step 2-4: RSASP1 - compute s = m^d mod n */
  if (fio___rsa_private_op(signature, em, key->n_len, key) != 0) {
    fio_secure_zero(em, sizeof(em));
    return -1;
  }

  *sig_len = key->n_len;

  /* Clear sensitive data */
  fio_secure_zero(em, sizeof(em));
  fio_secure_zero(n_words, sizeof(n_words));

  FIO_LOG_DEBUG2("RSA-PSS: Generated %zu-byte signature", *sig_len);
  return 0;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_RSA */
#undef FIO_RSA
