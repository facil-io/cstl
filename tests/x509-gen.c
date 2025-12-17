/* *****************************************************************************
X.509 Certificate Generation Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPTO
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Test Ed25519 Key Pair Generation
***************************************************************************** */

FIO_SFUNC void fio___test_x509_keypair_ed25519(void) {
  fprintf(stderr, "\t* Testing Ed25519 key pair generation\n");

  fio_x509_keypair_s kp;
  int result = fio_x509_keypair_ed25519(&kp);

  FIO_ASSERT(result == 0, "fio_x509_keypair_ed25519 failed");
  FIO_ASSERT(kp.type == FIO_X509_KEYPAIR_ED25519, "Wrong key type");
  FIO_ASSERT(kp.secret_key_len == 32, "Wrong secret key length");
  FIO_ASSERT(kp.public_key_len == 32, "Wrong public key length");

  /* Verify the key pair is valid by signing and verifying */
  const char *msg = "Test message for Ed25519 key pair validation";
  uint8_t sig[64];
  fio_ed25519_sign(sig, msg, strlen(msg), kp.secret_key, kp.public_key);

  int verify = fio_ed25519_verify(sig, msg, strlen(msg), kp.public_key);
  FIO_ASSERT(verify == 0, "Ed25519 key pair verification failed");

  /* Clear and verify */
  fio_x509_keypair_clear(&kp);
  FIO_ASSERT(kp.type == 0, "Key pair not cleared");

  fprintf(stderr, "\t  Ed25519 key pair generation tests passed\n");
}

/* *****************************************************************************
Test P-256 Key Pair Generation
***************************************************************************** */

FIO_SFUNC void fio___test_x509_keypair_p256(void) {
  fprintf(stderr, "\t* Testing P-256 key pair generation\n");

  fio_x509_keypair_s kp;
  int result = fio_x509_keypair_p256(&kp);

  FIO_ASSERT(result == 0, "fio_x509_keypair_p256 failed");
  FIO_ASSERT(kp.type == FIO_X509_KEYPAIR_P256, "Wrong key type");
  FIO_ASSERT(kp.secret_key_len == 32, "Wrong secret key length");
  FIO_ASSERT(kp.public_key_len == 65, "Wrong public key length");
  FIO_ASSERT(kp.public_key[0] == 0x04,
             "P-256 public key should start with 0x04");

  /* Clear and verify */
  fio_x509_keypair_clear(&kp);
  FIO_ASSERT(kp.type == 0, "Key pair not cleared");

  fprintf(stderr, "\t  P-256 key pair generation tests passed\n");
}

