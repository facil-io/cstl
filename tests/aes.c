/* *****************************************************************************
Test for 153 aes.h

Coverage: AES-128-GCM and AES-256-GCM authenticated encryption/decryption
using NIST SP 800-38D test vectors, plus deterministic edge-case roundtrips
and authentication-failure detection. Performance loops are intentionally
omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_AES
#include FIO_INCLUDE_FILE

/* *****************************************************************************
AES-128-GCM Known-Answer Tests (NIST SP 800-38D)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, aes128_gcm_kat)(void) {
  /* Test Case 1: empty plaintext, no AAD */
  {
    uint8_t key[16] = {0};
    uint8_t nonce[12] = {0};
    uint8_t expected_tag[16] = {0x58, 0xe2, 0xfc, 0xce, 0xfa, 0x7e,
                                0x30, 0x61, 0x36, 0x7f, 0x1d, 0x57,
                                0xa4, 0xe7, 0x45, 0x5a};
    uint8_t mac[16];

    fio_aes128_gcm_enc(mac, NULL, 0, NULL, 0, key, nonce);
    FIO_ASSERT(!memcmp(mac, expected_tag, 16),
               "AES-128-GCM Test Case 1 tag mismatch");
    FIO_ASSERT(!fio_aes128_gcm_dec(mac, NULL, 0, NULL, 0, key, nonce),
               "AES-128-GCM Test Case 1 decryption failed");
  }

  /* Test Case 2: 16-byte plaintext, no AAD */
  {
    uint8_t key[16] = {0};
    uint8_t nonce[12] = {0};
    uint8_t plaintext[16] = {0};
    uint8_t expected_ct[16] = {0x03, 0x88, 0xda, 0xce, 0x60, 0xb6,
                               0xa3, 0x92, 0xf3, 0x28, 0xc2, 0xb9,
                               0x71, 0xb2, 0xfe, 0x78};
    uint8_t expected_tag[16] = {0xab, 0x6e, 0x47, 0xd4, 0x2c, 0xec,
                                0x13, 0xbd, 0xf5, 0x3a, 0x67, 0xb2,
                                0x12, 0x57, 0xbd, 0xdf};
    uint8_t buffer[16];
    uint8_t mac[16];

    FIO_MEMCPY(buffer, plaintext, 16);
    fio_aes128_gcm_enc(mac, buffer, 16, NULL, 0, key, nonce);
    FIO_ASSERT(!memcmp(buffer, expected_ct, 16),
               "AES-128-GCM Test Case 2 ciphertext mismatch");
    FIO_ASSERT(!memcmp(mac, expected_tag, 16),
               "AES-128-GCM Test Case 2 tag mismatch");
    FIO_ASSERT(!fio_aes128_gcm_dec(mac, buffer, 16, NULL, 0, key, nonce),
               "AES-128-GCM Test Case 2 decryption auth failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 16),
               "AES-128-GCM Test Case 2 decryption roundtrip failed");
  }

  /* Test Case 3: 64-byte plaintext, no AAD */
  {
    uint8_t key[16] = {0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                       0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08};
    uint8_t nonce[12] = {0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                         0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88};
    uint8_t plaintext[64] = {
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59, 0x09,
        0xc5, 0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34,
        0xf7, 0xda, 0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, 0x1c,
        0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53, 0x2f, 0xcf, 0x0e, 0x24,
        0x49, 0xa6, 0xb5, 0x25, 0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6,
        0x57, 0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55};
    uint8_t expected_ct[64] = {
        0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24, 0x4b, 0x72, 0x21,
        0xb7, 0x84, 0xd0, 0xd4, 0x9c, 0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02,
        0xa4, 0xe0, 0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e, 0x21,
        0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c, 0x7d, 0x8f, 0x6a, 0x5a,
        0xac, 0x84, 0xaa, 0x05, 0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac,
        0x97, 0x3d, 0x58, 0xe0, 0x91, 0x47, 0x3f, 0x59, 0x85};
    uint8_t expected_tag[16] = {0x4d, 0x5c, 0x2a, 0xf3, 0x27, 0xcd,
                                0x64, 0xa6, 0x2c, 0xf3, 0x5a, 0xbd,
                                0x2b, 0xa6, 0xfa, 0xb4};
    uint8_t buffer[64];
    uint8_t mac[16];

    FIO_MEMCPY(buffer, plaintext, 64);
    fio_aes128_gcm_enc(mac, buffer, 64, NULL, 0, key, nonce);
    FIO_ASSERT(!memcmp(buffer, expected_ct, 64),
               "AES-128-GCM Test Case 3 ciphertext mismatch");
    FIO_ASSERT(!memcmp(mac, expected_tag, 16),
               "AES-128-GCM Test Case 3 tag mismatch");
    FIO_ASSERT(!fio_aes128_gcm_dec(mac, buffer, 64, NULL, 0, key, nonce),
               "AES-128-GCM Test Case 3 decryption auth failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 64),
               "AES-128-GCM Test Case 3 decryption roundtrip failed");
  }

  /* Test Case 4: 60-byte plaintext with 20-byte AAD */
  {
    uint8_t key[16] = {0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                       0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08};
    uint8_t nonce[12] = {0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                         0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88};
    uint8_t aad[20] = {0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe,
                       0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad,
                       0xbe, 0xef, 0xab, 0xad, 0xda, 0xd2};
    uint8_t plaintext[60] = {
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59, 0x09, 0xc5,
        0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
        0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, 0x1c, 0x3c, 0x0c, 0x95,
        0x95, 0x68, 0x09, 0x53, 0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
        0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57, 0xba, 0x63, 0x7b, 0x39};
    uint8_t expected_ct[60] = {
        0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24, 0x4b, 0x72, 0x21, 0xb7,
        0x84, 0xd0, 0xd4, 0x9c, 0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
        0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e, 0x21, 0xd5, 0x14, 0xb2,
        0x54, 0x66, 0x93, 0x1c, 0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
        0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97, 0x3d, 0x58, 0xe0, 0x91};
    uint8_t expected_tag[16] = {0x5b, 0xc9, 0x4f, 0xbc, 0x32, 0x21,
                                0xa5, 0xdb, 0x94, 0xfa, 0xe9, 0x5a,
                                0xe7, 0x12, 0x1a, 0x47};
    uint8_t buffer[60];
    uint8_t mac[16];

    FIO_MEMCPY(buffer, plaintext, 60);
    fio_aes128_gcm_enc(mac, buffer, 60, aad, 20, key, nonce);
    FIO_ASSERT(!memcmp(buffer, expected_ct, 60),
               "AES-128-GCM Test Case 4 ciphertext mismatch");
    FIO_ASSERT(!memcmp(mac, expected_tag, 16),
               "AES-128-GCM Test Case 4 tag mismatch");
    FIO_ASSERT(!fio_aes128_gcm_dec(mac, buffer, 60, aad, 20, key, nonce),
               "AES-128-GCM Test Case 4 decryption auth failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 60),
               "AES-128-GCM Test Case 4 decryption roundtrip failed");
  }
}

