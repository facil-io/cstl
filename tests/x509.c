/* *****************************************************************************
Test for 156 x509.h

Coverage: X.509v3 certificate parsing, validity checking, hostname matching
(CN and SAN with wildcards), DN comparison, signature verification, basic
constraints / key usage extensions, certificate chain validation, trust store
matching, and TLS 1.3 Certificate message parsing. Self-signed certificates
are generated with fio_x509_self_signed_cert using Ed25519 and P-256 key pairs
so signatures are real and verifiable. OpenSSL-generated P-256 / P-384 chains
with compressed public keys (SEC 1 02/03 || x) cover point decompression in
signature and chain verification. Performance loops and external references
are omitted.
***************************************************************************** */
#define FIO_TIME
#define FIO_SHA2
#define FIO_MATH
#define FIO_DER
#define FIO_RSA
#define FIO_ED25519
#define FIO_P256
#define FIO_P384
#define FIO_X509
#define FIO_PEM
#include "test-helpers.h"

#include "ec-compressed-fixtures.h"

/* *****************************************************************************
Static Test Certificate (minimal valid RSA parse-only)
***************************************************************************** */

/* clang-format off */

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

/* clang-format on */
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
    san_buf[i] = FIO_BUF_INFO2((char *)san_dns[i], FIO_STRLEN(san_dns[i]));
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
  FIO_ASSERT(
      fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert)) == 0,
      "minimal RSA certificate parse failed");
  FIO_ASSERT(cert.version == 2, "version should be v3 (2)");
  FIO_ASSERT(cert.signature_algo == FIO_X509_SIGNATURE_RSA_PKCS1_SHA256,
             "signature algorithm mismatch");
  FIO_ASSERT(cert.key_algo == FIO_X509_KEY_RSA, "key type should be RSA");
  FIO_ASSERT(cert.pubkey.rsa.n.buf != NULL && cert.pubkey.rsa.n.len > 0,
             "RSA modulus not extracted");
  FIO_ASSERT(cert.pubkey.rsa.e.buf != NULL && cert.pubkey.rsa.e.len == 3,
             "RSA exponent not extracted");
  FIO_ASSERT(cert.not_before > 0 && cert.not_after > cert.not_before,
             "validity period not parsed");
  FIO_ASSERT(cert.cn.buf != NULL && cert.cn.len == 4 &&
                 !FIO_MEMCMP(cert.cn.buf, "test", 4),
             "subject CN mismatch");
  FIO_ASSERT(cert.tbs.buf != NULL && cert.tbs.len > 0, "TBS data not captured");
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
  uint8_t *cert =
      FIO_NAME_TEST(stl,
                    x509_make_cert)(&cert_len, &kp, "localhost", san, 2, 0);

  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert, cert_len) == 0,
             "generated Ed25519 certificate parse failed");
  FIO_ASSERT(parsed.version == 2, "version mismatch");
  FIO_ASSERT(parsed.key_algo == FIO_X509_KEY_ED25519,
             "key type should be Ed25519");
  FIO_ASSERT(parsed.signature_algo == FIO_X509_SIGNATURE_ED25519,
             "signature algorithm should be Ed25519");
  FIO_ASSERT(parsed.pubkey.ed25519.key.buf != NULL &&
                 !FIO_MEMCMP(parsed.pubkey.ed25519.key.buf, kp.public_key, 32),
             "Ed25519 public key mismatch");
  FIO_ASSERT(parsed.cn.len == 9 && !FIO_MEMCMP(parsed.cn.buf, "localhost", 9),
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
  FIO_ASSERT(fio_x509_keypair_p256(&kp) == 0,
             "P-256 key pair generation failed");

  size_t cert_len;
  uint8_t *cert = FIO_NAME_TEST(
      stl,
      x509_make_cert)(&cert_len, &kp, "test.example.com", NULL, 0, 0);

  fio_x509_cert_s parsed;
  FIO_ASSERT(fio_x509_parse(&parsed, cert, cert_len) == 0,
             "generated P-256 certificate parse failed");
  FIO_ASSERT(parsed.version == 2, "version mismatch");
  FIO_ASSERT(parsed.key_algo == FIO_X509_KEY_ECDSA_P256,
             "key type should be P-256");
  FIO_ASSERT(parsed.signature_algo == FIO_X509_SIGNATURE_ECDSA_SHA256,
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
  uint8_t *cert =
      FIO_NAME_TEST(stl, x509_make_cert)(&cert_len, &kp, "Test CA", NULL, 0, 1);

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
  FIO_ASSERT(
      fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert)) == 0,
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
  uint8_t *cert =
      FIO_NAME_TEST(stl,
                    x509_make_cert)(&cert_len, &kp, "localhost", san, 3, 0);

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
  uint8_t *cert = FIO_NAME_TEST(
      stl,
      x509_make_cert)(&cert_len, &kp, "cn-only.example.com", NULL, 0, 0);

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
  /* clang-format off */
  uint8_t dn1[] = {0x30, 0x0F, 0x31, 0x0D, 0x30, 0x0B, 0x06, 0x03, 0x55,
                   0x04, 0x03, 0x0C, 0x04, 't',  'e',  's',  't'};
  uint8_t dn2[] = {0x30, 0x0F, 0x31, 0x0D, 0x30, 0x0B, 0x06, 0x03, 0x55,
                   0x04, 0x03, 0x0C, 0x04, 't',  'e',  's',  't'};
  uint8_t dn3[] = {0x30, 0x10, 0x31, 0x0E, 0x30, 0x0C, 0x06, 0x03, 0x55,
                   0x04, 0x03, 0x0C, 0x05, 'o',  't',  'h',  'e',  'r'};
  /* clang-format on */

  FIO_ASSERT(FIO_UBUF_INFO_IS_EQ(FIO_UBUF_INFO2(dn1, sizeof(dn1)),
                                 FIO_UBUF_INFO2(dn2, sizeof(dn2))),
             "identical DNs should be equal");
  FIO_ASSERT(!FIO_UBUF_INFO_IS_EQ(FIO_UBUF_INFO2(dn1, sizeof(dn1)),
                                  FIO_UBUF_INFO2(dn3, sizeof(dn3))),
             "different DNs should not be equal");
  FIO_ASSERT(!FIO_UBUF_INFO_IS_EQ(FIO_UBUF_INFO2(dn1, sizeof(dn1)),
                                  FIO_UBUF_INFO2(dn1, sizeof(dn1) - 1)),
             "different length DNs should not be equal");
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_trust_store)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(fio_x509_keypair_ed25519(&kp) == 0,
             "Ed25519 key pair generation failed");

  size_t cert_len;
  uint8_t *cert =
      FIO_NAME_TEST(stl,
                    x509_make_cert)(&cert_len, &kp, "Test Root", NULL, 0, 1);

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

  /* a TLS leaf needs digitalSignature (a CA-only key usage is rejected) */
  size_t cert_len;
  uint8_t *cert =
      FIO_NAME_TEST(stl,
                    x509_make_cert)(&cert_len, &kp, "Test Root", NULL, 0, 0);

  const fio_ubuf_info_s certs[] = {FIO_UBUF_INFO2(cert, cert_len)};
  int64_t now = (int64_t)fio_time_real().tv_sec;

  int ret = fio_x509_verify_chain(certs, 1, NULL, now, NULL);
  FIO_ASSERT(ret == FIO_X509_OK,
             "self-signed chain without hostname/trust should pass: %s",
             fio_x509_error_str(ret));

  const uint8_t *roots[] = {cert};
  const size_t root_lens[] = {cert_len};
  fio_x509_trust_store_s trust = {.roots = roots,
                                  .root_lens = root_lens,
                                  .root_count = 1};
  ret = fio_x509_verify_chain(certs, 1, "Test Root", now, &trust);
  FIO_ASSERT(ret == FIO_X509_OK,
             "self-signed chain with trust and hostname should pass: %s",
             fio_x509_error_str(ret));

  ret = fio_x509_verify_chain(certs, 1, "wrong", now, &trust);
  FIO_ASSERT(ret == FIO_X509_ERR_HOSTNAME_MISMATCH,
             "wrong hostname should fail");

  FIO_MEM_FREE(cert, cert_len + 64);
  fio_x509_keypair_clear(&kp);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_chain_invalid_inputs)(void) {
  const fio_ubuf_info_s certs[] = {
      FIO_UBUF_INFO2((uint8_t *)test_minimal_cert, sizeof(test_minimal_cert))};

  FIO_ASSERT(fio_x509_verify_chain(NULL, 1, NULL, 1750000000, NULL) ==
                 FIO_X509_ERR_PARSE,
             "NULL certs should return PARSE");
  FIO_ASSERT(fio_x509_verify_chain(certs, 0, NULL, 1750000000, NULL) ==
                 FIO_X509_ERR_EMPTY_CHAIN,
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
  int count = (int)fio_tls_parse_certificate_message(entries, 5, msg, msg_len);
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
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(FIO_X509_ERR_HOSTNAME_MISMATCH)) > 0,
             "HOSTNAME_MISMATCH error string missing");
  FIO_ASSERT(FIO_STRLEN(fio_x509_error_str(-999)) > 0,
             "unknown error string missing");
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_invalid_inputs)(void) {
  fio_x509_cert_s cert;
  FIO_ASSERT(
      fio_x509_parse(NULL, test_minimal_cert, sizeof(test_minimal_cert)) == -1,
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
  FIO_ASSERT(fio_x509_keypair_ed25519(NULL) == -1, "NULL keypair should fail");
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
Strict DER Structure Tests (trailing data, extra elements, BIT STRING padding)
***************************************************************************** */

/** Returns 0 if a mutated copy of test_minimal_cert parses. */
FIO_SFUNC int FIO_NAME_TEST(stl, x509_parse_mutant)(const uint8_t *der,
                                                    size_t len) {
  fio_x509_cert_s cert;
  return fio_x509_parse(&cert, der, len);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_strict_der)(void) {
  const size_t len = sizeof(test_minimal_cert);
  uint8_t buf[sizeof(test_minimal_cert) + 8];
  FIO_ASSERT(len == 0x12F, "test certificate layout changed");
  const size_t tbs_end = 4 + 3 + 0xD6; /* outer header + TBS TLV */
  const size_t sig_unused = len - 0x41; /* signature BIT STRING unused byte */

#define X509_MUTANT_RESET() FIO_MEMCPY(buf, test_minimal_cert, len)
  X509_MUTANT_RESET();
  FIO_ASSERT(!FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
             "unmodified certificate should parse");

  /* trailing byte after the Certificate: not part of it, reported by der */
  buf[len] = 0;
  {
    fio_x509_cert_s c;
    FIO_ASSERT(!fio_x509_parse(&c, buf, len + 1) && c.der.buf == buf &&
                   c.der.len == len,
               "cert->der should cover only the Certificate TLV");
  }

  /* extra element inside the Certificate SEQUENCE */
  X509_MUTANT_RESET();
  buf[3] += 2;
  buf[len] = 0x05;
  buf[len + 1] = 0x00;
  FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len + 2),
             "extra Certificate element must be rejected");

  /* extra element at the end of TBSCertificate */
  X509_MUTANT_RESET();
  buf[3] += 2;
  buf[6] += 2;
  FIO_MEMMOVE(buf + tbs_end + 2, buf + tbs_end, len - tbs_end);
  buf[tbs_end] = 0x05;
  buf[tbs_end + 1] = 0x00;
  FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len + 2),
             "extra TBSCertificate element must be rejected");

  /* signature BIT STRING with unused bits */
  X509_MUTANT_RESET();
  FIO_ASSERT(buf[sig_unused - 2] == 0x03 && buf[sig_unused] == 0,
             "signature BIT STRING offset");
  buf[sig_unused] = 1;
  FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
             "signature BIT STRING unused bits must be rejected");

  /* subjectPublicKey BIT STRING with unused bits (03 4B 00 30 48) */
  X509_MUTANT_RESET();
  for (size_t i = 0; i + 4 < len; ++i) {
    if (buf[i] == 0x03 && buf[i + 1] == 0x4B && buf[i + 2] == 0x00 &&
        buf[i + 3] == 0x30) {
      buf[i + 2] = 1;
      break;
    }
  }
  FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
             "subjectPublicKey BIT STRING unused bits must be rejected");

  /* TBS signature algorithm differs from the outer signatureAlgorithm */
  X509_MUTANT_RESET();
  FIO_ASSERT(buf[27] == 0x0B && buf[26] == 0x01, "TBS algorithm offset");
  buf[27] = 0x0C;
  FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
             "inner / outer AlgorithmIdentifier mismatch must be rejected");

  /* non-minimal INTEGERs decode unambiguously (tolerated); invalid values
   * (negative / even RSA parameters) are rejected */
