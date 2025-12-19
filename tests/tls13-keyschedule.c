/* *****************************************************************************
TLS 1.3 Key Schedule Tests - RFC 8448 Test Vectors
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPT
#define FIO_TLS13
#include FIO_INCLUDE_FILE

/* *****************************************************************************
RFC 8448 Test Vectors - Simple 1-RTT Handshake
https://www.rfc-editor.org/rfc/rfc8448#section-3

These test vectors are from the "Simple 1-RTT Handshake" example.
***************************************************************************** */

/* Helper to print hex for debugging */
FIO_SFUNC void fio___test_print_hex(const char *label,
                                    const uint8_t *data,
                                    size_t len) {
  fprintf(stderr, "%s: ", label);
  for (size_t i = 0; i < len; ++i)
    fprintf(stderr, "%02x", data[i]);
  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test HKDF-Expand-Label basic functionality
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_hkdf_expand_label_basic(void) {
  /* Test that HKDF-Expand-Label produces non-zero output */
  uint8_t secret[32];
  uint8_t output[32];
  uint8_t zero[32] = {0};

  /* Fill secret with test data */
  for (size_t i = 0; i < 32; ++i)
    secret[i] = (uint8_t)i;

  fio_tls13_hkdf_expand_label(output, 32, secret, 32, "test", 4, NULL, 0, 0);

  FIO_ASSERT(FIO_MEMCMP(output, zero, 32) != 0,
             "HKDF-Expand-Label produced zero output");

  FIO_LOG_DDEBUG("TLS 1.3 HKDF-Expand-Label basic test: PASSED");
}

/* *****************************************************************************
Test Derive-Secret basic functionality
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_derive_secret_basic(void) {
  uint8_t secret[32];
  uint8_t output[32];
  uint8_t zero[32] = {0};

  /* Fill secret with test data */
  for (size_t i = 0; i < 32; ++i)
    secret[i] = (uint8_t)i;

  fio_tls13_derive_secret(output, secret, 32, "derived", 7, NULL, 0, 0);

  FIO_ASSERT(FIO_MEMCMP(output, zero, 32) != 0,
             "Derive-Secret produced zero output");

  FIO_LOG_DDEBUG("TLS 1.3 Derive-Secret basic test: PASSED");
}

/* *****************************************************************************
RFC 8448 Section 3 - Simple 1-RTT Handshake Test Vectors

These are the actual test vectors from RFC 8448 for TLS_AES_128_GCM_SHA256.
***************************************************************************** */

/* ECDHE shared secret (from X25519 key exchange) */
/* clang-format off */
static const uint8_t rfc8448_ecdhe_secret[32] = {
    0x8b, 0xd4, 0x05, 0x4f, 0xb5, 0x5b, 0x9d, 0x63,
    0xfd, 0xfb, 0xac, 0xf9, 0xf0, 0x4b, 0x9f, 0x0d,
    0x35, 0xe6, 0xd6, 0x3f, 0x53, 0x75, 0x63, 0xef,
    0xd4, 0x62, 0x72, 0x90, 0x0f, 0x89, 0x49, 0x2d
};

/* Hash of ClientHello...ServerHello (transcript hash at ServerHello) */
static const uint8_t rfc8448_hello_hash[32] = {
    0x86, 0x0c, 0x06, 0xed, 0xc0, 0x78, 0x58, 0xee,
    0x8e, 0x78, 0xf0, 0xe7, 0x42, 0x8c, 0x58, 0xed,
    0xd6, 0xb4, 0x3f, 0x2c, 0xa3, 0xe6, 0xe9, 0x5f,
    0x02, 0xed, 0x06, 0x3c, 0xf0, 0xe1, 0xca, 0xd8
};

/* Expected handshake secret */
static const uint8_t rfc8448_handshake_secret[32] = {
    0x1d, 0xc8, 0x26, 0xe9, 0x36, 0x06, 0xaa, 0x6f,
    0xdc, 0x0a, 0xad, 0xc1, 0x2f, 0x74, 0x1b, 0x01,
    0x04, 0x6a, 0xa6, 0xb9, 0x9f, 0x69, 0x1e, 0xd2,
    0x21, 0xa9, 0xf0, 0xca, 0x04, 0x3f, 0xbe, 0xac
};

