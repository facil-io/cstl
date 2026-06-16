/* *****************************************************************************
Test for 154 ed25519.h

Coverage: Ed25519 keypair generation, public key derivation, sign/verify, and
Ed25519-to-X25519 key conversion using RFC 8032 test vectors and deterministic
edge cases. Performance loops are intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_ED25519
#define FIO_SHA2
#include FIO_INCLUDE_FILE

/* *****************************************************************************
RFC 8032 Ed25519 Test Vectors
https://www.rfc-editor.org/rfc/rfc8032#section-7.1
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, ed25519_rfc8032)(void) {
  /* Test Vector 1: Empty message. */
  {
    static const uint8_t sk[32] = {
        0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60, 0xba, 0x84, 0x4a,
        0xf4, 0x92, 0xec, 0x2c, 0xc4, 0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32,
        0x69, 0x19, 0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60};
    static const uint8_t expected_pk[32] = {
        0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7, 0xd5, 0x4b, 0xfe,
        0xd3, 0xc9, 0x64, 0x07, 0x3a, 0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6,
        0x23, 0x25, 0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a};
    static const uint8_t expected_sig[64] = {
        0xe5, 0x56, 0x43, 0x00, 0xc3, 0x60, 0xac, 0x72, 0x90, 0x86, 0xe2,
        0xcc, 0x80, 0x6e, 0x82, 0x8a, 0x84, 0x87, 0x7f, 0x1e, 0xb8, 0xe5,
        0xd9, 0x74, 0xd8, 0x73, 0xe0, 0x65, 0x22, 0x49, 0x01, 0x55, 0x5f,
        0xb8, 0x82, 0x15, 0x90, 0xa3, 0x3b, 0xac, 0xc6, 0x1e, 0x39, 0x70,
        0x1c, 0xf9, 0xb4, 0x6b, 0xd2, 0x5b, 0xf5, 0xf0, 0x59, 0x5b, 0xbe,
        0x24, 0x65, 0x51, 0x41, 0x43, 0x8e, 0x7a, 0x10, 0x0b};

    uint8_t pk[32], sig[64];
    fio_ed25519_public_key(pk, sk);
    FIO_ASSERT(!FIO_MEMCMP(pk, expected_pk, 32),
               "Ed25519 public key derivation failed (RFC 8032 vector 1)");

    fio_ed25519_sign(sig, "", 0, sk, pk);
    FIO_ASSERT(!FIO_MEMCMP(sig, expected_sig, 64),
               "Ed25519 signature failed (RFC 8032 vector 1)");
    FIO_ASSERT(fio_ed25519_verify(sig, "", 0, pk) == 0,
               "Ed25519 verification failed (RFC 8032 vector 1)");
  }

  /* Test Vector 2: Single byte message (0x72). */
  {
    static const uint8_t sk[32] = {
        0x4c, 0xcd, 0x08, 0x9b, 0x28, 0xff, 0x96, 0xda, 0x9d, 0xb6, 0xc3,
        0x46, 0xec, 0x11, 0x4e, 0x0f, 0x5b, 0x8a, 0x31, 0x9f, 0x35, 0xab,
        0xa6, 0x24, 0xda, 0x8c, 0xf6, 0xed, 0x4f, 0xb8, 0xa6, 0xfb};
    static const uint8_t expected_pk[32] = {
        0x3d, 0x40, 0x17, 0xc3, 0xe8, 0x43, 0x89, 0x5a, 0x92, 0xb7, 0x0a,
        0xa7, 0x4d, 0x1b, 0x7e, 0xbc, 0x9c, 0x98, 0x2c, 0xcf, 0x2e, 0xc4,
        0x96, 0x8c, 0xc0, 0xcd, 0x55, 0xf1, 0x2a, 0xf4, 0x66, 0x0c};
    static const uint8_t message[1] = {0x72};
    static const uint8_t expected_sig[64] = {
        0x92, 0xa0, 0x09, 0xa9, 0xf0, 0xd4, 0xca, 0xb8, 0x72, 0x0e, 0x82,
        0x0b, 0x5f, 0x64, 0x25, 0x40, 0xa2, 0xb2, 0x7b, 0x54, 0x16, 0x50,
        0x3f, 0x8f, 0xb3, 0x76, 0x22, 0x23, 0xeb, 0xdb, 0x69, 0xda, 0x08,
        0x5a, 0xc1, 0xe4, 0x3e, 0x15, 0x99, 0x6e, 0x45, 0x8f, 0x36, 0x13,
        0xd0, 0xf1, 0x1d, 0x8c, 0x38, 0x7b, 0x2e, 0xae, 0xb4, 0x30, 0x2a,
        0xee, 0xb0, 0x0d, 0x29, 0x16, 0x12, 0xbb, 0x0c, 0x00};

    uint8_t pk[32], sig[64];
    fio_ed25519_public_key(pk, sk);
    FIO_ASSERT(!FIO_MEMCMP(pk, expected_pk, 32),
               "Ed25519 public key derivation failed (RFC 8032 vector 2)");

    fio_ed25519_sign(sig, message, 1, sk, pk);
    FIO_ASSERT(!FIO_MEMCMP(sig, expected_sig, 64),
               "Ed25519 signature failed (RFC 8032 vector 2)");
    FIO_ASSERT(fio_ed25519_verify(sig, message, 1, pk) == 0,
               "Ed25519 verification failed (RFC 8032 vector 2)");
  }

  /* Test Vector 3: Two byte message. */
  {
    static const uint8_t sk[32] = {
        0xc5, 0xaa, 0x8d, 0xf4, 0x3f, 0x9f, 0x83, 0x7b, 0xed, 0xb7, 0x44,
        0x2f, 0x31, 0xdc, 0xb7, 0xb1, 0x66, 0xd3, 0x85, 0x35, 0x07, 0x6f,
        0x09, 0x4b, 0x85, 0xce, 0x3a, 0x2e, 0x0b, 0x44, 0x58, 0xf7};
    static const uint8_t expected_pk[32] = {
        0xfc, 0x51, 0xcd, 0x8e, 0x62, 0x18, 0xa1, 0xa3, 0x8d, 0xa4, 0x7e,
        0xd0, 0x02, 0x30, 0xf0, 0x58, 0x08, 0x16, 0xed, 0x13, 0xba, 0x33,
        0x03, 0xac, 0x5d, 0xeb, 0x91, 0x15, 0x48, 0x90, 0x80, 0x25};
    static const uint8_t message[2] = {0xaf, 0x82};
    static const uint8_t expected_sig[64] = {
        0x62, 0x91, 0xd6, 0x57, 0xde, 0xec, 0x24, 0x02, 0x48, 0x27, 0xe6,
        0x9c, 0x3a, 0xbe, 0x01, 0xa3, 0x0c, 0xe5, 0x48, 0xa2, 0x84, 0x74,
        0x3a, 0x44, 0x5e, 0x36, 0x80, 0xd7, 0xdb, 0x5a, 0xc3, 0xac, 0x18,
        0xff, 0x9b, 0x53, 0x8d, 0x16, 0xf2, 0x90, 0xae, 0x67, 0xf7, 0x60,
        0x98, 0x4d, 0xc6, 0x59, 0x4a, 0x7c, 0x15, 0xe9, 0x71, 0x6e, 0xd2,
        0x8d, 0xc0, 0x27, 0xbe, 0xce, 0xea, 0x1e, 0xc4, 0x0a};

    uint8_t pk[32], sig[64];
    fio_ed25519_public_key(pk, sk);
    FIO_ASSERT(!FIO_MEMCMP(pk, expected_pk, 32),
               "Ed25519 public key derivation failed (RFC 8032 vector 3)");

    fio_ed25519_sign(sig, message, 2, sk, pk);
    FIO_ASSERT(!FIO_MEMCMP(sig, expected_sig, 64),
               "Ed25519 signature failed (RFC 8032 vector 3)");
    FIO_ASSERT(fio_ed25519_verify(sig, message, 2, pk) == 0,
               "Ed25519 verification failed (RFC 8032 vector 3)");
  }
}

