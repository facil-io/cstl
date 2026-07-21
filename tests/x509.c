/* *****************************************************************************
Test for 156 x509.h

Coverage: X.509v3 certificate parsing, validity checking, hostname matching
(CN and SAN with wildcards), DN comparison, signature verification, basic
constraints / key usage extensions, certificate chain validation, trust store
matching, and TLS 1.3 Certificate message parsing. Self-signed certificates
are generated with fio_x509_self_signed_cert using Ed25519 and P-256 key pairs
so signatures are real and verifiable. Performance loops and external
references are omitted.
***************************************************************************** */
#define FIO_TIME
#define FIO_SHA2
#define FIO_MATH
#define FIO_DER
#define FIO_RSA
#define FIO_ED25519
#define FIO_P256
#define FIO_X509
#include "test-helpers.h"

/* *****************************************************************************
Static Test Certificate (minimal valid RSA parse-only)
***************************************************************************** */

/* Minimal self-signed RSA certificate structure for parse-only tests.
 * The signature is fake, so signature verification is not tested with this
 * certificate; signature tests use generated Ed25519/P-256 certs. */
static const uint8_t test_minimal_cert[] = {
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
    0x3D, 0x3E, 0x3F};

/* *****************************************************************************
Generated Certificate Helpers
***************************************************************************** */

FIO_SFUNC uint8_t *FIO_NAME_TEST(stl, x509_make_cert)(size_t *out_len,
                                                      fio_x509_keypair_s *kp,
                                                      const char *cn,
                                                      const char **san_dns,
                                                      size_t san_count,
                                                      int is_ca) {
  fio_buf_info_s san_buf[8];
  FIO_ASSERT(san_count <= 8, "too many SAN entries for test helper");
  for (size_t i = 0; i < san_count; ++i)
    san_buf[i] =
        FIO_BUF_INFO2((char *)san_dns[i], FIO_STRLEN(san_dns[i]));
  fio_x509_cert_options_s opts = {
      .cn = FIO_BUF_INFO2((char *)cn, FIO_STRLEN(cn)),
      .san_dns = san_buf,
      .san_dns_count = san_count,
      .is_ca = is_ca,
  };
  size_t need = fio_x509_self_signed_cert(NULL, 0, kp, &opts);
  FIO_ASSERT(need > 0, "certificate size calculation failed");
  uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, need + 64, 0);
  FIO_ASSERT(buf, "certificate allocation failed");
  size_t len = fio_x509_self_signed_cert(buf, need + 64, kp, &opts);
  FIO_ASSERT(len > 0 && len <= need + 64, "certificate generation failed");
  *out_len = len;
  return buf;
}