/* *****************************************************************************
Test ASN.1 Encoding Functions
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_encoding(void) {
  fprintf(stderr, "\t* Testing ASN.1 encoding functions\n");

  uint8_t buf[256];

  /* Test length encoding */
  {
    /* Short form */
    size_t len = fio_asn1_encode_length(buf, 50);
    FIO_ASSERT(len == 1, "Short form length should be 1 byte");
    FIO_ASSERT(buf[0] == 50, "Short form length value wrong");

    /* Long form (1 byte) */
    len = fio_asn1_encode_length(buf, 200);
    FIO_ASSERT(len == 2, "Long form (1 byte) length should be 2 bytes");
    FIO_ASSERT(buf[0] == 0x81, "Long form indicator wrong");
    FIO_ASSERT(buf[1] == 200, "Long form length value wrong");

    /* Long form (2 bytes) */
    len = fio_asn1_encode_length(buf, 300);
    FIO_ASSERT(len == 3, "Long form (2 bytes) length should be 3 bytes");
    FIO_ASSERT(buf[0] == 0x82, "Long form indicator wrong");
    FIO_ASSERT(buf[1] == 0x01, "Long form high byte wrong");
    FIO_ASSERT(buf[2] == 0x2C, "Long form low byte wrong");
  }

  /* Test INTEGER encoding */
  {
    /* Small integer */
    size_t len = fio_asn1_encode_integer_small(buf, 42);
    FIO_ASSERT(len == 3, "INTEGER(42) should be 3 bytes");
    FIO_ASSERT(buf[0] == 0x02, "INTEGER tag wrong");
    FIO_ASSERT(buf[1] == 0x01, "INTEGER length wrong");
    FIO_ASSERT(buf[2] == 42, "INTEGER value wrong");

    /* Integer needing leading zero */
    len = fio_asn1_encode_integer_small(buf, 128);
    FIO_ASSERT(len == 4, "INTEGER(128) should be 4 bytes");
    FIO_ASSERT(buf[0] == 0x02, "INTEGER tag wrong");
    FIO_ASSERT(buf[1] == 0x02, "INTEGER length wrong");
    FIO_ASSERT(buf[2] == 0x00, "INTEGER leading zero missing");
    FIO_ASSERT(buf[3] == 128, "INTEGER value wrong");
  }

  /* Test OID encoding */
  {
    /* Ed25519 OID: 1.3.101.112 */
    size_t len = fio_asn1_encode_oid(buf, FIO_OID_ED25519);
    FIO_ASSERT(len > 0, "OID encoding failed");
    FIO_ASSERT(buf[0] == 0x06, "OID tag wrong");

    /* Verify by parsing back */
    fio_asn1_element_s elem;
    FIO_ASSERT(fio_asn1_parse(&elem, buf, len) != NULL, "Failed to parse OID");
    FIO_ASSERT(fio_asn1_oid_eq(&elem, FIO_OID_ED25519),
               "OID roundtrip failed for Ed25519");

    /* SHA256 with RSA OID */
    len = fio_asn1_encode_oid(buf, FIO_OID_SHA256_WITH_RSA);
    FIO_ASSERT(len > 0, "OID encoding failed");
    FIO_ASSERT(fio_asn1_parse(&elem, buf, len) != NULL, "Failed to parse OID");
    FIO_ASSERT(fio_asn1_oid_eq(&elem, FIO_OID_SHA256_WITH_RSA),
               "OID roundtrip failed for SHA256-RSA");
  }

  /* Test UTF8String encoding */
  {
    const char *str = "localhost";
    size_t len = fio_asn1_encode_utf8_string(buf, str, strlen(str));
    FIO_ASSERT(len == 2 + strlen(str), "UTF8String length wrong");
    FIO_ASSERT(buf[0] == 0x0C, "UTF8String tag wrong");
    FIO_ASSERT(buf[1] == strlen(str), "UTF8String length byte wrong");
    FIO_ASSERT(memcmp(buf + 2, str, strlen(str)) == 0,
               "UTF8String content wrong");
  }

  /* Test BIT STRING encoding */
  {
    uint8_t bits[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    size_t len = fio_asn1_encode_bit_string(buf, bits, 4, 0);
    FIO_ASSERT(len == 7, "BIT STRING length wrong");
    FIO_ASSERT(buf[0] == 0x03, "BIT STRING tag wrong");
    FIO_ASSERT(buf[1] == 5, "BIT STRING length byte wrong");
    FIO_ASSERT(buf[2] == 0, "BIT STRING unused bits wrong");
    FIO_ASSERT(memcmp(buf + 3, bits, 4) == 0, "BIT STRING content wrong");
  }

  /* Test SEQUENCE header encoding */
  {
    size_t len = fio_asn1_encode_sequence_header(buf, 100);
    FIO_ASSERT(len == 2, "SEQUENCE header length wrong");
    FIO_ASSERT(buf[0] == 0x30, "SEQUENCE tag wrong");
    FIO_ASSERT(buf[1] == 100, "SEQUENCE length wrong");
  }

  /* Test BOOLEAN encoding */
  {
    size_t len = fio_asn1_encode_boolean(buf, 1);
    FIO_ASSERT(len == 3, "BOOLEAN length wrong");
    FIO_ASSERT(buf[0] == 0x01, "BOOLEAN tag wrong");
    FIO_ASSERT(buf[1] == 0x01, "BOOLEAN length wrong");
    FIO_ASSERT(buf[2] == 0xFF, "BOOLEAN TRUE value wrong");

    len = fio_asn1_encode_boolean(buf, 0);
    FIO_ASSERT(buf[2] == 0x00, "BOOLEAN FALSE value wrong");
  }

  /* Test NULL encoding */
  {
    size_t len = fio_asn1_encode_null(buf);
    FIO_ASSERT(len == 2, "NULL length wrong");
    FIO_ASSERT(buf[0] == 0x05, "NULL tag wrong");
    FIO_ASSERT(buf[1] == 0x00, "NULL length wrong");
  }

  /* Test UTCTime encoding */
  {
    /* 2024-01-15 12:30:45 UTC = 1705321845 */
    int64_t ts = 1705321845;
    size_t len = fio_asn1_encode_utc_time(buf, ts);
    FIO_ASSERT(len == 15, "UTCTime length wrong");
    FIO_ASSERT(buf[0] == 0x17, "UTCTime tag wrong");
    FIO_ASSERT(buf[1] == 13, "UTCTime content length wrong");
    /* Should be "240115123045Z" */
    FIO_ASSERT(memcmp(buf + 2, "240115123045Z", 13) == 0,
               "UTCTime content wrong: %.*s",
               13,
               buf + 2);
  }

  fprintf(stderr, "\t  ASN.1 encoding tests passed\n");
}