#define X509_MUTANT_INSERT(at, byte)                                           \
  do {                                                                         \
    X509_MUTANT_RESET();                                                       \
    buf[3] += 1;                                                               \
    buf[6] += 1;                                                               \
    FIO_MEMMOVE(buf + (at) + 1, buf + (at), len - (at));                       \
    buf[(at)] = (byte);                                                        \
  } while (0)
  FIO_ASSERT(buf[12] == 0x02 && buf[13] == 0x01 && buf[14] == 0x01,
             "serial offset");
  X509_MUTANT_INSERT(14, 0x00); /* serial 02 02 00 01 */
  buf[13] = 2;
  FIO_ASSERT(!FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len + 1),
             "non-minimal serial (leading 0x00) should parse");
  X509_MUTANT_INSERT(14, 0xFF); /* serial 02 02 FF 81 */
  buf[13] = 2;
  buf[15] = 0x81;
  FIO_ASSERT(!FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len + 1),
             "non-minimal serial (leading 0xFF) should parse");
  X509_MUTANT_INSERT(14, 0x00); /* serial 02 02 00 81 (minimal, positive) */
  buf[13] = 2;
  buf[15] = 0x81;
  FIO_ASSERT(!FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len + 1),
             "minimal two byte serial should parse");
  X509_MUTANT_RESET();
  buf[14] = 0xFF; /* negative serial: tolerated per RFC 5280 4.1.2.2 */
  FIO_ASSERT(!FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
             "negative (but minimal) serial should parse");
  X509_MUTANT_INSERT(11, 0x00); /* version A0 04 02 02 00 02 */
  buf[8] = 4;
  buf[10] = 2;
  {
    fio_x509_cert_s c;
    FIO_ASSERT(!fio_x509_parse(&c, buf, len + 1) && c.version == 2,
               "non-minimal version should parse as v3");
  }
  /* signed TBS bytes may be non-minimal (the signature fixes them) */
  X509_MUTANT_INSERT(16, 0x81); /* TBS algorithm 30 81 0D: same, re-encoded */
  FIO_ASSERT(buf[15] == 0x30 && buf[17] == 0x0D, "TBS algorithm offset");
  FIO_ASSERT(!FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len + 1),
             "non-minimal header inside the signed TBS should parse");
#undef X509_MUTANT_INSERT

  /* unsigned envelope: non-minimal headers decode unambiguously and are
   * accepted. The fingerprint stays the hash of the exact bytes, so it
   * changes: fingerprints identify bytes, not certificates. */
#define X509_MUTANT_OUTER_INSERT(at, byte)                                     \
  do {                                                                         \
    X509_MUTANT_RESET();                                                       \
    buf[3] += 1;                                                               \
    FIO_MEMMOVE(buf + (at) + 1, buf + (at), len - (at));                       \
    buf[(at)] = (byte);                                                        \
  } while (0)
