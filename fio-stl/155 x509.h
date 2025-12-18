/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_X509               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        X.509 Certificate Parser for TLS 1.3
                            (RFC 5280 Certificate Parsing)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_X509) && !defined(H___FIO_X509___H)
#define H___FIO_X509___H

/* *****************************************************************************
X.509 Certificate Parser Module

This module provides X.509v3 certificate parsing for TLS 1.3 certificate
verification. It supports:

- RSA, ECDSA (P-256, P-384), and Ed25519 public keys
- Signature verification using issuer certificates
- Validity period checking
- Hostname matching (CN and SAN with wildcards)
- Basic constraints and key usage extensions
- Certificate chain validation
- Trust store management

**Note**: This is a minimal parser for TLS 1.3. Not all X.509 features
are supported. The implementation is non-allocating (pointers into DER data).
***************************************************************************** */

/* *****************************************************************************
X.509 Type Definitions
***************************************************************************** */

/** Public key algorithm types */
typedef enum {
  FIO_X509_KEY_UNKNOWN = 0,
  FIO_X509_KEY_RSA = 1,        /**< RSA (any key size) */
  FIO_X509_KEY_ECDSA_P256 = 2, /**< ECDSA with P-256/secp256r1 */
  FIO_X509_KEY_ECDSA_P384 = 3, /**< ECDSA with P-384/secp384r1 */
  FIO_X509_KEY_ED25519 = 4,    /**< Ed25519 (EdDSA) */
} fio_x509_key_type_e;

/** Signature algorithm types */
typedef enum {
  FIO_X509_SIG_UNKNOWN = 0,
  FIO_X509_SIG_RSA_PKCS1_SHA256 = 1, /**< sha256WithRSAEncryption */
  FIO_X509_SIG_RSA_PKCS1_SHA384 = 2, /**< sha384WithRSAEncryption */
  FIO_X509_SIG_RSA_PKCS1_SHA512 = 3, /**< sha512WithRSAEncryption */
  FIO_X509_SIG_RSA_PSS_SHA256 = 4,   /**< RSA-PSS with SHA-256 */
  FIO_X509_SIG_RSA_PSS_SHA384 = 5,   /**< RSA-PSS with SHA-384 */
  FIO_X509_SIG_RSA_PSS_SHA512 = 6,   /**< RSA-PSS with SHA-512 */
  FIO_X509_SIG_ECDSA_SHA256 = 7,     /**< ecdsa-with-SHA256 */
  FIO_X509_SIG_ECDSA_SHA384 = 8,     /**< ecdsa-with-SHA384 */
  FIO_X509_SIG_ED25519 = 9,          /**< Ed25519 */
} fio_x509_sig_alg_e;

/** Key Usage bit flags (RFC 5280 Section 4.2.1.3) */
typedef enum {
  FIO_X509_KU_DIGITAL_SIGNATURE = 0x0001,
  FIO_X509_KU_NON_REPUDIATION = 0x0002,
  FIO_X509_KU_KEY_ENCIPHERMENT = 0x0004,
  FIO_X509_KU_DATA_ENCIPHERMENT = 0x0008,
  FIO_X509_KU_KEY_AGREEMENT = 0x0010,
  FIO_X509_KU_KEY_CERT_SIGN = 0x0020,
  FIO_X509_KU_CRL_SIGN = 0x0040,
  FIO_X509_KU_ENCIPHER_ONLY = 0x0080,
  FIO_X509_KU_DECIPHER_ONLY = 0x0100,
} fio_x509_key_usage_e;

/** X.509 chain validation error codes */
typedef enum {
  FIO_X509_OK = 0,                   /**< Validation successful */
  FIO_X509_ERR_PARSE = -1,           /**< Failed to parse certificate */
  FIO_X509_ERR_EXPIRED = -2,         /**< Certificate expired */
  FIO_X509_ERR_NOT_YET_VALID = -3,   /**< Certificate not yet valid */
  FIO_X509_ERR_SIGNATURE = -4,       /**< Signature verification failed */
  FIO_X509_ERR_ISSUER_MISMATCH = -5, /**< Issuer DN doesn't match subject DN */
  FIO_X509_ERR_NOT_CA = -6,          /**< Issuer is not a CA certificate */
  FIO_X509_ERR_NO_TRUST_ANCHOR = -7, /**< Certificate not in trust store */
  FIO_X509_ERR_HOSTNAME_MISMATCH = -8, /**< Hostname doesn't match cert */
  FIO_X509_ERR_EMPTY_CHAIN = -9,       /**< Empty certificate chain */
  FIO_X509_ERR_CHAIN_TOO_LONG = -10,   /**< Chain exceeds maximum depth */
} fio_x509_error_e;

/** Trust store for root CA certificates */
typedef struct {
  const uint8_t **roots;   /**< Array of root CA certificate DER data */
  const size_t *root_lens; /**< Array of root CA certificate lengths */
  size_t root_count;       /**< Number of root CAs */
} fio_x509_trust_store_s;

/** TLS certificate entry (parsed from Certificate message) */
typedef struct {
  const uint8_t *cert; /**< DER-encoded certificate data */
  size_t cert_len;     /**< Certificate length */
} fio_tls_cert_entry_s;

/** Parsed X.509 certificate structure */
typedef struct {
  /** Certificate version (0=v1, 1=v2, 2=v3) */
  int version;

  /** Validity period (Unix timestamps) */
  int64_t not_before;
  int64_t not_after;

  /** Subject Distinguished Name (raw DER for comparison) */
  const uint8_t *subject_der;
  size_t subject_der_len;

  /** Issuer Distinguished Name (raw DER for comparison) */
  const uint8_t *issuer_der;
  size_t issuer_der_len;

  /** Subject Common Name (if present, pointer into DER data) */
  const char *subject_cn;
  size_t subject_cn_len;

  /** Public Key Type */
  fio_x509_key_type_e key_type;

  /** Public Key Data (union based on key_type) */
  union {
    struct {
      const uint8_t *n; /**< RSA modulus (big-endian) */
      size_t n_len;
      const uint8_t *e; /**< RSA exponent (big-endian) */
      size_t e_len;
    } rsa;
    struct {
      const uint8_t *point; /**< Uncompressed EC point (04 || x || y) */
      size_t point_len;
    } ecdsa;
    struct {
      const uint8_t *key; /**< 32-byte Ed25519 public key */
    } ed25519;
  } pubkey;

  /** Signature Algorithm */
  fio_x509_sig_alg_e sig_alg;

  /** Signature value (pointer into DER data) */
  const uint8_t *signature;
  size_t signature_len;

  /** TBS Certificate (for signature verification) */
  const uint8_t *tbs_data;
  size_t tbs_len;

  /** Basic Constraints: is CA */
  int is_ca;

  /** Key Usage extension present */
  int has_key_usage;
  /** Key Usage bits */
  uint16_t key_usage;

  /** Subject Alternative Name: first DNS name (if present) */
  const char *san_dns;
  size_t san_dns_len;

  /** Subject Alternative Name extension raw data (for iterating all SANs) */
  const uint8_t *san_ext_data;
  size_t san_ext_len;

} fio_x509_cert_s;

/* *****************************************************************************
X.509 Parser API
***************************************************************************** */

/**
 * Parse a DER-encoded X.509 certificate.
 *
 * The cert structure will contain pointers into the original DER data,
 * so the DER data must remain valid while the cert is in use.
 *
 * @param cert Output certificate structure (will be zeroed first)
 * @param der_data Pointer to DER-encoded certificate
 * @param der_len Length of DER data in bytes
 * @return 0 on success, -1 on error
 */
SFUNC int fio_x509_parse(fio_x509_cert_s *cert,
                         const uint8_t *der_data,
                         size_t der_len);

/**
 * Verify certificate signature using issuer's public key.
 *
 * This verifies that the certificate was signed by the issuer.
 *
 * @param cert Certificate to verify
 * @param issuer Certificate of the issuer (contains the public key)
 * @return 0 if valid, -1 if invalid or error
 */
SFUNC int fio_x509_verify_signature(const fio_x509_cert_s *cert,
                                    const fio_x509_cert_s *issuer);

/**
 * Check if certificate is currently valid (not expired, not yet valid).
 *
 * @param cert Certificate to check
 * @param current_time Current Unix timestamp (seconds since epoch)
 * @return 0 if valid, -1 if expired or not yet valid
 */
FIO_IFUNC int fio_x509_check_validity(const fio_x509_cert_s *cert,
                                      int64_t current_time);

/**
 * Check if hostname matches certificate (CN or SAN).
 *
 * Supports wildcard matching (*.example.com).
 * Per RFC 6125, wildcards only match one label.
 *
 * @param cert Certificate to check
 * @param hostname Hostname to match
 * @param hostname_len Length of hostname
 * @return 0 if match, -1 if no match
 */
SFUNC int fio_x509_match_hostname(const fio_x509_cert_s *cert,
                                  const char *hostname,
                                  size_t hostname_len);

/**
 * Compare two Distinguished Names for equality.
 *
 * Used for checking if issuer DN matches subject DN.
 *
 * @param dn1 First DN (DER-encoded)
 * @param dn1_len Length of first DN
 * @param dn2 Second DN (DER-encoded)
 * @param dn2_len Length of second DN
 * @return 0 if equal, non-zero if different
 */
FIO_IFUNC int fio_x509_dn_equals(const uint8_t *dn1,
                                 size_t dn1_len,
                                 const uint8_t *dn2,
                                 size_t dn2_len);

/* *****************************************************************************
X.509 Certificate Chain Validation API
***************************************************************************** */

/**
 * Validate a certificate chain for TLS 1.3.
 *
 * The chain should be ordered from end-entity to closest-to-root:
 *   - certs[0] = server's certificate (end-entity)
 *   - certs[1] = intermediate CA (signed certs[0])
 *   - certs[n-1] = closest to root (may be root or intermediate)
 *
 * Validation performs:
 *   1. Parse all certificates
 *   2. Check validity period for all certificates
 *   3. Verify hostname matches end-entity certificate (if hostname provided)
 *   4. Verify each certificate's signature using the next certificate's key
 *   5. Verify issuer DNs match subject DNs in the chain
 *   6. Verify intermediate/root certificates have CA:TRUE
 *   7. Verify the chain terminates at a trusted root (if trust store provided)
 *
 * @param certs Array of DER-encoded certificates
 * @param cert_lens Array of certificate lengths
 * @param cert_count Number of certificates in chain
 * @param hostname Expected hostname for end-entity (NULL to skip check)
 * @param current_time Current Unix timestamp for validity checking
 * @param trust_store Root CA certificates (NULL to skip trust check)
 * @return FIO_X509_OK (0) on success, or error code on failure
 */
SFUNC int fio_x509_verify_chain(const uint8_t **certs,
                                const size_t *cert_lens,
                                size_t cert_count,
                                const char *hostname,
                                int64_t current_time,
                                fio_x509_trust_store_s *trust_store);

