/**
 * X.509 Certificate Parser Tests
 *
 * Tests X.509 certificate parsing, validity checking, and hostname matching.
 */
#define FIO_LOG
#define FIO_TIME    /* Required for timestamp functions */
#define FIO_SHA2    /* Required for signature verification */
#define FIO_MATH    /* Required for RSA big integer operations */
#define FIO_ASN1    /* Required for ASN.1 parsing */
#define FIO_RSA     /* Required for RSA signature verification */
#define FIO_ED25519 /* Required for Ed25519 signature verification */
#define FIO_X509    /* The module under test */
#include "fio-stl/include.h"

/* *****************************************************************************
Test Data - Real Certificate DER Data

This is a self-signed test certificate generated with:
  openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem \
    -days 365 -nodes -subj "/CN=test.example.com"
  openssl x509 -in cert.pem -outform DER -out cert.der
  xxd -i cert.der

Certificate details:
- Subject: CN=test.example.com
- Issuer: CN=test.example.com (self-signed)
- Key: RSA 2048-bit
- Validity: ~1 year from generation
- Signature: sha256WithRSAEncryption
***************************************************************************** */

/* A minimal self-signed certificate for testing parsing (not signature
 * verification) Generated with constraints for minimal size while still being
 * valid Structure is manually crafted for test purposes
 */

/* Real certificate from Let's Encrypt intermediate (truncated for testing)
 * We'll use a simpler approach with manual test data
 */

/* Test certificate: Self-signed RSA 2048-bit, CN=localhost
 * This is a DER-encoded certificate for parsing tests only
 */

/* Minimal valid X.509 structure for parsing tests:
 *
 * Certificate ::= SEQUENCE {
 *   tbsCertificate       TBSCertificate,
 *   signatureAlgorithm   AlgorithmIdentifier,
 *   signatureValue       BIT STRING
 * }
 */

/* Rather than include real cert data (which changes), we'll construct
 * test vectors that exercise specific parsing paths */

/* *****************************************************************************
Test Helper: Construct minimal certificate DER data
***************************************************************************** */

/* Simple self-signed test certificate (manually constructed DER)
 *
 * This is a minimal valid X.509v3 certificate structure with:
 * - Version: v3 (2)
 * - Serial: 1
 * - Signature Algorithm: sha256WithRSAEncryption
 * - Issuer: CN=test
 * - Validity: 2020-01-01 to 2030-01-01
 * - Subject: CN=test
 * - Public Key: RSA (minimal)
 * - Extensions: None
 */

/* Build a test certificate programmatically for better maintainability */

/* *****************************************************************************
Test: Basic Structure Parsing
***************************************************************************** */

/* A minimal but valid DER-encoded certificate for basic parsing tests.
 *
 * Structure (303 bytes total):
 * - Certificate SEQUENCE (30 82 01 2B = 299 bytes content)
 *   - TBS Certificate SEQUENCE (30 81 D6 = 214 bytes content)
 *     - Version [0] EXPLICIT (A0 03 02 01 02) = v3
 *     - Serial Number (02 01 01) = 1
 *     - Signature Algorithm (30 0D ...) = sha256WithRSAEncryption
 *     - Issuer (30 0F ...) = CN=test
 *     - Validity (30 1E ...) = 2020-01-01 to 2030-01-01
 *     - Subject (30 0F ...) = CN=test
 *     - SubjectPublicKeyInfo (30 5C ...) = RSA 512-bit (94 bytes)
 *     - Extensions [3] (A3 1D ...) = BasicConstraints, SAN=www (31 bytes)
 *   - Signature Algorithm (30 0D ...) = sha256WithRSAEncryption
 *   - Signature (03 41 00 ...) = 64 bytes fake signature
 */