#define X509_MUTANT_SAME(msg)                                                  \
  do {                                                                         \
    fio_x509_cert_s c;                                                         \
    FIO_ASSERT(!fio_x509_parse(&c, buf, len + 1) && c.der.len == len + 1 &&   \
                   c.signature_algo == ref.signature_algo &&                   \
                   FIO_UBUF_INFO_IS_EQ(c.signature, ref.signature) &&          \
                   FIO_UBUF_INFO_IS_EQ(c.tbs, ref.tbs),                        \
               msg);                                                           \
    fio_x509_fingerprint(&c);                                                  \
    FIO_ASSERT(FIO_MEMCMP(c.fingerprint, ref.fingerprint, 32),                 \
               "fingerprint must hash the exact bytes (%s)",                   \
               msg);                                                           \
  } while (0)
  {
    const size_t alg_at = sig_unused - 2 - 15; /* outer 30 0D 06 09 .. 05 00 */
    fio_x509_cert_s ref;
    FIO_ASSERT(!fio_x509_parse(&ref, test_minimal_cert, len),
               "reference parse");
    fio_x509_fingerprint(&ref);
    X509_MUTANT_RESET();
    FIO_ASSERT(buf[alg_at] == 0x30 && buf[alg_at + 1] == 0x0D &&
                   buf[alg_at + 13] == 0x05 && buf[sig_unused - 1] == 0x41,
               "outer envelope offsets");
    /* 30 82 01 2B -> 30 83 00 01 2B */
    X509_MUTANT_RESET();
    FIO_MEMMOVE(buf + 3, buf + 2, len - 2);
    buf[1] = 0x83;
    buf[2] = 0x00;
    {
      fio_x509_cert_s c;
      FIO_ASSERT(!fio_x509_parse(&c, buf, len + 1) && c.der.len == len + 1 &&
                     FIO_UBUF_INFO_IS_EQ(c.tbs, ref.tbs),
                 "non-minimal Certificate length should parse");
      fio_x509_fingerprint(&c);
      FIO_ASSERT(FIO_MEMCMP(c.fingerprint, ref.fingerprint, 32),
                 "fingerprint must hash the exact bytes");
    }
    X509_MUTANT_OUTER_INSERT(alg_at + 1, 0x81); /* 30 81 0D */
    X509_MUTANT_SAME("non-minimal signatureAlgorithm length should parse");
    X509_MUTANT_OUTER_INSERT(alg_at + 14, 0x81); /* 30 0E .. 05 81 00 */
    buf[alg_at + 1] += 1;
    X509_MUTANT_SAME("equivalent signatureAlgorithm encoding should parse");
    X509_MUTANT_OUTER_INSERT(sig_unused - 1, 0x81); /* 03 81 41 */
    X509_MUTANT_SAME("non-minimal signatureValue length should parse");

    /* unsupported algorithms: byte equality required */
    X509_MUTANT_RESET();
    buf[27] = 0x7F; /* inner OID 1.2.840.113549.1.1.127 (unsupported) */
    buf[alg_at + 12] = 0x7F;
    {
      fio_x509_cert_s c;
      FIO_ASSERT(!fio_x509_parse(&c, buf, len) &&
                     c.signature_algo == FIO_X509_SIGNATURE_UNKNOWN,
                 "byte-equal unsupported algorithms should parse (UNKNOWN)");
    }
    buf[alg_at + 12] = 0x7E;
    FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
               "differing unsupported algorithms must be rejected");
  }
#undef X509_MUTANT_SAME
#undef X509_MUTANT_OUTER_INSERT
  {
    X509_MUTANT_RESET();
    size_t n_at = 0, e_at = 0; /* RSAPublicKey: 02 41 00 <n...> 02 03 01 00 01 */
    for (size_t i = 0; i + 5 < len; ++i) {
      if (!n_at && buf[i] == 0x02 && buf[i + 1] == 0x41 && buf[i + 2] == 0x00)
        n_at = i + 2;
      if (buf[i] == 0x02 && buf[i + 1] == 0x03 && buf[i + 2] == 0x01 &&
          buf[i + 3] == 0x00 && buf[i + 4] == 0x01)
        e_at = i + 2;
    }
    FIO_ASSERT(n_at && e_at && (buf[n_at + 1] & 0x80) && (buf[e_at - 3] & 1),
               "RSA key offsets");
    buf[n_at + 1] = 0x00; /* 00 00 C3..: two redundant zeros, then 0x52 */
    buf[n_at + 2] &= 0x7F;
    {
      fio_x509_cert_s c;
      FIO_ASSERT(!fio_x509_parse(&c, buf, len) && c.pubkey.rsa.n.len == 63 &&
                     c.pubkey.rsa.n.buf[0] == buf[n_at + 2],
                 "non-minimal RSA modulus should parse with zeros stripped");
    }
    X509_MUTANT_RESET();
    buf[n_at] = 0x80; /* negative modulus */
    FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
               "negative RSA modulus must be rejected");
    X509_MUTANT_RESET();
    buf[e_at - 3] &= 0xFE; /* even modulus */
    FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
               "even RSA modulus must be rejected");
    X509_MUTANT_RESET();
    buf[e_at + 2] = 0x00; /* even exponent 0x010000 */
    FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
               "even RSA exponent must be rejected");
    X509_MUTANT_RESET();
    buf[e_at] = 0x81; /* negative exponent */
    FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
               "negative RSA exponent must be rejected");
    X509_MUTANT_RESET();
    buf[e_at] = 0x00; /* 02 03 00 00 01: e = 1 */
    FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(buf, len),
               "RSA exponent 1 (zero padded) must be rejected");
    X509_MUTANT_RESET();
    buf[e_at] = 0x00; /* 02 03 00 01 01: e = 257, non-minimal */
    buf[e_at + 1] = 0x01;
    {
      fio_x509_cert_s c;
      FIO_ASSERT(!fio_x509_parse(&c, buf, len) && c.pubkey.rsa.e.len == 2,
                 "non-minimal RSA exponent should parse with zeros stripped");
    }
  }

  /* every proper prefix fails (no reads past the end under ASan) */
  for (size_t i = 0; i < len; ++i) {
    uint8_t *tmp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, i + 1, 0);
    FIO_ASSERT(tmp, "allocation failed");
    FIO_MEMCPY(tmp, test_minimal_cert, i);
    FIO_ASSERT(FIO_NAME_TEST(stl, x509_parse_mutant)(tmp, i),
               "truncated certificate (%zu bytes) should fail",
               i);
    FIO_MEM_FREE(tmp, i + 1);
  }
#undef X509_MUTANT_RESET
}

/* *****************************************************************************
DER Certificate Builder (Ed25519) for chain policy tests
***************************************************************************** */

typedef struct {
  uint8_t b[1536];
  size_t n;
} x509_tb_s;

FIO_SFUNC void x509_tb_raw(x509_tb_s *o, const void *d, size_t n) {
  FIO_ASSERT(o->n + n <= sizeof(o->b), "test builder overflow");
  if (n)
    FIO_MEMCPY(o->b + o->n, d, n);
  o->n += n;
}

FIO_SFUNC void x509_tb_tlv(x509_tb_s *o, uint8_t tag, const void *d, size_t n) {
  uint8_t h[4];
  size_t hl = 0;
  h[hl++] = tag;
  if (n < 128) {
    h[hl++] = (uint8_t)n;
  } else if (n < 256) {
    h[hl++] = 0x81;
    h[hl++] = (uint8_t)n;
  } else {
    h[hl++] = 0x82;
    h[hl++] = (uint8_t)(n >> 8);
    h[hl++] = (uint8_t)n;
  }
  x509_tb_raw(o, h, hl);
  x509_tb_raw(o, d, n);
}

FIO_SFUNC void x509_tb_wrap(x509_tb_s *o, uint8_t tag, const x509_tb_s *in) {
  x509_tb_tlv(o, tag, in->b, in->n);
}

/** Name ::= SEQUENCE { SET { SEQUENCE { CN OID, UTF8String } } } */
FIO_SFUNC void x509_tb_name(x509_tb_s *o, const char *cn) {
  static const uint8_t cn_oid[] = {0x06, 0x03, 0x55, 0x04, 0x03};
  x509_tb_s atv = {0}, set = {0}, seq = {0};
  x509_tb_raw(&atv, cn_oid, sizeof(cn_oid));
  x509_tb_tlv(&atv, 0x0C, cn, FIO_STRLEN(cn));
  x509_tb_wrap(&set, 0x30, &atv);
  x509_tb_wrap(&seq, 0x31, &set);
  x509_tb_wrap(o, 0x30, &seq);
}

/** Appends Extension { OID, [critical], OCTET STRING { value } } */
FIO_SFUNC void x509_tb_ext(x509_tb_s *exts,
                           const uint8_t *oid,
                           size_t oid_len,
                           int critical,
                           const x509_tb_s *value) {
  static const uint8_t crit[] = {0x01, 0x01, 0xFF};
  x509_tb_s e = {0};
  x509_tb_tlv(&e, 0x06, oid, oid_len);
  if (critical)
    x509_tb_raw(&e, crit, sizeof(crit));
  x509_tb_wrap(&e, 0x04, value);
  x509_tb_wrap(exts, 0x30, &e);
}