/* *****************************************************************************
Basic Parsing Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x509_parse_basic)(void) {
  fio_x509_cert_s cert;
  FIO_ASSERT(fio_x509_parse(&cert, test_minimal_cert,
                            sizeof(test_minimal_cert)) == 0,
             "minimal RSA certificate parse failed");
  FIO_ASSERT(cert.version == 2, "version should be v3 (2)");
  FIO_ASSERT(cert.sig_alg == FIO_X509_SIG_RSA_PKCS1_SHA256,
             "signature algorithm mismatch");
  FIO_ASSERT(cert.key_type == FIO_X509_KEY_RSA, "key type should be RSA");
  FIO_ASSERT(cert.pubkey.rsa.n.buf != NULL && cert.pubkey.rsa.n.len > 0,
             "RSA modulus not extracted");
  FIO_ASSERT(cert.pubkey.rsa.e.buf != NULL && cert.pubkey.rsa.e.len == 3,
             "RSA exponent not extracted");
  FIO_ASSERT(cert.not_before > 0 && cert.not_after > cert.not_before,
             "validity period not parsed");
  FIO_ASSERT(cert.cn.buf != NULL && cert.cn.len == 4 &&
                 !FIO_MEMCMP(cert.cn.buf, "test", 4),
             "subject CN mismatch");
  FIO_ASSERT(cert.tbs.buf != NULL && cert.tbs.len > 0,
             "TBS data not captured");
  FIO_ASSERT(cert.signature.buf != NULL && cert.signature.len > 0,
             "signature not extracted");
  FIO_ASSERT(cert.issuer.buf != NULL && cert.subject.buf != NULL,
             "Distinguished Names not captured");
  FIO_ASSERT(cert.serial.buf != NULL && cert.serial.len == 1 &&
                 cert.serial.buf[0] == 0x01,
             "serial number not captured");
  FIO_ASSERT(cert.san_dns.buf != NULL && cert.san_dns.len == 3 &&
                 !FIO_MEMCMP(cert.san_dns.buf, "www", 3),
             "SAN DNS missing");
  FIO_ASSERT(cert.san_ip.buf == NULL || cert.san_ip.len == 0,
             "SAN IP should be absent");
  FIO_ASSERT(cert.der.buf == test_minimal_cert &&
                 cert.der.len == sizeof(test_minimal_cert),
             "DER source should be referenced (not copied)");
  fio_x509_fingerprint(&cert);
  fio_u256 expected_fp =
      fio_sha256(test_minimal_cert, sizeof(test_minimal_cert));
  FIO_ASSERT(!FIO_MEMCMP(cert.fingerprint, expected_fp.u8, 32),
             "fingerprint should equal SHA-256 of DER");
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_generated_ed25519)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0,
             "Ed25519 key pair generation failed");

  const char *san[] = {"localhost", "127.0.0.1"};
  size_t cert_len;
  uint8_t *cert = FIO_NAME_TEST(stl, x509_make_cert)(
      &cert_len, &kp, "localhost", san, 2, 0);

  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert, cert_len) == 0,
             "generated Ed25519 certificate parse failed");
  FIO_ASSERT(parsed.version == 2, "version mismatch");
  FIO_ASSERT(parsed.key_type == FIO_X509_KEY_ED25519,
             "key type should be Ed25519");
  FIO_ASSERT(parsed.sig_alg == FIO_X509_SIG_ED25519,
             "signature algorithm should be Ed25519");
  FIO_ASSERT(parsed.pubkey.ed25519.key.buf != NULL &&
                 !FIO_MEMCMP(parsed.pubkey.ed25519.key.buf, kp.public_key, 32),
             "Ed25519 public key mismatch");
  FIO_ASSERT(parsed.cn.len == 9 &&
                 !FIO_MEMCMP(parsed.cn.buf, "localhost", 9),
             "subject CN mismatch");
  FIO_ASSERT(parsed.is_ca == 0, "end-entity should not be CA");
  FIO_ASSERT(parsed.has_key_usage &&
                 (parsed.key_usage & FIO_X509_KU_DIGITAL_SIGNATURE),
             "digitalSignature key usage missing");
  FIO_ASSERT(parsed.san_dns.buf != NULL, "SAN DNS missing");

  FIO_ASSERT(fio_x509_verify_signature(&parsed, &parsed) == 0,
             "Ed25519 self-signature verification failed");

  FIO_MEM_FREE(cert, cert_len + 64);
  fio_x509_keypair_clear(&kp);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_generated_p256)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_p256(&kp) == 0, "P-256 key pair generation failed");

  size_t cert_len;
  uint8_t *cert = FIO_NAME_TEST(stl, x509_make_cert)(
      &cert_len, &kp, "test.example.com", NULL, 0, 0);

  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert, cert_len) == 0,
             "generated P-256 certificate parse failed");
  FIO_ASSERT(parsed.version == 2, "version mismatch");
  FIO_ASSERT(parsed.key_type == FIO_X509_KEY_ECDSA_P256,
             "key type should be P-256");
  FIO_ASSERT(parsed.sig_alg == FIO_X509_SIG_ECDSA_SHA256,
             "signature algorithm should be ECDSA-SHA256");
  FIO_ASSERT(parsed.pubkey.ecdsa.point.buf != NULL &&
                 parsed.pubkey.ecdsa.point.len == 65 &&
                 !FIO_MEMCMP(parsed.pubkey.ecdsa.point.buf, kp.public_key, 65),
             "P-256 public key mismatch");
  FIO_ASSERT(fio_x509_verify_signature(&parsed, &parsed) == 0,
             "P-256 self-signature verification failed");

  FIO_MEM_FREE(cert, cert_len + 64);
  fio_x509_keypair_clear(&kp);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_ca_flag)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0,
             "Ed25519 key pair generation failed");

  size_t cert_len;
  uint8_t *cert = FIO_NAME_TEST(stl, x509_make_cert)(
      &cert_len, &kp, "Test CA", NULL, 0, 1);

  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert, cert_len) == 0,
             "CA certificate parse failed");
  FIO_ASSERT(parsed.is_ca == 1, "CA flag should be set");
  FIO_ASSERT(parsed.has_key_usage &&
                 (parsed.key_usage & FIO_X509_KU_KEY_CERT_SIGN),
             "keyCertSign key usage missing for CA");
  FIO_ASSERT(fio_x509_verify_signature(&parsed, &parsed) == 0,
             "CA self-signature verification failed");

  FIO_MEM_FREE(cert, cert_len + 64);
  fio_x509_keypair_clear(&kp);
}

/* *****************************************************************************
Validity and Hostname Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x509_validity)(void) {
  fio_x509_cert_s cert;
  FIO_ASSERT(fio_x509_parse(&cert, test_minimal_cert,
                            sizeof(test_minimal_cert)) == 0,
             "parse failed for validity test");

  FIO_ASSERT(fio_x509_check_validity(&cert, 1750000000) == 0,
             "certificate should be valid at 2025 timestamp");
  FIO_ASSERT(fio_x509_check_validity(&cert, 1925000000) == -1,
             "certificate should be expired");
  FIO_ASSERT(fio_x509_check_validity(&cert, 1546300800) == -1,
             "certificate should be not yet valid");
  FIO_ASSERT(fio_x509_check_validity(NULL, 1750000000) == -1,
             "NULL cert validity check should fail");
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_hostname)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0,
             "Ed25519 key pair generation failed");

  const char *san[] = {"localhost", "*.example.com", "test.local"};
  size_t cert_len;
  uint8_t *cert = FIO_NAME_TEST(stl, x509_make_cert)(
      &cert_len, &kp, "localhost", san, 3, 0);

  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert, cert_len) == 0,
             "parse failed for hostname test");

  FIO_ASSERT(fio_x509_match_hostname(&parsed, "localhost", 9) == 0,
             "exact SAN match failed");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "LOCALHOST", 9) == 0,
             "case-insensitive SAN match failed");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "www.example.com", 15) == 0,
             "wildcard SAN match failed");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "api.example.com", 15) == 0,
             "wildcard SAN match failed");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "example.com", 11) == -1,
             "wildcard should not match apex");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "sub.www.example.com", 19) == -1,
             "wildcard should not match multiple levels");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "other.local", 11) == -1,
             "non-matching SAN should fail");

  FIO_MEM_FREE(cert, cert_len + 64);
  fio_x509_keypair_clear(&kp);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_hostname_cn_fallback)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0,
             "Ed25519 key pair generation failed");

  size_t cert_len;
  uint8_t *cert = FIO_NAME_TEST(stl, x509_make_cert)(
      &cert_len, &kp, "cn-only.example.com", NULL, 0, 0);

  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert, cert_len) == 0,
             "parse failed for CN fallback test");
  FIO_ASSERT(parsed.san_ext.buf == NULL, "SAN should be absent");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "cn-only.example.com", 19) == 0,
             "CN exact match failed");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "CN-ONLY.EXAMPLE.COM", 19) == 0,
             "CN case-insensitive match failed");
  FIO_ASSERT(fio_x509_match_hostname(&parsed, "other.example.com", 17) == -1,
             "wrong CN should fail");

  FIO_MEM_FREE(cert, cert_len + 64);
  fio_x509_keypair_clear(&kp);
}

/* *****************************************************************************
DN and Trust Store Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x509_dn)(void) {
  uint8_t dn1[] = {0x30, 0x0F, 0x31, 0x0D, 0x30, 0x0B, 0x06, 0x03, 0x55,
                   0x04, 0x03, 0x0C, 0x04, 't',  'e',  's',  't'};
  uint8_t dn2[] = {0x30, 0x0F, 0x31, 0x0D, 0x30, 0x0B, 0x06, 0x03, 0x55,
                   0x04, 0x03, 0x0C, 0x04, 't',  'e',  's',  't'};
  uint8_t dn3[] = {0x30, 0x10, 0x31, 0x0E, 0x30, 0x0C, 0x06, 0x03, 0x55,
                   0x04, 0x03, 0x0C, 0x05, 'o',  't',  'h',  'e',  'r'};

  FIO_ASSERT(FIO_BUF_INFO_IS_EQ(FIO_UBUF_INFO2(dn1, sizeof(dn1)),
                                FIO_UBUF_INFO2(dn2, sizeof(dn2))),
             "identical DNs should be equal");
  FIO_ASSERT(!FIO_BUF_INFO_IS_EQ(FIO_UBUF_INFO2(dn1, sizeof(dn1)),
                                 FIO_UBUF_INFO2(dn3, sizeof(dn3))),
             "different DNs should not be equal");
  FIO_ASSERT(!FIO_BUF_INFO_IS_EQ(FIO_UBUF_INFO2(dn1, sizeof(dn1)),
                                 FIO_UBUF_INFO2(dn1, sizeof(dn1) - 1)),
             "different length DNs should not be equal");
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_trust_store)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0,
             "Ed25519 key pair generation failed");

  size_t cert_len;
  uint8_t *cert = FIO_NAME_TEST(stl, x509_make_cert)(
      &cert_len, &kp, "Test Root", NULL, 0, 1);

  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert, cert_len) == 0,
             "root cert parse failed");

  const uint8_t *roots[] = {cert};
  const size_t root_lens[] = {cert_len};
  fio_x509_trust_store_s trust = {.roots = roots,
                                  .root_lens = root_lens,
                                  .root_count = 1};
  FIO_ASSERT(fio_x509_is_trusted(&parsed, &trust) == 0,
             "self-signed root should be trusted");

  fio_x509_trust_store_s empty = {0};
  FIO_ASSERT(fio_x509_is_trusted(&parsed, &empty) == -1,
             "empty trust store should reject");
  FIO_ASSERT(fio_x509_is_trusted(NULL, &trust) == -1,
             "NULL cert should fail trust check");
  FIO_ASSERT(fio_x509_is_trusted(&parsed, NULL) == -1,
             "NULL trust store should fail");

  FIO_MEM_FREE(cert, cert_len + 64);
  fio_x509_keypair_clear(&kp);
}

/* *****************************************************************************
Chain Validation Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x509_chain_self_signed)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0,
             "Ed25519 key pair generation failed");

  size_t cert_len;
  uint8_t *cert = FIO_NAME_TEST(stl, x509_make_cert)(
      &cert_len, &kp, "Test Root", NULL, 0, 1);

  const uint8_t *certs[] = {cert};
  const size_t cert_lens[] = {cert_len};
  int64_t now = (int64_t)fio_time_real().tv_sec;

  int ret = fio_x509_verify_chain(certs, cert_lens, 1, NULL, now, NULL);
  FIO_ASSERT(ret == FIO_X509_OK,
             "self-signed chain without hostname/trust should pass: %s",
             fio_x509_error_str(ret));

  const uint8_t *roots[] = {cert};
  const size_t root_lens[] = {cert_len};
  fio_x509_trust_store_s trust = {.roots = roots,
                                  .root_lens = root_lens,
                                  .root_count = 1};
  ret = fio_x509_verify_chain(certs, cert_lens, 1, "Test Root", now, &trust);
  FIO_ASSERT(ret == FIO_X509_OK,
             "self-signed chain with trust and hostname should pass: %s",
             fio_x509_error_str(ret));

  ret = fio_x509_verify_chain(certs, cert_lens, 1, "wrong", now, &trust);
  FIO_ASSERT(ret == FIO_X509_ERR_HOSTNAME_MISMATCH,
             "wrong hostname should fail");

  FIO_MEM_FREE(cert, cert_len + 64);
  fio_x509_keypair_clear(&kp);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_chain_invalid_inputs)(void) {
  const uint8_t *certs[] = {test_minimal_cert};
  const size_t cert_lens[] = {sizeof(test_minimal_cert)};

  FIO_ASSERT(fio_x509_verify_chain(NULL, cert_lens, 1, NULL, 1750000000,
                                    NULL) == FIO_X509_ERR_PARSE,
             "NULL certs should return PARSE");
  FIO_ASSERT(fio_x509_verify_chain(certs, NULL, 1, NULL, 1750000000, NULL) ==
                 FIO_X509_ERR_PARSE,
             "NULL cert_lens should return PARSE");
  FIO_ASSERT(fio_x509_verify_chain(certs, cert_lens, 0, NULL, 1750000000,
                                    NULL) == FIO_X509_ERR_EMPTY_CHAIN,
             "empty chain should return EMPTY_CHAIN");
}

/* *****************************************************************************
TLS Certificate Message Parsing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x509_tls_cert_message)(void) {
  size_t cert_len = sizeof(test_minimal_cert);
  size_t entry_len = 3 + cert_len + 2;
  size_t msg_len = 1 + 3 + entry_len;
  uint8_t *msg = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, msg_len, 0);
  FIO_ASSERT(msg, "TLS Certificate message allocation failed");

  uint8_t *p = msg;
  *p++ = 0; /* request_context length */
  *p++ = (uint8_t)((entry_len >> 16) & 0xFF);
  *p++ = (uint8_t)((entry_len >> 8) & 0xFF);
  *p++ = (uint8_t)(entry_len & 0xFF);
  *p++ = (uint8_t)((cert_len >> 16) & 0xFF);
  *p++ = (uint8_t)((cert_len >> 8) & 0xFF);
  *p++ = (uint8_t)(cert_len & 0xFF);
  FIO_MEMCPY(p, test_minimal_cert, cert_len);
  p += cert_len;
  *p++ = 0;
  *p++ = 0; /* extensions length */

  fio_tls_cert_entry_s entries[5];
  int count =
      (int)fio_tls_parse_certificate_message(entries, 5, msg, msg_len);
  FIO_ASSERT(count == 1, "expected 1 certificate entry, got %d", count);
  FIO_ASSERT(entries[0].cert_len == cert_len, "certificate length mismatch");
  FIO_ASSERT(!FIO_MEMCMP(entries[0].cert, test_minimal_cert, cert_len),
             "certificate data mismatch");

  FIO_ASSERT(fio_tls_parse_certificate_message(NULL, 5, msg, msg_len) ==
                 FIO_TLS_CERT_PARSE_ERROR,
             "NULL entries should fail");
  FIO_ASSERT(fio_tls_parse_certificate_message(entries, 0, msg, msg_len) ==
                 FIO_TLS_CERT_PARSE_ERROR,
             "zero max_entries should fail");
  FIO_ASSERT(fio_tls_parse_certificate_message(entries, 5, NULL, msg_len) ==
                 FIO_TLS_CERT_PARSE_ERROR,
             "NULL data should fail");
  FIO_ASSERT(fio_tls_parse_certificate_message(entries, 5, msg, 3) ==
                 FIO_TLS_CERT_PARSE_ERROR,
             "truncated message should fail");

  FIO_MEM_FREE(msg, msg_len);
}

