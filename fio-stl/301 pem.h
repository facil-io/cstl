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
      const uint8_t *n; /**< RSA modulus (big-endian) */
      size_t n_len;
      const uint8_t *e; /**< RSA public exponent (big-endian) */
      size_t e_len;
      const uint8_t *d; /**< RSA private exponent (big-endian) */
      size_t d_len;
      const uint8_t *p; /**< RSA prime p (optional) */
      size_t p_len;
      const uint8_t *q; /**< RSA prime q (optional) */
      size_t q_len;
    } rsa;
    struct {
      uint8_t private_key[32]; /**< P-256 scalar (32 bytes) */
      uint8_t public_key[65];  /**< Uncompressed point (optional, can derive) */
      int has_public_key;      /**< 1 if public_key is populated */
    } ecdsa_p256;
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
 * @param out Output structure to fill with parsed PEM block info
 * @param der_buf Buffer to store decoded DER data (caller-provided)
 * @param der_buf_len Size of der_buf
 * @param pem_data PEM-encoded data
 * @param pem_len Length of PEM data
 * @return Number of bytes consumed from pem_data, or 0 on error
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
 * @param cert Output certificate structure (from fio_x509.h)
 * @param pem_data PEM-encoded certificate data
 * @param pem_len Length of PEM data
 * @return 0 on success, -1 on error
 */
SFUNC int fio_pem_parse_certificate(fio_x509_cert_s *cert,
                                    const char *pem_data,
                                    size_t pem_len);

/**
 * Parse private key from PEM file content.
 *
 * Supports:
 * - "PRIVATE KEY" (PKCS#8 PrivateKeyInfo)
 * - "RSA PRIVATE KEY" (PKCS#1 RSAPrivateKey)
 * - "EC PRIVATE KEY" (SEC1 ECPrivateKey)
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

/** Find a string in data (like memmem but simpler) */
FIO_SFUNC const char *fio___pem_find(const char *haystack,
                                     size_t haystack_len,
                                     const char *needle,
                                     size_t needle_len) {
  if (needle_len > haystack_len || needle_len == 0)
    return NULL;

  const char *end = haystack + haystack_len - needle_len + 1;
  for (const char *p = haystack; p < end; ++p) {
    if (FIO_MEMCMP(p, needle, needle_len) == 0)
      return p;
  }
  return NULL;
}

/** Skip whitespace and newlines */
FIO_SFUNC const char *fio___pem_skip_ws(const char *p, const char *end) {
  while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
    ++p;
  return p;
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

  /* Find -----BEGIN */
  static const char begin_marker[] = "-----BEGIN ";
  static const char end_marker[] = "-----END ";
  static const char dashes[] = "-----";

  const char *begin = fio___pem_find(pem_data, pem_len, begin_marker, 11);
  if (!begin)
    return 0;

  /* Extract label */
  const char *label_start = begin + 11;
  const char *label_end = fio___pem_find(label_start,
                                         pem_len - (label_start - pem_data),
                                         dashes,
                                         5);
  if (!label_end)
    return 0;

  out->label = label_start;
  out->label_len = (size_t)(label_end - label_start);

  /* Find start of base64 data (after -----) */
  const char *data_start = label_end + 5;
  data_start = fio___pem_skip_ws(data_start, pem_data + pem_len);

  /* Find -----END <label>----- */
  char end_pattern[128];
  if (out->label_len + 16 > sizeof(end_pattern))
    return 0;

  FIO_MEMCPY(end_pattern, end_marker, 9);
  FIO_MEMCPY(end_pattern + 9, out->label, out->label_len);
  FIO_MEMCPY(end_pattern + 9 + out->label_len, dashes, 5);
  size_t end_pattern_len = 9 + out->label_len + 5;

  const char *data_end = fio___pem_find(data_start,
                                        pem_len - (data_start - pem_data),
                                        end_pattern,
                                        end_pattern_len);
  if (!data_end)
    return 0;

  /* Trim trailing whitespace from data */
  while (data_end > data_start &&
         (data_end[-1] == ' ' || data_end[-1] == '\t' || data_end[-1] == '\r' ||
          data_end[-1] == '\n'))
    --data_end;

  /* Base64 decode the data */
  size_t base64_len = (size_t)(data_end - data_start);
  fio_str_info_s der_str = FIO_STR_INFO3((char *)der_buf, 0, der_buf_len);

  if (fio_string_write_base64dec(&der_str, NULL, data_start, base64_len) != 0) {
    /* Buffer too small or decode error */
    return 0;
  }

  out->der = der_buf;
  out->der_len = der_str.len;

  /* Return total bytes consumed (up to and including end marker) */
  const char *consumed_end = data_end + end_pattern_len;
  /* Skip past any trailing newline */
  while (consumed_end < pem_data + pem_len &&
         (*consumed_end == '\r' || *consumed_end == '\n'))
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

  /* Allocate buffer for DER data (PEM is ~4/3 larger due to base64) */
  size_t der_buf_len = pem_len; /* Conservative estimate */
  uint8_t *der_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, der_buf_len, 0);
  if (!der_buf)
    return -1;

  size_t der_len =
      fio_pem_get_certificate_der(der_buf, der_buf_len, pem_data, pem_len);
  if (der_len == 0) {
    FIO_MEM_FREE(der_buf, der_buf_len);
    return -1;
  }

  /* Parse the X.509 certificate */
  int result = fio_x509_parse(cert, der_buf, der_len);

  /* Note: The cert structure contains pointers into der_buf,
   * so we cannot free it here. The caller must manage the lifetime.
   * For now, we leak the buffer - a proper implementation would
   * require the caller to provide the buffer or use a different API. */
  if (result != 0) {
    FIO_MEM_FREE(der_buf, der_buf_len);
  }
  /* If successful, der_buf is intentionally not freed - cert points into it */

  return result;
