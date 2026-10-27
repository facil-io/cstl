/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_PEM                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          PEM File Parser for TLS 1.3
                    (RFC 7468 - Textual Encodings of PKIX)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_PEM) && !defined(H___FIO_PEM___H)
#define H___FIO_PEM___H

/* *****************************************************************************
PEM Parser Module

This module provides PEM file parsing for loading X.509 certificates and
private keys from PEM-encoded files. It supports:

- Certificate parsing ("CERTIFICATE" label)
- PKCS#8 private keys ("PRIVATE KEY" label)
- Legacy RSA private keys ("RSA PRIVATE KEY" label - PKCS#1)
- Legacy EC private keys ("EC PRIVATE KEY" label - SEC1)

PEM format (RFC 7468):
  -----BEGIN <label>-----
  <base64-encoded DER data>
  -----END <label>-----

**Note**: This parser does not support encrypted private keys.
***************************************************************************** */

/* *****************************************************************************
PEM Type Definitions
***************************************************************************** */

/** Private key algorithm types */
typedef enum {
  FIO_PEM_KEY_UNKNOWN = 0,
  FIO_PEM_KEY_RSA = 1,
  FIO_PEM_KEY_ECDSA_P256 = 2,
  FIO_PEM_KEY_ED25519 = 3,
  FIO_PEM_KEY_ECDSA_P384 = 4,
} fio_pem_key_type_e;

/** Parsed PEM block */
typedef struct {
  const uint8_t *der; /**< Pointer to decoded DER data */
  size_t der_len;     /**< Length of DER data */
  const char *label;  /**< PEM label (e.g., "CERTIFICATE", "PRIVATE KEY") */
  size_t label_len;   /**< Length of label */
} fio_pem_s;

/** Parsed private key structure */
typedef struct {
  fio_pem_key_type_e type;
  union {
    struct {
      uint8_t n[FIO_RSA_MAX_BYTES]; /**< RSA modulus (big-endian) */
      size_t n_len;
      uint8_t e[FIO_RSA_MAX_BYTES]; /**< RSA public exponent (big-endian) */
      size_t e_len;
      uint8_t d[FIO_RSA_MAX_BYTES]; /**< RSA private exponent (big-endian) */
      size_t d_len;
      uint8_t p[FIO_RSA_MAX_BYTES]; /**< RSA prime p (optional) */
      size_t p_len;
      uint8_t q[FIO_RSA_MAX_BYTES]; /**< RSA prime q (optional) */
      size_t q_len;
      uint8_t dP[FIO_RSA_MAX_BYTES]; /**< d mod (p-1) (optional) */
      size_t dP_len;
      uint8_t dQ[FIO_RSA_MAX_BYTES]; /**< d mod (q-1) (optional) */
      size_t dQ_len;
      uint8_t qInv[FIO_RSA_MAX_BYTES]; /**< q^-1 mod p (optional) */
      size_t qInv_len;
    } rsa;
    struct {
      uint8_t private_key[32]; /**< P-256 scalar (32 bytes) */
      /** Embedded SEC 1 public key (optional, can derive): the first
       * `public_key_len` bytes, 65 (04 || x || y) or 33 (02/03 || x). */
      uint8_t public_key[65];
      int has_public_key;    /**< 1 if public_key is populated */
      size_t public_key_len; /**< 65 or 33 when populated, otherwise 0 */
    } ecdsa_p256;
    struct {
      uint8_t private_key[48]; /**< P-384 scalar (48 bytes) */
      /** Embedded SEC 1 public key (optional, can derive): the first
       * `public_key_len` bytes, 97 (04 || x || y) or 49 (02/03 || x). */
      uint8_t public_key[97];
      int has_public_key;    /**< 1 if public_key is populated */
      size_t public_key_len; /**< 97 or 49 when populated, otherwise 0 */
    } ecdsa_p384;
    struct {
      uint8_t private_key[32]; /**< Ed25519 seed (32 bytes) */
      uint8_t public_key[32];  /**< Ed25519 public key (optional) */
      int has_public_key;      /**< 1 if public_key is populated */
    } ed25519;
  };
} fio_pem_private_key_s;

/* *****************************************************************************
PEM Parser API
***************************************************************************** */