/* *****************************************************************************
AES-256-GCM Known-Answer Tests (NIST SP 800-38D)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, aes256_gcm_kat)(void) {
  /* Test Case 13: empty plaintext, no AAD */
  {
    uint8_t key[32] = {0};
    uint8_t nonce[12] = {0};
    uint8_t expected_tag[16] = {0x53, 0x0f, 0x8a, 0xfb, 0xc7, 0x45,
                                0x36, 0xb9, 0xa9, 0x63, 0xb4, 0xf1,
                                0xc4, 0xcb, 0x73, 0x8b};
    uint8_t mac[16];

    fio_aes256_gcm_enc(mac, NULL, 0, NULL, 0, key, nonce);
    FIO_ASSERT(!memcmp(mac, expected_tag, 16),
               "AES-256-GCM Test Case 13 tag mismatch");
    FIO_ASSERT(!fio_aes256_gcm_dec(mac, NULL, 0, NULL, 0, key, nonce),
               "AES-256-GCM Test Case 13 decryption failed");
  }

  /* Test Case 14: 16-byte plaintext, no AAD */
  {
    uint8_t key[32] = {0};
    uint8_t nonce[12] = {0};
    uint8_t plaintext[16] = {0};
    uint8_t expected_ct[16] = {0xce, 0xa7, 0x40, 0x3d, 0x4d, 0x60,
                               0x6b, 0x6e, 0x07, 0x4e, 0xc5, 0xd3,
                               0xba, 0xf3, 0x9d, 0x18};
    uint8_t expected_tag[16] = {0xd0, 0xd1, 0xc8, 0xa7, 0x99, 0x99,
                                0x6b, 0xf0, 0x26, 0x5b, 0x98, 0xb5,
                                0xd4, 0x8a, 0xb9, 0x19};
    uint8_t buffer[16];
    uint8_t mac[16];

    FIO_MEMCPY(buffer, plaintext, 16);
    fio_aes256_gcm_enc(mac, buffer, 16, NULL, 0, key, nonce);
    FIO_ASSERT(!memcmp(buffer, expected_ct, 16),
               "AES-256-GCM Test Case 14 ciphertext mismatch");
    FIO_ASSERT(!memcmp(mac, expected_tag, 16),
               "AES-256-GCM Test Case 14 tag mismatch");
    FIO_ASSERT(!fio_aes256_gcm_dec(mac, buffer, 16, NULL, 0, key, nonce),
               "AES-256-GCM Test Case 14 decryption auth failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 16),
               "AES-256-GCM Test Case 14 decryption roundtrip failed");
  }

  /* Test Case 15: 64-byte plaintext, no AAD */
  {
    uint8_t key[32] = {0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                       0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
                       0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                       0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08};
    uint8_t nonce[12] = {0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                         0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88};
    uint8_t plaintext[64] = {
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59, 0x09,
        0xc5, 0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34,
        0xf7, 0xda, 0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, 0x1c,
        0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53, 0x2f, 0xcf, 0x0e, 0x24,
        0x49, 0xa6, 0xb5, 0x25, 0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6,
        0x57, 0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55};
    uint8_t expected_ct[64] = {
        0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07, 0xf4, 0x7f, 0x37,
        0xa3, 0x2a, 0x84, 0x42, 0x7d, 0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5,
        0xc0, 0xc9, 0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa, 0x8c,
        0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d, 0xa7, 0xb0, 0x8b, 0x10,
        0x56, 0x82, 0x88, 0x38, 0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a,
        0x0a, 0xbc, 0xc9, 0xf6, 0x62, 0x89, 0x80, 0x15, 0xad};
    uint8_t expected_tag[16] = {0xb0, 0x94, 0xda, 0xc5, 0xd9, 0x34,
                                0x71, 0xbd, 0xec, 0x1a, 0x50, 0x22,
                                0x70, 0xe3, 0xcc, 0x6c};
    uint8_t buffer[64];
    uint8_t mac[16];

    FIO_MEMCPY(buffer, plaintext, 64);
    fio_aes256_gcm_enc(mac, buffer, 64, NULL, 0, key, nonce);
    FIO_ASSERT(!memcmp(buffer, expected_ct, 64),
               "AES-256-GCM Test Case 15 ciphertext mismatch");
    FIO_ASSERT(!memcmp(mac, expected_tag, 16),
               "AES-256-GCM Test Case 15 tag mismatch");
    FIO_ASSERT(!fio_aes256_gcm_dec(mac, buffer, 64, NULL, 0, key, nonce),
               "AES-256-GCM Test Case 15 decryption auth failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 64),
               "AES-256-GCM Test Case 15 decryption roundtrip failed");
  }

  /* Test Case 16: 60-byte plaintext with 20-byte AAD */
  {
    uint8_t key[32] = {0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                       0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
                       0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                       0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08};
    uint8_t nonce[12] = {0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                         0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88};
    uint8_t aad[20] = {0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe,
                       0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad,
                       0xbe, 0xef, 0xab, 0xad, 0xda, 0xd2};
    uint8_t plaintext[60] = {
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59, 0x09, 0xc5,
        0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
        0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, 0x1c, 0x3c, 0x0c, 0x95,
        0x95, 0x68, 0x09, 0x53, 0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
        0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57, 0xba, 0x63, 0x7b, 0x39};
    uint8_t expected_ct[60] = {
        0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07, 0xf4, 0x7f, 0x37,
        0xa3, 0x2a, 0x84, 0x42, 0x7d, 0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5,
        0xc0, 0xc9, 0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa, 0x8c,
        0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d, 0xa7, 0xb0, 0x8b, 0x10,
        0x56, 0x82, 0x88, 0x38, 0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a,
        0x0a, 0xbc, 0xc9, 0xf6, 0x62};
    uint8_t expected_tag[16] = {0x76, 0xfc, 0x6e, 0xce, 0x0f, 0x4e,
                                0x17, 0x68, 0xcd, 0xdf, 0x88, 0x53,
                                0xbb, 0x2d, 0x55, 0x1b};
    uint8_t buffer[60];
    uint8_t mac[16];

    FIO_MEMCPY(buffer, plaintext, 60);
    fio_aes256_gcm_enc(mac, buffer, 60, aad, 20, key, nonce);
    FIO_ASSERT(!memcmp(buffer, expected_ct, 60),
               "AES-256-GCM Test Case 16 ciphertext mismatch");
    FIO_ASSERT(!memcmp(mac, expected_tag, 16),
               "AES-256-GCM Test Case 16 tag mismatch");
    FIO_ASSERT(!fio_aes256_gcm_dec(mac, buffer, 60, aad, 20, key, nonce),
               "AES-256-GCM Test Case 16 decryption auth failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 60),
               "AES-256-GCM Test Case 16 decryption roundtrip failed");
  }
}