/* *****************************************************************************
Test Self-Signed Certificate Generation (Ed25519)
***************************************************************************** */

FIO_SFUNC void fio___test_x509_self_signed_ed25519(void) {
  fprintf(stderr, "\t* Testing Ed25519 self-signed certificate generation\n");

  /* Generate key pair */
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0, "Key pair generation failed");

  /* Set up certificate options */
  const char *san_dns[] = {"localhost", "127.0.0.1"};
  fio_x509_cert_options_s opts = {
      .subject_cn = "localhost",
      .subject_org = "Test Organization",
      .subject_org_len = 17,
      .subject_c = "US",
      .san_dns = san_dns,
      .san_dns_count = 2,
      .is_ca = 0,
  };

  /* Calculate required buffer size */
  size_t cert_len = fio_x509_self_signed_cert(NULL, 0, &kp, &opts);
  FIO_ASSERT(cert_len > 0, "Certificate size calculation failed");
  fprintf(stderr, "\t  Certificate size: %zu bytes\n", cert_len);

  /* Allocate buffer and generate certificate */
  uint8_t *cert_buf = malloc(cert_len + 100); /* Extra space for safety */
  FIO_ASSERT(cert_buf != NULL, "Memory allocation failed");

  size_t actual_len =
      fio_x509_self_signed_cert(cert_buf, cert_len + 100, &kp, &opts);
  FIO_ASSERT(actual_len > 0, "Certificate generation failed");
  FIO_ASSERT(actual_len <= cert_len + 10, "Certificate larger than expected");

  /* Parse the generated certificate */
  fio_x509_cert_s parsed;
  int parse_result = fio_x509_parse(&parsed, cert_buf, actual_len);
  FIO_ASSERT(parse_result == 0, "Failed to parse generated certificate");

  /* Verify certificate fields */
  FIO_ASSERT(parsed.version == 2, "Certificate version should be v3 (2)");
  FIO_ASSERT(parsed.key_type == FIO_X509_KEY_ED25519,
             "Key type should be Ed25519");
  FIO_ASSERT(parsed.sig_alg == FIO_X509_SIG_ED25519,
             "Signature algorithm should be Ed25519");

  /* Verify public key matches */
  FIO_ASSERT(parsed.pubkey.ed25519.key != NULL, "Public key not parsed");
  FIO_ASSERT(memcmp(parsed.pubkey.ed25519.key, kp.public_key, 32) == 0,
             "Public key mismatch");

  /* Verify subject CN */
  FIO_ASSERT(parsed.subject_cn != NULL, "Subject CN not parsed");
  FIO_ASSERT(parsed.subject_cn_len == strlen("localhost"),
             "Subject CN length wrong");
  FIO_ASSERT(memcmp(parsed.subject_cn, "localhost", parsed.subject_cn_len) == 0,
             "Subject CN content wrong");

  /* Verify validity period */
  FIO_ASSERT(parsed.not_before > 0, "not_before not set");
  FIO_ASSERT(parsed.not_after > parsed.not_before,
             "not_after should be after not_before");
  FIO_ASSERT(parsed.not_after - parsed.not_before >= 365 * 24 * 60 * 60 - 100,
             "Validity period should be ~1 year");

  /* Verify BasicConstraints (CA:FALSE) */
  FIO_ASSERT(parsed.is_ca == 0, "Certificate should not be CA");

  /* Verify KeyUsage */
  FIO_ASSERT(parsed.has_key_usage, "KeyUsage extension should be present");
  FIO_ASSERT(parsed.key_usage & FIO_X509_KU_DIGITAL_SIGNATURE,
             "digitalSignature should be set");

  /* Verify SAN extension */
  FIO_ASSERT(parsed.san_ext_data != NULL, "SAN extension should be present");
  FIO_ASSERT(parsed.san_dns != NULL, "SAN DNS name should be parsed");

  /* Verify self-signature */
  int sig_result = fio_x509_verify_signature(&parsed, &parsed);
  FIO_ASSERT(sig_result == 0, "Self-signature verification failed");

  /* Clean up */
  free(cert_buf);
  fio_x509_keypair_clear(&kp);

  fprintf(stderr, "\t  Ed25519 self-signed certificate tests passed\n");
}

