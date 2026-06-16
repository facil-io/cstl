/* *****************************************************************************
Test for 154 p256.h

Coverage: P-256 (secp256r1) field arithmetic, base point validation, scalar
multiplication, ECDSA sign/verify using RFC 6979 / NIST test vectors, DER
signature parsing, invalid signature rejection, and ECDHE key exchange
including compressed public keys. Relevant hardening assertions from the
archived P-256 tests and tls13-p256-sign.c are included. Performance loops are
intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_P256
#define FIO_SHA2
#define FIO_RAND
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#endif

/* *****************************************************************************
P-256 Curve Constants
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_constants)(void) {
  FIO_ASSERT(FIO___P256_P[0] == 0xFFFFFFFFFFFFFFFFULL, "P-256 P[0] incorrect");
  FIO_ASSERT(FIO___P256_P[1] == 0x00000000FFFFFFFFULL, "P-256 P[1] incorrect");
  FIO_ASSERT(FIO___P256_P[2] == 0x0000000000000000ULL, "P-256 P[2] incorrect");
  FIO_ASSERT(FIO___P256_P[3] == 0xFFFFFFFF00000001ULL, "P-256 P[3] incorrect");

  FIO_ASSERT(FIO___P256_N[0] == 0xF3B9CAC2FC632551ULL, "P-256 N[0] incorrect");
  FIO_ASSERT(FIO___P256_N[1] == 0xBCE6FAADA7179E84ULL, "P-256 N[1] incorrect");
  FIO_ASSERT(FIO___P256_N[2] == 0xFFFFFFFFFFFFFFFFULL, "P-256 N[2] incorrect");
  FIO_ASSERT(FIO___P256_N[3] == 0xFFFFFFFF00000000ULL, "P-256 N[3] incorrect");
}

/* *****************************************************************************
P-256 Field Arithmetic
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_field_ops)(void) {
  fio___p256_fe_s a, b, c;

  /* 1 + 2 = 3 */
  fio___p256_fe_one(a);
  b[0] = 2;
  b[1] = b[2] = b[3] = 0;
  fio___p256_fe_add(c, a, b);
  FIO_ASSERT(c[0] == 3 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "P-256 field addition 1+2 failed");

  /* 5 - 3 = 2 */
  a[0] = 5;
  a[1] = a[2] = a[3] = 0;
  b[0] = 3;
  b[1] = b[2] = b[3] = 0;
  fio___p256_fe_sub(c, a, b);
  FIO_ASSERT(c[0] == 2 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "P-256 field subtraction 5-3 failed");

  /* 2 * 3 = 6 */
  a[0] = 2;
  b[0] = 3;
  fio___p256_fe_mul(c, a, b);
  FIO_ASSERT(c[0] == 6 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "P-256 field multiplication 2*3 failed");

  /* 4^2 = 16 */
  a[0] = 4;
  fio___p256_fe_sqr(c, a);
  FIO_ASSERT(c[0] == 16 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "P-256 field squaring 4^2 failed");

  /* 7 * 7^(-1) = 1 */
  a[0] = 7;
  a[1] = a[2] = a[3] = 0;
  fio___p256_fe_inv(b, a);
  fio___p256_fe_mul(c, a, b);
  FIO_ASSERT(c[0] == 1 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "P-256 field inversion 7 * 7^(-1) != 1");

  /* Gx * Gx^(-1) = 1 */
  fio___p256_fe_copy(a, FIO___P256_GX);
  fio___p256_fe_inv(b, a);
  fio___p256_fe_mul(c, a, b);
  FIO_ASSERT(c[0] == 1 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "P-256 field inversion Gx * Gx^(-1) != 1");

  /* Regression case: 0x168f08f8e7 * 0x1e39a5057d81 mod p. */
  a[0] = 0x168f08f8e7ULL;
  a[1] = a[2] = a[3] = 0;
  b[0] = 0x1e39a5057d81ULL;
  b[1] = b[2] = b[3] = 0;
  fio___p256_fe_mul(c, a, b);
  FIO_ASSERT(c[0] == 0x70d7203e34913767ULL && c[1] == 0x2a9d7ULL &&
                 c[2] == 0 && c[3] == 0,
             "P-256 regression multiplication result incorrect");

  /* Regression case: base6^2 mod p. */
  a[0] = 0x199417c8c0bb7601ULL;
  a[1] = 0xd63b78e780e1341eULL;
  a[2] = 0x000cbc21fe4561c8ULL;
  a[3] = 0;
  fio___p256_fe_sqr(c, a);
  FIO_ASSERT(c[0] == 0xd2b4192c08178833ULL &&
                 c[1] == 0x6fb4ed6ea70cff34ULL &&
                 c[2] == 0x9b30eca65bf400f1ULL &&
                 c[3] == 0x15b30a3d45561e22ULL,
             "P-256 regression squaring base6 incorrect");
}

