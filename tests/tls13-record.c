/* *****************************************************************************
Test - TLS 1.3 Record Layer (RFC 8446 Section 5)
***************************************************************************** */
#include "test-helpers.h"

/* TLS13 requires HKDF which requires SHA2 */
#define FIO_SHA2
#define FIO_HKDF
#define FIO_AES
#define FIO_CHACHA
#define FIO_TLS13
#include FIO_INCLUDE_FILE

/* Helper to print hex for debugging */
FIO_SFUNC void print_hex(const char *label, const uint8_t *data, size_t len) {
  fprintf(stderr, "%s: ", label);
  for (size_t i = 0; i < len; ++i)
    fprintf(stderr, "%02x", data[i]);
  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test: Nonce Construction (RFC 8446 Section 5.3)
***************************************************************************** */
FIO_SFUNC void test_nonce_construction(void) {
  fprintf(stderr, "\t* Testing nonce construction\n");

  /* Test case: IV and sequence number from RFC 8446 examples */
  uint8_t iv[12] =
      {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
  uint8_t nonce[12];

  /* Sequence number 0 - nonce should equal IV */
  fio_tls13_build_nonce(nonce, iv, 0);
  FIO_ASSERT(!memcmp(nonce, iv, 12), "Nonce with seq=0 should equal IV");

  /* Sequence number 1 - last byte should be XORed */
  fio_tls13_build_nonce(nonce, iv, 1);
  FIO_ASSERT(nonce[11] == (iv[11] ^ 1),
             "Nonce with seq=1: last byte should be IV[11] XOR 1");
  FIO_ASSERT(!memcmp(nonce, iv, 11),
             "Nonce with seq=1: first 11 bytes should match IV");

  /* Sequence number 0x0102030405060708 - test full 64-bit XOR */
  uint64_t seq = 0x0102030405060708ULL;
  fio_tls13_build_nonce(nonce, iv, seq);

  /* Expected: IV XOR (0x00 0x00 0x00 0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07
   * 0x08) */
  uint8_t expected[12] = {0x00,
                          0x01,
                          0x02,
                          0x03,
                          0x04 ^ 0x01,
                          0x05 ^ 0x02,
                          0x06 ^ 0x03,
                          0x07 ^ 0x04,
                          0x08 ^ 0x05,
                          0x09 ^ 0x06,
                          0x0a ^ 0x07,
                          0x0b ^ 0x08};
  FIO_ASSERT(!memcmp(nonce, expected, 12),
             "Nonce construction with 64-bit sequence number failed");

  fprintf(stderr, "\t  - Nonce construction tests passed\n");
}

/* *****************************************************************************
Test: Record Header Parsing
***************************************************************************** */
FIO_SFUNC void test_header_parsing(void) {
  fprintf(stderr, "\t* Testing record header parsing\n");

  /* Valid application data header */
  uint8_t valid_header[] = {0x17, 0x03, 0x03, 0x00, 0x20}; /* type=23, len=32 */
  uint8_t payload[32] = {0};
  uint8_t record[37];
  FIO_MEMCPY(record, valid_header, 5);
  FIO_MEMCPY(record + 5, payload, 32);

  fio_tls13_content_type_e ct;
  size_t payload_len;
  const uint8_t *p =
      fio_tls13_record_parse_header(record, 37, &ct, &payload_len);

  FIO_ASSERT(p != NULL, "Valid header should parse successfully");
  FIO_ASSERT(ct == FIO_TLS13_CONTENT_APPLICATION_DATA,
             "Content type should be application_data (23)");
  FIO_ASSERT(payload_len == 32, "Payload length should be 32");
  FIO_ASSERT(p == record + 5, "Payload pointer should point after header");

  /* Incomplete header */
  p = fio_tls13_record_parse_header(record, 4, &ct, &payload_len);
  FIO_ASSERT(p == NULL, "Incomplete header should return NULL");

  /* Incomplete payload */
  p = fio_tls13_record_parse_header(record, 20, &ct, &payload_len);
  FIO_ASSERT(p == NULL, "Incomplete payload should return NULL");

  /* Invalid content type */
  uint8_t invalid_type[] = {0x00, 0x03, 0x03, 0x00, 0x10};
  p = fio_tls13_record_parse_header(invalid_type, 5, &ct, &payload_len);
  FIO_ASSERT(p == NULL, "Invalid content type should return NULL");

  /* Test all valid content types */
  uint8_t types[] = {20, 21, 22, 23}; /* CCS, Alert, Handshake, AppData */
  for (size_t i = 0; i < 4; ++i) {
    record[0] = types[i];
    p = fio_tls13_record_parse_header(record, 37, &ct, &payload_len);
    FIO_ASSERT(p != NULL, "Valid content type %d should parse", types[i]);
    FIO_ASSERT(ct == (fio_tls13_content_type_e)types[i],
               "Content type should match");
  }

  fprintf(stderr, "\t  - Header parsing tests passed\n");
}

/* *****************************************************************************
Test: Record Encryption/Decryption Round-trip
***************************************************************************** */
FIO_SFUNC void test_roundtrip(fio_tls13_cipher_type_e cipher,
                              const char *cipher_name,
                              uint8_t key_len) {
  fprintf(stderr, "\t* Testing %s round-trip\n", cipher_name);

  /* Generate random key and IV */
  uint8_t key[32];
  uint8_t iv[12];
  fio_rand_bytes(key, key_len);
  fio_rand_bytes(iv, 12);

  /* Initialize encryption and decryption keys */
  fio_tls13_record_keys_s enc_keys, dec_keys;
  fio_tls13_record_keys_init(&enc_keys, key, key_len, iv, cipher);
  fio_tls13_record_keys_init(&dec_keys, key, key_len, iv, cipher);

  /* Test various plaintext sizes */
  size_t test_sizes[] = {0,
                         1,
                         15,
                         16,
                         17,
                         31,
                         32,
                         33,
                         63,
                         64,
                         65,
                         127,
                         128,
                         129,
                         255,
                         256,
                         1000,
                         16384};
  size_t num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t t = 0; t < num_tests; ++t) {
    size_t pt_len = test_sizes[t];

    /* Generate random plaintext */
    uint8_t *plaintext = NULL;
    if (pt_len > 0) {
      plaintext = (uint8_t *)malloc(pt_len);
      FIO_ASSERT(plaintext, "Failed to allocate plaintext buffer");
      fio_rand_bytes(plaintext, pt_len);
    }

    /* Allocate output buffers */
    size_t max_ct_len =
        FIO_TLS13_RECORD_HEADER_LEN + pt_len + 1 + FIO_TLS13_TAG_LEN;
    uint8_t *ciphertext = (uint8_t *)malloc(max_ct_len);
    uint8_t *decrypted = (uint8_t *)malloc(pt_len + 1);
    FIO_ASSERT(ciphertext && decrypted, "Failed to allocate buffers");

    /* Encrypt */
    int ct_len = fio_tls13_record_encrypt(ciphertext,
                                          max_ct_len,
                                          plaintext,
                                          pt_len,
                                          FIO_TLS13_CONTENT_APPLICATION_DATA,
                                          &enc_keys);
    FIO_ASSERT(ct_len > 0, "Encryption failed for size %zu", pt_len);
    FIO_ASSERT((size_t)ct_len == max_ct_len,
               "Ciphertext length mismatch for size %zu",
               pt_len);

    /* Verify header */
    FIO_ASSERT(ciphertext[0] == FIO_TLS13_CONTENT_APPLICATION_DATA,
               "Outer content type should be application_data");
    FIO_ASSERT(ciphertext[1] == 0x03 && ciphertext[2] == 0x03,
               "Legacy version should be 0x0303");

    /* Decrypt */
    fio_tls13_content_type_e content_type;
    int dec_len = fio_tls13_record_decrypt(decrypted,
                                           pt_len + 1,
                                           &content_type,
                                           ciphertext,
                                           ct_len,
                                           &dec_keys);
    FIO_ASSERT(dec_len >= 0, "Decryption failed for size %zu", pt_len);
    FIO_ASSERT((size_t)dec_len == pt_len,
               "Decrypted length mismatch for size %zu: got %d, expected %zu",
               pt_len,
               dec_len,
               pt_len);
    FIO_ASSERT(content_type == FIO_TLS13_CONTENT_APPLICATION_DATA,
               "Content type mismatch for size %zu",
               pt_len);

    /* Verify plaintext matches */
    if (pt_len > 0) {
      FIO_ASSERT(!memcmp(decrypted, plaintext, pt_len),
                 "Decrypted data mismatch for size %zu",
                 pt_len);
    }

    /* Cleanup */
    free(plaintext);
    free(ciphertext);
    free(decrypted);
  }

  /* Clear keys */
  fio_tls13_record_keys_clear(&enc_keys);
  fio_tls13_record_keys_clear(&dec_keys);

  fprintf(stderr, "\t  - %s round-trip tests passed\n", cipher_name);
}

/* *****************************************************************************
Test: Sequence Number Increment
***************************************************************************** */
FIO_SFUNC void test_sequence_number(void) {
  fprintf(stderr, "\t* Testing sequence number increment\n");

  uint8_t key[16] = {0};
  uint8_t iv[12] = {0};
  fio_tls13_record_keys_s keys;
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  FIO_ASSERT(keys.sequence_number == 0, "Initial sequence number should be 0");

  /* Encrypt a few records and verify sequence number increments */
  uint8_t plaintext[16] = {0};
  uint8_t ciphertext[64];

  for (uint64_t i = 0; i < 5; ++i) {
    FIO_ASSERT(keys.sequence_number == i,
               "Sequence number should be %llu before encrypt",
               (unsigned long long)i);

    int ret = fio_tls13_record_encrypt(ciphertext,
                                       sizeof(ciphertext),
                                       plaintext,
                                       sizeof(plaintext),
                                       FIO_TLS13_CONTENT_APPLICATION_DATA,
                                       &keys);
    FIO_ASSERT(ret > 0, "Encryption should succeed");
    FIO_ASSERT(keys.sequence_number == i + 1,
               "Sequence number should be %llu after encrypt",
               (unsigned long long)(i + 1));
  }

  /* Test decryption sequence number increment */
  fio_tls13_record_keys_s dec_keys;
  fio_tls13_record_keys_init(&dec_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);

  /* Reset encryption keys to match */
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  int ct_len = fio_tls13_record_encrypt(ciphertext,
                                        sizeof(ciphertext),
                                        plaintext,
                                        sizeof(plaintext),
                                        FIO_TLS13_CONTENT_APPLICATION_DATA,
                                        &keys);
  FIO_ASSERT(ct_len > 0, "Encryption should succeed");

  uint8_t decrypted[32];
  fio_tls13_content_type_e ct;
  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         sizeof(decrypted),
                                         &ct,
                                         ciphertext,
                                         ct_len,
                                         &dec_keys);
  FIO_ASSERT(dec_len >= 0, "Decryption should succeed");
  FIO_ASSERT(dec_keys.sequence_number == 1,
             "Decryption sequence number should increment");

  fio_tls13_record_keys_clear(&keys);
  fio_tls13_record_keys_clear(&dec_keys);

  fprintf(stderr, "\t  - Sequence number tests passed\n");
}

