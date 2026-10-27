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

#define FIO_MATH
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

/* Barrett scalar reduction must match generic division for all t < 2^512. */
FIO_SFUNC void FIO_NAME_TEST(stl, p256_scalar_reduce)(void) {
  const uint64_t n_ext[8] = {FIO___P256_N[0],
                             FIO___P256_N[1],
                             FIO___P256_N[2],
                             FIO___P256_N[3],
                             0,
                             0,
                             0,
                             0};
  for (size_t round = 0; round < 4096; ++round) {
    uint64_t t[8], expect[8];
    fio___p256_scalar_s got;
    for (size_t i = 0; i < 8; ++i)
      t[i] = fio_rand64();
    switch (round) { /* edge cases first */
    case 0: FIO_MEMSET(t, 0, sizeof(t)); break;
    case 1: FIO_MEMSET(t, 0xFF, sizeof(t)); break;
    case 2:
      FIO_MEMSET(t, 0, sizeof(t));
      FIO_MEMCPY(t, FIO___P256_N, 32);
      break;
    case 3:
      FIO_MEMSET(t, 0, sizeof(t));
      FIO_MEMCPY(t, FIO___P256_N, 32);
      t[0] -= 1;
      break;
    case 4: /* (n-1)^2, the largest product of reduced scalars */
    {
      uint64_t nm1[4] = {FIO___P256_N[0] - 1,
                         FIO___P256_N[1],
                         FIO___P256_N[2],
                         FIO___P256_N[3]};
      fio_math_mul(t, nm1, nm1, 4);
      break;
    }
    case 5: FIO_MEMSET(t + 4, 0, 32); break; /* t < 2^256 */
    case 6: /* n * 2^256 */
      FIO_MEMSET(t, 0, sizeof(t));
      FIO_MEMCPY(t + 4, FIO___P256_N, 32);
      break;
    default:
      if (round & 1) /* sparse top limbs */
        t[7] &= 0xFF;
      break;
    }
    fio_math_div(NULL, expect, t, n_ext, 8);
    fio___p256_scalar_reduce(got, t);
    FIO_ASSERT(!memcmp(got, expect, sizeof(got)) &&
                   !(expect[4] | expect[5] | expect[6] | expect[7]),
               "P-256 Barrett scalar reduction mismatch (round %zu)",
               round);
  }
  { /* a * a^-1 == 1 */
    fio___p256_scalar_s a, a_inv, one;
    for (size_t i = 0; i < 4; ++i)
      a[i] = fio_rand64();
    fio___p256_scalar_sub_n_if(a, (uint64_t)fio___p256_scalar_ct_gte_n(a));
    a[0] |= 1;
    fio___p256_scalar_inv(a_inv, a);
    fio___p256_scalar_mul(one, a, a_inv);
    FIO_ASSERT(one[0] == 1 && !(one[1] | one[2] | one[3]),
               "P-256 scalar inversion failed");
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

  /* Not strict DER: r's MSB is 0xef but lacks the required leading zero, so
   * DER reads it as a negative INTEGER. */
  static const uint8_t der_sig_neg_r[] = {
      0x30, 0x45, 0x02, 0x20, 0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8,
      0xfd, 0x11, 0x40, 0xdd, 0x9c, 0xd4, 0x5e, 0x81, 0xd6, 0x9d, 0x2c,
      0x87, 0x7b, 0x56, 0xaa, 0xf9, 0x91, 0xc3, 0x4d, 0x0e, 0xa8, 0x4e,
      0xaf, 0x37, 0x16, 0x02, 0x21, 0x00, 0xf7, 0xcb, 0x1c, 0x94, 0x2d,
      0x65, 0x7c, 0x41, 0xd4, 0x36, 0xc7, 0xa1, 0xb6, 0xe2, 0x9f, 0x65,
      0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf, 0xf4, 0x06, 0x4d, 0xc4, 0xab,
      0x2f, 0x84, 0x3a, 0xcd, 0xa8};
  /* Strict DER: 30 46 | 02 21 00 r | 02 21 00 s */
  uint8_t der_sig[72] = {0x30, 0x46, 0x02, 0x21, 0x00};
  FIO_MEMCPY(der_sig + 5, der_sig_neg_r + 4, sizeof(der_sig_neg_r) - 4);

  FIO_ASSERT(fio_ecdsa_p256_verify(der_sig, sizeof(der_sig), msg_hash, pubkey,
                                   sizeof(pubkey)) == 0,
             "P-256 DER signature verification failed");
  FIO_ASSERT(fio_ecdsa_p256_verify(der_sig_neg_r, sizeof(der_sig_neg_r),
                                   msg_hash, pubkey, sizeof(pubkey)) != 0,
             "P-256 DER negative r should fail");
  { /* negative s: drop s's required leading zero */
    uint8_t tmp[71];
    FIO_MEMCPY(tmp, der_sig, 38);
    tmp[1] = 0x45;
    tmp[38] = 0x20;
    FIO_MEMCPY(tmp + 39, der_sig + 40, 32);
    FIO_ASSERT(fio_ecdsa_p256_verify(tmp, sizeof(tmp), msg_hash, pubkey,
                                     sizeof(pubkey)) != 0,
               "P-256 DER negative s should fail");
  }

  /* Every truncation must fail without reading past the buffer (ASan). */
  for (size_t len = 0; len < sizeof(der_sig); ++len) {
    uint8_t *tmp = (uint8_t *)malloc(len ? len : 1);
    FIO_ASSERT_ALLOC(tmp);
    FIO_MEMCPY(tmp, der_sig, len);
    FIO_ASSERT(fio_ecdsa_p256_verify(tmp, len, msg_hash, pubkey,
                                     sizeof(pubkey)) != 0,
               "P-256 truncated DER (%zu bytes) should fail",
               len);
    free(tmp);
  }
  { /* r consumes the whole sequence; s tag would be read past the end */
    static const uint8_t bad[8] = {0x30, 0x06, 0x02, 0x04, 1, 2, 3, 4};
    uint8_t *tmp = (uint8_t *)malloc(sizeof(bad));
    FIO_ASSERT_ALLOC(tmp);
    FIO_MEMCPY(tmp, bad, sizeof(bad));
    FIO_ASSERT(fio_ecdsa_p256_verify(tmp, sizeof(bad), msg_hash, pubkey,
                                     sizeof(pubkey)) != 0,
               "P-256 DER missing s should fail");
    free(tmp);
  }
  { /* parser-level strictness checks */
    static const uint8_t ok[] = {0x30, 0x06, 0x02, 0x01, 0x01,
                                 0x02, 0x01, 0x01, 0x00};
    static const uint8_t longform[] = {0x30, 0x81, 0x06, 0x02, 0x01,
                                       0x01, 0x02, 0x01, 0x01};
    static const uint8_t nonmin[] = {0x30, 0x07, 0x02, 0x02, 0x00,
                                     0x01, 0x02, 0x01, 0x01};
    static const uint8_t extra[] = {0x30, 0x09, 0x02, 0x01, 0x01, 0x02,
                                    0x01, 0x01, 0x02, 0x01, 0x01};
    uint8_t r[32], s[32];
    FIO_ASSERT(!fio___p256_parse_der_signature(r, s, ok, sizeof(ok) - 1) &&
                   r[31] == 1 && s[31] == 1,
               "P-256 DER minimal small integers should parse");
    FIO_ASSERT(fio___p256_parse_der_signature(r, s, ok, sizeof(ok)),
               "P-256 DER trailing data should fail");
    FIO_ASSERT(fio___p256_parse_der_signature(r, s, longform, sizeof(longform)),
               "P-256 DER long-form length should fail");
    FIO_ASSERT(fio___p256_parse_der_signature(r, s, nonmin, sizeof(nonmin)),
               "P-256 DER non-minimal integer should fail");
    FIO_ASSERT(fio___p256_parse_der_signature(r, s, extra, sizeof(extra)),
               "P-256 DER extra element should fail");
    { /* lenient-DER forms (accepted by der.h) stay rejected here */
      static const uint8_t int_longform[] = {0x30, 0x07, 0x02, 0x81, 0x01,
                                             0x01, 0x02, 0x01, 0x01};
      static const uint8_t indefinite[] = {0x30, 0x80, 0x02, 0x01, 0x01, 0x02,
                                           0x01, 0x01, 0x00, 0x00};
      FIO_ASSERT(fio___p256_parse_der_signature(r, s, int_longform,
                                               sizeof(int_longform)),
                 "P-256 DER long-form INTEGER length should fail");
      FIO_ASSERT(fio___p256_parse_der_signature(r, s, indefinite,
                                               sizeof(indefinite)),
                 "P-256 DER indefinite length should fail");
    }
  }

  { /* field coordinates must be canonical (< p) */
    fio___p256_fe_s fe;
    uint8_t b[32];
    fio___p256_fe_to_bytes(b, FIO___P256_P);
    FIO_ASSERT(fio___p256_fe_from_bytes_canonical(fe, b) != 0,
               "P-256 coordinate == p should fail");
    b[31] -= 1;
    FIO_ASSERT(fio___p256_fe_from_bytes_canonical(fe, b) == 0,
               "P-256 coordinate p-1 should pass");
    b[19] = 0x01; /* p + 1 (p ends with 0x00000000_FFFFFFFF_FFFFFFFF...) */
    FIO_MEMSET(b + 20, 0, 12);
    FIO_ASSERT(fio___p256_fe_from_bytes_canonical(fe, b) != 0,
               "P-256 coordinate p+1 should fail");
    FIO_MEMSET(b, 0xFF, 32);
    FIO_ASSERT(fio___p256_fe_from_bytes_canonical(fe, b) != 0,
               "P-256 coordinate 2^256-1 should fail");
  }
}

/* *****************************************************************************
P-256 Point at Infinity Handling (Q = -G, so G + Q = infinity)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_infinity)(void) {
  fio___p256_point_affine_s g, q;
  fio___p256_fe_copy(g.x, FIO___P256_GX);
  fio___p256_fe_copy(g.y, FIO___P256_GY);
  fio___p256_fe_copy(q.x, g.x);
  fio___p256_fe_neg(q.y, g.y);
  uint8_t qx[32], qy[32], r[32], s[32] = {0}, e[32];
  fio___p256_fe_to_bytes(qx, q.x);
  fio___p256_fe_to_bytes(qy, q.y);
  s[31] = 1;

  { /* valid signature for Q = -G: with s = 1, R = e*G - r*G = (e - r)*G.
     * Pick R = k*G with k = 2^16, r = R.x, e = r + k: verification must
     * succeed. e and r share their low 16 bits, so Shamir steps that need
     * G + Q (infinity) occur after steps that differ (r is a real point). */
    fio___p256_scalar_s k = {0x10000};
    fio___p256_point_jacobian_s rj;
    fio___p256_point_affine_s ra;
    fio___p256_point_mul(&rj, k, &g);
    fio___p256_point_to_affine(&ra, &rj);
    fio___p256_fe_to_bytes(r, ra.x);
    FIO_MEMCPY(e, r, 32);
    unsigned carry = 1;
    for (size_t i = 30; i-- && carry;) {
      carry += e[i];
      e[i] = (uint8_t)carry;
      carry >>= 8;
    }
    FIO_ASSERT(!carry && r[0] != 0xFF && e[0] != 0xFF &&
                   (r[31] | r[30]),
               "P-256 infinity test vector setup out of range");
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, s, e, qx, qy) == 0,
               "P-256 valid signature with Q = -G should verify");
    e[31] ^= 1;
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, s, e, qx, qy) != 0,
               "P-256 modified hash with Q = -G should fail");
  }
  { /* r == e, s = 1: u1 == u2, so u1*G + u2*Q = infinity -> reject */
    FIO_MEMCPY(e, r, 32);
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, s, e, qx, qy) != 0,
               "P-256 signature whose sum is infinity should fail");
  }
  { /* infinity / off-curve public key: affine (0,0) is not on the curve */
    uint8_t zero[32] = {0};
    FIO_ASSERT(fio_ecdsa_p256_verify_raw(r, s, e, zero, zero) != 0,
               "P-256 public key (0,0) should fail");
  }
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
P-256 Public Key Derivation
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256_public_key)(void) {
  /* RFC 6979 A.2.5 key pair */
  static const uint8_t sk[32] = {
      0xc9, 0xaf, 0xa9, 0xd8, 0x45, 0xba, 0x75, 0x16, 0x6b, 0x5c, 0x21,
      0x57, 0x67, 0xb1, 0xd6, 0x93, 0x4e, 0x50, 0xc3, 0xdb, 0x36, 0xe8,
      0x9b, 0x12, 0x7b, 0x8a, 0x62, 0x2b, 0x12, 0x0f, 0x67, 0x21};
  static const uint8_t pk[65] = {
      0x04, 0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61,
      0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b,
      0x61, 0xfa, 0x6c, 0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
      0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99, 0xa4, 0x1a, 0xe9,
      0xe9, 0x56, 0x28, 0xbc, 0x64, 0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e,
      0x9f, 0x51, 0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99};
  uint8_t out[65];
  FIO_ASSERT(!fio_p256_public_key(out, sk) && !FIO_MEMCMP(out, pk, 65),
             "P-256 public key from RFC 6979 secret key mismatch");

  /* derived public key matches keypair output */
  for (int i = 0; i < 4; ++i) {
    uint8_t ksk[32], kpk[65];
    FIO_ASSERT(!fio_p256_keypair(ksk, kpk), "P-256 keypair failed");
    FIO_ASSERT(!fio_p256_public_key(out, ksk) && !FIO_MEMCMP(out, kpk, 65),
               "P-256 public key should match keypair output");
  }

  /* invalid scalars: 0, n, n+1, 2^256-1; output is zeroed */
  uint8_t bad[32];
  FIO_MEMSET(bad, 0, 32);
  FIO_MEMSET(out, 0xAA, 65);
  FIO_ASSERT(fio_p256_public_key(out, bad) == -1 && !out[0] && !out[64],
             "P-256 public key should reject zero scalar");
  fio___p256_fe_to_bytes(bad, FIO___P256_N);
  FIO_ASSERT(fio_p256_public_key(out, bad) == -1,
             "P-256 public key should reject n");
  bad[31] += 1;
  FIO_ASSERT(fio_p256_public_key(out, bad) == -1,
             "P-256 public key should reject n+1");
  bad[31] -= 2; /* n - 1 is the largest valid scalar */
  FIO_ASSERT(fio_p256_public_key(out, bad) == 0 && out[0] == 0x04,
             "P-256 public key should accept n-1");
  FIO_MEMSET(bad, 0xFF, 32);
  FIO_ASSERT(fio_p256_public_key(out, bad) == -1,
             "P-256 public key should reject 2^256-1");
  FIO_ASSERT(fio_p256_public_key(out, NULL) == -1 && !out[0],
             "P-256 public key should reject NULL secret");
  FIO_ASSERT(fio_p256_public_key(NULL, sk) == -1,
             "P-256 public key should reject NULL output");
}

