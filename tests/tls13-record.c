/* *****************************************************************************
Test for fio-stl/190 tls13.h — TLS 1.3 Record Layer

Coverage: record header parsing, nonce construction, AEAD record
encryption/decryption roundtrip for all three TLS 1.3 cipher suites,
sequence-number handling, authentication failure detection, content-type
preservation, record-size limits, and encrypted alert records.

No crypto algorithm vectors are tested here (those live in sha.c, aes.c,
chacha.c, hkdf.c).  This file is strictly TLS record-layer behavior.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SHA2
#define FIO_HKDF
#define FIO_AES
#define FIO_CHACHA
#define FIO_ED25519
#define FIO_P256
#define FIO_TLS13
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Helper: allocate with FIO_MEM_REALLOC
***************************************************************************** */
FIO_SFUNC void *tls13_test_alloc(size_t len) {
  return FIO_MEM_REALLOC(NULL, 0, len, 0);
}

FIO_SFUNC void tls13_test_free(void *p, size_t len) {
  (void)len;
  if (p)
    FIO_MEM_FREE(p, len);
}

/* *****************************************************************************
Nonce construction (RFC 8446 Section 5.3)
***************************************************************************** */
FIO_SFUNC void test_tls13_record_nonce(void) {
  uint8_t iv[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
  uint8_t nonce[12];

  fio_tls13_build_nonce(nonce, iv, 0);
  FIO_ASSERT(!FIO_MEMCMP(nonce, iv, 12),
             "nonce with seq=0 should equal IV");

  fio_tls13_build_nonce(nonce, iv, 1);
  FIO_ASSERT(nonce[11] == (uint8_t)(iv[11] ^ 1),
             "nonce with seq=1: last byte should be IV[11]^1");
  FIO_ASSERT(!FIO_MEMCMP(nonce, iv, 11),
             "nonce with seq=1: first 11 bytes should match IV");

  uint64_t seq = 0x0102030405060708ULL;
  fio_tls13_build_nonce(nonce, iv, seq);
  uint8_t expected[12] = {0x00, 0x01, 0x02, 0x03, 0x04 ^ 0x01, 0x05 ^ 0x02,
                          0x06 ^ 0x03, 0x07 ^ 0x04, 0x08 ^ 0x05, 0x09 ^ 0x06,
                          0x0a ^ 0x07, 0x0b ^ 0x08};
  FIO_ASSERT(!FIO_MEMCMP(nonce, expected, 12),
             "nonce with 64-bit seq failed");
}

/* *****************************************************************************
Record header parsing
***************************************************************************** */
FIO_SFUNC void test_tls13_record_header(void) {
  uint8_t record[37] = {0x17, 0x03, 0x03, 0x00, 0x20};
  fio_tls13_content_type_e ct;
  size_t payload_len;
  const uint8_t *p =
      fio_tls13_record_parse_header(record, 37, &ct, &payload_len);
  FIO_ASSERT(p == record + 5, "payload pointer mismatch");
  FIO_ASSERT(ct == FIO_TLS13_CONTENT_APPLICATION_DATA, "content type mismatch");
  FIO_ASSERT(payload_len == 32, "payload length mismatch");

  FIO_ASSERT(!fio_tls13_record_parse_header(record, 4, &ct, &payload_len),
             "incomplete header should fail");
  FIO_ASSERT(!fio_tls13_record_parse_header(record, 20, &ct, &payload_len),
             "incomplete payload should fail");

  uint8_t bad_type[] = {0x00, 0x03, 0x03, 0x00, 0x10};
  FIO_ASSERT(!fio_tls13_record_parse_header(bad_type, 5, &ct, &payload_len),
             "invalid content type should fail");

  uint8_t valid_types[] = {20, 21, 22, 23};
  for (size_t i = 0; i < sizeof(valid_types); ++i) {
    record[0] = valid_types[i];
    p = fio_tls13_record_parse_header(record, 37, &ct, &payload_len);
    FIO_ASSERT(p != NULL, "valid content type %d failed", valid_types[i]);
    FIO_ASSERT(ct == (fio_tls13_content_type_e)valid_types[i],
               "content type %d mismatch", valid_types[i]);
  }

  /* record_overflow: length > 16640 */
  uint8_t big[5] = {0x17, 0x03, 0x03, 0x41, 0x01};
  FIO_ASSERT(!fio_tls13_record_parse_header(big, 5, &ct, &payload_len),
             "oversized record length should fail");
}

/* *****************************************************************************
Record roundtrip helpers
***************************************************************************** */
FIO_SFUNC void test_tls13_record_roundtrip_one(
    fio_tls13_cipher_type_e cipher,
    uint8_t key_len,
    const uint8_t *plaintext,
    size_t plaintext_len) {
  uint8_t key[32];
  uint8_t iv[12];
  fio_rand_bytes(key, key_len);
  fio_rand_bytes(iv, 12);

  fio_tls13_record_keys_s enc_keys, dec_keys;
  fio_tls13_record_keys_init(&enc_keys, key, key_len, iv, cipher);
  fio_tls13_record_keys_init(&dec_keys, key, key_len, iv, cipher);

  size_t max_ct_len =
      FIO_TLS13_RECORD_HEADER_LEN + plaintext_len + 1 + FIO_TLS13_TAG_LEN;
  uint8_t *ciphertext = (uint8_t *)tls13_test_alloc(max_ct_len);
  uint8_t *decrypted = (uint8_t *)tls13_test_alloc(plaintext_len + 1);
  FIO_ASSERT(ciphertext && decrypted, "allocation failed");

  int ct_len = fio_tls13_record_encrypt(ciphertext,
                                        max_ct_len,
                                        plaintext,
                                        plaintext_len,
                                        FIO_TLS13_CONTENT_APPLICATION_DATA,
                                        &enc_keys);
  FIO_ASSERT(ct_len == (int)max_ct_len,
             "ciphertext length mismatch for cipher %d size %zu",
             cipher,
             plaintext_len);
  FIO_ASSERT(ciphertext[0] == FIO_TLS13_CONTENT_APPLICATION_DATA,
             "outer content type must be application_data");
  FIO_ASSERT(ciphertext[1] == 0x03 && ciphertext[2] == 0x03,
             "legacy version must be 0x0303");

  fio_tls13_content_type_e content_type;
  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         plaintext_len + 1,
                                         &content_type,
                                         ciphertext,
                                         (size_t)ct_len,
                                         &dec_keys);
  FIO_ASSERT(dec_len == (int)plaintext_len,
             "decrypted length mismatch for cipher %d: got %d expected %zu",
             cipher,
             dec_len,
             plaintext_len);
  FIO_ASSERT(content_type == FIO_TLS13_CONTENT_APPLICATION_DATA,
             "inner content type mismatch");
  FIO_ASSERT(!FIO_MEMCMP(decrypted, plaintext, plaintext_len),
             "decrypted data mismatch");

  tls13_test_free(ciphertext, max_ct_len);
  tls13_test_free(decrypted, plaintext_len + 1);
  fio_tls13_record_keys_clear(&enc_keys);
  fio_tls13_record_keys_clear(&dec_keys);
}

