/* *****************************************************************************
TLS 1.3 Client Certificate Authentication Tests

Tests for mutual TLS (mTLS) / client certificate authentication per RFC 8446:
- Section 4.3.2: CertificateRequest message
- Section 4.4.2: Certificate message (client)
- Section 4.4.3: CertificateVerify message (client)

Test Categories:
1. CertificateRequest building/parsing
2. Client Certificate building
3. Client CertificateVerify building
4. Server verification of client cert
5. Empty client Certificate handling
6. Full mTLS handshake integration test
***************************************************************************** */
#define FIO_LOG
#define FIO_CRYPT
#define FIO_TLS13
#include "test-helpers.h"

/* *****************************************************************************
Test Certificate and Private Key (Ed25519)

These are test certificates for testing purposes only.
DO NOT use in production.
***************************************************************************** */

/* Server Ed25519 private key (32-byte seed) */
static const uint8_t server_ed25519_private_key[32] = {
    0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60, 0xba, 0x84, 0x4a,
    0xf4, 0x92, 0xec, 0x2c, 0xc4, 0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32,
    0x69, 0x19, 0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60};

/* Client Ed25519 private key (different from server) */
static const uint8_t client_ed25519_private_key[32] = {
    0x4c, 0xcd, 0x08, 0x9b, 0x28, 0xff, 0x96, 0xda, 0x9d, 0xb6, 0xc3,
    0x46, 0xec, 0x11, 0x4e, 0x0f, 0x5b, 0x8a, 0x31, 0x9f, 0x35, 0xab,
    0xa6, 0x24, 0xda, 0x8c, 0xf6, 0xed, 0x4f, 0xb8, 0xa6, 0xfb};

/* Derived public keys */
static uint8_t server_ed25519_public_key[32];
static uint8_t client_ed25519_public_key[32];

/* Test certificates */
static uint8_t server_certificate[512];
static size_t server_certificate_len = 0;
static uint8_t client_certificate[512];
static size_t client_certificate_len = 0;

/* Build a minimal self-signed certificate for testing */
FIO_SFUNC size_t build_test_certificate(uint8_t *cert,
                                        const uint8_t *private_key,
                                        const uint8_t *public_key,
                                        const char *cn) {
  uint8_t *p = cert;
  size_t cn_len = strlen(cn);

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

  /* Issuer: CN=<cn> */
  *p++ = 0x30;
  *p++ = (uint8_t)(0x0B + cn_len);
  *p++ = 0x31;
  *p++ = (uint8_t)(0x09 + cn_len);
  *p++ = 0x30;
  *p++ = (uint8_t)(0x07 + cn_len);
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x55;
  *p++ = 0x04;
  *p++ = 0x03;
  *p++ = 0x0C;
  *p++ = (uint8_t)cn_len;
  FIO_MEMCPY(p, cn, cn_len);
  p += cn_len;

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

  /* Subject: CN=<cn> (same as issuer for self-signed) */
  *p++ = 0x30;
  *p++ = (uint8_t)(0x0B + cn_len);
  *p++ = 0x31;
  *p++ = (uint8_t)(0x09 + cn_len);
  *p++ = 0x30;
  *p++ = (uint8_t)(0x07 + cn_len);
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x55;
  *p++ = 0x04;
  *p++ = 0x03;
  *p++ = 0x0C;
  *p++ = (uint8_t)cn_len;
  FIO_MEMCPY(p, cn, cn_len);
  p += cn_len;

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
  FIO_MEMCPY(p, public_key, 32);
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
                   private_key,
                   public_key);

  *p++ = 0x03;
  *p++ = 0x41;
  *p++ = 0x00; /* No unused bits */
  FIO_MEMCPY(p, signature, 64);
  p += 64;

  /* Update certificate length */
  *cert_len_ptr = (uint8_t)(p - cert_start);

  return (size_t)(p - cert);
}