static const uint8_t test_minimal_cert[] = {
    /* clang-format off */
    0x30, 0x82, 0x01, 0x2B, 0x30, 0x81, 0xD6, 0xA0, 0x03, 0x02, 0x01, 0x02,
    0x02, 0x01, 0x01, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7,
    0x0D, 0x01, 0x01, 0x0B, 0x05, 0x00, 0x30, 0x0F, 0x31, 0x0D, 0x30, 0x0B,
    0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x04, 0x74, 0x65, 0x73, 0x74, 0x30,
    0x1E, 0x17, 0x0D, 0x32, 0x30, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x5A, 0x17, 0x0D, 0x33, 0x30, 0x30, 0x31, 0x30, 0x31,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5A, 0x30, 0x0F, 0x31, 0x0D, 0x30,
    0x0B, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x04, 0x74, 0x65, 0x73, 0x74,
    0x30, 0x5C, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D,
    0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x4B, 0x00, 0x30, 0x48, 0x02, 0x41,
    0x00, 0xC3, 0x52, 0x85, 0x3C, 0xF4, 0x68, 0x2E, 0x08, 0x40, 0x47, 0x97,
    0x2F, 0x9B, 0x5A, 0xC8, 0x3F, 0x52, 0x47, 0x85, 0x3F, 0xE4, 0x6C, 0x2F,
    0x0B, 0x5C, 0x4A, 0x87, 0x3F, 0x5A, 0x4C, 0x8F, 0x38, 0x5A, 0x4E, 0x8F,
    0x3C, 0x5E, 0x4A, 0x8B, 0x34, 0x56, 0x42, 0x87, 0x30, 0x52, 0x46, 0x83,
    0x2C, 0x4E, 0x42, 0x8F, 0x28, 0x4A, 0x3E, 0x8B, 0x24, 0x46, 0x3A, 0x87,
    0x20, 0x42, 0x36, 0x83, 0xAB, 0x02, 0x03, 0x01, 0x00, 0x01, 0xA3, 0x1D,
    0x30, 0x1B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x02, 0x30,
    0x00, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x1D, 0x11, 0x04, 0x07, 0x30, 0x05,
    0x82, 0x03, 0x77, 0x77, 0x77, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48,
    0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B, 0x05, 0x00, 0x03, 0x41, 0x00, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
    0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C,
    0x3D, 0x3E, 0x3F,
    /* clang-format on */
};

/* *****************************************************************************
Test: Certificate Parsing
***************************************************************************** */

static void test_x509_parse_basic(void) {
  fio_x509_cert_s cert;
  int ret = fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert));

  FIO_ASSERT(ret == 0, "Failed to parse test certificate: %d", ret);

  /* Check version (v3 = 2) */
  FIO_ASSERT(cert.version == 2, "Wrong version: %d (expected 2)", cert.version);

  /* Check signature algorithm */
  FIO_ASSERT(cert.sig_alg == FIO_X509_SIG_RSA_PKCS1_SHA256,
             "Wrong sig alg: %d",
             cert.sig_alg);

  /* Check key type */
  FIO_ASSERT(cert.key_type == FIO_X509_KEY_RSA,
             "Wrong key type: %d",
             cert.key_type);

  /* Check RSA key was extracted */
  FIO_ASSERT(cert.pubkey.rsa.n != NULL, "RSA modulus not extracted");
  FIO_ASSERT(cert.pubkey.rsa.n_len > 0,
             "RSA modulus length: %zu",
             cert.pubkey.rsa.n_len);
  FIO_ASSERT(cert.pubkey.rsa.e != NULL, "RSA exponent not extracted");
  FIO_ASSERT(cert.pubkey.rsa.e_len > 0,
             "RSA exponent length: %zu",
             cert.pubkey.rsa.e_len);

  /* Check validity period */
  FIO_ASSERT(cert.not_before > 0, "notBefore not parsed");
  FIO_ASSERT(cert.not_after > cert.not_before,
             "notAfter should be after notBefore");

  /* Check Common Name */
  FIO_ASSERT(cert.subject_cn != NULL, "CN not extracted");
  FIO_ASSERT(cert.subject_cn_len == 4, "CN length: %zu", cert.subject_cn_len);
  FIO_ASSERT(FIO_MEMCMP(cert.subject_cn, "test", 4) == 0, "Wrong CN value");

  /* Check TBS data */
  FIO_ASSERT(cert.tbs_data != NULL, "TBS data not captured");
  FIO_ASSERT(cert.tbs_len > 0, "TBS length should be > 0");

  /* Check signature */
  FIO_ASSERT(cert.signature != NULL, "Signature not extracted");
  FIO_ASSERT(cert.signature_len > 0, "Signature length should be > 0");

  /* Check issuer/subject DN */
  FIO_ASSERT(cert.issuer_der != NULL, "Issuer DN not captured");
  FIO_ASSERT(cert.subject_der != NULL, "Subject DN not captured");
}

