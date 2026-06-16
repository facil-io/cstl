/* *****************************************************************************
Test for X25519 helpers in 154 ed25519.h

Coverage: X25519 keypair generation, public key derivation, ECDH shared secret
computation using RFC 7748 vectors, ECIES encrypt/decrypt roundtrips with both
ChaCha20-Poly1305 and AES256-GCM, Ed25519-to-X25519 key conversion, and
ciphertext-length macros. Performance loops are intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_ED25519
#define FIO_SHA2
#define FIO_CHACHA
#define FIO_AES
#include FIO_INCLUDE_FILE

/* *****************************************************************************
RFC 7748 X25519 Test Vectors (carry-overflow regression test)
https://www.rfc-editor.org/rfc/rfc7748#section-5.2
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x25519_rfc7748)(void) {
  /* RFC 7748 §5.2 vector 1. */
  {
    static const uint8_t scalar[32] = {
        0xa5, 0x46, 0xe3, 0x6b, 0xf0, 0x52, 0x7c, 0x9d, 0x3b, 0x16, 0x15,
        0x4b, 0x82, 0x46, 0x5e, 0xdd, 0x62, 0x14, 0x4c, 0x0a, 0xc1, 0xfc,
        0x5a, 0x18, 0x50, 0x6a, 0x22, 0x44, 0xba, 0x44, 0x9a, 0xc4};
    static const uint8_t u_in[32] = {
        0xe6, 0xdb, 0x68, 0x67, 0x58, 0x30, 0x30, 0xdb, 0x35, 0x94, 0xc1,
        0xa4, 0x24, 0xb1, 0x5f, 0x7c, 0x72, 0x66, 0x24, 0xec, 0x26, 0xb3,
        0x35, 0x3b, 0x10, 0xa9, 0x03, 0xa6, 0xd0, 0xab, 0x1c, 0x4c};
    static const uint8_t expected[32] = {
        0xc3, 0xda, 0x55, 0x37, 0x9d, 0xe9, 0xc6, 0x90, 0x8e, 0x94, 0xea,
        0x4d, 0xf2, 0x8d, 0x08, 0x4f, 0x32, 0xec, 0xcf, 0x03, 0x49, 0x1c,
        0x71, 0xf7, 0x54, 0xb4, 0x07, 0x55, 0x77, 0xa2, 0x85, 0x52};

    uint8_t result[32];
    FIO_ASSERT(fio_x25519_shared_secret(result, scalar, u_in) == 0,
               "X25519 RFC 7748 §5.2 vector 1 failed");
    FIO_ASSERT(!FIO_MEMCMP(result, expected, 32),
               "X25519 RFC 7748 §5.2 vector 1 mismatch");
  }

  /* RFC 7748 §5.2 vector 2. */
  {
    static const uint8_t scalar[32] = {
        0x4b, 0x66, 0xe9, 0xd4, 0xd1, 0xb4, 0x67, 0x3c, 0x5a, 0xd2, 0x26,
        0x91, 0x95, 0x7d, 0x6a, 0xf5, 0xc1, 0x1b, 0x64, 0x21, 0xe0, 0xea,
        0x01, 0xd4, 0x2c, 0xa4, 0x16, 0x9e, 0x79, 0x18, 0xba, 0x0d};
    static const uint8_t u_in[32] = {
        0xe5, 0x21, 0x0f, 0x12, 0x78, 0x68, 0x11, 0xd3, 0xf4, 0xb7, 0x95,
        0x9d, 0x05, 0x38, 0xae, 0x2c, 0x31, 0xdb, 0xe7, 0x10, 0x6f, 0xc0,
        0x3c, 0x3e, 0xfc, 0x4c, 0xd5, 0x49, 0xc7, 0x15, 0xa4, 0x93};
    static const uint8_t expected[32] = {
        0x95, 0xcb, 0xde, 0x94, 0x76, 0xe8, 0x90, 0x7d, 0x7a, 0xad, 0xe4,
        0x5c, 0xb4, 0xb8, 0x73, 0xf8, 0x8b, 0x59, 0x5a, 0x68, 0x79, 0x9f,
        0xa1, 0x52, 0xe6, 0xf8, 0xf7, 0x64, 0x7a, 0xac, 0x79, 0x57};

    uint8_t result[32];
    FIO_ASSERT(fio_x25519_shared_secret(result, scalar, u_in) == 0,
               "X25519 RFC 7748 §5.2 vector 2 failed");
    FIO_ASSERT(!FIO_MEMCMP(result, expected, 32),
               "X25519 RFC 7748 §5.2 vector 2 mismatch");
  }

  /* RFC 7748 §6.1 DH exchange test vector. */
  {
    static const uint8_t alice_priv[32] = {
        0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d, 0x3c, 0x16, 0xc1,
        0x72, 0x51, 0xb2, 0x66, 0x45, 0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0,
        0x99, 0x2a, 0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a};
    static const uint8_t alice_pub[32] = {
        0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54, 0x74, 0x8b, 0x7d,
        0xdc, 0xb4, 0x3e, 0xf7, 0x5a, 0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38,
        0x1a, 0xf4, 0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a};
    static const uint8_t bob_priv[32] = {
        0x5d, 0xab, 0x08, 0x7e, 0x62, 0x4a, 0x8a, 0x4b, 0x79, 0xe1, 0x7f,
        0x8b, 0x83, 0x80, 0x0e, 0xe6, 0x6f, 0x3b, 0xb1, 0x29, 0x26, 0x18,
        0xb6, 0xfd, 0x1c, 0x2f, 0x8b, 0x27, 0xff, 0x88, 0xe0, 0xeb};
    static const uint8_t bob_pub[32] = {
        0xde, 0x9e, 0xdb, 0x7d, 0x7b, 0x7d, 0xc1, 0xb4, 0xd3, 0x5b, 0x61,
        0xc2, 0xec, 0xe4, 0x35, 0x37, 0x3f, 0x83, 0x43, 0xc8, 0x5b, 0x78,
        0x67, 0x4d, 0xad, 0xfc, 0x7e, 0x14, 0x6f, 0x88, 0x2b, 0x4f};
    static const uint8_t expected_shared[32] = {
        0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1, 0x72, 0x8e, 0x3b,
        0xf4, 0x80, 0x35, 0x0f, 0x25, 0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1,
        0x9e, 0x33, 0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42};

    uint8_t derived_alice_pub[32];
    uint8_t derived_bob_pub[32];
    uint8_t alice_shared[32];
    uint8_t bob_shared[32];

    fio_x25519_public_key(derived_alice_pub, alice_priv);
    FIO_ASSERT(!FIO_MEMCMP(derived_alice_pub, alice_pub, 32),
               "X25519 public key derivation (Alice) mismatch");

    fio_x25519_public_key(derived_bob_pub, bob_priv);
    FIO_ASSERT(!FIO_MEMCMP(derived_bob_pub, bob_pub, 32),
               "X25519 public key derivation (Bob) mismatch");

    FIO_ASSERT(fio_x25519_shared_secret(alice_shared, alice_priv, bob_pub) == 0,
               "X25519 RFC 7748 §6.1 Alice side failed");
    FIO_ASSERT(fio_x25519_shared_secret(bob_shared, bob_priv, alice_pub) == 0,
               "X25519 RFC 7748 §6.1 Bob side failed");
    FIO_ASSERT(!FIO_MEMCMP(alice_shared, expected_shared, 32),
               "X25519 RFC 7748 §6.1 Alice shared secret mismatch");
    FIO_ASSERT(!FIO_MEMCMP(bob_shared, expected_shared, 32),
               "X25519 RFC 7748 §6.1 Bob shared secret mismatch");
    FIO_ASSERT(!FIO_MEMCMP(alice_shared, bob_shared, 32),
               "X25519 Alice and Bob shared secrets differ");
  }
}