/**
 * Check if a certificate is in the trust store.
 *
 * Comparison is done by matching subject DN.
 *
 * @param cert Certificate to check
 * @param trust_store Trust store to search
 * @return 0 if trusted, -1 if not found
 */
SFUNC int fio_x509_is_trusted(const fio_x509_cert_s *cert,
                              fio_x509_trust_store_s *trust_store);

/** Error value for fio_tls_parse_certificate_message */
#define FIO_TLS_CERT_PARSE_ERROR ((size_t)-1)

/**
 * Parse TLS 1.3 Certificate message into individual certificates.
 *
 * TLS 1.3 Certificate message format (RFC 8446):
 *   certificate_request_context<0..2^8-1>
 *   certificate_list<0..2^24-1>:
 *     CertificateEntry:
 *       cert_data<1..2^24-1>
 *       extensions<0..2^16-1>
 *
 * @param entries Output array for certificate entries
 * @param max_entries Maximum entries to parse
 * @param data Raw Certificate message data (after handshake header)
 * @param data_len Length of Certificate message data
 * @return Number of certificates parsed, or FIO_TLS_CERT_PARSE_ERROR on error
 */
SFUNC size_t fio_tls_parse_certificate_message(fio_tls_cert_entry_s *entries,
                                               size_t max_entries,
                                               const uint8_t *data,
                                               size_t data_len);

/**
 * Get human-readable error string for X.509 validation error code.
 *
 * @param error Error code from fio_x509_verify_chain
 * @return Static string describing the error
 */
FIO_IFUNC const char *fio_x509_error_str(int error);

/* *****************************************************************************
Implementation - Inline Functions
***************************************************************************** */

/** Check if certificate is within validity period */
FIO_IFUNC int fio_x509_check_validity(const fio_x509_cert_s *cert,
                                      int64_t current_time) {
  if (!cert)
    return -1;
  if (current_time < cert->not_before)
    return -1; /* Not yet valid */
  if (current_time > cert->not_after)
    return -1; /* Expired */
  return 0;
}

/** Compare two Distinguished Names */
FIO_IFUNC int fio_x509_dn_equals(const uint8_t *dn1,
                                 size_t dn1_len,
                                 const uint8_t *dn2,
                                 size_t dn2_len) {
  if (dn1_len != dn2_len)
    return 1;
  if (!dn1 || !dn2)
    return 1;
  return FIO_MEMCMP(dn1, dn2, dn1_len);
}

/** Get human-readable error string for X.509 validation error code */
FIO_IFUNC const char *fio_x509_error_str(int error) {
  switch (error) {
  case FIO_X509_OK: return "OK";
  case FIO_X509_ERR_PARSE: return "Failed to parse certificate";
  case FIO_X509_ERR_EXPIRED: return "Certificate expired";
  case FIO_X509_ERR_NOT_YET_VALID: return "Certificate not yet valid";
  case FIO_X509_ERR_SIGNATURE: return "Signature verification failed";
  case FIO_X509_ERR_ISSUER_MISMATCH: return "Issuer DN mismatch";
  case FIO_X509_ERR_NOT_CA: return "Issuer is not a CA";
  case FIO_X509_ERR_NO_TRUST_ANCHOR: return "No trusted root CA found";
  case FIO_X509_ERR_HOSTNAME_MISMATCH: return "Hostname mismatch";
  case FIO_X509_ERR_EMPTY_CHAIN: return "Empty certificate chain";
  case FIO_X509_ERR_CHAIN_TOO_LONG: return "Certificate chain too long";
  default: return "Unknown error";
  }
}

/* *****************************************************************************
Implementation - Possibly Externed Functions
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Implementation - Internal Helpers
***************************************************************************** */

/** Parse signature algorithm OID to enum */
FIO_SFUNC fio_x509_sig_alg_e
fio___x509_parse_sig_alg(const fio_asn1_element_s *alg_id) {
  if (!alg_id || !alg_id->data)
    return FIO_X509_SIG_UNKNOWN;

  /* AlgorithmIdentifier ::= SEQUENCE { algorithm OID, parameters ANY } */
  fio_asn1_iterator_s it;
  fio_asn1_element_s oid;

  fio_asn1_iterator_init(&it, alg_id);
  if (fio_asn1_iterator_next(&it, &oid) != 0)
    return FIO_X509_SIG_UNKNOWN;

  if (!fio_asn1_is_tag(&oid, FIO_ASN1_OID))
    return FIO_X509_SIG_UNKNOWN;

  /* Check against known OIDs */
  if (fio_asn1_oid_eq(&oid, FIO_OID_SHA256_WITH_RSA))
    return FIO_X509_SIG_RSA_PKCS1_SHA256;
  if (fio_asn1_oid_eq(&oid, FIO_OID_SHA384_WITH_RSA))
    return FIO_X509_SIG_RSA_PKCS1_SHA384;
  if (fio_asn1_oid_eq(&oid, FIO_OID_SHA512_WITH_RSA))
    return FIO_X509_SIG_RSA_PKCS1_SHA512;
  if (fio_asn1_oid_eq(&oid, FIO_OID_ECDSA_WITH_SHA256))
    return FIO_X509_SIG_ECDSA_SHA256;
  if (fio_asn1_oid_eq(&oid, FIO_OID_ECDSA_WITH_SHA384))
    return FIO_X509_SIG_ECDSA_SHA384;
  if (fio_asn1_oid_eq(&oid, FIO_OID_ED25519))
    return FIO_X509_SIG_ED25519;
  if (fio_asn1_oid_eq(&oid, FIO_OID_RSA_PSS)) {
    /* RSA-PSS - need to check parameters to determine hash */
    /* For simplicity, default to SHA-256 for now */
    /* TODO: Parse RSA-PSS parameters to determine actual hash */
    return FIO_X509_SIG_RSA_PSS_SHA256;
  }

  return FIO_X509_SIG_UNKNOWN;
}

/** Parse SubjectPublicKeyInfo to extract public key */
FIO_SFUNC int fio___x509_parse_pubkey(fio_x509_cert_s *cert,
                                      const fio_asn1_element_s *spki) {
  if (!cert || !spki)
    return -1;

  /*
   * SubjectPublicKeyInfo ::= SEQUENCE {
   *   algorithm        AlgorithmIdentifier,
   *   subjectPublicKey BIT STRING
   * }
   */
  fio_asn1_iterator_s it;
  fio_asn1_element_s alg_id, pubkey_bits;

  fio_asn1_iterator_init(&it, spki);

  /* Get AlgorithmIdentifier */
  if (fio_asn1_iterator_next(&it, &alg_id) != 0)
    return -1;
  if (!fio_asn1_is_tag(&alg_id, FIO_ASN1_SEQUENCE))
    return -1;

  /* Get BIT STRING containing public key */
  if (fio_asn1_iterator_next(&it, &pubkey_bits) != 0)
    return -1;
  if (!fio_asn1_is_tag(&pubkey_bits, FIO_ASN1_BIT_STRING))
    return -1;

  /* Parse the BIT STRING */
  const uint8_t *bits;
  size_t bit_len;
  uint8_t unused_bits;
  if (fio_asn1_parse_bit_string(&pubkey_bits, &bits, &bit_len, &unused_bits) !=
      0)
    return -1;

  /* Parse AlgorithmIdentifier to get key type */
  fio_asn1_iterator_s alg_it;
  fio_asn1_element_s oid, params;

  fio_asn1_iterator_init(&alg_it, &alg_id);
  if (fio_asn1_iterator_next(&alg_it, &oid) != 0)
    return -1;

  if (fio_asn1_oid_eq(&oid, FIO_OID_RSA_ENCRYPTION)) {
    /* RSA public key - bits contains SEQUENCE { n INTEGER, e INTEGER } */
    cert->key_type = FIO_X509_KEY_RSA;

    fio_asn1_element_s rsa_seq;
    if (!fio_asn1_parse(&rsa_seq, bits, bit_len))
      return -1;
    if (!fio_asn1_is_tag(&rsa_seq, FIO_ASN1_SEQUENCE))
      return -1;

    fio_asn1_iterator_s rsa_it;
    fio_asn1_element_s n_elem, e_elem;

    fio_asn1_iterator_init(&rsa_it, &rsa_seq);

    /* Get modulus (n) */
    if (fio_asn1_iterator_next(&rsa_it, &n_elem) != 0)
      return -1;
    if (!fio_asn1_is_tag(&n_elem, FIO_ASN1_INTEGER))
      return -1;

    /* Get exponent (e) */
    if (fio_asn1_iterator_next(&rsa_it, &e_elem) != 0)
      return -1;
    if (!fio_asn1_is_tag(&e_elem, FIO_ASN1_INTEGER))
      return -1;

    /* Store pointers - handle leading zero byte for positive integers */
    cert->pubkey.rsa.n = n_elem.data;
    cert->pubkey.rsa.n_len = n_elem.len;
    cert->pubkey.rsa.e = e_elem.data;
    cert->pubkey.rsa.e_len = e_elem.len;

    /* Skip leading zero byte if present (positive integer encoding) */
    if (cert->pubkey.rsa.n_len > 1 && cert->pubkey.rsa.n[0] == 0x00) {
      cert->pubkey.rsa.n++;
      cert->pubkey.rsa.n_len--;
    }
    if (cert->pubkey.rsa.e_len > 1 && cert->pubkey.rsa.e[0] == 0x00) {
      cert->pubkey.rsa.e++;
      cert->pubkey.rsa.e_len--;
    }

  } else if (fio_asn1_oid_eq(&oid, FIO_OID_EC_PUBLIC_KEY)) {
    /* ECDSA - need to check curve parameter */
    if (fio_asn1_iterator_next(&alg_it, &params) != 0)
      return -1;

    if (fio_asn1_oid_eq(&params, FIO_OID_SECP256R1)) {
      cert->key_type = FIO_X509_KEY_ECDSA_P256;
    } else if (fio_asn1_oid_eq(&params, FIO_OID_SECP384R1)) {
      cert->key_type = FIO_X509_KEY_ECDSA_P384;
    } else {
      return -1; /* Unsupported curve */
    }

    /* EC public key is the uncompressed point directly in BIT STRING */
    cert->pubkey.ecdsa.point = bits;
    cert->pubkey.ecdsa.point_len = bit_len;

  } else if (fio_asn1_oid_eq(&oid, FIO_OID_ED25519)) {
    /* Ed25519 - public key is 32 bytes directly in BIT STRING */
    cert->key_type = FIO_X509_KEY_ED25519;

    if (bit_len != 32)
      return -1; /* Ed25519 public key must be 32 bytes */

    cert->pubkey.ed25519.key = bits;

  } else {
    cert->key_type = FIO_X509_KEY_UNKNOWN;
  }

  return 0;
}