#endif
}

/* *****************************************************************************
Implementation - PKCS#8 Private Key Parsing
***************************************************************************** */

/**
 * Parse PKCS#8 PrivateKeyInfo (RFC 5958):
 *
 * PrivateKeyInfo ::= SEQUENCE {
 *   version                   Version,
 *   privateKeyAlgorithm       AlgorithmIdentifier,
 *   privateKey                OCTET STRING,
 *   attributes           [0]  IMPLICIT Attributes OPTIONAL
 * }
 */
FIO_SFUNC int fio___pem_parse_pkcs8(fio_pem_private_key_s *key,
                                    const uint8_t *der,
                                    size_t der_len) {
#if !defined(H___FIO_ASN1___H)
  (void)key;
  (void)der;
  (void)der_len;
  return -1;
#else
  if (!key || !der || der_len == 0)
    return -1;

  fio_asn1_element_s seq;
  if (!fio_asn1_parse(&seq, der, der_len))
    return -1;
  if (!fio_asn1_is_tag(&seq, FIO_ASN1_SEQUENCE))
    return -1;

  fio_asn1_iterator_s it;
  fio_asn1_element_s elem;
  fio_asn1_iterator_init(&it, &seq);

  /* Parse version (INTEGER) */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER))
    return -1;

  /* Parse privateKeyAlgorithm (AlgorithmIdentifier = SEQUENCE) */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE))
    return -1;

  /* Extract algorithm OID */
  fio_asn1_iterator_s alg_it;
  fio_asn1_element_s oid, params;
  fio_asn1_iterator_init(&alg_it, &elem);

  if (fio_asn1_iterator_next(&alg_it, &oid) != 0)
    return -1;
  if (!fio_asn1_is_tag(&oid, FIO_ASN1_OID))
    return -1;

  /* Parse privateKey (OCTET STRING) */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_OCTET_STRING))
    return -1;

  const uint8_t *priv_key_data = elem.data;
  size_t priv_key_len = elem.len;

  /* Determine key type from algorithm OID */
  if (fio_asn1_oid_eq(&oid, FIO_OID_RSA_ENCRYPTION)) {
    /* RSA private key - privateKey contains RSAPrivateKey */
    key->type = FIO_PEM_KEY_RSA;

    /* Parse RSAPrivateKey structure */
    fio_asn1_element_s rsa_seq;
    if (!fio_asn1_parse(&rsa_seq, priv_key_data, priv_key_len))
      return -1;
    if (!fio_asn1_is_tag(&rsa_seq, FIO_ASN1_SEQUENCE))
      return -1;

    fio_asn1_iterator_s rsa_it;
    fio_asn1_element_s rsa_elem;
    fio_asn1_iterator_init(&rsa_it, &rsa_seq);

    /* version */
    if (fio_asn1_iterator_next(&rsa_it, &rsa_elem) != 0)
      return -1;

    /* modulus (n) */
    if (fio_asn1_iterator_next(&rsa_it, &rsa_elem) != 0)
      return -1;
    if (!fio_asn1_is_tag(&rsa_elem, FIO_ASN1_INTEGER))
      return -1;
    key->rsa.n = rsa_elem.data;
    key->rsa.n_len = rsa_elem.len;
    /* Skip leading zero if present */
    if (key->rsa.n_len > 1 && key->rsa.n[0] == 0x00) {
      key->rsa.n++;
      key->rsa.n_len--;
    }

    /* publicExponent (e) */
    if (fio_asn1_iterator_next(&rsa_it, &rsa_elem) != 0)
      return -1;
    if (!fio_asn1_is_tag(&rsa_elem, FIO_ASN1_INTEGER))
      return -1;
    key->rsa.e = rsa_elem.data;
    key->rsa.e_len = rsa_elem.len;
    if (key->rsa.e_len > 1 && key->rsa.e[0] == 0x00) {
      key->rsa.e++;
      key->rsa.e_len--;
    }

    /* privateExponent (d) */
    if (fio_asn1_iterator_next(&rsa_it, &rsa_elem) != 0)
      return -1;
    if (!fio_asn1_is_tag(&rsa_elem, FIO_ASN1_INTEGER))
      return -1;
    key->rsa.d = rsa_elem.data;
    key->rsa.d_len = rsa_elem.len;
    if (key->rsa.d_len > 1 && key->rsa.d[0] == 0x00) {
      key->rsa.d++;
      key->rsa.d_len--;
    }

    /* prime1 (p) */
    if (fio_asn1_iterator_next(&rsa_it, &rsa_elem) == 0 &&
        fio_asn1_is_tag(&rsa_elem, FIO_ASN1_INTEGER)) {
      key->rsa.p = rsa_elem.data;
      key->rsa.p_len = rsa_elem.len;
      if (key->rsa.p_len > 1 && key->rsa.p[0] == 0x00) {
        key->rsa.p++;
        key->rsa.p_len--;
      }

      /* prime2 (q) */
      if (fio_asn1_iterator_next(&rsa_it, &rsa_elem) == 0 &&
          fio_asn1_is_tag(&rsa_elem, FIO_ASN1_INTEGER)) {
        key->rsa.q = rsa_elem.data;
        key->rsa.q_len = rsa_elem.len;
        if (key->rsa.q_len > 1 && key->rsa.q[0] == 0x00) {
          key->rsa.q++;
          key->rsa.q_len--;
        }
      }
    }

    return 0;

  } else if (fio_asn1_oid_eq(&oid, FIO_OID_EC_PUBLIC_KEY)) {
    /* EC private key - check curve parameter */
    if (fio_asn1_iterator_next(&alg_it, &params) != 0)
      return -1;

    if (fio_asn1_oid_eq(&params, FIO_OID_SECP256R1)) {
      key->type = FIO_PEM_KEY_ECDSA_P256;

      /* Parse ECPrivateKey structure (SEC1) */
      fio_asn1_element_s ec_seq;
      if (!fio_asn1_parse(&ec_seq, priv_key_data, priv_key_len))
        return -1;
      if (!fio_asn1_is_tag(&ec_seq, FIO_ASN1_SEQUENCE))
        return -1;

      fio_asn1_iterator_s ec_it;
      fio_asn1_element_s ec_elem;
      fio_asn1_iterator_init(&ec_it, &ec_seq);

      /* version */
      if (fio_asn1_iterator_next(&ec_it, &ec_elem) != 0)
        return -1;

      /* privateKey (OCTET STRING, 32 bytes for P-256) */
      if (fio_asn1_iterator_next(&ec_it, &ec_elem) != 0)
        return -1;
      if (!fio_asn1_is_tag(&ec_elem, FIO_ASN1_OCTET_STRING))
        return -1;
      if (ec_elem.len != 32)
        return -1;

      FIO_MEMCPY(key->ecdsa_p256.private_key, ec_elem.data, 32);

      /* Optional: publicKey [1] BIT STRING */
      key->ecdsa_p256.has_public_key = 0;
      while (fio_asn1_iterator_next(&ec_it, &ec_elem) == 0) {
        if (fio_asn1_is_context_tag(&ec_elem, 1)) {
          /* Parse BIT STRING containing public key */
          fio_asn1_element_s bits;
          if (fio_asn1_parse(&bits, ec_elem.data, ec_elem.len) &&
              fio_asn1_is_tag(&bits, FIO_ASN1_BIT_STRING)) {
            const uint8_t *pubkey_bits;
            size_t pubkey_len;
            uint8_t unused;
            if (fio_asn1_parse_bit_string(&bits,
                                          &pubkey_bits,
                                          &pubkey_len,
                                          &unused) == 0) {
              if (pubkey_len == 65 && pubkey_bits[0] == 0x04) {
                FIO_MEMCPY(key->ecdsa_p256.public_key, pubkey_bits, 65);
                key->ecdsa_p256.has_public_key = 1;
              }
            }
          }
          break;
        }
      }

      return 0;
    }
    /* Other curves not supported */
    return -1;

  } else if (fio_asn1_oid_eq(&oid, FIO_OID_ED25519)) {
    /* Ed25519 private key */
    key->type = FIO_PEM_KEY_ED25519;

    /* For Ed25519, privateKey is an OCTET STRING containing another
     * OCTET STRING with the 32-byte seed */
    fio_asn1_element_s seed;
    if (!fio_asn1_parse(&seed, priv_key_data, priv_key_len))
      return -1;
    if (!fio_asn1_is_tag(&seed, FIO_ASN1_OCTET_STRING))
      return -1;
    if (seed.len != 32)
      return -1;

    FIO_MEMCPY(key->ed25519.private_key, seed.data, 32);
    key->ed25519.has_public_key = 0;

    return 0;
  }

  return -1; /* Unknown algorithm */
