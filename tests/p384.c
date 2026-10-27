/* *****************************************************************************
Test for 154 p384.h

Coverage: P-384 (secp384r1) field arithmetic, base point validation, scalar
multiplication, and ECDSA verify using RFC 6979 / NIST test vectors, plus DER
signature parsing, invalid signature rejection, and compressed public keys
(SEC 1 point decompression, cross-checked against OpenSSL when available).
Signing: constant-time field reduction / Barrett scalar reduction vs long
division references, the constant-time ladder vs variable-time multiplication
(random and edge scalars), public key derivation vectors, sign/verify round
trips, invalid key rejection, and OpenSSL sign/verify/derivation cross-checks.
Performance loops are intentionally omitted.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_MATH
#define FIO_P384
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
P-384 Curve Constants
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384_constants)(void) {
  FIO_ASSERT(FIO___P384_P[0] == 0x00000000FFFFFFFFULL, "P-384 P[0] incorrect");
  FIO_ASSERT(FIO___P384_P[1] == 0xFFFFFFFF00000000ULL, "P-384 P[1] incorrect");
  FIO_ASSERT(FIO___P384_P[2] == 0xFFFFFFFFFFFFFFFEULL, "P-384 P[2] incorrect");
  FIO_ASSERT(FIO___P384_P[3] == 0xFFFFFFFFFFFFFFFFULL, "P-384 P[3] incorrect");
  FIO_ASSERT(FIO___P384_P[4] == 0xFFFFFFFFFFFFFFFFULL, "P-384 P[4] incorrect");
  FIO_ASSERT(FIO___P384_P[5] == 0xFFFFFFFFFFFFFFFFULL, "P-384 P[5] incorrect");

  FIO_ASSERT(FIO___P384_N[0] == 0xECEC196ACCC52973ULL, "P-384 N[0] incorrect");
  FIO_ASSERT(FIO___P384_N[1] == 0x581A0DB248B0A77AULL, "P-384 N[1] incorrect");
  FIO_ASSERT(FIO___P384_N[2] == 0xC7634D81F4372DDFULL, "P-384 N[2] incorrect");
  FIO_ASSERT(FIO___P384_N[3] == 0xFFFFFFFFFFFFFFFFULL, "P-384 N[3] incorrect");
  FIO_ASSERT(FIO___P384_N[4] == 0xFFFFFFFFFFFFFFFFULL, "P-384 N[4] incorrect");
  FIO_ASSERT(FIO___P384_N[5] == 0xFFFFFFFFFFFFFFFFULL, "P-384 N[5] incorrect");
}

/* *****************************************************************************
P-384 Field Arithmetic
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384_field_ops)(void) {
  fio___p384_fe_s a, b, c;

  /* 1 + 2 = 3 */
  fio___p384_fe_one(a);
  b[0] = 2;
  b[1] = b[2] = b[3] = b[4] = b[5] = 0;
  fio___p384_fe_add(c, a, b);
  FIO_ASSERT(c[0] == 3 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "P-384 field addition 1+2 failed");

  /* 5 - 3 = 2 */
  a[0] = 5;
  a[1] = a[2] = a[3] = a[4] = a[5] = 0;
  b[0] = 3;
  b[1] = b[2] = b[3] = b[4] = b[5] = 0;
  fio___p384_fe_sub(c, a, b);
  FIO_ASSERT(c[0] == 2 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "P-384 field subtraction 5-3 failed");

  /* 2 * 3 = 6 */
  a[0] = 2;
  b[0] = 3;
  fio___p384_fe_mul(c, a, b);
  FIO_ASSERT(c[0] == 6 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "P-384 field multiplication 2*3 failed");

  /* 4^2 = 16 */
  a[0] = 4;
  fio___p384_fe_sqr(c, a);
  FIO_ASSERT(c[0] == 16 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "P-384 field squaring 4^2 failed");

  /* 7 * 7^(-1) = 1 */
  a[0] = 7;
  a[1] = a[2] = a[3] = a[4] = a[5] = 0;
  fio___p384_fe_inv(b, a);
  fio___p384_fe_mul(c, a, b);
  FIO_ASSERT(c[0] == 1 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "P-384 field inversion 7 * 7^(-1) != 1");

  /* Gx * Gx^(-1) = 1 */
  fio___p384_fe_copy(a, FIO___P384_GX);
  fio___p384_fe_inv(b, a);
  fio___p384_fe_mul(c, a, b);
  FIO_ASSERT(c[0] == 1 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "P-384 field inversion Gx * Gx^(-1) != 1");
}

/* *****************************************************************************
P-384 Base Point and Scalar Multiplication
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384_base_point)(void) {
  fio___p384_fe_s y2, x3, t;

  fio___p384_fe_sqr(y2, FIO___P384_GY);
  fio___p384_fe_sqr(t, FIO___P384_GX);
  fio___p384_fe_mul(x3, t, FIO___P384_GX);
  fio___p384_fe_sub(t, x3, FIO___P384_GX);
  fio___p384_fe_sub(t, t, FIO___P384_GX);
  fio___p384_fe_sub(t, t, FIO___P384_GX);
  fio___p384_fe_add(t, t, FIO___P384_B);

  FIO_ASSERT(fio___p384_fe_eq(y2, t) == 0,
             "P-384 base point G is not on the curve");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p384_scalar_mul)(void) {
  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  /* 1 * G = G */
  {
    fio___p384_scalar_s one = {1, 0, 0, 0, 0, 0};
    fio___p384_point_jacobian_s r;
    fio___p384_point_mul(&r, one, &g);
    fio___p384_point_affine_s r_aff;
    fio___p384_point_to_affine(&r_aff, &r);
    FIO_ASSERT(fio___p384_fe_eq(r_aff.x, FIO___P384_GX) == 0,
               "P-384 1*G x-coordinate mismatch");
    FIO_ASSERT(fio___p384_fe_eq(r_aff.y, FIO___P384_GY) == 0,
               "P-384 1*G y-coordinate mismatch");
  }

  /* 2 * G is on the curve. */
  {
    fio___p384_scalar_s two = {2, 0, 0, 0, 0, 0};
    fio___p384_point_jacobian_s r;
    fio___p384_point_mul(&r, two, &g);
    fio___p384_point_affine_s r_aff;
    fio___p384_point_to_affine(&r_aff, &r);

    fio___p384_fe_s y2, x3, t;
    fio___p384_fe_sqr(y2, r_aff.y);
    fio___p384_fe_sqr(t, r_aff.x);
    fio___p384_fe_mul(x3, t, r_aff.x);
    fio___p384_fe_sub(t, x3, r_aff.x);
    fio___p384_fe_sub(t, t, r_aff.x);
    fio___p384_fe_sub(t, t, r_aff.x);
    fio___p384_fe_add(t, t, FIO___P384_B);
    FIO_ASSERT(fio___p384_fe_eq(y2, t) == 0, "P-384 2*G is not on the curve");
  }
}

/* *****************************************************************************
P-384 ECDSA RFC 6979 / NIST Test Vector (shared)
***************************************************************************** */

/* SHA-384("sample") */
static const uint8_t nist_hash[48] = {
    0x9a, 0x90, 0x83, 0x50, 0x5b, 0xc9, 0x22, 0x76, 0xae, 0xc4, 0xbe, 0x31,
    0x26, 0x96, 0xef, 0x7b, 0xf3, 0xbf, 0x60, 0x3f, 0x4b, 0xbd, 0x38, 0x11,
    0x96, 0xa0, 0x29, 0xf3, 0x40, 0x58, 0x53, 0x12, 0x31, 0x3b, 0xca, 0x4a,
    0x9b, 0x5b, 0x89, 0x0e, 0xfe, 0xe4, 0x2c, 0x77, 0xb1, 0xee, 0x25, 0xfe};
/* RFC 6979 A.2.6 public key (0x04 || x || y) */
static const uint8_t nist_pub[97] = {
    0x04, 0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4, 0x56, 0x86,
    0x18, 0x02, 0x9f, 0x42, 0x7f, 0xa5, 0xda, 0x9a, 0x8b, 0xc4, 0xae,
    0x92, 0xe0, 0x2e, 0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64,
    0xde, 0xf8, 0xf0, 0xea, 0x90, 0x55, 0x86, 0x60, 0x64, 0xa2, 0x54,
    0x51, 0x54, 0x80, 0xbc, 0x13, 0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d,
    0x57, 0x24, 0x4e, 0xa8, 0xef, 0x9a, 0xc0, 0xc6, 0x21, 0x89, 0x67,
    0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9, 0xf5, 0x4c, 0xa8, 0x4b,
    0x3f, 0x1c, 0x9d, 0xb1, 0x28, 0x8b, 0x23, 0x1c, 0x3a, 0xe0, 0xd4,
    0xfe, 0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20};
