/* *****************************************************************************
Ed25519 / X25519 Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPTO
#include FIO_INCLUDE_FILE

/* *****************************************************************************
RFC 8032 Test Vectors for Ed25519
https://www.rfc-editor.org/rfc/rfc8032#section-7.1
***************************************************************************** */

/* Test field arithmetic */
FIO_SFUNC void FIO_NAME_TEST(stl, fe25519_basic)(void) {
  fprintf(stderr, "\t* Testing field arithmetic basics\n");

  /* Test: load and store identity */
  // {
  //   uint8_t one[32] = {1};
  //   uint8_t out[32];
  //   /* This would require exposing internal functions, skip for now */
  // }

  fprintf(stderr, "\t  Field arithmetic tests skipped (internal functions)\n");
}

FIO_SFUNC void FIO_NAME_TEST(stl, ed25519_rfc8032)(void) {
  fprintf(stderr, "\t* Testing Ed25519 (RFC 8032 test vectors)\n");

  /* Test Vector 1: Empty message */
  {
    /* clang-format off */
    static const uint8_t sk[32] = {
        0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
        0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
        0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
        0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60
    };
    static const uint8_t expected_pk[32] = {
        0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
        0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
        0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
        0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a
    };
    static const uint8_t expected_sig[64] = {
        0xe5, 0x56, 0x43, 0x00, 0xc3, 0x60, 0xac, 0x72,
        0x90, 0x86, 0xe2, 0xcc, 0x80, 0x6e, 0x82, 0x8a,
        0x84, 0x87, 0x7f, 0x1e, 0xb8, 0xe5, 0xd9, 0x74,
        0xd8, 0x73, 0xe0, 0x65, 0x22, 0x49, 0x01, 0x55,
        0x5f, 0xb8, 0x82, 0x15, 0x90, 0xa3, 0x3b, 0xac,
        0xc6, 0x1e, 0x39, 0x70, 0x1c, 0xf9, 0xb4, 0x6b,
        0xd2, 0x5b, 0xf5, 0xf0, 0x59, 0x5b, 0xbe, 0x24,
        0x65, 0x51, 0x41, 0x43, 0x8e, 0x7a, 0x10, 0x0b
    };
    /* clang-format on */

    uint8_t pk[32], sig[64];

#if (DEBUG - 1 + 1)
    /* Debug: print the clamped scalar */
    fio_u512 h = fio_sha512(sk, 32);
    h.u8[0] &= 248;
    h.u8[31] &= 127;
    h.u8[31] |= 64;
    fprintf(stderr, "Clamped scalar: ");
    for (int i = 0; i < 32; i++)
      fprintf(stderr, "%02x", h.u8[i]);
    fprintf(stderr, "\n");
#endif

    /* Debug: test encoding of 1*B (should give base point) */
    // {
    //   uint8_t one[32] = {1}; /* scalar = 1 */
    //   uint8_t bp[32];
    //   /* We need to expose the internal function for testing */
    //   /* For now, just test the public key derivation */
    // }

    fio_ed25519_public_key(pk, sk);

#if (DEBUG - 1 + 1)
    fprintf(stderr, "Expected PK: ");
    for (int i = 0; i < 32; i++)
      fprintf(stderr, "%02x", expected_pk[i]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Got PK:      ");
    for (int i = 0; i < 32; i++)
      fprintf(stderr, "%02x", pk[i]);
    fprintf(stderr, "\n");
#endif

    FIO_ASSERT(!memcmp(pk, expected_pk, 32),
               "Ed25519 public key derivation failed (test vector 1)");

    fio_ed25519_sign(sig, "", 0, sk, pk);

    FIO_ASSERT(!memcmp(sig, expected_sig, 64),
               "Ed25519 signature failed (test vector 1)");

    int verify_result = fio_ed25519_verify(sig, "", 0, pk);
    FIO_ASSERT(verify_result == 0,
               "Ed25519 verification failed (test vector 1)");
  }

  /* Test Vector 2: Single byte message (0x72) */
  {
    /* clang-format off */
    static const uint8_t sk[32] = {
        0x4c, 0xcd, 0x08, 0x9b, 0x28, 0xff, 0x96, 0xda,
        0x9d, 0xb6, 0xc3, 0x46, 0xec, 0x11, 0x4e, 0x0f,
        0x5b, 0x8a, 0x31, 0x9f, 0x35, 0xab, 0xa6, 0x24,
        0xda, 0x8c, 0xf6, 0xed, 0x4f, 0xb8, 0xa6, 0xfb
    };
    static const uint8_t expected_pk[32] = {
        0x3d, 0x40, 0x17, 0xc3, 0xe8, 0x43, 0x89, 0x5a,
        0x92, 0xb7, 0x0a, 0xa7, 0x4d, 0x1b, 0x7e, 0xbc,
        0x9c, 0x98, 0x2c, 0xcf, 0x2e, 0xc4, 0x96, 0x8c,
        0xc0, 0xcd, 0x55, 0xf1, 0x2a, 0xf4, 0x66, 0x0c
    };
    static const uint8_t message[1] = {0x72};
    static const uint8_t expected_sig[64] = {
        0x92, 0xa0, 0x09, 0xa9, 0xf0, 0xd4, 0xca, 0xb8,
        0x72, 0x0e, 0x82, 0x0b, 0x5f, 0x64, 0x25, 0x40,
        0xa2, 0xb2, 0x7b, 0x54, 0x16, 0x50, 0x3f, 0x8f,
        0xb3, 0x76, 0x22, 0x23, 0xeb, 0xdb, 0x69, 0xda,
        0x08, 0x5a, 0xc1, 0xe4, 0x3e, 0x15, 0x99, 0x6e,
        0x45, 0x8f, 0x36, 0x13, 0xd0, 0xf1, 0x1d, 0x8c,
        0x38, 0x7b, 0x2e, 0xae, 0xb4, 0x30, 0x2a, 0xee,
        0xb0, 0x0d, 0x29, 0x16, 0x12, 0xbb, 0x0c, 0x00
    };
    /* clang-format on */

    uint8_t pk[32], sig[64];
    fio_ed25519_public_key(pk, sk);

    FIO_ASSERT(!memcmp(pk, expected_pk, 32),
               "Ed25519 public key derivation failed (test vector 2)");

    fio_ed25519_sign(sig, message, 1, sk, pk);

    FIO_ASSERT(!memcmp(sig, expected_sig, 64),
               "Ed25519 signature failed (test vector 2)");

    int verify_result = fio_ed25519_verify(sig, message, 1, pk);
    FIO_ASSERT(verify_result == 0,
               "Ed25519 verification failed (test vector 2)");
  }

  /* Test Vector 3: Two byte message */
  {
    /* clang-format off */
    static const uint8_t sk[32] = {
        0xc5, 0xaa, 0x8d, 0xf4, 0x3f, 0x9f, 0x83, 0x7b,
        0xed, 0xb7, 0x44, 0x2f, 0x31, 0xdc, 0xb7, 0xb1,
        0x66, 0xd3, 0x85, 0x35, 0x07, 0x6f, 0x09, 0x4b,
        0x85, 0xce, 0x3a, 0x2e, 0x0b, 0x44, 0x58, 0xf7
    };
    static const uint8_t expected_pk[32] = {
        0xfc, 0x51, 0xcd, 0x8e, 0x62, 0x18, 0xa1, 0xa3,
        0x8d, 0xa4, 0x7e, 0xd0, 0x02, 0x30, 0xf0, 0x58,
        0x08, 0x16, 0xed, 0x13, 0xba, 0x33, 0x03, 0xac,
        0x5d, 0xeb, 0x91, 0x15, 0x48, 0x90, 0x80, 0x25
    };
    static const uint8_t message[2] = {0xaf, 0x82};
    static const uint8_t expected_sig[64] = {
        0x62, 0x91, 0xd6, 0x57, 0xde, 0xec, 0x24, 0x02,
        0x48, 0x27, 0xe6, 0x9c, 0x3a, 0xbe, 0x01, 0xa3,
        0x0c, 0xe5, 0x48, 0xa2, 0x84, 0x74, 0x3a, 0x44,
        0x5e, 0x36, 0x80, 0xd7, 0xdb, 0x5a, 0xc3, 0xac,
        0x18, 0xff, 0x9b, 0x53, 0x8d, 0x16, 0xf2, 0x90,
        0xae, 0x67, 0xf7, 0x60, 0x98, 0x4d, 0xc6, 0x59,
        0x4a, 0x7c, 0x15, 0xe9, 0x71, 0x6e, 0xd2, 0x8d,
        0xc0, 0x27, 0xbe, 0xce, 0xea, 0x1e, 0xc4, 0x0a
    };
    /* clang-format on */

    uint8_t pk[32], sig[64];
    fio_ed25519_public_key(pk, sk);

    FIO_ASSERT(!memcmp(pk, expected_pk, 32),
               "Ed25519 public key derivation failed (test vector 3)");

    fio_ed25519_sign(sig, message, 2, sk, pk);

    FIO_ASSERT(!memcmp(sig, expected_sig, 64),
               "Ed25519 signature failed (test vector 3)");

    int verify_result = fio_ed25519_verify(sig, message, 2, pk);
    FIO_ASSERT(verify_result == 0,
               "Ed25519 verification failed (test vector 3)");
  }

  /* Test invalid signature detection */
  {
    uint8_t sk[32], pk[32], sig[64];
    fio_ed25519_keypair(sk, pk);

    const char *message = "Test message for signature verification";
    fio_ed25519_sign(sig, message, strlen(message), sk, pk);

    /* Verify valid signature */
    FIO_ASSERT(fio_ed25519_verify(sig, message, strlen(message), pk) == 0,
               "Ed25519 verification failed for valid signature");

    /* Modify signature and verify it fails */
    sig[0] ^= 0x01;
    FIO_ASSERT(fio_ed25519_verify(sig, message, strlen(message), pk) != 0,
               "Ed25519 verification should fail for modified signature");

    /* Restore signature, modify message */
    sig[0] ^= 0x01;
    const char *wrong_message = "Wrong message for signature verification";
    FIO_ASSERT(
        fio_ed25519_verify(sig, wrong_message, strlen(wrong_message), pk) != 0,
        "Ed25519 verification should fail for wrong message");
  }

  fprintf(stderr, "\t  Ed25519 tests passed.\n");
}

/* *****************************************************************************
RFC 7748 Test Vectors for X25519
https://www.rfc-editor.org/rfc/rfc7748#section-5.2
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x25519_rfc7748)(void) {
  fprintf(stderr, "\t* Testing X25519 (RFC 7748 test vectors)\n");

  /* Test Vector 1 */
  {
    /* clang-format off */
    static const uint8_t scalar[32] = {
        0xa5, 0x46, 0xe3, 0x6b, 0xf0, 0x52, 0x7c, 0x9d,
        0x3b, 0x16, 0x15, 0x4b, 0x82, 0x46, 0x5e, 0xdd,
        0x62, 0x14, 0x4c, 0x0a, 0xc1, 0xfc, 0x5a, 0x18,
        0x50, 0x6a, 0x22, 0x44, 0xba, 0x44, 0x9a, 0xc4
    };
    static const uint8_t u_in[32] = {
        0xe6, 0xdb, 0x68, 0x67, 0x58, 0x30, 0x30, 0xdb,
        0x35, 0x94, 0xc1, 0xa4, 0x24, 0xb1, 0x5f, 0x7c,
        0x72, 0x66, 0x24, 0xec, 0x26, 0xb3, 0x35, 0x3b,
        0x10, 0xa9, 0x03, 0xa6, 0xd0, 0xab, 0x1c, 0x4c
    };
    static const uint8_t expected[32] = {
        0xc3, 0xda, 0x55, 0x37, 0x9d, 0xe9, 0xc6, 0x90,
        0x8e, 0x94, 0xea, 0x4d, 0xf2, 0x8d, 0x08, 0x4f,
        0x32, 0xec, 0xcf, 0x03, 0x49, 0x1c, 0x71, 0xf7,
        0x54, 0xb4, 0x07, 0x55, 0x77, 0xa2, 0x85, 0x52
    };
    /* clang-format on */

    uint8_t result[32];
    fio_x25519_shared_secret(result, scalar, u_in);

    FIO_ASSERT(!memcmp(result, expected, 32),
               "X25519 scalar multiplication failed (test vector 1)");
  }

  /* Test Vector 2 */
  {
    /* clang-format off */
    static const uint8_t scalar[32] = {
        0x4b, 0x66, 0xe9, 0xd4, 0xd1, 0xb4, 0x67, 0x3c,
        0x5a, 0xd2, 0x26, 0x91, 0x95, 0x7d, 0x6a, 0xf5,
        0xc1, 0x1b, 0x64, 0x21, 0xe0, 0xea, 0x01, 0xd4,
        0x2c, 0xa4, 0x16, 0x9e, 0x79, 0x18, 0xba, 0x0d
    };
    static const uint8_t u_in[32] = {
        0xe5, 0x21, 0x0f, 0x12, 0x78, 0x68, 0x11, 0xd3,
        0xf4, 0xb7, 0x95, 0x9d, 0x05, 0x38, 0xae, 0x2c,
        0x31, 0xdb, 0xe7, 0x10, 0x6f, 0xc0, 0x3c, 0x3e,
        0xfc, 0x4c, 0xd5, 0x49, 0xc7, 0x15, 0xa4, 0x93
    };
    static const uint8_t expected[32] = {
        0x95, 0xcb, 0xde, 0x94, 0x76, 0xe8, 0x90, 0x7d,
        0x7a, 0xad, 0xe4, 0x5c, 0xb4, 0xb8, 0x73, 0xf8,
        0x8b, 0x59, 0x5a, 0x68, 0x79, 0x9f, 0xa1, 0x52,
        0xe6, 0xf8, 0xf7, 0x64, 0x7a, 0xac, 0x79, 0x57
    };
    /* clang-format on */

    uint8_t result[32];
    fio_x25519_shared_secret(result, scalar, u_in);

    FIO_ASSERT(!memcmp(result, expected, 32),
               "X25519 scalar multiplication failed (test vector 2)");
  }

  /* Test base point multiplication (public key derivation) */
  {
    /* clang-format off */
    static const uint8_t sk[32] = {
        0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d,
        0x3c, 0x16, 0xc1, 0x72, 0x51, 0xb2, 0x66, 0x45,
        0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0, 0x99, 0x2a,
        0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a
    };
    static const uint8_t expected_pk[32] = {
        0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54,
        0x74, 0x8b, 0x7d, 0xdc, 0xb4, 0x3e, 0xf7, 0x5a,
        0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38, 0x1a, 0xf4,
        0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a
    };
    /* clang-format on */

    uint8_t pk[32];
    fio_x25519_public_key(pk, sk);

    FIO_ASSERT(!memcmp(pk, expected_pk, 32),
               "X25519 public key derivation failed");
  }

  /* Test ECDH key exchange */
  {
    uint8_t alice_sk[32], alice_pk[32];
    uint8_t bob_sk[32], bob_pk[32];
    uint8_t alice_shared[32], bob_shared[32];

    fio_x25519_keypair(alice_sk, alice_pk);
    fio_x25519_keypair(bob_sk, bob_pk);

    int alice_result = fio_x25519_shared_secret(alice_shared, alice_sk, bob_pk);
    int bob_result = fio_x25519_shared_secret(bob_shared, bob_sk, alice_pk);

    FIO_ASSERT(alice_result == 0 && bob_result == 0,
               "X25519 shared secret computation failed");

    FIO_ASSERT(!memcmp(alice_shared, bob_shared, 32),
               "X25519 ECDH key exchange failed - shared secrets don't match");
  }

  fprintf(stderr, "\t  X25519 tests passed.\n");
}