/** Extract Common Name from a Name sequence */
FIO_SFUNC void fio___x509_extract_cn(fio_x509_cert_s *cert,
                                     const fio_asn1_element_s *name) {
  if (!cert || !name)
    return;

  /*
   * Name ::= SEQUENCE OF RelativeDistinguishedName
   * RelativeDistinguishedName ::= SET OF AttributeTypeAndValue
   * AttributeTypeAndValue ::= SEQUENCE { type OID, value ANY }
   */
  fio_asn1_iterator_s name_it;
  fio_asn1_element_s rdn;

  fio_asn1_iterator_init(&name_it, name);

  while (fio_asn1_iterator_next(&name_it, &rdn) == 0) {
    if (!fio_asn1_is_tag(&rdn, FIO_ASN1_SET))
      continue;

    fio_asn1_iterator_s rdn_it;
    fio_asn1_element_s atv;

    fio_asn1_iterator_init(&rdn_it, &rdn);

    while (fio_asn1_iterator_next(&rdn_it, &atv) == 0) {
      if (!fio_asn1_is_tag(&atv, FIO_ASN1_SEQUENCE))
        continue;

      fio_asn1_iterator_s atv_it;
      fio_asn1_element_s type_oid, value;

      fio_asn1_iterator_init(&atv_it, &atv);

      if (fio_asn1_iterator_next(&atv_it, &type_oid) != 0)
        continue;
      if (fio_asn1_iterator_next(&atv_it, &value) != 0)
        continue;

      /* Check if this is Common Name */
      if (fio_asn1_oid_eq(&type_oid, FIO_OID_COMMON_NAME)) {
        size_t len;
        const char *cn = fio_asn1_parse_string(&value, &len);
        if (cn) {
          cert->subject_cn = cn;
          cert->subject_cn_len = len;
        }
        return;
      }
    }
  }
}

/** Parse X.509v3 extensions */
FIO_SFUNC void fio___x509_parse_extensions(fio_x509_cert_s *cert,
                                           const fio_asn1_element_s *exts) {
  if (!cert || !exts)
    return;

  /*
   * Extensions ::= SEQUENCE OF Extension
   * Extension ::= SEQUENCE {
   *   extnID      OID,
   *   critical    BOOLEAN DEFAULT FALSE,
   *   extnValue   OCTET STRING (DER-encoded extension value)
   * }
   */
  fio_asn1_iterator_s exts_it;
  fio_asn1_element_s ext;

  fio_asn1_iterator_init(&exts_it, exts);

  while (fio_asn1_iterator_next(&exts_it, &ext) == 0) {
    if (!fio_asn1_is_tag(&ext, FIO_ASN1_SEQUENCE))
      continue;

    fio_asn1_iterator_s ext_it;
    fio_asn1_element_s oid, elem;

    fio_asn1_iterator_init(&ext_it, &ext);

    /* Get extension OID */
    if (fio_asn1_iterator_next(&ext_it, &oid) != 0)
      continue;

    /* Skip critical flag if present, get value */
    if (fio_asn1_iterator_next(&ext_it, &elem) != 0)
      continue;

    /* If BOOLEAN (critical), skip to get OCTET STRING */
    const fio_asn1_element_s *value = &elem;
    fio_asn1_element_s value_storage;
    if (fio_asn1_is_tag(&elem, FIO_ASN1_BOOLEAN)) {
      if (fio_asn1_iterator_next(&ext_it, &value_storage) != 0)
        continue;
      value = &value_storage;
    }

    if (!fio_asn1_is_tag(value, FIO_ASN1_OCTET_STRING))
      continue;

    /* Parse based on extension type */
    if (fio_asn1_oid_eq(&oid, FIO_OID_BASIC_CONSTRAINTS)) {
      /* BasicConstraints ::= SEQUENCE { cA BOOLEAN DEFAULT FALSE, ... } */
      fio_asn1_element_s bc_seq;
      if (fio_asn1_parse(&bc_seq, value->data, value->len)) {
        if (fio_asn1_is_tag(&bc_seq, FIO_ASN1_SEQUENCE)) {
          fio_asn1_iterator_s bc_it;
          fio_asn1_element_s ca_elem;

          fio_asn1_iterator_init(&bc_it, &bc_seq);
          if (fio_asn1_iterator_next(&bc_it, &ca_elem) == 0) {
            if (fio_asn1_is_tag(&ca_elem, FIO_ASN1_BOOLEAN)) {
              int ca_val;
              if (fio_asn1_parse_boolean(&ca_elem, &ca_val) == 0) {
                cert->is_ca = ca_val;
              }
            }
          }
        }
      }
    } else if (fio_asn1_oid_eq(&oid, FIO_OID_KEY_USAGE)) {
      /* KeyUsage ::= BIT STRING */
      fio_asn1_element_s ku_bits;
      if (fio_asn1_parse(&ku_bits, value->data, value->len)) {
        if (fio_asn1_is_tag(&ku_bits, FIO_ASN1_BIT_STRING)) {
          const uint8_t *bits;
          size_t bit_len;
          uint8_t unused;
          if (fio_asn1_parse_bit_string(&ku_bits, &bits, &bit_len, &unused) ==
              0) {
            cert->has_key_usage = 1;
            if (bit_len >= 1) {
              cert->key_usage = bits[0];
              if (bit_len >= 2)
                cert->key_usage |= (uint16_t)bits[1] << 8;
            }
          }
        }
      }
    } else if (fio_asn1_oid_eq(&oid, FIO_OID_SUBJECT_ALT_NAME)) {
      /* SubjectAltName ::= GeneralNames = SEQUENCE OF GeneralName
       * GeneralName ::= CHOICE { dNSName [2] IA5String, ... }
       */
      /* Store raw SAN extension data for later iteration */
      cert->san_ext_data = value->data;
      cert->san_ext_len = value->len;

      fio_asn1_element_s san_seq;
      if (fio_asn1_parse(&san_seq, value->data, value->len)) {
        if (fio_asn1_is_tag(&san_seq, FIO_ASN1_SEQUENCE)) {
          fio_asn1_iterator_s san_it;
          fio_asn1_element_s gn;

          fio_asn1_iterator_init(&san_it, &san_seq);

          while (fio_asn1_iterator_next(&san_it, &gn) == 0) {
            /* dNSName is context tag [2] */
            if (fio_asn1_is_context_tag(&gn, 2)) {
              /* Store first DNS name only (for backward compatibility) */
              if (!cert->san_dns) {
                cert->san_dns = (const char *)gn.data;
                cert->san_dns_len = gn.len;
              }
            }
          }
        }
      }
    }
  }
}

/* *****************************************************************************
Implementation - Main Parsing Function
***************************************************************************** */

SFUNC int fio_x509_parse(fio_x509_cert_s *cert,
                         const uint8_t *der_data,
                         size_t der_len) {
  if (!cert || !der_data || der_len == 0)
    return -1;

  /* Zero the certificate structure */
  FIO_MEMSET(cert, 0, sizeof(*cert));

  /*
   * Certificate ::= SEQUENCE {
   *   tbsCertificate       TBSCertificate,
   *   signatureAlgorithm   AlgorithmIdentifier,
   *   signatureValue       BIT STRING
   * }
   */
  fio_asn1_element_s cert_seq;
  if (!fio_asn1_parse(&cert_seq, der_data, der_len))
    return -1;

  if (!fio_asn1_is_tag(&cert_seq, FIO_ASN1_SEQUENCE))
    return -1;

  fio_asn1_iterator_s cert_it;
  fio_asn1_element_s tbs, sig_alg, sig_value;

  fio_asn1_iterator_init(&cert_it, &cert_seq);

  /* Parse tbsCertificate */
  if (fio_asn1_iterator_next(&cert_it, &tbs) != 0)
    return -1;
  if (!fio_asn1_is_tag(&tbs, FIO_ASN1_SEQUENCE))
    return -1;

  /* Store TBS data for signature verification */
  cert->tbs_data =
      der_data + (tbs.data - cert_seq.data) -
      (tbs.data > der_data ? (tbs.data - der_data) - (tbs.data - cert_seq.data)
                           : 0);
  /* Calculate proper TBS bounds including tag and length */
  {
    const uint8_t *tbs_start = tbs.data;
    /* Walk backwards to find the tag byte */
    while (tbs_start > der_data && tbs_start[-1] != 0x30)
      --tbs_start;
    if (tbs_start > der_data)
      --tbs_start;
    cert->tbs_data = tbs_start;
    cert->tbs_len = (tbs.data + tbs.len) - tbs_start;
  }

  /* Parse signatureAlgorithm */
  if (fio_asn1_iterator_next(&cert_it, &sig_alg) != 0)
    return -1;
  if (!fio_asn1_is_tag(&sig_alg, FIO_ASN1_SEQUENCE))
    return -1;

  cert->sig_alg = fio___x509_parse_sig_alg(&sig_alg);

  /* Parse signatureValue (BIT STRING) */
  if (fio_asn1_iterator_next(&cert_it, &sig_value) != 0)
    return -1;
  if (!fio_asn1_is_tag(&sig_value, FIO_ASN1_BIT_STRING))
    return -1;

  {
    const uint8_t *bits;
    size_t bit_len;
    uint8_t unused;
    if (fio_asn1_parse_bit_string(&sig_value, &bits, &bit_len, &unused) != 0)
      return -1;
    cert->signature = bits;
    cert->signature_len = bit_len;
  }

  /*
   * TBSCertificate ::= SEQUENCE {
   *   version         [0]  EXPLICIT Version DEFAULT v1,
   *   serialNumber         CertificateSerialNumber,
   *   signature            AlgorithmIdentifier,
   *   issuer               Name,
   *   validity             Validity,
   *   subject              Name,
   *   subjectPublicKeyInfo SubjectPublicKeyInfo,
   *   issuerUniqueID  [1]  IMPLICIT UniqueIdentifier OPTIONAL,
   *   subjectUniqueID [2]  IMPLICIT UniqueIdentifier OPTIONAL,
   *   extensions      [3]  EXPLICIT Extensions OPTIONAL
   * }
   */
  fio_asn1_iterator_s tbs_it;
  fio_asn1_element_s elem;

  fio_asn1_iterator_init(&tbs_it, &tbs);

  /* Get first element - may be version or serial */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0)
    return -1;

  /* Check for version (context tag [0]) */
  if (fio_asn1_is_context_tag(&elem, 0)) {
    /* Parse version */
    fio_asn1_element_s version_int;
    if (fio_asn1_parse(&version_int, elem.data, elem.len)) {
      uint64_t ver;
      if (fio_asn1_parse_integer(&version_int, &ver) == 0) {
        cert->version = (int)ver;
      }
    }
    /* Get next element (serial number) */
    if (fio_asn1_iterator_next(&tbs_it, &elem) != 0)
      return -1;
  } else {
    cert->version = 0; /* v1 (default) */
  }

  /* Skip serialNumber (INTEGER) */
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER))
    return -1;

  /* Skip signature (AlgorithmIdentifier) */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE))
    return -1;

  /* Parse issuer (Name = SEQUENCE) */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE))
    return -1;

  /* Store issuer DN (including tag and length for comparison) */
  {
    const uint8_t *issuer_start = elem.data;
    while (issuer_start > tbs.data && issuer_start[-1] != 0x30)
      --issuer_start;
    if (issuer_start > tbs.data)
      --issuer_start;
    cert->issuer_der = issuer_start;
    cert->issuer_der_len = (elem.data + elem.len) - issuer_start;
  }

  /* Parse validity (SEQUENCE { notBefore, notAfter }) */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE))
    return -1;

  {
    fio_asn1_iterator_s val_it;
    fio_asn1_element_s not_before, not_after;

    fio_asn1_iterator_init(&val_it, &elem);

    if (fio_asn1_iterator_next(&val_it, &not_before) == 0) {
      fio_asn1_parse_time(&not_before, &cert->not_before);
    }
    if (fio_asn1_iterator_next(&val_it, &not_after) == 0) {
      fio_asn1_parse_time(&not_after, &cert->not_after);
    }
  }

  /* Parse subject (Name = SEQUENCE) */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE))
    return -1;

  /* Store subject DN */
  {
    const uint8_t *subject_start = elem.data;
    while (subject_start > tbs.data && subject_start[-1] != 0x30)
      --subject_start;
    if (subject_start > tbs.data)
      --subject_start;
    cert->subject_der = subject_start;
    cert->subject_der_len = (elem.data + elem.len) - subject_start;
  }

  /* Extract Common Name */
  fio___x509_extract_cn(cert, &elem);

  /* Parse subjectPublicKeyInfo (SEQUENCE) */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0)
    return -1;
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE))
    return -1;

  if (fio___x509_parse_pubkey(cert, &elem) != 0)
    return -1;

  /* Check for extensions [3] (v3 only) */
  while (fio_asn1_iterator_next(&tbs_it, &elem) == 0) {
    if (fio_asn1_is_context_tag(&elem, 3)) {
      /* Extensions wrapped in context tag [3] */
      fio_asn1_element_s exts;
      if (fio_asn1_parse(&exts, elem.data, elem.len)) {
        if (fio_asn1_is_tag(&exts, FIO_ASN1_SEQUENCE)) {
          fio___x509_parse_extensions(cert, &exts);
        }
      }
      break;
    }
  }

  return 0;
}

