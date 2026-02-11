/* *****************************************************************************
TLS 1.3 Client Handshake State Machine Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPT
#define FIO_TLS13
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Helper Functions
***************************************************************************** */

/* Print hex for debugging */
FIO_SFUNC void print_hex(const char *label, const uint8_t *data, size_t len) {
  fprintf(stderr, "%s: ", label);
  for (size_t i = 0; i < len; ++i)
    fprintf(stderr, "%02x", data[i]);
  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test: Client Initialization
***************************************************************************** */
FIO_SFUNC void test_client_init(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Verify initial state */
  FIO_ASSERT(client.state == FIO_TLS13_STATE_START,
             "Initial state should be START");
  FIO_ASSERT(client.server_name != NULL, "Server name should be set");
  FIO_ASSERT(strcmp(client.server_name, "example.com") == 0,
             "Server name should match");

  /* Verify random was generated (not all zeros) */
  uint8_t zeros[32] = {0};
  FIO_ASSERT(FIO_MEMCMP(client.client_random, zeros, 32) != 0,
             "Client random should not be all zeros");

  /* Verify X25519 keypair was generated */
  FIO_ASSERT(FIO_MEMCMP(client.x25519_private_key, zeros, 32) != 0,
             "X25519 private key should not be all zeros");
  FIO_ASSERT(FIO_MEMCMP(client.x25519_public_key, zeros, 32) != 0,
             "X25519 public key should not be all zeros");

  /* Test with NULL server name */
  fio_tls13_client_s client2;
  fio_tls13_client_init(&client2, NULL);
  FIO_ASSERT(client2.server_name == NULL, "Server name should be NULL");
  FIO_ASSERT(client2.state == FIO_TLS13_STATE_START,
             "Initial state should be START");

  fio_tls13_client_destroy(&client);
  fio_tls13_client_destroy(&client2);
}

/* *****************************************************************************
Test: Client Destroy (Zeroes Secrets)
***************************************************************************** */
FIO_SFUNC void test_client_destroy(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Fill some secrets with non-zero data */
  FIO_MEMSET(client.shared_secret, 0xAA, 32);
  FIO_MEMSET(client.handshake_secret, 0xBB, 48);
  FIO_MEMSET(client.master_secret, 0xCC, 48);

  fio_tls13_client_destroy(&client);

  /* Verify secrets are zeroed */
  uint8_t zeros32[32] = {0};
  uint8_t zeros48[48] = {0};
  FIO_ASSERT(FIO_MEMCMP(client.shared_secret, zeros32, 32) == 0,
             "Shared secret should be zeroed");
  FIO_ASSERT(FIO_MEMCMP(client.handshake_secret, zeros48, 48) == 0,
             "Handshake secret should be zeroed");
  FIO_ASSERT(FIO_MEMCMP(client.master_secret, zeros48, 48) == 0,
             "Master secret should be zeroed");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_START,
             "State should be reset to START");
}

/* *****************************************************************************
Test: ClientHello Generation
***************************************************************************** */
FIO_SFUNC void test_client_hello_generation(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Buffer must be large enough for hybrid ClientHello (~1400 bytes) */
  uint8_t out[2048];
  int len = fio_tls13_client_start(&client, out, sizeof(out));

  FIO_ASSERT(len > 0, "ClientHello generation should succeed");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_WAIT_SH,
             "State should be WAIT_SH after start");

  /* Verify record header */
  FIO_ASSERT(out[0] == FIO_TLS13_CONTENT_HANDSHAKE,
             "Content type should be handshake (22)");
  FIO_ASSERT(out[1] == 0x03 && out[2] == 0x03,
             "Legacy version should be 0x0303");

  /* Verify handshake header */
  FIO_ASSERT(out[5] == FIO_TLS13_HS_CLIENT_HELLO,
             "Handshake type should be ClientHello (1)");

  /* Verify length consistency */
  uint16_t record_len = ((uint16_t)out[3] << 8) | out[4];
  FIO_ASSERT(len == (int)(5 + record_len),
             "Total length should match record header");

  /* Test with insufficient buffer */
  uint8_t small_buf[10];
  fio_tls13_client_s client2;
  fio_tls13_client_init(&client2, "example.com");
  int len2 = fio_tls13_client_start(&client2, small_buf, sizeof(small_buf));
  FIO_ASSERT(len2 == -1, "Should fail with insufficient buffer");

  /* Test calling start twice */
  int len3 = fio_tls13_client_start(&client, out, sizeof(out));
  FIO_ASSERT(len3 == -1, "Should fail when not in START state");

  fio_tls13_client_destroy(&client);
  fio_tls13_client_destroy(&client2);
}