/* *****************************************************************************
P-256 Base Point and Scalar Multiplication
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_base_point)(void) {
  fio___p256_fe_s y2, x3, t;

  fio___p256_fe_sqr(y2, FIO___P256_GY);
  fio___p256_fe_sqr(t, FIO___P256_GX);
  fio___p256_fe_mul(x3, t, FIO___P256_GX);
  fio___p256_fe_sub(t, x3, FIO___P256_GX);
  fio___p256_fe_sub(t, t, FIO___P256_GX);
  fio___p256_fe_sub(t, t, FIO___P256_GX);
  fio___p256_fe_add(t, t, FIO___P256_B);

  FIO_ASSERT(fio___p256_fe_eq(y2, t) == 0,
             "P-256 base point G is not on the curve");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p256_scalar_mul)(void) {
  fio___p256_point_affine_s g;
  fio___p256_fe_copy(g.x, FIO___P256_GX);
  fio___p256_fe_copy(g.y, FIO___P256_GY);

  /* 1 * G = G */
  {
    fio___p256_scalar_s one = {1, 0, 0, 0};
    fio___p256_point_jacobian_s r;
    fio___p256_point_mul(&r, one, &g);
    fio___p256_point_affine_s r_aff;
    fio___p256_point_to_affine(&r_aff, &r);
    FIO_ASSERT(fio___p256_fe_eq(r_aff.x, FIO___P256_GX) == 0,
               "P-256 1*G x-coordinate mismatch");
    FIO_ASSERT(fio___p256_fe_eq(r_aff.y, FIO___P256_GY) == 0,
               "P-256 1*G y-coordinate mismatch");
  }

  /* 2 * G is on the curve. */
  {
    fio___p256_scalar_s two = {2, 0, 0, 0};
    fio___p256_point_jacobian_s r;
    fio___p256_point_mul(&r, two, &g);
    fio___p256_point_affine_s r_aff;
    fio___p256_point_to_affine(&r_aff, &r);

    fio___p256_fe_s y2, x3, t;
    fio___p256_fe_sqr(y2, r_aff.y);
    fio___p256_fe_sqr(t, r_aff.x);
    fio___p256_fe_mul(x3, t, r_aff.x);
    fio___p256_fe_sub(t, x3, r_aff.x);
    fio___p256_fe_sub(t, t, r_aff.x);
    fio___p256_fe_sub(t, t, r_aff.x);
    fio___p256_fe_add(t, t, FIO___P256_B);
    FIO_ASSERT(fio___p256_fe_eq(y2, t) == 0, "P-256 2*G is not on the curve");
  }

  /* G + G = 2G */
  {
    fio___p256_point_jacobian_s a, b;
    fio___p256_point_to_jacobian(&a, &g);
    fio___p256_point_double(&a, &a);
    fio___p256_point_to_jacobian(&b, &g);
    fio___p256_point_add_mixed(&b, &b, &g);
    fio___p256_point_affine_s a_aff, b_aff;
    fio___p256_point_to_affine(&a_aff, &a);
    fio___p256_point_to_affine(&b_aff, &b);
    FIO_ASSERT(fio___p256_fe_eq(a_aff.x, b_aff.x) == 0,
               "P-256 G + G != 2G");
  }
}

