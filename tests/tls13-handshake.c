/* *****************************************************************************
TLS 1.3 Handshake Message Tests

Tests for handshake message building and parsing functions.
***************************************************************************** */
#define FIO_LOG
#define FIO_CRYPT
#define FIO_TLS13
#include "fio-stl.h"

/* *****************************************************************************
Test: Handshake Header
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_handshake_header(void) {
  FIO_LOG_INFO("Testing TLS 1.3 handshake header...");

  /* Test write and parse round-trip */
  uint8_t header[4];
  fio_tls13_write_handshake_header(header, FIO_TLS13_HS_CLIENT_HELLO, 0x123456);

  FIO_ASSERT(header[0] == FIO_TLS13_HS_CLIENT_HELLO,
             "Handshake type should be ClientHello");
  FIO_ASSERT(header[1] == 0x12, "Length byte 0 should be 0x12");
  FIO_ASSERT(header[2] == 0x34, "Length byte 1 should be 0x34");
  FIO_ASSERT(header[3] == 0x56, "Length byte 2 should be 0x56");

  /* Parse it back */
  fio_tls13_handshake_type_e msg_type;
  size_t body_len;

  /* Need to provide enough data for the body too */
  uint8_t full_msg[4 + 0x123456];
  FIO_MEMCPY(full_msg, header, 4);

  const uint8_t *body = fio_tls13_parse_handshake_header(full_msg,
                                                         sizeof(full_msg),
                                                         &msg_type,
                                                         &body_len);

  FIO_ASSERT(body != NULL, "Parse should succeed");
  FIO_ASSERT(msg_type == FIO_TLS13_HS_CLIENT_HELLO,
             "Parsed type should be ClientHello");
  FIO_ASSERT(body_len == 0x123456, "Parsed length should be 0x123456");
  FIO_ASSERT(body == full_msg + 4, "Body should point after header");

  /* Test incomplete data */
  body = fio_tls13_parse_handshake_header(header, 3, &msg_type, &body_len);
  FIO_ASSERT(body == NULL, "Parse should fail with incomplete header");

  /* Test incomplete body */
  body = fio_tls13_parse_handshake_header(header, 4, &msg_type, &body_len);
  FIO_ASSERT(body == NULL, "Parse should fail with incomplete body");

  FIO_LOG_INFO("  PASS: Handshake header tests");
}

/* *****************************************************************************
Test: ClientHello Building
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_client_hello(void) {
  FIO_LOG_INFO("Testing TLS 1.3 ClientHello building...");

  uint8_t buffer[512];
  uint8_t random[32];
  uint8_t x25519_pubkey[32];

  /* Fill with test data */
  for (int i = 0; i < 32; ++i) {
    random[i] = (uint8_t)i;
    x25519_pubkey[i] = (uint8_t)(0x80 + i);
  }

  /* Build ClientHello with SNI */
  int len = fio_tls13_build_client_hello(buffer,
                                         sizeof(buffer),
                                         random,
                                         "example.com",
                                         x25519_pubkey,
                                         NULL,
                                         0);

  FIO_ASSERT(len > 0, "ClientHello build should succeed");
  FIO_ASSERT(len < 512, "ClientHello should fit in buffer");

  /* Verify handshake header */
  FIO_ASSERT(buffer[0] == FIO_TLS13_HS_CLIENT_HELLO,
             "First byte should be ClientHello type (1)");

  /* Parse the header */
  fio_tls13_handshake_type_e msg_type;
  size_t body_len;
  const uint8_t *body = fio_tls13_parse_handshake_header(buffer,
                                                         (size_t)len,
                                                         &msg_type,
                                                         &body_len);

  FIO_ASSERT(body != NULL, "Should parse ClientHello header");
  FIO_ASSERT(msg_type == FIO_TLS13_HS_CLIENT_HELLO,
             "Type should be ClientHello");
  FIO_ASSERT(body_len == (size_t)(len - 4), "Body length should match");

  /* Verify legacy version (TLS 1.2 = 0x0303) */
  FIO_ASSERT(body[0] == 0x03 && body[1] == 0x03,
             "Legacy version should be TLS 1.2");

  /* Verify random */
  FIO_ASSERT(FIO_MEMCMP(body + 2, random, 32) == 0,
             "Random should match input");

  /* Verify session ID length is 0 */
  FIO_ASSERT(body[34] == 0, "Session ID length should be 0");

  /* Build without SNI */
  len = fio_tls13_build_client_hello(buffer,
                                     sizeof(buffer),
                                     random,
                                     NULL,
                                     x25519_pubkey,
                                     NULL,
                                     0);
  FIO_ASSERT(len > 0, "ClientHello without SNI should succeed");

  /* Build with custom cipher suites */
  uint16_t suites[] = {FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256};
  len = fio_tls13_build_client_hello(buffer,
                                     sizeof(buffer),
                                     random,
                                     "test.example.com",
                                     x25519_pubkey,
                                     suites,
                                     1);
  FIO_ASSERT(len > 0, "ClientHello with custom suites should succeed");

  FIO_LOG_INFO("  PASS: ClientHello building tests");
}