/* *****************************************************************************
X25519 ECDH Roundtrip
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x25519_ecdh_roundtrip)(void) {
  uint8_t alice_sk[32], alice_pk[32];
  uint8_t bob_sk[32], bob_pk[32];
  uint8_t alice_shared[32], bob_shared[32];

  fio_x25519_keypair(alice_sk, alice_pk);
  fio_x25519_keypair(bob_sk, bob_pk);

  FIO_ASSERT(FIO_MEMCMP(alice_sk, bob_sk, 32) != 0,
             "X25519 keypairs should be different");

  FIO_ASSERT(fio_x25519_shared_secret(alice_shared, alice_sk, bob_pk) == 0,
             "X25519 Alice shared secret computation failed");
  FIO_ASSERT(fio_x25519_shared_secret(bob_shared, bob_sk, alice_pk) == 0,
             "X25519 Bob shared secret computation failed");
  FIO_ASSERT(!FIO_MEMCMP(alice_shared, bob_shared, 32),
             "X25519 ECDH shared secrets don't match");

  uint8_t non_zero = 0;
  for (int i = 0; i < 32; ++i)
    non_zero |= alice_shared[i];
  FIO_ASSERT(non_zero != 0, "X25519 shared secret should not be all zeros");
}

/* *****************************************************************************
X25519 ECIES Roundtrips
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x25519_ecies_chacha)(void) {
  uint8_t sk[32], pk[32];
  fio_x25519_keypair(sk, pk);

  const char *message = "Hello, this is a secret message!";
  size_t message_len = FIO_STRLEN(message);
  size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);

  uint8_t *ciphertext =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, ciphertext_len, ciphertext_len);
  uint8_t *plaintext =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, message_len + 1, message_len + 1);
  FIO_ASSERT(ciphertext && plaintext, "ECIES ChaCha allocation failed");

  FIO_ASSERT(fio_x25519_encrypt(ciphertext,
                                message,
                                message_len,
                                fio_chacha20_poly1305_enc,
                                pk) == 0,
             "ECIES ChaCha20-Poly1305 encryption failed");
  FIO_ASSERT(fio_x25519_decrypt(plaintext,
                                ciphertext,
                                ciphertext_len,
                                fio_chacha20_poly1305_dec,
                                sk) == 0,
             "ECIES ChaCha20-Poly1305 decryption failed");
  plaintext[message_len] = '\0';
  FIO_ASSERT(!FIO_MEMCMP(plaintext, message, message_len),
             "ECIES ChaCha20-Poly1305 decrypted message mismatch");

  /* Tamper with encrypted data and confirm detection. */
  ciphertext[40] ^= 0x01;
  FIO_ASSERT(fio_x25519_decrypt(plaintext,
                                ciphertext,
                                ciphertext_len,
                                fio_chacha20_poly1305_dec,
                                sk) != 0,
             "ECIES ChaCha20-Poly1305 should detect tampered ciphertext");
  ciphertext[40] ^= 0x01;

  /* Tamper with MAC and confirm detection. */
  ciphertext[35] ^= 0x01;
  FIO_ASSERT(fio_x25519_decrypt(plaintext,
                                ciphertext,
                                ciphertext_len,
                                fio_chacha20_poly1305_dec,
                                sk) != 0,
             "ECIES ChaCha20-Poly1305 should detect tampered MAC");

  FIO_MEM_FREE(ciphertext, ciphertext_len);
  FIO_MEM_FREE(plaintext, message_len + 1);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x25519_ecies_aes)(void) {
  uint8_t sk[32], pk[32];
  fio_x25519_keypair(sk, pk);

  const char *message = "AES-GCM encrypted secret message";
  size_t message_len = FIO_STRLEN(message);
  size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);

  uint8_t *ciphertext =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, ciphertext_len, ciphertext_len);
  uint8_t *plaintext =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, message_len, message_len);
  FIO_ASSERT(ciphertext && plaintext, "ECIES AES allocation failed");

  FIO_ASSERT(fio_x25519_encrypt(ciphertext,
                                message,
                                message_len,
                                fio_aes256_gcm_enc,
                                pk) == 0,
             "ECIES AES256-GCM encryption failed");
  FIO_ASSERT(fio_x25519_decrypt(plaintext,
                                ciphertext,
                                ciphertext_len,
                                fio_aes256_gcm_dec,
                                sk) == 0,
             "ECIES AES256-GCM decryption failed");
  FIO_ASSERT(!FIO_MEMCMP(plaintext, message, message_len),
             "ECIES AES256-GCM decrypted message mismatch");

  ciphertext[35] ^= 0x01;
  FIO_ASSERT(fio_x25519_decrypt(plaintext,
                                ciphertext,
                                ciphertext_len,
                                fio_aes256_gcm_dec,
                                sk) != 0,
             "ECIES AES256-GCM should detect tampered ciphertext");

  FIO_MEM_FREE(ciphertext, ciphertext_len);
  FIO_MEM_FREE(plaintext, message_len);
}

