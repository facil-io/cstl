/* *****************************************************************************
TLS 1.3 HelloRetryRequest (HRR) Tests

Tests for RFC 8446 Section 4.1.4 HelloRetryRequest handling.
***************************************************************************** */
#define FIO_LOG
#define FIO_CRYPT
#define FIO_TLS13
#include "test-helpers.h"

/* *****************************************************************************
Test: HRR Random Detection
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_hrr_detection(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 HRR detection...");

  /* HRR magic random value from RFC 8446 */
  static const uint8_t hrr_random[32] = {
      0xCF, 0x21, 0xAD, 0x74, 0xE5, 0x9A, 0x61, 0x11, 0xBE, 0x1D, 0x8C,
      0x02, 0x1E, 0x65, 0xB8, 0x91, 0xC2, 0xA2, 0x11, 0x16, 0x7A, 0xBB,
      0x8C, 0x5E, 0x07, 0x9E, 0x09, 0xE2, 0xC8, 0xA8, 0x33, 0x9C};

  /* Verify the constant matches */
  FIO_ASSERT(FIO_MEMCMP(FIO_TLS13_HRR_RANDOM, hrr_random, 32) == 0,
             "HRR_RANDOM constant should match RFC 8446 value");

  /* Build a minimal HRR message */
  uint8_t hrr_data[128];
  uint8_t *p = hrr_data;

  /* Legacy version: TLS 1.2 (0x0303) */
  *p++ = 0x03;
  *p++ = 0x03;

  /* HRR random (32 bytes) */
  FIO_MEMCPY(p, hrr_random, 32);
  p += 32;

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

  /* supported_versions extension (required) */
  *p++ = 0x00;
  *p++ = 0x2B; /* type (43) */
  *p++ = 0x00;
  *p++ = 0x02; /* length */
  *p++ = 0x03;
  *p++ = 0x04; /* TLS 1.3 */

  /* key_share extension - in HRR, only contains selected group (2 bytes)
   * Per RFC 8446 Section 4.2.8: "In a HelloRetryRequest message, the
   * 'key_share' extension contains a single NamedGroup value" */
  *p++ = 0x00;
  *p++ = 0x33; /* type (51) */
  *p++ = 0x00;
  *p++ = 0x02; /* length: just the group (2 bytes) */
  *p++ = 0x00;
  *p++ = 0x17; /* group: secp256r1 (23) */

  /* Write extensions length */
  size_t ext_len = (size_t)(p - ext_start);
  ext_len_ptr[0] = (uint8_t)(ext_len >> 8);
  ext_len_ptr[1] = (uint8_t)(ext_len & 0xFF);

  size_t hrr_len = (size_t)(p - hrr_data);

  /* Parse it */
  fio_tls13_server_hello_s sh;
  int ret = fio_tls13_parse_server_hello(&sh, hrr_data, hrr_len);

  FIO_ASSERT(ret == 0, "HRR parse should succeed");
  FIO_ASSERT(sh.is_hello_retry_request == 1, "Should detect HRR");
  FIO_ASSERT(sh.cipher_suite == FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256,
             "Cipher suite should be AES-128-GCM-SHA256");
  FIO_ASSERT(sh.key_share_group == FIO_TLS13_GROUP_SECP256R1,
             "Selected group should be P-256");

  /* Test with normal ServerHello (not HRR) */
  uint8_t normal_random[32];
  fio_rand_bytes(normal_random, 32);
  FIO_MEMCPY(hrr_data + 2, normal_random, 32);

  ret = fio_tls13_parse_server_hello(&sh, hrr_data, hrr_len);
  FIO_ASSERT(ret == 0, "Normal SH parse should succeed");
  FIO_ASSERT(sh.is_hello_retry_request == 0, "Should NOT detect HRR");

  FIO_LOG_DDEBUG("  PASS: HRR detection tests");
}

