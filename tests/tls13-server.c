/* *****************************************************************************
TLS 1.3 Server Tests

Tests for the TLS 1.3 server handshake state machine.

Test Categories:
1. Unit tests - Server state machine, message building/parsing
2. Integration tests - Client-server handshake (local loopback)
***************************************************************************** */
#define FIO_LOG
#define FIO_CRYPT
#define FIO_TLS13
#include "fio-stl.h"

/* *****************************************************************************
Test Certificate and Private Key (Ed25519)

This is a self-signed test certificate for testing purposes only.
Generated specifically for these tests - DO NOT use in production.
***************************************************************************** */

/* Ed25519 private key (32-byte seed) */
static const uint8_t test_ed25519_private_key[32] = {
    0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60, 0xba, 0x84, 0x4a,
    0xf4, 0x92, 0xec, 0x2c, 0xc4, 0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32,
    0x69, 0x19, 0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60};

/* Ed25519 public key (derived from private key) */
static uint8_t test_ed25519_public_key[32];

/* Minimal self-signed X.509 certificate (DER-encoded)
 * This is a simplified certificate structure for testing.
 * In production, use properly generated certificates. */
static uint8_t test_certificate[512];
static size_t test_certificate_len = 0;

/* Build a minimal self-signed certificate for testing */
FIO_SFUNC void build_test_certificate(void) {
  /* Derive public key from private key */
  fio_ed25519_public_key(test_ed25519_public_key, test_ed25519_private_key);

  /* Build a minimal X.509 certificate structure
   * This is a simplified version - real certificates are more complex */
  uint8_t *p = test_certificate;

  /* SEQUENCE (Certificate) */
  *p++ = 0x30;
  uint8_t *cert_len_ptr = p++;
  uint8_t *cert_start = p;

  /* SEQUENCE (TBSCertificate) */
  *p++ = 0x30;
  uint8_t *tbs_len_ptr = p++;
  uint8_t *tbs_start = p;

  /* Version: v3 (2) - [0] EXPLICIT INTEGER */
  *p++ = 0xA0;
  *p++ = 0x03;
  *p++ = 0x02;
  *p++ = 0x01;
  *p++ = 0x02;

  /* Serial Number: INTEGER */
  *p++ = 0x02;
  *p++ = 0x01;
  *p++ = 0x01;

  /* Signature Algorithm: Ed25519 (1.3.101.112) */
  *p++ = 0x30;
  *p++ = 0x05;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x2B;
  *p++ = 0x65;
  *p++ = 0x70;

  /* Issuer: CN=test */
  *p++ = 0x30;
  *p++ = 0x0F;
  *p++ = 0x31;
  *p++ = 0x0D;
  *p++ = 0x30;
  *p++ = 0x0B;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x55;
  *p++ = 0x04;
  *p++ = 0x03;
  *p++ = 0x0C;
  *p++ = 0x04;
  *p++ = 't';
  *p++ = 'e';
  *p++ = 's';
  *p++ = 't';

  /* Validity: UTCTime (not before/after) */
  *p++ = 0x30;
  *p++ = 0x1E;
  /* Not Before: 240101000000Z */
  *p++ = 0x17;
  *p++ = 0x0D;
  FIO_MEMCPY(p, "240101000000Z", 13);
  p += 13;
  /* Not After: 341231235959Z */
  *p++ = 0x17;
  *p++ = 0x0D;
  FIO_MEMCPY(p, "341231235959Z", 13);
  p += 13;

  /* Subject: CN=test (same as issuer for self-signed) */
  *p++ = 0x30;
  *p++ = 0x0F;
  *p++ = 0x31;
  *p++ = 0x0D;
  *p++ = 0x30;
  *p++ = 0x0B;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x55;
  *p++ = 0x04;
  *p++ = 0x03;
  *p++ = 0x0C;
  *p++ = 0x04;
  *p++ = 't';
  *p++ = 'e';
  *p++ = 's';
  *p++ = 't';

  /* SubjectPublicKeyInfo: Ed25519 */
  *p++ = 0x30;
  *p++ = 0x2A;
  /* Algorithm: Ed25519 */
  *p++ = 0x30;
  *p++ = 0x05;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x2B;
  *p++ = 0x65;
  *p++ = 0x70;
  /* Public Key: BIT STRING */
  *p++ = 0x03;
  *p++ = 0x21;
  *p++ = 0x00; /* No unused bits */
  FIO_MEMCPY(p, test_ed25519_public_key, 32);
  p += 32;

  /* Update TBS length */
  *tbs_len_ptr = (uint8_t)(p - tbs_start);

  /* Signature Algorithm: Ed25519 */
  *p++ = 0x30;
  *p++ = 0x05;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x2B;
  *p++ = 0x65;
  *p++ = 0x70;

  /* Signature: BIT STRING (Ed25519 signature over TBS) */
  uint8_t signature[64];
  fio_ed25519_sign(signature,
                   tbs_start - 2, /* Include SEQUENCE header */
                   (size_t)(p - tbs_start + 2),
                   test_ed25519_private_key,
                   test_ed25519_public_key);

  *p++ = 0x03;
  *p++ = 0x41;
  *p++ = 0x00; /* No unused bits */
  FIO_MEMCPY(p, signature, 64);
  p += 64;

  /* Update certificate length */
  *cert_len_ptr = (uint8_t)(p - cert_start);

  test_certificate_len = (size_t)(p - test_certificate);
}

