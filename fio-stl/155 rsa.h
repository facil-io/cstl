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
 *
 * Leading zero bytes are ignored. Keys are rejected (-1) unless n is odd and
 * 256..512 bytes long and e is odd, >= 3 and at most 64 bits.
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
 * All optional fields are indicated by a non-zero length. CRT signing requires
 * all five CRT fields and e. When e is provided, every signature is verified
 * before it is returned, so faults or inconsistent keys return -1 instead of
 * emitting a signature.
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
 * @param sig_len    Output: signature length (modulus length without leading
 *                   zeros); unchanged on error
 * @param msg_hash   Pre-computed hash of the message to sign
 * @param hash_len   Hash length (32, 48, or 64 bytes)
 * @param hash_alg   Hash algorithm (FIO_RSA_HASH_SHA256, etc.)
 * @param key        RSA private key
 * @return 0 on success, -1 on error (invalid input or key, failed self-check)
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

/** Strip DER INTEGER sign-extension zero bytes from big-endian input. */
FIO_IFUNC void fio___rsa_trim_be(const uint8_t **bytes, size_t *byte_len) {
  while (*byte_len > 1 && **bytes == 0) {
    ++(*bytes);
    --(*byte_len);
  }
}

/** Compare two big integers (variable time, public values only). */
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

/** Constant-time less-than: returns 1 if a < b, else 0. */
FIO_SFUNC uint64_t fio___rsa_ct_lt(const uint64_t *a,
                                   const uint64_t *b,
                                   size_t word_count) {
  uint64_t borrow = 0, diff_ = 0; /* only the final borrow is needed */
  for (size_t i = 0; i < word_count; ++i)
    diff_ = fio_math_subc64(a[i], b[i], borrow, &borrow);
  (void)diff_;
  return borrow & 1;
}

/** Constant-time word comparison: returns 0 if equal, non-zero otherwise. */
FIO_SFUNC uint64_t fio___rsa_ct_neq(const uint64_t *a,
                                    const uint64_t *b,
                                    size_t word_count) {
  uint64_t diff = 0;
  for (size_t i = 0; i < word_count; ++i)
    diff |= a[i] ^ b[i];
  return diff;
}

/** Modular inverse of an odd 64-bit value modulo 2^64 (Newton-Raphson). */
FIO_SFUNC uint64_t fio___rsa_inv_mod_2pow64(uint64_t a) {
  uint64_t x = 1; /* a must be odd: precision doubles per step (1->64 bits) */
  for (int i = 0; i < 6; ++i)
    x = x * (2 - a * x);
  return x;
}

/** The value 1, zero padded to FIO_RSA_MAX_WORDS words. */
static const uint64_t fio___rsa_one[FIO_RSA_MAX_WORDS] = {1};

/* *****************************************************************************
Implementation - Montgomery Arithmetic (constant-time in operand values)

A context holds an odd modulus `n` (`len` words, R = 2^(64*len)), the word
constant n0 = -n^-1 mod 2^64 and R^2 mod n. Setup never divides, so secret
moduli (CRT primes) are safe; only the modulus bit length is branched on.
***************************************************************************** */

typedef struct {
  uint64_t n[FIO_RSA_MAX_WORDS];
  uint64_t r2[FIO_RSA_MAX_WORDS]; /* R^2 mod n */
  uint64_t n0;                    /* -n^-1 mod 2^64 */
  size_t len;
} fio___rsa_mont_s;

/** Branchless r = t - (n & mask) over `len` words (t has len + 1 words). */
FIO_IFUNC void fio___rsa_cond_sub(uint64_t *r,
                                  const uint64_t *t,
                                  uint64_t top,
                                  const uint64_t *n,
                                  size_t len) {
  uint64_t borrow = 0, diff_ = 0; /* compare pass: only the borrow is needed */
  for (size_t i = 0; i < len; ++i)
    diff_ = fio_math_subc64(t[i], n[i], borrow, &borrow);
  (void)diff_;
  /* subtract when t >= n (no borrow) or when a carry word is set */
  const uint64_t mask = (uint64_t)0 - ((top | (borrow ^ 1)) & 1);
  borrow = 0;
  for (size_t i = 0; i < len; ++i)
    r[i] = fio_math_subc64(t[i], n[i] & mask, borrow, &borrow);
}

/**
 * CIOS Montgomery multiplication: r = a * b * R^-1 mod n.
 *
 * Requires a < R and b < n; the output is fully reduced (< n). `r` may alias
 * `a` or `b`.
 */