/* *****************************************************************************
Test: Transcript Hash with message_hash
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_hrr_transcript(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 HRR transcript hash...");

  /* Initialize a client */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Simulate ClientHello1 being added to transcript */
  uint8_t fake_ch1[100];
  fio_rand_bytes(fake_ch1, sizeof(fake_ch1));
  fio___tls13_transcript_update(&client, fake_ch1, sizeof(fake_ch1));

  /* Get hash before replacement */
  uint8_t hash_before[32];
  fio___tls13_transcript_hash(&client, hash_before);

  /* Replace transcript with message_hash */
  fio___tls13_transcript_replace_with_message_hash(&client);

  /* Get hash after replacement */
  uint8_t hash_after[32];
  fio___tls13_transcript_hash(&client, hash_after);

  /* The hashes should be different (transcript was replaced) */
  FIO_ASSERT(FIO_MEMCMP(hash_before, hash_after, 32) != 0,
             "Transcript hash should change after message_hash replacement");

  /* Verify the new transcript starts with message_hash structure:
   * msg_type (1) = 254, length (3) = 32, hash (32) */
  /* We can't directly verify the internal state, but we can verify
   * that adding the same data produces different results */
  fio_tls13_client_s client2;
  fio_tls13_client_init(&client2, "example.com");
  fio___tls13_transcript_update(&client2, fake_ch1, sizeof(fake_ch1));

  uint8_t hash_original[32];
  fio___tls13_transcript_hash(&client2, hash_original);

  /* Original should match hash_before */
  FIO_ASSERT(FIO_MEMCMP(hash_before, hash_original, 32) == 0,
             "Original transcript hash should match");

  fio_tls13_client_destroy(&client);
  fio_tls13_client_destroy(&client2);

  FIO_LOG_DDEBUG("  PASS: HRR transcript hash tests");
}

/* *****************************************************************************
Test: Second HRR Rejection
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_hrr_second_rejection(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 second HRR rejection...");

  /* Initialize a client */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Simulate first HRR received */
  client.hrr_received = 1;
  client.hrr_selected_group = FIO_TLS13_GROUP_SECP256R1;

  /* Build a second HRR message */
  static const uint8_t hrr_random[32] = {
      0xCF, 0x21, 0xAD, 0x74, 0xE5, 0x9A, 0x61, 0x11, 0xBE, 0x1D, 0x8C,
      0x02, 0x1E, 0x65, 0xB8, 0x91, 0xC2, 0xA2, 0x11, 0x16, 0x7A, 0xBB,
      0x8C, 0x5E, 0x07, 0x9E, 0x09, 0xE2, 0xC8, 0xA8, 0x33, 0x9C};

  fio_tls13_server_hello_s sh;
  FIO_MEMSET(&sh, 0, sizeof(sh));
  FIO_MEMCPY(sh.random, hrr_random, 32);
  sh.is_hello_retry_request = 1;
  sh.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  sh.key_share_group = FIO_TLS13_GROUP_SECP256R1;

  /* Build minimal HRR message for the handler */
  uint8_t hrr_msg[64];
  uint8_t *p = hrr_msg;

  /* Handshake header */
  *p++ = FIO_TLS13_HS_SERVER_HELLO;
  *p++ = 0;
  *p++ = 0;
  *p++ = 38; /* body length */

  /* Body */
  *p++ = 0x03;
  *p++ = 0x03; /* version */
  FIO_MEMCPY(p, hrr_random, 32);
  p += 32;
  *p++ = 0;    /* session_id_len */
  *p++ = 0x13; /* cipher suite */
  *p++ = 0x01;
  *p++ = 0; /* compression */

  size_t hrr_msg_len = (size_t)(p - hrr_msg);

  /* Try to handle second HRR - should fail */
  int result = fio___tls13_handle_hello_retry_request(&client,
                                                      &sh,
                                                      hrr_msg,
                                                      hrr_msg_len);

  FIO_ASSERT(result == -1, "Second HRR should be rejected");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_ERROR,
             "Client should be in error state");
  FIO_ASSERT(client.alert_description == FIO_TLS13_ALERT_UNEXPECTED_MESSAGE,
             "Alert should be unexpected_message");

  fio_tls13_client_destroy(&client);

  FIO_LOG_DDEBUG("  PASS: Second HRR rejection tests");
}

/* *****************************************************************************
Test: HRR No Change Detection
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_hrr_no_change(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 HRR no-change detection...");

  /* Initialize a client */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Client offers X25519 first, so if server selects X25519,
   * it wouldn't change anything (illegal_parameter) */

  static const uint8_t hrr_random[32] = {
      0xCF, 0x21, 0xAD, 0x74, 0xE5, 0x9A, 0x61, 0x11, 0xBE, 0x1D, 0x8C,
      0x02, 0x1E, 0x65, 0xB8, 0x91, 0xC2, 0xA2, 0x11, 0x16, 0x7A, 0xBB,
      0x8C, 0x5E, 0x07, 0x9E, 0x09, 0xE2, 0xC8, 0xA8, 0x33, 0x9C};

  fio_tls13_server_hello_s sh;
  FIO_MEMSET(&sh, 0, sizeof(sh));
  FIO_MEMCPY(sh.random, hrr_random, 32);
  sh.is_hello_retry_request = 1;
  sh.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  sh.key_share_group = FIO_TLS13_GROUP_X25519; /* Same as what we offered */

  /* Build minimal HRR message */
  uint8_t hrr_msg[64];
  uint8_t *p = hrr_msg;

  /* Handshake header */
  *p++ = FIO_TLS13_HS_SERVER_HELLO;
  *p++ = 0;
  *p++ = 0;
  *p++ = 38;

  /* Body */
  *p++ = 0x03;
  *p++ = 0x03;
  FIO_MEMCPY(p, hrr_random, 32);
  p += 32;
  *p++ = 0;
  *p++ = 0x13;
  *p++ = 0x01;
  *p++ = 0;

  size_t hrr_msg_len = (size_t)(p - hrr_msg);

  /* Try to handle HRR that wouldn't change anything - should fail */
  int result = fio___tls13_handle_hello_retry_request(&client,
                                                      &sh,
                                                      hrr_msg,
                                                      hrr_msg_len);

  FIO_ASSERT(result == -1, "HRR with no change should be rejected");
  FIO_ASSERT(client.state == FIO_TLS13_STATE_ERROR,
             "Client should be in error state");
  FIO_ASSERT(client.alert_description == FIO_TLS13_ALERT_ILLEGAL_PARAMETER,
             "Alert should be illegal_parameter");

  fio_tls13_client_destroy(&client);

  FIO_LOG_DDEBUG("  PASS: HRR no-change detection tests");
}