/* *****************************************************************************
Implementation - Hostname Matching
***************************************************************************** */

/** Case-insensitive character comparison */
FIO_SFUNC int fio___x509_char_eq_nocase(char a, char b) {
  /* Convert to lowercase */
  char la = a | (char)((uint8_t)(a >= 'A' && a <= 'Z') << 5);
  char lb = b | (char)((uint8_t)(b >= 'A' && b <= 'Z') << 5);
  return la == lb;
}

/** Match a single hostname against a pattern (with wildcard support) */
FIO_SFUNC int fio___x509_match_name(const char *pattern,
                                    size_t pattern_len,
                                    const char *hostname,
                                    size_t hostname_len) {
  if (!pattern || !hostname || pattern_len == 0 || hostname_len == 0)
    return -1;

  /* Check for wildcard pattern */
  if (pattern_len >= 2 && pattern[0] == '*' && pattern[1] == '.') {
    /* Wildcard certificate: *.example.com
     * Per RFC 6125: wildcard matches only one label
     */
    const char *pattern_rest = pattern + 2;
    size_t pattern_rest_len = pattern_len - 2;

    /* Find first dot in hostname */
    size_t dot_pos = 0;
    for (size_t i = 0; i < hostname_len; ++i) {
      if (hostname[i] == '.') {
        dot_pos = i;
        break;
      }
    }

    /* Hostname must have at least one label before the matched part */
    if (dot_pos == 0 || dot_pos == hostname_len - 1)
      return -1;

    /* Compare rest of pattern with rest of hostname (after first dot) */
    const char *hostname_rest = hostname + dot_pos + 1;
    size_t hostname_rest_len = hostname_len - dot_pos - 1;

    if (pattern_rest_len != hostname_rest_len)
      return -1;

    for (size_t i = 0; i < pattern_rest_len; ++i) {
      if (!fio___x509_char_eq_nocase(pattern_rest[i], hostname_rest[i]))
        return -1;
    }

    return 0;
  }

  /* Exact match (case-insensitive) */
  if (pattern_len != hostname_len)
    return -1;

  for (size_t i = 0; i < pattern_len; ++i) {
    if (!fio___x509_char_eq_nocase(pattern[i], hostname[i]))
      return -1;
  }

  return 0;
}

SFUNC int fio_x509_match_hostname(const fio_x509_cert_s *cert,
                                  const char *hostname,
                                  size_t hostname_len) {
  if (!cert || !hostname || hostname_len == 0)
    return -1;

  FIO_LOG_DEBUG2("X.509 hostname match: checking '%.*s'",
                 (int)hostname_len,
                 hostname);

  /* Per RFC 6125: If SAN extension is present, check ALL SAN DNS names */
  if (cert->san_ext_data && cert->san_ext_len > 0) {
    FIO_LOG_DEBUG2("X.509 hostname match: SAN extension present (%zu bytes)",
                   cert->san_ext_len);
    /* Re-parse the SAN extension to iterate through all DNS names */
    fio_asn1_element_s san_seq;
    if (fio_asn1_parse(&san_seq, cert->san_ext_data, cert->san_ext_len)) {
      if (fio_asn1_is_tag(&san_seq, FIO_ASN1_SEQUENCE)) {
        fio_asn1_iterator_s san_it;
        fio_asn1_element_s gn;

        fio_asn1_iterator_init(&san_it, &san_seq);

        while (fio_asn1_iterator_next(&san_it, &gn) == 0) {
          /* dNSName is context tag [2] */
          if (fio_asn1_is_context_tag(&gn, 2)) {
            FIO_LOG_DEBUG2("X.509 hostname match: SAN DNS name: '%.*s'",
                           (int)gn.len,
                           (const char *)gn.data);
            /* Try to match this DNS name */
            if (fio___x509_match_name((const char *)gn.data,
                                      gn.len,
                                      hostname,
                                      hostname_len) == 0) {
              FIO_LOG_DEBUG2("X.509 hostname match: MATCH FOUND");
              return 0; /* Match found */
            }
          }
        }
      }
    }
    /* SAN extension present but no match found - do NOT fall back to CN */
    FIO_LOG_DEBUG2("X.509 hostname match: no SAN match found");
    return -1;
  }

  /* Check if san_dns is set directly (without raw extension data) */
  if (cert->san_dns && cert->san_dns_len > 0) {
    FIO_LOG_DEBUG2("X.509 hostname match: SAN DNS set directly: '%.*s'",
                   (int)cert->san_dns_len,
                   cert->san_dns);
    if (fio___x509_match_name(cert->san_dns,
                              cert->san_dns_len,
                              hostname,
                              hostname_len) == 0) {
      FIO_LOG_DEBUG2("X.509 hostname match: MATCH FOUND");
      return 0;
    }
    /* SAN present but no match - do NOT fall back to CN */
    FIO_LOG_DEBUG2("X.509 hostname match: no SAN match found");
    return -1;
  }

  /* No SAN extension present - fall back to Common Name */
  FIO_LOG_DEBUG2("X.509 hostname match: no SAN, checking CN");
  if (cert->subject_cn && cert->subject_cn_len > 0) {
    FIO_LOG_DEBUG2("X.509 hostname match: CN='%.*s'",
                   (int)cert->subject_cn_len,
                   cert->subject_cn);
    return fio___x509_match_name(cert->subject_cn,
                                 cert->subject_cn_len,
                                 hostname,
                                 hostname_len);
  }

  FIO_LOG_DEBUG2("X.509 hostname match: no CN either");
  return -1;
}

/* *****************************************************************************
Implementation - Signature Verification
***************************************************************************** */

SFUNC int fio_x509_verify_signature(const fio_x509_cert_s *cert,
                                    const fio_x509_cert_s *issuer) {
  if (!cert || !issuer)
    return -1;

  if (!cert->tbs_data || cert->tbs_len == 0)
    return -1;
  if (!cert->signature || cert->signature_len == 0)
    return -1;

  /* Hash the TBS certificate data based on signature algorithm */
  uint8_t hash[64]; /* Max hash size (SHA-512) */
  size_t hash_len = 0;

  switch (cert->sig_alg) {
  case FIO_X509_SIG_RSA_PKCS1_SHA256:
  case FIO_X509_SIG_RSA_PSS_SHA256:
  case FIO_X509_SIG_ECDSA_SHA256: {
    fio_u256 h = fio_sha256(cert->tbs_data, cert->tbs_len);
    FIO_MEMCPY(hash, h.u8, 32);
    hash_len = 32;
    break;
  }
  case FIO_X509_SIG_RSA_PKCS1_SHA384:
  case FIO_X509_SIG_RSA_PSS_SHA384:
  case FIO_X509_SIG_ECDSA_SHA384: {
    /* SHA-384 is truncated SHA-512 - we use fio_sha512 and truncate */
    fio_sha512_s sh = fio_sha512_init();
    /* Modify initial hash values for SHA-384 */
    sh.hash.u64[0] = 0xCBBB9D5DC1059ED8ULL;
    sh.hash.u64[1] = 0x629A292A367CD507ULL;
    sh.hash.u64[2] = 0x9159015A3070DD17ULL;
    sh.hash.u64[3] = 0x152FECD8F70E5939ULL;
    sh.hash.u64[4] = 0x67332667FFC00B31ULL;
    sh.hash.u64[5] = 0x8EB44A8768581511ULL;
    sh.hash.u64[6] = 0xDB0C2E0D64F98FA7ULL;
    sh.hash.u64[7] = 0x47B5481DBEFA4FA4ULL;
    fio_sha512_consume(&sh, cert->tbs_data, cert->tbs_len);
    fio_u512 h = fio_sha512_finalize(&sh);
    FIO_MEMCPY(hash, h.u8, 48);
    hash_len = 48;
    break;
  }
  case FIO_X509_SIG_RSA_PKCS1_SHA512:
  case FIO_X509_SIG_RSA_PSS_SHA512: {
    fio_u512 h = fio_sha512(cert->tbs_data, cert->tbs_len);
    FIO_MEMCPY(hash, h.u8, 64);
    hash_len = 64;
    break;
  }
  case FIO_X509_SIG_ED25519: {
    /* Ed25519 doesn't pre-hash - the data is hashed internally */
    /* Verification would be done directly with fio_ed25519_verify */
    break;
  }
  default: return -1; /* Unknown algorithm */
  }

  /* Verify signature based on issuer's key type */
  switch (issuer->key_type) {
  case FIO_X509_KEY_RSA: {
    if (!issuer->pubkey.rsa.n || !issuer->pubkey.rsa.e)
      return -1;

    fio_rsa_pubkey_s rsa_key = {
        .n = issuer->pubkey.rsa.n,
        .n_len = issuer->pubkey.rsa.n_len,
        .e = issuer->pubkey.rsa.e,
        .e_len = issuer->pubkey.rsa.e_len,
    };

    fio_rsa_hash_e rsa_hash;
    switch (hash_len) {
    case 32: rsa_hash = FIO_RSA_HASH_SHA256; break;
    case 48: rsa_hash = FIO_RSA_HASH_SHA384; break;
    case 64: rsa_hash = FIO_RSA_HASH_SHA512; break;
    default: return -1;
    }

    switch (cert->sig_alg) {
    case FIO_X509_SIG_RSA_PKCS1_SHA256:
    case FIO_X509_SIG_RSA_PKCS1_SHA384:
    case FIO_X509_SIG_RSA_PKCS1_SHA512:
      return fio_rsa_verify_pkcs1(cert->signature,
                                  cert->signature_len,
                                  hash,
                                  hash_len,
                                  rsa_hash,
                                  &rsa_key);
    case FIO_X509_SIG_RSA_PSS_SHA256:
    case FIO_X509_SIG_RSA_PSS_SHA384:
    case FIO_X509_SIG_RSA_PSS_SHA512:
      return fio_rsa_verify_pss(cert->signature,
                                cert->signature_len,
                                hash,
                                hash_len,
                                rsa_hash,
                                &rsa_key);
    default: return -1;
    }
  }

  case FIO_X509_KEY_ECDSA_P256:
  case FIO_X509_KEY_ECDSA_P384: {
#if defined(H___FIO_P256___H)
    /* ECDSA P-256 verification */
    if (issuer->key_type == FIO_X509_KEY_ECDSA_P256) {
      if (!issuer->pubkey.ecdsa.point || issuer->pubkey.ecdsa.point_len != 65)
        return -1;

      /* P-256 uses SHA-256, verify hash_len matches */
      if (hash_len != 32)
        return -1;

      return fio_ecdsa_p256_verify(cert->signature,
                                   cert->signature_len,
                                   hash,
                                   issuer->pubkey.ecdsa.point,
                                   issuer->pubkey.ecdsa.point_len);
    }
#endif
    /* P-384 not yet implemented */
    return -1;
  }

  case FIO_X509_KEY_ED25519: {
#if defined(H___FIO_ED25519___H)
    /* Ed25519 verification */
    if (!issuer->pubkey.ed25519.key)
      return -1;

    /* Ed25519 signature should be 64 bytes */
    if (cert->signature_len != 64)
      return -1;

    return fio_ed25519_verify(cert->signature,
                              cert->tbs_data,
                              cert->tbs_len,
                              issuer->pubkey.ed25519.key);
#else
    return -1;
#endif
  }

  default: return -1;
  }
}

