/* *****************************************************************************
TLS 1.3 Alert Handling Tests (RFC 8446 Section 6)

Tests for alert message building, sending, and error condition handling.
***************************************************************************** */
#define FIO_LOG
#define FIO_CRYPT
#define FIO_TLS13
#include "test-helpers.h"

/* *****************************************************************************
Test: Alert Constants
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_alert_constants(void) {
  /* Verify alert codes match RFC 8446 Section 6 */
  FIO_ASSERT(FIO_TLS13_ALERT_CLOSE_NOTIFY == 0, "close_notify should be 0");
  FIO_ASSERT(FIO_TLS13_ALERT_UNEXPECTED_MESSAGE == 10,
             "unexpected_message should be 10");
  FIO_ASSERT(FIO_TLS13_ALERT_BAD_RECORD_MAC == 20,
             "bad_record_mac should be 20");
  FIO_ASSERT(FIO_TLS13_ALERT_RECORD_OVERFLOW == 22,
             "record_overflow should be 22");
  FIO_ASSERT(FIO_TLS13_ALERT_HANDSHAKE_FAILURE == 40,
             "handshake_failure should be 40");
  FIO_ASSERT(FIO_TLS13_ALERT_BAD_CERTIFICATE == 42,
             "bad_certificate should be 42");
  FIO_ASSERT(FIO_TLS13_ALERT_ILLEGAL_PARAMETER == 47,
             "illegal_parameter should be 47");
  FIO_ASSERT(FIO_TLS13_ALERT_DECODE_ERROR == 50, "decode_error should be 50");
  FIO_ASSERT(FIO_TLS13_ALERT_DECRYPT_ERROR == 51, "decrypt_error should be 51");
  FIO_ASSERT(FIO_TLS13_ALERT_PROTOCOL_VERSION == 70,
             "protocol_version should be 70");
  FIO_ASSERT(FIO_TLS13_ALERT_MISSING_EXTENSION == 109,
             "missing_extension should be 109");
  FIO_ASSERT(FIO_TLS13_ALERT_UNSUPPORTED_EXTENSION == 110,
             "unsupported_extension should be 110");
  FIO_ASSERT(FIO_TLS13_ALERT_CERTIFICATE_REQUIRED == 116,
             "certificate_required should be 116");

  /* Verify alert levels */
  FIO_ASSERT(FIO_TLS13_ALERT_LEVEL_WARNING == 1, "warning level should be 1");
  FIO_ASSERT(FIO_TLS13_ALERT_LEVEL_FATAL == 2, "fatal level should be 2");
}

/* *****************************************************************************
Test: Alert Name Function
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_alert_names(void) {
  /* Test known alerts */
  FIO_ASSERT(strcmp(fio_tls13_alert_name(FIO_TLS13_ALERT_CLOSE_NOTIFY),
                    "close_notify") == 0,
             "close_notify name");
  FIO_ASSERT(strcmp(fio_tls13_alert_name(FIO_TLS13_ALERT_UNEXPECTED_MESSAGE),
                    "unexpected_message") == 0,
             "unexpected_message name");
  FIO_ASSERT(strcmp(fio_tls13_alert_name(FIO_TLS13_ALERT_BAD_RECORD_MAC),
                    "bad_record_mac") == 0,
             "bad_record_mac name");
  FIO_ASSERT(strcmp(fio_tls13_alert_name(FIO_TLS13_ALERT_RECORD_OVERFLOW),
                    "record_overflow") == 0,
             "record_overflow name");
  FIO_ASSERT(strcmp(fio_tls13_alert_name(FIO_TLS13_ALERT_HANDSHAKE_FAILURE),
                    "handshake_failure") == 0,
             "handshake_failure name");
  FIO_ASSERT(strcmp(fio_tls13_alert_name(FIO_TLS13_ALERT_DECODE_ERROR),
                    "decode_error") == 0,
             "decode_error name");
  FIO_ASSERT(strcmp(fio_tls13_alert_name(FIO_TLS13_ALERT_DECRYPT_ERROR),
                    "decrypt_error") == 0,
             "decrypt_error name");

  /* Test unknown alert */
  FIO_ASSERT(strcmp(fio_tls13_alert_name(255), "unknown_alert") == 0,
             "unknown alert should return 'unknown_alert'");
}