/* *****************************************************************************
Test: Authentication Failure Detection
***************************************************************************** */
FIO_SFUNC void test_auth_failure(void) {
  fprintf(stderr, "\t* Testing authentication failure detection\n");

  uint8_t key[16];
  uint8_t iv[12];
  fio_rand_bytes(key, 16);
  fio_rand_bytes(iv, 12);

  fio_tls13_record_keys_s enc_keys, dec_keys;
  fio_tls13_record_keys_init(&enc_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);
  fio_tls13_record_keys_init(&dec_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);

  uint8_t plaintext[32];
  fio_rand_bytes(plaintext, 32);

  uint8_t ciphertext[64];
  int ct_len = fio_tls13_record_encrypt(ciphertext,
                                        sizeof(ciphertext),
                                        plaintext,
                                        32,
                                        FIO_TLS13_CONTENT_APPLICATION_DATA,
                                        &enc_keys);
  FIO_ASSERT(ct_len > 0, "Encryption should succeed");

  /* Corrupt the ciphertext */
  ciphertext[10] ^= 0xFF;

  uint8_t decrypted[64];
  fio_tls13_content_type_e ct;
  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         sizeof(decrypted),
                                         &ct,
                                         ciphertext,
                                         ct_len,
                                         &dec_keys);
  FIO_ASSERT(dec_len == -1, "Decryption should fail with corrupted ciphertext");

  /* Test with wrong key */
  fio_tls13_record_keys_init(&enc_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);
  ct_len = fio_tls13_record_encrypt(ciphertext,
                                    sizeof(ciphertext),
                                    plaintext,
                                    32,
                                    FIO_TLS13_CONTENT_APPLICATION_DATA,
                                    &enc_keys);

  uint8_t wrong_key[16];
  fio_rand_bytes(wrong_key, 16);
  fio_tls13_record_keys_init(&dec_keys,
                             wrong_key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);

  dec_len = fio_tls13_record_decrypt(decrypted,
                                     sizeof(decrypted),
                                     &ct,
                                     ciphertext,
                                     ct_len,
                                     &dec_keys);
  FIO_ASSERT(dec_len == -1, "Decryption should fail with wrong key");

  fio_tls13_record_keys_clear(&enc_keys);
  fio_tls13_record_keys_clear(&dec_keys);

  fprintf(stderr, "\t  - Authentication failure tests passed\n");
}