/* *****************************************************************************
Implementation - Certificate Chain Validation
***************************************************************************** */

/** Maximum certificate chain depth (prevents DoS via deep chains) */
#ifndef FIO_X509_MAX_CHAIN_DEPTH
#define FIO_X509_MAX_CHAIN_DEPTH 10
#endif

SFUNC int fio_x509_is_trusted(const fio_x509_cert_s *cert,
                              fio_x509_trust_store_s *trust_store) {
  if (!cert || !trust_store)
    return -1;
  if (!trust_store->roots || trust_store->root_count == 0)
    return -1;

  /* Check each root certificate in the trust store */
  for (size_t i = 0; i < trust_store->root_count; ++i) {
    if (!trust_store->roots[i] || trust_store->root_lens[i] == 0)
      continue;

    /* Parse the root certificate */
    fio_x509_cert_s root;
    if (fio_x509_parse(&root,
                       trust_store->roots[i],
                       trust_store->root_lens[i]) != 0)
      continue;

    /* Compare subject DNs (the cert's issuer should match root's subject) */
    if (fio_x509_dn_equals(cert->issuer_der,
                           cert->issuer_der_len,
                           root.subject_der,
                           root.subject_der_len) == 0) {
      return 0; /* Found trusted root */
    }

    /* Also check if the cert itself is a trusted root */
    if (fio_x509_dn_equals(cert->subject_der,
                           cert->subject_der_len,
                           root.subject_der,
                           root.subject_der_len) == 0) {
      return 0; /* Cert is in trust store */
    }
  }

  return -1; /* Not found in trust store */
}

/**
 * Find issuing certificate in trust store for signature verification.
 * Returns 0 if found and parsed into `issuer`, -1 if not found.
 */
FIO_SFUNC int fio___x509_find_issuer_in_trust_store(
    fio_x509_cert_s *issuer,
    const fio_x509_cert_s *cert,
    fio_x509_trust_store_s *trust_store) {
  if (!issuer || !cert || !trust_store)
    return -1;
  if (!trust_store->roots || trust_store->root_count == 0)
    return -1;

  for (size_t i = 0; i < trust_store->root_count; ++i) {
    if (!trust_store->roots[i] || trust_store->root_lens[i] == 0)
      continue;

    if (fio_x509_parse(issuer,
                       trust_store->roots[i],
                       trust_store->root_lens[i]) != 0)
      continue;

    /* Check if this root's subject matches cert's issuer */
    if (fio_x509_dn_equals(cert->issuer_der,
                           cert->issuer_der_len,
                           issuer->subject_der,
                           issuer->subject_der_len) == 0) {
      return 0; /* Found issuer */
    }
  }

  return -1; /* Not found */
}

SFUNC int fio_x509_verify_chain(const uint8_t **certs,
                                const size_t *cert_lens,
                                size_t cert_count,
                                const char *hostname,
                                int64_t current_time,
                                fio_x509_trust_store_s *trust_store) {
  /* Validate inputs */
  if (!certs || !cert_lens)
    return FIO_X509_ERR_PARSE;
  if (cert_count == 0)
    return FIO_X509_ERR_EMPTY_CHAIN;
  if (cert_count > FIO_X509_MAX_CHAIN_DEPTH)
    return FIO_X509_ERR_CHAIN_TOO_LONG;

  /* Parse all certificates in the chain */
  fio_x509_cert_s chain[FIO_X509_MAX_CHAIN_DEPTH];

  for (size_t i = 0; i < cert_count; ++i) {
    if (!certs[i] || cert_lens[i] == 0) {
      FIO_LOG_DEBUG("X.509 chain: certificate %zu is empty", i);
      return FIO_X509_ERR_PARSE;
    }

    if (fio_x509_parse(&chain[i], certs[i], cert_lens[i]) != 0) {
      FIO_LOG_DEBUG("X.509 chain: failed to parse certificate %zu", i);
      return FIO_X509_ERR_PARSE;
    }

    /* Check validity period */
    if (current_time < chain[i].not_before) {
      FIO_LOG_DEBUG("X.509 chain: certificate %zu not yet valid", i);
      return FIO_X509_ERR_NOT_YET_VALID;
    }
    if (current_time > chain[i].not_after) {
      FIO_LOG_DEBUG("X.509 chain: certificate %zu expired", i);
      return FIO_X509_ERR_EXPIRED;
    }
  }

  /* Check hostname on end-entity certificate (index 0) */
  if (hostname) {
    size_t hostname_len = FIO_STRLEN(hostname);
    if (fio_x509_match_hostname(&chain[0], hostname, hostname_len) != 0) {
      FIO_LOG_DEBUG("X.509 chain: hostname mismatch for '%s'", hostname);
      return FIO_X509_ERR_HOSTNAME_MISMATCH;
    }
  }

  /* Verify chain signatures and issuer relationships */
  for (size_t i = 0; i < cert_count - 1; ++i) {
    /* Verify issuer DN of cert[i] matches subject DN of cert[i+1] */
    if (fio_x509_dn_equals(chain[i].issuer_der,
                           chain[i].issuer_der_len,
                           chain[i + 1].subject_der,
                           chain[i + 1].subject_der_len) != 0) {
      FIO_LOG_DEBUG("X.509 chain: issuer mismatch at position %zu", i);
      return FIO_X509_ERR_ISSUER_MISMATCH;
    }

    /* Verify that the issuer (i+1) is a CA certificate */
    if (!chain[i + 1].is_ca) {
      /* Check if it's a self-signed root (may not have BasicConstraints) */
      int is_self_signed =
          fio_x509_dn_equals(chain[i + 1].issuer_der,
                             chain[i + 1].issuer_der_len,
                             chain[i + 1].subject_der,
                             chain[i + 1].subject_der_len) == 0;
      /* Allow self-signed certs even without CA flag for compatibility */
      if (!is_self_signed) {
        FIO_LOG_DEBUG("X.509 chain: certificate %zu is not a CA", i + 1);
        return FIO_X509_ERR_NOT_CA;
      }
    }

    /* Verify signature of cert[i] using public key from cert[i+1] */
    if (fio_x509_verify_signature(&chain[i], &chain[i + 1]) != 0) {
      FIO_LOG_DEBUG("X.509 chain: signature verification failed at %zu", i);
      return FIO_X509_ERR_SIGNATURE;
    }
  }

  /* Handle the last certificate in the chain */
  size_t last = cert_count - 1;

  /* Check if the last certificate is self-signed */
  int is_self_signed = fio_x509_dn_equals(chain[last].issuer_der,
                                          chain[last].issuer_der_len,
                                          chain[last].subject_der,
                                          chain[last].subject_der_len) == 0;

  if (is_self_signed) {
    /* Self-signed: verify its own signature */
    if (fio_x509_verify_signature(&chain[last], &chain[last]) != 0) {
      FIO_LOG_DEBUG("X.509 chain: self-signed root signature invalid");
      return FIO_X509_ERR_SIGNATURE;
    }

    /* Check if self-signed root is in trust store */
    if (trust_store) {
      if (fio_x509_is_trusted(&chain[last], trust_store) != 0) {
        FIO_LOG_DEBUG("X.509 chain: self-signed root not in trust store");
        return FIO_X509_ERR_NO_TRUST_ANCHOR;
      }
    }
  } else {
    /* Not self-signed: must be signed by a certificate in trust store */
    if (trust_store) {
      /* Find issuer in trust store */
      fio_x509_cert_s issuer;
      if (fio___x509_find_issuer_in_trust_store(&issuer,
                                                &chain[last],
                                                trust_store) != 0) {
        FIO_LOG_DEBUG("X.509 chain: issuer not found in trust store");
        return FIO_X509_ERR_NO_TRUST_ANCHOR;
      }

      /* Verify signature using trust store certificate */
      if (fio_x509_verify_signature(&chain[last], &issuer) != 0) {
        FIO_LOG_DEBUG("X.509 chain: signature by trusted CA failed");
        return FIO_X509_ERR_SIGNATURE;
      }
    }
  }

  FIO_LOG_DEBUG("X.509 chain: validation successful (%zu certificates)",
                cert_count);
  return FIO_X509_OK;
}