/* *****************************************************************************
P-256 ECDSA RFC 6979 / NIST Test Vector
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdsa_nist)(void) {
  /* SHA-256("sample") */
  static const uint8_t msg_hash[32] = {
      0xaf, 0x2b, 0xdb, 0xe1, 0xaa, 0x9b, 0x6e, 0xc1, 0xe2, 0xad, 0xe1,
      0xd6, 0x94, 0xf4, 0x1f, 0xc7, 0x1a, 0x83, 0x1d, 0x02, 0x68, 0xe9,
      0x89, 0x15, 0x62, 0x11, 0x3d, 0x8a, 0x62, 0xad, 0xd1, 0xbf};

  static const uint8_t pubkey[65] = {
      0x04, 0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61,
      0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b,
      0x61, 0xfa, 0x6c, 0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
      0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99, 0xa4, 0x1a, 0xe9,
      0xe9, 0x56, 0x28, 0xbc, 0x64, 0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e,
      0x9f, 0x51, 0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99};

  static const uint8_t r[32] = {
      0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8, 0xfd, 0x11, 0x40, 0xdd,
      0x9c, 0xd4, 0x5e, 0x81, 0xd6, 0x9d, 0x2c, 0x87, 0x7b, 0x56, 0xaa,
      0xf9, 0x91, 0xc3, 0x4d, 0x0e, 0xa8, 0x4e, 0xaf, 0x37, 0x16};
  static const uint8_t s[32] = {
      0xf7, 0xcb, 0x1c, 0x94, 0x2d, 0x65, 0x7c, 0x41, 0xd4, 0x36, 0xc7,
      0xa1, 0xb6, 0xe2, 0x9f, 0x65, 0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf,
      0xf4, 0x06, 0x4d, 0xc4, 0xab, 0x2f, 0x84, 0x3a, 0xcd, 0xa8};

  FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, s, msg_hash, pubkey + 1, pubkey + 33) ==
                 0,
             "P-256 ECDSA NIST/RFC 6979 test vector failed");
}