/* *****************************************************************************
Test: Padding Removal
***************************************************************************** */
FIO_SFUNC void test_padding_removal(void) {
  fprintf(stderr, "\t* Testing padding removal\n");

  /* This test verifies that zero padding is correctly stripped during
   * decryption. Since we can't easily inject padding into the encrypted
   * record, we test the basic case where no padding is present. */

  uint8_t key[16];
  uint8_t iv[12];
  fio_rand_bytes(key, 16);
  fio_rand_bytes(iv, 12);

  fio_tls13_record_keys_s enc_keys, dec_keys;
  fio_tls13_record_keys_init(&enc_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);
  fio_tls13_record_keys_init(&dec_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);

  /* Test with plaintext that ends with zeros (should not be confused with
   * padding) */
  uint8_t plaintext[16] = {0x01,
                           0x02,
                           0x03,
                           0x04,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00};

  uint8_t ciphertext[64];
  int ct_len = fio_tls13_record_encrypt(ciphertext,
                                        sizeof(ciphertext),
                                        plaintext,
                                        16,
                                        FIO_TLS13_CONTENT_APPLICATION_DATA,
                                        &enc_keys);
  FIO_ASSERT(ct_len > 0, "Encryption should succeed");

  uint8_t decrypted[64];
  fio_tls13_content_type_e ct;
  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         sizeof(decrypted),
                                         &ct,
                                         ciphertext,
                                         ct_len,
                                         &dec_keys);
  FIO_ASSERT(dec_len == 16, "Decrypted length should be 16");
  FIO_ASSERT(!memcmp(decrypted, plaintext, 16),
             "Decrypted data should match plaintext");

  fio_tls13_record_keys_clear(&enc_keys);
  fio_tls13_record_keys_clear(&dec_keys);

  fprintf(stderr, "\t  - Padding removal tests passed\n");
}