/* Strict DER: 30 66 | 02 31 00 r | 02 31 00 s */
static const uint8_t nist_der[] = {
    0x30, 0x66, 0x02, 0x31, 0x00, 0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8,
    0xaa, 0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b, 0x3f, 0x88, 0x14,
    0x06, 0x66, 0xce, 0x9f, 0xa7, 0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13,
    0x3c, 0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9, 0x6e, 0x36, 0xdd,
    0x1e, 0x80, 0xfa, 0xbe, 0x46, 0x02, 0x31, 0x00, 0x99, 0xef, 0x4a, 0xeb,
    0x15, 0xf1, 0x78, 0xce, 0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26, 0x20, 0x3b, 0x63, 0x51,
    0xd0, 0xa3, 0xa9, 0x4f, 0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8};

/* *****************************************************************************
P-384 ECDSA RFC 6979 / NIST Test Vector
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384_ecdsa_nist)(void) {
  static const uint8_t r[48] = {
      0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa, 0xd4, 0x73, 0x6e,
      0x56, 0xc6, 0x91, 0x91, 0x6b, 0x3f, 0x88, 0x14, 0x06, 0x66, 0xce,
      0x9f, 0xa7, 0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c, 0x81,
      0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9, 0x6e, 0x36, 0xdd, 0x1e,
      0x80, 0xfa, 0xbe, 0x46};
  static const uint8_t s[48] = {
      0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce, 0xa1, 0xfe, 0x40,
      0xdb, 0x26, 0x03, 0x13, 0x8f, 0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62,
      0x45, 0x26, 0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f, 0xa3,
      0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e, 0x7b, 0x82, 0xc7, 0x1a,
      0x38, 0x62, 0x8a, 0xc8};

  FIO_ASSERT(
      fio_ecdsa_p384_verify_raw(r, s, nist_hash, nist_pub + 1, nist_pub + 49) ==
          0,
      "P-384 ECDSA NIST/RFC 6979 test vector failed");
}

/* *****************************************************************************
P-384 DER Signature Parsing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384_der_parsing)(void) {
  FIO_ASSERT(fio_ecdsa_p384_verify(nist_der,
                                   sizeof(nist_der),
                                   nist_hash,
                                   nist_pub,
                                   sizeof(nist_pub)) == 0,
             "P-384 DER signature verification failed");

  { /* negative r: drop r's required leading zero (r MSB is 0x94) */
    uint8_t tmp[sizeof(nist_der) - 1];
    tmp[0] = 0x30;
    tmp[1] = nist_der[1] - 1;
    tmp[2] = 0x02;
    tmp[3] = 0x30;
    FIO_MEMCPY(tmp + 4, nist_der + 5, sizeof(nist_der) - 5);
    FIO_ASSERT(fio_ecdsa_p384_verify(tmp,
                                     sizeof(tmp),
                                     nist_hash,
                                     nist_pub,
                                     sizeof(nist_pub)) != 0,
               "P-384 DER negative r should fail");
  }
  { /* negative s: drop s's required leading zero (s MSB is 0x99) */
    uint8_t tmp[sizeof(nist_der) - 1];
    FIO_MEMCPY(tmp, nist_der, 53);
    tmp[1] = nist_der[1] - 1;
    tmp[54] = 0x30;
    FIO_MEMCPY(tmp + 55, nist_der + 56, 48);
    FIO_ASSERT(fio_ecdsa_p384_verify(tmp,
                                     sizeof(tmp),
                                     nist_hash,
                                     nist_pub,
                                     sizeof(nist_pub)) != 0,
               "P-384 DER negative s should fail");
  }
  { /* redundant extra leading zero on s */
    uint8_t tmp[sizeof(nist_der) + 1];
    FIO_MEMCPY(tmp, nist_der, 56);
    tmp[1] = nist_der[1] + 1;
    tmp[54] = 0x32;
    tmp[56] = 0x00;
    FIO_MEMCPY(tmp + 57, nist_der + 56, 48);
    FIO_ASSERT(fio_ecdsa_p384_verify(tmp,
                                     sizeof(tmp),
                                     nist_hash,
                                     nist_pub,
                                     sizeof(nist_pub)) != 0,
               "P-384 DER over-long s should fail");
  }
  /* Every truncation must fail without reading past the buffer (ASan). */
  for (size_t len = 0; len < sizeof(nist_der); ++len) {
    uint8_t *tmp = (uint8_t *)malloc(len ? len : 1);
    FIO_ASSERT_ALLOC(tmp);
    FIO_MEMCPY(tmp, nist_der, len);
    FIO_ASSERT(fio_ecdsa_p384_verify(tmp,
                                     len,
                                     nist_hash,
                                     nist_pub,
                                     sizeof(nist_pub)) != 0,
               "P-384 truncated DER (%zu bytes) should fail",
               len);
    free(tmp);
  }
  { /* r consumes the whole sequence; s tag would be read past the end */
    static const uint8_t bad[8] = {0x30, 0x06, 0x02, 0x04, 1, 2, 3, 4};
    uint8_t *tmp = (uint8_t *)malloc(sizeof(bad));
    FIO_ASSERT_ALLOC(tmp);
    FIO_MEMCPY(tmp, bad, sizeof(bad));
    FIO_ASSERT(fio_ecdsa_p384_verify(tmp,
                                     sizeof(bad),
                                     nist_hash,
                                     nist_pub,
                                     sizeof(nist_pub)) != 0,
               "P-384 DER missing s should fail");
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
    static const uint8_t zero_len[] = {0x30, 0x06, 0x02, 0x00, 0x02,
                                       0x02, 0x01, 0x01};
    uint8_t r[48], s[48];
    FIO_ASSERT(!fio___p384_parse_der_signature(r, s, ok, sizeof(ok) - 1) &&
                   r[47] == 1 && s[47] == 1,
               "P-384 DER minimal small integers should parse");
    FIO_ASSERT(fio___p384_parse_der_signature(r, s, ok, sizeof(ok)),
               "P-384 DER trailing data should fail");
    FIO_ASSERT(fio___p384_parse_der_signature(r, s, longform, sizeof(longform)),
               "P-384 DER long-form length should fail");
    FIO_ASSERT(fio___p384_parse_der_signature(r, s, nonmin, sizeof(nonmin)),
               "P-384 DER non-minimal integer should fail");
    FIO_ASSERT(fio___p384_parse_der_signature(r, s, extra, sizeof(extra)),
               "P-384 DER extra element should fail");
    { /* lenient-DER forms (accepted by der.h) stay rejected here */
      static const uint8_t int_longform[] = {0x30, 0x07, 0x02, 0x81, 0x01,
                                             0x01, 0x02, 0x01, 0x01};
      static const uint8_t indefinite[] = {0x30, 0x80, 0x02, 0x01, 0x01, 0x02,
                                           0x01, 0x01, 0x00, 0x00};
      FIO_ASSERT(fio___p384_parse_der_signature(r, s, int_longform,
                                               sizeof(int_longform)),
                 "P-384 DER long-form INTEGER length should fail");
      FIO_ASSERT(fio___p384_parse_der_signature(r, s, indefinite,
                                               sizeof(indefinite)),
                 "P-384 DER indefinite length should fail");
    }
    FIO_ASSERT(fio___p384_parse_der_signature(r, s, zero_len, sizeof(zero_len)),
               "P-384 DER zero-length integer should fail");
  }

  { /* field coordinates must be canonical (< p) */
    fio___p384_fe_s fe;
    uint8_t b[48];
    fio___p384_fe_to_bytes(b, FIO___P384_P);
    FIO_ASSERT(fio___p384_fe_from_bytes_canonical(fe, b) != 0,
               "P-384 coordinate == p should fail");
    b[47] -= 1;
    FIO_ASSERT(fio___p384_fe_from_bytes_canonical(fe, b) == 0,
               "P-384 coordinate p-1 should pass");
    b[47] += 2; /* p ends with ...00000000_FFFFFFFF: p+1 carries */
    b[46] = b[45] = b[44] = 0;
    b[43] = 0x01;
    FIO_ASSERT(fio___p384_fe_from_bytes_canonical(fe, b) != 0,
               "P-384 coordinate p+1 should fail");
    FIO_MEMSET(b, 0xFF, 48);
    FIO_ASSERT(fio___p384_fe_from_bytes_canonical(fe, b) != 0,
               "P-384 coordinate 2^384-1 should fail");
  }
}