#endif
}

/* *****************************************************************************
Implementation - Legacy RSA Private Key Parsing (PKCS#1)
***************************************************************************** */

/**
 * Parse PKCS#1 RSAPrivateKey:
 *
 * RSAPrivateKey ::= SEQUENCE {
 *   version           Version,
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
#if !defined(H___FIO_ASN1___H)
  (void)key;
  (void)der;
  (void)der_len;
  return -1;
#else
  if (!key || !der || der_len == 0)
    return -1;

  key->type = FIO_PEM_KEY_RSA;

  fio_asn1_element_s seq;
  if (!fio_asn1_parse(&seq, der, der_len))
    return -1;
  if (!fio_asn1_is_tag(&seq, FIO_ASN1_SEQUENCE))
    return -1;

  fio_asn1_iterator_s it;
  fio_asn1_element_s elem;
  fio_asn1_iterator_init(&it, &seq);

  /* version */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER))
    return -1;

  /* modulus (n) */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER))
    return -1;
  key->rsa.n = elem.data;
  key->rsa.n_len = elem.len;
  if (key->rsa.n_len > 1 && key->rsa.n[0] == 0x00) {
    key->rsa.n++;
    key->rsa.n_len--;
  }

  /* publicExponent (e) */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER))
    return -1;
  key->rsa.e = elem.data;
  key->rsa.e_len = elem.len;
  if (key->rsa.e_len > 1 && key->rsa.e[0] == 0x00) {
    key->rsa.e++;
    key->rsa.e_len--;
  }

  /* privateExponent (d) */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER))
    return -1;
  key->rsa.d = elem.data;
  key->rsa.d_len = elem.len;
  if (key->rsa.d_len > 1 && key->rsa.d[0] == 0x00) {
    key->rsa.d++;
    key->rsa.d_len--;
  }

  /* prime1 (p) */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER))
    return -1;
  key->rsa.p = elem.data;
  key->rsa.p_len = elem.len;
  if (key->rsa.p_len > 1 && key->rsa.p[0] == 0x00) {
    key->rsa.p++;
    key->rsa.p_len--;
  }

  /* prime2 (q) */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER))
    return -1;
  key->rsa.q = elem.data;
  key->rsa.q_len = elem.len;
  if (key->rsa.q_len > 1 && key->rsa.q[0] == 0x00) {
    key->rsa.q++;
    key->rsa.q_len--;
  }

  return 0;