/* Expected client handshake traffic secret */
static const uint8_t rfc8448_client_hs_traffic_secret[32] = {
    0xb3, 0xed, 0xdb, 0x12, 0x6e, 0x06, 0x7f, 0x35,
    0xa7, 0x80, 0xb3, 0xab, 0xf4, 0x5e, 0x2d, 0x8f,
    0x3b, 0x1a, 0x95, 0x07, 0x38, 0xf5, 0x2e, 0x96,
    0x00, 0x74, 0x6a, 0x0e, 0x27, 0xa5, 0x5a, 0x21
};

/* Expected server handshake traffic secret */
static const uint8_t rfc8448_server_hs_traffic_secret[32] = {
    0xb6, 0x7b, 0x7d, 0x69, 0x0c, 0xc1, 0x6c, 0x4e,
    0x75, 0xe5, 0x42, 0x13, 0xcb, 0x2d, 0x37, 0xb4,
    0xe9, 0xc9, 0x12, 0xbc, 0xde, 0xd9, 0x10, 0x5d,
    0x42, 0xbe, 0xfd, 0x59, 0xd3, 0x91, 0xad, 0x38
};

/* Expected client handshake write key */
static const uint8_t rfc8448_client_hs_key[16] = {
    0xdb, 0xfa, 0xa6, 0x93, 0xd1, 0x76, 0x2c, 0x5b,
    0x66, 0x6a, 0xf5, 0xd9, 0x50, 0x25, 0x8d, 0x01
};

/* Expected client handshake write IV */
static const uint8_t rfc8448_client_hs_iv[12] = {
    0x5b, 0xd3, 0xc7, 0x1b, 0x83, 0x6e, 0x0b, 0x76,
    0xbb, 0x73, 0x26, 0x5f
};

/* Expected server handshake write key */
static const uint8_t rfc8448_server_hs_key[16] = {
    0x3f, 0xce, 0x51, 0x60, 0x09, 0xc2, 0x17, 0x27,
    0xd0, 0xf2, 0xe4, 0xe8, 0x6e, 0xe4, 0x03, 0xbc
};

/* Expected server handshake write IV */
static const uint8_t rfc8448_server_hs_iv[12] = {
    0x5d, 0x31, 0x3e, 0xb2, 0x67, 0x12, 0x76, 0xee,
    0x13, 0x00, 0x0b, 0x30
};

/* Hash of ClientHello...server Finished (for application secrets) */
static const uint8_t rfc8448_server_finished_hash[32] = {
    0x96, 0x08, 0x10, 0x2a, 0x0f, 0x1c, 0xcc, 0x6d,
    0xb6, 0x25, 0x0b, 0x7b, 0x7e, 0x41, 0x7b, 0x1a,
    0x00, 0x0e, 0xaa, 0xda, 0x3d, 0xaa, 0xe4, 0x77,
    0x7a, 0x76, 0x86, 0xc9, 0xff, 0x83, 0xdf, 0x13
};

/* Expected master secret */
static const uint8_t rfc8448_master_secret[32] = {
    0x18, 0xdf, 0x06, 0x84, 0x3d, 0x13, 0xa0, 0x8b,
    0xf2, 0xa4, 0x49, 0x84, 0x4c, 0x5f, 0x8a, 0x47,
    0x80, 0x01, 0xbc, 0x4d, 0x4c, 0x62, 0x79, 0x84,
    0xd5, 0xa4, 0x1d, 0xa8, 0xd0, 0x40, 0x29, 0x19
};