/* *****************************************************************************
P-384 Point at Infinity Handling (Q = -G, so G + Q = infinity)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384_infinity)(void) {
  fio___p384_point_affine_s g, q;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);
  fio___p384_fe_copy(q.x, g.x);
  fio___p384_fe_neg(q.y, g.y);
  uint8_t qx[48], qy[48], r[48], s[48] = {0}, e[48];
  fio___p384_fe_to_bytes(qx, q.x);
  fio___p384_fe_to_bytes(qy, q.y);
  s[47] = 1;

  { /* valid signature for Q = -G: with s = 1, R = e*G - r*G = (e - r)*G.
     * Pick R = k*G with k = 2^16, r = R.x, e = r + k: verification must
     * succeed. e and r share their low 16 bits, so Shamir steps that need
     * G + Q (infinity) occur after steps that differ (r is a real point). */
    fio___p384_scalar_s k = {0x10000};
    fio___p384_point_jacobian_s rj;
    fio___p384_point_affine_s ra;
    fio___p384_point_mul(&rj, k, &g);
    fio___p384_point_to_affine(&ra, &rj);
    fio___p384_fe_to_bytes(r, ra.x);
    FIO_MEMCPY(e, r, 48);
    unsigned carry = 1;
    for (size_t i = 46; i-- && carry;) {
      carry += e[i];
      e[i] = (uint8_t)carry;
      carry >>= 8;
    }
    FIO_ASSERT(!carry && r[0] != 0xFF && e[0] != 0xFF &&
                   (r[47] | r[46]),
               "P-384 infinity test vector setup out of range");
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(r, s, e, qx, qy) == 0,
               "P-384 valid signature with Q = -G should verify");
    e[47] ^= 1;
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(r, s, e, qx, qy) != 0,
               "P-384 modified hash with Q = -G should fail");
  }
  { /* r == e, s = 1: u1 == u2, so u1*G + u2*Q = infinity -> reject */
    FIO_MEMCPY(e, r, 48);
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(r, s, e, qx, qy) != 0,
               "P-384 signature whose sum is infinity should fail");
  }
  { /* infinity / off-curve public key: affine (0,0) is not on the curve */
    uint8_t zero[48] = {0};
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(r, s, e, zero, zero) != 0,
               "P-384 public key (0,0) should fail");
  }
}

/* *****************************************************************************
P-384 Invalid Signature Rejection
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384_invalid_sigs)(void) {
  static const uint8_t r[48] = {
      0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa, 0xd4, 0x73, 0x6e,
      0x56, 0xc6, 0x91, 0x91, 0x6b, 0x3f, 0x88, 0x14, 0x06, 0x66, 0xce,
      0x9f, 0xa7, 0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c, 0x81,
      0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9, 0x6e, 0x36, 0xdd, 0x1e,
      0x80, 0xfa, 0xbe, 0x46};
  static const uint8_t s[48] = {
      0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce, 0xa1, 0xfe, 0x40,
      0xdb, 0x26, 0x03, 0x13, 0x8f, 0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62,
      0x45, 0x26, 0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f, 0xa3,
      0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e, 0x7b, 0x82, 0xc7, 0x1a,
      0x38, 0x62, 0x8a, 0xc8};

  FIO_ASSERT(
      fio_ecdsa_p384_verify_raw(r, s, nist_hash, nist_pub + 1, nist_pub + 49) ==
          0,
      "P-384 valid signature should verify");

  /* Modified r. */
  {
    uint8_t bad_r[48];
    FIO_MEMCPY(bad_r, r, 48);
    bad_r[0] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(bad_r,
                                         s,
                                         nist_hash,
                                         nist_pub + 1,
                                         nist_pub + 49) != 0,
               "P-384 should reject modified r");
  }

  /* Modified s. */
  {
    uint8_t bad_s[48];
    FIO_MEMCPY(bad_s, s, 48);
    bad_s[0] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(r,
                                         bad_s,
                                         nist_hash,
                                         nist_pub + 1,
                                         nist_pub + 49) != 0,
               "P-384 should reject modified s");
  }

  /* Modified message hash. */
  {
    uint8_t bad_hash[48];
    FIO_MEMCPY(bad_hash, nist_hash, 48);
    bad_hash[0] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(r,
                                         s,
                                         bad_hash,
                                         nist_pub + 1,
                                         nist_pub + 49) != 0,
               "P-384 should reject modified message hash");
  }

  /* r = 0 or s = 0. */
  {
    uint8_t zero[48] = {0};
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(zero,
                                         s,
                                         nist_hash,
                                         nist_pub + 1,
                                         nist_pub + 49) != 0,
               "P-384 should reject r = 0");
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(r,
                                         zero,
                                         nist_hash,
                                         nist_pub + 1,
                                         nist_pub + 49) != 0,
               "P-384 should reject s = 0");
  }

  /* r >= n or s >= n. */
  {
    uint8_t n_bytes[48] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xC7, 0x63, 0x4D, 0x81, 0xF4, 0x37, 0x2D, 0xDF, 0x58,
        0x1A, 0x0D, 0xB2, 0x48, 0xB0, 0xA7, 0x7A, 0xEC, 0xEC, 0x19, 0x6A,
        0xCC, 0xC5, 0x29, 0x73};
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(n_bytes,
                                         s,
                                         nist_hash,
                                         nist_pub + 1,
                                         nist_pub + 49) != 0,
               "P-384 should reject r = n");
    FIO_ASSERT(fio_ecdsa_p384_verify_raw(r,
                                         n_bytes,
                                         nist_hash,
                                         nist_pub + 1,
                                         nist_pub + 49) != 0,
               "P-384 should reject s = n");
  }
}

/* *****************************************************************************
P-384 ECDSA Verify With Compressed Public Keys (SEC 1: 02/03 || x)
***************************************************************************** */

/** Compressed form (49 bytes) of an uncompressed public key (97 bytes). */
FIO_SFUNC void fio___p384_test_compress(uint8_t out[49], const uint8_t pk[97]) {
  out[0] = (uint8_t)(0x02 | (pk[96] & 1));
  FIO_MEMCPY(out + 1, pk + 1, 48);
}

/** Big-endian 384-bit `x = a + k` (no overflow expected). */
FIO_SFUNC void fio___p384_test_x(uint8_t x[48],
                                 const uint64_t a[6],
                                 uint32_t k) {
  fio___p384_fe_to_bytes(x, a);
  for (size_t i = 48; i-- && k;) {
    k += x[i];
    x[i] = (uint8_t)k;
    k >>= 8;
  }
  FIO_ASSERT(!k, "P-384 test x overflow");
}

/** Returns 0 if the affine point satisfies y^2 = x^3 - 3x + b. */
FIO_SFUNC int fio___p384_test_on_curve(const fio___p384_point_affine_s *p) {
  fio___p384_fe_s y2, t;
  fio___p384_fe_sqr(y2, p->y);
  fio___p384_fe_sqr(t, p->x);
  fio___p384_fe_mul(t, t, p->x);
  fio___p384_fe_sub(t, t, p->x);
  fio___p384_fe_sub(t, t, p->x);
  fio___p384_fe_sub(t, t, p->x);
  fio___p384_fe_add(t, t, FIO___P384_B);
  return fio___p384_fe_eq(y2, t);
}

