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
} fio_x509_key_algo_e;

/** Signature algorithm types */
typedef enum {
  FIO_X509_SIGNATURE_UNKNOWN = 0,
  FIO_X509_SIGNATURE_RSA_PKCS1_SHA256 = 1, /**< sha256WithRSAEncryption */
  FIO_X509_SIGNATURE_RSA_PKCS1_SHA384 = 2, /**< sha384WithRSAEncryption */
  FIO_X509_SIGNATURE_RSA_PKCS1_SHA512 = 3, /**< sha512WithRSAEncryption */
  FIO_X509_SIGNATURE_RSA_PSS_SHA256 = 4,   /**< RSA-PSS with SHA-256 */
  FIO_X509_SIGNATURE_RSA_PSS_SHA384 = 5,   /**< RSA-PSS with SHA-384 */
  FIO_X509_SIGNATURE_RSA_PSS_SHA512 = 6,   /**< RSA-PSS with SHA-512 */
  FIO_X509_SIGNATURE_ECDSA_SHA256 = 7,     /**< ecdsa-with-SHA256 */
  FIO_X509_SIGNATURE_ECDSA_SHA384 = 8,     /**< ecdsa-with-SHA384 */
  FIO_X509_SIGNATURE_ED25519 = 9,          /**< Ed25519 */
} fio_x509_signature_algo_e;

/** Key Usage bit flags (RFC 5280 Section 4.2.1.3)
 *
 * ASN.1 BIT STRING uses MSB-first bit ordering:
 * - Bit 0 = MSB of first byte (0x80)
 * - Bit 1 = 0x40, Bit 2 = 0x20, etc.
 * - Bits 8+ are in the second byte
 */
typedef enum {
  FIO_X509_KU_DIGITAL_SIGNATURE = 0x0080, /* bit 0 = MSB of byte 0 */
  FIO_X509_KU_NON_REPUDIATION = 0x0040,   /* bit 1 */
  FIO_X509_KU_KEY_ENCIPHERMENT = 0x0020,  /* bit 2 */
  FIO_X509_KU_DATA_ENCIPHERMENT = 0x0010, /* bit 3 */
  FIO_X509_KU_KEY_AGREEMENT = 0x0008,     /* bit 4 */
  FIO_X509_KU_KEY_CERT_SIGN = 0x0004,     /* bit 5 */
  FIO_X509_KU_CRL_SIGN = 0x0002,          /* bit 6 */
  FIO_X509_KU_ENCIPHER_ONLY = 0x0001,     /* bit 7 */
  FIO_X509_KU_DECIPHER_ONLY = 0x8000,     /* bit 8 = MSB of byte 1 */
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
  FIO_X509_ERR_KEY_USAGE = -11,   /**< (Extended) Key Usage forbids this use */
  FIO_X509_ERR_PATH_LENGTH = -12, /**< Basic Constraints pathLen exceeded */
  FIO_X509_ERR_NAME_CONSTRAINTS = -13,   /**< Name outside CA's constraints */
  FIO_X509_ERR_CRITICAL_EXTENSION = -14, /**< Unknown critical extension */
} fio_x509_error_e;

/** Certificate purpose for chain validation (Extended Key Usage). */
typedef enum {
  FIO_X509_PURPOSE_TLS = 0,    /**< TLS server OR client authentication */
  FIO_X509_PURPOSE_SERVER = 1, /**< TLS server authentication (serverAuth) */
  FIO_X509_PURPOSE_CLIENT = 2, /**< TLS client authentication (clientAuth) */
} fio_x509_purpose_e;

/** `fio_x509_cert_s.ext_flags` bits (set by fio_x509_parse) */
#define FIO_X509_EXT_EKU              0x01 /**< Extended Key Usage present */
#define FIO_X509_EXT_EKU_SERVER       0x02 /**< EKU lists serverAuth */
#define FIO_X509_EXT_EKU_CLIENT       0x04 /**< EKU lists clientAuth */
#define FIO_X509_EXT_EKU_ANY          0x08 /**< EKU lists anyExtendedKeyUsage */
#define FIO_X509_EXT_UNKNOWN_CRITICAL 0x10 /**< unrecognized critical ext. */

/** `fio_x509_cert_s.path_len` value when no pathLenConstraint is present */
#define FIO_X509_PATH_LEN_NONE 0xFF

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

/**
 * Parsed X.509 certificate structure.
 *
 * All buffer fields are non-owning views (`fio_buf_info_s` /
 * `fio_ubuf_info_s`) into the original DER data, which must remain valid
 * while the certificate is in use.
 *
 * When a certificate is obtained through fio_io_peer_info_next (TLS peer
 * certificate inspection), the views point into memory owned by the TLS
 * backend and remain valid until the next fio_io_peer_info_next call (on ANY
 * connection) or until the connection is closed, whichever comes first.
 */
typedef struct fio_x509_cert_s {
  /* Buffer views (16 bytes each, 8-byte aligned — no padding) */

  /** Raw DER data (points to the original input, NOT a copy) */
  fio_ubuf_info_s der;

  /** Certificate serial number (raw INTEGER contents, pointer into DER) */
  fio_ubuf_info_s serial;

  /** Subject Distinguished Name (raw DER for comparison) */
  fio_ubuf_info_s subject;

  /** Issuer Distinguished Name (raw DER for comparison) */
  fio_ubuf_info_s issuer;

  /** Subject Common Name (if present, pointer into DER data) */
  fio_buf_info_s cn;

  /** Public Key Data (union based on key_algo) */
  union {
    struct {
      fio_ubuf_info_s n; /**< RSA modulus (big-endian) */
      fio_ubuf_info_s e; /**< RSA exponent (big-endian) */
    } rsa;
    struct {
      /** SEC 1 EC point: uncompressed (04 || x || y) or compressed
       * (02/03 || x); not validated by the parser. */
      fio_ubuf_info_s point;
    } ecdsa;
    struct {
      fio_ubuf_info_s key; /**< 32-byte Ed25519 public key */
    } ed25519;
  } pubkey;

  /** Signature value (pointer into DER data) */
  fio_ubuf_info_s signature;

  /** TBS Certificate (for signature verification) */
  fio_ubuf_info_s tbs;

  /** Subject Alternative Name: first DNS name (if present) */
  fio_buf_info_s san_dns;

  /** Subject Alternative Name extension raw data (for iterating all SANs) */
  fio_ubuf_info_s san_ext;

  /** Subject Alternative Name: first IP address (if present) */
  fio_ubuf_info_s san_ip;

  /** Name Constraints extension value (raw NameConstraints SEQUENCE TLV) */
  fio_ubuf_info_s name_constraints;

  /** Validity period (Unix timestamps) */
  int64_t not_before;
  int64_t not_after;

  /** SHA-256 fingerprint of the DER data (see fio_x509_fingerprint) */
  uint8_t fingerprint[32];

  /* Small fields grouped at the end (no bitfields — byte access is faster) */

  /** Public Key Type */
  fio_x509_key_algo_e key_algo;
  /** Signature Algorithm */
  fio_x509_signature_algo_e signature_algo;
  /** Key Usage extension bits */
  uint16_t key_usage;
  /** Peer chain verification state: non-zero if a TLS backend verified this
   * certificate's chain (see fio_io_peer_info_next).  Always zero after
   * fio_x509_parse — parsing does not verify anything. */
  uint8_t verified;
  /** Position in the peer certificate chain (0 = leaf certificate).
   *  Iteration is capped at 128 certificates (max value 127) as a
   *  deep-nesting / DoS guard.  Set by fio_io_peer_info_next.
   *  Always zero after fio_x509_parse. */
  uint8_t chain_index;
  /** Certificate version (0=v1, 1=v2, 2=v3) */
  uint8_t version;
  /** Basic Constraints: is CA */
  uint8_t is_ca;
  /** Key Usage extension present */
  uint8_t has_key_usage;
  /** Basic Constraints pathLenConstraint (FIO_X509_PATH_LEN_NONE if absent;
   *  values above 254 are clamped to 254) */
  uint8_t path_len;
  /** Extension flags (FIO_X509_EXT_* bits) */
  uint8_t ext_flags;

} fio_x509_cert_s;

/* *****************************************************************************
X.509 OID Constants (fio_u128 byte-15 design)
*****************************************************************************

An OID value is a plain fio_u128 where bytes 0-14 hold the DER content bytes
(zero-padded) and byte 15 holds the content length. Compare parsed OID
elements against these constants with fio___der_oid_eq (two u64 lane ==,
no memcmp, no call) after building the value once with fio___der_oid_value.
Generated by `bun ./ai-tools/oid-table.js c` - do not edit by hand.
***************************************************************************** */

/* clang-format off */
/* Signature Algorithms */
#define FIO_X509_OID_SHA256_WITH_RSA \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B, [15] = 9}}) /* 1.2.840.113549.1.1.11 */
#define FIO_X509_OID_SHA384_WITH_RSA \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0C, [15] = 9}}) /* 1.2.840.113549.1.1.12 */
#define FIO_X509_OID_SHA512_WITH_RSA \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0D, [15] = 9}}) /* 1.2.840.113549.1.1.13 */
#define FIO_X509_OID_RSA_PSS \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A, [15] = 9}}) /* 1.2.840.113549.1.1.10 */
#define FIO_X509_OID_ECDSA_WITH_SHA256 \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, [15] = 8}}) /* 1.2.840.10045.4.3.2 */
#define FIO_X509_OID_ECDSA_WITH_SHA384 \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x03, [15] = 8}}) /* 1.2.840.10045.4.3.3 */
#define FIO_X509_OID_ECDSA_WITH_SHA512 \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x04, [15] = 8}}) /* 1.2.840.10045.4.3.4 */
#define FIO_X509_OID_ED25519 \
  ((fio_u128){.u8 = {0x2B, 0x65, 0x70, [15] = 3}}) /* 1.3.101.112 */
#define FIO_X509_OID_ED448 \
  ((fio_u128){.u8 = {0x2B, 0x65, 0x71, [15] = 3}}) /* 1.3.101.113 */
#define FIO_X509_OID_MGF1 \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x08, [15] = 9}}) /* 1.2.840.113549.1.1.8 */

/* Hash Algorithms (RSA-PSS parameters) */
#define FIO_X509_OID_SHA256 \
  ((fio_u128){.u8 = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, [15] = 9}}) /* 2.16.840.1.101.3.4.2.1 */
#define FIO_X509_OID_SHA384 \
  ((fio_u128){.u8 = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, [15] = 9}}) /* 2.16.840.1.101.3.4.2.2 */
#define FIO_X509_OID_SHA512 \
  ((fio_u128){.u8 = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, [15] = 9}}) /* 2.16.840.1.101.3.4.2.3 */

/* Public Key Algorithms */
#define FIO_X509_OID_RSA_ENCRYPTION \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, [15] = 9}}) /* 1.2.840.113549.1.1.1 */
#define FIO_X509_OID_EC_PUBLIC_KEY \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, [15] = 7}}) /* 1.2.840.10045.2.1 */

/* Elliptic Curves */
#define FIO_X509_OID_SECP256R1 \
  ((fio_u128){.u8 = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, [15] = 8}}) /* 1.2.840.10045.3.1.7 */
#define FIO_X509_OID_SECP384R1 \
  ((fio_u128){.u8 = {0x2B, 0x81, 0x04, 0x00, 0x22, [15] = 5}}) /* 1.3.132.0.34 */
#define FIO_X509_OID_SECP521R1 \
  ((fio_u128){.u8 = {0x2B, 0x81, 0x04, 0x00, 0x23, [15] = 5}}) /* 1.3.132.0.35 */
#define FIO_X509_OID_X25519 \
  ((fio_u128){.u8 = {0x2B, 0x65, 0x6E, [15] = 3}}) /* 1.3.101.110 */
#define FIO_X509_OID_X448 \
  ((fio_u128){.u8 = {0x2B, 0x65, 0x6F, [15] = 3}}) /* 1.3.101.111 */

/* X.509 Extensions */
#define FIO_X509_OID_SUBJECT_KEY_ID \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x0E, [15] = 3}}) /* 2.5.29.14 */
#define FIO_X509_OID_KEY_USAGE \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x0F, [15] = 3}}) /* 2.5.29.15 */
#define FIO_X509_OID_SUBJECT_ALT_NAME \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x11, [15] = 3}}) /* 2.5.29.17 */
#define FIO_X509_OID_NAME_CONSTRAINTS \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x1E, [15] = 3}}) /* 2.5.29.30 */
#define FIO_X509_OID_BASIC_CONSTRAINTS \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x13, [15] = 3}}) /* 2.5.29.19 */
#define FIO_X509_OID_CRL_DIST_POINTS \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x1F, [15] = 3}}) /* 2.5.29.31 */
#define FIO_X509_OID_CERT_POLICIES \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x20, [15] = 3}}) /* 2.5.29.32 */
#define FIO_X509_OID_AUTH_KEY_ID \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x23, [15] = 3}}) /* 2.5.29.35 */
#define FIO_X509_OID_EXT_KEY_USAGE \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x25, [15] = 3}}) /* 2.5.29.37 */

/* Extended Key Usage */
#define FIO_X509_OID_EKU_SERVER_AUTH \
  ((fio_u128){.u8 = {0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01, [15] = 8}}) /* 1.3.6.1.5.5.7.3.1 */