/**
 * Parse a single PEM block from data.
 *
 * Finds the next -----BEGIN <label>----- and -----END <label>----- markers,
 * base64 decodes the content between them, and returns the DER data.
 *
 * The Base64 body may use the standard or URL-safe alphabet, any whitespace
 * and optional padding; undecodable bytes (e.g. encrypted-PEM headers) or
 * data after padding are rejected.
 *
 * @param out Output structure to fill with parsed PEM block info
 * @param der_buf Buffer to store decoded DER data (caller-provided)
 * @param der_buf_len Size of der_buf
 * @param pem_data PEM-encoded data
 * @param pem_len Length of PEM data
 * @return Number of bytes consumed from pem_data (through the END marker and
 *         any whitespace following it), or 0 on error
 */
SFUNC size_t fio_pem_parse(fio_pem_s *out,
                           uint8_t *der_buf,
                           size_t der_buf_len,
                           const char *pem_data,
                           size_t pem_len);

/**
 * Parse certificate from PEM file content.
 *
 * Handles "CERTIFICATE" label and parses the X.509 certificate.
 *
 * On success the certificate owns a heap copy of its DER data (`cert->der`);
 * release it with `fio_pem_certificate_free`. On error nothing is retained.
 *
 * @param cert Output certificate structure (from fio_x509.h)
 * @param pem_data PEM-encoded certificate data
 * @param pem_len Length of PEM data
 * @return 0 on success, -1 on error
 */
SFUNC int fio_pem_parse_certificate(fio_x509_cert_s *cert,
                                    const char *pem_data,
                                    size_t pem_len);

/**
 * Releases the DER buffer owned by a certificate that was successfully
 * parsed with `fio_pem_parse_certificate` and zeroes the structure.
 *
 * Do NOT call this for certificates parsed with `fio_x509_parse` directly.
 */
SFUNC void fio_pem_certificate_free(fio_x509_cert_s *cert);

/**
 * Parse private key from PEM file content.
 *
 * Supports:
 * - "PRIVATE KEY" (PKCS#8 PrivateKeyInfo)
 * - "RSA PRIVATE KEY" (PKCS#1 RSAPrivateKey)
 * - "EC PRIVATE KEY" (SEC1 ECPrivateKey)
 *
 * EC keys may be P-256 (`FIO_PEM_KEY_ECDSA_P256`) or P-384
 * (`FIO_PEM_KEY_ECDSA_P384`); the curve comes from the namedCurve OID or,
 * when a legacy SEC1 key omits it, from the scalar length (32 / 48 bytes).
 *
 * @param key Output private key structure
 * @param pem_data PEM-encoded private key data
 * @param pem_len Length of PEM data
 * @return 0 on success, -1 on error
 */
SFUNC int fio_pem_parse_private_key(fio_pem_private_key_s *key,
                                    const char *pem_data,
                                    size_t pem_len);

/**
 * Get the DER-encoded certificate from PEM data.
 *
 * This is a convenience function that extracts just the DER bytes
 * without parsing the X.509 structure.
 *
 * @param der_out Output buffer for DER data
 * @param der_out_len Size of output buffer
 * @param pem_data PEM-encoded certificate data
 * @param pem_len Length of PEM data
 * @return Length of DER data written, or 0 on error
 */
SFUNC size_t fio_pem_get_certificate_der(uint8_t *der_out,
                                         size_t der_out_len,
                                         const char *pem_data,
                                         size_t pem_len);

/**
 * Securely clear a private key structure.
 *
 * @param key Private key to clear
 */
FIO_IFUNC void fio_pem_private_key_clear(fio_pem_private_key_s *key);

/* *****************************************************************************
Implementation - Inline Functions
***************************************************************************** */

/** Securely clear private key */
FIO_IFUNC void fio_pem_private_key_clear(fio_pem_private_key_s *key) {
  if (key) {
    fio_secure_zero(key, sizeof(*key));
  }
}

/* *****************************************************************************
Implementation - Possibly Externed Functions
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Implementation - PEM Parsing Helpers
***************************************************************************** */