FIO_SFUNC void fio___rsa_mont_mul(uint64_t *r,
                                  const uint64_t *a,
                                  const uint64_t *b,
                                  const fio___rsa_mont_s *m) {
  const size_t len = m->len;
  const uint64_t *n = m->n;
  uint64_t t[FIO_RSA_MAX_WORDS + 2];
  FIO_MEMSET(t, 0, (len + 2) * sizeof(uint64_t));
  for (size_t i = 0; i < len; ++i) {
    const uint64_t ai = a[i];
    uint64_t c = 0, hi, c1, c2, lo;
    for (size_t j = 0; j < len; ++j) { /* t += a[i] * b */
      lo = fio_math_mulc64(ai, b[j], &hi);
      lo = fio_math_addc64(lo, c, 0, &c1);
      t[j] = fio_math_addc64(t[j], lo, 0, &c2);
      c = hi + c1 + c2;
    }
    t[len] = fio_math_addc64(t[len], c, 0, &c1);
    t[len + 1] = c1;
    /* t = (t + u * n) / 2^64, where u makes the low word zero */
    const uint64_t u = t[0] * m->n0;
    lo = fio_math_mulc64(u, n[0], &hi);
    /* the low word is 0 mod 2^64 by choice of u; only the carry is needed */
    const uint64_t zero_ = fio_math_addc64(t[0], lo, 0, &c1);
    (void)zero_;
    c = hi + c1;
    for (size_t j = 1; j < len; ++j) {
      lo = fio_math_mulc64(u, n[j], &hi);
      lo = fio_math_addc64(lo, c, 0, &c1);
      t[j - 1] = fio_math_addc64(t[j], lo, 0, &c2);
      c = hi + c1 + c2;
    }
    t[len - 1] = fio_math_addc64(t[len], c, 0, &c1);
    t[len] = t[len + 1] + c1;
  }
  /* t < 2n: one conditional subtraction */
  fio___rsa_cond_sub(r, t, t[len], n, len);
  fio_secure_zero(t, sizeof(t));
}

/** Constant-time modular doubling: x = 2x mod n, for x < n. */
FIO_SFUNC void fio___rsa_mod_dbl(uint64_t *x, const fio___rsa_mont_s *m) {
  uint64_t carry = 0;
  for (size_t i = 0; i < m->len; ++i) {
    const uint64_t w = x[i];
    x[i] = (w << 1) | carry;
    carry = w >> 63;
  }
  fio___rsa_cond_sub(x, x, carry, m->n, m->len);
}

/** Constant-time modular addition: r = a + b mod n, for a, b < n. */
FIO_SFUNC void fio___rsa_mod_add(uint64_t *r,
                                 const uint64_t *a,
                                 const uint64_t *b,
                                 const fio___rsa_mont_s *m) {
  const uint64_t carry = fio_math_add(r, a, b, m->len);
  fio___rsa_cond_sub(r, r, carry, m->n, m->len);
}

/** Constant-time modular subtraction: r = a - b mod n, for a, b < n. */
FIO_SFUNC void fio___rsa_mod_sub(uint64_t *r,
                                 const uint64_t *a,
                                 const uint64_t *b,
                                 const fio___rsa_mont_s *m) {
  const uint64_t mask = (uint64_t)0 - fio_math_sub(r, a, b, m->len);
  uint64_t c = 0;
  for (size_t i = 0; i < m->len; ++i)
    r[i] = fio_math_addc64(r[i], m->n[i] & mask, c, &c);
}

/**
 * Initializes a Montgomery context from a big-endian modulus.
 *
 * Returns -1 (no state written beyond `m`) unless `n` is odd, greater than 1
 * and at most FIO_RSA_MAX_WORDS words long.
 */