FIO_SFUNC void FIO_NAME_TEST(stl, x25519_ecies_edges)(void) {
  uint8_t sk[32], pk[32];
  fio_x25519_keypair(sk, pk);

  /* Empty message roundtrip. */
  {
    size_t ct_len = FIO_X25519_CIPHERTEXT_LEN(0);
    uint8_t ciphertext[48];
    FIO_ASSERT(fio_x25519_encrypt(ciphertext,
                                  NULL,
                                  0,
                                  fio_chacha20_poly1305_enc,
                                  pk) == 0,
               "ECIES encryption of empty message failed");
    FIO_ASSERT(fio_x25519_decrypt(NULL,
                                  ciphertext,
                                  ct_len,
                                  fio_chacha20_poly1305_dec,
                                  sk) == 0,
               "ECIES decryption of empty message failed");
  }

  /* Large message roundtrip. */
  {
    size_t message_len = 10000;
    uint8_t *message =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, message_len, message_len);
    for (size_t i = 0; i < message_len; ++i)
      message[i] = (uint8_t)(i & 0xFF);

    size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);
    uint8_t *ciphertext =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, ciphertext_len, ciphertext_len);
    uint8_t *plaintext =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, message_len, message_len);
    FIO_ASSERT(message && ciphertext && plaintext,
               "ECIES large message allocation failed");

    FIO_ASSERT(fio_x25519_encrypt(ciphertext,
                                  message,
                                  message_len,
                                  fio_chacha20_poly1305_enc,
                                  pk) == 0,
               "ECIES large message encryption failed");
    FIO_ASSERT(fio_x25519_decrypt(plaintext,
                                  ciphertext,
                                  ciphertext_len,
                                  fio_chacha20_poly1305_dec,
                                  sk) == 0,
               "ECIES large message decryption failed");
    FIO_ASSERT(!FIO_MEMCMP(plaintext, message, message_len),
               "ECIES large message decryption mismatch");

    FIO_MEM_FREE(message, message_len);
    FIO_MEM_FREE(ciphertext, ciphertext_len);
    FIO_MEM_FREE(plaintext, message_len);
  }

  /* Wrong key decryption fails. */
  {
    uint8_t sk2[32], pk2[32];
    fio_x25519_keypair(sk2, pk2);

    const char *message = "Wrong key test";
    size_t message_len = FIO_STRLEN(message);
    size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);
    uint8_t *ciphertext =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, ciphertext_len, ciphertext_len);
    uint8_t *plaintext =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, message_len, message_len);
    FIO_ASSERT(ciphertext && plaintext, "ECIES wrong-key allocation failed");

    FIO_ASSERT(fio_x25519_encrypt(ciphertext,
                                  message,
                                  message_len,
                                  fio_chacha20_poly1305_enc,
                                  pk) == 0,
               "ECIES wrong-key encryption failed");
    FIO_ASSERT(fio_x25519_decrypt(plaintext,
                                  ciphertext,
                                  ciphertext_len,
                                  fio_chacha20_poly1305_dec,
                                  sk2) != 0,
               "ECIES should fail with wrong secret key");

    FIO_MEM_FREE(ciphertext, ciphertext_len);
    FIO_MEM_FREE(plaintext, message_len);
  }

  /* Ciphertext length macros. */
  FIO_ASSERT(FIO_X25519_CIPHERTEXT_LEN(0) == 48,
             "FIO_X25519_CIPHERTEXT_LEN(0) should be 48");
  FIO_ASSERT(FIO_X25519_CIPHERTEXT_LEN(100) == 148,
             "FIO_X25519_CIPHERTEXT_LEN(100) should be 148");
  FIO_ASSERT(FIO_X25519_PLAINTEXT_LEN(48) == 0,
             "FIO_X25519_PLAINTEXT_LEN(48) should be 0");
  FIO_ASSERT(FIO_X25519_PLAINTEXT_LEN(148) == 100,
             "FIO_X25519_PLAINTEXT_LEN(148) should be 100");
  FIO_ASSERT(FIO_X25519_PLAINTEXT_LEN(47) == 0,
             "FIO_X25519_PLAINTEXT_LEN(47) should be 0");
}