/** Find a string in data (memchr on the first byte, then compare) */
FIO_SFUNC const char *fio___pem_find(const char *haystack,
                                     size_t haystack_len,
                                     const char *needle,
                                     size_t needle_len) {
  if (needle_len > haystack_len || needle_len == 0)
    return NULL;
  const char *end = haystack + haystack_len - needle_len + 1;
  const char *p = haystack;
  while (p < end) {
    p = (const char *)FIO_MEMCHR(p, needle[0], (size_t)(end - p));
    if (!p)
      return NULL;
    if (FIO_MEMCMP(p, needle, needle_len) == 0)
      return p;
    ++p;
  }
  return NULL;
}

/**
 * Base64 decoder for PEM bodies (standard or URL-safe alphabet).
 *
 * Lenient where the result is unambiguous: whitespace is ignored, padding is
 * optional (any number of trailing `=`), non-zero trailing bits are dropped.
 * Errors: any other byte (cannot be decoded), data after padding (would be
 * silently dropped or mis-framed) and a dangling single sextet (no byte).
 *
 * Returns the decoded length, or (size_t)-1 on error / insufficient space.
 */
FIO_SFUNC size_t fio___pem_base64_decode(uint8_t *dest,
                                         size_t dest_len,
                                         const char *src,
                                         size_t len) {
  /* 0x00-0x3F: value, 0x40: '=', 0x80: whitespace, 0xFF: invalid */
  static const uint8_t map[256] = {
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x80, 0x80,
      0x80, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0x3E, 0xFF, 0x3F,
      0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF,
      0xFF, 0x40, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
      0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,
      0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F,
      0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
      0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
      0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF};
  const uint8_t *p = (const uint8_t *)src;
  const uint8_t *end = p + len;
  size_t out = 0;
  uint32_t acc = 0;
  size_t n = 0;   /* sextets in the current quantum */
  size_t pad = 0; /* '=' seen */
  for (; p < end; ++p) {
    const uint8_t v = map[*p];
    if (v < 0x40) {
      if (pad)
        return (size_t)-1; /* data after padding */
      acc = (acc << 6) | v;
      if (++n == 4) {
        if (dest_len - out < 3)
          return (size_t)-1;
        dest[out] = (uint8_t)(acc >> 16);
        dest[out + 1] = (uint8_t)(acc >> 8);
        dest[out + 2] = (uint8_t)acc;
        out += 3;
        n = 0;
        acc = 0;
      }
    } else if (v == 0x80) {
      continue;
    } else if (v == 0x40) {
      pad = 1;
    } else {
      return (size_t)-1; /* not decodable (e.g. RFC 1421 headers) */
    }
  }
  if (n == 1)
    return (size_t)-1; /* 6 bits: not a byte */
  if (n == 2) {        /* 12 bits -> 1 byte (extra bits ignored) */
    if (dest_len - out < 1)
      return (size_t)-1;
    dest[out++] = (uint8_t)(acc >> 4);
  } else if (n == 3) { /* 18 bits -> 2 bytes (extra bits ignored) */
    if (dest_len - out < 2)
      return (size_t)-1;
    dest[out] = (uint8_t)(acc >> 10);
    dest[out + 1] = (uint8_t)(acc >> 2);
    out += 2;
  }
  return out;
}

/* *****************************************************************************
Implementation - Core PEM Parser
***************************************************************************** */