/* Expected client application traffic secret */
static const uint8_t rfc8448_client_app_traffic_secret[32] = {
    0x9e, 0x40, 0x64, 0x6c, 0xe7, 0x9a, 0x7f, 0x9d,
    0xc0, 0x5a, 0xf8, 0x88, 0x9b, 0xce, 0x65, 0x52,
    0x87, 0x5a, 0xfa, 0x0b, 0x06, 0xdf, 0x00, 0x87,
    0xf7, 0x92, 0xeb, 0xb7, 0xc1, 0x75, 0x04, 0xa5
};

/* Expected server application traffic secret */
static const uint8_t rfc8448_server_app_traffic_secret[32] = {
    0xa1, 0x1a, 0xf9, 0xf0, 0x55, 0x31, 0xf8, 0x56,
    0xad, 0x47, 0x11, 0x6b, 0x45, 0xa9, 0x50, 0x32,
    0x82, 0x04, 0xb4, 0xf4, 0x4b, 0xfb, 0x6b, 0x3a,
    0x4b, 0x4f, 0x1f, 0x3f, 0xcb, 0x63, 0x16, 0x43
};

/* Expected client application write key */
static const uint8_t rfc8448_client_app_key[16] = {
    0x17, 0x42, 0x2d, 0xda, 0x59, 0x6e, 0xd5, 0xd9,
    0xac, 0xd8, 0x90, 0xe3, 0xc6, 0x3f, 0x50, 0x51
};

/* Expected client application write IV */
static const uint8_t rfc8448_client_app_iv[12] = {
    0x5b, 0x78, 0x92, 0x3d, 0xee, 0x08, 0x57, 0x90,
    0x33, 0xe5, 0x23, 0xd9
};

/* Expected server application write key */
static const uint8_t rfc8448_server_app_key[16] = {
    0x9f, 0x02, 0x28, 0x3b, 0x6c, 0x9c, 0x07, 0xef,
    0xc2, 0x6b, 0xb9, 0xf2, 0xac, 0x92, 0xe3, 0x56
};

/* Expected server application write IV */
static const uint8_t rfc8448_server_app_iv[12] = {
    0xcf, 0x78, 0x2b, 0x88, 0xdd, 0x83, 0x54, 0x9a,
    0xad, 0xf1, 0xe9, 0x84
};
/* clang-format on */