#define FIO_X509_OID_EKU_CLIENT_AUTH \
  ((fio_u128){.u8 = {0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x02, [15] = 8}}) /* 1.3.6.1.5.5.7.3.2 */
#define FIO_X509_OID_EKU_ANY \
  ((fio_u128){.u8 = {0x55, 0x1D, 0x25, 0x00, [15] = 4}}) /* 2.5.29.37.0 */

/* Distinguished Name Attributes */
#define FIO_X509_OID_COMMON_NAME \
  ((fio_u128){.u8 = {0x55, 0x04, 0x03, [15] = 3}}) /* 2.5.4.3 */
#define FIO_X509_OID_COUNTRY \
  ((fio_u128){.u8 = {0x55, 0x04, 0x06, [15] = 3}}) /* 2.5.4.6 */
#define FIO_X509_OID_LOCALITY \
  ((fio_u128){.u8 = {0x55, 0x04, 0x07, [15] = 3}}) /* 2.5.4.7 */
#define FIO_X509_OID_STATE \
  ((fio_u128){.u8 = {0x55, 0x04, 0x08, [15] = 3}}) /* 2.5.4.8 */
#define FIO_X509_OID_ORGANIZATION \
  ((fio_u128){.u8 = {0x55, 0x04, 0x0A, [15] = 3}}) /* 2.5.4.10 */
#define FIO_X509_OID_ORG_UNIT \
  ((fio_u128){.u8 = {0x55, 0x04, 0x0B, [15] = 3}}) /* 2.5.4.11 */

/* clang-format on */

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
 * Computes the SHA-256 fingerprint of the certificate's DER data, storing
 * the raw 32-byte hash in `cert->fingerprint`.
 *
 * This is the standard fingerprint: it identifies the exact certificate bytes
 * (`cert->der`), not an issuer/serial or public-key identity. One signed
 * certificate may have several valid encodings (non-minimal headers outside
 * the signed TBS, or an ECDSA signature (r, s) that is also valid as
 * (r, n - s)), so do not blocklist by fingerprint;
 * use issuer + serial or an SPKI hash. Pinning (allow-listing) by fingerprint
 * fails closed.
 *
 * Call after fio_x509_parse. This is a lazy (on-demand) operation so that
 * parsing alone never pays the hashing cost.
 *
 * @param cert Parsed certificate (must have been filled by fio_x509_parse)
 */
SFUNC void fio_x509_fingerprint(fio_x509_cert_s *cert);

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
 *   6. Verify issuers are CAs (CA:TRUE, keyCertSign when Key Usage is
 *      present, pathLenConstraint and DNS/IP/directoryName Name Constraints)
 *   7. Reject unrecognized critical extensions; enforce the leaf's
 *      digitalSignature Key Usage and the Extended Key Usage purpose
 *   8. Verify the chain terminates at a trusted root (if trust store provided)
 *
 * The purpose is FIO_X509_PURPOSE_SERVER when `hostname` is set, otherwise
 * FIO_X509_PURPOSE_TLS (see fio_x509_verify_chain_purpose).
 *
 * @param certs Array of DER-encoded certificate views
 * @param cert_count Number of certificates in chain
 * @param hostname Expected hostname for end-entity (NULL to skip check)
 * @param current_time Current Unix timestamp for validity checking
 * @param trust_store Root CA certificates (NULL to skip trust check)
 * @return FIO_X509_OK (0) on success, or error code on failure
 */
SFUNC int fio_x509_verify_chain(const fio_ubuf_info_s *certs,
                                size_t cert_count,
                                const char *hostname,
                                int64_t current_time,
                                fio_x509_trust_store_s *trust_store);

/**
 * Same as fio_x509_verify_chain, with an explicit Extended Key Usage purpose.
 */
SFUNC int fio_x509_verify_chain_purpose(const fio_ubuf_info_s *certs,
                                        size_t cert_count,
                                        const char *hostname,
                                        int64_t current_time,
                                        fio_x509_trust_store_s *trust_store,
                                        fio_x509_purpose_e purpose);

/**
 * Check if a certificate is trusted by the trust store.
 *
 * Trusted means the certificate is byte-identical to a trust anchor, or a
 * trust anchor whose subject DN equals the certificate's issuer DN verifies
 * the certificate's signature. DN equality alone never establishes trust.
 * No validity, purpose or chain policy is checked; prefer
 * fio_x509_verify_chain for trust decisions.
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
 * The whole message must be consumed exactly (no trailing bytes). A message
 * holding more than `max_entries` certificates is an error (never a silently
 * truncated chain).
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
  case FIO_X509_ERR_KEY_USAGE: return "Key usage forbids this use";
  case FIO_X509_ERR_PATH_LENGTH: return "CA path length exceeded";
  case FIO_X509_ERR_NAME_CONSTRAINTS: return "Name constraints violated";
  case FIO_X509_ERR_CRITICAL_EXTENSION:
    return "Unrecognized critical extension";
  default: return "Unknown error";
  }
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

/**
 * Certificate generation options.
 *
 * All string fields are `fio_buf_info_s` views (non-owning) — a zero `.len`
 * marks the field as absent. There is NO implicit strlen fallback: callers
 * wrapping C strings should use `FIO_BUF_INFO2((char *)s, FIO_STRLEN(s))`.
 */
typedef struct {
  fio_buf_info_s cn;      /**< Subject Common Name (required) */
  fio_buf_info_s org;     /**< Subject Organization (optional) */
  fio_buf_info_s ou;      /**< Subject Organizational Unit (optional) */
  fio_buf_info_s country; /**< Subject Country (optional, 2 chars) */
  int64_t not_before;     /**< Validity start (Unix timestamp, 0 = now) */
  int64_t not_after;      /**< Validity end (Unix timestamp, 0 = +1 year) */
  const fio_buf_info_s *san_dns; /**< SAN DNS names (IP literal: iPAddress) */
  size_t san_dns_count;          /**< Number of SAN DNS entries */
  int is_ca;                     /**< Set CA:TRUE in BasicConstraints */
  uint16_t key_usage;            /**< Key Usage bits (0 = default for type) */
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
 * Call with buf=NULL to get the maximum possible certificate size.
 * This returns a worst-case size that accounts for variable-length DER
 * encoding (e.g., serial numbers with varying leading zeros), ensuring
 * that a buffer of this size will always be sufficient for the actual
 * generation call.
 *
 * @param buf Output buffer (can be NULL to calculate max size)
 * @param buf_len Buffer size (ignored if buf is NULL)
 * @param keypair Key pair to use for signing
 * @param options Certificate options
 * @return Number of bytes written (if buf!=NULL) or max needed (if buf==NULL),
 * or 0 on error
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

/** Securely clear key pair */
FIO_IFUNC void fio_x509_keypair_clear(fio_x509_keypair_s *keypair) {
  if (keypair) {
    fio_secure_zero(keypair->secret_key, sizeof(keypair->secret_key));
    FIO_MEMSET(keypair, 0, sizeof(*keypair));
  }
}

/* *****************************************************************************
Implementation - Possibly Externed Functions
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Implementation - Internal Helpers
***************************************************************************** */

/** Internal: exact raw tag byte match (class, constructed bit and number). */
#define FIO___X509_TAG(e, byte) ((e)->tag == (uint8_t)(byte))

/** Internal: 1 when an iterator consumed its SEQUENCE exactly. */
#define FIO___X509_DONE(it) ((it)->pos == (it)->end)

/** Internal: 1 if the certificate is self-issued (issuer DN == subject DN). */
#define FIO___X509_SELF_ISSUED(c) FIO_UBUF_INFO_IS_EQ((c)->issuer, (c)->subject)

/** Internal: parses the single TLV filling `outer`'s content exactly. */
FIO_IFUNC int fio___x509_unwrap(fio_der_element_s *inner,
                                const fio_der_element_s *outer) {
  return -(!outer->len || fio_der_parse(inner, outer->data, outer->len) !=
                              outer->data + outer->len);
}

/** Internal: non-empty, non-negative INTEGER (sign bit clear). Redundant
 * leading zeros are tolerated: they decode unambiguously. */
FIO_IFUNC int fio___x509_is_uint(const fio_der_element_s *e) {
  return FIO___X509_TAG(e, 0x02) && e->len && !(e->data[0] & 0x80);
}

/** Internal: INTEGER magnitude without leading zero octets (>= 1 octet). */
FIO_IFUNC fio_ubuf_info_s fio___x509_uint_strip(const fio_der_element_s *e) {
  fio_ubuf_info_s r = FIO_UBUF_INFO2((uint8_t *)e->data, e->len);
  while (r.len > 1 && !r.buf[0]) {
    ++r.buf;
    --r.len;
  }
  return r;
}

/** Internal: hash AlgorithmIdentifier (NULL/absent params) -> digest length
 * (32/48/64), or 0 if malformed or unsupported. */
FIO_SFUNC size_t fio___x509_hash_algo_len(const fio_der_element_s *alg) {
  fio_der_iterator_s it;
  fio_der_element_s oid, params;
  if (!FIO___X509_TAG(alg, 0x30))
    return 0;
  fio_der_iterator_init(&it, alg);
  if (fio_der_iterator_next(&it, &oid) || !FIO___X509_TAG(&oid, 0x06))
    return 0;
  if (!FIO___X509_DONE(&it) &&
      (fio_der_iterator_next(&it, &params) || !FIO___X509_TAG(&params, 0x05) ||
       params.len || !FIO___X509_DONE(&it)))
    return 0;
  fio_u128 v = fio___der_oid_value(&oid);
  if (fio___der_oid_eq(v, FIO_X509_OID_SHA256))
    return 32;
  if (fio___der_oid_eq(v, FIO_X509_OID_SHA384))
    return 48;
  if (fio___der_oid_eq(v, FIO_X509_OID_SHA512))
    return 64;
  return 0;
}

/**
 * Internal: parse RSASSA-PSS-params (RFC 4055) following the PSS OID.
 *
 *   RSASSA-PSS-params ::= SEQUENCE {
 *     hashAlgorithm    [0] HashAlgorithm    DEFAULT sha1,
 *     maskGenAlgorithm [1] MaskGenAlgorithm DEFAULT mgf1SHA1,
 *     saltLength       [2] INTEGER          DEFAULT 20,
 *     trailerField     [3] TrailerField     DEFAULT trailerFieldBC }
 *
 * Only the profile fio_rsa_verify_pss implements is accepted: SHA-2 hash, MGF1
 * with the same hash, salt length == hash length, trailer 1. Anything else is
 * UNKNOWN (rejected at verification) - parameters are never assumed.
 */
FIO_SFUNC fio_x509_signature_algo_e
fio___x509_parse_pss_params(fio_der_iterator_s *alg_it) {
  fio_der_element_s params, e, inner, mgf_oid, mgf_hash;
  fio_der_iterator_s it, mgf_it;
  size_t hash_len = 0, mgf_hash_len = 0;
  uint64_t salt = 20, trailer = 1;
  if (fio_der_iterator_next(alg_it, &params) || !FIO___X509_DONE(alg_it) ||
      !FIO___X509_TAG(&params, 0x30))
    return FIO_X509_SIGNATURE_UNKNOWN;
  fio_der_iterator_init(&it, &params);
  int next = fio_der_iterator_next(&it, &e);
  if (!next && FIO___X509_TAG(&e, 0xA0)) {
    if (fio___x509_unwrap(&inner, &e))
      return FIO_X509_SIGNATURE_UNKNOWN;
    hash_len = fio___x509_hash_algo_len(&inner);
    next = fio_der_iterator_next(&it, &e);
  }
  if (!next && FIO___X509_TAG(&e, 0xA1)) {
    /* MaskGenAlgorithm ::= SEQUENCE { mgf1 OID, HashAlgorithm } */
    if (fio___x509_unwrap(&inner, &e) || !FIO___X509_TAG(&inner, 0x30))
      return FIO_X509_SIGNATURE_UNKNOWN;
    fio_der_iterator_init(&mgf_it, &inner);
    if (fio_der_iterator_next(&mgf_it, &mgf_oid) ||
        !FIO___X509_TAG(&mgf_oid, 0x06) ||
        !fio___der_oid_eq(fio___der_oid_value(&mgf_oid), FIO_X509_OID_MGF1) ||
        fio_der_iterator_next(&mgf_it, &mgf_hash) || !FIO___X509_DONE(&mgf_it))
      return FIO_X509_SIGNATURE_UNKNOWN;
    mgf_hash_len = fio___x509_hash_algo_len(&mgf_hash);
    next = fio_der_iterator_next(&it, &e);
  }
  if (!next && FIO___X509_TAG(&e, 0xA2)) {
    if (fio___x509_unwrap(&inner, &e) || fio_der_parse_integer(&inner, &salt))
      return FIO_X509_SIGNATURE_UNKNOWN;
    next = fio_der_iterator_next(&it, &e);
  }
  if (!next && FIO___X509_TAG(&e, 0xA3)) {
    if (fio___x509_unwrap(&inner, &e) ||
        fio_der_parse_integer(&inner, &trailer))
      return FIO_X509_SIGNATURE_UNKNOWN;
    next = fio_der_iterator_next(&it, &e);
  }
  /* defaults (SHA-1) are unsupported; no element may remain */
  if (!next || !FIO___X509_DONE(&it) || !hash_len || mgf_hash_len != hash_len ||
      salt != hash_len || trailer != 1)
    return FIO_X509_SIGNATURE_UNKNOWN;
  switch (hash_len) {
  case 32: return FIO_X509_SIGNATURE_RSA_PSS_SHA256;
  case 48: return FIO_X509_SIGNATURE_RSA_PSS_SHA384;
  default: return FIO_X509_SIGNATURE_RSA_PSS_SHA512;
  }
}

/** Parse signature AlgorithmIdentifier to enum (UNKNOWN if malformed) */
FIO_SFUNC fio_x509_signature_algo_e
fio___x509_parse_signature_algo(const fio_der_element_s *alg_id) {
  if (!alg_id || !FIO___X509_TAG(alg_id, 0x30))
    return FIO_X509_SIGNATURE_UNKNOWN;

  /* AlgorithmIdentifier ::= SEQUENCE { algorithm OID, parameters ANY } */
  fio_der_iterator_s it;
  fio_der_element_s oid, params;

  fio_der_iterator_init(&it, alg_id);
  if (fio_der_iterator_next(&it, &oid) != 0 || !FIO___X509_TAG(&oid, 0x06))
    return FIO_X509_SIGNATURE_UNKNOWN;

  /* Check against known OIDs (build value once, then test constants) */
  fio_u128 oid_val = fio___der_oid_value(&oid);
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_RSA_PSS))
    return fio___x509_parse_pss_params(&it);

  /* Other algorithms: parameters absent or NULL (RFC 4055 / 5758 / 8410) */
  if (!FIO___X509_DONE(&it) &&
      (fio_der_iterator_next(&it, &params) || !FIO___X509_TAG(&params, 0x05) ||
       params.len || !FIO___X509_DONE(&it)))
    return FIO_X509_SIGNATURE_UNKNOWN;

  if (fio___der_oid_eq(oid_val, FIO_X509_OID_SHA256_WITH_RSA))
    return FIO_X509_SIGNATURE_RSA_PKCS1_SHA256;
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_SHA384_WITH_RSA))
    return FIO_X509_SIGNATURE_RSA_PKCS1_SHA384;
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_SHA512_WITH_RSA))
    return FIO_X509_SIGNATURE_RSA_PKCS1_SHA512;
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_ECDSA_WITH_SHA256))
    return FIO_X509_SIGNATURE_ECDSA_SHA256;
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_ECDSA_WITH_SHA384))
    return FIO_X509_SIGNATURE_ECDSA_SHA384;
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_ED25519))
    return FIO_X509_SIGNATURE_ED25519;

  return FIO_X509_SIGNATURE_UNKNOWN;
}