/* *****************************************************************************
Test Self-Signed Certificate Generation (P-256)
***************************************************************************** */

FIO_SFUNC void fio___test_x509_self_signed_p256(void) {
  fprintf(stderr, "\t* Testing P-256 self-signed certificate generation\n");

  /* Generate key pair */
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_p256(&kp) == 0, "Key pair generation failed");

  /* Set up certificate options */
  fio_x509_cert_options_s opts = {
      .subject_cn = "test.example.com",
      .is_ca = 0,
  };

  /* Calculate required buffer size */
  size_t cert_len = fio_x509_self_signed_cert(NULL, 0, &kp, &opts);
  FIO_ASSERT(cert_len > 0, "Certificate size calculation failed");
  fprintf(stderr, "\t  Certificate size: %zu bytes\n", cert_len);

  /* Allocate buffer and generate certificate */
  uint8_t *cert_buf = malloc(cert_len + 100); /* Extra space for safety */
  FIO_ASSERT(cert_buf != NULL, "Memory allocation failed");

  size_t actual_len =
      fio_x509_self_signed_cert(cert_buf, cert_len + 100, &kp, &opts);
  FIO_ASSERT(actual_len > 0, "Certificate generation failed");

  /* Parse the generated certificate */
  fio_x509_cert_s parsed;
  int parse_result = fio_x509_parse(&parsed, cert_buf, actual_len);
  FIO_ASSERT(parse_result == 0, "Failed to parse generated certificate");

  /* Verify certificate fields */
  FIO_ASSERT(parsed.version == 2, "Certificate version should be v3 (2)");
  FIO_ASSERT(parsed.key_type == FIO_X509_KEY_ECDSA_P256,
             "Key type should be P-256");
  FIO_ASSERT(parsed.sig_alg == FIO_X509_SIG_ECDSA_SHA256,
             "Signature algorithm should be ECDSA-SHA256");

  /* Verify public key matches */
  FIO_ASSERT(parsed.pubkey.ecdsa.point != NULL, "Public key not parsed");
  FIO_ASSERT(parsed.pubkey.ecdsa.point_len == 65, "Public key length wrong");
  FIO_ASSERT(memcmp(parsed.pubkey.ecdsa.point, kp.public_key, 65) == 0,
             "Public key mismatch");

  /* Clean up */
  free(cert_buf);
  fio_x509_keypair_clear(&kp);

  fprintf(stderr, "\t  P-256 self-signed certificate tests passed\n");
}