/* *****************************************************************************
Test: Server Initialization and Cleanup
***************************************************************************** */
FIO_SFUNC void test_server_init_destroy(void) {
  FIO_LOG_DDEBUG("Testing server init/destroy\n");

  fio_tls13_server_s server;

  /* Test initialization */
  fio_tls13_server_init(&server);
  FIO_ASSERT(server.state == FIO_TLS13_SERVER_STATE_START,
             "Initial state should be START");
  FIO_ASSERT(!fio_tls13_server_is_connected(&server),
             "Should not be connected");
  FIO_ASSERT(!fio_tls13_server_is_error(&server), "Should not be in error");

  /* Test state name */
  FIO_ASSERT(strcmp(fio_tls13_server_state_name(&server), "START") == 0,
             "State name should be START");

  /* Test destroy */
  fio_tls13_server_destroy(&server);

  /* Verify secrets are zeroed */
  uint8_t zeros[32] = {0};
  FIO_ASSERT(FIO_MEMCMP(server.shared_secret, zeros, 32) == 0,
             "Shared secret should be zeroed after destroy");

  FIO_LOG_DDEBUG("  - Server init/destroy tests passed\n");
}

/* *****************************************************************************
Test: Server Certificate Configuration
***************************************************************************** */
FIO_SFUNC void test_server_cert_config(void) {
  FIO_LOG_DDEBUG("Testing server certificate configuration\n");

  build_test_certificate();

  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  /* Set certificate chain */
  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);

  FIO_ASSERT(server.cert_chain_count == 1, "Should have 1 certificate");
  FIO_ASSERT(server.cert_chain[0] == test_certificate,
             "Certificate pointer should match");
  FIO_ASSERT(server.cert_chain_lens[0] == test_certificate_len,
             "Certificate length should match");

  /* Set private key */
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  FIO_ASSERT(server.private_key == test_ed25519_private_key,
             "Private key pointer should match");
  FIO_ASSERT(server.private_key_len == 32, "Private key length should be 32");
  FIO_ASSERT(server.private_key_type == FIO_TLS13_SIG_ED25519,
             "Private key type should be Ed25519");

  fio_tls13_server_destroy(&server);

  FIO_LOG_DDEBUG("  - Server certificate configuration tests passed\n");
}

/* *****************************************************************************
Test: ClientHello Parsing
***************************************************************************** */
FIO_SFUNC void test_client_hello_parsing(void) {
  FIO_LOG_DDEBUG("Testing ClientHello parsing\n");

  /* Build a ClientHello using the client API */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");

  /* Buffer must be large enough for hybrid ClientHello (~1400 bytes) */
  uint8_t ch_record[2048];
  int ch_len = fio_tls13_client_start(&client, ch_record, sizeof(ch_record));
  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");

  /* Parse the ClientHello (skip record header) */
  fio_tls13_client_hello_s ch;
  const uint8_t *ch_msg = ch_record + 5; /* Skip 5-byte record header */
  size_t ch_msg_len = (size_t)ch_len - 5;

  /* Skip handshake header to get body */
  FIO_ASSERT(ch_msg[0] == FIO_TLS13_HS_CLIENT_HELLO,
             "Should be ClientHello message");
  const uint8_t *ch_body = ch_msg + 4;
  size_t ch_body_len = ch_msg_len - 4;

  int ret = fio___tls13_parse_client_hello(&ch, ch_body, ch_body_len);
  FIO_ASSERT(ret == 0, "ClientHello parsing should succeed");

  /* Verify parsed data */
  FIO_ASSERT(ch.cipher_suite_count >= 1, "Should have at least 1 cipher suite");
  FIO_ASSERT(ch.has_supported_versions == 1, "Should support TLS 1.3");
  FIO_ASSERT(ch.key_share_count >= 1, "Should have at least 1 key share");
  FIO_ASSERT(ch.server_name != NULL, "Should have SNI");
  FIO_ASSERT(ch.server_name_len == 16, "SNI length should be 16");
  FIO_ASSERT(FIO_MEMCMP(ch.server_name, "test.example.com", 16) == 0,
             "SNI should match");

  /* Check for X25519 key share */
  int found_x25519 = 0;
  for (size_t i = 0; i < ch.key_share_count; ++i) {
    if (ch.key_share_groups[i] == FIO_TLS13_GROUP_X25519) {
      found_x25519 = 1;
      FIO_ASSERT(ch.key_share_lens[i] == 32, "X25519 key should be 32 bytes");
      break;
    }
  }
  FIO_ASSERT(found_x25519, "Should have X25519 key share");

  fio_tls13_client_destroy(&client);

  FIO_LOG_DDEBUG("  - ClientHello parsing tests passed\n");
}