SFUNC size_t fio_pem_parse(fio_pem_s *out,
                           uint8_t *der_buf,
                           size_t der_buf_len,
                           const char *pem_data,
                           size_t pem_len) {
  if (!out || !der_buf || !pem_data || pem_len == 0)
    return 0;

  FIO_MEMSET(out, 0, sizeof(*out));

  static const char begin_marker[] = "-----BEGIN ";
  static const char end_marker[] = "-----END ";
  static const char dashes[] = "-----";
  const char *const pem_end = pem_data + pem_len;

  const char *begin = fio___pem_find(pem_data, pem_len, begin_marker, 11);
  if (!begin)
    return 0;

  /* Extract label */
  const char *label_start = begin + 11;
  const char *label_end =
      fio___pem_find(label_start, (size_t)(pem_end - label_start), dashes, 5);
  if (!label_end)
    return 0;
  size_t label_len = (size_t)(label_end - label_start);

  /* Label must fit and stay on the BEGIN line (a broken BEGIN line must not
   * swallow the body up to the next run of dashes). */
  char end_pattern[128];
  if (label_len + 16 > sizeof(end_pattern))
    return 0;
  for (size_t i = 0; i < label_len; ++i) {
    if (label_start[i] == '\n' || label_start[i] == '\r')
      return 0;
  }

  /* Base64 data starts after the closing dashes */
  const char *data_start = label_end + 5;

  /* Find -----END <label>----- */
  FIO_MEMCPY(end_pattern, end_marker, 9);
  FIO_MEMCPY(end_pattern + 9, label_start, label_len);
  FIO_MEMCPY(end_pattern + 9 + label_len, dashes, 5);
  size_t end_pattern_len = 9 + label_len + 5;

  const char *end_at = fio___pem_find(data_start,
                                      (size_t)(pem_end - data_start),
                                      end_pattern,
                                      end_pattern_len);
  if (!end_at)
    return 0;

  /* Strict Base64 decode of everything between the markers */
  size_t der_len = fio___pem_base64_decode(der_buf,
                                           der_buf_len,
                                           data_start,
                                           (size_t)(end_at - data_start));
  if (der_len == (size_t)-1)
    return 0;

  out->der = der_buf;
  out->der_len = der_len;
  out->label = label_start;
  out->label_len = label_len;

  /* Consume through the END marker (untrimmed) and trailing whitespace */
  const char *consumed_end = end_at + end_pattern_len;
  while (consumed_end < pem_end &&
         (*consumed_end == '\r' || *consumed_end == '\n' ||
          *consumed_end == ' ' || *consumed_end == '\t'))
    ++consumed_end;

  return (size_t)(consumed_end - pem_data);
}

/* *****************************************************************************
Implementation - Certificate Parsing
***************************************************************************** */

SFUNC size_t fio_pem_get_certificate_der(uint8_t *der_out,
                                         size_t der_out_len,
                                         const char *pem_data,
                                         size_t pem_len) {
  if (!der_out || !pem_data || pem_len == 0)
    return 0;

  fio_pem_s pem;
  size_t consumed =
      fio_pem_parse(&pem, der_out, der_out_len, pem_data, pem_len);
  if (consumed == 0)
    return 0;

  /* Verify it's a certificate */
  if (pem.label_len != 11 || FIO_MEMCMP(pem.label, "CERTIFICATE", 11) != 0) {
    FIO_LOG_DEBUG("PEM: expected CERTIFICATE label, got '%.*s'",
                  (int)pem.label_len,
                  pem.label);
    return 0;
  }

  return pem.der_len;
}

SFUNC int fio_pem_parse_certificate(fio_x509_cert_s *cert,
                                    const char *pem_data,
                                    size_t pem_len) {
#if !defined(H___FIO_X509___H)
  FIO_LOG_ERROR("PEM: X.509 module not available");
  (void)cert;
  (void)pem_data;
  (void)pem_len;
  return -1;
#else
  if (!cert || !pem_data || pem_len == 0)
    return -1;
  FIO_MEMSET(cert, 0, sizeof(*cert));

  /* Decoded DER is always smaller than its Base64 armor */
  size_t der_buf_len = pem_len;
  uint8_t *der_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, der_buf_len, 0);
  if (!der_buf)
    return -1;

  size_t der_len =
      fio_pem_get_certificate_der(der_buf, der_buf_len, pem_data, pem_len);
  if (der_len == 0) {
    FIO_MEM_FREE(der_buf, der_buf_len);
    return -1;
  }
  /* Trim to the exact DER size so the allocation is described by cert->der
   * and can be released by fio_pem_certificate_free. */
  if (der_len != der_buf_len) {
    uint8_t *tmp =
        (uint8_t *)FIO_MEM_REALLOC(der_buf, der_buf_len, der_len, der_len);
    if (!tmp) {
      FIO_MEM_FREE(der_buf, der_buf_len);
      return -1;
    }
    der_buf = tmp;
  }

  /* The cert structure points into der_buf (owned by cert->der). */
  if (fio_x509_parse(cert, der_buf, der_len) != 0 || cert->der.buf != der_buf ||
      cert->der.len != der_len) {
    FIO_MEM_FREE(der_buf, der_len);
    FIO_MEMSET(cert, 0, sizeof(*cert));
    return -1;
  }
  return 0;
#endif
}