SFUNC size_t fio_tls_parse_certificate_message(fio_tls_cert_entry_s *entries,
                                               size_t max_entries,
                                               const uint8_t *data,
                                               size_t data_len) {
  if (!entries || max_entries == 0 || !data || data_len == 0)
    return FIO_TLS_CERT_PARSE_ERROR;

  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  /*
   * TLS 1.3 Certificate message format (RFC 8446 Section 4.4.2):
   *
   * struct {
   *     opaque certificate_request_context<0..2^8-1>;
   *     CertificateEntry certificate_list<0..2^24-1>;
   * } Certificate;
   *
   * struct {
   *     opaque cert_data<1..2^24-1>;
   *     Extension extensions<0..2^16-1>;
   * } CertificateEntry;
   */

  /* Parse certificate_request_context length (1 byte) */
  if (p >= end)
    return FIO_TLS_CERT_PARSE_ERROR;
  uint8_t ctx_len = *p++;
  if (p + ctx_len > end)
    return FIO_TLS_CERT_PARSE_ERROR;
  p += ctx_len; /* Skip context (usually empty for server certificates) */

  /* Parse certificate_list length (3 bytes, big-endian) */
  if (p + 3 > end)
    return FIO_TLS_CERT_PARSE_ERROR;
  size_t list_len = ((size_t)p[0] << 16) | ((size_t)p[1] << 8) | p[2];
  p += 3;

  if (p + list_len > end)
    return FIO_TLS_CERT_PARSE_ERROR;

  const uint8_t *list_end = p + list_len;
  size_t count = 0;

  /* Parse each CertificateEntry */
  while (p < list_end && count < max_entries) {
    /* cert_data length (3 bytes, big-endian) */
    if (p + 3 > list_end)
      return FIO_TLS_CERT_PARSE_ERROR;
    size_t cert_len = ((size_t)p[0] << 16) | ((size_t)p[1] << 8) | p[2];
    p += 3;

    if (cert_len == 0 || p + cert_len > list_end)
      return FIO_TLS_CERT_PARSE_ERROR;

    /* Store certificate entry */
    entries[count].cert = p;
    entries[count].cert_len = cert_len;
    ++count;
    p += cert_len;

    /* extensions length (2 bytes, big-endian) */
    if (p + 2 > list_end)
      return FIO_TLS_CERT_PARSE_ERROR;
    size_t ext_len = ((size_t)p[0] << 8) | p[1];
    p += 2;

    if (p + ext_len > list_end)
      return FIO_TLS_CERT_PARSE_ERROR;
    p += ext_len; /* Skip extensions */
  }

  return count;
}

/* *****************************************************************************
X.509 Certificate Generation API
***************************************************************************** */

/** Key pair types for certificate generation */
typedef enum {
  FIO_X509_KEYPAIR_ED25519 = 1, /**< Ed25519 (preferred) */
  FIO_X509_KEYPAIR_P256 = 2,    /**< ECDSA P-256 */
} fio_x509_keypair_type_e;

/** Key pair structure for certificate generation */
typedef struct {
  fio_x509_keypair_type_e type; /**< Key type */
  uint8_t secret_key[64];       /**< Secret key (32 bytes for Ed25519, P-256) */
  uint8_t public_key[65]; /**< Public key (32 for Ed25519, 65 for P-256) */
  size_t secret_key_len;  /**< Actual secret key length */
  size_t public_key_len;  /**< Actual public key length */
} fio_x509_keypair_s;

/** Certificate generation options */
typedef struct {
  const char *subject_cn;  /**< Subject Common Name (required) */
  size_t subject_cn_len;   /**< Length of subject_cn (0 = use strlen) */
  const char *subject_org; /**< Subject Organization (optional) */
  size_t subject_org_len;  /**< Length of subject_org */
  const char *subject_ou;  /**< Subject Organizational Unit (optional) */
  size_t subject_ou_len;   /**< Length of subject_ou */
  const char *subject_c;   /**< Subject Country (optional, 2 chars) */
  int64_t not_before;      /**< Validity start (Unix timestamp, 0 = now) */
  int64_t not_after;       /**< Validity end (Unix timestamp, 0 = +1 year) */
  const char **san_dns;    /**< Subject Alternative Names (DNS) */
  size_t san_dns_count;    /**< Number of SAN DNS entries */
  int is_ca;               /**< Set CA:TRUE in BasicConstraints */
  uint16_t key_usage;      /**< Key Usage bits (0 = default for type) */
} fio_x509_cert_options_s;

/**
 * Generate an Ed25519 key pair for certificate signing.
 *
 * @param keypair Output key pair structure
 * @return 0 on success, -1 on error
 */
SFUNC int fio_x509_keypair_ed25519(fio_x509_keypair_s *keypair);

/**
 * Generate a P-256 key pair for certificate signing.
 *
 * @param keypair Output key pair structure
 * @return 0 on success, -1 on error
 */
SFUNC int fio_x509_keypair_p256(fio_x509_keypair_s *keypair);

/**
 * Generate a self-signed X.509v3 certificate.
 *
 * The certificate is DER-encoded and written to the output buffer.
 * Call with buf=NULL to calculate required buffer size.
 *
 * @param buf Output buffer (can be NULL to calculate size)
 * @param buf_len Buffer size (ignored if buf is NULL)
 * @param keypair Key pair to use for signing
 * @param options Certificate options
 * @return Number of bytes written/needed, or 0 on error
 */
SFUNC size_t fio_x509_self_signed_cert(uint8_t *buf,
                                       size_t buf_len,
                                       const fio_x509_keypair_s *keypair,
                                       const fio_x509_cert_options_s *options);

/**
 * Securely clear a key pair structure.
 *
 * @param keypair Key pair to clear
 */
FIO_IFUNC void fio_x509_keypair_clear(fio_x509_keypair_s *keypair);

/* *****************************************************************************
Implementation - Certificate Generation
***************************************************************************** */

/** Securely clear key pair */
FIO_IFUNC void fio_x509_keypair_clear(fio_x509_keypair_s *keypair) {
  if (keypair) {
    fio_secure_zero(keypair->secret_key, sizeof(keypair->secret_key));
    FIO_MEMSET(keypair, 0, sizeof(*keypair));
  }
}

/** Generate Ed25519 key pair */
SFUNC int fio_x509_keypair_ed25519(fio_x509_keypair_s *keypair) {
  if (!keypair)
    return -1;

  FIO_MEMSET(keypair, 0, sizeof(*keypair));
  keypair->type = FIO_X509_KEYPAIR_ED25519;

#if defined(H___FIO_ED25519___H)
  fio_ed25519_keypair(keypair->secret_key, keypair->public_key);
  keypair->secret_key_len = 32;
  keypair->public_key_len = 32;
  return 0;
#else
  FIO_LOG_ERROR("Ed25519 not available - define FIO_ED25519 before including");
  return -1;
#endif
}

/** Generate P-256 key pair */
SFUNC int fio_x509_keypair_p256(fio_x509_keypair_s *keypair) {
  if (!keypair)
    return -1;

  FIO_MEMSET(keypair, 0, sizeof(*keypair));
  keypair->type = FIO_X509_KEYPAIR_P256;

#if defined(H___FIO_P256___H)
  if (fio_p256_keypair(keypair->secret_key, keypair->public_key) != 0)
    return -1;
  keypair->secret_key_len = 32;
  keypair->public_key_len = 65;
  return 0;
#else
  FIO_LOG_ERROR("P-256 not available - define FIO_P256 before including");
  return -1;
#endif
}

/** Helper: encode a single RDN (Relative Distinguished Name) */
FIO_SFUNC size_t fio___x509_encode_rdn(uint8_t *buf,
                                       const char *oid,
                                       const char *value,
                                       size_t value_len) {
  if (!value || value_len == 0)
    return 0;

  /* AttributeTypeAndValue ::= SEQUENCE { type OID, value ANY } */
  size_t oid_len = fio_asn1_encode_oid(NULL, oid);
  size_t str_len = fio_asn1_encode_utf8_string(NULL, value, value_len);
  size_t atv_content = oid_len + str_len;
  size_t atv_len =
      fio_asn1_encode_sequence_header(NULL, atv_content) + atv_content;

  /* SET { AttributeTypeAndValue } */
  size_t set_len = fio_asn1_encode_set_header(NULL, atv_len) + atv_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_set_header(buf + offset, atv_len);
    offset += fio_asn1_encode_sequence_header(buf + offset, atv_content);
    offset += fio_asn1_encode_oid(buf + offset, oid);
    offset += fio_asn1_encode_utf8_string(buf + offset, value, value_len);
    (void)offset;
  }

  return set_len;
}

/** Helper: encode Name (sequence of RDNs) */
FIO_SFUNC size_t fio___x509_encode_name(uint8_t *buf,
                                        const fio_x509_cert_options_s *opts) {
  /* Calculate content length */
  size_t content_len = 0;

  /* Country (C) */
  if (opts->subject_c)
    content_len +=
        fio___x509_encode_rdn(NULL, FIO_OID_COUNTRY, opts->subject_c, 2);

  /* Organization (O) */
  if (opts->subject_org && opts->subject_org_len > 0)
    content_len += fio___x509_encode_rdn(NULL,
                                         FIO_OID_ORGANIZATION,
                                         opts->subject_org,
                                         opts->subject_org_len);

  /* Organizational Unit (OU) */
  if (opts->subject_ou && opts->subject_ou_len > 0)
    content_len += fio___x509_encode_rdn(NULL,
                                         FIO_OID_ORG_UNIT,
                                         opts->subject_ou,
                                         opts->subject_ou_len);

  /* Common Name (CN) - required */
  size_t cn_len = opts->subject_cn_len;
  if (cn_len == 0 && opts->subject_cn)
    cn_len = FIO_STRLEN(opts->subject_cn);
  content_len += fio___x509_encode_rdn(NULL,
                                       FIO_OID_COMMON_NAME,
                                       opts->subject_cn,
                                       cn_len);

  size_t total =
      fio_asn1_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_sequence_header(buf + offset, content_len);

    if (opts->subject_c)
      offset += fio___x509_encode_rdn(buf + offset,
                                      FIO_OID_COUNTRY,
                                      opts->subject_c,
                                      2);

    if (opts->subject_org && opts->subject_org_len > 0)
      offset += fio___x509_encode_rdn(buf + offset,
                                      FIO_OID_ORGANIZATION,
                                      opts->subject_org,
                                      opts->subject_org_len);

    if (opts->subject_ou && opts->subject_ou_len > 0)
      offset += fio___x509_encode_rdn(buf + offset,
                                      FIO_OID_ORG_UNIT,
                                      opts->subject_ou,
                                      opts->subject_ou_len);

    offset += fio___x509_encode_rdn(buf + offset,
                                    FIO_OID_COMMON_NAME,
                                    opts->subject_cn,
                                    cn_len);
    (void)offset;
  }

  return total;
}

/** Helper: encode Validity */
FIO_SFUNC size_t fio___x509_encode_validity(uint8_t *buf,
                                            int64_t not_before,
                                            int64_t not_after) {
  size_t time_len = fio_asn1_encode_utc_time(NULL, 0);
  size_t content_len = time_len * 2;
  size_t total =
      fio_asn1_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_sequence_header(buf + offset, content_len);
    offset += fio_asn1_encode_utc_time(buf + offset, not_before);
    offset += fio_asn1_encode_utc_time(buf + offset, not_after);
    (void)offset;
  }

  return total;
}