/* *****************************************************************************
Test: Browser-style ClientHello with Multiple Key Shares

Browsers typically send 2 key shares: X25519 (32 bytes) + P-256 (65 bytes).
This tests that the server correctly parses and selects from multiple key
shares.
***************************************************************************** */
FIO_SFUNC void test_browser_client_hello_parsing(void) {
  FIO_LOG_DDEBUG("Testing browser-style ClientHello with 2 key shares\n");

  /* Build a synthetic ClientHello with 2 key shares like a browser would send.
   * Key share extension format:
   *   2 bytes: total length of key shares
   *   For each key share:
   *     2 bytes: group (0x001d for X25519, 0x0017 for P-256)
   *     2 bytes: key length
   *     N bytes: key data
   */

  /* Generate random key share data */
  uint8_t x25519_key[32];
  uint8_t p256_key[65]; /* Uncompressed point: 0x04 + X(32) + Y(32) */
  fio_rand_bytes(x25519_key, 32);
  p256_key[0] = 0x04; /* Uncompressed point indicator */
  fio_rand_bytes(p256_key + 1, 64);

  /* Build a minimal ClientHello with 2 key shares */
  uint8_t ch_body[512];
  uint8_t *p = ch_body;

  /* Legacy version: TLS 1.2 (0x0303) */
  *p++ = 0x03;
  *p++ = 0x03;

  /* Random (32 bytes) */
  fio_rand_bytes(p, 32);
  p += 32;

  /* Legacy session ID (empty) */
  *p++ = 0;

  /* Cipher suites: 2 bytes length + 2 bytes per suite */
  *p++ = 0x00;
  *p++ = 0x02; /* 2 bytes = 1 cipher suite */
  *p++ = 0x13;
  *p++ = 0x01; /* TLS_AES_128_GCM_SHA256 */

  /* Legacy compression methods */
  *p++ = 0x01; /* 1 method */
  *p++ = 0x00; /* null compression */

  /* Extensions */
  uint8_t *ext_len_ptr = p;
  p += 2;
  uint8_t *ext_start = p;

  /* supported_versions extension */
  *p++ = 0x00;
  *p++ = 0x2b; /* Extension type: supported_versions (43) */
  *p++ = 0x00;
  *p++ = 0x03; /* Extension length */
  *p++ = 0x02; /* Versions length */
  *p++ = 0x03;
  *p++ = 0x04; /* TLS 1.3 (0x0304) */

  /* signature_algorithms extension */
  *p++ = 0x00;
  *p++ = 0x0d; /* Extension type: signature_algorithms (13) */
  *p++ = 0x00;
  *p++ = 0x04; /* Extension length */
  *p++ = 0x00;
  *p++ = 0x02; /* Algorithms length */
  *p++ = 0x04;
  *p++ = 0x03; /* ecdsa_secp256r1_sha256 */

  /* key_share extension with 2 key shares (X25519 + P-256) */
  *p++ = 0x00;
  *p++ = 0x33; /* Extension type: key_share (51) */

  /* Calculate key share extension length:
   * 2 bytes for shares_len
   * X25519: 2 (group) + 2 (len) + 32 (key) = 36
   * P-256:  2 (group) + 2 (len) + 65 (key) = 69
   * Total: 2 + 36 + 69 = 107
   */
  uint16_t key_share_ext_len = 2 + 36 + 69;
  *p++ = (uint8_t)(key_share_ext_len >> 8);
  *p++ = (uint8_t)(key_share_ext_len & 0xFF);

  /* Key shares length (36 + 69 = 105) */
  uint16_t shares_len = 36 + 69;
  *p++ = (uint8_t)(shares_len >> 8);
  *p++ = (uint8_t)(shares_len & 0xFF);

  /* X25519 key share */
  *p++ = 0x00;
  *p++ = 0x1d; /* Group: X25519 (29) */
  *p++ = 0x00;
  *p++ = 0x20; /* Key length: 32 */
  FIO_MEMCPY(p, x25519_key, 32);
  p += 32;

  /* P-256 key share */
  *p++ = 0x00;
  *p++ = 0x17; /* Group: secp256r1 (23) */
  *p++ = 0x00;
  *p++ = 0x41; /* Key length: 65 */
  FIO_MEMCPY(p, p256_key, 65);
  p += 65;

  /* Update extensions length */
  size_t ext_len = (size_t)(p - ext_start);
  ext_len_ptr[0] = (uint8_t)(ext_len >> 8);
  ext_len_ptr[1] = (uint8_t)(ext_len & 0xFF);

  size_t ch_body_len = (size_t)(p - ch_body);

  /* Parse the ClientHello */
  fio_tls13_client_hello_s ch;
  int ret = fio___tls13_parse_client_hello(&ch, ch_body, ch_body_len);
  FIO_ASSERT(ret == 0, "Browser ClientHello parsing should succeed");

  /* Verify we got 2 key shares */
  FIO_ASSERT(ch.key_share_count == 2,
             "Should have 2 key shares (got %zu)",
             ch.key_share_count);

  /* Verify X25519 key share */
  FIO_ASSERT(ch.key_share_groups[0] == FIO_TLS13_GROUP_X25519,
             "First key share should be X25519 (got 0x%04x)",
             ch.key_share_groups[0]);
  FIO_ASSERT(ch.key_share_lens[0] == 32,
             "X25519 key should be 32 bytes (got %u)",
             (unsigned)ch.key_share_lens[0]);
  FIO_ASSERT(ch.key_share_offsets[0] == 0,
             "X25519 offset should be 0 (got %u)",
             (unsigned)ch.key_share_offsets[0]);
  FIO_ASSERT(
      FIO_MEMCMP(ch.key_shares + ch.key_share_offsets[0], x25519_key, 32) == 0,
      "X25519 key data should match");

  /* Verify P-256 key share */
  FIO_ASSERT(ch.key_share_groups[1] == FIO_TLS13_GROUP_SECP256R1,
             "Second key share should be P-256 (got 0x%04x)",
             ch.key_share_groups[1]);
  FIO_ASSERT(ch.key_share_lens[1] == 65,
             "P-256 key should be 65 bytes (got %u)",
             (unsigned)ch.key_share_lens[1]);
  FIO_ASSERT(ch.key_share_offsets[1] == 32,
             "P-256 offset should be 32 (got %u)",
             (unsigned)ch.key_share_offsets[1]);
  FIO_ASSERT(
      FIO_MEMCMP(ch.key_shares + ch.key_share_offsets[1], p256_key, 65) == 0,
      "P-256 key data should match");

  /* Test key share selection - should select X25519 */
  build_test_certificate();
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  const uint8_t *client_key_share;
  size_t client_key_share_len;
  ret = fio___tls13_server_select_key_share(&server,
                                            &ch,
                                            &client_key_share,
                                            &client_key_share_len);
  FIO_ASSERT(ret == 0, "Key share selection should succeed");
  FIO_ASSERT(server.key_share_group == FIO_TLS13_GROUP_X25519,
             "Should select X25519");
  FIO_ASSERT(client_key_share_len == 32, "Selected key should be 32 bytes");
  FIO_ASSERT(FIO_MEMCMP(client_key_share, x25519_key, 32) == 0,
             "Selected key data should match X25519 key");

  fio_tls13_server_destroy(&server);

  FIO_LOG_DDEBUG("  - Browser ClientHello parsing tests passed\n");
}

