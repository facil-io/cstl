/* *****************************************************************************
Test for 152 chacha20poly1305.h

Coverage: ChaCha20 stream cipher, Poly1305 one-time authenticator,
ChaCha20-Poly1305 AEAD, and XChaCha20-Poly1305 (extended nonce) using known
vectors from RFC 7539 and the XChaCha20 draft. Includes deterministic edge-case
roundtrips and authentication-failure detection. Performance loops are
intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CHACHA
#include FIO_INCLUDE_FILE

/* *****************************************************************************
ChaCha20 Known-Answer Tests (RFC 7539)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, chacha20_kat)(void) {
  struct {
    const char *name;
    const uint8_t key[32];
    const uint8_t nonce[12];
    uint32_t counter;
    const char *plaintext;
    const uint8_t *expected;
    size_t expected_len;
  } data[] = {
      {
          .name = "RFC 7539 all-zero key/nonce",
          .key = {0},
          .nonce = {0},
          .counter = 0,
          .plaintext =
              "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
              "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
          .expected =
              (const uint8_t *)
              "\x3a\xeb\x52\x24\xec\xf8\x49\x92\x9b\x9d\x82\x8d\xb1"
              "\xce\xd4\xdd\x83\x20\x25\xe8\x01\x8b\x81\x60\xb8\x22"
              "\x84\xf3\xc9\x49\xaa\x5a\x8e\xca\x00\xbb\xb4\xa7\x3b"
              "\xda\xd1\x92\xb5\xc4\x2f\x73\xf2\xfd\x4e\x27\x36\x44"
              "\xc8\xb3\x61\x25\xa6\x4a\xdd\xeb\x00\x6c\x13\xa0",
          .expected_len = 64,
      },
      {
          .name = "RFC 7539 Section 2.4.2",
          .key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f},
          .nonce = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x4a, 0x00, 0x00, 0x00, 0x00},
          .counter = 1,
          .plaintext =
              "Ladies and Gentlemen of the class of '99: If I could offer "
              "you only one tip for the future, sunscreen would be it.",
          .expected =
              (const uint8_t *)
              "\x6e\x2e\x35\x9a\x25\x68\xf9\x80\x41\xba\x07\x28\xdd"
              "\x0d\x69\x81\xe9\x7e\x7a\xec\x1d\x43\x60\xc2\x0a\x27"
              "\xaf\xcc\xfd\x9f\xae\x0b\xf9\x1b\x65\xc5\x52\x47\x33"
              "\xab\x8f\x59\x3d\xab\xcd\x62\xb3\x57\x16\x39\xd6\x24"
              "\xe6\x51\x52\xab\x8f\x53\x0c\x35\x9f\x08\x61\xd8\x07"
              "\xca\x0d\xbf\x50\x0d\x6a\x61\x56\xa3\x8e\x08\x8a\x22"
              "\xb6\x5e\x52\xbc\x51\x4d\x16\xcc\xf8\x06\x81\x8c\xe9"
              "\x1a\xb7\x79\x37\x36\x5a\xf9\x0b\xbf\x74\xa3\x5b\xe6"
              "\xb4\x0b\x8e\xed\xf2\x78\x5e\x42\x87\x4d",
          .expected_len = 114,
      },
  };

  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    char buffer[256];
    size_t len = FIO_STRLEN(data[i].plaintext);
    FIO_ASSERT(len <= sizeof(buffer), "chacha20 test vector too long");
    FIO_MEMCPY(buffer, data[i].plaintext, len);
    fio_chacha20(buffer, len, data[i].key, data[i].nonce, data[i].counter);
    FIO_ASSERT(!memcmp(buffer, data[i].expected, len),
               "ChaCha20 KAT %s encryption mismatch",
               data[i].name);
    fio_chacha20(buffer, len, data[i].key, data[i].nonce, data[i].counter);
    FIO_ASSERT(!memcmp(buffer, data[i].plaintext, len),
               "ChaCha20 KAT %s decryption roundtrip failed",
               data[i].name);
  }
}

/* *****************************************************************************
Poly1305 Known-Answer Test (RFC 7539)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, poly1305_kat)(void) {
  const uint8_t key[32] = {0x85, 0xd6, 0xbe, 0x78, 0x57, 0x55, 0x6d, 0x33,
                           0x7f, 0x44, 0x52, 0xfe, 0x42, 0xd5, 0x06, 0xa8,
                           0x01, 0x03, 0x80, 0x8a, 0xfb, 0x0d, 0xb2, 0xfd,
                           0x4a, 0xbf, 0xf6, 0xaf, 0x41, 0x49, 0xf5, 0x1b};
  const char *msg = "Cryptographic Forum Research Group";
  const uint8_t expected[16] = {0xa8, 0x06, 0x1d, 0xc1, 0x30, 0x51, 0x36,
                                0xc6, 0xc2, 0x2b, 0x8b, 0xaf, 0x0c, 0x01,
                                0x27, 0xa9};
  uint8_t auth[24] = {0};

  fio_poly1305_auth(auth, (char *)msg, FIO_STRLEN(msg), NULL, 0, key);
  FIO_ASSERT(!memcmp(auth, expected, 16),
             "Poly1305 RFC 7539 authentication mismatch");
  FIO_ASSERT(!fio_buf2u64u(auth + 16),
             "Poly1305 wrote beyond the 16-byte MAC");
}

/* *****************************************************************************
ChaCha20-Poly1305 AEAD Known-Answer Tests (RFC 7539)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, chacha20_poly1305_kat)(void) {
  struct {
    const char *name;
    const uint8_t key[32];
    const uint8_t nonce[12];
    const uint8_t *ad;
    size_t ad_len;
    const char *msg;
    const uint8_t *expected;
    const uint8_t mac[16];
  } data[] = {
      {
          .name = "RFC 7539 test vector 1",
          .key = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
                  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
                  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
                  0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f},
          .nonce = {0x07, 0x00, 0x00, 0x00, 0x40, 0x41,
                    0x42, 0x43, 0x44, 0x45, 0x46, 0x47},
          .ad = (const uint8_t[]){0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1,
                                  0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7},
          .ad_len = 12,
          .msg =
              "Ladies and Gentlemen of the class of '99: If I could offer "
              "you only one tip for the future, sunscreen would be it.",
          .expected =
              (const uint8_t *)
              "\xd3\x1a\x8d\x34\x64\x8e\x60\xdb\x7b\x86\xaf\xbc\x53"
              "\xef\x7e\xc2\xa4\xad\xed\x51\x29\x6e\x08\xfe\xa9\xe2"
              "\xb5\xa7\x36\xee\x62\xd6\x3d\xbe\xa4\x5e\x8c\xa9\x67"
              "\x12\x82\xfa\xfb\x69\xda\x92\x72\x8b\x1a\x71\xde\x0a"
              "\x9e\x06\x0b\x29\x05\xd6\xa5\xb6\x7e\xcd\x3b\x36\x92"
              "\xdd\xbd\x7f\x2d\x77\x8b\x8c\x98\x03\xae\xe3\x28\x09"
              "\x1b\x58\xfa\xb3\x24\xe4\xfa\xd6\x75\x94\x55\x85\x80"
              "\x8b\x48\x31\xd7\xbc\x3f\xf4\xde\xf0\x8e\x4b\x7a\x9d"
              "\xe5\x76\xd2\x65\x86\xce\xc6\x4b\x61\x16",
          .mac = {0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a,
                  0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91},
      },
      {
          .name = "RFC 7539 test vector 2",
          .key = {0x1c, 0x92, 0x40, 0xa5, 0xeb, 0x55, 0xd3, 0x8a,
                  0xf3, 0x33, 0x88, 0x86, 0x04, 0xf6, 0xb5, 0xf0,
                  0x47, 0x39, 0x17, 0xc1, 0x40, 0x2b, 0x80, 0x09,
                  0x9d, 0xca, 0x5c, 0xbc, 0x20, 0x70, 0x75, 0xc0},
          .nonce = {0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
                    0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
          .ad = (const uint8_t[]){0xf3, 0x33, 0x88, 0x86, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x4e, 0x91},
          .ad_len = 12,
          .msg =
              "Internet-Drafts are draft documents valid for a maximum of "
              "six months and may be updated, replaced, or obsoleted by "
              "other documents at any time. It is inappropriate to use "
              "Internet-Drafts as reference material or to cite them other "
              "than as /\xe2\x80\x9cwork in progress./\xe2\x80\x9d",
          .expected =
              (const uint8_t *)
              "\x64\xa0\x86\x15\x75\x86\x1a\xf4\x60\xf0\x62\xc7\x9b"
              "\xe6\x43\xbd\x5e\x80\x5c\xfd\x34\x5c\xf3\x89\xf1\x08"
              "\x67\x0a\xc7\x6c\x8c\xb2\x4c\x6c\xfc\x18\x75\x5d\x43"
              "\xee\xa0\x9e\xe9\x4e\x38\x2d\x26\xb0\xbd\xb7\xb7\x3c"
              "\x32\x1b\x01\x00\xd4\xf0\x3b\x7f\x35\x58\x94\xcf\x33"
              "\x2f\x83\x0e\x71\x0b\x97\xce\x98\xc8\xa8\x4a\xbd\x0b"
              "\x94\x81\x14\xad\x17\x6e\x00\x8d\x33\xbd\x60\xf9\x82"
              "\xb1\xff\x37\xc8\x55\x97\x97\xa0\x6e\xf4\xf0\xef\x61"
              "\xc1\x86\x32\x4e\x2b\x35\x06\x38\x36\x06\x90\x7b\x6a"
              "\x7c\x02\xb0\xf9\xf6\x15\x7b\x53\xc8\x67\xe4\xb9\x16"
              "\x6c\x76\x7b\x80\x4d\x46\xa5\x9b\x52\x16\xcd\xe7\xa4"
              "\xe9\x90\x40\xc5\xa4\x04\x33\x22\x5e\xe2\x82\xa1\xb0"
              "\xa0\x6c\x52\x3e\xaf\x45\x34\xd7\xf8\x3f\xa1\x15\x5b"
              "\x00\x47\x71\x8c\xbc\x54\x6a\x0d\x07\x2b\x04\xb3\x56"
              "\x4e\xea\x1b\x42\x22\x73\xf5\x48\x27\x1a\x0b\xb2\x31"
              "\x60\x53\xfa\x76\x99\x19\x55\xeb\xd6\x31\x59\x43\x4e"
              "\xce\xbb\x4e\x46\x6d\xae\x5a\x10\x73\xa6\x72\x76\x27"
              "\x09\x7a\x10\x49\xe6\x17\xd9\x1d\x36\x10\x94\xfa\x68"
              "\xf0\xff\x77\x98\x71\x30\x30\x5b\xea\xba\x2e\xda\x04"
              "\xdf\x99\x7b\x71\x4d\x6c\x6f\x2c\x29\xa6\xad\x5c\xb4"
              "\x02\x2b\x02\x70\x9b",
          .mac = {0xee, 0xad, 0x9d, 0x67, 0x89, 0x0c, 0xbb, 0x22,
                  0x39, 0x23, 0x36, 0xfe, 0xa1, 0x85, 0x1f, 0x38},
      },
  };

  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    size_t len = FIO_STRLEN(data[i].msg);
    char buffer[512];
    uint8_t mac[24] = {0};
    FIO_ASSERT(len <= sizeof(buffer), "chacha20-poly1305 test vector too long");
    FIO_MEMCPY(buffer, data[i].msg, len);

    fio_chacha20_poly1305_enc(
        mac, buffer, len, data[i].ad, data[i].ad_len, data[i].key, data[i].nonce);
    FIO_ASSERT(!memcmp(buffer, data[i].expected, len),
               "ChaCha20-Poly1305 %s ciphertext mismatch",
               data[i].name);
    FIO_ASSERT(!memcmp(mac, data[i].mac, 16),
               "ChaCha20-Poly1305 %s tag mismatch",
               data[i].name);

    FIO_ASSERT(!fio_chacha20_poly1305_dec(
                   mac, buffer, len, data[i].ad, data[i].ad_len, data[i].key, data[i].nonce),
               "ChaCha20-Poly1305 %s decryption authentication failed",
               data[i].name);
    FIO_ASSERT(!memcmp(buffer, data[i].msg, len),
               "ChaCha20-Poly1305 %s decryption roundtrip failed",
               data[i].name);
    FIO_ASSERT(!fio_buf2u64u(mac + 16),
               "ChaCha20-Poly1305 %s wrote beyond the 16-byte MAC",
               data[i].name);
  }
}

/* *****************************************************************************
XChaCha20-Poly1305 Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, xchacha20_poly1305_kat)(void) {
  /* HChaCha20 subkey derivation vector from draft-irtf-cfrg-xchacha */
  const uint8_t h_key[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                             0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                             0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  const uint8_t h_nonce[16] = {0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a,
                               0x00, 0x00, 0x00, 0x00, 0x31, 0x41, 0x59, 0x27};
  const uint8_t expected_subkey[32] = {
      0x82, 0x41, 0x3b, 0x42, 0x27, 0xb2, 0x7b, 0xfe, 0xd3, 0x0e, 0x42,
      0x50, 0x8a, 0x87, 0x7d, 0x73, 0xa0, 0xf9, 0xe4, 0xd5, 0x8a, 0x74,
      0xa8, 0x53, 0xc1, 0x2e, 0xc4, 0x13, 0x26, 0xd3, 0xec, 0xdc};
  uint8_t subkey[32];
  fio___hchacha20(subkey, h_key, h_nonce);
  FIO_ASSERT(!memcmp(subkey, expected_subkey, 32),
             "HChaCha20 subkey derivation mismatch");

  /* XChaCha20-Poly1305 AEAD vector from draft-irtf-cfrg-xchacha */
  const uint8_t key[32] = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
                           0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
                           0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
                           0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f};
  const uint8_t nonce[24] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
                             0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
                             0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57};
  const uint8_t aad[12] = {0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1,
                           0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7};
  const char *plaintext =
      "Ladies and Gentlemen of the class of '99: If I could offer you only "
      "one tip for the future, sunscreen would be it.";
  const uint8_t expected_tag[16] = {0xc0, 0x87, 0x59, 0x24, 0xc1, 0xc7,
                                    0x98, 0x79, 0x47, 0xde, 0xaf, 0xd8,
                                    0x78, 0x0a, 0xcf, 0x49};
  const uint8_t expected_ciphertext[] = {
      0xbd, 0x6d, 0x17, 0x9d, 0x3e, 0x83, 0xd4, 0x3b, 0x95, 0x76, 0x57, 0x94,
      0x93, 0xc0, 0xe9, 0x39, 0x57, 0x2a, 0x17, 0x00, 0x25, 0x2b, 0xfa, 0xcc,
      0xbe, 0xd2, 0x90, 0x2c, 0x21, 0x39, 0x6c, 0xbb, 0x73, 0x1c, 0x7f, 0x1b,
      0x0b, 0x4a, 0xa6, 0x44, 0x0b, 0xf3, 0xa8, 0x2f, 0x4e, 0xda, 0x7e, 0x39,
      0xae, 0x64, 0xc6, 0x70, 0x8c, 0x54, 0xc2, 0x16, 0xcb, 0x96, 0xb7, 0x2e,
      0x12, 0x13, 0xb4, 0x52, 0x2f, 0x8c, 0x9b, 0xa4, 0x0d, 0xb5, 0xd9, 0x45,
      0xb1, 0x1b, 0x69, 0xb9, 0x82, 0xc1, 0xbb, 0x9e, 0x3f, 0x3f, 0xac, 0x2b,
      0xc3, 0x69, 0x48, 0x8f, 0x76, 0xb2, 0x38, 0x35, 0x65, 0xd3, 0xff, 0xf9,
      0x21, 0xf9, 0x66, 0x4c, 0x97, 0x63, 0x7d, 0xa9, 0x76, 0x88, 0x12, 0xf6,
      0x15, 0xc6, 0x8b, 0x13, 0xb5, 0x2e};
  size_t len = FIO_STRLEN(plaintext);
  char buffer[256];
  uint8_t mac[16];

  FIO_ASSERT(len <= sizeof(buffer), "XChaCha20-Poly1305 test vector too long");
  FIO_MEMCPY(buffer, plaintext, len);
  fio_xchacha20_poly1305_enc(mac, buffer, len, aad, sizeof(aad), key, nonce);

  FIO_ASSERT(!memcmp(buffer, expected_ciphertext, len),
             "XChaCha20-Poly1305 ciphertext mismatch");
  FIO_ASSERT(!memcmp(mac, expected_tag, 16),
             "XChaCha20-Poly1305 tag mismatch");

  FIO_ASSERT(!fio_xchacha20_poly1305_dec(mac, buffer, len, aad, sizeof(aad), key, nonce),
             "XChaCha20-Poly1305 decryption authentication failed");
  FIO_ASSERT(!memcmp(buffer, plaintext, len),
             "XChaCha20-Poly1305 decryption roundtrip failed");
}