/* *****************************************************************************
Test CA Certificate Generation
***************************************************************************** */

FIO_SFUNC void fio___test_x509_ca_certificate(void) {
  fprintf(stderr, "\t* Testing CA certificate generation\n");

  /* Generate key pair */
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0, "Key pair generation failed");

  /* Set up CA certificate options */
  fio_x509_cert_options_s opts = {
      .subject_cn = "Test CA",
      .subject_org = "Test Organization",
      .subject_org_len = 17,
      .is_ca = 1,
  };

  /* Generate certificate */
  size_t cert_len = fio_x509_self_signed_cert(NULL, 0, &kp, &opts);
  uint8_t *cert_buf = malloc(cert_len + 100);
  size_t actual_len =
      fio_x509_self_signed_cert(cert_buf, cert_len + 100, &kp, &opts);
  FIO_ASSERT(actual_len > 0, "CA certificate generation failed");

  /* Parse and verify */
  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert_buf, actual_len) == 0,
             "Failed to parse CA certificate");

  /* Verify CA flag */
  FIO_ASSERT(parsed.is_ca == 1, "CA flag should be set");

  /* Verify KeyUsage for CA */
  FIO_ASSERT(parsed.has_key_usage, "KeyUsage should be present");
  FIO_ASSERT(parsed.key_usage & FIO_X509_KU_KEY_CERT_SIGN,
             "keyCertSign should be set for CA");

  /* Verify self-signature */
  FIO_ASSERT(fio_x509_verify_signature(&parsed, &parsed) == 0,
             "CA self-signature verification failed");

  free(cert_buf);
  fio_x509_keypair_clear(&kp);

  fprintf(stderr, "\t  CA certificate tests passed\n");
}

/* *****************************************************************************
Test Certificate Chain Validation with Generated Certificates
***************************************************************************** */

FIO_SFUNC void fio___test_x509_chain_validation(void) {
  fprintf(stderr, "\t* Testing certificate chain with generated certs\n");

  /* Generate CA key pair and certificate */
  fio_x509_keypair_s ca_kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&ca_kp) == 0,
             "CA key pair generation failed");

  fio_x509_cert_options_s ca_opts = {
      .subject_cn = "Test Root CA",
      .is_ca = 1,
  };

  size_t ca_len = fio_x509_self_signed_cert(NULL, 0, &ca_kp, &ca_opts);
  uint8_t *ca_buf = malloc(ca_len + 100);
  size_t ca_actual =
      fio_x509_self_signed_cert(ca_buf, ca_len + 100, &ca_kp, &ca_opts);
  FIO_ASSERT(ca_actual > 0, "CA certificate generation failed");

  /* Parse CA certificate */
  fio_x509_cert_s ca_cert;
  FIO_ASSERT(fio_x509_parse(&ca_cert, ca_buf, ca_actual) == 0,
             "Failed to parse CA certificate");

  /* Verify CA certificate is self-signed and valid */
  FIO_ASSERT(fio_x509_verify_signature(&ca_cert, &ca_cert) == 0,
             "CA self-signature invalid");

  /* Set up trust store with our CA */
  const uint8_t *roots[] = {ca_buf};
  const size_t root_lens[] = {ca_actual};
  fio_x509_trust_store_s trust_store = {
      .roots = roots,
      .root_lens = root_lens,
      .root_count = 1,
  };

  /* Verify CA is in trust store */
  FIO_ASSERT(fio_x509_is_trusted(&ca_cert, &trust_store) == 0,
             "CA should be in trust store");

  /* Clean up */
  free(ca_buf);
  fio_x509_keypair_clear(&ca_kp);

  fprintf(stderr, "\t  Certificate chain validation tests passed\n");
}