/* *****************************************************************************
Test: Server Message Building
***************************************************************************** */
FIO_SFUNC void test_server_message_building(void) {
  FIO_LOG_DDEBUG("Testing server message building\n");

  build_test_certificate();

  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  /* Configure server */
  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  /* Set negotiated parameters */
  server.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  server.key_share_group = FIO_TLS13_GROUP_X25519;
  server.signature_scheme = FIO_TLS13_SIG_ED25519;
  server.use_sha384 = 0;

  /* Generate random and keypair */
  fio_rand_bytes(server.server_random, 32);
  fio_x25519_keypair(server.x25519_private_key, server.x25519_public_key);

  /* Test ServerHello building */
  uint8_t sh_buf[256];
  int sh_len = fio___tls13_build_server_hello(&server, sh_buf, sizeof(sh_buf));
  FIO_ASSERT(sh_len > 0, "ServerHello building should succeed");
  FIO_ASSERT(sh_buf[0] == FIO_TLS13_HS_SERVER_HELLO,
             "Should be ServerHello message");

  /* Test EncryptedExtensions building */
  uint8_t ee_buf[64];
  int ee_len =
      fio___tls13_build_encrypted_extensions(&server, ee_buf, sizeof(ee_buf));
  FIO_ASSERT(ee_len > 0, "EncryptedExtensions building should succeed");
  FIO_ASSERT(ee_buf[0] == FIO_TLS13_HS_ENCRYPTED_EXTENSIONS,
             "Should be EncryptedExtensions message");

  /* Test Certificate building */
  uint8_t cert_buf[1024];
  int cert_len =
      fio___tls13_build_certificate(&server, cert_buf, sizeof(cert_buf));
  FIO_ASSERT(cert_len > 0, "Certificate building should succeed");
  FIO_ASSERT(cert_buf[0] == FIO_TLS13_HS_CERTIFICATE,
             "Should be Certificate message");

  fio_tls13_server_destroy(&server);

  FIO_LOG_DDEBUG("  - Server message building tests passed\n");
}