FIO_SFUNC int fio___rsa_mont_init(fio___rsa_mont_s *m,
                                  const uint8_t *n_be,
                                  size_t n_len) {
  const size_t len = (n_len + 7) / 8;
  if (!n_be || !len || len > FIO_RSA_MAX_WORDS)
    return -1;
  m->len = len;
  fio___rsa_bytes_to_words(m->n, len, n_be, n_len);
  const size_t msb = fio_math_msb_index(m->n, len);
  if (!(m->n[0] & 1) || !msb)
    return -1; /* even, or n == 1 */
  m->n0 = (uint64_t)0 - fio___rsa_inv_mod_2pow64(m->n[0]);
  /* x = 2^msb < n; double to 2^(64*len + len) mod n = Mont(2^len), then six
   * Montgomery squarings give Mont(2^(64*len)) = R^2 mod n. */
  uint64_t *x = m->r2;
  FIO_MEMSET(x, 0, len * sizeof(uint64_t));
  x[msb >> 6] = (uint64_t)1 << (msb & 63);
  for (size_t i = msb; i < len * 65; ++i)
    fio___rsa_mod_dbl(x, m);
  for (int i = 0; i < 6; ++i)
    fio___rsa_mont_mul(x, x, x, m);
  return 0;
}

/** r = a * b mod n, for a < R and b < n. */
FIO_SFUNC void fio___rsa_mul_mod(uint64_t *r,
                                 const uint64_t *a,
                                 const uint64_t *b,
                                 const fio___rsa_mont_s *m) {
  fio___rsa_mont_mul(r, a, b, m);     /* a*b/R */
  fio___rsa_mont_mul(r, r, m->r2, m); /* a*b */
}

/** Constant-time reduction: r = v mod n for any `v_len`-word value. */
FIO_SFUNC void fio___rsa_reduce(uint64_t *r,
                                const uint64_t *v,
                                size_t v_len,
                                const fio___rsa_mont_s *m) {
  const size_t len = m->len;
  uint64_t acc[FIO_RSA_MAX_WORDS];
  uint64_t c[FIO_RSA_MAX_WORDS];
  FIO_MEMSET(acc, 0, len * sizeof(uint64_t));
  /* Horner over R-sized chunks, acc = value * R mod n */
  for (size_t k = (v_len + len - 1) / len; k--;) {
    const size_t off = k * len;
    const size_t cnt = (v_len - off) < len ? (v_len - off) : len;
    FIO_MEMSET(c, 0, len * sizeof(uint64_t));
    FIO_MEMCPY(c, v + off, cnt * sizeof(uint64_t));
    fio___rsa_mont_mul(acc, acc, m->r2, m); /* acc * R */
    fio___rsa_mont_mul(c, c, m->r2, m);     /* chunk * R */
    fio___rsa_mod_add(acc, acc, c, m);
  }
  fio___rsa_mont_mul(r, acc, fio___rsa_one, m);
  fio_secure_zero(acc, sizeof(acc));
  fio_secure_zero(c, sizeof(c));
}

/**
 * Constant-time modular exponentiation: r = base^exp mod n, for base < R.
 *
 * Fixed 4-bit window over every bit of the `exp_len`-word exponent: each
 * window performs 4 squarings and 1 multiplication by a table entry chosen
 * with a full masked scan, so the operation sequence and memory access
 * pattern are independent of the exponent and base.
 */
FIO_SFUNC void fio___rsa_exp_ct(uint64_t *r,
                                const uint64_t *base,
                                const uint64_t *exp,
                                size_t exp_len,
                                const fio___rsa_mont_s *m) {
  const size_t len = m->len;
  uint64_t tbl[16][FIO_RSA_MAX_WORDS];
  uint64_t acc[FIO_RSA_MAX_WORDS];
  uint64_t sel[FIO_RSA_MAX_WORDS];
  fio___rsa_mont_mul(tbl[0], fio___rsa_one, m->r2, m); /* Mont(1) */
  fio___rsa_mont_mul(tbl[1], base, m->r2, m);          /* Mont(base) */
  for (size_t i = 2; i < 16; ++i)
    fio___rsa_mont_mul(tbl[i], tbl[i - 1], tbl[1], m);
  FIO_MEMCPY(acc, tbl[0], len * sizeof(uint64_t));
  for (size_t i = exp_len * 16; i--;) {
    const uint64_t win = (exp[i >> 4] >> ((i & 15) << 2)) & 15;
    for (int s = 0; s < 4; ++s)
      fio___rsa_mont_mul(acc, acc, acc, m);
    FIO_MEMSET(sel, 0, len * sizeof(uint64_t));
    for (uint64_t k = 0; k < 16; ++k) {
      const uint64_t mask = (uint64_t)0 - (uint64_t)fio_ct_false(k ^ win);
      for (size_t j = 0; j < len; ++j)
        sel[j] |= tbl[k][j] & mask;
    }
    fio___rsa_mont_mul(acc, acc, sel, m);
  }
  fio___rsa_mont_mul(r, acc, fio___rsa_one, m);
  fio_secure_zero(tbl, sizeof(tbl));
  fio_secure_zero(acc, sizeof(acc));
  fio_secure_zero(sel, sizeof(sel));
}