/* *****************************************************************************
Test: ServerHello Parsing
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_server_hello(void) {
  FIO_LOG_INFO("Testing TLS 1.3 ServerHello parsing...");

  /* Construct a minimal ServerHello */
  uint8_t sh_data[128];
  uint8_t *p = sh_data;

  /* Legacy version: TLS 1.2 (0x0303) */
  *p++ = 0x03;
  *p++ = 0x03;

  /* Random (32 bytes) - not HRR */
  for (int i = 0; i < 32; ++i)
    *p++ = (uint8_t)i;

  /* Session ID length (0) */
  *p++ = 0;

  /* Cipher suite: AES-128-GCM-SHA256 */
  *p++ = 0x13;
  *p++ = 0x01;

  /* Compression method: null */
  *p++ = 0;

  /* Extensions length */
  uint8_t *ext_len_ptr = p;
  p += 2;
  uint8_t *ext_start = p;

  /* supported_versions extension */
  *p++ = 0x00; /* type high */
  *p++ = 0x2B; /* type low (43) */
  *p++ = 0x00; /* length high */
  *p++ = 0x02; /* length low */
  *p++ = 0x03; /* TLS 1.3 high */
  *p++ = 0x04; /* TLS 1.3 low */

  /* key_share extension */
  *p++ = 0x00; /* type high */
  *p++ = 0x33; /* type low (51) */
  *p++ = 0x00; /* length high */
  *p++ = 0x24; /* length low (36 = 2 + 2 + 32) */
  *p++ = 0x00; /* group high */
  *p++ = 0x1D; /* group low (29 = x25519) */
  *p++ = 0x00; /* key len high */
  *p++ = 0x20; /* key len low (32) */
  for (int i = 0; i < 32; ++i)
    *p++ = (uint8_t)(0xAA + i);

  /* Write extensions length */
  size_t ext_len = (size_t)(p - ext_start);
  ext_len_ptr[0] = (uint8_t)(ext_len >> 8);
  ext_len_ptr[1] = (uint8_t)(ext_len & 0xFF);

  size_t sh_len = (size_t)(p - sh_data);

  /* Parse it */
  fio_tls13_server_hello_s sh;
  int ret = fio_tls13_parse_server_hello(&sh, sh_data, sh_len);

  FIO_ASSERT(ret == 0, "ServerHello parse should succeed");
  FIO_ASSERT(sh.cipher_suite == FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256,
             "Cipher suite should be AES-128-GCM-SHA256");
  FIO_ASSERT(sh.is_hello_retry_request == 0, "Should not be HRR");
  FIO_ASSERT(sh.key_share_group == FIO_TLS13_GROUP_X25519,
             "Key share group should be x25519");
  FIO_ASSERT(sh.key_share_len == 32, "Key share length should be 32");
  FIO_ASSERT(sh.key_share[0] == 0xAA, "Key share data should match");

  /* Test HelloRetryRequest detection */
  uint8_t hrr_random[32] = {0xCF, 0x21, 0xAD, 0x74, 0xE5, 0x9A, 0x61, 0x11,
                            0xBE, 0x1D, 0x8C, 0x02, 0x1E, 0x65, 0xB8, 0x91,
                            0xC2, 0xA2, 0x11, 0x16, 0x7A, 0xBB, 0x8C, 0x5E,
                            0x07, 0x9E, 0x09, 0xE2, 0xC8, 0xA8, 0x33, 0x9C};
  FIO_MEMCPY(sh_data + 2, hrr_random, 32);

  ret = fio_tls13_parse_server_hello(&sh, sh_data, sh_len);
  FIO_ASSERT(ret == 0, "HRR parse should succeed");
  FIO_ASSERT(sh.is_hello_retry_request == 1, "Should detect HRR");

  FIO_LOG_INFO("  PASS: ServerHello parsing tests");
}