static const uint8_t x509_tb_oid_bc[] = {0x55, 0x1D, 0x13};
static const uint8_t x509_tb_oid_ku[] = {0x55, 0x1D, 0x0F};
static const uint8_t x509_tb_oid_eku[] = {0x55, 0x1D, 0x25};
static const uint8_t x509_tb_oid_san[] = {0x55, 0x1D, 0x11};
static const uint8_t x509_tb_oid_nc[] = {0x55, 0x1D, 0x1E};
static const uint8_t x509_tb_oid_unknown[] = {0x2A, 0x03, 0x04}; /* 1.2.3.4 */

FIO_SFUNC void x509_tb_ext_bc(x509_tb_s *exts, int ca, int path_len) {
  static const uint8_t t[] = {0x01, 0x01, 0xFF};
  x509_tb_s c = {0}, v = {0};
  if (ca)
    x509_tb_raw(&c, t, sizeof(t));
  if (path_len >= 0) {
    uint8_t pl = (uint8_t)path_len;
    x509_tb_tlv(&c, 0x02, &pl, 1);
  }
  x509_tb_wrap(&v, 0x30, &c);
  x509_tb_ext(exts, x509_tb_oid_bc, 3, 1, &v);
}

FIO_SFUNC void x509_tb_ext_ku(x509_tb_s *exts, uint8_t bits) {
  uint8_t c[2] = {0, bits};
  while (c[0] < 7 && !(bits & (1U << c[0])))
    ++c[0];
  x509_tb_s v = {0};
  x509_tb_tlv(&v, 0x03, c, 2);
  x509_tb_ext(exts, x509_tb_oid_ku, 3, 1, &v);
}

FIO_SFUNC void x509_tb_ext_eku(x509_tb_s *exts, uint8_t last_arc) {
  uint8_t oid[] = {0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, last_arc};
  x509_tb_s c = {0}, v = {0};
  x509_tb_tlv(&c, 0x06, oid, sizeof(oid));
  x509_tb_wrap(&v, 0x30, &c);
  x509_tb_ext(exts, x509_tb_oid_eku, 3, 0, &v);
}

FIO_SFUNC void x509_tb_ext_san(x509_tb_s *exts, const char *dns) {
  x509_tb_s c = {0}, v = {0};
  x509_tb_tlv(&c, 0x82, dns, FIO_STRLEN(dns));
  x509_tb_wrap(&v, 0x30, &c);
  x509_tb_ext(exts, x509_tb_oid_san, 3, 0, &v);
}

/** NameConstraints with one subtree: tag 0xA0 (permitted) or 0xA1 (excluded),
 * `gn_tag` GeneralName tag (0x82 dNSName, 0x81 rfc822Name, ...) */
FIO_SFUNC void x509_tb_ext_nc(x509_tb_s *exts,
                              uint8_t subtree_tag,
                              uint8_t gn_tag,
                              const char *base) {
  x509_tb_s gn = {0}, gs = {0}, st = {0}, v = {0};
  x509_tb_tlv(&gn, gn_tag, base, FIO_STRLEN(base));
  x509_tb_wrap(&gs, 0x30, &gn);
  x509_tb_wrap(&st, subtree_tag, &gs);
  x509_tb_wrap(&v, 0x30, &st);
  x509_tb_ext(exts, x509_tb_oid_nc, 3, 1, &v);
}

/** Builds an Ed25519-signed v3 certificate into `out` (returns length). */
FIO_SFUNC size_t x509_tb_cert(x509_tb_s *out,
                              const char *subject,
                              const char *issuer,
                              const fio_x509_keypair_s *subject_kp,
                              const fio_x509_keypair_s *issuer_kp,
                              const x509_tb_s *exts) {
  static const uint8_t ver[] = {0xA0, 0x03, 0x02, 0x01, 0x02};
  static const uint8_t alg[] = {0x30, 0x05, 0x06, 0x03, 0x2B, 0x65, 0x70};
  static const uint8_t validity[] = {
      0x30, 0x1E, 0x17, 0x0D, '2', '0', '0', '1', '0', '1', '0', '0',
      '0',  '0',  '0',  '0',  'Z', 0x17, 0x0D, '4', '9', '0', '1', '0',
      '1',  '0',  '0',  '0',  '0', '0',  '0',  'Z'};
  static uint8_t serial = 0;
  x509_tb_s tbs = {0}, tmp = {0}, cert = {0};
  uint8_t bits[65];
  uint8_t sn = (uint8_t)((++serial & 0x7F) | 1);

  x509_tb_raw(&tbs, ver, sizeof(ver));
  x509_tb_tlv(&tbs, 0x02, &sn, 1);
  x509_tb_raw(&tbs, alg, sizeof(alg));
  x509_tb_name(&tbs, issuer);
  x509_tb_raw(&tbs, validity, sizeof(validity));
  x509_tb_name(&tbs, subject);
  x509_tb_raw(&tmp, alg, sizeof(alg));
  bits[0] = 0;
  FIO_MEMCPY(bits + 1, subject_kp->public_key, 32);
  x509_tb_tlv(&tmp, 0x03, bits, 33);
  x509_tb_wrap(&tbs, 0x30, &tmp);
  if (exts && exts->n) {
    tmp.n = 0;
    x509_tb_wrap(&tmp, 0x30, exts);
    x509_tb_wrap(&tbs, 0xA3, &tmp);
  }
  x509_tb_wrap(&cert, 0x30, &tbs);
  fio_ed25519_sign(bits + 1,
                   cert.b,
                   cert.n,
                   issuer_kp->secret_key,
                   issuer_kp->public_key);
  x509_tb_raw(&cert, alg, sizeof(alg));
  x509_tb_tlv(&cert, 0x03, bits, 65);
  out->n = 0;
  x509_tb_wrap(out, 0x30, &cert);
  return out->n;
}

/** CA extensions: CA:TRUE (+pathLen), keyCertSign */
FIO_SFUNC void x509_tb_ca_exts(x509_tb_s *exts, int path_len) {
  exts->n = 0;
  x509_tb_ext_bc(exts, 1, path_len);
  x509_tb_ext_ku(exts, 0x04);
}

/** Leaf extensions: digitalSignature, serverAuth, SAN dns */
FIO_SFUNC void x509_tb_leaf_exts(x509_tb_s *exts, const char *dns) {
  exts->n = 0;
  x509_tb_ext_ku(exts, 0x80);
  x509_tb_ext_eku(exts, 1);
  x509_tb_ext_san(exts, dns);
}

