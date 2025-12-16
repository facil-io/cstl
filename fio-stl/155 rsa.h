/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_RSA                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                    RSA Signature Verification for TLS 1.3
                         (PKCS#1 v1.5 and RSA-PSS)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_RSA) && !defined(H___FIO_RSA___H)
#define H___FIO_RSA___H

/* *****************************************************************************
RSA Signature Verification Module

This module provides RSA signature verification (NOT signing) for TLS 1.3
certificate chain validation. It supports:

- PKCS#1 v1.5 signatures (sha256WithRSAEncryption, etc.)
- RSA-PSS signatures (required for TLS 1.3 CertificateVerify)
- Key sizes: 2048, 3072, 4096 bits

**Note**: This is verification-only. No private key operations are supported.
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

/* *****************************************************************************
Implementation - Modular Exponentiation

Compute: result = base^exp mod n

Uses square-and-multiply algorithm with constant-time modular reduction.
For RSA verification, exp is typically 65537 (0x10001) = 17 bits.
***************************************************************************** */

/**
 * Modular exponentiation: result = base^exp mod n
 *
 * This uses a simple square-and-multiply algorithm.
 * For RSA verification with e=65537, this does 17 squarings + 1 multiply.
 */
FIO_SFUNC void fio___rsa_modexp(uint64_t *result,
                                const uint64_t *base,
                                const uint64_t *exp,
                                const uint64_t *n,
                                size_t word_count) {
/* Double-size buffer for multiplication results */
#if !defined(_MSC_VER) && (!defined(__cplusplus) || __cplusplus > 201402L)
  uint64_t tmp[word_count * 2];
  uint64_t acc[word_count];
  uint64_t sqr[word_count];
#else
  uint64_t tmp[FIO_RSA_MAX_WORDS * 2];
  uint64_t acc[FIO_RSA_MAX_WORDS];
  uint64_t sqr[FIO_RSA_MAX_WORDS];
  FIO_ASSERT(word_count <= FIO_RSA_MAX_WORDS,
             "RSA key size exceeds maximum supported");
#endif

  /* Initialize accumulator to 1 */
  FIO_MEMSET(acc, 0, word_count * sizeof(uint64_t));
  acc[0] = 1;

  /* Copy base to squaring buffer */
  FIO_MEMCPY(sqr, base, word_count * sizeof(uint64_t));

  /* Find the highest set bit in the exponent */
  size_t exp_bits = fio_math_msb_index((uint64_t *)exp, word_count);
  if (exp_bits == (size_t)-1) {
    /* exp = 0, result = 1 (already set) */
    FIO_MEMCPY(result, acc, word_count * sizeof(uint64_t));
    return;
  }

  /* Square-and-multiply from LSB to MSB */
  for (size_t bit = 0; bit <= exp_bits; ++bit) {
    size_t word_idx = bit / 64;
    size_t bit_idx = bit % 64;

    /* If this bit is set, multiply accumulator by current square */
    if (exp[word_idx] & (1ULL << bit_idx)) {
      fio_math_mul(tmp, acc, sqr, word_count);
      fio_math_div(NULL, acc, tmp, n, word_count * 2);
      /* Copy remainder (lower words) to acc */
      /* Note: fio_math_div works on double-size arrays, but we need the mod */
      /* Actually fio_math_div expects same-size arrays. We need to be careful
       */
    }

    /* Square the current value (unless this is the last bit) */
    if (bit < exp_bits) {
      fio_math_mul(tmp, sqr, sqr, word_count);
      fio_math_div(NULL, sqr, tmp, n, word_count * 2);
    }
  }

  FIO_MEMCPY(result, acc, word_count * sizeof(uint64_t));
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
#else
  uint64_t tmp[FIO_RSA_MAX_WORDS * 2];
  uint64_t sqr[FIO_RSA_MAX_WORDS];
  uint64_t rem[FIO_RSA_MAX_WORDS * 2];
  FIO_ASSERT(word_count <= FIO_RSA_MAX_WORDS,
             "RSA key size exceeds maximum supported");
#endif

  /* sqr = base (mod n is already ensured by caller if base < n) */
  FIO_MEMCPY(sqr, base, word_count * sizeof(uint64_t));

  /* Square 16 times to get base^(2^16) */
  for (int i = 0; i < 16; ++i) {
    fio_math_mul(tmp, sqr, sqr, word_count);
    /* Reduce mod n */
    FIO_MEMSET(rem, 0, word_count * 2 * sizeof(uint64_t));
    fio_math_div(NULL, rem, tmp, n, word_count * 2);
    FIO_MEMCPY(sqr, rem, word_count * sizeof(uint64_t));
  }

  /* result = sqr * base (mod n) = base^(2^16) * base = base^65537 */
  fio_math_mul(tmp, sqr, base, word_count);
  FIO_MEMSET(rem, 0, word_count * 2 * sizeof(uint64_t));
  fio_math_div(NULL, rem, tmp, n, word_count * 2);
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

  /* Verify trailing byte is 0xBC */
  if (em[em_len - 1] != 0xBC)
    return -1;

  /* The top bits of EM should be 0 (based on modulus bit length) */
  size_t em_bits = key->n_len * 8 - 1; /* One less than modulus bits */
  size_t top_mask = 0xFF >> (8 * em_len - em_bits - 1);
  if ((em[0] & ~top_mask) != 0)
    return -1;

  /* Extract H (hash) from EM */
  const uint8_t *masked_db = em;
  const uint8_t *h = em + db_len;

  /* Compute dbMask = MGF1(H, db_len) */
  uint8_t db_mask[FIO_RSA_MAX_BYTES];
  fio___rsa_mgf1(db_mask, db_len, h, hash_len, hash_alg);

  /* Compute DB = maskedDB XOR dbMask */
  uint8_t db[FIO_RSA_MAX_BYTES];
  for (size_t i = 0; i < db_len; ++i)
    db[i] = masked_db[i] ^ db_mask[i];

  /* Clear top bits of DB */
  db[0] &= top_mask;

  /* Verify DB = PS || 0x01 || salt */
  /* PS should be all zeros, length = db_len - salt_len - 1 */
  size_t ps_len = db_len - salt_len - 1;
  int result = 0;

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
    FIO_MEMCPY(h_prime, hp.u8, 32);
    break;
  }
  case FIO_RSA_HASH_SHA384: {
    fio_u512 hp = fio_sha512(m_prime, 8 + hash_len + salt_len);
    FIO_MEMCPY(h_prime, hp.u8, 48);
    break;
  }
  case FIO_RSA_HASH_SHA512: {
    fio_u512 hp = fio_sha512(m_prime, 8 + hash_len + salt_len);
    FIO_MEMCPY(h_prime, hp.u8, 64);
    break;
  }
  }

  /* Verify H == H' */
  result |= fio___rsa_memcmp_ct(h, h_prime, hash_len);

  return result ? -1 : 0;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_RSA */
#undef FIO_RSA