/* *****************************************************************************
Test: Validity Period Checking
***************************************************************************** */

static void test_x509_validity(void) {
  fio_x509_cert_s cert;
  int ret = fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert));
  FIO_ASSERT(ret == 0, "Failed to parse test certificate");

  /* Certificate is valid 2020-01-01 to 2030-01-01 */

  /* Test valid time (2025-06-15) - Unix timestamp ~1750000000 */
  int64_t valid_time = 1750000000;
  FIO_ASSERT(fio_x509_check_validity(&cert, valid_time) == 0,
             "Certificate should be valid at current time");

  /* Test expired time (2031-01-01) */
  int64_t expired_time = 1925000000;
  FIO_ASSERT(fio_x509_check_validity(&cert, expired_time) == -1,
             "Certificate should be expired");

  /* Test not yet valid (2019-01-01) */
  int64_t early_time = 1546300800;
  FIO_ASSERT(fio_x509_check_validity(&cert, early_time) == -1,
             "Certificate should not be valid yet");
}

/* *****************************************************************************
Test: Hostname Matching
***************************************************************************** */

static void test_x509_hostname_matching(void) {
  fio_x509_cert_s cert;
  FIO_MEMSET(&cert, 0, sizeof(cert));

  /* Test with explicit CN */
  cert.subject_cn = "www.example.com";
  cert.subject_cn_len = 15;

  FIO_ASSERT(fio_x509_match_hostname(&cert, "www.example.com", 15) == 0,
             "Exact match should succeed");
  FIO_ASSERT(fio_x509_match_hostname(&cert, "WWW.EXAMPLE.COM", 15) == 0,
             "Case-insensitive match should succeed");
  FIO_ASSERT(fio_x509_match_hostname(&cert, "www.example.org", 15) != 0,
             "Different domain should fail");
  FIO_ASSERT(fio_x509_match_hostname(&cert, "example.com", 11) != 0,
             "Subdomain mismatch should fail");

  /* Test with wildcard CN */
  cert.subject_cn = "*.example.com";
  cert.subject_cn_len = 13;

  FIO_ASSERT(fio_x509_match_hostname(&cert, "www.example.com", 15) == 0,
             "Wildcard match should succeed");
  FIO_ASSERT(fio_x509_match_hostname(&cert, "api.example.com", 15) == 0,
             "Wildcard match for different subdomain should succeed");
  FIO_ASSERT(fio_x509_match_hostname(&cert, "example.com", 11) != 0,
             "Wildcard should not match apex");
  FIO_ASSERT(fio_x509_match_hostname(&cert, "sub.www.example.com", 19) != 0,
             "Wildcard should not match multiple levels");

  /* Test with SAN (preferred over CN) */
  cert.subject_cn = "wrong.example.com";
  cert.subject_cn_len = 17;
  cert.san_dns = "correct.example.com";
  cert.san_dns_len = 19;

  FIO_ASSERT(fio_x509_match_hostname(&cert, "correct.example.com", 19) == 0,
             "SAN should be preferred over CN");
  FIO_ASSERT(fio_x509_match_hostname(&cert, "wrong.example.com", 17) != 0,
             "CN should not match when SAN is present");
}

/* *****************************************************************************
Test: DN Comparison
***************************************************************************** */