/* *****************************************************************************
Key Conversion Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, ed25519_x25519_conversion)(void) {
  fprintf(stderr, "\t* Testing Ed25519 <-> X25519 key conversion\n");

  /* Test that converted keys work for ECDH */
  {
    uint8_t ed_sk[32], ed_pk[32];
    uint8_t x_sk[32], x_pk[32];

    /* Generate Ed25519 keypair */
    fio_ed25519_keypair(ed_sk, ed_pk);

    /* Convert to X25519 */
    fio_ed25519_sk_to_x25519(x_sk, ed_sk);
    fio_ed25519_pk_to_x25519(x_pk, ed_pk);

    /* Generate another X25519 keypair for testing */
    uint8_t other_sk[32], other_pk[32];
    fio_x25519_keypair(other_sk, other_pk);

    /* Compute shared secrets */
    uint8_t shared1[32], shared2[32];
    int r1 = fio_x25519_shared_secret(shared1, x_sk, other_pk);
    int r2 = fio_x25519_shared_secret(shared2, other_sk, x_pk);

    FIO_ASSERT(r1 == 0 && r2 == 0,
               "X25519 shared secret with converted keys failed");

    FIO_ASSERT(
        !memcmp(shared1, shared2, 32),
        "ECDH with converted Ed25519 keys failed - shared secrets don't match");
  }

  fprintf(stderr, "\t  Key conversion tests passed.\n");
}