FIO_SFUNC int x509_tb_verify(const x509_tb_s *leaf,
                             const x509_tb_s *inter,
                             const x509_tb_s *root,
                             const char *host,
                             fio_x509_trust_store_s *trust,
                             fio_x509_purpose_e purpose) {
  fio_ubuf_info_s certs[3];
  size_t n = 0;
  certs[n++] = FIO_UBUF_INFO2((uint8_t *)leaf->b, leaf->n);
  if (inter)
    certs[n++] = FIO_UBUF_INFO2((uint8_t *)inter->b, inter->n);
  if (root)
    certs[n++] = FIO_UBUF_INFO2((uint8_t *)root->b, root->n);
  return fio_x509_verify_chain_purpose(certs,
                                       n,
                                       host,
                                       1750000000,
                                       trust,
                                       purpose);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_chain_policy)(void) {
  fio_x509_keypair_s root_kp, inter_kp, leaf_kp, evil_kp;
  FIO_ASSERT(!fio_x509_keypair_ed25519(&root_kp) &&
                 !fio_x509_keypair_ed25519(&inter_kp) &&
                 !fio_x509_keypair_ed25519(&leaf_kp) &&
                 !fio_x509_keypair_ed25519(&evil_kp),
             "Ed25519 key pair generation failed");
  static x509_tb_s root, inter, leaf, evil_root, tmp, exts;
  const fio_x509_purpose_e SRV = FIO_X509_PURPOSE_SERVER;
  int r;

  x509_tb_ca_exts(&exts, -1);
  x509_tb_cert(&root, "Root", "Root", &root_kp, &root_kp, &exts);
  x509_tb_cert(&evil_root, "Root", "Root", &evil_kp, &evil_kp, &exts);
  x509_tb_cert(&inter, "Inter", "Root", &inter_kp, &root_kp, &exts);
  x509_tb_leaf_exts(&exts, "www.example.com");
  x509_tb_cert(&leaf, "leaf", "Inter", &leaf_kp, &inter_kp, &exts);

  const uint8_t *roots[2] = {root.b, NULL};
  size_t root_lens[2] = {root.n, 0};
  fio_x509_trust_store_s trust = {.roots = roots,
                                  .root_lens = root_lens,
                                  .root_count = 1};

  r = x509_tb_verify(&leaf, &inter, NULL, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_OK, "valid chain failed: %s", fio_x509_error_str(r));
  r = x509_tb_verify(&leaf, &inter, &root, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_OK,
             "valid chain with root failed: %s",
             fio_x509_error_str(r));
  r = fio_x509_verify_chain(
      (fio_ubuf_info_s[]){FIO_UBUF_INFO2(leaf.b, leaf.n),
                          FIO_UBUF_INFO2(inter.b, inter.n)},
      2,
      "www.example.com",
      1750000000,
      &trust);
  FIO_ASSERT(r == FIO_X509_OK, "fio_x509_verify_chain failed");

  /* --- trust predicate (F1): DN equality alone never trusts --- */
  {
    fio_x509_cert_s c;
    FIO_ASSERT(!fio_x509_parse(&c, root.b, root.n) &&
                   !fio_x509_is_trusted(&c, &trust),
               "anchor itself should be trusted");
    FIO_ASSERT(!fio_x509_parse(&c, inter.b, inter.n) &&
                   !fio_x509_is_trusted(&c, &trust),
               "certificate signed by an anchor should be trusted");
    FIO_ASSERT(!fio_x509_parse(&c, evil_root.b, evil_root.n) &&
                   fio_x509_is_trusted(&c, &trust) == -1,
               "same-DN impostor must not be trusted");
    FIO_ASSERT(!fio_x509_parse(&c, leaf.b, leaf.n) &&
                   fio_x509_is_trusted(&c, &trust) == -1,
               "unrelated certificate must not be trusted");
  }
  /* impostor without extensions (passes leaf checks, fails at the anchor) */
  x509_tb_cert(&tmp, "Root", "Root", &evil_kp, &evil_kp, NULL);
  r = x509_tb_verify(&tmp, NULL, NULL, NULL, &trust, SRV);
  FIO_ASSERT(r == FIO_X509_ERR_SIGNATURE,
             "self-signed same-DN impostor must fail: %s",
             fio_x509_error_str(r));

  /* --- two same-DN anchors, both orders (H7) --- */
  roots[0] = evil_root.b;
  root_lens[0] = evil_root.n;
  roots[1] = root.b;
  root_lens[1] = root.n;
  trust.root_count = 2;
  r = x509_tb_verify(&leaf, &inter, NULL, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_OK,
             "second same-DN anchor should be tried: %s",
             fio_x509_error_str(r));
  roots[0] = root.b;
  root_lens[0] = root.n;
  roots[1] = evil_root.b;
  root_lens[1] = evil_root.n;
  r = x509_tb_verify(&leaf, &inter, NULL, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_OK, "first anchor should verify");
  trust.root_count = 1;

  /* --- issuer must be a CA (F2) --- */
  exts.n = 0;
  x509_tb_ext_ku(&exts, 0x04);
  x509_tb_cert(&tmp, "Inter", "Root", &inter_kp, &root_kp, &exts);
  r = x509_tb_verify(&leaf, &tmp, NULL, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_ERR_NOT_CA,
             "non-CA issuer must fail: %s",
             fio_x509_error_str(r));
  /* v3 self-signed non-CA issuer */
  x509_tb_leaf_exts(&exts, "www.example.com");
  x509_tb_cert(&tmp, "Self", "Self", &inter_kp, &inter_kp, &exts);
  {
    x509_tb_s l2;
    x509_tb_cert(&l2, "leaf", "Self", &leaf_kp, &inter_kp, &exts);
    r = x509_tb_verify(&l2, &tmp, NULL, "www.example.com", NULL, SRV);
    FIO_ASSERT(r == FIO_X509_ERR_NOT_CA,
               "self-signed non-CA issuer must fail: %s",
               fio_x509_error_str(r));
  }
  /* CA without keyCertSign */
  exts.n = 0;
  x509_tb_ext_bc(&exts, 1, -1);
  x509_tb_ext_ku(&exts, 0x80);
  x509_tb_cert(&tmp, "Inter", "Root", &inter_kp, &root_kp, &exts);
  r = x509_tb_verify(&leaf, &tmp, NULL, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_ERR_KEY_USAGE,
             "CA without keyCertSign must fail: %s",
             fio_x509_error_str(r));

  /* --- pathLenConstraint (F2) --- */
  x509_tb_ca_exts(&exts, 0);
  x509_tb_cert(&tmp, "Root", "Root", &root_kp, &root_kp, &exts);
  roots[0] = tmp.b;
  root_lens[0] = tmp.n;
  r = x509_tb_verify(&leaf, &inter, &tmp, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_ERR_PATH_LENGTH,
             "pathLen 0 root above an intermediate must fail: %s",
             fio_x509_error_str(r));
  roots[0] = root.b;
  root_lens[0] = root.n;
  x509_tb_ca_exts(&exts, 0);
  x509_tb_cert(&tmp, "Inter", "Root", &inter_kp, &root_kp, &exts);
  r = x509_tb_verify(&leaf, &tmp, &root, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_OK,
             "pathLen 0 intermediate issuing a leaf is valid: %s",
             fio_x509_error_str(r));

  /* --- leaf Key Usage / Extended Key Usage (F3) --- */
  exts.n = 0;
  x509_tb_ext_ku(&exts, 0x20); /* keyEncipherment only */
  x509_tb_ext_san(&exts, "www.example.com");
  x509_tb_cert(&tmp, "leaf", "Inter", &leaf_kp, &inter_kp, &exts);
  r = x509_tb_verify(&tmp, &inter, NULL, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_ERR_KEY_USAGE,
             "leaf without digitalSignature must fail: %s",
             fio_x509_error_str(r));
  exts.n = 0;
  x509_tb_ext_ku(&exts, 0x80);
  x509_tb_ext_eku(&exts, 2); /* clientAuth only */
  x509_tb_ext_san(&exts, "www.example.com");
  x509_tb_cert(&tmp, "leaf", "Inter", &leaf_kp, &inter_kp, &exts);
  r = x509_tb_verify(&tmp, &inter, NULL, "www.example.com", &trust, SRV);
  FIO_ASSERT(r == FIO_X509_ERR_KEY_USAGE,
             "clientAuth-only leaf must fail server purpose: %s",
             fio_x509_error_str(r));
  r = x509_tb_verify(&tmp,
                     &inter,
                     NULL,
                     NULL,
                     &trust,
                     FIO_X509_PURPOSE_CLIENT);
  FIO_ASSERT(r == FIO_X509_OK,
             "clientAuth leaf should pass client purpose: %s",
             fio_x509_error_str(r));
  r = x509_tb_verify(&leaf, &inter, NULL, NULL, &trust, FIO_X509_PURPOSE_CLIENT);
  FIO_ASSERT(r == FIO_X509_ERR_KEY_USAGE,
             "serverAuth-only leaf must fail client purpose");
  exts.n = 0;
  x509_tb_ext_ku(&exts, 0x80);
  x509_tb_ext_eku(&exts, 3); /* codeSigning */
  x509_tb_ext_san(&exts, "www.example.com");
  x509_tb_cert(&tmp, "leaf", "Inter", &leaf_kp, &inter_kp, &exts);
  r = x509_tb_verify(&tmp, &inter, NULL, NULL, &trust, FIO_X509_PURPOSE_TLS);
  FIO_ASSERT(r == FIO_X509_ERR_KEY_USAGE, "codeSigning leaf must fail TLS");

  /* --- unknown critical extensions (F4) --- */
  {
    x509_tb_s v = {0};
    x509_tb_tlv(&v, 0x05, NULL, 0);
    x509_tb_leaf_exts(&exts, "www.example.com");
    x509_tb_ext(&exts, x509_tb_oid_unknown, 3, 1, &v);
    x509_tb_cert(&tmp, "leaf", "Inter", &leaf_kp, &inter_kp, &exts);
    fio_x509_cert_s c;
    FIO_ASSERT(!fio_x509_parse(&c, tmp.b, tmp.n),
               "unknown critical extension should still parse");
    r = x509_tb_verify(&tmp, &inter, NULL, "www.example.com", &trust, SRV);
    FIO_ASSERT(r == FIO_X509_ERR_CRITICAL_EXTENSION,
               "unknown critical extension must fail: %s",
               fio_x509_error_str(r));
    x509_tb_leaf_exts(&exts, "www.example.com");
    x509_tb_ext(&exts, x509_tb_oid_unknown, 3, 0, &v);
    x509_tb_cert(&tmp, "leaf", "Inter", &leaf_kp, &inter_kp, &exts);
    r = x509_tb_verify(&tmp, &inter, NULL, "www.example.com", &trust, SRV);
    FIO_ASSERT(r == FIO_X509_OK, "unknown non-critical extension is fine");
    /* duplicate recognized extension */
    x509_tb_leaf_exts(&exts, "www.example.com");
    x509_tb_ext_san(&exts, "www.example.com");
    x509_tb_cert(&tmp, "leaf", "Inter", &leaf_kp, &inter_kp, &exts);
    FIO_ASSERT(fio_x509_parse(&c, tmp.b, tmp.n) == -1,
               "duplicate extension must be rejected");
  }

  /* --- Name Constraints (F4) --- */
  {
    x509_tb_s nc_inter, l2;
    x509_tb_ca_exts(&exts, -1);
    x509_tb_ext_nc(&exts, 0xA0, 0x82, "example.com");
    x509_tb_cert(&nc_inter, "Inter", "Root", &inter_kp, &root_kp, &exts);
    r = x509_tb_verify(&leaf, &nc_inter, NULL, "www.example.com", &trust, SRV);
    FIO_ASSERT(r == FIO_X509_OK,
               "permitted DNS name should pass: %s",
               fio_x509_error_str(r));
    x509_tb_leaf_exts(&exts, "www.evil.com");
    x509_tb_cert(&l2, "leaf", "Inter", &leaf_kp, &inter_kp, &exts);
    r = x509_tb_verify(&l2, &nc_inter, NULL, "www.evil.com", &trust, SRV);
    FIO_ASSERT(r == FIO_X509_ERR_NAME_CONSTRAINTS,
               "name outside permitted subtree must fail: %s",
               fio_x509_error_str(r));
    r = x509_tb_verify(&l2, &nc_inter, NULL, NULL, &trust, SRV);
    FIO_ASSERT(r == FIO_X509_ERR_NAME_CONSTRAINTS,
               "SAN outside permitted subtree must fail without hostname");

    x509_tb_ca_exts(&exts, -1);
    x509_tb_ext_nc(&exts, 0xA1, 0x82, "bad.example.com");
    x509_tb_cert(&nc_inter, "Inter", "Root", &inter_kp, &root_kp, &exts);
    x509_tb_leaf_exts(&exts, "*.example.com");
    x509_tb_cert(&l2, "leaf", "Inter", &leaf_kp, &inter_kp, &exts);
    r = x509_tb_verify(&l2, &nc_inter, NULL, "ok.example.com", &trust, SRV);
    FIO_ASSERT(r == FIO_X509_OK, "non-excluded name should pass");
    r = x509_tb_verify(&l2, &nc_inter, NULL, "bad.example.com", &trust, SRV);
    FIO_ASSERT(r == FIO_X509_ERR_NAME_CONSTRAINTS,
               "excluded name via wildcard must fail: %s",
               fio_x509_error_str(r));

    x509_tb_ca_exts(&exts, -1);
    x509_tb_ext_nc(&exts, 0xA0, 0x81, "example.com"); /* rfc822Name */
    x509_tb_cert(&nc_inter, "Inter", "Root", &inter_kp, &root_kp, &exts);
    r = x509_tb_verify(&leaf, &nc_inter, NULL, "www.example.com", &trust, SRV);
    FIO_ASSERT(r == FIO_X509_ERR_NAME_CONSTRAINTS,
               "unsupported constraint form must fail closed");
  }

  fio_x509_keypair_clear(&root_kp);
  fio_x509_keypair_clear(&inter_kp);
  fio_x509_keypair_clear(&leaf_kp);
  fio_x509_keypair_clear(&evil_kp);
}