SFUNC void fio_pem_certificate_free(fio_x509_cert_s *cert) {
#if defined(H___FIO_X509___H)
  if (!cert)
    return;
  if (cert->der.buf)
    FIO_MEM_FREE(cert->der.buf, cert->der.len);
  FIO_MEMSET(cert, 0, sizeof(*cert));
#else
  (void)cert;
#endif
}

/* *****************************************************************************
Implementation - Strict DER helpers (exact tags, exact consumption)
***************************************************************************** */
#if defined(H___FIO_DER___H)

/** Parses one element that must span exactly `len` bytes with tag `tag`. */
FIO_SFUNC int fio___pem_der_whole(fio_der_element_s *e,
                                  const uint8_t *data,
                                  size_t len,
                                  uint8_t tag) {
  if (!data || !len || fio_der_parse(e, data, len) != data + len)
    return -1;
  return -(e->tag != tag);
}

/** Reads the next element of a SEQUENCE, which must have the exact tag. */
FIO_SFUNC int fio___pem_der_next(fio_der_iterator_s *it,
                                 fio_der_element_s *e,
                                 uint8_t tag) {
  if (fio_der_iterator_next(it, e))
    return -1;
  return -(e->tag != tag);
}

/** Peeks the tag byte of the next element (0 if none). */
FIO_IFUNC uint8_t fio___pem_der_peek(const fio_der_iterator_s *it) {
  return (it->pos && it->pos < it->end) ? it->pos[0] : 0;
}

/** True if the iterator consumed every byte of its container. */
FIO_IFUNC int fio___pem_der_done(const fio_der_iterator_s *it) {
  return it->pos == it->end;
}

/** Reads a single-byte version INTEGER (0..max); returns it or -1. */
FIO_SFUNC int fio___pem_der_version(fio_der_iterator_s *it, uint8_t max) {
  fio_der_element_s e;
  if (fio___pem_der_next(it, &e, 0x02) || e.len != 1 || e.data[0] > max)
    return -1;
  return (int)e.data[0];
}

/**
 * Copies a positive, minimally encoded, non-zero INTEGER into `dst`
 * (big-endian, without the sign padding byte).
 */
FIO_SFUNC int fio___pem_der_uint(fio_der_iterator_s *it,
                                 uint8_t *dst,
                                 size_t *dst_len,
                                 size_t cap) {
  fio_der_element_s e;
  if (fio___pem_der_next(it, &e, 0x02) || !e.len)
    return -1;
  const uint8_t *src = e.data;
  size_t len = e.len;
  if (src[0] & 0x80)
    return -1; /* negative */
  if (src[0] == 0) {
    if (len == 1 || !(src[1] & 0x80))
      return -1; /* zero or non-minimal */
    ++src;
    --len;
  }
  if (len > cap)
    return -1;
  FIO_MEMCPY(dst, src, len);
  *dst_len = len;
  return 0;
}

#endif /* H___FIO_DER___H */

/* *****************************************************************************
Implementation - RSA Private Key Parsing (PKCS#1)
***************************************************************************** */

/**
 * Parse PKCS#1 RSAPrivateKey (RFC 8017):
 *
 * RSAPrivateKey ::= SEQUENCE {
 *   version           Version,  -- 0 (two-prime); 1 (multi) unsupported
 *   modulus           INTEGER,  -- n
 *   publicExponent    INTEGER,  -- e
 *   privateExponent   INTEGER,  -- d
 *   prime1            INTEGER,  -- p
 *   prime2            INTEGER,  -- q
 *   exponent1         INTEGER,  -- d mod (p-1)
 *   exponent2         INTEGER,  -- d mod (q-1)
 *   coefficient       INTEGER,  -- (inverse of q) mod p
 *   otherPrimeInfos   OtherPrimeInfos OPTIONAL
 * }
 */
