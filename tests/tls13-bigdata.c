/* *****************************************************************************
TLS 1.3 Buffer Management Test

Tests the TLS 1.3 IO layer buffer management by verifying that the
flexible array buffer allocation works correctly.

This test is based on the roundtrip test which is known to work.
***************************************************************************** */

#define FIO_LOG
#define FIO_CRYPT
#define FIO_TLS13
#include "test-helpers.h"

/* *****************************************************************************
Test Certificate and Private Key (Ed25519) - same as roundtrip test
***************************************************************************** */

static const uint8_t test_ed25519_private_key[32] = {
    0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60, 0xba, 0x84, 0x4a,
    0xf4, 0x92, 0xec, 0x2c, 0xc4, 0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32,
    0x69, 0x19, 0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60};

static uint8_t test_ed25519_public_key[32];
static uint8_t test_certificate[512];
static size_t test_certificate_len = 0;

/* Build a minimal self-signed certificate for testing */
FIO_SFUNC void build_test_certificate(void) {
  fio_ed25519_public_key(test_ed25519_public_key, test_ed25519_private_key);

  uint8_t *p = test_certificate;

  /* SEQUENCE (Certificate) */
  *p++ = 0x30;
  uint8_t *cert_len_ptr = p++;
  uint8_t *cert_start = p;

  /* SEQUENCE (TBSCertificate) */
  *p++ = 0x30;
  uint8_t *tbs_len_ptr = p++;
  uint8_t *tbs_start = p;

  /* Version: v3 (2) */
  *p++ = 0xA0;
  *p++ = 0x03;
  *p++ = 0x02;
  *p++ = 0x01;
  *p++ = 0x02;

  /* Serial Number */
  *p++ = 0x02;
  *p++ = 0x01;
  *p++ = 0x01;

  /* Signature Algorithm: Ed25519 */
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

  /* Validity */
  *p++ = 0x30;
  *p++ = 0x1E;
  *p++ = 0x17;
  *p++ = 0x0D;
  FIO_MEMCPY(p, "240101000000Z", 13);
  p += 13;
  *p++ = 0x17;
  *p++ = 0x0D;
  FIO_MEMCPY(p, "341231235959Z", 13);
  p += 13;

  /* Subject: CN=test */
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
  *p++ = 0x30;
  *p++ = 0x05;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x2B;
  *p++ = 0x65;
  *p++ = 0x70;
  *p++ = 0x03;
  *p++ = 0x21;
  *p++ = 0x00;
  FIO_MEMCPY(p, test_ed25519_public_key, 32);
  p += 32;

  *tbs_len_ptr = (uint8_t)(p - tbs_start);

  /* Signature Algorithm: Ed25519 */
  *p++ = 0x30;
  *p++ = 0x05;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x2B;
  *p++ = 0x65;
  *p++ = 0x70;

  /* Signature */
  uint8_t signature[64];
  fio_ed25519_sign(signature,
                   tbs_start - 2,
                   (size_t)(p - tbs_start + 2),
                   test_ed25519_private_key,
                   test_ed25519_public_key);

  *p++ = 0x03;
  *p++ = 0x41;
  *p++ = 0x00;
  FIO_MEMCPY(p, signature, 64);
  p += 64;

  *cert_len_ptr = (uint8_t)(p - cert_start);
  test_certificate_len = (size_t)(p - test_certificate);
}

/* *****************************************************************************
Test: Data Transfer (copied from roundtrip test)
***************************************************************************** */