/* *****************************************************************************
Error Strings and Invalid Inputs
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x509_error_strings)(void) {
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_OK)) > 0,
             "OK error string missing");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_PARSE)) > 0,
             "PARSE error string missing");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_SIGNATURE)) > 0,
             "SIGNATURE error string missing");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_HOSTNAME_MISMATCH)) >
                 0,
             "HOSTNAME_MISMATCH error string missing");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(-999)) > 0,
             "unknown error string missing");
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_invalid_inputs)(void) {
  fio_x509_cert_s cert;
  FIO_ASSERT(fio_x509_parse(NULL, test_minimal_cert,
                            sizeof(test_minimal_cert)) == -1,
             "NULL cert should fail");
  FIO_ASSERT(fio_x509_parse(&cert, NULL, 100) == -1, "NULL data should fail");
  FIO_ASSERT(fio_x509_parse(&cert, test_minimal_cert, 0) == -1,
             "zero length should fail");
  FIO_ASSERT(fio_x509_parse(&cert, test_minimal_cert, 10) == -1,
             "truncated data should fail");

  FIO_ASSERT(fio_x509_match_hostname(NULL, "test", 4) == -1,
             "NULL cert hostname match should fail");
  FIO_MEMSET(&cert, 0, sizeof(cert));
  FIO_ASSERT(fio_x509_match_hostname(&cert, NULL, 4) == -1,
             "NULL hostname should fail");
  FIO_ASSERT(fio_x509_match_hostname(&cert, "test", 0) == -1,
             "zero hostname length should fail");

  /* Key pair generation error handling */
  FIO_ASSERT(fio_x509_keypair_ed25519(NULL) == -1,
             "NULL keypair should fail");
  FIO_ASSERT(fio_x509_keypair_p256(NULL) == -1,
             "NULL P-256 keypair should fail");

  /* Certificate generation error handling */
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0,
             "Ed25519 key pair generation failed");
  FIO_ASSERT(fio_x509_self_signed_cert(NULL, 0, &kp, NULL) == 0,
             "NULL options should fail");
  fio_x509_cert_options_s opts = {0};
  FIO_ASSERT(fio_x509_self_signed_cert(NULL, 0, &kp, &opts) == 0,
             "missing subject CN should fail");

  opts.cn = FIO_BUF_INFO2((char *)"test", 4);
  size_t len = fio_x509_self_signed_cert(NULL, 0, &kp, &opts);
  FIO_ASSERT(len > 0, "valid options should succeed");

  uint8_t small_buf[10];
  FIO_ASSERT(fio_x509_self_signed_cert(small_buf, 10, &kp, &opts) == 0,
             "buffer too small should fail");

  fio_x509_keypair_clear(&kp);
  FIO_ASSERT(kp.type == 0, "key pair not cleared");
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  FIO_NAME_TEST(stl, x509_parse_basic)();
  FIO_NAME_TEST(stl, x509_generated_ed25519)();
  FIO_NAME_TEST(stl, x509_generated_p256)();
  FIO_NAME_TEST(stl, x509_ca_flag)();
  FIO_NAME_TEST(stl, x509_validity)();
  FIO_NAME_TEST(stl, x509_hostname)();
  FIO_NAME_TEST(stl, x509_hostname_cn_fallback)();
  FIO_NAME_TEST(stl, x509_dn)();
  FIO_NAME_TEST(stl, x509_trust_store)();
  FIO_NAME_TEST(stl, x509_chain_self_signed)();
  FIO_NAME_TEST(stl, x509_chain_invalid_inputs)();
  FIO_NAME_TEST(stl, x509_tls_cert_message)();
  FIO_NAME_TEST(stl, x509_error_strings)();
  FIO_NAME_TEST(stl, x509_invalid_inputs)();
  return 0;
}