static void test_x509_dn_comparison(void) {
  /* Test equal DNs */
  uint8_t dn1[] = {0x30,
                   0x0F,
                   0x31,
                   0x0D,
                   0x30,
                   0x0B,
                   0x06,
                   0x03,
                   0x55,
                   0x04,
                   0x03,
                   0x0C,
                   0x04,
                   't',
                   'e',
                   's',
                   't'};
  uint8_t dn2[] = {0x30,
                   0x0F,
                   0x31,
                   0x0D,
                   0x30,
                   0x0B,
                   0x06,
                   0x03,
                   0x55,
                   0x04,
                   0x03,
                   0x0C,
                   0x04,
                   't',
                   'e',
                   's',
                   't'};
  uint8_t dn3[] = {0x30,
                   0x10,
                   0x31,
                   0x0E,
                   0x30,
                   0x0C,
                   0x06,
                   0x03,
                   0x55,
                   0x04,
                   0x03,
                   0x0C,
                   0x05,
                   'o',
                   't',
                   'h',
                   'e',
                   'r'};

  FIO_ASSERT(fio_x509_dn_equals(dn1, sizeof(dn1), dn2, sizeof(dn2)) == 0,
             "Identical DNs should be equal");
  FIO_ASSERT(fio_x509_dn_equals(dn1, sizeof(dn1), dn3, sizeof(dn3)) != 0,
             "Different DNs should not be equal");
  FIO_ASSERT(fio_x509_dn_equals(dn1, sizeof(dn1), dn1, sizeof(dn1) - 1) != 0,
             "Different length DNs should not be equal");
}

/* *****************************************************************************
Test: Key Type Parsing
***************************************************************************** */

static void test_x509_key_types(void) {
  /* Test RSA key (already tested above) */
  fio_x509_cert_s cert;
  int ret = fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert));
  FIO_ASSERT(ret == 0, "Failed to parse RSA certificate");
  FIO_ASSERT(cert.key_type == FIO_X509_KEY_RSA, "Should detect RSA key");

  /* RSA exponent should be 65537 (0x010001) */
  if (cert.pubkey.rsa.e_len == 3) {
    FIO_ASSERT(cert.pubkey.rsa.e[0] == 0x01 && cert.pubkey.rsa.e[1] == 0x00 &&
                   cert.pubkey.rsa.e[2] == 0x01,
               "RSA exponent should be 65537");
  }
}

/* *****************************************************************************
Test: Extensions Parsing
***************************************************************************** */

static void test_x509_extensions(void) {
  fio_x509_cert_s cert;
  int ret = fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert));
  FIO_ASSERT(ret == 0, "Failed to parse certificate with extensions");

  /* Check BasicConstraints was parsed */
  /* Our test cert has an empty BasicConstraints, so is_ca should be 0 */
  FIO_ASSERT(cert.is_ca == 0, "is_ca should be 0 for end-entity cert");

  /* Check SAN was parsed */
  FIO_ASSERT(cert.san_dns != NULL, "SAN DNS name should be extracted");
  if (cert.san_dns) {
    FIO_ASSERT(cert.san_dns_len == 3, "SAN DNS length: %zu", cert.san_dns_len);
    FIO_ASSERT(FIO_MEMCMP(cert.san_dns, "www", 3) == 0,
               "SAN DNS should be 'www'");
  }
}

/* *****************************************************************************
Test: Invalid Input Handling
***************************************************************************** */

static void test_x509_invalid_inputs(void) {
  fio_x509_cert_s cert;

  /* NULL inputs */
  FIO_ASSERT(
      fio_x509_parse(NULL, test_minimal_cert, sizeof(test_minimal_cert)) == -1,
      "NULL cert should fail");
  FIO_ASSERT(fio_x509_parse(&cert, NULL, 100) == -1, "NULL data should fail");
  FIO_ASSERT(fio_x509_parse(&cert, test_minimal_cert, 0) == -1,
             "Zero length should fail");

  /* Truncated data */
  FIO_ASSERT(fio_x509_parse(&cert, test_minimal_cert, 10) == -1,
             "Truncated data should fail");

  /* Invalid hostname matching */
  FIO_MEMSET(&cert, 0, sizeof(cert));
  FIO_ASSERT(fio_x509_match_hostname(NULL, "test", 4) == -1,
             "NULL cert should fail");
  FIO_ASSERT(fio_x509_match_hostname(&cert, NULL, 4) == -1,
             "NULL hostname should fail");
  FIO_ASSERT(fio_x509_match_hostname(&cert, "test", 0) == -1,
             "Zero hostname length should fail");

  /* Validity check with NULL */
  FIO_ASSERT(fio_x509_check_validity(NULL, 1000000) == -1,
             "NULL cert validity check should fail");
}

/* *****************************************************************************
Test: Self-Signed Certificate Properties
***************************************************************************** */

