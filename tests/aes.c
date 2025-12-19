/* *****************************************************************************
Test - AES-GCM
***************************************************************************** */
#include "test-helpers.h"

#define FIO_AES
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL
#include <openssl/evp.h>
#endif

/* Speed test wrapper for AES-GCM encryption */
FIO_SFUNC uintptr_t fio__aes128_gcm_speed_wrapper(char *msg, size_t len) {
  uint8_t mac[16];
  uint8_t key[16] = {0x00,
                     0x01,
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
                     0x0f};
  uint8_t nonce[12] =
      {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
  fio_aes128_gcm_enc(mac, msg, len, NULL, 0, key, nonce);
  return (uintptr_t)fio_buf2u64u(mac);
}

FIO_SFUNC uintptr_t fio__aes256_gcm_speed_wrapper(char *msg, size_t len) {
  uint8_t mac[16];
  uint8_t key[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                     0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                     0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  uint8_t nonce[12] =
      {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
  fio_aes256_gcm_enc(mac, msg, len, NULL, 0, key, nonce);
  return (uintptr_t)fio_buf2u64u(mac);
}

/* Helper to print hex for debugging */
FIO_SFUNC void print_hex(const char *label, const uint8_t *data, size_t len) {
  fprintf(stderr, "%s: ", label);
  for (size_t i = 0; i < len; ++i)
    fprintf(stderr, "%02x", data[i]);
  fprintf(stderr, "\n");
}

int main(void) {
  FIO_LOG_DDEBUG("Testing AES-GCM implementation");
#if FIO___HAS_X86_AES_INTRIN
  FIO_LOG_DDEBUG("(using x86 AES-NI)");
#elif FIO___HAS_ARM_AES_INTRIN
  FIO_LOG_DDEBUG("(using ARM Crypto Extensions)");

#else
  FIO_LOG_DDEBUG("(using software fallback)");
#endif

  /* **************************************************************************
   * Test AES-128-GCM (NIST SP 800-38D test vectors)
   * *************************************************************************/
  {
    FIO_LOG_DDEBUG("Testing AES-128-GCM (NIST test vectors)");

    /* Test Case 1: Empty plaintext, no AAD */
    {
      uint8_t key[16] = {0};
      uint8_t nonce[12] = {0};
      uint8_t expected_tag[16] = {0x58,
                                  0xe2,
                                  0xfc,
                                  0xce,
                                  0xfa,
                                  0x7e,
                                  0x30,
                                  0x61,
                                  0x36,
                                  0x7f,
                                  0x1d,
                                  0x57,
                                  0xa4,
                                  0xe7,
                                  0x45,
                                  0x5a};
      uint8_t mac[16];

      fio_aes128_gcm_enc(mac, NULL, 0, NULL, 0, key, nonce);
      FIO_ASSERT(!memcmp(mac, expected_tag, 16),
                 "AES-128-GCM Test Case 1 failed (empty plaintext)");
    }

    /* Test Case 2: 16-byte plaintext, no AAD */
    {
      uint8_t key[16] = {0};
      uint8_t nonce[12] = {0};
      uint8_t plaintext[16] = {0};
      uint8_t expected_ct[16] = {0x03,
                                 0x88,
                                 0xda,
                                 0xce,
                                 0x60,
                                 0xb6,
                                 0xa3,
                                 0x92,
                                 0xf3,
                                 0x28,
                                 0xc2,
                                 0xb9,
                                 0x71,
                                 0xb2,
                                 0xfe,
                                 0x78};
      uint8_t expected_tag[16] = {0xab,
                                  0x6e,
                                  0x47,
                                  0xd4,
                                  0x2c,
                                  0xec,
                                  0x13,
                                  0xbd,
                                  0xf5,
                                  0x3a,
                                  0x67,
                                  0xb2,
                                  0x12,
                                  0x57,
                                  0xbd,
                                  0xdf};
      uint8_t buffer[16];
      uint8_t mac[16];

      FIO_MEMCPY(buffer, plaintext, 16);
      fio_aes128_gcm_enc(mac, buffer, 16, NULL, 0, key, nonce);

      FIO_ASSERT(!memcmp(buffer, expected_ct, 16),
                 "AES-128-GCM Test Case 2 ciphertext failed");
      FIO_ASSERT(!memcmp(mac, expected_tag, 16),
                 "AES-128-GCM Test Case 2 tag failed");

      /* Test decryption */
      int ret = fio_aes128_gcm_dec(mac, buffer, 16, NULL, 0, key, nonce);
      FIO_ASSERT(ret == 0, "AES-128-GCM Test Case 2 decryption auth failed");
      FIO_ASSERT(!memcmp(buffer, plaintext, 16),
                 "AES-128-GCM Test Case 2 decryption failed");
    }

    /* Test Case 3: 64-byte plaintext, no AAD */
    {
      uint8_t key[16] = {0xfe,
                         0xff,
                         0xe9,
                         0x92,
                         0x86,
                         0x65,
                         0x73,
                         0x1c,
                         0x6d,
                         0x6a,
                         0x8f,
                         0x94,
                         0x67,
                         0x30,
                         0x83,
                         0x08};
      uint8_t nonce[12] = {0xca,
                           0xfe,
                           0xba,
                           0xbe,
                           0xfa,
                           0xce,
                           0xdb,
                           0xad,
                           0xde,
                           0xca,
                           0xf8,
                           0x88};
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
      uint8_t expected_tag[16] = {0x4d,
                                  0x5c,
                                  0x2a,
                                  0xf3,
                                  0x27,
                                  0xcd,
                                  0x64,
                                  0xa6,
                                  0x2c,
                                  0xf3,
                                  0x5a,
                                  0xbd,
                                  0x2b,
                                  0xa6,
                                  0xfa,
                                  0xb4};
      uint8_t buffer[64];
      uint8_t mac[16];

      FIO_MEMCPY(buffer, plaintext, 64);
      fio_aes128_gcm_enc(mac, buffer, 64, NULL, 0, key, nonce);

      FIO_ASSERT(!memcmp(buffer, expected_ct, 64),
                 "AES-128-GCM Test Case 3 ciphertext failed");
      FIO_ASSERT(!memcmp(mac, expected_tag, 16),
                 "AES-128-GCM Test Case 3 tag failed");

      /* Test decryption */
      int ret = fio_aes128_gcm_dec(mac, buffer, 64, NULL, 0, key, nonce);
      FIO_ASSERT(ret == 0, "AES-128-GCM Test Case 3 decryption auth failed");
      FIO_ASSERT(!memcmp(buffer, plaintext, 64),
                 "AES-128-GCM Test Case 3 decryption failed");
    }

    /* Test Case 4: 60-byte plaintext with 20-byte AAD */
    {
      uint8_t key[16] = {0xfe,
                         0xff,
                         0xe9,
                         0x92,
                         0x86,
                         0x65,
                         0x73,
                         0x1c,
                         0x6d,
                         0x6a,
                         0x8f,
                         0x94,
                         0x67,
                         0x30,
                         0x83,
                         0x08};
      uint8_t nonce[12] = {0xca,
                           0xfe,
                           0xba,
                           0xbe,
                           0xfa,
                           0xce,
                           0xdb,
                           0xad,
                           0xde,
                           0xca,
                           0xf8,
                           0x88};
      uint8_t aad[20] = {0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe,
                         0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad,
                         0xbe, 0xef, 0xab, 0xad, 0xda, 0xd2};
      uint8_t plaintext[60] = {
          0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59,
          0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7, 0xa9, 0x53,
          0x15, 0x34, 0xf7, 0xda, 0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31,
          0x8a, 0x72, 0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
          0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25, 0xb1, 0x6a,
          0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57, 0xba, 0x63, 0x7b, 0x39};
      uint8_t expected_ct[60] = {
          0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24, 0x4b, 0x72,
          0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c, 0xe3, 0xaa, 0x21, 0x2f,
          0x2c, 0x02, 0xa4, 0xe0, 0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac,
          0xa1, 0x2e, 0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
          0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05, 0x1b, 0xa3,
          0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97, 0x3d, 0x58, 0xe0, 0x91};
      uint8_t expected_tag[16] = {0x5b,
                                  0xc9,
                                  0x4f,
                                  0xbc,
                                  0x32,
                                  0x21,
                                  0xa5,
                                  0xdb,
                                  0x94,
                                  0xfa,
                                  0xe9,
                                  0x5a,
                                  0xe7,
                                  0x12,
                                  0x1a,
                                  0x47};
      uint8_t buffer[60];
      uint8_t mac[16];

      FIO_MEMCPY(buffer, plaintext, 60);
      fio_aes128_gcm_enc(mac, buffer, 60, aad, 20, key, nonce);

      FIO_ASSERT(!memcmp(buffer, expected_ct, 60),
                 "AES-128-GCM Test Case 4 ciphertext failed");
      FIO_ASSERT(!memcmp(mac, expected_tag, 16),
                 "AES-128-GCM Test Case 4 tag failed");

      /* Test decryption */
      int ret = fio_aes128_gcm_dec(mac, buffer, 60, aad, 20, key, nonce);
      FIO_ASSERT(ret == 0, "AES-128-GCM Test Case 4 decryption auth failed");
      FIO_ASSERT(!memcmp(buffer, plaintext, 60),
                 "AES-128-GCM Test Case 4 decryption failed");
    }
  }

  /* **************************************************************************
   * Test AES-256-GCM (NIST SP 800-38D test vectors)
   * *************************************************************************/
  {
    FIO_LOG_DDEBUG("Testing AES-256-GCM (NIST test vectors)");

    /* Test Case 13: Empty plaintext, no AAD (AES-256) */
    {
      uint8_t key[32] = {0};
      uint8_t nonce[12] = {0};
      uint8_t expected_tag[16] = {0x53,
                                  0x0f,
                                  0x8a,
                                  0xfb,
                                  0xc7,
                                  0x45,
                                  0x36,
                                  0xb9,
                                  0xa9,
                                  0x63,
                                  0xb4,
                                  0xf1,
                                  0xc4,
                                  0xcb,
                                  0x73,
                                  0x8b};
      uint8_t mac[16];

      fio_aes256_gcm_enc(mac, NULL, 0, NULL, 0, key, nonce);
      FIO_ASSERT(!memcmp(mac, expected_tag, 16),
                 "AES-256-GCM Test Case 13 failed (empty plaintext)");
    }

    /* Test Case 14: 16-byte plaintext, no AAD (AES-256) */
    {
      uint8_t key[32] = {0};
      uint8_t nonce[12] = {0};
      uint8_t plaintext[16] = {0};
      uint8_t expected_ct[16] = {0xce,
                                 0xa7,
                                 0x40,
                                 0x3d,
                                 0x4d,
                                 0x60,
                                 0x6b,
                                 0x6e,
                                 0x07,
                                 0x4e,
                                 0xc5,
                                 0xd3,
                                 0xba,
                                 0xf3,
                                 0x9d,
                                 0x18};
      uint8_t expected_tag[16] = {0xd0,
                                  0xd1,
                                  0xc8,
                                  0xa7,
                                  0x99,
                                  0x99,
                                  0x6b,
                                  0xf0,
                                  0x26,
                                  0x5b,
                                  0x98,
                                  0xb5,
                                  0xd4,
                                  0x8a,
                                  0xb9,
                                  0x19};
      uint8_t buffer[16];
      uint8_t mac[16];

      FIO_MEMCPY(buffer, plaintext, 16);
      fio_aes256_gcm_enc(mac, buffer, 16, NULL, 0, key, nonce);

      FIO_ASSERT(!memcmp(buffer, expected_ct, 16),
                 "AES-256-GCM Test Case 14 ciphertext failed");
      FIO_ASSERT(!memcmp(mac, expected_tag, 16),
                 "AES-256-GCM Test Case 14 tag failed");

      /* Test decryption */
      int ret = fio_aes256_gcm_dec(mac, buffer, 16, NULL, 0, key, nonce);
      FIO_ASSERT(ret == 0, "AES-256-GCM Test Case 14 decryption auth failed");
      FIO_ASSERT(!memcmp(buffer, plaintext, 16),
                 "AES-256-GCM Test Case 14 decryption failed");
    }

    /* Test Case 15: 64-byte plaintext, no AAD (AES-256) */
    {
      uint8_t key[32] = {0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                         0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
                         0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                         0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08};
      uint8_t nonce[12] = {0xca,
                           0xfe,
                           0xba,
                           0xbe,
                           0xfa,
                           0xce,
                           0xdb,
                           0xad,
                           0xde,
                           0xca,
                           0xf8,
                           0x88};
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
      uint8_t expected_tag[16] = {0xb0,
                                  0x94,
                                  0xda,
                                  0xc5,
                                  0xd9,
                                  0x34,
                                  0x71,
                                  0xbd,
                                  0xec,
                                  0x1a,
                                  0x50,
                                  0x22,
                                  0x70,
                                  0xe3,
                                  0xcc,
                                  0x6c};
      uint8_t buffer[64];
      uint8_t mac[16];

      FIO_MEMCPY(buffer, plaintext, 64);
      fio_aes256_gcm_enc(mac, buffer, 64, NULL, 0, key, nonce);

      FIO_ASSERT(!memcmp(buffer, expected_ct, 64),
                 "AES-256-GCM Test Case 15 ciphertext failed");
      FIO_ASSERT(!memcmp(mac, expected_tag, 16),
                 "AES-256-GCM Test Case 15 tag failed");

      /* Test decryption */
      int ret = fio_aes256_gcm_dec(mac, buffer, 64, NULL, 0, key, nonce);
      FIO_ASSERT(ret == 0, "AES-256-GCM Test Case 15 decryption auth failed");
      FIO_ASSERT(!memcmp(buffer, plaintext, 64),
                 "AES-256-GCM Test Case 15 decryption failed");
    }

    /* Test Case 16: 60-byte plaintext with 20-byte AAD (AES-256) */
    {
      uint8_t key[32] = {0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                         0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
                         0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
                         0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08};
      uint8_t nonce[12] = {0xca,
                           0xfe,
                           0xba,
                           0xbe,
                           0xfa,
                           0xce,
                           0xdb,
                           0xad,
                           0xde,
                           0xca,
                           0xf8,
                           0x88};
      uint8_t aad[20] = {0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe,
                         0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad,
                         0xbe, 0xef, 0xab, 0xad, 0xda, 0xd2};
      uint8_t plaintext[60] = {
          0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59,
          0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7, 0xa9, 0x53,
          0x15, 0x34, 0xf7, 0xda, 0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31,
          0x8a, 0x72, 0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
          0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25, 0xb1, 0x6a,
          0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57, 0xba, 0x63, 0x7b, 0x39};
      uint8_t expected_ct[60] = {
          0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07, 0xf4, 0x7f,
          0x37, 0xa3, 0x2a, 0x84, 0x42, 0x7d, 0x64, 0x3a, 0x8c, 0xdc,
          0xbf, 0xe5, 0xc0, 0xc9, 0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55,
          0xd1, 0xaa, 0x8c, 0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d,
          0xa7, 0xb0, 0x8b, 0x10, 0x56, 0x82, 0x88, 0x38, 0xc5, 0xf6,
          0x1e, 0x63, 0x93, 0xba, 0x7a, 0x0a, 0xbc, 0xc9, 0xf6, 0x62};
      uint8_t expected_tag[16] = {0x76,
                                  0xfc,
                                  0x6e,
                                  0xce,
                                  0x0f,
                                  0x4e,
                                  0x17,
                                  0x68,
                                  0xcd,
                                  0xdf,
                                  0x88,
                                  0x53,
                                  0xbb,
                                  0x2d,
                                  0x55,
                                  0x1b};
      uint8_t buffer[60];
      uint8_t mac[16];

      FIO_MEMCPY(buffer, plaintext, 60);
      fio_aes256_gcm_enc(mac, buffer, 60, aad, 20, key, nonce);

      FIO_ASSERT(!memcmp(buffer, expected_ct, 60),
                 "AES-256-GCM Test Case 16 ciphertext failed");
      FIO_ASSERT(!memcmp(mac, expected_tag, 16),
                 "AES-256-GCM Test Case 16 tag failed");

      /* Test decryption */
      int ret = fio_aes256_gcm_dec(mac, buffer, 60, aad, 20, key, nonce);
      FIO_ASSERT(ret == 0, "AES-256-GCM Test Case 16 decryption auth failed");
      FIO_ASSERT(!memcmp(buffer, plaintext, 60),
                 "AES-256-GCM Test Case 16 decryption failed");
    }
  }

  /* **************************************************************************
   * Test authentication failure detection
   * *************************************************************************/
  {
    FIO_LOG_DDEBUG("Testing AES-GCM authentication failure detection");
    uint8_t key[16] = {0};
    uint8_t nonce[12] = {0};
    uint8_t plaintext[16] = {0};
    uint8_t buffer[16];
    uint8_t mac[16];
    uint8_t bad_mac[16];

    FIO_MEMCPY(buffer, plaintext, 16);
    fio_aes128_gcm_enc(mac, buffer, 16, NULL, 0, key, nonce);

    /* Corrupt the MAC */
    FIO_MEMCPY(bad_mac, mac, 16);
    bad_mac[0] ^= 0x01;

    int ret = fio_aes128_gcm_dec(bad_mac, buffer, 16, NULL, 0, key, nonce);
    FIO_ASSERT(ret == -1,
               "AES-GCM should detect authentication failure with bad MAC");
  }

  /* **************************************************************************
   * Test roundtrip with random data
   * *************************************************************************/
  {
    FIO_LOG_DDEBUG("Testing AES-GCM roundtrip with random data");
    uint8_t key[32];
    uint8_t nonce[12];
    uint8_t aad[64];
    uint8_t plaintext[256];
    uint8_t buffer[256];
    uint8_t mac[16];

    /* Generate random test data */
    fio_rand_bytes(key, 32);
    fio_rand_bytes(nonce, 12);
    fio_rand_bytes(aad, 64);
    fio_rand_bytes(plaintext, 256);

    /* Test AES-128-GCM roundtrip */
    FIO_MEMCPY(buffer, plaintext, 256);
    fio_aes128_gcm_enc(mac, buffer, 256, aad, 64, key, nonce);
    FIO_ASSERT(memcmp(buffer, plaintext, 256) != 0,
               "AES-128-GCM ciphertext should differ from plaintext");

    int ret = fio_aes128_gcm_dec(mac, buffer, 256, aad, 64, key, nonce);
    FIO_ASSERT(ret == 0, "AES-128-GCM roundtrip decryption auth failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 256),
               "AES-128-GCM roundtrip decryption failed");

    /* Test AES-256-GCM roundtrip */
    FIO_MEMCPY(buffer, plaintext, 256);
    fio_aes256_gcm_enc(mac, buffer, 256, aad, 64, key, nonce);
    FIO_ASSERT(memcmp(buffer, plaintext, 256) != 0,
               "AES-256-GCM ciphertext should differ from plaintext");

    ret = fio_aes256_gcm_dec(mac, buffer, 256, aad, 64, key, nonce);
    FIO_ASSERT(ret == 0, "AES-256-GCM roundtrip decryption auth failed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 256),
               "AES-256-GCM roundtrip decryption failed");
  }

  /* **************************************************************************
   * Test various message lengths
   * *************************************************************************/
  {
    FIO_LOG_DDEBUG("Testing AES-GCM with various message lengths");
    uint8_t key[16] = {0x00,
                       0x01,
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
                       0x0f};
    uint8_t nonce[12] = {0xca,
                         0xfe,
                         0xba,
                         0xbe,
                         0xfa,
                         0xce,
                         0xdb,
                         0xad,
                         0xde,
                         0xca,
                         0xf8,
                         0x88};
    uint8_t plaintext[300];
    uint8_t buffer[300];
    uint8_t mac[16];

    fio_rand_bytes(plaintext, 300);

    /* Test lengths: 0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129 */
    size_t test_lengths[] = {0,
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
                             300};
    for (size_t i = 0; i < sizeof(test_lengths) / sizeof(test_lengths[0]);
         ++i) {
      size_t len = test_lengths[i];
      FIO_MEMCPY(buffer, plaintext, len);
      fio_aes128_gcm_enc(mac, buffer, len, NULL, 0, key, nonce);

      int ret = fio_aes128_gcm_dec(mac, buffer, len, NULL, 0, key, nonce);
      FIO_ASSERT(ret == 0,
                 "AES-128-GCM roundtrip failed for length %zu (auth)",
                 len);
      FIO_ASSERT(!memcmp(buffer, plaintext, len),
                 "AES-128-GCM roundtrip failed for length %zu (data)",
                 len);
    }
  }

  FIO_LOG_DDEBUG("AES-GCM tests passed!");

  /* Performance tests moved to tests/performance-crypto.c */

  return 0;
}