FIO_SFUNC void FIO_NAME_TEST(stl, p384_compressed_verify)(void) {
  fio___p384_point_affine_s pt, g;
  uint8_t c[49], pk[97], buf[98], sig[sizeof(nist_der)];
  const uint64_t zero[6] = {0};
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  { /* decompression recovers G (prefix of G's parity) and -G (the other) */
    fio___p384_fe_s ny;
    pk[0] = 0x04;
    fio___p384_fe_to_bytes(pk + 1, FIO___P384_GX);
    fio___p384_fe_to_bytes(pk + 49, FIO___P384_GY);
    fio___p384_test_compress(c, pk);
    FIO_ASSERT(!fio___p384_point_decompress(&pt, c) &&
                   !fio___p384_fe_eq(pt.x, FIO___P384_GX) &&
                   !fio___p384_fe_eq(pt.y, FIO___P384_GY),
               "P-384 decompress(G) failed");
    c[0] ^= 1;
    fio___p384_fe_neg(ny, FIO___P384_GY);
    FIO_ASSERT(!fio___p384_point_decompress(&pt, c) &&
                   !fio___p384_fe_eq(pt.x, FIO___P384_GX) &&
                   !fio___p384_fe_eq(pt.y, ny),
               "P-384 decompress(-G) failed");
  }

  /* decompression round trip for k*G (both y parities are expected) */
  for (uint64_t i = 1, seen = 0; i <= 32; ++i) {
    fio___p384_scalar_s k = {i * 0x9E3779B97F4A7C15ULL,
                             i,
                             i * 0xD1B54A32D192ED03ULL,
                             0,
                             i << 20,
                             0x0123456789ABCDEFULL ^ i};
    fio___p384_point_jacobian_s rj;
    fio___p384_point_affine_s ra;
    fio___p384_point_mul(&rj, k, &g);
    fio___p384_point_to_affine(&ra, &rj);
    pk[0] = 0x04;
    fio___p384_fe_to_bytes(pk + 1, ra.x);
    fio___p384_fe_to_bytes(pk + 49, ra.y);
    fio___p384_test_compress(c, pk);
    seen |= (uint64_t)1 << (c[0] & 1);
    FIO_ASSERT(!fio___p384_point_decompress(&pt, c),
               "P-384 decompress(k*G) failed");
    fio___p384_fe_to_bytes(buf, pt.x);
    fio___p384_fe_to_bytes(buf + 48, pt.y);
    FIO_ASSERT(!FIO_MEMCMP(buf, pk + 1, 96),
               "P-384 decompressed point should equal the uncompressed key");
    FIO_ASSERT(i < 32 || seen == 3, "P-384 round trip missed a y parity");
  }

  /* RFC 6979 vector: verify with both encodings */
  FIO_MEMCPY(pk, nist_pub, 97);
  FIO_MEMCPY(sig, nist_der, sizeof(sig));
  fio___p384_test_compress(c, pk);
  FIO_ASSERT(!fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, c, 49),
             "P-384 verify should accept a compressed public key");
  FIO_ASSERT(!fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, pk, 97),
             "P-384 verify should accept an uncompressed public key");
  /* tampered signature / hash */
  sig[sizeof(sig) - 1] ^= 1;
  FIO_ASSERT(fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, c, 49),
             "P-384 tampered signature should fail (compressed key)");
  sig[sizeof(sig) - 1] ^= 1;
  FIO_MEMCPY(buf, nist_hash, 48);
  buf[0] ^= 1;
  FIO_ASSERT(fio_ecdsa_p384_verify(sig, sizeof(sig), buf, c, 49),
             "P-384 tampered hash should fail (compressed key)");
  /* the other prefix is -Q: a different key */
  c[0] ^= 1;
  FIO_ASSERT(fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, c, 49),
             "P-384 compressed key with the wrong y parity should fail");
  c[0] ^= 1;
  /* lengths that don't match the prefix */
  FIO_MEMCPY(buf, c, 49);
  buf[49] = 0;
  FIO_ASSERT(fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, buf, 50) &&
                 fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, c, 48) &&
                 fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, pk, 49) &&
                 fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, pk, 96) &&
                 fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, pk, 65) &&
                 fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, c, 33),
             "P-384 public key length / prefix mismatch should fail");

  /* invalid prefixes: identity (00), hybrid (06/07, RFC 5480), others */
  static const uint8_t bad_prefix[] =
      {0x00, 0x01, 0x04, 0x05, 0x06, 0x07, 0xFF};
  for (size_t i = 0; i < sizeof(bad_prefix); ++i) {
    FIO_MEMCPY(buf, c, 49);
    buf[0] = bad_prefix[i];
    FIO_ASSERT(fio___p384_point_decompress(&pt, buf) &&
                   fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, buf, 49),
               "P-384 compressed prefix 0x%02X should fail",
               bad_prefix[i]);
  }
  FIO_MEMCPY(buf, pk, 97);
  buf[0] = (uint8_t)(0x06 | (pk[96] & 1)); /* hybrid form, otherwise valid */
  FIO_ASSERT(fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, buf, 97),
             "P-384 hybrid public key (06/07 || x || y) should fail");
  FIO_MEMSET(buf, 0, sizeof(buf));
  FIO_ASSERT(fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, buf, 1) &&
                 fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, buf, 49) &&
                 fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, buf, 97),
             "P-384 point at infinity / zero public keys should fail");

  { /* x without a curve point (y^2 not a square), and non-canonical x >= p */
    uint32_t on = 0, off = 0;
    for (uint32_t k = 1; k < 256 && (!on || !off); ++k) {
      buf[0] = 0x02;
      fio___p384_test_x(buf + 1, zero, k);
      if (fio___p384_point_decompress(&pt, buf)) {
        off = off ? off : k;
      } else {
        on = on ? on : k;
        FIO_ASSERT(!fio___p384_test_on_curve(&pt),
                   "P-384 decompressed point should be on the curve");
      }
    }
    FIO_ASSERT(on && off, "P-384 test x values not found");
    buf[0] = 0x02;
    fio___p384_test_x(buf + 1, zero, off);
    FIO_ASSERT(fio_ecdsa_p384_verify(sig, sizeof(sig), nist_hash, buf, 49),
               "P-384 compressed x with no curve point should fail");
    fio___p384_test_x(buf + 1, FIO___P384_P, on); /* x + p, x valid */
    FIO_ASSERT(fio___p384_point_decompress(&pt, buf),
               "P-384 non-canonical compressed x (x + p) should fail");
    fio___p384_test_x(buf + 1, FIO___P384_P, 0);
    FIO_ASSERT(fio___p384_point_decompress(&pt, buf),
               "P-384 compressed x == p should fail");
  }
}

/* *****************************************************************************
P-384 Constant-Time Arithmetic (reduction, Barrett, ladder) vs References
***************************************************************************** */

/** Reference: t mod m via (variable-time) long division. */
FIO_SFUNC void fio___p384_test_ref_mod(uint64_t r[6],
                                       const uint64_t t[12],
                                       const uint64_t m[6]) {
  uint64_t m_ext[12] = {m[0], m[1], m[2], m[3], m[4], m[5], 0, 0, 0, 0, 0, 0};
  uint64_t rem[12] = {0};
  uint64_t tc[12];
  FIO_MEMCPY(tc, t, sizeof(tc));
  fio_math_div(NULL, rem, tc, m_ext, 12);
  FIO_MEMCPY(r, rem, 6 * sizeof(uint64_t));
}