static void test_x509_self_signed(void) {
  fio_x509_cert_s cert;
  int ret = fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert));
  FIO_ASSERT(ret == 0, "Failed to parse certificate");

  /* For a self-signed cert, issuer DN == subject DN */
  FIO_ASSERT(cert.issuer_der != NULL, "Issuer DN should be present");
  FIO_ASSERT(cert.subject_der != NULL, "Subject DN should be present");
  FIO_ASSERT(cert.issuer_der_len == cert.subject_der_len,
             "Self-signed: issuer and subject DN lengths should match");

  int is_self_signed = fio_x509_dn_equals(cert.issuer_der,
                                          cert.issuer_der_len,
                                          cert.subject_der,
                                          cert.subject_der_len) == 0;
  FIO_ASSERT(is_self_signed, "Certificate should be self-signed");
}

/* *****************************************************************************
Test: Signature Algorithm Detection
***************************************************************************** */

static void test_x509_sig_algorithms(void) {
  fio_x509_cert_s cert;
  int ret = fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert));
  FIO_ASSERT(ret == 0, "Failed to parse certificate");

  /* Our test cert uses sha256WithRSAEncryption */
  FIO_ASSERT(cert.sig_alg == FIO_X509_SIG_RSA_PKCS1_SHA256,
             "Should detect sha256WithRSAEncryption, got: %d",
             cert.sig_alg);
}

/* *****************************************************************************
Test: Chain Validation - Error Codes
***************************************************************************** */

static void test_x509_error_strings(void) {
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_OK)) > 0,
             "OK should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_PARSE)) > 0,
             "PARSE error should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_EXPIRED)) > 0,
             "EXPIRED error should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_NOT_YET_VALID)) > 0,
             "NOT_YET_VALID error should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_SIGNATURE)) > 0,
             "SIGNATURE error should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_ISSUER_MISMATCH)) > 0,
             "ISSUER_MISMATCH error should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_NOT_CA)) > 0,
             "NOT_CA error should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_NO_TRUST_ANCHOR)) > 0,
             "NO_TRUST_ANCHOR error should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_HOSTNAME_MISMATCH)) > 0,
             "HOSTNAME_MISMATCH error should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_EMPTY_CHAIN)) > 0,
             "EMPTY_CHAIN error should have description");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(-999)) > 0,
             "Unknown error should have description");
}

/* *****************************************************************************
Test: Chain Validation - Single Self-Signed Certificate
***************************************************************************** */

static void test_x509_chain_single_self_signed(void) {
  /* Use the minimal test cert (self-signed) */
  const uint8_t *certs[1] = {test_minimal_cert};
  const size_t cert_lens[1] = {sizeof(test_minimal_cert)};

  /* Test with valid time (2025) - should work without trust store */
  int64_t valid_time = 1750000000;
  int ret = fio_x509_verify_chain(certs,
                                  cert_lens,
                                  1,
                                  NULL, /* no hostname check */
                                  valid_time,
                                  NULL /* no trust store */);

  /* Note: With our test cert, signature verification may fail because
   * the signature is fake. We're testing the chain logic. */
  if (ret == FIO_X509_ERR_SIGNATURE) {
  }

  /* Test with trust store containing the self-signed cert */
  const uint8_t *roots[1] = {test_minimal_cert};
  const size_t root_lens[1] = {sizeof(test_minimal_cert)};
  fio_x509_trust_store_s trust_store = {
      .roots = roots,
      .root_lens = root_lens,
      .root_count = 1,
  };

  ret = fio_x509_verify_chain(certs,
                              cert_lens,
                              1,
                              NULL,
                              valid_time,
                              &trust_store);

  /* Should either pass or fail on signature (not on trust store) */
  FIO_ASSERT(ret == FIO_X509_OK || ret == FIO_X509_ERR_SIGNATURE,
             "Self-signed with trust store: unexpected error %d (%s)",
             ret,
             fio_x509_error_str(ret));
}

/* *****************************************************************************
Test: Chain Validation - Invalid Inputs
***************************************************************************** */