/**
 * Modular exponentiation by a PUBLIC exponent e >= 1 (square-and-multiply
 * branches on exponent bits only): r = base^e mod n, for base < R.
 */
FIO_SFUNC void fio___rsa_exp_pub(uint64_t *r,
                                 const uint64_t *base,
                                 uint64_t e,
                                 const fio___rsa_mont_s *m) {
  uint64_t b[FIO_RSA_MAX_WORDS];
  uint64_t acc[FIO_RSA_MAX_WORDS];
  fio___rsa_mont_mul(b, base, m->r2, m);
  FIO_MEMCPY(acc, b, m->len * sizeof(uint64_t));
  for (size_t i = fio_math_msb_index(&e, 1); i--;) {
    fio___rsa_mont_mul(acc, acc, acc, m);
    if ((e >> i) & 1)
      fio___rsa_mont_mul(acc, acc, b, m);
  }
  fio___rsa_mont_mul(r, acc, fio___rsa_one, m);
  fio_secure_zero(b, sizeof(b));
  fio_secure_zero(acc, sizeof(acc));
}

/**
 * Parses a public exponent: odd, >= 3 and at most 64 bits (larger exponents
 * are a verification DoS vector and unused in practice). Returns 0 on error.
 */
FIO_SFUNC uint64_t fio___rsa_parse_e(const uint8_t *e, size_t e_len) {
  if (!e || !e_len)
    return 0;
  fio___rsa_trim_be(&e, &e_len);
  if (e_len > 8)
    return 0;
  uint64_t v = 0;
  for (size_t i = 0; i < e_len; ++i)
    v = (v << 8) | e[i];
  if (!(v & 1) || v < 3)
    return 0;
  return v;
}

/** Returns emBits = modBits - 1 for a trimmed (nonzero-leading) modulus. */
FIO_IFUNC size_t fio___rsa_em_bits(const uint8_t *n, size_t n_len) {
  size_t bits = (n_len - 1) * 8;
  for (uint8_t top = n[0]; top; top >>= 1)
    ++bits;
  return bits - 1;
}

/**
 * RSA public key operation: result = sig^e mod n (RSAVP1).
 *
 * Validates the key: n must be odd and 256..FIO_RSA_MAX_BYTES bytes long
 * (after stripping leading zeros) and e must be odd, >= 3 and <= 64 bits.
 * `sig_len` must equal the trimmed modulus length; on success `result`
 * receives exactly `sig_len` bytes. Returns 0 on success, -1 on error.
 */