/* *****************************************************************************
Test: Build Alert Message
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_build_alert(void) {
  uint8_t buf[8];

  /* Test building a fatal alert */
  int len = fio_tls13_build_alert(buf,
                                  sizeof(buf),
                                  FIO_TLS13_ALERT_LEVEL_FATAL,
                                  FIO_TLS13_ALERT_HANDSHAKE_FAILURE);
  FIO_ASSERT(len == 2, "Alert message should be 2 bytes");
  FIO_ASSERT(buf[0] == FIO_TLS13_ALERT_LEVEL_FATAL, "Level should be fatal");
  FIO_ASSERT(buf[1] == FIO_TLS13_ALERT_HANDSHAKE_FAILURE,
             "Description should be handshake_failure");

  /* Test building a warning alert (close_notify) */
  len = fio_tls13_build_alert(buf,
                              sizeof(buf),
                              FIO_TLS13_ALERT_LEVEL_WARNING,
                              FIO_TLS13_ALERT_CLOSE_NOTIFY);
  FIO_ASSERT(len == 2, "Alert message should be 2 bytes");
  FIO_ASSERT(buf[0] == FIO_TLS13_ALERT_LEVEL_WARNING,
             "Level should be warning");
  FIO_ASSERT(buf[1] == FIO_TLS13_ALERT_CLOSE_NOTIFY,
             "Description should be close_notify");

  /* Test with insufficient buffer */
  len = fio_tls13_build_alert(buf, 1, FIO_TLS13_ALERT_LEVEL_FATAL, 0);
  FIO_ASSERT(len == -1, "Should fail with insufficient buffer");

  /* Test with NULL buffer */
  len = fio_tls13_build_alert(NULL, 8, FIO_TLS13_ALERT_LEVEL_FATAL, 0);
  FIO_ASSERT(len == -1, "Should fail with NULL buffer");
}

/* *****************************************************************************
Test: Plaintext Alert Record
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_plaintext_alert(void) {
  uint8_t buf[16];

  /* Build a plaintext alert record */
  int len = fio_tls13_send_alert_plaintext(buf,
                                           sizeof(buf),
                                           FIO_TLS13_ALERT_LEVEL_FATAL,
                                           FIO_TLS13_ALERT_DECODE_ERROR);

  FIO_ASSERT(len == 7, "Plaintext alert record should be 7 bytes");

  /* Verify record header */
  FIO_ASSERT(buf[0] == FIO_TLS13_CONTENT_ALERT, "Content type should be alert");
  FIO_ASSERT(buf[1] == 0x03, "Legacy version major should be 0x03");
  FIO_ASSERT(buf[2] == 0x03, "Legacy version minor should be 0x03");
  FIO_ASSERT(buf[3] == 0x00, "Length high byte should be 0");
  FIO_ASSERT(buf[4] == 0x02, "Length low byte should be 2");

  /* Verify alert content */
  FIO_ASSERT(buf[5] == FIO_TLS13_ALERT_LEVEL_FATAL,
             "Alert level should be fatal");
  FIO_ASSERT(buf[6] == FIO_TLS13_ALERT_DECODE_ERROR,
             "Alert description should be decode_error");

  /* Test with insufficient buffer */
  len = fio_tls13_send_alert_plaintext(buf, 6, FIO_TLS13_ALERT_LEVEL_FATAL, 0);
  FIO_ASSERT(len == -1, "Should fail with insufficient buffer");
}