/* *****************************************************************************
Test: Cookie Handling
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_hrr_cookie(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 HRR cookie handling...");

  /* Initialize a client */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Add some data to transcript (simulating ClientHello1) */
  uint8_t fake_ch1[100];
  fio_rand_bytes(fake_ch1, sizeof(fake_ch1));
  fio___tls13_transcript_update(&client, fake_ch1, sizeof(fake_ch1));

  static const uint8_t hrr_random[32] = {
      0xCF, 0x21, 0xAD, 0x74, 0xE5, 0x9A, 0x61, 0x11, 0xBE, 0x1D, 0x8C,
      0x02, 0x1E, 0x65, 0xB8, 0x91, 0xC2, 0xA2, 0x11, 0x16, 0x7A, 0xBB,
      0x8C, 0x5E, 0x07, 0x9E, 0x09, 0xE2, 0xC8, 0xA8, 0x33, 0x9C};

  /* Build HRR with cookie extension */
  uint8_t hrr_msg[256];
  uint8_t *p = hrr_msg;

  /* Handshake header - will fill in length later */
  *p++ = FIO_TLS13_HS_SERVER_HELLO;
  uint8_t *len_ptr = p;
  p += 3;

  uint8_t *body_start = p;

  /* Body */
  *p++ = 0x03;
  *p++ = 0x03; /* version */
  FIO_MEMCPY(p, hrr_random, 32);
  p += 32;
  *p++ = 0;    /* session_id_len */
  *p++ = 0x13; /* cipher suite */
  *p++ = 0x01;
  *p++ = 0; /* compression */

  /* Extensions */
  uint8_t *ext_len_ptr = p;
  p += 2;
  uint8_t *ext_start = p;

  /* supported_versions extension */
  *p++ = 0x00;
  *p++ = 0x2B;
  *p++ = 0x00;
  *p++ = 0x02;
  *p++ = 0x03;
  *p++ = 0x04;

  /* key_share extension - P-256 */
  *p++ = 0x00;
  *p++ = 0x33;
  *p++ = 0x00;
  *p++ = 0x02;
  *p++ = 0x00;
  *p++ = 0x17; /* secp256r1 */

  /* cookie extension */
  uint8_t test_cookie[] = "test-cookie-data-12345";
  size_t cookie_len = sizeof(test_cookie) - 1;

  *p++ = 0x00;
  *p++ = 0x2C; /* cookie extension type (44) */
  /* Extension data length: cookie_len(2) + cookie */
  *p++ = (uint8_t)((2 + cookie_len) >> 8);
  *p++ = (uint8_t)((2 + cookie_len) & 0xFF);
  /* Cookie length */
  *p++ = (uint8_t)(cookie_len >> 8);
  *p++ = (uint8_t)(cookie_len & 0xFF);
  /* Cookie data */
  FIO_MEMCPY(p, test_cookie, cookie_len);
  p += cookie_len;

  /* Write extensions length */
  size_t ext_len = (size_t)(p - ext_start);
  ext_len_ptr[0] = (uint8_t)(ext_len >> 8);
  ext_len_ptr[1] = (uint8_t)(ext_len & 0xFF);

  /* Write body length */
  size_t body_len = (size_t)(p - body_start);
  len_ptr[0] = (uint8_t)((body_len >> 16) & 0xFF);
  len_ptr[1] = (uint8_t)((body_len >> 8) & 0xFF);
  len_ptr[2] = (uint8_t)(body_len & 0xFF);

  size_t hrr_msg_len = (size_t)(p - hrr_msg);

  fio_tls13_server_hello_s sh;
  FIO_MEMSET(&sh, 0, sizeof(sh));
  FIO_MEMCPY(sh.random, hrr_random, 32);
  sh.is_hello_retry_request = 1;
  sh.cipher_suite = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  sh.key_share_group = FIO_TLS13_GROUP_SECP256R1;

  /* Handle HRR */
  int result = fio___tls13_handle_hello_retry_request(&client,
                                                      &sh,
                                                      hrr_msg,
                                                      hrr_msg_len);