/** Fills a 6-limb value: one of the edge patterns for `i`, else random. */
FIO_SFUNC void fio___p384_test_value(uint64_t v[6], size_t i) {
  FIO_MEMSET(v, 0, 48);
  switch (i) {
  case 0: /* p - 1 */
    FIO_MEMCPY(v, FIO___P384_P, 48);
    v[0] -= 1;
    return;
  case 1: /* n - 1 */
    FIO_MEMCPY(v, FIO___P384_N, 48);
    v[0] -= 1;
    return;
  case 2: /* 2^384 - 1 */
    FIO_MEMSET(v, 0xFF, 48);
    return;
  case 3: /* 0 */
    return;
  case 4: /* 1 */
    v[0] = 1;
    return;
  case 5: /* 2^383 */
    v[5] = 1ULL << 63;
    return;
  case 6: /* p */
    FIO_MEMCPY(v, FIO___P384_P, 48);
    return;
  case 7: /* n */
    FIO_MEMCPY(v, FIO___P384_N, 48);
    return;
  default: fio_rand_bytes(v, 48); return;
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, p384_ct_arith)(void) {
  /* Field multiplication (NIST reduction + branch-free carry folding) and
   * scalar multiplication (Barrett) vs long-division references. */
  for (size_t i = 0; i < 2000; ++i) {
    uint64_t a[6], b[6], t[12], ref[6], got[6];
    fio___p384_test_value(a, i < 64 ? (i >> 3) : 99);
    fio___p384_test_value(b, i < 64 ? (i & 7) : 99);
    if (i >= 64 && (i & 3) == 0) { /* high-bit heavy operands */
      a[5] |= 0xFFFFFFFF00000000ULL;
      b[5] |= 0xFFFFFFFF00000000ULL;
    }
    { /* field: operands must be < p (reduce them via the reference first) */
      uint64_t ta[12] = {0}, tb[12] = {0}, ar[6], br[6];
      FIO_MEMCPY(ta, a, 48);
      FIO_MEMCPY(tb, b, 48);
      fio___p384_test_ref_mod(ar, ta, FIO___P384_P);
      fio___p384_test_ref_mod(br, tb, FIO___P384_P);
      fio_math_mul(t, ar, br, 6);
      fio___p384_test_ref_mod(ref, t, FIO___P384_P);
      fio___p384_fe_mul(got, ar, br);
      FIO_ASSERT(!FIO_MEMCMP(ref, got, 48),
                 "P-384 field mul mismatch vs reference (case %zu)",
                 i);
    }
    /* scalar: Barrett reduction is valid for any 768-bit input */
    fio_math_mul(t, a, b, 6);
    fio___p384_test_ref_mod(ref, t, FIO___P384_N);
    fio___p384_scalar_reduce(got, t);
    FIO_ASSERT(!FIO_MEMCMP(ref, got, 48),
               "P-384 scalar reduce mismatch vs reference (case %zu)",
               i);
  }
  { /* fe_reduce on crafted 768-bit inputs whose first carry fold lands at
     * or above 2^384 (they need the second fold; found with a model of the
     * reduction), plus 2^768 - 1 */
    static const uint64_t fold2[][12] = {
        {0xFFFFFFFF00000001ULL, 0x00000000FFFFFFFEULL, 0x0000000200000003ULL,
         0xFFFFFFFDFFFFFFFFULL, 0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL,
         0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
         0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL},
        {0xFFFFFFFF00000002ULL, 0xFFFFFFF0FFFFFFFEULL, 0x0000000200000002ULL,
         0xFFFFFFFDFFFFFFFFULL, 0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL,
         0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
         0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL},
        {0x9667847ED8A80152ULL, 0xB063C9446B16D095ULL, 0x89047BC0EB32A4F1ULL,
         0x94AE4AE2FDA9EF93ULL, 0xCDAF8CAD738DF7AEULL, 0xD888B4514CBD80D9ULL,
         0xE09360236BD9FB4EULL, 0x75D48D064E3BB9ACULL, 0x6AD42452AAA2C3D9ULL,
         0x6BC3B91AF932A97FULL, 0xB1087EBFE2BED93FULL, 0xD849549FFF549B4BULL},
        {0x9667847ED8A80153ULL, 0xB063C9346B16D095ULL, 0x89047BC0EB32A4F1ULL,
         0x94AE4AE2FDA9EF93ULL, 0xCDAF8CAD738DF7AEULL, 0xD888B4514CBD80D9ULL,
         0xE09360236BD9FB4EULL, 0x75D48D064E3BB9ACULL, 0x6AD42452AAA2C3D9ULL,
         0x6BC3B91AF932A97FULL, 0xB1087EBFE2BED93FULL, 0xD849549FFF549B4BULL},
        {~0ULL, ~0ULL, ~0ULL, ~0ULL, ~0ULL, ~0ULL,
         ~0ULL, ~0ULL, ~0ULL, ~0ULL, ~0ULL, ~0ULL},
    };
    for (size_t i = 0; i < sizeof(fold2) / sizeof(fold2[0]); ++i) {
      uint64_t ref[6], got[6];
      fio___p384_test_ref_mod(ref, fold2[i], FIO___P384_P);
      fio___p384_fe_reduce(got, fold2[i]);
      FIO_ASSERT(!FIO_MEMCMP(ref, got, 48),
                 "P-384 fe_reduce crafted input %zu mismatch",
                 i);
    }
    for (size_t i = 0; i < 1000; ++i) { /* random 768-bit inputs */
      uint64_t t[12], ref[6], got[6];
      fio_rand_bytes(t, sizeof(t));
      fio___p384_test_ref_mod(ref, t, FIO___P384_P);
      fio___p384_fe_reduce(got, t);
      FIO_ASSERT(!FIO_MEMCMP(ref, got, 48), "P-384 fe_reduce random mismatch");
    }
  }
  { /* Barrett edges: t = 2^768 - 1, t = n^2, t = n * (n - 1) */
    uint64_t t[12], ref[6], got[6], nm1[6];
    FIO_MEMSET(t, 0xFF, sizeof(t));
    fio___p384_test_ref_mod(ref, t, FIO___P384_N);
    fio___p384_scalar_reduce(got, t);
    FIO_ASSERT(!FIO_MEMCMP(ref, got, 48), "P-384 Barrett 2^768-1 failed");
    fio_math_mul(t, FIO___P384_N, FIO___P384_N, 6);
    fio___p384_scalar_reduce(got, t);
    FIO_ASSERT(fio___p384_scalar_is_zero(got), "P-384 Barrett n^2 failed");
    FIO_MEMCPY(nm1, FIO___P384_N, 48);
    nm1[0] -= 1;
    fio_math_mul(t, FIO___P384_N, nm1, 6);
    fio___p384_scalar_reduce(got, t);
    FIO_ASSERT(fio___p384_scalar_is_zero(got), "P-384 Barrett n(n-1) failed");
  }
  for (size_t i = 0; i < 8; ++i) { /* scalar inversion: a * a^-1 == 1 */
    fio___p384_scalar_s a, inv, one;
    fio_rand_bytes(a, sizeof(a));
    a[5] &= 0x7FFFFFFFFFFFFFFFULL;
    a[0] |= 1;
    fio___p384_scalar_inv(inv, a);
    fio___p384_scalar_mul(one, a, inv);
    FIO_ASSERT(one[0] == 1 && !(one[1] | one[2] | one[3] | one[4] | one[5]),
               "P-384 scalar inversion failed");
  }
  for (size_t i = 0; i < 64; ++i) { /* secret-safe predicates vs public ones */
    fio___p384_scalar_s a;
    fio___p384_test_value(a, i < 8 ? i : 99);
    if (i >= 8 && (i & 1))
      a[5] = a[4] = a[3] = ~0ULL; /* near n */
    FIO_ASSERT(fio___p384_scalar_ct_gte_n(a) == fio___p384_scalar_gte_n(a),
               "P-384 ct_gte_n mismatch");
    FIO_ASSERT(fio___p384_scalar_ct_is_zero(a) == fio___p384_scalar_is_zero(a),
               "P-384 ct_is_zero mismatch");
    FIO_ASSERT((fio___p384_fe_ct_is_zero_mask(a) & 1) ==
                   (uint64_t)fio___p384_fe_is_zero(a),
               "P-384 fe_ct_is_zero_mask mismatch");
  }
}

/** Compares two Jacobian points by their affine form; 0 if equal. */
FIO_SFUNC int fio___p384_test_point_cmp(const fio___p384_point_jacobian_s *a,
                                        const fio___p384_point_jacobian_s *b) {
  const int ai = fio___p384_point_is_infinity(a);
  const int bi = fio___p384_point_is_infinity(b);
  if (ai || bi)
    return !(ai && bi);
  fio___p384_point_affine_s aa, ba;
  fio___p384_point_to_affine(&aa, a);
  fio___p384_point_to_affine(&ba, b);
  return fio___p384_fe_eq(aa.x, ba.x) | fio___p384_fe_eq(aa.y, ba.y);
}

/** k = floor(n / 2) + add */
FIO_SFUNC void fio___p384_test_half_n(fio___p384_scalar_s k, uint64_t add) {
  FIO_MEMCPY(k, FIO___P384_N, 48);
  for (int l = 0; l < 6; ++l)
    k[l] = (k[l] >> 1) | (l < 5 ? (k[l + 1] << 63) : 0);
  k[0] += add; /* n is odd: no carry for add <= 1 */
}

FIO_SFUNC void FIO_NAME_TEST(stl, p384_ladder)(void) {
  fio___p384_point_affine_s g, q;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);
  { /* a second base point Q = 7G */
    fio___p384_scalar_s seven = {7};
    fio___p384_point_jacobian_s j;
    fio___p384_point_mul_vartime(&j, seven, &g);
    fio___p384_point_to_affine(&q, &j);
  }
  for (size_t i = 0; i < 300; ++i) {
    fio___p384_scalar_s k = {0};
    switch (i) {
    case 0: k[0] = 1; break;
    case 1: k[0] = 2; break;
    case 2: k[0] = 3; break;
    case 3: /* n - 1 */
      FIO_MEMCPY(k, FIO___P384_N, 48);
      k[0] -= 1;
      break;
    case 4: /* n - 2 */
      FIO_MEMCPY(k, FIO___P384_N, 48);
      k[0] -= 2;
      break;
    case 5: k[5] = 1ULL << 63; break;           /* 2^383 */
    case 6: fio___p384_test_half_n(k, 0); break; /* (n-1)/2 */
    case 7: fio___p384_test_half_n(k, 1); break; /* (n+1)/2 */
    case 8: k[5] = k[4] = k[3] = ~0ULL; break;   /* top ones */
    case 9: k[0] = ~0ULL; break;
    case 10: k[1] = 1; break; /* 2^64 */
    case 11:
      k[3] = 0x8000000000000000ULL;
      k[0] = 1;
      break;
    case 12: /* alternating bits, high bit set (< n) */
      for (int l = 0; l < 6; ++l)
        k[l] = 0xAAAAAAAAAAAAAAAAULL;
      break;
    case 13:
      for (int l = 0; l < 6; ++l)
        k[l] = 0x5555555555555555ULL;
      break;
    default:
      fio_rand_bytes(k, sizeof(k));
      if (i & 1)
        k[5] |= 1ULL << 63; /* high-bit pattern */
      if (fio___p384_scalar_gte_n(k))
        k[5] >>= 1;
    }
    fio___p384_point_jacobian_s ct, vt;
    const fio___p384_point_affine_s *base = (i & 2) ? &q : &g;
    fio___p384_point_mul(&ct, k, base);
    fio___p384_point_mul_vartime(&vt, k, base);
    FIO_ASSERT(!fio___p384_test_point_cmp(&ct, &vt),
               "P-384 constant-time ladder != variable-time mul (case %zu)",
               i);
  }
  { /* k = 0 and k = n yield infinity */
    fio___p384_scalar_s k = {0};
    fio___p384_point_jacobian_s r;
    fio___p384_point_mul(&r, k, &g);
    FIO_ASSERT(fio___p384_point_is_infinity(&r), "P-384 ladder 0*G != O");
    FIO_MEMCPY(k, FIO___P384_N, 48);
    fio___p384_point_mul(&r, k, &g);
    FIO_ASSERT(fio___p384_point_is_infinity(&r), "P-384 ladder n*G != O");
  }
  { /* constant-time point ops agree with the variable-time ones */
    fio___p384_point_jacobian_s a, b, c, inf;
    fio___p384_point_affine_s nq;
    fio___p384_point_to_jacobian(&a, &q);
    fio___p384_point_double(&b, &a);
    fio___p384_point_double_ct(&c, &a);
    FIO_ASSERT(!fio___p384_test_point_cmp(&b, &c), "P-384 double_ct mismatch");
    fio___p384_point_add_mixed(&b, &b, &g); /* 14G + G: generic case */
    fio___p384_point_double_ct(&c, &a);
    fio___p384_point_add_mixed_ct(&c, &c, &g);
    FIO_ASSERT(!fio___p384_test_point_cmp(&b, &c), "P-384 add_ct mismatch");
    fio___p384_point_add_mixed_ct(&c, &a, &q); /* equal -> double */
    fio___p384_point_double(&b, &a);
    FIO_ASSERT(!fio___p384_test_point_cmp(&b, &c), "P-384 add_ct(P,P) failed");
    fio___p384_fe_copy(nq.x, q.x); /* opposite -> infinity */
    fio___p384_fe_neg(nq.y, q.y);
    fio___p384_point_add_mixed_ct(&c, &a, &nq);
    FIO_ASSERT(fio___p384_point_is_infinity(&c), "P-384 add_ct(P,-P) != O");
    fio___p384_point_set_infinity(&inf); /* infinity + q -> q */
    fio___p384_point_add_mixed_ct(&c, &inf, &q);
    FIO_ASSERT(!fio___p384_test_point_cmp(&a, &c), "P-384 add_ct(O,Q) failed");
    fio___p384_point_double_ct(&c, &inf);
    FIO_ASSERT(fio___p384_point_is_infinity(&c), "P-384 double_ct(O) != O");
  }
}