/** Parse SubjectPublicKeyInfo to extract public key */
FIO_SFUNC int fio___x509_parse_pubkey(fio_x509_cert_s *cert,
                                      const fio_der_element_s *spki) {
  if (!cert || !spki)
    return -1;

  /*
   * SubjectPublicKeyInfo ::= SEQUENCE {
   *   algorithm        AlgorithmIdentifier,
   *   subjectPublicKey BIT STRING
   * }
   */
  fio_der_iterator_s it;
  fio_der_element_s alg_id, pubkey_bits;

  fio_der_iterator_init(&it, spki);
  if (fio_der_iterator_next(&it, &alg_id) != 0 ||
      !FIO___X509_TAG(&alg_id, 0x30) ||
      fio_der_iterator_next(&it, &pubkey_bits) != 0 ||
      !FIO___X509_TAG(&pubkey_bits, 0x03) || !FIO___X509_DONE(&it))
    return -1;

  /* Parse the BIT STRING (keys are whole octets: no unused bits allowed) */
  const uint8_t *bits;
  size_t bit_len;
  uint8_t unused_bits;
  if (fio_der_parse_bit_string(&pubkey_bits, &bits, &bit_len, &unused_bits) !=
          0 ||
      unused_bits)
    return -1;

  /* AlgorithmIdentifier ::= SEQUENCE { OID, parameters OPTIONAL } */
  fio_der_iterator_s alg_it;
  fio_der_element_s oid, params;
  int has_params;

  fio_der_iterator_init(&alg_it, &alg_id);
  if (fio_der_iterator_next(&alg_it, &oid) != 0 || !FIO___X509_TAG(&oid, 0x06))
    return -1;
  has_params = !FIO___X509_DONE(&alg_it);
  if (has_params && (fio_der_iterator_next(&alg_it, &params) != 0 ||
                     !FIO___X509_DONE(&alg_it)))
    return -1;

  fio_u128 oid_val = fio___der_oid_value(&oid);
  if (fio___der_oid_eq(oid_val, FIO_X509_OID_RSA_ENCRYPTION)) {
    /* RSA public key - bits contains SEQUENCE { n INTEGER, e INTEGER } */
    if (has_params && (!FIO___X509_TAG(&params, 0x05) || params.len))
      return -1;
    cert->key_algo = FIO_X509_KEY_RSA;

    fio_der_element_s rsa_seq;
    if (!bit_len || fio_der_parse(&rsa_seq, bits, bit_len) != bits + bit_len ||
        !FIO___X509_TAG(&rsa_seq, 0x30))
      return -1;

    fio_der_iterator_s rsa_it;
    fio_der_element_s n_elem, e_elem;

    fio_der_iterator_init(&rsa_it, &rsa_seq);
    if (fio_der_iterator_next(&rsa_it, &n_elem) != 0 ||
        !fio___x509_is_uint(&n_elem) ||
        fio_der_iterator_next(&rsa_it, &e_elem) != 0 ||
        !fio___x509_is_uint(&e_elem) || !FIO___X509_DONE(&rsa_it))
      return -1;

    /* Store magnitudes with every leading zero stripped (the RSA module sizes
     * the modulus from its length, so padding must never reach it) */
    cert->pubkey.rsa.n = fio___x509_uint_strip(&n_elem);
    cert->pubkey.rsa.e = fio___x509_uint_strip(&e_elem);
    /* Match the RSA module: odd n; odd e >= 3 that fits in 64 bits */
    if (!(cert->pubkey.rsa.n.buf[cert->pubkey.rsa.n.len - 1] & 1) ||
        !(cert->pubkey.rsa.e.buf[cert->pubkey.rsa.e.len - 1] & 1) ||
        cert->pubkey.rsa.e.len > 8 ||
        (cert->pubkey.rsa.e.len == 1 && cert->pubkey.rsa.e.buf[0] < 3))
      return -1;

  } else if (fio___der_oid_eq(oid_val, FIO_X509_OID_EC_PUBLIC_KEY)) {
    /* ECDSA - the named curve parameter is required */
    if (!has_params || !FIO___X509_TAG(&params, 0x06))
      return -1;

    fio_u128 curve_val = fio___der_oid_value(&params);
    if (fio___der_oid_eq(curve_val, FIO_X509_OID_SECP256R1)) {
      cert->key_algo = FIO_X509_KEY_ECDSA_P256;
    } else if (fio___der_oid_eq(curve_val, FIO_X509_OID_SECP384R1)) {
      cert->key_algo = FIO_X509_KEY_ECDSA_P384;
    } else {
      return -1; /* Unsupported curve */
    }

    /* EC public key is the SEC 1 point directly in the BIT STRING:
     * uncompressed (04 || x || y) or compressed (02/03 || x), RFC 5480 2.2.
     * The point is decoded and validated (on-curve) when it is used. */
    cert->pubkey.ecdsa.point = FIO_UBUF_INFO2((uint8_t *)bits, bit_len);

  } else if (fio___der_oid_eq(oid_val, FIO_X509_OID_ED25519)) {
    /* Ed25519 - no parameters, 32 byte key directly in BIT STRING (RFC 8410) */
    if (has_params || bit_len != 32)
      return -1;
    cert->key_algo = FIO_X509_KEY_ED25519;
    cert->pubkey.ed25519.key = FIO_UBUF_INFO2((uint8_t *)bits, bit_len);

  } else {
    cert->key_algo = FIO_X509_KEY_UNKNOWN;
  }

  return 0;
}

/** Extract Common Name from a Name sequence */
FIO_SFUNC void fio___x509_extract_cn(fio_x509_cert_s *cert,
                                     const fio_der_element_s *name) {
  if (!cert || !name)
    return;

  /*
   * Name ::= SEQUENCE OF RelativeDistinguishedName
   * RelativeDistinguishedName ::= SET OF AttributeTypeAndValue
   * AttributeTypeAndValue ::= SEQUENCE { type OID, value ANY }
   */
  fio_der_iterator_s name_it;
  fio_der_element_s rdn;

  fio_der_iterator_init(&name_it, name);

  while (fio_der_iterator_next(&name_it, &rdn) == 0) {
    if (!fio_der_is_tag(&rdn, FIO_DER_SET))
      continue;

    fio_der_iterator_s rdn_it;
    fio_der_element_s atv;

    fio_der_iterator_init(&rdn_it, &rdn);

    while (fio_der_iterator_next(&rdn_it, &atv) == 0) {
      if (!fio_der_is_tag(&atv, FIO_DER_SEQUENCE))
        continue;

      fio_der_iterator_s atv_it;
      fio_der_element_s type_oid, value;

      fio_der_iterator_init(&atv_it, &atv);

      if (fio_der_iterator_next(&atv_it, &type_oid) != 0)
        continue;
      if (fio_der_iterator_next(&atv_it, &value) != 0)
        continue;

      /* Check if this is Common Name */
      if (fio___der_oid_eq(fio___der_oid_value(&type_oid),
                           FIO_X509_OID_COMMON_NAME)) {
        size_t len;
        const char *cn = fio_der_parse_string(&value, &len);
        if (cn) {
          cert->cn = FIO_BUF_INFO2((char *)cn, len);
        }
        return;
      }
    }
  }
}

/**
 * Parse X.509v3 extensions. Returns 0 on success, -1 on malformed input.
 *
 * Recognized extensions must be well formed and unique. Unrecognized critical
 * extensions are flagged (FIO_X509_EXT_UNKNOWN_CRITICAL) and rejected by chain
 * validation, so parsing for inspection still succeeds.
 */