#if defined(H___FIO_P256___H)
  FIO_ASSERT(result == 1, "HRR with cookie should succeed");
  FIO_ASSERT(client.hrr_received == 1, "hrr_received should be set");
  FIO_ASSERT(client.hrr_cookie != NULL, "Cookie should be stored");
  FIO_ASSERT(client.hrr_cookie_len == cookie_len, "Cookie length should match");
  FIO_ASSERT(FIO_MEMCMP(client.hrr_cookie, test_cookie, cookie_len) == 0,
             "Cookie data should match");
#else
  /* P-256 not available, HRR should fail */
  FIO_ASSERT(result == -1, "HRR should fail without P-256 support");
#endif

  fio_tls13_client_destroy(&client);

  FIO_LOG_DDEBUG("  PASS: HRR cookie handling tests");
}

/* *****************************************************************************
Test: ClientHello2 Building
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_client_hello2(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 ClientHello2 building...");

  /* Initialize a client */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Simulate HRR received */
  client.hrr_received = 1;
  client.hrr_selected_group = FIO_TLS13_GROUP_X25519;

  /* Build ClientHello2 */
  uint8_t out[1024];
  int len = fio___tls13_build_client_hello2(&client, out, sizeof(out));

  FIO_ASSERT(len > 0, "ClientHello2 build should succeed");
  FIO_ASSERT(len > 100, "ClientHello2 should be substantial");

  /* Verify record header */
  FIO_ASSERT(out[0] == FIO_TLS13_CONTENT_HANDSHAKE,
             "Record type should be handshake");
  FIO_ASSERT(out[1] == 0x03 && out[2] == 0x03,
             "Legacy version should be TLS 1.2");

  /* Verify handshake header */
  FIO_ASSERT(out[5] == FIO_TLS13_HS_CLIENT_HELLO,
             "Handshake type should be ClientHello");

  /* Verify random is same as original */
  FIO_ASSERT(FIO_MEMCMP(out + 5 + 4 + 2, client.client_random, 32) == 0,
             "Random should be same as original");

  fio_tls13_client_destroy(&client);

  FIO_LOG_DDEBUG("  PASS: ClientHello2 building tests");
}

/* *****************************************************************************
Test: ClientHello2 with Cookie
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_client_hello2_cookie(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 ClientHello2 with cookie...");

  /* Initialize a client */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "example.com");

  /* Simulate HRR received with cookie */
  client.hrr_received = 1;
  client.hrr_selected_group = FIO_TLS13_GROUP_X25519;

  uint8_t test_cookie[] = "test-cookie-data";
  size_t cookie_len = sizeof(test_cookie) - 1;
  client.hrr_cookie = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, cookie_len, 0);
  FIO_MEMCPY(client.hrr_cookie, test_cookie, cookie_len);
  client.hrr_cookie_len = cookie_len;

  /* Build ClientHello2 */
  uint8_t out[1024];
  int len = fio___tls13_build_client_hello2(&client, out, sizeof(out));

  FIO_ASSERT(len > 0, "ClientHello2 with cookie should succeed");

  /* Search for cookie extension in the output */
  int found_cookie = 0;
  for (size_t i = 0; i < (size_t)len - cookie_len; ++i) {
    if (FIO_MEMCMP(out + i, test_cookie, cookie_len) == 0) {
      found_cookie = 1;
      break;
    }
  }
  FIO_ASSERT(found_cookie, "ClientHello2 should contain the cookie");

  fio_tls13_client_destroy(&client);

  FIO_LOG_DDEBUG("  PASS: ClientHello2 with cookie tests");
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  FIO_LOG_DDEBUG("=== TLS 1.3 HelloRetryRequest Tests ===\n");

  fio___test_tls13_hrr_detection();
  fio___test_tls13_hrr_transcript();
  fio___test_tls13_hrr_second_rejection();
  fio___test_tls13_hrr_no_change();
  fio___test_tls13_hrr_cookie();
  fio___test_tls13_client_hello2();
  fio___test_tls13_client_hello2_cookie();

  FIO_LOG_DDEBUG("\n=== All TLS 1.3 HRR Tests PASSED ===");
  return 0;
}