/* *****************************************************************************
Test: Maximum Record Size
***************************************************************************** */
FIO_SFUNC void test_max_record_size(void) {
  fprintf(stderr, "\t* Testing maximum record size handling\n");

  uint8_t key[16];
  uint8_t iv[12];
  fio_rand_bytes(key, 16);
  fio_rand_bytes(iv, 12);

  fio_tls13_record_keys_s keys;
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  /* Test with maximum plaintext size (16384 bytes) */
  uint8_t *plaintext = (uint8_t *)malloc(FIO_TLS13_MAX_PLAINTEXT_LEN);
  FIO_ASSERT(plaintext, "Failed to allocate max plaintext buffer");
  fio_rand_bytes(plaintext, FIO_TLS13_MAX_PLAINTEXT_LEN);

  size_t max_ct_len = FIO_TLS13_RECORD_HEADER_LEN +
                      FIO_TLS13_MAX_PLAINTEXT_LEN + 1 + FIO_TLS13_TAG_LEN;
  uint8_t *ciphertext = (uint8_t *)malloc(max_ct_len);
  FIO_ASSERT(ciphertext, "Failed to allocate max ciphertext buffer");

  int ct_len = fio_tls13_record_encrypt(ciphertext,
                                        max_ct_len,
                                        plaintext,
                                        FIO_TLS13_MAX_PLAINTEXT_LEN,
                                        FIO_TLS13_CONTENT_APPLICATION_DATA,
                                        &keys);
  FIO_ASSERT(ct_len > 0, "Encryption of max size should succeed");

  /* Reset keys for decryption */
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  uint8_t *decrypted = (uint8_t *)malloc(FIO_TLS13_MAX_PLAINTEXT_LEN + 1);
  FIO_ASSERT(decrypted, "Failed to allocate decrypted buffer");

  fio_tls13_content_type_e ct;
  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         FIO_TLS13_MAX_PLAINTEXT_LEN + 1,
                                         &ct,
                                         ciphertext,
                                         ct_len,
                                         &keys);
  FIO_ASSERT(dec_len == (int)FIO_TLS13_MAX_PLAINTEXT_LEN,
             "Decrypted length should match max plaintext");
  FIO_ASSERT(!memcmp(decrypted, plaintext, FIO_TLS13_MAX_PLAINTEXT_LEN),
             "Decrypted data should match plaintext");

  /* Test rejection of oversized plaintext */
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  ct_len = fio_tls13_record_encrypt(ciphertext,
                                    max_ct_len + 100,
                                    plaintext,
                                    FIO_TLS13_MAX_PLAINTEXT_LEN + 1,
                                    FIO_TLS13_CONTENT_APPLICATION_DATA,
                                    &keys);
  FIO_ASSERT(ct_len == -1, "Oversized plaintext should be rejected");

  free(plaintext);
  free(ciphertext);
  free(decrypted);
  fio_tls13_record_keys_clear(&keys);

  fprintf(stderr, "\t  - Maximum record size tests passed\n");
}