FIO_SFUNC int fio___x509_parse_extensions(fio_x509_cert_s *cert,
                                          const fio_der_element_s *exts) {
  /*
   * Extensions ::= SEQUENCE OF Extension
   * Extension ::= SEQUENCE {
   *   extnID      OID,
   *   critical    BOOLEAN DEFAULT FALSE,
   *   extnValue   OCTET STRING (DER-encoded extension value)
   * }
   */
  enum {
    FIO___X509_SEEN_BC = 1,
    FIO___X509_SEEN_KU = 2,
    FIO___X509_SEEN_SAN = 4,
    FIO___X509_SEEN_EKU = 8,
    FIO___X509_SEEN_NC = 16,
  };
  unsigned seen = 0;
  fio_der_iterator_s exts_it;
  fio_der_element_s ext;

  fio_der_iterator_init(&exts_it, exts);

  while (fio_der_iterator_next(&exts_it, &ext) == 0) {
    fio_der_iterator_s ext_it, it;
    fio_der_element_s oid, value, v, e;
    int critical = 0;
    unsigned kind;

    if (!FIO___X509_TAG(&ext, 0x30))
      return -1;
    fio_der_iterator_init(&ext_it, &ext);
    if (fio_der_iterator_next(&ext_it, &oid) != 0 ||
        !FIO___X509_TAG(&oid, 0x06) ||
        fio_der_iterator_next(&ext_it, &value) != 0)
      return -1;
    if (FIO___X509_TAG(&value, 0x01)) {
      if (fio_der_parse_boolean(&value, &critical) ||
          fio_der_iterator_next(&ext_it, &value) != 0)
        return -1;
    }
    if (!FIO___X509_TAG(&value, 0x04) || !FIO___X509_DONE(&ext_it))
      return -1;

    /* Classify (build OID value once per extension) */
    fio_u128 ext_oid = fio___der_oid_value(&oid);
    if (fio___der_oid_eq(ext_oid, FIO_X509_OID_BASIC_CONSTRAINTS))
      kind = FIO___X509_SEEN_BC;
    else if (fio___der_oid_eq(ext_oid, FIO_X509_OID_KEY_USAGE))
      kind = FIO___X509_SEEN_KU;
    else if (fio___der_oid_eq(ext_oid, FIO_X509_OID_SUBJECT_ALT_NAME))
      kind = FIO___X509_SEEN_SAN;
    else if (fio___der_oid_eq(ext_oid, FIO_X509_OID_EXT_KEY_USAGE))
      kind = FIO___X509_SEEN_EKU;
    else if (fio___der_oid_eq(ext_oid, FIO_X509_OID_NAME_CONSTRAINTS))
      kind = FIO___X509_SEEN_NC;
    else {
      /* Informational extensions need no processing even when critical
       * (certificate policies: any policy is acceptable to this validator) */
      if (critical && !fio___der_oid_eq(ext_oid, FIO_X509_OID_SUBJECT_KEY_ID) &&
          !fio___der_oid_eq(ext_oid, FIO_X509_OID_AUTH_KEY_ID) &&
          !fio___der_oid_eq(ext_oid, FIO_X509_OID_CRL_DIST_POINTS) &&
          !fio___der_oid_eq(ext_oid, FIO_X509_OID_CERT_POLICIES))
        cert->ext_flags |= FIO_X509_EXT_UNKNOWN_CRITICAL;
      continue;
    }
    if ((seen & kind) || fio___x509_unwrap(&v, &value))
      return -1; /* duplicate or malformed extnValue */
    seen |= kind;

    switch (kind) {
    case FIO___X509_SEEN_BC: {
      /* BasicConstraints ::= SEQUENCE {
       *   cA BOOLEAN DEFAULT FALSE, pathLenConstraint INTEGER (0..MAX) OPT } */
      if (!FIO___X509_TAG(&v, 0x30))
        return -1;
      fio_der_iterator_init(&it, &v);
      int next = fio_der_iterator_next(&it, &e);
      if (!next && FIO___X509_TAG(&e, 0x01)) {
        int ca_val;
        if (fio_der_parse_boolean(&e, &ca_val))
          return -1;
        cert->is_ca = (uint8_t)ca_val;
        next = fio_der_iterator_next(&it, &e);
      }
      if (!next) {
        uint64_t pl;
        if (!fio___x509_is_uint(&e))
          return -1;
        if (fio_der_parse_integer(&e, &pl))
          pl = 254; /* larger than 64 bits: unlimited in practice */
        if (cert->is_ca)
          cert->path_len = (uint8_t)(pl > 254 ? 254 : pl);
        next = fio_der_iterator_next(&it, &e);
      }
      if (!next || !FIO___X509_DONE(&it))
        return -1;
      break;
    }
    case FIO___X509_SEEN_KU: {
      /* KeyUsage ::= BIT STRING. Padding (unused) bits carry no meaning and
       * are masked off, so set padding bits never grant a usage. */
      const uint8_t *bits;
      size_t bit_len;
      uint8_t unused;
      if (!FIO___X509_TAG(&v, 0x03) ||
          fio_der_parse_bit_string(&v, &bits, &bit_len, &unused))
        return -1;
      cert->has_key_usage = 1;
      if (bit_len >= 1) {
        const uint8_t pad = (uint8_t)((1U << unused) - 1);
        cert->key_usage = (uint8_t)(bits[0] & (bit_len == 1 ? ~pad : 0xFF));
        if (bit_len >= 2)
          cert->key_usage |= (uint16_t)(bits[1] & (bit_len == 2 ? ~pad : 0xFF))
                             << 8;
      }
      break;
    }
    case FIO___X509_SEEN_EKU: {
      /* ExtKeyUsageSyntax ::= SEQUENCE SIZE (1..MAX) OF KeyPurposeId (OID) */
      /* (an empty list is tolerated: it grants no purpose, failing closed) */
      if (!FIO___X509_TAG(&v, 0x30))
        return -1;
      cert->ext_flags |= FIO_X509_EXT_EKU;
      fio_der_iterator_init(&it, &v);
      while (fio_der_iterator_next(&it, &e) == 0) {
        if (!FIO___X509_TAG(&e, 0x06))
          return -1;
        fio_u128 p = fio___der_oid_value(&e);
        if (fio___der_oid_eq(p, FIO_X509_OID_EKU_SERVER_AUTH))
          cert->ext_flags |= FIO_X509_EXT_EKU_SERVER;
        else if (fio___der_oid_eq(p, FIO_X509_OID_EKU_CLIENT_AUTH))
          cert->ext_flags |= FIO_X509_EXT_EKU_CLIENT;
        else if (fio___der_oid_eq(p, FIO_X509_OID_EKU_ANY))
          cert->ext_flags |= FIO_X509_EXT_EKU_ANY;
      }
      if (!FIO___X509_DONE(&it))
        return -1;
      break;
    }
    case FIO___X509_SEEN_SAN: {
      /* SubjectAltName ::= GeneralNames = SEQUENCE SIZE (1..MAX) OF
       * GeneralName ::= CHOICE { dNSName [2] IA5String, iPAddress [7] ... } */
      /* An empty list is tolerated: the extension is still present, so
       * hostname matching never falls back to the CN (and nothing matches) */
      if (!FIO___X509_TAG(&v, 0x30))
        return -1;
      /* Store raw SAN SEQUENCE for later iteration */
      cert->san_ext = FIO_UBUF_INFO2((uint8_t *)value.data, value.len);
      fio_der_iterator_init(&it, &v);
      while (fio_der_iterator_next(&it, &e) == 0) {
        if (FIO___X509_TAG(&e, 0x82) && !cert->san_dns.buf)
          cert->san_dns = FIO_BUF_INFO2((char *)e.data, e.len);
        else if (FIO___X509_TAG(&e, 0x87) && !cert->san_ip.buf)
          cert->san_ip = FIO_UBUF_INFO2((uint8_t *)e.data, e.len);
      }
      if (!FIO___X509_DONE(&it))
        return -1;
      break;
    }
    case FIO___X509_SEEN_NC:
      /* NameConstraints ::= SEQUENCE { permitted [0], excluded [1] } -
       * subtrees are validated when enforced (fio_x509_verify_chain) */
      if (!FIO___X509_TAG(&v, 0x30))
        return -1;
      cert->name_constraints = FIO_UBUF_INFO2((uint8_t *)value.data, value.len);
      break;
    }
  }
  return -(!FIO___X509_DONE(&exts_it));
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
  cert->path_len = FIO_X509_PATH_LEN_NONE;

  /*
   * Certificate ::= SEQUENCE {
   *   tbsCertificate       TBSCertificate,
   *   signatureAlgorithm   AlgorithmIdentifier,
   *   signatureValue       BIT STRING
   * }
   * Inner structures must be consumed exactly (no trailing data / elements).
   * Bytes after the Certificate are not part of it: `cert->der` covers the
   * Certificate TLV only, so callers can detect (or allow) trailing data.
   */
  fio_der_element_s cert_seq;
  const uint8_t *cert_end = fio_der_parse(&cert_seq, der_data, der_len);
  if (!cert_end || !FIO___X509_TAG(&cert_seq, 0x30))
    return -1;
  /* Reference the original DER data (not a copy) */
  cert->der =
      FIO_UBUF_INFO2((uint8_t *)der_data, (size_t)(cert_end - der_data));

  fio_der_iterator_s cert_it;
  fio_der_element_s tbs, signature_algo, sig_value;

  fio_der_iterator_init(&cert_it, &cert_seq);

  /* Parse tbsCertificate (capture TLV start before advancing iterator) */
  const uint8_t *tbs_start = cert_it.pos;
  if (fio_der_iterator_next(&cert_it, &tbs) != 0 || !FIO___X509_TAG(&tbs, 0x30))
    return -1;

  /* Store TBS data (full TLV) for signature verification */
  cert->tbs = FIO_UBUF_INFO2((uint8_t *)tbs_start,
                             (size_t)((tbs.data + tbs.len) - tbs_start));

  /* Parse signatureAlgorithm and signatureValue (BIT STRING), then end */
  if (fio_der_iterator_next(&cert_it, &signature_algo) != 0 ||
      !FIO___X509_TAG(&signature_algo, 0x30) ||
      fio_der_iterator_next(&cert_it, &sig_value) != 0 ||
      !FIO___X509_TAG(&sig_value, 0x03) || !FIO___X509_DONE(&cert_it))
    return -1;

  cert->signature_algo = fio___x509_parse_signature_algo(&signature_algo);

  {
    const uint8_t *bits;
    size_t bit_len;
    uint8_t unused;
    if (fio_der_parse_bit_string(&sig_value, &bits, &bit_len, &unused) != 0 ||
        unused)
      return -1; /* signatures are whole octets */
    cert->signature = FIO_UBUF_INFO2((uint8_t *)bits, bit_len);
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
  fio_der_iterator_s tbs_it;
  fio_der_element_s elem;

  fio_der_iterator_init(&tbs_it, &tbs);

  /* Get first element - may be version or serial */
  if (fio_der_iterator_next(&tbs_it, &elem) != 0)
    return -1;

  /* Check for version (context tag [0]) */
  if (FIO___X509_TAG(&elem, 0xA0)) {
    fio_der_element_s version_int;
    uint64_t ver;
    if (fio___x509_unwrap(&version_int, &elem) ||
        fio_der_parse_integer(&version_int, &ver) || ver > 2)
      return -1;
    cert->version = (uint8_t)ver;
    /* Get next element (serial number) */
    if (fio_der_iterator_next(&tbs_it, &elem) != 0)
      return -1;
  }

  /* serialNumber (INTEGER) */
  if (!FIO___X509_TAG(&elem, 0x02) || !elem.len)
    return -1;
  cert->serial = FIO_UBUF_INFO2((uint8_t *)elem.data, elem.len);

  /* signature (AlgorithmIdentifier) MUST equal signatureAlgorithm: byte
   * equal, or decoding to the same supported algorithm (supported values pin
   * every parameter, so equal values mean identical semantics). Unsupported
   * algorithms require byte equality. */
  if (fio_der_iterator_next(&tbs_it, &elem) != 0 ||
      !FIO___X509_TAG(&elem, 0x30))
    return -1;
  if ((elem.len != signature_algo.len ||
       FIO_MEMCMP(elem.data, signature_algo.data, elem.len)) &&
      (cert->signature_algo == FIO_X509_SIGNATURE_UNKNOWN ||
       fio___x509_parse_signature_algo(&elem) != cert->signature_algo))
    return -1;

  /* Parse issuer (Name = SEQUENCE; capture TLV start before advancing) */
  const uint8_t *issuer_start = tbs_it.pos;
  if (fio_der_iterator_next(&tbs_it, &elem) != 0 ||
      !FIO___X509_TAG(&elem, 0x30))
    return -1;

  /* Store issuer DN (full TLV, including tag and length, for comparison) */
  cert->issuer =
      FIO_UBUF_INFO2((uint8_t *)issuer_start,
                     (size_t)((elem.data + elem.len) - issuer_start));

  /* Parse validity (SEQUENCE { notBefore Time, notAfter Time }) */
  if (fio_der_iterator_next(&tbs_it, &elem) != 0 ||
      !FIO___X509_TAG(&elem, 0x30))
    return -1;
  {
    fio_der_iterator_s val_it;
    fio_der_element_s not_before, not_after;
    fio_der_iterator_init(&val_it, &elem);
    if (fio_der_iterator_next(&val_it, &not_before) != 0 ||
        fio_der_parse_time(&not_before, &cert->not_before) != 0 ||
        fio_der_iterator_next(&val_it, &not_after) != 0 ||
        fio_der_parse_time(&not_after, &cert->not_after) != 0 ||
        !FIO___X509_DONE(&val_it))
      return -1;
  }

  /* Parse subject (Name = SEQUENCE; capture TLV start before advancing) */
  const uint8_t *subject_start = tbs_it.pos;
  if (fio_der_iterator_next(&tbs_it, &elem) != 0 ||
      !FIO___X509_TAG(&elem, 0x30))
    return -1;

  /* Store subject DN (full TLV) */
  cert->subject =
      FIO_UBUF_INFO2((uint8_t *)subject_start,
                     (size_t)((elem.data + elem.len) - subject_start));

  /* Extract Common Name */
  fio___x509_extract_cn(cert, &elem);

  /* Parse subjectPublicKeyInfo (SEQUENCE) */
  if (fio_der_iterator_next(&tbs_it, &elem) != 0 ||
      !FIO___X509_TAG(&elem, 0x30) || fio___x509_parse_pubkey(cert, &elem))
    return -1;

  /* Optional trailing fields, in order: [1] / [2] (v2+), [3] (v3 only) */
  int next = fio_der_iterator_next(&tbs_it, &elem);
  if (!next && FIO___X509_TAG(&elem, 0x81)) {
    if (cert->version < 1)
      return -1;
    next = fio_der_iterator_next(&tbs_it, &elem);
  }
  if (!next && FIO___X509_TAG(&elem, 0x82)) {
    if (cert->version < 1)
      return -1;
    next = fio_der_iterator_next(&tbs_it, &elem);
  }
  if (!next && FIO___X509_TAG(&elem, 0xA3)) {
    fio_der_element_s exts;
    if (cert->version != 2 || fio___x509_unwrap(&exts, &elem) ||
        !FIO___X509_TAG(&exts, 0x30) ||
        fio___x509_parse_extensions(cert, &exts))
      return -1;
    next = fio_der_iterator_next(&tbs_it, &elem);
  }
  if (!next || !FIO___X509_DONE(&tbs_it))
    return -1; /* unexpected element or trailing data */

  return 0;
}

/** Computes the SHA-256 fingerprint of the certificate's DER data. */
SFUNC void fio_x509_fingerprint(fio_x509_cert_s *cert) {
  if (!cert || !cert->der.buf || !cert->der.len)
    return;
  fio_u256 hash = fio_sha256(cert->der.buf, cert->der.len);
  FIO_MEMCPY(cert->fingerprint, hash.u8, 32);
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

/** Internal: strict dotted-quad IPv4 literal (no leading zeros). 0 on success.
 */
FIO_SFUNC int fio___x509_parse_ipv4(uint8_t out[4], const char *s, size_t len) {
  size_t i = 0;
  for (size_t part = 0; part < 4; ++part) {
    if (part) {
      if (i >= len || s[i] != '.')
        return -1;
      ++i;
    }
    size_t start = i;
    unsigned v = 0;
    while (i < len && i - start < 3 && s[i] >= '0' && s[i] <= '9')
      v = v * 10 + (unsigned)(s[i++] - '0');
    if (i == start || v > 255 || (s[start] == '0' && i - start > 1))
      return -1;
    out[part] = (uint8_t)v;
  }
  return -(i != len);
}

/** Internal: IPv4 / IPv6 literal to network order bytes. Returns 4, 16 or 0.
 */
FIO_SFUNC size_t fio___x509_parse_ip(uint8_t out[16],
                                     const char *s,
                                     size_t len) {
  size_t colon = 0;
  while (colon < len && s[colon] != ':')
    ++colon;
  if (colon == len)
    return fio___x509_parse_ipv4(out, s, len) ? 0 : 4;
  /* IPv6 (RFC 4291 2.2): hex groups, at most one "::", optional IPv4 tail */
  uint16_t g[8];
  size_t n = 0, i = 0;
  int gap = -1;
  if (s[0] == ':') {
    if (len < 2 || s[1] != ':')
      return 0;
    gap = 0;
    i = 2;
  }
  while (i < len) {
    size_t j = i;
    unsigned v = 0;
    if (n == 8)
      return 0;
    while (j < len && j - i < 5) {
      unsigned c = (unsigned)(uint8_t)s[j];
      unsigned d = (c - '0' < 10)         ? c - '0'
                   : ((c | 32) - 'a' < 6) ? (c | 32) - 'a' + 10
                                          : 16;
      if (d > 15)
        break;
      v = (v << 4) | d;
      ++j;
    }
    if (j < len && s[j] == '.') { /* embedded IPv4 occupies the last 2 groups */
      uint8_t v4[4];
      if (n > 6 || fio___x509_parse_ipv4(v4, s + i, len - i))
        return 0;
      g[n++] = (uint16_t)((v4[0] << 8) | v4[1]);
      g[n++] = (uint16_t)((v4[2] << 8) | v4[3]);
      break;
    }
    if (j == i || j - i > 4)
      return 0;
    g[n++] = (uint16_t)v;
    i = j;
    if (i == len)
      break;
    if (s[i] != ':' || ++i == len)
      return 0;
    if (s[i] == ':') {
      if (gap >= 0)
        return 0;
      gap = (int)n;
      ++i;
    }
  }
  if (gap < 0 ? n != 8 : n > 7)
    return 0;
  /* expand: groups [0, head) lead, [head, n) end the address, "::" zeros fill
   * the gap. Output indices stay in a fixed 8-group loop (0 <= head <= n <= 8)
   * so the 16-byte bound is obvious (GCC -Wstringop-overflow). */
  const size_t head = gap < 0 ? n : (size_t)gap;
  const size_t tail_at = 8 - (n - head); /* first output group of the tail */
  for (size_t k = 0; k < 8; ++k) {
    uint16_t v = 0;
    if (k < head)
      v = g[k];
    else if (k >= tail_at)
      v = g[(k - tail_at + head) & 7];
    out[k * 2] = (uint8_t)(v >> 8);
    out[k * 2 + 1] = (uint8_t)v;
  }
  return 16;
}

SFUNC int fio_x509_match_hostname(const fio_x509_cert_s *cert,
                                  const char *hostname,
                                  size_t hostname_len) {
  if (!cert || !hostname || hostname_len == 0)
    return -1;

  FIO_LOG_DEBUG2("X.509 hostname match: checking '%.*s'",
                 (int)hostname_len,
                 hostname);

  /* RFC 6125 / 9525: an IP literal matches only iPAddress SAN entries,
   * never DNS names or the CN. */
  uint8_t ip[16];
  const size_t ip_len = fio___x509_parse_ip(ip, hostname, hostname_len);
  const uint8_t san_tag = ip_len ? 0x87 : 0x82;

  /* Per RFC 6125: If SAN extension is present, check ALL SAN entries */
  if (cert->san_ext.buf && cert->san_ext.len > 0) {
    FIO_LOG_DEBUG2("X.509 hostname match: SAN extension present (%zu bytes)",
                   cert->san_ext.len);
    /* Re-parse the SAN extension to iterate through all names */
    fio_der_element_s san_seq;
    if (fio_der_parse(&san_seq, cert->san_ext.buf, cert->san_ext.len) &&
        FIO___X509_TAG(&san_seq, 0x30)) {
      fio_der_iterator_s san_it;
      fio_der_element_s gn;
      fio_der_iterator_init(&san_it, &san_seq);
      while (fio_der_iterator_next(&san_it, &gn) == 0) {
        if (!FIO___X509_TAG(&gn, san_tag))
          continue;
        if (ip_len ? (gn.len == ip_len && !FIO_MEMCMP(gn.data, ip, ip_len))
                   : !fio___x509_match_name((const char *)gn.data,
                                            gn.len,
                                            hostname,
                                            hostname_len)) {
          FIO_LOG_DEBUG2("X.509 hostname match: MATCH FOUND");
          return 0; /* Match found */
        }
      }
    }
    /* SAN extension present but no match found - do NOT fall back to CN */
    FIO_LOG_DEBUG2("X.509 hostname match: no SAN match found");
    return -1;
  }

  /* SAN views set directly (without raw extension data) */
  if (ip_len)
    return -(cert->san_ip.len != ip_len ||
             FIO_MEMCMP(cert->san_ip.buf, ip, ip_len));
  if (cert->san_dns.buf && cert->san_dns.len > 0) {
    /* SAN present - do NOT fall back to CN */
    return fio___x509_match_name(cert->san_dns.buf,
                                 cert->san_dns.len,
                                 hostname,
                                 hostname_len);
  }

  /* No SAN extension present - fall back to Common Name */
  FIO_LOG_DEBUG2("X.509 hostname match: no SAN, checking CN");
  if (cert->cn.buf && cert->cn.len > 0) {
    FIO_LOG_DEBUG2("X.509 hostname match: CN='%.*s'",
                   (int)cert->cn.len,
                   cert->cn.buf);
    return fio___x509_match_name(cert->cn.buf,
                                 cert->cn.len,
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

  if (!cert->tbs.buf || cert->tbs.len == 0)
    return -1;
  if (!cert->signature.buf || cert->signature.len == 0)
    return -1;

  /* Hash the TBS certificate data based on signature algorithm */
  uint8_t hash[64]; /* Max hash size (SHA-512) */
  size_t hash_len = 0;

  switch (cert->signature_algo) {
  case FIO_X509_SIGNATURE_RSA_PKCS1_SHA256:
  case FIO_X509_SIGNATURE_RSA_PSS_SHA256:
  case FIO_X509_SIGNATURE_ECDSA_SHA256: {
    fio_u256 h = fio_sha256(cert->tbs.buf, cert->tbs.len);
    fio_memcpy32(hash, h.u8);
    hash_len = 32;
    break;
  }
  case FIO_X509_SIGNATURE_RSA_PKCS1_SHA384:
  case FIO_X509_SIGNATURE_RSA_PSS_SHA384:
  case FIO_X509_SIGNATURE_ECDSA_SHA384: {
    fio_u512 h = fio_sha384(cert->tbs.buf, cert->tbs.len);
    FIO_MEMCPY(hash, h.u8, 48);
    hash_len = 48;
    break;
  }
  case FIO_X509_SIGNATURE_RSA_PKCS1_SHA512:
  case FIO_X509_SIGNATURE_RSA_PSS_SHA512: {
    fio_u512 h = fio_sha512(cert->tbs.buf, cert->tbs.len);
    fio_memcpy64(hash, h.u8);
    hash_len = 64;
    break;
  }
  case FIO_X509_SIGNATURE_ED25519: {
    /* Ed25519 doesn't pre-hash - the data is hashed internally */
    /* Verification would be done directly with fio_ed25519_verify */
    break;
  }
  default: return -1; /* Unknown algorithm */
  }

  /* Verify signature based on issuer's key type */
  switch (issuer->key_algo) {
  case FIO_X509_KEY_RSA: {
    if (!issuer->pubkey.rsa.n.buf || !issuer->pubkey.rsa.e.buf)
      return -1;

    fio_rsa_pubkey_s rsa_key = {
        .n = issuer->pubkey.rsa.n.buf,
        .n_len = issuer->pubkey.rsa.n.len,
        .e = issuer->pubkey.rsa.e.buf,
        .e_len = issuer->pubkey.rsa.e.len,
    };

    fio_rsa_hash_e rsa_hash;
    switch (hash_len) {
    case 32: rsa_hash = FIO_RSA_HASH_SHA256; break;
    case 48: rsa_hash = FIO_RSA_HASH_SHA384; break;
    case 64: rsa_hash = FIO_RSA_HASH_SHA512; break;
    default: return -1;
    }

    switch (cert->signature_algo) {
    case FIO_X509_SIGNATURE_RSA_PKCS1_SHA256:
    case FIO_X509_SIGNATURE_RSA_PKCS1_SHA384:
    case FIO_X509_SIGNATURE_RSA_PKCS1_SHA512:
      return fio_rsa_verify_pkcs1(cert->signature.buf,
                                  cert->signature.len,
                                  hash,
                                  hash_len,
                                  rsa_hash,
                                  &rsa_key);
    case FIO_X509_SIGNATURE_RSA_PSS_SHA256:
    case FIO_X509_SIGNATURE_RSA_PSS_SHA384:
    case FIO_X509_SIGNATURE_RSA_PSS_SHA512:
      return fio_rsa_verify_pss(cert->signature.buf,
                                cert->signature.len,
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
    if (issuer->key_algo == FIO_X509_KEY_ECDSA_P256) {
      if (!issuer->pubkey.ecdsa.point.buf) /* 65 or 33 bytes: verify */
        return -1;

      /* algorithm must be ECDSA with SHA-256 (no cross-family reuse) */
      if (cert->signature_algo != FIO_X509_SIGNATURE_ECDSA_SHA256)
        return -1;

      return fio_ecdsa_p256_verify(cert->signature.buf,
                                   cert->signature.len,
                                   hash,
                                   issuer->pubkey.ecdsa.point.buf,
                                   issuer->pubkey.ecdsa.point.len);
    }
#endif
#if defined(H___FIO_P384___H)
    /* ECDSA P-384 verification */
    if (issuer->key_algo == FIO_X509_KEY_ECDSA_P384) {
      if (!issuer->pubkey.ecdsa.point.buf) /* 97 or 49 bytes: verify */
        return -1;

      /* algorithm must be ECDSA with SHA-384 (no cross-family reuse) */
      if (cert->signature_algo != FIO_X509_SIGNATURE_ECDSA_SHA384)
        return -1;

      return fio_ecdsa_p384_verify(cert->signature.buf,
                                   cert->signature.len,
                                   hash,
                                   issuer->pubkey.ecdsa.point.buf,
                                   issuer->pubkey.ecdsa.point.len);
    }
#endif
    /* Unsupported ECDSA curve */
    return -1;
  }

  case FIO_X509_KEY_ED25519: {
#if defined(H___FIO_ED25519___H)
    /* Ed25519 verification */
    if (!issuer->pubkey.ed25519.key.buf ||
        cert->signature_algo != FIO_X509_SIGNATURE_ED25519)
      return -1;

    /* Ed25519 signature should be 64 bytes */
    if (cert->signature.len != 64)
      return -1;

    return fio_ed25519_verify(cert->signature.buf,
                              cert->tbs.buf,
                              cert->tbs.len,
                              issuer->pubkey.ed25519.key.buf);
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

/**
 * Internal: find a trust anchor for `cert`: an anchor byte-identical to
 * `cert`, or one whose subject DN equals `cert`'s issuer DN AND whose key
 * verifies `cert`'s signature. Every same-DN anchor is tried (key rollover).
 *
 * Returns FIO_X509_OK (anchor parsed into `anchor`), FIO_X509_ERR_SIGNATURE
 * (DN matched, no key verified) or FIO_X509_ERR_NO_TRUST_ANCHOR.
 */
FIO_SFUNC int fio___x509_find_anchor(fio_x509_cert_s *anchor,
                                     const fio_x509_cert_s *cert,
                                     fio_x509_trust_store_s *trust_store) {
  int r = FIO_X509_ERR_NO_TRUST_ANCHOR;
  if (!anchor || !cert || !trust_store || !trust_store->roots ||
      !trust_store->root_lens)
    return r;

  for (size_t i = 0; i < trust_store->root_count; ++i) {
    const uint8_t *der = trust_store->roots[i];
    const size_t len = trust_store->root_lens[i];
    if (!der || !len || fio_x509_parse(anchor, der, len) != 0)
      continue;
    if (FIO_UBUF_INFO_IS_EQ(anchor->der, cert->der))
      return FIO_X509_OK; /* the certificate itself is a trust anchor */
    if (!FIO_UBUF_INFO_IS_EQ(cert->issuer, anchor->subject))
      continue;
    if (!fio_x509_verify_signature(cert, anchor))
      return FIO_X509_OK;
    r = FIO_X509_ERR_SIGNATURE;
  }
  return r;
}

SFUNC int fio_x509_is_trusted(const fio_x509_cert_s *cert,
                              fio_x509_trust_store_s *trust_store) {
  fio_x509_cert_s anchor;
  return -(fio___x509_find_anchor(&anchor, cert, trust_store) != FIO_X509_OK);
}

/** Internal: may `ca` issue certificates? (RFC 5280 6.1.4 (k), (n)) */
FIO_SFUNC int fio___x509_check_issuer(const fio_x509_cert_s *ca) {
  /* legacy v1/v2 self-issued roots carry no Basic Constraints */
  if (!ca->is_ca && (ca->version >= 2 || !FIO___X509_SELF_ISSUED(ca)))
    return FIO_X509_ERR_NOT_CA;
  if (ca->has_key_usage && !(ca->key_usage & FIO_X509_KU_KEY_CERT_SIGN))
    return FIO_X509_ERR_KEY_USAGE;
  return FIO_X509_OK;
}

/** Internal: Extended Key Usage permits `purpose`? (absent EKU = any) */
FIO_IFUNC int fio___x509_eku_allows(const fio_x509_cert_s *c,
                                    fio_x509_purpose_e purpose) {
  uint8_t need = (purpose == FIO_X509_PURPOSE_SERVER) ? FIO_X509_EXT_EKU_SERVER
                 : (purpose == FIO_X509_PURPOSE_CLIENT)
                     ? FIO_X509_EXT_EKU_CLIENT
                     : (FIO_X509_EXT_EKU_SERVER | FIO_X509_EXT_EKU_CLIENT);
  need |= FIO_X509_EXT_EKU_ANY;
  return !(c->ext_flags & FIO_X509_EXT_EKU) || (c->ext_flags & need);
}

/** Internal: DNS name within a dNSName subtree (RFC 5280 4.2.1.10). */
FIO_SFUNC int fio___x509_nc_dns_match(const uint8_t *base,
                                      size_t blen,
                                      const uint8_t *name,
                                      size_t nlen) {
  if (!blen)
    return 1; /* empty constraint matches every name */
  if (blen > nlen)
    return 0;
  const uint8_t *tail = name + (nlen - blen);
  for (size_t i = 0; i < blen; ++i)
    if (!fio___x509_char_eq_nocase((char)tail[i], (char)base[i]))
      return 0;
  /* "example.com" covers itself and "*.example.com"; ".example.com" only
   * subdomains (common practice) */
  return blen == nlen || base[0] == '.' || tail[-1] == '.';
}

/** Internal: GeneralName types handled by Name Constraints */
#define FIO___X509_GN_DNS 0x82
#define FIO___X509_GN_IP  0x87
#define FIO___X509_GN_DN  0xA4

/**
 * Internal: does a name of GeneralName tag `type` satisfy NameConstraints?
 * `name` is the dNSName text, iPAddress octets, or Name SEQUENCE content.
 *
 * Supported: dNSName and iPAddress subtrees, permitted directoryName subtrees
 * (raw DER RDN prefix: encoding mismatches fail closed). Any other form, a
 * non-zero minimum, any maximum, or malformed data fail closed (-1).
 */
FIO_SFUNC int fio___x509_nc_check_name(const fio_der_element_s *nc,
                                       uint8_t type,
                                       const uint8_t *name,
                                       size_t len) {
  fio_der_iterator_s it, sub_it, gs_it;
  fio_der_element_s subtrees, gs, base, dn;
  int has_permitted = 0, permitted = 0;
  fio_der_iterator_init(&it, nc);
  while (fio_der_iterator_next(&it, &subtrees) == 0) {
    const int excluded = FIO___X509_TAG(&subtrees, 0xA1);
    if (!excluded && !FIO___X509_TAG(&subtrees, 0xA0))
      return -1;
    fio_der_iterator_init(&sub_it, &subtrees);
    while (fio_der_iterator_next(&sub_it, &gs) == 0) {
      /* GeneralSubtree ::= SEQUENCE { base, minimum [0] DEFAULT 0,
       *                               maximum [1] OPTIONAL } - MUST be bare */
      if (!FIO___X509_TAG(&gs, 0x30))
        return -1;
      fio_der_iterator_init(&gs_it, &gs);
      if (fio_der_iterator_next(&gs_it, &base))
        return -1;
      if (!FIO___X509_DONE(&gs_it)) { /* only an explicit default minimum 0 */
        fio_der_element_s min;
        if (fio_der_iterator_next(&gs_it, &min) ||
            !FIO___X509_TAG(&min, 0x80) || !min.len || !FIO___X509_DONE(&gs_it))
          return -1;
        for (size_t i = 0; i < min.len; ++i)
          if (min.data[i])
            return -1;
      }
      int m;
      switch (base.tag) {
      case FIO___X509_GN_DNS:
        if (type != base.tag)
          continue;
        m = fio___x509_nc_dns_match(base.data, base.len, name, len);
        break;
      case FIO___X509_GN_IP: /* address || mask (8 or 32 octets) */
        if (base.len != 8 && base.len != 32)
          return -1;
        if (type != base.tag)
          continue;
        m = (base.len == len * 2);
        for (size_t i = 0; m && i < len; ++i)
          m = !((name[i] ^ base.data[i]) & base.data[len + i]);
        break;
      case FIO___X509_GN_DN:
        if (excluded || fio___x509_unwrap(&dn, &base) ||
            !FIO___X509_TAG(&dn, 0x30))
          return -1; /* excluded DN needs RFC 4518 matching: unsupported */
        if (type != base.tag)
          continue;
        m = dn.len <= len && !FIO_MEMCMP(dn.data, name, dn.len);
        break;
      default: return -1; /* unsupported constraint form: fail closed */
      }
      if (excluded) {
        if (m)
          return -1;
      } else {
        has_permitted = 1;
        permitted |= m;
      }
    }
    if (!FIO___X509_DONE(&sub_it))
      return -1;
  }
  if (!FIO___X509_DONE(&it))
    return -1;
  return -(has_permitted && !permitted);
}

/** Internal: apply `ca`'s Name Constraints to the names of `c`. */
FIO_SFUNC int fio___x509_nc_check_cert(const fio_x509_cert_s *ca,
                                       const fio_x509_cert_s *c,
                                       const char *hostname,
                                       size_t hostname_len) {
  fio_der_element_s nc, e, dn;
  fio_der_iterator_s it;
  if (!ca->name_constraints.buf)
    return 0;
  if (!fio_der_parse(&nc, ca->name_constraints.buf, ca->name_constraints.len))
    return -1;
  /* subject DN (when not empty) */
  if (fio_der_parse(&dn, c->subject.buf, c->subject.len) && dn.len &&
      fio___x509_nc_check_name(&nc, FIO___X509_GN_DN, dn.data, dn.len))
    return -1;
  /* every SAN entry of a constrained type */
  if (c->san_ext.buf && fio_der_parse(&e, c->san_ext.buf, c->san_ext.len)) {
    fio_der_iterator_init(&it, &e);
    while (fio_der_iterator_next(&it, &e) == 0) {
      const uint8_t *name = e.data;
      size_t len = e.len;
      if (FIO___X509_TAG(&e, FIO___X509_GN_DN)) {
        if (fio___x509_unwrap(&dn, &e))
          return -1;
        name = dn.data;
        len = dn.len;
      } else if (!FIO___X509_TAG(&e, FIO___X509_GN_DNS) &&
                 !FIO___X509_TAG(&e, FIO___X509_GN_IP))
        continue;
      if (fio___x509_nc_check_name(&nc, e.tag, name, len))
        return -1;
    }
  }
  /* the identity actually matched (covers wildcard SANs and CN fallback) */
  if (hostname) {
    uint8_t ip[16];
    size_t ip_len = fio___x509_parse_ip(ip, hostname, hostname_len);
    if (ip_len ? fio___x509_nc_check_name(&nc, FIO___X509_GN_IP, ip, ip_len)
               : fio___x509_nc_check_name(&nc,
                                          FIO___X509_GN_DNS,
                                          (const uint8_t *)hostname,
                                          hostname_len))
      return -1;
  }
  return 0;
}

/** Internal: apply `ca`'s Name Constraints to chain[0..below). */
FIO_SFUNC int fio___x509_nc_check_chain(const fio_x509_cert_s *ca,
                                        const fio_x509_cert_s *chain,
                                        size_t below,
                                        const char *hostname,
                                        size_t hostname_len) {
  if (!ca->name_constraints.buf)
    return 0;
  for (size_t k = 0; k < below; ++k) {
    /* self-issued intermediates are exempt (RFC 5280 6.1.3 (b)) */
    if (k && FIO___X509_SELF_ISSUED(chain + k))
      continue;
    if (fio___x509_nc_check_cert(ca,
                                 chain + k,
                                 k ? NULL : hostname,
                                 hostname_len))
      return FIO_X509_ERR_NAME_CONSTRAINTS;
  }
  return 0;
}

SFUNC int fio_x509_verify_chain(const fio_ubuf_info_s *certs,
                                size_t cert_count,
                                const char *hostname,
                                int64_t current_time,
                                fio_x509_trust_store_s *trust_store) {
  return fio_x509_verify_chain_purpose(certs,
                                       cert_count,
                                       hostname,
                                       current_time,
                                       trust_store,
                                       hostname ? FIO_X509_PURPOSE_SERVER
                                                : FIO_X509_PURPOSE_TLS);
}

SFUNC int fio_x509_verify_chain_purpose(const fio_ubuf_info_s *certs,
                                        size_t cert_count,
                                        const char *hostname,
                                        int64_t current_time,
                                        fio_x509_trust_store_s *trust_store,
                                        fio_x509_purpose_e purpose) {
  int r;
  size_t hostname_len = 0;
  /* Validate inputs */
  if (!certs)
    return FIO_X509_ERR_PARSE;
  if (cert_count == 0)
    return FIO_X509_ERR_EMPTY_CHAIN;
  if (cert_count > FIO_X509_MAX_CHAIN_DEPTH)
    return FIO_X509_ERR_CHAIN_TOO_LONG;

  /* Parse all certificates in the chain */
  fio_x509_cert_s chain[FIO_X509_MAX_CHAIN_DEPTH];

  for (size_t i = 0; i < cert_count; ++i) {
    if (!certs[i].buf || certs[i].len == 0) {
      FIO_LOG_DEBUG("X.509 chain: certificate %zu is empty", i);
      return FIO_X509_ERR_PARSE;
    }

    if (fio_x509_parse(&chain[i], certs[i].buf, certs[i].len) != 0) {
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

    /* RFC 5280 4.2: unrecognized critical extensions MUST be rejected */
    if (chain[i].ext_flags & FIO_X509_EXT_UNKNOWN_CRITICAL) {
      FIO_LOG_DEBUG("X.509 chain: certificate %zu has unknown critical ext.",
                    i);
      return FIO_X509_ERR_CRITICAL_EXTENSION;
    }

    /* Extended Key Usage must allow the purpose (leaf and CAs) */
    if (!fio___x509_eku_allows(&chain[i], purpose)) {
      FIO_LOG_DEBUG("X.509 chain: certificate %zu EKU forbids purpose", i);
      return FIO_X509_ERR_KEY_USAGE;
    }
  }

  /* TLS 1.3 signs with the leaf key (RFC 8446 4.4.2.2) */
  if (chain[0].has_key_usage &&
      !(chain[0].key_usage & FIO_X509_KU_DIGITAL_SIGNATURE)) {
    FIO_LOG_DEBUG("X.509 chain: leaf lacks digitalSignature key usage");
    return FIO_X509_ERR_KEY_USAGE;
  }

  /* Check hostname on end-entity certificate (index 0) */
  if (hostname) {
    hostname_len = FIO_STRLEN(hostname);
    if (fio_x509_match_hostname(&chain[0], hostname, hostname_len) != 0) {
      FIO_LOG_DEBUG("X.509 chain: hostname mismatch for '%s'", hostname);
      return FIO_X509_ERR_HOSTNAME_MISMATCH;
    }
  }

  /* Verify chain signatures, issuer relationships and CA constraints */
  size_t intermediates = 0; /* non-self-issued CAs below the current issuer */
  for (size_t i = 0; i < cert_count - 1; ++i) {
    const fio_x509_cert_s *issuer = chain + i + 1;
    /* Verify issuer DN of cert[i] matches subject DN of cert[i+1] */
    if (!FIO_UBUF_INFO_IS_EQ(chain[i].issuer, issuer->subject)) {
      FIO_LOG_DEBUG("X.509 chain: issuer mismatch at position %zu", i);
      return FIO_X509_ERR_ISSUER_MISMATCH;
    }

    /* The issuer must be a CA allowed to sign certificates */
    if ((r = fio___x509_check_issuer(issuer))) {
      FIO_LOG_DEBUG("X.509 chain: certificate %zu may not issue", i + 1);
      return r;
    }

    /* pathLenConstraint: max non-self-issued intermediates below issuer */
    if (i && !FIO___X509_SELF_ISSUED(chain + i))
      ++intermediates;
    if (issuer->path_len != FIO_X509_PATH_LEN_NONE &&
        intermediates > issuer->path_len) {
      FIO_LOG_DEBUG("X.509 chain: path length exceeded at %zu", i + 1);
      return FIO_X509_ERR_PATH_LENGTH;
    }

    /* Name Constraints apply to every certificate below the issuer */
    if ((r = fio___x509_nc_check_chain(issuer,
                                       chain,
                                       i + 1,
                                       hostname,
                                       hostname_len)))
      return r;

    /* Verify signature of cert[i] using public key from cert[i+1] */
    if (fio_x509_verify_signature(&chain[i], issuer) != 0) {
      FIO_LOG_DEBUG("X.509 chain: signature verification failed at %zu", i);
      return FIO_X509_ERR_SIGNATURE;
    }
  }

  /* Handle the last certificate in the chain */
  size_t last = cert_count - 1;

  /* Check if the last certificate is self-signed */
  if (FIO___X509_SELF_ISSUED(chain + last)) {
    /* Self-signed: verify its own signature (well-formedness check) */
    if (fio_x509_verify_signature(&chain[last], &chain[last]) != 0) {
      FIO_LOG_DEBUG("X.509 chain: self-signed root signature invalid");
      return FIO_X509_ERR_SIGNATURE;
    }
  }

  /* The last certificate must chain to a trust anchor.
   *
   * A trust anchor is identified by its (subject DN, public key) pair, so
   * the anchor's public key MUST verify the certificate's signature.  For a
   * self-signed certificate this proves the certificate IS the anchor (same
   * key) — DN matching alone would allow an impostor to mint a self-signed
   * certificate copying a trusted anchor's name. */
  if (trust_store) {
    fio_x509_cert_s anchor;
    if ((r = fio___x509_find_anchor(&anchor, &chain[last], trust_store))) {
      FIO_LOG_DEBUG("X.509 chain: no trust anchor verified the chain");
      return r;
    }
    /* anchor constraints apply below the anchor (the chain's own CAs were
     * already applied above when the anchor is chain[last] itself) */
    if ((anchor.der.len != chain[last].der.len ||
         FIO_MEMCMP(anchor.der.buf, chain[last].der.buf, anchor.der.len)) &&
        (r = fio___x509_nc_check_chain(&anchor,
                                       chain,
                                       cert_count,
                                       hostname,
                                       hostname_len)))
      return r;
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
   *
   * Bounds use remaining-length arithmetic (no pointer overflow).
   */
  const uint8_t *p = data;
  size_t rem = data_len;

  /* certificate_request_context (1 byte length) */
  size_t ctx_len = *p++;
  --rem;
  if (ctx_len > rem)
    return FIO_TLS_CERT_PARSE_ERROR;
  p += ctx_len;
  rem -= ctx_len;

  /* certificate_list must fill the rest of the message exactly */
  if (rem < 3)
    return FIO_TLS_CERT_PARSE_ERROR;
  size_t list_len = ((size_t)p[0] << 16) | ((size_t)p[1] << 8) | p[2];
  p += 3;
  rem -= 3;
  if (list_len != rem)
    return FIO_TLS_CERT_PARSE_ERROR;

  size_t count = 0;
  while (rem) {
    /* cert_data length (3 bytes, big-endian) */
    if (rem < 3)
      return FIO_TLS_CERT_PARSE_ERROR;
    size_t cert_len = ((size_t)p[0] << 16) | ((size_t)p[1] << 8) | p[2];
    p += 3;
    rem -= 3;
    if (cert_len == 0 || cert_len > rem)
      return FIO_TLS_CERT_PARSE_ERROR;
    /* never return a silently truncated chain */
    if (count == max_entries)
      return FIO_TLS_CERT_PARSE_ERROR;
    entries[count].cert = p;
    entries[count].cert_len = cert_len;
    ++count;
    p += cert_len;
    rem -= cert_len;

    /* extensions length (2 bytes, big-endian) */
    if (rem < 2)
      return FIO_TLS_CERT_PARSE_ERROR;
    size_t ext_len = ((size_t)p[0] << 8) | p[1];
    p += 2;
    rem -= 2;
    if (ext_len > rem)
      return FIO_TLS_CERT_PARSE_ERROR;
    p += ext_len;
    rem -= ext_len;
  }

  return count;
}

/* *****************************************************************************
Implementation - Certificate Generation
***************************************************************************** */

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
                                       fio_u128 oid,
                                       fio_buf_info_s value) {
  if (!value.buf || value.len == 0)
    return 0;

  /* AttributeTypeAndValue ::= SEQUENCE { type OID, value ANY } */
  size_t oid_len = fio___der_encode_oid(NULL, oid);
  size_t str_len = fio_der_encode_utf8_string(NULL, value.buf, value.len);
  size_t atv_content = oid_len + str_len;
  size_t atv_len =
      fio_der_encode_sequence_header(NULL, atv_content) + atv_content;

  /* SET { AttributeTypeAndValue } */
  size_t set_len = fio_der_encode_set_header(NULL, atv_len) + atv_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_set_header(buf + offset, atv_len);
    offset += fio_der_encode_sequence_header(buf + offset, atv_content);
    offset += fio___der_encode_oid(buf + offset, oid);
    offset += fio_der_encode_utf8_string(buf + offset, value.buf, value.len);
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
  if (opts->country.len)
    content_len +=
        fio___x509_encode_rdn(NULL, FIO_X509_OID_COUNTRY, opts->country);

  /* Organization (O) */
  if (opts->org.len)
    content_len +=
        fio___x509_encode_rdn(NULL, FIO_X509_OID_ORGANIZATION, opts->org);

  /* Organizational Unit (OU) */
  if (opts->ou.len)
    content_len += fio___x509_encode_rdn(NULL, FIO_X509_OID_ORG_UNIT, opts->ou);

  /* Common Name (CN) - required */
  content_len +=
      fio___x509_encode_rdn(NULL, FIO_X509_OID_COMMON_NAME, opts->cn);

  size_t total =
      fio_der_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_sequence_header(buf + offset, content_len);

    if (opts->country.len)
      offset += fio___x509_encode_rdn(buf + offset,
                                      FIO_X509_OID_COUNTRY,
                                      opts->country);

    if (opts->org.len)
      offset += fio___x509_encode_rdn(buf + offset,
                                      FIO_X509_OID_ORGANIZATION,
                                      opts->org);

    if (opts->ou.len)
      offset +=
          fio___x509_encode_rdn(buf + offset, FIO_X509_OID_ORG_UNIT, opts->ou);

    offset +=
        fio___x509_encode_rdn(buf + offset, FIO_X509_OID_COMMON_NAME, opts->cn);
    (void)offset;
  }

  return total;
}

/** Helper: encode a validity Time (RFC 5280 4.1.2.5: UTCTime through 2049,
 * GeneralizedTime otherwise). Returns 0 if unrepresentable. */
FIO_SFUNC size_t fio___x509_encode_time(uint8_t *buf, int64_t t) {
  size_t len = fio_der_encode_utc_time(NULL, t);
  if (len)
    return buf ? fio_der_encode_utc_time(buf, t) : len;
  return fio_der_encode_generalized_time(buf, t);
}

/** Helper: encode Validity */
FIO_SFUNC size_t fio___x509_encode_validity(uint8_t *buf,
                                            int64_t not_before,
                                            int64_t not_after) {
  size_t nb_len = fio___x509_encode_time(NULL, not_before);
  size_t na_len = fio___x509_encode_time(NULL, not_after);
  if (!nb_len || !na_len)
    return 0;
  size_t content_len = nb_len + na_len;
  size_t total =
      fio_der_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_sequence_header(buf + offset, content_len);
    offset += fio___x509_encode_time(buf + offset, not_before);
    offset += fio___x509_encode_time(buf + offset, not_after);
    (void)offset;
  }

  return total;
}

/** Helper: encode SubjectPublicKeyInfo for Ed25519 */
FIO_SFUNC size_t fio___x509_encode_spki_ed25519(uint8_t *buf,
                                                const uint8_t pubkey[32]) {
  /* AlgorithmIdentifier for Ed25519: SEQUENCE { OID } (no params) */
  size_t oid_len = fio___der_encode_oid(NULL, FIO_X509_OID_ED25519);
  size_t alg_content = oid_len;
  size_t alg_len =
      fio_der_encode_sequence_header(NULL, alg_content) + alg_content;

  /* BIT STRING containing 32-byte public key */
  size_t bits_len = fio_der_encode_bit_string(NULL, pubkey, 32, 0);

  size_t content_len = alg_len + bits_len;
  size_t total =
      fio_der_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_sequence_header(buf + offset, content_len);
    offset += fio_der_encode_sequence_header(buf + offset, alg_content);
    offset += fio___der_encode_oid(buf + offset, FIO_X509_OID_ED25519);
    offset += fio_der_encode_bit_string(buf + offset, pubkey, 32, 0);
    (void)offset;
  }

  return total;
}

/** Helper: encode SubjectPublicKeyInfo for P-256 */
FIO_SFUNC size_t fio___x509_encode_spki_p256(uint8_t *buf,
                                             const uint8_t pubkey[65]) {
  /* AlgorithmIdentifier: SEQUENCE { ecPublicKey OID, secp256r1 OID } */
  size_t ec_oid_len = fio___der_encode_oid(NULL, FIO_X509_OID_EC_PUBLIC_KEY);
  size_t curve_oid_len = fio___der_encode_oid(NULL, FIO_X509_OID_SECP256R1);
  size_t alg_content = ec_oid_len + curve_oid_len;
  size_t alg_len =
      fio_der_encode_sequence_header(NULL, alg_content) + alg_content;

  /* BIT STRING containing 65-byte uncompressed point */
  size_t bits_len = fio_der_encode_bit_string(NULL, pubkey, 65, 0);

  size_t content_len = alg_len + bits_len;
  size_t total =
      fio_der_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_sequence_header(buf + offset, content_len);
    offset += fio_der_encode_sequence_header(buf + offset, alg_content);
    offset += fio___der_encode_oid(buf + offset, FIO_X509_OID_EC_PUBLIC_KEY);
    offset += fio___der_encode_oid(buf + offset, FIO_X509_OID_SECP256R1);
    offset += fio_der_encode_bit_string(buf + offset, pubkey, 65, 0);
    (void)offset;
  }

  return total;
}

/** Helper: encode BasicConstraints extension */
FIO_SFUNC size_t fio___x509_encode_ext_basic_constraints(uint8_t *buf,
                                                         int is_ca) {
  /* BasicConstraints ::= SEQUENCE { cA BOOLEAN DEFAULT FALSE } */
  size_t bc_content = is_ca ? fio_der_encode_boolean(NULL, 1) : 0;
  size_t bc_len = fio_der_encode_sequence_header(NULL, bc_content) + bc_content;

  /* Wrap in OCTET STRING */
  size_t octet_len = fio_der_encode_octet_string(NULL, NULL, bc_len);

  /* Extension: SEQUENCE { OID, critical BOOLEAN, value OCTET STRING } */
  size_t oid_len = fio___der_encode_oid(NULL, FIO_X509_OID_BASIC_CONSTRAINTS);
  size_t crit_len = fio_der_encode_boolean(NULL, 1); /* critical = TRUE */
  size_t ext_content = oid_len + crit_len + octet_len;
  size_t total =
      fio_der_encode_sequence_header(NULL, ext_content) + ext_content;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_sequence_header(buf + offset, ext_content);
    offset +=
        fio___der_encode_oid(buf + offset, FIO_X509_OID_BASIC_CONSTRAINTS);
    offset += fio_der_encode_boolean(buf + offset, 1); /* critical */

    /* OCTET STRING containing BasicConstraints */
    size_t octet_hdr = fio_der_encode_octet_string(buf + offset, NULL, bc_len);
    /* Write the actual BasicConstraints content */
    uint8_t *bc_buf = buf + offset + octet_hdr - bc_len;
    size_t bc_off = 0;
    bc_off += fio_der_encode_sequence_header(bc_buf + bc_off, bc_content);
    if (is_ca)
      bc_off += fio_der_encode_boolean(bc_buf + bc_off, 1);
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

  size_t bits_len = fio_der_encode_bit_string(NULL, ku_bytes, ku_len, unused);

  /* Wrap in OCTET STRING */
  size_t octet_len = fio_der_encode_octet_string(NULL, NULL, bits_len);

  /* Extension: SEQUENCE { OID, critical BOOLEAN, value OCTET STRING } */
  size_t oid_len = fio___der_encode_oid(NULL, FIO_X509_OID_KEY_USAGE);
  size_t crit_len = fio_der_encode_boolean(NULL, 1); /* critical = TRUE */
  size_t ext_content = oid_len + crit_len + octet_len;
  size_t total =
      fio_der_encode_sequence_header(NULL, ext_content) + ext_content;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_sequence_header(buf + offset, ext_content);
    offset += fio___der_encode_oid(buf + offset, FIO_X509_OID_KEY_USAGE);
    offset += fio_der_encode_boolean(buf + offset, 1); /* critical */

    /* OCTET STRING containing KeyUsage BIT STRING */
    size_t octet_hdr =
        fio_der_encode_octet_string(buf + offset, NULL, bits_len);
    uint8_t *ku_buf = buf + offset + octet_hdr - bits_len;
    fio_der_encode_bit_string(ku_buf, ku_bytes, ku_len, unused);
    offset += octet_hdr;
    (void)offset;
  }

  return total;
}

/** Helper: encode SubjectAltName extension */
FIO_SFUNC size_t fio___x509_encode_ext_san(uint8_t *buf,
                                           const fio_buf_info_s *dns_names,
                                           size_t dns_count) {
  if (!dns_names || dns_count == 0)
    return 0;

  /* GeneralNames ::= SEQUENCE OF GeneralName
   * GeneralName ::= CHOICE { dNSName [2] IA5String, iPAddress [7] OCTET } */
  uint8_t ip[16];
  size_t san_content = 0;
  for (size_t i = 0; i < dns_count; ++i) {
    size_t name_len =
        fio___x509_parse_ip(ip, dns_names[i].buf, dns_names[i].len);
    if (!name_len)
      name_len = dns_names[i].len;
    /* Context tag [2] or [7] + length + data */
    san_content +=
        fio_der_encode_context_header(NULL, 2, name_len, 0) + name_len;
  }

  size_t san_len =
      fio_der_encode_sequence_header(NULL, san_content) + san_content;

  /* Wrap in OCTET STRING */
  size_t octet_len = fio_der_encode_octet_string(NULL, NULL, san_len);

  /* Extension: SEQUENCE { OID, value OCTET STRING } (not critical) */
  size_t oid_len = fio___der_encode_oid(NULL, FIO_X509_OID_SUBJECT_ALT_NAME);
  size_t ext_content = oid_len + octet_len;
  size_t total =
      fio_der_encode_sequence_header(NULL, ext_content) + ext_content;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_sequence_header(buf + offset, ext_content);
    offset += fio___der_encode_oid(buf + offset, FIO_X509_OID_SUBJECT_ALT_NAME);

    /* OCTET STRING containing SAN */
    size_t octet_hdr = fio_der_encode_octet_string(buf + offset, NULL, san_len);
    uint8_t *san_buf = buf + offset + octet_hdr - san_len;
    size_t san_off = 0;
    san_off += fio_der_encode_sequence_header(san_buf + san_off, san_content);
    for (size_t i = 0; i < dns_count; ++i) {
      size_t ip_len =
          fio___x509_parse_ip(ip, dns_names[i].buf, dns_names[i].len);
      size_t name_len = ip_len ? ip_len : dns_names[i].len;
      san_off += fio_der_encode_context_header(san_buf + san_off,
                                               ip_len ? 7 : 2,
                                               name_len,
                                               0);
      FIO_MEMCPY(san_buf + san_off,
                 ip_len ? (const void *)ip : (const void *)dns_names[i].buf,
                 name_len);
      san_off += name_len;
    }
    offset += octet_hdr;
    (void)offset;
  }

  return total;
}

/** Helper: encode Extended Key Usage extension (serverAuth + clientAuth) */
FIO_SFUNC size_t fio___x509_encode_ext_eku(uint8_t *buf, int is_ca) {
  /* For CA certificates, we don't add EKU (it's for end-entity certs) */
  if (is_ca)
    return 0;

  /*
   * ExtKeyUsageSyntax ::= SEQUENCE SIZE (1..MAX) OF KeyPurposeId
   * KeyPurposeId ::= OBJECT IDENTIFIER
   *
   * serverAuth and clientAuth, so the certificate serves TLS servers and
   * mutual-TLS clients alike.
   */
  size_t eku_content =
      fio___der_encode_oid(NULL, FIO_X509_OID_EKU_SERVER_AUTH) +
      fio___der_encode_oid(NULL, FIO_X509_OID_EKU_CLIENT_AUTH);
  size_t eku_len =
      fio_der_encode_sequence_header(NULL, eku_content) + eku_content;

  /* Wrap in OCTET STRING */
  size_t octet_len = fio_der_encode_octet_string(NULL, NULL, eku_len);

  /* Extension: SEQUENCE { OID, value OCTET STRING } (not critical) */
  size_t oid_len = fio___der_encode_oid(NULL, FIO_X509_OID_EXT_KEY_USAGE);
  size_t ext_content = oid_len + octet_len;
  size_t total =
      fio_der_encode_sequence_header(NULL, ext_content) + ext_content;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_sequence_header(buf + offset, ext_content);
    offset += fio___der_encode_oid(buf + offset, FIO_X509_OID_EXT_KEY_USAGE);

    /* OCTET STRING containing EKU SEQUENCE */
    size_t octet_hdr = fio_der_encode_octet_string(buf + offset, NULL, eku_len);
    uint8_t *eku_buf = buf + offset + octet_hdr - eku_len;
    size_t eku_off = 0;
    eku_off += fio_der_encode_sequence_header(eku_buf + eku_off, eku_content);
    eku_off +=
        fio___der_encode_oid(eku_buf + eku_off, FIO_X509_OID_EKU_SERVER_AUTH);
    eku_off +=
        fio___der_encode_oid(eku_buf + eku_off, FIO_X509_OID_EKU_CLIENT_AUTH);
    (void)eku_off;
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
    /* Default key usage based on certificate type and key algorithm */
    if (opts->is_ca) {
      ku = FIO_X509_KU_KEY_CERT_SIGN | FIO_X509_KU_CRL_SIGN;
    } else {
      /*
       * For ECDSA/Ed25519 certificates: digitalSignature only
       * KEY_ENCIPHERMENT is for RSA key exchange, NOT for ECDSA!
       * Using KEY_ENCIPHERMENT with ECDSA causes Chrome to reject with
       * ERR_SSL_KEY_USAGE_INCOMPATIBLE
       */
      ku = FIO_X509_KU_DIGITAL_SIGNATURE;
    }
  }
  ext_content += fio___x509_encode_ext_key_usage(NULL, ku);

  /* Extended Key Usage (serverAuth for TLS server certificates) */
  ext_content += fio___x509_encode_ext_eku(NULL, opts->is_ca);

  /* SubjectAltName */
  if (opts->san_dns && opts->san_dns_count > 0)
    ext_content +=
        fio___x509_encode_ext_san(NULL, opts->san_dns, opts->san_dns_count);

  /* Extensions SEQUENCE */
  size_t exts_len =
      fio_der_encode_sequence_header(NULL, ext_content) + ext_content;

  /* Wrap in context tag [3] */
  size_t total = fio_der_encode_context_header(NULL, 3, exts_len, 1) + exts_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_context_header(buf + offset, 3, exts_len, 1);
    offset += fio_der_encode_sequence_header(buf + offset, ext_content);
    offset +=
        fio___x509_encode_ext_basic_constraints(buf + offset, opts->is_ca);
    offset += fio___x509_encode_ext_key_usage(buf + offset, ku);
    offset += fio___x509_encode_ext_eku(buf + offset, opts->is_ca);
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
FIO_SFUNC size_t
fio___x509_encode_signature_algo(uint8_t *buf, fio_x509_keypair_type_e type) {
  fio_u128 oid;
  int has_params;

  switch (type) {
  case FIO_X509_KEYPAIR_ED25519:
    oid = FIO_X509_OID_ED25519;
    has_params = 0;
    break;
  case FIO_X509_KEYPAIR_P256:
    oid = FIO_X509_OID_ECDSA_WITH_SHA256;
    has_params = 0;
    break;
  default: return 0;
  }

  size_t oid_len = fio___der_encode_oid(NULL, oid);
  size_t content_len = oid_len;
  if (has_params)
    content_len += fio_der_encode_null(NULL);

  size_t total =
      fio_der_encode_sequence_header(NULL, content_len) + content_len;

  if (buf) {
    size_t offset = 0;
    offset += fio_der_encode_sequence_header(buf + offset, content_len);
    offset += fio___der_encode_oid(buf + offset, oid);
    if (has_params)
      offset += fio_der_encode_null(buf + offset);
    (void)offset;
  }

  return total;
}

/** Generate self-signed certificate */
SFUNC size_t fio_x509_self_signed_cert(uint8_t *buf,
                                       size_t buf_len,
                                       const fio_x509_keypair_s *keypair,
                                       const fio_x509_cert_options_s *options) {
  if (!keypair || !options || !options->cn.len)
    return 0;

  /* Set default validity if not specified */
  int64_t not_before = options->not_before;
  int64_t not_after = options->not_after;
  const int64_t now = (int64_t)fio_time_real().tv_sec;
  if (not_before == 0)
    not_before = now - 86400; /* start validity 1 day ago */
  if (not_after == 0)
    not_after = now + (365 * 24 * 60 * 60); /* +1 year */
  if (not_before > not_after)
    return 0; /* reversed validity interval */

  /* Calculate TBSCertificate content length */
  size_t tbs_content = 0;

  /* Version [0] EXPLICIT INTEGER (v3 = 2) */
  size_t version_int = fio_der_encode_integer_small(NULL, 2);
  size_t version_len =
      fio_der_encode_context_header(NULL, 0, version_int, 1) + version_int;
  tbs_content += version_len;

  /* Serial number: 16 bytes, DER-encoded INTEGER.
   * Size = 1 (tag) + 1 (length) + 16 (content) = 18 bytes.
   * High bit is always cleared (serial[0] &= 0x7F), so no sign-pad zero, and
   * the first byte is forced non-zero (see below), so no stripping - the
   * actual encoding is always exactly 18 bytes. */
  size_t serial_len = 18;
  tbs_content += serial_len;

  /* Signature algorithm */
  size_t signature_algo_len =
      fio___x509_encode_signature_algo(NULL, keypair->type);
  tbs_content += signature_algo_len;

  /* Issuer (same as subject for self-signed) */
  size_t issuer_len = fio___x509_encode_name(NULL, options);
  tbs_content += issuer_len;

  /* Validity */
  size_t validity_len = fio___x509_encode_validity(NULL, not_before, not_after);
  if (!validity_len)
    return 0; /* validity dates outside years 0000-9999 */
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
      fio_der_encode_sequence_header(NULL, tbs_content) + tbs_content;

  /* Signature algorithm (again, in outer Certificate) */
  size_t outer_signature_algo_len =
      fio___x509_encode_signature_algo(NULL, keypair->type);

  /* Signature value (BIT STRING) */
  size_t sig_value_len;
  switch (keypair->type) {
  case FIO_X509_KEYPAIR_ED25519:
    sig_value_len = fio_der_encode_bit_string(NULL, NULL, 64, 0);
    break;
  case FIO_X509_KEYPAIR_P256:
    /* ECDSA signature is DER-encoded SEQUENCE { r INTEGER, s INTEGER }
     * Maximum size: 2 + 2 + (2 + 33) + (2 + 33) = 74 bytes */
    sig_value_len = fio_der_encode_bit_string(NULL, NULL, 74, 0);
    break;
  default: return 0;
  }

  /* Certificate SEQUENCE */
  size_t cert_content = tbs_len + outer_signature_algo_len + sig_value_len;
  size_t total =
      fio_der_encode_sequence_header(NULL, cert_content) + cert_content;

  if (!buf)
    return total;

  if (buf_len < total)
    return 0;

  /* Generate random serial number (20 bytes max per RFC 5280).
   * The first byte must be non-zero: fio_der_encode_integer strips leading
   * zero bytes, which would make the actual serial TLV shorter than the
   * serial_len used when sizing the TBSCertificate SEQUENCE (declared
   * length mismatch -> strict DER parsers reject the certificate). */
  uint8_t serial[16];
  do {
    fio_rand_bytes(serial, sizeof(serial));
    serial[0] &= 0x7F; /* Ensure positive */
  } while (!serial[0] || !fio_buf2u64u(serial) || !fio_buf2u64u(serial + 8));

  /* Now encode everything */
  size_t offset = 0;

  /* We need to encode TBS first to sign it, but we don't know the final
   * certificate length yet. Encode TBS to a temporary location. */
  uint8_t *tbs_start = buf + fio_der_encode_sequence_header(NULL, cert_content);

  /* Encode TBSCertificate */
  size_t tbs_offset = 0;
  tbs_offset +=
      fio_der_encode_sequence_header(tbs_start + tbs_offset, tbs_content);

  /* Version */
  tbs_offset +=
      fio_der_encode_context_header(tbs_start + tbs_offset, 0, version_int, 1);
  tbs_offset += fio_der_encode_integer_small(tbs_start + tbs_offset, 2);

  /* Serial */
  tbs_offset +=
      fio_der_encode_integer(tbs_start + tbs_offset, serial, sizeof(serial));

  /* Signature algorithm */
  tbs_offset +=
      fio___x509_encode_signature_algo(tbs_start + tbs_offset, keypair->type);

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
    /* Hash the TBS with SHA-256 */
    fio_u256 hash = fio_sha256(tbs_start, tbs_len);

    /* Sign using proper ECDSA P-256 (returns DER-encoded signature) */
    if (fio_ecdsa_p256_sign(signature,
                            &actual_sig_len,
                            sizeof(signature),
                            hash.u8,
                            keypair->secret_key) != 0)
      return 0;
    break;
  }
  default: return 0;
  }

  /* Recalculate actual signature BIT STRING length */
  size_t actual_sig_bits_len =
      fio_der_encode_bit_string(NULL, signature, actual_sig_len, 0);

  /* Recalculate certificate content length */
  cert_content = tbs_len + outer_signature_algo_len + actual_sig_bits_len;
  total = fio_der_encode_sequence_header(NULL, cert_content) + cert_content;

  if (buf_len < total)
    return 0;

  /* Encode final certificate */
  offset = 0;
  offset += fio_der_encode_sequence_header(buf + offset, cert_content);

  /* Copy TBS (already encoded) */
  FIO_MEMMOVE(buf + offset, tbs_start, tbs_len);
  offset += tbs_len;

  /* Signature algorithm */
  offset += fio___x509_encode_signature_algo(buf + offset, keypair->type);

  /* Signature value */
  offset +=
      fio_der_encode_bit_string(buf + offset, signature, actual_sig_len, 0);

  return total;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_X509 */
#undef FIO_X509