/* *****************************************************************************
Test full key schedule with RFC 8448 vectors
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_rfc8448_key_schedule(void) {
  uint8_t early_secret[32];
  uint8_t handshake_secret[32];
  uint8_t master_secret[32];
  uint8_t client_hs_traffic[32];
  uint8_t server_hs_traffic[32];
  uint8_t client_app_traffic[32];
  uint8_t server_app_traffic[32];
  uint8_t key[32];
  uint8_t iv[12];

  /* Step 1: Derive Early Secret (no PSK) */
  fio_tls13_derive_early_secret(early_secret, NULL, 0, 0);

  /* Step 2: Derive Handshake Secret */
  fio_tls13_derive_handshake_secret(handshake_secret,
                                    early_secret,
                                    rfc8448_ecdhe_secret,
                                    32,
                                    0);

  FIO_ASSERT(!FIO_MEMCMP(handshake_secret, rfc8448_handshake_secret, 32),
             "Handshake secret mismatch");
  FIO_LOG_DDEBUG("  Handshake Secret: PASSED");

  /* Step 3: Derive client handshake traffic secret */
  fio_tls13_derive_secret(client_hs_traffic,
                          handshake_secret,
                          32,
                          "c hs traffic",
                          12,
                          rfc8448_hello_hash,
                          32,
                          0);

  FIO_ASSERT(
      !FIO_MEMCMP(client_hs_traffic, rfc8448_client_hs_traffic_secret, 32),
      "Client handshake traffic secret mismatch");
  FIO_LOG_DDEBUG("  Client HS Traffic Secret: PASSED");

  /* Step 4: Derive server handshake traffic secret */
  fio_tls13_derive_secret(server_hs_traffic,
                          handshake_secret,
                          32,
                          "s hs traffic",
                          12,
                          rfc8448_hello_hash,
                          32,
                          0);

  FIO_ASSERT(
      !FIO_MEMCMP(server_hs_traffic, rfc8448_server_hs_traffic_secret, 32),
      "Server handshake traffic secret mismatch");
  FIO_LOG_DDEBUG("  Server HS Traffic Secret: PASSED");

  /* Step 5: Derive client handshake keys */
  fio_tls13_derive_traffic_keys(key, 16, iv, client_hs_traffic, 0);

  FIO_ASSERT(!FIO_MEMCMP(key, rfc8448_client_hs_key, 16),
             "Client handshake key mismatch");
  FIO_ASSERT(!FIO_MEMCMP(iv, rfc8448_client_hs_iv, 12),
             "Client handshake IV mismatch");
  FIO_LOG_DDEBUG("  Client HS Key/IV: PASSED");

  /* Step 6: Derive server handshake keys */
  fio_tls13_derive_traffic_keys(key, 16, iv, server_hs_traffic, 0);

  FIO_ASSERT(!FIO_MEMCMP(key, rfc8448_server_hs_key, 16),
             "Server handshake key mismatch");
  FIO_ASSERT(!FIO_MEMCMP(iv, rfc8448_server_hs_iv, 12),
             "Server handshake IV mismatch");
  FIO_LOG_DDEBUG("  Server HS Key/IV: PASSED");

  /* Step 7: Derive Master Secret */
  fio_tls13_derive_master_secret(master_secret, handshake_secret, 0);

  FIO_ASSERT(!FIO_MEMCMP(master_secret, rfc8448_master_secret, 32),
             "Master secret mismatch");
  FIO_LOG_DDEBUG("  Master Secret: PASSED");

  /* Step 8: Derive client application traffic secret */
  fio_tls13_derive_secret(client_app_traffic,
                          master_secret,
                          32,
                          "c ap traffic",
                          12,
                          rfc8448_server_finished_hash,
                          32,
                          0);

  FIO_ASSERT(
      !FIO_MEMCMP(client_app_traffic, rfc8448_client_app_traffic_secret, 32),
      "Client application traffic secret mismatch");
  FIO_LOG_DDEBUG("  Client App Traffic Secret: PASSED");

  /* Step 9: Derive server application traffic secret */
  fio_tls13_derive_secret(server_app_traffic,
                          master_secret,
                          32,
                          "s ap traffic",
                          12,
                          rfc8448_server_finished_hash,
                          32,
                          0);

  FIO_ASSERT(
      !FIO_MEMCMP(server_app_traffic, rfc8448_server_app_traffic_secret, 32),
      "Server application traffic secret mismatch");
  FIO_LOG_DDEBUG("  Server App Traffic Secret: PASSED");

  /* Step 10: Derive client application keys */
  fio_tls13_derive_traffic_keys(key, 16, iv, client_app_traffic, 0);

  FIO_ASSERT(!FIO_MEMCMP(key, rfc8448_client_app_key, 16),
             "Client application key mismatch");
  FIO_ASSERT(!FIO_MEMCMP(iv, rfc8448_client_app_iv, 12),
             "Client application IV mismatch");
  FIO_LOG_DDEBUG("  Client App Key/IV: PASSED");

  /* Step 11: Derive server application keys */
  fio_tls13_derive_traffic_keys(key, 16, iv, server_app_traffic, 0);

  FIO_ASSERT(!FIO_MEMCMP(key, rfc8448_server_app_key, 16),
             "Server application key mismatch");
  FIO_ASSERT(!FIO_MEMCMP(iv, rfc8448_server_app_iv, 12),
             "Server application IV mismatch");
  FIO_LOG_DDEBUG("  Server App Key/IV: PASSED");

  FIO_LOG_DDEBUG("TLS 1.3 RFC 8448 Key Schedule: ALL PASSED");
}