/* *****************************************************************************
ECIES Public Key Encryption Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, x25519_ecies)(void) {
  fprintf(stderr, "\t* Testing X25519 ECIES (public key encryption)\n");

  /* Test basic encryption/decryption */
  {
    uint8_t sk[32], pk[32];
    fio_x25519_keypair(sk, pk);

    const char *message = "Hello, this is a secret message!";
    size_t message_len = strlen(message);
    size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);

    uint8_t *ciphertext = malloc(ciphertext_len);
    uint8_t *plaintext = malloc(message_len + 1);

    FIO_ASSERT(ciphertext && plaintext, "Memory allocation failed");

    /* Encrypt */
    int enc_result = fio_x25519_encrypt(ciphertext,
                                        message,
                                        message_len,
                                        fio_chacha20_poly1305_enc,
                                        pk);
    FIO_ASSERT(enc_result == 0, "ECIES encryption failed");

    /* Decrypt */
    int dec_result = fio_x25519_decrypt(plaintext,
                                        ciphertext,
                                        ciphertext_len,
                                        fio_chacha20_poly1305_dec,
                                        sk);
    FIO_ASSERT(dec_result == 0, "ECIES decryption failed");

    plaintext[message_len] = '\0';
    FIO_ASSERT(!memcmp(plaintext, message, message_len),
               "ECIES decrypted message doesn't match original");

    free(ciphertext);
    free(plaintext);
  }

  /* Test empty message */
  {
    uint8_t sk[32], pk[32];
    fio_x25519_keypair(sk, pk);

    size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(0);
    uint8_t ciphertext[48]; /* 32 + 16 + 0 */

    int enc_result =
        fio_x25519_encrypt(ciphertext, NULL, 0, fio_chacha20_poly1305_enc, pk);
    FIO_ASSERT(enc_result == 0, "ECIES encryption of empty message failed");

    int dec_result = fio_x25519_decrypt(NULL,
                                        ciphertext,
                                        ciphertext_len,
                                        fio_chacha20_poly1305_dec,
                                        sk);
    FIO_ASSERT(dec_result == 0, "ECIES decryption of empty message failed");
  }

  /* Test tamper detection - modified ciphertext */
  {
    uint8_t sk[32], pk[32];
    fio_x25519_keypair(sk, pk);

    const char *message = "Tamper test message";
    size_t message_len = strlen(message);
    size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);

    uint8_t *ciphertext = malloc(ciphertext_len);
    uint8_t *plaintext = malloc(message_len);

    /* Encrypt */
    fio_x25519_encrypt(ciphertext,
                       message,
                       message_len,
                       fio_chacha20_poly1305_enc,
                       pk);

    /* Tamper with encrypted data */
    ciphertext[50] ^= 0x01;

    /* Decryption should fail */
    int dec_result = fio_x25519_decrypt(plaintext,
                                        ciphertext,
                                        ciphertext_len,
                                        fio_chacha20_poly1305_dec,
                                        sk);
    FIO_ASSERT(dec_result != 0, "ECIES should detect tampered ciphertext");

    free(ciphertext);
    free(plaintext);
  }

  /* Test tamper detection - modified MAC */
  {
    uint8_t sk[32], pk[32];
    fio_x25519_keypair(sk, pk);

    const char *message = "MAC tamper test";
    size_t message_len = strlen(message);
    size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);

    uint8_t *ciphertext = malloc(ciphertext_len);
    uint8_t *plaintext = malloc(message_len);

    /* Encrypt */
    fio_x25519_encrypt(ciphertext,
                       message,
                       message_len,
                       fio_chacha20_poly1305_enc,
                       pk);

    /* Tamper with MAC (bytes 32-47) */
    ciphertext[35] ^= 0x01;

    /* Decryption should fail */
    int dec_result = fio_x25519_decrypt(plaintext,
                                        ciphertext,
                                        ciphertext_len,
                                        fio_chacha20_poly1305_dec,
                                        sk);
    FIO_ASSERT(dec_result != 0, "ECIES should detect tampered MAC");

    free(ciphertext);
    free(plaintext);
  }

  /* Test wrong key decryption */
  {
    uint8_t sk1[32], pk1[32];
    uint8_t sk2[32], pk2[32];
    fio_x25519_keypair(sk1, pk1);
    fio_x25519_keypair(sk2, pk2);

    const char *message = "Wrong key test";
    size_t message_len = strlen(message);
    size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);

    uint8_t *ciphertext = malloc(ciphertext_len);
    uint8_t *plaintext = malloc(message_len);

    /* Encrypt to pk1 */
    fio_x25519_encrypt(ciphertext,
                       message,
                       message_len,
                       fio_chacha20_poly1305_enc,
                       pk1);

    /* Try to decrypt with sk2 (wrong key) */
    int dec_result = fio_x25519_decrypt(plaintext,
                                        ciphertext,
                                        ciphertext_len,
                                        fio_chacha20_poly1305_dec,
                                        sk2);
    FIO_ASSERT(dec_result != 0, "ECIES should fail with wrong secret key");

    free(ciphertext);
    free(plaintext);
  }

  /* Test large message */
  {
    uint8_t sk[32], pk[32];
    fio_x25519_keypair(sk, pk);

    size_t message_len = 10000;
    uint8_t *message = malloc(message_len);
    for (size_t i = 0; i < message_len; i++)
      message[i] = (uint8_t)(i & 0xFF);

    size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);
    uint8_t *ciphertext = malloc(ciphertext_len);
    uint8_t *plaintext = malloc(message_len);

    /* Encrypt */
    int enc_result = fio_x25519_encrypt(ciphertext,
                                        message,
                                        message_len,
                                        fio_chacha20_poly1305_enc,
                                        pk);
    FIO_ASSERT(enc_result == 0, "ECIES encryption of large message failed");

    /* Decrypt */
    int dec_result = fio_x25519_decrypt(plaintext,
                                        ciphertext,
                                        ciphertext_len,
                                        fio_chacha20_poly1305_dec,
                                        sk);
    FIO_ASSERT(dec_result == 0, "ECIES decryption of large message failed");

    FIO_ASSERT(!memcmp(plaintext, message, message_len),
               "ECIES large message decryption mismatch");

    free(message);
    free(ciphertext);
    free(plaintext);
  }

  /* Test ciphertext length macros */
  {
    FIO_ASSERT(FIO_X25519_CIPHERTEXT_LEN(0) == 48,
               "FIO_X25519_CIPHERTEXT_LEN(0) should be 48");
    FIO_ASSERT(FIO_X25519_CIPHERTEXT_LEN(100) == 148,
               "FIO_X25519_CIPHERTEXT_LEN(100) should be 148");
    FIO_ASSERT(FIO_X25519_PLAINTEXT_LEN(48) == 0,
               "FIO_X25519_PLAINTEXT_LEN(48) should be 0");
    FIO_ASSERT(FIO_X25519_PLAINTEXT_LEN(148) == 100,
               "FIO_X25519_PLAINTEXT_LEN(148) should be 100");
    FIO_ASSERT(FIO_X25519_PLAINTEXT_LEN(47) == 0,
               "FIO_X25519_PLAINTEXT_LEN(47) should be 0 (invalid)");
  }

  /* Test encryption with Ed25519 converted keys */
  {
    uint8_t ed_sk[32], ed_pk[32];
    uint8_t x_sk[32], x_pk[32];

    /* Generate Ed25519 keypair and convert to X25519 */
    fio_ed25519_keypair(ed_sk, ed_pk);
    fio_ed25519_sk_to_x25519(x_sk, ed_sk);
    fio_ed25519_pk_to_x25519(x_pk, ed_pk);

    const char *message = "Encrypt to Ed25519 key owner";
    size_t message_len = strlen(message);
    size_t ciphertext_len = FIO_X25519_CIPHERTEXT_LEN(message_len);

    uint8_t *ciphertext = malloc(ciphertext_len);
    uint8_t *plaintext = malloc(message_len + 1);

    /* Encrypt using converted X25519 public key */
    int enc_result = fio_x25519_encrypt(ciphertext,
                                        message,
                                        message_len,
                                        fio_chacha20_poly1305_enc,
                                        x_pk);
    FIO_ASSERT(enc_result == 0, "ECIES encryption with converted key failed");

    /* Decrypt using converted X25519 secret key */
    int dec_result = fio_x25519_decrypt(plaintext,
                                        ciphertext,
                                        ciphertext_len,
                                        fio_chacha20_poly1305_dec,
                                        x_sk);
    FIO_ASSERT(dec_result == 0, "ECIES decryption with converted key failed");

    plaintext[message_len] = '\0';
    FIO_ASSERT(!memcmp(plaintext, message, message_len),
               "ECIES with converted keys: decrypted message mismatch");

    free(ciphertext);
    free(plaintext);
  }

  fprintf(stderr, "\t  ECIES tests passed.\n");
}