/* Initialize test certificates */
FIO_SFUNC void init_test_certificates(void) {
  /* Derive public keys */
  fio_ed25519_public_key(server_ed25519_public_key, server_ed25519_private_key);
  fio_ed25519_public_key(client_ed25519_public_key, client_ed25519_private_key);

  /* Build certificates */
  server_certificate_len = build_test_certificate(server_certificate,
                                                  server_ed25519_private_key,
                                                  server_ed25519_public_key,
                                                  "server");
  client_certificate_len = build_test_certificate(client_certificate,
                                                  client_ed25519_private_key,
                                                  client_ed25519_public_key,
                                                  "client");
}

/* *****************************************************************************
Test: CertificateRequest Building and Parsing
***************************************************************************** */
FIO_SFUNC void test_certificate_request_build_parse(void) {
  /* Build a CertificateRequest message */
  uint8_t cr_msg[256];
  uint8_t context[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  uint16_t sig_algs[] = {FIO_TLS13_SIG_ED25519,
                         FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256,
                         FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256};
  size_t sig_alg_count = 3;

  int cr_len = fio_tls13_build_certificate_request(cr_msg,
                                                   sizeof(cr_msg),
                                                   context,
                                                   sizeof(context),
                                                   sig_algs,
                                                   sig_alg_count);
  FIO_ASSERT(cr_len > 0, "CertificateRequest build should succeed");

  /* Parse the CertificateRequest message (skip 4-byte handshake header) */
  fio_tls13_certificate_request_s cr;
  int ret =
      fio_tls13_parse_certificate_request(&cr, cr_msg + 4, (size_t)cr_len - 4);
  FIO_ASSERT(ret == 0, "CertificateRequest parse should succeed");

  /* Verify parsed data */
  FIO_ASSERT(cr.certificate_request_context_len == sizeof(context),
             "Context length should match");
  FIO_ASSERT(
      FIO_MEMCMP(cr.certificate_request_context, context, sizeof(context)) == 0,
      "Context should match");
  FIO_ASSERT(cr.signature_algorithm_count == sig_alg_count,
             "Signature algorithm count should match");
  for (size_t i = 0; i < sig_alg_count; ++i) {
    FIO_ASSERT(cr.signature_algorithms[i] == sig_algs[i],
               "Signature algorithm should match");
  }
}

/* *****************************************************************************
Test: Server Require Client Cert API
***************************************************************************** */
FIO_SFUNC void test_server_require_client_cert(void) {
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  /* Default should be no client cert required */
  FIO_ASSERT(server.require_client_cert == 0,
             "Default should be no client cert required");

  /* Test setting modes */
  fio_tls13_server_require_client_cert(&server, 1);
  FIO_ASSERT(server.require_client_cert == 1, "Should be optional (1)");

  fio_tls13_server_require_client_cert(&server, 2);
  FIO_ASSERT(server.require_client_cert == 2, "Should be required (2)");

  fio_tls13_server_require_client_cert(&server, 0);
  FIO_ASSERT(server.require_client_cert == 0, "Should be none (0)");

  /* Test client cert received/verified APIs */
  FIO_ASSERT(fio_tls13_server_client_cert_received(&server) == 0,
             "No client cert received yet");
  FIO_ASSERT(fio_tls13_server_client_cert_verified(&server) == 0,
             "No client cert verified yet");

  size_t cert_len;
  FIO_ASSERT(fio_tls13_server_get_client_cert(&server, &cert_len) == NULL,
             "No client cert available");
  FIO_ASSERT(cert_len == 0, "Cert length should be 0");

  fio_tls13_server_destroy(&server);
}

/* *****************************************************************************
Test: Client Set Certificate API
***************************************************************************** */
FIO_SFUNC void test_client_set_cert(void) {
  init_test_certificates();

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");

  /* Set client certificate */
  fio_tls13_client_set_cert(&client,
                            client_certificate,
                            client_certificate_len,
                            client_ed25519_private_key,
                            32,
                            FIO_TLS13_SIG_ED25519);

  FIO_ASSERT(client.client_cert == client_certificate,
             "Client cert pointer should match");
  FIO_ASSERT(client.client_cert_len == client_certificate_len,
             "Client cert length should match");

  /* Check cert_requested API */
  FIO_ASSERT(fio_tls13_client_cert_requested(&client) == 0,
             "No cert request received yet");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Full mTLS Handshake (Client-Server Integration)
***************************************************************************** */
FIO_SFUNC void test_mtls_handshake(void) {
  init_test_certificates();

  /* Initialize server with client cert required */
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *server_certs[] = {server_certificate};
  size_t server_cert_lens[] = {server_certificate_len};
  fio_tls13_server_set_cert_chain(&server, server_certs, server_cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   server_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);
  fio_tls13_server_require_client_cert(&server, 2); /* Required */

  /* Initialize client with certificate */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");
  fio_tls13_client_skip_verification(&client, 1); /* Skip server cert verify */
  fio_tls13_client_set_cert(&client,
                            client_certificate,
                            client_certificate_len,
                            client_ed25519_private_key,
                            32,
                            FIO_TLS13_SIG_ED25519);

  /* Buffers for handshake messages */
  uint8_t client_out[4096];
  uint8_t server_out[4096];
  size_t server_out_len;

  /* Step 1: Client sends ClientHello */
  int ch_len = fio_tls13_client_start(&client, client_out, sizeof(client_out));
  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");
  /* Step 2: Server processes ClientHello, sends ServerHello..Finished */
  int consumed = fio_tls13_server_process(&server,
                                          client_out,
                                          (size_t)ch_len,
                                          server_out,
                                          sizeof(server_out),
                                          &server_out_len);
  FIO_ASSERT(consumed == ch_len, "Server should consume entire ClientHello");
  FIO_ASSERT(server_out_len > 0, "Server should generate response");
  /* Server should now be waiting for client certificate */
  FIO_ASSERT(server.state == FIO_TLS13_SERVER_STATE_WAIT_CLIENT_CERT,
             "Server should be in WAIT_CLIENT_CERT state (got %s)",
             fio_tls13_server_state_name(&server));

  /* Step 3: Client processes server flight */
  size_t client_out_len = 0;
  size_t offset = 0;
  while (offset < server_out_len) {
    int proc = fio_tls13_client_process(&client,
                                        server_out + offset,
                                        server_out_len - offset,
                                        client_out,
                                        sizeof(client_out),
                                        &client_out_len);
    if (proc <= 0) {
    }
    FIO_ASSERT(proc > 0,
               "Client should process server records (proc=%d)",
               proc);
    offset += (size_t)proc;
  }

  /* Client should have received CertificateRequest */
  FIO_ASSERT(fio_tls13_client_cert_requested(&client) == 1,
             "Client should have received CertificateRequest");

  /* Client should have generated Certificate + CertificateVerify + Finished */
  FIO_ASSERT(client_out_len > 0, "Client should generate response");
  /* Step 4: Server processes client Certificate */
  offset = 0;
  while (offset < client_out_len &&
         server.state != FIO_TLS13_SERVER_STATE_CONNECTED) {
    consumed = fio_tls13_server_process(&server,
                                        client_out + offset,
                                        client_out_len - offset,
                                        server_out,
                                        sizeof(server_out),
                                        &server_out_len);
    if (consumed <= 0)
      break;
    offset += (size_t)consumed;
  }

  /* Verify handshake completed */
  FIO_ASSERT(fio_tls13_server_is_connected(&server),
             "Server should be connected (state=%s)",
             fio_tls13_server_state_name(&server));
  FIO_ASSERT(fio_tls13_client_is_connected(&client),
             "Client should be connected");

  /* Verify client certificate was received and verified */
  FIO_ASSERT(fio_tls13_server_client_cert_received(&server) == 1,
             "Server should have received client certificate");
  FIO_ASSERT(fio_tls13_server_client_cert_verified(&server) == 1,
             "Server should have verified client certificate");

  /* Get client certificate from server */
  size_t got_cert_len;
  const uint8_t *got_cert =
      fio_tls13_server_get_client_cert(&server, &got_cert_len);
  FIO_ASSERT(got_cert != NULL, "Should be able to get client certificate");
  FIO_ASSERT(got_cert_len == client_certificate_len,
             "Client certificate length should match");
  FIO_ASSERT(FIO_MEMCMP(got_cert, client_certificate, client_certificate_len) ==
                 0,
             "Client certificate should match");

  /* Test application data exchange */
  const char *test_msg = "Hello from client!";
  uint8_t encrypted[256];
  uint8_t decrypted[256];

  int enc_len = fio_tls13_client_encrypt(&client,
                                         encrypted,
                                         sizeof(encrypted),
                                         (const uint8_t *)test_msg,
                                         strlen(test_msg));
  FIO_ASSERT(enc_len > 0, "Client encryption should succeed");

  int dec_len = fio_tls13_server_decrypt(&server,
                                         decrypted,
                                         sizeof(decrypted),
                                         encrypted,
                                         (size_t)enc_len);
  FIO_ASSERT(dec_len == (int)strlen(test_msg),
             "Decryption length should match");
  FIO_ASSERT(FIO_MEMCMP(decrypted, test_msg, strlen(test_msg)) == 0,
             "Decrypted message should match");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
}

/* *****************************************************************************
Test: mTLS with Empty Client Certificate (Optional Mode)
***************************************************************************** */
FIO_SFUNC void test_mtls_empty_client_cert(void) {
  init_test_certificates();

  /* Initialize server with client cert optional */
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *server_certs[] = {server_certificate};
  size_t server_cert_lens[] = {server_certificate_len};
  fio_tls13_server_set_cert_chain(&server, server_certs, server_cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   server_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);
  fio_tls13_server_require_client_cert(&server, 1); /* Optional */

  /* Initialize client WITHOUT certificate */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");
  fio_tls13_client_skip_verification(&client, 1); /* Skip server cert verify */
  /* Note: NOT calling fio_tls13_client_set_cert() */

  /* Buffers for handshake messages */
  uint8_t client_out[4096];
  uint8_t server_out[4096];
  size_t server_out_len;

  /* Step 1: Client sends ClientHello */
  int ch_len = fio_tls13_client_start(&client, client_out, sizeof(client_out));
  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");

  /* Step 2: Server processes ClientHello */
  int consumed = fio_tls13_server_process(&server,
                                          client_out,
                                          (size_t)ch_len,
                                          server_out,
                                          sizeof(server_out),
                                          &server_out_len);
  FIO_ASSERT(consumed == ch_len, "Server should consume entire ClientHello");

  /* Step 3: Client processes server flight */
  size_t client_out_len = 0;
  size_t offset = 0;
  while (offset < server_out_len) {
    int proc = fio_tls13_client_process(&client,
                                        server_out + offset,
                                        server_out_len - offset,
                                        client_out,
                                        sizeof(client_out),
                                        &client_out_len);
    FIO_ASSERT(proc > 0, "Client should process server records (optional)");
    offset += (size_t)proc;
  }

  /* Client should have received CertificateRequest but has no cert */
  FIO_ASSERT(fio_tls13_client_cert_requested(&client) == 1,
             "Client should have received CertificateRequest");

  /* Client should send empty Certificate + Finished (no CertificateVerify) */
  FIO_ASSERT(client_out_len > 0, "Client should generate response");

  /* Step 4: Server processes client's empty Certificate + Finished */
  offset = 0;
  while (offset < client_out_len &&
         server.state != FIO_TLS13_SERVER_STATE_CONNECTED) {
    consumed = fio_tls13_server_process(&server,
                                        client_out + offset,
                                        client_out_len - offset,
                                        server_out,
                                        sizeof(server_out),
                                        &server_out_len);
    if (consumed <= 0)
      break;
    offset += (size_t)consumed;
  }

  /* Handshake should complete (optional mode allows empty cert) */
  FIO_ASSERT(fio_tls13_server_is_connected(&server),
             "Server should be connected (state=%s)",
             fio_tls13_server_state_name(&server));

  /* No client certificate should be received */
  FIO_ASSERT(fio_tls13_server_client_cert_received(&server) == 0,
             "Server should NOT have received client certificate");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
}

/* *****************************************************************************
Test: mTLS with Empty Client Certificate (Required Mode - Should Fail)
***************************************************************************** */
FIO_SFUNC void test_mtls_empty_client_cert_required(void) {
  init_test_certificates();

  /* Initialize server with client cert required */
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *server_certs[] = {server_certificate};
  size_t server_cert_lens[] = {server_certificate_len};
  fio_tls13_server_set_cert_chain(&server, server_certs, server_cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   server_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);
  fio_tls13_server_require_client_cert(&server, 2); /* Required */

  /* Initialize client WITHOUT certificate */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");
  fio_tls13_client_skip_verification(&client, 1); /* Skip server cert verify */

  /* Buffers for handshake messages */
  uint8_t client_out[4096];
  uint8_t server_out[4096];
  size_t server_out_len;

  /* Step 1: Client sends ClientHello */
  int ch_len = fio_tls13_client_start(&client, client_out, sizeof(client_out));
  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");

  /* Step 2: Server processes ClientHello */
  int consumed = fio_tls13_server_process(&server,
                                          client_out,
                                          (size_t)ch_len,
                                          server_out,
                                          sizeof(server_out),
                                          &server_out_len);
  FIO_ASSERT(consumed == ch_len, "Server should consume entire ClientHello");

  /* Step 3: Client processes server flight */
  size_t client_out_len = 0;
  size_t offset = 0;
  while (offset < server_out_len) {
    int proc = fio_tls13_client_process(&client,
                                        server_out + offset,
                                        server_out_len - offset,
                                        client_out,
                                        sizeof(client_out),
                                        &client_out_len);
    FIO_ASSERT(proc > 0, "Client should process server records (required)");
    offset += (size_t)proc;
  }

  /* Step 4: Server processes client's empty Certificate */
  offset = 0;
  int error_occurred = 0;
  while (offset < client_out_len) {
    consumed = fio_tls13_server_process(&server,
                                        client_out + offset,
                                        client_out_len - offset,
                                        server_out,
                                        sizeof(server_out),
                                        &server_out_len);
    if (consumed < 0) {
      error_occurred = 1;
      break;
    }
    if (consumed == 0)
      break;
    offset += (size_t)consumed;
  }

  /* Server should reject empty certificate when required */
  FIO_ASSERT(error_occurred || fio_tls13_server_is_error(&server),
             "Server should reject empty certificate when required");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
}

/* *****************************************************************************
Test: Server State Names Include New States
***************************************************************************** */
FIO_SFUNC void test_server_state_names(void) {
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  /* Test all state names */
  server.state = FIO_TLS13_SERVER_STATE_START;
  FIO_ASSERT(strcmp(fio_tls13_server_state_name(&server), "START") == 0,
             "START state name");

  server.state = FIO_TLS13_SERVER_STATE_WAIT_CLIENT_CERT;
  FIO_ASSERT(strcmp(fio_tls13_server_state_name(&server), "WAIT_CLIENT_CERT") ==
                 0,
             "WAIT_CLIENT_CERT state name");

  server.state = FIO_TLS13_SERVER_STATE_WAIT_CERT_VERIFY;
  FIO_ASSERT(strcmp(fio_tls13_server_state_name(&server), "WAIT_CERT_VERIFY") ==
                 0,
             "WAIT_CERT_VERIFY state name");

  server.state = FIO_TLS13_SERVER_STATE_WAIT_FINISHED;
  FIO_ASSERT(strcmp(fio_tls13_server_state_name(&server), "WAIT_FINISHED") == 0,
             "WAIT_FINISHED state name");

  server.state = FIO_TLS13_SERVER_STATE_CONNECTED;
  FIO_ASSERT(strcmp(fio_tls13_server_state_name(&server), "CONNECTED") == 0,
             "CONNECTED state name");

  fio_tls13_server_destroy(&server);
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  test_certificate_request_build_parse();
  test_server_require_client_cert();
  test_client_set_cert();
  test_server_state_names();
  test_mtls_handshake();
  test_mtls_empty_client_cert();
  test_mtls_empty_client_cert_required();
  return 0;
}