/* *****************************************************************************
Test Error Handling
***************************************************************************** */

FIO_SFUNC void fio___test_x509_error_handling(void) {
  fprintf(stderr, "\t* Testing error handling\n");

  fio_x509_keypair_s kp;
  fio_x509_cert_options_s opts = {0};

  /* NULL keypair */
  FIO_ASSERT(fio_x509_keypair_ed25519(NULL) == -1,
             "Should fail with NULL keypair");

  /* NULL options */
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0, "Key pair generation failed");
  FIO_ASSERT(fio_x509_self_signed_cert(NULL, 0, &kp, NULL) == 0,
             "Should fail with NULL options");

  /* Missing subject CN */
  FIO_ASSERT(fio_x509_self_signed_cert(NULL, 0, &kp, &opts) == 0,
             "Should fail with missing subject CN");

  /* Valid options */
  opts.subject_cn = "test";
  size_t len = fio_x509_self_signed_cert(NULL, 0, &kp, &opts);
  FIO_ASSERT(len > 0, "Should succeed with valid options");

  /* Buffer too small */
  uint8_t small_buf[10];
  FIO_ASSERT(fio_x509_self_signed_cert(small_buf, 10, &kp, &opts) == 0,
             "Should fail with buffer too small");

  fio_x509_keypair_clear(&kp);

  fprintf(stderr, "\t  Error handling tests passed\n");
}

/* *****************************************************************************
Test Hostname Matching with Generated Certificate
***************************************************************************** */

FIO_SFUNC void fio___test_x509_hostname_matching(void) {
  fprintf(stderr, "\t* Testing hostname matching with generated cert\n");

  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0, "Key pair generation failed");

  /* Certificate with SAN */
  const char *san_dns[] = {"localhost", "*.example.com", "test.local"};
  fio_x509_cert_options_s opts = {
      .subject_cn = "localhost",
      .san_dns = san_dns,
      .san_dns_count = 3,
  };

  size_t cert_len = fio_x509_self_signed_cert(NULL, 0, &kp, &opts);
  uint8_t *cert_buf = malloc(cert_len + 100);
  size_t actual_len =
      fio_x509_self_signed_cert(cert_buf, cert_len + 100, &kp, &opts);
  FIO_ASSERT(actual_len > 0, "Certificate generation failed");

  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert_buf, actual_len) == 0,
             "Failed to parse certificate");

  /* Test exact match */
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "localhost", 9) == 0,
             "Should match 'localhost'");

  /* Test wildcard match */
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "www.example.com", 15) == 0,
             "Should match '*.example.com'");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "api.example.com", 15) == 0,
             "Should match '*.example.com'");

  /* Test non-match */
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "other.local", 11) != 0,
             "Should not match 'other.local'");

  free(cert_buf);
  fio_x509_keypair_clear(&kp);

  fprintf(stderr, "\t  Hostname matching tests passed\n");
}

/* *****************************************************************************
Main Test Runner
***************************************************************************** */

int main(void) {
  fprintf(stderr, "* Testing X.509 Certificate Generation\n");

  fio___test_asn1_encoding();
  fio___test_x509_keypair_ed25519();
  fio___test_x509_keypair_p256();
  fio___test_x509_self_signed_ed25519();
  fio___test_x509_self_signed_p256();
  fio___test_x509_ca_certificate();
  fio___test_x509_chain_validation();
  fio___test_x509_error_handling();
  fio___test_x509_hostname_matching();

  fprintf(stderr, "* All X.509 certificate generation tests passed!\n");
  return 0;
}