/* *****************************************************************************
P-384 Public Key Derivation (known vectors)
***************************************************************************** */

/** Hex string to bytes (test helper; asserts on bad input). */
FIO_SFUNC void fio___p384_test_hex(uint8_t *out, size_t len, const char *hex) {
  FIO_ASSERT(strlen(hex) == len * 2, "P-384 test hex length");
  for (size_t i = 0; i < len; ++i) {
    unsigned v = 0;
    for (size_t j = 0; j < 2; ++j) {
      const char c = hex[i * 2 + j];
      unsigned d = 0x100U;
      if (c >= '0' && c <= '9')
        d = (unsigned)(c - '0');
      else if (c >= 'a' && c <= 'f')
        d = (unsigned)(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F')
        d = (unsigned)(c - 'A' + 10);
      FIO_ASSERT(d < 16, "P-384 test hex digit");
      v = (v << 4) | d;
    }
    out[i] = (uint8_t)v;
  }
}

/* RFC 6979 A.2.6 private key (its public key is nist_pub) */
static const char *fio___p384_test_rfc_d =
    "6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA"
    "9AA47740787137D896D5724E4C70A825F872C9EA60D2EDF5";

FIO_SFUNC void FIO_NAME_TEST(stl, p384_public_key)(void) {
  static const struct {
    const char *d, *x, *y;
  } vec[] = {
      /* 3G (NIST point multiplication vector) */
      {"000000000000000000000000000000000000000000000000"
       "000000000000000000000000000000000000000000000003",
       "077A41D4606FFA1464793C7E5FDC7D98CB9D3910202DCD06"
       "BEA4F240D3566DA6B408BBAE5026580D02D7E5C70500C831",
       "C995F7CA0B0C42837D0BBE9602A9FC998520B41C85115AA5"
       "F7684C0EDC111EACC24ABD6BE4B5D298B65F28600A2F1DF1"},
      /* (n-1)G = -G */
      {"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
       "C7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52972",
       "AA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B98"
       "59F741E082542A385502F25DBF55296C3A545E3872760AB7",
       "C9E821B569D9D390A26167406D6D23D6070BE242D765EB83"
       "1625CEEC4A0F473EF59F4E30E2817E6285BCE2846F15F1A0"},
      /* 2^383 G (computed with an independent affine-arithmetic model) */
      {"800000000000000000000000000000000000000000000000"
       "000000000000000000000000000000000000000000000000",
       "E3E25CDB160208B6474E2B34D72BF586BBA14F72C3F97F51"
       "5A405D1429196E6673161B78AD80AFE664EE504D4B161AB7",
       "3770B64D5442695959FB89DA7EB3A7CAFCBA079D32031DF6"
       "213049C1CC509E3F9120CAF8DD9109115F403859AC337ACC"},
  };
  uint8_t d[48], pub[97], want[97];
  fio___p384_test_hex(d, 48, fio___p384_test_rfc_d);
  FIO_ASSERT(!fio_p384_public_key(pub, d) && !FIO_MEMCMP(pub, nist_pub, 97),
             "P-384 public key derivation (RFC 6979 A.2.6) failed");
  for (size_t i = 0; i < sizeof(vec) / sizeof(vec[0]); ++i) {
    fio___p384_test_hex(d, 48, vec[i].d);
    want[0] = 0x04;
    fio___p384_test_hex(want + 1, 48, vec[i].x);
    fio___p384_test_hex(want + 49, 48, vec[i].y);
    FIO_ASSERT(!fio_p384_public_key(pub, d) && !FIO_MEMCMP(pub, want, 97),
               "P-384 public key derivation vector %zu failed",
               i);
  }
  { /* invalid secret keys: 0, n, n + 1, 2^384 - 1 (public_key zeroed) */
    uint8_t bad[4][48];
    FIO_MEMSET(bad[0], 0, 48);
    fio___p384_fe_to_bytes(bad[1], FIO___P384_N);
    FIO_MEMCPY(bad[2], bad[1], 48);
    bad[2][47] += 1;
    FIO_MEMSET(bad[3], 0xFF, 48);
    for (size_t i = 0; i < 4; ++i) {
      FIO_MEMSET(pub, 0xAA, sizeof(pub));
      FIO_ASSERT(fio_p384_public_key(pub, bad[i]) == -1,
                 "P-384 public key should reject invalid scalar %zu",
                 i);
      for (size_t j = 0; j < sizeof(pub); ++j)
        FIO_ASSERT(!pub[j], "P-384 public key not zeroed on failure");
    }
    FIO_ASSERT(fio_p384_public_key(NULL, d) == -1,
               "P-384 public key NULL output should fail");
    FIO_MEMSET(pub, 0xAA, sizeof(pub));
    FIO_ASSERT(fio_p384_public_key(pub, NULL) == -1 && !pub[0] && !pub[96],
               "P-384 public key NULL secret should fail (zeroed)");
  }
  { /* keypair: valid scalar, consistent public key */
    uint8_t sk[48], pk[97], pk2[97];
    FIO_ASSERT(!fio_p384_keypair(sk, pk), "P-384 keypair failed");
    FIO_ASSERT(!fio_p384_public_key(pk2, sk) && !FIO_MEMCMP(pk, pk2, 97),
               "P-384 keypair public key mismatch");
    FIO_ASSERT(fio_p384_keypair(NULL, pk) == -1 &&
                   fio_p384_keypair(sk, NULL) == -1,
               "P-384 keypair NULL arguments should fail");
  }
}

/* *****************************************************************************
P-384 ECDSA Signing
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384_sign)(void) {
  uint8_t d[48], sig[104], comp[49], h[48];
  size_t sig_len = 0;
  fio___p384_test_hex(d, 48, fio___p384_test_rfc_d);
  fio___p384_test_compress(comp, nist_pub);

  { /* RFC 6979 A.2.6, SHA-384, message "test": verifies with the key */
    uint8_t r[48], s[48];
    fio_u512 th = fio_sha384("test", 4);
    fio___p384_test_hex(r,
                        48,
                        "8203B63D3C853E8D77227FB377BCF7B7B772E97892A80F36"
                        "AB775D509D7A5FEB0542A7F0812998DA8F1DD3CA3CF023DB");
    fio___p384_test_hex(s,
                        48,
                        "DDD0760448D42D8A43AF45AF836FCE4DE8BE06B485E9B61B"
                        "827C2F13173923E06A739F040649A667BF3B828246BAA5A5");
    FIO_ASSERT(!fio_ecdsa_p384_verify_raw(r,
                                          s,
                                          th.u8,
                                          nist_pub + 1,
                                          nist_pub + 49),
               "P-384 RFC 6979 'test' vector failed");
  }

  /* round trips with the RFC key over varying hashes, both key forms */
  FIO_MEMCPY(h, nist_hash, 48);
  for (size_t i = 0; i < 16; ++i) {
    FIO_ASSERT(!fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, d),
               "P-384 sign failed");
    FIO_ASSERT(sig_len >= 8 && sig_len <= 104 && sig[0] == 0x30 &&
                   sig[1] == sig_len - 2,
               "P-384 signature DER framing invalid");
    FIO_ASSERT(!fio_ecdsa_p384_verify(sig, sig_len, h, nist_pub, 97),
               "P-384 sign/verify round trip failed (uncompressed key)");
    FIO_ASSERT(!fio_ecdsa_p384_verify(sig, sig_len, h, comp, 49),
               "P-384 sign/verify round trip failed (compressed key)");
    h[i] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p384_verify(sig, sig_len, h, nist_pub, 97),
               "P-384 signature verified against a different hash");
    h[i] ^= 0x01;
    sig[sig_len - 1] ^= 0x01;
    FIO_ASSERT(fio_ecdsa_p384_verify(sig, sig_len, h, nist_pub, 97),
               "P-384 tampered signature verified");
    h[(i * 7) % 48] += (uint8_t)(i + 1);
  }
  /* hash >= n is reduced (all-0xFF digest) */
  FIO_MEMSET(h, 0xFF, 48);
  FIO_ASSERT(!fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, d) &&
                 !fio_ecdsa_p384_verify(sig, sig_len, h, nist_pub, 97),
             "P-384 sign/verify with digest >= n failed");
  { /* random nonces: two signatures of the same hash differ */
    uint8_t sig2[104];
    size_t sig2_len = 0;
    FIO_ASSERT(!fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, d) &&
                   !fio_ecdsa_p384_sign(sig2, &sig2_len, sizeof(sig2), h, d),
               "P-384 sign failed");
    FIO_ASSERT(sig_len != sig2_len || FIO_MEMCMP(sig, sig2, sig_len),
               "P-384 signatures should use fresh nonces");
  }
  for (size_t i = 0; i < 8; ++i) { /* random keypairs, both key encodings */
    uint8_t sk[48], pk[97];
    fio_rand_bytes(h, 48);
    FIO_ASSERT(!fio_p384_keypair(sk, pk), "P-384 keypair failed");
    fio___p384_test_compress(comp, pk);
    FIO_ASSERT(!fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, sk) &&
                   !fio_ecdsa_p384_verify(sig, sig_len, h, pk, 97) &&
                   !fio_ecdsa_p384_verify(sig, sig_len, h, comp, 49),
               "P-384 random keypair sign/verify failed");
    comp[0] ^= 1; /* -Q */
    FIO_ASSERT(fio_ecdsa_p384_verify(sig, sig_len, h, comp, 49),
               "P-384 signature verified with the negated key");
  }
  { /* invalid inputs: bad scalars, small buffer, NULLs */
    uint8_t bad[48], pk[97];
    FIO_MEMSET(bad, 0, 48);
    FIO_ASSERT(fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, bad) == -1,
               "P-384 sign should reject d = 0");
    fio___p384_fe_to_bytes(bad, FIO___P384_N);
    FIO_ASSERT(fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, bad) == -1,
               "P-384 sign should reject d = n");
    bad[47] += 1;
    FIO_ASSERT(fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, bad) == -1,
               "P-384 sign should reject d = n + 1");
    FIO_MEMSET(bad, 0xFF, 48);
    FIO_ASSERT(fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, bad) == -1,
               "P-384 sign should reject d = 2^384 - 1");
    fio___p384_fe_to_bytes(bad, FIO___P384_N);
    bad[47] -= 1; /* n - 1 is valid */
    FIO_ASSERT(!fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, bad) &&
                   !fio_p384_public_key(pk, bad) &&
                   !fio_ecdsa_p384_verify(sig, sig_len, h, pk, 97),
               "P-384 d = n - 1 should sign and verify");
    FIO_ASSERT(fio_ecdsa_p384_sign(sig, &sig_len, 103, h, d) == -1,
               "P-384 sign should reject a buffer < 104 bytes");
    FIO_ASSERT(fio_ecdsa_p384_sign(NULL, &sig_len, sizeof(sig), h, d) == -1 &&
                   fio_ecdsa_p384_sign(sig, NULL, sizeof(sig), h, d) == -1 &&
                   fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), NULL, d) ==
                       -1 &&
                   fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, NULL) ==
                       -1,
               "P-384 sign should reject NULL arguments");
  }
}