FIO_SFUNC void test_tls13_record_roundtrip_cipher(
    fio_tls13_cipher_type_e cipher,
    uint8_t key_len) {
  uint8_t small[256];
  fio_rand_bytes(small, sizeof(small));

  size_t sizes[] = {0,   1,   15,  16,  17,   31,   32,   33,
                    127, 128, 255, 256, 1024, 4096, 16384};
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t sz = sizes[i];
    if (sz <= sizeof(small)) {
      test_tls13_record_roundtrip_one(cipher, key_len, small, sz);
    } else {
      uint8_t *large = (uint8_t *)tls13_test_alloc(sz);
      FIO_ASSERT(large, "allocation failed");
      fio_rand_bytes(large, sz);
      test_tls13_record_roundtrip_one(cipher, key_len, large, sz);
      tls13_test_free(large, sz);
    }
  }
}

FIO_SFUNC void test_tls13_record_roundtrip(void) {
  test_tls13_record_roundtrip_cipher(FIO_TLS13_CIPHER_AES_128_GCM, 16);
  test_tls13_record_roundtrip_cipher(FIO_TLS13_CIPHER_AES_256_GCM, 32);
  test_tls13_record_roundtrip_cipher(FIO_TLS13_CIPHER_CHACHA20_POLY1305, 32);
}