/* *****************************************************************************
Test: Full Client-Server Handshake (Loopback)
***************************************************************************** */
FIO_SFUNC void test_client_server_handshake(void) {
  FIO_LOG_DDEBUG("Testing full client-server handshake\n");

  build_test_certificate();

  /* Initialize client */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");
  fio_tls13_client_skip_verification(&client, 1); /* Skip cert verification */

  /* Initialize server */
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  /* Buffers for message exchange */
  uint8_t client_out[8192];
  uint8_t server_out[8192];
  size_t out_len;

  /* Step 1: Client generates ClientHello */
  int ch_len = fio_tls13_client_start(&client, client_out, sizeof(client_out));
  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_WAIT_SH,
             "Client should be in WAIT_SH state");
  FIO_LOG_DDEBUG("  - ClientHello generated (%d bytes)\n", ch_len);

  /* Step 2: Server processes ClientHello and generates response */
  out_len = 0;
  int consumed = fio_tls13_server_process(&server,
                                          client_out,
                                          (size_t)ch_len,
                                          server_out,
                                          sizeof(server_out),
                                          &out_len);
  FIO_ASSERT(consumed == ch_len, "Server should consume entire ClientHello");
  FIO_ASSERT(out_len > 0, "Server should generate response");
  FIO_ASSERT(server.state == FIO_TLS13_SERVER_STATE_WAIT_FINISHED,
             "Server should be in WAIT_FINISHED state");
  fprintf(
      stderr,
      "\t  - Server processed ClientHello, generated response (%zu bytes)\n",
      out_len);

  /* Verify SNI was captured */
  FIO_ASSERT(server.client_sni_len == 16, "SNI length should be 16");
  FIO_ASSERT(strcmp(server.client_sni, "test.example.com") == 0,
             "SNI should match");

  /* Step 3: Client processes server response */
  size_t client_response_len = 0;
  size_t offset = 0;
  while (offset < out_len && !fio_tls13_client_is_connected(&client) &&
         !fio_tls13_client_is_error(&client)) {
    size_t msg_out_len = 0;
    int proc =
        fio_tls13_client_process(&client,
                                 server_out + offset,
                                 out_len - offset,
                                 client_out + client_response_len,
                                 sizeof(client_out) - client_response_len,
                                 &msg_out_len);
    if (proc <= 0)
      break;
    offset += (size_t)proc;
    client_response_len += msg_out_len;
  }

  FIO_ASSERT(!fio_tls13_client_is_error(&client),
             "Client should not be in error state");
  FIO_ASSERT(fio_tls13_client_is_connected(&client),
             "Client should be connected");
  FIO_ASSERT(client_response_len > 0, "Client should generate Finished");
  fprintf(stderr,
          "\t  - Client processed server response, generated Finished (%zu "
          "bytes)\n",
          client_response_len);

  /* Step 4: Server processes client Finished */
  out_len = 0;
  consumed = fio_tls13_server_process(&server,
                                      client_out,
                                      client_response_len,
                                      server_out,
                                      sizeof(server_out),
                                      &out_len);
  FIO_ASSERT(consumed == (int)client_response_len,
             "Server should consume client Finished");
  FIO_ASSERT(fio_tls13_server_is_connected(&server),
             "Server should be connected");
  FIO_LOG_DDEBUG("  - Server processed client Finished\n");

  /* Step 5: Test application data exchange */
  const char *test_message = "Hello from client!";
  size_t test_message_len = strlen(test_message);

  /* Client encrypts message */
  int enc_len = fio_tls13_client_encrypt(&client,
                                         client_out,
                                         sizeof(client_out),
                                         (const uint8_t *)test_message,
                                         test_message_len);
  FIO_ASSERT(enc_len > 0, "Client encryption should succeed");
  FIO_LOG_DDEBUG("  - Client encrypted message (%d bytes)\n", enc_len);

  /* Server decrypts message */
  uint8_t decrypted[256];
  int dec_len = fio_tls13_server_decrypt(&server,
                                         decrypted,
                                         sizeof(decrypted),
                                         client_out,
                                         (size_t)enc_len);
  FIO_ASSERT(dec_len == (int)test_message_len, "Decrypted length should match");
  FIO_ASSERT(FIO_MEMCMP(decrypted, test_message, test_message_len) == 0,
             "Decrypted message should match");
  FIO_LOG_DDEBUG("  - Server decrypted message successfully\n");

  /* Server sends response */
  const char *response = "Hello from server!";
  size_t response_len = strlen(response);

  enc_len = fio_tls13_server_encrypt(&server,
                                     server_out,
                                     sizeof(server_out),
                                     (const uint8_t *)response,
                                     response_len);
  FIO_ASSERT(enc_len > 0, "Server encryption should succeed");

  /* Client decrypts response */
  dec_len = fio_tls13_client_decrypt(&client,
                                     decrypted,
                                     sizeof(decrypted),
                                     server_out,
                                     (size_t)enc_len);
  FIO_ASSERT(dec_len == (int)response_len, "Decrypted length should match");
  FIO_ASSERT(FIO_MEMCMP(decrypted, response, response_len) == 0,
             "Decrypted response should match");
  FIO_LOG_DDEBUG("  - Client decrypted server response successfully\n");

  /* Cleanup */
  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);

  FIO_LOG_DDEBUG("  - Full client-server handshake tests passed\n");
}