/* *****************************************************************************
Test Finished key derivation
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_finished_key(void) {
  uint8_t finished_key[32];
  uint8_t zero[32] = {0};

  /* Derive finished key from client handshake traffic secret */
  fio_tls13_derive_finished_key(finished_key,
                                rfc8448_client_hs_traffic_secret,
                                0);

  FIO_ASSERT(FIO_MEMCMP(finished_key, zero, 32) != 0, "Finished key is zero");

  /* Derive finished key from server handshake traffic secret */
  fio_tls13_derive_finished_key(finished_key,
                                rfc8448_server_hs_traffic_secret,
                                0);

  FIO_ASSERT(FIO_MEMCMP(finished_key, zero, 32) != 0,
             "Server finished key is zero");

  FIO_LOG_DDEBUG("TLS 1.3 Finished Key derivation: PASSED");
}

/* *****************************************************************************
Test traffic secret update (key update)
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_traffic_update(void) {
  uint8_t new_secret[32];
  uint8_t zero[32] = {0};

  /* Update client application traffic secret */
  fio_tls13_update_traffic_secret(new_secret,
                                  rfc8448_client_app_traffic_secret,
                                  0);

  FIO_ASSERT(FIO_MEMCMP(new_secret, zero, 32) != 0,
             "Updated traffic secret is zero");
  FIO_ASSERT(FIO_MEMCMP(new_secret, rfc8448_client_app_traffic_secret, 32) != 0,
             "Updated traffic secret unchanged");

  FIO_LOG_DDEBUG("TLS 1.3 Traffic Secret Update: PASSED");
}

/* *****************************************************************************
Test SHA-384 variant (for TLS_AES_256_GCM_SHA384)
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_sha384_variant(void) {
  uint8_t early_secret[48];
  uint8_t handshake_secret[48];
  uint8_t master_secret[48];
  uint8_t traffic_secret[48];
  uint8_t key[32];
  uint8_t iv[12];
  uint8_t zero48[48] = {0};
  uint8_t zero32[32] = {0};
  uint8_t zero12[12] = {0};

  /* Fake ECDHE secret for testing */
  uint8_t ecdhe[48];
  for (size_t i = 0; i < 48; ++i)
    ecdhe[i] = (uint8_t)i;

  /* Fake transcript hash */
  uint8_t hash[48];
  for (size_t i = 0; i < 48; ++i)
    hash[i] = (uint8_t)(0x80 + i);

  /* Test early secret derivation */
  fio_tls13_derive_early_secret(early_secret, NULL, 0, 1);
  FIO_ASSERT(FIO_MEMCMP(early_secret, zero48, 48) != 0,
             "SHA-384 early secret is zero");

  /* Test handshake secret derivation */
  fio_tls13_derive_handshake_secret(handshake_secret,
                                    early_secret,
                                    ecdhe,
                                    48,
                                    1);
  FIO_ASSERT(FIO_MEMCMP(handshake_secret, zero48, 48) != 0,
             "SHA-384 handshake secret is zero");

  /* Test master secret derivation */
  fio_tls13_derive_master_secret(master_secret, handshake_secret, 1);
  FIO_ASSERT(FIO_MEMCMP(master_secret, zero48, 48) != 0,
             "SHA-384 master secret is zero");

  /* Test traffic secret derivation */
  fio_tls13_derive_secret(traffic_secret,
                          handshake_secret,
                          48,
                          "c hs traffic",
                          12,
                          hash,
                          48,
                          1);
  FIO_ASSERT(FIO_MEMCMP(traffic_secret, zero48, 48) != 0,
             "SHA-384 traffic secret is zero");

  /* Test traffic key derivation (AES-256 = 32 byte key) */
  fio_tls13_derive_traffic_keys(key, 32, iv, traffic_secret, 1);
  FIO_ASSERT(FIO_MEMCMP(key, zero32, 32) != 0, "SHA-384 traffic key is zero");
  FIO_ASSERT(FIO_MEMCMP(iv, zero12, 12) != 0, "SHA-384 traffic IV is zero");

  FIO_LOG_DDEBUG("TLS 1.3 SHA-384 variant: PASSED");
}