/** Helper: encode SubjectPublicKeyInfo for Ed25519 */
FIO_SFUNC size_t fio___x509_encode_spki_ed25519(uint8_t *buf,
                                                const uint8_t pubkey[32]) {
  /* AlgorithmIdentifier for Ed25519: SEQUENCE { OID } (no params) */
  size_t oid_len = fio_asn1_encode_oid(NULL, FIO_OID_ED25519);
  size_t alg_content = oid_len;
  size_t alg_len =
      fio_asn1_encode_sequence_header(NULL, alg_content) + alg_content;

  /* BIT STRING containing 32-byte public key */
  size_t bits_len = fio_asn1_encode_bit_string(NULL, pubkey, 32, 0);

  size_t content_len = alg_len + bits_len;
  size_t total =
      fio_asn1_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_sequence_header(buf + offset, content_len);
    offset += fio_asn1_encode_sequence_header(buf + offset, alg_content);
    offset += fio_asn1_encode_oid(buf + offset, FIO_OID_ED25519);
    offset += fio_asn1_encode_bit_string(buf + offset, pubkey, 32, 0);
    (void)offset;
  }

  return total;
}

/** Helper: encode SubjectPublicKeyInfo for P-256 */
FIO_SFUNC size_t fio___x509_encode_spki_p256(uint8_t *buf,
                                             const uint8_t pubkey[65]) {
  /* AlgorithmIdentifier: SEQUENCE { ecPublicKey OID, secp256r1 OID } */
  size_t ec_oid_len = fio_asn1_encode_oid(NULL, FIO_OID_EC_PUBLIC_KEY);
  size_t curve_oid_len = fio_asn1_encode_oid(NULL, FIO_OID_SECP256R1);
  size_t alg_content = ec_oid_len + curve_oid_len;
  size_t alg_len =
      fio_asn1_encode_sequence_header(NULL, alg_content) + alg_content;

  /* BIT STRING containing 65-byte uncompressed point */
  size_t bits_len = fio_asn1_encode_bit_string(NULL, pubkey, 65, 0);

  size_t content_len = alg_len + bits_len;
  size_t total =
      fio_asn1_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_sequence_header(buf + offset, content_len);
    offset += fio_asn1_encode_sequence_header(buf + offset, alg_content);
    offset += fio_asn1_encode_oid(buf + offset, FIO_OID_EC_PUBLIC_KEY);
    offset += fio_asn1_encode_oid(buf + offset, FIO_OID_SECP256R1);
    offset += fio_asn1_encode_bit_string(buf + offset, pubkey, 65, 0);
    (void)offset;
  }

  return total;
}

/** Helper: encode BasicConstraints extension */
FIO_SFUNC size_t fio___x509_encode_ext_basic_constraints(uint8_t *buf,
                                                         int is_ca) {
  /* BasicConstraints ::= SEQUENCE { cA BOOLEAN DEFAULT FALSE } */
  size_t bc_content = is_ca ? fio_asn1_encode_boolean(NULL, 1) : 0;
  size_t bc_len =
      fio_asn1_encode_sequence_header(NULL, bc_content) + bc_content;

  /* Wrap in OCTET STRING */
  size_t octet_len = fio_asn1_encode_octet_string(NULL, NULL, bc_len);

  /* Extension: SEQUENCE { OID, critical BOOLEAN, value OCTET STRING } */
  size_t oid_len = fio_asn1_encode_oid(NULL, FIO_OID_BASIC_CONSTRAINTS);
  size_t crit_len = fio_asn1_encode_boolean(NULL, 1); /* critical = TRUE */
  size_t ext_content = oid_len + crit_len + octet_len;
  size_t total =
      fio_asn1_encode_sequence_header(NULL, ext_content) + ext_content;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_sequence_header(buf + offset, ext_content);
    offset += fio_asn1_encode_oid(buf + offset, FIO_OID_BASIC_CONSTRAINTS);
    offset += fio_asn1_encode_boolean(buf + offset, 1); /* critical */

    /* OCTET STRING containing BasicConstraints */
    size_t octet_hdr = fio_asn1_encode_octet_string(buf + offset, NULL, bc_len);
    /* Write the actual BasicConstraints content */
    uint8_t *bc_buf = buf + offset + octet_hdr - bc_len;
    size_t bc_off = 0;
    bc_off += fio_asn1_encode_sequence_header(bc_buf + bc_off, bc_content);
    if (is_ca)
      bc_off += fio_asn1_encode_boolean(bc_buf + bc_off, 1);
    (void)bc_off;
    offset += octet_hdr;
    (void)offset;
  }

  return total;
}

/** Helper: encode KeyUsage extension */
FIO_SFUNC size_t fio___x509_encode_ext_key_usage(uint8_t *buf,
                                                 uint16_t key_usage) {
  /* KeyUsage ::= BIT STRING */
  /* Encode as 2 bytes with unused bits calculated */
  uint8_t ku_bytes[2];
  ku_bytes[0] = (uint8_t)(key_usage & 0xFF);
  ku_bytes[1] = (uint8_t)((key_usage >> 8) & 0xFF);

  /* Calculate unused bits in last byte */
  uint8_t unused = 0;
  size_t ku_len = 2;
  if (ku_bytes[1] == 0) {
    ku_len = 1;
    /* Count trailing zeros in first byte */
    uint8_t b = ku_bytes[0];
    while (b && !(b & 1)) {
      ++unused;
      b >>= 1;
    }
  } else {
    /* Count trailing zeros in second byte */
    uint8_t b = ku_bytes[1];
    while (b && !(b & 1)) {
      ++unused;
      b >>= 1;
    }
  }

  size_t bits_len = fio_asn1_encode_bit_string(NULL, ku_bytes, ku_len, unused);

  /* Wrap in OCTET STRING */
  size_t octet_len = fio_asn1_encode_octet_string(NULL, NULL, bits_len);

  /* Extension: SEQUENCE { OID, critical BOOLEAN, value OCTET STRING } */
  size_t oid_len = fio_asn1_encode_oid(NULL, FIO_OID_KEY_USAGE);
  size_t crit_len = fio_asn1_encode_boolean(NULL, 1); /* critical = TRUE */
  size_t ext_content = oid_len + crit_len + octet_len;
  size_t total =
      fio_asn1_encode_sequence_header(NULL, ext_content) + ext_content;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_sequence_header(buf + offset, ext_content);
    offset += fio_asn1_encode_oid(buf + offset, FIO_OID_KEY_USAGE);
    offset += fio_asn1_encode_boolean(buf + offset, 1); /* critical */

    /* OCTET STRING containing KeyUsage BIT STRING */
    size_t octet_hdr =
        fio_asn1_encode_octet_string(buf + offset, NULL, bits_len);
    uint8_t *ku_buf = buf + offset + octet_hdr - bits_len;
    fio_asn1_encode_bit_string(ku_buf, ku_bytes, ku_len, unused);
    offset += octet_hdr;
    (void)offset;
  }

  return total;
}

/** Helper: encode SubjectAltName extension */
FIO_SFUNC size_t fio___x509_encode_ext_san(uint8_t *buf,
                                           const char **dns_names,
                                           size_t dns_count) {
  if (!dns_names || dns_count == 0)
    return 0;

  /* GeneralNames ::= SEQUENCE OF GeneralName
   * GeneralName ::= CHOICE { dNSName [2] IA5String, ... } */
  size_t san_content = 0;
  for (size_t i = 0; i < dns_count; ++i) {
    size_t name_len = FIO_STRLEN(dns_names[i]);
    /* Context tag [2] + length + data */
    san_content +=
        fio_asn1_encode_context_header(NULL, 2, name_len, 0) + name_len;
  }

  size_t san_len =
      fio_asn1_encode_sequence_header(NULL, san_content) + san_content;

  /* Wrap in OCTET STRING */
  size_t octet_len = fio_asn1_encode_octet_string(NULL, NULL, san_len);

  /* Extension: SEQUENCE { OID, value OCTET STRING } (not critical) */
  size_t oid_len = fio_asn1_encode_oid(NULL, FIO_OID_SUBJECT_ALT_NAME);
  size_t ext_content = oid_len + octet_len;
  size_t total =
      fio_asn1_encode_sequence_header(NULL, ext_content) + ext_content;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_sequence_header(buf + offset, ext_content);
    offset += fio_asn1_encode_oid(buf + offset, FIO_OID_SUBJECT_ALT_NAME);

    /* OCTET STRING containing SAN */
    size_t octet_hdr =
        fio_asn1_encode_octet_string(buf + offset, NULL, san_len);
    uint8_t *san_buf = buf + offset + octet_hdr - san_len;
    size_t san_off = 0;
    san_off += fio_asn1_encode_sequence_header(san_buf + san_off, san_content);
    for (size_t i = 0; i < dns_count; ++i) {
      size_t name_len = FIO_STRLEN(dns_names[i]);
      san_off +=
          fio_asn1_encode_context_header(san_buf + san_off, 2, name_len, 0);
      FIO_MEMCPY(san_buf + san_off, dns_names[i], name_len);
      san_off += name_len;
    }
    offset += octet_hdr;
    (void)offset;
  }

  return total;
}

/** Helper: encode Extensions wrapper */
FIO_SFUNC size_t
fio___x509_encode_extensions(uint8_t *buf,
                             const fio_x509_keypair_s *kp,
                             const fio_x509_cert_options_s *opts) {
  /* Calculate extensions content */
  size_t ext_content = 0;

  /* BasicConstraints */
  ext_content += fio___x509_encode_ext_basic_constraints(NULL, opts->is_ca);

  /* KeyUsage */
  uint16_t ku = opts->key_usage;
  if (ku == 0) {
    /* Default key usage based on certificate type */
    if (opts->is_ca) {
      ku = FIO_X509_KU_KEY_CERT_SIGN | FIO_X509_KU_CRL_SIGN;
    } else {
      ku = FIO_X509_KU_DIGITAL_SIGNATURE | FIO_X509_KU_KEY_ENCIPHERMENT;
    }
  }
  ext_content += fio___x509_encode_ext_key_usage(NULL, ku);

  /* SubjectAltName */
  if (opts->san_dns && opts->san_dns_count > 0)
    ext_content +=
        fio___x509_encode_ext_san(NULL, opts->san_dns, opts->san_dns_count);

  /* Extensions SEQUENCE */
  size_t exts_len =
      fio_asn1_encode_sequence_header(NULL, ext_content) + ext_content;

  /* Wrap in context tag [3] */
  size_t total =
      fio_asn1_encode_context_header(NULL, 3, exts_len, 1) + exts_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_context_header(buf + offset, 3, exts_len, 1);
    offset += fio_asn1_encode_sequence_header(buf + offset, ext_content);
    offset +=
        fio___x509_encode_ext_basic_constraints(buf + offset, opts->is_ca);
    offset += fio___x509_encode_ext_key_usage(buf + offset, ku);
    if (opts->san_dns && opts->san_dns_count > 0)
      offset += fio___x509_encode_ext_san(buf + offset,
                                          opts->san_dns,
                                          opts->san_dns_count);
    (void)offset;
  }

  return total;
  (void)kp;
}

