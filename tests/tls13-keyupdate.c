/* *****************************************************************************
TLS 1.3 KeyUpdate Tests - RFC 8446 Section 4.6.3
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPT
#define FIO_TLS13
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Test KeyUpdate message building
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_keyupdate_build(void) {
  uint8_t msg[8];

  /* Test building KeyUpdate with update_not_requested */
  int len = fio_tls13_build_key_update(msg,
                                       sizeof(msg),
                                       FIO_TLS13_KEY_UPDATE_NOT_REQUESTED);
  FIO_ASSERT(len == 5, "KeyUpdate message should be 5 bytes, got %d", len);
  FIO_ASSERT(msg[0] == FIO_TLS13_HS_KEY_UPDATE,
             "KeyUpdate type should be 24, got %d",
             msg[0]);
  FIO_ASSERT(msg[1] == 0 && msg[2] == 0 && msg[3] == 1,
             "KeyUpdate length should be 1");
  FIO_ASSERT(msg[4] == FIO_TLS13_KEY_UPDATE_NOT_REQUESTED,
             "KeyUpdate request should be 0, got %d",
             msg[4]);

  /* Test building KeyUpdate with update_requested */
  len = fio_tls13_build_key_update(msg,
                                   sizeof(msg),
                                   FIO_TLS13_KEY_UPDATE_REQUESTED);
  FIO_ASSERT(len == 5, "KeyUpdate message should be 5 bytes, got %d", len);
  FIO_ASSERT(msg[4] == FIO_TLS13_KEY_UPDATE_REQUESTED,
             "KeyUpdate request should be 1, got %d",
             msg[4]);

  /* Test with insufficient buffer */
  len = fio_tls13_build_key_update(msg, 4, FIO_TLS13_KEY_UPDATE_NOT_REQUESTED);
  FIO_ASSERT(len == -1, "Should fail with insufficient buffer");

  /* Test with invalid request_update value */
  len = fio_tls13_build_key_update(msg, sizeof(msg), 2);
  FIO_ASSERT(len == -1, "Should fail with invalid request_update value");

  FIO_LOG_DDEBUG("TLS 1.3 KeyUpdate build: PASSED");
}

/* *****************************************************************************
Test KeyUpdate message parsing
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_keyupdate_parse(void) {
  int request_update;

  /* Test parsing update_not_requested */
  uint8_t data0[] = {FIO_TLS13_KEY_UPDATE_NOT_REQUESTED};
  int ret = fio_tls13_parse_key_update(data0, 1, &request_update);
  FIO_ASSERT(ret == 0, "Should parse valid KeyUpdate");
  FIO_ASSERT(request_update == FIO_TLS13_KEY_UPDATE_NOT_REQUESTED,
             "Should be update_not_requested");

  /* Test parsing update_requested */
  uint8_t data1[] = {FIO_TLS13_KEY_UPDATE_REQUESTED};
  ret = fio_tls13_parse_key_update(data1, 1, &request_update);
  FIO_ASSERT(ret == 0, "Should parse valid KeyUpdate");
  FIO_ASSERT(request_update == FIO_TLS13_KEY_UPDATE_REQUESTED,
             "Should be update_requested");

  /* Test with wrong length */
  uint8_t data2[] = {0, 0};
  ret = fio_tls13_parse_key_update(data2, 2, &request_update);
  FIO_ASSERT(ret == -1, "Should fail with wrong length");

  ret = fio_tls13_parse_key_update(data2, 0, &request_update);
  FIO_ASSERT(ret == -1, "Should fail with zero length");

  /* Test with invalid value */
  uint8_t data3[] = {2};
  ret = fio_tls13_parse_key_update(data3, 1, &request_update);
  FIO_ASSERT(ret == -1, "Should fail with invalid value");

  FIO_LOG_DDEBUG("TLS 1.3 KeyUpdate parse: PASSED");
}