/* *****************************************************************************
Test: Multiple Cipher Suites
***************************************************************************** */
FIO_SFUNC void test_cipher_suite_negotiation(void) {
  FIO_LOG_DDEBUG("Testing cipher suite negotiation\n");

  build_test_certificate();

  /* Test each supported cipher suite */
  uint16_t cipher_suites[] = {
      FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256,
      FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256,
      FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384,
  };
  const char *cipher_names[] = {
      "AES-128-GCM-SHA256",
      "ChaCha20-Poly1305-SHA256",
      "AES-256-GCM-SHA384",
  };
  (void)cipher_names; /* Used only in debug logging */

  for (size_t i = 0; i < sizeof(cipher_suites) / sizeof(cipher_suites[0]);
       ++i) {
    FIO_LOG_DDEBUG("  - Testing %s\n", cipher_names[i]);

    /* Initialize client and server */
    fio_tls13_client_s client;
    fio_tls13_client_init(&client, "test");
    fio_tls13_client_skip_verification(&client, 1);

    fio_tls13_server_s server;
    fio_tls13_server_init(&server);

    const uint8_t *certs[] = {test_certificate};
    size_t cert_lens[] = {test_certificate_len};
    fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
    fio_tls13_server_set_private_key(&server,
                                     test_ed25519_private_key,
                                     32,
                                     FIO_TLS13_SIG_ED25519);

    /* Perform handshake */
    uint8_t client_out[8192];
    uint8_t server_out[8192];
    size_t out_len;

    int ch_len =
        fio_tls13_client_start(&client, client_out, sizeof(client_out));
    FIO_ASSERT(ch_len > 0, "ClientHello should succeed");

    out_len = 0;
    int consumed = fio_tls13_server_process(&server,
                                            client_out,
                                            (size_t)ch_len,
                                            server_out,
                                            sizeof(server_out),
                                            &out_len);
    FIO_ASSERT(consumed > 0, "Server should process ClientHello");

    /* Process server response */
    size_t client_response_len = 0;
    size_t offset = 0;
    while (offset < out_len && !fio_tls13_client_is_connected(&client)) {
      size_t msg_out_len = 0;
      int proc =
          fio_tls13_client_process(&client,
                                   server_out + offset,
                                   out_len - offset,
                                   client_out + client_response_len,
                                   sizeof(client_out) - client_response_len,
                                   &msg_out_len);
      if (proc <= 0)
        break;
      offset += (size_t)proc;
      client_response_len += msg_out_len;
    }

    FIO_ASSERT(fio_tls13_client_is_connected(&client),
               "Client should be connected");

    /* Process client Finished */
    out_len = 0;
    consumed = fio_tls13_server_process(&server,
                                        client_out,
                                        client_response_len,
                                        server_out,
                                        sizeof(server_out),
                                        &out_len);
    FIO_ASSERT(fio_tls13_server_is_connected(&server),
               "Server should be connected");

    /* Verify negotiated cipher suite */
    FIO_ASSERT(client.cipher_suite == server.cipher_suite,
               "Cipher suites should match");

    fio_tls13_client_destroy(&client);
    fio_tls13_server_destroy(&server);
  }

  FIO_LOG_DDEBUG("  - Cipher suite negotiation tests passed\n");
}

/* *****************************************************************************
Test: Error Handling
***************************************************************************** */
FIO_SFUNC void test_error_handling(void) {
  FIO_LOG_DDEBUG("Testing error handling\n");

  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  /* Test processing garbage data */
  uint8_t garbage[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t out[256];
  size_t out_len;

  int ret = fio_tls13_server_process(&server,
                                     garbage,
                                     sizeof(garbage),
                                     out,
                                     sizeof(out),
                                     &out_len);
  /* Should fail or need more data */
  FIO_ASSERT(ret <= 0 || server.state == FIO_TLS13_SERVER_STATE_START,
             "Garbage should be rejected");

  /* Test encryption before connected */
  uint8_t plaintext[] = "test";
  ret = fio_tls13_server_encrypt(&server,
                                 out,
                                 sizeof(out),
                                 plaintext,
                                 sizeof(plaintext));
  FIO_ASSERT(ret == -1, "Encryption should fail when not connected");

  /* Test decryption before connected */
  ret = fio_tls13_server_decrypt(&server,
                                 out,
                                 sizeof(out),
                                 garbage,
                                 sizeof(garbage));
  FIO_ASSERT(ret == -1, "Decryption should fail when not connected");

  fio_tls13_server_destroy(&server);

  FIO_LOG_DDEBUG("  - Error handling tests passed\n");
}

/* *****************************************************************************
Test: State Machine Transitions
***************************************************************************** */
FIO_SFUNC void test_state_machine(void) {
  FIO_LOG_DDEBUG("Testing state machine transitions\n");

  build_test_certificate();

  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  /* Verify initial state */
  FIO_ASSERT(server.state == FIO_TLS13_SERVER_STATE_START,
             "Initial state should be START");

  /* Generate ClientHello */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test");
  fio_tls13_client_skip_verification(&client, 1);

  uint8_t client_out[8192];
  uint8_t server_out[8192];
  size_t out_len;

  int ch_len = fio_tls13_client_start(&client, client_out, sizeof(client_out));
  FIO_ASSERT(ch_len > 0, "ClientHello should succeed");

  /* Process ClientHello - should transition to WAIT_FINISHED */
  out_len = 0;
  int consumed = fio_tls13_server_process(&server,
                                          client_out,
                                          (size_t)ch_len,
                                          server_out,
                                          sizeof(server_out),
                                          &out_len);
  FIO_ASSERT(consumed > 0, "Should process ClientHello");
  FIO_ASSERT(server.state == FIO_TLS13_SERVER_STATE_WAIT_FINISHED,
             "State should be WAIT_FINISHED after processing ClientHello");

  /* Process server response on client side */
  size_t client_response_len = 0;
  size_t offset = 0;
  while (offset < out_len && !fio_tls13_client_is_connected(&client)) {
    size_t msg_out_len = 0;
    int proc =
        fio_tls13_client_process(&client,
                                 server_out + offset,
                                 out_len - offset,
                                 client_out + client_response_len,
                                 sizeof(client_out) - client_response_len,
                                 &msg_out_len);
    if (proc <= 0)
      break;
    offset += (size_t)proc;
    client_response_len += msg_out_len;
  }

  /* Process client Finished - should transition to CONNECTED */
  out_len = 0;
  consumed = fio_tls13_server_process(&server,
                                      client_out,
                                      client_response_len,
                                      server_out,
                                      sizeof(server_out),
                                      &out_len);
  FIO_ASSERT(consumed > 0, "Should process client Finished");
  FIO_ASSERT(server.state == FIO_TLS13_SERVER_STATE_CONNECTED,
             "State should be CONNECTED after processing client Finished");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);

  FIO_LOG_DDEBUG("  - State machine transition tests passed\n");
}