/** Lenient-but-safe extension decoding (parse / chain level). */
FIO_SFUNC void FIO_NAME_TEST(stl, x509_lenient_ext)(void) {
  fio_x509_keypair_s ca_kp, kp;
  FIO_ASSERT(!fio_x509_keypair_ed25519(&ca_kp) && !fio_x509_keypair_ed25519(&kp),
             "Ed25519 key pair generation failed");
  static x509_tb_s ca, leaf, exts, v, c;
  fio_x509_cert_s cert;
  const uint8_t *roots[1];
  size_t root_lens[1];
  fio_x509_trust_store_s trust = {.roots = roots,
                                  .root_lens = root_lens,
                                  .root_count = 1};
  int r;

  /* KU padding bits are masked: 7 unused bits, padding claims keyCertSign */
  {
    static const uint8_t ku[] = {0x07, 0x84};
    exts.n = v.n = 0;
    x509_tb_tlv(&v, 0x03, ku, sizeof(ku));
    x509_tb_ext(&exts, x509_tb_oid_ku, 3, 1, &v);
    x509_tb_cert(&leaf, "leaf", "leaf", &kp, &kp, &exts);
    FIO_ASSERT(!fio_x509_parse(&cert, leaf.b, leaf.n) && cert.has_key_usage &&
                   cert.key_usage == FIO_X509_KU_DIGITAL_SIGNATURE,
               "KU padding bits should be masked off");
  }

  /* empty SAN / EKU: tolerated, but grant nothing (no CN fallback) */
  {
    exts.n = v.n = c.n = 0;
    x509_tb_wrap(&v, 0x30, &c);
    x509_tb_ext(&exts, x509_tb_oid_san, 3, 0, &v);
    x509_tb_cert(&leaf, "www.example.com", "www.example.com", &kp, &kp, &exts);
    FIO_ASSERT(!fio_x509_parse(&cert, leaf.b, leaf.n),
               "empty SAN should parse");
    FIO_ASSERT(fio_x509_match_hostname(&cert, "www.example.com", 15) == -1,
               "empty SAN must not fall back to the CN");
    exts.n = 0;
    x509_tb_ext(&exts, x509_tb_oid_eku, 3, 0, &v);
    x509_tb_cert(&leaf, "leaf", "leaf", &kp, &kp, &exts);
    FIO_ASSERT(!fio_x509_parse(&cert, leaf.b, leaf.n),
               "empty EKU should parse");
    roots[0] = leaf.b;
    root_lens[0] = leaf.n;
    r = x509_tb_verify(&leaf, NULL, NULL, NULL, &trust, FIO_X509_PURPOSE_TLS);
    FIO_ASSERT(r == FIO_X509_ERR_KEY_USAGE,
               "empty EKU must grant no purpose: %s",
               fio_x509_error_str(r));
  }

  /* GeneralSubtree: explicit default minimum 0 is fine, maximum fails */
  for (int i = 0; i < 3; ++i) {
    static const uint8_t extra[3][3] = {{0x80, 0x01, 0x00},
                                        {0x80, 0x01, 0x01},
                                        {0x81, 0x01, 0x05}};
    x509_tb_s gn = {0}, gs = {0}, st = {0};
    x509_tb_tlv(&gn, 0x82, "example.com", 11);
    x509_tb_raw(&gn, extra[i], 3);
    x509_tb_wrap(&gs, 0x30, &gn);
    x509_tb_wrap(&st, 0xA0, &gs);
    v.n = 0;
    x509_tb_wrap(&v, 0x30, &st);
    x509_tb_ca_exts(&exts, -1);
    x509_tb_ext(&exts, x509_tb_oid_nc, 3, 1, &v);
    x509_tb_cert(&ca, "CA", "CA", &ca_kp, &ca_kp, &exts);
    x509_tb_leaf_exts(&exts, "www.example.com");
    x509_tb_cert(&leaf, "leaf", "CA", &kp, &ca_kp, &exts);
    roots[0] = ca.b;
    root_lens[0] = ca.n;
    r = x509_tb_verify(&leaf, NULL, NULL, "www.example.com", &trust,
                       FIO_X509_PURPOSE_SERVER);
    FIO_ASSERT(i ? r == FIO_X509_ERR_NAME_CONSTRAINTS : r == FIO_X509_OK,
               "GeneralSubtree case %d: %s",
               i,
               fio_x509_error_str(r));
  }
  fio_x509_keypair_clear(&ca_kp);
  fio_x509_keypair_clear(&kp);
}