/* *****************************************************************************
Test traffic secret update derivation
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_traffic_secret_update(void) {
  /* Test secret from RFC 8448 */
  uint8_t client_app_secret[32] = {
      0x9e, 0x40, 0x64, 0x6c, 0xe7, 0x9a, 0x7f, 0x9d, 0xc0, 0x5a, 0xf8,
      0x88, 0x9b, 0xce, 0x65, 0x52, 0x87, 0x5a, 0xfa, 0x0b, 0x06, 0xdf,
      0x00, 0x87, 0xf7, 0x92, 0xeb, 0xb7, 0xc1, 0x75, 0x04, 0xa5};

  uint8_t new_secret[32];
  uint8_t zero[32] = {0};

  /* Derive next traffic secret */
  fio_tls13_update_traffic_secret(new_secret, client_app_secret, 0);

  /* Verify it's not zero */
  FIO_ASSERT(FIO_MEMCMP(new_secret, zero, 32) != 0,
             "New traffic secret should not be zero");

  /* Verify it's different from original */
  FIO_ASSERT(FIO_MEMCMP(new_secret, client_app_secret, 32) != 0,
             "New traffic secret should differ from original");

  /* Verify determinism - same input produces same output */
  uint8_t new_secret2[32];
  fio_tls13_update_traffic_secret(new_secret2, client_app_secret, 0);
  FIO_ASSERT(FIO_MEMCMP(new_secret, new_secret2, 32) == 0,
             "Traffic secret update should be deterministic");

  FIO_LOG_DDEBUG("TLS 1.3 traffic secret update: PASSED");
}

/* *****************************************************************************
Test traffic secret update with SHA-384
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_traffic_secret_update_sha384(void) {
  uint8_t secret[48];
  uint8_t new_secret[48];
  uint8_t zero[48] = {0};

  /* Initialize with test data */
  for (size_t i = 0; i < 48; ++i)
    secret[i] = (uint8_t)(0x42 + i);

  /* Derive next traffic secret with SHA-384 */
  fio_tls13_update_traffic_secret(new_secret, secret, 1);

  /* Verify it's not zero */
  FIO_ASSERT(FIO_MEMCMP(new_secret, zero, 48) != 0,
             "SHA-384 new traffic secret should not be zero");

  /* Verify it's different from original */
  FIO_ASSERT(FIO_MEMCMP(new_secret, secret, 48) != 0,
             "SHA-384 new traffic secret should differ from original");

  FIO_LOG_DDEBUG("TLS 1.3 traffic secret update (SHA-384): PASSED");
}

/* *****************************************************************************
Test KeyUpdate processing - updates receiving keys
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_keyupdate_process(void) {
  /* Set up initial traffic secret and keys */
  uint8_t traffic_secret[32] = {0x9e, 0x40, 0x64, 0x6c, 0xe7, 0x9a, 0x7f, 0x9d,
                                0xc0, 0x5a, 0xf8, 0x88, 0x9b, 0xce, 0x65, 0x52,
                                0x87, 0x5a, 0xfa, 0x0b, 0x06, 0xdf, 0x00, 0x87,
                                0xf7, 0x92, 0xeb, 0xb7, 0xc1, 0x75, 0x04, 0xa5};
  uint8_t original_secret[32];
  FIO_MEMCPY(original_secret, traffic_secret, 32);

  fio_tls13_record_keys_s keys;
  uint8_t key[16], iv[12];
  fio_tls13_derive_traffic_keys(key, 16, iv, traffic_secret, 0);
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  /* Simulate some records being processed */
  keys.sequence_number = 42;

  uint8_t original_key[16];
  FIO_MEMCPY(original_key, keys.key, 16);

  /* Process KeyUpdate with update_requested */
  uint8_t ku_body[] = {FIO_TLS13_KEY_UPDATE_REQUESTED};
  uint8_t key_update_pending = 0;

  int ret = fio_tls13_process_key_update(traffic_secret,
                                         &keys,
                                         ku_body,
                                         1,
                                         &key_update_pending,
                                         0, /* use_sha384 */
                                         16,
                                         FIO_TLS13_CIPHER_AES_128_GCM);

  FIO_ASSERT(ret == 0, "KeyUpdate processing should succeed");

  /* Verify traffic secret was updated */
  FIO_ASSERT(FIO_MEMCMP(traffic_secret, original_secret, 32) != 0,
             "Traffic secret should be updated");

  /* Verify keys were updated */
  FIO_ASSERT(FIO_MEMCMP(keys.key, original_key, 16) != 0,
             "Keys should be updated");

  /* Verify sequence number was reset */
  FIO_ASSERT(keys.sequence_number == 0,
             "Sequence number should be reset to 0, got %llu",
             (unsigned long long)keys.sequence_number);

  /* Verify pending flag was set (because update_requested) */
  FIO_ASSERT(key_update_pending == 1,
             "key_update_pending should be set for update_requested");

  FIO_LOG_DDEBUG("TLS 1.3 KeyUpdate process: PASSED");
}