/* *****************************************************************************
Ed25519 Signature Edge Cases
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, ed25519_signature_edge_cases)(void) {
  uint8_t sk[32], pk[32], sig[64];
  fio_ed25519_keypair(sk, pk);

  const char *message = "Test message for signature verification";
  size_t message_len = FIO_STRLEN(message);

  fio_ed25519_sign(sig, message, message_len, sk, pk);
  FIO_ASSERT(fio_ed25519_verify(sig, message, message_len, pk) == 0,
             "Valid Ed25519 signature should verify");

  /* Modify signature and verify it fails. */
  sig[0] ^= 0x01;
  FIO_ASSERT(fio_ed25519_verify(sig, message, message_len, pk) != 0,
             "Ed25519 verification should fail for modified signature");
  sig[0] ^= 0x01;

  /* Modify message and verify it fails. */
  const char *wrong_message = "Wrong message for signature verification";
  FIO_ASSERT(fio_ed25519_verify(sig, wrong_message, FIO_STRLEN(wrong_message), pk) !=
                 0,
             "Ed25519 verification should fail for wrong message");

  /* Flip each byte of the signature. */
  for (size_t i = 0; i < 64; ++i) {
    uint8_t corrupted_sig[64];
    FIO_MEMCPY(corrupted_sig, sig, 64);
    corrupted_sig[i] ^= 0x01;
    FIO_ASSERT(fio_ed25519_verify(corrupted_sig, message, message_len, pk) != 0,
               "Ed25519 should reject signature with byte %zu flipped",
               i);
  }

  /* All-zero signature should fail. */
  {
    uint8_t zero_sig[64] = {0};
    FIO_ASSERT(fio_ed25519_verify(zero_sig, message, message_len, pk) != 0,
               "All-zero Ed25519 signature should fail");
  }

  /* All-ones signature should fail. */
  {
    uint8_t ones_sig[64];
    FIO_MEMSET(ones_sig, 0xFF, 64);
    FIO_ASSERT(fio_ed25519_verify(ones_sig, message, message_len, pk) != 0,
               "All-ones Ed25519 signature should fail");
  }

  /* Empty message signature roundtrip. */
  {
    uint8_t empty_sig[64];
    fio_ed25519_sign(empty_sig, "", 0, sk, pk);
    FIO_ASSERT(fio_ed25519_verify(empty_sig, "", 0, pk) == 0,
               "Empty message Ed25519 signature should verify");
    empty_sig[0] ^= 0x01;
    FIO_ASSERT(fio_ed25519_verify(empty_sig, "", 0, pk) != 0,
               "Corrupted empty message Ed25519 signature should fail");
  }

  /* Large message (64KB). */
  {
    size_t large_len = 65536;
    uint8_t *large_msg =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, large_len, large_len);
    FIO_ASSERT(large_msg != NULL, "Failed to allocate large message");
    for (size_t i = 0; i < large_len; ++i)
      large_msg[i] = (uint8_t)(i & 0xFF);

    uint8_t large_sig[64];
    fio_ed25519_sign(large_sig, large_msg, large_len, sk, pk);
    FIO_ASSERT(fio_ed25519_verify(large_sig, large_msg, large_len, pk) == 0,
               "Large message Ed25519 signature should verify");

    large_msg[large_len / 2] ^= 0x01;
    FIO_ASSERT(fio_ed25519_verify(large_sig, large_msg, large_len, pk) != 0,
               "Modified large message Ed25519 should fail verification");
    FIO_MEM_FREE(large_msg, large_len);
  }

  /* Signatures are deterministic. */
  {
    uint8_t sig2[64];
    fio_ed25519_sign(sig2, message, message_len, sk, pk);
    FIO_ASSERT(!FIO_MEMCMP(sig, sig2, 64),
               "Ed25519 signatures should be deterministic");
  }

  /* Different messages produce different signatures. */
  {
    const char *message2 = "Different test message";
    uint8_t sig2[64];
    fio_ed25519_sign(sig2, message2, FIO_STRLEN(message2), sk, pk);
    FIO_ASSERT(FIO_MEMCMP(sig, sig2, 64) != 0,
               "Different messages should produce different Ed25519 signatures");
  }

  /* Different keys produce different signatures. */
  {
    uint8_t sk2[32], pk2[32], sig2[64];
    fio_ed25519_keypair(sk2, pk2);
    fio_ed25519_sign(sig2, message, message_len, sk2, pk2);
    FIO_ASSERT(FIO_MEMCMP(sig, sig2, 64) != 0,
               "Different keys should produce different Ed25519 signatures");
  }

  /* Signature from one key does not verify with another key. */
  {
    uint8_t sk2[32], pk2[32];
    fio_ed25519_keypair(sk2, pk2);
    FIO_ASSERT(fio_ed25519_verify(sig, message, message_len, pk2) != 0,
               "Ed25519 signature should not verify with wrong public key");
  }
}