/* *****************************************************************************
Test: Encrypted Alert Record
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_encrypted_alert(void) {
  /* Set up test keys */
  uint8_t key[16] = {0x01,
                     0x02,
                     0x03,
                     0x04,
                     0x05,
                     0x06,
                     0x07,
                     0x08,
                     0x09,
                     0x0a,
                     0x0b,
                     0x0c,
                     0x0d,
                     0x0e,
                     0x0f,
                     0x10};
  uint8_t iv[12] =
      {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c};

  fio_tls13_record_keys_s keys;
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  uint8_t buf[64];

  /* Build an encrypted alert record */
  int len = fio_tls13_send_alert(buf,
                                 sizeof(buf),
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_BAD_RECORD_MAC,
                                 &keys);

  /* Expected size: 5 (header) + 2 (alert) + 1 (content type) + 16 (tag) = 24 */
  FIO_ASSERT(len == 24,
             "Encrypted alert record should be 24 bytes (got %d)",
             len);

  /* Verify record header */
  FIO_ASSERT(buf[0] == FIO_TLS13_CONTENT_APPLICATION_DATA,
             "Outer content type should be application_data");
  FIO_ASSERT(buf[1] == 0x03, "Legacy version major should be 0x03");
  FIO_ASSERT(buf[2] == 0x03, "Legacy version minor should be 0x03");

  /* Decrypt and verify */
  uint8_t decrypted[32];
  fio_tls13_content_type_e content_type;

  /* Reset keys for decryption (sequence number was incremented) */
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         sizeof(decrypted),
                                         &content_type,
                                         buf,
                                         (size_t)len,
                                         &keys);

  FIO_ASSERT(dec_len == 2,
             "Decrypted alert should be 2 bytes (got %d)",
             dec_len);
  FIO_ASSERT(content_type == FIO_TLS13_CONTENT_ALERT,
             "Inner content type should be alert");
  FIO_ASSERT(decrypted[0] == FIO_TLS13_ALERT_LEVEL_FATAL,
             "Alert level should be fatal");
  FIO_ASSERT(decrypted[1] == FIO_TLS13_ALERT_BAD_RECORD_MAC,
             "Alert description should be bad_record_mac");

  fio_tls13_record_keys_clear(&keys);
}

/* *****************************************************************************
Test: Record Overflow Detection
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_record_overflow(void) {
  /* Create a record header with length exceeding max ciphertext length */
  uint8_t oversized_record[10] = {
      FIO_TLS13_CONTENT_APPLICATION_DATA, /* Content type */
      0x03,
      0x03,                                      /* Legacy version */
      (FIO_TLS13_MAX_CIPHERTEXT_LEN + 1) >> 8,   /* Length high byte */
      (FIO_TLS13_MAX_CIPHERTEXT_LEN + 1) & 0xFF, /* Length low byte */
      0x00,
      0x00,
      0x00,
      0x00,
      0x00 /* Dummy payload */
  };

  fio_tls13_content_type_e content_type;
  size_t payload_len;

  /* This should fail because length exceeds max */
  const uint8_t *payload =
      fio_tls13_record_parse_header(oversized_record,
                                    sizeof(oversized_record),
                                    &content_type,
                                    &payload_len);

  FIO_ASSERT(payload == NULL,
             "Record with oversized length should be rejected");

  /* Test with exactly max ciphertext length (should be accepted if data
   * present) */
  uint8_t max_record[5] = {FIO_TLS13_CONTENT_APPLICATION_DATA,
                           0x03,
                           0x03,
                           (FIO_TLS13_MAX_CIPHERTEXT_LEN) >> 8,
                           (FIO_TLS13_MAX_CIPHERTEXT_LEN)&0xFF};

  /* This should fail because we don't have enough data, but the length is valid
   */
  payload = fio_tls13_record_parse_header(max_record,
                                          sizeof(max_record),
                                          &content_type,
                                          &payload_len);

  /* Should fail because we don't have the full payload */
  FIO_ASSERT(payload == NULL, "Incomplete record should be rejected");
}

/* *****************************************************************************
Test: AEAD Decryption Failure Alert
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_aead_failure_alert(void) {
  /* Set up test keys */
  uint8_t key[16] = {0x01,
                     0x02,
                     0x03,
                     0x04,
                     0x05,
                     0x06,
                     0x07,
                     0x08,
                     0x09,
                     0x0a,
                     0x0b,
                     0x0c,
                     0x0d,
                     0x0e,
                     0x0f,
                     0x10};
  uint8_t iv[12] =
      {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c};

  fio_tls13_record_keys_s keys;
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  /* Create a valid encrypted record first */
  uint8_t plaintext[] = "Hello, TLS 1.3!";
  uint8_t encrypted[64];

  int enc_len = fio_tls13_record_encrypt(encrypted,
                                         sizeof(encrypted),
                                         plaintext,
                                         sizeof(plaintext) - 1,
                                         FIO_TLS13_CONTENT_APPLICATION_DATA,
                                         &keys);
  FIO_ASSERT(enc_len > 0, "Encryption should succeed");

  /* Corrupt the ciphertext (flip a bit in the encrypted data) */
  encrypted[10] ^= 0x01;

  /* Reset keys for decryption */
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  /* Try to decrypt - should fail with bad_record_mac */
  uint8_t decrypted[64];
  fio_tls13_content_type_e content_type;

  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         sizeof(decrypted),
                                         &content_type,
                                         encrypted,
                                         (size_t)enc_len,
                                         &keys);

  FIO_ASSERT(dec_len == -1, "Decryption of corrupted record should fail");

  fio_tls13_record_keys_clear(&keys);
}