/* *****************************************************************************
Test: Content Type Handling
***************************************************************************** */
FIO_SFUNC void test_content_types(void) {
  fprintf(stderr, "\t* Testing content type handling\n");

  uint8_t key[16];
  uint8_t iv[12];
  fio_rand_bytes(key, 16);
  fio_rand_bytes(iv, 12);

  fio_tls13_content_type_e types[] = {FIO_TLS13_CONTENT_ALERT,
                                      FIO_TLS13_CONTENT_HANDSHAKE,
                                      FIO_TLS13_CONTENT_APPLICATION_DATA};

  for (size_t i = 0; i < 3; ++i) {
    fio_tls13_record_keys_s enc_keys, dec_keys;
    fio_tls13_record_keys_init(&enc_keys,
                               key,
                               16,
                               iv,
                               FIO_TLS13_CIPHER_AES_128_GCM);
    fio_tls13_record_keys_init(&dec_keys,
                               key,
                               16,
                               iv,
                               FIO_TLS13_CIPHER_AES_128_GCM);

    uint8_t plaintext[16];
    fio_rand_bytes(plaintext, 16);

    uint8_t ciphertext[64];
    int ct_len = fio_tls13_record_encrypt(ciphertext,
                                          sizeof(ciphertext),
                                          plaintext,
                                          16,
                                          types[i],
                                          &enc_keys);
    FIO_ASSERT(ct_len > 0,
               "Encryption should succeed for content type %d",
               types[i]);

    /* Outer type should always be application_data */
    FIO_ASSERT(ciphertext[0] == FIO_TLS13_CONTENT_APPLICATION_DATA,
               "Outer content type should be application_data");

    uint8_t decrypted[64];
    fio_tls13_content_type_e ct;
    int dec_len = fio_tls13_record_decrypt(decrypted,
                                           sizeof(decrypted),
                                           &ct,
                                           ciphertext,
                                           ct_len,
                                           &dec_keys);
    FIO_ASSERT(dec_len >= 0, "Decryption should succeed");
    FIO_ASSERT(ct == types[i],
               "Inner content type should match: expected %d, got %d",
               types[i],
               ct);

    fio_tls13_record_keys_clear(&enc_keys);
    fio_tls13_record_keys_clear(&dec_keys);
  }

  fprintf(stderr, "\t  - Content type tests passed\n");
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  fprintf(stderr, "Testing TLS 1.3 Record Layer (RFC 8446 Section 5)\n");

  /* Test nonce construction */
  test_nonce_construction();

  /* Test header parsing */
  test_header_parsing();

  /* Test round-trip for all cipher suites */
  test_roundtrip(FIO_TLS13_CIPHER_AES_128_GCM, "AES-128-GCM", 16);
  test_roundtrip(FIO_TLS13_CIPHER_AES_256_GCM, "AES-256-GCM", 32);
  test_roundtrip(FIO_TLS13_CIPHER_CHACHA20_POLY1305, "ChaCha20-Poly1305", 32);

  /* Test sequence number increment */
  test_sequence_number();

  /* Test authentication failure detection */
  test_auth_failure();

  /* Test padding removal */
  test_padding_removal();

  /* Test maximum record size */
  test_max_record_size();

  /* Test content type handling */
  test_content_types();

  fprintf(stderr, "\nAll TLS 1.3 Record Layer tests passed!\n");
  return 0;
}