/* *****************************************************************************
Ed25519 to X25519 Conversion (X25519 ownership)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x25519_ed25519_conversion)(void) {
  uint8_t ed_sk[32], ed_pk[32];
  uint8_t x_sk[32], x_pk[32];
  uint8_t other_sk[32], other_pk[32];
  uint8_t shared1[32], shared2[32];

  fio_ed25519_keypair(ed_sk, ed_pk);
  fio_ed25519_sk_to_x25519(x_sk, ed_sk);
  fio_ed25519_pk_to_x25519(x_pk, ed_pk);

  fio_x25519_keypair(other_sk, other_pk);

  FIO_ASSERT(fio_x25519_shared_secret(shared1, x_sk, other_pk) == 0,
             "X25519 ECDH with converted Ed25519 secret key failed");
  FIO_ASSERT(fio_x25519_shared_secret(shared2, other_sk, x_pk) == 0,
             "X25519 ECDH with converted Ed25519 public key failed");
  FIO_ASSERT(!FIO_MEMCMP(shared1, shared2, 32),
             "X25519 ECDH with converted Ed25519 keys mismatch");

  /* ECIES to converted Ed25519 key owner. */
  const char *message = "Encrypt to Ed25519 key owner";
  size_t message_len = FIO_STRLEN(message);
  size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);
  uint8_t *ciphertext =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, ciphertext_len, ciphertext_len);
  uint8_t *plaintext =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, message_len + 1, message_len + 1);
  FIO_ASSERT(ciphertext && plaintext, "ECIES converted-key allocation failed");

  FIO_ASSERT(fio_x25519_encrypt(ciphertext,
                                message,
                                message_len,
                                fio_chacha20_poly1305_enc,
                                x_pk) == 0,
             "ECIES encryption with converted Ed25519 key failed");
  FIO_ASSERT(fio_x25519_decrypt(plaintext,
                                ciphertext,
                                ciphertext_len,
                                fio_chacha20_poly1305_dec,
                                x_sk) == 0,
             "ECIES decryption with converted Ed25519 key failed");
  plaintext[message_len] = '\0';
  FIO_ASSERT(!FIO_MEMCMP(plaintext, message, message_len),
             "ECIES with converted Ed25519 keys message mismatch");

  FIO_MEM_FREE(ciphertext, ciphertext_len);
  FIO_MEM_FREE(plaintext, message_len + 1);
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  FIO_NAME_TEST(stl, x25519_rfc7748)();
  FIO_NAME_TEST(stl, x25519_ecdh_roundtrip)();
  FIO_NAME_TEST(stl, x25519_ecies_chacha)();
  FIO_NAME_TEST(stl, x25519_ecies_aes)();
  FIO_NAME_TEST(stl, x25519_ecies_edges)();
  FIO_NAME_TEST(stl, x25519_ed25519_conversion)();
  return 0;
}