/* *****************************************************************************
Test: State Name Helper
***************************************************************************** */
FIO_SFUNC void test_state_names(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  FIO_ASSERT(strcmp(fio_tls13_client_state_name(&client), "START") == 0,
             "START state name");

  client.state = FIO_TLS13_STATE_WAIT_SH;
  FIO_ASSERT(strcmp(fio_tls13_client_state_name(&client), "WAIT_SH") == 0,
             "WAIT_SH state name");

  client.state = FIO_TLS13_STATE_WAIT_EE;
  FIO_ASSERT(strcmp(fio_tls13_client_state_name(&client), "WAIT_EE") == 0,
             "WAIT_EE state name");

  client.state = FIO_TLS13_STATE_WAIT_CERT_CR;
  FIO_ASSERT(strcmp(fio_tls13_client_state_name(&client), "WAIT_CERT_CR") == 0,
             "WAIT_CERT_CR state name");

  client.state = FIO_TLS13_STATE_WAIT_CERT;
  FIO_ASSERT(strcmp(fio_tls13_client_state_name(&client), "WAIT_CERT") == 0,
             "WAIT_CERT state name");

  client.state = FIO_TLS13_STATE_WAIT_CV;
  FIO_ASSERT(strcmp(fio_tls13_client_state_name(&client), "WAIT_CV") == 0,
             "WAIT_CV state name");

  client.state = FIO_TLS13_STATE_WAIT_FINISHED;
  FIO_ASSERT(strcmp(fio_tls13_client_state_name(&client), "WAIT_FINISHED") == 0,
             "WAIT_FINISHED state name");

  client.state = FIO_TLS13_STATE_CONNECTED;
  FIO_ASSERT(strcmp(fio_tls13_client_state_name(&client), "CONNECTED") == 0,
             "CONNECTED state name");

  client.state = FIO_TLS13_STATE_ERROR;
  FIO_ASSERT(strcmp(fio_tls13_client_state_name(&client), "ERROR") == 0,
             "ERROR state name");

  /* Test NULL client */
  FIO_ASSERT(strcmp(fio_tls13_client_state_name(NULL), "NULL") == 0,
             "NULL client state name");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Status Query Functions
***************************************************************************** */
FIO_SFUNC void test_status_queries(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  /* Initial state */
  FIO_ASSERT(!fio_tls13_client_is_connected(&client),
             "Should not be connected initially");
  FIO_ASSERT(!fio_tls13_client_is_error(&client),
             "Should not be in error initially");

  /* Connected state */
  client.state = FIO_TLS13_STATE_CONNECTED;
  FIO_ASSERT(fio_tls13_client_is_connected(&client),
             "Should be connected in CONNECTED state");
  FIO_ASSERT(!fio_tls13_client_is_error(&client),
             "Should not be in error in CONNECTED state");

  /* Error state */
  client.state = FIO_TLS13_STATE_ERROR;
  FIO_ASSERT(!fio_tls13_client_is_connected(&client),
             "Should not be connected in ERROR state");
  FIO_ASSERT(fio_tls13_client_is_error(&client),
             "Should be in error in ERROR state");

  /* NULL client */
  FIO_ASSERT(!fio_tls13_client_is_connected(NULL),
             "NULL client should not be connected");
  FIO_ASSERT(!fio_tls13_client_is_error(NULL),
             "NULL client should not be in error");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Transcript Hash Computation
***************************************************************************** */
FIO_SFUNC void test_transcript_hash(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  /* Test SHA-256 transcript */
  client.use_sha384 = 0;
  uint8_t test_data[] = "Hello, TLS 1.3!";
  fio___tls13_transcript_update(&client, test_data, sizeof(test_data) - 1);

  uint8_t hash1[48];
  fio___tls13_transcript_hash(&client, hash1);

  /* Verify hash is not zero */
  uint8_t zeros[32] = {0};
  FIO_ASSERT(FIO_MEMCMP(hash1, zeros, 32) != 0,
             "SHA-256 transcript hash should not be zero");

  /* Verify hash is deterministic */
  uint8_t hash2[48];
  fio___tls13_transcript_hash(&client, hash2);
  FIO_ASSERT(FIO_MEMCMP(hash1, hash2, 32) == 0,
             "Transcript hash should be deterministic");

  /* Test SHA-384 transcript */
  fio_tls13_client_s client2;
  fio_tls13_client_init(&client2, NULL);
  client2.use_sha384 = 1;
  fio___tls13_transcript_update(&client2, test_data, sizeof(test_data) - 1);

  uint8_t hash3[48];
  fio___tls13_transcript_hash(&client2, hash3);

  uint8_t zeros48[48] = {0};
  FIO_ASSERT(FIO_MEMCMP(hash3, zeros48, 48) != 0,
             "SHA-384 transcript hash should not be zero");

  /* SHA-256 and SHA-384 hashes should be different */
  FIO_ASSERT(FIO_MEMCMP(hash1, hash3, 32) != 0,
             "SHA-256 and SHA-384 hashes should differ");

  fio_tls13_client_destroy(&client);
  fio_tls13_client_destroy(&client2);
}

/* *****************************************************************************
Test: Key Derivation with Mock Data
***************************************************************************** */
FIO_SFUNC void test_key_derivation(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  /* Set up mock data */
  client.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  client.use_sha384 = 0;

  /* Generate a mock shared secret */
  fio_rand_bytes(client.shared_secret, 32);

  /* Add some data to transcript */
  uint8_t mock_ch[] = "Mock ClientHello data for testing";
  uint8_t mock_sh[] = "Mock ServerHello data for testing";
  fio___tls13_transcript_update(&client, mock_ch, sizeof(mock_ch) - 1);
  fio___tls13_transcript_update(&client, mock_sh, sizeof(mock_sh) - 1);

  /* Derive handshake keys */
  int ret = fio___tls13_derive_handshake_keys(&client);
  FIO_ASSERT(ret == 0, "Handshake key derivation should succeed");

  /* Verify secrets are not zero */
  uint8_t zeros48[48] = {0};
  FIO_ASSERT(FIO_MEMCMP(client.early_secret, zeros48, 32) != 0,
             "Early secret should not be zero");
  FIO_ASSERT(FIO_MEMCMP(client.handshake_secret, zeros48, 32) != 0,
             "Handshake secret should not be zero");
  FIO_ASSERT(FIO_MEMCMP(client.client_handshake_traffic_secret, zeros48, 32) !=
                 0,
             "Client HS traffic secret should not be zero");
  FIO_ASSERT(FIO_MEMCMP(client.server_handshake_traffic_secret, zeros48, 32) !=
                 0,
             "Server HS traffic secret should not be zero");

  /* Verify keys are initialized */
  FIO_ASSERT(client.client_handshake_keys.key_len == 16,
             "Client HS key length should be 16");
  FIO_ASSERT(client.server_handshake_keys.key_len == 16,
             "Server HS key length should be 16");

  /* Derive application keys */
  ret = fio___tls13_derive_app_keys(&client);
  FIO_ASSERT(ret == 0, "Application key derivation should succeed");

  FIO_ASSERT(FIO_MEMCMP(client.master_secret, zeros48, 32) != 0,
             "Master secret should not be zero");
  FIO_ASSERT(FIO_MEMCMP(client.client_app_traffic_secret, zeros48, 32) != 0,
             "Client app traffic secret should not be zero");
  FIO_ASSERT(FIO_MEMCMP(client.server_app_traffic_secret, zeros48, 32) != 0,
             "Server app traffic secret should not be zero");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Finished Message Building and Verification
***************************************************************************** */
FIO_SFUNC void test_finished_message(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  /* Set up mock data */
  client.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  client.use_sha384 = 0;
  fio_rand_bytes(client.shared_secret, 32);

  /* Add transcript data */
  uint8_t mock_data[] = "Mock handshake data";
  fio___tls13_transcript_update(&client, mock_data, sizeof(mock_data) - 1);

  /* Derive handshake keys */
  fio___tls13_derive_handshake_keys(&client);

  /* Build client Finished */
  uint8_t finished_msg[64];
  int len = fio___tls13_build_client_finished(&client,
                                              finished_msg,
                                              sizeof(finished_msg));
  FIO_ASSERT(len > 0, "Client Finished building should succeed");
  FIO_ASSERT(len == 4 + 32, "Client Finished should be 36 bytes for SHA-256");

  /* Verify handshake header */
  FIO_ASSERT(finished_msg[0] == FIO_TLS13_HS_FINISHED,
             "Handshake type should be Finished (20)");

  /* Test with insufficient buffer */
  uint8_t small_buf[10];
  int len2 =
      fio___tls13_build_client_finished(&client, small_buf, sizeof(small_buf));
  FIO_ASSERT(len2 == -1, "Should fail with insufficient buffer");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Server Finished Verification
***************************************************************************** */
FIO_SFUNC void test_server_finished_verification(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  /* Set up mock data */
  client.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  client.use_sha384 = 0;
  fio_rand_bytes(client.shared_secret, 32);

  /* Add transcript data */
  uint8_t mock_data[] = "Mock handshake data";
  fio___tls13_transcript_update(&client, mock_data, sizeof(mock_data) - 1);

  /* Derive handshake keys */
  fio___tls13_derive_handshake_keys(&client);

  /* Compute expected server Finished verify_data */
  uint8_t transcript_hash[48];
  fio___tls13_transcript_hash(&client, transcript_hash);

  uint8_t finished_key[48];
  fio_tls13_derive_finished_key(finished_key,
                                client.server_handshake_traffic_secret,
                                0);

  uint8_t expected_verify_data[48];
  fio_tls13_compute_finished(expected_verify_data,
                             finished_key,
                             transcript_hash,
                             0);

  /* Verify with correct data */
  int ret =
      fio___tls13_verify_server_finished(&client, expected_verify_data, 32);
  FIO_ASSERT(ret == 0, "Verification should succeed with correct data");

  /* Verify with wrong data */
  expected_verify_data[0] ^= 0xFF;
  ret = fio___tls13_verify_server_finished(&client, expected_verify_data, 32);
  FIO_ASSERT(ret != 0, "Verification should fail with wrong data");

  /* Verify with wrong length */
  ret = fio___tls13_verify_server_finished(&client, expected_verify_data, 16);
  FIO_ASSERT(ret != 0, "Verification should fail with wrong length");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Application Data Encryption/Decryption
***************************************************************************** */
FIO_SFUNC void test_app_data_encryption(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  /* Set up mock connected state */
  client.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  client.use_sha384 = 0;
  fio_rand_bytes(client.shared_secret, 32);

  /* Derive keys */
  fio___tls13_derive_handshake_keys(&client);
  fio___tls13_derive_app_keys(&client);
  client.state = FIO_TLS13_STATE_CONNECTED;

  /* Test encryption */
  uint8_t plaintext[] = "Hello, TLS 1.3 application data!";
  uint8_t ciphertext[256];
  int enc_len = fio_tls13_client_encrypt(&client,
                                         ciphertext,
                                         sizeof(ciphertext),
                                         plaintext,
                                         sizeof(plaintext) - 1);
  FIO_ASSERT(enc_len > 0, "Encryption should succeed");
  FIO_ASSERT((size_t)enc_len > sizeof(plaintext) - 1,
             "Ciphertext should be larger than plaintext");

  /* Test decryption (need to use server keys for round-trip) */
  /* For this test, we'll verify the encryption produces valid output */
  FIO_ASSERT(ciphertext[0] == FIO_TLS13_CONTENT_APPLICATION_DATA,
             "Outer content type should be application_data");

  /* Test encryption when not connected */
  fio_tls13_client_s client2;
  fio_tls13_client_init(&client2, NULL);
  int enc_len2 = fio_tls13_client_encrypt(&client2,
                                          ciphertext,
                                          sizeof(ciphertext),
                                          plaintext,
                                          sizeof(plaintext) - 1);
  FIO_ASSERT(enc_len2 == -1, "Encryption should fail when not connected");

  fio_tls13_client_destroy(&client);
  fio_tls13_client_destroy(&client2);
}

/* *****************************************************************************
Test: Cipher Suite Selection
***************************************************************************** */
FIO_SFUNC void test_cipher_suite_helpers(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  /* Test AES-128-GCM-SHA256 */
  client.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  FIO_ASSERT(fio___tls13_hash_len(&client) == 32,
             "AES-128-GCM hash length should be 32");
  FIO_ASSERT(fio___tls13_key_len(&client) == 16,
             "AES-128-GCM key length should be 16");
  FIO_ASSERT(fio___tls13_cipher_type(&client) == FIO_TLS13_CIPHER_AES_128_GCM,
             "AES-128-GCM cipher type");

  /* Test AES-256-GCM-SHA384 */
  client.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384;
  client.use_sha384 = 1;
  FIO_ASSERT(fio___tls13_hash_len(&client) == 48,
             "AES-256-GCM hash length should be 48");
  FIO_ASSERT(fio___tls13_key_len(&client) == 32,
             "AES-256-GCM key length should be 32");
  FIO_ASSERT(fio___tls13_cipher_type(&client) == FIO_TLS13_CIPHER_AES_256_GCM,
             "AES-256-GCM cipher type");

  /* Test ChaCha20-Poly1305-SHA256 */
  client.cipher_suite = FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256;
  client.use_sha384 = 0;
  FIO_ASSERT(fio___tls13_hash_len(&client) == 32,
             "ChaCha20 hash length should be 32");
  FIO_ASSERT(fio___tls13_key_len(&client) == 32,
             "ChaCha20 key length should be 32");
  FIO_ASSERT(fio___tls13_cipher_type(&client) ==
                 FIO_TLS13_CIPHER_CHACHA20_POLY1305,
             "ChaCha20 cipher type");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Error State Handling
***************************************************************************** */
FIO_SFUNC void test_error_handling(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  /* Set error state */
  fio___tls13_set_error(&client,
                        FIO_TLS13_ALERT_LEVEL_FATAL,
                        FIO_TLS13_ALERT_HANDSHAKE_FAILURE);

  FIO_ASSERT(client.state == FIO_TLS13_STATE_ERROR, "State should be ERROR");
  FIO_ASSERT(client.alert_level == FIO_TLS13_ALERT_LEVEL_FATAL,
             "Alert level should be FATAL");
  FIO_ASSERT(client.alert_description == FIO_TLS13_ALERT_HANDSHAKE_FAILURE,
             "Alert description should be HANDSHAKE_FAILURE");
  FIO_ASSERT(fio_tls13_client_is_error(&client), "Should be in error state");

  /* Verify operations fail in error state */
  uint8_t out[256];
  size_t out_len;
  uint8_t dummy_in[10] = {0};
  int ret = fio_tls13_client_process(&client,
                                     dummy_in,
                                     sizeof(dummy_in),
                                     out,
                                     sizeof(out),
                                     &out_len);
  FIO_ASSERT(ret == -1, "Process should fail in error state");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Mock Handshake Flow (State Transitions)
***************************************************************************** */
FIO_SFUNC void test_mock_handshake_flow(void) {
  /* This test verifies state transitions with mock server responses */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Step 1: Generate ClientHello (buffer must hold hybrid ~1400 bytes) */
  uint8_t ch_out[2048];
  int ch_len = fio_tls13_client_start(&client, ch_out, sizeof(ch_out));
  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_WAIT_SH,
             "State should be WAIT_SH");

  /* Note: Full handshake testing requires a mock server implementation
   * or integration with a real TLS server. The following tests verify
   * that the state machine rejects invalid inputs correctly. */

  /* Test: Invalid record type in WAIT_SH state */
  uint8_t invalid_record[] = {
      0x17,
      0x03,
      0x03,
      0x00,
      0x05, /* application_data record header */
      0x01,
      0x02,
      0x03,
      0x04,
      0x05 /* dummy payload */
  };
  uint8_t out[256];
  size_t out_len;
  int ret = fio_tls13_client_process(&client,
                                     invalid_record,
                                     sizeof(invalid_record),
                                     out,
                                     sizeof(out),
                                     &out_len);
  FIO_ASSERT(ret == -1, "Should reject application_data in WAIT_SH state");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_ERROR, "Should be in error state");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Record Header Writing
***************************************************************************** */
FIO_SFUNC void test_record_header_writing(void) {
  uint8_t header[5];

  /* Test handshake record */
  fio___tls13_write_record_header(header, FIO_TLS13_CONTENT_HANDSHAKE, 0x1234);
  FIO_ASSERT(header[0] == 22, "Content type should be handshake (22)");
  FIO_ASSERT(header[1] == 0x03, "Major version should be 0x03");
  FIO_ASSERT(header[2] == 0x03, "Minor version should be 0x03");
  FIO_ASSERT(header[3] == 0x12, "Length high byte");
  FIO_ASSERT(header[4] == 0x34, "Length low byte");

  /* Test application data record */
  fio___tls13_write_record_header(header,
                                  FIO_TLS13_CONTENT_APPLICATION_DATA,
                                  0x0100);
  FIO_ASSERT(header[0] == 23, "Content type should be application_data (23)");
  FIO_ASSERT(header[3] == 0x01, "Length high byte");
  FIO_ASSERT(header[4] == 0x00, "Length low byte");
}

/* *****************************************************************************
Test: Change Cipher Spec Handling
***************************************************************************** */
FIO_SFUNC void test_ccs_handling(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, NULL);

  /* Start handshake (buffer must hold hybrid ~1400 bytes) */
  uint8_t ch_out[2048];
  fio_tls13_client_start(&client, ch_out, sizeof(ch_out));

  /* CCS record (should be ignored in TLS 1.3) */
  uint8_t ccs_record[] = {
      0x14,
      0x03,
      0x03,
      0x00,
      0x01, /* CCS record header */
      0x01  /* CCS message */
  };

  uint8_t out[256];
  size_t out_len;
  int ret = fio_tls13_client_process(&client,
                                     ccs_record,
                                     sizeof(ccs_record),
                                     out,
                                     sizeof(out),
                                     &out_len);

  /* CCS should be consumed but ignored */
  FIO_ASSERT(ret == (int)sizeof(ccs_record), "CCS record should be consumed");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_WAIT_SH,
             "State should still be WAIT_SH");
  FIO_ASSERT(out_len == 0, "No output should be generated");

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  /* Basic initialization tests */
  test_client_init();
  test_client_destroy();

  /* ClientHello tests */
  test_client_hello_generation();

  /* Helper function tests */
  test_state_names();
  test_status_queries();
  test_cipher_suite_helpers();
  test_record_header_writing();

  /* Transcript and key derivation tests */
  test_transcript_hash();
  test_key_derivation();

  /* Finished message tests */
  test_finished_message();
  test_server_finished_verification();

  /* Application data tests */
  test_app_data_encryption();

  /* Error handling tests */
  test_error_handling();

  /* Handshake flow tests */
  test_mock_handshake_flow();
  test_ccs_handling();
  return 0;
}