/* *****************************************************************************
Test determinism - same inputs produce same outputs
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_determinism(void) {
  uint8_t secret1[32], secret2[32];
  uint8_t key1[16], key2[16];
  uint8_t iv1[12], iv2[12];

  /* Derive handshake secret twice */
  uint8_t early[32];
  fio_tls13_derive_early_secret(early, NULL, 0, 0);

  fio_tls13_derive_handshake_secret(secret1,
                                    early,
                                    rfc8448_ecdhe_secret,
                                    32,
                                    0);
  fio_tls13_derive_handshake_secret(secret2,
                                    early,
                                    rfc8448_ecdhe_secret,
                                    32,
                                    0);

  FIO_ASSERT(!FIO_MEMCMP(secret1, secret2, 32),
             "Handshake secret not deterministic");

  /* Derive traffic keys twice */
  fio_tls13_derive_traffic_keys(key1, 16, iv1, secret1, 0);
  fio_tls13_derive_traffic_keys(key2, 16, iv2, secret1, 0);

  FIO_ASSERT(!FIO_MEMCMP(key1, key2, 16), "Traffic key not deterministic");
  FIO_ASSERT(!FIO_MEMCMP(iv1, iv2, 12), "Traffic IV not deterministic");

  FIO_LOG_DDEBUG("TLS 1.3 Determinism: PASSED");
}

/* *****************************************************************************
Test edge cases
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_edge_cases(void) {
  uint8_t output[48];
  uint8_t secret[48];
  uint8_t zero[48] = {0};

  /* Fill secret */
  for (size_t i = 0; i < 48; ++i)
    secret[i] = (uint8_t)i;

  /* Test with empty label */
  fio_tls13_hkdf_expand_label(output, 32, secret, 32, "", 0, NULL, 0, 0);
  FIO_ASSERT(FIO_MEMCMP(output, zero, 32) != 0,
             "Empty label produced zero output");

  /* Test with maximum output length (255) */
  uint8_t large_output[255];
  uint8_t large_zero[255] = {0};
  fio_tls13_hkdf_expand_label(large_output,
                              255,
                              secret,
                              32,
                              "test",
                              4,
                              NULL,
                              0,
                              0);
  FIO_ASSERT(FIO_MEMCMP(large_output, large_zero, 255) != 0,
             "Max length output is zero");

  /* Test with context */
  uint8_t context[32];
  for (size_t i = 0; i < 32; ++i)
    context[i] = (uint8_t)(0xAA + i);

  fio_tls13_hkdf_expand_label(output,
                              32,
                              secret,
                              32,
                              "test",
                              4,
                              context,
                              32,
                              0);
  FIO_ASSERT(FIO_MEMCMP(output, zero, 32) != 0, "Output with context is zero");

  FIO_LOG_DDEBUG("TLS 1.3 Edge Cases: PASSED");
}

/* *****************************************************************************
Main test runner
***************************************************************************** */
int main(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 Key Schedule (RFC 8446 Section 7.1)...\n");

  FIO_LOG_DDEBUG("=== Basic Functionality Tests ===");
  fio___test_tls13_hkdf_expand_label_basic();
  fio___test_tls13_derive_secret_basic();

  FIO_LOG_DDEBUG("=== RFC 8448 Test Vectors ===");
  fio___test_tls13_rfc8448_key_schedule();

  FIO_LOG_DDEBUG("=== Finished Key Tests ===");
  fio___test_tls13_finished_key();

  FIO_LOG_DDEBUG("=== Traffic Update Tests ===");
  fio___test_tls13_traffic_update();

  FIO_LOG_DDEBUG("=== SHA-384 Variant Tests ===");
  fio___test_tls13_sha384_variant();

  FIO_LOG_DDEBUG("=== Determinism Tests ===");
  fio___test_tls13_determinism();

  FIO_LOG_DDEBUG("=== Edge Case Tests ===");
  fio___test_tls13_edge_cases();

  FIO_LOG_DDEBUG("\nAll TLS 1.3 Key Schedule tests passed!");
  return 0;
}