/* *****************************************************************************
P-384 OpenSSL Cross-Validation (compressed public keys)
***************************************************************************** */

#if HAVE_OPENSSL
FIO_SFUNC void FIO_NAME_TEST(stl, p384_openssl_compressed)(void) {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  const uint8_t msg[] = "P-384 OpenSSL compressed key test";
  fio_u512 h512 = fio_sha384(msg, sizeof(msg) - 1);
  for (int i = 0; i < 8; ++i) {
    uint8_t pk[97], comp[49], ours[49];
    unsigned char sig[112];
    unsigned int sig_len = sizeof(sig);
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp384r1);
    FIO_ASSERT(key && EC_KEY_generate_key(key) == 1,
               "OpenSSL P-384 key generation failed");
    const EC_GROUP *group = EC_KEY_get0_group(key);
    const EC_POINT *point = EC_KEY_get0_public_key(key);
    FIO_ASSERT(EC_POINT_point2oct(group,
                                  point,
                                  POINT_CONVERSION_UNCOMPRESSED,
                                  pk,
                                  sizeof(pk),
                                  NULL) == 97 &&
                   EC_POINT_point2oct(group,
                                      point,
                                      POINT_CONVERSION_COMPRESSED,
                                      comp,
                                      sizeof(comp),
                                      NULL) == 49,
               "OpenSSL P-384 point encoding failed");
    fio___p384_test_compress(ours, pk);
    FIO_ASSERT(!FIO_MEMCMP(comp, ours, 49),
               "P-384 compressed encoding should match OpenSSL's");
    FIO_ASSERT(ECDSA_sign(0, h512.u8, 48, sig, &sig_len, key) == 1,
               "OpenSSL P-384 ECDSA_sign failed");
    FIO_ASSERT(!fio_ecdsa_p384_verify(sig, sig_len, h512.u8, comp, 49),
               "P-384 should verify an OpenSSL signature (compressed key)");
    FIO_ASSERT(!fio_ecdsa_p384_verify(sig, sig_len, h512.u8, pk, 97),
               "P-384 should verify an OpenSSL signature (uncompressed key)");
    comp[0] ^= 1;
    FIO_ASSERT(fio_ecdsa_p384_verify(sig, sig_len, h512.u8, comp, 49),
               "P-384 OpenSSL signature with the negated key should fail");
    EC_KEY_free(key);
    h512.u8[i] ^= 0x5A;
  }
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