FIO_SFUNC int fio___rsa_public_op(uint8_t *result,
                                  const uint8_t *sig,
                                  size_t sig_len,
                                  const fio_rsa_pubkey_s *key) {
  if (!result || !sig || !key || !key->n)
    return -1;

  const uint8_t *n = key->n;
  size_t n_len = key->n_len;
  fio___rsa_trim_be(&n, &n_len);

  /* Only 2048-4096 bit keys; signature must be same length as modulus */
  if (n_len > FIO_RSA_MAX_BYTES || n_len < 256 || sig_len != n_len)
    return -1;
  const uint64_t e = fio___rsa_parse_e(key->e, key->e_len);
  if (!e)
    return -1;

  fio___rsa_mont_s m;
  if (fio___rsa_mont_init(&m, n, n_len))
    return -1;

  uint64_t s[FIO_RSA_MAX_WORDS];
  fio___rsa_bytes_to_words(s, m.len, sig, sig_len);
  if (fio___rsa_cmp(s, m.n, m.len) >= 0)
    return -1; /* Signature must be < modulus */

  fio___rsa_exp_pub(s, s, e, &m);
  fio___rsa_words_to_bytes(result, n_len, s, m.len);
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

  /* Compute sig^e mod n (on success sig_len is the trimmed modulus length) */
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

  const size_t k = sig_len; /* Encoded message length */
  const size_t digestinfo_len = FIO___RSA_DIGESTINFO_PREFIX_LEN + hash_len;
  if (k < 11 + digestinfo_len)
    return -1; /* Message too short */
  const size_t ps_len = k - 3 - digestinfo_len;

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
      fio_u512 h = fio_sha384(hash_input, seed_len + 4);
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

  /* Compute sig^e mod n (validates the key and sig_len) */
  uint8_t em_buf[FIO_RSA_MAX_BYTES];
  if (fio___rsa_public_op(em_buf, sig, sig_len, key) != 0)
    return -1;

  /*
   * RSA-PSS Verification (RFC 8017 Section 9.1.2):
   *
   * EM = maskedDB || H || 0xBC, emLen = ceil(emBits / 8), emBits = modBits-1
   *
   * where:
   *   - maskedDB = DB XOR dbMask
   *   - H = Hash(M')
   *   - M' = 0x00 00 00 00 00 00 00 00 || mHash || salt
   *   - DB = PS || 0x01 || salt
   *   - PS = zero bytes
   */
  const uint8_t *n = key->n;
  size_t n_len = key->n_len;
  fio___rsa_trim_be(&n, &n_len); /* n_len == sig_len after public_op */
  const size_t em_bits = fio___rsa_em_bits(n, n_len);
  const size_t em_len = (em_bits + 7) / 8;
  const size_t top_mask = 0xFF >> (8 * em_len - em_bits);

  /* Accumulate all PSS validation checks in constant time.
   * No return statement appears after the public-key operation. */
  int result = 0;

  /* When modBits - 1 is a multiple of 8, EM is one byte shorter than the
   * modulus and the leading octet of the RSAVP1 output must be zero. */
  const uint8_t *em = em_buf + (n_len - em_len);
  if (n_len != em_len)
    result |= em_buf[0];

  size_t db_len = em_len - hash_len - 1;

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
  db[0] = 0; /* db_len >= 190 (n_len >= 256); silences -Wmaybe-uninitialized */
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
    fio_u512 hp = fio_sha384(m_prime, 8 + hash_len + salt_len);
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

/** Secret CRT scratch, wiped as one unit. */
typedef struct {
  fio___rsa_mont_s p, q;
  uint64_t dp[FIO_RSA_MAX_WORDS], dq[FIO_RSA_MAX_WORDS];
  uint64_t qinv[FIO_RSA_MAX_WORDS], exp[FIO_RSA_MAX_WORDS];
  uint64_t r[FIO_RSA_MAX_WORDS + 1], t[FIO_RSA_MAX_WORDS];
  uint64_t sp[FIO_RSA_MAX_WORDS], sq[FIO_RSA_MAX_WORDS];
  uint8_t rnd[(FIO_RSA_MAX_WORDS + 1) * 8];
} fio___rsa_crt_s;

/**
 * Computes one blinded CRT half: out = m^d mod p, blinded by a random r in
 * [1, p-1] (m' = m * r^e, s = m'^d * r^(p-2)). `w->exp` and `w->t` are
 * scratch.
 */
FIO_SFUNC void fio___rsa_crt_half(uint64_t *out,
                                  const uint64_t *m_words,
                                  size_t m_len,
                                  const uint64_t *d,
                                  uint64_t e,
                                  const fio___rsa_mont_s *p,
                                  fio___rsa_crt_s *w) {
  const size_t len = p->len;
  /* r = random mod p (one extra word bounds the bias by 2^-64), r != 0 */
  const size_t rnd_len = (len + 1) * 8;
  if (fio_rand_bytes_secure(w->rnd, rnd_len) != 0)
    fio_rand_bytes(w->rnd, rnd_len);
  fio___rsa_bytes_to_words(w->r, len + 1, w->rnd, rnd_len);
  fio___rsa_reduce(w->r, w->r, len + 1, p);
  uint64_t any = 0;
  for (size_t i = 0; i < len; ++i)
    any |= w->r[i];
  w->r[0] |= (uint64_t)fio_ct_false(any);

  fio___rsa_reduce(out, m_words, m_len, p); /* m mod p */
  fio___rsa_exp_pub(w->t, w->r, e, p);      /* r^e */
  fio___rsa_mul_mod(out, w->t, out, p);     /* m' = r^e * m */
  fio___rsa_exp_ct(out, out, d, len, p);    /* s' = m'^d = s * r */
  /* r^-1 = r^(p-2) (Fermat) */
  FIO_MEMCPY(w->exp, p->n, len * sizeof(uint64_t));
  /* p is odd and > 1 (mont_init), so p >= 3 and p - 2 never borrows */
  uint64_t borrow_ = fio_math_sub(w->exp, w->exp, fio___rsa_one, len);
  borrow_ |= fio_math_sub(w->exp, w->exp, fio___rsa_one, len);
  (void)borrow_;
  fio___rsa_exp_ct(w->t, w->r, w->exp, len, p);
  fio___rsa_mul_mod(out, out, w->t, p);
}

/**
 * CRT private key operation with RSA blinding: s = m^d mod n.
 *
 * Requires p, q, dP, dQ and qInv (e is validated by the caller). Prime and
 * exponent widths are validated; key consistency is verified by the caller
 * re-encrypting the result. Returns 0 on success, -1 on malformed keys.
 */
FIO_SFUNC int fio___rsa_private_op_crt(uint64_t *s,
                                       const uint64_t *m_words,
                                       const fio___rsa_mont_s *mn,
                                       uint64_t e,
                                       const fio_rsa_privkey_s *key,
                                       fio___rsa_crt_s *w) {
  const uint8_t *p_be = key->p, *q_be = key->q, *dP_be = key->dP,
                *dQ_be = key->dQ, *qInv_be = key->qInv;
  size_t p_len = key->p_len, q_len = key->q_len, dP_len = key->dP_len,
         dQ_len = key->dQ_len, qInv_len = key->qInv_len;
  fio___rsa_trim_be(&p_be, &p_len);
  fio___rsa_trim_be(&q_be, &q_len);
  fio___rsa_trim_be(&dP_be, &dP_len);
  fio___rsa_trim_be(&dQ_be, &dQ_len);
  fio___rsa_trim_be(&qInv_be, &qInv_len);

  /* bytes(p) + bytes(q) is bytes(n) or bytes(n) + 1 for p * q == n */
  const size_t n_bytes = mn->len * 8;
  if (p_len + q_len > n_bytes + 1 || dP_len > p_len || dQ_len > q_len ||
      qInv_len > p_len)
    return -1;
  if (fio___rsa_mont_init(&w->p, p_be, p_len) ||
      fio___rsa_mont_init(&w->q, q_be, q_len))
    return -1;

  fio___rsa_bytes_to_words(w->dp, w->p.len, dP_be, dP_len);
  fio___rsa_bytes_to_words(w->dq, w->q.len, dQ_be, dQ_len);
  fio___rsa_bytes_to_words(w->qinv, w->p.len, qInv_be, qInv_len);

  fio___rsa_crt_half(w->sp, m_words, mn->len, w->dp, e, &w->p, w);
  fio___rsa_crt_half(w->sq, m_words, mn->len, w->dq, e, &w->q, w);

  /* h = qInv * (s_p - s_q mod p) mod p; s = s_q + q * h (mod n) */
  fio___rsa_reduce(w->t, w->sq, w->q.len, &w->p);
  fio___rsa_mod_sub(w->t, w->sp, w->t, &w->p);
  fio___rsa_mul_mod(w->t, w->qinv, w->t, &w->p);
  /* zero-extend h and q to the modulus width (both < n) */
  FIO_MEMSET(w->sp, 0, mn->len * sizeof(uint64_t));
  FIO_MEMCPY(w->sp, w->t, w->p.len * sizeof(uint64_t));
  FIO_MEMSET(w->t, 0, mn->len * sizeof(uint64_t));
  FIO_MEMCPY(w->t, w->q.n, w->q.len * sizeof(uint64_t));
  FIO_MEMSET(w->sq + w->q.len, 0, (mn->len - w->q.len) * sizeof(uint64_t));
  fio___rsa_mul_mod(s, w->t, w->sp, mn);
  fio___rsa_mod_add(s, s, w->sq, mn);
  return 0;
}

/**
 * RSA private key operation: result = m^d mod n (RSASP1 per RFC 8017)
 *
 * Uses CRT with RSA blinding when CRT parameters and the public exponent are
 * available, otherwise the plain constant-time Montgomery ladder over d.
 * When e is available the result is re-encrypted and compared with the
 * message, so faults or inconsistent keys never emit a signature.
 * Returns 0 on success, -1 on invalid keys/input (result is zeroed).
 */
FIO_SFUNC int fio___rsa_private_op(uint8_t *result,
                                   const uint8_t *message,
                                   size_t msg_len,
                                   const fio_rsa_privkey_s *key) {
  if (!result || !message || !key || !key->n || !key->d)
    return -1;

  const uint8_t *n = key->n;
  const uint8_t *d = key->d;
  size_t n_len = key->n_len;
  size_t d_len = key->d_len;
  fio___rsa_trim_be(&n, &n_len);
  fio___rsa_trim_be(&d, &d_len);

  /* Only 2048-4096 bit keys; message must be same length as modulus */
  if (n_len > FIO_RSA_MAX_BYTES || n_len < 256 || msg_len != n_len ||
      d_len > n_len)
    return -1;

  const int have_crt =
      (key->p && key->p_len && key->q && key->q_len && key->dP && key->dP_len &&
       key->dQ && key->dQ_len && key->qInv && key->qInv_len);
  uint64_t e = 0;
  if (key->e && key->e_len) {
    e = fio___rsa_parse_e(key->e, key->e_len);
    if (!e)
      return -1;
  }

  struct {
    fio___rsa_mont_s n;
    uint64_t m[FIO_RSA_MAX_WORDS];
    uint64_t s[FIO_RSA_MAX_WORDS];
    uint64_t d[FIO_RSA_MAX_WORDS];
    fio___rsa_crt_s crt;
  } w;
  int rc = -1;

  if (fio___rsa_mont_init(&w.n, n, n_len))
    goto cleanup;
  fio___rsa_bytes_to_words(w.m, w.n.len, message, msg_len);
  if (!fio___rsa_ct_lt(w.m, w.n.n, w.n.len))
    goto cleanup; /* message must be < modulus */

  if (have_crt && e) {
    rc = fio___rsa_private_op_crt(w.s, w.m, &w.n, e, key, &w.crt);
  } else {
    /* No CRT parameters: production signing keys should supply them so the
     * CRT + blinding path is used. */
    fio___rsa_bytes_to_words(w.d, w.n.len, d, d_len);
    fio___rsa_exp_ct(w.s, w.m, w.d, w.n.len, &w.n);
    rc = 0;
  }

  if (!rc && e) { /* fault / key-consistency check: s^e mod n == m */
    fio___rsa_exp_pub(w.d, w.s, e, &w.n);
    if (fio___rsa_ct_neq(w.d, w.m, w.n.len))
      rc = -1;
  }

  if (!rc)
    fio___rsa_words_to_bytes(result, n_len, w.s, w.n.len);

cleanup:
  fio_secure_zero(&w, sizeof(w));
  return rc;
}

/**
 * EMSA-PSS-ENCODE per RFC 8017 Section 9.1.1
 *
 * Encodes a message hash into the PSS format for signing.
 *
 * @param em        Output buffer for encoded message (em_len bytes)
 * @param em_len    Encoded message length, ceil(emBits / 8)
 * @param msg_hash  Pre-computed hash of the message
 * @param hash_len  Hash length (32, 48, or 64 bytes)
 * @param hash_alg  Hash algorithm used
 * @param top_mask  Mask clearing the 8*em_len - emBits leftmost bits
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

  /* Validate sizes: em_len >= hash_len + salt_len + 2 */
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
    fio_sha512_s sha = fio_sha384_init();
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
  if (!signature || !sig_len || !msg_hash || !key || !key->n)
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

  const uint8_t *n = key->n;
  size_t n_len = key->n_len;
  fio___rsa_trim_be(&n, &n_len);

  /* Validate key size (a trimmed modulus this long has a nonzero top byte) */
  if (n_len > FIO_RSA_MAX_BYTES || n_len < 256)
    return -1;

  /* emBits = modBits - 1; EM is one byte shorter when emBits % 8 == 0 */
  const size_t em_bits = fio___rsa_em_bits(n, n_len);
  const size_t em_len = (em_bits + 7) / 8;
  const size_t top_mask = 0xFF >> (8 * em_len - em_bits);

  /* Step 1: EMSA-PSS-ENCODE (left-padded with zeros to the modulus length) */
  uint8_t em[FIO_RSA_MAX_BYTES];
  em[0] = 0;
  if (fio___rsa_emsa_pss_encode(em + (n_len - em_len),
                                em_len,
                                msg_hash,
                                hash_len,
                                hash_alg,
                                top_mask) != 0) {
    return -1;
  }

  /* Step 2-4: RSASP1 - compute s = m^d mod n */
  int rc = fio___rsa_private_op(signature, em, n_len, key);
  fio_secure_zero(em, sizeof(em));
  if (rc)
    return -1;

  *sig_len = n_len;
  FIO_LOG_DEBUG2("RSA-PSS: Generated %zu-byte signature", *sig_len);
  return 0;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_RSA */
#undef FIO_RSA