/* *****************************************************************************
IP identities, RSA-PSS parameters, TLS splitting, generation boundaries
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x509_hostname_ip)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(!fio_x509_keypair_ed25519(&kp), "key generation failed");
  const char *san[] = {"localhost", "127.0.0.1", "::1"};
  size_t cert_len;
  uint8_t *cert =
      FIO_NAME_TEST(stl, x509_make_cert)(&cert_len, &kp, "localhost", san, 3, 0);
  fio_x509_cert_s c;
  FIO_ASSERT(!fio_x509_parse(&c, cert, cert_len), "parse failed");
  FIO_ASSERT(c.san_ip.len == 4, "IP literal should be encoded as iPAddress");
  FIO_ASSERT(!fio_x509_match_hostname(&c, "127.0.0.1", 9), "IPv4 SAN match");
  FIO_ASSERT(!fio_x509_match_hostname(&c, "::1", 3), "IPv6 SAN match");
  FIO_ASSERT(!fio_x509_match_hostname(&c, "0:0:0:0:0:0:0:1", 15),
             "IPv6 long form SAN match");
  FIO_ASSERT(fio_x509_match_hostname(&c, "127.0.0.2", 9) == -1,
             "wrong IPv4 must fail");
  FIO_ASSERT(fio_x509_match_hostname(&c, "127.000.0.1", 11) == -1,
             "non-canonical IPv4 must fail");
  FIO_ASSERT(!fio_x509_match_hostname(&c, "localhost", 9), "DNS SAN match");
  FIO_MEM_FREE(cert, cert_len + 64);

  /* CN that looks like an IP never authorizes an IP literal */
  cert =
      FIO_NAME_TEST(stl, x509_make_cert)(&cert_len, &kp, "10.0.0.1", NULL, 0, 0);
  FIO_ASSERT(!fio_x509_parse(&c, cert, cert_len), "parse failed");
  FIO_ASSERT(fio_x509_match_hostname(&c, "10.0.0.1", 8) == -1,
             "CN must not match an IP literal");
  FIO_MEM_FREE(cert, cert_len + 64);

  {
    uint8_t ip[16];
    FIO_ASSERT(fio___x509_parse_ip(ip, "1.2.3", 5) == 0, "short IPv4");
    FIO_ASSERT(fio___x509_parse_ip(ip, "1.2.3.256", 9) == 0, "IPv4 range");
    FIO_ASSERT(fio___x509_parse_ip(ip, "1::2::3", 7) == 0, "double ::");
    FIO_ASSERT(fio___x509_parse_ip(ip, "1:2:3:4:5:6:7:8:9", 17) == 0,
               "too many groups");
    FIO_ASSERT(fio___x509_parse_ip(ip, "::ffff:1.2.3.4", 14) == 16 &&
                   ip[10] == 0xFF && ip[15] == 4,
               "IPv4-mapped IPv6");
    FIO_ASSERT(fio___x509_parse_ip(ip, "12345::", 7) == 0, "long group");
    FIO_ASSERT(fio___x509_parse_ip(ip, "1:", 2) == 0, "trailing colon");
  }
  fio_x509_keypair_clear(&kp);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_pss_params)(void) {
  /* AlgorithmIdentifier { id-RSASSA-PSS, params } builder */
  static const uint8_t pss_oid[] =
      {0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A};
  static const uint8_t mgf1_oid[] =
      {0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x08};
  struct {
    uint8_t hash, mgf_hash, salt;
    int has_params;
    fio_x509_signature_algo_e expect;
  } cases[] = {
      {1, 1, 32, 1, FIO_X509_SIGNATURE_RSA_PSS_SHA256},
      {2, 2, 48, 1, FIO_X509_SIGNATURE_RSA_PSS_SHA384},
      {3, 3, 64, 1, FIO_X509_SIGNATURE_RSA_PSS_SHA512},
      {1, 1, 20, 1, FIO_X509_SIGNATURE_UNKNOWN}, /* salt != hash length */
      {1, 2, 32, 1, FIO_X509_SIGNATURE_UNKNOWN}, /* MGF1 hash differs */
      {2, 2, 32, 1, FIO_X509_SIGNATURE_UNKNOWN},
      {0, 0, 0, 0, FIO_X509_SIGNATURE_UNKNOWN}, /* no params (SHA-1) */
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    x509_tb_s hash = {0}, tmp = {0}, params = {0}, alg = {0}, seq = {0};
    uint8_t h_oid[] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0};
    x509_tb_raw(&alg, pss_oid, sizeof(pss_oid));
    if (cases[i].has_params) {
      h_oid[8] = cases[i].hash;
      x509_tb_tlv(&tmp, 0x06, h_oid, sizeof(h_oid));
      x509_tb_wrap(&hash, 0x30, &tmp);
      x509_tb_wrap(&params, 0xA0, &hash);
      tmp.n = hash.n = 0;
      h_oid[8] = cases[i].mgf_hash;
      x509_tb_tlv(&tmp, 0x06, h_oid, sizeof(h_oid));
      x509_tb_raw(&hash, mgf1_oid, sizeof(mgf1_oid));
      x509_tb_wrap(&hash, 0x30, &tmp);
      tmp.n = 0;
      x509_tb_wrap(&tmp, 0x30, &hash);
      x509_tb_wrap(&params, 0xA1, &tmp);
      tmp.n = 0;
      x509_tb_tlv(&tmp, 0x02, &cases[i].salt, 1);
      x509_tb_wrap(&params, 0xA2, &tmp);
      x509_tb_wrap(&alg, 0x30, &params);
    }
    x509_tb_wrap(&seq, 0x30, &alg);
    fio_der_element_s e;
    FIO_ASSERT(fio_der_parse(&e, seq.b, seq.n), "PSS test encoding");
    FIO_ASSERT(fio___x509_parse_signature_algo(&e) == cases[i].expect,
               "RSA-PSS parameter case %zu",
               i);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_tls_cert_message_strict)(void) {
  /* two entries of 1-byte "certificates" */
  uint8_t msg[] = {0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x01, 0xAA,
                   0x00, 0x00, 0x00, 0x00, 0x01, 0xBB, 0x00, 0x00, 0xFF};
  fio_tls_cert_entry_s entries[2];
  FIO_ASSERT(fio_tls_parse_certificate_message(entries, 2, msg, 16) == 2,
             "two entries expected");
  FIO_ASSERT(fio_tls_parse_certificate_message(entries, 1, msg, 16) ==
                 FIO_TLS_CERT_PARSE_ERROR,
             "more entries than capacity must be an error");
  FIO_ASSERT(fio_tls_parse_certificate_message(entries, 2, msg, 17) ==
                 FIO_TLS_CERT_PARSE_ERROR,
             "trailing data after certificate_list must be rejected");
  for (size_t i = 1; i < 16; ++i)
    FIO_ASSERT(fio_tls_parse_certificate_message(entries, 2, msg, i) ==
                   FIO_TLS_CERT_PARSE_ERROR,
               "truncated message (%zu) must fail",
               i);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_generation_dates)(void) {
  fio_x509_keypair_s kp;
  FIO_ASSERT(!fio_x509_keypair_ed25519(&kp), "key generation failed");
  fio_x509_cert_options_s opts = {
      .cn = FIO_BUF_INFO2((char *)"dates", 5),
      .not_before = 1750000000,
      .not_after = 1700000000,
  };
  FIO_ASSERT(fio_x509_self_signed_cert(NULL, 0, &kp, &opts) == 0,
             "reversed validity interval must be rejected");
  opts.not_after = 2556143999; /* 2050-12-31T23:59:59Z (GeneralizedTime) */
  size_t need = fio_x509_self_signed_cert(NULL, 0, &kp, &opts);
  FIO_ASSERT(need, "size query failed");
  uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, need, 0);
  FIO_ASSERT(buf, "allocation failed");
  size_t len = fio_x509_self_signed_cert(buf, need, &kp, &opts);
  fio_x509_cert_s c;
  FIO_ASSERT(len && !fio_x509_parse(&c, buf, len), "generation failed");
  FIO_ASSERT(c.not_before == 1750000000 && c.not_after == 2556143999,
             "validity round trip failed (%lld)",
             (long long)c.not_after);
  FIO_MEM_FREE(buf, need);
  fio_x509_keypair_clear(&kp);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x509_new_oids)(void) {
  struct {
    fio_u128 oid;
    const char *dot;
  } t[] = {
      {FIO_X509_OID_MGF1, "1.2.840.113549.1.1.8"},
      {FIO_X509_OID_SHA256, "2.16.840.1.101.3.4.2.1"},
      {FIO_X509_OID_SHA384, "2.16.840.1.101.3.4.2.2"},
      {FIO_X509_OID_SHA512, "2.16.840.1.101.3.4.2.3"},
      {FIO_X509_OID_NAME_CONSTRAINTS, "2.5.29.30"},
      {FIO_X509_OID_EKU_ANY, "2.5.29.37.0"},
  };
  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    uint8_t buf[32];
    char str[64];
    fio_der_element_s e;
    size_t len = fio___der_encode_oid(buf, t[i].oid);
    FIO_ASSERT(len && fio_der_parse(&e, buf, len) == buf + len,
               "OID encode failed");
    FIO_ASSERT(fio_der_parse_oid(&e, str, sizeof(str)) > 0 &&
                   !strcmp(str, t[i].dot),
               "OID %s round trip failed",
               t[i].dot);
  }
}