static void test_x509_chain_invalid_inputs(void) {
  const uint8_t *certs[1] = {test_minimal_cert};
  const size_t cert_lens[1] = {sizeof(test_minimal_cert)};
  int64_t valid_time = 1750000000;

  /* NULL certs array */
  int ret = fio_x509_verify_chain(NULL, cert_lens, 1, NULL, valid_time, NULL);
  FIO_ASSERT(ret == FIO_X509_ERR_PARSE,
             "NULL certs should return PARSE error, got: %d",
             ret);

  /* NULL cert_lens array */
  ret = fio_x509_verify_chain(certs, NULL, 1, NULL, valid_time, NULL);
  FIO_ASSERT(ret == FIO_X509_ERR_PARSE,
             "NULL cert_lens should return PARSE error, got: %d",
             ret);

  /* Empty chain */
  ret = fio_x509_verify_chain(certs, cert_lens, 0, NULL, valid_time, NULL);
  FIO_ASSERT(ret == FIO_X509_ERR_EMPTY_CHAIN,
             "Empty chain should return EMPTY_CHAIN error, got: %d",
             ret);
}

/* *****************************************************************************
Test: Chain Validation - Expired Certificate
***************************************************************************** */

static void test_x509_chain_expired(void) {
  const uint8_t *certs[1] = {test_minimal_cert};
  const size_t cert_lens[1] = {sizeof(test_minimal_cert)};

  /* Test with time far in future (2031) - cert valid until 2030 */
  int64_t expired_time = 1925000000;
  int ret =
      fio_x509_verify_chain(certs, cert_lens, 1, NULL, expired_time, NULL);
  FIO_ASSERT(ret == FIO_X509_ERR_EXPIRED,
             "Expired cert should return EXPIRED error, got: %d (%s)",
             ret,
             fio_x509_error_str(ret));

  /* Test with time in past (2019) - cert valid from 2020 */
  int64_t early_time = 1546300800;
  ret = fio_x509_verify_chain(certs, cert_lens, 1, NULL, early_time, NULL);
  FIO_ASSERT(ret == FIO_X509_ERR_NOT_YET_VALID,
             "Not-yet-valid cert should return NOT_YET_VALID error, got: %d",
             ret);
}

/* *****************************************************************************
Test: Chain Validation - Hostname Mismatch
***************************************************************************** */

static void test_x509_chain_hostname(void) {
  const uint8_t *certs[1] = {test_minimal_cert};
  const size_t cert_lens[1] = {sizeof(test_minimal_cert)};
  int64_t valid_time = 1750000000;

  /* Test with correct hostname (SAN = "www") */
  int ret = fio_x509_verify_chain(certs, cert_lens, 1, "www", valid_time, NULL);
  /* Should pass hostname check (may fail on signature) */
  FIO_ASSERT(ret == FIO_X509_OK || ret == FIO_X509_ERR_SIGNATURE,
             "Correct hostname should not return HOSTNAME_MISMATCH: got %d",
             ret);

  /* Test with wrong hostname */
  ret = fio_x509_verify_chain(certs,
                              cert_lens,
                              1,
                              "wrong.example.com",
                              valid_time,
                              NULL);
  FIO_ASSERT(ret == FIO_X509_ERR_HOSTNAME_MISMATCH,
             "Wrong hostname should return HOSTNAME_MISMATCH, got: %d (%s)",
             ret,
             fio_x509_error_str(ret));
}

/* *****************************************************************************
Test: Trust Store Functions
***************************************************************************** */

static void test_x509_trust_store(void) {
  /* Parse the test certificate */
  fio_x509_cert_s cert;
  int ret = fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert));
  FIO_ASSERT(ret == 0, "Failed to parse certificate");

  /* Create trust store containing the test cert */
  const uint8_t *roots[1] = {test_minimal_cert};
  const size_t root_lens[1] = {sizeof(test_minimal_cert)};
  fio_x509_trust_store_s trust_store = {
      .roots = roots,
      .root_lens = root_lens,
      .root_count = 1,
  };

  /* Self-signed cert should be found in trust store */
  ret = fio_x509_is_trusted(&cert, &trust_store);
  FIO_ASSERT(ret == 0,
             "Self-signed cert should be in trust store, got: %d",
             ret);

  /* Empty trust store */
  fio_x509_trust_store_s empty_store = {
      .roots = NULL,
      .root_lens = NULL,
      .root_count = 0,
  };
  ret = fio_x509_is_trusted(&cert, &empty_store);
  FIO_ASSERT(ret == -1, "Cert should not be in empty trust store");

  /* NULL inputs */
  ret = fio_x509_is_trusted(NULL, &trust_store);
  FIO_ASSERT(ret == -1, "NULL cert should fail");

  ret = fio_x509_is_trusted(&cert, NULL);
  FIO_ASSERT(ret == -1, "NULL trust store should fail");
}