/* *****************************************************************************
Sequence number increment
***************************************************************************** */
FIO_SFUNC void test_tls13_record_sequence(void) {
  uint8_t key[16] = {0};
  uint8_t iv[12] = {0};
  fio_tls13_record_keys_s keys;
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  FIO_ASSERT(keys.sequence_number == 0, "initial sequence number must be 0");

  uint8_t plaintext[16] = {0};
  uint8_t ciphertext[64];
  for (uint64_t i = 0; i < 5; ++i) {
    FIO_ASSERT(keys.sequence_number == i,
               "sequence number before encrypt should be %llu",
               (unsigned long long)i);
    FIO_ASSERT(fio_tls13_record_encrypt(ciphertext,
                                        sizeof(ciphertext),
                                        plaintext,
                                        sizeof(plaintext),
                                        FIO_TLS13_CONTENT_APPLICATION_DATA,
                                        &keys) > 0,
               "encrypt failed");
    FIO_ASSERT(keys.sequence_number == i + 1,
               "sequence number after encrypt should be %llu",
               (unsigned long long)(i + 1));
  }

  fio_tls13_record_keys_s dec_keys;
  fio_tls13_record_keys_init(&dec_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  int ct_len = fio_tls13_record_encrypt(ciphertext,
                                        sizeof(ciphertext),
                                        plaintext,
                                        sizeof(plaintext),
                                        FIO_TLS13_CONTENT_APPLICATION_DATA,
                                        &keys);
  FIO_ASSERT(ct_len > 0, "encrypt failed");
  uint8_t decrypted[32];
  fio_tls13_content_type_e ct;
  FIO_ASSERT(fio_tls13_record_decrypt(decrypted,
                                      sizeof(decrypted),
                                      &ct,
                                      ciphertext,
                                      (size_t)ct_len,
                                      &dec_keys) >= 0,
             "decrypt failed");
  FIO_ASSERT(dec_keys.sequence_number == 1,
             "decryption sequence number should increment");
  fio_tls13_record_keys_clear(&keys);
  fio_tls13_record_keys_clear(&dec_keys);
}

/* *****************************************************************************
Authentication failure detection
***************************************************************************** */
FIO_SFUNC void test_tls13_record_auth_failure(void) {
  uint8_t key[16], iv[12];
  fio_rand_bytes(key, 16);
  fio_rand_bytes(iv, 12);

  fio_tls13_record_keys_s enc_keys, dec_keys;
  fio_tls13_record_keys_init(&enc_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  fio_tls13_record_keys_init(&dec_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  uint8_t plaintext[32];
  fio_rand_bytes(plaintext, sizeof(plaintext));
  uint8_t ciphertext[64];
  int ct_len = fio_tls13_record_encrypt(ciphertext,
                                        sizeof(ciphertext),
                                        plaintext,
                                        sizeof(plaintext),
                                        FIO_TLS13_CONTENT_APPLICATION_DATA,
                                        &enc_keys);
  FIO_ASSERT(ct_len > 0, "encrypt failed");

  ciphertext[10] ^= 0xFF;
  uint8_t decrypted[64];
  fio_tls13_content_type_e ct;
  FIO_ASSERT(fio_tls13_record_decrypt(decrypted,
                                      sizeof(decrypted),
                                      &ct,
                                      ciphertext,
                                      (size_t)ct_len,
                                      &dec_keys) == -1,
             "corrupted ciphertext should fail authentication");

  fio_tls13_record_keys_init(&enc_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  ct_len = fio_tls13_record_encrypt(ciphertext,
                                    sizeof(ciphertext),
                                    plaintext,
                                    sizeof(plaintext),
                                    FIO_TLS13_CONTENT_APPLICATION_DATA,
                                    &enc_keys);
  FIO_ASSERT(ct_len > 0, "encrypt failed");

  uint8_t wrong_key[16];
  fio_rand_bytes(wrong_key, 16);
  fio_tls13_record_keys_init(&dec_keys, wrong_key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  FIO_ASSERT(fio_tls13_record_decrypt(decrypted,
                                      sizeof(decrypted),
                                      &ct,
                                      ciphertext,
                                      (size_t)ct_len,
                                      &dec_keys) == -1,
             "wrong key should fail authentication");

  fio_tls13_record_keys_clear(&enc_keys);
  fio_tls13_record_keys_clear(&dec_keys);
}

/* *****************************************************************************
Content type preservation
***************************************************************************** */
FIO_SFUNC void test_tls13_record_content_types(void) {
  uint8_t key[16], iv[12];
  fio_rand_bytes(key, 16);
  fio_rand_bytes(iv, 12);
  uint8_t plaintext[16];
  fio_rand_bytes(plaintext, sizeof(plaintext));

  fio_tls13_content_type_e types[] = {FIO_TLS13_CONTENT_ALERT,
                                      FIO_TLS13_CONTENT_HANDSHAKE,
                                      FIO_TLS13_CONTENT_APPLICATION_DATA};
  for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
    fio_tls13_record_keys_s enc_keys, dec_keys;
    fio_tls13_record_keys_init(&enc_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
    fio_tls13_record_keys_init(&dec_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

    uint8_t ciphertext[64];
    int ct_len = fio_tls13_record_encrypt(ciphertext,
                                          sizeof(ciphertext),
                                          plaintext,
                                          sizeof(plaintext),
                                          types[i],
                                          &enc_keys);
    FIO_ASSERT(ct_len > 0, "encrypt failed for content type %d", types[i]);
    FIO_ASSERT(ciphertext[0] == FIO_TLS13_CONTENT_APPLICATION_DATA,
               "outer type must be application_data");

    uint8_t decrypted[64];
    fio_tls13_content_type_e ct;
    int dec_len = fio_tls13_record_decrypt(decrypted,
                                           sizeof(decrypted),
                                           &ct,
                                           ciphertext,
                                           (size_t)ct_len,
                                           &dec_keys);
    FIO_ASSERT(dec_len == (int)sizeof(plaintext), "decrypt length mismatch");
    FIO_ASSERT(ct == types[i], "content type not preserved");

    fio_tls13_record_keys_clear(&enc_keys);
    fio_tls13_record_keys_clear(&dec_keys);
  }
}

/* *****************************************************************************
Record size limits
***************************************************************************** */
FIO_SFUNC void test_tls13_record_size_limits(void) {
  uint8_t key[16], iv[12];
  fio_rand_bytes(key, 16);
  fio_rand_bytes(iv, 12);
  fio_tls13_record_keys_s keys;
  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  size_t max_plain = FIO_TLS13_MAX_PLAINTEXT_LEN;
  uint8_t *plaintext = (uint8_t *)tls13_test_alloc(max_plain);
  FIO_ASSERT(plaintext, "allocation failed");
  fio_rand_bytes(plaintext, max_plain);

  size_t max_ct_len =
      FIO_TLS13_RECORD_HEADER_LEN + max_plain + 1 + FIO_TLS13_TAG_LEN;
  uint8_t *ciphertext = (uint8_t *)tls13_test_alloc(max_ct_len);
  FIO_ASSERT(ciphertext, "allocation failed");

  FIO_ASSERT(fio_tls13_record_encrypt(ciphertext,
                                      max_ct_len,
                                      plaintext,
                                      max_plain,
                                      FIO_TLS13_CONTENT_APPLICATION_DATA,
                                      &keys) > 0,
             "max plaintext should encrypt");

  fio_tls13_record_keys_init(&keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  FIO_ASSERT(fio_tls13_record_encrypt(ciphertext,
                                      max_ct_len + 100,
                                      plaintext,
                                      max_plain + 1,
                                      FIO_TLS13_CONTENT_APPLICATION_DATA,
                                      &keys) == -1,
             "oversized plaintext should fail");

  tls13_test_free(plaintext, max_plain);
  tls13_test_free(ciphertext, max_ct_len);
  fio_tls13_record_keys_clear(&keys);
}

/* *****************************************************************************
Encrypted alert records
***************************************************************************** */
FIO_SFUNC void test_tls13_record_alert(void) {
  uint8_t key[16], iv[12];
  fio_rand_bytes(key, 16);
  fio_rand_bytes(iv, 12);
  fio_tls13_record_keys_s enc_keys, dec_keys;
  fio_tls13_record_keys_init(
      &enc_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  fio_tls13_record_keys_init(
      &dec_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  uint8_t out[64];
  int len = fio_tls13_send_alert(out,
                                 sizeof(out),
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_HANDSHAKE_FAILURE,
                                 &enc_keys);
  FIO_ASSERT(len > 0, "encrypted alert build failed");
  FIO_ASSERT(out[0] == FIO_TLS13_CONTENT_APPLICATION_DATA,
             "alert record outer type must be application_data");

  uint8_t decrypted[16];
  fio_tls13_content_type_e ct;
  int dec_len = fio_tls13_record_decrypt(decrypted,
                                         sizeof(decrypted),
                                         &ct,
                                         out,
                                         (size_t)len,
                                         &dec_keys);
  FIO_ASSERT(dec_len == 2, "alert decrypt length mismatch");
  FIO_ASSERT(ct == FIO_TLS13_CONTENT_ALERT, "alert content type mismatch");
  FIO_ASSERT(decrypted[0] == FIO_TLS13_ALERT_LEVEL_FATAL,
             "alert level mismatch");
  FIO_ASSERT(decrypted[1] == FIO_TLS13_ALERT_HANDSHAKE_FAILURE,
             "alert description mismatch");

  uint8_t plain[16];
  int plain_len = fio_tls13_send_alert_plaintext(plain,
                                                  sizeof(plain),
                                                  FIO_TLS13_ALERT_LEVEL_WARNING,
                                                  FIO_TLS13_ALERT_CLOSE_NOTIFY);
  FIO_ASSERT(plain_len == 7, "plaintext alert length mismatch");
  FIO_ASSERT(plain[0] == FIO_TLS13_CONTENT_ALERT,
             "plaintext alert type mismatch");
  FIO_ASSERT(plain[5] == FIO_TLS13_ALERT_LEVEL_WARNING,
             "plaintext alert level mismatch");
  FIO_ASSERT(plain[6] == FIO_TLS13_ALERT_CLOSE_NOTIFY,
             "plaintext alert description mismatch");

  fio_tls13_record_keys_clear(&enc_keys);
  fio_tls13_record_keys_clear(&dec_keys);
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  test_tls13_record_nonce();
  test_tls13_record_header();
  test_tls13_record_roundtrip();
  test_tls13_record_sequence();
  test_tls13_record_auth_failure();
  test_tls13_record_content_types();
  test_tls13_record_size_limits();
  test_tls13_record_alert();
  return 0;
}