/* *****************************************************************************
Compressed EC public keys (SEC 1: 02/03 || x) - OpenSSL-generated chains
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x509_compressed_ec)(void) {
  static const struct {
    const char *ca;
    const char *leaf;
    fio_x509_key_algo_e algo;
    size_t point_len;
  } curves[] = {
      {fio___test_ecc_p256_ca,
       fio___test_ecc_p256_leaf,
       FIO_X509_KEY_ECDSA_P256,
       33},
      {fio___test_ecc_p384_ca,
       fio___test_ecc_p384_leaf,
       FIO_X509_KEY_ECDSA_P384,
       49},
  };
  const int64_t now = 1893456000; /* 2030-01-01, inside both validities */
  const fio_x509_purpose_e SRV = FIO_X509_PURPOSE_SERVER;
  for (size_t i = 0; i < sizeof(curves) / sizeof(curves[0]); ++i) {
    uint8_t ca_der[1024], leaf_der[1024], bad[1024];
    size_t ca_len = fio_pem_get_certificate_der(ca_der,
                                                sizeof(ca_der),
                                                curves[i].ca,
                                                FIO_STRLEN(curves[i].ca));
    size_t leaf_len = fio_pem_get_certificate_der(leaf_der,
                                                  sizeof(leaf_der),
                                                  curves[i].leaf,
                                                  FIO_STRLEN(curves[i].leaf));
    FIO_ASSERT(ca_len && leaf_len, "compressed EC fixture %zu: PEM failed", i);
    fio_x509_cert_s ca, leaf, tmp;
    FIO_ASSERT(!fio_x509_parse(&ca, ca_der, ca_len) &&
                   !fio_x509_parse(&leaf, leaf_der, leaf_len),
               "compressed EC fixture %zu: parse failed",
               i);
    FIO_ASSERT(ca.key_algo == curves[i].algo &&
                   leaf.key_algo == curves[i].algo &&
                   ca.pubkey.ecdsa.point.len == curves[i].point_len &&
                   leaf.pubkey.ecdsa.point.len == curves[i].point_len &&
                   ca.pubkey.ecdsa.point.buf[0] == 0x02 &&
                   leaf.pubkey.ecdsa.point.buf[0] == 0x03,
               "compressed EC fixture %zu: point should stay compressed",
               i);
    FIO_ASSERT(!fio_x509_verify_signature(&ca, &ca) &&
                   !fio_x509_verify_signature(&leaf, &ca),
               "compressed EC fixture %zu: signature verification failed",
               i);
    FIO_ASSERT(fio_x509_verify_signature(&ca, &leaf),
               "compressed EC fixture %zu: wrong issuer key should fail",
               i);

    /* chain: leaf + CA and leaf alone, trust anchor = CA */
    const uint8_t *roots[1] = {ca_der};
    size_t root_lens[1] = {ca_len};
    fio_x509_trust_store_s trust = {.roots = roots,
                                    .root_lens = root_lens,
                                    .root_count = 1};
    fio_ubuf_info_s certs[2] = {FIO_UBUF_INFO2(leaf_der, leaf_len),
                                FIO_UBUF_INFO2(ca_der, ca_len)};
    FIO_ASSERT(fio_x509_verify_chain_purpose(certs,
                                             2,
                                             "localhost",
                                             now,
                                             &trust,
                                             SRV) == FIO_X509_OK,
               "compressed EC fixture %zu: chain (leaf + CA) should verify",
               i);
    FIO_ASSERT(fio_x509_verify_chain_purpose(certs,
                                             1,
                                             "localhost",
                                             now,
                                             &trust,
                                             SRV) == FIO_X509_OK,
               "compressed EC fixture %zu: chain (leaf) should verify",
               i);
    FIO_ASSERT(fio_x509_verify_chain_purpose(certs,
                                             2,
                                             "example.com",
                                             now,
                                             &trust,
                                             SRV) != FIO_X509_OK,
               "compressed EC fixture %zu: wrong hostname should fail",
               i);

    /* tampered leaf signature (last byte of the signature value) */
    FIO_MEMCPY(bad, leaf_der, leaf_len);
    bad[leaf_len - 1] ^= 0x01;
    certs[0] = FIO_UBUF_INFO2(bad, leaf_len);
    FIO_ASSERT(!fio_x509_parse(&tmp, bad, leaf_len) &&
                   fio_x509_verify_signature(&tmp, &ca) &&
                   fio_x509_verify_chain_purpose(certs,
                                                 2,
                                                 "localhost",
                                                 now,
                                                 &trust,
                                                 SRV) != FIO_X509_OK,
               "compressed EC fixture %zu: tampered leaf should fail",
               i);
    certs[0] = FIO_UBUF_INFO2(leaf_der, leaf_len);

    /* mutated CA point: the negated key (03) and an invalid prefix (05) */
    static const uint8_t prefixes[] = {0x03, 0x05, 0x04};
    const size_t at = (size_t)(ca.pubkey.ecdsa.point.buf - ca_der);
    for (size_t j = 0; j < sizeof(prefixes); ++j) {
      FIO_MEMCPY(bad, ca_der, ca_len);
      bad[at] = prefixes[j];
      FIO_ASSERT(!fio_x509_parse(&tmp, bad, ca_len) &&
                     fio_x509_verify_signature(&leaf, &tmp),
                 "compressed EC fixture %zu: CA point prefix %02X should fail",
                 i,
                 (unsigned)prefixes[j]);
      roots[0] = bad;
      certs[1] = FIO_UBUF_INFO2(bad, ca_len);
      FIO_ASSERT(fio_x509_verify_chain_purpose(certs,
                                               2,
                                               "localhost",
                                               now,
                                               &trust,
                                               SRV) != FIO_X509_OK,
                 "compressed EC fixture %zu: chain with CA prefix %02X "
                 "should fail",
                 i,
                 (unsigned)prefixes[j]);
      roots[0] = ca_der;
      certs[1] = FIO_UBUF_INFO2(ca_der, ca_len);
    }
  }
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
  FIO_NAME_TEST(stl, x509_strict_der)();
  FIO_NAME_TEST(stl, x509_chain_policy)();
  FIO_NAME_TEST(stl, x509_lenient_ext)();
  FIO_NAME_TEST(stl, x509_hostname_ip)();
  FIO_NAME_TEST(stl, x509_pss_params)();
  FIO_NAME_TEST(stl, x509_tls_cert_message_strict)();
  FIO_NAME_TEST(stl, x509_generation_dates)();
  FIO_NAME_TEST(stl, x509_new_oids)();
  FIO_NAME_TEST(stl, x509_compressed_ec)();
  return 0;
}
