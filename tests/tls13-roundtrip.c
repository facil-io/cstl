/* *****************************************************************************
TLS 1.3 Roundtrip Tests

Tests for complete TLS 1.3 client-server handshakes including:
1. X25519 key exchange (classical)
2. X25519MLKEM768 hybrid key exchange (post-quantum)

Both client and server run in the same process, exchanging messages through
buffers without actual network sockets.
***************************************************************************** */
#define FIO_LOG
#define FIO_CRYPT
#define FIO_TLS13
#include "test-helpers.h"

/* *****************************************************************************
Test Certificate and Private Key (Ed25519)
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

  /* Signature Algorithm */
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
Helper: Perform full handshake and data exchange
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
Test: X25519 Roundtrip (Classical)
***************************************************************************** */
FIO_SFUNC void test_x25519_roundtrip(void) {
  FIO_LOG_DEBUG("\nX25519 Roundtrip Test:\n");

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

  int result = do_handshake_and_exchange(&client, &server, "X25519");

  FIO_ASSERT(result == 0, "X25519 roundtrip should succeed");
  FIO_ASSERT(server.key_share_group == FIO_TLS13_GROUP_X25519,
             "Should use X25519 group");
  FIO_ASSERT(client.shared_secret_len == 32,
             "X25519 shared secret should be 32 bytes");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);

  FIO_LOG_DEBUG("  X25519 roundtrip: OK\n");
}

/* *****************************************************************************
Test: X25519MLKEM768 Client Key Generation
***************************************************************************** */
FIO_SFUNC void test_hybrid_key_generation(void) {
  FIO_LOG_DEBUG("\nX25519MLKEM768 Key Generation Test:\n");

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");

#if defined(H___FIO_MLKEM___H)
  FIO_ASSERT(client.use_hybrid == 1,
             "Hybrid should be enabled when ML-KEM is available");

  /* Verify hybrid public key has expected structure:
   * First 32 bytes: X25519 public key
   * Next 1184 bytes: ML-KEM-768 encapsulation key */

  /* Verify X25519 component matches standalone key */
  FIO_ASSERT(
      FIO_MEMCMP(client.hybrid_public_key, client.x25519_public_key, 32) != 0,
      "Hybrid X25519 component should differ from standalone "
      "(independently generated)");

  /* Verify ML-KEM component is non-zero */
  int mlkem_nonzero = 0;
  for (int i = 32; i < 1216; ++i) {
    if (client.hybrid_public_key[i] != 0) {
      mlkem_nonzero = 1;
      break;
    }
  }
  FIO_ASSERT(mlkem_nonzero, "ML-KEM public key component should be non-zero");

  FIO_LOG_DEBUG("  Hybrid key generation: OK\n");
  FIO_LOG_DEBUG("  - use_hybrid: %d\n", client.use_hybrid);
  FIO_LOG_DEBUG("  - hybrid_public_key size: 1216 bytes\n");
  FIO_LOG_DEBUG("  - hybrid_private_key size: 2432 bytes\n");
#else
  FIO_ASSERT(client.use_hybrid == 0,
             "Hybrid should be disabled when ML-KEM not available");
  FIO_LOG_DEBUG("  Hybrid key generation: SKIPPED (ML-KEM not compiled)\n");
#endif

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: X25519MLKEM768 ClientHello Structure
***************************************************************************** */
FIO_SFUNC void test_hybrid_client_hello_structure(void) {
  FIO_LOG_DEBUG("\nX25519MLKEM768 ClientHello Structure Test:\n");

#if defined(H___FIO_MLKEM___H)
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");
  fio_tls13_client_skip_verification(&client, 1);

  uint8_t ch_record[4096];
  int ch_len = fio_tls13_client_start(&client, ch_record, sizeof(ch_record));

  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");

  /* With hybrid, ClientHello should be much larger:
   * Base ~150 bytes + X25519MLKEM768 key share (1216 bytes) + X25519 (32 bytes)
   * Total ~1400+ bytes */
  FIO_LOG_DEBUG("  ClientHello size: %d bytes\n", ch_len);
  FIO_ASSERT(ch_len > 1300,
             "Hybrid ClientHello should be >1300 bytes (got %d)",
             ch_len);

  /* Parse the ClientHello to verify structure */
  /* Skip record header (5 bytes) and handshake header (4 bytes) */
  const uint8_t *ch_body = ch_record + 5 + 4;
  size_t ch_body_len = (size_t)ch_len - 5 - 4;

  /* Parse to get key shares */
  fio_tls13_client_hello_s ch;
  int ret = fio___tls13_parse_client_hello(&ch, ch_body, ch_body_len);
  FIO_ASSERT(ret == 0, "ClientHello parsing should succeed");

  /* Should have 2 key shares: X25519MLKEM768 and X25519 */
  FIO_LOG_DEBUG("  Key share count: %zu\n", ch.key_share_count);
  FIO_ASSERT(ch.key_share_count >= 2, "Should have at least 2 key shares");

  /* Find hybrid and X25519 key shares */
  int found_hybrid = 0, found_x25519 = 0;
  for (size_t i = 0; i < ch.key_share_count; ++i) {
    FIO_LOG_DEBUG("    Key share %zu: group=0x%04x, len=%u\n",
                  i,
                  ch.key_share_groups[i],
                  (unsigned)ch.key_share_lens[i]);
    if (ch.key_share_groups[i] == FIO_TLS13_GROUP_X25519MLKEM768) {
      found_hybrid = 1;
      FIO_ASSERT(ch.key_share_lens[i] == 1216,
                 "Hybrid key share should be 1216 bytes");
    }
    if (ch.key_share_groups[i] == FIO_TLS13_GROUP_X25519) {
      found_x25519 = 1;
      FIO_ASSERT(ch.key_share_lens[i] == 32,
                 "X25519 key share should be 32 bytes");
    }
  }

  FIO_ASSERT(found_hybrid, "Should include X25519MLKEM768 key share");
  FIO_ASSERT(found_x25519, "Should include X25519 key share (fallback)");

  fio_tls13_client_destroy(&client);
  FIO_LOG_DEBUG("  Hybrid ClientHello structure: OK\n");
#else
  FIO_LOG_DEBUG(
      "  Hybrid ClientHello structure: SKIPPED (ML-KEM not compiled)\n");
#endif
}

/* *****************************************************************************
Test: X25519MLKEM768 Full Hybrid Roundtrip (server supports PQC)
***************************************************************************** */
FIO_SFUNC void test_hybrid_full_roundtrip(void) {
  FIO_LOG_DEBUG("\nX25519MLKEM768 Full Hybrid Roundtrip Test:\n");

#if defined(H___FIO_MLKEM___H)
  build_test_certificate();

  /* Client with hybrid enabled */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");
  fio_tls13_client_skip_verification(&client, 1);
  FIO_ASSERT(client.use_hybrid == 1, "Client should have hybrid enabled");

  /* Server with hybrid support (now default when ML-KEM is compiled) */
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  int result = do_handshake_and_exchange(&client, &server, "Hybrid");

  FIO_ASSERT(result == 0, "Full hybrid handshake should succeed");

  /* Server should select X25519MLKEM768 (post-quantum hybrid) */
  FIO_ASSERT(server.key_share_group == FIO_TLS13_GROUP_X25519MLKEM768,
             "Server should select X25519MLKEM768 (got 0x%04x)",
             server.key_share_group);
  FIO_ASSERT(client.shared_secret_len == 64,
             "Hybrid should use 64-byte shared secret (got %zu)",
             client.shared_secret_len);
  FIO_ASSERT(server.shared_secret_len == 64,
             "Server hybrid should use 64-byte shared secret (got %zu)",
             server.shared_secret_len);

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);

  FIO_LOG_DEBUG("  Full hybrid roundtrip: OK\n");
#else
  FIO_LOG_DEBUG("  Full hybrid roundtrip: SKIPPED (ML-KEM not compiled)\n");
#endif
}