/* *****************************************************************************
ChaCha20-Poly1305 Deterministic Edge Cases
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, chacha20_poly1305_edges)(void) {
  const size_t max_len = 4096;
  uint8_t key[32], nonce[12], mac[16];
  uint8_t plaintext[4096], buffer[4096], aad[256];
  size_t sizes[] = {0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 1024, 4096};

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 12);
  fio_rand_bytes(plaintext, sizeof(plaintext));

  /* Empty plaintext with non-empty AAD */
  {
    fio_chacha20_poly1305_enc(mac, NULL, 0, aad, 16, key, nonce);
    int non_zero = 0;
    for (int i = 0; i < 16; ++i)
      non_zero |= mac[i];
    FIO_ASSERT(non_zero, "empty plaintext should produce a non-zero MAC");
    FIO_ASSERT(!fio_chacha20_poly1305_dec(mac, NULL, 0, aad, 16, key, nonce),
               "empty plaintext decryption should succeed");
  }

  /* Empty AAD with non-empty plaintext */
  {
    FIO_MEMCPY(buffer, plaintext, 32);
    fio_chacha20_poly1305_enc(mac, buffer, 32, NULL, 0, key, nonce);
    FIO_ASSERT(!fio_chacha20_poly1305_dec(mac, buffer, 32, NULL, 0, key, nonce),
               "empty AAD decryption should succeed");
    FIO_ASSERT(!memcmp(buffer, plaintext, 32), "empty AAD roundtrip failed");
  }

  /* Both plaintext and AAD empty */
  {
    fio_chacha20_poly1305_enc(mac, NULL, 0, NULL, 0, key, nonce);
    FIO_ASSERT(!fio_chacha20_poly1305_dec(mac, NULL, 0, NULL, 0, key, nonce),
               "all-empty decryption should succeed");
  }

  /* Vary plaintext length across block boundaries */
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t len = sizes[i];
    FIO_ASSERT(len <= max_len, "edge-case size exceeds buffer");
    fio_rand_bytes(plaintext, len ? len : 1);
    FIO_MEMCPY(buffer, plaintext, len);
    fio_chacha20_poly1305_enc(mac, buffer, len, NULL, 0, key, nonce);
    FIO_ASSERT(!fio_chacha20_poly1305_dec(mac, buffer, len, NULL, 0, key, nonce),
               "ChaCha20-Poly1305 length %zu decryption auth failed", len);
    FIO_ASSERT(!memcmp(buffer, plaintext, len),
               "ChaCha20-Poly1305 length %zu roundtrip failed", len);
  }

  /* Vary AAD length */
  {
    size_t pt_len = 32;
    fio_rand_bytes(plaintext, pt_len);
    size_t aad_sizes[] = {0, 1, 15, 16, 17, 31, 32, 33, 255, 256};
    for (size_t i = 0; i < sizeof(aad_sizes) / sizeof(aad_sizes[0]); ++i) {
      FIO_MEMCPY(buffer, plaintext, pt_len);
      fio_chacha20_poly1305_enc(mac, buffer, pt_len, aad, aad_sizes[i], key, nonce);
      FIO_ASSERT(!fio_chacha20_poly1305_dec(mac, buffer, pt_len, aad, aad_sizes[i], key, nonce),
                 "ChaCha20-Poly1305 AAD length %zu auth failed", aad_sizes[i]);
      FIO_ASSERT(!memcmp(buffer, plaintext, pt_len),
                 "ChaCha20-Poly1305 AAD length %zu roundtrip failed",
                 aad_sizes[i]);
    }
  }

  /* Corrupted ciphertext, tag, and AAD must fail authentication */
  {
    size_t len = 32;
    uint8_t bad_aad[16];
    fio_rand_bytes(plaintext, len);
    fio_rand_bytes(aad, 16);
    FIO_MEMCPY(bad_aad, aad, 16);
    bad_aad[0] ^= 0x01;
    FIO_MEMCPY(buffer, plaintext, len);
    fio_chacha20_poly1305_enc(mac, buffer, len, aad, 16, key, nonce);

    buffer[0] ^= 0x01;
    FIO_ASSERT(fio_chacha20_poly1305_dec(mac, buffer, len, aad, 16, key, nonce),
               "corrupted ciphertext should fail verification");
    buffer[0] ^= 0x01;

    mac[0] ^= 0x01;
    FIO_ASSERT(fio_chacha20_poly1305_dec(mac, buffer, len, aad, 16, key, nonce),
               "corrupted tag should fail verification");
    mac[0] ^= 0x01;

    FIO_ASSERT(fio_chacha20_poly1305_dec(mac, buffer, len, bad_aad, 16, key, nonce),
               "corrupted AAD should fail verification");
  }

  /* Wrong key and wrong nonce must fail */
  {
    size_t len = 32;
    uint8_t key2[32], nonce2[12];
    fio_rand_bytes(key2, 32);
    fio_rand_bytes(nonce2, 12);
    fio_rand_bytes(plaintext, len);
    FIO_MEMCPY(buffer, plaintext, len);
    fio_chacha20_poly1305_enc(mac, buffer, len, NULL, 0, key, nonce);

    FIO_ASSERT(fio_chacha20_poly1305_dec(mac, buffer, len, NULL, 0, key2, nonce),
               "wrong key should fail verification");
    FIO_ASSERT(fio_chacha20_poly1305_dec(mac, buffer, len, NULL, 0, key, nonce2),
               "wrong nonce should fail verification");
  }

  /* Deterministic encryption: identical inputs produce identical outputs */
  {
    uint8_t key3[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
                        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};
    uint8_t nonce3[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                          0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
    char msg[] = "deterministic chacha20poly1305";
    char buf1[64], buf2[64];
    uint8_t mac1[16], mac2[16];
    size_t msg_len = FIO_STRLEN(msg);
    FIO_MEMCPY(buf1, msg, msg_len);
    FIO_MEMCPY(buf2, msg, msg_len);
    fio_chacha20_poly1305_enc(mac1, buf1, msg_len, NULL, 0, key3, nonce3);
    fio_chacha20_poly1305_enc(mac2, buf2, msg_len, NULL, 0, key3, nonce3);
    FIO_ASSERT(!memcmp(buf1, buf2, msg_len),
               "ChaCha20-Poly1305 encryption should be deterministic");
    FIO_ASSERT(!memcmp(mac1, mac2, 16), "ChaCha20-Poly1305 tag should be deterministic");
  }

  /* Single-byte plaintext */
  {
    char p = 0x42;
    char b = p;
    fio_chacha20_poly1305_enc(mac, &b, 1, NULL, 0, key, nonce);
    FIO_ASSERT(!fio_chacha20_poly1305_dec(mac, &b, 1, NULL, 0, key, nonce),
               "single-byte decryption failed");
    FIO_ASSERT(b == p, "single-byte roundtrip failed");
  }

  /* All-zero and all-ones key/nonce roundtrips */
  {
    uint8_t zkey[32] = {0};
    uint8_t znonce[12] = {0};
    uint8_t okey[32];
    uint8_t ononce[12];
    FIO_MEMSET(okey, 0xFF, 32);
    FIO_MEMSET(ononce, 0xFF, 12);

    FIO_MEMCPY(buffer, plaintext, 32);
    fio_chacha20_poly1305_enc(mac, buffer, 32, NULL, 0, zkey, znonce);
    FIO_ASSERT(!fio_chacha20_poly1305_dec(mac, buffer, 32, NULL, 0, zkey, znonce),
               "all-zero key/nonce decryption failed");

    FIO_MEMCPY(buffer, plaintext, 32);
    fio_chacha20_poly1305_enc(mac, buffer, 32, NULL, 0, okey, ononce);
    FIO_ASSERT(!fio_chacha20_poly1305_dec(mac, buffer, 32, NULL, 0, okey, ononce),
               "all-ones key/nonce decryption failed");
  }

  /* Flip every byte of the tag and confirm detection */
  {
    size_t len = 32;
    uint8_t bad_mac[16];
    fio_rand_bytes(plaintext, len);
    FIO_MEMCPY(buffer, plaintext, len);
    fio_chacha20_poly1305_enc(mac, buffer, len, NULL, 0, key, nonce);
    for (int i = 0; i < 16; ++i) {
      FIO_MEMCPY(bad_mac, mac, 16);
      bad_mac[i] ^= 0x01;
      FIO_ASSERT(fio_chacha20_poly1305_dec(bad_mac, buffer, len, NULL, 0, key, nonce),
                 "flipped MAC byte %d should fail verification", i);
    }
  }
}