/* *****************************************************************************
Test: EncryptedExtensions Parsing
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_encrypted_extensions(void) {
  FIO_LOG_INFO("Testing TLS 1.3 EncryptedExtensions parsing...");

  /* Empty extensions */
  uint8_t ee_empty[] = {0x00, 0x00}; /* extensions length = 0 */
  fio_tls13_encrypted_extensions_s ee;

  int ret =
      fio_tls13_parse_encrypted_extensions(&ee, ee_empty, sizeof(ee_empty));
  FIO_ASSERT(ret == 0, "Empty EE parse should succeed");
  FIO_ASSERT(ee.has_server_name == 0, "Should not have SNI");

  /* With SNI acknowledgment */
  uint8_t ee_sni[] = {
      0x00,
      0x04, /* extensions length = 4 */
      0x00,
      0x00, /* server_name extension type */
      0x00,
      0x00 /* extension data length = 0 (acknowledgment) */
  };

  ret = fio_tls13_parse_encrypted_extensions(&ee, ee_sni, sizeof(ee_sni));
  FIO_ASSERT(ret == 0, "EE with SNI parse should succeed");
  FIO_ASSERT(ee.has_server_name == 1, "Should have SNI acknowledgment");

  FIO_LOG_INFO("  PASS: EncryptedExtensions parsing tests");
}

/* *****************************************************************************
Test: Certificate Parsing
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_certificate(void) {
  FIO_LOG_INFO("Testing TLS 1.3 Certificate parsing...");

  /* Construct minimal Certificate message */
  uint8_t cert_msg[64];
  uint8_t *p = cert_msg;

  /* Certificate request context length (0 for server) */
  *p++ = 0;

  /* Certificate list length (3 bytes) */
  /* List contains: cert_len(3) + cert_data(8) + extensions_len(2) = 13 bytes */
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x0D; /* 13 bytes total */

  /* First certificate entry */
  /* Certificate data length (3 bytes) */
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x08; /* 8 bytes of cert data */

  /* Fake certificate data */
  for (int i = 0; i < 8; ++i)
    *p++ = (uint8_t)(0x30 + i);

  /* Extensions length for this cert entry */
  *p++ = 0x00;
  *p++ = 0x00;

  size_t msg_len = (size_t)(p - cert_msg);

  fio_tls13_certificate_s cert;
  int ret = fio_tls13_parse_certificate(&cert, cert_msg, msg_len);

  FIO_ASSERT(ret == 0, "Certificate parse should succeed");
  FIO_ASSERT(cert.cert_data != NULL, "Should have cert data pointer");
  FIO_ASSERT(cert.cert_len == 8, "Cert length should be 8");
  FIO_ASSERT(cert.cert_data[0] == 0x30, "Cert data should match");

  FIO_LOG_INFO("  PASS: Certificate parsing tests");
}

/* *****************************************************************************
Test: CertificateVerify Parsing
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_certificate_verify(void) {
  FIO_LOG_INFO("Testing TLS 1.3 CertificateVerify parsing...");

  /* Construct CertificateVerify message */
  uint8_t cv_msg[128];
  uint8_t *p = cv_msg;

  /* Signature algorithm: ed25519 (0x0807) */
  *p++ = 0x08;
  *p++ = 0x07;

  /* Signature length */
  *p++ = 0x00;
  *p++ = 0x40; /* 64 bytes */

  /* Fake signature */
  for (int i = 0; i < 64; ++i)
    *p++ = (uint8_t)i;

  size_t msg_len = (size_t)(p - cv_msg);

  fio_tls13_certificate_verify_s cv;
  int ret = fio_tls13_parse_certificate_verify(&cv, cv_msg, msg_len);

  FIO_ASSERT(ret == 0, "CertificateVerify parse should succeed");
  FIO_ASSERT(cv.signature_scheme == FIO_TLS13_SIG_ED25519,
             "Signature scheme should be ed25519");
  FIO_ASSERT(cv.signature_len == 64, "Signature length should be 64");
  FIO_ASSERT(cv.signature != NULL, "Should have signature pointer");
  FIO_ASSERT(cv.signature[0] == 0x00, "Signature data should match");

  FIO_LOG_INFO("  PASS: CertificateVerify parsing tests");
}