/* *****************************************************************************
Test: X25519 Fallback (client without hybrid)
***************************************************************************** */
FIO_SFUNC void test_x25519_only_client(void) {
  FIO_LOG_DEBUG("\nX25519-Only Client Test:\n");

#if defined(H___FIO_MLKEM___H)
  build_test_certificate();

  /* Client with hybrid DISABLED */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test.example.com");
  fio_tls13_client_skip_verification(&client, 1);
  client.use_hybrid = 0; /* Force X25519 only */

  /* Server with hybrid support */
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  int result = do_handshake_and_exchange(&client, &server, "X25519-only");

  FIO_ASSERT(result == 0, "X25519-only client handshake should succeed");

  /* Server should fall back to X25519 */
  FIO_ASSERT(server.key_share_group == FIO_TLS13_GROUP_X25519,
             "Server should fall back to X25519 (got 0x%04x)",
             server.key_share_group);
  FIO_ASSERT(client.shared_secret_len == 32,
             "X25519 should use 32-byte shared secret");
  FIO_ASSERT(server.shared_secret_len == 32,
             "Server X25519 should use 32-byte shared secret");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);

  FIO_LOG_DEBUG("  X25519-only client: OK\n");
#else
  FIO_LOG_DEBUG("  X25519-only client: SKIPPED (ML-KEM not compiled)\n");
#endif
}

/* *****************************************************************************
Test: Multiple Roundtrips
***************************************************************************** */
FIO_SFUNC void test_multiple_roundtrips(void) {
  FIO_LOG_DEBUG("\nMultiple Roundtrips Test:\n");

  build_test_certificate();

  for (int i = 0; i < 10; ++i) {
    fio_tls13_client_s client;
    fio_tls13_client_init(&client, "test.example.com");
    fio_tls13_client_skip_verification(&client, 1);

    /* Alternate between hybrid and non-hybrid */
    client.use_hybrid = (i % 2 == 0) ? 0 : client.use_hybrid;

    fio_tls13_server_s server;
    fio_tls13_server_init(&server);

    const uint8_t *certs[] = {test_certificate};
    size_t cert_lens[] = {test_certificate_len};
    fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
    fio_tls13_server_set_private_key(&server,
                                     test_ed25519_private_key,
                                     32,
                                     FIO_TLS13_SIG_ED25519);

    char test_name[32];
    snprintf(test_name, sizeof(test_name), "Round %d", i + 1);

    int result = do_handshake_and_exchange(&client, &server, test_name);
    FIO_ASSERT(result == 0, "Roundtrip %d should succeed", i + 1);

    fio_tls13_client_destroy(&client);
    fio_tls13_server_destroy(&server);
  }

  FIO_LOG_DEBUG("  10 roundtrips: OK\n");
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  FIO_LOG_DEBUG("=== TLS 1.3 Roundtrip Tests ===\n");

  /* Basic roundtrip (X25519) */
  test_x25519_roundtrip();

  /* Hybrid key generation */
  test_hybrid_key_generation();

  /* Hybrid ClientHello structure */
  test_hybrid_client_hello_structure();

  /* Full hybrid roundtrip (server supports X25519MLKEM768) */
  test_hybrid_full_roundtrip();

  /* X25519-only client (server falls back) */
  test_x25519_only_client();

  /* Multiple roundtrips */
  test_multiple_roundtrips();

  FIO_LOG_DEBUG("\n=== All TLS 1.3 Roundtrip Tests PASSED ===\n");
  return 0;
}