/* *****************************************************************************
Test KeyUpdate processing - update_not_requested doesn't set pending
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_keyupdate_no_response(void) {
  uint8_t traffic_secret[32];
  for (size_t i = 0; i < 32; ++i)
    traffic_secret[i] = (uint8_t)(0x10 + i);

  fio_tls13_record_keys_s keys;
  uint8_t key[16], iv[12];
  fio_tls13_derive_traffic_keys(key, 16, iv, traffic_secret, 0);
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  /* Process KeyUpdate with update_not_requested */
  uint8_t ku_body[] = {FIO_TLS13_KEY_UPDATE_NOT_REQUESTED};
  uint8_t key_update_pending = 0;

  int ret = fio_tls13_process_key_update(traffic_secret,
                                         &keys,
                                         ku_body,
                                         1,
                                         &key_update_pending,
                                         0,
                                         16,
                                         FIO_TLS13_CIPHER_AES_128_GCM);

  FIO_ASSERT(ret == 0, "KeyUpdate processing should succeed");

  /* Verify pending flag was NOT set */
  FIO_ASSERT(key_update_pending == 0,
             "key_update_pending should NOT be set for update_not_requested");

  FIO_LOG_DDEBUG("TLS 1.3 KeyUpdate no response: PASSED");
}

/* *****************************************************************************
Test KeyUpdate response sending
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_keyupdate_response(void) {
  uint8_t traffic_secret[32];
  for (size_t i = 0; i < 32; ++i)
    traffic_secret[i] = (uint8_t)(0x20 + i);

  uint8_t original_secret[32];
  FIO_MEMCPY(original_secret, traffic_secret, 32);

  fio_tls13_record_keys_s keys;
  uint8_t key[16], iv[12];
  fio_tls13_derive_traffic_keys(key, 16, iv, traffic_secret, 0);
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  uint8_t original_key[16];
  FIO_MEMCPY(original_key, keys.key, 16);

  uint8_t key_update_pending = 1;
  uint8_t out[64];

  int enc_len =
      fio_tls13_send_key_update_response(out,
                                         sizeof(out),
                                         traffic_secret,
                                         &keys,
                                         &key_update_pending,
                                         0,
                                         16,
                                         FIO_TLS13_CIPHER_AES_128_GCM);

  FIO_ASSERT(enc_len > 0,
             "KeyUpdate response should be encrypted, got %d",
             enc_len);

  /* Verify it's a valid TLS record */
  FIO_ASSERT(out[0] == FIO_TLS13_CONTENT_APPLICATION_DATA,
             "Should be application_data record type");

  /* Verify traffic secret was updated */
  FIO_ASSERT(FIO_MEMCMP(traffic_secret, original_secret, 32) != 0,
             "Traffic secret should be updated after response");

  /* Verify keys were updated */
  FIO_ASSERT(FIO_MEMCMP(keys.key, original_key, 16) != 0,
             "Keys should be updated after response");

  /* Verify sequence number was reset */
  FIO_ASSERT(keys.sequence_number == 0,
             "Sequence number should be reset after response");

  /* Verify pending flag was cleared */
  FIO_ASSERT(key_update_pending == 0,
             "key_update_pending should be cleared after response");

  FIO_LOG_DDEBUG("TLS 1.3 KeyUpdate response: PASSED");
}