/* *****************************************************************************
Test: Finished Message
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_finished(void) {
  FIO_LOG_INFO("Testing TLS 1.3 Finished message...");

  uint8_t verify_data[32];
  for (int i = 0; i < 32; ++i)
    verify_data[i] = (uint8_t)(0x55 + i);

  /* Build Finished message */
  uint8_t fin_msg[64];
  int len = fio_tls13_build_finished(fin_msg, sizeof(fin_msg), verify_data, 32);

  FIO_ASSERT(len == 36,
             "Finished message should be 36 bytes (4 header + 32 data)");
  FIO_ASSERT(fin_msg[0] == FIO_TLS13_HS_FINISHED,
             "Type should be Finished (20)");
  FIO_ASSERT(fin_msg[1] == 0x00, "Length high byte");
  FIO_ASSERT(fin_msg[2] == 0x00, "Length mid byte");
  FIO_ASSERT(fin_msg[3] == 0x20, "Length low byte (32)");

  /* Parse and verify */
  int ret = fio_tls13_parse_finished(fin_msg + 4, 32, verify_data, 32);
  FIO_ASSERT(ret == 0, "Finished verification should succeed");

  /* Test with wrong data */
  uint8_t wrong_data[32];
  FIO_MEMCPY(wrong_data, verify_data, 32);
  wrong_data[0] ^= 0xFF;

  ret = fio_tls13_parse_finished(fin_msg + 4, 32, wrong_data, 32);
  FIO_ASSERT(ret != 0, "Finished verification should fail with wrong data");

  /* Test with wrong length */
  ret = fio_tls13_parse_finished(fin_msg + 4, 31, verify_data, 32);
  FIO_ASSERT(ret != 0, "Finished verification should fail with wrong length");

  FIO_LOG_INFO("  PASS: Finished message tests");
}

/* *****************************************************************************
Test: Full ClientHello/ServerHello Round-Trip
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_hello_roundtrip(void) {
  FIO_LOG_INFO("Testing TLS 1.3 Hello round-trip...");

  /* Generate X25519 keypair */
  uint8_t client_sk[32], client_pk[32];
  fio_rand_bytes(client_sk, 32);
  fio_x25519_public_key(client_pk, client_sk);

  /* Build ClientHello */
  uint8_t ch_buf[512];
  uint8_t client_random[32];
  fio_rand_bytes(client_random, 32);

  int ch_len = fio_tls13_build_client_hello(ch_buf,
                                            sizeof(ch_buf),
                                            client_random,
                                            "cloudflare.com",
                                            client_pk,
                                            NULL,
                                            0);

  FIO_ASSERT(ch_len > 100, "ClientHello should be substantial");
  FIO_ASSERT(ch_len < 400, "ClientHello should not be too large");

  /* Parse the header */
  fio_tls13_handshake_type_e msg_type;
  size_t body_len;
  const uint8_t *body = fio_tls13_parse_handshake_header(ch_buf,
                                                         (size_t)ch_len,
                                                         &msg_type,
                                                         &body_len);

  FIO_ASSERT(body != NULL, "Should parse ClientHello");
  FIO_ASSERT(msg_type == FIO_TLS13_HS_CLIENT_HELLO, "Should be ClientHello");
  FIO_ASSERT(body_len + 4 == (size_t)ch_len, "Lengths should match");

  FIO_LOG_INFO("  ClientHello size: %d bytes", ch_len);
  FIO_LOG_INFO("  PASS: Hello round-trip tests");
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  FIO_LOG_INFO("=== TLS 1.3 Handshake Message Tests ===\n");

  fio___test_tls13_handshake_header();
  fio___test_tls13_client_hello();
  fio___test_tls13_server_hello();
  fio___test_tls13_encrypted_extensions();
  fio___test_tls13_certificate();
  fio___test_tls13_certificate_verify();
  fio___test_tls13_finished();
  fio___test_tls13_hello_roundtrip();

  FIO_LOG_INFO("\n=== All TLS 1.3 Handshake Tests PASSED ===");
  return 0;
}