/* *****************************************************************************
Ed25519 to X25519 Key Conversion
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, ed25519_x25519_conversion)(void) {
  uint8_t ed_sk[32], ed_pk[32];
  uint8_t x_sk[32], x_pk[32];
  uint8_t other_sk[32], other_pk[32];
  uint8_t shared1[32], shared2[32];

  fio_ed25519_keypair(ed_sk, ed_pk);
  fio_ed25519_sk_to_x25519(x_sk, ed_sk);
  fio_ed25519_pk_to_x25519(x_pk, ed_pk);

  fio_x25519_keypair(other_sk, other_pk);

  FIO_ASSERT(fio_x25519_shared_secret(shared1, x_sk, other_pk) == 0,
             "X25519 shared secret with converted Ed25519 secret key failed");
  FIO_ASSERT(fio_x25519_shared_secret(shared2, other_sk, x_pk) == 0,
             "X25519 shared secret with converted Ed25519 public key failed");
  FIO_ASSERT(!FIO_MEMCMP(shared1, shared2, 32),
             "ECDH with converted Ed25519 keys produced mismatched shared secrets");
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  FIO_NAME_TEST(stl, ed25519_rfc8032)();
  FIO_NAME_TEST(stl, ed25519_signature_edge_cases)();
  FIO_NAME_TEST(stl, ed25519_x25519_conversion)();
  return 0;
}