/* *****************************************************************************
P-384 OpenSSL Cross-Validation (signing / key derivation)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384_openssl_sign)(void) {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  for (int i = 0; i < 8; ++i) {
    uint8_t sk[48], pk[97], comp[49], ossl_pk[97], sig[104];
    size_t sig_len = 0;
    uint8_t h[48];
    fio_rand_bytes(h, sizeof(h));

    /* OpenSSL verifies our signature; EC_KEY_check_key proves pk == d*G */
    FIO_ASSERT(!fio_p384_keypair(sk, pk), "P-384 keypair failed");
    FIO_ASSERT(!fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, sk),
               "P-384 sign failed");
    {
      EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp384r1);
      FIO_ASSERT(key, "OpenSSL EC_KEY_new_by_curve_name failed");
      const EC_GROUP *group = EC_KEY_get0_group(key);
      EC_POINT *pt = EC_POINT_new(group);
      BIGNUM *d = BN_bin2bn(sk, 48, NULL);
      FIO_ASSERT(pt && d && EC_POINT_oct2point(group, pt, pk, 97, NULL) == 1,
                 "OpenSSL failed to load our P-384 public key");
      FIO_ASSERT(EC_KEY_set_private_key(key, d) == 1 &&
                     EC_KEY_set_public_key(key, pt) == 1 &&
                     EC_KEY_check_key(key) == 1,
                 "OpenSSL: our P-384 public key doesn't match the secret key");
      FIO_ASSERT(ECDSA_verify(0, h, 48, sig, (int)sig_len, key) == 1,
                 "OpenSSL failed to verify our P-384 signature");
      h[0] ^= 1;
      FIO_ASSERT(ECDSA_verify(0, h, 48, sig, (int)sig_len, key) != 1,
                 "OpenSSL verified our P-384 signature for a wrong hash");
      h[0] ^= 1;
      BN_free(d);
      EC_POINT_free(pt);
      EC_KEY_free(key);
    }

    /* OpenSSL key: we derive the same public key and verify its signature */
    {
      unsigned char osig[112];
      unsigned int osig_len = sizeof(osig);
      EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp384r1);
      FIO_ASSERT(key && EC_KEY_generate_key(key) == 1,
                 "OpenSSL P-384 key generation failed");
      const EC_GROUP *group = EC_KEY_get0_group(key);
      FIO_ASSERT(BN_bn2binpad(EC_KEY_get0_private_key(key), sk, 48) == 48 &&
                     EC_POINT_point2oct(group,
                                        EC_KEY_get0_public_key(key),
                                        POINT_CONVERSION_UNCOMPRESSED,
                                        ossl_pk,
                                        sizeof(ossl_pk),
                                        NULL) == 97,
                 "OpenSSL P-384 key export failed");
      FIO_ASSERT(!fio_p384_public_key(pk, sk) && !FIO_MEMCMP(pk, ossl_pk, 97),
                 "P-384 public key derivation differs from OpenSSL");
      FIO_ASSERT(ECDSA_sign(0, h, 48, osig, &osig_len, key) == 1,
                 "OpenSSL P-384 ECDSA_sign failed");
      fio___p384_test_compress(comp, pk);
      FIO_ASSERT(!fio_ecdsa_p384_verify(osig, osig_len, h, pk, 97) &&
                     !fio_ecdsa_p384_verify(osig, osig_len, h, comp, 49),
                 "P-384 should verify OpenSSL's signature");
      /* our signature with OpenSSL's key, verified by OpenSSL */
      FIO_ASSERT(!fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), h, sk) &&
                     ECDSA_verify(0, h, 48, sig, (int)sig_len, key) == 1,
                 "OpenSSL failed to verify our signature (OpenSSL key)");
      EC_KEY_free(key);
    }
  }
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}
#endif /* HAVE_OPENSSL */

/* *****************************************************************************
Documentation examples (154 p384.md) - kept compiling and running
***************************************************************************** */

FIO_SFUNC int p384_doc_sign_and_verify(const void *msg, size_t len) {
  uint8_t secret_key[48], public_key[97], sig[104];
  size_t sig_len = 0;
  fio_u512 hash = fio_sha384(msg, len);
  int r = -1;
  if (fio_p384_keypair(secret_key, public_key))
    return -1;
  if (!fio_ecdsa_p384_sign(sig, &sig_len, sizeof(sig), hash.u8, secret_key))
    r = fio_ecdsa_p384_verify(sig, sig_len, hash.u8, public_key, 97);
  fio_secure_zero(secret_key, sizeof(secret_key));
  return r;
}

FIO_SFUNC int p384_doc_verify_cert_sig(const uint8_t *sig,
                                       size_t sig_len,
                                       const uint8_t hash[48],
                                       const uint8_t pubkey[97]) {
  if (fio_ecdsa_p384_verify(sig, sig_len, hash, pubkey, 97))
    return -1;
  return 0;
}

FIO_SFUNC void FIO_NAME_TEST(stl, p384_doc_example)(void) {
  FIO_ASSERT(!p384_doc_sign_and_verify("facil.io", 8),
             "documentation sign_and_verify example failed");
  uint8_t sk[48], pk[97], sig[104], hash[48];
  size_t sig_len = 0;
  FIO_ASSERT(!fio_p384_keypair(sk, pk) &&
                 !fio_ecdsa_p384_sign(sig,
                                      &sig_len,
                                      sizeof(sig),
                                      fio_sha384("facil.io", 8).u8,
                                      sk),
             "documentation example setup failed");
  FIO_MEMCPY(hash, fio_sha384("facil.io", 8).u8, 48);
  FIO_ASSERT(!p384_doc_verify_cert_sig(sig, sig_len, hash, pk),
             "documentation verify_cert_sig example failed");
  hash[0] ^= 1;
  FIO_ASSERT(p384_doc_verify_cert_sig(sig, sig_len, hash, pk),
             "documentation verify_cert_sig should reject another hash");
  fio_secure_zero(sk, sizeof(sk));
  fprintf(stderr, "* P-384 documentation examples: OK\n");
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  FIO_NAME_TEST(stl, p384_constants)();
  FIO_NAME_TEST(stl, p384_field_ops)();
  FIO_NAME_TEST(stl, p384_base_point)();
  FIO_NAME_TEST(stl, p384_scalar_mul)();
  FIO_NAME_TEST(stl, p384_ecdsa_nist)();
  FIO_NAME_TEST(stl, p384_der_parsing)();
  FIO_NAME_TEST(stl, p384_infinity)();
  FIO_NAME_TEST(stl, p384_invalid_sigs)();
  FIO_NAME_TEST(stl, p384_compressed_verify)();
  FIO_NAME_TEST(stl, p384_ct_arith)();
  FIO_NAME_TEST(stl, p384_ladder)();
  FIO_NAME_TEST(stl, p384_public_key)();
  FIO_NAME_TEST(stl, p384_sign)();
  FIO_NAME_TEST(stl, p384_doc_example)();
#if HAVE_OPENSSL
  FIO_NAME_TEST(stl, p384_openssl_compressed)();
  FIO_NAME_TEST(stl, p384_openssl_sign)();
#endif
  return 0;
}