/* *****************************************************************************
P-256 DER Signature Parsing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_der_parsing)(void) {
  static const uint8_t msg_hash[32] = {
      0xaf, 0x2b, 0xdb, 0xe1, 0xaa, 0x9b, 0x6e, 0xc1, 0xe2, 0xad, 0xe1,
      0xd6, 0x94, 0xf4, 0x1f, 0xc7, 0x1a, 0x83, 0x1d, 0x02, 0x68, 0xe9,
      0x89, 0x15, 0x62, 0x11, 0x3d, 0x8a, 0x62, 0xad, 0xd1, 0xbf};

  static const uint8_t pubkey[65] = {
      0x04, 0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61,
      0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b,
      0x61, 0xfa, 0x6c, 0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
      0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99, 0xa4, 0x1a, 0xe9,
      0xe9, 0x56, 0x28, 0xbc, 0x64, 0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e,
      0x9f, 0x51, 0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99};

  static const uint8_t der_sig[] = {
      0x30, 0x45, 0x02, 0x20, 0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8,
      0xfd, 0x11, 0x40, 0xdd, 0x9c, 0xd4, 0x5e, 0x81, 0xd6, 0x9d, 0x2c,
      0x87, 0x7b, 0x56, 0xaa, 0xf9, 0x91, 0xc3, 0x4d, 0x0e, 0xa8, 0x4e,
      0xaf, 0x37, 0x16, 0x02, 0x21, 0x00, 0xf7, 0xcb, 0x1c, 0x94, 0x2d,
      0x65, 0x7c, 0x41, 0xd4, 0x36, 0xc7, 0xa1, 0xb6, 0xe2, 0x9f, 0x65,
      0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf, 0xf4, 0x06, 0x4d, 0xc4, 0xab,
      0x2f, 0x84, 0x3a, 0xcd, 0xa8};

  FIO_ASSERT(fio_ecdsa_p256_verify(der_sig, sizeof(der_sig), msg_hash, pubkey,
                                   sizeof(pubkey)) == 0,
             "P-256 DER signature verification failed");
}

/* *****************************************************************************
P-256 Invalid Signature Rejection
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_invalid_sigs)(void) {
  static const uint8_t msg_hash[32] = {
      0xaf, 0x2b, 0xdb, 0xe1, 0xaa, 0x9b, 0x6e, 0xc1, 0xe2, 0xad, 0xe1,
      0xd6, 0x94, 0xf4, 0x1f, 0xc7, 0x1a, 0x83, 0x1d, 0x02, 0x68, 0xe9,
      0x89, 0x15, 0x62, 0x11, 0x3d, 0x8a, 0x62, 0xad, 0xd1, 0xbf};

  static const uint8_t pubkey[65] = {
      0x04, 0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61,
      0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b,
      0x61, 0xfa, 0x6c, 0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
      0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99, 0xa4, 0x1a, 0xe9,
      0xe9, 0x56, 0x28, 0xbc, 0x64, 0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e,
      0x9f, 0x51, 0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99};

  static const uint8_t r[32] = {
      0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8, 0xfd, 0x11, 0x40, 0xdd,
      0x9c, 0xd4, 0x5e, 0x81, 0xd6, 0x9d, 0x2c, 0x87, 0x7b, 0x56, 0xaa,
      0xf9, 0x91, 0xc3, 0x4d, 0x0e, 0xa8, 0x4e, 0xaf, 0x37, 0x16};
  static const uint8_t s[32] = {
      0xf7, 0xcb, 0x1c, 0x94, 0x2d, 0x65, 0x7c, 0x41, 0xd4, 0x36, 0xc7,
      0xa1, 0xb6, 0xe2, 0x9f, 0x65, 0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf,
      0xf4, 0x06, 0x4d, 0xc4, 0xab, 0x2f, 0x84, 0x3a, 0xcd, 0xa8};

  FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, s, msg_hash, pubkey + 1, pubkey + 33) ==
                 0,
             "P-256 valid signature should verify");

  /* Modified r. */
  {
    uint8_t bad_r[32];
    FIO_MEMCPY(bad_r, r, 32);
    bad_r[0] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(bad_r, s, msg_hash, pubkey + 1,
                                         pubkey + 33) != 0,
               "P-256 should reject modified r");
  }

  /* Modified s. */
  {
    uint8_t bad_s[32];
    FIO_MEMCPY(bad_s, s, 32);
    bad_s[0] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, bad_s, msg_hash, pubkey + 1,
                                         pubkey + 33) != 0,
               "P-256 should reject modified s");
  }

  /* Modified message hash. */
  {
    uint8_t bad_hash[32];
    FIO_MEMCPY(bad_hash, msg_hash, 32);
    bad_hash[0] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, s, bad_hash, pubkey + 1,
                                         pubkey + 33) != 0,
               "P-256 should reject modified message hash");
  }

  /* r = 0 or s = 0. */
  {
    uint8_t zero[32] = {0};
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(zero, s, msg_hash, pubkey + 1,
                                         pubkey + 33) != 0,
               "P-256 should reject r = 0");
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, zero, msg_hash, pubkey + 1,
                                         pubkey + 33) != 0,
               "P-256 should reject s = 0");
  }

  /* r >= n or s >= n. */
  {
    uint8_t n_bytes[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17,
        0x9E, 0x84, 0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51};
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(n_bytes, s, msg_hash, pubkey + 1,
                                         pubkey + 33) != 0,
               "P-256 should reject r = n");
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, n_bytes, msg_hash, pubkey + 1,
                                         pubkey + 33) != 0,
               "P-256 should reject s = n");
  }

  /* Flip every byte of r and s. */
  for (size_t i = 0; i < 32; ++i) {
    uint8_t bad_r[32];
    FIO_MEMCPY(bad_r, r, 32);
    bad_r[i] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(bad_r, s, msg_hash, pubkey + 1,
                                         pubkey + 33) != 0,
               "P-256 should reject r with byte %zu flipped", i);
  }
  for (size_t i = 0; i < 32; ++i) {
    uint8_t bad_s[32];
    FIO_MEMCPY(bad_s, s, 32);
    bad_s[i] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, bad_s, msg_hash, pubkey + 1,
                                         pubkey + 33) != 0,
               "P-256 should reject s with byte %zu flipped", i);
  }
}