/* *****************************************************************************
Performance Testing
***************************************************************************** */

/* Number of iterations for performance tests */
#ifndef FIO_ED25519_PERF_ITERATIONS
#define FIO_ED25519_PERF_ITERATIONS 1000
#endif

FIO_SFUNC int64_t fio___ed25519_time_us(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return ((int64_t)t.tv_sec * 1000000) + (int64_t)t.tv_nsec / 1000;
}

FIO_SFUNC void FIO_NAME_TEST(stl, ed25519_performance)(void) {
  const size_t iterations = FIO_ED25519_PERF_ITERATIONS;
  int64_t start, end;
  double elapsed_us, ops_per_sec;

  /* Pre-generate test data */
  uint8_t sk[32], pk[32], sig[64];
  uint8_t x_sk[32], x_pk[32], x_other_sk[32], x_other_pk[32];
  const char *message = "Performance test message for Ed25519 signing";
  size_t message_len = strlen(message);

  fio_ed25519_keypair(sk, pk);
  fio_x25519_keypair(x_sk, x_pk);
  fio_x25519_keypair(x_other_sk, x_other_pk);

#ifdef DEBUG
  fprintf(stderr,
          "\n\t* Ed25519/X25519 Performance Tests "
          "(DEBUG mode - results may be slower)\n");
#else
  fprintf(stderr, "\n\t* Ed25519/X25519 Performance Tests\n");
#endif
  fprintf(stderr,
          "\t  Running %zu iterations per operation...\n\n",
          iterations);

  /* --- Ed25519 Key Generation --- */
  {
    uint8_t tmp_sk[32], tmp_pk[32];
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_ed25519_keypair(tmp_sk, tmp_pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "Ed25519 keypair generation",
            ops_per_sec,
            elapsed_us / iterations);
  }

  /* --- Ed25519 Public Key Derivation --- */
  {
    uint8_t tmp_pk[32];
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_ed25519_public_key(tmp_pk, sk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "Ed25519 public key derivation",
            ops_per_sec,
            elapsed_us / iterations);
  }

  /* --- Ed25519 Signing --- */
  {
    uint8_t tmp_sig[64];
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_ed25519_sign(tmp_sig, message, message_len, sk, pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "Ed25519 sign (45 byte message)",
            ops_per_sec,
            elapsed_us / iterations);
  }

  /* Pre-sign for verification tests */
  fio_ed25519_sign(sig, message, message_len, sk, pk);

  /* --- Ed25519 Verification --- */
  {
    volatile int result = 0;
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      result = fio_ed25519_verify(sig, message, message_len, pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    (void)result;
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "Ed25519 verify (45 byte message)",
            ops_per_sec,
            elapsed_us / iterations);
  }

  /* --- Ed25519 Sign + Verify Combined --- */
  {
    uint8_t tmp_sig[64];
    volatile int result = 0;
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_ed25519_sign(tmp_sig, message, message_len, sk, pk);
      result = fio_ed25519_verify(tmp_sig, message, message_len, pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    (void)result;
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "Ed25519 sign+verify roundtrip",
            ops_per_sec,
            elapsed_us / iterations);
  }

  fprintf(stderr, "\n");

  /* --- Ed25519 Signing Speed with Various Message Sizes --- */
  fprintf(stderr, "\t  Ed25519 Signing Speed by Message Size:\n");
  {
    static const size_t msg_sizes[] =
        {0, 32, 64, 128, 256, 512, 1024, 4096, 16384, 65536};
    static const size_t num_sizes = sizeof(msg_sizes) / sizeof(msg_sizes[0]);
    uint8_t tmp_sig[64];
    uint8_t *msg_buf = malloc(65536);

    /* Fill buffer with test data */
    for (size_t i = 0; i < 65536; ++i)
      msg_buf[i] = (uint8_t)(i & 0xFF);

    for (size_t s = 0; s < num_sizes; ++s) {
      size_t msg_size = msg_sizes[s];
      /* Adjust iterations for larger messages */
      size_t iters = iterations;
      if (msg_size > 4096)
        iters = iterations / 4;
      else if (msg_size > 1024)
        iters = iterations / 2;

      start = fio___ed25519_time_us();
      for (size_t i = 0; i < iters; ++i) {
        fio_ed25519_sign(tmp_sig, msg_buf, msg_size, sk, pk);
        FIO_COMPILER_GUARD;
      }
      end = fio___ed25519_time_us();
      elapsed_us = (double)(end - start);
      ops_per_sec = (iters * 1000000.0) / elapsed_us;

      char label[64];
      if (msg_size < 1024)
        snprintf(label, sizeof(label), "    Sign (%zu bytes)", msg_size);
      else
        snprintf(label, sizeof(label), "    Sign (%zuKB)", msg_size / 1024);

      fprintf(stderr,
              "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
              label,
              ops_per_sec,
              elapsed_us / iters);
    }
    free(msg_buf);
  }

  fprintf(stderr, "\n");

  /* --- Ed25519 Verification Speed with Various Message Sizes --- */
  fprintf(stderr, "\t  Ed25519 Verification Speed by Message Size:\n");
  {
    static const size_t msg_sizes[] =
        {0, 32, 64, 128, 256, 512, 1024, 4096, 16384, 65536};
    static const size_t num_sizes = sizeof(msg_sizes) / sizeof(msg_sizes[0]);
    uint8_t tmp_sig[64];
    uint8_t *msg_buf = malloc(65536);
    volatile int result = 0;

    /* Fill buffer with test data */
    for (size_t i = 0; i < 65536; ++i)
      msg_buf[i] = (uint8_t)(i & 0xFF);

    for (size_t s = 0; s < num_sizes; ++s) {
      size_t msg_size = msg_sizes[s];
      /* Pre-sign the message */
      fio_ed25519_sign(tmp_sig, msg_buf, msg_size, sk, pk);

      /* Adjust iterations for larger messages */
      size_t iters = iterations;
      if (msg_size > 4096)
        iters = iterations / 4;
      else if (msg_size > 1024)
        iters = iterations / 2;

      start = fio___ed25519_time_us();
      for (size_t i = 0; i < iters; ++i) {
        result = fio_ed25519_verify(tmp_sig, msg_buf, msg_size, pk);
        FIO_COMPILER_GUARD;
      }
      end = fio___ed25519_time_us();
      (void)result;
      elapsed_us = (double)(end - start);
      ops_per_sec = (iters * 1000000.0) / elapsed_us;

      char label[64];
      if (msg_size < 1024)
        snprintf(label, sizeof(label), "    Verify (%zu bytes)", msg_size);
      else
        snprintf(label, sizeof(label), "    Verify (%zuKB)", msg_size / 1024);

      fprintf(stderr,
              "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
              label,
              ops_per_sec,
              elapsed_us / iters);
    }
    free(msg_buf);
  }

  fprintf(stderr, "\n");

  /* --- X25519 Key Generation --- */
  {
    uint8_t tmp_sk[32], tmp_pk[32];
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_keypair(tmp_sk, tmp_pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "X25519 keypair generation",
            ops_per_sec,
            elapsed_us / iterations);
  }

  /* --- X25519 Public Key Derivation --- */
  {
    uint8_t tmp_pk[32];
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_public_key(tmp_pk, x_sk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "X25519 public key derivation",
            ops_per_sec,
            elapsed_us / iterations);
  }

  /* --- X25519 Shared Secret (ECDH) --- */
  {
    uint8_t tmp_shared[32];
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_shared_secret(tmp_shared, x_sk, x_other_pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "X25519 shared secret (ECDH)",
            ops_per_sec,
            elapsed_us / iterations);
  }

  /* --- X25519 Full Key Exchange --- */
  {
    uint8_t a_sk[32], a_pk[32], b_sk[32], b_pk[32];
    uint8_t a_shared[32], b_shared[32];
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_keypair(a_sk, a_pk);
      fio_x25519_keypair(b_sk, b_pk);
      fio_x25519_shared_secret(a_shared, a_sk, b_pk);
      fio_x25519_shared_secret(b_shared, b_sk, a_pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "X25519 full key exchange (2 parties)",
            ops_per_sec,
            elapsed_us / iterations);
  }

  fprintf(stderr, "\n");

  /* --- Key Conversion: Ed25519 SK to X25519 --- */
  {
    uint8_t tmp_x_sk[32];
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_ed25519_sk_to_x25519(tmp_x_sk, sk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "Ed25519 SK -> X25519 SK conversion",
            ops_per_sec,
            elapsed_us / iterations);
  }

  /* --- Key Conversion: Ed25519 PK to X25519 --- */
  {
    uint8_t tmp_x_pk[32];
    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_ed25519_pk_to_x25519(tmp_x_pk, pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "Ed25519 PK -> X25519 PK conversion",
            ops_per_sec,
            elapsed_us / iterations);
  }

  fprintf(stderr, "\n");

  /* --- ECIES Encryption (small message) --- */
  {
    const char *small_msg = "Small test message";
    size_t small_len = strlen(small_msg);
    size_t ct_len = FIO_X25519_CIPHERTEXT_LEN(small_len);
    uint8_t *ciphertext = malloc(ct_len);

    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_encrypt(ciphertext,
                         small_msg,
                         small_len,
                         fio_chacha20_poly1305_enc,
                         x_pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "ECIES encrypt (18 byte message, using ChaCha20Poly1305)",
            ops_per_sec,
            elapsed_us / iterations);
    free(ciphertext);
  }

  /* --- ECIES Decryption (small message) --- */
  {
    const char *small_msg = "Small test message";
    size_t small_len = strlen(small_msg);
    size_t ct_len = FIO_X25519_CIPHERTEXT_LEN(small_len);
    uint8_t *ciphertext = malloc(ct_len);
    uint8_t *plaintext = malloc(small_len);
    fio_x25519_encrypt(ciphertext,
                       small_msg,
                       small_len,
                       fio_chacha20_poly1305_enc,
                       x_pk);

    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_decrypt(plaintext,
                         ciphertext,
                         ct_len,
                         fio_chacha20_poly1305_dec,
                         x_sk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "ECIES decrypt (18 byte message, using ChaCha20Poly1305)",
            ops_per_sec,
            elapsed_us / iterations);
    free(ciphertext);
    free(plaintext);
  }

  /* --- ECIES Encryption (1KB message) --- */
  {
    size_t large_len = 1024;
    uint8_t *large_msg = malloc(large_len);
    for (size_t i = 0; i < large_len; ++i)
      large_msg[i] = (uint8_t)(i & 0xFF);
    size_t ct_len = FIO_X25519_CIPHERTEXT_LEN(large_len);
    uint8_t *ciphertext = malloc(ct_len);

    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_encrypt(ciphertext,
                         large_msg,
                         large_len,
                         fio_chacha20_poly1305_enc,
                         x_pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "ECIES encrypt (1KB message, using ChaCha20Poly1305)",
            ops_per_sec,
            elapsed_us / iterations);
    free(large_msg);
    free(ciphertext);
  }

  /* --- ECIES Decryption (1KB message) --- */
  {
    size_t large_len = 1024;
    uint8_t *large_msg = malloc(large_len);
    for (size_t i = 0; i < large_len; ++i)
      large_msg[i] = (uint8_t)(i & 0xFF);
    size_t ct_len = FIO_X25519_CIPHERTEXT_LEN(large_len);
    uint8_t *ciphertext = malloc(ct_len);
    uint8_t *plaintext = malloc(large_len);
    fio_x25519_encrypt(ciphertext,
                       large_msg,
                       large_len,
                       fio_chacha20_poly1305_enc,
                       x_pk);

    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_decrypt(plaintext,
                         ciphertext,
                         ct_len,
                         fio_chacha20_poly1305_dec,
                         x_sk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "ECIES decrypt (1KB message, using ChaCha20Poly1305)",
            ops_per_sec,
            elapsed_us / iterations);
    free(large_msg);
    free(ciphertext);
    free(plaintext);
  }

  /* --- ECIES Encryption (1KB message) AES --- */
  {
    size_t large_len = 1024;
    uint8_t *large_msg = malloc(large_len);
    for (size_t i = 0; i < large_len; ++i)
      large_msg[i] = (uint8_t)(i & 0xFF);
    size_t ct_len = FIO_X25519_CIPHERTEXT_LEN(large_len);
    uint8_t *ciphertext = malloc(ct_len);

    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_encrypt(ciphertext,
                         large_msg,
                         large_len,
                         fio_aes256_gcm_enc,
                         x_pk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "ECIES encrypt (1KB message, using AES256-GCM)",
            ops_per_sec,
            elapsed_us / iterations);
    free(large_msg);
    free(ciphertext);
  }

  /* --- ECIES Decryption (1KB message) AES --- */
  {
    size_t large_len = 1024;
    uint8_t *large_msg = malloc(large_len);
    for (size_t i = 0; i < large_len; ++i)
      large_msg[i] = (uint8_t)(i & 0xFF);
    size_t ct_len = FIO_X25519_CIPHERTEXT_LEN(large_len);
    uint8_t *ciphertext = malloc(ct_len);
    uint8_t *plaintext = malloc(large_len);
    fio_x25519_encrypt(ciphertext,
                       large_msg,
                       large_len,
                       fio_aes256_gcm_enc,
                       x_pk);

    start = fio___ed25519_time_us();
    for (size_t i = 0; i < iterations; ++i) {
      fio_x25519_decrypt(plaintext,
                         ciphertext,
                         ct_len,
                         fio_aes256_gcm_dec,
                         x_sk);
      FIO_COMPILER_GUARD;
    }
    end = fio___ed25519_time_us();
    elapsed_us = (double)(end - start);
    ops_per_sec = (iterations * 1000000.0) / elapsed_us;
    fprintf(stderr,
            "\t  %-40s %10.2f ops/sec  (%6.2f us/op)\n",
            "ECIES decrypt (1KB message, using AES256-GCM)",
            ops_per_sec,
            elapsed_us / iterations);
    free(large_msg);
    free(ciphertext);
    free(plaintext);
  }

  fprintf(stderr, "\n\t  Performance tests complete.\n");
}

/* *****************************************************************************
Main Test Function
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, ed25519)(void) {
  fprintf(stderr, "\t* Testing Ed25519 / X25519 implementation\n");
  FIO_NAME_TEST(stl, ed25519_rfc8032)();
  FIO_NAME_TEST(stl, x25519_rfc7748)();
  FIO_NAME_TEST(stl, ed25519_x25519_conversion)();
  FIO_NAME_TEST(stl, x25519_ecies)();
#if !DEBUG
  FIO_NAME_TEST(stl, ed25519_performance)();
#endif
  fprintf(stderr, "\t* Ed25519 / X25519 tests complete.\n");
}

/* *****************************************************************************
Test Runner
***************************************************************************** */

int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;
  FIO_NAME_TEST(stl, ed25519)();
  return 0;
}