FIO_SFUNC int fio___pem_parse_rsa_private_key(fio_pem_private_key_s *key,
                                              const uint8_t *der,
                                              size_t der_len) {
#if !defined(H___FIO_DER___H)
  (void)key;
  (void)der;
  (void)der_len;
  return -1;
#else
  fio_der_element_s seq;
  fio_der_iterator_s it;
  if (!key || fio___pem_der_whole(&seq, der, der_len, 0x30))
    return -1;
  fio_der_iterator_init(&it, &seq);
  key->type = FIO_PEM_KEY_RSA;
  if (fio___pem_der_version(&it, 0) != 0 ||
      fio___pem_der_uint(&it, key->rsa.n, &key->rsa.n_len, FIO_RSA_MAX_BYTES) ||
      fio___pem_der_uint(&it, key->rsa.e, &key->rsa.e_len, FIO_RSA_MAX_BYTES) ||
      fio___pem_der_uint(&it, key->rsa.d, &key->rsa.d_len, FIO_RSA_MAX_BYTES) ||
      fio___pem_der_uint(&it, key->rsa.p, &key->rsa.p_len, FIO_RSA_MAX_BYTES) ||
      fio___pem_der_uint(&it, key->rsa.q, &key->rsa.q_len, FIO_RSA_MAX_BYTES) ||
      fio___pem_der_uint(&it,
                         key->rsa.dP,
                         &key->rsa.dP_len,
                         FIO_RSA_MAX_BYTES) ||
      fio___pem_der_uint(&it,
                         key->rsa.dQ,
                         &key->rsa.dQ_len,
                         FIO_RSA_MAX_BYTES) ||
      fio___pem_der_uint(&it,
                         key->rsa.qInv,
                         &key->rsa.qInv_len,
                         FIO_RSA_MAX_BYTES) ||
      !fio___pem_der_done(&it))
    return -1;
  return 0;
#endif
}

/* *****************************************************************************
Implementation - EC Private Key Parsing (SEC1)
***************************************************************************** */

/**
 * Parse SEC1 ECPrivateKey (RFC 5915), P-256 or P-384:
 *
 * ECPrivateKey ::= SEQUENCE {
 *   version        INTEGER { ecPrivkeyVer1(1) },
 *   privateKey     OCTET STRING,
 *   parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
 *   publicKey  [1] BIT STRING OPTIONAL
 * }
 *
 * `curve` is the curve named by an enclosing PKCS#8 AlgorithmIdentifier
 * (`FIO_PEM_KEY_ECDSA_P256` / `FIO_PEM_KEY_ECDSA_P384`) or
 * `FIO_PEM_KEY_UNKNOWN` for a bare SEC1 key. The curve is taken from the
 * `parameters` OID (which must agree with `curve` when both are known), else
 * from `curve`, else - for legacy files without parameters - from the scalar
 * length (32 bytes: P-256, 48 bytes: P-384). The scalar must be exactly the
 * curve's length (RFC 5915: ceiling(log2(n)/8) octets).
 */