/* *****************************************************************************
P-256 ECDHE Key Exchange
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdhe)(void) {
  uint8_t sk1[32], pk1[65];
  uint8_t sk2[32], pk2[65];
  uint8_t shared1[32], shared2[32];

  FIO_ASSERT(fio_p256_keypair(sk1, pk1) == 0,
             "P-256 first keypair generation failed");
  FIO_ASSERT(pk1[0] == 0x04,
             "P-256 public key should be uncompressed (0x04 prefix)");

  FIO_ASSERT(fio_p256_keypair(sk2, pk2) == 0,
             "P-256 second keypair generation failed");
  FIO_ASSERT(pk2[0] == 0x04,
             "P-256 public key should be uncompressed (0x04 prefix)");

  FIO_ASSERT(FIO_MEMCMP(sk1, sk2, 32) != 0,
             "P-256 secret keys should be different");
  FIO_ASSERT(FIO_MEMCMP(pk1, pk2, 65) != 0,
             "P-256 public keys should be different");

  FIO_ASSERT(fio_p256_shared_secret(shared1, sk1, pk2, 65) == 0,
             "P-256 Alice shared secret failed");
  FIO_ASSERT(fio_p256_shared_secret(shared2, sk2, pk1, 65) == 0,
             "P-256 Bob shared secret failed");
  FIO_ASSERT(!FIO_MEMCMP(shared1, shared2, 32),
             "P-256 ECDH shared secrets mismatch");

  uint8_t non_zero = 0;
  for (int i = 0; i < 32; ++i)
    non_zero |= shared1[i];
  FIO_ASSERT(non_zero != 0, "P-256 shared secret should not be all zeros");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdhe_invalid_inputs)(void) {
  uint8_t sk[32], pk[65], shared[32];
  FIO_ASSERT(fio_p256_keypair(sk, pk) == 0, "P-256 keypair generation failed");

  FIO_ASSERT(fio_p256_keypair(NULL, pk) == -1,
             "P-256 keypair should reject NULL secret key");
  FIO_ASSERT(fio_p256_keypair(sk, NULL) == -1,
             "P-256 keypair should reject NULL public key");
  FIO_ASSERT(fio_p256_shared_secret(NULL, sk, pk, 65) == -1,
             "P-256 shared secret should reject NULL output");
  FIO_ASSERT(fio_p256_shared_secret(shared, NULL, pk, 65) == -1,
             "P-256 shared secret should reject NULL secret key");
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, NULL, 65) == -1,
             "P-256 shared secret should reject NULL public key");
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, pk, 64) == -1,
             "P-256 shared secret should reject invalid key length (64)");
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, pk, 32) == -1,
             "P-256 shared secret should reject invalid key length (32)");

  uint8_t bad_pk[65];
  FIO_MEMCPY(bad_pk, pk, 65);
  bad_pk[0] = 0x05;
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, bad_pk, 65) == -1,
             "P-256 shared secret should reject invalid prefix");

  FIO_MEMCPY(bad_pk, pk, 65);
  bad_pk[64] ^= 0xFF;
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, bad_pk, 65) == -1,
             "P-256 shared secret should reject point not on curve");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdhe_compressed)(void) {
  uint8_t sk_alice[32], pk_alice[65];
  uint8_t sk_bob[32], pk_bob[65];
  uint8_t shared_uncomp[32], shared_comp[32];

  FIO_ASSERT(fio_p256_keypair(sk_alice, pk_alice) == 0,
             "P-256 Alice keypair generation failed");
  FIO_ASSERT(fio_p256_keypair(sk_bob, pk_bob) == 0,
             "P-256 Bob keypair generation failed");

  FIO_ASSERT(fio_p256_shared_secret(shared_uncomp, sk_alice, pk_bob, 65) == 0,
             "P-256 uncompressed shared secret failed");

  uint8_t pk_bob_compressed[33];
  pk_bob_compressed[0] = (pk_bob[64] & 1) ? 0x03 : 0x02;
  FIO_MEMCPY(pk_bob_compressed + 1, pk_bob + 1, 32);

  FIO_ASSERT(fio_p256_shared_secret(shared_comp, sk_alice, pk_bob_compressed,
                                    33) == 0,
             "P-256 compressed shared secret failed");
  FIO_ASSERT(!FIO_MEMCMP(shared_uncomp, shared_comp, 32),
             "P-256 compressed and uncompressed shared secrets mismatch");
}

/* *****************************************************************************
P-256 Sign/Verify Roundtrip (replaces tls13-p256-sign.c)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_sign_verify_roundtrip)(void) {
  uint8_t sk[32], pk[65];
  FIO_ASSERT(fio_p256_keypair(sk, pk) == 0,
             "P-256 keypair generation for sign/verify failed");

  const uint8_t msg[] = "TLS 1.3, server CertificateVerify";
  fio_u256 hash = fio_sha256(msg, sizeof(msg) - 1);

  uint8_t sig[72];
  size_t sig_len = 0;
  FIO_ASSERT(fio_ecdsa_p256_sign(sig, &sig_len, sizeof(sig), hash.u8, sk) == 0,
             "P-256 signing should succeed");
  FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, hash.u8, pk, sizeof(pk)) == 0,
             "P-256 self verification should accept generated signature");

  /* Corrupted signature should fail. */
  sig[4] ^= 0x01;
  FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, hash.u8, pk, sizeof(pk)) != 0,
             "P-256 should reject corrupted generated signature");
}