/* *****************************************************************************
Test KeyUpdate roundtrip - encrypt response, decrypt, verify
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_keyupdate_roundtrip(void) {
  /* Set up matching sender and receiver keys */
  uint8_t sender_secret[32];
  uint8_t receiver_secret[32];
  for (size_t i = 0; i < 32; ++i) {
    sender_secret[i] = (uint8_t)(0x30 + i);
    receiver_secret[i] = (uint8_t)(0x30 + i);
  }

  fio_tls13_record_keys_s sender_keys, receiver_keys;
  uint8_t key[16], iv[12];

  fio_tls13_derive_traffic_keys(key, 16, iv, sender_secret, 0);
  fio_tls13_record_keys_init(&sender_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);

  fio_tls13_derive_traffic_keys(key, 16, iv, receiver_secret, 0);
  fio_tls13_record_keys_init(&receiver_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);

  /* Sender sends KeyUpdate response */
  uint8_t key_update_pending = 1;
  uint8_t encrypted[64];

  int enc_len =
      fio_tls13_send_key_update_response(encrypted,
                                         sizeof(encrypted),
                                         sender_secret,
                                         &sender_keys,
                                         &key_update_pending,
                                         0,
                                         16,
                                         FIO_TLS13_CIPHER_AES_128_GCM);
  FIO_ASSERT(enc_len > 0, "Encryption should succeed");

  /* Receiver decrypts */
  uint8_t decrypted[64];
  fio_tls13_content_type_e content_type;
  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         sizeof(decrypted),
                                         &content_type,
                                         encrypted,
                                         (size_t)enc_len,
                                         &receiver_keys);

  FIO_ASSERT(dec_len > 0, "Decryption should succeed, got %d", dec_len);
  FIO_ASSERT(content_type == FIO_TLS13_CONTENT_HANDSHAKE,
             "Content type should be handshake");
  FIO_ASSERT(dec_len == 5, "Decrypted KeyUpdate should be 5 bytes");
  FIO_ASSERT(decrypted[0] == FIO_TLS13_HS_KEY_UPDATE,
             "Should be KeyUpdate message");
  FIO_ASSERT(decrypted[4] == FIO_TLS13_KEY_UPDATE_NOT_REQUESTED,
             "Should be update_not_requested");

  FIO_LOG_DDEBUG("TLS 1.3 KeyUpdate roundtrip: PASSED");
}

/* *****************************************************************************
Test multiple key updates in sequence
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_keyupdate_multiple(void) {
  uint8_t traffic_secret[32];
  for (size_t i = 0; i < 32; ++i)
    traffic_secret[i] = (uint8_t)(0x40 + i);

  fio_tls13_record_keys_s keys;
  uint8_t key[16], iv[12];
  fio_tls13_derive_traffic_keys(key, 16, iv, traffic_secret, 0);
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  /* Store secrets after each update to verify they're all different */
  uint8_t secrets[5][32];
  FIO_MEMCPY(secrets[0], traffic_secret, 32);

  uint8_t ku_body[] = {FIO_TLS13_KEY_UPDATE_NOT_REQUESTED};
  uint8_t key_update_pending = 0;

  /* Perform 4 key updates */
  for (int i = 1; i < 5; ++i) {
    int ret = fio_tls13_process_key_update(traffic_secret,
                                           &keys,
                                           ku_body,
                                           1,
                                           &key_update_pending,
                                           0,
                                           16,
                                           FIO_TLS13_CIPHER_AES_128_GCM);
    FIO_ASSERT(ret == 0, "KeyUpdate %d should succeed", i);
    FIO_MEMCPY(secrets[i], traffic_secret, 32);

    /* Verify each secret is different from all previous */
    for (int j = 0; j < i; ++j) {
      FIO_ASSERT(FIO_MEMCMP(secrets[i], secrets[j], 32) != 0,
                 "Secret %d should differ from secret %d",
                 i,
                 j);
    }
  }

  FIO_LOG_DDEBUG("TLS 1.3 KeyUpdate multiple: PASSED");
}

/* *****************************************************************************
Main test runner
***************************************************************************** */
int main(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 KeyUpdate (RFC 8446 Section 4.6.3)...\n");

  FIO_LOG_DDEBUG("=== KeyUpdate Message Tests ===");
  fio___test_tls13_keyupdate_build();
  fio___test_tls13_keyupdate_parse();

  FIO_LOG_DDEBUG("=== Traffic Secret Update Tests ===");
  fio___test_tls13_traffic_secret_update();
  fio___test_tls13_traffic_secret_update_sha384();

  FIO_LOG_DDEBUG("=== KeyUpdate Processing Tests ===");
  fio___test_tls13_keyupdate_process();
  fio___test_tls13_keyupdate_no_response();

  FIO_LOG_DDEBUG("=== KeyUpdate Response Tests ===");
  fio___test_tls13_keyupdate_response();
  fio___test_tls13_keyupdate_roundtrip();

  FIO_LOG_DDEBUG("=== Multiple KeyUpdate Tests ===");
  fio___test_tls13_keyupdate_multiple();

  FIO_LOG_DDEBUG("\nAll TLS 1.3 KeyUpdate tests passed!");
  return 0;
}