/* *****************************************************************************
AES-GCM Deterministic Edge Cases
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, aes_gcm_edges)(void) {
  const size_t max_len = 4096;
  uint8_t key16[16], nonce[12], mac[16];
  uint8_t key32[32];
  uint8_t plaintext[4096], buffer[4096], aad[256];
  size_t lengths[] = {0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 1024, 4096};

  fio_rand_bytes(key16, 16);
  fio_rand_bytes(key32, 32);
  fio_rand_bytes(nonce, 12);

  /* Empty plaintext and AAD must produce a non-zero tag and decrypt */
  {
    uint8_t zkey16[16] = {0};
    uint8_t zkey32[32] = {0};
    uint8_t znonce[12] = {0};
    uint8_t mac1[16], mac2[16];

    fio_aes128_gcm_enc(mac1, NULL, 0, NULL, 0, zkey16, znonce);
    fio_aes256_gcm_enc(mac2, NULL, 0, NULL, 0, zkey32, znonce);
    FIO_ASSERT(!fio_aes128_gcm_dec(mac1, NULL, 0, NULL, 0, zkey16, znonce),
               "AES-128-GCM empty decryption failed");
    FIO_ASSERT(!fio_aes256_gcm_dec(mac2, NULL, 0, NULL, 0, zkey32, znonce),
               "AES-256-GCM empty decryption failed");
  }

  /* Authentication failure with a corrupted tag */
  {
    uint8_t p[16] = {0};
    uint8_t b[16];
    FIO_MEMCPY(b, p, 16);
    fio_aes128_gcm_enc(mac, b, 16, NULL, 0, key16, nonce);
    mac[0] ^= 0x01;
    FIO_ASSERT(fio_aes128_gcm_dec(mac, b, 16, NULL, 0, key16, nonce),
               "AES-128-GCM should detect a corrupted tag");
  }

  /* AES-128-GCM length sweep */
  for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
    size_t len = lengths[i];
    FIO_ASSERT(len <= max_len, "AES edge-case length exceeds buffer");
    fio_rand_bytes(plaintext, len ? len : 1);
    FIO_MEMCPY(buffer, plaintext, len);

    fio_aes128_gcm_enc(mac, buffer, len, NULL, 0, key16, nonce);
    FIO_ASSERT(!fio_aes128_gcm_dec(mac, buffer, len, NULL, 0, key16, nonce),
               "AES-128-GCM length %zu auth failed", len);
    FIO_ASSERT(!memcmp(buffer, plaintext, len),
               "AES-128-GCM length %zu roundtrip failed", len);
  }

  /* AES-256-GCM length sweep */
  for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
    size_t len = lengths[i];
    FIO_ASSERT(len <= max_len, "AES edge-case length exceeds buffer");
    fio_rand_bytes(plaintext, len ? len : 1);
    FIO_MEMCPY(buffer, plaintext, len);

    fio_aes256_gcm_enc(mac, buffer, len, NULL, 0, key32, nonce);
    FIO_ASSERT(!fio_aes256_gcm_dec(mac, buffer, len, NULL, 0, key32, nonce),
               "AES-256-GCM length %zu auth failed", len);
    FIO_ASSERT(!memcmp(buffer, plaintext, len),
               "AES-256-GCM length %zu roundtrip failed", len);
  }

  /* AAD length sweep with fixed plaintext */
  {
    size_t pt_len = 32;
    size_t aad_sizes[] = {0, 1, 15, 16, 17, 31, 32, 33, 255, 256};
    fio_rand_bytes(plaintext, pt_len);
    fio_rand_bytes(aad, 256);

    for (size_t i = 0; i < sizeof(aad_sizes) / sizeof(aad_sizes[0]); ++i) {
      FIO_MEMCPY(buffer, plaintext, pt_len);
      fio_aes128_gcm_enc(mac, buffer, pt_len, aad, aad_sizes[i], key16, nonce);
      FIO_ASSERT(!fio_aes128_gcm_dec(mac, buffer, pt_len, aad, aad_sizes[i], key16, nonce),
                 "AES-128-GCM AAD length %zu auth failed", aad_sizes[i]);
      FIO_ASSERT(!memcmp(buffer, plaintext, pt_len),
                 "AES-128-GCM AAD length %zu roundtrip failed", aad_sizes[i]);
    }
  }

  /* Corrupted ciphertext, AAD, and wrong key must fail */
  {
    size_t len = 32;
    uint8_t key2[16];
    uint8_t bad_aad[16];
    fio_rand_bytes(plaintext, len);
    fio_rand_bytes(aad, 16);
    FIO_MEMCPY(bad_aad, aad, 16);
    bad_aad[0] ^= 0x01;
    FIO_MEMCPY(buffer, plaintext, len);

    fio_aes128_gcm_enc(mac, buffer, len, aad, 16, key16, nonce);

    buffer[0] ^= 0x01;
    FIO_ASSERT(fio_aes128_gcm_dec(mac, buffer, len, aad, 16, key16, nonce),
               "AES-128-GCM corrupted ciphertext should fail");
    buffer[0] ^= 0x01;

    FIO_ASSERT(fio_aes128_gcm_dec(mac, buffer, len, bad_aad, 16, key16, nonce),
               "AES-128-GCM corrupted AAD should fail");

    fio_rand_bytes(key2, 16);
    FIO_ASSERT(fio_aes128_gcm_dec(mac, buffer, len, aad, 16, key2, nonce),
               "AES-128-GCM wrong key should fail");
  }

  /* Single-byte plaintext */
  {
    uint8_t p = 0x42;
    uint8_t b = p;
    fio_aes128_gcm_enc(mac, &b, 1, NULL, 0, key16, nonce);
    FIO_ASSERT(!fio_aes128_gcm_dec(mac, &b, 1, NULL, 0, key16, nonce),
               "AES-128-GCM single-byte decryption failed");
    FIO_ASSERT(b == p, "AES-128-GCM single-byte roundtrip failed");
  }

  /* All-zero and all-ones key/nonce roundtrips */
  {
    uint8_t zkey[16] = {0};
    uint8_t znonce[12] = {0};
    uint8_t okey[16];
    uint8_t ononce[12];
    FIO_MEMSET(okey, 0xFF, 16);
    FIO_MEMSET(ononce, 0xFF, 12);

    fio_rand_bytes(plaintext, 32);
    FIO_MEMCPY(buffer, plaintext, 32);
    fio_aes128_gcm_enc(mac, buffer, 32, NULL, 0, zkey, znonce);
    FIO_ASSERT(!fio_aes128_gcm_dec(mac, buffer, 32, NULL, 0, zkey, znonce),
               "AES-128-GCM all-zero key/nonce decryption failed");

    FIO_MEMCPY(buffer, plaintext, 32);
    fio_aes128_gcm_enc(mac, buffer, 32, NULL, 0, okey, ononce);
    FIO_ASSERT(!fio_aes128_gcm_dec(mac, buffer, 32, NULL, 0, okey, ononce),
               "AES-128-GCM all-ones key/nonce decryption failed");
  }

  /* Flip every byte of the tag and confirm detection */
  {
    size_t len = 32;
    uint8_t bad_mac[16];
    fio_rand_bytes(plaintext, len);
    FIO_MEMCPY(buffer, plaintext, len);
    fio_aes128_gcm_enc(mac, buffer, len, NULL, 0, key16, nonce);
    for (int i = 0; i < 16; ++i) {
      FIO_MEMCPY(bad_mac, mac, 16);
      bad_mac[i] ^= 0x01;
      FIO_ASSERT(fio_aes128_gcm_dec(bad_mac, buffer, len, NULL, 0, key16, nonce),
                 "AES-128-GCM flipped MAC byte %d should fail", i);
    }
  }

  /* Deterministic encryption */
  {
    uint8_t dkey[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    uint8_t dnonce[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                          0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
    char msg[] = "deterministic aes-gcm test!!!!!";
    char buf1[64], buf2[64];
    uint8_t mac1[16], mac2[16];
    size_t msg_len = FIO_STRLEN(msg);
    FIO_MEMCPY(buf1, msg, msg_len);
    FIO_MEMCPY(buf2, msg, msg_len);
    fio_aes128_gcm_enc(mac1, buf1, msg_len, NULL, 0, dkey, dnonce);
    fio_aes128_gcm_enc(mac2, buf2, msg_len, NULL, 0, dkey, dnonce);
    FIO_ASSERT(!memcmp(buf1, buf2, msg_len),
               "AES-128-GCM encryption should be deterministic");
    FIO_ASSERT(!memcmp(mac1, mac2, 16),
               "AES-128-GCM tag should be deterministic");
  }

  /* AES-128 and AES-256 must differ for the same input */
  {
    size_t len = 32;
    uint8_t buf128[32], buf256[32];
    uint8_t mac128[16], mac256[16];
    fio_rand_bytes(plaintext, len);
    FIO_MEMCPY(buf128, plaintext, len);
    FIO_MEMCPY(buf256, plaintext, len);
    fio_aes128_gcm_enc(mac128, buf128, len, NULL, 0, key16, nonce);
    fio_aes256_gcm_enc(mac256, buf256, len, NULL, 0, key32, nonce);
    FIO_ASSERT(memcmp(buf128, buf256, len),
               "AES-128 and AES-256 ciphertexts should differ");
    FIO_ASSERT(memcmp(mac128, mac256, 16),
               "AES-128 and AES-256 tags should differ");
  }
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr, "AES GHASH implementation: %s\n", FIO___AES_GHASH_IMPL);
  FIO_NAME_TEST(stl, aes128_gcm_kat)();
  FIO_NAME_TEST(stl, aes256_gcm_kat)();
  FIO_NAME_TEST(stl, aes_gcm_edges)();
  return 0;
}