/* *****************************************************************************
Test: No Content Type After Padding
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_no_content_type(void) {
  /* Set up test keys */
  uint8_t key[16] = {0x01,
                     0x02,
                     0x03,
                     0x04,
                     0x05,
                     0x06,
                     0x07,
                     0x08,
                     0x09,
                     0x0a,
                     0x0b,
                     0x0c,
                     0x0d,
                     0x0e,
                     0x0f,
                     0x10};
  uint8_t iv[12] =
      {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c};

  fio_tls13_record_keys_s keys;
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  /* Create a record with all-zero plaintext (no content type) */
  uint8_t all_zeros[16] = {0};
  uint8_t encrypted[64];

  /* Manually encrypt all zeros - this simulates a malformed record
   * where the content type byte is missing (all padding) */
  int enc_len = fio_tls13_record_encrypt(encrypted,
                                         sizeof(encrypted),
                                         all_zeros,
                                         sizeof(all_zeros),
                                         FIO_TLS13_CONTENT_APPLICATION_DATA,
                                         &keys);
  FIO_ASSERT(enc_len > 0, "Encryption should succeed");

  /* Reset keys for decryption */
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  /* Decrypt - should succeed because content type is appended */
  uint8_t decrypted[64];
  fio_tls13_content_type_e content_type;

  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         sizeof(decrypted),
                                         &content_type,
                                         encrypted,
                                         (size_t)enc_len,
                                         &keys);

  /* This should succeed because fio_tls13_record_encrypt appends the content
   * type */
  FIO_ASSERT(dec_len >= 0, "Decryption should succeed");
  FIO_ASSERT(content_type == FIO_TLS13_CONTENT_APPLICATION_DATA,
             "Content type should be application_data");

  fio_tls13_record_keys_clear(&keys);
}

/* *****************************************************************************
Test: Client Error State Alerts
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_client_error_alerts(void) {
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Start handshake - need 1536+ bytes for X25519MLKEM768 hybrid ClientHello */
  uint8_t ch_buf[2048];
  int ch_len = fio_tls13_client_start(&client, ch_buf, sizeof(ch_buf));
  FIO_ASSERT(ch_len > 0, "ClientHello should be generated");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_WAIT_SH,
             "Client should be waiting for ServerHello");

  /* Send a malformed record (wrong content type) */
  uint8_t bad_record[10] = {
      FIO_TLS13_CONTENT_APPLICATION_DATA, /* Wrong - should be handshake */
      0x03,
      0x03, /* Version */
      0x00,
      0x01, /* Length = 1 */
      0x00  /* Dummy data */
  };

  uint8_t out[512];
  size_t out_len = 0;

  int result = fio_tls13_client_process(&client,
                                        bad_record,
                                        sizeof(bad_record),
                                        out,
                                        sizeof(out),
                                        &out_len);

  FIO_ASSERT(result == -1, "Processing bad record should fail");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_ERROR,
             "Client should be in error state");
  FIO_ASSERT(client.alert_description == FIO_TLS13_ALERT_UNEXPECTED_MESSAGE,
             "Alert should be unexpected_message (got %d)",
             client.alert_description);

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Test: Server Error State Alerts
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_server_error_alerts(void) {
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  /* Send a malformed record (wrong content type) */
  uint8_t bad_record[10] = {
      FIO_TLS13_CONTENT_APPLICATION_DATA, /* Wrong - should be handshake */
      0x03,
      0x03, /* Version */
      0x00,
      0x01, /* Length = 1 */
      0x00  /* Dummy data */
  };

  uint8_t out[512];
  size_t out_len = 0;

  int result = fio_tls13_server_process(&server,
                                        bad_record,
                                        sizeof(bad_record),
                                        out,
                                        sizeof(out),
                                        &out_len);

  FIO_ASSERT(result == -1, "Processing bad record should fail");
  FIO_ASSERT(server.state == FIO_TLS13_SERVER_STATE_ERROR,
             "Server should be in error state");
  FIO_ASSERT(server.alert_description == FIO_TLS13_ALERT_UNEXPECTED_MESSAGE,
             "Alert should be unexpected_message (got %d)",
             server.alert_description);

  fio_tls13_server_destroy(&server);
}