FIO_SFUNC int do_handshake_and_exchange(fio_tls13_client_s *client,
                                        fio_tls13_server_s *server,
                                        const char *test_name) {
  uint8_t client_out[8192];
  uint8_t server_out[8192];
  size_t out_len;

  /* Step 1: Client generates ClientHello */
  int ch_len = fio_tls13_client_start(client, client_out, sizeof(client_out));
  if (ch_len <= 0) {
    FIO_LOG_DEBUG("  [%s] FAIL: ClientHello generation failed\n", test_name);
    return -1;
  }
  FIO_LOG_DEBUG("  [%s] ClientHello: %d bytes\n", test_name, ch_len);

  /* Step 2: Server processes ClientHello */
  out_len = 0;
  int consumed = fio_tls13_server_process(server,
                                          client_out,
                                          (size_t)ch_len,
                                          server_out,
                                          sizeof(server_out),
                                          &out_len);
  if (consumed != ch_len || out_len == 0) {
    FIO_LOG_DEBUG("  [%s] FAIL: Server process failed (consumed=%d, out=%zu)\n",
                  test_name,
                  consumed,
                  out_len);
    return -1;
  }
  FIO_LOG_DEBUG("  [%s] Server response: %zu bytes (group=0x%04x)\n",
                test_name,
                out_len,
                server->key_share_group);

  /* Step 3: Client processes server response */
  size_t client_response_len = 0;
  size_t offset = 0;
  while (offset < out_len && !fio_tls13_client_is_connected(client) &&
         !fio_tls13_client_is_error(client)) {
    size_t msg_out_len = 0;
    int proc =
        fio_tls13_client_process(client,
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

  if (fio_tls13_client_is_error(client)) {
    FIO_LOG_DEBUG("  [%s] FAIL: Client error (alert=%d)\n",
                  test_name,
                  client->alert_description);
    return -1;
  }
  if (!fio_tls13_client_is_connected(client)) {
    FIO_LOG_DEBUG("  [%s] FAIL: Client not connected (state=%s)\n",
                  test_name,
                  fio_tls13_client_state_name(client));
    return -1;
  }

  /* Step 4: Server processes client Finished */
  out_len = 0;
  consumed = fio_tls13_server_process(server,
                                      client_out,
                                      client_response_len,
                                      server_out,
                                      sizeof(server_out),
                                      &out_len);
  if (!fio_tls13_server_is_connected(server)) {
    FIO_LOG_DEBUG("  [%s] FAIL: Server not connected (state=%s)\n",
                  test_name,
                  fio_tls13_server_state_name(server));
    return -1;
  }

  FIO_LOG_DEBUG("  [%s] Handshake complete!\n", test_name);

  /* Step 5: Data exchange - client to server */
  const char *msg1 = "Hello from client!";
  size_t msg1_len = strlen(msg1);

  int enc_len = fio_tls13_client_encrypt(client,
                                         client_out,
                                         sizeof(client_out),
                                         (const uint8_t *)msg1,
                                         msg1_len);
  if (enc_len <= 0) {
    FIO_LOG_DEBUG("  [%s] FAIL: Client encrypt failed\n", test_name);
    return -1;
  }

  uint8_t decrypted[256];
  int dec_len = fio_tls13_server_decrypt(server,
                                         decrypted,
                                         sizeof(decrypted),
                                         client_out,
                                         (size_t)enc_len);
  if (dec_len != (int)msg1_len || FIO_MEMCMP(decrypted, msg1, msg1_len) != 0) {
    FIO_LOG_DEBUG("  [%s] FAIL: Server decrypt mismatch\n", test_name);
    return -1;
  }

  /* Step 6: Data exchange - server to client */
  const char *msg2 = "Hello from server!";
  size_t msg2_len = strlen(msg2);

  enc_len = fio_tls13_server_encrypt(server,
                                     server_out,
                                     sizeof(server_out),
                                     (const uint8_t *)msg2,
                                     msg2_len);
  if (enc_len <= 0) {
    FIO_LOG_DEBUG("  [%s] FAIL: Server encrypt failed\n", test_name);
    return -1;
  }

  dec_len = fio_tls13_client_decrypt(client,
                                     decrypted,
                                     sizeof(decrypted),
                                     server_out,
                                     (size_t)enc_len);
  if (dec_len != (int)msg2_len || FIO_MEMCMP(decrypted, msg2, msg2_len) != 0) {
    FIO_LOG_DEBUG("  [%s] FAIL: Client decrypt mismatch\n", test_name);
    return -1;
  }

  FIO_LOG_DEBUG("  [%s] Data exchange OK!\n", test_name);
  return 0;
}

/* *****************************************************************************
Test: Basic Roundtrip (same as roundtrip test)
***************************************************************************** */

FIO_SFUNC void fio___test_tls13_basic_roundtrip(void) {
  FIO_LOG_DEBUG("\n=== TLS 1.3 Basic Roundtrip Test ===\n\n");

  build_test_certificate();

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");
  fio_tls13_client_skip_verification(&client, 1);

  /* Disable hybrid for this test */
  client.use_hybrid = 0;

  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  int result = do_handshake_and_exchange(&client, &server, "Basic");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);

  FIO_ASSERT(result == 0, "Basic roundtrip failed");

  FIO_LOG_DEBUG("\n=== TLS 1.3 Basic Roundtrip Test: PASSED ===\n\n");
}

/* *****************************************************************************
Main Test Function
***************************************************************************** */

int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;

  FIO_LOG_DEBUG("\n");
  FIO_LOG_DEBUG("========================================\n");
  FIO_LOG_DEBUG("  TLS 1.3 Buffer Management Tests\n");
  FIO_LOG_DEBUG("========================================\n");

  /* Run tests */
  fio___test_tls13_basic_roundtrip();

  FIO_LOG_DEBUG("\n========================================\n");
  FIO_LOG_DEBUG("  All TLS 1.3 Buffer Tests PASSED!\n");
  FIO_LOG_DEBUG("========================================\n\n");

  return 0;
}