/* *****************************************************************************
Test: Browser ClientHello with GREASE + X25519MLKEM768 (Kyber) + X25519

Modern browsers send key shares in this order:
1. GREASE (random group, small key) - to test server ignores unknown groups
2. X25519MLKEM768 (0x11ec, 1216 bytes) - post-quantum hybrid key exchange
3. X25519 (0x001d, 32 bytes) - classical key exchange (fallback)

This tests that all 3 key shares are correctly parsed and stored.
***************************************************************************** */
FIO_SFUNC void test_browser_grease_kyber_x25519(void) {
  FIO_LOG_DDEBUG(
      "Testing browser ClientHello with GREASE + X25519MLKEM768 + X25519\n");

  /* Generate known X25519 key for verification */
  uint8_t x25519_key[32];
  for (int i = 0; i < 32; ++i)
    x25519_key[i] = (uint8_t)(0xAA + i); /* Known pattern */

  /* Build a minimal ClientHello with 3 key shares:
   * 1. GREASE (0x0a0a, 1 byte)
   * 2. X25519MLKEM768 (0x11ec, 1216 bytes) - post-quantum hybrid
   * 3. X25519 (0x001d, 32 bytes)
   */
  uint8_t ch_body[2048];
  uint8_t *p = ch_body;

  /* Legacy version: TLS 1.2 (0x0303) */
  *p++ = 0x03;
  *p++ = 0x03;

  /* Random (32 bytes) */
  fio_rand_bytes(p, 32);
  p += 32;

  /* Legacy session ID (empty) */
  *p++ = 0;

  /* Cipher suites: 2 bytes length + 2 bytes per suite */
  *p++ = 0x00;
  *p++ = 0x02; /* 2 bytes = 1 cipher suite */
  *p++ = 0x13;
  *p++ = 0x01; /* TLS_AES_128_GCM_SHA256 */

  /* Legacy compression methods */
  *p++ = 0x01; /* 1 method */
  *p++ = 0x00; /* null compression */

  /* Extensions */
  uint8_t *ext_len_ptr = p;
  p += 2;
  uint8_t *ext_start = p;

  /* supported_versions extension */
  *p++ = 0x00;
  *p++ = 0x2b; /* Extension type: supported_versions (43) */
  *p++ = 0x00;
  *p++ = 0x03; /* Extension length */
  *p++ = 0x02; /* Versions length */
  *p++ = 0x03;
  *p++ = 0x04; /* TLS 1.3 (0x0304) */

  /* signature_algorithms extension */
  *p++ = 0x00;
  *p++ = 0x0d; /* Extension type: signature_algorithms (13) */
  *p++ = 0x00;
  *p++ = 0x04; /* Extension length */
  *p++ = 0x00;
  *p++ = 0x02; /* Algorithms length */
  *p++ = 0x04;
  *p++ = 0x03; /* ecdsa_secp256r1_sha256 */

  /* key_share extension with 3 key shares:
   * GREASE: 2 (group) + 2 (len) + 1 (key) = 5
   * Kyber:  2 (group) + 2 (len) + 1216 (key) = 1220
   * X25519: 2 (group) + 2 (len) + 32 (key) = 36
   * Total shares: 5 + 1220 + 36 = 1261
   * Extension: 2 (shares_len) + 1261 = 1263
   */
  *p++ = 0x00;
  *p++ = 0x33; /* Extension type: key_share (51) */

  uint16_t key_share_ext_len = 2 + 5 + 1220 + 36;
  *p++ = (uint8_t)(key_share_ext_len >> 8);
  *p++ = (uint8_t)(key_share_ext_len & 0xFF);

  /* Key shares length */
  uint16_t shares_len = 5 + 1220 + 36;
  *p++ = (uint8_t)(shares_len >> 8);
  *p++ = (uint8_t)(shares_len & 0xFF);

  /* GREASE key share (0x0a0a, 1 byte) */
  *p++ = 0x0a;
  *p++ = 0x0a; /* Group: GREASE */
  *p++ = 0x00;
  *p++ = 0x01; /* Key length: 1 */
  *p++ = 0x00; /* GREASE key data (1 byte) */

  /* Kyber key share (0x11ec, 1216 bytes) */
  *p++ = 0x11;
  *p++ = 0xec; /* Group: Kyber (4588) */
  *p++ = 0x04;
  *p++ = 0xc0; /* Key length: 1216 (0x04c0) */
  /* Fill with pattern to detect if we accidentally read from here */
  for (int i = 0; i < 1216; ++i)
    *p++ = (uint8_t)(0x80 + (i & 0x7F)); /* 0x80-0xFF pattern */

  /* X25519 key share (0x001d, 32 bytes) */
  *p++ = 0x00;
  *p++ = 0x1d; /* Group: X25519 (29) */
  *p++ = 0x00;
  *p++ = 0x20; /* Key length: 32 */
  FIO_MEMCPY(p, x25519_key, 32);
  p += 32;

  /* Update extensions length */
  size_t ext_len = (size_t)(p - ext_start);
  ext_len_ptr[0] = (uint8_t)(ext_len >> 8);
  ext_len_ptr[1] = (uint8_t)(ext_len & 0xFF);

  size_t ch_body_len = (size_t)(p - ch_body);

  /* Parse the ClientHello */
  fio_tls13_client_hello_s ch;
  int ret = fio___tls13_parse_client_hello(&ch, ch_body, ch_body_len);
  FIO_ASSERT(ret == 0, "ClientHello parsing should succeed");

  /* We should have 3 key shares stored (GREASE + X25519MLKEM768 + X25519) */
  FIO_ASSERT(ch.key_share_count == 3,
             "Should have 3 key shares (got %zu) - GREASE + X25519MLKEM768 + "
             "X25519",
             ch.key_share_count);

  /* Verify GREASE key share */
  FIO_ASSERT(ch.key_share_groups[0] == 0x0a0a,
             "First key share should be GREASE (got 0x%04x)",
             ch.key_share_groups[0]);
  FIO_ASSERT(ch.key_share_lens[0] == 1,
             "GREASE key should be 1 byte (got %u)",
             (unsigned)ch.key_share_lens[0]);
  FIO_ASSERT(ch.key_share_offsets[0] == 0,
             "GREASE offset should be 0 (got %u)",
             (unsigned)ch.key_share_offsets[0]);

  /* Verify X25519MLKEM768 key share */
  FIO_ASSERT(ch.key_share_groups[1] == FIO_TLS13_GROUP_X25519MLKEM768,
             "Second key share should be X25519MLKEM768 (got 0x%04x)",
             ch.key_share_groups[1]);
  FIO_ASSERT(ch.key_share_lens[1] == 1216,
             "X25519MLKEM768 key should be 1216 bytes (got %u)",
             (unsigned)ch.key_share_lens[1]);
  FIO_ASSERT(ch.key_share_offsets[1] == 1,
             "X25519MLKEM768 offset should be 1 (got %u)",
             (unsigned)ch.key_share_offsets[1]);

  /* Verify X25519 key share */
  FIO_ASSERT(ch.key_share_groups[2] == FIO_TLS13_GROUP_X25519,
             "Third key share should be X25519 (got 0x%04x)",
             ch.key_share_groups[2]);
  FIO_ASSERT(ch.key_share_lens[2] == 32,
             "X25519 key should be 32 bytes (got %u)",
             (unsigned)ch.key_share_lens[2]);
  FIO_ASSERT(ch.key_share_offsets[2] == 1217,
             "X25519 offset should be 1217 (1 + 1216) (got %u)",
             (unsigned)ch.key_share_offsets[2]);

  /* CRITICAL: Verify X25519 key DATA is correct */
  const uint8_t *stored_x25519 = ch.key_shares + ch.key_share_offsets[2];
  FIO_ASSERT(FIO_MEMCMP(stored_x25519, x25519_key, 32) == 0,
             "X25519 key data should match! First bytes: %02x %02x %02x %02x "
             "(expected %02x %02x %02x %02x)",
             stored_x25519[0],
             stored_x25519[1],
             stored_x25519[2],
             stored_x25519[3],
             x25519_key[0],
             x25519_key[1],
             x25519_key[2],
             x25519_key[3]);

  /* Test key share selection - should select X25519 */
  build_test_certificate();
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  const uint8_t *client_key_share;
  size_t client_key_share_len;
  ret = fio___tls13_server_select_key_share(&server,
                                            &ch,
                                            &client_key_share,
                                            &client_key_share_len);
  FIO_ASSERT(ret == 0, "Key share selection should succeed");

  /* Server now prefers X25519MLKEM768 when available (post-quantum security) */
#if defined(H___FIO_MLKEM___H)
  FIO_ASSERT(server.key_share_group == FIO_TLS13_GROUP_X25519MLKEM768,
             "Should select X25519MLKEM768 group (got 0x%04x)",
             server.key_share_group);
  FIO_ASSERT(client_key_share_len == 1216,
             "Selected key should be 1216 bytes (got %zu)",
             client_key_share_len);
#else
  FIO_ASSERT(server.key_share_group == FIO_TLS13_GROUP_X25519,
             "Should select X25519 group");
  FIO_ASSERT(client_key_share_len == 32, "Selected key should be 32 bytes");
  FIO_ASSERT(FIO_MEMCMP(client_key_share, x25519_key, 32) == 0,
             "Selected key data should match X25519 key");
#endif

  fio_tls13_server_destroy(&server);

  FIO_LOG_DDEBUG("  - Browser GREASE + X25519MLKEM768 + X25519 ClientHello "
                 "tests passed\n");
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  FIO_LOG_DDEBUG("=== TLS 1.3 Server Tests ===\n\n");

  /* Unit tests */
  FIO_LOG_DDEBUG("Unit Tests:");
  test_server_init_destroy();
  test_server_cert_config();
  test_client_hello_parsing();
  test_browser_client_hello_parsing();
  test_browser_grease_kyber_x25519();
  test_server_message_building();
  test_state_machine();
  test_error_handling();

  /* Integration tests */
  FIO_LOG_DDEBUG("Integration Tests:");
  test_client_server_handshake();
  test_cipher_suite_negotiation();

  FIO_LOG_DDEBUG("=== All TLS 1.3 Server Tests PASSED ===\n\n");
  return 0;
}