FIO_SFUNC int fio___pem_parse_ec_private_key(fio_pem_private_key_s *key,
                                             const uint8_t *der,
                                             size_t der_len,
                                             fio_pem_key_type_e curve) {
#if !defined(H___FIO_DER___H)
  (void)key;
  (void)der;
  (void)der_len;
  (void)curve;
  return -1;
#else
  fio_der_element_s seq, e, inner, scalar;
  fio_der_iterator_s it;
  uint8_t *sk, *pk;
  int *has_pk;
  size_t *pk_len, sk_len;
  if (!key || fio___pem_der_whole(&seq, der, der_len, 0x30))
    return -1;
  if (curve != FIO_PEM_KEY_UNKNOWN && curve != FIO_PEM_KEY_ECDSA_P256 &&
      curve != FIO_PEM_KEY_ECDSA_P384)
    return -1;
  fio_der_iterator_init(&it, &seq);

  if (fio___pem_der_version(&it, 1) != 1)
    return -1;
  if (fio___pem_der_next(&it, &scalar, 0x04))
    return -1;

  /* parameters [0] EXPLICIT: exactly one namedCurve OID, secp256r1 or
   * secp384r1, matching the PKCS#8 AlgorithmIdentifier (if any) */
  if (fio___pem_der_peek(&it) == 0xA0) {
    fio_u128 oid_val;
    fio_pem_key_type_e named;
    if (fio_der_iterator_next(&it, &e) ||
        fio___pem_der_whole(&inner, e.data, e.len, 0x06))
      return -1;
    oid_val = fio___der_oid_value(&inner);
    if (fio___der_oid_eq(oid_val, FIO_X509_OID_SECP256R1))
      named = FIO_PEM_KEY_ECDSA_P256;
    else if (fio___der_oid_eq(oid_val, FIO_X509_OID_SECP384R1))
      named = FIO_PEM_KEY_ECDSA_P384;
    else
      return -1;
    if (curve != FIO_PEM_KEY_UNKNOWN && curve != named)
      return -1;
    curve = named;
  }
  if (curve == FIO_PEM_KEY_UNKNOWN)
    curve =
        (scalar.len == 48) ? FIO_PEM_KEY_ECDSA_P384 : FIO_PEM_KEY_ECDSA_P256;

  if (curve == FIO_PEM_KEY_ECDSA_P384) {
    sk = key->ecdsa_p384.private_key;
    sk_len = 48;
    pk = key->ecdsa_p384.public_key;
    has_pk = &key->ecdsa_p384.has_public_key;
    pk_len = &key->ecdsa_p384.public_key_len;
    FIO_MEMSET(pk, 0, sizeof(key->ecdsa_p384.public_key));
  } else {
    sk = key->ecdsa_p256.private_key;
    sk_len = 32;
    pk = key->ecdsa_p256.public_key;
    has_pk = &key->ecdsa_p256.has_public_key;
    pk_len = &key->ecdsa_p256.public_key_len;
    FIO_MEMSET(pk, 0, sizeof(key->ecdsa_p256.public_key));
  }
  if (scalar.len != sk_len)
    return -1;
  key->type = curve;
  FIO_MEMCPY(sk, scalar.data, sk_len);
  *has_pk = 0;
  *pk_len = 0;

  /* publicKey [1] EXPLICIT: BIT STRING, no unused bits, SEC 1 point:
   * uncompressed (04 || x || y) or compressed (02/03 || x). The encoding is
   * checked here; consumers compare it with the public key they derive. */
  if (fio___pem_der_peek(&it) == 0xA1) {
    const uint8_t *bits;
    size_t bits_len;
    uint8_t unused;
    if (fio_der_iterator_next(&it, &e) ||
        fio___pem_der_whole(&inner, e.data, e.len, 0x03) ||
        fio_der_parse_bit_string(&inner, &bits, &bits_len, &unused) || unused ||
        !((bits_len == (sk_len << 1) + 1 && bits[0] == 0x04) ||
          (bits_len == sk_len + 1 && (bits[0] == 0x02 || bits[0] == 0x03))))
      return -1;
    FIO_MEMCPY(pk, bits, bits_len);
    *has_pk = 1;
    *pk_len = bits_len;
  }
  return -(!fio___pem_der_done(&it));
#endif
}

/* *****************************************************************************
Implementation - PKCS#8 Private Key Parsing
***************************************************************************** */

/**
 * Parse PKCS#8 / OneAsymmetricKey (RFC 5208 / RFC 5958):
 *
 * PrivateKeyInfo ::= SEQUENCE {
 *   version                   Version,            -- 0 (v1) or 1 (v2)
 *   privateKeyAlgorithm       AlgorithmIdentifier,
 *   privateKey                OCTET STRING,
 *   attributes           [0]  IMPLICIT Attributes OPTIONAL,
 *   publicKey            [1]  IMPLICIT BIT STRING OPTIONAL  -- v2 only
 * }
 */