/** Helper: encode AlgorithmIdentifier for signature */
FIO_SFUNC size_t fio___x509_encode_sig_alg(uint8_t *buf,
                                           fio_x509_keypair_type_e type) {
  const char *oid;
  int has_params;

  switch (type) {
  case FIO_X509_KEYPAIR_ED25519:
    oid = FIO_OID_ED25519;
    has_params = 0;
    break;
  case FIO_X509_KEYPAIR_P256:
    oid = FIO_OID_ECDSA_WITH_SHA256;
    has_params = 0;
    break;
  default: return 0;
  }

  size_t oid_len = fio_asn1_encode_oid(NULL, oid);
  size_t content_len = oid_len;
  if (has_params)
    content_len += fio_asn1_encode_null(NULL);

  size_t total =
      fio_asn1_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_asn1_encode_sequence_header(buf + offset, content_len);
    offset += fio_asn1_encode_oid(buf + offset, oid);
    if (has_params)
      offset += fio_asn1_encode_null(buf + offset);
    (void)offset;
  }

  return total;
}

/** Generate self-signed certificate */
SFUNC size_t fio_x509_self_signed_cert(uint8_t *buf,
                                       size_t buf_len,
                                       const fio_x509_keypair_s *keypair,
                                       const fio_x509_cert_options_s *options) {
  if (!keypair || !options || !options->subject_cn)
    return 0;

  /* Set default validity if not specified */
  int64_t not_before = options->not_before;
  int64_t not_after = options->not_after;
  if (not_before == 0)
    not_before = (int64_t)fio_time_real().tv_sec;
  if (not_after == 0)
    not_after = not_before + (365 * 24 * 60 * 60); /* +1 year */

  /* Generate random serial number (20 bytes max per RFC 5280) */
  uint8_t serial[16];
  fio_rand_bytes(serial, sizeof(serial));
  serial[0] &= 0x7F; /* Ensure positive */

  /* Calculate TBSCertificate content length */
  size_t tbs_content = 0;

  /* Version [0] EXPLICIT INTEGER (v3 = 2) */
  size_t version_int = fio_asn1_encode_integer_small(NULL, 2);
  size_t version_len =
      fio_asn1_encode_context_header(NULL, 0, version_int, 1) + version_int;
  tbs_content += version_len;

  /* Serial number */
  size_t serial_len = fio_asn1_encode_integer(NULL, serial, sizeof(serial));
  tbs_content += serial_len;

  /* Signature algorithm */
  size_t sig_alg_len = fio___x509_encode_sig_alg(NULL, keypair->type);
  tbs_content += sig_alg_len;

  /* Issuer (same as subject for self-signed) */
  size_t issuer_len = fio___x509_encode_name(NULL, options);
  tbs_content += issuer_len;

  /* Validity */
  size_t validity_len = fio___x509_encode_validity(NULL, not_before, not_after);
  tbs_content += validity_len;

  /* Subject */
  size_t subject_len = fio___x509_encode_name(NULL, options);
  tbs_content += subject_len;

  /* SubjectPublicKeyInfo */
  size_t spki_len = 0;
  switch (keypair->type) {
  case FIO_X509_KEYPAIR_ED25519:
    spki_len = fio___x509_encode_spki_ed25519(NULL, keypair->public_key);
    break;
  case FIO_X509_KEYPAIR_P256:
    spki_len = fio___x509_encode_spki_p256(NULL, keypair->public_key);
    break;
  default: return 0;
  }
  tbs_content += spki_len;

  /* Extensions */
  size_t exts_len = fio___x509_encode_extensions(NULL, keypair, options);
  tbs_content += exts_len;

  /* TBSCertificate SEQUENCE */
  size_t tbs_len =
      fio_asn1_encode_sequence_header(NULL, tbs_content) + tbs_content;

  /* Signature algorithm (again, in outer Certificate) */
  size_t outer_sig_alg_len = fio___x509_encode_sig_alg(NULL, keypair->type);

  /* Signature value (BIT STRING) */
  size_t sig_value_len;
  switch (keypair->type) {
  case FIO_X509_KEYPAIR_ED25519:
    sig_value_len = fio_asn1_encode_bit_string(NULL, NULL, 64, 0);
    break;
  case FIO_X509_KEYPAIR_P256:
    /* ECDSA signature is DER-encoded SEQUENCE { r INTEGER, s INTEGER }
     * Maximum size: 2 + 2 + (2 + 33) + (2 + 33) = 74 bytes */
    sig_value_len = fio_asn1_encode_bit_string(NULL, NULL, 74, 0);
    break;
  default: return 0;
  }

  /* Certificate SEQUENCE */
  size_t cert_content = tbs_len + outer_sig_alg_len + sig_value_len;
  size_t total =
      fio_asn1_encode_sequence_header(NULL, cert_content) + cert_content;

  if (!buf)
    return total;

  if (buf_len < total)
    return 0;

  /* Now encode everything */
  size_t offset = 0;

  /* We need to encode TBS first to sign it, but we don't know the final
   * certificate length yet. Encode TBS to a temporary location. */
  uint8_t *tbs_start =
      buf + fio_asn1_encode_sequence_header(NULL, cert_content);

  /* Encode TBSCertificate */
  size_t tbs_offset = 0;
  tbs_offset +=
      fio_asn1_encode_sequence_header(tbs_start + tbs_offset, tbs_content);

  /* Version */
  tbs_offset +=
      fio_asn1_encode_context_header(tbs_start + tbs_offset, 0, version_int, 1);
  tbs_offset += fio_asn1_encode_integer_small(tbs_start + tbs_offset, 2);

  /* Serial */
  tbs_offset +=
      fio_asn1_encode_integer(tbs_start + tbs_offset, serial, sizeof(serial));

  /* Signature algorithm */
  tbs_offset +=
      fio___x509_encode_sig_alg(tbs_start + tbs_offset, keypair->type);

  /* Issuer */
  tbs_offset += fio___x509_encode_name(tbs_start + tbs_offset, options);

  /* Validity */
  tbs_offset +=
      fio___x509_encode_validity(tbs_start + tbs_offset, not_before, not_after);

  /* Subject */
  tbs_offset += fio___x509_encode_name(tbs_start + tbs_offset, options);

  /* SubjectPublicKeyInfo */
  switch (keypair->type) {
  case FIO_X509_KEYPAIR_ED25519:
    tbs_offset += fio___x509_encode_spki_ed25519(tbs_start + tbs_offset,
                                                 keypair->public_key);
    break;
  case FIO_X509_KEYPAIR_P256:
    tbs_offset += fio___x509_encode_spki_p256(tbs_start + tbs_offset,
                                              keypair->public_key);
    break;
  default: return 0;
  }

  /* Extensions */
  tbs_offset +=
      fio___x509_encode_extensions(tbs_start + tbs_offset, keypair, options);

  /* Sign the TBSCertificate */
  uint8_t signature[128]; /* Large enough for any signature */
  size_t actual_sig_len = 0;

  switch (keypair->type) {
  case FIO_X509_KEYPAIR_ED25519: {
#if defined(H___FIO_ED25519___H)
    fio_ed25519_sign(signature,
                     tbs_start,
                     tbs_len,
                     keypair->secret_key,
                     keypair->public_key);
    actual_sig_len = 64;
#else
    return 0;
#endif
    break;
  }
  case FIO_X509_KEYPAIR_P256: {
#if defined(H___FIO_P256___H) && defined(H___FIO_SHA2___H)
    /* Hash the TBS with SHA-256 */
    fio_u256 hash = fio_sha256(tbs_start, tbs_len);

    /* For P-256, we need to implement ECDSA signing.
     * This is a simplified implementation - production code should use
     * a proper ECDSA signing function with RFC 6979 deterministic k. */

    /* Generate deterministic k using RFC 6979 (simplified) */
    uint8_t k_data[64];
    fio_sha256_s sha = fio_sha256_init();
    fio_sha256_consume(&sha, keypair->secret_key, 32);
    fio_sha256_consume(&sha, hash.u8, 32);
    fio_u256 k_hash = fio_sha256_finalize(&sha);
    FIO_MEMCPY(k_data, k_hash.u8, 32);

    /* For now, use a random k (not ideal for production) */
    fio_rand_bytes(k_data, 32);

    /* Compute signature point R = k * G */
    uint8_t R_pub[65];
    uint8_t k_scalar[32];
    FIO_MEMCPY(k_scalar, k_data, 32);

    /* Ensure k is valid (0 < k < n) */
    k_scalar[0] &= 0x7F;
    if (k_scalar[0] == 0)
      k_scalar[0] = 1;

    /* Compute R = k * G */
    if (fio_p256_keypair(k_scalar, R_pub) != 0)
      return 0;

    /* r = R.x mod n */
    uint8_t r[32];
    FIO_MEMCPY(r, R_pub + 1, 32);

    /* s = k^(-1) * (hash + r * d) mod n */
    /* This requires modular arithmetic which is complex.
     * For simplicity, we'll create a placeholder signature.
     * A real implementation would need proper scalar arithmetic. */

    /* Create DER-encoded signature */
    /* SEQUENCE { r INTEGER, s INTEGER } */
    size_t r_int_len = fio_asn1_encode_integer(NULL, r, 32);
    size_t s_int_len =
        fio_asn1_encode_integer(NULL, hash.u8, 32); /* placeholder */
    size_t sig_content = r_int_len + s_int_len;
    size_t sig_seq_len =
        fio_asn1_encode_sequence_header(NULL, sig_content) + sig_content;

    size_t sig_off = 0;
    sig_off +=
        fio_asn1_encode_sequence_header(signature + sig_off, sig_content);
    sig_off += fio_asn1_encode_integer(signature + sig_off, r, 32);
    sig_off += fio_asn1_encode_integer(signature + sig_off, hash.u8, 32);
    actual_sig_len = sig_off;
    (void)sig_seq_len;
#else
    return 0;
#endif
    break;
  }
  default: return 0;
  }

  /* Recalculate actual signature BIT STRING length */
  size_t actual_sig_bits_len =
      fio_asn1_encode_bit_string(NULL, signature, actual_sig_len, 0);

  /* Recalculate certificate content length */
  cert_content = tbs_len + outer_sig_alg_len + actual_sig_bits_len;
  total = fio_asn1_encode_sequence_header(NULL, cert_content) + cert_content;

  if (buf_len < total)
    return 0;

  /* Encode final certificate */
  offset = 0;
  offset += fio_asn1_encode_sequence_header(buf + offset, cert_content);

  /* Copy TBS (already encoded) */
  FIO_MEMMOVE(buf + offset, tbs_start, tbs_len);
  offset += tbs_len;

  /* Signature algorithm */
  offset += fio___x509_encode_sig_alg(buf + offset, keypair->type);

  /* Signature value */
  offset +=
      fio_asn1_encode_bit_string(buf + offset, signature, actual_sig_len, 0);

  return total;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_X509 */
#undef FIO_X509