/* *****************************************************************************
Test: HelloRetryRequest Second HRR Alert
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_second_hrr_alert(void) {
  /* This test verifies that receiving a second HelloRetryRequest
   * triggers an unexpected_message alert per RFC 8446 Section 4.1.4 */

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Simulate that we already received an HRR */
  client.hrr_received = 1;
  client.state = FIO_TLS13_STATE_WAIT_SH2;

  /* Build a fake HelloRetryRequest (ServerHello with HRR random) */
  uint8_t hrr_record[256];
  uint8_t *p = hrr_record;

  /* Record header */
  *p++ = FIO_TLS13_CONTENT_HANDSHAKE;
  *p++ = 0x03;
  *p++ = 0x03;

  /* We'll fill in length later */
  uint8_t *len_ptr = p;
  p += 2;

  uint8_t *hs_start = p;

  /* Handshake header: ServerHello */
  *p++ = FIO_TLS13_HS_SERVER_HELLO;
  uint8_t *hs_len_ptr = p;
  p += 3;

  uint8_t *body_start = p;

  /* Legacy version */
  *p++ = 0x03;
  *p++ = 0x03;

  /* HRR random (magic value from RFC 8446) */
  static const uint8_t hrr_random[32] = {
      0xCF, 0x21, 0xAD, 0x74, 0xE5, 0x9A, 0x61, 0x11, 0xBE, 0x1D, 0x8C,
      0x02, 0x1E, 0x65, 0xB8, 0x91, 0xC2, 0xA2, 0x11, 0x16, 0x7A, 0xBB,
      0x8C, 0x5E, 0x07, 0x9E, 0x09, 0xE2, 0xC8, 0xA8, 0x33, 0x9C};
  FIO_MEMCPY(p, hrr_random, 32);
  p += 32;

  /* Session ID (empty) */
  *p++ = 0;

  /* Cipher suite */
  *p++ = 0x13;
  *p++ = 0x01;

  /* Compression */
  *p++ = 0;

  /* Extensions length */
  *p++ = 0;
  *p++ = 6;

  /* supported_versions extension */
  *p++ = 0x00;
  *p++ = 0x2b; /* Extension type 43 */
  *p++ = 0x00;
  *p++ = 0x02; /* Length */
  *p++ = 0x03;
  *p++ = 0x04; /* TLS 1.3 */

  /* Fill in lengths */
  size_t body_len = (size_t)(p - body_start);
  hs_len_ptr[0] = (uint8_t)(body_len >> 16);
  hs_len_ptr[1] = (uint8_t)(body_len >> 8);
  hs_len_ptr[2] = (uint8_t)(body_len);

  size_t hs_len = (size_t)(p - hs_start);
  len_ptr[0] = (uint8_t)(hs_len >> 8);
  len_ptr[1] = (uint8_t)(hs_len);

  size_t record_len = (size_t)(p - hrr_record);

  /* Process the second HRR */
  uint8_t out[512];
  size_t out_len = 0;

  int result = fio_tls13_client_process(&client,
                                        hrr_record,
                                        record_len,
                                        out,
                                        sizeof(out),
                                        &out_len);

  FIO_ASSERT(result == -1, "Processing second HRR should fail");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_ERROR,
             "Client should be in error state");
  FIO_ASSERT(client.alert_description == FIO_TLS13_ALERT_UNEXPECTED_MESSAGE,
             "Alert should be unexpected_message for second HRR (got %d)",
             client.alert_description);

  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
Main Test Runner
***************************************************************************** */
int main(void) {
  fio___test_tls13_alert_constants();
  fio___test_tls13_alert_names();
  fio___test_tls13_build_alert();
  fio___test_tls13_plaintext_alert();
  fio___test_tls13_encrypted_alert();
  fio___test_tls13_record_overflow();
  fio___test_tls13_aead_failure_alert();
  fio___test_tls13_no_content_type();
  fio___test_tls13_client_error_alerts();
  fio___test_tls13_server_error_alerts();
  fio___test_tls13_second_hrr_alert();

  return 0;
}