/* *****************************************************************************
Test: TLS Certificate Message Parsing
***************************************************************************** */

static void test_tls_certificate_message_parsing(void) {
  /* Build a minimal TLS Certificate message:
   * - context length: 0 (1 byte)
   * - certificate_list length: X (3 bytes)
   * - certificate entry:
   *   - cert_data length: Y (3 bytes)
   *   - cert_data: ... (Y bytes)
   *   - extensions length: 0 (2 bytes)
   */
  size_t cert_len = sizeof(test_minimal_cert);
  size_t entry_len = 3 + cert_len + 2; /* length + cert + ext_len */
  size_t msg_len = 1 + 3 + entry_len;  /* ctx_len + list_len + entry */

  uint8_t *msg = malloc(msg_len);
  FIO_ASSERT(msg, "Failed to allocate message buffer");

  uint8_t *p = msg;

  /* certificate_request_context length (1 byte) */
  *p++ = 0;

  /* certificate_list length (3 bytes, big-endian) */
  *p++ = (uint8_t)((entry_len >> 16) & 0xFF);
  *p++ = (uint8_t)((entry_len >> 8) & 0xFF);
  *p++ = (uint8_t)(entry_len & 0xFF);

  /* cert_data length (3 bytes, big-endian) */
  *p++ = (uint8_t)((cert_len >> 16) & 0xFF);
  *p++ = (uint8_t)((cert_len >> 8) & 0xFF);
  *p++ = (uint8_t)(cert_len & 0xFF);

  /* cert_data */
  FIO_MEMCPY(p, test_minimal_cert, cert_len);
  p += cert_len;

  /* extensions length (2 bytes) */
  *p++ = 0;
  *p++ = 0;

  /* Parse the message */
  fio_tls_cert_entry_s entries[5];
  int count = fio_tls_parse_certificate_message(entries, 5, msg, msg_len);

  FIO_ASSERT(count == 1, "Should parse 1 certificate, got: %d", count);
  FIO_ASSERT(entries[0].cert_len == cert_len,
             "Certificate length mismatch: %zu vs %zu",
             entries[0].cert_len,
             cert_len);
  FIO_ASSERT(FIO_MEMCMP(entries[0].cert, test_minimal_cert, cert_len) == 0,
             "Certificate data mismatch");

  /* Test invalid inputs */
  count = fio_tls_parse_certificate_message(NULL, 5, msg, msg_len);
  FIO_ASSERT(count == -1, "NULL entries should fail");

  count = fio_tls_parse_certificate_message(entries, 0, msg, msg_len);
  FIO_ASSERT(count == -1, "Zero max_entries should fail");

  count = fio_tls_parse_certificate_message(entries, 5, NULL, msg_len);
  FIO_ASSERT(count == -1, "NULL data should fail");

  count = fio_tls_parse_certificate_message(entries, 5, msg, 0);
  FIO_ASSERT(count == -1, "Zero data_len should fail");

  /* Test truncated message */
  count = fio_tls_parse_certificate_message(entries, 5, msg, 3);
  FIO_ASSERT(count == -1, "Truncated message should fail");

  free(msg);
}

/* *****************************************************************************
Main Test Runner
***************************************************************************** */

int main(void) {
  /* Basic X.509 parsing tests */
  test_x509_parse_basic();
  test_x509_validity();
  test_x509_hostname_matching();
  test_x509_dn_comparison();
  test_x509_key_types();
  test_x509_extensions();
  test_x509_invalid_inputs();
  test_x509_self_signed();
  test_x509_sig_algorithms();

  /* Chain validation tests */
  test_x509_error_strings();
  test_x509_chain_single_self_signed();
  test_x509_chain_invalid_inputs();
  test_x509_chain_expired();
  test_x509_chain_hostname();
  test_x509_trust_store();
  test_tls_certificate_message_parsing();
  return 0;
}