/* *****************************************************************************
XChaCha20-Poly1305 Deterministic Edge Cases
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, xchacha20_poly1305_edges)(void) {
  uint8_t key[32], nonce[24], mac[16];
  uint8_t plaintext[1024], buffer[1024], aad[32];
  size_t sizes[] = {0, 1, 15, 16, 17, 63, 64, 65, 127, 128, 129, 255, 256, 1024};

  fio_rand_bytes(key, 32);
  fio_rand_bytes(nonce, 24);
  fio_rand_bytes(plaintext, sizeof(plaintext));

  /* Empty plaintext roundtrip */
  {
    fio_xchacha20_poly1305_enc(mac, NULL, 0, aad, 16, key, nonce);
    int non_zero = 0;
    for (int i = 0; i < 16; ++i)
      non_zero |= mac[i];
    FIO_ASSERT(non_zero, "XChaCha20-Poly1305 empty MAC should be non-zero");
    FIO_ASSERT(!fio_xchacha20_poly1305_dec(mac, NULL, 0, aad, 16, key, nonce),
               "XChaCha20-Poly1305 empty plaintext decryption failed");
  }

  /* Empty AAD roundtrip */
  {
    FIO_MEMCPY(buffer, plaintext, 32);
    fio_xchacha20_poly1305_enc(mac, buffer, 32, NULL, 0, key, nonce);
    FIO_ASSERT(!fio_xchacha20_poly1305_dec(mac, buffer, 32, NULL, 0, key, nonce),
               "XChaCha20-Poly1305 empty AAD decryption failed");
  }

  /* Vary plaintext length */
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t len = sizes[i];
    fio_rand_bytes(plaintext, len ? len : 1);
    FIO_MEMCPY(buffer, plaintext, len);
    fio_xchacha20_poly1305_enc(mac, buffer, len, aad, 32, key, nonce);
    FIO_ASSERT(!fio_xchacha20_poly1305_dec(mac, buffer, len, aad, 32, key, nonce),
               "XChaCha20-Poly1305 length %zu auth failed", len);
    FIO_ASSERT(!memcmp(buffer, plaintext, len),
               "XChaCha20-Poly1305 length %zu roundtrip failed", len);
  }

  /* Corruption detection */
  {
    size_t len = 32;
    fio_rand_bytes(plaintext, len);
    FIO_MEMCPY(buffer, plaintext, len);
    fio_xchacha20_poly1305_enc(mac, buffer, len, NULL, 0, key, nonce);

    buffer[0] ^= 0x01;
    FIO_ASSERT(fio_xchacha20_poly1305_dec(mac, buffer, len, NULL, 0, key, nonce),
               "XChaCha20-Poly1305 corrupted ciphertext should fail");
    buffer[0] ^= 0x01;

    mac[0] ^= 0x01;
    FIO_ASSERT(fio_xchacha20_poly1305_dec(mac, buffer, len, NULL, 0, key, nonce),
               "XChaCha20-Poly1305 corrupted tag should fail");
    mac[0] ^= 0x01;

    uint8_t key2[32], nonce2[24];
    fio_rand_bytes(key2, 32);
    fio_rand_bytes(nonce2, 24);
    FIO_ASSERT(fio_xchacha20_poly1305_dec(mac, buffer, len, NULL, 0, key2, nonce),
               "XChaCha20-Poly1305 wrong key should fail");
    FIO_ASSERT(fio_xchacha20_poly1305_dec(mac, buffer, len, NULL, 0, key, nonce2),
               "XChaCha20-Poly1305 wrong nonce should fail");
  }

  /* Determinism */
  {
    uint8_t key3[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
                        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};
    uint8_t nonce3[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                          0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                          0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
    char msg[] = "deterministic xchacha20poly1305";
    char buf1[64], buf2[64];
    uint8_t mac1[16], mac2[16];
    size_t msg_len = FIO_STRLEN(msg);
    FIO_MEMCPY(buf1, msg, msg_len);
    FIO_MEMCPY(buf2, msg, msg_len);
    fio_xchacha20_poly1305_enc(mac1, buf1, msg_len, NULL, 0, key3, nonce3);
    fio_xchacha20_poly1305_enc(mac2, buf2, msg_len, NULL, 0, key3, nonce3);
    FIO_ASSERT(!memcmp(buf1, buf2, msg_len),
               "XChaCha20-Poly1305 encryption should be deterministic");
    FIO_ASSERT(!memcmp(mac1, mac2, 16),
               "XChaCha20-Poly1305 tag should be deterministic");
  }

  /* XChaCha20 standalone roundtrip */
  {
    fio_rand_bytes(plaintext, 128);
    FIO_MEMCPY(buffer, plaintext, 128);
    fio_xchacha20(buffer, 128, key, nonce, 0);
    FIO_ASSERT(memcmp(buffer, plaintext, 128),
               "XChaCha20 ciphertext should differ from plaintext");
    fio_xchacha20(buffer, 128, key, nonce, 0);
    FIO_ASSERT(!memcmp(buffer, plaintext, 128),
               "XChaCha20 roundtrip failed");
  }
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  FIO_NAME_TEST(stl, chacha20_kat)();
  FIO_NAME_TEST(stl, poly1305_kat)();
  FIO_NAME_TEST(stl, chacha20_poly1305_kat)();
  FIO_NAME_TEST(stl, xchacha20_poly1305_kat)();
  FIO_NAME_TEST(stl, chacha20_poly1305_edges)();
  FIO_NAME_TEST(stl, xchacha20_poly1305_edges)();
  return 0;
}