FIO_SFUNC int fio___pem_parse_pkcs8(fio_pem_private_key_s *key,
                                    const uint8_t *der,
                                    size_t der_len) {
#if !defined(H___FIO_DER___H)
  (void)key;
  (void)der;
  (void)der_len;
  return -1;
#else
  fio_der_element_s seq, alg, oid, params, priv, e;
  fio_der_iterator_s it, alg_it;
  int has_params = 0;
  if (!key || fio___pem_der_whole(&seq, der, der_len, 0x30))
    return -1;
  fio_der_iterator_init(&it, &seq);

  int version = fio___pem_der_version(&it, 1);
  if (version < 0)
    return -1;

  /* AlgorithmIdentifier ::= SEQUENCE { OID, parameters ANY OPTIONAL } */
  if (fio___pem_der_next(&it, &alg, 0x30))
    return -1;
  fio_der_iterator_init(&alg_it, &alg);
  if (fio___pem_der_next(&alg_it, &oid, 0x06))
    return -1;
  if (!fio___pem_der_done(&alg_it)) {
    if (fio_der_iterator_next(&alg_it, &params) || !fio___pem_der_done(&alg_it))
      return -1;
    has_params = 1;
  }

  if (fio___pem_der_next(&it, &priv, 0x04))
    return -1;

  /* Optional trailing fields, in order */
  if (fio___pem_der_peek(&it) == 0xA0 && fio_der_iterator_next(&it, &e))
    return -1;
  if (fio___pem_der_peek(&it) == 0x81) {
    if (version != 1 || fio_der_iterator_next(&it, &e) || !e.len || e.data[0])
      return -1;
  }
  if (!fio___pem_der_done(&it))
    return -1;

  fio_u128 oid_val = fio___der_oid_value(&oid);
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_RSA_ENCRYPTION)) {
    /* parameters: NULL (RFC 8017), tolerate absence */
    if (has_params && (params.tag != 0x05 || params.len))
      return -1;
    return fio___pem_parse_rsa_private_key(key, priv.data, priv.len);
  }
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_EC_PUBLIC_KEY)) {
    /* parameters: namedCurve, secp256r1 or secp384r1 (RFC 5480) */
    fio_pem_key_type_e curve = FIO_PEM_KEY_UNKNOWN;
    if (!has_params || params.tag != 0x06)
      return -1;
    if (fio___der_oid_eq(fio___der_oid_value(&params), FIO_X509_OID_SECP256R1))
      curve = FIO_PEM_KEY_ECDSA_P256;
    else if (fio___der_oid_eq(fio___der_oid_value(&params),
                              FIO_X509_OID_SECP384R1))
      curve = FIO_PEM_KEY_ECDSA_P384;
    else
      return -1;
    return fio___pem_parse_ec_private_key(key, priv.data, priv.len, curve);
  }
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_ED25519)) {
    /* parameters MUST be absent; CurvePrivateKey ::= OCTET STRING (RFC 8410).
     * An optional v2 publicKey is not imported (callers derive it). */
    fio_der_element_s seed;
    if (has_params || fio___pem_der_whole(&seed, priv.data, priv.len, 0x04) ||
        seed.len != 32)
      return -1;
    key->type = FIO_PEM_KEY_ED25519;
    fio_memcpy32(key->ed25519.private_key, seed.data);
    key->ed25519.has_public_key = 0;
    return 0;
  }
  return -1; /* Unknown algorithm */
#endif
}

/* *****************************************************************************
Implementation - Main Private Key Parser
***************************************************************************** */

SFUNC int fio_pem_parse_private_key(fio_pem_private_key_s *key,
                                    const char *pem_data,
                                    size_t pem_len) {
  if (!key || !pem_data || pem_len == 0)
    return -1;

  FIO_MEMSET(key, 0, sizeof(*key));

  /* Allocate buffer for DER data */
  size_t der_buf_len = pem_len;
  uint8_t *der_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, der_buf_len, 0);
  if (!der_buf)
    return -1;

  int result = -1;
  fio_pem_s pem;
  if (fio_pem_parse(&pem, der_buf, der_buf_len, pem_data, pem_len)) {
    /* Dispatch based on label */
    if (pem.label_len == 11 && FIO_MEMCMP(pem.label, "PRIVATE KEY", 11) == 0) {
      result = fio___pem_parse_pkcs8(key, pem.der, pem.der_len);
    } else if (pem.label_len == 15 &&
               FIO_MEMCMP(pem.label, "RSA PRIVATE KEY", 15) == 0) {
      result = fio___pem_parse_rsa_private_key(key, pem.der, pem.der_len);
    } else if (pem.label_len == 14 &&
               FIO_MEMCMP(pem.label, "EC PRIVATE KEY", 14) == 0) {
      result = fio___pem_parse_ec_private_key(key,
                                              pem.der,
                                              pem.der_len,
                                              FIO_PEM_KEY_UNKNOWN);
    } else {
      FIO_LOG_DEBUG("PEM: unsupported private key label '%.*s'",
                    (int)pem.label_len,
                    pem.label);
    }
  }

  /* The buffer held (possibly partially) decoded secret key material. */
  fio_secure_zero(der_buf, der_buf_len);
  FIO_MEM_FREE(der_buf, der_buf_len);

  if (result != 0)
    fio_pem_private_key_clear(key);
  return result;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_PEM */
#undef FIO_PEM