/* *****************************************************************************
P-256 OpenSSL Cross-Validation (library only)
***************************************************************************** */

#if HAVE_OPENSSL
FIO_SFUNC void FIO_NAME_TEST(stl, p256_openssl_verify)(void) {
  uint8_t secret_key[32] = {0xc9, 0xaf, 0xa9, 0xd8, 0x45, 0xba, 0x75, 0x16,
                            0x6b, 0x5c, 0x21, 0x57, 0x67, 0xb1, 0xd6, 0x93,
                            0x4e, 0x50, 0xc3, 0xdb, 0x36, 0xe8, 0x9b, 0x12,
                            0x7b, 0x8a, 0x62, 0x2b, 0x12, 0x0f, 0x67, 0x21};
  uint8_t public_key[65] = {
      0x04, 0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61,
      0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b,
      0x61, 0xfa, 0x6c, 0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
      0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99, 0xa4, 0x1a, 0xe9,
      0xe9, 0x56, 0x28, 0xbc, 0x64, 0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e,
      0x9f, 0x51, 0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99};

  const uint8_t msg[] = "P-256 OpenSSL verification test";
  fio_u256 msg_hash = fio_sha256(msg, sizeof(msg) - 1);
  uint8_t sig[72];
  size_t sig_len = 0;

  FIO_ASSERT(fio_ecdsa_p256_sign(sig, &sig_len, sizeof(sig), msg_hash.u8,
                                 secret_key) == 0,
             "P-256 signing for OpenSSL cross-check failed");
  FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, msg_hash.u8, public_key,
                                   sizeof(public_key)) == 0,
             "P-256 self verification for OpenSSL cross-check failed");

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
  FIO_ASSERT(key, "OpenSSL EC_KEY_new_by_curve_name failed");
  const EC_GROUP *group = EC_KEY_get0_group(key);
  BIGNUM *x = BN_bin2bn(public_key + 1, 32, NULL);
  BIGNUM *y = BN_bin2bn(public_key + 33, 32, NULL);
  EC_POINT *point = EC_POINT_new(group);
  FIO_ASSERT(x && y && point, "OpenSSL point allocation failed");
  FIO_ASSERT(EC_POINT_set_affine_coordinates(group, point, x, y, NULL) == 1,
             "OpenSSL public point setup failed");
  FIO_ASSERT(EC_KEY_set_public_key(key, point) == 1,
             "OpenSSL public key setup failed");
  FIO_ASSERT(ECDSA_verify(0, msg_hash.u8, 32, sig, (int)sig_len, key) == 1,
             "OpenSSL should verify generated P-256 ECDSA signature");

  EC_POINT_free(point);
  BN_free(x);
  BN_free(y);
  EC_KEY_free(key);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}
#endif /* HAVE_OPENSSL */

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  FIO_NAME_TEST(stl, p256_constants)();
  FIO_NAME_TEST(stl, p256_field_ops)();
  FIO_NAME_TEST(stl, p256_base_point)();
  FIO_NAME_TEST(stl, p256_scalar_mul)();
  FIO_NAME_TEST(stl, p256_ecdsa_nist)();
  FIO_NAME_TEST(stl, p256_der_parsing)();
  FIO_NAME_TEST(stl, p256_invalid_sigs)();
  FIO_NAME_TEST(stl, p256_ecdhe)();
  FIO_NAME_TEST(stl, p256_ecdhe_invalid_inputs)();
  FIO_NAME_TEST(stl, p256_ecdhe_compressed)();
  FIO_NAME_TEST(stl, p256_sign_verify_roundtrip)();
#if HAVE_OPENSSL
  FIO_NAME_TEST(stl, p256_openssl_verify)();
#endif
  return 0;
}