/* *****************************************************************************
P-256 ECDSA Verify With Compressed Public Keys (SEC 1: 02/03 || x)
***************************************************************************** */

/** Compressed form (33 bytes) of an uncompressed public key (65 bytes). */
FIO_SFUNC void fio___p256_test_compress(uint8_t out[33], const uint8_t pk[65]) {
  out[0] = (uint8_t)(0x02 | (pk[64] & 1));
  FIO_MEMCPY(out + 1, pk + 1, 32);
}

/** Big-endian 256-bit `x = a + k` (no overflow expected). */
FIO_SFUNC void fio___p256_test_x(uint8_t x[32],
                                 const uint64_t a[4],
                                 uint32_t k) {
  fio___p256_fe_to_bytes(x, a);
  for (size_t i = 32; i-- && k;) {
    k += x[i];
    x[i] = (uint8_t)k;
    k >>= 8;
  }
  FIO_ASSERT(!k, "P-256 test x overflow");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p256_compressed_verify)(void) {
  fio___p256_point_affine_s pt;
  uint8_t c[33], pk[65], sk[32], sig[72], buf[66];
  size_t sig_len = 0;
  const uint64_t zero[4] = {0};
  const uint8_t msg[] = "compressed P-256 public key";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);

  { /* decompression recovers G (prefix of G's parity) and -G (the other) */
    fio___p256_fe_s ny;
    pk[0] = 0x04;
    fio___p256_fe_to_bytes(pk + 1, FIO___P256_GX);
    fio___p256_fe_to_bytes(pk + 33, FIO___P256_GY);
    fio___p256_test_compress(c, pk);
    FIO_ASSERT(!fio___p256_point_decompress(&pt, c) &&
                   !fio___p256_fe_eq(pt.x, FIO___P256_GX) &&
                   !fio___p256_fe_eq(pt.y, FIO___P256_GY),
               "P-256 decompress(G) failed");
    c[0] ^= 1;
    fio___p256_fe_neg(ny, FIO___P256_GY);
    FIO_ASSERT(!fio___p256_point_decompress(&pt, c) &&
                   !fio___p256_fe_eq(pt.x, FIO___P256_GX) &&
                   !fio___p256_fe_eq(pt.y, ny),
               "P-256 decompress(-G) failed");
  }

  for (int i = 0; i < 16; ++i) {
    FIO_ASSERT(!fio_p256_keypair(sk, pk), "P-256 keypair failed");
    fio___p256_test_compress(c, pk);
    /* decompression round trip */
    FIO_ASSERT(!fio___p256_point_decompress(&pt, c), "P-256 decompress failed");
    fio___p256_fe_to_bytes(buf, pt.x);
    fio___p256_fe_to_bytes(buf + 32, pt.y);
    FIO_ASSERT(!FIO_MEMCMP(buf, pk + 1, 64),
               "P-256 decompressed point should equal the uncompressed key");
    /* sign / verify with both encodings */
    FIO_ASSERT(!fio_ecdsa_p256_sign(sig, &sig_len, sizeof(sig), h.u8, sk),
               "P-256 sign failed");
    FIO_ASSERT(!fio_ecdsa_p256_verify(sig, sig_len, h.u8, c, 33),
               "P-256 verify should accept a compressed public key");
    FIO_ASSERT(!fio_ecdsa_p256_verify(sig, sig_len, h.u8, pk, 65),
               "P-256 verify should accept an uncompressed public key");
    /* tampered signature / hash */
    sig[sig_len - 1] ^= 1;
    FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, h.u8, c, 33),
               "P-256 tampered signature should fail (compressed key)");
    sig[sig_len - 1] ^= 1;
    h.u8[0] ^= 1;
    FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, h.u8, c, 33),
               "P-256 tampered hash should fail (compressed key)");
    h.u8[0] ^= 1;
    /* the other prefix is -Q: a different key */
    c[0] ^= 1;
    FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, h.u8, c, 33),
               "P-256 compressed key with the wrong y parity should fail");
    c[0] ^= 1;
    /* lengths that don't match the prefix */
    FIO_MEMCPY(buf, c, 33);
    buf[33] = 0;
    FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, h.u8, buf, 34) &&
                   fio_ecdsa_p256_verify(sig, sig_len, h.u8, c, 32) &&
                   fio_ecdsa_p256_verify(sig, sig_len, h.u8, pk, 33) &&
                   fio_ecdsa_p256_verify(sig, sig_len, h.u8, pk, 64),
               "P-256 public key length / prefix mismatch should fail");
  }

  /* invalid prefixes: identity (00), hybrid (06/07, RFC 5480), others */
  static const uint8_t bad_prefix[] =
      {0x00, 0x01, 0x04, 0x05, 0x06, 0x07, 0xFF};
  for (size_t i = 0; i < sizeof(bad_prefix); ++i) {
    FIO_MEMCPY(buf, c, 33);
    buf[0] = bad_prefix[i];
    FIO_ASSERT(fio___p256_point_decompress(&pt, buf) &&
                   fio_ecdsa_p256_verify(sig, sig_len, h.u8, buf, 33),
               "P-256 compressed prefix 0x%02X should fail",
               bad_prefix[i]);
  }
  FIO_MEMCPY(buf, pk, 65);
  buf[0] = (uint8_t)(0x06 | (pk[64] & 1)); /* hybrid form, otherwise valid */
  FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, h.u8, buf, 65),
             "P-256 hybrid public key (06/07 || x || y) should fail");
  FIO_MEMSET(buf, 0, sizeof(buf));
  FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, h.u8, buf, 1) &&
                 fio_ecdsa_p256_verify(sig, sig_len, h.u8, buf, 33) &&
                 fio_ecdsa_p256_verify(sig, sig_len, h.u8, buf, 65),
             "P-256 point at infinity / zero public keys should fail");

  { /* x without a curve point (y^2 not a square), and non-canonical x >= p */
    uint32_t on = 0, off = 0;
    for (uint32_t k = 1; k < 256 && (!on || !off); ++k) {
      buf[0] = 0x02;
      fio___p256_test_x(buf + 1, zero, k);
      if (fio___p256_point_decompress(&pt, buf)) {
        off = off ? off : k;
      } else {
        on = on ? on : k;
        FIO_ASSERT(!fio___p256_point_validate(&pt),
                   "P-256 decompressed point should be on the curve");
      }
    }
    FIO_ASSERT(on && off, "P-256 test x values not found");
    buf[0] = 0x02;
    fio___p256_test_x(buf + 1, zero, off);
    FIO_ASSERT(fio_ecdsa_p256_verify(sig, sig_len, h.u8, buf, 33),
               "P-256 compressed x with no curve point should fail");
    fio___p256_test_x(buf + 1, FIO___P256_P, on); /* x + p, x valid */
    FIO_ASSERT(fio___p256_point_decompress(&pt, buf),
               "P-256 non-canonical compressed x (x + p) should fail");
    fio___p256_test_x(buf + 1, FIO___P256_P, 0);
    FIO_ASSERT(fio___p256_point_decompress(&pt, buf),
               "P-256 compressed x == p should fail");
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
  { /* OpenSSL's compressed encoding: same bytes, verifies natively */
    uint8_t comp[33], ours[33];
    FIO_ASSERT(EC_POINT_point2oct(group,
                                  point,
                                  POINT_CONVERSION_COMPRESSED,
                                  comp,
                                  sizeof(comp),
                                  NULL) == 33,
               "OpenSSL compressed point encoding failed");
    fio___p256_test_compress(ours, public_key);
    FIO_ASSERT(!FIO_MEMCMP(comp, ours, 33),
               "P-256 compressed encoding should match OpenSSL's");
    FIO_ASSERT(!fio_ecdsa_p256_verify(sig, sig_len, msg_hash.u8, comp, 33),
               "P-256 verify should accept OpenSSL's compressed public key");
    /* an OpenSSL signature verifies natively with the compressed key */
    BIGNUM *d = BN_bin2bn(secret_key, 32, NULL);
    FIO_ASSERT(d && EC_KEY_set_private_key(key, d) == 1,
               "OpenSSL private key setup failed");
    unsigned char osig[80];
    unsigned int osig_len = sizeof(osig);
    FIO_ASSERT(ECDSA_sign(0, msg_hash.u8, 32, osig, &osig_len, key) == 1,
               "OpenSSL ECDSA_sign failed");
    FIO_ASSERT(!fio_ecdsa_p256_verify(osig, osig_len, msg_hash.u8, comp, 33),
               "P-256 should verify an OpenSSL signature (compressed key)");
    BN_clear_free(d);
  }

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
  FIO_NAME_TEST(stl, p256_scalar_reduce)();
  FIO_NAME_TEST(stl, p256_ecdsa_nist)();
  FIO_NAME_TEST(stl, p256_der_parsing)();
  FIO_NAME_TEST(stl, p256_infinity)();
  FIO_NAME_TEST(stl, p256_invalid_sigs)();
  FIO_NAME_TEST(stl, p256_public_key)();
  FIO_NAME_TEST(stl, p256_compressed_verify)();
  FIO_NAME_TEST(stl, p256_ecdhe)();
  FIO_NAME_TEST(stl, p256_ecdhe_invalid_inputs)();
  FIO_NAME_TEST(stl, p256_ecdhe_compressed)();
  FIO_NAME_TEST(stl, p256_sign_verify_roundtrip)();
#if HAVE_OPENSSL
  FIO_NAME_TEST(stl, p256_openssl_verify)();
#endif
  return 0;
}