#endif
}

/* *****************************************************************************
Implementation - Legacy EC Private Key Parsing (SEC1)
***************************************************************************** */

/**
 * Parse SEC1 ECPrivateKey:
 *
 * ECPrivateKey ::= SEQUENCE {
 *   version        INTEGER { ecPrivkeyVer1(1) },
 *   privateKey     OCTET STRING,
 *   parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
 *   publicKey  [1] BIT STRING OPTIONAL
 * }
 */
FIO_SFUNC int fio___pem_parse_ec_private_key(fio_pem_private_key_s *key,
                                             const uint8_t *der,
                                             size_t der_len) {
#if !defined(H___FIO_ASN1___H)
  (void)key;
  (void)der;
  (void)der_len;
  return -1;
#else
  if (!key || !der || der_len == 0)
    return -1;

  fio_asn1_element_s seq;
  if (!fio_asn1_parse(&seq, der, der_len))
    return -1;
  if (!fio_asn1_is_tag(&seq, FIO_ASN1_SEQUENCE))
    return -1;

  fio_asn1_iterator_s it;
  fio_asn1_element_s elem;
  fio_asn1_iterator_init(&it, &seq);

  /* version */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER))
    return -1;

  /* privateKey (OCTET STRING) */
  if (fio_asn1_iterator_next(&it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_OCTET_STRING))
    return -1;

  /* Determine curve from key length or parameters */
  if (elem.len == 32) {
    /* Assume P-256 based on key length */
    key->type = FIO_PEM_KEY_ECDSA_P256;
    FIO_MEMCPY(key->ecdsa_p256.private_key, elem.data, 32);
    key->ecdsa_p256.has_public_key = 0;

    /* Look for parameters [0] to confirm curve */
    while (fio_asn1_iterator_next(&it, &elem) == 0) {
      if (fio_asn1_is_context_tag(&elem, 0)) {
        /* Parse curve OID */
        fio_asn1_element_s curve_oid;
        if (fio_asn1_parse(&curve_oid, elem.data, elem.len) &&
            fio_asn1_is_tag(&curve_oid, FIO_ASN1_OID)) {
          if (!fio_asn1_oid_eq(&curve_oid, FIO_OID_SECP256R1)) {
            /* Not P-256, unsupported */
            return -1;
          }
        }
      } else if (fio_asn1_is_context_tag(&elem, 1)) {
        /* Parse public key [1] BIT STRING */
        fio_asn1_element_s bits;
        if (fio_asn1_parse(&bits, elem.data, elem.len) &&
            fio_asn1_is_tag(&bits, FIO_ASN1_BIT_STRING)) {
          const uint8_t *pubkey_bits;
          size_t pubkey_len;
          uint8_t unused;
          if (fio_asn1_parse_bit_string(&bits,
                                        &pubkey_bits,
                                        &pubkey_len,
                                        &unused) == 0) {
            if (pubkey_len == 65 && pubkey_bits[0] == 0x04) {
              FIO_MEMCPY(key->ecdsa_p256.public_key, pubkey_bits, 65);
              key->ecdsa_p256.has_public_key = 1;
            }
          }
        }
      }
    }

    return 0;
  }

  return -1; /* Unsupported key length/curve */
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

  fio_pem_s pem;
  size_t consumed =
      fio_pem_parse(&pem, der_buf, der_buf_len, pem_data, pem_len);
  if (consumed == 0) {
    FIO_MEM_FREE(der_buf, der_buf_len);
    return -1;
  }

  int result = -1;

  /* Dispatch based on label */
  if (pem.label_len == 11 && FIO_MEMCMP(pem.label, "PRIVATE KEY", 11) == 0) {
    /* PKCS#8 PrivateKeyInfo */
    result = fio___pem_parse_pkcs8(key, pem.der, pem.der_len);
  } else if (pem.label_len == 15 &&
             FIO_MEMCMP(pem.label, "RSA PRIVATE KEY", 15) == 0) {
    /* PKCS#1 RSAPrivateKey */
    result = fio___pem_parse_rsa_private_key(key, pem.der, pem.der_len);
  } else if (pem.label_len == 14 &&
             FIO_MEMCMP(pem.label, "EC PRIVATE KEY", 14) == 0) {
    /* SEC1 ECPrivateKey */
    result = fio___pem_parse_ec_private_key(key, pem.der, pem.der_len);
  } else {
    FIO_LOG_DEBUG("PEM: unsupported private key label '%.*s'",
                  (int)pem.label_len,
                  pem.label);
  }

  FIO_MEM_FREE(der_buf, der_buf_len);

  if (result != 0) {
    fio_pem_private_key_clear(key);
  }

  return result;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_PEM */
#undef FIO_PEM
